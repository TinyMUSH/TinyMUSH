/**
 * @file functions.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Declarations for MUSHCode functions & function processing
 * @version 3.3
 * @date 2021-01-03
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

/**
 * @brief Type definitions.
 * 
 */

#define MAX_NFARGS 30

typedef struct fun
{
    const char *name;   /*!< Function name */
    void (*fun)();      /*!< Handler */
    int nargs;          /*!< Number of args needed or expected */
    unsigned int flags; /*!< Function flags */
    int perms;          /*!< Access to function */
    EXTFUNCS *xperms;   /*!< Extended access to function */
} FUN;

typedef struct ufun
{
    char *name;         /*!< Function name */
    dbref obj;          /*!< Object ID */
    int atr;            /*!< Attribute ID */
    unsigned int flags; /*!< Function flags */
    int perms;          /*!< Access to function */
    struct ufun *next;  /*!< Next ufun in chain */
} UFUN;

typedef struct delim
{
    size_t len;
    char str[MAX_DELIM_LEN];
} Delim;

typedef struct var_entry
{
    char *text; /*!< variable text */
} VARENT;

typedef struct component_def
{
    int (*typer_func)(); /*!< type-checking handler */
    char *def_val;       /*!< default value */
} COMPONENT;

typedef struct structure_def
{
    char *s_name;        /*!< name of the structure */
    char **c_names;      /*!< array of component names */
    COMPONENT **c_array; /*!< array of pointers to components */
    int c_count;         /*!< number of components */
    char delim;          /*!< output delimiter when unloading */
    int need_typecheck;  /*!< any components without types of any? */
    int n_instances;     /*!< number of instances out there */
    char *names_base;    /*!< pointer for later freeing */
    char *defs_base;     /*!< pointer for later freeing */
} STRUCTDEF;

typedef struct instance_def
{
    STRUCTDEF *datatype; /*!< pointer to structure data type def */
} INSTANCE;

typedef struct data_def
{
    char *text;
} STRUCTDATA;

typedef struct object_stack OBJSTACK;
struct object_stack
{
    char *data;
    OBJSTACK *next;
};

typedef struct object_grid
{
    int rows;
    int cols;
    char ***data;
} OBJGRID;

typedef struct object_xfuncs
{
    NAMEDFUNC **func;
    int count;
} OBJXFUNCS;

/**
 * @brief Constants used in delimiter macros.
 * 
 */
#define DELIM_EVAL 0x001   /*!< Must eval delimiter. */
#define DELIM_NULL 0x002   /*!< Null delimiter okay. */
#define DELIM_CRLF 0x004   /*!< '%r' delimiter okay. */
#define DELIM_STRING 0x008 /*!< Multi-character delimiter okay. */
/**
 * @brief Function-specific flags used in the function table.
 * 
 * from handle_sort (sort, isort): 
 * 
 */
#define SORT_OPER 0x0f /*!< mask to select sort operation bits */
#define SORT_ITEMS 0
#define SORT_POS 1
/** 
 * from handle_sets (setunion, setdiff, setinter, lunion, ldiff, linter):
 * 
 */
#define SET_OPER 0x0f /*!< mask to select set operation bits */
#define SET_UNION 0
#define SET_INTERSECT 1
#define SET_DIFF 2
#define SET_TYPE 0x10 /*!< set type is given, don't autodetect */
/** 
 * from process_tables (tables, rtables, ctables):
 * from perform_border (border, rborder, cborder):
 * from perform_align (align, lalign):
 * 
 */
#define JUST_TYPE 0x0f /*!< mask to select justification bits */
#define JUST_LEFT 0x01
#define JUST_RIGHT 0x02
#define JUST_CENTER 0x04
#define JUST_REPEAT 0x10
#define JUST_COALEFT 0x20
#define JUST_COARIGHT 0x40
/**
 * from handle_logic (and, or, andbool, orbool, land, lor, landbool, lorbool,
 * cand, cor, candbool, corbool, xor, xorbool):
 * from handle_flaglists (andflags, orflags):
 * from handle_filter (filter, filterbool):
 * 
 */
#define LOGIC_OPER 0x0f /*!< mask to select boolean operation bits */
#define LOGIC_AND 0
#define LOGIC_OR 1
#define LOGIC_XOR 2
#define LOGIC_BOOL 0x10 /*!< interpret operands as boolean, not int */
#define LOGIC_LIST 0x40 /*!< operands come in a list, not separately */
/** 
 * from handle_vectors (vadd, vsub, vmul, vdot):
 * 
 */
#define VEC_OPER 0x0f /*!< mask to select vector operation bits */
#define VEC_ADD 0
#define VEC_SUB 1
#define VEC_MUL 2
#define VEC_DOT 3
#define VEC_CROSS 4 /*!< not implemented */
#define VEC_OR 7
#define VEC_AND 8
#define VEC_XOR 9
/** 
 * from handle_vector (vmag, vunit):
 * 
 */
#define VEC_MAG 5
#define VEC_UNIT 6
/** 
 * from perform_loop (loop, parse):
 * from perform_iter (list, iter, whentrue, whenfalse, istrue, isfalse):
 * 
 */
#define BOOL_COND_TYPE 0x0f   /*!< mask to select exit-condition bits */
#define BOOL_COND_NONE 1      /*!< loop until end of list */
#define BOOL_COND_FALSE 2     /*!< loop until true */
#define BOOL_COND_TRUE 3      /*!< loop until false */
#define FILT_COND_TYPE 0x0f0  /*!< mask to select filter bits */
#define FILT_COND_NONE 0x010  /*!< show all results */
#define FILT_COND_FALSE 0x020 /*!< show only false results */
#define FILT_COND_TRUE 0x030  /*!< show only true results */
#define LOOP_NOTIFY 0x100     /*!< send loop results directly to enactor */
#define LOOP_TWOLISTS 0x200   /*!< process two lists */
/** 
 * from handle_okpres (hears, moves, knows): 
 * 
 */
#define PRESFN_OPER 0x0f  /*!< Mask to select bits */
#define PRESFN_HEARS 0x01 /*!< Detect hearing */
#define PRESFN_MOVES 0x02 /*!< Detect movement */
#define PRESFN_KNOWS 0x04 /*!< Detect knows */
/** 
 * from perform_get (get, get_eval, xget, eval(a,b)):
 * 
 */
#define GET_EVAL 0x01  /*!< evaluate the attribute */
#define GET_XARGS 0x02 /*!< obj and attr are two separate args */
/** 
 * from handle_pop (pop, peek, toss):
 * 
 */
#define POP_PEEK 0x01 /*!< don't remove item from stack */
#define POP_TOSS 0x02 /*!< don't display item from stack */
/** 
 * from perform_regedit (regedit, regediti, regeditall, regeditalli):
 * from perform_regparse (regparse, regparsei):
 * from perform_regrab (regrab, regrabi, regraball, regraballi):
 * from perform_regmatch (regmatch, regmatchi):
 * from perform_grep (grep, grepi, wildgrep, regrep, regrepi):
 * 
 */
#define REG_CASELESS 0x01  /*!< XXX must equal PCRE_CASELESS */
#define REG_MATCH_ALL 0x02 /*!< operate on all matches in a list */
#define REG_TYPE 0x0c      /*!< mask to select grep type bits */
#define GREP_EXACT 0
#define GREP_WILD 4
#define GREP_REGEXP 8
/**
 * from handle_trig (sin, cos, tan, asin, acos, atan, sind, cosd, tand, asind, acosd, atand):
 * 
 */
#define TRIG_OPER 0x0f /*!< mask to select trig function bits */
#define TRIG_CO 0x01   /*!< co-function, like cos as opposed to sin */
#define TRIG_TAN 0x02  /*!< tan-function, like cot as opposed to cos */
#define TRIG_ARC 0x04  /*!< arc-function, like asin as opposed to sin */
#define TRIG_REC 0x08  /*!<  -- reciprocal, like sec as opposed to sin */
#define TRIG_DEG 0x10  /*!< angles are in degrees, not radians */
/** 
 * from handle_pronoun (obj, poss, subj, aposs):
 * 
 */
#define PRONOUN_OBJ 0
#define PRONOUN_POSS 1
#define PRONOUN_SUBJ 2
#define PRONOUN_APOSS 3
/** 
 * from do_ufun(): 
 * 
 */
#define U_LOCAL 0x01   /*!< ulocal: preserve global registers */
#define U_PRIVATE 0x02 /*!< ulocal: preserve global registers */
/** 
 * from handle_ifelse() and handle_if() 
 * 
 */
#define IFELSE_OPER 0x0f    /*!< mask */
#define IFELSE_BOOL 0x01    /*!< check for boolean (defaults to nonzero) */
#define IFELSE_FALSE 0x02   /*!< order false,true instead of true,false */
#define IFELSE_DEFAULT 0x04 /*!< only two args, use condition as output */
#define IFELSE_TOKEN 0x08   /*!< allow switch-token substitution */
/** 
 * from handle_timestamps() 
 * 
 */
#define TIMESTAMP_MOD 0x01 /*!< lastmod() */
#define TIMESTAMP_ACC 0X02 /*!< lastaccess() */
#define TIMESTAMP_CRE 0x04 /*!< creation() */
/** 
 * Miscellaneous 
 * 
 */
#define LATTR_COUNT 0x01     /*!< nattr: just return attribute count */
#define LOCFN_WHERE 0x01     /*!< loc: where() vs. loc() */
#define NAMEFN_FULLNAME 0x01 /*!< name: fullname() vs. name() */
#define CHECK_PARENTS 0x01   /*!< hasattrp: recurse up the parent chain */
#define CONNINFO_IDLE 0x01   /*!< conninfo: idle() vs. conn() */
#define UCALL_SANDBOX 0x01   /*!< ucall: sandbox() vs. ucall() */

/**
 * @brief Miscellaneous macros.
 * 
 * Get function flags. Note that Is_Func() and Func_Mask() are identical;
 * they are given specific names for code clarity.
 * 
 */
#define Func_Flags(x) (((FUN *)(x)[-1])->flags)
#define Is_Func(x) (((FUN *)fargs[-1])->flags & (x))
#define Func_Mask(x) (((FUN *)fargs[-1])->flags & (x))
/** 
 * Check access to built-in function. 
 * 
 */
#define Check_Func_Access(p, f) (check_access(p, (f)->perms) && (!((f)->xperms) || check_mod_access(p, (f)->xperms)))
/** 
 * Trim spaces. 
 * 
 */
#define Eat_Spaces(x) trim_space_sep((x), &SPACE_DELIM)
/**
 * @brief Handling CPU time checking.
 * 
 * @note CPU time "clock()" compatibility notes:
 *
 * Linux clock() doesn't necessarily start at 0. BSD clock() does appear to
 * always start at 0.
 *
 * Linux sets CLOCKS_PER_SEC to 1000000, citing POSIX, so its clock() will wrap
 * around from (32-bit) INT_MAX to INT_MIN every 72 cpu-minutes or so. The
 * actual clock resolution is low enough that, for example, it probably never
 * returns odd numbers.
 *
 * BSD sets CLOCKS_PER_SEC to 100, so theoretically I could hose a cpu for 250
 * days and see what it does when it hits INT_MAX. Any bets? Any possible
 * reason to care?
 *
 * NetBSD clock() can occasionally decrease as the scheduler's estimate of how
 * much cpu the mush will use during the current timeslice is revised, so we
 * can't use subtraction.
 *
 * BSD clock() returns -1 if there is an error.
 * 
 * @note CPU time logic notes:
 *
 * B = mudstate.cputime_base L = mudstate.cputime_base + mudconf.func_cpu_lim N = mudstate.cputime_now
 *
 * Assuming B != -1 and N != -1 to catch errors on BSD, the possible
 * combinations of these values are as follows (note >> means "much greater
 * than", not right shift):
 *
 * 1. B <  L  normal    -- limit should be checked, and is not wrapped yet
 * 2. B == L  disabled  -- limit should not be checked
 * 3. B >  L  strange   -- probably misconfigured
 * 4. B >> L  wrapped   -- limit should be checked, and note L wrapped
 *
 * 1.  normal: 
 *  1a. N << B          -- too much, N wrapped 
 *  1b. N <  B          -- fine, NetBSD counted backwards
 *  1c. N >= B, N <= L  -- fine
 *  1d. N >  L          -- too much
 *
 * 2.  disabled:
 *  2a. always          -- fine, not checking
 *
 * 3.  strange:
 *  3a. always          -- fine, I guess we shouldn't check
 *
 * 4.  wrapped:
 *  4a. N <= L          -- fine, N wrapped but not over limit yet
 *  4b. N >  L, N << B  -- too much, N wrapped
 *  4c. N <  B          -- fine, NetBSD counted backwards
 *  4d. N >= B          -- fine
 *
 * Note that 1a, 1d, and 4b are the cases where we can be certain that too much
 * cpu has been used. The code below only checks for 1d. The other two are
 * corner cases that require some distinction between "x > y" and "x >> y".
 */
#define Too_Much_CPU() ((mudstate.cputime_now = clock()), ((mudstate.cputime_now > mudstate.cputime_base + mudconf.func_cpu_lim) && (mudstate.cputime_base + mudconf.func_cpu_lim > mudstate.cputime_base) && (mudstate.cputime_base != -1) && (mudstate.cputime_now != -1)))

/**
 * @brief Global declarations.
 * 
 */
extern const Delim SPACE_DELIM;
extern OBJXFUNCS xfunctions;
extern FUN flist[];

#endif /* __FUNCTIONS_H */
