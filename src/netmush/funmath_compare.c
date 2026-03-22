/**
 * @file funmath_compare.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Numeric comparison built-ins: gt, gte, lt, lte, eq, neq, ncomp
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
 * @brief Takes two numbers, and returns 1 if and only if &lt;number1&gt; is greater
 *        than &lt;number2&gt;, and 0 otherwise.
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
void fun_gt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtold(fargs[0], (char **)NULL) > strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if and only if &lt;number1&gt; is greater
 *        than or equal to &lt;number2&gt;, and 0 otherwise.
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
void fun_gte(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtold(fargs[0], (char **)NULL) >= strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if and only if &lt;number1&gt; is less
 *        than &lt;number2&gt;, and 0 otherwise.
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
void fun_lt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtold(fargs[0], (char **)NULL) < strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if and only if &lt;number1&gt; is less
 *        than or equal to &lt;number2&gt;, and 0 otherwise.
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
void fun_lte(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtold(fargs[0], (char **)NULL) <= strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if they are equal and 0 if they
 *        are not.
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
void fun_eq(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtold(fargs[0], (char **)NULL) == strtold(fargs[1], (char **)NULL));
}

/**
 * @brief   Takes two numbers, and returns 1 if they are not equal and 0 if
 *          they are equal.
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
void fun_neq(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtold(fargs[0], (char **)NULL) != strtold(fargs[1], (char **)NULL));
}

/**
 * @brief This function returns 0 if the two numbers are equal, 1 if the first
 *        number is greater than the second number, and -1 if the first number
 *        is less than the second number.
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
void fun_ncomp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double x = strtold(fargs[0], (char **)NULL);
    long double y = strtold(fargs[1], (char **)NULL);

    if (x == y)
    {
        XSAFELBCHR('0', buff, bufc);
    }
    else if (x < y)
    {
        XSAFELBSTR("-1", buff, bufc);
    }
    else
    {
        XSAFELBCHR('1', buff, bufc);
    }
}


