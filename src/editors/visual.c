/*
 * meg4/editors/visual.c
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
 * @brief Visual scripting code editor
 *
 */

#include "editors.h"

static int unaw = 0;

/**
 * Initialize code editor
 */
void visual_init(void)
{
    unaw = meg4_width(meg4_font, 2, lang[MENU_UNAVAIL], NULL);
}

/**
 * Free resources
 */
void visual_free(void)
{
}

/**
 * Controller
 */
int visual_ctrl(void)
{
    int clk = le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R), px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(last && !clk) {
        if(px >= 614 && px < 626 && py < 12) { meg4_switchmode(MEG4_MODE_CODE); last = 0; return 1; }
        if(px >= 626 && px < 638 && py < 12) { meg4_switchmode(MEG4_MODE_DEBUG); last = 0; return 1; }
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void visual_menu(uint32_t *dst, int dw, int dh, int dp)
{
    /* code */      menu_icon(dst, dw, dh, dp, 614,  48, 48, 1, 0, MENU_CODE);
    /* debugger */  menu_icon(dst, dw, dh, dp, 626, 104, 48, 1, 0, MENU_DEBUG);
}

/**
 * View
 */
void visual_view(void)
{
    if(!meg4.src || meg4.src_len < 1 || memcmp(meg4.src, "#!c\n", 4)) {
        meg4_text(meg4.valt, (640 - unaw) / 2, 190, 2560, theme[THEME_D], theme[THEME_L], 2, meg4_font, lang[MENU_UNAVAIL]);
    } else {
    }
}
