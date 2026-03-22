/**
 * @file funmath_arith.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Arithmetic division built-ins: sub, div, floordiv, fdiv, modulo, remainder
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
 * @brief Returns the result of subtracting &lt;number2&gt; from &lt;number1&gt;.
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
void fun_sub(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL) - strtold(fargs[1], (char **)NULL), FPTS_DIG);
}

/**
 * @brief   Returns the integer quotient from dividing &lt;number1&gt; by &lt;number2&gt;,
 *          rounded towards zero.
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
void fun_div(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    /**
     * The C '/' operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     */
    long double top = strtold(fargs[0], (char **)NULL);
    long double bot = strtold(fargs[1], (char **)NULL);

    if (bot == 0)
    {
        XSAFELBSTR("#-1 DIVIDE BY ZERO", buff, bufc);
        return;
    }

    if (top < 0)
    {
        if (bot < 0)
        {
            top = (-top) / (-bot);
        }
        else
        {
            top = -((-top) / bot);
        }
    }
    else
    {
        if (bot < 0)
        {
            top = -(top / (-bot));
        }
        else
        {
            top = top / bot;
        }
    }

    fval(buff, bufc, top, 0);
}

/**
 * @brief   Returns the integer quotient from dividing &lt;number1&gt; by &lt;number2&gt;,
 *          rounded down (towards zero if positive, away from zero if negative).
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
void fun_floordiv(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    /**
     * The C '/' operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     *
     */
    long long top = strtoll(fargs[0], (char **)NULL, 10);
    long long bot = strtoll(fargs[1], (char **)NULL, 10);
    long long res = 0;

    if (bot == 0)
    {
        XSAFELBSTR("#-1 DIVIDE BY ZERO", buff, bufc);
        return;
    }

    if (top < 0)
    {
        if (bot < 0)
        {
            res = (-top) / (-bot);
        }
        else
        {
            res = -((-top) / bot);

            if (top % bot)
            {
                res--;
            }
        }
    }
    else
    {
        if (bot < 0)
        {
            res = -(top / (-bot));

            if (top % bot)
            {
                res--;
            }
        }
        else
        {
            res = top / bot;
        }
    }

    fval(buff, bufc, res, 0);
}

/**
 * @brief Returns the floating point quotient from dividing &lt;number1&gt; by &lt;number2&gt;.
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
void fun_fdiv(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double bot = strtold(fargs[1], (char **)NULL);

    if (bot == 0)
    {
        XSAFELBSTR("#-1 DIVIDE BY ZERO", buff, bufc);
    }
    else
    {
        fval(buff, bufc, (strtold(fargs[0], (char **)NULL) / bot), FPTS_DIG);
    }
}

/**
 * @brief Returns the smallest integer with the same sign as &lt;integer2&gt; such
 *        that the difference between &lt;integer1&gt; and the result is divisible
 *        by &lt;integer2&gt;.
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
void fun_modulo(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long long top = strtoll(fargs[0], (char **)NULL, 10);
    long long bot = strtoll(fargs[1], (char **)NULL, 10);

    /**
     * The C '%' operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     *
     */
    if (bot == 0)
    {
        bot = 1;
    }

    if (top < 0)
    {
        if (bot < 0)
        {
            top = -((-top) % (-bot));
        }
        else
        {
            top = (bot - ((-top) % bot)) % bot;
        }
    }
    else
    {
        if (bot < 0)
        {
            top = -(((-bot) - (top % (-bot))) % (-bot));
        }
        else
        {
            top = top % bot;
        }
    }

    fval(buff, bufc, top, 0);
}

/**
 * @brief Returns the smallest integer with the same sign as &lt;integer1&gt; such
 *        that the difference between &lt;integer1&gt; and the result is divisible
 *        by &lt;integer2&gt;.
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
void fun_remainder(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long long top = strtoll(fargs[0], (char **)NULL, 10);
    long long bot = strtoll(fargs[1], (char **)NULL, 10);

    /**
     * The C '%' operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     *
     */
    if (bot == 0)
    {
        bot = 1;
    }

    if (top < 0)
    {
        if (bot < 0)
        {
            top = -((-top) % (-bot));
        }
        else
        {
            top = -((-top) % bot);
        }
    }
    else
    {
        if (bot < 0)
        {
            top = top % (-bot);
        }
        else
        {
            top = top % bot;
        }
    }

    fval(buff, bufc, top, 0);
}


