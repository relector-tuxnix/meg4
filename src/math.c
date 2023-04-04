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
    u.f = val * 0.5f;
    u.i = (0x5f3759df - (u.i >> 1));
    return val * (1.5f - (u.f * val * val));
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
