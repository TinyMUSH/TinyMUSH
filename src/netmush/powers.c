/**
 * @file powers.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Power manipulation routines
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @note If everybody is thinking alike, then somebody isn’t thinking. - GGSP
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <ctype.h>
#include <string.h>

/*
 * ---------------------------------------------------------------------------
 * * ph_any: set or clear indicated bit, no security checking
 */

int ph_any(dbref target, dbref player __attribute__((unused)), POWER power, int fpowers, int reset)
{
    POWER current;
    POWER newvalue;

    if (fpowers & POWER_EXT)
    {
        /* Memoize read to avoid redundant macro calls */
        current = Powers2(target);
        newvalue = reset ? (current & ~power) : (current | power);
        s_Powers2(target, newvalue);
    }
    else
    {
        /* Memoize read to avoid redundant macro calls */
        current = Powers(target);
        newvalue = reset ? (current & ~power) : (current | power);
        s_Powers(target, newvalue);
    }

    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * ph_god: only GOD may set or clear the bit
 */

int ph_god(dbref target, dbref player, POWER power, int fpowers, int reset)
{
    if (!God(player))
    {
        return 0;
    }

    return (ph_any(target, player, power, fpowers, reset));
}

/*
 * ---------------------------------------------------------------------------
 * * ph_wiz: only WIZARDS (or GOD) may set or clear the bit
 */

int ph_wiz(dbref target, dbref player, POWER power, int fpowers, int reset)
{
    if (!Wizard(player) && !God(player))
    {
        return 0;
    }

    return (ph_any(target, player, power, fpowers, reset));
}

/*
 * ---------------------------------------------------------------------------
 * * ph_wizroy: only WIZARDS, ROYALTY, (or GOD) may set or clear the bit
 */

int ph_wizroy(dbref target, dbref player, POWER power, int fpowers, int reset)
{
    if (!WizRoy(player) && !God(player))
    {
        return 0;
    }

    return (ph_any(target, player, power, fpowers, reset));
}

/* ---------------------------------------------------------------------------
 * ph_restrict_player: Only Wizards can set this on players, but
 * ordinary players can set it on other types of objects.
 */

int ph_restrict_player(dbref target, dbref player, POWER power, int fpowers, int reset)
{
    if (isPlayer(target) && !Wizard(player) && !God(player))
    {
        return 0;
    }

    return (ph_any(target, player, power, fpowers, reset));
}

/* ---------------------------------------------------------------------------
 * ph_privileged: You can set this power on a non-player object, if you
 * yourself have this power and are a player who owns themselves (i.e.,
 * no robots). Only God can set this on a player.
 */

int ph_privileged(dbref target, dbref player, POWER power, int fpowers, int reset)
{
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

        /* Memoize power read to avoid redundant macro calls in both branches */
        POWER player_power = (fpowers & POWER_EXT) ? Powers2(player) : Powers(player);

        if (player_power & power)
        {
            return (ph_any(target, player, power, fpowers, reset));
        }
        return 0;
    }

    return (ph_any(target, player, power, fpowers, reset));
}

/*
 * ---------------------------------------------------------------------------
 * * ph_inherit: only players may set or clear this bit.
 */

int ph_inherit(dbref target, dbref player, POWER power, int fpowers, int reset)
{
    if (!Inherits(player))
    {
        return 0;
    }

    return (ph_any(target, player, power, fpowers, reset));
}
/* *INDENT-OFF* */

/* All power names must be in lowercase!
 * Array is terminated by an entry with NULL powername for safe iteration.
 */

POWERENT gen_powers[] = {
    {(char *)"announce", POW_ANNOUNCE, 0, 0, ph_wiz},
    {(char *)"attr_read", POW_MDARK_ATTR, 0, 0, ph_wiz},
    {(char *)"attr_write", POW_WIZ_ATTR, 0, 0, ph_wiz},
    {(char *)"boot", POW_BOOT, 0, 0, ph_wiz},
    {(char *)"builder", POW_BUILDER, POWER_EXT, 0, ph_wiz},
    {(char *)"chown_anything", POW_CHOWN_ANY, 0, 0, ph_wiz},
    {(char *)"cloak", POW_CLOAK, POWER_EXT, 0, ph_god},
    {(char *)"comm_all", POW_COMM_ALL, 0, 0, ph_wiz},
    {(char *)"control_all", POW_CONTROL_ALL, 0, 0, ph_god},
    {(char *)"expanded_who", POW_WIZARD_WHO, 0, 0, ph_wiz},
    {(char *)"find_unfindable", POW_FIND_UNFIND, 0, 0, ph_wiz},
    {(char *)"free_money", POW_FREE_MONEY, 0, 0, ph_wiz},
    {(char *)"free_quota", POW_FREE_QUOTA, 0, 0, ph_wiz},
    {(char *)"guest", POW_GUEST, 0, 0, ph_god},
    {(char *)"halt", POW_HALT, 0, 0, ph_wiz},
    {(char *)"hide", POW_HIDE, 0, 0, ph_wiz},
    {(char *)"idle", POW_IDLE, 0, 0, ph_wiz},
    {(char *)"link_any_home", POW_LINKHOME, POWER_EXT, 0, ph_wiz},
    {(char *)"link_to_anything", POW_LINKTOANY, POWER_EXT, 0, ph_wiz},
    {(char *)"link_variable", POW_LINKVAR, POWER_EXT, 0, ph_wiz},
    {(char *)"long_fingers", POW_LONGFINGERS, 0, 0, ph_wiz},
    {(char *)"no_destroy", POW_NO_DESTROY, 0, 0, ph_wiz},
    {(char *)"open_anywhere", POW_OPENANYLOC, POWER_EXT, 0, ph_wiz},
    {(char *)"pass_locks", POW_PASS_LOCKS, 0, 0, ph_wiz},
    {(char *)"poll", POW_POLL, 0, 0, ph_wiz},
    {(char *)"prog", POW_PROG, 0, 0, ph_wiz},
    {(char *)"quota", POW_CHG_QUOTAS, 0, 0, ph_wiz},
    {(char *)"search", POW_SEARCH, 0, 0, ph_wiz},
    {(char *)"see_all", POW_EXAM_ALL, 0, 0, ph_wiz},
    {(char *)"see_queue", POW_SEE_QUEUE, 0, 0, ph_wiz},
    {(char *)"see_hidden", POW_SEE_HIDDEN, 0, 0, ph_wiz},
    {(char *)"stat_any", POW_STAT_ANY, 0, 0, ph_wiz},
    {(char *)"steal_money", POW_STEAL, 0, 0, ph_wiz},
    {(char *)"tel_anywhere", POW_TEL_ANYWHR, 0, 0, ph_wiz},
    {(char *)"tel_anything", POW_TEL_UNRST, 0, 0, ph_wiz},
    {(char *)"unkillable", POW_UNKILLABLE, 0, 0, ph_wiz},
    {(char *)"use_module", POW_USE_MODULE, POWER_EXT, 0, ph_god},
    {(char *)"watch_logins", POW_WATCH, 0, 0, ph_wiz},
    {NULL, 0, POWER_EXT, 0, 0}};

/* *INDENT-ON* */

/*
 * ---------------------------------------------------------------------------
 * * init_powertab: initialize power hash tables.
 */

void init_powertab(void)
{
    POWERENT *fp;
    hashinit(&mushstate.powers_htab, 25 * mushconf.hash_factor, HT_STR | HT_KEYREF);

    for (fp = gen_powers; fp->powername; fp++)
    {
        hashadd((char *)fp->powername, (int *)fp, &mushstate.powers_htab, 0);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * display_powers: display available powers.
 */

void display_powertab(dbref player)
{
    char *buf, *bp;
    POWERENT *fp;
    int is_wizard, is_god;

    bp = buf = XMALLOC(LBUF_SIZE, "buf");
    if (!buf)
    {
        return;
    }
    XSAFELBSTR((char *)"Powers:", buf, &bp);

    /* Memoize permission checks to avoid repeated macro calls in loop */
    is_wizard = Wizard(player);
    is_god = God(player);

    for (fp = gen_powers; fp->powername; fp++)
    {
        if ((fp->listperm & CA_WIZARD) && !is_wizard)
        {
            continue;
        }

        if ((fp->listperm & CA_GOD) && !is_god)
        {
            continue;
        }

        XSAFELBCHR(' ', buf, &bp);
        XSAFELBSTR((char *)fp->powername, buf, &bp);
    }

    *bp = '\0';
    notify(player, buf);
    XFREE(buf);
}

POWERENT *find_power(dbref thing __attribute__((unused)), const char *powername)
{
    /* Build a lowercase, trimmed token without modifying the input.
     * The 'thing' parameter is unused; this function performs a global lookup
     * regardless of the object passed, consistent with the hash table design.
     */
    char namebuf[SBUF_SIZE];
    size_t i = 0;

    if (!powername)
    {
        return NULL;
    }

    /* Skip leading whitespace */
    while (*powername && isspace((unsigned char)*powername))
    {
        powername++;
    }

    /* Tolerate a leading negation/plus sign if passed inadvertently */
    if (*powername == '!' || *powername == '+')
    {
        powername++;
        while (*powername && isspace((unsigned char)*powername))
        {
            powername++;
        }
    }

    /* Copy until delimiter or NUL, guarding against overflow */
    while (*powername)
    {
        unsigned char ch = (unsigned char)*powername;
        if (isspace(ch) || ch == '=' || ch == ',')
        {
            break;
        }
        if (i >= (sizeof(namebuf) - 1))
        {
            /* Name too long for our static buffer */
            return NULL;
        }
        namebuf[i++] = (char)tolower(ch);
        powername++;
    }
    namebuf[i] = '\0';

    if (i == 0)
    {
        return NULL;
    }

    return (POWERENT *)hashfind(namebuf, &mushstate.powers_htab);
}

int decode_power(dbref player, char *powername, POWERSET *pset)
{
    POWERENT *pent;
    if (!pset)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s", "Internal error: NULL POWERSET");
        return 0;
    }

    pset->word1 = 0;
    pset->word2 = 0;

    if (!powername || !*powername)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s", "Power not specified.");
        return 0;
    }
    /* Use find_power to ensure case-insensitive lookup without mutating input.
     * find_power handles trimming and case normalization internally.
     */
    pent = find_power(GOD, powername);

    if (!pent)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: Power not found.", powername);
        return 0;
    }

    if (pent->powerpower & POWER_EXT)
    {
        pset->word2 = pent->powervalue;
    }
    else
    {
        pset->word1 = pent->powervalue;
    }

    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * power_set: Set or clear a specified power on an object.
 */

void power_set(dbref target, dbref player, char *power, int key)
{
    POWERENT *fp;
    int negate, result;
    /*
     * Trim spaces, and handle the negation character.
     * Early exit if the input is empty or whitespace-only.
     */
    negate = 0;

    while (*power && isspace((unsigned char)*power))
    {
        power++;
    }

    if (*power == '!')
    {
        negate = 1;
        power++;
    }

    while (*power && isspace((unsigned char)*power))
    {
        power++;
    }

    /* Early exit: ensure a power name was specified after trimming */
    if (*power == '\0')
    {
        if (negate)
        {
            notify(player, "You must specify a power to clear.");
        }
        else
        {
            notify(player, "You must specify a power to set.");
        }

        return;
    }

    fp = find_power(target, power);

    if (fp == NULL)
    {
        notify(player, "I don't understand that power.");
        return;
    }

    /*
     * Invoke the power handler, and print feedback
     */
    result = fp->handler(target, player, fp->powervalue, fp->powerpower, negate);

    if (!result)
    {
        notify(player, NOPERM_MESSAGE);
    }
    else
    {
        /* Mark target modified regardless of quiet notifications */
        s_Modified(target);
        if (!(key & SET_QUIET) && !Quiet(player))
        {
            notify(player, (negate ? "Cleared." : "Set."));
        }
    }

    return;
}

/*
 * ---------------------------------------------------------------------------
 * * has_power: does object have power visible to player?
 */

int has_power(dbref player, dbref it, char *powername)
{
    POWERENT *fp;
    POWER fv;

    fp = find_power(it, powername);

    if (fp == NULL)
    {
        return 0;
    }

    /* Conditionally read only the required power bank to minimize macro overhead.
     * Unlike power_description which iterates and needs both, has_power checks a single
     * power once, so unconditional memoization would be wasteful.
     */
    if (fp->powerpower & POWER_EXT)
    {
        fv = Powers2(it);
    }
    else
    {
        fv = Powers(it);
    }

    if (fv & fp->powervalue)
    {
        if ((fp->listperm & CA_WIZARD) && !Wizard(player))
        {
            return 0;
        }

        if ((fp->listperm & CA_GOD) && !God(player))
        {
            return 0;
        }

        return 1;
    }

    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * * power_description: Return an mbuf containing the type and powers on thing.
 */

char *power_description(dbref player, dbref target)
{
    char *buff, *bp;
    POWERENT *fp;
    POWER f1, f2;
    POWER fv;
    int is_wizard, is_god;

    /*
     * Allocate the return buffer
     */
    bp = buff = XMALLOC(MBUF_SIZE, "buff");
    if (!buff)
    {
        return NULL;
    }
    /*
     * Store the header strings and object type
     */
    XSAFEMBSTR((char *)"Powers:", buff, &bp);

    /* Memoize power reads and permission checks for efficiency */
    f1 = Powers(target);
    f2 = Powers2(target);
    is_wizard = Wizard(player);
    is_god = God(player);

    for (fp = gen_powers; fp->powername; fp++)
    {
        if (fp->powerpower & POWER_EXT)
        {
            fv = f2;
        }
        else
        {
            fv = f1;
        }

        if (fv & fp->powervalue)
        {
            if ((fp->listperm & CA_WIZARD) && !is_wizard)
            {
                continue;
            }

            if ((fp->listperm & CA_GOD) && !is_god)
            {
                continue;
            }

            XSAFEMBCHR(' ', buff, &bp);
            XSAFEMBSTR((char *)fp->powername, buff, &bp);
        }
    }

    /*
     * Terminate the string, and return the buffer to the caller
     */
    *bp = '\0';
    return buff;
}

/*
 * ---------------------------------------------------------------------------
 * * decompile_powers: Produce commands to set powers on target.
 */

void decompile_powers(dbref player, dbref thing, char *thingname)
{
    POWER f1, f2;
    POWERENT *fp;
    char *buf;
    /*
     * Report generic powers.
     * thingname is assumed non-NULL from caller; it will be strip_ansi'd once for output.
     */
    f1 = Powers(thing);
    f2 = Powers2(thing);

    /* Strip ANSI codes once before the loop instead of repeatedly inside */
    buf = strip_ansi(thingname);
    if (!buf)
    {
        return;
    }

    for (fp = gen_powers; fp->powername; fp++)
    {
        /* Memoize POWER_EXT check to avoid redundant bitwise operations */
        int is_ext = (fp->powerpower & POWER_EXT);

        /*
         * Skip if we shouldn't decompile this power
         */
        if (fp->listperm & CA_NO_DECOMP)
        {
            continue;
        }

        /*
         * Skip if this power is not set
         */

        if (is_ext)
        {
            if (!(f2 & fp->powervalue))
            {
                continue;
            }
        }
        else
        {
            if (!(f1 & fp->powervalue))
            {
                continue;
            }
        }

        /*
         * Skip if we can't see this power
         */

        if (!check_access(player, fp->listperm))
        {
            continue;
        }

        /*
         * We made it this far, report this power
         */
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@power %s=%s", buf, fp->powername);
    }

    /* Free the stripped buffer once after the loop */
    XFREE(buf);
}

/* ---------------------------------------------------------------------------
 * cf_power_access: Modify who can set a power. Basically like
 * cf_flag_access.
 */

int cf_power_access(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    char *fstr, *permstr, *tokst;
    POWERENT *fp;

    /* Validate input: str must be provided for strtok_r to succeed */
    if (!str || !*str)
    {
        return -1;
    }

    fstr = strtok_r(str, " \t=,", &tokst);
    permstr = strtok_r(NULL, " \t=,", &tokst);

    if (!fstr || !*fstr)
    {
        return -1;
    }

    if (!permstr || !*permstr)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s", "Missing power access qualifier");
        return -1;
    }

    /* Normalize permstr to lowercase for case-insensitive matching.
     * Safe to modify in-place: strtok_r() returns a pointer into the 'str' buffer,
     * which is mutable and not used after this point. Lowercasing avoids repeated
     * string allocations and keeps the lookup efficient.
     */
    for (char *p = permstr; *p; ++p)
    {
        *p = (char)tolower((unsigned char)*p);
    }

    if ((fp = find_power(GOD, fstr)) == NULL)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "No such power", fstr);
        return -1;
    }

    /*
     * Don't change the handlers on special things.
     */

    if ((fp->handler != ph_any) && (fp->handler != ph_wizroy) && (fp->handler != ph_wiz) && (fp->handler != ph_god) && (fp->handler != ph_restrict_player) && (fp->handler != ph_privileged))
    {
        log_write(LOG_CONFIGMODS, "CFG", "PERM", "Cannot change access for power: %s", fp->powername);
        return -1;
    }

    if (!strcmp(permstr, (char *)"any"))
    {
        fp->handler = ph_any;
    }
    else if (!strcmp(permstr, (char *)"royalty"))
    {
        fp->handler = ph_wizroy;
    }
    else if (!strcmp(permstr, (char *)"wizard"))
    {
        fp->handler = ph_wiz;
    }
    else if (!strcmp(permstr, (char *)"god"))
    {
        fp->handler = ph_god;
    }
    else if (!strcmp(permstr, (char *)"restrict_player"))
    {
        fp->handler = ph_restrict_player;
    }
    else if (!strcmp(permstr, (char *)"privileged"))
    {
        fp->handler = ph_privileged;
    }
    else
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Power access", permstr);
        return -1;
    }

    return 0;
}
