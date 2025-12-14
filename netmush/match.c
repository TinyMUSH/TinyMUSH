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
        return;
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

        if (Good_obj(what) && could_doit(md.player, what, A_LOCK))
        {
            confidence |= CON_LOCK;
        }

        restore_match_state(&save_md);
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
     * Equal confidence, pick randomly
     */
    md.count++;

    if (md.count > 1 && random_range(0, (md.count) - 1) == 0)
    {
        md.match = what;
    }

    return;
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

    if (!name)
    {
        XMEMSET(md.string, 0, LBUF_SIZE);
        return;
    }

    p = name;
    q = md.string;
    space_left = LBUF_SIZE - 1;

    XMEMSET(md.string, 0, LBUF_SIZE);

    while (isspace(*p))
    {
        p++; /* remove inital spaces */
    }

    while (*p && space_left > 0)
    {
        while (*p && !isspace(*p) && space_left > 0)
        {
            *q++ = *p++;
            space_left--;
        }

        p++;
        while (*p && isspace(*p))
            p++;
        ;

        if (*p && space_left > 0)
        {
            *q++ = ' ';
            space_left--;
        }
    }

    *q = '\0'; /* remove terminal spaces and terminate string */
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
    char *p, *q, *buf, *bp;
    dbref *np, nref;
    /*
     * Global or local reference? Global references are automatically
     * * prepended with an additional underscore. i.e., #__foo_ is a global
     * * reference, and #_foo_ is a local reference.
     * * Our beginning and end underscores have already been stripped, so
     * * we would see only _foo or foo.
     * *
     * * We are not allowed to nibble our buffer, because we've got a
     * * pointer into the match string. Therefore we must copy it.
     * * If we're matching locally we copy the dbref of the owner first,
     * * which means that we then need to worry about buffer size.
     */

    if (!str)
    {
        return NOTHING;
    }

    buf = XMALLOC(LBUF_SIZE, "buf");
    if (!buf)
    {
        return NOTHING;
    }

    if (*str == '_')
    {
        for (p = buf, q = str; *q; p++, q++)
        {
            *p = tolower(*q);
        }

        *p = '\0';
    }
    else
    {
        if (!Good_obj(md.player))
        {
            XFREE(buf);
            return NOTHING;
        }

        bp = buf;
        SAFE_LTOS(buf, &bp, Owner(md.player), LBUF_SIZE);
        SAFE_LB_CHR('.', buf, &bp);

        for (q = str; *q; q++)
        {
            SAFE_LB_CHR(tolower(*q), buf, &bp);
        }

        *bp = '\0';
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

    if (buf)
        XFREE(buf);
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
        else
        {
            mname++;
        }

        if (!mname)
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

    if (match >= 0 && Good_obj(match))
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

    if (Good_obj(md.absolute_form) && (md.absolute_form == md.player))
    {
        if (Good_obj(md.player))
        {
            promote_match(md.player, CON_DBREF | CON_LOCAL);
        }
        return;
    }

    if (!string_compare(md.string, "me"))
    {
        if (Good_obj(md.player))
        {
            promote_match(md.player, CON_TOKEN | CON_LOCAL);
        }
    }

    return;
}

void match_home(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (!string_compare(md.string, "home"))
    {
        if (Good_obj(HOME))
        {
            promote_match(HOME, CON_TOKEN);
        }
    }

    return;
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

        if (Good_obj(loc))
        {
            if (loc == md.absolute_form)
            {
                promote_match(loc, CON_DBREF | CON_LOCAL);
            }
            else if (md.string && !string_compare(md.string, "here"))
            {
                promote_match(loc, CON_TOKEN | CON_LOCAL);
            }
            else if (md.string && PureName(loc) && !string_compare(md.string, (char *)PureName(loc)))
            {
                promote_match(loc, CON_COMPLETE | CON_LOCAL);
            }
        }
    }
}

void match_list(dbref first, int local)
{
    char *namebuf;
    int iteration_count = 0;
    const int MAX_ITERATIONS = 10000;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    for (; (first != NOTHING) && (Next(first) != first) && (iteration_count < MAX_ITERATIONS); first = Next(first), iteration_count++)
    {
        if (first == md.absolute_form)
        {
            promote_match(first, CON_DBREF | local);
            return;
        }

        /*
         * Warning: make sure there are no other calls to Name() in
         * promote_match or its called subroutines; they
         * would overwrite Name()'s static buffer which is
         * needed by string_match().
         */
        namebuf = (char *)PureName(first);
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
}

void match_possession(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Good_loc(md.player))
    {
        match_list(Contents(md.player), CON_LOCAL);
    }
}

void match_neighbor(void)
{
    dbref loc;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_location(md.player))
    {
        loc = Location(md.player);

        if (Good_obj(loc))
        {
            match_list(Contents(loc), CON_LOCAL);
        }
    }
}

int match_exit_internal(dbref loc, dbref baseloc, int local)
{
    dbref exit;
    int result, key;

    if (!Good_obj(loc) || !Has_exits(loc))
    {
        return 1;
    }

    if (!Good_obj(baseloc))
    {
        return 1;
    }

    result = 0;
    for (exit = Exits(loc); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
    {
        if (exit == md.absolute_form)
        {
            key = 0;

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

            if (Exit_Visible(exit, md.player, key))
            {
                promote_match(exit, CON_DBREF | local);
                return 1;
            }
        }

        if (PureName(exit) && matches_exit_from_list(md.string, (char *)PureName(exit)))
        {
            promote_match(exit, CON_COMPLETE | local);
            result = 1;
        }
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

    if (Good_obj(loc))
    {
        (void)match_exit_internal(loc, loc, CON_LOCAL);
    }
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

        for (lev = 0, parent = loc; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); lev++)
        {
            dbref next_parent = Parent(parent);
            if (!Good_obj(next_parent) || next_parent == parent)
            {
                break;
            }
            if (match_exit_internal(parent, loc, CON_LOCAL))
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
        for (lev = 0, parent = (md.player); (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); lev++)
        {
            dbref next_parent = Parent(parent);
            if (!Good_obj(next_parent) || next_parent == parent)
            {
                break;
            }
            if (match_exit_internal(parent, md.player, CON_LOCAL))
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

    if (Good_obj(md.player) && Has_exits(md.player))
    {
        if (Good_obj(mushconf.master_room))
        {
            (void)match_exit_internal(mushconf.master_room, mushconf.master_room, 0);
        }
    }
}

void match_zone_exit(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_exits(md.player))
    {
        dbref zone = Zone(md.player);
        if (Good_obj(zone))
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
    if (md.count < 0)
    {
        return NOTHING;
    }

    switch (md.count)
    {
    case 0:
        return NOTHING;

    case 1:
        if (Good_obj(md.match))
        {
            return md.match;
        }
        return NOTHING;

    default:
        if (!Good_obj(md.match))
        {
            return NOTHING;
        }
        return ((mushconf.no_ambiguous_match) ? (md.match) : AMBIGUOUS);
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

    if (match < 0)
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

    m_state->confidence = md.confidence;
    m_state->count = md.count;
    m_state->pref_type = md.pref_type;
    m_state->check_keys = md.check_keys;
    m_state->absolute_form = md.absolute_form;
    m_state->match = md.match;
    m_state->player = md.player;
    m_state->string = XMALLOC(LBUF_SIZE, "m_state->string");
    if (!m_state->string)
    {
        return;
    }
    XSTRCPY(m_state->string, md.string);
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
        XSTRCPY(md.string, m_state->string);
        XFREE(m_state->string);
    }
}

void init_match(dbref player, const char *name, int type)
{
    if (!Good_obj(player))
    {
        player = NOTHING;
    }

    md.confidence = -1;
    md.count = md.check_keys = 0;
    md.pref_type = type;
    md.match = NOTHING;
    md.player = player;
    munge_space_for_match((const char *)name);
    md.absolute_form = absolute_name(1);
}

void init_match_check_keys(dbref player, const char *name, int type)
{
    init_match(player, name, type);
    if (md.pref_type != NOTYPE && type != NOTYPE)
    {
        md.check_keys = 1;
    }
}
