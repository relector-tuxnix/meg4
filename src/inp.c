/*
 * meg4/inp.c
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
 * @brief User input (keyboard, mouse, gamepad) functions
 * @chapter Input
 *
 */

#include "meg4.h"

/* scratch area for complex inputs */
char meg4_kbdtmpbuf[8], meg4_kbdtmpsht, meg4_kbdtmpidx, meg4_emptyaltgr;
static char meg4_kc[16], meg4_padbtnsp[sizeof(meg4.mmio.padbtns)] = { 0 }, meg4_padbtnsr[sizeof(meg4.mmio.padbtns)] = { 0 };
extern int load_list;

/**
 * Special key mappings for complex inputs
 * IMPORTANT NOTE: "key" must be totally unique, they must differ in at least one character which isn't the terminating zero
 */
char *meg4_kbdcompose[] = {
 /* key         character   shift+                  key         character   shift+ */
    "ss",       "ß",        "ẞ",                    "a`",       "à",        "À",
    "a'",       "á",        "Á",                    "a^",       "â",        "Â",
    "a~",       "ã",        "Ã",                    "a:",       "ä",        "Ä",
    "ao",       "å",        "Å",                    "ae",       "æ",        "Æ",
    "e`",       "è",        "È",                    "e'",       "é",        "É",
    "e^",       "ê",        "Ê",                    "e:",       "ë",        "Ë",
    "i`",       "ì",        "Ì",                    "i'",       "í",        "Í",
    "i^",       "î",        "Î",                    "i~",       "ĩ",        "Ĩ",
    "i:",       "ï",        "Ï",                    "ij",       "ĳ",        "Ĳ",
    "o`",       "ò",        "Ò",                    "o'",       "ó",        "Ó",
    "o^",       "ô",        "Ô",                    "o~",       "õ",        "Õ",
    "o:",       "ö",        "Ö",                    "o\"",      "ő",        "Ő",
    "oe",       "œ",        "Œ",                    "u`",       "ù",        "Ù",
    "u'",       "ú",        "Ú",                    "u^",       "û",        "Û",
    "u:",       "ü",        "Ü",                    "u\"",      "ű",        "Ű",
    "n'",       "ń",        "Ń",                    "n~",       "ñ",        "Ñ",
    "c'",       "ć",        "Ć",                    "c,",       "ç",        "Ç",
    "y'",       "ý",        "Ý",                    "y^",       "ŷ",        "Ŷ",
    "y:",       "ÿ",        "Ÿ",
    /* TODO: add all variants */
    NULL
};
char *meg4_kbdemoji[] = {
 /* key         character   shift+ */
    "k",        "⌨",        NULL,       /* 02328 KEYBOARD */
    "p",        "🎮",        NULL,       /* 1F3AE VIDEO GAME (should look like a gamepad) */
    "m",        "🖱",        NULL,       /* 1F5B1 THREE BUTTON MOUSE */
    "n",        "⏎",        NULL,       /* 023CE RETURN SYMBOL (enter, newline) */
    "e",        "⏏",        NULL,       /* 023CF EJECT SYMBOL (escape) */
    "a",        "Ⓐ",        NULL,       /* 024B6 CIRCLED LATIN CAPITAL LETTER A */
    "b",        "Ⓑ",        NULL,       /* 024B7 CIRCLED LATIN CAPITAL LETTER B */
    "x",        "Ⓧ",        NULL,       /* 024CD CIRCLED LATIN CAPITAL LETTER X */
    "y",        "Ⓨ",        NULL,       /* 024CE CIRCLED LATIN CAPITAL LETTER Y */
    "u",        "△",        NULL,       /* 025B3 WHITE UP-POINTING TRIANGLE */
    "r",        "▷",        NULL,       /* 025B7 WHITE RIGHT-POINTING TRIANGLE */
    "d",        "▽",        NULL,       /* 025BD WHITE DOWN-POINTING TRIANGLE */
    "l",        "◁",        NULL,       /* 025C1 WHITE LEFT-POINTING TRIANGLE */
    "<3",       "❤",        NULL,       /* 02764 HEART */
    ":)",       "😀",        NULL,       /* GRINNING FACE */
    ":~)",      "😂",        NULL,       /* FACE WITH TEARS OF JOY */
    ":d",       "😃",        NULL,       /* SMILING FACE WITH OPEN MOUTH (supposed to look like ":D", but lowercase only keys) */
    ">d",       "😆",        NULL,       /* SMILING FACE WITH OPEN MOUTH AND TIGHTLY-CLOSED EYES */
    ";)",       "😉",        NULL,       /* WINKING FACE */
    "<)",       "😌",        NULL,       /* RELIEVED FACE */
    "8)",       "😎",        NULL,       /* SMILING FACE WITH SUNGLASSES */
    ":|",       "😐",        NULL,       /* NEUTRAL FACE */
    ":/",       "😕",        NULL,       /* CONFUSED FACE */
    ":s",       "😖",        NULL,       /* CONFOUNDED FACE */
    ":*",       "😘",        NULL,       /* FACE THROWING A KISS */
    ":p",       "😛",        NULL,       /* FACE WITH STUCK-OUT TONGUE */
    ";p",       "😜",        NULL,       /* FACE WITH STUCK-OUT TONGUE AND WINKING EYE */
    ">p",       "😝",        NULL,       /* FACE WITH STUCK-OUT TONGUE AND TIGHTLY-CLOSED EYES */
    ":(",       "😞",        NULL,       /* DISAPPOINTED FACE */
    "<(",       "😟",        NULL,       /* WORRIED FACE */
    ">(",       "😠",        NULL,       /* ANGRY FACE */
    ":~(",      "😭",        NULL,       /* LOUDLY CRYING FACE */
    ":o",       "😮",        NULL,       /* FACE WITH OPEN MOUTH */
    "<o",       "😯",        NULL,       /* HUSHED FACE */
    NULL
};
char *meg4_kbdkata[] = {
 /* key         character   shift+                  key         character   shift+ */
    ",",        "、",        "゛",                    ".",        "。",        "゜",
    "[",        "「",        NULL,                   "[",        "」",        NULL,
    "a",        "ァ",        "ア",                    "ha",       "ㇵ",        "ハ",
    "i",        "ィ",        "イ",                    "hi",       "ㇶ",        "ヒ",
    "u",        "ゥ",        "ウ",                    "hu",       "ㇷ",        "フ",
    "e",        "ェ",        "エ",                    "he",       "ㇸ",        "ヘ",
    "o",        "ォ",        "オ",                    "ho",       "ㇹ",        "ホ",
    "ka",       "ヵ",        "カ",                    "ma",       "マ",        "マ",
    "ki",       "キ",        "キ",                    "mi",       "ミ",        "ミ",
    "ku",       "ㇰ",        "ク",                    "mu",       "ㇺ",        "ム",
    "ke",       "ヶ",        "ケ",                    "me",       "メ",        "メ",
    "ko",       "コ",        "コ",                    "mo",       "モ",        "モ",
    "sa",       "サ",        "サ",                    "ya",       "ャ",        "ヤ",
    "si",       "ㇱ",        "シ",                    "yu",       "ュ",        "ユ",
    "su",       "ㇲ",        "ス",                    "yo",       "ョ",        "ヨ",
    "se",       "セ",        "セ",                    "ra",       "ㇻ",        "ラ",
    "so",       "ソ",        "ソ",                    "ri",       "ㇼ",        "リ",
    "ta",       "タ",        "タ",                    "ru",       "ㇽ",        "ル",
    "ti",       "チ",        "チ",                    "re",       "ㇾ",        "レ",
    "tu",       "ッ",        "ツ",                    "ro",       "ㇿ",        "ロ",
    "te",       "テ",        "テ",                    "wa",       "ヮ",        "ワ",
    "to",       "ト",        "ト",                    "wo",       "ヲ",        "ヲ",
    "na",       "ナ",        "ナ",                    "nn",       "ン",        "ン",
    "ni",       "ニ",        "ニ",                    "nu",       "ㇴ",        "ヌ",
    "ne",       "ネ",        "ネ",                    "no",       "ノ",        "ノ",
    NULL
};
char *meg4_kbdhira[] = {
 /* key         character   shift+                  key         character   shift+ */
    ",",        "、",        "゛",                    ".",        "。",        "゜",
    "[",        "「",        NULL,                   "[",        "」",        NULL,
    "a",        "ぁ",        "あ",                    "ha",       "は",        "は",
    "i",        "ぃ",        "い",                    "hi",       "ひ",        "ひ",
    "u",        "ぅ",        "う",                    "hu",       "ふ",        "ふ",
    "e",        "ぇ",        "え",                    "he",       "へ",        "へ",
    "o",        "ぉ",        "お",                    "ho",       "ほ",        "ほ",
    "ka",       "ゕ",        "か",                    "ma",       "ま",        "ま",
    "ki",       "き",        "き",                    "mi",       "み",        "み",
    "ku",       "ㇰ",        "く",                    "mu",       "む",        "む",
    "ke",       "ゖ",        "け",                    "me",       "め",        "め",
    "ko",       "こ",        "こ",                    "mo",       "も",        "も",
    "sa",       "さ",        "さ",                    "ya",       "ゃ",        "や",
    "si",       "し",        "し",                    "yu",       "ゅ",        "ゆ",
    "su",       "ㇲ",        "す",                    "yo",       "ょ",        "よ",
    "se",       "せ",        "せ",                    "ra",       "ら",        "ら",
    "so",       "そ",        "そ",                    "ri",       "り",        "り",
    "ta",       "た",        "た",                    "ru",       "る",        "る",
    "ti",       "ち",        "ち",                    "re",       "れ",        "れ",
    "tu",       "っ",        "つ",                    "ro",       "ろ",        "ろ",
    "te",       "て",        "て",                    "wa",       "ゎ",        "わ",
    "to",       "と",        "と",                    "wo",       "を",        "を",
    "na",       "な",        "な",                    "nn",       "ん",        "ん",
    "ni",       "に",        "に",                    "nu",       "ぬ",        "ぬ",
    "ne",       "ね",        "ね",                    "no",       "の",        "の",
    NULL
};
char **meg4_kbdcomplex[] = { meg4_kbdcompose, meg4_kbdemoji, meg4_kbdkata, meg4_kbdhira, NULL, NULL, NULL };

/**
 * Clear gamepad button
 */
void meg4_clrpad(int pad, int btn)
{
    int i, j;

    if(pad < 0 || pad > 3) return;
    meg4.mmio.padbtns[pad] &= ~btn;
    if(!pad) {
        for(j = 1, i = 0; i < 8; i++, j <<= 1)
            if(!(btn & j))
                meg4_clrkey(meg4.mmio.padkeys[i]);
    }
}

/**
 * Set gamepad button
 */
void meg4_setpad(int pad, int btn)
{
    if(pad < 0 || pad > 3) return;
    meg4.mmio.padbtns[pad] |= btn;
    if(!pad)
        switch(btn) {
            case MEG4_BTN_L: if(meg4.mmio.padkeys[0]) meg4_setkey(meg4.mmio.padkeys[0]); break;
            case MEG4_BTN_U: if(meg4.mmio.padkeys[1]) meg4_setkey(meg4.mmio.padkeys[1]); break;
            case MEG4_BTN_R: if(meg4.mmio.padkeys[2]) meg4_setkey(meg4.mmio.padkeys[2]); break;
            case MEG4_BTN_D: if(meg4.mmio.padkeys[3]) meg4_setkey(meg4.mmio.padkeys[3]); break;
            case MEG4_BTN_A: if(meg4.mmio.padkeys[4]) meg4_setkey(meg4.mmio.padkeys[4]); break;
            case MEG4_BTN_B: if(meg4.mmio.padkeys[5]) meg4_setkey(meg4.mmio.padkeys[5]); break;
            case MEG4_BTN_X: if(meg4.mmio.padkeys[6]) meg4_setkey(meg4.mmio.padkeys[6]); break;
            case MEG4_BTN_Y: if(meg4.mmio.padkeys[7]) meg4_setkey(meg4.mmio.padkeys[7]); break;
        }
}

/**
 * Gets the current state of a gamepad button.
 * @param pad gamepad index, 0 to 3
 * @param btn one of the [gamepad] buttons, `BTN_`
 * @return Zero if not pressed, non-zero if pressed.
 * @see [prspad], [relpad], [getbtn], [getclk], [getkey]
 */
int meg4_api_getpad(int pad, int btn)
{
    return (pad < 0 || pad > 3 || btn < 0 || btn > 255) ? 0 : meg4.mmio.padbtns[pad] & btn;
}

/**
 * Returns true if gamepad button was pressed since last call.
 * @param pad gamepad index, 0 to 3
 * @param btn one of the [gamepad] buttons, `BTN_`
 * @return Zero if not pressed, non-zero if pressed.
 * @see [relpad], [getpad], [getbtn], [getclk], [getkey]
 */
int meg4_api_prspad(int pad, int btn)
{
    int ret = (pad < 0 || pad > 3 || btn < 0 || btn > 255) ? 0 :
        (meg4.mmio.padbtns[pad] & btn) && !(meg4_padbtnsp[pad] & btn);
    memcpy(meg4_padbtnsp, meg4.mmio.padbtns, sizeof(meg4.mmio.padbtns));
    return ret;
}

/**
 * Returns true if gamepad button was released since last call.
 * @param pad gamepad index, 0 to 3
 * @param btn one of the [gamepad] buttons, `BTN_`
 * @return Zero if wasn't released, non-zero if released.
 * @see [prspad], [getpad], [getbtn], [getclk], [getkey]
 */
int meg4_api_relpad(int pad, int btn)
{
    int ret = (pad < 0 || pad > 3 || btn < 0 || btn > 255) ? 0 :
        !(meg4.mmio.padbtns[pad] & btn) && (meg4_padbtnsr[pad] & btn);
    memcpy(meg4_padbtnsr, meg4.mmio.padbtns, sizeof(meg4.mmio.padbtns));
    return ret;
}

/**
 * Set mouse pointer coordinates
 */
void meg4_setptr(int x, int y)
{
    if(meg4.mmio.ptrx != x || meg4.mmio.ptry != y)
        meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
    meg4.mmio.ptrx = x;
    meg4.mmio.ptry = y;
}

/**
 * Set mouse scroll "buttons"
 */
void meg4_setscr(int u, int d, int l, int r)
{
    if(meg4.mmio.ptrspr == MEG4_PTR_ERR) meg4.mmio.ptrspr = MEG4_PTR_NORM;
    meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
    meg4.mmio.ptrbtn |= (u ? MEG4_SCR_U : 0) | (d ? MEG4_SCR_D : 0) | (l ? MEG4_SCR_L : 0) | (r ? MEG4_SCR_R : 0);
}

/**
 * Clear mouse buttons
 */
void meg4_clrbtn(int btn)
{
    if(meg4.mmio.ptrspr == MEG4_PTR_ERR) meg4.mmio.ptrspr = MEG4_PTR_NORM;
    meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
    meg4.mmio.ptrbtn &= ~btn;
}

/**
 * Set mouse buttons
 */
void meg4_setbtn(int btn)
{
    if(meg4.mmio.ptrspr == MEG4_PTR_ERR) meg4.mmio.ptrspr = MEG4_PTR_NORM;
    meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
    meg4.mmio.ptrbtn |= btn;
}

/**
 * Gets the mouse button's state.
 * @param btn one of the [pointer] buttons, `BTN_` or `SCR_`
 * @return Zero if not pressed, non-zero if pressed.
 * @see [prspad], [relpad], [getpad], [getclk], [getkey]
 */
int meg4_api_getbtn(int btn)
{
    int ret = (btn < 0 || btn > 255) ? 0 : meg4.mmio.ptrbtn & btn;
    meg4.mmio.ptrbtn &= ~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R);
    return ret;
}

/**
 * Gets mouse button clicking.
 * @param btn one of the [pointer] buttons, `BTN_`
 * @return Zero if not clciked, non-zero if clicked.
 * @see [prspad], [relpad], [getpad], [getbtn], [getkey]
 */
int meg4_api_getclk(int btn)
{
    int ret = (btn < 0 || btn > 255) || ((meg4.mmio.ptrbtn >> 8) & btn) || !(meg4.mmio.ptrbtn & btn) ? 0 : meg4.mmio.ptrbtn & btn;
    meg4.mmio.ptrbtn &= (~(MEG4_SCR_U | MEG4_SCR_D | MEG4_SCR_L | MEG4_SCR_R) & 0xff);
    meg4.mmio.ptrbtn |= (meg4.mmio.ptrbtn << 8);
    return ret;
}

/**
 * Clear a pressed key state for a scancode
 */
void meg4_clrkey(int sc)
{
    int i;

    if(sc < 1 || sc >= MEG4_NUM_KEY || !(meg4.mmio.kbdkeys[sc >> 3] & (1 << (sc & 7)))) return;
    meg4.mmio.kbdkeys[sc >> 3] &= ~(1 << (sc & 7));
    for(i = 0; i < 8; i++)
        if(meg4.mmio.padkeys[i] && meg4.mmio.padkeys[i] == sc)
            meg4.mmio.padbtns[0] &= ~(1 << i);
    /* an empty AltGr press and release (no combination with other keys) acts as a Compose key */
    if(sc == MEG4_KEY_RALT && meg4_emptyaltgr)
        meg4_pushkey("\002");
    meg4_emptyaltgr = 0;
}

/**
 * Set a pressed key state for a scancode
 */
void meg4_setkey(int sc)
{
    int i;

    meg4_emptyaltgr = 0;
    if(sc < 0 || sc >= MEG4_NUM_KEY) return;
    if(!(meg4.mmio.kbdkeys[sc >> 3] & (1 << (sc & 7)))) {
        if(meg4_api_getkey(MEG4_KEY_LALT) || meg4_api_getkey(MEG4_KEY_RALT)) {
            switch(sc) {
                case MEG4_KEY_H: meg4_pushkey("#"); break;          /* U+00023 hashmark */
                case MEG4_KEY_S: meg4_pushkey("$"); break;          /* U+00024 dollar */
                case MEG4_KEY_A: meg4_pushkey("&"); break;          /* U+00026 ampersand */
                case MEG4_KEY_L: meg4_pushkey("£"); break;          /* U+000A3 pound */
                case MEG4_KEY_Y: meg4_pushkey("¥"); break;          /* U+000A5 yen */
                case MEG4_KEY_E: meg4_pushkey("€"); break;          /* U+020AC euro */
                case MEG4_KEY_R: meg4_pushkey("₹"); break;          /* U+020B9 rupee */
                case MEG4_KEY_B: meg4_pushkey("₿"); break;          /* U+020BF bitcoin */
                case MEG4_KEY_N: meg4_pushkey("元"); break;          /* U+05143 yuan */
                case MEG4_KEY_U: meg4_pushkey("\001"); break;       /* unicode input mode */
                case MEG4_KEY_SPACE: meg4_pushkey("\002"); break;   /* compose */
                case MEG4_KEY_I: meg4_pushkey("\003"); break;       /* icon input (emoji) */
                case MEG4_KEY_K: meg4_pushkey("\004"); break;       /* katakana input */
                case MEG4_KEY_J: meg4_pushkey("\005"); break;       /* hiragana input */
#ifndef NOEDITORS
                /* fallback function keys, Alt + 1..0 */
                case MEG4_KEY_1: case MEG4_KEY_2: case MEG4_KEY_3: case MEG4_KEY_4: case MEG4_KEY_5:
                case MEG4_KEY_6: case MEG4_KEY_7: case MEG4_KEY_8: case MEG4_KEY_9: case MEG4_KEY_0:
                    sc = sc - MEG4_KEY_1 + MEG4_KEY_F1;
                    goto normal;
#endif
            }
        } else
        if(meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL)) {
            switch(sc) {
                case MEG4_KEY_A: meg4_pushkey("Sel"); break;
                case MEG4_KEY_I: meg4_pushkey("Inv"); break;
                case MEG4_KEY_X: meg4_pushkey("Cut"); break;
                case MEG4_KEY_C: meg4_pushkey("Cpy"); break;
                case MEG4_KEY_V: meg4_pushkey("Pst"); break;
                case MEG4_KEY_F: meg4_pushkey("Find"); break;
                case MEG4_KEY_G: meg4_pushkey("Next"); break;
                case MEG4_KEY_H: meg4_pushkey("Repl"); break;
                case MEG4_KEY_J: meg4_pushkey("Line"); break;
                case MEG4_KEY_D: meg4_pushkey("Func"); break;
                case MEG4_KEY_N: meg4_pushkey("BMrk"); break;
                case MEG4_KEY_B: meg4_pushkey("TgBM"); break;
                case MEG4_KEY_Z: meg4_pushkey("Undo"); break;
                case MEG4_KEY_Y: meg4_pushkey("Redo"); break;
                case MEG4_KEY_L:
#ifndef NOEDITORS
                    meg4_switchmode(MEG4_MODE_LOAD);
                    /* break and not return, because we want kbdkeys to be set */
#endif
                break;
                case MEG4_KEY_S:
#ifndef NOEDITORS
                    if(meg4.mode != MEG4_MODE_LOAD && meg4.mode != MEG4_MODE_SAVE)
                        meg4_switchmode(MEG4_MODE_SAVE);
#endif
                return;
                case MEG4_KEY_R:
#ifndef NOEDITORS
                    if(meg4.mode >= MEG4_MODE_HELP)
                        meg4_switchmode(!cpu_compile() ? MEG4_MODE_CODE : MEG4_MODE_GAME);
#endif
                return;
            }
        } else {
#ifndef NOEDITORS
normal:
#endif
            switch(sc) {
                case MEG4_KEY_INT1:
                case MEG4_KEY_LSUPER:
                case MEG4_KEY_RSUPER: meg4_pushkey("\001"); break;  /* use the "Win" (or GUI) key as a UNICODE input key */
                case MEG4_KEY_SELECT: meg4_pushkey("Sel"); break;   /* some keyboards (like VT) have separate keys for these */
                case MEG4_KEY_CUT: meg4_pushkey("Cut"); break;
                case MEG4_KEY_COPY: meg4_pushkey("Cpy"); break;
                case MEG4_KEY_PASTE: meg4_pushkey("Pst"); break;
                case MEG4_KEY_FIND: meg4_pushkey("Find"); break;
                case MEG4_KEY_AGAIN: meg4_pushkey("Redo"); break;
                case MEG4_KEY_UNDO: meg4_pushkey("Undo"); break;
                case MEG4_KEY_INT3: meg4_pushkey("¥"); break;       /* on Japanese keyboards */
                case MEG4_KEY_LNG3: meg4_pushkey("\004"); break;
                case MEG4_KEY_LNG4: meg4_pushkey("\005"); break;
                case MEG4_KEY_RALT: meg4_emptyaltgr = 1; break;
                case MEG4_KEY_F11: main_fullscreen(); return;
#ifndef NOEDITORS
                /* code editor has its own help shower depending on function context */
                case MEG4_KEY_F1: if(meg4.mode != MEG4_MODE_CODE) { meg4_switchmode(MEG4_MODE_HELP); return; } break;
                case MEG4_KEY_F2: meg4_switchmode(MEG4_MODE_CODE); return;
                case MEG4_KEY_F3: meg4_switchmode(MEG4_MODE_SPRITE); return;
                case MEG4_KEY_F4: meg4_switchmode(MEG4_MODE_MAP); return;
                case MEG4_KEY_F5: meg4_switchmode(MEG4_MODE_FONT); return;
                case MEG4_KEY_F6: meg4_switchmode(MEG4_MODE_SOUND); return;
                case MEG4_KEY_F7: meg4_switchmode(MEG4_MODE_MUSIC); return;
                case MEG4_KEY_F8: meg4_switchmode(MEG4_MODE_OVERLAY); return;
                case MEG4_KEY_F9: meg4_switchmode(MEG4_MODE_VISUAL); return;
                case MEG4_KEY_F10: meg4_switchmode(MEG4_MODE_DEBUG); return;
                case MEG4_KEY_F12: meg4_takescreenshot = 1; return;
#endif
            }
        }
    }
    meg4.mmio.kbdkeys[sc >> 3] |= (1 << (sc & 7));
    for(i = 0; i < 8; i++)
        if(meg4.mmio.padkeys[i] && meg4.mmio.padkeys[i] == sc) {
            meg4.mmio.padbtns[0] |= (1 << i);
            memmove(&meg4_kc[1], &meg4_kc[0], 15);
            meg4_kc[0] = i;
            if(!memcmp(meg4_kc + 1, "\005\002\000\002\000\003\003\001\001", 9)) {
                if(i == 4) meg4.mmio.kbdkeys[0] |= 1; else if(i == 7) meg4.mode = MEG4_MODE_GURU;
            }
        }
}

/**
 * Gets the current state of a key.
 * @param sc scancode, 1 to 144, see [keyboard]
 * @return Zero if not pressed, non-zero if pressed.
 * @see [prspad], [relpad], [getpad], [getbtn], [getclk]
 */
int meg4_api_getkey(int sc)
{
    if(sc < 0 || sc >= MEG4_NUM_KEY) return 0;
    return meg4.mmio.kbdkeys[sc >> 3] & (1 << (sc & 7)) ? 1 : 0;
}

/**
 * Handle special combinations and push an UTF-8 key into the keyboard queue
 */
void meg4_pushkey(char *key)
{
    uint32_t unicode;
    char *s = key, tmp[4];
    char **map, **found;
    int i, p;

    switch(s[0]) {
        case 0:
        case 6: case 7: /* reserved for future use, should new complex input maps needed */
            meg4.kbdmode = 0;
        break;
        case 1: case 2: case 3: case 4: case 5:
            meg4.kbdmode = meg4.kbdmode == s[0] ? 0 : s[0];
            memset(meg4_kbdtmpbuf, 0, sizeof(meg4_kbdtmpbuf));
            meg4_kbdtmpsht = meg4_kbdtmpidx = 0;
        break;
        case 27:
            if(meg4.kbdmode) meg4.kbdmode = 0;
#ifndef NOEDITORS
            else if(meg4.mode == MEG4_MODE_GAME)
                meg4_switchmode(meg4.code && meg4.code_len && meg4.pc ? MEG4_MODE_DEBUG : MEG4_MODE_CODE);
            else if(meg4.mode == MEG4_MODE_LOAD && !load_list)
                meg4_switchmode(MEG4_MODE_CODE);
            else goto pushkey;
#endif
        break;
        default:
            switch(meg4.kbdmode) {
                case 0: /* normal mode, push key to the queue */
pushkey:            if(((meg4.mmio.kbdhead + 1) & 15) != meg4.mmio.kbdtail) {
                        memcpy(&meg4.mmio.kbd[(int)meg4.mmio.kbdhead], s, 4);
                        meg4.mmio.kbdhead = ((meg4.mmio.kbdhead + 1) & 15);
                    }
                break;
                case 1: /* unicode codepoint input */
                    if(s[0] == 8 || !memcmp(s, "Del", 4)) {
                        if(meg4_kbdtmpidx > 0) meg4_kbdtmpbuf[(int)--meg4_kbdtmpidx] = 0;
                    } else
                    if((s[0] >= '0' && s[0] <= '9') || (s[0] >= 'a' && s[0] <= 'f') || (s[0] >= 'A' && s[0] <= 'F')) {
                        if(s[0] >= 'a' && s[0] <= 'f') s[0] -= 'a' - 'A';
                        meg4_kbdtmpbuf[(int)meg4_kbdtmpidx++] = s[0];
                    }
                    if(meg4_kbdtmpidx == 5 || s[0] == '\n') {
                        meg4.kbdmode = 0;
                        for(unicode = i = 0; meg4_kbdtmpbuf[i]; i++) {
                            unicode <<= 4;
                            if(meg4_kbdtmpbuf[i] >= '0' && meg4_kbdtmpbuf[i] <= '9') unicode += meg4_kbdtmpbuf[i] - '0';
                            if(meg4_kbdtmpbuf[i] >= 'A' && meg4_kbdtmpbuf[i] <= 'F') unicode += meg4_kbdtmpbuf[i] - 'A' + 10;
                        }
                        if(unicode >= ' ') {
                            memset(tmp, 0, sizeof(tmp));
                            if(unicode<0x80) { tmp[0]=unicode; } else
                            if(unicode<0x800) { tmp[0]=((unicode>>6)&0x1F)|0xC0; tmp[1]=(unicode&0x3F)|0x80; } else
                            if(unicode<0x10000) { tmp[0]=((unicode>>12)&0x0F)|0xE0; tmp[1]=((unicode>>6)&0x3F)|0x80; tmp[2]=(unicode&0x3F)|0x80; }
                            else { tmp[0]=((unicode>>18)&0x07)|0xF0; tmp[1]=((unicode>>12)&0x3F)|0x80; tmp[2]=((unicode>>6)&0x3F)|0x80; tmp[3]=(unicode&0x3F)|0x80; }
                            s = tmp; goto pushkey;
                        }
                    }
                break;
                default: /* every other, special key input modes */
                    if(s[0] == 8 || !memcmp(s, "Del", 4)) {
                        if(meg4_kbdtmpidx > 0) meg4_kbdtmpbuf[(int)--meg4_kbdtmpidx] = 0;
                        else goto pushkey;
                    } else
                    if(meg4_kbdtmpidx < 7 && s[0] > ' ' && !(s[0] & 0x80) && !s[1]) {
                        if(!meg4_kbdtmpidx) meg4_kbdtmpsht = (s[0] >= 'A' && s[0] <= 'Z');
                        meg4_kbdtmpbuf[(int)meg4_kbdtmpidx++] = (s[0] >= 'A' && s[0] <= 'Z') ? s[0] + 'a' - 'A' : s[0];
                        for(map = meg4_kbdcomplex[meg4.kbdmode - 2], found = NULL, p = 0; map && *map; map += 3) {
                            if(!memcmp(map[0], meg4_kbdtmpbuf, meg4_kbdtmpidx)) {
                                /* exact match */
                                if(!map[0][(int)meg4_kbdtmpidx]) { found = map; p = 1; break; }
                                /* possible match */
                                if(!found) found = map;
                                p++;
                            }
                        }
                        if(p == 0) {
                            /* no possible match */
                            memset(meg4_kbdtmpbuf, 0, sizeof(meg4_kbdtmpbuf));
                            meg4_kbdtmpsht = meg4_kbdtmpidx = 0;
                        } else
                        if(p == 1 && found) {
                            memset(meg4_kbdtmpbuf, 0, sizeof(meg4_kbdtmpbuf));
                            meg4_kbdtmpsht = meg4_kbdtmpidx = 0;
                            /* compose only: turn off compose mode after an exact match */
                            if(meg4.kbdmode == 2) meg4.kbdmode = 0;
                            s = meg4_kbdtmpsht && found[2] ? found[2] : found[1];
                            goto pushkey;
                        }
                    }
                break;
            }
        break;
    }
}

/**
 * Pop an UTF-8 key from the keyboard queue. See [keyboard], and for the blocking version [getc].
 * @return The UTF-8 representation of the key, or 0 if the queue was empty (no blocking).
 * @see [pendkey], [lenkey], [speckey], [getc]
 */
uint32_t meg4_api_popkey(void)
{
    uint32_t ret = 0;
    if(meg4.mmio.kbdhead != meg4.mmio.kbdtail) {
        ret = le32toh(meg4.mmio.kbd[(int)meg4.mmio.kbdtail]);
        meg4.mmio.kbdtail = ((meg4.mmio.kbdtail + 1) & 15);
    }
    return ret;
}

/**
 * Returns true if there's a key pending in the queue (but leaves the key in the queue, does not pop it).
 * @return Returns 1 if there are keys in the queue pending.
 * @see [pendkey], [lenkey], [speckey]
 */
int meg4_api_pendkey(void)
{
    return meg4.mmio.kbdhead != meg4.mmio.kbdtail;
}

/**
 * Returns the length of a UTF-8 key in bytes.
 * @param key the key, popped from the queue
 * @return UTF-8 representation's length in bytes.
 * @see [popkey], [pendkey], [speckey]
 */
int meg4_api_lenkey(uint32_t key)
{
    uint8_t *k = (uint8_t*)&key;
    return !k[0] ? 0 : (k[1] ? (k[2] ? (k[3] ? 4 : 3) : 2) : 1);
}

/**
 * Returns true if key is a special key.
 * @param key the key, popped from the queue
 * @return Returns 1 if it's a special key, and 0 if it's a printable one.
 * @see [popkey], [pendkey], [lenkey]
 */
int meg4_api_speckey(uint32_t key)
{
    uint8_t *k = (uint8_t*)&key;
    /* one character long keys and UTF-8 multibytes are printable characters. Multiple ANSI chars are special keys */
    return (k[0] < 32) || (k[1] && !(k[1] & 0x80)) ? 1 : 0;
}
