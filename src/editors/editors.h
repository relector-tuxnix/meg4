/*
 * meg4/editors/editors.h
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
 * @brief Common editors header
 *
 */

#ifndef MEG4_EDITORS
#define MEG4_EDITORS

#include "../meg4.h"
#include "../data.h"
#include "../lang.h"
#include "../stb_image.h"

#define LABEL_COLOR 0xff4d1f09 /* deliberately not in theme, because has to match floppy image */
/* color entries in theme GIMP palette */
enum { THEME_BG, THEME_FG, THEME_D, THEME_L, THEME_MENU_BG, THEME_MENU_FG, THEME_MENU_D, THEME_MENU_L, THEME_ACTIVE_D,
    THEME_ACTIVE_BG, THEME_SEL_BG, THEME_SEL_FG, THEME_INP_BG, THEME_INP_FG, THEME_INP_D, THEME_INP_L, THEME_BTN_BG, THEME_BTN_FG,
    THEME_BTN_D, THEME_BTN_L, THEME_HELP_TITLE, THEME_HELP_LINK, THEME_HELP_HDR, THEME_HELP_EM, THEME_HELP_STRESS, THEME_HELP_MONO,
    THEME_HELP_KEY, THEME_LN_BG, THEME_LN_FG, THEME_ERR_LN, THEME_ERR_BG, THEME_ERR_FG, THEME_HL_BG, THEME_HL_C, THEME_HL_P,
    THEME_HL_O, THEME_HL_N, THEME_HL_S, THEME_HL_D, THEME_HL_T, THEME_HL_K, THEME_HL_F, THEME_HL_V };
/* text input types */
enum { TEXTINP_NAME, TEXTINP_STR, TEXTINP_NUM, TEXTINP_HEX };
/* highlight token types: comment, pseudo, operator, number, string, delimiter, type, keyword, function, variable */
enum { HL_C, HL_P, HL_O, HL_N, HL_S, HL_D, HL_T, HL_K, HL_F, HL_V };
/* last pointer button */
extern int last;

/* meg4.c */
int meg4_isbyte(void *buf, uint8_t byte, int len);

/* for highlighter */
typedef struct {
    uint8_t type;           /* token type, same as highlight token types, see above */
    int id, pos, len;       /* id could mean lots of things, pos is the position in source and len is the length in bytes */
} tok_t;
#include "../cpu.h"

/* hl.c */
void hl_init(void);
void hl_free(void);
char ***hl_find(char *lng);
int *hl_tokenize(char ***r, char *str, char *end, int *tokens, int *len, int *mem, int *pos);
tok_t *hl_tok(char ***r, char *str, int *len);

/* pro.c */
void pro_init(void);
void pro_export(void);
int  pro_license(uint8_t *buf, int len);

/* formats.c */
void meg4_export(char *name, int binary);
int  meg4_import(char *name, uint8_t *buf, int len, int lvl);
int  meg4_theme(uint8_t *buf, int len);

/* menu.c */
extern int menu_scroll, menu_scrmax, menu_scrhgt, menu_stat;
void menu_init(void);
int  menu_ctrl(void);
void menu_view(uint32_t *dst, int dw, int dh, int dp);
void menu_icon(uint32_t *dst, int dw, int dh, int dp, int dx, int sx, int sy, int ctrl, int key, int stat);

/* textinp.c */
extern char *textinp_cur, *textinp_buf;
extern uint32_t textinp_key;
void textinp_init(int x, int y, int w, uint32_t color, uint32_t shadow, int ft, uint8_t *font, int scr, int type, char *str, int maxlen);
void textinp_free(void);
int  textinp_ctrl(void);
void textinp_view(uint32_t *dst, int dp);
int  textinp_cursor(uint32_t *dst, int dp);

/* toolbox.c */
void toolbox_paint(int sx, int sy, int sw, int sh, int x, int y, uint8_t idxs, uint8_t idxe);
void toolbox_fill(int sx, int sy, int sw, int sh, int x, int y, uint8_t idxs, uint8_t idxe);
uint8_t toolbox_pick(int sx, int sy, int sw, int sh, int x, int y);
void toolbox_selall(int sx, int sy, int sw, int sh);
void toolbox_selinv(int sx, int sy, int sw, int sh);
void toolbox_selrect(int sx, int sy, int sw, int sh, int x, int y, int w, int h);
void toolbox_selfuzzy(int sx, int sy, int sw, int sh, int x, int y);
void toolbox_cut(int sx, int sy, int sw, int sh);
void toolbox_copy(int sx, int sy, int sw, int sh);
void toolbox_paste(int sx, int sy, int sw, int sh, int px, int py);
void toolbox_pasteview(int sx, int sy, int sw, int sh, int x, int y, int w, int h, int px, int py);
void toolbox_del(int sx, int sy, int sw, int sh);
void toolbox_shl(int sx, int sy, int sw, int sh);
void toolbox_shu(int sx, int sy, int sw, int sh);
void toolbox_shd(int sx, int sy, int sw, int sh);
void toolbox_shr(int sx, int sy, int sw, int sh);
void toolbox_rcw(int sx, int sy, int sw, int sh);
void toolbox_flv(int sx, int sy, int sw, int sh);
void toolbox_flh(int sx, int sy, int sw, int sh);
void toolbox_btn(int x, int y, int c, int t);
void toolbox_histadd(void);
void toolbox_histundo(void);
void toolbox_histredo(void);
void toolbox_init(uint8_t *buf, int w, int h);
void toolbox_free(void);
int  toolbox_ctrl(void);
int  toolbox_notectrl(uint8_t *note, int snd);
void toolbox_view(int x, int y, int type);
void toolbox_blit(int x, int y, int dp, int w, int h, int sx, int sy, int sw, int sh, int sp, int chk);
void toolbox_map(int x, int y, int dp, int w, int h, int mx, int my, int zoom, int mapsel, int px, int py, int sprs, int spre);
void toolbox_note(int x, int y, int dp, uint32_t fg, uint8_t *note);
void toolbox_noteview(int x, int y, int dp, int playing, uint8_t *note);
void toolbox_stat(void);

/* load.c */
extern int load_list;
int  load_insert(uint8_t *buf, int len);
void load_init(void);
void load_free(void);
int  load_ctrl(void);
void load_menu(uint32_t *dst, int dw, int dh, int dp);
void load_view(void);
void load_bg(void);

/* save.c */
void save_init(void);
void save_free(void);
int  save_ctrl(void);
void save_view(void);

/* help.c */
void help_init(void);
void help_free(void);
int  help_find(char *section, int len, int func);
void help_show(int page);
void help_hide(void);
int  help_ctrl(void);
void help_menu(uint32_t *dst, int dw, int dh, int dp);
void help_view(void);

/* debug.c */
void debug_init(void);
void debug_free(void);
int  debug_ctrl(void);
void debug_menu(uint32_t *dst, int dw, int dh, int dp);
void debug_view(void);

/* visual.c */
void visual_init(void);
void visual_free(void);
int  visual_ctrl(void);
void visual_menu(uint32_t *dst, int dw, int dh, int dp);
void visual_view(void);

/* code.c */
#define code_error(a,b) code_seterr(a,b)
/*#define code_error(a,b) do{printf("(compiler %s:%u) ",__FILE__,__LINE__);code_seterr(a,b);}while(0)*/
void code_setpos(int line, uint32_t pos);
void code_seterr(uint32_t pos, const char *msg);
void code_init(void);
void code_free(void);
int  code_ctrl(void);
void code_menu(uint32_t *dst, int dw, int dh, int dp);
void code_view(void);

/* sprite.c */
void sprite_init(void);
void sprite_free(void);
int  sprite_ctrl(void);
void sprite_menu(uint32_t *dst, int dw, int dh, int dp);
void sprite_view(void);

/* map.c */
void map_init(void);
void map_free(void);
int  map_ctrl(void);
void map_menu(uint32_t *dst, int dw, int dh, int dp);
void map_view(void);

/* font.c */
void font_init(void);
void font_free(void);
int  font_ctrl(void);
void font_menu(uint32_t *dst, int dw, int dh, int dp);
void font_view(void);

/* sound.c */
void sound_addwave(int wave);
void sound_init(void);
void sound_free(void);
int  sound_ctrl(void);
void sound_menu(uint32_t *dst, int dw, int dh, int dp);
void sound_view(void);

/* music.c */
void music_init(void);
void music_free(void);
int  music_ctrl(void);
void music_menu(uint32_t *dst, int dw, int dh, int dp);
void music_view(void);

/* overlay.c */
extern int overlay_idx;
void overlay_init(void);
void overlay_free(void);
int  overlay_ctrl(void);
void overlay_menu(uint32_t *dst, int dw, int dh, int dp);
void overlay_view(void);

#endif /* MEG4_EDITORS */
