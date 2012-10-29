/* mguests.c - multiguest code originally ported from DarkZone */

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
#include "externs.h"		/* required by interface */
#include "interface.h"		/* required by code */

#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */

extern void do_lock ( dbref, dbref, int, char *, char * );

typedef int object_flag_type;

dbref create_guest ( int num ) {
    dbref player, aowner;

    int found, same_str, aflags;

    char name[LBUF_SIZE * 2];

    char base[PLAYER_NAME_LIMIT * 2];

    char prefixes[LBUF_SIZE], suffixes[LBUF_SIZE], *pp, *sp, *tokp, *toks;

    if ( !Wizard ( mudconf.guest_nuker ) || !Good_obj ( mudconf.guest_nuker ) ) {
        mudconf.guest_nuker = GOD;
    }

    /*
     * If basename only is provided, guests are named <basename><number>.
     * * if guest_prefixes and/or guest_suffixes is provided, names will
     * * be generated using a sequential combination of the two lists,
     * * and the guests created will get an alias of <basename><number>.
     * * If we run out of possible name combinations before we run into
     * * the max number of guests, we start calling guests <basename><number>.
     * * If we generate a name longer than the limit, we also fall back on
     * * the basename scheme.
     */

    found = 0;
    if ( *mudconf.guest_prefixes && *mudconf.guest_suffixes ) {
        strcpy ( prefixes, mudconf.guest_prefixes );
        for ( pp = strtok_r ( prefixes, " \t", &tokp ); pp && !found;
                pp = strtok_r ( NULL, " \t", &tokp ) ) {
            strcpy ( suffixes, mudconf.guest_suffixes );
            for ( sp = strtok_r ( suffixes, " \t", &toks );
                    sp && !found; sp = strtok_r ( NULL, " \t", &toks ) ) {
                sprintf ( name, "%s%s", pp, sp );
                if ( lookup_player ( GOD, name, 0 ) == NOTHING ) {
                    found = 1;
                }
            }
        }
    } else if ( *mudconf.guest_prefixes || *mudconf.guest_suffixes ) {
        strcpy ( prefixes, ( *mudconf.guest_prefixes ?
                             mudconf.guest_prefixes : mudconf.guest_suffixes ) );
        for ( pp = strtok_r ( prefixes, " \t", &tokp ); pp && !found;
                pp = strtok_r ( NULL, " \t", &tokp ) ) {
            if ( lookup_player ( GOD, pp, 0 ) == NOTHING ) {
                strcpy ( name, pp );
                found = 1;
            }
        }
    }

    sprintf ( base, "%s%d", mudconf.guest_basename, num + 1 );
    same_str = 1;
    if ( !found || ( strlen ( name ) >= PLAYER_NAME_LIMIT ) ) {
        strcpy ( name, base );
    } else if ( strcasecmp ( name, base ) ) {
        if ( !badname_check ( base ) || !ok_player_name ( base ) ||
                ( lookup_player ( GOD, base, 0 ) != NOTHING ) ) {
            log_printf2 ( LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest connect failed in alias check: %s", base );
            return NOTHING;
        }
        same_str = 0;
    }

    /*
     * Make the player.
     */

    player = create_player ( name, mudconf.guest_password,
                             mudconf.guest_nuker, 0, 1 );

    if ( player == NOTHING ) {
        log_printf2 ( LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest connect failed in create_player: %s", name );
        return NOTHING;
    }

    /*
     * Add an alias for the basename.
     */

    if ( !same_str ) {
        atr_pget_info ( player, A_ALIAS, &aowner, &aflags );
        atr_add ( player, A_ALIAS, base, player, aflags );
        add_player_name ( player, base );
    }

    /*
     * Turn the player into a guest.
     */

    s_Guest ( player );
    move_object ( player,
                  ( Good_loc ( mudconf.guest_start_room ) ? mudconf.
                    guest_start_room : ( Good_loc ( mudconf.start_room ) ? mudconf.
                                         start_room : 0 ) ) );
    s_Flags ( player,
              ( Flags ( mudconf.guest_char ) & ~TYPE_MASK & ~mudconf.stripped_flags.
                word1 ) | TYPE_PLAYER );
    s_Flags2 ( player,
               Flags2 ( mudconf.guest_char ) & ~mudconf.stripped_flags.word2 );
    s_Flags3 ( player,
               Flags3 ( mudconf.guest_char ) & ~mudconf.stripped_flags.word3 );
    s_Pennies ( player, Pennies ( mudconf.guest_char ) );
    s_Zone ( player, Zone ( mudconf.guest_char ) );
    s_Parent ( player, Parent ( mudconf.guest_char ) );

    /*
     * Make sure the guest is locked.
     */

    do_lock ( player, player, A_LOCK, tmprintf ( "#%d", player ), "me" );
    do_lock ( player, player, A_LENTER, tmprintf ( "#%d", player ), "me" );
    do_lock ( player, player, A_LUSE, tmprintf ( "#%d", player ), "me" );

    /*
     * Copy all attributes.
     */

    atr_cpy ( GOD, player, mudconf.guest_char );
    return player;
}

void destroy_guest ( dbref guest ) {
    if ( !Wizard ( mudconf.guest_nuker ) || !Good_obj ( mudconf.guest_nuker ) ) {
        mudconf.guest_nuker = GOD;
    }

    if ( !Guest ( guest ) ) {
        return;
    }

    atr_add_raw ( guest, A_DESTROYER, tmprintf ( "%d", mudconf.guest_nuker ) );
    destroy_player ( guest );
    destroy_obj ( mudconf.guest_nuker, guest );
}

char *make_guest ( DESC *d ) {
    int i;

    dbref guest;

    char name[SBUF_SIZE];

    /*
     * Nuke extra guests.
     */

    for ( i = 0; i < mudconf.number_guests; i++ ) {
        sprintf ( name, "%s%d", mudconf.guest_basename, i + 1 );
        guest = lookup_player ( GOD, name, 0 );
        if ( ( guest != NOTHING ) && !Connected ( guest ) ) {
            destroy_guest ( guest );
        }
    }

    /*
     * Find the first free guest ID.
     */

    for ( i = 0; i < mudconf.number_guests; i++ ) {
        sprintf ( name, "%s%d", mudconf.guest_basename, i + 1 );
        if ( lookup_player ( GOD, name, 0 ) == NOTHING ) {
            break;
        }
    }

    if ( i == mudconf.number_guests ) {
        queue_string ( d,
                       "GAME: All guests are currently in use. Please try again later.\n" );
        return NULL;
    }

    if ( ( guest = create_guest ( i ) ) == NOTHING ) {
        queue_string ( d, "GAME: Error creating guest ID, please try again later.\n" );
        log_printf2 ( LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Error creating guest ID. '%s' already exists.\n", name );
        return NULL;
    }
    return Name ( guest );
}
