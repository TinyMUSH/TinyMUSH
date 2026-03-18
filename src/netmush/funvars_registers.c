/**
 * @file funvars_registers.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Q-register management functions
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

int set_register(const char *funcname, char *name, char *data)
{
    /*
     * Return number of characters set. -1 indicates a name error. -2
     * indicates that a limit was exceeded.
     */
    int i, regnum, len, a_size, *tmp_lens;
    char *p, **tmp_regs;

    if (!name || !*name)
    {
        return -1;
    }

    if (name[1] == '\0')
    {
        /*
         * Single-letter q-register. We allocate these either as a
         * block of 10 or a block of 36. (Most code won't go beyond
         * %q0-%q9, especially legacy code which predates the larger
         * number of global registers.)
         */
        regnum = qidx_chartab((unsigned char)*name);

        if ((regnum < 0) || (regnum >= mushconf.max_global_regs))
        {
            return -1;
        }

        /*
         * Check to see if we're just clearing. If we're clearing a
         * register that doesn't exist, then we do nothing. Otherwise
         * we wipe out the data.
         */

        if (!data || !*data)
        {
            if (!mushstate.rdata || !mushstate.rdata->q_alloc || (regnum >= mushstate.rdata->q_alloc))
            {
                return 0;
            }

            if (mushstate.rdata->q_regs[regnum])
            {
                XFREE(mushstate.rdata->q_regs[regnum]);
                mushstate.rdata->q_regs[regnum] = NULL;
                mushstate.rdata->q_lens[regnum] = 0;
                mushstate.rdata->dirty++;
            }

            return 0;
        }

        /*
         * We're actually setting a register. Take care of allocating
         * space first.
         */

        if (!mushstate.rdata)
        {
            mushstate.rdata = (GDATA *)XMALLOC(sizeof(GDATA), funcname);
            mushstate.rdata->q_alloc = mushstate.rdata->xr_alloc = 0;
            mushstate.rdata->q_regs = mushstate.rdata->x_names = mushstate.rdata->x_regs = NULL;
            mushstate.rdata->q_lens = mushstate.rdata->x_lens = NULL;
            mushstate.rdata->dirty = 0;
        }

        if (!mushstate.rdata->q_alloc)
        {
            a_size = (regnum < 10) ? 10 : mushconf.max_global_regs;
            mushstate.rdata->q_alloc = a_size;
            mushstate.rdata->q_regs = XCALLOC(a_size, sizeof(char *), "mushstate.rdata->q_regs");
            mushstate.rdata->q_lens = XCALLOC(a_size, sizeof(int), "mushstate.rdata->q_lens");
            mushstate.rdata->q_alloc = a_size;
        }
        else if (regnum >= mushstate.rdata->q_alloc)
        {
            a_size = mushconf.max_global_regs;
            tmp_regs = XREALLOC(mushstate.rdata->q_regs, a_size * sizeof(char *), "tmp_regs");
            tmp_lens = XREALLOC(mushstate.rdata->q_lens, a_size * sizeof(int), "tmp_lens");
            XMEMSET(&tmp_regs[mushstate.rdata->q_alloc], (int)0, (a_size - mushstate.rdata->q_alloc) * sizeof(char *));
            XMEMSET(&tmp_lens[mushstate.rdata->q_alloc], (int)0, (a_size - mushstate.rdata->q_alloc) * sizeof(int));
            mushstate.rdata->q_regs = tmp_regs;
            mushstate.rdata->q_lens = tmp_lens;
            mushstate.rdata->q_alloc = a_size;
        }

        /*
         * Set it.
         */

        if (!mushstate.rdata->q_regs[regnum])
        {
            mushstate.rdata->q_regs[regnum] = XMALLOC(LBUF_SIZE, "mushstate.rdata->q_regs[regnum]");
        }

        len = strlen(data);
        XMEMCPY(mushstate.rdata->q_regs[regnum], data, len + 1);
        mushstate.rdata->q_lens[regnum] = len;
        mushstate.rdata->dirty++;
        return len;
    }

    /*
     * We have an arbitrarily-named register. Check for data-clearing
     * first, since that's easier.
     */

    if (!data || !*data)
    {
        if (!mushstate.rdata || !mushstate.rdata->xr_alloc)
        {
            return 0;
        }

        for (p = name; *p; p++)
        {
            *p = tolower(*p);
        }

        for (i = 0; i < mushstate.rdata->xr_alloc; i++)
        {
            if (mushstate.rdata->x_names[i] && !strcmp(name, mushstate.rdata->x_names[i]))
            {
                if (mushstate.rdata->x_regs[i])
                {
                    XFREE(mushstate.rdata->x_names[i]);
                    mushstate.rdata->x_names[i] = NULL;
                    XFREE(mushstate.rdata->x_regs[i]);
                    mushstate.rdata->x_regs[i] = NULL;
                    mushstate.rdata->x_lens[i] = 0;
                    mushstate.rdata->dirty++;
                    return 0;
                }
                else
                {
                    return 0;
                }
            }
        }

        return 0; /* register unset, so just return */
    }

    /*
     * Check for a valid name. We enforce names beginning with a letter,
     * in case we want to do something special with naming conventions at
     * some later date. We also limit the characters than can go into a
     * name.
     */

    if (strlen(name) >= SBUF_SIZE)
    {
        return -1;
    }

    if (!isalpha(*name))
    {
        return -1;
    }

    for (p = name; *p; p++)
    {
        if (isalnum(*p) || (*p == '_') || (*p == '-') || (*p == '.') || (*p == '#'))
        {
            *p = tolower(*p);
        }
        else
        {
            return -1;
        }
    }

    len = strlen(data);

    /*
     * If we have no existing data, life is easy; just set it.
     */

    if (!mushstate.rdata)
    {
        mushstate.rdata = (GDATA *)XMALLOC(sizeof(GDATA), (funcname));
        mushstate.rdata->q_alloc = mushstate.rdata->xr_alloc = 0;
        mushstate.rdata->q_regs = mushstate.rdata->x_names = mushstate.rdata->x_regs = NULL;
        mushstate.rdata->q_lens = mushstate.rdata->x_lens = NULL;
        mushstate.rdata->dirty = 0;
    }

    if (!mushstate.rdata->xr_alloc)
    {
        a_size = NUM_ENV_VARS;
        mushstate.rdata->x_names = XCALLOC(a_size, sizeof(char *), "mushstate.rdata->x_names");
        mushstate.rdata->x_regs = XCALLOC(a_size, sizeof(char *), "mushstate.rdata->x_regs");
        mushstate.rdata->x_lens = XCALLOC(a_size, sizeof(int), "mushstate.rdata->x_lens");
        mushstate.rdata->xr_alloc = a_size;
        mushstate.rdata->x_names[0] = XMALLOC(SBUF_SIZE, "mushstate.rdata->x_names[0]");
        XSTRCPY(mushstate.rdata->x_names[0], name);
        mushstate.rdata->x_regs[0] = XMALLOC(LBUF_SIZE, "mushstate.rdata->x_regs[0]");
        XMEMCPY(mushstate.rdata->x_regs[0], data, len + 1);
        mushstate.rdata->x_lens[0] = len;
        mushstate.rdata->dirty++;
        return len;
    }

    /*
     * Search for an existing entry to replace.
     */

    for (i = 0; i < mushstate.rdata->xr_alloc; i++)
    {
        if (mushstate.rdata->x_names[i] && !strcmp(name, mushstate.rdata->x_names[i]))
        {
            XMEMCPY(mushstate.rdata->x_regs[i], data, len + 1);
            mushstate.rdata->x_lens[i] = len;
            mushstate.rdata->dirty++;
            return len;
        }
    }

    /*
     * Check for an empty cell to insert into.
     */

    for (i = 0; i < mushstate.rdata->xr_alloc; i++)
    {
        if (mushstate.rdata->x_names[i] == NULL)
        {
            mushstate.rdata->x_names[i] = XMALLOC(SBUF_SIZE, "mushstate.rdata->x_names[i]");
            XSTRCPY(mushstate.rdata->x_names[i], name);

            if (!mushstate.rdata->x_regs[i]) /* should never happen */
                mushstate.rdata->x_regs[i] = XMALLOC(LBUF_SIZE, "mushstate.rdata->x_regs[i]");

            XMEMCPY(mushstate.rdata->x_regs[i], data, len + 1);
            mushstate.rdata->x_lens[i] = len;
            mushstate.rdata->dirty++;
            return len;
        }
    }

    /*
     * Oops. We're out of room in our existing array. Go allocate more
     * space, unless we're at our limit.
     */
    regnum = mushstate.rdata->xr_alloc;
    a_size = regnum + NUM_ENV_VARS;

    if (a_size > mushconf.register_limit)
    {
        a_size = mushconf.register_limit;

        if (a_size <= regnum)
        {
            return -2;
        }
    }

    tmp_regs = (char **)XREALLOC(mushstate.rdata->x_names, a_size * sizeof(char *), "tmp_regs");
    mushstate.rdata->x_names = tmp_regs;
    tmp_regs = (char **)XREALLOC(mushstate.rdata->x_regs, a_size * sizeof(char *), "tmp_regs");
    mushstate.rdata->x_regs = tmp_regs;
    tmp_lens = (int *)XREALLOC(mushstate.rdata->x_lens, a_size * sizeof(int), "tmp_lens");
    mushstate.rdata->x_lens = tmp_lens;
    XMEMSET(&mushstate.rdata->x_names[mushstate.rdata->xr_alloc], (int)0, (a_size - mushstate.rdata->xr_alloc) * sizeof(char *));
    XMEMSET(&mushstate.rdata->x_regs[mushstate.rdata->xr_alloc], (int)0, (a_size - mushstate.rdata->xr_alloc) * sizeof(char *));
    XMEMSET(&mushstate.rdata->x_lens[mushstate.rdata->xr_alloc], (int)0, (a_size - mushstate.rdata->xr_alloc) * sizeof(int));
    mushstate.rdata->xr_alloc = a_size;
    /*
     * Now we know we can insert into the first empty.
     */
    mushstate.rdata->x_names[regnum] = XMALLOC(SBUF_SIZE, "mushstate.rdata->x_names[regnum]");
    XSTRCPY(mushstate.rdata->x_names[regnum], name);
    mushstate.rdata->x_regs[regnum] = XMALLOC(LBUF_SIZE, "mushstate.rdata->x_regs[regnum]");
    XMEMCPY(mushstate.rdata->x_regs[regnum], data, len + 1);
    mushstate.rdata->x_lens[regnum] = len;
    mushstate.rdata->dirty++;
    return len;
}

char *get_register(GDATA *g, char *r)
{
    /*
     * Given a pointer to a register data structure, and the name of a
     * register, return a pointer to the string value of that register.
     * This may modify r, turning it lowercase.
     */
    int regnum;
    char *p;

    if (!g || !r || !*r)
    {
        return NULL;
    }

    if (r[1] == '\0')
    {
        regnum = qidx_chartab((unsigned char)r[0]);

        if ((regnum < 0) || (regnum >= mushconf.max_global_regs))
        {
            return NULL;
        }
        else if ((g->q_alloc > regnum) && g->q_regs[regnum])
        {
            return g->q_regs[regnum];
        }

        return NULL;
    }

    if (!g->xr_alloc)
    {
        return NULL;
    }

    for (p = r; *p; p++)
    {
        *p = tolower(*p);
    }

    for (regnum = 0; regnum < g->xr_alloc; regnum++)
    {
        if (g->x_names[regnum] && !strcmp(r, g->x_names[regnum]))
        {
            if (g->x_regs[regnum])
            {
                return g->x_regs[regnum];
            }

            return NULL;
        }
    }

    return NULL;
}

void fun_setq(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int result, count, i;

    if (nfargs < 2)
    {
        XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (SETQ) EXPECTS AT LEAST 2 ARGUMENTS BUT GOT %d", nfargs);
        return;
    }

    if (nfargs % 2 != 0)
    {
        XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (SETQ) EXPECTS AN EVEN NUMBER OF ARGUMENTS BUT GOT %d", nfargs);
        return;
    }

    if (nfargs > MAX_NFARGS - 2)
    {
        /*
         * Prevent people from doing something dumb by providing this
         * too many arguments and thus having the fifteenth register
         * contain the remaining args. Cut them off at the
         * fourteenth.
         */
        XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (SETQ) EXPECTS NO MORE THAN %d ARGUMENTS BUT GOT %d", MAX_NFARGS - 2, nfargs);
        return;
    }

    if (nfargs == 2)
    {
        result = set_register("fun_setq", fargs[0], fargs[1]);

        if (result == -1)
        {
            XSAFELBSTR("#-1 INVALID GLOBAL REGISTER", buff, bufc);
        }
        else if (result == -2)
        {
            XSAFELBSTR("#-1 REGISTER LIMIT EXCEEDED", buff, bufc);
        }

        return;
    }

    count = 0;

    for (i = 0; i < nfargs; i += 2)
    {
        result = set_register("fun_setq", fargs[i], fargs[i + 1]);

        if (result < 0)
        {
            count++;
        }
    }

    if (count > 0)
    {
        XSAFESPRINTF(buff, bufc, "#-1 ENCOUNTERED %d ERRORS", count);
    }
}

void fun_setr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int result;
    result = set_register("fun_setr", fargs[0], fargs[1]);

    if (result == -1)
    {
        XSAFELBSTR("#-1 INVALID GLOBAL REGISTER", buff, bufc);
    }
    else if (result == -2)
    {
        XSAFELBSTR("#-1 REGISTER LIMIT EXCEEDED", buff, bufc);
    }
    else if (result > 0)
    {
        XSAFESTRNCAT(buff, bufc, fargs[1], result, LBUF_SIZE);
    }
}

void read_register(char *regname, char *buff, char **bufc)
{
    int regnum;
    char *p;

    if (regname[1] == '\0')
    {
        regnum = qidx_chartab((unsigned char)*regname);

        if ((regnum < 0) || (regnum >= mushconf.max_global_regs))
        {
            XSAFELBSTR("#-1 INVALID GLOBAL REGISTER", buff, bufc);
        }
        else
        {
            if (mushstate.rdata && (mushstate.rdata->q_alloc > regnum) && mushstate.rdata->q_regs[regnum])
            {
                XSAFESTRNCAT(buff, bufc, mushstate.rdata->q_regs[regnum], mushstate.rdata->q_lens[regnum], LBUF_SIZE);
            }
        }

        return;
    }

    if (!mushstate.rdata || !mushstate.rdata->xr_alloc)
    {
        return;
    }

    for (p = regname; *p; p++)
    {
        *p = tolower(*p);
    }

    for (regnum = 0; regnum < mushstate.rdata->xr_alloc; regnum++)
    {
        if (mushstate.rdata->x_names[regnum] && !strcmp(regname, mushstate.rdata->x_names[regnum]))
        {
            if (mushstate.rdata->x_regs[regnum])
            {
                XSAFESTRNCAT(buff, bufc, mushstate.rdata->x_regs[regnum], mushstate.rdata->x_lens[regnum], LBUF_SIZE);
                return;
            }
        }
    }
}

void fun_r(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    read_register(fargs[0], buff, bufc);
}

/*
 * --------------------------------------------------------------------------
 * lregs: List all the non-empty q-registers.
 */

void fun_lregs(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i;
    GDATA *g;
    char *bb_p;

    if (!mushstate.rdata)
    {
        return;
    }

    bb_p = *bufc;
    g = mushstate.rdata;

    for (i = 0; i < g->q_alloc; i++)
    {
        if (g->q_regs[i] && *(g->q_regs[i]))
        {
            if (*bufc != bb_p)
            {
                print_separator(&SPACE_DELIM, buff, bufc);
            }

            XSAFELBCHR(qidx_str(i), buff, bufc);
        }
    }

    for (i = 0; i < g->xr_alloc; i++)
    {
        if (g->x_names[i] && *(g->x_names[i]) && g->x_regs[i] && *(g->x_regs[i]))
        {
            if (*bufc != bb_p)
            {
                print_separator(&SPACE_DELIM, buff, bufc);
            }

            XSAFELBSTR(g->x_names[i], buff, bufc);
        }
    }
}

/*
 * --------------------------------------------------------------------------
 * wildmatch: Set the results of a wildcard match into the global registers.
 * wildmatch(<string>,<wildcard pattern>,<register list>)
 */

void fun_wildmatch(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, nqregs;
    char *t_args[NUM_ENV_VARS], **qregs; /* %0-%9 is limiting */

    if (!wild(fargs[1], fargs[0], t_args, NUM_ENV_VARS))
    {
        XSAFELBCHR('0', buff, bufc);
        return;
    }

    XSAFELBCHR('1', buff, bufc);
    /*
     * Parse the list of registers. Anything that we don't get is assumed
     * to be -1. Fill them in.
     */
    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);

    for (i = 0; i < nqregs; i++)
    {
        set_register("fun_wildmatch", qregs[i], t_args[i]);
    }

    /*
     * Need to free up allocated memory from the match.
     */

    for (i = 0; i < NUM_ENV_VARS; i++)
    {
        if (t_args[i])
        {
            XFREE(t_args[i]);
        }
    }

    XFREE(qregs);
}

/*
 * --------------------------------------------------------------------------
 * qvars: Set the contents of a list into a named list of global registers
 * qvars(<register list>,<list of elements>[,<input delim>])
 */

void fun_qvars(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, nqregs, n_elems;
    char **qreg_names, **elems;
    char *varlist, *elemlist;
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
    {
        return;
    }

    if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1])
    {
        return;
    }

    varlist = XMALLOC(LBUF_SIZE, "varlist");
    XSTRCPY(varlist, fargs[0]);
    nqregs = list2arr(&qreg_names, LBUF_SIZE / 2, varlist, &SPACE_DELIM);

    if (nqregs == 0)
    {
        XFREE(varlist);
        XFREE(qreg_names);
        return;
    }

    elemlist = XMALLOC(LBUF_SIZE, "elemlist");
    XSTRCPY(elemlist, fargs[1]);
    n_elems = list2arr(&elems, LBUF_SIZE / 2, elemlist, &isep);

    if (n_elems != nqregs)
    {
        XSAFELBSTR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
        XFREE(varlist);
        XFREE(elemlist);
        XFREE(qreg_names);
        XFREE(elems);
        return;
    }

    for (i = 0; i < n_elems; i++)
    {
        set_register("fun_qvars", qreg_names[i], elems[i]);
    }

    XFREE(varlist);
    XFREE(elemlist);
    XFREE(qreg_names);
    XFREE(elems);
}

/*---------------------------------------------------------------------------
 * fun_qsub: "Safe" substitution using $name$ dollar-variables.
 *           Can specify beginning and ending variable markers.
 */

void fun_qsub(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *nextp, *strp;
    Delim bdelim, edelim;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 3, buff, bufc))
    {
        return;
    }

    if (!fargs[0] || !*fargs[0])
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &bdelim, DELIM_STRING))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &edelim, DELIM_STRING))
    {
        return;
    }

    /*
     * Defaulted space delims are actually $
     */
    if ((bdelim.len == 1) && (bdelim.str[0] == ' '))
    {
        bdelim.str[0] = '$';
    }

    if ((edelim.len == 1) && (edelim.str[0] == ' '))
    {
        edelim.str[0] = '$';
    }

    nextp = fargs[0];

    while (nextp && ((strp = split_token(&nextp, &bdelim)) != NULL))
    {
        XSAFELBSTR(strp, buff, bufc);

        if (nextp)
        {
            strp = split_token(&nextp, &edelim);
            read_register(strp, buff, bufc);
        }
    }
}

/*---------------------------------------------------------------------------
 * fun_nofx: Prevent certain types of side-effects.
 */

int calc_limitmask(char *lstr)
{
    char *p;
    int lmask = 0;

    for (p = lstr; *p; p++)
    {
        switch (*p)
        {
        case 'd':
        case 'D':
            lmask |= FN_DBFX;
            break;

        case 'q':
        case 'Q':
            lmask |= FN_QFX;
            break;

        case 'o':
        case 'O':
            lmask |= FN_OUTFX;
            break;

        case 'v':
        case 'V':
            lmask |= FN_VARFX;
            break;

        case 's':
        case 'S':
            lmask |= FN_STACKFX;
            break;

        case ' ':
            /*
             * ignore spaces
             */
            /*
             * EMPTY
             */
            break;

        default:
            return -1;
        }
    }

    return lmask;
}

void fun_nofx(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int save_state, lmask;
    char *str;
    lmask = calc_limitmask(fargs[0]);

    if (lmask == -1)
    {
        XSAFESTRNCAT(buff, bufc, "#-1 INVALID LIMIT", 17, LBUF_SIZE);
        return;
    }

    save_state = mushstate.f_limitmask;
    mushstate.f_limitmask |= lmask;
    str = fargs[1];
    eval_expression_string(buff, bufc, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
    mushstate.f_limitmask = save_state;
}

/*
 * ---------------------------------------------------------------------------
 * ucall: Call a u-function, passing through only certain local registers,
 * and restoring certain local registers afterwards.
 *
 * ucall(<register names to pass thru>,<registers to keep local>,<o/a>,<args>)
 * sandbox(<object>,<limiter>,<reg pass>,<reg keep>,<o/a>,<args>)
 *
 * Registers to pass through to function @_ to pass through all, @_ <list> to
 * pass through all except <list> blank to pass through none, <list> to pass
 * through just those Registers whose value should be local (restored to
 * value pre-function call) @_ to restore all, @_ <list> to restore all
 * except <list> blank to restore none, <list> to keep just those on the list
 * @_! to return to original values (like a uprivate() would) @_! <list> to
 * return to original values, keeping new values on <list>
 */

char is_in_array(char *word, char **list, int list_length)
{
    int n;

    for (n = 0; n < list_length; n++)
        if (!strcasecmp(word, list[n]))
        {
            return 1;
        }

    return 0;
}

void handle_ucall(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner = NOTHING, thing = NOTHING, obj = NOTHING;
    ATTR *ap = NULL;
    GDATA *preserve = NULL, *tmp = NULL;
    char *atext = NULL, *str = NULL, *callp = NULL, *call_list = NULL, *cbuf = NULL, **cregs = NULL;
    int aflags = 0, alen = 0, anum = 0, trace_flag = 0, i = 0, ncregs = 0, lmask = 0, save_state = 0, extra_args = 0;

    /*
     * Three arguments to ucall(), five to sandbox()
     */

    if (nfargs < 3)
    {
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
        return;
    }

    if (Is_Func(UCALL_SANDBOX) && (nfargs < 5))
    {
        XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
        return;
    }

    /*
     * Figure our our limits
     */

    if (Is_Func(UCALL_SANDBOX))
    {
        lmask = calc_limitmask(fargs[1]);

        if (lmask == -1)
        {
            XSAFESTRNCAT(buff, bufc, "#-1 INVALID LIMIT", 17, LBUF_SIZE);
            return;
        }

        save_state = mushstate.f_limitmask;
        mushstate.f_limitmask |= lmask;
    }

    /*
     * Save everything to start with, then construct our pass-in
     */
    preserve = save_global_regs("fun_ucall.save");

    if (Is_Func(UCALL_SANDBOX))
    {
        callp = Eat_Spaces(fargs[2]);
    }
    else
    {
        callp = Eat_Spaces(fargs[0]);
    }

    if (!*callp)
    {
        if (mushstate.rdata)
        {
            for (int z = 0; z < mushstate.rdata->q_alloc; z++)
            {
                if (mushstate.rdata->q_regs[z])
                    XFREE(mushstate.rdata->q_regs[z]);
            }

            for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
            {
                if (mushstate.rdata->x_names[z])
                    XFREE(mushstate.rdata->x_names[z]);

                if (mushstate.rdata->x_regs[z])
                    XFREE(mushstate.rdata->x_regs[z]);
            }

            if (mushstate.rdata->q_regs)
            {
                XFREE(mushstate.rdata->q_regs);
            }

            if (mushstate.rdata->q_lens)
            {
                XFREE(mushstate.rdata->q_lens);
            }

            if (mushstate.rdata->x_names)
            {
                XFREE(mushstate.rdata->x_names);
            }

            if (mushstate.rdata->x_regs)
            {
                XFREE(mushstate.rdata->x_regs);
            }

            if (mushstate.rdata->x_lens)
            {
                XFREE(mushstate.rdata->x_lens);
            }

            XFREE(mushstate.rdata);
        }

        mushstate.rdata = NULL;
    }
    else if (!strcmp(callp, "@_"))
    {
        /*
         * Pass everything in
         */
        /*
         * EMPTY
         */
    }
    else if (!strncmp(callp, "@_ ", 3) && callp[3])
    {
        /*
         * Pass in everything EXCEPT the named registers
         */
        call_list = XMALLOC(LBUF_SIZE, "call_list");
        XSTRCPY(call_list, callp + 3);
        ncregs = list2arr(&cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM);

        for (i = 0; i < ncregs; i++)
        {
            set_register("fun_ucall", cregs[i], NULL);
        }

        XFREE(call_list);
    }
    else
    {
        /*
         * Pass in ONLY the named registers
         */
        if (mushstate.rdata)
        {
            for (int z = 0; z < mushstate.rdata->q_alloc; z++)
            {
                if (mushstate.rdata->q_regs[z])
                    XFREE(mushstate.rdata->q_regs[z]);
            }

            for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
            {
                if (mushstate.rdata->x_names[z])
                    XFREE(mushstate.rdata->x_names[z]);

                if (mushstate.rdata->x_regs[z])
                    XFREE(mushstate.rdata->x_regs[z]);
            }

            if (mushstate.rdata->q_regs)
            {
                XFREE(mushstate.rdata->q_regs);
            }

            if (mushstate.rdata->q_lens)
            {
                XFREE(mushstate.rdata->q_lens);
            }

            if (mushstate.rdata->x_names)
            {
                XFREE(mushstate.rdata->x_names);
            }

            if (mushstate.rdata->x_regs)
            {
                XFREE(mushstate.rdata->x_regs);
            }

            if (mushstate.rdata->x_lens)
            {
                XFREE(mushstate.rdata->x_lens);
            }

            XFREE(mushstate.rdata);
        }
        mushstate.rdata = NULL;
        call_list = XMALLOC(LBUF_SIZE, "call_list");
        XSTRCPY(call_list, callp);
        ncregs = list2arr(&cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM);

        for (i = 0; i < ncregs; i++)
        {
            set_register("fun_ucall", cregs[i], get_register(preserve, cregs[i]));
        }

        XFREE(call_list);
    }

    /*
     * What to call: <obj>/<attr> or <attr> or #lambda/<code>
     */
    if (string_prefix((Is_Func(UCALL_SANDBOX)) ? fargs[4] : fargs[2], "#lambda/"))
    {
        thing = player;
        anum = NOTHING;
        ap = NULL;
        atext = XMALLOC(LBUF_SIZE, "lambda.atext");
        alen = strlen(((Is_Func(UCALL_SANDBOX)) ? fargs[4] : fargs[2]) + 8);
        __xstrcpy(atext, (Is_Func(UCALL_SANDBOX)) ? fargs[4] : fargs[2] + 8);
        atext[alen] = '\0';
        aowner = player;
        aflags = 0;
    }
    else
    {
        if (parse_attrib(player, (Is_Func(UCALL_SANDBOX)) ? fargs[4] : fargs[2], &thing, &anum, 0))
        {
            if ((anum == NOTHING) || !(Good_obj(thing)))
                ap = NULL;
            else
                ap = atr_num(anum);
        }
        else
        {
            thing = player;
            ap = atr_str((Is_Func(UCALL_SANDBOX)) ? fargs[4] : fargs[2]);
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
     * Find our perspective
     */

    if (Is_Func(UCALL_SANDBOX))
    {
        obj = match_thing(player, fargs[0]);

        if (Cannot_Objeval(player, obj))
        {
            obj = player;
        }
    }
    else
    {
        obj = thing;
    }

    /*
     * If the trace flag is on this attr, set the object Trace
     */

    if (!Trace(obj) && (aflags & AF_TRACE))
    {
        trace_flag = 1;
        s_Trace(obj);
    }
    else
    {
        trace_flag = 0;
    }

    /*
     * Evaluate it using the rest of the passed function args
     */
    str = atext;
    extra_args = nfargs - ((Is_Func(UCALL_SANDBOX)) ? 5 : 3);
    eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str,
                          (extra_args > 0) ? ((Is_Func(UCALL_SANDBOX)) ? &(fargs[5]) : &(fargs[3])) : NULL,
                          extra_args);
    XFREE(atext);

    /*
     * Reset the trace flag if we need to
     */

    if (trace_flag)
    {
        c_Trace(obj);
    }

    /*
     * Restore / clean registers
     */

    if (Is_Func(UCALL_SANDBOX))
    {
        callp = Eat_Spaces(fargs[3]);
    }
    else
    {
        callp = Eat_Spaces(fargs[1]);
    }

    if (!*callp)
    {
        /*
         * Restore nothing, so we keep our data as-is.
         */
        if (preserve)
        {
            for (int z = 0; z < preserve->q_alloc; z++)
            {
                if (preserve->q_regs[z])
                    XFREE(preserve->q_regs[z]);
            }

            for (int z = 0; z < preserve->xr_alloc; z++)
            {
                if (preserve->x_names[z])
                    XFREE(preserve->x_names[z]);

                if (preserve->x_regs[z])
                    XFREE(preserve->x_regs[z]);
            }

            if (preserve->q_regs)
            {
                XFREE(preserve->q_regs);
            }

            if (preserve->q_lens)
            {
                XFREE(preserve->q_lens);
            }

            if (preserve->x_names)
            {
                XFREE(preserve->x_names);
            }

            if (preserve->x_regs)
            {
                XFREE(preserve->x_regs);
            }

            if (preserve->x_lens)
            {
                XFREE(preserve->x_lens);
            }

            XFREE(preserve);
        }
    }
    else if (!strncmp(callp, "@_!", 3) && ((callp[3] == '\0') || (callp[3] == ' ')))
    {
        if (callp[3] == '\0')
        {
            /*
             * Clear out all data
             */
            restore_global_regs("fun_ucall.restore", preserve);
        }
        else
        {
            /*
             * Go back to the original registers, but ADD BACK IN
             * the new values of the registers on the list.
             */
            tmp = preserve;
            preserve = mushstate.rdata; /* preserve is now the
                                         * new vals */
            mushstate.rdata = tmp;      /* this is now the original
                                         * vals */
            call_list = XMALLOC(LBUF_SIZE, "call_list");
            XSTRCPY(call_list, callp + 4);
            ncregs = list2arr(&cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM);

            for (i = 0; i < ncregs; i++)
            {
                set_register("fun_ucall", cregs[i], get_register(preserve, cregs[i]));
            }

            XFREE(call_list);

            if (preserve)
            {
                for (int z = 0; z < preserve->q_alloc; z++)
                {
                    if (preserve->q_regs[z])
                        XFREE(preserve->q_regs[z]);
                }

                for (int z = 0; z < preserve->xr_alloc; z++)
                {
                    if (preserve->x_names[z])
                        XFREE(preserve->x_names[z]);

                    if (preserve->x_regs[z])
                        XFREE(preserve->x_regs[z]);
                }

                if (preserve->q_regs)
                {
                    XFREE(preserve->q_regs);
                }

                if (preserve->q_lens)
                {
                    XFREE(preserve->q_lens);
                }

                if (preserve->x_names)
                {
                    XFREE(preserve->x_names);
                }

                if (preserve->x_regs)
                {
                    XFREE(preserve->x_regs);
                }

                if (preserve->x_lens)
                {
                    XFREE(preserve->x_lens);
                }

                XFREE(preserve);
            }
        }
    }
    else if (!strncmp(callp, "@_", 2) && ((callp[2] == '\0') || (callp[2] == ' ')))
    {
        if (callp[2] == '\0')
        {
            /*
             * Restore all registers we had before
             */
            call_list = NULL;
        }
        else
        {
            /*
             * Restore all registers EXCEPT the ones listed. We
             * assume that this list is going to be pretty short,
             * so we can do a crude, unsorted search.
             */
            call_list = XMALLOC(LBUF_SIZE, "call_list");
            XSTRCPY(call_list, callp + 3);
            ncregs = list2arr(&cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM);
        }

        if (preserve)
        {
            char cbuf[2];
            for (i = 0; i < preserve->q_alloc; i++)
            {
                if (preserve->q_regs[i] && *(preserve->q_regs[i]))
                {
                    cbuf[0] = qidx_str(i);
                    cbuf[1] = '\0';

                    if (!call_list || !is_in_array(cbuf, cregs, ncregs))
                        set_register("fun_ucall", cbuf, preserve->q_regs[i]);
                }
            }

            for (i = 0; i < preserve->xr_alloc; i++)
            {
                if (preserve->x_names[i] && *(preserve->x_names[i]) && preserve->x_regs[i] && *(preserve->x_regs[i]))
                {
                    if (!call_list || !is_in_array(preserve->x_names[i], cregs, ncregs))
                    {
                        set_register("fun_ucall", preserve->x_names[i], preserve->x_regs[i]);
                    }
                }
            }
        }


        if (call_list != NULL)
        {
            XFREE(call_list);
        }

        if (preserve)
        {
            for (int z = 0; z < preserve->q_alloc; z++)
            {
                if (preserve->q_regs[z])
                    XFREE(preserve->q_regs[z]);
            }

            for (int z = 0; z < preserve->xr_alloc; z++)
            {
                if (preserve->x_names[z])
                    XFREE(preserve->x_names[z]);

                if (preserve->x_regs[z])
                    XFREE(preserve->x_regs[z]);
            }

            if (preserve->q_regs)
            {
                XFREE(preserve->q_regs);
            }

            if (preserve->q_lens)
            {
                XFREE(preserve->q_lens);
            }

            if (preserve->x_names)
            {
                XFREE(preserve->x_names);
            }

            if (preserve->x_regs)
            {
                XFREE(preserve->x_regs);
            }

            if (preserve->x_lens)
            {
                XFREE(preserve->x_lens);
            }

            XFREE(preserve);
        }
    }
    else
    {
        /*
         * Restore ONLY these named registers
         */
        call_list = XMALLOC(LBUF_SIZE, "call_list");
        XSTRCPY(call_list, callp);
        ncregs = list2arr(&cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM);

        for (i = 0; i < ncregs; i++)
        {
            set_register("fun_ucall", cregs[i], get_register(preserve, cregs[i]));
        }

        XFREE(call_list);

        if (preserve)
        {
            for (int z = 0; z < preserve->q_alloc; z++)
            {
                if (preserve->q_regs[z])
                    XFREE(preserve->q_regs[z]);
            }

            for (int z = 0; z < preserve->xr_alloc; z++)
            {
                if (preserve->x_names[z])
                    XFREE(preserve->x_names[z]);

                if (preserve->x_regs[z])
                    XFREE(preserve->x_regs[z]);
            }

            if (preserve->q_regs)
            {
                XFREE(preserve->q_regs);
            }

            if (preserve->q_lens)
            {
                XFREE(preserve->q_lens);
            }

            if (preserve->x_names)
            {
                XFREE(preserve->x_names);
            }

            if (preserve->x_regs)
            {
                XFREE(preserve->x_regs);
            }

            if (preserve->x_lens)
            {
                XFREE(preserve->x_lens);
            }

            XFREE(preserve);
        }
    }

    if (Is_Func(UCALL_SANDBOX))
    {
        mushstate.f_limitmask = save_state;
    }
}

