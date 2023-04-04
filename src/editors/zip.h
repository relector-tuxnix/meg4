/*
 * meg4/editors/zip.h
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
 * @brief PKZIP definitions
 *
 */

#include <stdint.h>

unsigned char *stbi_zlib_compress(unsigned char *data, int data_len, int *out_len, int quality);
unsigned int stbiw__crc32(unsigned char *buffer, int len);

#define ZIP_DATA_STORE      0
#define ZIP_DATA_DEFLATE    8
#define ZIP_LOCAL_MAGIC     0x04034b50
#define ZIP_CENTRAL_MAGIC   0x02014b50
#define ZIP_EOCD32_MAGIC    0x06054b50
typedef struct {
    uint32_t    magic;
    uint16_t    version;
    uint16_t    flags;
    uint16_t    method;
    uint32_t    mtime;
    uint32_t    crc32;
    uint32_t    comp;
    uint32_t    orig;
    uint16_t    fnlen;
    uint16_t    extlen;
} __attribute__((packed)) zip_local_t;
typedef struct {
    uint32_t    magic;
    uint16_t    madeby;
    uint16_t    version;
    uint16_t    flags;
    uint16_t    method;
    uint32_t    mtime;
    uint32_t    crc32;
    uint32_t    comp;
    uint32_t    orig;
    uint16_t    fnlen;
    uint16_t    extlen;
    uint16_t    comlen;
    uint16_t    disk;
    uint16_t    iattr;
    uint32_t    eattr;
    uint32_t    offs;
} __attribute__((packed)) zip_central_t;
typedef struct {
    uint32_t    magic;
    uint16_t    numdisk;
    uint16_t    strtdisk;
    uint16_t    totdisk;
    uint16_t    nument;
    uint32_t    cdsize;
    uint32_t    totoffs;
    uint16_t    comlen;
} __attribute__((packed)) zip_eocd32_t;

void zip_open(void);
void zip_add(char *fn, uint8_t *buf, int len);
uint8_t *zip_close(int *len);
