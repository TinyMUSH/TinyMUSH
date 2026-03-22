/**
 * @file funmath_bitwise.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Bitwise integer built-ins: shl, shr, band, bor, bnand
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/**
 * @brief This function returns the result of leftwards bit-shifting &lt;number&gt;
 *        by &lt;count&gt; times.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_shl(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) << strtoll(fargs[1], (char **)NULL, 10), 0);
}

/**
 * @brief This function returns the result of rightwards bit-shifting &lt;number&gt;
 *        by &lt;count&gt; times.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_shr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) >> strtoll(fargs[1], (char **)NULL, 10), 0);
}

/**
 * @brief Intended for use on a bitfield, this function performs a binary AND
 *        between two numbers.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_band(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) & strtoll(fargs[1], (char **)NULL, 10), 0);
}

/**
 * @brief Intended for use on a bitfield, this function performs a binary OR
 *        between two numbers. It is most useful for "turning on" bits in a
 *        bitfield.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_bor(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) | strtoll(fargs[1], (char **)NULL, 10), 0);
}

/**
 * @brief Intended for use on a bitfield, this function performs a binary AND
 *        between a number and the complement of another number.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_bnand(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) & ~(strtoll(fargs[1], (char **)NULL, 10)), 0);
}


