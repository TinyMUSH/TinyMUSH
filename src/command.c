/* command.c - command parser and support routines */

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


#include "help.h"		/* required by code */
#include "command.h"		/* required by code */
#include "functions.h"		/* required by code */
#include "match.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "vattr.h"		/* required by code */
#include "pcre.h"		/* required by code */

extern void list_cf_access( dbref );

extern void list_cf_read_access( dbref );

extern void list_siteinfo( dbref );

extern void logged_out( dbref, dbref, int, char * );

extern void list_functable( dbref );

extern void list_funcaccess( dbref );

extern void list_bufstats( dbref );

extern void list_buftrace( dbref );

extern void list_cached_objs( dbref );

extern void list_cached_attrs( dbref );

extern void list_rawmemory( dbref );

extern int atr_match( dbref, dbref, char, char *, char *, int );

extern int list_check( dbref, dbref, char, char *, char *, int, int * );

extern void do_enter_internal( dbref, dbref, int );

extern int regexp_match( char *, char *, int, char **, int );

extern void register_prefix_cmds( const char * );

#define CACHING "attribute"

#define NOGO_MESSAGE "You can't go that way."

/* Take care of all the assorted problems associated with getrusage(). */

#ifdef hpux
#define HAVE_GETRUSAGE 1
#include <sys/syscall.h>
#define getrusage(x,p)   syscall(SYS_GETRUSAGE,x,p)
#endif

#ifdef _SEQUENT_
#define HAVE_GET_PROCESS_STATS 1
#include <sys/procstats.h>
#endif

/* This must be the LAST thing we include. */

#include "cmdtabs.h"		/* required by code */

/*
 * ---------------------------------------------------------------------------
 * Hook macros.
 *
 * We never want to call hooks in the case of @addcommand'd commands (both for
 * efficiency reasons and the fact that we might NOT match an @addcommand
 * even if we've been told there is one), but we leave this to the hook-adder
 * to prevent.
 */

#define CALL_PRE_HOOK(x,a,na) \
if (((x)->pre_hook != NULL) && !((x)->callseq & CS_ADDED)) { \
    process_hook((x)->pre_hook, (x)->callseq & CS_PRESERVE|CS_PRIVATE, \
                 player, cause, (a), (na)); \
}

#define CALL_POST_HOOK(x,a,na) \
if (((x)->post_hook != NULL) && !((x)->callseq & CS_ADDED)) { \
    process_hook((x)->post_hook, (x)->callseq & CS_PRESERVE|CS_PRIVATE, \
                 player, cause, (a), (na)); \
}

CMDENT *prefix_cmds[256];

CMDENT *goto_cmdp, *enter_cmdp, *leave_cmdp, *internalgoto_cmdp;

/*
 * ---------------------------------------------------------------------------
 * Main body of code.
 */

void init_cmdtab( void ) {
    CMDENT *cp;

    ATTR *ap;

    char *p, *q;

    char *cbuff;

    int i;

    hashinit( &mudstate.command_htab, 250 * HASH_FACTOR, HT_STR );

    /*
     * Load attribute-setting commands
     */

    cbuff = alloc_sbuf( "init_cmdtab" );
    for( ap = attr; ap->name; ap++ ) {
        if( ( ap->flags & AF_NOCMD ) == 0 ) {
            p = cbuff;
            *p++ = '@';
            for( q = ( char * ) ap->name; *q; p++, q++ ) {
                *p = tolower( *q );
            }
            *p = '\0';
            cp = ( CMDENT * ) XMALLOC( sizeof( CMDENT ), "init_cmdtab" );
            cp->cmdname = XSTRDUP( cbuff, "init_cmdtab.cmdname" );
            cp->perms = CA_NO_GUEST | CA_NO_SLAVE;
            cp->switches = NULL;
            if( ap->flags & ( AF_WIZARD | AF_MDARK ) ) {
                cp->perms |= CA_WIZARD;
            }
            cp->extra = ap->number;
            cp->callseq = CS_TWO_ARG;
            cp->pre_hook = NULL;
            cp->post_hook = NULL;
            cp->userperms = NULL;
            cp->info.handler = do_setattr;
            if( hashadd( cp->cmdname, ( int * ) cp,
                         &mudstate.command_htab, 0 ) ) {
                XFREE( cp->cmdname, "init_cmdtab.2" );
                XFREE( cp, "init_cmdtab.3" );
            } else {
                /*
                 * also add the __ alias form
                 */
                hashadd( tmprintf( "__%s", cp->cmdname ), ( int * ) cp, &mudstate.command_htab, HASH_ALIAS );
            }
        }
    }
    free_sbuf( cbuff );

    /*
     * Load the builtin commands, plus __ aliases
     */

    for( cp = command_table; cp->cmdname; cp++ ) {
        hashadd( cp->cmdname, ( int * ) cp, &mudstate.command_htab, 0 );
        hashadd( tmprintf( "__%s", cp->cmdname ), ( int * ) cp, &mudstate.command_htab, HASH_ALIAS );
    }

    /*
     * Set the builtin prefix commands
     */
    for( i = 0; i < 256; i++ ) {
        prefix_cmds[i] = NULL;
    }
    register_prefix_cmds( "\":;\\#&" );	/* ":;\#&  */

    goto_cmdp = ( CMDENT * ) hashfind( "goto", &mudstate.command_htab );
    enter_cmdp = ( CMDENT * ) hashfind( "enter", &mudstate.command_htab );
    leave_cmdp = ( CMDENT * ) hashfind( "leave", &mudstate.command_htab );
    internalgoto_cmdp =
        ( CMDENT * ) hashfind( "internalgoto", &mudstate.command_htab );
}

void reset_prefix_cmds( void ) {
    int i;

    char cn[2] = "x";

    for( i = 0; i < 256; i++ ) {
        if( prefix_cmds[i] ) {
            cn[0] = i;
            prefix_cmds[i] =
                ( CMDENT * ) hashfind( cn, &mudstate.command_htab );
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * check_access: Check if player has access to function. Note that the
 * calling function may also give permission denied messages on failure.
 */

int check_access( dbref player, int mask ) {
    int mval, nval;

    /*
     * Check if we have permission to execute
     */

    if( mask & ( CA_DISABLED | CA_STATIC ) ) {
        return 0;
    }
    if( God( player ) || mudstate.initializing ) {
        return 1;
    }

    /*
     * Check for bits that we have to have. Since we know that we're not
     * God at this point, if it is God-only, it fails. (God in
     * combination with other stuff is implicitly checked, since we
     * return false if we don't find the other bits.)
     */

    if( ( mval = mask & ( CA_ISPRIV_MASK | CA_MARKER_MASK ) ) == CA_GOD ) {
        return 0;
    }
    if( mval ) {
        mval = mask & CA_ISPRIV_MASK;
        nval = mask & CA_MARKER_MASK;
        if( mval && !nval ) {
            if( !( ( ( mask & CA_WIZARD ) && Wizard( player ) ) ||
                    ( ( mask & CA_ADMIN ) && WizRoy( player ) ) ||
                    ( ( mask & CA_BUILDER ) && Builder( player ) ) ||
                    ( ( mask & CA_STAFF ) && Staff( player ) ) ||
                    ( ( mask & CA_HEAD ) && Head( player ) ) ||
                    ( ( mask & CA_IMMORTAL ) && Immortal( player ) ) ||
                    ( ( mask & CA_MODULE_OK ) && Can_Use_Module( player ) ) ) ) {
                return 0;
            }
        } else if( !mval && nval ) {
            if( !( ( ( mask & CA_MARKER0 ) && H_Marker0( player ) ) ||
                    ( ( mask & CA_MARKER1 ) && H_Marker1( player ) ) ||
                    ( ( mask & CA_MARKER2 ) && H_Marker2( player ) ) ||
                    ( ( mask & CA_MARKER3 ) && H_Marker3( player ) ) ||
                    ( ( mask & CA_MARKER4 ) && H_Marker4( player ) ) ||
                    ( ( mask & CA_MARKER5 ) && H_Marker5( player ) ) ||
                    ( ( mask & CA_MARKER6 ) && H_Marker6( player ) ) ||
                    ( ( mask & CA_MARKER7 ) && H_Marker7( player ) ) ||
                    ( ( mask & CA_MARKER8 ) && H_Marker8( player ) ) ||
                    ( ( mask & CA_MARKER9 ) && H_Marker9( player ) ) ) ) {
                return 0;
            }
        } else {
            if( !( ( ( mask & CA_WIZARD ) && Wizard( player ) ) ||
                    ( ( mask & CA_ADMIN ) && WizRoy( player ) ) ||
                    ( ( mask & CA_BUILDER ) && Builder( player ) ) ||
                    ( ( mask & CA_STAFF ) && Staff( player ) ) ||
                    ( ( mask & CA_HEAD ) && Head( player ) ) ||
                    ( ( mask & CA_IMMORTAL ) && Immortal( player ) ) ||
                    ( ( mask & CA_MODULE_OK ) && Can_Use_Module( player ) ) ||
                    ( ( mask & CA_MARKER0 ) && H_Marker0( player ) ) ||
                    ( ( mask & CA_MARKER1 ) && H_Marker1( player ) ) ||
                    ( ( mask & CA_MARKER2 ) && H_Marker2( player ) ) ||
                    ( ( mask & CA_MARKER3 ) && H_Marker3( player ) ) ||
                    ( ( mask & CA_MARKER4 ) && H_Marker4( player ) ) ||
                    ( ( mask & CA_MARKER5 ) && H_Marker5( player ) ) ||
                    ( ( mask & CA_MARKER6 ) && H_Marker6( player ) ) ||
                    ( ( mask & CA_MARKER7 ) && H_Marker7( player ) ) ||
                    ( ( mask & CA_MARKER8 ) && H_Marker8( player ) ) ||
                    ( ( mask & CA_MARKER9 ) && H_Marker9( player ) ) ) ) {
                return 0;
            }
        }
    }
    /*
     * Check the things that we can't be.
     */

    if( ( ( mask & CA_ISNOT_MASK ) && !Wizard( player ) ) &&
            ( ( ( mask & CA_NO_HAVEN ) && Player_haven( player ) ) ||
              ( ( mask & CA_NO_ROBOT ) && Robot( player ) ) ||
              ( ( mask & CA_NO_SLAVE ) && Slave( player ) ) ||
              ( ( mask & CA_NO_SUSPECT ) && Suspect( player ) ) ||
              ( ( mask & CA_NO_GUEST ) && Guest( player ) ) ) ) {
        return 0;
    }
    return 1;
}


/*
 * ---------------------------------------------------------------------------
 * check_mod_access: Go through sequence of module call-outs, treating all of
 * them like permission checks.
 */

int check_mod_access( dbref player, EXTFUNCS *xperms ) {
    int i;

    for( i = 0; i < xperms->num_funcs; i++ ) {
        if( !xperms->ext_funcs[i] ) {
            continue;
        }
        if( !( ( xperms->ext_funcs[i]->handler )( player ) ) ) {
            return 0;
        }
    }
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * check_userdef_access: Check if user has access to command with user-def'd
 * permissions.
 */

int check_userdef_access( dbref player, HOOKENT *hookp, char *cargs[], int ncargs ) {
    char *buf;

    char *bp, *tstr, *str;

    dbref aowner;

    int result, aflags, alen;

    GDATA *preserve;

    /*
     * We have user-defined command permissions. Go evaluate the obj/attr
     * pair that we've been given. If that result is nonexistent, we
     * consider it a failure. We use boolean truth here.
     *
     * Note that unlike before and after hooks, we always preserve the
     * registers. (When you get right down to it, this thing isn't really
     * a hook. It's just convenient to re-use the same code that we use
     * with hooks.)
     */

    tstr = atr_get( hookp->thing, hookp->atr, &aowner, &aflags, &alen );
    if( !tstr ) {
        return 0;
    }
    if( !*tstr ) {
        free_lbuf( tstr );
        return 0;
    }
    str = tstr;

    preserve = save_global_regs( "check_userdef_access" );

    bp = buf = alloc_lbuf( "check_userdef_access" );
    exec( buf, &bp, hookp->thing, player, player,
          EV_EVAL | EV_FCHECK | EV_TOP, &str, cargs, ncargs );

    restore_global_regs( "check_userdef_access", preserve );

    result = xlate( buf );

    free_lbuf( buf );
    free_lbuf( tstr );

    return result;
}

/*
 * ---------------------------------------------------------------------------
 * process_hook: Evaluate a hook function.
 */

static void process_hook( HOOKENT *hp, int save_globs, dbref player, dbref cause, char *cargs[], int ncargs ) {
    char *buf, *bp;

    char *tstr, *str;

    dbref aowner;

    int aflags, alen;

    GDATA *preserve;

    /*
     * We know we have a non-null hook. We want to evaluate the obj/attr
     * pair of that hook. We consider the enactor to be the player who
     * executed the command that caused this hook to be called.
     */

    tstr = atr_get( hp->thing, hp->atr, &aowner, &aflags, &alen );
    str = tstr;

    if( save_globs & CS_PRESERVE ) {
        preserve = save_global_regs( "process_hook" );
    } else if( save_globs & CS_PRIVATE ) {
        preserve = mudstate.rdata;
        mudstate.rdata = NULL;
    }
    buf = bp = alloc_lbuf( "process_hook" );
    exec( buf, &bp, hp->thing, player, player, EV_EVAL | EV_FCHECK | EV_TOP,
          &str, cargs, ncargs );
    free_lbuf( buf );

    if( save_globs & CS_PRESERVE ) {
        restore_global_regs( "process_hook", preserve );
    } else if( save_globs & CS_PRIVATE ) {
        Free_RegData( mudstate.rdata );
        mudstate.rdata = preserve;
    }
    free_lbuf( tstr );
}

void call_move_hook( dbref player, dbref cause, int state ) {
    if( internalgoto_cmdp ) {
        if( !state ) {	/* before move */
            CALL_PRE_HOOK( internalgoto_cmdp, NULL, 0 );
        } else {	/* after move */

            CALL_POST_HOOK( internalgoto_cmdp, NULL, 0 );
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * process_cmdent: Perform indicated command with passed args.
 */

void process_cmdent( CMDENT *cmdp, char *switchp, dbref player, dbref cause, int interactive, char *arg, char *unp_command, char *cargs[], int ncargs ) {
    char *buf1, *buf2, tchar, *bp, *str, *buff, *s, *j, *new, *pname, *lname;

    char *args[MAX_ARG], *aargs[NUM_ENV_VARS];

    int nargs, i, interp, key, xkey, aflags, alen;

    int hasswitch = 0;

    int cmd_matches = 0;

    dbref aowner;

    ADDENT *add;

    GDATA *preserve;

    /*
     * Perform object type checks.
     */

    if( Invalid_Objtype( player ) ) {
        notify( player, "Command incompatible with invoker type." );
        return;
    }
    /*
     * Check if we have permission to execute the command
     */

    if( !Check_Cmd_Access( player, cmdp, cargs, ncargs ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }
    /*
     * Check global flags
     */

    if( ( !Builder( player ) ) && Protect( CA_GBL_BUILD ) &&
            !( mudconf.control_flags & CF_BUILD ) ) {
        notify( player, "Sorry, building is not allowed now." );
        return;
    }
    if( Protect( CA_GBL_INTERP ) && !( mudconf.control_flags & CF_INTERP ) ) {
        notify( player,
                "Sorry, queueing and triggering are not allowed now." );
        return;
    }
    key = cmdp->extra & ~SW_MULTIPLE;
    if( key & SW_GOT_UNIQUE ) {
        i = 1;
        key = key & ~SW_GOT_UNIQUE;
    } else {
        i = 0;
    }

    /*
     * Check command switches.  Note that there may be more than one, and
     * that we OR all of them together along with the extra value from
     * the command table to produce the key value in the handler call.
     */

    if( switchp && cmdp->switches ) {
        do {
            buf1 = strchr( switchp, '/' );
            if( buf1 ) {
                *buf1++ = '\0';
            }
            xkey = search_nametab( player, cmdp->switches, switchp );
            if( xkey == -1 ) {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Unrecognized switch '%s' for command '%s'.", switchp, cmdp->cmdname );
                return;
            } else if( xkey == -2 ) {
                notify( player, NOPERM_MESSAGE );
                return;
            } else if( !( xkey & SW_MULTIPLE ) ) {
                if( i == 1 ) {
                    notify( player, "Illegal combination of switches." );
                    return;
                }
                i = 1;
            } else {
                xkey &= ~SW_MULTIPLE;
            }
            key |= xkey;
            switchp = buf1;
            hasswitch = 1;
        } while( buf1 );
    } else if( switchp && !( cmdp->callseq & CS_ADDED ) ) {
        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Command %s does not take switches.", cmdp->cmdname );
        return;
    }
    /*
     * At this point we're guaranteed we're going to execute something.
     * Let's check to see if we have a pre-command hook.
     */

    CALL_PRE_HOOK( cmdp, cargs, ncargs );

    /*
     * If the command normally has interpreted args, but the user
     * specified, /noeval, just do EV_STRIP.
     *
     * If the command is interpreted, or we're interactive (and the command
     * isn't specified CS_NOINTERP), eval the args.
     *
     * The others are obvious.
     */
    if( ( cmdp->callseq & CS_INTERP ) && ( key & SW_NOEVAL ) ) {
        interp = EV_STRIP;
        key &= ~SW_NOEVAL;	/* Remove SW_NOEVAL from 'key' */
    } else if( ( cmdp->callseq & CS_INTERP ) ||
               !( interactive || ( cmdp->callseq & CS_NOINTERP ) ) ) {
        interp = EV_EVAL | EV_STRIP;
    } else if( cmdp->callseq & CS_STRIP ) {
        interp = EV_STRIP;
    } else if( cmdp->callseq & CS_STRIP_AROUND ) {
        interp = EV_STRIP_AROUND;
    } else {
        interp = 0;
    }

    switch( cmdp->callseq & CS_NARG_MASK ) {
    case CS_NO_ARGS:	/* <cmd>   (no args) */
        ( * ( cmdp->info.handler ) )( player, cause, key );
        break;
    case CS_ONE_ARG:	/* <cmd> <arg> */

        /*
         * If an unparsed command, just give it to the handler
         */

        if( cmdp->callseq & CS_UNPARSE ) {
            ( * ( cmdp->info.handler ) )( player, unp_command );
            break;
        }
        /*
         * Interpret if necessary, but not twice for CS_ADDED
         */

        if( ( interp & EV_EVAL ) && !( cmdp->callseq & CS_ADDED ) ) {
            buf1 = bp = alloc_lbuf( "process_cmdent" );
            str = arg;
            exec( buf1, &bp, player, cause, cause,
                  interp | EV_FCHECK | EV_TOP, &str, cargs, ncargs );
        } else {
            buf1 = parse_to( &arg, '\0', interp | EV_TOP );
        }

        /*
         * Call the correct handler
         */

        if( cmdp->callseq & CS_CMDARG ) {
            ( * ( cmdp->info.handler ) )( player, cause, key, buf1,
                                          cargs, ncargs );
        } else {
            if( cmdp->callseq & CS_ADDED ) {

                preserve =
                    save_global_regs( "process_cmdent_added" );

                /*
                 * Construct the matching buffer.
                 */

                /*
                 * In the case of a single-letter prefix, we
                 * want to just skip past that first letter.
                 * Otherwise we want to go past the first
                 * word.
                 */
                if( !( cmdp->callseq & CS_LEADIN ) ) {
                    for( j = unp_command;
                            *j && ( *j != ' ' ); j++ );
                } else {
                    j = unp_command;
                    j++;
                }
                new = alloc_lbuf( "process_cmdent.soft" );
                bp = new;
                if( !*j ) {
                    /*
                     * No args
                     */
                    if( !( cmdp->callseq & CS_LEADIN ) ) {
                        safe_str( cmdp->cmdname, new,
                                  &bp );
                    } else {
                        safe_str( unp_command, new,
                                  &bp );
                    }
                    if( switchp ) {
                        safe_chr( '/', new, &bp );
                        safe_str( switchp, new, &bp );
                    }
                    *bp = '\0';
                } else {
                    if( !( cmdp->callseq & CS_LEADIN ) ) {
                        j++;
                    }
                    safe_str( cmdp->cmdname, new, &bp );
                    if( switchp ) {
                        safe_chr( '/', new, &bp );
                        safe_str( switchp, new, &bp );
                    }
                    if( !( cmdp->callseq & CS_LEADIN ) ) {
                        safe_chr( ' ', new, &bp );
                    }
                    safe_str( j, new, &bp );
                    *bp = '\0';
                }

                /*
                 * Now search against the attributes, unless
                 * we can't pass the uselock.
                 */

                for( add = ( ADDENT * ) cmdp->info.added;
                        add != NULL; add = add->next ) {

                    buff = atr_get( add->thing, add->atr,
                                    &aowner, &aflags, &alen );
                    /*
                     * Skip the '$' character, and the
                     * next
                     */
                    for( s = buff + 2;
                            *s && ( ( *s != ':' )
                                    || ( * ( s - 1 ) == '\\' ) ); s++ );
                    if( !*s ) {
                        free_lbuf( buff );
                        break;
                    }
                    *s++ = '\0';

                    if( ( ( !( aflags & AF_REGEXP ) &&
                            wild( buff + 1, new, aargs,
                                  NUM_ENV_VARS ) ) ||
                            ( ( aflags & AF_REGEXP ) &&
                              regexp_match( buff + 1, new,
                                            ( ( aflags & AF_CASE ) ?
                                              0 : PCRE_CASELESS ),
                                            aargs, NUM_ENV_VARS ) ) )
                            && ( !mudconf.addcmd_obey_uselocks
                                 || could_doit( player,
                                                add->thing, A_LUSE ) ) ) {
                        process_cmdline( ( ( !( cmdp->
                                                callseq &
                                                CS_ACTOR )
                                             || God( player ) ) ?
                                           add->thing : player ),
                                         player, s, aargs,
                                         NUM_ENV_VARS, NULL );
                        for( i = 0; i < NUM_ENV_VARS;
                                i++ ) {
                            if( aargs[i] )
                                free_lbuf( aargs
                                           [i] );
                        }
                        cmd_matches++;
                    }
                    free_lbuf( buff );
                    if( cmd_matches
                            && mudconf.addcmd_obey_stop
                            && Stop_Match( add->thing ) ) {
                        break;
                    }
                }

                if( !cmd_matches
                        && !mudconf.addcmd_match_blindly ) {
                    /*
                     * The command the player typed
                     * didn't match any of the wildcard
                     * patterns we have for that
                     * addcommand. We should raise an
                     * error. We DO NOT go back into
                     * trying to match other stuff --
                     * this is a 'Huh?' situation.
                     */
                    notify( player, mudconf.huh_msg );
                    pname = log_getname( player, "process_cmdent" );
                    if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( player ) ) {
                        lname = log_getname( Location( player ), "process_cmdent" );
                        log_write( LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s", pname, lname, new );
                        XFREE( lname, "process_cmdent" );
                    } else {
                        log_write( LOG_BADCOMMANDS, "CMD", "BAD", "%s entered: %s", pname, new );
                    }
                    XFREE( pname, "process_cmdent" );
                }
                free_lbuf( new );

                restore_global_regs( "process_cmdent",
                                     preserve );
            } else
                ( * ( cmdp->info.handler ) )( player, cause, key,
                                              buf1 );
        }

        /*
         * Free the buffer if one was allocated
         */

        if( ( interp & EV_EVAL ) && !( cmdp->callseq & CS_ADDED ) ) {
            free_lbuf( buf1 );
        }

        break;
    case CS_TWO_ARG:	/* <cmd> <arg1> = <arg2> */

        /*
         * Interpret ARG1
         */

        buf2 = parse_to( &arg, '=', EV_STRIP_TS );

        /*
         * Handle when no '=' was specified
         */

        if( !arg || ( arg && !*arg ) ) {
            arg = &tchar;
            *arg = '\0';
        }
        buf1 = bp = alloc_lbuf( "process_cmdent.2" );
        str = buf2;
        exec( buf1, &bp, player, cause, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL | EV_TOP,
              &str, cargs, ncargs );

        if( cmdp->callseq & CS_ARGV ) {

            /*
             * Arg2 is ARGV style.  Go get the args
             */

            parse_arglist( player, cause, cause, arg, '\0',
                           interp | EV_STRIP_LS | EV_STRIP_TS,
                           args, MAX_ARG, cargs, ncargs );
            for( nargs = 0; ( nargs < MAX_ARG ) && args[nargs];
                    nargs++ );

            /*
             * Call the correct command handler
             */

            if( cmdp->callseq & CS_CMDARG ) {
                ( * ( cmdp->info.handler ) )( player, cause, key,
                                              buf1, args, nargs, cargs, ncargs );
            } else {
                ( * ( cmdp->info.handler ) )( player, cause, key,
                                              buf1, args, nargs );
            }

            /*
             * Free the argument buffers
             */

            for( i = 0; i <= nargs; i++ )
                if( args[i] ) {
                    free_lbuf( args[i] );
                }

        } else {

            /*
             * Arg2 is normal style.  Interpret if needed
             */


            if( interp & EV_EVAL ) {
                buf2 = bp = alloc_lbuf( "process_cmdent.3" );
                str = arg;
                exec( buf2, &bp, player, cause, cause,
                      interp | EV_FCHECK | EV_TOP,
                      &str, cargs, ncargs );
            } else if( cmdp->callseq & CS_UNPARSE ) {
                buf2 = parse_to( &arg, '\0',
                                 interp | EV_TOP | EV_NO_COMPRESS );
            } else {
                buf2 = parse_to( &arg, '\0',
                                 interp | EV_STRIP_LS | EV_STRIP_TS |
                                 EV_TOP );
            }

            /*
             * Call the correct command handler
             */

            if( cmdp->callseq & CS_CMDARG ) {
                ( * ( cmdp->info.handler ) )( player, cause, key,
                                              buf1, buf2, cargs, ncargs );
            } else {
                ( * ( cmdp->info.handler ) )( player, cause, key,
                                              buf1, buf2 );
            }

            /*
             * Free the buffer, if needed
             */

            if( interp & EV_EVAL ) {
                free_lbuf( buf2 );
            }
        }

        /*
         * Free the buffer obtained by evaluating Arg1
         */

        free_lbuf( buf1 );
        break;
    }

    /*
     * And now we go do the posthook, if we have one.
     */

    CALL_POST_HOOK( cmdp, cargs, ncargs );

    return;
}

/*
 * ---------------------------------------------------------------------------
 * process_command: Execute a command.
 */

char *process_command( dbref player, dbref cause, int interactive, char *command, char *args[], int nargs ) {
    static char preserve_cmd[LBUF_SIZE];

    char *p, *q, *arg, *lcbuf, *slashp, *cmdsave, *bp, *str, *evcmd;

    char *gbuf, *gc, *pname, *lname;

    int succ, aflags, alen, i, got_stop, pcount, retval = 0;

    dbref exit, aowner, parent;

    CMDENT *cmdp;

    NUMBERTAB *np;

    if( mudstate.cmd_invk_ctr == mudconf.cmd_invk_lim ) {
        return command;
    }
    mudstate.cmd_invk_ctr++;

    /*
     * Robustify player
     */

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = ( char * ) "< process_command >";

    if( !command ) {
        log_write_raw( 1, "ABORT! command.c, null command in process_command().\n" );
        abort();
    }
    if( !Good_obj( player ) ) {
        log_write( LOG_BUGS, "CMD", "PLYR", "Bad player in process_command: %d", player );
        mudstate.debug_cmd = cmdsave;
        return command;
    }
    /*
     * Make sure player isn't going or halted
     */

    if( Going( player ) || ( Halted( player ) && !( ( Typeof( player ) == TYPE_PLAYER ) && interactive ) ) ) {
        notify_check( Owner( player ), Owner( player ), MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Attempt to execute command by halted object #%d", player );
        mudstate.debug_cmd = cmdsave;
        return command;
    }
    pname = log_getname( player, "process_command" );
    if( Suspect( player ) ) {
        if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( player ) ) {
            lname = log_getname( Location( player ), "process_command" );
            log_write( LOG_SUSPECTCMDS, "CMD", "SUSP", "%s in %s entered: %s", pname, lname, command );
            XFREE( lname, "process_command" );
        } else {
            log_write( LOG_SUSPECTCMDS, "CMD", "SUSP", "%s entered: %s", pname, command );
        }
    } else {
        if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( player ) ) {
            lname = log_getname( Location( player ), "process_command" );
            log_write( LOG_SUSPECTCMDS, "CMD", "ALL", "%s in %s entered: %s", pname, lname, command );
            XFREE( lname, "process_command" );
        } else {
            log_write( LOG_SUSPECTCMDS, "CMD", "ALL", "%s entered: %s", pname, command );
        }
    }
    XFREE( pname, "process_command" );

    s_Accessed( player );

    /*
     * Reset recursion and other limits. Baseline the CPU counter.
     */

    mudstate.func_nest_lev = 0;
    mudstate.func_invk_ctr = 0;
    mudstate.f_limitmask = 0;
    mudstate.ntfy_nest_lev = 0;
    mudstate.lock_nest_lev = 0;
    if( mudconf.func_cpu_lim > 0 ) {
        mudstate.cputime_base = clock();
    }

    if( Verbose( player ) ) {
        if( H_Redirect( player ) ) {
            np = ( NUMBERTAB * ) nhashfind( player, &mudstate.redir_htab );
            if( np ) {
                notify_check( np->num, np->num, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s] %s", Name( player ), command );
            } else {
                /*
                 * We have no pointer, we should have no
                 * flag.
                 */
                s_Flags3( player, Flags3( player ) & ~HAS_REDIRECT );
            }
        } else {
            notify_check( Owner( player ), Owner( player ), MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s] %s", Name( player ), command );
        }
    }
    /*
     * NOTE THAT THIS WILL BREAK IF "GOD" IS NOT A DBREF.
     */
    if( mudconf.control_flags & CF_GODMONITOR ) {
        raw_notify( GOD, "%s(#%d)%c %s", Name( player ), player,( interactive ) ? '|' : ':', command );
    }
    /*
     * Eat leading whitespace, and space-compress if configured
     */

    while( *command && isspace( *command ) ) {
        command++;
    }

    strcpy( preserve_cmd, command );
    mudstate.debug_cmd = command;
    mudstate.curr_cmd = preserve_cmd;

    if( mudconf.space_compress ) {
        p = q = command;
        while( *p ) {
            while( *p && !isspace( *p ) ) {
                *q++ = *p++;
            }
            while( *p && isspace( *p ) ) {
                p++;
            }
            if( *p ) {
                *q++ = ' ';
            }
        }
        *q = '\0';
    }
    /*
     * Allow modules to intercept command strings.
     */

    CALL_SOME_MODULES( retval, process_command,
                       ( player, cause, interactive, command, args, nargs ) );
    if( retval > 0 ) {
        mudstate.debug_cmd = cmdsave;
        return preserve_cmd;
    }
    /*
     * Now comes the fun stuff.  First check for single-letter leadins.
     * We check these before checking HOME because they are among the
     * most frequently executed commands, and they can never be the HOME
     * command.
     */

    i = command[0] & 0xff;
    if( ( prefix_cmds[i] != NULL ) && command[0] ) {
        process_cmdent( prefix_cmds[i], NULL, player, cause,
                        interactive, command, command, args, nargs );
        mudstate.debug_cmd = cmdsave;
        return preserve_cmd;
    }
    /*
     * Check for the HOME command. You cannot do hooks on this because
     * home is not part of the traditional command table.
     */

    if( Has_location( player ) && string_compare( command, "home" ) == 0 ) {
        if( ( ( Fixed( player ) ) || ( Fixed( Owner( player ) ) ) ) &&
                !( WizRoy( player ) ) ) {
            notify( player, mudconf.fixed_home_msg );
            mudstate.debug_cmd = cmdsave;
            return preserve_cmd;
        }
        do_move( player, cause, 0, "home" );
        mudstate.debug_cmd = cmdsave;
        return preserve_cmd;
    }
    /*
     * Only check for exits if we may use the goto command
     */

    if( Check_Cmd_Access( player, goto_cmdp, args, nargs ) ) {

        /*
         * Check for an exit name
         */

        init_match_check_keys( player, command, TYPE_EXIT );
        match_exit_with_parents();
        exit = last_match_result();
        if( exit != NOTHING ) {
            if( mudconf.exit_calls_move ) {
                /*
                 * Exits literally call the 'move' command.
                 * Note that, later, when we go to matching
                 * master-room and other global-ish exits,
                 * that we also need to have move_match_more
                 * set to 'yes', or we'll match here only to
                 * encounter dead silence when we try to find
                 * the exit inside the move routine. We also
                 * need to directly find what the pointer for
                 * the move (goto) command is, since we could
                 * have @addcommand'd it (and probably did,
                 * if this conf option is on). Finally, we've
                 * got to make this look like we really did
                 * type 'goto <exit>', or the @addcommand
                 * will just skip over the string.
                 */
                cmdp = ( CMDENT * ) hashfind( "goto",
                                              &mudstate.command_htab );
                if( cmdp ) {	/* just in case */
                    gbuf =
                        alloc_lbuf( "process_command.goto" );
                    gc = gbuf;
                    safe_str( cmdp->cmdname, gbuf, &gc );
                    safe_chr( ' ', gbuf, &gc );
                    safe_str( command, gbuf, &gc );
                    *gc = '\0';
                    process_cmdent( cmdp, NULL, player,
                                    cause, interactive, command, gbuf,
                                    args, nargs );
                    free_lbuf( gbuf );
                }
            } else {
                /*
                 * Execute the pre-hook for the goto command
                 */
                CALL_PRE_HOOK( goto_cmdp, args, nargs );
                move_exit( player, exit, 0, NOGO_MESSAGE, 0 );
                /*
                 * Execute the post-hook for the goto command
                 */
                CALL_POST_HOOK( goto_cmdp, args, nargs );
            }
            mudstate.debug_cmd = cmdsave;
            return preserve_cmd;
        }
        /*
         * Check for an exit in the master room
         */

        init_match_check_keys( player, command, TYPE_EXIT );
        match_master_exit();
        exit = last_match_result();
        if( exit != NOTHING ) {
            if( mudconf.exit_calls_move ) {
                cmdp = ( CMDENT * ) hashfind( "goto",
                                              &mudstate.command_htab );
                if( cmdp ) {
                    gbuf =
                        alloc_lbuf( "process_command.goto" );
                    gc = gbuf;
                    safe_str( cmdp->cmdname, gbuf, &gc );
                    safe_chr( ' ', gbuf, &gc );
                    safe_str( command, gbuf, &gc );
                    *gc = '\0';
                    process_cmdent( cmdp, NULL, player,
                                    cause, interactive, command, gbuf,
                                    args, nargs );
                    free_lbuf( gbuf );
                }
            } else {
                CALL_PRE_HOOK( goto_cmdp, args, nargs );
                move_exit( player, exit, 1, NOGO_MESSAGE, 0 );
                CALL_POST_HOOK( goto_cmdp, args, nargs );
            }
            mudstate.debug_cmd = cmdsave;
            return preserve_cmd;
        }
    }
    /*
     * Set up a lowercase command and an arg pointer for the hashed
     * command check.  Since some types of argument processing destroy
     * the arguments, make a copy so that we keep the original command
     * line intact.  Store the edible copy in lcbuf after the lowercased
     * command.
     */
    /*
     * Removed copy of the rest of the command, since it's ok to allow it
     * to be trashed.  -dcm
     */

    lcbuf = alloc_lbuf( "process_commands.lcbuf" );
    for( p = command, q = lcbuf; *p && !isspace( *p ); p++, q++ ) {
        *q = tolower( *p );     /* Make lowercase command */
    }
    *q++ = '\0';		/* Terminate command */
    while( *p && isspace( *p ) ) {
        p++;    /* Skip spaces before arg */
    }
    arg = p;		/* Remember where arg starts */

    /*
     * Strip off any command switches and save them
     */

    slashp = strchr( lcbuf, '/' );
    if( slashp ) {
        *slashp++ = '\0';
    }

    /*
     * Check for a builtin command (or an alias of a builtin command)
     */

    cmdp = ( CMDENT * ) hashfind( lcbuf, &mudstate.command_htab );
    if( cmdp != NULL ) {
        if( mudconf.space_compress && ( cmdp->callseq & CS_NOSQUISH ) ) {
            /*
             * We handle this specially -- there is no space
             * compression involved, so we must go back to the
             * preserved command.
             */
            strcpy( command, preserve_cmd );
            arg = command;
            while( *arg && !isspace( *arg ) ) {
                arg++;
            }
            if( *arg )	/* we stopped on the space, advance
					 * to next */
            {
                arg++;
            }
        }
        process_cmdent( cmdp, slashp, player, cause, interactive, arg,
                        command, args, nargs );
        free_lbuf( lcbuf );
        mudstate.debug_cmd = cmdsave;
        return preserve_cmd;
    }
    /*
     * Check for enter and leave aliases, user-defined commands on the
     * player, other objects where the player is, on objects in the
     * player's inventory, and on the room that holds the player. We
     * evaluate the command line here to allow chains of $-commands to
     * work.
     */

    str = evcmd = alloc_lbuf( "process_command.evcmd" );
    strcpy( evcmd, command );
    bp = lcbuf;
    exec( lcbuf, &bp, player, cause, cause,
          EV_EVAL | EV_FCHECK | EV_STRIP | EV_TOP, &str, args, nargs );
    free_lbuf( evcmd );
    succ = 0;

    /*
     * Idea for enter/leave aliases from R'nice@TinyTIM
     */

    if( Has_location( player ) && Good_obj( Location( player ) ) ) {

        /*
         * Check for a leave alias, if we have permissions to use the
         * 'leave' command.
         */

        if( Check_Cmd_Access( player, leave_cmdp, args, nargs ) ) {
            p = atr_pget( Location( player ), A_LALIAS, &aowner,
                          &aflags, &alen );
            if( *p ) {
                if( matches_exit_from_list( lcbuf, p ) ) {
                    free_lbuf( lcbuf );
                    free_lbuf( p );
                    CALL_PRE_HOOK( leave_cmdp, args, nargs );
                    do_leave( player, player, 0 );
                    CALL_POST_HOOK( leave_cmdp, args,
                                    nargs );
                    return preserve_cmd;
                }
            }
            free_lbuf( p );
        }
        /*
         * Check for enter aliases, if we have permissions to use the
         * 'enter' command.
         */

        if( Check_Cmd_Access( player, enter_cmdp, args, nargs ) ) {
            DOLIST( exit, Contents( Location( player ) ) ) {
                p = atr_pget( exit, A_EALIAS, &aowner, &aflags,
                              &alen );
                if( *p ) {
                    if( matches_exit_from_list( lcbuf, p ) ) {
                        free_lbuf( lcbuf );
                        free_lbuf( p );
                        CALL_PRE_HOOK( enter_cmdp, args,
                                       nargs );
                        do_enter_internal( player, exit,
                                           0 );
                        CALL_POST_HOOK( enter_cmdp,
                                        args, nargs );
                        return preserve_cmd;
                    }
                }
                free_lbuf( p );
            }
        }
    }
    /*
     * At each of the following stages, we check to make sure that we
     * haven't hit a match on a STOP-set object.
     */

    got_stop = 0;

    /*
     * Check for $-command matches on me
     */

    if( mudconf.match_mine ) {
        if( ( ( Typeof( player ) != TYPE_PLAYER ) ||
                mudconf.match_mine_pl ) &&
                ( atr_match( player, player, AMATCH_CMD, lcbuf, preserve_cmd,
                             1 ) > 0 ) ) {
            succ++;
            got_stop = Stop_Match( player );
        }
    }
    /*
     * Check for $-command matches on nearby things and on my room
     */

    if( !got_stop && Has_location( player ) ) {
        succ += list_check( Contents( Location( player ) ), player,
                            AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop );

        if( !got_stop &&
                atr_match( Location( player ), player, AMATCH_CMD, lcbuf,
                           preserve_cmd, 1 ) > 0 ) {
            succ++;
            got_stop = Stop_Match( Location( player ) );
        }
    }
    /*
     * Check for $-command matches in my inventory
     */

    if( !got_stop && Has_contents( player ) )
        succ += list_check( Contents( player ), player,
                            AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop );

    /*
     * If we didn't find anything, and we're checking local masters, do
     * those checks. Do it for the zone of the player's location first,
     * and then, if nothing is found, on the player's personal zone.
     * Walking back through the parent tree stops when a match is found.
     * Also note that these matches are done in the style of the master
     * room: parents of the contents of the rooms aren't checked for
     * commands. We try to maintain 2.2/MUX compatibility here, putting
     * both sets of checks together.
     */

    if( Has_location( player ) && Good_obj( Location( player ) ) ) {

        /*
         * 2.2 style location
         */

        if( !succ && mudconf.local_masters ) {
            pcount = 0;
            parent = Parent( Location( player ) );
            while( !succ && !got_stop &&
                    Good_obj( parent ) && ParentZone( parent ) &&
                    ( pcount < mudconf.parent_nest_lim ) ) {
                if( Has_contents( parent ) ) {
                    succ +=
                        list_check( Contents( parent ),
                                    player, AMATCH_CMD, lcbuf,
                                    preserve_cmd,
                                    mudconf.match_zone_parents,
                                    &got_stop );
                }
                parent = Parent( parent );
                pcount++;
            }
        }
        /*
         * MUX style location
         */

        if( ( !succ ) && mudconf.have_zones &&
                ( Zone( Location( player ) ) != NOTHING ) ) {
            if( Typeof( Zone( Location( player ) ) ) == TYPE_ROOM ) {

                /*
                 * zone of player's location is a parent room
                 */
                if( Location( player ) != Zone( player ) ) {
                    /*
                     * check parent room exits
                     */
                    init_match_check_keys( player, command,
                                           TYPE_EXIT );
                    match_zone_exit();
                    exit = last_match_result();
                    if( exit != NOTHING ) {
                        if( mudconf.exit_calls_move ) {
                            cmdp =
                                ( CMDENT * )
                                hashfind( "goto",
                                          &mudstate.
                                          command_htab );
                            if( cmdp ) {
                                gbuf =
                                    alloc_lbuf
                                    ( "process_command.goto" );
                                gc = gbuf;
                                safe_str( cmdp->
                                          cmdname,
                                          gbuf, &gc );
                                safe_chr( ' ',
                                          gbuf, &gc );
                                safe_str
                                ( command,
                                  gbuf, &gc );
                                *gc = '\0';
                                process_cmdent
                                ( cmdp,
                                  NULL,
                                  player,
                                  cause,
                                  interactive,
                                  command,
                                  gbuf, args,
                                  nargs );
                                free_lbuf
                                ( gbuf );
                            }
                        } else {
                            CALL_PRE_HOOK
                            ( goto_cmdp, args,
                              nargs );
                            move_exit( player, exit,
                                       1, NOGO_MESSAGE,
                                       0 );
                            CALL_POST_HOOK
                            ( goto_cmdp, args,
                              nargs );
                        }
                        mudstate.debug_cmd = cmdsave;
                        return preserve_cmd;
                    }
                    if( !got_stop ) {
                        succ +=
                            list_check( Contents( Zone
                                                  ( Location( player ) ) ),
                                        player, AMATCH_CMD, lcbuf,
                                        preserve_cmd, 1,
                                        &got_stop );

                    }
                }	/* end of parent room checks */
            } else
                /*
                 * try matching commands on area zone object
                 */

                if( !got_stop && !succ && mudconf.have_zones
                        && ( Zone( Location( player ) ) != NOTHING ) ) {
                    succ +=
                        atr_match( Zone( Location( player ) ), player,
                                   AMATCH_CMD, lcbuf, preserve_cmd, 1 );
                }
        }		/* end of matching on zone of player's
				 * location */
    }
    /*
     * 2.2 style player
     */

    if( !succ && mudconf.local_masters ) {
        parent = Parent( player );
        if( !Has_location( player ) || !Good_obj( Location( player ) ) ||
                ( ( parent != Location( player ) ) &&
                  ( parent != Parent( Location( player ) ) ) ) ) {
            pcount = 0;
            while( !succ && !got_stop &&
                    Good_obj( parent ) && ParentZone( parent ) &&
                    ( pcount < mudconf.parent_nest_lim ) ) {
                if( Has_contents( parent ) ) {
                    succ +=
                        list_check( Contents( parent ),
                                    player, AMATCH_CMD, lcbuf,
                                    preserve_cmd, 0, &got_stop );
                }
                parent = Parent( parent );
                pcount++;
            }
        }
    }
    /*
     * MUX style player
     */

    /*
     * if nothing matched with parent room/zone object, try matching zone
     * commands on the player's personal zone
     */
    if( !got_stop && !succ && mudconf.have_zones &&
            ( Zone( player ) != NOTHING ) &&
            ( !Has_location( player ) || !Good_obj( Location( player ) ) ||
              ( Zone( Location( player ) ) != Zone( player ) ) ) ) {
        succ += atr_match( Zone( player ), player, AMATCH_CMD, lcbuf,
                           preserve_cmd, 1 );
    }
    /*
     * If we didn't find anything, try in the master room
     */

    if( !got_stop && !succ ) {
        if( Good_loc( mudconf.master_room ) ) {
            succ += list_check( Contents( mudconf.master_room ),
                                player, AMATCH_CMD, lcbuf,
                                preserve_cmd, 0, &got_stop );
            if( !got_stop && atr_match( mudconf.master_room,
                                        player, AMATCH_CMD, lcbuf, preserve_cmd,
                                        0 ) > 0 ) {
                succ++;
            }
        }
    }
    /*
     * Allow modules to intercept, if still no match. This time we pass
     * both the lower-cased evaluated buffer and the preserved command.
     */

    if( !succ ) {
        CALL_SOME_MODULES( succ, process_no_match,
                           ( player, cause, interactive, lcbuf, preserve_cmd,
                             args, nargs ) );
    }
    free_lbuf( lcbuf );

    /*
     * If we still didn't find anything, tell how to get help.
     */

    if( !succ ) {
        notify( player, mudconf.huh_msg );
        pname = log_getname( player, "process_command" );
        if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( player ) ) {
            lname = log_getname( Location( player ), "process_command" );
            log_write( LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s", pname, lname, command );
            XFREE( lname, "process_command" );
        } else {
            log_write( LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s", pname, command );
        }
        XFREE( pname, "process_command" );
    }
    mudstate.debug_cmd = cmdsave;
    return preserve_cmd;
}

/*
 *
 * --------------------------------------------------------------------------- *
 * process_cmdline: Execute a semicolon/pipe-delimited series of commands.
 */

void process_cmdline( dbref player, dbref cause, char *cmdline, char *args[], int nargs, BQUE *qent ) {
    char *cp, *cmdsave, *save_poutnew, *save_poutbufc, *save_pout;

    char *log_cmdbuf, *pname, *lname;

    int save_inpipe, numpipes;

    dbref save_poutobj, save_enactor, save_player;

#ifndef NO_LAG_CHECK
    struct timeval begin_time, end_time;

    int used_time;

#ifndef NO_TIMECHECKING
    struct timeval obj_time;

#endif
#ifdef TRACK_USER_TIME
    struct rusage usage;

    struct timeval b_utime, e_utime;

#endif
#endif

    if( mudstate.cmd_nest_lev == mudconf.cmd_nest_lim ) {
        return;
    }
    mudstate.cmd_nest_lev++;

    cmdsave = mudstate.debug_cmd;
    save_enactor = mudstate.curr_enactor;
    save_player = mudstate.curr_player;
    mudstate.curr_enactor = cause;
    mudstate.curr_player = player;

    save_inpipe = mudstate.inpipe;
    save_poutobj = mudstate.poutobj;
    save_poutnew = mudstate.poutnew;
    save_poutbufc = mudstate.poutbufc;
    save_pout = mudstate.pout;

    mudstate.break_called = 0;

    while( cmdline && ( !qent || qent == mudstate.qfirst ) &&
            !mudstate.break_called ) {
        cp = parse_to( &cmdline, ';', 0 );
        if( cp && *cp ) {
            numpipes = 0;
            while( cmdline && ( *cmdline == '|' ) &&
                    ( !qent || qent == mudstate.qfirst ) &&
                    ( numpipes < mudconf.ntfy_nest_lim ) ) {
                cmdline++;
                numpipes++;

                mudstate.inpipe = 1;
                mudstate.poutnew =
                    alloc_lbuf( "process_cmdline.pipe" );
                mudstate.poutbufc = mudstate.poutnew;
                mudstate.poutobj = player;
                mudstate.debug_cmd = cp;

                /*
                 * No lag check on piped commands
                 */
                process_command( player, cause, 0, cp,
                                 args, nargs );
                if( mudstate.pout
                        && mudstate.pout != save_pout ) {
                    free_lbuf( mudstate.pout );
                    mudstate.pout = NULL;
                }
                *mudstate.poutbufc = '\0';
                mudstate.pout = mudstate.poutnew;
                cp = parse_to( &cmdline, ';', 0 );
            }

            mudstate.inpipe = save_inpipe;
            mudstate.poutnew = save_poutnew;
            mudstate.poutbufc = save_poutbufc;
            mudstate.poutobj = save_poutobj;
            mudstate.debug_cmd = cp;

            /*
             * Is the queue still linked like we think it is?
             */
            if( qent && qent != mudstate.qfirst ) {
                if( mudstate.pout
                        && mudstate.pout != save_pout ) {
                    free_lbuf( mudstate.pout );
                    mudstate.pout = NULL;
                }
                break;
            }
#ifndef NO_LAG_CHECK
            get_tod( &begin_time );
#ifdef TRACK_USER_TIME
            getrusage( RUSAGE_SELF, &usage );
            b_utime.tv_sec = usage.ru_utime.tv_sec;
            b_utime.tv_usec = usage.ru_utime.tv_usec;
#endif
#endif				/* ! NO_LAG_CHECK */

            log_cmdbuf = process_command( player, cause,
                                          0, cp, args, nargs );

            if( mudstate.pout && mudstate.pout != save_pout ) {
                free_lbuf( mudstate.pout );
                mudstate.pout = save_pout;
            }
            save_poutbufc = mudstate.poutbufc;
#ifndef NO_LAG_CHECK
            get_tod( &end_time );
#ifdef TRACK_USER_TIME
            getrusage( RUSAGE_SELF, &usage );
            e_utime.tv_sec = usage.ru_utime.tv_sec;
            e_utime.tv_usec = usage.ru_utime.tv_usec;
#endif
            used_time = msec_diff( end_time, begin_time );
            if( ( used_time / 1000 ) >= mudconf.max_cmdsecs ) {
                pname = log_getname( player, "process_cmdline" );
                if( ( mudconf.log_info & LOGOPT_LOC ) && Has_location( player ) ) {
                    lname = log_getname( Location( player ), "process_cmdline" );
                    log_write( LOG_PROBLEMS, "CMD", "CPU","%s in %s queued command taking %.2f secs (enactor #%d): %s", pname, lname, ( double )( used_time / 1000 ), mudstate.qfirst->cause, log_cmdbuf );
                    XFREE( lname, "process_cmdline" );
                } else {
                    log_write( LOG_PROBLEMS, "CMD", "CPU","%s queued command taking %.2f secs (enactor #%d): %s", pname, ( double )( used_time / 1000 ), mudstate.qfirst->cause, log_cmdbuf );
                }
                XFREE( pname, "process_cmdline" );
            }
#ifndef NO_TIMECHECKING
            /*
             * Don't use msec_add(), this is more accurate
             */

            obj_time = Time_Used( player );
#ifndef TRACK_USER_TIME
            obj_time.tv_usec += end_time.tv_usec -
                                begin_time.tv_usec;
            obj_time.tv_sec += end_time.tv_sec - begin_time.tv_sec;
#else
            obj_time.tv_usec += e_utime.tv_usec - b_utime.tv_usec;
            obj_time.tv_sec += e_utime.tv_sec - b_utime.tv_sec;
#endif				/* ! TRACK_USER_TIME */
            if( obj_time.tv_usec < 0 ) {
                obj_time.tv_usec += 1000000;
                obj_time.tv_sec--;
            } else if( obj_time.tv_usec >= 1000000 ) {
                obj_time.tv_sec += obj_time.tv_usec / 1000000;
                obj_time.tv_usec = obj_time.tv_usec % 1000000;
            }
            s_Time_Used( player, obj_time );
#endif				/* ! NO_TIMECHECKING */
#endif				/* ! NO_LAG_CHECK */
        }
    }

    mudstate.debug_cmd = cmdsave;
    mudstate.curr_enactor = save_enactor;
    mudstate.curr_player = save_player;

    mudstate.cmd_nest_lev--;
}

/*
 * ---------------------------------------------------------------------------
 * list_cmdtable: List internal commands. Note that user-defined command
 * permissions are ignored in this context.
 */

static void list_cmdtable( dbref player ) {
    CMDENT *cmdp, *modcmds;

    char *buf, *bp;

    MODULE *mp;

    buf = alloc_lbuf( "list_cmdtable" );
    bp = buf;
    safe_str( ( char * ) "Built-in commands:", buf, &bp );
    for( cmdp = command_table; cmdp->cmdname; cmdp++ ) {
        if( check_access( player, cmdp->perms ) ) {
            if( !( cmdp->perms & CF_DARK ) ) {
                safe_chr( ' ', buf, &bp );
                safe_str( ( char * ) cmdp->cmdname, buf, &bp );
            }
        }
    }

    /*
     * Players get the list of logged-out cmds too
     */

    if( isPlayer( player ) ) {
        display_nametab( player, logout_cmdtable, buf, 1 );
    } else {
        notify( player, buf );
    }

    WALK_ALL_MODULES( mp ) {
        if( ( modcmds = DLSYM_VAR( mp->handle, mp->modname, "cmdtable", CMDENT * ) ) != NULL ) {
            bp = buf;
            safe_sprintf( buf, &bp, "Module %s commands:", mp->modname );
            for( cmdp = modcmds; cmdp->cmdname; cmdp++ ) {
                if( check_access( player, cmdp->perms ) ) {
                    if( !( cmdp->perms & CF_DARK ) ) {
                        safe_chr( ' ', buf, &bp );
                        safe_str( ( char * ) cmdp->cmdname, buf, &bp );
                    }
                }
            }
            notify( player, buf );
        }
    }

    free_lbuf( buf );
}

/*
 * ---------------------------------------------------------------------------
 * list_attrtable: List available attributes.
 */

static void list_attrtable( dbref player ) {
    ATTR *ap;

    char *buf, *bp, *cp;

    buf = alloc_lbuf( "list_attrtable" );
    bp = buf;
    for( cp = ( char * ) "Attributes:"; *cp; cp++ ) {
        *bp++ = *cp;
    }
    for( ap = attr; ap->name; ap++ ) {
        if( See_attr( player, player, ap, player, 0 ) ) {
            *bp++ = ' ';
            for( cp = ( char * )( ap->name ); *cp; cp++ ) {
                *bp++ = *cp;
            }
        }
    }
    *bp = '\0';
    raw_notify( player, NULL, buf );
    free_lbuf( buf );
}

/*
 * ---------------------------------------------------------------------------
 * list_cmdaccess: List access commands.
 */

static void helper_list_cmdaccess( dbref player, CMDENT *ctab, char *buff ) {
    CMDENT *cmdp;

    ATTR *ap;

    for( cmdp = ctab; cmdp->cmdname; cmdp++ ) {
        if( check_access( player, cmdp->perms ) ) {
            if( !( cmdp->perms & CF_DARK ) ) {
                if( cmdp->userperms ) {
                    ap = atr_num( cmdp->userperms->atr );
                    if( !ap ) {
                        sprintf( buff,
                                 "%s: user(#%d/?BAD?)",
                                 cmdp->cmdname,
                                 cmdp->userperms->thing );
                    } else {
                        sprintf( buff,
                                 "%s: user(#%d/%s)",
                                 cmdp->cmdname,
                                 cmdp->userperms->thing,
                                 ap->name );
                    }
                } else {
                    sprintf( buff, "%s:", cmdp->cmdname );
                }
                listset_nametab( player, access_nametab,
                                 cmdp->perms, buff, 1 );
            }
        }
    }
}

static void list_cmdaccess( dbref player ) {
    char *buff, *p, *q;

    CMDENT *cmdp, *ctab;

    ATTR *ap;

    MODULE *mp;

    buff = alloc_sbuf( "list_cmdaccess" );
    helper_list_cmdaccess( player, command_table, buff );

    WALK_ALL_MODULES( mp ) {
        if( ( ctab = DLSYM_VAR( mp->handle, mp->modname, "cmdtable",
                                CMDENT * ) ) != NULL ) {
            helper_list_cmdaccess( player, ctab, buff );
        }
    }

    for( ap = attr; ap->name; ap++ ) {
        p = buff;
        *p++ = '@';
        for( q = ( char * ) ap->name; *q; p++, q++ ) {
            *p = tolower( *q );
        }
        if( ap->flags & AF_NOCMD ) {
            continue;
        }
        *p = '\0';
        cmdp = ( CMDENT * ) hashfind( buff, &mudstate.command_htab );
        if( cmdp == NULL ) {
            continue;
        }
        if( !check_access( player, cmdp->perms ) ) {
            continue;
        }
        if( !( cmdp->perms & CF_DARK ) ) {
            sprintf( buff, "%s:", cmdp->cmdname );
            listset_nametab( player, access_nametab,
                             cmdp->perms, buff, 1 );
        }
    }
    free_sbuf( buff );
}

/*
 * ---------------------------------------------------------------------------
 * list_cmdswitches: List switches for commands.
 */

static void list_cmdswitches( dbref player ) {
    char *buff;

    CMDENT *cmdp, *ctab;

    MODULE *mp;

    buff = alloc_sbuf( "list_cmdswitches" );

    for( cmdp = command_table; cmdp->cmdname; cmdp++ ) {
        if( cmdp->switches ) {
            if( check_access( player, cmdp->perms ) ) {
                if( !( cmdp->perms & CF_DARK ) ) {
                    sprintf( buff, "%s:", cmdp->cmdname );
                    display_nametab( player, cmdp->switches,
                                     buff, 0 );
                }
            }
        }
    }

    WALK_ALL_MODULES( mp ) {
        if( ( ctab = DLSYM_VAR( mp->handle, mp->modname, "cmdtable",
                                CMDENT * ) ) != NULL ) {
            for( cmdp = ctab; cmdp->cmdname; cmdp++ ) {
                if( cmdp->switches ) {
                    if( check_access( player, cmdp->perms ) ) {
                        if( !( cmdp->perms & CF_DARK ) ) {
                            sprintf( buff, "%s:",
                                     cmdp->cmdname );
                            display_nametab( player,
                                             cmdp->switches,
                                             buff, 0 );
                        }
                    }
                }
            }
        }
    }

    free_sbuf( buff );
}

/*
 * ---------------------------------------------------------------------------
 * list_attraccess: List access to attributes.
 */

static void list_attraccess( dbref player ) {
    char *buff;

    ATTR *ap;

    buff = alloc_sbuf( "list_attraccess" );
    for( ap = attr; ap->name; ap++ ) {
        if( Read_attr( player, player, ap, player, 0 ) ) {
            sprintf( buff, "%s:", ap->name );
            listset_nametab( player, attraccess_nametab,
                             ap->flags, buff, 1 );
        }
    }
    free_sbuf( buff );
}

/*
 * ---------------------------------------------------------------------------
 * list_attrtypes: List attribute "types" (wildcards and permissions)
 */

static void list_attrtypes( dbref player ) {
    char *buff;

    KEYLIST *kp;

    if( !mudconf.vattr_flag_list ) {
        notify( player, "No attribute type patterns defined." );
        return;
    }
    buff = alloc_sbuf( "list_attrtypes" );
    for( kp = mudconf.vattr_flag_list; kp != NULL; kp = kp->next ) {
        sprintf( buff, "%s:", kp->name );
        listset_nametab( player, attraccess_nametab, kp->data, buff, 1 );
    }
    free_sbuf( buff );
}

/*
 * ---------------------------------------------------------------------------
 * cf_access: Change command or switch permissions.
 */

int cf_access( int *vp, char *str, long extra, dbref player, char *cmd ) {
    CMDENT *cmdp;

    char *ap;

    int set_switch;

    for( ap = str; *ap && !isspace( *ap ) && ( *ap != '/' ); ap++ );
    if( *ap == '/' ) {
        set_switch = 1;
        *ap++ = '\0';
    } else {
        set_switch = 0;
        if( *ap ) {
            *ap++ = '\0';
        }
        while( *ap && isspace( *ap ) ) {
            ap++;
        }
    }

    cmdp = ( CMDENT * ) hashfind( str, &mudstate.command_htab );
    if( cmdp != NULL ) {
        if( set_switch )
            return cf_ntab_access( ( int * ) cmdp->switches, ap,
                                   extra, player, cmd );
        else
            return cf_modify_bits( & ( cmdp->perms ), ap,
                                   extra, player, cmd );
    } else {
        cf_log_notfound( player, cmd, "Command", str );
        return -1;
    }
}

/*
 * ---------------------------------------------------------------------------
 * cf_acmd_access: Change command permissions for all attr-setting cmds.
 */

int cf_acmd_access( int *vp, char *str, long extra, dbref player, char *cmd ) {
    CMDENT *cmdp;

    ATTR *ap;

    char *buff, *p, *q;

    int failure, save;

    buff = alloc_sbuf( "cf_acmd_access" );
    for( ap = attr; ap->name; ap++ ) {
        p = buff;
        *p++ = '@';
        for( q = ( char * ) ap->name; *q; p++, q++ ) {
            *p = tolower( *q );
        }
        *p = '\0';
        cmdp = ( CMDENT * ) hashfind( buff, &mudstate.command_htab );
        if( cmdp != NULL ) {
            save = cmdp->perms;
            failure = cf_modify_bits( & ( cmdp->perms ), str,
                                      extra, player, cmd );
            if( failure != 0 ) {
                cmdp->perms = save;
                free_sbuf( buff );
                return -1;
            }
        }
    }
    free_sbuf( buff );
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * cf_attr_access: Change access on an attribute.
 */

int cf_attr_access( int *vp, char *str, long extra, dbref player, char *cmd ) {
    ATTR *ap;

    char *sp;

    for( sp = str; *sp && !isspace( *sp ); sp++ );
    if( *sp ) {
        *sp++ = '\0';
    }
    while( *sp && isspace( *sp ) ) {
        sp++;
    }

    ap = atr_str( str );
    if( ap != NULL ) {
        return cf_modify_bits( & ( ap->flags ), sp, extra, player, cmd );
    } else {
        cf_log_notfound( player, cmd, "Attribute", str );
        return -1;
    }
}

/*
 * ---------------------------------------------------------------------------
 * cf_attr_type: Define attribute flags for new user-named attributes whose
 * names match a certain pattern.
 */

int cf_attr_type( int *vp, char *str, long extra, dbref player, char *cmd ) {
    char *privs;

    KEYLIST *kp;

    int succ;

    /*
     * Split our string into the attribute pattern and privileges. Also
     * uppercase it, while we're at it. Make sure it's not longer than an
     * attribute name can be.
     */

    for( privs = str; *privs && !isspace( *privs ); privs++ ) {
        *privs = toupper( *privs );
    }
    if( *privs ) {
        *privs++ = '\0';
    }
    while( *privs && isspace( *privs ) ) {
        privs++;
    }
    if( strlen( str ) >= VNAME_SIZE ) {
        str[VNAME_SIZE - 1] = '\0';
    }

    /*
     * Create our new data blob. Make sure that we're setting the privs
     * to something reasonable before trying to link it in. (If we're
     * not, an error will have been logged; we don't need to do it.)
     */

    kp = ( KEYLIST * ) XMALLOC( sizeof( KEYLIST ), "cf_attr_type.kp" );
    kp->data = 0;

    succ = cf_modify_bits( & ( kp->data ), privs, extra, player, cmd );
    if( succ < 0 ) {
        XFREE( kp, "cf_attr_type.kp" );
        return -1;
    }
    kp->name = XSTRDUP( str, "cf_attr_type.name" );
    kp->next = mudconf.vattr_flag_list;
    mudconf.vattr_flag_list = kp;

    return ( succ );
}

/*
 * ---------------------------------------------------------------------------
 * cf_cmd_alias: Add a command alias.
 */

int cf_cmd_alias( int *vp, char *str, long extra, dbref player, char *cmd ) {
    char *alias, *orig, *ap, *tokst;

    CMDENT *cmdp, *cmd2;

    NAMETAB *nt;

    int *hp;

    alias = strtok_r( str, " \t=,", &tokst );
    orig = strtok_r( NULL, " \t=,", &tokst );

    if( !orig ) {		/* we only got one argument to @alias. Bad. */
        cf_log_syntax( player, cmd, "Invalid original for alias %s",
                       alias );
        return -1;
    }
    if( alias[0] == '_' && alias[1] == '_' ) {
        cf_log_syntax( player, cmd,
                       "Alias %s would cause @addcommand conflict", alias );
        return -1;
    }
    for( ap = orig; *ap && ( *ap != '/' ); ap++ );
    if( *ap == '/' ) {

        /*
         * Switch form of command aliasing: create an alias for a
         * command + a switch
         */

        *ap++ = '\0';

        /*
         * Look up the command
         */

        cmdp = ( CMDENT * ) hashfind( orig, ( HASHTAB * ) vp );
        if( cmdp == NULL ) {
            cf_log_notfound( player, cmd, "Command", orig );
            return -1;
        }
        /*
         * Look up the switch
         */

        nt = find_nametab_ent( player, ( NAMETAB * ) cmdp->switches, ap );
        if( !nt ) {
            cf_log_notfound( player, cmd, "Switch", ap );
            return -1;
        }
        /*
         * Got it, create the new command table entry
         */

        cmd2 = ( CMDENT * ) XMALLOC( sizeof( CMDENT ), "cf_cmd_alias" );
        cmd2->cmdname = XSTRDUP( alias, "cf_cmd_alias.cmdname" );
        cmd2->switches = cmdp->switches;
        cmd2->perms = cmdp->perms | nt->perm;
        cmd2->extra = ( cmdp->extra | nt->flag ) & ~SW_MULTIPLE;
        if( !( nt->flag & SW_MULTIPLE ) ) {
            cmd2->extra |= SW_GOT_UNIQUE;
        }
        cmd2->callseq = cmdp->callseq;

        /*
         * KNOWN PROBLEM: We are not inheriting the hook that the
         * 'original' command had -- we will have to add it manually
         * (whereas an alias of a non-switched command is just
         * another hashtable entry for the same command pointer and
         * therefore gets the hook). This is preferable to having to
         * search the hashtable for hooks when a hook is deleted,
         * though.
         */
        cmd2->pre_hook = NULL;
        cmd2->post_hook = NULL;
        cmd2->userperms = NULL;

        cmd2->info.handler = cmdp->info.handler;
        if( hashadd( cmd2->cmdname, ( int * ) cmd2, ( HASHTAB * ) vp, 0 ) ) {
            XFREE( cmd2->cmdname, "cf_cmd_alias.cmdname" );
            XFREE( cmd2, "cf_cmd_alias" );
        }
    } else {

        /*
         * A normal (non-switch) alias
         */

        hp = hashfind( orig, ( HASHTAB * ) vp );
        if( hp == NULL ) {
            cf_log_notfound( player, cmd, "Entry", orig );
            return -1;
        }
        hashadd( alias, hp, ( HASHTAB * ) vp, HASH_ALIAS );
    }
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * list_df_flags: List default flags at create time.
 */

static void list_df_flags( dbref player ) {
    char *playerb, *roomb, *thingb, *exitb, *robotb, *stripb;

    playerb = decode_flags( player, mudconf.player_flags );
    roomb = decode_flags( player, mudconf.room_flags );
    exitb = decode_flags( player, mudconf.exit_flags );
    thingb = decode_flags( player, mudconf.thing_flags );
    robotb = decode_flags( player, mudconf.robot_flags );
    stripb = decode_flags( player, mudconf.stripped_flags );

    raw_notify( player, "Default flags: Players...P%s  Rooms...R%s  Exits...E%s  Things...%s  Robots...P%s  Stripped...%s", playerb, roomb, exitb, thingb, robotb, stripb );

    free_sbuf( playerb );
    free_sbuf( roomb );
    free_sbuf( exitb );
    free_sbuf( thingb );
    free_sbuf( robotb );
    free_sbuf( stripb );
}

/*
 * ---------------------------------------------------------------------------
 * list_costs: List the costs of things.
 */

#define coin_name(s)	(((s)==1) ? mudconf.one_coin : mudconf.many_coins)

static void list_costs( dbref player ) {
    char *buff;

    buff = alloc_mbuf( "list_costs" );
    *buff = '\0';
    if( mudconf.quotas ) {
        sprintf( buff, " and %d quota", mudconf.room_quota );
    }
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Digging a room costs %d %s%s.", mudconf.digcost, coin_name( mudconf.digcost ), buff );
    if( mudconf.quotas ) {
        sprintf( buff, " and %d quota", mudconf.exit_quota );
    }
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,  "Opening a new exit costs %d %s%s.", mudconf.opencost, coin_name( mudconf.opencost ), buff );
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Linking an exit, home, or dropto costs %d %s.", mudconf.linkcost, coin_name( mudconf.linkcost ) );
    if( mudconf.quotas ) {
        sprintf( buff, " and %d quota", mudconf.thing_quota );
    }
    if( mudconf.createmin == mudconf.createmax )
        raw_notify( player, "Creating a new thing costs %d %s%s.", mudconf.createmin, coin_name( mudconf.createmin ), buff );
    else
        raw_notify( player, "Creating a new thing costs between %d and %d %s%s.", mudconf.createmin, mudconf.createmax, mudconf.many_coins, buff );
    if( mudconf.quotas ) {
        sprintf( buff, " and %d quota", mudconf.player_quota );
    }
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Creating a robot costs %d %s%s.", mudconf.robotcost, coin_name( mudconf.robotcost ), buff );
    if( mudconf.killmin == mudconf.killmax ) {
        raw_notify( player, "Killing costs %d %s, with a %d%% chance of success.", mudconf.killmin, coin_name( mudconf.digcost ), ( mudconf.killmin * 100 ) / mudconf.killguarantee );
    } else {
        raw_notify( player, "Killing costs between %d and %d %s.", mudconf.killmin, mudconf.killmax, mudconf.many_coins );
        raw_notify( player, "You must spend %d %s to guarantee success.", mudconf.killguarantee, coin_name( mudconf.killguarantee ) );
    }
    raw_notify( player, "Computationally expensive commands and functions (ie: @entrances, @find, @search, @stats (with an argument or switch), search(), and stats()) cost %d %s.", mudconf.searchcost, coin_name( mudconf.searchcost ) );
    if( mudconf.machinecost > 0 )
        raw_notify( player, "Each command run from the queue costs 1/%d %s.", mudconf.machinecost, mudconf.one_coin );
    if( mudconf.waitcost > 0 ) {
        raw_notify( player, "A %d %s deposit is charged for putting a command on the queue.", mudconf.waitcost, mudconf.one_coin );
        raw_notify( player, "The deposit is refunded when the command is run or canceled." );
    }
    if( mudconf.sacfactor == 0 ) {
        sprintf( buff, "%d", mudconf.sacadjust );
    } else if( mudconf.sacfactor == 1 ) {
        if( mudconf.sacadjust < 0 )
            sprintf( buff, "<create cost> - %d",
                     -mudconf.sacadjust );
        else if( mudconf.sacadjust > 0 ) {
            sprintf( buff, "<create cost> + %d", mudconf.sacadjust );
        } else {
            sprintf( buff, "<create cost>" );
        }
    } else {
        if( mudconf.sacadjust < 0 )
            sprintf( buff, "(<create cost> / %d) - %d",
                     mudconf.sacfactor, -mudconf.sacadjust );
        else if( mudconf.sacadjust > 0 )
            sprintf( buff, "(<create cost> / %d) + %d",
                     mudconf.sacfactor, mudconf.sacadjust );
        else {
            sprintf( buff, "<create cost> / %d", mudconf.sacfactor );
        }
    }
    raw_notify( player, "The value of an object is %s.", buff );
    if( mudconf.clone_copy_cost )
        raw_notify( player, "The default value of cloned objects is the value of the original object." );
    else
        raw_notify( player, "The default value of cloned objects is %d %s.", mudconf.createmin, coin_name( mudconf.createmin ) );

    free_mbuf( buff );
}

/*
 * ---------------------------------------------------------------------------
 * list_options: List boolean game options from mudconf. list_config: List
 * non-boolean game options.
 */

extern void list_options( dbref );

static void list_params( dbref player ) {
    time_t now;

    now = time( NULL );

    raw_notify( player, "Prototypes:  Room...#%d  Exit...#%d  Thing...#%d  Player...#%d", mudconf.room_proto, mudconf.exit_proto, mudconf.thing_proto, mudconf.player_proto );

    raw_notify( player, "Attr Defaults:  Room...#%d  Exit...#%d  Thing...#%d  Player...#%d", mudconf.room_defobj, mudconf.exit_defobj, mudconf.thing_defobj, mudconf.player_defobj );

    raw_notify( player, "Default Parents:  Room...#%d  Exit...#%d  Thing...#%d  Player...#%d", mudconf.room_parent, mudconf.exit_parent, mudconf.thing_parent, mudconf.player_parent );

    raw_notify( player, NULL, "Limits:" );
    raw_notify( player, "  Function recursion...%d  Function invocation...%d", mudconf.func_nest_lim, mudconf.func_invk_lim );
    raw_notify( player, "  Command recursion...%d  Command invocation...%d", mudconf.cmd_nest_lim, mudconf.cmd_invk_lim );
    raw_notify( player, "  Output...%d  Queue...%d  CPU...%d  Wild...%d  Aliases...%d", mudconf.output_limit, mudconf.queuemax, mudconf.func_cpu_lim_secs, mudconf.wild_times_lim, mudconf.max_player_aliases );
    raw_notify( player, "  Forwardlist...%d  Propdirs... %d  Registers...%d  Stacks...%d", mudconf.fwdlist_lim, mudconf.propdir_lim, mudconf.register_limit, mudconf.stack_lim );
    raw_notify( player, "  Variables...%d  Structures...%d  Instances...%d", mudconf.numvars_lim, mudconf.struct_lim, mudconf.instance_lim );
    raw_notify( player, "  Objects...%d  Allowance...%d  Trace levels...%d  Connect tries...%d", mudconf.building_limit, mudconf.paylimit, mudconf.trace_limit, mudconf.retry_limit );
    if( mudconf.max_players >= 0 )
        raw_notify( player, "  Logins...%d", mudconf.max_players );

    raw_notify( player, "Nesting:  Locks...%d  Parents...%d  Messages...%d  Zones...%d", mudconf.lock_nest_lim, mudconf.parent_nest_lim, mudconf.ntfy_nest_lim, mudconf.zone_nest_lim );

    raw_notify( player, "Timeouts:  Idle...%d  Connect...%d  Tries...%d  Lag...%d", mudconf.idle_timeout, mudconf.conn_timeout, mudconf.retry_limit, mudconf.max_cmdsecs );

    raw_notify( player, "Money:  Start...%d  Daily...%d  Singular: %s  Plural: %s", mudconf.paystart, mudconf.paycheck, mudconf.one_coin, mudconf.many_coins );
    if( mudconf.payfind > 0 )
        raw_notify( player, "Chance of finding money: 1 in %d", mudconf.payfind );

    raw_notify( player, "Start Quotas:  Total...%d  Rooms...%d  Exits...%d  Things...%d  Players...%d", mudconf.start_quota, mudconf.start_room_quota, mudconf.start_exit_quota, mudconf.start_thing_quota, mudconf.start_player_quota );

    raw_notify( player, NULL, "Dbrefs:" );
    raw_notify( player, "  MasterRoom...#%d  StartRoom...#%d  StartHome...#%d  DefaultHome...#%d", mudconf.master_room, mudconf.start_room, mudconf.start_home, mudconf.default_home );

    if( Wizard( player ) ) {

        raw_notify( player, "  GuestChar...#%d  GuestStart...#%d  Freelist...#%d", mudconf.guest_char, mudconf.guest_start_room, mudstate.freelist );

        raw_notify( player, "Queue run sizes:  No net activity... %d  Activity... %d", mudconf.queue_chunk, mudconf.active_q_chunk );

        raw_notify( player, "Intervals:  Dump...%d  Clean...%d  Idlecheck...%d  Optimize...%d",  mudconf.dump_interval, mudconf.check_interval, mudconf.idle_interval, mudconf.dbopt_interval );

        raw_notify( player, "Timers:  Dump...%d  Clean...%d  Idlecheck...%d", ( int )( mudstate.dump_counter - now ), ( int )( mudstate.check_counter - now ), ( int )( mudstate.idle_counter - now ) );

        raw_notify( player, "Scheduling:  Timeslice...%d  Max_Quota...%d  Increment...%d", mudconf.timeslice, mudconf.cmd_quota_max, mudconf.cmd_quota_incr );

        raw_notify( player, "Size of %s cache:  Width...%d  Size...%d", CACHING, mudconf.cache_width, mudconf.cache_size );
    }
}

/*
 * ---------------------------------------------------------------------------
 * list_vattrs: List user-defined attributes
 */

static void list_vattrs( dbref player ) {
    VATTR *va;

    int na;

    char *buff;

    buff = alloc_lbuf( "list_vattrs" );
    raw_notify( player, NULL, "--- User-Defined Attributes ---");
    for( va = vattr_first(), na = 0; va; va = vattr_next( va ), na++ ) {
        if( !( va->flags & AF_DELETED ) ) {
            sprintf( buff, "%s(%d):", va->name, va->number );
            listset_nametab( player, attraccess_nametab, va->flags, buff, 1 );
        }
    }

    raw_notify( player, "%d attributes, next=%d", na, mudstate.attr_next );
    free_lbuf( buff );
}

/*
 * ---------------------------------------------------------------------------
 * list_hashstats: List information from hash tables
 */

static void list_hashstat( dbref player, const char *tab_name, HASHTAB *htab ) {
    char *buff;

    buff = hashinfo( tab_name, htab );
    raw_notify( player, NULL, buff);
    free_mbuf( buff );
}

static void list_nhashstat( dbref player, const char *tab_name, NHSHTAB *htab ) {
    char *buff;

    buff = nhashinfo( tab_name, htab );
    raw_notify( player, NULL, buff);
    free_mbuf( buff );
}

static void list_hashstats( dbref player ) {
    MODULE *mp;

    MODHASHES *m_htab, *hp;

    MODNHASHES *m_ntab, *np;

    raw_notify( player, NULL, "Hash Stats       Size Entries Deleted   Empty Lookups    Hits  Checks Longest");
    list_hashstat( player, "Commands", &mudstate.command_htab );
    list_hashstat( player, "Logged-out Cmds", &mudstate.logout_cmd_htab );
    list_hashstat( player, "Functions", &mudstate.func_htab );
    list_hashstat( player, "User Functions", &mudstate.ufunc_htab );
    list_hashstat( player, "Flags", &mudstate.flags_htab );
    list_hashstat( player, "Powers", &mudstate.powers_htab );
    list_hashstat( player, "Attr names", &mudstate.attr_name_htab );
    list_hashstat( player, "Vattr names", &mudstate.vattr_name_htab );
    list_hashstat( player, "Player Names", &mudstate.player_htab );
    list_hashstat( player, "References", &mudstate.nref_htab );
    list_nhashstat( player, "Net Descriptors", &mudstate.desc_htab );
    list_nhashstat( player, "Queue Entries", &mudstate.qpid_htab );
    list_nhashstat( player, "Forwardlists", &mudstate.fwdlist_htab );
    list_nhashstat( player, "Propdirs", &mudstate.propdir_htab );
    list_nhashstat( player, "Redirections", &mudstate.redir_htab );
    list_nhashstat( player, "Overlaid $-cmds", &mudstate.parent_htab );
    list_nhashstat( player, "Object Stacks", &mudstate.objstack_htab );
    list_nhashstat( player, "Object Grids", &mudstate.objgrid_htab );
    list_hashstat( player, "Variables", &mudstate.vars_htab );
    list_hashstat( player, "Structure Defs", &mudstate.structs_htab );
    list_hashstat( player, "Component Defs", &mudstate.cdefs_htab );
    list_hashstat( player, "Instances", &mudstate.instance_htab );
    list_hashstat( player, "Instance Data", &mudstate.instdata_htab );
    list_hashstat( player, "Module APIs", &mudstate.api_func_htab );

    WALK_ALL_MODULES( mp ) {
        m_htab = DLSYM_VAR( mp->handle, mp->modname,
                            "hashtable", MODHASHES * );
        if( m_htab ) {
            for( hp = m_htab; hp->htab != NULL; hp++ ) {
                list_hashstat( player, hp->tabname, hp->htab );
            }
        }
        m_ntab = DLSYM_VAR( mp->handle, mp->modname,
                            "nhashtable", MODNHASHES * );
        if( m_ntab ) {
            for( np = m_ntab; np->tabname != NULL; np++ ) {
                list_nhashstat( player, np->tabname, np->htab );
            }
        }
    }
}

static void list_textfiles( dbref player ) {
    int i;

    raw_notify( player, NULL, "Help File        Size Entries Deleted   Empty Lookups    Hits  Checks Longest");

    for( i = 0; i < mudstate.helpfiles; i++ )
        list_hashstat( player, mudstate.hfiletab[i],
                       &mudstate.hfile_hashes[i] );
}

/* These are from 'udb_cache.c'. */
extern time_t cs_ltime;

extern int cs_writes;		/* total writes */

extern int cs_reads;		/* total reads */

extern int cs_dbreads;		/* total read-throughs */

extern int cs_dbwrites;		/* total write-throughs */

extern int cs_dels;		/* total deletes */

extern int cs_checks;		/* total checks */

extern int cs_rhits;		/* total reads filled from cache */

extern int cs_ahits;		/* total reads filled active cache */

extern int cs_whits;		/* total writes to dirty cache */

extern int cs_fails;		/* attempts to grab nonexistent */

extern int cs_resets;		/* total cache resets */

extern int cs_syncs;		/* total cache syncs */

extern int cs_size;		/* total cache size */

/*
 * ---------------------------------------------------------------------------
 * list_db_stats: Get useful info from the DB layer about hash stats, etc.
 */

static void list_db_stats( dbref player ) {
    raw_notify( player, "DB Cache Stats   Writes       Reads  (over %d seconds)", ( int )( time( NULL ) - cs_ltime ) );
    raw_notify( player, "Calls      %12d%12d", cs_writes, cs_reads );
    raw_notify( player, "Cache Hits %12d%12d", cs_whits, cs_rhits );
    raw_notify( player, "I/O        %12d%12d", cs_dbwrites, cs_dbreads );
    raw_notify( player, "Failed                 %12d", cs_fails );
    raw_notify( player, "Hit ratio            %2.0f%%         %2.0f%%", ( cs_writes ? ( float ) cs_whits / cs_writes * 100 : 0.0 ), ( cs_reads ? ( float ) cs_rhits / cs_reads * 100 : 0.0 ) );
    raw_notify( player, "\nDeletes    %12d", cs_dels );
    raw_notify( player, "Checks     %12d", cs_checks );
    raw_notify( player, "Syncs      %12d", cs_syncs );
    raw_notify( player, "Cache Size %12d bytes", cs_size );
}

/*
 * ---------------------------------------------------------------------------
 * list_process: List local resource usage stats of the MUSH process. Adapted
 * from code by Claudius@PythonMUCK, posted to the net by Howard/Dark_Lord.
 */

static void list_process( dbref player ) {
    int pid, psize, maxfds;

#if defined(HAVE_GETRUSAGE) && defined(STRUCT_RUSAGE_COMPLETE)
    struct rusage usage;

    int ixrss, idrss, isrss, curr, last, dur;

    getrusage( RUSAGE_SELF, &usage );
    /*
     * Calculate memory use from the aggregate totals
     */

    curr = mudstate.mstat_curr;
    last = 1 - curr;
    dur = mudstate.mstat_secs[curr] - mudstate.mstat_secs[last];
    if( dur > 0 ) {
        ixrss = ( mudstate.mstat_ixrss[curr] -
                  mudstate.mstat_ixrss[last] ) / dur;
        idrss = ( mudstate.mstat_idrss[curr] -
                  mudstate.mstat_idrss[last] ) / dur;
        isrss = ( mudstate.mstat_isrss[curr] -
                  mudstate.mstat_isrss[last] ) / dur;
    } else {
        ixrss = 0;
        idrss = 0;
        isrss = 0;
    }
#endif

#ifdef HAVE_GETDTABLESIZE
    maxfds = getdtablesize();
#else
    maxfds = sysconf( _SC_OPEN_MAX );
#endif


    pid = getpid();
    psize = getpagesize();

    /*
     * Go display everything
     */

    raw_notify( player, "Process ID:  %10d        %10d bytes per page", pid, psize );
#if defined(HAVE_GETRUSAGE) && defined(STRUCT_RUSAGE_COMPLETE)
    raw_notify( player, "Time used:   %10d user   %10d sys", usage.ru_utime.tv_sec, usage.ru_stime.tv_sec );
    /*
     * raw_notify(player, "Resident mem:%10d shared %10d private%10d stack", *ixrss, idrss, isrss);
     */
    raw_notify( player, "Integral mem:%10d shared %10d private%10d stack", usage.ru_ixrss, usage.ru_idrss, usage.ru_isrss );
    raw_notify( player, "Max res mem: %10d pages  %10d bytes", usage.ru_maxrss, ( usage.ru_maxrss * psize ) );
    raw_notify( player, "Page faults: %10d hard   %10d soft   %10d swapouts", usage.ru_majflt, usage.ru_minflt, usage.ru_nswap );
    raw_notify( player, "Disk I/O:    %10d reads  %10d writes", usage.ru_inblock, usage.ru_oublock );
    raw_notify( player, "Network I/O: %10d in     %10d out", usage.ru_msgrcv, usage.ru_msgsnd );
    raw_notify( player, "Context swi: %10d vol    %10d forced %10d sigs", usage.ru_nvcsw, usage.ru_nivcsw, usage.ru_nsignals );
    raw_notify( player, "Descs avail: %10d", maxfds );
#endif
}

/*
 * ---------------------------------------------------------------------------
 * list_memory: Breaks down memory usage of the process
 */

extern Chain *sys_c;

extern NAME *names, *purenames;

extern POOL pools[NUM_POOLS];

extern int anum_alc_top;

void list_memory( dbref player ) {
    double total = 0, each = 0, each2 = 0;

    int i, j;

    CMDENT *cmd;

    ADDENT *add;

    NAMETAB *name;

    VATTR *vattr;

    ATTR *attr;

    FUN *func;

    UFUN *ufunc;

    Cache *cp;

    Chain *sp;

    HASHENT *htab;

    struct help_entry *hlp;

    FLAGENT *flag;

    POWERENT *power;

    OBJSTACK *stack;

    OBJGRID *grid;

    VARENT *xvar;

    STRUCTDEF *this_struct;

    INSTANCE *inst_ptr;

    STRUCTDATA *data_ptr;

    /*
     * Calculate size of object structures
     */

    each = mudstate.db_top * sizeof( OBJ );
    raw_notify( player, "Object structures: %12.2fk", each / 1024 );
    total += each;

#ifdef MEMORY_BASED
    each = 0;
    /*
     * Calculate size of stored attribute text
     */
    DO_WHOLE_DB( i ) {
        each += obj_siz( & ( db[i].attrtext ) );
        each -= sizeof( Obj );
    }

    raw_notify( player, "Stored attrtext  : %12.2fk", each / 1024 );
    total += each;
#endif

    /*
     * Calculate size of mudstate and mudconf structures
     */

    each = sizeof( CONFDATA ) + sizeof( STATEDATA );
    raw_notify( player, "mudconf/mudstate : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of cache
     */

    each = cs_size;
    raw_notify( player, "Cache data       : %12.2fk", each / 1024 );
    total += each;

    each = sizeof( Chain ) * mudconf.cache_width;
    for( i = 0; i < mudconf.cache_width; i++ ) {
        sp = &sys_c[i];
        for( cp = sp->head; cp != NULL; cp = cp->nxt ) {
            each += sizeof( Cache );
            each2 += cp->keylen;
        }
    }
    raw_notify( player, "Cache keys       : %12.2fk", each2 / 1024 );
    raw_notify( player, "Cache overhead   : %12.2fk", each / 1024 );
    total += each + each2;

    /*
     * Calculate size of object pipelines
     */

    each = 0;
    for( i = 0; i < NUM_OBJPIPES; i++ ) {
        if( mudstate.objpipes[i] ) {
            each += obj_siz( mudstate.objpipes[i] );
        }
    }

    raw_notify( player, "Object pipelines : %12.2fk", each / 1024);
    total += each;

    /*
     * Calculate size of name caches
     */

    each = sizeof( NAME * ) * mudstate.db_top * 2;
    for( i = 0; i < mudstate.db_top; i++ ) {
        if( purenames[i] ) {
            each += strlen( purenames[i] ) + 1;
        }
        if( names[i] ) {
            each += strlen( names[i] ) + 1;
        }
    }
    raw_notify( player, "Name caches      : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of buffers
     */

    each = sizeof( POOL ) * NUM_POOLS;
    for( i = 0; i < NUM_POOLS; i++ ) {
        each += pools[i].max_alloc * ( pools[i].pool_size +
                                       sizeof( POOLHDR ) + sizeof( POOLFTR ) );
    }
    raw_notify( player, "Buffers          : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of command hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.command_htab.hashsize;
    for( i = 0; i < mudstate.command_htab.hashsize; i++ ) {
        htab = mudstate.command_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each +=
                strlen( mudstate.command_htab.entry[i]->target.s ) +
                1;

            /*
             * Add up all the little bits in the CMDENT.
             */

            if( !( htab->flags & HASH_ALIAS ) ) {
                each += sizeof( CMDENT );
                cmd = ( CMDENT * ) htab->data;
                each += strlen( cmd->cmdname ) + 1;
                if( ( name = cmd->switches ) != NULL ) {
                    for( j = 0; name[j].name != NULL; j++ ) {
                        each += sizeof( NAMETAB );
                        each +=
                            strlen( name[j].name ) + 1;
                    }
                }
                if( cmd->callseq & CS_ADDED ) {
                    add = cmd->info.added;
                    while( add != NULL ) {
                        each += sizeof( ADDENT );
                        each += strlen( add->name ) + 1;
                        add = add->next;
                    }
                }
            }
            htab = htab->next;
        }
    }
    raw_notify( player, "Command table    : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of logged-out commands hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.logout_cmd_htab.hashsize;
    for( i = 0; i < mudstate.logout_cmd_htab.hashsize; i++ ) {
        htab = mudstate.logout_cmd_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            if( !( htab->flags & HASH_ALIAS ) ) {
                name = ( NAMETAB * ) htab->data;
                each += sizeof( NAMETAB );
                each += strlen( name->name ) + 1;
            }
            htab = htab->next;
        }
    }
    raw_notify( player, "Logout cmd htab  : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of functions hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.func_htab.hashsize;
    for( i = 0; i < mudstate.func_htab.hashsize; i++ ) {
        htab = mudstate.func_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            if( !( htab->flags & HASH_ALIAS ) ) {
                func = ( FUN * ) htab->data;
                each += sizeof( FUN );
            }
            /*
             * We don't count func->name because we already got
             * it with htab->target.s
             */
            htab = htab->next;
        }
    }
    raw_notify( player, "Functions htab   : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of user-defined functions hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.ufunc_htab.hashsize;
    for( i = 0; i < mudstate.ufunc_htab.hashsize; i++ ) {
        htab = mudstate.ufunc_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            if( !( htab->flags & HASH_ALIAS ) ) {
                ufunc = ( UFUN * ) htab->data;
                while( ufunc != NULL ) {
                    each += sizeof( UFUN );
                    each += strlen( ufunc->name ) + 1;
                    ufunc = ufunc->next;
                }
            }
            htab = htab->next;
        }
    }
    raw_notify( player, "U-functions htab : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of flags hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.flags_htab.hashsize;
    for( i = 0; i < mudstate.flags_htab.hashsize; i++ ) {
        htab = mudstate.flags_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            if( !( htab->flags & HASH_ALIAS ) ) {
                flag = ( FLAGENT * ) htab->data;
                each += sizeof( FLAGENT );
            }
            /*
             * We don't count flag->flagname because we already
             * got it with htab->target.s
             */
            htab = htab->next;
        }
    }
    raw_notify( player, "Flags htab       : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of powers hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.powers_htab.hashsize;
    for( i = 0; i < mudstate.powers_htab.hashsize; i++ ) {
        htab = mudstate.powers_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            if( !( htab->flags & HASH_ALIAS ) ) {
                power = ( POWERENT * ) htab->data;
                each += sizeof( POWERENT );
            }
            /*
             * We don't count power->powername because we already
             * got it with htab->target.s
             */
            htab = htab->next;
        }
    }
    raw_notify( player, "Powers htab      : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of helpfile hashtables
     */

    each = 0;

    for( j = 0; j < mudstate.helpfiles; j++ ) {
        each += sizeof( HASHENT * ) * mudstate.hfile_hashes[j].hashsize;
        for( i = 0; i < mudstate.hfile_hashes[j].hashsize; i++ ) {
            htab = mudstate.hfile_hashes[j].entry[i];
            while( htab != NULL ) {
                each += sizeof( HASHENT );
                each += strlen( htab->target.s ) + 1;
                if( !( htab->flags & HASH_ALIAS ) ) {
                    each += sizeof( struct help_entry );
                    hlp = ( struct help_entry * ) htab->data;
                }
                htab = htab->next;
            }
        }
    }
    raw_notify( player, "Helpfiles htabs  : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of vattr name hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.vattr_name_htab.hashsize;
    for( i = 0; i < mudstate.vattr_name_htab.hashsize; i++ ) {
        htab = mudstate.vattr_name_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            each += sizeof( VATTR );
            htab = htab->next;
        }
    }
    raw_notify( player, "Vattr name htab  : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate size of attr name hashtable
     */

    each = 0;
    each += sizeof( HASHENT * ) * mudstate.attr_name_htab.hashsize;
    for( i = 0; i < mudstate.attr_name_htab.hashsize; i++ ) {
        htab = mudstate.attr_name_htab.entry[i];
        while( htab != NULL ) {
            each += sizeof( HASHENT );
            each += strlen( htab->target.s ) + 1;
            if( !( htab->flags & HASH_ALIAS ) ) {
                attr = ( ATTR * ) htab->data;
                each += sizeof( ATTR );
                each += strlen( attr->name ) + 1;
            }
            htab = htab->next;
        }
    }
    raw_notify( player, "Attr name htab   : %12.2fk", each / 1024 );
    total += each;

    /*
     * Calculate the size of anum_table
     */

    each = sizeof( ATTR * ) * anum_alc_top;
    raw_notify( player, "Attr num table   : %12.2fk", each / 1024 );
    total += each;

    /*
     * --- After this point, we only report if it's non-zero.
     */

    /*
     * Calculate size of object stacks
     */

    each = 0;
    for( stack = ( OBJSTACK * ) hash_firstentry( ( HASHTAB * ) & mudstate.objstack_htab ); stack != NULL; stack = ( OBJSTACK * ) hash_nextentry( ( HASHTAB * ) & mudstate.objstack_htab ) ) {
        each += sizeof( OBJSTACK );
        each += strlen( stack->data ) + 1;
    }
    if( each ) {
        raw_notify( player, "Object stacks    : %12.2fk", each / 1024 );
    }
    total += each;

    /*
     * Calculate the size of grids
     */

    each = 0;
    for( grid = ( OBJGRID * ) hash_firstentry( ( HASHTAB * ) & mudstate.objgrid_htab ); grid != NULL; grid = ( OBJGRID * ) hash_nextentry( ( HASHTAB * ) & mudstate.objgrid_htab ) ) {
        each += sizeof( OBJGRID );
        each += sizeof( char ** ) * grid->rows * grid->cols;
        for( i = 0; i < grid->rows; i++ ) {
            for( j = 0; j < grid->cols; j++ ) {
                if( grid->data[i][j] != NULL ) {
                    each += strlen( grid->data[i][j] ) + 1;
                }
            }
        }
    }
    if( each ) {
        raw_notify( player, "Object grids     : %12.2fk", each / 1024 );
    }
    total += each;

    /*
     * Calculate the size of xvars.
     */

    each = 0;
    for( xvar = ( VARENT * ) hash_firstentry( &mudstate.vars_htab ); xvar != NULL; xvar = ( VARENT * ) hash_nextentry( &mudstate.vars_htab ) ) {
        each += sizeof( VARENT );
        each += strlen( xvar->text ) + 1;
    }
    if( each ) {
        raw_notify( player, "X-Variables      : %12.2fk", each / 1024 );
    }
    total += each;

    /*
     * Calculate the size of overhead associated with structures.
     */

    each = 0;
    for( this_struct = ( STRUCTDEF * ) hash_firstentry( &mudstate.structs_htab ); this_struct != NULL; this_struct = ( STRUCTDEF * ) hash_nextentry( &mudstate.structs_htab ) ) {
        each += sizeof( STRUCTDEF );
        each += strlen( this_struct->s_name ) + 1;
        for( i = 0; i < this_struct->c_count; i++ ) {
            each += strlen( this_struct->c_names[i] ) + 1;
            each += sizeof( COMPONENT );
            each += strlen( this_struct->c_array[i]->def_val ) + 1;
        }
    }
    for( inst_ptr = ( INSTANCE * ) hash_firstentry( &mudstate.instance_htab ); inst_ptr != NULL; inst_ptr = ( INSTANCE * ) hash_nextentry( &mudstate.instance_htab ) ) {
        each += sizeof( INSTANCE );
    }
    if( each ) {
        raw_notify( player, "Struct var defs  : %12.2fk", each / 1024 );
    }
    total += each;

    /*
     * Calculate the size of data associated with structures.
     */

    each = 0;
    for( data_ptr =
                ( STRUCTDATA * ) hash_firstentry( &mudstate.instdata_htab );
            data_ptr != NULL;
            data_ptr =
                ( STRUCTDATA * ) hash_nextentry( &mudstate.instdata_htab ) ) {
        each += sizeof( STRUCTDATA );
        if( data_ptr->text ) {
            each += strlen( data_ptr->text ) + 1;
        }
    }
    if( each ) {
        raw_notify( player, "Struct var data  : %12.2fk", each / 1024 );
    }
    total += each;

    /*
     * Report end total.
     */

    raw_notify( player, "\r\nTotal            : %12.2fk", total / 1024 );
}

/*
 * ---------------------------------------------------------------------------
 * do_list: List information stored in internal structures.
 */

#define	LIST_ATTRIBUTES	1
#define	LIST_COMMANDS	2
#define	LIST_COSTS	3
#define	LIST_FLAGS	4
#define	LIST_FUNCTIONS	5
#define	LIST_GLOBALS	6
#define	LIST_ALLOCATOR	7
#define	LIST_LOGGING	8
#define	LIST_DF_FLAGS	9
#define	LIST_PERMS	10
#define	LIST_ATTRPERMS	11
#define	LIST_OPTIONS	12
#define	LIST_HASHSTATS	13
#define	LIST_BUFTRACE	14
#define	LIST_CONF_PERMS	15
#define	LIST_SITEINFO	16
#define	LIST_POWERS	17
#define	LIST_SWITCHES	18
#define	LIST_VATTRS	19
#define	LIST_DB_STATS	20	/* GAC 4/6/92 */
#define	LIST_PROCESS	21
#define	LIST_BADNAMES	22
#define LIST_CACHEOBJS	23
#define LIST_TEXTFILES  24
#define LIST_PARAMS	25
#define LIST_CF_RPERMS	26
#define LIST_ATTRTYPES	27
#define LIST_FUNCPERMS	28
#define LIST_MEMORY	29
#define LIST_CACHEATTRS 30
#define LIST_RAWMEM	31
/* *INDENT-OFF* */

NAMETAB		list_names[] = {
    { ( char * ) "allocations", 2, CA_WIZARD, LIST_ALLOCATOR},
    { ( char * ) "attr_permissions", 6, CA_WIZARD, LIST_ATTRPERMS},
    { ( char * ) "attr_types", 6, CA_PUBLIC, LIST_ATTRTYPES},
    { ( char * ) "attributes", 2, CA_PUBLIC, LIST_ATTRIBUTES},
    { ( char * ) "bad_names", 2, CA_WIZARD, LIST_BADNAMES},
    { ( char * ) "buffers", 2, CA_WIZARD, LIST_BUFTRACE},
    { ( char * ) "cache", 2, CA_WIZARD, LIST_CACHEOBJS},
    { ( char * ) "cache_attrs", 6, CA_WIZARD, LIST_CACHEATTRS},
    { ( char * ) "commands", 3, CA_PUBLIC, LIST_COMMANDS},
    { ( char * ) "config_permissions", 8, CA_GOD, LIST_CONF_PERMS},
    { ( char * ) "config_read_perms", 4, CA_PUBLIC, LIST_CF_RPERMS},
    { ( char * ) "costs", 3, CA_PUBLIC, LIST_COSTS},
    { ( char * ) "db_stats", 2, CA_WIZARD, LIST_DB_STATS},
    { ( char * ) "default_flags", 1, CA_PUBLIC, LIST_DF_FLAGS},
    { ( char * ) "flags", 2, CA_PUBLIC, LIST_FLAGS},
    { ( char * ) "func_permissions", 5, CA_WIZARD, LIST_FUNCPERMS},
    { ( char * ) "functions", 2, CA_PUBLIC, LIST_FUNCTIONS},
    { ( char * ) "globals", 1, CA_WIZARD, LIST_GLOBALS},
    { ( char * ) "hashstats", 1, CA_WIZARD, LIST_HASHSTATS},
    { ( char * ) "logging", 1, CA_GOD, LIST_LOGGING},
    { ( char * ) "memory", 1, CA_WIZARD, LIST_MEMORY},
    { ( char * ) "options", 1, CA_PUBLIC, LIST_OPTIONS},
    { ( char * ) "params", 2, CA_PUBLIC, LIST_PARAMS},
    { ( char * ) "permissions", 2, CA_WIZARD, LIST_PERMS},
    { ( char * ) "powers", 2, CA_WIZARD, LIST_POWERS},
    { ( char * ) "process", 2, CA_WIZARD, LIST_PROCESS},
    { ( char * ) "raw_memory", 1, CA_WIZARD, LIST_RAWMEM},
    { ( char * ) "site_information", 2, CA_WIZARD, LIST_SITEINFO},
    { ( char * ) "switches", 2, CA_PUBLIC, LIST_SWITCHES},
    { ( char * ) "textfiles", 1, CA_WIZARD, LIST_TEXTFILES},
    { ( char * ) "user_attributes", 1, CA_WIZARD, LIST_VATTRS},
    {NULL, 0, 0, 0}
};

/* *INDENT-ON* */

extern NAMETAB enable_names[];

extern NAMETAB logoptions_nametab[];

extern NAMETAB logdata_nametab[];

void do_list( dbref player, dbref cause, int extra, char *arg ) {
    int flagvalue;

    flagvalue = search_nametab( player, list_names, arg );
    switch( flagvalue ) {
    case LIST_ALLOCATOR:
        list_bufstats( player );
        break;
    case LIST_BUFTRACE:
        list_buftrace( player );
        break;
    case LIST_ATTRIBUTES:
        list_attrtable( player );
        break;
    case LIST_COMMANDS:
        list_cmdtable( player );
        break;
    case LIST_SWITCHES:
        list_cmdswitches( player );
        break;
    case LIST_COSTS:
        list_costs( player );
        break;
    case LIST_OPTIONS:
        list_options( player );
        break;
    case LIST_HASHSTATS:
        list_hashstats( player );
        break;
    case LIST_SITEINFO:
        list_siteinfo( player );
        break;
    case LIST_FLAGS:
        display_flagtab( player );
        break;
    case LIST_FUNCPERMS:
        list_funcaccess( player );
        break;
    case LIST_FUNCTIONS:
        list_functable( player );
        break;
    case LIST_GLOBALS:
        interp_nametab( player, enable_names, mudconf.control_flags,
                        ( char * ) "Global parameters:", ( char * ) "enabled",
                        ( char * ) "disabled" );
        break;
    case LIST_DF_FLAGS:
        list_df_flags( player );
        break;
    case LIST_PERMS:
        list_cmdaccess( player );
        break;
    case LIST_CONF_PERMS:
        list_cf_access( player );
        break;
    case LIST_CF_RPERMS:
        list_cf_read_access( player );
        break;
    case LIST_POWERS:
        display_powertab( player );
        break;
    case LIST_ATTRPERMS:
        list_attraccess( player );
        break;
    case LIST_VATTRS:
        list_vattrs( player );
        break;
    case LIST_LOGGING:
        interp_nametab( player, logoptions_nametab, mudconf.log_options,
                        ( char * ) "Events Logged:", ( char * ) "enabled",
                        ( char * ) "disabled" );
        interp_nametab( player, logdata_nametab, mudconf.log_info,
                        ( char * ) "Information Logged:", ( char * ) "yes",
                        ( char * ) "no" );
        break;
    case LIST_DB_STATS:
        list_db_stats( player );
        break;
    case LIST_PROCESS:
        list_process( player );
        break;
    case LIST_BADNAMES:
        badname_list( player, "Disallowed names:" );
        break;
    case LIST_CACHEOBJS:
        list_cached_objs( player );
        break;
    case LIST_TEXTFILES:
        list_textfiles( player );
        break;
    case LIST_PARAMS:
        list_params( player );
        break;
    case LIST_ATTRTYPES:
        list_attrtypes( player );
        break;
    case LIST_MEMORY:
        list_memory( player );
        break;
    case LIST_CACHEATTRS:
        list_cached_attrs( player );
        break;
    case LIST_RAWMEM:
        list_rawmemory( player );
        break;
    default:
        display_nametab( player, list_names,
                         ( char * ) "Unknown option.  Use one of:", 1 );
    }
}
