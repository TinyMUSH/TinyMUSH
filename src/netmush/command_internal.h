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
 * Defined in command_init.c, referenced in command_access.c and other command_*.c modules.
 */
extern CMDENT *prefix_cmds[256];
extern CMDENT *goto_cmdp;
extern CMDENT *enter_cmdp;
extern CMDENT *leave_cmdp;
extern CMDENT *internalgoto_cmdp;
