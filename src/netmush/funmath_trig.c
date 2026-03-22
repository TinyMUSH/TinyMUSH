/**
 * @file funmath_trig.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Transcendental and scientific math functions: trig, log, power, base conversion
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
 * @brief Validate math function arguments (range check only, no delimiter)
 * @return int 1 if valid, 0 if error (already printed to buff)
 */
static inline int
validate_math_args(const char *func_name, int nfargs, int min_args, int max_args, char *buff, char **bufc)
{
    return fn_range_check(func_name, nfargs, min_args, max_args, buff, bufc);
}

/**
 * @brief Returns the square root of <number>.
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
void fun_sqrt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double val;
    val = strtold(fargs[0], (char **)NULL);

    if (val < 0)
    {
        XSAFELBSTR("#-1 SQUARE ROOT OF NEGATIVE", buff, bufc);
    }
    else if (val == 0)
    {
        XSAFELBCHR('0', buff, bufc);
    }
    else
    {
        fval(buff, bufc, sqrtl(val), FPTS_DIG);
    }
}

/**
 * @brief Returns the result of raising the numeric constant e to &lt;power&gt;.
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
void fun_exp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, expl(strtold(fargs[0], (char **)NULL)), FPTS_DIG);
}

/**
 * @brief If only given one argument, this function returns a list of numbers
 *        from 0 to &lt;number&gt;-1. &lt;number&gt; must be at least 1.
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
void fun_ln(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double val;
    val = strtold(fargs[0], (char **)NULL);

    if (val > 0)
    {
        fval(buff, bufc, logl(val), FPTS_DIG);
    }
    else
    {
        XSAFELBSTR("#-1 LN OF NEGATIVE OR ZERO", buff, bufc);
    }
}

/**
 * @brief Handle trigonometrical functions (sin, cos, tan, etc...)
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
void handle_trig(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double val;
    int oper, flag;
    long double (*const trig_funcs[8])(long double) = {sinl, cosl, tanl, NULL, asinl, acosl, atanl, NULL}; /* XXX no cotangent function */
    flag = Func_Flags(fargs);
    oper = flag & TRIG_OPER;
    val = strtold(fargs[0], (char **)NULL);

    if ((flag & TRIG_ARC) && !(flag & TRIG_TAN) && ((val < -1) || (val > 1)))
    {
        XSAFESPRINTF(buff, bufc, "#-1 %s ARGUMENT OUT OF RANGE", ((FUN *)fargs[-1])->name);
        return;
    }

    if ((flag & TRIG_DEG) && !(flag & TRIG_ARC))
    {
        val = val * ((long double)M_PI / 180.0);
    }

    val = (trig_funcs[oper])((long double)val);

    if ((flag & TRIG_DEG) && (flag & TRIG_ARC))
    {
        val = (val * 180.0) / (long double)M_PI;
    }

    fval(buff, bufc, val, FPTS_DIG);
}

/**
 * @brief Convert a character to its decimal value.
 *
 * @param ch Character
 * @param base Base to convert from
 * @return int Value in decimal base.
 */
int fromBaseX(char ch, int base)
{
    int i = -1;
    if (ch == '+' || ch == '-')
    {
        i = base > 36 ? 62 : -1;
    }
    else if (ch == '/' || ch == '_')
    {
        i = base > 36 ? 63 : -1;
    }
    else if (ch >= 'A' && ch <= 'Z')
    {
        i = base > 36 ? ch - 65 : ch - 55;
    }
    else if (ch >= 'a' && ch <= 'z')
    {
        i = base > 36 ? ch - 71 : ch - 87;
    }
    else if (ch >= '0' && ch <= '9')
    {
        i = base > 36 ? ch + 4 : ch - 48;
    }
    if (i > base)
    {
        i = -1;
    }
    return i;
}

/**
 * @brief Convert decimal value to its base X character representation
 *
 * @param i Decimal value
 * @param base Base to convert to
 * @return char Character
 */
char toBaseX(int i, int base)
{
    char ch = 0;
    if (base > 36)
    {
        if (i == 62)
        {
            ch = '-';
        }
        else if (i == 63)
        {
            ch = '_';
        }
        else if (i >= 0 && i <= 25)
        {
            ch = i + 65;
        }
        else if (i >= 26 && i <= 51)
        {
            ch = i + 71;
        }
        else if (i >= 52 && i <= 61)
        {
            ch = i - 4;
        }
    }
    else
    {
        if (i >= 0 && i <= 9)
        {
            ch = i + 48;
        }
        else if (i >= 10 && i <= 35)
        {
            ch = i + 55;
        }
    }

    return ch;
}

/**
 * @brief Convert to various bases
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
void fun_baseconv(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long long n, m;
    int from, to, isneg;
    char *p, *nbp;
    char *nbuf = XMALLOC(LBUF_SIZE, "nbuf");

    if (!is_integer(fargs[1]) || !is_integer(fargs[2]))
    {
        XSAFELBSTR("#-1 INVALID BASE", buff, bufc);
        return;
    }

    from = (int)strtol(fargs[1], (char **)NULL, 10);
    to = (int)strtol(fargs[2], (char **)NULL, 10);

    if ((from < 2) || (from > 64) || (to < 2) || (to > 64))
    {
        XSAFELBSTR("#-1 BASE OUT OF RANGE", buff, bufc);
        return;
    }
    /**
     * Parse the number to convert
     *
     */
    p = Eat_Spaces(fargs[0]);
    n = 0;

    if (p)
    {
        /**
         * If we have a leading hyphen, and we're in base 63/64,
         * always treat it as a minus sign. PennMUSH consistency.
         *
         */
        if ((from < 63) && (to < 63) && (*p == '-'))
        {
            isneg = 1;
            p++;
        }
        else
        {
            isneg = 0;
        }

        while (*p)
        {
            n *= from;

            if (fromBaseX((unsigned char)*p, from) >= 0)
            {
                n += fromBaseX((unsigned char)*p, from);
                p++;
            }
            else
            {
                XSAFELBSTR("#-1 MALFORMED NUMBER", buff, bufc);
                return;
            }
        }

        if (isneg)
        {
            XSAFELBCHR('-', buff, bufc);
        }
    }

    /**
     * Handle the case of 0 and less than base case.
     *
     */
    if (n < to)
    {
        XSAFELBCHR(toBaseX((unsigned char)n, to), buff, bufc);
        return;
    }

    /**
     * Build up the number backwards, then reverse it.
     *
     */
    nbp = nbuf;

    while (n > 0)
    {
        m = n % to;
        n = n / to;
        XSAFELBCHR(toBaseX((unsigned char)m, to), nbuf, &nbp);
    }

    nbp--;

    while (nbp >= nbuf)
    {
        XSAFELBCHR(*nbp, buff, bufc);
        nbp--;
    }
    XFREE(nbuf);
}

/**
 * @brief Returns the result of raising &lt;number&gt; to the &lt;power&gt;'th power.
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
void fun_power(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double val1 = strtold(fargs[0], (char **)NULL);
    long double val2 = strtold(fargs[1], (char **)NULL);

    if (val1 < 0)
    {
        XSAFELBSTR("#-1 POWER OF NEGATIVE", buff, bufc);
    }
    else
    {
        fval(buff, bufc, powl(val1, val2), FPTS_DIG);
    }
}

/**
 * @brief Returns the result of raising &lt;number&gt; to the &lt;power&gt;'th power.
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
void fun_log(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double val = 0.0, base = 0.0;

    if (!validate_math_args(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
    {
        return;
    }

    val = strtold(fargs[0], (char **)NULL);

    if (nfargs == 2)
    {
        base = strtold(fargs[1], (char **)NULL);
    }
    else
    {
        base = 10;
    }

    if ((val <= 0) || (base <= 0))
    {
        XSAFELBSTR("#-1 LOG OF NEGATIVE OR ZERO", buff, bufc);
    }
    else if (base == 1)
    {
        XSAFELBSTR("#-1 DIVISION BY ZERO", buff, bufc);
    }
    else
    {
        fval(buff, bufc, logl(val) / logl(base), FPTS_DIG);
    }
}
