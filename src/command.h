/* command.h - declarations used by the command processor */

#include "copyright.h"

#ifndef __COMMAND_H
#define __COMMAND_H

typedef struct addedentry ADDENT;
struct addedentry
{
    dbref thing;
    int atr;
    char *name;
    struct addedentry *next;
};

typedef struct cmdentry CMDENT;
struct cmdentry
{
    char *cmdname;
    NAMETAB *switches;
    int perms;
    int extra;
    int callseq;
    /* int  (*modperms)(); */
    HOOKENT *userperms;
    HOOKENT *pre_hook;
    HOOKENT *post_hook;
    union
    {
        void (*handler)();
        ADDENT *added;
    } info;
};

/* Command handler call conventions */

#define CS_NO_ARGS 0x00000      /* No arguments */
#define CS_ONE_ARG 0x00001      /* One argument */
#define CS_TWO_ARG 0x00002      /* Two arguments */
#define CS_NARG_MASK 0x00003    /* Argument count mask */
#define CS_ARGV 0x00004         /* ARG2 is in ARGV form */
#define CS_INTERP 0x00010       /* Interpret ARG2 if 2 args, ARG1 if 1 */
#define CS_NOINTERP 0x00020     /* Never interp ARG2 if 2 or ARG1 if 1 */
#define CS_CAUSE 0x00040        /* Pass cause to old command handler */
#define CS_UNPARSE 0x00080      /* Pass unparsed cmd to old-style handler */
#define CS_CMDARG 0x00100       /* Pass in given command args */
#define CS_STRIP 0x00200        /* Strip braces even when not interpreting */
#define CS_STRIP_AROUND 0x00400 /* Strip braces around entire string only */
#define CS_ADDED 0x00800        /* Command has been added by @addcommand */
#define CS_LEADIN 0x01000       /* Command is a single-letter lead-in */
#define CS_PRESERVE 0x02000     /* For hooks, preserve global registers */
#define CS_NOSQUISH 0x04000     /* Do not space-compress */
#define CS_FUNCTION 0x08000     /* Can call with command() */
#define CS_ACTOR 0x10000        /* @addcommand executed by player, not obj */
#define CS_PRIVATE 0x20000      /* For hooks, use private global registers */

/* Command permission flags */

#define CA_PUBLIC 0x00000000    /* No access restrictions */
#define CA_GOD 0x00000001       /* GOD only... */
#define CA_WIZARD 0x00000002    /* Wizards only */
#define CA_BUILDER 0x00000004   /* Builders only */
#define CA_IMMORTAL 0x00000008  /* Immortals only */
#define CA_STAFF 0x00000010     /* Must have STAFF flag */
#define CA_HEAD 0x00000020      /* Must have HEAD flag */
#define CA_MODULE_OK 0x00000040 /* Must have MODULE_OK power */
#define CA_ADMIN 0x00000080     /* Wizard or royal */

#define CA_ISPRIV_MASK (CA_GOD | CA_WIZARD | CA_BUILDER | CA_IMMORTAL | CA_STAFF | CA_HEAD | CA_ADMIN | CA_MODULE_OK)

#define CA_NO_HAVEN 0x00000100   /* Not by HAVEN players */
#define CA_NO_ROBOT 0x00000200   /* Not by ROBOT players */
#define CA_NO_SLAVE 0x00000400   /* Not by SLAVE players */
#define CA_NO_SUSPECT 0x00000800 /* Not by SUSPECT players */
#define CA_NO_GUEST 0x00001000   /* Not by GUEST players */

#define CA_ISNOT_MASK (CA_NO_HAVEN | CA_NO_ROBOT | CA_NO_SLAVE | CA_NO_SUSPECT | CA_NO_GUEST)

#define CA_MARKER0 0x00002000
#define CA_MARKER1 0x00004000
#define CA_MARKER2 0x00008000
#define CA_MARKER3 0x00010000
#define CA_MARKER4 0x00020000
#define CA_MARKER5 0x00040000
#define CA_MARKER6 0x00080000
#define CA_MARKER7 0x00100000
#define CA_MARKER8 0x00200000
#define CA_MARKER9 0x00400000

#define CA_MARKER_MASK (CA_MARKER0 | CA_MARKER1 | CA_MARKER2 | CA_MARKER3 | CA_MARKER4 | CA_MARKER5 | CA_MARKER6 | CA_MARKER7 | CA_MARKER8 | CA_MARKER9)

#define CA_GBL_BUILD 0x00800000  /* Requires the global BUILDING flag */
#define CA_GBL_INTERP 0x01000000 /* Requires the global INTERP flag */
#define CA_DISABLED 0x02000000   /* Command completely disabled */
#define CA_STATIC 0x04000000     /* Cannot be changed at runtime */
#define CA_NO_DECOMP 0x08000000  /* Don't include in @decompile */

#define CA_LOCATION 0x10000000 /* Invoker must have location */
#define CA_CONTENTS 0x20000000 /* Invoker must have contents */
#define CA_PLAYER 0x40000000   /* Invoker must be a player */
#define CF_DARK 0x80000000     /* Command doesn't show up in list */

#define SW_MULTIPLE 0x80000000   /* This sw may be spec'd w/others */
#define SW_GOT_UNIQUE 0x40000000 /* Already have a unique option */
#define SW_NOEVAL 0x20000000     /* Don't parse args before calling \
                                  * handler */
/* (typically via a switch alias) */

#define LIST_ATTRIBUTES 1
#define LIST_COMMANDS 2
#define LIST_COSTS 3
#define LIST_FLAGS 4
#define LIST_FUNCTIONS 5
#define LIST_GLOBALS 6
#define LIST_ALLOCATOR 7
#define LIST_LOGGING 8
#define LIST_DF_FLAGS 9
#define LIST_PERMS 10
#define LIST_ATTRPERMS 11
#define LIST_OPTIONS 12
#define LIST_HASHSTATS 13
#define LIST_BUFTRACE 14
#define LIST_CONF_PERMS 15
#define LIST_SITEINFO 16
#define LIST_POWERS 17
#define LIST_SWITCHES 18
#define LIST_VATTRS 19
#define LIST_DB_STATS 20 /* GAC 4/6/92 */
#define LIST_PROCESS 21
#define LIST_BADNAMES 22
#define LIST_CACHEOBJS 23
#define LIST_TEXTFILES 24
#define LIST_PARAMS 25
#define LIST_CF_RPERMS 26
#define LIST_ATTRTYPES 27
#define LIST_FUNCPERMS 28
#define LIST_MEMORY 29
#define LIST_CACHEATTRS 30
#define LIST_RAWMEM 31


/*
 * #define Check_Cmd_Access(p,c,a,n) \ (check_access(p,(c)->perms) && \
 * (!((c)->xperms) || check_mod_access(p,(c)->xperms)) && \
 * (!((c)->userperms) || check_userdef_access(p,(c)->userperms,a,n) ||
 * God(p)))
 */

#define Check_Cmd_Access(p, c, a, n) \
    (check_access(p, (c)->perms) &&  \
     (!((c)->userperms) || check_userdef_access(p, (c)->userperms, a, n) || God(p)))

#endif /* __COMMAND_H */
