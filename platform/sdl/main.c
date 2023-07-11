/*
 * meg4/platform/sdl/main.c
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
 * @brief SDL "platform" for the MEG-4
 *
 */

#include "meg4.h"
#define SDL_ENABLE_OLD_NAMES
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "../../src/stb_image.h"    /* for stbi_zlib_decompress */
#include "data.h"

#if SDL_VERSION_ATLEAST(3,0,0)
#include <SDL_main.h>
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define SDL_WINDOW_FULLSCREEN_DESKTOP 1
#define cdevice jdevice
#define caxis jaxis
#define cbutton button
#define meg4_showcursor()    SDL_ShowCursor()
#define meg4_hidecursor()    SDL_HideCursor()
SDL_Gamepad  *controller[4] = { 0 };
#else
#define meg4_showcursor()    SDL_ShowCursor(SDL_ENABLE)
#define meg4_hidecursor()    SDL_ShowCursor(SDL_DISABLE)
SDL_GameController *controller[4] = { 0 };
#endif
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *screen = NULL;
SDL_Event event;
SDL_AudioSpec have;
int controllerid[4] = { -1, -1, -1, -1 };

int main_draw = 1, main_ret, main_w = 0, main_h = 0, win_w, win_h, win_f = 0, audio = 0, main_alt = 0;
int main_keymap[SDL_NUM_SCANCODES];
void main_delay(int msec);

#include "../common.h"

/**
 * Exit emulator
 */
void main_quit(void)
{
    main_log(1, "quitting...         ");
    meg4_poweroff();
    meg4_showcursor();
    if(screen) { SDL_DestroyTexture(screen); screen = NULL; }
/* this crashes sometimes... but only sometimes... We'll exit so should be freed anyway */
/*    if(renderer) { SDL_DestroyRenderer(renderer); renderer = NULL; }*/
    if(window) {
        SDL_DestroyWindow(window);
#ifndef __EMSCRIPTEN__
        /* restore original screen resolution */
        if(win_f && (main_w != win_w || main_h != win_h)) {
#if SDL_VERSION_ATLEAST(3,0,0)
            window = SDL_CreateWindow("MEG-4", main_w, main_h, 1);
#else
            window = SDL_CreateWindow("MEG-4", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, main_w, main_h,
                SDL_WINDOW_FULLSCREEN);
#endif
            if(window) SDL_DestroyWindow(window);
        }
#endif
        window = NULL;
    }
    if(audio) {
#if SDL_VERSION_ATLEAST(3,0,0)
        SDL_PauseAudioDevice(audio);
#else
        SDL_PauseAudioDevice(audio, 1);
#endif
        SDL_CloseAudioDevice(audio); audio = 0; }
    SDL_Quit();
#ifdef __EMSCRIPTEN__
    /* don't let emscripten fool you, this won't cancel the loop. it will quit... but neither of these work with asyncify! */
    emscripten_cancel_main_loop();
    /*emscripten_force_exit(0);*/
#else
    /* DO NOT call exit(), that crashes android... */
    /* exit(0); */
#endif
}

/**
 * Create window
 */
void main_win(int w, int h, int f)
{
    int p;
    void *data;
    SDL_Surface *srf;

    if(screen) { SDL_DestroyTexture(screen); screen = NULL; }
    if(renderer) { SDL_DestroyRenderer(renderer); renderer = NULL; }
    if(window) { SDL_DestroyWindow(window); window = NULL; }

    if(!f) { win_w = w; win_h = h; }
    win_f = f;
#if SDL_VERSION_ATLEAST(3,0,0)
    window = SDL_CreateWindow("MEG-4", f ? main_w : w, f ? main_h : h, f);
#else
    window = SDL_CreateWindow("MEG-4",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        f ? main_w : w, f ? main_h : h,
        f ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);
#endif
    if(!window) return;
#if SDL_VERSION_ATLEAST(3,0,0)
    renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    if(!renderer) {
        renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_SOFTWARE);
        if(!renderer) return;
    }
#else
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if(!renderer) return;
    }
#endif
    if(meg4_icons.buf) {
#if SDL_VERSION_ATLEAST(3,0,0)
        srf = SDL_CreateSurfaceFrom((Uint32 *)meg4_icons.buf, meg4_icons.w, meg4_icons.h, meg4_icons.w * 4,
            SDL_PIXELFORMAT_RGBA8888);
#else
        srf = SDL_CreateRGBSurfaceFrom((Uint32 *)meg4_icons.buf, meg4_icons.w, 64, 32, meg4_icons.w * 4,
            0xFF, 0xFF00, 0xFF0000, 0xFF000000);
#endif
        if(srf) {
            SDL_SetWindowIcon(window, srf);
#if SDL_VERSION_ATLEAST(3,0,0)
            SDL_DestroySurface(srf);
#else
            SDL_FreeSurface(srf);
#endif
        }
    }
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 640, 400);
    if(screen) {
#if SDL_VERSION_ATLEAST(2,0,12)
        SDL_SetTextureScaleMode(screen,
#if SDL_VERSION_ATLEAST(3,0,0)
            SDL_SCALEMODE_NEAREST
#else
            SDL_ScaleModeNearest
#endif
        );
#endif
        SDL_LockTexture(screen, NULL, &data, &p);
        memset(data, 0, p * 400);
        SDL_UnlockTexture(screen);
    }
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    win_f ^= 1;
    SDL_SetWindowFullscreen(window, win_f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#if SDL_VERSION_ATLEAST(2,0,12)
    SDL_SetTextureScaleMode(screen, nearest || (!((win_f ? main_w : win_w) % 320) && !((win_f ? main_h : win_h) % 200)) ?
#if SDL_VERSION_ATLEAST(3,0,0)
        SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR
#else
        SDL_ScaleModeNearest : SDL_ScaleModeLinear
#endif
    );
#endif
}

/**
 * Make window focused
 */
void main_focus(void)
{
    SDL_RaiseWindow(window);
    if(win_f) SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

/**
 * Calculate pointer coordinates
 */
void main_pointer(SDL_Rect *dst, int x, int y)
{
    meg4_setptr(x < dst->x || !dst->w ? 0 : (x >= dst->x + dst->w ? meg4.screen.w : (x - dst->x) * meg4.screen.w / dst->w),
        y < dst->y || !dst->h ? 0 : (y >= dst->y + dst->h ? meg4.screen.h : (y - dst->y) * meg4.screen.h / dst->h));
}

/**
 * Main SDL emulator loop
 */
/* emscripten does not allow return value... */
void main_loop(void) {
#ifdef __EMSCRIPTEN__
#define exit_loop() main_quit()
#else
#define exit_loop() do{ main_ret = 0; return; }while(0)
#endif
    int i, p;
    void *data;
#ifndef NOEDITORS
    char *fn;
#endif
#if SDL_VERSION_ATLEAST(3,0,0)
    SDL_FRect src, dst;
    SDL_Rect idst;
    float mx, my;
#else
    SDL_Rect src, dst;
#endif

    meg4_run();
    data = NULL; p = 0;
    if(main_draw) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        if(screen) SDL_LockTexture(screen, NULL, &data, &p);
    }
    meg4_redraw((uint32_t*)data, 640, 400, p);
    if(data) SDL_UnlockTexture(screen);
    src.x = src.y = 0; src.w = meg4.screen.w; src.h = meg4.screen.h;
    if(src.w && src.h) {
        if(nearest) {
            for(i = 1; i < 16 && (i + 1) * 320 <= win_w && (i + 1) * 200 <= win_h; i++);
            dst.w = 320 * i; dst.h = 200 * i;
        } else {
            dst.w = win_w; dst.h = src.h * win_w / src.w;
            if(dst.h > win_h) { dst.h = win_h; dst.w = src.w * win_h / src.h; }
        }
    } else dst.w = dst.h = 0;
    dst.x = (win_w - dst.w) / 2; dst.y = (win_h - dst.h) / 2;
    if(main_draw) {
        if(screen) SDL_RenderCopy(renderer, screen, &src, &dst);
        SDL_RenderPresent(renderer);
    }
#if SDL_VERSION_ATLEAST(3,0,0)
    SDL_GetMouseState(&mx, &my);
    idst.x = dst.x; idst.y = dst.y; idst.w = dst.w; idst.h = dst.h;
    main_pointer(&idst, (int)mx, (int)my);
#else
    SDL_GetMouseState(&i, &p);
    main_pointer(&dst, i, p);
#endif
    event.type = 0; main_ret = 1;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
#if SDL_VERSION_ATLEAST(3,0,0)
            case SDL_EVENT_QUIT: exit_loop(); break;
            case SDL_EVENT_WINDOW_RESIZED: {
#else
            case SDL_QUIT: exit_loop(); break;
            case SDL_WINDOWEVENT:
                switch(event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE: exit_loop(); break;
                    case SDL_WINDOWEVENT_RESIZED: case SDL_WINDOWEVENT_SIZE_CHANGED:
#endif
                        win_w = event.window.data1; win_h = event.window.data2;
                        i = win_w / 320;
#if SDL_VERSION_ATLEAST(2,0,12)
                        SDL_SetTextureScaleMode(screen, nearest || (!(win_w % 320) && !(win_h % 200)) ?
#if SDL_VERSION_ATLEAST(3,0,0)
                                SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR
#else
                                SDL_ScaleModeNearest : SDL_ScaleModeLinear
#endif
                        );
#endif
                    break;
                }
            break;
            case SDL_MOUSEBUTTONDOWN:
                switch(event.button.button) {
                    case SDL_BUTTON_LEFT:   meg4_setbtn(MEG4_BTN_L); break;
                    case SDL_BUTTON_MIDDLE: meg4_setbtn(MEG4_BTN_M); break;
                    case SDL_BUTTON_RIGHT:  meg4_setbtn(MEG4_BTN_R); break;
                }
            break;
            case SDL_MOUSEBUTTONUP:
                switch(event.button.button) {
                    case SDL_BUTTON_LEFT:   meg4_clrbtn(MEG4_BTN_L); break;
                    case SDL_BUTTON_MIDDLE: meg4_clrbtn(MEG4_BTN_M); break;
                    case SDL_BUTTON_RIGHT:  meg4_clrbtn(MEG4_BTN_R); break;
                }
            break;
            case SDL_MOUSEWHEEL:
                meg4_setscr(event.wheel.y > 0, event.wheel.y < 0, event.wheel.x < 0, event.wheel.x > 0);
            break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_LALT: case SDLK_LCTRL: main_alt = 1; break;
                    case SDLK_RETURN:
                        if(main_alt) {
                            main_fullscreen();
                            return;
                        }
                        meg4_pushkey("\n\0\0");
                    break;
                    case SDLK_q: if(main_alt) { exit_loop(); } break;
                    case SDLK_ESCAPE: if(main_alt) { exit_loop(); } else meg4_pushkey("\x1b\0\0"); break;
                    /* only for special keys that aren't handled by SDL_TEXTINPUT events */
                    case SDLK_F1: meg4_pushkey("F1\0"); break;
                    case SDLK_F2: meg4_pushkey("F2\0"); break;
                    case SDLK_F3: meg4_pushkey("F3\0"); break;
                    case SDLK_F4: meg4_pushkey("F4\0"); break;
                    case SDLK_F5: meg4_pushkey("F5\0"); break;
                    case SDLK_F6: meg4_pushkey("F6\0"); break;
                    case SDLK_F7: meg4_pushkey("F7\0"); break;
                    case SDLK_F8: meg4_pushkey("F8\0"); break;
                    case SDLK_F9: meg4_pushkey("F9\0"); break;
                    case SDLK_F10: meg4_pushkey("F10"); break;
                    case SDLK_F11: meg4_pushkey("F11"); break;
                    case SDLK_F12: meg4_pushkey("F12"); break;
                    case SDLK_PRINTSCREEN: meg4_pushkey("PSc"); break;
                    case SDLK_SCROLLLOCK: meg4_pushkey("SLk"); break;
                    case SDLK_NUMLOCKCLEAR: meg4_pushkey("NLk"); break;
                    case SDLK_BACKSPACE: meg4_pushkey("\b\0\0"); break;
                    case SDLK_TAB: meg4_pushkey("\t\0\0"); break;
                    case SDLK_CAPSLOCK: meg4_pushkey("CLk"); break;
                    case SDLK_UP: meg4_pushkey("Up\0"); break;
                    case SDLK_DOWN: meg4_pushkey("Down"); break;
                    case SDLK_LEFT: meg4_pushkey("Left"); break;
                    case SDLK_RIGHT: meg4_pushkey("Rght"); break;
                    case SDLK_HOME: meg4_pushkey("Home"); break;
                    case SDLK_END: meg4_pushkey("End"); break;
                    case SDLK_PAGEUP: meg4_pushkey("PgUp"); break;
                    case SDLK_PAGEDOWN: meg4_pushkey("PgDn"); break;
                    case SDLK_INSERT: meg4_pushkey("Ins"); break;
                    case SDLK_DELETE: meg4_pushkey("Del"); break;
                }
                if(event.key.keysym.scancode < SDL_NUM_SCANCODES) meg4_setkey(main_keymap[event.key.keysym.scancode]);
            break;
            case SDL_KEYUP:
                switch(event.key.keysym.sym) {
                    case SDLK_LALT: case SDLK_LCTRL: main_alt = 0; break;
                }
                if(event.key.keysym.scancode < SDL_NUM_SCANCODES) meg4_clrkey(main_keymap[event.key.keysym.scancode]);
            break;
            case SDL_TEXTINPUT:
                if(!main_alt && (uint8_t)event.text.text[0] >= 32)
                    meg4_pushkey((char*)&event.text.text);
            break;
            case SDL_CONTROLLERDEVICEADDED:
                for(i = 0; i < 4 && controllerid[i] != (int)event.cdevice.which; i++);
                if(i >= 4) for(i = 0; i < 4 && controllerid[i] != -1; i++);
                if(i < 4) {
                    if(controller[i]) SDL_GameControllerClose(controller[i]);
                    controller[i] = SDL_GameControllerOpen(event.cdevice.which);
                    controllerid[i] = event.cdevice.which;
                }
            break;
            case SDL_CONTROLLERDEVICEREMOVED:
                for(i = 0; i < 4 && controllerid[i] != (int)event.cdevice.which; i++);
                if(i < 4) {
                    if(controller[i]) SDL_GameControllerClose(controller[i]);
                    controller[i] = NULL;
                    controllerid[i] = -1;
                }
            break;
            case SDL_CONTROLLERBUTTONDOWN:
                for(i = 0; i < 4 && controllerid[i] != (int)event.cbutton.which; i++);
                if(i < 4) {
                    switch(event.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: meg4_setpad(i, MEG4_BTN_L); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_UP: meg4_setpad(i, MEG4_BTN_U); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: meg4_setpad(i, MEG4_BTN_R); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: meg4_setpad(i, MEG4_BTN_D); break;
                        case SDL_CONTROLLER_BUTTON_A: meg4_setpad(i, MEG4_BTN_A); break;
                        case SDL_CONTROLLER_BUTTON_B: meg4_setpad(i, MEG4_BTN_B); break;
                        case SDL_CONTROLLER_BUTTON_X: meg4_setpad(i, MEG4_BTN_X); break;
                        case SDL_CONTROLLER_BUTTON_Y: meg4_setpad(i, MEG4_BTN_Y); break;
                    }
                }
            break;
            case SDL_CONTROLLERBUTTONUP:
                for(i = 0; i < 4 && controllerid[i] != (int)event.cbutton.which; i++);
                if(i < 4) {
                    switch(event.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: meg4_clrpad(i, MEG4_BTN_L); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_UP: meg4_clrpad(i, MEG4_BTN_U); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: meg4_clrpad(i, MEG4_BTN_R); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: meg4_clrpad(i, MEG4_BTN_D); break;
                        case SDL_CONTROLLER_BUTTON_A: meg4_clrpad(i, MEG4_BTN_A); break;
                        case SDL_CONTROLLER_BUTTON_B: meg4_clrpad(i, MEG4_BTN_B); break;
                        case SDL_CONTROLLER_BUTTON_X: meg4_clrpad(i, MEG4_BTN_X); break;
                        case SDL_CONTROLLER_BUTTON_Y: meg4_clrpad(i, MEG4_BTN_Y); break;
                    }
                }
            break;
            case SDL_CONTROLLERAXISMOTION:
                for(i = 0; i < 4 && controllerid[i] != (int)event.caxis.which; i++);
                if(i < 4) {
                    if(event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY || event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY) {
                        meg4_clrpad(i, MEG4_BTN_L | MEG4_BTN_R);
                        if(event.caxis.value < -le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_L);
                        if(event.caxis.value >  le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_R);
                    }
                    if(event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX) {
                        meg4_clrpad(i, MEG4_BTN_U | MEG4_BTN_D);
                        if(event.caxis.value < -le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_U);
                        if(event.caxis.value >  le16toh(meg4.mmio.padtres)) meg4_setpad(i, MEG4_BTN_D);
                    }
                }
            break;
            /* normally finger events are automatically converted to mouse events, but in case SDL is configured not to do so */
#if FINGEREVENTS
            case SDL_FINGERUP: case SDL_FINGERDOWN: case SDL_FINGERMOTION:
                SDL_GetWindowPosition(window, &i, &p);
                main_pointer(&dst, event.tfinger.x * main_w - i, event.tfinger.y * main_h - p);
                switch(event.tfinger.fingerId) {
                    case 0: i = MEG4_BTN_L; break;
                    case 1: i = MEG4_BTN_M; break;
                    case 2: i = MEG4_BTN_R; break;
                    default: i = -1; break;
                }
                if(i != -1) {
                    if(event.type == SDL_FINGERUP)
                        meg4_clrbtn(i);
                    else
                        meg4_setbtn(i);
                }
            break;
#endif
#ifndef NOEDITORS
            case SDL_DROPFILE:
                if(event.drop.file) {
                    if((data = main_readfile(!memcmp(event.drop.file, "file://", 7) ? event.drop.file + 7 : event.drop.file, &i))) {
                        fn = strrchr(event.drop.file, SEP[0]); if(!fn) fn = event.drop.file; else fn++;
                        meg4_insert(fn, (uint8_t*)data, i);
                        free(data);
                    }
                    SDL_free(event.drop.file);
                }
            break;
#endif
        }
    }
}

/**
 * Workaround a stupid iOS and Android bug
 */
int main_stupidios(void *data, SDL_Event *event)
{
    (void)data;
    switch(event->type) {
        case SDL_APP_WILLENTERBACKGROUND: main_draw = 0; break;
        case SDL_APP_WILLENTERFOREGROUND: main_draw = 1; break;
    }
    return 1;
}

/**
 * Get text from clipboard (must be freed by caller)
 */
char *main_getclipboard(void)
{
    return SDL_GetClipboardText();
}

/**
 * Set text to clipboard
 */
void main_setclipboard(char *str)
{
    SDL_SetClipboardText((const char*)str);
}

/**
 * Show on-screen keyboard
 */
void main_osk_show(void)
{
#if defined(__ANDROID__) || defined(__IOS__)
    SDL_StartTextInput();
#endif
}

/**
 * Hide on-screen keyboard
 */
void main_osk_hide(void)
{
#if defined(__ANDROID__) || defined(__IOS__)
    SDL_StopTextInput();
#endif
}

/**
 * SDL audio callback
 */
void main_audio(void *ctx, Uint8 *buf, int len)
{
    (void)ctx;
    meg4_audiofeed((float*)buf, len >> 2);
}

/**
 * Delay
 */
void main_delay(int msec)
{
    SDL_Delay(msec);
}

/**
 * Print program version and copyright
 */
void main_hdr(void)
{
    printf("\r\nMEG-4 v%s (SDL%d, build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, SDL_MAJOR_VERSION, BUILD);
}

/**
 * The real main procedure
 */
int main(int argc, char **argv)
{
    int i, j, w;
    char **infile = NULL, *ptr2;
    uint8_t *ptr;
    SDL_RWops *ops = NULL;
    SDL_AudioSpec want;
    SDL_version ver;
#ifdef __EMSCRIPTEN__
    char detlng[3] = { 0 }, *lng = detlng;

    (void)argc; (void)argv;
    i = EM_ASM_INT({
        var ln=document.location.href.split('?')[1];if(ln==undefined)ln=navigator.language.substr(0,2);
        return ln.charCodeAt(1) * 256 + ln.charCodeAt(0);
    });
    detlng[0] = i & 0xff; detlng[1] = (i >> 8) & 0xff; detlng[2] = 0;
#else
    char *fn;
    int32_t tickdiff;
    uint32_t ticks;
#if SDL_VERSION_ATLEAST(3,0,0)
    const SDL_DisplayMode *dm;
#else
    SDL_DisplayMode dm;
#endif
#ifdef __WIN32__
    SDL_SysWMinfo wmInfo = { 0 };
    char *lng = main_lng;
#else
    char *lng = getenv("LANG");
#endif
    main_parsecommandline(argc, argv, &lng, &infile);
#endif
    main_hdr();
    for(i = 0; i < 3; i++) printf("  %s\r\n", copyright[i]);
    printf("\r\n");
    fflush(stdout);

    SDL_GetVersion(&ver);
    sprintf(meg4plat, "SDL %u.%u.%u", ver.major, ver.minor, ver.patch);

    /* set up keymap */
    memset(main_keymap, 0, sizeof(main_keymap));
    main_keymap[SDL_SCANCODE_A] = MEG4_KEY_A;
    main_keymap[SDL_SCANCODE_B] = MEG4_KEY_B;
    main_keymap[SDL_SCANCODE_C] = MEG4_KEY_C;
    main_keymap[SDL_SCANCODE_D] = MEG4_KEY_D;
    main_keymap[SDL_SCANCODE_E] = MEG4_KEY_E;
    main_keymap[SDL_SCANCODE_F] = MEG4_KEY_F;
    main_keymap[SDL_SCANCODE_G] = MEG4_KEY_G;
    main_keymap[SDL_SCANCODE_H] = MEG4_KEY_H;
    main_keymap[SDL_SCANCODE_I] = MEG4_KEY_I;
    main_keymap[SDL_SCANCODE_J] = MEG4_KEY_J;
    main_keymap[SDL_SCANCODE_K] = MEG4_KEY_K;
    main_keymap[SDL_SCANCODE_L] = MEG4_KEY_L;
    main_keymap[SDL_SCANCODE_M] = MEG4_KEY_M;
    main_keymap[SDL_SCANCODE_N] = MEG4_KEY_N;
    main_keymap[SDL_SCANCODE_O] = MEG4_KEY_O;
    main_keymap[SDL_SCANCODE_P] = MEG4_KEY_P;
    main_keymap[SDL_SCANCODE_Q] = MEG4_KEY_Q;
    main_keymap[SDL_SCANCODE_R] = MEG4_KEY_R;
    main_keymap[SDL_SCANCODE_S] = MEG4_KEY_S;
    main_keymap[SDL_SCANCODE_T] = MEG4_KEY_T;
    main_keymap[SDL_SCANCODE_U] = MEG4_KEY_U;
    main_keymap[SDL_SCANCODE_V] = MEG4_KEY_V;
    main_keymap[SDL_SCANCODE_W] = MEG4_KEY_W;
    main_keymap[SDL_SCANCODE_X] = MEG4_KEY_X;
    main_keymap[SDL_SCANCODE_Y] = MEG4_KEY_Y;
    main_keymap[SDL_SCANCODE_Z] = MEG4_KEY_Z;
    main_keymap[SDL_SCANCODE_1] = MEG4_KEY_1;
    main_keymap[SDL_SCANCODE_2] = MEG4_KEY_2;
    main_keymap[SDL_SCANCODE_3] = MEG4_KEY_3;
    main_keymap[SDL_SCANCODE_4] = MEG4_KEY_4;
    main_keymap[SDL_SCANCODE_5] = MEG4_KEY_5;
    main_keymap[SDL_SCANCODE_6] = MEG4_KEY_6;
    main_keymap[SDL_SCANCODE_7] = MEG4_KEY_7;
    main_keymap[SDL_SCANCODE_8] = MEG4_KEY_8;
    main_keymap[SDL_SCANCODE_9] = MEG4_KEY_9;
    main_keymap[SDL_SCANCODE_0] = MEG4_KEY_0;
    main_keymap[SDL_SCANCODE_RETURN] = MEG4_KEY_ENTER;
    main_keymap[SDL_SCANCODE_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[SDL_SCANCODE_TAB] = MEG4_KEY_TAB;
    main_keymap[SDL_SCANCODE_SPACE] = MEG4_KEY_SPACE;
    main_keymap[SDL_SCANCODE_MINUS] = MEG4_KEY_MINUS;
    main_keymap[SDL_SCANCODE_EQUALS] = MEG4_KEY_EQUAL;
    main_keymap[SDL_SCANCODE_LEFTBRACKET] = MEG4_KEY_LBRACKET;
    main_keymap[SDL_SCANCODE_RIGHTBRACKET] = MEG4_KEY_RBRACKET;
    main_keymap[SDL_SCANCODE_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[SDL_SCANCODE_NONUSHASH] = MEG4_KEY_BACKSLASH;
    main_keymap[SDL_SCANCODE_SEMICOLON] = MEG4_KEY_SEMICOLON;
    main_keymap[SDL_SCANCODE_APOSTROPHE] = MEG4_KEY_APOSTROPHE;
    main_keymap[SDL_SCANCODE_GRAVE] = MEG4_KEY_BACKQUOTE;
    main_keymap[SDL_SCANCODE_COMMA] = MEG4_KEY_COMMA;
    main_keymap[SDL_SCANCODE_PERIOD] = MEG4_KEY_PERIOD;
    main_keymap[SDL_SCANCODE_SLASH] = MEG4_KEY_SLASH;
    main_keymap[SDL_SCANCODE_CAPSLOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[SDL_SCANCODE_F1] = MEG4_KEY_F1;
    main_keymap[SDL_SCANCODE_F2] = MEG4_KEY_F2;
    main_keymap[SDL_SCANCODE_F3] = MEG4_KEY_F3;
    main_keymap[SDL_SCANCODE_F4] = MEG4_KEY_F4;
    main_keymap[SDL_SCANCODE_F5] = MEG4_KEY_F5;
    main_keymap[SDL_SCANCODE_F6] = MEG4_KEY_F6;
    main_keymap[SDL_SCANCODE_F7] = MEG4_KEY_F7;
    main_keymap[SDL_SCANCODE_F8] = MEG4_KEY_F8;
    main_keymap[SDL_SCANCODE_F9] = MEG4_KEY_F9;
    main_keymap[SDL_SCANCODE_F10] = MEG4_KEY_F10;
    main_keymap[SDL_SCANCODE_F11] = MEG4_KEY_F11;
    main_keymap[SDL_SCANCODE_F12] = MEG4_KEY_F12;
    main_keymap[SDL_SCANCODE_PRINTSCREEN] = MEG4_KEY_PRSCR;
    main_keymap[SDL_SCANCODE_SCROLLLOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[SDL_SCANCODE_PAUSE] = MEG4_KEY_PAUSE;
    main_keymap[SDL_SCANCODE_INSERT] = MEG4_KEY_INS;
    main_keymap[SDL_SCANCODE_HOME] = MEG4_KEY_HOME;
    main_keymap[SDL_SCANCODE_PAGEUP] = MEG4_KEY_PGUP;
    main_keymap[SDL_SCANCODE_DELETE] = MEG4_KEY_DEL;
    main_keymap[SDL_SCANCODE_END] = MEG4_KEY_END;
    main_keymap[SDL_SCANCODE_PAGEDOWN] = MEG4_KEY_PGDN;
    main_keymap[SDL_SCANCODE_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[SDL_SCANCODE_LEFT] = MEG4_KEY_LEFT;
    main_keymap[SDL_SCANCODE_DOWN] = MEG4_KEY_DOWN;
    main_keymap[SDL_SCANCODE_UP] = MEG4_KEY_UP;
    main_keymap[SDL_SCANCODE_NUMLOCKCLEAR] = MEG4_KEY_NUMLOCK;
    main_keymap[SDL_SCANCODE_KP_DIVIDE] = MEG4_KEY_KP_DIV;
    main_keymap[SDL_SCANCODE_KP_MULTIPLY] = MEG4_KEY_KP_MUL;
    main_keymap[SDL_SCANCODE_KP_MINUS] = MEG4_KEY_KP_SUB;
    main_keymap[SDL_SCANCODE_KP_PLUS] = MEG4_KEY_KP_ADD;
    main_keymap[SDL_SCANCODE_KP_ENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[SDL_SCANCODE_KP_1] = MEG4_KEY_KP_1;
    main_keymap[SDL_SCANCODE_KP_2] = MEG4_KEY_KP_2;
    main_keymap[SDL_SCANCODE_KP_3] = MEG4_KEY_KP_3;
    main_keymap[SDL_SCANCODE_KP_4] = MEG4_KEY_KP_4;
    main_keymap[SDL_SCANCODE_KP_5] = MEG4_KEY_KP_5;
    main_keymap[SDL_SCANCODE_KP_6] = MEG4_KEY_KP_6;
    main_keymap[SDL_SCANCODE_KP_7] = MEG4_KEY_KP_7;
    main_keymap[SDL_SCANCODE_KP_8] = MEG4_KEY_KP_8;
    main_keymap[SDL_SCANCODE_KP_9] = MEG4_KEY_KP_9;
    main_keymap[SDL_SCANCODE_KP_0] = MEG4_KEY_KP_0;
    main_keymap[SDL_SCANCODE_KP_PERIOD] = MEG4_KEY_KP_DEC;
    main_keymap[SDL_SCANCODE_DECIMALSEPARATOR] = MEG4_KEY_KP_DEC;
    main_keymap[SDL_SCANCODE_NONUSBACKSLASH] = MEG4_KEY_LESS;
    main_keymap[SDL_SCANCODE_APPLICATION] = MEG4_KEY_APP;
    main_keymap[SDL_SCANCODE_POWER] = MEG4_KEY_POWER;
    main_keymap[SDL_SCANCODE_KP_EQUALS] = MEG4_KEY_EQUAL;
    main_keymap[SDL_SCANCODE_EXECUTE] = MEG4_KEY_EXEC;
    main_keymap[SDL_SCANCODE_HELP] = MEG4_KEY_HELP;
    main_keymap[SDL_SCANCODE_MENU] = MEG4_KEY_MENU;
    main_keymap[SDL_SCANCODE_SELECT] = MEG4_KEY_SELECT;
    main_keymap[SDL_SCANCODE_STOP] = MEG4_KEY_STOP;
    main_keymap[SDL_SCANCODE_AGAIN] = MEG4_KEY_AGAIN;
    main_keymap[SDL_SCANCODE_UNDO] = MEG4_KEY_UNDO;
    main_keymap[SDL_SCANCODE_CUT] = MEG4_KEY_CUT;
    main_keymap[SDL_SCANCODE_COPY] = MEG4_KEY_COPY;
    main_keymap[SDL_SCANCODE_PASTE] = MEG4_KEY_PASTE;
    main_keymap[SDL_SCANCODE_FIND] = MEG4_KEY_FIND;
    main_keymap[SDL_SCANCODE_MUTE] = MEG4_KEY_MUTE;
    main_keymap[SDL_SCANCODE_VOLUMEUP] = MEG4_KEY_VOLUP;
    main_keymap[SDL_SCANCODE_VOLUMEDOWN] = MEG4_KEY_VOLDN;
    main_keymap[SDL_SCANCODE_INTERNATIONAL1] = MEG4_KEY_INT1;
    main_keymap[SDL_SCANCODE_INTERNATIONAL2] = MEG4_KEY_INT2;
    main_keymap[SDL_SCANCODE_INTERNATIONAL3] = MEG4_KEY_INT3;
    main_keymap[SDL_SCANCODE_INTERNATIONAL4] = MEG4_KEY_INT4;
    main_keymap[SDL_SCANCODE_INTERNATIONAL5] = MEG4_KEY_INT5;
    main_keymap[SDL_SCANCODE_INTERNATIONAL6] = MEG4_KEY_INT6;
    main_keymap[SDL_SCANCODE_INTERNATIONAL7] = MEG4_KEY_INT7;
    main_keymap[SDL_SCANCODE_INTERNATIONAL8] = MEG4_KEY_INT8;
    main_keymap[SDL_SCANCODE_LANG1] = MEG4_KEY_LNG1;
    main_keymap[SDL_SCANCODE_LANG2] = MEG4_KEY_LNG2;
    main_keymap[SDL_SCANCODE_LANG3] = MEG4_KEY_LNG3;
    main_keymap[SDL_SCANCODE_LANG4] = MEG4_KEY_LNG4;
    main_keymap[SDL_SCANCODE_LANG5] = MEG4_KEY_LNG5;
    main_keymap[SDL_SCANCODE_LANG6] = MEG4_KEY_LNG6;
    main_keymap[SDL_SCANCODE_LANG7] = MEG4_KEY_LNG7;
    main_keymap[SDL_SCANCODE_LANG8] = MEG4_KEY_LNG8;
    main_keymap[SDL_SCANCODE_LCTRL] = MEG4_KEY_LCTRL;
    main_keymap[SDL_SCANCODE_LSHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[SDL_SCANCODE_LALT] = MEG4_KEY_LALT;
    main_keymap[SDL_SCANCODE_LGUI] = MEG4_KEY_LSUPER;
    main_keymap[SDL_SCANCODE_RCTRL] = MEG4_KEY_RCTRL;
    main_keymap[SDL_SCANCODE_RSHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[SDL_SCANCODE_RALT] = MEG4_KEY_RALT;
    main_keymap[SDL_SCANCODE_RGUI] = MEG4_KEY_RSUPER;
    /* initialize screen and other SDL stuff */
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER)) {
        main_log(0, "unable to initialize SDL");
        return 1;
    }
    /* initialize audio */
    memset(&want, 0, sizeof(want));
    memset(&have, 0, sizeof(have));
    want.freq = 44100;
#if SDL_VERSION_ATLEAST(3,0,0)
    want.format = SDL_AUDIO_F32;
#else
    want.format = AUDIO_F32;
#endif
    want.channels = 1;
    want.samples = 4096;
    want.callback = main_audio;
    audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if(audio && (have.freq != 44100 || have.channels != 1 || have.format !=
#if SDL_VERSION_ATLEAST(3,0,0)
        SDL_AUDIO_F32
#else
        AUDIO_F32
#endif
      )) {
        SDL_CloseAudioDevice(audio); audio = 0;
    }
    if(verbose && audio) main_log(1, "audio opened %uHz, %u bits", have.freq, 32);
    /* turn on the emulator */
    meg4_poweron(lng);
#if !defined(NOEDITORS) && !defined(__EMSCRIPTEN__)
    for(; infile && *infile; infile++) {
        if((ptr = main_readfile(*infile, &i))) {
            fn = strrchr(*infile, SEP[0]); if(!fn) fn = *infile; else fn++;
            meg4_insert(fn, ptr, i);
            free(ptr);
        }
    }
#else
    (void)infile;
#endif

#ifdef __EMSCRIPTEN__
    win_w = main_w = EM_ASM_INT({ return Module.canvas.width; });
    win_h = main_h = EM_ASM_INT({ return Module.canvas.height; });
    main_win(main_w, main_h, 0);
#else
#if SDL_VERSION_ATLEAST(3,0,0)
    dm = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    main_w = dm->w; main_h = dm->h;
#else
    SDL_GetDesktopDisplayMode(0, &dm);
    /*dm.w = 640; dm.h = 400;*/
    main_w = dm.w; main_h = dm.h;
#endif
#if DEBUG
    main_win(640, 400, 0);
#else
    for(win_w = win_h = 0; win_w + 320 < main_w && win_h + 200 < main_h; win_w += 320, win_h += 200);
    main_win(win_w/*main_w*/, win_h/*main_h*/, 0/*1*/);
    main_fullscreen();
#endif
#endif
    if(!window || !renderer) {
        meg4_poweroff();
        SDL_Quit();
        main_log(0, "unable to get SDL window");
        return 1;
    }
#ifdef __WIN32__
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    hwnd = wmInfo.info.win.window;
#endif
    /* uncompress built-in gamecontrollerdb */
    ptr = (uint8_t*)binary_gamecontrollerdb + 3;
    i = *ptr++; ptr += 6; if(i & 4) { w = *ptr++; w += (*ptr++ << 8); ptr += w; } if(i & 8) { while(*ptr++ != 0); }
    if(i & 16) { while(*ptr++ != 0); } j = sizeof(binary_gamecontrollerdb) - (size_t)(ptr - binary_gamecontrollerdb);
    w = 0; ptr2 = (char*)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)ptr, j, 4096, (int*)&w, 0);
    if(ptr2) {
        ops = SDL_RWFromConstMem(ptr2, w);
        SDL_GameControllerAddMappingsFromRW(ops, 0);
#if SDL_VERSION_ATLEAST(3,0,0)
        SDL_DestroyRW(ops);
#else
        SDL_FreeRW(ops);
#endif
        free(ptr2);
    }
#if SDL_VERSION_ATLEAST(3,0,0)
    SDL_SetGamepadEventsEnabled(SDL_ENABLE);
#else
    SDL_GameControllerEventState(SDL_ENABLE);
#endif
    meg4_hidecursor();
    if(audio) {
#if SDL_VERSION_ATLEAST(3,0,0)
        SDL_PlayAudioDevice(audio);
#else
        SDL_PauseAudioDevice(audio, 0);
#endif
    }
#if !defined(__ANDROID__) && !defined(__IOS__)
    SDL_StartTextInput();
#endif

    /* execute the main emulator loop */
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 60, 1);
    /* this never reached! cancel_main_loop does not cancel the loop, it actually quits the app! */
#else
    SDL_AddEventWatch(main_stupidios, NULL);
    while(1) {
        ticks = SDL_GetTicks();
        /* emscripten does not allow return value... so we have to use a global */
        main_loop();
        if(!main_ret) break;
        tickdiff = (1000/60) - (SDL_GetTicks() - ticks);
        if(tickdiff > 0 && tickdiff < 1000) {
            if(verbose == 2) { printf("meg4: free time %d msec  \r",tickdiff); fflush(stdout); }
            SDL_Delay(tickdiff);
        }
    }
    main_quit();
#endif
    return 0;
}
