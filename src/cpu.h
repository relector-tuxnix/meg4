/*
 * meg4/cpu.h
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
 * @brief Central Processing Unit (and some compiler types, because it also needs these bytecodes)
 *
 */

/* bytecode: mnemonic  dst, src
 * each instruction is 32 bits, lower 8 bits stores the opcode
 * IID  - in instruction data, upper 24 bits
 * IMM  - immediate, 32 bits, either signed, unsigned or float
 * ACC  - accumulator, either signed, unsigned or float
 * SP   - stack pointer
 * BP   - base pointer of the function stack frame (above parameters, below locals)
 * DP   - data pointer (top of globals)
 * []   - address, use the data in memory
 * [[]] - means that the address of the data is taken from another address */
enum {
    /* transfer control */
    BC_DEBUG,   /*                  invoke the built-in debugger (no operation in MEG-4 PRO) */
    BC_RET,     /*                  return from function */
    BC_SCALL,   /* IID              do a system call, see api.h for MEG-4 API codes */
    BC_CALL,    /* IMM              do a local function call */
    BC_JMP,     /* IMM              jump to address */
    BC_JZ,      /* IMM              jump if ACC is zero */
    BC_JNZ,     /* IMM              jump if ACC is not zero */
    BC_JS,      /* IMM              pop value, adjust with ACC's sign and jump if less than or equal to zero */
    BC_JNS,     /* IMM              pop value, adjust with ACC's sign and jump if greater than zero */
    BC_SW,      /* IID, IMM, IMM... switch, numcase in opcode, others: smallest case, default address, case addresses */
    /* immediate constants */
    BC_CI,      /* ACC, IMM         integer constant */
    BC_CF,      /* ACC, IMM         floating point constant */
    /* stack operations */
    BC_BND,     /* IID              bound check */
    BC_LEA,     /* ACC, DP + IID    load effective (DP relative) address */
    BC_ADR,     /* ACC, BP + IID    load local (BP relative) address */
    BC_SP,      /* SP,  SP + IID    adjust stack pointer */
    BC_PSHCI,   /* [SP], IMM        push an integer immediate constant */
    BC_PSHCF,   /* [SP], IMM        push a float immediate constant */
    BC_PUSHI,   /* [SP], ACC        push the integer accumulator */
    BC_PUSHF,   /* [SP], ACC        push the float accumulator */
    BC_POPI,    /* ACC, [SP]        pop an integer */
    BC_POPF,    /* ACC, [SP]        pop a float */
    BC_CNVI,    /* [SP]             convert the element on top of the stack to integer */
    BC_CNVF,    /* [SP]             convert the element on top of the stack to float */
    /* load */
    BC_LDB,     /* ACC, [ACC]       load byte */
    BC_LDW,     /* ACC, [ACC]       load word */
    BC_LDI,     /* ACC, [ACC]       load integer */
    BC_LDF,     /* ACC, [ACC]       load float */
    /* store */
    BC_STB,     /* [[SP]], ACC      store byte */
    BC_STW,     /* [[SP]], ACC      store word */
    BC_STI,     /* [[SP]], ACC      store integer */
    BC_STF,     /* [[SP]], ACC      store float */
    /* BASIC's READ */
    BC_RDB,     /* [ACC], [[30000]] read and store byte (0x30000 load from address, 0x30004 current index, 0x30008 max index) */
    BC_RDW,     /* [ACC], [[30000]] read and store word */
    BC_RDI,     /* [ACC], [[30000]] read and store integer */
    BC_RDF,     /* [ACC], [[30000]] read and store float */
    /* increment / decrement */
    BC_INCB,    /* [[SP]], IID      increment byte */
    BC_INCW,    /* [[SP]], IID      increment word */
    BC_INCI,    /* [[SP]], IID      increment integer */
    BC_DECB,    /* [[SP]], IID      decrement byte */
    BC_DECW,    /* [[SP]], IID      decrement word */
    BC_DECI,    /* [[SP]], IID      decrement integer */
    /* bit fiddling */
    BC_NOT,     /* ACC              logical not */
    BC_NEG,     /* ACC              bitwise not (negate, same as XOR -1) */
    BC_OR,      /* [SP], ACC        bitwise or */
    BC_XOR,     /* [SP], ACC        bitwise xor */
    BC_AND,     /* [SP], ACC        bitwise and */
    BC_SHL,     /* [SP], ACC        shift left */
    BC_SHR,     /* [SP], ACC        shift right */
    /* comparators */
    BC_EQ,      /* [SP], ACC        equal */
    BC_NE,      /* [SP], ACC        not equal */
    BC_LTS,     /* [SP], ACC        less than (signed) */
    BC_GTS,     /* [SP], ACC        greater than (signed) */
    BC_LES,     /* [SP], ACC        less than or equal (signed) */
    BC_GES,     /* [SP], ACC        greater than or equal (signed) */
    BC_LTU,     /* [SP], ACC        less than (unsigned) */
    BC_GTU,     /* [SP], ACC        greater than (unsigned) */
    BC_LEU,     /* [SP], ACC        less than or equal (unsigned) */
    BC_GEU,     /* [SP], ACC        greater than or equal (unsigned) */
    BC_LTF,     /* [SP], ACC        less than (float) */
    BC_GTF,     /* [SP], ACC        greater than (float) */
    BC_LEF,     /* [SP], ACC        less than or equal (float) */
    BC_GEF,     /* [SP], ACC        greater than or equal (float) */
    /* arithmetic operators */
    BC_ADDI,    /* [SP], ACC        integer addition */
    BC_SUBI,    /* [SP], ACC        integer subtraction */
    BC_MULI,    /* [SP], ACC        integer multiplication */
    BC_DIVI,    /* [SP], ACC        integer division */
    BC_MODI,    /* [SP], ACC        integer modulus */
    BC_POWI,    /* [SP], ACC        integer exponent (power of) */
    BC_ADDF,    /* [SP], ACC        float addition */
    BC_SUBF,    /* [SP], ACC        float subtraction */
    BC_MULF,    /* [SP], ACC        float multiplication */
    BC_DIVF,    /* [SP], ACC        float division */
    BC_MODF,    /* [SP], ACC        float modulus */
    BC_POWF,    /* [SP], ACC        float exponent (power of) */

    BC_LAST
};

/* identifier types */
enum { T_DEF, T_STRUCT, T_FLD, T_LABEL, T_FUNC, T_FUNCP, T_SCALAR, T_PTR, T_PTR2, T_PTR3, T_PTR4 };
/* must match the types in misc/hl.json */
enum { T_VOID, T_FLOAT, T_I8, T_I16, T_I32, T_U8, T_U16, T_U32, T_STR, T_ADDR,          /* common to all languages */
    C_CHAR, C_SHORT, C_INT, C_SIGNED, C_UNSIGNED, C_ENUM, C_STRUCT, C_UNION, C_TYPEDEF  /* C specific */ };
#define T(a,b)  (((b)<<4)|(a))
/* operators in precedence order, compiler uses the highlighter tokens except for these as it needs a language independent list
 * NOTE: here exact order and aligning with BC_ enums above within one group DOES matter */
enum {
    /* assignments */   O_AOR, O_AAND, O_AXOR, O_ASHL, O_ASHR, O_AADD, O_ASUB, O_AMUL, O_ADIV, O_AMOD, O_AEXP, O_LET,
    /* logical */       O_CND, O_LOR, O_LAND,
    /* bitwise */       O_BOR, O_XOR, O_BAND,
    /* conditionals */  O_EQ, O_NE, O_LT, O_GT, O_LE, O_GE,
    /* arithmetic */    O_SHL, O_SHR, O_ADD, O_SUB, O_MUL, O_DIV, O_MOD, O_EXP,
    /* unaries */       O_NOT, O_BNOT, O_INC, O_DEC,
    /* field select */  O_FLD, O_DFLD,
    O_LAST
};

#define N_EXPR  256             /* longest expression allowed, in tokens */
#define N_DIM 4                 /* number of supported array dimensions */
#define N_ARG 32                /* number of function arguments supported */

#ifdef MEG4_EDITORS
typedef struct {
    char name[256];             /* name of the identifier */
    int t, s, l;                /* type, size, length (with scalars, size == length) */
    int a[N_DIM], f[N_DIM];     /* array dimensions, first indeces */
    int p, o;                   /* position where it was defined, field offset */
    int r;                      /* number of references */
} idn_t;

typedef struct {
    char *name;                 /* name of the built-in define */
    int val;                    /* its value */
} bdef_t;

typedef struct {
    char str[256];              /* unescaped string */
    int l, n;                   /* length, number of references / offset */
} cstr_t;

typedef struct {
    int id, p, n, t[N_ARG];     /* identifier, position of declaration, number of arguments, argument types */
} func_t;

typedef struct {
    char ***r;                  /* language rules */
    int ntok;                   /* number of tokens */
    tok_t *tok;                 /* tokens, see highlighter in editors/editors.h */
    int nid, ng;                /* number of identifiers, number of globals */
    idn_t *id;                  /* identifiers */
    char **ops;                 /* operators in precedence order, must match O_ defines */
    int nsct;                   /* number of structs */
    int *structs;               /* struct members, each block N_ARG long and these are identifiers */
    int nstr;                   /* number of strings */
    cstr_t *str;                /* strings */
    int nf, cf, pf, lf, ls, lc; /* number of functions, current function, last parameter id, nested level, last statement, last constant */
    func_t *f;                  /* functions */
    int ncd, acd, *cd;          /* number of code debug records, allocated records, debug records */
    int ndd, add, *dd;          /* number of data debug records, allocated records, debug records */
    int nc, ac;                 /* number of code, allocated code */
    uint32_t *code;             /* bytecode */
} compiler_t;

/* helper for debug */
void comp_dump(compiler_t *comp, int s);

/* common compiler functions */
int  comp_const(compiler_t *comp, int s, int type, int *i, float *f);
int  comp_findid(compiler_t *comp, tok_t *tok);
int  comp_addid(compiler_t *comp, tok_t *tok, int type);
int  comp_addstr(compiler_t *comp, int t);
void comp_addhdr(compiler_t *comp);
int  comp_addinit(compiler_t *comp);
int  comp_addfunc(compiler_t *comp, int id, int s, int nargs, int *argtypes);
int  comp_addlbl(compiler_t *comp, int s);
int  comp_addjmp(compiler_t *comp, int s);
int  comp_chkids(compiler_t *comp, int s);
int  comp_eval(compiler_t *comp, int s, int e, int *val);
int  comp_expr(compiler_t *comp, int s, int e, int *type, int *end, int lvl);
void comp_resolve(compiler_t *comp, int i);
void comp_load(compiler_t *comp, int t);
void comp_store(compiler_t *comp, int t);
void comp_push(compiler_t *comp, int t);
void comp_cmp(compiler_t *comp, int op, int t1, int t2);
void comp_cdbg(compiler_t *comp, int s);
void comp_ddbg(compiler_t *comp, int s, int id);
void comp_gen(compiler_t *comp, int inst);

/* comp_c.c - C */
int  comp_c(compiler_t *comp);

/* comp_bas.c - BASIC */
int  comp_bas(compiler_t *comp);

/* comp_asm.c - Assembly */
int  comp_asm(compiler_t *comp);
#endif

/* comp_lua.c - Lua */
void comp_lua_init(void);
void comp_lua_free(void);
int  comp_lua(char *str, int len);
void cpu_lua(void);
