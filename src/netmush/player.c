/**
 * @file player.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Player handling and processing
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

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <crypt.h>
#endif

/* ---------------------------------------------------------------------------
 * decrypt_logindata, encrypt_logindata: Decode and encode login info.
 */

void decrypt_logindata(char *atrbuf, LDATA *info)
{
    int i;
    info->tot_good = 0;
    info->tot_bad = 0;
    info->new_bad = 0;

    for (i = 0; i < NUM_GOOD; i++)
    {
        info->good[i].host = NULL;
        info->good[i].dtm = NULL;
    }

    for (i = 0; i < NUM_BAD; i++)
    {
        info->bad[i].host = NULL;
        info->bad[i].dtm = NULL;
    }

    if (*atrbuf == '#')
    {
        atrbuf++;
        info->tot_good = (int)strtol(grabto(&atrbuf, ';'), (char **)NULL, 10);

        for (i = 0; i < NUM_GOOD; i++)
        {
            info->good[i].host = grabto(&atrbuf, ';');
            info->good[i].dtm = grabto(&atrbuf, ';');
        }

        info->new_bad = (int)strtol(grabto(&atrbuf, ';'), (char **)NULL, 10);
        info->tot_bad = (int)strtol(grabto(&atrbuf, ';'), (char **)NULL, 10);

        for (i = 0; i < NUM_BAD; i++)
        {
            info->bad[i].host = grabto(&atrbuf, ';');
            info->bad[i].dtm = grabto(&atrbuf, ';');
        }
    }
}

void encrypt_logindata(char *atrbuf, LDATA *info)
{
    char *bp, nullc;
    int i;
    /*
     * Make sure the SPRINTF call tracks NUM_GOOD and NUM_BAD for the
     * * number of host/dtm pairs of each type.
     */
    nullc = '\0';

    for (i = 0; i < NUM_GOOD; i++)
    {
        if (!info->good[i].host)
        {
            info->good[i].host = &nullc;
        }

        if (!info->good[i].dtm)
        {
            info->good[i].dtm = &nullc;
        }
    }

    for (i = 0; i < NUM_BAD; i++)
    {
        if (!info->bad[i].host)
        {
            info->bad[i].host = &nullc;
        }

        if (!info->bad[i].dtm)
        {
            info->bad[i].dtm = &nullc;
        }
    }

    bp = XMALLOC(LBUF_SIZE, "bp");
    XSPRINTF(bp, "#%d;%s;%s;%s;%s;%s;%s;%s;%s;%d;%d;%s;%s;%s;%s;%s;%s;", info->tot_good, info->good[0].host, info->good[0].dtm, info->good[1].host, info->good[1].dtm, info->good[2].host, info->good[2].dtm, info->good[3].host, info->good[3].dtm, info->new_bad, info->tot_bad, info->bad[0].host, info->bad[0].dtm, info->bad[1].host, info->bad[1].dtm, info->bad[2].host, info->bad[2].dtm);
    XSTRCPY(atrbuf, bp);
    XFREE(bp);
}

/* ---------------------------------------------------------------------------
 * record_login: Record successful or failed login attempt.
 * If successful, report last successful login and number of failures since
 * last successful login.
 */

void record_login(dbref player, int isgood, char *ldate, char *lhost, char *lusername)
{
    LDATA login_info;
    char *atrbuf;
    dbref aowner;
    int aflags, alen, i;
    char *s;
    atrbuf = atr_get(player, A_LOGINDATA, &aowner, &aflags, &alen);
    decrypt_logindata(atrbuf, &login_info);

    if (isgood)
    {
        if (login_info.new_bad > 0)
        {
            notify(player, "");
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "**** %d failed connect%s since your last successful connect. ****", login_info.new_bad, (login_info.new_bad == 1 ? "" : "s"));
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Most recent attempt was from %s on %s.", login_info.bad[0].host, login_info.bad[0].dtm);
            notify(player, "");
            login_info.new_bad = 0;
        }

        if (login_info.good[0].host && *login_info.good[0].host && login_info.good[0].dtm && *login_info.good[0].dtm)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Last connect was from %s on %s.", login_info.good[0].host, login_info.good[0].dtm);
        }

        for (i = NUM_GOOD - 1; i > 0; i--)
        {
            login_info.good[i].dtm = login_info.good[i - 1].dtm;
            login_info.good[i].host = login_info.good[i - 1].host;
        }

        login_info.good[0].dtm = ldate;
        login_info.good[0].host = lhost;
        login_info.tot_good++;

        if (*lusername)
        {
            s = XASPRINTF("s", "%s@%s", lusername, lhost);
            atr_add_raw(player, A_LASTSITE, s);
            XFREE(s);
        }
        else
        {
            atr_add_raw(player, A_LASTSITE, lhost);
        }
    }
    else
    {
        for (i = NUM_BAD - 1; i > 0; i--)
        {
            login_info.bad[i].dtm = login_info.bad[i - 1].dtm;
            login_info.bad[i].host = login_info.bad[i - 1].host;
        }

        login_info.bad[0].dtm = ldate;
        login_info.bad[0].host = lhost;
        login_info.tot_bad++;
        login_info.new_bad++;
    }

    encrypt_logindata(atrbuf, &login_info);
    atr_add_raw(player, A_LOGINDATA, atrbuf);
    XFREE(atrbuf);
}

/* ---------------------------------------------------------------------------
 * check_pass: Test a password to see if it is correct.
 */

int check_pass(dbref player, const char *password)
{
    dbref aowner;
    int aflags, alen;
    char *target;
    target = atr_get(player, A_PASS, &aowner, &aflags, &alen);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    struct crypt_data cdata;
    cdata.initialized = 0;
    if (*target && strcmp(target, password) && strcmp(crypt_r(password, "XX", &cdata), target))
#else
    if (*target && strcmp(target, password) && strcmp(crypt(password, "XX"), target))
#endif
    {
        XFREE(target);
        return 0;
    }

    XFREE(target);

    /*
     * This is needed to prevent entering the raw encrypted password from
     * * working.  Do it better if you like, but it's needed.
     */

    if ((strlen(password) == 13) && (password[0] == 'X') && (password[1] == 'X'))
    {
        return 0;
    }

    return 1;
}

/* ---------------------------------------------------------------------------
 * connect_player: Try to connect to an existing player.
 */

dbref connect_player(char *name, char *password, char *host, char *username, char *ip_addr)
{
    dbref player, aowner;
    int aflags, alen;
    time_t tt;
    char time_str[26], *player_last, *allowance;
    struct tm tm_buf;
    time(&tt);
    localtime_r(&tt, &tm_buf);
    strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", &tm_buf);

    if ((player = lookup_player(NOTHING, name, 0)) == NOTHING)
    {
        return NOTHING;
    }

    if (!check_pass(player, password))
    {
        record_login(player, 0, time_str, host, username);
        return NOTHING;
    }

    time(&tt);
    char time_str2[26];
    struct tm tm_buf2;
    localtime_r(&tt, &tm_buf2);
    strftime(time_str2, sizeof(time_str2), "%a %b %d %H:%M:%S %Y", &tm_buf2);
    /*
     * compare to last connect see if player gets salary
     */
    player_last = atr_get(player, A_LAST, &aowner, &aflags, &alen);

    if (strncmp(player_last, time_str2, 10) != 0)
    {
        if (Pennies(player) < mushconf.paylimit)
        {
            /*
             * Don't heap coins on players who already have lots of money.
             */
            allowance = atr_pget(player, A_ALLOWANCE, &aowner, &aflags, &alen);

            if (*allowance == '\0')
            {
                giveto(player, mushconf.paycheck);
            }
            else
            {
                giveto(player, (int)strtol(allowance, (char **)NULL, 10));
            }

            XFREE(allowance);
        }
    }

    atr_add_raw(player, A_LAST, time_str2);
    XFREE(player_last);

    if (ip_addr && *ip_addr)
    {
        atr_add_raw(player, A_LASTIP, ip_addr);
    }

    return player;
}

/* ---------------------------------------------------------------------------
 * create_player: Create a new player.
 */

dbref create_player(char *name, char *password, dbref creator, int isrobot, int isguest)
{
    dbref player;
    char *pbuf;
    /*
     * Make sure the password is OK.  Name is checked in create_obj
     */
    pbuf = trim_spaces(password);

    if (!isguest && !ok_password(pbuf, creator))
    {
        XFREE(pbuf);
        return NOTHING;
    }

    /*
     * If so, go create him
     */
    player = create_obj(creator, TYPE_PLAYER, name, isrobot);

    if (player == NOTHING)
    {
        XFREE(pbuf);
        return NOTHING;
    }

    /*
     * initialize everything
     */

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        if (cam__mp->create_player)
        {
            (*(cam__mp->create_player))(creator, player, isrobot, isguest);
        }
    }

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    struct crypt_data cdata;
    cdata.initialized = 0;
    s_Pass(player, crypt_r(pbuf, "XX", &cdata));
#else
    s_Pass(player, crypt(pbuf, "XX"));
#endif
    s_Home(player, (Good_home(mushconf.start_home) ? mushconf.start_home : (Good_home(mushconf.start_room) ? mushconf.start_room : 0)));
    XFREE(pbuf);
    return player;
}

/* ---------------------------------------------------------------------------
 * do_password: Change the password for a player
 */

void do_password(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *oldpass, char *newpass)
{
    dbref aowner;
    int aflags, alen;
    char *target;
    target = atr_get(player, A_PASS, &aowner, &aflags, &alen);

    if (!*target || !check_pass(player, oldpass))
    {
        notify(player, "Sorry.");
    }
    else if (!ok_password(newpass, player))
    {
        /*
         * Do nothing, notification is handled by ok_password()
         */
    }
    else
    {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        struct crypt_data cdata;
        cdata.initialized = 0;
        atr_add_raw(player, A_PASS, crypt_r(newpass, "XX", &cdata));
#else
        atr_add_raw(player, A_PASS, crypt(newpass, "XX"));
#endif
        notify(player, "Password changed.");
    }

    XFREE(target);
}

/* ---------------------------------------------------------------------------
 * do_last Display login history data.
 */

void disp_from_on(dbref player, char *dtm_str, char *host_str)
{
    if (dtm_str && *dtm_str && host_str && *host_str)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "     From: %s   On: %s", dtm_str, host_str);
    }
}

void do_last(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *who)
{
    dbref target, aowner;
    LDATA login_info;
    char *atrbuf;
    int i, aflags, alen;

    if (!who || !*who)
    {
        target = Owner(player);
    }
    else if (!(string_compare(who, "me")))
    {
        target = Owner(player);
    }
    else
    {
        target = lookup_player(player, who, 1);
    }

    if (target == NOTHING)
    {
        notify(player, "I couldn't find that player.");
    }
    else if (!Controls(player, target))
    {
        notify(player, NOPERM_MESSAGE);
    }
    else
    {
        atrbuf = atr_get(target, A_LOGINDATA, &aowner, &aflags, &alen);
        decrypt_logindata(atrbuf, &login_info);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Total successful connects: %d", login_info.tot_good);

        for (i = 0; i < NUM_GOOD; i++)
        {
            disp_from_on(player, login_info.good[i].host, login_info.good[i].dtm);
        }

        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Total failed connects: %d", login_info.tot_bad);

        for (i = 0; i < NUM_BAD; i++)
        {
            disp_from_on(player, login_info.bad[i].host, login_info.bad[i].dtm);
        }

        XFREE(atrbuf);
    }
}

/* ---------------------------------------------------------------------------
 * add_player_name, delete_player_name, lookup_player:
 * Manage playername->dbref mapping
 */

int add_player_name(dbref player, char *name)
{
    int stat;
    dbref *p;
    char *temp, *tp;
    /*
     * Convert to all lowercase
     */
    tp = temp = XMALLOC(LBUF_SIZE, "tp");
    XSAFELBSTR(name, temp, &tp);

    for (tp = temp; *tp; tp++)
    {
        *tp = tolower(*tp);
    }

    p = (int *)hashfind(temp, &mushstate.player_htab);

    if (p)
    {
        /*
         * Entry found in the hashtable.  If a player, succeed if the
         * * numbers match (already correctly in the hash table),
         * * fail if they don't.
         */
        if (Good_obj(*p) && isPlayer(*p))
        {
            XFREE(temp);

            if (*p == player)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        /*
         * It's an alias (or an incorrect entry).  Clobber it
         */
        XFREE(p);
        p = (dbref *)XMALLOC(sizeof(dbref), "p");
        *p = player;
        stat = hashrepl(temp, p, &mushstate.player_htab);
        XFREE(temp);
    }
    else
    {
        p = (dbref *)XMALLOC(sizeof(dbref), "p");
        *p = player;
        stat = hashadd(temp, p, &mushstate.player_htab, 0);
        XFREE(temp);
        stat = (stat < 0) ? 0 : 1;
    }

    return stat;
}

int delete_player_name(dbref player, char *name)
{
    dbref *p;
    char *temp, *tp;
    tp = temp = XMALLOC(LBUF_SIZE, "temp");
    XSAFELBSTR(name, temp, &tp);

    for (tp = temp; *tp; tp++)
    {
        *tp = tolower(*tp);
    }

    p = (int *)hashfind(temp, &mushstate.player_htab);

    if (!p || (*p == NOTHING) || ((player != NOTHING) && (*p != player)))
    {
        XFREE(temp);
        return 0;
    }

    XFREE(p);
    hashdelete(temp, &mushstate.player_htab);
    XFREE(temp);
    return 1;
}

dbref lookup_player(dbref doer, char *name, int check_who)
{
    dbref *p, thing;
    char *temp, *tp;

    if (!string_compare(name, "me"))
    {
        return doer;
    }

    if (*name == LOOKUP_TOKEN)
    {
        do
        {
            name++;
        } while (isspace(*name));
    }

    if (*name == NUMBER_TOKEN)
    {
        name++;

        if (!is_number(name))
        {
            return NOTHING;
        }

        thing = (int)strtol(name, (char **)NULL, 10);

        if (!Good_obj(thing))
        {
            return NOTHING;
        }

        if (!((Typeof(thing) == TYPE_PLAYER) || God(doer)))
        {
            thing = NOTHING;
        }

        return thing;
    }

    tp = temp = XMALLOC(LBUF_SIZE, "temp");
    XSAFELBSTR(name, temp, &tp);

    for (tp = temp; *tp; tp++)
    {
        *tp = tolower(*tp);
    }

    p = (int *)hashfind(temp, &mushstate.player_htab);
    XFREE(temp);

    if (!p)
    {
        if (check_who)
        {
            thing = find_connected_name(doer, name);

            if (Hidden(thing) && !See_Hidden(doer))
            {
                thing = NOTHING;
            }
        }
        else
        {
            thing = NOTHING;
        }
    }
    else if (!Good_obj(*p))
    {
        thing = NOTHING;
    }
    else
    {
        thing = *p;
    }

    return thing;
}

void load_player_names(void)
{
    dbref aowner;
    int aflags, alen;
    char *alias, *p, *tokp;

    for (dbref i = 0; i < mushstate.db_top; i++)
    {
        if (Typeof(i) == TYPE_PLAYER)
        {
            add_player_name(i, Name(i));
        }
    }

    alias = XMALLOC(LBUF_SIZE, "alias");

    for (dbref i = 0; i < mushstate.db_top; i++)
    {
        if (Typeof(i) == TYPE_PLAYER)
        {
            alias = atr_get_str(alias, i, A_ALIAS, &aowner, &aflags, &alen);

            if (*alias)
            {
                for (p = strtok_r(alias, ";", &tokp); p; p = strtok_r(NULL, ";", &tokp))
                {
                    add_player_name(i, p);
                }
            }
        }
    }
    XFREE(alias);
}

/* ---------------------------------------------------------------------------
 * badname_add, badname_check, badname_list: Add/look for/display bad names.
 */

void badname_add(char *bad_name)
{
    BADNAME *bp;
    /*
     * Make a new node and link it in at the top
     */
    bp = (BADNAME *)XMALLOC(sizeof(BADNAME), "bp");
    bp->name = XSTRDUP(bad_name, "bp->name");
    bp->next = mushstate.badname_head;
    mushstate.badname_head = bp;
}

void badname_remove(char *bad_name)
{
    BADNAME *bp, *backp;
    /*
     * Look for an exact match on the bad name and remove if found
     */
    backp = NULL;

    for (bp = mushstate.badname_head; bp; backp = bp, bp = bp->next)
    {
        if (!string_compare(bad_name, bp->name))
        {
            if (backp)
            {
                backp->next = bp->next;
            }
            else
            {
                mushstate.badname_head = bp->next;
            }

            XFREE(bp->name);
            XFREE(bp);
            return;
        }
    }
}

int badname_check(char *bad_name)
{
    BADNAME *bp;

    /*
     * Walk the badname list, doing wildcard matching.  If we get a hit
     * * then return false.  If no matches in the list, return true.
     */

    for (bp = mushstate.badname_head; bp; bp = bp->next)
    {
        if (quick_wild(bp->name, bad_name))
        {
            return 0;
        }
    }

    return 1;
}

void badname_list(dbref player, const char *prefix)
{
    BADNAME *bp;
    char *buff, *bufp;
    /*
     * Construct an lbuf with all the names separated by spaces
     */
    buff = bufp = XMALLOC(LBUF_SIZE, "bufp");
    XSAFELBSTR((char *)prefix, buff, &bufp);

    for (bp = mushstate.badname_head; bp; bp = bp->next)
    {
        XSAFELBCHR(' ', buff, &bufp);
        XSAFELBSTR(bp->name, buff, &bufp);
    }

    /*
     * Now display it
     */
    notify(player, buff);
    XFREE(buff);
}
