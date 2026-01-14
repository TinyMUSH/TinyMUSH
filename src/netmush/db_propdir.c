/**
 * @file db_propdir.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Property directory management for multi-parent inheritance
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
int propdir_ck(int, dbref, dbref, int, char *);

/**
 * @brief Set propdir
 *
 * @param thing DBref of thing
 * @param ifp   Propdir.
 */
void propdir_set(dbref thing, PROPDIR *ifp)
{
    PROPDIR *fp = NULL, *xfp = NULL;
    int i = 0, stat = 0;

    /**
     * If propdir list is null, clear
     *
     */
    if (!ifp || (ifp->count <= 0))
    {
        propdir_clr(thing);
        return;
    }

    /**
     * Copy input propdir to a correctly-sized buffer
     *
     */
    fp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "fp");
    fp->data = (int *)XCALLOC(ifp->count, sizeof(int), "fp->data");

    for (i = 0; i < ifp->count; i++)
    {
        fp->data[i] = ifp->data[i];
    }

    fp->count = ifp->count;

    /**
     * Replace an existing propdir, or add a new one
     *
     */
    xfp = propdir_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        stat = nhashrepl(thing, (int *)fp, &mushstate.propdir_htab);
    }
    else
    {
        stat = nhashadd(thing, (int *)fp, &mushstate.propdir_htab);
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
 * @brief Clear a propdir
 *
 * @param thing DBref of thing
 */
void propdir_clr(dbref thing)
{
    PROPDIR *xfp = NULL;

    /**
     * If a propdir exists, delete it
     *
     */
    xfp = propdir_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        nhashdelete(thing, &mushstate.propdir_htab);
    }
}

/**
 * @brief Load a propdir
 *
 * @param fp        Propdir
 * @param player    DBref of player
 * @param atext     Text
 * @return int
 */
int propdir_load(PROPDIR *fp, dbref player, char *atext)
{
    dbref target = NOTHING;
    int i = 0, count = 0, errors = 0, fail = 0;
    int *tmp_array = (int *)XCALLOC((LBUF_SIZE / 2), sizeof(int), "tmp_array");
    char *tp = XMALLOC(LBUF_SIZE, "tp");
    char *bp = tp, *dp = NULL;

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
                fail = (!Good_obj(target) || !Parentable(player, target));
            }
            else
            {
                fail = !Good_obj(target);
            }

            if (fail)
            {
                if (!mushstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot parent to #%d: Permission denied.", target);
                }

                errors++;
            }
            else if (count < mushconf.propdir_lim)
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
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot parent to #%d: Propdir limit exceeded.", target);
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
 * @brief Rewrite a propdir
 *
 * @param fp    Propdir
 * @param atext Text
 * @return int
 */
int propdir_rewrite(PROPDIR *fp, char *atext)
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
 * @brief Check a popdir
 *
 * @param key       Key
 * @param player    DBref of player
 * @param thing     DBref of thing
 * @param anum      Attribute
 * @param atext     Attribute text
 * @return int
 */
int propdir_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
    PROPDIR *fp = NULL;
    int count = 0;

    if (mushstate.standalone)
    {
        return 1;
    }

    count = 0;

    if (atext && *atext)
    {
        fp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "fp");
        fp->data = NULL;
        propdir_load(fp, player, atext);
    }
    else
    {
        fp = NULL;
    }

    /**
     * Set the cached propdir
     *
     */
    propdir_set(thing, fp);
    count = propdir_rewrite(fp, atext);

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
 * @brief Fetch a Propdir
 *
 * @param thing     DBref of thing
 * @return PROPDIR*
 */
PROPDIR *propdir_get(dbref thing)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *tp = NULL;
    PROPDIR *fp = NULL;

    if (!mushstate.standalone)
    {
        return (PROPDIR *)nhashfind((thing), &mushstate.propdir_htab);
    }

    if (!fp)
    {
        fp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "fp");
        fp->data = NULL;
    }

    tp = atr_get(thing, A_PROPDIR, &aowner, &aflags, &alen);
    propdir_load(fp, GOD, tp);
    XFREE(tp);
    return fp;
}
