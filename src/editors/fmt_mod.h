/*
 * meg4/editors/fmt_mod.h
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
 * @brief Amiga MOD format routines
 *
 */

extern uint16_t dsp_periods[MEG4_NUM_NOTE];

/**
 * Add a waveform
 */
static int waveform(uint8_t *tmp)
{
    int j, l, s, e;

    if(!tmp || (!tmp[0] && !tmp[1])) return 0;
    l = (tmp[1]<<8)|tmp[0]; s = (tmp[3]<<8)|tmp[2]; e = (tmp[5]<<8)|tmp[4]; tmp[6] &= 0xf;
    /* failsafes */
    if(l > (int)sizeof(meg4.waveforms[0]) - 8) { l = sizeof(meg4.waveforms[0]) - 8; tmp[0] = l & 0xff; tmp[1] = (l >> 8) & 0xff; }
    if(e < 2 || s + 1 > l) { tmp[2] = tmp[3] = tmp[4] = tmp[5] = 0; } else
    if(s + e > l) { e = l - s; if(e < 2) tmp[2] = tmp[3] = tmp[4] = tmp[5] = 0; else { tmp[4] = e & 0xff; tmp[5] = (e >> 8) & 0xff; } }
    /* find a match */
    for(j = 0; j < 31 && memcmp(meg4.waveforms[j], tmp, 8 + l); j++);
    /* without a match, find an empty slot for this wave */
    if(j >= 31) {
        for(j = 0; j < 31 && (meg4.waveforms[j][0] || meg4.waveforms[j][1]); j++);
        if(j >= 31) {
            main_log(1, "Not enough slots to load waveform");
            meg4.mmio.ptrspr = MEG4_PTR_ERR;
            return 0;
        }
        memcpy(meg4.waveforms[j], tmp, 8 + l); memset(&meg4.waveforms[j][8 + l], 0, sizeof(meg4.waveforms[0]) - 8 - l);
    }
    return j + 1;
}

/**
 * Convert MEG-4 note to MOD note
 */
static void note_meg2mod(uint8_t *dst, uint8_t *src)
{
    uint16_t period = dsp_periods[(uint32_t)src[0]];
    uint8_t w = src[1] & 31, et = src[2] & 0xf, ep = src[3];
    if(et >= 0x10) { ep = ((et & 0xf) << 4) | (ep & 0xf); et >>= 4; }
    dst[0] = (w & 0xf0) | ((period >> 8) & 0xf);    /* period hi, wave hi */
    dst[1] = period & 0xff;                         /* period lo */
    dst[2] = ((w & 0xf) << 4) | (et & 0xf);         /* wave lo, effect type */
    dst[3] = ep;                                    /* effect param */
}

/**
 * Convert MOD note to MEG-4 note
 */
static void note_mod2meg(uint8_t *dst, uint8_t *src, int *tr)
{
    int j, d = 65535;
    uint16_t i, n, period = ((src[0] & 0xf) << 8) | src[1];
    uint8_t w = ((src[0] & 0xf0) | (src[2] >> 4)) & 31, et = src[2] & 0xf, ep = src[3];
    if(et == 0xE) { et = (et << 4) | (ep >> 4); ep &= 0xf; }
    if(et == 8 || et == 0xD || et == 0xE0 || et == 0xE3 || et == 0xE6 || et == 0xE8 || et == 0xEE || et == 0xEF || (et ==0xF && ep >= 0x20))
        et = ep = 0;
    for(i = n = 0; i < MEG4_NUM_NOTE && d; i++) {
        j = dsp_periods[i] > period ? dsp_periods[i] - period : period - dsp_periods[i]; if(j < d) { d = j; n = i; }
    }
    if(w && tr && !tr[w - 1]) n = et = ep = 0;      /* failsafe, clear the note if we couldn't load the sample for it */
    dst[0] = n;                                     /* note (use closest if there was no direct match) */
    dst[1] = w && tr ? tr[w - 1] : w;               /* waveform */
    dst[2] = et;                                    /* effect type */
    dst[3] = ep;                                    /* effect param */
}

/**
 * Export an Amiga MOD file
 * @param track track number, -1 to export sounds
 */
static uint8_t *export_mod(int track, int *len)
{
    int i, j, l = 0, n = 0, p;
    uint8_t *out = NULL, *ptr;

    /* n = number of rows */
    if(track == -1) {
        for(i = 0; i < (int)(sizeof(meg4.mmio.sounds)/sizeof(meg4.mmio.sounds[0])); i++)
            if(meg4.mmio.sounds[i]) n = i + 1;
    } else {
        for(i = 0; i < (int)(sizeof(meg4.tracks[0]) / 16); i++)
            if(!meg4_isbyte(&meg4.tracks[track][i * 16], 0, 16)) n = i + 1;
    }
    if(!len || n < 1) return NULL;
    /* add patterns' length */
    p = ((n + 63) >> 6) * 1024;
    l += p;
    /* add samples' length */
    for(i = 0; i < (int)(sizeof(meg4.waveforms)/sizeof(meg4.waveforms[0])); i++)
        l += ((meg4.waveforms[i][1]<<8)|meg4.waveforms[i][0]) << 1;
    out = (uint8_t*)malloc(1084 + l);
    if(!out) return NULL;
    *len = 1084 + l;
    memset(out, 0, 1084 + l);

    /* song "title" */
    if(track == -1)
        strcpy((char*)out, "MEG-4 SFX");
    else
        sprintf((char*)out, "MEG-4 TRACK%02X", track);

    /* samples meta */
    for(i = 0; i < 31; i++) {
        if(meg4.waveforms[i][0] || meg4.waveforms[i][1]) {
            sprintf((char*)out + 20 + i * 30, "#%02u", i + 1);
            out[42 + i * 30] = meg4.waveforms[i][1];                        /* number of samples */
            out[43 + i * 30] = meg4.waveforms[i][0];
            out[44 + i * 30] = meg4.waveforms[i][6] & 0xf;                  /* finetune */
            out[45 + i * 30] = meg4.waveforms[i][7];                        /* volume, 0..64 */
            out[46 + i * 30] = meg4.waveforms[i][3];                        /* loop start */
            out[47 + i * 30] = meg4.waveforms[i][2];
            out[48 + i * 30] = meg4.waveforms[i][5];                        /* loop length */
            out[49 + i * 30] = meg4.waveforms[i][4];
            if(!out[48 + i * 30] && !out[49 + i * 30]) out[49 + i * 30]++;  /* for some reason FastTracker can't use length 0 */
        }
    }
    /* not header, erhm... middler? */
    out[950] = (n + 63) >> 6;                                               /* song length in patterns */
    for(i = 0; (uint8_t)i < out[950]; i++) out[952 + i] = i;                /* song pattern positions */
    memcpy(out + 1080, "M.K.", 4);                                          /* magic */
    /* patterns */
    for(ptr = out + 1084, i = 0; i < n; i++, ptr += 16) {
        if(track == -1)
            note_meg2mod(ptr, &meg4.mmio.sounds[i]);
        else {
            note_meg2mod(ptr + 0, &meg4.tracks[track][i * 16 + 0]);
            note_meg2mod(ptr + 4, &meg4.tracks[track][i * 16 + 4]);
            note_meg2mod(ptr + 8, &meg4.tracks[track][i * 16 + 8]);
            note_meg2mod(ptr +12, &meg4.tracks[track][i * 16 +12]);
        }
    }
    /* samples */
    for(ptr = out + 1084 + p, i = 0; i < (int)(sizeof(meg4.waveforms)/sizeof(meg4.waveforms[0])); i++)
        for(j = 0; (uint16_t)j < ((meg4.waveforms[i][1]<<8)|meg4.waveforms[i][0]); j++, ptr += 2)
            ptr[0] = ptr[1] = meg4.waveforms[i][8 + j];
    return out;
}

/**
 * Import an Amiga MOD file
 * @param track track number, -1 to import sounds
 */
static int format_mod(int track, uint8_t *buf, int len)
{
    char tw[32] = { 0 };
    int i, j, l, k, m, n = 0, tr[32] = { 0 };
    uint8_t *ptr, tmp[sizeof(meg4.waveforms[0])];

    if(!buf || len < 1084 || track < -1 || track >= (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0]))) return 0;
    /* get number of patterns */
    for(i = 952; i < 952 + buf[950] && 1084 + buf[i] * 1024 < len; i++) if(buf[i] > n) n = buf[i];
    n++;
    if(1084 + n * 1024 > len) return 0;
    if(track == -1) {
        m = sizeof(meg4.mmio.sounds);
        memset(meg4.mmio.sounds, 0, sizeof(meg4.mmio.sounds));
        /* load all waves for sound effects */
        memset(meg4.waveforms, 0, sizeof(meg4.waveforms));
        memset(tw, 1, sizeof(tw));
    } else {
        m = sizeof(meg4.tracks[0]);
        memset(meg4.tracks[track], 0, sizeof(meg4.tracks[0]));
        /* get referenced waves */
        for(k = 0, i = 952; i < 952 + buf[950] && 1084 + buf[i] * 1024 < len && k < m; i++)
            for(ptr = buf + 1084 + buf[i] * 1024, j = 0; j < 64 && k < m; j++, k += 16)
                for(l = 0; l < 4; l++, ptr += 4) { tw[((ptr[0] & 0xf0) | (ptr[2] >> 4)) & 31] = 1; if((ptr[2] & 0xf) == 0xD) j = 64; }
    }
    /* load waves */
    for(ptr = buf + 1084 + n * 1024, i = 0; i < 31 && ptr < buf + len - 1; i++) {
        l = k = (buf[42 + i * 30] << 8) | buf[43 + i * 30];
        if(l < 1) continue;
        if(k >= (int)sizeof(tmp) - 8) k = (int)sizeof(tmp) - 8;
        if(ptr + k * 2 > buf + len) k = (buf + len - ptr) / 2;
        if(k > 0 && tw[i + 1]) {
            tmp[0] = k & 0xff; tmp[1] = (k >> 8) & 0xff;                            /* number of samples */
            tmp[2] = buf[47 + i * 30]; tmp[3] = buf[46 + i * 30];                   /* loop start */
            tmp[4] = buf[49 + i * 30]; tmp[5] = buf[48 + i * 30];                   /* loop length */
            if(!tmp[5] && tmp[4] == 1) tmp[4] = 0;
            tmp[6] = buf[44 + i * 30] & 0xf;                                        /* finetune */
            tmp[7] = (buf[45 + i * 30] > 64 ? 64 : buf[45 + i * 30]);               /* volume */
            if(tmp[7] > 64) tmp[7] = 64;
            for(j = 0; j < k; j++) tmp[8 + j] = ((int)(int8_t)ptr[j * 2] + (int)(int8_t)ptr[j * 2 + 1]) / 2; /* samples */
            tr[i] = waveform(tmp);
        }
        ptr += 2 * l;
    }
    /* load patterns */
    for(k = 0, i = 952; i < 952 + buf[950] && 1084 + buf[i] * 1024 < len && k < m; i++)
        for(ptr = buf + 1084 + buf[i] * 1024, j = 0; j < 64 && k < m; j++, ptr += 16)
            if(track == -1) {
                note_mod2meg(&meg4.mmio.sounds[k], ptr, tr); k += 4;
            } else {
                note_mod2meg(&meg4.tracks[track][k], ptr + 0, tr); k += 4;
                note_mod2meg(&meg4.tracks[track][k], ptr + 4, tr); k += 4;
                note_mod2meg(&meg4.tracks[track][k], ptr + 8, tr); k += 4;
                note_mod2meg(&meg4.tracks[track][k], ptr +12, tr); k += 4;
                /* handle pattern break command */
                if((ptr[2] & 0xf) == 0xD || (ptr[6] & 0xf) == 0xD || (ptr[10] & 0xf) == 0xD || (ptr[14] & 0xf) == 0xD) j = 64;
            }
    return 1;
}
