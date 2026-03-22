/**
 * @file funmath.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Numeric and logic built-ins: arithmetic, trig, bitwise, and statistics helpers
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
 * @brief Fix weird math results
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param result Result to check
 * @return unsigned int Result of the check
 */
unsigned int fp_check_weird(char *buff, char **bufc, long double result)
{
    fp_union_uint fp_sign_mask, fp_exp_mask, fp_mant_mask, fp_val;
    const long double d_zero = 0.0;
    unsigned int fp_sign = 0, fp_exp = 0, fp_mant = 0;

    XMEMSET(fp_sign_mask.u, 0, sizeof(fp_sign_mask));
    XMEMSET(fp_exp_mask.u, 0, sizeof(fp_exp_mask));
    XMEMSET(fp_val.u, 0, sizeof(fp_val));
    fp_exp_mask.d = 1.0 / d_zero;          /* = inf */
    fp_sign_mask.d = -1.0 / fp_exp_mask.d; /* = -0.0~ */

    for (size_t i = 0; i < FP_SIZE; i++)
    {
        fp_mant_mask.u[i] = ~(fp_exp_mask.u[i] | fp_sign_mask.u[i]);
    }

    fp_val.d = result;
    fp_sign = fp_mant = 0;
    fp_exp = FP_EXP_ZERO | FP_EXP_WEIRD;

    for (size_t i = 0; (i < FP_SIZE) && fp_exp; i++)
    {
        if (fp_exp_mask.u[i])
        {
            unsigned int x = (fp_exp_mask.u[i] & fp_val.u[i]);

            if (x == fp_exp_mask.u[i])
            {
                /**
                 * these bits are all set. can't be zero
                 * exponent, but could still be max (weird)
                 * exponent.
                 *
                 */
                fp_exp &= ~FP_EXP_ZERO;
            }
            else if (x == 0)
            {
                /**
                 * none of these bits are set. can't be max
                 * exponent, but could still be zero
                 * exponent.
                 *
                 */
                fp_exp &= ~FP_EXP_WEIRD;
            }
            else
            {
                /**
                 * some bits were set but not others. can't
                 * be either zero exponent or max exponent.
                 *
                 */
                fp_exp = 0;
            }
        }

        fp_sign |= (fp_sign_mask.u[i] & fp_val.u[i]);
        fp_mant |= (fp_mant_mask.u[i] & fp_val.u[i]);
    }

    if (fp_exp == FP_EXP_WEIRD)
    {
        if (fp_sign)
        {
            XSAFELBCHR('-', buff, bufc);
        }

        if (fp_mant)
        {
            XSAFESTRNCAT(buff, bufc, "NaN", 3, LBUF_SIZE);
        }
        else
        {
            XSAFESTRNCAT(buff, bufc, "Inf", 3, LBUF_SIZE);
        }
    }

    return fp_exp;
}

/**
 * @brief Copy the floating point value into a buffer and make it presentable
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param result Result to present
 * @param precision Precision
 */
void fval(char *buff, char **bufc, long double result, int precision)
{
    char *p = NULL, *buf1 = NULL;

    switch (fp_check_weird(buff, bufc, result))
    {
    case FP_EXP_WEIRD:
        return;

    case FP_EXP_ZERO:
        result = 0.0;
        break;

    default:
        break;
    }

    buf1 = *bufc;
    XSAFESPRINTF(buff, bufc, "%0.*Lf", precision, result);
    **bufc = '\0';

    /**
     * If integer, we're done.
     *
     */
    p = strrchr(buf1, '.');
    if (p == NULL)
    {
        return;
    }

    /**
     * Remove useless zeroes
     *
     */
    p = strrchr(buf1, '0');
    if (p == NULL)
    {
        return;
    }
    else if (*(p + 1) == '\0')
    {
        while (*p == '0')
        {
            *p-- = '\0';
        }

        *bufc = p + 1;
    }

    /**
     * take care of dangling '.'
     *
     */
    p = strrchr(buf1, '.');

    if ((p != NULL) && (*(p + 1) == '\0'))
    {
        *p = '\0';
        *bufc = p;
    }

    /**
     * Handle bogus result of "-0" from sprintf.  Yay, cclib.
     *
     */
    if (!strcmp(buf1, "-0"))
    {
        *buf1 = '0';
        *bufc = buf1 + 1;
    }
}

/**
 * @brief Return the PI constant.
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
void fun_pi(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, M_PI, (fargs[0] && *fargs[0]) ? (int)strtol(fargs[0], (char **)NULL, 10) : FPTS_DIG);
}

/**
 * @brief Return the E constant.
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
void fun_e(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, M_E, (fargs[0] && *fargs[0]) ? (int)strtol(fargs[0], (char **)NULL, 10) : FPTS_DIG);
}

/**
 * @brief Returns -1, 0, or 1 depending on whether its argument is negative,
 *        zero, or positive (respectively).
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
void fun_sign(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double num = strtold(fargs[0], (char **)NULL);

    if (num < 0)
    {
        XSAFESTRNCAT(buff, bufc, "-1", 2, LBUF_SIZE);
    }
    else
    {
        XSAFEBOOL(buff, bufc, (num > 0));
    }
}

/**
 * @brief Returns the absolute value of its argument.
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
void fun_abs(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double num = strtold(fargs[0], (char **)NULL);

    if (num == 0)
    {
        XSAFELBCHR('0', buff, bufc);
    }
    else if (num < 0)
    {
        fval(buff, bufc, -num, FPTS_DIG);
    }
    else
    {
        fval(buff, bufc, num, FPTS_DIG);
    }
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
void fun_floor(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, floorl(strtold(fargs[0], (char **)NULL)), FPTS_DIG);
}

/**
 * @brief Returns the smallest integer greater than or equal to &lt;number&gt;.
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
void fun_ceil(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, ceill(strtold(fargs[0], (char **)NULL)), FPTS_DIG);
}

/**
 * @brief Rounds &lt;number&gt; to &lt;places&gt; decimal places.
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
void fun_round(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL), strtol(fargs[1], (char **)NULL, 10));
}

/**
 * @brief Returns the value of &lt;number&gt; after truncating off any fractional value.
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
void fun_trunc(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double x;

    x = strtold(fargs[0], (char **)NULL);
    x = (x >= 0) ? floor(x) : ceil(x);

    fval(buff, bufc, x, FPTS_DIG);
}

/**
 * @brief Returns &lt;number&gt;, incremented by 1 (the &lt;number&gt;, plus 1).
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
void fun_inc(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL) + 1.0, FPTS_DIG);
}

/**
 * @brief Returns &lt;number&gt;, decremented by 1 (the &lt;number&gt;, minus 1).
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
void fun_dec(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL) - 1.0, FPTS_DIG);
}

/**
 * @brief Return a random number from 0 to arg1 - 1
 */
void fun_rand(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int num = (int)strtol(fargs[0], (char **)NULL, 10);

    if (num < 1)
    {
        XSAFELBCHR('0', buff, bufc);
    }
    else
    {
        XSAFESPRINTF(buff, bufc, "%ld", random_range(0, (num)-1));
    }
}

/**
 * @brief Roll XdY dice
 */
void fun_die(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int n, die, count;
    int total = 0;

    if (!fargs[0] || !fargs[1])
    {
        XSAFELBCHR('0', buff, bufc);
        return;
    }

    n = (int)strtol(fargs[0], (char **)NULL, 10);
    die = (int)strtol(fargs[1], (char **)NULL, 10);

    if ((n == 0) || (die <= 0))
    {
        XSAFELBCHR('0', buff, bufc);
        return;
    }

    if ((n < 1) || (n > 100))
    {
        XSAFELBSTR("#-1 NUMBER OUT OF RANGE", buff, bufc);
        return;
    }

    for (count = 0; count < n; count++)
    {
        total += (int)random_range(1, die);
    }

    XSAFELTOS(buff, bufc, total, LBUF_SIZE);
}


/**
 * @brief Returns the result of adding its arguments together.
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
void fun_add(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if (nfargs < 2)
    {
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
    }
    else
    {
        long double sum = strtold(fargs[0], (char **)NULL);

        for (int i = 1; i < nfargs; i++)
        {
            sum += strtold(fargs[i], (char **)NULL);
        }

        fval(buff, bufc, sum, FPTS_DIG);
    }

    return;
}

/**
 * @brief Returns the result of multiplying its arguments together.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_mul(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if (nfargs < 2)
    {
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
    }
    else
    {
        long double prod = strtold(fargs[0], (char **)NULL);

        for (int i = 1; i < nfargs; i++)
        {
            prod *= strtold(fargs[i], (char **)NULL);
        }

        fval(buff, bufc, prod, FPTS_DIG);
    }

    return;
}

/**
 * @brief Returns the largest number from among its arguments.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_max(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if (nfargs < 1)
    {
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
    }
    else
    {
        long double max = strtold(fargs[0], (char **)NULL);
        long double val = 0.0;

        for (int i = 1; i < nfargs; i++)
        {
            val = strtold(fargs[i], (char **)NULL);
            max = (max < val) ? val : max;
        }

        fval(buff, bufc, max, FPTS_DIG);
    }
}

/**
 * @brief Returns the smallest number from among its arguments.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_min(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if (nfargs < 1)
    {
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
    }
    else
    {
        long double min = strtold(fargs[0], (char **)NULL);
        long double val = 0.0;

        for (int i = 1; i < nfargs; i++)
        {
            val = strtold(fargs[i], (char **)NULL);
            min = (min > val) ? val : min;
        }

        fval(buff, bufc, min, FPTS_DIG);
    }
}

/**
 * @brief Force a number to conform to specified bounds.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_bound(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double min = 0.0, max = 0.0, val = 0.0;
    char *cp = NULL;

    if (!validate_math_args(((FUN *)fargs[-1])->name, nfargs, 1, 3, buff, bufc))
    {
        return;
    }
    val = strtold(fargs[0], (char **)NULL);

    if (nfargs < 2)
    {
        /**
         * just the number; no bounds enforced
         *
         */
        fval(buff, bufc, val, FPTS_DIG);
        return;
    }

    if (nfargs > 1)
    {
        for (cp = fargs[1]; *cp && isspace(*cp); cp++)
        {
            /**
             * if empty, don't check the minimum
             *
             */
        }

        if (*cp)
        {
            min = strtold(fargs[1], (char **)NULL);
            val = (val < min) ? min : val;
        }
    }

    if (nfargs > 2)
    {
        for (cp = fargs[2]; *cp && isspace(*cp); cp++)
        {
            /**
             * if empty, don't check the maximum
             *
             */
        }

        if (*cp)
        {
            max = strtold(fargs[2], (char **)NULL);
            val = (val > max) ? max : val;
        }
    }

    fval(buff, bufc, val, FPTS_DIG);
}

/**
 * @brief Adds a list of numbers together.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_ladd(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double sum = 0.0;
    char *cp = NULL, *curr = NULL;
    Delim isep;

    if (nfargs == 0)
    {
        XSAFELBCHR('0', buff, bufc);
        return;
    }

    if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
    {
        return;
    }

    sum = 0.0;
    cp = trim_space_sep(fargs[0], &isep);

    while (cp)
    {
        curr = split_token(&cp, &isep);
        sum += strtold(curr, (char **)NULL);
    }

    fval(buff, bufc, sum, FPTS_DIG);
}

/**
 * @brief Obtains the largest number out of a list of numbers (i.e., the maximum).
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_lmax(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double max = 0.0, val = 0.0;
    char *cp = NULL, *curr = NULL;
    Delim isep;

    if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
    {
        return;
    }

    cp = trim_space_sep(fargs[0], &isep);

    if (cp)
    {
        curr = split_token(&cp, &isep);
        max = strtold(curr, (char **)NULL);

        while (cp)
        {
            curr = split_token(&cp, &isep);
            val = strtold(curr, (char **)NULL);

            if (max < val)
            {
                max = val;
            }
        }

        fval(buff, bufc, max, FPTS_DIG);
    }
}

/**
 * @brief Obtains the smallest number out of a list of numbers (i.e., the minimum).
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_lmin(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double min = 0.0, val = 0.0;
    char *cp = NULL, *curr = NULL;
    Delim isep;

    if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
    {
        return;
    }

    cp = trim_space_sep(fargs[0], &isep);

    if (cp)
    {
        curr = split_token(&cp, &isep);
        min = strtold(curr, (char **)NULL);

        while (cp)
        {
            curr = split_token(&cp, &isep);
            val = strtold(curr, (char **)NULL);

            if (min > val)
            {
                min = val;
            }
        }

        fval(buff, bufc, min, FPTS_DIG);
    }
}

