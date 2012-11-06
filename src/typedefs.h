/* typedefs.h */

#include "copyright.h"

#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

/* Type definitions */

typedef int	dbref;
typedef int	FLAG;
typedef int	POWER;

typedef unsigned char Uchar;

typedef struct hookentry HOOKENT;
struct hookentry {
    dbref thing;
    int atr;
};

typedef struct key_linked_list KEYLIST;
struct key_linked_list {
    char *name;
    int data;
    struct key_linked_list *next;
};

typedef struct str_linked_list LINKEDLIST;
struct str_linked_list {
    char *name;
    char *value;
    struct str_linked_list *next;
};

typedef struct named_function NAMEDFUNC;
struct named_function {
    char *fn_name;
    int ( *handler )( dbref );
};

typedef struct external_funcs EXTFUNCS;
struct external_funcs {
    int num_funcs;
    NAMEDFUNC **ext_funcs;
};

typedef struct global_register_data GDATA;
struct global_register_data {
    int q_alloc;
    char **q_regs;
    int *q_lens;
    int xr_alloc;
    char **x_names;
    char **x_regs;
    int *x_lens;
    int dirty;
};

typedef struct bque BQUE;	/* Command queue */
struct bque {
    BQUE	*next;
    dbref	player;		/* player who will do command - halt is #-1 */
    dbref	cause;		/* player causing command (for %N) */
    int     pid;            /* internal process ID */
    int	waittime;	/* time to run command */
    dbref	sem;		/* blocking semaphore */
    int	attr;		/* blocking attribute */
    char	*text;		/* buffer for comm, env, and scr text */
    char	*comm;		/* command */
    char	*env[NUM_ENV_VARS];	/* environment vars */
    GDATA	*gdata;		/* temp vars */
    int	nargs;		/* How many args I have */
};

#endif	/* __TYPEDEFS_H */
