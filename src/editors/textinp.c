/*
 * meg4/editors/textinp.c
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
 * @brief Non-code editor text input
 *
 */

#include "editors.h"

char *textinp_cur = NULL, *textinp_buf = NULL;
uint32_t textinp_key = 0;
static char *textinp_end = NULL, textinp_save[256];
static uint32_t textinp_color, textinp_shadow;
static uint8_t *textinp_font;
static int textinp_dx, textinp_dy, textinp_dw, textinp_ft, textinp_scr, textinp_type, textinp_maxlen;

/**
 * Initialize text input
 */
void textinp_init(int x, int y, int w, uint32_t color, uint32_t shadow, int ft, uint8_t *font, int scr, int type, char *str, int maxlen)
{
    textinp_free();
    if(x < 0 || y < 0 || w < 1 || !color || !str || maxlen < 1) return;
    textinp_dx = x;
    textinp_dy = y;
    textinp_dw = w;
    textinp_color = color;
    textinp_shadow = shadow;
    textinp_ft = !ft ? 1 : ft;
    textinp_font = font ? font : meg4_font;
    textinp_scr = scr;
    textinp_type = type;
    textinp_maxlen = maxlen > (int)sizeof(textinp_save) ? (int)sizeof(textinp_save) : maxlen;
    if(type == TEXTINP_NUM && str[0] == '0' && str[1] == 0) str[0] = 0;
    textinp_buf = str;
    textinp_end = textinp_cur = str + strlen(str);
    memcpy(textinp_save, str, textinp_maxlen);
    main_osk_show();
}

/**
 * Free resources
 */
void textinp_free(void)
{
    if(textinp_buf) main_osk_hide();
    textinp_cur = textinp_buf = textinp_end = NULL;
}

/**
 * Add a key
 */
void textinp_addkey(uint32_t key)
{
    unsigned int k = meg4_api_lenkey(key);
    switch(textinp_type) {
        case TEXTINP_NAME:
            if((key == htole32(' ') && textinp_cur == textinp_buf) || key == htole32('/') ||
                key == htole32('\\') || key == htole32('|') || key == htole32('?') ||
                key == htole32('*') || key == htole32('\"')) break;
addkey:     if((int)(textinp_end - textinp_buf + k) < textinp_maxlen) {
                memmove(textinp_cur + k, textinp_cur, textinp_end - textinp_cur + 1);
                memcpy(textinp_cur, &key, k);
                textinp_cur += k; textinp_end += k;
            }
        break;
        case TEXTINP_STR:
            if(key == htole32(' ') && textinp_cur == textinp_buf) break;
            goto addkey;
        case TEXTINP_NUM:
            if(key < htole32('0') || key > htole32('9') ||
                (key == htole32('0') && textinp_buf == textinp_cur)) break;
            goto addkey;
        case TEXTINP_HEX:
            if(key >= htole32('a') && key <= htole32('f')) key -= 'a' - 'A';
            if((key < htole32('0') || key > htole32('9')) && (key < htole32('A') ||
                key > htole32('F'))) break;
            goto addkey;
    }
}

/**
 * Controller
 */
int textinp_ctrl(void)
{
    char *s, *cb;
    uint32_t k;

    textinp_key = 0;
    if(!textinp_buf || !textinp_cur) return 0;
    if((le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R)) && (le16toh(meg4.mmio.ptrx) < textinp_dx ||
      le16toh(meg4.mmio.ptrx) >= textinp_dx + textinp_dw || le16toh(meg4.mmio.ptry) < textinp_dy ||
      le16toh(meg4.mmio.ptry) >= textinp_dy + textinp_ft * 8)) {
        textinp_free(); return 1;
    }
    textinp_key = meg4_api_popkey();
    if(!textinp_key) return 0;
    if(textinp_key == htole32(27) || textinp_key == htole32('\n')) {
        if(textinp_key == htole32(27)) memcpy(textinp_buf, textinp_save, textinp_maxlen);
        else { for(s = textinp_end; s > textinp_buf && s[-1] == ' '; s--) *s = 0; }
        textinp_free();
        return 0;
    }
    if(meg4_api_speckey(textinp_key)) {
        if(!memcmp(&textinp_key, "Left", 4)) {
            if(textinp_cur > textinp_buf)
                do { textinp_cur--; } while(textinp_cur > textinp_buf && (*textinp_cur & 0xC0) == 0x80);
        } else
        if(!memcmp(&textinp_key, "Rght", 4)) {
            if(textinp_cur < textinp_end)
                do { textinp_cur++; } while(textinp_cur < textinp_end && (*textinp_cur & 0xC0) == 0x80);
        } else
        if(!memcmp(&textinp_key, "Up", 3) || !memcmp(&textinp_key, "Home", 4)) textinp_cur = textinp_buf; else
        if(!memcmp(&textinp_key, "Down", 4) || !memcmp(&textinp_key, "End", 4)) textinp_cur = textinp_end; else
        if(!memcmp(&textinp_key, "Del", 4)) {
            if(textinp_cur < textinp_end) {
                s = textinp_cur; do { s++; } while(s < textinp_end && (*s & 0xC0) == 0x80);
                memcpy(textinp_cur, s, textinp_end - s + 1);
                textinp_end -= s - textinp_cur;
            }
        } else
        if(!memcmp(&textinp_key, "Pst", 4)) {
            cb = main_getclipboard();
            if(cb) {
                for(s = cb; *s; ) {
                    s = meg4_utf8(s, &k);
                    if(!k) break;
                    textinp_addkey(k);
                }
                free(cb);
            }
        } else
        if(textinp_key == htole32(8)) {
            if(textinp_cur > textinp_buf) {
                s = textinp_cur; do { textinp_cur--; } while(textinp_cur > textinp_buf && (*textinp_cur & 0xC0) == 0x80);
                memcpy(textinp_cur, s, textinp_end - s + 1);
                textinp_end -= s - textinp_cur;
            }
        }
    } else {
        textinp_addkey(textinp_key);
    }
    return 1;
}

/**
 * View
 */
void textinp_view(uint32_t *dst, int dp)
{
    int x0, y0, x1, y1, i = 0;

    if(!textinp_buf || !textinp_cur || !dst || dp < 4) return;
    x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(textinp_dx); x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(textinp_dx + textinp_dw);
    y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(textinp_dy); y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(textinp_dy + textinp_ft * 8);
    if(textinp_scr) {
        i = meg4_width(textinp_font, textinp_ft, textinp_buf, textinp_cur);
        if(i > textinp_dw - 4) i -= textinp_dw - 4; else i = 0;
    }
    meg4_text(dst, textinp_dx - i, textinp_dy, dp, textinp_color, textinp_shadow, textinp_ft, textinp_font, textinp_buf);
    meg4.mmio.cropx0 = x0; meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
}

/**
 * Display textinput's cursor
 */
int textinp_cursor(uint32_t *dst, int dp)
{
    /* do not modify meg4.mmio.ptrspr */
    if(meg4.mmio.ptrspr != MEG4_PTR_ERR && textinp_buf && le16toh(meg4.mmio.ptrx) >= textinp_dx &&
      le16toh(meg4.mmio.ptrx) < textinp_dx + textinp_dw && le16toh(meg4.mmio.ptry) >= textinp_dy &&
      le16toh(meg4.mmio.ptry) < textinp_dy + textinp_ft * 8) {
        meg4_blit(dst, le16toh(meg4.mmio.ptrx) - ((MEG4_PTR_TEXT >> 10) & 7), le16toh(meg4.mmio.ptry) - ((MEG4_PTR_TEXT >> 10) & 7),
            dp, 8, 8, meg4_icons.buf, 8, 64, meg4_icons.w * 4, 1);
        return 1;
    }
    return 0;
}
