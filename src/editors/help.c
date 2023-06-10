/*
 * meg4/editors/help.c
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
 * @brief Built-in help system (see lang/(langcode).md files)
 *
 */

#include "editors.h"
#include "unicode.h"
#include "help.h"

#define LINEHEIGHT 10

/* help page redefined ANSI control codes */
enum { NUL, NORMAL, LINK, HEADING, EM, STRESS, KEY, SUP, SUB, TAB, NL, ROW, RIGHT, MONO, SRC,
    COMMENT, PSEUDO, OP, NUM, STR, SEP, TYPE, KEYWORD, FUNC, VAR };

/* the help pages themselves */
help_t *help_pages = NULL;
int help_numpages = 0, help_current = 0;

/* other variables */
static char search[64], lastsearch[64];
static int tocw, bckw, hist[128];

/**
 * Initialize help
 */
void help_init(void)
{
    link_t *link;
    int i, j, k, x, y, n, a, tr, td, *hl, nhl, siz, spcw = meg4_width(meg4_font, 1, " ", NULL);
    char *md = NULL, *buf, *ps, *pe, *l, *s, *e, *d, state = NORMAL, onlyspace, firstcode, *lng;

    tocw = meg4_width(meg4_font, 1, lang[HELP_TOC], NULL);
    bckw = meg4_width(meg4_font, 1, lang[HELP_BACK], NULL);
    for(i = 0; i < (int)(sizeof(hist)/sizeof(int)); i++) hist[i] = -1;
    for(i = 0; help_md[i * 2] && !md; i++)
        if(help_md[i * 2][0] == meg4.mmio.locale[0] && help_md[i * 2][1] == meg4.mmio.locale[1])
            md = (char*)help_md[i * 2 + 1];
    if(!md) md = (char*)help_md[1];
    buf = (char*)stbi_zlib_decode_malloc_guesssize_headerflag(md + 3, ((uint8_t)md[2]<<16)|((uint8_t)md[1]<<8)|(uint8_t)md[0],
        65536, &i, 1);
    if(!buf) return;
    /* This isn't really bullet-proof, so write the built-in help pages with sense. Very limited MarkDown support. */
    for(pe = ps = buf; *ps;) {
        /* chapter */
        if(!memcmp(pe, "# ", 2)) {
            for(s = pe + 2; *pe && *pe != '\n'; pe++);
            for(e = pe; *pe == '\n'; pe++);
            if(!*pe) break;
        } else s = e = NULL;
        /* find paragraph end */
        for(ps = pe; *pe && memcmp(pe, "\n## ", 4) && memcmp(pe, "\n# ", 3); pe++);
        if(*pe == '\n') pe++;
        /* add a new help page */
        if(*ps && pe > ps) {
            help_pages = (help_t*)realloc(help_pages, (help_numpages + 1) * sizeof(help_t));
            if(!help_pages) { help_numpages = 0; break; }
            memset(&help_pages[help_numpages], 0, sizeof(help_t));
            siz = pe - ps + 1;
            help_pages[help_numpages].content = d = (char*)malloc(siz);
            if(!help_pages[help_numpages].content) break;
            if(s) {
                help_pages[help_numpages].chapter = (char*)malloc(e - s + 1);
                if(help_pages[help_numpages].chapter) {
                    memcpy(help_pages[help_numpages].chapter, s, e - s);
                    help_pages[help_numpages].chapter[e - s] = 0;
                    help_pages[help_numpages].chapw = meg4_width(meg4_font, 1, s, e);
                }
            }
            for(e = ps + 3; e < pe && *e && *e != '\n'; e++) *d++ = *e;
            help_pages[help_numpages].titw = meg4_width(meg4_font, 1, ps + 3, e);
            *d++ = 0; while(e < pe && *e == '\n') e++;
            n = 2*LINEHEIGHT + 16; firstcode = 1; a = tr = td = 0;
            while(e < pe && *e) {
                s = e; y = n;
                while(e < pe && *e && *e != '\n') e++;
                for(; e < pe && *e == '\n'; e++, n += LINEHEIGHT);
                td = 0;
                /* first, parse whole line or multi-line tags */
                /* skip table header separator */
                if(s[0] == '|' && s[1] == '-') {
                    for(i = 0, s++; i < 31 && s < e && *s && *s != '\n'; s++) {
                        if(*s == '|') i++;
                        if(*s == ':') a |= (1 << i);
                    }
                    firstcode = 0; n -= LINEHEIGHT; continue;
                }
                /* skip horizontal ruler */
                if(!memcmp(s, "<hr>", 4)) continue;
                onlyspace = 1;
                /* heading */
                if(!memcmp(s, "### ", 4)) {
                    *d++ = HEADING; s += 4; n += LINEHEIGHT/2;
                    memcpy(d, s, e - s); d += e - s;
                    *d++ = NORMAL;
                } else
                /* source code */
                if(!memcmp(s, "```", 3)) {
                    s += 3; lng = s; while(s < e && *s && *s != '\n') { s++; } *s = 0;
                    for(s = e; e < pe && memcmp(e, "```", 3); e++)
                        if(*e == '\n') n += LINEHEIGHT;
                    if(!*lng) {
                        *d++ = MONO;
                        memcpy(d, s, e - s); d += e - s;
                    } else {
                        hl = hl_tokenize(hl_find(lng), s, e, NULL, &nhl, NULL, NULL);
                        if(hl) {
                            siz += nhl;
                            i = (uintptr_t)d - (uintptr_t)help_pages[help_numpages].content;
                            help_pages[help_numpages].content = (char*)realloc(help_pages[help_numpages].content, siz);
                            if(!help_pages[help_numpages].content) break;
                            d = help_pages[help_numpages].content + i;
                            *d++ = SRC;
                            for(i = k = 0; i < nhl; i++) {
                                j = i + 1 < nhl ? hl[i + 1] >> 4 : e - s;
                                if(j == k) continue;
                                *d++ = (hl[i] & 0xf) + COMMENT;
                                for(; k < j; k++) *d++ = s[k];
                            }
                            free(hl);
                        } else {
                            *d++ = MONO;
                            memcpy(d, s, e - s); d += e - s;
                        }
                        /* copy the very first code block simplified prototype, the code editor will display it */
                        if(firstcode) {
                            help_pages[help_numpages].code = (char*)malloc(e - s + 1);
                            if(help_pages[help_numpages].code) {
                                l = help_pages[help_numpages].code;
                                while(s < e && *s && *s != '(') { *l++ = *s++; } *l++ = *s++;
                                while(s < e && *s && *s != ')' && *s != '\n') {
                                    while(s < e && *s && *s != '.' && *s != ' ' && *s != ')') s++;
                                    while(s < e && (*s == ' ' || *s == '\n')) s++;
                                    while(s < e && *s && *s != ',' && *s != ')' && *s != '\n') { *l++ = *s++; }
                                    if(*s == ')') break;
                                    if(*s == ',') { *l++ = *s++; *l++ = ' '; }
                                    while(s < e && (*s == ' ' || *s == '\n')) s++;
                                }
                                *l++ = *s++;
                                *l = 0;
                            }
                        }
                    }
                    *d++ = state = NORMAL;
                    e += 4;
                    firstcode = 0;
                    n -= LINEHEIGHT;
                    continue;
                } else {
                    /* parse in-line tags. This is VERY simple, no nested tags allowed */
                    firstcode = 0;
                    for(x = 16; s < e; s++) {
                        if(*s == '\\' && state != MONO) { s++; *d++ = *s; x += meg4_width(meg4_font, 1, s, s + 1); } else
                        if(!memcmp(s, "<kbd>", 5)) { s += 4; *d++ = KEY; x += 3; } else
                        if(!memcmp(s, "</kbd>", 6)) { s += 5; *d++ = state = NORMAL; x += 2; } else
                        if(!memcmp(s, "<dt>", 4)) { s += 3; *d++ = HEADING;  n += LINEHEIGHT/2; } else
                        if(!memcmp(s, "</dt>", 5)) { s += 4; if(s[1] != '\n') { *d++ = '\n'; } *d++ = state = NORMAL; } else
                        if(!memcmp(s, "<dd>", 4)) { s += 3; if(s[1] == '\n') { s++; } } else
                        if(!memcmp(s, "</dd>", 5)) { s += 4; if(s[1] == '\n') { s++; n -= LINEHEIGHT; } } else
                        if(!memcmp(s, "**", 2)) { s++; state = (state == NORMAL ? STRESS : NORMAL); *d++ = state; } else
                        if(!memcmp(s, "^^", 2)) { s++; state = (state == NORMAL ? SUP : NORMAL); *d++ = state; } else
                        if(!memcmp(s, ",,", 2)) { s++; state = (state == NORMAL ? SUB : NORMAL); *d++ = state; } else
                        if(*s == '`') { state = (state == NORMAL ? MONO : NORMAL); *d++ = state; } else
                        if(*s == '*' && !onlyspace) { state = (state == NORMAL ? EM : NORMAL); *d++ = state; } else
                        if(*s == '|') {
                            if(!td) { *d++ = ROW; tr++; }
                            while(d > help_pages[help_numpages].content && x > 0 && d[-1] == ' ') { d--; x -= spcw; }
                            while(s + 1 < e && s[1] == ' ') s++;
                            x = x > 16 ? (x + 63) & ~63 : x;
                            if(td < 31 && a & (1 << td)) {
                                for(l = s + 1; l < e && *l && *l != '|' && *l != '\n'; l++);
                                *d++ = RIGHT; *d++ = (x > 16 ? 64 : 48) - meg4_width(meg4_font, 1, s + 1, l);
                                x += d[-1];
                            } else {
                                if(x > 0 && s[1] && s[1] != '\n') *d++ = '|';
                            }
                            td++;
                            if(s[1] == '\n' && s[2] == '\n') { a = 0; tr = 0; }
                        } else
                        if(*s == '[') {
                            s++; *d++ = LINK; i = x;
                            for(l = s; s < e && *s && *s != ']'; s++) {
                                if((*s & 0xc0) == 0xc0 || !(*s & 0x80)) x += meg4_width(meg4_font, 1, s, s + 1);
                                *d++ = *s;
                            }
                            *d++ = state = NORMAL;
                            if(s[1] == '(') { s += 2; if(*s == '#') { s++; } for(l = s; s < e && *s && *s != ')'; s++); }
                            help_pages[help_numpages].link = (link_t*)realloc(help_pages[help_numpages].link,
                                (help_pages[help_numpages].numlink + 1) * sizeof(link_t));
                            if(help_pages[help_numpages].link) {
                                link = &help_pages[help_numpages].link[help_pages[help_numpages].numlink++];
                                link->x = i; link->y = y + 12;
                                link->w = x - i; link->page = 0;
                                link->link = (char*)malloc(s - l + 1);
                                if(link->link) { memcpy(link->link, l, s - l); link->link[s - l] = 0; }
                                /* break into two if link exceeds the line */
                                if(link->x + link->w > 630) {
                                    i = link->w - (630 - link->x);
                                    link->w = 630 - link->x - 16;
                                    help_pages[help_numpages].link = (link_t*)realloc(help_pages[help_numpages].link,
                                        (help_pages[help_numpages].numlink + 1) * sizeof(link_t));
                                    if(help_pages[help_numpages].link) {
                                        link = &help_pages[help_numpages].link[help_pages[help_numpages].numlink++];
                                        link->x = 16; link->y = y + 12 + LINEHEIGHT;
                                        link->w = i; link->page = 0;
                                        link->link = (char*)malloc(s - l + 1);
                                        if(link->link) { memcpy(link->link, l, s - l); link->link[s - l] = 0; }
                                    } else help_pages[help_numpages].numlink = 0;
                                }
                            } else help_pages[help_numpages].numlink = 0;
                        } else {
                            if(onlyspace && *s != ' ') onlyspace = 0;
                            if((*s & 0xc0) == 0xc0 || !(*s & 0x80)) x += meg4_width(meg4_font, 1, s, s + 1);
                            *d++ = *s;
                        }
                        if(x >= 616) { x = 16; y += LINEHEIGHT; }
                    }
                }
            }
            if(!help_pages[help_numpages].content) break;
            while(d > help_pages[help_numpages].content && (d[-1] == ' ' || d[-1] == '\n')) d--;
            *d++ = 0;
            help_pages[help_numpages].content = (char*)realloc(help_pages[help_numpages].content, d - help_pages[help_numpages].content);
            help_numpages++;
        } else break;
    }
    free(buf);
    /* resolve links. This way we take care of forward references as well */
    if(help_pages) {
        for(i = 0; i < help_numpages; i++) {
            if(help_pages[i].link) {
                for(n = 0; n < help_pages[i].numlink; n++)
                    if(help_pages[i].link[n].link) {
                        help_pages[i].link[n].page = help_find(help_pages[i].link[n].link, 0, 0);
                        if(help_pages[i].link[n].page == -1)
                            main_log(1, "unresolved help page link '%s' on page '%s'", help_pages[i].link[n].link, help_pages[i].content);
                        free(help_pages[i].link[n].link);
                        help_pages[i].link[n].link = NULL;
                    }
            }
        }
    }
}

/**
 * Free resources
 */
void help_free(void)
{
    int i, j;
    if(help_pages) {
        for(i = 0; i < help_numpages; i++) {
            if(help_pages[i].content) free(help_pages[i].content);
            if(help_pages[i].chapter) free(help_pages[i].chapter);
            if(help_pages[i].code) free(help_pages[i].code);
            if(help_pages[i].link) {
                for(j = 0; j < help_pages[i].numlink; j++)
                    if(help_pages[i].link[j].link) free(help_pages[i].link[j].link);
                free(help_pages[i].link);
            }
        }
        free(help_pages); help_pages = NULL;
    }
    help_numpages = 0;
}

/**
 * Find a section by its name
 */
int help_find(char *section, int len, int func)
{
    char *s1, *s2, *end;
    int i, a, b, r;

    if(!section || !*section) return -1;
    if(len < 1) len = strlen(section);
    end = section + len;
    for(i = 0; i < help_numpages; i++) {
        if(func && !help_pages[i].code) continue;
        for(r = 1, s1 = section, s2 = help_pages[i].content; s1 < end && *s1 && *s2;) {
            /* do UNICODE compliant lowercase comparision */
            if((*s1 & 128) != 0) {
                if((*s1 & 32) == 0 ) { a=((s1[0] & 0x1F)<<6)+(s1[1] & 0x3F); s1++; } else
                if((*s1 & 16) == 0 ) { a=((((s1[0] & 0xF)<<6)+(s1[1] & 0x3F))<<6)+(s1[2] & 0x3F); s1+=2; }
                else a = 0;
            } else a = *s1;
            s1++;
            if((*s2 & 128) != 0) {
                if((*s2 & 32) == 0 ) { b=((s2[0] & 0x1F)<<6)+(s2[1] & 0x3F); s2++; } else
                if((*s2 & 16) == 0 ) { b=((((s2[0] & 0xF)<<6)+(s2[1] & 0x3F))<<6)+(s2[2] & 0x3F); s2+=2; }
                else b = 0;
            } else b = *s2;
            s2++;
            if(unicode_tolower(a) != unicode_tolower(b)) { r = 0; break; }
        }
        if(r && s1 == end && !*s2)
            return i;
    }
    return -1;
}

/**
 * Show a section
 */
void help_show(int page)
{
    if(meg4.mode != MEG4_MODE_HELP) meg4_switchmode(MEG4_MODE_HELP);
    help_current = page >= 0 && page < help_numpages ? page : -1;
    menu_scroll = menu_scrmax = last = 0;
    memset(lastsearch, 0, sizeof(lastsearch));
}

/**
 * Hide help
 */
void help_hide(void)
{
    help_current = -1;
    meg4_switchmode(-1);
}

/**
 * Controller
 */
int help_ctrl(void)
{
    uint32_t key;
    int i, y, clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(!textinp_buf) {
        /* clear the search field if user pressed Esc in the textinput window */
        if(textinp_key == htole32('\x1b')) {
            textinp_key = 0;
            memset(search, 0, sizeof(search));
            memset(lastsearch, 0, sizeof(lastsearch));
        } else {
            /* start search if user pressed any normal key */
            key = meg4_api_popkey();
            if(!meg4_api_speckey(key)) {
                memset(search, 0, sizeof(search));
                memcpy(search, &key, 4);
                goto dosearch;
            } else
            /* go back in history if Backspace pressed */
            if(key == htole32(8)) {
                i = hist[0]; memmove(&hist[0], &hist[1], sizeof(hist) - sizeof(int));
                hist[sizeof(hist)/sizeof(int) - 1] = -1;
                help_show(i);
            } else
            /* go back to the page where we came from if user pressed Esc */
            if(key == htole32('\x1b')) {
                meg4_switchmode(-1);
                return 1;
            }
        }
    }
    if(!last && clk) {
        /* search input clicked */
        if(py < 12 && px >= 512) {
dosearch:   help_current = -1; memset(lastsearch, 0, sizeof(lastsearch));
            textinp_init(514, 2, 124, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_STR, search, sizeof(search) - 1);
        }
        if(help_current < 0 || help_current >= help_numpages)
            if(search[0]) {
                /* search results */
                for(i = 0; i < help_numpages; i++) {
                    if(help_pages[i].y && py >= help_pages[i].y + 12 - menu_scroll &&
                      py < help_pages[i].y + 12 - menu_scroll + LINEHEIGHT &&
                      px >= 16 && px < 16 + help_pages[i].titw) {
                        memmove(&hist[1], &hist[0], sizeof(hist) - sizeof(int)); hist[0] = help_current;
                        help_show(i); break;
                    }
                }
            } else
                /* table of contents */
                for(y = LINEHEIGHT + 16 + 12 - menu_scroll, i = 0; i < help_numpages; i++, y += LINEHEIGHT) {
                    if(help_pages[i].chapter) y += LINEHEIGHT/2 + LINEHEIGHT;
                    if(py >= y && py < y + LINEHEIGHT &&
                      px >= 16 && px < 16 + help_pages[i].titw) {
                        memmove(&hist[1], &hist[0], sizeof(hist) - sizeof(int)); hist[0] = help_current;
                        help_show(i); break;
                    }
                }
        else {
            if(py >= 12 && py < 12 + LINEHEIGHT) {
                /* back link */
                if(px >= 2 && px < 8 + bckw) {
                    i = hist[0]; memmove(&hist[0], &hist[1], sizeof(hist) - sizeof(int));
                    hist[sizeof(hist)/sizeof(int) - 1] = -1;
                    help_show(i);
                } else
                /* table of contents link */
                if(px >= 24 + bckw && px < 24 + bckw + tocw) {
                    memmove(&hist[1], &hist[0], sizeof(hist) - sizeof(int)); hist[0] = help_current;
                    help_show(-1);
                }
            } else
            /* all the other links on the help page */
            if(help_pages[help_current].link)
                for(i = 0; i < help_pages[help_current].numlink; i++)
                    if(py >= help_pages[help_current].link[i].y - menu_scroll &&
                        py < help_pages[help_current].link[i].y - menu_scroll + LINEHEIGHT &&
                        px >= help_pages[help_current].link[i].x &&
                        px < help_pages[help_current].link[i].x + help_pages[help_current].link[i].w) {
                            memmove(&hist[1], &hist[0], sizeof(hist) - sizeof(int)); hist[0] = help_current;
                            help_show(help_pages[help_current].link[i].page); break;
                        }
        }
    }
    if(help_current >= 0 && textinp_buf)
        textinp_free();
    last = clk;
    return 1;
}

/**
 * Menu
 */
void help_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int x0, x1;
    /* search input */
    meg4_blit(dst, 504, 2, dp, 8, 8, meg4_edicons.buf, 120, 40, meg4_edicons.w * 4, 1);
    meg4_box(dst, dw, dh, dp, 512, 0, 128, 12,
        theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
    if(!textinp_buf && search[0]) {
        x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(514);
        x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(636);
        meg4_text(dst, 514, 2, dp, theme[THEME_FG], 0, 1, meg4_font, search);
        meg4.mmio.cropx0 = x0;
        meg4.mmio.cropx1 = x1;
    }
}

/**
 * View
 */
void help_view(void)
{
    uint8_t *fnt;
    uint32_t c, *d, *dst, *kbg, fg = theme[THEME_FG], bg = 0, sh = 0, bg2;
    char *str, state = NORMAL, shadow = 0;
    int i, x, y, y0, dx, dy = 0, sy = 0, l, r, m, p = 640, dp = 640 * 4, tr;
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    menu_scrhgt = 388;
    meg4.mmio.ptrspr = MEG4_PTR_NORM;
    if(help_current < 0 || help_current >= help_numpages) {
        if(search[0]) {
            /* display search results */
            meg4_text(meg4.valt, 2, LINEHEIGHT, 640 * 4, theme[THEME_HELP_TITLE], htole32(0x3f000000), 2, meg4_font, lang[HELP_RESULTS]);
            y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(LINEHEIGHT + 16);
            /* only do the actual search if the search string has changed */
            if(memcmp(search, lastsearch, sizeof(lastsearch))) {
                memcpy(lastsearch, search, sizeof(lastsearch));
                for(i = 0; i < help_numpages; i++)
                    help_pages[i].y = !strcmp(help_pages[i].content, search) ||
                        strstr(help_pages[i].content + strlen(help_pages[i].content) + 1, search) ? -1 : 0;
            }
            for(dy = le16toh(meg4.mmio.cropy0) - menu_scroll, i = 0; i < help_numpages; i++) {
                if(!help_pages[i].y) continue;
                if(help_pages[i].y == -1) help_pages[i].y = dy + menu_scroll;
                if(py >= dy + 12 && py < dy + 12 + LINEHEIGHT &&
                  px >= 16 && px < 16 + help_pages[i].titw)
                    meg4.mmio.ptrspr = MEG4_PTR_HAND;
                meg4_text(meg4.valt, 16, dy, 640 * 4, theme[THEME_HELP_LINK], 0, 1, meg4_font, help_pages[i].content);
                dy += LINEHEIGHT;
            }
        } else {
            /* display table of contents */
            meg4_text(meg4.valt, 2, LINEHEIGHT, 640 * 4, theme[THEME_HELP_TITLE], htole32(0x3f000000), 2, meg4_font, lang[HELP_TOC]);
            y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(LINEHEIGHT + 16);
            for(dy = le16toh(meg4.mmio.cropy0) - menu_scroll, i = 0; i < help_numpages; i++) {
                help_pages[i].y = -1;
                if(help_pages[i].chapter) {
                    dy += LINEHEIGHT/2;
                    meg4_text(meg4.valt, 8, dy, 640 * 4, theme[THEME_HELP_TITLE], htole32(0x3f000000), 1, meg4_font, help_pages[i].chapter);
                    dy += LINEHEIGHT;
                }
                if(py >= dy + 12 && py < dy + 12 + LINEHEIGHT &&
                  px >= 16 && px < 16 + help_pages[i].titw)
                    meg4.mmio.ptrspr = MEG4_PTR_HAND;
                meg4_text(meg4.valt, 16, dy, 640 * 4, theme[THEME_HELP_LINK], 0, 1, meg4_font, help_pages[i].content);
                dy += LINEHEIGHT;
            }
        }
        menu_scrmax = dy + menu_scroll;
        meg4.mmio.cropy0 = y0;
    } else {
        /* display one help page */
        meg4_text(meg4.valt, 2, 3, 640 * 4, theme[THEME_HELP_LINK], htole32(0x3f000000), 1, meg4_font, "\x11");
        meg4_text(meg4.valt, 10, 2, 640 * 4, theme[THEME_HELP_LINK], htole32(0x3f000000), 1, meg4_font, lang[HELP_BACK]);
        meg4_text(meg4.valt, 24 + bckw, 2, 640 * 4, theme[THEME_HELP_LINK], htole32(0x3f000000), 1, meg4_font, lang[HELP_TOC]);
        for(i = help_current; i > 0 && !help_pages[i].chapter; i--);
        meg4_text(meg4.valt, 630 - help_pages[i].chapw, 2, 640 * 4, theme[THEME_HELP_TITLE], 0, 1, meg4_font, help_pages[i].chapter);
        if(py >= 12 && py < 12 + LINEHEIGHT && (
          (px >= 2 && px < 8 + bckw) ||
          (px >= 24 + bckw && px < 24 + bckw + tocw)))
            meg4.mmio.ptrspr = MEG4_PTR_HAND;
        str = help_pages[help_current].content;
        meg4_text(meg4.valt, 2, LINEHEIGHT, 640 * 4, theme[THEME_HELP_TITLE], htole32(0x3f000000), 2, meg4_font, str);
        str += strlen(str) + 1;
        kbg = meg4_edicons.buf + 40 * meg4_edicons.w + 131;
        y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(LINEHEIGHT + 16);
        dy = le16toh(meg4.mmio.cropy0) - menu_scroll + LINEHEIGHT; dx = 16; tr = 0;
        if(help_pages[help_current].link)
            for(i = 0; i < help_pages[help_current].numlink; i++)
                if(py >= help_pages[help_current].link[i].y - menu_scroll &&
                    py < help_pages[help_current].link[i].y - menu_scroll + LINEHEIGHT &&
                    px >= help_pages[help_current].link[i].x &&
                    px < help_pages[help_current].link[i].x + help_pages[help_current].link[i].w) {
                        meg4.mmio.ptrspr = MEG4_PTR_HAND; break;
                    }
        fnt = (uint8_t*)&sh;
        fnt[0] = (0xc0 * (theme[THEME_BG] & 0xff)) >> 8;
        fnt[1] = (0xc0 * ((theme[THEME_BG] >> 8) & 0xff)) >> 8;
        fnt[2] = (0xc0 * ((theme[THEME_BG] >> 16) & 0xff)) >> 8;
        fnt[3] = 0xff;
        bg2 = htole32(0xFF000000) | ((theme[THEME_BG] + htole32(0x040404)) & htole32(0x7F7F7F));
        while(*str) {
            str = meg4_utf8(str, &c);
            /* interpret control codes */
            switch(c) {
                case NORMAL:
                    if(state == KEY) {
                        if(dy + sy > LINEHEIGHT + 16)
                            meg4_blit(meg4.valt, dx - 1, dy, dp, 3, 8, meg4_edicons.buf, 132, 40, meg4_edicons.w * 4, 1);
                        dx += 2;
                    }
                    state = NORMAL; sy = 0; bg = shadow = 0; fg = theme[THEME_FG];
                continue;
                case LINK: state = LINK; sy = 0; bg = 0; shadow = 1; fg = theme[THEME_HELP_LINK]; continue;
                case HEADING:
                    dy += LINEHEIGHT/2; dx = 8; state = HEADING; sy = 0; bg = 0; shadow = 1; fg = theme[THEME_HELP_HDR];
                continue;
                case EM: state = EM; sy = 0; bg = shadow = 0; fg = theme[THEME_HELP_EM]; continue;
                case STRESS: state = STRESS; sy = 0; bg = shadow = 0; fg = theme[THEME_HELP_STRESS]; continue;
                case KEY:
                    if(dy + sy > LINEHEIGHT + 16)
                        meg4_blit(meg4.valt, dx, dy, dp, 3, 8, meg4_edicons.buf, 128, 40, meg4_edicons.w * 4, 1);
                    dx += 3;
                    state = KEY; sy = 0; bg = shadow = 0; fg = theme[THEME_HELP_KEY];
                continue;
                case SUP: sy = -2; continue;
                case SUB: sy = 2; continue;
                case TAB: dx = (dx + 32) & ~31; continue;
                case ROW:
                    if(!(tr & 1)) meg4_box(meg4.valt, 630, 388, dp, 14, dy - 1, 610, LINEHEIGHT, bg2, bg2, bg2, 0, 0, 0, 0, 0);
                    if(!tr) { fg = theme[THEME_HELP_HDR]; shadow = 1; }
                    else { fg = theme[THEME_FG]; shadow = 0; }
                    tr++;
                continue;
                case '|':
                case RIGHT: if(state != MONO) { dx = (dx > 16 ? ((dx + 63) & ~63) : dx) + (c == RIGHT ? *str++ : 0); continue; } break;
                case MONO: state = MONO; sy = 0; bg = shadow = 0; fg = theme[THEME_HELP_MONO]; continue;
                case SRC:
                    sy = 0; bg = theme[THEME_HL_BG]; shadow = 0; fg = theme[THEME_FG];
                    meg4_box(meg4.valt, 630, 388, dp, 14, dy, 610, LINEHEIGHT, bg, bg, bg, 0, 0, 0, 0, 0);
                    continue;
                case COMMENT: case PSEUDO: case OP: case NUM: case STR: case SEP: case TYPE: case KEYWORD: case FUNC: case VAR:
                    fg = theme[c - COMMENT + THEME_HL_C];
                    continue;
            }
            if(dx >= 616 || c == '\n') {
                dx = 16; dy += LINEHEIGHT;
                if(bg == theme[THEME_HL_BG] && *str != NORMAL)
                    meg4_box(meg4.valt, 630, 388, dp, 14, dy, 610, LINEHEIGHT, bg, bg, bg, 0, 0, 0, 0, 0);
                if(*str == '\n') { tr = 0; }
                if(c == '\n') continue;
            }
            if(dy >= 388 && help_pages[help_current].h > 0) break;
            /* special characters: keyboard, gamepad and mouse */
            if(c == 0x2328 || c == 0x1F3AE || c == 0x1F5B1) {
                if(dy + sy > LINEHEIGHT + 16)
                    meg4_blit(meg4.valt, dx, dy + sy, dp, 8, 8, meg4_icons.buf, c == 0x2328 ? 32 : (c == 0x1F3AE ? 40 : 48), 64, meg4_icons.w * 4, 1);
                dx += 9;
                continue;
            }
            /* emoji */
            if(c >= EMOJI_FIRST && c < EMOJI_LAST) {
                c -= EMOJI_FIRST; if(c >= (uint32_t)(sizeof(emoji)/sizeof(emoji[0])) || !emoji[c]) continue;
                if(dy + sy > LINEHEIGHT + 16)
                    meg4_blit(meg4.valt, dx, dy + sy, dp, 8, 8, meg4_icons.buf, ((int)emoji[c] % 13) * 8, 64 + ((int)emoji[c] / 13) * 8, meg4_icons.w * 4, 1);
                dx += 9;
                continue;
            }
            if(c < 32 || c > 0xffff) continue;
            if(!meg4_font[8 * 65536 + c] && meg4_isbyte(meg4_font + 8 * c, 0, 8)) c = 0;
            fnt = meg4_font + 8 * c;
            if(state == MONO) { l = 0; r = 7; } else { l = meg4_font[8 * 65536 + c] & 0xf; r = meg4_font[8 * 65536 + c] >> 4; }
            for(dst = meg4.valt + (dy + sy) * p + dx, y = 0; y < 8; y++, fnt++, dst += p)
                if(((dy + sy + y) >= LINEHEIGHT + 16) && (dy + sy + y < 388)) {
                    for(d = dst, x = l, m = (1 << l); x <= r && dx + x < 630; x++, d++, m <<= 1) {
                        if(*fnt & m) { *d = fg; if(shadow) d[p + 1] = sh; } else
                        if(state == KEY) *d = kbg[y * meg4_edicons.w]; else
                        if(bg) *d = bg;
                    }
                    if(state == KEY) *d = kbg[y * meg4_edicons.w]; else
                    if(bg) *d = bg;
                }
            dx += (state == MONO ? 6 : r - l + 2);
        }
/*for(i = 0; i < help_pages[help_current].numlink; i++)meg4_box(meg4.valt, 630, 388, dp, help_pages[help_current].link[i].x, help_pages[help_current].link[i].y-12, help_pages[help_current].link[i].w, LINEHEIGHT, 0xffffffff, 0, 0xffffffff, 0, 0, 0, 0, 0);*/
        if(!help_pages[help_current].h) help_pages[help_current].h = dy + menu_scroll;
        menu_scrmax = help_pages[help_current].h;
        meg4.mmio.cropy0 = y0;
    }
}
