/*
 * meg4/editors/fmt_tic80.h
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
 * @brief Parse a TIC-80 cartridge
 *
 */

/* -------- stuff needed to decrypt a TIC-80 png cartridge (see https://github.com/nesbox/TIC-80, src/ext/png.c) -------- */
typedef union {
    struct { uint32_t bits:8; uint32_t size:24; } st;
    uint8_t data[4];
} TicHeader_t;
#define BITS_IN_BYTE 8
#define HEADER_BITS 4
#define HEADER_SIZE ((int)sizeof(TicHeader_t) * BITS_IN_BYTE / HEADER_BITS)
#define MIN(a,b)            ((a) < (b) ? (a) : (b))
#define MAX(a,b)            ((a) > (b) ? (a) : (b))
#define CLAMP(v,a,b)        (MIN(MAX(v,a),b))
#define BITCHECK(a,b)       (!!((a) & (1ULL<<(b))))
#define _BITSET(a,b)        ((a) |= (1ULL<<(b)))
#define _BITCLEAR(a,b)      ((a) &= ~(1ULL<<(b)))
static void bitcpy(uint8_t* dst, uint32_t to, const uint8_t* src, uint32_t from, uint32_t size) {
    uint32_t i;
    for(i = 0; i < size; i++, to++, from++)
        BITCHECK(src[from >> 3], from & 7) ? _BITSET(dst[to >> 3], to & 7) : _BITCLEAR(dst[to >> 3], to & 7);
}
static int32_t ceildiv(int32_t a, int32_t b) { return (a + b - 1) / b; }
/* -------- TIC-80 png stuff end -------- */

/* defaults for TIC-80 */
static const uint8_t Sweetie16[] = {0x1a, 0x1c, 0x2c, 0x5d, 0x27, 0x5d, 0xb1, 0x3e, 0x53, 0xef, 0x7d, 0x57, 0xff, 0xcd, 0x75, 0xa7, 0xf0, 0x70, 0x38, 0xb7, 0x64, 0x25, 0x71, 0x79, 0x29, 0x36, 0x6f, 0x3b, 0x5d, 0xc9, 0x41, 0xa6, 0xf6, 0x73, 0xef, 0xf7, 0xf4, 0xf4, 0xf4, 0x94, 0xb0, 0xc2, 0x56, 0x6c, 0x86, 0x33, 0x3c, 0x57};
static const uint8_t Waveforms[] = {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};

/* TIC-80 chunk types */
enum
{
    CHUNK_DUMMY,        /* 0 */
    CHUNK_TILES,        /* 1 */
    CHUNK_SPRITES,      /* 2 */
    CHUNK_COVER_DEP,    /* 3 - deprecated chunk */
    CHUNK_MAP,          /* 4 */
    CHUNK_CODE,         /* 5 */
    CHUNK_FLAGS,        /* 6 */
    CHUNK_TEMP2,        /* 7 */
    CHUNK_TEMP3,        /* 8 */
    CHUNK_SAMPLES,      /* 9 */
    CHUNK_WAVEFORM,     /* 10 */
    CHUNK_TEMP4,        /* 11 */
    CHUNK_PALETTE,      /* 12 */
    CHUNK_PATTERNS_DEP, /* 13 - deprecated chunk */
    CHUNK_MUSIC,        /* 14 */
    CHUNK_PATTERNS,     /* 15 */
    CHUNK_CODE_ZIP,     /* 16 */
    CHUNK_DEFAULT,      /* 17 */
    CHUNK_SCREEN,       /* 18 */
    CHUNK_BINARY,       /* 19 */
    CHUNK_LANG          /* 20 */
};

/**
 * Import a TIC-80 file
 */
static int format_tic80(uint8_t *buf, int len)
{
    uint8_t *ptr, *end, *pat = NULL, *mus = NULL;
    int i, j, k, l, m, type, bank, size = 0, pat_size = 0, mus_size = 0;

    if(!buf || len < 4) return 0;

    meg4.src_len = 0; memset(meg4.src_bm, 0, sizeof(meg4.src_bm));
    meg4.src = (char*)realloc(meg4.src, 8 + 8 * 65536);
    if(!meg4.src) return 0;
    memset(meg4.src, 0, 8 + 8 * 65536);
    memcpy(meg4.src, "#!lua\n\n", 7);

    for(end = buf + len; buf + 4 < end; buf += size) {
        /* header */
        type = buf[0] & 0x1f; bank = buf[0] >> 5; size = (buf[2] << 8) | buf[1]; buf += 4;
        if(!size && (type == CHUNK_CODE || type == CHUNK_BINARY)) size = 65536;
        /* not sure what to do with these bank stuff... */
        if(type != CHUNK_CODE && bank > 0) continue;
        /* data */
        switch(type) {
            case CHUNK_DEFAULT:
                memset(meg4.mmio.palette, 0, sizeof(meg4.mmio.palette));
                for(i = 0; i < 16; i++)
                    meg4.mmio.palette[i] = (0xff << 24) | (Sweetie16[i * 3 + 2] << 16) | (Sweetie16[i * 3 + 1] << 8) | Sweetie16[i * 3 + 0];
                memset(meg4.waveforms, 0, sizeof(meg4.waveforms));
                for(j = 0; j < (int)(sizeof(Waveforms)/sizeof(Waveforms[0])/16); j++) {
                    meg4.waveforms[j][0] = 32;
                    meg4.waveforms[j][7] = 0xff;
                    for(i = 0; i < 16; i++) {
                        meg4.waveforms[j][i * 2 + 8] = Waveforms[j * 16 + i] & 0xf0;
                        meg4.waveforms[j][i * 2 + 9] = (Waveforms[j * 16 + i] & 0x0f) << 4;
                    }
                }
            break;
            case CHUNK_PALETTE:
                memset(meg4.mmio.palette, 0, sizeof(meg4.mmio.palette));
                for(i = 0; i < (size > 768 ? 768 : size) / 3; i++)
                    meg4.mmio.palette[i] = (0xff << 24) | (buf[i * 3 + 2] << 16) | (buf[i * 3 + 1] << 8) | buf[i * 3 + 0];
            break;
            case CHUNK_WAVEFORM:
                memset(meg4.waveforms, 0, sizeof(meg4.waveforms));
                for(j = 0; j < (size > 31 * 16 ? 31 : size / 16); j++) {
                    meg4.waveforms[j][0] = 32;
                    meg4.waveforms[j][7] = 0xff;
                    for(i = 0; i < 16; i++) {
                        meg4.waveforms[j][i * 2 + 8] = buf[j * 16 + i] & 0xf0;
                        meg4.waveforms[j][i * 2 + 9] = (buf[j * 16 + i] & 0x0f) << 4;
                    }
                }
            break;
            case CHUNK_TILES:
            case CHUNK_SPRITES:
                k = type == CHUNK_SPRITES ? 16384 : 0;
                memset(meg4.mmio.sprites + k, 0, 16384);
                memset(meg4.mmio.sprites + 32768, 0, 32768);
                /* TIC-8 stores the sprites as an array, each 32 bytes, separate 8 x 8 x 4 bit images */
                for(j = 0, ptr = buf; j < size / 32; j++) {
                    l = k + (j >> 5) * 256 * 8 + ((j & 31) << 3);
                    for(i = 0; i < 8; i++, ptr += 4)
                        for(m = 0; m < 4; m++) {
                            meg4.mmio.sprites[l + i * 256 + m * 2 + 0] = ptr[m] & 0xf;
                            meg4.mmio.sprites[l + i * 256 + m * 2 + 1] = (ptr[m] & 0xf0) >> 4;
                        }
                }
            break;
            case CHUNK_MAP:
                memset(meg4.mmio.map, 0, sizeof(meg4.mmio.map));
                /* TIC-80 map size is 240 x 136 x 8 bit */
                for(i = 0; i < 240 * 136; i++)
                    meg4.mmio.map[(i / 240) * 320 + (i % 240)] = buf[i];
            break;
            case CHUNK_SAMPLES:
                /* TODO: sound samples */
            break;
            case CHUNK_PATTERNS: pat = buf; pat_size = size; break;
            case CHUNK_MUSIC: mus = buf; mus_size = size; break;
            case CHUNK_CODE:
                memcpy(meg4.src + bank * 65536 + 7, buf, size);
            break;
        }
    }
    meg4.src_len = strlen(meg4.src) + 1;
    /* decode music tracks */
    if(pat && pat_size > 0 && mus && mus_size > 0) {
        /* TODO: music tracks */
    }
    return 1;
}
