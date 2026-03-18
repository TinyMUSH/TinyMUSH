/**
 * @file funvars_stacks.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object stack functions
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

void fun_empty(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

void fun_items(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

    XSAFELTOS(buff, bufc, StackCount(it), LBUF_SIZE);
}

void fun_push(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

void fun_dup(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

void fun_swap(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

void handle_pop(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
        XSAFELBSTR(sp->data, buff, bufc);
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

            over = XSAFELBSTR(tp->data, buff, bufc);
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

        over = XSAFELBSTR(sp->data, buff, bufc);
    }
}

/*
 * --------------------------------------------------------------------------
 * regedit: Edit a string for sed/perl-like s//
 * regedit(<string>,<regexp>,<replacement>) Derived from the PennMUSH code.
 */

