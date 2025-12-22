/**
 * @file comsys.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief DarkZone-style channel subsystem for player-to-player communication
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
#include <errno.h>
#include <limits.h>

/* --------------------------------------------------------------------------
 * Constants.
 */

#define NO_CHAN_MSG "That is not a valid channel name."

#define CHAN_FLAG_PUBLIC 0x00000010
#define CHAN_FLAG_LOUD 0x00000020
#define CHAN_FLAG_P_JOIN 0x00000040
#define CHAN_FLAG_P_TRANS 0x00000080
#define CHAN_FLAG_P_RECV 0x00000100
#define CHAN_FLAG_O_JOIN 0x00000200
#define CHAN_FLAG_O_TRANS 0x00000400
#define CHAN_FLAG_O_RECV 0x00000800
#define CHAN_FLAG_SPOOF 0x00001000

#define CBOOT_QUIET 1      /* No boot message, just has left */
#define CEMIT_NOHEADER 1   /* Channel emit without header */
#define CHANNEL_SET 1      /* Set channel flag */
#define CHANNEL_CHARGE 2   /* Set channel charge */
#define CHANNEL_DESC 4     /* Set channel desc */
#define CHANNEL_LOCK 8     /* Set channel lock */
#define CHANNEL_OWNER 16   /* Set channel owner */
#define CHANNEL_JOIN 32    /* Channel lock: join */
#define CHANNEL_TRANS 64   /* Channel lock: transmit */
#define CHANNEL_RECV 128   /* Channel lock: receive */
#define CHANNEL_HEADER 256 /* Set channel header */
#define CLIST_FULL 1       /* Full listing of channels */
#define CLIST_HEADER 2     /* Header listing of channels */
#define CWHO_ALL 1         /* Show disconnected players on channel */

#define MAX_CHAN_NAME_LEN 20
#define MAX_CHAN_ALIAS_LEN 10
#define MAX_CHAN_DESC_LEN 256
#define MAX_CHAN_HEAD_LEN 64

/* --------------------------------------------------------------------------
 * Configuration and hash tables.
 */

struct mod_comsys_confstorage
{
    char *public_channel; /* Name of public channel */
    char *guests_channel; /* Name of guests channel */
    char *public_calias;  /* Alias of public channel */
    char *guests_calias;  /* Alias of guests channel */
} mod_comsys_config;

CONF mod_comsys_conftable[] = {
    {(char *)"guests_calias", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mod_comsys_config.guests_calias, SBUF_SIZE},
    {(char *)"guests_channel", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mod_comsys_config.guests_channel, SBUF_SIZE},
    {(char *)"public_calias", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mod_comsys_config.public_calias, SBUF_SIZE},
    {(char *)"public_channel", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mod_comsys_config.public_channel, SBUF_SIZE},
    {NULL, NULL, 0, 0, NULL, 0}};

HASHTAB mod_comsys_comsys_htab;
HASHTAB mod_comsys_calias_htab;
HASHTAB mod_comsys_comlist_htab;

MODHASHES mod_comsys_hashtable[] = {
    {"Channels", &mod_comsys_comsys_htab, 15, 8},
    {"Channel aliases", &mod_comsys_calias_htab, 500, 16},
    {NULL, NULL, 0, 0}};

MODHASHES mod_comsys_nhashtable[] = {
    {"Channel lists", &mod_comsys_comlist_htab, 100, 16},
    {NULL, NULL, 0, 0}};

MODVER mod_comsys_version;

/* --------------------------------------------------------------------------
 * Structure definitions.
 */

typedef struct com_player CHANWHO;
struct com_player
{
    dbref player;
    int is_listening;
    CHANWHO *next;
};

typedef struct com_channel CHANNEL;
struct com_channel
{
    char *name;
    dbref owner;
    unsigned int flags;
    char *header;          /* channel header prefixing messages */
    int num_who;           /* number of people on the channel */
    CHANWHO *who;          /* linked list of players on channel */
    int num_connected;     /* number of connected players on channel */
    CHANWHO **connect_who; /* array of connected player who structs */
    int charge;            /* cost to use channel */
    int charge_collected;  /* amount paid thus far */
    int num_sent;          /* number of messages sent */
    char *descrip;         /* description */
    BOOLEXP *join_lock;    /* who can join */
    BOOLEXP *trans_lock;   /* who can transmit */
    BOOLEXP *recv_lock;    /* who can receive */
};

typedef struct com_alias COMALIAS;
struct com_alias
{
    dbref player;
    char *alias;
    char *title;
    CHANNEL *channel;
};

typedef struct com_list COMLIST;
struct com_list
{
    COMALIAS *alias_ptr;
    COMLIST *next;
};

/* --------------------------------------------------------------------------
 * Macros.
 */

#define check_owned_channel(p, c)              \
    if (!Comm_All((p)) && ((p) != (c)->owner)) \
    {                                          \
        notify((p), NOPERM_MESSAGE);           \
        return;                                \
    }

#define find_channel(d, n, p)                                  \
    (p) = ((CHANNEL *)hashfind((n), &mod_comsys_comsys_htab)); \
    if (!(p))                                                  \
    {                                                          \
        notify((d), NO_CHAN_MSG);                              \
        return;                                                \
    }

#define lookup_channel(s) ((CHANNEL *)hashfind((s), &mod_comsys_comsys_htab))

#define lookup_clist(d) \
    ((COMLIST *)nhashfind((int)(d), &mod_comsys_comlist_htab))

#define ok_joinchannel(d, c) \
    ok_chanperms((d), (c), CHAN_FLAG_P_JOIN, CHAN_FLAG_O_JOIN, (c)->join_lock)

#define ok_recvchannel(d, c) \
    ok_chanperms((d), (c), CHAN_FLAG_P_RECV, CHAN_FLAG_O_RECV, (c)->recv_lock)

#define ok_sendchannel(d, c) \
    ok_chanperms((d), (c), CHAN_FLAG_P_TRANS, CHAN_FLAG_O_TRANS, (c)->trans_lock)

#define clear_chan_alias(n, a) \
    XFREE((a)->alias);         \
    if ((a)->title)            \
        XFREE((a)->title);     \
    XFREE((a));                \
    hashdelete((n), &mod_comsys_calias_htab)

/* --------------------------------------------------------------------------
 * Basic channel utilities.
 */

static int is_onchannel(dbref player, CHANNEL *chp)
{
    CHANWHO *wp;

    for (wp = chp->who; wp != NULL; wp = wp->next)
    {
        if (wp->player == player)
            return 1;
    }

    return 0;
}

static int is_listenchannel(dbref player, CHANNEL *chp)
{
    int i;

    if (!chp->connect_who)
        return 0;

    for (i = 0; i < chp->num_connected; i++)
    {
        if (chp->connect_who[i] && chp->connect_who[i]->player == player)
            return (chp->connect_who[i]->is_listening);
    }

    return 0;
}

static int is_listening_disconn(dbref player, CHANNEL *chp)
{
    CHANWHO *wp;

    for (wp = chp->who; wp != NULL; wp = wp->next)
    {
        if (wp->player == player)
            return (wp->is_listening);
    }

    return 0;
}

static int ok_channel_string(char *str, int maxlen, int ok_spaces, int ok_ansi)
{
    char *p;

    if (!str || !*str)
        return 0;

    if ((int)strlen(str) > maxlen - 1)
        return 0;

    for (p = str; *p; p++)
    {
        if ((!ok_spaces && isspace(*p)) ||
            (!ok_ansi && (*p == ESC_CHAR)))
        {
            return 0;
        }
    }

    return 1;
}

static char *munge_comtitle(char *title)
{
    static char tbuf[MBUF_SIZE];
    char *tp;
    tp = tbuf;

    *tbuf = 0x00;

    if (strchr(title, ESC_CHAR))
    {
        XSAFESTRCAT(tbuf, &tp, title, MBUF_SIZE - (strlen(ANSI_NORMAL) + 1));
        XSAFEMBSTR(ANSI_NORMAL, tbuf, &tp);
    }
    else
    {
        XSAFEMBSTR(title, tbuf, &tp);
    }

    return tbuf;
}

static int ok_chanperms(dbref player, CHANNEL *chp, int pflag, int oflag, BOOLEXP *c_lock)
{
    if (Comm_All(player))
        return 1;

    if (!Good_obj(player))
        return 0;

    switch (Typeof(player))
    {
    case TYPE_PLAYER:
        if (chp->flags & pflag)
            return 1;

        break;

    case TYPE_THING:
        if (chp->flags & oflag)
            return 1;

        break;

    default: /* only players and things on channels */
        return 0;
    }

    /* If we don't have a flag, and we don't have a lock, we default to
     * permission denied.
     */
    if (!c_lock)
        return 0;

    /* Channel locks are evaluated with respect to the channel owner. */

    if (eval_boolexp(player, chp->owner, chp->owner, c_lock))
        return 1;

    return 0;
}

COMALIAS *lookup_calias(dbref player, char *alias_str)
{
    char *s;
    COMALIAS *cas;
    s = XMALLOC(LBUF_SIZE, "s");
    if (!s)
    {
        return NULL;
    }
    XSNPRINTF(s, LBUF_SIZE, "%d.%s", player, alias_str);
    cas = (COMALIAS *)hashfind(s, &mod_comsys_calias_htab);
    XFREE(s);
    return (cas);
}

/* --------------------------------------------------------------------------
 * More complex utilities.
 */

static void update_comwho(CHANNEL *chp)
{
    /* We have to call this every time a channel is joined or left,
     * explicitly, as well as when players connect and disconnect.
     * This is a candidate for optimization; we should really just update
     * the list in-place, but this will do.
     */
    CHANWHO *wp;
    int i, count;
    /* We're only interested in whether or not a player is connected,
     * not whether or not they're actually listening to the channel.
     */
    count = 0;

    for (wp = chp->who; wp != NULL; wp = wp->next)
    {
        if (!isPlayer(wp->player) || Connected(wp->player))
            count++;
    }

    if (chp->connect_who)
        XFREE(chp->connect_who);

    chp->num_connected = count;

    if (count > 0)
    {
        chp->connect_who = (CHANWHO **)XCALLOC(count, sizeof(CHANWHO *), "chp->connect_who");
        if (!chp->connect_who)
        {
            chp->num_connected = 0;
            return;
        }
        i = 0;

        for (wp = chp->who; wp != NULL; wp = wp->next)
        {
            if (!isPlayer(wp->player) || Connected(wp->player))
            {
                if (i < count)
                {
                    chp->connect_who[i] = wp;
                    i++;
                }
            }
        }
    }
    else
    {
        chp->connect_who = NULL;
    }
}

static void com_message(CHANNEL *chp, char *msg, dbref cause)
{
    int i;
    CHANWHO *wp;
    char *mp, msg_ns[LBUF_SIZE];
    char *mh, *mh_ns;
    mh = mh_ns = NULL;
    if (chp->num_sent < INT_MAX)
        chp->num_sent++;
    mp = NULL;

    if (!chp->connect_who)
        return;

    for (i = 0; i < chp->num_connected; i++)
    {
        wp = chp->connect_who[i];
        if (!wp)
            continue;

        if (wp->is_listening && ok_recvchannel(wp->player, chp))
        {
            if (isPlayer(wp->player))
            {
                if (Nospoof(wp->player) && (wp->player != cause) &&
                    (wp->player != mushstate.curr_enactor) &&
                    (wp->player != mushstate.curr_player))
                {
                    if (!mp)
                    {
                        mp = msg_ns;
                        XSAFELBCHR('[', msg_ns, &mp);
                        safe_name(cause, msg_ns, &mp);
                        XSAFELBCHR('(', msg_ns, &mp);
                        XSAFELBCHR('#', msg_ns, &mp);
                        XSAFELTOS(msg_ns, &mp, cause, LBUF_SIZE);
                        XSAFELBCHR(')', msg_ns, &mp);

                        if (Good_obj(cause) && cause != Owner(cause))
                        {
                            XSAFELBCHR('{', msg_ns, &mp);
                            safe_name(Owner(cause), msg_ns, &mp);
                            XSAFELBCHR('}', msg_ns, &mp);
                        }

                        if (cause != mushstate.curr_enactor)
                        {
                            XSAFESTRCAT(msg_ns, &mp, "<-(#", LBUF_SIZE);
                            XSAFELTOS(msg_ns, &mp, cause, LBUF_SIZE);
                            XSAFELBCHR(')', msg_ns, &mp);
                        }

                        XSAFESTRCAT(msg_ns, &mp, "] ", LBUF_SIZE);
                        XSAFELBSTR(msg, msg_ns, &mp);
                    }

                    if (mushconf.have_pueblo != 1)
                    {
                        raw_notify(wp->player, msg_ns);
                    }
                    else
                    {

                        if (Html(wp->player))
                        {
                            if (!mh_ns)
                            {
                                mh_ns = XMALLOC(LBUF_SIZE, "mh_ns");
                                if (mh_ns)
                                {
                                    html_escape(msg_ns, mh_ns, 0);
                                }
                            }

                            raw_notify(wp->player, mh_ns);
                        }
                        else
                        {
                            raw_notify(wp->player, msg_ns);
                        }
                    }
                }
                else
                {
                    if (mushconf.have_pueblo != 1)
                    {
                        raw_notify(wp->player, msg);
                    }
                    else
                    {

                        if (Html(wp->player))
                        {
                            if (!mh)
                            {
                                mh = XMALLOC(LBUF_SIZE, "mh");
                                if (mh)
                                {
                                    html_escape(msg, mh, 0);
                                }
                            }

                            raw_notify(wp->player, mh);
                        }
                        else
                        {
                            raw_notify(wp->player, msg);
                        }
                    }
                }
            }
            else
            {
                notify_with_cause(wp->player, cause, msg);
            }
        }
    }

    if (mh)
        XFREE(mh);

    if (mh_ns)
        XFREE(mh_ns);
}

static void remove_from_channel(dbref player, CHANNEL *chp, int is_quiet)
{
    /* We assume that the player's channel aliases have already been
     * removed, and that other cleanup that is not directly related to
     * the channel structure itself has been accomplished. (We also
     * do no sanity-checking.)
     */
    char *s;
    CHANWHO *wp, *prev;

    /* Should never happen, but just in case... */

    if ((chp->num_who == 0) || !chp->who)
        return;

    /* If we only had one person, we can just nuke stuff. */

    if (chp->num_who == 1)
    {
        chp->num_who = 0;
        XFREE(chp->who);
        chp->who = NULL;
        chp->num_connected = 0;
        XFREE(chp->connect_who);
        chp->connect_who = NULL;
        return;
    }

    for (wp = chp->who, prev = NULL; wp != NULL; wp = wp->next)
    {
        if (wp->player == player)
        {
            if (prev)
            {
                prev->next = wp->next;
            }
            else
            {
                chp->who = wp->next;
            }

            XFREE(wp);
            break;
        }
        else
        {
            prev = wp;
        }
    }

    chp->num_who--;
    update_comwho(chp);

    if (!is_quiet &&
        (!isPlayer(player) || (Connected(player) && !Hidden(player))))
    {
        s = XMALLOC(LBUF_SIZE, "remove_from_channel.s");
        if (s)
        {
            XSNPRINTF(s, LBUF_SIZE, "%s %s has left this channel.", chp->header, Name(player));
            com_message(chp, s, player);
            XFREE(s);
        }
    }
}

static void zorch_alias_from_list(COMALIAS *cap)
{
    COMLIST *clist, *cl_ptr, *prev;
    clist = lookup_clist(cap->player);

    if (!clist)
        return;

    prev = NULL;

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next)
    {
        if (cl_ptr->alias_ptr == cap)
        {
            if (prev)
                prev->next = cl_ptr->next;
            else
            {
                clist = cl_ptr->next;

                if (clist)
                    nhashrepl((int)cap->player, (int *)clist,
                              &mod_comsys_comlist_htab);
                else
                    nhashdelete((int)cap->player, &mod_comsys_comlist_htab);
            }

            XFREE(cl_ptr);
            return;
        }

        prev = cl_ptr;
    }
}

static void process_comsys(dbref player, char *arg, COMALIAS *cap)
{
    CHANWHO *wp;
    char *buff, *name_buf, tbuf[LBUF_SIZE], *tp, *s;
    int i;

    if (!arg || !*arg)
    {
        notify(player, "No message.");
        return;
    }

    if (!strcmp(arg, (char *)"on"))
    {
        for (wp = cap->channel->who; wp != NULL; wp = wp->next)
        {
            if (wp->player == player)
                break;
        }

        if (!wp)
        {
            log_write(LOG_ALWAYS, "BUG", "COM", "Object #%d with alias %s is on channel %s but not on its player list.", player, cap->alias, cap->channel->name);
            notify(player, "An unusual channel error has been detected.");
            return;
        }

        if (wp->is_listening)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You are already on channel %s.", cap->channel->name);
            return;
        }

        wp->is_listening = 1;

        /* Only tell people that we've joined if we're an object, or
         * we're a connected and non-hidden player.
         */
        if (!isPlayer(player) || (Connected(player) && !Hidden(player)))
        {
            s = XMALLOC(LBUF_SIZE, "process_comsys.s");
            if (s)
            {
                XSNPRINTF(s, LBUF_SIZE, "%s %s has joined this channel.", cap->channel->header, Name(player));
                com_message(cap->channel, s, player);
                XFREE(s);
            }
        }

        return;
    }
    else if (!strcmp(arg, (char *)"off"))
    {
        for (wp = cap->channel->who; wp != NULL; wp = wp->next)
        {
            if (wp->player == player)
                break;
        }

        if (!wp)
        {
            log_write(LOG_ALWAYS, "BUG", "COM", "Object #%d with alias %s is on channel %s but not on its player list.", player, cap->alias, cap->channel->name);
            notify(player, "An unusual channel error has been detected.");
            return;
        }

        if (wp->is_listening == 0)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You are not on channel %s.", cap->channel->name);
            return;
        }

        wp->is_listening = 0;
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You leave channel %s.", cap->channel->name);

        /* Only tell people about it if we're an object, or we're a
         * connected and non-hidden player.
         */
        if (!isPlayer(player) || (Connected(player) && !Hidden(player)))
        {
            s = XMALLOC(LBUF_SIZE, "process_comsys.s");
            if (s)
            {
                XSNPRINTF(s, LBUF_SIZE, "%s %s has left this channel.", cap->channel->header, Name(player));
                com_message(cap->channel, s, player);
                XFREE(s);
            }
        }

        return;
    }
    else if (!strcmp(arg, (char *)"who"))
    {
        /* Allow players who have an alias for a channel to see who is
         * on it, even if they are not actively receiving.
         */
        notify(player, "-- Players --");

        if (cap->channel->connect_who)
        {
            for (i = 0; i < cap->channel->num_connected; i++)
            {
                wp = cap->channel->connect_who[i];
                if (!wp)
                    continue;

                if (isPlayer(wp->player))
                {
                    if (wp->is_listening && Connected(wp->player) &&
                        (!Hidden(wp->player) || See_Hidden(player)))
                    {
                        buff = unparse_object(player, wp->player, 0);
                        notify(player, buff);
                        XFREE(buff);
                    }
                }
            }
        }

        notify(player, "-- Objects -- ");

        if (cap->channel->connect_who)
        {
            for (i = 0; i < cap->channel->num_connected; i++)
            {
                wp = cap->channel->connect_who[i];
                if (!wp)
                    continue;

                if (!isPlayer(wp->player))
                {
                    if (wp->is_listening)
                    {
                        buff = unparse_object(player, wp->player, 0);
                        notify(player, buff);
                        XFREE(buff);
                    }
                }
            }
        }

        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "-- %s --", cap->channel->name);
        return;
    }
    else
    {
        if (Gagged(player))
        {
            notify(player, NOPERM_MESSAGE);
            return;
        }

        if (!is_listenchannel(player, cap->channel))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You must be on %s to do that.", cap->channel->name);
            return;
        }

        if (!ok_sendchannel(player, cap->channel))
        {
            notify(player, "You cannot transmit on that channel.");
            return;
        }

        if (!payfor(player, Guest(player) ? 0 : cap->channel->charge))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
            return;
        }

        if (cap->channel->charge_collected < INT_MAX - cap->channel->charge)
            cap->channel->charge_collected += cap->channel->charge;
        if (Good_obj(cap->channel->owner))
            giveto(cap->channel->owner, cap->channel->charge);

        if (cap->title)
        {
            if (cap->channel->flags & CHAN_FLAG_SPOOF)
            {
                name_buf = cap->title;
            }
            else
            {
                tp = tbuf;
                XSAFELBSTR(cap->title, tbuf, &tp);
                XSAFELBCHR(' ', tbuf, &tp);
                safe_name(player, tbuf, &tp);
                *tp = '\0';
                name_buf = tbuf;
            }
        }
        else
        {
            name_buf = NULL;
        }

        s = XMALLOC(LBUF_SIZE * 2, "process_comsys");
        if (s)
        {
            if (*arg == ':')
            {
                XSNPRINTF(s, LBUF_SIZE * 2, "%s %s %s", cap->channel->header, (name_buf) ? name_buf : Name(player), arg + 1);
            }
            else if (*arg == ';')
            {
                XSNPRINTF(s, LBUF_SIZE * 2, "%s %s%s", cap->channel->header, (name_buf) ? name_buf : Name(player), arg + 1);
            }
            else
            {
                XSNPRINTF(s, LBUF_SIZE * 2, "%s %s says, \"%s\"", cap->channel->header, (name_buf) ? name_buf : Name(player), arg);
            }
            com_message(cap->channel, s, player);
            XFREE(s);
            return;
        }
    }
}

/* --------------------------------------------------------------------------
 * Other externally-exposed utilities.
 */

void join_channel(dbref player, char *chan_name, char *alias_str, char *title_str)
{
    CHANNEL *chp;
    COMALIAS *cap;
    CHANWHO *wp;
    COMLIST *clist;
    int has_joined;
    char *s;

    if (!ok_channel_string(alias_str, MAX_CHAN_ALIAS_LEN, 0, 0))
    {
        notify(player, "That is not a valid channel alias.");
        return;
    }

    if (lookup_calias(player, alias_str) != NULL)
    {
        notify(player, "You are already using that channel alias.");
        return;
    }

    find_channel(player, chan_name, chp);
    has_joined = is_onchannel(player, chp);

    if (!has_joined && !ok_joinchannel(player, chp))
    {
        notify(player, "You cannot join that channel.");
        return;
    }

    /* Construct the alias. */
    cap = (COMALIAS *)XMALLOC(sizeof(COMALIAS), "join_channel.cap");
    cap->player = player;
    cap->alias = XSTRDUP(alias_str, "join_channel.cap->alias");

    if (!cap->alias)
    {
        notify(player, "Out of memory.");
        XFREE(cap);
        return;
    }

    /* Note that even if the player is already on this channel,
     * we do not inherit the channel title from other aliases.
     */
    if (title_str && *title_str)
    {
        cap->title = XSTRDUP(munge_comtitle(title_str), "join_channel.cap->title");

        if (!cap->title)
        {
            notify(player, "Out of memory.");
            XFREE(cap->alias);
            XFREE(cap);
            return;
        }
    }
    else
        cap->title = NULL;

    cap->channel = chp;
    s = XMALLOC(LBUF_SIZE, "join_channel.s");
    if (!s)
    {
        notify(player, "Out of memory.");
        if (cap->title)
            XFREE(cap->title);
        XFREE(cap->alias);
        XFREE(cap);
        return;
    }
    XSNPRINTF(s, LBUF_SIZE, "%d.%s", player, alias_str);
    hashadd(s, (int *)cap, &mod_comsys_calias_htab, 0);
    /* Add this to the list of all aliases for the player. */
    clist = (COMLIST *)XMALLOC(sizeof(COMLIST), "join_channel.clist");
    if (!clist)
    {
        notify(player, "Out of memory.");
        hashdelete(s, &mod_comsys_calias_htab);
        XFREE(s);
        if (cap->title)
            XFREE(cap->title);
        XFREE(cap->alias);
        XFREE(cap);
        return;
    }
    XFREE(s);
    clist->alias_ptr = cap;
    clist->next = lookup_clist(player);

    if (clist->next == NULL)
        nhashadd((int)player, (int *)clist, &mod_comsys_comlist_htab);
    else
        nhashrepl((int)player, (int *)clist, &mod_comsys_comlist_htab);

    /* If we haven't joined the channel, go do that. */

    if (!has_joined)
    {
        wp = (CHANWHO *)XMALLOC(sizeof(CHANWHO), "join_channel.wp");
        if (!wp)
        {
            notify(player, "Out of memory.");
            /* Remove the alias we just added */
            zorch_alias_from_list(cap);
            s = XMALLOC(LBUF_SIZE, "join_channel.cleanup");
            if (s)
            {
                XSNPRINTF(s, LBUF_SIZE, "%d.%s", player, alias_str);
                clear_chan_alias(s, cap);
                XFREE(s);
            }
            return;
        }
        wp->player = player;
        wp->is_listening = 1;

        if ((chp->num_who == 0) || (chp->who == NULL))
        {
            wp->next = NULL;
        }
        else
        {
            wp->next = chp->who;
        }

        chp->who = wp;
        if (chp->num_who < INT_MAX)
            chp->num_who++;
        update_comwho(chp);

        if (!isPlayer(player) || (Connected(player) && !Hidden(player)))
        {
            s = XMALLOC(LBUF_SIZE, "join_channel.s");
            if (s)
            {
                XSNPRINTF(s, LBUF_SIZE, "%s %s has joined this channel.", chp->header, Name(player));
                com_message(chp, s, player);
                XFREE(s);
            }
        }

        if (title_str)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Channel '%s' added with alias '%s' and title '%s'.", chp->name, alias_str, cap->title);
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Channel '%s' added with alias '%s'.", chp->name, alias_str);
        }
    }
    else
    {
        if (title_str)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Alias '%s' with title '%s' added for channel '%s'.", alias_str, cap->title, chp->name);
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Alias '%s' added for channel '%s'.", alias_str, chp->name);
        }
    }
}

void channel_clr(dbref player)
{
    CHANNEL **ch_array;
    COMLIST *clist, *cl_ptr, *next;
    int i, found, pos;
    char tbuf[SBUF_SIZE];
    /* We do not check if the comsys is enabled, because we want to clean
     * up our mess regardless.
     */
    clist = lookup_clist(player);

    if (!clist)
        return;

    /* Figure out all the channels we're on, then free up aliases. */
    ch_array = (CHANNEL **)XCALLOC(mod_comsys_comsys_htab.entries, sizeof(CHANNEL *), "ch_array");
    if (!ch_array)
    {
        return;
    }
    pos = 0;

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = next)
    {
        /* Bug #76: Check for NULL alias_ptr before dereferencing */
        if (!cl_ptr->alias_ptr)
        {
            next = cl_ptr->next;
            XFREE(cl_ptr);
            continue;
        }

        /* This is unnecessarily redundant, but it's not as if
         * a player is going to be on tons of channels.
         */
        found = 0;

        for (i = 0;
             (i < mod_comsys_comsys_htab.entries) && (ch_array[i] != NULL);
             i++)
        {
            if (ch_array[i] == cl_ptr->alias_ptr->channel)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            if (pos >= mod_comsys_comsys_htab.entries)
            {
                log_write(LOG_BUGS, "BUG", "CHR", "ch_array overflow in mod_comsys_cleanup_player");
                break;
            }

            ch_array[pos] = cl_ptr->alias_ptr->channel;
            pos++;
        }

        XSNPRINTF(tbuf, SBUF_SIZE, "%d.%s", player, cl_ptr->alias_ptr->alias);
        clear_chan_alias(tbuf, cl_ptr->alias_ptr);
        next = cl_ptr->next;
        XFREE(cl_ptr);
    }

    nhashdelete((int)player, &mod_comsys_comlist_htab);

    /* Remove from channels. */

    for (i = 0; i < pos; i++)
        remove_from_channel(player, ch_array[i], 0);

    XFREE(ch_array);
}

void mod_comsys_announce_connect(dbref player, const char *reason __attribute__((unused)), int num __attribute__((unused)))
{
    CHANNEL *chp;
    char *s;

    /* It's slightly easier to just go through the channels and see
     * which ones the player is on, for announcement purposes.
     */

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        if (is_onchannel(player, chp))
        {
            update_comwho(chp);

            if ((chp->flags & CHAN_FLAG_LOUD) && !Hidden(player) &&
                is_listenchannel(player, chp))
            {
                s = XMALLOC(LBUF_SIZE, "mod_comsys_announce_connect.s");
                if (s)
                {
                    XSNPRINTF(s, LBUF_SIZE, "%s %s has connected.", chp->header, Name(player));
                    com_message(chp, s, player);
                    XFREE(s);
                }
            }
        }
    }
}

void mod_comsys_announce_disconnect(dbref player, const char *reason __attribute__((unused)), int num __attribute__((unused)))
{
    CHANNEL *chp;
    char *s;

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        if (is_onchannel(player, chp))
        {
            if ((chp->flags & CHAN_FLAG_LOUD) && !Hidden(player) &&
                is_listenchannel(player, chp))
            {
                s = XMALLOC(LBUF_SIZE, "mod_comsys_announce_disconnect.s");
                if (s)
                {
                    XSNPRINTF(s, LBUF_SIZE, "%s %s has disconnected.", chp->header, Name(player));
                    com_message(chp, s, player);
                    XFREE(s);
                }

                update_comwho(chp);
            }
        }
    }
}

void update_comwho_all(void)
{
    CHANNEL *chp;

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        update_comwho(chp);
    }
}

void comsys_chown(dbref from_player, dbref to_player)
{
    CHANNEL *chp;

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        if (chp->owner == from_player)
            chp->owner = to_player;
    }
}

/* --------------------------------------------------------------------------
 * Comsys commands: channel administration.
 */

void do_ccreate(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *name)
{
    CHANNEL *chp;
    char buf[LBUF_SIZE];

    if (!Comm_All(player))
    {
        notify(player, NOPERM_MESSAGE);
        return;
    }

    if (!ok_channel_string(name, MAX_CHAN_NAME_LEN, 1, 0))
    {
        notify(player, NO_CHAN_MSG);
        return;
    }

    if (lookup_channel(name) != NULL)
    {
        notify(player, "That channel name is in use.");
        return;
    }

    chp = (CHANNEL *)XMALLOC(sizeof(CHANNEL), "do_ccreate.chp");

    if (!chp)
    {
        notify(player, "Out of memory.");
        return;
    }

    chp->name = XSTRDUP(name, "do_ccreate.chp->name");

    if (!chp->name)
    {
        notify(player, "Out of memory.");
        XFREE(chp);
        return;
    }

    chp->owner = Good_obj(player) ? Owner(player) : GOD;
    chp->flags = CHAN_FLAG_P_JOIN | CHAN_FLAG_P_TRANS | CHAN_FLAG_P_RECV |
                 CHAN_FLAG_O_JOIN | CHAN_FLAG_O_TRANS | CHAN_FLAG_O_RECV;
    chp->who = NULL;
    chp->num_who = 0;
    chp->connect_who = NULL;
    chp->num_connected = 0;
    chp->charge = 0;
    chp->charge_collected = 0;
    chp->num_sent = 0;
    chp->descrip = NULL;
    chp->join_lock = chp->trans_lock = chp->recv_lock = NULL;
    XSNPRINTF(buf, MBUF_SIZE, "[%s]", chp->name);
    chp->header = XSTRDUP(buf, "do_ccreate.chp->header");

    if (!chp->header)
    {
        notify(player, "Out of memory.");
        XFREE(chp->name);
        XFREE(chp);
        return;
    }

    hashadd(name, (int *)chp, &mod_comsys_comsys_htab, 0);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Channel %s created.", name);
}

void do_cdestroy(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *name)
{
    CHANNEL *chp;
    COMALIAS **alias_array;
    COMALIAS *cap;
    char **name_array;
    HASHTAB *htab;
    HASHENT *hptr;
    int i, count;
    char *s;
    find_channel(player, name, chp);
    check_owned_channel(player, chp);
    /* We have the wonderful joy of cleaning out all the aliases
     * that are currently pointing to this channel. We begin by
     * warning everyone that it's going away, and then we obliterate
     * it. We have to delete the pointers one by one or we run into
     * hashtable chaining issues.
     */
    s = XMALLOC(LBUF_SIZE, "do_cdestroy.s");
    if (s)
    {
        XSNPRINTF(s, LBUF_SIZE, "Channel %s has been destroyed by %s.", chp->name, Name(player));
        com_message(chp, s, player);
        XFREE(s);
    }
    htab = &mod_comsys_calias_htab;
    alias_array = (COMALIAS **)XCALLOC(htab->entries, sizeof(COMALIAS *), "alias_array");
    name_array = (char **)XCALLOC(htab->entries, sizeof(char *), "name_array");
    if (!alias_array || !name_array)
    {
        notify(player, "Out of memory - cannot clean up channel aliases.");
        XFREE(alias_array);
        XFREE(name_array);
        /* Still destroy the channel itself even if we can't clean aliases */
        XFREE(chp->name);
        if (chp->who)
            XFREE(chp->who);
        if (chp->connect_who)
            XFREE(chp->connect_who);
        if (chp->descrip)
            XFREE(chp->descrip);
        XFREE(chp->header);
        if (chp->join_lock)
            free_boolexp(chp->join_lock);
        if (chp->trans_lock)
            free_boolexp(chp->trans_lock);
        if (chp->recv_lock)
            free_boolexp(chp->recv_lock);
        XFREE(chp);
        hashdelete(name, &mod_comsys_comsys_htab);
        return;
    }
    count = 0;

    for (i = 0; i < htab->hashsize; i++)
    {
        for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next)
        {
            cap = (COMALIAS *)hptr->data;

            if (cap->channel == chp)
            {
                if (count >= htab->entries)
                {
                    log_write(LOG_BUGS, "BUG", "CHR", "alias_array overflow in do_cdestroy");
                    break;
                }

                name_array[count] = hptr->target.s;
                alias_array[count] = cap;
                count++;
            }
        }
    }

    /* Delete the aliases from the players' lists, then wipe them out. */

    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            zorch_alias_from_list(alias_array[i]);
            clear_chan_alias(name_array[i], alias_array[i]);
        }
    }

    XFREE(name_array);
    XFREE(alias_array);
    /* Zap the channel itself. */
    XFREE(chp->name);

    if (chp->who)
        XFREE(chp->who);

    if (chp->connect_who)
        XFREE(chp->connect_who);

    if (chp->descrip)
        XFREE(chp->descrip);

    XFREE(chp->header);

    if (chp->join_lock)
        free_boolexp(chp->join_lock);

    if (chp->trans_lock)
        free_boolexp(chp->trans_lock);

    if (chp->recv_lock)
        free_boolexp(chp->recv_lock);

    XFREE(chp);
    hashdelete(name, &mod_comsys_comsys_htab);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Channel %s destroyed.", name);
}

void do_channel(dbref player, dbref cause __attribute__((unused)), int key, char *chan_name, char *arg)
{
    CHANNEL *chp;
    BOOLEXP *boolp;
    dbref new_owner;
    int c_charge, negate, flag;
    find_channel(player, chan_name, chp);
    check_owned_channel(player, chp);

    if (!key || (key & CHANNEL_SET))
    {
        if (*arg == '!')
        {
            negate = 1;
            arg++;
        }
        else
        {
            negate = 0;
        }

        if (!strcasecmp(arg, (char *)"public"))
        {
            flag = CHAN_FLAG_PUBLIC;
        }
        else if (!strcasecmp(arg, (char *)"loud"))
        {
            flag = CHAN_FLAG_LOUD;
        }
        else if (!strcasecmp(arg, (char *)"spoof"))
        {
            flag = CHAN_FLAG_SPOOF;
        }
        else if (!strcasecmp(arg, (char *)"p_join"))
        {
            flag = CHAN_FLAG_P_JOIN;
        }
        else if (!strcasecmp(arg, (char *)"p_transmit"))
        {
            flag = CHAN_FLAG_P_TRANS;
        }
        else if (!strcasecmp(arg, (char *)"p_receive"))
        {
            flag = CHAN_FLAG_P_RECV;
        }
        else if (!strcasecmp(arg, (char *)"o_join"))
        {
            flag = CHAN_FLAG_O_JOIN;
        }
        else if (!strcasecmp(arg, (char *)"o_transmit"))
        {
            flag = CHAN_FLAG_O_TRANS;
        }
        else if (!strcasecmp(arg, (char *)"o_receive"))
        {
            flag = CHAN_FLAG_O_RECV;
        }
        else
        {
            notify(player, "That is not a valid channel flag name.");
            return;
        }

        if (negate)
            chp->flags &= ~flag;
        else
            chp->flags |= flag;

        notify(player, "Set.");
    }
    else if (key & CHANNEL_LOCK)
    {
        if (arg && *arg)
        {
            boolp = parse_boolexp(player, arg, 0);

            if (boolp == TRUE_BOOLEXP)
            {
                notify(player, "I don't understand that key.");
                free_boolexp(boolp);
                return;
            }

            if (key & CHANNEL_JOIN)
            {
                if (chp->join_lock)
                    free_boolexp(chp->join_lock);

                chp->join_lock = boolp;
            }
            else if (key & CHANNEL_RECV)
            {
                if (chp->recv_lock)
                    free_boolexp(chp->recv_lock);

                chp->recv_lock = boolp;
            }
            else if (key & CHANNEL_TRANS)
            {
                if (chp->trans_lock)
                    free_boolexp(chp->trans_lock);

                chp->trans_lock = boolp;
            }
            else
            {
                notify(player, "You must specify a valid lock type.");
                free_boolexp(boolp);
                return;
            }

            notify(player, "Channel locked.");
        }
        else
        {
            if (key & CHANNEL_JOIN)
            {
                if (chp->join_lock)
                    free_boolexp(chp->join_lock);

                chp->join_lock = NULL;
            }
            else if (key & CHANNEL_RECV)
            {
                if (chp->recv_lock)
                    free_boolexp(chp->recv_lock);

                chp->recv_lock = NULL;
            }
            else if (key & CHANNEL_TRANS)
            {
                if (chp->trans_lock)
                    free_boolexp(chp->trans_lock);

                chp->trans_lock = NULL;
            }

            notify(player, "Channel unlocked.");
        }
    }
    else if (key & CHANNEL_OWNER)
    {
        new_owner = lookup_player(player, arg, 1);

        if (Good_obj(new_owner))
        {
            chp->owner = Owner(new_owner); /* no robots */
            notify(player, "Owner set.");
        }
        else
        {
            notify(player, "No such player.");
        }
    }
    else if (key & CHANNEL_CHARGE)
    {
        char *endptr;
        long val;

        errno = 0;
        val = strtol(arg, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || val < 0 || val > 32767)
        {
            notify(player, "That is not a reasonable cost.");
            return;
        }

        c_charge = (int)val;
        chp->charge = c_charge;
        notify(player, "Set.");
    }
    else if (key & CHANNEL_DESC)
    {
        if (arg && *arg && !ok_channel_string(arg, MAX_CHAN_DESC_LEN, 1, 1))
        {
            notify(player, "That is not a reasonable channel description.");
            return;
        }

        if (chp->descrip)
            XFREE(chp->descrip);

        if (arg && *arg)
        {
            chp->descrip = XSTRDUP(arg, "do_channel.chp->descrip");
            if (!chp->descrip)
            {
                notify(player, "Out of memory.");
                return;
            }
        }
        else
            chp->descrip = NULL;

        notify(player, "Set.");
    }
    else if (key & CHANNEL_HEADER)
    {
        if (arg && *arg && !ok_channel_string(arg, MAX_CHAN_HEAD_LEN, 1, 1))
        {
            notify(player, "That is not a reasonable channel header.");
            return;
        }

        XFREE(chp->header);

        if (!arg)
            chp->header = XSTRDUP("", "do_channel.chp->header");
        else
            chp->header = XSTRDUP(arg, "do_channel.chp->header");

        if (!chp->header)
        {
            notify(player, "Out of memory.");
            return;
        }

        notify(player, "Set.");
    }
    else
    {
        notify(player, "Invalid channel command.");
    }
}

void do_cboot(dbref player, dbref cause __attribute__((unused)), int key, char *name, char *objstr)
{
    CHANNEL *chp;
    dbref thing;
    COMLIST *chead, *clist, *cl_ptr, *next, *prev;
    char *t, *s;
    char tbuf[SBUF_SIZE];
    find_channel(player, name, chp);
    check_owned_channel(player, chp);
    thing = match_thing(player, objstr);

    if (thing == NOTHING)
        return;

    if (!is_onchannel(thing, chp))
    {
        notify(player, "Your target is not on that channel.");
        return;
    }

    /* Clear out all of the player's aliases for this channel. */
    chead = clist = lookup_clist(thing);

    if (clist)
    {
        prev = NULL;

        for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = next)
        {
            next = cl_ptr->next;

            if (cl_ptr->alias_ptr->channel == chp)
            {
                if (prev)
                    prev->next = cl_ptr->next;
                else
                    clist = cl_ptr->next;

                XSNPRINTF(tbuf, SBUF_SIZE, "%d.%s", thing, cl_ptr->alias_ptr->alias);
                clear_chan_alias(tbuf, cl_ptr->alias_ptr);
                XFREE(cl_ptr);
            }
            else
            {
                prev = cl_ptr;
            }
        }

        if (!clist)
            nhashdelete((int)thing, &mod_comsys_comlist_htab);
        else if (chead != clist)
            nhashrepl((int)thing, (int *)clist, &mod_comsys_comlist_htab);
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You boot %s off channel %s.", Name(thing), chp->name);
    notify_check(thing, thing, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s boots you off channel %s.", Name(player), chp->name);

    if (key & CBOOT_QUIET)
    {
        remove_from_channel(thing, chp, 0);
    }
    else
    {
        remove_from_channel(thing, chp, 1);
        t = tbuf;
        XSAFESBSTR(Name(player), tbuf, &t);
        s = XMALLOC(LBUF_SIZE, "do_cboot.s");
        if (s)
        {
            XSNPRINTF(s, LBUF_SIZE, "%s %s boots %s off the channel.", chp->header, tbuf, Name(thing));
            com_message(chp, s, player);
            XFREE(s);
        }
    }
}

void do_cemit(dbref player, dbref cause __attribute__((unused)), int key, char *chan_name, char *str)
{
    CHANNEL *chp;
    char *s;
    find_channel(player, chan_name, chp);
    check_owned_channel(player, chp);

    if (key & CEMIT_NOHEADER)
        com_message(chp, str, player);
    else
    {
        s = XMALLOC(LBUF_SIZE, "do_cemit.s");
        if (s)
        {
            XSNPRINTF(s, LBUF_SIZE, "%s %s", chp->header, str);
            com_message(chp, s, player);
            XFREE(s);
        }
    }
}

void do_cwho(dbref player, dbref cause __attribute__((unused)), int key, char *chan_name)
{
    CHANNEL *chp;
    CHANWHO *wp;
    int i;
    int p_count, o_count;
    find_channel(player, chan_name, chp);
    check_owned_channel(player, chp);
    p_count = o_count = 0;
    notify(player, "      Name                      Player?");

    if (key & CWHO_ALL)
    {
        for (wp = chp->who; wp != NULL; wp = wp->next)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s  %-25s %7s", (wp->is_listening) ? "[on]" : "    ", Name(wp->player), isPlayer(wp->player) ? "Yes" : "No");

            if (isPlayer(wp->player))
                p_count++;
            else
                o_count++;
        }
    }
    else
    {
        if (chp->connect_who)
        {
            for (i = 0; i < chp->num_connected; i++)
            {
                wp = chp->connect_who[i];
                if (!wp)
                    continue;

                if (!Hidden(wp->player) || See_Hidden(player))
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s  %-25s %7s", (wp->is_listening) ? "[on]" : "    ", Name(wp->player), isPlayer(wp->player) ? "Yes" : "No");

                    if (isPlayer(wp->player))
                        p_count++;
                    else
                        o_count++;
                }
            }
        }

        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Counted %d %s and %d %s on channel %s.", p_count, (p_count == 1) ? "player" : "players", o_count, (o_count == 1) ? "object" : "objects", chp->name);
    }
}

/* --------------------------------------------------------------------------
 * Comsys commands: player-usable.
 */

void do_addcom(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *alias_str, char *args[], int nargs)
{
    char *chan_name, *title_str;

    if (nargs < 1)
    {
        notify(player, "You need to specify a channel.");
        return;
    }

    chan_name = args[0];

    if (nargs < 2)
    {
        title_str = NULL;
    }
    else
    {
        title_str = args[1];
    }

    join_channel(player, chan_name, alias_str, title_str);
}

void do_delcom(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *alias_str)
{
    COMALIAS *cap;
    CHANNEL *chp;
    COMLIST *clist, *cl_ptr;
    int has_mult;
    char *s;
    s = XMALLOC(LBUF_SIZE, "do_delcom.s");
    if (!s)
    {
        notify(player, "Out of memory.");
        return;
    }
    XSNPRINTF(s, LBUF_SIZE, "%d.%s", player, alias_str);
    cap = (COMALIAS *)hashfind(s, &mod_comsys_calias_htab);

    if (!cap)
    {
        notify(player, "No such channel alias.");
        XFREE(s);
        return;
    }

    XFREE(s);
    chp = cap->channel; /* save this for later */
    zorch_alias_from_list(cap);
    s = XMALLOC(LBUF_SIZE, "do_delcom.s");
    if (s)
    {
        XSNPRINTF(s, LBUF_SIZE, "%d.%s", player, alias_str);
        clear_chan_alias(s, cap);
        XFREE(s);
    }
    /* Check if we have any aliases left pointing to that channel. */
    clist = lookup_clist(player);
    has_mult = 0;

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next)
    {
        if (cl_ptr->alias_ptr->channel == chp)
        {
            has_mult = 1;
            break;
        }
    }

    if (has_mult)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You remove the alias '%s' for channel %s.", alias_str, chp->name);
    }
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You leave channel %s.", chp->name);
        remove_from_channel(player, chp, 0);
    }
}

void do_clearcom(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
    notify(player, "You remove yourself from all channels.");
    channel_clr(player);
}

void do_comtitle(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *alias_str, char *title)
{
    COMALIAS *cap;
    char *s;
    s = XMALLOC(LBUF_SIZE, "do_comtitle.s");
    if (!s)
        return;
    XSNPRINTF(s, LBUF_SIZE, "%d.%s", player, alias_str);
    cap = (COMALIAS *)hashfind(s, &mod_comsys_calias_htab);
    XFREE(s);

    if (!cap)
    {
        notify(player, "No such channel alias.");
        return;
    }

    if (cap->title)
        XFREE(cap->title);
    cap->title = NULL; /* avoid dangling pointer after free */

    if (!title || !*title)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Title cleared on channel %s.", cap->channel->name);
        return;
    }

    cap->title = XSTRDUP(munge_comtitle(title), "do_comtitle.cap->title");

    if (!cap->title)
    {
        notify(player, "Out of memory.");
        return;
    }

    if (!cap->title)
    {
        notify(player, "Out of memory.");
        return;
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Title set to '%s' on channel %s.", cap->title, cap->channel->name);
}

void do_clist(dbref player, dbref cause __attribute__((unused)), int key, char *chan_name)
{
    CHANNEL *chp;
    char *buff, tbuf[LBUF_SIZE], *tp;
    int count = 0;

    if (chan_name && *chan_name)
    {
        find_channel(player, chan_name, chp);
        check_owned_channel(player, chp);
        notify(player, chp->name);
        tp = tbuf;
        XSAFELBSTR("Flags:", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_PUBLIC)
            XSAFELBSTR(" Public", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_LOUD)
            XSAFELBSTR(" Loud", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_SPOOF)
            XSAFELBSTR(" Spoof", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_P_JOIN)
            XSAFELBSTR(" P_Join", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_P_RECV)
            XSAFELBSTR(" P_Receive", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_P_TRANS)
            XSAFELBSTR(" P_Transmit", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_O_JOIN)
            XSAFELBSTR(" O_Join", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_O_RECV)
            XSAFELBSTR(" O_Receive", tbuf, &tp);

        if (chp->flags & CHAN_FLAG_O_TRANS)
            XSAFELBSTR(" O_Transmit", tbuf, &tp);

        *tp = '\0';
        notify(player, tbuf);

        if (chp->join_lock)
            buff = unparse_boolexp(player, chp->join_lock);
        else
            buff = XSTRDUP("*UNLOCKED*", "buff");

        if (buff)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Join Lock: %s", buff);
            XFREE(buff);
        }

        if (chp->trans_lock)
            buff = unparse_boolexp(player, chp->trans_lock);
        else
            buff = XSTRDUP("*UNLOCKED*", "buff");

        if (buff)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Transmit Lock: %s", buff);
            XFREE(buff);
        }

        if (chp->recv_lock)
            buff = unparse_boolexp(player, chp->recv_lock);
        else
            buff = XSTRDUP("*UNLOCKED*", "buff");

        if (buff)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Receive Lock: %s", buff);
            XFREE(buff);
        }

        if (chp->descrip)
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Description: %s", chp->descrip);

        return;
    }

    if (key & CLIST_FULL)
    {
        notify(player, "Channel              Flags      Locks  Charge  Balance  Users  Messages  Owner");
    }
    else if (key & CLIST_HEADER)
    {
        notify(player, "Channel              Owner              Header");
    }
    else
    {
        notify(player, "Channel              Owner              Description");
    }

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        if ((chp->flags & CHAN_FLAG_PUBLIC) ||
            Comm_All(player) || (chp->owner == player))
        {
            if (key & CLIST_FULL)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-20s %c%c%c%c%c%c%c%c%c  %c%c%c    %6d  %7d  %5d  %8d  #%d", chp->name, (chp->flags & CHAN_FLAG_PUBLIC) ? 'P' : '-', (chp->flags & CHAN_FLAG_LOUD) ? 'L' : '-', (chp->flags & CHAN_FLAG_SPOOF) ? 'S' : '-', (chp->flags & CHAN_FLAG_P_JOIN) ? 'J' : '-', (chp->flags & CHAN_FLAG_P_TRANS) ? 'X' : '-', (chp->flags & CHAN_FLAG_P_RECV) ? 'R' : '-', (chp->flags & CHAN_FLAG_O_JOIN) ? 'j' : '-', (chp->flags & CHAN_FLAG_O_TRANS) ? 'x' : '-', (chp->flags & CHAN_FLAG_O_RECV) ? 'r' : '-', (chp->join_lock) ? 'J' : '-', (chp->trans_lock) ? 'X' : '-', (chp->recv_lock) ? 'R' : '-', chp->charge, chp->charge_collected, chp->num_who, chp->num_sent, chp->owner);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-20s %-18s %-38.38s", chp->name, Name(chp->owner), ((key & CLIST_HEADER) ? chp->header : (chp->descrip ? chp->descrip : " ")));
            }

            count++;
        }
    }

    if (Comm_All(player))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "There %s %d %s.", (count == 1) ? "is" : "are", count, (count == 1) ? "channel" : "channels");
    }
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "There %s %d %s visible to you.", (count == 1) ? "is" : "are", count, (count == 1) ? "channel" : "channels");
    }
}

void do_comlist(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
    COMLIST *clist, *cl_ptr;
    int count = 0;
    clist = lookup_clist(player);

    if (!clist)
    {
        notify(player, "You are not on any channels.");
        return;
    }

    notify(player, "Alias      Channel              Title");

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next)
    {
        /* We are guaranteed alias and channel lengths that are not truncated.
         * We need to truncate title.
         */
        if (!cl_ptr->alias_ptr || !cl_ptr->alias_ptr->channel)
            continue;
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-10s %-20s %-40.40s  %s", cl_ptr->alias_ptr->alias, cl_ptr->alias_ptr->channel->name, (cl_ptr->alias_ptr->title) ? cl_ptr->alias_ptr->title : (char *)"                                        ", (is_listenchannel(player, cl_ptr->alias_ptr->channel)) ? "[on]" : " ");
        count++;
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You have %d channel %s.", count, (count == 1) ? "alias" : "aliases");
}

void do_allcom(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *cmd)
{
    COMLIST *clist, *cl_ptr;
    clist = lookup_clist(player);

    if (!clist)
    {
        notify(player, "You are not on any channels.");
        return;
    }

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next)
    {
        if (cl_ptr->alias_ptr)
            process_comsys(player, cmd, cl_ptr->alias_ptr);
    }
}

int mod_comsys_process_command(dbref player, dbref cause __attribute__((unused)), int interactive __attribute__((unused)), char *in_cmd, char *args[] __attribute__((unused)), int nargs __attribute__((unused)))
{
    /* Return 1 if we got something, 0 if we didn't. */
    char *arg;
    COMALIAS *cap;
    char cmd[LBUF_SIZE]; /* temp -- can't nibble our input */

    if (!in_cmd || !*in_cmd || Slave(player))
        return 0;

    if (strlen(in_cmd) >= LBUF_SIZE)
    {
        return 0; /* Input too long, cannot process */
    }

    /* Use XSTRNCPY for tracked string copying */
    XSTRNCPY(cmd, in_cmd, LBUF_SIZE - 1);
    arg = cmd;

    while (*arg && !isspace(*arg))
        arg++;

    if (*arg)
        *arg++ = '\0';

    cap = lookup_calias(player, cmd);

    if (!cap)
        return 0;

    while (*arg && isspace(*arg))
        arg++;

    if (!*arg)
    {
        notify(player, "No message.");
        return 1;
    }

    process_comsys(player, arg, cap);
    return 1;
}

/* --------------------------------------------------------------------------
 * Command tables.
 */

NAMETAB cboot_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, CBOOT_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB cemit_sw[] = {
    {(char *)"noheader", 1, CA_PUBLIC, CEMIT_NOHEADER},
    {NULL, 0, 0, 0}};

NAMETAB channel_sw[] = {
    {(char *)"charge", 1, CA_PUBLIC, CHANNEL_CHARGE},
    {(char *)"desc", 1, CA_PUBLIC, CHANNEL_DESC},
    {(char *)"header", 1, CA_PUBLIC, CHANNEL_HEADER},
    {(char *)"lock", 1, CA_PUBLIC, CHANNEL_LOCK},
    {(char *)"owner", 1, CA_PUBLIC, CHANNEL_OWNER},
    {(char *)"set", 1, CA_PUBLIC, CHANNEL_SET},
    {(char *)"join", 1, CA_PUBLIC, CHANNEL_JOIN | SW_MULTIPLE},
    {(char *)"transmit", 1, CA_PUBLIC, CHANNEL_TRANS | SW_MULTIPLE},
    {(char *)"receive", 1, CA_PUBLIC, CHANNEL_RECV | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB clist_sw[] = {
    {(char *)"full", 1, CA_PUBLIC, CLIST_FULL},
    {(char *)"header", 1, CA_PUBLIC, CLIST_HEADER},
    {NULL, 0, 0, 0}};

NAMETAB cwho_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, CWHO_ALL},
    {NULL, 0, 0, 0}};

CMDENT mod_comsys_cmdtable[] = {
    {(char *)"@cboot", cboot_sw, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_cboot}},
    {(char *)"@ccreate", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_ccreate}},
    {(char *)"@cdestroy", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_cdestroy}},
    {(char *)"@cemit", cemit_sw, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_cemit}},
    {(char *)"@channel", channel_sw, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_channel}},
    {(char *)"@clist", clist_sw, CA_NO_SLAVE, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_clist}},
    {(char *)"@cwho", cwho_sw, CA_NO_SLAVE, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_cwho}},
    {(char *)"addcom", NULL, CA_NO_SLAVE, 0, CS_TWO_ARG | CS_ARGV, NULL, NULL, NULL, {do_addcom}},
    {(char *)"allcom", NULL, CA_NO_SLAVE, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_allcom}},
    {(char *)"comlist", NULL, CA_NO_SLAVE, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_comlist}},
    {(char *)"comtitle", NULL, CA_NO_SLAVE, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_comtitle}},
    {(char *)"clearcom", NULL, CA_NO_SLAVE, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_clearcom}},
    {(char *)"delcom", NULL, CA_NO_SLAVE, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_delcom}},
    {(char *)NULL, NULL, 0, 0, 0, NULL, NULL, NULL, {NULL}}};

/* --------------------------------------------------------------------------
 * Initialization, and other fun with files.
 */

void mod_comsys_dump_database(FILE *fp)
{
    CHANNEL *chp;
    COMALIAS *cap;
    fprintf(fp, "+V4\n");

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        putstring(fp, chp->name);
        putref(fp, chp->owner);
        putref(fp, chp->flags);
        putref(fp, chp->charge);
        putref(fp, chp->charge_collected);
        putref(fp, chp->num_sent);
        putstring(fp, chp->descrip);
        putstring(fp, chp->header);
        putboolexp(fp, chp->join_lock);
        fprintf(fp, "-\n");
        putboolexp(fp, chp->trans_lock);
        fprintf(fp, "-\n");
        putboolexp(fp, chp->recv_lock);
        fprintf(fp, "-\n");
        fprintf(fp, "<\n");
    }

    fprintf(fp, "+V1\n");

    for (cap = (COMALIAS *)hash_firstentry(&mod_comsys_calias_htab);
         cap != NULL;
         cap = (COMALIAS *)hash_nextentry(&mod_comsys_calias_htab))
    {
        putref(fp, cap->player);
        putstring(fp, cap->channel->name);
        putstring(fp, cap->alias);
        putstring(fp, cap->title);
        putref(fp, is_listening_disconn(cap->player, cap->channel));
        fprintf(fp, "<\n");
    }

    fprintf(fp, "*** END OF DUMP ***\n");
}

static void comsys_flag_convert(CHANNEL *chp)
{
    /* Convert MUX-style comsys flags to the new style. */
    int old_flags, new_flags;
    old_flags = chp->flags;
    new_flags = 0;

    if (old_flags & 0x200)
        new_flags |= CHAN_FLAG_PUBLIC;

    if (old_flags & 0x100)
        new_flags |= CHAN_FLAG_LOUD;

    if (old_flags & 0x1)
        new_flags |= CHAN_FLAG_P_JOIN;

    if (old_flags & 0x2)
        new_flags |= CHAN_FLAG_P_TRANS;

    if (old_flags & 0x4)
        new_flags |= CHAN_FLAG_P_RECV;

    if (old_flags & (0x10 * 0x1))
        new_flags |= CHAN_FLAG_O_JOIN;

    if (old_flags & (0x10 * 0x2))
        new_flags |= CHAN_FLAG_O_TRANS;

    if (old_flags & (0x10 * 0x4))
        new_flags |= CHAN_FLAG_O_RECV;

    chp->flags = new_flags;
}

static void comsys_data_update(CHANNEL *chp, dbref obj)
{
    /* Copy data from a MUX channel object to a new-style channel. */
    char *key;
    dbref aowner;
    int aflags, alen;
    key = atr_get(obj, A_LOCK, &aowner, &aflags, &alen);
    chp->join_lock = parse_boolexp(obj, key, 1);
    XFREE(key);
    key = atr_get(obj, A_LUSE, &aowner, &aflags, &alen);
    chp->trans_lock = parse_boolexp(obj, key, 1);
    XFREE(key);
    key = atr_get(obj, A_LENTER, &aowner, &aflags, &alen);
    chp->recv_lock = parse_boolexp(obj, key, 1);
    XFREE(key);
    key = atr_pget(obj, A_DESC, &aowner, &aflags, &alen);

    if (*key)
        chp->descrip = XSTRDUP(key, "comsys_data_update.chp->descrip");
    else
        chp->descrip = NULL;

    XFREE(key);
}

static void read_comsys(FILE *fp, int com_ver)
{
    CHANNEL *chp;
    COMALIAS *cap;
    COMLIST *clist;
    CHANWHO *wp;
    char c, *s, *s1, buf[LBUF_SIZE];
    int done;
    done = 0;
    c = getc(fp);

    if (c == '+') /* do we have any channels? */
        done = 1;

    ungetc(c, fp);

    /* Load up the channels */

    while (!done)
    {
        chp = (CHANNEL *)XMALLOC(sizeof(CHANNEL), "chp");
        if (!chp)
        {
            log_write(LOG_ALWAYS, "MEM", "COM", "Failed to allocate channel during database load");
            return;
        }
        chp->name = getstring(fp, 1);
        if (!chp->name)
        {
            log_write(LOG_ALWAYS, "DB", "COM", "Failed to read channel name from database");
            XFREE(chp);
            return;
        }
        chp->owner = getref(fp);

        if (!Good_obj(chp->owner) || !isPlayer(chp->owner))
            chp->owner = GOD; /* sanitize */

        chp->flags = getref(fp);

        if (com_ver == 1)
            comsys_flag_convert(chp);

        chp->charge = getref(fp);
        chp->charge_collected = getref(fp);
        chp->num_sent = getref(fp);
        chp->header = NULL;

        if (com_ver == 1)
        {
            comsys_data_update(chp, getref(fp));
        }
        else
        {
            s = getstring(fp, 1);

            if (s && *s)
                chp->descrip = s;
            else
                chp->descrip = NULL;

            if (com_ver > 3)
            {
                s = (char *)getstring(fp, 1);

                if (s && *s)
                    chp->header = s;
            }

            if (com_ver == 2)
            {
                /* Inherently broken behavior. Can't deal with eval locks,
                 * among other things.
                 */
                chp->join_lock = getboolexp1(fp);
                getc(fp); /* eat newline */
                chp->trans_lock = getboolexp1(fp);
                getc(fp); /* eat newline */
                chp->recv_lock = getboolexp1(fp);
                getc(fp); /* eat newline */
            }
            else
            {
                chp->join_lock = getboolexp1(fp);

                if (getc(fp) != '\n')
                {
                    /* Uh oh. Format error. Trudge on valiantly... probably
                     * won't work, but we can try.
                     */
                    log_write_raw(1, "Missing newline while reading join lock for channel %s\n", chp->name);
                }

                c = getc(fp);

                if (c == '\n')
                {
                    getc(fp); /* eat the dash on the next line */
                    getc(fp); /* eat the newline on the next line */
                }
                else if (c == '-')
                {
                    getc(fp); /* eat the next newline */
                }
                else
                {
                    log_write_raw(1, "Expected termination sequence while reading join lock for channel %s\n", chp->name);
                }

                chp->trans_lock = getboolexp1(fp);

                if (getc(fp) != '\n')
                {
                    log_write_raw(1, "Missing newline while reading transmit lock for channel %s\n", chp->name);
                }

                c = getc(fp);

                if (c == '\n')
                {
                    getc(fp); /* eat the dash on the next line */
                    getc(fp); /* eat the newline on the next line */
                }
                else if (c == '-')
                {
                    getc(fp); /* eat the next newline */
                }
                else
                {
                    log_write_raw(1, "Expected termination sequence while reading transmit lock for channel %s\n", chp->name);
                }

                chp->recv_lock = getboolexp1(fp);

                if (getc(fp) != '\n')
                {
                    log_write_raw(1, "Missing newline while reading receive lock for channel %s\n", chp->name);
                }

                c = getc(fp);

                if (c == '\n')
                {
                    getc(fp); /* eat the dash on the next line */
                    getc(fp); /* eat the newline on the next line */
                }
                else if (c == '-')
                {
                    getc(fp); /* eat the next newline */
                }
                else
                {
                    log_write_raw(1, "Expected termination sequence while reading receive lock for channel %s\n", chp->name);
                }
            }
        }
        if (!chp->header)
        {
            XSNPRINTF(buf, MBUF_SIZE, "[%s]", chp->name);
            chp->header = XSTRDUP(buf, "chp->header");
            if (!chp->header)
            {
                log_write(LOG_ALWAYS, "MEM", "COM", "Failed to allocate header for channel %s", chp->name);
                /* Clean up and skip this channel */
                XFREE(chp->name);
                if (chp->descrip)
                    XFREE(chp->descrip);
                if (chp->join_lock)
                    free_boolexp(chp->join_lock);
                if (chp->trans_lock)
                    free_boolexp(chp->trans_lock);
                if (chp->recv_lock)
                    free_boolexp(chp->recv_lock);
                XFREE(chp);
                XFREE(getstring(fp, 0));
                c = getc(fp);
                if (c == '+')
                    done = 1;
                ungetc(c, fp);
                continue;
            }
        }

        chp->who = NULL;
        chp->num_who = 0;
        chp->connect_who = NULL;
        chp->num_connected = 0;
        hashadd(chp->name, (int *)chp, &mod_comsys_comsys_htab, 0);
        XFREE(getstring(fp, 0)); /* discard the < */
        c = getc(fp);

        if (c == '+') /* look ahead for the end of the channels */
            done = 1;

        ungetc(c, fp);
    }

    XFREE(getstring(fp, 0)); /* discard the version string */
    done = 0;
    c = getc(fp);

    if (c == '*') /* do we have any aliases? */
        done = 1;

    ungetc(c, fp);

    /* Load up the aliases */

    while (!done)
    {
        cap = (COMALIAS *)XMALLOC(sizeof(COMALIAS), "cap");
        if (!cap)
        {
            log_write(LOG_ALWAYS, "MEM", "COM", "Failed to allocate channel alias during database load");
            return;
        }
        cap->player = getref(fp);
        s = getstring(fp, 1);
        if (!s)
        {
            log_write(LOG_ALWAYS, "DB", "COM", "Failed to read channel name for alias");
            XFREE(cap);
            return;
        }
        cap->channel = lookup_channel(s);
        XFREE(s);
        if (!cap->channel)
        {
            log_write(LOG_ALWAYS, "DB", "COM", "Channel not found for alias, skipping");
            /* Skip this alias - read rest of data and continue */
            XFREE(getstring(fp, 1)); /* alias */
            XFREE(getstring(fp, 1)); /* title */
            getref(fp);              /* is_listening */
            XFREE(getstring(fp, 0)); /* < marker */
            XFREE(cap);
            c = getc(fp);
            if (c == '*')
                done = 1;
            ungetc(c, fp);
            continue;
        }
        cap->alias = getstring(fp, 1);
        if (!cap->alias)
        {
            log_write(LOG_ALWAYS, "DB", "COM", "Failed to read alias string");
            XFREE(cap);
            return;
        }
        s = getstring(fp, 1);

        if (s && *s)
            cap->title = s;
        else
            cap->title = NULL;

        s1 = XMALLOC(LBUF_SIZE, "s1");
        if (!s1)
        {
            log_write(LOG_ALWAYS, "MEM", "COM", "Failed to allocate hash key string during database load");
            if (cap->title)
                XFREE(cap->title);
            XFREE(cap->alias);
            XFREE(cap);
            return;
        }
        XSNPRINTF(s1, LBUF_SIZE, "%d.%s", cap->player, cap->alias);
        hashadd(s1, (int *)cap, &mod_comsys_calias_htab, 0);
        XFREE(s1);

        clist = (COMLIST *)XMALLOC(sizeof(COMLIST), "clist");
        if (!clist)
        {
            log_write(LOG_ALWAYS, "MEM", "COM", "Failed to allocate comlist during database load");
            /* Must remove from hash table before freeing */
            s1 = XMALLOC(LBUF_SIZE, "s1");
            if (s1)
            {
                XSNPRINTF(s1, LBUF_SIZE, "%d.%s", cap->player, cap->alias);
                hashdelete(s1, &mod_comsys_calias_htab);
                XFREE(s1);
            }
            if (cap->title)
                XFREE(cap->title);
            XFREE(cap->alias);
            XFREE(cap);
            return;
        }
        clist->alias_ptr = cap;
        clist->next = lookup_clist(cap->player);

        if (clist->next == NULL)
            nhashadd((int)cap->player, (int *)clist, &mod_comsys_comlist_htab);
        else
            nhashrepl((int)cap->player, (int *)clist,
                      &mod_comsys_comlist_htab);

        if (!is_onchannel(cap->player, cap->channel))
        {
            wp = (CHANWHO *)XMALLOC(sizeof(CHANWHO), "wp");
            if (!wp)
            {
                log_write(LOG_ALWAYS, "MEM", "COM", "Failed to allocate CHANWHO during database load");
                /* Note: We've already added to hash tables, accept the leak */
                getref(fp); /* consume is_listening */
                XFREE(getstring(fp, 0));
                c = getc(fp);
                if (c == '*')
                    done = 1;
                ungetc(c, fp);
                continue;
            }
            wp->player = cap->player;
            wp->is_listening = getref(fp);

            if ((cap->channel->num_who == 0) || (cap->channel->who == NULL))
            {
                wp->next = NULL;
            }
            else
            {
                wp->next = cap->channel->who;
            }

            cap->channel->who = wp;
            if (cap->channel->num_who < INT_MAX)
                cap->channel->num_who++;
        }
        else
        {
            getref(fp); /* toss the value */
        }

        XFREE(getstring(fp, 0)); /* discard the < */
        c = getc(fp);

        if (c == '*') /* look ahead for the end of the aliases */
            done = 1;

        ungetc(c, fp);
    }

    s = (char *)getstring(fp, 0);

    if (strcmp(s, (char *)"*** END OF DUMP ***"))
    {
        log_write(LOG_STARTUP, "INI", "COM", "Aborted load on unexpected line: %s", s);
    }
    XFREE(s);
}

static void sanitize_comsys(void)
{
    /* Because we can run into situations where the comsys db and
     * regular database are not in sync (ex: restore from backup),
     * we need to sanitize the comsys data structures at load time.
     * The comlists we have represent the dbrefs of objects on channels.
     * Thus we can just look at what objects are there, and work
     * accordingly.
     */
    int i, count;
    HASHTAB *htab;
    HASHENT *hptr;
    int *ptab;
    count = 0;
    htab = &mod_comsys_comlist_htab;
    ptab = (int *)XCALLOC(htab->entries, sizeof(int), "ptab");
    if (!ptab)
    {
        return;
    }

    for (i = 0; i < htab->hashsize; i++)
    {
        for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next)
        {
            if (!Good_obj(hptr->target.i))
            {
                if (count >= htab->entries)
                {
                    log_write(LOG_BUGS, "BUG", "CHR", "ptab overflow in sanitize_comsys");
                    break;
                }

                ptab[count] = hptr->target.i;
                count++;
            }
        }
    }

    /* Have to do this separately, so we don't mess up the hashtable
     * linking while we're trying to traverse it.
     */

    for (i = 0; i < count; i++)
        channel_clr(ptab[i]);

    XFREE(ptab);
}

void mod_comsys_make_minimal(void)
{
    CHANNEL *chp;
    do_ccreate(GOD, GOD, 0, mod_comsys_config.public_channel);
    chp = lookup_channel(mod_comsys_config.public_channel);

    if (chp)
    { /* should always be true, but be safe */
        chp->flags |= CHAN_FLAG_PUBLIC;
    }

    do_ccreate(GOD, GOD, 0, mod_comsys_config.guests_channel);
    chp = lookup_channel(mod_comsys_config.guests_channel);

    if (chp)
    { /* should always be true, but be safe */
        chp->flags |= CHAN_FLAG_PUBLIC;
    }
}

void mod_comsys_load_database(FILE *fp)
{
    char buffer[2 * MBUF_SIZE + 8]; /* depends on max length of params */

    if (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (!strncmp(buffer, (char *)"+V", 2))
        {
            char *endptr;
            long version;

            errno = 0;
            version = strtol(buffer + 2, &endptr, 10);

            if (errno == 0 && version >= 0 && version <= INT_MAX)
                read_comsys(fp, (int)version);
            else
                log_write(LOG_STARTUP, "INI", "COM", "Invalid comsys version.");

            sanitize_comsys();
        }
        else
        {
            log_write(LOG_STARTUP, "INI", "COM", "Unrecognized comsys format.");
            mod_comsys_make_minimal();
        }
    }
}

/* --------------------------------------------------------------------------
 * User functions.
 */

#define Grab_Channel(p)                                              \
    chp = lookup_channel(fargs[0]);                                  \
    if (!chp)                                                        \
    {                                                                \
        XSAFELBSTR((char *)"#-1 CHANNEL NOT FOUND", buff, bufc);    \
        return;                                                      \
    }                                                                \
    if ((!Comm_All(p) && ((p) != chp->owner)))                       \
    {                                                                \
        XSAFELBSTR((char *)"#-1 NO PERMISSION TO USE", buff, bufc); \
        return;                                                      \
    }

#define Comsys_User(p, t)                                            \
    t = lookup_player(p, fargs[0], 1);                               \
    if (!Good_obj(t) || (!Controls(p, t) && !Comm_All(p)))           \
    {                                                                \
        XSAFELBSTR((char *)"#-1 NO PERMISSION TO USE", buff, bufc); \
        return;                                                      \
    }

#define Grab_Alias(p, n)                                      \
    cap = lookup_calias(p, n);                                \
    if (!cap)                                                 \
    {                                                         \
        XSAFELBSTR((char *)"#-1 NO SUCH ALIAS", buff, bufc); \
        return;                                               \
    }

void fun_comlist(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    CHANNEL *chp;
    char *bb_p;
    Delim osep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1 - 1, 1, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    bb_p = *bufc;

    for (chp = (CHANNEL *)hash_firstentry(&mod_comsys_comsys_htab);
         chp != NULL;
         chp = (CHANNEL *)hash_nextentry(&mod_comsys_comsys_htab))
    {
        if ((chp->flags & CHAN_FLAG_PUBLIC) ||
            Comm_All(player) || (chp->owner == player))
        {
            if (*bufc != bb_p)
            {
                print_separator(&osep, buff, bufc);
            }

            XSAFELBSTR(chp->name, buff, bufc);
        }
    }
}

void fun_cwho(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    CHANNEL *chp;
    char *bb_p;
    int i;
    Grab_Channel(player);
    bb_p = *bufc;

    if (!chp->connect_who)
        return;

    for (i = 0; i < chp->num_connected; i++)
    {
        if (!chp->connect_who[i])
            continue;

        if (chp->connect_who[i]->is_listening &&
            (!isPlayer(chp->connect_who[i]->player) ||
             (Connected(chp->connect_who[i]->player) &&
              (!Hidden(chp->connect_who[i]->player) || See_Hidden(player)))))
        {
            if (*bufc != bb_p)
                XSAFELBCHR(' ', buff, bufc);

            XSAFELBCHR('#', buff, bufc);
            XSAFELTOS(buff, bufc, chp->connect_who[i]->player, LBUF_SIZE);
        }
    }
}

void fun_cwhoall(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    CHANNEL *chp;
    CHANWHO *wp;
    char *bb_p;
    Grab_Channel(player);
    bb_p = *bufc;

    for (wp = chp->who; wp != NULL; wp = wp->next)
    {
        if (*bufc != bb_p)
            XSAFELBCHR(' ', buff, bufc);

        XSAFELBCHR('#', buff, bufc);
        XSAFELTOS(buff, bufc, wp->player, LBUF_SIZE);
    }
}

void fun_comowner(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    CHANNEL *chp;
    Grab_Channel(player);
    XSAFELBCHR('#', buff, bufc);
    XSAFELTOS(buff, bufc, chp->owner, LBUF_SIZE);
}

void fun_comdesc(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    CHANNEL *chp;
    Grab_Channel(player);

    if (chp->descrip)
        XSAFELBSTR(chp->descrip, buff, bufc);
}

void fun_comheader(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    CHANNEL *chp;
    Grab_Channel(player);

    if (chp->header)
        XSAFELBSTR(chp->header, buff, bufc);
}

void fun_comalias(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref target;
    COMLIST *clist, *cl_ptr;
    char *bb_p;
    Comsys_User(player, target);
    clist = lookup_clist(target);

    if (!clist)
        return;

    bb_p = *bufc;

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next)
    {
        /* Bug #75: Check for NULL alias_ptr to handle database corruption */
        if (!cl_ptr->alias_ptr)
            continue;

        if (*bufc != bb_p)
            XSAFELBCHR(' ', buff, bufc);

        XSAFELBSTR(cl_ptr->alias_ptr->alias, buff, bufc);
    }
}

void fun_cominfo(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref target;
    COMALIAS *cap;
    Comsys_User(player, target);
    Grab_Alias(target, fargs[1]);
    if (cap->channel)
        XSAFELBSTR(cap->channel->name, buff, bufc);
    else
        XSAFELBSTR((char *)"#-1 INVALID CHANNEL", buff, bufc);
}

void fun_comtitle(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    dbref target;
    COMALIAS *cap;
    Comsys_User(player, target);
    Grab_Alias(target, fargs[1]);

    if (cap->title)
        XSAFELBSTR(cap->title, buff, bufc);
}

void fun_cemit(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    CHANNEL *chp;
    Grab_Channel(player);
    com_message(chp, fargs[1], player);
}

FUN mod_comsys_functable[] = {
    {"CEMIT", fun_cemit, 2, 0, CA_PUBLIC, NULL},
    {"COMALIAS", fun_comalias, 1, 0, CA_PUBLIC, NULL},
    {"COMDESC", fun_comdesc, 1, 0, CA_PUBLIC, NULL},
    {"COMHEADER", fun_comheader, 1, 0, CA_PUBLIC, NULL},
    {"COMINFO", fun_cominfo, 2, 0, CA_PUBLIC, NULL},
    {"COMLIST", fun_comlist, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"COMOWNER", fun_comowner, 1, 0, CA_PUBLIC, NULL},
    {"COMTITLE", fun_comtitle, 2, 0, CA_PUBLIC, NULL},
    {"CWHO", fun_cwho, 1, 0, CA_PUBLIC, NULL},
    {"CWHOALL", fun_cwhoall, 1, 0, CA_PUBLIC, NULL},
    {NULL, NULL, 0, 0, 0, NULL}};

/* --------------------------------------------------------------------------
 * Initialization.
 */

void mod_comsys_cleanup_startup(void)
{
    update_comwho_all();
}

void mod_comsys_create_player(dbref creator __attribute__((unused)), dbref player, int isrobot __attribute__((unused)), int isguest)
{
    if (isguest && (player != 1))
    {
        if (*mod_comsys_config.guests_channel)
            join_channel(player, mod_comsys_config.guests_channel,
                         mod_comsys_config.guests_calias, NULL);
    }
    else if (player != 1)
    { /* avoid problems with minimal db */
        if (*mod_comsys_config.public_channel)
            join_channel(player, mod_comsys_config.public_channel,
                         mod_comsys_config.public_calias, NULL);
    }
}

void mod_comsys_destroy_obj(dbref player __attribute__((unused)), dbref obj)
{
    channel_clr(obj);
}

void mod_comsys_destroy_player(dbref player, dbref victim)
{
    if (Good_obj(player))
        comsys_chown(victim, Owner(player));
}

void mod_comsys_init(void)
{

    mod_comsys_config.public_channel = XSTRDUP("Public", "mod_comsys_init.mod_comsys_config.public_channel");
    if (!mod_comsys_config.public_channel)
        mod_comsys_config.public_channel = (char *)"Public";

    mod_comsys_config.guests_channel = XSTRDUP("Guests", "mod_comsys_init.mod_comsys_config.guests_channel");
    if (!mod_comsys_config.guests_channel)
        mod_comsys_config.guests_channel = (char *)"Guests";

    mod_comsys_config.public_calias = XSTRDUP("pub", "mod_comsys_init.mod_comsys_config.public_calias");
    if (!mod_comsys_config.public_calias)
        mod_comsys_config.public_calias = (char *)"pub";

    mod_comsys_config.guests_calias = XSTRDUP("g", "mod_comsys_init.mod_comsys_config.guests_calias");
    if (!mod_comsys_config.guests_calias)
        mod_comsys_config.guests_calias = (char *)"g";

    mod_comsys_version.version = XSTRDUP(mushstate.version.versioninfo, "mod_comsys_init.mod_comsys_version.version");
    if (!mod_comsys_version.version)
        mod_comsys_version.version = (char *)"unknown";

    mod_comsys_version.author = XSTRDUP(TINYMUSH_AUTHOR, "mod_comsys_init.mod_comsys_version.author");
    if (!mod_comsys_version.author)
        mod_comsys_version.author = (char *)"unknown";

    mod_comsys_version.email = XSTRDUP(TINYMUSH_CONTACT, "mod_comsys_init.mod_comsys_version.email");
    if (!mod_comsys_version.email)
        mod_comsys_version.email = (char *)"unknown";

    mod_comsys_version.url = XSTRDUP(TINYMUSH_HOMEPAGE_URL, "mod_comsys_init.mod_comsys_version.url");
    if (!mod_comsys_version.url)
        mod_comsys_version.url = (char *)"unknown";

    mod_comsys_version.description = XSTRDUP("Communication system for TinyMUSH", "mod_comsys_init.mod_comsys_version.description");
    if (!mod_comsys_version.description)
        mod_comsys_version.description = (char *)"Communication system for TinyMUSH";

    mod_comsys_version.copyright = XSTRDUP(TINYMUSH_COPYRIGHT, "mod_comsys_init.mod_comsys_version.copyright");
    if (!mod_comsys_version.copyright)
        mod_comsys_version.copyright = (char *)"unknown";

    register_hashtables(mod_comsys_hashtable, mod_comsys_nhashtable);
    register_commands(mod_comsys_cmdtable);
    register_functions(mod_comsys_functable);
}
