/**
 * @file flags.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Flag manipulation routines
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

/**
 * @brief Set or clear indicated flag bit, no security checking
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_any(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    /*
     * Never let God drop his own wizbit.
     */
    if (God(target) && reset && (flag == WIZARD) && !(fflags & FLAG_WORD2) && !(fflags & FLAG_WORD3))
    {
        notify(player, "You cannot make God mortal.");
        return 0;
    }

    /*
     * Otherwise we can go do it.
     */

    if (fflags & FLAG_WORD3)
    {
        if (reset)
        {
            s_Flags3(target, Flags3(target) & ~flag);
        }
        else
        {
            s_Flags3(target, Flags3(target) | flag);
        }
    }
    else if (fflags & FLAG_WORD2)
    {
        if (reset)
        {
            s_Flags2(target, Flags2(target) & ~flag);
        }
        else
        {
            s_Flags2(target, Flags2(target) | flag);
        }
    }
    else
    {
        if (reset)
        {
            s_Flags(target, Flags(target) & ~flag);
        }
        else
        {
            s_Flags(target, Flags(target) | flag);
        }
    }

    return 1;
}

/**
 * @brief Only GOD may set or clear the bit
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_god(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!God(player))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Only WIZARDS (or GOD) may set or clear the bit
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_wiz(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Wizard(player) && !God(player))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Only WIZARDS, ROYALTY, (or GOD) may set or clear the bit
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_wizroy(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!WizRoy(player) && !God(player))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Only Wizards can set this on players, but ordinary players can set it
 * on other types of objects.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_restrict_player(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (isPlayer(target) && !Wizard(player) && !God(player))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief You can set this flag on a non-player object, if you yourself have
 * this flag and are a player who owns themselves (i.e., no robots). Only God
 * can set this on a player.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_privileged(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int has_it;

    if (!God(player))
    {
        if (!isPlayer(player) || (player != Owner(player)))
        {
            return 0;
        }

        if (isPlayer(target))
        {
            return 0;
        }

        if (fflags & FLAG_WORD3)
        {
            has_it = (Flags3(player) & flag) ? 1 : 0;
        }
        else if (fflags & FLAG_WORD2)
        {
            has_it = (Flags2(player) & flag) ? 1 : 0;
        }
        else
        {
            has_it = (Flags(player) & flag) ? 1 : 0;
        }

        if (!has_it)
        {
            return 0;
        }
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Only players may set or clear this bit.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_inherit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Inherits(player))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Manipulate the dark bit. Nonwizards may not set on players.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_dark_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!reset && isPlayer(target) && !((target == player) && Can_Hide(player)) && (!Wizard(player) && !God(player)))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Manipulate the going bit. Can only be cleared on objects slated for
 * destruction, by non-god. May only be set by god. Even god can't destroy 
 * nondestroyable objects.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_going_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (Going(target) && reset && (Typeof(target) != TYPE_GARBAGE))
    {
        notify(player, "Your object has been spared from destruction.");
        return (fh_any(target, player, flag, fflags, reset));
    }

    if (!God(player) || !destroyable(target))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Set or clear bits that affect hearing.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_hear_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int could_hear;
    int result;
    could_hear = Hearer(target);
    result = fh_any(target, player, flag, fflags, reset);
    handle_ears(target, could_hear, Hearer(target));
    return result;
}

/**
 * @brief Can set and reset this on everything but players.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_player_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (isPlayer(target))
    {
        return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
}

/**
 * @brief Check power bit to set/reset.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param fflags 
 * @param reset 
 * @return int 
 */
int fh_power_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (flag & WATCHER)
    {
        /*
	 * Wizards can set this on anything. Players with the Watch
	 * power can set this on themselves.
	 */
        if (Wizard(player) || ((Owner(player) == Owner(target)) && Can_Watch(player)))
        {
            return (fh_any(target, player, flag, fflags, reset));
        }
        else
        {
            return 0;
        }
    }

    return 0;
}

/* *INDENT-OFF* */

/**
 * @brief List of all available flags. All flags names MUST be in uppercase!
 */
FLAGENT gen_flags[] = {
    {"ABODE", ABODE, 'A', FLAG_WORD2, 0, fh_any},
    {"BLIND", BLIND, 'B', FLAG_WORD2, 0, fh_wiz},
    {"CHOWN_OK", CHOWN_OK, 'C', FLAG_WORD1, 0, fh_any},
    {"DARK", DARK, 'D', FLAG_WORD1, 0, fh_dark_bit},
    {"FREE", NODEFAULT, 'F', FLAG_WORD3, 0, fh_wiz},
    {"GOING", GOING, 'G', FLAG_WORD1, CA_NO_DECOMP, fh_going_bit},
    {"HAVEN", HAVEN, 'H', FLAG_WORD1, 0, fh_any},
    {"INHERIT", INHERIT, 'I', FLAG_WORD1, 0, fh_inherit},
    {"JUMP_OK", JUMP_OK, 'J', FLAG_WORD1, 0, fh_any},
    {"KEY", KEY, 'K', FLAG_WORD2, 0, fh_any},
    {"LINK_OK", LINK_OK, 'L', FLAG_WORD1, 0, fh_any},
    {"MONITOR", MONITOR, 'M', FLAG_WORD1, 0, fh_hear_bit},
    {"NOSPOOF", NOSPOOF, 'N', FLAG_WORD1, CA_WIZARD, fh_any},
    {"OPAQUE", OPAQUE, 'O', FLAG_WORD1, 0, fh_any},
    {"QUIET", QUIET, 'Q', FLAG_WORD1, 0, fh_any},
    {"STICKY", STICKY, 'S', FLAG_WORD1, 0, fh_any},
    {"TRACE", TRACE, 'T', FLAG_WORD1, 0, fh_any},
    {"UNFINDABLE", UNFINDABLE, 'U', FLAG_WORD2, 0, fh_any},
    {"VISUAL", VISUAL, 'V', FLAG_WORD1, 0, fh_any},
    {"WIZARD", WIZARD, 'W', FLAG_WORD1, 0, fh_god},
    {"ANSI", ANSI, 'X', FLAG_WORD2, 0, fh_any},
    {"PARENT_OK", PARENT_OK, 'Y', FLAG_WORD2, 0, fh_any},
    {"ROYALTY", ROYALTY, 'Z', FLAG_WORD1, 0, fh_wiz},
    {"AUDIBLE", HEARTHRU, 'a', FLAG_WORD1, 0, fh_hear_bit},
    {"BOUNCE", BOUNCE, 'b', FLAG_WORD2, 0, fh_any},
    {"CONNECTED", CONNECTED, 'c', FLAG_WORD2, CA_NO_DECOMP, fh_god},
    {"DESTROY_OK", DESTROY_OK, 'd', FLAG_WORD1, 0, fh_any},
    {"ENTER_OK", ENTER_OK, 'e', FLAG_WORD1, 0, fh_any},
    {"FIXED", FIXED, 'f', FLAG_WORD2, 0, fh_restrict_player},
    {"UNINSPECTED", UNINSPECTED, 'g', FLAG_WORD2, 0, fh_wizroy},
    {"HALTED", HALT, 'h', FLAG_WORD1, 0, fh_any},
    {"IMMORTAL", IMMORTAL, 'i', FLAG_WORD1, 0, fh_wiz},
    {"GAGGED", GAGGED, 'j', FLAG_WORD2, 0, fh_wiz},
    {"CONSTANT", CONSTANT_ATTRS, 'k', FLAG_WORD2, 0, fh_wiz},
    {"LIGHT", LIGHT, 'l', FLAG_WORD2, 0, fh_any},
    {"MYOPIC", MYOPIC, 'm', FLAG_WORD1, 0, fh_any},
    {"AUDITORIUM", AUDITORIUM, 'n', FLAG_WORD2, 0, fh_any},
    {"ZONE", ZONE_PARENT, 'o', FLAG_WORD2, 0, fh_any},
    {"PUPPET", PUPPET, 'p', FLAG_WORD1, 0, fh_hear_bit},
    {"TERSE", TERSE, 'q', FLAG_WORD1, 0, fh_any},
    {"ROBOT", ROBOT, 'r', FLAG_WORD1, 0, fh_player_bit},
    {"SAFE", SAFE, 's', FLAG_WORD1, 0, fh_any},
    {"TRANSPARENT", SEETHRU, 't', FLAG_WORD1, 0, fh_any},
    {"SUSPECT", SUSPECT, 'u', FLAG_WORD2, CA_WIZARD, fh_wiz},
    {"VERBOSE", VERBOSE, 'v', FLAG_WORD1, 0, fh_any},
    {"STAFF", STAFF, 'w', FLAG_WORD2, 0, fh_wiz},
    {"SLAVE", SLAVE, 'x', FLAG_WORD2, CA_WIZARD, fh_wiz},
    {"ORPHAN", ORPHAN, 'y', FLAG_WORD3, 0, fh_any},
    {"CONTROL_OK", CONTROL_OK, 'z', FLAG_WORD2, 0, fh_any},
    {"STOP", STOP_MATCH, '!', FLAG_WORD2, 0, fh_wiz},
    {"COMMANDS", HAS_COMMANDS, '$', FLAG_WORD2, 0, fh_any},
    {"PRESENCE", PRESENCE, '^', FLAG_WORD3, 0, fh_wiz},
    {"NOBLEED", NOBLEED, '-', FLAG_WORD2, 0, fh_any},
    {"VACATION", VACATION, '|', FLAG_WORD2, 0, fh_restrict_player},
    {"HEAD", HEAD_FLAG, '?', FLAG_WORD2, 0, fh_wiz},
    {"WATCHER", WATCHER, '+', FLAG_WORD2, 0, fh_power_bit},
    {"HAS_DAILY", HAS_DAILY, '*', FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god},
    {"HAS_STARTUP", HAS_STARTUP, '=', FLAG_WORD1, CA_GOD | CA_NO_DECOMP, fh_god},
    {"HAS_FORWARDLIST", HAS_FWDLIST, '&', FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god},
    {"HAS_LISTEN", HAS_LISTEN, '@', FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god},
    {"HAS_PROPDIR", HAS_PROPDIR, ',', FLAG_WORD3, CA_GOD | CA_NO_DECOMP, fh_god},
    {"PLAYER_MAILS", PLAYER_MAILS, '`', FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god},
    {"HTML", HTML, '~', FLAG_WORD2, 0, fh_any},
    {"REDIR_OK", REDIR_OK, '>', FLAG_WORD3, 0, fh_any},
    {"HAS_REDIRECT", HAS_REDIRECT, '<', FLAG_WORD3, CA_GOD | CA_NO_DECOMP, fh_god},
    {"HAS_DARKLOCK", HAS_DARKLOCK, '.', FLAG_WORD3, CA_GOD | CA_NO_DECOMP, fh_god},
    {"SPEECHMOD", HAS_SPEECHMOD, '"', FLAG_WORD3, 0, fh_any},
    {"MARKER0", MARK_0, '0', FLAG_WORD3, 0, fh_god},
    {"MARKER1", MARK_1, '1', FLAG_WORD3, 0, fh_god},
    {"MARKER2", MARK_2, '2', FLAG_WORD3, 0, fh_god},
    {"MARKER3", MARK_3, '3', FLAG_WORD3, 0, fh_god},
    {"MARKER4", MARK_4, '4', FLAG_WORD3, 0, fh_god},
    {"MARKER5", MARK_5, '5', FLAG_WORD3, 0, fh_god},
    {"MARKER6", MARK_6, '6', FLAG_WORD3, 0, fh_god},
    {"MARKER7", MARK_7, '7', FLAG_WORD3, 0, fh_god},
    {"MARKER8", MARK_8, '8', FLAG_WORD3, 0, fh_god},
    {"MARKER9", MARK_9, '9', FLAG_WORD3, 0, fh_god},
    {"COLOR256", COLOR256, ':', FLAG_WORD3, 0, fh_any},
    {NULL, 0, ' ', 0, 0, NULL}};

OBJENT object_types[8] = {
    {"ROOM", 'R', CA_PUBLIC, OF_CONTENTS | OF_EXITS | OF_DROPTO | OF_HOME},
    {"THING", ' ', CA_PUBLIC, OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_SIBLINGS},
    {"EXIT", 'E', CA_PUBLIC, OF_SIBLINGS},
    {"PLAYER", 'P', CA_PUBLIC, OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_OWNER | OF_SIBLINGS},
    {"TYPE5", '+', CA_GOD, 0},
    {"GARBAGE", '_', CA_PUBLIC, OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_SIBLINGS},
    {"GARBAGE", '#', CA_GOD, 0}};

/* *INDENT-ON* */

/**
 * @brief Initialize flag hash tables.
 * 
 */
void init_flagtab(void)
{
    FLAGENT *fp;
    hashinit(&mushstate.flags_htab, 100 * mushconf.hash_factor, HT_STR | HT_KEYREF);

    for (fp = gen_flags; fp->flagname; fp++)
    {
        hashadd((char *)fp->flagname, (int *)fp, &mushstate.flags_htab, 0);
    }
}

/**
 * @brief Display available flags.
 * 
 * @param player 
 */
void display_flagtab(dbref player)
{
    char *buf;
    FLAGENT *fp;
    buf = XMALLOC(LBUF_SIZE, "buf");
    XSTRCPY(buf, "Flags:");

    for (fp = gen_flags; fp->flagname; fp++)
    {
        if ((fp->listperm & CA_WIZARD) && !Wizard(player))
        {
            continue;
        }

        if ((fp->listperm & CA_GOD) && !God(player))
        {
            continue;
        }
        XSPRINTFCAT(buf, " %s(%c)", (char *)fp->flagname, fp->flaglett);
    }

    notify(player, buf);
    XFREE(buf);
}

/**
 * @brief Search for a flag.
 * 
 * @param thing 
 * @param flagname 
 * @return FLAGENT* 
 */
FLAGENT *find_flag(dbref thing __attribute__((unused)), char *flagname)
{
    char *cp;

    /*
     * Make sure the flag name is uppercase
     */

    for (cp = flagname; *cp; cp++)
    {
        *cp = toupper(*cp);
    }

    return (FLAGENT *)hashfind(flagname, &mushstate.flags_htab);
}

/**
 * @brief Set or clear a specified flag on an object.
 * 
 * @param target 
 * @param player 
 * @param flag 
 * @param key 
 */
void flag_set(dbref target, dbref player, char *flag, int key)
{
    FLAGENT *fp;
    int negate, result;

    /**
     * Trim spaces, and handle the negation character
     */

    negate = 0;

    while (*flag && isspace(*flag))
    {
        flag++;
    }

    if (*flag == '!')
    {
        negate = 1;
        flag++;
    }

    while (*flag && isspace(*flag))
    {
        flag++;
    }

    /**
     * Make sure a flag name was specified
     */

    if (*flag == '\0')
    {
        if (negate)
        {
            notify(player, "You must specify a flag to clear.");
        }
        else
        {
            notify(player, "You must specify a flag to set.");
        }

        return;
    }

    fp = find_flag(target, flag);

    if (fp == NULL)
    {
        notify(player, "I don't understand that flag.");
        return;
    }

    /**
     * Invoke the flag handler, and print feedback
     */

    result = fp->handler(target, player, fp->flagvalue, fp->flagflag, negate);

    if (!result)
    {
        notify(player, NOPERM_MESSAGE);
    }
    else if (!(key & SET_QUIET) && !Quiet(player))
    {
        notify(player, (negate ? "Cleared." : "Set."));
        s_Modified(target);
    }

    return;
}

/*
 * ---------------------------------------------------------------------------
 * decode_flags: converts a flag set into corresponding letters.
 */

/**
 * @brief Converts a flag set into corresponding letters.
 * 
 * @param player 
 * @param flagset 
 * @return char* 
 */
char *decode_flags(dbref player, FLAGSET flagset)
{
    char *buf, *bp;
    FLAGENT *fp;
    int flagtype;
    FLAG fv;

    buf = XMALLOC(SBUF_SIZE, "bp");
    flagtype = (flagset.word1 & TYPE_MASK);

    if (object_types[flagtype].lett != ' ')
    {
        XSTRCCAT(buf, object_types[flagtype].lett);
    }

    for (fp = gen_flags; fp->flagname; fp++)
    {
        if (fp->flagflag & FLAG_WORD3)
        {
            fv = flagset.word3;
        }
        else if (fp->flagflag & FLAG_WORD2)
        {
            fv = flagset.word2;
        }
        else
        {
            fv = flagset.word1;
        }

        if (fv & fp->flagvalue)
        {
            if ((fp->listperm & CA_WIZARD) && !Wizard(player))
            {
                continue;
            }

            if ((fp->listperm & CA_GOD) && !God(player))
            {
                continue;
            }

            XSTRCCAT(buf, fp->flaglett);
        }
    }

    bp = XSTRDUP(buf, "bp");
    XFREE(buf);
    return bp;
}

/**
 * @brief Converts a thing's flags into corresponding letters.
 * 
 * @param player 
 * @param thing 
 * @return char* 
 */
char *unparse_flags(dbref player, dbref thing)
{
    char *buf, *bp;
    FLAGENT *fp;
    int flagtype;
    FLAG fv, flagword, flag2word, flag3word;

    if (!Good_obj(player) || !Good_obj(thing))
    {
        return XSTRDUP("#-2 ERROR", "buf");
    }

    buf = XMALLOC(SBUF_SIZE, "s");

    flagword = Flags(thing);
    flag2word = Flags2(thing);
    flag3word = Flags3(thing);
    flagtype = (flagword & TYPE_MASK);

    if (object_types[flagtype].lett != ' ')
    {
        XSTRCCAT(buf, object_types[flagtype].lett);
    }

    for (fp = gen_flags; fp->flagname; fp++)
    {
        if (fp->flagflag & FLAG_WORD3)
        {
            fv = flag3word;
        }
        else if (fp->flagflag & FLAG_WORD2)
        {
            fv = flag2word;
        }
        else
        {
            fv = flagword;
        }

        if (fv & fp->flagvalue)
        {
            if ((fp->listperm & CA_WIZARD) && !Wizard(player))
            {
                continue;
            }

            if ((fp->listperm & CA_GOD) && !God(player))
            {
                continue;
            }

            /**
             *  Don't show CONNECT on dark wizards to mortals 
             */

            if ((flagtype == TYPE_PLAYER) && isConnFlag(fp) && Can_Hide(thing) && Hidden(thing) && !See_Hidden(player))
            {
                continue;
            }

            /**
             * Check if this is a marker flag and we're at the beginning of a
             * buffer. If so, we need to insert a separator character first so
             * we don't end up running the dbref number into the flags.
             */

            if (!strlen(buf) && isMarkerFlag(fp))
            {
                XSTRCCAT(buf, mushconf.flag_sep[0]);
            }

            XSTRCCAT(buf, fp->flaglett);
        }
    }

    bp = XSTRDUP(buf, "bp");
    XFREE(buf);

    return bp;
}

/**
 * @brief Does object have flag visible to player?
 * 
 * @param player 
 * @param it 
 * @param flagname 
 * @return int 
 */
int has_flag(dbref player, dbref it, char *flagname)
{
    FLAGENT *fp;
    FLAG fv;
    fp = find_flag(it, flagname);

    if (fp == NULL)
    { /* find_flag() uppercases the string */
        if (!strcmp(flagname, "PLAYER"))
        {
            return isPlayer(it);
        }

        if (!strcmp(flagname, "THING"))
        {
            return isThing(it);
        }

        if (!strcmp(flagname, "ROOM"))
        {
            return isRoom(it);
        }

        if (!strcmp(flagname, "EXIT"))
        {
            return isExit(it);
        }

        return 0;
    }

    if (fp->flagflag & FLAG_WORD3)
    {
        fv = Flags3(it);
    }
    else if (fp->flagflag & FLAG_WORD2)
    {
        fv = Flags2(it);
    }
    else
    {
        fv = Flags(it);
    }

    if (fv & fp->flagvalue)
    {
        if ((fp->listperm & CA_WIZARD) && !Wizard(player))
        {
            return 0;
        }

        if ((fp->listperm & CA_GOD) && !God(player))
        {
            return 0;
        }

        /*
	 * don't show CONNECT on dark wizards to mortals
	 */
        if (isPlayer(it) && isConnFlag(fp) && Can_Hide(it) && Hidden(it) && !See_Hidden(player))
        {
            return 0;
        }

        return 1;
    }

    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * flag_description: Return an mbuf containing the type and flags on thing.
 */

/**
 * @brief Return a buffer containing the type and flags on thing.
 * 
 * @param player 
 * @param target 
 * @return char* 
 */
char *flag_description(dbref player, dbref target)
{
    char *buff, *bp;
    FLAGENT *fp;
    int otype;
    FLAG fv;

    otype = Typeof(target);
    buff = XMALLOC(MBUF_SIZE, "buff");

    /**
     * Store the header strings and object type
     */
    XSPRINTF(buff, "Type: %s Flags:", object_types[otype].name);

    if (object_types[otype].perm == CA_PUBLIC)
    {
        for (fp = gen_flags; fp->flagname; fp++)
        {
            if (fp->flagflag & FLAG_WORD3)
            {
                fv = Flags3(target);
            }
            else if (fp->flagflag & FLAG_WORD2)
            {
                fv = Flags2(target);
            }
            else
            {
                fv = Flags(target);
            }

            if (fv & fp->flagvalue)
            {
                if ((fp->listperm & CA_WIZARD) && !Wizard(player))
                {
                    continue;
                }

                if ((fp->listperm & CA_GOD) && !God(player))
                {
                    continue;
                }

                /**
                 * Don't show CONNECT on dark wizards to mortals
                 */
                 
                if (isPlayer(target) && isConnFlag(fp) && Can_Hide(target) && Hidden(target) && !See_Hidden(player))
                {
                    continue;
                }

                XSPRINTFCAT(buff, " %s", (char *)fp->flagname);
            }
        }
    }

    bp = XSTRDUP(buff, "bp");
    XFREE(buff);
    return bp;
}

/*
 *
 * --------------------------------------------------------------------------- *
 * Return an lbuf containing the name and number of an object
 */

/**
 * @brief Return a buffer containing the name and number of an object
 * 
 * @param target 
 * @return char* 
 */
char *unparse_object_numonly(dbref target)
{
    if (target == NOTHING)
    {
        return XSTRDUP("*NOTHING*", "buf");
    }
    else if (target == HOME)
    {
        return XSTRDUP("*HOME*", "buf");
    }
    else if (target == AMBIGUOUS)
    {
        return XSTRDUP("*VARIABLE*", "buf");
    }
    else if (!Good_obj(target))
    {
        return XASPRINTF("buf", "*ILLEGAL*(#%d)", target);
    }
    else
    {
        return XASPRINTF("buf", "%s(#%d)", Name(target), target);
    }
    return NULL;
}

/*
 * ---------------------------------------------------------------------------
 * Return an lbuf pointing to the object name and possibly the db# and flags
 */

/**
 * @brief Return a buffer pointing to the object name and possibly the db# and flags
 * 
 * @param player 
 * @param target 
 * @param obey_myopic 
 * @return char* 
 */
char *unparse_object(dbref player, dbref target, int obey_myopic)
{
    int exam;

    if (target == NOTHING)
    {
        return XSTRDUP("*NOTHING*", "buf");
    }
    else if (target == HOME)
    {
        return XSTRDUP("*HOME*", "buf");
    }
    else if (target == AMBIGUOUS)
    {
        return XSTRDUP("*VARIABLE*", "buf");
    }
    else if (isGarbage(target))
    {
        char *buf, *fp;
        fp = unparse_flags(player, target);
        buf = XASPRINTF("buf", "*GARBAGE*(#%d%s)", target, fp);
        XFREE(fp);
        return buf;
    }
    else if (!Good_obj(target))
    {
        return XASPRINTF("buf", "*ILLEGAL*(#%d)", target);
    }
    else
    {
        if (obey_myopic)
        {
            exam = MyopicExam(player, target);
        }
        else
        {
            exam = Examinable(player, target);
        }

        if (exam || (Flags(target) & (CHOWN_OK | JUMP_OK | LINK_OK | DESTROY_OK)) || (Flags2(target) & ABODE))
        {
            /* show everything */
            char *buf, *fp;
            fp = unparse_flags(player, target);
            buf = XASPRINTF("buf", "%s(#%d%s)", Name(target), target, fp);
            XFREE(fp);
            return buf;
        }
        else
        {
            /* show only the name. */
            return XSTRDUP(Name(target), "buf");
        }
    }

    return NULL;
}

/**
 * @brief Given a one-character flag abbrev, return the flag pointer.
 * 
 * @param this_letter 
 * @return FLAGENT* 
 */
FLAGENT *letter_to_flag(char this_letter)
{
    FLAGENT *fp;

    for (fp = gen_flags; fp->flagname; fp++)
    {
        if (fp->flaglett == this_letter)
        {
            return fp;
        }
    }

    return NULL;
}

/**
 * @brief Modify who can set a flag.
 * 
 * @param vp 
 * @param str 
 * @param extra 
 * @param player 
 * @param cmd 
 * @return int 
 */
int cf_flag_access(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    char *fstr, *permstr, *tokst;
    FLAGENT *fp;
    fstr = strtok_r(str, " \t=,", &tokst);
    permstr = strtok_r(NULL, " \t=,", &tokst);

    if (!fstr || !*fstr || !permstr || !*permstr)
    {
        return -1;
    }

    if ((fp = find_flag(GOD, fstr)) == NULL)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "No such flag", fstr);
        return -1;
    }

    /*
     * Don't change the handlers on special things.
     */

    if ((fp->handler != fh_any) && (fp->handler != fh_wizroy) && (fp->handler != fh_wiz) && (fp->handler != fh_god) && (fp->handler != fh_restrict_player) && (fp->handler != fh_privileged))
    {
        log_write(LOG_CONFIGMODS, "CFG", "PERM", "Cannot change access for flag: %s", fp->flagname);
        return -1;
    }

    if (!strcmp(permstr, (char *)"any"))
    {
        fp->handler = fh_any;
    }
    else if (!strcmp(permstr, (char *)"royalty"))
    {
        fp->handler = fh_wizroy;
    }
    else if (!strcmp(permstr, (char *)"wizard"))
    {
        fp->handler = fh_wiz;
    }
    else if (!strcmp(permstr, (char *)"god"))
    {
        fp->handler = fh_god;
    }
    else if (!strcmp(permstr, (char *)"restrict_player"))
    {
        fp->handler = fh_restrict_player;
    }
    else if (!strcmp(permstr, (char *)"privileged"))
    {
        fp->handler = fh_privileged;
    }
    else
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Flag access", permstr);
        return -1;
    }

    return 0;
}

/**
 * @brief Modify the name of a user-defined flag.
 * 
 * @param vp 
 * @param str 
 * @param extra 
 * @param player 
 * @param cmd 
 * @return int 
 */
int cf_flag_name(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    char *numstr, *namestr, *tokst;
    FLAGENT *fp;
    int flagnum = -1;
    char *flagstr, *cp;
    numstr = strtok_r(str, " \t=,", &tokst);
    namestr = strtok_r(NULL, " \t=,", &tokst);

    if (numstr && (strlen(numstr) == 1))
    {
        flagnum = (int)strtol(numstr, (char **)NULL, 10);
    }

    if ((flagnum < 0) || (flagnum > 9))
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Not a marker flag", numstr);
        return -1;
    }

    if ((fp = letter_to_flag(*numstr)) == NULL)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Marker flag", numstr);
        return -1;
    }

    /**
     * Our conditions: The flag name MUST start with an underscore. It must not
     * conflict with the name of any existing flag. There is a KNOWN MEMORY 
     * LEAK here -- if you name the flag and rename it later, the old bit of
     * memory for the name won't get freed. This should pretty much never
     * happen, since you're not going to run around arbitrarily giving your
     * flags new names all the time.
     */

    flagstr = XASPRINTF("flagstr", "_%s", namestr);

    if (strlen(flagstr) > 31)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Marker flag name too long: %s", namestr);
        XFREE(flagstr);
    }

    for (cp = flagstr; cp && *cp; cp++)
    {
        if (!isalnum(*cp) && (*cp != '_'))
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Illegal marker flag name: %s", namestr);
            XFREE(flagstr);
            return -1;
        }

        *cp = tolower(*cp);
    }

    if (hashfind(flagstr, &mushstate.flags_htab))
    {
        XFREE(flagstr);
        cf_log(player, "CNF", "SYNTX", cmd, "Marker flag name in use: %s", namestr);
        return -1;
    }

    for (cp = flagstr; cp && *cp; cp++)
    {
        *cp = toupper(*cp);
    }

    fp->flagname = (const char *)flagstr;
    hashadd((char *)fp->flagname, (int *)fp, &mushstate.flags_htab, 0);
    return 0;
}

/**
 * @brief convert a list of flag letters into its bit pattern. Also set the
 * type qualifier if specified and not already set.
 * 
 * @param player 
 * @param flaglist 
 * @param fset 
 * @param p_type 
 * @return int 
 */
int convert_flags(dbref player, char *flaglist, FLAGSET *fset, FLAG *p_type)
{
    int i, handled;
    char *s;
    FLAG flag1mask, flag2mask, flag3mask, type;
    FLAGENT *fp;
    flag1mask = flag2mask = flag3mask = 0;
    type = NOTYPE;

    for (s = flaglist; *s; s++)
    {
        handled = 0;

        /**
         * Check for object type
         */

        for (i = 0; (i <= 7) && !handled; i++)
        {
            if ((object_types[i].lett == *s) && !(((object_types[i].perm & CA_WIZARD) && !Wizard(player)) || ((object_types[i].perm & CA_GOD) && !God(player))))
            {
                if ((type != NOTYPE) && (type != i))
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%c: Conflicting type specifications.", *s);
                    return 0;
                }

                type = i;
                handled = 1;
            }
        }

        /**
         * Check generic flags
         */

        if (handled)
        {
            continue;
        }

        for (fp = gen_flags; (fp->flagname) && !handled; fp++)
        {
            if ((fp->flaglett == *s) && !(((fp->listperm & CA_WIZARD) && !Wizard(player)) || ((fp->listperm & CA_GOD) && !God(player))))
            {
                if (fp->flagflag & FLAG_WORD3)
                {
                    flag3mask |= fp->flagvalue;
                }
                else if (fp->flagflag & FLAG_WORD2)
                {
                    flag2mask |= fp->flagvalue;
                }
                else
                {
                    flag1mask |= fp->flagvalue;
                }

                handled = 1;
            }
        }

        if (!handled)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%c: Flag unknown or not valid for specified object type", *s);
            return 0;
        }
    }

    /**
     * return flags to search for and type
     */

    (*fset).word1 = flag1mask;
    (*fset).word2 = flag2mask;
    (*fset).word3 = flag3mask;
    *p_type = type;
    return 1;
}

/**
 * @brief Produce commands to set flags on target.
 * 
 * @param player 
 * @param thing 
 * @param thingname 
 */
void decompile_flags(dbref player, dbref thing, char *thingname)
{
    FLAG f1, f2, f3;
    FLAGENT *fp;
    char *s;
    /*
     * Report generic flags
     */
    f1 = Flags(thing);
    f2 = Flags2(thing);
    f3 = Flags3(thing);

    for (fp = gen_flags; fp->flagname; fp++)
    {
        /**
         * Skip if we shouldn't decompile this flag
         */

        if (fp->listperm & CA_NO_DECOMP)
        {
            continue;
        }

        /**
         * Skip if this flag is not set
         * 
         */

        if (fp->flagflag & FLAG_WORD3)
        {
            if (!(f3 & fp->flagvalue))
            {
                continue;
            }
        }
        else if (fp->flagflag & FLAG_WORD2)
        {
            if (!(f2 & fp->flagvalue))
            {
                continue;
            }
        }
        else
        {
            if (!(f1 & fp->flagvalue))
            {
                continue;
            }
        }

        /**
         * Skip if we can't see this flag
         * 
         */

        if (!check_access(player, fp->listperm))
        {
            continue;
        }

        /**
         * We made it this far, report this flag
         * 
         */

        s = strip_ansi(thingname);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@set %s=%s", s, fp->flagname);
        XFREE(s);
    }
}
