/*
 * meg4/editors/menu.c
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
 * @brief Editor menus
 *
 */

#include <stdio.h>
#include "editors.h"

extern const char *copyright[3];
static int menu_width = 0, menu_subwidth = 0, menu_active = 0, menu_sel = - 1, menu_pro = 0, menu_last = 0, subw = 0;
static char *keys[10] = { "Ctrl\0R", "Ctrl\0S", "Ctrl\0L", "Ctrl\0⏎", "F1\0", "GUI\0", "AltGr\0", "Alt\0I", "Alt\0K", "Alt\0J" };
static int keyw[20], titlew[HELP_TOC - MENU_HELP], zipt, zipb, zipo, zipw, zipbin = 0;
int menu_scroll, menu_scrmax, menu_scrhgt, menu_scrbtn, menu_scrpos, menu_scrsiz, menu_scrofs, menu_stat = 0, last = 0;

/**
 * Display a keyboard shortcut
 */
static void shortcut_view(uint32_t *dst, int x, int y, int dp, int idx)
{
    int i = 0;
    x -= 4 + keyw[idx * 2 + 0] + (keyw[idx * 2 + 1] ? 5 + keyw[idx * 2 + 1] : 0);
    meg4_blit(dst, x, y, dp, 3, 8, meg4_edicons.buf, 128, 40, meg4_edicons.w * 4, 1);
    x += 2;
    for(i = 1; i < keyw[idx * 2] - 1; i++)
        meg4_blit(dst, x + i, y, dp, 1, 8, meg4_edicons.buf, 131, 40, meg4_edicons.w * 4, 1);
    meg4_blit(dst, x + i, y, dp, 3, 8, meg4_edicons.buf, 132, 40, meg4_edicons.w * 4, 1);
    meg4_text(dst, x, y, dp, theme[THEME_HELP_KEY], 0, 1, meg4_font, keys[idx]);
    if(keyw[idx * 2 + 1] > 0) {
        x += i + 4;
        meg4_blit(dst, x, y, dp, 3, 8, meg4_edicons.buf, 128, 40, meg4_edicons.w * 4, 1);
        x += 2;
        for(i = 1; i < keyw[idx * 2 + 1] - 1; i++)
            meg4_blit(dst, x + i, y, dp, 1, 8, meg4_edicons.buf, 131, 40, meg4_edicons.w * 4, 1);
        meg4_blit(dst, x + i, y, dp, 3, 8, meg4_edicons.buf, 132, 40, meg4_edicons.w * 4, 1);
        meg4_text(dst, x, y, dp, theme[THEME_HELP_KEY], 0, 1, meg4_font, keys[idx] + strlen(keys[idx]) + 1);
    }
}

/**
 * Initialize menu
 */
void menu_init(void)
{
    int i, j, w = 0, k = 0;

    menu_active = menu_width = menu_scrmax = menu_scrhgt = 0;
    menu_pro = meg4_width(meg4_font, 1, lang[MENU_EXPWASM], NULL);
    for(i = 0; i < 10; i++) {
        keyw[i * 2 + 0] = meg4_width(meg4_font, 1, keys[i], NULL);
        keyw[i * 2 + 1] = meg4_width(meg4_font, 1, keys[i] + strlen(keys[i]) + 1, NULL);
        w = 4 + keyw[i * 2 + 0] + (keyw[i * 2 + 1] ? 5 + keyw[i * 2 + 1] : 0);
        if(w > k) k = w;
    }
    subw = meg4_width(meg4_font, 1, ">", NULL);
    for(i = MENU_ABOUT; i <= MENU_HELP; i++) {
        j = meg4_width(meg4_font, 1, lang[i], NULL) + (i == MENU_EXPWASM ? menu_pro : 0) + 8 + k;
        if(j > menu_width) menu_width = j;
    }
    menu_pro += 20;
    menu_subwidth = 0;
    for(i = MENU_CODEPOINT; i <= MENU_HIRAGANA; i++) {
        j = meg4_width(meg4_font, 1, lang[i], NULL) + 12 + k;
        if(j > menu_subwidth) menu_subwidth = j;
    }
    for(i = MENU_HELP; i < HELP_TOC; i++)
        titlew[i - MENU_HELP] = meg4_width(meg4_font, 1, lang[i], NULL) / 2;
    zipt = meg4_width(meg4_font, 2, lang[MENU_EXPZIP], NULL);
    zipb = meg4_width(meg4_font, 1, lang[MENU_BINARY], NULL);
    zipo = meg4_width(meg4_font, 1, lang[MENU_OK], NULL);
    zipw = zipt < 128 ? 128 : zipt;
    if(zipw < zipb + zipo + 32) zipw = zipb + zipo + 32;
}

/**
 * Controller
 */
int menu_ctrl(void)
{
    int clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry), l = menu_last;
    if(meg4.mode < MEG4_MODE_HELP && !load_list) return 0;
    /* peek into the keyboard queue if textinput isn't active and we aren't on the help page.
     * If there's an Escape key waiting, switch back to the game */
    if(!textinp_buf && meg4.mode != MEG4_MODE_HELP && meg4.mode != MEG4_MODE_CODE && !menu_active &&
      meg4.mmio.kbdhead != meg4.mmio.kbdtail && !memcmp(&meg4.mmio.kbd[(int)meg4.mmio.kbdtail], "\x1b", 2)) {
        meg4_api_popkey();
        if(load_list) { load_list = 0; meg4_getscreen(); }
        else meg4_switchmode(MEG4_MODE_GAME);
        return 1;
    }
    menu_last = clk;
    /* scrollbar */
    if(!menu_scrhgt || menu_scrmax < menu_scrhgt) menu_scrmax = 0;
    if(menu_scroll + menu_scrhgt > menu_scrmax) menu_scroll = menu_scrmax - menu_scrhgt;
    if(menu_scroll < 0) menu_scroll = 0;
    if(meg4.mode != MEG4_MODE_MAP && meg4.mmio.ptrbtn & MEG4_SCR_U) {
        menu_scroll -= 4; if(menu_scroll < 0) menu_scroll = 0;
        meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
        return 1;
    } else
    if(meg4.mode != MEG4_MODE_MAP && meg4.mmio.ptrbtn & MEG4_SCR_D) {
        menu_scroll += 4; if(menu_scroll > menu_scrmax - menu_scrhgt) menu_scroll = menu_scrmax - menu_scrhgt;
        if(menu_scroll < 0) menu_scroll = 0;
        meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
        return 1;
    }
    if(menu_scrsiz > 0 && (menu_scrofs || (px >= 632 && py >= menu_scrpos && py < menu_scrpos + menu_scrbtn))) {
        if(!l && clk)
            menu_scrofs =  py - menu_scrpos;
        else if(clk) {
            menu_scroll = (py - 12 - menu_scrofs) * (menu_scrmax - menu_scrhgt) / menu_scrsiz;
            if(menu_scroll > menu_scrmax - menu_scrhgt) menu_scroll = menu_scrmax - menu_scrhgt;
            if(menu_scroll < 0) menu_scroll = 0;
        } else if(!clk) menu_scrofs = 0;
        return 1;
    }
    /* menubar click release */
    if(l && !clk && py < 12) {
        if(px < 20) { if(menu_active == 1) menu_active = 0; else menu_active = 1; }
        if(px >= 20 && px < 20 + (MEG4_NUM_MODE - MEG4_MODE_CODE) * 12) {
            menu_active = last = 0; meg4_switchmode(MEG4_MODE_CODE + (px - 20) / 12);
        }
        return 1;
    }
    /* about window controller */
    if(menu_active == -1) {
        if(((l && !clk) || meg4_api_popkey())) menu_active = last = 0;
        return 1;
    } else
    /* export zip window controller */
    if(menu_active == -2) {
        textinp_ctrl();
        if(textinp_key == htole32('\n') && meg4_title[0]) { meg4_export(NULL, zipbin); menu_active = 0; textinp_free(); } else
        if(textinp_key == htole32('\x1b')) { menu_active = last = 0; } else
        if(l && !clk) {
            if(px < (632 - zipw) / 2 || px > (632 - zipw) / 2 + zipw + 8 ||
              py < 178 || py > 226) { menu_active = 0; textinp_free(); } else
            if(px >= 260 && px < 268 + zipb &&
              py >= 211 && py < 219) { zipbin ^= 1; } else
            if(px >= 260 && px < 380 &&
              py >= 198 && py < 210 && !textinp_buf) {
                textinp_init(260, 198, 120, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_NAME, meg4_title, sizeof(meg4_title));
            } else
            if(px >= 364 - zipo && px < 380 &&
              py >= 209 && py < 221) { meg4_export(NULL, zipbin); menu_active = last = 0; textinp_free(); }
        }
        return 1;
    } else
    /* popup menu controller */
    if(menu_active > 0) {
        if(l && !clk) {
            switch(menu_sel) {
                case MENU_ABOUT: menu_active = -1; return 1;
                case MENU_RUN: meg4_switchmode(!cpu_compile() ? MEG4_MODE_CODE : MEG4_MODE_GAME); break;
                case MENU_SAVE: meg4_switchmode(MEG4_MODE_SAVE); break;
#ifndef EMBED
                case MENU_IMPORT: main_openfile(); break;
                case MENU_EXPZIP:
                    if(!meg4_title[0]) strcpy(meg4_title, lang[MENU_NONAME]);
                    textinp_init(260, 198, 120, theme[THEME_INP_FG], 0, 1, meg4_font, 1, TEXTINP_NAME, meg4_title, sizeof(meg4_title));
                    menu_active = -2;
                    return 1;
#endif
#ifdef MEG4_PRO
                case MENU_EXPWASM: pro_export(); break;
#endif
                case MENU_FULLSCR: main_fullscreen(); break;
                case MENU_HELP: meg4_switchmode(MEG4_MODE_HELP); break;
                case MENU_CODEPOINT: meg4_pushkey("\001"); break;
                case MENU_COMPOSE: meg4_pushkey("\002"); break;
                case MENU_ICONEMOJI: meg4_pushkey("\003"); break;
                case MENU_KATAKANA: meg4_pushkey("\004"); break;
                case MENU_HIRAGANA: meg4_pushkey("\005"); break;
            }
            menu_active = last = 0;
        }
        return 1;
    }
    return 0;
}

/**
 * View
 */
void menu_view(uint32_t *dst, int dw, int dh, int dp)
{
    char tmp[128];
    uint32_t c, s;
    int a, i, j, t, y, y2 = (MENU_INPUTS - MENU_ABOUT) * 10 + 13;
    int clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    /* menubar icons */
    if(menu_active > 0) meg4_box(dst, dw, dh, dp, 2, 0, 16, 12, theme[THEME_ACTIVE_D], theme[THEME_ACTIVE_BG], 0, 0, 0, 0, 0, 0);
    meg4_blit(dst, 2, menu_active > 0 ? 2 : 1, dp, 16, 8, meg4_edicons.buf, 32, 48, meg4_edicons.w * 4, 1);
    t = meg4.mode == MEG4_MODE_LOAD ? MENU_LIST - MENU_HELP : meg4.mode - MEG4_MODE_HELP;
    for(y = 22, i = MEG4_MODE_CODE; i < MEG4_NUM_MODE; i++, y += 12) {
        if(py < 12 && px >= y  && px < y + 12) t = i - MEG4_MODE_HELP;
        if(meg4.mode == i || (i == MEG4_MODE_CODE && meg4.mode >= MEG4_MODE_DEBUG && meg4.mode <= i)) {
            meg4_box(dst, dw, dh, dp, y - 2, 0, 12, 12, theme[THEME_ACTIVE_D], theme[THEME_ACTIVE_BG], 0, 0, 0, 0, 0, 0); j = 3;
        } else j = 2;
        meg4_blit(dst, y, j, dp, 8, 8, meg4_edicons.buf, 48 + ((i - MEG4_MODE_CODE) << 3), 48, meg4_edicons.w * 4, 1);
    }
    menu_stat = 0;
    switch(meg4.mode) {
        case MEG4_MODE_HELP: help_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_DEBUG: debug_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_VISUAL: visual_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_CODE: code_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_SPRITE: sprite_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_MAP: map_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_FONT: font_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_SOUND: sound_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_MUSIC: music_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_OVERLAY: overlay_menu(dst, dw, dh, dp); break;
        case MEG4_MODE_LOAD: load_menu(dst, dw, dh, dp); break;
    }
    i = menu_stat ? menu_stat - MENU_HELP : t;
    meg4_text(dst, 320 - titlew[i], 1, dp, theme[THEME_MENU_FG], htole32(0x3f000000), 1, meg4_font, lang[i + MENU_HELP]);
    /* popup menu */
    menu_sel = -1;
    if(menu_active > 0) {
        if(menu_active == 2 && (px < menu_width + 4 || px > menu_width + menu_subwidth + 8 ||
          py < y2 || py > y2 + (MENU_HIRAGANA - MENU_CODEPOINT + 1) * 10 + 2)) menu_active = 1;
        meg4_box(dst, dw, dh, dp, 1, 10, menu_width + 6, (MENU_HELP - MENU_ABOUT + 1) * 10 + 4,
            theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], htole32(0x3f000000), 0, 0, 0, 0);
        for(y = 13, i = MENU_ABOUT; i <= MENU_HELP; i++, y += 10) {
            a = (!meg4_pro && i == MENU_EXPWASM)
#ifdef EMBED
                || i == MENU_IMPORT || i == MENU_EXPZIP
#endif
            ;
            if((menu_active == 2 && i == MENU_INPUTS) ||
              (px >= 2 && px < menu_width + 6 && py >= y && py < y + 10)) {
                menu_sel = i;
                if(menu_active == 1 && i == MENU_INPUTS) menu_active = 2;
                if(a) {
                    meg4_box(dst, dw, dh, dp, 3, y - 1, menu_width + 2, 10, 0, theme[THEME_ACTIVE_BG], 0, 0, 0, 0, 0, 0);
                    c = theme[THEME_MENU_BG]; s = 0;
                } else {
                    meg4_box(dst, dw, dh, dp, 3, y - 1, menu_width + 2, 10, 0, theme[THEME_SEL_BG], 0, 0, 0, 0, 0, 0);
                    c = theme[THEME_SEL_FG]; s = htole32(0x3f000000);
                }
            }
            else if(a) { c = theme[THEME_ACTIVE_BG]; s = 0; }
            else { c = theme[THEME_MENU_FG]; s = 0; }
            meg4_blit(dst, 4, y, dp, 8, 8, meg4_edicons.buf, 48 + ((i - MENU_ABOUT) << 3), 40, meg4_edicons.w * 4, 1);
            meg4_text(dst, 14, y, dp, c, s, 1, meg4_font, lang[i]);
            switch(i) {
                case MENU_RUN:     shortcut_view(dst, menu_width + 4, y, dp, 0); break;
                case MENU_SAVE:    shortcut_view(dst, menu_width + 4, y, dp, 1); break;
                case MENU_IMPORT:  shortcut_view(dst, menu_width + 4, y, dp, 2); break;
                case MENU_EXPWASM: if(!meg4_pro) meg4_text(dst, menu_pro, y, dp, c, s, 1, meg4_font, "(PRO)"); break;
                case MENU_FULLSCR: shortcut_view(dst, menu_width + 4, y, dp, 3); break;
                case MENU_INPUTS:  meg4_text(dst, menu_width + 4 - subw, y, dp, c, s, 1, meg4_font, ">"); break;
                case MENU_HELP:    shortcut_view(dst, menu_width + 4, y, dp, 4); break;
            }
        }
        if(menu_active == 2) {
            meg4_box(dst, dw, dh, dp, menu_width + 4, y2 - 3, menu_subwidth + 4, (MENU_HIRAGANA - MENU_CODEPOINT + 1) * 10 + 4,
                theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], htole32(0x3f000000), 0, 0, 0, 0);
            for(y = y2, i = MENU_CODEPOINT; i <= MENU_HIRAGANA; i++, y += 10) {
                if(px >= menu_width + 4 && px < menu_width + menu_subwidth + 8 &&
                  py >= y && py < y + 10) {
                    menu_sel = i;
                    meg4_box(dst, dw, dh, dp, menu_width + 6, y - 1, menu_subwidth, 10, 0, theme[THEME_SEL_BG], 0, 0, 0, 0, 0, 0);
                    c = theme[THEME_SEL_FG]; s = htole32(0x3f000000);
                } else { c = theme[THEME_MENU_FG]; s = 0; }
                meg4_text(dst, menu_width + 7, y, dp, c, s, 1, meg4_font, lang[i]);
                switch(i) {
                    case MENU_CODEPOINT: shortcut_view(dst, menu_width + menu_subwidth + 5, y, dp, 5); break;
                    case MENU_COMPOSE:   shortcut_view(dst, menu_width + menu_subwidth + 5, y, dp, 6); break;
                    case MENU_ICONEMOJI: shortcut_view(dst, menu_width + menu_subwidth + 5, y, dp, 7); break;
                    case MENU_KATAKANA:  shortcut_view(dst, menu_width + menu_subwidth + 5, y, dp, 8); break;
                    case MENU_HIRAGANA:  shortcut_view(dst, menu_width + menu_subwidth + 5, y, dp, 9); break;
                }
            }
        }
    } else
    /* about window */
    if(menu_active == -1) {
        sprintf(tmp, "MEG-4 %s%s", meg4_pro ? "PRO " : "", lang[LOAD_FANCON]);
        i = meg4_width(meg4_font, 2, tmp, NULL);
        meg4_box(dst, dw, dh, dp, 160, 120, i > 320 ? i + 20 : 320, 160,
            theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], htole32(0x3f000000), 0, 0, 0, 0);
        meg4_blit(dst, 268, 130, dp, 104, 64, meg4_icons.buf, 0, 0, meg4_icons.w * 4, 1);
        y = 200;
        meg4_text(dst, (640 - i) / 2, y, dp, theme[THEME_SEL_FG], htole32(0x3f000000), 2, meg4_font, tmp);
        y += 20;
        sprintf(tmp, "%s v%u.%u.%u", lang[LOAD_FW], meg4.mmio.fwver[0], meg4.mmio.fwver[1], meg4.mmio.fwver[2]);
        meg4_text(dst, (640 - meg4_width(meg4_font, 2, tmp, NULL)) / 2, y, dp, theme[THEME_SEL_FG], htole32(0x3f000000), 2, meg4_font, tmp);
        if(meg4_pro && meg4_author[0]) {
            meg4_text(dst, (640 - meg4_width(meg4_font, 1, lang[MENU_REGTO], NULL)) / 2, y + 32, dp, theme[THEME_ACTIVE_BG], 0,
                1, meg4_font, lang[MENU_REGTO]);
            meg4_text(dst, (640 - meg4_width(meg4_font, 1, meg4_author, NULL)) / 2, y + 40, dp, theme[THEME_MENU_FG], htole32(0x3f000000),
                1, meg4_font, meg4_author);
        } else {
            y += 28;
            for(i = 0; i < 3; i++, y += 10)
                meg4_text(dst, 164, y, dp, theme[THEME_ACTIVE_BG], 0, 1, meg4_font, (char*)copyright[i]);
        }
    } else
    /* export zip window */
    if(menu_active == -2) {
        meg4_box(dst, dw, dh, dp, (632 - zipw) / 2, 176, zipw + 8, 48,
            theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], htole32(0x3f000000), 0, 0, 0, 0);
        meg4_text(dst, (640 - zipt) / 2, 177, dp, theme[THEME_SEL_FG], htole32(0x3f000000),
            2, meg4_font, lang[MENU_EXPZIP]);
        meg4_box(dst, dw, dh, dp, 258, 196, 124, 12,
            theme[THEME_MENU_D], theme[THEME_INP_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
        if(!textinp_buf)
            meg4_text(dst, 260, 198, dp, theme[THEME_INP_FG], 0, 1, meg4_font, meg4_title);
        meg4_chk(dst, dw, dh, dp, 258, 212, theme[THEME_MENU_D], theme[THEME_MENU_L], zipbin);
        meg4_text(dst, 266, 211, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, lang[MENU_BINARY]);
        if(px >= 364 - zipo && px < 380 &&
          py >= 209 && py < 221 && clk) {
            meg4_box(dst, dw, dh, dp, 364 - zipo, 209, zipo + 16, 12, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
            meg4_text(dst, 372 - zipo, 211, dp, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[MENU_OK]);
        } else {
            meg4_box(dst, dw, dh, dp, 364 - zipo, 209, zipo + 16, 12, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
            meg4_text(dst, 372 - zipo, 211, dp, theme[THEME_BTN_FG], htole32(0x3f000000), 1, meg4_font, lang[MENU_OK]);
        }
    }
    /* scrollbar */
    if(!menu_scrmax || menu_scrmax <= menu_scrhgt) {
        menu_scrbtn = menu_scrsiz = 0;
        meg4_box(dst, dw, dh, dp, 632, 12, 8, dh - 12, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], 0, 0, 0, 0, 0);
    } else {
        menu_scrbtn = 8 + (dh - 20) * menu_scrhgt / menu_scrmax;
        menu_scrsiz = dh - 12 - menu_scrbtn;
        menu_scrpos = 12 + menu_scrsiz * menu_scroll / (menu_scrmax - menu_scrhgt);
        meg4_box(dst, dw, dh, dp, 632, 12, 8, dh - 12, theme[THEME_INP_BG], theme[THEME_INP_BG], theme[THEME_INP_BG], 0, 0, 0, 0, 0);
        if(menu_scrofs)
            meg4_box(dst, dw, dh, dp, 632, menu_scrpos, 8, menu_scrbtn, theme[THEME_MENU_D], theme[THEME_MENU_BG], theme[THEME_MENU_L], 0, 0, 0, 0, 0);
        else
            meg4_box(dst, dw, dh, dp, 632, menu_scrpos, 8, menu_scrbtn, theme[THEME_MENU_L], theme[THEME_MENU_BG], theme[THEME_MENU_D], 0, 0, 0, 0, 0);
    }
    if(menu_active) meg4.mmio.ptrspr = MEG4_PTR_NORM;
}

/**
 * Display a menu icon
 */
void menu_icon(uint32_t *dst, int dw, int dh, int dp, int dx, int sx, int sy, int ctrl, int key, int stat)
{
    int i = (ctrl ? (meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL)) : 1) && (key ? meg4_api_getkey(key) : 0);
    if(le16toh(meg4.mmio.ptrx) >= dx && le16toh(meg4.mmio.ptrx) < dx + 12 && le16toh(meg4.mmio.ptry) < 12) {
        menu_stat = stat;
        if((le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L)) i = 1;
    }
    if(i) {
        meg4_box(dst, dw, dh, dp, dx, 0, 12, 12, theme[THEME_ACTIVE_D], theme[THEME_ACTIVE_BG], 0, 0, 0, 0, 0, 0);
        meg4_blit(dst, dx + 2, 3, dp, 8, 8, meg4_edicons.buf, sx, sy, meg4_edicons.w * 4, 1);
    } else
        meg4_blit(dst, dx + 2, 2, dp, 8, 8, meg4_edicons.buf, sx, sy, meg4_edicons.w * 4, 1);
}
