/* match.c - Routines for parsing arguments */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mudconf */
#include "game.h"      /* required by mudconf */
#include "alloc.h"     /* required by mudconf */
#include "flags.h"     /* required by mudconf */
#include "htab.h"      /* required by mudconf */
#include "ltdl.h"      /* required by mudconf */
#include "udb.h"       /* required by mudconf */
#include "udb_defs.h"  /* required by mudconf */
#include "mushconf.h"  /* required by code */
#include "db.h"        /* required by externs */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by code */
#include "match.h"     /* required by code */
#include "attrs.h"     /* required by code */
#include "powers.h"    /* required by code */
#include "stringutil.h" /* required by code */

#define CON_LOCAL 0x01    /* Match is near me */
#define CON_TYPE 0x02     /* Match is of requested type */
#define CON_LOCK 0x04     /* I pass the lock on match */
#define CON_COMPLETE 0x08 /* Name given is the full name */
#define CON_TOKEN 0x10    /* Name is a special token */
#define CON_DBREF 0x20    /* Name is a dbref */

MSTATE md;

void promote_match(dbref what, int confidence)
{
    /*
     * Check for type and locks, if requested
     */
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

    if (Randomize(md.count) == 0)
    {
        md.match = what;
    }

    return;
}

/* ---------------------------------------------------------------------------
 * This function removes repeated spaces from the template to which object
 * names are being matched.  It also removes initial and terminal spaces.
 */

char *munge_space_for_match(const char *name)
{
    static char buffer[LBUF_SIZE]; // XXX Should return a buffer instead of a static pointer
    char *q;
    const char *p;
    p = name;
    q = buffer;

    while (isspace(*p))
    {
        p++; /* remove inital spaces */
    }

    while (*p)
    {
        while (*p && !isspace(*p))
        {
            *q++ = *p++;
        }

        while (*p && isspace(*++p))
            ;

        if (*p)
        {
            *q++ = ' ';
        }
    }

    *q = '\0'; /* remove terminal spaces and terminate
				 * string
				 */
    return (buffer);
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

    if (*md.string == LOOKUP_TOKEN)
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
    buf = XMALLOC(LBUF_SIZE, "buf");

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
        bp = buf;
        SAFE_LTOS(buf, &bp, Owner(md.player), LBUF_SIZE);
        SAFE_LB_CHR('.', buf, &bp);

        for (q = str; *q; q++)
        {
            SAFE_LB_CHR(tolower(*q), buf, &bp);
        }

        *bp = '\0';
    }

    np = (int *)hashfind(buf, &mudstate.nref_htab);

    if (np && Good_obj(*np))
    {
        nref = *np;
    }
    else
    {
        nref = NOTHING;
    }

    XFREE(buf);
    return nref;
}

/* returns nnn if name = #nnn, else NOTHING */

dbref absolute_name(int need_pound)
{
    dbref match;
    char *mname;
    mname = md.string;

    if (need_pound)
    {
        if (*md.string != NUMBER_TOKEN)
        {
            return NOTHING;
        }
        else
        {
            mname++;
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

    if (Good_obj(md.absolute_form) && (md.absolute_form == md.player))
    {
        promote_match(md.player, CON_DBREF | CON_LOCAL);
        return;
    }

    if (!string_compare(md.string, "me"))
    {
        promote_match(md.player, CON_TOKEN | CON_LOCAL);
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
        promote_match(HOME, CON_TOKEN);
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
            else if (!string_compare(md.string, "here"))
            {
                promote_match(loc, CON_TOKEN | CON_LOCAL);
            }
            else if (!string_compare(md.string, (char *)PureName(loc)))
            {
                promote_match(loc, CON_COMPLETE | CON_LOCAL);
            }
        }
    }
}

void match_list(dbref first, int local)
{
    char *namebuf;

    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    DOLIST(first, first)
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

    if (Good_loc(md.player))
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

    result = 0;
    DOLIST(exit, Exits(loc))
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

        if (matches_exit_from_list(md.string, (char *)PureName(exit)))
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

    loc = Location(md.player);

    if (Good_obj(md.player) && Has_location(md.player))
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
        ITER_PARENTS(loc, parent, lev)
        {
            if (match_exit_internal(parent, loc, CON_LOCAL))
            {
                break;
            }
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
        ITER_PARENTS(md.player, parent, lev)
        {
            if (match_exit_internal(parent, md.player, CON_LOCAL))
            {
                break;
            }
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
        (void)match_exit_internal(mudconf.master_room, mudconf.master_room, 0);
}

void match_zone_exit(void)
{
    if (md.confidence >= CON_DBREF)
    {
        return;
    }

    if (Good_obj(md.player) && Has_exits(md.player))
    {
        (void)match_exit_internal(Zone(md.player), Zone(md.player), 0);
    }
}

void match_everything(int key)
{
    /*
     * Try matching me, then here, then absolute, then player FIRST, since
     * this will hit most cases. STOP if we get something, since those are
     * exact matches.
     */
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
        return ((mudconf.no_ambiguous_match) ? (md.match) : AMBIGUOUS);
    }
}

/* use this if you don't care about ambiguity */

dbref last_match_result(void)
{
    return md.match;
}

dbref match_status(dbref player, dbref match)
{
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
    return match_status(md.player, match_result());
}

void save_match_state(MSTATE *m_state)
{
    m_state->confidence = md.confidence;
    m_state->count = md.count;
    m_state->pref_type = md.pref_type;
    m_state->check_keys = md.check_keys;
    m_state->absolute_form = md.absolute_form;
    m_state->match = md.match;
    m_state->player = md.player;
    m_state->string = XMALLOC(LBUF_SIZE, "m_state->string");
    XSTRCPY(m_state->string, md.string);
}

void restore_match_state(MSTATE *m_state)
{
    md.confidence = m_state->confidence;
    md.count = m_state->count;
    md.pref_type = m_state->pref_type;
    md.check_keys = m_state->check_keys;
    md.absolute_form = m_state->absolute_form;
    md.match = m_state->match;
    md.player = m_state->player;
    XSTRCPY(md.string, m_state->string);
    XFREE(m_state->string);
}

void init_match(dbref player, const char *name, int type)
{
    md.confidence = -1;
    md.count = md.check_keys = 0;
    md.pref_type = type;
    md.match = NOTHING;
    md.player = player;
    md.string = munge_space_for_match((const char *)name);
    md.absolute_form = absolute_name(1);
}

void init_match_check_keys(dbref player, const char *name, int type)
{
    init_match(player, name, type);
    md.check_keys = 1;
}
