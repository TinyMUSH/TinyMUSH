/* player_c.c - Player cache routines */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mudconf */
#include "game.h"      /* required by mudconf */
#include "alloc.h"     /* required by mudconf */
#include "flags.h"     /* required by mudconf */
#include "htab.h"      /* required by mudconf */
#include "ltdl.h"      /* required by mudconf */
#include "udb.h"       /* required by mudconf */
#include "udb_defs.h"  /* required by mudconf */
#include "mushconf.h"  /* required by code */
#include "db.h"        /* required by externs */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by code */
#include "attrs.h"     /* required by code */
#include "player_c.h"  /* required by code */

NHSHTAB pcache_htab;

PCACHE *pcache_head;

#define PF_DEAD 0x0001
#define PF_REF 0x0002
#define PF_MONEY_CH 0x0004
#define PF_QMAX_CH 0x0008

void pcache_init(void)
{
    nhashinit(&pcache_htab, 15 * mudconf.hash_factor);
    pcache_head = NULL;
}

void pcache_reload1(dbref player, PCACHE *pp)
{
    char *cp;
    cp = atr_get_raw(player, A_MONEY);

    if (cp && *cp)
    {
        pp->money = (int)strtol(cp, (char **)NULL, 10);
    }
    else
    {
        pp->money = 0;
    }

    cp = atr_get_raw(player, A_QUEUEMAX);

    if (cp && *cp)
    {
        pp->qmax = (int)strtol(cp, (char **)NULL, 10);
    }
    else if (!Wizard(player))
    {
        pp->qmax = mudconf.queuemax;
    }
    else
    {
        pp->qmax = -1;
    }
}

PCACHE *pcache_find(dbref player)
{
    PCACHE *pp;
    pp = (PCACHE *)nhashfind(player, &pcache_htab);

    if (pp)
    {
        pp->cflags |= PF_REF;
        return pp;
    }

    pp = XMALLOC(sizeof(PCACHE), "pp");
    pp->queue = 0;
    pp->cflags = PF_REF;
    pp->player = player;
    pcache_reload1(player, pp);
    pp->next = pcache_head;
    pcache_head = pp;
    nhashadd(player, (int *)pp, &pcache_htab);
    return pp;
}

void pcache_reload(dbref player)
{
    PCACHE *pp;

    if (Good_owner(player))
    {
        pp = pcache_find(player);
        pcache_reload1(player, pp);
    }
}

void pcache_save(PCACHE *pp)
{
    char *tbuf;

    if (pp->cflags & PF_DEAD)
    {
        return;
    }

    if (pp->cflags & PF_MONEY_CH)
    {
        tbuf = ltos(pp->money);
        atr_add_raw(pp->player, A_MONEY, tbuf);
        XFREE(tbuf);
    }

    if (pp->cflags & PF_QMAX_CH)
    {
        tbuf = XMALLOC(SBUF_SIZE, "tbuf");
        sprintf(tbuf, "%d", pp->qmax);
        atr_add_raw(pp->player, A_QUEUEMAX, tbuf);
        XFREE(tbuf);
    }

    pp->cflags &= ~(PF_MONEY_CH | PF_QMAX_CH);
}

void pcache_trim(void)
{
    PCACHE *pp, *pplast, *ppnext;
    pp = pcache_head;
    pplast = NULL;

    while (pp)
    {
        if (!(pp->cflags & PF_DEAD) && (pp->queue || (pp->cflags & PF_REF)))
        {
            pp->cflags &= ~PF_REF;
            pplast = pp;
            pp = pp->next;
        }
        else
        {
            ppnext = pp->next;

            if (pplast)
            {
                pplast->next = ppnext;
            }
            else
            {
                pcache_head = ppnext;
            }

            if (!(pp->cflags & PF_DEAD))
            {
                pcache_save(pp);
                nhashdelete(pp->player, &pcache_htab);
            }

            XFREE(pp);
            pp = ppnext;
        }
    }
}

void pcache_sync(void)
{
    PCACHE *pp;
    pp = pcache_head;

    while (pp)
    {
        pcache_save(pp);
        pp = pp->next;
    }
}

int a_Queue(dbref player, int adj)
{
    PCACHE *pp;

    if (Good_owner(player))
    {
        pp = pcache_find(player);
        pp->queue += adj;
        return pp->queue;
    }
    else
    {
        return 0;
    }
}

void s_Queue(dbref player, int val)
{
    PCACHE *pp;

    if (Good_owner(player))
    {
        pp = pcache_find(player);
        pp->queue = val;
    }
}

int QueueMax(dbref player)
{
    PCACHE *pp;
    int m;
    m = 0;

    if (Good_owner(player))
    {
        pp = pcache_find(player);

        if (pp->qmax >= 0)
        {
            m = pp->qmax;
        }
        else
        {
            m = mudstate.db_top + 1;

            if (m < mudconf.queuemax)
            {
                m = mudconf.queuemax;
            }
        }
    }

    return m;
}

int Pennies(dbref obj)
{
    PCACHE *pp;
    char *cp;

    if (!mudstate.standalone && Good_owner(obj))
    {
        pp = pcache_find(obj);
        return pp->money;
    }

    cp = atr_get_raw(obj, A_MONEY);
    return ((int)strtol(cp, (char **)NULL, 10));
}

void s_Pennies(dbref obj, int howfew)
{
    PCACHE *pp;
    char *tbuf;

    if (!mudstate.standalone && Good_owner(obj))
    {
        pp = pcache_find(obj);
        pp->money = howfew;
        pp->cflags |= PF_MONEY_CH;
    }

    tbuf = ltos(howfew);
    atr_add_raw(obj, A_MONEY, tbuf);
    XFREE(tbuf);
}
