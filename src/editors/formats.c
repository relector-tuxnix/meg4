/*
 * meg4/editors/formats.c
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
 * @brief Import / export files in various formats
 *
 */

#include <stdio.h>
#include "editors.h"
#include "../stb_image_write.h"
#include "zip.h"
#include "fmt_mod.h"
#include "fmt_mid.h"
#include "fmt_tic80.h"
#include "fmt_pico8.h"

#ifndef MEG4_PRO
/* have some dummy implementation, because importer needs them */
void pro_init(void) {}
int  pro_license(uint8_t *buf, int len) { (void)buf; (void)len; return 0; }
#endif

/**
 * Read in a hex number in ASCII
 */
uint32_t gethex(uint8_t *ptr, int len)
{
    uint32_t ret = 0;
    for(;len--;ptr++) {
        if(*ptr>='0' && *ptr<='9') {          ret <<= 4; ret += (uint32_t)(*ptr-'0'); }
        else if(*ptr >= 'a' && *ptr <= 'f') { ret <<= 4; ret += (uint32_t)(*ptr-'a'+10); }
        else if(*ptr >= 'A' && *ptr <= 'F') { ret <<= 4; ret += (uint32_t)(*ptr-'A'+10); }
        else break;
    }
    return ret;
}

/**
 * RFC4648 base64 decoder. Decode in place, return out length
 */
static int base64_decode(char *s)
{
    char *out = s, *ret = s;
    int b = 0, c = 0, d;
    while(*s && *s != '=' && *s != '<') {
        while(*s && (*s == ' ' || *s == '\r' || *s == '\n')) s++;
        if(!*s) break;
        if(*s >= 'A' && *s <= 'Z') d = *s - 'A'; else
        if(*s >= 'a' && *s <= 'z') d = *s - 'a' + 26; else
        if(*s >= '0' && *s <= '9') d = *s - '0' + 52; else
        if(*s == '+' || *s == '-' || *s == '.') d = 62; else
        if(*s == '/' || *s == '_') d = 63; else
            continue;
        b += d; c++; s++;
        if(c == 4) {
            *out++ = (b >> 16);
            *out++ = ((b >> 8) & 0xff);
            *out++ = (b & 0xff);
            b = c = 0;
        } else {
            b <<= 6;
        }
    }
    switch(c) {
        case 1: return 0;
        case 2: *out++ = (b >> 10); break;
        case 3: *out++ = (b >> 16); *out++ = ((b >> 8) & 0xff); break;
    }
    *out = 0;
    return (int)((uintptr_t)out - (uintptr_t)ret);
}

/**
 * Add a bitmap to font
 */
static void bitmap(uint32_t unicode, int l, int t, int w, int h, uint8_t *bitmap)
{
    int x, y, s, m;
    if(unicode > 0xffff || l > 7 || t > 7 || w < 1 || h < 1 || !bitmap) return;
    if(t + h > 8) h = 8 - t;
    for(y = s = 0; y < h; y++, s += w)
        for(x = 0, m = (1 << l); x < w && m < 0x100; x++, m <<= 1)
            if(bitmap[s + x]) meg4.font[(unicode << 3) + t + y] |= m;
}

/**
 * Export MEG-4 state as a project file
 */
void meg4_export(char *name, int binary)
{
    char tmp[256];
    uint32_t pal[256], color;
    uint8_t *zip, *out, *end, *s, *d;
    int i, j, k, l, m, o, r, g, b, len = 0;

    main_log(1, "exporting project with %s files", binary ? "binary" : "text");
    zip_open();

    /* meta info */
    sprintf(tmp, "MEG-4 Firmware v%u.%u.%u\r\n%s\r\n", meg4.mmio.fwver[0], meg4.mmio.fwver[1], meg4.mmio.fwver[2], meg4_title);
    zip_add("metainfo.txt", (uint8_t*)tmp, strlen(tmp));

    /* source. Unfortunately we have to convert newlines for dummy Windows tools and users */
    if(meg4.src && meg4.src_len) {
        for(len = meg4.src_len, end = (uint8_t*)meg4.src + meg4.src_len, s = (uint8_t*)meg4.src; s < end; s++)
            if(*s == '\n') len++;
        out = (uint8_t*)malloc(len);
        if(out) {
            for(s = (uint8_t*)meg4.src, d = out; s < end && *s; s++) { if(*s == '\n') { *d++ = '\r'; } *d++ = *s; } len = d - out;
            /* grab the extension from the shebang */
            strcpy(tmp, "program.");
            if(meg4.src[0] == '#' && meg4.src[1] == '!') {
                for(d = (uint8_t*)tmp + 8, s = (uint8_t*)meg4.src + 2; s < end && d < (uint8_t*)tmp + 32 && ((*s >= '0' && *s <= '9') ||
                    (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || *s == '_'); s++, d++) *d = *s;
                *d = 0;
            } else strcat(tmp, "txt");
            zip_add(tmp, out, len);
            free(out);
        }
    }

    /* sprites */
    if(!meg4_isbyte(meg4.mmio.sprites, 0, 256 * 256)) {
        out = stbi_write_png_to_mem(meg4.mmio.sprites, 256, 256, 256, 1, &len, (uint8_t*)meg4.mmio.palette, 0);
        if(out) {
            zip_add("sprites.png", out, len);
            free(out);
        }
    }

    /* map */
    if(!meg4_isbyte(meg4.mmio.map, 0, 320 * 200)) {
        if(binary) {
            for(i = 1; i < 256; i++) {
                r = g = b = 0;
                for(j = 0; j < 8; j++)
                    for(k = 0; k < 8; k++) {
                        color = meg4.mmio.palette[meg4.mmio.sprites[(((((meg4.mmio.mapsel << 8) | i) >> 5) + j) << 8) + ((i & 31) << 3) + k]];
                        r += color & 0xff; g += (color >> 8) & 0xff; b += (color >> 16) & 0xff;
                    }
                r >>= 6; g >>= 6; b >>= 6;
                pal[i] = (0xff << 24) | (b << 16) | (g << 8) | (r);
            }
            pal[0] = meg4.mmio.mapsel << 24;
            out = stbi_write_png_to_mem(meg4.mmio.map, 320, 320, 200, 1, &len, (uint8_t*)pal, 0);
            if(out) {
                zip_add("map.png", out, len);
                free(out);
            }
        } else {
            out = (uint8_t*)malloc(1024 + 320 * 200 * 5);
            if(out) {
                d = out + sprintf((char*)out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
                d += sprintf((char*)d, "<map version=\"1.5\" orientation=\"orthogonal\" renderorder=\"left-down\" compressionlevel=\"0\""
                    " width=\"320\" height=\"200\" tilewidth=\"8\" tileheight=\"8\" infinite=\"0\" nextlayerid=\"2\" nextobjectid=\"1\">\r\n");
                d += sprintf((char*)d, " <tileset firstgid=\"1\" name=\"MEG-4 Sprites\" tilewidth=\"8\" tileheight=\"8\" tilecount=\"1024\" "
                    " columns=\"32\" tilerendersize=\"grid\">\r\n");
                d += sprintf((char*)d, "  <image source=\"sprites.png\" width=\"256\" height=\"256\"/>\r\n");
                d += sprintf((char*)d, " </tileset>\r\n");
                d += sprintf((char*)d, " <layer id=\"1\" name=\"MEG-4 Map\" width=\"320\" height=\"200\">\r\n");
                d += sprintf((char*)d, "  <data encoding=\"csv\">\r\n");
                for(len = 0; len < 320 * 200; len++)
                    d += sprintf((char*)d, "%s%s%u", len ? "," : "", len && !(len % 320) ? "\r\n" : "",
                        meg4.mmio.map[len] ? 1 + ((meg4.mmio.mapsel << 8) | meg4.mmio.map[len]) : 0);
                d += sprintf((char*)d, "\r\n  </data>\r\n");
                d += sprintf((char*)d, " </layer>\r\n");
                d += sprintf((char*)d, "</map>\r\n");
                zip_add("map.tmx", out, (uintptr_t)d - (uintptr_t)out);
                free(out);
            }
        }
    }

    /* font (only save if it differs to the built-in one) */
    if(!meg4_isbyte(meg4.font, 0, 32 * 8) || memcmp(meg4.font + 32 * 8, meg4_font + 32 * 8, 65504 * 8)) {
        for(i = len = 0; i < 65536; i++) if(i == 0 || i == 32 || i == 160 || !meg4_isbyte(meg4.font + (i << 3), 0, 8)) len++;
        if(binary) {
            out = (uint8_t*)malloc(32 + len * 16);
            if(out) {
                memset(out, 0, 32 + len * 16);
                d = out;
                /* PSF2 header */
                d[0] = 0x72; d[1] = 0xb5; d[2] = 0x4a; d[3] = 0x86;
                d[8] = 32; d[12] = 1; d[16] = len & 0xff; d[17] = (len >> 8) & 0xff;
                d[20] = d[24] = d[28] = 8; d += 32;
                /* glyhps */
                for(i = 0; i < 65536; i++) {
                    s = meg4.font + (i << 3);
                    if(i == 0 || i == 32 || i == 160 || !meg4_isbyte(s, 0, 8)) {
                        /* unfortunately we can't use memcpy, we have to mirror the bits */
                        for(j = 0; j < 8; j++, s++, d++)
                            for(l = 0; l < 8; l++)
                                if(s[0] & (1 << l)) d[0] |= (0x80 >> l);
                    }
                }
                /* UNICODE table */
                for(i = 0; i < 65536; i++)
                    if(i == 0 || i == 32 || i == 160 || !meg4_isbyte(meg4.font + (i << 3), 0, 8)) {
                        if(i < 0x80) *d++ = i; else
                        if(i < 0x800) { *d++ = ((i>>6)&0x1F)|0xC0; *d++ = (i&0x3F)|0x80; }
                        else { *d++ = ((i>>12)&0x0F)|0xE0; *d++ = ((i>>6)&0x3F)|0x80; *d++ = (i&0x3F)|0x80; }
                        *d++ = 0xff;
                    }
                zip_add("font.psfu", out, (uintptr_t)d - (uintptr_t)out);
                free(out);
            }
        } else {
            out = (uint8_t*)malloc(1024 + len * 256);
            if(out) {
                d = out + sprintf((char*)out, "STARTFONT 2.2\r\n");
                d += sprintf((char*)d, "FONT -MEG4-8x8-Regular-R-Normal--8-80-75-75-C-80-ISO10646-1\r\n");
                d += sprintf((char*)d, "SIZE 8 75 75\r\n");
                d += sprintf((char*)d, "FONTBOUNDINGBOX 8 8 0 -1\r\n");
                d += sprintf((char*)d, "STARTPROPERTIES 11\r\n");
                d += sprintf((char*)d, "FOUNDRY \"MEG4\"\r\n");
                d += sprintf((char*)d, "FAMILY_NAME \"8x8\"\r\n");
                d += sprintf((char*)d, "WEIGHT_NAME \"Regular\"\r\n");
                d += sprintf((char*)d, "SLANT \"R\"\r\n");
                d += sprintf((char*)d, "SETWIDTH_NAME \"Normal\"\r\n");
                d += sprintf((char*)d, "PIXEL_SIZE 8\r\n");
                d += sprintf((char*)d, "CHARSET_REGISTRY \"ISO10646\"\r\n");
                d += sprintf((char*)d, "CHARSET_ENCODING \"1\"\r\n");
                d += sprintf((char*)d, "COPYRIGHT \"GPLv3+\"\r\n");
                d += sprintf((char*)d, "FONT_ASCENT 7\r\n");
                d += sprintf((char*)d, "FONT_DESCENT 1\r\n");
                d += sprintf((char*)d, "ENDPROPERTIES\r\n");
                d += sprintf((char*)d, "CHARS %u\r\n", len);
                for(i = 0, s = meg4.font; i < 65536; i++, s += 8)
                    if(i == 0 || i == 32 || i == 160 || !meg4_isbyte(s, 0, 8)) {
                        d += sprintf((char*)d, "STARTCHAR uni%04X\r\n", i);
                        d += sprintf((char*)d, "ENCODING %u\r\n", i);
                        d += sprintf((char*)d, "BBX 8 8 0 -1\r\n");
                        d += sprintf((char*)d, "BITMAP\r\n");
                        for(j = 0; j < 8; j++) {
                            /* unfortunately bdf uses right to left for whatever reason, so we have to mirror the bitmap */
                            for(o = 0, m = 1; m < 256; m <<= 1, o <<= 1)
                                if(s[j] & m) o |= 1;
                            d += sprintf((char*)d, "%02X\r\n", o >> 1);
                        }
                        d += sprintf((char*)d, "ENDCHAR\r\n");
                    }
                d += sprintf((char*)d, "ENDFONT\r\n");
                zip_add("font.bdf", out, (uintptr_t)d - (uintptr_t)out);
                free(out);
            }
        }
    }

    /* sound effects */
    if(!meg4_isbyte(meg4.mmio.sounds, 0, sizeof(meg4.mmio.sounds)) || !meg4_isbyte(meg4.waveforms, 0, sizeof(meg4.waveforms))) {
        out = export_mod(-1, &len);
        if(out) {
            zip_add("sounds.mod", out, len);
            free(out);
        }
    }

    /* music tracks */
    for(i = 0; i < (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])); i++) {
        if(!meg4_isbyte(meg4.tracks[i], 0, sizeof(meg4.tracks[0]))) {
            out = export_mod(i, &len);
            if(out) {
                sprintf(tmp, "music%02X.mod", i);
                zip_add(tmp, out, len);
                free(out);
            }
        }
    }

    /* memory overlays */
    for(i = 0; i < 256; i++)
        if(meg4.ovls[i].data && meg4.ovls[i].size > 0) {
            if(binary) {
                sprintf(tmp, "mem%02X.bin", i);
                zip_add(tmp, meg4.ovls[i].data, meg4.ovls[i].size);
            } else {
                /* overlays are binary blobs, so the best we can do is using the same format as `hexdump -C` */
                l = ((meg4.ovls[i].size + 15) >> 4);
                len = l * 80;
                out = (uint8_t*)malloc(len + 1);
                if(out) {
                    memset(out, ' ', len);
                    for(j = 0, s = meg4.ovls[i].data, d = out; j < l; j++, s += 16, d += 80) {
                        sprintf((char*)d, "%08X", j << 4); d[8] = ' '; d[60] = '|'; d[78] = '\r'; d[79] = '\n';
                        for(m = 0; m < 16 && (j << 4) + m < (int)meg4.ovls[i].size; m++) {
                            sprintf((char*)d + 10 + m * 3 + (m > 7 ? 1 : 0), "%02X", s[m]); d[12 + m * 3 + (m > 7 ? 1 : 0)] = ' ';
                            d[61 + m] = s[m] >= ' ' && s[m] < 128 ? s[m] : '.';
                        }
                        d[61 + m] = '|';
                    }
                    sprintf(tmp, "mem%02X.txt", i);
                    zip_add(tmp, out, len);
                    free(out);
                }
            }
        }

    /* write out archive */
    len = 0;
    zip = zip_close(&len);
    if(zip) {
        if(name) {
            main_writefile(name, zip, len);
        } else {
            sprintf(tmp, "%s.zip", meg4_title[0] ? meg4_title : "noname");
            if(!main_savefile(tmp, zip, len)) {
                main_log(1, "unable to save");
                meg4.mmio.ptrspr = MEG4_PTR_ERR;
            } else
                main_log(1, "save successful");
        }
        free(zip);
    }
}

/**
 * Detect format and do the actual asset import
 */
int meg4_import(char *name, uint8_t *buf, int len, int lvl)
{
    TicHeader_t header;
    zip_local_t *loc;
    signed char tmp[sizeof(meg4.waveforms[0])];
    int ret = 1, a, b, c, i, j, k, l, t, w, h, x, y;
    uint8_t *uncomp = NULL, *end = buf + len, *s, *e, *o, *ptr, *frg, bmp[256], dec[4];
    uint32_t *data, unicode;

    if(!buf || len < 4) return 0;

    /* if it's a zip, iterate on its contents */
    if(!memcmp(buf, "PK\003\004", 4)) {
        /* zip in a zip bomb? Not on my watch */
        if(lvl) return 0;
        main_log(1, "importing project");
        for(s = buf; s < end - 4 && !memcmp(s, "PK\003\004", 4);) {
            loc = (zip_local_t*)s;
            memset(tmp, 0, 64); memcpy(tmp, s + sizeof(zip_local_t), loc->fnlen > 63 ? 63 : loc->fnlen);
            e = (uint8_t*)strrchr((char*)tmp, '/'); if(!e) e = (uint8_t*)tmp; else e++;
            i = 0;
            switch(loc->method) {
                case ZIP_DATA_STORE:
                    i = meg4_import((char*)e, s + sizeof(zip_local_t) + loc->fnlen + loc->extlen, loc->comp, 1);
                break;
                case ZIP_DATA_DEFLATE:
                    uncomp = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag(
                        (const char*)s + sizeof(zip_local_t) + loc->fnlen + loc->extlen, loc->comp, 65536, &k, 0);
                    if(uncomp) { i = meg4_import((char*)e, uncomp, k, 1); free(uncomp); uncomp = NULL; }
                break;
            }
            if(ret && !i) ret = 0;
            s += sizeof(zip_local_t) + loc->fnlen + loc->extlen + loc->comp;
        }
        meg4_switchmode(MEG4_MODE_CODE);
        return 1;
    }

    /* if gzipped */
    if(buf[0] == 0x1f && buf[1] == 0x8b) {
        uncomp = buf + 3;
        i = *uncomp++; uncomp += 6; if(i & 4) { k = *uncomp++; k += (*uncomp++ << 8); uncomp += k; } if(i & 8) { while(*uncomp++ != 0); }
        if(i & 16) { while(*uncomp++ != 0); } j = len - (size_t)(uncomp - buf);
        uncomp = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)uncomp, j, 65536, &k, 0);
        if(uncomp && k > 16) { buf = uncomp; len = k; } else { ret = 0; goto end; }
    }

    /* MEG-4 PRO license */
    if(!lvl && len > 64 && !memcmp(buf, "---------------------MEG-4 PRO License File---------------------", 64)) {
        main_log(1, "license file detected");
        if(pro_license(buf, len)) main_cfgsave("license.txt", buf, len);
        goto end;
    }

    /* MEG-4 theme */
    if(!lvl && len > 12 && !memcmp(buf, "GIMP Palette", 12)) {
        main_log(1, "theme detected", i);
        if(meg4_theme(buf, len)) main_cfgsave("theme.gpl", buf, len);
        goto end;
    }

    /* memory overlays (do this soon, in case the overlay's data contains some magic bytes which would be interpreted) */
    if(name && strlen(name) == 9 && !memcmp(name, "mem", 3) && name[5] == '.') {
        i = gethex((uint8_t*)name + 3, 2);
        if(len > MEG4_MEM_USER + (int)sizeof(meg4.data)) len = MEG4_MEM_USER + (int)sizeof(meg4.data);
        meg4.ovls[i].size = len;
        meg4.ovls[i].data = (uint8_t*)realloc(meg4.ovls[i].data, len);
        if(meg4.ovls[i].data) {
            /* binary format */
            if(!memcmp(name + 6, "bin", 4)) {
                main_log(1, "memory overlay %02X (bin) detected", i);
                memcpy(meg4.ovls[i].data, buf, len);
            } else
            /* hexdump format */
            if(!memcmp(name + 6, "txt", 4)) {
                main_log(1, "memory overlay %02X (hex) detected", i);
                memset(meg4.ovls[i].data, 0, len);
                for(s = buf; s < end - 10;) {
                    j = gethex(s, 8); s += 9; if(*s == ' ') s++;
                    if(j >= len) break;
                    for(k = 0; k < 16 && j + k < len; k++) {
                        meg4.ovls[i].data[j + k] = gethex(s, 2); s += 3;
                        if(*s == ' ') s++;
                    }
                    while(s < end && *s && *s != '\r' && *s != '\n') s++;
                    while(s < end && (*s == '\r' || *s == '\n')) s++;
                }
            }
        } else meg4.ovls[i].size = 0;
        if(!lvl) { overlay_idx = i; meg4_switchmode(MEG4_MODE_OVERLAY); }
        goto end;
    }

    /* tic cartridge */
    if((e = name ? (uint8_t*)strrchr(name, '.') : NULL) && !memcmp(e, ".tic", 5)) {
        main_log(1, "tic-80 (bin) detected");
        if((ret = format_tic80(buf, len)) && !lvl) meg4_switchmode(MEG4_MODE_CODE);
        goto end;
    }

    /* pico8 cartridge (text) */
    if(len > 16 && !memcmp(buf, "pico-8 cartridge", 16)) {
        main_log(1, "pico-8 (text) detected");
        if((ret = format_pico8(buf, len)) && !lvl) meg4_switchmode(MEG4_MODE_CODE);
        goto end;
    }

    /* PNG, this could be a lot of things */
    if(!memcmp(buf, "\x89PNG", 4)) {
        i = ((buf[18] << 8) | buf[19]); j = ((buf[22] << 8) | buf[23]);
        if(i == 256 && j == 256) {
            for(s = buf + 8; s < end - 12; s += k + 12) {
                k = ((s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3]);
                /* could be a tic-80 cartridge */
                if(!memcmp(s + 4, "caRt", 4)) { e = s + 8; goto ticbin; }
                /* or an indexed png with our sprites */
                if(!memcmp(s + 4, "PLTE", 4)) {
                    memset(meg4.mmio.palette, 0, sizeof(meg4.mmio.palette));
                    memset(meg4.mmio.sprites, 0, sizeof(meg4.mmio.sprites));
                    for(s += 8, e = s + k + 4, i = 0; s < e - 4 && i < 256; i++, s += 3)
                        meg4.mmio.palette[i] = (0xff << 24) | (s[2] << 16) | (s[1] << 8) | s[0];
                    if(!memcmp(e + 4, "tRNS", 4)) {
                        k = ((e[0] << 24) | (e[1] << 16) | (e[2] << 8) | e[3]); if(k > 256) k = 256;
                        for(i = 0, e += 8; i < k; i++, e++)
                            *((uint8_t*)&meg4.mmio.palette[i] + 3) = e[0];
                    }
                    s = (uint8_t*)stbi_load_from_memory(buf, len, &i, &j, &k, 1);
                    if(s) {
                        i *= j; if(i > (int)sizeof(meg4.mmio.sprites)) i = sizeof(meg4.mmio.sprites);
                        main_log(1, "sprites (png) detected");
                        memcpy(meg4.mmio.sprites, s, i);
                        free(s);
                    } else ret = 0;
                    if(!lvl) meg4_switchmode(MEG4_MODE_SPRITE);
                    goto end;
                }
            }
            /* no palette chunk, this must be a truecolor image */
            s = (uint8_t*)stbi_load_from_memory(buf, len, &i, &j, &k, 4);
            if(s) {
                memcpy(meg4.mmio.palette, default_pal, sizeof(meg4.mmio.palette));
                memset(meg4.mmio.sprites, 0, sizeof(meg4.mmio.sprites));
                main_log(1, "sprites (truecolor png) detected");
                for(j = k = 0, e = s; j < 256; j++)
                    for(i = 0; i < 256; i++, k++, e += 4)
                        meg4.mmio.sprites[k] = meg4_palidx(e);
                free(s);
            } else ret = 0;
            if(!lvl) meg4_switchmode(MEG4_MODE_SPRITE);
            goto end;
        } else
        /* if it's a map */
        if(buf[25] == 3 && i == 320 && j == 200) {
            main_log(1, "map (png) detected");
            memset(meg4.mmio.map, 0, sizeof(meg4.mmio.map));
            meg4.mmio.mapsel = 0;
            s = (uint8_t*)stbi_load_from_memory(buf, len, &i, &j, &k, 1);
            if(s) {
                i *= j; if(i > (int)sizeof(meg4.mmio.map)) i = sizeof(meg4.mmio.map);
                memcpy(meg4.mmio.map, s, i);
                free(s);
                for(s = buf + 8; s < end - 12; s += k + 12) {
                    k = ((s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3]);
                    if(!memcmp(s + 4, "PLTE", 4)) { meg4.mmio.mapsel = s[11]; break; }
                }
            } else ret = 0;
            if(!lvl) meg4_switchmode(MEG4_MODE_MAP);
            goto end;
        } else
        /* otherwise probably a foreign cartridge */
        if((buf[25] == 2 || buf[25] == 6) && ((i == 160 && j == 205) || (i == 256 && j == 256))) {
            e = (uint8_t*)malloc(i * j);
            if(e) {
                memset(e, 0, i * j);
                s = (uint8_t*)stbi_load_from_memory(buf, len, &i, &j, &k, 4);
                if(s) {
                    /* unfortunately there's no way of knowing that what we decrypt here is actually valid data... */
                    if(i == 160) {
                        for(i = 0; i < 160 * 205; i++)
                            e[i] = ((s[i * 4 + 0] & 3) << 4) | ((s[i * 4 + 1] & 3) << 2) |
                                     ((s[i * 4 + 2] & 3) << 0) | ((s[i * 4 + 3] & 3) << 6);
                        main_log(1, "pico-8 (png) detected");
                        if((ret = format_pico8(e, 160 * 205)) && !lvl) meg4_switchmode(MEG4_MODE_CODE);
                    } else {
                        for(k = 0; k < HEADER_SIZE; k++)
                            bitcpy(header.data, k * HEADER_BITS, s, k << 3, HEADER_BITS);
                        if(header.st.bits > 0 && header.st.bits <= BITS_IN_BYTE && header.st.size > 0
                          && header.st.size <= i * j * 4 * header.st.bits / BITS_IN_BYTE - HEADER_SIZE) {
                            k = header.st.size + ceildiv(header.st.size * BITS_IN_BYTE % header.st.bits, BITS_IN_BYTE);
                            e = (uint8_t*)realloc(e, k);
                            if(e) {
                                for (i = 0, j = ceildiv(header.st.size * BITS_IN_BYTE, header.st.bits); i < j; i++)
                                    bitcpy(e, i * header.st.bits, s + HEADER_SIZE, i << 3, header.st.bits);
                                free(s);
ticbin:                         main_log(1, "tic-80 (png) detected");
                                s = (uint8_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char *)e, k, 8192, &k, 0);
                                if(s) { if((ret = format_tic80(s, k)) && !lvl) { meg4_switchmode(MEG4_MODE_CODE); } free(s); }
                                free(buf);
                                goto end;
                            }
                        }
                    }
                    free(s);
                }
                free(e);
            }
        }
        goto end;
    }

    /* meta info */
    if(len > 25 && !memcmp(buf, "MEG-4 Firmware", 14)) {
        for(s = e = buf + 0x17; e < end && e < s + sizeof(meg4_title) - 1 && *e && *e != '\r' && *e != '\n'; e++);
        memset(meg4_title, 0, sizeof(meg4_title));
        memcpy(meg4_title, s, e - s);
        goto end;
    }

    /* source code */
    if(!memcmp(buf, "#!", 2)) {
        main_log(1, "source code detected");
        meg4.src_len = 0; memset(meg4.src_bm, 0, sizeof(meg4.src_bm));
        meg4.src = (char*)realloc(meg4.src, len + 1);
        if(meg4.src) {
            for(i = 0; i < len && buf[i]; i++)
                if(buf[i] >= ' ' || buf[i] == '\t' || buf[i] == '\n') meg4.src[meg4.src_len++] = buf[i];
            meg4.src[meg4.src_len++] = 0;
        }
        ret = meg4.src && meg4.src_len > 0;
        if(!lvl) { meg4_switchmode(MEG4_MODE_CODE); /* make sure highligh refreshed */ code_init(); }
        goto end;
    }

    /* Tiled tmx */
    if(len > 5 && !memcmp(buf, "<?xml", 5)) {
        main_log(1, "map (tmx) detected");
        ret = 0;
        for(s = buf; s < end - 6 && *s && memcmp(s, "<layer", 6); s++);
        if(s[0] == '<') {
            for(w = h = 0; s < end - 9 && *s != '>'; s++) {
                if(!memcmp(s, " width=", 7)) { s += 8; w = atoi((char*)s); }
                if(!memcmp(s, " height=", 8)) { s += 9; h = atoi((char*)s); }
            }
            if(w > 0 && h > 0) {
                if(h > 200) h = 200;
                /* we don't support XML and zstd, everything else is okay */
                for(s++, k = 0; s < end - 20 && *s && *s != '>'; s++) {
                    if(!memcmp(s, " encoding=\"csv\"", 15)) k = 1; else
                    if(!memcmp(s, " encoding=\"base64\"", 18)) k = 2; else
                    if(!memcmp(s, " compression=\"gzip\"", 19)) k = 3; else
                    if(!memcmp(s, " compression=\"zlib\"", 19)) k = 4;
                }
                if(k && *s == '>') {
                    memset(meg4.mmio.map, 0, sizeof(meg4.mmio.map));
                    meg4.mmio.mapsel = 0;
                    s++;
                    switch(k) {
                        case 1: /* CSV */
                            for(i = j = 0; *s && *s != '<' && j < h;) {
                                while(*s && (*s == ' ' || *s == '\r' || *s == '\n' || *s == ',')) s++;
                                if(!*s) break;
                                k = atoi((char*)s); if(k > 0) { k--; } while(*s && *s >= '0' && *s <= '9') s++;
                                if(i < 320 && k < 1024) { meg4.mmio.map[j * 320 + i] = k & 0xff; if(k) meg4.mmio.mapsel = (k >> 8) & 3; }
                                i++; if(i >= w) { i = 0; j++; }
                            }
                            ret = 1;
                        break;
                        case 2: /* base64 */
                            while(*s && (*s == ' ' || *s == '\r' || *s == '\n')) s++;
                            if(!*s) break;
                            base64_decode((char*)s);
                            data = (uint32_t*)s;
readuints:                  for(i = j = 0; j < h; data++) {
                                k = (int)(*data); if(k > 0) k--;
                                if(i < 320 && k < 1024) { meg4.mmio.map[j * 320 + i] = k & 0xff; if(k) meg4.mmio.mapsel = (k >> 8) & 3; }
                                i++; if(i >= w) { i = 0; j++; }
                            }
                            if((uintptr_t)data != (uintptr_t)s) free(data);
                            ret = 1;
                        break;
                        case 3: /* gzip? */
                        case 4: /* zlib */
                            while(*s && (*s == ' ' || *s == '\r' || *s == '\n')) s++;
                            if(!*s) break;
                            i = base64_decode((char*)s);
                            /* makes no sense... there's no gzip compression, uses zlib, just with a different header */
                            if(s[0] == 0x78) { s += 2; i -= 2; } else
                            if(s[0] == 0x1F && s[1] == 0x8B) { s += 10; i -= 10; } else
                                break;
                            data = (uint32_t*)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)s, i, 4096, &i, 0);
                            if(data) goto readuints;
                        break;
                    }
                    if(!lvl) meg4_switchmode(MEG4_MODE_MAP);
                }
            }
        }
        goto end;
    }

    /* Scalable Screen Font */
    if(len > 32 && !memcmp(buf, "SFN2", 4) && buf[10] == 8 && buf[11] == 8) {
        main_log(1, "font (ssfn) detected");
        memset(meg4.font, 0, sizeof(meg4.font));
        for(s = buf + le32toh(*((uint32_t*)(buf + 16))), c = 0; c < 0x010000 && s < end; c++) {
            if(s[0] == 0xFF) { c += 65535; s++; } else
            if((s[0] & 0xC0) == 0xC0) { j = (((s[0] & 0x3F) << 8) | s[1]); c += j; s += 2; } else
            if((s[0] & 0xC0) == 0x80) { j = (s[0] & 0x3F); c += j; s++; } else {
                ptr = s + 6; o = meg4.font + c * 8;
                for(i = a = 0; i < s[1]; i++, ptr += s[0] & 0x40 ? 6 : 5) {
                    if(ptr[0] == 255 && ptr[1] == 255) continue;
                    frg = buf + (s[0] & 0x40 ? ((ptr[5] << 24) | (ptr[4] << 16) | (ptr[3] << 8) | ptr[2]) :
                        ((ptr[4] << 16) | (ptr[3] << 8) | ptr[2]));
                    if((frg[0] & 0xE0) != 0x80) continue;
                    o += (int)(ptr[1] - a); a = ptr[1]; k = ((frg[0] & 0x0F) + 1) << 3; j = frg[1] + 1; frg += 2;
                    for(b = 1; j; j--, a++, o++)
                        for(l = 0; l < k; l++, b <<= 1) {
                            if(b > 0x80) { frg++; b = 1; }
                            if(*frg & b) *o |= b;
                        }
                }
                s += 6 + s[1] * (s[0] & 0x40 ? 6 : 5);
            }
        }
        meg4_recalcfont(0, 0xffff);
        if(!lvl) meg4_switchmode(MEG4_MODE_FONT);
        goto end;
    }

    /* BDF font */
    if(len > 9 && !memcmp(buf, "STARTFONT", 9)) {
        main_log(1, "font (bdf) detected");
        memset(meg4.font, 0, sizeof(meg4.font));
        for(b = w = h = l = t = 0, s = buf, unicode = b = 0; s < end - 8 && *s; ) {
            if(!memcmp(s, "FONT_ASCENT ", 12)) { s += 12; b = atoi((char*)s); } else
            if(!memcmp(s, "ENCODING ", 9)) { s += 9; unicode = atoi((char*)s); } else
            if(!memcmp(s, "BBX ", 4)) {
                s += 4; w = atoi((char*)s); while(s < end && *s && *s!=' ') { s++; } while(s < end && *s==' ') s++;
                h = atoi((char*)s); while(s < end && *s && *s!=' ') { s++; } while(s < end && *s==' ') s++;
                l = atoi((char*)s); while(s < end && *s && *s!=' ') { s++; } while(s < end && *s==' ') s++;
                t = atoi((char*)s);
            } else
            if(!memcmp(s, "BITMAP", 6) && w > 0 && w <= 16 && h > 0 && h <= 16/* && w * h < (int)sizeof(bmp)*/) {
                s += 6; while(s < end && *s && *s!='\n') { s++; } while(s < end && (*s=='\n' || *s=='\r')) s++;
                memset(bmp, 0, sizeof(bmp));
                for(i = 0;i < w * h && s < end - 2 && *s; s += 2) {
                    while(s < end - 2 && (*s=='\n' || *s=='\r')) s++;
                    c = gethex(s, 2);
                    for(j=0x80,k=0;j && k < w && i < w * h;k++,j>>=1,i++) if(c & j) bmp[i] = 1;
                }
                bitmap(unicode, l, b - t - h, w, h, bmp);
                w = h = l = t = 0;
            }
            while(s < end && *s && *s != '\r' && *s != '\n') s++;
            while(s < end && (*s == '\r' || *s == '\n')) s++;
        }
        meg4_recalcfont(0, 0xffff);
        if(!lvl) meg4_switchmode(MEG4_MODE_FONT);
        goto end;
    }

    /* FontForge's SFD font */
    if(len > 12 && !memcmp(buf, "SplineFontDB", 12)) {
        main_log(1, "font (sfd) detected");
        memset(meg4.font, 0, sizeof(meg4.font));
        for(a = b = 0, s = buf; s < end - 8 && *s; ) {
            if(!memcmp(s, "BitmapFont: ", 12)) {
                s += 12; while(*s == ' ') s++;
                a = atoi((char*)s); while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                b = atoi((char*)s);
            } else
            if(!memcmp(s, "BDFChar:", 8)) {
                s += 8; while(*s == ' ') s++;
                while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                unicode = atoi((char*)s); while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                w = atoi((char*)s); while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                l = atoi((char*)s); while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                x = atoi((char*)s); while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                t = atoi((char*)s); while(s < end - 8 && *s && *s != ' ') { s++; } while(*s == ' ') s++;
                y = atoi((char*)s); while(s < end - 8 && *s && *s != '\n' && *s != '\r') s++;
                while(s < end - 8 && *s && (*s == '\n' || *s == '\r')) s++;
                h = a; x -= l - 1; y -= t - 1;
                x = (x + 7) & ~7;
                if(w > 0 && w <= 16 && h > 0 && h <= 16 && x > 0 && x <= 16 && y > 0 && y <= 16 && x * y < (int)sizeof(bmp)) {
                    memset(bmp, 0, sizeof(bmp));
                    for(i = 0, k = 4; i < x * y;) {
                        if(k > 3) {
                            if(s >= end - 8 || !*s || *s == '\r' || *s == '\n') break;
                            k = 0;
                            if(*s == 'z') { dec[0] = dec[1] = dec[2] = dec[3] = 0; s++; }
                            else {
                                c = ((((s[0]-'!')*85 + s[1]-'!')*85 + s[2]-'!')*85 + s[3]-'!')*85 + s[4]-'!';
                                dec[0] = (c >> 24) & 0xFF; dec[1] = (c >> 16) & 0xFF; dec[2] = (c >> 8) & 0xFF; dec[3] = c & 0xFF;
                                s += 5;
                            }
                        }
                        c = dec[k++];
                        for(j = 0x80; j; j >>= 1, i++) if(c & j) bmp[i] = 1;
                    }
                    bitmap(unicode, 0, 0, x, y, bmp);
                }
            }
            while(s < end && *s && *s != '\r' && *s != '\n') s++;
            while(s < end && (*s == '\r' || *s == '\n')) s++;
        }
        meg4_recalcfont(0, 0xffff);
        if(!lvl) meg4_switchmode(MEG4_MODE_FONT);
        goto end;
    }

    /* PC Screen Font */
    if(len > 32 && buf[0]==0x72 && buf[1]==0xB5 && buf[2]==0x4A && buf[3]==0x86 && buf[8]==32 && buf[24]==8 && buf[28]==8) {
        main_log(1, "font (psf) detected");
        memset(meg4.font, 0, sizeof(meg4.font));
        a = (buf[17] << 8) | buf[16];
        frg = buf + 32; ptr = frg + a * 8;
        if(ptr < end) {
            while(ptr < end) {
                c = ptr[0];
                if(c == 0xFF) { frg += 8; } else {
                    if((c & 128) != 0) {
                        if((c & 32) == 0 ) { c=((ptr[0] & 0x1F)<<6)+(ptr[1] & 0x3F); ptr++; } else
                        if((c & 16) == 0 ) { c=((((ptr[0] & 0xF)<<6)+(ptr[1] & 0x3F))<<6)+(ptr[2] & 0x3F); ptr+=2; } else
                        if((c & 8) == 0 ) { c=((((((ptr[0] & 0x7)<<6)+(ptr[1] & 0x3F))<<6)+(ptr[2] & 0x3F))<<6)+(ptr[3] & 0x3F); ptr+=3;}
                        else c=0;
                    }
                    if(c <= 0xffff)
                        for(o = meg4.font + c * 8, j = 0; j < 8; j++, o++)
                            for(l = 0; l < 8; l++)
                                if(frg[j] & (1 << l)) o[0] |= (0x80 >> l);
                }
                ptr++;
            }
        } else {
            for(o = meg4.font, i = 0; i < a; i++)
                for(j = 0; j < 8; j++, frg++, o++)
                    for(l = 0; l < 8; l++)
                        if(frg[0] & (1 << l)) o[0] |= (0x80 >> l);
        }
        meg4_recalcfont(0, 0xffff);
        if(!lvl) meg4_switchmode(MEG4_MODE_FONT);
        goto end;
    }

    /* RIFF Wave */
    if(len > 44 && !memcmp(buf, "RIFF", 4) && !memcmp(buf + 8, "WAVEfmt", 7) && (buf[20] == 1 || buf[20] == 3) &&
      /*mono or stereo*/(buf[22] == 1 || buf[22] == 2) && !buf[23] && !buf[35]) {
        ptr = buf + 12; while(memcmp(ptr, "data", 4) && ptr < end) { ptr += ((ptr[5]<<8)|ptr[4]) + 8; }
        if(ptr < end && !memcmp(ptr, "data", 4)) {
            memset(tmp, 0, sizeof(tmp));
            ptr += 4; l = (ptr[3]<<24)|(ptr[2]<<16)|(ptr[1]<<8)|ptr[0]; if(end > ptr + l + 4) end = ptr + l + 4;
            ptr += 4;
            c = buf[22];                                                                        /* number of channels */
            l = w = (int)((buf[31]<<24)|(buf[30]<<16)|(buf[29]<<8)|buf[28])/c/(buf[34]>>3);     /* number of samples */
            if(l > (int)sizeof(meg4.waveforms[0]) - 8) l = (int)sizeof(meg4.waveforms[0]) - 8;
            for(i = 0; i < l && ptr < end; i++)
                for(k = y = x = 0; i + k < l && i * l / w == (i + k) * l / w && ptr < end; k++) {
                    for(j = 0; j < c && ptr < end; j++)
                        switch(buf[34]) {
                            case 8: y += *((int8_t*)ptr++) - 128; x++; break;
                            case 16: ptr++; y += *((int8_t*)ptr++); x++; break;
                            case 24: ptr += 2; y += *((int8_t*)ptr++); x++; break;
                            case 32:
                                if(buf[20] == 1) { ptr += 3; y += *((int8_t*)ptr++); }
                                else { y += (int)((float)((ptr[3]<<24)|(ptr[2]<<16)|(ptr[1]<<8)|ptr[0]) * 127.0f); ptr += 4; }
                                x++;
                            break;
                        }
                    tmp[8 + i] = x ? y / x : 0; x = 0;
                }
            if(i < l && x) tmp[8 + i] = y / x;
            tmp[0] = l & 0xff; tmp[1] = (l >> 8) & 0xff; tmp[7] = 64;
            /* if filename is "dspXX.wav", then load at XX, otherwise at first empty slot */
            if(name && strlen(name) == 9 && !memcmp(name, "dsp", 3) && name[5] == '.' && (i = gethex((uint8_t*)name + 3, 2)) > 0 && i < 32)
                memcpy(&meg4.waveforms[i][0], tmp, sizeof(meg4.waveforms[0]));
            else
                i = waveform((uint8_t*)tmp);
            if(i) main_log(1, "waveform %u (wav) detected", i);
            else main_log(1, "waveform detected, but no free slots");
            if(i && !lvl) { meg4_switchmode(MEG4_MODE_SOUND); sound_addwave(i); } else ret = 0;
        } else ret = 0;
        goto end;
    }

    /* General MIDI */
    if(len >= 23 && !memcmp(buf, "MThd", 4)) {
        /* no place to store track number in MIDI, so always find the first free slot */
        for(i = 0; i < (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])) && !meg4_isbyte(meg4.tracks[i], 0, sizeof(meg4.tracks[0])); i++);
        if(i >= (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0]))) {
            main_log(1, "music track (midi) detected, but no free slots");
            ret = 0; goto end;
        }
        main_log(1, "music track %u (midi) detected", i);
        if((ret = format_mid(i, buf, len)) && !lvl) meg4_switchmode(MEG4_MODE_MUSIC);
        goto end;
    }

    /* Amiga MOD */
    if(len > 1084 && !memcmp(buf + 1080, "M.K.", 4)) {
        i = gethex(buf + 11, 2);
        if(!memcmp(buf, "MEG-4 SFX", 9)) {
            main_log(1, "sounds detected");
            if((ret = format_mod(-1, buf, len)) && !lvl) meg4_switchmode(MEG4_MODE_SOUND);
        } else {
            /* if song name does not tell the track number, find the first free slot */
            if(memcmp(buf, "MEG-4 TRACK", 11) || i < 0 || i >= (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])))
                for(i = 0; i < (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])) && !meg4_isbyte(meg4.tracks[i], 0, sizeof(meg4.tracks[0])); i++);
            if(i < 0 || i >= (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0]))) {
                main_log(1, "music track detected, but no valid track id and no free slots either");
                ret = 0; goto end;
            }
            main_log(1, "music track %u (mod) detected", i);
            if((ret = format_mod(i, buf, len)) && !lvl) meg4_switchmode(MEG4_MODE_MUSIC);
        }
        goto end;
    }

    /* sprites or map in indexed TGA (sadly no real magic, so check for this as last) */
    if(len > 18 && !buf[0] && buf[1] == 1 && (buf[2] == 1 || buf[2] == 9) && !buf[3] && !buf[6] &&
      (buf[7] == 24 || buf[7] == 32)) {
        t = (buf[11] << 8) + buf[10]; w = (buf[13] << 8) + buf[12]; h = (buf[15] << 8) + buf[14];
        s = buf + 18; j = buf[7] >> 3; e = NULL;
        if(w == 256 && h == 256 && !buf[4]) {
            /* if it's a sprite image */
            main_log(1, "sprites (tga) detected");
            memset(meg4.mmio.palette, 0, sizeof(meg4.mmio.palette));
            memset(meg4.mmio.sprites, 0, sizeof(meg4.mmio.sprites));
            /* palette */
            for(i = 0; i < buf[5] && i < 256; i++, s += j)
                meg4.mmio.palette[i] = ((j > 3 ? s[3] : 0xff) << 24) | (s[2] << 16) | (s[1] << 8) | s[0];
            e = meg4.mmio.sprites;
        } else
        if(w == 320 && h == 200 && buf[4] < 4) {
            /* if it's a map */
            main_log(1, "map (tga) detected");
            memset(meg4.mmio.map, 0, sizeof(meg4.mmio.map));
            meg4.mmio.mapsel = buf[4];
            e = meg4.mmio.map;
        }
        if(e) {
            s = buf + 18 + buf[5] * j;
            /* uncompressed indexed image */
            if(buf[2] == 1) { for(y = 0; y < h; y++) memcpy(e + y * w, s + ((!t ? h - y - 1 : y) * w), w); } else
            /* compressed indexed image */
            if(buf[2] == 9) {
                for(x = y = i = 0, l = w * h; x < l && s < end;) {
                    k = *s++;
                    if(k > 127) {
                        k -= 127; x += k; j = *s++;
                        while(k-- && i < l) {
                            if(!(i % w)) { i = ((!t ? h - y - 1 : y) * w); y++; }
                            if(i < l) e[i++] = j;
                        }
                    } else {
                        k++; x += k;
                        while(k-- && i < l && s < end) {
                            if(!(i % w)) { i = ((!t ? h - y - 1 : y) * w); y++; }
                            if(i < l) e[i++] = *s++;
                        }
                    }
                }
            }
            if(!lvl) meg4_switchmode(w == 256 ? MEG4_MODE_SPRITE : MEG4_MODE_MAP);
            goto end;
        }
    }

    ret = 0;
end:if(uncomp) free(uncomp);
    meg4_recalcmipmap();
    return ret;
}

/**
 * MEG-4 Theme (in a GIMP Palette file)
 */
int meg4_theme(uint8_t *buf, int len)
{
    uint8_t *ptr;
    int i;

    if(!buf || len < 12 || memcmp(buf, "GIMP Palette", 12)) return 0;
    memset(theme, 0, sizeof(theme));
    /* skip over header */
    for(ptr = buf; ptr < buf + len && (*ptr < '0' || *ptr > '9'); ptr++) {
        while(ptr < buf + len && *ptr != '\n') ptr++;
        while(ptr < buf + len && (*ptr == ' ' || *ptr == '\r' || *ptr == '\n')) ptr++;
    }
    /* read in color RGB triplets */
    for(i = 0; i < (int)(sizeof(theme)/sizeof(theme[0])) && ptr < buf + len; i++) {
        theme[i] = 0xff000000;
        while(ptr < buf + len && *ptr == ' ') ptr++;
        theme[i] |= (atoi((char*)ptr) & 0xff); while(ptr < buf + len && *ptr >= '0' && *ptr <= '9') ptr++;
        while(ptr < buf + len && *ptr == ' ') ptr++;
        theme[i] |= (atoi((char*)ptr) & 0xff) << 8; while(ptr < buf + len && *ptr >= '0' && *ptr <= '9') ptr++;
        while(ptr < buf + len && *ptr == ' ') ptr++;
        theme[i] |= (atoi((char*)ptr) & 0xff) << 16; while(ptr < buf + len && *ptr >= '0' && *ptr <= '9') ptr++;
        while(ptr < buf + len && *ptr != '\n') ptr++;
        while(ptr < buf + len && (*ptr == '\r' || *ptr == '\n')) ptr++;
    }
    return 1;
}
