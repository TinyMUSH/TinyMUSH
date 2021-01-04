/**
 * @file typedefs.h
 * @author TinyMUSH (https://github.com/TinyMUSH)
 * @brief Type definitions for variables
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"

#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

typedef int dbref;
typedef int FLAG;
typedef int POWER;

typedef struct hookentry
{
    dbref thing;
    int atr;
} HOOKENT;

typedef struct key_linked_list
{
    char *name;
    int data;
    struct key_linked_list *next;
} KEYLIST;

typedef struct str_linked_list
{
    char *name;
    char *value;
    struct str_linked_list *next;
} LINKEDLIST;

typedef struct named_function
{
    char *fn_name;
    int (*handler)(dbref);
} NAMEDFUNC;

typedef struct external_funcs
{
    int num_funcs;
    NAMEDFUNC **ext_funcs;
} EXTFUNCS;

typedef struct global_register_data
{
    int q_alloc;
    char **q_regs;
    int *q_lens;
    int xr_alloc;
    char **x_names;
    char **x_regs;
    int *x_lens;
    int dirty;
} GDATA;

typedef struct bque BQUE; /*!< Command queue */
struct bque
{
    BQUE *next;              /*!< pointer to next command */
    dbref player;            /*!< player who will do command - halt is #-1 */
    dbref cause;             /*!< player causing command (for %N) */
    int pid;                 /*!< internal process ID */
    int waittime;            /*!< time to run command */
    dbref sem;               /*!< blocking semaphore */
    int attr;                /*!< blocking attribute */
    char *text;              /*!< buffer for comm, env, and scr text */
    char *comm;              /*!< command */
    char *env[NUM_ENV_VARS]; /*!< environment vars */
    GDATA *gdata;            /*!< temp vars */
    int nargs;               /*!< How many args I have */
};

/**
 * Return values for cf_ functions.
 * 
 */
typedef enum CF_RESULT
{
    CF_Failure = -1,
    CF_Success,
    CF_Partial
} CF_Result;

#endif /* __TYPEDEFS_H */
