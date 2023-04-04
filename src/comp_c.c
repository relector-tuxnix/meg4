/*
 * meg4/comp_c.c
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
 * @brief Compiler specific functions for the C language
 *
 */

#include "meg4.h"

#ifndef NOEDITORS
#include "editors/editors.h"    /* for the highlighter token defines */
#include "api.h"

/* must match the keywords in misc/hl.json */
enum { C_IF, C_ELSE, C_SWITCH, C_CASE, C_DEFAULT, C_FOR, C_WHILE, C_DO, C_BREAK, C_CONTINUE, C_RETURN, C_GOTO, C_SIZEOF, C_DEBUG };

/* operators in precedence order, must match O_ defines in cpu.h */
char *c_ops[] = {
    /* assignments */   "|=", "&=", "^=", "<<=", ">>=", "+=", "-=", "*=", "/=", "%=", "", "=",
    /* logical */       "?", "||", "&&",
    /* bitwise */       "|", "^", "&",
    /* conditionals */  "==", "!=", "<", ">", "<=", ">=",
    /* arithmetic */    "<<", ">>", "+", "-", "*", "/", "%", "",
    /* unaries */       "!", "~", "++", "--",
    /* field select */  ".", "->"
};

/**
 * Convert tokens
 */
static int gettok(compiler_t *comp, int *s, tok_t **tok, int *nt, int *l, int lvl)
{
    int i, j, k, n;

    if(lvl > 8) return -1;
    if(comp->tok[*s].type == HL_V && (i = comp_findid(comp, &comp->tok[*s])) >= MEG4_NUM_API + MEG4_NUM_BDEF &&
      (comp->id[i].t & 15) == T_DEF) {
        j = (comp->id[i].t >> 4);
        if(j == T_I32) {
            comp->tok[*s].type = HL_C;
            comp->tok[*s].id = comp->id[i].f[0];
        } else {
            i = comp->id[i].f[0]; j = i + 3; n = comp->tok[i].id - 2;
            if(n > 1) {
                *tok = (tok_t*)realloc(*tok, (*l + n) * sizeof(tok_t));
                if(!*tok) return 0;
                memset(&(*tok)[*l], 0, n * sizeof(tok_t));
                *l += n - 1;
            }
            for(i = 0; i < n; i++, j++)
                if((k = gettok(comp, &j, tok, nt, l, lvl + 1)) < 1) {
                    if(k == -1 && !lvl) code_error(comp->tok[*s].pos, lang[ERR_RECUR]);
                    return k;
                }
            return 1;
        }
    } else
    if(comp->tok[*s].type == HL_S) {
        if(comp_addstr(comp, *s) < 0) return 0;
        if(*nt > 0 && comp->tok[*s].type == HL_S && (*tok)[*nt - 1].type == HL_S) {
            (*tok)[*nt - 1].id = comp->tok[*s].id;
            return 1;
        }
    } else
    if(comp->tok[*s].type == HL_T) {
        /* typedef only allowed with structs */
        if(comp->tok[*s].id == C_TYPEDEF) {
            (*s)++;
            if(comp->tok[*s].id != C_STRUCT && comp->tok[*s].id != C_UNION) { code_error(comp->tok[(*s) - 1].pos, lang[ERR_NOTHERE]); return 0; }
        }
        /* convert multiple tokens into a single one */
        if((comp->tok[*s].id == C_SIGNED || comp->tok[*s].id == C_UNSIGNED)) {
            (*s)++;
            if(*s >= comp->ntok || comp->tok[*s].type != HL_T || comp->tok[*s].id < C_CHAR || comp->tok[*s].id > C_INT)
                { code_error(comp->tok[(*s) - 1].pos, lang[ERR_NOTHERE]); return 0; }
            if(comp->tok[(*s) - 1].id == C_SIGNED)
                switch(comp->tok[*s].id) {
                    case C_CHAR: comp->tok[*s].id = T_I8; break;
                    case C_SHORT: comp->tok[*s].id = T_I16; break;
                    case C_INT: comp->tok[*s].id = T_I32; break;
                }
            else
                switch(comp->tok[*s].id) {
                    case C_CHAR: comp->tok[*s].id = T_U8; break;
                    case C_SHORT: comp->tok[*s].id = T_U16; break;
                    case C_INT: comp->tok[*s].id = T_U32; break;
                }
        } else
            switch(comp->tok[*s].id) {
                case C_CHAR: comp->tok[*s].id = T_I8; break;
                case C_SHORT: comp->tok[*s].id = T_I16; break;
                case C_INT: comp->tok[*s].id = T_I32; break;
            }
    }
    memcpy(&(*tok)[(*nt)++], &comp->tok[*s], sizeof(tok_t));
    return 1;
}

/**
 * Check if token is the given one and skip it
 */
static int skip(compiler_t *comp, int s, char t)
{
    if(s >= comp->ntok || comp->tok[s].type != HL_D || comp->tok[s].id != t) { code_error(comp->tok[s].pos, lang[ERR_SYNTAX]); return 0; }
    return s + 1;
}

/**
 * Get a type
 */
static int gettype(compiler_t *comp, int *s, int *p, int t)
{
    int i = 0;

    *p = T_SCALAR;
    if(t == T_STR) { *p = T_PTR; t = T_I8; }     /* str_t  -> char* */
    if(t == T_ADDR) { *p = T_PTR; t = T_U8; }    /* addr_t -> uint8_t* */
    while(comp->tok[*s].type == HL_O && meg4.src[comp->tok[*s].pos] == '*') {
        if(i >= N_DIM) { code_error(comp->tok[*s].pos, lang[ERR_TOOCLX]); return 0; }
        *p = T_PTR + i; (*s)++;
    }
    if(comp->tok[*s].type != HL_V) { code_error(comp->tok[*s].pos, lang[ERR_SYNTAX]); return 0; }
    return T((*p), t);
}

/**
 * Get array subscription
 */
static int getarr(compiler_t *comp, int *s, int i, int p)
{
    int e, l;

    if(comp->tok[*s].type == HL_D && comp->tok[*s].id == '[') {
        /* we do not support pointer arrays (at least, not yet in this version) */
        if(p != T_SCALAR) { code_error(comp->tok[*s].pos, lang[ERR_SYNTAX]); return 0; }
        /* array */
        for(l = 0; *s < comp->ntok && comp->tok[*s].type == HL_D && comp->tok[*s].id == '['; l++) {
            if(l >= N_DIM) { code_error(comp->tok[*s].pos, lang[ERR_NUMARG]); return 0; }
            (*s)++; for(e = *s; e < comp->ntok && (comp->tok[e].type != HL_D || comp->tok[e].id != ']'); e++);
            if(!comp_eval(comp, *s, e, &comp->id[i].a[l])) return 0;
            if(comp->id[i].a[l] < 1) { code_error(comp->tok[*s].pos, lang[ERR_BADARG]); return 0; }
            comp->id[i].l *= comp->id[i].a[l];
            *s = e + 1; comp->id[i].t &= 0xfff0; comp->id[i].t |= T_PTR + l;
        }
    }
    return 1;
}

/**
 * Get case value
 */
static int getcase(compiler_t *comp, int s, int *val)
{
    if((comp->tok[s].type != HL_N && comp->tok[s].type != HL_C) || !comp_eval(comp, s, s + 1, val)) return 0;
    return comp->tok[s + 1].type == HL_D && comp->tok[s + 1].id == ':';
}

/**
 * Parse initializer(s)
 */
static int getinit(compiler_t *comp, int s, int id, int len, int lvl, int offs)
{
    tok_t *tok = comp->tok;
    int i, e, p, maxlvl;
    float f;

    /* failsafe, should never happen */
    if(comp->id[id].s > len || offs + len > (int)meg4.dp) { code_error(tok[s].pos, lang[ERR_TOOLNG]); return 0; }

    /* special case, "{ 0 }" must match any type, any dimensions. We have already zeroed the data, so nothing to do */
    if(tok[s].type == HL_D && tok[s].id == '{' && tok[s + 1].type == HL_N && tok[s + 1].len == 1 &&
        meg4.src[tok[s + 1].pos] == '0' && tok[s + 2].type == HL_D && tok[s + 2].id == '}') return s + 3;

    if((comp->id[id].t & 15) == T_STRUCT) {
        /* TODO: struct init */
        return 0;
    } else
    if((comp->id[id].t & 15) < T_SCALAR) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
    else {
        maxlvl = (comp->id[id].t & 15) - T_SCALAR;
        if(tok[s].type == HL_S) {
            /* string constant */
            if(lvl + 1 != maxlvl || comp->id[id].s != 4 || (comp->id[id].t >> 4) == T_FLOAT) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
            i = htole32(comp->str[tok[s].id].n);
            memcpy(meg4.data + offs, &i, comp->id[id].s);
            s++;
        } else
        if(lvl == maxlvl) {
            if(tok[s].type == HL_C) {
                /* character constant */
                i = tok[s].id;
                if(meg4_api_lenkey(i) > comp->id[id].s) { code_error(tok[s].pos, lang[ERR_TOOBIG]); return 0; }
                memcpy(meg4.data + offs, &i, comp->id[id].s);
                s++;
            } else
            if(tok[s].type == HL_N && (comp->id[id].t >> 4) == T_FLOAT) {
                /* float constant */
                if(!comp_const(comp, s, T(T_SCALAR, T_FLOAT), NULL, &f)) return 0;
                memcpy(&i, &f, 4); i = htole32(i);
                memcpy(meg4.data + offs, &i, comp->id[id].s);
                s++;
            } else {
                /* integer expression (possibly with built-in defines and operations) */
                for(e = s, p = 0; e < comp->ntok && meg4.src[tok[e].pos] != '}' && meg4.src[tok[e].pos] != ';'; e++) {
                    if(meg4.src[tok[e].pos] == '(') p++; else
                    if(meg4.src[tok[e].pos] == ')') { p--; if(p < 0) { code_error(tok[e].pos, lang[ERR_NOOPEN]); return 0; } } else
                    if(meg4.src[tok[e].pos] == ',' && !p) break;
                }
                if(!comp_eval(comp, s, e, &i)) return 0;
                i = htole32(i);
                memcpy(meg4.data + offs, &i, comp->id[id].s);
                s = e;
            }
        } else
        if(lvl < maxlvl) {
            /* array initializer */
            if(!(s = skip(comp, s, '{')) || !comp->id[id].a[lvl]) return 0;
            i = len / comp->id[id].a[lvl];
            for(p = 0; s + 1 < comp->ntok && meg4.src[tok[s].pos] != '}' && meg4.src[tok[s].pos] != ';'; p++, offs += i) {
                if(p >= comp->id[id].a[lvl]) { code_error(tok[s].pos, lang[ERR_TOOLNG]); return 0; }
                if(!(s = getinit(comp, s, id, i, lvl + 1, offs))) return 0;
                if(meg4.src[tok[s].pos] == ',') s++;
            }
            if(!(s = skip(comp, s, '}'))) return 0;
        } else { code_error(tok[s].pos, lang[ERR_TOOLNG]); return 0; }
    }
    return s;
}

/**
 * Parse expressions
 */
static int expression(compiler_t *comp, int s)
{
    tok_t *tok = comp->tok;
    int i = s, e, p = 0, t = T(T_SCALAR, T_I32);

    if(tok[s].type == HL_D && tok[s].id == ';') return s;
    do {
        comp_cdbg(comp, s);
        for(e = s; e < comp->ntok && meg4.src[tok[e].pos] != ';'; e++) {
            if(meg4.src[tok[e].pos] == '(') { p++; i = s; }
            if(meg4.src[tok[e].pos] == ')') { p--; if(p < 0) break; }
            if(meg4.src[tok[e].pos] == ',' && !p) break;
        }
        if(p > 0) { code_error(tok[i].pos, lang[ERR_NOCLOSE]); return 0; }
        if(!comp_expr(comp, s, e, &t, O_AOR)) return 0;
        s = e + 1;
    } while(meg4.src[tok[e].pos] == ',');
    return e;
}

/**
 * Parse statements
 */
static int statement(compiler_t *comp, int s, int sw, int sl, int *el)
{
    tok_t *tok = comp->tok;
    int i, j, k, l, t, p, m, nid, sz = 0, l1, l2, l3, n, c[N_EXPR], d;

    if(s >= comp->ntok || (tok[s].type == HL_D && tok[s].id == ';')) return s + 1; else
    if(tok[s].type == HL_K && tok[s].id == C_DEBUG) {
        s++; while(s < comp->ntok && tok[s].type == HL_D && tok[s].id == ';') s++;
        comp_gen(comp, BC_DEBUG); comp_cdbg(comp, s); return s;
    } else
    if(tok[s].type == HL_D && tok[s].id == '{') {
        /* compound */
        j = s++; comp->lf++; nid = comp->nid;
        while(s < comp->ntok && tok[s].type == HL_T) {
            /* local variables */
            if(sw > 0 || comp->lf != 1) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
            t = tok[s++].id;
            if(t >= C_CHAR || s + 1 >= comp->ntok) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
            while(s + 1 < comp->ntok && (tok[s].type != HL_D || tok[s].id != ';')) {
                if(!(m = gettype(comp, &s, &p, t))) return 0;
                s++; if((i = comp_addid(comp, &tok[s - 1], m)) < 0 || !getarr(comp, &s, i, p)) return 0;
                sz += comp->id[i].l;
                comp->id[i].o = -sz;
                if(s >= comp->ntok || tok[s].len != 1 || (meg4.src[tok[s].pos] != ';' && meg4.src[tok[s].pos] != ','))
                    { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                if(tok[s].type == HL_D && meg4.src[tok[s].pos] == ',') s++;
            }
            if(!(s = skip(comp, s, ';'))) return 0;
        }
        if(sz) comp_gen(comp, (-sz << 8) | BC_SP);
        while(s < comp->ntok && (tok[s].type != HL_D || tok[s].id != '}')) {
            if(!(s = statement(comp, s, sw, sl, el))) return 0;
        }
        if(tok[s].type != HL_D || tok[s].id != '}') { code_error(tok[j].pos, lang[ERR_NOCLOSE]); return 0; }
        for(i = nid; i < comp->nid; i++)
            if((comp->id[i].t & 15) == T_LABEL) {
                if(i != nid) memcpy(&comp->id[nid], &comp->id[i], sizeof(idn_t));
                nid++;
            }
        comp->nid = nid;
        if(sz && comp->ls != C_RETURN && comp->ls != C_GOTO) comp_gen(comp, (sz << 8) | BC_SP);
        s++;
    } else
    if(tok[s].type == HL_T) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; } else
    if(tok[s].type == HL_V && tok[s + 1].type == HL_D && tok[s + 1].id == ':') {
        /* label */
        if(!comp_addlbl(comp, s)) return 0;
        comp->ls = 0;
        s += 2;
    } else
    if(tok[s].type == HL_K) {
        /* control structures */
        comp->ls = tok[s].id;
        switch(tok[s].id) {
            case C_SWITCH:
                s++; if(!(s = skip(comp, s, '('))) return 0;
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ')'))) return 0;
                m = s; if(!(s = skip(comp, s, '{'))) return 0;
                n = d = j = 0; memset(c, 0, sizeof(c)); k = -2147483647; l = 2147483647;
                for(i = s; i < comp->ntok; i++) {
                    if(tok[i].type == HL_D && tok[i].id == '{') j++;
                    if(tok[i].type == HL_D && tok[i].id == '}') { j--; if(j < 0) break; }
                    if(!j && tok[i].type == HL_K) {
                        if(tok[i].id == C_CASE) {
                            if(n >= N_EXPR) { code_error(tok[m].pos, lang[ERR_TOOCLX]); return 0; }
                            i++; if(!getcase(comp, i, &j)) { code_error(tok[i].pos, lang[ERR_SYNTAX]); return 0; }
                            if(j > k) k = j;
                            if(j < l) l = j;
                            c[n++] = j;
                            for(j = 0; j < n - 1; j++)
                                if(c[j] == c[n - 1]) { code_error(tok[i].pos, lang[ERR_ALRDEF]); return 0; }
                            j = 0;
                        }
                        if(tok[i].id == C_DEFAULT) {
                            if(d) { code_error(tok[i].pos, lang[ERR_ALRDEF]); return 0; }
                            d = 1;
                        }
                    }
                }
                if(k < l) { code_error(tok[m].pos, lang[ERR_SYNTAX]); return 0; }
                k -= l; k++;
                if(k >= 256) { code_error(tok[m].pos, lang[ERR_TOOCLX]); return 0; }
                comp_gen(comp, BC_SW | (k << 8));
                sw = comp->nc;
                comp_gen(comp, l);
                for(i = 0; i <= k; i++)
                    comp_gen(comp, 0);
                l3 = 0;
                if(!(s = statement(comp, m, sw, sl, &l3))) return 0;
                if(comp->code[comp->nc - 2] == BC_JMP && l3 == comp->nc - 1) { l3 = comp->code[l3]; comp->nc -= 2; }
                for(i = 0; i <= k; i++)
                    if(!comp->code[sw + 1 + i]) comp->code[sw + 1 + i] = comp->nc;
                while(l3) { l2 = comp->code[l3]; comp->code[l3] = comp->nc; l3 = l2; }
                comp->ls = C_SWITCH;
            break;
            case C_CASE:
                if(!sw) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                s++; if(!getcase(comp, s, &j)) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                comp->code[sw + 2 + j - (int)comp->code[sw]] = comp->nc;
                s += 2;
            break;
            case C_DEFAULT:
                if(!sw) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                for(i = 0, k = (int)(comp->code[sw - 1] >> 8); i <= k; i++)
                    if(!comp->code[sw + 1 + i]) comp->code[sw + 1 + i] = comp->nc;
                s++; if(!(s = skip(comp, s, ':'))) return 0;
            break;
            case C_IF:
                s++; if(!(s = skip(comp, s, '('))) return 0;
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ')'))) return 0;
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JNZ;
                else comp_gen(comp, BC_JZ);
                l1 = comp->nc; comp_gen(comp, 0);
                if(!(s = statement(comp, s, 0, sl, el))) return 0;
                if(tok[s].type == HL_K && tok[s].id == C_ELSE) {
                    if(comp->ls != C_RETURN && comp->ls != C_GOTO) {
                        comp->code[l1] = comp->nc + 2;
                        comp_gen(comp, BC_JMP); l1 = comp->nc; comp_gen(comp, 0);
                    } else {
                        comp->code[l1] = comp->nc; l1 = 0;
                    }
                    s++; if(!(s = statement(comp, s, 0, sl, el))) return 0;
                }
                if(l1) comp->code[l1] = comp->nc;
                comp->ls = C_IF;
            break;
            case C_FOR:
                s++; if(!(s = skip(comp, s, '('))) return 0;
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ';'))) return 0;
                l1 = comp->nc;
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ';'))) return 0;
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JNZ;
                else comp_gen(comp, BC_JZ);
                l2 = comp->nc; comp_gen(comp, 0);
                comp_gen(comp, BC_JMP); comp_gen(comp, 0);
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ')'))) return 0;
                comp_gen(comp, BC_JMP); comp_gen(comp, l1);
                comp->code[l2 + 2] = comp->nc; l3 = 0;
                if(!(s = statement(comp, s, 0, l1, &l3))) return 0;
                comp_gen(comp, BC_JMP);
                comp_gen(comp, l2 + 3);
                comp->code[l2] = comp->nc;
                while(l3) { l2 = comp->code[l3]; comp->code[l3] = comp->nc; l3 = l2; }
                comp->ls = C_FOR;
            break;
            case C_WHILE:
                s++; if(!(s = skip(comp, s, '('))) return 0;
                l1 = comp->nc;
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ')'))) return 0;
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JNZ;
                else comp_gen(comp, BC_JZ);
                l2 = comp->nc; comp_gen(comp, 0); l3 = 0;
                if(!(s = statement(comp, s, 0, l1, &l3))) return 0;
                comp_gen(comp, BC_JMP);
                comp_gen(comp, l1);
                comp->code[l2] = comp->nc;
                while(l3) { l2 = comp->code[l3]; comp->code[l3] = comp->nc; l3 = l2; }
                comp->ls = C_WHILE;
            break;
            case C_DO:
                s++;
                l1 = comp->nc; l3 = 0;
                if(!(s = statement(comp, s, 0, l1, &l3))) return 0;
                if(tok[s].type != HL_K || tok[s].id != C_WHILE) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; } else s++;
                if(!(s = skip(comp, s, '('))) return 0;
                if(!(s = expression(comp, s))) return 0;
                if(!(s = skip(comp, s, ')'))) return 0;
                if(!(s = skip(comp, s, ';'))) return 0;
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JZ;
                else comp_gen(comp, BC_JNZ);
                comp_gen(comp, l1);
                while(l3) { l2 = comp->code[l3]; comp->code[l3] = comp->nc; l3 = l2; }
                comp->ls = C_DO;
            break;
            case C_BREAK:
                if(!el) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                s++; if(!(s = skip(comp, s, ';'))) return 0;
                comp_gen(comp, BC_JMP);
                comp_gen(comp, *el);
                *el = comp->nc - 1;
            break;
            case C_CONTINUE:
                if(!sl) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                s++; if(!(s = skip(comp, s, ';'))) return 0;
                comp_gen(comp, BC_JMP);
                comp_gen(comp, sl);
            break;
            case C_RETURN:
                s++;
                if(comp->id[comp->f[comp->cf].id].t != T(T_FUNC, T_VOID)) {
                    if(meg4.src[tok[s].pos] == ';') { code_error(tok[s - 1].pos, lang[ERR_NUMARG]); return 0; }
                    if(!(s = expression(comp, s))) return 0;
                }
                if(!(s = skip(comp, s, ';'))) return 0;
                comp_gen(comp, BC_RET);
            break;
            case C_GOTO:
                comp_gen(comp, BC_JMP);
                s++; if(!comp_addjmp(comp, s)) return 0;
                if(!(s = skip(comp, s, ';'))) return 0;
            break;
        }
    } else {
        /* expression */
        if(!(s = expression(comp, s)) || !(s = skip(comp, s, ';'))) return 0;
        comp->ls = 0;
    }
    return s;
}

/**
 * Compile C source
 */
int comp_c(compiler_t *comp)
{
    tok_t *tok;
    int i, j, k, l, s, e, p, t, P, B, S, n, m, nt = 0, tf = -1, setupid = -1, loopid = -1, args[N_ARG], indef = -1, isdef[N_ARG];

    if(!meg4.src || !comp || !comp->tok) return 0;

    /* allocate a bit more so that lookahead won't read out of bounds */
    l = comp->ntok + 8;
    tok = (tok_t*)malloc(l * sizeof(tok_t));
    if(!tok) return 0;
    memset(tok, 0, l * sizeof(tok_t));
    comp->ops = c_ops;

    /* first pass, pre-compiler */
    for(s = 0; s < comp->ntok; s++) {
        if(comp->tok[s].type == HL_P) {
            if(s + comp->tok[s].id >= comp->ntok) { code_error(comp->tok[s].pos, lang[ERR_SYNTAX]); return 0; }
            if(comp->tok[s].id > 2 && comp->tok[s + 1].len == 6 && !memcmp(&meg4.src[comp->tok[s + 1].pos], "define", 6) && comp->tok[s + 2].type == HL_V) {
                if(indef < 0 || isdef[indef]) {
                    if((k = comp_addid(comp, &comp->tok[s + 2], T(T_DEF, T_DEF))) < 0) { code_error(comp->tok[s + 2].pos, lang[ERR_ALRDEF]); return 0; }
                    comp->id[k].f[0] = s;
                }
                s += comp->tok[s].id;
            } else
            if(comp->tok[s].id == 2 && comp->tok[s + 1].len == 5 && !memcmp(&meg4.src[comp->tok[s + 1].pos], "ifdef", 5) && comp->tok[s + 2].type == HL_V) {
                if(indef >= N_ARG) { code_error(comp->tok[s].pos, lang[ERR_TOOCLX]); return 0; }
                if(indef < 0 || isdef[indef])
                    isdef[++indef] = comp_findid(comp, &comp->tok[s + 2]) != -1;
                else
                    isdef[++indef] = 0;
                s += comp->tok[s].id;
            } else
            if(comp->tok[s].id == 2 && comp->tok[s + 1].len == 6 && !memcmp(&meg4.src[comp->tok[s + 1].pos], "ifndef", 6) && comp->tok[s + 2].type == HL_V) {
                if(indef >= N_ARG) { code_error(comp->tok[s].pos, lang[ERR_TOOCLX]); return 0; }
                if(indef < 0 || isdef[indef])
                    isdef[++indef] = comp_findid(comp, &comp->tok[s + 2]) == -1;
                else
                    isdef[++indef] = 0;
                s += comp->tok[s].id;
            } else
            if(comp->tok[s].id > 1 && comp->tok[s + 1].len == 2 && !memcmp(&meg4.src[comp->tok[s + 1].pos], "if", 2)) {
                if(indef >= N_ARG) { code_error(comp->tok[s].pos, lang[ERR_TOOCLX]); return 0; }
                if(indef < 0 || isdef[indef]) {
                    if(!comp_eval(comp, s + 2, s + comp->tok[s].id + 1, &j)) { code_error(comp->tok[s + 2].pos, lang[ERR_SYNTAX]); return 0; }
                    isdef[++indef] = j != 0;
                } else
                    isdef[++indef] = 0;
                s += comp->tok[s].id;
            } else
            if(comp->tok[s].id == 1 && comp->tok[s + 1].len == 4 && !memcmp(&meg4.src[comp->tok[s + 1].pos], "else", 4) && indef >= 0) {
                if(indef < 0) { code_error(comp->tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                if(indef < 1 || isdef[indef - 1])
                    isdef[indef] ^= 1;
                s += comp->tok[s].id;
            } else
            if(comp->tok[s].id == 1 && comp->tok[s + 1].len == 5 && !memcmp(&meg4.src[comp->tok[s + 1].pos], "endif", 5) && indef >= 0) {
                if(indef < 0) { code_error(comp->tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                indef--;
                s += comp->tok[s].id;
            } else { code_error(comp->tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        } else
        if(indef < 0 || isdef[indef]) {
            /* replace defines with their values (only for user defined defines, built-in ones are
             * handled by comp_expr() as those might be context dependent like __FUNC__ or __LINE__) */
            if(!gettok(comp, &s, &tok, &nt, &l, 0)) return 0;
        }
    }
    free(comp->tok); comp->tok = tok; comp->ntok = nt;

    /* second pass, detect enums, structs, globals and functions */
    for(s = P = B = S = 0; s < comp->ntok; s++) {
        /* check if (), [] and {} have closing pairs */
        if(tok[s].type == HL_D) {
            switch(meg4.src[tok[s].pos]) {
                case '(': P++; break;
                case ')': P--; if(P < 0) { code_error(tok[s].pos, lang[ERR_NOOPEN]); return 0; } break;
                case '[': B++; break;
                case ']': B--; if(B < 0) { code_error(tok[s].pos, lang[ERR_NOOPEN]); return 0; } break;
                case '{':
                    if(P != 0 || B != 0) goto noclose;
                    S++; break;
                case '}':
                    if(P != 0 || B != 0) goto noclose;
                    S--; if(S == 0) { tf = -1; } if(S < 0) { code_error(tok[s].pos, lang[ERR_NOOPEN]); return 0; } break;
            }
        } else
        if(tok[s].type == HL_T) {
            if(tok[s].id == C_ENUM) {
                /* enums */
                if(P != 0 || B != 0 || S != 0) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                s++; if(!(s = skip(comp, s, '{'))) return 0;
                for(i = 0; s + 1 < comp->ntok && (tok[s].type != HL_D || tok[s].id != '}'); i++) {
                    if(tok[s].type != HL_V) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    j = s++;
                    switch(meg4.src[tok[s].pos]) {
                        case ',': case '}': break;
                        case '=':
                            for(e = s + 1; e < comp->ntok && (tok[e].type == HL_D || tok[e].type == HL_N || tok[e].type == HL_O) &&
                                meg4.src[tok[e].pos] != ',' && meg4.src[tok[e].pos] != '}'; e++);
                            if(e >= comp->ntok || (meg4.src[tok[e].pos] != ',' && meg4.src[tok[e].pos] != '}')) {
                                code_error(tok[e].pos, lang[ERR_SYNTAX]); return 0;
                            }
                            if(!comp_eval(comp, s + 1, e, &i)) return 0;
                            s = e;
                        break;
                        default: code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0;
                    }
                    if((k = comp_addid(comp, &comp->tok[j], T(T_DEF, T_I32))) < 0) { code_error(comp->tok[j].pos, lang[ERR_ALRDEF]); return 0; }
                    comp->id[k].f[0] = i;
                    if(tok[s].type == HL_D && tok[s].id == ',') s++;
                }
                if(!skip(comp, s, '}')) return 0;
            } else
            if(tok[s].id == C_STRUCT || tok[s].id == C_UNION) {
                /* struct */
                if(P != 0 || B != 0 || S != 0) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                s++; if(!(s = skip(comp, s, '{'))) return 0;
                for(i = 0; s + 1 < comp->ntok && (tok[s].type != HL_D || tok[s].id != '}'); i++) {
                    if(tok[s].type != HL_T) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    s++;
                }
                if(s + 2 >= comp->ntok || tok[s].type != HL_D || tok[s].id != '}' || tok[s + 1].type != HL_V ||
                    tok[s + 2].type != HL_D || tok[s + 2].id != ';') { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                s += 2; /* FIXME: struct name in tok[s - 1] */
            } else
            if((tok[s + 1].type == HL_F || (s + 2 < comp->ntok && meg4.src[tok[s + 1].pos] == '*' && tok[s + 2].type == HL_F))) {
                if(P != 0 || B != 0 || S != 0) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                i = s++; p = T_FUNC; if(tok[s].type == HL_O) { s++; p = T_FUNCP; }
                j = tf = s++;
                if(!skip(comp, s, '(')) return 0;
                /* function */
                if((i = comp_addid(comp, &tok[s - 1], T(p, tok[i].id))) < 0) return 0;
                if(tok[s - 1].len == 5 && !memcmp(&meg4.src[tok[s - 1].pos], "setup", 5)) setupid = i;
                if(tok[s - 1].len == 4 && !memcmp(&meg4.src[tok[s - 1].pos], "loop", 4)) loopid = i;
                for(s++, n = 0; s + 1 < comp->ntok && (tok[s].type != HL_D || tok[s].id != ')'); n++) {
                    if(tok[s].type != HL_T) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    if(!n && tok[s].id == T_VOID && tok[s + 1].type == HL_D && tok[s + 1].id == ')') { s++; break; }
                    t = tok[s++].id;
                    if(!(args[n] = gettype(comp, &s, &p, t))) return 0;
                    s++;
                    if(tok[s].type == HL_D) {
                        if(meg4.src[tok[s].pos] == '[') { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                        if(meg4.src[tok[s].pos] == ',') s++;
                    }
                }
                if(s + 1 >= comp->ntok || tok[s + 1].type != HL_D || tok[s + 1].id != '{') { code_error(tok[s + 1].pos, lang[ERR_SYNTAX]); return 0; }
                if(comp_addfunc(comp, i, j, n, args) < 0) { code_error(tok[j].pos, lang[ERR_SYNTAX]); return 0; }
            } else
            if(tok[s + 1].type == HL_V || (meg4.src[tok[s + 1].pos] == '*' && (tok[s + 2].type == HL_V || (meg4.src[tok[s + 2].pos] == '*' &&
              (tok[s + 3].type == HL_V || (meg4.src[tok[s + 3].pos] == '*' && (tok[s + 4].type == HL_V || (meg4.src[tok[s + 4].pos] == '*' &&
              tok[s + 5].type == HL_V)))))))) {
                if(S == 0) {
                    /* global variables */
                    if(P != 0 || B != 0) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                    t = tok[s++].id;
                    if(t >= C_CHAR || s + 1 >= comp->ntok) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                    while(s + 1 < comp->ntok && (tok[s].type != HL_D || meg4.src[tok[s].pos] != ';')) {
                        if(!(m = gettype(comp, &s, &p, t))) return 0;
                        j = s++; if((i = comp_addid(comp, &tok[j], m)) < 0 || !getarr(comp, &s, i, p)) return 0;
                        comp->id[i].o = meg4.dp + MEG4_MEM_USER;
                        if(s >= comp->ntok || tok[s].len != 1 || (meg4.src[tok[s].pos] != '=' && meg4.src[tok[s].pos] != ';' && meg4.src[tok[s].pos] != ','))
                            { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                        if(meg4.dp + comp->id[i].l >= meg4.sp) { code_error(tok[j].pos, lang[ERR_MEMORY]); return 0; }
                        comp_ddbg(comp, j, i);
                        if(meg4.src[tok[s].pos] == '=') {
                            /* we don't know the string constants' addresses yet, so just skip over initialization for now */
                            for(s++; s + 1 < comp->ntok && (tok[s].type != HL_D || meg4.src[tok[s].pos] != ';'); s++) {
                                if(tok[s].type == HL_D) {
                                    if(meg4.src[tok[s].pos] == '{') { S++; } else
                                    if(meg4.src[tok[s].pos] == '}') { S--; if(S < 0) { code_error(tok[s].pos, lang[ERR_NOOPEN]); return 0; } } else
                                    if(meg4.src[tok[s].pos] == ',' && !S) break;
                                }
                            }
                        }
                        meg4.dp += comp->id[i].l;
                        if(tok[s].type == HL_D && meg4.src[tok[s].pos] == ',') s++;
                    }
                    if(!skip(comp, s, ';')) return 0;
                }
            } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        } else
        if(tf != -1 && tok[s].type == HL_V && tok[s].len == 8 && !memcmp(&meg4.src[tok[s].pos], "__FUNC__", 8)) {
            /* we got a __FUNC__ reference, add the current function's name to the string literals */
            if((tok[s].id = comp_addstr(comp, tf)) < 0) return 0;
            comp->str[tok[s].id].n++;
        } else
        if(!S) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
    }
    if(P != 0 || B != 0 || S != 0) {
noclose:for(P = B = S = 0; s >= 0; s--)
            if(tok[s].type == HL_D) {
                switch(meg4.src[tok[s].pos]) {
                    case '(': P--; if(P < 0) { code_error(tok[s].pos, lang[ERR_NOCLOSE]); return 0; } break;
                    case ')': P++; break;
                    case '[': B--; if(B < 0) { code_error(tok[s].pos, lang[ERR_NOCLOSE]); return 0; } break;
                    case ']': B++; break;
                    case '{': S--; if(S < 0) { code_error(tok[s].pos, lang[ERR_NOCLOSE]); return 0; } break;
                    case '}': S++; break;
                }
            }
    }
    /* check setup and loop functions */
    if(setupid == -1 && loopid == -1) { code_error(4, lang[ERR_NOCODE]); return 0; }
    if(setupid != -1 && (comp->id[setupid].t != T(T_FUNC, T_VOID) || comp->f[comp->id[setupid].f[0]].n)) { code_error(comp->id[setupid].p, lang[ERR_BADPROTO]); return 0; }
    if(loopid != -1 && (comp->id[loopid].t != T(T_FUNC, T_VOID) || comp->f[comp->id[loopid].f[0]].n)) { code_error(comp->id[loopid].p, lang[ERR_BADPROTO]); return 0; }
    /* finalize initialized data */
    if(!comp_addinit(comp)) return 0;
    /* save number of global identifiers */
    comp->ng = comp->nid;

    /* third pass, detect global initializers */
    for(s = P = S = 0; s < comp->ntok; s++) {
        if(tok[s].type == HL_D) {
            switch(meg4.src[tok[s].pos]) {
                case '(': P++; break;
                case ')': P--; break;
                case '{': S++; break;
                case '}': S--; break;
            }
        } else
        if(!S && !P && tok[s].type == HL_T && tok[s].id < C_CHAR &&
          (tok[s + 1].type == HL_V || (meg4.src[tok[s + 1].pos] == '*' && (tok[s + 2].type == HL_V || (meg4.src[tok[s + 2].pos] == '*' &&
          (tok[s + 3].type == HL_V || (meg4.src[tok[s + 3].pos] == '*' && (tok[s + 4].type == HL_V || (meg4.src[tok[s + 4].pos] == '*' &&
          tok[s + 5].type == HL_V))))))))) {
            s++;
            while(s + 1 < comp->ntok && meg4.src[tok[s].pos] != ';') {
                while(meg4.src[tok[s].pos] == '*') s++;
                if((i = comp_findid(comp, &tok[s++])) < 0) { code_error(tok[s - 1].pos, lang[ERR_UNDEF]); return 0; }
                while(s + 1 < comp->ntok && meg4.src[tok[s].pos] == '[')
                    for(s++; s + 1 < comp->ntok && meg4.src[tok[s - 1].pos] != ']' && meg4.src[tok[s].pos] != ';'; s++);
                if(meg4.src[tok[s].pos] == '=') {
                    if(!(e = getinit(comp, s + 1, i, comp->id[i].l, 0, comp->id[i].o - MEG4_MEM_USER))) return 0;
                    s = e;
                }
                if(tok[s].type == HL_D && meg4.src[tok[s].pos] == ',') s++;
            }
            if(!skip(comp, s, ';')) return 0;
        }
    }

    /* fourth pass, parse statements in functions */
    for(i = 0; i < comp->nf; i++) {
        comp->cf = i; s = comp->f[i].p; comp->nid = comp->ng;
        if(!s) continue;
        s += 2;
        /* add parameters, and record the last function parameter's id */
        if(!comp->f[i].n && tok[s].type == HL_T && tok[s].id == T_VOID) s++;
        for(n = 0; s + 1 < comp->ntok && meg4.src[tok[s].pos] != ')'; n++) {
            while(tok[s].type != HL_V) s++;
            if((j = comp_addid(comp, &tok[s], comp->f[i].t[n])) < 0) return 0;
            comp->id[j].o = n * 4;
            s++;
        }
        comp->pf = comp->nid;
        comp->id[comp->f[i].id].o = comp->nc;
        comp->lf = comp->ls = 0;
        comp_cdbg(comp, comp->f[i].p);
        if(!(s = statement(comp, s + 1, 0, 0, NULL)) || !comp_chkids(comp, comp->pf)) return 0;
        if(comp->ls != C_RETURN && comp->ls != C_GOTO) {
            if(comp->id[comp->f[i].id].t != T(T_FUNC, T_VOID)) { code_error(tok[s - 1].pos, lang[ERR_NORET]); return 0; }
            comp_gen(comp, BC_RET);
        }
    }
    return 1;
}

#endif /* NOEDITORS */
