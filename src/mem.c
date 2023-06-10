/*
 * meg4/mem.c
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
 * @brief Memory Management Unit
 * @chapter Memory
 *
 */

#include "meg4.h"
#include "lang.h"
/* should be in stdlib.h */
char *gcvt(double number, int ndigit, char *buf);

/* for meg4_api_inflate() and meg4_api_deflate() */
int stbi_zlib_uncompress(unsigned char *buffer, int len, unsigned char *obuf, int olen);
unsigned char *stbi_zlib_compress(unsigned char *data, int data_len, int *out_len, int quality);

/**
 * Return a formated string (multiple API uses this, like meg4_api_sprintf, meg4_api_printf and meg4_api_trace etc.)
 */
int meg4_snprintf(char *dst, int len, char *fmt)
{
    uint32_t sp = meg4.sp, uval;
    uint8_t c;
    char *end = dst + len - 1, *d = dst, pad, tmp[64];
    int p1, p2, val, i, s, m;
    float fval;

    if(!dst || len < 1) return 0;
    *d = 0;
    if(!fmt || !*fmt) return 0;
    /* parse string and get arguments from VM's stack */
    meg4.sp += 4;
    while(d < end && *fmt) {
        if(*fmt == '%') {
            fmt++;
            if(*fmt == '%') { *d++ = *fmt++; continue; }
            p1 = p2 = 0; pad = *fmt == '0' ? '0' : ' ';
            while(*fmt >= '0' && *fmt <= '9') { p1 *= 10; p1 += *fmt++ - '0'; }
            if(*fmt == '.') { fmt++; while(*fmt >= '0' && *fmt <= '9') { p2 *= 10; p2 += *fmt++ - '0'; } } else p2 = 4;
            if(p1 > 32) p1 = 32;
            if(p2 > 8) p2 = 8;
            if(*fmt == 'p') { p1 = 5; pad = '0'; }
            i = 63; tmp[i] = 0;
            switch(*fmt++) {
                case 'd':
                    val = cpu_popi();
                    if(val < 0) { s = 1; uval = -val; } else { s = 0; uval = val; }
                    goto dec;
                break;
                case 'u':
                    uval = (uint32_t)cpu_popi();
                    s = 0;
dec:                do {
                        tmp[--i] = '0' + (uval % 10);
                        uval /= 10;
                    } while(uval != 0 && i > 1);
                    if(s) tmp[--i] = '-';
                    goto copy;
                break;
                case 'o':
                    uval = (uint32_t)cpu_popi();
                    do {
                        tmp[--i] = '0' + (uval & 7);
                        uval >>= 3;
                    } while(uval != 0 && i > 1);
                    goto copy;
                break;
                case 'b':
                    uval = (uint32_t)cpu_popi();
                    do {
                        tmp[--i] = '0' + (uval & 1);
                        uval >>= 1;
                    } while(uval != 0 && i > 1);
                    goto copy;
                break;
                case 'p': case 'x': case 'X':
                    uval = (uint32_t)cpu_popi();
                    do {
                        m = uval & 0xF;
                        tmp[--i] = m + (m > 9 ? (fmt[-1] == 'x' ? 0x57 : 0x37) : 0x30);
                        uval >>= 4;
                    } while(uval != 0 && i > 0);
copy:               while(i > 63 - p1) tmp[--i] = pad;
                    while(tmp[i] && d < end) *d++ = tmp[i++];
                break;
                case 'c':
                    if(meg4.sp >= sizeof(meg4.data) - 4) { MEG4_DEBUGGER(ERR_STACK); memset(tmp, 0, 4); }
                    else memcpy(tmp, meg4.data + meg4.sp, 4);
                    meg4.sp += 4;
                    i = !tmp[0] ? 0 : (tmp[1] ? (tmp[2] ? (tmp[3] ? 4 : 3) : 2) : 1);
                    if(i && d + i < end) { memcpy(d, tmp, i); d += i; }
                break;
                case 'f':
                    fval = cpu_popf();
                    gcvt((double)fval, p2, tmp);
                    for(i = 0; tmp[i] && tmp[i] != '.' && tmp[i] != 'e' && tmp[i] != 'E'; i++);
                    for(p1 -= i; d < end && p1 > 0; p1--) *d++ = pad;
                    for(i = 0; tmp[i] && d < end;) *d++ = tmp[i++];
                break;
                case 's':
                    uval = (uint32_t)cpu_popi();
                    if(!uval) {
                        i -= 6; memcpy(tmp + i, "(null)", 6);
                        p1 = 0; goto copy;
                    }
                    do {
                        c = meg4_api_inb(uval++);
                        if(!c) break;
                        *d++ = (char)c;
                    } while(d < end);
                break;
                default:
                    /* failsafe, never reached. comp_expr() should already have the format string checked */
                    meg4.sp += 4; MEG4_DEBUGGER(ERR_BADFMT);
                break;
            }
        } else
            *d++ = *fmt++;
    }
    *d = 0;
    meg4.sp = sp;
    return d - dst;
}

/**
 * Memory address translation
 */
uint8_t *meg4_memaddr(addr_t src)
{
    if(src < sizeof(meg4.mmio)) return (uint8_t*)&meg4.mmio + src;
    if(src < sizeof(meg4.mmio) + 0x4000) return meg4.waveforms[meg4.mmio.wavesel && meg4.mmio.wavesel < 32 ? meg4.mmio.wavesel - 1 : 0] +
        src - sizeof(meg4.mmio); else
    if(src < sizeof(meg4.mmio) + 0x8000) return meg4.tracks[meg4.mmio.tracksel < 8 ? meg4.mmio.tracksel : 0] + src - sizeof(meg4.mmio) - 0x4000; else
    if(src < MEG4_MEM_USER) return meg4.font + (meg4.mmio.fontsel < 16 ? meg4.mmio.fontsel * 32768 : 0) + src - sizeof(meg4.mmio) - 0x8000; else
    if(src < MEG4_MEM_LIMIT) return meg4.data + src - MEG4_MEM_USER;
    return NULL;
}

/**
 * Record free memory, used by meg4_api_realloc() and meg4_api_free() too.
 * We deliberately use the simplest algorithm possible here, optimizing to O(1) for malloc only programs
 */
void meg4_freerec(addr_t addr, uint32_t size)
{
    uint32_t i, m = addr + size;

    if(addr < MEG4_MEM_USER || size < 1) return;
    if(meg4.dp == m) {
        meg4.dp -= size;
        if(!meg4.numfmm) return;
    }
    for(i = 0; i < meg4.numfmm; i += 2) {
        if(meg4.fmm[i] == m) { meg4.fmm[i] -= size; meg4.fmm[i + 1] += size; return; }
        if(meg4.fmm[i] + meg4.fmm[i + 1] == addr) { meg4.fmm[i + 1] += size; return; }
        /* double free? */
        if(meg4.fmm[i] <= addr && meg4.fmm[i] + meg4.fmm[i + 1] >= m) return;
    }
    meg4.fmm = (uint32_t*)realloc(meg4.fmm, (meg4.numfmm + 2) * sizeof(uint32_t));
    if(!meg4.fmm) { meg4.numfmm = 0; }
    else { meg4.fmm[meg4.numfmm++] = addr; meg4.fmm[meg4.numfmm++] = size; }
}

/**
 * Read in one byte from memory.
 * @param src address, 0x00000 to 0xBFFFF
 * @return Returns the value at that address.
 */
uint8_t meg4_api_inb(addr_t src)
{
    uint8_t *ptr = meg4_memaddr(src);
    return ptr ? *ptr : 0;
}

/**
 * Read in a word (two bytes) from memory.
 * @param src address, 0x00000 to 0xBFFFE
 * @return Returns the value at that address.
 */
uint16_t meg4_api_inw(addr_t src)
{
    return le16toh((meg4_api_inb(src + 1) << 8) | meg4_api_inb(src));
}

/**
 * Read in an integer (four bytes) from memory.
 * @param src address, 0x00000 to 0xBFFFC
 * @return Returns the value at that address.
 */
uint32_t meg4_api_ini(addr_t src)
{
    return le32toh((meg4_api_inb(src + 3) << 24) | (meg4_api_inb(src + 2) << 16) | (meg4_api_inb(src + 1) << 8) | meg4_api_inb(src));
}

/**
 * Write out one byte to memory.
 * @param dst address, 0x00000 to 0xBFFFF
 * @param value value to set, 0 to 255
 */
void meg4_api_outb(addr_t dst, uint8_t value)
{
    uint8_t *ptr = meg4_memaddr(dst);
    /* do not allow overwriting the firmware version, the timers or the status registers */
    if(dst < 16 || (dst >= 0x4BA && dst < 0x500) || dst >= MEG4_MEM_LIMIT || !ptr) return;
    *ptr = value;
    if(dst >= 0x488 && dst < 0x48C) meg4_getscreen();
    if(dst >= 0x49E && dst < 0x4A9) meg4_getview();
}

/**
 * Write out a word (two bytes) to memory.
 * @param dst address, 0x00000 to 0xBFFFE
 * @param value value to set, 0 to 65535
 */
void meg4_api_outw(addr_t dst, uint16_t value)
{
    uint16_t val = htole16(value);
    if(dst < 16 || dst >= MEG4_MEM_LIMIT - 1) return;
    meg4_api_outb(dst, val & 0xff);
    meg4_api_outb(dst + 1, (val >> 8) & 0xff);
}

/**
 * Write out an integer (four bytes) to memory.
 * @param dst address, 0x00000 to 0xBFFFC
 * @param value value to set, 0 to 4294967295
 */
void meg4_api_outi(addr_t dst, uint32_t value)
{
    uint32_t val = htole32(value);
    if(dst < 16 || dst >= MEG4_MEM_LIMIT - 3) return;
    meg4_api_outb(dst, val & 0xff);
    meg4_api_outb(dst + 1, (val >> 8) & 0xff);
    meg4_api_outb(dst + 2, (val >> 16) & 0xff);
    meg4_api_outb(dst + 3, (val >> 24) & 0xff);
}

/**
 * Saves memory area to overlay.
 * @param overlay index of overlay to write to, 0 to 255
 * @param src memory offset to save from, 0x00000 to 0xBFFFF
 * @param size number of bytes to save
 * @return Returns 1 on success, 0 on error.
 * @see [memload]
 */
int meg4_api_memsave(uint8_t overlay, addr_t src, uint32_t size)
{
    uint32_t i, l;
    char tmp[sizeof meg4_title + 32];

    if(!size || src >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    if(src + size > MEG4_MEM_LIMIT) size = MEG4_MEM_LIMIT - src;
    if(meg4_title[0]) {
        strcpy(tmp, "overlays/"); strcat(tmp, meg4_title); strcat(tmp, "/mem"); l = strlen(tmp);
        i = (overlay >> 4);  tmp[l+0] = i < 10 ? i + '0' : i - 10 + 'A';
        i = (overlay & 0xf); tmp[l+1] = i < 10 ? i + '0' : i - 10 + 'A';
        strcpy(tmp + l + 2, ".bin");
    }
    meg4.ovls[(int)overlay].data = (uint8_t*)realloc(meg4.ovls[(int)overlay].data, size);
    if(meg4.ovls[(int)overlay].data) {
        if(src >= MEG4_MEM_USER) memcpy(meg4.ovls[(int)overlay].data, meg4.data + src - MEG4_MEM_USER, size);
        else for(i = 0; i < size; i++) meg4.ovls[(int)overlay].data[i] = meg4_api_inb(src + i);
        meg4.ovls[(int)overlay].size = size;
    } else meg4.ovls[(int)overlay].size = 0;
    return meg4_title[0] ? main_cfgsave(tmp, meg4.ovls[(int)overlay].data, meg4.ovls[(int)overlay].size) : 1;
}

/**
 * Loads an overlay into the specified memory area.
 * @param dst memory offset to load to, 0x00000 to 0xBFFFF
 * @param overlay index of overlay to read from, 0 to 255
 * @param maxsize maximum number of bytes to load
 * @return Returns the number of bytes actually loaded.
 * @see [memsave]
 */
int meg4_api_memload(addr_t dst, uint8_t overlay, uint32_t maxsize)
{
    uint8_t *buf = NULL;
    uint32_t i, l, len = 0;
    char tmp[sizeof meg4_title + 32];

    if(dst >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    if(dst + maxsize > MEG4_MEM_LIMIT) maxsize = MEG4_MEM_LIMIT - dst;
    meg4_api_memset(dst, 0, maxsize);
    if(meg4_title[0]) {
        strcpy(tmp, "overlays/"); strcat(tmp, meg4_title); strcat(tmp, "/mem"); l = strlen(tmp);
        i = (overlay >> 4);  tmp[l+0] = i < 10 ? i + '0' : i - 10 + 'A';
        i = (overlay & 0xf); tmp[l+1] = i < 10 ? i + '0' : i - 10 + 'A';
        strcpy(tmp + l + 2, ".bin");
        buf = main_cfgload(tmp, (int*)&len);
    }
    if(buf) {
        if(len > maxsize) len = maxsize;
        if(dst >= MEG4_MEM_USER) memcpy(meg4.data + dst - MEG4_MEM_USER, buf, len);
        else for(i = 0; i < len; i++) meg4_api_outb(dst + i, buf[i]);
        free(buf);
    } else
    if(meg4.ovls[(int)overlay].data && meg4.ovls[(int)overlay].size > 0) {
        len = meg4.ovls[(int)overlay].size;
        if(len > maxsize) len = maxsize;
        if(dst >= MEG4_MEM_USER) memcpy(meg4.data + dst - MEG4_MEM_USER, meg4.ovls[(int)overlay].data, len);
        else for(i = 0; i < len; i++) meg4_api_outb(dst + i, meg4.ovls[(int)overlay].data[i]);
    } else len = 0;
    return (int)len;
}

/**
 * Copy memory regions.
 * @param dst destination address, 0x00000 to 0xBFFFF
 * @param src source address, 0x00000 to 0xBFFFF
 * @param len number of bytes to copy
 */
void meg4_api_memcpy(addr_t dst, addr_t src, uint32_t len)
{
    int i, l = len;
    if(src + l >= MEG4_MEM_LIMIT) l = MEG4_MEM_LIMIT - src;
    if(dst + l >= MEG4_MEM_LIMIT) l = MEG4_MEM_LIMIT - dst;
    if(src >= 16 && dst >= 16 && src + l < sizeof(meg4.mmio) && dst + l < sizeof(meg4.mmio)) {
        memmove((uint8_t*)&meg4.mmio + dst, (uint8_t*)&meg4.mmio + src, l);
        if(dst <= 0x48C && dst + l >= 0x488) meg4_getscreen();
        if(dst <= 0x4A9 && dst + l >= 0x49E) meg4_getview();
    } else
    if(src >= MEG4_MEM_USER && dst >= MEG4_MEM_USER)
        memmove(meg4.data + dst - MEG4_MEM_USER, meg4.data + src - MEG4_MEM_USER, l);
    else
    if(src <= dst && src + l > dst)
        for(i = l - 1; i >= 0; i--)
            meg4_api_outb(dst + i, meg4_api_inb(src + i));
    else
        for(i = 0; i < l; i++)
            meg4_api_outb(dst + i, meg4_api_inb(src + i));
}

/**
 * Set memory region to a given value.
 * @param dst destination address, 0x00000 to 0xBFFFF
 * @param value value to set, 0 to 255
 * @param len number of bytes to set
 */
void meg4_api_memset(addr_t dst, uint8_t value, uint32_t len)
{
    int i, l = len;
    if(dst + l >= MEG4_MEM_LIMIT) l = MEG4_MEM_LIMIT - dst;
    if(dst >= 16 && dst + l < sizeof(meg4.mmio)) {
        memset((uint8_t*)&meg4.mmio + dst, value, l);
        if(dst <= 0x48C && dst + l >= 0x488) meg4_getscreen();
        if(dst <= 0x4A9 && dst + l >= 0x49E) meg4_getview();
    } else
    if(dst >= MEG4_MEM_USER)
        memset(meg4.data + dst - MEG4_MEM_USER, value, l);
    else
        for(i = 0; i < l; i++)
            meg4_api_outb(dst + i, value);
}

/**
 * Compare memory regions.
 * @param addr0 first address, 0x00000 to 0xBFFFF
 * @param addr1 second address, 0x00000 to 0xBFFFF
 * @param len number of bytes to compare
 * @return Returns difference, 0 if the two memory region matches.
 */
int meg4_api_memcmp(addr_t addr0, addr_t addr1, uint32_t len)
{
    int ret = 0, i = 0, l = len;
    if(l > 0) {
        if(addr0 + l >= MEG4_MEM_LIMIT) l = MEG4_MEM_LIMIT - addr0;
        if(addr1 + l >= MEG4_MEM_LIMIT) l = MEG4_MEM_LIMIT - addr1;
        if(addr0 + l < sizeof(meg4.mmio) && addr1 +l < sizeof(meg4.mmio))
            ret = memcmp((uint8_t*)&meg4.mmio + addr0, (uint8_t*)&meg4.mmio + addr1, l);
        else
        if(addr0 >= MEG4_MEM_USER && addr1 >= MEG4_MEM_USER)
            ret = memcmp(meg4.data + addr0 - MEG4_MEM_USER, meg4.data + addr1 - MEG4_MEM_USER, l);
        else
            for(;!ret && i < l; i++) ret = meg4_api_inb(addr0 + i) - meg4_api_inb(addr1 + i);
    }
    return ret;
}

/**
 * Compress a buffer using RFC1950 deflate (zlib).
 * @param dst destination address, 0x30000 to 0xBFFFF
 * @param src source address, 0x30000 to 0xBFFFF
 * @param len number of bytes to compress
 * @return 0 or negative on error, otherwise the length of the compressed buffer and compressed data in dst.
 * @see [inflate]
 */
int meg4_api_deflate(addr_t dst, addr_t src, uint32_t len)
{
    uint8_t *buf;
    int s;

    if(len > sizeof(meg4.data) || dst < MEG4_MEM_USER || src < MEG4_MEM_USER || src + len >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    buf = stbi_zlib_compress(meg4.data + src - MEG4_MEM_USER, (int)len, &s, 9);
    if(buf) {
        if(s > 2 && dst + s - 2 < meg4.sp) { s -= 2; memcpy(meg4.data + dst - MEG4_MEM_USER, buf + 2, s); } else s = 0;
        free(buf);
    } else s = 0;
    return s;
}

/**
 * Uncompress a buffer with RFC1950 deflate (zlib) compressed data.
 * @param dst destination address, 0x30000 to 0xBFFFF
 * @param src source address, 0x30000 to 0xBFFFF
 * @param len number of compressed bytes
 * @return 0 or negative on error, otherwise the length of the uncompressed buffer and uncompressed data in dst.
 * @see [deflate]
 */
int meg4_api_inflate(addr_t dst, addr_t src, uint32_t len)
{
    uint8_t *buf;
    uint32_t i, k;

    if(len > sizeof(meg4.data) || dst < MEG4_MEM_USER || src < MEG4_MEM_USER || src + len >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    buf = meg4.data + src - MEG4_MEM_USER;
    /* skip over zlib header (if any) */
    if(buf[0] == 0x78 && (buf[1] == 0x01 || buf[1] == 0x5E || buf[1] == 0x9C || buf[1] == 0xDA)) { buf += 2; len -= 2; } else
    /* skip over gzip header (if any) */
    if(buf[0] == 0x1F && buf[1] == 0x8B && buf[2] == 8) {
        buf += 3; i = *buf++; buf += 6; if(i & 4) { k = *buf++; k += (*buf++ << 8); buf += k; } if(i & 8) { while(*buf++ != 0); }
        if(i & 16) { while(*buf++ != 0); } if(i & 2) { buf += 2; } len -= (buf - (meg4.data + src - MEG4_MEM_USER)) + 8;
    }
    return stbi_zlib_uncompress(buf, len, meg4.data + dst - MEG4_MEM_USER, meg4.sp - (dst - MEG4_MEM_USER));
}

/**
 * Returns the number of ticks since power on.
 * @return The elapsed time in milliseconds since power on.
 * @see [now]
 */
float meg4_api_time(void)
{
    return (float)(le32toh(meg4.mmio.tick));
}

/**
 * Returns the UNIX timestamp. Check the byte at offset 0000C to see if it overflows.
 * @return The elapsed time in seconds since 1 Jan 1970 midnight, Greenwich Mean Time.
 * @see [time]
 */
uint32_t meg4_api_now(void)
{
    return (uint32_t)(le64toh(meg4.mmio.now));
}

/**
 * Converts an ASCII decimal string into an integer number.
 * @param src string address, 0x00000 to 0xBFFFF
 * @return The number value of the string.
 * @see [itoa], [str], [val]
 */
int meg4_api_atoi(str_t src)
{
    int ret = 0, sig = 0, val;
    if(src < MEG4_MEM_USER || src + 4 >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    if(meg4_api_inb(src) == '-') { sig = 1; src++; }
    if(meg4_api_inb(src) == '0' && (meg4_api_inb(src + 1) == 'x' || meg4_api_inb(src + 1) == 'X')) {
        src += 2;
        while(src < MEG4_MEM_LIMIT) {
            val = meg4_api_inb(src++);
            if(val >= '0' && val <= '9') val -= '0'; else
            if(val >= 'A' && val <= 'F') val -= 'A' - 10; else
            if(val >= 'a' && val <= 'f') val -= 'a' - 10; else break;
            ret <<= 4;
            ret += val;
        }
    } else
        while(src < MEG4_MEM_LIMIT) {
            val = meg4_api_inb(src++);
            if(val < '0' || val > '9') break;
            ret *= 10;
            ret += val - '0';
        }
    return sig ? -ret : ret;
}

/**
 * Converts an integer number into an ASCII decimal string.
 * @param value the value, -2147483648 to 2147483647
 * @return The converted string.
 * @see [atoi], [str], [val]
 */
str_t meg4_api_itoa(int value)
{
    char *tmp = (char*)meg4.data + sizeof(meg4.data) - 16, *s = tmp;
    int val = 1;

    memset(tmp, 0, 16);
    if(value < 0) { value = -value; *s++ = '-'; }
    while(val * 10 < value) val *= 10;
    do {
        *s++ = (value / val) + '0';
        val /= 10;
    } while(val > 0 && s < &tmp[31]);
    return (str_t)MEG4_MEM_LIMIT - 16;
}

/**
 * Converts an ASCII decimal string into a floating point number.
 * @param src string address, 0x00000 to 0xBFFFF
 * @return The number value of the string.
 * @see [itoa], [atoi], [str]
 */
float meg4_api_val(str_t src)
{
    if(src < MEG4_MEM_USER || src + 4 >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0.0f; }
    return (float)strtod((char*)meg4.data + src - MEG4_MEM_USER, NULL);
}

/**
 * Converts a floating point number into an ASCII decimal string.
 * @param value the value
 * @return The converted string.
 * @see [atoi], [itoa], [val]
 */
str_t meg4_api_str(float value)
{
    char *tmp = (char*)meg4.data + sizeof(meg4.data) - 64;
#if 0
    /* this would require stdio.h, which isn't allowed in base libmeg4.a */
    l = sprintf(tmp, "%g", value);
#else
    gcvt((double)value, 8, tmp);
#endif
    return (str_t)MEG4_MEM_LIMIT - 64;
}

/**
 * Returns a zero terminated UTF-8 string created using format and arguments.
 * @param fmt [format string]
 * @param ... optional arguments
 * @return Constructed string.
 */
str_t meg4_api_sprintf(str_t fmt, ...)
{
    if(fmt < MEG4_MEM_USER || fmt + 4 >= MEG4_MEM_LIMIT - 256) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    meg4_snprintf((char*)meg4.data + sizeof(meg4.data) - 256, 256, (char*)meg4.data + fmt - MEG4_MEM_USER);
    return (str_t)MEG4_MEM_LIMIT - 256;
}

/**
 * Return the number of bytes in a string (without the terminating zero).
 * @param src string address, 0x00000 to 0xBFFFF
 * @return The number of bytes in the string.
 * @see [mblen]
 */
int meg4_api_strlen(str_t src)
{
    char *s;
    int ret = 0;

    if(src < MEG4_MEM_USER || src + 1 >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    s = (char*)meg4.data + src - MEG4_MEM_USER;
    while(*s++ && ret < 255) ret++;
    return ret;
}

/**
 * Return the number of UTF-8 multi-byte characters in a string (without the terminating zero).
 * @param src string address, 0x00000 to 0xBFFFF
 * @return The number of characters in the string.
 * @see [strlen]
 */
int meg4_api_mblen(str_t src)
{
    char *s, *e;
    int ret = 0;
    uint32_t c;

    if(src < MEG4_MEM_USER || src + 1 >= MEG4_MEM_LIMIT) { MEG4_DEBUGGER(ERR_BADARG); return 0; }
    s = (char*)meg4.data + src - MEG4_MEM_USER; e = s + 255;
    while(*s && s < e) {
        s = meg4_utf8(s, &c);
        if(!c) break;
        ret++;
    }
    return ret;
}

/**
 * Allocates user memory dynamically.
 * @param size number of bytes to allocate
 * @return Address of the new allocated buffer or NULL on error.
 * @see [realloc], [free]
 */
addr_t meg4_api_malloc(uint32_t size)
{
    uint32_t i, j, k, m;
    addr_t ret = 0;

    /* look for best fit, when there's no freed memory, this is O(1) */
    for(m = k = 0xffffffff, i = 0; i < meg4.numfmm; i += 2)
        if(meg4.fmm[i + 1] > size) {
            j = meg4.fmm[i + 1] - size;
            if(j < m) { m = j; k = i; if(!j) break; }
        }
    /* without a match, allocate from top */
    if(k >= meg4.numfmm) {
        /* check if there's enough free memory */
        if((int)size >= (int)meg4.sp - (int)meg4.dp) return 0;
        ret = meg4.dp + MEG4_MEM_USER; meg4.dp += size;
    } else {
        ret = meg4.fmm[k];
        /* if there's no more free memory in this slot, free allocation data */
        if(!(meg4.fmm[k + 1] -= size)) {
            memcpy(&meg4.fmm[k], &meg4.fmm[k + 2], meg4.numfmm - k);
            meg4.numfmm -= 2;
            if(!meg4.numfmm) { free(meg4.fmm); meg4.fmm = NULL; }
        } else meg4.fmm[k] += size;
    }
    meg4.amm = (uint32_t*)realloc(meg4.amm, (meg4.numamm + 2) * sizeof(uint32_t));
    if(!meg4.amm) { meg4.numamm = 0; return 0; }
    meg4.amm[meg4.numamm++] = ret; meg4.amm[meg4.numamm++] = size;
    if(ret + size - MEG4_MEM_USER >= sizeof(meg4.data)) size = sizeof(meg4.data) + MEG4_MEM_USER - ret;
    memset(meg4.data + ret - MEG4_MEM_USER, 0, size);
    return ret;
}

/**
 * Resize a previously allocated buffer.
 * @param addr address of the allocated buffer
 * @param size number of bytes to resize to
 * @return Address of the new allocated buffer or NULL on error.
 * @see [malloc], [free]
 */
addr_t meg4_api_realloc(addr_t addr, uint32_t size)
{
    uint32_t i, s = 0;
    addr_t ret = 0;

    if(addr) {
        if(!size) { meg4_api_free(addr); return 0; }
        /* find the original size */
        for(i = 0; i < meg4.numamm; i += 2)
            if(meg4.amm[i] == addr) {
                s = meg4.amm[i + 1];
                /* if new size is smaller */
                if(size < s) {
                    meg4.amm[i + 1] = size;
                    meg4_freerec(addr + s, size - s);
                    return addr;
                }
                break;
            }
    }
    /* if new size is bigger, allocate a new buffer and copy old contents into */
    ret = meg4_api_malloc(size);
    if(!ret || (!addr && size)) return ret;
    if(s) {
        memcpy(meg4.data + ret - MEG4_MEM_USER, meg4.data + addr - MEG4_MEM_USER, s);
        meg4_freerec(addr, s);
    }
    return ret;
}

/**
 * Frees dynamically allocated user memory.
 * @param addr address of the allocated buffer
 * @return 1 on success, 0 on error.
 * @see [malloc], [realloc]
 */
int meg4_api_free(addr_t addr)
{
    uint32_t i, s = 0;
    int ret = 0;

    for(i = 0; i < meg4.numamm; i += 2)
        if(meg4.amm[i] == addr) {
            s = meg4.amm[i + 1];
            memcpy(&meg4.amm[i], &meg4.amm[i + 2], meg4.numamm - i);
            meg4.numamm -= 2;
            if(!meg4.numamm) { free(meg4.amm); meg4.amm = NULL; }
            ret = 1;
            break;
        }
    meg4_freerec(addr, s);
    return ret;
}
