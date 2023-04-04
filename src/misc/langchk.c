/*
 * meg4/misc/langchk.c
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
 * @brief Quick'n'dirty tool to check if we have all glyphs for the translations
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STBI_ASSERT(x)
#include "../stb_image.h"
#include "emoji.h"

int main(int argc, char **argv)
{
    DIR *dir;
    struct dirent *ent;
    FILE *f = NULL;
    uint8_t *font, *ptr, *buf, has[0x110000];
    uint32_t size, c;
    int i, j, k, s;
    char fn[256];

    (void)argc; (void)argv;
    printf("MEG-4 - Language Checker Tool by bzt Copyright (C) 2023 GPLv3\n\n");

    /* read the font */
    f = fopen("default.sfn", "rb");
    if(!f) { fprintf(stderr, "langchk: default.sfn not found!\n"); return 1; }
    fseek(f, 0, SEEK_END);
    size = (int)ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(size);
    if(!buf) { fclose(f); fprintf(stderr, "langchk: memory allocation error\n"); return 1; }
    fread(buf, 1, size, f);
    fclose(f);

    /* get which glyphs are defined in the font */
    memset(has, 0, sizeof(has));
    ptr = buf + 3;
    i = *ptr++; ptr += 6; if(i & 4) { k = *ptr++; k += (*ptr++ << 8); ptr += k; } if(i & 8) { while(*ptr++ != 0); }
    if(i & 16) { while(*ptr++ != 0); } j = size - (int)(ptr - buf);
    font = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)ptr, j, 65536, &s, 0);
    free(buf);
    if(font) {
        for(buf = font + *((uint32_t*)(font + 16)), c = 0; c < 0x010000 && buf < font + s; c++) {
            if(buf[0] == 0xFF) { c += 65535; buf++; } else
            if((buf[0] & 0xC0) == 0xC0) { j = (((buf[0] & 0x3F) << 8) | buf[1]); c += j; buf += 2; } else
            if((buf[0] & 0xC0) == 0x80) { j = (buf[0] & 0x3F); c += j; buf++; } else {
                has[c] = 1; buf += 6 + buf[1] * (buf[0] & 0x40 ? 6 : 5);
            }
        }
        free(font);
    }

    /* get translations */
    strcpy(fn, "../lang");
    if((dir = opendir(fn))) {
        strcat(fn, "/");
        while((ent = readdir(dir))) {
            if(ent->d_name[0] == '.') continue;
            strcpy(fn + 8, ent->d_name);
            f = fopen(fn, "rb");
            if(f) {
                fseek(f, 0, SEEK_END);
                size = (int)ftell(f);
                fseek(f, 0, SEEK_SET);
                buf = ptr = malloc(size + 1);
                if(!buf) { fclose(f); fprintf(stderr, "langchk: memory allocation error\n"); return 1; }
                fread(buf, 1, size, f); buf[size] = 0;
                fclose(f);
                while(*ptr) {
                    if((*ptr & 128) != 0) {
                        if(!(*ptr & 32)) { c = ((*ptr & 0x1F)<<6)|(*(ptr+1) & 0x3F); ptr += 1; } else
                        if(!(*ptr & 16)) { c = ((*ptr & 0xF)<<12)|((*(ptr+1) & 0x3F)<<6)|(*(ptr+2) & 0x3F); ptr += 2; } else
                        if(!(*ptr & 8)) { c = ((*ptr & 0x7)<<18)|((*(ptr+1) & 0x3F)<<12)|((*(ptr+2) & 0x3F)<<6)|(*(ptr+3) & 0x3F); ptr += 3; }
                        else c = 0;
                    } else c = *ptr;
                    ptr++;
                    if(c != 32 && c != 160 && c != 0x2328 && c != 0x1F3AE && c != 0x1F5B1 && (c < EMOJI_FIRST || c >= EMOJI_LAST)) has[c] |= 2;
                }
                free(buf);
            }
        }
        closedir(dir);
    }

    /* check if there's a used codepoint without a glyph */
    for(i = j = 0; i < (int)sizeof(has); i++)
        if(has[i] == 2) {
            if(!j) { printf("Missing glyphs:\n"); } j++;
            printf("  U+%04X\n", i);
        }
    if(!j) printf("OK! We have all the glyphs needed for the translations!\n");

    return 0;
}
