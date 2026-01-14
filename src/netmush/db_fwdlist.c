/**
 * @file db_fwdlist.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Forward list management for AUDIBLE attribute propagation
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
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

/**
 * Check routine forward declaration.
 *
 */
int fwdlist_ck(int, dbref, dbref, int, char *);

/**
 * @brief Set cached forwarding lists
 *
 * @param thing DBref of thing
 * @param ifp   Forward list
 */
void fwdlist_set(dbref thing, FWDLIST *ifp)
{
    FWDLIST *fp = NULL, *xfp = NULL;
    int stat = 0;

    /**
     * If fwdlist is null, clear
     *
     */
    if (!ifp || (ifp->count <= 0))
    {
        fwdlist_clr(thing);
        return;
    }

    /**
     * Copy input forwardlist to a correctly-sized buffer
     *
     */
    fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
    fp->data = (int *)XCALLOC(ifp->count, sizeof(int), "fp->data");

    for (int i = 0; i < ifp->count; i++)
    {
        fp->data[i] = ifp->data[i];
    }

    fp->count = ifp->count;

    /**
     * Replace an existing forwardlist, or add a new one
     *
     */
    xfp = fwdlist_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        stat = nhashrepl(thing, (int *)fp, &mushstate.fwdlist_htab);
    }
    else
    {
        stat = nhashadd(thing, (int *)fp, &mushstate.fwdlist_htab);
    }

    if (stat < 0)
    {
        /**
         * the add or replace failed
         *
         */
        if (fp->data)
        {
            XFREE(fp->data);
        }

        XFREE(fp);
    }
}

/**
 * @brief Clear cached forwarding lists
 *
 * @param thing DBref of thing
 */
void fwdlist_clr(dbref thing)
{
    FWDLIST *xfp = NULL;
    /**
     * If a forwardlist exists, delete it
     *
     */
    xfp = fwdlist_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        nhashdelete(thing, &mushstate.fwdlist_htab);
    }
}

/**
 * @brief Load text into a forwardlist.
 *
 * @param fp        Forward list
 * @param player    DBref of player
 * @param atext     Text
 * @return int
 */
int fwdlist_load(FWDLIST *fp, dbref player, char *atext)
{
    dbref target = NOTHING;
    int i = 0, count = 0, errors = 0, fail = 0;
    int *tmp_array = (int *)XCALLOC((LBUF_SIZE / 2), sizeof(int), "tmp_array");
    char *tp = XMALLOC(LBUF_SIZE, "tp");
    char *dp = NULL, *bp = tp;

    XSTRCPY(tp, atext);

    do
    {
        for (; *bp && isspace(*bp); bp++)
        {
            /**
             * skip spaces
             *
             */
        }

        for (dp = bp; *bp && !isspace(*bp); bp++)
        {
            /**
             * remember string
             *
             */
        }

        if (*bp)
        {
            /**
             * terminate string
             *
             */
            *bp++ = '\0';
        }

        if ((*dp++ == '#') && isdigit(*dp))
        {
            char *endptr = NULL;
            long val = 0;

            errno = 0;
            val = strtol(dp, &endptr, 10);

            if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == dp || *endptr != '\0')
            {
                target = NOTHING;
                fail = 1;
            }
            else
            {
                target = (int)val;
            }

            if (!mushstate.standalone)
            {
                fail = (!Good_obj(target) || (!God(player) && !controls(player, target) && (!Link_ok(target) || !could_doit(player, target, A_LLINK))));
            }
            else
            {
                fail = !Good_obj(target);
            }

            if (fail)
            {
                if (!mushstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot forward to #%d: Permission denied.", target);
                }

                errors++;
            }
            else if (count < mushconf.fwdlist_lim)
            {
                if (fp->data)
                {
                    fp->data[count++] = target;
                }
                else
                {
                    tmp_array[count++] = target;
                }
            }
            else
            {
                if (!mushstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot forward to #%d: Forwardlist limit exceeded.", target);
                }

                errors++;
            }
        }
    } while (*bp);

    XFREE(tp);

    if ((fp->data == NULL) && (count > 0))
    {
        fp->data = (int *)XCALLOC(count, sizeof(int), "fp->data");

        for (i = 0; i < count; i++)
        {
            fp->data[i] = tmp_array[i];
        }
    }

    fp->count = count;
    XFREE(tmp_array);
    return errors;
}

/**
 * @brief Generate a text string from a FWDLIST buffer.
 *
 * @param fp    Forward list
 * @param atext Text buffer
 * @return int
 */
int fwdlist_rewrite(FWDLIST *fp, char *atext)
{
    char *tp = NULL, *bp = NULL;
    int count = 0;

    if (fp && fp->count)
    {
        count = fp->count;
        tp = XMALLOC(SBUF_SIZE, "tp");
        bp = atext;

        for (int i = 0; i < fp->count; i++)
        {
            if (Good_obj(fp->data[i]))
            {
                XSPRINTF(tp, "#%d ", fp->data[i]);
                XSAFELBSTR(tp, atext, &bp);
            }
            else
            {
                count--;
            }
        }

        *bp = '\0';
        XFREE(tp);
    }
    else
    {
        count = 0;
        *atext = '\0';
    }

    return count;
}

/**
 * @brief Check a list of dbref numbers to forward to for AUDIBLE
 *
 * @param key       Key
 * @param player    DBref of player
 * @param thing     DBref of thing
 * @param anum      Attribute
 * @param atext     Attribute text
 * @return int
 */
int fwdlist_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
    FWDLIST *fp = NULL;
    int count = 0;

    if (mushstate.standalone)
    {
        return 1;
    }

    count = 0;

    if (atext && *atext)
    {
        fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
        fp->data = NULL;
        fwdlist_load(fp, player, atext);
    }
    else
    {
        fp = NULL;
    }

    /**
     * Set the cached forwardlist
     *
     */
    fwdlist_set(thing, fp);
    count = fwdlist_rewrite(fp, atext);

    if (fp)
    {
        if (fp->data)
        {
            XFREE(fp->data);
        }

        XFREE(fp);
    }

    return ((count > 0) || !atext || !*atext);
}

/**
 * @brief Fetch a forward list
 *
 * @param thing     DBref of thing
 * @return FWDLIST*
 */
FWDLIST *fwdlist_get(dbref thing)
{
    dbref aowner = NOTHING;
    FWDLIST *fp = NULL;
    int aflags = 0, alen = 0;
    char *tp = NULL;

    if (!mushstate.standalone)
    {
        return (FWDLIST *)nhashfind((thing), &mushstate.fwdlist_htab);
    }

    if (!fp)
    {
        fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
        fp->data = NULL;
    }

    tp = atr_get(thing, A_FORWARDLIST, &aowner, &aflags, &alen);
    fwdlist_load(fp, GOD, tp);
    XFREE(tp);
    return fp;
}
