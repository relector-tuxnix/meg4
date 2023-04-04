/*
 * meg4/editors/map.c
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
 * @brief Map editor
 *
 */

#include <stdio.h>
#include "editors.h"

static int tool = 0, mapsel = 0, dx = -1, dy = -1, ox, oy, mm, mb, ms, mp, mo = 0, mx = 0, zoom = 0, sprs = 0, spre = 0;
static int tilesx = 0, tilesy = 0, tileex = 0, tileey = 0, inpaste = 0;

/**
 * Zoom in
 */
void map_zoomin(int x, int y)
{
    if(zoom > 0) {
        zoom--;
        mx = ((zoom == 2 ? -152 : mx) + x) * 2 - x;
        menu_scroll = ((zoom == 2 ? - 47 : menu_scroll) + y) * 2 - y;
    }
}

/**
 * Zoom out
 */
void map_zoomout(int x, int y)
{
    if(zoom < 3) {
        zoom++;
        mx = (mx + x) / 2 - x;
        menu_scroll = (menu_scroll + y) / 2 - y;
    }
}

/**
 * Initialize map editor
 */
void map_init(void)
{
    mapsel = meg4.mmio.mapsel;
    toolbox_init(meg4.mmio.map, 320, 200);
}

/**
 * Free resources
 */
void map_free(void)
{
    toolbox_free();
}

/**
 * Controller
 */
int map_ctrl(void)
{
    int npix = (8 >> zoom), key, clk = le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R);
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(py >= 16 && py < 310 && px >= 4 && px < 628) {
        if(le16toh(meg4.mmio.ptrbtn) & MEG4_SCR_D) map_zoomout(px - 4, py - 16); else
        if(le16toh(meg4.mmio.ptrbtn) & MEG4_SCR_U) map_zoomin(px - 4, py - 16);
        if(le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_R) {
            if(!last) { dx = px; dy = py; ox = mx; oy = menu_scroll; }
            else { mx = ox - (px - dx); menu_scroll = oy - (py - dy); }
        }
    }
    if(ms > 0 && (mo || (py >= 392 && px >= mp && px < mp + mb))) {
        if(!last && clk)
            mo = px - mp;
        else if(clk)
            mx = (px - mo) * (mm - 624) / ms;
        else if(!clk)
            mo = 0;
    }
    if(px >= 4 && px < 4+624 && py >= 16 && py < 16+294) {
        tileex = (px - 4 + (zoom == 3 ? -152 : mx)) / npix;
        tileey = (py - 16 + (zoom == 3 ? -47 : menu_scroll)) / npix;
    } else
        tileex = tileey = -1;
    if(last && !clk) {
        /* failsafes */
        if(sprs < 0) sprs = 0;
        if(spre < sprs) spre = sprs;
        if(sprs > 255) sprs = 255;
        if(spre > 255) spre = 255;
        if(px >= 4 && px < 4+624 && py >= 16 && py < 16+294) {
            if(tool == 3)
                toolbox_selrect(0, 0, 320, 200, tilesx, tilesy, tileex - tilesx + 1, tileey - tilesy + 1);
            if(tool == 0 || inpaste)
                toolbox_histadd();
        } else
        if(py >= 314 && py < 327) {
            if(px >= 240 && px < 253) { map_zoomin(311, 146); } else
            if(px >= 256 && px < 269) { map_zoomout(311, 146); } else
            if(px >= 296 && px < 309) { if(mapsel > 0) { mapsel--; } else mapsel = 3; } else
            if(px >= 319 && px < 332) { if(mapsel < 3) { mapsel++; } else mapsel = 0; } else
            switch(toolbox_ctrl()) {
                case 0: toolbox_shl(0, 0, 320, 200); inpaste = 0; break;
                case 1: toolbox_shu(0, 0, 320, 200); inpaste = 0; break;
                case 2: toolbox_shd(0, 0, 320, 200); inpaste = 0; break;
                case 3: toolbox_shr(0, 0, 320, 200); inpaste = 0; break;
                case 4: toolbox_rcw(0, 0, 320, 200); inpaste = 0; break;
                case 5: toolbox_flv(0, 0, 320, 200); inpaste = 0; break;
                case 6: toolbox_flh(0, 0, 320, 200); inpaste = 0; break;
                case 7:  tool = 0; inpaste = 0; break;
                case 8:  tool = 1; inpaste = 0; break;
                case 9:  tool = 2; inpaste = 0; break;
                case 10: tool = 3; inpaste = 0; break;
                case 11: tool = 4; inpaste = 0; break;
            }
        } else
        if(py < 12) {
            if(px >= 552 && px < 564) toolbox_histundo(); else
            if(px >= 564 && px < 576) toolbox_histredo(); else
            if(px >= 576 && px < 588) goto cut; else
            if(px >= 588 && px < 600) goto copy; else
            if(px >= 600 && px < 612) goto paste; else
            if(px >= 618 && px < 630) goto del;
        }
    } else
    if(!last && clk) {
        if(px >= 4 && px < 4+624 && py >= 16 && py < 16+294) {
            tilesx = tileex; tilesy = tileey;
            switch(tool) {
                case 1: toolbox_fill(0, 0, 320, 200, tileex, tileey, sprs, spre); break;
                case 2: sprs = spre = toolbox_pick(0, 0, 320, 200, tileex, tileey); tool = 0; break;
                case 4: toolbox_selfuzzy(0, 0, 320, 200, tileex, tileey); break;
            }
        } else
        if(px >= 339 && px < 628 && py >= 317 && py < 390) {
            sprs = spre = ((py - 317) / 9) * 32 + (px - 339) / 9;
            if(tool > 1) tool = 0;
        }
    } else
    if(last && clk) {
        if(px >= 4 && px < 4+624 && py >= 16 && py < 16+294) {
            if(inpaste) {
                toolbox_paste(0, 0, 320, 200, tileex, tileey);
            } else
            if(tool == 0)
                toolbox_paint(0, 0, 320, 200, tileex, tileey, sprs, spre);
        } else
        if(px >= 339 && px < 628 && py >= 317 && py < 390) {
            spre = ((py - 317) / 9) * 32 + (px - 339) / 9;
        }
    } else {
        key = meg4_api_popkey();
        if(key) {
            if(!memcmp(&key, "Up", 3)) sprs = spre = (sprs - 32) & 0xff; else
            if(!memcmp(&key, "Down", 4)) sprs = spre = (sprs + 32) & 0xff; else
            if(!memcmp(&key, "Left", 4)) sprs = spre = (sprs - 1) & 0xff; else
            if(!memcmp(&key, "Rght", 4)) sprs = spre = (sprs + 1) & 0xff; else
            if(!memcmp(&key, "Undo", 4)) toolbox_histundo(); else
            if(!memcmp(&key, "Redo", 4)) toolbox_histredo(); else
            if(!memcmp(&key, "Sel", 4)) toolbox_selall(0, 0, 320, 200); else
            if(!memcmp(&key, "Inv", 4)) toolbox_selinv(0, 0, 320, 200); else
            if(!memcmp(&key, "Cut", 4)) {
cut:            inpaste = 0;
                toolbox_cut(0, 0, 320, 200);
            } else
            if(!memcmp(&key, "Cpy", 4)) {
copy:           inpaste = 0;
                toolbox_copy(0, 0, 320, 200);
            } else
            if(!memcmp(&key, "Pst", 4)) {
paste:          inpaste = 1; tool = 0;
            } else
            if(!memcmp(&key, "Del", 4)) {
del:            inpaste = 0;
                toolbox_del(0, 0, 320, 200);
            }
        }
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void map_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    char tmp[32];

    if(tileex >= 0 && tileex < 320 && tileey >= 0 && tileey < 200) {
        sprintf(tmp, "%u,%u", tileex, tileey);
        meg4_text(dst, 480, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
    }
    /* undo */      menu_icon(dst, dw, dh, dp, 552,   8, 56, 1, MEG4_KEY_Z, STAT_UNDO);
    /* redo */      menu_icon(dst, dw, dh, dp, 564,  16, 56, 1, MEG4_KEY_Y, STAT_REDO);
    /* cut */       menu_icon(dst, dw, dh, dp, 576, 136, 40, 1, MEG4_KEY_X, STAT_CUT);
    /* copy */      menu_icon(dst, dw, dh, dp, 588, 144, 40, 1, MEG4_KEY_C, STAT_COPY);
    /* paste */     menu_icon(dst, dw, dh, dp, 600, 152, 40, 1, MEG4_KEY_V, STAT_PASTE);
    /* delete */    menu_icon(dst, dw, dh, dp, 618, 136, 48, 0, MEG4_KEY_DEL, STAT_ERASE);
    /* additional cursors */
    if(tool >= 3 && tool <= 4 && px >= 4 && px < 4+624 && py >= 16 && py < 16+294) {
        if(meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL))
            meg4_blit(dst, px - MEG4_PTR_HOTSPOT_X + 4, py - MEG4_PTR_HOTSPOT_Y + 6, dp, 8, 8, meg4_edicons.buf, 120, 48, meg4_edicons.w * 4, 1);
        if(meg4_api_getkey(MEG4_KEY_LSHIFT) || meg4_api_getkey(MEG4_KEY_RSHIFT))
            meg4_blit(dst, px - MEG4_PTR_HOTSPOT_X + 4, py - MEG4_PTR_HOTSPOT_Y + 6, dp, 8, 8, meg4_edicons.buf, 128, 48, meg4_edicons.w * 4, 1);
    }

    toolbox_stat();
    if(py >= 314 && py < 327) {
        if(px >= 240 && px < 269) menu_stat = STAT_ZOOM;
        if(px >= 296 && px < 332) menu_stat = STAT_BANK;
    }
}

/**
 * View
 */
void map_view(void)
{
    int i, j, k, l, p, npix = (8 >> zoom);
    char tmp[2] = { 0 };

    menu_scrhgt = 294; menu_scrmax = 200 * npix;
    if(menu_scroll + menu_scrhgt >= menu_scrmax) menu_scroll = menu_scrmax - menu_scrhgt;
    if(menu_scroll < 0) menu_scroll = 0;
    /* vertical scrollbar */
    mm = 320 * npix;
    if(mx + 624 > mm) mx = mm - 624;
    if(mx < 0) mx = 0;
    if(mm <= 624) {
        mb = ms = 0;
        meg4_box(meg4.valt, 640, 388, 2560, 0, 380, 632, 8, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], 0, 0, 0, 0, 0);
    } else {
        mb = 8 + 624 * 624 / mm;
        ms = 632 - mb;
        mp = ms * mx / (mm - 624);
        meg4_box(meg4.valt, 640, 388, 2560, 0, 380, 632, 8, theme[THEME_INP_BG], theme[THEME_INP_BG], theme[THEME_INP_BG], 0, 0, 0, 0, 0);
        if(mo)
            meg4_box(meg4.valt, 640, 388, 2560, mp, 380, mb, 8, theme[THEME_MENU_D], theme[THEME_MENU_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
        else
            meg4_box(meg4.valt, 640, 388, 2560, mp, 380, mb, 8, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], 0, 0, 0, 0, 0);
    }
    /* map */
    meg4_box(meg4.valt, 640, 388, 2560, 3, 3, 626, 296, theme[THEME_D], 0, theme[THEME_L], 0, 0, 0, 0, 0);
    toolbox_map(4, 4, 2560, 624, 294, zoom == 3 ? -152 : mx, zoom == 3 ? -47 : menu_scroll, zoom, mapsel, tileex, tileey,
        tool == 0 ? (inpaste ? -1 : sprs) : 256, spre);
    if(tool == 3 && (le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L) && le16toh(meg4.mmio.ptrx) >= 4 && le16toh(meg4.mmio.ptrx) < 4+624 &&
      le16toh(meg4.mmio.ptry) >= 16 && le16toh(meg4.mmio.ptry) < 16+294) {
        meg4_box(meg4.valt, 640, 388, 2560, 4 - (zoom == 3 ? -152 : mx) + tilesx * npix,
            4 - (zoom == 3 ? -47 : menu_scroll) + tilesy * npix, (tileex - tilesx + 1) * npix, (tileey - tilesy + 1) * npix,
            theme[THEME_SEL_BG], 0, theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
      }
    /* tool buttons */
    toolbox_view(10, 302, tool);
    toolbox_btn(240, 302, 0x0e, 0);
    toolbox_btn(256, 302, 0x0f, 0);
    toolbox_btn(296, 302, 0x11, 0);
    tmp[0] = '0' + mapsel;
    meg4_text(meg4.valt, 312, 304, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
    toolbox_btn(319, 302, 0x10, 0);

    /* sprite palette */
    meg4_box(meg4.valt, 640, 388, 2560, 338, 302, 291, 75, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 9, 9);
    for(j = k = 0; j < 8; j++) {
        for(i = 0; i < 32; i++, k++)
            if(k) toolbox_blit(340 + i * 9, 304 + j * 9, 2560, 8, 8, i << 3, ((mapsel << 3) + j) << 3, 8, 8, 256, -1);
            else meg4_blit(meg4.valt, 340 + i * 9, 304 + j * 9, 2560, 8, 8, meg4_edicons.buf, 144, 48, meg4_edicons.w * 4, 1);
    }
    l = spre < sprs ? sprs : spre;
    j = sprs >> 5; i = sprs & 31; k = (l >> 5) - j + 1; p = (l & 31) - i + 1;
    if(i + p > 32) p = 32 - i;
    if(j + k > 8) k = 8 - j;
    meg4_box(meg4.valt, 640, 388, 2560, 339 + i * 9, 303 + j * 9, p * 9 + 1, k * 9 + 1, theme[THEME_SEL_FG], 0,
        theme[THEME_SEL_FG], 0, 0, 0, 0, 0);
}
