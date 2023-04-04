/*
 * meg4/tests/converter/main.c
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
 * @brief Tests MEG-4 import/export capabilities
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <stdio.h>
#include "../../src/meg4.h"
#include "editors.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_ONLY_PNG
#define STBI_WRITE_NO_FAILURE_STRINGS
#define STBI_WRITE_NO_SIMD
#define STBI_WRITE_NO_STDIO
#define STBIW_ASSERT(x)
#include "../../src/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STBI_ASSERT(x)
#include "../../src/stb_image.h"
#include "../../src/floppy.h"

meg4_t meg4;
uint8_t *meg4_font = NULL, *meg4_defwaves = NULL, *meg4_init = NULL;
uint32_t meg4_init_len = 0;
char meg4_title[64], meg4_author[64], meg4_pro = 0;
void meg4_switchmode(int mode) { (void)mode; }
void meg4_recalcfont(int s, int e) { (void)s; (void)e; }
void meg4_recalcmipmap(void) { }

uint8_t meg4_palidx(uint8_t *rgba)
{
    uint8_t ret = 0, *b;
    int i, dr, dg, db, d, dm = 0x7fffffff;

    for(i = 0; i < 256 && dm > 0; i++) {
        b = (uint8_t*)&meg4.mmio.palette[i];
        if(rgba[0] == b[0] && rgba[1] == b[1] && rgba[2] == b[2]) return i;
        db = rgba[2] > b[2] ? rgba[2] - b[2] : b[2] - rgba[2];
        dg = rgba[1] > b[1] ? rgba[1] - b[1] : b[1] - rgba[1];
        dr = rgba[0] > b[0] ? rgba[0] - b[0] : b[0] - rgba[0];
        d = ((dr*dr) << 1) + (db*db) + ((dg*dg) << 2);
        if(d < dm) { dm = d; ret = i; }
    }
    return ret;
}
void sound_addwave(int wave) { (void)wave; }
void code_init(void) { }
int overlay_idx = 0;

int main_cfgsave(char *cfg, uint8_t *buf, int len) { (void)cfg; (void)buf; (void)len; return 1; }

int main_writefile(char *file, uint8_t *buf, int size)
{
    int ret = 0;
    FILE *f;

    if(!file || !*file || !buf || size < 1) return 0;
    f = fopen(file, "wb");
    if(f) {
        ret = ((int)fwrite(buf, 1, size, f) == size);
        fclose(f);
    }
    return ret;
}
int main_savefile(const char *name, uint8_t *buf, int len) { return main_writefile((char*)name, buf, len); }

/**
 * Log messages
 */
void main_log(int lvl, const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    (void)lvl;
    printf("meg4: "); vprintf(fmt, args); printf("\r\n");
    __builtin_va_end(args);
}

/**
 * Import files
 */
int main_import(char *name, uint8_t *buf, int len)
{
    uint32_t s;
    uint8_t *end = buf + len - 12, *floppy = NULL;

    if(!memcmp(buf, "\x89PNG", 4) && ((buf[18] << 8) | buf[19]) == 210 && ((buf[22] << 8) | buf[23]) == 220) {
        for(buf += 8; buf < end; buf += s + 12) {
            s = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
            if(!memcmp(buf + 4, "flPy", 4) && stbiw__crc32(buf + 4, s + 4) == (uint32_t)((buf[8+s]<<24)|(buf[9+s]<<16)|(buf[10+s]<<8)|buf[11+s])) {
                floppy = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char *)buf + 8, s, 65536, &len, 1);
                break;
            }
        }
        if(!floppy) return 0;
        s = meg4_deserialize(floppy, len);
        free(floppy);
        return s;
    } else
        return meg4_import(name, buf, len, 0);
}

/**
 * Main function
 */
int main(int argc, char **argv)
{
    FILE *f;
    uint8_t *buf = NULL;
    int i, len = 0;

    /* load input */
    if(argc < 1 || !argv[1]) {
        printf("MEG-4 Converter by bzt Copyright (C) 2023 GPLv3+\r\n\r\n");
        printf("%s <somefile>\r\n", argv[0]);
        exit(1);
    }
    f = fopen(argv[1], "rb");
    if(f) {
        fseek(f, 0, SEEK_END);
        len = (int)ftell(f);
        fseek(f, 0, SEEK_SET);
        buf = malloc(len);
        if(buf) fread(buf, 1, len, f); else len = 0;
        fclose(f);
    }
    printf("importing...\n");

    memset(&meg4, 0, sizeof(meg4));
    memcpy(meg4.mmio.palette, default_pal, sizeof(meg4.mmio.palette));
    /* just in case, if someone wants to import MIDI files */
    meg4_defwaves = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char *)binary_sounds_mod, sizeof(binary_sounds_mod), 65536, &i, 1);

    /* import */
    if(!main_import(argv[1], buf, len)) {
        free(buf); free(meg4_defwaves);
        printf("unable to load file\n");
        exit(1);
    }
    free(buf);
    free(meg4_defwaves);

    printf("exporting...\n");

    /* export */
    meg4_export(argv[2] ? argv[2] : "output.zip", 0);
    return 0;
}
