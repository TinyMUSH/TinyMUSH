/**
 * @file funvars.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Structure, variable, stack, and regexp functions
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

#include <ctype.h>
#include <string.h>
#include <pcre.h>

/*
 * ---------------------------------------------------------------------------
 * setq, setr, r: set and read global registers.
 */

/**
 * @brief Convert ascii characters to global register (%q?) id
 *
 * @param ch    ascii character to convert
 * @return char global register id
 */
char qidx_chartab(int ch)
{
    if (ch > (86 + mushconf.max_global_regs))
    { // > z
        return -1;
    }
    else if (ch >= 97)
    { // >= a
        return ch - 87;
    }
    else if (ch > (54 + mushconf.max_global_regs))
    { // > Z
        return -1;
    }
    else if (ch >= 65)
    { // >= A
        return ch - 55;
    }
    else if (ch > 57)
    { // > 9
        return -1;
    }
    else if (ch >= 48)
    { // >= 0
        return ch - 48;
    }
    return -1;
}

/**
 * @brief convert global register (%q?) id to ascii character
 *
 * @param id    global register id
 * @return char ascii code of the register
 */
char qidx_str(int id)
{
    if (id > 35)
    {
        return 0;
    }
    else if (id >= 10)
    { // > z
        return id + 87;
    }
    else if (id >= 0)
    {
        return id + 48;
    }
    return 0;
}

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

void fun_setq(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
            SAFE_LB_STR("#-1 INVALID GLOBAL REGISTER", buff, bufc);
        }
        else if (result == -2)
        {
            SAFE_LB_STR("#-1 REGISTER LIMIT EXCEEDED", buff, bufc);
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

void fun_setr(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int result;
    result = set_register("fun_setr", fargs[0], fargs[1]);

    if (result == -1)
    {
        SAFE_LB_STR("#-1 INVALID GLOBAL REGISTER", buff, bufc);
    }
    else if (result == -2)
    {
        SAFE_LB_STR("#-1 REGISTER LIMIT EXCEEDED", buff, bufc);
    }
    else if (result > 0)
    {
        SAFE_STRNCAT(buff, bufc, fargs[1], result, LBUF_SIZE);
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
            SAFE_LB_STR("#-1 INVALID GLOBAL REGISTER", buff, bufc);
        }
        else
        {
            if (mushstate.rdata && (mushstate.rdata->q_alloc > regnum) && mushstate.rdata->q_regs[regnum])
            {
                SAFE_STRNCAT(buff, bufc, mushstate.rdata->q_regs[regnum], mushstate.rdata->q_lens[regnum], LBUF_SIZE);
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
                SAFE_STRNCAT(buff, bufc, mushstate.rdata->x_regs[regnum], mushstate.rdata->x_lens[regnum], LBUF_SIZE);
                return;
            }
        }
    }
}

void fun_r(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    read_register(fargs[0], buff, bufc);
}

/*
 * --------------------------------------------------------------------------
 * lregs: List all the non-empty q-registers.
 */

void fun_lregs(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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

            SAFE_LB_CHR(qidx_str(i), buff, bufc);
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

            SAFE_LB_STR(g->x_names[i], buff, bufc);
        }
    }
}

/*
 * --------------------------------------------------------------------------
 * wildmatch: Set the results of a wildcard match into the global registers.
 * wildmatch(<string>,<wildcard pattern>,<register list>)
 */

void fun_wildmatch(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int i, nqregs;
    char *t_args[NUM_ENV_VARS], **qregs; /* %0-%9 is limiting */

    if (!wild(fargs[1], fargs[0], t_args, NUM_ENV_VARS))
    {
        SAFE_LB_CHR('0', buff, bufc);
        return;
    }

    SAFE_LB_CHR('1', buff, bufc);
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
        SAFE_LB_STR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
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
        SAFE_LB_STR(strp, buff, bufc);

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

void fun_nofx(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
    int save_state, lmask;
    char *str;
    lmask = calc_limitmask(fargs[0]);

    if (lmask == -1)
    {
        SAFE_STRNCAT(buff, bufc, "#-1 INVALID LIMIT", 17, LBUF_SIZE);
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

void handle_ucall(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref aowner = NOTHING, thing = NOTHING, obj = NOTHING;
    ATTR *ap = NULL;
    GDATA *preserve = NULL, *tmp = NULL;
    char *atext = NULL, *str = NULL, *callp = NULL, *call_list = NULL, *cbuf = NULL, **cregs = NULL;
    int aflags = 0, alen = 0, anum = 0, trace_flag = 0, i = 0, ncregs = 0, lmask = 0, save_state = 0;

    /*
     * Three arguments to ucall(), five to sandbox()
     */

    if (nfargs < 3)
    {
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
        return;
    }

    if (Is_Func(UCALL_SANDBOX) && (nfargs < 5))
    {
        SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
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
            SAFE_STRNCAT(buff, bufc, "#-1 INVALID LIMIT", 17, LBUF_SIZE);
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
    eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str, (Is_Func(UCALL_SANDBOX)) ? &(fargs[5]) : &(fargs[3]), nfargs - ((Is_Func(UCALL_SANDBOX)) ? 5 : 3));
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
    SAFE_LTOS(tbuf, &tp, obj, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);
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
                    SAFE_LB_CHR(' ', buff, bufc);
                }

                SAFE_LB_STR(strchr(hptr->target.s, '.') + 1, buff, bufc);
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
    SAFE_LTOS(tbuf, &tp, obj, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = name; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(name, tbuf, &tp);
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
    SAFE_LTOS(pre, &tp, obj, LBUF_SIZE);
    SAFE_SB_CHR('.', pre, &tp);
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
        SAFE_SB_STR(pre, tbuf, &tp);
        SAFE_SB_STR(xvar_names[i], tbuf, &tp);
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
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);
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

void fun_x(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    VARENT *xvar;
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *tp, *p;
    /*
     * Variable string is '<dbref number minus #>.<variable name>'
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], tbuf, &tp);
    *tp = '\0';

    if ((xvar = (VARENT *)hashfind(tbuf, &mushstate.vars_htab)))
    {
        SAFE_LB_STR(xvar->text, buff, bufc);
    }
    XFREE(tbuf);
}

void fun_setx(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    set_xvar(player, fargs[0], fargs[1]);
}

void fun_store(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    set_xvar(player, fargs[0], fargs[1]);
    SAFE_LB_STR(fargs[1], buff, bufc);
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
        SAFE_LB_STR("#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc);
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
    SAFE_LTOS(pre, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', pre, &tp);
    *tp = '\0';

    for (i = 0; i < n_xvars; i++)
    {
        tp = tbuf;
        SAFE_SB_STR(pre, tbuf, &tp);
        SAFE_SB_STR(xvar_names[i], tbuf, &tp);
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
            SAFE_LB_STR("#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc);
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

void fun_lvars(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    print_htab_matches(player, &mushstate.vars_htab, buff, bufc);
}

void fun_clearvars(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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

void fun_structure(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep; /* delim for default values */
    Delim osep; /* output delim for structure values */
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *cbuf = XMALLOC(SBUF_SIZE, "cbuf");
    char *p, *tp, *cp;
    char *comp_names, *type_names, *default_vals;
    char **comp_array, **type_array, **def_array;
    int n_comps, n_types, n_defs;
    int i;
    STRUCTDEF *this_struct;
    COMPONENT *this_comp;
    int check_type = 0;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 4, 6, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6 - 1, &isep, DELIM_STRING))
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
     * Prevent null delimiters and line delimiters.
     */

    if ((osep.len > 1) || (osep.str[0] == '\0') || (osep.str[0] == '\r'))
    {
        notify_quiet(player, "You cannot use that output delimiter.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Enforce limits.
     */

    if (StructCount(player) > mushconf.struct_lim)
    {
        notify_quiet(player, "Too many structures.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * If our structure name is too long, reject it.
     */

    if (strlen(fargs[0]) > (SBUF_SIZE / 2) - 9)
    {
        notify_quiet(player, "Structure name is too long.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * No periods in structure names
     */

    if (strchr(fargs[0], '.'))
    {
        notify_quiet(player, "Structure names cannot contain periods.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * The hashtable is indexed by <dbref number>.<structure name>
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], tbuf, &tp);
    *tp = '\0';

    /*
     * If we have this structure already, reject.
     */

    if (hashfind(tbuf, &mushstate.structs_htab))
    {
        notify_quiet(player, "Structure is already defined.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Split things up. Make sure lists are the same size. If everything
     * eventually goes well, comp_names and default_vals will REMAIN
     * allocated.
     */
    comp_names = XSTRDUP(fargs[1], "comp_names");
    n_comps = list2arr(&comp_array, LBUF_SIZE / 2, comp_names, &SPACE_DELIM);

    if (n_comps < 1)
    {
        notify_quiet(player, "There must be at least one component.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(comp_names);
        XFREE(comp_array);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Make sure that we have a sane name for the components. They must
     * be smaller than half an SBUF.
     */

    for (i = 0; i < n_comps; i++)
    {
        if (strlen(comp_array[i]) > (SBUF_SIZE / 2) - 9)
        {
            notify_quiet(player, "Component name is too long.");
            SAFE_LB_CHR('0', buff, bufc);
            XFREE(comp_names);
            XFREE(comp_array);
            XFREE(cbuf);
            XFREE(tbuf);
            return;
        }
    }

    type_names = XMALLOC(LBUF_SIZE, "type_names");
    XSTRCPY(type_names, fargs[2]);
    n_types = list2arr(&type_array, LBUF_SIZE / 2, type_names, &SPACE_DELIM);

    /*
     * Make sure all types are valid. We look only at the first char, so
     * typos will not be caught.
     */

    for (i = 0; i < n_types; i++)
    {
        switch (*(type_array[i]))
        {
        case 'a':
        case 'A':
        case 'c':
        case 'C':
        case 'd':
        case 'D':
        case 'i':
        case 'I':
        case 'f':
        case 'F':
        case 's':
        case 'S':
            /*
             * Valid types
             */
            break;

        default:
            notify_quiet(player, "Invalid data type specified.");
            SAFE_LB_CHR('0', buff, bufc);
            XFREE(comp_names);
            XFREE(comp_array);
            XFREE(type_names);
            XFREE(type_array);
            XFREE(cbuf);
            XFREE(tbuf);
            return;
        }
    }

    if (fargs[3] && *fargs[3])
    {
        default_vals = XSTRDUP(fargs[3], "default_vals");
        n_defs = list2arr(&def_array, LBUF_SIZE / 2, default_vals, &isep);
    }
    else
    {
        default_vals = NULL;
        n_defs = 0;
    }

    if ((n_comps != n_types) || (n_defs && (n_comps != n_defs)))
    {
        notify_quiet(player, "List sizes must be identical.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(comp_names);
        XFREE(comp_array);
        XFREE(type_names);
        XFREE(type_array);

        if (default_vals)
        {
            XFREE(default_vals);
            XFREE(def_array);
        }

        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Allocate the structure and stuff it in the hashtable. Note that we
     * retain one of the string arrays allocated by list2arr!
     */
    this_struct = (STRUCTDEF *)XMALLOC(sizeof(STRUCTDEF), "this_struct");
    this_struct->s_name = XSTRDUP(fargs[0], "this_struct->s_name");
    this_struct->c_names = comp_array;
    this_struct->c_array = (COMPONENT **)XCALLOC(n_comps, sizeof(COMPONENT *), "this_struct->c_array");
    this_struct->c_count = n_comps;
    this_struct->delim = osep.str[0];
    this_struct->n_instances = 0;
    this_struct->names_base = comp_names;
    this_struct->defs_base = default_vals;
    hashadd(tbuf, (int *)this_struct, &mushstate.structs_htab, 0);
    mushstate.max_structs = mushstate.structs_htab.entries > mushstate.max_structs ? mushstate.structs_htab.entries : mushstate.max_structs;
    /*
     * Now that we're done with the base name, we can stick the joining
     * period on the end.
     */
    SAFE_SB_CHR('.', tbuf, &tp);
    *tp = '\0';

    /*
     * Allocate each individual component.
     */

    for (i = 0; i < n_comps; i++)
    {
        cp = cbuf;
        SAFE_SB_STR(tbuf, cbuf, &cp);

        for (p = comp_array[i]; *p; p++)
        {
            *p = tolower(*p);
        }

        SAFE_SB_STR(comp_array[i], cbuf, &cp);
        *cp = '\0';
        this_comp = (COMPONENT *)XMALLOC(sizeof(COMPONENT), "this_comp");
        this_comp->def_val = (default_vals ? def_array[i] : NULL);

        switch (*(type_array[i]))
        {
        case 'a':
        case 'A':
            this_comp->typer_func = NULL;
            break;

        case 'c':
        case 'C':
            this_comp->typer_func = istype_char;
            check_type = 1;
            break;

        case 'd':
        case 'D':
            this_comp->typer_func = istype_dbref;
            check_type = 1;
            break;

        case 'i':
        case 'I':
            this_comp->typer_func = istype_int;
            check_type = 1;
            break;

        case 'f':
        case 'F':
            this_comp->typer_func = istype_float;
            check_type = 1;
            break;

        case 's':
        case 'S':
            this_comp->typer_func = istype_string;
            check_type = 1;
            break;

        default:
            /*
             * Should never happen
             */
            this_comp->typer_func = NULL;
        }

        this_struct->need_typecheck = check_type;
        this_struct->c_array[i] = this_comp;
        hashadd(cbuf, (int *)this_comp, &mushstate.cdefs_htab, 0);
        mushstate.max_cdefs = mushstate.cdefs_htab.entries > mushstate.max_cdefs ? mushstate.cdefs_htab.entries : mushstate.max_cdefs;
    }

    XFREE(type_names);
    XFREE(type_array);

    if (default_vals)
    {
        XFREE(def_array);
    }

    s_StructCount(player, StructCount(player) + 1);
    SAFE_LB_CHR('1', buff, bufc);
    XFREE(cbuf);
    XFREE(tbuf);
}

void fun_construct(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep;
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *ibuf = XMALLOC(SBUF_SIZE, "ibuf");
    char *cbuf = XMALLOC(SBUF_SIZE, "cbuf");
    char *p, *tp, *ip, *cp;
    STRUCTDEF *this_struct;
    char *comp_names, *init_vals;
    char **comp_array, **vals_array;
    int n_comps, n_vals;
    int i;
    COMPONENT *c_ptr;
    INSTANCE *inst_ptr;
    STRUCTDATA *d_ptr;
    int retval;
    /*
     * This one is complicated: We need two, four, or five args.
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 5, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &isep, DELIM_STRING))
    {
        return;
    }

    if (nfargs == 3)
    {
        XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (CONSTRUCT) EXPECTS 2 OR 4 OR 5 ARGUMENTS BUT GOT %d", nfargs);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Enforce limits.
     */

    if (InstanceCount(player) > mushconf.instance_lim)
    {
        notify_quiet(player, "Too many instances.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * If our instance name is too long, reject it.
     */

    if (strlen(fargs[0]) > (SBUF_SIZE / 2) - 9)
    {
        notify_quiet(player, "Instance name is too long.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Make sure this instance doesn't exist.
     */
    ip = ibuf;
    SAFE_LTOS(ibuf, &ip, player, LBUF_SIZE);
    SAFE_SB_CHR('.', ibuf, &ip);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], ibuf, &ip);
    *ip = '\0';

    if (hashfind(ibuf, &mushstate.instance_htab))
    {
        notify_quiet(player, "That instance has already been defined.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Look up the structure.
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[1]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[1], tbuf, &tp);
    *tp = '\0';
    this_struct = (STRUCTDEF *)hashfind(tbuf, &mushstate.structs_htab);

    if (!this_struct)
    {
        notify_quiet(player, "No such structure.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Check to make sure that all the component names are valid, if we
     * have been given defaults. Also, make sure that the defaults are of
     * the appropriate type.
     */
    SAFE_SB_CHR('.', tbuf, &tp);
    *tp = '\0';

    if (fargs[2] && *fargs[2] && fargs[3] && *fargs[3])
    {
        comp_names = XMALLOC(LBUF_SIZE, "comp_names");
        XSTRCPY(comp_names, fargs[2]);
        n_comps = list2arr(&comp_array, LBUF_SIZE / 2, comp_names, &SPACE_DELIM);
        init_vals = XMALLOC(LBUF_SIZE, "init_vals");
        XSTRCPY(init_vals, fargs[3]);
        n_vals = list2arr(&vals_array, LBUF_SIZE / 2, init_vals, &isep);

        if (n_comps != n_vals)
        {
            notify_quiet(player, "List sizes must be identical.");
            SAFE_LB_CHR('0', buff, bufc);
            XFREE(comp_names);
            XFREE(init_vals);
            XFREE(comp_array);
            XFREE(vals_array);
            XFREE(cbuf);
            XFREE(ibuf);
            XFREE(tbuf);
            return;
        }

        for (i = 0; i < n_comps; i++)
        {
            cp = cbuf;
            SAFE_SB_STR(tbuf, cbuf, &cp);

            for (p = comp_array[i]; *p; p++)
            {
                *p = tolower(*p);
            }

            SAFE_SB_STR(comp_array[i], cbuf, &cp);
            c_ptr = (COMPONENT *)hashfind(cbuf, &mushstate.cdefs_htab);

            if (!c_ptr)
            {
                notify_quiet(player, "Invalid component name.");
                SAFE_LB_CHR('0', buff, bufc);
                XFREE(comp_names);
                XFREE(init_vals);
                XFREE(comp_array);
                XFREE(vals_array);
                XFREE(cbuf);
                XFREE(ibuf);
                XFREE(tbuf);
                return;
            }

            if (c_ptr->typer_func)
            {
                retval = (*(c_ptr->typer_func))(vals_array[i]);

                if (!retval)
                {
                    notify_quiet(player, "Default value is of invalid type.");
                    SAFE_LB_CHR('0', buff, bufc);
                    XFREE(comp_names);
                    XFREE(init_vals);
                    XFREE(comp_array);
                    XFREE(vals_array);
                    XFREE(cbuf);
                    XFREE(ibuf);
                    XFREE(tbuf);
                    return;
                }
            }
        }
    }
    else if ((!fargs[2] || !*fargs[2]) && (!fargs[3] || !*fargs[3]))
    {
        /*
         * Blank initializers. This is just fine.
         */
        comp_names = init_vals = NULL;
        comp_array = vals_array = NULL;
        n_comps = n_vals = 0;
    }
    else
    {
        notify_quiet(player, "List sizes must be identical.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Go go gadget constructor. Allocate the instance. We should have
     * already made sure that the instance doesn't exist.
     */
    inst_ptr = (INSTANCE *)XMALLOC(sizeof(INSTANCE), "inst_ptr");
    inst_ptr->datatype = this_struct;
    hashadd(ibuf, (int *)inst_ptr, &mushstate.instance_htab, 0);
    mushstate.max_instance = mushstate.instance_htab.entries > mushstate.max_instance ? mushstate.instance_htab.entries : mushstate.max_instance;

    /*
     * Populate with default values.
     */

    for (i = 0; i < this_struct->c_count; i++)
    {
        d_ptr = (STRUCTDATA *)XMALLOC(sizeof(STRUCTDATA), "d_ptr");

        if (this_struct->c_array[i]->def_val)
        {
            d_ptr->text = (char *)XSTRDUP(this_struct->c_array[i]->def_val, "d_ptr->text");
        }
        else
        {
            d_ptr->text = NULL;
        }

        tp = tbuf;
        SAFE_SB_STR(ibuf, tbuf, &tp);
        SAFE_SB_CHR('.', tbuf, &tp);
        SAFE_SB_STR(this_struct->c_names[i], tbuf, &tp);
        *tp = '\0';
        hashadd(tbuf, (int *)d_ptr, &mushstate.instdata_htab, 0);
        mushstate.max_instdata = mushstate.instdata_htab.entries > mushstate.max_instdata ? mushstate.instdata_htab.entries : mushstate.max_instdata;
    }

    /*
     * Overwrite with component values.
     */

    for (i = 0; i < n_comps; i++)
    {
        tp = tbuf;
        SAFE_SB_STR(ibuf, tbuf, &tp);
        SAFE_SB_CHR('.', tbuf, &tp);
        SAFE_SB_STR(comp_array[i], tbuf, &tp);
        *tp = '\0';
        d_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

        if (d_ptr)
        {
            if (d_ptr->text)
            {
                XFREE(d_ptr->text);
            }

            if (vals_array[i] && *(vals_array[i]))
                d_ptr->text = XSTRDUP(vals_array[i], "d_ptr->text");
            else
            {
                d_ptr->text = NULL;
            }
        }
    }

    if (comp_names)
    {
        XFREE(comp_names);
        XFREE(comp_array);
    }

    if (init_vals)
    {
        XFREE(init_vals);
        XFREE(vals_array);
    }

    this_struct->n_instances += 1;
    s_InstanceCount(player, InstanceCount(player) + 1);
    SAFE_LB_CHR('1', buff, bufc);
    XFREE(cbuf);
    XFREE(ibuf);
    XFREE(tbuf);
}

void load_structure(dbref player, char *buff, char **bufc, char *inst_name, char *str_name, char *raw_text, char sep, int use_def_delim)
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *ibuf = XMALLOC(SBUF_SIZE, "ibuf");
    char *p, *tp, *ip;
    STRUCTDEF *this_struct;
    char *val_list;
    char **val_array;
    int n_vals;
    INSTANCE *inst_ptr;
    STRUCTDATA *d_ptr;
    int i;
    Delim isep;

    /*
     * Enforce limits.
     */

    if (InstanceCount(player) > mushconf.instance_lim)
    {
        notify_quiet(player, "Too many instances.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * If our instance name is too long, reject it.
     */

    if (strlen(inst_name) > (SBUF_SIZE / 2) - 9)
    {
        notify_quiet(player, "Instance name is too long.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);

        return;
    }

    /*
     * Make sure this instance doesn't exist.
     */
    ip = ibuf;
    SAFE_LTOS(ibuf, &ip, player, LBUF_SIZE);
    SAFE_SB_CHR('.', ibuf, &ip);

    for (p = inst_name; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(inst_name, ibuf, &ip);
    *ip = '\0';

    if (hashfind(ibuf, &mushstate.instance_htab))
    {
        notify_quiet(player, "That instance has already been defined.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);

        return;
    }

    /*
     * Look up the structure.
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = str_name; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(str_name, tbuf, &tp);
    *tp = '\0';
    this_struct = (STRUCTDEF *)hashfind(tbuf, &mushstate.structs_htab);

    if (!this_struct)
    {
        notify_quiet(player, "No such structure.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);

        return;
    }

    /*
     * Chop up the raw stuff according to the delimiter.
     */
    isep.len = 1;

    if (use_def_delim)
    {
        isep.str[0] = this_struct->delim;
    }
    else
    {
        isep.str[0] = sep;
    }

    val_list = XMALLOC(LBUF_SIZE, "val_list");
    XSTRCPY(val_list, raw_text);
    n_vals = list2arr(&val_array, LBUF_SIZE / 2, val_list, &isep);

    if (n_vals != this_struct->c_count)
    {
        notify_quiet(player, "Incorrect number of components.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(val_list);
        XFREE(val_array);
        XFREE(ibuf);
        XFREE(tbuf);

        return;
    }

    /*
     * Check the types of the data we've been passed.
     */

    for (i = 0; i < n_vals; i++)
    {
        if (this_struct->c_array[i]->typer_func && !((*(this_struct->c_array[i]->typer_func))(val_array[i])))
        {
            notify_quiet(player, "Value is of invalid type.");
            SAFE_LB_CHR('0', buff, bufc);
            XFREE(val_list);
            XFREE(val_array);
            XFREE(ibuf);
            XFREE(tbuf);

            return;
        }
    }

    /*
     * Allocate the instance. We should have already made sure that the
     * instance doesn't exist.
     */
    inst_ptr = (INSTANCE *)XMALLOC(sizeof(INSTANCE), "inst_ptr");
    inst_ptr->datatype = this_struct;
    hashadd(ibuf, (int *)inst_ptr, &mushstate.instance_htab, 0);
    mushstate.max_instance = mushstate.instance_htab.entries > mushstate.max_instance ? mushstate.instance_htab.entries : mushstate.max_instance;

    /*
     * Stuff data into memory.
     */

    for (i = 0; i < this_struct->c_count; i++)
    {
        d_ptr = (STRUCTDATA *)XMALLOC(sizeof(STRUCTDATA), "d_ptr");

        if (val_array[i] && *(val_array[i]))
            d_ptr->text = XSTRDUP(val_array[i], "d_ptr->text");
        else
        {
            d_ptr->text = NULL;
        }

        tp = tbuf;
        SAFE_SB_STR(ibuf, tbuf, &tp);
        SAFE_SB_CHR('.', tbuf, &tp);
        SAFE_SB_STR(this_struct->c_names[i], tbuf, &tp);
        *tp = '\0';
        hashadd(tbuf, (int *)d_ptr, &mushstate.instdata_htab, 0);
        mushstate.max_instdata = mushstate.instdata_htab.entries > mushstate.max_instdata ? mushstate.instdata_htab.entries : mushstate.max_instdata;
    }

    XFREE(val_list);
    XFREE(val_array);
    this_struct->n_instances += 1;
    s_InstanceCount(player, InstanceCount(player) + 1);
    SAFE_LB_CHR('1', buff, bufc);
    XFREE(ibuf);
    XFREE(tbuf);
}

void fun_load(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, 0))
    {
        return;
    }

    load_structure(player, buff, bufc, fargs[0], fargs[1], fargs[2], isep.str[0], (nfargs != 4) ? 1 : 0);
}

void fun_read(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it, aowner;
    int atr, aflags, alen;
    char *atext;

    if (!parse_attrib(player, fargs[0], &it, &atr, 1) || (atr == NOTHING))
    {
        SAFE_LB_CHR('0', buff, bufc);
        return;
    }

    atext = atr_pget(it, atr, &aowner, &aflags, &alen);
    load_structure(player, buff, bufc, fargs[1], fargs[2], atext, GENERIC_STRUCT_DELIM, 0);
    XFREE(atext);
}

void fun_delimit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, aowner;
    int atr, aflags, alen, nitems, i, over = 0;
    char *atext, **ptrs;
    Delim isep;
    /*
     * This function is unusual in that the second argument is a
     * delimiter string of arbitrary length, rather than a character. The
     * input delimiter is the final, optional argument; if it's not
     * specified it defaults to the "null" structure delimiter. (This
     * function's primary purpose is to extract out data that's been
     * stored as a "null"-delimited structure, but it's also useful for
     * transforming any delim-separated list to a list whose elements are
     * separated by arbitrary strings.)
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, 0))
    {
        return;
    }

    if (nfargs != 3)
    {
        isep.str[0] = GENERIC_STRUCT_DELIM;
    }

    if (!parse_attrib(player, fargs[0], &it, &atr, 1) || (atr == NOTHING))
    {
        SAFE_NOPERM(buff, bufc);
        return;
    }

    atext = atr_pget(it, atr, &aowner, &aflags, &alen);
    nitems = list2arr(&ptrs, LBUF_SIZE / 2, atext, &isep);

    if (nitems)
    {
        over = SAFE_LB_STR(ptrs[0], buff, bufc);
    }

    for (i = 1; !over && (i < nitems); i++)
    {
        over = SAFE_LB_STR(fargs[1], buff, bufc);

        if (!over)
        {
            over = SAFE_LB_STR(ptrs[i], buff, bufc);
        }
    }

    XFREE(atext);
    XFREE(ptrs);
}

void fun_z(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *p, *tp;
    STRUCTDATA *s_ptr;
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], tbuf, &tp);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[1]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[1], tbuf, &tp);
    *tp = '\0';
    s_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

    if (!s_ptr || !s_ptr->text)
    {
        XFREE(tbuf);
        return;
    }

    SAFE_LB_STR(s_ptr->text, buff, bufc);
    XFREE(tbuf);
}

void fun_modify(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *cbuf = XMALLOC(SBUF_SIZE, "cbuf");
    char *endp, *p, *tp, *cp;
    INSTANCE *inst_ptr;
    COMPONENT *c_ptr;
    STRUCTDATA *s_ptr;
    char **words, **vals;
    int retval, nwords, nvals, i, n_mod;
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    /*
     * Find the instance first, since this is how we get our typechecker.
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], tbuf, &tp);
    *tp = '\0';
    endp = tp; /* save where we are */
    inst_ptr = (INSTANCE *)hashfind(tbuf, &mushstate.instance_htab);

    if (!inst_ptr)
    {
        notify_quiet(player, "No such instance.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(tbuf);
        XFREE(cbuf);
        return;
    }

    /*
     * Process for each component in the list.
     */
    nwords = list2arr(&words, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);
    nvals = list2arr(&vals, LBUF_SIZE / 2, fargs[2], &isep);
    n_mod = 0;

    for (i = 0; i < nwords; i++)
    {
        /*
         * Find the component and check the type.
         */
        if (inst_ptr->datatype->need_typecheck)
        {
            cp = cbuf;
            SAFE_LTOS(cbuf, &cp, player, LBUF_SIZE);
            SAFE_SB_CHR('.', cbuf, &cp);
            SAFE_SB_STR(inst_ptr->datatype->s_name, cbuf, &cp);
            SAFE_SB_CHR('.', cbuf, &cp);

            for (p = words[i]; *p; p++)
            {
                *p = tolower(*p);
            }

            SAFE_SB_STR(words[i], cbuf, &cp);
            *cp = '\0';
            c_ptr = (COMPONENT *)hashfind(cbuf, &mushstate.cdefs_htab);

            if (!c_ptr)
            {
                notify_quiet(player, "No such component.");
                continue;
            }

            if (c_ptr->typer_func)
            {
                retval = (*(c_ptr->typer_func))(fargs[2]);

                if (!retval)
                {
                    notify_quiet(player, "Value is of invalid type.");
                    continue;
                }
            }
        }

        /*
         * Now go set it.
         */
        tp = endp;
        SAFE_SB_CHR('.', tbuf, &tp);
        SAFE_SB_STR(words[i], tbuf, &tp);
        *tp = '\0';
        s_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

        if (!s_ptr)
        {
            notify_quiet(player, "No such data.");
            continue;
        }

        if (s_ptr->text)
        {
            XFREE(s_ptr->text);
        }

        if ((i < nvals) && vals[i] && *vals[i])
        {
            s_ptr->text = XSTRDUP(vals[i], "s_ptr->text");
        }
        else
        {
            s_ptr->text = NULL;
        }

        n_mod++;
    }

    XFREE(words);
    XFREE(vals);
    SAFE_LTOS(buff, bufc, n_mod, LBUF_SIZE);
    XFREE(tbuf);
    XFREE(cbuf);
}

void unload_structure(dbref player, char *buff, char **bufc, char *inst_name, char sep, int use_def_delim)
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *ibuf = XMALLOC(SBUF_SIZE, "ibuf");
    INSTANCE *inst_ptr;
    char *p, *tp, *ip;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    int i;
    /*
     * Get the instance.
     */
    ip = ibuf;
    SAFE_LTOS(ibuf, &ip, player, LBUF_SIZE);
    SAFE_SB_CHR('.', ibuf, &ip);

    for (p = inst_name; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(inst_name, ibuf, &ip);
    *ip = '\0';
    inst_ptr = (INSTANCE *)hashfind(ibuf, &mushstate.instance_htab);

    if (!inst_ptr)
    {
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * From the instance, we can get a pointer to the structure. We then
     * have the information we need to figure out what components are
     * associated with this, and print them appropriately.
     */
    SAFE_SB_CHR('.', ibuf, &ip);
    *ip = '\0';
    this_struct = inst_ptr->datatype;

    /*
     * Our delimiter is a special case.
     */
    if (use_def_delim)
    {
        sep = this_struct->delim;
    }

    for (i = 0; i < this_struct->c_count; i++)
    {
        if (i != 0)
        {
            SAFE_LB_CHR(sep, buff, bufc);
        }

        tp = tbuf;
        SAFE_SB_STR(ibuf, tbuf, &tp);
        SAFE_SB_STR(this_struct->c_names[i], tbuf, &tp);
        *tp = '\0';
        d_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

        if (d_ptr && d_ptr->text)
        {
            SAFE_LB_STR(d_ptr->text, buff, bufc);
        }
    }
    XFREE(ibuf);
    XFREE(tbuf);
}

void fun_unload(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, 0))
    {
        return;
    }

    unload_structure(player, buff, bufc, fargs[0], isep.str[0], (nfargs != 2) ? 1 : 0);
}

void fun_write(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it, aowner;
    int atrnum, aflags;
    char tbuf[LBUF_SIZE], *tp, *str;
    ATTR *attr;

    if (!parse_thing_slash(player, fargs[0], &str, &it))
    {
        SAFE_NOMATCH(buff, bufc);
        return;
    }

    tp = tbuf;
    *tp = '\0';
    unload_structure(player, tbuf, &tp, fargs[1], GENERIC_STRUCT_DELIM, 0);

    if (*tbuf)
    {
        atrnum = mkattr(str);

        if (atrnum <= 0)
        {
            SAFE_LB_STR("#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc);
            return;
        }

        attr = atr_num(atrnum);
        atr_pget_info(it, atrnum, &aowner, &aflags);

        if (!attr || !Set_attr(player, it, attr, aflags) || (attr->check != NULL))
        {
            SAFE_NOPERM(buff, bufc);
        }
        else
        {
            atr_add(it, atrnum, tbuf, Owner(player), aflags | AF_STRUCTURE);
        }
    }
}

void fun_destruct(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *ibuf = XMALLOC(SBUF_SIZE, "ibuf");
    INSTANCE *inst_ptr;
    char *p, *tp, *ip;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    int i;
    /*
     * Get the instance.
     */
    ip = ibuf;
    SAFE_LTOS(ibuf, &ip, player, LBUF_SIZE);
    SAFE_SB_CHR('.', ibuf, &ip);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], ibuf, &ip);
    *ip = '\0';
    inst_ptr = (INSTANCE *)hashfind(ibuf, &mushstate.instance_htab);

    if (!inst_ptr)
    {
        notify_quiet(player, "No such instance.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Now we can get a pointer to the structure and find the rest of the
     * components.
     */
    this_struct = inst_ptr->datatype;
    XFREE(inst_ptr);
    hashdelete(ibuf, &mushstate.instance_htab);
    SAFE_SB_CHR('.', ibuf, &ip);
    *ip = '\0';

    for (i = 0; i < this_struct->c_count; i++)
    {
        tp = tbuf;
        SAFE_SB_STR(ibuf, tbuf, &tp);
        SAFE_SB_STR(this_struct->c_names[i], tbuf, &tp);
        *tp = '\0';
        d_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

        if (d_ptr)
        {
            if (d_ptr->text)
            {
                XFREE(d_ptr->text);
            }

            XFREE(d_ptr);
            hashdelete(tbuf, &mushstate.instdata_htab);
        }
    }

    this_struct->n_instances -= 1;
    s_InstanceCount(player, InstanceCount(player) - 1);
    SAFE_LB_CHR('1', buff, bufc);
    XFREE(ibuf);
    XFREE(tbuf);
}

void fun_unstructure(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *cbuf = XMALLOC(SBUF_SIZE, "cbuf");
    char *p, *tp, *cp;
    STRUCTDEF *this_struct;
    int i;
    /*
     * Find the structure
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, player, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    SAFE_SB_STR(fargs[0], tbuf, &tp);
    *tp = '\0';
    this_struct = (STRUCTDEF *)hashfind(tbuf, &mushstate.structs_htab);

    if (!this_struct)
    {
        notify_quiet(player, "No such structure.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Can't delete what's in use.
     */

    if (this_struct->n_instances > 0)
    {
        notify_quiet(player, "This structure is in use.");
        SAFE_LB_CHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Wipe the structure from the hashtable.
     */
    hashdelete(tbuf, &mushstate.structs_htab);
    /*
     * Wipe out every component definition.
     */
    SAFE_SB_CHR('.', tbuf, &tp);
    *tp = '\0';

    for (i = 0; i < this_struct->c_count; i++)
    {
        cp = cbuf;
        SAFE_SB_STR(tbuf, cbuf, &cp);
        SAFE_SB_STR(this_struct->c_names[i], cbuf, &cp);
        *cp = '\0';

        if (this_struct->c_array[i])
        {
            XFREE(this_struct->c_array[i]);
        }

        hashdelete(cbuf, &mushstate.cdefs_htab);
    }

    /*
     * Free up our bit of memory.
     */
    XFREE(this_struct->s_name);

    if (this_struct->names_base)
    {
        XFREE(this_struct->names_base);
    }

    if (this_struct->defs_base)
    {
        XFREE(this_struct->defs_base);
    }

    XFREE(this_struct->c_names);
    XFREE(this_struct);
    s_StructCount(player, StructCount(player) - 1);
    SAFE_LB_CHR('1', buff, bufc);
    XFREE(cbuf);
    XFREE(tbuf);
}

void fun_lstructures(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    print_htab_matches(player, &mushstate.structs_htab, buff, bufc);
}

void fun_linstances(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    print_htab_matches(player, &mushstate.instance_htab, buff, bufc);
}

void structure_clr(dbref thing)
{
    /*
     * Wipe out all structure information associated with an object. Find
     * all the object's instances. Destroy them. Then, find all the
     * object's defined structures, and destroy those.
     */
    HASHTAB *htab;
    HASHENT *hptr;
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *ibuf = XMALLOC(SBUF_SIZE, "ibuf");
    char *cbuf = XMALLOC(SBUF_SIZE, "cbuf");
    char *tp, *ip, *cp, *tname;
    int i, j, len, count;
    INSTANCE **inst_array;
    char **name_array;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    STRUCTDEF **struct_array;
    /*
     * The instance table is indexed as <dbref number>.<instance name>
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, thing, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);
    /*
     * Because of the hashtable rechaining that's done, we cannot simply
     * walk the hashtable and delete entries as we go. Instead, we've got
     * to keep track of all of our pointers, and go back and do them one
     * by one.
     */
    inst_array = (INSTANCE **)XCALLOC(mushconf.instance_lim + 1, sizeof(INSTANCE *), "inst_array");
    name_array = (char **)XCALLOC(mushconf.instance_lim + 1, sizeof(char *), "name_array");
    htab = &mushstate.instance_htab;
    count = 0;

    for (i = 0; i < htab->hashsize; i++)
    {
        for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next)
        {
            if (!strncmp(tbuf, hptr->target.s, len))
            {
                name_array[count] = (char *)hptr->target.s;
                inst_array[count] = (INSTANCE *)hptr->data;
                count++;
            }
        }
    }

    /*
     * Now that we have the pointers to the instances, we can get the
     * structure definitions, and use that to hunt down and wipe the
     * components.
     */

    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            this_struct = inst_array[i]->datatype;
            XFREE(inst_array[i]);
            hashdelete(name_array[i], &mushstate.instance_htab);
            ip = ibuf;
            SAFE_SB_STR(name_array[i], ibuf, &ip);
            SAFE_SB_CHR('.', ibuf, &ip);
            *ip = '\0';

            for (j = 0; j < this_struct->c_count; j++)
            {
                cp = cbuf;
                SAFE_SB_STR(ibuf, cbuf, &cp);
                SAFE_SB_STR(this_struct->c_names[j], cbuf, &cp);
                *cp = '\0';
                d_ptr = (STRUCTDATA *)hashfind(cbuf, &mushstate.instdata_htab);

                if (d_ptr)
                {
                    if (d_ptr->text)
                        XFREE(d_ptr->text);

                    XFREE(d_ptr);
                    hashdelete(cbuf, &mushstate.instdata_htab);
                }
            }

            this_struct->n_instances -= 1;
        }
    }

    XFREE(inst_array);
    XFREE(name_array);
    /*
     * The structure table is indexed as <dbref number>.<struct name>
     */
    tp = tbuf;
    SAFE_LTOS(tbuf, &tp, thing, LBUF_SIZE);
    SAFE_SB_CHR('.', tbuf, &tp);
    *tp = '\0';
    len = strlen(tbuf);
    /*
     * Again, we have the hashtable rechaining problem.
     */
    struct_array = (STRUCTDEF **)XCALLOC(mushconf.struct_lim + 1, sizeof(STRUCTDEF *), "struct_array");
    name_array = (char **)XCALLOC(mushconf.struct_lim + 1, sizeof(char *), "name_array2");
    htab = &mushstate.structs_htab;
    count = 0;

    for (i = 0; i < htab->hashsize; i++)
    {
        for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next)
        {
            if (!strncmp(tbuf, hptr->target.s, len))
            {
                name_array[count] = (char *)hptr->target.s;
                struct_array[count] = (STRUCTDEF *)hptr->data;
                count++;
            }
        }
    }

    /*
     * We have the pointers to the structures. Flag a big error if
     * they're still in use, wipe them from the hashtable, then wipe out
     * every component definition. Free up the memory.
     */

    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            if (struct_array[i]->n_instances > 0)
            {
                tname = log_getname(thing);
                log_write(LOG_ALWAYS, "BUG", "STRUCT", "%s's structure %s has %d allocated instances uncleared.", tname, name_array[i], struct_array[i]->n_instances);
                XFREE(tname);
            }

            hashdelete(name_array[i], &mushstate.structs_htab);
            ip = ibuf;
            SAFE_SB_STR(name_array[i], ibuf, &ip);
            SAFE_SB_CHR('.', ibuf, &ip);
            *ip = '\0';

            for (j = 0; j < struct_array[i]->c_count; j++)
            {
                cp = cbuf;
                SAFE_SB_STR(ibuf, cbuf, &cp);
                SAFE_SB_STR(struct_array[i]->c_names[j], cbuf, &cp);
                *cp = '\0';

                if (struct_array[i]->c_array[j])
                {
                    XFREE(struct_array[i]->c_array[j]);
                }

                hashdelete(cbuf, &mushstate.cdefs_htab);
            }

            XFREE(struct_array[i]->s_name);

            if (struct_array[i]->names_base)
                XFREE(struct_array[i]->names_base);

            if (struct_array[i]->defs_base)
                XFREE(struct_array[i]->defs_base);

            XFREE(struct_array[i]->c_names);
            XFREE(struct_array[i]);
        }
    }

    XFREE(struct_array);
    XFREE(name_array);
    XFREE(cbuf);
    XFREE(ibuf);
    XFREE(tbuf);
}

/*
 * --------------------------------------------------------------------------
 * Auxiliary functions for stacks.
 */

/*
 * ---------------------------------------------------------------------------
 * Object stack functions.
 */

void stack_clr(dbref thing)
{
    OBJSTACK *sp, *tp, *xp;
    sp = ((OBJSTACK *)nhashfind(thing, &mushstate.objstack_htab));

    if (sp)
    {
        for (tp = sp; tp != NULL;)
        {
            XFREE(tp->data);
            xp = tp;
            tp = tp->next;
            XFREE(xp);
        }

        nhashdelete(thing, &mushstate.objstack_htab);
        s_StackCount(thing, 0);
    }
}

int stack_set(dbref thing, OBJSTACK *sp)
{
    OBJSTACK *xsp;
    char *tname;
    int stat;

    if (!sp)
    {
        nhashdelete(thing, &mushstate.objstack_htab);
        return 1;
    }

    xsp = ((OBJSTACK *)nhashfind(thing, &mushstate.objstack_htab));

    if (xsp)
    {
        stat = nhashrepl(thing, (int *)sp, &mushstate.objstack_htab);
    }
    else
    {
        stat = nhashadd(thing, (int *)sp, &mushstate.objstack_htab);
        mushstate.max_stacks = mushstate.objstack_htab.entries > mushstate.max_stacks ? mushstate.objstack_htab.entries : mushstate.max_stacks;
    }

    if (stat < 0)
    { /* failure for some reason */
        tname = log_getname(thing);
        log_write(LOG_BUGS, "STK", "SET", "%s, Failure", tname);
        XFREE(tname);
        stack_clr(thing);
        return 0;
    }

    return 1;
}

void fun_empty(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 1, buff, bufc))
    {
        return;
    }

    if (!fargs[0])
    {
        it = player;
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    stack_clr(it);
}

void fun_items(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it;

    if (!fargs[0])
    {
        it = player;
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    SAFE_LTOS(buff, bufc, StackCount(it), LBUF_SIZE);
}

void fun_push(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it;
    char *data;
    OBJSTACK *sp;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 2, buff, bufc))
    {
        return;
    }

    if (!fargs[1])
    {
        it = player;

        if (!fargs[0] || !*fargs[0])
        {
            data = (char *)"";
        }
        else
        {
            data = fargs[0];
        }
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
        data = fargs[1];
    }

    if (StackCount(it) + 1 > mushconf.stack_lim)
    {
        return;
    }

    sp = (OBJSTACK *)XMALLOC(sizeof(OBJSTACK), "sp");

    if (!sp)
    { /* out of memory, ouch */
        return;
    }

    sp->next = ((OBJSTACK *)nhashfind(it, &mushstate.objstack_htab));
    sp->data = (char *)XMALLOC(sizeof(char) * (strlen(data) + 1), "sp->data");

    if (!sp->data)
    {
        return;
    }

    XSTRCPY(sp->data, data);

    if (stack_set(it, sp))
    {
        s_StackCount(it, StackCount(it) + 1);
    }
}

void fun_dup(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it;
    OBJSTACK *hp; /* head of stack */
    OBJSTACK *tp; /* temporary stack pointer */
    OBJSTACK *sp; /* new stack element */
    int pos, count = 0;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 2, buff, bufc))
    {
        return;
    }

    if (!fargs[0])
    {
        it = player;
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    if (StackCount(it) + 1 > mushconf.stack_lim)
    {
        return;
    }

    if (!fargs[1] || !*fargs[1])
    {
        pos = 0;
    }
    else
    {
        pos = (int)strtol(fargs[1], (char **)NULL, 10);
    }

    hp = ((OBJSTACK *)nhashfind(it, &mushstate.objstack_htab));

    for (tp = hp; (count != pos) && (tp != NULL); count++, tp = tp->next)
        ;

    if (!tp)
    {
        notify_quiet(player, "No such item on stack.");
        return;
    }

    sp = (OBJSTACK *)XMALLOC(sizeof(OBJSTACK), "sp");

    if (!sp)
    {
        return;
    }

    sp->next = hp;
    sp->data = (char *)XMALLOC(sizeof(char) * (strlen(tp->data) + 1), "sp->data");

    if (!sp->data)
    {
        return;
    }

    XSTRCPY(sp->data, tp->data);

    if (stack_set(it, sp))
    {
        s_StackCount(it, StackCount(it) + 1);
    }
}

void fun_swap(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it;
    OBJSTACK *sp, *tp;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 1, buff, bufc))
    {
        return;
    }

    if (!fargs[0])
    {
        it = player;
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    sp = ((OBJSTACK *)nhashfind(it, &mushstate.objstack_htab));

    if (!sp || (sp->next == NULL))
    {
        notify_quiet(player, "Not enough items on stack.");
        return;
    }

    tp = sp->next;
    sp->next = tp->next;
    tp->next = sp;
    stack_set(it, tp);
}

void handle_pop(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref it;
    int pos, count = 0, peek_flag, toss_flag;
    OBJSTACK *sp;
    OBJSTACK *prev = NULL;
    peek_flag = Is_Func(POP_PEEK);
    toss_flag = Is_Func(POP_TOSS);

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 2, buff, bufc))
    {
        return;
    }

    if (!fargs[0])
    {
        it = player;
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    if (!fargs[1] || !*fargs[1])
    {
        pos = 0;
    }
    else
    {
        pos = (int)strtol(fargs[1], (char **)NULL, 10);
    }

    sp = ((OBJSTACK *)nhashfind(it, &mushstate.objstack_htab));

    if (!sp)
    {
        return;
    }

    while (count != pos)
    {
        if (!sp)
        {
            return;
        }

        prev = sp;
        sp = sp->next;
        count++;
    }

    if (!sp)
    {
        return;
    }

    if (!toss_flag)
    {
        SAFE_LB_STR(sp->data, buff, bufc);
    }

    if (!peek_flag)
    {
        if (count == 0)
        {
            stack_set(it, sp->next);
        }
        else
        {
            prev->next = sp->next;
        }

        XFREE(sp->data);
        XFREE(sp);
        s_StackCount(it, StackCount(it) - 1);
    }
}

void fun_popn(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;
    int pos, nitems, i, count = 0, over = 0;
    OBJSTACK *sp, *tp, *xp;
    OBJSTACK *prev = NULL;
    Delim osep;
    char *bb_p;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it))
    {
        return;
    }
    if (!Controls(player, it))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    pos = (int)strtol(fargs[1], (char **)NULL, 10);
    nitems = (int)strtol(fargs[2], (char **)NULL, 10);
    sp = ((OBJSTACK *)nhashfind(it, &mushstate.objstack_htab));

    if (!sp)
    {
        return;
    }

    while (count != pos)
    {
        if (!sp)
        {
            return;
        }

        prev = sp;
        sp = sp->next;
        count++;
    }

    if (!sp)
    {
        return;
    }

    /*
     * We've now hit the start item, the first item. Copy 'em off.
     */

    for (i = 0, tp = sp, bb_p = *bufc; (i < nitems) && (tp != NULL); i++)
    {
        if (!over)
        {
            /*
             * We have to pop off the items regardless of whether
             * or not there's an overflow, but we can save
             * ourselves some copying if so.
             */
            if (*bufc != bb_p)
            {
                print_separator(&osep, buff, bufc);
            }

            over = SAFE_LB_STR(tp->data, buff, bufc);
        }

        xp = tp;
        tp = tp->next;
        XFREE(xp->data);
        XFREE(xp);
        s_StackCount(it, StackCount(it) - 1);
    }

    /*
     * Relink the chain.
     */

    if (count == 0)
    {
        stack_set(it, tp);
    }
    else
    {
        prev->next = tp;
    }
}

void fun_lstack(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim osep;
    dbref it;
    OBJSTACK *sp;
    char *bb_p;
    int over = 0;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 2, buff, bufc))
    {
        return;
    };

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    if (!fargs[0])
    {
        it = player;
    }
    else
    {
        it = match_thing(player, fargs[0]);
        if (!Good_obj(it))
        {
            return;
        }
        if (!Controls(player, it))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    bb_p = *bufc;

    for (sp = ((OBJSTACK *)nhashfind(it, &mushstate.objstack_htab)); (sp != NULL) && !over; sp = sp->next)
    {
        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        over = SAFE_LB_STR(sp->data, buff, bufc);
    }
}

/*
 * --------------------------------------------------------------------------
 * regedit: Edit a string for sed/perl-like s//
 * regedit(<string>,<regexp>,<replacement>) Derived from the PennMUSH code.
 */

void perform_regedit(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    pcre *re;
    pcre_extra *study = NULL;
    const char *errptr;
    int case_option, all_option;
    int erroffset, subpatterns, len;
    int offsets[PCRE_MAX_OFFSETS];
    char *r, *start, *tbuf;
    char tmp;
    int match_offset = 0;
    case_option = Func_Mask(REG_CASELESS);
    all_option = Func_Mask(REG_MATCH_ALL);

    if ((re = pcre_compile(fargs[1], case_option, &errptr, &erroffset, mushstate.retabs)) == NULL)
    {
        /*
         * Matching error. Note that this returns a null string
         * rather than '#-1 REGEXP ERROR: <error>', as PennMUSH does,
         * in order to remain consistent with our other regexp
         * functions.
         */
        notify_quiet(player, errptr);
        return;
    }

    /*
     * Study the pattern for optimization, if we're going to try multiple
     * matches.
     */
    if (all_option)
    {
        study = pcre_study(re, 0, &errptr);

        if (errptr != NULL)
        {
            XFREE(re);
            notify_quiet(player, errptr);
            return;
        }
    }

    len = strlen(fargs[0]);
    start = fargs[0];
    subpatterns = pcre_exec(re, study, fargs[0], len, 0, 0, offsets, PCRE_MAX_OFFSETS);

    /*
     * If there's no match, just return the original.
     */

    if (subpatterns < 0)
    {
        XFREE(re);

        if (study)
        {
            XFREE(study);
        }

        SAFE_LB_STR(fargs[0], buff, bufc);
        return;
    }

    do
    {
        /*
         * If we had too many subpatterns for the offsets vector, set
         * the number to 1/3rd of the size of the offsets vector.
         */
        if (subpatterns == 0)
        {
            subpatterns = PCRE_MAX_OFFSETS / 3;
        }

        /*
         * Copy up to the start of the matched area.
         */
        tmp = fargs[0][offsets[0]];
        fargs[0][offsets[0]] = '\0';
        SAFE_LB_STR(start, buff, bufc);
        fargs[0][offsets[0]] = tmp;

        /*
         * Copy in the replacement, putting in captured
         * sub-expressions.
         */

        for (r = fargs[2]; *r; r++)
        {
            int offset, have_brace = 0;
            char *endsub;

            if (*r != '$')
            {
                SAFE_LB_CHR(*r, buff, bufc);
                continue;
            }

            r++;

            if (*r == '{')
            {
                have_brace = 1;
                r++;
            }

            offset = strtoul(r, &endsub, 10);

            if (r == endsub || (have_brace && *endsub != '}'))
            {
                /*
                 * Not a valid number.
                 */
                SAFE_LB_CHR('$', buff, bufc);

                if (have_brace)
                {
                    SAFE_LB_CHR('{', buff, bufc);
                }

                r--;
                continue;
            }

            r = endsub - 1;

            if (have_brace)
            {
                r++;
            }

            tbuf = XMALLOC(LBUF_SIZE, "tbuf");
            if (pcre_copy_substring(fargs[0], offsets, subpatterns, offset, tbuf, LBUF_SIZE) >= 0)
            {
                SAFE_LB_STR(tbuf, buff, bufc);
            }
            XFREE(tbuf);
        }

        start = fargs[0] + offsets[1];
        match_offset = offsets[1];
    } while (all_option && (((offsets[0] == offsets[1]) &&
                             /*
                              * PCRE docs note: Perl special-cases the empty-string match in split
                              * and /g. To emulate, first try the match again at the same position
                              * with PCRE_NOTEMPTY, then advance the starting offset if that
                              * fails.
                              */
                             (((subpatterns = pcre_exec(re, study, fargs[0], len, match_offset, PCRE_NOTEMPTY, offsets, PCRE_MAX_OFFSETS)) >= 0) || ((match_offset++ < len) && (subpatterns = pcre_exec(re, study, fargs[0], len, match_offset, 0, offsets, PCRE_MAX_OFFSETS)) >= 0))) ||
                            ((match_offset <= len) && (subpatterns = pcre_exec(re, study, fargs[0], len, match_offset, 0, offsets, PCRE_MAX_OFFSETS)) >= 0)));

    /*
     * Copy everything after the matched bit.
     */
    SAFE_LB_STR(start, buff, bufc);
    XFREE(re);

    if (study)
    {
        XFREE(study);
    }
}

/*
 * --------------------------------------------------------------------------
 * wildparse: Set the results of a wildcard match into named variables.
 * wildparse(<string>,<pattern>,<list of variable names>)
 */

void fun_wildparse(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int i, nqregs;
    char *t_args[NUM_ENV_VARS], **qregs;

    if (!wild(fargs[1], fargs[0], t_args, NUM_ENV_VARS))
    {
        return;
    }

    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);

    for (i = 0; i < nqregs; i++)
    {
        if (qregs[i] && *qregs[i])
        {
            set_xvar(player, qregs[i], t_args[i]);
        }
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
 * ---------------------------------------------------------------------------
 * perform_regparse: Slurp a string into up to ten named variables ($0 - $9).
 * REGPARSE, REGPARSEI. Unlike regmatch(), this returns no value.
 * regparse(string, pattern, named vars)
 */

void perform_regparse(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int i, nqregs;
    int case_option;
    char **qregs;
    char *matchbuf = XMALLOC(LBUF_SIZE, "matchbuf");
    pcre *re;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    int subpatterns;
    case_option = Func_Mask(REG_CASELESS);

    if ((re = pcre_compile(fargs[1], case_option, &errptr, &erroffset, mushstate.retabs)) == NULL)
    {
        /*
         * Matching error.
         */
        notify_quiet(player, errptr);
        XFREE(matchbuf);
        return;
    }

    subpatterns = pcre_exec(re, NULL, fargs[0], strlen(fargs[0]), 0, 0, offsets, PCRE_MAX_OFFSETS);

    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if (subpatterns == 0)
    {
        subpatterns = PCRE_MAX_OFFSETS / 3;
    }

    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);

    for (i = 0; i < nqregs; i++)
    {
        if (qregs[i] && *qregs[i])
        {
            if (pcre_copy_substring(fargs[0], offsets, subpatterns, i, matchbuf, LBUF_SIZE) < 0)
            {
                set_xvar(player, qregs[i], NULL);
            }
            else
            {
                set_xvar(player, qregs[i], matchbuf);
            }
        }
    }

    XFREE(re);
    XFREE(qregs);
    XFREE(matchbuf);
}

/*
 * ---------------------------------------------------------------------------
 * perform_regrab: Like grab() and graball(), but with a regexp pattern.
 * REGRAB, REGRABI. Derived from PennMUSH.
 */

void perform_regrab(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int case_option, all_option;
    char *r, *s, *bb_p;
    pcre *re;
    pcre_extra *study;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    case_option = Func_Mask(REG_CASELESS);
    all_option = Func_Mask(REG_MATCH_ALL);

    if (all_option)
    {
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
            }
        }
    }
    else
    {
        if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
        {
            return;
        }

        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
        {
            return;
        }
    }

    s = trim_space_sep(fargs[0], &isep);
    bb_p = *bufc;

    if ((re = pcre_compile(fargs[1], case_option, &errptr, &erroffset, mushstate.retabs)) == NULL)
    {
        /*
         * Matching error. Note difference from PennMUSH behavior:
         * Regular expression errors return 0, not #-1 with an error
         * message.
         */
        notify_quiet(player, errptr);
        return;
    }

    study = pcre_study(re, 0, &errptr);

    if (errptr != NULL)
    {
        notify_quiet(player, errptr);
        XFREE(re);
        return;
    }

    do
    {
        r = split_token(&s, &isep);

        if (pcre_exec(re, study, r, strlen(r), 0, 0, offsets, PCRE_MAX_OFFSETS) >= 0)
        {
            if (*bufc != bb_p)
            {
                /*
                 * if true, all_option also
                 * * true
                 */
                print_separator(&osep, buff, bufc);
            }

            SAFE_LB_STR(r, buff, bufc);

            if (!all_option)
            {
                break;
            }
        }
    } while (s);

    XFREE(re);

    if (study)
    {
        XFREE(study);
    }
}

/*
 * ---------------------------------------------------------------------------
 * perform_regmatch: Return 0 or 1 depending on whether or not a regular
 * expression matches a string. If a third argument is specified, dump the
 * results of a regexp pattern match into a set of arbitrary r()-registers.
 * REGMATCH, REGMATCHI
 *
 * regmatch(string, pattern, list of registers) If the number of matches exceeds
 * the registers, those bits are tossed out. If -1 is specified as a register
 * number, the matching bit is tossed. Therefore, if the list is "-1 0 3 5",
 * the regexp $0 is tossed, and the regexp $1, $2, and $3 become r(0), r(3),
 * and r(5), respectively.
 *
 * PCRE modifications adapted from PennMUSH.
 *
 */

void perform_regmatch(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    int case_option;
    int i, nqregs;
    char **qregs;
    pcre *re;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    int subpatterns;
    char tbuf[LBUF_SIZE];
    case_option = Func_Mask(REG_CASELESS);

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
    {
        return;
    }

    if ((re = pcre_compile(fargs[1], case_option, &errptr, &erroffset, mushstate.retabs)) == NULL)
    {
        /*
         * Matching error. Note difference from PennMUSH behavior:
         * Regular expression errors return 0, not #-1 with an error
         * message.
         */
        notify_quiet(player, errptr);
        SAFE_LB_CHR('0', buff, bufc);
        return;
    }

    subpatterns = pcre_exec(re, NULL, fargs[0], strlen(fargs[0]), 0, 0, offsets, PCRE_MAX_OFFSETS);
    SAFE_BOOL(buff, bufc, (subpatterns >= 0));

    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if (subpatterns == 0)
    {
        subpatterns = PCRE_MAX_OFFSETS / 3;
    }

    /*
     * If we don't have a third argument, we're done.
     */
    if (nfargs != 3)
    {
        XFREE(re);
        return;
    }

    /*
     * We need to parse the list of registers. Anything that we don't get
     * is assumed to be -1. If we didn't match, or the match went wonky,
     * then set the register to empty. Otherwise, fill the register with
     * the subexpression.
     */
    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);

    for (i = 0; i < nqregs; i++)
    {
        if (pcre_copy_substring(fargs[0], offsets, subpatterns, i, tbuf, LBUF_SIZE) < 0)
        {
            set_register("perform_regmatch", qregs[i], NULL);
        }
        else
        {
            set_register("perform_regmatch", qregs[i], tbuf);
        }
    }

    XFREE(re);
    XFREE(qregs);
}

/*
 * ---------------------------------------------------------------------------
 * fun_until: Much like while(), but operates on multiple lists ala mix().
 * until(eval_fn,cond_fn,list1,list2,compare_str,delim,output delim) The
 * delimiter terminators are MANDATORY. The termination condition is a REGEXP
 * match (thus allowing this to be also used as 'eval until a termination
 * condition is NOT met').
 */

void fun_until(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    dbref aowner1 = NOTHING, thing1 = NOTHING, aowner2 = NOTHING, thing2 = NOTHING;
    int aflags1 = 0, aflags2 = 0, anum1 = 0, anum2 = 0, alen1 = 0, alen2 = 0;
    ATTR *ap = NULL, *ap2 = NULL;
    char *atext1 = NULL, *atext2 = NULL, *atextbuf = NULL, *condbuf = NULL;
    char *cp[NUM_ENV_VARS], *os[NUM_ENV_VARS];
    int count[LBUF_SIZE / 2];
    int i = 0, is_exact_same = 0, is_same = 0, nwords = 0, lastn = 0, wc = 0;
    char *str = NULL, *dp = NULL, *savep = NULL, *bb_p = NULL;
    pcre *re = NULL;
    const char *errptr = NULL;
    int erroffset = 0;
    int offsets[PCRE_MAX_OFFSETS];
    int subpatterns = 0;
    /*
     * We need at least 6 arguments. The last 2 args must be delimiters.
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 6, 12, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, nfargs - 1, &isep, DELIM_STRING))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, nfargs, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    lastn = nfargs - 4;

    /*
     * Make sure we have a valid regular expression.
     */

    if ((re = pcre_compile(fargs[lastn + 1], 0, &errptr, &erroffset, mushstate.retabs)) == NULL)
    {
        /*
         * Return nothing on a bad match.
         */
        notify_quiet(player, errptr);
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

    if ((thing1 == thing2) && (ap->number == ap2->number))
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

    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");

    if (!is_same)
    {
        condbuf = XMALLOC(LBUF_SIZE, "condbuf");
    }

    bb_p = *bufc;

    /*
     * Process the list one element at a time. We need to find out what
     * the longest list is; assume null-padding for shorter lists.
     */

    for (i = 0; i < NUM_ENV_VARS; i++)
    {
        cp[i] = NULL;
    }

    cp[2] = trim_space_sep(fargs[2], &isep);
    nwords = count[2] = countwords(cp[2], &isep);

    for (i = 3; i <= lastn; i++)
    {
        cp[i] = trim_space_sep(fargs[i], &isep);
        count[i] = countwords(cp[i], &isep);

        if (count[i] > nwords)
        {
            nwords = count[i];
        }
    }

    for (wc = 0; (wc < nwords) && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU(); wc++)
    {
        for (i = 2; i <= lastn; i++)
        {
            if (count[i])
            {
                os[i - 2] = split_token(&cp[i], &isep);
            }
            else
            {
                os[i - 2] = 0;
            }
        }

        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        XMEMCPY(atextbuf, atext1, alen1);
        atextbuf[alen1] = 0;
        str = atextbuf;
        savep = *bufc;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(os[0]), lastn - 1);

        if (!is_same)
        {
            XMEMCPY(atextbuf, atext2, alen2);
            atextbuf[alen2] = '\0';
            dp = savep = condbuf;
            str = atextbuf;
            eval_expression_string(condbuf, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(os[0]), lastn - 1);
        }

        subpatterns = pcre_exec(re, NULL, savep, strlen(savep), 0, 0, offsets, PCRE_MAX_OFFSETS);

        if (subpatterns >= 0)
        {
            break;
        }
    }

    XFREE(re);
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
}

/*
 * ---------------------------------------------------------------------------
 * perform_grep: grep (exact match), wildgrep (wildcard match), regrep
 * (regexp match), and case-insensitive versions. (There is no
 * case-insensitive wildgrep, since all wildcard matches are caseless.)
 */

void perform_grep(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int grep_type, caseless;
    pcre *re = NULL;
    pcre_extra *study = NULL;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    char *patbuf, *patc, *attrib, *p, *bb_p;
    int ca, aflags, alen;
    dbref thing, aowner, it;
    Delim osep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    };

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    grep_type = Func_Mask(REG_TYPE);
    caseless = Func_Mask(REG_CASELESS);
    it = match_thing(player, fargs[0]);

    if (!Good_obj(it))
    {
        SAFE_NOMATCH(buff, bufc);
        return;
    }
    else if (!(Examinable(player, it)))
    {
        SAFE_NOPERM(buff, bufc);
        return;
    }

    /*
     * Make sure there's an attribute and a pattern
     */

    if (!fargs[1] || !*fargs[1])
    {
        SAFE_LB_STR("#-1 NO SUCH ATTRIBUTE", buff, bufc);
        return;
    }

    if (!fargs[2] || !*fargs[2])
    {
        SAFE_LB_STR("#-1 INVALID GREP PATTERN", buff, bufc);
        return;
    }

    switch (grep_type)
    {
    case GREP_EXACT:
        if (caseless)
        {
            for (p = fargs[2]; *p; p++)
            {
                *p = tolower(*p);
            }
        }

        break;

    case GREP_REGEXP:
        if ((re = pcre_compile(fargs[2], caseless, &errptr, &erroffset, mushstate.retabs)) == NULL)
        {
            notify_quiet(player, errptr);
            return;
        }

        study = pcre_study(re, 0, &errptr);

        if (errptr != NULL)
        {
            XFREE(re);
            notify_quiet(player, errptr);
            return;
        }

        break;

    default:
        /*
         * No special set-up steps.
         */
        break;
    }

    bb_p = *bufc;
    patc = patbuf = XMALLOC(LBUF_SIZE, "patbuf");
    XSAFESPRINTF(patbuf, &patc, "#%d/%s", it, fargs[1]);
    olist_push();

    if (parse_attrib_wild(player, patbuf, &thing, 0, 0, 1, 1))
    {
        for (ca = olist_first(); ca != NOTHING; ca = olist_next())
        {
            attrib = atr_get(thing, ca, &aowner, &aflags, &alen);

            if ((grep_type == GREP_EXACT) && caseless)
            {
                for (p = attrib; *p; p++)
                {
                    *p = tolower(*p);
                }
            }

            if (((grep_type == GREP_EXACT) && strstr(attrib, fargs[2])) || ((grep_type == GREP_WILD) && quick_wild(fargs[2], attrib)) || ((grep_type == GREP_REGEXP) && (pcre_exec(re, study, attrib, alen, 0, 0, offsets, PCRE_MAX_OFFSETS) >= 0)))
            {
                if (*bufc != bb_p)
                {
                    print_separator(&osep, buff, bufc);
                }

                SAFE_LB_STR((char *)(atr_num(ca))->name, buff, bufc);
            }

            XFREE(attrib);
        }
    }

    XFREE(patbuf);
    olist_pop();

    if (re)
    {
        XFREE(re);
    }

    if (study)
    {
        XFREE(study);
    }
}

/*
 * ---------------------------------------------------------------------------
 * Grids.
 * gridmake(<rows>, <columns>[,<grid text>][,<col odelim>][,<row odelim>])
 * gridload(<grid text>[,<odelim for row elems>][,<odelim between rows>])
 * gridset(<y range>,<x range>,<value>[,<input sep for ranges>])
 * gridsize() - returns rows cols grid( , [,<odelim for row elems>][,<odelim
 * between rows>]) - whole grid grid(<y>,<x>) - show particular coordinate
 * grid(<y range>,<x range>[,<odelim for row elems>][,<odelim between rows>])
 */

void grid_free(dbref thing, OBJGRID *ogp)
{
    int r, c;

    if (ogp)
    {
        for (r = 0; r < ogp->rows; r++)
        {
            for (c = 0; c < ogp->cols; c++)
            {
                if (ogp->data[r][c] != NULL)
                {
                    XFREE(ogp->data[r][c]);
                }
            }

            XFREE(ogp->data[r]);
        }

        nhashdelete(thing, &mushstate.objgrid_htab);
        XFREE(ogp);
    }
}

void fun_gridmake(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    OBJGRID *ogp;
    int rows, cols, dimension, r, c, status, data_rows, data_elems;
    char *rbuf, *pname;
    char **row_text, **elem_text;
    Delim csep, rsep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 5, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &(csep), DELIM_STRING))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &(rsep), DELIM_STRING))
    {
        return;
    }

    rows = (int)strtol(fargs[0], (char **)NULL, 10);
    cols = (int)strtol(fargs[1], (char **)NULL, 10);
    dimension = rows * cols;

    if ((dimension > mushconf.max_grid_size) || (dimension < 0))
    {
        SAFE_LB_STR("#-1 INVALID GRID SIZE", buff, bufc);
        return;
    }

    ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (ogp)
    {
        grid_free(player, ogp);
    }

    if (dimension == 0)
    {
        return;
    }

    /*
     * We store the grid on a row-by-row basis, i.e., the first index is
     * the y-coord and the second is the x-coord.
     */
    ogp = (OBJGRID *)XMALLOC(sizeof(OBJGRID), "ogp");
    ogp->rows = rows;
    ogp->cols = cols;
    ogp->data = (char ***)XCALLOC(rows, sizeof(char ***), "ogp->data");

    for (r = 0; r < rows; r++)
    {
        ogp->data[r] = (char **)XCALLOC(cols, sizeof(char *), "ogp->data[r]");
    }

    status = nhashadd(player, (int *)ogp, &mushstate.objgrid_htab);

    if (status < 0)
    {
        pname = log_getname(player);
        log_write(LOG_BUGS, "GRD", "MAKE", "%s Failure");
        XFREE(pname);
        grid_free(player, ogp);
        SAFE_LB_STR("#-1 FAILURE", buff, bufc);
        return;
    }

    /*
     * Populate data if we have any
     */

    if (!fargs[2] || !*fargs[2])
    {
        return;
    }

    rbuf = XMALLOC(LBUF_SIZE, "rbuf");
    XSTRCPY(rbuf, fargs[2]);
    data_rows = list2arr(&row_text, LBUF_SIZE / 2, rbuf, &rsep);

    if (data_rows > rows)
    {
        SAFE_LB_STR("#-1 TOO MANY DATA ROWS", buff, bufc);
        XFREE(rbuf);
        grid_free(player, ogp);
        return;
    }

    for (r = 0; r < data_rows; r++)
    {
        data_elems = list2arr(&elem_text, LBUF_SIZE / 2, row_text[r], &csep);

        if (data_elems > cols)
        {
            XSAFESPRINTF(buff, bufc, "#-1 ROW %d HAS TOO MANY ELEMS", r);
            XFREE(rbuf);
            grid_free(player, ogp);
            return;
        }

        for (c = 0; c < data_elems; c++)
        {
            if (ogp->data[r][c] != NULL)
            {
                XFREE(ogp->data[r][c]);
            }
            if (*elem_text[c])
            {
                ogp->data[r][c] = XSTRDUP(elem_text[c], "gp->data[gr][gc]");
            }
        }
    }

    XFREE(rbuf);
}

void fun_gridsize(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    OBJGRID *ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (!ogp)
    {
        SAFE_LB_STR("0 0", buff, bufc);
    }
    else
    {
        XSAFESPRINTF(buff, bufc, "%d %d", ogp->rows, ogp->cols);
    }
}

void fun_gridset(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    OBJGRID *ogp;
    char *xlist, *ylist;
    int n_x, n_y, r, c, i, j, errs = 0;
    char **x_elems, **y_elems;
    Delim isep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
    {
        return;
    }

    ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (!ogp)
    {
        SAFE_LB_STR("#-1 NO GRID", buff, bufc);
        return;
    }

    /*
     * Handle the common case of just one position and a simple
     * separator, first.
     */

    if ((isep.len == 1) && *fargs[0] && !strchr(fargs[0], isep.str[0]) && *fargs[1] && !strchr(fargs[1], isep.str[0]))
    {
        r = (int)strtol(fargs[0], (char **)NULL, 10) - 1;
        c = (int)strtol(fargs[1], (char **)NULL, 10) - 1;

        if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
        {
            errs++;
        }
        else
        {
            if (ogp->data[r][c] != NULL)
            {
                XFREE(ogp->data[r][c]);
            }
            if (*fargs[2])
            {
                ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
            }
        }

        if (errs)
        {
            XSAFESPRINTF(buff, bufc, "#-1 GOT %d OUT OF RANGE ERRORS", errs);
        }

        return;
    }

    /*
     * Complex ranges
     */

    if (fargs[0] && *fargs[0])
    {
        ylist = XMALLOC(LBUF_SIZE, "ylist");
        XSTRCPY(ylist, fargs[0]);
        n_y = list2arr(&y_elems, LBUF_SIZE / 2, ylist, &isep);

        if ((n_y == 1) && !*y_elems[0])
        {
            XFREE(ylist);
            n_y = -1;
        }
    }
    else
    {
        n_y = -1;
    }

    if (fargs[1] && *fargs[1])
    {
        xlist = XMALLOC(LBUF_SIZE, "xlist");
        XSTRCPY(xlist, fargs[1]);
        n_x = list2arr(&x_elems, LBUF_SIZE / 2, xlist, &isep);

        if ((n_x == 1) && !*x_elems[0])
        {
            XFREE(xlist);
            n_x = -1;
        }
    }
    else
    {
        n_x = -1;
    }

    errs = 0;

    if (n_y == -1)
    {
        for (r = 0; r < ogp->rows; r++)
        {
            if (n_x == -1)
            {
                for (c = 0; c < ogp->cols; c++)
                {
                    if (ogp->data[r][c] != NULL)
                    {
                        XFREE(ogp->data[r][c]);
                    }
                    if (*fargs[2])
                    {
                        ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                    }
                }
            }
            else
            {
                for (i = 0; i < n_x; i++)
                {
                    c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                    if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
                    {
                        errs++;
                    }
                    else
                    {
                        if (ogp->data[r][c] != NULL)
                        {
                            XFREE(ogp->data[r][c]);
                        }
                        if (*fargs[2])
                        {
                            ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (j = 0; j < n_y; j++)
        {
            r = (int)strtol(y_elems[j], (char **)NULL, 10) - 1;

            if ((r < 0) || (r >= ogp->rows))
            {
                errs++;
            }
            else
            {
                if (n_x == -1)
                {
                    for (c = 0; c < ogp->cols; c++)
                    {
                        if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
                        {
                            errs++;
                        }
                        else
                        {
                            if (ogp->data[r][c] != NULL)
                            {
                                XFREE(ogp->data[r][c]);
                            }
                            if (*fargs[2])
                            {
                                ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < n_x; i++)
                    {
                        c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                        if ((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols))
                        {
                            errs++;
                        }
                        else
                        {
                            if (ogp->data[r][c] != NULL)
                            {
                                XFREE(ogp->data[r][c]);
                            }
                            if (*fargs[2])
                            {
                                ogp->data[r][c] = XSTRDUP(fargs[2], "gp->data[gr][gc]");
                            }
                        }
                    }
                }
            }
        }
    }

    if (n_x > 0)
    {
        XFREE(xlist);
    }

    if (n_y > 0)
    {
        XFREE(ylist);
    }

    if (errs)
    {
        XSAFESPRINTF(buff, bufc, "#-1 GOT %d OUT OF RANGE ERRORS", errs);
    }
}

void fun_grid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim csep, rsep;
    OBJGRID *ogp;
    char *xlist, *ylist;
    int n_x, n_y, r, c, i, j;
    char **x_elems, **y_elems;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 4, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &(csep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &(rsep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    ogp = ((OBJGRID *)nhashfind(player, &mushstate.objgrid_htab));

    if (!ogp)
    {
        SAFE_LB_STR("#-1 NO GRID", buff, bufc);
        return;
    }

    /*
     * Handle the common case of just one position, first
     */

    if (fargs[0] && *fargs[0] && !strchr(fargs[0], ' ') && fargs[1] && *fargs[1] && !strchr(fargs[1], ' '))
    {
        r = (int)strtol(fargs[0], (char **)NULL, 10) - 1;
        c = (int)strtol(fargs[1], (char **)NULL, 10) - 1;

        if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
        {
            SAFE_LB_STR((ogp)->data[(r)][(c)], buff, bufc);
        }

        return;
    }

    /*
     * Complex ranges
     */

    if (!fargs[0] || !*fargs[0])
    {
        n_y = -1;
    }
    else
    {
        ylist = XMALLOC(LBUF_SIZE, "ylist");
        XSTRCPY(ylist, fargs[0]);
        n_y = list2arr(&y_elems, LBUF_SIZE / 2, ylist, &SPACE_DELIM);

        if ((n_y == 1) && !*y_elems[0])
        {
            XFREE(ylist);
            n_y = -1;
        }
    }

    if (!fargs[1] || !*fargs[1])
    {
        n_x = -1;
    }
    else
    {
        xlist = XMALLOC(LBUF_SIZE, "xlist");
        XSTRCPY(xlist, fargs[1]);
        n_x = list2arr(&x_elems, LBUF_SIZE / 2, xlist, &SPACE_DELIM);

        if ((n_x == 1) && !*x_elems[0])
        {
            XFREE(xlist);
            n_x = -1;
        }
    }

    if (n_y == -1)
    {
        for (r = 0; r < ogp->rows; r++)
        {
            if (r != 0)
            {
                print_separator(&rsep, buff, bufc);
            }

            if (n_x == -1)
            {
                for (c = 0; c < ogp->cols; c++)
                {
                    if (c != 0)
                    {
                        print_separator(&csep, buff, bufc);
                    }
                    if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
                    {
                        SAFE_LB_STR(ogp->data[r][c], buff, bufc);
                    }
                }
            }
            else
            {
                for (i = 0; i < n_x; i++)
                {
                    c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                    if (i != 0)
                    {
                        print_separator(&csep, buff, bufc);
                    }
                    if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
                    {
                        SAFE_LB_STR(ogp->data[r][c], buff, bufc);
                    }
                }
            }
        }
    }
    else
    {
        for (j = 0; j < n_y; j++)
        {
            if (j != 0)
            {
                print_separator(&rsep, buff, bufc);
            }

            r = (int)strtol(y_elems[j], (char **)NULL, 10) - 1;

            if (!((r < 0) || (r >= ogp->rows)))
            {
                if (n_x == -1)
                {
                    for (c = 0; c < ogp->cols; c++)
                    {
                        if (c != 0)
                        {
                            print_separator(&csep, buff, bufc);
                        }
                        if (!((r < 0) || (c < 0) || (r >= ogp->rows) || ((c) >= ogp->cols) || (ogp->data[r][c] == NULL)))
                        {
                            SAFE_LB_STR(ogp->data[r][c], buff, bufc);
                        }
                    }
                }
                else
                {
                    for (i = 0; i < n_x; i++)
                    {
                        c = (int)strtol(x_elems[i], (char **)NULL, 10) - 1;
                        if (i != 0)
                        {
                            print_separator(&csep, buff, bufc);
                        }
                        if (!((r < 0) || (c < 0) || (r >= ogp->rows) || (c >= ogp->cols) || (ogp->data[r][c] == NULL)))
                        {
                            SAFE_LB_STR(ogp->data[r][c], buff, bufc);
                        }
                    }
                }
            }
        }
    }

    if (n_x > 0)
    {
        XFREE(xlist);
    }

    if (n_y > 0)
    {
        XFREE(ylist);
    }
}
