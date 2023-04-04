/*
 * meg4/lang.h
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
 * @brief Multilanguage support, dictionary keywords
 *
 */
#ifndef LANG_H
#define LANG_H
enum {
    LOAD_FANCON,        /*** for practical reasons, line numbers must  ***/
    LOAD_FW,            /*** match with the strings in lang/(code).h files! ***/

    MENU_CODEPOINT,
    MENU_COMPOSE,
    MENU_ICONEMOJI,
    MENU_KATAKANA,
    MENU_HIRAGANA,

    MENU_REGTO,
    MENU_NONAME,
    MENU_BINARY,
    MENU_OK,
    MENU_LOAD,
    MENU_UNAVAIL,
    MENU_EXPORT,
    MENU_ABOUT,
    MENU_RUN,
    MENU_SAVE,
    MENU_IMPORT,
    MENU_EXPZIP,
    MENU_EXPWASM,
    MENU_FULLSCR,
    MENU_INPUTS,
    MENU_HELP,

    MENU_DEBUG,
    MENU_VISUAL,
    MENU_CODE,
    MENU_SPRITE,
    MENU_MAP,
    MENU_FONT,
    MENU_SOUND,
    MENU_MUSIC,
    MENU_OVERLAY,
    MENU_LIST,

    STAT_FIND,
    STAT_REPLACE,
    STAT_GOTO,
    STAT_FUNC,
    STAT_BOOK,
    STAT_UNDO,
    STAT_REDO,
    STAT_CUT,
    STAT_COPY,
    STAT_PASTE,
    STAT_ERASE,
    STAT_PLAY,
    STAT_INSERT,
    STAT_DELETE,
    STAT_ADDR,
    STAT_LEN,
    STAT_SHL,
    STAT_SHU,
    STAT_SHD,
    STAT_SHR,
    STAT_ROT,
    STAT_FLIPV,
    STAT_FLIPH,
    STAT_PAINT,
    STAT_FILL,
    STAT_PICKER,
    STAT_RECT,
    STAT_FUZZY,
    STAT_HSV,
    STAT_ZOOM,
    STAT_BANK,
    STAT_PITCH,
    STAT_WAVE,
    STAT_EFF,
    STAT_EFFX,
    STAT_EFFY,
    STAT_WED,
    STAT_TUNE,
    STAT_VOL,
    STAT_LOOP,
    STAT_SINE,
    STAT_TRI,
    STAT_SAW,
    STAT_SQR,
    STAT_SB,
    STAT_WSH,
    STAT_WGR,
    STAT_WUP,
    STAT_WDN,
    STAT_WSP,
    STAT_WPL,
    STAT_SPRSEL,
    STAT_GLPSEL,
    STAT_SNDSEL,
    STAT_TRKSEL,
    STAT_STEP,

    HELP_TOC,
    HELP_RESULTS,
    HELP_BACK,

    DBG_CALLSTACK,
    DBG_REGISTERS,
    DBG_LINE,
    DBG_SRC,
    DBG_ADDR,
    DBG_SIZE,
    DBG_VAR,
    DBG_VAL,
    DBG_CODE,
    DBG_DATA,
    DBG_STACK,

    CODE_SETUP,
    CODE_LOOP,

    OVL_PALETTE,
    OVL_SOUNDS,
    OVL_MAP,
    OVL_SPRITES,
    OVL_WAVEFORM,
    OVL_TRACK,
    OVL_GLYPHS,
    OVL_USER,

    ERR_UNKLNG,
    ERR_MEMORY,
    ERR_STACK,
    ERR_RECUR,
    ERR_TOOLNG,
    ERR_TOOCLX,
    ERR_TOOBIG,
    ERR_RANGE,
    ERR_NOTSTC,
    ERR_SYNTAX,
    ERR_NOCLOSE,
    ERR_NOOPEN,
    ERR_NODATA,
    ERR_NOCODE,
    ERR_NORET,
    ERR_BADPROTO,
    ERR_NUMARG,
    ERR_BADARG,
    ERR_BADFMT,
    ERR_BADDRF,
    ERR_BADADR,
    ERR_BADLVAL,
    ERR_BADCND,
    ERR_NOTOP,
    ERR_ALRDEF,
    ERR_UNDEF,
    ERR_NOTHERE,
    ERR_DIVZERO,
    ERR_BOUNDS,
    ERR_BADSYS,

    /* must be the last */
    NUMTEXTS
};

#define NUMLANGS         8

extern char **lang;

#endif
