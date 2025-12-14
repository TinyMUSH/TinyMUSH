/**
 * @file timer.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Subroutines for (system-) timed events
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
#include <sys/resource.h>
#include <unistd.h>

typedef struct cron_entry CRONTAB;

struct cron_entry
{
    dbref obj;
    int atr;
    char *cronstr;

    unsigned char minute[(((MINUTE_COUNT)-1) >> 3) + 1];
    unsigned char hour[(((HOUR_COUNT)-1) >> 3) + 1];
    unsigned char dom[(((DOM_COUNT)-1) >> 3) + 1];
    unsigned char month[(((MONTH_COUNT)-1) >> 3) + 1];
    unsigned char dow[(((DOW_COUNT)-1) >> 3) + 1];
    int flags;
    CRONTAB *next;
};

CRONTAB *cron_head = NULL;

void check_cron(void)
{
    // Removed: struct tm *ltime;
    int minute, hour, dom, month, dow;
    CRONTAB *crp;
    char *cmd;
    dbref aowner;
    int aflags, alen;
    /*
     * Convert our time to a zero basis, so the elements can be used as indices.
     */
    struct tm ltime;
    localtime_r(&mushstate.events_counter, &ltime);
    minute = ltime.tm_min - FIRST_MINUTE;
    hour = ltime.tm_hour - FIRST_HOUR;
    dom = ltime.tm_mday - FIRST_DOM;
    month = ltime.tm_mon + 1 - FIRST_MONTH; /* must convert 0-11 to 1-12 */
    dow = ltime.tm_wday - FIRST_DOW;

    /*
     * Do it if the minute, hour, and month match, plus a day selection
     * matches. We handle stars and the day-of-month vs. day-of-week
     * exactly like Unix (Vixie) cron does.
     */

    for (crp = cron_head; crp != NULL; crp = crp->next)
    {
        if ((crp->minute[minute >> 3] & (1 << (minute & 0x7))) && (crp->hour[hour >> 3] & (1 << (hour & 0x7))) && (crp->month[month >> 3] & (1 << (month & 0x7))) && (((crp->flags & DOM_STAR) || (crp->flags & DOW_STAR)) ? ((crp->dow[dow >> 3] & (1 << (dow & 0x7))) && (crp->dom[dom >> 3] & (1 << (dom & 0x7)))) : ((crp->dow[dow >> 3] & (1 << (dow & 0x7))) || (crp->dom[dom >> 3] & (1 << (dom & 0x7))))))
        {
            cmd = atr_pget(crp->obj, crp->atr, &aowner, &aflags, &alen);
            if (*cmd && Good_obj(crp->obj))
            {
                wait_que(crp->obj, crp->obj, 0, NOTHING, 0, cmd, (char **)NULL, 0, NULL);
            }
            XFREE(cmd);
        }
    }
}

char *parse_cronlist(dbref player, unsigned char *bits, int low, int high, char *bufp)
{
    int i, n_begin, n_end, step_size;
    unsigned char *_bits = bits; /* Default is all off */
    int _start = 0, _stop = (high - low + 1);
    int _startbyte = (_start >> 3);
    int _stopbyte = (_stop >> 3);

    if (_startbyte == _stopbyte)
    {
        _bits[_startbyte] &= ((0xff >> (8 - (_start & 0x7))) | (0xff << ((_stop & 0x7) + 1)));
    }
    else
    {
        _bits[_startbyte] &= 0xff >> (8 - (_start & 0x7));
        while (++_startbyte < _stopbyte)
            _bits[_startbyte] = 0;
        _bits[_stopbyte] &= 0xff << ((_stop & 0x7) + 1);
    }

    if (!bufp || !*bufp)
    {
        return NULL;
    }

    if (!*bufp)
    {
        return NULL;
    }

    /*
     * We assume we're at the beginning of what we needed to parse.
     * All leading whitespace-skipping should have been taken care of
     * either before this function was called, or at the end of this
     * function.
     */

    while (*bufp && !isspace(*bufp))
    {
        if (*bufp == '*')
        {
            n_begin = low;
            n_end = high;
            bufp++;
        }
        else if (isdigit(*bufp))
        {
            n_begin = (int)strtol(bufp, (char **)NULL, 10);

            while (*bufp && isdigit(*bufp))
            {
                bufp++;
            }

            if (*bufp != '-')
            {
                /*
                 * We have a single number, not a range.
                 */
                n_end = n_begin;
            }
            else
            {
                /*
                 * Eat the dash, get the range.
                 */
                bufp++;
                n_end = (int)strtol(bufp, (char **)NULL, 10);

                while (*bufp && isdigit(*bufp))
                {
                    bufp++;
                }
            }
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cron parse error at: %s", bufp);
            return NULL;
        }

        /*
         * Check for step size.
         */

        if (*bufp == '/')
        {
            bufp++; /* eat the slash */
            step_size = (int)strtol(bufp, (char **)NULL, 10);

            if (step_size < 1)
            {
                notify(player, "Invalid step size.");
                return NULL;
            }

            while (*bufp && isdigit(*bufp))
            {
                bufp++;
            }
        }
        else
        {
            step_size = 1;
        }

        /*
         * Go set it.
         */

        for (i = n_begin; i <= n_end; i += step_size)
        {
            if ((i >= low) && (i <= high))
                bits[i - low] |= (1 << ((i - low) & 0x7));
        }

        /*
         * We've made it through one pass. If the next character isn't a comma, we break out of this loop.
         */

        if (*bufp == ',')
        {
            bufp++;
        }
        else
        {
            break;
        }
    }

    /*
     * Skip over trailing garbage.
     */

    while (*bufp && !isspace(*bufp))
    {
        bufp++;
    }

    /*
     * Initially, bufp pointed to the beginning of what we parsed. We have
     * to return it so we know where to start the next bit of parsing.
     * Skip spaces as well.
     */

    while (isspace(*bufp))
    {
        bufp++;
    }

    return bufp;
}

int call_cron(dbref player, dbref thing, int attrib, char *timestr)
{
    int errcode;
    CRONTAB *crp;
    char *bufp;

    /*
     * Don't allow duplicate entries.
     */

    for (crp = cron_head; crp != NULL; crp = crp->next)
    {
        if ((crp->obj == thing) && (crp->atr == attrib) && !strcmp(crp->cronstr, timestr))
        {
            return -1;
        }
    }

    crp = (CRONTAB *)XMALLOC(sizeof(CRONTAB), "crp");
    crp->obj = thing;
    crp->atr = attrib;
    crp->flags = 0;
    crp->cronstr = XSTRDUP(timestr, "crp->cronstr");

    /*
     * The time string is: <min> <hour> <day of month> <month> <day of week>
     * Legal values also include asterisks, and <x>-<y> (for a range).
     * We do NOT support step size.
     */

    errcode = 0;
    bufp = timestr;

    while (isspace(*bufp))
    {
        bufp++;
    }

    bufp = parse_cronlist(player, crp->minute, FIRST_MINUTE, LAST_MINUTE, bufp);

    if (!bufp || !*bufp)
    {
        errcode = 1;
    }
    else
    {
        bufp = parse_cronlist(player, crp->hour, FIRST_HOUR, LAST_HOUR, bufp);

        if (!bufp || !*bufp)
        {
            errcode = 1;
        }
    }

    if (!errcode)
    {
        if (*bufp == '*')
        {
            crp->flags |= DOM_STAR;
        }

        bufp = parse_cronlist(player, crp->dom, FIRST_DOM, LAST_DOM, bufp);

        if (!bufp || !*bufp)
        {
            errcode = 1;
        }
    }

    if (!errcode)
    {
        bufp = parse_cronlist(player, crp->month, FIRST_MONTH, LAST_MONTH, bufp);

        if (!bufp || !*bufp)
        {
            errcode = 1;
        }
    }

    if (!errcode)
    {
        if (*bufp == '*')
        {
            crp->flags |= DOW_STAR;
        }

        bufp = parse_cronlist(player, crp->dow, FIRST_DOW, LAST_DOW, bufp);
    }

    /*
     * Sundays can be either 0 or 7.
     */

    if (crp->dow[0] & 1)
    {
        crp->dow[0] |= 128;
    }

    if (crp->dow[0] & 128)
    {
        crp->dow[0] |= 1;
    }

    if (errcode)
    {
        XFREE(crp->cronstr);
        XFREE(crp);
        return 0;
    }

    /*
     * Relink the list, now that we know we have something good.
     */

    crp->next = cron_head;
    cron_head = crp;
    return 1;
}

void do_cron(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int key, char *objstr, char *timestr)
{
    dbref thing;
    int attrib, retcode;

    if (!timestr || !*timestr)
    {
        notify(player, "No times given.");
        return;
    }

    if (!parse_attrib(player, objstr, &thing, &attrib, 0) || (attrib == NOTHING) || !Good_obj(thing))
    {
        notify(player, "No match.");
        return;
    }

    if (!Controls(player, thing))
    {
        notify(player, NOPERM_MESSAGE);
        return;
    }

    retcode = call_cron(player, thing, attrib, timestr);

    if (retcode == 0)
    {
        notify(player, "Syntax errors. No cron entry made.");
    }
    else if (retcode == -1)
    {
        notify(player, "That cron entry already exists.");
    }
    else
    {
        notify(player, "Cron entry added.");
    }
}

int cron_clr(dbref thing, int attr)
{
    CRONTAB *crp, *next, *prev;
    int count;
    count = 0;

    for (crp = cron_head, prev = NULL; crp != NULL;)
    {
        if ((crp->obj == thing) && ((attr == NOTHING) || (crp->atr == attr)))
        {
            count++;
            next = crp->next;
            XFREE(crp->cronstr);
            XFREE(crp);

            if (prev)
            {
                prev->next = next;
            }
            else
            {
                cron_head = next;
            }

            crp = next;
        }
        else
        {
            prev = crp;
            crp = crp->next;
        }
    }

    return count;
}

void do_crondel(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int key, char *objstr)
{
    dbref thing;
    int attr, count;

    if (!objstr || !*objstr)
    {
        notify(player, "No match.");
        return;
    }

    attr = NOTHING;

    if (!parse_attrib(player, objstr, &thing, &attr, 0) || (attr == NOTHING))
    {
        if ((*objstr != '#') || ((thing = parse_dbref(objstr + 1)) == NOTHING))
        {
            notify(player, "No match.");
        }
    }

    if (!Controls(player, thing))
    {
        notify(player, NOPERM_MESSAGE);
        return;
    }

    count = cron_clr(thing, attr);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Removed %d cron entries.", count);
}

void do_crontab(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int key, char *objstr)
{
    dbref thing;
    int count;
    CRONTAB *crp;
    char *bufp;
    ATTR *ap;

    if (objstr && *objstr)
    {
        thing = match_thing(player, objstr);

        if (!Good_obj(thing))
        {
            return;
        }

        if (!Controls(player, thing))
        {
            notify(player, NOPERM_MESSAGE);
            return;
        }
    }
    else
    {
        thing = NOTHING;
    }

    /*
     * List it if it's the thing we want, or, if we didn't specify a
     * thing, list things belonging to us (or everything, if we're
     * normally entitled to see the entire queue).
     */

    count = 0;

    for (crp = cron_head; crp != NULL; crp = crp->next)
    {
        if (((thing == NOTHING) && ((Owner(crp->obj) == player) || See_Queue(player))) || (crp->obj == thing))
        {
            count++;
            bufp = unparse_object(player, crp->obj, 0);
            ap = atr_num(crp->atr);

            if (!ap)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s has a cron entry that contains bad attribute number %d.", bufp, crp->atr);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s/%s: %s", bufp, ap->name, crp->cronstr);
            }

            XFREE(bufp);
        }
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Matched %d cron %s.", count, (count == 1) ? "entry" : "entries");
}

/* ---------------------------------------------------------------------------
 * General timer things.
 */

void init_timer(void)
{
    mushstate.now = time(NULL);
    mushstate.dump_counter = ((mushconf.dump_offset == 0) ? mushconf.dump_interval : mushconf.dump_offset) + mushstate.now;
    mushstate.check_counter = ((mushconf.check_offset == 0) ? mushconf.check_interval : mushconf.check_offset) + mushstate.now;
    mushstate.idle_counter = mushconf.idle_interval + mushstate.now;
    mushstate.mstats_counter = 15 + mushstate.now;

    /*
     * The events counter is the next time divisible by sixty (i.e., that puts us at the beginning of a minute).
     */

    mushstate.events_counter = mushstate.now + (60 - (mushstate.now % 60));
    alarm(1);
}

void dispatch(void)
{
    char *cmdsave;
    cmdsave = mushstate.debug_cmd;
    mushstate.debug_cmd = (char *)"< dispatch >";

    /*
     * This routine can be used to poll from interface.c
     */

    if (!mushstate.alarm_triggered)
    {
        return;
    }

    mushstate.alarm_triggered = 0;
    mushstate.now = time(NULL);
    do_second();

    /*
     * Module API hook
     */

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        if (cam__mp->do_second)
        {
            (*(cam__mp->do_second))();
        }
    }

    /*
     * Free list reconstruction
     */

    if ((mushconf.control_flags & CF_DBCHECK) && (mushstate.check_counter <= mushstate.now))
    {
        mushstate.check_counter = mushconf.check_interval + mushstate.now;
        mushstate.debug_cmd = (char *)"< dbck >";
        do_dbck(NOTHING, NOTHING, 0);
        cache_sync();
        pcache_trim();
    }

    /*
     * Database dump routines
     */

    if ((mushconf.control_flags & CF_CHECKPOINT) && (mushstate.dump_counter <= mushstate.now))
    {
        mushstate.dump_counter = mushconf.dump_interval + mushstate.now;
        mushstate.debug_cmd = (char *)"< dump >";
        fork_and_dump(NOTHING, NOTHING, 0);
    }

    /*
     * Idle user check
     */

    if ((mushconf.control_flags & CF_IDLECHECK) && (mushstate.idle_counter <= mushstate.now))
    {
        mushstate.idle_counter = mushconf.idle_interval + mushstate.now;
        mushstate.debug_cmd = (char *)"< idlecheck >";
        check_idle();
    }

    /*
     * Check for execution of attribute events
     */

    if (mushconf.control_flags & CF_EVENTCHECK)
    {
        if (mushstate.now >= mushstate.events_counter)
        {
            mushstate.debug_cmd = (char *)"< croncheck >";
            check_cron();
            mushstate.events_counter += 60;
        }
    }

    /*
     * Memory use stats
     */

    if (mushstate.mstats_counter <= mushstate.now)
    {
        int curr;
        mushstate.mstats_counter = 15 + mushstate.now;
        curr = mushstate.mstat_curr;

        if (mushstate.now > mushstate.mstat_secs[curr])
        {
            struct rusage usage;
            curr = 1 - curr;
            getrusage(RUSAGE_SELF, &usage);
            mushstate.mstat_ixrss[curr] = usage.ru_ixrss;
            mushstate.mstat_idrss[curr] = usage.ru_idrss;
            mushstate.mstat_isrss[curr] = usage.ru_isrss;
            mushstate.mstat_secs[curr] = mushstate.now;
            mushstate.mstat_curr = curr;
        }
    }

    /*
     * reset alarm
     */

    alarm(1);
    mushstate.debug_cmd = cmdsave;
}

/*
 * ---------------------------------------------------------------------------
 * do_timewarp: Adjust various internal timers.
 */

void do_timewarp(dbref player, dbref cause, int key, char *arg)
{
    int secs;
    secs = (int)strtol(arg, (char **)NULL, 10);

    /*
     * Sem Wait queues
     */

    if ((key == 0) || (key & TWARP_QUEUE))
    {
        do_queue(player, cause, QUEUE_WARP, arg);
    }

    if (key & TWARP_DUMP)
    {
        mushstate.dump_counter -= secs;
    }

    if (key & TWARP_CLEAN)
    {
        mushstate.check_counter -= secs;
    }

    if (key & TWARP_IDLE)
    {
        mushstate.idle_counter -= secs;
    }

    if (key & TWARP_EVENTS)
    {
        mushstate.events_counter -= secs;
    }
}
