/**
 * @file functions.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief MUSH function handlers
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

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <dlfcn.h>

extern int parse_ext_access(int *, EXTFUNCS **, char *, NAMETAB *, dbref, char *);

UFUN *ufun_head;
UFUN *ufun_tail;
OBJXFUNCS xfunctions;
const Delim SPACE_DELIM = {1, " "};

/**
 * @brief Initialize the function hashtable
 *
 */
void init_functab(void)
{
    FUN *fp;
    hashinit(&mushstate.func_htab, 250 * mushconf.hash_factor, HT_STR | HT_KEYREF);

    for (fp = flist; fp->name; fp++)
    {
        hashadd((char *)fp->name, (int *)fp, &mushstate.func_htab, 0);
    }

    ufun_head = NULL;
    ufun_tail = NULL;
    hashinit(&mushstate.ufunc_htab, 15 * mushconf.hash_factor, HT_STR);

    xfunctions.func = NULL;
    xfunctions.count = 0;
}

/**
 * @brief Add a function
 *
 * @param player DBref of player
 * @param cause Not used
 * @param key Key
 * @param fname Function name
 * @param target Target
 */
void do_function(dbref player, dbref cause __attribute__((unused)), int key, char *fname, char *target)
{
    UFUN *ufp, *ufp2;
    ATTR *ap;
    char *np, *bp;
    int atr, aflags;
    dbref obj, aowner;

    /**
     * Check for list first
     *
     */
    if (key & FUNCT_LIST)
    {
        if (fname && *fname)
        {
            /**
             * Make it case-insensitive, and look it up.
             * Note: UFUN names stored lowercase for consistent lookup.
             *
             */
            for (bp = fname; *bp; bp++)
            {
                *bp = tolower(*bp);
            }

            ufp = (UFUN *)hashfind(fname, &mushstate.ufunc_htab);

            if (ufp)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/%s", ufp->name, ufp->obj, ((ATTR *)atr_num(ufp->atr))->name);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s not found in user function table.", fname);
            }

            return;
        }

        /**
         * No name given, list them all.
         *
         */
        for (ufp = (UFUN *)hash_firstentry(&mushstate.ufunc_htab); ufp != NULL; ufp = (UFUN *)hash_nextentry(&mushstate.ufunc_htab))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/%s", ufp->name, ufp->obj, ((ATTR *)atr_num(ufp->atr))->name);
        }

        return;
    }

    /**
     * Make a local uppercase copy of the function name for builtin check
     *
     */
    bp = np = XMALLOC(SBUF_SIZE, "np");
    XSAFESBSTR(fname, np, &bp);
    *bp = '\0';

    char *np_upper = XMALLOC(SBUF_SIZE, "np_upper");
    XSTRCPY(np_upper, np);

    for (bp = np_upper; *bp; bp++)
    {
        *bp = toupper(*bp);
    }

    /**
     * Verify that the function doesn't exist in the builtin table
     *
     */
    if (hashfind(np_upper, &mushstate.func_htab) != NULL)
    {
        notify_quiet(player, "Function already defined in builtin function table.");
        XFREE(np_upper);
        XFREE(np);
        return;
    }

    XFREE(np_upper);

    /**
     * Normalize to lowercase for UFUN hash lookups
     */
    for (bp = np; *bp; bp++)
    {
        *bp = tolower(*bp);
    }

    /**
     * Make sure the target object exists
     *
     */
    if (!parse_attrib(player, target, &obj, &atr, 0))
    {
        notify_quiet(player, NOMATCH_MESSAGE);
        XFREE(np);
        return;
    }

    /**
     * Make sure the attribute exists
     *
     */
    if (atr == NOTHING)
    {
        notify_quiet(player, "No such attribute.");
        XFREE(np);
        return;
    }

    /**
     * Make sure attribute is readably by me
     *
     */
    ap = atr_num(atr);

    if (!ap)
    {
        notify_quiet(player, "No such attribute.");
        XFREE(np);
        return;
    }

    atr_get_info(obj, atr, &aowner, &aflags);

    if (!See_attr(player, obj, ap, aowner, aflags))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        XFREE(np);
        return;
    }

    /**
     * Privileged functions require you control the obj.
     *
     */
    if ((key & FUNCT_PRIV) && !Controls(player, obj))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        XFREE(np);
        return;
    }

    /**
     * See if function already exists.  If so, redefine it
     *
     */
    ufp = (UFUN *)hashfind(np, &mushstate.ufunc_htab);

    if (!ufp)
    {
        ufp = (UFUN *)XMALLOC(sizeof(UFUN), "ufp");
        ufp->name = XSTRDUP(np, "ufp->name");

        /**
         * Store name in lowercase for case-insensitive lookup consistency.
         */
        for (bp = (char *)ufp->name; *bp; bp++)
        {
            *bp = tolower(*bp);
        }

        ufp->obj = obj;
        ufp->atr = atr;
        ufp->perms = CA_PUBLIC;
        ufp->next = NULL;

        /**
         * Append to tail for O(1) insertion.
         */
        if (!ufun_head)
        {
            ufun_head = ufp;
            ufun_tail = ufp;
        }
        else
        {
            ufun_tail->next = ufp;
            ufun_tail = ufp;
        }

        /**
         * Add to hash table (np already lowercase).
         */
        if (hashadd(np, (int *)ufp, &mushstate.ufunc_htab, 0))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Function %s not defined.", fname);
            XFREE(ufp->name);
            XFREE(ufp);
            XFREE(np);
            return;
        }
    }

    ufp->obj = obj;
    ufp->atr = atr;
    ufp->flags = 0;

    if (key & FUNCT_NO_EVAL)
    {
        ufp->flags |= FN_NO_EVAL;
    }

    if (key & FUNCT_PRIV)
    {
        ufp->flags |= FN_PRIV;
    }

    if (key & FUNCT_NOREGS)
    {
        ufp->flags |= FN_NOREGS;
    }
    else if (key & FUNCT_PRES)
    {
        ufp->flags |= FN_PRES;
    }

    XFREE(np);

    if (!Quiet(player))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Function %s defined.", fname);
    }
}

/**
 * @brief List available functions.
 *
 * @param player
 */
void list_functable(dbref player)
{
    char *buf = XMALLOC(LBUF_SIZE, "buf");
    XSTRCPY(buf, "Built-in functions:");

    for (FUN *fp = flist; fp->name; fp++)
    {
        if (Check_Func_Access(player, fp))
        {
            XSPRINTFCAT(buf, " %s", fp->name);
        }
    }

    notify(player, buf);

    for (MODULE *mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        FUN *modfns;
        char *s = XASPRINTF("s", "mod_%s_%s", mp->modname, "functable");

        if ((modfns = (FUN *)dlsym(mp->handle, s)) != NULL)
        {
            XSPRINTF(buf, "Module %s functions:", mp->modname);
            for (FUN *fp = modfns; fp->name; fp++)
            {
                if (Check_Func_Access(player, fp))
                {
                    XSPRINTFCAT(buf, " %s", fp->name);
                }
            }

            notify(player, buf);
        }
        XFREE(s);
    }

    XSTRCPY(buf, "User-defined functions:");
    for (UFUN *ufp = ufun_head; ufp; ufp = ufp->next)
    {
        if (check_access(player, ufp->perms))
        {
            XSPRINTFCAT(buf, " %s", ufp->name);
        }
    }

    notify(player, buf);
    XFREE(buf);
}

/*
 * ---------------------------------------------------------------------------
 * list_funcaccess: List access on functions.
 */

/**
 * @brief Helper function for list_funcaccess
 *
 * @param player DBref of player
 * @param fp Function
 */
void helper_list_funcaccess(dbref player, FUN *fp)
{
    char *buff = XMALLOC(SBUF_SIZE, "list_funcaccess");
    char *bp = NULL;

    while (fp->name)
    {
        if (Check_Func_Access(player, fp))
        {
            bp = buff;
            if (!fp->xperms)
            {
                XSPRINTF(buff, "%-30.30s ", fp->name);
            }
            else
            {
                XSAFELBSTR((char *)fp->name, buff, &bp);
                XSAFELBCHR(':', buff, &bp);

                for (int i = 0; i < fp->xperms->num_funcs; i++)
                {
                    if (fp->xperms->ext_funcs[i])
                    {
                        XSAFELBCHR(' ', buff, &bp);
                        XSAFELBSTR(fp->xperms->ext_funcs[i]->fn_name, buff, &bp);
                    }
                }
            }

            listset_nametab(player, access_nametab, fp->perms, true, buff);
        }

        fp++;
    }
}

/**
 * @brief List function access rights
 *
 * @param player DBref of player
 */
void list_funcaccess(dbref player)
{
    FUN *ftab = NULL;
    UFUN *ufp = NULL;
    MODULE *mp = NULL;
    bool header = false;

    notify(player, "Build-in                       Access");
    notify(player, "------------------------------ ------------------------------------------------");

    helper_list_funcaccess(player, flist);

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        char *buff = XASPRINTF("s", "mod_%s_%s", mp->modname, "functable");
        if ((ftab = (FUN *)dlsym(mp->handle, buff)) != NULL)
        {
            raw_notify(player, "\nModule %-23.23s Access", mp->modname);
            notify(player, "------------------------------ ------------------------------------------------");
            helper_list_funcaccess(player, ftab);
        }
        XFREE(buff);
    }

    header = false;
    for (ufp = ufun_head; ufp; ufp = ufp->next)
    {
        if (check_access(player, ufp->perms))
        {
            if (!header)
            {
                notify(player, "\nUser-defined                   Access");
                notify(player, "------------------------------ ------------------------------------------------");
                header = true;
            }
            char *buff = XASPRINTF("s", "%14s:", ufp->name);
            listset_nametab(player, access_nametab, ufp->perms, true, buff);
            XFREE(buff);
        }
    }

    notify(player, "-------------------------------------------------------------------------------");
}

/*
 * ---------------------------------------------------------------------------
 * cf_func_access:
 */

/**
 * @brief Set access on functions
 *
 * @param vp Not used
 * @param str Name of function
 * @param extra Nametab
 * @param player DBref of player
 * @param cmd Command
 * @return int Result
 */
int cf_func_access(int *vp __attribute__((unused)), char *str, long extra, dbref player, char *cmd)
{
    FUN *fp;
    UFUN *ufp;
    char *ap;

    for (ap = str; *ap && !isspace(*ap); ap++)
    {
        /* Skip over */
    }

    if (*ap)
    {
        *ap++ = '\0';
    }

    for (fp = flist; fp->name; fp++)
    {
        if (!string_compare(fp->name, str))
        {
            return (parse_ext_access(&fp->perms, &fp->xperms, ap, (NAMETAB *)extra, player, cmd));
        }
    }

    for (ufp = ufun_head; ufp; ufp = ufp->next)
    {
        if (!string_compare(ufp->name, str))
        {
            return (cf_modify_bits(&ufp->perms, ap, extra, player, cmd));
        }
    }

    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Function", str);
    return -1;
}

/**
 * @brief List of existing MUSHCode functions in alphabetical order.
 *
 */
FUN flist[] = {
    /**
     * - @ -
     */
    {"@@", fun_null, 1, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    /**
     * - A -
     */
    {"ABS", fun_abs, 1, 0, CA_PUBLIC, NULL},
    {"ACOS", handle_trig, 1, TRIG_ARC | TRIG_CO, CA_PUBLIC, NULL},
    {"ACOSD", handle_trig, 1, TRIG_ARC | TRIG_CO | TRIG_DEG, CA_PUBLIC, NULL},
    {"ADD", fun_add, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"AFTER", fun_after, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"ALIGN", fun_align, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"ALPHAMAX", fun_alphamax, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"ALPHAMIN", fun_alphamin, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"AND", handle_logic, 0, FN_VARARGS | LOGIC_AND, CA_PUBLIC, NULL},
    {"ANDBOOL", handle_logic, 0, FN_VARARGS | LOGIC_AND | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"ANDFLAGS", handle_flaglists, 2, 0, CA_PUBLIC, NULL},
    {"ANSI", fun_ansi, 2, 0, CA_PUBLIC, NULL},
    {"ANSIPOS", fun_ansipos, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"APOSS", handle_pronoun, 1, PRONOUN_APOSS, CA_PUBLIC, NULL},
    {"ART", fun_art, 1, 0, CA_PUBLIC, NULL},
    {"ASIN", handle_trig, 1, TRIG_ARC, CA_PUBLIC, NULL},
    {"ASIND", handle_trig, 1, TRIG_ARC | TRIG_DEG, CA_PUBLIC, NULL},
    {"ATAN", handle_trig, 1, TRIG_ARC | TRIG_TAN, CA_PUBLIC, NULL},
    {"ATAND", handle_trig, 1, TRIG_ARC | TRIG_TAN | TRIG_DEG, CA_PUBLIC, NULL},
    /**
     * - B -
     */
    {"BAND", fun_band, 2, 0, CA_PUBLIC, NULL},
    {"BASECONV", fun_baseconv, 3, 0, CA_PUBLIC, NULL},
    {"BEEP", fun_beep, 0, 0, CA_WIZARD, NULL},
    {"BEFORE", fun_before, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"BENCHMARK", fun_benchmark, 2, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"BNAND", fun_bnand, 2, 0, CA_PUBLIC, NULL},
    {"BOUND", fun_bound, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"BOR", fun_bor, 2, 0, CA_PUBLIC, NULL},
    {"BORDER", perform_border, 0, FN_VARARGS | JUST_LEFT, CA_PUBLIC, NULL},
    /**
     * - C -
     */
    {"CANDBOOL", handle_logic, 0, FN_VARARGS | FN_NO_EVAL | LOGIC_AND | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"CAND", handle_logic, 0, FN_VARARGS | FN_NO_EVAL | LOGIC_AND, CA_PUBLIC, NULL},
    {"CAPSTR", fun_capstr, -1, 0, CA_PUBLIC, NULL},
    {"CASE", fun_case, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"CAT", fun_cat, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"CBORDER", perform_border, 0, FN_VARARGS | JUST_CENTER, CA_PUBLIC, NULL},
    {"CCOUNT", fun_ccount, 0, 0, CA_PUBLIC, NULL},
    {"CDEPTH", fun_cdepth, 0, 0, CA_PUBLIC, NULL},
    {"CEIL", fun_ceil, 1, 0, CA_PUBLIC, NULL},
    {"CENTER", fun_center, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"CHILDREN", fun_children, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"CHOMP", fun_chomp, 1, 0, CA_PUBLIC, NULL},
    {"CHOOSE", fun_choose, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"CLEARVARS", fun_clearvars, 0, FN_VARFX, CA_PUBLIC, NULL},
    {"COLUMNS", fun_columns, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"COMMAND", fun_command, 0, FN_VARARGS | FN_DBFX, CA_PUBLIC, NULL},
    {"COMP", fun_comp, 2, 0, CA_PUBLIC, NULL},
    {"CON", fun_con, 1, 0, CA_PUBLIC, NULL},
    {"CONFIG", fun_config, 1, 0, CA_PUBLIC, NULL},
    {"CONN", handle_conninfo, 1, 0, CA_PUBLIC, NULL},
    {"CONNRECORD", fun_connrecord, 0, 0, CA_PUBLIC, NULL},
    {"CONSTRUCT", fun_construct, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    {"CONTROLS", fun_controls, 2, 0, CA_PUBLIC, NULL},
    {"CONVSECS", fun_convsecs, 1, 0, CA_PUBLIC, NULL},
    {"CONVTIME", fun_convtime, 1, 0, CA_PUBLIC, NULL},
    {"COR", handle_logic, 0, FN_VARARGS | FN_NO_EVAL | LOGIC_OR, CA_PUBLIC, NULL},
    {"CORBOOL", handle_logic, 0, FN_VARARGS | FN_NO_EVAL | LOGIC_OR | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"COS", handle_trig, 1, TRIG_CO, CA_PUBLIC, NULL},
    {"COSD", handle_trig, 1, TRIG_CO | TRIG_DEG, CA_PUBLIC, NULL},
    {"CREATE", fun_create, 0, FN_VARARGS | FN_DBFX, CA_PUBLIC, NULL},
    {"CREATION", handle_timestamp, 1, TIMESTAMP_CRE, CA_PUBLIC, NULL},
    {"CTABLES", process_tables, 0, FN_VARARGS | JUST_CENTER, CA_PUBLIC, NULL},
    /**
     * - D -
     */
    {"DEC", fun_dec, 1, 0, CA_PUBLIC, NULL},
    {"DECRYPT", fun_decrypt, 2, 0, CA_PUBLIC, NULL},
    {"DEFAULT", fun_default, 2, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"DELETE", fun_delete, 3, 0, CA_PUBLIC, NULL},
    {"DELIMIT", fun_delimit, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    {"DESTRUCT", fun_destruct, 1, FN_VARFX, CA_PUBLIC, NULL},
    {"DIE", fun_die, 2, 0, CA_PUBLIC, NULL},
    {"DIFFPOS", fun_diffpos, 2, 0, CA_PUBLIC, NULL},
    {"DIST2D", fun_dist2d, 4, 0, CA_PUBLIC, NULL},
    {"DIST3D", fun_dist3d, 6, 0, CA_PUBLIC, NULL},
    {"DIV", fun_div, 2, 0, CA_PUBLIC, NULL},
    {"DOING", fun_doing, 1, 0, CA_PUBLIC, NULL},
    {"DUP", fun_dup, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    /**
     * - E -
     */
    {"E", fun_e, 1, 0, CA_PUBLIC, NULL},
    {"EDEFAULT", fun_edefault, 2, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"EDIT", fun_edit, 3, 0, CA_PUBLIC, NULL},
    {"ELEMENTS", fun_elements, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"ELOCK", fun_elock, 2, 0, CA_PUBLIC, NULL},
    {"ELOCKSTR", fun_elockstr, 3, 0, CA_PUBLIC, NULL},
    {"EMPTY", fun_empty, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    {"ENCRYPT", fun_encrypt, 2, 0, CA_PUBLIC, NULL},
    {"ENTRANCES", fun_entrances, 0, FN_VARARGS, CA_NO_GUEST, NULL},
    {"EQ", fun_eq, 2, 0, CA_PUBLIC, NULL},
    {"ESC", fun_esc, -1, 0, CA_PUBLIC, NULL},
    {"ESCAPE", fun_escape, -1, 0, CA_PUBLIC, NULL},
    {"ETIMEFMT", fun_etimefmt, 2, 0, CA_PUBLIC, NULL},
    {"EXCLUDE", fun_exclude, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"EXIT", fun_exit, 1, 0, CA_PUBLIC, NULL},
    {"EXP", fun_exp, 1, 0, CA_PUBLIC, NULL},
    {"EXTRACT", fun_extract, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"EVAL", fun_eval, 0, FN_VARARGS | GET_EVAL | GET_XARGS, CA_PUBLIC, NULL},
    /**
     * - F -
     */
    {"FCOUNT", fun_fcount, 0, 0, CA_PUBLIC, NULL},
    {"FDEPTH", fun_fdepth, 0, 0, CA_PUBLIC, NULL},
    {"FDIV", fun_fdiv, 2, 0, CA_PUBLIC, NULL},
    {"FILTER", handle_filter, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"FILTERBOOL", handle_filter, 0, FN_VARARGS | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"FINDABLE", fun_findable, 2, 0, CA_PUBLIC, NULL},
    {"FIRST", fun_first, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"FLAGS", fun_flags, 1, 0, CA_PUBLIC, NULL},
    {"FLOOR", fun_floor, 1, 0, CA_PUBLIC, NULL},
    {"FLOORDIV", fun_floordiv, 2, 0, CA_PUBLIC, NULL},
    {"FOLD", fun_fold, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"FORCE", fun_force, 2, FN_QFX, CA_PUBLIC, NULL},
    {"FOREACH", fun_foreach, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"FULLNAME", handle_name, 1, NAMEFN_FULLNAME, CA_PUBLIC, NULL},
    /**
     * - G -
     */
    {"GET", perform_get, 1, 0, CA_PUBLIC, NULL},
    {"GET_EVAL", perform_get, 1, GET_EVAL, CA_PUBLIC, NULL},
    {"GRAB", fun_grab, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"GRABALL", fun_graball, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"GREP", perform_grep, 0, FN_VARARGS | GREP_EXACT, CA_PUBLIC, NULL},
    {"GREPI", perform_grep, 0, FN_VARARGS | GREP_EXACT | REG_CASELESS, CA_PUBLIC, NULL},
    {"GRID", fun_grid, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"GRIDMAKE", fun_gridmake, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"GRIDSET", fun_gridset, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"GRIDSIZE", fun_gridsize, 0, 0, CA_PUBLIC, NULL},
    {"GROUP", fun_group, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"GT", fun_gt, 2, 0, CA_PUBLIC, NULL},
    {"GTE", fun_gte, 2, 0, CA_PUBLIC, NULL},
    /**
     * - H -
     */
    {"HASATTR", fun_hasattr, 2, 0, CA_PUBLIC, NULL},
    {"HASATTRP", fun_hasattr, 2, CHECK_PARENTS, CA_PUBLIC, NULL},
    {"HASFLAG", fun_hasflag, 2, 0, CA_PUBLIC, NULL},
    {"HASFLAGS", fun_hasflags, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"HASMODULE", fun_hasmodule, 1, 0, CA_PUBLIC, NULL},
    {"HASPOWER", fun_haspower, 2, 0, CA_PUBLIC, NULL},
    {"HASTYPE", fun_hastype, 2, 0, CA_PUBLIC, NULL},
    {"HEARS", handle_okpres, 2, PRESFN_HEARS, CA_PUBLIC, NULL},
    {"HELPTEXT", fun_helptext, 2, 0, CA_PUBLIC, NULL},
    {"HOME", fun_home, 1, 0, CA_PUBLIC, NULL},
    {"HTML_ESCAPE", fun_html_escape, -1, 0, CA_PUBLIC, NULL},
    {"HTML_UNESCAPE", fun_html_unescape, -1, 0, CA_PUBLIC, NULL},
    /**
     * - I -
     */
    {"IBREAK", fun_ibreak, 1, 0, CA_PUBLIC, NULL},
    {"IDLE", handle_conninfo, 1, CONNINFO_IDLE, CA_PUBLIC, NULL},
    {"IFELSE", handle_ifelse, 0, IFELSE_BOOL | FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"IFTRUE", handle_ifelse, 0, IFELSE_TOKEN | IFELSE_BOOL | FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"IFFALSE", handle_ifelse, 0, IFELSE_FALSE | IFELSE_TOKEN | IFELSE_BOOL | FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"IFZERO", handle_ifelse, 0, IFELSE_FALSE | FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"ILEV", fun_ilev, 0, 0, CA_PUBLIC, NULL},
    {"INC", fun_inc, 1, 0, CA_PUBLIC, NULL},
    {"INDEX", fun_index, 4, 0, CA_PUBLIC, NULL},
    {"INSERT", fun_insert, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"INUM", fun_inum, 1, 0, CA_PUBLIC, NULL},
    {"INZONE", scan_zone, 1, TYPE_ROOM, CA_PUBLIC, NULL},
    {"ISALNUM", fun_isalnum, 1, 0, CA_PUBLIC, NULL},
    {"ISDBREF", fun_isdbref, 1, 0, CA_PUBLIC, NULL},
    {"ISNUM", fun_isnum, 1, 0, CA_PUBLIC, NULL},
    {"ISOBJID", fun_isobjid, 1, 0, CA_PUBLIC, NULL},
    {"ISWORD", fun_isword, 1, 0, CA_PUBLIC, NULL},
    {"ISORT", handle_sort, 0, FN_VARARGS | SORT_POS, CA_PUBLIC, NULL},
    {"ITEMIZE", fun_itemize, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"ITEMS", fun_items, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    {"ITER", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_NONE | FILT_COND_NONE, CA_PUBLIC, NULL},
    {"ITER2", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_NONE | FILT_COND_NONE | LOOP_TWOLISTS, CA_PUBLIC, NULL},
    {"ISFALSE", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_NONE | FILT_COND_FALSE, CA_PUBLIC, NULL},
    {"ISTRUE", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_NONE | FILT_COND_TRUE, CA_PUBLIC, NULL},
    {"ITEXT", fun_itext, 1, 0, CA_PUBLIC, NULL},
    {"ITEXT2", fun_itext2, 1, 0, CA_PUBLIC, NULL},
    /**
     * - J -
     */
    {"JOIN", fun_join, 0, FN_VARARGS, CA_PUBLIC, NULL},
    /**
     * - K -
     */
    {"KNOWS", handle_okpres, 2, PRESFN_KNOWS, CA_PUBLIC, NULL},
    /**
     * - L -
     */
    {"LADD", fun_ladd, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LALIGN", fun_lalign, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LAND", handle_logic, 0, FN_VARARGS | LOGIC_LIST | LOGIC_AND, CA_PUBLIC, NULL},
    {"LANDBOOL", handle_logic, 0, FN_VARARGS | LOGIC_LIST | LOGIC_AND | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"LAST", fun_last, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LASTACCESS", handle_timestamp, 1, TIMESTAMP_ACC, CA_PUBLIC, NULL},
    {"LASTCREATE", fun_lastcreate, 2, 0, CA_PUBLIC, NULL},
    {"LASTMOD", handle_timestamp, 1, TIMESTAMP_MOD, CA_PUBLIC, NULL},
    {"LATTR", handle_lattr, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LCON", fun_lcon, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LCSTR", fun_lcstr, -1, 0, CA_PUBLIC, NULL},
    {"LDELETE", fun_ldelete, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LDIFF", handle_sets, 0, FN_VARARGS | SET_TYPE | SET_DIFF, CA_PUBLIC, NULL},
    {"LEDIT", fun_ledit, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LEFT", fun_left, 2, 0, CA_PUBLIC, NULL},
    {"LET", fun_let, 0, FN_VARARGS | FN_NO_EVAL | FN_VARFX, CA_PUBLIC, NULL},
    {"LEXITS", fun_lexits, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LFALSE", handle_listbool, 0, FN_VARARGS | IFELSE_BOOL | IFELSE_FALSE, CA_PUBLIC, NULL},
    {"LIST", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | FN_OUTFX | BOOL_COND_NONE | FILT_COND_NONE | LOOP_NOTIFY, CA_PUBLIC, NULL},
    {"LIST2", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | FN_OUTFX | BOOL_COND_NONE | FILT_COND_NONE | LOOP_NOTIFY | LOOP_TWOLISTS, CA_PUBLIC, NULL},
    {"LIT", fun_lit, -1, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"LINK", fun_link, 2, FN_DBFX, CA_PUBLIC, NULL},
    {"LINSTANCES", fun_linstances, 0, FN_VARFX, CA_PUBLIC, NULL},
    {"LINTER", handle_sets, 0, FN_VARARGS | SET_TYPE | SET_INTERSECT, CA_PUBLIC, NULL},
    {"LJUST", fun_ljust, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LMAX", fun_lmax, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LMIN", fun_lmin, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LN", fun_ln, 1, 0, CA_PUBLIC, NULL},
    {"LNUM", fun_lnum, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LOAD", fun_load, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    {"LOC", handle_loc, 1, 0, CA_PUBLIC, NULL},
    {"LOCATE", fun_locate, 3, 0, CA_PUBLIC, NULL},
    {"LOCALIZE", fun_localize, 1, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"LOCK", fun_lock, 1, 0, CA_PUBLIC, NULL},
    {"LOG", fun_log, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LPARENT", fun_lparent, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LOOP", perform_loop, 0, FN_VARARGS | FN_NO_EVAL | FN_OUTFX | LOOP_NOTIFY, CA_PUBLIC, NULL},
    {"LOR", handle_logic, 0, FN_VARARGS | LOGIC_LIST | LOGIC_OR, CA_PUBLIC, NULL},
    {"LORBOOL", handle_logic, 0, FN_VARARGS | LOGIC_LIST | LOGIC_OR | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"LPOS", fun_lpos, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LRAND", fun_lrand, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LREGS", fun_lregs, 0, 0, CA_PUBLIC, NULL},
    {"LREPLACE", fun_lreplace, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"LSTACK", fun_lstack, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    {"LSTRUCTURES", fun_lstructures, 0, FN_VARFX, CA_PUBLIC, NULL},
    {"LT", fun_lt, 2, 0, CA_PUBLIC, NULL},
    {"LTE", fun_lte, 2, 0, CA_PUBLIC, NULL},
    {"LTRUE", handle_listbool, 0, FN_VARARGS | IFELSE_BOOL, CA_PUBLIC, NULL},
    {"LUNION", handle_sets, 0, FN_VARARGS | SET_TYPE | SET_UNION, CA_PUBLIC, NULL},
    {"LVARS", fun_lvars, 0, FN_VARFX, CA_PUBLIC, NULL},
    {"LWHO", fun_lwho, 0, 0, CA_PUBLIC, NULL},
    /**
     * - M -
     */
    {"MAP", fun_map, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MATCH", fun_match, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MATCHALL", fun_matchall, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MAX", fun_max, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MEMBER", fun_member, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MERGE", fun_merge, 3, 0, CA_PUBLIC, NULL},
    {"MID", fun_mid, 3, 0, CA_PUBLIC, NULL},
    {"MIN", fun_min, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MIX", fun_mix, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MODULO", fun_modulo, 2, 0, CA_PUBLIC, NULL},
    {"MODIFY", fun_modify, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    {"MODULES", fun_modules, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MONEY", fun_money, 1, 0, CA_PUBLIC, NULL},
    {"MOVES", handle_okpres, 2, PRESFN_MOVES, CA_PUBLIC, NULL},
    {"MUDNAME", fun_mushname, 0, 0, CA_PUBLIC, NULL},
    {"MUSHNAME", fun_mushname, 0, 0, CA_PUBLIC, NULL},
    {"MUL", fun_mul, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MUNGE", fun_munge, 0, FN_VARARGS, CA_PUBLIC, NULL},
    /**
     * - N -
     */
    {"NAME", handle_name, 1, 0, CA_PUBLIC, NULL},
    {"NATTR", handle_lattr, 1, LATTR_COUNT, CA_PUBLIC, NULL},
    {"NCOMP", fun_ncomp, 2, 0, CA_PUBLIC, NULL},
    {"NEARBY", fun_nearby, 2, 0, CA_PUBLIC, NULL},
    {"NEQ", fun_neq, 2, 0, CA_PUBLIC, NULL},
    {"NESCAPE", fun_escape, -1, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"NEXT", fun_next, 1, 0, CA_PUBLIC, NULL},
    {"NOFX", fun_nofx, 2, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"NONZERO", handle_ifelse, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"NOT", fun_not, 1, 0, CA_PUBLIC, NULL},
    {"NOTBOOL", fun_notbool, 1, 0, CA_PUBLIC, NULL},
    {"NSECURE", fun_secure, -1, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"NULL", fun_null, 1, 0, CA_PUBLIC, NULL},
    {"NUM", fun_num, 1, 0, CA_PUBLIC, NULL},
    /**
     * - O -
     */
    {"OBJ", handle_pronoun, 1, PRONOUN_OBJ, CA_PUBLIC, NULL},
    {"OBJCALL", fun_objcall, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"OBJEVAL", fun_objeval, 2, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"OBJID", fun_objid, 1, 0, CA_PUBLIC, NULL},
    {"OBJMEM", fun_objmem, 1, 0, CA_PUBLIC, NULL},
    {"OEMIT", fun_oemit, 2, FN_OUTFX, CA_PUBLIC, NULL},
    {"OR", handle_logic, 0, FN_VARARGS | LOGIC_OR, CA_PUBLIC, NULL},
    {"ORBOOL", handle_logic, 0, FN_VARARGS | LOGIC_OR | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"ORFLAGS", handle_flaglists, 2, LOGIC_OR, CA_PUBLIC, NULL},
    {"OWNER", fun_owner, 1, 0, CA_PUBLIC, NULL},
    /**
     * - P -
     */
    {"PARENT", fun_parent, 1, 0, CA_PUBLIC, NULL},
    {"PARSE", perform_loop, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"PEEK", handle_pop, 0, FN_VARARGS | FN_STACKFX | POP_PEEK, CA_PUBLIC, NULL},
    {"PEMIT", fun_pemit, 2, FN_OUTFX, CA_PUBLIC, NULL},
    {"PFIND", fun_pfind, 1, 0, CA_PUBLIC, NULL},
    {"PI", fun_pi, 1, 0, CA_PUBLIC, NULL},
    {"PLAYMEM", fun_playmem, 1, 0, CA_PUBLIC, NULL},
    {"PMATCH", fun_pmatch, 1, 0, CA_PUBLIC, NULL},
    {"POP", handle_pop, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    {"POPN", fun_popn, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    {"PORTS", fun_ports, 0, FN_VARARGS, CA_WIZARD, NULL},
    {"POS", fun_pos, 2, 0, CA_PUBLIC, NULL},
    {"POSS", handle_pronoun, 1, PRONOUN_POSS, CA_PUBLIC, NULL},
    {"POWER", fun_power, 2, 0, CA_PUBLIC, NULL},
    {"PRIVATE", fun_private, 1, FN_NO_EVAL, CA_PUBLIC, NULL},
    {"PROGRAMMER", fun_programmer, 1, 0, CA_PUBLIC, NULL},
    {"PS", fun_ps, 1, 0, CA_PUBLIC, NULL},
    {"PUSH", fun_push, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    /**
     * - Q -
     */
    {"QSUB", fun_qsub, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"QVARS", fun_qvars, 0, FN_VARARGS, CA_PUBLIC, NULL},
    /**
     * - R -
     */
    {"R", fun_r, 1, 0, CA_PUBLIC, NULL},
    {"RAND", fun_rand, 1, 0, CA_PUBLIC, NULL},
    {"RBORDER", perform_border, 0, FN_VARARGS | JUST_RIGHT, CA_PUBLIC, NULL},
    {"READ", fun_read, 3, FN_VARFX, CA_PUBLIC, NULL},
    {"REGEDIT", perform_regedit, 3, 0, CA_PUBLIC, NULL},
    {"REGEDITALL", perform_regedit, 3, REG_MATCH_ALL, CA_PUBLIC, NULL},
    {"REGEDITALLI", perform_regedit, 3, REG_MATCH_ALL | REG_CASELESS, CA_PUBLIC, NULL},
    {"REGEDITI", perform_regedit, 3, REG_CASELESS, CA_PUBLIC, NULL},
    {"REGRAB", perform_regrab, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"REGRABALL", perform_regrab, 0, FN_VARARGS | REG_MATCH_ALL, CA_PUBLIC, NULL},
    {"REGRABALLI", perform_regrab, 0, FN_VARARGS | REG_MATCH_ALL | REG_CASELESS, CA_PUBLIC, NULL},
    {"REGRABI", perform_regrab, 0, FN_VARARGS | REG_CASELESS, CA_PUBLIC, NULL},
    {"REGREP", perform_grep, 0, FN_VARARGS | GREP_REGEXP, CA_PUBLIC, NULL},
    {"REGREPI", perform_grep, 0, FN_VARARGS | GREP_REGEXP | REG_CASELESS, CA_PUBLIC, NULL},
    {"REGMATCH", perform_regmatch, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"REGMATCHI", perform_regmatch, 0, FN_VARARGS | REG_CASELESS, CA_PUBLIC, NULL},
    {"REGPARSE", perform_regparse, 3, FN_VARFX, CA_PUBLIC, NULL},
    {"REGPARSEI", perform_regparse, 3, FN_VARFX | REG_CASELESS, CA_PUBLIC, NULL},
    {"REMAINDER", fun_remainder, 2, 0, CA_PUBLIC, NULL},
    {"REMIT", fun_remit, 2, FN_OUTFX, CA_PUBLIC, NULL},
    {"REMOVE", fun_remove, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"REPEAT", fun_repeat, 2, 0, CA_PUBLIC, NULL},
    {"REPLACE", fun_replace, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"REST", fun_rest, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"RESTARTS", fun_restarts, 0, 0, CA_PUBLIC, NULL},
    {"RESTARTTIME", fun_restarttime, 0, 0, CA_PUBLIC, NULL},
    {"REVERSE", fun_reverse, -1, 0, CA_PUBLIC, NULL},
    {"REVWORDS", fun_revwords, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"RIGHT", fun_right, 2, 0, CA_PUBLIC, NULL},
    {"RJUST", fun_rjust, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"RLOC", fun_rloc, 2, 0, CA_PUBLIC, NULL},
    {"ROOM", fun_room, 1, 0, CA_PUBLIC, NULL},
    {"ROUND", fun_round, 2, 0, CA_PUBLIC, NULL},
    {"RTABLES", process_tables, 0, FN_VARARGS | JUST_RIGHT, CA_PUBLIC, NULL},
    /**
     * - S -
     */
    {"S", fun_s, -1, 0, CA_PUBLIC, NULL},
    {"SANDBOX", handle_ucall, 0, FN_VARARGS | UCALL_SANDBOX, CA_PUBLIC, NULL},
    {"SCRAMBLE", fun_scramble, 1, 0, CA_PUBLIC, NULL},
    {"SEARCH", fun_search, -1, 0, CA_PUBLIC, NULL},
    {"SECS", fun_secs, 0, 0, CA_PUBLIC, NULL},
    {"SECURE", fun_secure, -1, 0, CA_PUBLIC, NULL},
    {"SEES", fun_sees, 2, 0, CA_PUBLIC, NULL},
    {"SESSION", fun_session, 1, 0, CA_PUBLIC, NULL},
    {"SET", fun_set, 2, 0, CA_PUBLIC, NULL},
    {"SETDIFF", handle_sets, 0, FN_VARARGS | SET_DIFF, CA_PUBLIC, NULL},
    {"SETINTER", handle_sets, 0, FN_VARARGS | SET_INTERSECT, CA_PUBLIC, NULL},
    {"SETQ", fun_setq, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"SETR", fun_setr, 2, 0, CA_PUBLIC, NULL},
    {"SETX", fun_setx, 2, FN_VARFX, CA_PUBLIC, NULL},
    {"SETUNION", handle_sets, 0, FN_VARARGS | SET_UNION, CA_PUBLIC, NULL},
    {"SHL", fun_shl, 2, 0, CA_PUBLIC, NULL},
    {"SHR", fun_shr, 2, 0, CA_PUBLIC, NULL},
    {"SHUFFLE", fun_shuffle, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"SIGN", fun_sign, 1, 0, CA_PUBLIC, NULL},
    {"SIN", handle_trig, 1, 0, CA_PUBLIC, NULL},
    {"SIND", handle_trig, 1, TRIG_DEG, CA_PUBLIC, NULL},
    {"SORT", handle_sort, 0, FN_VARARGS | SORT_ITEMS, CA_PUBLIC, NULL},
    {"SORTBY", fun_sortby, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"SPACE", fun_space, 1, 0, CA_PUBLIC, NULL},
    {"SPEAK", fun_speak, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"SPLICE", fun_splice, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"SQRT", fun_sqrt, 1, 0, CA_PUBLIC, NULL},
    {"SQUISH", fun_squish, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"STARTTIME", fun_starttime, 0, 0, CA_PUBLIC, NULL},
    {"STATS", fun_stats, 1, 0, CA_PUBLIC, NULL},
    {"STEP", fun_step, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"STORE", fun_store, 2, FN_VARFX, CA_PUBLIC, NULL},
    {"STRCAT", fun_strcat, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"STREQ", fun_streq, 2, 0, CA_PUBLIC, NULL},
    {"STRIPANSI", fun_stripansi, 1, 0, CA_PUBLIC, NULL},
    {"STRIPCHARS", fun_stripchars, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"STRLEN", fun_strlen, -1, 0, CA_PUBLIC, NULL},
    {"STRMATCH", fun_strmatch, 2, 0, CA_PUBLIC, NULL},
    {"STRTRUNC", fun_left, 2, 0, CA_PUBLIC, NULL},
    {"STRUCTURE", fun_structure, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    {"SUB", fun_sub, 2, 0, CA_PUBLIC, NULL},
    {"SUBEVAL", fun_subeval, 1, 0, CA_PUBLIC, NULL},
    {"SUBJ", handle_pronoun, 1, PRONOUN_SUBJ, CA_PUBLIC, NULL},
    {"SWAP", fun_swap, 0, FN_VARARGS | FN_STACKFX, CA_PUBLIC, NULL},
    {"SWITCH", fun_switch, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"SWITCHALL", fun_switchall, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    /**
     * - T -
     */
    {"T", fun_t, 1, 0, CA_PUBLIC, NULL},
    {"TABLE", fun_table, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"TABLES", process_tables, 0, FN_VARARGS | JUST_LEFT, CA_PUBLIC, NULL},
    {"TAN", handle_trig, 1, TRIG_TAN, CA_PUBLIC, NULL},
    {"TAND", handle_trig, 1, TRIG_TAN | TRIG_DEG, CA_PUBLIC, NULL},
    {"TEL", fun_tel, 2, 0, CA_PUBLIC, NULL},
    {"TIME", fun_time, 0, 0, CA_PUBLIC, NULL},
    {"TIMEFMT", fun_timefmt, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"TOKENS", fun_tokens, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"TOSS", handle_pop, 0, FN_VARARGS | FN_STACKFX | POP_TOSS, CA_PUBLIC, NULL},
    {"TRANSLATE", fun_translate, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"TRIGGER", fun_trigger, 0, FN_VARARGS | FN_QFX, CA_PUBLIC, NULL},
    {"TRIM", fun_trim, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"TRUNC", fun_trunc, 1, 0, CA_PUBLIC, NULL},
    {"TYPE", fun_type, 1, 0, CA_PUBLIC, NULL},
    /**
     * - U -
     */
    {"U", do_ufun, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"UCALL", handle_ucall, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"UCSTR", fun_ucstr, -1, 0, CA_PUBLIC, NULL},
    {"UDEFAULT", fun_udefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"ULOCAL", do_ufun, 0, FN_VARARGS | U_LOCAL, CA_PUBLIC, NULL},
    {"UNLOAD", fun_unload, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    {"UNMATCHALL", fun_matchall, 0, FN_VARARGS | IFELSE_FALSE, CA_PUBLIC, NULL},
    {"UNSTRUCTURE", fun_unstructure, 1, FN_VARFX, CA_PUBLIC, NULL},
    {"UNTIL", fun_until, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"UPRIVATE", do_ufun, 0, FN_VARARGS | U_PRIVATE, CA_PUBLIC, NULL},
    {"URL_ESCAPE", fun_url_escape, -1, 0, CA_PUBLIC, NULL},
    {"URL_UNESCAPE", fun_url_unescape, -1, 0, CA_PUBLIC, NULL},
    {"USETRUE", handle_ifelse, 0, IFELSE_DEFAULT | IFELSE_BOOL | FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    {"USEFALSE", handle_ifelse, 0, IFELSE_FALSE | IFELSE_DEFAULT | IFELSE_BOOL | FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL},
    /**
     * - V -
     */
    {"V", fun_v, 1, 0, CA_PUBLIC, NULL},
    {"VADD", handle_vectors, 0, FN_VARARGS | VEC_ADD, CA_PUBLIC, NULL},
    {"VAND", handle_vectors, 0, FN_VARARGS | VEC_AND, CA_PUBLIC, NULL},
    {"VALID", fun_valid, 2, FN_VARARGS, CA_PUBLIC, NULL},
    {"VDIM", fun_words, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"VDOT", handle_vectors, 0, FN_VARARGS | VEC_DOT, CA_PUBLIC, NULL},
    {"VERSION", fun_version, 0, 0, CA_PUBLIC, NULL},
    {"VISIBLE", fun_visible, 2, 0, CA_PUBLIC, NULL},
    {"VMAG", handle_vector, 0, FN_VARARGS | VEC_MAG, CA_PUBLIC, NULL},
    {"VMUL", handle_vectors, 0, FN_VARARGS | VEC_MUL, CA_PUBLIC, NULL},
    {"VOR", handle_vectors, 0, FN_VARARGS | VEC_OR, CA_PUBLIC, NULL},
    {"VSUB", handle_vectors, 0, FN_VARARGS | VEC_SUB, CA_PUBLIC, NULL},
    {"VUNIT", handle_vector, 0, FN_VARARGS | VEC_UNIT, CA_PUBLIC, NULL},
    {"VXOR", handle_vectors, 0, FN_VARARGS | VEC_XOR, CA_PUBLIC, NULL},
    /**
     * - W -
     */
    {"WAIT", fun_wait, 2, FN_QFX, CA_PUBLIC, NULL},
    {"WHENFALSE", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_FALSE | FILT_COND_NONE, CA_PUBLIC, NULL},
    {"WHENTRUE", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_TRUE | FILT_COND_NONE, CA_PUBLIC, NULL},
    {"WHENFALSE2", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_FALSE | FILT_COND_NONE | LOOP_TWOLISTS, CA_PUBLIC, NULL},
    {"WHENTRUE2", perform_iter, 0, FN_VARARGS | FN_NO_EVAL | BOOL_COND_TRUE | FILT_COND_NONE | LOOP_TWOLISTS, CA_PUBLIC, NULL},
    {"WHERE", handle_loc, 1, LOCFN_WHERE, CA_PUBLIC, NULL},
    {"WHILE", fun_while, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"WILDGREP", perform_grep, 0, FN_VARARGS | GREP_WILD, CA_PUBLIC, NULL},
    {"WILDMATCH", fun_wildmatch, 3, 0, CA_PUBLIC, NULL},
    {"WILDPARSE", fun_wildparse, 3, FN_VARFX, CA_PUBLIC, NULL},
    {"WIPE", fun_wipe, 1, FN_DBFX, CA_PUBLIC, NULL},
    {"WORDPOS", fun_wordpos, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"WORDS", fun_words, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"WRITABLE", fun_writable, 2, 0, CA_PUBLIC, NULL},
    {"WRITE", fun_write, 2, FN_VARFX, CA_PUBLIC, NULL},
    /**
     * - X -
     */
    {"X", fun_x, 1, FN_VARFX, CA_PUBLIC, NULL},
    {"XCON", fun_xcon, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"XGET", perform_get, 2, GET_XARGS, CA_PUBLIC, NULL},
    {"XOR", handle_logic, 0, FN_VARARGS | LOGIC_XOR, CA_PUBLIC, NULL},
    {"XORBOOL", handle_logic, 0, FN_VARARGS | LOGIC_XOR | LOGIC_BOOL, CA_PUBLIC, NULL},
    {"XVARS", fun_xvars, 0, FN_VARARGS | FN_VARFX, CA_PUBLIC, NULL},
    /**
     * - Z -
     */
    {"Z", fun_z, 2, FN_VARFX, CA_PUBLIC, NULL},
    {"ZFUN", fun_zfun, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"ZONE", fun_zone, 1, 0, CA_PUBLIC, NULL},
    {"ZWHO", scan_zone, 1, TYPE_PLAYER, CA_PUBLIC, NULL},
    /**
     * - END -
     */
    {NULL, NULL, 0, 0, 0, NULL}};
