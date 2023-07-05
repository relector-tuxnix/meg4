/*
 * meg4/gpu.c
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
 * @brief Graphics Processing Unit
 * @chapter Graphics
 *
 * The line, circle and ellipse drawing functions use the Bresenham algorithm
 * from here: http://members.chello.at/%7Eeasyfilter/Bresenham.pdf (but not the
 * Bezier curves, because that's a nonsense what easyfilter does with them.
 * Instead we use a reentrant mid-point method both for quad and cubic,
 * uncomparably faster and provides very good results in 8 iterations)
 *
 * 3D triangle code inspired by https://bellarg.org/TinyGL
 *
 */

#include "meg4.h"
#include "editors/editors.h"    /* for theme[] */
#ifndef NOEDITORS
#include "stb_image_write.h"    /* for the screenshot feature */
#endif
#define EMOJI_IMPL
#include "misc/emoji.h"
#include <math.h>
float cosf(float);
float sinf(float);
float tanf(float);
float fabsf(float);
extern char meg4_kbdtmpbuf[8], meg4_kbdtmpsht, *textinp_cur;
extern uint32_t meg4_lasttick;
extern uint16_t oldsx, oldsy;
void menu_view(uint32_t *dst, int dw, int dh, int dp);
void textinp_view(uint32_t *dst, int dp);
typedef struct { uint16_t d, o, id; float x, y; } maze_spr_t;
static int mazecmp(const void *a, const void *b) { return ((maze_spr_t*)b)->d - ((maze_spr_t*)a)->d; }
static float posX = 0.0, posY = 0.0;
static float prj[16], cam[16], cami[16], camp[3] = { 0.0, 0.25, 1.0 };
static float vpt[3], vps[3];
static uint16_t zbuf[640*400];
static int zclear = 0;

#define clip_funcdef(name, sign, dir, dir1, dir2) \
    static float name(float* c, float* a, float* b) { \
        float t, d[4], den; \
        d[0] = (b[0] - a[0]); \
        d[1] = (b[1] - a[1]); \
        d[2] = (b[2] - a[2]); \
        d[3] = (b[3] - a[3]); \
        den = -(sign d[dir]) + d[3]; \
        if (den == 0) t = 0; else t = (sign a[dir] - a[3]) / den; \
        c[dir1] = a[dir1] + t * d[dir1]; \
        c[dir2] = a[dir2] + t * d[dir2]; \
        c[3] = a[3] + t * d[3]; \
        c[dir] = sign c[3]; \
        return t; \
    }
clip_funcdef(clip_xmin, -, 0, 1, 2)
clip_funcdef(clip_xmax, +, 0, 1, 2)
clip_funcdef(clip_ymin, -, 1, 0, 2)
clip_funcdef(clip_ymax, +, 1, 0, 2)
clip_funcdef(clip_zmin, -, 2, 0, 1)
clip_funcdef(clip_zmax, +, 2, 0, 1)
static float (*clip_proc[6])(float*, float*, float*) = {clip_xmin, clip_xmax, clip_ymin, clip_ymax, clip_zmin, clip_zmax};

typedef struct {
    float pos[3], nor[3], tex[2], ec[4], pc[4], col[4], uz, vz;
    int x, y, z, u, v, r, g, b, a, clip;
} vert_t;

static vert_t verts[256] = { 0 };
static int nvert = 0;

typedef struct {
    int i[3], u[3], v[3], p[3];
} face_t;

static face_t faces[1024] = { 0 };
static int nface = 0;

/**
 * Draw a 3D triangle with gradient
 */
static void draw_triangle_grd(vert_t* v0, vert_t* v1, vert_t* v2)
{
    vert_t *p0 = v0, *p1 = v1, *p2 = v2;
    vert_t *pr1, *pr2, *l1, *l2;
    float fdx1, fdx2, fdy1, fdy2, fz;
    uint16_t* pz1;
    uint32_t* pp1;
    int part, dx1, dy1, dx2, dy2, error, derror, x1, dxdy_min, dxdy_max, x2=0, dx2dy2=0, x, y;
    int z1, dzdx, dzdy, dzdl_min, dzdl_max;
    int r1, drdx, drdy, drdl_min, drdl_max;
    int g1, dgdx, dgdy, dgdl_min, dgdl_max;
    int b1, dbdx, dbdy, dbdl_min, dbdl_max;
    int a1, dadx, dady, dadl_min, dadl_max;
    int X0 = le16toh(meg4.mmio.cropx0), X1 = le16toh(meg4.mmio.cropx1), Y0 = le16toh(meg4.mmio.cropy0), Y1 = le16toh(meg4.mmio.cropy1);

    if(p1->y < p0->y) { p0 = v1; p1 = v0; }
    if(p2->y < p0->y) { p2 = p1; p1 = p0; p0 = v2; } else
    if(p2->y < p1->y) { v1 = p1; p2 = p1; p2 = v1; }

    fdx1 = p1->x - p0->x; fdy1 = p1->y - p0->y; fdx2 = p2->x - p0->x; fdy2 = p2->y - p0->y;
    fz = fdx1 * fdy2 - fdx2 * fdy1;
    if(fz != 0.0 && fz != -0.0) fz = 1.0 / fz;
    fdx1 *= fz; fdy1 *= fz; fdx2 *= fz; fdy2 *= fz;

    {
        float d1, d2;
        {
            d1 = p1->z - p0->z; d2 = p2->z - p0->z;
            dzdx = (int)(fdy2 * d1 - fdy1 * d2);
            dzdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->r - p0->r; d2 = p2->r - p0->r;
            drdx = (int)(fdy2 * d1 - fdy1 * d2);
            drdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->g - p0->g; d2 = p2->g - p0->g;
            dgdx = (int)(fdy2 * d1 - fdy1 * d2);
            dgdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->b - p0->b; d2 = p2->b - p0->b;
            dbdx = (int)(fdy2 * d1 - fdy1 * d2);
            dbdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->a - p0->a; d2 = p2->a - p0->a;
            dadx = (int)(fdy2 * d1 - fdy1 * d2);
            dady = (int)(fdx1 * d2 - fdx2 * d1);
        }
    }

    y = p0->y;
    part = 640 * y;
    pp1 = meg4.screen.buf + part;
    pz1 = zbuf + part;

    for(part = 0; part < 2; part++) {
        int nb_lines;
        {
            register int update_left, update_right;
            if(part == 0) {
                if (fz > 0) { update_left = 1; update_right = 1; l1 = p0; l2 = p2; pr1 = p0; pr2 = p1; }
                else { update_left = 1; update_right = 1; l1 = p0; l2 = p1; pr1 = p0; pr2 = p2; }
                nb_lines = p1->y - p0->y;
            } else {
                if (fz > 0) { update_left = 0; update_right = 1; pr1 = p1; pr2 = p2; }
                else { update_left = 1; update_right = 0; l1 = p1; l2 = p2; }
                nb_lines = p2->y - p1->y + 1;
            }
            if(update_left) {
                {
                    register int tmp;
                    dy1 = l2->y - l1->y; dx1 = l2->x - l1->x;
                    tmp = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
                    x1 = l1->x; error = 0; derror = tmp & 0x0000ffff; dxdy_min = tmp >> 16;
                }
                dxdy_max = dxdy_min + 1;
                z1 = l1->z; dzdl_min = (dzdy + dzdx * dxdy_min); dzdl_max = dzdl_min + dzdx;
                r1 = l1->r; drdl_min = (drdy + drdx * dxdy_min); drdl_max = drdl_min + drdx;
                g1 = l1->g; dgdl_min = (dgdy + dgdx * dxdy_min); dgdl_max = dgdl_min + dgdx;
                b1 = l1->b; dbdl_min = (dbdy + dbdx * dxdy_min); dbdl_max = dbdl_min + dbdx;
                a1 = l1->b; dadl_min = (dady + dadx * dxdy_min); dadl_max = dadl_min + dadx;
            }
            if(update_right) {
                dx2 = (pr2->x - pr1->x); dy2 = (pr2->y - pr1->y);
                dx2dy2 = dy2 > 0 ? (dx2 << 16) / dy2 : 0;
                x2 = pr1->x << 16;
            }
        }
        if(p0->y + nb_lines > Y1) nb_lines = Y1 - p0->y;

        while(nb_lines > 0) {
            nb_lines--;
            if(y >= Y0) {
                register uint8_t* pp;
                register int n;
                register uint16_t* pz;
                register uint32_t z;
                register int or1, og1, ob1, oa1, da1;
                n = (x2 >> 16); if(n > X1) { n = X1; } n -= x1; pp = (uint8_t*)(pp1 + x1); pz = pz1 + x1; z = z1;
                or1 = r1; og1 = g1; ob1 = b1; oa1 = a1; x = x1;
                while(n >= 0) {
                    {
                        register uint32_t zz = z >> 14;
                        if(x >= X0 && zz >= pz[0]) {
                            da1 = 255 - oa1;
                            pp[2] = (ob1*oa1 + da1*pp[2]) >> 8;
                            pp[1] = (og1*oa1 + da1*pp[1]) >> 8;
                            pp[0] = (or1*oa1 + da1*pp[0]) >> 8;
                            pz[0] = zz;
                        }
                    }
                    z += dzdx; og1 += dgdx; or1 += drdx; ob1 += dbdx; oa1 += dadx; pz++; pp += 4; n--; x++;
                }
            }
            error += derror;
            if(error > 0) {
                error -= 0x10000;
                x1 += dxdy_max; z1 += dzdl_max; r1 += drdl_max; g1 += dgdl_max; b1 += dbdl_max; a1 += dadl_max;
            } else {
                x1 += dxdy_min; z1 += dzdl_min; r1 += drdl_min; g1 += dgdl_min; b1 += dbdl_min; a1 += dadl_min;
            }
            x2 += dx2dy2; pp1 += 640; pz1 += 640; y++;
        }
    }
}

/**
 * Draw a 3D triangle with texture
 */
static void draw_triangle_tex(vert_t* v0, vert_t* v1, vert_t* v2)
{
    vert_t *p0 = v0, *p1 = v1, *p2 = v2;
    vert_t *pr1, *pr2, *l1, *l2;
    float fdx1, fdx2, fdy1, fdy2, fz;
    uint16_t* pz1;
    uint32_t* pp1;
    int part, dx1, dy1, dx2, dy2, error, derror, x1, dxdy_min, dxdy_max, x2=0, dx2dy2=0, x, y;
    int z1, dzdx, dzdy, dzdl_min, dzdl_max;
    int r1, drdx, drdy, drdl_min, drdl_max;
    int g1, dgdx, dgdy, dgdl_min, dgdl_max;
    int b1, dbdx, dbdy, dbdl_min, dbdl_max;
    int X0 = le16toh(meg4.mmio.cropx0), X1 = le16toh(meg4.mmio.cropx1), Y0 = le16toh(meg4.mmio.cropy0), Y1 = le16toh(meg4.mmio.cropy1);
    float sz1, dszdx, dszdy, dszdl_min, dszdl_max;
    float tz1, dtzdx, dtzdy, dtzdl_min, dtzdl_max;
    float fdzdx;

    if(p1->y < p0->y) { p0 = v1; p1 = v0; }
    if(p2->y < p0->y) { p2 = p1; p1 = p0; p0 = v2; } else
    if(p2->y < p1->y) { v1 = p1; p2 = p1; p2 = v1; }

    fdx1 = p1->x - p0->x; fdy1 = p1->y - p0->y; fdx2 = p2->x - p0->x; fdy2 = p2->y - p0->y;
    fz = fdx1 * fdy2 - fdx2 * fdy1;
    if(fz != 0.0 && fz != -0.0) fz = 1.0 / fz;

    {
        float d1, d2;
        {
            d1 = p1->z - p0->z; d2 = p2->z - p0->z;
            dzdx = (int)(fdy2 * d1 - fdy1 * d2);
            dzdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->r - p0->r; d2 = p2->r - p0->r;
            drdx = (int)(fdy2 * d1 - fdy1 * d2);
            drdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->g - p0->g; d2 = p2->g - p0->g;
            dgdx = (int)(fdy2 * d1 - fdy1 * d2);
            dgdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->b - p0->b; d2 = p2->b - p0->b;
            dbdx = (int)(fdy2 * d1 - fdy1 * d2);
            dbdy = (int)(fdx1 * d2 - fdx2 * d1);
        }
        {
            float zedzed;
            zedzed = (float)p0->z; p0->uz = (float)p0->u * zedzed; p0->vz = (float)p0->v * zedzed;
            zedzed = (float)p1->z; p1->uz = (float)p1->u * zedzed; p1->vz = (float)p1->v * zedzed;
            zedzed = (float)p2->z; p2->uz = (float)p2->u * zedzed; p2->vz = (float)p2->v * zedzed;
        }
        {
            d1 = p1->uz - p0->uz; d2 = p2->uz - p0->uz;
            dszdx = (fdy2 * d1 - fdy1 * d2); dszdy = (fdx1 * d2 - fdx2 * d1);
        }
        {
            d1 = p1->vz - p0->vz; d2 = p2->vz - p0->vz;
            dtzdx = (fdy2 * d1 - fdy1 * d2); dtzdy = (fdx1 * d2 - fdx2 * d1);
        }
    }

    y = p0->y;
    part = 640 * y;
    pp1 = meg4.screen.buf + part;
    pz1 = zbuf + part;
    fdzdx = (float)dzdx;

    for (part = 0; part < 2; part++) {
        int nb_lines;
        {
            register int update_left, update_right;
            if(part == 0) {
                if (fz > 0) { update_left = 1; update_right = 1; l1 = p0; l2 = p2; pr1 = p0; pr2 = p1; }
                else { update_left = 1; update_right = 1; l1 = p0; l2 = p1; pr1 = p0; pr2 = p2; }
                nb_lines = p1->y - p0->y;
            } else {
                if (fz > 0) { update_left = 0; update_right = 1; pr1 = p1; pr2 = p2; }
                else { update_left = 1; update_right = 0; l1 = p1; l2 = p2; }
                nb_lines = p2->y - p1->y + 1;
            }
            if(update_left) {
                {
                    register int tmp;
                    dy1 = l2->y - l1->y; dx1 = l2->x - l1->x;
                    tmp = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
                    x1 = l1->x; error = 0; derror = tmp & 0x0000ffff; dxdy_min = tmp >> 16;
                }
                dxdy_max = dxdy_min + 1;
                z1 = l1->z; dzdl_min = (dzdy + dzdx * dxdy_min); dzdl_max = dzdl_min + dzdx;
                r1 = l1->r; drdl_min = (drdy + drdx * dxdy_min); drdl_max = drdl_min + drdx;
                g1 = l1->g; dgdl_min = (dgdy + dgdx * dxdy_min); dgdl_max = dgdl_min + dgdx;
                b1 = l1->b; dbdl_min = (dbdy + dbdx * dxdy_min); dbdl_max = dbdl_min + dbdx;
                sz1 = l1->uz; dszdl_min = (dszdy + dszdx * dxdy_min); dszdl_max = dszdl_min + dszdx;
                tz1 = l1->vz; dtzdl_min = (dtzdy + dtzdx * dxdy_min); dtzdl_max = dtzdl_min + dtzdx;
            }
            if(update_right) {
                dx2 = (pr2->x - pr1->x); dy2 = (pr2->y - pr1->y);
                dx2dy2 = dy2 > 0 ? (dx2 << 16) / dy2 : 0;
                x2 = pr1->x << 16;
            }
        }
        if(p0->y + nb_lines > Y1) nb_lines = Y1 - p0->y;

        while(nb_lines > 0) {
            nb_lines--;
            if(y >= Y0) {
                register uint16_t* pz;
                register uint8_t* pp, *c;
                register uint32_t s, t, z;
                register int n;
                register int or1, og1, ob1, da1;
                float sz, tz, fzl, zinv;
                or1 = r1; og1 = g1; ob1 = b1;
                n = (x2 >> 16); if(n > X1) { n = X1; } n -= x1;
                fzl = (float)z1; zinv = 1.0 / fzl;
                pp = (uint8_t*)(pp1 + x1);
                pz = pz1 + x1; z = z1; sz = sz1; tz = tz1; x = x1;
                {
                    register int dsdx, dtdx;
                    {
                        float ss, tt;
                        ss = (sz * zinv); tt = (tz * zinv);
                        s = (int)ss; t = (int)tt;
                        dsdx = (int)((dszdx - ss * fdzdx) * zinv);
                        dtdx = (int)((dtzdx - tt * fdzdx) * zinv);
                    }
                    while(n >= 0) {
                        {
                            register uint32_t zz = z >> 14;
                            if(x >= X0 && zz >= pz[0]) {
                                c = (uint8_t*)&meg4.mmio.palette[(int)meg4.mmio.sprites[(t << 8) | s]];
                                da1 = 255 - c[3];
                                pp[2] = (((c[2] * ob1) >> 8)*c[3] + da1*pp[2]) >> 8;
                                pp[1] = (((c[1] * og1) >> 8)*c[3] + da1*pp[1]) >> 8;
                                pp[0] = (((c[0] * or1) >> 8)*c[3] + da1*pp[0]) >> 8;
                                pz[0] = zz;
                            }
                        }
                        z += dzdx; s += dsdx; t += dtdx; og1 += dgdx; or1 += drdx; ob1 += dbdx; x++;
                        pz += 1; pp += 4; n -= 1;
                    }
                }
            }

            error += derror;
            if(error > 0) {
                error -= 0x10000; x1 += dxdy_max; z1 += dzdl_max; r1 += drdl_max; g1 += dgdl_max; b1 += dbdl_max;
                sz1 += dszdl_max; tz1 += dtzdl_max;
            } else {
                x1 += dxdy_min; z1 += dzdl_min; r1 += drdl_min; g1 += dgdl_min; b1 += dbdl_min;
                sz1 += dszdl_min; tz1 += dtzdl_min;
            }
            x2 += dx2dy2; pp1 += 640; pz1 += 640; y++;
        }
    }
}

/**
 * Translate a vertex point
 */
static void transvp(vert_t *v)
{
    float w = v->pc[3] * (1.0 + 1E-5);
    if(!(v->clip = (v->pc[0] < -w) | ((v->pc[0] > w) << 1) | ((v->pc[1] < -w) << 2) | ((v->pc[1] > w) << 3) | ((v->pc[2] < -w) << 4) | ((v->pc[2] > w) << 5))) {
        w = 1.0 / v->pc[3];
        v->x = (int)(v->pc[0] * w * vps[0] + vpt[0]);
        v->y = (int)(v->pc[1] * w * vps[1] + vpt[1]);
        v->z = (int)(v->pc[2] * w * vps[2] + vpt[2]);
    }
    v->r = (int)(v->col[0] * 255.0f) & 0xff;
    v->g = (int)(v->col[1] * 255.0f) & 0xff;
    v->b = (int)(v->col[2] * 255.0f) & 0xff;
    v->a = (int)(v->col[3] * 255.0f) & 0xff;
    v->u = (int)(v->tex[0] * 255.0f) & 0xff;
    v->v = (int)(v->tex[1] * 255.0f) & 0xff;
}

/**
 * Update temporary clipped triangle points
 */
static void updateTmp(vert_t* q, vert_t* v0, vert_t* v1, float t)
{
    float r = 1.0f - t;
    q->col[0] = r * v0->col[0] + t * v1->col[0];
    q->col[1] = r * v0->col[1] + t * v1->col[1];
    q->col[2] = r * v0->col[2] + t * v1->col[2];
    q->col[3] = r * v0->col[3] + t * v1->col[3];
    q->tex[0] = r * v0->tex[0] + t * v1->tex[0];
    q->tex[1] = r * v0->tex[1] + t * v1->tex[1];
    transvp(q);
}

/**
 * Clip 3D triangle
 */
static void clip_triangle(vert_t* v0, vert_t* v1, vert_t* v2, int tex, int clip_bit)
{
    vert_t *q[3], tmp1, tmp2;
    float n;
    int co, co1, ca, cc[3], clip_mask;

    cc[0] = v0->clip; cc[1] = v1->clip; cc[2] = v2->clip;
    co = cc[0] | cc[1] | cc[2];
    if (co == 0) {
        n = (float)(v1->x - v0->x) * (float)(v2->y - v0->y) - (float)(v2->x - v0->x) * (float)(v1->y - v0->y);
        if(n >= -0.0f) return;
        tex ? draw_triangle_tex(v0, v1, v2) : draw_triangle_grd(v0, v1, v2);
    } else {
        ca = cc[0] & cc[1] & cc[2];
        if(!ca) {
            while (clip_bit < 6 && (co & (1 << clip_bit)) == 0) clip_bit++;
            if (clip_bit == 6) return;
            clip_mask = 1 << clip_bit;
            co1 = (cc[0] ^ cc[1] ^ cc[2]) & clip_mask;
            if (co1) {
                if (cc[0] & clip_mask) { q[0] = v0; q[1] = v1; q[2] = v2; } else
                if (cc[1] & clip_mask) { q[0] = v1; q[1] = v2; q[2] = v0; }
                else { q[0] = v2; q[1] = v0; q[2] = v1; }
                n = clip_proc[clip_bit](tmp1.pc, q[0]->pc, q[1]->pc);
                updateTmp(&tmp1, q[0], q[1], n);
                n = clip_proc[clip_bit](tmp2.pc, q[0]->pc, q[2]->pc);
                updateTmp(&tmp2, q[0], q[2], n);
                clip_triangle(&tmp1, q[1], q[2], tex, clip_bit + 1);
                clip_triangle(&tmp2, &tmp1, q[2], tex, clip_bit + 1);
            } else {
                if ((cc[0] & clip_mask) == 0) { q[0] = v0; q[1] = v1; q[2] = v2; } else
                if ((cc[1] & clip_mask) == 0) { q[0] = v1; q[1] = v2; q[2] = v0; }
                else { q[0] = v2; q[1] = v0; q[2] = v1; }
                n = clip_proc[clip_bit](tmp1.pc, q[0]->pc, q[1]->pc);
                updateTmp(&tmp1, q[0], q[1], n);
                n = clip_proc[clip_bit](tmp2.pc, q[0]->pc, q[2]->pc);
                updateTmp(&tmp2, q[0], q[2], n);
                clip_triangle(q[0], &tmp1, &tmp2, tex, clip_bit + 1);
            }
        }
    }
}

/**
 * Set light on 3D triangle vertices
 */
static void shader(vert_t *v)
{
    float R = 0, G = 0, B = 0, dot, d[3] = { 0 };
    d[0] = ((float)((int16_t)le16toh(meg4.mmio.lspx))/32767.0) - v->ec[0];
    d[1] = ((float)((int16_t)le16toh(meg4.mmio.lspy))/32767.0) - v->ec[1];
    d[2] = ((float)((int16_t)le16toh(meg4.mmio.lspz))/32767.0) - v->ec[2];
    meg4_normv3(d);
    dot = d[0] * v->nor[0] + d[1] * v->nor[1] + d[2] * v->nor[2];
    if (dot > 0) {
        /* diffuse light */
        R = dot * v->col[0];
        G = dot * v->col[1];
        B = dot * v->col[2];
    }
    v->col[0] = R < 0.0f ? 0.0f : (R > 1.0f ? 1.0f : R);
    v->col[1] = G < 0.0f ? 0.0f : (G > 1.0f ? 1.0f : G);
    v->col[2] = B < 0.0f ? 0.0f : (B > 1.0f ? 1.0f : B);
}

/**
 * Add a vertex to the GPU buffer (VBO)
 */
static void vertex(float x, float y, float z)
{
    vert_t *v = &verts[nvert];
    float fx, fy, fz;

    if(nvert >= 256) return;
    nvert++;
    memset(v, 0, sizeof(vert_t));
    /* world coordinates */
    v->pos[0] = fx = x;
    v->pos[1] = fy = y;
    v->pos[2] = fz = z;
    /* eye coordinates */
    v->ec[0] = (fx * cam[0] + fy * cam[1] + fz * cam[2] + cam[3]);
    v->ec[1] = (fx * cam[4] + fy * cam[5] + fz * cam[6] + cam[7]);
    v->ec[2] = (fx * cam[8] + fy * cam[9] + fz * cam[10] + cam[11]);
    v->ec[3] = (fx * cam[12] + fy * cam[13] + fz * cam[14] + cam[15]);
    /* projection coordinates */
    v->pc[0] = (v->ec[0] * prj[0] + v->ec[1] * prj[1] + v->ec[2] * prj[2] + v->ec[3] * prj[3]);
    v->pc[1] = (v->ec[0] * prj[4] + v->ec[1] * prj[5] + v->ec[2] * prj[6] + v->ec[3] * prj[7]);
    v->pc[2] = (v->ec[0] * prj[8] + v->ec[1] * prj[9] + v->ec[2] * prj[10] + v->ec[3] * prj[11]);
    v->pc[3] = (v->ec[0] * prj[12] + v->ec[1] * prj[13] + v->ec[2] * prj[14] + v->ec[3] * prj[15]);
}

/**
 * Add a triangle face to the GPU buffer (EBO)
 */
static void face(int i0, int p0, int u0, int v0, int i1, int p1, int u1, int v1, int i2, int p2, int u2, int v2)
{
    float a[3], b[3], n[3];
    face_t *f = &faces[nface];

    if(nface >= (int)(sizeof(faces)/sizeof(faces[0])) || i0 >= nvert || i1 >= nvert || i2 >= nvert) return;
    nface++;
    f->i[0] = i0; f->p[0] = p0; f->u[0] = u0; f->v[0] = v0;
    f->i[1] = i1; f->p[1] = p1; f->u[1] = u1; f->v[1] = v1;
    f->i[2] = i2; f->p[2] = p2; f->u[2] = u2; f->v[2] = v2;
    /* calculate smooth normals */
    a[0] = verts[i1].pos[0] - verts[i0].pos[0]; a[1] = verts[i1].pos[1] - verts[i0].pos[1]; a[2] = verts[i1].pos[2] - verts[i0].pos[2];
    b[0] = verts[i2].pos[0] - verts[i0].pos[0]; b[1] = verts[i2].pos[1] - verts[i0].pos[1]; b[2] = verts[i2].pos[2] - verts[i0].pos[2];
    n[0] = a[1] * b[2] - a[2] * b[1]; n[1] = a[2] * b[0] - a[0] * b[2]; n[2] = a[0] * b[1] - a[1] * b[0];
    verts[i0].nor[0] += n[0]; verts[i0].nor[1] += n[1]; verts[i0].nor[2] += n[2];
    verts[i1].nor[0] += n[0]; verts[i1].nor[1] += n[1]; verts[i1].nor[2] += n[2];
    verts[i2].nor[0] += n[0]; verts[i2].nor[1] += n[1]; verts[i2].nor[2] += n[2];
}

/**
 * Process triangles in VBO + EBO, clip and display them
 */
static void processtri(int tex)
{
    face_t *f;
    vert_t *v0, *v1, *v2;
    int i;
    uint32_t col;

    if(!nvert || !nface) return;
    /* convert using inverse view matrix and normalize normal vectors */
    for(i = 0, v0 = verts; i < nvert; i++, v0++) {
        v0->nor[0] = (v0->nor[0] * cami[0] + v0->nor[1] * cami[1] + v0->nor[2] * cami[2]);
        v0->nor[1] = (v0->nor[0] * cami[4] + v0->nor[1] * cami[5] + v0->nor[2] * cami[6]);
        v0->nor[2] = (v0->nor[0] * cami[8] + v0->nor[1] * cami[9] + v0->nor[2] * cami[10]);
        meg4_normv3(v0->nor);
        transvp(v0);
    }
    for(i = 0, f = faces; i < nface; i++, f++) {
        v0 = &verts[f->i[0]]; v1 = &verts[f->i[1]]; v2 = &verts[f->i[2]];
        if(tex) {
            v0->tex[0] = f->u[0]; v0->tex[1] = f->v[0]; v0->col[0] = v0->col[1] = v0->col[2] = v0->col[3] = 1.0f;
            v1->tex[0] = f->u[1]; v1->tex[1] = f->v[1]; v1->col[0] = v1->col[1] = v1->col[2] = v1->col[3] = 1.0f;
            v2->tex[0] = f->u[2]; v2->tex[1] = f->v[2]; v2->col[0] = v2->col[1] = v2->col[2] = v2->col[3] = 1.0f;
        } else {
            col = meg4.mmio.palette[f->p[0]];
            v0->col[0] = (float)(col & 0xff) / 255.0f;
            v0->col[1] = (float)((col >> 8) & 0xff) / 255.0f;
            v0->col[2] = (float)((col >> 16) & 0xff) / 255.0f;
            v0->col[3] = (float)((col >> 24) & 0xff) / 255.0f;

            col = meg4.mmio.palette[f->p[1]];
            v1->col[0] = (float)(col & 0xff) / 255.0f;
            v1->col[1] = (float)((col >> 8) & 0xff) / 255.0f;
            v1->col[2] = (float)((col >> 16) & 0xff) / 255.0f;
            v1->col[3] = (float)((col >> 24) & 0xff) / 255.0f;

            col = meg4.mmio.palette[f->p[2]];
            v2->col[0] = (float)(col & 0xff) / 255.0f;
            v2->col[1] = (float)((col >> 8) & 0xff) / 255.0f;
            v2->col[2] = (float)((col >> 16) & 0xff) / 255.0f;
            v2->col[3] = (float)((col >> 24) & 0xff) / 255.0f;
        }
        shader(v0); shader(v1); shader(v2);
        clip_triangle(v0, v1, v2, tex, 0);
    }
}

/**
 * Get screen
 */
void meg4_getscreen(void)
{
    float zsize = (1 << (16 + 14));

    if(meg4.mode == MEG4_MODE_GAME) {
        if(meg4.mmio.scrx == 0xffff || meg4.mmio.scry == 0xffff) {
            meg4.screen.w = 640; meg4.screen.h = 400;
            meg4.screen.buf = meg4.vram;
        } else {
            meg4.screen.w = 320; meg4.screen.h = 200;
            if(le16toh(meg4.mmio.scrx) > 320) meg4.mmio.scrx = htole16(320);
            if(le16toh(meg4.mmio.scry) > 200) meg4.mmio.scry = htole16(200);
            meg4.screen.buf = meg4.vram + le16toh(meg4.mmio.scry) * 640 + le16toh(meg4.mmio.scrx);
        }
    } else {
#ifndef NOEDITORS
        if(meg4.mode <= MEG4_MODE_SAVE && !load_list) {
            /* loader and saver have the same resolution as the game */
            meg4.screen.w = 320; meg4.screen.h = 200;
        } else {
            /* everything else uses double */
            meg4.screen.w = 640; meg4.screen.h = 400;
        }
        meg4.mmio.scrx = meg4.mmio.scry = 0;
        meg4.screen.buf = meg4.valt;
#endif
    }
    vpt[0] = (((float)meg4.screen.w - 0.5) / 2.0);
    vpt[1] = (((float)meg4.screen.h - 0.5) / 2.0);
    vpt[2] = ((zsize - 0.5) / 2.0) + ((1 << 14)) / 2;
    vps[0] = ((float)meg4.screen.w - 0.5) / 2.0;
    vps[1] = -((float)meg4.screen.h - 0.5) / 2.0;
    vps[2] = -((zsize - 0.5) / 2.0);
}

/**
 * Get projection, view and inverse view matrices
 */
void meg4_getview(void)
{
    float xaxis[3], yaxis[3], zaxis[3], camf[3], camr[3], camu[3], tmp[16];
    float yaw = (float)((le16toh(meg4.mmio.camyaw) + 270) % 360) * MEG4_PI / 180.0f;
    float pitch = (float)((le16toh(meg4.mmio.camyaw) + 270) % 360) * MEG4_PI / 180.0f, cosp = cosf(pitch);
    float fov = (float)(le16toh(meg4.mmio.camfov)), x,y,A,B,C,D, n = 0.1f, f = 100.0f;

    if(fov > 0) {
        /* perspective projection */
        y = 1.0/tanf(fov * MEG4_PI / 360.0); x = y/1.6; A = n-f; C = 2*f*n/A; A = (f+n)/A; B = -1; D = 0;
    } else {
        /* orthographic projection */
        fov = -fov / 127.0; A = 2.0 * fov; B = A * 1.6;
        x = 2.0 / B; y = 2.0 / A; A = -2.0 / (f-n); B = -(f+n) / (f-n); C = 0; D = 1;
    }
    prj[0]=x;   prj[1]=0;   prj[2]=0;   prj[3]=0;
    prj[4]=0;   prj[5]=y;   prj[6]=0;   prj[7]=0;
    prj[8]=0;   prj[9]=0;   prj[10]=A;  prj[11]=C;
    prj[12]=0;  prj[13]=0;  prj[14]=B;  prj[15]=D;
    /* calculate view matrix and its inverse from camera */
    camf[0] = cosf(yaw) * cosp;
    camf[1] = sinf(pitch);
    camf[2] = sinf(yaw) * cosp;
    meg4_normv3(camf);
    camr[0] = -camf[2]; camr[1] = 0; camr[2] = camf[0];
    meg4_normv3(camr);
    camu[0] = camr[1] * camf[2] - camr[2] * camf[1];
    camu[1] = camr[2] * camf[0] - camr[0] * camf[2];
    camu[2] = camr[0] * camf[1] - camr[1] * camf[0];
    meg4_normv3(camu);
    xaxis[0] = camf[1] * camu[2] - camf[2] * camu[1];
    xaxis[1] = camf[2] * camu[0] - camf[0] * camu[2];
    xaxis[2] = camf[0] * camu[1] - camf[1] * camu[0];
    yaxis[0] = xaxis[1] * camf[2] - xaxis[2] * camf[1];
    yaxis[1] = xaxis[2] * camf[0] - xaxis[0] * camf[2];
    yaxis[2] = xaxis[0] * camf[1] - xaxis[1] * camf[0];
    zaxis[0] = -camf[0]; zaxis[1] = -camf[1]; zaxis[2] = -camf[2];
    cam[0] = xaxis[0];  cam[1] = yaxis[0];  cam[2] = zaxis[0];  cam[3] = 0;
    cam[4] = xaxis[1];  cam[5] = yaxis[1];  cam[6] = zaxis[1];  cam[7] = 0;
    cam[8] = xaxis[2];  cam[9] = yaxis[2];  cam[10] = zaxis[2]; cam[11] = 0;
    cam[12] = -(xaxis[0] * camp[0] + xaxis[1] * camp[1] + xaxis[2] * camp[2]);
    cam[13] = -(yaxis[0] * camp[0] + yaxis[1] * camp[1] + yaxis[2] * camp[2]);
    cam[14] = -(zaxis[0] * camp[0] + zaxis[1] * camp[1] + zaxis[2] * camp[2]);
    cam[15] = 1;
    meg4_invm4(tmp, cam);
    cami[0] = tmp[0]; cami[1] = tmp[4]; cami[2] = tmp[8]; cami[3] = tmp[12];
    cami[4] = tmp[1]; cami[5] = tmp[5]; cami[6] = tmp[9]; cami[7] = tmp[13];
    cami[8] = tmp[2]; cami[9] = tmp[6]; cami[10] = tmp[10]; cami[11] = tmp[14];
    cami[12] = tmp[3]; cami[13] = tmp[7]; cami[14] = tmp[11]; cami[15] = tmp[15];
}

/**
 * Redraw the platform's framebuffer with the MEG-4's VRAM
 */
void meg4_redraw(uint32_t *dst, int dw, int dh, int dp)
{
#ifndef NOEDITORS
    char fn[32] = "meg4_scr_0000000000.png";
    uint64_t now = le64toh(meg4.mmio.now);
    uint8_t *buf;
#endif
    char tmp[32], *gf = "Software Failure.", *gm = "Guru Meditation #";
    int x, y = 0, w, h, i, j, p = dp >> 2, x0, x1, y0, y1;
    uint32_t *ptr, *d = dst, c;

    if(!dst || dw < 1 || dh < 1 || dp < 4) return;
    ptr = meg4.screen.buf;
    w = (meg4.screen.w < dw ? meg4.screen.w : dw);
    h = meg4.screen.h < dh ? meg4.screen.h : dh;
    x0 = meg4.mmio.cropx0; meg4.mmio.cropx0 = 0; x1 = meg4.mmio.cropx1; meg4.mmio.cropx1 = htole16(w);
    y0 = meg4.mmio.cropy0; meg4.mmio.cropy0 = 0; y1 = meg4.mmio.cropy1; meg4.mmio.cropy1 = htole16(h);
    /* panic, this should only be shown when editors aren't compiled in. But just in case something really goes wrong, no ifdef */
    if(meg4.mode == MEG4_MODE_GURU) {
        meg4.screen.w = 320; meg4.screen.h = 200; for(y = 0, j = w << 2; y < h; y++, d += p) memset(d, 0, j);
        meg4_blit(dst, 108, 8, dp, 104, 64, meg4_icons.buf, 0, 0, meg4_icons.w * 4, 1);
        x0 = meg4_width(meg4_font, 1, gf, NULL); meg4_text(dst, (w - x0) / 2, 126, dp, htole32(0xff0000f0), 0, 1, meg4_font, gf);
        j = strlen(gm); memcpy(tmp, gm, j);
        for(c = meg4.pc, x = 28; x >= 0; x -= 4, j++) { y = (c >> x) & 0xf; tmp[j] = '0' + (y > 9 ? y - 10 + 'A' : y); } tmp[j] = 0;
        j = meg4_width(meg4_font, 1, tmp, NULL); if(x0 < j) x0 = j;
        c = htole32(0xff0000f0); meg4_text(dst, (w - j) / 2, 138, dp, c, 0, 1, meg4_font, tmp);
        if(le32toh(meg4.mmio.tick) & 512) {
            for(x0 += 16, d = dst + 120 * p + ((320 - x0) >> 1), x = 0; x < x0; x++, d++) d[0] = d[p] = d[32*p] = d[33*p] = c;
            for(d = dst + 122 * p + ((320 - x0) >> 1), x = 0; x < 32; x++, d += p) d[0] = d[1] = d[x0-2] = d[x0-1] = c;
        }
        meg4_blit(dst, le16toh(meg4.mmio.ptrx) - 4, le16toh(meg4.mmio.ptry) -4, dp, 8, 8, meg4_icons.buf, 24, 64, meg4_icons.w * 4, 1);
        return;
    }
    c = htole32(0xffaaaaaa);
#ifndef NOEDITORS
    if(meg4.mode > MEG4_MODE_SAVE || load_list) {
        c = theme[THEME_MENU_BG];
        for(; y < 12; y++, d += p)
            for(x = 0; x < w; x++) d[x] = c;
    }
#endif
    for(j = w << 2; y < h; y++, ptr += 640, d += p) {
        memcpy(d, ptr, j);
        if(w < dw) d[w] = 0;
    }
#ifndef NOEDITORS
    if(meg4.mode > MEG4_MODE_SAVE || load_list)
        menu_view(dst, dw, dh, dp);
    textinp_view(dst, dp);
#endif
    /* special input methods */
    if(meg4.kbdmode) {
        for(d = dst + (h - 10) * p + w - 50, y = 0; y < 10; y++, d += p)
            for(x = 0; x < 50; x++) d[x] = c;
        if(meg4.kbdmode == 1) strcpy(tmp, "U+");
        else { strcpy(tmp, "⌨"); if(meg4_kbdtmpsht) strcat(tmp, "⇮"); }
        meg4_text(dst, w - 49, h - 9, dp, htole32(0xff333333), 0, 1, meg4_font, tmp);
        meg4_text(dst, w - 32, h - 9, dp, htole32(0xff222222), 0, 1, meg4_font, meg4_kbdtmpbuf);
    }
#ifndef NOEDITORS
    if(!textinp_cursor(dst, dp))
#endif
    {
        i = MEG4_PTR_ICON;
        if(i < 0x3ff) {
            if(i >= 0x3fb && meg4_icons.buf)
                /* built-in cursors */
                meg4_blit(dst, le16toh(meg4.mmio.ptrx) - MEG4_PTR_HOTSPOT_X, le16toh(meg4.mmio.ptry) - MEG4_PTR_HOTSPOT_Y, dp, 8, 8,
                    meg4_icons.buf, (i - 0x3fb) * 8, 64, meg4_icons.w * 4, 1);
            else
                /* user provided cursor */
                meg4_spr(dst, dp, le16toh(meg4.mmio.ptrx) - MEG4_PTR_HOTSPOT_X, le16toh(meg4.mmio.ptry) - MEG4_PTR_HOTSPOT_Y, i, 1, 0);
        }
    }
    meg4.mmio.cropx0 = x0; meg4.mmio.cropx1 = x1; meg4.mmio.cropy0 = y0; meg4.mmio.cropy1 = y1;
#ifndef NOEDITORS
    /* take a screenshot */
    if(meg4_takescreenshot) {
        meg4_takescreenshot = 0;
        buf = stbi_write_png_to_mem((unsigned char*)dst, dp, w, h, 4, &i, NULL, 0);
        if(buf) {
            for(j = 18; now > 0 && j > 8; j--, now /= 10) fn[j] += now % 10;
            main_writefile(fn, buf, i);
            free(buf);
        }
    }
#endif
}

/**
 * Get a screenshot
 */
void meg4_screenshot(uint32_t *dst, int dx, int dy, int dp)
{
    int x, y, w, h, i;
    uint8_t *src, *line;

    if(!dst || dx < 0 || dy < 0 || dp < 4) return;
    /* do not use meg4.screen, that might point to alternate vram */
    if(meg4.mode == MEG4_MODE_GAME) { x = meg4.mmio.scrx; y = meg4.mmio.scry; } else { x = oldsx; y = oldsy; }
    if(x == 0xffff || y == 0xffff) {
        w = 640; h = 400;
        src = (uint8_t*)meg4.vram;
    } else {
        w = 320; h = 200;
        x = le16toh(x) > 320 ? 320 : le16toh(x);
        y = le16toh(y) > 200 ? 200 : le16toh(y);
        src = (uint8_t*)(meg4.vram + y * 640 + x);
    }
    dp >>= 2; dst += dy * dp + dx;
    /* only write if screen isn't empty */
    for(y = i = 0; !i && y < h; y++)
        for(line = src + y * 640 * 4, x = 0; !i && x < 4 * w; x += 4)
            if(line[x] || line[x+1] || line[x+2]) i = 1;
    if(!i) return;
    if(w == 320) {
        /* 320 x 200 -> 160 x 100 */
        for(y = 0; y < 100; y++, src += 5120, dst += dp)
            for(line = src, x = 0; x < 160; x++, line += 8) {
                dst[x] = (((line[0] + line[4] + line[2560] + line[2564]) >> 2) & 0xff) |
                    ((((line[1] + line[5] + line[2561] + line[2565]) >> 2) & 0xff) << 8) |
                    ((((line[2] + line[6] + line[2562] + line[2566]) >> 2) & 0xff) << 16) |
                    (0xff << 24);
            }
    } else {
        /* 640 x 400 -> 160 x 100 */
        for(y = 0; y < 100; y++, src += 10240, dst += dp)
            for(line = src, x = 0; x < 160; x++, line += 16) {
                dst[x] = (((line[0] + line[4] + line[8] + line[12] +
                            line[2560] + line[2564] + line[2568] + line[2572] +
                            line[5120] + line[5124] + line[5128] + line[5132] +
                            line[7680] + line[7684] + line[7688] + line[7692]) >> 4) & 0xff) |
                         ((((line[1] + line[5] + line[9] + line[13] +
                            line[2561] + line[2565] + line[2569] + line[2573] +
                            line[5121] + line[5125] + line[5129] + line[5133] +
                            line[7681] + line[7685] + line[7689] + line[7693]) >> 4) & 0xff) << 8) |
                         ((((line[2] + line[6] + line[10] + line[14] +
                            line[2562] + line[2566] + line[2570] + line[2574] +
                            line[5122] + line[5126] + line[5130] + line[5134] +
                            line[7682] + line[7686] + line[7690] + line[7694]) >> 4) & 0xff) << 16) |
                         (0xff << 24);
            }
    }
}

/**
 * Recalculate font widths
 */
void meg4_recalcfont(int s, int e)
{
    uint8_t *fnt = meg4.font + 8 * s, *ptr = meg4.font + 8 * 65536 + s;
    int i, x, y, l, r;

    if(s < 0 || s > 0xffff || e < 0 || e > 0xffff || e < s) return;
    memset(ptr, 0, e - s + 1);
    for(i = s; i <= e; i++, fnt += 8, ptr++) {
        if(i == 32 || i == 160) { *ptr = 0x30; continue; }
        for(l = 7, r = y = 0; y < 8; y++)
            for(x = 0; x < 8; x++)
                if(fnt[y] & (1 << x)) {
                    if(l > x) l = x;
                    if(r < x) r = x;
                }
        if(l > r) l = r;
        *ptr = (r << 4) | l;
    }
}

/**
 * Recalculate mipmap cache
 */
void meg4_recalcmipmap(void)
{
    int i, j, k, p;
    uint8_t *a = meg4.mipmap, *b00, *b01, *b10, *b11;
    for(j = 0; j < 128; j++)
        for(i = 0; i < 128; i++, a += 4) {
            b00 = (uint8_t*)&meg4.mmio.palette[(int)meg4.mmio.sprites[(j << 9) + (i << 1)]];
            b01 = (uint8_t*)&meg4.mmio.palette[(int)meg4.mmio.sprites[(j << 9) + (i << 1) + 1]];
            b10 = (uint8_t*)&meg4.mmio.palette[(int)meg4.mmio.sprites[(j << 9) + (i << 1) + 256]];
            b11 = (uint8_t*)&meg4.mmio.palette[(int)meg4.mmio.sprites[(j << 9) + (i << 1) + 257]];
            a[0] = (b00[0] + b01[0] + b10[0] + b11[0]) >> 2;
            a[1] = (b00[1] + b01[1] + b10[1] + b11[1]) >> 2;
            a[2] = (b00[2] + b01[2] + b10[2] + b11[2]) >> 2;
            a[3] = (b00[3] + b01[3] + b10[3] + b11[3]) >> 2;
        }
    for(b00 = meg4.mipmap, k = 64, p = 512; k > 16; k >>= 1, p >>= 1)
        for(j = 0; j < k; j++, b00 += p)
            for(i = 0; i < k; i++, a += 4, b00 += 8) {
                a[0] = (b00[0] + b00[4] + b00[p+0] + b00[p+4]) >> 2;
                a[1] = (b00[1] + b00[5] + b00[p+1] + b00[p+5]) >> 2;
                a[2] = (b00[2] + b00[6] + b00[p+2] + b00[p+6]) >> 2;
                a[3] = (b00[3] + b00[7] + b00[p+3] + b00[p+7]) >> 2;
            }
}

/**
 * Return closest match of a truecolor pixel on palette
 */
uint8_t meg4_palidx(uint8_t *rgba)
{
    uint8_t ret = 0, *b;
    int i, dr, dg, db, d, dm = 0x7fffffff;

    for(i = 0; i < 256 && dm > 0; i++) {
        b = (uint8_t*)&meg4.mmio.palette[i];
        if(rgba[0] == b[0] && rgba[1] == b[1] && rgba[2] == b[2]) return i;
        db = rgba[2] > b[2] ? rgba[2] - b[2] : b[2] - rgba[2];
        dg = rgba[1] > b[1] ? rgba[1] - b[1] : b[1] - rgba[1];
        dr = rgba[0] > b[0] ? rgba[0] - b[0] : b[0] - rgba[0];
        d = ((dr*dr) << 1) + (db*db) + ((dg*dg) << 2);
        if(d < dm) { dm = d; ret = i; }
    }
    return ret;
}

/**
 * Blit a sprite to screen
 */
void meg4_spr(uint32_t *dst, int dp, int x, int y, int sprite, int scale, int type)
{
    int i, j, k, l, m, w, p = 0, siz[] = { 1, 2, 4, 0, 8, 16, 24, 32 }, A, B, C, D, p2 = 2 * dp, p3 = 3 * dp;
    int x0 = le16toh(meg4.mmio.cropx0), y0 = le16toh(meg4.mmio.cropy0), x1 = le16toh(meg4.mmio.cropx1), y1 = le16toh(meg4.mmio.cropy1);
    uint8_t *s, *d, *a, *b;

    if(x >= x1 || y >= y1 || sprite > 1023 || scale < -3 || scale > 4 || type < 0 || type > 7) return;
    if(!scale) scale = 1;
    w = siz[scale + 3];
    if(x + w < x0 || y + w < y0) return;
    d = (uint8_t*)dst + y * dp + x * 4;
    if(scale < 0) {
        /* downscale, use precalculated average values in mipmap table */
        switch(scale) {
            case -1: s = meg4.mipmap + ((sprite & ~31) << 7) + ((sprite & 31) << 4); p = 9; break;
            case -2: s = meg4.mipmap + 128*128*4 + ((sprite & ~31) << 6) + ((sprite & 31) << 3); p = 8; break;
            case -3: s = meg4.mipmap + 128*128*4 + 64*64*4 + ((sprite & ~31) << 5) + ((sprite & 31) << 2); p = 7; break;
        }
        for(j = 0; j < w; j++, d += dp)
            if(y + j >= y0 && y + j < y1)
                for(a = d, i = 0; i < w; i++, a += 4)
                    if(x + i >= x0 && x + i < y1) {
                        switch(type) {
                            case 1: b = s + ((w - i - 1) << p) + (j << 2); break;
                            case 2: b = s + ((w - j - 1) << p) + ((w - i - 1) << 2); break;
                            case 3: b = s + (i << p) + ((w - j - 1) << 2); break;
                            case 4: b = s + ((w - j - 1) << p) + (i << 2); break;
                            case 5: b = s + ((w - i - 1) << p) + ((w - j - 1) << 2); break;
                            case 6: b = s + (j << p) + ((w - i - 1) << 2); break;
                            case 7: b = s + ((w - j - 1) << p) + ((w - i - 1) << 2); break;
                            default: b = s + (j << p) + (i << 2); break;
                        }
                        if(b[3]) {
                            D = 255 - b[3];
                            a[0] = (b[0]*b[3] + D*a[0]) >> 8; a[1] = (b[1]*b[3] + D*a[1]) >> 8; a[2] = (b[2]*b[3] + D*a[2]) >> 8;
                        }
                    }
    } else {
        /* normal or upscale */
        k = scale * 4; p = scale * dp; s = meg4.mmio.sprites + ((sprite & ~31) << 6) + ((sprite & 31) << 3);
        for(j = 0, l = y; j < 8; j++, l += scale, d += p)
            if(l >= y0 && l < y1)
                for(a = d, i = 0, m = x; i < 8; i++, m += scale, a += k)
                    if(m >= x0 && m < y1) {
                        switch(type) {
                            case 1: b = s + ((7 - i) << 8) + (j); break;
                            case 2: b = s + ((7 - j) << 8) + (7 - i); break;
                            case 3: b = s + (i << 8) + (7 - j); break;
                            case 4: b = s + ((7 - j) << 8) + (i); break;
                            case 5: b = s + ((7 - i) << 8) + (7 - j); break;
                            case 6: b = s + (j << 8) + (7 - i); break;
                            case 7: b = s + ((7 - j) << 8) + (7 - i); break;
                            default: b = s + (j << 8) + (i); break;
                        }
                        if(b[0]) {
                            b = (uint8_t*)&meg4.mmio.palette[(int)b[0]];
                            A = b[2]*b[3]; B = b[1]*b[3]; C = b[0]*b[3]; D = 255 - b[3];
                            switch(scale) {
                                case 1:
                                    a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                break;
                                case 2:
                                    a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                    a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                                    a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                                    a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                                break;
                                case 3:
                                    a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                    a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                                    a[10] = (A + D*a[10]) >> 8; a[9] = (B + D*a[9]) >> 8; a[8] = (C + D*a[8]) >> 8;
                                    a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                                    a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                                    a[dp+10] = (A + D*a[dp+10]) >> 8; a[dp+9] = (B + D*a[dp+9]) >> 8; a[dp+8] = (C + D*a[dp+8]) >> 8;
                                    a[p2+2] = (A + D*a[p2+2]) >> 8; a[p2+1] = (B + D*a[p2+1]) >> 8; a[p2+0] = (C + D*a[p2+0]) >> 8;
                                    a[p2+6] = (A + D*a[p2+6]) >> 8; a[p2+5] = (B + D*a[p2+5]) >> 8; a[p2+4] = (C + D*a[p2+4]) >> 8;
                                    a[p2+10] = (A + D*a[p2+10]) >> 8; a[p2+9] = (B + D*a[p2+9]) >> 8; a[p2+8] = (C + D*a[p2+8]) >> 8;
                                break;
                                case 4:
                                    a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                    a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                                    a[10] = (A + D*a[10]) >> 8; a[9] = (B + D*a[9]) >> 8; a[8] = (C + D*a[8]) >> 8;
                                    a[14] = (A + D*a[14]) >> 8; a[13] = (B + D*a[13]) >> 8; a[12] = (C + D*a[12]) >> 8;
                                    a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                                    a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                                    a[dp+10] = (A + D*a[dp+10]) >> 8; a[dp+9] = (B + D*a[dp+9]) >> 8; a[dp+8] = (C + D*a[dp+8]) >> 8;
                                    a[dp+14] = (A + D*a[dp+14]) >> 8; a[dp+13] = (B + D*a[dp+13]) >> 8; a[dp+12] = (C + D*a[dp+12]) >> 8;
                                    a[p2+2] = (A + D*a[p2+2]) >> 8; a[p2+1] = (B + D*a[p2+1]) >> 8; a[p2+0] = (C + D*a[p2+0]) >> 8;
                                    a[p2+6] = (A + D*a[p2+6]) >> 8; a[p2+5] = (B + D*a[p2+5]) >> 8; a[p2+4] = (C + D*a[p2+4]) >> 8;
                                    a[p2+10] = (A + D*a[p2+10]) >> 8; a[p2+9] = (B + D*a[p2+9]) >> 8; a[p2+8] = (C + D*a[p2+8]) >> 8;
                                    a[p2+14] = (A + D*a[p2+14]) >> 8; a[p2+13] = (B + D*a[p2+13]) >> 8; a[p2+12] = (C + D*a[p2+12]) >> 8;
                                    a[p3+2] = (A + D*a[p3+2]) >> 8; a[p3+1] = (B + D*a[p3+1]) >> 8; a[p3+0] = (C + D*a[p3+0]) >> 8;
                                    a[p3+6] = (A + D*a[p3+6]) >> 8; a[p3+5] = (B + D*a[p3+5]) >> 8; a[p3+4] = (C + D*a[p3+4]) >> 8;
                                    a[p3+10] = (A + D*a[p3+10]) >> 8; a[p3+9] = (B + D*a[p3+9]) >> 8; a[p3+8] = (C + D*a[p3+8]) >> 8;
                                    a[p3+14] = (A + D*a[p3+14]) >> 8; a[p3+13] = (B + D*a[p3+13]) >> 8; a[p3+12] = (C + D*a[p3+12]) >> 8;
                                break;
                            }
                        }
                    }
    }
}

/**
 * Blit icons to screen
 */
void meg4_blit(uint32_t *dst, int x, int y, int dp, int w, int h, uint32_t *src, int sx, int sy, int sp, int t)
{
    int i, j, tp = t * dp, p2 = 2 * dp, p3 = 3 * dp, t4 = t * 4, A, B, C, D;
    uint8_t *s = (uint8_t*)src, *d = (uint8_t*)dst, *a, *b;

    if(w < 1 || h < 1 || !src || !dst) return;
    s += sp * sy + sx * 4; d += dp * y + x * 4;
    for(j = 0; j < h; j++, d += tp, s += sp)
        if(y + j >= le16toh(meg4.mmio.cropy0) && y + j < le16toh(meg4.mmio.cropy1))
            for(i = 0, a = d, b = s; i < w; i++, a += t4, b += 4) {
                if(!b[3] || x + i < le16toh(meg4.mmio.cropx0) || x + i >= le16toh(meg4.mmio.cropx1)) continue;
                A = b[2]*b[3]; B = b[1]*b[3]; C = b[0]*b[3]; D = 255 - b[3];
                switch(t) {
                    case 1:
                        a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                    break;
                    case 2:
                        a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                        a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                        a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                        a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                    break;
                    case 3:
                        a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                        a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                        a[10] = (A + D*a[10]) >> 8; a[9] = (B + D*a[9]) >> 8; a[8] = (C + D*a[8]) >> 8;
                        a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                        a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                        a[dp+10] = (A + D*a[dp+10]) >> 8; a[dp+9] = (B + D*a[dp+9]) >> 8; a[dp+8] = (C + D*a[dp+8]) >> 8;
                        a[p2+2] = (A + D*a[p2+2]) >> 8; a[p2+1] = (B + D*a[p2+1]) >> 8; a[p2+0] = (C + D*a[p2+0]) >> 8;
                        a[p2+6] = (A + D*a[p2+6]) >> 8; a[p2+5] = (B + D*a[p2+5]) >> 8; a[p2+4] = (C + D*a[p2+4]) >> 8;
                        a[p2+10] = (A + D*a[p2+10]) >> 8; a[p2+9] = (B + D*a[p2+9]) >> 8; a[p2+8] = (C + D*a[p2+8]) >> 8;
                    break;
                    case 4:
                        a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                        a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                        a[10] = (A + D*a[10]) >> 8; a[9] = (B + D*a[9]) >> 8; a[8] = (C + D*a[8]) >> 8;
                        a[14] = (A + D*a[14]) >> 8; a[13] = (B + D*a[13]) >> 8; a[12] = (C + D*a[12]) >> 8;
                        a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                        a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                        a[dp+10] = (A + D*a[dp+10]) >> 8; a[dp+9] = (B + D*a[dp+9]) >> 8; a[dp+8] = (C + D*a[dp+8]) >> 8;
                        a[dp+14] = (A + D*a[dp+14]) >> 8; a[dp+13] = (B + D*a[dp+13]) >> 8; a[dp+12] = (C + D*a[dp+12]) >> 8;
                        a[p2+2] = (A + D*a[p2+2]) >> 8; a[p2+1] = (B + D*a[p2+1]) >> 8; a[p2+0] = (C + D*a[p2+0]) >> 8;
                        a[p2+6] = (A + D*a[p2+6]) >> 8; a[p2+5] = (B + D*a[p2+5]) >> 8; a[p2+4] = (C + D*a[p2+4]) >> 8;
                        a[p2+10] = (A + D*a[p2+10]) >> 8; a[p2+9] = (B + D*a[p2+9]) >> 8; a[p2+8] = (C + D*a[p2+8]) >> 8;
                        a[p2+14] = (A + D*a[p2+14]) >> 8; a[p2+13] = (B + D*a[p2+13]) >> 8; a[p2+12] = (C + D*a[p2+12]) >> 8;
                        a[p3+2] = (A + D*a[p3+2]) >> 8; a[p3+1] = (B + D*a[p3+1]) >> 8; a[p3+0] = (C + D*a[p3+0]) >> 8;
                        a[p3+6] = (A + D*a[p3+6]) >> 8; a[p3+5] = (B + D*a[p3+5]) >> 8; a[p3+4] = (C + D*a[p3+4]) >> 8;
                        a[p3+10] = (A + D*a[p3+10]) >> 8; a[p3+9] = (B + D*a[p3+9]) >> 8; a[p3+8] = (C + D*a[p3+8]) >> 8;
                        a[p3+14] = (A + D*a[p3+14]) >> 8; a[p3+13] = (B + D*a[p3+13]) >> 8; a[p3+12] = (C + D*a[p3+12]) >> 8;
                    break;
                }
            }
}

#ifndef NOEDITORS
/**
 * Blit distorted to screen
 */
void meg4_blitd(uint32_t *dst, int x, int y, int dp, int w1, int w2, int h, uint32_t *src, int sx, int sy, int sw, int sh, int sp)
{
    int i, j, w, D, m = w2 > w1 ? w2 : w1;
    uint8_t *s, *d, *b;

    if(w1 < 1 || w2 < 1 || h < 1 || sw < 1 || sh < 1 || !src || !dst) return;
    for(j = 0; j < h; j++) {
        if(y + j >= 0 && y + j < 388) {
            w = (j * w2 + (h - j) * w1) / h;
            i = (j * sh / h); if(sy + i >= sh) break;
            s = (uint8_t*)src + (sy + i) * sp + sx * 4;
            d = (uint8_t*)dst + (y + j) * dp + (x + (m - w)/2) * 4;
            for(i = 0; i < w; i++, d += 4) {
                b = s + (i * sw / w) * 4;
                if(!b[3] || x + i < 0 || x + i >= 630) continue;
                D = 255 - b[3];
                d[2] = (b[2]*b[3] + D*d[2]) >> 8; d[1] = (b[1]*b[3] + D*d[1]) >> 8; d[0] = (b[0]*b[3] + D*d[0]) >> 8;
            }
        }
    }
}

/**
 * Display a filled box with altering background, borders and shadow
 * l - light border color
 * b - background color (altering will use 4 gradient lighter)
 * d - dark border color
 * s - shadow color or zero
 * t - pixels to skip at top (header)
 * o - scroll offset
 * r - size of a row in pixels
 * c - size of a coloumn in pixels or zero if there are only rows
 */
void meg4_box(uint32_t *dst, int dw, int dh, int dp, int x, int y, int w, int h, uint32_t l, uint32_t b, uint32_t d,
    uint32_t s, int t, int o, int r, int c)
{
    int i, j, k, p, p2, pitch, A, B, C, D;
    uint32_t bg1, bg2;
    uint8_t *a;

    if(x >= dw || x + w < 0 || y >= dh || y + h < 0 || w < 1 || h < 1 || !dst) return;
    if(!l) l = b;
    if(!d) d = b;
    a = (uint8_t*)&s;
    if(!a[3]) s = A = B = C = D = 0;
    else { A = a[0]*a[3]; B = a[1]*a[3]; C = a[2]*a[3]; D = 255 - a[3]; }
    pitch = dp >> 2;
    p = y * pitch + x;
    p2 = (y + h - 1) * pitch + x;
    for(i=0; i < w && x + i < dw; i++) {
        if(y >= le16toh(meg4.mmio.cropy0) && y < le16toh(meg4.mmio.cropy1) && x + i >= 0) dst[p + i] = l;
        if(y + h - 1 >= le16toh(meg4.mmio.cropy0) && y + h - 1 < le16toh(meg4.mmio.cropy1) && x + i >= 0) dst[p2 + i] = d;
    }
    if(s && y + h >= le16toh(meg4.mmio.cropy0) && y + h < le16toh(meg4.mmio.cropy1)) {
        a = (uint8_t*)&dst[p2 + w];
        if(x + w < dw) { a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8; }
        if(x + w + 1 < dw) { a += 4; a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8; }
    }
    p += pitch; p2 = p;
    for(j = 1; j + 1 < h && y + j < le16toh(meg4.mmio.cropy1); j++, p += pitch)
        if(y + j >= le16toh(meg4.mmio.cropy0)) {
            if(x >= 0) dst[p] = l;
            if(x + w - 1 < dw) dst[p + w - 1] = d;
            if(s && j > 1) {
                a = (uint8_t*)&dst[p + w];
                if(x + w < dw) { a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8; }
                if(x + w + 1 < dw) { a += 4; a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8; }
            }
        }
    if(b) {
        y += t; p = p2 + t * pitch;
        bg2 = (b & htole32(0xFF000000)) | ((b + htole32(0x040404)) & htole32(0x7F7F7F));
        if(c) {
            for(j=0; j + t + 2 < h && y + j < le16toh(meg4.mmio.cropy1); j++, p += pitch)
                if(y + j >= le16toh(meg4.mmio.cropy0))
                    for(i=1; i + 1 < w && x + i < dw; ) {
                        bg1 = r > 0 && (((j + o) / r) & 1) ^ ((i / c) & 1) ? bg2 : b;
                        for(k = 0; k < c && i + 1 < w && x + i < dw; i++, k++) dst[p + i] = bg1;
                    }
        } else {
            for(j=0; j + t + 2 < h && y + j < le16toh(meg4.mmio.cropy1); j++, p += pitch)
                if(y + j >= le16toh(meg4.mmio.cropy0)) {
                    bg1 = r > 0 && (((j + o) / r) & 1) ? bg2 : b;
                    for(i=1; i + 1 < w && x + i < dw; i++) dst[p + i] = bg1;
                }
        }
        if(s) {
            p += pitch;
            if(y + h >= le16toh(meg4.mmio.cropy0) && y + h < le16toh(meg4.mmio.cropy1)) {
                for(i = 0, a = (uint8_t*)&dst[p + 1]; i < w + 1 && x + i < dw; i++, a += 4) {
                    a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                }
                p += pitch;
                if(y + h + 1 < le16toh(meg4.mmio.cropy1)) {
                    for(i = 0, a = (uint8_t*)&dst[p + 1]; i < w + 1 && x + i < dw; i++, a += 4) {
                        a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                    }
                }
            }
        }
    }
}

/**
 * Display a checkbox
 */
void meg4_chk(uint32_t *dst, int dw, int dh, int dp, int x, int y, uint32_t l, uint32_t d, int c)
{
    int p = dp >> 2;
    meg4_box(dst, dw, dh, dp, x, y, 7, 7, l, theme[THEME_INP_BG], d, 0, 0, 0, 0, 0);
    if(c) {
        y += 2; y *= p; x += 2;
        dst[y + x] = dst[y + x + 2] = dst[y + p + x + 1] = dst[y + p + p + x] = dst[y + p + p + x + 2] = theme[THEME_INP_FG];
    }
}

/**
 * Display a number string with extra small glyphs (4x5)
 */
void meg4_number(uint32_t *dst, int dw, int dh, int dp, int x, int y, uint32_t c, int n)
{
    int i, j, k, l, m = 1000, d = 0, p;
    uint32_t b[] = {0x4aaa4,0x4c444,0xc248e,0xc2c2c,0x24ae2,0xe8c2c,0x68eac,0xe2444,0x4a4a4,0xea62c};

    if(!dst) return;
    dp /= 4; p = dp * y;
    if(dh > (int)le16toh(meg4.mmio.cropy1)) dh = le16toh(meg4.mmio.cropy1);
    if(!n) { x += 12; goto zero; }
    if(n < 0) { n = -n; j = n < 100 ? (n < 10 ? 8 : 4) : 0; dst[x + 2*dp + j] = dst[x + 2*dp + 1 + j] = dst[x + 2*dp + 2 + j] = c; }
    for(; m > 0 && x < dw; m /= 10, x += 4)
        if(n >= m) {
            d = (n/m)%10;
zero:       for(k = 1<<19, j = l = 0; j < 5 && y + j < dh; j++, l += dp)
                if(y + j >= le16toh(meg4.mmio.cropy0))
                    for(i = 0; i < 4 && x + i < dw; i++, k >>= 1)
                        if((b[d] & k)) dst[p + l + x + i] = c;
        }
}

/**
 * Display one full width monospace character (only respects Y cropping, but not X)
 */
void meg4_char(uint32_t *dst, int dp, int dx, int dy, uint32_t c, int type, uint8_t *font, int idx)
{
    uint8_t *fnt;
    uint32_t *d, *ds, *y0, *y1;
    int x, y, m, p = dp / 4, pt = type * p;

    if(!dst || dp < 4 || !c || type < 1 || type > 2 || !font || idx < 0 || idx > 0xffff) return;
    fnt = font + 8 * idx;
    y0 = dst + le16toh(meg4.mmio.cropy0) * p + le16toh(meg4.mmio.cropx0);
    y1 = dst + le16toh(meg4.mmio.cropy1) * p + le16toh(meg4.mmio.cropx0);
    for(ds = dst + dy * p + dx, y = 0; y < 8; y++, fnt++, ds += pt)
        if(ds >= y0 && ds < y1)
            switch(type) {
                case 1:
                    for(d = ds, x = 0, m = 1; x < 8; x++, d++, m <<= 1)
                        if(*fnt & m) d[0] = c;
                break;
                case 2:
                    for(d = ds, x = 0, m = 1; x < 8; x++, d += 2, m <<= 1)
                        if(*fnt & m) d[0] = d[1] = d[p] = d[p + 1] = c;
                break;
            }
}
#endif

/**
 * Draw string
 * type = 0: do not draw, just return width in pixels
 * type > 0: draw proportional scaled
 * type < 0: draw monospace scaled
 */
int meg4_text(uint32_t *dst, int dx, int dy, int dp, uint32_t color, uint32_t shadow, int type, uint8_t *font, char *str)
{
    int x, y, Y, w = 0, W = 0, j = 0, k, m, n, l, r, p, p2, p3, p4, px, pt, t = type < 0 ? -type : type, s = t * 8, inv = 0;
    uint32_t c, A, B, C, D, E, F, G, H;
    uint8_t *fnt, *a, *b = (uint8_t*)&color, *d, *e;
    char *end;

    if(type < -4 || type > 4 || !font || !str || (type && (!dst || dx < 0 || dy < 0 || dp < 4))) return 0;
    /* force a maximum number of characters to display just in case */
    l = strlen(str); if(l > 256) { l = 256; } end = str + l;
    if((uintptr_t)str >= (uintptr_t)&meg4.data && (uintptr_t)str < (uintptr_t)&meg4.data + sizeof(meg4.data) &&
      end > (char*)meg4.data + sizeof(meg4.data) - 1)
        end = (char*)meg4.data + sizeof(meg4.data) - 1;
    p = dp >> 2; p2 = 2 * dp; p3 = 3 * dp; p4 = 4 * dp; px = 4 * t; pt = t * dp;
    d = (uint8_t*)(dst + dy * p + dx);
    A = b[2]*b[3]; B = b[1]*b[3]; C = b[0]*b[3]; D = 255 - b[3];
    b = (uint8_t*)&shadow;
    E = b[2]*b[3]; F = b[1]*b[3]; G = b[0]*b[3]; H = 255 - b[3];
    while(str < end && *str) {
#ifndef NOEDITORS
        inv = (str == textinp_cur) && (le16toh(meg4.mmio.tick) & 512);
#endif
        str = meg4_utf8(str, &c);
        if(c == '\r') { w = j = 0; d = (uint8_t*)(dst + dy * p + dx); continue; }
        if(c == '\n') { w = j = 0; dy += s; d = (uint8_t*)(dst + dy * p + dx); continue; }
        if(dy >= le16toh(meg4.mmio.cropy1)) break;
        /* special characters: keyboard, gamepad and mouse */
        if(c == 0x2328 || c == 0x1F3AE || c == 0x1F5B1) {
            if(!type) { w += 8 + 1; if(W < w) W = w; } else {
#ifndef NOEDITORS
                if(inv)
                    meg4_box(dst, le16toh(meg4.mmio.cropx1), le16toh(meg4.mmio.cropy1), dp, dx + j, dy, s, s, color, color, color, 0, 0, 0, 0, 0);
#endif
                meg4_blit(dst, dx + j, dy, dp, 8, 8, meg4_icons.buf, c == 0x2328 ? 32 : (c == 0x1F3AE ? 40 : 48), 64,
                    meg4_icons.w * 4, t);
                j += s + 1;
            }
            continue;
        }
        /* emoji */
        if(c >= EMOJI_FIRST && c < EMOJI_LAST) {
            c -= EMOJI_FIRST; if(c >= (uint32_t)(sizeof(emoji)/sizeof(emoji[0])) || !emoji[c]) continue;
            if(!type) { w += 8 + 1; if(W < w) W = w; } else {
#ifndef NOEDITORS
                if(inv)
                    meg4_box(dst, le16toh(meg4.mmio.cropx1), le16toh(meg4.mmio.cropy1), dp, dx + j, dy, s, s, color, color, color, 0, 0, 0, 0, 0);
#endif
                meg4_blit(dst, dx + j, dy, dp, 8, 8, meg4_icons.buf, ((int)emoji[c] % 13) * 8, 64 + ((int)emoji[c] / 13) * 8,
                    meg4_icons.w * 4, t);
                j += s + 1;
            }
            continue;
        }
        if(c > 0xffff) continue;
        if(!font[8 * 65536 + c] && meg4_isbyte(font + 8 * c, 0, 8)) c = 0;
        fnt = font + 8 * c;
        if(type < 0) { l = 0; r = 7; } else { l = font[8 * 65536 + c] & 0xf; r = font[8 * 65536 + c] >> 4; }
        w += r - l + 2; if(W < w) W = w;
        if(c == 32) { j += t * (r - l + 1) + 1; continue; }
        if(!type) continue;
        for(e = d + (j << 2), y = 0, Y = dy + t + (shadow ? 1 : 0); y < 8 && Y < le16toh(meg4.mmio.cropy1); y++, Y += t, fnt++, e += pt) {
            if(dy + y * t >= le16toh(meg4.mmio.cropy0))
                for(a = e, x = l, m = (1 << l), n = dx + j, k = n + (shadow ? 1 : 0); x <= r && k < le16toh(meg4.mmio.cropx1); x++, k += t, a += px, m <<= 1)
                    if(((!inv && (*fnt & m)) || (inv && !(*fnt & m))) && n + x - l >= le16toh(meg4.mmio.cropx0))
                        switch(t) {
                            case 1:
                                a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                if(shadow) {
                                    a[dp+6] = (E + H*a[dp+6]) >> 8; a[dp+5] = (F + H*a[dp+5]) >> 8; a[dp+4] = (G + H*a[dp+4]) >> 8;
                                }
                            break;
                            case 2:
                                a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                                a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                                a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                                if(shadow) {
                                    a[dp+10] = (E + H*a[dp+10]) >> 8; a[dp+9] = (F + H*a[dp+9]) >> 8; a[dp+8] = (G + H*a[dp+8]) >> 8;
                                    a[p2+6] = (E + H*a[p2+6]) >> 8; a[p2+5] = (F + H*a[p2+5]) >> 8; a[p2+4] = (G + H*a[p2+4]) >> 8;
                                    a[p2+10] = (E + H*a[p2+10]) >> 8; a[p2+9] = (F + H*a[p2+9]) >> 8; a[p2+8] = (G + H*a[p2+8]) >> 8;
                                }
                            break;
                            case 3:
                                a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                                a[10] = (A + D*a[10]) >> 8; a[9] = (B + D*a[9]) >> 8; a[8] = (C + D*a[8]) >> 8;
                                a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                                a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                                a[dp+10] = (A + D*a[dp+10]) >> 8; a[dp+9] = (B + D*a[dp+9]) >> 8; a[dp+8] = (C + D*a[dp+8]) >> 8;
                                a[p2+2] = (A + D*a[p2+2]) >> 8; a[p2+1] = (B + D*a[p2+1]) >> 8; a[p2+0] = (C + D*a[p2+0]) >> 8;
                                a[p2+6] = (A + D*a[p2+6]) >> 8; a[p2+5] = (B + D*a[p2+5]) >> 8; a[p2+4] = (C + D*a[p2+4]) >> 8;
                                a[p2+10] = (A + D*a[p2+10]) >> 8; a[p2+9] = (B + D*a[p2+9]) >> 8; a[p2+8] = (C + D*a[p2+8]) >> 8;
                                if(shadow) {
                                    a[dp+14] = (E + H*a[dp+14]) >> 8; a[dp+13] = (F + H*a[dp+13]) >> 8; a[dp+12] = (G + H*a[dp+12]) >> 8;
                                    a[p2+14] = (E + H*a[p2+14]) >> 8; a[p2+13] = (F + H*a[p2+13]) >> 8; a[p2+12] = (G + H*a[p2+12]) >> 8;
                                    a[p3+6] = (E + H*a[p3+6]) >> 8; a[p3+5] = (F + H*a[p3+5]) >> 8; a[p3+4] = (G + H*a[p3+4]) >> 8;
                                    a[p3+10] = (E + H*a[p3+10]) >> 8; a[p3+9] = (F + H*a[p3+9]) >> 8; a[p3+8] = (G + H*a[p3+8]) >> 8;
                                    a[p3+14] = (E + H*a[p3+14]) >> 8; a[p3+13] = (F + H*a[p3+13]) >> 8; a[p3+12] = (G + H*a[p3+12]) >> 8;
                                }
                            break;
                            case 4:
                                a[2] = (A + D*a[2]) >> 8; a[1] = (B + D*a[1]) >> 8; a[0] = (C + D*a[0]) >> 8;
                                a[6] = (A + D*a[6]) >> 8; a[5] = (B + D*a[5]) >> 8; a[4] = (C + D*a[4]) >> 8;
                                a[10] = (A + D*a[10]) >> 8; a[9] = (B + D*a[9]) >> 8; a[8] = (C + D*a[8]) >> 8;
                                a[14] = (A + D*a[14]) >> 8; a[13] = (B + D*a[13]) >> 8; a[12] = (C + D*a[12]) >> 8;
                                a[dp+2] = (A + D*a[dp+2]) >> 8; a[dp+1] = (B + D*a[dp+1]) >> 8; a[dp+0] = (C + D*a[dp+0]) >> 8;
                                a[dp+6] = (A + D*a[dp+6]) >> 8; a[dp+5] = (B + D*a[dp+5]) >> 8; a[dp+4] = (C + D*a[dp+4]) >> 8;
                                a[dp+10] = (A + D*a[dp+10]) >> 8; a[dp+9] = (B + D*a[dp+9]) >> 8; a[dp+8] = (C + D*a[dp+8]) >> 8;
                                a[dp+14] = (A + D*a[dp+14]) >> 8; a[dp+13] = (B + D*a[dp+13]) >> 8; a[dp+12] = (C + D*a[dp+12]) >> 8;
                                a[p2+2] = (A + D*a[p2+2]) >> 8; a[p2+1] = (B + D*a[p2+1]) >> 8; a[p2+0] = (C + D*a[p2+0]) >> 8;
                                a[p2+6] = (A + D*a[p2+6]) >> 8; a[p2+5] = (B + D*a[p2+5]) >> 8; a[p2+4] = (C + D*a[p2+4]) >> 8;
                                a[p2+10] = (A + D*a[p2+10]) >> 8; a[p2+9] = (B + D*a[p2+9]) >> 8; a[p2+8] = (C + D*a[p2+8]) >> 8;
                                a[p2+14] = (A + D*a[p2+14]) >> 8; a[p2+13] = (B + D*a[p2+13]) >> 8; a[p2+12] = (C + D*a[p2+12]) >> 8;
                                a[p3+2] = (A + D*a[p3+2]) >> 8; a[p3+1] = (B + D*a[p3+1]) >> 8; a[p3+0] = (C + D*a[p3+0]) >> 8;
                                a[p3+6] = (A + D*a[p3+6]) >> 8; a[p3+5] = (B + D*a[p3+5]) >> 8; a[p3+4] = (C + D*a[p3+4]) >> 8;
                                a[p3+10] = (A + D*a[p3+10]) >> 8; a[p3+9] = (B + D*a[p3+9]) >> 8; a[p3+8] = (C + D*a[p3+8]) >> 8;
                                a[p3+14] = (A + D*a[p3+14]) >> 8; a[p3+13] = (B + D*a[p3+13]) >> 8; a[p3+12] = (C + D*a[p3+12]) >> 8;
                                if(shadow) {
                                    a[dp+18] = (E + H*a[dp+18]) >> 8; a[dp+17] = (F + H*a[dp+17]) >> 8; a[dp+16] = (G + H*a[dp+16]) >> 8;
                                    a[p2+18] = (E + H*a[p2+18]) >> 8; a[p2+17] = (F + H*a[p2+17]) >> 8; a[p2+16] = (G + H*a[p2+16]) >> 8;
                                    a[p3+18] = (E + H*a[p3+18]) >> 8; a[p3+17] = (F + H*a[p3+17]) >> 8; a[p3+16] = (G + H*a[p3+16]) >> 8;
                                    a[p4+6] = (E + H*a[p4+6]) >> 8; a[p4+5] = (F + H*a[p4+5]) >> 8; a[p4+4] = (G + H*a[p4+4]) >> 8;
                                    a[p4+10] = (E + H*a[p4+10]) >> 8; a[p4+9] = (F + H*a[p4+9]) >> 8; a[p4+8] = (G + H*a[p4+8]) >> 8;
                                    a[p4+14] = (E + H*a[p4+14]) >> 8; a[p4+13] = (F + H*a[p4+13]) >> 8; a[p4+12] = (G + H*a[p4+12]) >> 8;
                                    a[p4+18] = (E + H*a[p4+18]) >> 8; a[p4+17] = (F + H*a[p4+17]) >> 8; a[p4+16] = (G + H*a[p3+16]) >> 8;
                                }
                            break;
                        }
        }
        j += t * (r - l + 1) + 1;
    }
    if(W < w) W = w;
#ifndef NOEDITORS
    if(type && (str >= end || !*str) && (str == textinp_cur) && (le16toh(meg4.mmio.tick) & 512))
        meg4_box(dst, le16toh(meg4.mmio.cropx1), le16toh(meg4.mmio.cropy1), dp, dx + j, dy, t * (type < 0 ? 8 : 4), s, color, color, color, 0, 0, 0, 0, 0);
#endif
    return W;
}

/**
 * Returns text length in pixels
 */
int meg4_width(uint8_t *font, int8_t type, char *str, char *end)
{
    int w = 0, W = 0, l, r, t = type < 0 ? -type : (!type ? 1 : type);
    uint32_t c;

    if(!font || type < -4 || type > 4 || !str || !*str) return 0;
    if(!end) end = str + 256;
    if((uintptr_t)str >= (uintptr_t)&meg4.data && (uintptr_t)str < (uintptr_t)&meg4.data + sizeof(meg4.data) &&
      (uintptr_t)end > (uintptr_t)&meg4.data + sizeof(meg4.data)) end = (char*)meg4.data + sizeof(meg4.data);
    while(str < end && *str) {
        str = meg4_utf8(str, &c);
        if(W < w) W = w;
        if(c == '\r' || c == '\n') w = 0; else
        if(c == 0x2328 || c == 0x1F3AE || c == 0x1F5B1 ||
          (c >= EMOJI_FIRST && c < EMOJI_LAST && c - EMOJI_FIRST < (uint32_t)(sizeof(emoji)/sizeof(emoji[0])) && emoji[c - EMOJI_FIRST]))
            w += t * 8 + 1; else
        if(c <= 0xffff) {
            if(!font[8 * 65536 + c] && meg4_isbyte(font + 8 * c, 0, 8)) c = 0;
            if(type < 0) { l = 0; r = 7; } else { l = font[8 * 65536 + c] & 0xf; r = font[8 * 65536 + c] >> 4; }
            w += t * (r - l + 1) + 1;
        }
    }
    if(W < w) W = w;
    return W;
}

/**
 * Remaps map with new sprites
 */
void meg4_remap(uint8_t *replace)
{
    int i;
    if(replace) for(i = 0; i < 320 * 200; i++) meg4.mmio.map[i] = replace[(int)meg4.mmio.map[i]];
}

/**
 * Set a pixel at a coordinate with specific color channels
 */
static __inline__ void meg4_setpixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint8_t *d;
    int i;
    if(a && x >= le16toh(meg4.mmio.cropx0) && x < le16toh(meg4.mmio.cropx1) && y >= le16toh(meg4.mmio.cropy0) && y < le16toh(meg4.mmio.cropy1)) {
        d = (uint8_t*)&meg4.vram[y * 640 + x]; i = 255 - a;
        d[2] = (b*a + i*d[2]) >> 8; d[1] = (g*a + i*d[1]) >> 8; d[0] = (r*a + i*d[0]) >> 8;
    }
}

/**
 * Fill a scanline
 */
static __inline__ void meg4_fill(uint16_t x0, uint16_t x1, uint16_t y, uint8_t *c)
{
    uint8_t *d;
    int i, x, xs, xe, r, g, b;
    if(c[3] && x0 < x1 && y >= le16toh(meg4.mmio.cropy0) && y < le16toh(meg4.mmio.cropy1)) {
        xs = x0 < le16toh(meg4.mmio.cropx0) ? le16toh(meg4.mmio.cropx0) : x0;
        xe = x1 < le16toh(meg4.mmio.cropx1) ? x1 : le16toh(meg4.mmio.cropx1);
        i = 255 - c[3]; r = c[0]*c[3]; g = c[1]*c[3]; b = c[2]*c[3];
        d = (uint8_t*)&meg4.vram[y * 640 + xs];
        for(x = xs; x <= xe; x++, d += 4) { d[2] = (b + i*d[2]) >> 8; d[1] = (g + i*d[1]) >> 8; d[0] = (r + i*d[0]) >> 8; }
    }
}

/**
 * Recursively calculate Bezier curve
 */
static int bezx, bezy;
static __inline__ void meg4_bezier(uint8_t palidx, int x0,int y0, int x1,int y1, int x2,int y2, int x3,int y3, int l)
{
    int m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, m4x, m4y,m5x, m5y;
    if(l < 8 && (x0 != x3 || y0 != y3)) {
        m0x = ((x1-x0)/2) + x0;     m0y = ((y1-y0)/2) + y0;
        m1x = ((x2-x1)/2) + x1;     m1y = ((y2-y1)/2) + y1;
        m2x = ((x3-x2)/2) + x2;     m2y = ((y3-y2)/2) + y2;
        m3x = ((m1x-m0x)/2) + m0x;  m3y = ((m1y-m0y)/2) + m0y;
        m4x = ((m2x-m1x)/2) + m1x;  m4y = ((m2y-m1y)/2) + m1y;
        m5x = ((m4x-m3x)/2) + m3x;  m5y = ((m4y-m3y)/2) + m3y;
        meg4_bezier(palidx, x0,y0, m0x,m0y, m3x,m3y, m5x,m5y, l + 1);
        meg4_bezier(palidx, m5x,m5y, m4x,m4y, m2x,m2y, x3,y3, l + 1);
    }
    if(l) {
        /* FIXME: we should use a specialized line drawing routine here that uses 8 bit fixed precision coordinates and
         * calculates alpha accordingly. This sticks the point to the pixel grid, thus loosing anti-alias details */
        meg4_api_line(palidx, bezx >> 8, bezy >> 8, x3 >> 8, y3 >> 8);
        bezx = x3; bezy = y3;
    }
}

/**
 * Clears the entire screen and resets display offsets, also sets the console's background color.
 * @param palidx color, palette index 0 to 255
 * @see [pget], [pset]
 */
void meg4_api_cls(uint8_t palidx)
{
    uint32_t bg = (0xff << 24) | meg4.mmio.palette[(int)palidx], fg = meg4.mmio.palette[(int)meg4.mmio.conf];
    uint8_t c[4];
    int i, j;

    meg4.mmio.scrx = meg4.mmio.scry = meg4.mmio.conx = meg4.mmio.cony = 0;
    meg4.screen.w = 320; meg4.screen.h = 200; meg4.screen.buf = meg4.vram;
    for(i = 0; i < 640 * 400; i++) meg4.vram[i] = bg;
    /* set console's color either black or white depending if this clear color is bright or dark */
    meg4_conrst(); meg4.mmio.conb = palidx; c[3] = 0xff;
    j = ((fg & 0xff) + ((fg >> 8) & 0xff) + ((fg >> 16) & 0xff)) / 3;
    if(((bg & 0xff) + ((bg >> 8) & 0xff) + ((bg >> 16) & 0xff)) / 3 < 0x7f) {
        /* check if both background and foreground are dark */
        if(j < 0x7f) { c[0] = c[1] = c[2] = 255; meg4.mmio.conf = meg4_palidx(c); }
    } else {
        /* check if both background and foreground are bright */
        if(j >= 0x80) { c[0] = c[1] = c[2] = 0; meg4.mmio.conf = meg4_palidx(c); }
    }
    if(zclear) {
        zclear = 0;
        memset(zbuf, 0, sizeof(zbuf));
    }
}

/**
 * Get pixel at a coordinate and return color RGBA.
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @return A packed color, with RGBA channels (red is in the least significant byte).
 * @see [cls], [pget], [pset]
 */
uint32_t meg4_api_cget(uint16_t x, uint16_t y)
{
    return x < 640 && y < 400 ? le32toh(meg4.vram[y * 640 + x]) : 0;
}

/**
 * Get pixel at a coordinate and return its palette index.
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @return Color in palette index, 0 to 255.
 * @see [cls], [pset], [cget]
 */
uint8_t meg4_api_pget(uint16_t x, uint16_t y)
{
    return x < 640 && y < 400 ? meg4_palidx((uint8_t*)&meg4.vram[y * 640 + x]) : 0;
}

/**
 * Plots a pixel at a coordinate.
 * @param palidx color, palette index 0 to 255
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @see [cls], [pget]
 */
void meg4_api_pset(uint8_t palidx, uint16_t x, uint16_t y)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    if(x < 640 && y < 400)
        meg4_setpixel(x, y, c[0], c[1], c[2], c[3]);
}

/**
 * Returns displayed text's width in pixels.
 * @param type font type, -4 to 4
 * @param str string to measure
 * @return Number of pixels required to display text.
 * @see [text]
 */
uint16_t meg4_api_width(int8_t type, str_t str)
{
    if(type < -4 || type > 4 || str < MEG4_MEM_USER || str >= MEG4_MEM_LIMIT) return 0;
    return meg4_width(meg4.font, type, (char*)meg4.data + str - MEG4_MEM_USER, (char*)meg4.data + sizeof(meg4.data) - 1);
}

/**
 * Prints text on screen.
 * @param palidx color, palette index 0 to 255
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @param type font type, -4 to -1 monospace, 1 to 4 proportional
 * @param shidx shadow's color, palette index 0 to 255
 * @param sha shadow's alpha, 0 (fully transparent) to 255 (fully opaque)
 * @param str string to display
 * @see [width]
 */
void meg4_api_text(uint8_t palidx, int16_t x, int16_t y, int8_t type, uint8_t shidx, uint8_t sha, str_t str)
{
    uint32_t shadow = shidx ? (meg4.mmio.palette[(int)shidx] & htole32(0xffffff)) | (sha << 24) : 0;
    if(!type) type = 1;
    if(str < MEG4_MEM_USER || str >= MEG4_MEM_LIMIT) return;
    meg4_text(meg4.vram, x, y, 2560, meg4.mmio.palette[(int)palidx], shadow, type, meg4.font,
        (char*)meg4.data + str - MEG4_MEM_USER);
}

/**
 * Draws an anti-aliased line.
 * @param palidx color, palette index 0 to 255
 * @param x0 starting X coordinate in pixels
 * @param y0 starting Y coordinate in pixels
 * @param x1 ending X coordinate in pixels
 * @param y1 ending Y coordinate in pixels
 * @see [qbez], [cbez]
 */
void meg4_api_line(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    /* (coordinates here are signed, because the turtle might wander off screen) */
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, x2, a;
    int dx = abs(x1-x0), dy = abs(y1-y0), err = dx*dx+dy*dy;
    int e2 = err == 0 ? 1 : 0xffff7fl/sqrt(err);

    if(!c[3] || (x0 == x1 && y0 == y1)) return;
    dx *= e2; dy *= e2; err = dx-dy;
    while(1) {
        a = err-dx+dy; if(a < 0) a = -a;
        a = 255 - (a >> 16); a = a * c[3] / 255;
        meg4_setpixel(x0, y0, c[0], c[1], c[2], a);
        e2 = err; x2 = x0;
        if(2*e2 >= -dx) {
            if(x0 == x1) break;
            if(e2+dy < 0xff0000l) {
                a = 255 - ((e2+dy) >> 16); a = a * c[3] / 255;
                meg4_setpixel(x0, y0 + sy, c[0], c[1], c[2], a);
            }
            err -= dy; x0 += sx;
        }
        if(2*e2 <= dy) {
            if(y0 == y1) break;
            if(dx-e2 < 0xff0000l) {
                a = 255 - ((dx-e2) >> 16); a = a * c[3] / 255;
                meg4_setpixel(x2 + sx, y0, c[0], c[1], c[2], a);
            }
            err += dx; y0 += sy;
        }
    }
}

/**
 * Draws a quadratic Bezier curve.
 * @param palidx color, palette index 0 to 255
 * @param x0 starting X coordinate in pixels
 * @param y0 starting Y coordinate in pixels
 * @param x1 ending X coordinate in pixels
 * @param y1 ending Y coordinate in pixels
 * @param cx control point X coordinate in pixels
 * @param cy control point Y coordinate in pixels
 * @see [line], [cbez]
 */
void meg4_api_qbez(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
    int16_t cx, int16_t cy)
{
    if(((uint8_t*)&meg4.mmio.palette[(int)palidx])[3]) {
        bezx = x0 << 8; bezy = y0 << 8;
        meg4_bezier(palidx, x0 << 8, y0 << 8, (x0 << 8) + (((cx << 8) - (x0 << 8)) >> 1), (y0 << 8) + (((cy << 8) - (y0 << 8)) >> 1),
            (cx << 8) + (((x1 << 8) - (cx << 8)) >> 1), (cy << 8) + (((y1 << 8) - (cy << 8)) >> 1), (x1 << 8), (y1 << 8), 0);
    }
}

/**
 * Draws a cubic Bezier curve.
 * @param palidx color, palette index 0 to 255
 * @param x0 starting X coordinate in pixels
 * @param y0 starting Y coordinate in pixels
 * @param x1 ending X coordinate in pixels
 * @param y1 ending Y coordinate in pixels
 * @param cx0 first control point X coordinate in pixels
 * @param cy0 first control point Y coordinate in pixels
 * @param cx1 second control point X coordinate in pixels
 * @param cy1 second control point Y coordinate in pixels
 * @see [line], [qbez]
 */
void meg4_api_cbez(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
    int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1)
{
    if(((uint8_t*)&meg4.mmio.palette[(int)palidx])[3]) {
        bezx = x0 << 8; bezy = y0 << 8;
        meg4_bezier(palidx, x0 << 8, y0 << 8, cx0 << 8, cy0 << 8, cx1 << 8, cy1 << 8, x1 << 8, y1 << 8, 0);
    }
}

/**
 * Draws a triangle.
 * @param palidx color, palette index 0 to 255
 * @param x0 first edge X coordinate in pixels
 * @param y0 first edge Y coordinate in pixels
 * @param x1 second edge X coordinate in pixels
 * @param y1 second edge Y coordinate in pixels
 * @param x2 third edge X coordinate in pixels
 * @param y2 third edge Y coordinate in pixels
 * @see [ftri], [tri2d], [tri3d], [tritx], [mesh], [trns]
 */
void meg4_api_tri(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    if(((uint8_t*)&meg4.mmio.palette[(int)palidx])[3]) {
        meg4_api_line(palidx, x0, y0, x1, y1);
        meg4_api_line(palidx, x1, y1, x2, y2);
        meg4_api_line(palidx, x2, y2, x0, y0);
    }
}

/**
 * Draws a filled triangle.
 * @param palidx color, palette index 0 to 255
 * @param x0 first edge X coordinate in pixels
 * @param y0 first edge Y coordinate in pixels
 * @param x1 second edge X coordinate in pixels
 * @param y1 second edge Y coordinate in pixels
 * @param x2 third edge X coordinate in pixels
 * @param y2 third edge Y coordinate in pixels
 * @see [tri], [tri2d], [tri3d], [tritx], [mesh], [trns]
 */
void meg4_api_ftri(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    int i, j, h, s, xa, xb, y, d1, d2, d3;
    float a, b, ia, ib;

    meg4_api_tri(palidx, x0, y0, x1, y1, x2, y2);
    if(!c[3] || (y0 == y1 && y0 == y2) || (x0 == x1 && x0 == x2)) return;
    if(y0 > y1) { i = x0; x0 = x1; x1 = i; i = y0; y0 = y1; y1 = i; }
    if(y0 > y2) { i = x0; x0 = x2; x2 = i; i = y0; y0 = y2; y2 = i; }
    if(y1 > y2) { i = x1; x1 = x2; x2 = i; i = y1; y1 = y2; y2 = i; }
    d1 = y1 - y0; d2 = y2 - y1; d3 = y2 - y0;
    if(y0 > le16toh(meg4.mmio.cropy0)) { y = y0; i = 0; } else { y = le16toh(meg4.mmio.cropy0); i = le16toh(meg4.mmio.cropy0) - y0; }
    for(; i < d3 && y < le16toh(meg4.mmio.cropy1); i++, y++) {
        h = i > d1 || y1 == y0; s = h ? d2 : d1;
        a = (float)i / (float)d3; ia = 1.0 - a; b = ((float)i - (float)(h ? d1 : 0)) / (float)s; ib = 1.0 - b;
        xa = ia * x0 + a * x2; xb = h ? ib * x1 + b * x2 : ib * x0 + b * x1;
        if(xa > xb) { j = xa; xa = xb; xb = j; }
        meg4_fill(xa + 1, xb, y, c);
    }
}

/**
 * Draws a filled triangle with color gradients.
 * @param pi0 first edge color, palette index 0 to 255
 * @param x0 first edge X coordinate in pixels
 * @param y0 first edge Y coordinate in pixels
 * @param pi1 second edge color, palette index 0 to 255
 * @param x1 second edge X coordinate in pixels
 * @param y1 second edge Y coordinate in pixels
 * @param pi2 third edge color, palette index 0 to 255
 * @param x2 third edge X coordinate in pixels
 * @param y2 third edge Y coordinate in pixels
 * @see [tri], [ftri], [tri3d], [tritx], [mesh], [trns]
 */
void meg4_api_tri2d(uint8_t pi0, int16_t x0, int16_t y0,
    uint8_t pi1, int16_t x1, int16_t y1,
    uint8_t pi2, int16_t x2, int16_t y2)
{
    uint8_t c[4], *c0, *c1, *c2, *d, e[4], f[4];
    int i, j, k, l, m, h, s, x, xa, xb, xs, xe, y, d1, d2, d3;
    float a, b, ia, ib, g, ig;

    if((y0 == y1 && y0 == y2) || (x0 == x1 && x0 == x2)) return;
    if(y0 > y1) { i = x0; x0 = x1; x1 = i; i = y0; y0 = y1; y1 = i; i = pi0; pi0 = pi1; pi1 = i; }
    if(y0 > y2) { i = x0; x0 = x2; x2 = i; i = y0; y0 = y2; y2 = i; i = pi0; pi0 = pi2; pi2 = i; }
    if(y1 > y2) { i = x1; x1 = x2; x2 = i; i = y1; y1 = y2; y2 = i; i = pi1; pi1 = pi2; pi2 = i; }
    c0 = (uint8_t*)&meg4.mmio.palette[(int)pi0];
    c1 = (uint8_t*)&meg4.mmio.palette[(int)pi1];
    c2 = (uint8_t*)&meg4.mmio.palette[(int)pi2];
    d1 = y1 - y0; d2 = y2 - y1; d3 = y2 - y0;
    if(y0 > le16toh(meg4.mmio.cropy0)) { y = y0; i = 0; } else { y = le16toh(meg4.mmio.cropy0); i = le16toh(meg4.mmio.cropy0) - y0; }
    for(; i < d3 && y < le16toh(meg4.mmio.cropy1); i++, y++) {
        h = i > d1 || y1 == y0; s = h ? d2 : d1;
        a = (float)i / (float)d3; ia = 1.0 - a; b = ((float)i - (float)(h ? d1 : 0)) / (float)s; ib = 1.0 - b;
        xa = ia * x0 + a * x2; xb = h ? ib * x1 + b * x2 : ib * x0 + b * x1;
        if(xa > xb) { j = xa; xa = xb; xb = j; }
        xs = xa < le16toh(meg4.mmio.cropx0) ? le16toh(meg4.mmio.cropx0) : xa;
        xe = xb < le16toh(meg4.mmio.cropx1) ? xb : le16toh(meg4.mmio.cropx1);
        k = xs - xa; l = xb - xa + 1;
        e[0] = ia * c0[0] + a * c2[0]; e[1] = ia * c0[1] + a * c2[1]; e[2] = ia * c0[2] + a * c2[2]; e[3] = ia * c0[3] + a * c2[3];
        if(h) {
            f[0] = ib * c1[0] + b * c2[0]; f[1] = ib * c1[1] + b * c2[1]; f[2] = ib * c1[2] + b * c2[2]; f[3] = ib * c1[3] + b * c2[3];
        } else {
            f[0] = ib * c0[0] + b * c1[0]; f[1] = ib * c0[1] + b * c1[1]; f[2] = ib * c0[2] + b * c1[2]; f[3] = ib * c0[3] + b * c1[3];
        }
        d = (uint8_t*)&meg4.vram[y * 640 + xs];
        for(x = xs; x <= xe; k++, x++, d += 4) {
            g = (float)k / (float)l; ig = 1.0 - g;
            c[0] = ig * e[0] + g * f[0]; c[1] = ig * e[1] + g * f[1]; c[2] = ig * e[2] + g * f[2]; c[3] = ig * e[3] + g * f[3];
            m = 255 - c[3]; d[2] = (c[2]*c[3] + m*d[2]) >> 8; d[1] = (c[1]*c[3] + m*d[1]) >> 8; d[0] = (c[0]*c[3] + m*d[0]) >> 8;
        }
    }
}

/**
 * Draws a filled triangle with color gradients in [3D space].
 * @param pi0 first edge color, palette index 0 to 255
 * @param x0 first edge X coordinate in space
 * @param y0 first edge Y coordinate in space
 * @param z0 first edge Z coordinate in space
 * @param pi1 second edge color, palette index 0 to 255
 * @param x1 second edge X coordinate in space
 * @param y1 second edge Y coordinate in space
 * @param z1 second edge Z coordinate in space
 * @param pi2 third edge color, palette index 0 to 255
 * @param x2 third edge X coordinate in space
 * @param y2 third edge Y coordinate in space
 * @param z2 third edge Z coordinate in space
 * @see [tri], [ftri], [tri2d], [tritx], [mesh], [trns]
 */
void meg4_api_tri3d(uint8_t pi0, int16_t x0, int16_t y0, int16_t z0,
    uint8_t pi1, int16_t x1, int16_t y1, int16_t z1,
    uint8_t pi2, int16_t x2, int16_t y2, int16_t z2)
{
    zclear = 1; nvert = nface = 0;
    vertex(x0/32767.0f, y0/32767.0f, z0/32767.0f);
    vertex(x1/32767.0f, y1/32767.0f, z1/32767.0f);
    vertex(x2/32767.0f, y2/32767.0f, z2/32767.0f);
    face(0, pi0, 0, 0, 1, pi1, 0, 0, 2, pi2, 0, 0);
    processtri(0);
}

/**
 * Draws a textured triangle in [3D space].
 * @param u0 first edge texture X coordinate 0 to 255
 * @param v0 first edge texture Y coordinate 0 to 255
 * @param x0 first edge X coordinate in space
 * @param y0 first edge Y coordinate in space
 * @param z0 first edge Z coordinate in space
 * @param u1 second edge texture X coordinate 0 to 255
 * @param v1 second edge texture Y coordinate 0 to 255
 * @param x1 second edge X coordinate in space
 * @param y1 second edge Y coordinate in space
 * @param z1 second edge Z coordinate in space
 * @param u2 third edge texture X coordinate 0 to 255
 * @param v2 third edge texture Y coordinate 0 to 255
 * @param x2 third edge X coordinate in space
 * @param y2 third edge Y coordinate in space
 * @param z2 third edge Z coordinate in space
 * @see [tri], [ftri], [tri2d], [tri3d], [mesh], [trns]
 */
void meg4_api_tritx(uint8_t u0, uint8_t v0, int16_t x0, int16_t y0, int16_t z0,
    uint8_t u1, uint8_t v1, int16_t x1, int16_t y1, int16_t z1,
    uint8_t u2, uint8_t v2, int16_t x2, int16_t y2, int16_t z2)
{
    zclear = 1; nvert = nface = 0;
    vertex(x0/32767.0f, y0/32767.0f, z0/32767.0f);
    vertex(x1/32767.0f, y1/32767.0f, z1/32767.0f);
    vertex(x2/32767.0f, y2/32767.0f, z2/32767.0f);
    face(0, 0, u0, v0, 1, 0, u1, v1, 2, 0, u2, v2);
    processtri(1);
}

/**
 * Draws a mesh made of triangles in [3D space], using indeces to verticles and texture coordinates (or palette).
 * @param verts address of vertices array, 3 x 2 bytes each, X, Y, Z
 * @param uvs address of UVs array (if 0, then palette is used), 2 x 1 bytes each, texture X, Y
 * @param numtri number of triangles
 * @param tris address of triangles array with indices, 6 x 1 bytes each, vi1, ui1/pi1, vi2, ui2/pi2, vi3, ui3/pi3
 * @see [tri], [ftri], [tri2d], [tri3d], [tritx], [trns]
 */
void meg4_api_mesh(addr_t verts, addr_t uvs, uint16_t numtri, addr_t tris)
{
    int16_t *vs, *v;
    uint8_t *uv, *tr, *ptr;
    uint32_t i, mi;

    zclear = 1; nvert = nface = 0;
    if(numtri > sizeof(faces)/sizeof(faces[0])) numtri = sizeof(faces)/sizeof(faces[0]);
    if(verts < MEG4_MEM_USER || verts + 6 * 256 >= MEG4_MEM_LIMIT ||
      (uvs && (uvs < MEG4_MEM_USER || uvs + 512 >= MEG4_MEM_LIMIT)) ||
      tris < MEG4_MEM_USER || !numtri || tris + numtri * 6 >= MEG4_MEM_LIMIT) return;

    /* get number of vertices, maximum index */
    tr = (uint8_t*)(meg4.data + tris - MEG4_MEM_USER);
    for(i = mi = 0, ptr = tr; i < numtri; i++, ptr += 6) {
        if(ptr[0] > mi) mi = ptr[0];
        if(ptr[2] > mi) mi = ptr[2];
        if(ptr[4] > mi) mi = ptr[4];
    }
    /* add vertices */
    vs = (int16_t*)(meg4.data + verts - MEG4_MEM_USER);
    for(i = 0, v = vs; i <= mi; i++, v += 3)
        vertex(v[0]/32767.0f, v[1]/32767.0f, v[2]/32767.0f);
    /* add triangle faces */
    if(uvs) {
        uv = (uint8_t*)(meg4.data + uvs - MEG4_MEM_USER);
        for(i = 0, ptr = tr; i < numtri; i++, ptr += 6)
            face(ptr[0], 0, uv[ptr[1] << 1], uv[(ptr[1] << 1) + 1],
                 ptr[2], 0, uv[ptr[3] << 1], uv[(ptr[3] << 1) + 1],
                 ptr[4], 0, uv[ptr[5] << 1], uv[(ptr[5] << 1) + 1]);
        processtri(1);
    } else {
        for(i = 0, ptr = tr; i < numtri; i++, ptr += 6)
            face(ptr[0], ptr[1], 0, 0, ptr[2], ptr[3], 0, 0, ptr[4], ptr[5], 0, 0);
        processtri(0);
    }
}

/**
 * Draws a rectangle.
 * @param palidx color, palette index 0 to 255
 * @param x0 top left corner X coordinate in pixels
 * @param y0 top left corner Y coordinate in pixels
 * @param x1 bottom right X coordinate in pixels
 * @param y1 bottom right Y coordinate in pixels
 * @see [frect]
 */
void meg4_api_rect(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    uint8_t *d;
    int i, x, xs, xe, r, g, b;
    if(c[3] && x0 < x1 && y0 < y1 && x0 < le16toh(meg4.mmio.cropx1) && x1 > le16toh(meg4.mmio.cropx0) &&
      y0 < le16toh(meg4.mmio.cropy1) && y1 > le16toh(meg4.mmio.cropy0)) {
        xs = x0 < le16toh(meg4.mmio.cropx0) ? le16toh(meg4.mmio.cropx0) : x0;
        xe = x1 < le16toh(meg4.mmio.cropx1) ? x1 : le16toh(meg4.mmio.cropx1);
        i = 255 - c[3]; r = c[0]*c[3]; g = c[1]*c[3]; b = c[2]*c[3];
        if(y0 >= le16toh(meg4.mmio.cropy0) && y0 < le16toh(meg4.mmio.cropy1)) {
            d = (uint8_t*)&meg4.vram[y0 * 640 + xs];
            for(x = xs; x <= xe; x++, d += 4) { d[2] = (b + i*d[2]) >> 8; d[1] = (g + i*d[1]) >> 8; d[0] = (r + i*d[0]) >> 8; }
        }
        if(y1 >= le16toh(meg4.mmio.cropy0) && y1 < le16toh(meg4.mmio.cropy1)) {
            d = (uint8_t*)&meg4.vram[y1 * 640 + xs];
            for(x = xs; x <= xe; x++, d += 4) { d[2] = (b + i*d[2]) >> 8; d[1] = (g + i*d[1]) >> 8; d[0] = (r + i*d[0]) >> 8; }
        }
        xs = y0 < le16toh(meg4.mmio.cropy0) ? le16toh(meg4.mmio.cropy0) : y0;
        xe = y1 < le16toh(meg4.mmio.cropy1) ? y1 : le16toh(meg4.mmio.cropy1);
        for(x = xs + 1; x < xe; x++) {
            if(x0 >= le16toh(meg4.mmio.cropx0) && x0 < le16toh(meg4.mmio.cropy1)) {
                d = (uint8_t*)&meg4.vram[x * 640 + x0];
                d[2] = (b + i*d[2]) >> 8; d[1] = (g + i*d[1]) >> 8; d[0] = (r + i*d[0]) >> 8;
            }
            if(x1 >= le16toh(meg4.mmio.cropx0) && x1 < le16toh(meg4.mmio.cropx1)) {
                d = (uint8_t*)&meg4.vram[x * 640 + x1];
                d[2] = (b + i*d[2]) >> 8; d[1] = (g + i*d[1]) >> 8; d[0] = (r + i*d[0]) >> 8;
            }
        }
    }
}

/**
 * Draws a filled rectangle.
 * @param palidx color, palette index 0 to 255
 * @param x0 top left corner X coordinate in pixels
 * @param y0 top left corner Y coordinate in pixels
 * @param x1 bottom right X coordinate in pixels
 * @param y1 bottom right Y coordinate in pixels
 * @see [rect]
 */
void meg4_api_frect(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    uint8_t *d;
    int i, x, xs, xe, y, ys, ye, r, g, b;
    if(c[3] && x0 < x1 && y0 < y1 && x0 < le16toh(meg4.mmio.cropx1) && x1 > le16toh(meg4.mmio.cropx0) &&
      y0 < le16toh(meg4.mmio.cropy1) && y1 > le16toh(meg4.mmio.cropy0)) {
        xs = x0 < le16toh(meg4.mmio.cropx0) ? le16toh(meg4.mmio.cropx0) : x0;
        xe = x1 < le16toh(meg4.mmio.cropx1) ? x1 : le16toh(meg4.mmio.cropx1);
        ys = y0 < le16toh(meg4.mmio.cropy0) ? le16toh(meg4.mmio.cropy0) : y0;
        ye = y1 < le16toh(meg4.mmio.cropy1) ? y1 : le16toh(meg4.mmio.cropy1);
        i = 255 - c[3]; r = c[0]*c[3]; g = c[1]*c[3]; b = c[2]*c[3];
        for(y = ys; y <= ye; y++) {
            d = (uint8_t*)&meg4.vram[y * 640 + xs];
            for(x = xs; x <= xe; x++, d += 4) { d[2] = (b + i*d[2]) >> 8; d[1] = (g + i*d[1]) >> 8; d[0] = (r + i*d[0]) >> 8; }
        }
    }
}

/**
 * Draws a circle.
 * @param palidx color, palette index 0 to 255
 * @param x center X coordinate in pixels
 * @param y center Y coordinate in pixels
 * @param r radius in pixels
 * @see [fcirc], [ellip], [fellip]
 */
void meg4_api_circ(uint8_t palidx, int16_t x, int16_t y, uint16_t r)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    int x1 = -r, y1 = 0, a, x2, e2, err = 2-2*r;
    if(r > 0) {
        r = 1-err;
        do {
            a = err-2*(x1+y1)-2; if(a < 0) a = -a;
            a = 255 * a / r; a = (255 - a) * c[3] / 255;
            meg4_setpixel(x - x1, y + y1, c[0], c[1], c[2], a);
            meg4_setpixel(x - y1, y - x1, c[0], c[1], c[2], a);
            meg4_setpixel(x + x1, y - y1, c[0], c[1], c[2], a);
            meg4_setpixel(x + y1, y + x1, c[0], c[1], c[2], a);
            e2 = err; x2 = x1;
            if(err+y1 > 0) {
                a = 255*(err-2*x1-1)/r; a = (255 - a) * c[3] / 255;
                if(a > 0) {
                    meg4_setpixel(x - x1, y + y1 + 1, c[0], c[1], c[2], a);
                    meg4_setpixel(x - y1 - 1, y - x1, c[0], c[1], c[2], a);
                    meg4_setpixel(x + x1, y - y1 - 1, c[0], c[1], c[2], a);
                    meg4_setpixel(x + y1 + 1, y + x1, c[0], c[1], c[2], a);
                }
                err += ++x1*2+1;
            }
            if(e2+x2 <= 0) {
                a = 255*(2*y1+3-e2)/r; a = (255 - a) * c[3] / 255;
                if(a > 0) {
                    meg4_setpixel(x - x2 - 1, y + y1, c[0], c[1], c[2], a);
                    meg4_setpixel(x - y1, y - x2 - 1, c[0], c[1], c[2], a);
                    meg4_setpixel(x + x2 + 1, y - y1, c[0], c[1], c[2], a);
                    meg4_setpixel(x + y1, y + x2 + 1, c[0], c[1], c[2], a);
                }
                err += ++y1*2+1;
            }
        } while(x1 < 0);
    } else
    meg4_setpixel(x, y, c[0], c[1], c[2], c[3]);
}

/**
 * Draws a filled circle.
 * @param palidx color, palette index 0 to 255
 * @param x center X coordinate in pixels
 * @param y center Y coordinate in pixels
 * @param r radius in pixels
 * @see [circ], [ellip], [fellip]
 */
void meg4_api_fcirc(uint8_t palidx, int16_t x, int16_t y, uint16_t r)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    int x1 = -r, y1 = 0, a, x2, e2, err = 2-2*r;
    if(r < 2)
        meg4_setpixel(x, y, c[0], c[1], c[2], c[3]);
    if(r > 0) {
        r = 1-err;
        do {
            a = err-2*(x1+y1)-2; if(a < 0) a = -a;
            a = 255 * a / r; a = (255 - a) * c[3] / 255;
            meg4_setpixel(x - x1, y + y1, c[0], c[1], c[2], a);
            meg4_setpixel(x - y1, y - x1, c[0], c[1], c[2], a);
            meg4_setpixel(x + x1, y - y1, c[0], c[1], c[2], a);
            meg4_setpixel(x + y1, y + x1, c[0], c[1], c[2], a);
            if(x1) {
                meg4_fill(x + x1 + 1, x - x1 - 1, y + y1, c);
                meg4_fill(x + x1 + 1, x - x1 - 1, y - y1, c);
            }
            e2 = err; x2 = x1;
            if(err+y1 > 0) {
                a = 255*(err-2*x1-1)/r; a = (255 - a) * c[3] / 255;
                if(a > 0) {
                    meg4_setpixel(x - x1, y + y1 + 1, c[0], c[1], c[2], a);
                    meg4_setpixel(x - y1 - 1, y - x1, c[0], c[1], c[2], a);
                    meg4_setpixel(x + x1, y - y1 - 1, c[0], c[1], c[2], a);
                    meg4_setpixel(x + y1 + 1, y + x1, c[0], c[1], c[2], a);
                }
                err += ++x1*2+1;
            }
            if(e2+x2 <= 0) err += ++y1*2+1;
        } while(x1 < 0);
    }
}

/**
 * Draws an ellipse.
 * @param palidx color, palette index 0 to 255
 * @param x0 top left corner X coordinate in pixels
 * @param y0 top left corner Y coordinate in pixels
 * @param x1 bottom right X coordinate in pixels
 * @param y1 bottom right Y coordinate in pixels
 * @see [circ], [fcirc], [fellip]
 */
void meg4_api_ellip(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    int a = x0 > x1 ? x0 - x1 : x1 - x0, b = y0 > y1 ? y0 - y1 : y1 - y0, b1 = b&1, f, A;
    float dx = 4*(a-1.0)*b*b, dy = 4*(b1+1)*a*a;
    float ed, i, err = b1*a*a-dx+dy;

    if(!c[3]) return;
    if(a == 0 || b == 0) { meg4_api_line(palidx, x0,y0, x1,y1); return; }
    if(x0 > x1) { x0 = x1; x1 += a; }
    if(y0 > y1) y0 = y1;
    y0 += (b+1)/2; y1 = y0-b1;
    a = 8*a*a; b1 = 8*b*b;

    while(1) {
        i = dx < dy ? dx : dy; ed = dx < dy ? dy : dx;
        if(y0 == y1+1 && err > dy && a > b1) ed = 255*4./a;
        else ed = 255/(ed+2*ed*i*i/(4*ed*ed+i*i));
        i = ed*fabsf(err+dx-dy); A = (255 - (int)i) * c[3] / 255;
        if(A > 0) {
            meg4_setpixel(x0, y0, c[0], c[1], c[2], A);
            meg4_setpixel(x0, y1, c[0], c[1], c[2], A);
            meg4_setpixel(x1, y0, c[0], c[1], c[2], A);
            meg4_setpixel(x1, y1, c[0], c[1], c[2], A);
        }
        if((f = (2*err+dy >= 0))) {
            if(x0 >= x1) break;
            i = ed*(err+dx);
            if(i < 255) {
                A = (255 - (int)i) * c[3] / 255;
                meg4_setpixel(x0, y0 + 1, c[0], c[1], c[2], A);
                meg4_setpixel(x0, y1 - 1, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y0 + 1, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y1 - 1, c[0], c[1], c[2], A);
            }
        }
        if(2*err <= dx) {
            i = ed*(dy-err);
            if(i < 255) {
                A = (255 - (int)i) * c[3] / 255;
                meg4_setpixel(x0 + 1, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x1 - 1, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x0 + 1, y1, c[0], c[1], c[2], A);
                meg4_setpixel(x1 - 1, y0, c[0], c[1], c[2], A);
            }
            y0++; y1--; err += dy += a;
        }
        if(f) { x0++; x1--; err -= dx -= b1; }
    }
    if(--x0 == x1++)
        while(y0-y1 < b) {
            i = 255*4*fabsf(err+dx)/b1; ++y0; --y1; A = (255 - (int)i) * c[3] / 255;
            if(A > 0) {
                meg4_setpixel(x0, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x0, y1, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y1, c[0], c[1], c[2], A);
            }
            err += dy += a;
        }
}

/**
 * Draws a filled ellipse.
 * @param palidx color, palette index 0 to 255
 * @param x0 top left corner X coordinate in pixels
 * @param y0 top left corner Y coordinate in pixels
 * @param x1 bottom right X coordinate in pixels
 * @param y1 bottom right Y coordinate in pixels
 * @see [circ], [fcirc], [ellip]
 */
void meg4_api_fellip(uint8_t palidx, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t *c = (uint8_t*)&meg4.mmio.palette[(int)palidx];
    int a = x0 > x1 ? x0 - x1 : x1 - x0, b = y0 > y1 ? y0 - y1 : y1 - y0, b1 = b&1, f, A;
    float dx = 4*(a-1.0)*b*b, dy = 4*(b1+1)*a*a;
    float ed, i, err = b1*a*a-dx+dy;

    if(!c[3]) return;
    if(a == 0 || b == 0) { meg4_api_line(palidx, x0,y0, x1,y1); return; }
    if(x0 > x1) { x0 = x1; x1 += a; }
    if(y0 > y1) y0 = y1;
    y0 += (b+1)/2; y1 = y0-b1;
    a = 8*a*a; b1 = 8*b*b;

    while(1) {
        i = dx < dy ? dx : dy; ed = dx < dy ? dy : dx;
        if(y0 == y1+1 && err > dy && a > b1) ed = 255*4./a;
        else ed = 255/(ed+2*ed*i*i/(4*ed*ed+i*i));
        i = ed*fabsf(err+dx-dy); A = (255 - (int)i) * c[3] / 255;
        if(A > 0) {
            meg4_setpixel(x0, y0, c[0], c[1], c[2], A);
            meg4_setpixel(x0, y1, c[0], c[1], c[2], A);
            meg4_setpixel(x1, y0, c[0], c[1], c[2], A);
            meg4_setpixel(x1, y1, c[0], c[1], c[2], A);
        }
        meg4_fill(x0 + 1, x1 - 1, y0, c);
        meg4_fill(x0 + 1, x1 - 1, y1, c);
        if((f = (2*err+dy >= 0))) {
            if(x0 >= x1) break;
            i = ed*(err+dx);
            if(i < 255) {
                A = (255 - (int)i) * c[3] / 255;
                meg4_setpixel(x0, y0 + 1, c[0], c[1], c[2], A);
                meg4_setpixel(x0, y1 - 1, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y0 + 1, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y1 - 1, c[0], c[1], c[2], A);
            }
        }
        if(2*err <= dx) {
            i = ed*(dy-err);
            if(i < 255) {
                A = (255 - (int)i) * c[3] / 255;
                meg4_setpixel(x0 + 1, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x1 - 1, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x0 + 1, y1, c[0], c[1], c[2], A);
                meg4_setpixel(x1 - 1, y0, c[0], c[1], c[2], A);
            }
            y0++; y1--; err += dy += a;
        }
        if(f) { x0++; x1--; err -= dx -= b1; }
    }
    if(--x0 == x1++)
        while(y0-y1 < b) {
            i = 255*4*fabsf(err+dx)/b1; ++y0; --y1; A = (255 - (int)i) * c[3] / 255;
            if(A > 0) {
                meg4_setpixel(x0, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x0, y1, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y0, c[0], c[1], c[2], A);
                meg4_setpixel(x1, y1, c[0], c[1], c[2], A);
            }
            err += dy += a;
        }
}

/**
 * Moves turtle to the given position on screen or in the maze.
 * @param x X coordinate in pixels (or 1/128 tiles with [maze])
 * @param y Y coordinate in pixels
 * @param deg direction in degrees, 0 to 359, 0 is upwards on screen, 90 is to the right
 * @see [left], [right], [up], [down], [color], [forw], [back]
 */
void meg4_api_move(int16_t x, int16_t y, uint16_t deg)
{
    if(meg4.mmio.turtlepen)
        meg4_api_line(meg4.mmio.turtlepalidx, le16toh(meg4.mmio.turtlex), le16toh(meg4.mmio.turtley), x, y);
    meg4.mmio.turtlex = htole16(x);
    meg4.mmio.turtley = htole16(y);
    meg4.mmio.turtlea = htole16(deg % 360);
    posX = (float)y / 128.0; posY = (float)x / 128.0;
}

/**
 * Turns turtle left.
 * @param deg change in degrees, 0 to 359
 * @see [move], [right], [up], [down], [color], [forw], [back]
 */
void meg4_api_left(uint16_t deg)
{
    deg %= 360;
    meg4.mmio.turtlea = htole16((le16toh(meg4.mmio.turtlea) < deg ? le16toh(meg4.mmio.turtlea) + 360 : le16toh(meg4.mmio.turtlea)) - deg);
}

/**
 * Turns turtle right.
 * @param deg change in degrees, 0 to 359
 * @see [move], [left], [up], [down], [color], [forw], [back]
 */
void meg4_api_right(uint16_t deg)
{
    deg %= 360;
    meg4.mmio.turtlea = htole16((le16toh(meg4.mmio.turtlea) + deg) % 360);
}

/**
 * Lifts turtle's tail up. The turtle will move without drawing a line.
 * @see [move], [left], [right], [down], [color], [forw], [back]
 */
void meg4_api_up(void)
{
    meg4.mmio.turtlepen = 0;
}

/**
 * Puts turtle's tail down. The turtle will move drawing a line (see [color]).
 * @see [move], [left], [right], [up], [color], [forw], [back]
 */
void meg4_api_down(void)
{
    meg4.mmio.turtlepen = 1;
}

/**
 * Sets turtle paint color.
 * @param palidx color, palette index 0 to 255
 * @see [move], [left], [right], [up], [down], [forw], [back]
 */
void meg4_api_color(uint8_t palidx)
{
    meg4.mmio.turtlepalidx = palidx;
}

/**
 * Moves turtle forward.
 * @param cnt amount in pixels (or 1/128 tiles with [maze])
 * @see [move], [left], [right], [up], [down], [color], [back]
 */
void meg4_api_forw(uint16_t cnt)
{
    int16_t oldx = le16toh(meg4.mmio.turtlex), oldy = le16toh(meg4.mmio.turtley);
    meg4.mmio.turtlex = htole16(le16toh(meg4.mmio.turtlex) + (int)(cnt * cosf(((le16toh(meg4.mmio.turtlea) + 270) % 360) * MEG4_PI / 180.0)));
    meg4.mmio.turtley = htole16(le16toh(meg4.mmio.turtley) + (int)(cnt * sinf(((le16toh(meg4.mmio.turtlea) + 270) % 360) * MEG4_PI / 180.0)));
    if(meg4.mmio.turtlepen)
        meg4_api_line(meg4.mmio.turtlepalidx, oldx, oldy, le16toh(meg4.mmio.turtlex), le16toh(meg4.mmio.turtley));
    posX = (float)(le16toh(meg4.mmio.turtley)) / 128.0; posY = (float)(le16toh(meg4.mmio.turtlex)) / 128.0;
}

/**
 * Moves turtle backward.
 * @param cnt amount in pixels (or 1/128 tiles with [maze])
 * @see [move], [left], [right], [up], [down], [color], [forw]
 */
void meg4_api_back(uint16_t cnt)
{
    int16_t oldx = le16toh(meg4.mmio.turtlex), oldy = le16toh(meg4.mmio.turtley);
    meg4.mmio.turtlex = htole16(le16toh(meg4.mmio.turtlex) - (int)(cnt * cosf(((le16toh(meg4.mmio.turtlea) + 270) % 360) * MEG4_PI / 180.0)));
    meg4.mmio.turtley = htole16(le16toh(meg4.mmio.turtley) - (int)(cnt * sinf(((le16toh(meg4.mmio.turtlea) + 270) % 360) * MEG4_PI / 180.0)));
    if(meg4.mmio.turtlepen)
        meg4_api_line(meg4.mmio.turtlepalidx, oldx, oldy, le16toh(meg4.mmio.turtlex), le16toh(meg4.mmio.turtley));
    posX = (float)(le16toh(meg4.mmio.turtley)) / 128.0; posY = (float)(le16toh(meg4.mmio.turtlex)) / 128.0;
}

/**
 * Displays a sprite, or multiple adjacent sprites.
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @param sprite sprite id, 0 to 1023
 * @param sw number of horizontal sprites
 * @param sh number of vertical sprites
 * @param scale scale, -3 to 4
 * @param type transform, 0=normal, 1=rotate 90, 2=rotate 180, 3=rotate 270, 4=flip vertically, 5=flip+90, 6=flip horizontally, 7=flip+270
 * @see [dlg], [stext]
 */
void meg4_api_spr(int16_t x, int16_t y, uint16_t sprite, uint8_t sw, uint8_t sh, int8_t scale, uint8_t type)
{
    int i, j, k, m, l, s, siz[] = { 1, 2, 4, 0, 8, 16, 24, 32 }, X, Y;
    if(sprite > 1023 || sw < 1 || sh < 1 || scale < -3 || scale > 4 || type > 7) return;
    s = siz[(!scale ? 1 : scale) + 3];
    m = 32 - (sprite & 31); if(sw < m) m = sw;
    for(j = 0, k = sprite, Y = y; j < sh && k < 1024; j++, k += 32, Y += s)
        for(i = 0, l = k, X = x; i < m; i++, l++, X += s)
            switch(type) {
                case 1: meg4_spr(meg4.vram, 2560, x + (sh - j - 1) * s, y + i * s, l, scale, type); break;
                case 2: meg4_spr(meg4.vram, 2560, x + (sw - i - 1) * s, y + (sh - j - 1) * s, l, scale, type); break;
                case 3: meg4_spr(meg4.vram, 2560, x + j * s, y + (sw - i - 1) * s, l, scale, type); break;
                case 4: meg4_spr(meg4.vram, 2560, X, y + (sh - j - 1) * s, l, scale, type); break;
                case 5: meg4_spr(meg4.vram, 2560, x + (sh - j - 1) * s, y + (sh - i - 1) * s, l, scale, type); break;
                case 6: meg4_spr(meg4.vram, 2560, x + (sw - i - 1) * s, Y, l, scale, type); break;
                case 7: meg4_spr(meg4.vram, 2560, x + (sh - i - 1) * s, y + (sh - j - 1) * s, l, scale, type); break;
                default: meg4_spr(meg4.vram, 2560, X, Y, l, scale, type); break;
            }
}

/**
 * Displays a dialog window using sprites.
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @param w dialog width in pixels
 * @param h dialog height in pixels
 * @param scale scale, -3 to 4
 * @param tl top left corner sprite id
 * @param tm top middle sprite id
 * @param tr top right corner sprite id
 * @param ml middle left side sprite id
 * @param bg background sprite id
 * @param mr middle right side sprite id
 * @param bl bottom left corner sprite id
 * @param bm bottom middle sprite id
 * @param br bottom right corner sprite id
 * @see [spr], [stext]
 */
void meg4_api_dlg(int16_t x, int16_t y, uint16_t w, uint16_t h, int8_t scale,
    uint16_t tl, uint16_t tm, uint16_t tr,
    uint16_t ml, uint16_t bg, uint16_t mr,
    uint16_t bl, uint16_t bm, uint16_t br)
{
    int i, j, k, l, x0, x1, y0, y1, s, siz[] = { 1, 2, 4, 0, 8, 16, 24, 32 };
    if(scale < -3 || scale > 4) return;
    s = siz[(!scale ? 1 : scale) + 3];
    if(x + s >= le16toh(meg4.mmio.cropx1) || y + s >= le16toh(meg4.mmio.cropy1) || w > 640 - 2 * s || h > 400 - 2 * s ||
      x + w + s < le16toh(meg4.mmio.cropx0) || y + h + s < le16toh(meg4.mmio.cropy0)) return;
    k = x + w; if(k > le16toh(meg4.mmio.cropx1)) k = le16toh(meg4.mmio.cropx1);
    l = y + h; if(l > le16toh(meg4.mmio.cropy1)) l = le16toh(meg4.mmio.cropy1);
    x0 = le16toh(meg4.mmio.cropx0); x1 = le16toh(meg4.mmio.cropx1); y0 = le16toh(meg4.mmio.cropy0); y1 = le16toh(meg4.mmio.cropy1);
    if(y - s > y0) {
        meg4_spr(meg4.vram, 2560, x - s, y - s, tl, scale, 0);
        meg4.mmio.cropx1 = htole16(k);
        for(i = 0; i < w; i += s) meg4_spr(meg4.vram, 2560, x + i, y - s, tm, scale, 0);
        meg4.mmio.cropx1 = htole16(x1);
        meg4_spr(meg4.vram, 2560, x + w, y - s, tr, scale, 0);
    }
    meg4.mmio.cropy1 = htole16(l);
    for(j = 0; j < h; j += s) {
        meg4_spr(meg4.vram, 2560, x - s, y + j, ml, scale, 0);
        meg4.mmio.cropx1 = htole16(k);
        for(i = 0; i < w; i += s) meg4_spr(meg4.vram, 2560, x + i, y + j, bg, scale, 0);
        meg4.mmio.cropx1 = htole16(x1);
        meg4_spr(meg4.vram, 2560, x + w, y + j, mr, scale, 0);
    }
    if(y + h < y1) {
        meg4.mmio.cropy1 = htole16(y1);
        meg4_spr(meg4.vram, 2560, x - s, y + h, bl, scale, 0);
        meg4.mmio.cropx1 = htole16(k);
        for(i = 0; i < w; i += s) meg4_spr(meg4.vram, 2560, x + i, y + h, bm, scale, 0);
        meg4.mmio.cropx1 = htole16(x1);
        meg4_spr(meg4.vram, 2560, x + w, y + h, br, scale, 0);
    }
    meg4.mmio.cropx0 = htole16(x0); meg4.mmio.cropx1 = htole16(x1); meg4.mmio.cropy0 = htole16(y0); meg4.mmio.cropy1 = htole16(y1);
}

/**
 * Displays text on screen using sprites.
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @param fs first sprite id to be used for displaying
 * @param fu first UNICODE (smallest character) in string
 * @param sw number of horizontal sprites
 * @param sh number of vertical sprites
 * @param scale scale, -3 to 4
 * @param str zero terminated UTF-8 string
 * @see [spr], [dlg]
 */
void meg4_api_stext(int16_t x, int16_t y, uint16_t fs, uint16_t fu, uint8_t sw, uint8_t sh, int8_t scale, str_t str)
{
    uint32_t c;
    char *end, *t;
    int l, s, siz[] = { 1, 2, 4, 0, 8, 16, 24, 32 };

    if(x >= le16toh(meg4.mmio.cropx1) || y >= le16toh(meg4.mmio.cropy1) || fs > 1023 || sw < 1 || sh < 1 || scale < -3 || scale > 4 ||
      str < MEG4_MEM_USER || str >= MEG4_MEM_LIMIT) return;
    t = (char*)meg4.data + str - MEG4_MEM_USER;
    if(!*t) return;
    s = siz[(!scale ? 1 : scale) + 3];
    /* force a maximum number of characters to display just in case */
    l = strlen(t); if(l > 256) { l = 256; } end = t + l;
    if((uintptr_t)t >= (uintptr_t)&meg4.data && (uintptr_t)t < (uintptr_t)&meg4.data + sizeof(meg4.data) &&
      end > (char*)meg4.data + sizeof(meg4.data) - 1)
        end = (char*)meg4.data + sizeof(meg4.data) - 1;
    for(l = x; t < end && *t; x += s * sw) {
        t = meg4_utf8(t, &c);
        if(c == '\r') { x = l; continue; }
        if(c == '\n') { x = l; y += s * sh; continue; }
        if(c < ' ' || c < fu) break;
        /* TODO: take into account sprite height (sh) too */
        c -= fu; c *= sw;
        if(y >= le16toh(meg4.mmio.cropy1) || fs + c > 1023) break;
        meg4_api_spr(x, y, fs + c, sw, sh, scale, 0);
    }
}

/**
 * Replaces tiles on map. Can be used to animate tiles on the map.
 * @param replace an array of 256 sprite ids
 * @see [mget], [mset], [map], [maze]
 */
void meg4_api_remap(addr_t replace)
{
    if(replace < MEG4_MEM_USER || replace >= MEG4_MEM_LIMIT - 256) return;
    meg4_remap(meg4.data + replace - MEG4_MEM_USER);
}

/**
 * Returns one tile on map.
 * @param mx X coordinate on map in tiles
 * @param my Y coordinate on map in tiles
 * @return The sprite id of the tile at the given coordinate.
 * @see [remap], [mset], [map], [maze]
 */
uint16_t meg4_api_mget(uint16_t mx, uint16_t my)
{
    return (mx >= 320 || my >= 200) ? 0 : le16toh((meg4.mmio.mapsel << 8) | meg4.mmio.map[my * 320 + mx]);
}

/**
 * Sets one tile on map.
 * @param mx X coordinate on map in tiles
 * @param my Y coordinate on map in tiles
 * @param sprite sprite id, 0 to 1023
 * @see [remap], [mget], [map], [maze]
 */
void meg4_api_mset(uint16_t mx, uint16_t my, uint16_t sprite)
{
    uint16_t val = htole16(sprite);
    if(mx >= 320 || my >= 200 || sprite >= 0x3ff) return;
    meg4.mmio.mapsel = val >> 8;
    meg4.mmio.map[my * 320 + mx] = val & 0xff;
}

/**
 * Draws (part of) the map.
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @param mx X coordinate on map in tiles
 * @param my Y coordinate on map in tiles
 * @param mw number of horizontal tiles
 * @param mh number of vertical tiles
 * @param scale scale, -3 to 4
 * @see [remap], [mget], [mset], [maze]
 */
void meg4_api_map(int16_t x, int16_t y, uint16_t mx, uint16_t my, uint16_t mw, uint16_t mh, int8_t scale)
{
    int i, j, k, l, s, siz[] = { 1, 2, 4, 0, 8, 16, 24, 32 };
    if(x >= le16toh(meg4.mmio.cropx1) || y >= le16toh(meg4.mmio.cropy1) || scale < -3 || scale > 4) return;
    if(mw < 1) mw = 320;
    if(mh < 1) mh = 200;
    if(mx + mw > 320) mw = 320; else mw += mx;
    if(my + mh > 200) mh = 200; else mh += my;
    s = siz[(!scale ? 1 : scale) + 3];
    for(j = my, k = my * 320; j < mh && y < le16toh(meg4.mmio.cropy1); j++, y += s, k += 320)
        if(y + s > le16toh(meg4.mmio.cropy0))
            for(i = mx, l = x; i < mw; i++, l += s)
                if(l + s > le16toh(meg4.mmio.cropx0) && l < le16toh(meg4.mmio.cropx1) && meg4.mmio.map[k + i])
                    meg4_spr(meg4.vram, 2560, l, y, le16toh((meg4.mmio.mapsel << 8) | meg4.mmio.map[k + i]), scale, 0);
}

/**
 * Displays map as a 3D maze, using turtle's position.
 * @param mx X coordinate on map in tiles
 * @param my Y coordinate on map in tiles
 * @param mw number of horizontal tiles
 * @param mh number of vertical tiles
 * @param scale number of sprites per tiles in power of two, 0 to 3
 * @param sky sky tile index
 * @param grd ground tile index
 * @param door first door tile index
 * @param wall first wall tile index
 * @param obj first object tile index
 * @param numnpc number of NPC records
 * @param npc an uint32_t array of numnpc times x,y,tile index triplets
 * @see [remap], [mget], [mset], [map]
 */
void meg4_api_maze(uint16_t mx, uint16_t my, uint16_t mw, uint16_t mh, uint8_t scale,
    uint16_t sky, uint16_t grd, uint16_t door, uint16_t wall, uint16_t obj, uint8_t numnpc, addr_t npc)
{
#define SPRMAX 512
    float dirX = -1.0, dirY = 0.0, planeX = 0.0, planeY = 0.66;
    float rd, dX, dY, fX, fY, sX, sY, rX, rY, zb[640];
    int x0 = le16toh(meg4.mmio.cropx0), x1 = le16toh(meg4.mmio.cropx1), y0 = le16toh(meg4.mmio.cropy0), y1 = le16toh(meg4.mmio.cropy1);
    int i, j, s, e, l, x, y, z, n, p, w = meg4.screen.w, h = meg4.screen.h, hw = w / 2, hh = h / 2, cx, cy, tx, ty, ts, tw, tn, tb, ti, si;
    maze_spr_t spr[SPRMAX];
    uint32_t *inp, c;
    uint8_t *a, *b;

    /* this code originally from Lode's raycasting tutorial, but heavily rewritten an optimized. Unfortunately Lode fucked up
     * the coordinates and I'm too lazy to fix all his calculations. Quick'n'dirty workaround: transpoze the map instead */

    if(mx >= 320 || my >= 200 || scale > 3) return;
    if(mx + mw > 320) mw = 320 - mx;
    if(my + mh > 200) mh = 200 - my;
    if(mw < 1 || mh < 1) return;
    if(wall < 1) wall = 1;
    if(!door || door > wall) door = wall;
    if(obj < wall) obj = 1024;
    ts = 1 << (scale + 3); tw = 256 / ts; tn = tw * tw; if(tn > 256) tn = 256;
    if(sky >= tn) sky = 0;
    if(grd >= tn) grd = 0;
    tb = scale ? 0 : (meg4.mmio.mapsel & 1) * 256 * 64;
    si = ((sky >> (5 - scale)) << (scale + 11)) + ((sky & (tw - 1)) << (scale + 3)) + tb;

    /* failsafe, make sure turtle is on the map, on a walkable tile */
    if(le16toh(meg4.mmio.turtlex) >= ((mw + 1) << 7) || le16toh(meg4.mmio.turtley) >= ((mh + 1) << 7) ||
      meg4.mmio.map[(my + (le16toh(meg4.mmio.turtley) >> 7)) * 320 + mx + (le16toh(meg4.mmio.turtlex) >> 7)] >= wall) {
        meg4.mmio.turtlex = meg4.mmio.turtley = 64; posX = posY = 0.5;
        /* not checking the borders, because mazes are often surrounded by walls */
        for(y = 1; y < mh - 1; y++)
            for(x = 1; x < mw - 1; x++)
                if(meg4.mmio.map[(my + y) * 320 + mx + x] < door) {
                    meg4.mmio.turtlex = htole16((x << 7) + 64);
                    meg4.mmio.turtley = htole16((y << 7) + 64);
                    posX = (float)(le16toh(meg4.mmio.turtley)) / 128.0;
                    posY = (float)(le16toh(meg4.mmio.turtlex)) / 128.0;
                    x = mw; y = mh; break;
                }
    }
    fY = meg4_api_cos(le16toh(meg4.mmio.turtlea + 180)); fX = meg4_api_sin(le16toh(meg4.mmio.turtlea + 180));
    dX = dirX; dirX = dirX * fX - dirY * fY; dirY = dX * fY + dirY * fX;
    dX = planeX; planeX = planeX * fX - planeY * fY; planeY = dX * fY + planeY * fX;

    /* ceiling and floor */
    dX = ((dirX + planeX) - (dirX - planeX)) / (float)w;
    dY = ((dirY + planeY) - (dirY - planeY)) / (float)w;
    for(y = hh, p = hh * 640, l = p - 640; y < h; y++, p += 640, l -= 640) {
        rd = (float)hh / (float)(y - hh + 1);
        sX = rd * dX; sY = rd * dY;
        fX = posX + rd * (dirX - planeX) + (float)x0 * sX;
        fY = posY + rd * (dirY - planeY) + (float)x0 * sY;
        for(x = x0; x < x1; x++, fX += sX, fY += sY) {
            cx = (int)(fX < 0.0 ? -1.0 : fX); cy = (int)(fY < 0.0 ? -1.0 : fY);
            z = h - y - 1; i = (((int)(ts * (fY - (float)cy)) & (ts - 1)) << 8) + ((int)(ts * (fX - (float)cx)) & (ts - 1));
            if(y >= y0 && y < y1) {
                j = cx >= 0 && cx < mh && cy >= 0 && cy < mw ? meg4.mmio.map[(my + cx) * 320 + cy + mx] : grd;
                if(j < 1 || j >= obj || (j >= door && j < wall)) j = grd;
                if(j > 0 && j < (int)wall) {
                    ti = ((j >> (5 - scale)) << (scale + 11)) + ((j & (tw - 1)) << (scale + 3)) + tb;
                    meg4.screen.buf[p + x] = meg4.mmio.palette[meg4.mmio.sprites[i + ti]];
                }
            }
            if(sky && z >= y0 && z < y1)
                meg4.screen.buf[l + x] = meg4.mmio.palette[meg4.mmio.sprites[i + si]];
        }
    }

    /* walls */
    for(x = x0; x < x1; x++) {
        cx = (int)posX; cy = (int)posY;
        rd = (float)(x << 1) / (float)w - 1; rX = dirX + rd * planeX; rY = dirY + rd * planeY;
        dX = (rX == 0.0 || rX == -0.0) ? 65536.0 : fabsf(1 / rX);
        dY = (rY == 0.0 || rY == -0.0) ? 65536.0 : fabsf(1 / rY);
        if(rX < 0.0) { tx = -1; sX = (posX - (float)cx) * dX; } else { tx = 1; sX = ((float)cx + 1.0 - posX) * dX; }
        if(rY < 0.0) { ty = -1; sY = (posY - (float)cy) * dY; } else { ty = 1; sY = ((float)cy + 1.0 - posY) * dY; }
        i = cx >= 0 && cx < mh && cy >= 0 && cy < mw ? meg4.mmio.map[(cx + my) * 320 + cy + mx] : 0;
        n = (i >= (int)door && i < (int)wall) ? wall : door;
        j = sX >= sY ? 1 : 0; zb[x] = 65536.0;
        while(cx >= 0 && cx < mh && cy >= 0 && cy < mw) {
            i = meg4.mmio.map[(cx + my) * 320 + cy + mx];
            if(i >= n && i < obj) {
                zb[x] = rd = j ? sY - dY : sX - dX;
                if(i >= tn) break;
                l = (int)((float)h / rd) >> 1;
                s = -l + hh; if(s < y0) s = y0;
                e = l + hh;  if(e >= y1) e = y1 - 1;
                ti = ((i >> (5 - scale)) << (scale + 11)) + ((i & (tw - 1)) << (scale + 3)) + tb;
                fX = j ? posX + rd * rX : posY + rd * rY; fX -= (int)fX;
                tx = (int)(fX * (float)ts); if((!j && rX > 0.0) || (j && rY < 0.0)) tx = ts - tx - 1;
                n = l << 1;
                for(y = s, p = s * 640; y < e; y++, p += 640) {
                    ty = (((y - hh + l) << (scale + 11)) / n) & 0xffffff00;
                    c = meg4.mmio.palette[meg4.mmio.sprites[ty + tx + ti]]; if(j) { c >>= 1; c &= 0x7F7F7F; }
                    meg4.screen.buf[p + x] = c;
                }
                break;
            }
            if(sX < sY) { sX += dX; cx += tx; j = 0; } else { sY += dY; cy += ty; j = 1; }
            n = door;
        }
    }

    /* sprites */
    n = 0; inp = NULL;
    /* user specified sprites (NPCs and walkable objects) */
    if(numnpc && npc >= MEG4_MEM_USER && npc < MEG4_MEM_LIMIT - 256) {
        if(npc + numnpc * 12 >= MEG4_MEM_LIMIT - 256) numnpc = (MEG4_MEM_LIMIT - 256 - npc) / 12;
        inp = (uint32_t*)(meg4.data + npc - MEG4_MEM_USER);
        s = (mw + 1) << 7; e = (mh + 1) << 7;
        for(i = j = 0; i < (int)numnpc; i++, j += 3) {
            inp[j + 2] &= htole32(0x0fffffff); l = le32toh(inp[j + 2]) & 0x3fff;
            if(le32toh(inp[j + 0]) < (uint32_t)s && le32toh(inp[j + 1]) < (uint32_t)e && l && l >= obj && l < tn) {
                fX = (float)le32toh(inp[j + 1]) / 128.0; fY = (float)le32toh(inp[j + 0]) / 128.0;
                spr[n].x = fX; fX = posX - fX;
                spr[n].y = fY; fY = posY - fY;
                spr[n].d = z = (int)(fX * fX + fY * fY);
                spr[n].o = i;
                if(z < 576) spr[n++].id = l;
            }
        }
    }
    /* add non-walkable object sprites around the player from map */
    if(obj < tn) {
        cx = (int)posX - 24; if(cx < 0) cx = 0;
        tx = (int)posX + 24; if(tx > mw) tx = mw;
        cy = (int)posY - 24; if(cy < 0) cy = 0;
        ty = (int)posY + 24; if(ty > mw) ty = mw;
        for(y = cy; y < ty && n < SPRMAX; y++)
            for(x = cx; x < ty && n < SPRMAX; x++) {
                j = meg4.mmio.map[(my + y) * 320 + mx + x];
                if(j && j >= obj && j < tn) {
                    fX = (float)y + 0.5; fY = (float)x + 0.5;
                    spr[n].x = fX; fX = posX - fX;
                    spr[n].y = fY; fY = posY - fY;
                    spr[n].d = z = (int)(fX * fX + fY * fY);
                    spr[n].o = 0xffff;
                    if(z < 576) spr[n++].id = j;
                }
            }
    }
    if(n > 0) {
        qsort(spr, n, sizeof(maze_spr_t), mazecmp);
        rd = 1.0 / (planeX * dirY - dirX * planeY);
        for(i = 0; i < n; i++) {
            ti = ((spr[i].id >> (5 - scale)) << (scale + 11)) + ((spr[i].id & (tw - 1)) << (scale + 3)) + tb;
            fX = spr[i].x - posX; fY = spr[i].y - posY;
            rX = rd * (dirY * fX - dirX * fY); rY = rd * (-planeY * fX + planeX * fY);
            if(rY > 0) {
                z = (int)((float)hw * (1 + rX / rY)); j = (int)((float)h / rY); if(j < 0) { j = -j; } l = j >> 1;
                s = -l + hh; if(s < y0) s = y0;
                e = l + hh;  if(e >= y1) e = y1 - 1;
                cy = l + z; if(cy >= x1) cy = x1 - 1;
                cx = -l + z;
                for(x = cx < x0 ? x0 : cx; x < cy; x++) {
                    tx = (((x - cx) << (scale + 11)) / j) >> 8;
                    if(rY < zb[x]) {
                        /* set the MSB of tile id in array if the NPC can see the player and they are close */
                        if(x == z && inp && spr[i].o != 0xffff) {
                            y = spr[i].o * 3 + 2;
                            if(spr[i].d < 64) inp[y] |= htole32(0x80000000); /* distance is less than 8 tiles */
                            if(spr[i].d < 16) inp[y] |= htole32(0x40000000); /* distance is less than 4 tiles */
                            if(spr[i].d <  4) inp[y] |= htole32(0x20000000); /* distance is less than 2 tiles */
                            if(spr[i].d <  2) inp[y] |= htole32(0x10000000); /* they are on the same or neightbouring tiles */
                        }
                        for(y = s, p = s * 640; y < e; y++, p += 640) {
                            ty = (((y - hh + l)  << (scale + 11)) / j) & 0xffffff00;
                            a = (uint8_t*)&meg4.screen.buf[p + x]; b = (uint8_t*)&meg4.mmio.palette[meg4.mmio.sprites[ty + tx + ti]];
                            if(b[3] > 0) { si = 255 - b[3]; a[0] = (a[0]*si + b[0]*b[3]) >> 8; a[1] = (a[1]*si + b[1]*b[3]) >> 8; a[2] = (a[2]*si + b[2]*b[3]) >> 8; }
                        }
                    }
                }
            }
        }
    }

    /* navigate in the maze */
    if(meg4_api_getpad(0, MEG4_BTN_U) || meg4_api_getpad(0, MEG4_BTN_D)) {
        rd = (le32toh(meg4.mmio.tick) - meg4_lasttick) / 1000.0 * (float)(le16toh(meg4.mmio.mazew)) * (meg4_api_getpad(0, MEG4_BTN_D) ? -1 : 1);
        fX = dirX * rd; fY = dirY * rd; cx = (int)posX; x = (int)(posX + fX); cy = (int)posY; y = (int)(posY + fY);
        /* diagonals */
        if( (x < cx && y < cy && x >= 0 && y >= 0 && meg4.mmio.map[(x + my) * 320 + y + mx] < wall &&
              (meg4.mmio.map[(cx + my) * 320 + y + mx] < wall || meg4.mmio.map[(x + my) * 320 + cy + mx] < wall)) ||
            (x > cx && y < cy && x < mh && y >= 0 && meg4.mmio.map[(x + my) * 320 + y + mx] < wall &&
              (meg4.mmio.map[(cx + my) * 320 + y + mx] < wall || meg4.mmio.map[(x + my) * 320 + cy + mx] < wall)) ||
            (x < cx && y > cy && x >= 0 && y < mw && meg4.mmio.map[(x + my) * 320 + y + mx] < wall &&
              (meg4.mmio.map[(cx + my) * 320 + y + mx] < wall || meg4.mmio.map[(x + my) * 320 + cy + mx] < wall)) ||
            (x > cx && y > cy && x < mh && y < mw && meg4.mmio.map[(x + my) * 320 + y + mx] < wall &&
              (meg4.mmio.map[(cx + my) * 320 + y + mx] < wall || meg4.mmio.map[(x + my) * 320 + cy + mx] < wall))) {
                posX += fX; posY += fY;
        } else
        /* allow "sliding" along the walls, but don't allow to go too close to the wall */
        if(x >= 0 && x < mh && y >= 0 && y < mw) {
            if(meg4.mmio.map[(cx + my) * 320 + y + mx] < wall) posY += fY;
            if(meg4.mmio.map[(x + my) * 320 + cy + mx] < wall) posX += fX;
            rX = 0.25 + (cx > 0 && meg4.mmio.map[(cx - 1 + my) * 320 + cy + mx] >= wall ? (float)cx : 0.0);
            rY = 0.75 + (cx < mh - 1 && meg4.mmio.map[(cx + 1 + my) * 320 + cy + mx] >= wall ? (float)cx : (float)mh - 1.9);
            if(posX < rX) posX = rX;
            if(posX >= rY) posX = rY;
            rX = 0.25 + (cy > 0 && meg4.mmio.map[(cx + my) * 320 + cy - 1 + mx] >= wall ? (float)cy : 0.0);
            rY = 0.75 + (cy < mw - 1 && meg4.mmio.map[(cx + my) * 320 + cy + 1 + mx] >= wall ? (float)cy : (float)mw - 1.9);
            if(posY < rX) posY = rX;
            if(posY >= rY) posY = rY;
        }
        /* failsafe, should not needed, only save the new position if it's on a walkable tile */
        x = (int)posX; y = (int)posY;
        if(x >= 0 && x < mh && y >= 0 && y < mw && meg4.mmio.map[(x + my) * 320 + y + mx] < wall) {
            meg4.mmio.turtlex = htole16((int)(posY * 128.0));
            meg4.mmio.turtley = htole16((int)(posX * 128.0));
        } else {
            /* set position back where it was before the movement */
            posX = (float)(le16toh(meg4.mmio.turtley)) / 128.0;
            posY = (float)(le16toh(meg4.mmio.turtlex)) / 128.0;
        }
    }
    /* rotations */
    i = le16toh(meg4.mmio.mazer); if(i < 1 || i > 90) { i = 1; meg4.mmio.mazer = htole16(1); }
    if(meg4_api_getpad(0, MEG4_BTN_L)) meg4_api_left(i); else
    if(meg4_api_getpad(0, MEG4_BTN_R)) meg4_api_right(i);
}
