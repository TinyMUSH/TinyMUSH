/**
 * @file funvars_struct_ops.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Structure operation functions (modify, unload, destruct)
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
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[0], tbuf, &tp);
    *tp = '\0';
    endp = tp; /* save where we are */
    inst_ptr = (INSTANCE *)hashfind(tbuf, &mushstate.instance_htab);

    if (!inst_ptr)
    {
        notify_quiet(player, "No such instance.");
        XSAFELBCHR('0', buff, bufc);
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
            XSAFELTOS(cbuf, &cp, player, LBUF_SIZE);
            XSAFESBCHR('.', cbuf, &cp);
            XSAFESBSTR(inst_ptr->datatype->s_name, cbuf, &cp);
            XSAFESBCHR('.', cbuf, &cp);

            for (p = words[i]; *p; p++)
            {
                *p = tolower(*p);
            }

            XSAFESBSTR(words[i], cbuf, &cp);
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
        XSAFESBCHR('.', tbuf, &tp);
        XSAFESBSTR(words[i], tbuf, &tp);
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
    XSAFELTOS(buff, bufc, n_mod, LBUF_SIZE);
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
    XSAFELTOS(ibuf, &ip, player, LBUF_SIZE);
    XSAFESBCHR('.', ibuf, &ip);

    for (p = inst_name; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(inst_name, ibuf, &ip);
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
    XSAFESBCHR('.', ibuf, &ip);
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
            XSAFELBCHR(sep, buff, bufc);
        }

        tp = tbuf;
        XSAFESBSTR(ibuf, tbuf, &tp);
        XSAFESBSTR(this_struct->c_names[i], tbuf, &tp);
        *tp = '\0';
        d_ptr = (STRUCTDATA *)hashfind(tbuf, &mushstate.instdata_htab);

        if (d_ptr && d_ptr->text)
        {
            XSAFELBSTR(d_ptr->text, buff, bufc);
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

void fun_write(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, aowner;
    int atrnum, aflags;
    char tbuf[LBUF_SIZE], *tp, *str;
    ATTR *attr;

    if (!parse_thing_slash(player, fargs[0], &str, &it))
    {
        XSAFENOMATCH(buff, bufc);
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
            XSAFELBSTR("#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc);
            return;
        }

        attr = atr_num(atrnum);
        atr_pget_info(it, atrnum, &aowner, &aflags);

        if (!attr || !Set_attr(player, it, attr, aflags) || (attr->check != NULL))
        {
            XSAFENOPERM(buff, bufc);
        }
        else
        {
            atr_add(it, atrnum, tbuf, Owner(player), aflags | AF_STRUCTURE);
        }
    }
}

void fun_destruct(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
    XSAFELTOS(ibuf, &ip, player, LBUF_SIZE);
    XSAFESBCHR('.', ibuf, &ip);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[0], ibuf, &ip);
    *ip = '\0';
    inst_ptr = (INSTANCE *)hashfind(ibuf, &mushstate.instance_htab);

    if (!inst_ptr)
    {
        notify_quiet(player, "No such instance.");
        XSAFELBCHR('0', buff, bufc);
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
    XSAFESBCHR('.', ibuf, &ip);
    *ip = '\0';

    for (i = 0; i < this_struct->c_count; i++)
    {
        tp = tbuf;
        XSAFESBSTR(ibuf, tbuf, &tp);
        XSAFESBSTR(this_struct->c_names[i], tbuf, &tp);
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
    XSAFELBCHR('1', buff, bufc);
    XFREE(ibuf);
    XFREE(tbuf);
}

void fun_unstructure(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
    XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);

    for (p = fargs[0]; *p; p++)
    {
        *p = tolower(*p);
    }

    XSAFESBSTR(fargs[0], tbuf, &tp);
    *tp = '\0';
    this_struct = (STRUCTDEF *)hashfind(tbuf, &mushstate.structs_htab);

    if (!this_struct)
    {
        notify_quiet(player, "No such structure.");
        XSAFELBCHR('0', buff, bufc);
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
        XSAFELBCHR('0', buff, bufc);
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
    XSAFESBCHR('.', tbuf, &tp);
    *tp = '\0';

    for (i = 0; i < this_struct->c_count; i++)
    {
        cp = cbuf;
        XSAFESBSTR(tbuf, cbuf, &cp);
        XSAFESBSTR(this_struct->c_names[i], cbuf, &cp);
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
    XSAFELBCHR('1', buff, bufc);
    XFREE(cbuf);
    XFREE(tbuf);
}

void fun_lstructures(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    print_htab_matches(player, &mushstate.structs_htab, buff, bufc);
}

void fun_linstances(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
    XSAFELTOS(tbuf, &tp, thing, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);
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
            XSAFESBSTR(name_array[i], ibuf, &ip);
            XSAFESBCHR('.', ibuf, &ip);
            *ip = '\0';

            for (j = 0; j < this_struct->c_count; j++)
            {
                cp = cbuf;
                XSAFESBSTR(ibuf, cbuf, &cp);
                XSAFESBSTR(this_struct->c_names[j], cbuf, &cp);
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
    XSAFELTOS(tbuf, &tp, thing, LBUF_SIZE);
    XSAFESBCHR('.', tbuf, &tp);
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
            XSAFESBSTR(name_array[i], ibuf, &ip);
            XSAFESBCHR('.', ibuf, &ip);
            *ip = '\0';

            for (j = 0; j < struct_array[i]->c_count; j++)
            {
                cp = cbuf;
                XSAFESBSTR(ibuf, cbuf, &cp);
                XSAFESBSTR(struct_array[i]->c_names[j], cbuf, &cp);
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

