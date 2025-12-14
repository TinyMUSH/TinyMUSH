/**
 * @file funmath.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Math and logic functions
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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
            SAFE_LB_CHR('-', buff, bufc);
        }

        if (fp_mant)
        {
            SAFE_STRNCAT(buff, bufc, "NaN", 3, LBUF_SIZE);
        }
        else
        {
            SAFE_STRNCAT(buff, bufc, "Inf", 3, LBUF_SIZE);
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
    SAFE_SPRINTF(buff, bufc, "%0.*Lf", precision, result);
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
void fun_pi(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, M_PI, (fargs[0] && *fargs[0]) ? atoi(fargs[0]) : FPTS_DIG);
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
void fun_e(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, M_E, (fargs[0] && *fargs[0]) ? atoi(fargs[0]) : FPTS_DIG);
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
void fun_sign(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double num = strtold(fargs[0], (char **)NULL);

    if (num < 0)
    {
        SAFE_STRNCAT(buff, bufc, "-1", 2, LBUF_SIZE);
    }
    else
    {
        SAFE_BOOL(buff, bufc, (num > 0));
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
void fun_abs(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double num = strtold(fargs[0], (char **)NULL);

    if (num == 0)
    {
        SAFE_LB_CHR('0', buff, bufc);
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
 * @brief   Returns the integer quotient from dividing <number1> by <number2>,
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
void fun_floor(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, floorl(strtold(fargs[0], (char **)NULL)), FPTS_DIG);
}

/**
 * @brief Returns the smallest integer greater than or equal to <number>.
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
void fun_ceil(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, ceill(strtold(fargs[0], (char **)NULL)), FPTS_DIG);
}

/**
 * @brief Rounds <number> to <places> decimal places.
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
void fun_round(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL), strtol(fargs[1], (char **)NULL, 10));
}

/**
 * @brief Returns the value of <number> after truncating off any fractional value.
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
void fun_trunc(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double x;

    x = strtold(fargs[0], (char **)NULL);
    x = (x >= 0) ? floor(x) : ceil(x);

    fval(buff, bufc, x, FPTS_DIG);
}

/**
 * @brief Returns <number>, incremented by 1 (the <number>, plus 1).
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
void fun_inc(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL) + 1.0, FPTS_DIG);
}

/**
 * @brief Returns <number>, decremented by 1 (the <number>, minus 1).
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
void fun_dec(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL) - 1.0, FPTS_DIG);
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
void fun_sqrt(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double val;
    val = strtold(fargs[0], (char **)NULL);

    if (val < 0)
    {
        SAFE_LB_STR("#-1 SQUARE ROOT OF NEGATIVE", buff, bufc);
    }
    else if (val == 0)
    {
        SAFE_LB_CHR('0', buff, bufc);
    }
    else
    {
        fval(buff, bufc, sqrtl(val), FPTS_DIG);
    }
}

/**
 * @brief Returns the result of raising the numeric constant e to <power>.
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
void fun_exp(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, expl(strtold(fargs[0], (char **)NULL)), FPTS_DIG);
}

/**
 * @brief If only given one argument, this function returns a list of numbers
 *        from 0 to <number>-1.  <number> must be at least 1.
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
void fun_ln(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double val;
    val = strtold(fargs[0], (char **)NULL);

    if (val > 0)
    {
        fval(buff, bufc, logl(val), FPTS_DIG);
    }
    else
    {
        SAFE_LB_STR("#-1 LN OF NEGATIVE OR ZERO", buff, bufc);
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
void handle_trig(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double val;
    int oper, flag;
    long double (*const trig_funcs[8])(long double) = {sinl, cosl, tanl, NULL, asinl, acosl, atanl, NULL}; /* XXX no cotangent function */
    flag = Func_Flags(fargs);
    oper = flag & TRIG_OPER;
    val = strtold(fargs[0], (char **)NULL);

    if ((flag & TRIG_ARC) && !(flag & TRIG_TAN) && ((val < -1) || (val > 1)))
    {
        SAFE_SPRINTF(buff, bufc, "#-1 %s ARGUMENT OUT OF RANGE", ((FUN *)fargs[-1])->name);
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
 * @brief Convert a character to it's decimal value.
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
 * @brief Convert decival value to it's base X character representation
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
void fun_baseconv(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long long n, m;
    int from, to, isneg;
    char *p, *nbp;
    char *nbuf = XMALLOC(LBUF_SIZE, "nbuf");

    if (!is_integer(fargs[1]) || !is_integer(fargs[2]))
    {
        SAFE_LB_STR("#-1 INVALID BASE", buff, bufc);
        return;
    }

    from = (int)strtol(fargs[1], (char **)NULL, 10);
    to = (int)strtol(fargs[2], (char **)NULL, 10);

    if ((from < 2) || (from > 64) || (to < 2) || (to > 64))
    {
        SAFE_LB_STR("#-1 BASE OUT OF RANGE", buff, bufc);
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
                SAFE_LB_STR("#-1 MALFORMED NUMBER", buff, bufc);
                return;
            }
        }

        if (isneg)
        {
            SAFE_LB_CHR('-', buff, bufc);
        }
    }

    /**
     * Handle the case of 0 and less than base case.
     *
     */
    if (n < to)
    {
        SAFE_LB_CHR(toBaseX((unsigned char)n, to), buff, bufc);
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
        SAFE_LB_CHR(toBaseX((unsigned char)m, to), nbuf, &nbp);
    }

    nbp--;

    while (nbp >= nbuf)
    {
        SAFE_LB_CHR(*nbp, buff, bufc);
        nbp--;
    }
    XFREE(nbuf);
}

/**
 * @brief Takes two numbers, and returns 1 if and only if <number1> is greater
 *        than <number2>, and 0 otherwise.
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
void fun_gt(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtold(fargs[0], (char **)NULL) > strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if and only if <number1> is greater
 *        than or equal to <number2>, and 0 otherwise.
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
void fun_gte(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtold(fargs[0], (char **)NULL) >= strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if and only if <number1> is less
 *        than <number2>, and 0 otherwise.
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
void fun_lt(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtold(fargs[0], (char **)NULL) < strtold(fargs[1], (char **)NULL));
}

/**
 * @brief Takes two numbers, and returns 1 if and only if <number1> is less
 *        than or equal to <number2>, and 0 otherwise.
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
void fun_lte(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtold(fargs[0], (char **)NULL) <= strtold(fargs[1], (char **)NULL));
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
void fun_eq(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtold(fargs[0], (char **)NULL) == strtold(fargs[1], (char **)NULL));
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
void fun_neq(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtold(fargs[0], (char **)NULL) != strtold(fargs[1], (char **)NULL));
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
void fun_ncomp(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double x = strtold(fargs[0], (char **)NULL);
    long double y = strtold(fargs[1], (char **)NULL);

    if (x == y)
    {
        SAFE_LB_CHR('0', buff, bufc);
    }
    else if (x < y)
    {
        SAFE_LB_STR("-1", buff, bufc);
    }
    else
    {
        SAFE_LB_CHR('1', buff, bufc);
    }
}

/**
 * @brief Returns the result of subtracting <number2> from <number1>.
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
void fun_sub(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, strtold(fargs[0], (char **)NULL) - strtold(fargs[1], (char **)NULL), FPTS_DIG);
}

/**
 * @brief   Returns the integer quotient from dividing <number1> by <number2>,
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
void fun_div(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    /**
     * The C '/' operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     */
    long double top = strtold(fargs[0], (char **)NULL);
    long double bot = strtold(fargs[1], (char **)NULL);

    if (bot == 0)
    {
        SAFE_LB_STR("#-1 DIVIDE BY ZERO", buff, bufc);
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
 * @brief   Returns the integer quotient from dividing <number1> by <number2>,
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
void fun_floordiv(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
        SAFE_LB_STR("#-1 DIVIDE BY ZERO", buff, bufc);
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
 * @brief Returns the floating point quotient from dividing <number1> by <number2>.
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
void fun_fdiv(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double bot = strtold(fargs[1], (char **)NULL);

    if (bot == 0)
    {
        SAFE_LB_STR("#-1 DIVIDE BY ZERO", buff, bufc);
    }
    else
    {
        fval(buff, bufc, (strtold(fargs[0], (char **)NULL) / bot), FPTS_DIG);
    }
}

/**
 * @brief Returns the smallest integer with the same sign as <integer2> such
 *        that the difference between <integer1> and the result is divisible
 *        by <integer2>.
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
void fun_modulo(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
 * @brief Returns the smallest integer with the same sign as <integer1> such
 *        that the difference between <integer1> and the result is divisible
 *        by <integer2>.
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
void fun_remainder(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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

/**
 * @brief Returns the result of raising <number> to the <power>'th power.
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
void fun_power(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double val1 = strtold(fargs[0], (char **)NULL);
    long double val2 = strtold(fargs[1], (char **)NULL);

    if (val1 < 0)
    {
        SAFE_LB_STR("#-1 POWER OF NEGATIVE", buff, bufc);
    }
    else
    {
        fval(buff, bufc, powl(val1, val2), FPTS_DIG);
    }
}

/**
 * @brief Returns the result of raising <number> to the <power>'th power.
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
void fun_log(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
        SAFE_LB_STR("#-1 LOG OF NEGATIVE OR ZERO", buff, bufc);
    }
    else if (base == 1)
    {
        SAFE_LB_STR("#-1 DIVISION BY ZERO", buff, bufc);
    }
    else
    {
        fval(buff, bufc, logl(val) / logl(base), FPTS_DIG);
    }
}

/**
 * @brief This function returns the result of leftwards bit-shifting <number>
 *        by <count> times.
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
void fun_shl(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) << strtoll(fargs[1], (char **)NULL, 10), 0);
}

/**
 * @brief This function returns the result of rightwards bit-shifting <number>
 *        by <count> times.
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
void fun_shr(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_band(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_bor(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_bnand(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    fval(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) & ~(strtoll(fargs[1], (char **)NULL, 10)), 0);
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
void fun_add(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    if (nfargs < 2)
    {
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
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
void fun_mul(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    if (nfargs < 2)
    {
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
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
void fun_max(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    if (nfargs < 1)
    {
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
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
void fun_min(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    if (nfargs < 1)
    {
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
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
void fun_bound(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
 * @brief Returns the distance between the Cartesian points in two dimensions
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
void fun_dist2d(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double d;
    long double r;
    d = strtold(fargs[0], (char **)NULL) - strtold(fargs[2], (char **)NULL);
    r = d * d;
    d = strtold(fargs[1], (char **)NULL) - strtold(fargs[3], (char **)NULL);
    r += d * d;
    d = sqrtl(r);
    fval(buff, bufc, d, FPTS_DIG);
}

/**
 * @brief Returns the distance between the Cartesian points in three dimensions.
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
void fun_dist3d(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    long double d;
    long double r;
    d = strtold(fargs[0], (char **)NULL) - strtold(fargs[3], (char **)NULL);
    r = d * d;
    d = strtold(fargs[1], (char **)NULL) - strtold(fargs[4], (char **)NULL);
    r += d * d;
    d = strtold(fargs[2], (char **)NULL) - strtold(fargs[5], (char **)NULL);
    r += d * d;
    d = sqrtl(r);
    fval(buff, bufc, d, FPTS_DIG);
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
 * @param ncargs Nomber of command's arguments
 */
void fun_ladd(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    long double sum = 0.0;
    char *cp = NULL, *curr = NULL;
    Delim isep;

    if (nfargs == 0)
    {
        SAFE_LB_CHR('0', buff, bufc);
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
 * @param ncargs Nomber of command's arguments
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
 * @param ncargs Nomber of command's arguments
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

/**
 * @brief Handle Operations on a single vector: VMAG, VUNIT (VDIM is implemented by fun_words)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void handle_vector(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char **v1 = NULL;
    long double tmp = 0, res = 0;
    int n = 0, oper = Func_Mask(VEC_OPER);
    Delim isep, osep;

    if (oper == VEC_UNIT)
    {
        if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 3, 2, DELIM_STRING, &isep))
        {
            return;
        }
        if (nfargs < 3)
        {
            XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
        }
        else
        {
            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
            {
                return;
            }
        }
    }
    else
    {
        if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
        {
            return;
        }
    }

    /**
     * split the list up, or return if the list is empty
     *
     */
    if (!fargs[0] || !*fargs[0])
    {
        return;
    }

    n = list2arr(&v1, LBUF_SIZE, fargs[0], &isep);

    /**
     * calculate the magnitude
     *
     */
    for (int i = 0; i < n; i++)
    {
        tmp = strtold(v1[i], (char **)NULL);
        res += tmp * tmp;
    }

    /**
     * if we're just calculating the magnitude, return it
     *
     */
    if (oper == VEC_MAG)
    {
        if (res > 0)
        {
            fval(buff, bufc, sqrtl(res), FPTS_DIG);
        }
        else
        {
            SAFE_LB_CHR('0', buff, bufc);
        }

        XFREE(v1);
        return;
    }

    if (res <= 0)
    {
        SAFE_LB_STR("#-1 CAN'T MAKE UNIT VECTOR FROM ZERO-LENGTH VECTOR", buff, bufc);
        XFREE(v1);
        return;
    }

    res = sqrtl(res);
    fval(buff, bufc, strtold(v1[0], (char **)NULL) / res, FPTS_DIG);

    for (int i = 1; i < n; i++)
    {
        print_separator(&osep, buff, bufc);
        fval(buff, bufc, strtold(v1[i], (char **)NULL) / res, FPTS_DIG);
    }

    XFREE(v1);
}

/**
 * @brief Handle operations on a pair of vectors: VADD, VSUB, VMUL, VDOT, VOR,
 *        VAND and VXOR.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void handle_vectors(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    char **v1 = NULL, **v2 = NULL;
    long double scalar = 0.0;
    int n = 0, m = 0, x = 0, y = 0;
    int oper = Func_Mask(VEC_OPER);

    if (oper != VEC_DOT)
    {
        if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
        {
            return;
        }
        if (nfargs < 4)
        {
            XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
        }
        else
        {
            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
            {
                return;
            }
        }
    }
    else
    {
        /**
         * dot product returns a scalar, so no output delim
         *
         */
        if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
        {
            return;
        }
    }

    /**
     * split the list up, or return if the list is empty
     *
     */
    if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1])
    {
        return;
    }

    n = list2arr(&v1, LBUF_SIZE, fargs[0], &isep);
    m = list2arr(&v2, LBUF_SIZE, fargs[1], &isep);

    /*
     * It's okay to have vmul() be passed a scalar first or second arg,
     * but everything else has to be same-dimensional.
     */
    if ((n != m) && !((oper == VEC_MUL) && ((n == 1) || (m == 1))))
    {
        SAFE_LB_STR("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufc);
        XFREE(v1);
        XFREE(v2);
        return;
    }

    switch (oper)
    {
    case VEC_ADD:
        fval(buff, bufc, strtold(v1[0], (char **)NULL) + strtold(v2[0], (char **)NULL), FPTS_DIG);

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            fval(buff, bufc, strtold(v1[0], (char **)NULL) + strtold(v2[0], (char **)NULL), FPTS_DIG);
        }

        break;

    case VEC_SUB:
        fval(buff, bufc, strtold(v1[0], (char **)NULL) - strtold(v2[0], (char **)NULL), FPTS_DIG);

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            fval(buff, bufc, strtold(v1[0], (char **)NULL) - strtold(v2[0], (char **)NULL), FPTS_DIG);
        }

        break;

    case VEC_OR:
        SAFE_BOOL(buff, bufc, xlate(v1[0]) || xlate(v2[0]));

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            SAFE_BOOL(buff, bufc, xlate(v1[i]) || xlate(v2[i]));
        }

        break;

    case VEC_AND:
        SAFE_BOOL(buff, bufc, xlate(v1[0]) && xlate(v2[0]));

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            SAFE_BOOL(buff, bufc, xlate(v1[i]) && xlate(v2[i]));
        }

        break;

    case VEC_XOR:
        x = xlate(v1[0]);
        y = xlate(v2[0]);
        SAFE_BOOL(buff, bufc, (x && !y) || (!x && y));

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            x = xlate(v1[i]);
            y = xlate(v2[i]);
            SAFE_BOOL(buff, bufc, (x && !y) || (!x && y));
        }

        break;

    case VEC_MUL:

        /**
         * if n or m is 1, this is scalar multiplication. otherwise,
         * multiply elementwise.
         *
         */
        if (n == 1)
        {
            scalar = strtold(v1[0], (char **)NULL);
            fval(buff, bufc, strtold(v2[0], (char **)NULL) * scalar, FPTS_DIG);

            for (int i = 1; i < m; i++)
            {
                print_separator(&osep, buff, bufc);
                fval(buff, bufc, strtold(v2[0], (char **)NULL) * scalar, FPTS_DIG);
            }
        }
        else if (m == 1)
        {
            scalar = strtold(v2[0], (char **)NULL);
            fval(buff, bufc, strtold(v1[0], (char **)NULL) * scalar, FPTS_DIG);

            for (int i = 1; i < n; i++)
            {
                print_separator(&osep, buff, bufc);
                fval(buff, bufc, strtold(v1[0], (char **)NULL) * scalar, FPTS_DIG);
            }
        }
        else
        {
            /**
             * vector elementwise product.
             *
             * Note this is a departure from TinyMUX, but an
             * imitation of the PennMUSH behavior: the
             * documentation in Penn claims it's a dot product,
             * but the actual behavior isn't. We implement dot
             * product separately!
             *
             */
            fval(buff, bufc, strtold(v1[0], (char **)NULL) * strtold(v2[0], (char **)NULL), FPTS_DIG);

            for (int i = 1; i < n; i++)
            {
                print_separator(&osep, buff, bufc);
                fval(buff, bufc, strtold(v1[0], (char **)NULL) * strtold(v2[0], (char **)NULL), FPTS_DIG);
            }
        }

        break;

    case VEC_DOT:
        /**
         * dot product: (a,b,c) . (d,e,f) = ad + be + cf
         *
         * no cross product implementation yet: it would be
         * (a,b,c) x (d,e,f) = (bf - ce, cd - af, ae - bd)
         *
         */
        scalar = 0;

        for (int i = 0; i < n; i++)
        {
            scalar += strtold(v1[0], (char **)NULL) * strtold(v2[0], (char **)NULL);
        }

        fval(buff, bufc, scalar, FPTS_DIG);
        break;

    default:
        /**
         * If we reached this, we're in trouble.
         *
         */
        SAFE_LB_STR("#-1 UNIMPLEMENTED", buff, bufc);
        break;
    }

    XFREE(v1);
    XFREE(v2);
}

/**
 * @brief If the input is a non-zero number, returns 0. If it is 0 or the
 *        equivalent (such as a non-numeric string), returns 1.
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
void fun_not(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) > 0 ? false : true);
}

/**
 * @brief Takes a boolean value, and returns its inverse.
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
void fun_notbool(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, !xlate(fargs[0]));
}

/**
 * @brief Takes a boolean value, and returns 0 if it is false, and 1 if true.
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
void fun_t(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_BOOL(buff, bufc, xlate(fargs[0]));
}

/**
 * @brief Convert differents values to Boolean.
 *
 * @param flag Calling function flags
 * @param str String to parse
 * @return true
 * @return false
 */
bool cvtfun(int flag, char *str)
{
    if (flag & LOGIC_BOOL)
    {
        return (xlate(str));
    }
    return strtoll(str, (char **)NULL, 10) > 0 ? true : false;
}

/**
 * @brief Handle multi-argument boolean funcs, various combinations of
 * [L,C][AND,OR,XOR][BOOL]
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void handle_logic(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep;
    bool val = false;
    char *str = NULL, *tbuf = NULL, *bp = NULL;
    int flag = Func_Flags(fargs);
    int oper = (flag & LOGIC_OPER);

    /**
     * most logic operations on an empty string should be false
     *
     */
    val = 0;

    if (flag & LOGIC_LIST)
    {
        if (nfargs == 0)
        {
            SAFE_LB_CHR('0', buff, bufc);
            return;
        }

        /**
         * the arguments come in a pre-evaluated list
         *
         */
        if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
        {
            return;
        }

        bp = trim_space_sep(fargs[0], &isep);

        while (bp)
        {
            tbuf = split_token(&bp, &isep);
            val = ((oper == LOGIC_XOR) && val) ? !cvtfun(flag, tbuf) : cvtfun(flag, tbuf);

            if (((oper == LOGIC_AND) && !val) || ((oper == LOGIC_OR) && val))
            {
                break;
            }
        }
    }
    else if (nfargs < 2)
    {
        /**
         * separate arguments, but not enough of them
         *
         */
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
        return;
    }
    else if (flag & FN_NO_EVAL)
    {
        /**
         * separate, unevaluated arguments
         *
         */
        tbuf = XMALLOC(LBUF_SIZE, "tbuf");

        for (int i = 0; i < nfargs; i++)
        {
            str = fargs[i];
            bp = tbuf;
            eval_expression_string(tbuf, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
            val = ((oper == LOGIC_XOR) && val) ? !cvtfun(flag, tbuf) : cvtfun(flag, tbuf);

            if (((oper == LOGIC_AND) && !val) || ((oper == LOGIC_OR) && val))
            {
                break;
            }
        }

        XFREE(tbuf);
    }
    else
    {
        /**
         * separate, pre-evaluated arguments
         *
         */
        for (int i = 0; i < nfargs; i++)
        {
            val = ((oper == LOGIC_XOR) && val) ? !cvtfun(flag, fargs[i]) : cvtfun(flag, fargs[i]);

            if (((oper == LOGIC_AND) && !val) || ((oper == LOGIC_OR) && val))
            {
                break;
            }
        }
    }

    SAFE_BOOL(buff, bufc, val);
}

/**
 * @brief Handle boolean values for an entire list
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void handle_listbool(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    bool n = false;
    char *tbuf = NULL, *bp = NULL, *bb_p = NULL;
    int flag = Func_Flags(fargs);

    if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 3, 2, DELIM_STRING, &isep))
    {
        return;
    }
    if (nfargs < 3)
    {
        XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        }
    }

    if (!fargs[0] || !*fargs[0])
    {
        return;
    }

    bb_p = *bufc;
    bp = trim_space_sep(fargs[0], &isep);

    while (bp)
    {
        tbuf = split_token(&bp, &isep);

        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        if (flag & IFELSE_BOOL)
        {
            n = xlate(tbuf);
        }
        else
        {
            n = !((int)strtoll(tbuf, (char **)NULL, 10) == 0) && is_number(tbuf) ? true : false;
        }

        if (flag & IFELSE_FALSE)
        {
            n = !n;
        }

        SAFE_BOOL(buff, bufc, n);
    }
}
