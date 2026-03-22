/**
 * @file funiter_compose.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Compositional iteration functions (foreach, munge, while)
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
void fun_foreach(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
