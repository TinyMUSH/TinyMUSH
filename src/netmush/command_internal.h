/**
 * @file command_internal.h
 * @brief Internal shared declarations for command subsystem globals.
 *
 * This header exposes shared globals used across the command_*.c modules
 * while keeping their single definitions in command.c.
 */

#pragma once

#include "typedefs.h"

/*
 * Global prefix dispatch array and commonly used command pointers.
 * Defined in command.c, referenced in command_init.c, command_access.c,
 * and later by other command_*.c modules.
 */
extern CMDENT *prefix_cmds[256];
extern CMDENT *goto_cmdp;
extern CMDENT *enter_cmdp;
extern CMDENT *leave_cmdp;
extern CMDENT *internalgoto_cmdp;

/*
 * Handler function pointer cache used by the dispatcher to avoid repeated casts.
 * These are defined in command.c and used by command_dispatch.c.
 */
extern void (*handler_cs_no_args)(dbref, dbref, int);
extern void (*handler_cs_one_args)(dbref, dbref, int, char *);
extern void (*handler_cs_one_args_unparse)(dbref, char *);
extern void (*handler_cs_one_args_cmdargs)(dbref, dbref, int, char *, char *[], int);
extern void (*handler_cs_two_args)(dbref, dbref, int, char *, char *);
extern void (*handler_cs_two_args_cmdargs)(dbref, dbref, int, char *, char *, char *[], int);
extern void (*handler_cs_two_args_argv)(dbref, dbref, int, char *, char *[], int);
extern void (*handler_cs_two_args_cmdargs_argv)(dbref, dbref, int, char *, char *[], int, char *[], int);
