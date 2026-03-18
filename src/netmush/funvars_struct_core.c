/**
 * @file funvars_struct_core.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Structure definition and construction functions
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
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(tbuf);
        return;
    }

    /*
     * The hashtable is indexed by <dbref number>.<structure name>
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

    /*
     * If we have this structure already, reject.
     */

    if (hashfind(tbuf, &mushstate.structs_htab))
    {
        notify_quiet(player, "Structure is already defined.");
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
            XSAFELBCHR('0', buff, bufc);
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
            XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
    XSAFESBCHR('.', tbuf, &tp);
    *tp = '\0';

    /*
     * Allocate each individual component.
     */

    for (i = 0; i < n_comps; i++)
    {
        cp = cbuf;
        XSAFESBSTR(tbuf, cbuf, &cp);

        for (p = comp_array[i]; *p; p++)
        {
            *p = tolower(*p);
        }

        XSAFESBSTR(comp_array[i], cbuf, &cp);
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
    XSAFELBCHR('1', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Make sure this instance doesn't exist.
     */
    ip = ibuf;
    XSAFELTOS(ibuf, &ip, player, LBUF_SIZE);
    XSAFESBCHR('.', ibuf, &ip);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[0], ibuf, &ip);
    *ip = '\0';

    if (hashfind(ibuf, &mushstate.instance_htab))
    {
        notify_quiet(player, "That instance has already been defined.");
        XSAFELBCHR('0', buff, bufc);
        XFREE(cbuf);
        XFREE(ibuf);
        XFREE(tbuf);
        return;
    }

    /*
     * Look up the structure.
     */
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = fargs[1]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[1], tbuf, &tp);
    *tp = '\0';
    this_struct = (STRUCTDEF *)hashfind(tbuf, &mushstate.structs_htab);

    if (!this_struct)
    {
        notify_quiet(player, "No such structure.");
        XSAFELBCHR('0', buff, bufc);
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
    XSAFESBCHR('.', tbuf, &tp);
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
            XSAFELBCHR('0', buff, bufc);
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
            XSAFESBSTR(tbuf, cbuf, &cp);

            for (p = comp_array[i]; *p; p++)
            {
                *p = tolower(*p);
            }

            XSAFESBSTR(comp_array[i], cbuf, &cp);
            c_ptr = (COMPONENT *)hashfind(cbuf, &mushstate.cdefs_htab);

            if (!c_ptr)
            {
                notify_quiet(player, "Invalid component name.");
                XSAFELBCHR('0', buff, bufc);
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
                    XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
        XSAFESBSTR(ibuf, tbuf, &tp);
        XSAFESBCHR('.', tbuf, &tp);
        XSAFESBSTR(this_struct->c_names[i], tbuf, &tp);
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
        XSAFESBSTR(ibuf, tbuf, &tp);
        XSAFESBCHR('.', tbuf, &tp);
        XSAFESBSTR(comp_array[i], tbuf, &tp);
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
    XSAFELBCHR('1', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);

        return;
    }

    /*
     * Make sure this instance doesn't exist.
     */
    ip = ibuf;
    XSAFELTOS(ibuf, &ip, player, LBUF_SIZE);
    XSAFESBCHR('.', ibuf, &ip);

    for (p = inst_name; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(inst_name, ibuf, &ip);
    *ip = '\0';

    if (hashfind(ibuf, &mushstate.instance_htab))
    {
        notify_quiet(player, "That instance has already been defined.");
        XSAFELBCHR('0', buff, bufc);
        XFREE(ibuf);
        XFREE(tbuf);

        return;
    }

    /*
     * Look up the structure.
     */
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = str_name; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(str_name, tbuf, &tp);
    *tp = '\0';
    this_struct = (STRUCTDEF *)hashfind(tbuf, &mushstate.structs_htab);

    if (!this_struct)
    {
        notify_quiet(player, "No such structure.");
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
            XSAFELBCHR('0', buff, bufc);
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
        XSAFESBSTR(ibuf, tbuf, &tp);
        XSAFESBCHR('.', tbuf, &tp);
        XSAFESBSTR(this_struct->c_names[i], tbuf, &tp);
        *tp = '\0';
        hashadd(tbuf, (int *)d_ptr, &mushstate.instdata_htab, 0);
        mushstate.max_instdata = mushstate.instdata_htab.entries > mushstate.max_instdata ? mushstate.instdata_htab.entries : mushstate.max_instdata;
    }

    XFREE(val_list);
    XFREE(val_array);
    this_struct->n_instances += 1;
    s_InstanceCount(player, InstanceCount(player) + 1);
    XSAFELBCHR('1', buff, bufc);
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

void fun_read(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, aowner;
    int atr, aflags, alen;
    char *atext;

    if (!parse_attrib(player, fargs[0], &it, &atr, 1) || (atr == NOTHING))
    {
        XSAFELBCHR('0', buff, bufc);
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
        XSAFENOPERM(buff, bufc);
        return;
    }

    atext = atr_pget(it, atr, &aowner, &aflags, &alen);
    nitems = list2arr(&ptrs, LBUF_SIZE / 2, atext, &isep);

    if (nitems)
    {
        over = XSAFELBSTR(ptrs[0], buff, bufc);
    }

    for (i = 1; !over && (i < nitems); i++)
    {
        over = XSAFELBSTR(fargs[1], buff, bufc);

        if (!over)
        {
            over = XSAFELBSTR(ptrs[i], buff, bufc);
        }
    }

    XFREE(atext);
    XFREE(ptrs);
}

void fun_z(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
    char *p, *tp;
    STRUCTDATA *s_ptr;
    tp = tbuf;
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[0], tbuf, &tp);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = fargs[1]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[1], tbuf, &tp);
    *tp = '\0';
    s_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

    if (!s_ptr || !s_ptr->text)
    {
        XFREE(tbuf);
        return;
    }

    XSAFELBSTR(s_ptr->text, buff, bufc);
    XFREE(tbuf);
}

