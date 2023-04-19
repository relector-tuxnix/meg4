/*
 * meg4/editors/toolbox.c
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
 * @brief Toolbox widget
 *
 */

#include <stdio.h>
#include "editors.h"

int tcw = 0, tch = 0;
static int tx = 0, ty = 0, tn = 0, tp = 0, th = 0, ts = 0, tsx = 0, tsy = 0, tsw = 0, tsh = 0, ta = 0, numhist = 0, curhist = 0;
static int px = 0, py = 0, ps = -1, po = 4, pz = 0, pp = 0;
static uint8_t *tb = NULL, *tc = NULL, *hist = NULL;

/* notes, must match MEG4_NOTE_x enums */
static char *tbnotes[] = { "...",
    "C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
    "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
    "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
    "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
    "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
    "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
    "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
    "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7"
};
/* effects, first character means the parameter type: 1 - one byte 0-255, 2 - two tetrads, 3 - two tetrads, but only
 * one of them can be set, 4 - lower tetrad only, 5 - waveform, 6 - lover tetrad as signed decimal */
static char *tbeffects[] = {
    "\002Arp",  /* 0 arpeggio */
    "\001Po/",  /* 1 portamento up */
    "\001Po\\", /* 2 portamento down */
    "\001Ptn",  /* 3 tone portamento */
    "\002Vib",  /* 4 vibrato */
    "\003Ctv",  /* 5 continue tone portamento + volume slide */
    "\003Cvv",  /* 6 continue vibrato + volume slide */
    "\002Trm",  /* 7 tremolo */
    NULL,       /* 8 stereo balance, not in MEG-4 */
    "\001Ofs",  /* 9 set sample offset */
    "\003Vls",  /* A volume slide */
    "\001Jmp",  /* B position jump */
    "\004Vol",  /* C set volume */
    NULL,       /* D pattern break, not in MEG-4 */
    NULL,       /* E fine effects, remapped to 16-31 */
    "\001Tpr",  /* F set tempo, ticks per row */
    NULL,       /* E0 Amiga filter on/off, not in MEG-4 */
    "\004Fp/",  /* E1 fine portamento up */
    "\004Fp\\", /* E2 fine portamento down */
    NULL,       /* E3 glissando, not in MEG-4 */
    "\005Svw",  /* E4 set vibrato waveform */
    "\006Ftn",  /* E5 set finetune */
    NULL,       /* E6 pattern loop, not in MEG-4 */
    "\005Stw",  /* E7 set tremolo waveform */
    NULL,       /* E8 fine stereo balance, not in MEG-4 */
    "\004Rtg",  /* E9 retrigger note */
    "\004Fv/",  /* EA fine volume slide up */
    "\004Fv\\", /* EB fine volume slide down */
    "\004Cut",  /* EC cut note */
    "\004Dly",  /* ED delay note */
    NULL,       /* EE pattern delay, not in MEG-4 */
    NULL        /* EF invert loop, not in MEG-4 */
};

/**
 * Find selection bounds
 */
void toolbox_bounds(void)
{
    int i, j;
    tsx = tp; tsy = th; tsw = tsh = ts = 0;
    if(!tc) return;
    for(j = 0; j < th; j++)
        for(i = 0; i < tp; i++) {
            tc[(j + th) * tp + i] &= 0x7F;
            if(tc[(j + th) * tp + i]) {
                if(i < tsx) tsx = i;
                if(j < tsy) tsy = j;
                if(i > tsw) tsw = i;
                if(j > tsh) tsh = j;
                ts++;
            }
        }
    tsw = tsw - tsx + 1;
    tsh = tsh - tsy + 1;
    tcw = tch = 0;
}

/**
 * "Paint", set one pixel or a brush at given position, with left click to color, with right click clear
 */
void toolbox_paint(int sx, int sy, int sw, int sh, int x, int y, uint8_t idxs, uint8_t idxe)
{
    int i, j, w, h;
    if(!tb || tp < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || x < 0 || y < 0 || x >= sw || y >= sh) return;
    if(tc && ts > 0 && !tc[(y + sy + th) * tp + sx + x]) return;
    if(idxs == idxe) {
        tb[(y + sy) * tp + sx + x] = meg4.mode == MEG4_MODE_SPRITE && ((le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_R)) ? 0 : idxs;
    } else {
        if(idxe < idxs) idxe = idxs;
        i = idxs & 31; j = idxe & 31; w = j - i + 1; i = idxs >> 5; j = idxe >> 5; h = j - i + 1;
        x -= w / 2; y -= h / 2;
        for(j = 0; j < h; j++)
            if(y + j >= sy && y + j < sy + sh)
                for(i = 0; i < w; i++)
                    if(x + i >= sx && x + i < sx + sw && (!tc || !ts || tc[(y + j + th) * tp + x + i]))
                        tb[(y + j) * tp + x + i] = (idxs + (j << 5) + i);
    }
}

/**
 * Flood fill continous area at given position using a brush. With SHIFT+click, use random elements from the brush
 */
void _toolbox_fill(int sx, int sy, int sw, int sh, int x, int y, uint8_t idxs, uint8_t idxe)
{
    int i, j, w, h, offs = (y + sy) * tp + sx + x, offs2 = (y + sy + th) * tp + sx + x;
    uint8_t orig;

    if(!tb || tp < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || x < 0 || y < 0 || x >= sw || y >= sh)
        return;
    if((!ts && tb[offs] == idxs) || (tc && ts > 0 && (tc[offs2] & 0x81) != 1)) return;
    orig = tb[offs]; if(tc) tc[offs2] |= 0x80;
    if(idxs == idxe) {
        tb[offs] = idxs;
    } else {
        if(idxe < idxs) idxe = idxs;
        i = idxs & 31; j = idxe & 31; w = j - i + 1; i = idxs >> 5; j = idxe >> 5; h = j - i + 1;
        if(meg4_api_getkey(MEG4_KEY_LSHIFT) || meg4_api_getkey(MEG4_KEY_RSHIFT)) { i = rand(); j = rand(); }
        else { i = x; j = y; }
        tb[offs] = idxs + ((j % h) << 5) + (i % w);
    }
    if(x > 0 && ((!ts && tb[offs - 1] == orig) || (tc && ts > 0 && (tc[offs2 - 1] & 1)))) _toolbox_fill(sx, sy, sw, sh, x - 1, y, idxs, idxe);
    if(x < sw && ((!ts && tb[offs + 1] == orig) || (tc && ts > 0 && (tc[offs2 + 1] & 1)))) _toolbox_fill(sx, sy, sw, sh, x + 1, y, idxs, idxe);
    if(y > 0 && ((!ts && tb[offs - tp] == orig) || (tc && ts > 0 && (tc[offs2 - tp] & 1)))) _toolbox_fill(sx, sy, sw, sh, x, y - 1, idxs, idxe);
    if(y < sh && ((!ts && tb[offs + tp] == orig) || (tc && ts > 0 && (tc[offs2 + tp] & 1)))) _toolbox_fill(sx, sy, sw, sh, x, y + 1, idxs, idxe);
}
void toolbox_fill(int sx, int sy, int sw, int sh, int x, int y, uint8_t idxs, uint8_t idxe)
{
    int i;
    if(!tb || tp < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || x < 0 || y < 0 || x >= sw || y >= sh)
        return;
    _toolbox_fill(sx, sy, sw, sh, x, y, idxs, idxe);
    if(tc) for(i = 0; i < tp * th; i++) tc[tp * th + i] &= 0x7F;
    toolbox_histadd();
}

/**
 * "Pipette", pick one pixel from given position
 */
uint8_t toolbox_pick(int sx, int sy, int sw, int sh, int x, int y)
{
    if(!tb || tp < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || x < 0 || y < 0 || x >= sw || y >= sh) return 0;
    return tb[(y + sy) * tp + sx + x];
}

/**
 * Select all
 */
void toolbox_selall(int sx, int sy, int sw, int sh)
{
    int i, j;

    memset(tc, 0, tp * th * 2);
    for(j = 0; j < sh; j++)
        for(i = 0; i < sw; i++)
            tc[(sy + j + th) * tp + sx + i] = 1;
    toolbox_bounds();
}

/**
 * Invert selection
 */
void toolbox_selinv(int sx, int sy, int sw, int sh)
{
    int i, j;

    for(j = 0; j < sh; j++)
        for(i = 0; i < sw; i++)
            tc[(sy + j + th) * tp + sx + i] ^= 1;
    toolbox_bounds();
}

/**
 * Rectangle selection
 */
void toolbox_selrect(int sx, int sy, int sw, int sh, int x, int y, int w, int h)
{
    int i, j;

    /* make sure we clear the selection as the clipboard, no matter the parameters */
    if(tc && !meg4_api_getkey(MEG4_KEY_LCTRL) && !meg4_api_getkey(MEG4_KEY_RCTRL) &&
        !meg4_api_getkey(MEG4_KEY_LSHIFT) && !meg4_api_getkey(MEG4_KEY_RSHIFT)) {
        memset(tc, 0, tp * th * 2);
        tsx = tsy = tsw = tsh = tcw = tch = ts = 0;
    }
    if(!tc || tp < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || x < 0 || y < 0 || x + w > sw || y + h > sh) return;
    if(w > 0 && h > 0)
        for(j = 0; j < h; j++)
            for(i = 0; i < w; i++)
                tc[(sy + y + j + th) * tp + sx + x + i] =
                    meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL) ? 0 : 1;
    toolbox_bounds();
}

/**
 * Fuzzy selection
 */
void _toolbox_fuzzy(int sx, int sy, int sw, int sh, int x, int y, uint8_t idx)
{
    int offs = (y + sy) * tp + sx + x;

    if(x < 0 || y < 0 || x >= sw || y >= sh || tb[offs] != idx || (tc[th * tp + offs] & 0x80)) return;
    tc[th * tp + offs] = 0x80 | (meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL) ? 0 : 1);
    if(x > 0 && tb[offs - 1] == idx) _toolbox_fuzzy(sx, sy, sw, sh, x - 1, y, idx);
    if(x < sw && tb[offs + 1] == idx) _toolbox_fuzzy(sx, sy, sw, sh, x + 1, y, idx);
    if(y > 0 && tb[offs - tp] == idx) _toolbox_fuzzy(sx, sy, sw, sh, x, y - 1, idx);
    if(y < sh && tb[offs + tp] == idx) _toolbox_fuzzy(sx, sy, sw, sh, x, y + 1, idx);
}
void toolbox_selfuzzy(int sx, int sy, int sw, int sh, int x, int y)
{
    if(!tb || !tc || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || x < 0 || y < 0 || x >= sw || y >= sh)
        return;
    if(!meg4_api_getkey(MEG4_KEY_LCTRL) && !meg4_api_getkey(MEG4_KEY_RCTRL) &&
        !meg4_api_getkey(MEG4_KEY_LSHIFT) && !meg4_api_getkey(MEG4_KEY_RSHIFT)) {
        memset(tc, 0, tp * th * 2);
    }
    _toolbox_fuzzy(sx, sy, sw, sh, x, y, tb[(y + sy) * tp + sx + x]);
    toolbox_bounds();
}

/**
 * Cut
 */
void toolbox_cut(int sx, int sy, int sw, int sh)
{
    toolbox_copy(sx, sy, sw, sh);
    toolbox_del(sx, sy, sw, sh);
}

/**
 * Copy
 */
void toolbox_copy(int sx, int sy, int sw, int sh)
{
    int i, j, o, s = tp * th;

    if(!tb || !tc || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    memset(tc, 0, tp * th);
    if(!ts || tsx < 0 || tsy < 0 || tsw < 1 || tsh < 1) {
        for(j = 0, o = sy * tp + sx; j < sh; j++, o += tp)
            memcpy(tc + j * tp, tb + o, sw);
        tcw = sw; tch = sh;
    } else {
        for(j = 0, o = tsy * tp + tsx; j < tsh; j++, o += tp)
            for(i = 0; i < tsw; i++)
                if(tc[s + o + i])
                    tc[j * tp + i] = tb[o + i];
        tcw = tsw; tch = tsh;
    }
}

/**
 * Paste
 */
void toolbox_paste(int sx, int sy, int sw, int sh, int px, int py)
{
    int i, j, k, l;

    if(!tb || !tc || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || px < 0 || py < 0 ||
      px >= sw || py >= sh || tcw < 1 || tch < 1) return;
    for(j = 0; j < tch; j++) {
        k = j + (tch != sh ? py - tch / 2 : 0);
        if(k >= 0 && k < sh)
            for(i = 0; i < tcw; i++) {
                l = i + (tcw != sw ? px - tcw / 2 : 0);
                if(l >= 0 && l < sw && (!ts || tc[(tsy + j + th) * tp + tsx + i]))
                    tb[(sy + k) * tp + sx + l] = tc[j * tp + i];
            }
    }
}

/**
 * Draw the pasted area
 */
void toolbox_pasteview(int sx, int sy, int sw, int sh, int x, int y, int w, int h, int px, int py)
{
    int i, j, k, l, D;
    uint32_t c;
    uint8_t *s, *d, *b = (uint8_t*)&c;

    if(!tc || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th || px < 0 || py < 0 ||
      px >= w || py >= h || tcw < 1 || tch < 1 || tsx < 0 || tsy < 0)
        return;
    /* this is tricky, because we have to reproduce the same integer rounding errors as with selection */
    for(j = 0; j < h; j++) {
        k = (j * sh / h) - py + tch / 2;
        if(k >= 0 && k < tch) {
            s = tc + k * tp;
            d = (uint8_t*)meg4.valt + (y + j) * 2560 + x * 4;
            for(i = 0; i < w; i++, d += 4) {
                l = (i * sw / w) - px + tcw / 2;
                if(l < 0 || l >= tcw || !*(s + l) || (ts && !tc[(k + tsy + th) * tp + tsx + l])) continue;
                c = meg4.mmio.palette[(int)*(s + l)];
                b[3] >>= 2;
                if(!b[3]) continue;
                D = 256 - b[3];
                d[2] = (b[2]*b[3] + D*d[2]) >> 8; d[1] = (b[1]*b[3] + D*d[1]) >> 8; d[0] = (b[0]*b[3] + D*d[0]) >> 8;
            }
        }
    }
}

/**
 * Delete
 */
void toolbox_del(int sx, int sy, int sw, int sh)
{
    int i, j, o, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    if(!tc || !ts)
        for(j = 0, o = sy * tp + sx; j < sh; j++, o += tp)
            memset(tb + o, 0, sw);
    else
        for(j = 0, o = sy * tp + sx; j < sh; j++, o += tp)
            for(i = 0; i < sw; i++)
                if(tc[s + o + i])
                    tb[o + i] = 0;
    toolbox_histadd();
}

/**
 * Shift to the left
 */
void toolbox_shl(int sx, int sy, int sw, int sh)
{
    uint8_t tmp = 0;
    int i, j, k, o, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    for(j = 0; j < sh; j++) {
        for(i = 0, k = -1; i < sw; i++) {
            o = (sy + j) * tp + sx + i;
            if(!ts || !tc || tc[s + o]) {
                if(k == -1) tmp = tb[o];
                else tb[k] = tb[o];
                k = o;
            }
        }
        if(k != -1) tb[k] = tmp;
    }
    toolbox_histadd();
}

/**
 * Shift upwards
 */
void toolbox_shu(int sx, int sy, int sw, int sh)
{
    uint8_t tmp = 0;
    int i, j, k, o, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    for(i = 0; i < sw; i++) {
        for(j = 0, k = -1; j < sh; j++) {
            o = (sy + j) * tp + sx + i;
            if(!ts || !tc || tc[s + o]) {
                if(k == -1) tmp = tb[o];
                else tb[k] = tb[o];
                k = o;
            }
        }
        if(k != -1) tb[k] = tmp;
    }
    toolbox_histadd();
}

/**
 * Shift downwards
 */
void toolbox_shd(int sx, int sy, int sw, int sh)
{
    uint8_t tmp = 0;
    int i, j, k, o, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    for(i = 0; i < sw; i++) {
        for(j = sh - 1, k = -1; j >= 0; j--) {
            o = (sy + j) * tp + sx + i;
            if(!ts || !tc || tc[s + o]) {
                if(k == -1) tmp = tb[o];
                else tb[k] = tb[o];
                k = o;
            }
        }
        if(k != -1) tb[k] = tmp;
    }
    toolbox_histadd();
}

/**
 * Shift to the right
 */
void toolbox_shr(int sx, int sy, int sw, int sh)
{
    uint8_t tmp = 0;
    int i, j, k, o, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    for(j = 0; j < sh; j++) {
        for(i = sw - 1, k = -1; i >= 0; i--) {
            o = (sy + j) * tp + sx + i;
            if(!ts || !tc || tc[s + o]) {
                if(k == -1) tmp = tb[o];
                else tb[k] = tb[o];
                k = o;
            }
        }
        if(k != -1) tb[k] = tmp;
    }
    toolbox_histadd();
}

/**
 * Rotate clock-wise
 */
void toolbox_rcw(int sx, int sy, int sw, int sh)
{
    int i, j, x, y, w, h;

    if(!tb || !tc || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    if(ts && tsw > 0 && tsh > 0) {
        x = tsx; y = tsy; w = tsw; h = tsh;
    } else {
        x = sx; y = sy; w = sw; h = sh;
    }
    if(w < 1 || h < 1 || w != h) {
        meg4.mmio.ptrspr = MEG4_PTR_ERR;
        return;
    }
    memcpy(tc, tb, tp * th);
    for(j = 0; j < h; j++)
        for(i = 0; i < w; i++)
            tb[(y + j) * tp + x + i] = tc[(y + h - i - 1) * tp + x + j];
    memset(tc, 0, tp * th);
    tcw = tch = 0;
    toolbox_histadd();
}

/**
 * Flip vertically
 */
void toolbox_flv(int sx, int sy, int sw, int sh)
{
    uint8_t tmp = 0;
    int i, j, f, l, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    for(i = 0; i < sw; i++) {
        for(j = f = 0, l = sh - 1; j < sh && f < l; j++, f++, l--) {
            while(f < l && ts && tc && !tc[s + (sy + f) * tp + sx + i]) f++;
            while(f < l && ts && tc && !tc[s + (sy + l) * tp + sx + i]) l--;
            if(f < l) {
                tmp = tb[(sy + f) * tp + sx + i];
                tb[(sy + f) * tp + sx + i] = tb[(sy + l) * tp + sx + i];
                tb[(sy + l) * tp + sx + i] = tmp;
            }
        }
    }
    toolbox_histadd();
}

/**
 * Flip horizontally
 */
void toolbox_flh(int sx, int sy, int sw, int sh)
{
    uint8_t tmp = 0;
    int i, j, f, l, s = tp * th;

    if(!tb || tp < 1 || th < 1 || sx < 0 || sx + sw > tp || sy < 0 || sy + sh > th) return;
    for(j = 0; j < sh; j++) {
        for(i = f = 0, l = sw - 1; i < sw && f < l; i++, f++, l--) {
            while(f < l && ts && tc && !tc[s + (sy + j) * tp + sx + f]) f++;
            while(f < l && ts && tc && !tc[s + (sy + j) * tp + sx + l]) l--;
            if(f < l) {
                tmp = tb[(sy + j) * tp + sx + f];
                tb[(sy + j) * tp + sx + f] = tb[(sy + j) * tp + sx + l];
                tb[(sy + j) * tp + sx + l] = tmp;
            }
        }
    }
    toolbox_histadd();
}

/**
 * Button
 */
void toolbox_btn(int x, int y, int c, int t)
{
    if(t || ((le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L) && le16toh(meg4.mmio.ptry) >= y + 12 && le16toh(meg4.mmio.ptry) < y + 25 &&
      le16toh(meg4.mmio.ptrx) >= x && le16toh(meg4.mmio.ptrx) < x + 13)) {
        meg4_box(meg4.valt, 640, 388, 2560, x, y, 13, 13, theme[THEME_BTN_D], theme[t > 0 ? THEME_SEL_BG : THEME_BTN_BG], theme[THEME_BTN_L], 0, 0, 0, 0, 0);
        meg4_char(meg4.valt, 2560, x + 3, y + 4, theme[t > 0 ? THEME_SEL_FG : THEME_BTN_FG], 1, meg4_font, c);
    } else {
        meg4_box(meg4.valt, 640, 388, 2560, x, y, 13, 13, theme[THEME_BTN_L], theme[THEME_BTN_BG], theme[THEME_BTN_D], 0, 0, 0, 0, 0);
        meg4_char(meg4.valt, 2560, x + 3, y + 3, theme[THEME_BTN_FG], 1, meg4_font, c);
    }
}

/**
 * Add to history
 */
void toolbox_histadd(void)
{
    if(!tb || tp < 1 || th < 1) return;
    curhist = numhist++;
    hist = (uint8_t*)realloc(hist, numhist * tp * th);
    if(!hist) { numhist = curhist = 0; return; }
    memcpy(hist + curhist * tp * th, tb, tp * th);
}

/**
 * Undo
 */
void toolbox_histundo(void)
{
    if(!hist || !numhist) return;
    if(curhist > 0) curhist--;
    memcpy(tb, hist + curhist * tp * th, tp * th);
}

/**
 * Redo
 */
void toolbox_histredo(void)
{
    if(!hist || !numhist) return;
    if(curhist + 1 < numhist) curhist++;
    memcpy(tb, hist + curhist * tp * th, tp * th);
}

/**
 * Initialize toolbox clipboard
 */
void toolbox_init(uint8_t *buf, int w, int h)
{
    tp = w; th = h; ts = 0; tb = buf;
    tc = (uint8_t*)realloc(tc, tp * th * 2);
    if(tc) memset(tc, 0, tp * th * 2);
    if(tb) toolbox_histadd();
}

/**
 * Free resources
 */
void toolbox_free(void)
{
    if(hist) { free(hist); hist = NULL; }
    if(tc) { free(tc); tc = NULL; }
    tp = th = ts = numhist = curhist = 0;
}

/**
 * Controller, returns which button was pressed or -1
 */
int toolbox_ctrl(void)
{
    return (le16toh(meg4.mmio.ptry) >= ty + 12 && le16toh(meg4.mmio.ptry) < ty + 25 && le16toh(meg4.mmio.ptrx) >= tx &&
        le16toh(meg4.mmio.ptrx) < tx + tn * 16) && ((le16toh(meg4.mmio.ptrx) - tx) & 0xf) < 13 ? (le16toh(meg4.mmio.ptrx) - tx) >> 4 : -1;
}

/**
 * Note editor controller
 */
int toolbox_notectrl(uint8_t *note, int snd)
{
    int i, j, ef, p = ps, clk = le16toh(meg4.mmio.ptrbtn) & MEG4_BTN_L, mx = le16toh(meg4.mmio.ptrx), my = le16toh(meg4.mmio.ptry);
    int semi[21] = { 0, -10, -8, 0, -5, -3, -1, 0, 2, 4, 0, 7, 9, 11, 0, 14, 16, 0, 19, 21, 23 };
    int flat[23] = { -12, -11, -9, -7, -6, -4, -2, 0, 1, 3, 5, 6, 8, 10, 12, 13, 15, 17, 18, 20, 22, 24, 25 };

    if(!note || meg4_api_getkey(MEG4_KEY_LCTRL) || meg4_api_getkey(MEG4_KEY_RCTRL)) { pz = pp = 0; return 0; }
    /* failsafes on effects */
    ef = note[2] < 0x10 ? note[2] : note[2] - 0xD0;
    if(ef < 0 || ef > 31 || !tbeffects[ef] || (snd && (note[2] == 0xB || note[2] == 0xF || note[2] == 0xE9)))
        ef = note[2] = note[3] = 0;
    else
        if(tbeffects[ef][0] == 3 && (note[3] & 0xf0)) note[3] &= 0xf0;
    ps = -1;
    if(last && !clk) {
        /* button release */
        if(mx >= px + 12 && mx < px + 24) {
            if(my >= py +  0 && my < py + 12) { if(note[0]) note[0]--; else note[0] = MEG4_NUM_NOTE - 1; } else
            if(my >= py + 16 && my < py + 28) { if(note[1] > snd) note[1]--; else note[1] = 31; } else
            if(my >= py + 32 && my < py + 44) {
                /* switch effect type */
                if(note[2] == 9 || note[2] == 0xE4 || note[2] == 0xE7 || note[2] == 0xE9) note[2] -= 2; else
                if(note[2] == 0xF) note[2] = 0xC; else
                if(note[2] == 0xE1) note[2] = 0xF; else
                if(note[2] == 0) note[2] = 0xED; else note[2]--;
                if(snd) {
                    if(note[2] == 0xB) note[2] = 0xA; else
                    if(note[2] == 0xF) note[2] = 0xC; else
                    if(note[2] == 0xE9) note[2] = 0xE7;
                }
            } else
            if(my >= py + 48 && my < py + 60 && tbeffects[ef]) {
                /* first (or only) effect parameter */
                switch(tbeffects[ef][0]) {
                    case 1: note[3]--; break;
                    case 2: note[3] -= 0x10; break;
                    case 3: note[3] -= 0x10; note[3] &= 0xf0; break;
                    case 4: if(!(note[3] & 0xf)) note[3] = (note[3] & 0xf0)|0xf; else note[3]--; break;
                    case 5: if(!(note[3] & 0xf)) note[3] = (note[3] & 0xf0)|3; else note[3]--; break;
                    case 6: note[3]--; note[3] &= 0xf; break;
                }
            } else
            if(my >= py + 64 && my < py + 76 && tbeffects[ef]) {
                /* second effect parameter */
                switch(tbeffects[ef][0]) {
                    case 2: if(!(note[3] & 0xf)) note[3] = (note[3] & 0xf0)|0xf; else note[3]--; break;
                    case 3: note[3]--; note[3] &= 0xf; break;
                }
            }
        } else
        if(mx >= px + 48 && mx < px + 60) {
            if(my >= py +  0 && my < py + 12) { if(note[0] < MEG4_NUM_NOTE - 1) note[0]++; else note[0] = 0; } else
            if(my >= py + 16 && my < py + 28) { if(note[1] < 31) note[1]++; else note[1] = snd; } else
            if(my >= py + 32 && my < py + 44) {
                /* switch effect type */
                if(note[2] == 7 || note[2] == 0xE2 || note[2] == 0xE5 || note[2] == 0xE7) note[2] += 2; else
                if(note[2] == 0xC) note[2] = 0xF; else
                if(note[2] == 0xF) note[2] = 0xE1; else
                if(note[2] == 0xED) note[2] = 0; else note[2]++;
                if(snd) {
                    if(note[2] == 0xB) note[2] = 0xC; else
                    if(note[2] == 0xF) note[2] = 0xE1; else
                    if(note[2] == 0xE9) note[2] = 0xEA;
                }
            } else
            if(my >= py + 48 && my < py + 60 && tbeffects[ef]) {
                /* first (or only) effect parameter */
                switch(tbeffects[ef][0]) {
                    case 1: note[3]++; break;
                    case 2: note[3] += 0x10; break;
                    case 3: note[3] += 0x10; note[3] &= 0xf0; break;
                    case 4: if((note[3] & 0xf) == 0xf) note[3] &= 0xf0; else note[3]++; break;
                    case 5: if((note[3] & 0xf) == 3) note[3] &= 0xf0; else note[3]++; break;
                    case 6: note[3]++; note[3] &= 0xf; break;
                }
            } else
            if(my >= py + 64 && my < py + 76 && tbeffects[ef]) {
                /* second effect parameter */
                switch(tbeffects[ef][0]) {
                    case 2: if((note[3] & 0xf) == 0xf) note[3] &= 0xf0; else note[3]++; break;
                    case 3: note[3]++; note[3] &= 0xf; break;
                }
            }
        } else
        if(mx >= px + 72 && mx < px + 84 && my >= py + 32 && my < py + 46) { if(po > 0) po--; } else
        if(mx >= px + 600 && mx < px + 612 && my >= py + 32 && my < py + 46) { if(po < 7) po++; }
    } else
    if(clk) {
        /* piano keyboard click */
        if(my >= py && my < py + 48)
            for(i = 1; i < 21 && ps == -1; i++) {
                j = po + (i < 8 ? -1 : (i < 15 ? 0 : (i < 22 ? 1 : 2)));
                if(i != 3 && i != 7 && i != 10 && i != 14 && i != 17 && j >= 0 && j <= 7 &&
                  mx >= px + 100 + i * 22 && mx < px + 120 + i * 22) {
                    ps = 23 + i; note[0] = 12 * po + semi[i]; break;
                }
            }
        if(my >= py && my < py + 80 && ps == -1)
            for(i = 0; i < 23 && ps == -1; i++) {
                j = po + (i < 1 ? -2 : (i < 8 ? -1 : (i < 15 ? 0 : (i < 22 ? 1 : 2))));
                if(j >= 0 && j <= 7 && mx >= px + 90 + i * 22 && mx < px + 110 + i * 22) {
                    ps = i; note[0] = 12 * po + flat[i]; break;
                }
            }
        if(snd && note[0] && !note[1]) note[1] = 1;
    } else
    if(!last && !clk) {
        i = meg4_api_getkey(MEG4_KEY_LSHIFT) || meg4_api_getkey(MEG4_KEY_RSHIFT) ? 10 : 0;
        if(meg4_api_getkey(MEG4_KEY_BACKSPACE)) { note[0] = note[1] = note[2] = note[3] = 0; } else
        /* waveform selection keys */
        if(meg4_api_getkey(MEG4_KEY_1)) { note[1] = 1 + i; } else
        if(meg4_api_getkey(MEG4_KEY_2)) { note[1] = 2 + i; } else
        if(meg4_api_getkey(MEG4_KEY_3)) { note[1] = 3 + i; } else
        if(meg4_api_getkey(MEG4_KEY_4)) { note[1] = 4 + i; } else
        if(meg4_api_getkey(MEG4_KEY_5)) { note[1] = 5 + i; } else
        if(meg4_api_getkey(MEG4_KEY_6)) { note[1] = 6 + i; } else
        if(meg4_api_getkey(MEG4_KEY_7)) { note[1] = 7 + i; } else
        if(meg4_api_getkey(MEG4_KEY_8)) { note[1] = 8 + i; } else
        if(meg4_api_getkey(MEG4_KEY_9)) { note[1] = 9 + i; } else
        if(meg4_api_getkey(MEG4_KEY_0)) { note[1] = 10 + i; } else
        /* change octave */
        if(!pz && meg4_api_getkey(MEG4_KEY_Z)) { if(po > 0) po--; } else
        if(!pp && meg4_api_getkey(MEG4_KEY_PERIOD)) { if(po < 7) po++; } else
        /* piano keyboard keys */
        if(meg4_api_getkey(MEG4_KEY_X)) { note[0] = 12 * po + 1; ps = 8; } else
        if(meg4_api_getkey(MEG4_KEY_D)) { note[0] = 12 * po + 2; ps = 31; } else
        if(meg4_api_getkey(MEG4_KEY_C)) { note[0] = 12 * po + 3; ps = 9; } else
        if(meg4_api_getkey(MEG4_KEY_F)) { note[0] = 12 * po + 4; ps = 32; } else
        if(meg4_api_getkey(MEG4_KEY_V)) { note[0] = 12 * po + 5; ps = 10; } else
        if(meg4_api_getkey(MEG4_KEY_B)) { note[0] = 12 * po + 6; ps = 11; } else
        if(meg4_api_getkey(MEG4_KEY_H)) { note[0] = 12 * po + 7; ps = 34; } else
        if(meg4_api_getkey(MEG4_KEY_N)) { note[0] = 12 * po + 8; ps = 12; } else
        if(meg4_api_getkey(MEG4_KEY_J)) { note[0] = 12 * po + 9; ps = 35; } else
        if(meg4_api_getkey(MEG4_KEY_M)) { note[0] = 12 * po +10; ps = 13; } else
        if(meg4_api_getkey(MEG4_KEY_K)) { note[0] = 12 * po +11; ps = 36; } else
        if(meg4_api_getkey(MEG4_KEY_COMMA)) { note[0] = 12 * po +12; ps = 14; }
        /* effects (with some sane parameter defaults) */
        if(meg4_api_getkey(MEG4_KEY_Q)) { note[2] = note[3] = 0; } else
        if(meg4_api_getkey(MEG4_KEY_W)) { note[2] = 0; note[3] = 0x37; } else
        if(meg4_api_getkey(MEG4_KEY_E)) { note[2] = 0; note[3] = 0x47; } else
        if(meg4_api_getkey(MEG4_KEY_R)) { note[2] = 1; note[3] = 0x14; } else
        if(meg4_api_getkey(MEG4_KEY_T)) { note[2] = 1; note[3] = 0x0A; } else
        if(meg4_api_getkey(MEG4_KEY_Y)) { note[2] = 2; note[3] = 0x0A; } else
        if(meg4_api_getkey(MEG4_KEY_U)) { note[2] = 2; note[3] = 0x14; } else
        if(meg4_api_getkey(MEG4_KEY_I)) { note[2] = 4; note[3] = 0x63; } else
        if(meg4_api_getkey(MEG4_KEY_O)) { note[2] = 4; note[3] = 0x6F; } else
        if(meg4_api_getkey(MEG4_KEY_P)) { note[2] = 7; note[3] = 0x63; } else
        if(meg4_api_getkey(MEG4_KEY_LBRACKET)) { note[2] = 7; note[3] = 0x6F; } else
        if(meg4_api_getkey(MEG4_KEY_RBRACKET)) { note[2] = 0xC; note[3] = 0; }
        if(snd) {
            if(note[0] && !note[1]) note[1] = 1; else
            if(!note[0] && note[1]) note[0] = MEG4_NOTE_C_4;
        }
    }
    pz = meg4_api_getkey(MEG4_KEY_Z);
    pp = meg4_api_getkey(MEG4_KEY_PERIOD);
    if(p != ps) {
        if(ps == -1) { meg4_playnote(note, 0); return -1; }
        else { meg4_playnote(note, 255); return 1; }
    }
    return 0;
}

/**
 * View
 */
void toolbox_view(int x, int y, int type)
{
    int sintbl[] = { 0x14, 0x20, 0x2c, 0x36, 0x3d, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3d, 0x36, 0x2c, 0x20 };
    ta = sintbl[(le16toh(meg4.mmio.tick) >> 7) & 15];
    tx = x; ty = y; tn = type < 0 ? 7 : 12;
    toolbox_btn(x, y, 0x1b, 0);
    toolbox_btn(x + 16, y, 0x18, 0);
    toolbox_btn(x + 32, y, 0x19, 0);
    toolbox_btn(x + 48, y, 0x1a, 0);
    toolbox_btn(x + 64, y, 0x1c, 0);
    toolbox_btn(x + 80, y, 0x12, 0);
    toolbox_btn(x + 96, y, 0x1d, 0);
    if(type >= 0) {
        toolbox_btn(x + 112, y, 0x13, type == 0);
        toolbox_btn(x + 128, y, 0x14, type == 1);
        toolbox_btn(x + 144, y, 0x15, type == 2);
        toolbox_btn(x + 160, y, 0x16, type == 3);
        toolbox_btn(x + 176, y, 0x17, type == 4);
    }
}

/**
 * Blit (possibly scaled up) sprites to screen
 */
void toolbox_blit(int x, int y, int dp, int w, int h, int sx, int sy, int sw, int sh, int sp, int chk)
{
    int i, j, k, l, sR, sG, sB, sA = 256 - ta, D;
    uint8_t *s, *d, *b, *cS = (uint8_t*)&theme[THEME_SEL_BG], bg[4];

    if(w < 1 || h < 1 || sw < 1 || sh < 1) return;
    sR = cS[0] * ta; sG = cS[1] * ta; sB = cS[2] * ta;
    memcpy(bg, &theme[THEME_INP_BG], 4); bg[0] = (bg[0] + 4) & 0x7f; bg[1] = (bg[1] + 4) & 0x7f; bg[2] = (bg[2] + 4) & 0x7f;
    for(j = 0; j < h; j++) {
        if(y + j >= 0 && y + j < 388) {
            k = (j * sh / h);
            s = meg4.mmio.sprites + (sy + k) * sp + sx;
            d = (uint8_t*)meg4.valt + (y + j) * dp + x * 4;
            for(i = 0; i < w; i++, d += 4) {
                if(x + i < 0 || x + i >= 630) continue;
                l = i * sw / w;
                if(!*(s + l)) {
                    b = (k & 1) ^ (l & 1) ? bg : (uint8_t*)&theme[THEME_INP_BG];
                    d[2] = b[2]; d[1] = b[1]; d[0] = b[0];
                } else {
                    b = (uint8_t*)&meg4.mmio.palette[(int)*(s + l)];
                    D = 256 - b[3];
                    d[2] = (b[2]*b[3] + D*d[2]) >> 8; d[1] = (b[1]*b[3] + D*d[1]) >> 8; d[0] = (b[0]*b[3] + D*d[0]) >> 8;
                }
                d[3] = 255;
                if(chk == -1 || !tc || !tc[(sy + th + k) * sp + sx + l]) continue;
                d[2] = (sB + sA*d[2]) >> 8; d[1] = (sG + sA*d[1]) >> 8; d[0] = (sR + sA*d[0]) >> 8;
            }
        }
    }
}

/**
 * Blit one (possibly scaled down) sprite to screen
 */
void toolbox_spr(int sx, int sy, int sw, int idx, int a)
{
    int i, j, k, l, dp = 2560, xs = 0, sp = 0, sd = 0, ss = 0, R, G, B, A, D, sR, sG, sB, sA = 256 -ta;
    uint8_t *s, *d, *e, *f, *b, *cS = (uint8_t*)&theme[THEME_SEL_BG], bg[4];

    switch(sw) {
        case 16: sp = 256; sd = 1; ss = 0; dp = 5120; xs = 1; break;
        case 8: sp = 256; sd = 1; ss = 0; break;
        case 4: sp = 512; sd = 2; ss = 2; break;
        case 2: sp = 1024; sd = 4; ss = 4; break;
        case 1: sp = 2048; sd = 8; ss = 6; break;
        default: return;
    }
    sR = cS[0] * ta; sG = cS[1] * ta; sB = cS[2] * ta;
    memcpy(bg, &theme[THEME_INP_BG], 4); bg[0] = (bg[0] + 4) & 0x7f; bg[1] = (bg[1] + 4) & 0x7f; bg[2] = (bg[2] + 4) & 0x7f;
    f = (uint8_t*)meg4.valt + sy * 2560 + sx * 4;
    s = meg4.mmio.sprites + (((idx >> 5) << 11) + ((idx & 31) << 3));
    for(j = 0; j < sw && sy + j < le16toh(meg4.mmio.cropy1); j += xs + 1, s += sp, f += dp)
        if(sy + j + xs >= le16toh(meg4.mmio.cropy0)) {
            for(e = s, d = f, i = 0; i < sw; i += xs + 1, e += sd, d += xs ? 8 : 4)
                if(sx + i + xs >= le16toh(meg4.mmio.cropx0) && sx + i < le16toh(meg4.mmio.cropx1)) {
                    if(idx) {
                        R = G = B = A = 0;
                        for(k = 0; k < sd; k++)
                            for(l = 0; l < sd; l++) {
                                b = (uint8_t*)&meg4.mmio.palette[(int)*(e + k * 256 + l)];
                                R += b[0]; G += b[1]; B += b[2]; A += b[3];
                            }
                        R >>= ss; G >>= ss; B >>= ss; A >>= ss;
                        if(a > 0) A >>= a;
                        D = 256 - A;
                        d[2] = (B*A + D*d[2]) >> 8; d[1] = (G*A + D*d[1]) >> 8; d[0] = (R*A + D*d[0]) >> 8;
                        if(xs) {
                            d[6] = (B*A + D*d[6]) >> 8; d[5] = (G*A + D*d[5]) >> 8; d[4] = (R*A + D*d[4]) >> 8;
                            d[2562] = (B*A + D*d[2562]) >> 8; d[2561] = (G*A + D*d[2561]) >> 8; d[2560] = (R*A + D*d[2560]) >> 8;
                            d[2566] = (B*A + D*d[2566]) >> 8; d[2565] = (G*A + D*d[2565]) >> 8; d[2564] = (R*A + D*d[2564]) >> 8;
                        }
                    } else {
                        if(a > 0) continue;
                        b = a == -1 || a == -3 ? bg : (uint8_t*)&theme[THEME_INP_BG];
                        d[0] = b[0]; d[1] = b[1]; d[2] = b[2];
                        if(xs) {
                            d[4] = d[2560] = d[2564] = b[0];
                            d[5] = d[2561] = d[2565] = b[1];
                            d[6] = d[2562] = d[2566] = b[2];
                        }
                    }
                    if(a < -1) {
                        d[2] = (sB + sA*d[2]) >> 8; d[1] = (sG + sA*d[1]) >> 8; d[0] = (sR + sA*d[0]) >> 8;
                        if(xs) {
                            d[6] = (sB + sA*d[6]) >> 8; d[5] = (sG + sA*d[5]) >> 8; d[4] = (sR + sA*d[4]) >> 8;
                            d[2562] = (sB + sA*d[2562]) >> 8; d[2561] = (sG + sA*d[2561]) >> 8; d[2560] = (sR + sA*d[2560]) >> 8;
                            d[2566] = (sB + sA*d[2566]) >> 8; d[2565] = (sG + sA*d[2565]) >> 8; d[2564] = (sR + sA*d[2564]) >> 8;
                        }
                    }
                }
        }
}

/**
 * Blit map to screen
 */
void toolbox_map(int x, int y, int dp, int w, int h, int mx, int my, int zoom, int mapsel, int px, int py, int sprs, int spre)
{
    int i, j, k, dx, dy, npix = 16 >> zoom, xs, xd = 0, sw = 0, sh = 0, ex, ey, x0, x1, y0, y1;

    x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = htole16(x);
    x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(x + w);
    y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = htole16(y);
    y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(y + h);
    dx = x - mx; dy = y - my; xs = npix == 16 ? 8 : npix; xd = npix == 16 ? 4 : 0;
    if(px < 0 || py < 0) sprs = 256;
    if(sprs == -1 && ts && tc && tcw > 0 && tch > 0) { sw = tcw; sh = tch; } else
    if(sprs >= 0 && sprs < 256) { i = sprs & 31; j = spre & 31; sw = j - i + 1; i = sprs >> 5; j = spre >> 5; sh = j - i + 1; }
    px -= sw / 2; ex = px + sw;
    py -= sh / 2; ey = py + sh;
    for(j = 0; j < 200 && dy + j * npix < y + h; j++)
        if(dy + (j + 1) * npix >= y)
            for(i = 0; i < 320; i++) {
                k = meg4.mmio.map[j * 320 + i];
                if(dx + (i + 1) * npix >= x && dx + i * npix < x + w) {
                    toolbox_spr(dx + i * npix, dy + j * npix, npix, (mapsel << 8) | k, ((j & 1) ^ (i & 1) ? -1 : 0) +
                        (ts && tc && tc[(j + th) * tp + i] ? -2 : 0));
                    if(i >= px && i < ex && j >= py && j < ey) {
                        if(sprs == -1 && tc) {
                            if(tc[((j - py + tsy + th) * 320) + i - px + tsx]) {
                                if(!tc[((j - py) * 320) + i - px])
                                    meg4_blit(meg4.valt, dx + i * npix, dy + j * npix, dp, npix, npix, meg4_edicons.buf, 144, 48, meg4_edicons.w * 4, 1);
                                else
                                    toolbox_spr(dx + i * npix, dy + j * npix, npix, (mapsel << 8) | tc[((j - py) * 320) + i - px], 1);
                            }
                        } else
                        if(!sprs && !(j - py) && !(i - px))
                            meg4_blit(meg4.valt, dx + i * npix + xd, dy + j * npix + xd, dp, xs, xs, meg4_edicons.buf, 144, 48, meg4_edicons.w * 4, 1);
                        else if(sprs < 256)
                            toolbox_spr(dx + i * npix, dy + j * npix, npix, (mapsel << 8) | (sprs + (j - py) * 32 + i - px), 1);
                    }
                }
            }
    meg4.mmio.cropx0 = x0; meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
}

/**
 * Display one note, a pitch-wave-effect triplet
 */
void toolbox_note(int x, int y, int dp, uint32_t fg, uint8_t *note)
{
    char tmp[8];

    meg4_text(meg4.valt, x, y, dp, fg, 0, -1, meg4_font, tbnotes[(uint32_t)note[0]]);
    if(note[1]) {
        sprintf(tmp, "%02X", note[1]);
        meg4_text(meg4.valt, x + 32, y, dp, fg, 0, 1, meg4_font, tmp);
    } else
        meg4_text(meg4.valt, x + 32, y, dp, fg, 0, 1, meg4_font, "..");
    if(note[2] || note[3]) {
        if(note[2] < 0x10) sprintf(tmp, "%X%02X", note[2], note[3]);
        else sprintf(tmp, "%02X%x", note[2], note[3]);
        meg4_text(meg4.valt, x + 48, y, dp, fg, 0, 1, meg4_font, tmp);
    } else
        meg4_text(meg4.valt, x + 48, y, dp, fg, 0, 1, meg4_font, "...");
}

/**
 * Display the note editor
 */
void toolbox_noteview(int x, int y, int dp, int channel, uint8_t *note)
{
    uint32_t fg, bg;
    int i, j, p = ps, o = po, keys[12] = { 8, 31, 9, 32, 10, 11, 34, 12, 35, 13, 36, 14 };
    int ef, wf[] = { 1, 3, 4, 7 };
    char tmp[8];

    if(!note || dp < 4) return;
    px = x; py = y + 12;
    /* show the pressed key on piano when playing music */
    if(channel != -1 && meg4.dalt.ch[channel].note && meg4.dalt.ch[channel].master && meg4.dalt.ch[channel].tremolo >= 16 &&
      meg4.dalt.ch[channel].sample && meg4.dalt.ch[channel].position >= 0.0f) {
        /* instead of the current note in note[0], use channel's remembered note */
        i = meg4.dalt.ch[channel].note - 1;
        o = i / 12; p = keys[i % 12];
        if(o == po - 1) { o = po; p -= 7; } else
        if(o == po + 1) { o = po; p += 7; }
    }

    /* pitch */
    meg4_blit(meg4.valt, x, y + 2, 2560, 8, 8, meg4_edicons.buf, 64, 56, meg4_edicons.w * 4, 1);
/*    meg4_char(meg4.valt, dp, x, y + 2, theme[THEME_FG], 1, meg4_font, 9);*/
    toolbox_btn(x + 12, y, 0x11, 0);
    meg4_text(meg4.valt, x + 28, y + 2, dp, theme[THEME_FG], 0, 1, meg4_font, tbnotes[(uint32_t)note[0]]);
    toolbox_btn(x + 48, y, 0x10, 0);

    /* waveform */
    meg4_blit(meg4.valt, x, y + 18, 2560, 8, 8, meg4_edicons.buf, 72, 56, meg4_edicons.w * 4, 1);
/*    meg4_char(meg4.valt, dp, x, y + 18, theme[THEME_FG], 1, meg4_font, 5);*/
    toolbox_btn(x + 12, y + 16, 0x11, 0);
    sprintf(tmp, "%02X", note[1]);
    meg4_text(meg4.valt, x + 46 - meg4_width(meg4_font, 1, tmp, NULL), y + 18, dp, theme[THEME_FG], 0, 1, meg4_font, tmp);
    toolbox_btn(x + 48, y + 16, 0x10, 0);

    /* effects */
    meg4_blit(meg4.valt, x, y + 34, 2560, 8, 8, meg4_edicons.buf, 80, 56, meg4_edicons.w * 4, 1);
/*    meg4_char(meg4.valt, dp, x, y + 34, theme[THEME_FG], 1, meg4_font, 8);*/
    ef = note[2] < 0x10 ? note[2] : note[2] - 0xD0;
    if(ef >= 0 && ef < 32 && tbeffects[ef]) {
        toolbox_btn(x + 12, y + 32, 0x11, 0);
        meg4_text(meg4.valt, x + 28, y + 34, dp, theme[THEME_FG], 0, 1, meg4_font, tbeffects[ef] + 1);
        toolbox_btn(x + 48, y + 32, 0x10, 0);
        toolbox_btn(x + 12, y + 48, 0x11, 0);
        switch(tbeffects[ef][0]) {
            case 1: sprintf(tmp, "%d", note[3]); break;
            case 2: case 3: sprintf(tmp, "%d", note[3] >> 4); break;
            case 4: sprintf(tmp, "%d", note[3] & 0xf); break;
            case 5: tmp[0] = '0' + (note[3] & 3); tmp[1] = wf[(uint32_t)note[3] & 3]; tmp[2] = ' '; tmp[3] = 0; break;
            case 6: sprintf(tmp, "%d", (note[3] & 0xf) < 7 ? note[3] & 0xf : (note[3] & 0xf) - 16); break;
        }
        meg4_text(meg4.valt, x + 46 - meg4_width(meg4_font, 1, tmp, NULL), y + 50, dp, theme[THEME_FG], 0, 1, meg4_font, tmp);
        toolbox_btn(x + 48, y + 48, 0x10, 0);
        if(tbeffects[ef][0] == 1 || tbeffects[ef][0] >= 4) {
            meg4_box(meg4.valt, 640, 388, dp, x + 12, y + 64, 13, 13, 0, theme[THEME_L], 0, 0, 0, 0, 0, 0);
            meg4_box(meg4.valt, 640, 388, dp, x + 48, y + 64, 13, 13, 0, theme[THEME_L], 0, 0, 0, 0, 0, 0);
        } else {
            toolbox_btn(x + 12, y + 64, 0x11, 0);
            switch(tbeffects[ef][0]) {
                case 2: sprintf(tmp, "%d", note[3] & 0xf); break;
                case 3: sprintf(tmp, "%d", -(note[3] & 0xf)); break;
            }
            meg4_text(meg4.valt, x + 46 - meg4_width(meg4_font, 1, tmp, NULL), y + 66, dp, theme[THEME_FG], 0, 1, meg4_font, tmp);
            toolbox_btn(x + 48, y + 64, 0x10, 0);
        }
    }

    /* octave selectors */
    i = !meg4_api_getkey(MEG4_KEY_LCTRL) && !meg4_api_getkey(MEG4_KEY_RCTRL) &&
        !meg4_api_getkey(MEG4_KEY_LSHIFT) && !meg4_api_getkey(MEG4_KEY_RSHIFT);
    toolbox_btn(x + 72, y + 32, 0x11, i && meg4_api_getkey(MEG4_KEY_Z) ? -1 : 0);
    toolbox_btn(x + 600, y + 32, 0x10, i && meg4_api_getkey(MEG4_KEY_PERIOD) ? -1 : 0);

    /* piano */
    tmp[1] = '-'; tmp[3] = 0;
    for(i = 0; i < 23; i++) {
        j = o + (i < 1 ? -2 : (i < 8 ? -1 : (i < 15 ? 0 : (i < 22 ? 1 : 2))));
        if(j >= 0 && j <= 7) {
            if(p == i) {
                fg = theme[THEME_SEL_FG];
                bg = theme[THEME_SEL_BG];
            } else {
                fg = htole32(0xff040404);
                bg = htole32(0xfffcfcfc);
            }
            meg4_box(meg4.valt, 640, 388, dp, x + 90 + i * 22, y, 20, 80, htole32(0xffffffff), bg, htole32(0xfff8f8f8), 0, 0, 0, 0, 0);
            tmp[0] = 'A' + ((i + 1) % 7); tmp[2] = '0' + j;
            meg4_text(meg4.valt, x + 93 + i * 22, y + 64, dp, fg, 0, 1, meg4_font, tmp);
        }
    }
    tmp[1] = '#'; tmp[3] = 0;
    for(i = 1; i < 21; i++) {
        j = o + (i < 8 ? -1 : (i < 15 ? 0 : 1));
        if(i != 3 && i != 7 && i != 10 && i != 14 && i != 17 && j >= 0 && j <= 7) {
            if(p == 23 + i) {
                fg = theme[THEME_SEL_FG];
                bg = theme[THEME_SEL_BG];
            } else {
                fg = htole32(0xfff8f8f8);
                bg = htole32(0xff040404);
            }
            meg4_box(meg4.valt, 640, 388, dp, x + 100 + i * 22, y, 20, 48, htole32(0xff080808), bg, htole32(0xff000000), 0, 0, 0, 0, 0);
            tmp[0] = 'A' + ((i + 1) % 7); tmp[2] = '0' + j;
            meg4_text(meg4.valt, x + 103 + i * 22, y + 32, dp, fg, 0, 1, meg4_font, tmp);
        }
    }
}

/**
 * Sets status text
 */
void toolbox_stat(void)
{
    int px = le16toh(meg4.mmio.ptrx), py = le16toh(meg4.mmio.ptry);
    if((py >= ty + 12 && py < ty + 25 && px >= tx && px < tx + tn * 16) && ((px - tx) & 0xf) < 13)
        menu_stat = STAT_SHL + ((px - tx) >> 4);
    if((meg4.mode == MEG4_MODE_SOUND || meg4.mode == MEG4_MODE_MUSIC) && py >= 312 && px >= 10 && px < 70)
        menu_stat = STAT_PITCH + ((py - 312) >> 4);
}
