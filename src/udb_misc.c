/* udb_misc.c - Misc support routines for unter style error management. */

/*
 * Stolen from mjr.
 *
 * Andrew Molitor, amolitor@eagle.wesleyan.edu
 */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"           /* required by mudconf */
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */

#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "interface.h"
#include "externs.h"		/* required by code */

/*
 * Log database errors
 */

void log_db_err( int obj, int attr, const char *txt ) {
    if( !mudstate.standalone ) {
        if( attr != NOTHING ) {
            log_write( LOG_ALWAYS, "DBM", "ERROR", "Could not %s object #%d attr #%d", txt, obj, attr );
        } else {
            log_write( LOG_ALWAYS, "DBM", "ERROR", "Could not %s object #%d", txt, obj );
        }
    } else {
        log_write_raw( 1, "Could not %s object #%d", txt, obj );
        if( attr != NOTHING ) {
            log_write_raw( 1, " attr #%d", attr );
        }
        log_write_raw( 1, "\n" );
    }
}

/*
 * print a series of warnings - do not exit
 */
/*
 * VARARGS
 */
void warning( char *p, ... ) {
    va_list ap;

    va_start( ap, p );

    while( 1 ) {
        if( p == ( char * ) 0 ) {
            break;
        }

        if( p == ( char * )-1 ) {
            p = ( char * ) strerror( errno );
        }

        log_write_raw( 1, "%s", p );
        p = va_arg( ap, char * );
    }
    va_end( ap );
}

/*
 * print a series of warnings - exit
 */
/*
 * VARARGS
 */
void fatal( char *p, ... ) {
    va_list ap;

    va_start( ap, p );

    while( 1 ) {
        if( p == ( char * ) 0 ) {
            break;
        }

        if( p == ( char * )-1 ) {
            p = ( char * ) strerror( errno );
        }

        log_write_raw( 1, "%s", p );
        p = va_arg( ap, char * );
    }
    va_end( ap );
    exit( 1 );
}
