/*
 * meg4/editors/overlay.c
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
 * @brief Memory overlays editor
 *
 */

#include <stdio.h>
#include "editors.h"

uint32_t gethex(uint8_t *ptr, int len);
int overlay_idx = 0;
static int loadw, savew, expw, loadx, savex, expx, lenx, addrx, unaw, addrw;
static char addr[8], len[8];

/**
 * Initialize overlay editor
 */
void overlay_init(void)
{
    int i, j;

    for(addrw = 0, i = OVL_PALETTE; i <= OVL_USER; i++) {
        j = meg4_width(meg4_font, 1, lang[i], NULL);
        if(j > addrw) addrw = j;
    }
    loadw = meg4_width(meg4_font, 1, lang[MENU_LOAD], NULL);
    savew = meg4_width(meg4_font, 1, lang[MENU_SAVE], NULL);
    expw = meg4_width(meg4_font, 1, lang[MENU_EXPORT], NULL);
    unaw = meg4_width(meg4_font, 1, lang[MENU_UNAVAIL], NULL);
    savex = 598 - expw - loadw - savew;
    loadx = 614 - expw - loadw;
    expx = 630 - expw;
    lenx = savex - 8 - 40;
    addrx = lenx - 8 - 32;
    strcpy(addr, "00000");
    strcpy(len, "0");
    last = 0;
}

/**
 * Free resources
 */
void overlay_free(void)
{
}

/**
 * Controller
 */
int overlay_ctrl(void)
{
#ifndef EMBED
    char tmp[32];
#endif
    int i, offs, size, key, clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    /* set the inputs to sane defaults */
    if(!textinp_buf) {
        if(!addr[0]) strcpy(addr, "00000");
        if(!len[0] || len[0] == '0') sprintf(len, "%u", atoi(len));
    }
    if((last || clk) && textinp_buf) { textinp_free(); last = clk; return 1; }
    if(last && !clk) {
        /* menubar click release */
        if(py < 12) {
            offs = (int)gethex((uint8_t*)addr, 5);
            size = atoi(len);
            if(offs + size >= MEG4_MEM_LIMIT) { size = MEG4_MEM_LIMIT - offs; sprintf(len, "%u", size); }
            /* delete */
            if(px >= addrx - 26 && px < addrx - 14) {
                if(meg4.ovls[overlay_idx].data) { free(meg4.ovls[overlay_idx].data); meg4.ovls[overlay_idx].data = NULL; }
                meg4.ovls[overlay_idx].size = 0;
            } else
            /* address */
            if(px >= addrx && px < addrx + 32) {
                textinp_init(addrx + 1, 1, 30, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_HEX, addr, 6);
            } else
            /* length */
            if(px >= lenx && px < lenx + 40) {
                textinp_init(lenx + 1, 1, 30, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_NUM, len, 7);
            } else
            /* save */
            if(px >= savex && px < savex + savew + 8) {
                if(size < 1) {
                    if(meg4.ovls[overlay_idx].data) { free(meg4.ovls[overlay_idx].data); meg4.ovls[overlay_idx].data = NULL; }
                    meg4.ovls[overlay_idx].size = 0;
                } else {
                    meg4.ovls[overlay_idx].data = (uint8_t*)realloc(meg4.ovls[overlay_idx].data, size);
                    if(meg4.ovls[overlay_idx].data) {
                        for(i = 0; i < size; i++)
                            meg4.ovls[overlay_idx].data[i] = meg4_api_inb(offs + i);
                        meg4.ovls[overlay_idx].size = size;
                    } else meg4.ovls[overlay_idx].size = 0;
                }
            } else
            /* load */
            if(px >= loadx && px < loadx + loadw + 8 && meg4.ovls[overlay_idx].data && meg4.ovls[overlay_idx].size > 0) {
                if((uint32_t)size > meg4.ovls[overlay_idx].size) size = meg4.ovls[overlay_idx].size;
                for(i = 0; i < size; i++)
                    meg4_api_outb(offs + i, meg4.ovls[overlay_idx].data[i]);
            } else
            /* export binary */
            if(px >= expx && px < expx + expw + 8 && meg4.ovls[overlay_idx].data && meg4.ovls[overlay_idx].size > 0) {
#ifndef EMBED
                sprintf(tmp, "mem%02X.bin", overlay_idx);
                main_savefile(tmp, meg4.ovls[overlay_idx].data, meg4.ovls[overlay_idx].size);
#endif
            }
        } else
        /* overlay table */
        if(py > 22 && py < 22 + 16 * 10 + 2 && px > 16 && px < 16 + 16 * 38 + 2) {
            overlay_idx = (((py - 23) / 10) << 4) | (((px - 17) / 38) & 0xf);
        }
    } else
    if(!clk) {
        key = meg4_api_popkey();
        if(!memcmp(&key, "Del", 4)) {
            if(meg4.ovls[overlay_idx].data) { free(meg4.ovls[overlay_idx].data); meg4.ovls[overlay_idx].data = NULL; }
            meg4.ovls[overlay_idx].size = 0;
        }
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void overlay_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int i, clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    char tmp[32];
    char *helper[] = { "00080", "..0047F", "00500", "..005FF", "00600", "..0FFFF", "10000", "..1FFFF", "20000", "..23FFF",
        "24000", "..27FFF", "28000", "..2FFFF", "30000", NULL };

    if(py > 22 && py < 22 + 16 * 10 + 2 && px > 16 && px < 16 + 16 * 38 + 2) {
        i = (((py - 23) / 10) << 4) | (((px - 17) / 38) & 0xf);
        sprintf(tmp, "%02X (%u)", i, i);
        meg4_text(dst, 120, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
    }

    /* delete */
    menu_icon(dst, dw, dh, dp, addrx - 26, 136, 48, 0, MEG4_KEY_DEL, STAT_ERASE);
    /* address */
    if(py < 12 && px >= addrx - 6 && py < addrx + 32) menu_stat = STAT_ADDR;
    meg4_text(dst, addrx - 6, 1, dp, theme[THEME_ACTIVE_BG], 0, 1, meg4_font, "@");
    meg4_box(dst, dw, dh, dp, addrx, 1, 32, 10, theme[THEME_INP_D], theme[THEME_INP_BG], theme[THEME_INP_L], 0, 0, 0, 0, 0);
    if(textinp_buf != addr)
        meg4_text(dst, addrx + 1, 1, dp, theme[THEME_FG], 0, 1, meg4_font, addr);
    /* address helper */
    else {
        meg4_box(dst, dw, dh, dp, addrx - 1, 12, addrw + 64, 84, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D],
            htole32(0x3f000000), 0, 0, 0, 0);
        sprintf(tmp, "..%05X", MEG4_MEM_LIMIT - 1);
        helper[15] = tmp;
        for(i = 0; i < 8; i++) {
            meg4_text(dst, addrx + 1, 14 + i * 10, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, helper[i * 2]);
            meg4_text(dst, addrx + 28, 14 + i * 10, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, helper[i * 2 + 1]);
            meg4_text(dst, addrx + 60, 14 + i * 10, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, lang[OVL_PALETTE + i]);
        }
    }
    /* size */
    if(py < 12 && px >= lenx - 6 && py < lenx + 40) menu_stat = STAT_LEN;
    meg4_text(dst, lenx - 4, 1, dp, theme[THEME_ACTIVE_BG], 0, 1, meg4_font, "[");
    meg4_box(dst, dw, dh, dp, lenx, 1, 40, 10, theme[THEME_INP_D], theme[THEME_INP_BG], theme[THEME_INP_L], 0, 0, 0, 0, 0);
    if(textinp_buf != len)
        meg4_text(dst, lenx + 38 - meg4_width(meg4_font, 1, len, NULL), 1, dp, theme[THEME_FG], 0, 1, meg4_font, len);
    meg4_text(dst, lenx + 42, 1, dp, theme[THEME_ACTIVE_BG], 0, 1, meg4_font, "]");
    /* save */
    if(px >= savex && px < savex + savew + 8 && py < 10 && clk)
        meg4_box(dst, dw, dh, dp, savex, 1, savew + 8, 10, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
    else
        meg4_box(dst, dw, dh, dp, savex, 1, savew + 8, 10, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
    meg4_text(dst, savex + 4, 2, dp, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[MENU_SAVE]);
    /* load */
    if(!meg4.ovls[overlay_idx].data || meg4.ovls[overlay_idx].size < 1) {
        meg4_box(dst, dw, dh, dp, loadx, 1, loadw + 8, 10, theme[THEME_ACTIVE_BG], theme[THEME_MENU_BG], theme[THEME_ACTIVE_BG], 0, 0, 0, 0, 0);
        meg4_text(dst, loadx + 4, 2, dp, theme[THEME_ACTIVE_BG], 0, 1, meg4_font, lang[MENU_LOAD]);
    } else {
        if(px >= loadx && px < loadx + loadw + 8 && py < 10 && clk)
            meg4_box(dst, dw, dh, dp, loadx, 1, loadw + 8, 10, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
        else
            meg4_box(dst, dw, dh, dp, loadx, 1, loadw + 8, 10, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
        meg4_text(dst, loadx + 4, 2, dp, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[MENU_LOAD]);
    }
    /* export */
    if(!meg4.ovls[overlay_idx].data || meg4.ovls[overlay_idx].size < 1
#ifdef EMBED
        || 1
#endif
    ) {
        meg4_box(dst, dw, dh, dp, 630 - expw, 1, expw + 8, 10, theme[THEME_ACTIVE_BG], theme[THEME_MENU_BG], theme[THEME_ACTIVE_BG], 0, 0, 0, 0, 0);
        meg4_text(dst, 634 - expw, 2, dp, theme[THEME_ACTIVE_BG], 0, 1, meg4_font, lang[MENU_EXPORT]);
    } else {
        if(px >= expx && px < expx + expw + 8 && py < 10 && clk)
            meg4_box(dst, dw, dh, dp, expx, 1, expw + 8, 10, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
        else
            meg4_box(dst, dw, dh, dp, expx, 1, expw + 8, 10, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
        meg4_text(dst, expx + 4, 2, dp, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[MENU_EXPORT]);
    }
}

/**
 * View
 */
void overlay_view(void)
{
    int i, j, l, m, y, y0, y1;
    char tmp[32], tmp2[64], *d;
    uint8_t *s;
    uint32_t fg;

    /* overlays table */
    meg4_box(meg4.valt, 640, 400, 2560, 16, 10, 16 * 38 + 2, 16 * 10 + 2, theme[THEME_D], theme[THEME_BG], theme[THEME_L],
        0, 0, 0, 10, 38);
    for(i = 0; i < 16; i++) {
        sprintf(tmp, "x%X", i);
        meg4_text(meg4.valt, 45 + i * 38, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
        sprintf(tmp, "%Xx", i);
        meg4_text(meg4.valt, 15 - meg4_width(meg4_font, 1, tmp, NULL), 11 + i * 10, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
    }
    for(i = 0; i < 256; i++) {
        if(overlay_idx == i) {
            meg4_box(meg4.valt, 640, 400, 2560, 17 + (i & 15) * 38, 11 + (i >> 4) * 10, 38, 10, theme[THEME_SEL_BG],
                theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            fg = theme[THEME_SEL_FG];
        } else
        if(!meg4.ovls[i].data || meg4.ovls[i].size < 1)
            fg = theme[THEME_L];
        else
            fg = theme[THEME_FG];
        sprintf(tmp, "%u", meg4.ovls[i].size);
        meg4_text(meg4.valt, 16 + (1 + (i & 15)) * 38 - meg4_width(meg4_font, 1, tmp, NULL), 11 + (i >> 4) * 10, 2560, fg, 0, 1, meg4_font,
            tmp);
    }
    /* current overlay */
    sprintf(tmp, "%02X", overlay_idx);
    meg4_text(meg4.valt, 30 - meg4_width(meg4_font, 1, tmp, NULL), 174, 2560, theme[THEME_FG], theme[THEME_D], 1, meg4_font, tmp);
    sprintf(tmp, "(%u)", overlay_idx);
    meg4_text(meg4.valt, 30 - meg4_width(meg4_font, 1, tmp, NULL), 182, 2560, theme[THEME_FG], theme[THEME_D], 1, meg4_font, tmp);
    /* hexdump of contents */
    if(!meg4.ovls[overlay_idx].data || meg4.ovls[overlay_idx].size < 1) {
        meg4_text(meg4.valt, (640 - unaw) / 2, 280, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
        menu_scrhgt = menu_scrmax = menu_scroll = 0;
    } else {
        meg4_box(meg4.valt, 640, 400, 2560, 32, 174, 16 * 37 + 2, 212, theme[THEME_D], theme[THEME_BG], theme[THEME_L],
            0, 0, menu_scroll, 10, 0);
        y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(175); y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(386);
        l = ((meg4.ovls[overlay_idx].size + 15) >> 4);
        menu_scrhgt = 210; menu_scrmax = l * 10;
        for(j = 0, s = meg4.ovls[overlay_idx].data, y = -menu_scroll; j < l && y < menu_scrhgt; j++, s += 16, y += 10) {
            if(y < -10) continue;
            memset(tmp2, 0, sizeof(tmp2));
            sprintf(tmp, "%08X", j << 4);
            meg4_text(meg4.valt, 35, 174 + y, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
            for(d = tmp2, m = 0; m < 16 && (j << 4) + m < (int)meg4.ovls[overlay_idx].size; m++) {
                sprintf(tmp, "%02X", s[m]);
                meg4_text(meg4.valt, 104 + m * 18 + (m > 7 ? 8 : 0), 174 + y, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                if(!(s[m] & 0x80)) { *d++ = s[m] >= ' ' ? s[m] : '.'; } else
                if((s[m] & 0xe0) == 0xc0 && (j << 4) + m + 1 < (int)meg4.ovls[overlay_idx].size && (s[m + 1] & 0xc0) == 0x80) {
                    *d++ = s[m]; *d++ = s[m + 1];
                } else
                if((s[m] & 0xf0) == 0xe0 && (j << 4) + m + 2 < (int)meg4.ovls[overlay_idx].size && (s[m + 1] & 0xc0) == 0x80 &&
                  (s[m + 2] & 0xc0) == 0x80) {
                    *d++ = s[m]; *d++ = s[m + 1]; *d++ = s[m + 2];
                } else *d++ = '.';
            }
            meg4_text(meg4.valt, 440, 174 + y, 2560, theme[THEME_FG], 0, -1, meg4_font, tmp2);
        }
        meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
    }
}
