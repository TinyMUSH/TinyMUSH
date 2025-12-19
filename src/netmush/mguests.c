/**
 * @file mguests.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief multiguest code originally ported from DarkZone
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <string.h>

dbref create_guest(int num)
{
    dbref player, aowner;
    int found, is_alias_needed, aflags;
    char *name = XMALLOC(LBUF_SIZE, "name");
    char *base = XMALLOC(LBUF_SIZE, "base");
    char *pp, *sp, *tokp, *toks;
    char *s;

    if (!Wizard(mushconf.guest_nuker) || !Good_obj(mushconf.guest_nuker))
    {
        mushconf.guest_nuker = GOD;
    }

    if (num < 0 || num >= mushconf.number_guests)
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest creation failed: invalid guest slot %d", num);
        XFREE(base);
        XFREE(name);
        return NOTHING;
    }

    if (!mushconf.guest_basename || !mushconf.guest_password)
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest creation failed: missing basename or password config");
        XFREE(base);
        XFREE(name);
        return NOTHING;
    }

    /* Initialize name to empty */
    name[0] = '\0';
    found = 0;

    if (mushconf.guest_prefixes && mushconf.guest_suffixes)
    {
        char *prefixes_copy = XMALLOC(LBUF_SIZE, "prefixes_copy");
        char *suffixes_copy = XMALLOC(LBUF_SIZE, "suffixes_copy");

        XSTRCPY(prefixes_copy, mushconf.guest_prefixes);
        XSTRCPY(suffixes_copy, mushconf.guest_suffixes);

        for (pp = strtok_r(prefixes_copy, " \t", &tokp); pp && !found; pp = strtok_r(NULL, " \t", &tokp))
        {
            /* Reset suffixes copy for inner loop */
            XSTRCPY(suffixes_copy, mushconf.guest_suffixes);

            for (sp = strtok_r(suffixes_copy, " \t", &toks); sp && !found; sp = strtok_r(NULL, " \t", &toks))
            {
                XSPRINTF(name, "%s%s", pp, sp);

                if (badname_check(name) && ok_player_name(name) && lookup_player(GOD, name, 0) == NOTHING)
                {
                    found = 1;
                }
            }
        }

        XFREE(suffixes_copy);
        XFREE(prefixes_copy);
    }
    else if (mushconf.guest_prefixes || mushconf.guest_suffixes)
    {
        char *prefixes_copy = XMALLOC(LBUF_SIZE, "prefixes_copy");
        XSTRCPY(prefixes_copy, (mushconf.guest_prefixes ? mushconf.guest_prefixes : mushconf.guest_suffixes));

        for (pp = strtok_r(prefixes_copy, " \t", &tokp); pp && !found; pp = strtok_r(NULL, " \t", &tokp))
        {
            if (badname_check(pp) && ok_player_name(pp) && lookup_player(GOD, pp, 0) == NOTHING)
            {
                XSTRCPY(name, pp);
                found = 1;
            }
        }

        XFREE(prefixes_copy);
    }

    XSPRINTF(base, "%s%d", mushconf.guest_basename, num + 1);
    is_alias_needed = 0;

    if (!found)
    {
        /* No generated name found, use basename */
        XSTRCPY(name, base);
    }
    else if (strlen(name) >= (size_t)mushconf.max_command_args)
    {
        /* Generated name too long, fall back to basename */
        XSTRCPY(name, base);
    }
    else if (strcasecmp(name, base))
    {
        /* Generated name differs from basename, set it as alias */
        /* badname_check returns 1 if name is OK, 0 if bad */
        if (badname_check(base) == 0 || !ok_player_name(base) || (lookup_player(GOD, base, 0) != NOTHING))
        {
            log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest connect failed in alias check: %s", base);
            XFREE(base);
            XFREE(name);
            return NOTHING;
        }

        is_alias_needed = 1;
    }

    /*
     * Make the player.
     */
    player = create_player(name, mushconf.guest_password, mushconf.guest_nuker, 0, 1);

    if (player == NOTHING)
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest connect failed in create_player: %s", name);
        XFREE(base);
        XFREE(name);
        return NOTHING;
    }

    /*
     * Add an alias for the basename.
     */

    if (is_alias_needed)
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

    if (Good_obj(mushconf.guest_char))
    {
        /* Copy flags from guest_char, but preserve TYPE_PLAYER and override stripped flags */
        s_Flags(player, (Flags(mushconf.guest_char) & ~TYPE_MASK & ~mushconf.stripped_flags.word1) | TYPE_PLAYER);
        s_Flags2(player, Flags2(mushconf.guest_char) & ~mushconf.stripped_flags.word2);
        s_Flags3(player, Flags3(mushconf.guest_char) & ~mushconf.stripped_flags.word3);
        s_Pennies(player, Pennies(mushconf.guest_char));
        s_Zone(player, Zone(mushconf.guest_char));
        s_Parent(player, Parent(mushconf.guest_char));
    }
    /*
     * Make sure the guest is locked.
     */
    s = XASPRINTF("lock_str", "#%d", player);
    if (s)
    {
        do_lock(player, player, A_LOCK, s, "me");
        do_lock(player, player, A_LENTER, s, "me");
        do_lock(player, player, A_LUSE, s, "me");
        XFREE(s);
    }
    else
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "WARN", "Failed to allocate lock string for guest");
    }
    /*
     * Copy all attributes.
     */
    if (Good_obj(mushconf.guest_char))
    {
        atr_cpy(GOD, player, mushconf.guest_char);
    }

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

    s = XASPRINTF("destroyer", "%d", mushconf.guest_nuker);
    if (s)
    {
        atr_add_raw(guest, A_DESTROYER, s);
        XFREE(s);
    }

    destroy_player(guest);
    destroy_obj(mushconf.guest_nuker, guest);
}

char *make_guest(DESC *d)
{
    int i, slot;
    dbref guest;
    char *name = XMALLOC(SBUF_SIZE, "name");

    if (!mushconf.guest_basename)
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Guest creation disabled: missing basename config");
        XFREE(name);
        return NULL;
    }

    if (mushconf.number_guests <= 0)
    {
        queue_string(d, NULL, "GAME: Guest creation is disabled.\n");
        XFREE(name);
        return NULL;
    }

    /*
     * Find the first free guest slot, cleaning up disconnected guests.
     * Stop at the first available slot.
     */
    slot = -1;

    for (i = 0; i < mushconf.number_guests; i++)
    {
        XSPRINTF(name, "%s%d", mushconf.guest_basename, i + 1);
        guest = lookup_player(GOD, name, 0);

        if (guest == NOTHING)
        {
            /* Slot is free */
            slot = i;
            break;
        }
        else if (!Connected(guest))
        {
            /* Guest exists but is disconnected, destroy it */
            destroy_guest(guest);
            /* After destruction, slot is now free */
            slot = i;
            break;
        }
        /* Connected guest, continue searching */
    }

    if (slot == -1)
    {
        queue_string(d, NULL, "GAME: All guests are currently in use. Please try again later.\n");
        XFREE(name);
        return NULL;
    }

    /* Create guest at the found slot */
    guest = create_guest(slot);
    XFREE(name);

    if (guest == NOTHING)
    {
        queue_string(d, NULL, "GAME: Error creating guest ID, please try again later.\n");
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Error creating guest ID at slot %d", slot);
        return NULL;
    }

    if (!Good_obj(guest) || !Name(guest))
    {
        log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "Created guest has invalid name");
        destroy_guest(guest);
        return NULL;
    }

    return Name(guest);
}
