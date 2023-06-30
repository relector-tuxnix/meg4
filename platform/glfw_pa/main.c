/*
 * meg4/platform/glfw_pa/main.c
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
 * @brief GLFW + OpenAL "platform" for the MEG-4
 * NOTE: use only GLES2 so this could be compiled for mobiles too
 * there's also a `NOGLES=1 make clean all` fallback for old-school OpenGL
 *
 */

#define _POSIX_C_SOURCE 199309L    /* needed for timespec and nanosleep() */
#include <time.h>
#include "meg4.h"
#include <GLFW/glfw3.h>
#include <portaudio.h>

GLFWwindow *window = NULL;
GLFWmonitor *monitor = NULL;
GLuint screen;
uint32_t *scrbuf = NULL;
#ifndef NOGLES
#include <GLES2/gl2.h>
GLuint program;
GLint mloc, ploc, tloc, sloc;
#endif
int controllerid[4] = { -1, -1, -1, -1 };
PaStream *pa = NULL;

int main_w = 0, main_h = 0, win_w, win_h, win_f = 0, audio = 0, main_alt = 0, main_keymap[512];
void main_pointer(GLFWwindow *window, double xpos, double ypos);
void main_delay(int msec);

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
#ifndef NOGLES
    if(program) { glDeleteProgram(program); program = 0; }
#endif
    if(scrbuf) { free(scrbuf); scrbuf = NULL; }
    if(screen != -1U) { glDeleteTextures(1, &screen); screen = -1U; }
    if(window) {
        glfwDestroyWindow(window);
        /* restore original screen resolution */
        if(win_f && (main_w != win_w || main_h != win_h)) {
            window = glfwCreateWindow(main_w, main_h, "MEG-4", monitor, NULL);
            if(window) glfwDestroyWindow(window);
        }
        window = NULL;
    }
    glfwTerminate();
    if(audio) { if(pa) { Pa_CloseStream(pa); pa = NULL; } Pa_Terminate(); audio = 0; }
    exit(0);
}

/**
 * Create window
 */
void main_win(int w, int h, int f)
{
    GLFWimage icon;

    if(window) { glfwDestroyWindow(window); window = NULL; }
    if(!f) { win_w = w; win_h = h; }
    win_f = f;
    window = glfwCreateWindow(f ? main_w : w, f ? main_h : h, "MEG-4", f ? monitor : NULL, NULL);
    if(!window) return;
    if(!f) glfwSetWindowPos(window, (main_w - w) / 2, (main_h - h) / 2);
    glfwMakeContextCurrent(window);
    if(meg4_icons.buf) {
        icon.width = meg4_icons.w; icon.height = 64; icon.pixels = (uint8_t*)meg4_icons.buf;
        glfwSetWindowIcon(window, 1, &icon);
    }
    glfwShowWindow(window);
}

/**
 * Toggle fullscreen
 */
void main_fullscreen(void)
{
    win_f ^= 1;
    if(!win_f)
        glfwSetWindowMonitor(window, NULL, (main_w - win_w) / 2, (main_h - win_h) / 2, win_w, win_h, 0);
    else
        glfwSetWindowMonitor(window, monitor, 0, 0, main_w, main_h, 0);
}

/**
 * Make window focused
 */
void main_focus(void)
{
    glfwFocusWindow(window);
    if(win_f) glfwSetWindowMonitor(window, monitor, 0, 0, main_w, main_h, 0);
}

/**
 * The glfw error callback
 */
void main_error(int error, const char *msg)
{
    main_log(0, "glfw error %d: %s", error, msg);
    main_quit();
}

/**
 * Process a keyboard event callback
 */
void main_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void)window; (void)mods;
    if(key == GLFW_KEY_UNKNOWN && scancode >= 119 && scancode <= 151) key = scancode;
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_LEFT_ALT) main_alt = 1;
        if(main_alt)
            switch(key) {
                case GLFW_KEY_ENTER: main_fullscreen(); return;
                case GLFW_KEY_Q: case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); return;
        }
        /* only for special keys that aren't handled by main_char() */
        switch(key) {
            case GLFW_KEY_ESCAPE: meg4_pushkey("\x1b\0\0"); break;
            case GLFW_KEY_F1: meg4_pushkey("F1\0"); break;
            case GLFW_KEY_F2: meg4_pushkey("F2\0"); break;
            case GLFW_KEY_F3: meg4_pushkey("F3\0"); break;
            case GLFW_KEY_F4: meg4_pushkey("F4\0"); break;
            case GLFW_KEY_F5: meg4_pushkey("F5\0"); break;
            case GLFW_KEY_F6: meg4_pushkey("F6\0"); break;
            case GLFW_KEY_F7: meg4_pushkey("F7\0"); break;
            case GLFW_KEY_F8: meg4_pushkey("F8\0"); break;
            case GLFW_KEY_F9: meg4_pushkey("F9\0"); break;
            case GLFW_KEY_F10: meg4_pushkey("F10"); break;
            case GLFW_KEY_F11: meg4_pushkey("F11"); break;
            case GLFW_KEY_F12: meg4_pushkey("F12"); break;
            case GLFW_KEY_PRINT_SCREEN: meg4_pushkey("PSc"); break;
            case GLFW_KEY_SCROLL_LOCK: meg4_pushkey("SLk"); break;
            case GLFW_KEY_NUM_LOCK: meg4_pushkey("NLk"); break;
            case GLFW_KEY_BACKSPACE: meg4_pushkey("\b\0\0"); break;
            case GLFW_KEY_TAB: meg4_pushkey("\t\0\0"); break;
            case GLFW_KEY_ENTER: meg4_pushkey("\n\0\0"); break;
            case GLFW_KEY_CAPS_LOCK: meg4_pushkey("CLk"); break;
            case GLFW_KEY_UP: meg4_pushkey("Up\0"); break;
            case GLFW_KEY_DOWN: meg4_pushkey("Down"); break;
            case GLFW_KEY_LEFT: meg4_pushkey("Left"); break;
            case GLFW_KEY_RIGHT: meg4_pushkey("Rght"); break;
            case GLFW_KEY_HOME: meg4_pushkey("Home"); break;
            case GLFW_KEY_END: meg4_pushkey("End"); break;
            case GLFW_KEY_PAGE_UP: meg4_pushkey("PgUp"); break;
            case GLFW_KEY_PAGE_DOWN: meg4_pushkey("PgDn"); break;
            case GLFW_KEY_INSERT: meg4_pushkey("Ins"); break;
            case GLFW_KEY_DELETE: meg4_pushkey("Del"); break;
        }
        if(key >= 0 && key < (int)(sizeof(main_keymap)/sizeof(main_keymap[0]))) meg4_setkey(main_keymap[key]);
    } else {
        if(key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_LEFT_ALT) main_alt = 0;
        if(key >= 0 && key < (int)(sizeof(main_keymap)/sizeof(main_keymap[0]))) meg4_clrkey(main_keymap[key]);
    }
}

/**
 * Text input event callback
 */
void main_char(GLFWwindow *window, unsigned int unicode)
{
    char s[5] = { 0 };

    (void)window;
    if(main_alt || unicode < 32) return;
    if(unicode<0x80) { s[0]=unicode; } else
    if(unicode<0x800) { s[0]=((unicode>>6)&0x1F)|0xC0; s[1]=(unicode&0x3F)|0x80; } else
    if(unicode<0x10000) { s[0]=((unicode>>12)&0x0F)|0xE0; s[1]=((unicode>>6)&0x3F)|0x80; s[2]=(unicode&0x3F)|0x80; }
    else { s[0]=((unicode>>18)&0x07)|0xF0; s[1]=((unicode>>12)&0x3F)|0x80; s[2]=((unicode>>6)&0x3F)|0x80; s[3]=(unicode&0x3F)|0x80; }
    meg4_pushkey(s);
}

/**
 * Mouse button event callback
 */
void main_mouse(GLFWwindow *window, int button, int action, int mods) {
    uint16_t m = 0;
    (void)window; (void)mods;
    switch(button) {
        case GLFW_MOUSE_BUTTON_LEFT:   m = MEG4_BTN_L; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: m = MEG4_BTN_M; break;
        case GLFW_MOUSE_BUTTON_RIGHT:  m = MEG4_BTN_R; break;
    }
    if(action == GLFW_PRESS)
        meg4_setbtn(m);
    else
        meg4_clrbtn(m);
}

/**
 * Mouse movement event callback
 */
void main_pointer(GLFWwindow *window, double xpos, double ypos)
{
    int x, y, w, h, ww, wh, xp = xpos, yp = ypos;
    if(!meg4.screen.w || !meg4.screen.h) return;
    glfwGetFramebufferSize(window, &ww, &wh);
    w = ww; h = (int)meg4.screen.h * ww / (int)meg4.screen.w;
    if(h > wh) { h = wh; w = (int)meg4.screen.w * wh / (int)meg4.screen.h; }
    x = (ww - w) >> 1; y = (wh - h) >> 1;
    meg4_setptr(xp < x || !w ? 0 : (xp >= x + w ? meg4.screen.w : (xp - x) * meg4.screen.w / w),
        yp < y || !h ? 0 : (yp >= y + h ? meg4.screen.h : (yp - y) * meg4.screen.h / h));
}

/**
 * Scrolling event callback
 */
void main_scroll(GLFWwindow *window, double xdelta, double ydelta) {
    (void)window;
    meg4_setscr(ydelta > 0.0, ydelta < 0.0, xdelta > 0.0, xdelta < 0.0);
}

/**
 * Joystick event callback
 */
void main_joystick(int jid, int event) {
    int i;
    for(i = 0; i < 4 && controllerid[i] != jid; i++);
    switch(event) {
        case GLFW_DISCONNECTED:
            if(i < 4) controllerid[i] = -1;
        break;
        case GLFW_CONNECTED:
            if(i >= 4) for(i = 0; i < 4 && controllerid[i] != -1; i++);
            if(i < 4) controllerid[i] = jid;
        break;
    }
}

/**
 * Drop file event callback
 */
void main_dropfile(GLFWwindow *window, int count, const char **fn)
{
#ifndef NOEDITORS
    uint8_t *ptr;
    int i, l;
    char *n;

    (void)window;
    for(i = 0; i < count; i++) {
        if((ptr = main_readfile(!memcmp(fn[i], "file://", 7) ? (char*)fn[i] + 7 : (char*)fn[i], &l))) {
            n = strrchr(fn[i], SEP[0]); if(!n) n = (char*)fn[i]; else n++;
            meg4_insert(n, ptr, l);
            free(ptr);
        }
    }
#else
    (void)window; (void)count; (void)fn;
#endif
}

/**
 * Get text from clipboard (must be freed by caller)
 */
char *main_getclipboard(void)
{
    const char *str = glfwGetClipboardString(NULL);
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
    glfwSetClipboardString(NULL, str);
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
 * PA audio callback
 */
static int main_audio(const void *inp, void *out, long unsigned int framesPerBuffer, const PaStreamCallbackTimeInfo *info,
    PaStreamCallbackFlags flags, void *ctx)
{
    (void)inp; (void)info; (void)flags; (void)ctx;
    meg4_audiofeed((float*)out, framesPerBuffer);
    return 0;
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
    printf("\r\nMEG-4 v%s ("
#ifndef NOGLES
        "GLES"
#else
        "OpenGL"
#endif
        ", build %u) by bzt Copyright (C) 2023 GPLv3+\r\n\r\n", meg4ver, BUILD);
}

/**
 * Set up orthographic projection in a matrix
 */
#ifndef NOGLES
void main_ortho(GLfloat *m, float l, float r, float b, float t, float n, float f)
{
    memset(m, 0, 16 * sizeof(GLfloat));
    m[ 0] = 2.0/(r-l);
    m[ 5] = 2.0/(t-b);
    m[10] = -2.0/(f-n);
    m[ 3] = -(r+l)/(r-l);
    m[ 7] = -(t+b)/(t-b);
    m[11] = -(f+n)/(f-n);
    m[15] = 1.0;
}
#endif

/**
 * The real main procedure
 */
int main(int argc, char **argv)
{
#ifndef NOGLES
    const GLchar *vstr = "attribute vec2 apos;\nattribute vec2 atex;\nvarying vec2 vtex;\nuniform mat4 mvp;\nvoid main(){gl_Position=mvp*vec4(apos,0.0,1.0);vtex=atex;}";
    const GLchar *fstr = "precision mediump float;\nvarying vec2 vtex;\nuniform sampler2D stex;\nvoid main(){gl_FragColor=texture2D(stex,vtex);}";
    GLushort idx[] = { 0, 1, 2, 0, 2, 3 };
    GLuint vshdr = 0, fshdr = 0;
    GLint ret;
    GLfloat mvp[16] = { 0 };
#endif
    GLfloat vert[16] = { 0 }, x1, y1, x2, y2;
    double mx, my;
    const GLFWvidmode *vidmode;
    GLFWgamepadstate state;
    int i, j, w, h, ww, wh, lx, ly, rx, ry;
    char *infile = NULL, *fn;
    int32_t tickdiff;
    uint32_t ticks;
    uint8_t *ptr;
    FILE *out, *err;
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
    main_keymap[GLFW_KEY_SPACE] = MEG4_KEY_SPACE;
    main_keymap[GLFW_KEY_APOSTROPHE] = MEG4_KEY_APOSTROPHE;
    main_keymap[GLFW_KEY_COMMA] = MEG4_KEY_COMMA;
    main_keymap[GLFW_KEY_MINUS] = MEG4_KEY_MINUS;
    main_keymap[GLFW_KEY_PERIOD] = MEG4_KEY_PERIOD;
    main_keymap[GLFW_KEY_SLASH] = MEG4_KEY_SLASH;
    main_keymap[GLFW_KEY_0] = MEG4_KEY_0;
    main_keymap[GLFW_KEY_1] = MEG4_KEY_1;
    main_keymap[GLFW_KEY_2] = MEG4_KEY_2;
    main_keymap[GLFW_KEY_3] = MEG4_KEY_3;
    main_keymap[GLFW_KEY_4] = MEG4_KEY_4;
    main_keymap[GLFW_KEY_5] = MEG4_KEY_5;
    main_keymap[GLFW_KEY_6] = MEG4_KEY_6;
    main_keymap[GLFW_KEY_7] = MEG4_KEY_7;
    main_keymap[GLFW_KEY_8] = MEG4_KEY_8;
    main_keymap[GLFW_KEY_9] = MEG4_KEY_9;
    main_keymap[GLFW_KEY_SEMICOLON] = MEG4_KEY_SEMICOLON;
    main_keymap[GLFW_KEY_EQUAL] = MEG4_KEY_EQUAL;
    main_keymap[GLFW_KEY_A] = MEG4_KEY_A;
    main_keymap[GLFW_KEY_B] = MEG4_KEY_B;
    main_keymap[GLFW_KEY_C] = MEG4_KEY_C;
    main_keymap[GLFW_KEY_D] = MEG4_KEY_D;
    main_keymap[GLFW_KEY_E] = MEG4_KEY_E;
    main_keymap[GLFW_KEY_F] = MEG4_KEY_F;
    main_keymap[GLFW_KEY_G] = MEG4_KEY_G;
    main_keymap[GLFW_KEY_H] = MEG4_KEY_H;
    main_keymap[GLFW_KEY_I] = MEG4_KEY_I;
    main_keymap[GLFW_KEY_J] = MEG4_KEY_J;
    main_keymap[GLFW_KEY_K] = MEG4_KEY_K;
    main_keymap[GLFW_KEY_L] = MEG4_KEY_L;
    main_keymap[GLFW_KEY_M] = MEG4_KEY_M;
    main_keymap[GLFW_KEY_N] = MEG4_KEY_N;
    main_keymap[GLFW_KEY_O] = MEG4_KEY_O;
    main_keymap[GLFW_KEY_P] = MEG4_KEY_P;
    main_keymap[GLFW_KEY_Q] = MEG4_KEY_Q;
    main_keymap[GLFW_KEY_R] = MEG4_KEY_R;
    main_keymap[GLFW_KEY_S] = MEG4_KEY_S;
    main_keymap[GLFW_KEY_T] = MEG4_KEY_T;
    main_keymap[GLFW_KEY_U] = MEG4_KEY_U;
    main_keymap[GLFW_KEY_V] = MEG4_KEY_V;
    main_keymap[GLFW_KEY_X] = MEG4_KEY_X;
    main_keymap[GLFW_KEY_Y] = MEG4_KEY_Y;
    main_keymap[GLFW_KEY_Z] = MEG4_KEY_Z;
    main_keymap[GLFW_KEY_LEFT_BRACKET] = MEG4_KEY_LBRACKET;
    main_keymap[GLFW_KEY_BACKSLASH] = MEG4_KEY_BACKSLASH;
    main_keymap[GLFW_KEY_RIGHT_BRACKET] = MEG4_KEY_RBRACKET;
    main_keymap[GLFW_KEY_GRAVE_ACCENT] = MEG4_KEY_BACKQUOTE;
    main_keymap[GLFW_KEY_WORLD_1] = MEG4_KEY_INT1;
    main_keymap[GLFW_KEY_WORLD_2] = MEG4_KEY_INT2;
    main_keymap[119] = MEG4_KEY_SELECT;
    main_keymap[120] = MEG4_KEY_STOP;
    main_keymap[121] = MEG4_KEY_AGAIN;
    main_keymap[122] = MEG4_KEY_UNDO;
    main_keymap[123] = MEG4_KEY_CUT;
    main_keymap[124] = MEG4_KEY_COPY;
    main_keymap[125] = MEG4_KEY_PASTE;
    main_keymap[126] = MEG4_KEY_FIND;
    main_keymap[127] = MEG4_KEY_MUTE;
    main_keymap[128] = MEG4_KEY_VOLUP;
    main_keymap[129] = MEG4_KEY_VOLDN;
    main_keymap[135] = MEG4_KEY_INT1;
    main_keymap[136] = MEG4_KEY_INT2;
    main_keymap[137] = MEG4_KEY_INT3;
    main_keymap[138] = MEG4_KEY_INT4;
    main_keymap[139] = MEG4_KEY_INT5;
    main_keymap[140] = MEG4_KEY_INT6;
    main_keymap[141] = MEG4_KEY_INT7;
    main_keymap[142] = MEG4_KEY_INT8;
    main_keymap[144] = MEG4_KEY_LNG1;
    main_keymap[145] = MEG4_KEY_LNG2;
    main_keymap[146] = MEG4_KEY_LNG3;
    main_keymap[147] = MEG4_KEY_LNG4;
    main_keymap[148] = MEG4_KEY_LNG5;
    main_keymap[149] = MEG4_KEY_LNG6;
    main_keymap[150] = MEG4_KEY_LNG7;
    main_keymap[151] = MEG4_KEY_LNG8;
    main_keymap[GLFW_KEY_ENTER] = MEG4_KEY_ENTER;
    main_keymap[GLFW_KEY_TAB] = MEG4_KEY_TAB;
    main_keymap[GLFW_KEY_BACKSPACE] = MEG4_KEY_BACKSPACE;
    main_keymap[GLFW_KEY_INSERT] = MEG4_KEY_INS;
    main_keymap[GLFW_KEY_DELETE] = MEG4_KEY_DEL;
    main_keymap[GLFW_KEY_RIGHT] = MEG4_KEY_RIGHT;
    main_keymap[GLFW_KEY_LEFT] = MEG4_KEY_LEFT;
    main_keymap[GLFW_KEY_DOWN] = MEG4_KEY_DOWN;
    main_keymap[GLFW_KEY_UP] = MEG4_KEY_UP;
    main_keymap[GLFW_KEY_PAGE_UP] = MEG4_KEY_PGUP;
    main_keymap[GLFW_KEY_PAGE_DOWN] = MEG4_KEY_PGDN;
    main_keymap[GLFW_KEY_HOME] = MEG4_KEY_HOME;
    main_keymap[GLFW_KEY_END] = MEG4_KEY_END;
    main_keymap[GLFW_KEY_CAPS_LOCK] = MEG4_KEY_CAPSLOCK;
    main_keymap[GLFW_KEY_SCROLL_LOCK] = MEG4_KEY_SCRLOCK;
    main_keymap[GLFW_KEY_NUM_LOCK] = MEG4_KEY_NUMLOCK;
    main_keymap[GLFW_KEY_PRINT_SCREEN] = MEG4_KEY_PRSCR;
    main_keymap[GLFW_KEY_PAUSE] = MEG4_KEY_PAUSE;
    main_keymap[GLFW_KEY_F1] = MEG4_KEY_F1;
    main_keymap[GLFW_KEY_F2] = MEG4_KEY_F2;
    main_keymap[GLFW_KEY_F3] = MEG4_KEY_F3;
    main_keymap[GLFW_KEY_F4] = MEG4_KEY_F4;
    main_keymap[GLFW_KEY_F5] = MEG4_KEY_F5;
    main_keymap[GLFW_KEY_F6] = MEG4_KEY_F6;
    main_keymap[GLFW_KEY_F7] = MEG4_KEY_F7;
    main_keymap[GLFW_KEY_F8] = MEG4_KEY_F8;
    main_keymap[GLFW_KEY_F9] = MEG4_KEY_F9;
    main_keymap[GLFW_KEY_F10] = MEG4_KEY_F10;
    main_keymap[GLFW_KEY_F11] = MEG4_KEY_F11;
    main_keymap[GLFW_KEY_F12] = MEG4_KEY_F12;
    main_keymap[GLFW_KEY_KP_0] = MEG4_KEY_KP_0;
    main_keymap[GLFW_KEY_KP_1] = MEG4_KEY_KP_1;
    main_keymap[GLFW_KEY_KP_2] = MEG4_KEY_KP_2;
    main_keymap[GLFW_KEY_KP_3] = MEG4_KEY_KP_3;
    main_keymap[GLFW_KEY_KP_4] = MEG4_KEY_KP_4;
    main_keymap[GLFW_KEY_KP_5] = MEG4_KEY_KP_5;
    main_keymap[GLFW_KEY_KP_6] = MEG4_KEY_KP_6;
    main_keymap[GLFW_KEY_KP_7] = MEG4_KEY_KP_7;
    main_keymap[GLFW_KEY_KP_8] = MEG4_KEY_KP_8;
    main_keymap[GLFW_KEY_KP_9] = MEG4_KEY_KP_9;
    main_keymap[GLFW_KEY_KP_DECIMAL] = MEG4_KEY_KP_DEC;
    main_keymap[GLFW_KEY_KP_DIVIDE] = MEG4_KEY_KP_DIV;
    main_keymap[GLFW_KEY_KP_MULTIPLY] = MEG4_KEY_KP_MUL;
    main_keymap[GLFW_KEY_KP_SUBTRACT] = MEG4_KEY_KP_SUB;
    main_keymap[GLFW_KEY_KP_ADD] = MEG4_KEY_KP_ADD;
    main_keymap[GLFW_KEY_KP_ENTER] = MEG4_KEY_KP_ENTER;
    main_keymap[GLFW_KEY_KP_EQUAL] = MEG4_KEY_KP_EQUAL;
    main_keymap[GLFW_KEY_LEFT_SHIFT] = MEG4_KEY_LSHIFT;
    main_keymap[GLFW_KEY_LEFT_CONTROL] = MEG4_KEY_LCTRL;
    main_keymap[GLFW_KEY_LEFT_ALT] = MEG4_KEY_LALT;
    main_keymap[GLFW_KEY_LEFT_SUPER] = MEG4_KEY_LSUPER;
    main_keymap[GLFW_KEY_RIGHT_SHIFT] = MEG4_KEY_RSHIFT;
    main_keymap[GLFW_KEY_RIGHT_CONTROL] = MEG4_KEY_RCTRL;
    main_keymap[GLFW_KEY_RIGHT_ALT] = MEG4_KEY_RALT;
    main_keymap[GLFW_KEY_RIGHT_SUPER] = MEG4_KEY_RSUPER;
    main_keymap[GLFW_KEY_MENU] = MEG4_KEY_MENU;
    scrbuf = (uint32_t*)malloc(640 * 400 * sizeof(uint32_t));
    if(!scrbuf) {
        main_log(0, "unable to allocate screen buffer");
        return 1;
    }
    /* PortAudio litters my screen with lots of garbage about missing daemons and server connection errors and such...
     * I won't install hudred of megabytes of bloatware just to make it shut up, so hush, little PortAudio */
    out = stdout; err = stderr; stdout = stderr = fopen(
#ifdef __WIN32__
        "NUL:"
#else
        "/dev/null"
#endif
        , "w");
    /* initialize the audio */
    audio = (Pa_Initialize() == paNoError) ? 1 : 0; pa = NULL;
    if(audio && (Pa_OpenDefaultStream(&pa, 0, 1, paFloat32, 44100, 4096, main_audio, NULL) != paNoError || !pa)) {
        Pa_Terminate(); audio = 0; pa = NULL;
    }
    /* restore stdout, stderr */
    fclose(stderr); stdout = out; stderr = err;
    if(verbose && audio) main_log(1, "audio opened %uHz, %u bits", 44100, 32);
    /* initialize screen and other stuff */
    if(!glfwInit()) {
        main_log(0, "unable to initialize GLFW");
        return 1;
    }
    glfwSetErrorCallback(main_error);
#ifndef NOGLES
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    if(!(monitor = glfwGetPrimaryMonitor()) || !(vidmode = glfwGetVideoMode(monitor))) {
        glfwTerminate();
        main_log(0, "unable to get GLFW monitor");
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
    win_w = main_w = vidmode->width;
    win_h = main_h = vidmode->height;
#if DEBUG
    main_win(640, 400, 0);
#else
    main_win(640/*main_w*/, 400/*main_h*/, 0/*1*/);
    main_fullscreen();
#endif
    if(!window) {
        meg4_poweroff();
        glfwTerminate();
        main_log(0, "unable to initialize GLFW window");
        return 1;
    }
#ifdef __WIN32__
    hwnd = glfwGetWin32Window(window);
#endif
    /* according to https://www.glfw.org/docs/latest/input_guide.html#gamepad_mapping we dont' have to load
     * gamecontrollerdb.txt ourselves, because glfw should already contain a copy of that */
#ifndef NOGLES
    if(!(vshdr = glCreateShader(GL_VERTEX_SHADER))) {
serr:   if(vshdr) glDeleteShader(vshdr);
        if(fshdr) glDeleteShader(fshdr);
        glfwTerminate();
        main_log(0, "unable to get shaders");
        return 1;
    }
    glShaderSource(vshdr, 1, &vstr, NULL);
    glCompileShader(vshdr);
    glGetShaderiv(vshdr, GL_COMPILE_STATUS, &ret);
    if(!ret) goto serr;
    if(!(fshdr = glCreateShader(GL_FRAGMENT_SHADER))) goto serr;
    glShaderSource(fshdr, 1, &fstr, NULL);
    glCompileShader(fshdr);
    glGetShaderiv(fshdr, GL_COMPILE_STATUS, &ret);
    if(!ret) goto serr;
    program = glCreateProgram();
    if(!program) goto serr;
    glAttachShader(program, vshdr);
    glAttachShader(program, fshdr);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ret);
    if(!ret) { glDeleteProgram(program); goto serr; }
    glDeleteShader(vshdr);
    glDeleteShader(fshdr);
    ploc = glGetAttribLocation(program, "apos");
    tloc = glGetAttribLocation(program, "atex");
    sloc = glGetUniformLocation(program, "stex");
    mloc = glGetUniformLocation(program, "mvp");
    glEnableVertexAttribArray(ploc);
    glEnableVertexAttribArray(tloc);
#endif
    for(i = 0, j = GLFW_JOYSTICK_1; i < 4 && j < GLFW_JOYSTICK_LAST; j++)
        if(glfwJoystickPresent(j)) controllerid[i++] = j;
    glfwSetJoystickCallback(main_joystick);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &screen);
    glBindTexture(GL_TEXTURE_2D, screen);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    /* swapinterval is buggy in glfw, so we take care of the 60 FPS delay ourselves */
    glfwSwapInterval(0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetKeyCallback(window, main_key);
    glfwSetCharCallback(window, main_char);
    glfwSetMouseButtonCallback(window, main_mouse);
    glfwSetCursorPosCallback(window, main_pointer);
    glfwSetScrollCallback(window, main_scroll);
    glfwSetDropCallback(window, main_dropfile);
    if(audio && pa) Pa_StartStream(pa);
    while(!glfwWindowShouldClose(window)) {
        ticks = (uint32_t)(glfwGetTime() * 1000);
        glfwGetFramebufferSize(window, &ww, &wh);
        glfwGetCursorPos(window, &mx, &my);
        main_pointer(window, mx, my);
        meg4_run();
        meg4_redraw(scrbuf, 640, 400, 640 * 4);
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
        glViewport(0, 0, ww, wh);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifndef NOGLES
        /* new-wave. Mega hyper super fast and overcomplicated GLES2 */
        glUseProgram(program);
        glVertexAttribPointer(ploc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vert);
        glVertexAttribPointer(tloc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vert[2]);
        glUniform1i(sloc, 0);
        main_ortho(mvp, -ww/2, ww/2, wh/2, -wh/2, 0.0, 1.0);
        glUniformMatrix4fv(mloc, 1, GL_FALSE, mvp);
#else
        /* old-school. Much simpler, and even faster... */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-ww/2, ww/2, -wh/2, wh/2, 0.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnable(GL_TEXTURE_2D);
#endif
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screen);
        j = ww / 320;
        i = nearest || (!(ww % 320) && (j == 1 || j == 2 || j == 4 || j == 8) && !(wh % 200)) ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 640, 400, 0, GL_RGBA, GL_UNSIGNED_BYTE, scrbuf);
#ifndef NOGLES
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, idx);
#else
        glBegin(GL_QUADS);
        glVertex3f(vert[0], vert[1], 0); glTexCoord2f(vert[3], vert[2]);
        glVertex3f(vert[4], vert[5], 0); glTexCoord2f(vert[7], vert[6]);
        glVertex3f(vert[8], vert[9], 0); glTexCoord2f(vert[11], vert[10]);
        glVertex3f(vert[12], vert[13], 0); glTexCoord2f(vert[15], vert[14]);
        glEnd();
        glFlush();
        glDisable(GL_TEXTURE_2D);
#endif
        /* no events for gamepads, we must poll... */
        for(i = 0; i < 4; i++)
            if(controllerid[i] != -1) {
                meg4_clrpad(i, 0xff);
                /* new gamepad interface, joysticks with proper mappings */
                if(glfwGetGamepadState(controllerid[i], &state)) {
                    lx = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] * 32768.0; rx = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * 32768.0;
                    ly = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * 32768.0; ry = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * 32768.0;
                    if(lx < -le16toh(meg4.mmio.padtres) || rx < -le16toh(meg4.mmio.padtres) || state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT])
                        meg4_setpad(i, MEG4_BTN_L);
                    if(lx >  le16toh(meg4.mmio.padtres) || rx >  le16toh(meg4.mmio.padtres) || state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT])
                        meg4_setpad(i, MEG4_BTN_R);
                    if(ly < -le16toh(meg4.mmio.padtres) || ry < -le16toh(meg4.mmio.padtres) || state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP])
                        meg4_setpad(i, MEG4_BTN_U);
                    if(ly >  le16toh(meg4.mmio.padtres) || ry >  le16toh(meg4.mmio.padtres) || state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN])
                        meg4_setpad(i, MEG4_BTN_D);
                    if(state.buttons[GLFW_GAMEPAD_BUTTON_A]) meg4_setpad(i, MEG4_BTN_A);
                    if(state.buttons[GLFW_GAMEPAD_BUTTON_B]) meg4_setpad(i, MEG4_BTN_B);
                    if(state.buttons[GLFW_GAMEPAD_BUTTON_X]) meg4_setpad(i, MEG4_BTN_X);
                    if(state.buttons[GLFW_GAMEPAD_BUTTON_Y]) meg4_setpad(i, MEG4_BTN_Y);
                }
#ifdef JOYFALLBACK
                else {
                    ptr = (uint8_t*)glfwGetJoystickButtons(controllerid[i], &w);
                    for(j = 0; ptr && j < w; j++) {
                        if(ptr[j] & GLFW_HAT_UP) meg4_setpad(i, MEG4_BTN_U);
                        if(ptr[j] & GLFW_HAT_RIGHT) meg4_setpad(i, MEG4_BTN_R);
                        if(ptr[j] & GLFW_HAT_DOWN) meg4_setpad(i, MEG4_BTN_D);
                        if(ptr[j] & GLFW_HAT_LEFT) meg4_setpad(i, MEG4_BTN_L);
                        /* just a guess because there's no mapping */
                        if(ptr[j] &  16) meg4_setpad(i, MEG4_BTN_A);
                        if(ptr[j] &  32) meg4_setpad(i, MEG4_BTN_B);
                        if(ptr[j] &  64) meg4_setpad(i, MEG4_BTN_X);
                        if(ptr[j] & 128) meg4_setpad(i, MEG4_BTN_Y);
                    }
                }
#endif
            }
        glfwSwapBuffers(window);
        glfwPollEvents();
        tickdiff = (1000/60) - ((uint32_t)(glfwGetTime() * 1000) - ticks);
        if(tickdiff > 0 && tickdiff < 1000) {
            if(verbose == 2) { printf("meg4: free time %d msec  \r",tickdiff); fflush(stdout); }
            main_delay(tickdiff);
        }
    }
    main_quit();
    return 0;
}
