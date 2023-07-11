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
#include <pthread.h>
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
#include <sound/asound.h>
#include "meg4.h"

#define ALSA_BUFFER_SIZE 4096
#define ALSA_PERIOD_SIZE 1024

enum { KBD, MOUSE, PAD };

int fb = -1, en = 0, ed[256], *sax = NULL, *say = NULL;
struct fb_fix_screeninfo fb_fix;
struct fb_var_screeninfo fb_var, fb_orig;
uint32_t *scrbuf = NULL;
uint8_t *fbuf = NULL, *foffs, *ffull;
struct termios oldt, newt;
int afd = -1, audio = 0;
volatile unsigned int period_size, boundary;
struct snd_pcm_mmap_status *mmap_status;
struct snd_pcm_mmap_control *mmap_control;
float abuf[ALSA_BUFFER_SIZE];
int16_t ibuf[2*ALSA_BUFFER_SIZE];
pthread_t th = 0;
void sync(void);

int main_w = 0, main_h = 0, main_exit = 0, main_alt = 0, main_sh = 0, main_meta = 0, main_caps = 0, main_keymap[512], main_kbd = 0;
int win_f = 0, win_w, win_h, win_fw, win_fh, win_dp, win_dp2, win_dp3, win_dp4, win_dp5, win_dp6, win_dp7, win_dp8;
void main_delay(int msec);
/* keyboard layout mapping */
const char *main_kbdlayout[2][128*4] = { {
/* KEY_RESERVED     0 */ "", "", "", "",    /* first layout is user configurable */
/* KEY_ESC          1 */ "", "", "", "",
#ifdef KBDMAP
#include KBDMAP
#else
#include "us.h"
#endif
}, {
/* KEY_RESERVED     0 */ "", "", "", "",    /* second layout is always International */
/* KEY_ESC          1 */ "", "", "", "",
#include "us.h"
} };
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

/* helpers for hw_params */
static struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit)
{
    struct snd_mask *m = param_to_mask(p, n);
    if (bit >= SNDRV_MASK_MAX)
        return;
    m->bits[0] = 0;
    m->bits[1] = 0;
    m->bits[bit >> 5] |= (1 << (bit & 31));
}

static void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    struct snd_interval *i = param_to_interval(p, n);
    i->min = val;
}

static void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    struct snd_interval *i = param_to_interval(p, n);
    i->min = val;
    i->max = val;
    i->integer = 1;
}

static unsigned int param_get_int(struct snd_pcm_hw_params *p, int n)
{
    struct snd_interval *i = param_to_interval(p, n);
    return (i->integer) ? i->max : 0;
}

static void param_init(struct snd_pcm_hw_params *p)
{
    struct snd_mask *m;
    struct snd_interval *i;
    int n;

    memset(p, 0, sizeof(struct snd_pcm_hw_params));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            m = param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            i = param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
    p->rmask = ~0U;
    p->cmask = 0;
    p->info = ~0U;
}

/**
 * Exit emulator
 */
void main_quit(void)
{
#ifdef USE_INIT
    pid_t pid;
#endif
    int i;

    main_log(1, "quitting...         ");
    meg4_poweroff();
    for(i = 0; i < en; i += 2)
        if(ed[i] > 0) { close(ed[i]); ed[i] = -1; }
    if(audio) {
        period_size = 0;
        if(th) { pthread_cancel(th); pthread_join(th, NULL); th = 0; }
        if(mmap_status) munmap(mmap_status, 4096);
        if(mmap_control) munmap(mmap_control, 4096);
        if(afd > 0) { ioctl(afd, SNDRV_PCM_IOCTL_DRAIN); close(afd); afd = -1; }
    }
    if(scrbuf) { free(scrbuf); scrbuf = NULL; }
    if(sax) { free(sax); sax = NULL; }
    if(say) { free(say); say = NULL; }
    if(fbuf) { memset(fbuf, 0, fb_fix.smem_len); msync(fbuf, fb_fix.smem_len, MS_SYNC); munmap(fbuf, fb_fix.smem_len); fbuf = NULL; }
    if(fb >= 0) {
        if(fb_orig.bits_per_pixel != 32) ioctl(fb, FBIOPUT_VSCREENINFO, &fb_orig);
        close(fb);
        fprintf(stdout, "\x1b[H\x1b[2J");
    }
    fprintf(stdout, "\x1b[?0c\x1b[?1q\x1b[?25h"); fflush(stdout);
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
    char dev[512] = "/dev/input", *fbd;
    int l, x, y, k = 0, c = 0;
    uint64_t ev_type, key_type[KEY_MAX/64 + 1];

    tcgetattr(STDIN_FILENO, &oldt);
    memcpy(&newt, &oldt, sizeof(newt));
    oldt.c_lflag |= ECHO;
    newt.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt);

    /* get keyboard, mouse and gamepad event devices */
    dh = opendir(dev); en = 0; memset(ed, 0, sizeof(ed)); dev[10] = '/';
    if(dh) {
        while(en < 256 && (de = readdir(dh)) != NULL) {
            if(memcmp(de->d_name, "event", 5)) continue;
            strcpy(dev + 11, de->d_name);
            ed[en] = open(dev, O_RDONLY | O_NDELAY | O_NONBLOCK);
            if(ed[en] > 0) {
                /* detect event device's type */
                ev_type = 0; memset(key_type, 0, sizeof(key_type));
                if(ioctl(ed[en], EVIOCGBIT(0, sizeof(ev_type)), &ev_type) >= 0 && (ev_type & (1 << EV_KEY)) &&
                  ioctl(ed[en], EVIOCGBIT(EV_KEY, KEY_MAX+1), &key_type) >= 0) {
                    /* let's see what keys are produced with EV_KEY events */
                    if(key_type[BTN_GAMEPAD / 64] & (1UL << (BTN_GAMEPAD & 63))) { ed[en + 1] = PAD + c; c++; en += 2; } else
                    if(key_type[BTN_MOUSE / 64] & (1UL << (BTN_MOUSE & 63))) { ed[en + 1] = MOUSE; en += 2; } else
                    if(key_type[KEY_ENTER / 64] & (1UL << (KEY_ENTER & 63))) { ed[en + 1] = KBD; k++; en += 2; } else ev_type = 0;
                }
                if(!ev_type) { close(ed[en]); ed[en] = -1; }
            }
        }
        closedir(dh);
    }
    if(k < 1) { main_log(0, "no keyboard device"); return; }

    /* get video device */
    fbd = "/dev/fb0"; fb = open(fbd, O_RDWR);
    if(fb < 0) { fbd = "/dev/graphics/fb0"; fb = open(fbd, O_RDWR); }
    if(fb < 0) return;
    /* configure framebuffer */
    if(ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) != 0 || fb_var.xres < 640 || fb_var.yres < 400) {
        /* try to set resolution */
        close(fb);
        l = open("/sys/class/graphics/fb0/mode", O_WRONLY); if(l > 0) { write(l, "U:1280x800p-0\n", 14); close(l); }
        fb = open(fbd, O_RDWR); if(fb < 0) return;
        if(ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) != 0 || fb_var.xres < 640 || fb_var.yres < 400) { close(fb); fb = -1; return; }
    }
    memcpy(&fb_orig, &fb_var, sizeof(struct fb_var_screeninfo));
    /* clear screen and hide cursor */
    fprintf(stdout, "\x1b[H\x1b[2J\x1b[?12l\x1b[?17c\x1b[?2q\x1b[?25l"); fflush(stdout);
    l = open("/sys/class/graphics/fbcon/cursor_blink", O_WRONLY); if(l > 0) { write(l, "0\n", 2); close(l); }
    if(fb_var.bits_per_pixel != 32) {
        fb_var.bits_per_pixel = 32;
        if(ioctl(fb, FBIOPUT_VSCREENINFO, &fb_var) != 0 ||
           ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) != 0 || fb_var.bits_per_pixel != 32) { close(fb); fb = -1; return; }
    }
    if(ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix) != 0 || fb_fix.type != FB_TYPE_PACKED_PIXELS) { close(fb); fb = -1; return; }
    fbuf = (uint8_t*)mmap(0, fb_fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if(fbuf == NULL || (intptr_t)fbuf == -1) { close(fb); fb = -1; fbuf = NULL; return; }
    memset(fbuf, 0, fb_fix.smem_len);
    /* calculate windowed and full screen sizes, keeping aspect ratio */
    main_w = fb_var.xres; main_h = fb_var.yres;
    win_w = 640; win_h = 400;
    while(main_w >= win_w + 640 && main_h >= win_h + 400) { win_w += 640; win_h += 400; }
    x = (main_w - win_w) >> 1; y = (main_h - win_h) >> 1;
    foffs = fbuf + y * fb_fix.line_length + (x << 2);
    win_fw = main_w; win_fh = 400 * main_w / 640;
    if(win_fh > main_h) { win_fh = main_h; win_fw = 640 * main_h / 400; }
    x = (main_w - win_fw) >> 1; y = (main_h - win_fh) >> 1;
    ffull = fbuf + y * fb_fix.line_length + (x << 2);
    l = (win_fw > win_fh ? win_fw : win_fh) + 1;
    sax = (int*)malloc(l * sizeof(int)); say = (int*)malloc(l * sizeof(int));
    win_dp = fb_fix.line_length >> 2; win_dp2 = win_dp << 1; win_dp3 = win_dp * 3; win_dp4 = win_dp << 2;
    win_dp5 = win_dp * 5; win_dp6 = win_dp * 6; win_dp7 = win_dp * 7; win_dp8 = win_dp << 3;
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    memset(fbuf, 0, fb_fix.smem_len);
    win_f ^= 1;
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
static void *main_audio(void *data)
{
    struct snd_xferi xfer = { 0 };
    int16_t *buf;
    unsigned int i, j, numframes = period_size;
    int ret, avail;

    (void)data;
    while(period_size) {
        numframes = period_size;
        meg4_audiofeed((float*)abuf, numframes);
        for(i = j = 0; i < numframes; i++, j += 2)
            ibuf[j] = ibuf[j + 1] = abuf[i] * 32767.0f;
        buf = ibuf;
        do {
            xfer.buf = buf;
            xfer.frames = numframes > period_size ? period_size : numframes;
            xfer.result = 0;
            if(!(ret = ioctl(afd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &xfer))) {
                avail = mmap_status->hw_ptr + ALSA_BUFFER_SIZE - mmap_control->appl_ptr;
                if(avail < 0) avail += boundary; else
                if((unsigned int)avail >= boundary) avail -= boundary;
                numframes -= xfer.result;
                buf += xfer.result;
            } else if(ret < 0) break;
        } while(period_size && numframes > 0);
    }
    return NULL;
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
 * Display screen with fixed scaler ("windowed" mode, pixel perfect, but does not fill the entire screen)
 */
void main_fix_scaler(void)
{
    uint32_t *src, *dst;
    int i, j, k, l, m, p;

    src = scrbuf;
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
}

/**
 * Display screen with arbitrary scaler (fullscreen mode, might be blurry, linear interpolation)
 */
void main_arb_scaler(void)
{
    uint8_t b[4];
    uint32_t *src, *dst, *c00, *c01, *c10, *c11, *csp, *dp, *ep;
    int o = 0, x, y, sx, sy, *csax, *csay, csx, csy, ex, ey, t1, t2, sstep, lx, ly, l;
    int sw = meg4.screen.w, sh = meg4.screen.h, sp = 640, w = win_fw, h = win_fh, p = fb_fix.line_length;

    src = scrbuf;
    dst = (uint32_t*)ffull;
    sx = (int) (65536.0 * (float)sw / (float)w); sy = (int) (65536.0 * (float)sh / (float)h);
    csp = src; ep = src + sp * sh - 1; dp = (uint32_t*)((uint8_t*)dst + o);
    csx = 0; csax = sax; for(x = 0; x <= w; x++) { *csax = csx; csax++; csx &= 0xffff; csx += sx; }
    csy = 0; csay = say; for(y = 0; y <= h; y++) { *csay = csy; csay++; csy &= 0xffff; csy += sy; }
    csay = say; ly = 0;
    for(y = l = 0; y < h; y++) {
        c00 = csp; c01 = csp; c01++; c10 = csp; c10 += sp; c11 = c10; c11++; csax = sax; lx = 0;
        for(x = 0; x < w - 1; x++) {
            if(c00 > ep) { c00 = ep; } if(c01 > ep) { c01 = ep; } if(c10 > ep) { c10 = ep; } if(c11 > ep) { c11 = ep; }
            ex = (*csax & 0xffff); ey = (*csay & 0xffff);
            t1 = (((((*c01 >> 24) - (*c00 >> 24)) * ex) >> 16) + (*c00 >> 24)) & 0xff;
            t2 = (((((*c11 >> 24) - (*c10 >> 24)) * ex) >> 16) + (*c10 >> 24)) & 0xff;
            b[3] = ((((t2 - t1) * ey) >> 16) + t1); if(b[3] == 254) b[3] = 255;
            t1 = (((((*c01 & 0xff) - (*c00 & 0xff)) * ex) >> 16) + (*c00 & 0xff)) & 0xff;
            t2 = (((((*c11 & 0xff) - (*c10 & 0xff)) * ex) >> 16) + (*c10 & 0xff)) & 0xff;
            b[0] = ((((t2 - t1) * ey) >> 16) + t1);
            t1 = ((((((*c01 >> 8) & 0xff) - ((*c00 >> 8) & 0xff)) * ex) >> 16) + ((*c00 >> 8) & 0xff)) & 0xff;
            t2 = ((((((*c11 >> 8) & 0xff) - ((*c10 >> 8) & 0xff)) * ex) >> 16) + ((*c10 >> 8) & 0xff)) & 0xff;
            b[1] = ((((t2 - t1) * ey) >> 16) + t1);
            t1 = ((((((*c01 >> 16) & 0xff) - ((*c00 >> 16) & 0xff)) * ex) >> 16) + ((*c00 >> 16) & 0xff)) & 0xff;
            t2 = ((((((*c11 >> 16) & 0xff) - ((*c10 >> 16) & 0xff)) * ex) >> 16) + ((*c10 >> 16) & 0xff)) & 0xff;
            b[2] = ((((t2 - t1) * ey) >> 16) + t1);
            *dp = !fb_var.red.offset ? /* RGBA */ *((uint32_t*)b) : /* BGRA */ (uint32_t)((b[0] << 16) | (b[1] << 8) | b[2]);
            dp++; csax++; if(*csax > 0) { sstep = (*csax >> 16); lx += sstep; if(lx < sw) { c00 += sstep; c01 += sstep; c10 += sstep; c11 += sstep; } }
        }
        csay++; if(*csay > 0) { sstep = (*csay >> 16); ly += sstep; if(ly < sh) csp += (sstep * sp); }
        o += p; dp = (uint32_t*)((uint8_t*)dst + o);
    }
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
    struct snd_pcm_hw_params params;
    struct snd_pcm_sw_params spar;
    struct snd_ctl_elem_list elist;
    struct snd_ctl_elem_info ei;
    struct snd_ctl_elem_value av;
    struct input_event ev[64];
    struct timespec ts;
    int i, j, k, n, mx = 160, my = 100, emptyalt = 0;
    char **infile = NULL, *fn;
    char s[5];
    uint32_t rrate = 44100;
    uint8_t *ptr;
    int32_t tickdiff;
    uint64_t ticks;
#ifdef USE_INIT
    char *lng = LANG, *fstype[] = { "vfat", "fat", "ext4", "ext3", "ext2", "auto" };
    (void)argc; (void)argv;
    /* we must do some housekeeping when we run as the init process. Assume read-only root fs */
    mount("none", "/dev", "devtmpfs", 0, NULL);
    mount("none", "/proc", "procfs", 0, NULL);
    mount("none", "/sys", "sysfs", 0, NULL);
    mount("none", "/tmp", "tmpfs", 0, NULL);
    for(i = 0; i < (int)(sizeof(fstype)/sizeof(fstype[0])) &&
      mount(FLOPPYDEV, "/mnt", fstype[i], MS_SYNCHRONOUS | MS_NODEV | MS_NOEXEC, NULL); i++);
    main_floppydir = "/mnt/MEG-4"; mkdir(main_floppydir, 0777);
    verbose = 1;
#else
    char *lng = getenv("LANG");
    main_parsecommandline(argc, argv, &lng, &infile);
    main_hdr();
    for(i = 0; i < 3; i++) printf("  %s\r\n", copyright[i]);
    printf("\r\n");
    fflush(stdout);
#endif
    strcpy(meg4plat, "fbdev_alsa");

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
    if((afd = open("/dev/snd/pcmC0D0p", O_RDWR)) > 0) {
        /* set up PCM device */
        param_init(&params);
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS, SNDRV_PCM_ACCESS_RW_INTERLEAVED);
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT, SNDRV_PCM_FORMAT_S16_LE);
        param_set_min(&params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, ALSA_BUFFER_SIZE);
        param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, ALSA_PERIOD_SIZE);
        param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, ALSA_BUFFER_SIZE / ALSA_PERIOD_SIZE);
        param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, 44100);
        param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS, 2);
        if(ioctl(afd, SNDRV_PCM_IOCTL_HW_PARAMS, &params)) { close(afd); afd = -1; goto noaudio; }
        period_size = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
        memset(&spar, 0, sizeof(spar));
        spar.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
        spar.period_step = 1;
        spar.avail_min = period_size;
        spar.start_threshold = ALSA_BUFFER_SIZE - period_size;
        spar.stop_threshold = ALSA_BUFFER_SIZE;
        spar.xfer_align = period_size / 2; /* for old kernels */
        if(ioctl(afd, SNDRV_PCM_IOCTL_SW_PARAMS, &spar)) { close(afd); afd = -1; goto noaudio; }
        boundary = spar.boundary;
        mmap_status = mmap(NULL, 4096, PROT_READ, MAP_SHARED, afd, SNDRV_PCM_MMAP_OFFSET_STATUS);
        if(!mmap_status || mmap_status == MAP_FAILED) { mmap_status = NULL; close(afd); afd = -1; goto noaudio; }
        mmap_control = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, afd, SNDRV_PCM_MMAP_OFFSET_CONTROL);
        if(!mmap_control || mmap_control == MAP_FAILED) {
            munmap(mmap_status, 4096); mmap_status = NULL; mmap_control = NULL; close(afd); afd = -1; goto noaudio;
        }
        audio = 1;
        /* try to unmute and set hardware volume to max */
        if((k = open("/dev/snd/controlC0", O_RDWR)) > 0) {
            memset(&elist, 0, sizeof(elist));
            if(!ioctl(k, SNDRV_CTL_IOCTL_ELEM_LIST, &elist)) {
                elist.pids = malloc(elist.count * sizeof(struct snd_ctl_elem_id));
                if(elist.pids) {
                    memset(elist.pids, 0, elist.count * sizeof(struct snd_ctl_elem_id));
                    elist.space = elist.count;
                    if(!ioctl(k, SNDRV_CTL_IOCTL_ELEM_LIST, &elist))
                        for(i = 0; i < (int)elist.count; i++) {
                            memset(&ei, 0, sizeof(ei));
                            ei.id.numid = elist.pids[i].numid;
                            if(!ioctl(k, SNDRV_CTL_IOCTL_ELEM_INFO, &ei) &&
                              (ei.access & SNDRV_CTL_ELEM_ACCESS_READWRITE) == SNDRV_CTL_ELEM_ACCESS_READWRITE &&
                              (ei.type == SNDRV_CTL_ELEM_TYPE_INTEGER || ei.type == SNDRV_CTL_ELEM_TYPE_BOOLEAN) &&
                              strstr((char*)ei.id.name, SNDRV_CTL_NAME_PLAYBACK)) {
                                memset(&av, 0, sizeof(av));
                                av.id.numid = ei.id.numid;
                                if(!ioctl(k, SNDRV_CTL_IOCTL_ELEM_READ, &av)) {
                                    for(j = 0; j < (int)ei.count; j++) av.value.integer.value[j] = ei.value.integer.max;
                                    ioctl(k, SNDRV_CTL_IOCTL_ELEM_WRITE, &av);
                                }
                            }
                        }
                    free(elist.pids);
                }
            }
            close(k);
        }
    }
noaudio:
    if(verbose && audio) main_log(1, "audio opened %uHz, %u bits", rrate, 16);

    scrbuf = (uint32_t*)malloc(640 * 400 * sizeof(uint32_t));
    if(!scrbuf) {
        main_log(0, "unable to allocate screen buffer");
        return 1;
    }
    main_win();
    if(fbuf == NULL) {
        main_log(0, "unable to initialize framebuffer");
        main_quit();
        return 1;
    }
    win_f = (win_w % 320) || (win_h % 200);

    /* turn on the emulator */
    meg4_poweron(lng);
#ifndef NOEDITORS
    for(; infile && *infile; infile++) {
        if((ptr = main_readfile(*infile, &i))) {
            fn = strrchr(*infile, SEP[0]); if(!fn) fn = *infile; else fn++;
            meg4_insert(fn, ptr, i);
            free(ptr);
        }
    }
#else
    (void)ptr; (void)infile;
#endif
    if(audio) {
        pthread_create(&th, NULL, main_audio, NULL);
        if(ioctl(afd, SNDRV_PCM_IOCTL_PREPARE) < 0) {
            munmap(mmap_status, 4096); munmap(mmap_control, 4096); mmap_status = NULL; mmap_control = NULL;
            close(afd); afd = -1; audio = 0; period_size = 0;
        }
    }
    meg4_setptr(mx, my);
    while(!main_exit) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ticks = (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
        /* handle user input events */
        for(n = 0; n < en; n += 2)
            if((k = read(ed[n], ev, sizeof(ev))) >= (int)sizeof(ev[0])) {
                for(i = 0, k /= sizeof(ev[0]); i < k; i++)
                    switch(ed[n + 1]) {
                        case MOUSE:
                            /* mouse events */
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
                        break;
                        case KBD:
                            /* keyboard events */
                            if(ev[i].type == EV_KEY && ev[i].value >= 0 && ev[i].value <= 2) {
                                if(ev[i].code > 0 && ev[i].code < (sizeof(main_keymap)/sizeof(main_keymap[0]))) {
                                    if(ev[i].value == 0) {
                                        /* key release */
                                        if(ev[i].code == KEY_LEFTCTRL || ev[i].code == KEY_LEFTALT) main_alt = 0;
                                        if(ev[i].code == KEY_LEFTMETA || ev[i].code == KEY_RIGHTMETA || ev[i].code == KEY_RIGHTALT) main_meta = 0;
                                        if(ev[i].code == KEY_LEFTSHIFT || ev[i].code == KEY_RIGHTSHIFT) main_sh = 0;
                                        if(ev[i].code == KEY_CAPSLOCK) main_caps = 0;
                                        if(ev[i].code == KEY_LEFTALT && main_sh && emptyalt) main_kbd ^= 1;
                                        emptyalt = 0;
                                        meg4_clrkey(main_keymap[ev[i].code]);
                                    } else
                                    if(ev[i].value == 1) {
                                        /* key press */
                                        if(ev[i].code == KEY_LEFTCTRL || ev[i].code == KEY_LEFTALT) main_alt = 1;
                                        if(ev[i].code == KEY_LEFTMETA || ev[i].code == KEY_RIGHTMETA || ev[i].code == KEY_RIGHTALT) main_meta = 1;
                                        if(ev[i].code == KEY_LEFTSHIFT || ev[i].code == KEY_RIGHTSHIFT) main_sh = 1;
                                        if(ev[i].code == KEY_CAPSLOCK) main_caps = 1;
                                        emptyalt = ev[i].code == KEY_LEFTALT;
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
                                                if(ev[i].code < sizeof(main_kbdlayout[0])/(sizeof(main_kbdlayout[0][0])*4)) {
                                                    memset(s, 0, sizeof(s));
                                                    strncpy(s, main_kbdlayout[main_kbd][(ev[i].code << 2) | (main_meta << 1) | (main_caps ^ main_sh)], 4);
                                                    if(s[0]) meg4_pushkey(s);
                                                    if(main_alt && s[0] == 'q' && !s[1]) main_exit = 1;
                                                }
                                            break;
                                        }
                                        meg4_setkey(main_keymap[ev[i].code]);
                                    }
                                }
                            }
                        break;
                        default:
                            /* controller events */
                            j = ed[n + 1] - PAD;
                            if(j < 4 && ev[i].type == EV_KEY) {
                                if(ev[i].value == 0) {
                                    switch(ev[i].code) {
                                        case BTN_DPAD_DOWN: meg4_clrpad(j, MEG4_BTN_D); break;
                                        case BTN_DPAD_UP:   meg4_clrpad(j, MEG4_BTN_U); break;
                                        case BTN_DPAD_LEFT: meg4_clrpad(j, MEG4_BTN_L); break;
                                        case BTN_DPAD_RIGHT:meg4_clrpad(j, MEG4_BTN_R); break;
                                        case BTN_A:         meg4_clrpad(j, MEG4_BTN_A); break;
                                        case BTN_B:         meg4_clrpad(j, MEG4_BTN_B); break;
                                        case BTN_X:         meg4_clrpad(j, MEG4_BTN_X); break;
                                        case BTN_Y:         meg4_clrpad(j, MEG4_BTN_Y); break;
                                    }
                                } else
                                if(ev[i].value == 1) {
                                    switch(ev[i].code) {
                                        case BTN_DPAD_DOWN: meg4_setpad(j, MEG4_BTN_D); break;
                                        case BTN_DPAD_UP:   meg4_setpad(j, MEG4_BTN_U); break;
                                        case BTN_DPAD_LEFT: meg4_setpad(j, MEG4_BTN_L); break;
                                        case BTN_DPAD_RIGHT:meg4_setpad(j, MEG4_BTN_R); break;
                                        case BTN_A:         meg4_setpad(j, MEG4_BTN_A); break;
                                        case BTN_B:         meg4_setpad(j, MEG4_BTN_B); break;
                                        case BTN_X:         meg4_setpad(j, MEG4_BTN_X); break;
                                        case BTN_Y:         meg4_setpad(j, MEG4_BTN_Y); break;
                                    }
                                }
                            }
                        break;
                    }
            }
        /* run the emulator and display screen */
        meg4_run();
        meg4_redraw(scrbuf, 640, 400, 640 * 4);
        if(win_f) main_arb_scaler();
        else main_fix_scaler();
        /* delay to run loop at 60 FPS */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        tickdiff = ((1000000000/60) - (((uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec) - ticks)) / 1000000;
        if(tickdiff > 0 && tickdiff < 1000) main_delay(tickdiff);
    }
    main_quit();
    return 0;
}
