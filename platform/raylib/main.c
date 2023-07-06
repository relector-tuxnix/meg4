/*
 * meg4/platform/raylib/main.c
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
#include <raylib.h>

Image image;
Texture2D screen;
AudioStream stream;

int main_w = 0, main_h = 0, win_w, win_h, win_f = 0, audio = 0, main_alt = 0, main_keymap[512];
void main_delay(int msec);

#define meg4_showcursor() EnableCursor()
#define meg4_hidecursor() DisableCursor()
#include "../common.h"

/**
 * Exit emulator
 */
void main_quit(void)
{
    main_log(1, "quitting...         ");
    meg4_poweroff();
    if(audio) { UnloadAudioStream(stream); CloseAudioDevice(); }
    UnloadImage(image);
    UnloadTexture(screen);
    ShowCursor();
    CloseWindow();
    exit(0);
}

/**
 * Create window
 */
void main_win(int w, int h, int f)
{
    Image icon;

    win_f = f;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(w, h, "MEG-4");
    SetExitKey(KEY_NULL);
    if(meg4_icons.buf) {
        icon.width = meg4_icons.w; icon.height = 64; icon.data = meg4_icons.buf; icon.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        SetWindowIcon(icon);
    }
    /* unfortuantely this only works *after* an InitWindow call was already made */
    main_w = GetMonitorWidth(GetCurrentMonitor());
    main_h = GetMonitorHeight(GetCurrentMonitor());
    for(win_w = win_h = 0; win_w + 320 < main_w && win_h + 200 < main_h; win_w += 320, win_h += 200);
    SetWindowMinSize(win_w, win_h);
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    ToggleFullscreen();
    if((win_f = IsWindowFullscreen()))
        SetWindowSize(main_w, main_h);
}

/**
 * Make window focused
 */
void main_focus(void)
{
    SetWindowFocused();
}

/**
 * Mouse movement event callback
 */
void main_pointer(float xpos, float ypos)
{
    int x, y, w, h, ww, wh, xp = xpos, yp = ypos;
    if(!meg4.screen.w || !meg4.screen.h) return;
    ww = GetRenderWidth(); wh = GetRenderHeight();
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
    const char *str = GetClipboardText();
    char *ret = NULL;
    if(str && *str) {
        ret = (char*)malloc(strlen(str) + 1);
        if(ret) strcpy(ret, str);
    }
    return ret;
}

/**
 * Set text to clipboard
 */
void main_setclipboard(char *str)
{
    SetClipboardText((const char*)str);
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
void main_audio(void *buffer, unsigned int frames)
{
    meg4_audiofeed((float*)buffer, frames);
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
    printf("\r\nMEG-4 v%s (raylib, build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, BUILD);
}

/**
 * The real main procedure
 */
int main(int argc, char **argv)
{
    FilePathList droppedFiles;
    Rectangle src, dst;
    Vector2 mousePosition, origin = { 0 };
    int i, w, h, ww, wh;
    unsigned int unicode;
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
    /* set up keymap */
    memset(main_keymap, 0, sizeof(main_keymap));
    main_keymap[KEY_SPACE] = MEG4_KEY_SPACE;
    main_keymap[KEY_APOSTROPHE] = MEG4_KEY_APOSTROPHE;
    main_keymap[KEY_COMMA] = MEG4_KEY_COMMA;
    main_keymap[KEY_MINUS] = MEG4_KEY_MINUS;
    main_keymap[KEY_PERIOD] = MEG4_KEY_PERIOD;
    main_keymap[KEY_SLASH] = MEG4_KEY_SLASH;
    main_keymap[KEY_ZERO] = MEG4_KEY_0;
    main_keymap[KEY_ONE] = MEG4_KEY_1;
    main_keymap[KEY_TWO] = MEG4_KEY_2;
    main_keymap[KEY_THREE] = MEG4_KEY_3;
    main_keymap[KEY_FOUR] = MEG4_KEY_4;
    main_keymap[KEY_FIVE] = MEG4_KEY_5;
    main_keymap[KEY_SIX] = MEG4_KEY_6;
    main_keymap[KEY_SEVEN] = MEG4_KEY_7;
    main_keymap[KEY_EIGHT] = MEG4_KEY_8;
    main_keymap[KEY_NINE] = MEG4_KEY_9;
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
    main_keymap[KEY_LEFT_BRACKET] = MEG4_KEY_LBRACKET;
    main_keymap[KEY_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[KEY_RIGHT_BRACKET] = MEG4_KEY_RBRACKET;
    main_keymap[KEY_ENTER] = MEG4_KEY_ENTER;
    main_keymap[KEY_TAB] = MEG4_KEY_TAB;
    main_keymap[KEY_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[KEY_INSERT] = MEG4_KEY_INS;
    main_keymap[KEY_DELETE] = MEG4_KEY_DEL;
    main_keymap[KEY_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[KEY_LEFT] = MEG4_KEY_LEFT;
    main_keymap[KEY_DOWN] = MEG4_KEY_DOWN;
    main_keymap[KEY_UP] = MEG4_KEY_UP;
    main_keymap[KEY_PAGE_UP] = MEG4_KEY_PGUP;
    main_keymap[KEY_PAGE_DOWN] = MEG4_KEY_PGDN;
    main_keymap[KEY_HOME] = MEG4_KEY_HOME;
    main_keymap[KEY_END] = MEG4_KEY_END;
    main_keymap[KEY_CAPS_LOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[KEY_SCROLL_LOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[KEY_NUM_LOCK] = MEG4_KEY_NUMLOCK;
    main_keymap[KEY_PRINT_SCREEN] = MEG4_KEY_PRSCR;
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
    main_keymap[KEY_KP_0] = MEG4_KEY_KP_0;
    main_keymap[KEY_KP_1] = MEG4_KEY_KP_1;
    main_keymap[KEY_KP_2] = MEG4_KEY_KP_2;
    main_keymap[KEY_KP_3] = MEG4_KEY_KP_3;
    main_keymap[KEY_KP_4] = MEG4_KEY_KP_4;
    main_keymap[KEY_KP_5] = MEG4_KEY_KP_5;
    main_keymap[KEY_KP_6] = MEG4_KEY_KP_6;
    main_keymap[KEY_KP_7] = MEG4_KEY_KP_7;
    main_keymap[KEY_KP_8] = MEG4_KEY_KP_8;
    main_keymap[KEY_KP_9] = MEG4_KEY_KP_9;
    main_keymap[KEY_KP_DECIMAL] = MEG4_KEY_KP_DEC;
    main_keymap[KEY_KP_DIVIDE] = MEG4_KEY_KP_DIV;
    main_keymap[KEY_KP_MULTIPLY] = MEG4_KEY_KP_MUL;
    main_keymap[KEY_KP_SUBTRACT] = MEG4_KEY_KP_SUB;
    main_keymap[KEY_KP_ADD] = MEG4_KEY_KP_ADD;
    main_keymap[KEY_KP_ENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[KEY_KP_EQUAL] = MEG4_KEY_KP_EQUAL;
    main_keymap[KEY_LEFT_SHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[KEY_LEFT_CONTROL] = MEG4_KEY_LCTRL;
    main_keymap[KEY_LEFT_ALT] = MEG4_KEY_LALT;
    main_keymap[KEY_LEFT_SUPER] = MEG4_KEY_LSUPER;
    main_keymap[KEY_RIGHT_SHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[KEY_RIGHT_CONTROL] = MEG4_KEY_RCTRL;
    main_keymap[KEY_RIGHT_ALT] = MEG4_KEY_RALT;
    main_keymap[KEY_RIGHT_SUPER] = MEG4_KEY_RSUPER;
    main_keymap[KEY_MENU] = MEG4_KEY_MENU;
    image.data = malloc(640 * 400 * sizeof(uint32_t));
    if(!image.data) {
        main_log(0, "unable to allocate screen buffer");
        return 1;
    }
    image.width = 640;
    image.height = 400;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    image.mipmaps = 1;

    InitAudioDevice();
    if((audio = IsAudioDeviceReady())) {
        SetAudioStreamBufferSizeDefault(4096);
        stream = LoadAudioStream(44100, 32, 1);
        if(!stream.sampleSize) { CloseAudioDevice(); audio = 0; }
        else SetAudioStreamCallback(stream, main_audio);
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
#if DEBUG
    main_win(640, 400, 0);
#else
    main_win(640/*main_w*/, 400/*main_h*/, 0/*1*/);
    main_fullscreen();
#endif
    if(audio) PlayAudioStream(stream);
    SetTargetFPS(60);
    HideCursor();
    screen = LoadTextureFromImage(image);
    while(!WindowShouldClose()) {
        /* mouse events */
        mousePosition = GetMousePosition();
        main_pointer(mousePosition.x, mousePosition.y);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) meg4_setbtn(MEG4_BTN_L);
        else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) meg4_clrbtn(MEG4_BTN_L);
        if(IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) meg4_setbtn(MEG4_BTN_M);
        else if(IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) meg4_clrbtn(MEG4_BTN_M);
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) meg4_setbtn(MEG4_BTN_R);
        else if(IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) meg4_clrbtn(MEG4_BTN_R);
        mousePosition = GetMouseWheelMoveV();
        meg4_setscr(mousePosition.y > 0.0, mousePosition.y < 0.0, mousePosition.x > 0.0, mousePosition.x < 0.0);
        /* keyboard events */
        main_alt = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_ALT);
        if(IsKeyPressed(KEY_ESCAPE)) meg4_pushkey("\x1b\0\0");
        for(i = 0; i < (int)(sizeof(main_keymap)/sizeof(main_keymap[0])); i++)
            if(main_keymap[i]) {
                if(IsKeyPressed(i)) {
                    if(main_alt)
                        switch(i) {
                            case KEY_ENTER: main_fullscreen(); break;
                            case KEY_Q: CloseWindow(); break;
                    }
                    /* only for special keys that aren't handled by GetCharPressed() */
                    switch(i) {
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
                        case KEY_PRINT_SCREEN: meg4_pushkey("PSc"); break;
                        case KEY_SCROLL_LOCK: meg4_pushkey("SLk"); break;
                        case KEY_NUM_LOCK: meg4_pushkey("NLk"); break;
                        case KEY_BACKSPACE: meg4_pushkey("\b\0\0"); break;
                        case KEY_TAB: meg4_pushkey("\t\0\0"); break;
                        case KEY_ENTER: meg4_pushkey("\n\0\0"); break;
                        case KEY_CAPS_LOCK: meg4_pushkey("CLk"); break;
                        case KEY_UP: meg4_pushkey("Up\0"); break;
                        case KEY_DOWN: meg4_pushkey("Down"); break;
                        case KEY_LEFT: meg4_pushkey("Left"); break;
                        case KEY_RIGHT: meg4_pushkey("Rght"); break;
                        case KEY_HOME: meg4_pushkey("Home"); break;
                        case KEY_END: meg4_pushkey("End"); break;
                        case KEY_PAGE_UP: meg4_pushkey("PgUp"); break;
                        case KEY_PAGE_DOWN: meg4_pushkey("PgDn"); break;
                        case KEY_INSERT: meg4_pushkey("Ins"); break;
                        case KEY_DELETE: meg4_pushkey("Del"); break;
                    }
                    meg4_setkey(main_keymap[i]);
                } else
                if(IsKeyReleased(i))
                    meg4_clrkey(main_keymap[i]);
            }
        while((unicode = GetCharPressed())) {
            if(main_alt || unicode < 32) continue;
            memset(s, 0, sizeof(s));
            if(unicode<0x80) { s[0]=unicode; } else
            if(unicode<0x800) { s[0]=((unicode>>6)&0x1F)|0xC0; s[1]=(unicode&0x3F)|0x80; } else
            if(unicode<0x10000) { s[0]=((unicode>>12)&0x0F)|0xE0; s[1]=((unicode>>6)&0x3F)|0x80; s[2]=(unicode&0x3F)|0x80; }
            else { s[0]=((unicode>>18)&0x07)|0xF0; s[1]=((unicode>>12)&0x3F)|0x80; s[2]=((unicode>>6)&0x3F)|0x80; s[3]=(unicode&0x3F)|0x80; }
            meg4_pushkey(s);
        }
        /* controller events */
        for(i = 0; i < 4; i++)
            if(IsGamepadAvailable(i)) {
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) meg4_setpad(i, MEG4_BTN_L);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) meg4_clrpad(i, MEG4_BTN_L);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) meg4_setpad(i, MEG4_BTN_R);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) meg4_clrpad(i, MEG4_BTN_R);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_UP)) meg4_setpad(i, MEG4_BTN_U);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_UP)) meg4_clrpad(i, MEG4_BTN_U);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) meg4_setpad(i, MEG4_BTN_D);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) meg4_clrpad(i, MEG4_BTN_D);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) meg4_setpad(i, MEG4_BTN_A);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) meg4_clrpad(i, MEG4_BTN_A);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) meg4_setpad(i, MEG4_BTN_B);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) meg4_clrpad(i, MEG4_BTN_B);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) meg4_setpad(i, MEG4_BTN_X);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) meg4_clrpad(i, MEG4_BTN_X);
                if(IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_UP)) meg4_setpad(i, MEG4_BTN_Y);
                else if(IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_UP)) meg4_clrpad(i, MEG4_BTN_Y);
                w = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_X); h = GetGamepadAxisMovement(i, GAMEPAD_AXIS_RIGHT_X);
                if(w < -le16toh(meg4.mmio.padtres) || h < -le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_L);
                else if(!IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) meg4_clrpad(i, MEG4_BTN_L);
                if(w > le16toh(meg4.mmio.padtres) || h > le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_R);
                else if(!IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) meg4_clrpad(i, MEG4_BTN_R);
                w = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_Y); h = GetGamepadAxisMovement(i, GAMEPAD_AXIS_RIGHT_Y);
                if(w < -le16toh(meg4.mmio.padtres) || h < -le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_U);
                else if(!IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_UP)) meg4_clrpad(i, MEG4_BTN_U);
                if(w > le16toh(meg4.mmio.padtres) || h > le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_D);
                else if(!IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) meg4_clrpad(i, MEG4_BTN_D);
            }
        /* drag'n'drop events */
        if(IsFileDropped()) {
            droppedFiles = LoadDroppedFiles();
            for(i = 0; i < (int)droppedFiles.count; i++) {
                if((ptr = main_readfile(!memcmp(droppedFiles.paths[i], "file://", 7) ? (char*)droppedFiles.paths[i] + 7 : (char*)droppedFiles.paths[i], &w))) {
                    fn = strrchr(droppedFiles.paths[i], SEP[0]); if(!fn) fn = (char*)droppedFiles.paths[i]; else fn++;
                    meg4_insert(fn, ptr, w);
                    free(ptr);
                }
            }
            UnloadDroppedFiles(droppedFiles);
        }

        /* run the emulator and display screen */
        meg4_run();
        meg4_redraw(image.data, 640, 400, 640 * 4);
        UpdateTexture(screen, image.data);
        if(le16toh(meg4.mmio.scrx) > 320 || le16toh(meg4.mmio.scry) > 200) {
            src.x = src.y = 0.0f;
        } else {
            src.x = (float)le16toh(meg4.mmio.scrx);
            src.y = (float)le16toh(meg4.mmio.scry);
        }
        src.width = (float)meg4.screen.w; src.height = (float)meg4.screen.h;
        ww = GetRenderWidth(); wh = GetRenderHeight();
        w = ww; h = (int)meg4.screen.h * ww / (int)meg4.screen.w;
        if(h > wh) { h = wh; w = (int)meg4.screen.w * wh / (int)meg4.screen.h; }
        dst.width = w; dst.height = h;
        dst.x = (ww - w) >> 1; dst.y = (wh - h) >> 1;
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(screen, src, dst, origin, 0.0f, WHITE);
        EndDrawing();
    }
    main_quit();
    return 0;
}
