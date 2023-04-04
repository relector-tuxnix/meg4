/*
 * meg4/editors/font.c
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
 * @brief Font editor
 *
 */

#include <stdio.h>
#include "editors.h"

uint32_t gethex(uint8_t *ptr, int len);
static char search[5] = { 0 }, clipboard[8] = { 0 }, *hist = NULL;
static int idx = 0, numhist = 0, curhist = 0, idxhist = 0, modified = 0;

/**
 * Initialize font editor
 */
void font_init(void)
{
    last = 0;
    menu_scroll = (idx >> 4) * 17;
}

/**
 * Free resources
 */
void font_free(void)
{
    if(hist) { free(hist); hist = NULL; }
    numhist = curhist = idxhist = 0;
}

/**
 * Add to history
 */
void font_histadd(uint8_t *fnt)
{
    if(!fnt) return;
    if(idxhist != idx) { numhist = 0; idxhist = idx; }
    curhist = numhist++;
    hist = (char*)realloc(hist, 16 * numhist);
    if(!hist) { curhist = numhist = 0; return; }
    memcpy(hist + curhist * 16, fnt, 8);
    memset(hist + curhist * 16 + 8, 0, 8);
}

/**
 * Finish add to history
 */
void font_histfinish(uint8_t *fnt)
{
    if(!fnt || !hist || !numhist || idxhist != idx) return;
    memcpy(hist + (numhist - 1) * 16 + 8, fnt, 8);
}

/**
 * Undo change
 */
void font_histundo(void)
{
    if(!hist || !numhist || idxhist != idx) return;
    memcpy(meg4.font + 8 * idx, hist + curhist * 16, 8);
    if(curhist > 0) curhist--;
    meg4_recalcfont(idx, idx);
}

/**
 * Redo change
 */
void font_histredo(void)
{
    if(!hist || !numhist || curhist + 1 >= numhist || idxhist != idx) return;
    curhist++;
    memcpy(meg4.font + 8 * idx, hist + curhist * 16 + 8, 8);
    meg4_recalcfont(idx, idx);
}

/**
 * Check if cursor is on screen
 */
void font_chkscroll(void)
{
    int i;
    idx &= 0xffff;
    i = (idx >> 4) * 17;
    if(menu_scroll > i) menu_scroll = i;
    if(menu_scroll + 21 * 17 < i) menu_scroll = i - 21 * 17;
    if(menu_scroll > menu_scrmax - menu_scrhgt) menu_scroll = menu_scrmax - menu_scrhgt;
    if(menu_scroll < 0) menu_scroll = 0;
}

/**
 * Controller
 */
int font_ctrl(void)
{
    uint8_t tmp[8], *fnt = meg4.font + 8 * idx;
    int i, j, key;
    int clk = le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R), px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if((last || clk) && textinp_buf) { textinp_free(); last = clk; return 1; }
    if(last && !clk) {
        /* menubar click release */
        if(py < 12) {
            if(px >= 502 && px < 514) font_histundo(); else
            if(px >= 514 && px < 526) font_histredo(); else
            if(px >= 526 && px < 538) goto cut; else
            if(px >= 538 && px < 550) goto copy; else
            if(px >= 550 && px < 562) goto paste; else
            if(px >= 568 && px < 580) goto del; else
            if(px >= 610)
                textinp_init(612, 2, 24, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_HEX, search, sizeof(search));
        } else
        if(px >= 11 && px < 11+40*8 && py >= 23 && py < 23+40*8) {
            /* glyph editor area release */
            if(modified) font_histfinish(fnt);
            else if(numhist > 0) numhist--;
        } else
        /* toolbar */
        switch(toolbox_ctrl()) {
            /* move left */
            case 0:
                font_histadd(fnt);
                for(i = 0; i < 8; i++) {
                    tmp[0] = fnt[i]; fnt[i] >>= 1;
                    if(tmp[0] & 1) fnt[i] |= 0x80;
                }
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
            /* move up */
            case 1:
                font_histadd(fnt);
                tmp[0] = fnt[0];
                memmove(&fnt[0], &fnt[1], 7);
                fnt[7] = tmp[0];
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
            /* move down */
            case 2:
                font_histadd(fnt);
                tmp[0] = fnt[7];
                memmove(&fnt[1], &fnt[0], 7);
                fnt[0] = tmp[0];
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
            /* move right */
            case 3:
                font_histadd(fnt);
                for(i = 0; i < 8; i++) {
                    tmp[0] = fnt[i]; fnt[i] <<= 1;
                    if(tmp[0] & 0x80) fnt[i] |= 1;
                }
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
            /* rotate clockwise */
            case 4:
                font_histadd(fnt);
                memcpy(tmp, fnt, 8); memset(fnt, 0, 8);
                for(j = 0; j < 8; j++)
                    for(i = 0; i < 8; i++)
                        if(tmp[i] & (1 << j)) fnt[j] |= (1 << (7 - i));
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
            /* flip vertically */
            case 5:
                font_histadd(fnt);
                memcpy(tmp, fnt, 8);
                for(i = 0; i < 8; i++) fnt[i] = tmp[7 - i];
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
            /* flip horizontally */
            case 6:
                font_histadd(fnt);
                memcpy(tmp, fnt, 8); memset(fnt, 0, 8);
                for(j = 0; j < 8; j++)
                    for(i = 0; i < 8; i++)
                        if(tmp[j] & (1 << i)) fnt[j] |= (1 << (7 - i));
                font_histfinish(fnt);
                meg4_recalcfont(idx, idx);
            break;
        }
    } else
    if(last && clk) {
        if(px >= 11 && px < 11+40*8 && py >= 23 && py < 23+40*8) {
            /* glyph editor area move with click */
            i = 8 * idx + (py - 23) / 40; j = meg4.font[i];
            if(clk & MEG4_BTN_L)
                meg4.font[i] |= 1 << ((px - 11) / 40);
            else
                meg4.font[i] &= ~(1 << ((px - 11) / 40));
            if(meg4.font[i] != j) { modified = 1; meg4_recalcfont(idx, idx); }
        }
    }
    if(!last && clk) {
        if(px >= 11 && px < 11+40*8 && py >= 23 && py < 23+40*8) {
            /* glyph editor area clicked */
            font_histadd(fnt);
            modified = 0;
        } else
        if(px >= 356 && px < 356+272 && py >= 23 && py < 23+372) {
            /* glyph table clicked */
            idx = (((menu_scroll / 17) + ((py - 23 + (menu_scroll % 17)) / 17)) << 4) + ((px - 356) / 17);
        }
    } else {
        key = meg4_api_popkey();
        if(key) {
            if(meg4_api_speckey(key)) {
                if(!memcmp(&key, "Up", 3))   { idx -= 16; font_chkscroll(); } else
                if(!memcmp(&key, "Down", 4)) { idx += 16; font_chkscroll(); } else
                if(!memcmp(&key, "Left", 4)) { idx--; font_chkscroll(); } else
                if(!memcmp(&key, "Rght", 4)) { idx++; font_chkscroll(); } else
                if(!memcmp(&key, "Undo", 4)) { font_histundo(); } else
                if(!memcmp(&key, "Redo", 4)) { font_histredo(); } else
                if(!memcmp(&key, "Cut", 4)) {
cut:                memcpy(clipboard, fnt, 8);
                    font_histadd(fnt);
                    memset(fnt, 0, 8);
                    font_histfinish(fnt);
                    meg4_recalcfont(idx, idx);
                } else
                if(!memcmp(&key, "Cpy", 4)) {
copy:               memcpy(clipboard, fnt, 8);
                } else
                if(!memcmp(&key, "Pst", 4)) {
paste:              font_histadd(fnt);
                    memcpy(fnt, clipboard, 8);
                    font_histfinish(fnt);
                    meg4_recalcfont(idx, idx);
                } else
                if(!memcmp(&key, "Del", 4)) {
del:                font_histadd(fnt);
                    memset(fnt, 0, 8);
                    font_histfinish(fnt);
                    meg4_recalcfont(idx, idx);
                }
                idx &= 0xffff;
            } else {
                /* for any other non-special key, jump to the key's glyph */
                fnt = (uint8_t*)&key;
                if((*fnt & 128) != 0) {
                    if(!(*fnt & 32)) { idx = ((*fnt & 0x1F)<<6)|(*(fnt+1) & 0x3F); } else
                    if(!(*fnt & 16)) { idx = ((*fnt & 0xF)<<12)|((*(fnt+1) & 0x3F)<<6)|(*(fnt+2) & 0x3F); } else
                    if(!(*fnt & 8)) { idx = ((*fnt & 0x7)<<18)|((*(fnt+1) & 0x3F)<<12)|((*(fnt+2) & 0x3F)<<6)|(*(fnt+3) & 0x3F); }
                    else idx = 0;
                } else idx = *fnt;
            }
            font_chkscroll();
        }
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void font_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int x0, x1, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    char tmp[32];

    sprintf(tmp, "U+%04X", idx);
    meg4_text(dst, 460, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
    /* undo */      menu_icon(dst, dw, dh, dp, 502,   8, 56, 1, MEG4_KEY_Z, STAT_UNDO);
    /* redo */      menu_icon(dst, dw, dh, dp, 514,  16, 56, 1, MEG4_KEY_Y, STAT_REDO);
    /* cut */       menu_icon(dst, dw, dh, dp, 526, 136, 40, 1, MEG4_KEY_X, STAT_CUT);
    /* copy */      menu_icon(dst, dw, dh, dp, 538, 144, 40, 1, MEG4_KEY_C, STAT_COPY);
    /* paste */     menu_icon(dst, dw, dh, dp, 550, 152, 40, 1, MEG4_KEY_V, STAT_PASTE);
    /* delete */    menu_icon(dst, dw, dh, dp, 568, 136, 48, 0, MEG4_KEY_DEL, STAT_ERASE);
    /* search */
    meg4_blit(dst, 590, 2, dp, 8, 8, meg4_edicons.buf, 120, 40, meg4_edicons.w * 4, 1);
    meg4_text(dst, 600, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, "U+");
    meg4_box(dst, dw, dh, dp, 610, 0, 30, 12,
        theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);

    toolbox_stat();
    if(py >= 22 && py < 396 && px >= 355 && px < 629) menu_stat = STAT_GLPSEL;
    if(!textinp_buf && search[0]) {
        x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(612);
        x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(636);
        meg4_text(dst, 612, 2, dp, theme[THEME_FG], 0, 1, meg4_font, search);
        meg4.mmio.cropx0 = x0;
        meg4.mmio.cropx1 = x1;
    }
}

/**
 * View
 */
void font_view(void)
{
    uint8_t *fnt;
    uint32_t fg, *dst;
    int i, j, l, r, c, y0, y1;
    char tmp[32];

    /* glyph table */
    menu_scrhgt = 372; menu_scrmax = 69632;
    meg4_box(meg4.valt, 640, 388, 2560, 355, 10, 274, 374, theme[THEME_D], theme[THEME_BG], theme[THEME_L],
        0, 0, menu_scroll, 17, 17);
    for(i = 0; i < 16; i++) {
        sprintf(tmp, "x%X", i);
        meg4_text(meg4.valt, 363 + i * 17, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
    }
    y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(11); y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(384);
    for(i = 0, c = (menu_scroll / 17) << 4; i < 24; i++) {
        sprintf(tmp, "%03Xx", i + (menu_scroll / 17));
        meg4_text(meg4.valt, 354 - meg4_width(meg4_font, 1, tmp, NULL), 14 + i * 17 - (menu_scroll % 17), 2560, theme[THEME_D], theme[THEME_L],
            1, meg4_font, tmp);
        for(j = 0; j < 16; j++, c++) {
            if(c == idx) {
                meg4_box(meg4.valt, 640, 388, 2560, 356 + j * 17, 11 + i * 17 - (menu_scroll % 17), 17, 17, theme[THEME_SEL_BG],
                    theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
                fg = theme[THEME_SEL_FG];
            } else
                fg = theme[THEME_FG];
            meg4_char(meg4.valt, 2560, 356 + j * 17, 11 + i * 17 - (menu_scroll % 17), fg, 2, meg4.font, c);
        }
    }
    meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;

    /* editor box */
    meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 40*8+2, 40*8+2, theme[THEME_D], theme[THEME_BG], theme[THEME_L],
        0, 0, 0, 40, 40);
    l = meg4.font[8 * 65536 + idx] & 0xf; r = (meg4.font[8 * 65536 + idx] >> 4) + 1;
    dst = meg4.valt + 11 * 640 + 11;
    for(i = 0; i < 40 * 8; i++, dst += 640)
        dst[l * 40 - 1] = dst[r * 40] = theme[THEME_SEL_BG];
    fnt = meg4.font + 8 * idx;
    for(j = 0, r = 11; j < 8; j++, fnt++, r += 40) {
        for(i = 0, l = 1; i < 8; i++, l <<= 1)
            if(*fnt & l)
                meg4_box(meg4.valt, 640, 388, 2560, 11 + i * 40, r, 40, 40, theme[THEME_FG], theme[THEME_FG], theme[THEME_FG],
                    0, 0, 0, 0, 0);
    }
    for(i = 0; i < 8; i++) {
        sprintf(tmp, "%u", 1 << i);
        meg4_text(meg4.valt, 32 + i * 40 - meg4_width(meg4_font, 1, tmp, NULL)/2, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
        sprintf(tmp, "%u", i);
        meg4_text(meg4.valt, 9 - meg4_width(meg4_font, 1, tmp, NULL), 26 + i * 40, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
    }
    /* tool buttons */
    toolbox_view(10, 340, -1);
}
