/*
 * meg4/comp_bas.c
 *
 * Copyright (C) 2023 bzt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @brief Compiler specific functions for the BASIC language
 *
 */

#include <errno.h>
#include <math.h>

#include "meg4.h"

#ifndef NOEDITORS
#include "editors/editors.h"    /* for the highlighter token defines */
#include "api.h"

/* must match the keywords in misc/hl.json */
enum { BAS_DIM, BAS_READ, BAS_RESTORE, BAS_DATA, BAS_SUB, BAS_FUNC, BAS_END, BAS_ON, BAS_GOTO, BAS_GOSUB, BAS_RETURN, BAS_FOR,
    BAS_TO, BAS_STEP, BAS_NEXT, BAS_IF, BAS_THEN, BAS_ELIF, BAS_ELSE, BAS_INPUT, BAS_PRINT, BAS_PEEK, BAS_POKE, BAS_LET, BAS_DEBUG };

/* operators in precedence order, must match O_ defines in cpu.h */
char *bas_ops[] = {
    /* assignments */   "", "", "", "", "", "", "", "", "", "", "", "",
    /* logical */       "", "or", "and",
    /* bitwise */       "", "", "@",
    /* conditionals */  "=", "<>", "<", ">", "<=", ">=",
    /* arithmetic */    "", "", "+", "-", "*", "/", "mod", "^",
    /* unaries */       "!", "", "", "",
    /* field select */  "", ""
};

/* start of formating strings */
static int fmt = 0;

/**
 * Get identifier type from the last byte of the name
 */
static int gettype(tok_t *tok)
{
    switch(meg4.src[tok->pos + tok->len - 1]) {
        case '$': return T_STR;
        case '%': return T_I32;
        case '#': return T_U16;
        case '!': return T_U8;
        default: return T_FLOAT;
    }
}

/**
 * Get argument types
 */
static int getargs(compiler_t *comp, int s, int e, int *args)
{
    int i, ret = 0, t;
    for(i = s; i < e; i++) {
        if(ret >= N_ARG - 1) { code_error(comp->tok[i].pos, lang[ERR_NUMARG]); return -1; }
        if(comp->tok[i].type == HL_V) { t = gettype(&comp->tok[i]); args[ret++] = t == T_STR ? T(T_PTR, T_I8) : T(T_SCALAR, t); } else
        if(comp->tok[i].type != HL_D || comp->tok[i].id != (i + 1 < e ? ',' : ')'))
            { code_error(comp->tok[i].pos, lang[ERR_SYNTAX]); return -1; }
    }
    return ret;
}

/**
 * Find the boundaries of a statement
 */
static int getst(compiler_t *comp, int s, int *e, int l)
{
    if(l > comp->ntok) l = comp->ntok;
    *e = l;
    while(s < l && comp->tok[s].type == HL_D && meg4.src[comp->tok[s].pos] == '\n') s++;
    if(s >= l) return s;
    for(*e = s; *e < l && !(comp->tok[*e].type == HL_D && meg4.src[comp->tok[*e].pos] == '\n'); (*e)++);
    return s;
}

/**
 * Parse a statement
 */
static int statement(compiler_t *comp, int s, int e)
{
    tok_t *tok = comp->tok;
    int i, j, k, l, m, n, o = s, p, l1, l2, end;
    float f;

    if(e > comp->ntok) e = comp->ntok;
    for(; s < e; ) {
        s = getst(comp, s, &l, e);
        if(s >= e || (tok[s].type == HL_K && tok[s].id == BAS_END && tok[s + 1].type == HL_K &&
          (tok[s + 1].id == BAS_SUB || tok[s + 1].id == BAS_FUNC))) break;
        /* skip over labels */
        if(tok[s].type == HL_V && s + 1 < l && tok[s + 1].type == HL_D && tok[s + 1].id == ':') {
            if(o && !comp_addlbl(comp, s)) return 0;
            s += 2;
        }
        /* skip "LET" keyword, "DATA" and other special statement lines that do not need further parsing */
        if(tok[s].type == HL_K) {
            switch(tok[s].id) {
                case BAS_LET: s++; break;
                case BAS_DIM:
                case BAS_DATA: if(o) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; } s = l; continue;
                case BAS_SUB: case BAS_FUNC:
                    /* if we're parsing the root, we have to skip subroutines and functions */
                    if(o) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                    for(m = tok[s].id, s = l; s < e; s = l) {
                        s = getst(comp, s, &l, e);
                        if(s >= e || (tok[s].type == HL_K && tok[s].id == BAS_END && tok[s + 1].type == HL_K &&
                            tok[s + 1].id == m)) break;
                    }
                    s = l;
                    if(s >= e) return s;
                    continue;
                break;
            }
        }
        /* assignment */
        if(tok[s].type == HL_V || tok[s].type == HL_F) {
            for(i = s + 1; i < l && meg4.src[tok[i].pos] != '='; i++);
            if(tok[i].len != 1 || meg4.src[tok[i].pos] != '=' || (j = comp_findid(comp, &tok[s])) < MEG4_NUM_API + MEG4_NUM_BDEF ||
              (comp->id[j].t & 15) < T_SCALAR) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
            k = comp->id[j].t;
            comp_cdbg(comp, s);
            end = 0;
            if(!comp_expr(comp, s, i, &k, &end, O_CND)) return 0;
            j = comp->code[--comp->nc] & 0xff;
            if(j < BC_LDB || j > BC_LDF || end) { code_error(tok[s].pos, lang[ERR_BADLVAL]); return 0; }
            comp_push(comp, T(T_SCALAR, T_I32));
            m = k;
            i++;
            if(!(s = comp_expr(comp, i, l, &m, &end, O_CND))) return 0;
            if(k != m) { code_error(tok[i].pos, lang[ERR_BADARG]); return 0; }
            comp_resolve(comp, end);
            comp_store(comp, k);
        } else
        /* other statements */
        if(tok[s].type == HL_K) {
            comp->ls = tok[s].id;
            switch(tok[s].id) {
                case BAS_DEBUG:
                    s++; while(s < l && comp->tok[s].type == HL_D && meg4.src[comp->tok[s].pos] == '\n') s++;
                    comp_gen(comp, BC_DEBUG); comp_cdbg(comp, s);
                break;
                case BAS_END:
                    if(s + 1 != l) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    comp_gen(comp, (MEG4_EXIT << 8) | BC_SCALL);
                break;
                case BAS_RETURN:
                    i = s++;
                    k = m = (comp->id[comp->f[comp->cf].id].t & ~0xff) | (comp->id[comp->f[comp->cf].id].t == T_FUNC ? T_SCALAR : T_PTR);
                    if(m != T(T_SCALAR, T_VOID)) {
                        if(s >= l) { code_error(tok[i].pos, lang[ERR_NUMARG]); return 0; }
                        end = 0;
                        if(!(s = comp_expr(comp, s, l, &m, &end, O_CND))) return 0;
                        if(k != m) { code_error(tok[i + 1].pos, lang[ERR_BADARG]); return 0; }
                        comp_resolve(comp, end);
                    }
                    if(s != l) { code_error(tok[i].pos, lang[ERR_SYNTAX]); return 0; }
                    comp_gen(comp, BC_RET);
                break;
                case BAS_GOTO:
                    comp_gen(comp, BC_JMP);
                    if(!comp_addjmp(comp, s + 1)) return 0;
                break;
                case BAS_GOSUB:
                    s++;
                    if(s >= l || tok[s].type != HL_V || (j = comp_findid(comp, &tok[s])) < 0 ||
                      comp->id[j].t != T(T_FUNC, T_VOID)) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    end = 0;
                    if(!(s = comp_expr(comp, s, l, &m, &end, O_CND))) return 0;
                    comp_resolve(comp, end);
                break;
                case BAS_RESTORE:
                    comp_gen(comp, BC_PSHCI);
                    comp_gen(comp, 4 + MEG4_MEM_USER);
                    comp_gen(comp, BC_CI);
                    comp_gen(comp, 0);
                    comp_gen(comp, BC_STI);
                break;
                case BAS_READ:
                    for(s++; s < l; ) {
                        if(tok[s].type == HL_V || tok[s].type == HL_F) {
                            if((j = comp_findid(comp, &tok[s])) < MEG4_NUM_API + MEG4_NUM_BDEF || (comp->id[j].t & 15) < T_SCALAR) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                            k = s; m = comp->id[j].t;
                            comp_cdbg(comp, s);
                            end = 0;
                            if(!(s = comp_expr(comp, s, l, &m, &end, O_CND))) return 0;
                            j = comp->code[--comp->nc] & 0xff;
                            if(j < BC_LDB || j > BC_LDF || end) { code_error(tok[s].pos, lang[ERR_BADLVAL]); return 0; }
                            comp_gen(comp, j - BC_LDB + BC_RDB);
                        } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                        if(tok[s].type == HL_O && tok[s].id == ',') s++;
                    }
                break;
                case BAS_POKE:
                    /* we have to push the two arguments in reverse order */
                    for(i = s + 1, p = 0; i < l; i++) {
                        if(meg4.src[tok[i].pos] == '(') p++; else
                        if(meg4.src[tok[i].pos] == ')') { p--; if(p < 0) { code_error(tok[i].pos, lang[ERR_NOOPEN]); return 0; } } else
                        if(meg4.src[tok[i].pos] == ',' && !p) break;
                    }
                    if(i >= l) { code_error(tok[s].pos, lang[ERR_NUMARG]); return 0; }
                    /* value */
                    comp_cdbg(comp, i + 1); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, i + 1, l, &m, &end, O_CND)) return 0;
                    comp_resolve(comp, end);
                    comp_push(comp, T(T_SCALAR, T_I32));
                    /* address */
                    comp_cdbg(comp, s + 1); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, s + 1, i, &m, &end, O_CND)) return 0;
                    comp_resolve(comp, end);
                    comp_push(comp, T(T_SCALAR, T_I32));
                    comp_gen(comp, (MEG4_OUTB << 8) | BC_SCALL);
                    comp_gen(comp, (8 << 8) | BC_SP);
                break;
                case BAS_PRINT:
                    for(i = ++s, p = 0; i <= l; i++) {
                        if(meg4.src[tok[i].pos] == '(') p++; else
                        if(meg4.src[tok[i].pos] == ')') { p--; if(p < 0) { code_error(tok[i].pos, lang[ERR_NOOPEN]); return 0; } } else
                        if((meg4.src[tok[i].pos] == ',' && !p) || meg4.src[tok[i].pos] == ';' || meg4.src[tok[i].pos] == '\n') {
                            /* push the expression */
                            comp_cdbg(comp, s); m = T(T_SCALAR, T_FLOAT); end = 0;
                            if(!comp_expr(comp, s, i, &m, &end, O_CND)) return 0;
                            comp_resolve(comp, end);
                            comp_push(comp, T(T_SCALAR, T_I32));
                            /* push an appropriate formating string */
                            if(m == T(T_PTR, T_I8) || m == T(T_SCALAR, T_STR)) k = fmt; else
                            if((m & 15) != T_SCALAR) { code_error(tok[s].pos, lang[ERR_BADARG]); return 0; } else
                            if((m >> 4) == T_FLOAT) k = fmt + 6; else k = fmt + 3;
                            if(meg4.src[tok[i].pos] == ',') k++; else
                            if(meg4.src[tok[i].pos] == ';') k += 2;
                            comp_gen(comp, BC_PSHCI);
                            comp_gen(comp, comp->str[k].n);
                            /* call printf */
                            comp_gen(comp, (MEG4_PRINT << 8) | BC_SCALL);
                            comp_gen(comp, (8 << 8) | BC_SP);
                            s = i + 1;
                        }
                    }
                break;
                case BAS_INPUT:
                    for(i = s + 1, p = 0; i < l; i++) {
                        if(meg4.src[tok[i].pos] == '(') p++; else
                        if(meg4.src[tok[i].pos] == ')') { p--; if(p < 0) { code_error(tok[i].pos, lang[ERR_NOOPEN]); return 0; } } else
                        if((meg4.src[tok[i].pos] == ',' && !p) || meg4.src[tok[i].pos] == ';') break;
                    }
                    if(i >= l) { code_error(tok[s].pos, lang[ERR_NUMARG]); return 0; }
                    if(i > s + 1) {
                        /* push the expression */
                        comp_cdbg(comp, s + 1); m = T(T_SCALAR, T_FLOAT); end = 0;
                        if(!comp_expr(comp, s + 1, i, &m, &end, O_CND)) return 0;
                        if(end || (m != T(T_PTR, T_I8) && m != T(T_SCALAR, T_STR))) { code_error(tok[s + 1].pos, lang[ERR_BADARG]); return 0; }
                        comp_push(comp, T(T_SCALAR, T_I32));
                        /* push an appropriate formating string */
                        comp_gen(comp, BC_PSHCI);
                        comp_gen(comp, comp->str[fmt + (meg4.src[tok[i].pos] == ',' ? 9 : 2)].n);
                        /* call printf */
                        comp_gen(comp, (MEG4_PRINT << 8) | BC_SCALL);
                        comp_gen(comp, (8 << 8) | BC_SP);
                    }
                    /* get the line */
                    comp_gen(comp, (MEG4_INPUT << 8) | BC_SCALL);
                    comp_gen(comp, BC_PSHCI);
                    comp_gen(comp, 255);
                    comp_gen(comp, BC_PSHCI);
                    comp_gen(comp, MEG4_MEM_LIMIT - 256);
                    i++;
                    comp_cdbg(comp, i); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, i, l, &m, &end, O_CND)) return 0;
                    j = comp->code[--comp->nc] & 0xff;
                    if(j < BC_LDB || j > BC_LDF || end) { code_error(tok[i].pos, lang[ERR_BADARG]); return 0; }
                    comp_push(comp, T(T_SCALAR, T_I32));
                    /* convert the result */
                    if(m == T(T_PTR, T_I8) || m == T(T_SCALAR, T_STR)) {
                        /* string, just copy to the destination */
                        comp_gen(comp, (MEG4_MEMCPY << 8) | BC_SCALL);
                        comp_gen(comp, (12 << 8) | BC_SP);
                    } else
                    if((m & 15) != T_SCALAR) { code_error(tok[i].pos, lang[ERR_BADARG]); return 0; } else {
                        /* number */
                        comp_gen(comp, BC_PSHCI);
                        comp_gen(comp, MEG4_MEM_LIMIT - 256);
                        comp_gen(comp, (((m >> 4) == T_FLOAT ? MEG4_VAL : MEG4_ATOI) << 8) | BC_SCALL);
                        comp_gen(comp, (4 << 8) | BC_SP);
                        comp_gen(comp, j - BC_LDB + BC_STB);
                        comp_gen(comp, (8 << 8) | BC_SP);
                    }
                break;
                case BAS_ON:
                    for(i = s + 1; i < l && tok[i].type != HL_K; i++);
                    if(i >= l || tok[i].type != HL_K || tok[i].id != BAS_GOTO) { code_error(tok[i >= l ? s : i].pos, lang[ERR_SYNTAX]); return 0; }
                    comp_cdbg(comp, s + 1); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, s + 1, i, &m, &end, O_CND)) return 0;
                    comp_resolve(comp, end);
                    for(s = ++i, p = 0; i < l && tok[i].type == HL_V; ) {
                        p++; i++;
                        if(!((tok[i].type == HL_K && tok[i].id == BAS_ELSE) ||
                          (tok[i].type == HL_D && tok[i].id == ','))) { code_error(tok[i].pos, lang[ERR_SYNTAX]); return 0; }
                        if(tok[i].type == HL_D) i++;
                    }
                    if(p < 1) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    comp_gen(comp, (p << 8) | BC_SW); comp_gen(comp, 1); p = comp->nc;
                    if(tok[i].type == HL_K) { if(!comp_addjmp(comp, i + 1)) return 0; }
                    else comp_gen(comp, 0);
                    for(i = s; i < l && tok[i].type != HL_K; i++)
                        if(tok[i].type == HL_V) { if(!comp_addjmp(comp, i)) return 0; }
                    if(tok[i].type != HL_K) comp->code[p] = comp->nc;
                break;
                case BAS_IF:
                    for(n = s, i = s + 1; i < l && tok[i].type != HL_K; i++);
                    if(i >= l || tok[i].type != HL_K || tok[i].id != BAS_THEN) { code_error(tok[i >= l ? s : i].pos, lang[ERR_SYNTAX]); return 0; }
                    comp_cdbg(comp, s + 1); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, s + 1, i, &m, &end, O_CND)) return 0;
                    comp_resolve(comp, end);
                    if(!end && comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JNZ;
                    else comp_gen(comp, BC_JZ);
                    i++;
                    if(i < l) {
                        /* one liner if, only "END", "GOTO" or label accepted */
                        if(tok[i].type == HL_K && tok[i].id == BAS_END) {
                            comp_gen(comp, comp->nc + 2);
                            comp_gen(comp, (MEG4_EXIT << 8) | BC_SCALL);
                        } else {
                            if(tok[i].type == HL_K && tok[i].id == BAS_GOTO) i++;
                            if(tok[i].type == HL_V) {
                                comp->code[comp->nc - 1] = comp->code[comp->nc - 1] == BC_JZ ? BC_JNZ : BC_JZ;
                                if(!comp_addjmp(comp, i + 1)) return 0;
                            } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                        }
                    } else {
                        /* multi-line if, with optional else branch */
                        l1 = comp->nc; comp_gen(comp, 0);
                        for(p = 1, s = i; i < e && p; i = l) {
                            i = getst(comp, i, &l, e);
                            if(i >= e || (tok[i].type == HL_K && tok[i].id == BAS_END && tok[i + 1].type == HL_K &&
                                tok[i + 1].id == BAS_IF)) p--; else
                            if(tok[i].type == HL_K && tok[i].id == BAS_IF) p++; else
                            if(p == 1 && tok[i].type == HL_K && (tok[i].id == BAS_ELSE || tok[i].id == BAS_ELIF)) break;
                        }
                        if(i >= e) { code_error(tok[n].pos, lang[ERR_NOCLOSE]); return 0; }
                        comp_cdbg(comp, s);
                        if(!(s = statement(comp, s, i))) return 0;
                        if(tok[s].type == HL_K && tok[s].id == BAS_ELSE) {
                            if(comp->ls != BAS_RETURN && comp->ls != BAS_GOTO) {
                                comp->code[l1] = comp->nc + 2;
                                comp_gen(comp, BC_JMP); l1 = comp->nc; comp_gen(comp, 0);
                            } else {
                                comp->code[l1] = comp->nc; l1 = 0;
                            }
                            for(p = 1, i = ++s; i < e && p; i = l) {
                                i = getst(comp, i, &l, e);
                                if(i >= e || (tok[i].type == HL_K && tok[i].id == BAS_END && tok[i + 1].type == HL_K &&
                                    tok[i + 1].id == BAS_IF)) p--; else
                                if(tok[i].type == HL_K && tok[i].id == BAS_IF) p++;
                            }
                            if(i >= e) { code_error(tok[n].pos, lang[ERR_NOCLOSE]); return 0; }
                            comp_cdbg(comp, s);
                            if(!(s = statement(comp, s, i))) return 0;
                        }
                        if(l1) comp->code[l1] = comp->nc;
                    }
                break;
                case BAS_FOR:
                    n = s++;
                    if(s >= l || tok[s].type != HL_V || (j = comp_findid(comp, &tok[s])) < MEG4_NUM_API + MEG4_NUM_BDEF ||
                      comp->id[j].t != T(T_SCALAR,T_FLOAT)) { code_error(tok[s].pos, lang[ERR_BADARG]); return 0; }
                    for(i = ++s; i < l && tok[i].type != HL_K; i++);
                    if(i >= l || tok[i].type != HL_K || tok[i].id != BAS_TO) { code_error(tok[i].pos, lang[ERR_SYNTAX]); return 0; }
                    /* set loop variable */
                    if(j < comp->ng) { comp->lc = comp->nc; comp_gen(comp, BC_CI); comp_gen(comp, comp->id[j].o); }
                    else comp_gen(comp, (comp->id[j].o << 8) | BC_ADR);
                    comp_gen(comp, BC_PUSHI);
                    comp_cdbg(comp, s); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, s, i, &m, &end, O_CND)) return 0;
                    if(m != T(T_SCALAR, T_FLOAT) || end) { code_error(tok[s].pos, lang[ERR_BADARG]); return 0; }
                    comp_gen(comp, BC_STF);
                    for(s += 2, i = s; i < l && tok[i].type != HL_K; i++);
                    /* get step, k variable id or if k == 0 then f is step constant */
                    k = 0; f = 1.0;
                    if(i < l) {
                        if(tok[i].type != HL_K || tok[i].id != BAS_STEP || i + 1 != l) { code_error(tok[i].pos, lang[ERR_SYNTAX]); return 0; }
                        if(tok[i + 1].type == HL_N) {
                            if(!comp_const(comp, i + 1, T(T_SCALAR, T_FLOAT), NULL, &f)) return 0;
                            if(f == 0.0 || f == -0.0) { code_error(tok[i + 1].pos, lang[ERR_BADARG]); return 0; }
                        } else
                        if(tok[i + 1].type != HL_V || (k = comp_findid(comp, &tok[i + 1])) < MEG4_NUM_API + MEG4_NUM_BDEF ||
                          comp->id[k].t != T(T_SCALAR, T_FLOAT)) { code_error(tok[i + 1].pos, lang[ERR_BADARG]); return 0; }
                    }
                    /* add check limit expression */
                    l1 = comp->nc;
                    comp_gen(comp, BC_PUSHF);
                    comp_cdbg(comp, s); m = T(T_SCALAR, T_FLOAT); end = 0;
                    if(!comp_expr(comp, s, i, &m, &end, O_CND)) return 0;
                    comp_resolve(comp, end);
                    if(!k) {
                        comp_gen(comp, f > 0.0 ? BC_LTF : BC_GTF);
                        comp_gen(comp, BC_JNZ);
                    } else {
                        comp_gen(comp, BC_SUBF);
                        comp_gen(comp, BC_PUSHF);
                        if(k < comp->ng) { comp->lc = comp->nc; comp_gen(comp, BC_CI); comp_gen(comp, comp->id[k].o); }
                        else comp_gen(comp, (comp->id[k].o << 8) | BC_ADR);
                        comp_gen(comp, BC_LDF);
                        comp_gen(comp, BC_JNS);
                    }
                    l2 = comp->nc; comp_gen(comp, 0);
                    comp_gen(comp, BC_JMP); comp_gen(comp, 0);
                    /* add step */
                    if(j < comp->ng) { comp->lc = comp->nc; comp_gen(comp, BC_CI); comp_gen(comp, comp->id[j].o); }
                    else comp_gen(comp, (comp->id[j].o << 8) | BC_ADR);
                    comp_gen(comp, BC_PUSHI);
                    comp_gen(comp, BC_LDF);
                    comp_gen(comp, BC_PUSHF);
                    if(!k) {
                        memcpy(&m, &f, 4);
                        comp_gen(comp, BC_CF);
                        comp_gen(comp, m);
                    } else {
                        if(k < comp->ng) { comp->lc = comp->nc; comp_gen(comp, BC_CI); comp_gen(comp, comp->id[k].o); }
                        else comp_gen(comp, (comp->id[k].o << 8) | BC_ADR);
                        comp_gen(comp, BC_LDF);
                    }
                    comp_gen(comp, BC_ADDF);
                    comp_gen(comp, BC_STF);
                    comp_gen(comp, BC_JMP); comp_gen(comp, l1);
                    comp->code[l2 + 2] = comp->nc;
                    for(p = 1, s = i; i < e && p; i = l) {
                        i = getst(comp, i, &l, e);
                        if(i >= e || (tok[i].type == HL_K && tok[i].id == BAS_NEXT)) p--; else
                        if(tok[i].type == HL_K && tok[i].id == BAS_FOR) p++;
                    }
                    if(i >= e || tok[i].type != HL_K || tok[i].id != BAS_NEXT) { code_error(tok[n].pos, lang[ERR_NOCLOSE]); return 0; }
                    if(i + 1 < e && tok[i + 1].type == HL_V && comp_findid(comp, &tok[i + 1]) != j) { code_error(tok[i + 1].pos, lang[ERR_BADARG]); return 0; }
                    comp_cdbg(comp, s);
                    if(!(s = statement(comp, s, i))) return 0;
                    comp_gen(comp, BC_JMP);
                    comp_gen(comp, l2 + 3);
                    comp->code[l2] = comp->nc;
                break;
                default: code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0;
            }
        } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        s = l;
    }
    return s;
}

/**
 * Compile BASIC source
 */
int comp_bas(compiler_t *comp)
{
    tok_t *tok;
    int i, j, k, l, m, n, o, to, s, e, sf, lf = -1, tf = -1, setupid = -1, loopid = -1, args[N_ARG];
    int readp = -1, ndata = 0;
    uint32_t *data = NULL;
    float f;

    if(!meg4.src || !comp || !comp->tok) return 0;
    comp->ops = bas_ops;
    tok = comp->tok;
    sf = comp->ntok;
    /* BASIC uses slightly different names, also has to be suffixed by the type, handle these here */
    for(i = 0; i < MEG4_NUM_API; i++) {
        if(!strcmp(comp->id[i].name, "pget")) strcpy(comp->id[i].name, "pget!"); else
        if(!strcmp(comp->id[i].name, "mget")) strcpy(comp->id[i].name, "mget!"); else
        if(!strcmp(comp->id[i].name, "inb"))  strcpy(comp->id[i].name, "peek"); else
        if(!strcmp(comp->id[i].name, "getc")) strcpy(comp->id[i].name, "inkey$"); else
        if(!strcmp(comp->id[i].name, "sqrt")) strcpy(comp->id[i].name, "sqr"); else
        if(!strcmp(comp->id[i].name, "atan")) strcpy(comp->id[i].name, "atn%"); else
            switch(meg4_api[i].ret) {
                case 1: case 2: strcat(comp->id[i].name, "%"); break;
                case 3: strcat(comp->id[i].name, "$"); break;
            }
    }
    /* add space for read pointer, counter and number of data */
    meg4.dp += 12;
    /* add strings required for print and input statements */
    fmt = comp->nstr; comp->nstr += 10;
    comp->str = (cstr_t*)realloc(comp->str, comp->nstr * sizeof(cstr_t));
    if(!comp->str) { comp->nstr = 0; return 0; }
    memset(&comp->str[fmt], 0, 10 * sizeof(cstr_t));
    memcpy(comp->str[fmt + 0].str, "%s\n", 3);  comp->str[fmt + 0].l = 3; comp->str[fmt + 0].n = 1;   /* 0 PRINT a$ */
    memcpy(comp->str[fmt + 1].str, "%s\t", 3);  comp->str[fmt + 1].l = 3; comp->str[fmt + 1].n = 1;   /* 1 PRINT a$, */
    memcpy(comp->str[fmt + 2].str, "%s", 2);    comp->str[fmt + 2].l = 2; comp->str[fmt + 2].n = 1;   /* 2 PRINT a$; */
    memcpy(comp->str[fmt + 3].str, " %d\n", 4); comp->str[fmt + 3].l = 4; comp->str[fmt + 3].n = 1;   /* 3 PRINT a% */
    memcpy(comp->str[fmt + 4].str, " %d\t", 4); comp->str[fmt + 4].l = 4; comp->str[fmt + 4].n = 1;   /* 4 PRINT a%, */
    memcpy(comp->str[fmt + 5].str, " %d", 3);   comp->str[fmt + 5].l = 3; comp->str[fmt + 5].n = 1;   /* 5 PRINT a%; */
    memcpy(comp->str[fmt + 6].str, " %f\n", 4); comp->str[fmt + 6].l = 4; comp->str[fmt + 6].n = 1;   /* 6 PRINT a */
    memcpy(comp->str[fmt + 7].str, " %f\t", 4); comp->str[fmt + 7].l = 4; comp->str[fmt + 7].n = 1;   /* 7 PRINT a, */
    memcpy(comp->str[fmt + 8].str, " %f", 3);   comp->str[fmt + 8].l = 3; comp->str[fmt + 8].n = 1;   /* 8 PRINT a; */
    memcpy(comp->str[fmt + 9].str, "%s? ", 4);  comp->str[fmt + 9].l = 2; comp->str[fmt + 9].n = 1;   /* 9 INPUT a$ (INPUT a$; uses 2) */

    /* first pass, detect globals, subroutines and functions */
    for(s = k = 0; s < comp->ntok; ) {
        while(s < comp->ntok && comp->tok[s].type == HL_D && meg4.src[comp->tok[s].pos] == '\n') s++;
        if(meg4.dp >= meg4.sp) { code_error(tok[s].pos, lang[ERR_MEMORY]); return 0; }
        for(e = s; e < comp->ntok && !(tok[e].type == HL_D && meg4.src[tok[e].pos] == '\n'); e++)
            if(tok[e].type == HL_S && comp_addstr(comp, e) < 0) return 0;

        if((!k || lf == setupid) && tok[s].type == HL_V && s + 1 < e && tok[s + 1].type == HL_D && tok[s + 1].id == ':') {
            if(!comp_addlbl(comp, s)) return 0;
            s += 2; if(s >= e) continue;
        }
        if(tok[s].type == HL_K && (tok[s].id == BAS_LET || tok[s].id == BAS_FOR)) {
            if(s + 1 >= e || tok[s + 1].type != HL_V) { code_error(tok[s].pos + 3, lang[ERR_SYNTAX]); return 0; }
            s++;
        }
        if(tok[s].type == HL_K) {
            if(tok[s].id == BAS_SUB) {
                if(s + 1 >= e || (tok[s + 1].type != HL_F && tok[s + 1].type != HL_V)) { code_error(tok[s + 1].pos, lang[ERR_SYNTAX]); return 0; }
                if(k) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                if((lf = comp_addid(comp, &tok[s + 1], T(T_FUNC, T_VOID))) < 0) return 0;
                if(tok[s + 1].len == 5 && !memcmp(&meg4.src[tok[s + 1].pos], "setup", 5)) { setupid = lf; sf = s; }
                if(tok[s + 1].len == 4 && !memcmp(&meg4.src[tok[s + 1].pos], "loop", 4)) loopid = lf;
                if(tok[s + 1].type == HL_F) n = getargs(comp, s + 2, e, args); else n = 0;
                if(n < 0) return 0;
                if(comp_addfunc(comp, lf, s + 1, n, args) < 0) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                k = 1; tf = s + 1;
            } else
            if(tok[s].id == BAS_FUNC) {
                if(s + 1 >= e || tok[s + 1].type != HL_F) { code_error(tok[s + 1].pos, lang[ERR_SYNTAX]); return 0; }
                if(k) { code_error(tok[s + (k ? 0 : 1)].pos, lang[ERR_NOTHERE]); return 0; }
                if((lf = comp_addid(comp, &tok[s + 1], T(T_FUNC, gettype(&tok[s + 1])))) < 0) return 0;
                n = getargs(comp, s + 2, e, args);
                if(n < 0) return 0;
                if(comp_addfunc(comp, lf, s + 1, n, args) < 0) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                k = 2; tf = s + 1;
            } else
            if(s + 1 < e && tok[s].id == BAS_END && tok[s + 1].type == HL_K) {
                if((tok[s + 1].id == BAS_SUB && k != 1) || (tok[s + 1].id == BAS_FUNC && k != 2)) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                k = 0; lf = tf = -1;
            } else
            if((!k || lf == setupid) && tok[s].id == BAS_DIM) {
                if(s + 1 >= e || (tok[s + 1].type != HL_F && tok[s + 1].type != HL_V)) { code_error(tok[s + 1].pos, lang[ERR_SYNTAX]); return 0; }
                if((j = comp_addid(comp, &tok[s + 1], T(T_PTR, gettype(&tok[s + 1])))) < 0) return 0;
                comp->id[j].o = meg4.dp + MEG4_MEM_USER;
                comp->id[j].a[0] = 10;
                comp->id[j].f[0] = 1;
                if(s + 2 < e && tok[s + 1].type == HL_F) {
                    if(tok[e - 1].type != HL_D || tok[e - 1].id != ')') { code_error(tok[s + 2].pos, lang[ERR_NOCLOSE]); return 0; }
                    for(l = 0, m = n = s + 3; n < e; n++) {
                        if(n + 1 == e || (tok[n].type == HL_D && tok[n].id == ',')) {
                            if(l >= N_DIM) { code_error(tok[m].pos, lang[ERR_NUMARG]); return 0; }
                            comp->id[j].t &= 0xfff0; comp->id[j].t |= T_PTR + l;
                            for(to = -1, o = m; o < n; o++) {
                                if(tok[o].type == HL_K && tok[o].id == BAS_TO) {
                                    if(to != -1) { code_error(tok[o].pos, lang[ERR_SYNTAX]); return 0; }
                                    to = o;
                                } else
                                if(tok[o].type != HL_N && tok[o].type != HL_O &&
                                  !(tok[n].type == HL_D && (tok[n].id == '(' || tok[n].id == ')'))
                                  ) { code_error(tok[o].pos, lang[ERR_SYNTAX]); return 0; }
                            }
                            if(to == -1) {
                                comp->id[j].f[l] = 1;
                                if(!comp_eval(comp, m, n, &comp->id[j].a[l])) return 0;
                            } else {
                                if(!comp_eval(comp, m, to, &comp->id[j].f[l])) return 0;
                                if(!comp_eval(comp, to + 1, n, &comp->id[j].a[l])) return 0;
                                comp->id[j].a[l] -= comp->id[j].f[l] - 1;
                            }
                            if(comp->id[j].a[l] < 1) { code_error(tok[m].pos, lang[ERR_BADARG]); return 0; }
                            comp->id[j].l *= comp->id[j].a[l];
                            m = n + 1; l++;
                        }
                    }
                } else
                    comp->id[j].l *= comp->id[j].a[0];
                meg4.dp += comp->id[j].l;
                comp_ddbg(comp, s + 1, j);
            } else
            if(tok[s].type == HL_V && tok[s].len == 8 && !memcmp(&meg4.src[tok[s].pos], "__FUNC__", 8)) {
                /* we got a __FUNC__ reference, add the current function's name to the string literals */
                if((tok[s].id = comp_addstr(comp, tf < 0 ? s : tf)) < 0) return 0;
                comp->str[tok[s].id].n++;
            } else
            if(tok[s].id == BAS_READ) {
                if(readp == -1) readp = tok[s].pos;
            } else
            if(tok[s].id == BAS_DATA) {
                if(k) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                for(j = s + 1; j < e; j++)
                    if(tok[j].type == HL_N || tok[j].type == HL_S) ndata++; else
                    if(tok[j].type != HL_D || tok[j].id != ',') { code_error(tok[j].pos, lang[ERR_SYNTAX]); return 0; }
            }
        } else
        if(tok[s].type == HL_V) {
            /* CBM easter egg */
            if(tok[s].len == 4 && !memcmp(&meg4.src[tok[s].pos], "WAIT", 4) &&
               tok[s + 1].type == HL_N && atoi(&meg4.src[tok[s + 1].pos]) == 6502 && tok[s + 2].type == HL_D &&
               tok[s + 3].type == HL_N && atoi(&meg4.src[tok[s + 3].pos]) == 1) {
                code_error(tok[s].pos, "Melinda on wedding night: Aha, I get it, that's why Microsoft!"); return 0;
            }
            /* global variable */
            if(!k || lf == setupid) {
                if((s + 1 < e && tok[s + 1].type == HL_O && tok[s + 1].len == 1 && meg4.src[tok[s + 1].pos] == '=')) {
                    if(comp_findid(comp, &tok[s]) < 0) {
                        if((j = comp_addid(comp, &tok[s], T(T_SCALAR, gettype(&tok[s])))) < 0) return 0;
                        comp->id[j].o = meg4.dp + MEG4_MEM_USER;
                        meg4.dp += comp->id[j].l;
                        comp_ddbg(comp, s, j);
                    }
                } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
            }
        }
        s = e;
    }
    /* we must have a setup subroutine, because we'll add statements in root to that */
    if(setupid == -1) {
        setupid = comp->nid++;
        comp->id = (idn_t*)realloc(comp->id, comp->nid * sizeof(idn_t));
        if(!comp->id) { comp->nid = 0; return 0; }
        memset(&comp->id[setupid], 0, sizeof(idn_t));
        memcpy(comp->id[setupid].name, "setup", 5);
        comp->id[setupid].r = 1;
        comp->id[setupid].t = T(T_FUNC, T_VOID);
        if(comp_addfunc(comp, setupid, -1, 0, NULL) < 0) { code_error(6, lang[ERR_SYNTAX]); return 0; }
    } else if(comp->id[setupid].t != T(T_FUNC, T_VOID) || comp->f[comp->id[setupid].f[0]].n) { code_error(comp->id[setupid].p, lang[ERR_BADPROTO]); return 0; }
    /* check loop subroutine */
    if(loopid != -1 && (comp->id[loopid].t != T(T_FUNC, T_VOID) || comp->f[comp->id[loopid].f[0]].n)) { code_error(comp->id[loopid].p, lang[ERR_BADPROTO]); return 0; }
    /* finalize initialized data */
    if(!comp_addinit(comp)) return 0;
    if(readp != -1 && !ndata) { code_error(readp, lang[ERR_NODATA]); return 0; }
    /* add "DATA" to initialized data */
    if(ndata) {
        meg4.dp = (meg4.dp + 3) & ~3;
        i = meg4.dp + MEG4_MEM_USER; i = htole32(i); memcpy(&meg4.data[0], &i, 4);
        i = htole32(ndata); memcpy(&meg4.data[8], &i, 4);
        data = (uint32_t*)(meg4.data + meg4.dp);
        for(s = 0; s < comp->ntok; ) {
            s = getst(comp, s, &e, comp->ntok);
            if(tok[s].type == HL_K && tok[s].id == BAS_DATA) {
                if(meg4.dp + ndata * 4 >= meg4.sp) { code_error(tok[s].pos, lang[ERR_MEMORY]); return 0; }
                for(j = s + 1; j < e; j++)
                    if(tok[j].type == HL_N) {
                        if(!comp_const(comp, j, T(T_SCALAR, T_FLOAT), NULL, &f)) return 0;
                        memcpy(&i, &f, 4); *data++ = htole32(i);
                    } else
                    if(tok[j].type == HL_S)
                        *data++ = htole32(comp->str[tok[j].id].n);
            }
            s = e;
        }
        meg4.dp += ndata * 4;
    }
    /* save number of global identifiers */
    comp->ng = comp->nid;

    /* second pass, parse statements in root (anything that's not in a subroutine's or function's body) */
    comp->cf = 0;
    comp->id[setupid].o = comp->nc;
    comp->lf = comp->ls = 0;
    if(!(s = statement(comp, 0, comp->ntok))) return 0;
    /* third pass, parse statements in setup subroutine (if it exists) */
    comp->lf = comp->ls = 0;
    if(!(s = statement(comp, sf + 2, comp->ntok))) return 0;
    /* at this point we must ensure that if program execution reaches here, it will return, no matter if we had
     * a setup subroutine or just statements in root */
    if(comp->code && comp->nc && comp->code[comp->nc - 1] != BC_RET)
        comp_gen(comp, BC_RET);

    /* fourth pass, parse every other subroutines and functions */
    for(i = 1; i < comp->nf; i++) {
        comp->cf = i; s = comp->f[i].p; comp->nid = comp->ng;
        /* failsafe, we have already parsed setup */
        if(!s || s == sf) continue;
        /* add parameters, and record the last function parameter's id */
        s++;
        if(tok[s].type == HL_F)
            for(s++, n = 0; n < comp->f[i].n && s + 1 < comp->ntok && (tok[s].type != HL_D || tok[s].id != ')'); n++) {
                if((j = comp_addid(comp, &tok[s], comp->f[i].t[n])) < 0) return 0;
                comp->id[j].o = n * 4;
                s++; if(tok[s].type == HL_D && tok[s].id == ',') s++;
            }
        s++;
        comp->pf = comp->nid;
        comp->id[comp->f[i].id].o = comp->nc;
        comp->lf = comp->ls = 0;
        /* detect and add local variables */
        for(k = s, n = 0; s < comp->ntok; ) {
            s = getst(comp, s, &e, comp->ntok);
            if(tok[s].type == HL_K && tok[s].id == BAS_END && tok[s + 1].type == HL_K && tok[s + 1].id == tok[comp->f[i].p].id) break;
            if(tok[s].type == HL_K && (tok[s].id == BAS_LET || tok[s].id == BAS_FOR)) {
                if(s + 1 >= e || tok[s + 1].type != HL_V) { code_error(tok[s].pos + 3, lang[ERR_SYNTAX]); return 0; }
                s++;
            }
            if((s + 1 < e && tok[s + 1].type == HL_O && tok[s + 1].len == 1 && meg4.src[tok[s + 1].pos] == '=')) {
                if(comp_findid(comp, &tok[s]) < 0) {
                    if((j = comp_addid(comp, &tok[s], T(T_SCALAR, gettype(&tok[s])))) < 0) return 0;
                    n += comp->id[j].l;
                    comp->id[j].o = -n;
                }
            }
            s = e;
        }
        /* statements */
        if(!(s = statement(comp, k, comp->ntok))) return 0;
        if(!comp_chkids(comp, comp->pf)) return 0;
        if(comp->ls != BAS_RETURN && comp->ls != BAS_GOTO) {
            if(tok[comp->f[i].p].id == BAS_FUNC) { code_error(tok[s].pos, lang[ERR_NORET]); return 0; }
            comp_gen(comp, BC_RET);
        }
    }

    return 1;
}

#endif /* NOEDITORS */
