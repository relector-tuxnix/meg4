/*
 * meg4/floppy.h
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
 * @brief Low level floppy encoding / decoding functions
 *
 */

/* serialization chunks */
enum { MEG4_CHUNK_META, MEG4_CHUNK_DATA, MEG4_CHUNK_CODE, MEG4_CHUNK_PAL, MEG4_CHUNK_SPRITES, MEG4_CHUNK_MAP, MEG4_CHUNK_FONT,
    MEG4_CHUNK_WAVE, MEG4_CHUNK_SFX, MEG4_CHUNK_TRACK, MEG4_CHUNK_OVL };

/**
 * Check if a buffer contains only a specific byte
 */
int meg4_isbyte(void *buf, uint8_t byte, int len)
{
    uint8_t *ptr = (uint8_t*)buf;
    int i;
    if(!buf || len < 1) return 0;
    for(i = 0; i < len && ptr[i] == byte; i++);
    return i >= len;
}

/**
 * Decode bytes
 */
static int meg4_rle_dec(uint8_t *inbuff, int inlen, uint8_t *outbuff, int outlen)
{
    int l, o = 0;
    uint8_t *end = inbuff + inlen;

    if(!inbuff || inlen < 2 || !outbuff || outlen < 2) return 0;
    while(inbuff < end && o < outlen) {
        l = ((*inbuff++) & 0x7F) + 1;
        if(inbuff[-1] & 0x80) {
            while(l--) outbuff[o++] = *inbuff;
            inbuff++;
        } else while(l--) outbuff[o++] = *inbuff++;
    }
    return o;
}

/**
 * Deserialize
 */
static int meg4_deserialize(uint8_t *buf, int len)
{
    /* ret is checked in load_insert() in editors/load.c */
    int ret = 1;
    uint32_t s, e, i, j, *d, fwver = 0;
    uint8_t *end, *ptr;
    int8_t pack;

    if(!buf || len < 4) return 0;
    for(end = buf + len; buf < end; buf += s) {
        s = ((buf[3] << 16) | (buf[2] << 8) | buf[1]) - 4; buf += 4; if(buf + s > end) s = end - buf;
        switch(buf[-4]) {

            case MEG4_CHUNK_META:
                memcpy(&fwver, buf, 3);
                /* we could check the firmware version of the console that saved this floppy, but
                 * for now that's pointless, there are no revision differences yet */
                (void)fwver;
                if(s >= 4 + (uint32_t)sizeof(meg4_title))
                    memcpy(meg4_title, buf + 4, sizeof(meg4_title));
                if(meg4_pro && s >= 4 + (uint32_t)sizeof(meg4_title) + (uint32_t)sizeof(meg4_author))
                    memcpy(meg4_author, buf + 4 + sizeof(meg4_title), sizeof(meg4_author));
            break;

            case MEG4_CHUNK_DATA:
                meg4.dp = s > sizeof(meg4.data) ? sizeof(meg4.data) : s;
                memcpy(meg4.data, buf, meg4.dp);
            break;

            case MEG4_CHUNK_CODE:
                meg4.src_len = meg4.code_len = 0;
                if(buf[0] == '#' && buf[1] == '!') {
#ifndef NOEDITORS
                    meg4.src = (char*)realloc(meg4.src, s + 1);
                    if(meg4.src) {
                        for(i = 0; i < s && buf[i]; i++)
                            if(buf[i] >= ' ' || buf[i] == '\t' || buf[i] == '\n') meg4.src[meg4.src_len++] = buf[i];
                        meg4.src[meg4.src_len++] = 0;
                    }
#endif
                } else {
                    meg4.src = NULL;
                    meg4.code = (uint32_t*)realloc(meg4.code, (s + 2) & ~3);
                    if(meg4.code) {
                        meg4.code_type = *buf;
                        meg4.code_len = (s + 2) >> 2;
                        for(d = (uint32_t*)(buf + 1), i = 0; i < meg4.code_len; i++, d++)
                            meg4.code[i] = le32toh(*d);
                    }
                }
            break;

            case MEG4_CHUNK_PAL:
                memcpy(meg4.mmio.palette, buf, s < sizeof(meg4.mmio.palette) ? s : sizeof(meg4.mmio.palette));
            break;

            case MEG4_CHUNK_SPRITES:
                ret |= 2;
                meg4_rle_dec(buf, s, meg4.mmio.sprites, sizeof(meg4.mmio.sprites));
            break;

            case MEG4_CHUNK_MAP:
                ret |= 4;
                meg4.mmio.mapsel = buf[0] & 3;
                meg4_rle_dec(buf + 1, s - 1, meg4.mmio.map, sizeof(meg4.mmio.map));
            break;

            case MEG4_CHUNK_FONT:
                ret |= 8;
                for(i = 0, ptr = buf; ptr < buf + s && i < 65536;) {
                    pack = *ptr++;
                    if(pack >= 0) {
                        j = pack + 1;
                        memcpy(meg4.font + (i << 3), ptr, ((i + j < 65536) ? j : 65536 - i) << 3);
                        i += j; ptr += j << 3;
                    } else
                        i += -pack;
                }
                meg4_recalcfont(0, 0xffff);
            break;

            case MEG4_CHUNK_WAVE:
                /* for waves, the index is one bigger as wave 00 means "use previous waveform" */
                if(s > 14 && buf[0]) {
                    i = buf[0] - 1;
                    if(i < (uint32_t)(sizeof(meg4.waveforms)/sizeof(meg4.waveforms[0])) && buf[1] == 0) {
                        memset(meg4.waveforms[i], 0, sizeof(meg4.waveforms[0]));
                        /* buf[1] is the format, only 8-bit mono supported for now, so convert other formats to native format */
                        e = (s - 2) < sizeof(meg4.waveforms[0]) ? (s - 2) : sizeof(meg4.waveforms[0]);
                        switch(buf[1]) {
                            case 0: /* 8-bit mono */
                                memcpy(meg4.waveforms[i], buf + 2, e);
                            break;
                            case 1: /* 8-bit stereo */
                                memcpy(meg4.waveforms[i], buf + 2, 8); e -= 8;
                                for(j = 0; j < e; j += 2)
                                    meg4.waveforms[i][8 + (j >> 1)] = ((int)(int8_t)buf[10 + j] + (int)(int8_t)buf[11 + j]) / 2;
                            break;
                            case 2: /* 16-bit mono */
                                memcpy(meg4.waveforms[i], buf + 2, 8); e -= 8;
                                for(j = 0; j < e; j += 2)
                                    meg4.waveforms[i][8 + (j >> 1)] = buf[11 + j];
                            break;
                            case 3: /* 16-bit stereo */
                                memcpy(meg4.waveforms[i], buf + 2, 8); e -= 8;
                                for(j = 0; j < e; j += 4)
                                    meg4.waveforms[i][8 + (j >> 2)] = ((int)(int8_t)buf[11 + j] + (int)(int8_t)buf[13 + j]) / 2;
                            break;
                        }
                    }
                }
            break;

            case MEG4_CHUNK_SFX:
                ret |= 16;
                memcpy(meg4.mmio.sounds, buf, s < sizeof(meg4.mmio.sounds) ? s : sizeof(meg4.mmio.sounds));
            break;

            case MEG4_CHUNK_TRACK:
                if(s > 1) {
                    ret |= 32;
                    i = buf[0];
                    if(i < (uint32_t)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])))
                        memcpy(meg4.tracks[i], buf + 1, (s - 1) < sizeof(meg4.tracks[0]) ? (s - 1) : sizeof(meg4.tracks[0]));
                }
            break;

            case MEG4_CHUNK_OVL:
                if(s > 1) {
                    ret |= 64;
                    i = buf[0];
                    if(s > (uint32_t)sizeof(meg4.data) + 1) s = (uint32_t)sizeof(meg4.data) + 1;
                    meg4.ovls[i].size = s - 1;
                    meg4.ovls[i].data = (uint8_t*)realloc(meg4.ovls[i].data, s - 1);
                    if(meg4.ovls[i].data) memcpy(meg4.ovls[i].data, buf + 1, s - 1);
                    else meg4.ovls[i].size = 0;
                }
            break;
        }
    }
    /* if there was no palette in the data */
    if(meg4_isbyte(meg4.mmio.palette, 0, sizeof(meg4.mmio.palette)))
        memcpy(meg4.mmio.palette, default_pal, sizeof(meg4.mmio.palette));
    meg4_recalcmipmap();
    return ret;
}

#ifndef NOEDITORS
/**
 * Encode bytes
 */
static uint8_t *meg4_rle_enc(uint8_t *inbuff, int inlen, int *outlen)
{
    uint8_t *outbuff;
    int i, k, l, o;

    if(!inbuff || inlen < 1 || !outlen) return NULL;
    outbuff = (uint8_t *)malloc(2 * inlen + 1); if(!outbuff) return NULL;
    k = o = 0; outbuff[o++] = 0;
    for(i = 0; i < inlen; i++) {
        for(l = 1; l < 128 && i + l < inlen && inbuff[i] == inbuff[i + l]; l++);
        if(l > 1) {
            l--; if(outbuff[k]) { outbuff[k]--; outbuff[o++] = 0x80 | l; } else outbuff[k] = 0x80 | l;
            outbuff[o++] = inbuff[i]; k = o; outbuff[o++] = 0; i += l; continue;
        }
        outbuff[k]++; outbuff[o++] = inbuff[i];
        if(outbuff[k] > 127) { outbuff[k]--; k = o; outbuff[o++] = 0; }
    }
    if(!(outbuff[k] & 0x80)) { if(outbuff[k]) outbuff[k]--; else o--; }
    *outlen = o;
    return outbuff;
}

/**
 * Serialize
 */
uint8_t *meg4_serialize(int *len, int type)
{
    int i, j, sprsiz = 0, mapsiz = 0, fontsiz = 0, sndsiz = 0, siz = 4 + 4 + sizeof(meg4_title) + sizeof(meg4_author) +
        (type ? (meg4.code && meg4.code_len > 0 ? 5 + meg4.code_len * 4 : 0) + (meg4_init_len > 0 ? 4 + meg4_init_len : 0) :
        (meg4.src && meg4.src_len > 0 && meg4.src[0] == '#' && meg4.src[1] == '!' ? 4 + meg4.src_len : 0));
    int trksiz[sizeof(meg4.tracks)/sizeof(meg4.tracks[0])] = { 0 };
    uint8_t *ret, *ptr, *spr = NULL, *map = NULL;
    uint32_t hdr, *d;

    /* count to total required size */
    if(memcmp(meg4.mmio.palette, default_pal, sizeof(meg4.mmio.palette))) {
        siz += 4 + sizeof(meg4.mmio.palette);
    }
    if(!meg4_isbyte(meg4.mmio.sprites, 0, sizeof(meg4.mmio.sprites))) {
        spr = meg4_rle_enc(meg4.mmio.sprites, sizeof(meg4.mmio.sprites), &sprsiz); if(spr) siz += 4 + sprsiz;
    }
    if(!meg4_isbyte(meg4.mmio.map, 0, sizeof(meg4.mmio.map))) {
        map = meg4_rle_enc(meg4.mmio.map, sizeof(meg4.mmio.map), &mapsiz); if(map) siz += 5 + mapsiz;
    }
    if(memcmp(meg4.font + 8 * 32, meg4_font + 8 * 32, 8 * 65504)) {
        for(i = 0; i < 65536; i += j) {
            for(j = 0; j < 127 && (i + j) < 65536 && !meg4_isbyte(meg4.font + ((i + j) << 3), 0, 8); j++);
            if(j) { fontsiz += 1 + (j << 3); }
            else { for(j = 0; j < 128 && (i + j) < 65536 && meg4_isbyte(meg4.font + ((i + j) << 3), 0, 8); j++){}; fontsiz++; }
        }
        siz += 4 + fontsiz;
    }
    for(i = 0; i < (int)(sizeof(meg4.waveforms)/sizeof(meg4.waveforms[0])); i++)
        if(meg4.waveforms[i][0] || meg4.waveforms[i][1]) siz += 6 + 8 + ((meg4.waveforms[i][1] << 8) | meg4.waveforms[i][0]);
    for(i = 0; i < (int)(sizeof(meg4.mmio.sounds)/sizeof(meg4.mmio.sounds[0])/4); i++)
        if(meg4.mmio.sounds[i * 4]) sndsiz = (i + 1) * 4;
    if(sndsiz) siz += 4 + sndsiz;
    for(j = 0; j < (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])); j++) {
        for(i = 0; i < (int)(sizeof(meg4.tracks[0])>>4); i++)
            if(!meg4_isbyte(meg4.tracks[j] + i * 16, 0, 16))
                trksiz[j] = (i + 1) * 16;
        if(trksiz[j]) siz += 5 + trksiz[j];
    }
    for(i = 0; i < 256; i++)
        if(meg4.ovls[i].data && meg4.ovls[i].size > 0) siz += 5 + meg4.ovls[i].size;
    ret = ptr = (uint8_t*)malloc(siz);
    if(!ret) { if(spr) { free(spr); } if(map) { free(map); } return NULL; }
    memset(ret, 0, siz);

    /* meta info */
    hdr = htole32(((4 + 4 + sizeof(meg4_title) + sizeof(meg4_author)) << 8) | MEG4_CHUNK_META);
    memcpy(ptr, &hdr, 4); ptr += 4;
    memcpy(ptr, meg4.mmio.fwver, 3); ptr += 4;
    memcpy(ptr, meg4_title, strlen(meg4_title)); ptr += sizeof(meg4_title);
    if(meg4_pro) memcpy(ptr, meg4_author, strlen(meg4_author));
    ptr += sizeof(meg4_author);

    if(!type) {
        /* code source */
        if(meg4.src && meg4.src_len > 0 && meg4.src[0] == '#' && meg4.src[1] == '!' ) {
            hdr = htole32(((4 + meg4.src_len) << 8) | MEG4_CHUNK_CODE);
            memcpy(ptr, &hdr, 4); ptr += 4;
            memcpy(ptr, meg4.src, meg4.src_len); ptr += meg4.src_len;
        }
    } else {
        /* PRO version saves compiled bytecode directly */
        if(meg4_init && meg4_init_len > 0) {
            hdr = htole32(((4 + meg4_init_len) << 8) | MEG4_CHUNK_DATA);
            memcpy(ptr, &hdr, 4); ptr += 4;
            memcpy(ptr, meg4_init, meg4_init_len); ptr += meg4_init_len;
        }
        if(meg4.code && meg4.code_len > 0) {
            /* don't save debug symbols */
            j = meg4.code_type < 0x10 && meg4.code[0] < meg4.code_len ? meg4.code[0] : meg4.code_len;
            hdr = htole32(((5 + j * 4) << 8) | MEG4_CHUNK_CODE);
            memcpy(ptr, &hdr, 4); ptr += 4; *ptr++ = meg4.code_type;
            for(d = (uint32_t*)ptr, i = 0; i < j; i++, d++)
                *d = htole32(meg4.code[i]);
            ptr += j * 4;
        }
    }

    /* palette */
    if(memcmp(meg4.mmio.palette, default_pal, sizeof(meg4.mmio.palette))) {
        hdr = htole32(((4 + sizeof(meg4.mmio.palette)) << 8) | MEG4_CHUNK_PAL);
        memcpy(ptr, &hdr, 4); ptr += 4;
        memcpy(ptr, meg4.mmio.palette, sizeof(meg4.mmio.palette)); ptr += sizeof(meg4.mmio.palette);
    }

    /* sprites */
    if(spr) {
        if(sprsiz > 0) {
            hdr = htole32(((4 + sprsiz) << 8) | MEG4_CHUNK_SPRITES);
            memcpy(ptr, &hdr, 4); ptr += 4;
            memcpy(ptr, spr, sprsiz); ptr += sprsiz;
        }
        free(spr);
    }

    /* map */
    if(map) {
        if(mapsiz > 0) {
            hdr = htole32(((5 + mapsiz) << 8) | MEG4_CHUNK_MAP);
            memcpy(ptr, &hdr, 4); ptr[4] = meg4.mmio.mapsel; ptr += 5;
            memcpy(ptr, map, mapsiz); ptr += mapsiz;
        }
        free(map);
    }

    /* font */
    if(fontsiz > 0) {
        hdr = htole32(((4 + fontsiz) << 8) | MEG4_CHUNK_FONT);
        memcpy(ptr, &hdr, 4); ptr += 4;
        for(i = 0; i < 65536; i += j) {
            for(j = 0; j < 127 && (i + j) < 65536 && !meg4_isbyte(meg4.font + ((i + j) << 3), 0, 8); j++);
            if(j) { *ptr++ = j; memcpy(ptr, meg4.font + ((i + j) << 3), j << 3); ptr += j << 3; }
            else { for(j = 0; j < 128 && (i + j) < 65536 && meg4_isbyte(meg4.font + ((i + j) << 3), 0, 8); j++){}; *ptr++ = -j; }
        }
    }

    /* waveforms */
    for(i = 0; i < (int)(sizeof(meg4.waveforms)/sizeof(meg4.waveforms[0])); i++)
        if(meg4.waveforms[i][0] || meg4.waveforms[i][1]) {
            j = ((meg4.waveforms[i][1]<<8)|meg4.waveforms[i][0]);
            hdr = htole32(((6 + 8 + j) << 8) | MEG4_CHUNK_WAVE);
            memcpy(ptr, &hdr, 4); ptr += 4; *ptr++ = i + 1; /* wavefor id is one bigger */
            *ptr++ = 0; /* this is the format, only 8-bit mono supported for now */
            memcpy(ptr, meg4.waveforms[i], 8 + j); ptr += 8 + j;
        }

    /* sounds */
    if(sndsiz) {
        hdr = htole32(((4 + sndsiz) << 8) | MEG4_CHUNK_SFX);
        memcpy(ptr, &hdr, 4); ptr += 4;
        memcpy(ptr, meg4.mmio.sounds, sndsiz); ptr += sndsiz;
    }

    /* tracks */
    for(i = 0; i < (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])); i++)
        if(trksiz[i]) {
            hdr = htole32(((5 + trksiz[i]) << 8) | MEG4_CHUNK_TRACK);
            memcpy(ptr, &hdr, 4); ptr += 4; *ptr++ = i;
            memcpy(ptr, meg4.tracks[i], trksiz[i]); ptr += trksiz[i];
        }

    /* overlays */
    for(i = 0; i < 256; i++)
        if(meg4.ovls[i].data && meg4.ovls[i].size > 0) {
            hdr = htole32(((5 + meg4.ovls[i].size) << 8) | MEG4_CHUNK_OVL);
            memcpy(ptr, &hdr, 4); ptr += 4; *ptr++ = i;
            memcpy(ptr, meg4.ovls[i].data, meg4.ovls[i].size); ptr += meg4.ovls[i].size;
        }

    if(len) *len = siz;
    return ret;
}

#endif
