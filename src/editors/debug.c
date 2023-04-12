/*
 * meg4/editors/debug.c
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
 * @brief Debugger
 *
 */

#include <stdio.h>
#include "editors.h"
#include "../api.h"

extern char errmsg[256];
typedef struct {
    uint32_t bp, pc, line, pos;
} cb_t;
static cb_t cb[33];
static int numcb = 0;

static int unaw = 0, cdew = 0, dtaw = 0, btnw = 0, tab = 0, numcd = 0, cont = 1;
static char **opmne = NULL;

/**
 * Disassemble opcodes
 */
uint32_t debug_disasm(uint32_t pc, char *out)
{
    char ***r;
    int i, p;

    *out = 0;
    if(!meg4.code || meg4.code_len < 4 || pc >= meg4.code_len || pc >= meg4.code[0]) return 0;
    if(!opmne) { r = hl_find("asm"); if(!r) { return 0; } opmne = r[7]; }
    i = meg4.code[pc] & 0xff; p = meg4.code[pc] >> 8;
    if(pc < 4) sprintf(out, ".hdr %05X", meg4.code[pc]); else
    if(i == BC_BND || (i >= BC_INCB && i <= BC_DECI)) sprintf(out, "%s %u", opmne[i], p); else
    if(i == BC_CALL) sprintf(out, "%s %05X", opmne[i], meg4.code[++pc]); else
    if(i == BC_SCALL) sprintf(out, "%s %s", opmne[i], p >= 0 && p < MEG4_NUM_API ? meg4_api[p].name : "???"); else
    if(i == BC_SP || i == BC_ADR || i == BC_LEA) sprintf(out, "%s %d", opmne[i], (int)(meg4.code[pc] & ~0xff) / 256); else
    if(i >= BC_JMP && i <= BC_JNZ) sprintf(out, "%s %05X", opmne[i], meg4.code[++pc]); else
    if(i == BC_CI || i == BC_PSHCI) sprintf(out, "%s %X", opmne[i], (int)meg4.code[++pc]); else
    if(i == BC_CF || i == BC_PSHCF) sprintf(out, "%s %.4g", opmne[i], *(float*)&meg4.code[++pc]); else
    if(i == BC_SW) { sprintf(out, "%s %d,%05X,...(+%u)", opmne[i], (int)meg4.code[pc + 1], meg4.code[pc + 2], p); pc += 2 + p; } else
    if((i >= BC_DEBUG && i < BC_SCALL) || (i >= BC_PUSHI && i < BC_LAST)) strcpy(out, opmne[i]); else
        sprintf(out, "??? %08x", meg4.code[pc]);
    return pc + 1;
}

/**
 * Find the closest source position to a program counter
 */
uint32_t debug_pos(uint32_t pc)
{
    uint32_t i, j = 0;
    if(meg4.code && meg4.code_len > 4 && meg4.code_len > meg4.code[0] && pc)
        for(i = meg4.code[0]; i < meg4.code_len && i < meg4.code[1] && meg4.code[i] <= pc; i += 2) j = meg4.code[i + 1];
    return j;
}

/**
 * Handle a run-time error
 */
void debug_rte(int err)
{
    code_error(debug_pos(meg4.pc), err ? lang[err] : NULL);
    meg4_switchmode(MEG4_MODE_DEBUG);
    numcb = 0;
}

/**
 * Do step-by-step execution
 */
void debug_step(void)
{
    if(cont) {
        meg4.mode = MEG4_MODE_GAME;
        cpu_fetch();
        /* if we have finished with setup() or loop(), restart loop() */
        if(!meg4.pc && meg4.code && meg4.code_len > 4) {
            meg4.pc = meg4.code[3];
            main_log(3, "CPU: new entry point %05X", meg4.pc);
        }
        meg4.mode = MEG4_MODE_DEBUG;
    } else
        debug_rte(ERR_BADADR);
    numcb = 0;
}

/**
 * Initialize debugger
 */
void debug_init(void)
{
    unaw = meg4_width(meg4_font, 1, lang[MENU_UNAVAIL], NULL);
    cdew = meg4_width(meg4_font, 1, lang[DBG_CODE], NULL);
    dtaw = meg4_width(meg4_font, 1, lang[DBG_DATA], NULL);
    btnw = cdew > dtaw ? cdew: dtaw;
}

/**
 * Free resources
 */
void debug_free(void)
{
    numcb = 0;
}

/**
 * Controller
 */
int debug_ctrl(void)
{
    uint32_t key;
    int clk = le16toh(meg4.mmio.ptrbtn) & (MEG4_BTN_L | MEG4_BTN_R), px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    if(last && !clk) {
        if(px >= 602 - btnw - 26 && px < 602 - btnw - 10 && py < 12) debug_step(); else
        if(px >= 602 - btnw && px < 610 && py < 12) tab ^= 1; else
        if(px >= 614 && px < 626 && py < 12) { meg4_switchmode(MEG4_MODE_VISUAL); last = 0; return 1; } else
        if(px >= 626 && px < 638 && py < 12) { meg4_switchmode(MEG4_MODE_CODE); last = 0; return 1; } else
        if(!tab && px >= 132 && px < 453 && py >= 36 && py < 36 + numcb * 10) {
            py = (py - 36) / 10; code_setpos(cb[py].line, cb[py].pos);
            meg4_switchmode(MEG4_MODE_CODE); last = 0; return 1;
        }
    } else
    if(!last && !clk) {
        key = meg4_api_popkey();
        if(key == htole32('\t')) { tab ^= 1; numcb = 0; } else
        if(key == htole32(' ')) debug_step();
    }
    last = clk;
    return 1;
}

/**
 * Menu
 */
void debug_menu(uint32_t *dst, int dw, int dh, int dp)
{
    int clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    /* step execution */
    menu_icon(dst, dw, dh, dp, 602 - btnw - 26, 112, 56, 0, MEG4_KEY_SPACE, STAT_STEP);

    /* code / data tab switcher */
    if(px >= 602 - btnw && px < 610 && py < 10 && clk)
        meg4_box(dst, dw, dh, dp, 602 - btnw, 1, btnw + 8, 10, theme[THEME_BTN_D], theme[THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
    else
        meg4_box(dst, dw, dh, dp, 602 - btnw, 1, btnw + 8, 10, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
    meg4_text(dst, 602 - btnw + (4 + (btnw - (tab ? cdew : dtaw)) / 2), 2, dp, theme[THEME_BTN_FG], 0, 1, meg4_font, lang[tab ? DBG_CODE : DBG_DATA]);

    /* visual */    menu_icon(dst, dw, dh, dp, 614, 112, 48, 1, 0, MENU_VISUAL);
    /* code */      menu_icon(dst, dw, dh, dp, 626,  48, 48, 1, 0, MENU_CODE);
}

/**
 * View
 */
void debug_view(void)
{
    float f;
    uint32_t u, o, *fp = (uint32_t*)&f;
    char tmp[256];
    int i, j, k, x1, y0, y1;
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);

    /* status bar */
    if(errmsg[0]) {
        meg4_box(meg4.valt, 632, 388, 2560, 0, 378, 632, 10, theme[THEME_ERR_BG], theme[THEME_ERR_BG], theme[THEME_ERR_BG], 0, 0, 0, 0, 0);
        meg4_text(meg4.valt, 4, 379, 2560, theme[THEME_ERR_FG], 0, 1, meg4_font, errmsg);
    }

    /* registers */
    meg4_box(meg4.valt, 640, 388, 2560, 10, 374-22, 444, 22, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 10, 0);
    meg4_text(meg4.valt, 10, 374-31, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_REGISTERS]);
    if(meg4.code_type < 0x10) {
        sprintf(tmp, "AC: %08X", meg4.ac);
        meg4_text(meg4.valt, 20, 374-20, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "%d", meg4.ac);
        meg4_text(meg4.valt, 82, 374-20, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "AF: %.8f", meg4.af);
        meg4_text(meg4.valt, 152, 374-20, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "CP: %4u", meg4.cp);
        meg4_text(meg4.valt, 20, 374-10, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "DP: %05X", meg4.dp + MEG4_MEM_USER);
        meg4_text(meg4.valt, 82, 374-10, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "BP: %05X", meg4.bp + MEG4_MEM_USER);
        meg4_text(meg4.valt, 152, 374-10, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
        sprintf(tmp, "SP: %05X", meg4.sp + MEG4_MEM_USER);
        meg4_text(meg4.valt, 222, 374-10, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
    } else
        meg4_text(meg4.valt, 10 + (274 - unaw) / 2, 374-16, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
    sprintf(tmp, "FLG: %02X%s%s%s%s", meg4.flg, meg4.flg & 1 ? " setup" : "", meg4.flg & 2 ? " io" : "", meg4.flg & 4 ? " tmr" : "",
        meg4.flg & 8 ? " stop" : "");
    meg4_text(meg4.valt, 288, 374-20, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
    sprintf(tmp, "TMR: %4d", meg4.tmr);
    meg4_text(meg4.valt, 285, 374-10, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
    sprintf(tmp, "PC: %08X", meg4.pc);
    meg4_text(meg4.valt, 362, 374-10, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);

    /* main area */
    menu_scrhgt = 362; x1 = meg4.mmio.cropx1; y0 = meg4.mmio.cropy0; y1 = meg4.mmio.cropy1;
    if(!tab) {
        /* code tab */
        meg4_text(meg4.valt, 10, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_CALLSTACK]);
        meg4_text(meg4.valt, 499, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_CODE]);
        if(!meg4.code || meg4.code_len < 4 || meg4.code_type >= 0x10) {
            meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 444, 332, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 10, 0);
            meg4_box(meg4.valt, 640, 388, 2560, 499, 10, 130, 364, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 10, 0);
            meg4_text(meg4.valt, 10 + (444 - unaw) / 2, 170, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
        } else {
            /* this has multiple O(n) algorithms, so cache the results */
            if(cb[0].pc != meg4.pc || !numcb || !numcd) {
                if(meg4.pc) {
                    cb[0].bp = meg4.bp + MEG4_MEM_USER; cb[0].pc = meg4.pc; cb[0].pos = debug_pos(meg4.pc);
                    for(i = 0, cb[0].line = 1; i < (int)cb[0].pos; i++) if(meg4.src[i] == '\n') cb[0].line++;
                    for(numcb = 1, j = (int)meg4.cp - 2; numcb < (int)(sizeof(cb)/sizeof(cb[0])) && j >= 0; j -= 2, numcb++) {
                        cb[numcb].bp = meg4.cs[j] + MEG4_MEM_USER; cb[numcb].pc = meg4.cs[j + 1]; cb[numcb].pos = debug_pos(meg4.cs[j + 1]);
                        for(i = 0, cb[numcb].line = 1; i < (int)cb[numcb].pos; i++) if(meg4.src[i] == '\n') cb[numcb].line++;
                    }
                    numcd = 0;
                }
                if(!numcd) {
                    for(u = cont = 0; u < meg4.code[0] && u < meg4.code_len; numcd++) {
                        u = debug_disasm(u, tmp);
                        if(u == meg4.pc) { cont = 1; menu_scroll = numcd > 16 ? (numcd - 16) * 10 : 0; }
                    }
                }
            }
            menu_scrmax = numcd * 10;
            if(menu_scroll + menu_scrhgt > menu_scrmax) menu_scroll = menu_scrmax - menu_scrhgt;
            if(menu_scroll < 0) menu_scroll = 0;
            /* disassembly */
            meg4_box(meg4.valt, 640, 388, 2560, 499, 10, 130, 364, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, menu_scroll, 10, 0);
            meg4.mmio.cropy0 = htole16(11); meg4.mmio.cropx1 = htole16(628); meg4.mmio.cropy1 = htole16(374);
            for(j = u = 0; u < meg4.code[0] && u < meg4.code_len && j < menu_scroll + menu_scrhgt; j += 10) {
                o = u; u = debug_disasm(u, tmp);
                if(j + 10 < menu_scroll) continue;
                meg4_text(meg4.valt, 504, 12 + j - menu_scroll, 2560, theme[o && o == meg4.pc ? THEME_SEL_FG : THEME_FG], 0, 1, meg4_font, tmp);
                sprintf(tmp, "%05X", o);
                meg4_text(meg4.valt, 497 - meg4_width(meg4_font, 1, tmp, NULL), 12 + j - menu_scroll, 2560,
                    theme[o && o == meg4.pc ? THEME_SEL_FG : (o < meg4.pc && u > meg4.pc ? THEME_ERR_FG : THEME_D)], theme[THEME_L],
                    1, meg4_font, tmp);
            }
            /* callstack */
            meg4.mmio.cropy0 = 0;
            meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 444, 332, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 12, 0, 10, 0);
            meg4_box(meg4.valt, 640, 388, 2560, 11, 11, 42, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 14, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "BP");
            meg4_box(meg4.valt, 640, 388, 2560, 53, 11, 42, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 56, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, "PC");
            meg4_box(meg4.valt, 640, 388, 2560, 95, 11, 32, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 98, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_LINE]);
            meg4_box(meg4.valt, 640, 388, 2560, 127, 11, 326, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 130, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_SRC]);
            meg4.mmio.cropx1 = htole16(453); meg4.mmio.cropy1 = htole16(342);
            meg4.mmio.ptrspr = MEG4_PTR_NORM;
            for(i = 0, j = 24; j < 342 && i < numcb; i++, j += 10) {
                sprintf(tmp, "%05X", cb[i].bp);
                meg4_text(meg4.valt, 12, j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                sprintf(tmp, "%08X", cb[i].pc);
                meg4_text(meg4.valt, 54, j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                sprintf(tmp, "%u", cb[i].line);
                meg4_text(meg4.valt, 126 - meg4_width(meg4_font, 1, tmp, NULL), j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                for(k = 0; k < (int)sizeof(tmp) - 1 && meg4.src[cb[i].pos + k] != '\n'; k++) { tmp[k] = meg4.src[cb[i].pos + k]; } tmp[k] = 0;
                meg4_text(meg4.valt, 132, j, 2560, theme[THEME_HELP_LINK], 0, 1, meg4_font, tmp);
                if(px >= 132 && px < 453 && py >= j + 12 && py < j + 21) meg4.mmio.ptrspr = MEG4_PTR_HAND;
            }
        }
    } else {
        /* data tab */
        meg4_text(meg4.valt, 10, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_DATA]);
        meg4_text(meg4.valt, 499, 1, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_STACK]);
        if(meg4.code_type >= 0x10) {
            meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 444, 332, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 10, 0);
            meg4_box(meg4.valt, 640, 388, 2560, 499, 10, 130, 364, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, 0, 10, 0);
            meg4_text(meg4.valt, 10 + (444 - unaw) / 2, 130, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[MENU_UNAVAIL]);
        } else {
            /* stack */
            u = meg4.bp + 64 - menu_scroll / 10 * 4; if(u > sizeof(meg4.data)) { u = sizeof(meg4.data); menu_scroll = 0; }
            i = (u >> 2) - ((meg4.sp - 64) >> 2);
            j = meg4.code && meg4.code[1] && meg4.code[1] < meg4.code_len ? (meg4.code_len - meg4.code[1]) >> 2 : 0;
            if(j > i) i = j;
            if(i < 32) i = 32;
            menu_scrmax = i * 10;
            if(menu_scroll + menu_scrhgt > menu_scrmax) {
                menu_scroll = menu_scrmax - menu_scrhgt;
                if(menu_scroll < 0) menu_scroll = 0;
                u = meg4.bp + 64 - menu_scroll / 10 * 4; if(u > sizeof(meg4.data)) u = sizeof(meg4.data);
            }
            meg4_box(meg4.valt, 640, 388, 2560, 499, 10, 130, 364, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 0, menu_scroll, 10, 0);
            meg4.mmio.cropy0 = htole16(11); meg4.mmio.cropx1 = htole16(628); meg4.mmio.cropy1 = htole16(374);
            for(i = 0; i < 37 && (int)u - 3 >= 0; i++, u -= 4) {
                sprintf(tmp, "%05X", u + MEG4_MEM_USER);
                meg4_text(meg4.valt, 497 - meg4_width(meg4_font, 1, tmp, NULL), 12 + i * 10 - (menu_scroll % 10), 2560,
                    theme[u == (meg4.sp & ~3) ? THEME_SEL_FG : (u == meg4.bp ? THEME_SEL_BG : THEME_D)], theme[THEME_L],
                    1, meg4_font, tmp);
                if(u == meg4.bp) {
                    meg4_box(meg4.valt, 640, 388, 2560, 500, 20 + i * 10 - (menu_scroll % 10), 128, 1, theme[THEME_SEL_BG],
                        theme[THEME_SEL_BG], theme[THEME_SEL_BG], 0, 0, 0, 0, 0);
                    meg4_text(meg4.valt, 614, 12 + i * 10 - (menu_scroll % 10), 2560, theme[THEME_SEL_BG], 0, 1, meg4_font, "BP");
                }
                if(u == (meg4.sp & ~3)) {
                    j = (meg4.sp & 3); if(j) j = 2 + j * 16;
                    meg4_box(meg4.valt, 640, 388, 2560, 500 + j, 20 + i * 10 - (menu_scroll % 10), 128 - j, 1, theme[THEME_SEL_FG],
                        theme[THEME_SEL_FG], theme[THEME_SEL_FG], 0, 0, 0, 0, 0);
                    meg4_text(meg4.valt, 614, 20 + i * 10 - (menu_scroll % 10), 2560, theme[THEME_SEL_FG], 0, 1, meg4_font, "SP");
                }
                for(j = 0; j < 4; j++) {
                    if(j && u + j == meg4.sp)
                        meg4_box(meg4.valt, 640, 388, 2560, 502 + j * 16, 13 + i * 10 - (menu_scroll % 10), 1, 7, theme[THEME_SEL_FG],
                            theme[THEME_SEL_FG], theme[THEME_SEL_FG], 0, 0, 0, 0, 0);
                    sprintf(tmp, "%02X", meg4.data[u + j]);
                    meg4_text(meg4.valt, 506 + j * 16, 12 + i * 10 - (menu_scroll % 10), 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                    sprintf(tmp, "%c", meg4.data[u + j] < 32 || meg4.data[u + j] > 126 ? '.' :  meg4.data[u + j]);
                    meg4_text(meg4.valt, 572 + j * 8, 12 + i * 10 - (menu_scroll % 10), 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                }
            }
            /* globals */
            meg4.mmio.cropy0 = 0;
            meg4_box(meg4.valt, 640, 388, 2560, 10, 10, 444, 332, theme[THEME_D], theme[THEME_BG], theme[THEME_L], 0, 12, menu_scroll, 10, 0);
            meg4_box(meg4.valt, 640, 388, 2560, 11, 11, 42, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 14, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_ADDR]);
            meg4_box(meg4.valt, 640, 388, 2560, 53, 11, 42, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 56, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_SIZE]);
            meg4_box(meg4.valt, 640, 388, 2560, 95, 11, 96, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 98, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_VAR]);
            meg4_box(meg4.valt, 640, 388, 2560, 191, 11, 262, 12, theme[THEME_L], theme[THEME_BG], theme[THEME_D], 0, 0, 0, 12, 0);
            meg4_text(meg4.valt, 194, 12, 2560, theme[THEME_D], theme[THEME_L], 1, meg4_font, lang[DBG_VAL]);
            if(meg4.code && meg4.code[1] && meg4.code[1] < meg4.code_len) {
                meg4.mmio.cropx1 = htole16(453); meg4.mmio.cropy1 = htole16(24); meg4.mmio.cropy1 = htole16(342);
                for(i = meg4.code[1], j = 24 - menu_scroll; j < 342 && i < (int)meg4.code_len; i += 4, j += 10) {
                    if(j + 10 < 24) continue;
                    /* address and size */
                    sprintf(tmp, "%05X", meg4.code[i + 2]);
                    meg4_text(meg4.valt, 12, j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                    sprintf(tmp, "%u", meg4.code[i + 3]);
                    meg4_text(meg4.valt, 96 - meg4_width(meg4_font, 1, tmp, NULL), j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                    /* variable name */
                    o = meg4.code[i + 1] & 0xffffff; u = meg4.code[i + 1] >> 24;
                    for(k = 0; k < (int)sizeof(tmp) - 1 && k < (int)u && meg4.src[o + k] != '\n'; k++) { tmp[k] = meg4.src[o + k]; } tmp[k] = 0;
                    meg4.mmio.cropx1 = htole16(192);
                    meg4_text(meg4.valt, 98, j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                    meg4.mmio.cropx1 = htole16(453);
                    /* value */
                    memset(tmp, 0, sizeof(tmp));
                    o = meg4.code[i] & 15;
                    if(o == T_SCALAR || o == T_PTR) {
                        for(o = meg4.code[i + 2], u = o + meg4.code[i + 3], k = 0; k + 16 < (int)sizeof(tmp) && o < u;)
                            switch(meg4.code[i] >> 4) {
                                case T_I8: k += sprintf(tmp + k, "%d ", meg4_api_inb(o)); o++; break;
                                case T_I16: k += sprintf(tmp + k, "%d ", meg4_api_inw(o)); o += 2; break;
                                case T_I32: k += sprintf(tmp + k, "%d ", meg4_api_ini(o)); o += 4; break;
                                case T_U8: k += sprintf(tmp + k, "%u ", meg4_api_inb(o)); o++; break;
                                case T_U16: k += sprintf(tmp + k, "%u ", meg4_api_inw(o)); o += 2; break;
                                case T_U32: k += sprintf(tmp + k, "%u ", meg4_api_ini(o)); o += 4; break;
                                case T_FLOAT: *fp = meg4_api_ini(o); k += sprintf(tmp + k, "%.8f ", f); o += 4; break;
                            }
                    } else
                    if(o > T_PTR) {
                        for(o = meg4.code[i + 2], u = o + meg4.code[i + 3], k = 0; k + 6 < (int)sizeof(tmp) && o < u; o += 4)
                            k += sprintf(tmp + k, "%05X ", meg4_api_ini(o));
                    }
                    meg4_text(meg4.valt, 194, j, 2560, theme[THEME_FG], 0, 1, meg4_font, tmp);
                }
            }
        }
    }
    meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
}
