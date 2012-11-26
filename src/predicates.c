/* predicates.c */

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
#include "command.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "ansi.h"		/* required by code */
#include "functions.h"		/* required by code */

extern int do_command( DESC *, char *, int );

extern void dump_database( void );

extern LOGFILETAB logfds_table[];

extern volatile int slave_pid;

extern volatile int slave_socket;

extern void load_quota( int *, dbref, int );

extern void save_quota( int *, dbref, int );

extern int get_gender( dbref );

static int type_quota( int );

static int pay_quota( dbref, int, int );

static char tmprintf_buff[LBUF_SIZE];

char *safe_snprintf(char *buff, size_t size, const char *format, ... ) {
    va_list ap;

    va_start( ap, format );
    vsnprintf( buff, size, format, ap );
    va_end( ap );

    buff[size - 1] = '\0';

    return(buff);
}

char *safe_vsnprintf(char *buff, size_t size, const char *format, va_list ap ) {
 
    vsnprintf( buff, size, format, ap );
 
    buff[size - 1] = '\0';
 
    return(buff);
}

char *tmprintf( const char *format, ... ) {
    va_list ap;

    va_start( ap, format );

    vsnprintf( tmprintf_buff, LBUF_SIZE, format, ap );

    va_end( ap );
    tmprintf_buff[LBUF_SIZE - 1] = '\0';
    return tmprintf_buff;
}

void safe_sprintf( char *str, char **bp, const char *format, ... ) {
    int len, n;
    va_list ap;

    va_start( ap, format );

    /*
     * Sigh, don't we wish _all_ vsprintf's returned int...
     */

    n = LBUF_SIZE - ( *bp - str );
    if( n <= 0 ) {
        **bp = '\0';
        return;
    }
    
    vsnprintf( *bp, n, format, ap );
    va_end( ap );
    len = strlen( *bp );
    n = ( ( len < n ) ? len : n );
    *bp += n;
    **bp = '\0';
}

/* ---------------------------------------------------------------------------
 * insert_first, remove_first: Insert or remove objects from lists.
 */

dbref insert_first( dbref head, dbref thing ) {
    s_Next( thing, head );
    return thing;
}

dbref remove_first( dbref head, dbref thing ) {
    dbref prev;

    if( head == thing ) {
        return ( Next( thing ) );
    }

    DOLIST( prev, head ) {
        if( Next( prev ) == thing ) {
            s_Next( prev, Next( thing ) );
            return head;
        }
    }
    return head;
}

/* ---------------------------------------------------------------------------
 * reverse_list: Reverse the order of members in a list.
 */

dbref reverse_list( dbref list ) {
    dbref newlist, rest;

    newlist = NOTHING;
    while( list != NOTHING ) {
        rest = Next( list );
        s_Next( list, newlist );
        newlist = list;
        list = rest;
    }
    return newlist;
}

/* ---------------------------------------------------------------------------
 * member - indicate if thing is in list
 */

int member( dbref thing, dbref list ) {
    DOLIST( list, list ) {
        if( list == thing ) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * is_integer, is_number: see if string contains just a number.
 */

int is_integer( char *str ) {
    while( *str && isspace( *str ) ) {
        str++;    /* Leading spaces */
    }
    if( ( *str == '-' ) || ( *str == '+' ) ) {	/* Leading minus or plus */
        str++;
        if( !*str ) {
            return 0;    /* but not if just a minus or plus */
        }
    }
    if( !isdigit( *str ) ) {	/* Need at least 1 integer */
        return 0;
    }
    while( *str && isdigit( *str ) ) {
        str++;    /* The number (int) */
    }
    while( *str && isspace( *str ) ) {
        str++;    /* Trailing spaces */
    }
    return ( *str ? 0 : 1 );
}

int is_number( char *str ) {
    int got_one;

    while( *str && isspace( *str ) ) {
        str++;    /* Leading spaces */
    }
    if( ( *str == '-' ) || ( *str == '+' ) ) {	/* Leading minus or plus */
        str++;
        if( !*str ) {
            return 0;    /* but not if just a minus or plus */
        }
    }
    got_one = 0;
    if( isdigit( *str ) ) {
        got_one = 1;    /* Need at least one digit */
    }
    while( *str && isdigit( *str ) ) {
        str++;    /* The number (int) */
    }
    if( *str == '.' ) {
        str++;    /* decimal point */
    }
    if( isdigit( *str ) ) {
        got_one = 1;    /* Need at least one digit */
    }
    while( *str && isdigit( *str ) ) {
        str++;    /* The number (fract) */
    }
    while( *str && isspace( *str ) ) {
        str++;    /* Trailing spaces */
    }
    return ( ( *str || !got_one ) ? 0 : 1 );
}

int could_doit( dbref player, dbref thing, int locknum ) {
    char *key;

    dbref aowner;

    int aflags, alen, doit;

    /*
     * no if nonplayer tries to get key
     */

    if( !isPlayer( player ) && Key( thing ) ) {
        return 0;
    }
    if( Pass_Locks( player ) ) {
        return 1;
    }

    key = atr_get( thing, locknum, &aowner, &aflags, &alen );
    doit = eval_boolexp_atr( player, thing, thing, key );
    free_lbuf( key );
    return doit;
}

static int canpayquota( dbref player, dbref who, int cost, int objtype ) {
    register int quota;

    int q_list[5];

    /*
     * If no cost, succeed
     */

    if( cost <= 0 ) {
        return 1;
    }

    /*
     * determine basic quota
     */

    load_quota( q_list, Owner( who ), A_RQUOTA );
    quota = q_list[QTYPE_ALL];

    /*
     * enough to build?  Wizards always have enough.
     */

    quota -= cost;
    if( ( quota < 0 ) && !Free_Quota( who ) && !Free_Quota( Owner( who ) ) ) {
        return 0;
    }

    if( mudconf.typed_quotas ) {
        quota = q_list[type_quota( objtype )];
        if( ( quota <= 0 ) &&
                !Free_Quota( player ) && !Free_Quota( Owner( player ) ) ) {
            return 0;
        }
    }

    return 1;
}


static int pay_quota( dbref who, int cost, int objtype ) {
    /*
     * If no cost, succeed.  Negative costs /must/ be managed, however
     */

    if( cost == 0 ) {
        return 1;
    }

    add_quota( who, -cost, type_quota( objtype ) );

    return 1;
}

int canpayfees( dbref player, dbref who, int pennies, int quota, int objtype ) {
    if( !Wizard( who ) && !Wizard( Owner( who ) ) &&
            !Free_Money( who ) && !Free_Money( Owner( who ) ) &&
            ( Pennies( Owner( who ) ) < pennies ) ) {
        if( player == who ) {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Sorry, you don't have enough %s.", mudconf.many_coins );
        } else {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Sorry, that player doesn't have enough %s.", mudconf.many_coins );
        }
        return 0;
    }
    if( mudconf.quotas ) {
        if( !canpayquota( player, who, quota, objtype ) ) {
            if( player == who ) {
                notify( player, "Sorry, your building contract has run out." );
            } else {
                notify( player, "Sorry, that player's building contract has run out." );
            }
            return 0;
        }
    }
    return 1;
}

static int type_quota( int objtype ) {
    int qtype;

    /*
     * determine typed quota
     */

    switch( objtype ) {
    case TYPE_ROOM:
        qtype = QTYPE_ROOM;
        break;
    case TYPE_EXIT:
        qtype = QTYPE_EXIT;
        break;
    case TYPE_PLAYER:
        qtype = QTYPE_PLAYER;
        break;
    default:
        qtype = QTYPE_THING;
    }
    return ( qtype );
}

int payfor( dbref who, dbref cost ) {
    dbref tmp;

    if( Wizard( who ) || Wizard( Owner( who ) ) ||
            Free_Money( who ) || Free_Money( Owner( who ) ) ||
            Immortal( who ) || Immortal( Owner( who ) ) ) {
        return 1;
    }
    who = Owner( who );
    if( ( tmp = Pennies( who ) ) >= cost ) {
        s_Pennies( who, tmp - cost );
        return 1;
    }
    return 0;
}

int payfees( dbref who, int pennies, int quota, int objtype ) {
    /*
     * You /must/ have called canpayfees() first.  If not, your
     * * database will be eaten by rabid squirrels.
     */
    if( mudconf.quotas ) {
        pay_quota( who, quota, objtype );
    }
    return payfor( who, pennies );
}

void add_quota( dbref who, int payment, int type ) {
    int q_list[5];

    load_quota( q_list, Owner( who ), A_RQUOTA );
    q_list[QTYPE_ALL] += payment;

    if( mudconf.typed_quotas ) {
        q_list[type] += payment;
    }

    save_quota( q_list, Owner( who ), A_RQUOTA );
}

void giveto( dbref who, int pennies ) {
    if( Wizard( who ) || Wizard( Owner( who ) ) ||
            Free_Money( who ) || Free_Money( Owner( who ) ) ||
            Immortal( who ) || Immortal( Owner( who ) ) ) {
        return;
    }
    who = Owner( who );
    s_Pennies( who, Pennies( who ) + pennies );
}

int ok_name( const char *name ) {
    const char *cp;

    char *purename = strip_ansi( name );

    /*
     * Disallow pure ANSI names
     */

    if( strlen( purename ) == 0 ) {
        return 0;
    }

    /*
     * Disallow leading spaces
     */

    if( isspace( *purename ) ) {
        return 0;
    }

    /*
     * Only printable characters outside of escape codes
     */

    for( cp = purename; *cp; ++cp ) {
        if( !isprint( *cp ) ) {
            return 0;
        }
    }

    /*
     * Disallow trailing spaces
     */
    cp--;
    if( isspace( *cp ) ) {
        return 0;
    }

    /*
     * Exclude names that start with or contain certain magic cookies
     */

    return ( *purename != LOOKUP_TOKEN &&
             *purename != NUMBER_TOKEN &&
             *purename != NOT_TOKEN &&
             !strchr( name, ARG_DELIMITER ) &&
             !strchr( name, AND_TOKEN ) &&
             !strchr( name, OR_TOKEN ) &&
             string_compare( purename, "me" ) &&
             string_compare( purename, "home" ) &&
             string_compare( purename, "here" ) );
}

int ok_player_name( const char *name ) {
    const char *cp, *good_chars;

    /*
     * Good name for a thing, not too long, and we either don't have a
     * * minimum player name length, or we're sufficiently long.
     */

    if( !ok_name( name ) ||
            ( strlen( name ) >= PLAYER_NAME_LIMIT ) ||
            ( mudconf.player_name_min &&
              ( strlen( name ) < mudconf.player_name_min ) ) ) {
        return 0;
    }

    if( mudconf.name_spaces || mudstate.standalone ) {
        good_chars = " `$_-.,'";
    } else {
        good_chars = "`$_-.,'";
    }

    /*
     * Make sure name only contains legal characters
     */

    for( cp = name; cp && *cp; cp++ ) {
        if( isalnum( *cp ) ) {
            continue;
        }
        if( !strchr( good_chars, *cp ) ) {
            return 0;
        }
    }
    return 1;
}

int ok_attr_name( const char *attrname ) {
    const char *scan;

    if( !isalpha( *attrname ) && ( *attrname != '_' ) ) {
        return 0;
    }
    for( scan = attrname; *scan; scan++ ) {
        if( isalnum( *scan ) ) {
            continue;
        }
        if( !strchr( "'?!`/-_.@#$^&~=+<>()%", *scan ) ) {
            return 0;
        }
    }
    return 1;
}

int ok_password( const char *password, dbref player ) {
    const char *scan;

    int num_upper = 0;

    int num_special = 0;

    int num_lower = 0;

    if( *password == '\0' ) {
        if( !mudstate.standalone )
            notify_quiet( player,
                          "Null passwords are not allowed." );
        return 0;
    }

    for( scan = password; *scan; scan++ ) {
        if( !( isprint( *scan ) && !isspace( *scan ) ) ) {
            if( !mudstate.standalone )
                notify_quiet( player,
                              "Illegal character in password." );
            return 0;
        }
        if( isupper( *scan ) ) {
            num_upper++;
        } else if( islower( *scan ) ) {
            num_lower++;
        } else if( ( *scan != '\'' ) && ( *scan != '-' ) ) {
            num_special++;
        }
    }

    /*
     * Needed.  Change it if you like, but be sure yours is the same.
     */
    if( ( strlen( password ) == 13 ) &&
            ( password[0] == 'X' ) && ( password[1] == 'X' ) ) {
        if( !mudstate.standalone )
            notify_quiet( player,
                          "Please choose another password." );
        return 0;
    }

    if( !mudstate.standalone && mudconf.safer_passwords ) {
        if( num_upper < 1 ) {
            notify_quiet( player,
                          "The password must contain at least one capital letter." );
            return 0;
        }
        if( num_lower < 1 ) {
            notify_quiet( player,
                          "The password must contain at least one lowercase letter." );
            return 0;
        }
        if( num_special < 1 ) {
            notify_quiet( player,
                          "The password must contain at least one number or a symbol other than the apostrophe or dash." );
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------------------
 * handle_ears: Generate the 'grows ears' and 'loses ears' messages.
 */

void handle_ears( dbref thing, int could_hear, int can_hear ) {
    char *buff, *bp;

    int gender;

    if( could_hear != can_hear ) {
        bp = buff = alloc_lbuf( "handle_ears" );
        if( isExit( thing ) ) {
            safe_exit_name( thing, buff, &bp );
        } else {
            safe_name( thing, buff, &bp );
        }
        gender = get_gender( thing );
        notify_check( thing, thing, ( MSG_ME | MSG_NBR | MSG_LOC | MSG_INV ), "%s %s %s listening.", buff, ( ( gender == 4 ) ? "are" : "is" ), ( can_hear ? "now" : "no longer" )  ); 
        free_lbuf( buff );
    }
}

/* for lack of better place the @switch code is here */

void do_switch( dbref player, dbref cause, int key, char *expr, char *args[], int nargs, char *cargs[], int ncargs ) {
    int a, any, now;

    char *buff, *tbuf, *bp, *str;

    if( !expr || ( nargs <= 0 ) ) {
        return;
    }

    now = key & SWITCH_NOW;
    key &= ~SWITCH_NOW;

    if( key == SWITCH_DEFAULT ) {
        if( mudconf.switch_df_all ) {
            key = SWITCH_ANY;
        } else {
            key = SWITCH_ONE;
        }
    }
    /*
     * now try a wild card match of buff with stuff in args
     */

    any = 0;
    buff = bp = alloc_lbuf( "do_switch" );
    for( a = 0; ( a < ( nargs - 1 ) ) && args[a] && args[a + 1]; a += 2 ) {
        bp = buff;
        str = args[a];
        exec( buff, &bp, player, cause, cause,
              EV_FCHECK | EV_EVAL | EV_TOP, &str, cargs, ncargs );
        if( wild_match( buff, expr ) ) {
            tbuf = replace_string( SWITCH_VAR, expr, args[a + 1] );
            if( now ) {
                process_cmdline( player, cause, tbuf,
                                 cargs, ncargs, NULL );
            } else {
                wait_que( player, cause, 0, NOTHING, 0,
                          tbuf, cargs, ncargs, mudstate.rdata );
            }
            free_lbuf( tbuf );
            if( key == SWITCH_ONE ) {
                free_lbuf( buff );
                return;
            }
            any = 1;
        }
    }
    free_lbuf( buff );
    if( ( a < nargs ) && !any && args[a] ) {
        tbuf = replace_string( SWITCH_VAR, expr, args[a] );
        if( now ) {
            process_cmdline( player, cause, tbuf, cargs, ncargs,
                             NULL );
        } else {
            wait_que( player, cause, 0, NOTHING, 0, tbuf, cargs,
                      ncargs, mudstate.rdata );
        }
        free_lbuf( tbuf );
    }
}

/* ---------------------------------------------------------------------------
 * do_end: Stop processing an action list, based on a conditional.
 */

void do_end( dbref player, dbref cause, int key, char *condstr, char *cmdstr, char *args[], int nargs ) {
    int k = key & ENDCMD_ASSERT;

    int n = xlate( condstr );

    if( ( !k && n ) || ( k && !n ) ) {
        mudstate.break_called = 1;
        if( cmdstr && *cmdstr ) {
            wait_que( player, cause, 0, NOTHING, 0, cmdstr,
                      args, nargs, mudstate.rdata );
        }
    }
}

/* ---------------------------------------------------------------------------
 * Command hooks.
 */

void do_hook( dbref player, dbref cause, int key, char *cmdname, char *target ) {
    CMDENT *cmdp;

    char *p;

    ATTR *ap;

    HOOKENT *hp;

    dbref thing, aowner;

    int atr, aflags;

    for( p = cmdname; p && *p; p++ ) {
        *p = tolower( *p );
    }
    if( !cmdname ||
            ( ( cmdp = ( CMDENT * ) hashfind( cmdname,
                       &mudstate.command_htab ) ) == NULL ) ||
            ( cmdp->callseq & CS_ADDED ) ) {
        notify( player, "That is not a valid built-in command." );
        return;
    }

    if( key == 0 ) {

        /*
         * List hooks only
         */

        if( cmdp->pre_hook ) {
            ap = atr_num( cmdp->pre_hook->atr );
            if( !ap ) {
                notify( player, "Before Hook contains bad attribute number." );
            } else {
                notify_check( player, player , MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Before Hook: #%d/%s", cmdp->pre_hook->thing, ap->name );
            }
        } else {
            notify( player, "Before Hook: none" );
        }

        if( cmdp->post_hook ) {
            ap = atr_num( cmdp->post_hook->atr );
            if( !ap ) {
                notify( player, "After Hook contains bad attribute number." );
            } else {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "After Hook: #%d/%s", cmdp->post_hook->thing, ap->name );
            }
        } else {
            notify( player, "After Hook: none" );
        }

        if( cmdp->userperms ) {
            ap = atr_num( cmdp->userperms->atr );
            if( !ap ) {
                notify( player, "User Permissions contains bad attribute number." );
            } else {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "User Permissions: #%d/%s", cmdp->userperms->thing, ap->name );
            }
        } else {
            notify( player, "User Permissions: none" );
        }

        return;
    }

    /*
     * Check for the hook flags.
     */

    if( key & HOOK_PRESERVE ) {
        cmdp->callseq &= ~CS_PRIVATE;
        cmdp->callseq |= CS_PRESERVE;
        notify( player,
                "Hooks will preserve the state of the global registers." );
        return;
    }
    if( key & HOOK_NOPRESERVE ) {
        cmdp->callseq &= ~( CS_PRESERVE | CS_PRIVATE );
        notify( player,
                "Hooks will not preserve the state of the global registers." );
        return;
    }
    if( key & HOOK_PRIVATE ) {
        cmdp->callseq &= ~CS_PRESERVE;
        cmdp->callseq |= CS_PRIVATE;
        notify( player, "Hooks will use private global registers." );
        return;
    }

    /*
     * If we didn't get a target, this is a hook deletion.
     */

    if( !target || !*target ) {
        if( key & HOOK_BEFORE ) {
            if( cmdp->pre_hook ) {
                XFREE( cmdp->pre_hook, "do_hook" );
                cmdp->pre_hook = NULL;
            }
            notify( player, "Hook removed." );
        } else if( key & HOOK_AFTER ) {
            if( cmdp->post_hook ) {
                XFREE( cmdp->post_hook, "do_hook" );
                cmdp->post_hook = NULL;
            }
            notify( player, "Hook removed." );
        } else if( key & HOOK_PERMIT ) {
            if( cmdp->userperms ) {
                XFREE( cmdp->userperms, "do_hook" );
                cmdp->userperms = NULL;
            }
            notify( player, "User-defined permissions removed." );
        } else {
            notify( player, "Unknown command switch." );
        }
        return;
    }

    /*
     * Find target object and attribute. Make sure it can be read, and
     * * that we control the object.
     */

    if( !parse_attrib( player, target, &thing, &atr, 0 ) ) {
        notify( player, NOMATCH_MESSAGE );
        return;
    }
    if( !Controls( player, thing ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }
    if( atr == NOTHING ) {
        notify( player, "No such attribute." );
        return;
    }
    ap = atr_num( atr );
    if( !ap ) {
        notify( player, "No such attribute." );
        return;
    }
    atr_get_info( thing, atr, &aowner, &aflags );
    if( !See_attr( player, thing, ap, aowner, aflags ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }

    /*
     * All right, we have what we need. Go allocate a hook.
     */

    hp = ( HOOKENT * ) XMALLOC( sizeof( HOOKENT ), "do_hook" );
    hp->thing = thing;
    hp->atr = atr;

    /*
     * If that kind of hook already existed, get rid of it. Put in the
     * * new one.
     */

    if( key & HOOK_BEFORE ) {
        if( cmdp->pre_hook ) {
            XFREE( cmdp->pre_hook, "do_hook" );
        }
        cmdp->pre_hook = hp;
        notify( player, "Hook added." );
    } else if( key & HOOK_AFTER ) {
        if( cmdp->post_hook ) {
            XFREE( cmdp->post_hook, "do_hook" );
        }
        cmdp->post_hook = hp;
        notify( player, "Hook added." );
    } else if( key & HOOK_PERMIT ) {
        if( cmdp->userperms ) {
            XFREE( cmdp->userperms, "do_hook" );
        }
        cmdp->userperms = hp;
        notify( player,
                "User-defined permissions will now be checked." );
    } else {
        XFREE( hp, "do_hook" );
        notify( player, "Unknown command switch." );
    }
}

/* ---------------------------------------------------------------------------
 * Command overriding and friends.
 */

void do_addcommand( dbref player, dbref cause, int key, char *name, char *command ) {
    CMDENT *old, *cmd;
    ADDENT *add, *nextp;
    dbref thing;
    int atr;
    char *s;
    char s1[MBUF_SIZE];

    /*
     * Sanity-check the command name and make it case-insensitive.
     */

    if( !*name || ( name[0] == '_' && name[1] == '_' ) ) {
        notify( player, "That is not a valid command name." );
        return;
    }

    for( s = name; *s; s++ ) {
        if( isspace( *s ) || ( *s == ESC_CHAR ) ) {
            notify( player, "That is not a valid command name." );
            return;
        }
        *s = tolower( *s );
    }

    if( !parse_attrib( player, command, &thing, &atr, 0 ) ||
            ( atr == NOTHING ) ) {
        notify( player, "No such attribute." );
        return;
    }

    old = ( CMDENT * ) hashfind( name, &mudstate.command_htab );

    if( old && ( old->callseq & CS_ADDED ) ) {

        /*
         * If it's already found in the hash table, and it's being
         * added using the same object and attribute...
         */

        for( nextp = ( ADDENT * ) old->info.added; nextp != NULL;
                nextp = nextp->next ) {
            if( ( nextp->thing == thing ) && ( nextp->atr == atr ) ) {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s already added.", name );
                return;
            }
        }

        /*
         * else tack it on to the existing entry...
         */

        add = ( ADDENT * ) XMALLOC( sizeof( ADDENT ), "addcommand.add" );
        add->thing = thing;
        add->atr = atr;
        add->name = XSTRDUP( name, "addcommand.addname" );
        add->next = ( ADDENT * ) old->info.added;
        if( key & ADDCMD_PRESERVE ) {
            old->callseq |= CS_ACTOR;
        } else {
            old->callseq &= ~CS_ACTOR;
        }
        old->info.added = add;
    } else {
        if( old ) {
            /*
             * Delete the old built-in
             */
            hashdelete( name, &mudstate.command_htab );
        }

        cmd = ( CMDENT * ) XMALLOC( sizeof( CMDENT ), "addcommand.cmd" );

        cmd->cmdname = XSTRDUP( name, "addcommand.cmdname" );
        cmd->switches = NULL;
        cmd->perms = 0;
        cmd->extra = 0;
        cmd->pre_hook = NULL;
        cmd->post_hook = NULL;
        cmd->userperms = NULL;
        cmd->callseq = CS_ADDED | CS_ONE_ARG |
                       ( ( old && ( old->callseq & CS_LEADIN ) ) ? CS_LEADIN : 0 ) |
                       ( ( key & ADDCMD_PRESERVE ) ? CS_ACTOR : 0 );
        add = ( ADDENT * ) XMALLOC( sizeof( ADDENT ), "addcommand.add" );
        add->thing = thing;
        add->atr = atr;
        add->name = XSTRDUP( name, "addcommand.addname" );
        add->next = NULL;
        cmd->info.added = add;

        hashadd( cmd->cmdname, ( int * ) cmd, &mudstate.command_htab, 0 );

        if( old ) {
            /*
             * If this command was the canonical form of the
             * * command (not an alias), point its aliases to
             * * the added command, while keeping the __ alias.
             */
            if( !strcmp( name, old->cmdname ) ) {
                snprintf(s1, MBUF_SIZE, "__%s", old->cmdname );
                hashdelete( s1, &mudstate.command_htab );
                hashreplall( ( int * ) old, ( int * ) cmd, &mudstate.command_htab );
                hashadd( s1, ( int * ) old, &mudstate.command_htab, 0 );
            }
        }
    }

    /*
     * We reset the one letter commands here so you can overload them
     */

    reset_prefix_cmds();
    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Command %s added.", name );
}

void do_listcommands( dbref player, dbref cause, int key, char *name ) {
    CMDENT *old;

    ADDENT *nextp;

    int didit = 0;

    char *s, *keyname;

    /*
     * Let's make this case insensitive...
     */

    for( s = name; *s; s++ ) {
        *s = tolower( *s );
    }

    if( *name ) {
        old = ( CMDENT * ) hashfind( name, &mudstate.command_htab );

        if( old && ( old->callseq & CS_ADDED ) ) {
            if( strcmp( name, old->cmdname ) ) {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s: alias for %s", name, old->cmdname );
                return;
            }

            for( nextp = ( ADDENT * ) old->info.added; nextp != NULL;
                    nextp = nextp->next ) {
                notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s: #%d/%s", nextp->name, nextp->thing, ( ( ATTR * ) atr_num( nextp->atr ) )->name );
            }
        } else {
            notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s not found in command table.", name );
        }
        return;
    } else {
        for( keyname = hash_firstkey( &mudstate.command_htab );
                keyname != NULL;
                keyname = hash_nextkey( &mudstate.command_htab ) ) {

            old =
                ( CMDENT * ) hashfind( keyname,
                                       &mudstate.command_htab );

            if( old && ( old->callseq & CS_ADDED ) ) {
                if( strcmp( keyname, old->cmdname ) ) {
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s: alias for %s", keyname, old->cmdname );
                    continue;
                }

                for( nextp = ( ADDENT * ) old->info.added;
                        nextp != NULL; nextp = nextp->next ) {
                    notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "%s: #%d/%s", nextp->name, nextp->thing, ( ( ATTR * ) atr_num( nextp->atr ) )->name );
                    didit = 1;
                }
            }
        }
    }
    if( !didit ) {
        notify( player, "No added commands found in command table." );
    }
}

void do_delcommand( dbref player, dbref cause, int key, char *name, char *command ) {
    CMDENT *old, *cmd;
    ADDENT *prev = NULL, *nextp;
    dbref thing;
    int atr;
    char *s;
    char s1[MBUF_SIZE];

    if( !*name ) {
        notify( player, "Sorry." );
        return;
    }

    if( *command ) {
        if( !parse_attrib( player, command, &thing, &atr, 0 ) ||
                ( atr == NOTHING ) ) {
            notify( player, "No such attribute." );
            return;
        }
    }

    /*
     * Let's make this case insensitive...
     */

    for( s = name; *s; s++ ) {
        *s = tolower( *s );
    }

    old = ( CMDENT * ) hashfind( name, &mudstate.command_htab );

    if( old && ( old->callseq & CS_ADDED ) ) {
        if( !*command ) {
            for( prev = ( ADDENT * ) old->info.added; prev != NULL;
                    prev = nextp ) {
                nextp = prev->next;
                /*
                 * Delete it!
                 */
                XFREE( prev->name, "delcommand.name" );
                XFREE( prev, "delcommand.addent" );
            }
            hashdelete( name, &mudstate.command_htab );
            snprintf(s1, MBUF_SIZE, "__%s", old->cmdname );
            if( ( cmd = ( CMDENT * ) hashfind( s1, &mudstate.command_htab ) ) != NULL ) {
                hashadd( cmd->cmdname, ( int * ) cmd,
                         &mudstate.command_htab, 0 );
                /*
                 * in case we deleted by alias
                 */
                if( strcmp( name, cmd->cmdname ) ) {
                    hashadd( name, ( int * ) cmd,
                             &mudstate.command_htab,
                             HASH_ALIAS );
                }

                /*
                 * the __ alias may have been temporarily
                 * * marked as the original hash entry
                 */
                snprintf(s1, MBUF_SIZE, "__%s", cmd->cmdname );
                hashdelete( s1, &mudstate.command_htab );
                hashadd( s1, ( int * ) cmd, &mudstate.command_htab, HASH_ALIAS );

                hashreplall( ( int * ) old, ( int * ) cmd, &mudstate.command_htab );
            } else {
                hashdelall( ( int * ) old, &mudstate.command_htab );
            }
            XFREE( old->cmdname, "delcommand.cmdname" );
            XFREE( old, "delcommand.cmdp" );
            reset_prefix_cmds();
            notify( player, "Done" );
            return;
        } else {
            for( nextp = ( ADDENT * ) old->info.added; nextp != NULL;
                    nextp = nextp->next ) {
                if( ( nextp->thing == thing )
                        && ( nextp->atr == atr ) ) {
                    /*
                     * Delete it!
                     */
                    XFREE( nextp->name, "delcommand.name" );
                    if( !prev ) {
                        if( !nextp->next ) {
                            hashdelete( name, &mudstate. command_htab );
                            snprintf(s1, MBUF_SIZE, "__%s", name );
                            if( ( cmd = ( CMDENT * ) hashfind ( s1, &mudstate.command_htab ) ) != NULL ) {
                                hashadd( cmd->cmdname, ( int * ) cmd, &mudstate.command_htab, 0 );
                                /*
                                 * in case we deleted by alias
                                 */
                                if( strcmp
                                        ( name,
                                          cmd->
                                          cmdname ) ) {
                                    hashadd
                                    ( name,
                                      ( int * ) cmd, &mudstate.command_htab, HASH_ALIAS );
                                }

                                /*
                                 * the __ alias may have been temporarily
                                 * * marked as the original hash entry
                                 */
                                 snprintf(s1, MBUF_SIZE, "__%s", cmd->cmdname );
                                hashdelete( s1, &mudstate.command_htab );
                                hashadd( s1, ( int * ) cmd,&mudstate.command_htab, HASH_ALIAS );

                                hashreplall( ( int * ) old, ( int * ) cmd, &mudstate.command_htab );
                            } else {
                                hashdelall( ( int * ) old, &mudstate.command_htab );
                            }
                            XFREE( old->cmdname, "delcommand.cmdname" );
                            XFREE( old, "delcommand.cmdp" );
                        } else {
                            old->info.added = nextp->next;
                            XFREE( nextp, "delcommand.addent" );
                        }
                    } else {
                        prev->next = nextp->next;
                        XFREE( nextp,
                               "delcommand.addent" );
                    }
                    reset_prefix_cmds();
                    notify( player, "Done" );
                    return;
                }
                prev = nextp;
            }
            notify( player, "Command not found in command table." );
        }
    } else {
        notify( player, "Command not found in command table." );
    }
}

/* @program 'glues' a user's input to a command. Once executed, the first
 * string input from any of the doer's logged in descriptors will be
 * substituted in <command> as %0. Commands already queued by the doer
 * will be processed normally.
 */

void handle_prog( DESC *d, char *message ) {
    DESC *all, *dsave;

    char *cmd;

    dbref aowner;

    int aflags, alen;

    /*
     * Allow the player to pipe a command while in interactive mode.
     * * Use telnet protocol's GOAHEAD command to show prompt
     */

    if( *message == '|' ) {

        dsave = d;
        do_command( d, message + 1, 1 );

        if( dsave == d ) {

            /*
             * We MUST check if we still have a descriptor, and it's
             * * the same one, since we could have piped a LOGOUT or
             * * QUIT!
             */

            /*
             * Use telnet protocol's GOAHEAD command to show prompt, make
             * sure that we haven't been issues an @quitprogram
             */

            if( d->program_data != NULL ) {
                queue_rawstring( d, NULL, "> \377\371" );
            }
            return;
        }
    }
    cmd = atr_get( d->player, A_PROGCMD, &aowner, &aflags, &alen );
    wait_que( d->program_data->wait_cause, d->player, 0, NOTHING, 0, cmd,
              ( char ** ) &message, 1, d->program_data->wait_data );

    /*
     * First, set 'all' to a descriptor we find for this player
     */

    all = ( DESC * ) nhashfind( d->player, &mudstate.desc_htab );
    Free_RegData( all->program_data->wait_data );
    XFREE( all->program_data, "program_data" );

    /*
     * Set info for all player descriptors to NULL
     */

    DESC_ITER_PLAYER( d->player, all )
    all->program_data = NULL;

    atr_clr( d->player, A_PROGCMD );
    free_lbuf( cmd );
}

static int ok_program( dbref player, dbref doer ) {
    if( ( !( Prog( player ) || Prog( Owner( player ) ) ) && !Controls( player, doer ) )
            || ( God( doer ) && !God( player ) ) ) {
        notify( player, NOPERM_MESSAGE );
        return 0;
    }
    if( !isPlayer( doer ) || !Good_obj( doer ) ) {
        notify( player, "No such player." );
        return 0;
    }
    if( !Connected( doer ) ) {
        notify( player, "Sorry, that player is not connected." );
        return 0;
    }
    return 1;
}

void do_quitprog( dbref player, dbref cause, int key, char *name ) {
    DESC *d;

    dbref doer;

    int isprog = 0;

    if( *name ) {
        doer = match_thing( player, name );
    } else {
        doer = player;
    }

    if( !ok_program( player, doer ) ) {
        return;
    }

    DESC_ITER_PLAYER( doer, d ) {
        if( d->program_data != NULL ) {
            isprog = 1;
        }
    }

    if( !isprog ) {
        notify( player, "Player is not in an @program." );
        return;
    }

    d = ( DESC * ) nhashfind( doer, &mudstate.desc_htab );

    Free_RegData( d->program_data->wait_data );
    XFREE( d->program_data, "program_data" );

    /*
     * Set info for all player descriptors to NULL
     */

    DESC_ITER_PLAYER( doer, d )
    d->program_data = NULL;

    atr_clr( doer, A_PROGCMD );
    notify( player, "@program cleared." );
    notify( doer, "Your @program has been terminated." );
}

void do_prog( dbref player, dbref cause, int key, char *name, char *command ) {
    DESC *d;

    PROG *program;

    int atr, aflags, lev, found;

    dbref doer, thing, aowner, parent;

    ATTR *ap;

    char *attrib, *msg;

    if( !name || !*name ) {
        notify( player, "No players specified." );
        return;
    }
    doer = match_thing( player, name );

    if( !ok_program( player, doer ) ) {
        return;
    }

    msg = command;
    attrib = parse_to( &msg, ':', 1 );

    if( msg && *msg ) {
        notify( doer, msg );
    }
    parse_attrib( player, attrib, &thing, &atr, 0 );
    if( atr != NOTHING ) {
        if( !atr_pget_info( thing, atr, &aowner, &aflags ) ) {
            notify( player, "Attribute not present on object." );
            return;
        }
        ap = atr_num( atr );

        /*
         * We've got to find this attribute in the object's
         * * parent chain, somewhere.
         */

        found = 0;
        ITER_PARENTS( thing, parent, lev ) {
            if( atr_get_info( parent, atr, &aowner, &aflags ) ) {
                found = 1;
                break;
            }
        }

        if( !found ) {
            notify( player, "Attribute not present on object." );
            return;
        }

        if( God( player ) ||
                ( !God( thing ) &&
                  See_attr( player, thing, ap, aowner, aflags ) &&
                  ( Wizard( player ) || ( aowner == Owner( player ) ) ) ) ) {
            /*
             * Check if cause already has an @prog input pending
             */
            DESC_ITER_PLAYER( doer, d ) {
                if( d->program_data != NULL ) {
                    notify( player,
                            "Input already pending." );
                    return;
                }
            }
            atr_add_raw( doer, A_PROGCMD, atr_get_raw( parent, atr ) );
        } else {
            notify( player, NOPERM_MESSAGE );
            return;
        }
    } else {
        notify( player, "No such attribute." );
        return;
    }

    program = ( PROG * ) XMALLOC( sizeof( PROG ), "do_prog" );
    program->wait_cause = player;

    if( mudstate.rdata ) {
        Alloc_RegData( "do_prog.gdata", mudstate.rdata,
                       program->wait_data );
        Copy_RegData( "do_prog.regs", mudstate.rdata,
                      program->wait_data );
    } else {
        program->wait_data = NULL;
    }

    /*
     * Now, start waiting.
     */
    DESC_ITER_PLAYER( doer, d ) {
        d->program_data = program;

        /*
         * Use telnet protocol's GOAHEAD command to show prompt
         */
        queue_rawstring( d, NULL, "> \377\371" );
    }

}

/* ---------------------------------------------------------------------------
 * do_restart: Restarts the game.
 */

void do_restart( dbref player, dbref cause, int key ) {
    LOGFILETAB *lp;

    MODULE *mp;

    char *name;

    if( mudstate.dumping ) {
        notify( player, "Dumping. Please try again later." );
        return;
    }

    /*
     * Make sure what follows knows we're restarting. No need to clear
     * * this, since this process is going away-- this is also set on
     * * startup when the restart.db is read.
     */

    mudstate.restarting = 1;

    raw_broadcast( 0, "GAME: Restart by %s, please wait.", Name( Owner( player ) ) );
    name = log_getname( player, "do_restart" );
    log_write( LOG_ALWAYS, "WIZ", "RSTRT" "Restart by %s", name );
    XFREE( name, "do_restart" );
    /*
     * Do a dbck first so we don't end up with an inconsistent state.
     * * Otherwise, since we don't write out GOING objects, the initial
     * * dbck at startup won't have valid data to work with in order to
     * * clean things out.
     */

    do_dbck( NOTHING, NOTHING, 0 );

    /*
     * Dump databases, etc.
     */

    dump_database_internal( DUMP_DB_RESTART );

    SYNC;
    CLOSE;

    if( slave_socket != -1 ) {
        shutdown( slave_socket, 2 );
        close( slave_socket );
        slave_socket = -1;
    }
    if( slave_pid != 0 ) {
        kill( slave_pid, SIGKILL );
    }

    logfile_close();

    alarm( 0 );
    dump_restart_db();

    for (mp = mudstate.modules_list; mp != NULL; mp = mp->next) {
        lt_dlclose( mp->handle );
    }

    execl( mudconf.exec_path, mudconf.exec_path, ( char * ) "-r" , ( char * ) "-c", mudconf.config_file, NULL );
}

/* ---------------------------------------------------------------------------
 * do_comment: Implement the @@ (comment) command. Very cpu-intensive :-)
 * do_eval is similar, except it gets passed on arg.
 */

void do_comment( dbref player, dbref cause, int key ) {
}

void do_eval( dbref player, dbref cause, int key, char *str ) {
}

/* ---------------------------------------------------------------------------
 */

static dbref promote_dflt( dbref old, dbref new ) {
    switch( new ) {
    case NOPERM:
        return NOPERM;
    case AMBIGUOUS:
        if( old == NOPERM ) {
            return old;
        } else {
            return new;
        }
    }

    if( ( old == NOPERM ) || ( old == AMBIGUOUS ) ) {
        return old;
    }

    return NOTHING;
}

dbref match_possessed( dbref player, dbref thing, char *target, dbref dflt, int check_enter ) {
    dbref result, result1;

    int control;

    char *buff, *start, *place, *s1, *d1, *temp;

    /*
     * First, check normally
     */

    if( Good_obj( dflt ) ) {
        return dflt;
    }

    /*
     * Didn't find it directly.  Recursively do a contents check
     */

    start = target;
    while( *target ) {

        /*
         * Fail if no ' characters
         */

        place = target;
        target = strchr( place, '\'' );
        if( ( target == NULL ) || !*target ) {
            return dflt;
        }

        /*
         * If string started with a ', skip past it
         */

        if( place == target ) {
            target++;
            continue;
        }
        /*
         * If next character is not an s or a space, skip past
         */

        temp = target++;
        if( !*target ) {
            return dflt;
        }
        if( ( *target != 's' ) && ( *target != 'S' ) && ( *target != ' ' ) ) {
            continue;
        }

        /*
         * If character was not a space make sure the following
         * * character is a space.
         */

        if( *target != ' ' ) {
            target++;
            if( !*target ) {
                return dflt;
            }
            if( *target != ' ' ) {
                continue;
            }
        }
        /*
         * Copy the container name to a new buffer so we can
         * * terminate it.
         */

        buff = alloc_lbuf( "match_possessed" );
        for( s1 = start, d1 = buff; *s1 && ( s1 < temp );
                *d1++ = ( *s1++ ) );
        *d1 = '\0';

        /*
         * Look for the container here and in our inventory.  Skip
         * * past if we can't find it.
         */

        init_match( thing, buff, NOTYPE );
        if( player == thing ) {
            match_neighbor();
            match_possession();
        } else {
            match_possession();
        }
        result1 = match_result();

        free_lbuf( buff );
        if( !Good_obj( result1 ) ) {
            dflt = promote_dflt( dflt, result1 );
            continue;
        }
        /*
         * If we don't control it and it is either dark or opaque,
         * * skip past.
         */

        control = Controls( player, result1 );
        if( ( Dark( result1 ) || Opaque( result1 ) ) && !control ) {
            dflt = promote_dflt( dflt, NOTHING );
            continue;
        }
        /*
         * Validate object has the ENTER bit set, if requested
         */

        if( ( check_enter ) && !Enter_ok( result1 ) && !control ) {
            dflt = promote_dflt( dflt, NOPERM );
            continue;
        }
        /*
         * Look for the object in the container
         */

        init_match( result1, target, NOTYPE );
        match_possession();
        result = match_result();
        result = match_possessed( player, result1, target, result,
                                  check_enter );
        if( Good_obj( result ) ) {
            return result;
        }
        dflt = promote_dflt( dflt, result );
    }
    return dflt;
}

/* ---------------------------------------------------------------------------
 * parse_range: break up <what>,<low>,<high> syntax
 */

void parse_range( char **name, dbref *low_bound, dbref *high_bound ) {
    char *buff1, *buff2;

    buff1 = *name;
    if( buff1 && *buff1 ) {
        *name = parse_to( &buff1, ',', EV_STRIP_TS );
    }
    if( buff1 && *buff1 ) {
        buff2 = parse_to( &buff1, ',', EV_STRIP_TS );
        if( buff1 && *buff1 ) {
            while( *buff1 && isspace( *buff1 ) ) {
                buff1++;
            }
            if( *buff1 == NUMBER_TOKEN ) {
                buff1++;
            }
            *high_bound = ( int ) strtol( buff1, ( char ** ) NULL, 10 );
            if( *high_bound >= mudstate.db_top ) {
                *high_bound = mudstate.db_top - 1;
            }
        } else {
            *high_bound = mudstate.db_top - 1;
        }
        while( *buff2 && isspace( *buff2 ) ) {
            buff2++;
        }
        if( *buff2 == NUMBER_TOKEN ) {
            buff2++;
        }
        *low_bound = ( int ) strtol( buff2, ( char ** ) NULL, 10 );
        if( *low_bound < 0 ) {
            *low_bound = 0;
        }
    } else {
        *low_bound = 0;
        *high_bound = mudstate.db_top - 1;
    }
}

int parse_thing_slash( dbref player, char *thing, char **after, dbref *it ) {
    char *str;

    /*
     * get name up to /
     */
    for( str = thing; *str && ( *str != '/' ); str++ );

    /*
     * If no / in string, return failure
     */

    if( !*str ) {
        *after = NULL;
        *it = NOTHING;
        return 0;
    }
    *str++ = '\0';
    *after = str;

    /*
     * Look for the object
     */

    init_match( player, thing, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    *it = match_result();

    /*
     * Return status of search
     */

    return ( Good_obj( *it ) );
}

extern NAMETAB lock_sw[];

int get_obj_and_lock( dbref player, char *what, dbref *it, ATTR **attr, char *errmsg, char **bufc ) {
    char *str, *tbuf;

    int anum;

    tbuf = alloc_lbuf( "get_obj_and_lock" );
    strcpy( tbuf, what );
    if( parse_thing_slash( player, tbuf, &str, it ) ) {

        /*
         * <obj>/<lock> syntax, use the named lock
         */

        anum = search_nametab( player, lock_sw, str );
        if( anum == -1 ) {
            free_lbuf( tbuf );
            safe_str( "#-1 LOCK NOT FOUND", errmsg, bufc );
            return 0;
        }
    } else {

        /*
         * Not <obj>/<lock>, do a normal get of the default lock
         */

        *it = match_thing( player, what );
        if( !Good_obj( *it ) ) {
            free_lbuf( tbuf );
            safe_str( "#-1 NOT FOUND", errmsg, bufc );
            return 0;
        }
        anum = A_LOCK;
    }

    /*
     * Get the attribute definition, fail if not found
     */

    free_lbuf( tbuf );
    *attr = atr_num( anum );
    if( !( *attr ) ) {
        safe_str( "#-1 LOCK NOT FOUND", errmsg, bufc );
        return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * where_is: Returns place where obj is linked into a list.
 * ie. location for players/things, source for exits, NOTHING for rooms.
 */

dbref where_is( dbref what ) {
    dbref loc;

    if( !Good_obj( what ) ) {
        return NOTHING;
    }

    switch( Typeof( what ) ) {
    case TYPE_PLAYER:
    case TYPE_THING:
    case TYPE_ZONE:
        loc = Location( what );
        break;
    case TYPE_EXIT:
        loc = Exits( what );
        break;
    default:
        loc = NOTHING;
        break;
    }
    return loc;
}

/* ---------------------------------------------------------------------------
 * where_room: Return room containing player, or NOTHING if no room or
 * recursion exceeded.  If player is a room, returns itself.
 */

dbref where_room( dbref what ) {
    int count;

    for( count = mudconf.ntfy_nest_lim; count > 0; count-- ) {
        if( !Good_obj( what ) ) {
            break;
        }
        if( isRoom( what ) ) {
            return what;
        }
        if( !Has_location( what ) ) {
            break;
        }
        what = Location( what );
    }
    return NOTHING;
}

int locatable( dbref player, dbref it, dbref cause ) {
    dbref loc_it, room_it;

    int findable_room;

    /*
     * No sense if trying to locate a bad object
     */

    if( !Good_obj( it ) ) {
        return 0;
    }

    loc_it = where_is( it );

    /*
     * Succeed if we can examine the target, if we are the target, if
     * * we can examine the location, if a wizard caused the lookup,
     * * or if the target caused the lookup.
     */

    if( Examinable( player, it ) ||
            Find_Unfindable( player ) ||
            ( loc_it == player ) ||
            ( ( loc_it != NOTHING ) &&
              ( Examinable( player, loc_it ) || loc_it == where_is( player ) ) ) ||
            Wizard( cause ) || ( it == cause ) ) {
        return 1;
    }

    room_it = where_room( it );
    if( Good_obj( room_it ) ) {
        findable_room = !Hideout( room_it );
    } else {
        findable_room = 1;
    }

    /*
     * Succeed if we control the containing room or if the target is
     * * findable and the containing room is not unfindable.
     */

    if( ( ( room_it != NOTHING ) && Examinable( player, room_it ) ) ||
            Find_Unfindable( player ) || ( Findable( it ) && findable_room ) ) {
        return 1;
    }

    /*
     * We can't do it.
     */

    return 0;
}

/* ---------------------------------------------------------------------------
 * nearby: Check if thing is nearby player (in inventory, in same room, or
 * IS the room.
 */

int nearby( dbref player, dbref thing ) {
    int thing_loc, player_loc;

    if( !Good_obj( player ) || !Good_obj( thing ) ) {
        return 0;
    }
    thing_loc = where_is( thing );
    if( thing_loc == player ) {
        return 1;
    }
    player_loc = where_is( player );
    if( ( thing_loc == player_loc ) || ( thing == player_loc ) ) {
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * master_attr: Get the evaluated text string of a master attribute.
 *              Note that this returns an lbuf, which must be freed by
 *              the caller.
 */

char *master_attr( dbref player, dbref thing, int what, char **sargs, int nsargs, int *f_ptr ) {
    /*
     * If the attribute exists, evaluate it and return pointer to lbuf.
     * * If not, return NULL.
     * * Respect global overrides.
     * * what is assumed to be more than 0.
     */

    char *d, *m, *buff, *bp, *str, *tbuf, *tp, *sp, *list, *bb_p, *lp;

    int t, aflags, alen, is_ok, lev;

    dbref aowner, master, parent, obj;

    ATTR *ap;

    GDATA *preserve;

    if( NoDefault( thing ) ) {
        master = NOTHING;
    } else {
        switch( Typeof( thing ) ) {
        case TYPE_ROOM:
            master = mudconf.room_defobj;
            break;
        case TYPE_EXIT:
            master = mudconf.exit_defobj;
            break;
        case TYPE_PLAYER:
            master = mudconf.player_defobj;
            break;
        case TYPE_GARBAGE:
            return NULL;
            break;	/* NOTREACHED */
        default:
            master = mudconf.thing_defobj;
        }
        if( master == thing ) {
            master = NOTHING;
        }
    }

    m = NULL;
    d = atr_pget( thing, what, &aowner, &aflags, &alen );
    if( Good_obj( master ) ) {
        ap = atr_num( what );
        t = ( ap && ( ap->flags & AF_DEFAULT ) ) ? 1 : 0;
    } else {
        t = 0;
    }
    if( t ) {
        m = atr_pget( master, what, &aowner, &aflags, &alen );
    }
    if( f_ptr ) {
        *f_ptr = aflags;
    }

    if( !( *d || ( t && *m ) ) ) {
        free_lbuf( d );
        if( m ) {
            free_lbuf( m );
        }
        return NULL;
    }

    /*
     * Construct any arguments that we're going to pass along on the
     * * stack.
     */

    switch( what ) {
    case A_LEXITS_FMT:
        list = alloc_lbuf( "master_attr.list" );
        bb_p = lp = list;
        is_ok = Darkened( player, thing );
        ITER_PARENTS( thing, parent, lev ) {
            if( !Has_exits( parent ) ) {
                continue;
            }
            DOLIST( obj, Exits( parent ) ) {
                if( Can_See_Exit( player, obj, is_ok ) ) {
                    if( lp != bb_p ) {
                        safe_chr( ' ', list, &lp );
                    }
                    safe_dbref( list, &lp, obj );
                }
            }
        }
        *lp = '\0';
        is_ok = 1;
        break;
    case A_LCON_FMT:
        list = alloc_lbuf( "master_attr.list" );
        bb_p = lp = list;
        is_ok = Sees_Always( player, thing );
        DOLIST( obj, Contents( thing ) ) {
            if( Can_See( player, obj, is_ok ) ) {
                if( lp != bb_p ) {
                    safe_chr( ' ', list, &lp );
                }
                safe_dbref( list, &lp, obj );
            }
        }
        *lp = '\0';
        is_ok = 1;
        break;
    default:
        list = NULL;
        is_ok = nsargs;
        break;
    }

    /*
     * Go do it.
     */

    preserve = save_global_regs( "master_attr_save" );
    buff = bp = alloc_lbuf( "master_attr.1" );
    if( t && *m ) {
        str = m;
        if( *d ) {
            sp = d;
            tbuf = tp = alloc_lbuf( "master_attr.deval" );
            exec( tbuf, &tp, thing, player, player,
                  EV_EVAL | EV_FIGNORE | EV_TOP,
                  &sp, ( ( list == NULL ) ? sargs : &list ), is_ok );
            exec( buff, &bp, thing, player, player,
                  EV_EVAL | EV_FIGNORE | EV_TOP, &str, &tbuf, 1 );
            free_lbuf( tbuf );
        } else {
            exec( buff, &bp, thing, player, player,
                  EV_EVAL | EV_FIGNORE | EV_TOP,
                  &str, ( ( list == NULL ) ? sargs : &list ), is_ok );
        }
    } else if( *d ) {
        str = d;
        exec( buff, &bp, thing, player, player,
              EV_EVAL | EV_FIGNORE | EV_TOP,
              &str, ( ( list == NULL ) ? sargs : &list ), is_ok );
    }
    *bp = '\0';
    free_lbuf( d );
    if( m ) {
        free_lbuf( m );
    }
    if( list ) {
        free_lbuf( list );
    }
    restore_global_regs( "master_attr_restore", preserve );
    return buff;
}

/* ---------------------------------------------------------------------------
 * did_it: Have player do something to/with thing
 */

void did_it( dbref player, dbref thing, int what, const char *def, int owhat, const char *odef, int awhat, int ctrl_flags, char *args[], int nargs, int msg_key ) {
    GDATA *preserve;

    char *d, *m, *buff, *act, *charges, *bp, *str;

    char *tbuf, *tp, *sp;

    dbref loc, aowner;

    int t, num, aflags, alen, need_pres;

    dbref master;

    ATTR *ap;

    int retval = 0;

    /*
     * If we need to call exec() from within this function, we first save
     * * the state of the global registers, in order to avoid munging them
     * * inappropriately. Do note that the restoration to their original
     * * values occurs BEFORE the execution of the @a-attribute. Therefore,
     * * any changing of setq() values done in the @-attribute and @o-attribute
     * * will NOT be passed on. This prevents odd behaviors that result from
     * * odd @verbs and so forth (the idea is to preserve the caller's control
     * * of the global register values).
     */

    need_pres = 0;

    if( NoDefault( thing ) ) {
        master = NOTHING;
    } else {
        switch( Typeof( thing ) ) {
        case TYPE_ROOM:
            master = mudconf.room_defobj;
            break;
        case TYPE_EXIT:
            master = mudconf.exit_defobj;
            break;
        case TYPE_PLAYER:
            master = mudconf.player_defobj;
            break;
        default:
            master = mudconf.thing_defobj;
        }
        if( master == thing || !Good_obj( master ) ) {
            master = NOTHING;
        }
    }

    /*
     * Module call. Modules can return a negative number, zero, or a
     * * positive number.
     * * Positive: Stop calling modules. Return; do not execute normal did_it().
     * * Zero: Continue calling modules. Execute normal did_it() if we get
     * *       to the end of the modules and nothing has returned non-zero.
     * * Negative: Stop calling modules. Execute normal did_it().
     */
    CALL_SOME_MODULES( retval, did_it, ( player, thing, master, what, def, owhat, def, awhat, ctrl_flags, args, nargs, msg_key ) );
    if( retval > 0 ) {
        return;
    }

    /*
     * message to player
     */

    m = NULL;

    if( what > 0 ) {

        /*
         * Check for global attribute format override. If it exists,
         * * use that. The actual attribute text we were provided
         * * will be passed to that as %0. (Note that if a global
         * * override exists, we never use a supplied server default.)
         * *
         * * Otherwise, we just go evaluate what we've got, and
         * * if that's nothing, we go do the default.
         */

        d = atr_pget( thing, what, &aowner, &aflags, &alen );
        if( Good_obj( master ) ) {
            ap = atr_num( what );
            t = ( ap && ( ap->flags & AF_DEFAULT ) ) ? 1 : 0;
        } else {
            t = 0;
        }

        if( t ) {
            m = atr_pget( master, what, &aowner, &aflags, &alen );
        }

        if( *d || ( t && *m ) ) {
            need_pres = 1;
            preserve = save_global_regs( "did_it_save" );
            buff = bp = alloc_lbuf( "did_it.1" );
            if( t && *m ) {
                str = m;
                if( *d ) {
                    sp = d;
                    tbuf = tp = alloc_lbuf( "did_it.deval" );
                    exec( tbuf, &tp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &sp, args, nargs );
                    exec( buff, &bp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &str, &tbuf, 1 );
                    free_lbuf( tbuf );
                } else {
                    exec( buff, &bp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &str, ( char ** ) NULL, 0 );
                }
            } else if( *d ) {
                str = d;
                exec( buff, &bp, thing, player, player,
                      EV_EVAL | EV_FIGNORE | EV_TOP,
                      &str, args, nargs );
            }
            *bp = '\0';
#ifdef PUEBLO_SUPPORT
            if( ( aflags & AF_HTML ) && Html( player ) ) {
                char *buff_cp = buff + strlen( buff );

                safe_crlf( buff, &buff_cp );
                notify_html( player, buff );
            } else {
                notify( player, buff );
            }
#else
            notify( player, buff );
#endif				/* PUEBLO_SUPPORT */
            free_lbuf( buff );
        } else if( def ) {
            notify( player, def );
        }
        free_lbuf( d );
    } else if( ( what < 0 ) && def ) {
        notify( player, def );
    }

    if( m ) {
        free_lbuf( m );
    }

    /*
     * message to neighbors
     */

    m = NULL;

    if( ( owhat > 0 ) && Has_location( player ) &&
            Good_obj( loc = Location( player ) ) ) {

        d = atr_pget( thing, owhat, &aowner, &aflags, &alen );
        if( Good_obj( master ) ) {
            ap = atr_num( owhat );
            t = ( ap && ( ap->flags & AF_DEFAULT ) ) ? 1 : 0;
        } else {
            t = 0;
        }

        if( t ) {
            m = atr_pget( master, owhat, &aowner, &aflags, &alen );
        }

        if( *d || ( t && *m ) ) {
            if( !need_pres ) {
                need_pres = 1;
                preserve = save_global_regs( "did_it_save" );
            }
            buff = bp = alloc_lbuf( "did_it.2" );
            if( t && *m ) {
                str = m;
                if( *d ) {
                    sp = d;
                    tbuf = tp = alloc_lbuf( "did_it.deval" );
                    exec( tbuf, &tp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &sp, args, nargs );
                    exec( buff, &bp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &str, &tbuf, 1 );
                    free_lbuf( tbuf );
                } else if( odef ) {
                    exec( buff, &bp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &str, ( char ** ) &odef, 1 );
                } else {
                    exec( buff, &bp, thing, player, player,
                          EV_EVAL | EV_FIGNORE | EV_TOP,
                          &str, ( char ** ) NULL, 0 );
                }
            } else if( *d ) {
                str = d;
                exec( buff, &bp, thing, player, player,
                      EV_EVAL | EV_FIGNORE | EV_TOP,
                      &str, args, nargs );
            }
            *bp = '\0';
            if( *buff ) {
                if( aflags & AF_NONAME ) {
                    notify_except2( loc, player, player, thing, msg_key, NULL, buff);
                } else {
                    notify_except2( loc, player, player, thing, msg_key, "%s %s", Name( player ), buff );
                }
            }
            free_lbuf( buff );
        } else if( odef ) {
            if( ctrl_flags & VERB_NONAME ) {
                notify_except2( loc, player, player, thing, msg_key, NULL, odef);
            } else {
                notify_except2( loc, player, player, thing, msg_key, "%s %s", Name( player ), odef );
            }
        }
        free_lbuf( d );
    } else if( ( owhat < 0 ) && odef && Has_location( player ) &&
               Good_obj( loc = Location( player ) ) ) {
        if( ctrl_flags & VERB_NONAME ) {
            notify_except2( loc, player, player, thing, msg_key, NULL,  odef);
        } else {
            notify_except2( loc, player, player, thing, msg_key, "%s %s", Name( player ), odef );
        }
    }

    if( m ) {
        free_lbuf( m );
    }

    /*
     * If we preserved the state of the global registers, restore them.
     */

    if( need_pres ) {
        restore_global_regs( "did_it_restore", preserve );
    }

    /*
     * do the action attribute
     */

    if( awhat > 0 ) {
        if( * ( act = atr_pget( thing, awhat, &aowner, &aflags, &alen ) ) ) {
            charges =
                atr_pget( thing, A_CHARGES, &aowner, &aflags,
                          &alen );
            if( *charges ) {
                num = ( int ) strtol( charges, ( char ** ) NULL, 10 );
                if( num > 0 ) {
                    buff = alloc_sbuf( "did_it.charges" );
                    sprintf( buff, "%d", num - 1 );
                    atr_add_raw( thing, A_CHARGES, buff );
                    free_sbuf( buff );
                } else if( * ( buff = atr_pget( thing, A_RUNOUT,
                                                &aowner, &aflags, &alen ) ) ) {
                    free_lbuf( act );
                    act = buff;
                } else {
                    free_lbuf( act );
                    free_lbuf( buff );
                    free_lbuf( charges );
                    return;
                }
            }
            free_lbuf( charges );

            /*
             * Skip any leading $<command>: or ^<monitor>: pattern.
             * * If we run off the end of string without finding an unescaped
             * * ':' (or there's nothing after it), go back to the beginning
             * * of the string and just use that.
             */

            if( ( *act == '$' ) || ( *act == '^' ) ) {
                for( tp = act + 1;
                        *tp && ( ( *tp != ':' )
                                 || ( * ( tp - 1 ) == '\\' ) ); tp++ );
                if( !*tp ) {
                    tp = act;
                } else {
                    tp++;	/* must advance past the ':' */
                }
            } else {
                tp = act;
            }

            /*
             * Go do it.
             */

            if( ctrl_flags & ( VERB_NOW | TRIG_NOW ) ) {
                preserve = save_global_regs( "did_it_save2" );
                process_cmdline( thing, player, tp, args, nargs,
                                 NULL );
                restore_global_regs( "did_it_restore2",
                                     preserve );
            } else {
                wait_que( thing, player, 0, NOTHING, 0, tp,
                          args, nargs, mudstate.rdata );
            }
        }
        free_lbuf( act );
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_verb: Command interface to did_it.
 */

void do_verb( dbref player, dbref cause, int key, char *victim_str, char *args[], int nargs ) {
    dbref actor, victim, aowner;

    int what, owhat, awhat, nxargs, restriction, aflags, i;

    ATTR *ap;

    const char *whatd, *owhatd;

    char *xargs[NUM_ENV_VARS];

    /*
     * Look for the victim
     */

    if( !victim_str || !*victim_str ) {
        notify( player, "Nothing to do." );
        return;
    }
    /*
     * Get the victim
     */

    init_match( player, victim_str, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    victim = noisy_match_result();
    if( !Good_obj( victim ) ) {
        return;
    }

    /*
     * Get the actor.  Default is my cause
     */

    if( ( nargs >= 1 ) && args[0] && *args[0] ) {
        init_match( player, args[0], NOTYPE );
        match_everything( MAT_EXIT_PARENTS );
        actor = noisy_match_result();
        if( !Good_obj( actor ) ) {
            return;
        }
    } else {
        actor = cause;
    }

    /*
     * Check permissions.  There are two possibilities * 1: Player * * *
     * controls both victim and actor.  In this case victim runs *    his
     *
     * *  * *  * * action list. * 2: Player controls actor.  In this case
     * * victim * does  * not run his *    action list and any attributes
     * * that * player cannot  * read from *    victim are defaulted.
     */

    if( !controls( player, actor ) ) {
        notify_quiet( player, NOPERM_MESSAGE );
        return;
    }
    restriction = !controls( player, victim );

    what = -1;
    owhat = -1;
    awhat = -1;
    whatd = NULL;
    owhatd = NULL;
    nxargs = 0;

    /*
     * Get invoker message attribute
     */

    if( nargs >= 2 ) {
        ap = atr_str( args[1] );
        if( ap && ( ap->number > 0 ) ) {
            what = ap->number;
        }
    }
    /*
     * Get invoker message default
     */

    if( ( nargs >= 3 ) && args[2] && *args[2] ) {
        whatd = args[2];
    }
    /*
     * Get others message attribute
     */

    if( nargs >= 4 ) {
        ap = atr_str( args[3] );
        if( ap && ( ap->number > 0 ) ) {
            owhat = ap->number;
        }
    }
    /*
     * Get others message default
     */

    if( ( nargs >= 5 ) && args[4] && *args[4] ) {
        owhatd = args[4];
    }
    /*
     * Get action attribute
     */

    if( nargs >= 6 ) {
        ap = atr_str( args[5] );
        if( ap ) {
            awhat = ap->number;
        }
    }
    /*
     * Get arguments
     */

    if( nargs >= 7 ) {
        parse_arglist( victim, actor, actor, args[6], '\0',
                       EV_STRIP_LS | EV_STRIP_TS, xargs, NUM_ENV_VARS,
                       ( char ** ) NULL, 0 );
        for( nxargs = 0; ( nxargs < NUM_ENV_VARS ) && xargs[nxargs];
                nxargs++ );
    }
    /*
     * If player doesn't control both, enforce visibility restrictions.
     * Regardless of control we still check if the player can read the
     * attribute, since we don't want him getting wiz-readable-only attrs.
     */

    atr_get_info( victim, what, &aowner, &aflags );
    if( what != -1 ) {
        ap = atr_num( what );
        if( !ap || !Read_attr( player, victim, ap, aowner, aflags ) ||
                ( restriction &&
                  ( ( ap->number == A_DESC ) && !mudconf.read_rem_desc &&
                    !Examinable( player, victim )
                    && !nearby( player, victim ) ) ) ) {
            what = -1;
        }
    }
    atr_get_info( victim, owhat, &aowner, &aflags );
    if( owhat != -1 ) {
        ap = atr_num( owhat );
        if( !ap || !Read_attr( player, victim, ap, aowner, aflags ) ||
                ( restriction &&
                  ( ( ap->number == A_DESC ) && !mudconf.read_rem_desc &&
                    !Examinable( player, victim )
                    && !nearby( player, victim ) ) ) ) {
            owhat = -1;
        }
    }
    if( restriction ) {
        awhat = 0;
    }

    /*
     * Go do it
     */

    did_it( actor, victim, what, whatd, owhat, owhatd, awhat,
            key & ( VERB_NOW | VERB_NONAME ), xargs, nxargs,
            ( ( ( key & VERB_SPEECH ) ? MSG_SPEECH : 0 ) |
              ( ( key & VERB_MOVE ) ? MSG_MOVE : 0 ) |
              ( ( key & VERB_PRESENT ) ? MSG_PRESENCE : 0 ) ) );

    /*
     * Free user args
     */

    for( i = 0; i < nxargs; i++ ) {
        free_lbuf( xargs[i] );
    }

}

/* ---------------------------------------------------------------------------
 * do_include: Run included text. This is very similar to a @trigger/now,
 * except that we only need to be able to read the attr, not control the
 * object, and it uses the original stack by default. If we get passed args,
 * we replace the entirety of the included text, just like a @trigger/now.
 * Also, unlike a trigger, we remain the thing running the action, and we
 * do not deplete charges.
 */

void do_include( dbref player, dbref cause, int key, char *object, char *argv[], int nargs, char *cargs[], int ncargs ) {
    dbref thing, aowner;
    int attrib, aflags, alen;
    char *act, *tp;
    char s[MBUF_SIZE];

    /*
     * Get the attribute. Default to getting it off ourselves.
     */

    snprintf( s, MBUF_SIZE,  "me/%s", object );
    if( !( ( parse_attrib( player, object, &thing, &attrib, 0 ) && ( attrib != NOTHING ) ) || ( parse_attrib( player, s, &thing, &attrib, 0 ) && ( attrib != NOTHING ) ) ) ) {
        notify_quiet( player, "No match." );
        return;
    }

    if( * ( act = atr_pget( thing, attrib, &aowner, &aflags, &alen ) ) ) {

        /*
         * Skip leading $command: or ^monitor:
         */

        if( ( *act == '$' ) || ( *act == '^' ) ) {
            for( tp = act + 1;
                    *tp && ( ( *tp != ':' ) || ( * ( tp - 1 ) == '\\' ) );
                    tp++ );
            if( !*tp ) {
                tp = act;
            } else {
                tp++;	/* must advance past the ':' */
            }
        } else {
            tp = act;
        }

        /*
         * Go do it. Use stack if we have it, otherwise use command stack.
         * * Note that an empty stack is still one arg but it's empty.
         */

        if( ( nargs > 1 ) || ( ( nargs == 1 ) && *argv[0] ) ) {
            process_cmdline( player, cause, tp, argv, nargs, NULL );
        } else {
            process_cmdline( player, cause, tp, cargs, ncargs,
                             NULL );
        }
    }

    free_lbuf( act );
}

/* ---------------------------------------------------------------------------
 * do_redirect: Redirect PUPPET, TRACE, VERBOSE output to another player.
 */

void do_redirect( dbref player, dbref cause, int key, char *from_name, char *to_name ) {
    dbref from_ref, to_ref;

    NUMBERTAB *np;

    /*
     * Find what object we're redirecting from. We must either control it,
     * * or it must be REDIR_OK.
     */

    init_match( player, from_name, NOTYPE );
    match_everything( 0 );
    from_ref = noisy_match_result();
    if( !Good_obj( from_ref ) ) {
        return;
    }

    /*
     * If we have no second argument, we are un-redirecting something
     * * which is already redirected. We can get rid of it if we control
     * * the object being redirected, or we control the target of the
     * * redirection.
     */

    if( !to_name || !*to_name ) {
        if( !H_Redirect( from_ref ) ) {
            notify( player, "That object is not being redirected." );
            return;
        }
        np = ( NUMBERTAB * ) nhashfind( from_ref, &mudstate.redir_htab );
        if( np ) {
            /*
             * This should always be true -- if we have the flag the
             * * hashtable lookup should succeed -- but just in case,
             * * we check. (We clear the flag upon startup.)
             * * If we have a weird situation, we don't care whether
             * * or not the control criteria gets met; we just fix it.
             */
            if( !Controls( player, from_ref ) && ( np->num != player ) ) {
                notify( player, NOPERM_MESSAGE );
                return;
            }
            if( np->num != player ) {
                notify_check( np->num, np->num, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Output from %s(#%d) is no being redirected to you.", Name( from_ref ), from_ref );
            }
            XFREE( np, "redir_struct" );
            nhashdelete( from_ref, &mudstate.redir_htab );
        }
        s_Flags3( from_ref, Flags3( from_ref ) & ~HAS_REDIRECT );
        notify( player, "Redirection stopped." );
        if( from_ref != player ) {
            notify( from_ref,
                    "You are no longer being redirected." );
        }
        return;
    }

    /*
     * If the object is already being redirected, we cannot do so again.
     */

    if( H_Redirect( from_ref ) ) {
        notify( player, "That object is already being redirected." );
        return;
    }

    /*
     * To redirect something, it needs to either be REDIR_OK or we
     * * need to control it.
     */

    if( !Controls( player, from_ref ) && !Redir_ok( from_ref ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }

    /*
     * Find the player that we're redirecting to. We must control the
     * * player.
     */

    to_ref = lookup_player( player, to_name, 1 );
    if( !Good_obj( to_ref ) ) {
        notify( player, "No such player." );
        return;
    }
    if( !Controls( player, to_ref ) ) {
        notify( player, NOPERM_MESSAGE );
        return;
    }

    /*
     * Insert it into the hashtable.
     */

    np = ( NUMBERTAB * ) XMALLOC( sizeof( NUMBERTAB ), "redir_struct" );
    np->num = to_ref;
    nhashadd( from_ref, ( int * ) np, &mudstate.redir_htab );
    s_Flags3( from_ref, Flags3( from_ref ) | HAS_REDIRECT );

    if( from_ref != player ) {
        notify_check( from_ref, from_ref ,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "You have been redirected to %s.", Name( to_ref ) );
    }
    if( to_ref != player ) {
        notify_check( to_ref, to_ref, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Output from %s(#%d) has been redirected to you.", Name( from_ref ), from_ref );
    }
    notify( player, "Redirected." );
}

/* ---------------------------------------------------------------------------
 * do_reference: Manipulate nrefs.
 */

void do_reference( dbref player, dbref cause, int key, char *ref_name, char *obj_name ) {
    HASHENT *hptr;

    HASHTAB *htab;

    int i, len, total, is_global;

    char tbuf[LBUF_SIZE], outbuf[LBUF_SIZE], *tp, *bp, *buff, *s;

    dbref target, *np;

    if( key & NREF_LIST ) {

        htab = &mudstate.nref_htab;

        if( !ref_name || !*ref_name ) {
            /*
             * Global only.
             */
            is_global = 1;
            tbuf[0] = '_';
            tbuf[1] = '\0';
            len = 1;
        } else {
            is_global = 0;
            if( !string_compare( ref_name, "me" ) ) {
                target = player;
            } else {
                target = lookup_player( player, ref_name, 1 );
                if( target == NOTHING ) {
                    notify( player, "No such player." );
                    return;
                }
                if( !Controls( player, target ) ) {
                    notify( player, NOPERM_MESSAGE );
                    return;
                }
            }
            tp = tbuf;
            safe_ltos( tbuf, &tp, player );
            safe_chr( '.', tbuf, &tp );
            *tp = '\0';
            len = strlen( tbuf );
        }

        total = 0;
        for( i = 0; i < htab->hashsize; i++ ) {
            for( hptr = htab->entry[i]; hptr != NULL;
                    hptr = hptr->next ) {
                if( !strncmp( tbuf, hptr->target.s, len ) ) {
                    total++;
                    bp = outbuf;
                    safe_sprintf( outbuf, &bp, "%s:  ", ( ( is_global ) ? hptr->target.s : strchr( hptr->target.s, '.' ) + 1 ) );
                    buff = unparse_object( player, * ( hptr->data ), 0 );
                    safe_str( buff, outbuf, &bp );
                    free_lbuf( buff );
                    if( Owner( player ) != Owner( * ( hptr->data ) ) ) {
                        safe_str( ( char * ) " [owner: ", outbuf, &bp );
                        buff = unparse_object( player, Owner( * ( hptr->data ) ), 0 );
                        safe_str( buff, outbuf, &bp );
                        free_lbuf( buff );
                        safe_chr( ']', outbuf, &bp );
                    }
                    *bp = '\0';
                    notify( player, outbuf );
                }
            }
        }

        notify_check( player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Total references: %d", total );

        return;
    }

    /*
     * We can only reference objects that we can examine.
     */

    if( obj_name && *obj_name ) {
        target = match_thing( player, obj_name );
        if( !Good_obj( target ) ) {
            return;
        }
        if( !Examinable( player, target ) ) {
            notify( player, NOPERM_MESSAGE );
            return;
        }
    } else {
        target = NOTHING;	/* indicates clear */
    }

    /*
     * If the reference name starts with an underscore, it's global.
     * * Only wizards can do that.
     */

    tp = tbuf;
    if( *ref_name == '_' ) {
        if( !Wizard( player ) ) {
            notify( player, NOPERM_MESSAGE );
            return;
        }
    } else {
        safe_ltos( tbuf, &tp, player );
        safe_chr( '.', tbuf, &tp );
    }
    for( s = ref_name; *s; s++ ) {
        safe_chr( tolower( *s ), tbuf, &tp );
    }
    *tp = '\0';

    /*
     * Does this reference name exist already?
     */

    np = ( int * ) hashfind( tbuf, &mudstate.nref_htab );
    if( np ) {
        if( target == NOTHING ) {
            XFREE( np, "nref" );
            hashdelete( tbuf, &mudstate.nref_htab );
            notify( player, "Reference cleared." );
        } else if( *np == target ) {
            /*
             * Already got it.
             */
            notify( player,
                    "That reference has already been made." );
        } else {
            /*
             * Replace it.
             */
            XFREE( np, "nref" );
            np = ( dbref * ) XMALLOC( sizeof( dbref ), "nref" );
            *np = target;
            hashrepl( tbuf, np, &mudstate.nref_htab );
            notify( player, "Reference updated." );
        }
        return;
    }

    /*
     * Didn't find it. We've got a new one (or an error if we have no
     * * target but the reference didn't exist).
     */

    if( target == NOTHING ) {
        notify( player, "No such reference to clear." );
        return;
    }

    np = ( dbref * ) XMALLOC( sizeof( dbref ), "nref" );
    *np = target;
    hashadd( tbuf, np, &mudstate.nref_htab, 0 );
    notify( player, "Referenced." );
}

/* ---------------------------------------------------------------------------
 * Miscellaneous stuff below.
 */
