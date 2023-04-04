/*
 * meg4/editors/zip.c
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
 * @brief PKZIP functions
 *
 */

#include <time.h>
#include "../meg4.h"
#include "zip.h"

/**
 * Variables to hold zip data
 */
static zip_central_t *cen = NULL;
static char **names = NULL;
static uint8_t *zip = NULL;
static int numfiles = 0, zipsize = 0, fnsize = 0;
static uint32_t mtime = 0;

/**
 * Free resources
 */
static void zip_free(void)
{
    int i;
    if(cen) { free(cen); cen = NULL; }
    if(names) {
        for(i = 0; i < numfiles; i++)
            if(names[i]) free(names[i]);
        free(names); names = NULL;
    }
    numfiles = zipsize = fnsize = 0; mtime = 0;
}

/**
 * Open zip
 */
void zip_open(void)
{
    struct tm *tm;
    time_t t;

    zip_free();
    if(zip) { free(zip); zip = NULL; }
    t = time(NULL);
    if((tm = gmtime(&t)))
        mtime = (((tm->tm_year - 80) & 0x7f) << 25) | ((tm->tm_mon + 1) << 21) | (tm->tm_mday << 16) | (tm->tm_hour << 11) |
            (tm->tm_min << 5) | (tm->tm_sec >> 1);
}

/**
 * Add a file to zip
 */
void zip_add(char *fn, uint8_t *buf, int len)
{
    uint32_t magic = htole32(ZIP_LOCAL_MAGIC);
    zip_central_t *hdr;
    uint8_t *comp;
    int l, siz;

    if(!fn || !*fn || !buf || len < 1) return;
    cen = (zip_central_t*)realloc(cen, (numfiles + 1) * sizeof(zip_central_t));
    names = (char**)realloc(names, (numfiles + 1) * sizeof(char**));
    if(!cen || !names) { zip_free(); if(zip) { free(zip); } zip = NULL; return; }
    l = strlen(fn);
    names[numfiles] = (char*)malloc(l + 1);
    if(!names[numfiles]) return;
    strcpy(names[numfiles], fn);
    comp = stbi_zlib_compress(buf, len, &siz, 9);
    if(comp && siz > len) { free(comp); comp = NULL; }
    if(!comp) { comp = buf - 2; siz = len + 2; }
    zip = (uint8_t*)realloc(zip, zipsize + sizeof(zip_local_t) + l + siz);
    if(!zip) { zip_free(); if(comp != buf - 2) { free(comp); } return; }
    memset(&cen[numfiles], 0, sizeof(zip_central_t));
    hdr = &cen[numfiles];
    hdr->magic = htole32(ZIP_CENTRAL_MAGIC);
    hdr->version = htole16(10);
    hdr->method = htole16(siz < len + 2 ? ZIP_DATA_DEFLATE : ZIP_DATA_STORE);
    hdr->mtime = htole32(mtime);
    hdr->crc32 = htole32(stbiw__crc32(buf, len));
    hdr->comp = htole32(siz - 2);
    hdr->orig = htole32(len);
    hdr->fnlen = htole16(l);
    hdr->eattr = htole32(0100000 << 16);
    hdr->offs = htole32(zipsize);
    memcpy(zip + zipsize, &magic, 4); zipsize += 4;
    memcpy(zip + zipsize, (uint8_t*)hdr + 6, sizeof(zip_local_t) - 4); zipsize += sizeof(zip_local_t) - 4;
    hdr->magic = htole32(ZIP_CENTRAL_MAGIC);
    memcpy(zip + zipsize, fn, l); zipsize += l;
    memcpy(zip + zipsize, comp + 2, siz - 2); zipsize += siz - 2;
    if(comp != buf - 2) free(comp);
    fnsize += l;
    numfiles++;
}

/**
 * Close and return archive
 */
uint8_t *zip_close(int *len)
{
    zip_eocd32_t eoc = { 0 };
    uint8_t *ret = zip;
    int i, l, s;

    zip = (uint8_t*)realloc(zip, zipsize + sizeof(zip_eocd32_t) + numfiles * sizeof(zip_central_t) + fnsize);
    if(!zip) { zip_free(); return NULL; }
    eoc.magic = htole32(ZIP_EOCD32_MAGIC);
    eoc.totdisk = htole16(1);
    eoc.nument = htole16(numfiles);
    eoc.totoffs = htole32(zipsize);
    for(s = zipsize, i = 0; i < numfiles; i++) {
        memcpy(zip + zipsize, &cen[i], sizeof(zip_central_t)); zipsize += sizeof(zip_central_t);
        l = strlen(names[i]); memcpy(zip + zipsize, names[i], l); zipsize += l;
    }
    eoc.cdsize = htole32(zipsize - s);
    memcpy(zip + zipsize, &eoc, sizeof(zip_eocd32_t)); zipsize += sizeof(zip_eocd32_t);
    if(len) *len = zipsize;
    zip_free();
    ret = zip; zip = NULL;
    return ret;
}

