/* netcommon.c */

/* This file contains routines used by the networking code that do not
 * depend on the implementation of the networking code.  The network-specific
 * portions of the descriptor data structure are not used.
 */

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


#include "file_c.h"		/* required by code */
#include "command.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "ansi.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "match.h"		/* required by code */

extern int process_output( DESC * d );

extern void handle_prog( DESC *, char * );

extern void record_login( dbref, int, char *, char *, char * );

extern dbref connect_player( char *, char *, char *, char *, char * );

extern char *make_guest( DESC * );

extern const char *conn_reasons[];

extern const char *conn_messages[];

extern int ansi_nchartab[];

/* ---------------------------------------------------------------------------
 * timeval_sub: return difference between two times as a timeval
 */

struct timeval timeval_sub( struct timeval now, struct timeval then ) {
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;
    if( now.tv_usec < 0 ) {
        now.tv_usec += 1000000;
        now.tv_sec--;
    }
    return now;
}

/* ---------------------------------------------------------------------------
 * msec_diff: return difference between two times in milliseconds
 */

int msec_diff( struct timeval now, struct timeval then ) {
    return ( ( now.tv_sec - then.tv_sec ) * 1000 +
             ( now.tv_usec - then.tv_usec ) / 1000 );
}

/* ---------------------------------------------------------------------------
 * msec_add: add milliseconds to a timeval
 */

struct timeval msec_add( struct timeval t, int x ) {
    t.tv_sec += x / 1000;
    t.tv_usec += ( x % 1000 ) * 1000;
    if( t.tv_usec >= 1000000 ) {
        t.tv_sec += t.tv_usec / 1000000;
        t.tv_usec = t.tv_usec % 1000000;
    }
    return t;
}

/* ---------------------------------------------------------------------------
 * update_quotas: Update timeslice quotas
 */

struct timeval update_quotas( struct timeval last, struct timeval current ) {
    int nslices;

    DESC *d;

    nslices = msec_diff( current, last ) / mudconf.timeslice;

    if( nslices > 0 ) {
        DESC_ITER_ALL( d ) {
            d->quota += mudconf.cmd_quota_incr * nslices;
            if( d->quota > mudconf.cmd_quota_max ) {
                d->quota = mudconf.cmd_quota_max;
            }
        }
    }
    return msec_add( last, nslices * mudconf.timeslice );
}

#ifdef PUEBLO_SUPPORT
/* raw_notify_html() -- raw_notify() without the newline */
void raw_notify_html( dbref player, const char *format, ... ) {
    DESC *d;
    char msg[LBUF_SIZE];
    char *s;
    va_list ap;
    
    va_start( ap, format );
    
    if( !format || !*format ) {
        if( ( s = va_arg(ap, char *) ) != NULL ) {
            strncpy(msg, s, LBUF_SIZE);
        } else {
            return;
        }
    } else {
        vsnprintf(msg, LBUF_SIZE, format, ap);
    }

    va_end(ap);

    if( mudstate.inpipe && ( player == mudstate.poutobj ) ) {
        safe_str( msg, mudstate.poutnew, &mudstate.poutbufc );
        return;
    }

    if( !Connected( player ) ) {
        return;
    }

    if( !Html( player ) ) {	/* Don't splooge HTML at a non-HTML player. */
        return;
    }

    DESC_ITER_PLAYER( player, d ) {
        queue_string( d, NULL, msg );
    }
}
#endif

/* ---------------------------------------------------------------------------
 * raw_notify: write a message to a player
 */

void raw_notify( dbref player, const char *format, ... ) {
    DESC *d;
    char msg[LBUF_SIZE];
    char *s;
    va_list ap;

    va_start( ap, format );
    
    if( !format || !*format ) {
        if( ( s = va_arg(ap, char *) ) != NULL ) {
            strncpy(msg, s, LBUF_SIZE);
        } else {
            return;
        }
    } else {
        vsnprintf(msg, LBUF_SIZE, format, ap);
    }

    va_end(ap);

    
    if( mudstate.inpipe && ( player == mudstate.poutobj ) ) {
        safe_str( msg, mudstate.poutnew, &mudstate.poutbufc );
        safe_crlf( mudstate.poutnew, &mudstate.poutbufc );
        return;
    }

    if( !Connected( player ) ) {
        return;
    }

    DESC_ITER_PLAYER( player, d ) {
        queue_string( d, NULL, msg );
        queue_write( d, "\r\n", 2 );
    }
}

void raw_notify_newline( dbref player ) {
    DESC *d;

    if( mudstate.inpipe && ( player == mudstate.poutobj ) ) {
        safe_crlf( mudstate.poutnew, &mudstate.poutbufc );
        return;
    }
    if( !Connected( player ) ) {
        return;
    }

    DESC_ITER_PLAYER( player, d ) {
        queue_write( d, "\r\n", 2 );
    }
}

/* ---------------------------------------------------------------------------
 * raw_broadcast: Send message to players who have indicated flags
 */

void raw_broadcast( int inflags, char *template, ... ) {
    char buff[LBUF_SIZE];
    DESC *d;

    int test_flag, which_flag, p_flag;

    va_list ap;

    va_start( ap, template );

    if( !template || !*template ) {
        return;
    }

    vsnprintf( buff, LBUF_SIZE, template, ap );

    /*
     * Note that this use of the flagwords precludes testing for
     * * type in this function. (Not that this matters, since we
     * * look at connected descriptors, which must be players.)
     */

    test_flag = inflags & ~( FLAG_WORD2 | FLAG_WORD3 );
    if( inflags & FLAG_WORD2 ) {
        which_flag = 2;
    } else if( inflags & FLAG_WORD3 ) {
        which_flag = 3;
    } else {
        which_flag = 1;
    }

    DESC_ITER_CONN( d ) {
        switch( which_flag ) {
        case 1:
            p_flag = Flags( d->player );
            break;
        case 2:
            p_flag = Flags2( d->player );
            break;
        case 3:
            p_flag = Flags3( d->player );
            break;
        default:
            p_flag = Flags( d->player );
        }
        /*
         * If inflags is 0, broadcast to everyone
         */

        if( ( p_flag & test_flag ) || ( !test_flag ) ) {
            queue_string( d, NULL, buff );
            queue_write( d, "\r\n", 2 );
            process_output( d );
        }
    }
    va_end( ap );
}

/* ---------------------------------------------------------------------------
 * clearstrings: clear out prefix and suffix strings
 */

void clearstrings( DESC *d ) {
    if( d->output_prefix ) {
        free_lbuf( d->output_prefix );
        d->output_prefix = NULL;
    }
    if( d->output_suffix ) {
        free_lbuf( d->output_suffix );
        d->output_suffix = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * queue_write: Add text to the output queue for the indicated descriptor.
 */

void queue_write( DESC *d, const char *b, int n ) {
    int left;

    char *name;

    TBLOCK *tp;

    if( n <= 0 ) {
        return;
    }

    if( d->output_size + n > mudconf.output_limit ) {
        process_output( d );
    }

    left = mudconf.output_limit - d->output_size - n;
    if( left < 0 ) {
        tp = d->output_head;
        if( tp == NULL ) {
            log_write( LOG_PROBLEMS, "QUE", "WRITE", "Flushing when output_head is null!" );
        } else {
            name = log_getname( d->player, "queue_write" );
            log_write( LOG_NET, "NET", "WRITE", "[%d/%s] Output buffer overflow, %d chars discarded by %s", d->descriptor, d->addr, tp->hdr.nchars, name );
            XFREE( name, "queue_write" );
            d->output_size -= tp->hdr.nchars;
            d->output_head = tp->hdr.nxt;
            d->output_lost += tp->hdr.nchars;
            if( d->output_head == NULL ) {
                d->output_tail = NULL;
            }
            XFREE( tp, "queue_write.tp" );
        }
    }
    /*
     * Allocate an output buffer if needed
     */

    if( d->output_head == NULL ) {
        tp = ( TBLOCK * ) XMALLOC( OUTPUT_BLOCK_SIZE, "queue_write" );
        tp->hdr.nxt = NULL;
        tp->hdr.start = tp->data;
        tp->hdr.end = tp->data;
        tp->hdr.nchars = 0;
        d->output_head = tp;
        d->output_tail = tp;
    } else {
        tp = d->output_tail;
    }

    /*
     * Now tp points to the last buffer in the chain
     */

    d->output_size += n;
    d->output_tot += n;
    do {

        /*
         * See if there is enough space in the buffer to hold the
         * * string.  If so, copy it and update the pointers..
         */

        left = OUTPUT_BLOCK_SIZE - ( tp->hdr.end - ( char * ) tp + 1 );
        if( n <= left ) {
            strncpy( tp->hdr.end, b, n );
            tp->hdr.end += n;
            tp->hdr.nchars += n;
            n = 0;
        } else {

            /*
             * It didn't fit.  Copy what will fit and then
             * * allocate * another buffer and retry.
             */

            if( left > 0 ) {
                strncpy( tp->hdr.end, b, left );
                tp->hdr.end += left;
                tp->hdr.nchars += left;
                b += left;
                n -= left;
            }
            tp = ( TBLOCK * ) XMALLOC( OUTPUT_BLOCK_SIZE,
                                       "queue_write.2" );
            tp->hdr.nxt = NULL;
            tp->hdr.start = tp->data;
            tp->hdr.end = tp->data;
            tp->hdr.nchars = 0;
            d->output_tail->hdr.nxt = tp;
            d->output_tail = tp;
        }
    } while( n > 0 );
}

void queue_string( DESC *d, const char *format, ... ) {
    char *new;
    char msg[LBUF_SIZE];
    char *s;
    va_list ap;
                
    va_start( ap, format );
                        
    if( !format || !*format ) {
        if( ( s = va_arg(ap, char *) ) != NULL ) {
            strncpy(msg, s, LBUF_SIZE);
        } else {
            return;
        }
    } else {
        vsnprintf(msg, LBUF_SIZE, format, ap);
    }
    
    va_end(ap);
    

    if( msg ) {
        if( !mudconf.ansi_colors ) {
            queue_write( d, msg, strlen( msg ) );
        } else {
            if( !Ansi( d->player ) && strchr( s, ESC_CHAR ) ) {
                new = strip_ansi( msg );
            } else if( NoBleed( d->player ) ) {
                new = normal_to_white( msg );
            } else if( d->colormap ) {
                new = ( char * ) remap_colors( msg, d->colormap );
            } else {
                new = ( char * ) msg;
            }
            queue_write( d, new, strlen( new ) );
        }
    }
}

void queue_rawstring( DESC *d, const char *format, ... ) {
    char msg[LBUF_SIZE];
    char *s;
    va_list ap;
                
    va_start( ap, format );
                        
    if( !format || !*format ) {
        if( ( s = va_arg(ap, char *) ) != NULL ) {
            strncpy(msg, s, LBUF_SIZE);
        } else {
            return;
        }
    } else {
        vsnprintf(msg, LBUF_SIZE, format, ap);
    }
    
    va_end(ap);


    queue_write( d, msg, strlen( msg ) );
}

void freeqs( DESC *d ) {
    TBLOCK *tb, *tnext;

    CBLK *cb, *cnext;

    tb = d->output_head;
    while( tb ) {
        tnext = tb->hdr.nxt;
        XFREE( tb, "freeqs.tb" );
        tb = tnext;
    }
    d->output_head = NULL;
    d->output_tail = NULL;

    cb = d->input_head;
    while( cb ) {
        cnext = ( CBLK * ) cb->hdr.nxt;
        free_lbuf( cb );
        cb = cnext;
    }

    d->input_head = NULL;
    d->input_tail = NULL;

    if( d->raw_input ) {
        free_lbuf( d->raw_input );
    }
    d->raw_input = NULL;
    d->raw_input_at = NULL;
}

/* ---------------------------------------------------------------------------
 * desc_addhash: Add a net descriptor to its player hash list.
 */

void desc_addhash( DESC *d ) {
    dbref player;

    DESC *hdesc;

    player = d->player;
    hdesc = ( DESC * ) nhashfind( ( int ) player, &mudstate.desc_htab );
    if( hdesc == NULL ) {
        d->hashnext = NULL;
        nhashadd( ( int ) player, ( int * ) d, &mudstate.desc_htab );
    } else {
        d->hashnext = hdesc;
        nhashrepl( ( int ) player, ( int * ) d, &mudstate.desc_htab );
    }
}

/* ---------------------------------------------------------------------------
 * desc_delhash: Remove a net descriptor from its player hash list.
 */

static void desc_delhash( DESC *d ) {
    DESC *hdesc, *last;

    dbref player;

    player = d->player;
    last = NULL;
    hdesc = ( DESC * ) nhashfind( ( int ) player, &mudstate.desc_htab );
    while( hdesc != NULL ) {
        if( d == hdesc ) {
            if( last == NULL ) {
                if( d->hashnext == NULL ) {
                    nhashdelete( ( int ) player,
                                 &mudstate.desc_htab );
                } else {
                    nhashrepl( ( int ) player,
                               ( int * ) d->hashnext,
                               &mudstate.desc_htab );
                }
            } else {
                last->hashnext = d->hashnext;
            }
            break;
        }
        last = hdesc;
        hdesc = hdesc->hashnext;
    }
    d->hashnext = NULL;
}

void welcome_user( DESC *d ) {
#ifdef PUEBLO_SUPPORT
    queue_rawstring( d, NULL, PUEBLO_SUPPORT_MSG );
#endif
    if( d->host_info & H_REGISTRATION ) {
        fcache_dump( d, FC_CONN_REG );
    } else {
        fcache_dump( d, FC_CONN );
    }
}

void save_command( DESC *d, CBLK *command ) {

    command->hdr.nxt = NULL;
    if( d->input_tail == NULL ) {
        d->input_head = command;
    } else {
        d->input_tail->hdr.nxt = command;
    }
    d->input_tail = command;
}

static void set_userstring( char **userstring, const char *command ) {
    while( *command && isascii( *command ) && isspace( *command ) ) {
        command++;
    }
    if( !*command ) {
        if( *userstring != NULL ) {
            free_lbuf( *userstring );
            *userstring = NULL;
        }
    } else {
        if( *userstring == NULL ) {
            *userstring = alloc_lbuf( "set_userstring" );
        }
        strcpy( *userstring, command );
    }
}

static void parse_connect( const char *msg, char *command, char *user, char *pass ) {
    char *p;

    if( strlen( msg ) > MBUF_SIZE ) {
        *command = '\0';
        *user = '\0';
        *pass = '\0';
        return;
    }
    while( *msg && isascii( *msg ) && isspace( *msg ) ) {
        msg++;
    }
    p = command;
    while( *msg && isascii( *msg ) && !isspace( *msg ) ) {
        *p++ = *msg++;
    }
    *p = '\0';
    while( *msg && isascii( *msg ) && isspace( *msg ) ) {
        msg++;
    }
    p = user;
    if( mudconf.name_spaces && ( *msg == '\"' ) ) {
        for( ; *msg && ( *msg == '\"' || isspace( *msg ) ); msg++ );
        while( *msg && *msg != '\"' ) {
            while( *msg && !isspace( *msg ) && ( *msg != '\"' ) ) {
                *p++ = *msg++;
            }
            if( *msg == '\"' ) {
                break;
            }
            while( *msg && isspace( *msg ) ) {
                msg++;
            }
            if( *msg && ( *msg != '\"' ) ) {
                *p++ = ' ';
            }
        }
        for( ; *msg && *msg == '\"'; msg++ );
    } else
        while( *msg && isascii( *msg ) && !isspace( *msg ) ) {
            *p++ = *msg++;
        }
    *p = '\0';
    while( *msg && isascii( *msg ) && isspace( *msg ) ) {
        msg++;
    }
    p = pass;
    while( *msg && isascii( *msg ) && !isspace( *msg ) ) {
        *p++ = *msg++;
    }
    *p = '\0';
}

static const char * time_format_1( time_t dt ) {
    register struct tm *delta;

    static char buf[64];

    if( dt < 0 ) {
        dt = 0;
    }

    delta = gmtime( &dt );
    if( delta->tm_yday > 0 ) {
        sprintf( buf, "%dd %02d:%02d",
                 delta->tm_yday, delta->tm_hour, delta->tm_min );
    } else {
        sprintf( buf, "%02d:%02d", delta->tm_hour, delta->tm_min );
    }
    return buf;
}

static const char *time_format_2( time_t dt ) {
    register struct tm *delta;

    static char buf[64];

    if( dt < 0 ) {
        dt = 0;
    }

    delta = gmtime( &dt );
    if( delta->tm_yday > 0 ) {
        sprintf( buf, "%dd", delta->tm_yday );
    } else if( delta->tm_hour > 0 ) {
        sprintf( buf, "%dh", delta->tm_hour );
    } else if( delta->tm_min > 0 ) {
        sprintf( buf, "%dm", delta->tm_min );
    } else {
        sprintf( buf, "%ds", delta->tm_sec );
    }
    return buf;
}

static void announce_connattr( DESC *d, dbref player, dbref loc, const char *reason, int num, int attr ) {
    dbref aowner, obj, zone;

    int aflags, alen, argn;

    char *buf;

    char *argv[7];

    /*
     * Pass session information on the stack:
     * *   %0 - reason message
     * *   %1 - current number of connections
     * *   %2 - connect time
     * *   %3 - last input
     * *   %4 - number of commands entered
     * *   %5 - bytes input
     * *   %6 - bytes output
     */

    argv[0] = ( char * ) reason;
    argv[1] = alloc_sbuf( "announce_connchange.num" );
    sprintf( argv[1], "%d", num );
    if( attr == A_ADISCONNECT ) {
        argn = 7;
        argv[2] = alloc_sbuf( "announce_connchange.conn" );
        argv[3] = alloc_sbuf( "announce_connchange.last" );
        argv[4] = alloc_sbuf( "announce_connchange.cmds" );
        argv[5] = alloc_sbuf( "announce_connchange.in" );
        argv[6] = alloc_sbuf( "announce_connchange.out" );
        sprintf( argv[2], "%ld", ( long ) d->connected_at );
        sprintf( argv[3], "%ld", ( long ) d->last_time );
        sprintf( argv[4], "%d", d->command_count );
        sprintf( argv[5], "%d", d->input_tot );
        sprintf( argv[6], "%d", d->output_tot );
    } else {
        argn = 2;
    }

    buf = atr_pget( player, attr, &aowner, &aflags, &alen );
    if( *buf ) {
        wait_que( player, player, 0, NOTHING, 0, buf, argv, argn, NULL );
    }
    free_lbuf( buf );

    if( Good_loc( mudconf.master_room ) && mudconf.use_global_aconn ) {
        buf = atr_pget( mudconf.master_room, attr, &aowner,
                        &aflags, &alen );
        if( *buf )
            wait_que( mudconf.master_room, player, 0, NOTHING, 0,
                      buf, argv, argn, NULL );
        free_lbuf( buf );
        DOLIST( obj, Contents( mudconf.master_room ) ) {
            if( !mudconf.global_aconn_uselocks ||
                    could_doit( player, obj, A_LUSE ) ) {
                buf = atr_pget( obj, attr, &aowner,
                                &aflags, &alen );
                if( *buf ) {
                    wait_que( obj, player, 0, NOTHING, 0,
                              buf, argv, argn, NULL );
                }
                free_lbuf( buf );
            }
        }
    }

    /*
     * do the zone of the player's location's possible a(dis)connect
     */
    if( mudconf.have_zones && ( ( zone = Zone( loc ) ) != NOTHING ) ) {
        switch( Typeof( zone ) ) {
        case TYPE_THING:
            buf = atr_pget( zone, attr, &aowner, &aflags, &alen );
            if( *buf ) {
                wait_que( zone, player, 0, NOTHING, 0, buf,
                          argv, argn, NULL );
            }
            free_lbuf( buf );
            break;
        case TYPE_ROOM:
            /*
             * check every object in the room for a (dis)connect
             * * action
             */
            DOLIST( obj, Contents( zone ) ) {
                buf = atr_pget( obj, attr, &aowner, &aflags, &alen );
                if( *buf ) {
                    wait_que( obj, player, 0, NOTHING, 0, buf, argv, argn, NULL );
                }
                free_lbuf( buf );
            }
            break;
        default:
            buf = log_getname( player, "announce_connattr" );
            log_write( LOG_PROBLEMS, "OBJ", "DAMAG","Invalid zone #%d for %s has bad type %d", zone, buf, Typeof( zone ) );
            XFREE( buf, "announce_connattr" );
        }
    }

    free_sbuf( argv[1] );
    if( attr == A_ADISCONNECT ) {
        free_sbuf( argv[2] );
        free_sbuf( argv[3] );
        free_sbuf( argv[4] );
        free_sbuf( argv[5] );
        free_sbuf( argv[6] );
    }
}

static void announce_connect( dbref player, DESC *d, const char *reason ) {
    dbref loc, aowner, temp;

    int aflags, alen, num, key, count;

    char *buf, *time_str;

    DESC *dtemp;

    desc_addhash( d );

    count = 0;
    DESC_ITER_CONN( dtemp )
    count++;

    if( mudstate.record_players < count ) {
        mudstate.record_players = count;
    }

    buf = atr_pget( player, A_TIMEOUT, &aowner, &aflags, &alen );
    d->timeout = ( int ) strtol( buf, ( char ** ) NULL, 10 );
    if( d->timeout <= 0 ) {
        d->timeout = mudconf.idle_timeout;
    }
    free_lbuf( buf );

    loc = Location( player );
    s_Connected( player );

#ifdef PUEBLO_SUPPORT
    if( d->flags & DS_PUEBLOCLIENT ) {
        s_Html( player );
    }
#endif

    if( *mudconf.motd_msg ) {
        if( mudconf.ansi_colors ) {
            raw_notify( player, "\n%sMOTD:%s %s\n", ANSI_HILITE, ANSI_NORMAL, mudconf.motd_msg );
        } else {
            raw_notify( player, "\nMOTD: %s\n", mudconf.motd_msg );
        }
    }

    if( Wizard( player ) ) {
        if( *mudconf.wizmotd_msg ) {
            if( mudconf.ansi_colors ) {
                raw_notify( player, "%sWIZMOTD:%s %s\n", ANSI_HILITE, ANSI_NORMAL, mudconf.wizmotd_msg );
            } else {
                raw_notify( player, "WIZMOTD: %s\n", mudconf.wizmotd_msg );
            }
        }
        if( !( mudconf.control_flags & CF_LOGIN ) ) {
            raw_notify( player, NULL, "*** Logins are disabled.");
        }
    }

    buf = atr_get( player, A_LPAGE, &aowner, &aflags, &alen );
    if( *buf ) {
        raw_notify( player, NULL, "REMINDER: Your PAGE LOCK is set. You may be unable to receive some pages.");
    }
    if( Dark( player ) ) {
        raw_notify( player, NULL, "REMINDER: You are set DARK.");
    }
    num = 0;
    DESC_ITER_PLAYER( player, dtemp ) num++;

    /*
     * Reset vacation flag
     */
    s_Flags2( player, Flags2( player ) & ~VACATION );

    if( num < 2 ) {
        sprintf( buf, "%s has connected.", Name( player ) );

        if( Hidden( player ) ) {
            raw_broadcast( WATCHER | FLAG_WORD2,
                           ( char * ) "GAME: %s has DARK-connected.",
                           Name( player ) );
        } else {
            raw_broadcast( WATCHER | FLAG_WORD2,
                           ( char * ) "GAME: %s has connected.", Name( player ) );
        }
    } else {
        sprintf( buf, "%s has reconnected.", Name( player ) );
        raw_broadcast( WATCHER | FLAG_WORD2,
                       ( char * ) "GAME: %s has reconnected.", Name( player ) );
    }

    key = MSG_INV;
    if( ( loc != NOTHING ) && !( Hidden( player ) && Can_Hide( player ) ) ) {
        key |= ( MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST );
    }

    temp = mudstate.curr_enactor;
    mudstate.curr_enactor = player;
    notify_check( player, player, key, NULL, buf );
    free_lbuf( buf );

    CALL_ALL_MODULES( announce_connect, ( player, reason, num ) );

    if( Suspect( player ) ) {
        raw_broadcast( WIZARD, ( char * ) "[Suspect] %s has connected.",
                       Name( player ) );
    }
    if( d->host_info & H_SUSPECT ) {
        raw_broadcast( WIZARD,
                       ( char * ) "[Suspect site: %s] %s has connected.",
                       d->addr, Name( player ) );
    }

    announce_connattr( d, player, loc, reason, num, A_ACONNECT );

    time_str = ctime( &mudstate.now );
    time_str[strlen( time_str ) - 1] = '\0';
    record_login( player, 1, time_str, d->addr, d->username );

#ifdef PUEBLO_SUPPORT
    look_in( player, Location( player ),
             ( LK_SHOWEXIT | LK_OBEYTERSE | LK_SHOWVRML ) );
#else
    look_in( player, Location( player ), ( LK_SHOWEXIT | LK_OBEYTERSE ) );
#endif
    mudstate.curr_enactor = temp;
}

void announce_disconnect( dbref player, DESC *d, const char *reason ) {
    dbref loc, temp;

    int num, key;

    char *buf;

    DESC *dtemp;

    if( Suspect( player ) ) {
        raw_broadcast( WIZARD,
                       ( char * ) "[Suspect] %s has disconnected.", Name( player ) );
    }
    if( d->host_info & H_SUSPECT ) {
        raw_broadcast( WIZARD,
                       ( char * ) "[Suspect site: %s] %s has disconnected.",
                       d->addr, Name( d->player ) );
    }
    loc = Location( player );
    num = -1;
    DESC_ITER_PLAYER( player, dtemp ) num++;

    temp = mudstate.curr_enactor;
    mudstate.curr_enactor = player;

    if( num < 1 ) {
        buf = alloc_mbuf( "announce_disconnect.only" );

        sprintf( buf, "%s has disconnected.", Name( player ) );
        key = MSG_INV;
        if( ( loc != NOTHING ) && !( Hidden( player ) && Can_Hide( player ) ) )
            key |=
                ( MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST );
        notify_check( player, player, key, NULL, buf );
        free_mbuf( buf );

        raw_broadcast( WATCHER | FLAG_WORD2,
                       ( char * ) "GAME: %s has disconnected.", Name( player ) );

        /*
         * Must reset flags before we do module stuff.
         */

        c_Connected( player );
#ifdef PUEBLO_SUPPORT
        c_Html( player );
#endif
    } else {
        buf = alloc_mbuf( "announce_disconnect.partial" );
        sprintf( buf, "%s has partially disconnected.", Name( player ) );
        key = MSG_INV;
        if( ( loc != NOTHING ) && !( Hidden( player ) && Can_Hide( player ) ) )
            key |=
                ( MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST );
        notify_check( player, player, key, NULL, buf );
        raw_broadcast( WATCHER | FLAG_WORD2,
                       ( char * ) "GAME: %s has partially disconnected.",
                       Name( player ) );
        free_mbuf( buf );
    }

    CALL_ALL_MODULES( announce_disconnect, ( player, reason, num ) );

    announce_connattr( d, player, loc, reason, num, A_ADISCONNECT );

    if( num < 1 ) {
        if( d->flags & DS_AUTODARK ) {
            s_Flags( d->player, Flags( d->player ) & ~DARK );
            d->flags &= ~DS_AUTODARK;
        }

        if( Guest( player ) ) {
            s_Flags( player, Flags( player ) | DARK );
        }
    }

    mudstate.curr_enactor = temp;
    desc_delhash( d );
}

int boot_off( dbref player, char *message ) {
    DESC *d, *dnext;

    int count;

    count = 0;
    DESC_SAFEITER_PLAYER( player, d, dnext ) {
        if( message && *message ) {
            queue_rawstring( d, NULL, message );
            queue_write( d, "\r\n", 2 );
        }
        shutdownsock( d, R_BOOT );
        count++;
    }
    return count;
}

int boot_by_port( int port, int no_god, char *message ) {
    DESC *d, *dnext;

    int count;

    count = 0;
    DESC_SAFEITER_ALL( d, dnext ) {
        if( ( d->descriptor == port ) && ( !no_god || !God( d->player ) ) ) {
            if( message && *message ) {
                queue_rawstring( d, NULL, message );
                queue_write( d, "\r\n", 2 );
            }
            shutdownsock( d, R_BOOT );
            count++;
        }
    }
    return count;
}

/* ---------------------------------------------------------------------------
 * desc_reload: Reload parts of net descriptor that are based on db info.
 */

void desc_reload( dbref player ) {
    DESC *d;

    char *buf;

    dbref aowner;

    int aflags, alen;

    DESC_ITER_PLAYER( player, d ) {
        buf = atr_pget( player, A_TIMEOUT, &aowner, &aflags, &alen );
        d->timeout = ( int ) strtol( buf, ( char ** ) NULL, 10 );
        if( d->timeout <= 0 ) {
            d->timeout = mudconf.idle_timeout;
        }
        free_lbuf( buf );
    }
}

/* ---------------------------------------------------------------------------
 * fetch_idle, fetch_connect: Return smallest idle time/largest connect time
 * for a player (or -1 if not logged in)
 */

int fetch_idle( dbref target, int port_num ) {
    DESC *d;

    int result, idletime;

    result = -1;
    if( port_num < 0 ) {
        DESC_ITER_PLAYER( target, d ) {
            idletime = ( mudstate.now - d->last_time );
            if( ( result == -1 ) || ( idletime < result ) ) {
                result = idletime;
            }
        }
    } else {
        DESC_ITER_CONN( d ) {
            if( d->descriptor == port_num ) {
                idletime = ( mudstate.now - d->last_time );
                if( ( result == -1 ) || ( idletime < result ) ) {
                    result = idletime;
                }
                return result;
            }
        }
    }
    return result;
}

int fetch_connect( dbref target, int port_num ) {
    DESC *d;

    int result, conntime;

    result = -1;
    if( port_num < 0 ) {
        DESC_ITER_PLAYER( target, d ) {
            conntime = ( mudstate.now - d->connected_at );
            if( conntime > result ) {
                result = conntime;
            }
        }
    } else {
        DESC_ITER_CONN( d ) {
            if( d->descriptor == port_num ) {
                conntime = ( mudstate.now - d->connected_at );
                if( conntime > result ) {
                    result = conntime;
                }
                return result;
            }
        }
    }
    return result;
}

void check_idle( void ) {
    DESC *d, *dnext;

    time_t idletime;

    DESC_SAFEITER_ALL( d, dnext ) {
        if( d->flags & DS_CONNECTED ) {
            idletime = mudstate.now - d->last_time;
            if( ( idletime > d->timeout ) && !Can_Idle( d->player ) ) {
                queue_rawstring( d, NULL, "*** Inactivity Timeout ***\r\n" );
                shutdownsock( d, R_TIMEOUT );
            } else if( mudconf.idle_wiz_dark &&
                       ( idletime > mudconf.idle_timeout ) &&
                       Can_Idle( d->player ) &&
                       Can_Hide( d->player ) && !Hidden( d->player ) ) {
                raw_notify( d->player, NULL, "*** Inactivity AutoDark ***");
                s_Flags( d->player, Flags( d->player ) | DARK );
                d->flags |= DS_AUTODARK;
            }
        } else {
            idletime = mudstate.now - d->connected_at;
            if( idletime > mudconf.conn_timeout ) {
                queue_rawstring( d, NULL, "*** Login Timeout ***\r\n" );
                shutdownsock( d, R_TIMEOUT );
            }
        }
    }
}

static char *trimmed_name( dbref player ) {
    static char cbuff[18];

    if( strlen( Name( player ) ) <= 16 ) {
        return Name( player );
    }
    strncpy( cbuff, Name( player ), 16 );
    cbuff[16] = '\0';
    return cbuff;
}

static char *trimmed_site( char *name ) {
    static char buff[MBUF_SIZE];

    if( ( ( int ) strlen( name ) <= mudconf.site_chars )
            || ( mudconf.site_chars == 0 ) ) {
        return name;
    }
    strncpy( buff, name, mudconf.site_chars );
    buff[mudconf.site_chars + 1] = '\0';
    return buff;
}

static void dump_users( DESC *e, char *match, int key ) {
    DESC *d;

    int count;

    char *buf, *fp, *sp, flist[4], slist[4];

    dbref room_it;

    while( match && *match && isspace( *match ) ) {
        match++;
    }
    if( !match || !*match ) {
        match = NULL;
    }

#ifdef PUEBLO_SUPPORT
    if( ( e->flags & DS_PUEBLOCLIENT ) && ( Html( e->player ) ) ) {
        queue_string( e, NULL, "<pre>" );
    }
#endif

    buf = alloc_mbuf( "dump_users" );
    if( key == CMD_SESSION ) {
        queue_rawstring( e, NULL, "                               " );
        queue_rawstring( e, NULL, "     Characters Input----  Characters Output---\r\n" );
    }
    queue_rawstring( e, NULL, "Player Name        On For Idle " );
    if( key == CMD_SESSION ) {
        queue_rawstring( e, NULL, "Port Pend  Lost     Total  Pend  Lost     Total\r\n" );
    } else if( ( e->flags & DS_CONNECTED ) && ( Wizard_Who( e->player ) ) &&
               ( key == CMD_WHO ) ) {
        queue_rawstring( e, NULL, "  Room    Cmds   Host\r\n" );
    } else {
        if( Wizard_Who( e->player ) || See_Hidden( e->player ) ) {
            queue_string( e, NULL, "  " );
        } else {
            queue_string( e, NULL, " " );
        }
        queue_string( e, NULL, mudstate.doing_hdr );
        queue_string( e, NULL, "\r\n" );
    }
    count = 0;
    DESC_ITER_CONN( d ) {
        if( !Hidden( d->player ) ||
                ( ( e->flags & DS_CONNECTED ) && See_Hidden( e->player ) ) ) {
            count++;
            if( match && !( string_prefix( Name( d->player ), match ) ) ) {
                continue;
            }
            if( ( key == CMD_SESSION ) &&
                    !( Wizard_Who( e->player )
                       && ( e->flags & DS_CONNECTED ) )
                    && ( d->player != e->player ) ) {
                continue;
            }

            /*
             * Get choice flags for wizards
             */

            fp = flist;
            sp = slist;
            if( ( e->flags & DS_CONNECTED ) && Wizard_Who( e->player ) ) {
                if( Hidden( d->player ) ) {
                    if( d->flags & DS_AUTODARK ) {
                        *fp++ = 'd';
                    } else {
                        *fp++ = 'D';
                    }
                }
                if( !Findable( d->player ) ) {
                    *fp++ = 'U';
                } else {
                    room_it = where_room( d->player );
                    if( Good_obj( room_it ) ) {
                        if( Hideout( room_it ) ) {
                            *fp++ = 'u';
                        }
                    } else {
                        *fp++ = 'u';
                    }
                }
                if( Suspect( d->player ) ) {
                    *fp++ = '+';
                }
                if( d->host_info & H_FORBIDDEN ) {
                    *sp++ = 'F';
                }
                if( d->host_info & H_REGISTRATION ) {
                    *sp++ = 'R';
                }
                if( d->host_info & H_SUSPECT ) {
                    *sp++ = '+';
                }
                if( d->host_info & H_GUEST ) {
                    *sp++ = 'G';
                }
            } else if( ( e->flags & DS_CONNECTED ) &&
                       See_Hidden( e->player ) ) {
                if( Hidden( d->player ) ) {
                    if( d->flags & DS_AUTODARK ) {
                        *fp++ = 'd';
                    } else {
                        *fp++ = 'D';
                    }
                }
            }
            *fp = '\0';
            *sp = '\0';

            if( ( e->flags & DS_CONNECTED ) && Wizard_Who( e->player )
                    && ( key == CMD_WHO ) ) {
                sprintf( buf, "%-16s%9s %4s%-3s#%-6d%5d%3s%-25s\r\n", trimmed_name( d->player ), time_format_1( mudstate.now - d->connected_at ), time_format_2( mudstate.now - d->last_time ), flist, Location( d->player ), d->command_count, slist, trimmed_site( ( ( d->username[0] != '\0' ) ? tmprintf( "%s@%s", d->username, d->addr ) : d->addr ) ) );
            } else if( key == CMD_SESSION ) {
                sprintf( buf, "%-16s%9s %4s%5d%5d%6d%10d%6d%6d%10d\r\n", trimmed_name( d->player ), time_format_1( mudstate.now - d->connected_at ), time_format_2( mudstate.now - d->last_time ), d->descriptor, d->input_size, d->input_lost, d->input_tot, d->output_size, d->output_lost, d->output_tot );
            } else if( Wizard_Who( e->player ) || See_Hidden( e->player ) ) {
                sprintf( buf, "%-16s%9s %4s%-3s%s\r\n", trimmed_name( d->player ), time_format_1( mudstate.now - d->connected_at ), time_format_2( mudstate.now - d->last_time ), flist, d->doing );
            } else {
                sprintf( buf, "%-16s%9s %4s  %s\r\n", trimmed_name( d->player ), time_format_1( mudstate.now - d->connected_at ), time_format_2( mudstate.now - d->last_time ), d->doing );
            }
            queue_string( e, NULL, buf );
        }
    }

    /*
     * sometimes I like the ternary operator....
     */

    sprintf( buf, "%d Player%slogged in, %d record, %s maximum.\r\n", count, ( count == 1 ) ? " " : "s ", mudstate.record_players, ( mudconf.max_players == -1 ) ? "no" : tmprintf( "%d", mudconf.max_players ) ); 
    queue_rawstring( e, NULL, buf );

#ifdef PUEBLO_SUPPORT
    if( ( e->flags & DS_PUEBLOCLIENT ) && ( Html( e->player ) ) ) {
        queue_string( e, NULL, "</pre>" );
    }
#endif

    free_mbuf( buf );
}

static void dump_info( DESC *call_by ) {
    DESC *d;

    char *temp;

    LINKEDLIST *llp;

    int count = 0;

    queue_rawstring( call_by, NULL, "### Begin INFO 1\r\n" );

    queue_rawstring( call_by, "Name: %s\r\n", mudconf.mud_name );

    temp = ( char * ) ctime( &mudstate.start_time );
    temp[strlen( temp ) - 1] = '\0';
    queue_rawstring( call_by, "Uptime: %s\r\n", temp );

    DESC_ITER_CONN( d ) {
        if( !Hidden( d->player ) ||
                ( ( call_by->flags & DS_CONNECTED )
                  && See_Hidden( call_by->player ) ) ) {
            count++;
        }
    }
    queue_rawstring( call_by, "Connected: %d\r\n", count );

    queue_rawstring( call_by, "Size: %d\r\n", mudstate.db_top );
    queue_rawstring( call_by, "Version: %d.%d.%d.%d\r\n", mudstate.version.major, mudstate.version.minor, mudstate.version.status, mudstate.version.revision );

    for( llp = mudconf.infotext_list; llp != NULL; llp = llp->next ) {
        queue_rawstring( call_by, "%s: %s\r\n", llp->name, llp->value );
    }

    queue_rawstring( call_by, NULL, "### End INFO\r\n" );
}

/* ---------------------------------------------------------------------------
 * do_colormap: Remap ANSI colors in output.
 */

void do_colormap( dbref player, dbref cause, int key, char *fstr, char *tstr ) {
    DESC *d;

    int from_color, to_color, i, x;

    from_color = ansi_nchartab[( unsigned char ) *fstr];
    to_color = ansi_nchartab[( unsigned char ) *tstr];

    if( ( from_color < I_ANSI_BLACK ) || ( from_color >= I_ANSI_NUM ) ) {
        notify( player, "That's not a valid color to change." );
        return;
    }
    if( ( to_color < I_ANSI_BLACK ) || ( to_color >= I_ANSI_NUM ) ) {
        notify( player, "That's not a valid color to remap to." );
        return;
    }

    DESC_ITER_PLAYER( player, d ) {
        if( d->colormap ) {
            if( from_color == to_color ) {
                d->colormap[from_color - I_ANSI_BLACK] = 0;
                /*
                 * If no changes, clear colormap
                 */
                x = 0;
                for( i = 0; i < I_ANSI_NUM - I_ANSI_BLACK; i++ )
                    if( d->colormap[i] != 0 ) {
                        x++;
                    }
                if( !x ) {
                    XFREE( d->colormap, "colormap" );
                    notify( player,
                            "Colors restored to standard." );
                } else {
                    notify( player,
                            "Color restored to standard." );
                }
            } else {
                d->colormap[from_color - I_ANSI_BLACK] =
                    to_color;
                notify( player, "Color remapped." );
            }
        } else {
            if( from_color == to_color ) {
                notify( player, "No color change." );
            } else {
                d->colormap =
                    ( int * ) XCALLOC( I_ANSI_NUM - I_ANSI_BLACK,
                                       sizeof( int ), "colormap" );
                d->colormap[from_color - I_ANSI_BLACK] =
                    to_color;
                notify( player, "Color remapped." );
            }
        }
    }
}


/* ---------------------------------------------------------------------------
 * do_doing: Set the doing string that appears in the WHO report.
 * Idea from R'nice@TinyTIM.
 */

static int sane_doing( char *arg, char *buff ) {
    char *p, *bp;

    int over = 0;

    for( p = arg; *p; p++ )
        if( ( *p == '\t' ) || ( *p == '\r' ) || ( *p == '\n' ) ) {
            *p = ' ';
        }

    bp = buff;
    if( !mudconf.ansi_colors || !strchr( arg, ESC_CHAR ) ) {
        over = safe_copy_str_fn( arg, buff, &bp, DOING_LEN - 1 );
    } else {
        over = safe_copy_str_fn( arg, buff, &bp, DOING_LEN - 5 );
        strcpy( bp, ANSI_NORMAL );
    }

    return over;
}


void do_doing( dbref player, dbref cause, int key, char *arg ) {
    DESC *d;

    int foundany, over;

    over = 0;
    if( key & DOING_HEADER ) {
        if( !( Can_Poll( player ) ) ) {
            notify( player, NOPERM_MESSAGE );
            return;
        }
        if( !arg || !*arg ) {
            strcpy( mudstate.doing_hdr, "Doing" );
            over = 0;
        } else {
            over = sane_doing( arg, mudstate.doing_hdr );
        }
        if( over ) {
            notify_check( player, player ,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Warning: %d characters lost.", over );
        }
        if( !Quiet( player ) && !( key & DOING_QUIET ) ) {
            notify( player, "Set." );
        }
    } else if( key & DOING_POLL ) {
        notify_check( player, player ,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Poll: %s", mudstate.doing_hdr );
    } else {
        foundany = 0;
        DESC_ITER_PLAYER( player, d ) {
            over = sane_doing( arg, d->doing );
            foundany = 1;
        }
        if( foundany ) {
            if( over ) {
                notify_check( player, player ,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Warning: %d characters lost.", over );
            }
            if( !Quiet( player ) && !( key & DOING_QUIET ) ) {
                notify( player, "Set." );
            }
        } else {
            notify( player, "Not connected." );
        }
    }
}
/* *INDENT-OFF* */

NAMETAB logout_cmdtable[] = {
    { ( char * ) "DOING",	5,	CA_PUBLIC,	CMD_DOING},
    { ( char * ) "LOGOUT",	6,	CA_PUBLIC,	CMD_LOGOUT},
    { ( char * ) "OUTPUTPREFIX",12,	CA_PUBLIC,	CMD_PREFIX|CMD_NOxFIX},
    { ( char * ) "OUTPUTSUFFIX",12,	CA_PUBLIC,	CMD_SUFFIX|CMD_NOxFIX},
    { ( char * ) "QUIT",	4,	CA_PUBLIC,	CMD_QUIT},
    { ( char * ) "SESSION",	7,	CA_PUBLIC,	CMD_SESSION},
    { ( char * ) "WHO",		3,	CA_PUBLIC,	CMD_WHO},
    { ( char * ) "PUEBLOCLIENT", 12,	CA_PUBLIC,      CMD_PUEBLOCLIENT},
    { ( char * ) "INFO",	4,	CA_PUBLIC,	CMD_INFO},
    {NULL,			0,	0,		0}
};

/* *INDENT-ON* */

void init_logout_cmdtab( void ) {
    NAMETAB *cp;

    /*
     * Make the htab bigger than the number of entries so that we find
     * * things on the first check.  Remember that the admin can add
     * * aliases.
     */

    hashinit( &mudstate.logout_cmd_htab, 3 * HASH_FACTOR, HT_STR );
    for( cp = logout_cmdtable; cp->flag; cp++ ) {
        hashadd( cp->name, ( int * ) cp, &mudstate.logout_cmd_htab, 0 );
    }
}

static void failconn( const char *logcode, const char *logtype, const char *logreason, DESC *d, int disconnect_reason, dbref player, int filecache, char *motd_msg, char *command, char *user, char *password, char *cmdsave ) {
    char *name;

    if( player != NOTHING ) {
        name = log_getname( player, "failconn" );
        log_write( LOG_LOGIN | LOG_SECURITY, logcode, "RJCT", "[%d/%s] %s rejected to %s (%s)", d->descriptor, d->addr, logtype, name, logreason );
        XFREE( name,  "failconn" );
    } else {
        log_write( LOG_LOGIN | LOG_SECURITY, logcode, "RJCT", "[%d/%s] %s rejected to %s (%s)", d->descriptor, d->addr, logtype, user, logreason );

    }

    fcache_dump( d, filecache );
    if( *motd_msg ) {
        queue_string( d, NULL, motd_msg );
        queue_write( d, "\r\n", 2 );
    }
    free_lbuf( command );
    free_lbuf( user );
    free_lbuf( password );
    shutdownsock( d, disconnect_reason );
    mudstate.debug_cmd = cmdsave;
    return;
}

static const char *connect_fail =
    "Either that player does not exist, or has a different password.\r\n";
static const char *create_fail =
    "Either there is already a player with that name, or that name is illegal.\r\n";

static int check_connect( DESC *d, char *msg ) {
    char *command, *user, *password, *buff, *cmdsave, *name;

    dbref player, aowner;

    int aflags, alen, nplayers, reason;

    DESC *d2;

    char *p, *lname, *pname;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = ( char * ) "< check_connect >";

    /*
     * Hide the password length from SESSION
     */

    d->input_tot -= ( strlen( msg ) + 1 );

    /*
     * Crack the command apart
     */

    command = alloc_lbuf( "check_conn.cmd" );
    user = alloc_lbuf( "check_conn.user" );
    password = alloc_lbuf( "check_conn.pass" );
    parse_connect( msg, command, user, password );

    if( !strncmp( command, "co", 2 ) || !strncmp( command, "cd", 2 ) ) {
        if( ( string_prefix( user, mudconf.guest_basename ) ) && Good_obj( mudconf.guest_char ) && ( mudconf.control_flags & CF_LOGIN ) ) {
            if( ( p = make_guest( d ) ) == NULL ) {
                queue_string( d, NULL, "All guests are tied up, please try again later.\n" );
                free_lbuf( command );
                free_lbuf( user );
                free_lbuf( password );
                return 0;
            }
            strcpy( user, p );
            strcpy( password, mudconf.guest_password );
        }
        /*
         * See if this connection would exceed the max #players
         */

        if( mudconf.max_players < 0 ) {
            nplayers = mudconf.max_players - 1;
        } else {
            nplayers = 0;
            DESC_ITER_CONN( d2 )
            nplayers++;
        }

        player = connect_player( user, password, d->addr, d->username, inet_ntoa( ( d->address ).sin_addr ) );
        if( player == NOTHING ) {

            /*
             * Not a player, or wrong password
             */

            queue_rawstring( d, NULL, connect_fail );
            log_write( LOG_LOGIN | LOG_SECURITY, "CON", "BAD", "[%d/%s] Failed connect to '%s'", d->descriptor, d->addr, user );
            user[3800] = '\0';
            if( -- ( d->retries_left ) <= 0 ) {
                free_lbuf( command );
                free_lbuf( user );
                free_lbuf( password );
                shutdownsock( d, R_BADLOGIN );
                mudstate.debug_cmd = cmdsave;
                return 0;
            }
        } else if( ( ( mudconf.control_flags & CF_LOGIN ) &&
                     ( nplayers < mudconf.max_players ) ) ||
                   WizRoy( player ) || God( player ) ) {

            if( Guest( player ) ) {
                reason = R_GUEST;
            } else if( !strncmp( command, "cd", 2 ) &&
                       ( Wizard( player ) || God( player ) ) ) {
                s_Flags( player, Flags( player ) | DARK );
                reason = R_DARK;
            } else {
                reason = R_CONNECT;
            }

            /*
             * First make sure we don't have a guest from a bad host.
             */

            if( Guest( player ) && ( d->host_info & H_GUEST ) ) {
                failconn( "CON", "Connect", "Guest Site Forbidden", d, R_GAMEDOWN, player, FC_CONN_SITE, mudconf.downmotd_msg, command, user, password, cmdsave );
                return 0;
            }

            /*
             * Logins are enabled, or wiz or god
             */
            pname = log_getname( player, "check_connect" );
            if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( player ) ) {
                lname = log_getname( Location( player ), "check_connect" );
                log_write( LOG_LOGIN, "CON", "LOGIN", "[%d/%s] %s in %s %s %s", d->descriptor, d->addr, pname, lname, conn_reasons[reason], user );
                XFREE( lname, "check_connect" );
            } else {
                log_write( LOG_LOGIN, "CON", "LOGIN", "[%d/%s] %s %s %s", d->descriptor, d->addr, pname, conn_reasons[reason], user );

            }
            XFREE( pname, "check_connect" );

            d->flags |= DS_CONNECTED;
            d->connected_at = time( NULL );
            d->player = player;

            /*
             * Check to see if the player is currently running
             * * an @program. If so, drop the new descriptor into
             * * it.
             */

            DESC_ITER_PLAYER( player, d2 ) {
                if( d2->program_data != NULL ) {
                    d->program_data = d2->program_data;
                    break;
                }
            }

            /*
             * Give the player the MOTD file and the settable
             * * MOTD message(s). Use raw notifies so the
             * * player doesn't try to match on the text.
             */

            if( Guest( player ) ) {
                fcache_dump( d, FC_CONN_GUEST );
            } else {
                buff =
                    atr_get( player, A_LAST, &aowner, &aflags,
                             &alen );
                if( !*buff ) {
                    fcache_dump( d, FC_CREA_NEW );
                } else {
                    fcache_dump( d, FC_MOTD );
                }
                if( Wizard( player ) ) {
                    fcache_dump( d, FC_WIZMOTD );
                }
                free_lbuf( buff );
            }
            announce_connect( player, d, conn_messages[reason] );

            /*
             * If stuck in an @prog, show the prompt
             */

            if( d->program_data != NULL ) {
                queue_rawstring( d, NULL, "> \377\371" );
            }

        } else if( !( mudconf.control_flags & CF_LOGIN ) ) {
            failconn( "CON", "Connect", "Logins Disabled", d,
                      R_GAMEDOWN, player, FC_CONN_DOWN,
                      mudconf.downmotd_msg, command, user, password,
                      cmdsave );
            return 0;
        } else {
            failconn( "CON", "Connect", "Game Full", d,
                      R_GAMEFULL, player, FC_CONN_FULL,
                      mudconf.fullmotd_msg, command, user, password,
                      cmdsave );
            return 0;
        }
    } else if( !strncmp( command, "cr", 2 ) ) {

        reason = R_CREATE;

        /*
         * Enforce game down
         */

        if( !( mudconf.control_flags & CF_LOGIN ) ) {
            failconn( "CRE", "Create", "Logins Disabled", d,
                      R_GAMEDOWN, NOTHING, FC_CONN_DOWN,
                      mudconf.downmotd_msg, command, user, password,
                      cmdsave );
            return 0;
        }
        /*
         * Enforce max #players
         */

        if( mudconf.max_players < 0 ) {
            nplayers = mudconf.max_players;
        } else {
            nplayers = 0;
            DESC_ITER_CONN( d2 )
            nplayers++;
        }
        if( nplayers > mudconf.max_players ) {

            /*
             * Too many players on, reject the attempt
             */

            failconn( "CRE", "Create", "Game Full", d,
                      R_GAMEFULL, NOTHING, FC_CONN_FULL,
                      mudconf.fullmotd_msg, command, user, password,
                      cmdsave );
            return 0;
        }
        if( d->host_info & H_REGISTRATION ) {
            fcache_dump( d, FC_CREA_REG );
        } else {
            player = create_player( user, password, NOTHING, 0, 0 );
            if( player == NOTHING ) {
                queue_rawstring( d, NULL, create_fail );
                log_write( LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "[%d/%s] Create of '%s' failed", d->descriptor, d->addr, user );
            } else {
                name = log_getname( player, "check_connect" );
                log_write( LOG_LOGIN | LOG_PCREATES, "CON", "CREA", "[%d/%s] %s %s", d->descriptor, d->addr, conn_reasons[reason], name );
                XFREE( name, "check_connect" );
                move_object( player, ( Good_loc( mudconf.start_room ) ? mudconf. start_room : 0 ) );
                d->flags |= DS_CONNECTED;
                d->connected_at = time( NULL );
                d->player = player;
                fcache_dump( d, FC_CREA_NEW );
                announce_connect( player, d, conn_messages[R_CREATE] );
            }
        }
    } else {
        welcome_user( d );
        log_write( LOG_LOGIN | LOG_SECURITY, "CON", "BAD", "[%d/%s] Failed connect: '%s'", d->descriptor, d->addr, msg );
        msg[150] = '\0';
    }
    free_lbuf( command );
    free_lbuf( user );
    free_lbuf( password );

    mudstate.debug_cmd = cmdsave;
    return 1;
}

static void logged_out_internal( DESC *d, int key, char *arg ) {
    switch( key ) {
    case CMD_QUIT:
        shutdownsock( d, R_QUIT );
        break;
    case CMD_LOGOUT:
        shutdownsock( d, R_LOGOUT );
        break;
    case CMD_WHO:
    case CMD_DOING:
    case CMD_SESSION:
        dump_users( d, arg, key );
        break;
    case CMD_PREFIX:
        set_userstring( &d->output_prefix, arg );
        break;
    case CMD_SUFFIX:
        set_userstring( &d->output_suffix, arg );
        break;
    case CMD_INFO:
        dump_info( d );
        break;
    case CMD_PUEBLOCLIENT:
#ifdef PUEBLO_SUPPORT
        /*
         * Set the descriptor's flag
         */
        d->flags |= DS_PUEBLOCLIENT;
        /*
         * If we're already connected, set the player's flag
         */
        if( d->flags & DS_CONNECTED ) {
            s_Html( d->player );
        }
        queue_rawstring( d, NULL, mudconf.pueblo_msg );
        queue_write( d, "\r\n", 2 );
        fcache_dump( d, FC_CONN_HTML );
        log_write( LOG_LOGIN, "CON", "HTML", "[%d/%s] PuebloClient enabled.", d->descriptor, d->addr );
#else
        queue_rawstring( d, NULL, "Sorry. This MUSH does not have Pueblo support enabled.\r\n" );
#endif
        break;
    default:
        log_write( LOG_BUGS, "BUG", "PARSE", "Logged-out command with no handler: '%s'", mudstate.debug_cmd );
    }
}

void do_command( DESC *d, char *command, int first ) {
    char *arg, *cmdsave, *log_cmdbuf, *pname, *lname;

    NAMETAB *cp;

    long begin_time, used_time;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = ( char * ) "< do_command >";

    if( d->flags & DS_CONNECTED ) {
        /*
         * Normal logged-in command processing
         */
        d->command_count++;
        if( d->output_prefix ) {
            queue_string( d, NULL, d->output_prefix );
            queue_write( d, "\r\n", 2 );
        }
        mudstate.curr_player = d->player;
        mudstate.curr_enactor = d->player;
        Free_RegData( mudstate.rdata );
        mudstate.rdata = NULL;
#ifndef NO_LAG_CHECK
        begin_time = time( NULL );
#endif				/* NO_LAG_CHECK */
        mudstate.cmd_invk_ctr = 0;
        log_cmdbuf = process_command( d->player, d->player, 1, command, ( char ** ) NULL, 0 );
#ifndef NO_LAG_CHECK
        used_time = time( NULL ) - begin_time;
        if( used_time >= mudconf.max_cmdsecs ) {
            pname = log_getname( d->player, "do_command" );
            if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( d->player ) ) {
                lname = log_getname( Location( d->player ), "do_command" );
                log_write( LOG_PROBLEMS, "CMD", "CPU", "%s in %s entered command taking %ld secs: %s", pname, lname, used_time, log_cmdbuf );
                XFREE( lname, "do_command" );
            } else {
                log_write( LOG_PROBLEMS, "CMD", "CPU", "%s entered command taking %ld secs: %s", pname, used_time, log_cmdbuf );
            }
            XFREE( pname, "do_command" );
        }
#endif				/* NO_LAG_CHECK */
        mudstate.curr_cmd = ( char * ) "";
        if( d->output_suffix ) {
            queue_string( d, NULL, d->output_suffix );
            queue_write( d, "\r\n", 2 );
        }
        mudstate.debug_cmd = cmdsave;
        return;
    }

    /*
     * Login screen (logged-out) command processing.
     */

    /*
     * Split off the command from the arguments
     */
    arg = command;
    while( *arg && !isspace( *arg ) ) {
        arg++;
    }
    if( *arg ) {
        *arg++ = '\0';
    }

    /*
     * Look up the command in the logged-out command table.
     */

    cp = ( NAMETAB * ) hashfind( command, &mudstate.logout_cmd_htab );
    if( cp == NULL ) {
        /*
         * Not in the logged-out command table, so maybe a
         * * connect attempt.
         */
        if( *arg ) {
            *--arg = ' ';    /* restore nullified space */
        }
        mudstate.debug_cmd = cmdsave;
        check_connect( d, command );
        return;
    }

    /*
     * The command was in the logged-out command table.  Perform prefix
     * * and suffix processing, and invoke the command handler.
     */

    d->command_count++;
    if( !( cp->flag & CMD_NOxFIX ) ) {
        if( d->output_prefix ) {
            queue_string( d, NULL, d->output_prefix );
            queue_write( d, "\r\n", 2 );
        }
    }
    if( cp->perm != CA_PUBLIC ) {
        queue_rawstring( d, NULL, "Permission denied.\r\n" );
    } else {
        mudstate.debug_cmd = cp->name;
        logged_out_internal( d, cp->flag & CMD_MASK, arg );
    }
    /*
     * QUIT or LOGOUT will close the connection and cause the
     * * descriptor to be freed!
     */
    if( ( ( cp->flag & CMD_MASK ) != CMD_QUIT ) &&
            ( ( cp->flag & CMD_MASK ) != CMD_LOGOUT ) &&
            !( cp->flag & CMD_NOxFIX ) ) {
        if( d->output_suffix ) {
            queue_string( d, NULL, d->output_suffix );
            queue_write( d, "\r\n", 2 );
        }
    }
    mudstate.debug_cmd = cmdsave;
}

void logged_out( dbref player, dbref cause, int key, char *arg ) {
    DESC *d, *dlast;

    if( key == CMD_PUEBLOCLIENT ) {
        /*
         * PUEBLOCLIENT affects all the player's connections.
         */
        DESC_ITER_PLAYER( player, d ) {
            logged_out_internal( d, key, arg );
        }
    } else {
        /*
         * Other logged-out commands affect only the player's
         * * most recently used connection.
         */
        dlast = NULL;
        DESC_ITER_PLAYER( player, d ) {
            if( dlast == NULL || d->last_time > dlast->last_time ) {
                dlast = d;
            }
        }
        if( dlast ) {
            logged_out_internal( dlast, key, arg );
        }
    }
}

void process_commands( void ) {
    int nprocessed;

    DESC *d, *dnext;

    CBLK *t;

    char *cmdsave;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = ( char * ) "process_commands";

    do {
        nprocessed = 0;
        DESC_SAFEITER_ALL( d, dnext ) {
            if( d->quota > 0 && ( t = d->input_head ) ) {
                d->quota--;
                nprocessed++;
                d->input_head = ( CBLK * ) t->hdr.nxt;
                if( !d->input_head ) {
                    d->input_tail = NULL;
                }
                d->input_size -= ( strlen( t->cmd ) + 1 );
                log_write( LOG_KBCOMMANDS, "CMD", "KBRD", "[%d/%s] Cmd: %s", d->descriptor, d->addr, t->cmd );
                /*
                 * ignore the IDLE psuedo-command
                 */
                if( strcmp( t->cmd, ( char * ) "IDLE" ) ) {
                    d->last_time = mudstate.now;
                    if( d->program_data != NULL ) {
                        handle_prog( d, t->cmd );
                    } else {
                        do_command( d, t->cmd, 1 );
                    }
                }
                free_lbuf( t );
            }
        }
    } while( nprocessed > 0 );
    mudstate.debug_cmd = cmdsave;
}

/* ---------------------------------------------------------------------------
 * site_check: Check for site flags in a site list.
 */

int site_check( struct in_addr host, SITE *site_list ) {
    SITE *this;

    int flag = 0;

    for( this = site_list; this; this = this->next ) {
        if( ( host.s_addr & this->mask.s_addr ) == this->address.s_addr ) {
            flag |= this->flag;
        }
    }
    return flag;
}

/* --------------------------------------------------------------------------
 * list_sites: Display information in a site list
 */

#define	S_SUSPECT	1
#define	S_ACCESS	2

static const char * stat_string( int strtype, int flag ) {
    const char *str;

    switch( strtype ) {
    case S_SUSPECT:
        if( flag ) {
            str = "Suspected";
        } else {
            str = "Trusted";
        }
        break;
    case S_ACCESS:
        switch( flag ) {
        case H_FORBIDDEN:
            str = "Forbidden";
            break;
        case H_REGISTRATION:
            str = "Registration";
            break;
        case H_GUEST:
            str = "NoGuest";
            break;
        case 0:
            str = "Unrestricted";
            break;
        default:
            str = "Strange";
        }
        break;
    default:
        str = "Strange";
    }
    return str;
}

static unsigned int mask_to_prefix( unsigned int mask_num ) {
    unsigned int i, result, tmp;

    /*
     * The number of bits in the mask is equal to the number of left
     * * shifts before it becomes zero. Binary search for that number.
     */

    for( i = 16, result = 0; i && mask_num; i >>= 1 )
        if( ( tmp = ( mask_num << i ) ) ) {
            result |= i;
            mask_num = tmp;
        }

    if( mask_num ) {
        result++;
    }

    return result;
}

static void list_sites( dbref player, SITE *site_list, const char *header_txt, int stat_type ) {
    char *buff, *str, *maskaddr;

    SITE *this;

    unsigned int bits;

    buff = alloc_mbuf( "list_sites.buff" );
    sprintf( buff, "----- %s -----", header_txt );
    notify( player, buff );
    notify( player, "IP Prefix         Mask              Status" );

    for( this = site_list; this; this = this->next ) {

        str = ( char * ) stat_string( stat_type, this->flag );
        bits = mask_to_prefix( ntohl( this->mask.s_addr ) );

        /*
         * special-case 0, can't shift by 32
         */
        if( ( bits == 0 && htonl( 0 ) == this->mask.s_addr ) ||
                htonl( 0xFFFFFFFFU << ( 32 - bits ) ) == this->mask.s_addr ) {

            sprintf( buff, "%-17s /%-16d %s",
                     inet_ntoa( this->address ), bits, str );

        } else {

            /*
             * Deal with bizarre stuff not along CIDRized boundaries.
             * * inet_ntoa() points to a static buffer, so we've got to
             * * allocate something temporary.
             */
            maskaddr = alloc_mbuf( "list_sites.mask" );
            strcpy( maskaddr, inet_ntoa( this->mask ) );
            sprintf( buff, "%-17s %-17s %s",
                     inet_ntoa( this->address ), maskaddr, str );
            free_mbuf( maskaddr );
        }

        notify( player, buff );
    }

    free_mbuf( buff );
}

/* ---------------------------------------------------------------------------
 * list_siteinfo: List information about specially-marked sites.
 */

void list_siteinfo( dbref player ) {
    list_sites( player, mudstate.access_list, "Site Access", S_ACCESS );
    list_sites( player, mudstate.suspect_list, "Suspected Sites",
                S_SUSPECT );
}

/* ---------------------------------------------------------------------------
 * make_ulist: Make a list of connected user numbers for the LWHO function.
 */

void make_ulist( dbref player, char *buff, char **bufc ) {
    char *cp;

    DESC *d;

    cp = *bufc;
    DESC_ITER_CONN( d ) {
        if( !See_Hidden( player ) && Hidden( d->player ) ) {
            continue;
        }
        if( cp != *bufc ) {
            safe_chr( ' ', buff, bufc );
        }
        safe_chr( '#', buff, bufc );
        safe_ltos( buff, bufc, d->player );
    }
}

/* ---------------------------------------------------------------------------
 * make_portlist: Make a list of ports for PORTS().
 */

void make_portlist( dbref player, dbref target, char *buff, char **bufc ) {
    DESC *d;

    int i = 0;

    DESC_ITER_CONN( d ) {
        if( ( target == NOTHING ) || ( d->player == target ) ) {
            safe_str( tmprintf( "%d ", d->descriptor ), buff, bufc );
            i = 1;
        }
    }
    if( i ) {
        ( *bufc )--;
    }
    **bufc = '\0';
}

/* ---------------------------------------------------------------------------
 * make_sessioninfo: Return information about a port, for SESSION().
 * List of numbers: command_count input_tot output_tot
 */

void make_sessioninfo( dbref player, dbref target, int port_num, char *buff, char **bufc ) {
    DESC *d;

    DESC_ITER_CONN( d ) {
        if( ( d->descriptor == port_num ) || ( d->player == target ) ) {
            if( Wizard_Who( player ) || Controls( player, d->player ) ) {
                safe_str( tmprintf( "%d %d %d", d->command_count, d->input_tot, d->output_tot ), buff, bufc );
                return;
            } else {
                notify_quiet( player, NOPERM_MESSAGE );
                safe_str( ( char * ) "-1 -1 -1", buff, bufc );
                return;
            }
        }
    }

    /*
     * Not found, return error.
     */

    safe_str( ( char * ) "-1 -1 -1", buff, bufc );
}

/* ---------------------------------------------------------------------------
 * get_doing: Return the DOING string of a player.
 */

char *get_doing( dbref target, int port_num ) {
    DESC *d;

    if( port_num < 0 ) {
        DESC_ITER_PLAYER( target, d ) {
            return d->doing;
        }
    } else {
        DESC_ITER_CONN( d ) {
            if( d->descriptor == port_num ) {
                return d->doing;
            }
        }
    }
    return NULL;
}

/* ---------------------------------------------------------------------------
 * get_programmer: Get the dbref of the controlling programmer, if any.
 */

dbref get_programmer( dbref target ) {
    DESC *d;

    DESC_ITER_CONN( d ) {
        if( ( d->player == target ) && ( d->program_data != NULL ) ) {
            return ( d->program_data->wait_cause );
        }
    }
    return NOTHING;
}

/* ---------------------------------------------------------------------------
 * find_connected_name: Resolve a playername from the list of connected
 * players using prefix matching.  We only return a match if the prefix
 * was unique.
 */

dbref find_connected_name( dbref player, char *name ) {
    DESC *d;

    dbref found;

    found = NOTHING;
    DESC_ITER_CONN( d ) {
        if( Good_obj( player ) &&
                !See_Hidden( player ) && Hidden( d->player ) ) {
            continue;
        }
        if( !string_prefix( Name( d->player ), name ) ) {
            continue;
        }
        if( ( found != NOTHING ) && ( found != d->player ) ) {
            return NOTHING;
        }
        found = d->player;
    }
    return found;
}


/* ---------------------------------------------------------------------------
 * find_connected_ambiguous: Resolve a playername from the list of connected
 * players using prefix matching.  If the prefix is non-unique, we return
 * the AMBIGUOUS token; if it does not exist, we return the NOTHING token.
 * was unique.
 */

dbref find_connected_ambiguous( dbref player, char *name ) {
    DESC *d;

    dbref found;

    found = NOTHING;
    DESC_ITER_CONN( d ) {
        if( Good_obj( player ) && !See_Hidden( player )
                && Hidden( d->player ) ) {
            continue;
        }
        if( !string_prefix( Name( d->player ), name ) ) {
            continue;
        }
        if( ( found != NOTHING ) && ( found != d->player ) ) {
            return AMBIGUOUS;
        }
        found = d->player;
    }
    return found;
}
