/**
 * @file funvars_xvars.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief X-variable (user-defined variable) management functions
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

#include <ctype.h>
#include <string.h>
#include <pcre2.h>

/*
 * --------------------------------------------------------------------------
 * Auxiliary stuff for structures and variables.
 */

void print_htab_matches(dbref obj, HASHTAB *htab, char *buff, char **bufc)
{
    /*
     * Lists out hashtable matches. Things which use this are
     * computationally expensive, and should be discouraged.
     */
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *tp, *bb_p;
    HASHENT *hptr;
    int i, len;
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, obj, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);
    bb_p = *bufc;

    for (i = 0; i < htab->hashsize; i++)
    {
        for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next)
        {
            if (!strncmp(tbuf, hptr->target.s, len))
            {
                if (*bufc != bb_p)
                {
                    XSAFELBCHR(' ', buff, bufc);
                }

                XSAFELBSTR(strchr(hptr->target.s, '.') + 1, buff, bufc);
            }
        }
    }
    XFREE(tbuf);
}

/*
 * ---------------------------------------------------------------------------
 * fun_x: Returns a variable. x(<variable name>) fun_setx: Sets a variable.
 * setx(<variable name>,<value>) fun_store: Sets and returns a variable.
 * store(<variable name>,<value>) fun_xvars: Takes a list, parses it, sets it
 * into variables. xvars(<space-separated variable list>,<list>,<delimiter>)
 * fun_let: Takes a list of variables and their values, sets them, executes a
 * function, and clears out the variables. (Scheme/ML-like.) If <list> is
 * empty, the values are reset to null. let(<space-separated var
 * list>,<list>,<body>,<delimiter>) fun_lvars: Shows a list of variables
 * associated with that object. fun_clearvars: Clears all variables
 * associated with that object.
 */

void set_xvar(dbref obj, char *name, char *data)
{
    VARENT *xvar;
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *tp, *p;

    /*
     * If we don't have at least one character in the name, toss it.
     */

    if (!name || !*name)
    {
        XFREE(tbuf);
        return;
    }

    /*
     * Variable string is '<dbref number minus #>.<variable name>'. We
     * lowercase all names. Note that we're going to end up automatically
     * truncating long names.
     */
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, obj, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = name; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(name, tbuf, &tp);
    *tp = '\0';

    /*
     * Search for it. If it exists, replace it. If we get a blank string,
     * delete the variable.
     */

    if ((xvar = (VARENT *)hashfind(tbuf, &mushstate.vars_htab)))
    {
        if (xvar->text)
        {
            XFREE(xvar->text);
        }

        if (data && *data)
        {
            xvar->text = (char *)XMALLOC(sizeof(char) * (strlen(data) + 1), "xvar->text");

            if (!xvar->text)
            {
                XFREE(tbuf);
                return; /* out of memory */
            }

            XSTRCPY(xvar->text, data);
        }
        else
        {
            xvar->text = NULL;
            XFREE(xvar);
            hashdelete(tbuf, &mushstate.vars_htab);
            s_VarsCount(obj, VarsCount(obj) - 1);
        }
    }
    else
    {
        /*
         * We haven't found it. If it's non-empty, set it, provided
         * we're not running into a limit on the number of vars per
         * object.
         */
        if (VarsCount(obj) + 1 > mushconf.numvars_lim)
        {
            XFREE(tbuf);
            return;
        }

        if (data && *data)
        {
            xvar = (VARENT *)XMALLOC(sizeof(VARENT), "xvar");

            if (!xvar)
            {
                XFREE(tbuf);
                return; /* out of memory */
            }

            xvar->text = (char *)XMALLOC(sizeof(char) * (strlen(data) + 1), "xvar->text");

            if (!xvar->text)
            {
                XFREE(tbuf);
                return; /* out of memory */
            }

            XSTRCPY(xvar->text, data);
            hashadd(tbuf, (int *)xvar, &mushstate.vars_htab, 0);
            s_VarsCount(obj, VarsCount(obj) + 1);
            mushstate.max_vars = mushstate.vars_htab.entries > mushstate.max_vars ? mushstate.vars_htab.entries : mushstate.max_vars;
        }
    }
    XFREE(tbuf);
}

void clear_xvars(dbref obj, char **xvar_names, int n_xvars)
{
    /*
     * Clear out an array of variable names.
     */
    char *pre = XMALLOC(SBUF_SIZE, "pre");
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *tp, *p;
    VARENT *xvar;
    int i;
    /*
     * Build our dbref bit first.
     */
    tp = pre;
    XSAFELTOS(pre, &tp, obj, LBUF_SIZE);
    XSAFESBCHR('.', pre, &tp);
    *tp = '\0';

    /*
     * Go clear stuff.
     */

    for (i = 0; i < n_xvars; i++)
    {
        for (p = xvar_names[i]; *p; p++)
        {
            *p = tolower(*p);
        }

        tp = tbuf;
        XSAFESBSTR(pre, tbuf, &tp);
        XSAFESBSTR(xvar_names[i], tbuf, &tp);
        *tp = '\0';

        if ((xvar = (VARENT *)hashfind(tbuf, &mushstate.vars_htab)))
        {
            if (xvar->text)
            {
                XFREE(xvar->text);
                xvar->text = NULL;
            }

            XFREE(xvar);
            hashdelete(tbuf, &mushstate.vars_htab);
        }
    }

    s_VarsCount(obj, VarsCount(obj) - n_xvars);
    XFREE(pre);
    XFREE(tbuf);
}

void xvars_clr(dbref player)
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *tp;
    HASHTAB *htab;
    HASHENT *hptr, *last, *next;
    int i, len;
    VARENT *xvar;
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);
    htab = &mushstate.vars_htab;

    for (i = 0; i < htab->hashsize; i++)
    {
        last = NULL;

        for (hptr = htab->entry[i]; hptr != NULL; hptr = next)
        {
            next = hptr->next;

            if (!strncmp(tbuf, hptr->target.s, len))
            {
                if (last == NULL)
                {
                    htab->entry[i] = next;
                }
                else
                {
                    last->next = next;
                }

                xvar = (VARENT *)hptr->data;
                XFREE(xvar->text);
                XFREE(xvar);
                XFREE(hptr->target.s);
                XFREE(hptr);
                htab->deletes++;
                htab->entries--;

                if (htab->entry[i] == NULL)
                {
                    htab->nulls++;
                }
            }
            else
            {
                last = hptr;
            }
        }
    }

    s_VarsCount(player, 0);
    XFREE(tbuf);
}

void fun_x(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    VARENT *xvar;
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *tp, *p;
    /*
     * Variable string is '<dbref number minus #>.<variable name>'
     */
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[0], tbuf, &tp);
    *tp = '\0';

    if ((xvar = (VARENT *)hashfind(tbuf, &mushstate.vars_htab)))
    {
        XSAFELBSTR(xvar->text, buff, bufc);
    }
    XFREE(tbuf);
}

void fun_setx(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    set_xvar(player, fargs[0], fargs[1]);
}

void fun_store(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    set_xvar(player, fargs[0], fargs[1]);
    XSAFELBSTR(fargs[1], buff, bufc);
}

void fun_xvars(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char **xvar_names, **elems;
    int n_xvars, n_elems;
    char *varlist, *elemlist;
    int i;
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
    {
        return;
    }

    varlist = XMALLOC(LBUF_SIZE, "varlist");
    XSTRCPY(varlist, fargs[0]);
    n_xvars = list2arr(&xvar_names, LBUF_SIZE / 2, varlist, &SPACE_DELIM);

    if (n_xvars == 0)
    {
        XFREE(varlist);
        XFREE(xvar_names);
        return;
    }

    if (!fargs[1] || !*fargs[1])
    {
        /*
         * Empty list, clear out the data.
         */
        clear_xvars(player, xvar_names, n_xvars);
        XFREE(varlist);
        XFREE(xvar_names);
        return;
    }

    elemlist = XMALLOC(LBUF_SIZE, "elemlist");
    XSTRCPY(elemlist, fargs[1]);
    n_elems = list2arr(&elems, LBUF_SIZE / 2, elemlist, &isep);

    if (n_elems != n_xvars)
    {
        XSAFELBSTR("#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc);
        XFREE(varlist);
        XFREE(elemlist);
        XFREE(xvar_names);
        XFREE(elems);
        return;
    }

    for (i = 0; i < n_elems; i++)
    {
        set_xvar(player, xvar_names[i], elems[i]);
    }

    XFREE(varlist);
    XFREE(elemlist);
    XFREE(xvar_names);
    XFREE(elems);
}

void fun_let(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char **xvar_names, **elems;
    char *old_xvars[LBUF_SIZE / 2];
    int n_xvars, n_elems;
    char *varlist, *elemlist;
    char *str, *bp, *p, *tp;
    char *pre = XMALLOC(SBUF_SIZE, "pre");
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    VARENT *xvar;
    int i;
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    if (!fargs[0] || !*fargs[0])
    {
        XFREE(tbuf);
        XFREE(pre);
        return;
    }

    varlist = bp = XMALLOC(LBUF_SIZE, "varlist");
    str = fargs[0];
    eval_expression_string(varlist, &bp, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
    n_xvars = list2arr(&xvar_names, LBUF_SIZE / 2, varlist, &SPACE_DELIM);

    if (n_xvars == 0)
    {
        XFREE(varlist);
        XFREE(xvar_names);
        XFREE(tbuf);
        XFREE(pre);
        return;
    }

    /*
     * Lowercase our variable names.
     */

    for (i = 0, p = xvar_names[i]; *p; p++)
    {
        *p = tolower(*p);
    }

    /*
     * Save our original values. Copying this stuff into an array is
     * unnecessarily expensive because we allocate and free memory that
     * we could theoretically just trade pointers around for -- but this
     * way is cleaner.
     */
    tp = pre;
    XSAFELTOS(pre, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', pre, &tp);
    *tp = '\0';

    for (i = 0; i < n_xvars; i++)
    {
        tp = tbuf;
        XSAFESBSTR(pre, tbuf, &tp);
        XSAFESBSTR(xvar_names[i], tbuf, &tp);
        *tp = '\0';

        if ((xvar = (VARENT *)hashfind(tbuf, &mushstate.vars_htab)))
        {
            if (xvar->text)
                old_xvars[i] = XSTRDUP(xvar->text, "old_xvars[i]");
            else
            {
                old_xvars[i] = NULL;
            }
        }
        else
        {
            old_xvars[i] = NULL;
        }
    }

    if (fargs[1] && *fargs[1])
    {
        /*
         * We have data, so we should initialize variables to their
         * values, ala xvars(). However, unlike xvars(), if we don't
         * get a list, we just leave the values alone (we don't clear
         * them out).
         */
        elemlist = bp = XMALLOC(LBUF_SIZE, "elemlist");
        str = fargs[1];
        eval_expression_string(elemlist, &bp, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
        n_elems = list2arr(&elems, LBUF_SIZE / 2, elemlist, &isep);

        if (n_elems != n_xvars)
        {
            XSAFELBSTR("#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc);
            XFREE(varlist);
            XFREE(elemlist);

            for (i = 0; i < n_xvars; i++)
            {
                if (old_xvars[i])
                {
                    XFREE(old_xvars[i]);
                }
            }

            XFREE(xvar_names);
            XFREE(elems);
            XFREE(tbuf);
            XFREE(pre);
            return;
        }

        for (i = 0; i < n_elems; i++)
        {
            set_xvar(player, xvar_names[i], elems[i]);
        }

        XFREE(elemlist);
    }

    /*
     * Now we go to execute our function body.
     */
    str = fargs[2];
    eval_expression_string(buff, bufc, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);

    /*
     * Restore the old values.
     */

    for (i = 0; i < n_xvars; i++)
    {
        set_xvar(player, xvar_names[i], old_xvars[i]);

        if (old_xvars[i])
        {
            XFREE(old_xvars[i]);
        }
    }

    XFREE(varlist);
    XFREE(xvar_names);
    XFREE(elems);
    XFREE(tbuf);
    XFREE(pre);
}

void fun_lvars(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    print_htab_matches(player, &mushstate.vars_htab, buff, bufc);
}

void fun_clearvars(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    /*
     * This is computationally expensive. Necessary, but its use should
     * be avoided if possible.
     */
    xvars_clr(player);
}

/*
 * ---------------------------------------------------------------------------
 * Structures.
 */

int istype_char(char *str)
{
    if (strlen(str) == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int istype_dbref(char *str)
{
    dbref it;

    if (*str++ != NUMBER_TOKEN)
    {
        return 0;
    }

    if (*str)
    {
        it = parse_dbref_only(str);
        return (Good_obj(it));
    }

    return 0;
}

int istype_int(char *str)
{
    return (is_integer(str));
}

int istype_float(char *str)
{
    return (is_number(str));
}

int istype_string(char *str)
{
    char *p;

    for (p = str; *p; p++)
    {
        if (isspace(*p))
        {
            return 0;
        }
    }

    return 1;
}

