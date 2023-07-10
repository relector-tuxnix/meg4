/*
 * meg4/platform/allegro/main.c
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
 * @brief raylib "platform" for the MEG-4
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <time.h>
#include "meg4.h"

/* direty hacks to get direct pixel access... */
#define ALLEGRO_SRC
#define ASSERT(x)
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/internal/aintern_bitmap.h>

#define SAMPLES_PER_BUFFER 1024

ALLEGRO_EVENT_QUEUE *queue = NULL;
ALLEGRO_TIMER *timer = NULL;
ALLEGRO_DISPLAY *disp = NULL;
ALLEGRO_BITMAP *screen = NULL;
ALLEGRO_JOYSTICK *controller[4] = { 0 };
ALLEGRO_AUDIO_STREAM *stream = NULL;

int main_w = 0, main_h = 0, win_w, win_h, win_f = 0, audio = 0, main_alt = 0, main_keymap[512];
void main_delay(int msec);

#define meg4_showcursor()   al_show_mouse_cursor(disp);
#define meg4_hidecursor()   al_hide_mouse_cursor(disp);
#include "../common.h"

/**
 * Exit emulator
 */
void main_quit(void)
{
    main_log(1, "quitting...         ");
    meg4_poweroff();
    if(screen) al_destroy_bitmap(screen);
    if(disp) al_destroy_display(disp);
    if(timer) al_destroy_timer(timer);
    if(queue) al_destroy_event_queue(queue);
    if(stream) { al_drain_audio_stream(stream); al_destroy_audio_stream(stream); }
    if(audio) al_uninstall_audio();
    exit(0);
}

/**
 * Create window
 */
void main_win(int w, int h, int f)
{
    ALLEGRO_BITMAP *icon;

    win_f = f; (void)w; (void)h;
    for(win_w = win_h = 0; win_w + 320 < main_w && win_h + 200 < main_h; win_w += 320, win_h += 200);
    if((disp = al_create_display(win_w, win_h))) {
        al_set_window_constraints(disp, 320, 200, main_w, main_h);
        al_set_window_title(disp, "MEG-4");
        if((icon = al_create_bitmap(meg4_icons.w, 64))) {
            memcpy(icon->memory, meg4_icons.buf, meg4_icons.w * 4 * 64);
            al_set_display_icon(disp, icon);
            al_destroy_bitmap(icon);
        }
    }
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    al_set_display_flag(disp, ALLEGRO_FULLSCREEN_WINDOW, !(al_get_display_flags(disp) & ALLEGRO_FULLSCREEN_WINDOW));
}

/**
 * Make window focused
 */
void main_focus(void)
{
}

/**
 * Mouse movement event callback
 */
void main_pointer(int xpos, int ypos)
{
    int x, y, w, h, ww, wh, xp = xpos, yp = ypos;
    if(!meg4.screen.w || !meg4.screen.h) return;
    ww = al_get_display_width(disp); wh = al_get_display_height(disp);
    w = ww; h = (int)meg4.screen.h * ww / (int)meg4.screen.w;
    if(h > wh) { h = wh; w = (int)meg4.screen.w * wh / (int)meg4.screen.h; }
    x = (ww - w) >> 1; y = (wh - h) >> 1;
    meg4_setptr(xp < x || !w ? 0 : (xp >= x + w ? meg4.screen.w : (xp - x) * meg4.screen.w / w),
        yp < y || !h ? 0 : (yp >= y + h ? meg4.screen.h : (yp - y) * meg4.screen.h / h));
}

/**
 * Get text from clipboard (must be freed by caller)
 */
char *main_getclipboard(void)
{
    /* this returns an allocated buffer */
    return (char*)al_get_clipboard_text(disp);
}

/**
 * Set text to clipboard
 */
void main_setclipboard(char *str)
{
    al_set_clipboard_text(disp, (const char*)str);
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
    printf("\r\nMEG-4 v%s (allegro%u, build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, ALLEGRO_VERSION, BUILD);
}

/**
 * The real main procedure
 */
int main(int argc, char **argv)
{
    ALLEGRO_MOUSE_STATE state;
    ALLEGRO_MONITOR_INFO info;
    ALLEGRO_EVENT event;
    float *abuf;
    int i, w, h, ww, wh, redraw, running = 1;
    char *infile = NULL, *fn;
    char s[5];
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

    if(!al_init()) {
        main_log(0, "unable to initialize allegro");
        return 1;
    }
    al_install_keyboard();
    al_install_mouse();
    al_install_joystick();
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
    timer = al_create_timer(1.0 / 60.0);
    queue = al_create_event_queue();

    /* set up keymap */
    memset(main_keymap, 0, sizeof(main_keymap));
    main_keymap[ALLEGRO_KEY_SPACE] = MEG4_KEY_SPACE;
    main_keymap[ALLEGRO_KEY_QUOTE] = MEG4_KEY_APOSTROPHE;
    main_keymap[ALLEGRO_KEY_TILDE] = MEG4_KEY_BACKQUOTE;
    main_keymap[ALLEGRO_KEY_COMMA] = MEG4_KEY_COMMA;
    main_keymap[ALLEGRO_KEY_MINUS] = MEG4_KEY_MINUS;
    main_keymap[ALLEGRO_KEY_FULLSTOP] = MEG4_KEY_PERIOD;
    main_keymap[ALLEGRO_KEY_SLASH] = MEG4_KEY_SLASH;
    main_keymap[ALLEGRO_KEY_0] = MEG4_KEY_0;
    main_keymap[ALLEGRO_KEY_1] = MEG4_KEY_1;
    main_keymap[ALLEGRO_KEY_2] = MEG4_KEY_2;
    main_keymap[ALLEGRO_KEY_3] = MEG4_KEY_3;
    main_keymap[ALLEGRO_KEY_4] = MEG4_KEY_4;
    main_keymap[ALLEGRO_KEY_5] = MEG4_KEY_5;
    main_keymap[ALLEGRO_KEY_6] = MEG4_KEY_6;
    main_keymap[ALLEGRO_KEY_7] = MEG4_KEY_7;
    main_keymap[ALLEGRO_KEY_8] = MEG4_KEY_8;
    main_keymap[ALLEGRO_KEY_9] = MEG4_KEY_9;
    main_keymap[ALLEGRO_KEY_SEMICOLON] = MEG4_KEY_SEMICOLON;
    main_keymap[ALLEGRO_KEY_EQUALS] = MEG4_KEY_EQUAL;
    main_keymap[ALLEGRO_KEY_A] = MEG4_KEY_A;
    main_keymap[ALLEGRO_KEY_B] = MEG4_KEY_B;
    main_keymap[ALLEGRO_KEY_C] = MEG4_KEY_C;
    main_keymap[ALLEGRO_KEY_D] = MEG4_KEY_D;
    main_keymap[ALLEGRO_KEY_E] = MEG4_KEY_E;
    main_keymap[ALLEGRO_KEY_F] = MEG4_KEY_F;
    main_keymap[ALLEGRO_KEY_G] = MEG4_KEY_G;
    main_keymap[ALLEGRO_KEY_H] = MEG4_KEY_H;
    main_keymap[ALLEGRO_KEY_I] = MEG4_KEY_I;
    main_keymap[ALLEGRO_KEY_J] = MEG4_KEY_J;
    main_keymap[ALLEGRO_KEY_K] = MEG4_KEY_K;
    main_keymap[ALLEGRO_KEY_L] = MEG4_KEY_L;
    main_keymap[ALLEGRO_KEY_M] = MEG4_KEY_M;
    main_keymap[ALLEGRO_KEY_N] = MEG4_KEY_N;
    main_keymap[ALLEGRO_KEY_O] = MEG4_KEY_O;
    main_keymap[ALLEGRO_KEY_P] = MEG4_KEY_P;
    main_keymap[ALLEGRO_KEY_Q] = MEG4_KEY_Q;
    main_keymap[ALLEGRO_KEY_R] = MEG4_KEY_R;
    main_keymap[ALLEGRO_KEY_S] = MEG4_KEY_S;
    main_keymap[ALLEGRO_KEY_T] = MEG4_KEY_T;
    main_keymap[ALLEGRO_KEY_U] = MEG4_KEY_U;
    main_keymap[ALLEGRO_KEY_V] = MEG4_KEY_V;
    main_keymap[ALLEGRO_KEY_X] = MEG4_KEY_X;
    main_keymap[ALLEGRO_KEY_Y] = MEG4_KEY_Y;
    main_keymap[ALLEGRO_KEY_Z] = MEG4_KEY_Z;
    main_keymap[ALLEGRO_KEY_OPENBRACE] = MEG4_KEY_LBRACKET;
    main_keymap[ALLEGRO_KEY_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[ALLEGRO_KEY_CLOSEBRACE] = MEG4_KEY_RBRACKET;
    main_keymap[ALLEGRO_KEY_ENTER] = MEG4_KEY_ENTER;
    main_keymap[ALLEGRO_KEY_TAB] = MEG4_KEY_TAB;
    main_keymap[ALLEGRO_KEY_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[ALLEGRO_KEY_INSERT] = MEG4_KEY_INS;
    main_keymap[ALLEGRO_KEY_DELETE] = MEG4_KEY_DEL;
    main_keymap[ALLEGRO_KEY_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[ALLEGRO_KEY_LEFT] = MEG4_KEY_LEFT;
    main_keymap[ALLEGRO_KEY_DOWN] = MEG4_KEY_DOWN;
    main_keymap[ALLEGRO_KEY_UP] = MEG4_KEY_UP;
    main_keymap[ALLEGRO_KEY_PGUP] = MEG4_KEY_PGUP;
    main_keymap[ALLEGRO_KEY_PGDN] = MEG4_KEY_PGDN;
    main_keymap[ALLEGRO_KEY_HOME] = MEG4_KEY_HOME;
    main_keymap[ALLEGRO_KEY_END] = MEG4_KEY_END;
    main_keymap[ALLEGRO_KEY_CAPSLOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[ALLEGRO_KEY_SCROLLLOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[ALLEGRO_KEY_NUMLOCK] = MEG4_KEY_NUMLOCK;
    main_keymap[ALLEGRO_KEY_PRINTSCREEN] = MEG4_KEY_PRSCR;
    main_keymap[ALLEGRO_KEY_PAUSE] = MEG4_KEY_PAUSE;
    main_keymap[ALLEGRO_KEY_F1] = MEG4_KEY_F1;
    main_keymap[ALLEGRO_KEY_F2] = MEG4_KEY_F2;
    main_keymap[ALLEGRO_KEY_F3] = MEG4_KEY_F3;
    main_keymap[ALLEGRO_KEY_F4] = MEG4_KEY_F4;
    main_keymap[ALLEGRO_KEY_F5] = MEG4_KEY_F5;
    main_keymap[ALLEGRO_KEY_F6] = MEG4_KEY_F6;
    main_keymap[ALLEGRO_KEY_F7] = MEG4_KEY_F7;
    main_keymap[ALLEGRO_KEY_F8] = MEG4_KEY_F8;
    main_keymap[ALLEGRO_KEY_F9] = MEG4_KEY_F9;
    main_keymap[ALLEGRO_KEY_F10] = MEG4_KEY_F10;
    main_keymap[ALLEGRO_KEY_F11] = MEG4_KEY_F11;
    main_keymap[ALLEGRO_KEY_F12] = MEG4_KEY_F12;
    main_keymap[ALLEGRO_KEY_PAD_0] = MEG4_KEY_KP_0;
    main_keymap[ALLEGRO_KEY_PAD_1] = MEG4_KEY_KP_1;
    main_keymap[ALLEGRO_KEY_PAD_2] = MEG4_KEY_KP_2;
    main_keymap[ALLEGRO_KEY_PAD_3] = MEG4_KEY_KP_3;
    main_keymap[ALLEGRO_KEY_PAD_4] = MEG4_KEY_KP_4;
    main_keymap[ALLEGRO_KEY_PAD_5] = MEG4_KEY_KP_5;
    main_keymap[ALLEGRO_KEY_PAD_6] = MEG4_KEY_KP_6;
    main_keymap[ALLEGRO_KEY_PAD_7] = MEG4_KEY_KP_7;
    main_keymap[ALLEGRO_KEY_PAD_8] = MEG4_KEY_KP_8;
    main_keymap[ALLEGRO_KEY_PAD_9] = MEG4_KEY_KP_9;
    main_keymap[ALLEGRO_KEY_PAD_DELETE] = MEG4_KEY_KP_DEC;
    main_keymap[ALLEGRO_KEY_PAD_SLASH] = MEG4_KEY_KP_DIV;
    main_keymap[ALLEGRO_KEY_PAD_ASTERISK] = MEG4_KEY_KP_MUL;
    main_keymap[ALLEGRO_KEY_PAD_MINUS] = MEG4_KEY_KP_SUB;
    main_keymap[ALLEGRO_KEY_PAD_PLUS] = MEG4_KEY_KP_ADD;
    main_keymap[ALLEGRO_KEY_PAD_ENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[ALLEGRO_KEY_PAD_EQUALS] = MEG4_KEY_KP_EQUAL;
    main_keymap[ALLEGRO_KEY_LSHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[ALLEGRO_KEY_LCTRL] = MEG4_KEY_LCTRL;
    main_keymap[ALLEGRO_KEY_ALT] = MEG4_KEY_LALT;
    main_keymap[ALLEGRO_KEY_LWIN] = MEG4_KEY_LSUPER;
    main_keymap[ALLEGRO_KEY_RSHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[ALLEGRO_KEY_RCTRL] = MEG4_KEY_RCTRL;
    main_keymap[ALLEGRO_KEY_ALTGR] = MEG4_KEY_RALT;
    main_keymap[ALLEGRO_KEY_RWIN] = MEG4_KEY_RSUPER;
    main_keymap[ALLEGRO_KEY_MENU] = MEG4_KEY_MENU;

    audio = al_install_audio() && al_reserve_samples(0);
    if(audio) {
        stream = al_create_audio_stream(4, SAMPLES_PER_BUFFER, 44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_1);
        if(!stream) { al_uninstall_audio(); audio = 0; } else
        if(!al_attach_audio_stream_to_mixer(stream, al_get_default_mixer())) { al_destroy_audio_stream(stream); al_uninstall_audio(); audio = 0; }
    }
    if(verbose && audio) main_log(1, "audio opened %uHz, %u bits", 44100, 32);

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
    al_get_monitor_info(0, &info);
    main_w = info.x2 - info.x1; main_h = info.y2 - info.y1;
#if DEBUG
    main_win(640, 400, 0);
#else
    main_win(640/*main_w*/, 400/*main_h*/, 0/*1*/);
    main_fullscreen();
#endif
    screen = al_create_bitmap(640, 400);
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_joystick_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_get_mouse_state(&state);
    main_pointer(al_get_mouse_state_axis(&state, 0), al_get_mouse_state_axis(&state, 1));
    meg4_hidecursor();
    al_reconfigure_joysticks();
    memset(controller, 0, sizeof(controller));
    for(i = 0; i < al_get_num_joysticks() && i < 4; i++)
        controller[i] = al_get_joystick(i);
    if(audio) al_register_event_source(queue, al_get_audio_stream_event_source(stream));
    al_start_timer(timer);
    redraw = 0; running = 1;
    while(running) {
        al_wait_for_event(queue, &event);
        switch(event.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE: running = 0; break;
            case ALLEGRO_EVENT_TIMER: meg4_run(); redraw = 1; break;
            /* audio event */
            case ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT:
                if((abuf = al_get_audio_stream_fragment(stream))) {
                    meg4_audiofeed(abuf, SAMPLES_PER_BUFFER);
                    al_set_audio_stream_fragment(stream, abuf);
                }
            break;
            /* mouse events */
            case ALLEGRO_EVENT_MOUSE_AXES:
                main_pointer(event.mouse.x, event.mouse.y);
                if(event.mouse.dz || event.mouse.dw) meg4_setscr(event.mouse.dz > 0, event.mouse.dz < 0, event.mouse.dw > 0, event.mouse.dw < 0);
            break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                main_pointer(event.mouse.x, event.mouse.y);
                switch(event.mouse.button) {
                    case 1: meg4_setbtn(MEG4_BTN_L); break;
                    case 2: meg4_setbtn(MEG4_BTN_R); break;
                    case 3: meg4_setbtn(MEG4_BTN_M); break;
                }
            break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                main_pointer(event.mouse.x, event.mouse.y);
                switch(event.mouse.button) {
                    case 1: meg4_clrbtn(MEG4_BTN_L); break;
                    case 2: meg4_clrbtn(MEG4_BTN_R); break;
                    case 3: meg4_clrbtn(MEG4_BTN_M); break;
                }
            break;
            /* keyboard events */
            case ALLEGRO_EVENT_KEY_DOWN:
                switch(event.keyboard.keycode) {
                    case ALLEGRO_KEY_ALT: case ALLEGRO_KEY_LCTRL: main_alt = 1; break;
                    case ALLEGRO_KEY_ENTER: if(main_alt) main_fullscreen(); else meg4_pushkey("\n\0\0"); break;
                    case ALLEGRO_KEY_Q: if(main_alt) { running = 0; } break;
                    case ALLEGRO_KEY_ESCAPE: if(main_alt) { running = 0; } else meg4_pushkey("\x1b\0\0"); break;
                    /* only for special keys that aren't handled by SDL_TEXTINPUT events */
                    case ALLEGRO_KEY_F1: meg4_pushkey("F1\0"); break;
                    case ALLEGRO_KEY_F2: meg4_pushkey("F2\0"); break;
                    case ALLEGRO_KEY_F3: meg4_pushkey("F3\0"); break;
                    case ALLEGRO_KEY_F4: meg4_pushkey("F4\0"); break;
                    case ALLEGRO_KEY_F5: meg4_pushkey("F5\0"); break;
                    case ALLEGRO_KEY_F6: meg4_pushkey("F6\0"); break;
                    case ALLEGRO_KEY_F7: meg4_pushkey("F7\0"); break;
                    case ALLEGRO_KEY_F8: meg4_pushkey("F8\0"); break;
                    case ALLEGRO_KEY_F9: meg4_pushkey("F9\0"); break;
                    case ALLEGRO_KEY_F10: meg4_pushkey("F10"); break;
                    case ALLEGRO_KEY_F11: meg4_pushkey("F11"); break;
                    case ALLEGRO_KEY_F12: meg4_pushkey("F12"); break;
                    case ALLEGRO_KEY_PRINTSCREEN: meg4_pushkey("PSc"); break;
                    case ALLEGRO_KEY_SCROLLLOCK: meg4_pushkey("SLk"); break;
                    case ALLEGRO_KEY_NUMLOCK: meg4_pushkey("NLk"); break;
                    case ALLEGRO_KEY_BACKSPACE: meg4_pushkey("\b\0\0"); break;
                    case ALLEGRO_KEY_TAB: meg4_pushkey("\t\0\0"); break;
                    case ALLEGRO_KEY_CAPSLOCK: meg4_pushkey("CLk"); break;
                    case ALLEGRO_KEY_UP: meg4_pushkey("Up\0"); break;
                    case ALLEGRO_KEY_DOWN: meg4_pushkey("Down"); break;
                    case ALLEGRO_KEY_LEFT: meg4_pushkey("Left"); break;
                    case ALLEGRO_KEY_RIGHT: meg4_pushkey("Rght"); break;
                    case ALLEGRO_KEY_HOME: meg4_pushkey("Home"); break;
                    case ALLEGRO_KEY_END: meg4_pushkey("End"); break;
                    case ALLEGRO_KEY_PGUP: meg4_pushkey("PgUp"); break;
                    case ALLEGRO_KEY_PGDN: meg4_pushkey("PgDn"); break;
                    case ALLEGRO_KEY_INSERT: meg4_pushkey("Ins"); break;
                    case ALLEGRO_KEY_DELETE: meg4_pushkey("Del"); break;
                }
                if(event.keyboard.keycode < 512 && main_keymap[event.keyboard.keycode]) meg4_setkey(main_keymap[event.keyboard.keycode]);
            break;
            case ALLEGRO_EVENT_KEY_UP:
                switch(event.keyboard.keycode) {
                    case ALLEGRO_KEY_ALT: case ALLEGRO_KEY_LCTRL: main_alt = 0; break;
                }
                if(event.keyboard.keycode < 512 && main_keymap[event.keyboard.keycode]) meg4_clrkey(main_keymap[event.keyboard.keycode]);
            break;
            case ALLEGRO_EVENT_KEY_CHAR:
                if(!main_alt && event.keyboard.unichar >= 32) {
                    memset(s, 0, sizeof(s));
                    if(event.keyboard.unichar<0x80) { s[0]=event.keyboard.unichar; } else
                    if(event.keyboard.unichar<0x800) { s[0]=((event.keyboard.unichar>>6)&0x1F)|0xC0; s[1]=(event.keyboard.unichar&0x3F)|0x80; } else
                    if(event.keyboard.unichar<0x10000) { s[0]=((event.keyboard.unichar>>12)&0x0F)|0xE0; s[1]=((event.keyboard.unichar>>6)&0x3F)|0x80; s[2]=(event.keyboard.unichar&0x3F)|0x80; }
                    else { s[0]=((event.keyboard.unichar>>18)&0x07)|0xF0; s[1]=((event.keyboard.unichar>>12)&0x3F)|0x80; s[2]=((event.keyboard.unichar>>6)&0x3F)|0x80; s[3]=(event.keyboard.unichar&0x3F)|0x80; }
                    meg4_pushkey(s);
                }
            break;
            /* gamepad events */
            case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
                al_reconfigure_joysticks();
                memset(controller, 0, sizeof(controller));
                for(i = 0; i < al_get_num_joysticks() && i < 4; i++)
                    controller[i] = al_get_joystick(i);
            break;
            case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
                for(i = 0; i < 4 && controller[i] != event.joystick.id; i++);
                if(i < 4) {
                    /* not sure about button order */
                    switch(event.joystick.button) {
                        case 0: meg4_setpad(i, MEG4_BTN_L); break;
                        case 1: meg4_setpad(i, MEG4_BTN_U); break;
                        case 2: meg4_setpad(i, MEG4_BTN_R); break;
                        case 3: meg4_setpad(i, MEG4_BTN_D); break;
                        case 4: meg4_setpad(i, MEG4_BTN_A); break;
                        case 5: meg4_setpad(i, MEG4_BTN_B); break;
                        case 6: meg4_setpad(i, MEG4_BTN_X); break;
                        case 7: meg4_setpad(i, MEG4_BTN_Y); break;
                    }
                }
            break;
            case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
                for(i = 0; i < 4 && controller[i] != event.joystick.id; i++);
                if(i < 4) {
                    switch(event.joystick.button) {
                        case 0: meg4_clrpad(i, MEG4_BTN_L); break;
                        case 1: meg4_clrpad(i, MEG4_BTN_U); break;
                        case 2: meg4_clrpad(i, MEG4_BTN_R); break;
                        case 3: meg4_clrpad(i, MEG4_BTN_D); break;
                        case 4: meg4_clrpad(i, MEG4_BTN_A); break;
                        case 5: meg4_clrpad(i, MEG4_BTN_B); break;
                        case 6: meg4_clrpad(i, MEG4_BTN_X); break;
                        case 7: meg4_clrpad(i, MEG4_BTN_Y); break;
                    }
                }
            break;
            case ALLEGRO_EVENT_JOYSTICK_AXIS:
                for(i = 0; i < 4 && controller[i] != event.joystick.id; i++);
                if(i < 4) {
                    w = event.joystick.pos * 32767.0f;
                    if(!event.joystick.axis) {
                        meg4_clrpad(i, MEG4_BTN_L | MEG4_BTN_R);
                        if(w < -le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_L);
                        if(w >  le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_R);
                    } else {
                        meg4_clrpad(i, MEG4_BTN_U | MEG4_BTN_D);
                        if(w < -le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_U);
                        if(w >  le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_D);
                    }
                }
            break;
        }

        if(redraw && al_is_event_queue_empty(queue)) {
            redraw = 0;
            /* display screen */
            meg4_redraw((uint32_t*)screen->memory, 640, 400, screen->pitch);
            al_clear_to_color(al_map_rgba_f(0, 0, 0, 1));
            ww = al_get_display_width(disp); wh = al_get_display_height(disp);
            w = ww; h = (int)meg4.screen.h * ww / (int)meg4.screen.w;
            if(h > wh) { h = wh; w = (int)meg4.screen.w * wh / (int)meg4.screen.h; }
            al_draw_scaled_bitmap(screen, 0, 0, (float)meg4.screen.w, (float)meg4.screen.h,
                (float)((ww - w) >> 1), (float)((wh - h) >> 1), (float)ww, (float)wh, 0);
            al_flip_display();
        }
    }
    main_quit();
    return 0;
}
