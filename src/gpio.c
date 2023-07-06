/*
 * meg4/gpio.c
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
 * @brief GPIO interface
 * @chapter GPIO
 *
 */

#include "meg4.h"

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

#define GPIO_DEV "/dev/gpiochip0"

/* physical GPIO pin to GPIO offset assignment RPi3B+ */
static int gpio_layout[] = { -1,
    /* 3.3v */ -1, -1, /* 5v */
                2, -1, /* 5v */
                3, -1, /* gnd */
                4, 14,
     /* gnd */ -1, 15,
               17, 18,
               27, -1, /* gnd */
               22, 23,
    /* 3.3v */ -1, 24,
               10, -1, /* gnd */
                9, 25,
               11, 8,
     /* gnd */ -1, 7,
                0, 1,
                5, -1, /* gnd */
                6, 12,
               13, -1, /* gnd */
               19, 16,
               26, 20,
     /* gnd */ -1, 21 };
#endif

/**
 * Query the GPIO board's revision number. Returns 0 if the platform isn't GPIO capable.
 * @return Board's revision number or 0 if not supported.
 */
uint32_t meg4_api_gpio_rev(void)
{
    uint32_t rev = 0;
#ifdef __linux__
    char tmp[4096], *ptr;
    int f;

    /* get board's revision number */
    rev = 0;
    f = open("/proc/cpuinfo", O_RDONLY);
    if(f >= 0) {
        memset(tmp, 0, sizeof(tmp));
        if(read(f, tmp, sizeof(tmp) - 1) < 1 || !strstr(tmp, "Raspberry Pi") || !(ptr = strstr(tmp, "\nRevision"))) { close(f); return 0; }
        close(f);
        for(; *ptr && *ptr != ':'; ptr++);
        if(*ptr != ':') return 0;
        for(ptr++; *ptr == ' '; ptr++);
        for(; *ptr && *ptr != '\n'; ptr++) {
            if(*ptr >= '0' && *ptr <= '9') {      rev <<= 4; rev += (uint32_t)(*ptr-'0'); }
            else if(*ptr >= 'a' && *ptr <= 'f') { rev <<= 4; rev += (uint32_t)(*ptr-'a'+10); }
            else if(*ptr >= 'A' && *ptr <= 'F') { rev <<= 4; rev += (uint32_t)(*ptr-'A'+10); }
            else break;
        }
    }
#endif
    return rev;
}

/**
 * Read the value of a GPIO pin.
 * @param pin physical pin number, 1 to 40
 * @return Returns 1 if the pin is high, 0 if it's low, -1 on error (GPIO pin not supported).
 * @see [gpio_set]
 */
int meg4_api_gpio_get(uint8_t pin)
{
#ifdef __linux__
    struct gpiohandle_request rq;
    struct gpiohandle_data data;
    int fd, ret;

    if(pin < 1 || pin > 40 || gpio_layout[(int)pin] == -1 || (fd = open(GPIO_DEV, O_RDWR)) < 0) return -1;
    memset(&rq, 0, sizeof(rq));
    memset(&data, 0, sizeof(data));
    rq.lineoffsets[0] = gpio_layout[(int)pin];
    rq.lines = 1;
    rq.flags = GPIOHANDLE_REQUEST_INPUT;
    ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
    close(fd);
    if(!ret && rq.fd >= 0) {
        ret = ioctl(rq.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
        close(rq.fd);
        return !ret ? data.values[0] : -1;
    }
#else
    (void)pin;
#endif
    return -1;
}

/**
 * Write the value to a GPIO pin.
 * @param pin physical pin number, 1 to 40
 * @param value 1 to set the pin high, 0 for low.
 * @return Returns 1 on success, 0 on error (GPIO pin not supported).
 * @see [gpio_get]
 */
int meg4_api_gpio_set(uint8_t pin, int value)
{
#ifdef __linux__
    struct gpiohandle_request rq;
    struct gpiohandle_data data;
    int fd, ret;

    if(pin < 1 || pin > 40 || gpio_layout[(int)pin] == -1 || (fd = open(GPIO_DEV, O_RDWR)) < 0) return 0;
    memset(&rq, 0, sizeof(rq));
    memset(&data, 0, sizeof(data));
    rq.lineoffsets[0] = gpio_layout[(int)pin];
    rq.lines = 1;
    rq.flags = GPIOHANDLE_REQUEST_OUTPUT;
    ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
    close(fd);
    if(!ret && rq.fd >= 0) {
        data.values[0] = !!value;
        ret = ioctl(rq.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        close(rq.fd);
        return !ret;
    }
#else
    (void)pin; (void)value;
#endif
    return 0;
}
