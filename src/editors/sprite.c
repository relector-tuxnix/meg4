/*
 * meg4/editors/sprite.c
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
 * @brief Sprite editor
 *
 */

#include <stdio.h>
#include "editors.h"

extern int tcw, tch;
static int wins = 0, wine = 0, npix = 0, tool = 0, palidx = 0, picker = 0, hue = 0, sat = 0, val = 0;
static int sprsx = 0, sprsy = 0, sprex = 0, sprey = 0, inpaste = 0;

/**
 * Color conversion sRGB to HSV
 */
static void rgb2hsv(uint32_t c, int *h, int *s, int *v)
{
    int r = (int)(((uint8_t*)&c)[2]), g = (int)(((uint8_t*)&c)[1]), b = (int)(((uint8_t*)&c)[0]), m, d;

    m = r < g? r : g; if(b < m) m = b;
    *v = r > g? r : g; if(b > *v) *v = b;
    d = *v - m; *h = 0;
    if(!*v) { *s = 0; return; }
    *s = d * 255 / *v;
    if(!*s) return;

    if(r == *v) *h = 43*(g - b) / d;
    else if(g == *v) *h = 85 + 43*(b - r) / d;
    else *h = 171 + 43*(r - g) / d;
    if(*h < 0) *h += 256;
}

/**
 * Color conversion HSV to sRGB
 */
static uint32_t hsv2rgb(int a, int h, int s, int v)
{
    int i, f, p, q, t;
    uint32_t c = (a & 255) << 24;

    if(!s) { ((uint8_t*)&c)[2] = ((uint8_t*)&c)[1] = ((uint8_t*)&c)[0] = v; }
    else {
        if(h > 255) i = 0; else i = h / 43;
        f = (h - i * 43) * 6;
        p = (v * (255 - s) + 127) >> 8;
        q = (v * (255 - ((s * f + 127) >> 8)) + 127) >> 8;
        t = (v * (255 - ((s * (255 - f) + 127) >> 8)) + 127) >> 8;
        switch(i) {
            case 0:  ((uint8_t*)&c)[2] = v; ((uint8_t*)&c)[1] = t; ((uint8_t*)&c)[0] = p; break;
            case 1:  ((uint8_t*)&c)[2] = q; ((uint8_t*)&c)[1] = v; ((uint8_t*)&c)[0] = p; break;
            case 2:  ((uint8_t*)&c)[2] = p; ((uint8_t*)&c)[1] = v; ((uint8_t*)&c)[0] = t; break;
            case 3:  ((uint8_t*)&c)[2] = p; ((uint8_t*)&c)[1] = q; ((uint8_t*)&c)[0] = v; break;
            case 4:  ((uint8_t*)&c)[2] = t; ((uint8_t*)&c)[1] = p; ((uint8_t*)&c)[0] = v; break;
            default: ((uint8_t*)&c)[2] = v; ((uint8_t*)&c)[1] = p; ((uint8_t*)&c)[0] = q; break;
        }
    }
    return c;
}

/**
 * Initialize sprite editor
 */
void sprite_init(void)
{
    picker = inpaste = 0;
    toolbox_init(meg4.mmio.sprites, 256, 256);
}

/**
 * Free resources
 */
void sprite_free(void)
{
    toolbox_free();
    meg4_recalcmipmap();
}

/**
 * Controller
 */
int sprite_ctrl(void)
{
    uint8_t *C = (uint8_t*)&meg4.mmio.palette[palidx];
    int key, clk = le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R), px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(picker) {
        /* HSV color picker */
        if(last && !clk) {
            if(picker > 1) picker = 1; else
            if(py < 56 || py >= 56+302 || px < 164 || px >= 164+312)
                picker = 0;
            else
            if(py >= 307+12 && py < 307+25) {
                if(px >= 250 && px < 263 && C[0] < 255) C[0]++;
                if(px >= 304 && px < 317 && C[1] < 255) C[1]++;
                if(px >= 358 && px < 371 && C[2] < 255) C[2]++;
                if(px >= 412 && px < 425 && C[3] < 255) C[3]++;
            } else
            if(py >= 328+12 && py < 328+25) {
                if(px >= 250 && px < 263 && C[0] > 0) C[0]--;
                if(px >= 304 && px < 317 && C[1] > 0) C[1]--;
                if(px >= 358 && px < 371 && C[2] > 0) C[2]--;
                if(px >= 412 && px < 425 && C[3] > 0) C[3]--;
            }
        } else
        if(!last && clk) {
            if(py >= 60 && py < 60+256) {
                if(px >= 174 && px < 190) picker = 2;
                if(px >= 192 && px < 448) picker = 3;
                if(px >= 450 && px < 566) picker = 4;
            }
        }
        if(clk) {
            switch(picker) {
                case 2:
                    if(py >= 60 && py < 60+256) C[3] = 255 - (py - 60);
                break;
                case 3:
                    if(py >= 60 && py < 60+256)
                        val = 255 - (py - 60);
                    if(px >= 192 && px < 448)
                        sat = px - 192;
                    meg4.mmio.palette[palidx] = hsv2rgb(C[3], hue, sat, val);
                break;
                case 4:
                    if(py >= 60 && py < 60+256)
                        hue = (py - 60);
                    meg4.mmio.palette[palidx] = hsv2rgb(C[3], hue, sat, val);
                break;
            }
        }
    } else {
        if(px >= 11 && px < 11+256 && py >= 23 && py < 23+256) {
            sprex = ((px - 11) * npix) >> 8;
            sprey = ((py - 23) * npix) >> 8;
        } else
            sprex = sprey = -1;
        if(last && !clk) {
            /* failsafes */
            if(wins < 0) wins = 0;
            if(wine < wins) wine = wins;
            if(wins > 1023) wins = 1023;
            if(wine > 1023) wine = 1023;
            /* sprite editor area release */
            if(px >= 11 && px < 11+256 && py >= 23 && py < 23+256) {
                if(tool == 3)
                    toolbox_selrect(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, sprsx, sprsy, sprex - sprsx + 1, sprey - sprsy + 1);
                if(tool == 0 || inpaste)
                    toolbox_histadd();
            }
            /* sprite selector area relase */
            if(px >= 12 && px < 589 && py >= 335 && py < 372) {
                palidx = ((py - 335) / 9) * 64 + (px - 12) / 9;
                if(tool > 1) tool = 0;
            } else
            /* HSV picker button click */
            if(palidx && py >= 366 + 12 && py < 366 + 25 && px >= 26 && px < 39)
                picker = 1;
            else
            /* menubar click */
            if(py < 12) {
                if(px >= 552 && px < 564) toolbox_histundo(); else
                if(px >= 564 && px < 576) toolbox_histredo(); else
                if(px >= 576 && px < 588) goto cut; else
                if(px >= 588 && px < 600) goto copy; else
                if(px >= 600 && px < 612) goto paste; else
                if(px >= 618 && px < 630) goto del;
            } else
            /* toolbar */
            switch(toolbox_ctrl()) {
                case 0: toolbox_shl(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 1: toolbox_shu(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 2: toolbox_shd(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 3: toolbox_shr(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 4: toolbox_rcw(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 5: toolbox_flv(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 6: toolbox_flh(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); inpaste = 0; break;
                case 7:  tool = 0; inpaste = 0; break;
                case 8:  tool = 1; inpaste = 0; break;
                case 9:  tool = 2; inpaste = 0; break;
                case 10: tool = 3; inpaste = 0; break;
                case 11: tool = 4; inpaste = 0; break;
            }
        } else
        if(!last && clk) {
            /* sprite editor area click */
            if(px >= 11 && px < 11+256 && py >= 23 && py < 23+256) {
                sprsx = sprex; sprsy = sprey;
                switch(tool) {
                    case 1: toolbox_fill(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, sprex, sprey, palidx, palidx); break;
                    case 2: palidx = toolbox_pick(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, sprex, sprey); tool = 0; break;
                    case 4: toolbox_selfuzzy(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, sprex, sprey); break;
                }
            } else
            /* sprite selector click */
            if(px >= 339 && px < 629 && py >= 23 && py < 313) {
                wins = wine = ((py - 23) / 9) * 32 + (px - 339) / 9;
            }
        } else
        if(last && clk) {
            /* sprite editor area movement */
            if(px >= 11 && px < 11+256 && py >= 23 && py < 23+256) {
                if(inpaste) {
                    toolbox_paste(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, sprex, sprey);
                } else
                if(tool == 0)
                    toolbox_paint(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, sprex, sprey, palidx, palidx);
            } else
            /* sprite selector area movement */
            if(px >= 339 && px < 629 && py >= 23 && py < 313) {
                wine = ((py - 23) / 9) * 32 + (px - 339) / 9;
                if(wine > 1023) wine = 1023;
            }
        } else {
            key = meg4_api_popkey();
            if(key) {
                if(!memcmp(&key, "Up", 3)) wins = wine = (wins - 32) & 0x3ff; else
                if(!memcmp(&key, "Down", 4)) wins = wine = (wins + 32) & 0x3ff; else
                if(!memcmp(&key, "Left", 4)) wins = wine = (wins - 1) & 0x3ff; else
                if(!memcmp(&key, "Rght", 4)) wins = wine = (wins + 1) & 0x3ff; else
                if(!memcmp(&key, "Undo", 4)) toolbox_histundo(); else
                if(!memcmp(&key, "Redo", 4)) toolbox_histredo(); else
                if(!memcmp(&key, "Sel", 4)) toolbox_selall(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); else
                if(!memcmp(&key, "Inv", 4)) toolbox_selinv(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix); else
                if(!memcmp(&key, "Cut", 4)) {
cut:                inpaste = 0;
                    toolbox_cut(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix);
                } else
                if(!memcmp(&key, "Cpy", 4)) {
copy:               inpaste = 0;
                    toolbox_copy(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix);
                } else
                if(!memcmp(&key, "Pst", 4)) {
paste:              inpaste = 0; tool = 0;
                    /* if the entire sprite editor area is selected, paste at once */
                    if(tcw == npix && tch == npix)
                        toolbox_paste(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix, 0, 0);
                    else
                        inpaste = 1;
                } else
                if(!memcmp(&key, "Del", 4)) {
del:                inpaste = 0;
                    toolbox_del(((wins & 31) << 3), ((wins >> 5) << 3), npix, npix);
                }
            }
        }
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void sprite_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    char tmp[32];

    if(sprex >= 0 && sprey >= 0) {
        sprintf(tmp, "%u, %u", sprex, sprey);
        meg4_text(dst, 130, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
    }

    sprintf(tmp, "%03X (%u)", wins, wins);
    meg4_text(dst, 480, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);

    /* undo */      menu_icon(dst, dw, dh, dp, 552,   8, 56, 1, MEG4_KEY_Z, STAT_UNDO);
    /* redo */      menu_icon(dst, dw, dh, dp, 564,  16, 56, 1, MEG4_KEY_Y, STAT_REDO);
    /* cut */       menu_icon(dst, dw, dh, dp, 576, 136, 40, 1, MEG4_KEY_X, STAT_CUT);
    /* copy */      menu_icon(dst, dw, dh, dp, 588, 144, 40, 1, MEG4_KEY_C, STAT_COPY);
    /* paste */     menu_icon(dst, dw, dh, dp, 600, 152, 40, 1, MEG4_KEY_V, STAT_PASTE);
    /* delete */    menu_icon(dst, dw, dh, dp, 618, 136, 48, 0, MEG4_KEY_DEL, STAT_ERASE);
    if(!picker) {
        /* additional cursors */
        if(tool >= 3 && tool <= 4 && px >= 11 && px < 11+256 && py >= 23 && py < 23+256) {
            if(meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL))
                meg4_blit(dst, px - MEG4_PTR_HOTSPOT_X + 4, py - MEG4_PTR_HOTSPOT_Y + 6, dp, 8, 8, meg4_edicons.buf, 120, 48, meg4_edicons.w * 4, 1);
            if(meg4_api_getkey(MEG4_KEY_LSHIFT) || meg4_api_getkey(MEG4_KEY_RSHIFT))
                meg4_blit(dst, px - MEG4_PTR_HOTSPOT_X + 4, py - MEG4_PTR_HOTSPOT_Y + 6, dp, 8, 8, meg4_edicons.buf, 128, 48, meg4_edicons.w * 4, 1);
        }
        toolbox_stat();
        if(py >= 22 && py < 313 && px >= 338 && px < 629) menu_stat = STAT_SPRSEL;
        if(py >= 366 + 12 && py < 366 + 25 && px >= 26 && px < 39) menu_stat = STAT_HSV;
    }
}

/**
 * View
 */
void sprite_view(void)
{
    uint32_t *dst, h;
    uint8_t *d, *a = (uint8_t*)&theme[THEME_L], *b = (uint8_t*)&theme[THEME_D], *C;
    int i, j, p, k, l;
    char tmp[64];

    /* sprite table */
    meg4_box(meg4.valt, 640, 388, 2560, 338, 10, 291, 291, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 9, 9);
    for(j = 0; j < 32; j++) {
        sprintf(tmp, "%X", j);
        meg4_text(meg4.valt, 340 + j * 9, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
        sprintf(tmp, "%03Xx", j * 2);
        meg4_text(meg4.valt, 337 - meg4_width(meg4_font, 1, tmp, NULL), 10 + j * 9, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, tmp);
        for(i = 0; i < 32; i++)
            toolbox_blit(340 + i * 9, 12 + j * 9, 2560, 8, 8, i << 3, j << 3, 8, 8, 256, 0);
    }
    l = wine < wins ? wins : wine;
    j = wins >> 5; i = wins & 31; k = (l >> 5) - j + 1; p = (l & 31) - i + 1;
    if(p < k) p = k;
    if(k < p) k = p;
    if(i + k > 32) k = 32 - i;
    if(j + k > 32) k = 32 - j;
    meg4_box(meg4.valt, 640, 388, 2560, 339 + i * 9, 11 + j * 9, k * 9 + 1, k * 9 + 1, theme[THEME_SEL_BG], 0,
        theme[THEME_SEL_BG], 0, 0, 0, 0, 0);

    /* editor box */
    npix = k << 3;
    meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 258, 258, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 4, 4);
    toolbox_blit(11, 11, 2560, 256, 256, i << 3, j << 3, npix, npix, 256, 1);
    if(le16toh(meg4.mmio.ptrx) >= 11 && le16toh(meg4.mmio.ptrx) < 11+256 && le16toh(meg4.mmio.ptry) >= 23 && le16toh(meg4.mmio.ptry) < 23+256) {
        if(tool == 3 && (le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L)) {
            meg4_box(meg4.valt, 640, 388, 2560, 11 + (sprsx << 8) / npix, 11 + (sprsy << 8) / npix,
                (((((le16toh(meg4.mmio.ptrx) - 11) * npix) >> 8) - sprsx + 1) << 8) / npix,
                (((((le16toh(meg4.mmio.ptry) - 23) * npix) >> 8) - sprsy + 1) << 8) / npix,
                theme[THEME_SEL_BG], 0, theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
        } else
        if(inpaste)
            toolbox_pasteview(i << 3, j << 3, npix, npix, 11, 11, 256, 256, sprex, sprey);
    }
    /* tool buttons */
    toolbox_view(10, 276, tool);
    /* palette */
    meg4_box(meg4.valt, 640, 388, 2560, 10, 320, 579, 39, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 3, 3);
    for(j = k = 0; j < 4; j++)
        for(i = 0; i < 64; i++, k++) {
            if(palidx == k)
                meg4_box(meg4.valt, 640, 388, 2560, 11 + i * 9, 321 + j * 9, 10, 10, theme[THEME_SEL_FG], 0, theme[THEME_SEL_FG],
                    0, 0, 0, 0, 0);
            h = meg4.mmio.palette[k];
            if(k) meg4_blitd(meg4.valt, 12 + i * 9, 322 + j * 9, 2560, 8, 8, 8, &h, 0, 0, 1, 1, 4);
            else meg4_blit(meg4.valt, 12 + i * 9, 322 + j * 9, 2560, 8, 8, meg4_edicons.buf, 144, 48, meg4_edicons.w * 4, 1);
        }
    meg4_box(meg4.valt, 640, 388, 2560, 10, 366, 13, 13, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 3, 3);
    h = meg4.mmio.palette[palidx];
    meg4_blitd(meg4.valt, 11, 367, 2560, 11, 11, 11, &h, 0, 0, 1, 1, 4);
    /* no palette modification for index 0, that's the "clear" color */
    if(!palidx) {
        meg4_box(meg4.valt, 640, 388, 2560, 26, 366, 13, 13, theme[THEME_D], theme[THEME_L], theme[THEME_D], 0, 0, 0, 0, 0);
        meg4_blit(meg4.valt, 29, 369, 2560, 8, 8, meg4_edicons.buf, 152, 48, meg4_edicons.w * 4, 1);
    } else {
        if((le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L) && le16toh(meg4.mmio.ptry) >= 366 + 12 && le16toh(meg4.mmio.ptry) < 366 + 25 &&
          le16toh(meg4.mmio.ptrx) >= 26 && le16toh(meg4.mmio.ptrx) < 39) {
            meg4_box(meg4.valt, 640, 388, 2560, 26, 366, 13, 13, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
            meg4_blit(meg4.valt, 29, 370, 2560, 8, 8, meg4_edicons.buf, 152, 48, meg4_edicons.w * 4, 1);
        } else {
            meg4_box(meg4.valt, 640, 388, 2560, 26, 366, 13, 13, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
            meg4_blit(meg4.valt, 29, 369, 2560, 8, 8, meg4_edicons.buf, 152, 48, meg4_edicons.w * 4, 1);
        }
    }
    sprintf(tmp, "%02X (%u)", palidx, palidx);
    meg4_text(meg4.valt, 48, 368, 2560, theme[THEME_FG], theme[THEME_D], 1, meg4_font, tmp);
    /* color picker */
    if(picker) {
        meg4_box(meg4.valt, 640, 388, 2560, 164, 44, 312, 302,
            theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], htole32(0x3f000000), 0, 0, 0, 0);
        dst = meg4.valt + 48 * 640 + 166;
        C = (uint8_t*)&meg4.mmio.palette[palidx];
        for(p = 8, j = 0; j < 256; j++, p += 640) {
            if(C[3] == 255-j) {
                dst[p - 4] = theme[THEME_SEL_FG]; dst[p - 3] = theme[THEME_SEL_FG]; dst[p - 2] = theme[THEME_SEL_FG];
                dst[p - 3 - 640] = theme[THEME_SEL_FG]; dst[p - 3 + 640] = theme[THEME_SEL_FG];
                dst[p - 4 - 2*640] = theme[THEME_SEL_FG]; dst[p - 4 - 640] = theme[THEME_SEL_FG];
                dst[p - 4 + 640] = theme[THEME_SEL_FG]; dst[p - 4 + 2*640] = theme[THEME_SEL_FG];
                for(i = 0; i < 15; i++)
                    dst[p + i] = theme[THEME_SEL_FG];
            } else
                for(i = 0; i < 15; i++) {
                    d = (j & 8) ^ (i & 8) ? a : b;
                    ((uint8_t*)&dst[p+i])[0] = (d[0]*j + (256 - j)*C[0])>>8;
                    ((uint8_t*)&dst[p+i])[1] = (d[1]*j + (256 - j)*C[1])>>8;
                    ((uint8_t*)&dst[p+i])[2] = (d[2]*j + (256 - j)*C[2])>>8;
                }
        }
        rgb2hsv(htole32(0xFF000000) | meg4.mmio.palette[palidx], &hue, &sat, &val);
        for(p = 26, j = 0; j < 256; j++, p += 640) {
            for(i = 0; i < 256; i++) {
                h = 255-j == val ? j+(((val * i) >> 8) & 0xFF) : j; h |= htole32(0xFF000000) | (h << 16) | (h << 8);
                dst[p + i] = sat == i || 255-j == val ? h : hsv2rgb(255, hue,i,255-j);
            }
        }
        for(p = 26 + 256 + 2, j = 0; j < 256; j++, p += 640) {
            h = hsv2rgb(255, j, 255, 255);
            if(hue == j) {
                h = theme[THEME_SEL_FG];
                dst[p + 19] = h; dst[p + 18] = h; dst[p + 17] = h; dst[p + 18 - 640] = h;
                dst[p + 18 + 640] = h; dst[p + 19 - 2*640] = h;
                dst[p + 19 - 640] = h; dst[p + 19 + 640] = h;
                dst[p + 19 + 2*640] = h;
            }
            for(i = 0; i < 15; i++)
                dst[p + i] = h;
        }
        sprintf(tmp, "R:%02X  G:%02X  B:%02X  A:%02X", C[0], C[1], C[2], C[3]);
        meg4_text(meg4.valt, 232, 320, 2560, theme[THEME_MENU_FG], 0, -1, meg4_font, tmp);
        /* R buttons */ toolbox_btn(250, 307, 0x1e, 0); toolbox_btn(250, 328, 0x1f, 0);
        /* G buttons */ toolbox_btn(304, 307, 0x1e, 0); toolbox_btn(304, 328, 0x1f, 0);
        /* B buttons */ toolbox_btn(358, 307, 0x1e, 0); toolbox_btn(358, 328, 0x1f, 0);
        /* A buttons */ toolbox_btn(412, 307, 0x1e, 0); toolbox_btn(412, 328, 0x1f, 0);
    }
}
