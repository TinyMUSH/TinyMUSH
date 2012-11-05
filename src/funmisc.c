/* funmisc.c - misc functions */

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
#include "externs.h"		/* required by code */
#include "interface.h"		/* required by code */

#include "functions.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "command.h"		/* required by code */
#include "match.h"		/* required by code */

extern NAMETAB indiv_attraccess_nametab[];

extern void do_pemit_list( dbref, char *, const char *, int );

extern void do_pemit( dbref, dbref, int, char *, char * );

extern void set_attr_internal( dbref, dbref, int, char *, int, char *, char ** );
extern int que_want( BQUE *, dbref, dbref );

/*
 * ---------------------------------------------------------------------------
 * fun_switch: Return value based on pattern matching (ala @switch/first)
 * fun_switchall: Similar, but ala @switch/all fun_case: Like switch(), but a
 * straight exact match instead of wildcard. NOTE: These functions expect
 * that their arguments have not been evaluated.
 */

void fun_switchall( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    int i, got_one;

    char *mbuff, *tbuff, *bp, *str, *save_token;

    /*
     * If we don't have at least 2 args, return nothing
     */

    if( nfargs < 2 ) {
        return;
    }
    /*
     * Evaluate the target in fargs[0]
     */

    mbuff = bp = alloc_lbuf( "fun_switchall" );
    str = fargs[0];
    exec( mbuff, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
          &str, cargs, ncargs );

    /*
     * Loop through the patterns looking for a match
     */

    mudstate.in_switch++;
    save_token = mudstate.switch_token;

    got_one = 0;
    for( i = 1; ( i < nfargs - 1 ) && fargs[i] && fargs[i + 1]; i += 2 ) {
        tbuff = bp = alloc_lbuf( "fun_switchall.2" );
        str = fargs[i];
        exec( tbuff, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
        if( quick_wild( tbuff, mbuff ) ) {
            got_one = 1;
            free_lbuf( tbuff );
            mudstate.switch_token = mbuff;
            str = fargs[i + 1];
            exec( buff, bufc, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
        } else {
            free_lbuf( tbuff );
        }
    }

    /*
     * If we didn't match, return the default if there is one
     */

    if( !got_one && ( i < nfargs ) && fargs[i] ) {
        mudstate.switch_token = mbuff;
        str = fargs[i];
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
    }
    free_lbuf( mbuff );
    mudstate.in_switch--;
    mudstate.switch_token = save_token;
}

void fun_switch( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    int i;

    char *mbuff, *tbuff, *bp, *str, *save_token;

    /*
     * If we don't have at least 2 args, return nothing
     */

    if( nfargs < 2 ) {
        return;
    }
    /*
     * Evaluate the target in fargs[0]
     */

    mbuff = bp = alloc_lbuf( "fun_switch" );
    str = fargs[0];
    exec( mbuff, &bp, player, caller, cause,
          EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );

    /*
     * Loop through the patterns looking for a match
     */

    mudstate.in_switch++;
    save_token = mudstate.switch_token;

    for( i = 1; ( i < nfargs - 1 ) && fargs[i] && fargs[i + 1]; i += 2 ) {
        tbuff = bp = alloc_lbuf( "fun_switch.2" );
        str = fargs[i];
        exec( tbuff, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
        if( quick_wild( tbuff, mbuff ) ) {
            free_lbuf( tbuff );
            mudstate.switch_token = mbuff;
            str = fargs[i + 1];
            exec( buff, bufc, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL,
                  &str, cargs, ncargs );
            free_lbuf( mbuff );
            mudstate.in_switch--;
            mudstate.switch_token = save_token;
            return;
        }
        free_lbuf( tbuff );
    }

    /*
     * Nope, return the default if there is one
     */

    if( ( i < nfargs ) && fargs[i] ) {
        mudstate.switch_token = mbuff;
        str = fargs[i];
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
    }
    free_lbuf( mbuff );
    mudstate.in_switch--;
    mudstate.switch_token = save_token;
}

void fun_case( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    int i;

    char *mbuff, *tbuff, *bp, *str;

    /*
     * If we don't have at least 2 args, return nothing
     */

    if( nfargs < 2 ) {
        return;
    }
    /*
     * Evaluate the target in fargs[0]
     */

    mbuff = bp = alloc_lbuf( "fun_case" );
    str = fargs[0];
    exec( mbuff, &bp, player, caller, cause,
          EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );

    /*
     * Loop through the patterns looking for an exact match
     */

    for( i = 1; ( i < nfargs - 1 ) && fargs[i] && fargs[i + 1]; i += 2 ) {
        tbuff = bp = alloc_lbuf( "fun_case.2" );
        str = fargs[i];
        exec( tbuff, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
        if( !strcmp( tbuff, mbuff ) ) {
            free_lbuf( tbuff );
            str = fargs[i + 1];
            exec( buff, bufc, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
            free_lbuf( mbuff );
            return;
        }
        free_lbuf( tbuff );
    }
    free_lbuf( mbuff );

    /*
     * Nope, return the default if there is one
     */

    if( ( i < nfargs ) && fargs[i] ) {
        str = fargs[i];
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
    }
    return;
}

void handle_ifelse( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    /*
     * This function now assumes that its arguments have not been
     * evaluated.
     */

    char *str, *mbuff, *bp, *save_token;

    int flag, n;

    char *tbuf = NULL;

    flag = Func_Flags( fargs );

    if( flag & IFELSE_DEFAULT ) {
        VaChk_Range( 1, 2 );
    } else {
        VaChk_Range( 2, 3 );
    }

    mbuff = bp = alloc_lbuf( "handle_ifelse" );
    str = fargs[0];
    exec( mbuff, &bp, player, caller, cause,
          EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );

    /*
     * We default to bool-style, but we offer the option of the MUX-style
     * nonzero -- it's true if it's not empty or zero.
     */

    if( !mbuff || !*mbuff ) {
        n = 0;
    } else if( flag & IFELSE_BOOL ) {
        /*
         * xlate() destructively modifies the string
         */
        tbuf = XSTRDUP( mbuff, "handle_ifelse.tbuf" );
        n = xlate( tbuf );
        XFREE( tbuf, "handle_ifelse.tbuf" );
    } else {
        n = !( ( ( int ) strtol( mbuff, ( char ** ) NULL, 10 ) == 0 ) && is_number( mbuff ) );
    }
    if( flag & IFELSE_FALSE ) {
        n = !n;
    }

    if( flag & IFELSE_DEFAULT ) {
        /*
         * If we got our condition, return the string, otherwise
         * return our 'else' default clause.
         */
        if( n ) {
            safe_str( mbuff, buff, bufc );
        } else {
            str = fargs[1];
            exec( buff, bufc, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
        }
        free_lbuf( mbuff );
        return;
    }
    /*
     * Not default mode: Use our condition to execute result clause
     */

    if( !n ) {
        if( nfargs != 3 ) {
            free_lbuf( mbuff );
            return;
        }
        /*
         * Do 'false' clause
         */
        str = fargs[2];
    } else {
        /*
         * Do 'true' clause
         */
        str = fargs[1];
    }

    if( flag & IFELSE_TOKEN ) {
        mudstate.in_switch++;
        save_token = mudstate.switch_token;
        mudstate.switch_token = mbuff;
    }
    exec( buff, bufc, player, caller, cause,
          EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
    free_lbuf( mbuff );

    if( flag & IFELSE_TOKEN ) {
        mudstate.in_switch--;
        mudstate.switch_token = save_token;
    }
}

/*
 * ---------------------------------------------------------------------------
 * fun_rand: Return a random number from 0 to arg1-1
 */

void fun_rand( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    int num;

    num = ( int ) strtol( fargs[0], ( char ** ) NULL, 10 );
    if( num < 1 ) {
        safe_chr( '0', buff, bufc );
    } else {
        safe_tmprintf_str( buff, bufc, "%ld", Randomize( num ) );
    }
}

/*
 * ---------------------------------------------------------------------------
 * die(<number of dice>,<sides>): Roll XdY dice. lrand(<range bottom>,<range
 * top>,<times>[,<delim>]): Generate random list.
 */

void fun_die( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    int n, die, count;

    int total = 0;

    if( !fargs[0] || !fargs[1] ) {
        safe_chr( '0', buff, bufc );
        return;
    }
    n = ( int ) strtol( fargs[0], ( char ** ) NULL, 10 );
    die = ( int ) strtol( fargs[1], ( char ** ) NULL, 10 );

    if( ( n == 0 ) || ( die <= 0 ) ) {
        safe_chr( '0', buff, bufc );
        return;
    }
    if( ( n < 1 ) || ( n > 100 ) ) {
        safe_str( "#-1 NUMBER OUT OF RANGE", buff, bufc );
        return;
    }
    for( count = 0; count < n; count++ ) {
        total += ( int ) random_range( 1, die );
    }

    safe_ltos( buff, bufc, total );
}


void fun_lrand( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    Delim osep;

    int n_times, r_bot, r_top, i;

    double n_range;

    unsigned int tmp;

    char *bb_p;

    /*
     * Special: the delim is really an output delim.
     */

    VaChk_Only_Out( 4 );

    /*
     * If we're generating no numbers, since this is a list function, we
     * return empty, rather than returning 0.
     */

    n_times = ( int ) strtol( fargs[2], ( char ** ) NULL, 10 );
    if( n_times < 1 ) {
        return;
    }
    if( n_times > LBUF_SIZE ) {
        n_times = LBUF_SIZE;
    }
    r_bot = ( int ) strtol( fargs[0], ( char ** ) NULL, 10 );
    r_top = ( int ) strtol( fargs[1], ( char ** ) NULL, 10 );

    if( r_top < r_bot ) {

        /*
         * This is an error condition. Just return an empty list. We
         * obviously can't return a random number between X and Y if
         * Y is less than X.
         */

        return;

    } else if( r_bot == r_top ) {

        /*
         * Just generate a list of n repetitions.
         */

        bb_p = *bufc;
        for( i = 0; i < n_times; i++ ) {
            if( *bufc != bb_p ) {
                print_sep( &osep, buff, bufc );
            }
            safe_ltos( buff, bufc, r_bot );
        }
        return;
    }
    /*
     * We've hit this point, we have a range. Generate a list.
     */

    n_range = ( double ) r_top - r_bot + 1;
    bb_p = *bufc;
    for( i = 0; i < n_times; i++ ) {
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        tmp = ( unsigned int ) Randomize( n_range );
        safe_ltos( buff, bufc, r_bot + tmp );
    }
}

/*
 * ---------------------------------------------------------------------------
 * fun_lnum: Return a list of numbers.
 */

#define Lnum_Place(x)	(((x) < 10) ? (2*(x)) : ((3*(x))-10))

void fun_lnum( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char tbuf[12];

    Delim osep;

    int bot, top, over, i;

    char *bb_p, *startp, *endp;

    static int lnum_init = 0;

    static char lnum_buff[290];

    if( nfargs == 0 ) {
        return;
    }
    /*
     * lnum() is special, since its single delimiter is really an output
     * delimiter.
     */
    VaChk_Out( 1, 3 );

    if( nfargs >= 2 ) {
        bot = ( int ) strtol( fargs[0], ( char ** ) NULL, 10 );
        top = ( int ) strtol( fargs[1], ( char ** ) NULL, 10 );
    } else {
        bot = 0;
        top = ( int ) strtol( fargs[0], ( char ** ) NULL, 10 );
        if( top-- < 1 ) {	/* still want to generate if arg is 1 */
            return;
        }
    }

    /*
     * We keep 0-100 pre-generated so we can do quick copies.
     */

    if( !lnum_init ) {
        strcpy( lnum_buff,
                ( char * )
                "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99" );
        lnum_init = 1;
    }
    /*
     * If it's an ascending sequence crossing from negative numbers into
     * positive, get the negative numbers out of the way first.
     */

    bb_p = *bufc;
    over = 0;
    if( ( bot < 0 ) && ( top >= 0 ) && ( osep.len == 1 ) && ( osep.str[0] == ' ' ) ) {
        while( ( bot < 0 ) && !over ) {
            if( *bufc != bb_p ) {
                print_sep( &osep, buff, bufc );
            }
            ltos( tbuf, bot );
            over = safe_str_fn( tbuf, buff, bufc );
            bot++;
        }
        if( over ) {
            return;
        }
    }
    /*
     * Copy as much out of the pre-gen as we can.
     */

    if( ( bot >= 0 ) && ( bot < 100 ) && ( top > bot ) &&
            ( osep.len == 1 ) && ( osep.str[0] == ' ' ) ) {
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        startp = lnum_buff + Lnum_Place( bot );
        if( top >= 99 ) {
            safe_str( startp, buff, bufc );
        } else {
            endp = lnum_buff + Lnum_Place( top + 1 ) - 1;
            *endp = '\0';
            safe_str( startp, buff, bufc );
            *endp = ' ';
        }
        if( top < 100 ) {
            return;
        } else {
            bot = 100;
        }
    }
    /*
     * Print a new list.
     */

    if( top == bot ) {
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        safe_ltos( buff, bufc, bot );
        return;
    } else if( top > bot ) {
        for( i = bot; ( i <= top ) && !over; i++ ) {
            if( *bufc != bb_p ) {
                print_sep( &osep, buff, bufc );
            }
            ltos( tbuf, i );
            over = safe_str_fn( tbuf, buff, bufc );
        }
    } else {
        for( i = bot; ( i >= top ) && !over; i-- ) {
            if( *bufc != bb_p ) {
                print_sep( &osep, buff, bufc );
            }
            ltos( tbuf, i );
            over = safe_str_fn( tbuf, buff, bufc );
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * fun_time: Returns nicely-formatted time.
 */

void fun_time( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *temp;

    temp = ( char * ) ctime( &mudstate.now );
    temp[strlen( temp ) - 1] = '\0';
    safe_str( temp, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_time: Seconds since 0:00 1/1/70
 */

void fun_secs( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.now );
}

/*
 * ---------------------------------------------------------------------------
 * fun_convsecs: converts seconds to time string, based off 0:00 1/1/70
 */

void fun_convsecs( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *temp;

    time_t tt;

    tt = strtol( fargs[0], ( char ** ) NULL, 10 );
    temp = ( char * ) ctime( &tt );
    temp[strlen( temp ) - 1] = '\0';
    safe_str( temp, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_convtime: converts time string to seconds, based off 0:00 1/1/70
 * additional auxiliary function and table used to parse time string, since
 * no ANSI standard function are available to do this.
 */

static const char *monthtab[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char daystab[] = {
    31, 29, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

/*
 * converts time string to a struct tm. Returns 1 on success, 0 on fail. Time
 * string format is always 24 characters long, in format Ddd Mmm DD HH:MM:SS
 * YYYY
 */

#define	get_substr(buf, p) { \
	p = strchr(buf, ' '); \
	if (p) { \
		*p++ = '\0'; \
		while (*p == ' ') p++; \
	} \
}

int do_convtime( char *str, struct tm *ttm ) {
    char *buf, *p, *q;

    int i;

    if( !str || !ttm ) {
        return 0;
    }
    while( *str == ' ' ) {
        str++;
    }
    buf = p = alloc_sbuf( "do_convtime" );	/* make a temp copy of arg */
    safe_sb_str( str, buf, &p );
    *p = '\0';

    get_substr( buf, p );	/* day-of-week or month */
    if( !p || strlen( buf ) != 3 ) {
        free_sbuf( buf );
        return 0;
    }
    for( i = 0; ( i < 12 ) && string_compare( monthtab[i], p ); i++ );
    if( i == 12 ) {
        get_substr( p, q );	/* month */
        if( !q || strlen( p ) != 3 ) {
            free_sbuf( buf );
            return 0;
        }
        for( i = 0; ( i < 12 ) && string_compare( monthtab[i], p ); i++ );
        if( i == 12 ) {
            free_sbuf( buf );
            return 0;
        }
        p = q;
    }
    ttm->tm_mon = i;

    get_substr( p, q );	/* day of month */
    if( !q || ( ttm->tm_mday = ( int ) strtol( p, ( char ** ) NULL, 10 ) ) < 1 || ttm->tm_mday > daystab[i] ) {
        free_sbuf( buf );
        return 0;
    }
    p = strchr( q, ':' );	/* hours */
    if( !p ) {
        free_sbuf( buf );
        return 0;
    }
    *p++ = '\0';
    if( ( ttm->tm_hour = ( int ) strtol( q, ( char ** ) NULL, 10 ) ) > 23 || ttm->tm_hour < 0 ) {
        free_sbuf( buf );
        return 0;
    }
    if( ttm->tm_hour == 0 ) {
        while( isspace( *q ) ) {
            q++;
        }
        if( *q != '0' ) {
            free_sbuf( buf );
            return 0;
        }
    }
    q = strchr( p, ':' );	/* minutes */
    if( !q ) {
        free_sbuf( buf );
        return 0;
    }
    *q++ = '\0';
    if( ( ttm->tm_min = ( int ) strtol( p, ( char ** ) NULL, 10 ) ) > 59 || ttm->tm_min < 0 ) {
        free_sbuf( buf );
        return 0;
    }
    if( ttm->tm_min == 0 ) {
        while( isspace( *p ) ) {
            p++;
        }
        if( *p != '0' ) {
            free_sbuf( buf );
            return 0;
        }
    }
    get_substr( q, p );	/* seconds */
    if( !p || ( ttm->tm_sec = ( int ) strtol( q, ( char ** ) NULL, 10 ) ) > 59 || ttm->tm_sec < 0 ) {
        free_sbuf( buf );
        return 0;
    }
    if( ttm->tm_sec == 0 ) {
        while( isspace( *q ) ) {
            q++;
        }
        if( *q != '0' ) {
            free_sbuf( buf );
            return 0;
        }
    }
    get_substr( p, q );	/* year */
    if( ( ttm->tm_year = ( int ) strtol( p, ( char ** ) NULL, 10 ) ) == 0 ) {
        while( isspace( *p ) ) {
            p++;
        }
        if( *p != '0' ) {
            free_sbuf( buf );
            return 0;
        }
    }
    free_sbuf( buf );
    if( ttm->tm_year > 100 ) {
        ttm->tm_year -= 1900;
    }
    if( ttm->tm_year < 0 ) {
        return 0;
    }
    /*
     * We don't whether or not it's daylight savings time.
     */
    ttm->tm_isdst = -1;

#define LEAPYEAR_1900(yr) ((yr)%400==100||((yr)%100!=0&&(yr)%4==0))
    return ( ttm->tm_mday != 29 || i != 1 || LEAPYEAR_1900( ttm->tm_year ) );
#undef LEAPYEAR_1900
}

void fun_convtime( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    struct tm *ttm;

    ttm = localtime( &mudstate.now );
    if( do_convtime( fargs[0], ttm ) ) {
        safe_ltos( buff, bufc, mktime( ttm ) );
    } else {
        safe_known_str( "-1", 2, buff, bufc );
    }
}

/*
 * ---------------------------------------------------------------------------
 * fun_timefmt: Interface to strftime().
 */

void fun_timefmt( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    time_t tt;

    struct tm *ttm;

    char str[LBUF_SIZE], tbuf[LBUF_SIZE], *tp, *p;

    int len;

    /*
     * Check number of arguments.
     */

    if( ( nfargs < 1 ) || !fargs[0] || !*fargs[0] ) {
        return;
    }
    if( nfargs == 1 ) {
        tt = mudstate.now;
    } else if( nfargs == 2 ) {
        tt = ( time_t ) strtol( fargs[1], ( char ** ) NULL, 10 );;
        if( tt < 0 ) {
            safe_str( "#-1 INVALID TIME", buff, bufc );
            return;
        }
    } else {
        safe_tmprintf_str( buff, bufc,
                           "#-1 FUNCTION (TIMEFMT) EXPECTS 1 OR 2 ARGUMENTS BUT GOT %d",
                           nfargs );
        return;
    }

    /*
     * Construct the format string. We need to convert instances of '$'
     * into percent signs for strftime(), unless we get a '$$', which we
     * treat as a literal '$'. Step on '$n' as invalid (output literal
     * '%n'), because some strftime()s use it to insert a newline.
     */

    for( tp = tbuf, p = fargs[0], len = 0;
            *p && ( len < LBUF_SIZE - 2 ); tp++, p++ ) {
        if( *p == '%' ) {
            *tp++ = '%';
            *tp = '%';
        } else if( *p == '$' ) {
            if( * ( p + 1 ) == '$' ) {
                *tp = '$';
                p++;
            } else if( * ( p + 1 ) == 'n' ) {
                *tp++ = '%';
                *tp++ = '%';
                *tp = 'n';
                p++;
            } else {
                *tp = '%';
            }
        } else {
            *tp = *p;
        }
    }
    *tp = '\0';

    /*
     * Get the time and format it. We do this using the local timezone.
     */

    ttm = localtime( &tt );
    strftime( str, LBUF_SIZE - 1, tbuf, ttm );
    safe_str( str, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_etimefmt: Format a number of seconds into a human-readable time.
 */

void fun_etimefmt( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *p, *mark, *tp;

    int raw_secs;

    int secs, mins, hours, days;

    int csecs, cmins, chours, cdays;

    int max, n, x, width;

    int hidezero, hideearly, showsuffix, clockfmt, usecap;

    char padc, timec;

    /*
     * Figure out time values
     */

    raw_secs = secs = ( int ) strtol( fargs[1], ( char ** ) NULL, 10 );
    if( secs < 0 ) {
        /*
         * Try to be semi-useful. Keep value of secs; zero out the
         * rest
         */
        mins = hours = days = 0;
    } else {
        days = secs / 86400;
        secs %= 86400;
        hours = secs / 3600;
        secs %= 3600;
        mins = secs / 60;
        secs %= 60;
    }

    /*
     * Parse and print format string
     */

    p = fargs[0];
    while( *p ) {
        if( *p == '$' ) {
            mark = p;	/* save place in case we need to go
					 * back */
            p++;
            if( !*p ) {
                safe_chr( '$', buff, bufc );
                break;
            } else if( *p == '$' ) {
                safe_chr( '$', buff, bufc );
                p++;
            } else {
                hidezero = hideearly = showsuffix = clockfmt =
                                                        usecap = 0;
                /*
                 * Optional width
                 */
                for( width = 0;
                        *p && isdigit( ( unsigned char ) *p ); p++ ) {
                    width *= 10;
                    width += *p - '0';
                }
                for( ;
                        ( *p == 'z' ) || ( *p == 'Z' ) ||
                        ( *p == 'x' ) || ( *p == 'X' ) ||
                        ( *p == 'c' ) || ( *p == 'C' ); p++ ) {
                    if( *p == 'z' ) {
                        hidezero = 1;
                    } else if( *p == 'Z' ) {
                        hideearly = 1;
                    } else if( ( *p == 'x' ) || ( *p == 'X' ) ) {
                        showsuffix = 1;
                    } else if( *p == 'c' ) {
                        clockfmt = 1;
                    } else if( *p == 'C' ) {
                        usecap = 1;
                    }
                }
                switch( *p ) {
                case 's':
                case 'S':
                    if( usecap ) {
                        n = raw_secs;
                    } else {
                        n = secs;
                    }
                    timec = 's';
                    break;
                case 'm':
                case 'M':
                    if( usecap )
                        n = mins + ( hours * 60 ) +
                            ( days * 24 * 60 );
                    else {
                        n = mins;
                    }
                    timec = 'm';
                    break;
                case 'h':
                case 'H':
                    if( usecap ) {
                        n = hours + ( days * 24 );
                    } else {
                        n = hours;
                    }
                    timec = 'h';
                    break;
                case 'd':
                case 'D':
                    n = days;
                    timec = 'd';
                    break;
                case 'a':
                case 'A':
                    /*
                     * Show the first non-zero thing
                     */
                    if( days > 0 ) {
                        n = days;
                        timec = 'd';
                    } else if( hours > 0 ) {
                        n = hours;
                        timec = 'h';
                    } else if( mins > 0 ) {
                        n = mins;
                        timec = 'm';
                    } else {
                        n = secs;
                        timec = 's';
                    }
                    break;
                default:
                    timec = ' ';
                }
                if( timec == ' ' ) {
                    while( *p && ( *p != '$' ) ) {
                        p++;
                    }
                    safe_known_str( mark, p - mark, buff,
                                    bufc );
                } else if( !clockfmt ) {
                    /*
                     * If it's 0 and we're hidezero, just
                     * hide it. If it's 0 and we're
                     * hideearly, we only hide it if we
                     * haven't got some bigger increment
                     * that's non-zero.
                     */
                    if( ( n == 0 ) &&
                            ( hidezero ||
                              ( hideearly &&
                                !( ( ( timec == 's' )
                                     && ( raw_secs > 0 ) )
                                   || ( ( timec == 'm' )
                                        && ( raw_secs >=
                                             60 ) )
                                   || ( ( timec == 'h' )
                                        && ( raw_secs >=
                                             3600 ) ) ) ) ) ) {
                        if( width > 0 ) {
                            padc =
                                isupper( *p ) ? '0' :
                                ' ';
                            if( showsuffix ) {
                                x = width + 1;
                                print_padding
                                ( x, max,
                                  padc );
                            } else {
                                print_padding
                                ( width,
                                  max, padc );
                            }
                        }
                    } else if( width > 0 ) {
                        if( isupper( *p ) ) {
                            safe_tmprintf_str( buff,
                                               bufc, "%0*d",
                                               width, n );
                        } else {
                            safe_tmprintf_str( buff,
                                               bufc, "%*d", width,
                                               n );
                        }
                        if( showsuffix ) {
                            safe_chr( timec, buff,
                                      bufc );
                        }
                    } else {
                        safe_ltos( buff, bufc, n );
                        if( showsuffix ) {
                            safe_chr( timec, buff,
                                      bufc );
                        }
                    }
                    p++;
                } else {
                    /*
                     * In clock format, we show
                     * <d>:<h>:<m>:<s>. The field
                     * specifier tells us where our
                     * division stops.
                     */
                    if( timec == 'd' ) {
                        cdays = days;
                        chours = hours;
                        cmins = mins;
                        csecs = secs;
                    } else if( timec == 'h' ) {
                        cdays = 0;
                        csecs = raw_secs;
                        chours = csecs / 3600;
                        csecs %= 3600;
                        cmins = csecs / 60;
                        csecs %= 60;
                    } else if( timec == 'm' ) {
                        cdays = chours = 0;
                        csecs = raw_secs;
                        cmins = csecs / 60;
                        csecs %= 60;
                    } else {
                        cdays = chours = cmins = 0;
                        csecs = raw_secs;
                    }
                    if( !hidezero || ( cdays != 0 ) ) {
                        safe_tmprintf_str( buff, bufc,
                                           isupper( *p ) ?
                                           "%0*d:%0*d:%0*d:%0*d" :
                                           "%*d:%*d:%*d:%*d",
                                           width, cdays,
                                           width, chours,
                                           width, cmins,
                                           width, csecs );
                    } else {
                        /*
                         * Start from the first
                         * non-zero thing
                         */
                        if( chours != 0 ) {
                            safe_tmprintf_str( buff,
                                               bufc,
                                               isupper( *p ) ?
                                               "%0*d:%0*d:%0*d" :
                                               "%*d:%*d:%*d",
                                               width, chours,
                                               width, cmins,
                                               width, csecs );
                        } else if( cmins != 0 ) {
                            safe_tmprintf_str( buff,
                                               bufc,
                                               isupper( *p ) ?
                                               "%0*d:%0*d" :
                                               "%*d:%*d", width,
                                               cmins, width,
                                               csecs );
                        } else {
                            safe_tmprintf_str( buff,
                                               bufc,
                                               isupper( *p ) ?
                                               "%0*d" : "%*d",
                                               width, csecs );
                        }
                    }
                    p++;
                }
            }
        } else {
            mark = p;
            while( *p && ( *p != '$' ) ) {
                p++;
            }
            safe_known_str( mark, p - mark, buff, bufc );
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * fun_starttime: What time did this system last reboot?
 */

void fun_starttime( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *temp;

    temp = ( char * ) ctime( &mudstate.start_time );
    temp[strlen( temp ) - 1] = '\0';
    safe_str( temp, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_restarts: How many times have we restarted?
 */

void fun_restarts( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.reboot_nums );
}

/*
 * ---------------------------------------------------------------------------
 * fun_restarttime: When did we last restart?
 */

void fun_restarttime( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *temp;

    temp = ( char * ) ctime( &mudstate.restart_time );
    temp[strlen( temp ) - 1] = '\0';
    safe_str( temp, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_version: Return the MUSH version.
 */

void fun_version( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    /* XXX To fix once the new version scheme is done */
    //safe_str(mudstate.version, buff, bufc);
    safe_str( "TinyMUSH", buff, bufc );

}

/*
 * ---------------------------------------------------------------------------
 * fun_mudname: Return the name of the mud.
 */

void fun_mudname( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_str( mudconf.mud_name, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_hasmodule: Return 1 if a module is installed, 0 if it is not.
 */

void fun_hasmodule( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    MODULE *mp;

    WALK_ALL_MODULES( mp ) {
        if( !strcasecmp( fargs[0], mp->modname ) ) {
            safe_chr( '1', buff, bufc );
            return;
        }
    }
    safe_chr( '0', buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * fun_connrecord: Get max number of simultaneous connects.
 */

void fun_connrecord( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.record_players );
}

/*
 * ---------------------------------------------------------------------------
 * State of the invocation and recursion counters.
 */

void fun_fcount( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.func_invk_ctr );
}

void fun_fdepth( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.func_nest_lev );
}

void fun_ccount( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.cmd_invk_ctr );
}

void fun_cdepth( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    safe_ltos( buff, bufc, mudstate.cmd_nest_lev );
}

/*
 * ---------------------------------------------------------------------------
 * fun_benchmark: Benchmark softcode.
 */

void fun_benchmark( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    struct timeval bt, et;

    int i, times;

    double min, max, total, ut;

    char ebuf[LBUF_SIZE], tbuf[LBUF_SIZE], *tp, *nstr, *s;

    /*
     * Evaluate our times argument
     */

    tp = nstr = alloc_lbuf( "fun_benchmark" );
    s = fargs[1];
    exec( nstr, &tp, player, caller, cause,
          EV_EVAL | EV_STRIP | EV_FCHECK, &s, cargs, ncargs );
    times = ( int ) strtol( nstr, ( char ** ) NULL, 10 );
    free_lbuf( nstr );
    if( times < 1 ) {
        safe_str( "#-1 TOO FEW TIMES", buff, bufc );
        return;
    }
    if( times > mudconf.func_invk_lim ) {
        safe_str( "#-1 TOO MANY TIMES", buff, bufc );
        return;
    }
    min = max = total = 0;

    for( i = 0; i < times; i++ ) {
        strcpy( ebuf, fargs[0] );
        s = ebuf;
        tp = tbuf;
        get_tod( &bt );
        exec( tbuf, &tp, player, caller, cause,
              EV_FCHECK | EV_STRIP | EV_EVAL, &s, cargs, ncargs );
        get_tod( &et );
        ut = ( ( et.tv_sec - bt.tv_sec ) * 1000000 ) +
             ( et.tv_usec - bt.tv_usec );
        if( ( ut < min ) || ( min == 0 ) ) {
            min = ut;
        }
        if( ut > max ) {
            max = ut;
        }
        total += ut;
        if( ( mudstate.func_invk_ctr >= mudconf.func_invk_lim ) ||
                ( Too_Much_CPU() ) ) {
            /*
             * Abort
             */
            notify( player,
                    tmprintf
                    ( "Limits exceeded at benchmark iteration %d.",
                      i + 1 ) );
            times = i + 1;
        }
    }

    safe_tmprintf_str( buff, bufc, "%.2f %.0f %.0f",
                       total / ( double ) times, min, max );
}

/*
 * ---------------------------------------------------------------------------
 * fun_s: Force substitution to occur. fun_subeval: Like s(), but don't do
 * function evaluations.
 */

void fun_s( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *str;

    str = fargs[0];
    exec( buff, bufc, player, caller, cause, EV_FIGNORE | EV_EVAL, &str,
          cargs, ncargs );
}

void fun_subeval( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    char *str;

    str = fargs[0];
    exec( buff, bufc, player, caller, cause,
          EV_NO_LOCATION | EV_NOFCHECK | EV_FIGNORE | EV_NO_COMPRESS,
          &str, ( char ** ) NULL, 0 );
}

/*------------------------------------------------------------------------
 * Side-effect functions.
 */

static int check_command( dbref player, char *name, char *buff, char **bufc, char *cargs[], int ncargs ) {
    CMDENT *cmdp;

    if( ( cmdp = ( CMDENT * ) hashfind( name, &mudstate.command_htab ) ) ) {

        /*
         * Note that these permission checks are NOT identical to the
         * ones in process_cmdent(). In particular, side-effects are
         * NOT subject to the CA_GBL_INTERP flag. This is a design
         * decision based on the concept that these are functions and
         * not commands, even though they behave like commands in
         * many respects. This is also the same reason why
         * side-effects don't trigger hooks.
         */

        if( Invalid_Objtype( player ) ||
                !Check_Cmd_Access( player, cmdp, cargs, ncargs ) ||
                ( !Builder( player ) && Protect( CA_GBL_BUILD ) &&
                  !( mudconf.control_flags & CF_BUILD ) ) ) {

            safe_noperm( buff, bufc );
            return 1;
        }
    }
    return 0;
}


void fun_link( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@link", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_link( player, cause, 0, fargs[0], fargs[1] );
}

void fun_tel( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@teleport", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_teleport( player, cause, 0, fargs[0], fargs[1] );
}

void fun_wipe( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@wipe", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_wipe( player, cause, 0, fargs[0] );
}

void fun_pemit( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@pemit", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_pemit_list( player, fargs[0], fargs[1], 0 );
}

void fun_remit( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@pemit", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_pemit_list( player, fargs[0], fargs[1], 1 );
}

void fun_oemit( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@oemit", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_pemit( player, cause, PEMIT_OEMIT, fargs[0], fargs[1] );
}

void fun_force( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( check_command( player, "@force", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_force( player, cause, FRC_NOW, fargs[0], fargs[1], cargs, ncargs );
}

void fun_trigger( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    if( nfargs < 1 ) {
        safe_str( "#-1 TOO FEW ARGUMENTS", buff, bufc );
        return;
    }
    if( check_command( player, "@trigger", buff, bufc, cargs, ncargs ) ) {
        return;
    }
    do_trigger( player, cause, TRIG_NOW, fargs[0], & ( fargs[1] ), nfargs - 1 );
}

void fun_wait( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    do_wait( player, cause, 0, fargs[0], fargs[1], cargs, ncargs );
}

void fun_command( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    CMDENT *cmdp;

    char tbuf1[1], tbuf2[1];

    char *p;

    int key;

    if( !fargs[0] || !*fargs[0] ) {
        return;
    }

    for( p = fargs[0]; *p; p++ ) {
        *p = tolower( *p );
    }

    cmdp = ( CMDENT * ) hashfind( fargs[0], &mudstate.command_htab );
    if( !cmdp ) {
        notify( player, "Command not found." );
        return;
    }
    if( Invalid_Objtype( player ) ||
            !Check_Cmd_Access( player, cmdp, cargs, ncargs ) ||
            ( !Builder( player ) && Protect( CA_GBL_BUILD ) &&
              !( mudconf.control_flags & CF_BUILD ) ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }
    if( !( cmdp->callseq & CS_FUNCTION ) || ( cmdp->callseq & CS_ADDED ) ) {
        notify( player, "Cannot call that command." );
        return;
    }
    /*
     * Strip command flags that are irrelevant.
     */

    key = cmdp->extra;
    key &= ~( SW_GOT_UNIQUE | SW_MULTIPLE | SW_NOEVAL );

    /*
     * Can't handle null args, so make sure there's something there.
     */

    tbuf1[0] = '\0';
    tbuf2[0] = '\0';

    switch( cmdp->callseq & CS_NARG_MASK ) {
    case CS_NO_ARGS:
        ( * ( cmdp->info.handler ) )( player, cause, key );
        break;
    case CS_ONE_ARG:
        ( * ( cmdp->info.handler ) )( player, cause, key,
                                      ( ( fargs[1] ) ? ( fargs[1] ) : tbuf1 ) );
        break;
    case CS_TWO_ARG:
        ( * ( cmdp->info.handler ) )( player, cause, key,
                                      ( ( fargs[1] ) ? ( fargs[1] ) : tbuf1 ),
                                      ( ( fargs[2] ) ? ( fargs[2] ) : tbuf2 ) );
        break;
    default:
        notify( player, "Invalid command handler." );
        return;
    }
}


/*------------------------------------------------------------------------
 * fun_create: Creates a room, thing or exit
 */

void fun_create( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    dbref thing;

    int cost;

    char *name;

    Delim isep;

    VaChk_Only_InPure( 3 );
    name = fargs[0];

    if( !name || !*name ) {
        safe_str( "#-1 ILLEGAL NAME", buff, bufc );
        return;
    }
    switch( isep.str[0] ) {
    case 'r':
        if( check_command( player, "@dig", buff, bufc, cargs, ncargs ) ) {
            return;
        }
        thing = create_obj( player, TYPE_ROOM, name, 0 );
        break;
    case 'e':
        if( check_command( player, "@open", buff, bufc, cargs, ncargs ) ) {
            return;
        }
        thing = create_obj( player, TYPE_EXIT, name, 0 );
        if( thing != NOTHING ) {
            s_Exits( thing, player );
            s_Next( thing, Exits( player ) );
            s_Exits( player, thing );
        }
        break;
    default:
        if( check_command( player, "@create", buff, bufc, cargs, ncargs ) ) {
            return;
        }
        if( fargs[1] && *fargs[1] ) {
            cost = ( int ) strtol( fargs[1], ( char ** ) NULL, 10 );
            if( cost < mudconf.createmin
                    || cost > mudconf.createmax ) {
                safe_str( "#-1 COST OUT OF RANGE", buff, bufc );
                return;
            }
        } else {
            cost = mudconf.createmin;
        }
        thing = create_obj( player, TYPE_THING, name, cost );
        if( thing != NOTHING ) {
            move_via_generic( thing, player, NOTHING, 0 );
            s_Home( thing, new_home( player ) );
        }
        break;
    }
    safe_dbref( buff, bufc, thing );
}

/*---------------------------------------------------------------------------
 * fun_set: sets an attribute on an object
 */

void fun_set( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    dbref thing, thing2, aowner;

    char *p, *buff2;

    int atr, atr2, aflags, alen, clear, flagvalue, could_hear;

    ATTR *attr, *attr2;

    /*
     * obj/attr form?
     */

    if( check_command( player, "@set", buff, bufc, cargs, ncargs ) ) {
        return;
    }

    if( parse_attrib( player, fargs[0], &thing, &atr, 0 ) ) {
        if( atr != NOTHING ) {

            /*
             * must specify flag name
             */

            if( !fargs[1] || !*fargs[1] ) {

                safe_str( "#-1 UNSPECIFIED PARAMETER", buff,
                          bufc );
            }
            /*
             * are we clearing?
             */

            clear = 0;
            p = fargs[1];
            if( *fargs[1] == NOT_TOKEN ) {
                p++;
                clear = 1;
            }
            /*
             * valid attribute flag?
             */

            flagvalue = search_nametab( player,
                                        indiv_attraccess_nametab, p );
            if( flagvalue < 0 ) {
                safe_str( "#-1 CAN NOT SET", buff, bufc );
                return;
            }
            /*
             * make sure attribute is present
             */

            if( !atr_get_info( thing, atr, &aowner, &aflags ) ) {
                safe_str( "#-1 ATTRIBUTE NOT PRESENT ON OBJECT",
                          buff, bufc );
                return;
            }
            /*
             * can we write to attribute?
             */

            attr = atr_num( atr );
            if( !attr || !Set_attr( player, thing, attr, aflags ) ) {
                safe_noperm( buff, bufc );
                return;
            }
            /*
             * just do it!
             */

            if( clear ) {
                aflags &= ~flagvalue;
            } else {
                aflags |= flagvalue;
            }
            could_hear = Hearer( thing );
            atr_set_flags( thing, atr, aflags );

            return;
        }
    }
    /*
     * find thing
     */

    if( ( thing = match_controlled( player, fargs[0] ) ) == NOTHING ) {
        safe_nothing( buff, bufc );
        return;
    }
    /*
     * check for attr set first
     */
    for( p = fargs[1]; *p && ( *p != ':' ); p++ );

    if( *p ) {
        *p++ = 0;
        atr = mkattr( fargs[1] );
        if( atr <= 0 ) {
            safe_str( "#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc );
            return;
        }
        attr = atr_num( atr );
        if( !attr ) {
            safe_noperm( buff, bufc );
            return;
        }
        atr_get_info( thing, atr, &aowner, &aflags );
        if( !Set_attr( player, thing, attr, aflags ) ) {
            safe_noperm( buff, bufc );
            return;
        }
        buff2 = alloc_lbuf( "fun_set" );

        /*
         * check for _
         */
        if( *p == '_' ) {
            strcpy( buff2, p + 1 );
            if( !parse_attrib( player, p + 1, &thing2, &atr2, 0 ) ||
                    ( atr == NOTHING ) ) {
                free_lbuf( buff2 );
                safe_nomatch( buff, bufc );
                return;
            }
            attr2 = atr_num( atr );
            p = buff2;
            atr_pget_str( buff2, thing2, atr2, &aowner,
                          &aflags, &alen );

            if( !attr2 ||
                    !See_attr( player, thing2, attr2, aowner, aflags ) ) {
                free_lbuf( buff2 );
                safe_noperm( buff, bufc );
                return;
            }
        }
        /*
         * set it
         */

        set_attr_internal( player, thing, atr, p, 0, buff, bufc );
        free_lbuf( buff2 );
        return;
    }
    /*
     * set/clear a flag
     */
    flag_set( thing, player, fargs[1], 0 );
}

/*---------------------------------------------------------------------------
 * fun_ps: Gets details about the queue.
 *   ps(): Lists everything on the queue by PID
 *   ps(<object or player>): Lists PIDs enqueued by object or player's stuff
 *   ps(<PID>): Results in '<PID>:<wait status> <command>'
 */

static void list_qpids( dbref player, dbref player_targ, dbref obj_targ, BQUE *queue, char *buff, char **bufc, char *bb_p ) {
    BQUE *tmp;

    for( tmp = queue; tmp; tmp = tmp->next ) {
        if( que_want( tmp, player_targ, obj_targ ) ) {
            if( *bufc != bb_p ) {
                print_sep( &SPACE_DELIM, buff, bufc );
            }
            safe_ltos( buff, bufc, tmp->pid );
        }
    }
}

void fun_ps( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs ) {
    int qpid;

    dbref player_targ, obj_targ;

    BQUE *qptr;

    ATTR *ap;

    char *bb_p;

    /*
     * Check for the PID case first.
     */

    if( fargs[0] && is_integer( fargs[0] ) ) {
        qpid = ( int ) strtol( fargs[0], ( char ** ) NULL, 10 );
        qptr = ( BQUE * ) nhashfind( qpid, &mudstate.qpid_htab );
        if( qptr == NULL ) {
            return;
        }
        if( ( qptr->waittime > 0 ) && ( Good_obj( qptr->sem ) ) ) {
            safe_tmprintf_str( buff, bufc, "#%d:#%d/%d %s",
                               qptr->player,
                               qptr->sem, qptr->waittime - mudstate.now,
                               qptr->comm );
        } else if( qptr->waittime > 0 ) {
            safe_tmprintf_str( buff, bufc, "#%d:%d %s",
                               qptr->player,
                               qptr->waittime - mudstate.now, qptr->comm );
        } else if( Good_obj( qptr->sem ) ) {
            if( qptr->attr == A_SEMAPHORE ) {
                safe_tmprintf_str( buff, bufc, "#%d:#%d %s",
                                   qptr->player, qptr->sem, qptr->comm );
            } else {
                ap = atr_num( qptr->attr );
                if( ap && ap->name ) {
                    safe_tmprintf_str( buff, bufc,
                                       "#%d:#%d/%s %s", qptr->player,
                                       qptr->sem, ap->name, qptr->comm );
                } else {
                    safe_tmprintf_str( buff, bufc,
                                       "#%d:#%d %s", qptr->player,
                                       qptr->sem, qptr->comm );
                }
            }
        } else {
            safe_tmprintf_str( buff, bufc, "#%d: %s", qptr->player,
                               qptr->comm );
        }
        return;
    }
    /*
     * We either have nothing specified, or an object or player.
     */

    if( !fargs[0] || !*fargs[0] ) {
        if( !See_Queue( player ) ) {
            return;
        }
        obj_targ = NOTHING;
        player_targ = NOTHING;
    } else {
        player_targ = Owner( player );
        if( See_Queue( player ) ) {
            obj_targ = match_thing( player, fargs[0] );
        } else {
            obj_targ = match_controlled( player, fargs[0] );
        }
        if( !Good_obj( obj_targ ) ) {
            return;
        }
        if( isPlayer( obj_targ ) ) {
            player_targ = obj_targ;
            obj_targ = NOTHING;
        }
    }

    /*
     * List all the PIDs that match.
     */

    bb_p = *bufc;
    list_qpids( player, player_targ, obj_targ, mudstate.qfirst,
                buff, bufc, bb_p );
    list_qpids( player, player_targ, obj_targ, mudstate.qlfirst,
                buff, bufc, bb_p );
    list_qpids( player, player_targ, obj_targ, mudstate.qwait,
                buff, bufc, bb_p );
    list_qpids( player, player_targ, obj_targ, mudstate.qsemfirst,
                buff, bufc, bb_p );
}
