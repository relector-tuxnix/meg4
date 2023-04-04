/*
 * meg4/dsp.c
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
 * @brief Digital Signal Processor
 * @chapter Audio
 *
 */

/*
 * ATTRIBUTION (AND APPRECIATION) NOTICE
 *
 * some parts are based on rombankzero's single header pocketmod.h (MIT-licensed), but
 * has been rewritten heavily to fit MEG-4's purpose. Big big kudos to rombankzero!
 * https://github.com/rombankzero/pocketmod
 */

#define DSP_SPT 882.0f  /* 882.0f = 44100 / 50 for PAL clock freq, 735.0f = 44100 / 60 for NTSC clock freq */

#include "meg4.h"
#include <math.h>
float sinf(float);
float fabsf(float);
float fmodf(float, float);
float powf(float, float);
float roundf(float);
int meg4_isbyte(void *buf, uint8_t byte, int len);

/* mod periods for notes, must match MEG4_NOTE_x enums */
uint16_t dsp_periods[MEG4_NUM_NOTE] = { 0,                                 /* no note */
  /*   C    C#     D    D#     E     F    F#     G    G#     A    A#     B  */
    6848, 6464, 6096, 5760, 5424, 5120, 4832, 4560, 4304, 4064, 3840, 3624, /* octave 0 */
    3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2032, 1920, 1812, /* octave 1 */
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960,  906, /* octave 2 */
     856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453, /* octave 3 */
     428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226, /* octave 4 */
     214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113, /* octave 5 */
     107,  101,   95,   90,   85,   80,   75,   71,   67,   63,   60,   56, /* octave 6 */
      53,   50,   47,   45,   42,   40,   37,   35,   33,   31,   30,   28  /* octave 7 */
};
static uint16_t dsp_finetune[15][MEG4_NUM_NOTE - 1];

/* stuff needed for generating the waveforms */
typedef float (*wavefunc_t)(float);
/* generated with rand() :-) But we must use a predictable output, so always use the same random for noise */
static int8_t wave_rand[128] = {-116,14,-63,111,-100,-13,16,111,-32,-78,13,-9,1,90,117,59,57,-19,-19,24,-34,70,-50,59,5,60,1,-29,
    -102,77,-92,-90,-37,-27,-106,-9,89,38,-26,-71,88,116,48,-39,78,37,-108,8,-110,1,-96,-15,-56,-17,-84,77,-85,46,-80,-59,-5,-44,
    -20,86,57,2,-50,18,-88,52,75,-128,40,-5,-39,-9,-96,-18,127,-77,111,-97,36,-73,14,80,-124,58,-2,-76,127,121,8,-21,80,-62,109,
    -98,84,-107,82,32,-107,-5,-101,-17,114,-68,93,113,-17,76,-112,-109,-124,31,99,-120,-39,-30,-67,-40,-37,69,68,-85,-121,49 };
static float wave_sine(float t)     { return sinf(t * 360 * 0.01745329252); }
static float wave_triangle(float t) { return t < 0.25 ? t / 0.25 : (t < 0.75 ? 1.0 - (t - 0.25) * 4.0 : -1.0 + (t - 0.75) / 0.25); }
static float wave_sawtooth(float t) { return 2 * (t - (int) (t + 0.5)); }
static float wave_square(float t)   { return t < 0.5 ? 1.0 : -1.0; }
static float wave_pulse(float t)    { return (fmodf(t, 1) < 0.3125 ? 1 : -1) * 0.7; }
static float wave_organ(float t)    { t *= 4; return (fabsf(fmodf(t, 2) - 1) - 0.5 + (fabsf(fmodf(t * 0.5, 2) - 1) - 0.5) / 2.0 - 0.1); }
static float wave_noise(float t)    { return (float)wave_rand[((int)t) & 0x7f] / 127.0; }
static float wave_phaser(float t)   { t *= 2; return (fabsf(fmodf(t, 2) - 1) - 0.5f + (fabsf(fmodf((t * 127 / 128), 2) - 1) - 0.5) / 2) - 0.25; }
static wavefunc_t waves[] = { wave_sine, wave_triangle, wave_sawtooth, wave_square, wave_pulse, wave_organ, wave_noise, wave_phaser };
static uint8_t *defwaves = NULL;

/**
 * The global dsp context, this is different for the editors (if compiled with editors, that is)
 */
meg4_dsp_t *dsp = &meg4.dram;

/**
 * Generate waveform
 */
void dsp_genwave(int idx, int wave)
{
    int i, l;

    /* wave 00 is reserved for "keep using previous" */
    if(idx < 1 || idx > 31 || wave < 0 || wave >= (int)(sizeof(waves)/sizeof(waves[0]))) return;
    idx--;
    memset(&meg4.waveforms[idx][2], 0, sizeof(meg4.waveforms[0]) - 2);
    l = (meg4.waveforms[idx][1]<<8)|meg4.waveforms[idx][0];     /* number of samples */
    if(l < 2) { l = 256; meg4.waveforms[idx][0] = 0; meg4.waveforms[idx][1] = 1; }
    meg4.waveforms[idx][7] = 64;                                /* volume */
    for(i = 0; i < l; i++)
        meg4.waveforms[idx][8 + i] = (int8_t)((*waves[wave])(i ? (float)i/(float)l : 0.0) * (float)127.0);
}

/**
 * Initialize DSP
 */
void dsp_init(void)
{
    int i, j;
    float f;

    dsp = &meg4.dram;
    /* generate tables */
    for(i = 1; i < 16; i++) {
        f = powf(2, ((float)-(i < 8 ? i : i - 16) / 12.0) / 8.0);
        for(j = 1; j < MEG4_NUM_NOTE; j++)
            dsp_finetune[i - 1][j - 1] = (int)roundf(f * dsp_periods[j]);
    }
}

/**
 * Free resources
 */
void dsp_free(void)
{
    if(defwaves) { free(defwaves); defwaves = NULL; }
}

/**
 * Reset DSP
 */
void dsp_reset(void)
{
    meg4.mmio.dsp_row = meg4.mmio.dsp_num = meg4.mmio.dsp_track = meg4.mmio.dsp_ticks = 0;
    memset(&meg4.dram, 0, sizeof(meg4.dram));
}

/**
 * Oscillators for vibrato / tremolo effects
 */
static int dsp_lfo(meg4_dsp_ch_t *ch, int step)
{
    static const uint8_t sin[16] = { 0x00,0x19,0x32,0x4a,0x62,0x78,0x8e,0xa2,0xb4,0xc5,0xd4,0xe0,0xec,0xf4,0xfa,0xfe };
    int x;
    switch((ch->et == 7 ? ch->lfo_trem : ch->lfo_vib) & 3) {
        case 0: step &= 0x3f; x = sin[step & 0x0f]; x = (step & 0x1f) < 0x10 ? x : 0xff - x;
            return step < 0x20 ? x : -x;                    /* Sine   */
        case 1: return 0xff - ((step & 0x3f) << 3);         /* Saw    */
        case 2: return (step & 0x3f) < 0x20 ? 0xff : -0xff; /* Square */
        default: return (rand() & 0x1ff) - 0xff;            /* Random */
    }
}

/**
 * Calculate portamento slide
 */
static void dsp_pitch_slide(meg4_dsp_ch_t *ch, int amount)
{
    int max = ch->finetune ? dsp_finetune[ch->finetune - 1][0] : 6848;
    int min = ch->finetune ? dsp_finetune[ch->finetune - 1][MEG4_NUM_NOTE - 2] : 28;
    ch->period += amount;
    ch->period = ch->period < min ? min : (ch->period > max ? max : ch->period);
    ch->dirty |= 1;
}

/**
 * Calculate volume slide
 */
static void dsp_volume_slide(meg4_dsp_ch_t *ch, int param)
{
    int change = ((param & 0xf0) ? (param >> 4) : -(param & 0x0f));
    ch->volume = (int)ch->volume + change > 64 ? 64 : ((int)ch->volume + change < 0 ? 0 : ch->volume + change);
    ch->dirty |= 2;
}

/**
 * Load one note into a channel
 */
static void dsp_note(int channel, uint8_t *note)
{
#define DSP_MEM(dst, src) (dst) = (src) ? (src) : (dst);
#define DSP_MEM2(dst, src) (dst) = (((src) & 0x0f) ? ((src) & 0x0f) : ((dst) & 0x0f)) | \
    (((src) & 0xf0) ? ((src) & 0xf0) : ((dst) & 0xf0));

    meg4_dsp_ch_t *ch = &dsp->ch[channel];
    int period = 0;

    /* failsafes */
    if(dsp->samples_per_tick < 1.0f) dsp->samples_per_tick = DSP_SPT;
    if(note[0] >= MEG4_NUM_NOTE || note[1] > 31) return;
    if((note[2] > 0xF && note[2] < 0xE1) || note[2] > 0xED || note[2] == 8 || note[2] == 0xE3 || note[2] == 0xE6 || note[2] == 0xE8 ||
      (channel >= 4 && (note[2] == 0xB || note[2] == 0xF || note[2] == 0xE9))) { ch->et = ch->ep = 0; }
    else { ch->et = note[2]; ch->ep = note[3]; }

    /* waveform (must be before setting note, as it might change finetune) */
    if(note[1]) {
        if(note[1] < 32 && (meg4.waveforms[note[1] - 1][0] || meg4.waveforms[note[1] - 1][1])) {
            ch->sample = note[1];
            ch->finetune = meg4.waveforms[note[1] - 1][6] & 0x0f;
            ch->volume = meg4.waveforms[note[1] - 1][7];
            if(ch->et != 0xED) ch->dirty |= 2;
        } else ch->sample = 0;
    }

    /* note */
    if(note[0]) {
        ch->note = note[0];
        period = ch->finetune ? dsp_finetune[(uint32_t)ch->finetune - 1][(uint32_t)note[0] - 1] : dsp_periods[(uint32_t)note[0]];
        if(ch->et != 0x3) {
            if(ch->et != 0xED) { ch->period = period; ch->dirty |= 1; ch->position = 0.0f; ch->lfo_step = 0; }
            else ch->delayed = period;
        }
    }

    /* effects */
    switch (ch->et) {
        case 0x3: DSP_MEM(ch->porta, ch->ep); DSP_MEM(ch->target, period); break;
        case 0x5: DSP_MEM(ch->target, period); break;
        case 0x4: DSP_MEM2(ch->vibra, ch->ep); break;
        case 0x7: DSP_MEM2(ch->trem, ch->ep); break;
        case 0xE1: DSP_MEM(ch->fporta, ch->ep); break;
        case 0xE2: DSP_MEM(ch->fvibra, ch->ep); break;
        case 0xEA: DSP_MEM(ch->fvsu, ch->ep); break;
        case 0xEB: DSP_MEM(ch->fvsd, ch->ep); break;
        case 0x9: if(period != 0 || note[1] != 0) { if(ch->ep) { ch->offs = ch->ep; } ch->position = ch->offs << 8; } break;
        case 0xB: dsp->row = ch->ep > 15 || (int)(ch->ep << 6) >= (int)dsp->num ? 0 : ch->ep << 6; break;
        case 0xC: ch->volume = (int)ch->ep; ch->dirty |= 2; break;
        case 0xE4: ch->lfo_vib = ch->ep & 3; break;
        case 0xE5: ch->finetune = ch->ep & 0xf; ch->dirty |= 1; break;
        case 0xE7: ch->lfo_trem = ch->ep & 3; break;
        case 0xF: if(ch->ep != 0) { if(ch->ep < 0x20) dsp->ticks_per_row = ch->ep; else dsp->samples_per_tick = (float)44100 / (0.4f * (float)ch->ep); } break;
        default: break;
    }
}

/**
 * Process one music or sound tick
 */
static void dsp_next_tick(int type)
{
    meg4_dsp_ch_t *ch;
    int i, tick, rate, order, closer, s = (type ? 4 : 0), e = (type ? 16 : 4);
    float period;
    static const float arpeggio[16] = {
        1.000000f, 1.059463f, 1.122462f, 1.189207f, 1.259921f, 1.334840f, 1.414214f, 1.498307f,
        1.587401f, 1.681793f, 1.781797f, 1.887749f, 2.000000f, 2.118926f, 2.244924f, 2.378414f
    };

    dsp->tick[type]++;
    if(!type) {
        if(dsp->tick[0] >= dsp->ticks_per_row) {
            if(dsp->ticks_per_row > 0) {
                if(dsp->row >= dsp->num) dsp->row = 0;
                for(i = 0; i < 4; i++)
                    dsp_note(i, &meg4.tracks[dsp->track][((dsp->row << 2) + i) << 2]);
                dsp->row++;
            }
            dsp->tick[0] = 0;
        }
    } else
    if(dsp->tick[1] >= 6) dsp->tick[1] = 0;
    tick = dsp->tick[type];

    /* Make per-tick adjustments for all channels */
    for (i = s; i < e; i++) {
        ch = &dsp->ch[i];
        if(type && !ch->master) continue;

        /* Handle effects that may happen on any tick */
        switch (ch->et) {
            case 0x0: ch->dirty |= 1; break;
            case 0xE9: if(!(ch->ep && tick % (int)ch->ep)) { ch->position = 0.0f; ch->lfo_step = 0; } break;
            case 0xEC: if(tick == (int)ch->ep) { ch->volume = 0; ch->dirty |= 2; } break;
            case 0xED: if(tick == (int)ch->ep && ch->sample) { ch->dirty |= 3; ch->period = ch->delayed; ch->position = 0.0f; ch->lfo_step = 0; } break;
            default: break;
        }

        if(!tick) {
            /* Handle effects that only happen on the first tick of a row */
            switch (ch->et) {
                case 0xE1: dsp_pitch_slide(ch, -(int)ch->fporta); break;
                case 0xE2: dsp_pitch_slide(ch, +(int)ch->fvibra); break;
                case 0xEA: dsp_volume_slide(ch, ch->fvsu << 4); break;
                case 0xEB: dsp_volume_slide(ch, ch->fvsd & 15); break;
                default: break;
            }
        } else {
            /* Handle effects that are not applied on the first tick */
            switch (ch->et) {
                case 0x1: dsp_pitch_slide(ch, -(int)ch->ep); break;
                case 0x2: dsp_pitch_slide(ch, +(int)ch->ep); break;
                case 0x5: dsp_volume_slide(ch, (int)ch->ep); goto tone;
                case 0x3:
tone:               rate = ch->porta; order = ch->period < ch->target; closer = ch->period + (order ? rate : -rate);
                    ch->period = (closer < ch->target) == order ? closer : ch->target;
                    ch->dirty |= 1;
                break;
                case 0x6: dsp_volume_slide(ch, (int)ch->ep); ch->lfo_step++; ch->dirty |= 1; break;
                case 0x4: ch->lfo_step++; ch->dirty |= 1; break;
                case 0x7: ch->lfo_step++; ch->dirty |= 2; break;
                case 0xA: dsp_volume_slide(ch, (int)ch->ep); break;
                default: break;
            }
        }

        /* Update channel volume/pitch if either is out of date */
        if(ch->dirty & 1) {
            ch->increment = 0.0f;
            if(ch->period) {
                period = ch->period;
                if(ch->et == 0x4 || ch->et == 0x6) { rate = ch->vibra & 0x0f; period += dsp_lfo(ch, (ch->vibra >> 4) * ch->lfo_step) * rate / 128.0f; }
                else if(ch->et == 0x0 && ch->ep) { period /= arpeggio[(ch->ep >> ((2 - tick % 3) << 2)) & 0x0f]; }
                ch->increment = 1773447.3f / (period * 44100);
            }
            ch->dirty &= ~1;
        }
        if(ch->dirty & 2) {
            rate = ch->volume + (ch->et == 0x7 ? (dsp_lfo(ch, ch->lfo_step * (ch->trem >> 4)) * (ch->trem & 0x0f) >> 6) : 0);
            ch->tremolo = rate > 64 ? 64 : rate;
            if(type && !ch->tremolo) ch->master = 0;
            ch->dirty &= ~2;
        }
    }
}

/**
 * Feed the audio device with raw PCM data
 */
void meg4_audiofeed(float *buf, int len)
{
    meg4_dsp_ch_t *ch;
    int8_t *smp;
    int i, j, k = 1, l, n, m, o, p = 1, s, e, d, num, rem, ls, ll, le;
    float volume, *ptr, t;

    if(!buf || len < 1 || !dsp) return;
    /* update the DSP status registers */
    if(dsp == &meg4.dram) {
        if(meg4.dram.ticks_per_row > 0) {
            meg4.mmio.dsp_row = htole16(meg4.dram.row); meg4.mmio.dsp_num = htole16(meg4.dram.num);
            meg4.mmio.dsp_track = meg4.dram.track;
            meg4.mmio.dsp_ticks = meg4.dram.ticks_per_row;
        } else {
            meg4.mmio.dsp_row = meg4.mmio.dsp_num = meg4.mmio.dsp_track = 0;
            meg4.mmio.dsp_ticks = 6;
        }
    }
    for(i = d = 0; i < 16; i++) {
        ch = &dsp->ch[i];
        if(ch->master && ch->tremolo && ch->sample && ch->position >= 0.0f && ch->increment > 0.0f) {
            if(i >= 4) { d++; k = 1; p = 2; }
            if(dsp == &meg4.dram)
                meg4.mmio.dsp_ch[i] = ((((int)ch->tremolo * 255) >> 6) << 24) | (ch->sample << 16) | htole16((int)ch->position & 0xffff);
        } else if(dsp == &meg4.dram) meg4.mmio.dsp_ch[i] = 0;
    }
    if(dsp->ticks_per_row) { d += 4; k = 0; }
    /* update the output buffer */
    memset(buf, 0, len * sizeof(float));
    if(k) { s = 4; e = 16; t = DSP_SPT; } else { s = 0; e = 4; t = dsp->samples_per_tick; }
    for(; k < p; k++) {
        for(ptr = buf, rem = len; rem > 0;) {
            num = (int)(t - dsp->sample[k]); if(num < 1) { break; } if(num > rem) { num = rem; }
            for(i = s; i < e; i++) {
                ch = &dsp->ch[i];
                if(ch->master && ch->sample && ch->position >= 0.0f && ch->increment > 0.0f) {
                    smp = (int8_t*)&meg4.waveforms[ch->sample - 1][0];
                    l = ((uint8_t)smp[1]<<8)|(uint8_t)smp[0];  ls = ((uint8_t)smp[3]<<8)|(uint8_t)smp[2];
                    ll = ((uint8_t)smp[5]<<8)|(uint8_t)smp[4]; le = ll > 0 ? ls + ll : l;
                    m = num; o = 0; volume = ch->master * ch->tremolo / (float) (128 * 255 * 64 * d);
                    do {
                        n = ((float)(le + 1) - ch->position) / ch->increment;
                        if(n > m) n = m;
                        for(j = 0; j < n; j++, ch->position += ch->increment)
                            ptr[o++] += (float)smp[8 + (int)ch->position] * volume;
                        if(ch->position > le) {
                            if(ll > 0) {
                                if(k) {
                                    if(ch->tremolo) ch->tremolo--;
                                    else { ch->position = -1.0f; ch->tremolo = meg4.waveforms[ch->sample - 1][7]; break; }
                                }
                                ch->position -= ll;
                            } else { ch->position = -1.0f; break; }
                        }
                        m -= n;
                    } while(n > 0);
                }
            }
            rem -= num; ptr += num; if((dsp->sample[k] += num) >= t) { dsp->sample[k] -= t; dsp_next_tick(k); }
        }
        s = e; e = 16; t = DSP_SPT;
    }
}

/**
 * Plays a sound effect.
 * @param sfx the index of the sound effect, 0 to 63
 * @param channel channel to be used, 0 to 11
 * @param volume volume to be used, 0 to 255, 0 turns off channel
 */
void meg4_api_sfx(uint8_t sfx, uint8_t channel, uint8_t volume)
{
    if(channel > 11 || !dsp) return;
    memset(&dsp->ch[4 + channel], 0, sizeof(meg4_dsp_ch_t));
    if(sfx < 64 && meg4.mmio.sounds[sfx << 2] && meg4.mmio.sounds[(sfx << 2) + 1] && volume) {
        dsp_note(4 + channel, &meg4.mmio.sounds[sfx << 2]);
        dsp->ch[4 + channel].master = volume;
        dsp_next_tick(1);
    }
}

/**
 * Plays a music track.
 * @param track the index of the music track, 0 to 7
 * @param row row to start playing from, 0 to 1023 (max song length)
 * @param volume volume to be used, 0 to 255, 0 turns off music
 */
void meg4_api_music(uint8_t track, uint16_t row, uint8_t volume)
{
    int i, n = 0;

    if(!dsp) return;
    if(dsp == &meg4.dram) { meg4.mmio.dsp_row = meg4.mmio.dsp_num = meg4.mmio.dsp_track = meg4.mmio.dsp_ticks = 0; }
    memset(&dsp->ch, 0, 4 * sizeof(meg4_dsp_ch_t));
    dsp->track = dsp->row = dsp->num = dsp->ticks_per_row = 0;
    if(track > sizeof(meg4.tracks)/sizeof(meg4.tracks[0]) || row >= (sizeof(meg4.tracks[0]) >> 4)) return;
    for(i = 0; i < (int)(sizeof(meg4.tracks[0]) / 16); i++)
        if(!meg4_isbyte(&meg4.tracks[track][i * 16], 0, 16)) n = i + 1;
    if(row >= n) return;
    if(volume) {
        if(dsp == &meg4.dram) {
            meg4.mmio.dsp_row = htole16(row); meg4.mmio.dsp_num = htole16(n); meg4.mmio.dsp_track = track; meg4.mmio.dsp_ticks = 6;
        }
        dsp->track = track; dsp->row = row; dsp->num = n; dsp->ticks_per_row = dsp->tick[0] = 6;
        dsp->ch[0].master = dsp->ch[1].master = dsp->ch[2].master = dsp->ch[3].master = volume;
        dsp_next_tick(0);
    }
}

#ifndef NOEDITORS
/**
 * Play one note directly (for the editors only)
 */
void meg4_playnote(uint8_t *note, uint8_t volume)
{
    int i;

    if(!dsp || dsp == &meg4.dram) return;
    for(i = 0; i < 16; i++) dsp->ch[i].master = 0;
    dsp->ticks_per_row = 0;
    if(note && volume) { dsp_note(4, note); dsp->ch[4].master = volume; dsp->tick[1] = 6; dsp_next_tick(1); }
}
#endif
