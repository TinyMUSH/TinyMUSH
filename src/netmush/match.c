/**
 * @file match.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Routines for parsing arguments
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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

#include <ctype.h>
#include <limits.h>

MSTATE md;

void init_mstate(void)
{
    md.confidence = 0;
    md.count = 0;
    md.pref_type = 0;
    md.check_keys = 0;
    md.absolute_form = NOTHING;
    md.match = NOTHING;
    md.player = NOTHING;
    md.string = XMALLOC(LBUF_SIZE, "buffer");
    if (!md.string)
    {
        log_write(LOG_ALWAYS, "MAT", "FATAL", "Cannot allocate match buffer");
        exit(1);
    }
    XMEMSET(md.string, 0, LBUF_SIZE);
}

void promote_match(dbref what, int confidence)
{
    /*
     * Check for type and locks, if requested
     */

    if (confidence < 0)
    {
        return;
    }

    if (md.pref_type != NOTYPE)
    {
        if (Good_obj(what) && (Typeof(what) == md.pref_type))
        {
            confidence |= CON_TYPE;
        }
    }

    if (md.check_keys)
    {
        MSTATE save_md;
        save_match_state(&save_md);

        /* Only proceed if state was saved successfully */
        if (save_md.confidence >= 0)
        {
            if (Good_obj(md.player) && Good_obj(what) && could_doit(md.player, what, A_LOCK))
            {
                confidence |= CON_LOCK;
            }

            restore_match_state(&save_md);
        }
    }

    /*
     * If nothing matched, take it
     */

    if (md.count == 0)
    {
        md.match = what;
        md.confidence = confidence;
        md.count = 1;
        return;
    }

    /*
     * If confidence is lower, ignore
     */

    if (confidence < md.confidence)
    {
        return;
    }

    /*
     * If confidence is higher, replace
     */

    if (confidence > md.confidence)
    {
        md.match = what;
        md.confidence = confidence;
        md.count = 1;
        return;
    }

    /*
     * Equal confidence, pick randomly using reservoir sampling
     * This ensures uniform probability (1/count) for each match
     */
    if (md.count < INT_MAX)
    {
        md.count++;
    }

    if (random_range(1, md.count) == 1)
    {
        md.match = what;
    }
}

/* ---------------------------------------------------------------------------
 * This function removes repeated spaces from the template to which object
 * names are being matched.  It also removes initial and terminal spaces.
 */

void munge_space_for_match(const char *name)
{
    char *q;
    const char *p;
    int space_left;

    if (!md.string)
    {
        return;
    }

    if (!name)
    {
        XMEMSET(md.string, 0, LBUF_SIZE);
        return;
    }

    p = name;
    q = md.string;
    space_left = LBUF_SIZE - 1;

    while (*p && isspace((unsigned char)*p))
    {
        p++; /* remove initial spaces */
    }

    while (*p && space_left > 0)
    {
        while (*p && !isspace((unsigned char)*p) && space_left > 0)
        {
            *q++ = *p++;
            space_left--;
        }

        if (!*p || space_left <= 0)
        {
            break;
        }

        while (*p && isspace((unsigned char)*p))
        {
            p++;
        }

        if (*p && space_left > 0)
        {
            *q++ = ' ';
            space_left--;
        }
    }

    /* Always ensure we have space for null terminator */
    if (space_left > 0)
    {
        *q = '\0';
    }
    else if (q > md.string)
    {
        *(q - 1) = '\0'; /* Terminate at last valid position */
    }
}

void match_player(void)
{
    dbref match;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.absolute_form) && isPlayer(md.absolute_form))
    {
        promote_match(md.absolute_form, CON_DBREF);
        return;
    }

    if (md.string && *md.string == LOOKUP_TOKEN)
    {
        match = lookup_player(NOTHING, md.string, 1);

        if (Good_obj(match))
        {
            promote_match(match, CON_TOKEN);
        }
    }
}

/* returns object dbref associated with named reference, else NOTHING */

dbref absolute_nref(char *str)
{
    char *p, *q;
    static char buf[LBUF_SIZE];
    dbref *np, nref;
    /*
     * Global or local reference? Global references are automatically
     * * prepended with an additional underscore. i.e., #__foo_ is a global
     * * reference, and #_foo_ is a local reference.
     * * Our beginning and end underscores have already been stripped, so
     * * we would see only _foo or foo.
     * *
     * * We use a static buffer to avoid repeated allocation/deallocation
     * * which was causing memory fragmentation.
     */

    if (!str)
    {
        return NOTHING;
    }

    XMEMSET(buf, 0, LBUF_SIZE);

    if (*str == '_')
    {
        for (p = buf, q = str; *q && (p - buf < LBUF_SIZE - 2); p++, q++)
        {
            *p = tolower(*q);
        }

        *p = '\0';
    }
    else
    {
        if (!Good_obj(md.player))
        {
            return NOTHING;
        }

        int remaining;
        int len;

        p = buf;
        remaining = LBUF_SIZE;

        /* Add owner dbref */
        len = snprintf(buf, remaining, "%d.", Owner(md.player));
        if (len < 0 || len >= remaining)
        {
            return NOTHING;
        }

        p = buf + len;
        remaining -= len;

        /* Add lowercase string */
        for (q = str; *q && remaining > 1; q++, p++, remaining--)
        {
            *p = tolower(*q);
        }

        *p = '\0';
    }

    np = (dbref *)hashfind(buf, &mushstate.nref_htab);

    if (np && Good_obj(*np))
    {
        nref = *np;
    }
    else
    {
        nref = NOTHING;
    }

    return nref;
}

/* returns nnn if name = #nnn, else NOTHING */

dbref absolute_name(int need_pound)
{
    dbref match;
    char *mname;
    mname = md.string;

    if (!mname)
    {
        return NOTHING;
    }

    if (need_pound)
    {
        if (*mname != NUMBER_TOKEN)
        {
            return NOTHING;
        }

        mname++;

        if (!*mname)
        {
            return NOTHING;
        }

        if ((*mname == '_') && *(mname + 1))
        {
            return absolute_nref(mname + 1);
        }
    }

    if (*mname)
    {
        match = parse_dbref(mname);

        if (Good_obj(match))
        {
            return match;
        }
    }

    return NOTHING;
}

void match_absolute(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.absolute_form))
    {
        promote_match(md.absolute_form, CON_DBREF);
    }
}

void match_numeric(void)
{
    dbref match;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    match = absolute_name(0);

    if (Good_obj(match))
    {
        promote_match(match, CON_DBREF);
    }
}

void match_me(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Good_obj(md.absolute_form) && (md.absolute_form == md.player))
    {
        promote_match(md.player, CON_DBREF | CON_LOCAL);
        return;
    }

    if (md.string && !string_compare(md.string, "me") && Good_obj(md.player))
    {
        promote_match(md.player, CON_TOKEN | CON_LOCAL);
    }
}

void match_home(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (md.string && !string_compare(md.string, "home") && Good_obj(HOME))
    {
        promote_match(HOME, CON_TOKEN);
    }
}

void match_here(void)
{
    dbref loc;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_location(md.player))
    {
        loc = Location(md.player);

        if (!Good_obj(loc))
        {
            return;
        }

        if (loc == md.absolute_form)
        {
            promote_match(loc, CON_DBREF | CON_LOCAL);
            return;
        }

        if (md.string)
        {
            if (!string_compare(md.string, "here"))
            {
                promote_match(loc, CON_TOKEN | CON_LOCAL);
            }
            else
            {
                char *locname = PureName(loc);
                if (locname && !string_compare(md.string, locname))
                {
                    promote_match(loc, CON_COMPLETE | CON_LOCAL);
                }
            }
        }
    }
}

void match_list(dbref first, int local)
{
    char *namebuf;
    int iteration_count = 0;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (first == NOTHING || !md.string)
    {
        return;
    }

    for (; (first != NOTHING) && (Next(first) != first) && (iteration_count < MAX_MATCH_ITERATIONS); first = Next(first), iteration_count++)
    {
        if (first == md.absolute_form)
        {
            promote_match(first, CON_DBREF | local);
            return; /* DBREF is highest confidence, no ambiguity possible */
        }

        /*
         * Warning: PureName() returns a static buffer.
         * Make sure there are no other calls to PureName() in
         * promote_match or its called subroutines that would
         * overwrite this buffer before string_match() uses it.
         */
        namebuf = PureName(first);
        if (!namebuf)
        {
            continue;
        }

        if (!string_compare(namebuf, md.string))
        {
            promote_match(first, CON_COMPLETE | local);
        }
        else if (string_match(namebuf, md.string))
        {
            promote_match(first, local);
        }
    }

    /* Log a warning if we hit the iteration cap, indicating a possible cycle/corruption */
    if (iteration_count >= MAX_MATCH_ITERATIONS)
    {
        log_write(LOG_PROBLEMS, "MAT", "WARN", "match_list reached MAX_MATCH_ITERATIONS at object #%d", first);
    }
}

void match_possession(void)
{
    dbref contents;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_contents(md.player))
    {
        contents = Contents(md.player);
        match_list(contents, CON_LOCAL);
    }
}

void match_neighbor(void)
{
    dbref loc, contents;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_location(md.player))
    {
        loc = Location(md.player);

        if (Good_obj(loc))
        {
            contents = Contents(loc);
            match_list(contents, CON_LOCAL);
        }
    }
}

bool match_exit_internal(dbref loc, dbref baseloc, int local)
{
    if (!Good_obj(loc) || !Has_exits(loc))
    {
        /* No exits available at this location: no match here, continue parents */
        return false;
    }

    if (!Good_obj(baseloc))
    {
        /* Invalid base location context: no match, continue parents */
        return false;
    }

    dbref exit = Exits(loc);
    if (exit == NOTHING || !md.string)
    {
        return false;
    }

    bool result = false;
    int iteration_count = 0;
    char *exitname;
    /* Precompute visibility key since it depends only on loc/baseloc/player */
    int key = 0;
    if (Examinable(md.player, loc))
    {
        key |= VE_LOC_XAM;
    }
    if (Dark(loc))
    {
        key |= VE_LOC_DARK;
    }
    if (Dark(baseloc))
    {
        key |= VE_BASE_DARK;
    }
    for (; (exit != NOTHING) && (Next(exit) != exit) && (iteration_count < MAX_MATCH_ITERATIONS); exit = Next(exit), iteration_count++)
    {
        if (exit == md.absolute_form)
        {
            if (Exit_Visible(exit, md.player, key))
            {
                promote_match(exit, CON_DBREF | local);
                return true;
            }
            else
            {
                /* DBREF exact match found but invisible: stop search to prevent incorrect parent match */
                return false;
            }
        }

        exitname = PureName(exit);
        if (exitname && matches_exit_from_list(md.string, exitname))
        {
            promote_match(exit, CON_COMPLETE | local);
            result = true;
        }
    }

    /* Log a warning if we hit the iteration cap, indicating a possible cycle/corruption */
    if (iteration_count >= MAX_MATCH_ITERATIONS)
    {
        log_write(LOG_PROBLEMS, "MAT", "WARN", "match_exit_internal reached MAX_MATCH_ITERATIONS at location #%d", loc);
    }
    return result;
}

void match_exit(void)
{
    dbref loc;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (!Good_obj(md.player) || !Has_location(md.player))
    {
        return;
    }

    loc = Location(md.player);

    if (!Good_obj(loc))
    {
        return;
    }

    (void)match_exit_internal(loc, loc, CON_LOCAL);
}

void match_exit_with_parents(void)
{
    dbref loc, parent;
    int lev;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_location(md.player))
    {
        loc = Location(md.player);
        if (!Good_obj(loc))
        {
            return;
        }

        /*
         * Contract: match_exit_internal() returns true only when a visible DBREF
         * match is found at the current parent; false means continue searching.
         */
        for (lev = 0, parent = loc; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); lev++)
        {
            if (match_exit_internal(parent, loc, CON_LOCAL))
            {
                /* Found a DBREF match, stop searching */
                break;
            }
            /* Check for circular parent chain before next iteration */
            dbref next_parent = Parent(parent);
            if (!Good_obj(next_parent) || next_parent == parent)
            {
                break;
            }
            parent = next_parent;
        }
    }
}

void match_carried_exit(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_exits(md.player))
    {
        (void)match_exit_internal(md.player, md.player, CON_LOCAL);
    }
}

void match_carried_exit_with_parents(void)
{
    dbref parent;
    int lev;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_exits(md.player))
    {
        /*
         * Contract: match_exit_internal() returns true only when a visible DBREF
         * match is found at the current parent; false means continue searching.
         */
        for (lev = 0, parent = md.player; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); lev++)
        {
            if (match_exit_internal(parent, md.player, CON_LOCAL))
            {
                /* Found a DBREF match, stop searching */
                break;
            }
            /* Check for circular parent chain before next iteration */
            dbref next_parent = Parent(parent);
            if (!Good_obj(next_parent) || next_parent == parent)
            {
                break;
            }
            parent = next_parent;
        }
    }
}

void match_master_exit(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Good_obj(mushconf.master_room) && Has_exits(mushconf.master_room))
    {
        (void)match_exit_internal(mushconf.master_room, mushconf.master_room, 0);
    }
}

void match_zone_exit(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player))
    {
        dbref zone = Zone(md.player);
        if (Good_obj(zone) && Has_exits(zone))
        {
            (void)match_exit_internal(zone, zone, 0);
        }
    }
}

void match_everything(int key)
{
    /*
     * Try matching me, then here, then absolute, then player FIRST, since
     * this will hit most cases. STOP if we get something, since those are
     * exact matches.
     */

    if (key < 0)
    {
        return;
    }

    /* Ensure match state is properly initialized */
    if (!md.string)
    {
        return;
    }

    match_me();
    match_here();
    match_absolute();

    if (key & MAT_NUMERIC)
    {
        match_numeric();
    }

    if (key & MAT_HOME)
    {
        match_home();
    }

    match_player();

    if (md.confidence >= CON_TOKEN)
    {
        return;
    }

    if (!(key & MAT_NO_EXITS))
    {
        if (key & MAT_EXIT_PARENTS)
        {
            match_carried_exit_with_parents();
            match_exit_with_parents();
        }
        else
        {
            match_carried_exit();
            match_exit();
        }
    }

    match_neighbor();
    match_possession();
}

dbref match_result(void)
{
    switch (md.count)
    {
    case 0:
        return NOTHING;

    case 1:
        return md.match;

    default:
        return mushconf.no_ambiguous_match ? md.match : AMBIGUOUS;
    }
}

/* use this if you don't care about ambiguity */

dbref last_match_result(void)
{
    return md.match;
}

dbref match_status(dbref player, dbref match)
{
    if (!Good_obj(player))
    {
        return NOTHING;
    }

    switch (match)
    {
    case NOTHING:
        notify(player, NOMATCH_MESSAGE);
        return NOTHING;

    case AMBIGUOUS:
        notify(player, AMBIGUOUS_MESSAGE);
        return NOTHING;

    case NOPERM:
        notify(player, NOPERM_MESSAGE);
        return NOTHING;
    }

    return match;
}

dbref noisy_match_result(void)
{
    if (!Good_obj(md.player))
    {
        return NOTHING;
    }
    return match_status(md.player, match_result());
}

void save_match_state(MSTATE *m_state)
{
    if (!m_state)
    {
        return;
    }

    /* Always initialize all fields first */
    m_state->confidence = md.confidence;
    m_state->count = md.count;
    m_state->pref_type = md.pref_type;
    m_state->check_keys = md.check_keys;
    m_state->absolute_form = md.absolute_form;
    m_state->match = md.match;
    m_state->player = md.player;

    if (md.string)
    {
        m_state->string = XMALLOC(LBUF_SIZE, "m_state->string");
        if (!m_state->string)
        {
            /* Mark state as invalid on allocation failure */
            m_state->confidence = -1;
            return;
        }
        XSTRCPY(m_state->string, md.string);
    }
    else
    {
        m_state->string = NULL;
    }
}

void restore_match_state(MSTATE *m_state)
{
    if (!m_state)
    {
        return;
    }

    md.confidence = m_state->confidence;
    md.count = m_state->count;
    md.pref_type = m_state->pref_type;
    md.check_keys = m_state->check_keys;
    md.absolute_form = m_state->absolute_form;
    md.match = m_state->match;
    md.player = m_state->player;

    if (m_state->string)
    {
        if (md.string)
        {
            XSTRCPY(md.string, m_state->string);
        }
        XFREE(m_state->string);
        m_state->string = NULL;
    }
    else if (md.string)
    {
        /* Clear md.string if m_state had no string */
        md.string[0] = '\0';
    }
}

void init_match(dbref player, const char *name, int type)
{
    if (!Good_obj(player))
    {
        player = NOTHING;
    }

    if (!name)
    {
        name = "";
    }

    md.confidence = -1;
    md.count = 0;
    md.check_keys = 0;
    md.pref_type = type;
    md.match = NOTHING;
    md.player = player;
    munge_space_for_match(name);
    md.absolute_form = absolute_name(1);
}

void init_match_check_keys(dbref player, const char *name, int type)
{
    init_match(player, name, type);
    if (type != NOTYPE)
    {
        md.check_keys = 1;
    }
}
