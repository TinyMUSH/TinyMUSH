/**
 * @file vattr.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Manages the user-defined attributes.
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
 * Current block we're putting stuff in
 */

char *stringblock = (char *)0;

/*
 * High water mark.
 */

int stringblock_hwm = 0;

void vattr_init(void)
{
    hashinit(&mudstate.vattr_name_htab, VATTR_HASH_SIZE, HT_STR | HT_KEYREF);
}

VATTR *vattr_find(char *name)
{
    return (VATTR *)hashfind(name, &mudstate.vattr_name_htab);
}

VATTR *vattr_alloc(char *name, int flags)
{
    int number;

    if (((number = mudstate.attr_next++) & 0x7f) == 0)
    {
        number = mudstate.attr_next++;
    }

    anum_extend(number);
    flags |= AF_DIRTY;
    return (vattr_define(name, number, flags));
}

VATTR *vattr_define(char *name, int number, int flags)
{
    VATTR *vp;

    /*
     * Be ruthless.
     */

    if (strlen(name) >= VNAME_SIZE)
    {
        name[VNAME_SIZE - 1] = '\0';
    }

    fixcase(name);

    if (!ok_attr_name(name))
    {
        return (NULL);
    }

    if ((vp = vattr_find(name)) != NULL)
    {
        return (vp);
    }

    vp = (VATTR *)XMALLOC(sizeof(VATTR), "vp");
    vp->name = store_string(name);
    vp->flags = flags;
    vp->number = number;
    hashadd(vp->name, (int *)vp, &mudstate.vattr_name_htab, 0);
    anum_extend(vp->number);
    anum_set(vp->number, (ATTR *)vp);
    return (vp);
}

#ifdef NEVER
void do_dbclean(dbref player, dbref cause, int key)
{
    VATTR *vp, *vpx;
    dbref i, end;
    int ca, n_oldtotal, n_oldtop, n_deleted, n_renumbered, n_objt, n_atrt, got;
    char *as, *str;
    int *used_table;
    ATTR **new_table;
    UFUN *ufp;
    CMDENT *cmdp;
    ADDENT *addp;
    raw_broadcast(0, "GAME: Cleaning database. Game may freeze for a few minutes.");
    used_table = (int *)XCALLOC(mudstate.attr_next, sizeof(int), "used_table");
    n_oldtotal = mudstate.attr_next;
    n_oldtop = anum_alc_top;
    n_deleted = n_renumbered = n_objt = n_atrt = 0;

    /*
     * Non-user-defined attributes are always considered used.
     */

    for (i = 0; i < A_USER_START; i++)
    {
        used_table[i] = i;
    }

    /*
     * Walk the database. Mark all the attribute numbers in use.
     */
    atr_push();
    for (i = 0; i < mudstate.db_top; i++)
    {
        for (ca = atr_head(i, &as); ca; ca = atr_next(&as))
        {
            used_table[ca] = ca;
        }
    }
    atr_pop();
    /*
     * Walk the vattr table. If a number isn't in use, zorch it.
     */
    vp = vattr_first();

    while (vp)
    {
        vpx = vp;
        vp = vattr_next(vp);

        if (used_table[vpx->number] == 0)
        {
            anum_set(vpx->number, NULL);
            hashdelete(vpx->name, &mudstate.vattr_name_htab);
            XFREE(vpx);
            n_deleted++;
        }
    }

    /*
     * The user-defined function, added command, and hook structures embed
     * * attribute numbers. Clean out the ones we've deleted, resetting them
     * * to the *Invalid (A_TEMP) attr.
     */

    for (ufp = (UFUN *)hash_firstentry(&mudstate.ufunc_htab); ufp != NULL; ufp = (UFUN *)hash_nextentry(&mudstate.ufunc_htab))
    {
        if (used_table[ufp->atr] == 0)
        {
            ufp->atr = A_TEMP;
        }
    }

    for (cmdp = (CMDENT *)hash_firstentry(&mudstate.command_htab); cmdp != NULL; cmdp = (CMDENT *)hash_nextentry(&mudstate.command_htab))
    {
        if (cmdp->pre_hook)
        {
            if (used_table[cmdp->pre_hook->atr] == 0)
            {
                cmdp->pre_hook->atr = A_TEMP;
            }
        }

        if (cmdp->post_hook)
        {
            if (used_table[cmdp->post_hook->atr] == 0)
            {
                cmdp->post_hook->atr = A_TEMP;
            }
        }

        if (cmdp->userperms)
        {
            if (used_table[cmdp->userperms->atr] == 0)
            {
                cmdp->userperms->atr = A_TEMP;
            }
        }

        if (cmdp->callseq & CS_ADDED)
        {
            for (addp = (ADDENT *)cmdp->info.added; addp != NULL; addp = addp->next)
            {
                if (used_table[addp->atr] == 0)
                {
                    addp->atr = A_TEMP;
                }
            }
        }
    }

    /*
     * Walk the table we've created of used statuses. When we find free
     * * slots, walk backwards to the first used slot at the end of the
     * * table. Write the number of the free slot into that used slot.
     */

    for (i = A_USER_START, end = mudstate.attr_next - 1; (i < mudstate.attr_next) && (i < end); i++)
    {
        if (used_table[i] == 0)
        {
            while ((end > i) && (used_table[end] == 0))
            {
                end--;
            }

            if (end > i)
            {
                used_table[end] = used_table[i] = i;
                end--;
            }
        }
    }

    /*
     * Renumber the necessary attributes in the vattr tables.
     */

    for (i = A_USER_START; i < mudstate.attr_next; i++)
    {
        if (used_table[i] != i)
        {
            vp = (VATTR *)anum_get(i);

            if (vp)
            {
                vp->number = used_table[i];
                vp->flags |= AF_DIRTY;
                anum_set(used_table[i], (ATTR *)vp);
                anum_set(i, NULL);
                n_renumbered++;
            }
        }
    }

    /*
     * Now we walk the database. For every object, if we have an attribute
     * * we're renumbering (the slot number is not equal to the array value
     * * at that slot), we delete the old attribute and add the new one.
     */
    atr_push();
    for (i = 0; i < mudstate.db_top; i++)
    {
        got = 0;

        for (ca = atr_head(i, &as); ca; ca = atr_next(&as))
        {
            if (used_table[ca] != ca)
            {
                str = atr_get_raw(i, ca);
                atr_add_raw(i, used_table[ca], str);
                atr_clr(i, ca);
                n_atrt++;
                got = 1;
            }
        }

        if (got)
        {
            n_objt++;
        }
    }
    atr_pop();

    /*
     * The new end of the attribute table is the first thing we've
     * * renumbered.
     */

    for (end = A_USER_START; ((end == used_table[end]) && (end < mudstate.attr_next)); end++)
        ;

    mudstate.attr_next = end;

    /*
     * We might be able to shrink the size of the attribute table.
     * * If the current size of the table is less than the initial
     * * size, shrink it back down to the initial size.
     * * Otherwise, shrink it down so it's the current top plus the
     * * initial size, as if we'd just called anum_extend() for it.
     */

    if (anum_alc_top > mudconf.init_size + A_USER_START)
    {
        if (mudstate.attr_next < mudconf.init_size + A_USER_START)
        {
            end = mudconf.init_size + A_USER_START;
        }
        else
        {
            end = mudstate.attr_next + mudconf.init_size;
        }

        if (end < anum_alc_top)
        {
            new_table = (ATTR **)XCALLOC(end + 1, sizeof(ATTR *), "dbclean.new_table");

            for (i = 0; i < mudstate.attr_next; i++)
            {
                new_table[i] = anum_table[i];
            }

            XFREE(anum_table);
            anum_table = new_table;
            anum_alc_top = end;
        }
    }

    /*
     * Go through the function and added command tables again, and
     * * take care of the attributes that got renumbered.
     */

    for (ufp = (UFUN *)hash_firstentry(&mudstate.ufunc_htab); ufp != NULL; ufp = (UFUN *)hash_nextentry(&mudstate.ufunc_htab))
    {
        if (used_table[ufp->atr] != ufp->atr)
        {
            ufp->atr = used_table[ufp->atr];
        }
    }

    for (cmdp = (CMDENT *)hash_firstentry(&mudstate.command_htab); cmdp != NULL; cmdp = (CMDENT *)hash_nextentry(&mudstate.command_htab))
    {
        if (cmdp->pre_hook)
        {
            if (used_table[cmdp->pre_hook->atr] != cmdp->pre_hook->atr)
                cmdp->pre_hook->atr = used_table[cmdp->pre_hook->atr];
        }

        if (cmdp->post_hook)
        {
            if (used_table[cmdp->post_hook->atr] != cmdp->post_hook->atr)
                cmdp->post_hook->atr = used_table[cmdp->post_hook->atr];
        }

        if (cmdp->userperms)
        {
            if (used_table[cmdp->userperms->atr] != cmdp->userperms->atr)
                cmdp->userperms->atr = used_table[cmdp->userperms->atr];
        }

        if (cmdp->callseq & CS_ADDED)
        {
            for (addp = (ADDENT *)cmdp->info.added; addp != NULL; addp = addp->next)
            {
                if (used_table[addp->atr] != addp->atr)
                {
                    addp->atr = used_table[addp->atr];
                }
            }
        }
    }

    /*
     * Clean up.
     */
    XFREE(used_table);

    if (anum_alc_top != n_oldtop)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cleaned %d user attribute slots (reduced to %d): %d deleted, %d renumbered (%d objects and %d individual attrs touched). Table size reduced from %d to %d.", n_oldtotal - A_USER_START, mudstate.attr_next - A_USER_START, n_deleted, n_renumbered, n_objt, n_atrt, n_oldtop, anum_alc_top);
    }
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cleaned %d attributes (now %d): %d deleted, %d renumbered (%d objects and %d individual attrs touched).", n_oldtotal, mudstate.attr_next, n_deleted, n_renumbered, n_objt, n_atrt);
    }

    raw_broadcast(0, "GAME: Database cleaning complete.");
}
#endif /* NEVER */

void vattr_delete(char *name)
{
    VATTR *vp;
    int number;
    fixcase(name);

    if (!ok_attr_name(name))
    {
        return;
    }

    number = 0;
    vp = (VATTR *)hashfind(name, &mudstate.vattr_name_htab);

    if (vp)
    {
        number = vp->number;
        anum_set(number, NULL);
        hashdelete(name, &mudstate.vattr_name_htab);
        XFREE(vp);
    }

    return;
}

VATTR *vattr_rename(char *name, char *newname)
{
    VATTR *vp;
    fixcase(name);

    if (!ok_attr_name(name))
    {
        return (NULL);
    }

    /*
     * Be ruthless.
     */

    if (strlen(newname) >= VNAME_SIZE)
    {
        newname[VNAME_SIZE - 1] = '\0';
    }

    fixcase(newname);

    if (!ok_attr_name(newname))
    {
        return (NULL);
    }

    /*
     * We must explicitly delete and add the name to the hashtable,
     * * since we are changing the data.
     */
    vp = (VATTR *)hashfind(name, &mudstate.vattr_name_htab);

    if (vp)
    {
        vp->name = store_string(newname);
        hashdelete(name, &mudstate.vattr_name_htab);
        hashadd(newname, (int *)vp, &mudstate.vattr_name_htab, 0);
    }

    return (vp);
}

VATTR *vattr_first(void)
{
    return (VATTR *)hash_firstentry(&mudstate.vattr_name_htab);
}

VATTR *vattr_next(VATTR *vp)
{
    if (vp == NULL)
    {
        return (vattr_first());
    }

    return ((VATTR *)hash_nextentry(&mudstate.vattr_name_htab));
}

void fixcase(char *name)
{
    char *cp = name;

    while (*cp)
    {
        *cp = toupper(*cp);
        cp++;
    }

    return;
}

/*
 * Some goop for efficiently storing strings we expect to
 * keep forever. There is no freeing mechanism.
 */

char *store_string(char *str)
{
    int len;
    char *ret;
    len = strlen(str);

    /*
     * If we have no block, or there's not enough room left in the
     * current one, get a new one.
     */

    if (!stringblock || (STRINGBLOCK - stringblock_hwm) < (len + 1))
    {
        stringblock = (char *)XMALLOC(STRINGBLOCK, "stringblock");

        if (!stringblock)
        {
            return ((char *)0);
        }

        stringblock_hwm = 0;
    }

    ret = stringblock + stringblock_hwm;
    XSTRCPY(ret, str);
    stringblock_hwm += (len + 1);
    return (ret);
}
