/**
 * @file mguests.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief multiguest code originally ported from DarkZone
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

typedef int object_flag_type;

dbref create_guest(int num)
{
    dbref player, aowner;
    int found, same_str, aflags;
    char *name = XMALLOC(LBUF_SIZE * 2, "name");
    char *base = XMALLOC(mushconf.max_command_args * 2, "base");
    char *prefixes = XMALLOC(LBUF_SIZE, "prefixes");
    char *suffixes = XMALLOC(LBUF_SIZE, "suffixes");
    char *pp, *sp, *tokp, *toks;
    char *s;

    if (!Wizard(mushconf.guest_nuker) || !Good_obj(mushconf.guest_nuker))
    {
        mushconf.guest_nuker = GOD;
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

    if (mushconf.guest_prefixes && mushconf.guest_suffixes)
    {
        XSTRCPY(prefixes, mushconf.guest_prefixes);

        for (pp = strtok_r(prefixes, " \t", &tokp); pp && !found; pp = strtok_r(NULL, " \t", &tokp))
        {
            XSTRCPY(suffixes, mushconf.guest_suffixes);

            for (sp = strtok_r(suffixes, " \t", &toks); sp && !found; sp = strtok_r(NULL, " \t", &toks))
            {
                XSPRINTF(name, "%s%s", pp, sp);

                if (lookup_player(GOD, name, 0) == NOTHING)
                {
                    found = 1;
                }
            }
        }
    }
    else if (mushconf.guest_prefixes || mushconf.guest_suffixes)
    {
        XSTRCPY(prefixes, (mushconf.guest_prefixes ? mushconf.guest_prefixes : mushconf.guest_suffixes));

        for (pp = strtok_r(prefixes, " \t", &tokp); pp && !found; pp = strtok_r(NULL, " \t", &tokp))
        {
            if (lookup_player(GOD, pp, 0) == NOTHING)
            {
                XSTRCPY(name, pp);
                found = 1;
            }
        }
    }

    XSPRINTF(base, "%s%d", mushconf.guest_basename, num + 1);
    same_str = 1;

    if (!found || (strlen(name) >= (size_t)mushconf.max_command_args))
    {
        XSTRCPY(name, base);
    }
    else if (strcasecmp(name, base))
    {
        if (!badname_check(base) || !ok_player_name(base) || (lookup_player(GOD, base, 0) != NOTHING))
        {
            log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest connect failed in alias check: %s", base);
            XFREE(suffixes);
            XFREE(prefixes);
            XFREE(base);
            XFREE(name);
            return NOTHING;
        }

        same_str = 0;
    }

    /*
     * Make the player.
     */
    player = create_player(name, mushconf.guest_password, mushconf.guest_nuker, 0, 1);

    if (player == NOTHING)
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest connect failed in create_player: %s", name);
        XFREE(suffixes);
        XFREE(prefixes);
        XFREE(base);
        XFREE(name);
        return NOTHING;
    }

    /*
     * Add an alias for the basename.
     */

    if (!same_str)
    {
        atr_pget_info(player, A_ALIAS, &aowner, &aflags);
        atr_add(player, A_ALIAS, base, player, aflags);
        add_player_name(player, base);
    }

    /*
     * Turn the player into a guest.
     */
    s_Guest(player);
    move_object(player, (Good_loc(mushconf.guest_start_room) ? mushconf.guest_start_room : (Good_loc(mushconf.start_room) ? mushconf.start_room : 0)));
    s_Flags(player, (Flags(mushconf.guest_char) & ~TYPE_MASK & ~mushconf.stripped_flags.word1) | TYPE_PLAYER);
    s_Flags2(player, Flags2(mushconf.guest_char) & ~mushconf.stripped_flags.word2);
    s_Flags3(player, Flags3(mushconf.guest_char) & ~mushconf.stripped_flags.word3);
    s_Pennies(player, Pennies(mushconf.guest_char));
    s_Zone(player, Zone(mushconf.guest_char));
    s_Parent(player, Parent(mushconf.guest_char));
    /*
     * Make sure the guest is locked.
     */
    s = XASPRINTF("s", "#%d", player);
    do_lock(player, player, A_LOCK, s, "me");
    do_lock(player, player, A_LENTER, s, "me");
    do_lock(player, player, A_LUSE, s, "me");
    XFREE(s);
    /*
     * Copy all attributes.
     */
    atr_cpy(GOD, player, mushconf.guest_char);

    XFREE(suffixes);
    XFREE(prefixes);
    XFREE(base);
    XFREE(name);
    return player;
}

void destroy_guest(dbref guest)
{
    char *s;

    if (!Wizard(mushconf.guest_nuker) || !Good_obj(mushconf.guest_nuker))
    {
        mushconf.guest_nuker = GOD;
    }

    if (!Guest(guest))
    {
        return;
    }

    s = XASPRINTF("s", "%d", mushconf.guest_nuker);
    atr_add_raw(guest, A_DESTROYER, s);
    XFREE(s);
    destroy_player(guest);
    destroy_obj(mushconf.guest_nuker, guest);
}

char *make_guest(DESC *d)
{
    int i;
    dbref guest;
    char *name = XMALLOC(SBUF_SIZE, "name");
    
    /*
     * Nuke extra guests.
     */

    for (i = 0; i < mushconf.number_guests; i++)
    {
        XSPRINTF(name, "%s%d", mushconf.guest_basename, i + 1);
        guest = lookup_player(GOD, name, 0);

        if ((guest != NOTHING) && !Connected(guest))
        {
            destroy_guest(guest);
        }
    }

    /*
     * Find the first free guest ID.
     */

    for (i = 0; i < mushconf.number_guests; i++)
    {
        XSPRINTF(name, "%s%d", mushconf.guest_basename, i + 1);

        if (lookup_player(GOD, name, 0) == NOTHING)
        {
            break;
        }
    }

    if (i == mushconf.number_guests)
    {
        queue_string(d, NULL, "GAME: All guests are currently in use. Please try again later.\n");
        XFREE(name);
        return NULL;
    }

    if ((guest = create_guest(i)) == NOTHING)
    {
        queue_string(d, NULL, "GAME: Error creating guest ID, please try again later.\n");
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Error creating guest ID. '%s' already exists.\n", name);
        XFREE(name);
        return NULL;
    }

    XFREE(name);
    return Name(guest);
}
