/*
 * meg4/editors/hl.c
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
 * @brief Syntax highlighter
 *
 */

#include "editors.h"

static char ***rules;
static int numrules;

/**
 * A very minimalistic, non-UTF-8 aware regexp matcher. Enough to match language keywords.
 * Returns how many bytes matched, 0 if pattern doesn't match, -1 if pattern is bad.
 */
static int match(char *regexp, char *str)
{
    unsigned char valid[256], *c=(unsigned char*)regexp, *s=(unsigned char *)str;
    int d, r, rmin, rmax, neg;
    if(!regexp || !regexp[0] || !str || !str[0]) return -1;
    while(*c) {
        if(*c == '(' || *c == ')') { c++; continue; }
        rmin = rmax = r = 1; neg = 0;
        memset(valid, 0, sizeof(valid));
        /* special case, non-greedy match */
        if(c[0] == '.' && c[1] == '*' && c[2] == '?') {
            c += 3; if(!*c) return -1;
            if(*c == '$') { c++; while(*s && *s != '\n') s++; }
            else { while(*s && !match((char*)c, (char*)s)) s++; }
        } else {
            /* get valid characters list */
            if(*c == '\\') { c++; valid[(unsigned int)*c] = 1; } else {
                if(*c == '[') {
                    c++; if(*c == '^') { c++; neg = 1; }
                    while(*c && *c != ']') {
                        if(*c == '\\') { c++; valid[(unsigned int)*c] = 1; }
                        if(c[1] == '-') { for(d = *c, c += 2; d <= *c; d++) valid[d] = 1; }
                        else valid[(unsigned int)(*c == '$' ? 10 : *c)] = 1;
                        c++;
                    }
                    if(!*c) return -1;
                    if(neg) { for(d = 0; d < 256; d++) valid[d] ^= 1; }
                }
                else if(*c == '.') { for(d = 0; d < 256; d++) valid[d] = 1; }
                else valid[(unsigned int)(*c == '$' ? 10 : *c)] = 1;
            }
            c++;
            /* make it case-insensitive */
            for(d = 0; d < 26; d++) {
                if(valid[d + 'a']) valid[d + 'A'] = 1; else
                if(valid[d + 'A']) valid[d + 'a'] = 1;
            }
            /* get repeat count */
            if(*c == '{') {
                c++; rmin = atoi((char*)c); rmax = 0; while(*c && *c != ',' && *c != '}') c++;
                if(*c == ',') { c++; if(*c != '}') { rmax = atoi((char*)c); while(*c && *c != '}') c++; } }
                if(*c != '}') return -1;
                c++;
            }
            else if(*c == '?') { c++; rmin = 0; rmax = 1; }
            else if(*c == '+') { c++; rmin = 1; rmax = 0; }
            else if(*c == '*') { c++; rmin = 0; rmax = 0; }
            /* do the match */
            for(r = 0; *s && valid[(unsigned int)*s] && (!rmax || r < rmax); s++, r++);
            /* allow exactly one + or - inside floating point numbers if they come right after the exponent marker */
            if(r && ((str[0] >= '0' && str[0] <= '9') || (str[0] == '-' && str[1] >= '0' && str[1] <= '9')) &&
                (*s == '+' || *s == '-') && (s[-1] == 'e' || s[-1] == 'E' || s[-1] == 'p' || s[-1] == 'P'))
                    for(s++; *s && valid[(unsigned int)*s] && (!rmax || r < rmax); s++, r++);
        }
        if((!*s && *c) || r < rmin) return 0;
    }
    return (int)((intptr_t)s - (intptr_t)str);
}

/**
 * Check if a string is listed in an array
 */
static int in_array(char *s, char **arr, int *idx)
{
    int i;

    if(idx) *idx = -1;
    if(!s || !*s || !arr) return 0;
    for(i = 0; arr[i]; i++)
        if(!strcmp(s, arr[i])) { if(idx) *idx = i; return 1; }
    return 0;
}

/**
 * Initialize syntax highlighter
 */
void hl_init(void)
{
    char *buf, *ps, *pe, *lng, *s, *e, *d;
    int i, j, k;

    numrules = 0; rules = NULL;
    buf = (char*)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)binary_hl_json, sizeof(binary_hl_json), 65536, &i, 1);
    if(!buf) return;
    for(ps = pe = buf + 1; *pe;) {
        for(lng = pe + 1, i = 0; *pe && !(!i && (*pe == ',' || *pe == '}')); pe++) { if(*pe == '[') { i++; } if(*pe == ']') { i--; } }
        *pe++ = 0;
        for(ps = lng; *ps != '\"'; ps++);
        *ps++ = 0;
        /* parse json into something that we can more easily handle */
        rules = (char***)realloc(rules, (numrules + 1) * 9 * sizeof(char**));
        if(!rules) { numrules = 0; return; }
        memset(&rules[numrules * 9], 0, 9 * sizeof(char**));
        rules[numrules * 9] = (char**)malloc(ps - lng + 1);
        if(!rules[numrules * 9]) return;
        memcpy((char*)rules[numrules * 9], lng, ps - lng + 1);
        for(s = ps + 1, i = k = 0, j = 1; *s && s < pe && j < 9; s++) {
            if(*s == '[') k++; else if(*s == ']') { k--; if(k == 1) { i = 0; j++; } } else
            if(*s == '\"') {
                s++; for(e = s; *e && *e != '\"'; e++) if(*e == '\\') e++;
                rules[numrules * 9 + j] = (char**)realloc(rules[numrules * 9 + j], (i + 2) * sizeof(char*));
                if(!rules[numrules * 9 + j]) return;
                rules[numrules * 9 + j][i] = d = (char*)malloc(e - s + 1);
                if(!rules[numrules * 9 + j][i]) return;
                rules[numrules * 9 + j][i + 1] = NULL;
                for(; s < e; s++)
                    if(s[0] == '\\' && s[1] == 'n') { *d++ = '\n'; s++; } else
                    if(s[0] != '\\' || s[1] != '\"') *d++ = *s;
                *d = 0; i++;
            }
        }
        numrules++;
    }
    free(buf);
}

/**
 * Free resources
 */
void hl_free(void)
{
    int i, j, k;

    if(rules) {
        for(i = 0; i < numrules; i++) {
            for(j = 0; j < 9; j++)
                if(rules[i * 9 + j]) {
                    if(j)
                        for(k = 0; rules[i * 9 + j][k]; k++)
                            if(rules[i * 9 + j][k]) free(rules[i * 9 + j][k]);
                    free(rules[i * 9 + j]);
                }
        }
        free(rules); rules = NULL;
    }
    numrules = 0;
}

/**
 * Find a language syntax ruleset
 */
char ***hl_find(char *lng)
{
    int i, l;

    if(lng && *lng && rules) {
        for(l = 0; lng[l] && lng[l] != ' ' && lng[l] != '\n'; l++);
        for(i = 0; i < numrules; i++)
            if(!memcmp((char*)rules[i * 9], lng, l) && !((char*)rules[i * 9])[l]) return &rules[i * 9 + 1];
    }
    return rules ? &rules[1] : NULL;
}

/**
 * For debugging, dump a ruleset
 * Note: quotes (") are deliberately unescaped.
 */
/*
void hl_dump(char ***rules)
{
    char *hl = "cpons.tkfv";
    int i, j;

    printf("language: \"%s\"\n", (char*)rules[-1]);
    for(j = 0; j < 8; j++) {
        printf("rules[%u] (hl_%c): ", j, hl[j]);
        if(rules[j])
            for(i = 0; rules[j][i]; i++)
                printf(" \"%s\"", rules[j][i]);
        printf("\n");
    }
    printf("\n\n");
}
*/

/**
 * Tokenize string into highlight blocks using a ruleset
 */
int *hl_tokenize(char ***r, char *str, char *end, int *tokens, int *len, int *mem, int *pos)
{
    char *s, *d, tmp[64];
    int i, j, k, l, m, nt = 0, at = 0, *t = NULL, size, p = (pos ? *pos : -1);

    if(r && str && *str) {
        if(!end) size = strlen(str); else size = end - str;
        if(tokens && mem) { t = tokens; at = *mem; }
        /* tokenize string */
        for(k = 0; k < size && str[k]; ) {
            if(nt + 2 >= at) {
                t = (int*)realloc(t, (at + 256) * sizeof(int));
                if(!t) return NULL;
                memset(t + at, 0, 256 * sizeof(int));
                at += 256;
            }
            if(!k && size > 3 && str[0] == '#' && str[1] == '!') {
                t[nt++] = 0; while(k < size && str[k] && str[k] != '\n') k++;
                continue;
            }
            if(str[k] == '(' || str[k] == ')') { t[nt++] = (k << 4) | 5; k++; continue; }
            j = 0; if(r[5]) for(i = 0; r[5][i]; i++) if(r[5][i][0] == str[k]) { j = 1; break; }
            if(str[k] == ' ' || str[k] == '\t' || str[k] == '\r' || str[k] == '\n' || j) {
                if(!nt || (t[nt - 1] & 0xf) != 5) t[nt++] = (k << 4) | 5;
                k++; continue;
            }
            for(m = 0; m < 4; m++)
                if(r[m] && !(m == 3 && nt && (t[nt - 1] & 0xf) == 9))
                    for(i = 0; r[m][i]; i++) {
                        l = match(r[m][i], str + k);
                        if(l > 0 && (m != 2 || !((str[k] >= 'a' && str[k] <= 'z') || (str[k] >= 'A' && str[k] <= 'Z')) || (
                          ((!k || str[k - 1] == ' ' || str[k - 1] == '\t' || str[k - 1] == '\r' || str[k - 1] == '\n' || str[k - 1] == ')' ||
                            str[k - 1] == ']' || (str[k - 1] >= '0' && str[k - 1] <= '9')) &&
                          (k + l >= size || !str[k + l] || str[k + l] == ' ' || str[k + l] == '\t' || str[k + l] == '\r' || str[k + l] == '\n' || str[k + l] == '(' ||
                            str[k + l] == '[' || (str[k + l] >= '0' && str[k + l] <= '9')))))) {
                            if(!nt || (t[nt - 1] & 0xf) != m) t[nt++] = (k << 4) | m;
                            k += l - 1; goto nextchar;
                        }
                    }
            if(r[4])
                for(i = 0; r[4][i]; i++) {
                    l = strlen(r[4][i]);
                    if(!memcmp(str + k, r[4][i], l)) {
                        if(!nt || (t[nt - 1] & 0xf) != 4) t[nt++] = (k << 4) | 4;
                        for(k += l; str[k]; k++) {
                            if(str[k] == '\\') k++; else
                            if(str[k] == r[4][i][l - 1]) { if(str[k + 1] != r[4][i][l - 1]) break; else k++; }
                        }
                        goto nextchar;
                    }
                }
            if(!nt || (t[nt - 1] & 0xf) != 9) t[nt++] = (k << 4) | 9;
nextchar:   if(str[k]) k++;
        }
        if(size > 0 && nt > 0 && t) {
            for(i = 0; i < nt; i++) {
                if(pos && p != -1 && (t[i] >> 4) >= p) { *pos = i; p = -1; }
                if((t[i] & 0xf) == 9) {
                    j = i + 1 < nt ? t[i + 1] >> 4 : k;
                    l = t[i] >> 4;
                    s = d = tmp;
                    if(s) {
                        for(; l < j; l++)
                            *d++ = str[l] >= 'A' && str[l] <= 'Z' ? str[l] + 'a' - 'A' : str[l];
                        *d = 0;
                        if(in_array(s, r[6], NULL)) t[i] = (t[i] & ~0xf) | 6; else
                        if(in_array(s, r[7], NULL)) t[i] = (t[i] & ~0xf) | 7; else
                        if(str[l] == '(' || (i + 2 < nt && (t[i + 2] & 0xf) == 5 && str[t[i + 2] >> 4] == '('))
                            t[i] = (t[i] & ~0xf) | 8;
                    } else l = j;
                }
                if(i && (t[i] & 0xf) == 3 && (t[i - 1] & 0xf) == 2 && (str[t[i - 1] >> 4] == '-' || str[t[i - 1] >> 4] == '.'))
                    t[i] -= 16;
            }
        }
    }
    if(len) *len = nt;
    if(mem) *mem = at;
    return t;
}

/**
 * Basicly the same as hl_tokenize, but omits comments, whitespaces and returns in a different format
 */
tok_t *hl_tok(char ***r, char *str, int *len)
{
    tok_t *t = NULL;
    char *s, *d, tmp[64];
    int i, j, k, l, m, n, p = -1, e = 0, nt = 0, at = 0, size, last = 0;

    if(r && str && *str) {
        size = strlen(str);
        /* tokenize string */
        for(k = 0; k < size && str[k]; ) {
            if(nt + 2 >= at) {
                t = (tok_t*)realloc(t, (at + 256) * sizeof(tok_t));
                if(!t) return NULL;
                memset(t + at, 0, 256 * sizeof(tok_t));
                at += 256;
            }
            if(!k && size > 3 && str[0] == '#' && str[1] == '!') {
                while(k < size && str[k] && str[k] != '\n') k++;
                while(str[k] == '\n') k++;
                continue;
            }
            if(p != -1 && k >= e) { t[p].id = nt - p - 1; p = -1; e = 0; }
            if(str[k] == '(' || str[k] == ')') { t[nt].type = last = 5; t[nt].id = str[k]; t[nt].len = 1; t[nt++].pos = k; k++; continue; }
            j = 0; if(r[5]) for(i = 0; r[5][i]; i++) if(r[5][i][0] == str[k]) { j = 1; break; }
            if(j) { if(!nt || str[t[nt - 1].pos] != '\n') { t[nt].type = last = 5; t[nt].id = str[k]; t[nt].len = 1; t[nt++].pos = k; } k++; continue; }
            if(str[k] == ' ' || str[k] == '\t' || str[k] == '\r' || str[k] == '\n') { last = 0; k++; continue; }
            for(m = 0; m < 4; m++)
                if(r[m] && !(m == 3 && nt && last == 9))
                    for(i = 0; r[m][i]; i++) {
                        l = match(r[m][i], str + k);
                        while(!m && str[k + l] == '\n') l++;
                        if(l > 0 && (m != 2 || !((str[k] >= 'a' && str[k] <= 'z') || (str[k] >= 'A' && str[k] <= 'Z')) || (
                          ((!k || str[k - 1] == ' ' || str[k - 1] == '\t' || str[k - 1] == '\r' || str[k - 1] == '\n' || str[k - 1] == ')' ||
                            str[k - 1] == ']' || (str[k - 1] >= '0' && str[k - 1] <= '9')) &&
                          (k + l >= size || !str[k + l] || str[k + l] == ' ' || str[k + l] == '\t' || str[k + l] == '\r' || str[k + l] == '\n' || str[k + l] == '(' ||
                            str[k + l] == '[' || (str[k + l] >= '0' && str[k + l] <= '9')))))) {
                            if(m == 1) { p = nt; e = k + l; t[nt].type = m; t[nt].len = l; t[nt++].pos = k; last = m; goto nextchar; }
                            if(m && (!nt || last != m)) {
                                if(nt && m == 3 && last == 2 && t[nt - 1].pos + t[nt - 1].len == k && str[t[nt - 1].pos] == '.') {
                                    t[nt - 1].type = m; t[nt - 1].len += l;
                                } else {
                                    t[nt].type = m; t[nt].id = i; t[nt].len = l; t[nt++].pos = k;
                                }
                            }
                            last = m;
                            k += l - 1; goto nextchar;
                        }
                    }
            if(r[4])
                for(i = 0; r[4][i]; i++) {
                    l = strlen(r[4][i]);
                    if(!memcmp(str + k, r[4][i], l)) {
                        if(!nt || last != 4) { t[nt].type = last = 4; t[nt].id = i; t[nt++].pos = k; }
                        for(k += l; str[k]; k++) {
                            if(str[k] == '\\') k++; else
                            if(str[k] == r[4][i][l - 1]) { if(str[k + 1] != r[4][i][l - 1]) break; else k++; }
                        }
                        t[nt - 1].len = k - t[nt - 1].pos + l;
                        goto nextchar;
                    }
                }
            if(!nt || last != 9) { t[nt].type = last = 9 ; t[nt++].pos = k; }
            if(nt) t[nt - 1].len++;
nextchar:   if(str[k]) k++;
        }
        if(size > 0 && nt > 0 && t) {
            for(i = 0; i < nt; i++) {
                if(t[i].type == 9) {
                    s = d = tmp;
                    for(l = t[i].pos, j = 0; j < t[i].len; j++, l++)
                        *d++ = str[l] >= 'A' && str[l] <= 'Z' ? str[l] + 'a' - 'A' : str[l];
                    *d = 0;
                    if(in_array(s, r[6], &n)) { t[i].type = 6; t[i].id = n; } else
                    if(in_array(s, r[7], &n)) { t[i].type = 7; t[i].id = n; } else
                    if(i + 1 < nt && str[t[i + 1].pos] == '(')
                        t[i].type = 8;
                }
            }
        }
    }
    if(len) *len = nt;
    return t;
}
