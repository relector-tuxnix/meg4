/*
 * meg4/comp.c
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
 * @brief Common compiler routines
 *
 */

#include "meg4.h"

#ifndef NOEDITORS
#define BDEF_IMPL
#include "editors/editors.h"    /* for the highlighter functions and code_error */
#include "api.h"
#include <errno.h>
#include <math.h>
uint32_t gethex(uint8_t *ptr, int len);
int casecmp(char *s1, char *s2, int len);

#define TOK_DBG 0

/* precedence levels, must match O_ defines in cpu.h */
static int prec[] = {
    /* assignments */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* logical */       2, 3, 4,
    /* bitwise */       5, 6, 7,
    /* conditionals */  8, 8, 9, 9, 9, 9,
    /* arithmetic */    10, 10, 11, 11, 12, 12, 12, 13,
    /* unaries */       14, 15, 16, 17,
    /* field select */  18, 19,
    20
};

/**
 * Debug helpers
 */
#if TOK_DBG
#include <stdio.h>

void comp_dump(compiler_t *comp, int s)
{
    int j,n;
    printf("s=%d type=%x id=%u pos=%4d '", s,comp->tok[s].type, comp->tok[s].id, comp->tok[s].pos);
    if(comp->tok[s].pos >= 0)
        for(n = comp->tok[s].pos, j = 0; j < comp->tok[s].len; n++, j++)
            if(meg4.src[n]=='\n') printf("\\n"); else printf("%c", meg4.src[n]);
    printf("'");
    if(comp->tok[s].type == HL_S && comp->str) printf(" str addr %x len %u", comp->str[comp->tok[s].id].n, comp->str[comp->tok[s].id].l);
    printf("\n");
}

void comp_dumpfunc(compiler_t *comp)
{
    int i;
    printf("------------functions-----------\n");
    for(i = 0; i < comp->nf; i++) {
        printf("%3u. id=%u p=%u n=%u ",i,comp->f[i].id,comp->f[i].p,comp->f[i].n);
        if(comp->f[i].id >= 0 && comp->f[i].id < comp->nid)
            printf("(name='%s' t=%02x f[0]=%u o=%u)",comp->id[comp->f[i].id].name,comp->id[comp->f[i].id].t,comp->id[comp->f[i].id].f[0],
                comp->id[comp->f[i].id].o);
        else
            printf("(invalid id?)");
        printf("\n");
    }
    printf("--------------------------------\n");
}
#endif

/**
 * Convert a number to constant
 */
int comp_const(compiler_t *comp, int s, int type, int *i, float *f)
{
    tok_t *tok = &comp->tok[s];
    errno = 0;
    if(type == T(T_SCALAR, T_FLOAT) && f) {
        *f = (float)strtod(&meg4.src[tok->pos], NULL);
    } else
    if(type >= T(T_SCALAR, T_I8) && type <= T(T_SCALAR, T_U32) && i) {
        if(meg4.code_type) {
            /* BASIC way of hexadecimal */
            if(meg4.src[tok->pos] == '$')
                *i = (int)strtol(&meg4.src[tok->pos + 1], NULL, 16);
            else
                *i = (int)strtol(&meg4.src[tok->pos], NULL, 10);
        } else
        if(meg4.src[tok->pos] == '0') {
            if((meg4.src[tok->pos + 1] == 'x' || meg4.src[tok->pos + 1] == 'X'))
                *i = (int)strtol(&meg4.src[tok->pos + 2], NULL, 16);
            else if((meg4.src[tok->pos + 1] == 'b' || meg4.src[tok->pos + 1] == 'B'))
                *i = (int)strtol(&meg4.src[tok->pos + 2], NULL, 2);
            else
                *i = (int)strtol(&meg4.src[tok->pos], NULL, 8);
        } else
            *i = (int)strtol(&meg4.src[tok->pos], NULL, 10);
    } else { code_error(tok->pos, lang[ERR_BADARG]); return 0; }
    if(errno == ERANGE) { code_error(tok->pos, lang[ERR_RANGE]); return 0; }
    return 1;
}

/**
 * Find an identifier
 */
int comp_findid(compiler_t *comp, tok_t *tok)
{
    int i;

    if(tok->len > 255) {
        code_error(tok->pos, lang[ERR_TOOLNG]);
        return -2;
    }
    if(comp->nid)
        for(i = comp->nid - 1; i >= 0; i--)
            if(!memcmp(comp->id[i].name, &meg4.src[tok->pos], tok->len) && !comp->id[i].name[tok->len])
                return i;
    return -1;
}

/**
 * Add an identifier with type
 */
int comp_addid(compiler_t *comp, tok_t *tok, int type)
{
    int i = comp_findid(comp, tok), j, k;

    /* too long or already defined? */
    if(i == -2) return -1;
    if(i >= 0) { code_error(tok->pos, lang[ERR_ALRDEF]); return -1; }
    /* types and keywords are not allowed as identifiers */
    for(k = HL_T; k <= HL_K; k++)
        for(j = 0; comp->r[k][j]; j++)
            if(!casecmp(comp->r[k][j], &meg4.src[tok->pos], tok->len) && !comp->r[k][j][tok->len])
                { code_error(tok->pos, lang[ERR_ALRDEF]); return -1; }
    if((type >> 4) == T_STR) { type = T((type & 15) + 1, T_I8); }
    /* add it to the list */
    i = comp->nid++;
    comp->id = (idn_t*)realloc(comp->id, comp->nid * sizeof(idn_t));
    if(!comp->id) { comp->nid = 0; return -1; }
    memset(&comp->id[i], 0, sizeof(idn_t));
    memcpy(comp->id[i].name, &meg4.src[tok->pos], tok->len);
    comp->id[i].p = tok->pos;
    comp->id[i].t = type;
    if((type & 15) == T_SCALAR)
        switch(type >> 4) {
            case T_I8: case T_U8: comp->id[i].s = 1; break;
            case T_I16: case T_U16: comp->id[i].s = 2; break;
            case T_I32: case T_U32: case T_FLOAT: case T_STR: comp->id[i].s = 4; break;
        }
    else comp->id[i].s = 4;
    comp->id[i].l = comp->id[i].s;
    return i;
}

/**
 * Add a string literal (handle escapes, concatenation and deduplication)
 */
int comp_addstr(compiler_t *comp, int t)
{
    tok_t *tok = &comp->tok[t];
    char tmp[256], *s, *e, *d = tmp;
    int i, l;

    s = &meg4.src[tok->pos];
    if(*s != '\"' && tok->len > (*s == '\'' ? 6 : 255)) {
        code_error(tok->pos, lang[ERR_TOOLNG]);
        return -1;
    }
    memset(tmp, 0, sizeof(tmp));
    if(*s == '\"' || *s == '\'') {
        for(s++, e = &meg4.src[tok->pos + tok->len - 1]; s < e; s++) {
            if(d >= tmp + sizeof(tmp) - 1) { code_error(tok->pos, lang[ERR_TOOLNG]); return -1; }
            if(*s == '\\') {
                s++;
                switch(*s) {
                    case '\\': *d++ = '\\'; break;
                    case '\"': *d++ = '\"'; break;
                    case '\'': *d++ = '\''; break;
                    case 't': *d++ = '\t'; break;
                    case 'r': *d++ = '\r'; break;
                    case 'n': *d++ = '\n'; break;
                    case 'e': *d++ = 0x1b; break;
                    case '0': *d++ = 0; break;
                    case 'x': s++; *d++ = (uint8_t)gethex((uint8_t*)s, 2); s++; break;
                    default: code_error(s - meg4.src - 1, lang[ERR_SYNTAX]); return -1;
                }
            } else *d++ = *s;
        }
        l = d - tmp;
        if(meg4.src[tok->pos] == '\'') {
            tok->type = HL_C; memcpy(&tok->id, tmp, 4);
            return 0;
        }
        if(t > 0 && comp->tok[t - 1].type == HL_S) {
            i = comp->tok[t - 1].id;
            if(comp->str[i].l + l > 255) {
                code_error(tok->pos, lang[ERR_TOOLNG]);
                return -1;
            }
            if(!comp->str[i].n) {
                memcpy(&comp->str[i].str[comp->str[i].l], tmp, l);
                comp->str[i].l += l;
                tok->id = i;
                return i;
            } else {
                memmove(tmp + comp->str[i].l, tmp, l);
                memcpy(tmp, comp->str[i].str, comp->str[i].l);
                l += comp->str[i].l;
            }
        }
    } else {
        l = tok->len;
        memcpy(tmp, s, l);
    }
    for(i = 0; i < comp->nstr; i++)
        if(comp->str[i].l == l && !memcmp(comp->str[i].str, tmp, l)) { comp->str[i].n++; break; }
    if(i >= comp->nstr) {
        i = comp->nstr++;
        comp->str = (cstr_t*)realloc(comp->str, comp->nstr * sizeof(cstr_t));
        if(!comp->str) { comp->nstr = 0; return -1; }
        memset(&comp->str[i], 0, sizeof(cstr_t));
        memcpy(comp->str[i].str, tmp, l);
        comp->str[i].l = l;
    }
    if(tok->type == HL_S) tok->id = i;
    return i;
}

/**
 * Add program header
 */
void comp_addhdr(compiler_t *comp)
{
    /* make space for global pointers */
    comp_gen(comp, 0);  /* code debug symbols */
    comp_gen(comp, 0);  /* data debug symbols */
    comp_gen(comp, 0);  /* setup function pointer */
    comp_gen(comp, 0);  /* loop function pointer */
}

/**
 * Finalize initialized data
 */
int comp_addinit(compiler_t *comp)
{
    int i, j;

    /* add strings to data */
    for(i = 0; i < comp->nstr; i++) {
        if(meg4.dp + comp->str[i].l + 1 >= meg4.sp) { code_error(meg4.code_type ? 6 : 4, lang[ERR_MEMORY]); return 0; }
        comp->str[i].n = meg4.dp + MEG4_MEM_USER;
        memcpy(meg4.data + meg4.dp, comp->str[i].str, comp->str[i].l + 1);
        meg4.dp += comp->str[i].l + 1;
    }
    /* get operator's precedence */
    for(j = 0; j < comp->ntok; j++)
        if(comp->tok[j].type == HL_O) {
            for(i = 0; i < O_LAST && !(!casecmp(comp->ops[i], &meg4.src[comp->tok[j].pos], comp->tok[j].len) && !comp->ops[i][comp->tok[j].len]); i++);
            if(i >= O_LAST) { code_error(comp->tok[j].pos, lang[ERR_SYNTAX]); return 0; }
            comp->tok[j].id = i;
        } else
        if(comp->tok[j].type == HL_D && comp->tok[j].id == '[') { comp->tok[j].type = HL_O; comp->tok[j].id = O_LAST; }
    /* add program header */
    comp_addhdr(comp);

#if TOK_DBG
printf("------------tokens-----------\n");
    for(i = 0; i < comp->ntok; i++) comp_dump(comp, i);
printf("-----------------------------\n");
#endif

    return 1;
}

/**
 * Add a function
 */
int comp_addfunc(compiler_t *comp, int id, int s, int nargs, int *argtypes)
{
    int i, f = -1;

    if(id < 0 || id >= comp->nid || (comp->id[id].t & 15) != T_FUNC || nargs > N_ARG) return -1;
    /* the first function must be setup, and the second loop, others afterwards */
    if(!comp->f) {
        comp->f = (func_t*)malloc(2 * sizeof(func_t)); if(!comp->f) { comp->nf = 0; return -1; }
        memset(comp->f, 0, 2 * sizeof(func_t)); comp->nf = 2;
    }
    if(!strcmp(comp->id[id].name, "setup")) { f = 0; comp->id[id].r++; } else
    if(!strcmp(comp->id[id].name, "loop")) { f = 1; comp->id[id].r++; } else {
        f = comp->nf++;
        comp->f = (func_t*)realloc(comp->f, comp->nf * sizeof(func_t)); if(!comp->f) { comp->nf = 0; return -1; }
        memset(&comp->f[f], 0, sizeof(func_t));
    }
    comp->f[f].id = id; comp->id[id].f[0] = f;
    comp->f[f].p = s;
    comp->f[f].n = nargs;
    for(i = 0; i < nargs; i++)
        comp->f[f].t[i] = argtypes[i];
    return f;
}

/**
 * Add a label
 */
int comp_addlbl(compiler_t *comp, int s)
{
    tok_t *tok = comp->tok;
    int i, j;

    /* check if this label was already referenced */
    for(i = comp->pf; i < comp->nid; i++)
        if(!memcmp(comp->id[i].name, &meg4.src[tok[s].pos], tok[s].len) && !comp->id[i].name[tok[s].len]) {
            if(comp->id[i].t != T(T_LABEL, T_VOID) || comp->id[i].o) { code_error(tok[s].pos, lang[ERR_ALRDEF]); return 0; }
            break;
        }
    if(i >= comp->nid && (i = comp_addid(comp, &tok[s], T(T_LABEL, T_VOID))) < 0) return 0;
    comp->id[i].f[2] = comp->cf;
    comp->id[i].o = comp->nc;
    if(!comp->code || !comp->id[i].f[1]) return 1;
    /* resolve forward references */
    for(i = comp->id[i].f[1], comp->id[i].f[1] = 0; i;) {
        j = comp->code[i];
        comp->code[i] = comp->nc;
        i = j;
    }
    return 1;
}

/**
 * Add a jump to label
 */
int comp_addjmp(compiler_t *comp, int s)
{
    int i;

    /* check for valid label */
    if(s >= comp->ntok || comp->tok[s].type != HL_V) { code_error(comp->tok[s].pos, lang[ERR_SYNTAX]); return 0; }
    /* if it's already exists as a referenced label */
    for(i = comp->pf; i < comp->nid; i++)
        if(!memcmp(comp->id[i].name, &meg4.src[comp->tok[s].pos], comp->tok[s].len) && !comp->id[i].name[comp->tok[s].len]) {
            if(comp->id[i].t != T(T_LABEL, T_VOID)) { code_error(comp->tok[s].pos, lang[ERR_BADARG]); return 0; }
            if(comp->id[i].f[2] != comp->cf) { code_error(comp->tok[s].pos, lang[ERR_BOUNDS]); return 0; }
            break;
        }
    if(i >= comp->nid && (i = comp_addid(comp, &comp->tok[s], T(T_LABEL, T_VOID))) < 0) return 0;
    /* add true reference or forward reference */
    if(comp->id[i].o) { comp_gen(comp, comp->id[i].o); }
    else { comp_gen(comp, comp->id[i].f[1]); comp->id[i].f[1] = comp->nc - 1; }
    return 1;
}

/**
 * Check if all referenced labels were defined, and there are no unused variables
 */
int comp_chkids(compiler_t *comp, int s)
{
    int i;
    for(i = s; i < comp->nid; i++) {
        if((comp->id[i].t & 15) == T_LABEL && (comp->id[i].o < 0 || comp->id[i].f[1])) { code_error(comp->id[i].p, lang[ERR_UNDEF]); return 0; }
        if((comp->id[i].t & 15) != T_DEF && comp->id[i].r < 1) { main_log(1, "warning, defined but unused: '%s'", comp->id[i].name); }
    }
    return 1;
}

/**
 * Evaluate one operator
 */
static int eval(tok_t *a, tok_t *b, int op, int l)
{
    if(l == 1 && op == O_SUB) { b->id = -b->id; return l; }
    if(l < (op == O_NOT || op == O_BNOT ? 1 : 2)) { code_error(b->pos, lang[ERR_SYNTAX]); return -1; }
    switch(op) {
        case O_EXP: a->id = (int)pow((double)a->id, (double)b->id); break;
        case O_MUL: a->id *= b->id; break;
        case O_DIV: if(b->id == 0) { code_error(b->pos, lang[ERR_DIVZERO]); return -1; } a->id /= b->id; break;
        case O_MOD: if(b->id == 0) { code_error(b->pos, lang[ERR_DIVZERO]); return -1; } a->id %= b->id; break;
        case O_ADD: a->id += b->id; break;
        case O_SUB: a->id -= b->id; break;
        case O_SHL: a->id <<= b->id; break;
        case O_SHR: a->id >>= b->id; break;
        case O_BAND: a->id &= b->id; break;
        case O_BOR: a->id |= b->id; break;
        case O_XOR: a->id ^= b->id; break;
        case O_NE: a->id = (a->id != b->id); break;
        case O_LE: a->id = (a->id <= b->id); break;
        case O_GE: a->id = (a->id >= b->id); break;
        case O_LT: a->id = (a->id < b->id); break;
        case O_GT: a->id = (a->id > b->id); break;
        case O_EQ: a->id = (a->id == b->id); break;
        case O_LAND: a->id = (a->id && b->id); break;
        case O_LOR: a->id = (a->id || b->id); break;
        case O_NOT: b->id = !b->id; break;
        case O_BNOT: b->id = ~b->id; break;
    }
    return (op == O_NOT || op == O_BNOT ? l : l - 1);
}

/**
 * Calculate a postfix expression
 */
int comp_eval(compiler_t *comp, int s, int e, int *val)
{
    tok_t *tok = comp->tok, stk[N_EXPR], tmp[N_EXPR];
    int i, j, l = 0, t = -1;

    for(i = s; i < e; i++) {
        if(l >= N_EXPR - 1 || t >= N_EXPR - 1) { code_error(tok[i].pos, lang[ERR_TOOCLX]); return 0; }
        memcpy(&tmp[l], &tok[i], sizeof(tok_t));
        switch(tok[i].type) {
            case HL_V:
                if((j = comp_findid(comp, &tok[i])) >= 0 && j > MEG4_NUM_API && j < MEG4_NUM_API + MEG4_NUM_BDEF) {
                    /* numeric built-in define */
                    if(j == MEG4_NUM_API + 1) {
                        /* __LINE__ (context dependent) */
                        for(j = 0, tmp[l].id = 1; (uint32_t)j < meg4.src_len && j < tok[i].pos; j++) if(meg4.src[j] == '\n') tmp[l].id++;
                    } else
                        /* all the rest */
                        tmp[l].id = meg4_bdefs[j - MEG4_NUM_API].val;
                    tmp[l++].type = HL_N;
                } else { code_error(tok[i].pos, lang[ERR_BADARG]); return 0; }
            break;
            case HL_N:
                if(!comp_const(comp, i, T(T_SCALAR, T_I32), &tmp[l].id, NULL)) return 0;
                l++;
            break;
            case HL_C: tmp[l++].type = HL_N; break;
            case HL_D:
                if(tok[i].id == '(') memcpy(&stk[++t], &tok[i], sizeof(tok_t)); else
                if(tok[i].id == ')') {
                    while(t > -1 && stk[t].type != HL_D)
                        if((l = eval(&tmp[l - 2], &tmp[l - 1], stk[t--].id, l)) < 0) return 0;
                    if(t < 0 || stk[t].type != HL_D) { code_error(tok[i].pos, lang[ERR_NOOPEN]); return 0; }
                    t--;
                } else { code_error(tok[i].pos, lang[ERR_SYNTAX]); return 0; }
            break;
            case HL_O:
                for(j = O_LOR; j <= O_BNOT && !(!casecmp(comp->ops[j], &meg4.src[tok[i].pos], tok[i].len) && !comp->ops[j][tok[i].len]); j++);
                if(j <= O_BNOT) tmp[l].id = j; else { code_error(tok[i].pos, lang[ERR_NOTHERE]); return 0; }
                while(t > -1 && stk[t].type != HL_D && prec[stk[t].id] >= prec[j])
                    if((l = eval(&tmp[l - 2], &tmp[l - 1], stk[t--].id, l)) < 0) return 0;
                memcpy(&stk[++t], &tmp[l], sizeof(tok_t));
            break;
            default: code_error(comp->tok[s].pos, lang[ERR_NOTSTC]); return 0;
        }
    }
    while(t > -1) {
        if(stk[t].type == HL_D) { code_error(stk[t].pos, lang[ERR_NOOPEN]); return 0; }
        if((l = eval(&tmp[l - 2], &tmp[l - 1], stk[t--].id, l)) < 0) return 0;
    }
    if(l != 1 || tmp[0].type != HL_N) { code_error(comp->tok[s].pos, lang[ERR_NOTSTC]); return 0; }
    *val = tmp[0].id;
    return 1;
}

/**
 * Add an expression to output (reentrant)
 */
int comp_expr(compiler_t *comp, int s, int e, int *type, int lvl)
{
    func_t *func;
    tok_t *tok = comp->tok;
    int i, j, k, l, m, n, o, p, q, t[N_ARG], T = meg4.code_type ? T(T_SCALAR, T_FLOAT) : T(T_SCALAR, T_I32);
    float f;

/*printf("expr s %d e %d lvl %d type %x\n",s,e,lvl,tok[s].type);*/
    if(s >= e || s >= comp->ntok) return s + 1;
    if(!type) type = &T;

    /* --- unary --- */
    if(tok[s].type == HL_N) {
        /* number literal */
        if(!comp_const(comp, s, *type, &i, &f)) return 0;
        if(*type == T(T_SCALAR, T_FLOAT)) {
            memcpy(&i, &f, 4);
            comp->lc = comp->nc;
            comp_gen(comp, BC_CF);
            comp_gen(comp, i);
        } else {
            comp->lc = comp->nc;
            comp_gen(comp, BC_CI);
            comp_gen(comp, i);
            if(*type >= T(T_SCALAR, T_U8) && *type < T(T_SCALAR, T_U32))
                *type = T(T_SCALAR, T_U32);
            else
                *type = T(T_SCALAR, T_I32);
        }
        s++;
    } else
    if(tok[s].type == HL_C) {
        /* character literal */
        comp->lc = comp->nc;
        comp_gen(comp, BC_CI);
        comp_gen(comp, tok[s].id);
        s++; *type = T(T_SCALAR, T_I32);
    } else
    if(tok[s].type == HL_S) {
        /* string literal */
        comp->lc = comp->nc;
        comp_gen(comp, BC_CI);
        comp_gen(comp, comp->str[tok[s].id].n);
        s++; *type = T(T_PTR, T_I8);
    } else
    if((tok[s].type == HL_K || tok[s].type == HL_F) && tok[s].len == 6 && !memcmp(&meg4.src[tok[s].pos], "sizeof", 6)) {
        /* sizeof */
        s++; n = 0; if(tok[s].type == HL_D && tok[s].id == '(') { n = 1; s++; }
        if(s >= e || (tok[s].type != HL_T && tok[s].type != HL_V)) { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        if(tok[s].type == HL_N || tok[s].type == HL_C) { i = 4; s++; } else
        if(tok[s].type == HL_S) { i = comp->str[tok[s].id].l; s++; } else
        if(tok[s].type == HL_V) {
            if((j = comp_findid(comp, &tok[s])) >= 0) {
                if(j == MEG4_NUM_API) {
                    i = comp->str[tok[s].id].l; s++;
                } else {
                    i = comp->id[j].l; k = 0; s++;
                    while(tok[s].type == HL_O && tok[s].id == O_LAST) {
                        if(k >= N_DIM || !comp->id[j].a[k]) { code_error(tok[s].pos, lang[ERR_NUMARG]); return 0; }
                        i /= comp->id[j].a[k++];
                        for(l = s; s < e && meg4.src[tok[s].pos] != ']'; s++);
                        if(s >= e || meg4.src[tok[s].pos] != ']') { code_error(tok[l].pos, lang[ERR_NOCLOSE]); return 0; }
                        s++;
                    }
                }
            } else { code_error(tok[s].pos, lang[ERR_UNDEF]); return 0; }
        } else {
            switch(tok[s++].id) {
                case T_VOID: i = 0; break;
                case T_I8: case T_U8: i = 1; break;
                case T_I16: case T_U16: i = 2; break;
                default: i = 4; break;
            }
            while(s < e && meg4.src[tok[s].pos] == '*') { i = 4; s++; }
        }
        if(n && s < e && tok[s].type == HL_D && tok[s].id == ')') s++;
        comp->lc = comp->nc;
        comp_gen(comp, BC_CI);
        comp_gen(comp, i);
        *type = T(T_SCALAR, T_I32);
    } else
    if(tok[s].type == HL_V) {
        /* variable (or built-in define) */
        if((j = comp_findid(comp, &tok[s])) >= 0) {
            if(j < MEG4_NUM_API + MEG4_NUM_BDEF) {
                /* built-in defines */
                comp->lc = comp->nc;
                comp_gen(comp, BC_CI);
                *type = T(T_SCALAR, T_I32);
                if(j == MEG4_NUM_API) {
                    /* __FUNC__ (context dependent) */
                    comp_gen(comp, comp->str[tok[s].id].n);
                    *type = T(T_PTR, T_I8);
                } else
                if(j == MEG4_NUM_API + 1) {
                    /* __LINE__ (context dependent) */
                    for(i = 0, l = 1; (uint32_t)i < meg4.src_len && i < tok[s].pos; i++) if(meg4.src[i] == '\n') l++;
                    comp_gen(comp, l);
                } else
                    /* all the rest */
                    comp_gen(comp, meg4_bdefs[j - MEG4_NUM_API].val);
            } else
            if((comp->id[j].t & 15) >= T_SCALAR) {
                /* global and local variables */
                comp->id[j].r++;
                if(j < comp->ng) { comp->lc = comp->nc; comp_gen(comp, BC_CI); comp_gen(comp, comp->id[j].o); }
                else comp_gen(comp, (comp->id[j].o << 8) | BC_ADR);
                *type = comp->id[j].t;
                if(s + 1 < e && tok[s + 1].type == HL_O && tok[s + 1].id == O_LAST) {
                    m = 0;
arrayindex:         s += 2; q = 1;
                    for(o = s, p = n = l = 0; o < e; o++) {
                        if(tok[o].type == HL_O && tok[o].id == O_LAST) { n++; } else
                        if(tok[o].type == HL_D) {
                            if(tok[o].id == '(') { p++; } else
                            if(tok[o].id == ')') { p--; } else
                            if(tok[o].id == ']') { n--; }
                            if((m && (p < 0 || (tok[o].id == ',' && !p))) || (!m && tok[o].id == ']' && n == -1)) {
                                if(l >= N_DIM || !comp->id[j].a[l]) { code_error(tok[s - 1].pos, lang[ERR_NUMARG]); return 0; }
                                comp_push(comp, T(T_SCALAR, T_I32));
                                t[0] = T(T_SCALAR, T_I32);
                                if(!(k = comp_expr(comp, s, o, &t[0], O_CND))) return 0;
                                if(k != o || (t[0] & 15) != T_SCALAR) { code_error(tok[s - 1].pos, lang[ERR_BADARG]); return 0; }
                                /* subtract the first index if it's not zero */
                                if(comp->id[j].f[l]) {
                                    comp_push(comp, T(T_SCALAR, T_I32));
                                    comp->lc = comp->nc;
                                    comp_gen(comp, BC_CI);
                                    comp_gen(comp, comp->id[j].f[l]);
                                    comp_gen(comp, BC_SUBI);
                                }
                                comp_gen(comp, (comp->id[j].a[l] << 8) | BC_BND);
                                /* multiply by dimension size */
                                q *= comp->id[j].a[l];
                                comp_gen(comp, BC_PUSHI);
                                comp->lc = comp->nc;
                                comp_gen(comp, BC_CI);
                                comp_gen(comp, comp->id[j].l / q);
                                comp_gen(comp, BC_MULI);
                                comp_gen(comp, BC_ADDI);
                                (*type)--; l++;
                                if(m && p < 0) break;
                                if(!m) {
                                    if(o + 1 < comp->ntok && tok[o + 1].type == HL_O && tok[o + 1].id == O_LAST) o++;
                                    else break;
                                    n = 0;
                                }
                                o++;
                                s = o;
                            }
                        }
                    }
                    if(o >= e || meg4.src[tok[o].pos] != (m ? ')' : ']')) { code_error(tok[s].pos, lang[ERR_NOCLOSE]); return 0; }
                    s = o;
                }
                comp_load(comp, *type);
            } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
            s++;
        } else { code_error(tok[s].pos, lang[ERR_UNDEF]); return 0; }
    } else
    /* hack: in BASIC these are not really variables, but actually argumentless system function calls encoded as "types" */
    if(meg4.code_type == 1 && tok[s].type == HL_T) {
        /* MEG-4 API must be lowercase, but BASIC keywords are case-insensitive */
        for(j = 0; j < MEG4_NUM_API; j--)
            if(!casecmp(comp->id[j].name, &meg4.src[tok[s].pos], tok[s].len) && !comp->id[j].name[tok[s].len]) break;
        if(j < MEG4_NUM_API) {
            comp_gen(comp, (j << 8) | BC_SCALL);
            *type = T((comp->id[j].t & 15) == T_FUNCP ? T_PTR : T_SCALAR, comp->id[j].t >> 4);
            s++;
        } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
    } else
    if(tok[s].type == HL_F) {
        /* function call */
        if((j = comp_findid(comp, &tok[s])) >= 0) {
            if((comp->id[j].t & 15) >= T_FUNC && (comp->id[j].t & 15) <= T_FUNCP) {
                s++; if(tok[s].type != HL_D || tok[s].id != '(') { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                comp->id[j].r++;
                /* tricky, because we must parse the arguments in reverse order */
                for(o = s + 1, p = n = 0; o < e; o++)
                    if(tok[o].type == HL_D) {
                        if(tok[o].id == '(') { p++; } else
                        if(tok[o].id == ')') { p--; if(p < 0) break; } else
                        if(tok[o].id == ',' && !p) n++;
                    }
                if(o >= e || tok[o].type != HL_D || tok[o].id != ')') { code_error(tok[s].pos, lang[ERR_NOCLOSE]); return 0; }
                if(o > s + 1) n++;
                func = &comp->f[comp->id[j].f[0]];
                /* get argument types. Tricky too, because of variadic arguments */
                if(j < MEG4_NUM_API) {
                    if(meg4_api[j].varg) {
                        /* first argument must be a format string */
                        if(tok[s + 1].type != HL_S) { code_error(tok[s + 1].pos, lang[ERR_BADARG]); return 0; }
                        t[0] = T(T_PTR, T_I8);
                        for(i = l = 1; i < tok[s + 1].len - 1; i++)
                            if(meg4.src[tok[s + 1].pos + i] == '%') {
                                if(l >= N_ARG) { code_error(tok[s - 1].pos, lang[ERR_NUMARG]); return 0; }
                                i++;
                                if(meg4.src[tok[s + 1].pos + i] == '%') continue;
                                for(; meg4.src[tok[s + 1].pos + i] == '.' || (meg4.src[tok[s + 1].pos + i] >= '0' && meg4.src[tok[s + 1].pos + i] <= '9'); i++);
                                switch(meg4.src[tok[s + 1].pos + i]) {
                                    case 'p': t[l++] = T(T_PTR, T_U8); break;
                                    case 's': t[l++] = T(T_PTR, T_I8); break;
                                    case 'u': case 'x': case 'X': case 'b': case 'o': t[l++] = T(T_SCALAR, T_U32); break;
                                    case 'c': case 'd': t[l++] = T(T_SCALAR, T_I32); break;
                                    case 'f': t[l++] = T(T_SCALAR, T_FLOAT); break;
                                    default: code_error(tok[s + 1].pos + i, lang[ERR_BADFMT]); return 0;
                                }
                            }
                    } else {
                        /* convert the API argument bitmask to a type list */
                        for(k = 0, l = 1; k < 16 && k < N_ARG; k++, l <<= 1) {
                            if(meg4_api[j].fmsk & l) t[k] = T(T_SCALAR, T_FLOAT); else
                            if((meg4_api[j].smsk & l)) t[k] = T(T_PTR, T_I8); else
                            if((meg4_api[j].amsk & l) || (meg4_api[j].umsk & l)) t[k] = T(T_SCALAR, T_U32); else
                                t[k] = T(T_SCALAR, T_I32);
                        }
                        l = meg4_api[j].narg;
                    }
                } else {
                    /* local function */
                    l = func->n; memcpy(t, func->t, sizeof(t));
                }
                if(n > N_ARG || n != l) { code_error(tok[s].pos, lang[ERR_NUMARG]); return 0; }
                if(n > 0) {
                    for(k = o, l = o - 1, p = 0, m = n - 1; l > s; l--) {
                        if(tok[l].type == HL_D) { if(tok[l].id == '(') { p--; } else if(tok[l].id == ')') { p++; } }
                        if(l == s + 1 || (tok[l].type == HL_D && tok[l].id == ',' && !p)) {
                            q = t[m]; if(tok[l].type == HL_D && tok[l].id) l++;
                            comp_cdbg(comp, l);
                            if(comp_expr(comp, l, k, &q, O_LET) != k) return 0;
                            if((q & 15) != (t[m] & 15) && !((t[m] & 15) == T_PTR && (q == T(T_SCALAR,T_I32) || q == T(T_SCALAR,T_U32))))
                                { code_error(tok[l].pos, lang[ERR_BADARG]); return 0; }
                            comp_push(comp, q);
                            m--; if(m >= 0) l--;
                            k = l;
                        }
                    }
                }
                comp_cdbg(comp, s - 1);
                if(j < MEG4_NUM_API) comp_gen(comp, BC_SCALL | (j << 8));
                else if(comp->id[j].o > 0) { comp_gen(comp, BC_CALL); comp_gen(comp, comp->id[j].o); }
                else {
                    /* forward function call */
                    comp_gen(comp, BC_CALL);
                    comp_gen(comp, comp->id[j].f[1]);
                    comp->id[j].f[1] = comp->nc - 1;
                }
                if(n > 0) comp_gen(comp, (n << 10) | BC_SP);
                s = o;
            } else
            if(meg4.code_type == 1 && (comp->id[j].t & 15) >= T_PTR) {
                /* BASIC thinks of arrays as functions */
                comp->id[j].r++;
                if(j < comp->ng) { comp->lc = comp->nc; comp_gen(comp, BC_CI); comp_gen(comp, comp->id[j].o); }
                else comp_gen(comp, (comp->id[j].o << 8) | BC_ADR);
                *type = comp->id[j].t;
                m = 1; goto arrayindex;
            } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
            *type = T((comp->id[j].t & 15) == T_FUNCP ? T_PTR : T_SCALAR, comp->id[j].t >> 4);
            s++;
        } else { code_error(tok[s].pos, lang[ERR_UNDEF]); return 0; }
    } else
    if(tok[s].type == HL_D && tok[s].id == '(') {
        m = s++; if(s >= e) { code_error(tok[m].pos, lang[ERR_NOCLOSE]); return 0; }
        if(tok[s].type == HL_T) {
            /* casting type */
            t[0] = T(T_SCALAR, tok[s++].id);
            while(s < e && tok[s].type == HL_O && tok[s].id == O_MUL) {
                if((t[0] & 15) >= T_PTR + N_DIM) { code_error(tok[s].pos, lang[ERR_RECUR]); return 0; }
                s++; t[0]++;
            }
            if(s >= e || tok[s].type != HL_D || tok[s].id != ')') { code_error(tok[m].pos, lang[ERR_NOCLOSE]); return 0; }
            s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
            *type = t[0];
        } else {
            if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
            if(s >= e || tok[s].type != HL_D || tok[s].id != ')') { code_error(tok[m].pos, lang[ERR_NOCLOSE]); return 0; }
            s++;
        }
    } else
    if(!meg4.code_type && tok[s].type == HL_O && tok[s].id == O_MUL) {
        /* dereference */
        m = s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
        if((*type & 15) < T_PTR) { code_error(tok[m].pos, lang[ERR_BADDRF]); return 0; }
        comp_load(comp, --(*type));
    } else
    if(tok[s].type == HL_O && tok[s].id == O_BAND) {
        /* address of */
        m = s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
        i = comp->code[--comp->nc];
        if(i < BC_LDB || i > BC_LDF || ((*type) & 15) >= T_PTR + N_DIM) { code_error(tok[m].pos, lang[ERR_BADADR]); return 0; }
        (*type)++;
    } else
    if(tok[s].type == HL_O && tok[s].id == O_NOT) {
        /* logical not */
        s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
        if(comp->nc - 2 == comp->lc && comp->code[comp->lc] == BC_CI)
            comp->code[comp->nc - 1] = !comp->code[comp->nc - 1];
        else
            comp_gen(comp, BC_NOT);
        *type = T(T_SCALAR, T_I32);
    } else
    if(tok[s].type == HL_O && tok[s].id == O_BNOT) {
        /* bitwise not */
        s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
        if(comp->nc - 2 == comp->lc && comp->code[comp->lc] == BC_CI)
            comp->code[comp->nc - 1] = ~comp->code[comp->nc - 1];
        else
            comp_gen(comp, BC_NEG);
        *type = T(T_SCALAR, T_I32);
    } else
    if(tok[s].type == HL_O && tok[s].id == O_ADD) {
        /* unary plus */
        s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
    } else
    if(tok[s].type == HL_O && tok[s].id == O_SUB) {
        /* unary minus */
        s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
        if(*type != T(T_SCALAR, T_FLOAT)) *type = T(T_SCALAR, T_I32);
        if(comp->nc - 2 == comp->lc && comp->code[comp->lc] == BC_CF) {
            memcpy(&f, &comp->code[comp->nc - 1], 4); f *= -1; memcpy(&comp->code[comp->nc - 1], &f, 4);
        } else
        if(comp->nc - 2 == comp->lc && comp->code[comp->lc] == BC_CI)
            comp->code[comp->nc - 1] = -comp->code[comp->nc - 1];
        else {
            comp_push(comp, *type);
            comp->lc = comp->nc;
            if(*type == T(T_SCALAR, T_FLOAT)) {
                f = -1; memcpy(&i, &f, 4); comp_gen(comp, BC_CF); comp_gen(comp, i); comp_gen(comp, BC_MULF);
            } else {
                i = -1; comp_gen(comp, BC_CI); comp_gen(comp, i); comp_gen(comp, BC_MULI);
            }
        }
    } else
    if(tok[s].type == HL_O && (tok[s].id == O_INC || tok[s].id == O_DEC)) {
        /* prefix increment / decrement */
        m = s++; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
        i = comp->code[--comp->nc];
        if(i < BC_LDB || i >= BC_LDF) { code_error(tok[m + 1].pos, lang[ERR_BADLVAL]); return 0; }
        comp_push(comp, T(T_SCALAR, T_I32));
        comp_gen(comp, (((*type & 15) > T_PTR ? 4 : 1) << 8) | (i - BC_LDB + (tok[m].id == O_INC ? BC_INCB : BC_DECB)));
        comp_gen(comp, i);
    } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }

    /* parse operators in top-down precedence order */
    while(s < e && tok[s].type == HL_O && prec[tok[s].id] >= prec[lvl]) {
        t[0] = *type;
        switch(tok[s].id) {
            case O_AOR: case O_AAND: case O_AXOR: case O_ASHL: case O_ASHR:
                i = comp->code[--comp->nc];
                if(i < BC_LDB || i >= BC_LDF) { code_error(tok[s].pos, lang[ERR_BADLVAL]); return 0; }
                comp_push(comp, T(T_SCALAR, T_I32));
                comp_gen(comp, i);
                comp_gen(comp, BC_PUSHI);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
                comp_gen(comp, o - O_AOR + BC_OR);
                comp_store(comp, *type = t[0]);
            break;
            case O_AADD: case O_ASUB: case O_AMUL: case O_ADIV: case O_AMOD: case O_AEXP:
                i = comp->code[--comp->nc];
                if(i < BC_LDB || i > BC_LDF) { code_error(tok[s].pos, lang[ERR_BADLVAL]); return 0; }
                comp_push(comp, T(T_SCALAR, T_I32));
                comp_gen(comp, i);
                comp_gen(comp, i == BC_LDF ? BC_PUSHF : BC_PUSHI);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
                comp_gen(comp, o - O_AADD + (i == BC_LDF ? BC_ADDF : BC_ADDI));
                comp_store(comp, *type = t[0]);
            break;
            case O_LET:
                i = comp->code[--comp->nc];
                if(i < BC_LDB || i > BC_LDF) { code_error(tok[s].pos, lang[ERR_BADLVAL]); return 0; }
                comp_push(comp, T(T_SCALAR, T_I32));
                s++; if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
                comp_store(comp, *type = t[0]);
            break;
            case O_CND:
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JNZ;
                else comp_gen(comp, BC_JZ);
                i = comp->nc; comp_gen(comp, 0);
                m = s++; if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
                if(s >= e || tok[s].type != HL_D || tok[s].id != ':') { code_error(tok[m].pos, lang[ERR_BADCND]); return 0; }
                comp_gen(comp, BC_JMP);
                comp->code[i] = comp->nc + 1; i = comp->nc; comp_gen(comp, 0);
                s++; if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
                comp->code[i] = comp->lc = comp->nc;
            break;
            case O_LOR:
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JZ;
                else comp_gen(comp, BC_JNZ);
                i = comp->nc; comp_gen(comp, 0);
                s++; if(!(s = comp_expr(comp, s, e, type, O_LOR + 1))) return 0;
                comp->code[i] = comp->nc;
                *type = T(T_SCALAR, T_I32);
            break;
            case O_LAND:
                if(comp->code[comp->nc - 1] == BC_NOT) comp->code[comp->nc - 1] = BC_JNZ;
                else comp_gen(comp, BC_JZ);
                i = comp->nc; comp_gen(comp, 0);
                s++; if(!(s = comp_expr(comp, s, e, type, O_LAND + 1))) return 0;
                comp->code[i] = comp->nc;
                *type = T(T_SCALAR, T_I32);
            break;
            case O_BOR: case O_XOR: case O_BAND:
                comp_push(comp, *type);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, o + 1))) return 0;
                comp_gen(comp, o - O_BOR + BC_OR);
                *type = ((t[0] >> 4) < T_U8 || (t[0] >> 4) > T_U32) || ((*type >> 4) < T_U8 || (*type >> 4) > T_U32) ?
                    T(T_SCALAR, T_U32) : T(T_SCALAR, T_I32);
            break;
            case O_EQ: case O_NE:
                comp_push(comp, *type);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, O_LT))) return 0;
                comp_cmp(comp, o, t[0], *type);
                *type = T(T_SCALAR, T_I32);
            break;
            case O_LT: case O_GT: case O_LE: case O_GE:
                comp_push(comp, *type);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, O_SHL))) return 0;
                comp_cmp(comp, o, t[0], *type);
                *type = T(T_SCALAR, T_I32);
            break;
            case O_SHL: case O_SHR:
                comp_push(comp, *type);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, o))) return 0;
                comp_gen(comp, o - O_SHL + BC_SHL);
                *type = ((t[0] >> 4) < T_U8 || (t[0] >> 4) > T_U32) ? T(T_SCALAR, T_U32) : T(T_SCALAR, T_I32);
            break;
            case O_ADD: case O_SUB:
                comp_push(comp, *type);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, O_MUL))) return 0;
                if(t[0] == T(T_SCALAR, T_FLOAT) || *type == T(T_SCALAR, T_FLOAT)) {
                    if(*type != T(T_SCALAR, T_FLOAT)) comp_gen(comp, BC_CNVF);
                    comp_gen(comp, o - O_ADD + BC_ADDF);
                    *type = T(T_SCALAR, T_FLOAT);
                } else {
                    if(o == O_SUB && (t[0] & 15) > T_PTR && t[0] == *type) {
                        comp_gen(comp, BC_SUBI);
                        comp_push(comp, *type);
                        comp->lc = comp->nc;
                        comp_gen(comp, BC_CI);
                        comp_gen(comp, 2);
                        comp_gen(comp, BC_SHR);
                    } else {
                        *type = t[0];
                        if((t[0] & 15) > T_PTR) {
                            comp_push(comp, *type);
                            comp->lc = comp->nc;
                            comp_gen(comp, BC_CI);
                            comp_gen(comp, 2);
                            comp_gen(comp, BC_SHL);
                        }
                        comp_gen(comp, o - O_ADD + BC_ADDI);
                    }
                    *type = ((t[0] >> 4) < T_U8 || (t[0] >> 4) > T_U32) || ((*type >> 4) < T_U8 || (*type >> 4) > T_U32) ?
                        T(T_SCALAR, T_U32) : T(T_SCALAR, T_I32);
                }
            break;
            case O_MUL: case O_DIV: case O_MOD: case O_EXP:
                comp_push(comp, *type);
                o = tok[s++].id; if(!(s = comp_expr(comp, s, e, type, O_INC))) return 0;
                if(t[0] == T(T_SCALAR, T_FLOAT) || *type == T(T_SCALAR, T_FLOAT)) {
                    if(*type != T(T_SCALAR, T_FLOAT)) comp_gen(comp, BC_CNVF);
                    comp_gen(comp, o - O_MUL + BC_MULF);
                    *type = T(T_SCALAR, T_FLOAT);
                } else {
                    comp_gen(comp, o - O_MUL + BC_MULI);
                    *type = ((t[0] >> 4) < T_U8 || (t[0] >> 4) > T_U32) || ((*type >> 4) < T_U8 || (*type >> 4) > T_U32) ?
                        T(T_SCALAR, T_U32) : T(T_SCALAR, T_I32);
                }
            break;
            case O_INC: case O_DEC:
                /* suffix increment / decrement */
                i = comp->code[--comp->nc];
                if(i < BC_LDB || i >= BC_LDF) { code_error(tok[s].pos, lang[ERR_BADLVAL]); return 0; }
                comp_push(comp, T(T_SCALAR, T_I32));
                comp_gen(comp, i);
                comp_gen(comp, (((*type & 15) > T_PTR ? 4 : 1) << 8) | (i - BC_LDB + (tok[s].id == O_INC ? BC_INCB : BC_DECB)));
                s++;
            break;
            case O_LAST:
                /* pointer disposition with array subscription */
                comp_push(comp, *type);
                m = s++; if(!(s = comp_expr(comp, s, e, type, O_LET))) return 0;
                if(s >= e || tok[s].type != HL_D || tok[s].id != ']') { code_error(tok[m].pos, lang[ERR_NOCLOSE]); return 0; }
                i = *type >> 4;
                if((*type & 15) != T_SCALAR || i < T_I8 || i > T_U32) { code_error(tok[m].pos, lang[ERR_BADARG]); return 0; } else
                if((t[0] & 15) < T_PTR) { code_error(tok[m].pos, lang[ERR_BADLVAL]); return 0; } else
                if((t[0] & 15) > T_PTR) {
                    comp_push(comp, t[0]);
                    comp->lc = comp->nc;
                    comp_gen(comp, BC_CI);
                    comp_gen(comp, 2);
                    comp_gen(comp, BC_SHL);
                }
                comp_gen(comp, BC_ADDI);
                comp_load(comp, t[0]);
                *type = t[0];
            break;
            default: code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0;
        }
    }
/*printf("expr ret s %d\n",s);*/
    return s;
}

/**
 * Generate a "load" instruction
 */
void comp_load(compiler_t *comp, int t)
{
    switch(t) {
        case T(T_SCALAR, T_FLOAT): comp_gen(comp, BC_LDF); break;
        case T(T_SCALAR, T_I8): case T(T_SCALAR, T_U8): comp_gen(comp, BC_LDB); break;
        case T(T_SCALAR, T_I16): case T(T_SCALAR, T_U16): comp_gen(comp, BC_LDW); break;
        default: comp_gen(comp, BC_LDI); break;
    }
}

/**
 * Generate a "store" instruction
 */
void comp_store(compiler_t *comp, int t)
{
    switch(t) {
        case T(T_SCALAR, T_FLOAT): comp_gen(comp, BC_STF); break;
        case T(T_SCALAR, T_I8): case T(T_SCALAR, T_U8): comp_gen(comp, BC_STB); break;
        case T(T_SCALAR, T_I16): case T(T_SCALAR, T_U16): comp_gen(comp, BC_STW); break;
        default: comp_gen(comp, BC_STI); break;
    }
}

/**
 * Generate a "push" instruction
 */
void comp_push(compiler_t *comp, int t)
{
    if(comp->nc - 2 == comp->lc && comp->code[comp->lc] == (t == T(T_SCALAR, T_FLOAT) ? BC_CF : BC_CI))
        comp->code[comp->lc] = t == T(T_SCALAR, T_FLOAT) ? BC_PSHCF : BC_PSHCI;
    else
        comp_gen(comp, t == T(T_SCALAR, T_FLOAT) ? BC_PUSHF : BC_PUSHI);
}

/**
 * Generate a comparator
 */
void comp_cmp(compiler_t *comp, int op, int t1, int t2)
{
    int t = BC_LTS;
    if(op == O_EQ || op == O_NE) {
        comp_gen(comp, op - O_EQ + BC_EQ);
    } else {
        if(t1 == T(T_SCALAR, T_FLOAT) || t2 == T(T_SCALAR, T_FLOAT)) t = BC_LTF; else
        if((t1 >= T(T_SCALAR, T_U8) && t1 <= T(T_SCALAR, T_U32)) ||
           (t2 >= T(T_SCALAR, T_U8) && t2 <= T(T_SCALAR, T_U32))) t = BC_LTU;
        comp_gen(comp, op - O_LT + t);
    }
}

/**
 * Add code debug records
 */
void comp_cdbg(compiler_t *comp, int s)
{
    if(comp->ncd + 2 >= comp->acd) {
        comp->acd += 512;
        comp->cd = (int*)realloc(comp->cd, comp->acd * 4);
        if(!comp->cd) { comp->ncd = comp->acd = 0; return; }
    }
    if(comp->ncd > 0 && comp->cd[comp->ncd - 2] == comp->nc) { comp->cd[comp->ncd - 1] = comp->tok[s].pos; return; }
    comp->cd[comp->ncd++] = comp->nc;
    comp->cd[comp->ncd++] = comp->tok[s].pos;
}

/**
 * Add data debug records
 */
void comp_ddbg(compiler_t *comp, int s, int id)
{
    if(comp->ndd + 4 >= comp->add) {
        comp->add += 1024;
        comp->dd = (int*)realloc(comp->dd, comp->add * 4);
        if(!comp->dd) { comp->ndd = comp->add = 0; return; }
    }
    if(comp->ndd > 0 && comp->dd[comp->ndd - 4] == comp->id[id].t &&
        comp->dd[comp->ndd - 3] == ((comp->tok[s].pos & 0xffffff) | (comp->tok[s].len << 24)) &&
        comp->dd[comp->ndd - 2] == comp->id[id].o && comp->dd[comp->ndd - 1] == comp->id[id].l) return;
    comp->dd[comp->ndd++] = comp->id[id].t;
    comp->dd[comp->ndd++] = (comp->tok[s].pos & 0xffffff) | (comp->tok[s].len << 24);
    comp->dd[comp->ndd++] = comp->id[id].o;
    comp->dd[comp->ndd++] = comp->id[id].l;
}

/**
 * Add opcodes to code
 */
void comp_gen(compiler_t *comp, int inst)
{
    if(comp->nc + 1 >= comp->ac) {
        comp->ac += 256;
        comp->code = (uint32_t*)realloc(comp->code, comp->ac * 4);
        if(!comp->code) { comp->nc = comp->ac = 0; return; }
    }
    comp->code[comp->nc++] = inst;
}

/**
 * Compile the source
 */
int cpu_compile(void)
{
    compiler_t comp;
    int ret = 0, i, j, k;
    int types[] = { T(T_FUNC,T_VOID), T(T_FUNC,T_I32), T(T_FUNCP,T_U8), T(T_FUNCP,T_I8), T(T_FUNC,T_FLOAT) };

    dsp_reset();
    cpu_init();
    cpu_getlang();
#if LUA
    if(meg4.code_type == 0x10) return comp_lua(meg4.src + 6, meg4.src_len - 7); else
#endif
    if(meg4.code_type > 1) { code_error(2, lang[ERR_UNKLNG]); return 0; }

    /* let's have the syntax highlighter do the heavy lifting... */
    memset(&comp, 0, sizeof(comp));
    comp.r = hl_find(meg4.src + 2);
    comp.tok = hl_tok(comp.r, meg4.src, &comp.ntok);
    if(!comp.tok) goto end;

    /* add system functions */
    comp.id = (idn_t*)malloc((MEG4_NUM_API + MEG4_NUM_BDEF) * sizeof(idn_t));
    if(!comp.id) goto end;
    memset(comp.id, 0, (MEG4_NUM_API + MEG4_NUM_BDEF) * sizeof(idn_t));
    for(i = 0; i < MEG4_NUM_API; i++) {
        strcpy(comp.id[i].name, meg4_api[i].name);
        comp.id[i].p = -1;
        comp.id[i].t = types[(int)meg4_api[i].ret];
    }
    /* add built-in defines */
    for(i = 0; i < MEG4_NUM_BDEF; i++) {
        strcpy(comp.id[MEG4_NUM_API + i].name, meg4_bdefs[i].name);
        comp.id[MEG4_NUM_API + i].p = -1;
        comp.id[MEG4_NUM_API + i].t = T(T_DEF, !i ? T_STR : T_I32);
    }
    comp.nid = MEG4_NUM_API + MEG4_NUM_BDEF;
    comp.cf = -1;

    /* compile the tokens */
    switch(meg4.code_type) {
        case 0: ret = comp_c(&comp); break;
        case 1: ret = comp_bas(&comp); break;
    }
    /* link program */
    if(ret) {
#if TOK_DBG
        comp_dumpfunc(&comp);
#endif
        /* resolve forward local function references */
        for(i = 0; i < comp.nf; i++)
            for(j = comp.id[comp.f[i].id].f[1], comp.id[comp.f[i].id].f[1] = 0; j;) {
                k = comp.code[j];
                comp.code[j] = comp.id[comp.f[i].id].o;
                j = k;
            }
        /* check if all forward labels has been declared and resolved, and there are no unused variables / functions */
        if(!comp_chkids(&comp, MEG4_NUM_API + MEG4_NUM_BDEF)) ret = 0;
    }
    if(ret) {
        /* failsafes */
        if(!comp.code) comp.nc = 0;
        if(!comp.cd) comp.ncd = 0;
        if(!comp.dd) comp.ndd = 0;
        if(!comp.nc) ret = 0; else {
            meg4.code_len = comp.nc + comp.ncd + comp.ndd;
            meg4.code = (uint32_t*)malloc(meg4.code_len * sizeof(uint32_t));
            if(!meg4.code) meg4.code_len = 0; else {
                /* copy text segment */
                memcpy(meg4.code, comp.code, comp.nc * sizeof(uint32_t));
                /* copy debug symbols */
                if(comp.cd && comp.ncd > 0) memcpy(&meg4.code[comp.nc], comp.cd, comp.ncd * sizeof(uint32_t));
                if(comp.dd && comp.ndd > 0) memcpy(&meg4.code[comp.nc + comp.ncd], comp.dd, comp.ndd * sizeof(uint32_t));
                /* header */
                meg4.code[0] = comp.nc;                                         /* size of text segment (code debug start) */
                meg4.code[1] = comp.nc + comp.ncd;                              /* data debug start */
                if(comp.id && comp.f) {
                    meg4.code[2] = comp.id[comp.f[0].id].o;                     /* address of setup() */
                    meg4.code[3] = comp.id[comp.f[1].id].o;                     /* address of loop() */
                }
            }
        }
        if(meg4.dp > 0) {
            /* copy initialized data segment */
            meg4_init_len = meg4.dp;
            meg4_init = (uint8_t*)realloc(meg4_init, meg4_init_len);
            if(!meg4_init) meg4_init_len = 0;
            else memcpy(meg4_init, meg4.data, meg4_init_len);
        }
    }

    /* free compiler resources */
end:if(comp.tok) free(comp.tok);
    if(comp.id) free(comp.id);
    if(comp.str) free(comp.str);
    if(comp.f) free(comp.f);
    if(comp.cd) free(comp.cd);
    if(comp.dd) free(comp.dd);
    if(comp.code) free(comp.code);
    if(ret) main_log(1, "compiled successfully, bytecode %u words (code %u, debug %u), data %u bytes",
        meg4.code_len, comp.nc > 4 ? comp.nc - 4 : 0, comp.ncd + comp.ndd, meg4.dp);
    return ret;
}
#endif /* NOEDITORS */
