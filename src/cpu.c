/*
 * meg4/cpu.c
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
 * @brief Central Processing Unit (the VM bytecode interpreter)
 *
 */

#define API_IMPL
#include "meg4.h"
#include "lang.h"
#include "cpu.h"
#include "api.h"

#include <math.h>
float powf(float, float);
uint32_t debug_disasm(uint32_t pc, char *out);

/* cpu_compile() is in its own compilation unit, in comp.c */

/**
 * Initialize the CPU
 */
void cpu_init(void)
{
    if(meg4.code) { free(meg4.code); meg4.code = NULL; }
    meg4_init_len = meg4.code_len = meg4.flg = meg4.tmr = meg4.cp = meg4.pc = meg4.dp = meg4.ac = 0; meg4.af = 0.0f;
    memset(meg4.data, 0, sizeof(meg4.data));
    /* top 256 bytes of the stack is used as temporary buffer by some system functions, like itoa(), gets() etc. */
    meg4.bp = meg4.sp = sizeof(meg4.data) - 256;
#if LUA
    comp_lua_init();
#endif
}

/**
 * Free resources
 */
void cpu_free(void)
{
#if LUA
    comp_lua_free();
#endif
}

/**
 * Get the source language's type
 */
void cpu_getlang(void)
{
    meg4.code_type = 0xff;
    if(!meg4.src || meg4.src_len < 2) return;
    /* third party scripting languages */
#if LUA
    if(!memcmp(meg4.src, "#!lua\n", 6)) meg4.code_type = 0x10; else
#endif
    /* built-in languages */
    if(!memcmp(meg4.src, "#!c\n", 4)) meg4.code_type = 0; else
    if(!memcmp(meg4.src, "#!bas\n", 6)) meg4.code_type = 1; else
    if(!memcmp(meg4.src, "#!asm\n", 6)) meg4.code_type = 15;
}

/**
 * Run the code
 */
void cpu_run(void)
{
    /* temporarily suspend execution after this many instructions and continue in next frame */
    int lim = 65536;

    /* check if there's bytecode, script finished or running is still blocked */
    if(!meg4.code || meg4.code_len < 4 ||                               /* no script */
        (meg4.flg & 8) ||                                               /* execution stopped */
       ((meg4.flg & 4) && meg4.mmio.tick < meg4.tmr) ||                 /* blocked for timer */
       ((meg4.flg & 2) && meg4.mmio.kbdhead == meg4.mmio.kbdtail)       /* blocked for io */
       ) return;
    meg4.flg &= ~4;
    switch(meg4.code_type) {
        /* third party scripting languages */
#if LUA
        case 0x10: cpu_lua(); break;
#endif
        /* all the other built-in languages */
        default:
            if(!meg4.pc) {
                /* failsafe, make sure entry points start in a consistent environment */
                meg4.tmr = meg4.cp = meg4.ac = 0; meg4.af = 0.0f;
                meg4.bp = meg4.sp = sizeof(meg4.data) - 256;
                /* have we ran setup() yet? */
                if(!(meg4.flg & 1)) {
                    meg4.flg |= 1;
                    meg4.pc = meg4.code[2] ? meg4.code[2] : meg4.code[3];
                } else
                    /* yes, so run loop() instead */
                    meg4.pc = meg4.code[3];
                /* nothing to run? */
                if(!meg4.pc) meg4.flg |= 8;
#ifndef NOEDITORS
                else main_log(3, "CPU: new entry point %05X", meg4.pc);
#endif
            }
            /* execute until function finishes, gets blocked or debugger invoked */
            do { cpu_fetch(); } while(meg4.pc && !(meg4.flg & ~1) && meg4.mode == MEG4_MODE_GAME && --lim);
        break;
    }
}

/**
 * Push an integer to stack
 */
addr_t cpu_pushi(int value)
{
    if((int)meg4.dp > (int)meg4.sp - 4) { MEG4_DEBUGGER(ERR_MEMORY); } else {
        meg4.sp -= 4;
        memcpy(meg4.data + meg4.sp, &value, 4);
    }
    return meg4.sp + MEG4_MEM_USER;
}

/**
 * Push a float to stack
 */
addr_t cpu_pushf(float value)
{
    if((int)meg4.dp > (int)meg4.sp - 4) { MEG4_DEBUGGER(ERR_MEMORY); } else {
        meg4.sp -= 4;
        memcpy(meg4.data + meg4.sp, &value, 4);
    }
    return meg4.sp + MEG4_MEM_USER;
}

/**
 * Pop an integer from stack
 */
int cpu_popi(void)
{
    int val = 0;

    if(meg4.sp >= sizeof(meg4.data) - 4) { MEG4_DEBUGGER(ERR_STACK); } else {
        memcpy(&val, meg4.data + meg4.sp, 4);
        meg4.sp += 4;
    }
    return val;
}

/**
 * Pop a float from stack
 */
float cpu_popf(void)
{
    float val = 0.0f;
    if(meg4.sp >= sizeof(meg4.data) - 4) { MEG4_DEBUGGER(ERR_STACK); } else {
        memcpy(&val, meg4.data + meg4.sp, 4);
        meg4.sp += 4;
    }
    return val;
}

/**
 * Get an integer from top of the stack
 */
int cpu_topi(uint32_t offs)
{
    int val = 0;
    if(meg4.sp + offs >= sizeof(meg4.data) - 4) { MEG4_DEBUGGER(ERR_STACK); } else
        memcpy(&val, meg4.data + meg4.sp + offs, 4);
    return val;
}

/**
 * Get a float from top of the stack
 */
float cpu_topf(uint32_t offs)
{
    float val = 0.0f;
    if(meg4.sp + offs >= sizeof(meg4.data) - 4) { MEG4_DEBUGGER(ERR_STACK); } else
        memcpy(&val, meg4.data + meg4.sp + offs, 4);
    return val;
}

/**
 * Fetch and execute one VM instruction
 */
void cpu_fetch(void)
{
#ifndef NOEDITORS
    char tmp[256];
#endif
    uint32_t pc, sp;
    int i, val;
    float fval;

    /* failsafes */
    if(meg4.code_type >= 0x10 || (meg4.flg & 8)) return;
    if(meg4.pc < 4 || meg4.pc >= meg4.code[0]) { meg4.pc = 0; return; }

    pc = meg4.pc + 1; i = meg4.code[meg4.pc]; val = i >> 8;
#ifndef NOEDITORS
    debug_disasm(meg4.pc, tmp);
    main_log(3, "CPU: %05X %s", meg4.pc, tmp);
#endif
    switch(i & 0xff) {
        /* transfer control */
        case BC_DEBUG:
#ifndef NOEDITORS
            meg4.pc++;      /* we want the instruction after the breakpoint to be reported, not the breakpoint itself */
            debug_rte(0);   /* invoke the built-in debugger without an actual run-time error; for MEG-4 PRO this is a NOP */
#endif
        break;
        case BC_RET:
            if(meg4.cp < 2) { meg4.cp = pc = 0; }
            else { meg4.cp -= 2; meg4.sp = meg4.bp; meg4.bp = meg4.cs[meg4.cp]; pc = meg4.cs[meg4.cp + 1] + 2; }
        break;
        case BC_SCALL:
            i = val;
            if(i < 0 || i >= MEG4_NUM_API) { MEG4_DEBUGGER(ERR_BADSYS); } else {
                val = 0; fval = 0; sp = meg4.sp;
                /* call the MEG-4 API */
                switch(i) {
                    MEG4_DISPATCH
                }
                if(meg4_api[i].ret == 4) { meg4.af = fval; meg4.ac = (int)fval; } else { meg4.ac = val; meg4.af = (float)val; }
                if(meg4.flg & 2) pc = meg4.pc;
                meg4.sp = sp;
            }
        break;
        case BC_CALL:
            if(meg4.cp >= sizeof(meg4.cs)) { MEG4_DEBUGGER(ERR_RECUR); } else {
                meg4.cs[meg4.cp] = meg4.bp; meg4.cs[meg4.cp + 1] = meg4.pc; meg4.cp += 2;
                meg4.bp = meg4.sp; pc = meg4.code[pc];
            }
        break;
        case BC_JMP: pc = meg4.code[pc]; break;
        case BC_JZ:  i = (int)meg4.code[pc]; pc = meg4.ac ? pc + 1 : (uint32_t)i; break;
        case BC_JNZ: i = (int)meg4.code[pc]; pc = meg4.ac ? (uint32_t)i : pc + 1; break;
        case BC_JS: case BC_JNS:
            if(meg4.af == 0.0 || meg4.af == -0.0) { MEG4_DEBUGGER(ERR_DIVZERO); } else {
                meg4.af = cpu_popf() * (meg4.af > 0.0 ? 1.9 : -1.0); meg4.ac = (int)meg4.af;
                val = (int)meg4.code[pc]; pc = ((i & 0xff) == BC_JS ? meg4.af > 0.0 : meg4.af <= 0.0) ? pc + 1 : (uint32_t)val;
            }
        break;
        case BC_SW:
            if(pc + val >= meg4.code[0]) { MEG4_DEBUGGER(ERR_BOUNDS); } else {
                i = meg4.ac - (int)meg4.code[pc];
                pc = i < 0 || i >= val ? meg4.code[pc + 1] : meg4.code[pc + 2 + i];
            }
        break;
        /* immediate constants */
        case BC_CI: meg4.ac = (int)meg4.code[pc++]; meg4.af = (float)meg4.ac; break;
        case BC_CF: memcpy(&meg4.af, &meg4.code[pc++], 4); meg4.ac = (int)meg4.af; break;
        /* stack operations */
        case BC_BND: if((uint32_t)meg4.ac >= (uint32_t)val) { MEG4_DEBUGGER(ERR_BOUNDS); } break;
        case BC_LEA:
            meg4.ac = MEG4_MEM_USER + (int)meg4.dp + (i & ~0xff) / 256 /* no shift, that would loose sign */; meg4.af = (float)meg4.ac;
            if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < MEG4_MEM_USER) { MEG4_DEBUGGER(ERR_BOUNDS); }
        break;
        case BC_ADR:
            meg4.ac = MEG4_MEM_USER + (int)meg4.bp + (i & ~0xff) / 256; meg4.af = (float)meg4.ac;
            if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < MEG4_MEM_USER) { MEG4_DEBUGGER(ERR_BOUNDS); }
        break;
        case BC_SP:
            i = (i & ~0xff) / 256;
            if(meg4.sp + i >= sizeof(meg4.data) || meg4.sp + i <= meg4.dp) { MEG4_DEBUGGER(ERR_STACK); }
            else { if(i < 0) { memset(meg4.data + meg4.sp + i, 0, -i); } meg4.sp += i; }
        break;
        case BC_PSHCI: meg4.ac = (int)meg4.code[pc++]; meg4.af = (float)meg4.ac; cpu_pushi(meg4.ac); break;
        case BC_PSHCF: memcpy(&meg4.af, &meg4.code[pc++], 4); meg4.ac = (int)meg4.af; cpu_pushf(meg4.af); break;
        case BC_PUSHI: cpu_pushi(meg4.ac); break;
        case BC_PUSHF: cpu_pushf(meg4.af); break;
        case BC_POPI: meg4.ac = cpu_popi(); meg4.af = (float)meg4.ac; break;
        case BC_POPF: meg4.af = cpu_popf(); meg4.ac = (int)meg4.af; break;
        case BC_CNVI: if(meg4.sp >= sizeof(meg4.data) || meg4.sp <= meg4.dp) { MEG4_DEBUGGER(ERR_STACK); } else {
            memcpy(&fval, meg4.data + meg4.sp, 4); val = (int)fval; memcpy(meg4.data + meg4.sp, &val, 4); } break;
        case BC_CNVF: if(meg4.sp >= sizeof(meg4.data) || meg4.sp <= meg4.dp) { MEG4_DEBUGGER(ERR_STACK); } else {
            memcpy(&val, meg4.data + meg4.sp, 4); fval = (float)val; memcpy(meg4.data + meg4.sp, &fval, 4); } break;
        /* load */
        case BC_LDB: if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < 0) { MEG4_DEBUGGER(ERR_BOUNDS); } else {
            meg4.ac = meg4_api_inb(meg4.ac); meg4.af = (float)meg4.ac; } break;
        case BC_LDW: if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < 0) { MEG4_DEBUGGER(ERR_BOUNDS); } else {
            meg4.ac = meg4_api_inw(meg4.ac); meg4.af = (float)meg4.ac; } break;
        case BC_LDI: if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < 0) { MEG4_DEBUGGER(ERR_BOUNDS); } else {
            meg4.ac = meg4_api_ini(meg4.ac); meg4.af = (float)meg4.ac; } break;
        case BC_LDF: if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < 0) { MEG4_DEBUGGER(ERR_BOUNDS); } else {
            i = meg4_api_ini(meg4.ac); memcpy(&meg4.af, &i, 4); meg4.ac = (int)meg4.af; } break;
        /* store */
        case BC_STB: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outb(i, meg4.ac); break;
        case BC_STW: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outw(i, meg4.ac); break;
        case BC_STI: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outi(i, meg4.ac); break;
        case BC_STF: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16) { MEG4_DEBUGGER(ERR_BOUNDS); } else { memcpy(&val, &meg4.af, 4); meg4_api_outi(i, val); } break;
        /* BASIC's READ (also allow it from Assembly) */
        case BC_RDB: i = *(int*)(meg4.data + 4); val = *(int*)(meg4.data) - MEG4_MEM_USER + (i << 2);
            if((meg4.code_type != 1 && meg4.code_type != 15) || (uint32_t)i >= *(uint32_t*)(meg4.data + 8)) { MEG4_DEBUGGER(ERR_NODATA); }
            else if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < MEG4_MEM_USER || val >= (int)sizeof(meg4.data)) { MEG4_DEBUGGER(ERR_BOUNDS); }
            else { meg4_api_outb(meg4.ac, (int)(*(float*)&meg4.data[val])); (*(int*)(meg4.data + 4))++; } break;
        case BC_RDW: i = *(int*)(meg4.data + 4); val = *(int*)(meg4.data) - MEG4_MEM_USER + (i << 2);
            if((meg4.code_type != 1 && meg4.code_type != 15) || (uint32_t)i >= *(uint32_t*)(meg4.data + 8)) { MEG4_DEBUGGER(ERR_NODATA); }
            else if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < MEG4_MEM_USER || val >= (int)sizeof(meg4.data)) { MEG4_DEBUGGER(ERR_BOUNDS); }
            else { meg4_api_outw(meg4.ac, (int)(*(float*)&meg4.data[val])); (*(int*)(meg4.data + 4))++; } break;
        case BC_RDI: i = *(int*)(meg4.data + 4); val = *(int*)(meg4.data) - MEG4_MEM_USER + (i << 2);
            if((meg4.code_type != 1 && meg4.code_type != 15) || (uint32_t)i >= *(uint32_t*)(meg4.data + 8)) { MEG4_DEBUGGER(ERR_NODATA); }
            else if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < MEG4_MEM_USER || val >= (int)sizeof(meg4.data)) { MEG4_DEBUGGER(ERR_BOUNDS); }
            else { meg4_api_outi(meg4.ac, (int)(*(float*)&meg4.data[val])); (*(int*)(meg4.data + 4))++; } break;
        case BC_RDF: i = *(int*)(meg4.data + 4); val = *(int*)(meg4.data) - MEG4_MEM_USER + (i << 2);
            if((meg4.code_type != 1 && meg4.code_type != 15) || (uint32_t)i >= *(uint32_t*)(meg4.data + 8)) { MEG4_DEBUGGER(ERR_NODATA); }
            else if(meg4.ac >= MEG4_MEM_LIMIT || meg4.ac < MEG4_MEM_USER || val >= (int)sizeof(meg4.data)) { MEG4_DEBUGGER(ERR_BOUNDS); }
            else { memcpy(meg4.data + meg4.ac - MEG4_MEM_USER, meg4.data + val, 4); (*(int*)(meg4.data + 4))++; } break;
        /* increment / decrement */
        case BC_INCB: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16 || val < 1) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outb(i, meg4_api_inb(i) + val); break;
        case BC_INCW: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16 || val < 1) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outw(i, meg4_api_inw(i) + val); break;
        case BC_INCI: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16 || val < 1) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outi(i, meg4_api_ini(i) + val); break;
        case BC_DECB: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16 || val < 1) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outb(i, meg4_api_inb(i) - val); break;
        case BC_DECW: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16 || val < 1) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outw(i, meg4_api_inw(i) - val); break;
        case BC_DECI: i = cpu_popi(); if(i >= MEG4_MEM_LIMIT || i < 16 || val < 1) { MEG4_DEBUGGER(ERR_BOUNDS); } else meg4_api_outi(i, meg4_api_ini(i) - val); break;
        /* bit fiddling */
        case BC_NOT: meg4.ac = !meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_NEG: meg4.ac = ~meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_OR:  meg4.ac |= cpu_popi(); meg4.af = (float)meg4.ac; break;
        case BC_AND: meg4.ac &= cpu_popi(); meg4.af = (float)meg4.ac; break;
        case BC_XOR: meg4.ac ^= cpu_popi(); meg4.af = (float)meg4.ac; break;
        case BC_SHL: meg4.ac = cpu_popi() << meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_SHR: meg4.ac = cpu_popi() >> meg4.ac; meg4.af = (float)meg4.ac; break;
        /* comparators */
        case BC_EQ:  meg4.ac = cpu_popi() == meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_NE:  meg4.ac = cpu_popi() != meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_LTS: meg4.ac = cpu_popi() <  meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_GTS: meg4.ac = cpu_popi() >  meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_LES: meg4.ac = cpu_popi() <= meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_GES: meg4.ac = cpu_popi() >= meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_LTU: meg4.ac = (uint32_t)cpu_popi() <  (uint32_t)meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_GTU: meg4.ac = (uint32_t)cpu_popi() >  (uint32_t)meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_LEU: meg4.ac = (uint32_t)cpu_popi() <= (uint32_t)meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_GEU: meg4.ac = (uint32_t)cpu_popi() >= (uint32_t)meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_LTF: meg4.ac = cpu_popf() <  meg4.af; meg4.af = (float)meg4.ac; break;
        case BC_GTF: meg4.ac = cpu_popf() >  meg4.af; meg4.af = (float)meg4.ac; break;
        case BC_LEF: meg4.ac = cpu_popf() <= meg4.af; meg4.af = (float)meg4.ac; break;
        case BC_GEF: meg4.ac = cpu_popf() >= meg4.af; meg4.af = (float)meg4.ac; break;
        /* arithmetic operators */
        case BC_ADDI: meg4.ac = cpu_popi() +  meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_SUBI: meg4.ac = cpu_popi() -  meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_MULI: meg4.ac = cpu_popi() *  meg4.ac; meg4.af = (float)meg4.ac; break;
        case BC_DIVI: if(!meg4.ac) { MEG4_DEBUGGER(ERR_DIVZERO); } else { meg4.ac = cpu_popi() / meg4.ac; meg4.af = (float)meg4.ac; } break;
        case BC_MODI: if(!meg4.ac) { MEG4_DEBUGGER(ERR_DIVZERO); } else { meg4.ac = cpu_popi() % meg4.ac; meg4.af = (float)meg4.ac; } break;
        case BC_POWI: meg4.ac = (int)powf((float)cpu_popi(), (float)meg4.ac); meg4.af = (float)meg4.ac; break;
        case BC_ADDF: meg4.af = cpu_popf() +  meg4.af; meg4.ac = (int)meg4.af; break;
        case BC_SUBF: meg4.af = cpu_popf() -  meg4.af; meg4.ac = (int)meg4.af; break;
        case BC_MULF: meg4.af = cpu_popf() *  meg4.af; meg4.ac = (int)meg4.af; break;
        case BC_DIVF: meg4.af = cpu_popf() /  meg4.af; meg4.ac = (int)meg4.af; break;
        case BC_MODF: fval = cpu_popf() / meg4.af; meg4.af = fval - (float)((int)fval); meg4.ac = (int)meg4.af; break;
        case BC_POWF: meg4.af = powf(cpu_popf(), meg4.af); meg4.ac = (int)meg4.af; break;
    }
    /* if an error happened and mode switched to debug (or guru), then leave PC so that it points to the failed instruction */
    if(meg4.mode == MEG4_MODE_GAME) meg4.pc = pc;
    /* failsafe, never leave this function with invalid PC */
    if(meg4.pc < 4 || meg4.pc >= meg4.code[0]) meg4.pc = 0;
}
