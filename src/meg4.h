/*
 * meg4/meg4.h
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
 * @brief Core emulator header
 *
 */

#ifndef _MEG4_H_
#define _MEG4_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _MSC_VER
#pragma pack(push)
#pragma pack(1)
#endif

/*****************************************************************************
 *                            MEG-4 Memory Map                               *
 *****************************************************************************/
/* all values (save fwver) in this struct MUST be little endian, regardless to the host machine */
typedef struct {
    /* General MMIO, 0 - 0x1FFFF */
    /* Misc */
    uint8_t  fwver[3];                      /* 00000 firmware version (big endian, major.minor.bugfix) */
    int8_t   perf;                          /* 00003 performance counter, available time in 1/1000th secs */
    uint32_t tick;                          /* 00004 number of 1/1000th second ticks since power on */
    uint64_t now;                           /* 00008 UTC unix timestamp */
    uint8_t  locale[2];                     /* 00010 current locale */
    /* Pointer */
    uint16_t ptrbtn, ptrspr;                /* 00012 pointer button state and sprite index */
    uint16_t ptrx, ptry;                    /* 00016 pointer coordinates */
    /* Keyboard */
    uint8_t  kbdtail, kbdhead;              /* 0001A keyboard queue head and tail */
    uint32_t kbd[16];                       /* 0001C keyboard queue */
    uint8_t  kbdkeys[18];                   /* 0005C key pressed states */
    /* Gamepads */
    uint16_t padtres;                       /* 0006E gamepad joystick treshold */
    uint8_t  padkeys[8];                    /* 00070 primary gamepad button - key mappings */
    uint8_t  padbtns[4];                    /* 00078 gamepad button states */
    /* Selectors */
    uint8_t  wavesel;                       /* 0007C waveform bank selector */
    uint8_t  tracksel;                      /* 0007D music track bank selector */
    uint8_t  fontsel;                       /* 0007E UNICODE code point upper byte for font mapping */
    uint8_t  mapsel;                        /* 0007F sprite selector for the map */
    /* GPU */
    uint32_t palette[256];                  /* 00080 palette */
    uint16_t cropx0, cropx1, cropy0, cropy1;/* 00480 crop area for draw functions */
    uint16_t scrx, scry;                    /* 00488 vram displayed on screen */
    uint8_t  turtlepen, turtlepalidx;       /* 0048C turtle pen down flag and pen color */
    uint16_t turtlea;                       /* 0048E turtle direction (angle in degrees, 0 - 359) */
    uint16_t turtlex, turtley;              /* 00490 turtle position (could be off-screen) */
    uint16_t mazew, mazer;                  /* 00494 maze walking and rotating speed */
    uint8_t  conf, conb;                    /* 00498 console foreground and background (for print) */
    uint16_t conx, cony;                    /* 0049A console cursor position (for print) */
    uint16_t camx, camy, camz;              /* 0049E camera position (for tri3d) */
    uint8_t  mbz2[22];                      /* reserved for future GPU use */
    /* DSP */
    uint8_t  dsp_ticks;                     /* 004BA current tempo */
    uint8_t  dsp_track;                     /* 004BB current track being played */
    uint16_t dsp_row;                       /* 004BC current row being played */
    uint16_t dsp_num;                       /* 004BE number of total rows in current track */
    uint32_t dsp_ch[16];                    /* 004C0 current channel status (volume << 24, waveform << 16, position) */
    uint8_t  sounds[64 * 4];                /* 00500 sound effects */
    /* GPU */
    uint8_t  map[320 * 200];                /* 00600 maps */
    uint8_t  sprites[256 * 256];            /* 10000 sprites */
    /* Dynamically Mapped Memory, 0x20000 - 0x2FFFF */
                                            /* 20000 waveform bank (see wavesel) */
                                            /* 24000 music track bank (see tracksel) */
                                            /* 28000-2FFFF fonts glyph bank (see fontsel) */
    /* Free, user available RAM, 0x30000 - 0xBFFFF */
}
#ifndef _MSC_VER
__attribute__((packed))
#endif
meg4_mmio_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#define MEG4_MEM_USER  0x30000              /* sizeof(meg4.mmio) + 0x10000 */
#define MEG4_MEM_LIMIT 0xC0000              /* sizeof(meg4.mmio) + 0x10000 + sizeof(meg4.data) */

/* mouse and gamepad buttons */
#define MEG4_BTN_L   1                      /* mouse left button, gamepad left */
#define MEG4_BTN_M   2                      /* mouse middle button */
#define MEG4_BTN_U   2                      /* gamepad up */
#define MEG4_BTN_R   4                      /* mouse right button, gamepad right */
#define MEG4_BTN_D   8                      /* gamepad down */
#define MEG4_SCR_U   8                      /* mouse scrolled up */
#define MEG4_BTN_A  16                      /* gamepad A button */
#define MEG4_SCR_D  16                      /* mouse scrolled down */
#define MEG4_BTN_B  32                      /* gamepad B button */
#define MEG4_SCR_L  32                      /* mouse scrolled left */
#define MEG4_BTN_X  64                      /* gamepad X button */
#define MEG4_SCR_R  64                      /* mouse scrolled right */
#define MEG4_BTN_Y 128                      /* gamepad Y button */

/* keyboard scancodes */
enum {
    /* first row */
    MEG4_KEY_CHEAT, MEG4_KEY_F1, MEG4_KEY_F2, MEG4_KEY_F3, MEG4_KEY_F4, MEG4_KEY_F5, MEG4_KEY_F6, MEG4_KEY_F7, MEG4_KEY_F8,
    MEG4_KEY_F9, MEG4_KEY_F10, MEG4_KEY_F11, MEG4_KEY_F12, MEG4_KEY_PRSCR, MEG4_KEY_SCRLOCK, MEG4_KEY_PAUSE,
    /* second row */
    MEG4_KEY_BACKQUOTE /* ` */, MEG4_KEY_1, MEG4_KEY_2, MEG4_KEY_3, MEG4_KEY_4, MEG4_KEY_5, MEG4_KEY_6, MEG4_KEY_7, MEG4_KEY_8,
    MEG4_KEY_9, MEG4_KEY_0, MEG4_KEY_MINUS /* - */, MEG4_KEY_EQUAL /* = */, MEG4_KEY_BACKSPACE,
    /* third row */
    MEG4_KEY_TAB, MEG4_KEY_Q, MEG4_KEY_W, MEG4_KEY_E, MEG4_KEY_R, MEG4_KEY_T, MEG4_KEY_Y, MEG4_KEY_U, MEG4_KEY_I, MEG4_KEY_O,
    MEG4_KEY_P, MEG4_KEY_LBRACKET /* [ */, MEG4_KEY_RBRACKET /* ] */, MEG4_KEY_ENTER,
    /* fourth row */
    MEG4_KEY_CAPSLOCK, MEG4_KEY_A, MEG4_KEY_S, MEG4_KEY_D, MEG4_KEY_F, MEG4_KEY_G, MEG4_KEY_H, MEG4_KEY_J, MEG4_KEY_K, MEG4_KEY_L,
    MEG4_KEY_SEMICOLON /* ; */, MEG4_KEY_APOSTROPHE /* ' */, MEG4_KEY_BACKSLASH /* \ */,
    /* fifth row */
    MEG4_KEY_LSHIFT, MEG4_KEY_LESS /* < */, MEG4_KEY_Z, MEG4_KEY_X, MEG4_KEY_C, MEG4_KEY_V, MEG4_KEY_B, MEG4_KEY_N, MEG4_KEY_M,
    MEG4_KEY_COMMA /* , */, MEG4_KEY_PERIOD /* . */, MEG4_KEY_SLASH /* / */, MEG4_KEY_RSHIFT,
    /* sixth row */
    MEG4_KEY_LCTRL, MEG4_KEY_LSUPER, MEG4_KEY_LALT, MEG4_KEY_SPACE, MEG4_KEY_RALT, MEG4_KEY_RSUPER, MEG4_KEY_MENU, MEG4_KEY_RCTRL,
    /* middle coloumn */
    MEG4_KEY_INS, MEG4_KEY_HOME, MEG4_KEY_PGUP, MEG4_KEY_DEL, MEG4_KEY_END, MEG4_KEY_PGDN,
    MEG4_KEY_UP, MEG4_KEY_LEFT, MEG4_KEY_DOWN, MEG4_KEY_RIGHT,
    /* numpad */
    MEG4_KEY_NUMLOCK, MEG4_KEY_KP_DIV /* / */, MEG4_KEY_KP_MUL /* * */, MEG4_KEY_KP_SUB /* - */,
    MEG4_KEY_KP_7, MEG4_KEY_KP_8, MEG4_KEY_KP_9, MEG4_KEY_KP_ADD /* + */,
    MEG4_KEY_KP_4, MEG4_KEY_KP_5, MEG4_KEY_KP_6,
    MEG4_KEY_KP_1, MEG4_KEY_KP_2, MEG4_KEY_KP_3, MEG4_KEY_KP_ENTER,
    MEG4_KEY_KP_0, MEG4_KEY_KP_DEC /* . */,
    /* special keys */
    MEG4_KEY_INT1, MEG4_KEY_INT2, MEG4_KEY_INT3, MEG4_KEY_INT4, MEG4_KEY_INT5, MEG4_KEY_INT6, MEG4_KEY_INT7, MEG4_KEY_INT8,
    MEG4_KEY_LNG1, MEG4_KEY_LNG2, MEG4_KEY_LNG3, MEG4_KEY_LNG4, MEG4_KEY_LNG5, MEG4_KEY_LNG6, MEG4_KEY_LNG7, MEG4_KEY_LNG8,
    MEG4_KEY_APP, MEG4_KEY_POWER, MEG4_KEY_KP_EQUAL, MEG4_KEY_EXEC, MEG4_KEY_HELP, MEG4_KEY_SELECT, MEG4_KEY_STOP, MEG4_KEY_AGAIN,
    MEG4_KEY_UNDO, MEG4_KEY_CUT, MEG4_KEY_COPY, MEG4_KEY_PASTE, MEG4_KEY_FIND, MEG4_KEY_MUTE, MEG4_KEY_VOLUP, MEG4_KEY_VOLDN,
    /* must be the last */
    MEG4_NUM_KEY
};
extern char meg4_keycode_assert[MEG4_NUM_KEY >= 144 ? -1 : 1];

/* music notes */
enum {
    MEG4_NOTE_NONE,
    /* octave 0 */
    MEG4_NOTE_C_0, MEG4_NOTE_Cs0, MEG4_NOTE_D_0, MEG4_NOTE_Ds0, MEG4_NOTE_E_0, MEG4_NOTE_F_0, MEG4_NOTE_Fs0,
    MEG4_NOTE_G_0, MEG4_NOTE_Gs0, MEG4_NOTE_A_0, MEG4_NOTE_As0, MEG4_NOTE_B_0,
    /* octave 1 */
    MEG4_NOTE_C_1, MEG4_NOTE_Cs1, MEG4_NOTE_D_1, MEG4_NOTE_Ds1, MEG4_NOTE_E_1, MEG4_NOTE_F_1, MEG4_NOTE_Fs1,
    MEG4_NOTE_G_1, MEG4_NOTE_Gs1, MEG4_NOTE_A_1, MEG4_NOTE_As1, MEG4_NOTE_B_1,
    /* octave 2 */
    MEG4_NOTE_C_2, MEG4_NOTE_Cs2, MEG4_NOTE_D_2, MEG4_NOTE_Ds2, MEG4_NOTE_E_2, MEG4_NOTE_F_2, MEG4_NOTE_Fs2,
    MEG4_NOTE_G_2, MEG4_NOTE_Gs2, MEG4_NOTE_A_2, MEG4_NOTE_As2, MEG4_NOTE_B_2,
    /* octave 3 */
    MEG4_NOTE_C_3, MEG4_NOTE_Cs3, MEG4_NOTE_D_3, MEG4_NOTE_Ds3, MEG4_NOTE_E_3, MEG4_NOTE_F_3, MEG4_NOTE_Fs3,
    MEG4_NOTE_G_3, MEG4_NOTE_Gs3, MEG4_NOTE_A_3, MEG4_NOTE_As3, MEG4_NOTE_B_3,
    /* octave 4 */
    MEG4_NOTE_C_4, MEG4_NOTE_Cs4, MEG4_NOTE_D_4, MEG4_NOTE_Ds4, MEG4_NOTE_E_4, MEG4_NOTE_F_4, MEG4_NOTE_Fs4,
    MEG4_NOTE_G_4, MEG4_NOTE_Gs4, MEG4_NOTE_A_4, MEG4_NOTE_As4, MEG4_NOTE_B_4,
    /* octave 5 */
    MEG4_NOTE_C_5, MEG4_NOTE_Cs5, MEG4_NOTE_D_5, MEG4_NOTE_Ds5, MEG4_NOTE_E_5, MEG4_NOTE_F_5, MEG4_NOTE_Fs5,
    MEG4_NOTE_G_5, MEG4_NOTE_Gs5, MEG4_NOTE_A_5, MEG4_NOTE_As5, MEG4_NOTE_B_5,
    /* octave 6 */
    MEG4_NOTE_C_6, MEG4_NOTE_Cs6, MEG4_NOTE_D_6, MEG4_NOTE_Ds6, MEG4_NOTE_E_6, MEG4_NOTE_F_6, MEG4_NOTE_Fs6,
    MEG4_NOTE_G_6, MEG4_NOTE_Gs6, MEG4_NOTE_A_6, MEG4_NOTE_As6, MEG4_NOTE_B_6,
    /* octave 7 */
    MEG4_NOTE_C_7, MEG4_NOTE_Cs7, MEG4_NOTE_D_7, MEG4_NOTE_Ds7, MEG4_NOTE_E_7, MEG4_NOTE_F_7, MEG4_NOTE_Fs7,
    MEG4_NOTE_G_7, MEG4_NOTE_Gs7, MEG4_NOTE_A_7, MEG4_NOTE_As7, MEG4_NOTE_B_7,
    MEG4_NUM_NOTE
};

/* one DSP channel */
typedef struct {
    uint8_t note;                           /* Note number (0 to 96) */
    uint8_t sample;                         /* Sample number (0 to 31) */
    uint8_t et;                             /* Current effect */
    uint8_t ep;                             /* Raw effect parameter value */
    uint8_t master;                         /* Master volume */
    uint8_t volume;                         /* Base volume without tremolo (0 to 255) */
    uint8_t tremolo;                        /* Volume with tremolo adjustment */
    uint16_t period;                        /* Note period (28 to 6848) */
    uint16_t delayed;                       /* Delayed note period (28 to 6848) */
    uint16_t target;                        /* Target period (for tone portamento) */
    uint8_t finetune;                       /* Note finetune (0 to 15) */
    uint8_t lfo_step;                       /* LFO step counter */
    uint8_t lfo_vib;                        /* LFO vibrato waveform */
    uint8_t lfo_trem;                       /* LFO tremolo waveform */
    uint8_t porta;                          /* Tone portamento parameter value */
    uint8_t vibra;                          /* Vibrato parameter value */
    uint8_t trem;                           /* Tremolo parameter value */
    uint8_t offs;                           /* Sample offset parameter value */
    uint8_t fporta;                         /* Fine portamento parameter value */
    uint8_t fvibra;                         /* Fine vibrato parameter value */
    uint8_t fvsu;                           /* Fine volume slide up parameter value */
    uint8_t fvsd;                           /* Fine volume slide down parameter value */
    uint8_t dirty;                          /* Pitch/volume dirty flags */
    float position;                         /* Position in sample data buffer */
    float increment;                        /* Position increment per output sample */
} meg4_dsp_ch_t;

/* DSP context */
typedef struct {
    meg4_dsp_ch_t ch[16];                   /* 4 music channels and 12 sound channels */
    int ticks_per_row;                      /* A.K.A. song speed (initially 6) */
    float samples_per_tick;                 /* Depends on sample rate and BPM */
    uint8_t track;                          /* track currently being played */
    uint16_t row;                           /* row currently being played */
    uint16_t num;                           /* number of total rows in track */
    int tick[2];                            /* Current tick in row */
    float sample[2];                        /* Current sample in tick */
} meg4_dsp_t;

/* GPU pixel buffer */
typedef struct {
    int w, h;                               /* width, height */
    uint32_t *buf;                          /* pixel data, RGBA */
} meg4_pixbuf_t;

/* overlay */
typedef struct {
    uint32_t size;                          /* overlay size */
    uint8_t *data;                          /* buffer data */
} meg4_ovl_t;

/* main MEG-4 context */
typedef struct {
    meg4_pixbuf_t screen;                   /* screen, buf not allocated, points into vram */
    uint32_t vram[640 * 400];               /* video ram */
#ifndef NOEDITORS
    uint32_t valt[640 * 400];               /* alternative video ram, only used by the editors */
#endif
    uint8_t font[9 * 65536];                /* font */
    uint8_t mode, kbdmode;                  /* operating mode, keyboard input mode */
    meg4_mmio_t mmio;                       /* user accessible mmio area */
    meg4_dsp_t dram;                        /* the default dsp ram */
#ifndef NOEDITORS
    meg4_dsp_t dalt;                        /* alternative dsp ram, only used by the editors */
#endif
    uint8_t waveforms[31][16384];           /* waveforms */
    uint8_t tracks[8][16384];               /* music tracks */
    uint8_t data[576 * 1024];               /* freely usable ram */
    uint32_t *code, code_len;               /* compiled bytecode and length */
    uint8_t  code_type;                     /* bytecode's type */
    uint32_t cs[512];                       /* callstack, bp + pc pairs */
    uint32_t tmr, dp, bp, sp, cp, pc;       /* delay timer, data, base, stack, callstack pointers and program counter */
    int ac;                                 /* integer accumulator register */
    float af;                               /* floating point accumulator register */
    uint8_t flg;                            /* flags (bit 0: setup done, 1: io block, 2: timer block, 3: stopped) */
    char *src;                              /* source code */
    uint32_t src_len, src_bm[42];           /* length and bookmarks */
    meg4_ovl_t ovls[256];                   /* overlays, MEG-4 "files" */
    uint32_t numfmm, *fmm, numamm, *amm;    /* dynamically allocated, freed and allocated records, both addr+size pairs */
    uint8_t mipmap[128*128*4 + 64*64*4 + 32*32*4];
} meg4_t;

/* api.h - scripts API */
typedef struct {
    char *name;                             /* name of the function */
    char ret;                               /* return type (0 void, 1 int, 2 addr, 3 string, 4 float) */
    char narg;                              /* number of arguments */
    char varg;                              /* index or first variadic parameter or 0 */
    uint16_t amsk;                          /* address bitmask, which arguments are pointers */
    uint16_t smsk;                          /* string bitmask, which arguments are strings */
    uint16_t fmsk;                          /* float bitmask, which arguments are floats */
    uint16_t umsk;                          /* unsigned bitmask, which arguments are unsigned */
} meg4_api_t;
extern meg4_api_t meg4_api[];

/* endianess */
#ifndef MEG4_BYTEORDER
# ifdef __linux__
#  include <endian.h>
#  define MEG4_BYTEORDER  __BYTE_ORDER
# elif defined(__OpenBSD__)
#  include <endian.h>
#  define MEG4_BYTEORDER  BYTE_ORDER
# else
#  if defined(__hppa__) || defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MIPSEB__)) || defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC)
#   define MEG4_BYTEORDER  4321
#  else
#   define MEG4_BYTEORDER  1234
#  endif
# endif
#endif
#if MEG4_BYTEORDER == 1234
# ifndef htole16
#  define htole16(x) ((uint16_t)(x))
#  define le16toh(x) ((uint16_t)(x))
#  define htole32(x) ((uint32_t)(x))
#  define le32toh(x) ((uint32_t)(x))
#  define htole64(x) ((uint64_t)(x))
#  define le64toh(x) ((uint64_t)(x))
# endif
#else
# ifndef htole16
#  define htole16(x) ((uint16_t)(((char)(char*)&(x)[1])||((char)(char*)&(x)[0])<<8))
#  define le16toh(x) ((uint16_t)(((char)(char*)(x)[1])||((char)(char*)(x)[0])<<8))
#  define htole32(x) ((uint32_t)(((char)(char*)&(x)[3])||((char)(char*)&(x)[2])<<8|| \
    ((char)(char*)&(x)[1])<<16||((char)(char*)&(x)[0])<<24))
#  define le32toh(x) ((uint32_t)(((char)(char*)(x)[3])||((char)(char*)(x)[2])<<8|| \
    ((char)(char*)(x)[1])<<16||((char)(char*)(x)[0])<<24))
#  define htole64(x) ((uint64_t)(((char)(char*)&(x)[7])||((char)(char*)&(x)[6])<<8|| \
    ((char)(char*)&(x)[5])<<16||((char)(char*)&(x)[4])<<24||((char)(char*)&(x)[3])<<32||((char)(char*)&(x)[2])<<40|| \
    ((char)(char*)&(x)[1])<<48||((char)(char*)&(x)[0])<<56))
#  define le64toh(x) ((uint64_t)(((char)(char*)(x)[7])||((char)(char*)(x)[6])<<8|| \
    ((char)(char*)(x)[5])<<16||((char)(char*)(x)[4])<<24||((char)(char*)(x)[3])<<32||((char)(char*)(x)[2])<<40|| \
    ((char)(char*)(x)[1])<<48||((char)(char*)(x)[0])<<56))
# endif
#endif

/* pointers */
#define MEG4_PTR_NORM       htole16(0x03fb) /* normal, default pointer */
#define MEG4_PTR_TEXT       htole16(0x93fc) /* text input, i-beam pointer */
#define MEG4_PTR_HAND       htole16(0x0bfd) /* links, hand pointer */
#define MEG4_PTR_ERR        htole16(0x93fe) /* error happened pointer */
#define MEG4_PTR_NONE       htole16(0xffff) /* hide pointer */
#define MEG4_PTR_ICON       (le16toh(meg4.mmio.ptrspr) & 0x3ff)
#define MEG4_PTR_HOTSPOT_X  ((le16toh(meg4.mmio.ptrspr) >> 10) & 7)
#define MEG4_PTR_HOTSPOT_Y  ((le16toh(meg4.mmio.ptrspr) >> 13) & 7)
#define MEG4_PTR_IS_SPR     (MEG4_PTR_ICON<0x3fb)

/* operating modes */
enum { MEG4_MODE_GAME, MEG4_MODE_GURU, MEG4_MODE_LOAD, MEG4_MODE_SAVE, MEG4_MODE_HELP, MEG4_MODE_DEBUG, MEG4_MODE_VISUAL,
    MEG4_MODE_CODE, MEG4_MODE_SPRITE, MEG4_MODE_MAP, MEG4_MODE_FONT, MEG4_MODE_SOUND, MEG4_MODE_MUSIC, MEG4_MODE_OVERLAY,
    MEG4_NUM_MODE };

#define MEG4_PI 3.14159265358979323846

/* invoke the built-in debugger */
#ifndef NOEDITORS
void debug_rte(int err);
#define MEG4_DEBUGGER(a)    debug_rte(a)
#else
#define MEG4_DEBUGGER(a)    meg4.mode = MEG4_MODE_GURU
#endif

/* initialized data segment */
extern uint8_t *meg4_init;
extern uint32_t meg4_init_len;

/* the MEG-4 pointer type, an universal address and string (in Lua strings aren't byte arrays...) */
typedef uint32_t addr_t;
typedef uint32_t str_t;

/* global variables */
extern const char meg4ver[], *copyright[3];
extern char meg4_title[64], meg4_author[64], meg4_pro, meg4_takescreenshot;
extern uint8_t *meg4_font, *meg4_defwaves;
extern meg4_pixbuf_t meg4_icons, meg4_edicons;
extern meg4_t meg4;
extern char meg4_mmio_assert[offsetof(meg4_mmio_t,sprites) != 0x10000 ||  sizeof(meg4_mmio_t) != 0x20000 ||
    sizeof(meg4.mmio) + 0x10000 + sizeof(meg4.data) != MEG4_MEM_LIMIT ? -1 : 1];

/* main.c - these have to be implemented in the platform's main.c. Most of them are provided by common.h */
void main_log(int lvl, const char* fmt, ...);
uint8_t* main_readfile(char *file, int *size);
int  main_writefile(char *file, uint8_t *buf, int size);
void main_openfile(void);
int  main_savefile(const char *name, uint8_t *buf, int len);
char **main_getfloppies(void);
uint8_t *main_cfgload(char *cfg, int *len);
int  main_cfgsave(char *cfg, uint8_t *buf, int len);
char *main_getclipboard(void);
void main_setclipboard(char *str);
void main_osk_show(void);
void main_osk_hide(void);
void main_fullscreen(void);
void main_focus(void);

/* meg4.c - top level MEG-4 API */
void meg4_poweron(char *region);
void meg4_poweroff(void);
void meg4_reset(void);
void meg4_run(void);
#ifndef NOEDITORS
int  meg4_load(uint8_t *buf, int len);
uint8_t *meg4_save(int *len);
void meg4_insert(char *name, uint8_t *buf, int len);
void meg4_switchmode(int mode);
#endif

/* cpu.c - bytecode VM */
void   cpu_init(void);
void   cpu_free(void);
void   cpu_getlang(void);
int    cpu_compile(void);
void   cpu_run(void);
addr_t cpu_pushi(int value);
addr_t cpu_pushf(float value);
int    cpu_popi(void);
float  cpu_popf(void);
int    cpu_topi(uint32_t offs);
float  cpu_topf(uint32_t offs);
void   cpu_fetch(void);

/* math.c - mathematical functions */
uint32_t meg4_api_rand(void);
float meg4_api_rnd(void);
float meg4_api_float(int val);
int   meg4_api_int(float val);
float meg4_api_floor(float val);
float meg4_api_ceil(float val);
float meg4_api_sgn(float val);
float meg4_api_abs(float val);
float meg4_api_exp(float val);
float meg4_api_log(float val);
float meg4_api_pow(float val, float exp);
float meg4_api_sqrt(float val);
float meg4_api_rsqrt(float val);
float meg4_api_pi(void);
float meg4_api_cos(uint16_t deg);
float meg4_api_sin(uint16_t deg);
float meg4_api_tan(uint16_t deg);
uint16_t meg4_api_acos(float val);
uint16_t meg4_api_asin(float val);
uint16_t meg4_api_atan(float val);
uint16_t meg4_api_atan2(float y, float x);

/* mem.c - memory management unit */
int   meg4_snprintf(char *dst, int len, char *fmt);
uint8_t meg4_api_inb(addr_t src);
uint16_t meg4_api_inw(addr_t src);
uint32_t meg4_api_ini(addr_t src);
void  meg4_api_outb(addr_t dst, uint8_t value);
void  meg4_api_outw(addr_t dst, uint16_t value);
void  meg4_api_outi(addr_t dst, uint32_t value);
int   meg4_api_memsave(uint8_t overlay, addr_t src, uint32_t size);
int   meg4_api_memload(addr_t dst, uint8_t overlay, uint32_t maxsize);
void  meg4_api_memcpy(addr_t dst, addr_t src, uint32_t len);
void  meg4_api_memset(addr_t dst, uint8_t value, uint32_t len);
int   meg4_api_memcmp(addr_t addr0, addr_t addr1, uint32_t len);
int   meg4_api_deflate(addr_t dst, addr_t src, uint32_t len);
int   meg4_api_inflate(addr_t dst, addr_t src, uint32_t len);
float meg4_api_time(void);
uint32_t meg4_api_now(void);
int   meg4_api_atoi(str_t src);
str_t meg4_api_itoa(int value);
float meg4_api_val(str_t src);
str_t meg4_api_str(float value);
str_t meg4_api_sprintf(str_t fmt, ...);
int   meg4_api_strlen(str_t src);
int   meg4_api_mblen(str_t src);
addr_t meg4_api_malloc(uint32_t size);
addr_t meg4_api_realloc(addr_t addr, uint32_t size);
int   meg4_api_free(addr_t addr);

/* cons.c - console, high level input / output */
char *meg4_utf8(char *str, uint32_t *out);
void meg4_conrst(void);
void meg4_putc(uint32_t c);
void meg4_api_putc(uint32_t chr);
void meg4_api_printf(str_t fmt, ...);
uint32_t meg4_api_getc(void);
str_t meg4_api_gets(void);
void meg4_api_trace(str_t fmt, ...);
void meg4_api_delay(uint16_t);
void meg4_api_exit(void);

/* inp.c - low level user input functions (for output, see dsp and gpu) */
void meg4_clrpad(int pad, int btn);
void meg4_setpad(int pad, int btn);
int  meg4_api_getpad(int pad, int btn);
int  meg4_api_prspad(int pad, int btn);
int  meg4_api_relpad(int pad, int btn);
void meg4_setptr(int x, int y);
void meg4_setscr(int u, int d, int l, int r);
void meg4_clrbtn(int btn);
void meg4_setbtn(int btn);
int  meg4_api_getbtn(int btn);
int  meg4_api_getclk(int btn);
void meg4_clrkey(int sc);
void meg4_setkey(int sc);
int  meg4_api_getkey(int sc);
void meg4_pushkey(char *key);
uint32_t meg4_api_popkey(void);
int  meg4_api_pendkey(void);
int  meg4_api_lenkey(uint32_t key);
int  meg4_api_speckey(uint32_t key);

/* dsp.c - sounds and music functionality */
void dsp_init(void);
void dsp_free(void);
void dsp_reset(void);
void dsp_genwave(int idx, int wave);
void meg4_audiofeed(float *buf, int len);
void meg4_api_sfx(uint8_t sfx, uint8_t channel, uint8_t volume);
void meg4_api_music(uint8_t track, uint16_t row, uint8_t volume);
#ifndef NOEDITORS
void meg4_playnote(uint8_t *note, uint8_t volume);
#endif

/* gpu.c - graphics and screen output */
void meg4_getscreen(void);
void meg4_redraw(uint32_t *dst, int dw, int dh, int dp);
void meg4_screenshot(uint32_t *dst, int dx, int dy, int dp);
void meg4_recalcfont(int s, int e);
void meg4_recalcmipmap(void);
uint8_t meg4_palidx(uint8_t *rgba);
void meg4_spr(uint32_t *dst, int dp, int x, int y, int sprite, int scale, int type);
void meg4_blit(uint32_t *dst, int x, int y, int dp, int w, int h, uint32_t *src, int sx, int sy, int sp, int t);
#ifndef NOEDITORS
void meg4_blitd(uint32_t *dst, int x, int y, int dp, int w1, int w2, int h, uint32_t *src, int sx, int sy, int sw, int sh, int sp);
void meg4_box(uint32_t *dst, int dw, int dh, int dp, int x, int y, int w, int h, uint32_t l, uint32_t b, uint32_t d,
    uint32_t s, int t, int o, int r, int c);
void meg4_chk(uint32_t *dst, int dw, int dh, int dp, int x, int y, uint32_t l, uint32_t d, int c);
void meg4_number(uint32_t *dst, int dw, int dh, int dp, int x, int y, uint32_t c, int n);
void meg4_char(uint32_t *dst, int dp, int dx, int dy, uint32_t c, int type, uint8_t *font, int idx);
#endif
int  meg4_text(uint32_t *dst, int dx, int dy, int dp, uint32_t color, uint32_t shadow, int type, uint8_t *font, char *str);
int  meg4_width(uint8_t *font, int8_t type, char *str, char *end);
void meg4_remap(uint8_t *replace);
void meg4_api_cls(uint8_t palidx);
uint32_t meg4_api_cget(uint16_t x, uint16_t y);
uint8_t  meg4_api_pget(uint16_t x, uint16_t y);
void meg4_api_pset(uint8_t palidx, uint16_t x, uint16_t y);
uint16_t meg4_api_width(int8_t type, str_t str);
void meg4_api_text(uint8_t palidx, int16_t x, int16_t y, int8_t type, uint8_t shadowidx, uint8_t shadowalpha, str_t str);
void meg4_api_line(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void meg4_api_qbez(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t cx, int16_t cy);
void meg4_api_cbez(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1);
void meg4_api_tri(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void meg4_api_ftri(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void meg4_api_tri2d(uint8_t pi0, int16_t x0, int16_t y0, uint8_t pi1, int16_t x1, int16_t y1, uint8_t pi2, int16_t x2, int16_t y2);
void meg4_api_tri3d(uint8_t pi0, int16_t x0, int16_t y0, int16_t z0, uint8_t pi1, int16_t x1, int16_t y1, int16_t z1,
    uint8_t pi2, int16_t x2, int16_t y2, int16_t z2);
void meg4_api_rect(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void meg4_api_frect(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void meg4_api_circ(uint8_t palidx, int16_t x, int16_t y, uint16_t r);
void meg4_api_fcirc(uint8_t palidx, int16_t x, int16_t y, uint16_t r);
void meg4_api_ellip(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void meg4_api_fellip(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void meg4_api_move(int16_t x, int16_t y, uint16_t deg);
void meg4_api_left(uint16_t deg);
void meg4_api_right(uint16_t deg);
void meg4_api_up(void);
void meg4_api_down(void);
void meg4_api_color(uint8_t palidx);
void meg4_api_forw(uint16_t cnt);
void meg4_api_back(uint16_t cnt);
void meg4_api_spr(int16_t x, int16_t y, uint16_t sprite, uint8_t sw, uint8_t sh, int8_t scale, uint8_t type);
void meg4_api_dlg(int16_t x, int16_t y, uint16_t w, uint16_t h, int8_t scale,
    uint16_t tl, uint16_t tm, uint16_t tr,
    uint16_t ml, uint16_t bg, uint16_t mr,
    uint16_t bl, uint16_t bm, uint16_t br);
void meg4_api_stext(int16_t x, int16_t y, uint16_t fs, uint16_t fu, uint8_t sw, uint8_t sh, int8_t scale, str_t str);
void meg4_api_remap(addr_t replace);
uint16_t meg4_api_mget(uint16_t mx, uint16_t my);
void meg4_api_mset(uint16_t mx, uint16_t my, uint16_t sprite);
void meg4_api_map(int16_t x, int16_t y, uint16_t mx, uint16_t my, uint16_t mw, uint16_t mh, int8_t scale);
void meg4_api_maze(uint16_t mx, uint16_t my, uint16_t mw, uint16_t mh, uint8_t scale,
    uint16_t sky, uint16_t grd, uint16_t door, uint16_t wall, uint16_t obj, uint8_t numnpc, addr_t npc);

#ifdef  __cplusplus
}
#endif

#endif /* _MEG4_H_ */
