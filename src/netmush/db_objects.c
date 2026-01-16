/**
 * @file db_objects.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core object management: database growth, names, initialization
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

#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

// ColorState constant
static const ColorState color_normal = {0};

OBJ *db = NULL;         /*!< struct database */
NAME *names = NULL;     /*!< Name buffer */
NAME *purenames = NULL; /*!< Pure Name Buffer */

/**
 * @brief Sanitize a name
 *
 * @param thing Thing to check name
 * @param outbuf Output buffer
 * @param bufc Tracking buffer
 */
void safe_name(dbref thing, char *outbuf, char **bufc)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    time_t save_access_time = 0L;
    char *buff = NULL, *buf = NULL;

    /**
     * Retrieving a name never counts against an object's access time.
     *
     */
    save_access_time = AccessTime(thing);

    if (!purenames[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        buf = ansi_strip_ansi(buff);
        purenames[thing] = XSTRDUP(buf, "purenames[thing]");
        XFREE(buf);
        XFREE(buff);
    }

    if (!names[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        names[thing] = XSTRDUP(buff, "names[thing]");
        s_NameLen(thing, alen);
        XFREE(buff);
    }

    XSAFESTRNCAT(outbuf, bufc, names[thing], NameLen(thing), LBUF_SIZE);
    s_AccessTime(thing, save_access_time);
}

/**
 * @brief Get the name of a thing
 *
 * @param thing     DBref of the thing
 * @return char*
 */
char *Name(dbref thing)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *buff = NULL, *buf = NULL;
    time_t save_access_time = AccessTime(thing);

    if (!purenames[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        buf = ansi_strip_ansi(buff);
        purenames[thing] = XSTRDUP(buf, "purenames[thing]");
        XFREE(buf);
        XFREE(buff);
    }

    if (!names[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        names[thing] = XSTRDUP(buff, "names[thing] ");
        s_NameLen(thing, alen);
        XFREE(buff);
    }

    s_AccessTime(thing, save_access_time);
    return names[thing];
}

/**
 * @brief Get the pure name (no ansi) of the thing
 *
 * @param thing     DBref of the thing
 * @return char*
 */
char *PureName(dbref thing)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *buff = NULL, *buf = NULL;
    time_t save_access_time = AccessTime(thing);

    if (!names[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        names[thing] = XSTRDUP(buff, "names[thing]");
        s_NameLen(thing, alen);
        XFREE(buff);
    }

    if (!purenames[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        buf = ansi_strip_ansi(buff);
        purenames[thing] = XSTRDUP(buf, "purenames[thing]");
        XFREE(buf);
        XFREE(buff);
    }

    s_AccessTime(thing, save_access_time);
    return purenames[thing];
}

/**
 * @brief Set the name of the thing
 *
 * @param thing     DBref of thing
 * @param s         Name
 */
void s_Name(dbref thing, char *s)
{
    int len = 0;
    char *buf = NULL;

    /**
     * Truncate the name if we have to
     *
     */
    if (s)
    {
        len = strlen(s);

        if (len > MBUF_SIZE)
        {
            len = MBUF_SIZE - 1;
            s[len] = '\0';
        }
    }
    else
    {
        len = 0;
    }

    atr_add_raw(thing, A_NAME, (char *)s);
    s_NameLen(thing, len);
    names[thing] = XSTRDUP(s, "names[thing]");
    buf = ansi_strip_ansi(s);
    purenames[thing] = XSTRDUP(buf, "purenames[thing]");
    XFREE(buf);
}

/**
 * @brief Sanitize an exit name
 *
 * @param it    DBref of exit
 * @param buff  Buffer
 * @param bufc  Tracking buffer
 */
void safe_exit_name(dbref it, char *buff, char **bufc)
{
    char *start = *bufc;
    char *cursor = NULL;
    char saved = '\0';
    ColorState *states = NULL;
    char *stripped = NULL;
    ColorState normal = color_normal;
    ColorType color_type = ColorTypeAnsi;

    safe_name(it, buff, bufc);

    cursor = start;
    while (*cursor && (*cursor != EXIT_DELIMITER))
    {
        ++cursor;
    }

    saved = *cursor;
    *cursor = '\0';

    int len = ansi_map_states_colorstate(start, &states, &stripped);
    ColorState final_state = (states && len >= 0) ? states[len] : normal;

    *cursor = saved;
    *bufc = cursor;

    if (memcmp(&final_state, &normal, sizeof(ColorState)) != 0)
    {
        char *seq = ansi_transition_colorstate(final_state, normal, color_type, false);

        if (seq)
        {
            XSAFELBSTR(seq, buff, bufc);
            XFREE(seq);
        }
    }

    if (states)
    {
        XFREE(states);
    }

    if (stripped)
    {
        XFREE(stripped);
    }
}

/**
 * @brief Add a raw attribute to a thing
 *
 * @param thing     DBref of the thing
 * @param s         Attribute
 */
void s_Pass(dbref thing, const char *s)
{
    if (mushstate.standalone)
    {
        log_write_raw(1, "P");
    }

    atr_add_raw(thing, A_PASS, (char *)s);
}

/**
 * @brief Manage user-named attributes.
 *
 * @param player    DBref of player
 * @param cause     DBref of cause
 * @param key       Key
 * @param aname     Attribute name
 * @param value     Value
 */
void do_attribute(dbref player, dbref cause, int key, char *aname, char *value)
{
    int success = 0, negate = 0, f = 0;
    char *buff = NULL, *sp = NULL, *p = NULL, *q = NULL, *tbuf = NULL, *tokst = NULL;
    VATTR *va = NULL;
    ATTR *va2 = NULL;

    /**
     * Look up the user-named attribute we want to play with.
     * Note vattr names have a limited size.
     *
     */
    buff = XMALLOC(SBUF_SIZE, "buff");

    for (p = buff, q = aname; *q && ((p - buff) < (VNAME_SIZE - 1)); p++, q++)
    {
        *p = toupper(*q);
    }

    *p = '\0';

    if (!(ok_attr_name(buff) && (va = vattr_find(buff))))
    {
        notify(player, "No such user-named attribute.");
        XFREE(buff);
        return;
    }

    switch (key)
    {
    case ATTRIB_ACCESS:

        /**
         * Modify access to user-named attribute
         *
         */
        for (sp = value; *sp; sp++)
        {
            *sp = toupper(*sp);
        }

        sp = strtok_r(value, " ", &tokst);
        success = 0;

        while (sp != NULL)
        {
            /**
             * Check for negation
             *
             */
            negate = 0;

            if (*sp == '!')
            {
                negate = 1;
                sp++;
            }

            /**
             * Set or clear the appropriate bit
             *
             */
            f = search_nametab(player, attraccess_nametab, sp);

            if (f > 0)
            {
                success = 1;

                if (negate)
                {
                    va->flags &= ~f;
                }
                else
                {
                    va->flags |= f;
                }

                /**
                 * Set the dirty bit
                 *
                 */
                va->flags |= AF_DIRTY;
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unknown permission: %s.", sp);
            }

            /**
             * Get the next token
             *
             */
            sp = strtok_r(NULL, " ", &tokst);
        }

        if (success && !Quiet(player))
        {
            notify(player, "Attribute access changed.");
        }

        break;

    case ATTRIB_RENAME:
        /**
         * Make sure the new name doesn't already exist
         *
         */
        va2 = atr_str(value);

        if (va2)
        {
            notify(player, "An attribute with that name already exists.");
            XFREE(buff);
            return;
        }

        if (vattr_rename(va->name, value) == NULL)
        {
            notify(player, "Attribute rename failed.");
        }
        else
        {
            notify(player, "Attribute renamed.");
        }

        break;

    case ATTRIB_DELETE:
        /**
         * Remove the attribute
         *
         */
        vattr_delete(buff);
        notify(player, "Attribute deleted.");
        break;

    case ATTRIB_INFO:
        /**
         * Print info, like \@list user_attr does
         *
         */
        if (!(va->flags & AF_DELETED))
        {
            tbuf = XMALLOC(LBUF_SIZE, "tbuf");
            XSPRINTF(tbuf, "%s(%d):", va->name, va->number);
            listset_nametab(player, attraccess_nametab, va->flags, true, tbuf);
            XFREE(tbuf);
        }
        else
        {
            notify(player, "That attribute has been deleted.");
        }

        break;
    }

    XFREE(buff);
    return;
}

/**
 * @brief Directly edit database fields
 *
 * @param player    DBref of plauyer
 * @param cause     DBref of cause
 * @param key       Key
 * @param arg1      Argument 1
 * @param arg2      Argument 2
 */
void do_fixdb(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
    dbref thing = NOTHING, res = NOTHING;
    char *tname = NULL, *buf = NULL;

    init_match(player, arg1, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();

    if (thing == NOTHING)
    {
        return;
    }

    res = NOTHING;

    switch (key)
    {
    case FIXDB_OWNER:
    case FIXDB_LOC:
    case FIXDB_CON:
    case FIXDB_EXITS:
    case FIXDB_NEXT:
        init_match(player, arg2, NOTYPE);
        match_everything(0);
        res = noisy_match_result();
        break;

    case FIXDB_PENNIES:
    {
        char *endptr = NULL;
        long val = 0;

        errno = 0;
        val = strtol(arg2, &endptr, 10);

        if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == arg2 || *endptr != '\0')
        {
            notify_quiet(player, "Invalid pennies value.");
            return;
        }

        res = (int)val;
        break;
    }
    }

    switch (key)
    {
    case FIXDB_OWNER:
        s_Owner(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owner set to #%d", res);
        }

        break;

    case FIXDB_LOC:
        s_Location(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Location set to #%d", res);
        }

        break;

    case FIXDB_CON:
        s_Contents(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Contents set to #%d", res);
        }

        break;

    case FIXDB_EXITS:
        s_Exits(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Exits set to #%d", res);
        }

        break;

    case FIXDB_NEXT:
        s_Next(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Exits set to #%d", res);
        }

        break;

    case FIXDB_PENNIES:
        s_Pennies(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Pennies set to %d", res);
        }

        break;

    case FIXDB_NAME:
        if (Typeof(thing) == TYPE_PLAYER)
        {
            if (!ok_player_name(arg2))
            {
                notify(player, "That's not a good name for a player.");
                return;
            }

            if (lookup_player(NOTHING, arg2, 0) != NOTHING)
            {
                notify(player, "That name is already in use.");
                return;
            }

            tname = log_getname(thing);
            buf = ansi_strip_ansi(arg2);
            log_write(LOG_SECURITY, "SEC", "CNAME", "%s renamed to %s", tname, buf);
            XFREE(buf);
            XFREE(tname);

            if (Suspect(player))
            {
                raw_broadcast(WIZARD, "[Suspect] %s renamed to %s", Name(thing), arg2);
            }

            delete_player_name(thing, Name(thing));
            s_Name(thing, arg2);
            add_player_name(thing, arg2);
        }
        else
        {
            if (!ok_name(arg2))
            {
                notify(player, "Warning: That is not a reasonable name.");
            }

            s_Name(thing, arg2);
        }

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Name set to %s", arg2);
        }

        break;
    }
}
void db_grow(dbref newtop)
{
    int newsize = 0, marksize = 0, delta = 0;
    MARKBYTE *newmarkbuf = NULL;
    OBJ *newdb = NULL;
    NAME *newnames = NULL, *newpurenames = NULL;
    char *cp = NULL;

    if (!mushstate.standalone)
    {
        delta = mushconf.init_size;
    }
    else
    {
        delta = 1000;
    }

    /**
     * Determine what to do based on requested size, current top and
     * size. Make sure we grow in reasonable-size chunks to prevent
     * frequent reallocations of the db array.
     *
     */
    if (newtop <= mushstate.db_top)
    {
        /**
         * If requested size is smaller than the current db size, ignore it
         *
         */
        return;
    }

    /**
     * If requested size is greater than the current db size but smaller
     * than the amount of space we have allocated, raise the db size
     * and initialize the new area.
     *
     */
    if (newtop <= mushstate.db_size)
    {
        for (int i = mushstate.db_top; i < newtop; i++)
        {
            names[i] = NULL;
            purenames[i] = NULL;
        }

        initialize_objects(mushstate.db_top, newtop);
        mushstate.db_top = newtop;
        return;
    }

    /**
     * Grow by a minimum of delta objects
     *
     */
    if (newtop <= mushstate.db_size + delta)
    {
        newsize = mushstate.db_size + delta;
    }
    else
    {
        newsize = newtop;
    }

    /**
     * Enforce minimum database size
     *
     */
    if (newsize < mushstate.min_size)
    {
        newsize = mushstate.min_size + delta;
    }

    /**
     * Grow the name tables
     *
     */
    newnames = (NAME *)XCALLOC(newsize + 1, sizeof(NAME), "newnames");

    if (!newnames)
    {
        log_write_raw(1, "ABORT! db.c, could not allocate space for %d item name cache in db_grow().\n", newsize);
        abort();
    }

    if (names)
    {
        /**
         * An old name cache exists.  Copy it.
         *
         */
        names -= 1;
        XMEMCPY((char *)newnames, (char *)names, (newtop + 1) * sizeof(NAME));
        cp = (char *)names;
        XFREE(cp);
    }
    else
    {
        /**
         * Creating a brand new struct database. Fill in the
         * 'reserved' area in case it gets referenced.
         *
         */
        names = newnames;

        for (int i = 0; i < 1; i++)
        {
            names[i] = NULL;
        }
    }

    names = newnames + 1;
    newnames = NULL;
    newpurenames = (NAME *)XCALLOC(newsize + 1, sizeof(NAME), "newpurenames");

    if (!newpurenames)
    {
        log_write_raw(1, "ABORT! db.c, could not allocate space for %d item name cache in db_grow().\n", newsize);
        abort();
    }

    XMEMSET((char *)newpurenames, 0, (newsize + 1) * sizeof(NAME));

    if (purenames)
    {
        /**
         * An old name cache exists.  Copy it.
         *
         */
        purenames -= 1;
        XMEMCPY((char *)newpurenames, (char *)purenames, (newtop + 1) * sizeof(NAME));
        cp = (char *)purenames;
        XFREE(cp);
    }
    else
    {
        /**
         * Creating a brand new struct database.  Fill in the
         * 'reserved' area in case it gets referenced.
         *
         */
        purenames = newpurenames;

        for (int i = 0; i < 1; i++)
        {
            purenames[i] = NULL;
        }
    }

    purenames = newpurenames + 1;
    newpurenames = NULL;
    /**
     * Grow the db array
     *
     */
    newdb = (OBJ *)XCALLOC(newsize + 1, sizeof(OBJ), "newdb");

    if (!newdb)
    {
        log_write(LOG_ALWAYS, "ALC", "DB", "Could not allocate space for %d item struct database.", newsize);
        abort();
    }

    if (db)
    {
        /**
         * An old struct database exists.  Copy it to the new buffer
         *
         */
        db -= 1;
        XMEMCPY((char *)newdb, (char *)db, (mushstate.db_top + 1) * sizeof(OBJ));
        cp = (char *)db;
        XFREE(cp);
    }
    else
    {
        /**
         * Creating a brand new struct database.  Fill in the
         * 'reserved' area in case it gets referenced.
         *
         */
        db = newdb;

        for (int i = 0; i < 1; i++)
        {
            s_Owner(i, GOD);
            s_Flags(i, (TYPE_GARBAGE | GOING));
            s_Flags2(i, 0);
            s_Flags3(i, 0);
            s_Powers(i, 0);
            s_Powers2(i, 0);
            s_Location(i, NOTHING);
            s_Contents(i, NOTHING);
            s_Exits(i, NOTHING);
            s_Link(i, NOTHING);
            s_Next(i, NOTHING);
            s_Zone(i, NOTHING);
            s_Parent(i, NOTHING);
        }
    }

    db = newdb + 1;
    newdb = NULL;
    /**
     * Go do the rest of the things
     *
     */

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        /**
         * Call all modules
         *
         */
        if (cam__mp->db_grow)
        {
            (*(cam__mp->db_grow))(newsize, newtop);
        }
    }

    for (int i = mushstate.db_top; i < newtop; i++)
    {
        names[i] = NULL;
        purenames[i] = NULL;
    }

    initialize_objects(mushstate.db_top, newtop);
    mushstate.db_top = newtop;
    mushstate.db_size = newsize;
    /**
     * Grow the db mark buffer
     *
     */
    marksize = (newsize + 7) >> 3;
    newmarkbuf = (MARKBYTE *)XMALLOC(marksize, "newmarkbuf");
    XMEMSET((char *)newmarkbuf, 0, marksize);

    if (mushstate.markbits)
    {
        marksize = (newtop + 7) >> 3;
        XMEMCPY((char *)newmarkbuf, (char *)mushstate.markbits, marksize);
        cp = (char *)mushstate.markbits;
        XFREE(cp);
    }

    mushstate.markbits = newmarkbuf;
}

/**
 * @brief Free a DB
 *
 */
void db_free(void)
{
    if (db != NULL)
    {
        db -= 1;
        XFREE((char *)db);
        db = NULL;
    }

    mushstate.db_top = 0;
    mushstate.db_size = 0;
    mushstate.freelist = NOTHING;
}

/**
 * @brief Create a minimal DB
 *
 */
void db_make_minimal(void)
{
    dbref obj = NOTHING;

    db_free();
    db_grow(1);
    s_Name(0, "Limbo");
    s_Flags(0, TYPE_ROOM);
    s_Flags2(0, 0);
    s_Flags3(0, 0);
    s_Powers(0, 0);
    s_Powers2(0, 0);
    s_Location(0, NOTHING);
    s_Exits(0, NOTHING);
    s_Link(0, NOTHING);
    s_Parent(0, NOTHING);
    s_Zone(0, NOTHING);
    s_Pennies(0, 1);
    s_Owner(0, 1);

    /**
     * should be #1
     *
     */
    load_player_names();
    obj = create_player((char *)"Wizard", (char *)"potrzebie", NOTHING, 0, 1);
    s_Flags(obj, Flags(obj) | WIZARD);
    s_Flags2(obj, 0);
    s_Flags3(obj, 0);
    s_Powers(obj, 0);
    s_Powers2(obj, 0);
    s_Pennies(obj, 1000);

    /**
     * Manually link to Limbo, just in case
     */
    s_Location(obj, 0);
    s_Next(obj, NOTHING);
    s_Contents(0, obj);
    s_Link(obj, 0);
}

/**
 * @brief Enforce completely numeric dbrefs
 *
 * @param s         String to check
 * @return dbref
 */
dbref parse_dbref_only(const char *s)
{
    int x;

    for (const char *p = s; *p; p++)
    {
        if (!isdigit(*p))
        {
            return NOTHING;
        }
    }

    char *endptr = NULL;
    long val = 0;

    errno = 0;
    val = strtol(s, &endptr, 10);

    if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == s || *endptr != '\0')
    {
        return NOTHING;
    }

    x = (int)val;
    return ((x >= 0) ? x : NOTHING);
}

/**
 * @brief Parse an object id
 *
 * @param s String
 * @param p Pointer to ':' in string
 * @return dbref
 */
dbref parse_objid(const char *s, const char *p)
{
    dbref it = NOTHING;
    time_t tt = 0L;
    const char *r = NULL;
    char *tbuf = XMALLOC(LBUF_SIZE, "tbuf");
    ;

    /**
     * We're passed two parameters: the start of the string, and the
     * pointer to where the ':' in the string is. If the latter is NULL,
     * go find it.
     *
     */
    if (p == NULL)
    {
        if ((p = strchr(s, ':')) == NULL)
        {
            XFREE(tbuf);
            return parse_dbref_only(s);
        }
    }

    /**
     * ObjID takes the format &lt;dbref&gt;:&lt;timestamp as long int&gt;
     * If we match the dbref but its creation time doesn't match the
     * timestamp, we don't have a match.
     */
    XSTRNCPY(tbuf, s, p - s);
    it = parse_dbref_only(tbuf);

    if (Good_obj(it))
    {
        p++;

        for (r = p; *r; r++)
        {
            if (!isdigit(*r))
            {
                XFREE(tbuf);
                return NOTHING;
            }
        }

        char *endptr = NULL;
        errno = 0;
        tt = (time_t)strtol(p, &endptr, 10);

        if (errno == ERANGE || endptr == p || *endptr != '\0')
        {
            XFREE(tbuf);
            return NOTHING;
        }
        XFREE(tbuf);
        return ((CreateTime(it) == tt) ? it : NOTHING);
    }

    XFREE(tbuf);
    return NOTHING;
}

/**
 * @brief Parse string for dbref
 *
 * @param s String
 * @return dbref
 */
dbref parse_dbref(const char *s)
{
    int x;

    /**
     * Either pure dbrefs or objids are okay
     *
     */
    for (const char *p = s; *p; p++)
    {
        if (!isdigit(*p))
        {
            if (*p == ':')
            {
                return parse_objid(s, p);
            }
            else
            {
                return NOTHING;
            }
        }
    }

    x = (int)strtol(s, (char **)NULL, 10);
    return ((x >= 0) ? x : NOTHING);
}

/**
 * @brief Write string to file, escaping char as needed
 *
 * @param f File
 * @param s String
 */
void putstring(FILE *f, const char *s)
{
    putc('"', f);

    while (s && *s)
    {
        switch (*s)
        {
        case '\n':
            fprintf(f, "\\n");
            break;

        case '\r':
            fprintf(f, "\\r");
            break;

        case '\t':
            fprintf(f, "\\t");
            break;

        case C_ANSI_ESC:
            fprintf(f, "\\e");
            break;
        case '\\':
        case '"':
            fprintf(f, "\\%c", *s);
            break;
        default:
            putc(*s, f);
            break;
        }

        s++;
    }

    fprintf(f, "\"\n");
}

void putref(FILE *f, int d)
{
    fprintf(f, "%d\n", d);
}

void putlong(FILE *f, long l)
{
    fprintf(f, "%ld\n", l);
}

/**
 * @brief Read a trring from file, unescaping char as needed
 *
 * @param f File
 * @param new_strings
 * @return char*
 */
char *getstring(FILE *f, bool new_strings)
{
    char *buf = XMALLOC(LBUF_SIZE, "buf");
    char *p = buf, *ret = NULL;
    int lastc = 0, c = fgetc(f);

    if (!new_strings || (c != '"'))
    {
        ungetc(c, f);
        c = '\0';

        while (true)
        {
            lastc = c;
            c = fgetc(f);

            /**
             * If EOF or null, return
             *
             */
            if (!c || (c == EOF))
            {
                *p = '\0';
                ret = XSTRDUP(buf, "getstring");
                XFREE(buf);
                return ret;
            }

            /**
             * If a newline, return if prior char is not a cr. Otherwise keep on truckin'
             *
             */
            if ((c == '\n') && (lastc != '\r'))
            {
                *p = '\0';
                ret = XSTRDUP(buf, "getstring");
                XFREE(buf);
                return ret;
            }

            XSAFELBCHR(c, buf, &p);
        }
    }
    else
    {
        while (true)
        {
            c = fgetc(f);

            if (c == '"')
            {
                if ((c = fgetc(f)) != '\n')
                {
                    ungetc(c, f);
                }

                *p = '\0';
                ret = XSTRDUP(buf, "getstring");
                XFREE(buf);
                return ret;
            }
            else if (c == '\\')
            {
                c = fgetc(f);

                switch (c)
                {
                case 'n':
                    c = '\n';
                    break;

                case 'r':
                    c = '\r';
                    break;

                case 't':
                    c = '\t';
                    break;

                case 'e':
                    c = C_ANSI_ESC;
                    break;
                }
            }

            if ((c == '\0') || (c == EOF))
            {
                *p = '\0';
                ret = XSTRDUP(buf, "getstring");
                XFREE(buf);
                return ret;
            }

            XSAFELBCHR(c, buf, &p);
        }
    }
}

/**
 * @brief Get dbref from file
 *
 * @param f         File
 * @return dbref
 */
dbref getref(FILE *f)
{
    dbref d = NOTHING;
    char *buf = XMALLOC(SBUF_SIZE, "buf");

    if (fgets(buf, SBUF_SIZE, f) != NULL)
    {
        char *endptr = NULL;
        long val = 0;

        errno = 0;
        val = strtol(buf, &endptr, 10);

        if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == buf || (*endptr != '\0' && *endptr != '\n'))
        {
            XFREE(buf);
            return NOTHING;
        }

        d = (dbref)val;
    }

    XFREE(buf);
    return (d);
}

/**
 * @brief Get long int from file
 *
 * @param f     File
 * @return long
 */
long getlong(FILE *f)
{
    long d = 0;
    char *buf = XMALLOC(SBUF_SIZE, "buf");

    if (fgets(buf, SBUF_SIZE, f) != NULL)
    {
        char *endptr = NULL;
        long val = 0;

        errno = 0;
        val = strtol(buf, &endptr, 10);

        if (errno == ERANGE || endptr == buf || (*endptr != '\0' && *endptr != '\n'))
        {
            XFREE(buf);
            return 0;
        }

        d = val;
    }

    XFREE(buf);
    return (d);
}

/**
 * @brief Initialize database backend (LMDB or GDBM)
 *
 * @param dbfile Database filename
 * @return int 0 on success, non-zero on error
 */
int init_database(char *dbfile)
{
    for (mushstate.db_block_size = 1; mushstate.db_block_size < (LBUF_SIZE * 4); mushstate.db_block_size = mushstate.db_block_size << 1)
    {
        /**
         * Calculate proper database block size
         *
         */
    }

    dddb_setfile(dbfile);
    dddb_init();
    
    /*
     * Log database backend and location
     */
#ifdef USE_LMDB
    log_write(LOG_ALWAYS, "INI", "LOAD", "Using LMDB database: %s.lmdb/", dbfile);
#elif USE_GDBM
    log_write(LOG_ALWAYS, "INI", "LOAD", "Using GDBM database: %s.gdbm", dbfile);
#else
    log_write(LOG_ALWAYS, "INI", "LOAD", "Using flatfile database: %s", dbfile);
#endif
    
    db_free();
    return (0);
}

/**
 * @brief checks back through a zone tree for control
 *
 * @param player
 * @param thing
 * @return bool
 */
bool check_zone(dbref player, dbref thing)
{
    if (mushstate.standalone)
    {
        return false;
    }

    if (!mushconf.have_zones || (Zone(thing) == NOTHING) || isPlayer(thing) || (mushstate.zone_nest_num + 1 == mushconf.zone_nest_lim))
    {
        mushstate.zone_nest_num = 0;
        return false;
    }

    /**
     * We check Control_OK on the thing itself, not on its ZMO
     * that allows us to have things default into a zone without
     * needing to be controlled by that ZMO.
     *
     */
    if (!Control_ok(thing))
    {
        return false;
    }

    mushstate.zone_nest_num++;

    /**
     * If the zone doesn't have a ControlLock, DON'T allow control.
     *
     */
    char *lcontrol = atr_get_raw(Zone(thing), A_LCONTROL);
    bool allowed = lcontrol && could_doit(player, Zone(thing), A_LCONTROL);

    if (lcontrol)
    {
        XFREE(lcontrol);
    }

    if (allowed)
    {
        mushstate.zone_nest_num = 0;
        return true;
    }
    else
    {
        return check_zone(player, Zone(thing));
    }
}

bool check_zone_for_player(dbref player, dbref thing)
{
    if (!Control_ok(Zone(thing)))
    {
        return false;
    }

    mushstate.zone_nest_num++;

    if (!mushconf.have_zones || (Zone(thing) == NOTHING) || (mushstate.zone_nest_num == mushconf.zone_nest_lim) || !(isPlayer(thing)))
    {
        mushstate.zone_nest_num = 0;
        return false;
    }

    char *lcontrol = atr_get_raw(Zone(thing), A_LCONTROL);
    bool allowed = lcontrol && could_doit(player, Zone(thing), A_LCONTROL);

    if (lcontrol)
    {
        XFREE(lcontrol);
    }

    if (allowed)
    {
        mushstate.zone_nest_num = 0;
        return true;
    }
    else
    {
        return check_zone(player, Zone(thing));
    }
}

/**
 * @brief Write a restart db.
 *
 */
void dump_restart_db(void)
{
    FILE *f = NULL;
    DESC *d = NULL;
    int version = 0;
    char *dbf = NULL;

    /**
     * We maintain a version number for the restart database,
     * so we can restart even if the format of the restart db
     * has been changed in the new executable.
     *
     */
    version |= RS_RECORD_PLAYERS;
    version |= RS_NEW_STRINGS;
    version |= RS_COUNT_REBOOTS;
    dbf = XASPRINTF("dbf", "%s/%s.db.RESTART", mushconf.dbhome, mushconf.mush_shortname);
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Restart DB: %s", dbf);
    f = fopen(dbf, "w");
    XFREE(dbf);
    fprintf(f, "+V%d\n", version);
    fprintf(f, "%d\n", (int)sock);
    fprintf(f, "%ld\n", (long)mushstate.start_time);
    fprintf(f, "%d\n", (int)mushstate.reboot_nums);
    putstring(f, mushstate.doing_hdr);
    fprintf(f, "%d\n", (int)mushstate.record_players);
    for (d = descriptor_list; (d); d = (d)->next)
    {
        fprintf(f, "%d\n", d->descriptor);
        fprintf(f, "%d\n", d->flags);
        fprintf(f, "%ld\n", d->connected_at);
        fprintf(f, "%d\n", d->command_count);
        fprintf(f, "%d\n", d->timeout);
        fprintf(f, "%d\n", d->host_info);
        fprintf(f, "%d\n", d->player);
        fprintf(f, "%ld\n", d->last_time);
        putstring(f, d->output_prefix);
        putstring(f, d->output_suffix);
        putstring(f, d->addr);
        putstring(f, d->doing);
        putstring(f, d->username);
    }
    fprintf(f, "0\n");
    fclose(f);
}

/**
 * @brief Load a restart DB
 *
 */
void load_restart_db(void)
{
    DESC *d = NULL;
    DESC *p = NULL;
    struct stat fstatbuf;
    int val = 0, version = 0, new_strings = 0;
    char *temp = NULL, *buf = XMALLOC(SBUF_SIZE, "buf");
    char *dbf = XASPRINTF("dbf", "%s/%s.db.RESTART", mushconf.dbhome, mushconf.mush_shortname);
    FILE *f = fopen(dbf, "r");

    if (!f)
    {
        log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Can't open restart DB %s", dbf);
        mushstate.restarting = 0;
        XFREE(temp);
        XFREE(dbf);
        return;
    }

    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Reading restart DB %s", dbf);

    if (fgets(buf, 3, f) != NULL)
    {
        if (strncmp(buf, "+V", 2))
        {
            log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Invalid restart DB: Version marker not found.");
            abort();
        }
    }
    else
    {
        log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Invalid restart DB: Cannot read.");
        abort();
    }

    version = getref(f);
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Restart DB version %d.", version);
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "RS_NEW_STRINGS: %s", version & RS_NEW_STRINGS ? "true" : "false");
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "RS_COUNT_REBOOTS: %s", version & RS_COUNT_REBOOTS ? "true" : "false");
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "RS_CONCENTRATE: %s", version & RS_CONCENTRATE ? "true" : "false");
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "RS_RECORD_PLAYERS: %s", version & RS_RECORD_PLAYERS ? "true" : "false");

    sock = getref(f);

    if (version & RS_NEW_STRINGS)
    {
        new_strings = 1;
    }

    maxd = sock + 1;
    mushstate.start_time = (time_t)getlong(f);
    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Start time: %ld", mushstate.start_time);

    if (version & RS_COUNT_REBOOTS)
    {
        mushstate.reboot_nums = getref(f) + 1;
        log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Reboot count: %d", mushstate.reboot_nums);
    }

    mushstate.doing_hdr = getstring(f, new_strings);

    if (version & RS_CONCENTRATE)
    {
        (void)getref(f);
    }

    if (version & RS_RECORD_PLAYERS)
    {
        mushstate.record_players = getref(f);
        log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Record Player: %d", mushstate.record_players);
    }

    while ((val = getref(f)) != 0)
    {
        ndescriptors++;
        d = XMALLOC(sizeof(DESC), "d");
        d->descriptor = val;
        d->flags = getref(f);
        d->connected_at = (time_t)getlong(f);
        d->retries_left = mushconf.retry_limit;
        d->command_count = getref(f);
        d->timeout = getref(f);
        d->host_info = getref(f);
        d->player = getref(f);
        d->last_time = (time_t)getlong(f);

        temp = getstring(f, new_strings);
        if (*temp)
        {
            d->output_prefix = temp;
        }
        else
        {
            d->output_prefix = NULL;
        }

        temp = getstring(f, new_strings);
        if (*temp)
        {
            d->output_suffix = temp;
        }
        else
        {
            d->output_suffix = NULL;
            XFREE(temp);
        }

        temp = getstring(f, new_strings);
        if (*temp)
        {
            XSTRNCPY(d->addr, temp, 50);
        }
        XFREE(temp);

        temp = getstring(f, new_strings);
        if (*temp)
        {
            d->doing = sane_doing(temp, "doing");
        }
        XFREE(temp);

        temp = getstring(f, new_strings);
        if (*temp)
        {
            XSTRNCPY(d->username, temp, 10);
        }
        XFREE(temp);

        d->colormap = NULL;

        if (version & RS_CONCENTRATE)
        {
            (void)getref(f);
            (void)getref(f);
        }

        d->output_size = 0;
        d->output_tot = 0;
        d->output_lost = 0;
        d->output_head = NULL;
        d->output_tail = NULL;
        d->input_head = NULL;
        d->input_tail = NULL;
        d->input_size = 0;
        d->input_tot = 0;
        d->input_lost = 0;
        d->raw_input = NULL;
        d->raw_input_at = NULL;
        d->quota = mushconf.cmd_quota_max;
        d->program_data = NULL;
        d->hashnext = NULL;

        /**
         * Note that d->address is NOT INITIALIZED, and it DOES get used later, particularly when checking logout.
         *
         */
        if (descriptor_list)
        {
            for (p = descriptor_list; p->next; p = p->next)
                ;

            d->prev = &p->next;
            p->next = d;
            d->next = NULL;
        }
        else
        {
            d->next = descriptor_list;
            d->prev = &descriptor_list;
            descriptor_list = d;
        }

        if (d->descriptor >= maxd)
        {
            maxd = d->descriptor + 1;
        }

        desc_addhash(d);

        if (isPlayer(d->player))
        {
            s_Flags2(d->player, Flags2(d->player) | CONNECTED);
        }
    }

    /**
     * In case we've had anything bizarre happen...
     *
     */
    for (d = descriptor_list; (d); d = (d)->next)
    {
        if (fstat(d->descriptor, &fstatbuf) < 0)
        {
            log_write(LOG_PROBLEMS, "ERR", "RESTART", "Bad descriptor %d", d->descriptor);
            bsd_conn_shutdown(d, R_SOCKDIED);
        }
    }

    for (d = descriptor_list; (d); d = (d)->next)
        if ((d)->flags & DS_CONNECTED)
        {
            if (!isPlayer(d->player))
            {
                bsd_conn_shutdown(d, R_QUIT);
            }
        }

    log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Restart DB read successfully.");

    fclose(f);
    remove(dbf);
    XFREE(dbf);
    XFREE(buf);
}
