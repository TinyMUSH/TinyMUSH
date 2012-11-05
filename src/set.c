/* set.c - commands which set parameters */

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

#include "match.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "ansi.h"		/* required by code */

extern NAMETAB indiv_attraccess_nametab[];

dbref match_controlled( dbref player, const char *name ) {
    dbref mat;

    init_match( player, name, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    mat = noisy_match_result();
    if( Good_obj( mat ) && !Controls( player, mat ) ) {
        notify_quiet( player, NOPERM_MESSAGE );
        return NOTHING;
    } else {
        return ( mat );
    }
}

dbref match_controlled_quiet( dbref player, const char *name ) {
    dbref mat;

    init_match( player, name, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    mat = match_result();
    if( Good_obj( mat ) && !Controls( player, mat ) ) {
        return NOTHING;
    } else {
        return ( mat );
    }
}

dbref match_affected( dbref player, const char *name ) {
    dbref mat;

    /*
     * We allow control, as well as having the same owner.
     */

    init_match( player, name, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    mat = noisy_match_result();
    if( Good_obj( mat ) && ( Owner( player ) != Owner( mat ) ) &&
            !Controls( player, mat ) ) {
        notify_quiet( player, NOPERM_MESSAGE );
        return NOTHING;
    } else {
        return ( mat );
    }
}

void do_chzone( dbref player, dbref cause, int key, const char *name, const char *newobj ) {
    dbref thing;

    dbref zone;

    if( !mudconf.have_zones ) {
        notify( player, "Zones disabled." );
        return;
    }
    init_match( player, name, NOTYPE );
    match_everything( 0 );
    if( ( thing = noisy_match_result() ) == NOTHING ) {
        return;
    }

    if( !newobj || !*newobj || !strcasecmp( newobj, "none" ) ) {
        zone = NOTHING;
    } else {
        init_match( player, newobj, NOTYPE );
        match_everything( 0 );
        if( ( zone = noisy_match_result() ) == NOTHING ) {
            return;
        }

        if( ( Typeof( zone ) != TYPE_THING )
                && ( Typeof( zone ) != TYPE_ROOM ) ) {
            notify( player, "Invalid zone object type." );
            return;
        }
    }

    if( !Wizard( player ) && !( Controls( player, thing ) ) &&
            !( check_zone_for_player( player, thing ) ) &&
            !( db[player].owner == db[thing].owner ) ) {
        notify( player, "You don't have the power to shift reality." );
        return;
    }

    /*
     * a player may change an object's zone to NOTHING or to an object he
     * * owns
     */
    if( ( zone != NOTHING ) && !Wizard( player ) &&
            !( Controls( player, zone ) ) &&
            !( db[player].owner == db[zone].owner ) ) {
        notify( player, "You cannot move that object to that zone." );
        return;
    }

    /*
     * only rooms may be zoned to other rooms
     */

    if( ( zone != NOTHING ) &&
            ( Typeof( zone ) == TYPE_ROOM ) && Typeof( thing ) != TYPE_ROOM ) {
        notify( player, "Only rooms may have parent rooms." );
        return;
    }

    /*
     * everything is okay, do the change
     */

    s_Zone( thing, zone );
    if( Typeof( thing ) != TYPE_PLAYER ) {
        /*
         * We do not strip flags and powers on players, due to the
         * * inconvenience involved in resetting them. Players will just
         * * have to be careful when @chzone'ing players with special
         * * privileges.
         * * For all other objects, we behave like @chown does.
         */
        if( key & CHZONE_NOSTRIP ) {
            if( !God( player ) ) {
                s_Flags( thing, Flags( thing ) & ~WIZARD );
            }
        } else {
            s_Flags( thing,
                     Flags( thing ) & ~mudconf.stripped_flags.word1 );
            s_Flags2( thing,
                      Flags2( thing ) & ~mudconf.stripped_flags.word2 );
            s_Flags3( thing,
                      Flags3( thing ) & ~mudconf.stripped_flags.word3 );
        }
        if( !( key & CHZONE_NOSTRIP ) || !God( player ) ) {
            s_Powers( thing, 0 );
            s_Powers2( thing, 0 );
        }
    }
    notify( player, "Zone changed." );
    s_Modified( thing );
}

void do_name( dbref player, dbref cause, int key, const char *name, char *newname ) {
    dbref thing;

    char *buff, *thingname;

    if( ( thing = match_controlled( player, name ) ) == NOTHING ) {
        return;
    }

    /*
     * check for bad name
     */
    if( ( *newname == '\0' ) || ( strip_ansi_len( newname ) == 0 ) ) {
        notify_quiet( player, "Give it what new name?" );
        return;
    }
    /*
     * check for renaming a player
     */
    if( isPlayer( thing ) ) {

        buff = trim_spaces( ( char * ) newname );
        if( !ok_player_name( buff ) || !badname_check( buff ) ) {
            notify_quiet( player, "You can't use that name." );
            free_lbuf( buff );
            return;
        } else if( string_compare( buff, Name( thing ) ) &&
                   ( lookup_player( NOTHING, buff, 0 ) != NOTHING ) ) {

            /*
             * string_compare allows changing foo to Foo, etc.
             */

            notify_quiet( player, "That name is already in use." );
            free_lbuf( buff );
            return;
        }
        /*
         * everything ok, notify
         */
        thingname = log_getname( thing, "do_name" );
        log_write( LOG_SECURITY, "SEC", "CNAME", "%s renamed to %s", thingname, buff );
        XFREE( thingname, "do_name" );

        if( Suspect( thing ) ) {
            raw_broadcast( WIZARD,
                           "[Suspect] %s renamed to %s", Name( thing ), buff );
        }
        delete_player_name( thing, Name( thing ) );
        s_Name( thing, buff );
        add_player_name( thing, Name( thing ) );
        if( !Quiet( player ) && !Quiet( thing ) ) {
            notify_quiet( player, "Name set." );
        }
        free_lbuf( buff );
        s_Modified( thing );
        return;
    } else {
        if( !ok_name( newname ) ) {
            notify_quiet( player, "That is not a reasonable name." );
            return;
        }
        /*
         * everything ok, change the name
         */
        s_Name( thing, newname );
        if( !Quiet( player ) && !Quiet( thing ) ) {
            notify_quiet( player, "Name set." );
        }
        s_Modified( thing );
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_alias: Make an alias for a player or object.
 */

static void set_player_aliases( dbref player, dbref target, char *oldalias, char *list, int aflags ) {
    int i, j, n_aliases, retcode;

    char *p, *tokp;

    char alias_buf[LBUF_SIZE], tmp_buf[LBUF_SIZE],
         *alias_ptrs[LBUF_SIZE / 2];


    /*
     * Clear out the original alias, so we can rewrite a new alias
     * * that uses the same names, if necessary.
     */

    Clear_Player_Aliases( target, oldalias );

    /*
     * Don't nibble the original buffer. Copy it all into an array, because
     * * we have to eat leading and trailing spaces.
     */

    retcode = 1;
    strcpy( tmp_buf, list );
    for( n_aliases = 0, p = strtok_r( tmp_buf, ";", &tokp ); p;
            n_aliases++, p = strtok_r( NULL, ";", &tokp ) ) {
        alias_ptrs[n_aliases] = trim_spaces( p );
    }

    /*
     * Enforce a maximum number of aliases.
     */

    if( n_aliases > mudconf.max_player_aliases ) {
        notify_quiet( player,
                      tmprintf( "You cannot have more than %d aliases.",
                                mudconf.max_player_aliases ) );
        retcode = 0;
    }

    /*
     * Enforce player name regulations.
     */

    for( i = 0; retcode && ( i < n_aliases ); i++ ) {
        if( lookup_player( NOTHING, alias_ptrs[i], 0 ) != NOTHING ) {
            notify_quiet( player,
                          tmprintf( "The name '%s' is already in use.",
                                    alias_ptrs[i] ) );
            retcode = 0;
        } else if( !( badname_check( alias_ptrs[i] ) &&
                      ok_player_name( alias_ptrs[i] ) ) ) {
            notify_quiet( player,
                          tmprintf( "You cannot use '%s' as an alias.",
                                    alias_ptrs[i] ) );
            retcode = 0;
        } else {

            /*
             * Make sure this alias doesn't duplicate another in the list.
             */

            for( j = i + 1; retcode && ( j < n_aliases ); j++ ) {
                if( !strcasecmp( alias_ptrs[i], alias_ptrs[j] ) ) {
                    notify_quiet( player,
                                  tmprintf
                                  ( "You have duplicated '%s' in your alias list.",
                                    alias_ptrs[i] ) );
                    retcode = 0;
                }
            }
        }
    }

    /*
     * Construct a new alias list, with spaces removed.
     */

    for( i = 0, p = alias_buf; retcode && ( i < n_aliases ); i++ ) {
        if( add_player_name( target, alias_ptrs[i] ) ) {
            if( p != alias_buf ) {
                safe_chr( ';', alias_buf, &p );
            }
            safe_str( alias_ptrs[i], alias_buf, &p );
        } else {

            retcode = 0;
            notify_quiet( player,
                          tmprintf
                          ( "The alias '%s' is already in use or is illegal.",
                            alias_ptrs[i] ) );

            /*
             * Ugh. Now we have to delete aliases we added up 'til now.
             */

            for( j = 0; j < i; j++ ) {
                delete_player_name( target, alias_ptrs[j] );
            }
        }
    }

    /*
     * Free memory allocated by trim_spaces().
     */

    for( i = 0; i < n_aliases; i++ ) {
        free_lbuf( alias_ptrs[i] );
    }

    /*
     * Twiddle the alias attribute on the object. Note that we have to
     * * do this regardless of the outcome, since we wiped out the original
     * * aliases from the player name table earlier.
     */

    if( retcode ) {
        atr_add( target, A_ALIAS, alias_buf, Owner( player ), aflags );
        if( !Quiet( player ) ) {
            notify_quiet( player, "Alias set." );
        }
    } else {
        atr_clr( target, A_ALIAS );
        notify_quiet( player, "Alias cleared due to error." );
    }
}

void do_alias( dbref player, dbref cause, int key, char *name, char *alias ) {
    dbref thing, aowner;

    int aflags, alen;

    ATTR *ap;

    char *oldalias, *trimalias;

    if( ( thing = match_controlled( player, name ) ) == NOTHING ) {
        return;
    }

    /*
     * check for renaming a player
     */

    ap = atr_num( A_ALIAS );
    if( isPlayer( thing ) ) {

        /*
         * Fetch the old alias
         */

        oldalias = atr_get( thing, A_ALIAS, &aowner, &aflags, &alen );
        trimalias = trim_spaces( alias );

        if( !Controls( player, thing ) ) {

            /*
             * Make sure we have rights to do it.  We can't do
             * the normal Set_attr check because ALIAS is
             * set CONSTANT and we want to keep people from
             * doing &ALIAS and bypassing the player name checks.
             */

            notify_quiet( player, NOPERM_MESSAGE );
        } else if( !*trimalias ) {

            /*
             * New alias is null, just clear it
             */

            Clear_Player_Aliases( thing, oldalias );
            atr_clr( thing, A_ALIAS );
            if( !Quiet( player ) ) {
                notify_quiet( player, "Alias removed." );
            }
        } else {

            /*
             * Remove the old name and add the new name
             */

            set_player_aliases( player, thing, oldalias,
                                trimalias, aflags );
        }
        free_lbuf( trimalias );
        free_lbuf( oldalias );
    } else {
        atr_pget_info( thing, A_ALIAS, &aowner, &aflags );

        /*
         * Make sure we have rights to do it
         */

        if( !Set_attr( player, thing, ap, aflags ) ) {
            notify_quiet( player, NOPERM_MESSAGE );
        } else {
            atr_add( thing, A_ALIAS, alias, Owner( player ), aflags );
            if( !Quiet( player ) ) {
                notify_quiet( player, "Set." );
            }
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_lock: Set a lock on an object or attribute.
 */

void do_lock( dbref player, dbref cause, int key, char *name, char *keytext ) {
    dbref thing, aowner;

    int atr, aflags;

    ATTR *ap;

    struct boolexp *okey;

    if( parse_attrib( player, name, &thing, &atr, 0 ) ) {
        if( atr != NOTHING ) {
            if( !atr_get_info( thing, atr, &aowner, &aflags ) ) {
                notify_quiet( player,
                              "Attribute not present on object." );
                return;
            }
            ap = atr_num( atr );

            if( ap && Lock_attr( player, thing, ap, aowner ) ) {
                aflags |= AF_LOCK;
                atr_set_flags( thing, atr, aflags );
                if( !Quiet( player ) && !Quiet( thing ) )
                    notify_quiet( player,
                                  "Attribute locked." );
            } else {
                notify_quiet( player, NOPERM_MESSAGE );
            }
            return;
        }
    }
    init_match( player, name, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    thing = match_result();

    switch( thing ) {
    case NOTHING:
        notify_quiet( player, "I don't see what you want to lock!" );
        return;
    case AMBIGUOUS:
        notify_quiet( player,
                      "I don't know which one you want to lock!" );
        return;
    default:
        if( !controls( player, thing ) ) {
            notify_quiet( player, "You can't lock that!" );
            return;
        }
    }

    okey = parse_boolexp( player, keytext, 0 );
    if( okey == TRUE_BOOLEXP ) {
        notify_quiet( player, "I don't understand that key." );
    } else {

        /*
         * everything ok, do it
         */

        if( !key ) {
            key = A_LOCK;
        }
        atr_add_raw( thing, key, unparse_boolexp_quiet( player, okey ) );
        if( key == A_LDARK ) {
            s_Has_Darklock( thing );
        }
        if( !Quiet( player ) && !Quiet( thing ) ) {
            notify_quiet( player, "Locked." );
        }
    }
    free_boolexp( okey );
}

/*
 * ---------------------------------------------------------------------------
 * * Remove a lock from an object of attribute.
 */

void do_unlock( dbref player, dbref cause, int key, char *name ) {
    dbref thing, aowner;

    int atr, aflags;

    ATTR *ap;

    if( parse_attrib( player, name, &thing, &atr, 0 ) ) {
        if( atr != NOTHING ) {
            if( !atr_get_info( thing, atr, &aowner, &aflags ) ) {
                notify_quiet( player,
                              "Attribute not present on object." );
                return;
            }
            ap = atr_num( atr );

            if( ap && Lock_attr( player, thing, ap, aowner ) ) {
                aflags &= ~AF_LOCK;
                atr_set_flags( thing, atr, aflags );
                if( Owner( player != Owner( thing ) ) )
                    if( !Quiet( player ) && !Quiet( thing ) )
                        notify_quiet( player,
                                      "Attribute unlocked." );
            } else {
                notify_quiet( player, NOPERM_MESSAGE );
            }
            return;
        }
    }
    if( !key ) {
        key = A_LOCK;
    }
    if( ( thing = match_controlled( player, name ) ) != NOTHING ) {
        atr_clr( thing, key );
        if( key == A_LDARK ) {
            c_Has_Darklock( thing );
        }
        if( !Quiet( player ) && !Quiet( thing ) ) {
            notify_quiet( player, "Unlocked." );
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_unlink: Unlink an exit from its destination or remove a dropto.
 */

void do_unlink( dbref player, dbref cause, int key, char *name ) {
    dbref exit;

    init_match( player, name, TYPE_EXIT );
    match_everything( 0 );
    exit = match_result();

    switch( exit ) {
    case NOTHING:
        notify_quiet( player, "Unlink what?" );
        break;
    case AMBIGUOUS:
        notify_quiet( player, AMBIGUOUS_MESSAGE );
        break;
    default:
        if( !controls( player, exit ) ) {
            notify_quiet( player, NOPERM_MESSAGE );
        } else {
            switch( Typeof( exit ) ) {
            case TYPE_EXIT:
                s_Location( exit, NOTHING );
                if( !Quiet( player ) ) {
                    notify_quiet( player, "Unlinked." );
                }
                break;
            case TYPE_ROOM:
                s_Dropto( exit, NOTHING );
                if( !Quiet( player ) )
                    notify_quiet( player,
                                  "Dropto removed." );
                break;
            default:
                notify_quiet( player, "You can't unlink that!" );
                break;
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * do_chown: Change ownership of an object or attribute.
 */

void do_chown( dbref player, dbref cause, int key, char *name, char *newown ) {
    dbref thing, owner, aowner;

    int atr, aflags, do_it, cost, quota;

    ATTR *ap;

    if( parse_attrib( player, name, &thing, &atr, 1 ) ) {
        if( atr != NOTHING ) {
            if( !*newown ) {
                owner = Owner( thing );
            } else if( !( string_compare( newown, "me" ) ) ) {
                owner = Owner( player );
            } else {
                owner = lookup_player( player, newown, 1 );
            }

            /*
             * You may chown an attr to yourself if you
             * own the object and the attr is not locked.
             * You may chown an attr to the owner of the
             * object if you own the attribute. To do
             * anything else you must be a wizard. Only
             * #1 can chown attributes on #1.
             */

            if( !atr_get_info( thing, atr, &aowner, &aflags ) ) {
                notify_quiet( player,
                              "Attribute not present on object." );
                return;
            }
            do_it = 0;
            if( owner == NOTHING ) {
                notify_quiet( player,
                              "I couldn't find that player." );
            } else if( God( thing ) && !God( player ) ) {
                notify_quiet( player, NOPERM_MESSAGE );
            } else if( Wizard( player ) ) {
                do_it = 1;
            } else if( owner == Owner( player ) ) {

                /*
                 * chown to me: only if I own the obj
                 * and !locked
                 */

                if( !Controls( player, thing ) ||
                        ( aflags & AF_LOCK ) ) {
                    notify_quiet( player, NOPERM_MESSAGE );
                } else {
                    do_it = 1;
                }
            } else if( owner == Owner( thing ) ) {

                /*
                 * chown to obj owner: only if I own attr
                 * and !locked
                 */

                if( ( Owner( player ) != aowner ) ||
                        ( aflags & AF_LOCK ) ) {
                    notify_quiet( player, NOPERM_MESSAGE );
                } else {
                    do_it = 1;
                }
            } else {
                notify_quiet( player, NOPERM_MESSAGE );
            }

            if( !do_it ) {
                return;
            }

            ap = atr_num( atr );
            if( !ap || !Set_attr( player, player, ap, aflags ) ) {
                notify_quiet( player, NOPERM_MESSAGE );
                return;
            }
            atr_set_owner( thing, atr, owner );
            if( !Quiet( player ) )
                notify_quiet( player,
                              "Attribute owner changed." );
            s_Modified( thing );
            return;
        }
    }
    init_match( player, name, TYPE_THING );
    match_possession();
    match_here();
    match_exit();
    match_me();
    if( Chown_Any( player ) ) {
        match_player();
        match_absolute();
    }
    switch( thing = match_result() ) {
    case NOTHING:
        notify_quiet( player, "You don't have that!" );
        return;
    case AMBIGUOUS:
        notify_quiet( player, "I don't know which you mean!" );
        return;
    }

    if( !*newown || !( string_compare( newown, "me" ) ) ) {
        owner = Owner( player );
    } else {
        owner = lookup_player( player, newown, 1 );
    }

    cost = 1;
    quota = 1;
    switch( Typeof( thing ) ) {
    case TYPE_ROOM:
        cost = mudconf.digcost;
        quota = mudconf.room_quota;
        break;
    case TYPE_THING:
        cost = OBJECT_DEPOSIT( Pennies( thing ) );
        quota = mudconf.thing_quota;
        break;
    case TYPE_EXIT:
        cost = mudconf.opencost;
        quota = mudconf.exit_quota;
        break;
    case TYPE_PLAYER:
        cost = mudconf.robotcost;
        quota = mudconf.player_quota;
    }

    if( owner == NOTHING ) {
        notify_quiet( player, "I couldn't find that player." );
    } else if( isPlayer( thing ) && !God( player ) ) {
        notify_quiet( player, "Players always own themselves." );
    } else if( ( ( !controls( player, thing ) && !Chown_Any( player ) &&
                   !( Chown_ok( thing ) &&
                      could_doit( player, thing, A_LCHOWN ) ) ) ||
                 ( isThing( thing ) && ( Location( thing ) != player ) &&
                   !Chown_Any( player ) ) ) ||
               ( !controls( player, owner ) && !Chown_Any( player ) ) || God( thing ) ) {
        notify_quiet( player, NOPERM_MESSAGE );
    } else if( canpayfees( player, owner, cost, quota, Typeof( thing ) ) ) {
        payfees( owner, cost, quota, Typeof( thing ) );
        payfees( Owner( thing ), -cost, -quota, Typeof( thing ) );

        if( God( player ) ) {
            s_Owner( thing, owner );
        } else {
            s_Owner( thing, Owner( owner ) );
        }
        atr_chown( thing );

        /*
         * If we're not stripping flags, and we're God, don't strip the
         * * WIZARD flag. Otherwise, do that, at least.
         */
        if( key & CHOWN_NOSTRIP ) {
            if( God( player ) ) {
                s_Flags( thing,
                         ( Flags( thing ) & ~CHOWN_OK ) | HALT );
            } else {
                s_Flags( thing,
                         ( Flags( thing ) & ~( CHOWN_OK | WIZARD ) ) |
                         HALT );
            }
        } else {
            s_Flags( thing,
                     ( Flags( thing ) &
                       ~( CHOWN_OK | mudconf.stripped_flags.
                          word1 ) ) | HALT );
            s_Flags2( thing,
                      ( Flags2( thing ) & ~( mudconf.stripped_flags.word2 ) ) );
            s_Flags3( thing,
                      ( Flags3( thing ) & ~( mudconf.stripped_flags.word3 ) ) );
        }

        /*
         * Powers are only preserved by God with nostrip
         */

        if( !( key & CHOWN_NOSTRIP ) || !God( player ) ) {
            s_Powers( thing, 0 );
            s_Powers2( thing, 0 );
        }

        halt_que( NOTHING, thing );
        if( !Quiet( player ) ) {
            notify_quiet( player, "Owner changed." );
        }
        s_Modified( thing );
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_set: Set flags or attributes on objects, or flags on attributes.
 */

void set_attr_internal( dbref player, dbref thing, int attrnum, char *attrtext, int key, char *buff, char **bufc ) {
    dbref aowner;

    int aflags, could_hear;

    ATTR *attr;

    attr = atr_num( attrnum );
    atr_pget_info( thing, attrnum, &aowner, &aflags );
    if( attr && Set_attr( player, thing, attr, aflags ) ) {
        if( ( attr->check != NULL ) &&
                ( !( *attr->check )( 0, player, thing, attrnum, attrtext ) ) ) {
            if( buff ) {
                safe_noperm( buff, bufc );
            }
            return;
        }
        could_hear = Hearer( thing );
        atr_add( thing, attrnum, attrtext, Owner( player ),
                 aflags & ~AF_STRUCTURE );
        handle_ears( thing, could_hear, Hearer( thing ) );
        if( !( key & SET_QUIET ) && !Quiet( player ) && !Quiet( thing ) ) {
            notify_quiet( player, "Set." );
        }
    } else {
        if( buff ) {
            safe_noperm( buff, bufc );
        } else {
            notify_quiet( player, NOPERM_MESSAGE );
        }
    }
}

void do_set( dbref player, dbref cause, int key, char *name, char *flag ) {
    dbref thing, thing2, aowner;

    char *p, *buff;

    int atr, atr2, aflags, alen, clear, flagvalue, could_hear;

    ATTR *attr, *attr2;

    /*
     * See if we have the <obj>/<attr> form, which is how you set * * *
     * attribute * flags.
     */

    if( parse_attrib( player, name, &thing, &atr, 1 ) ) {
        if( atr != NOTHING ) {

            /*
             * You must specify a flag name
             */

            if( !flag || !*flag ) {
                notify_quiet( player,
                              "I don't know what you want to set!" );
                return;
            }
            /*
             * Check for clearing
             */

            clear = 0;
            if( *flag == NOT_TOKEN ) {
                flag++;
                clear = 1;
            }
            /*
             * Make sure player specified a valid attribute flag
             */

            flagvalue = search_nametab( player,
                                        indiv_attraccess_nametab, flag );
            if( flagvalue < 0 ) {
                notify_quiet( player, "You can't set that!" );
                return;
            }
            /*
             * Make sure the object has the attribute present
             */

            if( !atr_get_info( thing, atr, &aowner, &aflags ) ) {
                notify_quiet( player,
                              "Attribute not present on object." );
                return;
            }
            /*
             * Make sure we can write to the attribute
             */

            attr = atr_num( atr );
            if( !attr || !Set_attr( player, thing, attr, aflags ) ) {
                notify_quiet( player, NOPERM_MESSAGE );
                return;
            }
            /*
             * Go do it
             */

            if( clear ) {
                aflags &= ~flagvalue;
            } else {
                aflags |= flagvalue;
            }
            could_hear = Hearer( thing );
            atr_set_flags( thing, atr, aflags );

            /*
             * Tell the player about it.
             */

            handle_ears( thing, could_hear, Hearer( thing ) );
            if( !( key & SET_QUIET ) &&
                    !Quiet( player ) && !Quiet( thing ) ) {
                if( clear ) {
                    notify_quiet( player, "Cleared." );
                } else {
                    notify_quiet( player, "Set." );
                }
            }
            return;
        }
    }
    /*
     * find thing
     */

    if( ( thing = match_controlled( player, name ) ) == NOTHING ) {
        return;
    }

    /*
     * check for attribute set first
     */
    for( p = flag; *p && ( *p != ':' ); p++ );

    if( *p ) {
        *p++ = 0;
        atr = mkattr( flag );
        if( atr <= 0 ) {
            notify_quiet( player, "Couldn't create attribute." );
            return;
        }
        attr = atr_num( atr );
        if( !attr ) {
            notify_quiet( player, NOPERM_MESSAGE );
            return;
        }
        atr_get_info( thing, atr, &aowner, &aflags );
        if( !Set_attr( player, thing, attr, aflags ) ) {
            notify_quiet( player, NOPERM_MESSAGE );
            return;
        }
        buff = alloc_lbuf( "do_set" );

        /*
         * check for _
         */
        if( *p == '_' ) {
            strcpy( buff, p + 1 );
            if( !parse_attrib( player, p + 1, &thing2, &atr2, 0 ) ||
                    ( atr2 == NOTHING ) ) {
                notify_quiet( player, "No match." );
                free_lbuf( buff );
                return;
            }
            attr2 = atr_num( atr2 );
            p = buff;
            atr_pget_str( buff, thing2, atr2, &aowner, &aflags,
                          &alen );

            if( !attr2 ||
                    !See_attr( player, thing2, attr2, aowner, aflags ) ) {
                notify_quiet( player, NOPERM_MESSAGE );
                free_lbuf( buff );
                return;
            }
        }
        /*
         * Go set it
         */

        set_attr_internal( player, thing, atr, p, key, NULL, NULL );
        free_lbuf( buff );
        return;
    }
    /*
     * Set or clear a flag
     */

    flag_set( thing, player, flag, key );
}

void do_power( dbref player, dbref cause, int key, char *name, char *flag ) {
    dbref thing;

    if( !flag || !*flag ) {
        notify_quiet( player, "I don't know what you want to set!" );
        return;
    }
    /*
     * find thing
     */

    if( ( thing = match_controlled( player, name ) ) == NOTHING ) {
        return;
    }

    power_set( thing, player, flag, key );
}

void do_setattr( dbref player, dbref cause, int attrnum, char *name, char *attrtext ) {
    dbref thing;

    init_match( player, name, NOTYPE );
    match_everything( MAT_EXIT_PARENTS );
    thing = noisy_match_result();

    if( thing == NOTHING ) {
        return;
    }
    set_attr_internal( player, thing, attrnum, attrtext, 0, NULL, NULL );
}



void do_cpattr( dbref player, dbref cause, int key, char *oldpair, char *newpair[], int nargs ) {
    int i, ca, got = 0;

    dbref oldthing;

    char **newthings, **newattrs, *tp;

    ATTR *oldattr;

    if( !*oldpair || !**newpair || !oldpair || !*newpair ) {
        return;
    }
    if( nargs < 1 ) {
        return;
    }

    /*
     * newpair gets whacked to bits by parse_to(). Do it just once.
     */

    newthings =
        ( char ** ) XCALLOC( nargs, sizeof( char * ), "do_cpattr.dbrefs" );
    newattrs = ( char ** ) XCALLOC( nargs, sizeof( char * ), "do_cpattr.attrs" );
    for( i = 0; i < nargs; i++ ) {
        tp = newpair[i];
        newthings[i] = parse_to( &tp, '/', 1 );
        newattrs[i] = tp;
    }

    olist_push();
    if( parse_attrib_wild( player,
                           ( ( strchr( oldpair, '/' ) == NULL ) ?
                             tmprintf( "me/%s", oldpair ) : oldpair ),
                           &oldthing, 0, 0, 1, 0 ) ) {
        for( ca = olist_first(); ca != NOTHING; ca = olist_next() ) {
            oldattr = atr_num( ca );
            if( oldattr ) {
                got = 1;
                for( i = 0; i < nargs; i++ ) {
                    do_set( player, cause, 0, newthings[i],
                            tmprintf( "%s:_#%d/%s",
                                      ( newattrs[i] ? newattrs[i] :
                                        oldattr->name ), oldthing,
                                      oldattr->name ) );
                }
            }
        }
    }
    if( !got ) {
        notify_quiet( player, "No matching attributes found." );
    }
    olist_pop();
    XFREE( newthings, "do_cpattr.dbrefs" );
    XFREE( newattrs, "do_cpattr.attrs" );
}


void do_mvattr( dbref player, dbref cause, int key, char *what, char *args[], int nargs ) {
    dbref thing, aowner, axowner;

    ATTR *in_attr, *out_attr;

    int i, anum, in_anum, aflags, alen, axflags, no_delete, num_copied;

    char *astr;

    /*
     * Make sure we have something to do.
     */

    if( nargs < 2 ) {
        notify_quiet( player, "Nothing to do." );
        return;
    }
    /*
     * Find and make sure we control the target object.
     */

    thing = match_controlled( player, what );
    if( thing == NOTHING ) {
        return;
    }

    /*
     * Look up the source attribute.  If it either doesn't exist or isn't
     * * * * * readable, use an empty string.
     */

    in_anum = -1;
    astr = alloc_lbuf( "do_mvattr" );
    in_attr = atr_str( args[0] );
    if( in_attr == NULL ) {
        *astr = '\0';
    } else {
        atr_get_str( astr, thing, in_attr->number, &aowner, &aflags,
                     &alen );
        if( !See_attr( player, thing, in_attr, aowner, aflags ) ) {
            *astr = '\0';
        } else {
            in_anum = in_attr->number;
        }
    }

    /*
     * Copy the attribute to each target in turn.
     */

    no_delete = 0;
    num_copied = 0;
    for( i = 1; i < nargs; i++ ) {
        anum = mkattr( args[i] );
        if( anum <= 0 ) {
            notify_quiet( player,
                          tmprintf
                          ( "%s: That's not a good name for an attribute.",
                            args[i] ) );
            continue;
        }
        out_attr = atr_num( anum );
        if( !out_attr ) {
            notify_quiet( player,
                          tmprintf( "%s: Permission denied.", args[i] ) );
        } else if( out_attr->number == in_anum ) {
            no_delete = 1;
        } else {
            atr_get_info( thing, out_attr->number, &axowner,
                          &axflags );
            if( !Set_attr( player, thing, out_attr, axflags ) ) {
                notify_quiet( player,
                              tmprintf( "%s: Permission denied.",
                                        args[i] ) );
            } else {
                atr_add( thing, out_attr->number, astr,
                         Owner( player ), aflags );
                num_copied++;
                if( !Quiet( player ) )
                    notify_quiet( player,
                                  tmprintf( "%s: Set.",
                                            out_attr->name ) );
            }
        }
    }

    /*
     * Remove the source attribute if we can.
     */

    if( num_copied < 1 ) {
        if( in_attr ) {
            notify_quiet( player,
                          tmprintf( "%s: Not copied anywhere. Not cleared.",
                                    in_attr->name ) );
        } else {
            notify_quiet( player,
                          "Not copied anywhere. Non-existent attribute." );
        }
    } else if( ( in_anum > 0 ) && !no_delete ) {
        in_attr = atr_num( in_anum );
        if( in_attr && Set_attr( player, thing, in_attr, aflags ) ) {
            atr_clr( thing, in_attr->number );
            if( !Quiet( player ) )
                notify_quiet( player,
                              tmprintf( "%s: Cleared.", in_attr->name ) );
        } else {
            if( in_attr ) {
                notify_quiet( player,
                              tmprintf
                              ( "%s: Could not remove old attribute.  Permission denied.",
                                in_attr->name ) );
            } else {
                notify_quiet( player,
                              "Could not remove old attribute. Non-existent attribute." );
            }
        }
    }
    free_lbuf( astr );
}

/*
 * ---------------------------------------------------------------------------
 * * parse_attrib, parse_attrib_wild: parse <obj>/<attr> tokens.
 */

int parse_attrib( dbref player, char *str, dbref *thing, int *atr, int ok_structs ) {
    ATTR *attr;

    char *buff;

    dbref aowner;

    int aflags;

    if( !str ) {
        return 0;
    }

    /*
     * Break apart string into obj and attr.  Return on failure
     */

    buff = alloc_lbuf( "parse_attrib" );
    strcpy( buff, str );
    if( !parse_thing_slash( player, buff, &str, thing ) ) {
        free_lbuf( buff );
        return 0;
    }
    /*
     * Get the named attribute from the object if we can
     */

    attr = atr_str( str );
    free_lbuf( buff );
    if( !attr ) {
        *atr = NOTHING;
        return NOTHING;
    } else {
        atr_pget_info( *thing, attr->number, &aowner, &aflags );
        if( !See_attr_all( player, *thing, attr, aowner, aflags,
                           ok_structs ) ) {
            *atr = NOTHING;
        } else {
            *atr = attr->number;
        }
    }
    return 1;
}

static void find_wild_attrs( dbref player, dbref thing, char *str, int check_exclude, int hash_insert, int get_locks, int ok_structs ) {
    ATTR *attr;

    char *as;

    dbref aowner;

    int ca, ok, aflags;

    /*
     * Walk the attribute list of the object
     */

    atr_push();
    for( ca = atr_head( thing, &as ); ca; ca = atr_next( &as ) ) {
        attr = atr_num( ca );

        /*
         * Discard bad attributes and ones we've seen before.
         */

        if( !attr ) {
            continue;
        }

        if( check_exclude &&
                ( ( attr->flags & AF_PRIVATE ) ||
                  nhashfind( ca, &mudstate.parent_htab ) ) ) {
            continue;
        }

        /*
         * If we aren't the top level remember this attr so we * * *
         * exclude * it in any parents.
         */

        atr_get_info( thing, ca, &aowner, &aflags );
        if( check_exclude && ( aflags & AF_PRIVATE ) ) {
            continue;
        }

        if( get_locks )
            ok = Read_attr_all( player, thing, attr, aowner,
                                aflags, ok_structs );
        else
            ok = See_attr_all( player, thing, attr, aowner,
                               aflags, ok_structs );

        /*
         * Enforce locality restriction on descriptions
         */

        if( ok && ( attr->number == A_DESC ) && !mudconf.read_rem_desc &&
                !Examinable( player, thing ) && !nearby( player, thing ) ) {
            ok = 0;
        }

        if( ok && quick_wild( str, ( char * ) attr->name ) ) {
            olist_add( ca );
            if( hash_insert ) {
                nhashadd( ca, ( int * ) attr,
                          &mudstate.parent_htab );
            }
        }
    }
    atr_pop();
}

int parse_attrib_wild( dbref player, char *str, dbref *thing, int check_parents, int get_locks, int df_star, int ok_structs ) {
    char *buff;

    dbref parent;

    int check_exclude, hash_insert, lev;

    if( !str ) {
        return 0;
    }

    buff = alloc_lbuf( "parse_attrib_wild" );
    strcpy( buff, str );

    /*
     * Separate name and attr portions at the first /
     */

    if( !parse_thing_slash( player, buff, &str, thing ) ) {

        /*
         * Not in obj/attr format, return if not defaulting to *
         */

        if( !df_star ) {
            free_lbuf( buff );
            return 0;
        }
        /*
         * Look for the object, return failure if not found
         */

        init_match( player, buff, NOTYPE );
        match_everything( MAT_EXIT_PARENTS );
        *thing = match_result();

        if( !Good_obj( *thing ) ) {
            free_lbuf( buff );
            return 0;
        }
        str = ( char * ) "*";
    }
    /*
     * Check the object (and optionally all parents) for attributes
     */

    if( check_parents ) {
        check_exclude = 0;
        hash_insert = check_parents;
        nhashflush( &mudstate.parent_htab, 0 );
        ITER_PARENTS( *thing, parent, lev ) {
            if( !Good_obj( Parent( parent ) ) ) {
                hash_insert = 0;
            }
            find_wild_attrs( player, parent, str, check_exclude,
                             hash_insert, get_locks, ok_structs );
            check_exclude = 1;
        }
    } else {
        find_wild_attrs( player, *thing, str, 0, 0, get_locks,
                         ok_structs );
    }
    free_lbuf( buff );
    return 1;
}


/*
 * ---------------------------------------------------------------------------
 * edit_string_ansi, do_edit: Modify attributes.
 */

void edit_string_ansi( char *src, char **dst, char **returnstr, char *from, char *to ) {
    edit_string( src, dst, from, to );

    if( mudconf.ansi_colors ) {
        edit_string( src, returnstr, from,
                     tmprintf( "%s%s%s%s", ANSI_HILITE, to, ANSI_NORMAL,
                               ANSI_NORMAL ) );
    } else {
        *returnstr = alloc_lbuf( "edit_string_ansi" );
        strcpy( *returnstr, *dst );
    }
}

void do_edit( dbref player, dbref cause, int key, char *it, char *args[], int nargs ) {
    dbref thing, aowner;

    int attr, got_one, aflags, alen, doit;

    int could_hear;

    char *from, *to, *result, *returnstr, *atext;

    ATTR *ap;

    /*
     * Make sure we have something to do.
     */

    if( ( nargs < 1 ) || !*args[0] ) {
        notify_quiet( player, "Nothing to do." );
        return;
    }
    from = args[0];
    to = ( nargs >= 2 ) ? args[1] : ( char * ) "";

    /*
     * Look for the object and get the attribute (possibly wildcarded)
     */

    olist_push();
    if( !it || !*it || !parse_attrib_wild( player, it, &thing, 0, 0, 0, 0 ) ) {
        notify_quiet( player, "No match." );
        olist_pop();
        return;
    }
    /*
     * Iterate through matching attributes, performing edit
     */

    got_one = 0;
    atext = alloc_lbuf( "do_edit.atext" );
    could_hear = Hearer( thing );

    for( attr = olist_first(); attr != NOTHING; attr = olist_next() ) {
        ap = atr_num( attr );
        if( ap ) {

            /*
             * Get the attr and make sure we can modify it.
             */

            atr_get_str( atext, thing, ap->number,
                         &aowner, &aflags, &alen );
            if( Set_attr( player, thing, ap, aflags ) ) {

                /*
                 * Do the edit and save the result
                 */

                got_one = 1;
                edit_string_ansi( atext, &result, &returnstr,
                                  from, to );
                if( ap->check != NULL ) {
                    doit = ( *ap->check )( 0, player, thing,
                                           ap->number, result );
                } else {
                    doit = 1;
                }
                if( doit ) {
                    atr_add( thing, ap->number, result,
                             Owner( player ), aflags );
                    if( !Quiet( player ) )
                        notify_quiet( player,
                                      tmprintf( "Set - %s: %s",
                                                ap->name, returnstr ) );
                }
                free_lbuf( result );
                free_lbuf( returnstr );
            } else {

                /*
                 * No rights to change the attr
                 */

                notify_quiet( player,
                              tmprintf( "%s: Permission denied.",
                                        ap->name ) );
            }

        }
    }

    /*
     * Clean up
     */

    free_lbuf( atext );
    olist_pop();

    if( !got_one ) {
        notify_quiet( player, "No matching attributes." );
    } else {
        handle_ears( thing, could_hear, Hearer( thing ) );
    }
}

void do_wipe( dbref player, dbref cause, int key, char *it ) {
    dbref thing, aowner;

    int attr, got_one, aflags, alen;

    int could_hear;

    ATTR *ap;

    char *atext;

    olist_push();
    if( !it || !*it || !parse_attrib_wild( player, it, &thing, 0, 0, 1, 1 ) ) {
        notify_quiet( player, "No match." );
        olist_pop();
        return;
    }
    /*
     * Iterate through matching attributes, zapping the writable ones
     */

    got_one = 0;
    atext = alloc_lbuf( "do_wipe.atext" );
    could_hear = Hearer( thing );

    for( attr = olist_first(); attr != NOTHING; attr = olist_next() ) {
        ap = atr_num( attr );
        if( ap ) {

            /*
             * Get the attr and make sure we can modify it.
             */

            atr_get_str( atext, thing, ap->number,
                         &aowner, &aflags, &alen );
            if( Set_attr( player, thing, ap, aflags ) ) {
                atr_clr( thing, ap->number );
                got_one = 1;
            }
        }
    }
    /*
     * Clean up
     */

    free_lbuf( atext );
    olist_pop();

    if( !got_one ) {
        notify_quiet( player, "No matching attributes." );
    } else {
        handle_ears( thing, could_hear, Hearer( thing ) );
        if( !Quiet( player ) ) {
            notify_quiet( player, "Wiped." );
        }
    }
}

void do_trigger( dbref player, dbref cause, int key, char *object, char *argv[], int nargs ) {
    dbref thing;

    int attrib;

    if( !( ( parse_attrib( player, object, &thing, &attrib, 0 )
             && ( attrib != NOTHING ) ) ||
            ( parse_attrib( player, tmprintf( "me/%s", object ),
                            &thing, &attrib, 0 )
              && ( attrib != NOTHING ) ) ) ) {
        notify_quiet( player, "No match." );
        return;
    }
    if( !controls( player, thing ) ) {
        notify_quiet( player, NOPERM_MESSAGE );
        return;
    }
    did_it( player, thing, A_NULL, NULL, A_NULL, NULL,
            attrib, key & TRIG_NOW, argv, nargs, 0 );

    /*
     * XXX be more descriptive as to what was triggered?
     */
    if( !( key & TRIG_QUIET ) && !Quiet( player ) ) {
        notify_quiet( player, "Triggered." );
    }

}

void do_use( dbref player, dbref cause, int key, char *object ) {
    char *df_use, *df_ouse, *temp, *bp;

    dbref thing, aowner;

    int aflags, alen, doit;

    init_match( player, object, NOTYPE );
    match_neighbor();
    match_possession();
    if( Wizard( player ) ) {
        match_absolute();
        match_player();
    }
    match_me();
    match_here();
    thing = noisy_match_result();
    if( thing == NOTHING ) {
        return;
    }

    /*
     * Make sure player can use it
     */

    if( !could_doit( player, thing, A_LUSE ) ) {
        did_it( player, thing, A_UFAIL,
                "You can't figure out how to use that.",
                A_OUFAIL, NULL, A_AUFAIL, 0, ( char ** ) NULL, 0,
                MSG_PRESENCE );
        return;
    }
    temp = alloc_lbuf( "do_use" );
    doit = 0;
    if( *atr_pget_str( temp, thing, A_USE, &aowner, &aflags, &alen ) ) {
        doit = 1;
    } else if( *atr_pget_str( temp, thing, A_OUSE, &aowner, &aflags, &alen ) ) {
        doit = 1;
    } else if( *atr_pget_str( temp, thing, A_AUSE, &aowner, &aflags, &alen ) ) {
        doit = 1;
    }
    free_lbuf( temp );

    if( doit ) {
        df_use = alloc_lbuf( "do_use.use" );
        df_ouse = alloc_lbuf( "do_use.ouse" );
        bp = df_use;
        safe_tmprintf_str( df_use, &bp, "You use %s", Name( thing ) );
        bp = df_ouse;
        safe_tmprintf_str( df_ouse, &bp, "uses %s", Name( thing ) );
        did_it( player, thing, A_USE, df_use, A_OUSE, df_ouse, A_AUSE,
                1, ( char ** ) NULL, 0, MSG_PRESENCE );
        free_lbuf( df_use );
        free_lbuf( df_ouse );
    } else {
        notify_quiet( player, "You can't figure out how to use that." );
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_setvattr: Set a user-named (or possibly a predefined) attribute.
 */

void do_setvattr( dbref player, dbref cause, int key, char *arg1, char *arg2 ) {
    char *s;

    int anum;

    arg1++;			/*
				 * skip the '&'
				 */
    for( s = arg1; *s && !isspace( *s ); s++ );	/*
							 * take to the space
							 */
    if( *s ) {
        *s++ = '\0';
    }	/*
				 * split it
				 */

    anum = mkattr( arg1 );	/*
				 * Get or make attribute
				 */
    if( anum <= 0 ) {
        notify_quiet( player,
                      "That's not a good name for an attribute." );
        return;
    }
    do_setattr( player, cause, anum, s, arg2 );
}
