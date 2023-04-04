/*
 * meg4/editors/music.c
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
 * @brief Music tracks editor
 *
 */

#include <stdio.h>
#include "editors.h"

#define MAXROW ((int)(sizeof(meg4.tracks[0])>>4))

static int track = 0, idx = 0, playing = 0, enabled = 15;
static uint8_t clipboard[4];

/**
 * Initialize tracks editor
 */
void music_init(void)
{
    last = 0;
    menu_scroll = idx * 10;
    memset(clipboard, 0, 4);
    playing = 0;
}

/**
 * Free resources
 */
void music_free(void)
{
}

/**
 * Check if pattern note cursor is on screen
 */
void music_chkscroll(int bottom)
{
    int i;
    if((idx >> 2) >= MAXROW) idx &= 3;
    i = (idx >> 2) * 10;
    if(menu_scroll > i) menu_scroll = i;
    if(menu_scroll + (27 - bottom) * 10 < i) menu_scroll = i - (27 - bottom) * 10;
    if(menu_scroll > menu_scrmax - menu_scrhgt) menu_scroll = menu_scrmax - menu_scrhgt;
    if(menu_scroll < 0) menu_scroll = 0;
}

/**
 * Controller
 */
int music_ctrl(void)
{
    uint32_t key;
    uint8_t *mus = &meg4.tracks[track][idx << 2];
    int j, ret, clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    ret = toolbox_notectrl(mus, 0);
    if(ret) {
        if(ret == -1) { idx += 4; music_chkscroll(0); }
    } else
    if(last && !clk) {
        /* menubar click release */
        if(py < 12) {
            if(px >= 540 && px < 552) goto play; else
            if(px >= 558 && px < 570) goto ins; else
            if(px >= 576 && px < 588) goto cut; else
            if(px >= 588 && px < 600) goto copy; else
            if(px >= 600 && px < 612) goto paste; else
            if(px >= 618 && px < 630) goto del;
        } else
        /* track selector clicked */
        if(px >= 11 && px < 27 && py >= 23 && py < 103) {
            track = (py - 23) / 10; idx = 0;
        } else
        /* channel enable / disable */
        if(px >= 16 && px < 80 && py >= 254 && py < 299) {
            j = (px - 16) / 16; enabled ^= (1 << j);
            meg4.dalt.ch[j].master = (enabled & (1 << j)) && playing ? 255 : 0;
        } else
        /* note patterns */
        if(py >= 23 && py < 303) {
            for(j = 0; j < 4; j++)
                if(px >= (j + 1) * 128 && px < (j + 1) * 128 + 72) {
                    idx = (((menu_scroll / 10) + ((py - 23 + (menu_scroll % 10)) / 10)) << 2) + j;
                    break;
                }
        }
    } else
    if(!last && !clk) {
        key = meg4_api_popkey();
        if(key) {
            if(!memcmp(&key, " ", 2)) {
play:           playing ^= 1;
                if(!playing) {
                    meg4_api_music(track, idx >> 2, 0);
                } else {
                    meg4_api_music(track, idx >> 2, 255);
                    for(j = 0; j < 4; j++) if(!(enabled & (1 << j))) meg4.dalt.ch[j].master = 0;
                }
            } else
            if(!memcmp(&key, "PgUp", 4)) { track = (track - 1) & 7; idx = 0; music_chkscroll(0); } else
            if(!memcmp(&key, "PgDn", 4)) { track = (track + 1) & 7; idx = 0; music_chkscroll(0); } else
            if(!memcmp(&key, "Up", 3))   { if(idx >> 2) { idx -= 4; } else { idx = ((MAXROW - 1) << 2) | (idx & 3); } music_chkscroll(0); } else
            if(!memcmp(&key, "Down", 4)) { if((idx >> 2) < MAXROW) { idx += 4; } else { idx &= 3; }  music_chkscroll(0); } else
            if(!memcmp(&key, "Left", 4)) { if(idx & 3) { idx--; } else { idx = (idx & ~3) | 3; } music_chkscroll(0); } else
            if(!memcmp(&key, "Rght", 4)) { if((idx & 3) < 3) { idx++; } else { idx = (idx & ~3); } music_chkscroll(0); } else
            if(!memcmp(&key, "Home", 4)) { idx &= 3; music_chkscroll(0); } else
            if(!memcmp(&key, "End", 4))  { idx = ((MAXROW - 1) << 2) | (idx & 3); music_chkscroll(0); } else
            if(!memcmp(&key, "Cut", 4)) {
cut:            memcpy(clipboard, mus, 4);
                memset(mus, 0, 4);
            } else
            if(!memcmp(&key, "Cpy", 4)) {
copy:           memcpy(clipboard, mus, 4);
            } else
            if(!memcmp(&key, "Pst", 4)) {
paste:          memcpy(mus, clipboard, 4);
            } else
            if(!memcmp(&key, "Ins", 4)) {
ins:            j = (idx & ~3) << 2;
                memmove(&meg4.tracks[track][j + 16], &meg4.tracks[track][j], sizeof(meg4.tracks[0]) - 16 - j);
                memset(&meg4.tracks[track][j], 0, 16);
            } else
            if(!memcmp(&key, "Del", 4)) {
del:            j = (idx & ~3) << 2;
                memmove(&meg4.tracks[track][j], &meg4.tracks[track][j + 16], sizeof(meg4.tracks[0]) - 16 - j);
                memset(&meg4.tracks[track][sizeof(meg4.tracks[0]) - 16], 0, 16);
            }
        }
    }

    last = clk;
    return 1;
}

/**
 * Menu
 */
void music_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    char tmp[32];

    sprintf(tmp, "%03X (%u)", idx >> 2, idx >> 2);
    meg4_text(dst, 425, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%03X (%u)", idx, idx);
    meg4_text(dst, 485, 2, dp, theme[THEME_MENU_FG], 0, 1, meg4_font, tmp);

    /* play/stop */ menu_icon(dst, dw, dh, dp, 540, playing ? 96 : 88, 56, 0, MEG4_KEY_SPACE, STAT_PLAY);
    /* insert */    menu_icon(dst, dw, dh, dp, 558,  48, 56, 0, MEG4_KEY_INS, STAT_INSERT);
    /* cut */       menu_icon(dst, dw, dh, dp, 576, 136, 40, 1, MEG4_KEY_X, STAT_CUT);
    /* copy */      menu_icon(dst, dw, dh, dp, 588, 144, 40, 1, MEG4_KEY_C, STAT_COPY);
    /* paste */     menu_icon(dst, dw, dh, dp, 600, 152, 40, 1, MEG4_KEY_V, STAT_PASTE);
    /* delete */    menu_icon(dst, dw, dh, dp, 618,  56, 56, 0, MEG4_KEY_DEL, STAT_DELETE);

    toolbox_stat();
    if(py >= 22 && py < 104 && px >= 11 && px < 29) menu_stat = STAT_TRKSEL;
}

/**
 * View
 */
void music_view(void)
{
    uint32_t fg, bg;
    int i, j, y0, y1;
    char tmp[32];

    /* tracks */
    meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 18, 82, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 10, 0);
    for(i = 0; i < 8; i++) {
        sprintf(tmp, "%02X", i);
        if(track == i) {
            meg4_box(meg4.valt, 640, 388, 2560, 11, 11 + i * 10, 16, 10, theme[THEME_SEL_BG],
                theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            fg = theme[THEME_SEL_FG];
        } else
            fg = theme[THEME_FG];
        meg4_text(meg4.valt, 25 - meg4_width(meg4_font, 1, tmp, NULL), 12 + i * 10, 2560, fg, 0, 1, meg4_font, tmp);
    }

    /* dump DSP status registers */
    meg4_box(meg4.valt, 640, 388, 2560, 10, 108, 73, 182, theme[THEME_L], 0, theme[THEME_D], 0, 0, 0, 0, 0);
    sprintf(tmp, "%u", meg4.dalt.ticks_per_row);
    meg4_text(meg4.valt, 14, 110, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "ticks");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 110, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.row);
    meg4_text(meg4.valt, 14, 120, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "row");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 120, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.num);
    meg4_text(meg4.valt, 14, 130, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "num");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 130, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    i = idx & 3; sprintf(tmp, "%u", i);
    meg4_text(meg4.valt, 14, 140, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "ch");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 140, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].volume);
    meg4_text(meg4.valt, 14, 150, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "volume");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 150, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].tremolo);
    meg4_text(meg4.valt, 14, 160, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "tremolo");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 160, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].period);
    meg4_text(meg4.valt, 14, 170, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "period");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 170, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].finetune);
    meg4_text(meg4.valt, 14, 180, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "finetune");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 180, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].et);
    meg4_text(meg4.valt, 14, 190, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "et");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 190, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].ep);
    meg4_text(meg4.valt, 14, 200, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "ep");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 200, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u", meg4.dalt.ch[i].sample);
    meg4_text(meg4.valt, 14, 210, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "sample");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 210, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    j = (int)meg4.dalt.ch[i].position; sprintf(tmp, "%u", j < 0 ? 0 : j);
    meg4_text(meg4.valt, 14, 220, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "position");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 220, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);
    j = (int)(meg4.dalt.ch[i].increment*1000); sprintf(tmp, "%u.%03u", j / 1000, j % 1000);
    meg4_text(meg4.valt, 14, 230, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "increment");
    meg4_text(meg4.valt, 80 - meg4_width(meg4_font, 1, tmp, NULL), 230, 2560, theme[THEME_L], 0, 1, meg4_font, tmp);

    /* update the cursor if playing music */
    if(playing) {
        idx = (meg4.dalt.row << 2) | (idx & 3);
        music_chkscroll(14);
    }

    /* patterns table */
    menu_scrhgt = 282; menu_scrmax = MAXROW * 10;
    y0 = meg4.mmio.cropy0; y1 = meg4.mmio.cropy1;
    for(j = 0; j < 4; j++) {
        /* channel's graph */
        if(enabled & (1 << j)) {
            meg4_box(meg4.valt, 640, 388, 2560, 16 + j * 16, 242, 13, 34, theme[THEME_D], 0, theme[THEME_L], 0, 0, 0, 0, 0);
            if(playing) {
                i = meg4.dalt.ch[j].tremolo >> 1;
                meg4_box(meg4.valt, 640, 388, 2560, 17 + j * 16, 275 - i, 11, i, 0, theme[THEME_FG], 0, 0, 0, 0, 0, 0);
            }
            meg4_chk(meg4.valt, 640, 388, 2560, 19 + j * 16, 278, theme[THEME_D], theme[THEME_L], 1);
        } else {
            meg4_box(meg4.valt, 640, 388, 2560, 16 + j * 16, 242, 13, 34, 0, theme[THEME_L], 0, 0, 0, 0, 0, 0);
            meg4_chk(meg4.valt, 640, 388, 2560, 19 + j * 16, 278, theme[THEME_D], theme[THEME_L], 0);
        }
        /* notes */
        meg4_box(meg4.valt, 640, 388, 2560, (j + 1) * 128, 10, 74, 282, theme[THEME_D], 0, theme[THEME_L],
            0, 0, menu_scroll, 10, 0);
        meg4.mmio.cropy0 = htole16(11); meg4.mmio.cropy1 = htole16(292);
        for(i = 0; i < 29; i++) {
            sprintf(tmp, "%02X", i + (menu_scroll / 10));
            meg4_text(meg4.valt, (j + 1) * 128 - 2 - meg4_width(meg4_font, 1, tmp, NULL), 12 + i * 10 - (menu_scroll % 10), 2560, theme[THEME_D], theme[THEME_L],
                1, meg4_font, tmp);
            if((menu_scroll / 10) + i == (idx >> 2)) {
                if(j == (idx & 3)) { bg = theme[THEME_SEL_BG]; fg = theme[THEME_SEL_FG]; }
                else { bg = theme[THEME_INP_BG]; fg = theme[THEME_INP_FG]; }
                meg4_box(meg4.valt, 640, 388, 2560, (j + 1) * 128 + 1, 11 + i * 10 - (menu_scroll % 10), 72, 10, bg, bg, bg, 0, 0, 0, 0, 0);
            } else
                fg = theme[THEME_FG];
            toolbox_note((j + 1) * 128 + 4, 11 + i * 10 - (menu_scroll % 10), 2560, fg,
                &meg4.tracks[track][(((menu_scroll / 10) + i) << 4) + (j << 2)]);
        }
        meg4.mmio.cropy0 = 0; meg4.mmio.cropy1 = htole16(388);
    }

    /* note editor */
    toolbox_noteview(10, 300, 2560, playing ? idx & 3 : -1, &meg4.tracks[track][idx << 2]);
    meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
}
