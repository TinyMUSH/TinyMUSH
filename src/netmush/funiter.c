/**
 * @file funiter.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core iteration functions (perform_loop, perform_iter, iter context accessors)
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

/**
 * @brief Load attribute text for iteration functions.
 *
 * Handles both \#lambda/ and regular obj/attr syntax.
 * Returns 0 on success, 1 on error (caller should return).
 *
 * @param player Player performing the lookup
 * @param farg Attribute argument (\#lambda/... or obj/attr)
 * @param thing Output: object dbref
 * @param ap Output: attribute pointer
 * @param atext Output: allocated attribute text
 * @param aowner Output: attribute owner
 * @param aflags Output: attribute flags
 * @param alen Output: attribute length
 * @return 0 on success, 1 on error
 */
int load_iter_attrib(dbref player, char *farg, dbref *thing, ATTR **ap, char **atext, dbref *aowner, int *aflags, int *alen)
{
    int anum = 0;

    *thing = NOTHING;
    *ap = NULL;
    *atext = NULL;
    *aowner = NOTHING;
    *aflags = 0;
    *alen = 0;

    if (string_prefix(farg, "#lambda/"))
    {
        *thing = player;
        *atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        *alen = strlen(farg + 8);
        __xstrcpy(*atext, farg + 8);
        (*atext)[*alen] = '\0';
        *aowner = player;
        *aflags = 0;
        return 0;
    }

    if (parse_attrib(player, farg, thing, &anum, 0))
    {
        if ((anum == NOTHING) || !(Good_obj(*thing)))
            *ap = NULL;
        else
            *ap = atr_num(anum);
    }
    else
    {
        *thing = player;
        *ap = atr_str(farg);
    }

    if (!(*ap))
    {
        return 1;
    }

    *atext = atr_pget(*thing, (*ap)->number, aowner, aflags, alen);
    if (!**atext || !(See_attr(player, *thing, *ap, *aowner, *aflags)))
    {
        XFREE(*atext);
        return 1;
    }

    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * perform_loop:
 */

/**
 * @brief backwards-compatible looping constructs: LOOP, PARSE
 *
 * @note See notes on perform_iter for the explanation.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Command arguments
 * @param ncargs Number of command arguments
 */
void perform_loop(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    char *curr = NULL, *objstring = NULL, *buff2 = NULL, *buff3 = NULL, *cp = NULL, *dp = NULL, *str = NULL, *result = NULL, *bb_p = NULL, *tbuf = NULL;
    int number = 0, flag = Func_Mask(LOOP_NOTIFY); /*!< 0 is parse(), 1 is loop() */

    if (flag)
    {
        if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
        {
            return;
        }

        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_EVAL | DELIM_STRING))
        {
            return;
        }
    }
    else
    {
        if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
        {
            return;
        };

        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_EVAL | DELIM_STRING))
        {
            return;
        }

        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_EVAL | DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        }
    }

    dp = cp = curr = XMALLOC(LBUF_SIZE, "curr");
    str = fargs[0];
    eval_expression_string(curr, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
    cp = trim_space_sep(cp, &isep);

    if (!*cp)
    {
        XFREE(curr);
        return;
    }

    bb_p = *bufc;

    while (cp && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (!flag && (*bufc != bb_p))
        {
            print_separator(&osep, buff, bufc);
        }

        number++;
        objstring = split_token(&cp, &isep);
        buff2 = replace_string(BOUND_VAR, objstring, fargs[1]);
        tbuf = ltos(number);
        buff3 = replace_string(LISTPLACE_VAR, tbuf, buff2);
        XFREE(tbuf);
        str = buff3;

        if (!flag)
        {
            eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
        }
        else
        {
            dp = result = XMALLOC(LBUF_SIZE, "result");
            eval_expression_string(result, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
            notify(cause, result);
            XFREE(result);
        }

        XFREE(buff2);
        XFREE(buff3);
    }

    XFREE(curr);
}

/*
 * ---------------------------------------------------------------------------
 * perform_iter: looping constructs
 *
 iter() and list() parse an expression, substitute elements of a list, one at
 a time, using the '##' replacement token. Uses of these functions can be
 nested. In older versions of MUSH, these functions could not be nested.
 parse() and loop() exist for reasons of backwards compatibility, since the
 peculiarities of the way substitutions were done in the string replacements
 make it necessary to provide some way of doing backwards compatibility, in
 order to avoid breaking a lot of code that relies upon particular patterns
 of necessary escaping.

 whentrue() and whenfalse() work similarly to iter(). whentrue() loops as long
 as the expression evaluates to true. whenfalse() loops as long as the
 expression evaluates to false.

 istrue() and isfalse() are inline filterbool() equivalents returning the
 elements of the list which are true or false, respectively.

 iter2(), list2(), etc. are two-list versions of all of the above.
 */

/**
 * @brief Looping constructs
 *
 * @note  iter() and list() parse an expression, substitute elements of a list,
 *        one at a time, using the '##' replacement token. Uses of these
 *        functions can be nested. In older versions of MUSH, these functions
 *        could not be nested. parse() and loop() exist for reasons of
 *        backwards compatibility, since the peculiarities of the way
 *        substitutions were done in the string replacements make it necessary
 *        to provide some way of doing backwards compatibility, in order to
 *        avoid breaking a lot of code that relies upon particular patterns
 *        of necessary escaping.
 *
 *        whentrue() and whenfalse() work similarly to iter(). whentrue() loops
 *        as long as the expression evaluates to true. whenfalse() loops as
 *        long as the expression evaluates to false.
 *
 *        istrue() and isfalse() are inline filterbool() equivalents returning
 *        the elements of the list which are true or false, respectively.
 *
 *        iter2(), list2(), etc. are two-list versions of all of the above.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Command arguments
 * @param ncargs Number of command arguments
 */
void perform_iter(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int bool_flag = 0, filt_flag = 0, two_flag = 0, need_result = 0, need_bool = 0;
    int cur_lev = 0, elen = 0, flag = 0; /*!< 0 is iter(), 1 is list() */
    char *list_str = NULL, *lp = NULL, *input_p = NULL, *list_str2 = NULL, *lp2 = NULL, *input_p2 = NULL;
    char *str = NULL, *bb_p = NULL, *work_buf = NULL, *ep = NULL, *savep = NULL, *dp = NULL, *result = NULL;
    bool is_true = false;

    /**
     * Enforce maximum nesting level.
     *
     */
    if (mushstate.in_loop >= MAX_ITER_NESTING - 1)
    {
        notify_quiet(player, "Exceeded maximum iteration nesting.");
        return;
    }

    /**
     * Figure out what functionality we're getting.
     *
     */
    flag = Func_Mask(LOOP_NOTIFY);
    bool_flag = Func_Mask(BOOL_COND_TYPE);
    filt_flag = Func_Mask(FILT_COND_TYPE);
    two_flag = Func_Mask(LOOP_TWOLISTS);
    need_result = (flag || (filt_flag != FILT_COND_NONE)) ? 1 : 0;
    need_bool = ((bool_flag != BOOL_COND_NONE) || (filt_flag != FILT_COND_NONE)) ? 1 : 0;

    if (!two_flag)
    {
        if (flag)
        {
            if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
            {
                return;
            }

            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_EVAL | DELIM_STRING))
            {
                return;
            }
        }
        else
        {
            if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
            {
                return;
            }

            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_EVAL | DELIM_STRING))
            {
                return;
            }

            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_EVAL | DELIM_STRING | DELIM_NULL | DELIM_CRLF))
            {
                return;
            }
        }

        ep = fargs[1];
    }
    else
    {
        if (flag)
        {
            if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
            {
                return;
            }

            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_EVAL | DELIM_STRING))
            {
                return;
            }
        }
        else
        {
            if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 5, buff, bufc))
            {
                return;
            }
            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_EVAL | DELIM_STRING))
            {
                return;
            }
            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_EVAL | DELIM_STRING | DELIM_NULL | DELIM_CRLF))
            {
                return;
            }
        }

        ep = fargs[2];
    }

    /**
     * The list argument is unevaluated. Go evaluate it.
     *
     */
    input_p = lp = list_str = XMALLOC(LBUF_SIZE, "list_str");
    str = fargs[0];
    eval_expression_string(list_str, &lp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
    input_p = trim_space_sep(input_p, &isep);

    /**
     * Same thing for the second list arg, if we have it
     *
     */
    if (two_flag)
    {
        input_p2 = lp2 = list_str2 = XMALLOC(LBUF_SIZE, "list_str2");
        str = fargs[1];
        eval_expression_string(list_str2, &lp2, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
        input_p2 = trim_space_sep(input_p2, &isep);
    }
    else
    {
        input_p2 = lp2 = list_str2 = NULL;
    }

    /**
     * If both lists are empty, we're done
     *
     */
    if (!(input_p && *input_p) && !(input_p2 && *input_p2))
    {
        XFREE(list_str);

        if (list_str2)
        {
            XFREE(list_str2);
        }

        return;
    }

    cur_lev = mushstate.in_loop++;
    mushstate.loop_token[cur_lev] = NULL;
    mushstate.loop_token2[cur_lev] = NULL;
    mushstate.loop_number[cur_lev] = 0;
    mushstate.loop_break[cur_lev] = 0;
    bb_p = *bufc;
    elen = strlen(ep);

    while ((input_p || input_p2) && !mushstate.loop_break[cur_lev] && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (!need_result && (*bufc != bb_p))
        {
            print_separator(&osep, buff, bufc);
        }

        if (input_p)
            mushstate.loop_token[cur_lev] = split_token(&input_p, &isep);
        else
        {
            mushstate.loop_token[cur_lev] = STRING_EMPTY;
        }

        if (input_p2)
            mushstate.loop_token2[cur_lev] = split_token(&input_p2, &isep);
        else
        {
            mushstate.loop_token2[cur_lev] = STRING_EMPTY;
        }

        mushstate.loop_number[cur_lev] += 1;
        work_buf = XMALLOC(LBUF_SIZE, "work_buf");

        /**
         * we might nibble this
         *
         */
        XMEMCPY(work_buf, ep, elen);
        work_buf[elen] = '\0';
        str = work_buf;
        savep = *bufc;

        if (!need_result)
        {
            eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

            if (need_bool)
            {
                is_true = xlate(savep);
            }
        }
        else
        {
            dp = result = XMALLOC(LBUF_SIZE, "result");
            eval_expression_string(result, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

            if (need_bool)
            {
                is_true = xlate(result);
            }

            if (flag)
            {
                notify(cause, result);
            }
            else if (((filt_flag == FILT_COND_TRUE) && is_true) || ((filt_flag == FILT_COND_FALSE) && !is_true))
            {
                if (*bufc != bb_p)
                {
                    print_separator(&osep, buff, bufc);
                }

                XSAFELBSTR(mushstate.loop_token[cur_lev], buff, bufc);
            }

            XFREE(result);
        }

        XFREE(work_buf);

        if (bool_flag != BOOL_COND_NONE)
        {
            if (!is_true && (bool_flag == BOOL_COND_TRUE))
            {
                break;
            }

            if (is_true && (bool_flag == BOOL_COND_FALSE))
            {
                break;
            }
        }
    }

    XFREE(list_str);

    if (list_str2)
    {
        XFREE(list_str2);
    }

    mushstate.in_loop--;
}

/**
 * @brief Obtain nested iter tokens (#!)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ilev(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    XSAFELTOS(buff, bufc, mushstate.in_loop - 1, LBUF_SIZE);
}

/**
 * @brief Obtain nested iter tokens (#@)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_inum(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int lev;
    lev = (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mushstate.in_loop - 1) || (lev < 0))
    {
        XSAFELBCHR('0', buff, bufc);
        return;
    }

    XSAFELTOS(buff, bufc, mushstate.loop_number[lev], LBUF_SIZE);
}

/**
 * @brief Obtain nested iter tokens (##)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_itext(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int lev;
    lev = (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mushstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    XSAFELBSTR(mushstate.loop_token[lev], buff, bufc);
}

/**
 * @brief Two-list version of iter
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_itext2(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int lev;
    lev = (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mushstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    XSAFELBSTR(mushstate.loop_token2[lev], buff, bufc);
}

/**
 * @brief Break out of an iter.
 *
 * @param buff Not used
 * @param bufc Not used
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ibreak(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int lev;
    lev = mushstate.in_loop - 1 - (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mushstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    mushstate.loop_break[lev] = 1;
}
