/**
 * @file funiter.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Iterator-style built-ins for looping, mapping, and folding over lists/attributes
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
static int load_iter_attrib(dbref player, char *farg, dbref *thing, ATTR **ap, char **atext, dbref *aowner, int *aflags, int *alen)
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
void fun_ilev(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_inum(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_itext(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_itext2(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_ibreak(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int lev;
    lev = mushstate.in_loop - 1 - (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mushstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    mushstate.loop_break[lev] = 1;
}

/**
 * @brief  iteratively eval an attrib with a list of arguments and an
 * optional base case.  With no base case, the first list element is passed
 * as %0 and the second is %1.  The attrib is then evaluated with these args,
 * the result is then used as %0 and the next arg is %1 and so it goes as
 * there are elements left in the list.  The optinal base case gives the user
 * a nice starting point.
 *
 * > &REP_NUM object=[%0][repeat(%1,%1)]
 * > say fold(OBJECT/REP_NUM,1 2 3 4 5,->)
 *   You say "->122333444455555"
 *
 * @note To use added list separator, you must use base case!
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
void fun_fold(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner = NOTHING, thing = NOTHING;
    int aflags = 0, alen = 0, i = 0;
    ATTR *ap = NULL;
    char *atext = NULL, *result = NULL, *curr = NULL, *bp = NULL, *str = NULL, *cp = NULL, *atextbuf = NULL, *op = NULL, *rstore = NULL;
    char *clist[3];
    Delim isep;

    /**
     * We need two to four arguments only
     *
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
    {
        return;
    };

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    /**
     * Two possibilities for the first arg: &lt;obj&gt;/&lt;attr&gt; and &lt;attr&gt;.
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    /**
     * Evaluate it using the rest of the passed function args
     *
     */
    cp = curr = fargs[1];
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    XMEMCPY(atextbuf, atext, alen);
    atextbuf[alen] = '\0';

    /**
     * may as well handle first case now
     *
     */
    i = 1;
    clist[2] = XMALLOC(SBUF_SIZE, "clist[2]");
    op = clist[2];
    XSAFELTOS(clist[2], &op, i, LBUF_SIZE);

    if ((nfargs >= 3) && (fargs[2]))
    {
        clist[0] = fargs[2];
        clist[1] = split_token(&cp, &isep);
        result = bp = XMALLOC(LBUF_SIZE, "result");
        str = atextbuf;
        eval_expression_string(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3);
        i++;
    }
    else
    {
        clist[0] = split_token(&cp, &isep);
        clist[1] = split_token(&cp, &isep);
        result = bp = XMALLOC(LBUF_SIZE, "fun_fold");
        str = atextbuf;
        eval_expression_string(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3);
        i += 2;
    }

    rstore = result;
    result = NULL;

    while (cp && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        clist[0] = rstore;
        clist[1] = split_token(&cp, &isep);
        op = clist[2];
        XSAFELTOS(clist[2], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        result = bp = XMALLOC(LBUF_SIZE, "bp");
        str = atextbuf;
        eval_expression_string(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3);
        XSTRCPY(rstore, result);
        XFREE(result);
        i++;
    }

    XSAFELBSTR(rstore, buff, bufc);
    XFREE(rstore);
    XFREE(atext);
    XFREE(atextbuf);
    XFREE(clist[2]);
}

/**
 * @brief iteratively perform a function with a list of arguments and return
 *        the arg, if the function evaluates to TRUE using the arg.
 *
 * > &IS_ODD object=mod(%0,2)
 * > say filter(object/is_odd,1 2 3 4 5)
 *   You say "1 3 5"
 * > say filter(object/is_odd,1-2-3-4-5,-)
 *   You say "1-3-5"
 *
 * @note If you specify a separator it is used to delimit returned list
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
void handle_filter(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int flag = 0; /*!< 0 is filter(), 1 is filterbool() */
    dbref aowner = NOTHING, thing = NOTHING;
    int aflags = 0, alen = 0, i = 0;
    ATTR *ap = NULL;
    char *atext = NULL, *result = NULL, *curr = NULL, *bp = NULL, *str = NULL, *cp = NULL, *op = NULL, *atextbuf = NULL, *bb_p = NULL;
    char *objs[2];

    flag = Func_Mask(LOGIC_BOOL);

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
    {
        return;
    }

    if (nfargs < 4)
    {
        XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        };
    }

    /**
     * Two possibilities for the first arg: &lt;obj&gt;/&lt;attr&gt; and &lt;attr&gt;.
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    /**
     * Now iteratively eval the attrib with the argument list
     *
     */
    cp = curr = trim_space_sep(fargs[1], &isep);
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    objs[1] = XMALLOC(SBUF_SIZE, "objs[1]");
    bb_p = *bufc;
    i = 1;

    while (cp)
    {
        objs[0] = split_token(&cp, &isep);
        op = objs[1];
        XSAFELTOS(objs[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        result = bp = XMALLOC(LBUF_SIZE, "bp");
        str = atextbuf;
        eval_expression_string(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);

        if ((!flag && (*result == '1')) || (flag && xlate(result)))
        {
            if (*bufc != bb_p)
            {
                print_separator(&osep, buff, bufc);
            }

            XSAFELBSTR(objs[0], buff, bufc);
        }

        XFREE(result);
        i++;
    }

    XFREE(atext);
    XFREE(atextbuf);
    XFREE(objs[1]);
}

/*
 * ---------------------------------------------------------------------------
 * fun_map:

 *
 */

/**
 * @brief Iteratively evaluate an attribute with a list of arguments.
 *
 * > &DIV_TWO object=fdiv(%0,2)
 * > say map(1 2 3 4 5,object/div_two)
 *   You say "0.5 1 1.5 2 2.5"
 * > say map(object/div_two,1-2-3-4-5,-)
 *   You say "0.5-1-1.5-2-2.5"
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
void fun_map(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner = NOTHING, thing = NOTHING;
    int aflags = 0, alen = 0, i = 0;
    ATTR *ap = NULL;
    char *atext = NULL, *str = NULL, *cp = NULL, *atextbuf = NULL, *bb_p = NULL, *op = NULL;
    char *objs[2];
    Delim isep, osep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
    {
        return;
    }

    if (nfargs < 4)
    {
        XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        };
    }

    /**
     * If we don't have anything for a second arg, don't bother.
     *
     */
    if (!fargs[1] || !*fargs[1])
    {
        return;
    }

    /**
     * Two possibilities for the second arg: &lt;obj&gt;/&lt;attr&gt; and &lt;attr&gt;.
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    /**
     * now process the list one element at a time
     *
     */
    cp = trim_space_sep(fargs[1], &isep);
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    objs[1] = XMALLOC(SBUF_SIZE, "objs[1]");
    bb_p = *bufc;
    i = 1;

    while (cp && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        objs[0] = split_token(&cp, &isep);
        op = objs[1];
        XSAFELTOS(objs[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        str = atextbuf;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);
        i++;
    }

    XFREE(atext);
    XFREE(atextbuf);
    XFREE(objs[1]);
}

/**
 * @brief Like map, but operates on two lists or more lists simultaneously,
 *        passing the elements as %0, %1, %2, etc.
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
void fun_mix(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner = NOTHING, thing = NOTHING;
    int aflags = 0, alen = 0, i = 0, lastn = 0, nwords = 0, wc = 0, count[NUM_ENV_VARS];
    ATTR *ap = NULL;
    char *str = NULL, *atext = NULL, *atextbuf = NULL, *bb_p = NULL;
    char *os[NUM_ENV_VARS], *cp[NUM_ENV_VARS];
    Delim isep;

    /**
     * Check to see if we have an appropriate number of arguments. If
     * there are more than three arguments, the last argument is ALWAYS
     * assumed to be a delimiter.
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 12, buff, bufc))
    {
        return;
    }

    if (nfargs < 4)
    {
        isep.str[0] = ' ';
        isep.len = 1;
        lastn = nfargs - 1;
    }
    else if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, nfargs, &isep, DELIM_STRING))
    {
        return;
    }
    else
    {
        lastn = nfargs - 2;
    }

    /**
     * Get the attribute, check the permissions.
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    for (i = 0; i < NUM_ENV_VARS; i++)
    {
        cp[i] = NULL;
    }

    bb_p = *bufc;

    /**
     * process the lists, one element at a time.
     *
     */
    nwords = 0;

    for (i = 0; i < lastn; i++)
    {
        cp[i] = trim_space_sep(fargs[i + 1], &isep);
        count[i] = countwords(cp[i], &isep);

        if (count[i] > nwords)
        {
            nwords = count[i];
        }
    }

    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");

    for (wc = 0; (wc < nwords) && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU(); wc++)
    {
        for (i = 0; i < lastn; i++)
        {
            if (wc < count[i])
            {
                os[i] = split_token(&cp[i], &isep);
            }
            else
            {
                os[i] = STRING_EMPTY;
            }
        }

        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';

        if (*bufc != bb_p)
        {
            print_separator(&isep, buff, bufc);
        }

        str = atextbuf;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, os, lastn);
    }

    XFREE(atext);
    XFREE(atextbuf);
}

/**
 * @brief A little like a fusion of iter() and mix(), it takes elements of
 *        a list X at a time and passes them into a single function as %0, %1,
 *        etc. step(<attribute>,<list>,<step size>,<delim>,<outdelim>)
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
void fun_step(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    ATTR *ap;
    dbref aowner, thing;
    int aflags, alen;
    char *atext, *str, *cp, *atextbuf, *bb_p, *os[NUM_ENV_VARS];
    Delim isep, osep;
    int step_size, i;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 5, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    if (nfargs < 5)
    {
        XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        }
    }

    step_size = (int)strtol(fargs[2], (char **)NULL, 10);

    if ((step_size < 1) || (step_size > NUM_ENV_VARS))
    {
        notify(player, "Illegal step size.");
        return;
    }

    /**
     * Get attribute. Check permissions.
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    cp = trim_space_sep(fargs[1], &isep);
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    bb_p = *bufc;

    while (cp && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        for (i = 0; cp && (i < step_size); i++)
        {
            os[i] = split_token(&cp, &isep);
        }

        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        str = atextbuf;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(os[0]), i);
    }

    XFREE(atext);
    XFREE(atextbuf);
}

/**
 * @brief Like map(), but it operates on a string, rather than on a
 * list, calling a user-defined function for each character in the string. No
 * delimiter is inserted between the results.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_foreach(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref aowner = NOTHING, thing = NOTHING;
    int aflags = 0, alen = 0, i = 0, in_string = 1;
    ATTR *ap = NULL;
    char *str = NULL, *atext = NULL, *atextbuf = NULL, *cp = NULL, *cbuf[2], *op = NULL;
    char start_token = 0, end_token = 0;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
    {
        return;
    }

    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    cbuf[0] = XMALLOC(LBUF_SIZE, "cbuf[0]");
    cp = Eat_Spaces(fargs[1]);
    start_token = '\0';
    end_token = '\0';

    if (nfargs > 2)
    {
        in_string = 0;
        start_token = *fargs[2];
    }

    if (nfargs > 3)
    {
        end_token = *fargs[3];
    }

    /**
     * first letter in string is 0, not 1
     *
     */
    i = -1;
    cbuf[1] = XMALLOC(SBUF_SIZE, "cbuf[1]");

    while (cp && *cp && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (!in_string)
        {
            /**
             * Look for a start token.
             *
             */
            while (*cp && (*cp != start_token))
            {
                XSAFELBCHR(*cp, buff, bufc);
                cp++;
                i++;
            }

            if (!*cp)
            {
                break;
            }

            /**
             * Skip to the next character. Don't copy the start token.
             *
             */
            cp++;
            i++;

            if (!*cp)
            {
                break;
            }

            in_string = 1;
        }

        if (*cp == end_token)
        {
            /**
             * We've found an end token. Skip over it. Note that
             * it's possible to have a start and end token next
             * to one another.
             *
             */
            cp++;
            i++;
            in_string = 0;
            continue;
        }

        i++;
        cbuf[0][0] = *cp++;
        cbuf[0][1] = '\0';
        op = cbuf[1];
        XSAFELTOS(cbuf[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        str = atextbuf;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cbuf, 2);
    }

    XFREE(atextbuf);
    XFREE(atext);
    XFREE(cbuf[0]);
    XFREE(cbuf[1]);
}

/**
 * @brief Combines two lists in an arbitrary manner.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_munge(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, alen, nptrs1, nptrs2, nresults, i, j;
    ATTR *ap;
    char *list1, *list2, *rlist, *st[2];
    char **ptrs1, **ptrs2, **results;
    char *atext, *bp, *str, *oldp;
    Delim isep, osep;
    oldp = *bufc;

    if ((nfargs == 0) || !fargs[0] || !*fargs[0])
    {
        return;
    }

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 5, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    if (nfargs < 5)
    {
        XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        }
    }

    /**
     * Find our object and attribute
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing, &ap, &atext, &aowner, &aflags, &alen))
    {
        return;
    }

    /**
     * Copy our lists and chop them up.
     *
     */
    list1 = XMALLOC(LBUF_SIZE, "list1");
    list2 = XMALLOC(LBUF_SIZE, "list2");
    XSTRCPY(list1, fargs[1]);
    XSTRCPY(list2, fargs[2]);
    nptrs1 = list2arr(&ptrs1, LBUF_SIZE / 2, list1, &isep);
    nptrs2 = list2arr(&ptrs2, LBUF_SIZE / 2, list2, &isep);

    if (nptrs1 != nptrs2)
    {
        XSAFELBSTR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
        XFREE(atext);
        XFREE(list1);
        XFREE(list2);
        XFREE(ptrs1);
        XFREE(ptrs2);
        return;
    }

    /**
     * Call the u-function with the first list as %0. Pass the input
     * separator as %1, which makes sorting, etc. easier.
     *
     */
    st[0] = fargs[1];
    st[1] = XMALLOC(LBUF_SIZE, "st[1]");
    bp = st[1];
    print_separator(&isep, st[1], &bp);
    bp = rlist = XMALLOC(LBUF_SIZE, "rlist");
    str = atext;
    eval_expression_string(rlist, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, st, 2);

    /**
     * Now that we have our result, put it back into array form. Search
     * through list1 until we find the element position, then copy the
     * corresponding element from list2.
     *
     */
    nresults = list2arr(&results, LBUF_SIZE / 2, rlist, &isep);

    for (i = 0; i < nresults; i++)
    {
        for (j = 0; j < nptrs1; j++)
        {
            if (!strcmp(results[i], ptrs1[j]))
            {
                if (*bufc != oldp)
                {
                    print_separator(&osep, buff, bufc);
                }

                XSAFELBSTR(ptrs2[j], buff, bufc);
                ptrs1[j][0] = '\0';
                break;
            }
        }
    }

    XFREE(atext);
    XFREE(list1);
    XFREE(list2);
    XFREE(rlist);
    XFREE(st[1]);
    XFREE(ptrs1);
    XFREE(ptrs2);
    XFREE(results);
}

/*
 * ---------------------------------------------------------------------------
 * fun_while:
 */

/**
 * @brief Evaluate a list until a termination condition is met:
 * while(EVAL_FN,CONDITION_FN,foo|flibble|baz|meep,1,|,-) where EVAL_FN is
 * "[strlen(%0)]" and CONDITION_FN is "[strmatch(%0,baz)]" would result in
 * '3-7-3' being returned. The termination condition is an EXACT not wild
 * match.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_while(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    dbref aowner1 = NOTHING, thing1 = NOTHING, aowner2 = NOTHING, thing2 = NOTHING;
    int aflags1 = 0, aflags2 = 0, alen1 = 0, alen2 = 0, i = 0, tmp_num = 0;
    int is_same = 0, is_exact_same = 0;
    ATTR *ap = NULL, *ap2 = NULL;
    char *atext1 = NULL, *atext2 = NULL, *atextbuf = NULL, *condbuf = NULL;
    char *objs[2], *cp = NULL, *str = NULL, *dp = NULL, *savep = NULL, *bb_p = NULL, *op = NULL;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 4, 6, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &isep, DELIM_STRING))
    {
        return;
    }

    if (nfargs < 6)
    {
        XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
        {
            return;
        }
    }

    /**
     * If our third arg is null (empty list), don't bother.
     *
     */
    if (!fargs[2] || !*fargs[2])
    {
        return;
    }

    /**
     * Our first and second args can be &lt;obj&gt;/&lt;attr&gt; or just &lt;attr&gt;. Use
     * them if we can access them, otherwise return an empty string.
     *
     */
    if (load_iter_attrib(player, fargs[0], &thing1, &ap, &atext1, &aowner1, &aflags1, &alen1))
    {
        return;
    }

    tmp_num = ap->number;

    if (load_iter_attrib(player, fargs[1], &thing2, &ap2, &atext2, &aowner2, &aflags2, &alen2))
    {
        XFREE(atext1);
        return;
    }

    /**
     * If our evaluation and condition are the same, we can save
     * ourselves some time later. There are two possibilities: we have
     * the exact same obj/attr pair, or the attributes contain identical
     * text.
     *
     */
    if ((thing1 == thing2) && (tmp_num == ap2->number))
    {
        is_same = 1;
        is_exact_same = 1;
    }
    else
    {
        is_exact_same = 0;
        atext2 = atr_pget(thing2, ap2->number, &aowner2, &aflags2, &alen2);

        if (!*atext2 || !See_attr(player, thing2, ap2, aowner2, aflags2))
        {
            XFREE(atext1);
            XFREE(atext2);
            return;
        }

        if (!strcmp(atext1, atext2))
        {
            is_same = 1;
        }
        else
        {
            is_same = 0;
        }
    }

    /**
     * Process the list one element at a time.
     *
     */
    cp = trim_space_sep(fargs[2], &isep);
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");

    if (!is_same)
    {
        condbuf = XMALLOC(LBUF_SIZE, "condbuf");
    }

    objs[1] = XMALLOC(SBUF_SIZE, "objs[1]");
    bb_p = *bufc;
    i = 1;

    while (cp && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        objs[0] = split_token(&cp, &isep);
        op = objs[1];
        XSAFELTOS(objs[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext1, alen1);
        atextbuf[alen1] = '\0';
        str = atextbuf;
        savep = *bufc;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);

        if (!is_same)
        {
            XMEMCPY(atextbuf, atext2, alen2);
            atextbuf[alen2] = '\0';
            dp = savep = condbuf;
            str = atextbuf;
            eval_expression_string(condbuf, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);
        }

        if (!strcmp(savep, fargs[3]))
        {
            break;
        }

        i++;
    }

    XFREE(atext1);

    if (!is_exact_same)
    {
        XFREE(atext2);
    }

    XFREE(atextbuf);

    if (!is_same)
    {
        XFREE(condbuf);
    }

    XFREE(objs[1]);
}
