/*
 * meg4/platform/sokol/main.c
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
 * @brief sokol "platform" for the MEG-4
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <time.h>
#include "meg4.h"

#define SOKOL_IMPL
#define SOKOL_AUDIO_IMPL
#if defined(__APPLE__)
#define SOKOL_METAL
#elif defined(__WIN32__)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES2
#else
#define SOKOL_GLCORE33
#endif
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_audio.h"
#include "sokol_gl.h"
#include "sokol_glue.h"

uint32_t *scrbuf = NULL;
sg_image img;
sgl_pipeline pip;
static const sg_pass_action pass_action = {
    .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f } }
};

int main_w = 0, main_h = 0, win_w, win_h, win_f = 0, audio = 0, main_alt = 0, main_keymap[512];
int mx = 160, my = 100;
void main_delay(int msec);
char main_clip[1048576];

#define meg4_showcursor()   sapp_show_mouse(1)
#define meg4_hidecursor()   sapp_show_mouse(0)
#include "../common.h"

/**
 * Logger
 */
void slog(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data)
{
    const char *log_level_str[] = { "panic", "error", "warning", "info", "unk4", "unk5", "unk6", "unk7" };
    (void)user_data;
    main_log(3, "sokol-%s:%s(%u):%s(%u): %s", log_level_str[log_level], tag, log_item, filename, line_nr, message);
}

/**
 * Exit emulator
 */
void main_quit(void)
{
    main_log(1, "quitting...         ");
    meg4_poweroff();
    if(audio) { saudio_shutdown(); audio = 0; }
    if(img.id != SG_INVALID_ID) { sg_destroy_image(img); img.id = SG_INVALID_ID; }
    if(scrbuf) { free(scrbuf); scrbuf = NULL; }
    meg4_showcursor();
    sgl_shutdown();
    sg_shutdown();
    exit(0);
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    sapp_toggle_fullscreen();
}

/**
 * Make window focused
 */
void main_focus(void)
{
    /* TODO: how to focus window with sokol? */
}

/**
 * Get text from clipboard (must be freed by caller)
 */
char *main_getclipboard(void)
{
    char *ret = NULL;
    /* we use a cache because sapp_get_clipboard_string() can only be called in pasted event handler */
    if(main_clip[0]) {
        ret = (char*)malloc(strlen(main_clip) + 1);
        if(ret) strcpy(ret, main_clip);
    }
    return ret;
}

/**
 * Set text to clipboard
 */
void main_setclipboard(char *str)
{
    sapp_set_clipboard_string(str);
}

/**
 * Show on-screen keyboard
 */
void main_osk_show(void)
{
    sapp_show_keyboard(1);
}

/**
 * Hide on-screen keyboard
 */
void main_osk_hide(void)
{
    sapp_show_keyboard(0);
}

/**
 * Audio callback
 */
void main_audio(float *buffer, int frames, int channels)
{
    (void)channels;
    meg4_audiofeed(buffer, frames);
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
    printf("\r\nMEG-4 v%s (sokol, build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, BUILD);
}

/**
 * Initialize Sokol
 */
static void init(void)
{
    memset(main_clip, 0, sizeof(main_clip));
    sg_setup(&(sg_desc){ .context = sapp_sgcontext(), .logger.func = slog });
    sgl_setup(&(sgl_desc_t){ .logger.func = slog });
    pip = sgl_make_pipeline(&(sg_pipeline_desc){ .colors[0] = { .write_mask = SG_COLORMASK_RGB } });
    img = sg_make_image(&(sg_image_desc){
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .width = 640,
        .height = 400,
        .usage = SG_USAGE_STREAM,
        .mag_filter = SG_FILTER_NEAREST,
        .min_filter = nearest ? SG_FILTER_NEAREST : SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    saudio_setup(&(saudio_desc){
        .sample_rate = 44100,
        .num_channels = 1,
        .stream_cb = main_audio,
        .logger.func = slog,
    });
    audio = saudio_isvalid();
    if(verbose && audio) main_log(1, "audio opened %uHz, %u bits", 44100, 32);
#ifndef DEBUG
    main_fullscreen();
#endif
    meg4_setptr(mx, my);
    meg4_hidecursor();
}

/**
 * Render one Sokol frame
 */
static void frame(void)
{
    int i, w, h, ww = sapp_width(), wh = sapp_height();
    float vert[16], x1, y1, x2, y2;

    meg4_run();
    meg4_redraw(scrbuf, 640, 400, 640 * 4);
    sg_update_image(img, &(sg_image_data){ .subimage[0][0] = { .ptr = scrbuf, .size = 640 * 400 * 4 } });
    if(nearest) {
        for(i = 1; i < 16 && (i + 1) * 320 <= ww && (i + 1) * 200 <= wh; i++);
        w = 320 * i; h = 200 * i;
    } else {
        w = ww; h = (int)meg4.screen.h * ww / (int)meg4.screen.w;
        if(h > wh) { h = wh; w = (int)meg4.screen.w * wh / (int)meg4.screen.h; }
    }
    w >>= 1; h >>= 1;
    i = le16toh(meg4.mmio.scrx) > 320 ? 0 : le16toh(meg4.mmio.scrx);
    x1 = (float)(i) / 640; x2 = (float)(i + meg4.screen.w) / 640;
    i = le16toh(meg4.mmio.scry) > 200 ? 0 : le16toh(meg4.mmio.scry);
    y1 = (float)(i) / 400; y2 = (float)(i + meg4.screen.h) / 400;
    vert[ 0] = -w; vert[ 1] = -h; vert[ 2] = x1; vert[ 3] = y1;
    vert[ 4] = -w; vert[ 5] =  h; vert[ 6] = x1; vert[ 7] = y2;
    vert[ 8] =  w; vert[ 9] =  h; vert[10] = x2; vert[11] = y2;
    vert[12] =  w; vert[13] = -h; vert[14] = x2; vert[15] = y1;

    sgl_defaults();
    sgl_enable_texture();
    sgl_matrix_mode_projection();
    sgl_ortho(-(float)ww*0.5f, (float)ww*0.5f, (float)wh*0.5f, -(float)wh*0.5f, 0.0f, +1.0f);

    sgl_texture(img);
    sgl_load_pipeline(pip);
    sgl_c3f(1.0f, 1.0f, 1.0f);
    sgl_begin_quads();
    sgl_v2f_t2f(vert[0], vert[1], vert[2], vert[3]);
    sgl_v2f_t2f(vert[4], vert[5], vert[6], vert[7]);
    sgl_v2f_t2f(vert[8], vert[9], vert[10], vert[11]);
    sgl_v2f_t2f(vert[12], vert[13], vert[14], vert[15]);
    sgl_end();

    sg_begin_default_passf(&pass_action, ww, wh);
    sgl_draw();
    sg_end_pass();
    sg_commit();
}

/**
 * Sokol event handler
 */
static void event(const sapp_event* ev)
{
    int i, n, l;
    char *str, *fn, s[5];
    uint8_t *ptr;

    switch(ev->type) {
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            mx += ev->mouse_dx; my += ev->mouse_dy;
            if(mx < 0) mx = 0;
            if(mx > meg4.screen.w) mx = meg4.screen.w;
            if(my < 0) my = 0;
            if(my > meg4.screen.h) my = meg4.screen.h;
            meg4_setptr(mx, my);
        break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            switch(ev->mouse_button) {
                case SAPP_MOUSEBUTTON_LEFT:   meg4_setbtn(MEG4_BTN_L); break;
                case SAPP_MOUSEBUTTON_MIDDLE: meg4_setbtn(MEG4_BTN_M); break;
                case SAPP_MOUSEBUTTON_RIGHT:  meg4_setbtn(MEG4_BTN_R); break;
                default: break;
            }
        break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            switch(ev->mouse_button) {
                case SAPP_MOUSEBUTTON_LEFT:   meg4_clrbtn(MEG4_BTN_L); break;
                case SAPP_MOUSEBUTTON_MIDDLE: meg4_clrbtn(MEG4_BTN_M); break;
                case SAPP_MOUSEBUTTON_RIGHT:  meg4_clrbtn(MEG4_BTN_R); break;
                default: break;
            }
        break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            meg4_setscr(ev->scroll_y > 0.0, ev->scroll_y < 0.0, ev->scroll_x > 0.0, ev->scroll_x < 0.0);
        break;
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch(ev->key_code) {
                case SAPP_KEYCODE_LEFT_ALT: case SAPP_KEYCODE_LEFT_CONTROL: main_alt = 1; break;
                case SAPP_KEYCODE_ENTER:
                    if(main_alt) {
                        main_fullscreen();
                        return;
                    }
                    meg4_pushkey("\n\0\0");
                break;
                case SAPP_KEYCODE_Q: if(main_alt) { main_quit(); } break;
                case SAPP_KEYCODE_ESCAPE: if(main_alt) { main_quit(); } else meg4_pushkey("\x1b\0\0"); break;
                /* only for special keys that aren't handled by SAPP_EVENTTYPE_CHAR events */
                case SAPP_KEYCODE_F1: meg4_pushkey("F1\0"); break;
                case SAPP_KEYCODE_F2: meg4_pushkey("F2\0"); break;
                case SAPP_KEYCODE_F3: meg4_pushkey("F3\0"); break;
                case SAPP_KEYCODE_F4: meg4_pushkey("F4\0"); break;
                case SAPP_KEYCODE_F5: meg4_pushkey("F5\0"); break;
                case SAPP_KEYCODE_F6: meg4_pushkey("F6\0"); break;
                case SAPP_KEYCODE_F7: meg4_pushkey("F7\0"); break;
                case SAPP_KEYCODE_F8: meg4_pushkey("F8\0"); break;
                case SAPP_KEYCODE_F9: meg4_pushkey("F9\0"); break;
                case SAPP_KEYCODE_F10: meg4_pushkey("F10"); break;
                case SAPP_KEYCODE_F11: meg4_pushkey("F11"); break;
                case SAPP_KEYCODE_F12: meg4_pushkey("F12"); break;
                case SAPP_KEYCODE_PRINT_SCREEN: meg4_pushkey("PSc"); break;
                case SAPP_KEYCODE_SCROLL_LOCK: meg4_pushkey("SLk"); break;
                case SAPP_KEYCODE_NUM_LOCK: meg4_pushkey("NLk"); break;
                case SAPP_KEYCODE_BACKSPACE: meg4_pushkey("\b\0\0"); break;
                case SAPP_KEYCODE_TAB: meg4_pushkey("\t\0\0"); break;
                case SAPP_KEYCODE_CAPS_LOCK: meg4_pushkey("CLk"); break;
                case SAPP_KEYCODE_UP: meg4_pushkey("Up\0"); break;
                case SAPP_KEYCODE_DOWN: meg4_pushkey("Down"); break;
                case SAPP_KEYCODE_LEFT: meg4_pushkey("Left"); break;
                case SAPP_KEYCODE_RIGHT: meg4_pushkey("Rght"); break;
                case SAPP_KEYCODE_HOME: meg4_pushkey("Home"); break;
                case SAPP_KEYCODE_END: meg4_pushkey("End"); break;
                case SAPP_KEYCODE_PAGE_UP: meg4_pushkey("PgUp"); break;
                case SAPP_KEYCODE_PAGE_DOWN: meg4_pushkey("PgDn"); break;
                case SAPP_KEYCODE_INSERT: meg4_pushkey("Ins"); break;
                case SAPP_KEYCODE_DELETE: meg4_pushkey("Del"); break;
                default: break;
            }
            if(ev->key_code > 0 && ev->key_code < 512 && main_keymap[ev->key_code]) meg4_setkey(main_keymap[ev->key_code]);
        break;
        case SAPP_EVENTTYPE_KEY_UP:
            switch(ev->key_code) {
                case SAPP_KEYCODE_LEFT_ALT: case SAPP_KEYCODE_LEFT_CONTROL: main_alt = 0; break;
                default: break;
            }
            if(ev->key_code > 0 && ev->key_code < 512 && main_keymap[ev->key_code]) meg4_clrkey(main_keymap[ev->key_code]);
        break;
        case SAPP_EVENTTYPE_CHAR:
            if(!main_alt && ev->char_code >= 32) {
                memset(s, 0, sizeof(s));
                if(ev->char_code<0x80) { s[0]=ev->char_code; } else
                if(ev->char_code<0x800) { s[0]=((ev->char_code>>6)&0x1F)|0xC0; s[1]=(ev->char_code&0x3F)|0x80; } else
                if(ev->char_code<0x10000) { s[0]=((ev->char_code>>12)&0x0F)|0xE0; s[1]=((ev->char_code>>6)&0x3F)|0x80; s[2]=(ev->char_code&0x3F)|0x80; }
                else { s[0]=((ev->char_code>>18)&0x07)|0xF0; s[1]=((ev->char_code>>12)&0x3F)|0x80; s[2]=((ev->char_code>>6)&0x3F)|0x80; s[3]=(ev->char_code&0x3F)|0x80; }
                meg4_pushkey(s);
            }
        break;
        case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
            /* according to the doc, sapp_get_clipboard_string() can only be called in pasted event handler. So we save the pasted
             * string in a cache, and when MEG-4 receives a paste keypress, it will call main_getclipboard() that reads this cache */
            memset(main_clip, 0, sizeof(main_clip));
            str = (char*)sapp_get_clipboard_string();
            if(str && *str) { strncpy(main_clip, str, sizeof(main_clip) - 1); }
        break;
        case SAPP_EVENTTYPE_FILES_DROPPED:
            for(i = 0, n = sapp_get_num_dropped_files(); i < n; i++) {
                str = (char*)sapp_get_dropped_file_path(i);
                if((ptr = main_readfile(!memcmp(str, "file://", 7) ? str + 7 : str, &l))) {
                    fn = strrchr(str, SEP[0]); if(!fn) fn = str; else fn++;
                    meg4_insert(fn, ptr, l);
                    free(ptr);
                }
            }
        break;
        default: break;
    }
}

/**
 * The real main procedure
 */
sapp_desc sokol_main(int argc, char* argv[])
{
    int i;
    char *infile = NULL, *fn;
    uint8_t *ptr;
#ifdef __WIN32__
    char *lng = main_lng;
#else
    char *lng = getenv("LANG");
#endif
    main_parsecommandline(argc, argv, &lng, &infile);
    main_hdr();
    for(i = 0; i < 3; i++) printf("  %s\r\n", copyright[i]);
    printf("\r\n");
    fflush(stdout);
    /* set up keymap */
    memset(main_keymap, 0, sizeof(main_keymap));
    main_keymap[SAPP_KEYCODE_SPACE] = MEG4_KEY_SPACE;
    main_keymap[SAPP_KEYCODE_APOSTROPHE] = MEG4_KEY_APOSTROPHE;
    main_keymap[SAPP_KEYCODE_COMMA] = MEG4_KEY_COMMA;
    main_keymap[SAPP_KEYCODE_MINUS] = MEG4_KEY_MINUS;
    main_keymap[SAPP_KEYCODE_PERIOD] = MEG4_KEY_PERIOD;
    main_keymap[SAPP_KEYCODE_SLASH] = MEG4_KEY_SLASH;
    main_keymap[SAPP_KEYCODE_0] = MEG4_KEY_0;
    main_keymap[SAPP_KEYCODE_1] = MEG4_KEY_1;
    main_keymap[SAPP_KEYCODE_2] = MEG4_KEY_2;
    main_keymap[SAPP_KEYCODE_3] = MEG4_KEY_3;
    main_keymap[SAPP_KEYCODE_4] = MEG4_KEY_4;
    main_keymap[SAPP_KEYCODE_5] = MEG4_KEY_5;
    main_keymap[SAPP_KEYCODE_6] = MEG4_KEY_6;
    main_keymap[SAPP_KEYCODE_7] = MEG4_KEY_7;
    main_keymap[SAPP_KEYCODE_8] = MEG4_KEY_8;
    main_keymap[SAPP_KEYCODE_9] = MEG4_KEY_9;
    main_keymap[SAPP_KEYCODE_SEMICOLON] = MEG4_KEY_SEMICOLON;
    main_keymap[SAPP_KEYCODE_EQUAL] = MEG4_KEY_EQUAL;
    main_keymap[SAPP_KEYCODE_A] = MEG4_KEY_A;
    main_keymap[SAPP_KEYCODE_B] = MEG4_KEY_B;
    main_keymap[SAPP_KEYCODE_C] = MEG4_KEY_C;
    main_keymap[SAPP_KEYCODE_D] = MEG4_KEY_D;
    main_keymap[SAPP_KEYCODE_E] = MEG4_KEY_E;
    main_keymap[SAPP_KEYCODE_F] = MEG4_KEY_F;
    main_keymap[SAPP_KEYCODE_G] = MEG4_KEY_G;
    main_keymap[SAPP_KEYCODE_H] = MEG4_KEY_H;
    main_keymap[SAPP_KEYCODE_I] = MEG4_KEY_I;
    main_keymap[SAPP_KEYCODE_J] = MEG4_KEY_J;
    main_keymap[SAPP_KEYCODE_K] = MEG4_KEY_K;
    main_keymap[SAPP_KEYCODE_L] = MEG4_KEY_L;
    main_keymap[SAPP_KEYCODE_M] = MEG4_KEY_M;
    main_keymap[SAPP_KEYCODE_N] = MEG4_KEY_N;
    main_keymap[SAPP_KEYCODE_O] = MEG4_KEY_O;
    main_keymap[SAPP_KEYCODE_P] = MEG4_KEY_P;
    main_keymap[SAPP_KEYCODE_Q] = MEG4_KEY_Q;
    main_keymap[SAPP_KEYCODE_R] = MEG4_KEY_R;
    main_keymap[SAPP_KEYCODE_S] = MEG4_KEY_S;
    main_keymap[SAPP_KEYCODE_T] = MEG4_KEY_T;
    main_keymap[SAPP_KEYCODE_U] = MEG4_KEY_U;
    main_keymap[SAPP_KEYCODE_V] = MEG4_KEY_V;
    main_keymap[SAPP_KEYCODE_X] = MEG4_KEY_X;
    main_keymap[SAPP_KEYCODE_Y] = MEG4_KEY_Y;
    main_keymap[SAPP_KEYCODE_Z] = MEG4_KEY_Z;
    main_keymap[SAPP_KEYCODE_LEFT_BRACKET] = MEG4_KEY_LBRACKET;
    main_keymap[SAPP_KEYCODE_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[SAPP_KEYCODE_RIGHT_BRACKET] = MEG4_KEY_RBRACKET;
    main_keymap[SAPP_KEYCODE_ENTER] = MEG4_KEY_ENTER;
    main_keymap[SAPP_KEYCODE_TAB] = MEG4_KEY_TAB;
    main_keymap[SAPP_KEYCODE_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[SAPP_KEYCODE_INSERT] = MEG4_KEY_INS;
    main_keymap[SAPP_KEYCODE_DELETE] = MEG4_KEY_DEL;
    main_keymap[SAPP_KEYCODE_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[SAPP_KEYCODE_LEFT] = MEG4_KEY_LEFT;
    main_keymap[SAPP_KEYCODE_DOWN] = MEG4_KEY_DOWN;
    main_keymap[SAPP_KEYCODE_UP] = MEG4_KEY_UP;
    main_keymap[SAPP_KEYCODE_PAGE_UP] = MEG4_KEY_PGUP;
    main_keymap[SAPP_KEYCODE_PAGE_DOWN] = MEG4_KEY_PGDN;
    main_keymap[SAPP_KEYCODE_HOME] = MEG4_KEY_HOME;
    main_keymap[SAPP_KEYCODE_END] = MEG4_KEY_END;
    main_keymap[SAPP_KEYCODE_CAPS_LOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[SAPP_KEYCODE_SCROLL_LOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[SAPP_KEYCODE_NUM_LOCK] = MEG4_KEY_NUMLOCK;
    main_keymap[SAPP_KEYCODE_PRINT_SCREEN] = MEG4_KEY_PRSCR;
    main_keymap[SAPP_KEYCODE_PAUSE] = MEG4_KEY_PAUSE;
    main_keymap[SAPP_KEYCODE_F1] = MEG4_KEY_F1;
    main_keymap[SAPP_KEYCODE_F2] = MEG4_KEY_F2;
    main_keymap[SAPP_KEYCODE_F3] = MEG4_KEY_F3;
    main_keymap[SAPP_KEYCODE_F4] = MEG4_KEY_F4;
    main_keymap[SAPP_KEYCODE_F5] = MEG4_KEY_F5;
    main_keymap[SAPP_KEYCODE_F6] = MEG4_KEY_F6;
    main_keymap[SAPP_KEYCODE_F7] = MEG4_KEY_F7;
    main_keymap[SAPP_KEYCODE_F8] = MEG4_KEY_F8;
    main_keymap[SAPP_KEYCODE_F9] = MEG4_KEY_F9;
    main_keymap[SAPP_KEYCODE_F10] = MEG4_KEY_F10;
    main_keymap[SAPP_KEYCODE_F11] = MEG4_KEY_F11;
    main_keymap[SAPP_KEYCODE_F12] = MEG4_KEY_F12;
    main_keymap[SAPP_KEYCODE_KP_0] = MEG4_KEY_KP_0;
    main_keymap[SAPP_KEYCODE_KP_1] = MEG4_KEY_KP_1;
    main_keymap[SAPP_KEYCODE_KP_2] = MEG4_KEY_KP_2;
    main_keymap[SAPP_KEYCODE_KP_3] = MEG4_KEY_KP_3;
    main_keymap[SAPP_KEYCODE_KP_4] = MEG4_KEY_KP_4;
    main_keymap[SAPP_KEYCODE_KP_5] = MEG4_KEY_KP_5;
    main_keymap[SAPP_KEYCODE_KP_6] = MEG4_KEY_KP_6;
    main_keymap[SAPP_KEYCODE_KP_7] = MEG4_KEY_KP_7;
    main_keymap[SAPP_KEYCODE_KP_8] = MEG4_KEY_KP_8;
    main_keymap[SAPP_KEYCODE_KP_9] = MEG4_KEY_KP_9;
    main_keymap[SAPP_KEYCODE_KP_DECIMAL] = MEG4_KEY_KP_DEC;
    main_keymap[SAPP_KEYCODE_KP_DIVIDE] = MEG4_KEY_KP_DIV;
    main_keymap[SAPP_KEYCODE_KP_MULTIPLY] = MEG4_KEY_KP_MUL;
    main_keymap[SAPP_KEYCODE_KP_SUBTRACT] = MEG4_KEY_KP_SUB;
    main_keymap[SAPP_KEYCODE_KP_ADD] = MEG4_KEY_KP_ADD;
    main_keymap[SAPP_KEYCODE_KP_ENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[SAPP_KEYCODE_KP_EQUAL] = MEG4_KEY_KP_EQUAL;
    main_keymap[SAPP_KEYCODE_LEFT_SHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[SAPP_KEYCODE_LEFT_CONTROL] = MEG4_KEY_LCTRL;
    main_keymap[SAPP_KEYCODE_LEFT_ALT] = MEG4_KEY_LALT;
    main_keymap[SAPP_KEYCODE_LEFT_SUPER] = MEG4_KEY_LSUPER;
    main_keymap[SAPP_KEYCODE_RIGHT_SHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[SAPP_KEYCODE_RIGHT_CONTROL] = MEG4_KEY_RCTRL;
    main_keymap[SAPP_KEYCODE_RIGHT_ALT] = MEG4_KEY_RALT;
    main_keymap[SAPP_KEYCODE_RIGHT_SUPER] = MEG4_KEY_RSUPER;
    main_keymap[SAPP_KEYCODE_MENU] = MEG4_KEY_MENU;

    scrbuf = (uint32_t*)malloc(640 * 400 * sizeof(uint32_t));
    if(!scrbuf) {
        main_log(0, "unable to allocate screen buffer");
        return (sapp_desc){ 0 };
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

    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = main_quit,
        .width = 0,
        .height = 0,
        .window_title = "MEG-4",
        .enable_clipboard = 1,
        .clipboard_size = sizeof(main_clip),
        .enable_dragndrop = 1,
        .icon.images = { { .width = meg4_icons.w, .height = 64, .pixels = { .ptr = meg4_icons.buf, .size = meg4_icons.w * 64 * 4 } } },
        .logger.func = slog
    };
}
