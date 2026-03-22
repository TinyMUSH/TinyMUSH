/**
 * @file funiter_apply.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Higher-order iteration functions (fold, filter, map, mix, step)
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

/* load_iter_attrib is defined in funiter.c */
extern int load_iter_attrib(dbref player, char *farg, dbref *thing, ATTR **ap, char **atext, dbref *aowner, int *aflags, int *alen);

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

