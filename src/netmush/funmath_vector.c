/**
 * @file funmath_vector.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Vector math operations: distance, magnitude, unit vector, and vector arithmetic
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
void fun_dist2d(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
void fun_dist3d(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
 * @param ncargs Number of command's arguments
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
            XSAFELBCHR('0', buff, bufc);
        }

        XFREE(v1);
        return;
    }

    if (res <= 0)
    {
        XSAFELBSTR("#-1 CAN'T MAKE UNIT VECTOR FROM ZERO-LENGTH VECTOR", buff, bufc);
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
 * @param ncargs Number of command's arguments
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
        XSAFELBSTR("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufc);
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
            fval(buff, bufc, strtold(v1[i], (char **)NULL) + strtold(v2[i], (char **)NULL), FPTS_DIG);
        }

        break;

    case VEC_SUB:
        fval(buff, bufc, strtold(v1[0], (char **)NULL) - strtold(v2[0], (char **)NULL), FPTS_DIG);

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            fval(buff, bufc, strtold(v1[i], (char **)NULL) - strtold(v2[i], (char **)NULL), FPTS_DIG);
        }

        break;

    case VEC_OR:
        XSAFEBOOL(buff, bufc, xlate(v1[0]) || xlate(v2[0]));

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            XSAFEBOOL(buff, bufc, xlate(v1[i]) || xlate(v2[i]));
        }

        break;

    case VEC_AND:
        XSAFEBOOL(buff, bufc, xlate(v1[0]) && xlate(v2[0]));

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            XSAFEBOOL(buff, bufc, xlate(v1[i]) && xlate(v2[i]));
        }

        break;

    case VEC_XOR:
        x = xlate(v1[0]);
        y = xlate(v2[0]);
        XSAFEBOOL(buff, bufc, (x && !y) || (!x && y));

        for (int i = 1; i < n; i++)
        {
            print_separator(&osep, buff, bufc);
            x = xlate(v1[i]);
            y = xlate(v2[i]);
            XSAFEBOOL(buff, bufc, (x && !y) || (!x && y));
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
                fval(buff, bufc, strtold(v2[i], (char **)NULL) * scalar, FPTS_DIG);
            }
        }
        else if (m == 1)
        {
            scalar = strtold(v2[0], (char **)NULL);
            fval(buff, bufc, strtold(v1[0], (char **)NULL) * scalar, FPTS_DIG);

            for (int i = 1; i < n; i++)
            {
                print_separator(&osep, buff, bufc);
                fval(buff, bufc, strtold(v1[i], (char **)NULL) * scalar, FPTS_DIG);
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
                fval(buff, bufc, strtold(v1[i], (char **)NULL) * strtold(v2[i], (char **)NULL), FPTS_DIG);
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
            scalar += strtold(v1[i], (char **)NULL) * strtold(v2[i], (char **)NULL);
        }

        fval(buff, bufc, scalar, FPTS_DIG);
        break;

    default:
        /**
         * If we reached this, we're in trouble.
         *
         */
        XSAFELBSTR("#-1 UNIMPLEMENTED", buff, bufc);
        break;
    }

    XFREE(v1);
    XFREE(v2);
}
