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
#include <fcntl.h>
#include <unistd.h>

#define GPIO_PREFIX "/sys/class/gpio"

/* Pin layouts. More might be added in the future */
static int gpio_layout_old[] = { 0, 0, 3, 5, 7, 0, 0, 26, 24, 21, 19, 23, 0, 0, 0, 0, 0, 11, 12, 0, 0, 0, 15, 16, 18, 22, 0, 13 };
static int gpio_layout_j8[] =  { 0, 0, 3, 5, 7, 29, 31, 26, 24, 21, 19, 23, 32, 33, 8, 10, 36, 11, 12, 35, 38, 40, 15, 16, 18, 22, 37, 13 };

/* the current layout (NULL = GPIO not initialized) and board's revision */
static int *gpio_layout = NULL;
static uint32_t gpio_rev = 0;

/**
 * Initialize the GPIO
 */
static int meg4_gpio_init(void)
{
    char tmp[4096], *ptr;
    int i, f;

    if(gpio_layout) return 1;
    /* get board's revision number */
    gpio_rev = 0;
    f = open("/proc/cpuinfo", O_RDONLY);
    if(f < 0) return 0;
    memset(tmp, 0, sizeof(tmp));
    if(read(f, tmp, sizeof(tmp) - 1) < 1 || !strstr(tmp, "Raspberry Pi") || !(ptr = strstr(tmp, "\nRevision"))) { close(f); return 0; }
    close(f);
    for(; *ptr && *ptr != ':'; ptr++);
    if(*ptr != ':') return 0;
    for(ptr++; *ptr == ' '; ptr++);
    for(; *ptr && *ptr != '\n'; ptr++) {
        if(*ptr >= '0' && *ptr <= '9') {      gpio_rev <<= 4; gpio_rev += (uint32_t)(*ptr-'0'); }
        else if(*ptr >= 'a' && *ptr <= 'f') { gpio_rev <<= 4; gpio_rev += (uint32_t)(*ptr-'a'+10); }
        else if(*ptr >= 'A' && *ptr <= 'F') { gpio_rev <<= 4; gpio_rev += (uint32_t)(*ptr-'A'+10); }
        else break;
    }
    if(!gpio_rev) return 0;
    /* determine pin layout */
    gpio_layout = (gpio_rev < 16 ? gpio_layout_old : gpio_layout_j8);
    /* export pins and reset them to output mode */
    strcpy(tmp, GPIO_PREFIX); ptr = tmp + strlen(tmp);
    for(i = 0; i < (int)(sizeof(gpio_layout_j8)/sizeof(gpio_layout_j8[0])); i++)
        if(gpio_layout[i]) {
            f = open(GPIO_PREFIX "/export", O_WRONLY);
            if(f >= 0) {
                if(gpio_layout[i] < 10) { ptr[0] = gpio_layout[i] + '0'; ptr[1] = 0; write(f, ptr, 1); }
                else { ptr[0] = (gpio_layout[i] / 10) + '0'; ptr[1] = (gpio_layout[i] % 10) + '0'; ptr[2] = 0; write(f, ptr, 2); }
                close(f);
                strcat(ptr, "/direction");
                f = open(ptr, O_WRONLY);
                if(f >= 0) { write(f, "out", 3); close(f); }
            } else { gpio_layout = NULL; return 0; }
        }
    return 1;
}

/**
 * Query the GPIO board's revision number. Returns 0 if GPIO isn't supported on the platform.
 * @return Board's revision number.
 */
uint32_t meg4_api_gpio_rev(void)
{
    if(!gpio_layout) meg4_gpio_init();
    return gpio_rev;
}

/**
 * Configure the direction of a pin.
 * @param pin pin number
 * @param dir 0=output, 1=input
 * @return Returns 1 on success, 0 on error (GPIO pin not supported on platform).
 * @see [gpio_get], [gpio_set]
 */
int meg4_api_gpio_conf(uint8_t pin, uint8_t dir)
{
    char tmp[128], *ptr;
    int f;

    if(pin > sizeof(gpio_layout_j8)/sizeof(gpio_layout_j8[0]) || (!gpio_layout && !meg4_gpio_init())) return 0;
    if(!gpio_layout || !gpio_layout[(int)pin]) return 0;
    strcpy(tmp, GPIO_PREFIX); ptr = tmp + strlen(tmp);
    if(gpio_layout[(int)pin] < 10) { ptr[0] = gpio_layout[(int)pin] + '0'; ptr[1] = 0; }
    else { ptr[0] = (gpio_layout[(int)pin] / 10) + '0'; ptr[1] = (gpio_layout[(int)pin] % 10) + '0'; ptr[2] = 0; }
    strcat(ptr, "/direction");
    f = open(ptr, O_WRONLY);
    if(f >= 0) { write(f, dir ? "in" : "out", dir ? 2 : 3); close(f); return 1; }
    return 0;
}

/**
 * Read the value of a GPIO input pin.
 * @param pin pin number
 * @return Returns 1 if the pin is high, 0 if it's low.
 * @see [gpio_conf], [gpio_set]
 */
int meg4_api_gpio_get(uint8_t pin)
{
    char tmp[128], *ptr;
    int f, ret = 0;

    if(pin > sizeof(gpio_layout_j8)/sizeof(gpio_layout_j8[0]) || (!gpio_layout && !meg4_gpio_init())) return 0;
    if(!gpio_rev || !gpio_layout || !gpio_layout[(int)pin]) return 0;
    strcpy(tmp, GPIO_PREFIX); ptr = tmp + strlen(tmp);
    if(gpio_layout[(int)pin] < 10) { ptr[0] = gpio_layout[(int)pin] + '0'; ptr[1] = 0; }
    else { ptr[0] = (gpio_layout[(int)pin] / 10) + '0'; ptr[1] = (gpio_layout[(int)pin] % 10) + '0'; ptr[2] = 0; }
    strcat(ptr, "/value");
    f = open(ptr, O_RDONLY);
    if(f >= 0) {
        if(read(f, tmp, 1) == 1 && tmp[0] != '0') ret = 1;
        close(f);
        return ret;
    }
    return 0;
}

/**
 * Write the value of a GPIO output pin.
 * @param pin pin number
 * @param value 1 to set the pin high, 0 for low.
 * @return Returns 1 on success, 0 on error (GPIO pin not supported on platform).
 * @see [gpio_conf], [gpio_get]
 */
int meg4_api_gpio_set(uint8_t pin, int value)
{
    char tmp[128], *ptr;
    int f, ret = 0;

    if(pin > sizeof(gpio_layout_j8)/sizeof(gpio_layout_j8[0]) || (!gpio_layout && !meg4_gpio_init())) return 0;
    if(!gpio_rev || !gpio_layout || !gpio_layout[(int)pin]) return 0;
    strcpy(tmp, GPIO_PREFIX); ptr = tmp + strlen(tmp);
    if(gpio_layout[(int)pin] < 10) { ptr[0] = gpio_layout[(int)pin] + '0'; ptr[1] = 0; }
    else { ptr[0] = (gpio_layout[(int)pin] / 10) + '0'; ptr[1] = (gpio_layout[(int)pin] % 10) + '0'; ptr[2] = 0; }
    strcat(ptr, "/value");
    f = open(ptr, O_WRONLY);
    if(f >= 0) {
        tmp[0] = value ? '1' : '0';
        if(write(f, tmp, 1) == 1) ret = 1;
        close(f);
        return ret;
    }
    return 0;
}
