/*
 * meg4/meg4.c
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
 * @brief Core emulator library
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for clock_gettime() */
#include <time.h>
#include "meg4.h"
#include "data.h"

#ifndef NOEDITORS
#include "editors/editors.h"
char *dict[NUMLANGS][NUMTEXTS + 1] = {{
#include "lang/en.h"
},{
#include "lang/hu.h"
},{
#include "lang/de.h"
},{
#include "lang/fr.h"
},{
#include "lang/es.h"
},{
#include "lang/ru.h"
},{
#include "lang/zh.h"
},{
#include "lang/ja.h"
},{
#include "lang/cs.h"
},{
#include "lang/da.h"
},{
#include "lang/el.h"
},{
#include "lang/fi.h"
},{
#include "lang/hr.h"
},{
#include "lang/it.h"
},{
#include "lang/nl.h"
},{
#include "lang/no.h"
},{
#include "lang/pl.h"
},{
#include "lang/pt.h"
},{
#include "lang/ro.h"
},{
#include "lang/sk.h"
},{
#include "lang/sl.h"
},{
#include "lang/sr.h"
},{
#include "lang/sv.h"
},{
#include "lang/cs.h"
},{
#include "lang/tr.h"
},{
#include "lang/uk.h"
}};
char **lang = NULL;
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_ONLY_PNG
#define STBI_WRITE_NO_FAILURE_STRINGS
#define STBI_WRITE_NO_SIMD
#define STBI_WRITE_NO_STDIO
#define STBIW_ASSERT(x)
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STBI_ASSERT(x)
#include "stb_image.h"

#ifdef __WIN32__
#include <windows.h>
static LARGE_INTEGER ticks_per_second, started, now;
static DWORD start, lastt;
#else
static struct timespec started, now;
#endif

const char meg4ver[] = "0.0.1";
const char *copyright[3] = {
    "You should have received a copy of the GNU General Public License",
    "along with this program; if not, write to the Free Software",
    "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA."
};
meg4_pixbuf_t meg4_icons, meg4_screen;
meg4_t meg4;
uint8_t *meg4_font = NULL, *meg4_defwaves = NULL, *meg4_init = NULL;
uint32_t meg4_lasttick = 0, meg4_init_len = 0;
char meg4_title[64], meg4_author[64], meg4_pro = 0, meg4_takescreenshot = 0;
static char meg4_locale[2];
#ifndef NOEDITORS
static int oldmode = MEG4_MODE_GAME, oldx0 = 0, oldx1 = htole16(320), oldy0 = 0, oldy1 = htole16(200), oldcs = MEG4_PTR_NORM;
#else
int comp_lua(char *str, int len);
#endif
#include "floppy.h"

/**
 * Power on the virtual console
 */
void meg4_poweron(char *region)
{
    uint8_t *buf;
    int len = 0;
#ifndef NOEDITORS
    int i;
#endif
    meg4_locale[0] = region[0]; meg4_locale[1] = region[1];
    memset(&meg4, 0 , sizeof(meg4));
    memset(&meg4_title, 0 , sizeof(meg4_title));
    memset(&meg4_author, 0 , sizeof(meg4_author));
    memset(&meg4_icons, 0 , sizeof(meg4_icons));
    srand(time(NULL));
    meg4_reset();
#ifdef NOEDITORS
    /* run the boundled game */
    buf = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char *)binary_game + 4,
        (int)(binary_game[0] | (binary_game[1] << 8) | (binary_game[2] << 16) | (binary_game[3] << 24)), 65536, &len, 1);
    if(buf) { meg4_deserialize(buf, len); free(buf); }
    if(!meg4.code || !meg4.code_len) meg4.mode = MEG4_MODE_GURU;
#if LUA
    /* must call Lua compile even though the code is already in bytecode format */
    else if(meg4.code_type == 0x10) comp_lua((char*)meg4.code, meg4.code_len << 2);
#endif
#else
    /* compiled with editors, load language dictionary and switch to loader mode */
    meg4_defwaves = NULL;
    memset(&meg4_edicons, 0 , sizeof(meg4_edicons));
    for(i = 0; i < NUMLANGS; i++) if(!strcmp(region, dict[i][0])) break;
    if(i >= NUMLANGS) { main_log(1, "no '%s' language dictionary, fallback to 'en'", region); i = 0; }
    lang = &dict[i][1];
    if((buf = main_cfgload("theme.gpl", &len))) { meg4_theme(buf, len); free(buf); }
    pro_init();
    menu_init();
    hl_init();      /* important that highlighter comes first, help initialization depends on it */
    help_init();
    meg4_switchmode(MEG4_MODE_LOAD);
#endif
    meg4_getscreen();
}

/**
 * Free internal resources
 */
static void meg4_free(void)
{
    int i;
#ifdef __WIN32__
    if(meg4.mmio.tick) timeEndPeriod(1000);
#endif
#ifndef NOEDITORS
    if(meg4_defwaves) { free(meg4_defwaves); meg4_defwaves = NULL; }
    if(meg4_edicons.buf) { free(meg4_edicons.buf); memset(&meg4_edicons, 0, sizeof(meg4_edicons)); }
#endif
    if(meg4_init) { free(meg4_init); meg4_init = NULL; } meg4_init_len = 0;
    if(meg4_font) { free(meg4_font); meg4_font = NULL; }
    if(meg4_icons.buf) { free(meg4_icons.buf); memset(&meg4_icons, 0, sizeof(meg4_icons)); }
    if(meg4.code) free(meg4.code);
    if(meg4.src) free(meg4.src);
    if(meg4.fmm) free(meg4.fmm);
    if(meg4.amm) free(meg4.amm);
    for(i = 0; i < 256; i++)
        if(meg4.ovls[i].data) free(meg4.ovls[i].data);
    dsp_free();
    cpu_free();
    memset(&meg4, 0, sizeof(meg4_t));
}

/**
 * Power off the virtual console
 */
void meg4_poweroff(void)
{
#ifndef NOEDITORS
    /* make sure all editor destructors gets called */
    meg4.src_len = 1; meg4_switchmode(MEG4_MODE_GAME);
    code_free();
    help_free();
    hl_free();
#endif
    meg4_free();
}

/**
 * Reset
 */
void meg4_reset(void)
{
    char *v = (char*)meg4ver;
    int c, i, j, k, l, m, n, s;
    uint8_t *font, *ptr, *frg, *buf, *o;

    main_log(1, "reset");
    meg4_free();
    meg4.mmio.fwver[0] = atoi(v); while(*v != '.') { v++; } v++;
    meg4.mmio.fwver[1] = atoi(v); while(*v != '.') { v++; } v++;
    meg4.mmio.fwver[2] = atoi(v);
    meg4_icons.buf = (uint32_t*)stbi_load_from_memory(binary_icons_png, sizeof(binary_icons_png), &meg4_icons.w, &meg4_icons.h, &i, 4);
    meg4.mmio.locale[0] = meg4_locale[0]; meg4.mmio.locale[1] = meg4_locale[1];
    meg4.mmio.cropx1 = htole16(640); meg4.mmio.cropy1 = htole16(400);
    meg4.mmio.mazew = htole16(3); meg4.mmio.mazer = htole16(2);
    meg4.mmio.ptrspr = MEG4_PTR_NORM;
    meg4.mmio.padkeys[0] = MEG4_KEY_LEFT;
    meg4.mmio.padkeys[1] = MEG4_KEY_UP;
    meg4.mmio.padkeys[2] = MEG4_KEY_RIGHT;
    meg4.mmio.padkeys[3] = MEG4_KEY_DOWN;
    meg4.mmio.padkeys[4] = MEG4_KEY_SPACE;
    meg4.mmio.padkeys[5] = MEG4_KEY_C;
    meg4.mmio.padkeys[6] = MEG4_KEY_X;
    meg4.mmio.padkeys[7] = MEG4_KEY_Z;
    meg4.screen.w = 320; meg4.screen.h = 200; meg4.screen.buf = meg4.vram;
    for(i = 0; i < 640 * 400; i++) meg4.vram[i] = htole32(0xff000000);
    memcpy(meg4.mmio.palette, default_pal, sizeof(meg4.mmio.palette));
    meg4.mmio.conf = 39; meg4_conrst();
    dsp_init();
    dsp_reset();
    cpu_init();
    /* uncompress and set default font */
    meg4_font = (uint8_t*)malloc(9 * 65536);
    if(meg4_font) {
        memset(meg4_font, 0, 9 * 65536);
        ptr = (uint8_t*)binary_default_sfn + 3;
        i = *ptr++; ptr += 6; if(i & 4) { k = *ptr++; k += (*ptr++ << 8); ptr += k; } if(i & 8) { while(*ptr++ != 0); }
        if(i & 16) { while(*ptr++ != 0); } j = sizeof(binary_default_sfn) - (size_t)(ptr - binary_default_sfn);
        font = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)ptr, j, 65536, &s, 0);
        if(font) {
            for(buf = font + le32toh(*((uint32_t*)(font + 16))), c = 0; c < 0x010000 && buf < font + s; c++) {
                if(buf[0] == 0xFF) { c += 65535; buf++; } else
                if((buf[0] & 0xC0) == 0xC0) { j = (((buf[0] & 0x3F) << 8) | buf[1]); c += j; buf += 2; } else
                if((buf[0] & 0xC0) == 0x80) { j = (buf[0] & 0x3F); c += j; buf++; } else {
                    ptr = buf + 6; o = meg4_font + c * 8;
                    for(i = n = 0; i < buf[1]; i++, ptr += buf[0] & 0x40 ? 6 : 5) {
                        if(ptr[0] == 255 && ptr[1] == 255) continue;
                        frg = font + (buf[0] & 0x40 ? ((ptr[5] << 24) | (ptr[4] << 16) | (ptr[3] << 8) | ptr[2]) :
                            ((ptr[4] << 16) | (ptr[3] << 8) | ptr[2]));
                        if((frg[0] & 0xE0) != 0x80) continue;
                        o += (int)(ptr[1] - n); n = ptr[1]; k = ((frg[0] & 0x0F) + 1) << 3; j = frg[1] + 1; frg += 2;
                        for(m = 1; j; j--, n++, o++)
                            for(l = 0; l < k; l++, m <<= 1) {
                                if(m > 0x80) { frg++; m = 1; }
                                if(*frg & m) *o |= m;
                            }
                    }
                    buf += 6 + buf[1] * (buf[0] & 0x40 ? 6 : 5);
                }
            }
            free(font);
        }
        memcpy(meg4.font, meg4_font, 8 * 65536);
        meg4_recalcfont(0, 0xffff);
        memcpy(meg4_font + 8 * 65536, meg4.font + 8 * 65536, 65536);
        /* clear control codes */
        memset(meg4.font, 0, 8 * 32);
        memset(meg4.font + 8 * 65536, 0, 32);
    }
#ifdef __WIN32__
    timeBeginPeriod(1000);
    if (QueryPerformanceFrequency(&ticks_per_second) == TRUE)
        QueryPerformanceCounter(&started);
    else {
        memset(&ticks_per_second, 0, sizeof(ticks_per_second));
        start = timeGetTime();
    }
    lastt = -1U;
#else
    clock_gettime(CLOCK_MONOTONIC, &started);
#endif
}

#ifndef NOEDITORS
/**
 * Load a floppy
 */
int meg4_load(uint8_t *buf, int len)
{
    int ret, i, j, l;
    uint32_t s;
    uint8_t *end = buf + len - 12, *floppy = NULL;
    char tmp[sizeof meg4_title + 32];

    if(!buf || len < 8 || memcmp(buf, "\x89PNG", 4) || ((buf[18] << 8) | buf[19]) != 210 || ((buf[22] << 8) | buf[23]) != 220)
        return 0;
    for(buf += 8; buf < end; buf += s + 12) {
        s = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
        if(!memcmp(buf + 4, "flPy", 4) && stbiw__crc32(buf + 4, s + 4) == (uint32_t)((buf[8+s]<<24)|(buf[9+s]<<16)|(buf[10+s]<<8)|buf[11+s])) {
            floppy = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char *)buf + 8, s, 65536, &len, 1);
            break;
        }
    }
    if(!floppy) return 0;
    meg4_reset();
    ret = meg4_deserialize(floppy, len);
    free(floppy);

    /* refresh overlays data from user's computer */
    if(ret && meg4_title[0])
        for(i = 0; i < 256; i++)
            if(meg4.ovls[i].data) {
                /* we don't have sprintf, no stdio used in libmeg4.a */
                strcpy(tmp, "overlays/"); strcat(tmp, meg4_title); strcat(tmp, "/mem"); l = strlen(tmp);
                j = (i >> 4);  tmp[l+0] = j < 10 ? j + '0' : j - 10 + 'A';
                j = (i & 0xf); tmp[l+1] = j < 10 ? j + '0' : j - 10 + 'A';
                strcpy(tmp + l + 2, ".bin"); floppy = main_cfgload(tmp, &l);
                if(floppy) { free(meg4.ovls[i].data); meg4.ovls[i].data = floppy; meg4.ovls[i].size = l; }
            }
    return ret;
}

/**
 * Save a floppy
 */
uint8_t *meg4_save(int *len)
{
    uint16_t x0, x1, y0, y1;
    uint8_t *out = NULL, *ptr, *buf, *comp;
    int w, h, c, size, siz;

    /* serialize to buf */
    if(!(buf = meg4_serialize(&siz, 0))) return NULL;
    comp = stbi_zlib_compress(buf, siz, &size, 9);
    free(buf);
    if(!comp) return NULL;

    ptr = (uint8_t*)stbi_load_from_memory(binary_floppy_png, sizeof(binary_floppy_png), &w, &h, &c, 4);
    if(ptr) {
        /* add screenshot and programming language's icon */
        meg4_screenshot((uint32_t*)ptr, 25, 96, w * 4);
        cpu_getlang();
        if(meg4_edicons.buf && meg4.code_type * 8 < meg4_edicons.w)
            meg4_blit((uint32_t*)ptr, w - 35, 98, w * 4, 8, 8, meg4_edicons.buf, meg4.code_type * 8, 64, meg4_edicons.w * 4, 1);
        /* add title */
        x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(25);  x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(w - 25);
        y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(202); y1 = meg4.mmio.cropy1; meg4.mmio.cropx1 = htole16(210);
        meg4_text((uint32_t*)ptr, 25, 202, w * 4, htole32(LABEL_COLOR), 0, 1, meg4_font, meg4_title);
        meg4.mmio.cropx0 = x0; meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
        /* save floppy */
        out = stbi_write_png_to_mem((unsigned char*)ptr, w * 4, w, h, 4, &siz, comp, size);
        if(len) *len = siz;
        free(ptr);
    }
    free(comp);
    return out;
}

/**
 * "Insert" floppies into drive, a dummy wrapper to import any kind of assets
 */
void meg4_insert(char *name, uint8_t *buf, int len)
{
    char *e;

    if(buf && len > 0) {
        main_log(1, "inserted %s (%u bytes)", name, len);
        /* if it's an image, check if it's also a floppy too */
        if(!memcmp(buf, "\x89PNG", 4) && ((buf[18] << 8) | buf[19]) == 210 && ((buf[22] << 8) | buf[23]) == 220) {
            main_log(1, "floppy detected");
            /* use the filename as title by default */
            strncpy(meg4_title, name, sizeof(meg4_title) - 1);
            e = strrchr(meg4_title, '.'); if(e) *e = 0;
            /* this will call meg4_load after the animation which will override title if meta chunk found */
            if(!load_insert(buf, len)) {
                main_log(1, "unable to load");
                meg4.mmio.ptrspr = MEG4_PTR_ERR;
            } else
                main_log(1, "load successful");
        } else
        /* nope, do normal asset import */
        if(!meg4_import(name, buf, len, 0)) {
            main_log(1, "unable to detect the format");
            meg4.mmio.ptrspr = MEG4_PTR_ERR;
        }
    }
}

/**
 * Switch operating mode
 */
extern meg4_dsp_t *dsp;
void meg4_switchmode(int mode)
{
    int i;

    if(mode == -1) mode = oldmode;
    if(mode == MEG4_MODE_GAME && !meg4.src_len) mode = MEG4_MODE_LOAD;
    /* clear keyboard queue */
    meg4.mmio.kbdhead = meg4.mmio.kbdtail = meg4.mmio.kbdkeys[0] = 0;
    textinp_free();
    if(mode < 0 || mode >= MEG4_NUM_MODE || mode == meg4.mode) return;
    /* free old mode's resources */
    switch(meg4.mode) {
        case MEG4_MODE_LOAD: load_free(); break;
        case MEG4_MODE_SAVE: save_free(); break;
        case MEG4_MODE_DEBUG: debug_free(); break;
        case MEG4_MODE_VISUAL: visual_free(); break;
        case MEG4_MODE_CODE: code_free(); break;
        case MEG4_MODE_SPRITE: sprite_free(); break;
        case MEG4_MODE_MAP: map_free(); break;
        case MEG4_MODE_FONT: font_free(); break;
        case MEG4_MODE_SOUND: sound_free(); break;
        case MEG4_MODE_MUSIC: music_free(); break;
        case MEG4_MODE_OVERLAY: overlay_free(); break;
        case MEG4_MODE_GAME:
            oldcs = meg4.mmio.ptrspr; meg4.mmio.ptrspr = MEG4_PTR_NORM;
            oldx0 = meg4.mmio.cropx0; oldx1 = meg4.mmio.cropx1;
            oldy0 = meg4.mmio.cropy0; oldy1 = meg4.mmio.cropy1;
        break;
    }
    /* set up new mode */
    oldmode = meg4.mode;
    meg4.mode = mode;
    if(mode != MEG4_MODE_GAME) {
        if(!meg4_defwaves)
            meg4_defwaves = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char *)binary_sounds_mod,
                sizeof(binary_sounds_mod), 65536, &i, 1);
        if(!meg4_edicons.buf)
            meg4_edicons.buf = (uint32_t*)stbi_load_from_memory(binary_editors_png, sizeof(binary_editors_png),
             &meg4_edicons.w, &meg4_edicons.h, &i, 4);
        meg4.mmio.cropx0 = meg4.mmio.cropy0 = 0; meg4.mmio.cropx1 = htole16(640); meg4.mmio.cropy1 = htole16(400);
        menu_scroll = menu_scrmax = 0;
        memset(&meg4.dalt, 0, sizeof(meg4.dalt));
        dsp = &meg4.dalt;
    } else {
        if(meg4_defwaves) { free(meg4_defwaves); meg4_defwaves = NULL; }
        if(meg4_edicons.buf) { free(meg4_edicons.buf); memset(&meg4_edicons, 0, sizeof(meg4_edicons)); }
        meg4.mmio.ptrspr = oldcs;
        meg4.mmio.cropx0 = oldx0; meg4.mmio.cropx1 = oldx1;
        meg4.mmio.cropy0 = oldy0; meg4.mmio.cropy1 = oldy1;
        dsp = &meg4.dram;
    }
    /* enter new mode */
    switch(meg4.mode) {
        case MEG4_MODE_HELP: help_show(-1); break;
        case MEG4_MODE_LOAD: load_init(); break;
        case MEG4_MODE_SAVE: save_init(); break;
        case MEG4_MODE_DEBUG: debug_init(); break;
        case MEG4_MODE_VISUAL: visual_init(); break;
        case MEG4_MODE_CODE: code_init(); break;
        case MEG4_MODE_SPRITE: sprite_init(); break;
        case MEG4_MODE_MAP: map_init(); break;
        case MEG4_MODE_FONT: font_init(); break;
        case MEG4_MODE_SOUND: sound_init(); break;
        case MEG4_MODE_MUSIC: music_init(); break;
        case MEG4_MODE_OVERLAY: overlay_init(); break;
    }
    /* in case one of the init functions have changed resolution */
    meg4_getscreen();
}
#endif

/**
 * Run the emulator
 */
void meg4_run(void)
{
    int i;
    /* get ticks, do this only once per frame generation */
#ifdef __WIN32__
    FILETIME ft;
    LARGE_INTEGER li;
    if(ticks_per_second.QuadPart) {
        QueryPerformanceCounter(&now);
        now.QuadPart -= started.QuadPart;
        now.QuadPart *= 1000;
        now.QuadPart /= ticks_per_second.QuadPart;
        meg4.mmio.tick = htole32(now.QuadPart);
    } else
        meg4.mmio.tick = htole32(timeGetTime() - start);
    if(le32toh(meg4.mmio.tick) / 1000 != lastt) {
        lastt = le32toh(meg4.mmio.tick) / 1000;
        GetSystemTimeAsFileTime(&ft);
        li.HighPart = ft.dwHighDateTime; li.LowPart = ft.dwLowDateTime;
        meg4.mmio.now = htole64((uint64_t)(li.QuadPart / 10000000 - INT64_C(11644473600)));
    }
#else
    clock_gettime(CLOCK_MONOTONIC, &now);
    meg4.mmio.now = htole64((uint64_t)now.tv_sec);
    meg4.mmio.tick = htole32(((now.tv_sec - started.tv_sec) * 1000 + (now.tv_nsec - started.tv_nsec) / 1000000));
#endif
    /* calculate how many msecs were unspent in the last frame, make sure not to overflow */
    i = 1000/60 - (le32toh(meg4.mmio.tick) - meg4_lasttick); meg4.mmio.perf = i < -127 ? -127 : i;
#ifndef NOEDITORS
    if(meg4.mode != MEG4_MODE_GAME) {
        /* clear the editors' screen */
        for(i = 0; i < 640 * 400; i++) meg4.valt[i] = theme[THEME_BG];
        /* handle events and redraw screen */
        if(!(menu_ctrl() | textinp_ctrl()))
            switch(meg4.mode) {
                case MEG4_MODE_LOAD: load_ctrl(); break;
                case MEG4_MODE_SAVE: save_ctrl(); break;
                case MEG4_MODE_HELP: help_ctrl(); break;
                case MEG4_MODE_DEBUG: debug_ctrl(); break;
                case MEG4_MODE_VISUAL: visual_ctrl(); break;
                case MEG4_MODE_CODE: code_ctrl(); break;
                case MEG4_MODE_SPRITE: sprite_ctrl(); break;
                case MEG4_MODE_MAP: map_ctrl(); break;
                case MEG4_MODE_FONT: font_ctrl(); break;
                case MEG4_MODE_SOUND: sound_ctrl(); break;
                case MEG4_MODE_MUSIC: music_ctrl(); break;
                case MEG4_MODE_OVERLAY: overlay_ctrl(); break;
            }
        switch(meg4.mode) {
            case MEG4_MODE_LOAD: load_view(); break;
            case MEG4_MODE_SAVE: save_view(); break;
            case MEG4_MODE_HELP: help_view(); break;
            case MEG4_MODE_DEBUG: debug_view(); break;
            case MEG4_MODE_VISUAL: visual_view(); break;
            case MEG4_MODE_CODE: code_view(); break;
            case MEG4_MODE_SPRITE: sprite_view(); break;
            case MEG4_MODE_MAP: map_view(); break;
            case MEG4_MODE_FONT: font_view(); break;
            case MEG4_MODE_SOUND: sound_view(); break;
            case MEG4_MODE_MUSIC: music_view(); break;
            case MEG4_MODE_OVERLAY: overlay_view(); break;
        }
    } else
#endif
    {
        cpu_run();
    }
    meg4_lasttick = le32toh(meg4.mmio.tick);
}

