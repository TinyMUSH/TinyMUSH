/**
 * @file funiter.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Functions for user-defined iterations over lists
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

/*
 * ---------------------------------------------------------------------------
 * perform_loop: backwards-compatible looping constructs: LOOP, PARSE See
 * notes on perform_iter for the explanation.
 */

void perform_loop(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int flag; /* 0 is parse(), 1 is loop() */
    char *curr, *objstring, *buff2, *buff3, *cp, *dp, *str, *result, *bb_p, *tbuf;
    int number = 0;
    flag = Func_Mask(LOOP_NOTIFY);

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
    exec(curr, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
    cp = trim_space_sep(cp, &isep);

    if (!*cp)
    {
        XFREE(curr);
        return;
    }

    bb_p = *bufc;

    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
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
            exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
        }
        else
        {
            dp = result = XMALLOC(LBUF_SIZE, "result");
            exec(result, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
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
 * iter() and list() parse an expression, substitute elements of a list, one at
 * a time, using the '##' replacement token. Uses of these functions can be
 * nested. In older versions of MUSH, these functions could not be nested.
 * parse() and loop() exist for reasons of backwards compatibility, since the
 * peculiarities of the way substitutions were done in the string
 * replacements make it necessary to provide some way of doing backwards
 * compatibility, in order to avoid breaking a lot of code that relies upon
 * particular patterns of necessary escaping.
 *
 * whentrue() and whenfalse() work similarly to iter(). whentrue() loops as long
 * as the expression evaluates to true. whenfalse() loops as long as the
 * expression evaluates to false.
 *
 * istrue() and isfalse() are inline filterbool() equivalents returning the
 * elements of the list which are true or false, respectively.
 *
 * iter2(), list2(), etc. are two-list versions of all of the above.
 */

void perform_iter(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int flag; /* 0 is iter(), 1 is list() */
    int bool_flag, filt_flag, two_flag;
    int need_result, need_bool;
    char *list_str, *lp, *input_p;
    char *list_str2, *lp2, *input_p2;
    char *str, *bb_p, *work_buf;
    char *ep, *savep, *dp, *result;
    int cur_lev, elen;
    bool is_true = false;

    /*
     * Enforce maximum nesting level.
     */

    if (mudstate.in_loop >= MAX_ITER_NESTING - 1)
    {
        notify_quiet(player, "Exceeded maximum iteration nesting.");
        return;
    }

    /*
     * Figure out what functionality we're getting.
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

    /*
     * The list argument is unevaluated. Go evaluate it.
     */
    input_p = lp = list_str = XMALLOC(LBUF_SIZE, "list_str");
    str = fargs[0];
    exec(list_str, &lp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
    input_p = trim_space_sep(input_p, &isep);

    /*
     * Same thing for the second list arg, if we have it
     */

    if (two_flag)
    {
        input_p2 = lp2 = list_str2 = XMALLOC(LBUF_SIZE, "list_str2");
        str = fargs[1];
        exec(list_str2, &lp2, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
        input_p2 = trim_space_sep(input_p2, &isep);
    }
    else
    {
        input_p2 = lp2 = list_str2 = NULL;
    }

    /*
     * If both lists are empty, we're done
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

    cur_lev = mudstate.in_loop++;
    mudstate.loop_token[cur_lev] = NULL;
    mudstate.loop_token2[cur_lev] = NULL;
    mudstate.loop_number[cur_lev] = 0;
    mudstate.loop_break[cur_lev] = 0;
    bb_p = *bufc;
    elen = strlen(ep);

    while ((input_p || input_p2) && !mudstate.loop_break[cur_lev] && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (!need_result && (*bufc != bb_p))
        {
            print_separator(&osep, buff, bufc);
        }

        if (input_p)
            mudstate.loop_token[cur_lev] = split_token(&input_p, &isep);
        else
        {
            mudstate.loop_token[cur_lev] = STRING_EMPTY;
        }

        if (input_p2)
            mudstate.loop_token2[cur_lev] = split_token(&input_p2, &isep);
        else
        {
            mudstate.loop_token2[cur_lev] = STRING_EMPTY;
        }

        mudstate.loop_number[cur_lev] += 1;
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
            exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

            if (need_bool)
            {
                is_true = xlate(savep);
            }
        }
        else
        {
            dp = result = XMALLOC(LBUF_SIZE, "result");
            exec(result, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

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

                SAFE_LB_STR(mudstate.loop_token[cur_lev], buff, bufc);
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

    mudstate.in_loop--;
}

/*
 * ---------------------------------------------------------------------------
 * itext(), inum(), ilev(): Obtain nested iter tokens (##, #@, #!).
 */

void fun_ilev(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    SAFE_LTOS(buff, bufc, mudstate.in_loop - 1, LBUF_SIZE);
}

void fun_inum(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int lev;
    lev = (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mudstate.in_loop - 1) || (lev < 0))
    {
        SAFE_LB_CHR('0', buff, bufc);
        return;
    }

    SAFE_LTOS(buff, bufc, mudstate.loop_number[lev], LBUF_SIZE);
}

void fun_itext(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int lev;
    lev = (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mudstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    SAFE_LB_STR(mudstate.loop_token[lev], buff, bufc);
}

void fun_itext2(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int lev;
    lev = (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mudstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    SAFE_LB_STR(mudstate.loop_token2[lev], buff, bufc);
}

void fun_ibreak(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int lev;
    lev = mudstate.in_loop - 1 - (int)strtol(fargs[0], (char **)NULL, 10);

    if ((lev > mudstate.in_loop - 1) || (lev < 0))
    {
        return;
    }

    mudstate.loop_break[lev] = 1;
}

/*
 * ---------------------------------------------------------------------------
 * fun_fold: iteratively eval an attrib with a list of arguments and an
 * optional base case.  With no base case, the first list element is passed
 * as %0 and the second is %1.  The attrib is then evaluated with these args,
 * the result is then used as %0 and the next arg is %1 and so it goes as
 * there are elements left in the list.  The optinal base case gives the user
 * a nice starting point.
 *
 * > &REP_NUM object=[%0][repeat(%1,%1)] > say fold(OBJECT/REP_NUM,1 2 3 4 5,->)
 * You say "->122333444455555"
 *
 * NOTE: To use added list separator, you must use base case!
 */

void fun_fold(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, alen, anum, i;
    ATTR *ap;
    char *atext, *result, *curr, *bp, *str, *cp, *atextbuf;
    char *op, *clist[3], *rstore;
    Delim isep;
    /*
     * We need two to four arguments only
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
    {
        return;
    };

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>.
     */
    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
    }
    /*
     * Evaluate it using the rest of the passed function args
     */
    cp = curr = fargs[1];
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    XMEMCPY(atextbuf, atext, alen);
    atextbuf[alen] = '\0';
    /*
     * may as well handle first case now
     */
    i = 1;
    clist[2] = XMALLOC(SBUF_SIZE, "clist[2]");
    op = clist[2];
    SAFE_LTOS(clist[2], &op, i, LBUF_SIZE);

    if ((nfargs >= 3) && (fargs[2]))
    {
        clist[0] = fargs[2];
        clist[1] = split_token(&cp, &isep);
        result = bp = XMALLOC(LBUF_SIZE, "result");
        str = atextbuf;
        exec(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3);
        i++;
    }
    else
    {
        clist[0] = split_token(&cp, &isep);
        clist[1] = split_token(&cp, &isep);
        result = bp = XMALLOC(LBUF_SIZE, "fun_fold");
        str = atextbuf;
        exec(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3);
        i += 2;
    }

    rstore = result;
    result = NULL;

    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
    {
        clist[0] = rstore;
        clist[1] = split_token(&cp, &isep);
        op = clist[2];
        SAFE_LTOS(clist[2], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        result = bp = XMALLOC(LBUF_SIZE, "bp");
        str = atextbuf;
        exec(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3);
        XSTRCPY(rstore, result);
        XFREE(result);
        i++;
    }

    SAFE_LB_STR(rstore, buff, bufc);
    XFREE(rstore);
    XFREE(atext);
    XFREE(atextbuf);
    XFREE(clist[2]);
}

/*
 * ---------------------------------------------------------------------------
 * fun_filter: iteratively perform a function with a list of arguments and
 * return the arg, if the function evaluates to TRUE using the arg.
 *
 * > &IS_ODD object=mod(%0,2) > say filter(object/is_odd,1 2 3 4 5) You say "1 3
 * 5" > say filter(object/is_odd,1-2-3-4-5,-) You say "1-3-5"
 *
 * NOTE:  If you specify a separator it is used to delimit returned list
 */

void handle_filter(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int flag; /* 0 is filter(), 1 is filterbool() */
    dbref aowner, thing;
    int aflags, alen, anum, i;
    ATTR *ap;
    char *atext, *result, *curr, *objs[2], *bp, *str, *cp, *op, *atextbuf;
    char *bb_p;
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

    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>.
     */
    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
    }
    /*
     * Now iteratively eval the attrib with the argument list
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
        SAFE_LTOS(objs[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        result = bp = XMALLOC(LBUF_SIZE, "bp");
        str = atextbuf;
        exec(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);

        if ((!flag && (*result == '1')) || (flag && xlate(result)))
        {
            if (*bufc != bb_p)
            {
                print_separator(&osep, buff, bufc);
            }

            SAFE_LB_STR(objs[0], buff, bufc);
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
 * fun_map: iteratively evaluate an attribute with a list of arguments. >
 * &DIV_TWO object=fdiv(%0,2) > say map(1 2 3 4 5,object/div_two) You say
 * "0.5 1 1.5 2 2.5" > say map(object/div_two,1-2-3-4-5,-) You say
 * "0.5-1-1.5-2-2.5"
 *
 */

void fun_map(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, alen, anum;
    ATTR *ap;
    char *atext, *objs[2], *str, *cp, *atextbuf, *bb_p, *op;
    Delim isep, osep;
    int i;

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

    /*
     * If we don't have anything for a second arg, don't bother.
     */
    if (!fargs[1] || !*fargs[1])
    {
        return;
    }

    /*
     * Two possibilities for the second arg: <obj>/<attr> and <attr>.
     */
    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
    }
    /*
     * now process the list one element at a time
     */
    cp = trim_space_sep(fargs[1], &isep);
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    objs[1] = XMALLOC(SBUF_SIZE, "objs[1]");
    bb_p = *bufc;
    i = 1;

    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        objs[0] = split_token(&cp, &isep);
        op = objs[1];
        SAFE_LTOS(objs[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        str = atextbuf;
        exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);
        i++;
    }

    XFREE(atext);
    XFREE(atextbuf);
    XFREE(objs[1]);
}

/*
 * ---------------------------------------------------------------------------
 * fun_mix: Like map, but operates on two lists or more lists simultaneously,
 * passing the elements as %0, %1, %2, etc.
 */

void fun_mix(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, alen, anum, i, lastn, nwords, wc;
    ATTR *ap;
    char *str, *atext, *os[NUM_ENV_VARS], *atextbuf, *bb_p;
    Delim isep;
    char *cp[NUM_ENV_VARS];
    int count[NUM_ENV_VARS];
    /*
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

    /*
     * Get the attribute, check the permissions.
     */
    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
    }

    for (i = 0; i < NUM_ENV_VARS; i++)
    {
        cp[i] = NULL;
    }

    bb_p = *bufc;
    /*
     * process the lists, one element at a time.
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

    for (wc = 0; (wc < nwords) && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU(); wc++)
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
        exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, os, lastn);
    }

    XFREE(atext);
    XFREE(atextbuf);
}

/*
 * ---------------------------------------------------------------------------
 * fun_step: A little like a fusion of iter() and mix(), it takes elements of
 * a list X at a time and passes them into a single function as %0, %1, etc.
 * step(<attribute>,<list>,<step size>,<delim>,<outdelim>)
 */

void fun_step(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    ATTR *ap;
    dbref aowner, thing;
    int aflags, alen, anum;
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

    /*
     * Get attribute. Check permissions.
     */
    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
    }

    cp = trim_space_sep(fargs[1], &isep);
    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
    bb_p = *bufc;

    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
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
        exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(os[0]), i);
    }

    XFREE(atext);
    XFREE(atextbuf);
}

/*
 * ---------------------------------------------------------------------------
 * fun_foreach: like map(), but it operates on a string, rather than on a
 * list, calling a user-defined function for each character in the string. No
 * delimiter is inserted between the results.
 */

void fun_foreach(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref aowner, thing;
    int aflags, alen, anum, i;
    ATTR *ap;
    char *str, *atext, *atextbuf, *cp, *cbuf[2], *op;
    char start_token, end_token;
    int in_string = 1;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
    {
        return;
    }

    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
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

    i = -1; /* first letter in string is 0, not 1 */
    cbuf[1] = XMALLOC(SBUF_SIZE, "cbuf[1]");

    while (cp && *cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (!in_string)
        {
            /*
	     * Look for a start token.
	     */
            while (*cp && (*cp != start_token))
            {
                SAFE_LB_CHR(*cp, buff, bufc);
                cp++;
                i++;
            }

            if (!*cp)
            {
                break;
            }

            /*
	     * Skip to the next character. Don't copy the start
	     * token.
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
            /*
	     * We've found an end token. Skip over it. Note that
	     * it's possible to have a start and end token next
	     * to one another.
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
        SAFE_LTOS(cbuf[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext, alen);
        atextbuf[alen] = '\0';
        str = atextbuf;
        exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cbuf, 2);
    }

    XFREE(atextbuf);
    XFREE(atext);
    XFREE(cbuf[0]);
    XFREE(cbuf[1]);
}

/*
 * ---------------------------------------------------------------------------
 * fun_munge: combines two lists in an arbitrary manner.
 */

void fun_munge(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, alen, anum, nptrs1, nptrs2, nresults, i, j;
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

    /*
     * Find our object and attribute
     */
    if (string_prefix(fargs[0], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen((fargs[0]) + 8);
        __xstrcpy(atext, fargs[0] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, fargs[0], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str(fargs[0]);
        }
        if (!ap)
        {
            return;
        }
        atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
        if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
        {
            XFREE(atext);
            return;
        }
    }
    /*
     * Copy our lists and chop them up.
     */
    list1 = XMALLOC(LBUF_SIZE, "list1");
    list2 = XMALLOC(LBUF_SIZE, "list2");
    XSTRCPY(list1, fargs[1]);
    XSTRCPY(list2, fargs[2]);
    nptrs1 = list2arr(&ptrs1, LBUF_SIZE / 2, list1, &isep);
    nptrs2 = list2arr(&ptrs2, LBUF_SIZE / 2, list2, &isep);

    if (nptrs1 != nptrs2)
    {
        SAFE_LB_STR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
        XFREE(atext);
        XFREE(list1);
        XFREE(list2);
        XFREE(ptrs1);
        XFREE(ptrs2);
        return;
    }

    /*
     * Call the u-function with the first list as %0. Pass the input
     * separator as %1, which makes sorting, etc. easier.
     */
    st[0] = fargs[1];
    st[1] = XMALLOC(LBUF_SIZE, "st[1]");
    bp = st[1];
    print_separator(&isep, st[1], &bp);
    bp = rlist = XMALLOC(LBUF_SIZE, "rlist");
    str = atext;
    exec(rlist, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, st, 2);
    /*
     * Now that we have our result, put it back into array form. Search
     * through list1 until we find the element position, then copy the
     * corresponding element from list2.
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

                SAFE_LB_STR(ptrs2[j], buff, bufc);
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
 * fun_while: Evaluate a list until a termination condition is met:
 * while(EVAL_FN,CONDITION_FN,foo|flibble|baz|meep,1,|,-) where EVAL_FN is
 * "[strlen(%0)]" and CONDITION_FN is "[strmatch(%0,baz)]" would result in
 * '3-7-3' being returned. The termination condition is an EXACT not wild
 * match.
 */

void fun_while(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    dbref aowner1 = NOTHING, thing1 = NOTHING, aowner2 = NOTHING, thing2 = NOTHING;
    int aflags1 = 0, aflags2 = 0, anum1 = 0, anum2 = 0, alen1 = 0, alen2 = 0, i = 0, tmp_num = 0;
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

    /*
     * If our third arg is null (empty list), don't bother.
     */

    if (!fargs[2] || !*fargs[2])
    {
        return;
    }

    /*
     * Our first and second args can be <obj>/<attr> or just <attr>. Use
     * them if we can access them, otherwise return an empty string.
     *
     * Note that for user-defined attributes, atr_str() returns a pointer to
     * a static, and that therefore we have to be careful about what
     * we're doing.
     */
    if (parse_attrib(player, fargs[0], &thing1, &anum1, 0))
    {
        if ((anum1 == NOTHING) || !(Good_obj(thing1)))
            ap = NULL;
        else
            ap = atr_num(anum1);
    }
    else
    {
        thing1 = player;
        ap = atr_str(fargs[0]);
    }

    if (!ap)
    {
        return;
    }

    atext1 = atr_pget(thing1, ap->number, &aowner1, &aflags1, &alen1);

    if (!*atext1 || !(See_attr(player, thing1, ap, aowner1, aflags1)))
    {
        XFREE(atext1);
        return;
    }

    tmp_num = ap->number;

    if (parse_attrib(player, fargs[1], &thing2, &anum2, 0))
    {
        if ((anum2 == NOTHING) || !(Good_obj(thing2)))
            ap2 = NULL;
        else
            ap2 = atr_num(anum2);
    }
    else
    {
        thing2 = player;
        ap2 = atr_str(fargs[1]);
    }

    if (!ap2)
    {
        XFREE(atext1); /* we allocated this, remember? */
        return;
    }

    /*
     * If our evaluation and condition are the same, we can save
     * ourselves some time later. There are two possibilities: we have
     * the exact same obj/attr pair, or the attributes contain identical
     * text.
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

    /*
     * Process the list one element at a time.
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

    while (cp && (mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
    {
        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        objs[0] = split_token(&cp, &isep);
        op = objs[1];
        SAFE_LTOS(objs[1], &op, i, LBUF_SIZE);
        XMEMCPY(atextbuf, atext1, alen1);
        atextbuf[alen1] = '\0';
        str = atextbuf;
        savep = *bufc;
        exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);

        if (!is_same)
        {
            XMEMCPY(atextbuf, atext2, alen2);
            atextbuf[alen2] = '\0';
            dp = savep = condbuf;
            str = atextbuf;
            exec(condbuf, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2);
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
