/*
 * meg4/tests/modplayer/main.c
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
 * @brief Tests MEG-4 Amiga MOD capabilities, imports a .mod and plays it
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <stdio.h>
#include <time.h>
#include <signal.h>
#ifdef TEST_PA
#include <portaudio.h>
#else
#include <SDL2/SDL.h>
#endif
#include "../../src/meg4.h"
meg4_t meg4;

int meg4_isbyte(void *buf, uint8_t byte, int len)
{
    uint8_t *ptr = (uint8_t*)buf;
    int i;
    if(!buf || len < 1) return 0;
    for(i = 0; i < len && ptr[i] == byte; i++);
    return i >= len;
}

#include "../../src/editors/fmt_mod.h"

/**
 * Audio callback wrappers
 */
#ifdef TEST_PA
PaStream *pa = NULL;
static int main_audio(const void *inp, void *out, long unsigned int framesPerBuffer, const PaStreamCallbackTimeInfo *info,
    PaStreamCallbackFlags flags, void *ctx)
{
    (void)inp; (void)info; (void)flags; (void)ctx;
    meg4_audiofeed((float*)out, framesPerBuffer);
    return 0;
}
#else
int audio;
SDL_AudioSpec want, have;
void main_audio(void *ctx, Uint8 *buf, int len)
{
    (void)ctx;
    meg4_audiofeed((float*)buf, len >> 2);
}
#endif

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
 * CTRL+C signal handler
 */
static int dorun = 1;
static void main_stop(int signal)
{
    (void)signal;
    dorun = 0;
}

/**
 * Compare two music buffers
 */
void compare(uint8_t *buf, int len, uint8_t *out, int olen)
{
    int i, n1, n2;
    if(olen != len) printf("lengths don't match\n");
    memset(buf, 0, 20); memset(out, 0, 20); /* clear song name */
    for(i = 0; i < 31; i++) {
        memset(buf + 20 + i * 30, 0, 22);   /* clear sample names */
        memset(out + 20 + i * 30, 0, 22);
    }
    if(memcmp(out, buf, len < olen ? len : olen)) {
        printf("buffers don't match\n");
        if(buf[950] != out[950] || memcmp(buf + 952, out + 952, 127))
            printf(" - pattern orders don't match\n");
        for(n1 = 0, i = 952; i < 952 + buf[950] && 1084 + buf[i] * 1024 < len; i++) if(buf[i] > n1) n1 = buf[i];
        n1++;
        for(n2 = 0, i = 952; i < 952 + out[950] && 1084 + out[i] * 1024 < olen; i++) if(out[i] > n2) n2 = buf[i];
        n2++;
        if(n1 != n2) printf(" - number of patterns don't match\n");
        else if(memcmp(buf + 1084, out + 1084, n1 * 1024)) printf(" - patterns table don't match\n");
        if(len - n1 * 1024 != olen - n2 * 1024) printf(" - sample sizes don't match\n");
        else if(memcmp(buf + 1084 + n1 * 1024, out + 1084 + n2 * 1024, olen - 1084 - n2 * 1024))
            printf(" - sample buffers don't match\n");
    }
}

/**
 * Main function
 */
int main(int argc, char **argv)
{
    struct timespec tv;
    FILE *f;
    uint8_t *buf = NULL, *out;
    int len = 0, olen = 0;

    /* load input */
    if(argc < 1 || !argv[1]) {
        printf("MEG-4 Amiga MOD Player by bzt Copyright (C) 2023 GPLv3+\r\n\r\n");
        printf("%s <in.mod> [out.mod]\r\n", argv[0]);
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

    /* initialize the DSP */
    memset(&meg4, 0, sizeof(meg4));
    dsp_init();

    /* import as track 0 */
    if(!format_mod(0, buf, len)) {
        free(buf);
        printf("unable to load mod\n");
        exit(1);
    }

    /* save output and do further import/export checks */
    if(argc > 2) {
        printf("exporting...\n");
        out = export_mod(0, &olen);
        if(out) {
            f = fopen(argv[2], "wb");
            if(f) {
                fwrite(out, 1, olen, f);
                fclose(f);
                printf("output saved\n");
            }
            compare(buf, len, out, olen);
            free(out);
        } else printf("error generating output\n");
        free(buf);
        exit(1);
    }
    free(buf);

    /* play the music */
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);
    printf("playing, press CTRL+C to stop...\n");

    /* open audio */
#ifdef TEST_PA
    Pa_Initialize(); pa = NULL;
    if(Pa_OpenDefaultStream(&pa, 0, 1, paFloat32, 44100, 4096, main_audio, NULL) != paNoError || !pa) {
        Pa_Terminate(); printf("pa error\n"); return 1;
    }
    Pa_StartStream(pa);
#else
    memset(&want, 0, sizeof(want));
    memset(&have, 0, sizeof(have));
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 4096;
    want.callback = main_audio;
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) || !(audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0)) ||
      have.freq != 44100 || have.format != AUDIO_F32 || have.channels != 1) { printf("SDL error"); return 1; }
    SDL_PauseAudioDevice(audio, 0);
#endif

    /* play track 0 from row 0 with max volume, in the background */
    meg4_api_music(0, 0, 255);

    /* do something in the foreground */
    while(dorun && meg4.mmio.dsp_num > 0) {
        printf("\rrow %4u / %4u", meg4.mmio.dsp_row, meg4.mmio.dsp_num); fflush(stdout);
        /* wait 1/10 sec */
        tv.tv_sec = 0; tv.tv_nsec = 100000000;
        nanosleep(&tv, NULL);
    }
    printf("\nstopped\n");

    /* free resources */
    dsp_free();
#ifdef TEST_PA
    Pa_CloseStream(pa);
    Pa_Terminate();
#else
    SDL_CloseAudioDevice(audio);
    SDL_Quit();
#endif
    return 0;
}
