/*
 * meg4/editors/fmt_mid.h
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
 * @brief General MIDI importer (for the insane General MIDI, Spec 1.1)
 * https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf
 */

/* originally I wanted to remove these, but I keep running into buggy midi files... so good to have, just set the define */
#define MIDI_DBG 0
#if MIDI_DBG > 0
#define DBG1(a) (printf a)
#else
#define DBG1(a)
#endif
#if MIDI_DBG > 1
#define DBG2(a) (printf a)
#else
#define DBG2(a)
#endif

/* midi track state */
typedef struct {
    uint8_t *ptr, *end, *evt;   /* chunk pointers */
    uint8_t ec, id;             /* remembered event's code (and sub-code if code is 0xff) */
    int len;                    /* remembered event length in bytes */
    int tick;                   /* current tick counter for this chunk */
    uint8_t channel;            /* current channel being used in this track */
} mtrk_t;

/**
 * Decode a variable length quantity
 */
static int mid_vlq(uint8_t **ptr)
{
    int p, out = 0;
    do { p = *(*ptr); out <<= 7; out |= p & 0x7F; (*ptr)++; } while((p & 0x80) && out < 0xfffffff);
    return out < 0xfffffff ? out : -1;
}

/**
 * Import a MIDI file
 * @param track track number
 */
static int format_mid(int track, uint8_t *buf, int len)
{
    int tpr00, tpq, i, j, k, l, n, v, iseof = 0, tick = 0, next = 0, rowtick = 0, tr[32];
    mtrk_t mtrk[16];
    uint8_t *ptr, *row, *end, rs = 0, tempo = 6, w, waves[16], dspwave[32], dspvol[4], dspch[16*128], tracks[16*128];

    if(!buf || len < 23 || track < 0 || track >= (int)(sizeof(meg4.tracks)/sizeof(meg4.tracks[0])) ||
        buf[9] > 1 /* only SMF0 and SMF1, the SMF2 variant not supported */) return 0;

    /* the DSP is working on 44100 Hz, meaning 44100*60 samples per minute, samples_per_tick is 882 for C-4, and there are 6 ticks
     * in a row using the default tempo, so 882 * 6 = 5292 samples per row. This gives 44100*60 / 5292 = 500 rows per minute. We
     * are using minute because with seconds 44100/5292 is 8.33333333* which would result in too much accumulated rounding errors.
     * Also for midi ticks we store hundredfold values (fixed point arithmetic), so 500 / 100 = 5 */
    if(!(buf[12] & 0x80)) {
        /* the default midi BPM is 120 quoter-notes per minute, and time is given in number of ticks per quoter-notes, so */
        tpq = ((buf[12] << 8) | buf[13]);
        tpr00 = tpq * 120 / 5;
    } else {
        /* SMTPE encoded time, first byte negative frame per sec, second byte ticks per frame, and we have 60 secs per minute */
        i = (buf[12] == 0xE3 ? 30 : -(int8_t)buf[12]) * (int)buf[13] * 60; /* ticks per minute */
        tpq = i / 120;  /* ticks per quoter-notes, assuming the default 120 BPM is true with SMTPE too... */
        tpr00 = i / 5;
    }
    /* tpr00 is now equals to midi ticks * 100 per DSP rows */
    DBG1(("midi header; midi ticks per dsp row %u.%02u, midi ticks per quoternotes %u, dsp ticks %u\n", tpr00/100, tpr00%100, tpq, tempo));

    /* channels might be stored in a single mtrk chunk or scattered through multiple mtrk chunks */
    memset(&mtrk, 0, sizeof(mtrk));
    for(ptr = buf + 8 + ((buf[6]<<8)|buf[7]), n = 0; ptr < buf + len && !memcmp(ptr, "MTrk", 4) && n < 16; n++) {
        mtrk[n].ptr = ptr + 8;
        mtrk[n].end = ptr = ptr + 8 + ((ptr[4]<<24)|(ptr[5]<<16)|(ptr[6]<<8)|ptr[7]);
        if(mtrk[n].end > buf + len) break;
    }
    /* no chunks found? */
    if(!n) return 0;

    /* assume Grand Piano when there's no Program Change message */
    memset(waves, 1, sizeof(waves));
    memset(tr, 0, sizeof(tr));

    /* no DSP channel asignments */
    memset(dspch, 0xff, sizeof(dspch));
    memset(dspvol, 0, sizeof(dspvol));
    /* clear patterns before start */
    memset(meg4.tracks[track], 0, sizeof(meg4.tracks[0]));
    row = meg4.tracks[track]; end = row + sizeof(meg4.tracks[0]);

    /* read in MIDI events */
    while(!iseof) {
        /* decode those insane timestamps and event lengths */
        iseof = 1; next = tick + 0xfffff;
        for(i = 0; i < n; i++)
            if(mtrk[i].ptr < mtrk[i].end) {
                iseof = 0; if(mtrk[i].tick > tick) continue;
                /* get delta time */
                j = mid_vlq(&mtrk[i].ptr);
                DBG2(("read track %d dtime %x ptr %lx data %02x\n",i,j,ptr-buf,*ptr));
                /* bad encoding, do not read this track any further */
                if(j < 0 || j > 0xfffff) {
badtrack:           memcpy(&mtrk[i], &mtrk[i + 1], (n - i - 1) * sizeof(mtrk_t)); n--; i--; continue;
                }
                mtrk[i].tick += j;
                if(mtrk[i].tick < next) next = mtrk[i].tick;
                mtrk[i].evt = ptr = mtrk[i].ptr;
                /* totally not explained in the spec, just a sidenote about some "running status"... */
                if(!(*ptr & 0x80)) {
                    if(!rs) goto badtrack;
                    mtrk[i].ec = rs; ptr += mtrk[i].len;
                } else {
                    mtrk[i].ec = *ptr; mtrk[i].evt++; mtrk[i].id = rs = 0;
                    /* would it really hurt to simply use <type><length><data> triplets all the time? */
                    switch(*ptr) {
                        case 0xF1: case 0xF4: case 0xF5: case 0xF9: case 0xFD: goto badtrack;
                        case 0xF6: case 0xF8: case 0xFA: case 0xFB: case 0xFC: case 0xFE: ptr++; break;
                        case 0xF2: ptr += 3; break;
                        case 0xF3: ptr += 2; break;
                        case 0xF0: case 0xF7: ptr++; j = mid_vlq(&ptr); if(j < 0) { goto badtrack; } ptr += j; break;
                        case 0xFF:
                            mtrk[i].id = ptr[1]; ptr += 2; j = mid_vlq(&ptr); if(j < 0) { goto badtrack; }
                            mtrk[i].evt = ptr; ptr += j; break;
                        default:
                            rs = *ptr;
                            switch((*ptr & 0x70) >> 4) {
                                case 0: case 1: case 2: case 3: case 6: case 7: ptr += 3; break;
                                case 4: case 5: ptr += 2; break;
                            }
                        break;
                    }
                    mtrk[i].len = ptr - mtrk[i].evt;
                    /* failsafe, should never happen */
                    if(mtrk[i].len < 0 || mtrk[i].len > 0xfffff) goto badtrack;
                }
                mtrk[i].ptr = ptr;
            }
        for(; tick <= next; tick++)
            for(i = 0; i < n; i++)
                if(mtrk[i].tick == tick && mtrk[i].evt) {
                    /* okay, everything we did so far was nothing more than dealing with the pure madness of
                     * the MIDI format; now finally we can actually read in the events in correct order... */
#if MIDI_DBG > 1
printf("parse event: tick %u in-row-delta %u track %u type %02x %02x len %u %x:", tick, tick-rowtick, i, mtrk[i].ec, mtrk[i].id, mtrk[i].len,(uint32_t)(mtrk[i].evt-buf));
for(j = 0; j < mtrk[i].len; j++) { printf(" %02x", mtrk[i].evt[j]); } printf("\n");
#endif
                    /* parse special events */
                    if(mtrk[i].ec == 0xFF)
                        switch(mtrk[i].id) {
                            case 0x51:
                                /* Set tempo, according to the spec, only allowed in the first track */
                                if(!i && mtrk[i].len == 3 && (mtrk[i].evt[0] || mtrk[i].evt[1] || mtrk[i].evt[2])) {
                                    /* yet another time calculation method within the same spec... third times the charm??? */
                                    j = 60000000/(int)((mtrk[i].evt[0]<<16)|(mtrk[i].evt[1]<<8)|mtrk[i].evt[2]); /* bpm */
                                    k = tpq * j / 5;
                                    DBG1(("set tempo new %u old %u (midi ticks per dsp row), bpm %u\n",k,tpr00,j));
                                    if(j > 0 && tpr00 != k) {
                                        tpr00 = k; k = 720 / j; if(k < 1) { k = 1; } if(k > 0x1f) k = 0x1f;
                                        /* this is problematic, midi has a magnitude more precise timing than our DSP */
                                        if(tempo != k) {
                                            DBG1(("    writing dsp tempo %u\n",k));
                                            l = !row[2] ? 2 : (!row[6] ? 6 : (!row[10] ? 10 : (!row[14] ? 14 : 2)));
                                            row[l] = 0xF; row[l + 1] = tempo = k; /* DSP set tempo effect */
                                        }
                                    }
                                }
                            break;
                            case 0x52:
                                /* Track End */
                                DBG1(("track %d end\n",i));
                                for(j = 0; j < 16*128; j++)
                                    /* silence all DSP channels that were playing a note which was turned on by this track */
                                    if(tracks[j] == i && dspch[j] != 0xff && dspvol[dspch[j]]) {
                                        k = dspch[j] * 4; if(row[k + 2] == 0xF) k += 16;
                                        DBG1(("    writing dsp silence row %lx ch %d\n",(row - meg4.tracks[track])>>4,k>>2));
                                        if(row + k < end) { row[k + 2] = 0xC; row[k + 3] = 0x0; }
                                        dspvol[dspch[j]] = 0; dspch[j] = 0xff;
                                    }
                            break;
                        }
                    /* adjust row when necessary */
                    j = (tick - rowtick) * 100 / tpr00;
                    if(j > 0) {
                        /* skip silence at the beginning */
                        if(row > meg4.tracks[track] || !row[0] || !row[1] || !row[4] || !row[5] || !row[8] || !row[9] || !row[12] || !row[13]) {
                            row += j * 16;
                            DBG1(("  --- adjust dsp row by %d (in-row-tick %u.00, tpr %u.%02u) ---\n",j,tick - rowtick, tpr00/100,tpr00%100));
                        }
                        if(row >= end) { n = 0; break; }
                        rowtick = tick;
                    }
                    /* parse normal event messages */
                    if((mtrk[i].ec & 0xf0) != 0xf0) {
                        mtrk[i].channel = mtrk[i].ec & 0x0f;
                        switch(mtrk[i].ec & 0xf0) {
                            case 0x80:
                            case 0x90:
                                /* Note ON / OFF */
                                if(mtrk[i].channel != 9 && waves[mtrk[i].channel] && mtrk[i].len == 2 && mtrk[i].evt[0] >= 12 && mtrk[i].evt[0] < 108) {
                                    k = mtrk[i].evt[0] - 11; v = (mtrk[i].ec & 0xf0) == 0x80 ? 0 : mtrk[i].evt[1] * 64 / 127;
                                    j = (mtrk[i].channel<<7) | k; l = dspch[j] * 4; w = waves[mtrk[i].channel];
                                    DBG1(("note %s  channel %u note %u v %u j %u (note %u wave %u dspch %x) --- dsp allocs %u %u %u %u\n",
                                        (mtrk[i].ec & 0xf0) == 0x80 ? "OFF" : "ON ", mtrk[i].channel,mtrk[i].evt[0],v,j,k,w,dspch[j],
                                        dspvol[0],dspvol[1],dspvol[2],dspvol[3]));
                                    /* remember track, we need to silence this note when track ends */
                                    tracks[j] = i;
                                    /* some midi files don't have note OFF messages, just note ON with velocity 0... */
                                    if(!v) {
                                        if(dspch[j] != 0xff && dspvol[dspch[j]]) {
                                            k = dspch[j] * 4; if(row[k + 2] == 0xF) k += 16;
                                            DBG1(("    writing dsp silence row %lx ch %d\n",(row - meg4.tracks[track])>>4,dspch[j]));
                                            if(row + k < end) { row[k + 2] = 0xC; row[k + 3] = 0; }
                                            dspvol[dspch[j]] = 0;
                                        }
                                        dspch[j] = 0xff;
                                    } else {
                                        /* see if we have a DSP channel for this note. Try to use the same channel as before */
                                        if(!(dspch[j] != 0xff && (!dspvol[dspch[j]] || (row[l + 0] == k && row[l + 1] == w)))) {
                                            dspch[j] = 0xff;
                                            /* let's see if the channel we used last time for this wave is free or not */
                                            if(!dspvol[dspwave[w]]) l = dspwave[w];
                                            /* nope, let's see if there's any free DSP channels at all at this moment */
                                            else for(l = 0; l < 4 && (dspvol[l] || row[l*4 + 0] || row[l*4 + 1]); l++);
                                            if(l < 4) dspch[j] = l;
                                        }
                                        if(dspch[j] != 0xff) {
                                            /* workaround buggy midi files without PC, assume they use Grand Piano */
                                            if(w == 1 && !tr[0]) w = tr[0] = waveform(meg4_defwaves);
                                            DBG1(("    writing dsp note row %lx ch %d (%u,%x,%u)\n",(row - meg4.tracks[track])>>4,dspch[j],k,w,v));
                                            l = dspch[j] * 4;
                                            if(v > 0) { row[l + 0] = k; row[l + 1] = w; }
                                            if(v != dspvol[dspch[j]] && row[l + 2] != 0xF) {
                                                row[l + 2] = 0xC; row[l + 3] = dspvol[dspch[j]] = v;
                                            }
                                            dspwave[w] = dspch[j];
                                        }
                                    }
                                }
                            break;
                            case 0xA0:
                                /* Key pressure */
                                if(mtrk[i].channel != 9 && waves[mtrk[i].channel] && mtrk[i].len == 2 && mtrk[i].evt[0] >= 12 && mtrk[i].evt[0] < 108) {
                                    k = mtrk[i].evt[0] - 11; v = mtrk[i].evt[1] * 64 / 127;
                                    j = (mtrk[i].channel<<7) | k; l = dspch[j] * 4;
                                    DBG1(("key pressure channel %u note %u v %u j %u\n",mtrk[i].channel,mtrk[i].evt[0],v,j));
                                    if(dspch[j] != 0xff && l < 16) {
                                        if(v != dspvol[dspch[j]] && row[l + 2] != 0xF) {
                                            DBG1(("    writing dsp volumechange row %lx ch %d (%u)\n",(row - meg4.tracks[track])>>4,dspch[j],v));
                                            row[l + 2] = 0xC; row[l + 3] = dspvol[dspch[j]] = v;
                                        }
                                        if(!v) dspch[j] = 0xff;
                                    }
                                }
                            break;
                            case 0xC0:
                                /* Program Change */
                                if(mtrk[i].channel != 9 && mtrk[i].len == 1) {
                                    j = mtrk[i].evt[0] / 4;
                                    /* no special effects support, we simply don't have enough waveform slots for that */
                                    if(j > 24) j = 0;
                                    DBG1(("program change channel %u instrument %u wave %u\n",mtrk[i].channel,mtrk[i].evt[0],tr[j]));
                                    if(!tr[j]) {
                                        /* load the default soundfont for this instrument */
                                        for(ptr = meg4_defwaves, k = 0; ptr && (ptr[0] || ptr[1]) && k < 32; k++, ptr += 8 + ((ptr[1]<<8)|ptr[0]))
                                            if(k == j) {
                                                tr[j] = waveform(ptr);
                                                DBG1(("    soundfont %u loaded at wave %u\n", j, tr[j]));
                                                break;
                                            }
                                    }
                                    waves[mtrk[i].channel] = tr[j];
                                }
                            break;
                            case 0xD0:
                                /* Channel pressure */
                                if(mtrk[i].channel != 9 && mtrk[i].len == 1) {
                                    v = mtrk[i].evt[1] * 64 / 127;
                                    DBG1(("channel pressure channel %u v %u\n",mtrk[i].channel,v));
                                    for(k = 0, j = (mtrk[i].channel<<7); k < 127; k++, j++)
                                        if(dspch[j] != 0xff) {
                                            l = dspch[j] * 4;
                                            if(v != dspvol[dspch[j]] && row[l + 2] != 0xF) {
                                                DBG1(("    writing dsp volumechange row %lx ch %d (%u)\n",(row - meg4.tracks[track])>>4,dspch[j],v));
                                                row[l + 2] = 0xC; row[l + 3] = dspvol[dspch[j]] = v;
                                            }
                                            if(!v) dspch[j] = 0xff;
                                        }
                                }
                            break;
                        }
                    }
                    mtrk[i].evt = NULL;
                }
        if(iseof || !n || next == tick + 0xfffff) break;
        tick = next;
    }
    if(row < end) {
        /* silence all DSP channels that were left playing a note */
        for(j = 0; j < 4; j++)
            if(dspvol[j]) {
                k = j * 4; if(row[k + 2] == 0xF) k += 16;
                DBG1(("    writing dsp silence row %lx ch %d\n",(row - meg4.tracks[track])>>4,j));
                if(row + k < end) { row[k + 2] = 0xC; row[k + 3] = 0x0; }
            }
    }
    return 1;
}
