/* look.c - commands which look at things */

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
#include "powers.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "command.h"		/* required by code */
#include "ansi.h"		/* required by code */

static void show_a_desc( dbref, dbref, const char * );

extern void ufun( char *, char *, int, int, int, dbref, dbref );

static int did_attr( dbref player, dbref thing, int what ) {
    /*
     * If the attribute exists, get it, notify the player, return 1.
     * * If not, return 0.
     */

    char *buff;

    buff = master_attr( player, thing, what, NULL, 0, NULL );
    if( buff ) {
        notify( player, buff );
        free_lbuf( buff );
        return 1;
    }
    return 0;
}

static void look_exits( dbref player, dbref loc, const char *exit_name ) {
    dbref thing, parent;

    char *buff, *e, *buff1, *e1;

    int foundany, lev, isdark;

    /*
     * make sure location has exits
     */

    if( !Good_obj( loc ) || !Has_exits( loc ) ) {
        return;
    }

    /*
     * Check to see if we're formatting exits in a player-specified way.
     */

    if( did_attr( player, loc, A_LEXITS_FMT ) ) {
        return;
    }

    /*
     * make sure there is at least one visible exit
     */

    foundany = 0;
    isdark = Darkened( player, loc );
    ITER_PARENTS( loc, parent, lev ) {
        if( !Has_exits( parent ) ) {
            continue;
        }
        DOLIST( thing, Exits( parent ) ) {
            if( Can_See_Exit( player, thing, isdark ) ) {
                foundany = 1;
                break;
            }
        }
        if( foundany ) {
            break;
        }
    }

    if( !foundany ) {
        return;
    }

    /*
     * Display the list of exit names
     */

    notify( player, exit_name );
    e = buff = alloc_lbuf( "look_exits" );
    e1 = buff1 = alloc_lbuf( "look_exits2" );
    ITER_PARENTS( loc, parent, lev ) {
        if( Transparent( loc ) ) {
            DOLIST( thing, Exits( parent ) ) {
                if( Can_See_Exit( player, thing, isdark ) ) {
                    e = buff;
                    safe_exit_name( thing, buff, &e );
                    if( Location( thing ) == NOTHING ) {
                        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s leads nowhere.", buff );
                    } else if( Location( thing ) == AMBIGUOUS ) {
                        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s leads somewhere.", buff );
                    } else if( Location( thing ) == HOME ) {
                        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s leads home.", buff );
                    } else if( Good_obj( Location( thing ) ) ) {
                        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s leads to %s.", buff, Name( Location ( thing ) ) );
                    } else {
                        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s leads elsewhere.", buff );
                    }
                }
            }
        } else {
            DOLIST( thing, Exits( parent ) ) {
                if( Can_See_Exit( player, thing, isdark ) ) {
                    if( buff != e )
                        safe_known_str( ( char * ) "  ", 2,
                                        buff, &e );
#ifdef PUEBLO_SUPPORT
                    if( Html( player ) ) {
                        e1 = buff1;
                        safe_exit_name( thing, buff1,
                                        &e1 );
                        safe_str( ( char * )
                                  "<a xch_cmd=\"", buff, &e );
                        /*
                         * XXX Just stripping ansi isn't really enough.
                         */
                        safe_str( strip_ansi( buff1 ),
                                  buff, &e );
                        safe_str( ( char * ) "\">", buff,
                                  &e );
                        /*
                         * XXX The exit name needs to be HTML escaped.
                         */
                        html_escape( buff1, buff, &e );
                        safe_str( ( char * ) "</a>", buff,
                                  &e );
                    } else {
#endif
                        /*
                         * Append this exit to the list
                         */
                        safe_exit_name( thing, buff,
                                        &e );
#ifdef PUEBLO_SUPPORT
                    }
#endif
                }
            }
        }
    }

#ifdef PUEBLO_SUPPORT
    if( !( Transparent( loc ) ) ) {
        if( Html( player ) ) {
            safe_chr( '\r', buff, &e );
            safe_chr( '\n', buff, &e );
            *e = '\0';
            notify_html( player, buff );
        } else {
            *e = '\0';
            notify( player, buff );
        }
    }
#else
    if( !( Transparent( loc ) ) ) {
        *e = '\0';
        notify( player, buff );
    }
#endif				/* PUEBLO_SUPPORT */

    free_lbuf( buff );
    free_lbuf( buff1 );
}

#define CONTENTS_LOCAL  0
#define CONTENTS_NESTED 1
#define CONTENTS_REMOTE 2

static void look_contents( dbref player, dbref loc, const char *contents_name, int style ) {
    dbref thing;

    dbref can_see_loc;

    char *buff;

#ifdef PUEBLO_SUPPORT
    char *html_buff, *html_cp;

    char remote_num[32];
#endif

    /*
     * Check if we're formatting contents in a player-specified way.
     */

    if( did_attr( player, loc, A_LCON_FMT ) ) {
        return;
    }

#ifdef PUEBLO_SUPPORT
    html_buff = html_cp = alloc_lbuf( "look_contents" );
#endif

    /*
     * check to see if he can see the location
     */

    can_see_loc = Sees_Always( player, loc );

    /*
     * check to see if there is anything there
     */

    DOLIST( thing, Contents( loc ) ) {
        if( Can_See( player, thing, can_see_loc ) ) {

            /*
             * something exists!  show him everything
             */

            notify( player, contents_name );
            DOLIST( thing, Contents( loc ) ) {
                if( Can_See( player, thing, can_see_loc ) ) {
                    buff =
                        unparse_object( player, thing, 1 );
#ifdef PUEBLO_SUPPORT
                    html_cp = html_buff;
                    if( Html( player ) ) {
                        safe_str( "<a xch_cmd=\"look ",
                                  html_buff, &html_cp );
                        /*
                         * XXX Just stripping ansi isn't enough.
                         */
                        switch( style ) {
                        case CONTENTS_LOCAL:
                            safe_str( PureName
                                      ( thing ), html_buff,
                                      &html_cp );
                            break;
                        case CONTENTS_NESTED:
                            safe_str( PureName
                                      ( Location( thing ) ),
                                      html_buff,
                                      &html_cp );
                            safe_str( "'s ",
                                      html_buff,
                                      &html_cp );
                            safe_str( PureName
                                      ( thing ), html_buff,
                                      &html_cp );
                            break;
                        case CONTENTS_REMOTE:
                            sprintf( remote_num,
                                     "#%d", thing );
                            safe_str( remote_num,
                                      html_buff,
                                      &html_cp );
                            break;
                        default:
                            break;
                        }
                        safe_str( "\">", html_buff,
                                  &html_cp );
                        html_escape( buff, html_buff,
                                     &html_cp );
                        safe_str( "</a>\r\n", html_buff,
                                  &html_cp );
                        *html_cp = 0;
                        notify_html( player, html_buff );
                    } else {
#endif
                        notify( player, buff );
#ifdef PUEBLO_SUPPORT
                    }
#endif
                    free_lbuf( buff );

                }
            }
            break;	/* we're done */
        }
    }
#ifdef PUEBLO_SUPPORT
    free_lbuf( html_buff );
#endif
}

static void pairs_print( dbref player, char *atext, char *buff, char **bufc ) {
    int pos, depth;

    char *str, *strbuf, *parenlist, *endp;

    static char *colors[5] = { ANSI_MAGENTA, ANSI_GREEN, ANSI_YELLOW,
                               ANSI_CYAN, ANSI_BLUE
                             };
    static char *revcolors[5] = { "m53[\033", "m23[\033", "m33[\033",
                                  "m63[\033", "m43[\033"
                                };
    static char *REVERSE_NORMAL = "m0[\033";

    static char *REVERSE_HIRED = "m13[\033m1[\033";

    str = strip_ansi( atext );
    strbuf = alloc_lbuf( "fun_parenmatch" );
    parenlist = alloc_lbuf( "fun_parenmatch" );
    endp = strbuf;
    parenlist[0] = 0;
    depth = 0;
    for( pos = 0; pos < ( int ) strlen( str ); pos++ ) {
        switch( str[pos] ) {
        case '(':
        case '{':
        case '[':
            if( str[pos - 1] != '\\' ) {
                depth++;
                parenlist[depth] = str[pos];
                safe_str( colors[depth % 5], strbuf, &endp );
                safe_chr( str[pos], strbuf, &endp );
                safe_ansi_normal( strbuf, &endp );
            } else {
                safe_chr( str[pos], strbuf, &endp );
            }
            break;
        case ']':
        case '}':
        case ')':
            /*
             * ASCII hack to check for matching parens.
             * Since parenlist[0] is 0, this also catches
             * the too many close parens bug.
             */
            if( str[pos - 1] != '\\' ) {
                if( ( parenlist[depth] & 96 ) == ( str[pos] & 96 ) ) {
                    safe_str( colors[depth % 5], strbuf,
                              &endp );
                    safe_chr( str[pos], strbuf, &endp );
                    safe_ansi_normal( strbuf, &endp );
                    depth--;
                } else {
                    safe_str( ANSI_HILITE, strbuf, &endp );
                    safe_str( ANSI_RED, strbuf, &endp );
                    safe_chr( str[pos], strbuf, &endp );
                    safe_ansi_normal( strbuf, &endp );
                    *endp = '\0';
                    safe_str( strbuf, buff, bufc );
                    safe_str( str + pos + 1, buff, bufc );
                    free_lbuf( strbuf );
                    free_lbuf( parenlist );
                    return;
                }
            } else {
                safe_chr( str[pos], strbuf, &endp );
            }
            break;
        default:
            safe_chr( str[pos], strbuf, &endp );
            break;
        }
    }

    if( depth == 0 ) {
        *endp = '\0';
        safe_str( strbuf, buff, bufc );
        free_lbuf( strbuf );
        free_lbuf( parenlist );
        return;
    }

    /*
     * If we reach this point there were too many left parens.
     * We've gotta go back.
     */

    endp = strbuf;
    parenlist[0] = 0;
    depth = 0;
    for( pos = strlen( str ) - 1; pos >= 0; pos-- ) {
        switch( str[pos] ) {
        case ']':
        case '}':
        case ')':
            depth++;
            parenlist[depth] = str[pos];
            safe_str( REVERSE_NORMAL, strbuf, &endp );
            safe_chr( str[pos], strbuf, &endp );
            safe_str( revcolors[depth % 5], strbuf, &endp );
            break;
        case '(':
        case '{':
        case '[':

            /*
             * ASCII hack to check for matching parens.
             * Since parenlist[0] is 0, this also catches
             * the too many close parens bug.
             */

            if( ( parenlist[depth] & 96 ) == ( str[pos] & 96 ) ) {
                safe_str( REVERSE_NORMAL, strbuf, &endp );
                safe_chr( str[pos], strbuf, &endp );
                safe_str( revcolors[depth % 5], strbuf, &endp );
                depth--;
            } else {
                safe_str( REVERSE_NORMAL, strbuf, &endp );
                safe_chr( str[pos], strbuf, &endp );
                safe_str( REVERSE_HIRED, strbuf, &endp );
                str[pos] = '\0';
                safe_str( str, buff, bufc );
                for( endp--; endp >= strbuf; endp-- ) {
                    safe_chr( *endp, buff, bufc );
                }
                **bufc = '\0';
                free_lbuf( strbuf );
                free_lbuf( parenlist );
                return;
            }
            break;
        default:
            safe_chr( str[pos], strbuf, &endp );
            break;
        }
    }

    /*
     * We won't get here, but what the hell.
     */

    for( endp--; endp >= strbuf; endp-- ) {
        safe_chr( *endp, buff, bufc );
    }
    **bufc = '\0';
    free_lbuf( strbuf );
    free_lbuf( parenlist );
}

static void pretty_format( char *dest, char **cp, char *p ) {
    /*
     * Pretty-print an attribute into a buffer (assumed to be an lbuf).
     */

    int indent_lev, i;

    indent_lev = 0;
    safe_crlf( dest, cp );

    while( *p ) {
        switch( *p ) {
        case '\\':
            /*
             * Skip escaped chars
             */
            safe_chr( *p, dest, cp );
            p++;
            safe_chr( *p, dest, cp );
            if( !*p ) {
                return;    /* we're done */
            }
            break;
        case '{':
            safe_crlf( dest, cp );
            for( i = 0; i < indent_lev; i++ ) {
                safe_str( ( char * ) INDENT_STR, dest, cp );
            }
            safe_chr( *p, dest, cp );
            safe_crlf( dest, cp );
            indent_lev++;
            for( i = 0; i < indent_lev; i++ ) {
                safe_str( ( char * ) INDENT_STR, dest, cp );
            }
            while( p[1] == ' ' ) {
                p++;
            }
            break;
        case '}':
            if( indent_lev > 0 ) {
                indent_lev--;
            }
            safe_crlf( dest, cp );
            for( i = 0; i < indent_lev; i++ ) {
                safe_str( ( char * ) INDENT_STR, dest, cp );
            }
            safe_chr( *p, dest, cp );
            safe_crlf( dest, cp );
            for( i = 0; i < indent_lev; i++ ) {
                safe_str( ( char * ) INDENT_STR, dest, cp );
            }
            while( p[1] == ' ' ) {
                p++;
            }
            break;
        case ';':
            safe_chr( *p, dest, cp );
            safe_crlf( dest, cp );
            for( i = 0; i < indent_lev; i++ ) {
                safe_str( ( char * ) INDENT_STR, dest, cp );
            }
            while( p[1] == ' ' ) {
                p++;
            }
            break;
        default:
            safe_chr( *p, dest, cp );
            break;
        }
        p++;
    }
    if( * ( *cp - 1 ) != '\n' ) {
        safe_crlf( dest, cp );
    }
}

static void pretty_print( char *dest, char *name, char *text ) {
    char *cp, *p, *word;

    cp = dest;
    p = text;

    safe_str( name, dest, &cp );

    /*
     * Pretty-print contents of text into dest.
     */

    switch( *text ) {
    case '$':
    case '^':
        /*
         * Do:  $command:<text to format>
         * * Nibble up the first bit then fall through to format the rest.
         */
        while( *p && ( *p != ':' ) ) {
            safe_chr( *p, dest, &cp );
            p++;
        }
        /*
         * Do the ':'
         */
        if( *p == ':' ) {
            safe_chr( ':', dest, &cp );
            do {
                p++;
            } while( isspace( *p ) );
        } else {
            return;
        }
        /*
         * FALLTHRU
         */

    case '@':
    case '&':
        /*
         * Normal formatting
         */
        pretty_format( dest, &cp, p );
        break;

    case '#':
        /*
         * Special case: If the first word starts with #, there is a
         * * second word, and it does NOT start with a #, this is a
         * * @force command.
         */
        word = p;
        while( *word && !isspace( *word ) ) {
            word++;
        }
        while( *word && isspace( *word ) ) {
            word++;
        }
        if( !*word || ( *word == '#' ) ) {
            /*
             * This is a list of dbrefs, probably. Bail.
             */
            safe_str( p, dest, &cp );
            return;
        }
        pretty_format( dest, &cp, p );
        break;

    default:
        /*
         * Ordinary text
         */
        safe_str( p, dest, &cp );
    }
    if( ( cp - 1 ) && ( * ( cp - 1 ) != '\n' ) ) {
        safe_crlf( dest, &cp );
    }
    safe_chr( '-', dest, &cp );
}


static void view_atr( dbref player, dbref thing, ATTR *ap, char *raw_text, dbref aowner, int aflags, int skip_tag, int is_special ) {
    char *text, *buf, *bp, *name_buf, *bb_p;

    char xbuf[16], gbuf[16];	/* larger than number of attr flags! */

    char flag_buf[32];

    char *xbufp, *gbufp, *fbp;

    BOOLEXP *bool;

    if( ap->flags & AF_IS_LOCK ) {
        bool = parse_boolexp( player, raw_text, 1 );
        text = unparse_boolexp( player, bool );
        free_boolexp( bool );
    } else if( aflags & AF_STRUCTURE ) {
        text = replace_string( GENERIC_STRUCT_STRDELIM,
                               mudconf.struct_dstr, raw_text );
    } else {
        text = raw_text;
    }

    /*
     * If we don't control the object or own the attribute, hide the
     * * attr owner and flag info.
     */

    if( !Controls( player, thing ) && ( Owner( player ) != aowner ) ) {
        if( skip_tag && ( ap->number == A_DESC ) ) {
            buf = text;
            notify( player, buf );
        } else {
            if( is_special == 0 ) {
                buf = tmprintf( "%s%s:%s %s", ANSI_HILITE, ap->name, ANSI_NORMAL, text );
                notify( player, buf );
            } else if( is_special == 1 ) {
                buf = alloc_lbuf( "view_atr.pretty" );
                pretty_print( buf, tmprintf( "%s%s:%s ", ANSI_HILITE, ap->name, ANSI_NORMAL ), text );
                notify( player, buf );
                free_lbuf( buf );
            } else {
                buf = alloc_lbuf( "view_atr.pairs" );
                bp = buf;
                safe_sprintf( buf, &bp, "%s%s:%s ", ANSI_HILITE, ap->name, ANSI_NORMAL );
                pairs_print( player, text, buf, &bp );
                *bp = '\0';
                notify( player, buf );
                free_lbuf( buf );
            }
            return;
        }
    }

    /*
     * Generate flags
     */

    Print_Attr_Flags( aflags, xbuf, xbufp );
    Print_Attr_Flags( ap->flags, gbuf, gbufp );

    fbp = xbuf;
    if( *xbuf && *gbuf ) {
        sprintf( flag_buf, "%s(%s)", xbuf, gbuf );
        fbp = flag_buf;
    } else if( *gbuf ) {
        sprintf( flag_buf, "(%s)", gbuf );
        fbp = flag_buf;
    }

    if( is_special == 1 ) {
        if( ( aowner != Owner( thing ) ) && ( aowner != NOTHING ) ) {
            name_buf = tmprintf( "%s%s [#%d%s]:%s ", ANSI_HILITE, ap->name, aowner, fbp, ANSI_NORMAL );
        } else if( *fbp ) {
            name_buf = tmprintf( "%s%s [%s]:%s ", ANSI_HILITE, ap->name, fbp, ANSI_NORMAL );
        } else if( !skip_tag || ( ap->number != A_DESC ) ) {
            name_buf = tmprintf( "%s%s:%s ", ANSI_HILITE, ap->name, ANSI_NORMAL );
        } else {
            name_buf = ( char * ) "";
            buf = text;
        }
        buf = alloc_lbuf( "view_atr.pretty_print" );
        pretty_print( buf, name_buf, text );
        notify( player, buf );
        free_lbuf( buf );
    } else if( is_special == 2 ) {
        buf = alloc_lbuf( "view_atr.pairs_print" );
        bb_p = buf;
        if( ( aowner != Owner( thing ) ) && ( aowner != NOTHING ) ) {
            safe_sprintf( buf, &bb_p, "%s%s [#%d%s]:%s ", ANSI_HILITE, ap->name, aowner, fbp, ANSI_NORMAL );
        } else if( *fbp ) {
            safe_sprintf( buf, &bb_p, "%s%s [%s]:%s ", ANSI_HILITE, ap->name, fbp, ANSI_NORMAL );
        } else if( !skip_tag || ( ap->number != A_DESC ) ) {
            safe_sprintf( buf, &bb_p, "%s%s:%s ", ANSI_HILITE, ap->name, ANSI_NORMAL );
        } else {
            /*
             * Just fine the way it is
             */
        }
        pairs_print( player, text, buf, &bb_p );
        *bb_p = '\0';
        notify( player, buf );
        free_lbuf( buf );
    } else {
        if( ( aowner != Owner( thing ) ) && ( aowner != NOTHING ) ) {
            buf = tmprintf( "%s%s [#%d%s]:%s %s", ANSI_HILITE, ap->name, aowner, fbp, ANSI_NORMAL, text );
        } else if( *fbp ) {
            buf = tmprintf( "%s%s [%s]:%s %s", ANSI_HILITE, ap->name, fbp, ANSI_NORMAL, text );
        } else if( !skip_tag || ( ap->number != A_DESC ) ) {
            buf = tmprintf( "%s%s:%s %s", ANSI_HILITE, ap->name, ANSI_NORMAL, text );
        } else {
            buf = text;
        }
        notify( player, buf );
    }

    if( ( aflags & AF_STRUCTURE ) && text ) {
        free_lbuf( text );
    }
}

static void look_atrs1( dbref player, dbref thing, dbref othing, int check_exclude, int hash_insert, int is_special ) {
    dbref aowner;

    int ca, aflags, alen;

    ATTR *attr, *cattr;

    char *as, *buf;

    cattr = ( ATTR * ) XMALLOC( sizeof( ATTR ), "look_atrs1" );
    for( ca = atr_head( thing, &as ); ca; ca = atr_next( &as ) ) {
        if( ( ca == A_DESC ) || ( ca == A_LOCK ) ) {
            continue;
        }
        attr = atr_num( ca );
        if( !attr ) {
            continue;
        }

        memcpy( ( char * ) cattr, ( char * ) attr, sizeof( ATTR ) );

        /*
         * Should we exclude this attr?
         * * We have a couple of things we exclude:
         * * Attributes explicitly marked no_inherit.
         * * Locks. Note that UseLock is NOT, strictly speaking, an
         * *   inherited lock, since it's just checked when the child
         * *   tries to inherit $commands from the parent; the child
         * *   itself doesn't acquire the parent's uselock.
         * * Attributes already slurped by upper-level objects.
         */

        if( check_exclude &&
                ( ( cattr->flags & AF_PRIVATE ) ||
                  ( cattr->flags & AF_IS_LOCK ) ||
                  nhashfind( ca, &mudstate.parent_htab ) ) ) {
            continue;
        }

        buf = atr_get( thing, ca, &aowner, &aflags, &alen );
        if( Read_attr_all( player, othing, cattr, aowner, aflags, 1 ) ) {

            if( !( check_exclude && ( aflags & AF_PRIVATE ) ) ) {
                if( hash_insert )
                    nhashadd( ca, ( int * ) cattr,
                              &mudstate.parent_htab );
                view_atr( player, thing, cattr, buf,
                          aowner, aflags, 0, is_special );
            }
        }
        free_lbuf( buf );
    }
    XFREE( cattr, "look_atrs1" );
}

static void look_atrs( dbref player, dbref thing, int check_parents, int is_special ) {
    dbref parent;

    int lev, check_exclude, hash_insert;

    if( !check_parents ) {
        look_atrs1( player, thing, thing, 0, 0, is_special );
    } else {
        hash_insert = 1;
        check_exclude = 0;
        nhashflush( &mudstate.parent_htab, 0 );
        ITER_PARENTS( thing, parent, lev ) {
            if( !Good_obj( Parent( parent ) ) ) {
                hash_insert = 0;
            }
            look_atrs1( player, parent, thing,
                        check_exclude, hash_insert, is_special );
            check_exclude = 1;
        }
    }
}

static void look_simple( dbref player, dbref thing, int obey_terse ) {
    char *buff;

    /*
     * Only makes sense for things that can hear
     */

    if( !Hearer( player ) ) {
        return;
    }

    /*
     * Get the name and db-number if we can examine it.
     */

    if( Examinable( player, thing ) ) {
        buff = unparse_object( player, thing, 1 );
        notify( player, buff );
        free_lbuf( buff );
    }

    if( obey_terse && Terse( player ) )
        did_it( player, thing, A_NULL, "You see nothing special.",
                A_ODESC, NULL, A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
    else
#ifndef PUEBLO_SUPPORT
        did_it( player, thing, A_DESC, "You see nothing special.",
                A_ODESC, NULL, A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
#else
        show_a_desc( player, thing, "You see nothing special." );
#endif

    if( !mudconf.quiet_look && ( !Terse( player ) || mudconf.terse_look ) ) {
        look_atrs( player, thing, 0, 0 );
    }
}

#ifdef PUEBLO_SUPPORT
static void show_a_desc( dbref player, dbref loc, const char *msg ) {
    char *got2;

    dbref aowner;

    int aflags, alen, indent = 0;

    indent = ( isRoom( loc ) && mudconf.indent_desc
               && atr_get_raw( loc, A_DESC ) );

    if( Html( player ) ) {
        got2 = atr_pget( loc, A_HTDESC, &aowner, &aflags, &alen );
        if( *got2 )
            did_it( player, loc, A_HTDESC, msg, A_ODESC, NULL,
                    A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
        else {
            if( indent ) {
                raw_notify_newline( player );
            }
            did_it( player, loc, A_DESC, msg, A_ODESC, NULL,
                    A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
            if( indent ) {
                raw_notify_newline( player );
            }
        }
        free_lbuf( got2 );
    } else {
        if( indent ) {
            raw_notify_newline( player );
        }
        did_it( player, loc, A_DESC, msg, A_ODESC, NULL,
                A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
        if( indent ) {
            raw_notify_newline( player );
        }
    }
}
#endif

static void show_desc( dbref player, dbref loc, int key ) {
    char *got;

    dbref aowner;

    int aflags, alen, indent = 0;

    indent = ( isRoom( loc ) && mudconf.indent_desc
               && atr_get_raw( loc, A_DESC ) );

    if( ( key & LK_OBEYTERSE ) && Terse( player ) )
        did_it( player, loc, A_NULL, NULL, A_ODESC, NULL,
                A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
    else if( ( Typeof( loc ) != TYPE_ROOM ) && ( key & LK_IDESC ) ) {
        if( * ( got = atr_pget( loc, A_IDESC, &aowner, &aflags, &alen ) ) )
            did_it( player, loc, A_IDESC, NULL, A_ODESC, NULL,
                    A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
        else {
#ifdef PUEBLO_SUPPORT
            show_a_desc( player, loc, NULL );
#else
            if( indent ) {
                raw_notify_newline( player );
            }
            did_it( player, loc, A_DESC, NULL, A_ODESC, NULL,
                    A_ADESC, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
            if( indent ) {
                raw_notify_newline( player );
            }
#endif
        }
        free_lbuf( got );
    } else {
#ifdef PUEBLO_SUPPORT
        show_a_desc( player, loc, NULL );
#else
        if( indent ) {
            raw_notify_newline( player );
        }
        did_it( player, loc, A_DESC, NULL, A_ODESC, NULL, A_ADESC, 0,
                ( char ** ) NULL, 0, MSG_PRESENCE );
        if( indent ) {
            raw_notify_newline( player );
        }
#endif
    }
}

void look_in( dbref player, dbref loc, int key ) {
    int pattr, oattr, aattr, is_terse, showkey;

    char *buff;

    is_terse = ( key & LK_OBEYTERSE ) ? Terse( player ) : 0;

    /*
     * Only makes sense for things that can hear
     */

    if( !Hearer( player ) ) {
        return;
    }

#ifdef PUEBLO_SUPPORT
    /*
     * If he needs the VRML URL, send it:
     */
    if( key & LK_SHOWVRML ) {
        show_vrml_url( player, loc );
    }
#endif

    /*
     * If we can't format it in a player-specified way, then show
     * * the name (and unparse info, if relevant). We only invoke
     * * the Pueblo formatting if we weren't given a @nameformat.
     */

    if( !did_attr( player, loc, A_NAME_FMT ) ) {
        buff = unparse_object( player, loc, 1 );
#ifdef PUEBLO_SUPPORT
        if( Html( player ) ) {
            notify_html( player, "<center><h3>" );
            notify( player, buff );
            notify_html( player, "</h3></center>" );
        } else {
            notify( player, buff );
        }
#else
        notify( player, buff );
#endif
        free_lbuf( buff );
    }

    if( !Good_obj( loc ) ) {
        return;
    }		/* If we went to NOTHING et al, skip the
				 * rest */

    /*
     * tell him the description
     */

    showkey = 0;
    if( loc == Location( player ) ) {
        showkey |= LK_IDESC;
    }
    if( key & LK_OBEYTERSE ) {
        showkey |= LK_OBEYTERSE;
    }
    show_desc( player, loc, showkey );

    /*
     * tell him the appropriate messages if he has the key
     */

    if( Typeof( loc ) == TYPE_ROOM ) {
        if( could_doit( player, loc, A_LOCK ) ) {
            pattr = A_SUCC;
            oattr = A_OSUCC;
            aattr = A_ASUCC;
        } else {
            pattr = A_FAIL;
            oattr = A_OFAIL;
            aattr = A_AFAIL;
        }
        if( is_terse ) {
            pattr = 0;
        }
        did_it( player, loc, pattr, NULL, oattr, NULL,
                aattr, 0, ( char ** ) NULL, 0, MSG_PRESENCE );
    }
    /*
     * tell him the attributes, contents and exits
     */

    if( ( key & LK_SHOWATTR ) && !mudconf.quiet_look && !is_terse ) {
        look_atrs( player, loc, 0, 0 );
    }
    if( !is_terse || mudconf.terse_contents ) {
        look_contents( player, loc, "Contents:", CONTENTS_LOCAL );
    }
    if( ( key & LK_SHOWEXIT ) && ( !is_terse || mudconf.terse_exits ) ) {
        look_exits( player, loc, "Obvious exits:" );
    }
}

static void look_here( dbref player, dbref thing, int key, int look_key ) {
    if( Good_obj( thing ) ) {
        if( key & LOOK_OUTSIDE ) {
            if( ( isRoom( thing ) ) || Opaque( thing ) ) {
                notify_quiet( player,
                              "You can't look outside." );
                return;
            }
            thing = Location( thing );
        }
        look_in( player, thing, look_key );
    }
}

void do_look( dbref player, dbref cause, int key, char *name ) {
    dbref thing, loc, look_key;

    look_key = LK_SHOWATTR | LK_SHOWEXIT;
    if( !mudconf.terse_look ) {
        look_key |= LK_OBEYTERSE;
    }

    loc = Location( player );
    if( !name || !*name ) {
        look_here( player, loc, key, look_key );
        return;
    }

    /*
     * Look for the target locally
     */

    thing = ( key & LOOK_OUTSIDE ) ? loc : player;
    init_match( thing, name, NOTYPE );
    match_exit_with_parents();
    match_neighbor();
    match_possession();
    if( Long_Fingers( player ) ) {
        match_absolute();
        match_player();
    }
    match_here();
    match_me();
    match_master_exit();
    thing = match_result();

    /*
     * Not found locally, check possessive
     */

    if( !Good_obj( thing ) ) {
        thing = match_status( player,
                              match_possessed( player,
                                               ( ( key & LOOK_OUTSIDE ) ? loc : player ),
                                               ( char * ) name, thing, 0 ) );
    }

    /*
     * First make sure that we aren't looking at our own location, since
     * * that gets handled a little differently.
     */
    if( thing == loc ) {
        look_here( player, thing, key, look_key );
        return;
    }

    /*
     * If we found something, go handle it
     */

    if( Good_obj( thing ) ) {
        switch( Typeof( thing ) ) {
        case TYPE_ROOM:
            look_in( player, thing, look_key );
            break;
        case TYPE_THING:
        case TYPE_PLAYER:
            look_simple( player, thing, !mudconf.terse_look );
            if( !Opaque( thing ) &&
                    ( !Terse( player ) || mudconf.terse_contents ) ) {
                look_contents( player, thing, "Carrying:",
                               CONTENTS_NESTED );
            }
            break;
        case TYPE_EXIT:
            look_simple( player, thing, !mudconf.terse_look );
            if( Transparent( thing ) && Good_obj( Location( thing ) ) ) {
                look_key &= ~LK_SHOWATTR;
                look_in( player, Location( thing ), look_key );
            }
            break;
        default:
            look_simple( player, thing, !mudconf.terse_look );
        }
    }
}


static void debug_examine( dbref player, dbref thing ) {
    dbref aowner;

    char *buf;

    int aflags, alen, ca;

    BOOLEXP *bool;

    ATTR *attr;

    char *as, *cp, nbuf[20];

    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,  "Number  = %d", thing );
    if( !Good_obj( thing ) ) {
        return;
    }

    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Name    = %s", Name( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Location= %d", Location( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Contents= %d", Contents( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Exits   = %d", Exits( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Link    = %d", Link( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Next    = %d", Next( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Owner   = %d", Owner( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Pennies = %d", Pennies( thing ) );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Zone    = %d", Zone( thing ) );
    buf = flag_description( player, thing );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Flags   = %s", buf );
    free_mbuf( buf );
    buf = power_description( player, thing );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Powers  = %s", buf );
    free_mbuf( buf );
    buf = atr_get( thing, A_LOCK, &aowner, &aflags, &alen );
    bool = parse_boolexp( player, buf, 1 );
    free_lbuf( buf );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Lock    = %s", unparse_boolexp( player, bool ) );
    free_boolexp( bool );

    buf = alloc_lbuf( "debug_examine" );
    cp = buf;
    safe_str( ( char * ) "Attr list: ", buf, &cp );

    for( ca = atr_head( thing, &as ); ca; ca = atr_next( &as ) ) {
        attr = atr_num( ca );
        if( !attr ) {
            continue;
        }

        atr_get_info( thing, ca, &aowner, &aflags );
        if( Read_attr( player, thing, attr, aowner, aflags ) ) {
            if( attr ) {	/* Valid attr. */
                safe_str( ( char * ) attr->name, buf, &cp );
                safe_chr( ' ', buf, &cp );
            } else {
                ltos( nbuf, ca );
                safe_str( nbuf, buf, &cp );
                safe_chr( ' ', buf, &cp );
            }
        }
    }
    notify( player, buf );
    free_lbuf( buf );

    for( ca = atr_head( thing, &as ); ca; ca = atr_next( &as ) ) {
        attr = atr_num( ca );
        if( !attr ) {
            continue;
        }

        buf = atr_get( thing, ca, &aowner, &aflags, &alen );
        if( Read_attr_all( player, thing, attr, aowner, aflags, 1 ) )
            view_atr( player, thing, attr, buf, aowner, aflags,
                      0, 0 );
        free_lbuf( buf );
    }
}

static void exam_wildattrs( dbref player, dbref thing, int do_parent, int is_special ) {
    int atr, aflags, alen, got_any;

    char *buf;

    dbref aowner;

    ATTR *ap;

    got_any = 0;
    for( atr = olist_first(); atr != NOTHING; atr = olist_next() ) {
        ap = atr_num( atr );
        if( !ap ) {
            continue;
        }

        if( do_parent && !( ap->flags & AF_PRIVATE ) ) {
            buf = atr_pget( thing, atr, &aowner, &aflags, &alen );
        } else {
            buf = atr_get( thing, atr, &aowner, &aflags, &alen );
        }

        /*
         * Decide if the player should see the attr:
         * If obj is Examinable and has rights to see, yes.
         * If a player and has rights to see, yes...
         *   except if faraway, attr=DESC, and
         *   remote DESC-reading is not turned on.
         * If I own the attrib and have rights to see, yes...
         *   except if faraway, attr=DESC, and
         *   remote DESC-reading is not turned on.
         */

        if( Examinable( player, thing ) &&
                Read_attr_all( player, thing, ap, aowner, aflags, 1 ) ) {
            got_any = 1;
            view_atr( player, thing, ap, buf,
                      aowner, aflags, 0, is_special );
        } else if( ( Typeof( thing ) == TYPE_PLAYER ) &&
                   Read_attr_all( player, thing, ap, aowner, aflags, 1 ) ) {
            got_any = 1;
            if( aowner == Owner( player ) ) {
                view_atr( player, thing, ap, buf,
                          aowner, aflags, 0, is_special );
            } else if( ( atr == A_DESC ) &&
                       ( mudconf.read_rem_desc || nearby( player, thing ) ) ) {
                show_desc( player, thing, 0 );
            } else if( atr != A_DESC ) {
                view_atr( player, thing, ap, buf,
                          aowner, aflags, 0, is_special );
            } else {
                notify( player,
                        "<Too far away to get a good look>" );
            }
        } else if( Read_attr_all( player, thing, ap, aowner, aflags, 1 ) ) {
            got_any = 1;
            if( aowner == Owner( player ) ) {
                view_atr( player, thing, ap, buf,
                          aowner, aflags, 0, is_special );
            } else if( ( atr == A_DESC ) &&
                       ( mudconf.read_rem_desc || nearby( player, thing ) ) ) {
                show_desc( player, thing, 0 );
            } else if( nearby( player, thing ) ) {
                view_atr( player, thing, ap, buf,
                          aowner, aflags, 0, is_special );
            } else {
                notify( player,
                        "<Too far away to get a good look>" );
            }
        }
        free_lbuf( buf );
    }
    if( !got_any ) {
        notify_quiet( player, "No matching attributes found." );
    }
}

void do_examine( dbref player, dbref cause, int key, char *name ) {
    dbref thing, content, exit, aowner, loc;

    char savec;

    char *temp, *buf, *buf2, timebuf[32];

    BOOLEXP *bool;

    int control, aflags, alen, do_parent, is_special;

    time_t save_access_time;

    /*
     * This command is pointless if the player can't hear.
     */

    if( !Hearer( player ) ) {
        return;
    }

    do_parent = key & EXAM_PARENT;
    is_special = 0;
    if( key & EXAM_PRETTY ) {
        is_special = 1;
    }
    if( key & EXAM_PAIRS ) {
        is_special = 2;
    }

    thing = NOTHING;
    if( !name || !*name ) {
        if( ( thing = Location( player ) ) == NOTHING ) {
            return;
        }
    } else {
        /*
         * Check for obj/attr first.
         */

        olist_push();
        if( parse_attrib_wild( player, name, &thing, do_parent,
                               1, 0, 1 ) ) {
            exam_wildattrs( player, thing, do_parent, is_special );
            olist_pop();
            return;
        }
        olist_pop();

        /*
         * Look it up
         */

        init_match( player, name, NOTYPE );
        match_everything( MAT_EXIT_PARENTS );
        thing = noisy_match_result();
        if( !Good_obj( thing ) ) {
            return;
        }
    }

    /*
     * We have to save our access time, because the very act of
     * * trying to examine the object will have touched it.
     */
    save_access_time = AccessTime( thing );

    /*
     * Check for the /debug switch
     */

    if( key & EXAM_DEBUG ) {
        if( !Examinable( player, thing ) ) {
            notify_quiet( player, NOPERM_MESSAGE );
        } else {
            debug_examine( player, thing );
        }
        return;
    }
    control = ( Examinable( player, thing ) || Link_exit( player, thing ) );

    if( control && !( key & EXAM_OWNER ) ) {
        buf2 = unparse_object( player, thing, 0 );
        notify( player, buf2 );
        free_lbuf( buf2 );
        if( mudconf.ex_flags ) {
            buf2 = flag_description( player, thing );
            notify( player, buf2 );
            free_mbuf( buf2 );
        }
    } else {
        if( ( key & EXAM_OWNER ) ||
                ( ( key & EXAM_DEFAULT ) && !mudconf.exam_public ) ) {
            if( mudconf.read_rem_name ) {
                buf2 = alloc_lbuf( "do_examine.pub_name" );
                strcpy( buf2, Name( thing ) );
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s is owned by %s", buf2, Name( Owner( thing ) ) );
                free_lbuf( buf2 );
            } else {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Owned by %s", Name( Owner( thing ) ) );
            }
            return;
        }
    }

    temp = alloc_lbuf( "do_examine.info" );

    if( control || mudconf.read_rem_desc || nearby( player, thing ) ) {
        temp =
            atr_get_str( temp, thing, A_DESC, &aowner, &aflags, &alen );
        if( *temp ) {
            if( Examinable( player, thing ) ||
                    ( aowner == Owner( player ) ) ) {
                view_atr( player, thing, atr_num( A_DESC ), temp,
                          aowner, aflags, 1, is_special );
            } else {
                show_desc( player, thing, 0 );
            }
        }
    } else {
        notify( player, "<Too far away to get a good look>" );
    }

    if( control ) {

        /*
         * print owner, key, and value
         */

        savec = mudconf.many_coins[0];
        mudconf.many_coins[0] =
            ( islower( savec ) ? toupper( savec ) : savec );
        buf2 = atr_get( thing, A_LOCK, &aowner, &aflags, &alen );
        bool = parse_boolexp( player, buf2, 1 );
        buf = unparse_boolexp( player, bool );
        free_boolexp( bool );
        strcpy( buf2, Name( Owner( thing ) ) );
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Owner: %s  Key: %s %s: %d", buf2, buf, mudconf.many_coins, Pennies( thing ) );
        free_lbuf( buf2 );
        mudconf.many_coins[0] = savec;

        buf2 = ( char * ) ctime( &CreateTime( thing ) );
        buf2[strlen( buf2 ) - 1] = '\0';
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Created: %s", buf2 );

        buf2 = ( char * ) ctime( &save_access_time );
        buf2[strlen( buf2 ) - 1] = '\0';
        strcpy( timebuf, buf2 );
        buf2 = ( char * ) ctime( &ModTime( thing ) );
        buf2[strlen( buf2 ) - 1] = '\0';
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Accessed: %s    Modified: %s", timebuf, buf2 );

        /*
         * Print the zone
         */

        if( mudconf.have_zones ) {
            buf2 = unparse_object( player, Zone( thing ), 0 );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Zone: %s", buf2 );
            free_lbuf( buf2 );
        }
        /*
         * print parent
         */

        loc = Parent( thing );
        if( loc != NOTHING ) {
            buf2 = unparse_object( player, loc, 0 );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Parent: %s", buf2 );
            free_lbuf( buf2 );
        }

        /*
         * Show the powers
         */

        buf2 = power_description( player, thing );
        notify( player, buf2 );
        free_mbuf( buf2 );

    }

    CALL_ALL_MODULES( examine, ( player, cause, thing, control, key ) );

    if( !( ( key & EXAM_OWNER ) || ( key & EXAM_BRIEF ) ) ) {
        look_atrs( player, thing, do_parent, is_special );
    }

    /*
     * show him interesting stuff
     */

    if( control ) {

        /*
         * Contents
         */

        if( Contents( thing ) != NOTHING ) {
            notify( player, "Contents:" );
            DOLIST( content, Contents( thing ) ) {
                buf2 = unparse_object( player, content, 0 );
                notify( player, buf2 );
                free_lbuf( buf2 );
            }
        }
        /*
         * Show stuff that depends on the object type
         */

        switch( Typeof( thing ) ) {
        case TYPE_ROOM:

            /*
             * tell him about exits
             */

            if( Exits( thing ) != NOTHING ) {
                notify( player, "Exits:" );
                DOLIST( exit, Exits( thing ) ) {
                    buf2 = unparse_object( player, exit, 0 );
                    notify( player, buf2 );
                    free_lbuf( buf2 );
                }
            } else {
                notify( player, "No exits." );
            }

            /*
             * print dropto if present
             */

            if( Dropto( thing ) != NOTHING ) {
                buf2 = unparse_object( player, Dropto( thing ), 0 );
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL, "Dropped objects go to: %s", buf2 );
                free_lbuf( buf2 );
            }
            break;
        case TYPE_THING:
        case TYPE_PLAYER:

            /*
             * tell him about exits
             */

            if( Exits( thing ) != NOTHING ) {
                notify( player, "Exits:" );
                DOLIST( exit, Exits( thing ) ) {
                    buf2 = unparse_object( player, exit, 0 );
                    notify( player, buf2 );
                    free_lbuf( buf2 );
                }
            } else {
                notify( player, "No exits." );
            }

            /*
             * print home
             */

            loc = Home( thing );
            buf2 = unparse_object( player, loc, 0 );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL, "Home: %s", buf2 );
            free_lbuf( buf2 );

            /*
             * print location if player can link to it
             */

            loc = Location( thing );
            if( ( Location( thing ) != NOTHING ) && ( Examinable( player, loc ) || Examinable( player, thing ) || Linkable( player, loc ) ) ) {
                buf2 = unparse_object( player, loc, 0 );
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL, "Location: %s", buf2 );
                free_lbuf( buf2 );
            }
            break;
        case TYPE_EXIT:
            buf2 = unparse_object( player, Exits( thing ), 0 );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL, "Source: %s", buf2 );
            free_lbuf( buf2 );

            /*
             * print destination
             */

            switch( Location( thing ) ) {
            case NOTHING:
                /*
                 * Special case. unparse_object() normally
                 * * returns -1 as '*NOTHING*'.
                 */
                notify( player, "Destination: *UNLINKED*" );
                break;
            default:
                buf2 = unparse_object( player, Location( thing ), 0 );
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL, "Destination: %s", buf2 );
                free_lbuf( buf2 );
                break;
            }
            break;
        default:
            break;
        }
    } else if( !Opaque( thing ) && nearby( player, thing ) ) {
        if( Has_contents( thing ) ) 
            look_contents( player, thing, "Contents:", CONTENTS_REMOTE );
        if( !isExit( thing ) ) {
            look_exits( player, thing, "Obvious exits:" );
        }
    }
    free_lbuf( temp );

    if( !control ) {
        if( mudconf.read_rem_name ) {
            buf2 = alloc_lbuf( "do_examine.pub_name" );
            strcpy( buf2, Name( thing ) );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s is owned by %s", buf2, Name( Owner( thing ) ) );
            free_lbuf( buf2 );
        } else {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Owned by %s", Name( Owner( thing ) ) );
        }
    }
}

void do_score( dbref player, dbref cause, int key ) {
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "You have %d %s.", Pennies( player ), ( Pennies( player ) == 1 ) ? mudconf.one_coin : mudconf.many_coins );
            
}

void do_inventory( dbref player, dbref cause, int key ) {
    dbref thing;

    char *buff, *e;

    thing = Contents( player );
    if( thing == NOTHING ) {
        notify( player, "You aren't carrying anything." );
    } else {
        notify( player, "You are carrying:" );
        DOLIST( thing, thing ) {
            buff = unparse_object( player, thing, 1 );
            notify( player, buff );
            free_lbuf( buff );
        }
    }

    thing = Exits( player );
    if( thing != NOTHING ) {
        notify( player, "Exits:" );
        e = buff = alloc_lbuf( "look_exits" );
        DOLIST( thing, thing ) {
            if( e != buff ) {
                safe_known_str( ( char * ) "  ", 2, buff, &e );
            }
            safe_exit_name( thing, buff, &e );
        }
        *e = '\0';
        notify( player, buff );
        free_lbuf( buff );
    }
    do_score( player, player, 0 );
}

void do_entrances( dbref player, dbref cause, int key, char *name ) {
    dbref thing, i, j;

    char *exit, *message;

    int control_thing, count, low_bound, high_bound;

    FWDLIST *fp;

    PROPDIR *pp;

    parse_range( &name, &low_bound, &high_bound );
    if( !name || !*name ) {
        if( Has_location( player ) ) {
            thing = Location( player );
        } else {
            thing = player;
        }
        if( !Good_obj( thing ) ) {
            return;
        }
    } else {
        init_match( player, name, NOTYPE );
        match_everything( MAT_EXIT_PARENTS );
        thing = noisy_match_result();
        if( !Good_obj( thing ) ) {
            return;
        }
    }

    if( !payfor( player, mudconf.searchcost ) ) {
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "You don't have enough %s.", mudconf.many_coins );
        return;
    }
    message = alloc_lbuf( "do_entrances" );
    control_thing = Examinable( player, thing );
    count = 0;
    for( i = low_bound; i <= high_bound; i++ ) {
        if( control_thing || Examinable( player, i ) ) {
            switch( Typeof( i ) ) {
            case TYPE_EXIT:
                if( Location( i ) == thing ) {
                    exit = unparse_object( player, Exits( i ), 0 );
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s (%s)", exit, Name( i ) );
                    free_lbuf( exit );
                    count++;
                }
                break;
            case TYPE_ROOM:
                if( Dropto( i ) == thing ) {
                    exit = unparse_object( player, i, 0 );
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s [dropto]", exit );
                    free_lbuf( exit );
                    count++;
                }
                break;
            case TYPE_THING:
            case TYPE_PLAYER:
                if( Home( i ) == thing ) {
                    exit = unparse_object( player, i, 0 );
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s [home]", exit );
                    free_lbuf( exit );
                    count++;
                }
                break;
            }

            /*
             * Check for parents
             */

            if( Parent( i ) == thing ) {
                exit = unparse_object( player, i, 0 );
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s [parent]", exit );
                free_lbuf( exit );
                count++;
            }

            /*
             * Check for propdir
             */

            if( H_Propdir( i ) ) {
                pp = propdir_get( i );
                if( !pp ) {
                    continue;
                }
                for( j = 0; j < pp->count; j++ ) {
                    if( pp->data[j] != thing ) {
                        continue;
                    }
                    exit = unparse_object( player, i, 0 );
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s [propdir]", exit );
                    free_lbuf( exit );
                    count++;
                }
            }

            /*
             * Check for forwarding
             */

            if( H_Fwdlist( i ) ) {
                fp = fwdlist_get( i );
                if( !fp ) {
                    continue;
                }
                for( j = 0; j < fp->count; j++ ) {
                    if( fp->data[j] != thing ) {
                        continue;
                    }
                    exit = unparse_object( player, i, 0 );
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s [forward]", exit );
                    free_lbuf( exit );
                    count++;
                }
            }
        }
    }
    free_lbuf( message );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%d entrance%s found.", count, ( count == 1 ) ? "" : "s" );
}

/* check the current location for bugs */

static void sweep_check( dbref player, dbref what, int key, int is_loc ) {
    dbref aowner, parent;

    int canhear, cancom, isplayer, ispuppet, isconnected, is_audible, attr,
        aflags, alen;
    int is_parent, lev;

    char *buf, *buf2, *bp, *as, *buff, *s;

    ATTR *ap;

    canhear = 0;
    cancom = 0;
    isplayer = 0;
    ispuppet = 0;
    isconnected = 0;
    is_audible = 0;
    is_parent = 0;

    if( ( key & SWEEP_LISTEN ) &&
            ( ( isExit( what ) || is_loc ) && Audible( what ) ) ) {
        is_audible = 1;
    }

    if( key & SWEEP_LISTEN ) {
        if( H_Listen( what ) || Bouncer( what ) ) {
            canhear = 1;
        } else if( Monitor( what ) ) {
            buff = alloc_lbuf( "Hearer" );
            for( attr = atr_head( what, &as ); attr;
                    attr = atr_next( &as ) ) {
                ap = atr_num( attr );
                if( !ap || ( ap->flags & AF_NOPROG ) ) {
                    continue;
                }

                atr_get_str( buff, what, attr, &aowner,
                             &aflags, &alen );

                /*
                 * Make sure we can execute it
                 */

                if( ( buff[0] != AMATCH_LISTEN ) ||
                        ( aflags & AF_NOPROG ) ) {
                    continue;
                }

                /*
                 * Make sure there's a : in it
                 */

                for( s = buff + 1; *s && ( *s != ':' ); s++ );
                if( s ) {
                    canhear = 1;
                    break;
                }
            }
            free_lbuf( buff );
        }
    }
    if( ( key & SWEEP_COMMANDS ) && !isExit( what ) ) {

        /*
         * Look for commands on the object and parents too
         */

        ITER_PARENTS( what, parent, lev ) {
            if( Commer( parent ) ) {
                cancom = 1;
                if( lev ) {
                    is_parent = 1;
                    break;
                }
            }
        }
    }
    if( key & SWEEP_CONNECT ) {
        if( Connected( what ) ||
                ( Puppet( what ) && Connected( Owner( what ) ) ) ||
                ( mudconf.player_listen && ( Typeof( what ) == TYPE_PLAYER ) &&
                  canhear && Connected( Owner( what ) ) ) ) {
            isconnected = 1;
        }
    }
    if( key & SWEEP_PLAYER || isconnected ) {
        if( Typeof( what ) == TYPE_PLAYER ) {
            isplayer = 1;
        }
        if( Puppet( what ) ) {
            ispuppet = 1;
        }
    }
    if( canhear || cancom || isplayer || ispuppet || isconnected ||
            is_audible || is_parent ) {
        buf = alloc_lbuf( "sweep_check.types" );
        bp = buf;

        if( cancom ) {
            safe_str( ( char * ) "commands ", buf, &bp );
        }
        if( canhear ) {
            safe_str( ( char * ) "messages ", buf, &bp );
        }
        if( is_audible ) {
            safe_str( ( char * ) "audible ", buf, &bp );
        }
        if( isplayer ) {
            safe_str( ( char * ) "player ", buf, &bp );
        }
        if( ispuppet ) {
            safe_str( ( char * ) "puppet(", buf, &bp );
            safe_name( Owner( what ), buf, &bp );
            safe_str( ( char * ) ") ", buf, &bp );
        }
        if( isconnected ) {
            safe_str( ( char * ) "connected ", buf, &bp );
        }
        if( is_parent ) {
            safe_str( ( char * ) "parent ", buf, &bp );
        }
        *--bp = '\0';	/* nuke the space at the end */
        if( !isExit( what ) ) {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "  %s is listening. [%s]", Name( what ), buf );
        } else {
            bp = buf2 = alloc_lbuf( "sweep_check.name" );
            safe_exit_name( what, buf2, &bp );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "  %s is listening. [%s]", buf2, buf );
            free_lbuf( buf2 );
        }
        free_lbuf( buf );
    }
}

void do_sweep( dbref player, dbref cause, int key, char *where ) {
    dbref here, sweeploc;

    int where_key, what_key;

    where_key = key & ( SWEEP_ME | SWEEP_HERE | SWEEP_EXITS );
    what_key =
        key & ( SWEEP_COMMANDS | SWEEP_LISTEN | SWEEP_PLAYER |
                SWEEP_CONNECT );

    if( where && *where ) {
        sweeploc = match_controlled( player, where );
        if( !Good_obj( sweeploc ) ) {
            return;
        }
    } else {
        sweeploc = player;
    }

    if( !where_key ) {
        where_key = -1;
    }
    if( !what_key ) {
        what_key = -1;
    } else if( what_key == SWEEP_VERBOSE ) {
        what_key = SWEEP_VERBOSE | SWEEP_COMMANDS;
    }

    /*
     * Check my location.  If I have none or it is dark, check just me.
     */

    if( where_key & SWEEP_HERE ) {
        notify( player, "Sweeping location..." );
        if( Has_location( sweeploc ) ) {
            here = Location( sweeploc );
            if( ( here == NOTHING ) ||
                    ( Dark( here ) && !mudconf.sweep_dark &&
                      !Examinable( player, here ) ) ) {
                notify_quiet( player,
                              "Sorry, it is dark here and you can't search for bugs" );
                sweep_check( player, sweeploc, what_key, 0 );
            } else {
                sweep_check( player, here, what_key, 1 );
                for( here = Contents( here ); here != NOTHING;
                        here = Next( here ) ) {
                    sweep_check( player, here, what_key, 0 );
                }
            }
        } else {
            sweep_check( player, sweeploc, what_key, 0 );
        }
    }
    /*
     * Check exits in my location
     */

    if( ( where_key & SWEEP_EXITS ) && Has_location( sweeploc ) ) {
        notify( player, "Sweeping exits..." );
        for( here = Exits( Location( sweeploc ) ); here != NOTHING;
                here = Next( here ) ) {
            sweep_check( player, here, what_key, 0 );
        }
    }
    /*
     * Check my inventory
     */

    if( ( where_key & SWEEP_ME ) && Has_contents( sweeploc ) ) {
        notify( player, "Sweeping inventory..." );
        for( here = Contents( sweeploc ); here != NOTHING;
                here = Next( here ) ) {
            sweep_check( player, here, what_key, 0 );
        }
    }
    /*
     * Check carried exits
     */

    if( ( where_key & SWEEP_EXITS ) && Has_exits( sweeploc ) ) {
        notify( player, "Sweeping carried exits..." );
        for( here = Exits( sweeploc ); here != NOTHING;
                here = Next( here ) ) {
            sweep_check( player, here, what_key, 0 );
        }
    }
    notify( player, "Sweep complete." );
}

/* Output the sequence of commands needed to duplicate the specified
 * object.  If you're moving things to another system, your mileage
 * will almost certainly vary.  (i.e. different flags, etc.)
 */

extern NAMETAB indiv_attraccess_nametab[];

void do_decomp( dbref player, dbref cause, int key, char *name, char *qual ) {
    BOOLEXP *bool;

    char *got, *thingname, *as, *ltext, *buff, *tbuf, *tmp;

    dbref aowner, thing;

    int val, aflags, alen, ca, wild_decomp;

    ATTR *attr;

    NAMETAB *np;

    /*
     * Check for obj/attr first
     */

    olist_push();
    if( parse_attrib_wild( player, name, &thing, 0, 1, 0, 1 ) ) {
        wild_decomp = 1;
    } else {
        wild_decomp = 0;
        init_match( player, name, TYPE_THING );
        match_everything( MAT_EXIT_PARENTS );
        thing = noisy_match_result();
    }

    /*
     * get result
     */
    if( thing == NOTHING ) {
        olist_pop();
        return;
    }

    if( !Examinable( player, thing ) ) {
        notify_quiet( player,
                      "You can only decompile things you can examine." );
        olist_pop();
        return;
    }
    thingname = atr_get( thing, A_LOCK, &aowner, &aflags, &alen );
    bool = parse_boolexp( player, thingname, 1 );

    /*
     * Determine the name of the thing to use in reporting and then
     * * report the command to make the thing.
     */

    if( qual && *qual ) {
        strcpy( thingname, qual );
    } else {
        switch( Typeof( thing ) ) {
        case TYPE_THING:
            strcpy( thingname, Name( thing ) );
            val = OBJECT_DEPOSIT( Pennies( thing ) );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@create %s=%d", translate_string( thingname, 1 ), val );
            break;
        case TYPE_ROOM:
            strcpy( thingname, Name( thing ) );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@dig/teleport %s", translate_string( thingname, 1 ) );
            strcpy( thingname, "here" );
            break;
        case TYPE_EXIT:
            strcpy( thingname, Name( thing ) );
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@open %s", translate_string( thingname, 1 ) );
            got = thingname;
            safe_exit_name( thing, thingname, &got );
            break;
        case TYPE_PLAYER:
            strcpy( thingname, "me" );
            break;
        }
    }

    strcpy( thingname, strip_ansi( thingname ) );

    /*
     * Report the lock (if any)
     */

    if( !wild_decomp && ( bool != TRUE_BOOLEXP ) ) {
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@lock %s=%s", thingname, unparse_boolexp_decompile( player, bool ) );
    }
    free_boolexp( bool );

    /*
     * Report attributes
     */

    buff = alloc_mbuf( "do_decomp.attr_name" );
    for( ca = ( wild_decomp ? olist_first() : atr_head( thing, &as ) ); ( wild_decomp ) ? ( ca != NOTHING ) : ( ca != ( int ) 0 ); ca = ( wild_decomp ? olist_next() : atr_next( &as ) ) ) {
        if( ( ca == A_NAME ) || ( ca == A_LOCK ) ) {
            continue;
        }
        attr = atr_num( ca );
        if( !attr ) {
            continue;
        }
        if( ( attr->flags & AF_NOCMD ) && !( attr->flags & AF_IS_LOCK ) ) {
            continue;
        }

        got = atr_get( thing, ca, &aowner, &aflags, &alen );
        if( aflags & AF_STRUCTURE ) {
            tmp = replace_string( GENERIC_STRUCT_STRDELIM,
                                  mudconf.struct_dstr, got );
            free_lbuf( got );
            got = tmp;
        }
        if( Read_attr_all( player, thing, attr, aowner, aflags, 1 ) ) {
            if( attr->flags & AF_IS_LOCK ) {
                bool = parse_boolexp( player, got, 1 );
                ltext = unparse_boolexp_decompile( player,
                                                   bool );
                free_boolexp( bool );
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@lock/%s %s=%s", attr->name, thingname, ltext );
            } else {
                strcpy( buff, attr->name );
                if( key & DECOMP_PRETTY ) {
                    tbuf = alloc_lbuf( "do_decomp.pretty" );
                    pretty_print( tbuf, tmprintf( "%c%s %s=", ( ( ca < A_USER_START ) ? '@' : '&' ), buff, thingname ), got );
                    notify( player, tbuf );
                    free_lbuf( tbuf );
                } else {
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%c%s %s=%s", ( ( ca < A_USER_START ) ? '@' : '&' ), buff, thingname, got );
                }
                for( np = indiv_attraccess_nametab;
                        np->name; np++ ) {
                    if( ( aflags & np->flag ) && check_access( player, np->perm ) && ( !( np->perm & CA_NO_DECOMP ) ) ) {
                        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@set %s/%s = %s", thingname, buff, np->name );
                    }
                }

                if( aflags & AF_LOCK ) {
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@lock %s/%s", thingname, buff );
                }
            }
        }
        free_lbuf( got );
    }
    free_mbuf( buff );

    if( !wild_decomp ) {
        decompile_flags( player, thing, thingname );
        decompile_powers( player, thing, thingname );
    }

    /*
     * If the object has a parent, report it
     */

    if( !wild_decomp && ( Parent( thing ) != NOTHING ) )
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@parent %s=#%d", thingname, Parent( thing ) );

    /*
     * If the object has a zone, report it
     */

    if( !wild_decomp && ( Zone( thing ) != NOTHING ) )
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "@chzone %s=#%d", thingname, Zone( thing ) );

    free_lbuf( thingname );
    olist_pop();
}

#ifdef PUEBLO_SUPPORT
void show_vrml_url( dbref thing, dbref loc ) {
    char *vrml_url;

    dbref aowner;

    int aflags, alen;

    /*
     * If they don't care about HTML, just return.
     */
    if( !Html( thing ) ) {
        return;
    }

    vrml_url = atr_pget( loc, A_VRML_URL, &aowner, &aflags, &alen );
    if( *vrml_url ) {
        char *vrml_message, *vrml_cp;

        vrml_message = vrml_cp = alloc_lbuf( "show_vrml_url" );
        safe_str( "<img xch_graph=load href=\"", vrml_message,
                  &vrml_cp );
        safe_str( vrml_url, vrml_message, &vrml_cp );
        safe_str( "\">", vrml_message, &vrml_cp );
        *vrml_cp = 0;
        notify_html( thing, vrml_message );
        free_lbuf( vrml_message );
    } else {
        notify_html( thing, "<img xch_graph=hide>" );
    }
    free_lbuf( vrml_url );
}
#endif
