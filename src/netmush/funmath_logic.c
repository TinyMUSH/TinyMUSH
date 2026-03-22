/**
 * @file funmath_logic.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Logical and boolean operations: not, and, or, xor, list boolean evaluation
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
void fun_not(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, strtoll(fargs[0], (char **)NULL, 10) > 0 ? false : true);
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
void fun_notbool(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, !xlate(fargs[0]));
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
void fun_t(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFEBOOL(buff, bufc, xlate(fargs[0]));
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
            XSAFELBCHR('0', buff, bufc);
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
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
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

    XSAFEBOOL(buff, bufc, val);
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

        XSAFEBOOL(buff, bufc, n);
    }
}
