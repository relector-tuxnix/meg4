/*
 * meg4/editors/save.c
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
 * @brief Save floppy window
 *
 */

#include "editors.h"

static meg4_pixbuf_t floppyimg = { 0 };
static uint32_t *origimg = NULL;
static int anim = 0, dir = 0;

/**
 * Save the original image and write title to the floppy's label
 */
void save_addlabel(void)
{
    int x0, y0, x1, y1;
    if(floppyimg.buf) {
        origimg = (uint32_t*)malloc(floppyimg.w * 8 * 4);
        if(origimg) {
            textinp_free();
            memcpy(origimg, floppyimg.buf + 202 * floppyimg.w, floppyimg.w * 8 * 4);
            x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(25);  x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(floppyimg.w - 25);
            y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(202); y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(210);
            meg4_text(floppyimg.buf, 25, 202, floppyimg.w * 4, htole32(LABEL_COLOR), 0, 1, meg4_font, meg4_title);
            meg4.mmio.cropx0 = x0; meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
        }
    }
}

/**
 * Initialize saver
 */
void save_init(void)
{
    int i;

    save_free();
    floppyimg.buf = (uint32_t*)stbi_load_from_memory(binary_floppy_png, sizeof(binary_floppy_png), &floppyimg.w, &floppyimg.h, &i, 4);
    if(floppyimg.buf) {
        meg4_screenshot(floppyimg.buf, 25, 96, floppyimg.w * 4);
        cpu_getlang();
        if(meg4.code_type * 8 < meg4_edicons.w)
            meg4_blit(floppyimg.buf, floppyimg.w - 35, 98, floppyimg.w * 4, 8, 8, meg4_edicons.buf, meg4.code_type * 8, 64,
                meg4_edicons.w * 4, 1);
        save_addlabel();
        anim = 60; dir = -1;
    }
}

/**
 * Free resources
 */
void save_free(void)
{
    if(floppyimg.buf) { free(floppyimg.buf); memset(&floppyimg, 0, sizeof(floppyimg)); }
    if(origimg) { free(origimg); origimg = NULL; }
}

/**
 * Controller
 */
int save_ctrl(void)
{
    char fn[256];
    uint8_t *buf;
    int len = 0;

    if(dir) return 1;
    if(textinp_key == htole32('\n') && meg4_title[0]) {
        buf = meg4_save(&len);
        if(buf) {
            strcpy(fn, meg4_title);
            strcat(fn, ".png");
            if(!main_savefile(fn, buf, len)) {
                main_log(1, "unable to save");
                meg4.mmio.ptrspr = MEG4_PTR_ERR;
            } else
                main_log(1, "save successful");
            free(buf);
        } else {
            main_log(1, "unable to serialize floppy");
            meg4.mmio.ptrspr = MEG4_PTR_ERR;
        }
    }
    if(textinp_key == htole32(27) || textinp_key == htole32('\n')) {
        save_addlabel(); dir = 1;
    }
    return 1;
}

/**
 * View
 */
void save_view(void)
{
    int y1, y2, w, h, x = (320-floppyimg.w)/2, y = (186-floppyimg.h)/2;

    load_bg();
    if(floppyimg.buf) {
        if(!dir)
            meg4_blit(meg4.valt, x, y, 640 * 4, floppyimg.w, floppyimg.h, floppyimg.buf, 0, 0, floppyimg.w * 4, 1);
        else {
            anim += dir;
            if(anim < 1) {
                w = floppyimg.w; h = floppyimg.h; y1 = y2 = anim = dir = 0;
                if(origimg) {
                    memcpy(floppyimg.buf + 202 * floppyimg.w, origimg, floppyimg.w * 8 * 4);
                    free(origimg); origimg = NULL;
                }
                textinp_init(x + 25, y + 202, floppyimg.w - 25, htole32(LABEL_COLOR), 0, 1, meg4_font,
                    0, TEXTINP_NAME, meg4_title, sizeof(meg4_title));
            } else
            if(anim <= 30) {
                w = (anim * 132 + (30 - anim) * floppyimg.w) / 30; h = (anim * 48 + (30 - anim) * floppyimg.h) / 30;
                y1 = (anim * 108) / 30; y2 = 0;
            } else { w = 132; h = 48; y1 = 108; y2 = anim < 50 ? ((anim - 30) * floppyimg.h) / 20 : floppyimg.h; }
            meg4_blitd(meg4.valt, x, y + y1, 640 * 4, w, floppyimg.w, h, floppyimg.buf, 0, y2, floppyimg.w, floppyimg.h, floppyimg.w * 4, 0);
            if(anim > 60)
                meg4_switchmode(-1);
        }
    }
}
