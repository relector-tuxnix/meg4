/*
 * meg4/platform/libretro/main.c
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
 * @brief RetroArch "platform" for the MEG-4
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <time.h>
#include "meg4.h"

#include "libretro.h"
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb = 0;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
uint32_t scrbuf32[640 * 400];
uint16_t scrbuf16[640 * 400];
uint8_t *game_buf = NULL;
char game_fn[256] = { 0 };
int game_len = 0;
int use_audio_cb = 0;
int pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;

int main_w = 0, main_h = 0, win_w, win_h, win_f = 0, audio = 0, main_alt = 0, main_keymap[512];
int mx = 160, my = 100, ml = 0, mm = 0, mr = 0;
void main_delay(int msec);
char floppydir_tmp[4096];

#define meg4_showcursor()
#define meg4_hidecursor()
#include "../common.h"

/**
 * Exit emulator
 */
void main_quit(void)
{
    main_log(1, "quitting...         ");
    meg4_poweroff();
    meg4_showcursor();
    if(game_buf) { free(game_buf); game_buf = NULL; }
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    /* not supported by libretro */
}

/**
 * Make window focused
 */
void main_focus(void)
{
    /* not supported by libretro */
}

/**
 * Get text from clipboard (must be freed by caller)
 */
char *main_getclipboard(void)
{
    /* not supported by libretro */
    return NULL;
}

/**
 * Set text to clipboard
 */
void main_setclipboard(char *str)
{
    /* not supported by libretro */
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
 * Delay
 */
void main_delay(int msec)
{
    struct timespec tv;
    tv.tv_sec = 0; tv.tv_nsec = msec * 1000000;
    nanosleep(&tv, NULL);
}

/**
 * Print program version and copyright
 */
void main_hdr(void)
{
    printf("\r\nMEG-4 v%s (libretro, build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, BUILD);
}

/**
 * libretro callback; keyboard
 */
static void keyboard_cb(bool down, unsigned keycode, uint32_t unicode, uint16_t mod)
{
    char s[5];

    (void)keycode; (void)mod;
    /* for a lot of keys, this simply doesn't work... for example this function is called for 'c', but not for 'x' nor for 'a'. Happy typing! */
    /*printf("Down: %s, Code: %d, Char: %u, Mod: %u.\n", down ? "yes" : "no", keycode, unicode, mod);*/
    if(!down) return;
    /* yeah, normal keypress doesn't work for Esc... */
    if(keycode == RETROK_ESCAPE) {
        if(main_alt) { retro_deinit(); environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL); } else meg4_pushkey("\x1b\0\0");
    } else
    if(!main_alt && unicode >= ' ') {
        memset(s, 0, sizeof(s));
        if(unicode<0x80) { s[0]=unicode; } else
        if(unicode<0x800) { s[0]=((unicode>>6)&0x1F)|0xC0; s[1]=(unicode&0x3F)|0x80; } else
        if(unicode<0x10000) { s[0]=((unicode>>12)&0x0F)|0xE0; s[1]=((unicode>>6)&0x3F)|0x80; s[2]=(unicode&0x3F)|0x80; }
        else { s[0]=((unicode>>18)&0x07)|0xF0; s[1]=((unicode>>12)&0x3F)|0x80; s[2]=((unicode>>6)&0x3F)|0x80; s[3]=(unicode&0x3F)|0x80; }
        meg4_pushkey(s);
    }
}

/**
 * libretro callback; audio
 */
static void audio_callback(void)
{
#define AUDIO_BUFSIZ 735    /* 44100 / 60 */
    float fbuf[AUDIO_BUFSIZ];
    int16_t ibuf[AUDIO_BUFSIZ * 2];
    int i, j;

    meg4_audiofeed(fbuf, AUDIO_BUFSIZ);
    if(!audio) return;
    for(i = j = 0; i < AUDIO_BUFSIZ; i++, j += 2)
        ibuf[j] = ibuf[j + 1] = fbuf[i] * 32767.0f;
    audio_batch_cb(ibuf, AUDIO_BUFSIZ);
}

/**
 * libretro callback; audio
 */
static void audio_set_state(bool enable)
{
   (void)enable;
}

/**
 * libretro callback; Global initialization.
 */
RETRO_API void retro_init(void)
{
    char *lng;
    unsigned int l = 0;

    memset(main_keymap, 0, sizeof(main_keymap));
    main_keymap[RETROK_SPACE] = MEG4_KEY_SPACE;
    main_keymap[RETROK_CARET] = MEG4_KEY_APOSTROPHE;
    main_keymap[RETROK_COMMA] = MEG4_KEY_COMMA;
    main_keymap[RETROK_MINUS] = MEG4_KEY_MINUS;
    main_keymap[RETROK_PERIOD] = MEG4_KEY_PERIOD;
    main_keymap[RETROK_SLASH] = MEG4_KEY_SLASH;
    main_keymap[RETROK_0] = MEG4_KEY_0;
    main_keymap[RETROK_1] = MEG4_KEY_1;
    main_keymap[RETROK_2] = MEG4_KEY_2;
    main_keymap[RETROK_3] = MEG4_KEY_3;
    main_keymap[RETROK_4] = MEG4_KEY_4;
    main_keymap[RETROK_5] = MEG4_KEY_5;
    main_keymap[RETROK_6] = MEG4_KEY_6;
    main_keymap[RETROK_7] = MEG4_KEY_7;
    main_keymap[RETROK_8] = MEG4_KEY_8;
    main_keymap[RETROK_9] = MEG4_KEY_9;
    main_keymap[RETROK_SEMICOLON] = MEG4_KEY_SEMICOLON;
    main_keymap[RETROK_EQUALS] = MEG4_KEY_EQUAL;
    main_keymap[RETROK_a] = MEG4_KEY_A;
    main_keymap[RETROK_b] = MEG4_KEY_B;
    main_keymap[RETROK_c] = MEG4_KEY_C;
    main_keymap[RETROK_d] = MEG4_KEY_D;
    main_keymap[RETROK_e] = MEG4_KEY_E;
    main_keymap[RETROK_f] = MEG4_KEY_F;
    main_keymap[RETROK_g] = MEG4_KEY_G;
    main_keymap[RETROK_h] = MEG4_KEY_H;
    main_keymap[RETROK_i] = MEG4_KEY_I;
    main_keymap[RETROK_j] = MEG4_KEY_J;
    main_keymap[RETROK_k] = MEG4_KEY_K;
    main_keymap[RETROK_l] = MEG4_KEY_L;
    main_keymap[RETROK_m] = MEG4_KEY_M;
    main_keymap[RETROK_n] = MEG4_KEY_N;
    main_keymap[RETROK_o] = MEG4_KEY_O;
    main_keymap[RETROK_p] = MEG4_KEY_P;
    main_keymap[RETROK_q] = MEG4_KEY_Q;
    main_keymap[RETROK_r] = MEG4_KEY_R;
    main_keymap[RETROK_s] = MEG4_KEY_S;
    main_keymap[RETROK_t] = MEG4_KEY_T;
    main_keymap[RETROK_u] = MEG4_KEY_U;
    main_keymap[RETROK_v] = MEG4_KEY_V;
    main_keymap[RETROK_x] = MEG4_KEY_X;
    main_keymap[RETROK_y] = MEG4_KEY_Y;
    main_keymap[RETROK_z] = MEG4_KEY_Z;
    main_keymap[RETROK_LEFTBRACKET] = MEG4_KEY_LBRACKET;
    main_keymap[RETROK_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[RETROK_RIGHTBRACKET] = MEG4_KEY_RBRACKET;
    main_keymap[RETROK_RETURN] = MEG4_KEY_ENTER;
    main_keymap[RETROK_TAB] = MEG4_KEY_TAB;
    main_keymap[RETROK_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[RETROK_INSERT] = MEG4_KEY_INS;
    main_keymap[RETROK_DELETE] = MEG4_KEY_DEL;
    main_keymap[RETROK_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[RETROK_LEFT] = MEG4_KEY_LEFT;
    main_keymap[RETROK_DOWN] = MEG4_KEY_DOWN;
    main_keymap[RETROK_UP] = MEG4_KEY_UP;
    main_keymap[RETROK_PAGEUP] = MEG4_KEY_PGUP;
    main_keymap[RETROK_PAGEDOWN] = MEG4_KEY_PGDN;
    main_keymap[RETROK_HOME] = MEG4_KEY_HOME;
    main_keymap[RETROK_END] = MEG4_KEY_END;
    main_keymap[RETROK_CAPSLOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[RETROK_SCROLLOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[RETROK_NUMLOCK] = MEG4_KEY_NUMLOCK;
    main_keymap[RETROK_PRINT] = MEG4_KEY_PRSCR;
    main_keymap[RETROK_PAUSE] = MEG4_KEY_PAUSE;
    main_keymap[RETROK_F1] = MEG4_KEY_F1;
    main_keymap[RETROK_F2] = MEG4_KEY_F2;
    main_keymap[RETROK_F3] = MEG4_KEY_F3;
    main_keymap[RETROK_F4] = MEG4_KEY_F4;
    main_keymap[RETROK_F5] = MEG4_KEY_F5;
    main_keymap[RETROK_F6] = MEG4_KEY_F6;
    main_keymap[RETROK_F7] = MEG4_KEY_F7;
    main_keymap[RETROK_F8] = MEG4_KEY_F8;
    main_keymap[RETROK_F9] = MEG4_KEY_F9;
    main_keymap[RETROK_F10] = MEG4_KEY_F10;
    main_keymap[RETROK_F11] = MEG4_KEY_F11;
    main_keymap[RETROK_F12] = MEG4_KEY_F12;
    main_keymap[RETROK_KP0] = MEG4_KEY_KP_0;
    main_keymap[RETROK_KP1] = MEG4_KEY_KP_1;
    main_keymap[RETROK_KP2] = MEG4_KEY_KP_2;
    main_keymap[RETROK_KP3] = MEG4_KEY_KP_3;
    main_keymap[RETROK_KP4] = MEG4_KEY_KP_4;
    main_keymap[RETROK_KP5] = MEG4_KEY_KP_5;
    main_keymap[RETROK_KP6] = MEG4_KEY_KP_6;
    main_keymap[RETROK_KP7] = MEG4_KEY_KP_7;
    main_keymap[RETROK_KP8] = MEG4_KEY_KP_8;
    main_keymap[RETROK_KP9] = MEG4_KEY_KP_9;
    main_keymap[RETROK_KP_PERIOD] = MEG4_KEY_KP_DEC;
    main_keymap[RETROK_KP_DIVIDE] = MEG4_KEY_KP_DIV;
    main_keymap[RETROK_KP_MULTIPLY] = MEG4_KEY_KP_MUL;
    main_keymap[RETROK_KP_MINUS] = MEG4_KEY_KP_SUB;
    main_keymap[RETROK_KP_PLUS] = MEG4_KEY_KP_ADD;
    main_keymap[RETROK_KP_ENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[RETROK_KP_EQUALS] = MEG4_KEY_KP_EQUAL;
    main_keymap[RETROK_LSHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[RETROK_LCTRL] = MEG4_KEY_LCTRL;
    main_keymap[RETROK_LALT] = MEG4_KEY_LALT;
    main_keymap[RETROK_LSUPER] = MEG4_KEY_LSUPER;
    main_keymap[RETROK_RSHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[RETROK_RCTRL] = MEG4_KEY_RCTRL;
    main_keymap[RETROK_RALT] = MEG4_KEY_RALT;
    main_keymap[RETROK_RSUPER] = MEG4_KEY_RSUPER;
    main_keymap[RETROK_MENU] = MEG4_KEY_MENU;

    if(environ_cb) environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &l);
    switch(l) {
        case RETRO_LANGUAGE_JAPANESE:   lng = "ja"; break;
        case RETRO_LANGUAGE_FRENCH:     lng = "fr"; break;
        case RETRO_LANGUAGE_SPANISH:    lng = "es"; break;
        case RETRO_LANGUAGE_GERMAN:     lng = "de"; break;
        case RETRO_LANGUAGE_ITALIAN:    lng = "it"; break;
        case RETRO_LANGUAGE_DUTCH:      lng = "nl"; break;
        case RETRO_LANGUAGE_PORTUGUESE_BRAZIL:
        case RETRO_LANGUAGE_PORTUGUESE_PORTUGAL: lng = "pt"; break;
        case RETRO_LANGUAGE_RUSSIAN:    lng = "ru"; break;
        case RETRO_LANGUAGE_KOREAN:     lng = "ko"; break;
        case RETRO_LANGUAGE_CHINESE_SIMPLIFIED:
        case RETRO_LANGUAGE_CHINESE_TRADITIONAL: lng = "zh"; break;
        case RETRO_LANGUAGE_POLISH:     lng = "pl"; break;
        case RETRO_LANGUAGE_VIETNAMESE: lng = "vi"; break;
        case RETRO_LANGUAGE_ARABIC:     lng = "ar"; break;
        case RETRO_LANGUAGE_GREEK:      lng = "el"; break;
        case RETRO_LANGUAGE_TURKISH:    lng = "tr"; break;
        case RETRO_LANGUAGE_SLOVAK:     lng = "sk"; break;
        case RETRO_LANGUAGE_FINNISH:    lng = "fi"; break;
        case RETRO_LANGUAGE_INDONESIAN: lng = "in"; break;
        case RETRO_LANGUAGE_SWEDISH:    lng = "sv"; break;
        case RETRO_LANGUAGE_UKRAINIAN:  lng = "uk"; break;
        case RETRO_LANGUAGE_CZECH:      lng = "cs"; break;
        case RETRO_LANGUAGE_CATALAN_VALENCIA:
        case RETRO_LANGUAGE_CATALAN:    lng = "ca"; break;
        case RETRO_LANGUAGE_HUNGARIAN:  lng = "hu"; break;
        default: lng = "en"; break;
    }
    /* turn on the emulator */
    meg4_poweron(lng);
}

/**
 * libretro callback; Global deinitialization.
 */
RETRO_API void retro_deinit(void)
{
    main_quit();
}

/**
 * libretro callback; Retrieves the internal libretro API version.
 */
RETRO_API unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

/**
 * libretro callback; Reports device changes.
 */
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port; (void)device;
}

/**
 * libretro callback; Retrieves information about the core.
 */
RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(struct retro_system_info));
    info->library_name     = "MEG-4";
    info->library_version  = meg4ver;
    info->valid_extensions = "png|zip|tga|tmx|mid|mod|wav|bdf|sfn|sfd|psf|gpl|txt";
    info->need_fullpath    = true;
    info->block_extract    = false;
}

/**
 * libretro callback; Get information about the desired audio and video.
 */
RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
    pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
    if(!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format)) {
        /* default format is deprecated... no comment */
        pixel_format = RETRO_PIXEL_FORMAT_RGB565;
        environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
    }
    info->timing = (struct retro_system_timing) {
        .fps = 60,
        .sample_rate = 44100,
    };
    info->geometry = (struct retro_game_geometry) {
        .base_width   = 320,
        .base_height  = 200,
        .max_width    = 640,
        .max_height   = 400,
        .aspect_ratio = (float)640 / (float)400,
    };
}

/**
 * libretro callback; Sets up the environment callback.
 */
RETRO_API void retro_set_environment(retro_environment_t cb)
{
    char *tmp;
    bool no_content = true;

    environ_cb = cb;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);
    main_floppydir = NULL;
    if(!cb(RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY, &main_floppydir) || !main_floppydir || !*main_floppydir)
        cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &main_floppydir);
#ifndef __WIN32__
    if(!main_floppydir) {
        tmp = getenv("HOME");
        if(tmp) {
            main_floppydir = floppydir_tmp;
            strncpy(main_floppydir, tmp, sizeof(floppydir_tmp) - 1);
            strncat(main_floppydir, "/MEG-4", sizeof(floppydir_tmp) - 1);
            mkdir(main_floppydir, 0777);
        }
    }
#endif
}

/**
 * libretro callback; Set up the audio sample callback.
 */
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

/**
 * libretro callback; Set up the audio sample batch callback.
 *
 * @see tic80_libretro_audio()
 */
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

/**
 * libretro callback; Set up the input poll callback.
 */
RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

/**
 * libretro callback; Set up the input state callback.
 */
RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

/**
 * libretro callback; Set up the video refresh callback.
 */
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

/**
 * libretro callback; Reset the game.
 */
RETRO_API void retro_reset(void)
{
    meg4_reset();
    /* if we have already loaded a game, reload it */
    if(game_fn[0] && game_buf && game_len > 0)
        meg4_insert(game_fn, game_buf, game_len);
}

/**
 * libretro callback; Render the screen and play the audio.
 */
RETRO_API void retro_run(void)
{
    int i, j, k, offs;

    /* handle events... by polling */
    input_poll_cb();
    /* mouse */
    mx += input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    my += input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    if(mx < 0) mx = 0;
    if(mx > meg4.screen.w) mx = meg4.screen.w;
    if(my < 0) my = 0;
    if(my > meg4.screen.h) my = meg4.screen.h;
    meg4_setptr(mx, my);
    /* we have only press events, but no release events... */
    i = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
    if(i && !ml) { ml = 1; meg4_setbtn(MEG4_BTN_L); } else
    if(!i && ml) { ml = 0; meg4_clrbtn(MEG4_BTN_L); }
    i = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
    if(i && !mm) { mm = 1; meg4_setbtn(MEG4_BTN_M); } else
    if(!i && mm) { mm = 0; meg4_clrbtn(MEG4_BTN_M); }
    i = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
    if(i && !mr) { mr = 1; meg4_setbtn(MEG4_BTN_R); } else
    if(!i && mr) { mr = 0; meg4_clrbtn(MEG4_BTN_R); }
    if(input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP)) meg4_setscr(1, 0, 0, 0); else
    if(input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN)) meg4_setscr(0, 1, 0, 0);
    if(input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP)) meg4_setscr(0, 0, 0, 1); else
    if(input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN)) meg4_setscr(0, 0, 1, 0);
    /* keyboard */
    for(i = RETROK_FIRST; i < RETROK_LAST; i++)
        if(main_keymap[i]) {
            j = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, i);
            k = meg4_api_getkey(main_keymap[i]);
            if(j && !k) {
                /*printf("keypress i %d\n",i);*/
                switch(i) {
                    case RETROK_LALT: case RETROK_LCTRL: main_alt = 1; break;
                    case RETROK_RETURN:
                        if(main_alt) main_fullscreen();
                        else meg4_pushkey("\n\0\0");
                    break;
                    case RETROK_q: if(main_alt) { retro_deinit(); environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL); } break;
/* doesn't work. Have to check in keyboard_cb */
/*                    case RETROK_ESCAPE: if(main_alt) { retro_deinit(); environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL); } else meg4_pushkey("\x1b\0\0"); break;*/
                    /* only for special keys that aren't handled by keyboard_cb */
                    case RETROK_F1: meg4_pushkey("F1\0"); break;
                    case RETROK_F2: meg4_pushkey("F2\0"); break;
                    case RETROK_F3: meg4_pushkey("F3\0"); break;
                    case RETROK_F4: meg4_pushkey("F4\0"); break;
                    case RETROK_F5: meg4_pushkey("F5\0"); break;
                    case RETROK_F6: meg4_pushkey("F6\0"); break;
                    case RETROK_F7: meg4_pushkey("F7\0"); break;
                    case RETROK_F8: meg4_pushkey("F8\0"); break;
                    case RETROK_F9: meg4_pushkey("F9\0"); break;
                    case RETROK_F10: meg4_pushkey("F10"); break;
                    case RETROK_F11: meg4_pushkey("F11"); break;
                    case RETROK_F12: meg4_pushkey("F12"); break;
                    case RETROK_PRINT: meg4_pushkey("PSc"); break;
                    case RETROK_SCROLLOCK: meg4_pushkey("SLk"); break;
                    case RETROK_NUMLOCK: meg4_pushkey("NLk"); break;
                    case RETROK_BACKSPACE: meg4_pushkey("\b\0\0"); break;
                    case RETROK_TAB: meg4_pushkey("\t\0\0"); break;
                    case RETROK_CAPSLOCK: meg4_pushkey("CLk"); break;
                    case RETROK_UP: meg4_pushkey("Up\0"); break;
                    case RETROK_DOWN: meg4_pushkey("Down"); break;
                    case RETROK_LEFT: meg4_pushkey("Left"); break;
                    case RETROK_RIGHT: meg4_pushkey("Rght"); break;
                    case RETROK_HOME: meg4_pushkey("Home"); break;
                    case RETROK_END: meg4_pushkey("End"); break;
                    case RETROK_PAGEUP: meg4_pushkey("PgUp"); break;
                    case RETROK_PAGEDOWN: meg4_pushkey("PgDn"); break;
                    case RETROK_INSERT: meg4_pushkey("Ins"); break;
                    case RETROK_DELETE: meg4_pushkey("Del"); break;
                    default: break;
                }
                meg4_setkey(main_keymap[i]);
            } else
            if(!j && k) {
                switch(i) {
                    case RETROK_LALT: case RETROK_LCTRL: main_alt = 0; break;
                    default: break;
                }
                meg4_clrkey(main_keymap[i]);
            }
        }
    /* gamepads */
    for(i = 0; i < 4; i++) {
        /* we have only press events, but no release events... */
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
        k = meg4_api_getpad(i, MEG4_BTN_U);
        if(j && !k) meg4_setpad(i, MEG4_BTN_U); else if(!j && k) meg4_clrpad(i, MEG4_BTN_U);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
        k = meg4_api_getpad(i, MEG4_BTN_D);
        if(j && !k) meg4_setpad(i, MEG4_BTN_D); else if(!j && k) meg4_clrpad(i, MEG4_BTN_D);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
        k = meg4_api_getpad(i, MEG4_BTN_L);
        if(j && !k) meg4_setpad(i, MEG4_BTN_L); else if(!j && k) meg4_clrpad(i, MEG4_BTN_L);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
        k = meg4_api_getpad(i, MEG4_BTN_R);
        if(j && !k) meg4_setpad(i, MEG4_BTN_R); else if(!j && k) meg4_clrpad(i, MEG4_BTN_R);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
        k = meg4_api_getpad(i, MEG4_BTN_A);
        if(j && !k) meg4_setpad(i, MEG4_BTN_A); else if(!j && k) meg4_clrpad(i, MEG4_BTN_A);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
        k = meg4_api_getpad(i, MEG4_BTN_B);
        if(j && !k) meg4_setpad(i, MEG4_BTN_B); else if(!j && k) meg4_clrpad(i, MEG4_BTN_B);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
        k = meg4_api_getpad(i, MEG4_BTN_X);
        if(j && !k) meg4_setpad(i, MEG4_BTN_X); else if(!j && k) meg4_clrpad(i, MEG4_BTN_X);
        j = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
        k = meg4_api_getpad(i, MEG4_BTN_Y);
        if(j && !k) meg4_setpad(i, MEG4_BTN_Y); else if(!j && k) meg4_clrpad(i, MEG4_BTN_Y);
    }

    meg4_run();
    meg4_redraw(scrbuf32, 640, 400, 640 * 4);

    /* the buffer is 640 x 400, but it is possible that only a 320 x 200 window is displayed out of it */
    offs = (le16toh(meg4.mmio.scry) > 200 ? 0 : le16toh(meg4.mmio.scry) * 640) +
        (le16toh(meg4.mmio.scrx) > 320 ? 0 : le16toh(meg4.mmio.scrx));
    /* Muhahaha, libretro sucks such a big time... on a real hw video channel order is independent to the CPU endianness
     * talking about wasting precious CPU time in a function that runs 60 times every sec... */
    if(pixel_format == RETRO_PIXEL_FORMAT_XRGB8888) {
#if MEG4_BYTEORDER == 1234
        for(i = 0; i < 640 * 400; i++)
            scrbuf32[i] = ((scrbuf32[i] >> 16) & 0xff) | ((scrbuf32[i] & 0xff) << 16) | (scrbuf32[i] & 0xff00);
#endif
        video_cb((void*)(scrbuf32 + offs), meg4.screen.w, meg4.screen.h, 640 * sizeof(uint32_t));
    } else {
        for(i = 0; i < 640 * 400; i++)
#if MEG4_BYTEORDER == 1234
            scrbuf16[i] = ((scrbuf32[i] & 0xf8) << 8) | ((scrbuf32[i] >> 19) & 0x1f) | ((scrbuf32[i] & 0xfc00) >> 5);
#else
            scrbuf16[i] = ((scrbuf32[i] >> 3) & 0x1f) | ((scrbuf32[i] & 0xf80000) >> 8) | ((scrbuf32[i] & 0xfc00) >> 5);
#endif
        video_cb((void*)(scrbuf16 + offs), meg4.screen.w, meg4.screen.h, 640 * sizeof(uint16_t));
    }
    /* play audio in the main thread... could this be any more inefficient? */
    if(!use_audio_cb) audio_callback();
}

/**
 * libretro callback; Load a game.
 */
RETRO_API bool retro_load_game(const struct retro_game_info *info)
{
    uint8_t *ptr;
    char *fn;
    int l;
    /* not sure why we have to do this in load game function, why not in retro_init, but all libretro examples do it here */
    struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
    struct retro_keyboard_callback cb = { keyboard_cb };
    struct retro_input_descriptor desc[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
        { 0 }
    };
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
    use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

    /* sometimes this is called with a NULL... and never return false from this function, retroarch will exit immediately! */
    if(!info || (!info->path && !info->data)) return true;

    /* save the floppy's data in game_X because we'll have to reload it on game reset */
    if(!info->path && info->data && info->size > 0) {
        strncpy(game_fn, "noname.png", sizeof(game_fn) - 1);
        meg4_insert(game_fn, (uint8_t *)info->data, info->size);
        game_buf = (uint8_t*)realloc(game_buf, info->size);
        if(game_buf) { memcpy(game_buf, info->data, info->size); game_len = info->size; }
        else { memset(game_fn, 0, sizeof(game_fn)); game_len = 0; }
    } else if(info->path && (ptr = main_readfile(!memcmp(info->path, "file://", 7) ? (char*)info->path + 7 : (char*)info->path, &l))) {
        fn = strrchr((char*)info->path, SEP[0]); if(!fn) fn = (char*)info->path; else fn++;
        meg4_insert(fn, ptr, l);
        game_buf = (uint8_t*)realloc(game_buf, l);
        if(game_buf) { strncpy(game_fn, fn, sizeof(game_fn) - 1); memcpy(game_buf, ptr, l); game_len = l; }
        else { memset(game_fn, 0, sizeof(game_fn)); game_len = 0; }
        free(ptr);
    }
    return true;
}

/**
 * libretro callback; Tells the core to unload the game.
 */
RETRO_API void retro_unload_game(void)
{
    meg4_reset();
    memset(game_fn, 0, sizeof(game_fn)); game_len = 0;
    if(game_buf) { free(game_buf); game_buf = NULL; }
}

/**
 * libretro callback; Retrieves the region for the content.
 */
RETRO_API unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC; /* 60 fps */
}

/**
 * libretro callback; Load a game using a subsystem.
 */
RETRO_API bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
    (void)num; (void)type;
    return retro_load_game(info);
}

/**
 * libretro callback; Retrieve the size of the serialized memory.
 */
size_t retro_serialize_size(void)
{
    return 0;
}

/**
 * libretro callback; Get the current persistent memory.
 */
RETRO_API bool retro_serialize(void *data, size_t size)
{
    (void)data; (void)size;
    return false;
}

/**
 * libretro callback; Given the serialized data, load it into the persistent memory.
 */
RETRO_API bool retro_unserialize(const void *data, size_t size)
{
    (void)data; (void)size;
    return false;
}

/**
 * libretro callback; Gets region of memory.
 */
RETRO_API void *retro_get_memory_data(unsigned id)
{
    (void)id;
    return NULL;
}

/**
 * libretro callback; Gets the size of the given memory slot.
 */
RETRO_API size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0;
}

/**
 * libretro callback; Reset all cheats to disabled.
 */
RETRO_API void retro_cheat_reset(void)
{
    meg4.mmio.kbdkeys[0] &= ~1;
}

/**
 * libretro callback; Enable/disable the given cheat code.
 */
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index; (void)code;
    if(enabled) meg4.mmio.kbdkeys[0] |= 1;
    else meg4.mmio.kbdkeys[0] &= ~1;
}
