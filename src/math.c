/*
 * meg4/math.c
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
 * @brief Math functions
 * @chapter Mathematics
 *
 */

#include "meg4.h"
#include <math.h>
/* no one should trust glibc's feature defines */
float floorf(float);
float ceilf(float);
float fabsf(float);
float expf(float);
float logf(float);
float powf(float, float);
float sqrtf(float);
float cosf(float);
float sinf(float);
float tanf(float);
float acosf(float);
float asinf(float);
float atanf(float);
float atan2f(float, float);

/**
 * Normalize a vec3
 */
void meg4_normv3(float *a)
{
    union { float f; uint32_t i; } u;
    float v = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
    u.f = v; u.i = 0x5F1FFFF9 - (u.i >> 1);
    v = u.f * 0.703952253f * (2.38924456f - v * u.f * u.f);
    a[0] *= v; a[1] *= v; a[2] *= v;
}

/**
 * Matrix4x4 multiplication
 */
void meg4_mulm4(float *ret, float *a, float *b)
{
    ret[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
    ret[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
    ret[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
    ret[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];
    ret[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
    ret[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
    ret[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
    ret[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];
    ret[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
    ret[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
    ret[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
    ret[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];
    ret[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
    ret[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
    ret[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
    ret[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

/**
 * Calculate inverse matrix
 */
void meg4_invm4(float *ret, float *m)
{
    float det =
          m[ 0]*m[ 5]*m[10]*m[15] - m[ 0]*m[ 5]*m[11]*m[14] + m[ 0]*m[ 6]*m[11]*m[13] - m[ 0]*m[ 6]*m[ 9]*m[15]
        + m[ 0]*m[ 7]*m[ 9]*m[14] - m[ 0]*m[ 7]*m[10]*m[13] - m[ 1]*m[ 6]*m[11]*m[12] + m[ 1]*m[ 6]*m[ 8]*m[15]
        - m[ 1]*m[ 7]*m[ 8]*m[14] + m[ 1]*m[ 7]*m[10]*m[12] - m[ 1]*m[ 4]*m[10]*m[15] + m[ 1]*m[ 4]*m[11]*m[14]
        + m[ 2]*m[ 7]*m[ 8]*m[13] - m[ 2]*m[ 7]*m[ 9]*m[12] + m[ 2]*m[ 4]*m[ 9]*m[15] - m[ 2]*m[ 4]*m[11]*m[13]
        + m[ 2]*m[ 5]*m[11]*m[12] - m[ 2]*m[ 5]*m[ 8]*m[15] - m[ 3]*m[ 4]*m[ 9]*m[14] + m[ 3]*m[ 4]*m[10]*m[13]
        - m[ 3]*m[ 5]*m[10]*m[12] + m[ 3]*m[ 5]*m[ 8]*m[14] - m[ 3]*m[ 6]*m[ 8]*m[13] + m[ 3]*m[ 6]*m[ 9]*m[12];
    if(det == 0.0 || det == -0.0) det = 1.0; else det = 1.0 / det;
    ret[ 0] = det *(m[ 5]*(m[10]*m[15] - m[11]*m[14]) + m[ 6]*(m[11]*m[13] - m[ 9]*m[15]) + m[ 7]*(m[ 9]*m[14] - m[10]*m[13]));
    ret[ 1] = -det*(m[ 1]*(m[10]*m[15] - m[11]*m[14]) + m[ 2]*(m[11]*m[13] - m[ 9]*m[15]) + m[ 3]*(m[ 9]*m[14] - m[10]*m[13]));
    ret[ 2] = det *(m[ 1]*(m[ 6]*m[15] - m[ 7]*m[14]) + m[ 2]*(m[ 7]*m[13] - m[ 5]*m[15]) + m[ 3]*(m[ 5]*m[14] - m[ 6]*m[13]));
    ret[ 3] = -det*(m[ 1]*(m[ 6]*m[11] - m[ 7]*m[10]) + m[ 2]*(m[ 7]*m[ 9] - m[ 5]*m[11]) + m[ 3]*(m[ 5]*m[10] - m[ 6]*m[ 9]));
    ret[ 4] = -det*(m[ 4]*(m[10]*m[15] - m[11]*m[14]) + m[ 6]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 7]*(m[ 8]*m[14] - m[10]*m[12]));
    ret[ 5] = det *(m[ 0]*(m[10]*m[15] - m[11]*m[14]) + m[ 2]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 3]*(m[ 8]*m[14] - m[10]*m[12]));
    ret[ 6] = -det*(m[ 0]*(m[ 6]*m[15] - m[ 7]*m[14]) + m[ 2]*(m[ 7]*m[12] - m[ 4]*m[15]) + m[ 3]*(m[ 4]*m[14] - m[ 6]*m[12]));
    ret[ 7] = det *(m[ 0]*(m[ 6]*m[11] - m[ 7]*m[10]) + m[ 2]*(m[ 7]*m[ 8] - m[ 4]*m[11]) + m[ 3]*(m[ 4]*m[10] - m[ 6]*m[ 8]));
    ret[ 8] = det *(m[ 4]*(m[ 9]*m[15] - m[11]*m[13]) + m[ 5]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 7]*(m[ 8]*m[13] - m[ 9]*m[12]));
    ret[ 9] = -det*(m[ 0]*(m[ 9]*m[15] - m[11]*m[13]) + m[ 1]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 3]*(m[ 8]*m[13] - m[ 9]*m[12]));
    ret[10] = det *(m[ 0]*(m[ 5]*m[15] - m[ 7]*m[13]) + m[ 1]*(m[ 7]*m[12] - m[ 4]*m[15]) + m[ 3]*(m[ 4]*m[13] - m[ 5]*m[12]));
    ret[11] = -det*(m[ 0]*(m[ 5]*m[11] - m[ 7]*m[ 9]) + m[ 1]*(m[ 7]*m[ 8] - m[ 4]*m[11]) + m[ 3]*(m[ 4]*m[ 9] - m[ 5]*m[ 8]));
    ret[12] = -det*(m[ 4]*(m[ 9]*m[14] - m[10]*m[13]) + m[ 5]*(m[10]*m[12] - m[ 8]*m[14]) + m[ 6]*(m[ 8]*m[13] - m[ 9]*m[12]));
    ret[13] = det *(m[ 0]*(m[ 9]*m[14] - m[10]*m[13]) + m[ 1]*(m[10]*m[12] - m[ 8]*m[14]) + m[ 2]*(m[ 8]*m[13] - m[ 9]*m[12]));
    ret[14] = -det*(m[ 0]*(m[ 5]*m[14] - m[ 6]*m[13]) + m[ 1]*(m[ 6]*m[12] - m[ 4]*m[14]) + m[ 2]*(m[ 4]*m[13] - m[ 5]*m[12]));
    ret[15] = det *(m[ 0]*(m[ 5]*m[10] - m[ 6]*m[ 9]) + m[ 1]*(m[ 6]*m[ 8] - m[ 4]*m[10]) + m[ 2]*(m[ 4]*m[ 9] - m[ 5]*m[ 8]));
}

/**
 * Transpose matrix
 */
void meg4_trpm4(float *ret, float *m) {
    ret[0] = m[0]; ret[1] = m[4]; ret[2] = m[8]; ret[3] = m[12];
    ret[4] = m[1]; ret[5] = m[5]; ret[6] = m[9]; ret[7] = m[13];
    ret[8] = m[2]; ret[9] = m[6]; ret[10] = m[10]; ret[11] = m[14];
    ret[12] = m[3]; ret[13] = m[7]; ret[14] = m[11]; ret[15] = m[15];
}

/**
 * Get random. Use `%` modulo to make it smaller, for example `1 + rand() % 6` returns random between 1 and 6, like a dice.
 * @return A random number between 0 and 2^^32^^-1 (4294967295).
 * @see [rnd]
 */
uint32_t meg4_api_rand(void)
{
    return rand() ^ (meg4.mmio.now >> 32) ^ (meg4.mmio.now & 0xffffffff);
}

/**
 * Get random. Same as [rand], but returns a floating point number.
 * @return A random number between 0.0 and 1.0.
 * @see [rand]
 */
float meg4_api_rnd(void)
{
    return (float)((double)meg4_api_rand() / (double)0xffffffff);
}

/**
 * Returns the floating point equivalent of an integer number.
 * @param val value
 * @return The floating point of value.
 * @see [int]
 */
float meg4_api_float(int val)
{
    return (float)(val);
}

/**
 * Returns the integer equivalent of a floating point number.
 * @param val value
 * @return The integer of value.
 * @see [float]
 */
int meg4_api_int(float val)
{
    return (int)(val);
}

/**
 * Returns the largest integral number that's not greater than value.
 * @param val value
 * @return The floor of value.
 * @see [ceil]
 */
float meg4_api_floor(float val)
{
    return floorf(val);
}

/**
 * Returns the smallest integral number that's not less than value.
 * @param val value
 * @return The ceiling of value.
 * @see [floor]
 */
float meg4_api_ceil(float val)
{
    return ceilf(val);
}

/**
 * Returns the sign of the value.
 * @param val value
 * @return Either 1.0 or -1.0.
 * @see [abs]
 */
float meg4_api_sgn(float val)
{
    return val >= 0.0 ? 1.0 : -1.0;
}

/**
 * Returns the absolute of the value.
 * @param val value
 * @return Either value or -value, always positive.
 * @see [sgn]
 */
float meg4_api_abs(float val)
{
    return fabsf(val);
}

/**
 * Returns the exponential of the value, i.e. base of natural logarithms raised to power of the value.
 * @param val value
 * @return Returns e^^val^^.
 * @see [log], [pow]
 */
float meg4_api_exp(float val)
{
    return expf(val);
}

/**
 * Returns the natural logarithm of the value.
 * @param val value
 * @return Returns natural logarithm of val.
 * @see [exp]
 */
float meg4_api_log(float val)
{
    return logf(val);
}

/**
 * Returns the value raised to the power of exponent. This is a slow operation, try to avoid.
 * @param val value
 * @param exp exponent
 * @return Returns val^^exp^^.
 * @see [exp], [sqrt], [rsqrt]
 */
float meg4_api_pow(float val, float exp)
{
    return powf(val, exp);
}

/**
 * Returns the square root of the value. This is a slow operation, try to avoid.
 * @param val value
 * @return Square root.
 * @see [pow], [rsqrt]
 */
float meg4_api_sqrt(float val)
{
    return sqrtf(val);
}

/**
 * Returns the reciprocal of the square root of the value (`1 / sqrt(val)`). Uses John Carmack's fast method.
 * @param val value
 * @return Reciprocal of the square root.
 * @see [pow], [sqrt]
 */
float meg4_api_rsqrt(float val)
{
    union { float f; uint32_t i; } u;
    u.f = val; u.i = 0x5F1FFFF9 - (u.i >> 1);
    return u.f * 0.703952253f * (2.38924456f - val * u.f * u.f);
}

/**
 * Clamps a value between the limits.
 * @param val value
 * @param minv minimum value
 * @param maxv maximum value
 * @return Clamped value.
 * @see [clampv2], [clampv3], [clampv4]
 */
float meg4_api_clamp(float val, float minv, float maxv)
{
    float v = val > maxv ? maxv : val;
    return v < minv ? minv : v;
}

/**
 * Linear interpolates two numbers.
 * @param a first float number
 * @param b second float number
 * @param t interpolation value between 0.0 and 1.0
 * @see [lerpv2], [lerpv3], [lerpv4], [lerpq], [slerpq]
 */
float meg4_api_lerp(float a, float b, float t)
{
    return (1.0f - t) * a + t * b;
}

/**
 * Returns π as a floating point number.
 * @return The value 3.14159265358979323846.
 */
float meg4_api_pi(void)
{
    return MEG4_PI;
}

/**
 * Returns cosine.
 * @param deg degree, 0 to 359, 0 is up, 90 on the right
 * @return Cosine of the degree, between -1.0 to 1.0.
 * @see [sin], [tan], [acos], [asin], [atan], [atan2]
 */
float meg4_api_cos(uint16_t deg)
{
    return cosf(((deg + 270) % 360) * MEG4_PI / 180.0);
}

/**
 * Returns sine.
 * @param deg degree, 0 to 359, 0 is up, 90 to the right
 * @return Sine of the degree, between -1.0 to 1.0.
 * @see [cos], [tan], [acos], [asin], [atan], [atan2]
 */
float meg4_api_sin(uint16_t deg)
{
    return sinf(((deg + 270) % 360) * MEG4_PI / 180.0);
}

/**
 * Returns tangent.
 * @param deg degree, 0 to 359, 0 is up, 90 to the right
 * @return Tangent of the degree, between -1.0 to 1.0.
 * @see [cos], [sin], [acos], [asin], [atan], [atan2]
 */
float meg4_api_tan(uint16_t deg)
{
    return tanf(((deg + 270) % 360) * MEG4_PI / 180.0);
}

/**
 * Returns arcus cosine.
 * @param val value, -1.0 to 1.0
 * @return Arcus cosine in degree, 0 to 359, 0 is up, 90 to the right.
 * @see [cos], [sin], [tan], [asin], [atan], [atan2]
 */
uint16_t meg4_api_acos(float val)
{
    return ((int)(acosf(val) * 180.0 / MEG4_PI) - 270) % 360;
}

/**
 * Returns arcus sine.
 * @param val value, -1.0 to 1.0
 * @return Arcus sine in degree, 0 to 359, 0 is up, 90 to the right.
 * @see [cos], [sin], [tan], [acos], [atan], [atan2]
 */
uint16_t meg4_api_asin(float val)
{
    return ((int)(asinf(val) * 180.0 / MEG4_PI) - 270) % 360;
}

/**
 * Returns arcus tangent.
 * @param val value, -1.0 to 1.0
 * @return Arcus tangent in degree, 0 to 359, 0 is up, 90 to the right.
 * @see [cos], [sin], [tan], [acos], [asin], [atan2]
 */
uint16_t meg4_api_atan(float val)
{
    return ((int)(atanf(val) * 180.0 / MEG4_PI) - 270) % 360;
}

/**
 * Returns arcus tangent for y/x, using the signs of y and x to determine the quadrant.
 * @param y Y coordinate
 * @param x X coordinate
 * @return Arcus tangent in degree, 0 to 359, 0 is up, 90 to the right.
 * @see [cos], [sin], [tan], [acos], [asin]
 */
uint16_t meg4_api_atan2(float y, float x)
{
    return ((int)(atan2f(y, x) * 180.0 / MEG4_PI) - 270) % 360;
}

/**
 * Calculates dot product of two vectors with two elements.
 * @param a address of two floats
 * @param b address of two floats
 * @return Dot product of the vectors.
 * @see [lenv2], [scalev2], [negv2], [addv2], [subv2], [mulv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
float meg4_api_dotv2(addr_t a, addr_t b)
{
    float *A, *B;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 || b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return 0.0f;
    A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    return A[0] * B[0] + A[1] * B[1];
}

/**
 * Calculates the length of a vector with two elements. This is slow, try to avoid (see [normv2]).
 * @param a address of two floats
 * @return Length of the vector.
 * @see [dotv2], [scalev2], [negv2], [addv2], [subv2], [mulv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
float meg4_api_lenv2(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return 0.0f;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    return sqrtf(A[0] * A[0] + A[1] * A[1]);
}

/**
 * Scales a vector with two elements.
 * @param a address of two floats
 * @param b scaler value
 * @see [dotv2], [lenv2], [negv2], [addv2], [subv2], [mulv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
void meg4_api_scalev2(addr_t a, float s)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] *= s; A[1] *= s;
}

/**
 * Negates a vector with two elements.
 * @param a address of two floats
 * @see [dotv2], [lenv2], [scalev2], [addv2], [subv2], [mulv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
void meg4_api_negv2(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] = -A[0]; A[1] = -A[1];
}

/**
 * Adds together vectors with two elements.
 * @param dst address of two floats
 * @param a address of two floats
 * @param b address of two floats
 * @see [dotv2], [lenv2], [scalev2], [negv2], [subv2], [mulv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
void meg4_api_addv2(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] + B[0]; R[1] = A[1] + B[1];
}

/**
 * Subtracts vectors with two elements.
 * @param dst address of two floats
 * @param a address of two floats
 * @param b address of two floats
 * @see [dotv2], [lenv2], [scalev2], [negv2], [addv2], [mulv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
void meg4_api_subv2(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] - B[0]; R[1] = A[1] - B[1];
}

/**
 * Multiplies vectors with two elements.
 * @param dst address of two floats
 * @param a address of two floats
 * @param b address of two floats
 * @see [dotv2], [lenv2], [scalev2], [negv2], [addv2], [subv2], [divv2], [clampv2], [lerpv2], [normv2]
 */
void meg4_api_mulv2(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] * B[0]; R[1] = A[1] * B[1];
}

/**
 * Divides vectors with two elements.
 * @param dst address of two floats
 * @param a address of two floats
 * @param b address of two floats
 * @see [dotv2], [lenv2], [scalev2], [negv2], [addv2], [subv2], [mulv2], [clampv2], [lerpv2], [normv2]
 */
void meg4_api_divv2(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = B[0] != 0.0 && B[0] != -0.0 ? A[0] / B[0] : B[0];
    R[1] = B[1] != 0.0 && B[1] != -0.0 ? A[1] / B[1] : B[1];
}

/**
 * Clamps vectors with two elements.
 * @param dst address of two floats
 * @param v address of two floats, input
 * @param minv address of two floats, minimum
 * @param maxv address of two floats, maximum
 * @see [dotv2], [lenv2], [scalev2], [negv2], [addv2], [subv2], [mulv2], [divv2], [lerpv2], [normv2]
 */
void meg4_api_clampv2(addr_t dst, addr_t v, addr_t minv, addr_t maxv)
{
    float *R, *A, *B, *C;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || v < MEG4_MEM_USER || v > MEG4_MEM_LIMIT - 256 ||
        minv < MEG4_MEM_USER || minv > MEG4_MEM_LIMIT - 256 || maxv < MEG4_MEM_USER || maxv > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + v - MEG4_MEM_USER);
    B = (float*)(meg4.data + minv - MEG4_MEM_USER); C = (float*)(meg4.data + maxv - MEG4_MEM_USER);
    R[0] = meg4_api_clamp(A[0], B[0], C[0]);
    R[1] = meg4_api_clamp(A[1], B[1], C[1]);
}

/**
 * Linear interpolates vectors with two elements.
 * @param dst address of two floats
 * @param a address of two floats
 * @param b address of two floats
 * @param t interpolation value between 0.0 and 1.0
 * @see [dotv2], [lenv2], [scalev2], [negv2], [addv2], [subv2], [mulv2], [divv2], [clampv2], [normv2]
 */
void meg4_api_lerpv2(addr_t dst, addr_t a, addr_t b, float t)
{
    float *R, *A, *B, r = 1.0f - t;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = r * A[0] + t * B[0];
    R[1] = r * A[1] + t * B[1];
}

/**
 * Normalizes a vector with two elements.
 * @param a address of two floats
 * @see [dotv2], [lenv2], [scalev2], [negv2], [addv2], [subv2], [mulv2], [divv2], [clampv2], [lerpv2]
 */
void meg4_api_normv2(addr_t a)
{
    union { float f; uint32_t i; } u;
    float *A, v;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    v = A[0] * A[0] + A[1] * A[1];
    u.f = v; u.i = 0x5F1FFFF9 - (u.i >> 1);
    v = u.f * 0.703952253f * (2.38924456f - v * u.f * u.f);
    A[0] *= v; A[1] *= v;
}

/**
 * Calculates dot product of two vectors with three elements.
 * @param a address of three floats
 * @param b address of three floats
 * @return Dot product of the vectors.
 * @see [lenv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
float meg4_api_dotv3(addr_t a, addr_t b)
{
    float *A, *B;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 || b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return 0.0f;
    A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    return A[0] * B[0] + A[1] * B[1] + A[2] * B[2];
}

/**
 * Calculates the length of a vector with three elements. This is slow, try to avoid (see [normv3]).
 * @param a address of three floats
 * @return Length of the vector.
 * @see [dotv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
float meg4_api_lenv3(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return 0.0f;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    return sqrtf(A[0] * A[0] + A[1] * A[1] + A[2] * A[2]);
}

/**
 * Scales a vector with three elements.
 * @param a address of three floats
 * @param b scaler value
 * @see [dotv3], [lenv3], [negv3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_scalev3(addr_t a, float s)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] *= s; A[1] *= s; A[2] *= s;
}

/**
 * Negates a vector with three elements.
 * @param a address of three floats
 * @see [dotv3], [lenv3], [scalev3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_negv3(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] = -A[0]; A[1] = -A[1]; A[2] = -A[2];
}

/**
 * Adds together vectors with three elements.
 * @param dst address of three floats
 * @param a address of three floats
 * @param b address of three floats
 * @see [dotv3], [lenv3], [scalev3], [negv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_addv3(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] + B[0]; R[1] = A[1] + B[1]; R[2] = A[2] + B[2];
}

/**
 * Subtracts vectors with three elements.
 * @param dst address of three floats
 * @param a address of three floats
 * @param b address of three floats
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_subv3(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] - B[0]; R[1] = A[1] - B[1]; R[2] = A[2] - B[2];
}

/**
 * Multiplies vectors with three elements.
 * @param dst address of three floats
 * @param a address of three floats
 * @param b address of three floats
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [subv3], [divv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_mulv3(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] * B[0]; R[1] = A[1] * B[1]; R[2] = A[2] * B[2];
}

/**
 * Divides vectors with three elements.
 * @param dst address of three floats
 * @param a address of three floats
 * @param b address of three floats
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [crossv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_divv3(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = B[0] != 0.0 && B[0] != -0.0 ? A[0] / B[0] : B[0];
    R[1] = B[1] != 0.0 && B[1] != -0.0 ? A[1] / B[1] : B[1];
    R[2] = B[2] != 0.0 && B[2] != -0.0 ? A[2] / B[2] : B[2];
}

/**
 * Calculates cross product of vectors with three elements.
 * @param dst address of three floats
 * @param a address of three floats
 * @param b address of three floats
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [divv3], [clampv3], [lerpv3], [normv3]
 */
void meg4_api_crossv3(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[1] * B[2] - A[2] * B[1];
    R[1] = A[2] * B[0] - A[0] * B[2];
    R[2] = A[0] * B[1] - A[1] * B[0];
}

/**
 * Clamps vectors with three elements.
 * @param dst address of three floats
 * @param v address of three floats, input
 * @param minv address of three floats, minimum
 * @param maxv address of three floats, maximum
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [lerpv3], [normv3]
 */
void meg4_api_clampv3(addr_t dst, addr_t v, addr_t minv, addr_t maxv)
{
    float *R, *A, *B, *C;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || v < MEG4_MEM_USER || v > MEG4_MEM_LIMIT - 256 ||
        minv < MEG4_MEM_USER || minv > MEG4_MEM_LIMIT - 256 || maxv < MEG4_MEM_USER || maxv > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + v - MEG4_MEM_USER);
    B = (float*)(meg4.data + minv - MEG4_MEM_USER); C = (float*)(meg4.data + maxv - MEG4_MEM_USER);
    R[0] = meg4_api_clamp(A[0], B[0], C[0]);
    R[1] = meg4_api_clamp(A[1], B[1], C[1]);
    R[2] = meg4_api_clamp(A[2], B[1], C[2]);
}

/**
 * Linear interpolates vectors with three elements.
 * @param dst address of three floats
 * @param a address of three floats
 * @param b address of three floats
 * @param t interpolation value between 0.0 and 1.0
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [normv3]
 */
void meg4_api_lerpv3(addr_t dst, addr_t a, addr_t b, float t)
{
    float *R, *A, *B, r = 1.0f - t;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = r * A[0] + t * B[0];
    R[1] = r * A[1] + t * B[1];
    R[2] = r * A[2] + t * B[2];
}

/**
 * Normalizes a vector with three elements.
 * @param a address of three floats
 * @see [dotv3], [lenv3], [scalev3], [negv3], [addv3], [subv3], [mulv3], [divv3], [crossv3], [clampv3], [lerpv3]
 */
void meg4_api_normv3(addr_t a)
{
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    meg4_normv3((float*)(meg4.data + a - MEG4_MEM_USER));
}

/**
 * Calculates dot product of two vectors with four elements.
 * @param a address of four floats
 * @param b address of four floats
 * @return Dot product of the vectors.
 * @see [lenv4], [scalev4], [negv4], [addv4], [subv4], [mulv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
float meg4_api_dotv4(addr_t a, addr_t b)
{
    float *A, *B;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 || b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return 0.0f;
    A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    return A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3];
}

/**
 * Calculates the length of a vector with four elements. This is slow, try to avoid (see [normv4]).
 * @param a address of four floats
 * @return Length of the vector.
 * @see [dotv4], [scalev4], [negv4], [addv4], [subv4], [mulv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
float meg4_api_lenv4(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return 0.0f;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    return sqrtf(A[0] * A[0] + A[1] * A[1] + A[2] * A[2] + A[3] * A[3]);
}

/**
 * Scales a vector with four elements.
 * @param a address of four floats
 * @param b scaler value
 * @see [dotv4], [lenv4], [negv4], [addv4], [subv4], [mulv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
void meg4_api_scalev4(addr_t a, float s)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] *= s; A[1] *= s; A[2] *= s; A[3] *= s;
}

/**
 * Negates a vector with four elements.
 * @param a address of four floats
 * @see [dotv4], [lenv4], [scalev4], [addv4], [subv4], [mulv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
void meg4_api_negv4(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] = -A[0]; A[1] = -A[1]; A[2] = -A[2]; A[3] = -A[3];
}

/**
 * Adds together vectors with four elements.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [dotv4], [lenv4], [negv4], [scalev4], [subv4], [mulv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
void meg4_api_addv4(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] + B[0]; R[1] = A[1] + B[1]; R[2] = A[2] + B[2]; R[3] = A[3] + B[3];
}

/**
 * Subtracts vectors with four elements.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [dotv4], [lenv4], [scalev4], [negv4], [addv4], [mulv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
void meg4_api_subv4(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] - B[0]; R[1] = A[1] - B[1]; R[2] = A[2] - B[2]; R[3] = A[3] - B[3];
}

/**
 * Multiplies vectors with four elements.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [dotv4], [lenv4], [scalev4], [negv4], [addv4], [subv4], [divv4], [clampv4], [lerpv4], [normv4]
 */
void meg4_api_mulv4(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[0] * B[0]; R[1] = A[1] * B[1]; R[2] = A[2] * B[2]; R[3] = A[3] * B[3];
}

/**
 * Divides vectors with four elements.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [dotv4], [lenv4], [scalev4], [negv4], [addv4], [subv4], [mulv4], [clampv4], [lerpv4], [normv4]
 */
void meg4_api_divv4(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = B[0] != 0.0 && B[0] != -0.0 ? A[0] / B[0] : B[0];
    R[1] = B[1] != 0.0 && B[1] != -0.0 ? A[1] / B[1] : B[1];
    R[2] = B[2] != 0.0 && B[2] != -0.0 ? A[2] / B[2] : B[2];
    R[3] = B[3] != 0.0 && B[3] != -0.0 ? A[3] / B[3] : B[3];
}

/**
 * Clamps vectors with four elements.
 * @param dst address of four floats
 * @param v address of four floats, input
 * @param minv address of four floats, minimum
 * @param maxv address of four floats, maximum
 * @see [dotv4], [lenv4], [scalev4], [negv4], [addv4], [subv4], [mulv4], [divv4], [lerpv4], [normv4]
 */
void meg4_api_clampv4(addr_t dst, addr_t v, addr_t minv, addr_t maxv)
{
    float *R, *A, *B, *C;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || v < MEG4_MEM_USER || v > MEG4_MEM_LIMIT - 256 ||
        minv < MEG4_MEM_USER || minv > MEG4_MEM_LIMIT - 256 || maxv < MEG4_MEM_USER || maxv > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + v - MEG4_MEM_USER);
    B = (float*)(meg4.data + minv - MEG4_MEM_USER); C = (float*)(meg4.data + maxv - MEG4_MEM_USER);
    R[0] = meg4_api_clamp(A[0], B[0], C[0]);
    R[1] = meg4_api_clamp(A[1], B[1], C[1]);
    R[2] = meg4_api_clamp(A[2], B[2], C[2]);
    R[3] = meg4_api_clamp(A[3], B[3], C[3]);
}

/**
 * Linear interpolates vectors with four elements.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @param t interpolation value between 0.0 and 1.0
 * @see [dotv4], [lenv4], [scalev4], [negv4], [addv4], [subv4], [mulv4], [divv4], [clampv4], [normv4]
 */
void meg4_api_lerpv4(addr_t dst, addr_t a, addr_t b, float t)
{
    float *R, *A, *B, r = 1.0f - t;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = r * A[0] + t * B[0];
    R[1] = r * A[1] + t * B[1];
    R[2] = r * A[2] + t * B[2];
    R[3] = r * A[3] + t * B[3];
}

/**
 * Normalizes a vector with four elements.
 * @param a address of four floats
 * @see [dotv4], [lenv4], [scalev4], [negv4], [addv4], [subv4], [mulv4], [divv4], [clampv4], [lerpv4]
 */
void meg4_api_normv4(addr_t a)
{
    union { float f; uint32_t i; } u;
    float *A, v;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    v = A[0] * A[0] + A[1] * A[1] + A[2] * A[2] + A[3] * A[3];
    u.f = v; u.i = 0x5F1FFFF9 - (u.i >> 1);
    v = u.f * 0.703952253f * (2.38924456f - v * u.f * u.f);
    A[0] *= v; A[1] *= v; A[2] *= v; A[3] *= v;
}

/**
 * Loads the identity quaternion.
 * @param a address of four floats
 * @see [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_idq(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    A[0] = A[1] = A[2] = 0.0f; A[3] = 1.0f;
}

/**
 * Loads a quaternion using Euler angles.
 * @param dst address of four floats
 * @param pitch rotation around X axis in degress, 0 to 359
 * @param yaw rotation around Y axis in degress, 0 to 359
 * @param roll rotation around Z axis in degress, 0 to 359
 * @see [idq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_eulerq(addr_t dst, uint16_t pitch, uint16_t yaw, uint16_t roll)
{
    float x = ((pitch + 270) % 360) * MEG4_PI / 360.0f;
    float y = ((yaw + 270) % 360) * MEG4_PI / 360.0f;
    float z = ((roll + 270) % 360) * MEG4_PI / 360.0f;
    float sr = sinf(x), cr = cosf(x);
    float sp = sinf(y), cp = cosf(y);
    float sy = sinf(z), cy = cosf(z);
    float *R;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER);
    R[0] = sr * cp * cy - cr * sp * sy;
    R[1] = cr * sp * cy + sr * cp * sy;
    R[2] = cr * cp * sy - sr * sp * cy;
    R[3] = cr * cp * cy + sr * sp * sy;
}

/**
 * Calculates dot product of a quaternion.
 * @param a address of four floats
 * @param b address of four floats
 * @return Dot product of the quaternion.
 * @see [idq], [eulerq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
float meg4_api_dotq(addr_t a, addr_t b) { return meg4_api_dotv4(a, b); }

/**
 * Calculates the length of a quaternion.
 * @param a address of four floats
 * @return Length of the quaternion.
 * @see [idq], [eulerq], [dotq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
float meg4_api_lenq(addr_t a) { return meg4_api_lenv4(a); }

/**
 * Scales a quaternion.
 * @param a address of four floats
 * @param b scaler value
 * @see [idq], [eulerq], [dotq], [lenq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_scaleq(addr_t a, float s) { meg4_api_scalev4(a, s); }

/**
 * Negates a quaternion.
 * @param a address of four floats
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_negq(addr_t a) { meg4_api_negv4(a); }

/**
 * Adds together quaternions.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [subq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_addq(addr_t dst, addr_t a, addr_t b) { meg4_api_addv4(dst, a, b); }

/**
 * Subtracts quaternions.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [mulq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_subq(addr_t dst, addr_t a, addr_t b) { meg4_api_subv4(dst, a, b); }

/**
 * Multiplies quaternions.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [rotq], [lerpq], [slerpq], [normq]
 */
void meg4_api_mulq(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    R[0] = A[3]*B[0] + A[0]*B[3] + A[1]*B[2] - A[2]*B[1];
    R[1] = A[3]*B[1] - A[0]*B[2] + A[1]*B[3] + A[2]*B[0];
    R[2] = A[3]*B[2] + A[0]*B[1] - A[1]*B[0] + A[2]*B[3];
    R[3] = A[3]*B[3] - A[0]*B[0] - A[1]*B[1] - A[2]*B[2];
}

/**
 * Rotates a vector with three elements by a quaternion.
 * @param dst address of three floats
 * @param q address of four floats
 * @param v address of three floats
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [lerpq], [slerpq], [normq]
 */
void meg4_api_rotq(addr_t dst, addr_t q, addr_t v)
{
    float *R, *A, *B, xy, xz, yz;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || q < MEG4_MEM_USER || q > MEG4_MEM_LIMIT - 256 ||
        v < MEG4_MEM_USER || v > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + q - MEG4_MEM_USER); B = (float*)(meg4.data + v - MEG4_MEM_USER);
    xy = A[0]*B[1] - A[1]*B[0];
    xz = A[0]*B[2] - A[2]*B[0];
    yz = A[1]*B[2] - A[2]*B[1];
    R[0] = 2.0f * (+A[3]*yz + A[1]*xy + A[2]*xz) + B[0];
    R[1] = 2.0f * (-A[0]*xy - A[3]*xz + A[2]*yz) + B[1];
    R[2] = 2.0f * (-A[0]*xz - A[1]*yz + A[3]*xy) + B[2];
}

/**
 * Linear interpolates two quaternions.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @param t interpolation value between 0.0 and 1.0
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [slerpq], [normq]
 */
void meg4_api_lerpq(addr_t dst, addr_t a, addr_t b, float t) { meg4_api_lerpv4(dst, a, b, t); }

/**
 * Spherical interpolates a quaternion.
 * @param dst address of four floats
 * @param a address of four floats
 * @param b address of four floats
 * @param t interpolation value between 0.0 and 1.0
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [normq]
 */
void meg4_api_slerpq(addr_t dst, addr_t a, addr_t b, float t)
{
    float *R, *A, *B, r = 1.0f - t, q = t, d, c;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    d = A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3]; c = fabsf(d);
    if(c < 0.999) {
        c = acosf(c);
        q = 1 / sinf(c);
        r = sinf(r * c) * q;
        q *= sinf(t * c);
        if(d < 0) q = -q;
    }
    R[0] = A[0] * r + B[0] * q;
    R[1] = A[1] * r + B[1] * q;
    R[2] = A[2] * r + B[2] * q;
    R[3] = A[3] * r + B[3] * q;
}

/**
 * Normalizes a quaternion.
 * @param a address of four floats
 * @see [idq], [eulerq], [dotq], [lenq], [scaleq], [negq], [addq], [subq], [mulq], [rotq], [lerpq], [slerpq]
 */
void meg4_api_normq(addr_t a) { meg4_api_normv4(a); }

/**
 * Loads an 4 x 4 identity matrix.
 * @param a address of 16 floats
 * @see [trsm4], [detm4], [addm4], [subm4], [mulm4], [mulm4v3], [mulm4v4], [invm4], [trpm4]
 */
void meg4_api_idm4(addr_t a)
{
    float *A;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    A = (float*)(meg4.data + a - MEG4_MEM_USER);
    memset(A, 0, 16 * sizeof(float)); A[0] = A[5] = A[10] = A[15] = 1.0f;
}

/**
 * Creates a 4 x 4 matrix with translation, rotation and scaling.
 * @param dst address of 16 floats, destination matrix
 * @param t address of three floats, translation vector
 * @param r address of four floats, rotation quaternion
 * @param s address of three floats, scaling vector
 * @see [idm4], [detm4], [addm4], [subm4], [mulm4], [mulm4v3], [mulm4v4], [invm4], [trpm4]
 */
void meg4_api_trsm4(addr_t dst, addr_t t, addr_t r, addr_t s)
{
    float *R, *T, *Q, *S, xx, xy, xz, xw, yy, yz, yw, zz, zw, sx, sy, sz;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || t < MEG4_MEM_USER || t > MEG4_MEM_LIMIT - 256 ||
        r < MEG4_MEM_USER || r > MEG4_MEM_LIMIT - 256 || s < MEG4_MEM_USER || s > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); T = (float*)(meg4.data + t - MEG4_MEM_USER);
    Q = (float*)(meg4.data + r - MEG4_MEM_USER); S = (float*)(meg4.data + s - MEG4_MEM_USER);
    xx = Q[0]*Q[0]; xy = Q[0]*Q[1]; xz = Q[0]*Q[2]; xw = Q[0]*Q[3];
    yy = Q[1]*Q[1]; yz = Q[1]*Q[2]; yw = Q[1]*Q[3];
    zz = Q[2]*Q[2]; zw = Q[2]*Q[3];
    sx = 2.0f * S[0], sy = 2.0f * S[1], sz = 2.0f * S[2];
    R[0] = sx * (- yy - zz + 0.5f); R[1] = sy * (- zw + xy); R[2] = sz * (+ xz + yw); R[3] = T[0];
    R[4] = sx * (+ xy + zw); R[5] = sy * (- xx - zz + 0.5f); R[6] = sz * (- xw + yz); R[7] = T[1];
    R[8] = sx * (- yw + xz); R[9] = sy * (+ xw + yz); R[10] = sz * (- xx - yy + 0.5f); R[11] = T[2];
    R[12] = R[13] = R[14] = 0; R[15] = 1;
}

/**
 * Returns the matrix's determinant.
 * @param a address of 16 floats
 * @return The matrix's determinant.
 * @see [idm4], [trsm4], [addm4], [subm4], [mulm4], [mulm4v3], [mulm4v4], [invm4], [trpm4]
 */
float meg4_api_detm4(addr_t a)
{
    float *m;
    if(a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return -0.0f;
    m = (float*)(meg4.data + a - MEG4_MEM_USER);
    return  m[ 0]*m[ 5]*m[10]*m[15] - m[ 0]*m[ 5]*m[11]*m[14] + m[ 0]*m[ 6]*m[11]*m[13] - m[ 0]*m[ 6]*m[ 9]*m[15]
          + m[ 0]*m[ 7]*m[ 9]*m[14] - m[ 0]*m[ 7]*m[10]*m[13] - m[ 1]*m[ 6]*m[11]*m[12] + m[ 1]*m[ 6]*m[ 8]*m[15]
          - m[ 1]*m[ 7]*m[ 8]*m[14] + m[ 1]*m[ 7]*m[10]*m[12] - m[ 1]*m[ 4]*m[10]*m[15] + m[ 1]*m[ 4]*m[11]*m[14]
          + m[ 2]*m[ 7]*m[ 8]*m[13] - m[ 2]*m[ 7]*m[ 9]*m[12] + m[ 2]*m[ 4]*m[ 9]*m[15] - m[ 2]*m[ 4]*m[11]*m[13]
          + m[ 2]*m[ 5]*m[11]*m[12] - m[ 2]*m[ 5]*m[ 8]*m[15] - m[ 3]*m[ 4]*m[ 9]*m[14] + m[ 3]*m[ 4]*m[10]*m[13]
          - m[ 3]*m[ 5]*m[10]*m[12] + m[ 3]*m[ 5]*m[ 8]*m[14] - m[ 3]*m[ 6]*m[ 8]*m[13] + m[ 3]*m[ 6]*m[ 9]*m[12];
}

/**
 * Adds matrices together.
 * @param dst address of 16 floats
 * @param a address of 16 floats
 * @param b address of 16 floats
 * @see [idm4], [trsm4], [detm4], [subm4], [mulm4], [mulm4v3], [mulm4v4], [invm4], [trpm4]
 */
void meg4_api_addm4(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    int i;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    for(i = 0; i < 16; i++) R[i] = A[i] + B[i];
}

/**
 * Subtracts matrices.
 * @param dst address of 16 floats
 * @param a address of 16 floats
 * @param b address of 16 floats
 * @see [idm4], [trsm4], [detm4], [addm4], [mulm4], [mulm4v3], [mulm4v4], [invm4], [trpm4]
 */
void meg4_api_subm4(addr_t dst, addr_t a, addr_t b)
{
    float *R, *A, *B;
    int i;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + a - MEG4_MEM_USER); B = (float*)(meg4.data + b - MEG4_MEM_USER);
    for(i = 0; i < 16; i++) R[i] = A[i] - B[i];
}

/**
 * Multiplies matrices.
 * @param dst address of 16 floats
 * @param a address of 16 floats
 * @param b address of 16 floats
 * @see [idm4], [trsm4], [detm4], [addm4], [subm4], [mulm4v3], [mulm4v4], [invm4], [trpm4]
 */
void meg4_api_mulm4(addr_t dst, addr_t a, addr_t b)
{
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256 ||
        b < MEG4_MEM_USER || b > MEG4_MEM_LIMIT - 256) return;
    meg4_mulm4((float*)(meg4.data + dst - MEG4_MEM_USER), (float*)(meg4.data + a - MEG4_MEM_USER), (float*)(meg4.data + b - MEG4_MEM_USER));
}

/**
 * Multiplies a vector with three elements by a matrix.
 * @param dst address of three floats
 * @param m address of 16 floats
 * @param v address of three floats
 * @see [idm4], [trsm4], [detm4], [addm4], [subm4], [mulm4], [mulm4v4], [invm4], [trpm4]
 */
void meg4_api_mulm4v3(addr_t dst, addr_t m, addr_t v)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || m < MEG4_MEM_USER || m > MEG4_MEM_LIMIT - 256 ||
        v < MEG4_MEM_USER || v > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + m - MEG4_MEM_USER); B = (float*)(meg4.data + v - MEG4_MEM_USER);
    R[0] = A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3];
    R[1] = A[4] * B[0] + A[5] * B[1] + A[6] * B[2] + A[7];
    R[2] = A[8] * B[0] + A[9] * B[1] + A[10] * B[2] + A[11];
}

/**
 * Multiplies a vector with four elements by a matrix.
 * @param dst address of four floats
 * @param m address of 16 floats
 * @param v address of four floats
 * @see [idm4], [trsm4], [detm4], [addm4], [subm4], [mulm4], [mulm4v3], [invm4], [trpm4]
 */
void meg4_api_mulm4v4(addr_t dst, addr_t m, addr_t v)
{
    float *R, *A, *B;
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || m < MEG4_MEM_USER || m > MEG4_MEM_LIMIT - 256 ||
        v < MEG4_MEM_USER || v > MEG4_MEM_LIMIT - 256) return;
    R = (float*)(meg4.data + dst - MEG4_MEM_USER); A = (float*)(meg4.data + m - MEG4_MEM_USER); B = (float*)(meg4.data + v - MEG4_MEM_USER);
    R[0] = A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3];
    R[1] = A[4] * B[0] + A[5] * B[1] + A[6] * B[2] + A[7] * B[3];
    R[2] = A[8] * B[0] + A[9] * B[1] + A[10] * B[2] + A[11] * B[3];
    R[3] = A[12] * B[0] + A[13] * B[1] + A[14] * B[2] + A[15] * B[3];
}

/**
 * Calculates inverse matrix.
 * @param dst address of 16 floats
 * @param a address of 16 floats
 * @see [idm4], [trsm4], [detm4], [addm4], [subm4], [mulm4], [mulm4v3], [mulm4v4], [trpm4]
 */
void meg4_api_invm4(addr_t dst, addr_t a)
{
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    meg4_invm4((float*)(meg4.data + dst - MEG4_MEM_USER), (float*)(meg4.data + a - MEG4_MEM_USER));
}

/**
 * Transpose matrix.
 * @param dst address of 16 floats
 * @param a address of 16 floats
 * @see [idm4], [trsm4], [detm4], [addm4], [subm4], [mulm4], [mulm4v3], [mulm4v4], [invm4]
 */
void meg4_api_trpm4(addr_t dst, addr_t a)
{
    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || a < MEG4_MEM_USER || a > MEG4_MEM_LIMIT - 256) return;
    meg4_trpm4((float*)(meg4.data + dst - MEG4_MEM_USER), (float*)(meg4.data + a - MEG4_MEM_USER));
}

/**
 * Translate a vertex cloud, aka. place a 3D model in [3D space].
 * @param dst destination vertices array, 3 x 2 bytes each, X, Y, Z
 * @param src source vertices array, 3 x 2 bytes each, X, Y, Z
 * @param num number of vertex coordinate triplets in the array
 * @param x world X coordinate, -32767 to 32767
 * @param y world Y coordinate, -32767 to 32767
 * @param z world Z coordinate, -32767 to 32767
 * @param pitch rotation around X axis in degress, 0 to 359
 * @param yaw rotation around Y axis in degress, 0 to 359
 * @param roll rotation around Z axis in degress, 0 to 359
 * @param scale scale, use 1.0 to keep original size
 * @see [mesh]
 */
void meg4_api_trns(addr_t dst, addr_t src, uint8_t num,
    int16_t x, int16_t y, int16_t z,
    uint16_t pitch, uint16_t yaw, uint16_t roll,
    float scale)
{
    float rx = ((pitch + 270) % 360) * MEG4_PI / 360.0f;
    float ry = ((yaw + 270) % 360) * MEG4_PI / 360.0f;
    float rz = ((roll + 270) % 360) * MEG4_PI / 360.0f;
    float sr = sinf(rx), cr = cosf(rx);
    float sp = sinf(ry), cp = cosf(ry);
    float sy = sinf(rz), cy = cosf(rz);
    float rw, T[12], xx, xy, xz, xw, yy, yz, yw, zz, zw, sx, sz;
    int16_t *D, *S;
    uint8_t i;

    if(dst < MEG4_MEM_USER || dst > MEG4_MEM_LIMIT - 256 || src < MEG4_MEM_USER || src > MEG4_MEM_LIMIT - 256) return;
    if(dst + num * 6 > MEG4_MEM_LIMIT - 256) num = (MEG4_MEM_LIMIT - 256 - dst) / 6;
    if(src + num * 6 > MEG4_MEM_LIMIT - 256) num = (MEG4_MEM_LIMIT - 256 - src) / 6;
    if(num < 1) return;
    D = (int16_t*)(meg4.data + dst - MEG4_MEM_USER);
    S = (int16_t*)(meg4.data + src - MEG4_MEM_USER);

    /* calculate the translation matrix */
    rx = sr * cp * cy - cr * sp * sy;
    ry = cr * sp * cy + sr * cp * sy;
    rz = cr * cp * sy - sr * sp * cy;
    rw = cr * cp * cy + sr * sp * sy;
    xx = rx*rx; xy = rx*ry; xz = rx*rz; xw = rx*rw;
    yy = ry*ry; yz = ry*rz; yw = ry*rw;
    zz = rz*rz; zw = rz*rw;
    sx = sy = sz = 2.0f * scale;
    T[0] = sx * (- yy - zz + 0.5f); T[1] = sy * (- zw + xy); T[2] = sz * (+ xz + yw);
    T[4] = sx * (+ xy + zw); T[5] = sy * (- xx - zz + 0.5f); T[6] = sz * (- xw + yz);
    T[8] = sx * (- yw + xz); T[9] = sy * (+ xw + yz); T[10] = sz * (- xx - yy + 0.5f);

    /* convert coordinates */
    for(i = 0; i < num; i++, D += 3, S += 3) {
        xx = (float)S[0] / 32767.0f; xy = (float)S[1] / 32767.0f; xz = (float)S[2] / 32767.0f;
        D[0] = (int16_t)((T[0] * xx + T[1] * xy + T[2] * xz) * 32768.0f) + x;
        D[1] = (int16_t)((T[4] * xx + T[5] * xy + T[6] * xz) * 32768.0f) + y;
        D[2] = (int16_t)((T[8] * xx + T[9] * xy + T[10] * xz) * 32768.0f) + z;
    }
}
