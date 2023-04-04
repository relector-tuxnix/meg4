/*
 * meg4/comp_lua.c
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
 * @brief Lua "compiler" and runner
 *
 */

#include "meg4.h"

/* we might need Lua even without editors */
#if LUA
#include "editors/editors.h"    /* for cpu defines and code_error() */
#include "lang.h"
#ifdef NOEDITORS
#define LUA_ERR_NUMARG "wrong number of arguments"
#define LUA_ERR_BADARG "bad argument type"
#define LUA_ERR_MEMORY "out of memory"
#define LUA_ERR_BADSYS "bad system call"
#define BDEF_IMPL
#else
#define LUA_ERR_NUMARG lang[ERR_NUMARG]
#define LUA_ERR_BADARG lang[ERR_BADARG]
#define LUA_ERR_MEMORY lang[ERR_MEMORY]
#define LUA_ERR_BADSYS lang[ERR_BADSYS]
#endif
#include "api.h"
#include "lua/lualib.h"
#include "lua/lprefix.h"
#include "lua/ldebug.h"
#include "lua/lauxlib.h"
#include "lua/lundump.h"
#include "lua/lobject.h"
#include <setjmp.h>             /* yes, Lua is really so messed up that we need this */

extern bdef_t meg4_bdefs[MEG4_NUM_BDEF + 1];
int str_format (lua_State *L);  /* I honestly don't know why isn't this function exposed to the Lua C API by default */
static jmp_buf jmpbuf;
static int continuity(lua_State *L, int a, long int b);
static lua_State *L = NULL, *S = NULL;
static size_t written = 0;
static uint8_t *luabuf = NULL, state = 0;
/* states:
 * 0 - parse globals in main (and statements in main as if they were in setup)
 * 1 - run setup() one time (if exists)
 * 2 - run loop() multiple times (if exists)
 * 3 - don't do anything, because Lua truly went fishing
 */

/**
 * Common Lua API callback
 */
static int callback(lua_State *L)
{
    meg4_api_t *api;
    size_t j, l;
    char *s;
    uint32_t addr = 0, dst = 0;
    float fval;
    int i, val, ret = 0, n = lua_gettop(L), idx = lua_tointeger(L, lua_upvalueindex(1));

    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    if(idx >= 0 && idx < MEG4_NUM_API) {
        api = &meg4_api[idx];
        /* do not rely on Lua error checks, do it ourselves and provide our own messages
         * whenever possible (because our messages are translated, unlike Lua's) */
        if(n != api->narg) {
            state = 3;
            luaL_error(L, "MEG-4 API %s: %s", api->name, LUA_ERR_NUMARG);
            return 0;
        }
        /* yeah, Lua's stack is upside-down */
        for(i = n - 1; i >= 0; i--) {
            /* check argument types and fill in stack */
            if(api->smsk & (1 << i)) {
                /* string */
                if(!lua_isstring(L, i + 1)) {
badarg:             state = 3;
                    luaL_error(L, "MEG-4 API %s:arg %u: %s", api->name, i + 1, LUA_ERR_BADARG);
                    return 0;
                }
                s = (char*)luaL_tolstring(L, i + 1, &l);
                if(meg4.dp + l + 5 >= meg4.sp) {
memory:             state = 3;
                    luaL_error(L, "MEG-4 API %s:arg %u: %s", api->name, i + 1, LUA_ERR_MEMORY);
                    return 0;
                } else {
                    cpu_pushi(meg4.dp + MEG4_MEM_USER);
                    memcpy(meg4.data + meg4.dp, s, l);
                    meg4.dp += l;
                    meg4.data[meg4.dp++] = 0;
                }
            } else
            if(api->amsk & (1 << i)) {
                /* memory address, could be a real address or a byte array as well */
                j = 1;
                if(!strcmp(api->name, "remap")) j = 2; else
                if(!strcmp(api->name, "memsave")) j = 3;
                if(lua_isinteger(L, i + 1) || lua_isnumber(L, i + 1)) {
                    /* real address */
                    if(lua_isinteger(L, i + 1)) addr = (uint32_t)lua_tointeger(L, i + 1);
                    else addr = (uint32_t)lua_tonumber(L, i + 1);
                    if(!(j & 1) || addr >= MEG4_MEM_LIMIT - 256) goto badarg;
                    if(meg4.dp + 4 >= meg4.sp) goto memory;
                    cpu_pushi(addr);
                    if(!strcmp(api->name, "memload")) dst = addr;
                } else
                if(lua_istable(L, i + 1)) {
                    /* byte array, just it's a table... */
                    l = lua_rawlen(L, i + 1);
                    /* This is from the official documentation, PIL section 27.1 Array Manipulation.
                     * Of course the function DOESN'T EVEN EXISTS... I mean it's not that I forget to include
                     * the header; grep it, there's no "luaL_getn" string in the entire Lua source code */
                    /* l = luaL_getn(L, i + 1); */
                    if(!(j & 2) || (j == 2 && l != 256)) goto badarg;
                    if(meg4.dp + l + 4 >= meg4.sp) goto memory;
                    cpu_pushi(meg4.dp + MEG4_MEM_USER);
                    /* this is the least efficient solution there could be. But we have no choice, Lua
                     * does not store array *khm* table elements as continuous values in memory... */
                    for(j = 0; j < l; j++) {
                        lua_rawgeti(L, i + 1, j + 1);
                        if(lua_isinteger(L, -1)) meg4.data[meg4.dp++] = (uint8_t)lua_tointeger(L, -1);
                        else goto badarg;
                        lua_pop(L, 1);
                    }
                } else goto badarg;
            } else
            if(api->fmsk & (1 << i)) {
                /* floating point */
                if(meg4.dp + 4 >= meg4.sp) goto memory;
                if(lua_isnumber(L, i + 1)) cpu_pushf(lua_tonumber(L, i + 1)); else
                if(lua_isinteger(L, i + 1)) cpu_pushf((float)lua_tointeger(L, i + 1)); else
                    goto badarg;
            } else {
                /* integer */
                if(meg4.dp + 4 >= meg4.sp) goto memory;
                if(lua_isinteger(L, i + 1)) cpu_pushi(lua_tointeger(L, i + 1)); else
                if(lua_isnumber(L, i + 1)) cpu_pushi((int)lua_tonumber(L, i + 1)); else
                    goto badarg;
            }
        }
        lua_pop(L, n);
        val = 0; fval = 0;
        /* call the MEG-4 API */
        switch(idx) {
            MEG4_DISPATCH
        }
        addr = (uint32_t)val;
        /* if the call wasn't blocking, get return value and push it to Lua stack */
        ret = 0;
        if(!(meg4.flg & 14)) {
            switch(api->ret) {
                case 1:
                    /* dirty hack: for memload, do not return the number of bytes loaded, rather create a table with the data */
                    if(!strcmp(api->name, "memload")) {
                        lua_newtable(L);
                        for(i = 0; i < val; i++) {
                            lua_pushinteger(L, meg4_api_inb(dst + i));  /* value */
                            lua_rawseti(L, -2, i + 1);                  /* key */
                        }
                    } else
                        lua_pushinteger(L, val);
                    ret = 1;
                break;
                case 2: lua_pushinteger(L, val); ret = 1; break;
                case 3: lua_pushstring(L, (char*)meg4.data + addr - MEG4_MEM_USER); ret = 1; break;
                case 4: lua_pushnumber(L, fval); ret = 1; break;
            }
        } else
            return lua_yieldk(L, 0, 0, (lua_KFunction)continuity);
    } else {
        state = 3;
        luaL_error(L, "MEG-4 API: %s", LUA_ERR_BADSYS);
    }
    return ret;
}

/**
 * Dealing a bit more of Lua's insanity
 */
static int continuity(lua_State *L, int a, long int b)
{
    (void)a; (void)b;
    return callback(L);
}

/**
 * Lua compatible print
 */
static int luab_print(lua_State *L)
{
    uint32_t c, lc = 0;
    size_t l;
    char *s, *e;
    int i, n = lua_gettop(L);

    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    /* Lua's print does not use printf format strings like MEG-4's print, so we have to mimic Lua's behaviour */
    for(i = 0; i < n; i++) {
        s = (char*)luaL_tolstring(L, i + 1, &l);
        if(s) {
            if(i) meg4_putc('\t');
            for(e = s + l; s < e;) {
                s = meg4_utf8(s, &c);
                lc = c;
                if(!c) break;
                meg4_putc(c);
            }
        }
        lua_pop(L, 1);
    }
    /* Lua print also forces a newline at the end */
    if(lc != '\n') meg4_putc('\n');
    return 0;
}

/**
 * Lua printf
 */
static int luab_printf(lua_State *L)
{
    uint32_t c;
    size_t l;
    char *s, *e;

    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    str_format(L);
    for(s = (char*)lua_tolstring(L, -1, &l), e = s + l; s < e && *s;) {
        s = meg4_utf8(s, &c);
        if(!c) break;
        meg4_putc(c);
    }
    lua_pop(L, 1);
    return 0;
}

/**
 * Lua sprintf
 */
static int luab_sprintf(lua_State *L)
{
    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    str_format(L);
    return 1;
}

/**
 * Lua trace
 */
static int luab_trace(lua_State *L)
{
    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    str_format(L);
    main_log(1, "trace: %s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

/**
 * Lua memcpy, one of the parameters could be a Lua table
 */
static int luab_memcpy(lua_State *L)
{
    int addr = 0, len = 0, i, l, err = 3;

    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    if(lua_gettop(L) != 3) {
        state = 3;
        luaL_error(L, "MEG-4 API %s: %s", "memcpy", LUA_ERR_NUMARG);
        return 0;
    }
    /* check length first */
    if(!lua_isinteger(L, 3) && !lua_isnumber(L, 3)) {
badarg: state = 3;
        luaL_error(L, "MEG-4 API %s:arg %u: %s", "memcpy", err, LUA_ERR_BADARG);
        return 0;
    } else {
        if(lua_isinteger(L, 3)) len = lua_tointeger(L, 3);
        else len = (int)lua_tonumber(L, 3);
        if(len < 1 || len >= MEG4_MEM_LIMIT) goto badarg;
    }
    /* simple case, if both source and dest are memory addresses */
    if((lua_isinteger(L, 1) || lua_isnumber(L, 1)) && (lua_isinteger(L, 2) || lua_isnumber(L, 2))) {
        if(lua_isinteger(L, 1)) i = lua_tointeger(L, 1); else i = (int)lua_tonumber(L, 1);
        if(lua_isinteger(L, 2)) l = lua_tointeger(L, 2); else l = (int)lua_tonumber(L, 2);
        if(i < 0 || i >= MEG4_MEM_LIMIT) { err = 1; goto badarg; }
        if(l < 0 || l >= MEG4_MEM_LIMIT) { err = 2; goto badarg; }
        meg4_api_memcpy((uint32_t)i, (uint32_t)l, len);
    } else
    /* if source is a table */
    if((lua_isinteger(L, 1) || lua_isnumber(L, 1)) && lua_istable(L, 2)) {
        err = 2;
        if(lua_isinteger(L, 1)) addr = lua_tointeger(L, 1); else addr = (int)lua_tonumber(L, 1);
        if(addr < 0 || addr >= MEG4_MEM_LIMIT) goto badarg;
        l = lua_rawlen(L, 2);
        if(l < len) len = l;
        if(addr + len >= MEG4_MEM_LIMIT) len = MEG4_MEM_LIMIT - addr;
        for(i = 0; i < len; i++) {
            lua_rawgeti(L, 2, i + 1);
            if(lua_isinteger(L, -1)) meg4_api_outb(addr + i, (uint8_t)lua_tointeger(L, -1));
            else goto badarg;
            lua_pop(L, 1);
        }
    } else
    /* if dest is a table */
    if(lua_istable(L, 1) && (lua_isinteger(L, 2) || lua_isnumber(L, 2))) {
        err = 1;
        if(lua_isinteger(L, 2)) addr = lua_tointeger(L, 2); else addr = (int)lua_tonumber(L, 2);
        if(addr < 0 || addr >= MEG4_MEM_LIMIT) goto badarg;
        l = lua_rawlen(L, 1);
        if(l < len) len = l;
        if(addr + len >= MEG4_MEM_LIMIT) len = MEG4_MEM_LIMIT - addr;
        lua_newtable(L);
        for(i = 0; i < len; i++) {
            lua_pushinteger(L, meg4_api_inb(addr + i)); /* value */
            lua_rawseti(L, -2, i + 1);                  /* key */
        }
        return 1;
    } else {
        err = !lua_isinteger(L, 1) && !lua_isnumber(L, 1) && !lua_istable(L, 1) ? 1 : 2;
        goto badarg;
    }
    return 0;
}

/**
 * Lua maze, the NPC list could be one Lua table instead of the numnpc+npcaddr arguments
 */
static int luab_maze(lua_State *L)
{
    int i, l;
    uint32_t par[12], *data;

    meg4.pc = L->ci && isLua(L->ci) ? pcRel(L->ci->u.l.savedpc, ci_func(L->ci)->p) : 0;
    meg4.sp = meg4.bp = sizeof(meg4.data) - 256; meg4.dp = 0;
    l = lua_gettop(L);
    if(l != 11 && l != 12) {
        state = 3;
        luaL_error(L, "MEG-4 API %s: %s", "maze", LUA_ERR_NUMARG);
        return 0;
    }
    /* hush, gcc */
    par[11] = 0;
    /* check integer parameters first */
    for(i = 0; i < 10; i++) {
        if(!lua_isinteger(L, i + 1) && !lua_isnumber(L, i + 1)) {
badarg:     state = 3;
            luaL_error(L, "MEG-4 API %s:arg %u: %s", "maze", i + 1, LUA_ERR_BADARG);
            return 0;
        } else {
            if(lua_isinteger(L, i + 1)) par[i] = (uint32_t)lua_tointeger(L, i + 1);
            else par[i] = (uint32_t)lua_tonumber(L, i + 1);
        }
    }
    /* if NPC list is a num+address pair */
    if(l == 12 && (lua_isinteger(L, 11) || lua_isnumber(L, 11)) && (lua_isinteger(L, 12) || lua_isnumber(L, 12))) {
        if(lua_isinteger(L, 11)) par[10] = (uint32_t)lua_tointeger(L, 11);
        else par[10] = (uint32_t)lua_tonumber(L, 11);
        if(par[11] >= meg4.sp / 12) { i = 10; goto badarg; }
        if(lua_isinteger(L, 12)) par[11] = (uint32_t)lua_tointeger(L, 12);
        else par[11] = (uint32_t)lua_tonumber(L, 12);
        if(par[11] >= MEG4_MEM_LIMIT - 256) { i = 11; goto badarg; }
    } else
    /* if NPC list is a table */
    if(l == 11 && lua_istable(L, 11)) {
        par[10] = l = lua_rawlen(L, 11);
        if(l > (int)meg4.sp / 12) { i = 10; goto badarg; }
        par[11] = MEG4_MEM_USER;
        data = (uint32_t*)meg4.data;
        lua_rawgeti(L, 2, 1);
        /* if argument is a flat table with 3 times integer values */
        if(lua_isinteger(L, -1) && !(l % 3)) {
            lua_pop(L, 1);
            par[10] /= 3;
            for(i = 0; i < l; i++) {
                lua_rawgeti(L, 2, i + 1);
                if(lua_isinteger(L, -1)) *data++ = (uint32_t)lua_tointeger(L, -1);
                else { i = 10; goto badarg; }
                lua_pop(L, 1);
            }
        } else
        /* if argument is a table with tables */
        if(lua_istable(L, -1)) {
            lua_pop(L, 1);
            for(i = 0; i < l; i++) {
                lua_rawgeti(L, 2, i + 1);
                if(lua_istable(L, -1)) {
                    lua_rawgeti(L, -1, 1);
                    *data++ = (uint32_t)lua_tointeger(L, -1); /* x */
                    lua_pop(L, 1);
                    lua_rawgeti(L, -1, 2);
                    *data++ = (uint32_t)lua_tointeger(L, -1); /* y */
                    lua_pop(L, 1);
                    lua_rawgeti(L, -1, 3);
                    *data++ = (uint32_t)lua_tointeger(L, -1); /* id */
                    lua_pop(L, 1);
                } else { i = 10; goto badarg; }
                lua_pop(L, 1);
            }
        } else { lua_pop(L, 1); i = 10; goto badarg; }
    } else { i = 10; goto badarg; }
    meg4_api_maze(par[0], par[1], par[2], par[3], par[4], par[5], par[6], par[7], par[8], par[9], par[10], par[11]);
    return 0;
}

/**
 * Lua bytecode writer
 */
static int writer(lua_State *L, const void *p, size_t size, void *u)
{
    (void)L; (void)u;
    luabuf = (uint8_t*)realloc(luabuf, written + size);
    if(luabuf) { memcpy(luabuf + written, p, size); written += size; } else written = 0;
    return 0;
}

/**
 * Get error from Lua
 */
static void geterror(void)
{
#ifndef NOEDITORS
    lua_Debug ar;
    int i = 0, l;
    char *s, *e = (char*)lua_tostring(L, -1);

    ar.currentline = 0;
    if(meg4.src && meg4.src_len > 6) {
        /* It's not just that such a should-be-obvious-and-simple thing is so overcomplicated, but lua_getinfo segfaults... */
/*
        lua_getstack(L, 0, &ar); lua_getinfo(L, "Sl", &ar); <-- segfaults
        lua_getstack(L, 1, &ar); lua_getinfo(L, "Sl", &ar); <-- this segfaults too
        for(i = -16; i < 16; i++)
            if(lua_getstack(L, i, &ar)) { lua_getinfo(L, "Sl", &ar); break; }
printf("i %d ar.cl %d line %d\n",i,ar.currentline,ar.linedefined + ar.currentline);
printf("cl %d\n", luaG_getfuncline(ci_func(L->ci)->p, currentpc(L->ci)));
*/
        /* I give up, it is literally impossible to get the faulting line from this big pile of crap. The workaround is
         * ugly, ineffective, but at least it works... as long as the first part of the message is enclosed in [] */
        if(e && (s = strstr(e, "]:"))) ar.currentline = atoi(s + 2);
        if(ar.currentline > 0)
            for(l = i = 0; i < (int)meg4.src_len && meg4.src[i] && l != ar.currentline; i++)
                if(meg4.src[i] == '\n') l++;
    }
    code_error(i < 6 ? 6 : i, e ? e : "Lua ERROR");
#else
    meg4.mode = MEG4_MODE_GURU;
#endif
    meg4.flg |= 8;
}

/**
 * Custom Lua panic handler, otherwise this messed up library would call abort() on us. Not kidding, it really would.
 */
static int panic(lua_State *L)
{
    (void)L;
    state = 3;
    geterror();
    longjmp(jmpbuf, 1);
    return 0;
}

/**
 * Initialize Lua
 */
void comp_lua_init(void)
{
    int i;

    /* already initialized? */
    if(S) return;
    S = luaL_newstate();
    if(S) {
        lua_atpanic(S, &panic);
        /* this must be stripped, no doFile nor require in baselib, no coroutine, io, os nor package modules either */
        luaL_openlibs(S);
        /* we want this to use our meg4_putc() and not libc stdout */
        lua_pushcclosure(S, &luab_print, 0);
        lua_setglobal(S, "print");
        for(i = 0; i < MEG4_NUM_API; i++) {
            /* handle special cases as much as we can. Only for callbacks that can't yield though */
            if(!strcmp(meg4_api[i].name, "printf"))  lua_pushcclosure(S, &luab_printf, 0); else
            if(!strcmp(meg4_api[i].name, "sprintf")) lua_pushcclosure(S, &luab_sprintf, 0); else
            if(!strcmp(meg4_api[i].name, "trace"))   lua_pushcclosure(S, &luab_trace, 0); else
            if(!strcmp(meg4_api[i].name, "memcpy"))  lua_pushcclosure(S, &luab_memcpy, 0); else
            if(!strcmp(meg4_api[i].name, "maze"))    lua_pushcclosure(S, &luab_maze, 0); else {
                /* for everything else, use the common API handler */
                lua_pushinteger(S, i);
                lua_pushcclosure(S, &callback, 1);
            }
            lua_setglobal(S, meg4_api[i].name);
        }
        for(i = 6; i < MEG4_NUM_BDEF; i++) {
            lua_pushinteger(S, meg4_bdefs[i].val);
            lua_setglobal(S, meg4_bdefs[i].name);
        }
    }
}

/**
 * Free resources
 */
void comp_lua_free(void)
{
    if(S) { lua_close(S); L = S = NULL; }
}

/**
 * Compile source to bytecode (or compile from bytecode, crazy Lua)
 */
int comp_lua(char *str, int len)
{
    int ret = 1;

    state = 0;
    written = 0; luabuf = NULL;
    if(str && len > 0 && S && setjmp(jmpbuf) == 0) {
        L = lua_newthread(S);
        if(L) lua_atpanic(L, &panic);
        if(!L || (luaL_loadbuffer(L, str, len, "main") != LUA_OK)) {
            geterror();
            ret = 0;
        } else {
            if(str == (char*)meg4.code) return 1; else
            if(lua_dump(L, writer, NULL, 0) != LUA_OK) {
#ifndef NOEDITORS
                /* don't call geterror(), no line number for sure this time */
                code_error(0, lua_tostring(L, -1));
#endif
                state = 3;
                written = ret = 0; if(luabuf) { free(luabuf); luabuf = NULL; }
            }
        }
    }
    if(meg4.code) free(meg4.code);
    /* lopcodes.h suggests `written` is multiple of 4, but I don't trust Lua and no Lua VM specification exists to be sure... */
    if(luabuf && (written & 3)) {
        luabuf = (uint8_t*)realloc(luabuf, written + 3);
        if(luabuf) { memset(luabuf + written, 0, 3); } else written = 0;
    }
    meg4.code = (uint32_t*)luabuf; meg4.code_len = (written + 3) >> 2;
    written = 0; luabuf = NULL;
    return ret;
}

/**
 * Run Lua script
 */
void cpu_lua(void)
{
    int n;

    if(state > 2) { meg4.flg |= 8; return; }
    if(L && setjmp(jmpbuf) == 0) {
        if(!state || (meg4.flg & 2) || (meg4.flg & 4)) {
            /* interpreting globals, or continuing suspended script (no function push on stack) */
            switch(lua_resume(L, S, 0, &n)) {
                case LUA_OK: if(state != 2) state++; break;
                case LUA_YIELD: return;
                default: geterror(); return;
            }
        }
        /* run the specified function */
        if(state > 1) meg4.flg |= 1;
        lua_getglobal(L, (meg4.flg & 1) ? "loop" : "setup");
        if(!lua_isfunction(L, -1)) { state++; lua_pop(L, 1); }
        else {
            /* Seriously, fuck Lua. This panics with "unprotected error in call" when the handler
             * calls yieldk. Absolutely NOT what the doc says about how lua_pcallk supposed to
             * work (you see, there's a "p" in the name, "p" as in "protected"...) */
/*          switch(lua_pcallk(L, 0, 0, 0, 0, &continuity)) {*/
            switch(lua_resume(L, S, 0, &n)) {
                case LUA_OK: state = 2; break;
                case LUA_YIELD: break;
                default: geterror(); break;
            }
        }
    }
}

#endif /* LUA */
