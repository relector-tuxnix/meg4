/*
 * meg4/editors/sound.c
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
 * @brief Sound effects editor
 *
 */

#include <stdio.h>
#include "editors.h"

static int idx = 0, wave = 0, loop = 0, playing = 0, widx = -1, unaw, expw;
static uint8_t sndcb[4], wavecb[sizeof(meg4.waveforms[0])] = { 0 }, lastwave = 0, *defwaves[32];
static int8_t minwave[512], maxwave[512];

/**
 * Initialize sfx editor
 */
void sound_init(void)
{
    uint8_t *ptr;
    int i;

    unaw = meg4_width(meg4_font, 1, lang[MENU_UNAVAIL], NULL);
    expw = meg4_width(meg4_font, 1, lang[MENU_EXPORT], NULL);
    last = 0;
    menu_scroll = idx * 10;
    memset(sndcb, 0, sizeof(sndcb));
    memset(wavecb, 0, sizeof(wavecb));
    wave = playing = loop = lastwave = 0;
    memset(defwaves, 0, sizeof(defwaves));
    for(ptr = meg4_defwaves, i = 0; ptr && (ptr[0] || ptr[1]) && i < 32; i++, ptr += 8 + ((ptr[1]<<8)|ptr[0])) defwaves[i] = ptr;
}

/**
 * Free resources
 */
void sound_free(void)
{
}

/**
 * Check if sound effect cursor is on screen
 */
void sound_chkscroll(void)
{
    int i;
    idx &= 0x3f;
    i = idx * 10;
    if(menu_scroll > i) menu_scroll = i;
    if(menu_scroll + 27 * 10 < i) menu_scroll = i - 27 * 10;
    if(menu_scroll > menu_scrmax - menu_scrhgt) menu_scroll = menu_scrmax - menu_scrhgt;
    if(menu_scroll < 0) menu_scroll = 0;
}

/**
 * A new wave was added (pun intended :-))
 */
void sound_addwave(int wave)
{
    int i;
    if(wave < 1 || wave > 31) return;
    /* find a sond effect that uses this wave */
    for(i = 0; i < 64 && meg4.mmio.sounds[(i << 2) + 1] != wave; i++);
    if(i >= 64) {
        /* none, so find an empty slot and add a sound effect with this wave */
        for(i = 0; i < 64 && meg4.mmio.sounds[(i << 2)]; i++);
        if(i < 64) {
            idx = i; meg4.mmio.sounds[(i << 2)] = MEG4_NOTE_C_4; meg4.mmio.sounds[(i << 2) + 1] = wave;
        }
    } else idx = i;
    sound_chkscroll();
}

/**
 * Controller
 */
int sound_ctrl(void)
{
#ifndef EMBED
    char fn[16];
    uint8_t riffhdr[44] = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
        16, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 8, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0 }, *buf;
#endif
    uint8_t *snd = &meg4.mmio.sounds[(idx << 2)], note[4] = { MEG4_NOTE_C_4, 0, 0, 0 };
    int8_t tmp, *ptr = meg4.mmio.sounds[(idx << 2) + 1] ? (int8_t*)&meg4.waveforms[(uint32_t)meg4.mmio.sounds[(idx << 2) + 1] - 1][0] : NULL;
    uint32_t key;
    float f;
    int i, j, l, clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(!wave)
        toolbox_notectrl(snd, 1);

    if(last && clk) {
        /* waveform editor area */
        if(wave && ptr) {
            if(loop && px >= 11 && px <= 11+512 && py >= 23 && py <= 23+256) {
                i = (px - 11) * (((uint8_t)ptr[1]<<8)|(uint8_t)ptr[0]) / 512;
                j = ((uint8_t)ptr[3]<<8)|(uint8_t)ptr[2];
                if(loop == 2) { loop = 1; ptr[2] = i & 0xff; ptr[3] = (i >> 8) & 0xff; ptr[4] = 1; ptr[5] = 0; } else
                if(j <= i) { i -= (j - 1); ptr[4] = i & 0xff; ptr[5] = (i >> 8) & 0xff; }
            } else
            if(wave && ptr && px >= 11 && px < 11+512 && py >= 23 && py < 23+256) {
                l = ((uint8_t)ptr[1]<<8)|(uint8_t)ptr[0];
                i = (px - 11) * l / 512; widx = l <= 512 ? i : -1;
                i = 128 - (py - 23);
                if(widx != -1 && i >= -128 && i < 128) { lastwave = 0; ptr[8 + widx] = i; }
            }
        }
    } else
    if(last && !clk) {
        widx = -1;
        /* menubar click release */
        if(py < 12) {
            if(px >= 576 && px < 588) goto cut; else
            if(px >= 588 && px < 600) goto copy; else
            if(px >= 600 && px < 612) goto paste; else
            if(px >= 618 && px < 630) goto del;
        } else
        /* sound effect table clicked */
        if(px >= 556 && px < 628 && py >= 23 && py < 299) {
            wave = playing = loop = 0; idx = (menu_scroll / 10) + (py - 23 + (menu_scroll % 10)) / 10;
        } else
        /* waveform editor area */
        if(loop) { loop = 0; if(ptr && !ptr[2] && !ptr[3] && ptr[4] == 1 && !ptr[5]) ptr[4] = 0; } else
        /* wave toolbar */
        if(px < 522 && py >= 288 && py < 302) {
            if(px >= 10 && px < 22) { playing = loop = 0; if(ptr) wave ^= 1; } else
            if(!wave && px >= 180 && px < 191 + 31 * 11) {
                /* wave selector buttons */
                snd[1] = 1 + (px - 180) / 11;
                if(!snd[0]) snd[0] = MEG4_NOTE_C_4;
            } else
            if(wave && ptr) {
                /* wave editor toolbar */
                l = ((uint8_t)ptr[1]<<8)|(uint8_t)ptr[0]; lastwave = 0;
                /* finetune */
                if(px >= 36 && px < 48) {
                    if(!(ptr[6] & 0xf)) ptr[6] |= 0xf; else
                    if((ptr[6] & 0xf) != 8) ptr[6]--;
                } else
                if(px >= 60 && px < 72) {
                    if((ptr[6] & 0xf) == 0xf) ptr[6] = 0; else
                    if((ptr[6] & 0xf) != 7) ptr[6]++;
                } else
                /* volume */
                if(px >= 88 && px < 100) { if((uint8_t)ptr[7] > 0) ptr[7]--; } else
                if(px >= 116 && px < 128) { if((uint8_t)ptr[7] < 64) ptr[7]++; } else
                /* set loop */
                if(px >= 136 && px < 148) { loop = 2; ptr[2] = ptr[3] = ptr[4] = ptr[5] = 0; } else
                /* generate default waves */
                if(px >= 180 && px < 192) { dsp_genwave(meg4.mmio.sounds[(idx << 2) + 1], 0); } else
                if(px >= 196 && px < 208) { dsp_genwave(meg4.mmio.sounds[(idx << 2) + 1], 1); } else
                if(px >= 212 && px < 226) { dsp_genwave(meg4.mmio.sounds[(idx << 2) + 1], 2); } else
                if(px >= 228 && px < 240) { dsp_genwave(meg4.mmio.sounds[(idx << 2) + 1], 3); } else
                if(px >= 244 && px < 256) {
                    memset(ptr, 0, sizeof(meg4.waveforms[0])); i = meg4.mmio.sounds[(idx << 2) + 1] - 1;
                    if(i >= 0 && i < 32 && defwaves[i])
                        memcpy(ptr, defwaves[i], 8 + ((defwaves[i][1]<<8)|defwaves[i][0]));
                } else
                if(px >= 260 && px < 272) { memset(ptr, 0, sizeof(meg4.waveforms[0])); } else
                /* sample length */
                if(px >= 282 && px < 294) { if(l > 0) { if(!ptr[0]) { ptr[1]--; } ptr[0]--; } } else
                if(px >= 298 && px < 310) {
                    if(l < (int)sizeof(meg4.waveforms[0]) - 8) { if(!l) { ptr[7] = 64; } ptr[8 + l] = 0; if((uint8_t)ptr[0] == 0xff) { ptr[1]++; } ptr[0]++; }
                } else
                /* play button */
                if(px >= 444 && px < 456) {
                    playing ^= 1; note[1] = snd[1];
                    meg4_playnote(note, playing * 255);
                } else
                /* export button */
                if(px >= 470 && px < 474 + expw) {
#ifndef EMBED
                    buf = (uint8_t*)malloc(sizeof(riffhdr) + l);
                    if(buf) {
                        sprintf(fn, "dsp%02X.wav", meg4.mmio.sounds[(idx << 2) + 1]);
                        memcpy(buf, riffhdr, sizeof(riffhdr));
                        for(i = 0; i < l; i++)
                            buf[sizeof(riffhdr) + i] = ptr[8 + i] + 128;
                        *((uint32_t*)(buf + 4)) = htole32(sizeof(riffhdr) + l - 8);
                        *((uint32_t*)(buf + 40)) = *((uint32_t*)(buf + 28)) = *((uint32_t*)(buf + 24)) = htole32(l);
                        main_savefile(fn, buf, sizeof(riffhdr) + l);
                        free(buf);
                    } else
                        meg4.mmio.ptrspr = MEG4_PTR_ERR;
#endif
                } else
                switch(toolbox_ctrl()) {
                    /* move left */
                    case 0: tmp = ptr[8]; memmove(ptr + 8, ptr + 9, l - 1); ptr[7 + l] = tmp; break;
                    /* enlarge vertically */
                    case 1:
                        for(i = 0; i < l; i++) { f = (float)ptr[8 + i] * 1.25; ptr[8 + i] = (int8_t)(f > 127.0 ? 127.0 : (f < -128.0 ? -128.0 : f)); }
                    break;
                    /* shrink vertically */
                    case 2:
                        for(i = 0; i < l; i++) { f = (float)ptr[8 + i] * 0.9; ptr[8 + i] = (int8_t)f; }
                    break;
                    /* move right */
                    case 3: tmp = ptr[7 + l]; memmove(ptr + 9, ptr + 8, l - 1); ptr[8] = tmp; break;
                    /* swap first and second half */
                    case 4:
                        for(i = 0; i < l / 2; i++) { tmp = ptr[8 + i]; ptr[8 + i] = ptr[8 + l/2 + i]; ptr[8 + l/2 + i] = tmp; }
                    break;
                    /* flip vertically */
                    case 5: for(i = 0; i < l; i++) { ptr[8 + i] *= -1; } break;
                    /* flip horizontally */
                    case 6:
                        for(i = 0; i < l/2; i++) { tmp = ptr[8 + i]; ptr[8 + i] = ptr[7 + l - i]; ptr[7 + l - i] = tmp; }
                    break;
                }
            }
        }
    } else {
        key = meg4_api_popkey();
        if(key) {
            if(!memcmp(&key, "Up", 3))   { idx--; sound_chkscroll(); } else
            if(!memcmp(&key, "Down", 4)) { idx++; sound_chkscroll(); } else
            if(!memcmp(&key, "Cut", 4)) {
cut:            if(wave && ptr) {
                    memcpy(wavecb, ptr, sizeof(wavecb));
                    memset(ptr, 0, sizeof(wavecb));
                } else {
                    memcpy(sndcb, snd, 4);
                    memset(snd, 0, 4);
                }
            } else
            if(!memcmp(&key, "Cpy", 4)) {
copy:           if(wave && ptr) {
                    memcpy(wavecb, ptr, sizeof(wavecb));
                } else {
                    memcpy(sndcb, snd, 4);
                }
            } else
            if(!memcmp(&key, "Pst", 4)) {
paste:          if(wave && ptr) {
                    memcpy(ptr, wavecb, sizeof(wavecb));
                } else {
                    memcpy(snd, sndcb, 4);
                }
            } else
            if(!memcmp(&key, "Del", 4)) {
del:            if(wave && ptr) {
                    memset(ptr, 0, sizeof(wavecb));
                } else {
                    memset(snd, 0, 4);
                }
            }
        }
    }
    if(wave && playing && meg4.dalt.ch[4].sample && meg4.dalt.ch[4].increment > 0.0f && meg4.dalt.ch[4].position < 0.0f)
        meg4.dalt.ch[4].position = 0.0f;
    last = clk;
    return 1;
}

/**
 * Menu
 */
void sound_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    /* cut */       menu_icon(dst, dw, dh, dp, 576, 136, 40, 1, MEG4_KEY_X, STAT_CUT);
    /* copy */      menu_icon(dst, dw, dh, dp, 588, 144, 40, 1, MEG4_KEY_C, STAT_COPY);
    /* paste */     menu_icon(dst, dw, dh, dp, 600, 152, 40, 1, MEG4_KEY_V, STAT_PASTE);
    /* delete */    menu_icon(dst, dw, dh, dp, 618, 136, 48, 0, MEG4_KEY_DEL, STAT_ERASE);

    if(!wave) {
        toolbox_stat();
        if(py >= 22 && py < 304 && px >= 555 && px < 629) menu_stat = STAT_SNDSEL;
    }
    if(py >= 288 && py < 301 && px >= 11 && px < 523) {
        menu_stat = 0;
        if(px >= 11 && px < 24) menu_stat = STAT_WED;
        if(wave) {
            if(px >= 28 && px < 61) menu_stat = STAT_TUNE; else
            if(px >= 80 && px < 117) menu_stat = STAT_VOL; else
            if(px >= 138 && px < 180) menu_stat = STAT_LOOP; else
            if(px >= 180 && px < 193) menu_stat = STAT_SINE; else
            if(px >= 196 && px < 209) menu_stat = STAT_TRI; else
            if(px >= 212 && px < 225) menu_stat = STAT_SAW; else
            if(px >= 228 && px < 241) menu_stat = STAT_SQR; else
            if(px >= 244 && px < 257) menu_stat = STAT_SB; else
            if(px >= 260 && px < 273) menu_stat = STAT_ERASE; else
            if(px >= 284 && px < 297) menu_stat = STAT_WSH; else
            if(px >= 298 && px < 311) menu_stat = STAT_WGR; else
            if(px >= 320 && px < 333) menu_stat = STAT_SHL; else
            if(px >= 336 && px < 349) menu_stat = STAT_WUP; else
            if(px >= 352 && px < 365) menu_stat = STAT_WDN; else
            if(px >= 366 && px < 379) menu_stat = STAT_SHR; else
            if(px >= 382 && px < 395) menu_stat = STAT_WSP; else
            if(px >= 398 && px < 411) menu_stat = STAT_FLIPV; else
            if(px >= 414 && px < 427) menu_stat = STAT_FLIPH; else
            if(px >= 444 && px < 457) menu_stat = STAT_WPL;
        } else {
            if(px >= 180 && px < 180 + 31 * 11) menu_stat = STAT_WAVE;
        }
    }
}

/**
 * View
 */
void sound_view(void)
{
    int8_t *ptr = NULL;
    uint32_t fg;
    int i, j, k, y0, y1, s, e, l;
    int clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    char tmp[32];

    /* sounds table */
    menu_scrhgt = 282; menu_scrmax = 640;
    meg4_box(meg4.valt, 640, 388, 2560, 555, 10, 74, 282, theme[THEME_D], theme[THEME_BG], theme[THEME_L],
        0, 0, menu_scroll, 10, 0);
    y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(11); y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(292);
    for(i = 0; i < 29; i++) {
        sprintf(tmp, "%02X", i + (menu_scroll / 10));
        meg4_text(meg4.valt, 554 - meg4_width(meg4_font, 1, tmp, NULL), 12 + i * 10 - (menu_scroll % 10), 2560, theme[THEME_D], theme[THEME_L],
            1, meg4_font, tmp);
        if(!wave && (menu_scroll / 10) + i == idx) {
            meg4_box(meg4.valt, 640, 388, 2560, 556, 11 + i * 10 - (menu_scroll % 10), 72, 10, theme[THEME_SEL_BG],
                theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
            fg = theme[THEME_SEL_FG];
        } else
            fg = theme[THEME_FG];
        toolbox_note(558, 11 + i * 10 - (menu_scroll % 10), 2560, fg, &meg4.mmio.sounds[((menu_scroll / 10) + i) << 2]);
    }
    meg4.mmio.cropy0 = 0; meg4.mmio.cropy1 = htole16(388);

    /* waveform editor */
    meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 512+3, 256+2, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 1, 0);
    if(!meg4.mmio.sounds[(idx << 2) + 1]) {
        meg4_text(meg4.valt, 10 + (514 - unaw) / 2, 130, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
    } else {
        ptr = (int8_t*)&meg4.waveforms[(uint32_t)meg4.mmio.sounds[(idx << 2) + 1] - 1][0];
        l = (((uint8_t)ptr[1]<<8)|(uint8_t)ptr[0]);
        if(l < 1) {
            meg4_text(meg4.valt, 10 + (514 - unaw) / 2, 130, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
        } else {
            /* loop selection display */
            if(ptr[4] || ptr[5]) {
                s = (((uint8_t)ptr[3]<<8)|(uint8_t)ptr[2]) * 512 / l;
                e = (((uint8_t)ptr[5]<<8)|(uint8_t)ptr[4]) * 512 / l;
                meg4_box(meg4.valt, 640, 388, 2560, 12 + s, 11, e - 1, 256, 0, theme[THEME_D], 0, 0, 0, 0, 0, 0);
                meg4_box(meg4.valt, 640, 388, 2560, 11 + s, 11, 1, 256, 0, theme[THEME_L], 0, 0, 0, 0, 0, 0);
                meg4_box(meg4.valt, 640, 388, 2560, 11 + s + e, 11, 1, 256, 0, theme[THEME_L], 0, 0, 0, 0, 0, 0);
            }
            /* generate aggregated wave cache */
            if(meg4.mmio.sounds[(idx << 2) + 1] != lastwave) {
                lastwave = meg4.mmio.sounds[(idx << 2) + 1];
                if(l > 512) {
                    memset(minwave, 127, sizeof(minwave));
                    memset(maxwave, -127, sizeof(maxwave));
                    for(i = 0; i < l; i++) {
                        s = i * 512 / l;
                        if(minwave[s] > ptr[8 + i]) minwave[s] = ptr[8 + i];
                        if(maxwave[s] < ptr[8 + i]) maxwave[s] = ptr[8 + i];
                    }
                } else {
                    for(i = 0; i < 512; i++) { s = i * l / 512; minwave[i] = maxwave[i] = ptr[8 + s]; }
                }
            }
            /* display wave */
            for(i = 0; i < 512; i++)
                if(minwave[i] <= maxwave[i]) {
                    j = (l <= 512 && widx == i * l / 512) || (meg4.dalt.ch[4].master &&
                        meg4.dalt.ch[4].sample == meg4.mmio.sounds[(idx << 2) + 1] &&
                        (l <= 512 ? i * l / 512 : i * 512 / l) == (int)meg4.dalt.ch[4].position);
                    fg = j ? theme[THEME_SEL_BG] : theme[THEME_L]; k = i * l / 512 == (i + 1) * l / 512;
                    if(l > 256 || k) {
                        if(maxwave[i] < 0)
                            meg4_box(meg4.valt, 640, 388, 2560, 12 + i, 11 + 128, 1, 1 - maxwave[i], 0, fg, 0, 0, 0, 0, 0, 0);
                        else
                        if(minwave[i] > 0)
                            meg4_box(meg4.valt, 640, 388, 2560, 12 + i, 12 + 128 - minwave[i], 1, minwave[i], 0, fg, 0, 0, 0, 0, 0, 0);
                    } else
                    if(!k)
                        meg4_box(meg4.valt, 640, 388, 2560, 12 + i, 11, 1, 11 + 256, 0, theme[THEME_BG], 0, 0, 0, 0, 0, 0);
                    meg4_box(meg4.valt, 640, 388, 2560, 12 + i, 11 + 128 - maxwave[i], 1, maxwave[i] - minwave[i] + 1, 0,
                        theme[j ? THEME_SEL_FG : THEME_INP_FG], 0, 0, 0, 0, 0, 0);
                }
        }
    }
    if(!meg4.mmio.sounds[(idx << 2) + 1]) {
        meg4_box(meg4.valt, 640, 388, 2560, 10, 276, 13, 13, theme[THEME_L], theme[THEME_L], theme[THEME_L], 0, 0, 0, 0, 0);
        meg4_char(meg4.valt, 2560, 13, 279, theme[THEME_D], 1, meg4_font, 12);
        fg = theme[THEME_L];
    } else {
        toolbox_btn(10, 276, 12 + wave, 0);
        fg = theme[THEME_FG];
    }
    meg4_char(meg4.valt, 2560, 28, 279, fg, 1, meg4_font, 9);
    sprintf(tmp, "%d", ptr ? ((ptr[6] & 15) > 7 ? ptr[6] - 16 : ptr[6]) : 0);
    meg4_text(meg4.valt, 60 - meg4_width(meg4_font, 1, tmp, NULL), 278, 2560, fg, 0, 1, meg4_font, tmp);
    meg4_char(meg4.valt, 2560, 80, 279, fg, 1, meg4_font, 10);
    sprintf(tmp, "%u", ptr ? (uint8_t)ptr[7] : 0);
    meg4_text(meg4.valt, 116 - meg4_width(meg4_font, 1, tmp, NULL), 278, 2560, fg, 0, 1, meg4_font, tmp);
    sprintf(tmp, "%u:%u", ptr ? ((uint8_t)ptr[3]<<8)|(uint8_t)ptr[2] : 0, ptr ? ((uint8_t)ptr[5]<<8)|(uint8_t)ptr[4] : 0);
    meg4_text(meg4.valt, 150, 278, 2560, fg, 0, 1, meg4_font, tmp);
    if(!wave) {
        meg4_char(meg4.valt, 2560, 138, 279, fg, 1, meg4_font, 11);
        /* display wave selector */
        for(i = 0; i < 31; i++) {
            sprintf(tmp, "%X", i + 1); j = (11 - meg4_width(meg4_font, 1, tmp, NULL)) / 2;
            if(meg4.waveforms[i][0] || meg4.waveforms[i][1]) {
                if((clk && py >= 276 + 12 && py < 276 + 25 &&
                  px >= 180 + i * 11 && px < 191 + i * 11)) {
                    meg4_box(meg4.valt, 640, 388, 2560, 180 + i * 11, 276, 10, 13, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
                    meg4_text(meg4.valt, 180 + j + i * 11, 276 + 3, 2560, theme[THEME_BTN_FG], 0, 1, meg4_font, tmp);
                } else {
                    meg4_box(meg4.valt, 640, 388, 2560, 180 + i * 11, 276, 10, 13, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
                    meg4_text(meg4.valt, 180 + j + i * 11, 276 + 2, 2560, theme[THEME_BTN_FG], 0, 1, meg4_font, tmp);
                }
            } else {
                meg4_box(meg4.valt, 640, 388, 2560, 180 + i * 11, 276, 10, 13, theme[THEME_L], theme[THEME_L], theme[THEME_L], 0, 0, 0, 0, 0);
                meg4_text(meg4.valt, 180 + j + i * 11, 276 + 2, 2560, theme[THEME_D], 0, 1, meg4_font, tmp);
            }
        }
    } else {
        /* legnth of wave indicators */
        meg4_text(meg4.valt, 8, 267, 2560, fg, 0, 1, meg4_font, "0");
        sprintf(tmp, "%u", ptr ? ((uint8_t)ptr[1]<<8)|(uint8_t)ptr[0] : 0);
        meg4_text(meg4.valt, 526 - meg4_width(meg4_font, 1, tmp, NULL), 267, 2560, fg, 0, 1, meg4_font, tmp);
        /* display wave editor toolbar */
        toolbox_btn(36, 276, 0x11, 0);
        toolbox_btn(60, 276, 0x10, 0);
        toolbox_btn(88, 276, 0x11, 0);
        toolbox_btn(116, 276, 0x10, 0);
        toolbox_btn(136, 276, 11, loop);
        toolbox_btn(180, 276, 1, 0);
        toolbox_btn(196, 276, 2, 0);
        toolbox_btn(212, 276, 3, 0);
        toolbox_btn(228, 276, 4, 0);
        if(meg4.mmio.sounds[(idx << 2) + 1] && defwaves[meg4.mmio.sounds[(idx << 2) + 1] - 1])
            toolbox_btn(244, 276, 5, 0);
        else
            meg4_box(meg4.valt, 640, 388, 2560, 244, 276, 13, 13, 0, theme[THEME_L], 0, 0, 0, 0, 0, 0);
        toolbox_btn(260, 276, 127, 0);
        toolbox_btn(284, 276, 6, 0);
        toolbox_btn(298, 276, 7, 0);
        toolbox_view(320, 276, -1);
        if((clk && py >= 276 + 12 && py < 276 + 25 &&
          px >= 444 && px < 444 + 13)) {
            meg4_box(meg4.valt, 640, 388, 2560, 444, 276, 13, 13, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
            meg4_blit(meg4.valt, 444 + 3, 276 + 4, 2560, 8, 8, meg4_edicons.buf, playing ? 96 : 88, 56, meg4_edicons.w * 4, 1);
        } else {
            meg4_box(meg4.valt, 640, 388, 2560, 444, 276, 13, 13, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
            meg4_blit(meg4.valt, 444 + 3, 276 + 3, 2560, 8, 8, meg4_edicons.buf, playing ? 96 : 88, 56, meg4_edicons.w * 4, 1);
        }
#ifndef EMBED
        if((clk && py >= 276 + 12 && py < 276 + 25 &&
          px >= 470 && px < 474 + expw)) {
            meg4_box(meg4.valt, 640, 388, 2560, 470, 276, 4 + expw, 13, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
            meg4_text(meg4.valt, 472, 276 + 3, 2560, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[MENU_EXPORT]);
        } else {
            meg4_box(meg4.valt, 640, 388, 2560, 470, 276, 4 + expw, 13, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
            meg4_text(meg4.valt, 472, 276 + 2, 2560, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[MENU_EXPORT]);
        }
#endif
    }

    /* note editor */
    if(!wave)
        toolbox_noteview(10, 300, 2560, -1, &meg4.mmio.sounds[idx << 2]);
    else
        meg4_text(meg4.valt, (640 - unaw) / 2, 332, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
    meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
}
