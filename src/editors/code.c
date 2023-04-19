/*
 * meg4/editors/code.c
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
 * @brief Code editor
 *
 */

#include <stdio.h>
#include "editors.h"
#include "help.h"

typedef struct {
    uint32_t start, oldsize, newsize;
    char *oldbuf, *newbuf;
} hist_t;
hist_t *hist = NULL;

/* we should have used char pointers, but if we resize the underlying buffer, stupid gcc complains about "use after free".
 * gcc is wrong, but to silence the warning we use indeces and integer arithmetic instead of pointer arithmetic */
static uint32_t allocsize, numnl, cursor = 0, sels, sele, lastc;
static int *tok, alloctok, numtok, postok, notc, col, row = 0, hlp, lhlp, mx = 0, cx = 0, modal, modalclk, numhist = 0, curhist = -1;
static char ***rules, search[64], replace[64], func[64], line[6];
/* these aren't static only for one reason, so that tests/runner (which has no interface) can print them */
int errline = 0, errpos = 0;
char errmsg[256] = { 0 };

/**
 * Case insensitive compare
 */
int casecmp(char *s1, char *s2, int len)
{
    int i;
    char a, b;
    for(i = 0; i < len; i++) {
        a = s1[i] >= 'A' && s1[i] <= 'Z' ? s1[i] + 'a' - 'A' : s1[i];
        b = s2[i] >= 'A' && s2[i] <= 'Z' ? s2[i] + 'a' - 'A' : s2[i];
        if(a != b) return a - b;
    }
    return 0;
}

/**
 * Find byte array in buffer
 * Sadly mingw's libc lacks memmem()...
 */
void *code_memmem(const void *haystack, size_t hl, const void *needle, size_t nl)
{
    uint8_t *c = (uint8_t*)haystack;
    if(!haystack || !needle || !hl || !nl || nl > hl) return NULL;
    hl -= nl - 1;
    while(hl) {
        if(!memcmp(c, needle, nl)) return c;
        c++; hl--;
    }
    return NULL;
}

/**
 * Toggle bookmarks
 */
void code_tglbmark(int line)
{
    int i, n = (int)(sizeof(meg4.src_bm)/sizeof(meg4.src_bm[0]));
    for(i = 0; i < n && meg4.src_bm[i] && meg4.src_bm[i] < (uint32_t)line; i++);
    if(i < n && meg4.src_bm[i] == (uint32_t)line) {
        memcpy(&meg4.src_bm[i], &meg4.src_bm[i + 1], (n - i - 1) * sizeof(int));
        meg4.src_bm[n - 1] = 0;
    } else
    if(i < n) {
        memmove(&meg4.src_bm[i + 1], &meg4.src_bm[i], (n - i - 1) * sizeof(int));
        meg4.src_bm[i] = line;
    } else {
        memmove(&meg4.src_bm[1], &meg4.src_bm[0], (n - 1) * sizeof(int));
        meg4.src_bm[n - 1] = line;
    }
}

/**
 * Add to history
 */
void code_histadd(uint32_t start, uint32_t oldsize, uint32_t newsize, char *oldbuf, char *newbuf)
{
    if((!oldsize && !newsize) || (!oldbuf && !newbuf)) return;
    curhist = numhist++;
    hist = (hist_t*)realloc(hist, numhist * sizeof(hist_t));
    if(!hist) { numhist = 0; curhist = -1; return; }
    hist[curhist].start = start;
    hist[curhist].oldsize = oldsize;
    hist[curhist].newsize = newsize;
    hist[curhist].oldbuf = hist[curhist].newbuf = NULL;
    if(oldsize > 0 && oldbuf) {
        hist[curhist].oldbuf = (char *)malloc(oldsize);
        if(hist[curhist].oldbuf) memcpy(hist[curhist].oldbuf, oldbuf, oldsize);
    }
    if(newbuf) {
        hist[curhist].newbuf = (char *)malloc(newsize);
        if(hist[curhist].newbuf) memcpy(hist[curhist].newbuf, newbuf, newsize);
    }
}

/**
 * Undo
 */
void code_histundo(void)
{
    uint32_t i;

    if(!hist || !numhist || curhist < 0) return;
    if(!hist[curhist].oldbuf) hist[curhist].oldsize = 0;
    if(!hist[curhist].newbuf) hist[curhist].newsize = 0;
    if(meg4.src_len + hist[curhist].oldsize + 1 > allocsize) {
        allocsize += hist[curhist].oldsize + 1;
        meg4.src = (char*)realloc(meg4.src, allocsize);
        if(!meg4.src) { meg4.mmio.ptrspr = MEG4_PTR_ERR; meg4.src_len = 0; return; }
    }
    memmove(meg4.src + hist[curhist].start + hist[curhist].oldsize, meg4.src + hist[curhist].start + hist[curhist].newsize,
        meg4.src_len - hist[curhist].newsize - hist[curhist].start + 1);
    if(hist[curhist].oldbuf) memcpy(meg4.src + hist[curhist].start, hist[curhist].oldbuf, hist[curhist].oldsize);
    meg4.src_len -= hist[curhist].newsize;
    meg4.src_len += hist[curhist].oldsize;
    cursor = hist[curhist].start + hist[curhist].oldsize;
    curhist--;
    /* update the internal editor state */
    for(numnl = 1, i = 0; i < meg4.src_len && meg4.src[i]; i++)
        if(meg4.src[i] == '\n') numnl++;
    postok = cursor; lastc = cursor - 1;
    tok = hl_tokenize(rules, meg4.src, meg4.src + meg4.src_len, tok, &numtok, &alloctok, &postok);
    sels = sele = -1U;
}

/**
 * Redo
 */
void code_histredo(void)
{
    uint32_t i;

    if(!hist || !numhist || curhist + 1 >= numhist) return;
    curhist++;
    if(!hist[curhist].oldbuf) hist[curhist].oldsize = 0;
    if(!hist[curhist].newbuf) hist[curhist].newsize = 0;
    if(meg4.src_len + hist[curhist].newsize + 1 > allocsize) {
        allocsize += hist[curhist].newsize + 1;
        meg4.src = (char*)realloc(meg4.src, allocsize);
        if(!meg4.src) { meg4.mmio.ptrspr = MEG4_PTR_ERR; meg4.src_len = 0; return; }
    }
    memmove(meg4.src + hist[curhist].start + hist[curhist].newsize, meg4.src + hist[curhist].start + hist[curhist].oldsize,
        meg4.src_len - hist[curhist].oldsize - hist[curhist].start + 1);
    if(hist[curhist].newbuf) memcpy(meg4.src + hist[curhist].start, hist[curhist].newbuf, hist[curhist].newsize);
    meg4.src_len -= hist[curhist].oldsize;
    meg4.src_len += hist[curhist].newsize;
    cursor = hist[curhist].start + hist[curhist].newsize;
    /* update the internal editor state */
    for(numnl = 1, i = 0; i < meg4.src_len && meg4.src[i]; i++)
        if(meg4.src[i] == '\n') numnl++;
    postok = cursor; lastc = cursor - 1;
    tok = hl_tokenize(rules, meg4.src, meg4.src + meg4.src_len, tok, &numtok, &alloctok, &postok);
    sels = sele = -1U;
}

/**
 * Delete portion of source
 */
void code_delete(uint32_t start, uint32_t end)
{
    uint32_t i;

    if(!meg4.src || meg4.src_len < 1 || start > meg4.src_len || end > meg4.src_len || end <= start) return;
    code_histadd(start, end - start, 0, meg4.src + start, NULL);
    for(i = start; i < end; i++) if(meg4.src[i] == '\n') numnl--;
    memmove(meg4.src + start, meg4.src + end, meg4.src_len - end);
    meg4.src_len -= end - start;
    cursor = start;
    postok = cursor;
    tok = hl_tokenize(rules, meg4.src, meg4.src + meg4.src_len, tok, &numtok, &alloctok, &postok);
    sels = sele = -1U;
}

/**
 * Insert string into source
 */
void code_insert(char *str, uint32_t len)
{
    uint32_t i;

    if(!str || !*str || !meg4.src || meg4.src_len < 1) return;
    if(len < 1) len = strlen(str);
    for(i = 0; i < len; i++)
        if(str[i] == '\r') { memmove(str + i, str + i + 1, len - i); i--; len--; }
    if(sels != -1U && sele != -1U && sels != sele) {
        code_delete(sels < sele ? sels : sele, sels < sele ? sele : sels);
        if(hist) {
            hist[curhist].newsize = len;
            hist[curhist].newbuf = (char *)malloc(len);
            if(hist[curhist].newbuf) memcpy(hist[curhist].newbuf, str, len);
        }
    } else
        code_histadd(cursor, 0, len, NULL, str);
    if(meg4.src_len + len > allocsize) {
        allocsize += 65536;
        meg4.src = (char*)realloc(meg4.src, allocsize);
        if(!meg4.src) { meg4.mmio.ptrspr = MEG4_PTR_ERR; meg4.src_len = 0; return; }
    }
    memmove(meg4.src + cursor + len, meg4.src + cursor, meg4.src_len - cursor);
    memcpy(meg4.src + cursor, str, len);
    meg4.src_len += len;
    cursor += len;
    for(i = 0; i < len; i++) if(str[i] == '\n') numnl++;
    postok = cursor;
    /* if it was the first line that was edited, refresh rules too */
    if(row < 2) {
        if(tok) free(tok);
        rules = hl_find(meg4.src + 2); tok = NULL; alloctok = numtok = 0;
        notc = memcmp(meg4.src, "#!c", 3) || (meg4.src[3] != '\r' && meg4.src[3] != '\n');
    }
    tok = hl_tokenize(rules, meg4.src, meg4.src + meg4.src_len, tok, &numtok, &alloctok, &postok);
}

/**
 * Insert up to 4 spaces instead of a tab
 */
void code_instab(void)
{
    char tabs[] = "    ";
    uint32_t c = cursor, n = 0;

    if(meg4.src && meg4.src_len > 0) {
        while(c > 0 && meg4.src[c - 1] != '\n') {
            do { c--; } while(c > 0 && (meg4.src[c] & 0xC0) == 0x80);
            n++;
        }
        n &= 3;
        code_insert(tabs, 4 - n);
    }
}

/**
 * Move cursor up one line
 */
void code_up(void)
{
    int i;
    if(!meg4.src || meg4.src_len < 1) return;
    /* find beginning of line */
    if(cursor > 0) cursor--;
    while(cursor > 0 && meg4.src[cursor] != '\n') cursor--;
    /* find the beginning of the previous line */
    while(cursor > 1 && meg4.src[cursor - 1] != '\n') cursor--;
    if(cursor < 2) cursor = 0;
    /* go forward to find the Nth character */
    for(i = 0; i < col && cursor < meg4.src_len - 1 && meg4.src[cursor] != '\n'; i++, cursor++) {
        if((meg4.src[cursor] & 128) != 0) {
            if(!(meg4.src[cursor] & 32)) cursor += 1; else
            if(!(meg4.src[cursor] & 16)) cursor += 2; else
            if(!(meg4.src[cursor] & 8)) cursor += 3;
        }
    }
}

/**
 * Move cursor down one line
 */
void code_down(void)
{
    int i;
    if(!meg4.src || meg4.src_len < 1) return;
    /* find end of the line */
    while(cursor < meg4.src_len - 1 && meg4.src[cursor] != '\n') cursor++;
    if(cursor < meg4.src_len - 1 && meg4.src[cursor] == '\n') cursor++;
    /* go forward to find the Nth character */
    for(i = 0; i < col && cursor < meg4.src_len - 1 && meg4.src[cursor] != '\n'; i++, cursor++) {
        if((meg4.src[cursor] & 128) != 0) {
            if(!(meg4.src[cursor] & 32)) cursor += 1; else
            if(!(meg4.src[cursor] & 16)) cursor += 2; else
            if(!(meg4.src[cursor] & 8)) cursor += 3;
        }
    }
}

/**
 * Move cursor left
 */
void code_left(void)
{
    if(meg4.src && meg4.src_len > 0 && cursor > 0)
        do { cursor--; } while(cursor > 0 && (meg4.src[cursor] & 0xC0) == 0x80);
}

/**
 * Move cursor right
 */
void code_right(void)
{
    if(meg4.src && meg4.src_len > 0 && cursor < meg4.src_len - 1)
        do { cursor++; } while(cursor < meg4.src_len - 1 && (meg4.src[cursor] & 0xC0) == 0x80);
}

/**
 * Delete or Backspace
 */
void code_del(int bkspc)
{
    if(!meg4.src || meg4.src_len < 1) return;
    if(sels == -1U || sele == -1U) {
        if(bkspc) { sele = cursor; code_left(); sels = cursor; }
        else { sels = cursor; code_right(); sele = cursor; }
    }
    if(sels != sele)
        code_delete(sels < sele ? sels : sele, sels < sele ? sele : sels);
}

/**
 * Return position from coordinates
 */
uint32_t code_pos(int x, int y)
{
    uint32_t c;
    int dx, dy;
    char *str = meg4.src, *end = meg4.src + meg4.src_len;

    if(!meg4.src || meg4.src_len < 1) return -1U;
    x += mx; y += menu_scroll;
    for(dx = dy = 0; dy < y && *str && str < end; str++) {
        /* skip over lines above the clicked line */
        if(dy + 9 < y) { while(*str && str < end && *str != '\n') str++; }
        str = meg4_utf8(str, &c) - 1;
        /* find position in line, only UTF-8 sequence start positions counts */
        if(dx >= x) return (uint32_t)(str - meg4.src);
        /* interpret control codes */
        if(c == '\t') { dx = (dx + 16) & ~15; } else
        if(c == '\n') { dx = 0; dy += 9; } else
        /* special characters: keyboard, gamepad and mouse */
        if(c == 0x2328 || c == 0x1F3AE || c == 0x1F5B1 || (c >= EMOJI_FIRST && c < EMOJI_LAST)) { dx += 9; } else
        if(c >= 32 && c <= 0xffff) {
            if(!meg4_font[8 * 65536 + c] && meg4_isbyte(meg4_font + 8 * c, 0, 8)) c = 0;
            dx += (meg4_font[8 * 65536 + c] >> 4) - (meg4_font[8 * 65536 + c] & 0xf) + 2;
        }
    }
    return (uint32_t)(str - meg4.src);
}

/**
 * Returns true if token gives a function declaration
 */
static int code_isfuncdecl(int i)
{
    if(!meg4.src || meg4.src_len < 1 || i < 1 || i + 2 >= numtok) return 0;
    return
      (((notc && (tok[i] & 15) == HL_K && (!casecmp(meg4.src + (tok[i] >> 4), "fun", 3) || !casecmp(meg4.src + (tok[i] >> 4), "sub", 3))) ||
        (tok[i] & 15) == HL_T) &&
      (tok[i + 1] & 15) == HL_D && (meg4.src[(tok[i + 1] >> 4)] == ' ' || meg4.src[(tok[i + 1] >> 4)] == '\t') &&
      ((tok[i + 2] & 15) == HL_F || (notc && (tok[i + 2] & 15) == HL_V)));
}

/**
 * Get the function for the help status
 */
void code_getfunc(void)
{
    int i, j, p;

    hlp = lhlp = -1;
    if(!meg4.src || meg4.src_len < 1) return;
    for(i = 0; i + 1 < numtok && (uint32_t)(tok[i] >> 4) <= cursor; i++);
    /* with Assembly there's no conventional function call, instead name prefixed by an SCALL keyword */
    if(meg4.src_len > 5 && !memcmp(meg4.src, "#!asm", 5)) {
        if(i > 3 && (tok[i - 1] & 15) == HL_V && (tok[i - 2] & 15) == HL_D && (tok[i - 3] & 15) == HL_K &&
          !casecmp(meg4.src + (tok[i - 3] >> 4), "scall", 5))
            hlp = help_find(meg4.src + (tok[i - 1] >> 4), (tok[i] >> 4) - (tok[i - 1] >> 4), 1);
        return;
    }
    if(i > 0 && i < numtok) {
        for(p = 1; i > 0 && p > 0; i--) {
            if(meg4.src[tok[i] >> 4] == '(') p--; else
            if(meg4.src[tok[i] >> 4] == ')') p++;
        }
        if(i > 0 && (tok[i] & 15) == HL_D) i--;
        if(i > 0 && i < numtok && (tok[i] & 15) == HL_F) {
            /* look for system functions first */
            if(meg4.src_len > 5 && !notc) {
                /* The ECMA-55 BASIC spec requires renaming some of the system functions, but show help for those too */
                if((tok[i + 1] >> 4) == (tok[i] >> 4) + 3 && !memcmp(meg4.src + (tok[i] >> 4), "peek", 4)) hlp = help_find("inb", 3, 1); else
                if((tok[i + 1] >> 4) == (tok[i] >> 4) + 3 && !memcmp(meg4.src + (tok[i] >> 4), "sqr", 3)) hlp = help_find("sqrt", 4, 1); else
                if((tok[i + 1] >> 4) == (tok[i] >> 4) + 4 && !memcmp(meg4.src + (tok[i] >> 4), "atn%", 4)) hlp = help_find("atan", 4, 1); else
                /* otherwise just remove the type suffix before lookup */
                    hlp = help_find(meg4.src + (tok[i] >> 4), (tok[i + 1] >> 4) - (tok[i] >> 4) -
                        (meg4.src[(tok[i + 1] >> 4) - 1] == '!' || meg4.src[(tok[i + 1] >> 4) - 1] == '#' ||
                         meg4.src[(tok[i + 1] >> 4) - 1] == '%' || meg4.src[(tok[i + 1] >> 4) - 1] == '$' ? 1 : 0), 1);
            } else
                hlp = help_find(meg4.src + (tok[i] >> 4), (tok[i + 1] >> 4) - (tok[i] >> 4), 1);
            /* if not found, then look for local function declaration */
            if(hlp == -1) {
                for(j = 0, p = (tok[i + 1] >> 4) - (tok[i] >> 4); j + 3 < numtok && lhlp == -1; j++)
                    if(code_isfuncdecl(j) && (tok[j + 3] >> 4) - (tok[j + 2] >> 4) == p &&
                      !memcmp(meg4.src + (tok[j + 2] >> 4), meg4.src + (tok[i] >> 4), p)) {
                        /* do not report the declaration that we're currently editing */
                        for(lhlp = j; j < numtok && (tok[j] & 15) != HL_K && meg4.src[tok[j] >> 4] != ')'; j++);
                        if((uint32_t)(tok[lhlp] >> 4) <= cursor && (uint32_t)(tok[j] >> 4) >= cursor) lhlp = -1;
                        break;
                    }
            }
        }
    }
}

/**
 * Find string
 */
void code_find(uint32_t start)
{
    int l;
    char *buf;

    if(!meg4.src || meg4.src_len < 1 || !search[0]) return;
    l = strlen(search);
    if(start + l > meg4.src_len - 1) start = 0;
    buf = code_memmem(meg4.src + start, meg4.src_len - 1, search, l);
    if(!buf && start > 0) buf = code_memmem(meg4.src, meg4.src_len - 1, search, l);
    if(buf) { sels = buf - meg4.src; sele = cursor = sels + l; }
    else meg4.mmio.ptrspr = MEG4_PTR_ERR;
}

/**
 * Replace string
 */
void code_replace(void)
{
    uint32_t i, j, ls, lr;
    char *buf, *s, *d;

    if(!meg4.src || meg4.src_len < 1 || !search[0]) return;
    if(sels != -1U && sele != -1U && sels != sele) { i = sels < sele ? sels : sele; j = sels < sele ? sele : sels; }
    else { i = 0; j = meg4.src_len - 1; }
    ls = strlen(search); lr = strlen(replace);
    /* do a quick check, if the search pattern isn't in the selected area, do nothing */
    if(!code_memmem(meg4.src + i, j - i, search, ls)) { meg4.mmio.ptrspr = MEG4_PTR_ERR; return; }
    buf = d = (char*)malloc(meg4.src_len + 1 + (lr > ls ? (j - i + 1) * (lr - ls) : 0));
    if(buf) {
        s = meg4.src;
        if(i > 0) { memcpy(d, s, i); s += i; d += i; }
        while(s < meg4.src + j) {
            if(!memcmp(s, search, ls)) { memcpy(d, replace, lr); d += lr; s += ls; cursor = d - buf; }
            else *d++ = *s++;
        }
        code_histadd(i, j - i, d - buf - i, meg4.src + i, buf + i);
        if(j < meg4.src_len + 1) { memcpy(d, s, meg4.src_len - j + 1); d += meg4.src_len - j + 1; }
        free(meg4.src); meg4.src = buf; meg4.src_len = allocsize = d - buf;
        /* update the internal editor state */
        for(numnl = 1, i = 0; i < meg4.src_len && meg4.src[i]; i++)
            if(meg4.src[i] == '\n') numnl++;
        postok = cursor;
        tok = hl_tokenize(rules, meg4.src, meg4.src + meg4.src_len, tok, &numtok, &alloctok, &postok);
        sels = sele = -1U;
    }
}

/**
 * Go to line
 */
void code_goto(int line)
{
    char *str = meg4.src, *end = meg4.src + meg4.src_len;

    sels = sele = -1U;
    if(!meg4.src || meg4.src_len < 1 || line < 2) { row = 1; cursor = mx = menu_scroll = 0; return; }
    if(line > (int)numnl) line = numnl;
    for(row = 1; row <= (int)numnl + 1 && *str && str < end; row++) {
        if(row == line) { cursor = str - meg4.src; break; }
        while(*str && str < end && *str != '\n') str++;
        if(*str == '\n') str++;
    }
    if(row == line) { cursor = str - meg4.src; }
    mx = 0; lastc = cursor - 1;
}

/**
 * Set line and position
 */
void code_setpos(int line, uint32_t pos)
{
    code_goto(line);
    cursor = pos;
}

/**
 * Set error message to be displayed
 */
void code_seterr(uint32_t pos, const char *msg)
{
    char *l = meg4.src;
    uint32_t i;

    if(!meg4.src || meg4.src_len < 1) { cursor = 0; errline = 1; return; }
    if(pos > meg4.src_len - 1) pos = meg4.src_len - 1;
    for(errline = 1, i = 0; i < pos; i++) if(meg4.src[i] == '\n') { errline++; l = meg4.src + i + 1; }
    code_goto(errline); errpos = pos; errmsg[0] = 0;
    /* we must copy this, because Lua strings might go missing any time... */
    if(msg && *msg) {
        meg4.mmio.ptrspr = MEG4_PTR_ERR;
        strncpy(errmsg, msg, sizeof(errmsg) - 1);
        for(col = 1; l < meg4.src + pos; col++) l = meg4_utf8(l, &i);
        main_log(1, "error, line %u col %u msg '%s'", errline, col, errmsg);
    } else errline = 0;
    cursor = pos;
}

/**
 * Initialize code editor
 */
void code_init(void)
{
    uint32_t i;

    allocsize = meg4.src_len + 65536;
    meg4.src = (char*)realloc(meg4.src, allocsize);
    if(!meg4.src) { meg4.mmio.ptrspr = MEG4_PTR_ERR; meg4.src_len = 0; return; }
    if(meg4.src_len < 4) {
        strcpy(meg4.src, "#!c\n\nvoid setup()\n{\n  /* ");
        strcat(meg4.src, lang[CODE_SETUP]);
        strcat(meg4.src, " */\n}\n\nvoid loop()\n{\n  /* ");
        strcat(meg4.src, lang[CODE_LOOP]);
        strcat(meg4.src, " */\n}\n");
        meg4.src_len = strlen(meg4.src) + 1;
    }
    for(numnl = 1, i = 0; i < meg4.src_len && meg4.src[i]; i++)
        if(meg4.src[i] == '\n') numnl++;
    /* failsafe, some disturbance in the force... let's correct it */
    if(i != meg4.src_len - 1) meg4.src_len = i + 1;
    if(cursor >= meg4.src_len) cursor = meg4.src_len - 1;
    rules = hl_find(meg4.src + 2); tok = NULL; alloctok = numtok = 0;
    notc = memcmp(meg4.src, "#!c", 3) || (meg4.src[3] != '\r' && meg4.src[3] != '\n');
    postok = cursor;
    tok = hl_tokenize(rules, meg4.src, meg4.src + meg4.src_len, tok, &numtok, &alloctok, &postok);
    sels = sele = -1U;
    code_getfunc();
    modal = numhist = 0; curhist = -1;
    memset(search, 0, sizeof(search));
    memset(replace, 0, sizeof(replace));
    memset(func, 0, sizeof(func));
    lastc = 0;
    if(errline) meg4.mmio.ptrspr = MEG4_PTR_ERR;
}

/**
 * Free resources
 */
void code_free(void)
{
    int i;
    if(hist) {
        for(i = 0; i < numhist; i++) {
            if(hist[i].oldbuf) free(hist[i].oldbuf);
            if(hist[i].newbuf) free(hist[i].newbuf);
        }
        free(hist); hist = NULL;
        numhist = 0; curhist = -1;
    }
    if(tok) { free(tok); tok = NULL; alloctok = numtok = 0; }
    if(meg4.src_len < 1) { free(meg4.src); meg4.src = NULL; meg4.src_len = 0; }
    else {
        meg4.src = (char*)realloc(meg4.src, meg4.src_len);
        if(!meg4.src) meg4.src_len = 0;
    }
    errline = 0; memset(errmsg, 0, sizeof(errmsg));
    meg4.mmio.ptrspr = MEG4_PTR_NORM;
}

/**
 * Controller
 */
int code_ctrl(void)
{
    uint32_t key, j, k;
    char *clipboard;
    int i, n, clk = le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R), px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(last && !clk) {
        /* function definitions release */
        if(modal == 5) {
            if(px >= 504 && py >= 25 && py < 390) {
                modalclk = (py - 25) / 10;
            } else modal = 0;
        } else
        /* bookmarks release */
        if(modal == 6) {
            if(px >= 504 && py >= 12 && py < 390) {
                modalclk = (py - 12) / 10;
            } else modal = 0;
        } else
        /* menubar release */
        if(py < 12) {
            if(px >= 492 && px < 504) goto find; else
            if(px >= 504 && px < 516) goto repl; else
            if(px >= 516 && px < 528) goto line; else
            if(px >= 528 && px < 540) goto func; else
            if(px >= 540 && px < 552) goto book; else
            if(px >= 552 && px < 564) code_histundo(); else
            if(px >= 564 && px < 576) code_histredo(); else
            if(px >= 576 && px < 588) { memcpy(&key, "Cut", 4); goto copy; } else
            if(px >= 588 && px < 600) goto copy; else
            if(px >= 600 && px < 612) goto paste; else
            if(px >= 614 && px < 626) { meg4_switchmode(MEG4_MODE_VISUAL); last = 0; return 1; }
            if(px >= 626 && px < 638) { meg4_switchmode(MEG4_MODE_DEBUG); last = 0; return 1; }
        } else
        if(sele == -1U || sele == sels) sels = sele = -1U;
    } else
    if(!last && clk) {
        /* editor area click */
        if(px >= 18 && px < 632 && py >= 12 && py < 378) {
            cursor = lastc = sels = code_pos(px - 18 - MEG4_PTR_HOTSPOT_X, py - 12); sele = -1U;
            code_getfunc();
        }
    } else
    if(last && clk) {
        /* editor area pointer move with click */
        if(px >= 18 && px < 632 && py >= 12 && py < 378) {
            sele = code_pos(px - 18 - MEG4_PTR_HOTSPOT_X, py - 12);
        }
    } else
    if(!last && !clk) {
        if(modal) {
            /* modal input (most of it handled by textinp, we just check for last enter os esc key) */
            if(!memcmp(&textinp_key, "\n", 2)) {
                switch(modal) {
                    case 1: modal = 0; code_find(0); break;
                    case 2: modal = 3; textinp_init(382, 391, 248, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_STR, replace, sizeof(replace)); break;
                    case 3: modal = 0; code_replace(); break;
                    case 4: modal = 0; code_goto(atoi(line)); break;
                    case 5: modalclk = 0; break;
                }
            } else
            if(!memcmp(&textinp_key, "\x1b", 2)) { modal = 0; modalclk = -1; } else {
                key = meg4_api_popkey();
                if(!memcmp(&key, "\x1b", 2)) { modal = 0; modalclk = -1; }
            }
        } else {
            /* editor area keypress */
            key = meg4_api_popkey();
            if(key) {
                lastc = cursor; errline = 0; memset(errmsg, 0, sizeof(errmsg));
                meg4.mmio.ptrspr = MEG4_PTR_NORM;
                i = (sels == -1U && (meg4_api_getkey(MEG4_KEY_LSHIFT) || meg4_api_getkey(MEG4_KEY_LSHIFT)));
                /* check keys */
                if(!memcmp(&key, "\x1b", 2)) meg4_switchmode(MEG4_MODE_GAME); else
                if(!memcmp(&key, "\x8", 2)) code_del(1); else
                if(!memcmp(&key, "Del", 4)) code_del(0); else
                if(!memcmp(&key, "Undo", 4)) code_histundo(); else
                if(!memcmp(&key, "Redo", 4)) code_histredo(); else
                if(!memcmp(&key, "Next", 4)) { code_find(cursor + 1); lastc = cursor; } else
                if(!memcmp(&key, "F1", 3)) help_show(hlp); else
                if(!memcmp(&key, "Find", 4)) {
find:               modal = 1; textinp_init(130, 391, 500, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_STR, search, sizeof(search));
                } else
                if(!memcmp(&key, "Repl", 4)) {
repl:               modal = 2; textinp_init(130, 391, 248, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_STR, search, sizeof(search));
                } else
                if(!memcmp(&key, "Line", 4)) {
line:               modal = 4; sprintf(line, "%u", row); textinp_init(130, 391, 64, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_STR, line, sizeof(line));
                } else
                if(!memcmp(&key, "Func", 4)) {
func:               modal = 5; modalclk = -1; textinp_init(518, 15, 110, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_NAME, func, sizeof(func));
                } else
                if(!memcmp(&key, "BMrk", 4)) {
book:               if(meg4.src_bm[0]) { modal = 6; modalclk = -1; }
                } else
                if(!memcmp(&key, "TgBM", 4)) { code_tglbmark(row); } else
                if(!memcmp(&key, "Up", 3)) {
                    if(meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_LCTRL)) {
                        n = (int)(sizeof(meg4.src_bm)/sizeof(meg4.src_bm[0]));
                        for(i = 0; i + 1 < n && meg4.src_bm[i] && meg4.src_bm[i + 1] < (uint32_t)row; i++);
                        if(meg4.src_bm[i] && meg4.src_bm[i] < (uint32_t)row) code_goto(meg4.src_bm[i]); else {
                            for(i = n - 1; i > 0 && !meg4.src_bm[i]; i--);
                            if(meg4.src_bm[i]) code_goto(meg4.src_bm[i]);
                        }
                    } else { if(i) { sels = cursor; } code_up(); }
                } else
                if(!memcmp(&key, "Down", 4)) {
                    if(meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_LCTRL)) {
                        n = (int)(sizeof(meg4.src_bm)/sizeof(meg4.src_bm[0]));
                        for(i = 0; i < n && meg4.src_bm[i] && meg4.src_bm[i] <= (uint32_t)row; i++);
                        if(i < n && meg4.src_bm[i]) code_goto(meg4.src_bm[i]); else
                        if(meg4.src_bm[0]) code_goto(meg4.src_bm[0]);
                    } else { if(i) { sels = cursor; } code_down(); }
                } else
                if(!memcmp(&key, "Left", 4)) { if(i) { sels = cursor; } code_left(); } else
                if(!memcmp(&key, "Rght", 4)) { if(i) { sels = cursor; } code_right(); } else
                if(!memcmp(&key, "Home", 4)) { if(i) { sels = cursor; } while(cursor > 1 && meg4.src[cursor - 1] != '\n') cursor--; } else
                if(!memcmp(&key, "End", 4)) { if(i) { sels = cursor; } while(cursor < meg4.src_len - 1 && meg4.src[cursor] != '\n') cursor++; } else
                if(!memcmp(&key, "PgUp", 4)) { if(i) { sels = cursor; } for(j = 0; j < 40; j++) code_up(); } else
                if(!memcmp(&key, "PgDn", 4)) { if(i) { sels = cursor; } for(j = 0; j < 40; j++) code_down(); } else
                if(!memcmp(&key, "Sel", 4)) { sels = 0; sele = cursor = lastc = meg4.src_len; } else
                if(!memcmp(&key, "Cpy", 4) || !memcmp(&key, "Cut", 4)) {
copy:               if(sels != -1U && sele != -1U) {
                        j = sels < sele ? sels : sele; k = sels < sele ? sele : sels;
                        if((clipboard = (char*)malloc(k - j + 1))) {
                            memcpy(clipboard, meg4.src + j, k - j); clipboard[k - j] = 0;
                            main_setclipboard(clipboard);
                            free(clipboard);
                            if(!memcmp(&key, "Cut", 4)) code_delete(j, k);
                        }
                    }
                } else
                if(!memcmp(&key, "Pst", 4)) {
paste:              if((clipboard = main_getclipboard())) { code_insert(clipboard, 0); free(clipboard); }
                } else
                if(!memcmp(&key, "\t", 2)) code_instab(); else
                if(!memcmp(&key, "\n", 2) || !meg4_api_speckey(key)) { code_insert((char*)&key, meg4_api_lenkey(key)); }
                /* check for shift release to end selection */
                if((meg4_api_getkey(MEG4_KEY_LSHIFT) || meg4_api_getkey(MEG4_KEY_LSHIFT))) sele = cursor;
                else if(lastc != cursor) sels = sele = -1U;
                code_getfunc();
            }
        }
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void code_menu(uint32_t *dst, int dw, int dh, int dp)
{
    /* find */      menu_icon(dst, dw, dh, dp, 492, 120, 40, 1, MEG4_KEY_F, STAT_FIND);
    /* replace */   menu_icon(dst, dw, dh, dp, 504,   0, 56, 1, MEG4_KEY_H, STAT_REPLACE);
    /* goto */      menu_icon(dst, dw, dh, dp, 516,  24, 56, 1, MEG4_KEY_L, STAT_GOTO);
    /* functions */ menu_icon(dst, dw, dh, dp, 528,  32, 56, 1, MEG4_KEY_D, STAT_FUNC);
    /* bookmarks */ menu_icon(dst, dw, dh, dp, 540,  40, 56, 1, MEG4_KEY_N, STAT_BOOK);
    /* undo */      menu_icon(dst, dw, dh, dp, 552,   8, 56, 1, MEG4_KEY_Z, STAT_UNDO);
    /* redo */      menu_icon(dst, dw, dh, dp, 564,  16, 56, 1, MEG4_KEY_Y, STAT_REDO);
    /* cut */       menu_icon(dst, dw, dh, dp, 576, 136, 40, 1, MEG4_KEY_X, STAT_CUT);
    /* copy */      menu_icon(dst, dw, dh, dp, 588, 144, 40, 1, MEG4_KEY_C, STAT_COPY);
    /* paste */     menu_icon(dst, dw, dh, dp, 600, 152, 40, 1, MEG4_KEY_V, STAT_PASTE);
    /* visual */    menu_icon(dst, dw, dh, dp, 614, 112, 48, 1, 0, MENU_VISUAL);
    /* debugger */  menu_icon(dst, dw, dh, dp, 626, 104, 48, 1, 0, MENU_DEBUG);
}

/**
 * View
 */
void code_view(void)
{
    uint32_t c, *d, *dst, fg = theme[THEME_FG], bg, cr = theme[THEME_FG], e, f, n, cu = 0;
    uint8_t *fnt;
    int i, j, inv, sel, x0, x1, y0, y1, dx, dy, ex, x, y, l, r, m, cl, bl = 0;
    char *str, *end = meg4.src + meg4.src_len, *par = NULL, tmp[64];

    if(!meg4.src || meg4.src_len < 1 || !tok || numtok < 1) return;
    /* failsafes */
    if(cursor > meg4.src_len - 1) cursor = meg4.src_len - 1;
    /* find matching parenthesis, highlighted differently */
    if(sele == -1U) {
        tmp[0] = tmp[1] = 0; j = 0;
        switch(meg4.src[cursor]) {
            case '(': tmp[0] = '('; tmp[1] = ')'; j = 1; break; case ')': tmp[0] = ')'; tmp[1] = '('; j = -1; break;
            case '[': tmp[0] = '['; tmp[1] = ']'; j = 1; break; case ']': tmp[0] = ']'; tmp[1] = '['; j = -1; break;
            case '{': tmp[0] = '{'; tmp[1] = '}'; j = 1; break; case '}': tmp[0] = '}'; tmp[1] = '{'; j = -1; break;
        }
        if(j) {
            /* not using tokens here, because we have to match inside string literals too */
            for(str = meg4.src + cursor + j, i = 0; !par && str >= meg4.src && str < end; str += j)
                if(*str == tmp[0]) i++; else if(*str == tmp[1] && --i < 0) par = str;
        }
    }
    /* scrollbar stuff */
    if(lastc != cursor) {
        if(lastc < cursor) { n = lastc; e = cursor; } else { n = cursor; e = lastc; }
        for(; n < e && meg4.src[n] != '\n'; n++);
        if(n != e) {
            for(row = 1, str = meg4.src; *str && str < end && str < meg4.src + cursor; str++)
                if(*str == '\n') row++;
        }
        i = (row - 1) * 9;
        if(!menu_scroll) menu_scroll = i - 180;
        if(menu_scroll + 360 < i) menu_scroll = i - 360;
        if(menu_scroll > i) menu_scroll = i;
    }
    menu_scrhgt = 378; menu_scrmax = numnl * 9;
    if(menu_scrmax < menu_scrhgt) menu_scrmax = 0;
    if(menu_scroll + menu_scrhgt > menu_scrmax) menu_scroll = menu_scrmax - menu_scrhgt;
    if(menu_scroll < 0) menu_scroll = 0;
    /* display source code */
    y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = 0; y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(378);
    x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(16); x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(632);
    bg = htole32(0xFF000000) | ((theme[THEME_BG] + htole32(0x040404)) & htole32(0x7F7F7F));
    if(menu_scroll < 9) {
        if(!modal && row == 1) {
            if(errline == 1) { e = theme[THEME_ERR_FG]; f = theme[THEME_ERR_BG]; } else { e = theme[THEME_FG]; f = theme[THEME_BG]; }
        } else { e = theme[THEME_LN_BG]; f = theme[THEME_LN_FG]; }
        meg4_box(meg4.valt, 632, 378, 2560, 0, -menu_scroll, 16, 9, theme[THEME_LN_BG], e, theme[THEME_LN_BG], 0, 0, 0, 0, 0);
        if(meg4.src_bm[0] == 1) {meg4.mmio.cropx0 = 0; meg4_blit(meg4.valt, 0, -menu_scroll, 2560, 8, 8, meg4_edicons.buf, 40, 56, meg4_edicons.w * 4, 1); meg4.mmio.cropx0 = htole16(16); }
        meg4_number(meg4.valt, 632, 378, 2560, 0, 2-menu_scroll, f, 1);
        if(errline == 1)
            meg4_box(meg4.valt, 632, 378, 2560, 16, -menu_scroll, 632-16, 9, theme[THEME_ERR_LN], theme[THEME_ERR_LN], theme[THEME_ERR_LN], 0, 0, 0, 0, 0);
    }
    for(dx = i = cl = cx = j = 0, n = 1, dy = -menu_scroll, str = meg4.src; dy < le16toh(meg4.mmio.cropy1) && *str && str < end &&
      n < numnl + 1 && n < 10000; str++) {
        /* skip over lines above the screen */
        if(dy + 9 < le16toh(meg4.mmio.cropy0)) { while(*str && str < end && *str != '\n') str++; }
        while(i + 1 < numtok && (tok[i + 1] >> 4) <= (int)(str - meg4.src)) i++;
        if(str == meg4.src + cursor) {
            col = cl; cx = dx; j = 1;
            inv = (le32toh(meg4.mmio.tick) & 512) && !modal;
        } else
            inv = j = 0;
        if(sels != -1U && sele != -1U && sels != sele) {
            sel = ((uint32_t)(str - meg4.src) >= (sels < sele ? sels : sele) && (uint32_t)(str - meg4.src) < (sels < sele ? sele : sels));
        } else
            sel = 0;
        fg = theme[i < numtok ? (str == par ? THEME_SEL_FG : THEME_HL_C + (tok[i] & 0xf)) : THEME_FG];
        str = meg4_utf8(str, &c) - 1;
        ex = dx - mx + 18;
        cl++; if(j) cu = c;
        /* interpret control codes */
        if(c == '\t') {
            l = dx; dx = (dx + 16) & ~15; l = dx - l;
            if(sel) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, l, 9, theme[THEME_SEL_BG], theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            if(inv) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, l, 8, cr, cr, cr, 0, 0, 0, 0, 0);
            continue;
        } else
        if(c == '\n') {
            if(sel) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, 5, 9, theme[THEME_SEL_BG], theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            if(inv) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, 5, 8, cr, cr, cr, 0, 0, 0, 0, 0);
            dx = cl = 0; dy += 9; n++;
            if(n == (uint32_t)errline)
                meg4_box(meg4.valt, 632, 378, 2560, 16, dy, 632-16, 9, theme[THEME_ERR_LN], theme[THEME_ERR_LN], theme[THEME_ERR_LN], 0, 0, 0, 0, 0);
            else if(!(n & 1))
                meg4_box(meg4.valt, 632, 378, 2560, 16, dy, 632-16, 9, bg, bg, bg, 0, 0, 0, 0, 0);
            if(!modal && row == (int)n) {
                if(n == (uint32_t)errline) { e = theme[THEME_ERR_FG]; f = theme[THEME_ERR_BG]; } else { e = theme[THEME_FG]; f = theme[THEME_BG]; }
            } else { e = theme[THEME_LN_BG]; f = theme[THEME_LN_FG]; }
            meg4_box(meg4.valt, 632, 378, 2560, 0, dy, 16, 9, theme[THEME_LN_BG], e, theme[THEME_LN_BG], 0, 0, 0, 0, 0);
            while(bl + 1 < (int)(sizeof(meg4.src_bm)/sizeof(meg4.src_bm[0])) && meg4.src_bm[bl] && meg4.src_bm[bl] < n) bl++;
            if(meg4.src_bm[bl] == n) { meg4.mmio.cropx0 = 0; meg4_blit(meg4.valt, 0, dy, 2560, 8, 8, meg4_edicons.buf, 40, 56, meg4_edicons.w * 4, 1); meg4.mmio.cropx0 = htole16(16); }
            meg4_number(meg4.valt, 632, 378, 2560, 0, 2 + dy, f, n);
        } else
        /* special characters: keyboard, gamepad and mouse */
        if(c == 0x2328 || c == 0x1F3AE || c == 0x1F5B1) {
            if(sel) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, 8, 9, theme[THEME_SEL_BG], theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            if(inv) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, 8, 8, cr, cr, cr, 0, 0, 0, 0, 0);
            meg4_blit(meg4.valt, ex, dy, 2560, 8, 8, meg4_icons.buf, c == 0x2328 ? 32 : (c == 0x1F3AE ? 40 : 48), 64, meg4_icons.w * 4, 1);
            dx += 9;
            continue;
        } else
        /* emoji */
        if(c >= EMOJI_FIRST && c < EMOJI_LAST) {
            c -= EMOJI_FIRST; if(c >= (uint32_t)(sizeof(emoji)/sizeof(emoji[0])) || !emoji[c]) continue;
            if(sel) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, 8, 9, theme[THEME_SEL_BG], theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            if(inv) meg4_box(meg4.valt, 632, 378, 2560, ex, dy, 8, 8, cr, cr, cr, 0, 0, 0, 0, 0);
            meg4_blit(meg4.valt, ex, dy, 2560, 8, 8, meg4_icons.buf, ((int)emoji[c] % 13) * 8, 64 + ((int)emoji[c] / 13) * 8, meg4_icons.w * 4, 1);
            dx += 9;
            continue;
        }
        if(c < 32 || c > 0xffff || dy + 9 < le16toh(meg4.mmio.cropy0)) continue;
        if(!meg4_font[8 * 65536 + c] && meg4_isbyte(meg4_font + 8 * c, 0, 8)) c = 0;
        l = meg4_font[8 * 65536 + c] & 0xf; r = meg4_font[8 * 65536 + c] >> 4;
        if(sel) {
            meg4_box(meg4.valt, 632, 378, 2560, ex, dy, r - l + 2, 9, theme[THEME_SEL_BG], theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            fg = theme[THEME_SEL_FG];
        }
        if(ex + 9 >= le16toh(meg4.mmio.cropx0) && ex < le16toh(meg4.mmio.cropx1)) {
            fnt = meg4_font + 8 * c;
            for(dst = meg4.valt + dy * 640 + ex, y = 0; y < 8 && dy + y < le16toh(meg4.mmio.cropy1); y++, fnt++, dst += 640)
                if(dy + y >= le16toh(meg4.mmio.cropy0)) {
                    for(d = dst, x = l, m = (1 << l); x <= r && ex + x < le16toh(meg4.mmio.cropx1); x++, d++, m <<= 1)
                        if(ex + x >= le16toh(meg4.mmio.cropx0) && ((!inv && (*fnt & m)) || (inv && !(*fnt & m)))) *d = inv ? cr : fg;
                }
        }
        dx += r - l + 2;
    }
    /* in case the last line does not end in a newline */
    if((str == meg4.src + cursor) && !modal) {
        if((le16toh(meg4.mmio.tick) & 512)) meg4_box(meg4.valt, 632, 378, 2560, dx + 18, dy, 5, 8, cr, cr, cr, 0, 0, 0, 0, 0);
        col = cl; row = n; cx = dx;
    }
    if(lastc != cursor) {
        lastc = cursor;
        if(mx + 606 < cx) mx = cx - 606;
        if(mx > cx) mx = cx;
        if(mx < 0) mx = 0;
    }
    /* mouse cursor */
    if(meg4.mmio.ptrspr != MEG4_PTR_ERR && !modal && !textinp_buf)
        meg4.mmio.ptrspr = le16toh(meg4.mmio.ptrx) >= 16 && le16toh(meg4.mmio.ptrx) < 632 && le16toh(meg4.mmio.ptry) >= 12 &&
            le16toh(meg4.mmio.ptry) < (dy + 21 < 378 ? dy + 21 : 378) ? MEG4_PTR_TEXT : MEG4_PTR_NORM;
    /* status bar */
    meg4.mmio.cropx0 = 0; meg4.mmio.cropx1 = htole16(632); meg4.mmio.cropy1 = htole16(388);
    if(errmsg[0]) {
        meg4_box(meg4.valt, 632, 388, 2560, 0, 378, 632, 10, theme[THEME_ERR_BG], theme[THEME_ERR_BG], theme[THEME_ERR_BG], 0, 0, 0, 0, 0);
        meg4_text(meg4.valt, 4, 379, 2560, theme[THEME_ERR_FG], 0, 1, meg4_font, errmsg);
    } else {
        meg4_box(meg4.valt, 632, 388, 2560, 0, 378, 632, 10, theme[THEME_MENU_BG], theme[THEME_MENU_BG], theme[THEME_MENU_BG], 0, 0, 0, 0, 0);
        /* row and coloumn */
        sprintf(tmp, "%u /", row);
        meg4_text(meg4.valt, 40 - meg4_width(meg4_font, 1, tmp, NULL), 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "%u", col + 1);
        meg4_text(meg4.valt, 64 - meg4_width(meg4_font, 1, tmp, NULL), 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
        /* current character under the cursor */
        sprintf(tmp, "U+%05X", cu);
        meg4_text(meg4.valt, 72, 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
        switch(modal) {
            case 1:
                /* find */
                meg4_blit(meg4.valt, 118, 379, 2560, 8, 8, meg4_edicons.buf, 120, 40, meg4_edicons.w * 4, 1);
                meg4_box(meg4.valt, 632, 388, 2560, 128, 378, 502, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
            break;
            case 2:
                /* search and replace, step 1 */
                meg4_blit(meg4.valt, 118, 379, 2560, 8, 8, meg4_edicons.buf, 0, 56, meg4_edicons.w * 4, 1);
                meg4_box(meg4.valt, 632, 388, 2560, 128, 378, 250, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
                meg4_box(meg4.valt, 632, 388, 2560, 380, 378, 250, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
                meg4.mmio.cropx1 = htole16(630); meg4_text(meg4.valt, 382, 379, 2560, theme[THEME_FG], 0, 1, meg4_font, replace); meg4.mmio.cropx1 = htole16(632);
            break;
            case 3:
                /* search and replace, step 2 */
                meg4_blit(meg4.valt, 118, 379, 2560, 8, 8, meg4_edicons.buf, 0, 56, meg4_edicons.w * 4, 1);
                meg4_box(meg4.valt, 632, 388, 2560, 128, 378, 250, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
                meg4.mmio.cropx1 = htole16(506); meg4_text(meg4.valt, 130, 379, 2560, theme[THEME_FG], 0, 1, meg4_font, search); meg4.mmio.cropx1 = htole16(632);
                meg4_box(meg4.valt, 632, 388, 2560, 380, 378, 250, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
            break;
            case 4:
                /* go to line */
                meg4_blit(meg4.valt, 118, 379, 2560, 8, 8, meg4_edicons.buf,24, 56, meg4_edicons.w * 4, 1);
                meg4_box(meg4.valt, 632, 388, 2560, 128, 378, 66, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
            break;
            case 5:
                /* function list */
                meg4_box(meg4.valt, 632, 388, 2560, 504, 0, 128, 376, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], 0, 0, 0, 0, 0);
                meg4_blit(meg4.valt, 506, 3, 2560, 8, 8, meg4_edicons.buf, 120, 40, meg4_edicons.w * 4, 1);
                meg4_box(meg4.valt, 632, 388, 2560, 516, 2, 114, 10, theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
                meg4.mmio.ptrspr = MEG4_PTR_NORM;
                l = strlen(func); meg4.mmio.cropx1 = htole16(628); meg4.mmio.cropy1 = htole16(374);
                for(i = x = 0, y = 1; i < numtok - 4 && x < 40; i++) {
                    if(code_isfuncdecl(i)) {
                        r = (tok[i + 3] >> 4) - (tok[i + 2] >> 4);
                        if(!func[0] || code_memmem(meg4.src + (tok[i + 2] >> 4), r, func, l)) {
                            if(modalclk == x) { code_goto(y); break; }
                            if(le16toh(meg4.mmio.ptry) >= 25 + x * 10 && le16toh(meg4.mmio.ptry) < 35 + x * 10 &&
                              le16toh(meg4.mmio.ptrx) >= 504 && le16toh(meg4.mmio.ptrx) < 630) {
                                meg4.mmio.ptrspr = MEG4_PTR_HAND;
                                meg4_box(meg4.valt, 632, 388, 2560, 506, 13 + x * 10, 124, 10, 0, theme[THEME_SEL_BG], 0, 0, 0, 0, 0, 0);
                                fg = theme[THEME_SEL_FG];
                            } else fg = theme[THEME_MENU_FG];
                            meg4_number(meg4.valt, 632, 388, 2560, 504, 16 + x * 10, fg, y);
                            memcpy(tmp, meg4.src + (tok[i + 2] >> 4), r); tmp[r] = 0;
                            meg4.mmio.cropx1 = htole16(630);
                            meg4_text(meg4.valt, 522, 14 + x * 10, 2560, fg, 0, 1, meg4_font, tmp);
                            meg4.mmio.cropx1 = htole16(632);
                            x++;
                        }
                    }
                    for(j = (tok[i] >> 4); j < (tok[i + 1] >> 4); j++) { if(meg4.src[j] == '\n') y++; }
                }
                if(modalclk != -1) modal = 0;
            break;
            case 6:
                /* bookmarks list */
                meg4_box(meg4.valt, 632, 388, 2560, 504, 0, 128, 376, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], 0, 0, 0, 0, 0);
                meg4.mmio.ptrspr = MEG4_PTR_NORM; memset(tmp, 0, sizeof(tmp));
                for(i = 0, n = 1, str = meg4.src; i < 42 && meg4.src_bm[i] && *str && n < numnl + 1 && n < 9999; i++) {
                    y = meg4.src_bm[i];
                    if(modalclk == i) { code_goto(y); break; }
                    for(; n < (uint32_t)y && *str && n < numnl + 1 && n < 9999; str++) if(*str == '\n') n++;
                    while(*str && (*str == ' ' || *str == '\t')) str++;
                    for(tmp[0] = 0, x = 0; x + 1 < (int)sizeof(tmp) && str[x] && str[x] != '\n'; x++) tmp[x] = str[x];
                    tmp[x] = 0;
                    if(le16toh(meg4.mmio.ptry) >= 12 + i * 10 && le16toh(meg4.mmio.ptry) < 22 + i * 10 &&
                      le16toh(meg4.mmio.ptrx) >= 504 && le16toh(meg4.mmio.ptrx) < 630) {
                        meg4.mmio.ptrspr = MEG4_PTR_HAND;
                        meg4_box(meg4.valt, 632, 388, 2560, 506, i * 10, 124, 10, 0, theme[THEME_SEL_BG], 0, 0, 0, 0, 0, 0);
                        fg = theme[THEME_SEL_FG];
                    } else fg = theme[THEME_MENU_FG];
                    meg4_number(meg4.valt, 632, 388, 2560, 504, 3 + i * 10, fg, y);
                    meg4.mmio.cropx1 = htole16(630);
                    meg4_text(meg4.valt, 522, 1 + i * 10, 2560, fg, 0, 1, meg4_font, tmp);
                    meg4.mmio.cropx1 = htole16(632);
                }
                if(modalclk != -1) modal = 0;
            break;
            default:
                if(hlp != -1 && help_pages[hlp].code) {
                    /* quick system call help */
                    meg4_blit(meg4.valt, 118, 379, 2560, 8, 8, meg4_edicons.buf, 112, 40, meg4_edicons.w * 4, 1);
                    meg4_text(meg4.valt, 128, 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, help_pages[hlp].code);
                } else
                if(lhlp != -1) {
                    /* local function help */
                    str = meg4.src + (tok[lhlp] >> 4);
                    for(tmp[0] = 0, x = 0; x + 3 < (int)sizeof(tmp) && *str && *str != '(' && str[x] != '\n'; x++) tmp[x] = *str++;
                    tmp[x++] = *str++;
                    for(y = x; x + 2 < (int)sizeof(tmp) && *str && *str != ')'; str++) {
                        if(*str == ',') { tmp[x++] = ','; tmp[x++] = ' '; y = x; } else
                        if(*str == ' ') x = y; else
                        if(*str > ' ') tmp[x++] = *str;
                    }
                    tmp[x++] = ')'; tmp[x] = 0;
                    meg4_text(meg4.valt, 128, 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
                } else {
                    /* cursor position in bytes */
                    sprintf(tmp, "%u /", cursor + 1);
                    meg4_text(meg4.valt, 168 - meg4_width(meg4_font, 1, tmp, NULL), 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
                    sprintf(tmp, "%u", meg4.src_len);
                    meg4_text(meg4.valt, 208 - meg4_width(meg4_font, 1, tmp, NULL), 379, 2560, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
                }
            break;
        }
    }
    meg4.mmio.cropx0 = x0; meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
}
