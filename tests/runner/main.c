/*
 * meg4/tests/runner/main.c
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
 * @brief A simple CLI tool to batch test running scripts
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "meg4.h"
#include "cpu.h"

int verbose = 2;

/**
 * Hooks that libmeg4.a might call and a platform must provide
 */
void main_log(int lvl, const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    if(verbose >= lvl) { printf("meg4: "); vprintf(fmt, args); printf("\r\n"); }
    __builtin_va_end(args);
}

uint8_t* main_readfile(char *file, int *size)
{
    uint8_t *data = NULL;
    FILE *f;

    if(!file || !*file || !size) return NULL;
    f = fopen(file, "rb");
    if(f) {
        fseek(f, 0L, SEEK_END);
        *size = (int)ftell(f);
        fseek(f, 0L, SEEK_SET);
        data = (uint8_t*)malloc(*size);
        if(data) {
            memset(data, 0, *size);
            if((int)fread(data, 1, *size, f) != *size) { free(data); data = NULL; *size = 0; }
        }
        fclose(f);
    }
    return data;
}

int main_writefile(char *file, uint8_t *buf, int size)
{
    int ret = 0;
    FILE *f;

    if(!file || !*file || !buf || size < 1) return 0;
    f = fopen(file, "wb");
    if(f) {
        ret = ((int)fwrite(buf, 1, size, f) == size);
        fclose(f);
    }
    return ret;
}

/* callbacks that can be empty */
void main_openfile(void) { }
int main_savefile(const char *name, uint8_t *buf, int len) { return main_writefile((char*)name, buf, len); }
char **main_getfloppies(void) { return NULL; }
int main_cfgsave(char *cfg, uint8_t *buf, int len) { (void)cfg; (void)buf; (void)len; return 1; }
uint8_t *main_cfgload(char *cfg, int *len) { (void)cfg; (void)len; return NULL; }
void main_fullscreen(void) { }
char *main_getclipboard(void) { return NULL; }
void main_setclipboard(char *str) { (void)str; }
void main_osk_show(void) { }
void main_osk_hide(void) { }

/* these would be normally displayed by code_view() or debug_view(), but we don't call meg4_redraw() at all... */
uint32_t debug_disasm(uint32_t pc, char *out);
extern int errline, errpos;
extern char errmsg[256];
void print_src(int s, int e)
{
    if(!meg4.src || (uint32_t)s > meg4.src_len || s > e) return;
    for(; s < e && meg4.src[s] != '\n'; s++)
        printf("%c", meg4.src[s]);
    printf("\r\n");
}
void print_error()
{
    int i, n = 1;
    if(errmsg[0]) {
        for(i = 0, n = 1; i < (int)meg4.src_len && n != errline; i++)
            if(meg4.src[i] == '\n') n++;
        errpos -= i;
        print_src(i, meg4.src_len);
        for(i = 0; i < errpos; i++) printf(" ");
        printf("^\r\n");
        if(meg4.mode == MEG4_MODE_DEBUG) {
            printf("meg4: pc %05X sp %05X bp %05X dp %05X cp %d callstack:\r\n", meg4.pc, meg4.sp, meg4.bp, meg4.dp, meg4.cp);
            if(meg4.code && meg4.code[0] && meg4.code[0] < meg4.code_len)
                for(i = 0; i < (int)meg4.cp; i++)
                    printf("  bp %05X pc %05X\r\n", meg4.code[meg4.code[0] + i * 2], meg4.code[meg4.code[0] + i * 2 + 1]);
        }
        exit(1);
    }
}

/**
 * The main procedure
 */
int main(int argc, char **argv)
{
    int i = 1, l, re = 0, disasm = 0;
    uint32_t pc;
    uint8_t *ptr;
    char *fn, tmp[256];

    /* "parse" command line arguments */
    if(argc < 2 || (argv[1][0] == '-' && argc < 3)) {
        printf("MEG-4 Script Runner by bzt Copyright (C) 2023 GPLv3+\r\n\r\n");
        printf("%s [-d|-v|-r] <script>\r\n", argv[0]);
        return 0;
    }
    if(argv[1][0] == '-') { i++; if(argv[1][1] == 'v') verbose = 3; else if(argv[1][1] == 'r') re++; else disasm++; }

    /* turn on the emulator */
    meg4_poweron("en");
    /* insert "floppy" */
    if((ptr = main_readfile(argv[i], &l))) {
        fn = strrchr(argv[i], '/'); if(!fn) fn = argv[i]; else fn++;
        meg4_insert(fn, ptr, l);
        free(ptr);
    }
    /* when program source insertion detected, import switches to code editor automatically */
    meg4.mode = MEG4_MODE_GAME;

    /* compile and run */
    i = cpu_compile();
    if(re) printf("meg4: first compile, cpu_compile() = %d\r\n", i);
    if(!i) print_error();
    else if(disasm) {
        if(!meg4.code || meg4.code_len < 4 || meg4.code_type >= 0x10) printf("No bytecode?\r\n");
        else {
            printf("meg4: disassembling...\r\n\r\nHeader (4 words)\r\n");
            printf("  %05X: %08x Code debug segment (header + text size)\r\n", 0, meg4.code[0]);
            printf("  %05X: %08x Data debug segment\r\n", 1, meg4.code[1]);
            printf("  %05X: %08x Entry point (setup)\r\n", 2, meg4.code[2]);
            printf("  %05X: %08x Entry point (loop)\r\n", 3, meg4.code[3]);
            printf("\r\nText Segment (%u words)\r\n", meg4.code[0] - 4);
            for(pc = 4; pc < meg4.code_len && pc < meg4.code[0];) {
                printf("  %05X: %08x ", pc, meg4.code[pc]);
                /* normally debug_disasm() returns a cropped sw, but here print all case options */
                if((meg4.code[pc] & 0xff) == BC_SW) {
                    l = meg4.code[pc++] >> 8;
                    printf("sw %d", meg4.code[pc++]);
                    for(i = 0; i <= l; i++) printf(",%05X", meg4.code[pc++]);
                    printf("\n");
                } else {
                    for(i = -1, l = meg4.code[0]; (uint32_t)l < meg4.code_len && (uint32_t)l < meg4.code[1]; l += 2)
                        if(meg4.code[l] == pc) { i = meg4.code[l + 1]; break; }
                    pc = debug_disasm(pc, tmp);
                    printf("%-24s", tmp);
                    if(i != -1) print_src(i, meg4.src_len);
                    else printf("\r\n");
                }
            }
            if(meg4.code_len > meg4.code[0]) {
                if(meg4.dp && meg4.code[1] && meg4.code_len > meg4.code[1]) {
                    printf("\r\nData Segment (%u bytes)\r\n", meg4.dp);
                    /* global variables */
                    for(l = 0, pc = meg4.code[1]; pc < meg4.code_len; pc += 4) {
                        printf("  %05X:", meg4.code[pc + 2]);
                        for(i = 0; i < (int)meg4.code[pc + 3]; i++, l++) printf(" %02x", meg4.data[meg4.code[pc + 2] - MEG4_MEM_USER + i]);
                        printf("\r\n");
                    }
                    /* constants */
                    if(l < (int)meg4.dp) {
                        printf("  %05X:", MEG4_MEM_USER + l);
                        for(; l < (int)meg4.dp; l++) printf(" %02x", meg4.data[l]);
                        printf("\r\n");
                    }
                }
                printf("\r\nCode Debug Segment (%u words)\r\n", meg4.code[1] - meg4.code[0]);
                for(pc = meg4.code[0]; pc < meg4.code_len && pc < meg4.code[1]; pc += 2) {
                    printf("  %05X: pc:%05X src:%u\t\t", pc, meg4.code[pc], meg4.code[pc + 1]);
                    print_src(meg4.code[pc + 1], meg4.src_len);
                }
                if(meg4.code[1] && meg4.code_len > meg4.code[1]) {
                    printf("\r\nData Debug Segment (%u words)\r\n", meg4.code_len - meg4.code[1]);
                    for(pc = meg4.code[1]; pc < meg4.code_len; pc += 4) {
                        i = meg4.code[pc + 1] & 0xffffff;
                        printf("  %05X: type:%02x src:%u[%u]\taddr:%05X len:%u\t", pc, meg4.code[pc], i, meg4.code[pc + 1] >> 24,
                            meg4.code[pc + 2], meg4.code[pc + 3]);
                        print_src(i, i + ((meg4.code[pc + 1] >> 24) & 0xff));
                    }
                }
            }
            printf("\r\n");
        }
    } else {
        /* simulate a few calls for testing, in reality this should run at 60 FPS infinitely */
        for(i = 0; i < 8; i++) {
            printf("\r\n----- iteration %d: setup done %d blocked io %d tmr %d critical %d -----\r\n", i, meg4.flg & 1, meg4.flg & 2, meg4.flg & 4, meg4.flg & 8);
            meg4_run();
            print_error();
            if(i == 3 || i == 5) meg4_pushkey("a");
        }

        if(re) {
            /* try again, globals and blocked state should be reset, and API should be still available */
            i = cpu_compile();
            printf("meg4: recompiled, cpu_compile() = %d\r\n", i);
            if(!i) print_error();
            else {
                for(i = 0; i < 4; i++) {
                    printf("\r\n----- iteration %d: setup done %d blocked io %d tmr %d critical %d -----\r\n", i, meg4.flg & 1, meg4.flg & 2, meg4.flg & 4, meg4.flg & 8);
                    meg4_run();
                    print_error();
                }
            }
        }
    }

    /* free resources */
    meg4_poweroff();
    return 0;
}
