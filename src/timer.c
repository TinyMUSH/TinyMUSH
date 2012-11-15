/* timer.c - Subroutines for (system-) timed events */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */
#include "typedefs.h"           /* required by mudconf */
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "interface.h"		/* required by code */
#include "externs.h"		/* required by interface */


#include "match.h"		/* required by code */
//#include "command.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "bitstring.h"		/* required by code */

extern void pool_reset( void );

extern unsigned int alarm( unsigned int seconds );

extern void pcache_trim( void );

extern void do_dbck( dbref, dbref, int );

extern void do_queue( dbref, dbref, int, char * );

/* ---------------------------------------------------------------------------
 * Cron-related things. This implementation is somewhat derivative of
 * Paul Vixie's cron implementation. See bitstring.h for the copyright
 * and other details.
 */

#define FIRST_MINUTE    0
#define LAST_MINUTE     59
#define MINUTE_COUNT    (LAST_MINUTE - FIRST_MINUTE + 1)

#define FIRST_HOUR      0
#define LAST_HOUR       23
#define HOUR_COUNT      (LAST_HOUR - FIRST_HOUR + 1)

#define FIRST_DOM       1
#define LAST_DOM        31
#define DOM_COUNT       (LAST_DOM - FIRST_DOM + 1)

#define FIRST_MONTH     1
#define LAST_MONTH      12
#define MONTH_COUNT     (LAST_MONTH - FIRST_MONTH + 1)

/* Note on DOW: 0 and 7 are both Sunday, for compatibility reasons. */
#define FIRST_DOW       0
#define LAST_DOW        7
#define DOW_COUNT       (LAST_DOW - FIRST_DOW + 1)

#define DOM_STAR        0x01
#define DOW_STAR        0x02

typedef struct cron_entry CRONTAB;

struct cron_entry {
    dbref obj;
    int atr;
    char *cronstr;
    bitstr_t bit_decl( minute, MINUTE_COUNT );
    bitstr_t bit_decl( hour, HOUR_COUNT );
    bitstr_t bit_decl( dom, DOM_COUNT );
    bitstr_t bit_decl( month, MONTH_COUNT );
    bitstr_t bit_decl( dow, DOW_COUNT );
    int flags;
    CRONTAB *next;
};

CRONTAB *cron_head = NULL;

#define set_cronbits(b,l,h,n) \
        if (((n) >= (l)) && ((n) <= (h))) bit_set((b), (n) - (l));

static void check_cron( void ) {
    struct tm *ltime;

    int minute, hour, dom, month, dow;

    CRONTAB *crp;

    char *cmd;

    dbref aowner;

    int aflags, alen;

    /*
     * Convert our time to a zero basis, so the elements can be used
     * * as indices.
     */

    ltime = localtime( &mudstate.events_counter );
    minute = ltime->tm_min - FIRST_MINUTE;
    hour = ltime->tm_hour - FIRST_HOUR;
    dom = ltime->tm_mday - FIRST_DOM;
    month = ltime->tm_mon + 1 - FIRST_MONTH;	/* must convert 0-11 to 1-12 */
    dow = ltime->tm_wday - FIRST_DOW;

    /*
     * Do it if the minute, hour, and month match, plus a day selection
     * * matches. We handle stars and the day-of-month vs. day-of-week
     * * exactly like Unix (Vixie) cron does.
     */

    for( crp = cron_head; crp != NULL; crp = crp->next ) {
        if( bit_test( crp->minute, minute ) &&
                bit_test( crp->hour, hour ) &&
                bit_test( crp->month, month ) &&
                ( ( ( crp->flags & DOM_STAR ) || ( crp->flags & DOW_STAR ) ) ?
                  ( bit_test( crp->dow, dow ) && bit_test( crp->dom, dom ) ) :
                  ( bit_test( crp->dow, dow ) || bit_test( crp->dom, dom ) ) ) ) {
            cmd =
                atr_pget( crp->obj, crp->atr, &aowner, &aflags,
                          &alen );
            if( *cmd && Good_obj( crp->obj ) ) {
                wait_que( crp->obj, crp->obj, 0, NOTHING, 0,
                          cmd, ( char ** ) NULL, 0, NULL );
            }
            free_lbuf( cmd );
        }
    }
}

static char *parse_cronlist( dbref player, bitstr_t *bits, int low, int high, char *bufp ) {
    int i, n_begin, n_end, step_size;

    bit_nclear( bits, 0, ( high - low + 1 ) );	/* Default is all off */

    if( !bufp || !*bufp ) {
        return NULL;
    }
    if( !*bufp ) {
        return NULL;
    }

    /*
     * We assume we're at the beginning of what we needed to parse.
     * * All leading whitespace-skipping should have been taken care of
     * * either before this function was called, or at the end of this
     * * function.
     */

    while( *bufp && !isspace( *bufp ) ) {
        if( *bufp == '*' ) {
            n_begin = low;
            n_end = high;
            bufp++;
        } else if( isdigit( *bufp ) ) {
            n_begin = ( int ) strtol( bufp, ( char ** ) NULL, 10 );
            while( *bufp && isdigit( *bufp ) ) {
                bufp++;
            }
            if( *bufp != '-' ) {
                /*
                 * We have a single number, not a range.
                 */
                n_end = n_begin;
            } else {
                /*
                 * Eat the dash, get the range.
                 */
                bufp++;
                n_end = ( int ) strtol( bufp, ( char ** ) NULL, 10 );
                while( *bufp && isdigit( *bufp ) ) {
                    bufp++;
                }
            }
        } else {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Cron parse error at: %s", bufp );
            return NULL;
        }

        /*
         * Check for step size.
         */

        if( *bufp == '/' ) {
            bufp++;	/* eat the slash */
            step_size = ( int ) strtol( bufp, ( char ** ) NULL, 10 );
            if( step_size < 1 ) {
                notify( player, "Invalid step size." );
                return NULL;
            }
            while( *bufp && isdigit( *bufp ) ) {
                bufp++;
            }
        } else {
            step_size = 1;
        }

        /*
         * Go set it.
         */

        for( i = n_begin; i <= n_end; i += step_size ) {
            set_cronbits( bits, low, high, i );
        }

        /*
         * We've made it through one pass. If the next character isn't
         * * a comma, we break out of this loop.
         */
        if( *bufp == ',' ) {
            bufp++;
        } else {
            break;
        }
    }

    /*
     * Skip over trailing garbage.
     */

    while( *bufp && !isspace( *bufp ) ) {
        bufp++;
    }

    /*
     * Initially, bufp pointed to the beginning of what we parsed. We have
     * * to return it so we know where to start the next bit of parsing.
     * * Skip spaces as well.
     */

    while( isspace( *bufp ) ) {
        bufp++;
    }

    return bufp;
}

int call_cron( dbref player, dbref thing, int attrib, char *timestr ) {
    int errcode;

    CRONTAB *crp;

    char *bufp;

    /*
     * Don't allow duplicate entries.
     */

    for( crp = cron_head; crp != NULL; crp = crp->next ) {
        if( ( crp->obj == thing ) && ( crp->atr == attrib ) &&
                !strcmp( crp->cronstr, timestr ) ) {
            return -1;
        }
    }

    crp = ( CRONTAB * ) XMALLOC( sizeof( CRONTAB ), "cron_entry" );
    crp->obj = thing;
    crp->atr = attrib;
    crp->flags = 0;
    crp->cronstr = XSTRDUP( timestr, "cron_entry.time" );

    /*
     * The time string is: <min> <hour> <day of month> <month> <day of week>
     * * Legal values also include asterisks, and <x>-<y> (for a range).
     * * We do NOT support step size.
     */

    errcode = 0;
    bufp = timestr;
    while( isspace( *bufp ) ) {
        bufp++;
    }
    bufp = parse_cronlist( player, crp->minute,
                           FIRST_MINUTE, LAST_MINUTE, bufp );
    if( !bufp || !*bufp ) {
        errcode = 1;
    } else {
        bufp =
            parse_cronlist( player, crp->hour, FIRST_HOUR, LAST_HOUR,
                            bufp );
        if( !bufp || !*bufp ) {
            errcode = 1;
        }
    }
    if( !errcode ) {
        if( *bufp == '*' ) {
            crp->flags |= DOM_STAR;
        }
        bufp =
            parse_cronlist( player, crp->dom, FIRST_DOM, LAST_DOM,
                            bufp );
        if( !bufp || !*bufp ) {
            errcode = 1;
        }
    }
    if( !errcode ) {
        bufp = parse_cronlist( player, crp->month,
                               FIRST_MONTH, LAST_MONTH, bufp );
        if( !bufp || !*bufp ) {
            errcode = 1;
        }
    }
    if( !errcode ) {
        if( *bufp == '*' ) {
            crp->flags |= DOW_STAR;
        }
        bufp =
            parse_cronlist( player, crp->dow, FIRST_DOW, LAST_DOW,
                            bufp );
    }

    /*
     * Sundays can be either 0 or 7.
     */

    if( bit_test( crp->dow, 0 ) ) {
        bit_set( crp->dow, 7 );
    }
    if( bit_test( crp->dow, 7 ) ) {
        bit_set( crp->dow, 0 );
    }

    if( errcode ) {
        XFREE( crp->cronstr, "cron_entry.time" );
        XFREE( crp, "cron_entry" );
        return 0;
    }

    /*
     * Relink the list, now that we know we have something good.
     */

    crp->next = cron_head;
    cron_head = crp;

    return 1;
}

void do_cron( dbref player, dbref cause, int key, char *objstr, char *timestr ) {
    dbref thing;

    int attrib, retcode;

    if( !timestr || !*timestr ) {
        notify( player, "No times given." );
        return;
    }

    if( !parse_attrib( player, objstr, &thing, &attrib, 0 ) ||
            ( attrib == NOTHING ) || !Good_obj( thing ) ) {
        notify( player, "No match." );
        return;
    }

    if( !Controls( player, thing ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }

    retcode = call_cron( player, thing, attrib, timestr );
    if( retcode == 0 ) {
        notify( player, "Syntax errors. No cron entry made." );
    } else if( retcode == -1 ) {
        notify( player, "That cron entry already exists." );
    } else {
        notify( player, "Cron entry added." );
    }
}

int cron_clr( dbref thing, int attr ) {
    CRONTAB *crp, *next, *prev;

    int count;

    count = 0;
    for( crp = cron_head, prev = NULL; crp != NULL; ) {
        if( ( crp->obj == thing ) &&
                ( ( attr == NOTHING ) || ( crp->atr == attr ) ) ) {
            count++;
            next = crp->next;
            XFREE( crp->cronstr, "cron_entry.time" );
            XFREE( crp, "cron_entry" );
            if( prev ) {
                prev->next = next;
            } else {
                cron_head = next;
            }
            crp = next;
        } else {
            prev = crp;
            crp = crp->next;
        }
    }
    return count;
}

void do_crondel( dbref player, dbref cause, int key, char *objstr ) {
    dbref thing;

    int attr, count;

    if( !objstr || !*objstr ) {
        notify( player, "No match." );
        return;
    }

    attr = NOTHING;
    if( !parse_attrib( player, objstr, &thing, &attr, 0 ) ||
            ( attr == NOTHING ) ) {
        if( ( *objstr != '#' ) ||
                ( ( thing = parse_dbref( objstr + 1 ) ) == NOTHING ) ) {
            notify( player, "No match." );
        }
    }

    if( !Controls( player, thing ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }

    count = cron_clr( thing, attr );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Removed %d cron entries.", count );
}

void do_crontab( dbref player, dbref cause, int key, char *objstr ) {
    dbref thing;

    int count;

    CRONTAB *crp;

    char *bufp;

    ATTR *ap;

    if( objstr && *objstr ) {
        thing = match_thing( player, objstr );
        if( !Good_obj( thing ) ) {
            return;
        }
        if( !Controls( player, thing ) ) {
            notify( player, NOPERM_MESSAGE );
            return;
        }
    } else {
        thing = NOTHING;
    }

    /*
     * List it if it's the thing we want, or, if we didn't specify a
     * * thing, list things belonging to us (or everything, if we're
     * * normally entitled to see the entire queue).
     */

    count = 0;
    for( crp = cron_head; crp != NULL; crp = crp->next ) {
        if( ( ( thing == NOTHING ) &&
                ( ( Owner( crp->obj ) == player ) || See_Queue( player ) ) ) ||
                ( crp->obj == thing ) ) {
            count++;
            bufp = unparse_object( player, crp->obj, 0 );
            ap = atr_num( crp->atr );
            if( !ap ) {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s has a cron entry that contains bad attribute number %d.", bufp, crp->atr );
            } else {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s/%s: %s", bufp, ap->name, crp->cronstr );
            }
            free_lbuf( bufp );
        }
    }
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Matched %d cron %s.", count, ( count == 1 ) ? "entry" : "entries" );
}

/* ---------------------------------------------------------------------------
 * General timer things.
 */

void  init_timer( void ) {
    mudstate.now = time( NULL );
    mudstate.dump_counter = ( ( mudconf.dump_offset == 0 ) ?
                              mudconf.dump_interval : mudconf.dump_offset ) + mudstate.now;
    mudstate.check_counter = ( ( mudconf.check_offset == 0 ) ?
                               mudconf.check_interval : mudconf.check_offset ) + mudstate.now;
    mudstate.idle_counter = mudconf.idle_interval + mudstate.now;
    mudstate.mstats_counter = 15 + mudstate.now;

    /*
     * The events counter is the next time divisible by sixty (i.e.,
     * * that puts us at the beginning of a minute).
     */

    mudstate.events_counter = mudstate.now + ( 60 - ( mudstate.now % 60 ) );

    alarm( 1 );
}

void dispatch( void ) {
    char *cmdsave;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = ( char * ) "< dispatch >";

    /*
     * This routine can be used to poll from interface.c
     */

    if( !mudstate.alarm_triggered ) {
        return;
    }
    mudstate.alarm_triggered = 0;
    mudstate.now = time( NULL );

    do_second();

    /*
     * Module API hook
     */

    CALL_ALL_MODULES( do_second, () );

    /*
     * Free list reconstruction
     */

    if( ( mudconf.control_flags & CF_DBCHECK ) &&
            ( mudstate.check_counter <= mudstate.now ) ) {
        mudstate.check_counter = mudconf.check_interval + mudstate.now;
        mudstate.debug_cmd = ( char * ) "< dbck >";
        do_dbck( NOTHING, NOTHING, 0 );
        SYNC;
        pcache_trim();
        pool_reset();
    }

    /*
     * Database dump routines
     */

    if( ( mudconf.control_flags & CF_CHECKPOINT ) &&
            ( mudstate.dump_counter <= mudstate.now ) ) {
        mudstate.dump_counter = mudconf.dump_interval + mudstate.now;
        mudstate.debug_cmd = ( char * ) "< dump >";
        fork_and_dump( NOTHING, NOTHING, 0 );
    }

    /*
     * Idle user check
     */

    if( ( mudconf.control_flags & CF_IDLECHECK ) &&
            ( mudstate.idle_counter <= mudstate.now ) ) {
        mudstate.idle_counter = mudconf.idle_interval + mudstate.now;
        mudstate.debug_cmd = ( char * ) "< idlecheck >";
        check_idle();

    }

    /*
     * Check for execution of attribute events
     */

    if( mudconf.control_flags & CF_EVENTCHECK ) {
        if( mudstate.now >= mudstate.events_counter ) {
            mudstate.debug_cmd = ( char * ) "< croncheck >";
            check_cron();
            mudstate.events_counter += 60;
        }
    }
#if defined(HAVE_GETRUSAGE) && defined(STRUCT_RUSAGE_COMPLETE)

    /*
     * Memory use stats
     */

    if( mudstate.mstats_counter <= mudstate.now ) {

        int curr;

        mudstate.mstats_counter = 15 + mudstate.now;
        curr = mudstate.mstat_curr;
        if( mudstate.now > mudstate.mstat_secs[curr] ) {

            struct rusage usage;

            curr = 1 - curr;
            getrusage( RUSAGE_SELF, &usage );
            mudstate.mstat_ixrss[curr] = usage.ru_ixrss;
            mudstate.mstat_idrss[curr] = usage.ru_idrss;
            mudstate.mstat_isrss[curr] = usage.ru_isrss;
            mudstate.mstat_secs[curr] = mudstate.now;
            mudstate.mstat_curr = curr;
        }
    }
#endif

    /*
     * reset alarm
     */

    alarm( 1 );
    mudstate.debug_cmd = cmdsave;
}

/*
 * ---------------------------------------------------------------------------
 * * do_timewarp: Adjust various internal timers.
 */

void do_timewarp( dbref player, dbref cause, int key, char *arg ) {
    int secs;

    secs = ( int ) strtol( arg, ( char ** ) NULL, 10 );

    if( ( key == 0 ) || ( key & TWARP_QUEUE ) )	/*
						 * Sem/Wait queues
						 */
    {
        do_queue( player, cause, QUEUE_WARP, arg );
    }
    if( key & TWARP_DUMP ) {
        mudstate.dump_counter -= secs;
    }
    if( key & TWARP_CLEAN ) {
        mudstate.check_counter -= secs;
    }
    if( key & TWARP_IDLE ) {
        mudstate.idle_counter -= secs;
    }
    if( key & TWARP_EVENTS ) {
        mudstate.events_counter -= secs;
    }
}
