/*
 * meg4/comp_asm.c
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
 * @brief Compiler specific functions for Assembly
 *
 */

#include "meg4.h"

#ifndef NOEDITORS
#include "editors/editors.h"    /* for the highlighter token defines */
#include "api.h"

/* must match the types in misc/hl.json */
enum { ASM_DATA, ASM_CODE, ASM_DB, ASM_DW, ASM_DI, ASM_DF };

/* BC_ defines in cpu.h must match keywords in misc/hl.json */

/**
 * Compile Assembly source
 */
int comp_asm(compiler_t *comp)
{
    tok_t *tok;
    int i, j, k, s, e, l, t;
    float f;

    if(!meg4.src || !comp || !comp->tok) return 0;
    tok = comp->tok;
    comp_addhdr(comp);

    for(s = 0; s < comp->ntok; ) {
        /* get line */
        while(s < comp->ntok && tok[s].type == HL_D && meg4.src[tok[s].pos] == '\n') s++;
        for(e = s; e < comp->ntok && !(tok[e].type == HL_D && meg4.src[tok[e].pos] == '\n'); e++);
        /* labels */
        if(tok[s].type == HL_V && s + 1 < e && tok[s + 1].type == HL_D && tok[s + 1].id == ':') {
            if(comp->cf == -1) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
            /* special labels that cannot be referenced, only allowed in the code section and has to be recorded in header */
            if( (tok[s].len == 5 && !memcmp(&meg4.src[tok[s].pos], "setup", 5)) ||
                (tok[s].len == 4 && !memcmp(&meg4.src[tok[s].pos], "loop", 4))) {
                if(comp->cf != 0) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                i = tok[s].len == 5 ? 2 : 3;
                if(comp->code[i] != 0) { code_error(tok[s].pos, lang[ERR_ALRDEF]); return 0; }
                comp->code[i] = comp->nc;
            } else
            if(comp->cf == -2) {
                /* data section labels */
                for(i = MEG4_NUM_API + MEG4_NUM_BDEF; i < comp->nid; i++)
                    if(!memcmp(comp->id[i].name, &meg4.src[tok[s].pos], tok[s].len) && !comp->id[i].name[tok[s].len]) {
                        if(comp->id[i].t != T(T_LABEL, T_I32) || comp->id[i].o != -1) { code_error(tok[s].pos, lang[ERR_ALRDEF]); return 0; }
                        /* resolve forward links */
                        for(k = meg4.dp, l = comp->id[i].f[1], comp->id[i].f[1] = 0; l;) {
                            memcpy(&j, meg4.data + l, 4);
                            memcpy(meg4.data + l, &k, 4);
                            l = j;
                        }
                        break;
                    }
                if(i >= comp->nid && (i = comp_addid(comp, &tok[s], T(T_LABEL, T_I32))) < 0) return 0;
                comp->id[i].o = meg4.dp;
            } else {
                /* code section labels */
                if(!comp_addlbl(comp, s)) return 0;
            }
            s += 2; if(s >= e) continue;
        }

        /********** data segment **********/
        if(tok[s].type == HL_T) {
            /* section specifiers */
            if(tok[s].id == ASM_DATA || tok[s].id == ASM_CODE) {
                if(e != s + 1) { code_error(tok[s].pos + 5, lang[ERR_SYNTAX]); return 0; }
                comp->cf = tok[s].id == ASM_DATA ? -2 : 0;
                continue;
            } else
            if(comp->cf == -2) {
                /* initialized data */
                t = tok[s++].id;
                while(s < e) {
                    if(meg4.dp >= meg4.sp) { code_error(tok[s].pos, lang[ERR_MEMORY]); return 0; }
                    switch(tok[s].type) {
                        case HL_V:
                            j = comp_findid(comp, &tok[s]);
                            if(t == ASM_DF || (j >= 0 && j <= MEG4_NUM_API)) { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                            /* built-in define */
                            if(j > MEG4_NUM_API && j < MEG4_NUM_API + MEG4_NUM_BDEF) goto doeval; else
                            /* existing data label */
                            if(j >= MEG4_NUM_API + MEG4_NUM_BDEF && comp->id[j].t == T(T_LABEL, T_I32)) {
                                if(comp->id[j].o == -1) {
                                    /* forward label */
                                    memcpy(meg4.data + meg4.dp, &comp->id[j].f[1], 4);
                                    comp->id[j].f[1] = meg4.dp;
                                } else {
                                    /* already defined */
                                    memcpy(meg4.data + meg4.dp, &comp->id[j].o, 4); meg4.dp += 4;
                                }
                            } else
                            if(j == -1) {
                                /* new forward reference */
                                if((j = comp_addid(comp, &comp->tok[s], T(T_LABEL, T_I32))) < 0) return 0;
                                comp->id[j].o = -1; comp->id[j].f[1] = meg4.dp;
                            } else { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                            meg4.dp += 4;
                        break;
                        case HL_N:
                            if(t == ASM_DF) {
                                if(!comp_const(comp, s, T(T_SCALAR, T_FLOAT), NULL, &f)) return 0;
                                memcpy(&i, &f, 4); i = htole32(i);
                                memcpy(meg4.data + meg4.dp, &i, 4);
                                meg4.dp += 4;
                            } else {
doeval:                         if(!comp_eval(comp, s, s + 1, &i)) return 0;
                                i = htole32(i);
                                switch(t) {
                                    case ASM_DB: memcpy(meg4.data + meg4.dp, &i, 1); meg4.dp++; break;
                                    case ASM_DW: memcpy(meg4.data + meg4.dp, &i, 2); meg4.dp += 2; break;
                                    case ASM_DI: memcpy(meg4.data + meg4.dp, &i, 4); meg4.dp += 4; break;
                                }
                            }
                        break;
                        case HL_S:
                            if((i = comp_addstr(comp, s)) < 0) return 0;
                            if(tok[s].type == HL_C) {
                                i = tok[s].id; j = meg4_api_lenkey((uint32_t)i);
                                memcpy(meg4.data + meg4.dp, &i, j);
                                if(t == ASM_DW) j = (j + 1) & ~1;
                                if(t == ASM_DI) j = (j + 3) & ~3;
                                meg4.dp += j;
                            } else {
                                memcpy(meg4.data + meg4.dp, comp->str[i].str, comp->str[i].l);
                                meg4.dp += comp->str[i].l + 1; comp->nstr = 0;
                            }
                        break;
                        default: code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0;
                    }
                    s++;
                    if(s < e && tok[s].type == HL_D && tok[s].id == ',') s++;
                }
            } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        } else

        /********** code segment **********/
        if(tok[s].type == HL_K && comp->cf >= 0) {
            /* check number of arguments */
            if((s + 1 != e && (tok[s].id < BC_SCALL || tok[s].id >= BC_PUSHI) && tok[s].id != BC_LDB && tok[s].id != BC_LDW) ||
               (tok[s].id == BC_SW && s + 5 >= e) ||
               (s + 2 != e && ((tok[s].id >= BC_SCALL && tok[s].id < BC_SW) || (tok[s].id >= BC_CI && tok[s].id < BC_PUSHI) ||
               (tok[s].id >= BC_INCB && tok[s].id <= BC_DECI))))
                { code_error(tok[s].pos, lang[ERR_NUMARG]); return 0; }
            /* with float argument */
            if(tok[s].id == BC_CF || tok[s].id == BC_PSHCF) {
                if(!comp_const(comp, ++s, T(T_SCALAR, T_FLOAT), NULL, &f)) return 0;
                memcpy(&i, &f, 4);
                comp_gen(comp, tok[s - 1].id);
                comp_gen(comp, htole32(i));
            } else
            /* with integer constant argument */
            if((tok[s].id >= BC_BND && tok[s].id <= BC_SP) || (tok[s].id >= BC_INCB && tok[s].id <= BC_DECI)) {
                if(!comp_const(comp, ++s, T(T_SCALAR, T_I32), &i, NULL)) return 0;
                if(i < 1 && (tok[s - 1].id == BC_BND || (tok[s - 1].id >= BC_INCB && tok[s - 1].id <= BC_DECI))) { code_error(tok[s].pos, lang[ERR_BADARG]); return 0; }
                comp_gen(comp, (htole32(i) << 8) | tok[s - 1].id);
            } else
            /* integer constant, built-in define or data label argument */
            if(tok[s].id == BC_CI || tok[s].id == BC_PSHCI) {
                s++; i = 0;
                switch(tok[s].type) {
                    case HL_V:
                        j = comp_findid(comp, &tok[s]);
                        if(j >= MEG4_NUM_API + MEG4_NUM_BDEF && comp->id[j].t == T(T_LABEL, T_I32) && comp->id[j].o >= 0) {
                            i = comp->id[j].o;
                        } else
                        if(j > MEG4_NUM_API && j < MEG4_NUM_API + MEG4_NUM_BDEF) {
                            if(!comp_eval(comp, s, s + 1, &i)) return 0;
                        } else { code_error(tok[s].pos, lang[ERR_UNDEF]); return 0; }
                    break;
                    case HL_N:
                        if(!comp_eval(comp, s, s + 1, &i)) return 0;
                    break;
                    case HL_S:
                        if((i = comp_addstr(comp, s)) < 0) return 0;
                        if(tok[s].type == HL_C) i = tok[s].id; else { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                    break;
                    default: code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0;
                }
                i = htole32(i);
                comp_gen(comp, tok[s - 1].id);
                comp_gen(comp, i);
            } else
            /* code label argument */
            if(tok[s].id >= BC_CALL && tok[s].id <= BC_JNZ) {
                comp_gen(comp, tok[s++].id);
                comp_addjmp(comp, s);
            } else
            /* system call, MEG-4 API argument */
            if(tok[s].id == BC_SCALL) {
                j = comp_findid(comp, &tok[++s]);
                if(j < 0 || j >= MEG4_NUM_API) { code_error(tok[s].pos, lang[ERR_BADSYS]); return 0; }
                comp_gen(comp, (htole32(j) << 8) | BC_SCALL);
            } else
            /* variable number of arguments */
            if(tok[s].id == BC_SW) {
                k = comp->nc; comp_gen(comp, 0); l = 0;
                /* first argument smallest value, can be an integer constant, built-in define or character */
                s++; i = 0;
                if(tok[s].type == HL_V || tok[s].type == HL_N) {
                    if(!comp_eval(comp, s, s + 1, &i)) return 0;
                } else
                if(tok[s].type == HL_S) {
                    if((i = comp_addstr(comp, s)) < 0) return 0;
                    if(tok[s].type == HL_C) i = tok[s].id; else { code_error(tok[s].pos, lang[ERR_NOTHERE]); return 0; }
                } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                comp_gen(comp, htole32(i));
                s++; if(tok[s].type != HL_D || tok[s].id != ',') { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                /* second parameter is the default case label */
                if(!comp_addjmp(comp, s)) return 0;
                s++; if(tok[s].type != HL_D || tok[s].id != ',') { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
                /* other arguments case labels */
                for(l = 0; l < 256 && s < e; s++, l++) {
                    if(!comp_addjmp(comp, s)) return 0;
                    if(tok[s + 1].type == HL_D && tok[s + 1].id == ',') s++;
                }
                /* patch the bytecode with opcode and number of cases */
                comp->code[k] = (htole32(l) << 8) | BC_SW;
            } else
            /* optional integer argument */
            if(tok[s].id == BC_LDB || tok[s].id == BC_LDW) {
                i = 0; if(s + 1 < e && !comp_eval(comp, s, s + 1, &i)) return 0;
                comp_gen(comp, ((!!i) << 8) | tok[s].id);
            } else
            /* mnemonics without any argument */
            if(tok[s].id < BC_SCALL || tok[s].id >= BC_PUSHI) comp_gen(comp, tok[s].id);
            else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        } else { code_error(tok[s].pos, lang[ERR_SYNTAX]); return 0; }
        s = e;
    }
    return 1;
}

#endif /* NOEDITORS */
