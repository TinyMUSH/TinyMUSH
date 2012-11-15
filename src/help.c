/* help.c - commands for giving help */

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
#include "interface.h"
#include "externs.h"		/* required by code */

#include "help.h"		/* required by code */

/* Functions used to build indexes */

typedef struct _help_indx_list {
    help_indx entry;
    struct _help_indx_list *next;
} help_indx_list;


int helpmkindx_dump_entries( FILE * wfp, long pos, help_indx_list * entries ) {
    int truepos;

    int truelen;

    int depth;

    help_indx_list *prev_ep, *ep;

    /*
     *  if we have more than one entry, the one on the top of the chain
     *  is going to have the actual pos we want to use to index with
     */
    truepos = ( long ) entries->entry.pos;
    truelen = ( int )( pos - entries->entry.pos );

    prev_ep = 0;
    depth = 0;

    for( ep = entries; ep; ep = ep->next ) {
        ep->entry.pos = ( long ) truepos;
        ep->entry.len = truelen;
        if( fwrite( &ep->entry, sizeof( help_indx ), 1, wfp ) < 1 ) {
            return ( -1 );
        }

        if( prev_ep ) {
            free( prev_ep );
        }

        if( depth++ ) {	/* don't want to try to free the top of the chain */
            prev_ep = ep;
        }
    }
    /*
     *  no attempt is made to free the last remaining struct as its actually the
     *  one on the top of the chain, ie. the statically allocated struct.
     */

    return ( 0 );
}


int helpmkindx( dbref player, char *confcmd, char *helpfile ) {
    long pos;

    int i, n, lineno, ntopics, actualdata;

    char *src, *dst;

    char *s, *topic;

    char line[LINE_SIZE + 1];

    help_indx_list *entries, *ep;

    FILE *rfp, *wfp;

    src = XSTRDUP( tmprintf( "%s.txt", helpfile ), "helpmkindx_src" );
    dst = XSTRDUP( tmprintf( "%s.indx", helpfile ), "helpmkindx_src" );

    cf_log_help_mkindx( player, confcmd, "Indexing %s", basename( src ) );

    if( ( rfp = fopen( src, "r" ) ) == NULL ) {
        cf_log_help_mkindx( player, confcmd, "can't open %s for reading", src );
        return ( -1 );
    }
    if( ( wfp = fopen( dst, "w" ) ) == NULL ) {
        cf_log_help_mkindx( player, confcmd, "can't open %s for writing", dst );
        return ( -1 );
    }

    pos = 0L;
    lineno = 0;
    ntopics = 0;
    actualdata = 0;

    /*
     * create initial entry storage
     */
    entries = ( help_indx_list * ) malloc( sizeof( help_indx_list ) );
    memset( entries, 0, sizeof( help_indx_list ) );

    while( fgets( line, LINE_SIZE, rfp ) != NULL ) {
        ++lineno;

        n = strlen( line );
        if( line[n - 1] != '\n' ) {
            cf_log_help_mkindx( player, confcmd, "line %d: line too long", lineno );
        }
        if( line[0] == '&' ) {
            ++ntopics;

            if( ( ntopics > 1 ) && actualdata ) {
                /*
                 *  we've hit the next topic, time to write the ones we've been
                 *  building
                 */
                actualdata = 0;
                if( helpmkindx_dump_entries( wfp, pos, entries ) ) {
                    cf_log_help_mkindx( player, confcmd, "error writing %s", dst );
                    return ( -1 );
                }
                memset( entries, 0, sizeof( help_indx_list ) );
            }

            if( entries->entry.pos ) {
                /*
                 *  we're already working on an entry... time to start nesting
                 */
                ep = entries;
                entries =
                    ( help_indx_list * )
                    malloc( sizeof( help_indx_list ) );
                memset( entries, 0, sizeof( help_indx_list ) );
                entries->next = ep;
            }

            for( topic = &line[1];
                    ( *topic == ' ' || *topic == '\t' )
                    && *topic != '\0'; topic++ );
            for( i = -1, s = topic; *s != '\n' && *s != '\0'; s++ ) {
                if( i >= TOPIC_NAME_LEN - 1 ) {
                    break;
                }
                if( *s != ' '
                        || entries->entry.topic[i] != ' ' ) {
                    entries->entry.topic[++i] = *s;
                }
            }
            entries->entry.topic[++i] = '\0';
            entries->entry.pos = pos + ( long ) n;
        } else if( n > 1 ) {
            /*
             *  a non blank line.  we can flush entries to the .indx file the next
             *  time we run into a topic line...
             */
            actualdata = 1;
        }
        pos += n;
    }
    if( helpmkindx_dump_entries( wfp, pos, entries ) ) {
        cf_log_help_mkindx( player, confcmd, "error writing %s", dst );
        return ( -1 );
    }
    fclose( rfp );
    fclose( wfp );
    cf_log_help_mkindx( player, confcmd, "%d topics indexed", ntopics );
    return ( 0 );
}

int helpindex_read( HASHTAB *htab, char *filename ) {
    help_indx entry;

    char *p;

    int count;

    FILE *fp;

    struct help_entry *htab_entry;

    /*
     * Let's clean out our hash table, before we throw it away.
     */
    for( p = hash_firstkey( htab ); p; p = hash_nextkey( htab ) ) {
        if( !( hashfindflags( p, htab ) & HASH_ALIAS ) ) {
            htab_entry = ( struct help_entry * ) hashfind( p, htab );
            XFREE( htab_entry, "helpindex_read.hent0" );
        }
    }

    hashflush( htab, 0 );

    if( ( fp = tf_fopen( filename, O_RDONLY ) ) == NULL ) {
        log_write( LOG_PROBLEMS, "HLP", "RINDX", "Can't open %s for reading.", filename );
        return -1;
    }
    count = 0;
    while( ( fread( ( char * ) &entry, sizeof( help_indx ), 1, fp ) ) == 1 ) {

        /*
         * Lowercase the entry and add all leftmost substrings.
         * * Substrings already added will be rejected by hashadd.
         */
        for( p = entry.topic; *p; p++ ) {
            *p = tolower( *p );
        }

        htab_entry =
            ( struct help_entry * ) XMALLOC( sizeof( struct help_entry ),
                                             "helpindex_read.hent1" );

        htab_entry->pos = entry.pos;
        htab_entry->len = entry.len;

        if( hashadd( entry.topic, ( int * ) htab_entry, htab, 0 ) == 0 ) {
            count++;
            while( p > ( entry.topic + 1 ) ) {
                p--;
                *p = '\0';
                if( !isspace( * ( p - 1 ) ) ) {
                    if( hashadd( entry.topic,
                                 ( int * ) htab_entry, htab,
                                 HASH_ALIAS ) == 0 ) {
                        count++;
                    } else {
                        /*
                         * It didn't make it into the hash
                         * * table
                         */
                        break;
                    }
                }
            }
        } else {
            log_write( LOG_ALWAYS, "HLP", "RINDX", "Topic already exists: %s", entry.topic );
            XFREE( htab_entry, "helpindex_read.hent1" );
        }
    }
    tf_fclose( fp );
    hashreset( htab );
    return count;
}

void helpindex_load( dbref player ) {
    int i;

    char buf[SBUF_SIZE + 8];

    if( !mudstate.hfiletab ) {
        if( ( player != NOTHING ) && !Quiet( player ) )
            notify( player,
                    "No indexed files have been configured." );
        return;
    }

    for( i = 0; i < mudstate.helpfiles; i++ ) {
        sprintf( buf, "%s.indx", mudstate.hfiletab[i] );
        helpindex_read( &mudstate.hfile_hashes[i], buf );
    }

    if( ( player != NOTHING ) && !Quiet( player ) ) {
        notify( player, "Indexed file cache updated." );
    }
}


void helpindex_init( void ) {
    /*
     * We do not need to do hashinits here, as this will already have
     * * been done by the add_helpfile() calls.
     */

    helpindex_load( NOTHING );
}

void help_write( dbref player, char *topic, HASHTAB *htab, char *filename, int eval ) {
    FILE *fp;

    char *p, *line, *result, *str, *bp;

    int entry_offset, entry_length;

    struct help_entry *htab_entry;

    char matched;

    char *topic_list, *buffp;

    if( *topic == '\0' ) {
        topic = ( char * ) "help";
    } else
        for( p = topic; *p; p++ ) {
            *p = tolower( *p );
        }
    htab_entry = ( struct help_entry * ) hashfind( topic, htab );
    if( htab_entry ) {
        entry_offset = htab_entry->pos;
        entry_length = htab_entry->len;
    } else if( strpbrk( topic, "*?\\" ) ) {
        matched = 0;
        for( result = hash_firstkey( htab ); result != NULL;
                result = hash_nextkey( htab ) ) {
            if( !( hashfindflags( result, htab ) & HASH_ALIAS ) &&
                    quick_wild( topic, result ) ) {
                if( matched == 0 ) {
                    matched = 1;
                    topic_list = alloc_lbuf( "help_write" );
                    buffp = topic_list;
                }
                safe_str( result, topic_list, &buffp );
                safe_chr( ' ', topic_list, &buffp );
                safe_chr( ' ', topic_list, &buffp );
            }
        }
        if( matched == 0 ) {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "No entry for '%s'.", topic );
        } else {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Here are the entries which match '%s':", topic );
            *buffp = '\0';
            notify( player, topic_list );
            free_lbuf( topic_list );
        }
        return;
    } else {
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "No entry for '%s'.", topic );
        return;
    }
    if( ( fp = tf_fopen( filename, O_RDONLY ) ) == NULL ) {
        notify( player, "Sorry, that function is temporarily unavailable." );
        log_write( LOG_PROBLEMS, "HLP", "OPEN", "Can't open %s for reading.", filename );
        return;
    }
    if( fseek( fp, entry_offset, 0 ) < 0L ) {
        notify( player, "Sorry, that function is temporarily unavailable." );
        log_write( LOG_PROBLEMS, "HLP", "SEEK", "Seek error in file %s.", filename );
        tf_fclose( fp );
        return;
    }
    line = alloc_lbuf( "help_write" );
    if( eval ) {
        result = alloc_lbuf( "help_write.2" );
    }
    for( ;; ) {
        if( fgets( line, LBUF_SIZE - 1, fp ) == NULL ) {
            break;
        }
        if( line[0] == '&' ) {
            break;
        }
        for( p = line; *p != '\0'; p++ )
            if( *p == '\n' ) {
                *p = '\0';
            }
        if( eval ) {
            str = line;
            bp = result;
            exec( result, &bp, 0, player, player, EV_NO_COMPRESS | EV_FIGNORE | EV_EVAL, &str, ( char ** ) NULL, 0 );
            notify_check(player , player , MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, NULL, result);
        } else {
            notify_check(player , player , MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, NULL, line);
        }
    }
    tf_fclose( fp );
    free_lbuf( line );
    if( eval ) {
        free_lbuf( result );
    }
}

/* ---------------------------------------------------------------------------
 * help_helper: Write entry into a buffer for a function.
 */

void help_helper( dbref player, int hf_num, int eval, char *topic, char *buff, char **bufc ) {
    char tbuf[SBUF_SIZE + 8];

    char tname[LBUF_SIZE];

    char *p, *q, *line, *result, *str, *bp;

    struct help_entry *htab_entry;

    int entry_offset, entry_length, count;

    FILE *fp;

    if( hf_num >= mudstate.helpfiles ) {
        log_write( LOG_BUGS, "BUG", "HELP", "Unknown help file number: %d", hf_num );
        safe_str( ( char * ) "#-1 NOT FOUND", buff, bufc );
        return;
    }

    if( !topic || !*topic ) {
        strcpy( tname, ( char * ) "help" );
    } else {
        for( p = topic, q = tname; *p; p++, q++ ) {
            *q = tolower( *p );
        }
        *q = '\0';
    }
    htab_entry = ( struct help_entry * ) hashfind( tname,
                 &mudstate.hfile_hashes[hf_num] );
    if( !htab_entry ) {
        safe_str( ( char * ) "#-1 NOT FOUND", buff, bufc );
        return;
    }
    entry_offset = htab_entry->pos;
    entry_length = htab_entry->len;

    sprintf( tbuf, "%s.txt", mudstate.hfiletab[hf_num] );
    if( ( fp = tf_fopen( tbuf, O_RDONLY ) ) == NULL ) {
        log_write( LOG_PROBLEMS, "HLP", "OPEN", "Can't open %s for reading.", tbuf );
        safe_str( ( char * ) "#-1 ERROR", buff, bufc );
        return;
    }
    if( fseek( fp, entry_offset, 0 ) < 0L ) {
        log_write( LOG_PROBLEMS, "HLP", "SEEK", "Seek error in file %s.", tbuf );
        tf_fclose( fp );
        safe_str( ( char * ) "#-1 ERROR", buff, bufc );
        return;
    }

    line = alloc_lbuf( "help_helper" );
    if( eval ) {
        result = alloc_lbuf( "help_helper.2" );
    }
    count = 0;
    for( ;; ) {
        if( fgets( line, LBUF_SIZE - 1, fp ) == NULL ) {
            break;
        }
        if( line[0] == '&' ) {
            break;
        }
        for( p = line; *p != '\0'; p++ )
            if( *p == '\n' ) {
                *p = '\0';
            }
        if( count > 0 ) {
            safe_crlf( buff, bufc );
        }
        if( eval ) {
            str = line;
            bp = result;
            exec( result, &bp, 0, player, player,
                  EV_NO_COMPRESS | EV_FIGNORE | EV_EVAL, &str,
                  ( char ** ) NULL, 0 );
            safe_str( result, buff, bufc );
        } else {
            safe_str( line, buff, bufc );
        }
        count++;
    }
    tf_fclose( fp );
    free_lbuf( line );
    if( eval ) {
        free_lbuf( result );
    }
}

/* ---------------------------------------------------------------------------
 * do_help: display information from new-format news and help files
 */

void do_help( dbref player, dbref cause, int key, char *message ) {
    char tbuf[SBUF_SIZE + 8];

    int hf_num;

    hf_num = key & ~HELP_RAWHELP;

    if( hf_num >= mudstate.helpfiles ) {
        log_write( LOG_BUGS, "BUG", "HELP", "Unknown help file number: %d", hf_num );
        notify( player, "No such indexed file found." );
        return;
    }

    sprintf( tbuf, "%s.txt", mudstate.hfiletab[hf_num] );
    help_write( player, message, &mudstate.hfile_hashes[hf_num], tbuf,
                ( key & HELP_RAWHELP ) ? 0 : 1 );
}
