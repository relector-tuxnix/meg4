/*
 * meg4/platform/fbdev_alsa/main.c
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
 * @brief Linux FrameBuffer + ALSA "platform" for the MEG-4
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <linux/kd.h>
#include <linux/fb.h>
#include <linux/input.h>
#define inline __inline__
#include <alsa/asoundlib.h>
#include "meg4.h"

int fb = -1, ek = -1, em = -1;
struct fb_fix_screeninfo fb_fix;
struct fb_var_screeninfo fb_var, fb_orig;
uint32_t *scrbuf = NULL;
uint8_t *fbuf = NULL, *foffs;
struct termios oldt, newt;
snd_pcm_t *audio = NULL;
snd_async_handler_t *pcm_callback = NULL;
snd_pcm_uframes_t buffer_size = 4096;
snd_pcm_uframes_t period_size = 256;
float *abuf = NULL;
void sync(void);

int main_w = 0, main_h = 0, main_exit = 0, win_w, win_h, win_x, win_y, main_alt = 0, main_sh = 0, main_meta = 0, main_caps = 0, main_keymap[512];
int win_dp, win_dp2, win_dp3, win_dp4, win_dp5, win_dp6, win_dp7, win_dp8;
void main_delay(int msec);
/* keyboard layout mapping */
char *main_kbdlayout[128*4] = {
/* KEY_RESERVED     0 */ "", "", "", "",
/* KEY_ESC          1 */ "", "", "", "",
#ifdef KBDMAP
#include KBDMAP
#else
#include "us.h"
#endif
};
/* interface's language */
#ifndef LANG
#define LANG "en"
#endif
/* the device and partition with the "MEG-4" directory, where the floppy images reside */
#ifndef FLOPPYDEV
#define FLOPPYDEV "/dev/sda1"
#endif

#define meg4_showcursor()
#define meg4_hidecursor()
#include "../common.h"

/**
 * Exit emulator
 */
void main_quit(void)
{
#ifdef USE_INIT
    pid_t pid;
#endif
    main_log(1, "quitting...         ");
    meg4_poweroff();
    if(ek >= 0) close(ek);
    if(em >= 0) close(em);
    if(audio) {
        if(pcm_callback) snd_async_del_handler (pcm_callback);
        snd_pcm_drain(audio); snd_pcm_close(audio); audio = NULL;
    }
    if(scrbuf) { free(scrbuf); scrbuf = NULL; }
    if(fbuf) { memset(fbuf, 0, fb_fix.smem_len); msync(fbuf, fb_fix.smem_len, MS_SYNC); munmap(fbuf, fb_fix.smem_len); fbuf = NULL; }
    if(abuf) { free(abuf); abuf = NULL; }
    if(fb >= 0) {
        if(fb_orig.bits_per_pixel != 32) ioctl(fb, FBIOPUT_VSCREENINFO, &fb_orig);
        close(fb);
    }
    fprintf(stdout, "\x1b[H\x1b[2J\x1b[?25h"); fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
#ifdef USE_INIT
    /* if we're running as init, make sure everything is written out to disk, and then power off the machine */
    sync();
    umount("/mnt");
    if((pid = fork()) == 0) { reboot(RB_POWER_OFF); exit(0); }
    waitpid(pid, NULL, 0);
    sleep(1);
#endif
    exit(0);
}

/**
 * Create window
 */
void main_win(void)
{
    DIR *dh;
    struct dirent *de;
    /* Linux kernel limits device names in 256 bytes, so this must be enough even with the path prefix */
    char dev[512] = "/dev/input/by-path";
    int l;

    /* get keyboard and mouse event device */
    dh = opendir(dev); ek = em = -1; dev[18] = '/';
    while((ek < 0 || em < 0) && (de = readdir(dh)) != NULL) {
        l = strlen(de->d_name);
        if(ek == -1 && l > 9 && !strcmp(de->d_name + l - 9, "event-kbd")) {
            strcpy(dev + 19, de->d_name); ek = open(dev, O_RDONLY | O_NDELAY | O_NONBLOCK);
        }
        if(em == -1 && l > 11 && !strcmp(de->d_name + l - 11, "event-mouse")) {
            strcpy(dev + 19, de->d_name); em = open(dev, O_RDONLY | O_NDELAY | O_NONBLOCK);
        }
    }
    closedir(dh);

    /* get video device */
    tcgetattr(STDIN_FILENO, &oldt);
    memcpy(&newt, &oldt, sizeof(newt));
    newt.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt);
    fprintf(stdout, "\x1b[H\x1b[2J\x1b[?25l"); fflush(stdout);
    fb = open("/dev/fb0", O_RDWR);
    if(fb < 0) fb = open("/dev/graphics/fb0", O_RDWR);
    if(fb < 0) return;
    if(ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) != 0 || fb_var.xres < 640 || fb_var.yres < 400) { close(fb); fb = -1; return; }
    memcpy(&fb_orig, &fb_var, sizeof(struct fb_var_screeninfo));
    if(fb_var.bits_per_pixel != 32) {
        fb_var.bits_per_pixel = 32;
        if(ioctl(fb, FBIOPUT_VSCREENINFO, &fb_var) != 0) { close(fb); fb = -1; return; }
    }
    if(ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix) != 0 || fb_fix.type != FB_TYPE_PACKED_PIXELS) { close(fb); fb = -1; return; }
    fbuf = (uint8_t*)mmap(0, fb_fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if(fbuf == NULL || (intptr_t)fbuf == -1) { close(fb); fb = -1; fbuf = NULL; return; }
    memset(fbuf, 0, fb_fix.smem_len);
    main_w = fb_var.xres; main_h = fb_var.yres;
    win_w = 640; win_h = 400;
    while(main_w >= win_w + 640 && main_h >= win_h + 400) { win_w += 640; win_h += 400; }
    win_x = (main_w - win_w) >> 1; win_y = (main_h - win_h) >> 1;
    foffs = fbuf + win_y * fb_fix.line_length + (win_x << 2);
    win_dp = fb_fix.line_length >> 2; win_dp2 = win_dp << 1; win_dp3 = win_dp * 3; win_dp4 = win_dp << 2;
    win_dp5 = win_dp * 5; win_dp6 = win_dp * 6; win_dp7 = win_dp * 7; win_dp8 = win_dp << 3;
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
}

/**
 * Make window focused
 */
void main_focus(void)
{
}

/**
 * Get text from clipboard (must be freed by caller)
 */
char *main_getclipboard(void)
{
    return NULL;
}

/**
 * Set text to clipboard
 */
void main_setclipboard(char *str)
{
    (void)str;
}

/**
 * Show on-screen keyboard
 */
void main_osk_show(void)
{
}

/**
 * Hide on-screen keyboard
 */
void main_osk_hide(void)
{
}

/**
 * Audio callback
 */
void main_audio(snd_async_handler_t *pcm_callback)
{
    snd_pcm_t *pcm_handle = snd_async_handler_get_pcm(pcm_callback);
    snd_pcm_uframes_t avail = snd_pcm_avail_update(pcm_handle);
    while(avail >= period_size) {
        meg4_audiofeed((float*)abuf, period_size);
        snd_pcm_writei(pcm_handle, abuf, period_size);
        avail = snd_pcm_avail_update(pcm_handle);
    }
}

/**
 * Delay
 */
void main_delay(int msec)
{
    struct timespec tv;
    tv.tv_sec = 0; tv.tv_nsec = msec * 1000000;
    while(nanosleep(&tv, &tv) == -1 && tv.tv_nsec > 1000000);
}

/**
 * Print program version and copyright
 */
void main_hdr(void)
{
    printf("\r\nMEG-4 v%s (fbdev, build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, BUILD);
}

/**
 * The real main procedure
 */
int main(int argc, char **argv)
{
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_hw_params_t *hw_params;
    struct input_event ev[64];
    struct timespec ts;
    int i, j, k, l, m, p, mx = 160, my = 100;
    char *infile = NULL, *fn;
    char s[5];
    uint32_t *src, *dst, rrate = 44100;
    uint8_t *ptr;
    int32_t tickdiff;
    uint64_t ticks;
#ifdef USE_INIT
    char *lng = LANG;
    (void)argc; (void)argv;
    /* we must do some housekeeping when we run as the init process */
    mkdir("/dev", 0777); mount("none", "/dev", "devtmpfs", 0, NULL);
    mkdir("/proc", 0777); mount("none", "/proc", "procfs", 0, NULL);
    mkdir("/sys", 0777); mount("none", "/sys", "sysfs", 0, NULL);
    mkdir("/tmp", 0777); mount("none", "/tmp", "tmpfs", 0, NULL);
    mkdir("/mnt", 0777); mount(FLOPPYDEV, "/mnt", "auto", MS_SYNCHRONOUS | MS_NODEV | MS_NOEXEC, NULL);
    open("/dev/stdin", O_RDONLY);
    open("/dev/stdout", O_WRONLY);
    open("/dev/stderr", O_WRONLY);
    main_floppydir = "/mnt/MEG-4"; mkdir(main_floppydir, 0777);
    verbose = 0;
#else
    char *lng = getenv("LANG");
    main_parsecommandline(argc, argv, &lng, &infile);
    main_hdr();
    for(i = 0; i < 3; i++) printf("  %s\r\n", copyright[i]);
    printf("\r\n");
    fflush(stdout);
#endif

    /* set up keymap */
    memset(main_keymap, 0, sizeof(main_keymap));
    main_keymap[KEY_SPACE] = MEG4_KEY_SPACE;
    main_keymap[KEY_APOSTROPHE] = MEG4_KEY_APOSTROPHE;
    main_keymap[KEY_COMMA] = MEG4_KEY_COMMA;
    main_keymap[KEY_MINUS] = MEG4_KEY_MINUS;
    main_keymap[KEY_DOT] = MEG4_KEY_PERIOD;
    main_keymap[KEY_SLASH] = MEG4_KEY_SLASH;
    main_keymap[KEY_0] = MEG4_KEY_0;
    main_keymap[KEY_1] = MEG4_KEY_1;
    main_keymap[KEY_2] = MEG4_KEY_2;
    main_keymap[KEY_3] = MEG4_KEY_3;
    main_keymap[KEY_4] = MEG4_KEY_4;
    main_keymap[KEY_5] = MEG4_KEY_5;
    main_keymap[KEY_6] = MEG4_KEY_6;
    main_keymap[KEY_7] = MEG4_KEY_7;
    main_keymap[KEY_8] = MEG4_KEY_8;
    main_keymap[KEY_9] = MEG4_KEY_9;
    main_keymap[KEY_SEMICOLON] = MEG4_KEY_SEMICOLON;
    main_keymap[KEY_EQUAL] = MEG4_KEY_EQUAL;
    main_keymap[KEY_A] = MEG4_KEY_A;
    main_keymap[KEY_B] = MEG4_KEY_B;
    main_keymap[KEY_C] = MEG4_KEY_C;
    main_keymap[KEY_D] = MEG4_KEY_D;
    main_keymap[KEY_E] = MEG4_KEY_E;
    main_keymap[KEY_F] = MEG4_KEY_F;
    main_keymap[KEY_G] = MEG4_KEY_G;
    main_keymap[KEY_H] = MEG4_KEY_H;
    main_keymap[KEY_I] = MEG4_KEY_I;
    main_keymap[KEY_J] = MEG4_KEY_J;
    main_keymap[KEY_K] = MEG4_KEY_K;
    main_keymap[KEY_L] = MEG4_KEY_L;
    main_keymap[KEY_M] = MEG4_KEY_M;
    main_keymap[KEY_N] = MEG4_KEY_N;
    main_keymap[KEY_O] = MEG4_KEY_O;
    main_keymap[KEY_P] = MEG4_KEY_P;
    main_keymap[KEY_Q] = MEG4_KEY_Q;
    main_keymap[KEY_R] = MEG4_KEY_R;
    main_keymap[KEY_S] = MEG4_KEY_S;
    main_keymap[KEY_T] = MEG4_KEY_T;
    main_keymap[KEY_U] = MEG4_KEY_U;
    main_keymap[KEY_V] = MEG4_KEY_V;
    main_keymap[KEY_X] = MEG4_KEY_X;
    main_keymap[KEY_Y] = MEG4_KEY_Y;
    main_keymap[KEY_Z] = MEG4_KEY_Z;
    main_keymap[KEY_LEFTBRACE] = MEG4_KEY_LBRACKET;
    main_keymap[KEY_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[KEY_RIGHTBRACE] = MEG4_KEY_RBRACKET;
    main_keymap[KEY_ENTER] = MEG4_KEY_ENTER;
    main_keymap[KEY_TAB] = MEG4_KEY_TAB;
    main_keymap[KEY_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[KEY_INSERT] = MEG4_KEY_INS;
    main_keymap[KEY_DELETE] = MEG4_KEY_DEL;
    main_keymap[KEY_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[KEY_LEFT] = MEG4_KEY_LEFT;
    main_keymap[KEY_DOWN] = MEG4_KEY_DOWN;
    main_keymap[KEY_UP] = MEG4_KEY_UP;
    main_keymap[KEY_PAGEUP] = MEG4_KEY_PGUP;
    main_keymap[KEY_PAGEDOWN] = MEG4_KEY_PGDN;
    main_keymap[KEY_HOME] = MEG4_KEY_HOME;
    main_keymap[KEY_END] = MEG4_KEY_END;
    main_keymap[KEY_CAPSLOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[KEY_SCROLLLOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[KEY_NUMLOCK] = MEG4_KEY_NUMLOCK;
    main_keymap[KEY_PRINT] = MEG4_KEY_PRSCR;
    main_keymap[KEY_PAUSE] = MEG4_KEY_PAUSE;
    main_keymap[KEY_F1] = MEG4_KEY_F1;
    main_keymap[KEY_F2] = MEG4_KEY_F2;
    main_keymap[KEY_F3] = MEG4_KEY_F3;
    main_keymap[KEY_F4] = MEG4_KEY_F4;
    main_keymap[KEY_F5] = MEG4_KEY_F5;
    main_keymap[KEY_F6] = MEG4_KEY_F6;
    main_keymap[KEY_F7] = MEG4_KEY_F7;
    main_keymap[KEY_F8] = MEG4_KEY_F8;
    main_keymap[KEY_F9] = MEG4_KEY_F9;
    main_keymap[KEY_F10] = MEG4_KEY_F10;
    main_keymap[KEY_F11] = MEG4_KEY_F11;
    main_keymap[KEY_F12] = MEG4_KEY_F12;
    main_keymap[KEY_KP0] = MEG4_KEY_KP_0;
    main_keymap[KEY_KP1] = MEG4_KEY_KP_1;
    main_keymap[KEY_KP2] = MEG4_KEY_KP_2;
    main_keymap[KEY_KP3] = MEG4_KEY_KP_3;
    main_keymap[KEY_KP4] = MEG4_KEY_KP_4;
    main_keymap[KEY_KP5] = MEG4_KEY_KP_5;
    main_keymap[KEY_KP6] = MEG4_KEY_KP_6;
    main_keymap[KEY_KP7] = MEG4_KEY_KP_7;
    main_keymap[KEY_KP8] = MEG4_KEY_KP_8;
    main_keymap[KEY_KP9] = MEG4_KEY_KP_9;
    main_keymap[KEY_KPDOT] = MEG4_KEY_KP_DEC;
    main_keymap[KEY_KPSLASH] = MEG4_KEY_KP_DIV;
    main_keymap[KEY_KPASTERISK] = MEG4_KEY_KP_MUL;
    main_keymap[KEY_KPMINUS] = MEG4_KEY_KP_SUB;
    main_keymap[KEY_KPPLUS] = MEG4_KEY_KP_ADD;
    main_keymap[KEY_KPENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[KEY_KPEQUAL] = MEG4_KEY_KP_EQUAL;
    main_keymap[KEY_LEFTSHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[KEY_LEFTCTRL] = MEG4_KEY_LCTRL;
    main_keymap[KEY_LEFTALT] = MEG4_KEY_LALT;
    main_keymap[KEY_LEFTMETA] = MEG4_KEY_LSUPER;
    main_keymap[KEY_RIGHTSHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[KEY_RIGHTCTRL] = MEG4_KEY_RCTRL;
    main_keymap[KEY_RIGHTALT] = MEG4_KEY_RALT;
    main_keymap[KEY_RIGHTMETA] = MEG4_KEY_RSUPER;
    main_keymap[KEY_MENU] = MEG4_KEY_MENU;

    /* huh, wanna see some seriously and conceptually fucked up api? */
    if(snd_pcm_open(&audio, "default", SND_PCM_STREAM_PLAYBACK, 0) >= 0) {
        snd_pcm_hw_params_malloc(&hw_params);
        snd_pcm_hw_params_any(audio, hw_params);
        snd_pcm_hw_params_set_access(audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(audio, hw_params, SND_PCM_FORMAT_FLOAT);
        snd_pcm_hw_params_set_rate_near(audio, hw_params, &rrate, NULL);
        snd_pcm_hw_params_set_channels(audio, hw_params, 1);
        snd_pcm_hw_params_set_buffer_size_near(audio, hw_params, &buffer_size);
        snd_pcm_hw_params_set_period_size_near(audio, hw_params, &period_size, NULL);
        if(snd_pcm_hw_params(audio, hw_params) >= 0) {
            snd_pcm_sw_params_malloc(&sw_params);
            snd_pcm_sw_params_current(audio, sw_params);
            snd_pcm_sw_params_set_start_threshold(audio, sw_params, buffer_size - period_size);
            snd_pcm_sw_params_set_avail_min(audio, sw_params, period_size);
            if(snd_pcm_sw_params(audio, sw_params) >= 0) {
                abuf = (float*)malloc(period_size * sizeof(float));
                if(abuf) {
                    memset(abuf, 0, period_size * sizeof(float));
                    snd_pcm_prepare(audio);
                    snd_async_add_pcm_handler(&pcm_callback, audio, main_audio, NULL);
                    snd_pcm_writei(audio, abuf, period_size);
                } else { snd_pcm_close(audio); audio = NULL; }
            } else { snd_pcm_close(audio); audio = NULL; }
            snd_pcm_sw_params_free(sw_params);
        } else { snd_pcm_close(audio); audio = NULL; }
        snd_pcm_hw_params_free(hw_params);
    } else audio = NULL;
    if(verbose && audio) main_log(1, "audio opened %uHz, %u bits", rrate, 32);

    scrbuf = (uint32_t*)malloc(640 * 400 * sizeof(uint32_t));
    if(!scrbuf) {
        main_log(0, "unable to allocate screen buffer");
        return 1;
    }
    main_win();
    if(fbuf == NULL) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
        free(scrbuf);
        main_log(0, "unable to initialize framebuffer");
        return 1;
    }
    /* turn on the emulator */
    meg4_poweron(lng);
#ifndef NOEDITORS
    if(infile) {
        if((ptr = main_readfile(infile, &i))) {
            fn = strrchr(infile, SEP[0]); if(!fn) fn = infile; else fn++;
            meg4_insert(fn, ptr, i);
            free(ptr);
        }
    }
#else
    (void)ptr; (void)infile;
#endif
    if(audio) snd_pcm_start(audio);
    while(!main_exit) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ticks = (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
        /* mouse events */
        if(em >= 0 && (k = read(em, ev, sizeof(ev))) >= (int)sizeof(ev[0])) {
            for(i = 0, k /= sizeof(ev[0]); i < k; i++)
                if(ev[i].type == EV_KEY && ev[i].value >= 0 && ev[i].value <= 1) {
                    j = ev[i].code == BTN_LEFT ? MEG4_BTN_L : (ev[i].code == BTN_RIGHT ? MEG4_BTN_R : MEG4_BTN_M);
                    if(ev[i].value) meg4_setbtn(j); else meg4_clrbtn(j);
                } else
                if(ev[i].type == EV_REL)
                    switch(ev[i].code) {
                        case REL_X:
                            mx += ev[i].value;
                            if(mx < 0) mx = 0;
                            if(mx > meg4.screen.w) mx = meg4.screen.w;
                            meg4_setptr(mx, my);
                        break;
                        case REL_Y:
                            my += ev[i].value;
                            if(my < 0) my = 0;
                            if(my > meg4.screen.h) my = meg4.screen.h;
                            meg4_setptr(mx, my);
                        break;
                        case REL_WHEEL: meg4_setscr(ev[i].value > 0, ev[i].value < 0, 0, 0); break;
                    }
        }
        /* keyboard events */
        if(ek >= 0 && (k = read(ek, ev, sizeof(ev))) >= (int)sizeof(ev[0])) {
            for(i = 0, k /= sizeof(ev[0]); i < k; i++)
                if(ev[i].type == EV_KEY && ev[i].value >= 0 && ev[i].value <= 2) {
                    if(ev[i].code > 0 && ev[i].code < (sizeof(main_keymap)/sizeof(main_keymap[0]))) {
                        if(ev[i].value == 0) {
                            /* key release */
                            if(ev[i].code == KEY_LEFTCTRL || ev[i].code == KEY_LEFTALT) main_alt = 0;
                            if(ev[i].code == KEY_LEFTMETA || ev[i].code == KEY_RIGHTMETA || ev[i].code == KEY_RIGHTALT) main_meta = 0;
                            if(ev[i].code == KEY_LEFTSHIFT || ev[i].code == KEY_RIGHTSHIFT) main_sh = 0;
                            if(ev[i].code == KEY_CAPSLOCK) main_caps = 0;
                            meg4_clrkey(main_keymap[ev[i].code]);
                        } else
                        if(ev[i].value == 1) {
                            /* key press */
                            if(ev[i].code == KEY_LEFTCTRL || ev[i].code == KEY_LEFTALT) main_alt = 1;
                            if(ev[i].code == KEY_LEFTMETA || ev[i].code == KEY_RIGHTMETA || ev[i].code == KEY_RIGHTALT) main_meta = 1;
                            if(ev[i].code == KEY_LEFTSHIFT || ev[i].code == KEY_RIGHTSHIFT) main_sh = 1;
                            if(ev[i].code == KEY_CAPSLOCK) main_caps = 1;
                            switch(ev[i].code) {
                                case KEY_ESC: if(main_alt) main_exit = 1; else meg4_pushkey("\x1b\0\0"); break;
                                case KEY_F1: meg4_pushkey("F1\0"); break;
                                case KEY_F2: meg4_pushkey("F2\0"); break;
                                case KEY_F3: meg4_pushkey("F3\0"); break;
                                case KEY_F4: meg4_pushkey("F4\0"); break;
                                case KEY_F5: meg4_pushkey("F5\0"); break;
                                case KEY_F6: meg4_pushkey("F6\0"); break;
                                case KEY_F7: meg4_pushkey("F7\0"); break;
                                case KEY_F8: meg4_pushkey("F8\0"); break;
                                case KEY_F9: meg4_pushkey("F9\0"); break;
                                case KEY_F10: meg4_pushkey("F10"); break;
                                case KEY_F11: meg4_pushkey("F11"); break;
                                case KEY_F12: meg4_pushkey("F12"); break;
                                case KEY_PRINT: meg4_pushkey("PSc"); break;
                                case KEY_SCROLLLOCK: meg4_pushkey("SLk"); break;
                                case KEY_NUMLOCK: meg4_pushkey("NLk"); break;
                                case KEY_BACKSPACE: meg4_pushkey("\b\0\0"); break;
                                case KEY_TAB: meg4_pushkey("\t\0\0"); break;
                                case KEY_ENTER: case KEY_KPENTER: meg4_pushkey("\n\0\0"); break;
                                case KEY_CAPSLOCK: meg4_pushkey("CLk"); break;
                                case KEY_UP: meg4_pushkey("Up\0"); break;
                                case KEY_DOWN: meg4_pushkey("Down"); break;
                                case KEY_LEFT: meg4_pushkey("Left"); break;
                                case KEY_RIGHT: meg4_pushkey("Rght"); break;
                                case KEY_HOME: meg4_pushkey("Home"); break;
                                case KEY_END: meg4_pushkey("End"); break;
                                case KEY_PAGEUP: meg4_pushkey("PgUp"); break;
                                case KEY_PAGEDOWN: meg4_pushkey("PgDn"); break;
                                case KEY_INSERT: meg4_pushkey("Ins"); break;
                                case KEY_DELETE: meg4_pushkey("Del"); break;
                                default:
                                    if(ev[i].code < sizeof(main_kbdlayout)/(sizeof(main_kbdlayout[0])*4)) {
                                        memset(s, 0, sizeof(s));
                                        strncpy(s, main_kbdlayout[(ev[i].code << 2) | (main_meta << 1) | (main_caps ^ main_sh)], 4);
                                        if(s[0]) meg4_pushkey(s);
                                        if(main_alt && s[0] == 'q' && !s[1]) main_exit = 1;
                                    }
                                break;
                            }
                            meg4_setkey(main_keymap[ev[i].code]);
                        }
                    }
                }
        }
        /* controller events */
        /* TODO: handle gamepads */

        /* run the emulator and display screen */
        meg4_run();
        meg4_redraw(scrbuf, 640, 400, 640 * 4);
        src = scrbuf + (le16toh(meg4.mmio.scrx) > 320 || le16toh(meg4.mmio.scry) > 200 ? 0 : le16toh(meg4.mmio.scry) * 640 + le16toh(meg4.mmio.scrx));
        dst = (uint32_t*)foffs;
        /* optimized upscaler for 1x, 2x, 3x, 4x, 6x and 8x */
        p = win_h / meg4.screen.h;
        if(!fb_var.red.offset) {
            /* RGBA */
            switch(p) {
                case 1:
                    for(j = 0, i = meg4.screen.w << 2; j < meg4.screen.h; j++, src += 640, dst += win_dp)
                        memcpy(dst, src, i);
                break;
                case 2:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp2)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 2)
                            dst[k] = dst[k + 1] = dst[k + win_dp] = dst[k + win_dp + 1] = src[i];
                break;
                case 3:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp3)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 3)
                            dst[k] = dst[k + 1] = dst[k + 2] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = src[i];
                break;
                case 4:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp4)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 4)
                            dst[k] = dst[k + 1] = dst[k + 2] = dst[k + 3] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] = dst[k + win_dp + 3] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = dst[k + win_dp2 + 3] =
                            dst[k + win_dp3] = dst[k + win_dp3 + 1] = dst[k + win_dp3 + 2] = dst[k + win_dp3 + 3] = src[i];
                break;
                case 6:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp6)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 6)
                            dst[k] = dst[k + 1] = dst[k + 2] = dst[k + 3] = dst[k + 4] = dst[k + 5] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] = dst[k + win_dp + 3] = dst[k + win_dp + 4] = dst[k + win_dp + 5] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = dst[k + win_dp2 + 3] =
                            dst[k + win_dp2 + 4] = dst[k + win_dp2 + 5] =
                            dst[k + win_dp3] = dst[k + win_dp3 + 1] = dst[k + win_dp3 + 2] = dst[k + win_dp3 + 3] =
                            dst[k + win_dp3 + 4] = dst[k + win_dp3 + 5] =
                            dst[k + win_dp4] = dst[k + win_dp4 + 1] = dst[k + win_dp4 + 2] = dst[k + win_dp4 + 3] =
                            dst[k + win_dp4 + 4] = dst[k + win_dp4 + 5] =
                            dst[k + win_dp5] = dst[k + win_dp5 + 1] = dst[k + win_dp5 + 2] = dst[k + win_dp5 + 3] =
                            dst[k + win_dp5 + 4] = dst[k + win_dp5 + 5] = src[i];
                break;
                case 8:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp8)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 8)
                            dst[k] = dst[k + 1] = dst[k + 2] = dst[k + 3] = dst[k + 4] = dst[k + 5] = dst[k + 6] = dst[k + 7] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] = dst[k + win_dp + 3] =
                            dst[k + win_dp + 4] = dst[k + win_dp + 5] = dst[k + win_dp + 6] = dst[k + win_dp + 7] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = dst[k + win_dp2 + 3] =
                            dst[k + win_dp2 + 4] = dst[k + win_dp2 + 5] = dst[k + win_dp2 + 6] = dst[k + win_dp2 + 7] =
                            dst[k + win_dp3] = dst[k + win_dp3 + 1] = dst[k + win_dp3 + 2] = dst[k + win_dp3 + 3] =
                            dst[k + win_dp3 + 4] = dst[k + win_dp3 + 5] = dst[k + win_dp3 + 6] = dst[k + win_dp3 + 7] =
                            dst[k + win_dp4] = dst[k + win_dp4 + 1] = dst[k + win_dp4 + 2] = dst[k + win_dp4 + 3] =
                            dst[k + win_dp4 + 4] = dst[k + win_dp4 + 5] = dst[k + win_dp4 + 6] = dst[k + win_dp4 + 7] =
                            dst[k + win_dp5] = dst[k + win_dp5 + 1] = dst[k + win_dp5 + 2] = dst[k + win_dp5 + 3] =
                            dst[k + win_dp5 + 4] = dst[k + win_dp5 + 5] = dst[k + win_dp5 + 6] = dst[k + win_dp5 + 7] =
                            dst[k + win_dp6] = dst[k + win_dp6 + 1] = dst[k + win_dp6 + 2] = dst[k + win_dp6 + 3] =
                            dst[k + win_dp6 + 4] = dst[k + win_dp6 + 5] = dst[k + win_dp6 + 6] = dst[k + win_dp6 + 7] =
                            dst[k + win_dp7] = dst[k + win_dp7 + 1] = dst[k + win_dp7 + 2] = dst[k + win_dp7 + 3] =
                            dst[k + win_dp7 + 4] = dst[k + win_dp7 + 5] = dst[k + win_dp7 + 6] = dst[k + win_dp7 + 7] = src[i];
                break;
                default:
                    for(j = 0; j < meg4.screen.h; j++, src += 640)
                        for(m = 0; m < p; m++, dst += win_dp)
                            for(i = k = 0; i < meg4.screen.w; i++)
                                for(l = 0; l < p; l++, k++)
                                    dst[k] = src[i];
                break;
            }
        } else {
            /* BGRA */
            switch(p) {
                case 1:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp)
                        for(i = 0; i < meg4.screen.w; i++)
                            dst[i] = ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
                case 2:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp2)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 2)
                            dst[k] = dst[k + 1] = dst[k + win_dp] = dst[k + win_dp + 1] =
                                ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
                case 3:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp3)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 3)
                            dst[k] = dst[k + 1] = dst[k + 2] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] =
                                ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
                case 4:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp4)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 4)
                            dst[k] = dst[k + 1] = dst[k + 2] = dst[k + 3] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] = dst[k + win_dp + 3] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = dst[k + win_dp2 + 3] =
                            dst[k + win_dp3] = dst[k + win_dp3 + 1] = dst[k + win_dp3 + 2] = dst[k + win_dp3 + 3] =
                                ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
                case 6:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp6)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 6)
                            dst[k] = dst[k + 1] = dst[k + 2] = dst[k + 3] = dst[k + 4] = dst[k + 5] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] = dst[k + win_dp + 3] = dst[k + win_dp + 4] = dst[k + win_dp + 5] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = dst[k + win_dp2 + 3] =
                            dst[k + win_dp2 + 4] = dst[k + win_dp2 + 5] =
                            dst[k + win_dp3] = dst[k + win_dp3 + 1] = dst[k + win_dp3 + 2] = dst[k + win_dp3 + 3] =
                            dst[k + win_dp3 + 4] = dst[k + win_dp3 + 5] =
                            dst[k + win_dp4] = dst[k + win_dp4 + 1] = dst[k + win_dp4 + 2] = dst[k + win_dp4 + 3] =
                            dst[k + win_dp4 + 4] = dst[k + win_dp4 + 5] =
                            dst[k + win_dp5] = dst[k + win_dp5 + 1] = dst[k + win_dp5 + 2] = dst[k + win_dp5 + 3] =
                            dst[k + win_dp5 + 4] = dst[k + win_dp5 + 5] =
                                ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
                case 8:
                    for(j = 0; j < meg4.screen.h; j++, src += 640, dst += win_dp8)
                        for(i = k = 0; i < meg4.screen.w; i++, k += 8)
                            dst[k] = dst[k + 1] = dst[k + 2] = dst[k + 3] = dst[k + 4] = dst[k + 5] = dst[k + 6] = dst[k + 7] =
                            dst[k + win_dp] = dst[k + win_dp + 1] = dst[k + win_dp + 2] = dst[k + win_dp + 3] =
                            dst[k + win_dp + 4] = dst[k + win_dp + 5] = dst[k + win_dp + 6] = dst[k + win_dp + 7] =
                            dst[k + win_dp2] = dst[k + win_dp2 + 1] = dst[k + win_dp2 + 2] = dst[k + win_dp2 + 3] =
                            dst[k + win_dp2 + 4] = dst[k + win_dp2 + 5] = dst[k + win_dp2 + 6] = dst[k + win_dp2 + 7] =
                            dst[k + win_dp3] = dst[k + win_dp3 + 1] = dst[k + win_dp3 + 2] = dst[k + win_dp3 + 3] =
                            dst[k + win_dp3 + 4] = dst[k + win_dp3 + 5] = dst[k + win_dp3 + 6] = dst[k + win_dp3 + 7] =
                            dst[k + win_dp4] = dst[k + win_dp4 + 1] = dst[k + win_dp4 + 2] = dst[k + win_dp4 + 3] =
                            dst[k + win_dp4 + 4] = dst[k + win_dp4 + 5] = dst[k + win_dp4 + 6] = dst[k + win_dp4 + 7] =
                            dst[k + win_dp5] = dst[k + win_dp5 + 1] = dst[k + win_dp5 + 2] = dst[k + win_dp5 + 3] =
                            dst[k + win_dp5 + 4] = dst[k + win_dp5 + 5] = dst[k + win_dp5 + 6] = dst[k + win_dp5 + 7] =
                            dst[k + win_dp6] = dst[k + win_dp6 + 1] = dst[k + win_dp6 + 2] = dst[k + win_dp6 + 3] =
                            dst[k + win_dp6 + 4] = dst[k + win_dp6 + 5] = dst[k + win_dp6 + 6] = dst[k + win_dp6 + 7] =
                            dst[k + win_dp7] = dst[k + win_dp7 + 1] = dst[k + win_dp7 + 2] = dst[k + win_dp7 + 3] =
                            dst[k + win_dp7 + 4] = dst[k + win_dp7 + 5] = dst[k + win_dp7 + 6] = dst[k + win_dp7 + 7] =
                                ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
                default:
                    for(j = 0; j < meg4.screen.h; j++, src += 640)
                        for(m = 0; m < p; m++, dst += win_dp)
                            for(i = k = 0; i < meg4.screen.w; i++)
                                for(l = 0; l < p; l++, k++)
                                    dst[k] = ((src[i] & 0xff) << 16) | (src[i] & 0xff00) | ((src[i] >> 16) & 0xff);
                break;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &ts);
        tickdiff = ((1000000000/60) - (((uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec) - ticks)) / 1000000;
        if(tickdiff > 0 && tickdiff < 1000) main_delay(tickdiff);
    }
    main_quit();
    return 0;
}
