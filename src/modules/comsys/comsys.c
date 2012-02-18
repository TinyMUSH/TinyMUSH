/* comsys.c - module implementing DarkZone-style channel system */
/* $Id: comsys.c,v 1.81 2008/01/10 00:04:18 lwl Exp $ */

#include "../../api.h"

extern BOOLEXP *FDECL(getboolexp1, (FILE *));
extern void FDECL(putboolexp, (FILE *, BOOLEXP *));

/* --------------------------------------------------------------------------
 * Constants.
 */

#define NO_CHAN_MSG "That is not a valid channel name."

#define CHAN_FLAG_PUBLIC	0x00000010
#define CHAN_FLAG_LOUD		0x00000020
#define CHAN_FLAG_P_JOIN	0x00000040
#define CHAN_FLAG_P_TRANS	0x00000080
#define CHAN_FLAG_P_RECV	0x00000100
#define CHAN_FLAG_O_JOIN	0x00000200
#define CHAN_FLAG_O_TRANS	0x00000400
#define CHAN_FLAG_O_RECV	0x00000800
#define CHAN_FLAG_SPOOF		0x00001000

#define CBOOT_QUIET	1	/* No boot message, just has left */
#define CEMIT_NOHEADER  1	/* Channel emit without header */
#define CHANNEL_SET	1	/* Set channel flag */
#define CHANNEL_CHARGE	2	/* Set channel charge */
#define CHANNEL_DESC	4	/* Set channel desc */
#define CHANNEL_LOCK	8	/* Set channel lock */
#define CHANNEL_OWNER	16	/* Set channel owner */
#define CHANNEL_JOIN	32	/* Channel lock: join */
#define CHANNEL_TRANS	64	/* Channel lock: transmit */
#define CHANNEL_RECV	128	/* Channel lock: receive */
#define CHANNEL_HEADER	256	/* Set channel header */
#define CLIST_FULL	1	/* Full listing of channels */
#define CLIST_HEADER	2	/* Header listing of channels */
#define CWHO_ALL	1	/* Show disconnected players on channel */

#define MAX_CHAN_NAME_LEN	20
#define MAX_CHAN_ALIAS_LEN	10
#define MAX_CHAN_DESC_LEN	256
#define MAX_CHAN_HEAD_LEN	64

/* --------------------------------------------------------------------------
 * Configuration and hash tables.
 */

struct mod_comsys_confstorage {
	char	*public_channel;	/* Name of public channel */
	char	*guests_channel;	/* Name of guests channel */
	char	*public_calias;		/* Alias of public channel */
	char	*guests_calias;		/* Alias of guests channel */
} mod_comsys_config;

CONF mod_comsys_conftable[] = {
{(char *)"guests_calias",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mod_comsys_config.guests_calias,	SBUF_SIZE},
{(char *)"guests_channel",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mod_comsys_config.guests_channel,	SBUF_SIZE},
{(char *)"public_calias",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mod_comsys_config.public_calias,	SBUF_SIZE},
{(char *)"public_channel",		cf_string,	CA_STATIC,	CA_PUBLIC,	(int *)&mod_comsys_config.public_channel,	SBUF_SIZE},
{ NULL,					NULL,		0,		0,		NULL,				0}};

HASHTAB mod_comsys_comsys_htab;
HASHTAB mod_comsys_calias_htab;
NHSHTAB mod_comsys_comlist_htab;

MODHASHES mod_comsys_hashtable[] = {
{ "Channels",		&mod_comsys_comsys_htab,	15,	8},
{ "Channel aliases",	&mod_comsys_calias_htab,	500,	16},
{ NULL,			NULL,				0,	0}};

MODNHASHES mod_comsys_nhashtable[] = {
{ "Channel lists",	&mod_comsys_comlist_htab,	100,	16},
{ NULL,			NULL,				0,	0}};

/* --------------------------------------------------------------------------
 * Structure definitions.
 */

typedef struct com_player CHANWHO;
struct com_player {
    dbref player;
    int is_listening;
    CHANWHO *next;
};

typedef struct com_channel CHANNEL;
struct com_channel {
    char *name;
    dbref owner;
    unsigned int flags;
    char *header;		/* channel header prefixing messages */
    int num_who;		/* number of people on the channel */
    CHANWHO *who;		/* linked list of players on channel */
    int num_connected;		/* number of connected players on channel */
    CHANWHO **connect_who;	/* array of connected player who structs */
    int charge;			/* cost to use channel */
    int charge_collected;	/* amount paid thus far */
    int num_sent;		/* number of messages sent */
    char *descrip;		/* description */
    BOOLEXP *join_lock;		/* who can join */
    BOOLEXP *trans_lock;	/* who can transmit */
    BOOLEXP *recv_lock;		/* who can receive */
};

typedef struct com_alias COMALIAS;
struct com_alias {
    dbref player;
    char *alias;
    char *title;
    CHANNEL *channel;
};

typedef struct com_list COMLIST;
struct com_list {
    COMALIAS *alias_ptr;
    COMLIST *next;
};

/* --------------------------------------------------------------------------
 * Macros.
 */

#define check_owned_channel(p,c) \
if (!Comm_All((p)) && ((p) != (c)->owner)) { \
    notify((p), NOPERM_MESSAGE); \
    return; \
}

#define find_channel(d,n,p) \
(p) = ((CHANNEL *) hashfind((n), &mod_comsys_comsys_htab)); \
if (!(p)) { \
    notify((d), NO_CHAN_MSG); \
    return; \
}

#define find_calias(d,a,p) \
(p)=((COMALIAS *) hashfind(tprintf("%d.%s",(d),(a)), &mod_comsys_calias_htab)); \
if (!(p)) { \
    notify((d), "No such channel alias."); \
    return; \
}

#define lookup_channel(s)  ((CHANNEL *) hashfind((s), &mod_comsys_comsys_htab))

#define lookup_calias(d,s)  \
((COMALIAS *) hashfind(tprintf("%d.%s",(d),(s)), &mod_comsys_calias_htab))

#define lookup_clist(d) \
((COMLIST *) nhashfind((int) (d), &mod_comsys_comlist_htab))

#define ok_joinchannel(d,c) \
ok_chanperms((d),(c),CHAN_FLAG_P_JOIN,CHAN_FLAG_O_JOIN,(c)->join_lock)

#define ok_recvchannel(d,c) \
ok_chanperms((d),(c),CHAN_FLAG_P_RECV,CHAN_FLAG_O_RECV,(c)->recv_lock)

#define ok_sendchannel(d,c) \
ok_chanperms((d),(c),CHAN_FLAG_P_TRANS,CHAN_FLAG_O_TRANS,(c)->trans_lock)

#define clear_chan_alias(n,a) \
XFREE((a)->alias, "clear_chan_alias.astring"); \
if ((a)->title) \
    XFREE((a)->title, "clear_chan_alias.title"); \
XFREE((a), "clear_chan_alias.alias"); \
hashdelete((n), &mod_comsys_calias_htab)

/* --------------------------------------------------------------------------
 * Basic channel utilities.
 */

INLINE static int is_onchannel(player, chp)
    dbref player;
    CHANNEL *chp;
{
    CHANWHO *wp;

    for (wp = chp->who; wp != NULL; wp = wp->next) {
	if (wp->player == player) 
	    return 1;
    }

    return 0;
}

INLINE static int is_listenchannel(player, chp)
    dbref player;
    CHANNEL *chp;
{
    int i;

    for (i = 0; i < chp->num_connected; i++) {
	if (chp->connect_who[i]->player == player)
	    return (chp->connect_who[i]->is_listening);
    }

    return 0;
}


INLINE static int is_listening_disconn(player, chp)
    dbref player;
    CHANNEL *chp;
{
    CHANWHO *wp;

    for (wp = chp->who; wp != NULL; wp = wp->next) {
	if (wp->player == player)
	    return (wp->is_listening);
    }

    return 0;
}


static int ok_channel_string(str, maxlen, ok_spaces, ok_ansi)
    char *str;
    int maxlen;
    int ok_spaces, ok_ansi;
{
    char *p;

    if (!str || !*str)
	return 0;

    if ((int)strlen(str) > maxlen - 1)
	return 0;

    for (p = str; *p; p++) {
	if ((!ok_spaces && isspace(*p)) ||
	    (!ok_ansi && (*p == ESC_CHAR))) {
	    return 0;
	}
    }

    return 1;
}

INLINE static char *munge_comtitle(title)
    char *title;
{
    static char tbuf[MBUF_SIZE];
    char *tp;

    tp = tbuf;

    if (strchr(title, ESC_CHAR)) {
	safe_copy_str(title, tbuf, &tp, MBUF_SIZE - 5);
	safe_mb_str(ANSI_NORMAL, tbuf, &tp);
    } else {
	safe_mb_str(title, tbuf, &tp);
    }

    return tbuf;
}

INLINE static int ok_chanperms(player, chp, pflag, oflag, c_lock)
    dbref player;
    CHANNEL *chp;
    int pflag, oflag;
    BOOLEXP *c_lock;
{
    if (Comm_All(player))
	return 1;

    switch (Typeof(player)) {
	case TYPE_PLAYER:
	    if (chp->flags & pflag)
		return 1;
	    break;
	case TYPE_THING:
	    if (chp->flags & oflag)
		return 1;
	    break;
	default:		/* only players and things on channels */
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


/* --------------------------------------------------------------------------
 * More complex utilities.
 */

static void update_comwho(chp)
    CHANNEL *chp;
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

    for (wp = chp->who; wp != NULL; wp = wp->next) {
	if (!isPlayer(wp->player) || Connected(wp->player))
	    count++;
    }

    if (chp->connect_who)
	XFREE(chp->connect_who, "update_comwho");

    chp->num_connected = count;

    if (count > 0) {
	chp->connect_who = (CHANWHO **) XCALLOC(count, sizeof(CHANWHO *), "update_comwho");
	i = 0;
	for (wp = chp->who; wp != NULL; wp = wp->next) {
	    if (!isPlayer(wp->player) || Connected(wp->player)) {
		chp->connect_who[i] = wp;
		i++;
	    }
	}
    } else {
	 chp->connect_who = NULL;
    }
}

static void com_message(chp, msg, cause)
    CHANNEL *chp;
    char *msg;
    dbref cause;
{
    int i;
    CHANWHO *wp;
    char *mp, msg_ns[LBUF_SIZE];
#ifdef PUEBLO_SUPPORT
    char *mh, *mh_ns;

    mh = mh_ns = NULL;
#endif

    chp->num_sent++;
    mp = NULL;

    for (i = 0; i < chp->num_connected; i++) {
	wp = chp->connect_who[i];
	if (wp->is_listening && ok_recvchannel(wp->player, chp)) {
	    if (isPlayer(wp->player)) {
		if (Nospoof(wp->player) && (wp->player != cause) &&
		    (wp->player != mudstate.curr_enactor) &&
		    (wp->player != mudstate.curr_player)) {
		    if (!mp) {
			/* Construct Nospoof buffer. Can't use tprintf
			 * because we end up calling it later.
			 */
			mp = msg_ns;
			safe_chr('[', msg_ns, &mp);
			safe_name(cause, msg_ns, &mp);
			safe_chr('(', msg_ns, &mp);
			safe_chr('#', msg_ns, &mp);
			safe_ltos(msg_ns, &mp, cause);
			safe_chr(')', msg_ns, &mp);
			if (cause != Owner(cause)) {
			    safe_chr('{', msg_ns, &mp);
			    safe_name(Owner(cause), msg_ns, &mp);
			    safe_chr('}', msg_ns, &mp);
			}
			if (cause != mudstate.curr_enactor) {
			    safe_known_str((char *) "<-(#", 4, msg_ns, &mp);
			    safe_ltos(msg_ns, &mp, cause);
			    safe_chr(')', msg_ns, &mp);
			}
			safe_known_str((char *) "] ", 2, msg_ns, &mp);
			safe_str(msg, msg_ns, &mp);
		    }
#ifndef PUEBLO_SUPPORT
		    raw_notify(wp->player, msg_ns);
#else
		    if (Html(wp->player)) {
			if (!mh_ns) {
			    mh_ns = alloc_lbuf("com_message.html.nospoof");
 			    html_escape(msg_ns, mh_ns, 0);
			}
			raw_notify(wp->player, mh_ns);
		    } else {
			raw_notify(wp->player, msg_ns);
		    }
#endif
		} else {
#ifndef PUEBLO_SUPPORT
		    raw_notify(wp->player, msg);
#else
		    if (Html(wp->player)) {
			if (!mh) {
			    mh = alloc_lbuf("com_message.html");
 			    html_escape(msg, mh, 0);
			}
			raw_notify(wp->player, mh);
		    } else {
			raw_notify(wp->player, msg);
		    }
#endif
		}
	    } else {
		notify_with_cause(wp->player, cause, msg);
	    }
	}
    }
#ifdef PUEBLO_SUPPORT
    if (mh)
	free_lbuf(mh);
    if (mh_ns)
	free_lbuf(mh_ns);
#endif
}

static void remove_from_channel(player, chp, is_quiet)
    dbref player;
    CHANNEL *chp;
    int is_quiet;
{
    /* We assume that the player's channel aliases have already been
     * removed, and that other cleanup that is not directly related to
     * the channel structure itself has been accomplished. (We also
     * do no sanity-checking.)
     */

    CHANWHO *wp, *prev;

    /* Should never happen, but just in case... */

    if ((chp->num_who == 0) || !chp->who)
	return;

    /* If we only had one person, we can just nuke stuff. */

    if (chp->num_who == 1) {
	chp->num_who = 0;
	XFREE(chp->who, "remove_from_channel.who");
	chp->num_connected = 0;
	XFREE(chp->connect_who, "remove_from_channel.connect_who");
	return;
    }

    for (wp = chp->who, prev = NULL; wp != NULL; wp = wp->next) {
	if (wp->player == player) {
	    if (prev) {
		prev->next = wp->next;
	    } else {
		chp->who = wp->next;
	    }
	    XFREE(wp, "remove_from_channel.who");
	    break;
	} else {
	    prev = wp;
	}
    }

    chp->num_who--;

    update_comwho(chp);

    if (!is_quiet &&
	(!isPlayer(player) || (Connected(player) && !Hidden(player)))) {
	com_message(chp, tprintf("%s %s has left this channel.",
				 chp->header, Name(player)),
		    player);
    }
}


INLINE static void zorch_alias_from_list(cap)
    COMALIAS *cap;
{
    COMLIST *clist, *cl_ptr, *prev;

    clist = lookup_clist(cap->player);
    if (!clist)
	return;

    prev = NULL;
    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next) {
	if (cl_ptr->alias_ptr == cap) {
	    if (prev)
		prev->next = cl_ptr->next;
	    else {
		clist = cl_ptr->next;
		if (clist)
		    nhashrepl((int) cap->player, (int *) clist,
			      &mod_comsys_comlist_htab);
		else
		    nhashdelete((int) cap->player, &mod_comsys_comlist_htab);
	    }
	    XFREE(cl_ptr, "zorch_alias.cl_ptr");
	    return;
	}
	prev = cl_ptr;
    }
}

static void process_comsys(player, arg, cap)
    dbref player;
    char *arg;
    COMALIAS *cap;
{
    CHANWHO *wp;
    char *buff, *name_buf, tbuf[LBUF_SIZE], *tp;
    int i;

    if (!arg || !*arg) {
	notify(player, "No message.");
	return;
    }

    if (!strcmp(arg, (char *) "on")) {

	for (wp = cap->channel->who; wp != NULL; wp = wp->next) {
	    if (wp->player == player)
		break;
	}
	if (!wp) {
	    STARTLOG(LOG_ALWAYS, "BUG", "COM")
	      log_printf("Object #%d with alias %s is on channel %s but not on its player list.", player, cap->alias, cap->channel->name);
	    ENDLOG
	    notify(player, "An unusual channel error has been detected.");
	    return;
	}
	if (wp->is_listening) {
	    notify(player, tprintf("You are already on channel %s.",
				   cap->channel->name));
	    return;
	}
	wp->is_listening = 1;

	/* Only tell people that we've joined if we're an object, or
	 * we're a connected and non-hidden player.
	 */
	if (!isPlayer(player) || (Connected(player) && !Hidden(player))) {
	    com_message(cap->channel,
			tprintf("%s %s has joined this channel.",
				cap->channel->header, Name(player)),
			player);
	}
	return;
    
    } else if (!strcmp(arg, (char *) "off")) {

	for (wp = cap->channel->who; wp != NULL; wp = wp->next) {
	    if (wp->player == player)
		break;
	}
	if (!wp) {
	    STARTLOG(LOG_ALWAYS, "BUG", "COM")
	      log_printf("Object #%d with alias %s is on channel %s but not on its player list.", player, cap->alias, cap->channel->name);
	    ENDLOG
	    notify(player, "An unusual channel error has been detected.");
	    return;
	}
	if (wp->is_listening == 0) {
	    notify(player, tprintf("You are not on channel %s.",
				   cap->channel->name));
	    return;
	}
	wp->is_listening = 0;
	notify(player, tprintf("You leave channel %s.", cap->channel->name));

	/* Only tell people about it if we're an object, or we're a 
	 * connected and non-hidden player.
	 */
	if (!isPlayer(player) || (Connected(player) && !Hidden(player))) {
	    com_message(cap->channel,
			tprintf("%s %s has left this channel.",
				cap->channel->header, Name(player)),
			player);
	}
	return;

    } else if (!strcmp(arg, (char *) "who")) {

	/* Allow players who have an alias for a channel to see who is
	 * on it, even if they are not actively receiving.
	 */

	notify(player, "-- Players --");

	for (i = 0; i < cap->channel->num_connected; i++) {
	    wp = cap->channel->connect_who[i];
	    if (isPlayer(wp->player)) {
		if (wp->is_listening && Connected(wp->player) &&
		    (!Hidden(wp->player) || See_Hidden(player))) {
		    buff = unparse_object(player, wp->player, 0);
		    notify(player, buff);
		    free_lbuf(buff);
		}
	    }
	}

	notify(player, "-- Objects -- ");

	for (i = 0; i < cap->channel->num_connected; i++) {
	    wp = cap->channel->connect_who[i];
	    if (!isPlayer(wp->player)) {
		if (wp->is_listening) {
		    buff = unparse_object(player, wp->player, 0);
		    notify(player, buff);
		    free_lbuf(buff);
		}
	    }
	}

	notify(player, tprintf("-- %s --", cap->channel->name));

	return;

    } else {

	if (Gagged(player)) {
	    notify(player, NOPERM_MESSAGE);
	    return;
	}

	if (!is_listenchannel(player, cap->channel)) {
	    notify(player, tprintf("You must be on %s to do that.",
				   cap->channel->name));
	    return;
	}

	if (!ok_sendchannel(player, cap->channel)) {
	    notify(player, "You cannot transmit on that channel.");
	    return;
	}

	if (!payfor(player, Guest(player) ? 0 : cap->channel->charge)) {
	    notify(player, tprintf("You don't have enough %s.",
				   mudconf.many_coins));
	    return;
	}
	cap->channel->charge_collected += cap->channel->charge;
	giveto(cap->channel->owner, cap->channel->charge);

	if (cap->title) {
	    if (cap->channel->flags & CHAN_FLAG_SPOOF) {
		name_buf = cap->title;
	    } else {
		tp = tbuf;
		safe_str(cap->title, tbuf, &tp);
		safe_chr(' ', tbuf, &tp);
		safe_name(player, tbuf, &tp);
		*tp = '\0';
		name_buf = tbuf;
	    }
	} else {
	    name_buf = NULL;
	}
	if (*arg == ':') {
	    com_message(cap->channel,
			tprintf("%s %s %s",
				cap->channel->header,
				(name_buf) ? name_buf : Name(player),
				arg + 1),
			player);
	} else if (*arg == ';') {
	    com_message(cap->channel,
			tprintf("%s %s%s",
				cap->channel->header,
				(name_buf) ? name_buf : Name(player),
				arg + 1),
			player);
	} else {
	    com_message(cap->channel,
			tprintf("%s %s says, \"%s\"",
				cap->channel->header,
				(name_buf) ? name_buf : Name(player),
				arg),
			player);
	}

	return;
    }
}

/* --------------------------------------------------------------------------
 * Other externally-exposed utilities.
 */

void join_channel(player, chan_name, alias_str, title_str)
    dbref player;
    char *chan_name, *alias_str, *title_str;
{
    CHANNEL *chp;
    COMALIAS *cap;
    CHANWHO *wp;
    COMLIST *clist;
    int has_joined;

    if (!ok_channel_string(alias_str, MAX_CHAN_ALIAS_LEN, 0, 0)) {
	notify(player, "That is not a valid channel alias.");
	return;
    }
    
    if (lookup_calias(player, alias_str) != NULL) {
	notify(player, "You are already using that channel alias.");
	return;
    }

    find_channel(player, chan_name, chp);

    has_joined = is_onchannel(player, chp);
    if (!has_joined && !ok_joinchannel(player, chp)) {
	notify(player, "You cannot join that channel.");
	return;
    }

    /* Construct the alias. */

    cap = (COMALIAS *) XMALLOC(sizeof(COMALIAS), "join_channel.alias");
    cap->player = player;
    cap->alias = XSTRDUP(alias_str, "join_channel.alias_str");

    /* Note that even if the player is already on this channel,
     * we do not inherit the channel title from other aliases.
     */
    if (title_str && *title_str)
	cap->title = XSTRDUP(munge_comtitle(title_str),
			     "join_channel.title_str");
    else
	cap->title = NULL;
    cap->channel = chp;

    hashadd(tprintf("%d.%s", player, alias_str), (int *) cap,
	    &mod_comsys_calias_htab, 0);

    /* Add this to the list of all aliases for the player. */

    clist = (COMLIST *) XMALLOC(sizeof(COMLIST), "join_channel.clist");
    clist->alias_ptr = cap;
    clist->next = lookup_clist(player);
    if (clist->next == NULL)
	nhashadd((int) player, (int *) clist, &mod_comsys_comlist_htab);
    else
	nhashrepl((int) player, (int *) clist, &mod_comsys_comlist_htab);

    /* If we haven't joined the channel, go do that. */

    if (!has_joined) {

	wp = (CHANWHO *) XMALLOC(sizeof(CHANWHO), "join_channel.who");
	wp->player = player;
	wp->is_listening = 1;

	if ((chp->num_who == 0) || (chp->who == NULL)) {
	    wp->next = NULL;
	} else {
	    wp->next = chp->who;
	}
	chp->who = wp;
	chp->num_who++;

	update_comwho(chp);
	
	if (!isPlayer(player) || (Connected(player) && !Hidden(player))) {
	    com_message(chp, tprintf("%s %s has joined this channel.",
				     chp->header, Name(player)),
			player);
	}

	if (title_str) {
	    notify(player,
		  tprintf("Channel '%s' added with alias '%s' and title '%s'.",
			  chp->name, alias_str, cap->title));
	} else {
	    notify(player,
		   tprintf("Channel '%s' added with alias '%s'.",
			   chp->name, alias_str));
	}

    } else {

	if (title_str) {
	    notify(player,
  	      tprintf("Alias '%s' with title '%s' added for channel '%s'.",
		      alias_str, cap->title, chp->name));
	} else {
	    notify(player,
		   tprintf("Alias '%s' added for channel '%s'.",
			   alias_str, chp->name));
	}
    }
}

void channel_clr(player)
    dbref player;
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

    ch_array = (CHANNEL **) XCALLOC(mod_comsys_comsys_htab.entries,
				    sizeof(CHANNEL *), "channel_clr.array");
    pos = 0;
    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = next) {

	/* This is unnecessarily redundant, but it's not as if
	 * a player is going to be on tons of channels.
	 */
	found = 0;
	for (i = 0;
	     (i < mod_comsys_comsys_htab.entries) && (ch_array[i] != NULL);
	     i++) {
	    if (ch_array[i] == cl_ptr->alias_ptr->channel) {
		found = 1;
		break;
	    }
	}
	if (!found) {
	    ch_array[pos] = cl_ptr->alias_ptr->channel;
	    pos++;
	}

	sprintf(tbuf, "%d.%s", player, cl_ptr->alias_ptr->alias),
	clear_chan_alias(tbuf, cl_ptr->alias_ptr);
	next = cl_ptr->next;
	XFREE(cl_ptr, "channel_clr.cl_ptr");
    }

    nhashdelete((int) player, &mod_comsys_comlist_htab);

    /* Remove from channels. */

    for (i = 0; i < pos; i++)
	remove_from_channel(player, ch_array[i], 0);
    XFREE(ch_array, "channel_clr.array");
}

void mod_comsys_announce_connect(player, reason, num)
    dbref player;
    const char *reason;
    int num;
{
    CHANNEL *chp;

    /* It's slightly easier to just go through the channels and see
     * which ones the player is on, for announcement purposes.
     */

    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
	if (is_onchannel(player, chp)) {
	    update_comwho(chp);
	    if ((chp->flags & CHAN_FLAG_LOUD) && !Hidden(player) &&
		is_listenchannel(player, chp)) {
		com_message(chp, tprintf("%s %s has connected.",
					 chp->header, Name(player)),
			    player);
	    }
	}
    }
}

void mod_comsys_announce_disconnect(player, reason, num)
    dbref player;
    const char *reason;
    int num;
{
    CHANNEL *chp;

    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
	if (is_onchannel(player, chp)) {
	    if ((chp->flags & CHAN_FLAG_LOUD) && !Hidden(player) &&
		is_listenchannel(player, chp)) {
		com_message(chp, tprintf("%s %s has disconnected.",
					 chp->header, Name(player)),
			    player);
	    }
	    update_comwho(chp);
	}
    }
}

void update_comwho_all()
{
    CHANNEL *chp;

    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
	update_comwho(chp);
    }
}

void comsys_chown(from_player, to_player)
    dbref from_player, to_player;
{
    CHANNEL *chp;

    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
	if (chp->owner == from_player)
	    chp->owner = to_player;
    }
}


/* --------------------------------------------------------------------------
 * Comsys commands: channel administration.
 */

void do_ccreate(player, cause, key, name)
    dbref player, cause;
    int key;
    char *name;
{
    CHANNEL *chp;
    char buf[LBUF_SIZE];

    if (!Comm_All(player)) {
	notify(player, NOPERM_MESSAGE);
	return;
    }

    if (!ok_channel_string(name, MAX_CHAN_NAME_LEN, 1, 0)) {
	notify(player, NO_CHAN_MSG);
	return;
    }

    if (lookup_channel(name) != NULL) {
	notify(player, "That channel name is in use.");
	return;
    }

    chp = (CHANNEL *) XMALLOC(sizeof(CHANNEL), "ccreate.channel");
    if (!chp) {
	notify(player, "Out of memory.");
	return;
    }

    chp->name = XSTRDUP(name, "ccreate.name");
    chp->owner = Owner(player);
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
    sprintf(buf, "[%s]", chp->name);
    chp->header = XSTRDUP(buf, "ccreate.header");

    hashadd(name, (int *) chp, &mod_comsys_comsys_htab, 0);

    notify(player, tprintf("Channel %s created.", name));
}


void do_cdestroy(player, cause, key, name)
    dbref player, cause;
    int key;
    char *name;
{
    CHANNEL *chp;
    COMALIAS **alias_array;
    COMALIAS *cap;
    char **name_array;
    HASHTAB *htab;
    HASHENT *hptr;
    int i, count;

    find_channel(player, name, chp);
    check_owned_channel(player, chp);

    /* We have the wonderful joy of cleaning out all the aliases
     * that are currently pointing to this channel. We begin by
     * warning everyone that it's going away, and then we obliterate
     * it. We have to delete the pointers one by one or we run into
     * hashtable chaining issues.
     */

    com_message(chp, tprintf("Channel %s has been destroyed by %s.",
			     chp->name, Name(player)),
		player);

    htab = &mod_comsys_calias_htab;
    alias_array = (COMALIAS **) XCALLOC(htab->entries, sizeof(COMALIAS *),
					"cdestroy.alias_array");
    name_array = (char **) XCALLOC(htab->entries, sizeof(char *),
				   "cdestroy.name_array");
    count = 0;
    for (i = 0; i < htab->hashsize; i++) {
	for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next) {
	    cap = (COMALIAS *) hptr->data;
	    if (cap->channel == chp) {
		name_array[count] = hptr->target.s;
		alias_array[count] = cap;
		count++;
	    }
	}
    }

    /* Delete the aliases from the players' lists, then wipe them out. */

    if (count > 0) {
	for (i = 0; i < count; i++) {
	    zorch_alias_from_list(alias_array[i]);
	    clear_chan_alias(name_array[i], alias_array[i]);
	}
    }

    XFREE(name_array, "cdestroy.name_array");
    XFREE(alias_array, "cdestroy.alias_array");

    /* Zap the channel itself. */

    XFREE(chp->name, "cdestroy.cname");
    if (chp->who)
	XFREE(chp->who, "cdestroy.who");
    if (chp->connect_who)
	XFREE(chp->connect_who, "cdestroy.connect_who");
    if (chp->descrip)
	XFREE(chp->descrip, "cdestroy.descrip");
    XFREE(chp->header, "cdestroy.header");
    if (chp->join_lock)
	free_boolexp(chp->join_lock);
    if (chp->trans_lock)
	free_boolexp(chp->trans_lock);
    if (chp->recv_lock)
	free_boolexp(chp->recv_lock);
    XFREE(chp, "cdestroy.channel");
    hashdelete(name, &mod_comsys_comsys_htab);

    notify(player, tprintf("Channel %s destroyed.", name));
}

void do_channel(player, cause, key, chan_name, arg)
    dbref player, cause;
    int key;
    char *chan_name, *arg;
{
    CHANNEL *chp;
    BOOLEXP *boolp;
    dbref new_owner;
    int c_charge, negate, flag;

    find_channel(player, chan_name, chp);
    check_owned_channel(player, chp);

    if (!key || (key & CHANNEL_SET)) {

	if (*arg == '!') {
	    negate = 1;
	    arg++;
	} else {
	    negate = 0;
	}

	if (!strcasecmp(arg, (char *) "public")) {
	    flag = CHAN_FLAG_PUBLIC;
	} else if (!strcasecmp(arg, (char *) "loud")) {
	    flag = CHAN_FLAG_LOUD;
	} else if (!strcasecmp(arg, (char *) "spoof")) {
	    flag = CHAN_FLAG_SPOOF;
	} else if (!strcasecmp(arg, (char *) "p_join")) {
	    flag = CHAN_FLAG_P_JOIN;
	} else if (!strcasecmp(arg, (char *) "p_transmit")) {
	    flag = CHAN_FLAG_P_TRANS;
	} else if (!strcasecmp(arg, (char *) "p_receive")) {
	    flag = CHAN_FLAG_P_RECV;
	} else if (!strcasecmp(arg, (char *) "o_join")) {
	    flag = CHAN_FLAG_O_JOIN;
	} else if (!strcasecmp(arg, (char *) "o_transmit")) {
	    flag = CHAN_FLAG_O_TRANS;
	} else if (!strcasecmp(arg, (char *) "o_receive")) {
	    flag = CHAN_FLAG_O_RECV;
	} else {
	    notify(player, "That is not a valid channel flag name.");
	    return;
	}

	if (negate)
	    chp->flags &= ~flag;
	else
	    chp->flags |= flag;

	notify(player, "Set.");

    } else if (key & CHANNEL_LOCK) {

	if (arg && *arg) {
	    boolp = parse_boolexp(player, arg, 0);
	    if (boolp == TRUE_BOOLEXP) {
		notify(player, "I don't understand that key.");
		free_boolexp(boolp);
		return;
	    }
	    if (key & CHANNEL_JOIN) {
		if (chp->join_lock)
		    free_boolexp(chp->join_lock);
		chp->join_lock = boolp;
	    } else if (key & CHANNEL_RECV) {
		if (chp->recv_lock)
		    free_boolexp(chp->recv_lock);
		chp->recv_lock = boolp;
	    } else if (key & CHANNEL_TRANS) {
		if (chp->trans_lock)
		    free_boolexp(chp->trans_lock);
		chp->trans_lock = boolp;
	    } else {
		notify(player, "You must specify a valid lock type.");
		free_boolexp(boolp);
		return;
	    }
	    notify(player, "Channel locked.");
	} else {
	    if (key & CHANNEL_JOIN) {
		if (chp->join_lock)
		    free_boolexp(chp->join_lock);
		chp->join_lock = NULL;
	    } else if (key & CHANNEL_RECV) {
		if (chp->recv_lock)
		    free_boolexp(chp->recv_lock);
		chp->recv_lock = NULL;
	    } else if (key & CHANNEL_TRANS) {
		if (chp->trans_lock)
		    free_boolexp(chp->trans_lock);
		chp->trans_lock = NULL;
	    }
	    notify(player, "Channel unlocked.");
	}

    } else if (key & CHANNEL_OWNER) {

	new_owner = lookup_player(player, arg, 1);
	if (Good_obj(new_owner)) {
	    chp->owner = Owner(new_owner); /* no robots */
	    notify(player, "Owner set.");
	} else {
	    notify(player, "No such player.");
	}

    } else if (key & CHANNEL_CHARGE) {

	c_charge = atoi(arg);
	if ((c_charge < 0) || (c_charge > 32767)) {
	    notify(player, "That is not a reasonable cost.");
	    return;
	}
	chp->charge = c_charge;
	notify(player, "Set.");

    } else if (key & CHANNEL_DESC) {

	if (arg && *arg && !ok_channel_string(arg, MAX_CHAN_DESC_LEN, 1, 1)) {
	    notify(player, "That is not a reasonable channel description.");
	    return;
	}

	if (chp->descrip)
	    XFREE(chp->descrip, "do_channel.desc");
	if (arg && *arg)
	    chp->descrip = XSTRDUP(arg, "do_channel.desc");
	notify(player, "Set.");

    } else if (key & CHANNEL_HEADER) {

	if (arg && *arg && !ok_channel_string(arg, MAX_CHAN_HEAD_LEN, 1, 1)) {
	    notify(player, "That is not a reasonable channel header.");
	    return;
	}

	XFREE(chp->header, "do_channel.header");
	if (!arg)
	    chp->header = XSTRDUP("", "do_channel.header");
	else
	    chp->header = XSTRDUP(arg, "do_channel.header");
	notify(player, "Set.");

    } else {
	notify(player, "Invalid channel command.");
    }
}


void do_cboot(player, cause, key, name, objstr)
    dbref player, cause;
    int key;
    char *name, *objstr;
{
    CHANNEL *chp;
    dbref thing;
    COMLIST *chead, *clist, *cl_ptr, *next, *prev;
    char *t;
    char tbuf[SBUF_SIZE];

    find_channel(player, name, chp);
    check_owned_channel(player, chp);

    thing = match_thing(player, objstr);
    if (thing == NOTHING)
	return;

    if (!is_onchannel(thing, chp)) {
	notify(player, "Your target is not on that channel.");
	return;
    }

    /* Clear out all of the player's aliases for this channel. */

    chead = clist = lookup_clist(thing);
    if (clist) {
	prev = NULL;
	for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = next) {
	    next = cl_ptr->next;
	    if (cl_ptr->alias_ptr->channel == chp) {
		if (prev)
		    prev->next = cl_ptr->next;
		else
		    clist = cl_ptr->next;
		sprintf(tbuf, "%d.%s", thing, cl_ptr->alias_ptr->alias);
		clear_chan_alias(tbuf, cl_ptr->alias_ptr);
		XFREE(cl_ptr, "do_cboot.cl_ptr");
	    } else {
		prev = cl_ptr;
	    }
	}
	if (!clist)
	    nhashdelete((int) thing, &mod_comsys_comlist_htab);
	else if (chead != clist)
	    nhashrepl((int) thing, (int *) clist, &mod_comsys_comlist_htab);
    }

    notify(player, tprintf("You boot %s off channel %s.",
			   Name(thing), chp->name));
    notify(thing, tprintf("%s boots you off channel %s.",
			  Name(player), chp->name));

    if (key & CBOOT_QUIET) {
	remove_from_channel(thing, chp, 0);
    } else {
	remove_from_channel(thing, chp, 1);
	t = tbuf;
	safe_sb_str(Name(player), tbuf, &t);
	com_message(chp, tprintf("%s %s boots %s off the channel.",
				 chp->header, tbuf, Name(thing)),
		    player);
    }
}

void do_cemit(player, cause, key, chan_name, str)
    dbref player, cause;
    int key;
    char *chan_name, *str;
{
    CHANNEL *chp;

    find_channel(player, chan_name, chp);
    check_owned_channel(player, chp);

    if (key & CEMIT_NOHEADER)
	com_message(chp, str, player);
    else
	com_message(chp, tprintf("%s %s", chp->header, str),
		    player);
}

void do_cwho(player, cause, key, chan_name)
    dbref player, cause;
    int key;
    char *chan_name;
{
    CHANNEL *chp;
    CHANWHO *wp;
    int i;
    int p_count, o_count;

    find_channel(player, chan_name, chp);
    check_owned_channel(player, chp);

    p_count = o_count = 0;

    notify(player, "      Name                      Player?");

    if (key & CWHO_ALL) {
	for (wp = chp->who; wp != NULL; wp = wp->next) {
	    notify(player, tprintf("%s  %-25s %7s",
				   (wp->is_listening) ? "[on]" : "    ",
				   Name(wp->player),
				   isPlayer(wp->player) ? "Yes" : "No"));
	    if (isPlayer(wp->player))
		p_count++;
	    else
		o_count++;
	}
    } else {
	for (i = 0; i < chp->num_connected; i++) {
	    wp = chp->connect_who[i];
	    if (!Hidden(wp->player) || See_Hidden(player)) {
		notify(player, tprintf("%s  %-25s %7s",
				       (wp->is_listening) ? "[on]" : "    ",
				       Name(wp->player),
				       isPlayer(wp->player) ? "Yes" : "No"));
		if (isPlayer(wp->player))
		    p_count++;
		else
		    o_count++;
	    }
	}
    }

    notify(player, tprintf("Counted %d %s and %d %s on channel %s.",
			   p_count, (p_count == 1) ? "player" : "players",
			   o_count, (o_count == 1) ? "object" : "objects",
			   chp->name));
}


/* --------------------------------------------------------------------------
 * Comsys commands: player-usable.
 */

void do_addcom(player, cause, key, alias_str, args, nargs)
    dbref player, cause;
    int key;
    char *alias_str;
    char *args[];
    int nargs;
{
    char *chan_name, *title_str;

    if (nargs < 1) {
	notify(player, "You need to specify a channel.");
	return;
    }
    chan_name = args[0];
    if (nargs < 2) {
	title_str = NULL;
    } else {
	title_str = args[1];
    }

    join_channel(player, chan_name, alias_str, title_str);
}

void do_delcom(player, cause, key, alias_str)
    dbref player, cause;
    int key;
    char *alias_str;
{
    COMALIAS *cap;
    CHANNEL *chp;
    COMLIST *clist, *cl_ptr;
    int has_mult;

    find_calias(player, alias_str, cap);

    chp = cap->channel;		/* save this for later */

    zorch_alias_from_list(cap);
    clear_chan_alias(tprintf("%d.%s", player, alias_str), cap);

    /* Check if we have any aliases left pointing to that channel. */

    clist = lookup_clist(player);
    has_mult = 0;
    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next) {
	if (cl_ptr->alias_ptr->channel == chp) {
	    has_mult = 1;
	    break;
	}
    }

    if (has_mult) {
	notify(player, tprintf("You remove the alias '%s' for channel %s.",
			       alias_str, chp->name));
    } else {
	notify(player, tprintf("You leave channel %s.", chp->name));
	remove_from_channel(player, chp, 0);
    }
}

void do_clearcom(player, cause, key)
    dbref player, cause;
    int key;
{
    notify(player, "You remove yourself from all channels.");
    channel_clr(player);
}


void do_comtitle(player, cause, key, alias_str, title)
    dbref player, cause;
    int key;
    char *alias_str, *title;
{
    COMALIAS *cap;

    find_calias(player, alias_str, cap);

    if (cap->title)
	XFREE(cap->title, "do_comtitle.title");

    if (!title || !*title) {
	notify(player, tprintf("Title cleared on channel %s.",
			       cap->channel->name));
	return;
    }

    cap->title = XSTRDUP(munge_comtitle(title), "do_comtitle.title");

    notify(player, tprintf("Title set to '%s' on channel %s.",
			   cap->title, cap->channel->name));
}

void do_clist(player, cause, key, chan_name)
    dbref player, cause;
    int key;
    char *chan_name;
{
    CHANNEL *chp;
    char *buff, tbuf[LBUF_SIZE], *tp;
    int count = 0;

    if (chan_name && *chan_name) {

	find_channel(player, chan_name, chp);
	check_owned_channel(player, chp);

	notify(player, chp->name);

	tp = tbuf;
	safe_str("Flags:", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_PUBLIC)
	    safe_str(" Public", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_LOUD)
	    safe_str(" Loud", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_SPOOF)
	    safe_str(" Spoof", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_P_JOIN)
	    safe_str(" P_Join", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_P_RECV)
	    safe_str(" P_Receive", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_P_TRANS)
	    safe_str(" P_Transmit", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_O_JOIN)
	    safe_str(" O_Join", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_O_RECV)
	    safe_str(" O_Receive", tbuf, &tp);
	if (chp->flags & CHAN_FLAG_O_TRANS)
	    safe_str(" O_Transmit", tbuf, &tp);
	*tp = '\0';
	notify(player, tbuf);

	if (chp->join_lock)
	    buff = unparse_boolexp(player, chp->join_lock);
	else
	    buff = (char *) "*UNLOCKED*";
	notify(player, tprintf("Join Lock: %s", buff));

	if (chp->trans_lock)
	    buff = unparse_boolexp(player, chp->trans_lock);
	else
	    buff = (char *) "*UNLOCKED*";
	notify(player, tprintf("Transmit Lock: %s", buff));

	if (chp->recv_lock)
	    buff = unparse_boolexp(player, chp->recv_lock);
	else
	    buff = (char *) "*UNLOCKED*";
	notify(player, tprintf("Receive Lock: %s", buff));

	if (chp->descrip)
	    notify(player, tprintf("Description: %s", chp->descrip));

	return;
    }

    if (key & CLIST_FULL) {
	notify(player, "Channel              Flags      Locks  Charge  Balance  Users  Messages  Owner");
    } else if (key & CLIST_HEADER) {
	notify(player, "Channel              Owner              Header");
    } else {
	notify(player, "Channel              Owner              Description");
    }

    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
	if ((chp->flags & CHAN_FLAG_PUBLIC) ||
	    Comm_All(player) || (chp->owner == player)) {

	    if (key & CLIST_FULL) {
		notify(player,
         tprintf("%-20s %c%c%c%c%c%c%c%c%c  %c%c%c    %6d  %7d  %5d  %8d  #%d",
		  chp->name,
		  (chp->flags & CHAN_FLAG_PUBLIC) ? 'P' : '-',
		  (chp->flags & CHAN_FLAG_LOUD) ? 'L' : '-',
		  (chp->flags & CHAN_FLAG_SPOOF) ? 'S' : '-',
		  (chp->flags & CHAN_FLAG_P_JOIN) ? 'J' : '-',
		  (chp->flags & CHAN_FLAG_P_TRANS) ? 'X' : '-',
		  (chp->flags & CHAN_FLAG_P_RECV) ? 'R' : '-',
		  (chp->flags & CHAN_FLAG_O_JOIN) ? 'j' : '-',
		  (chp->flags & CHAN_FLAG_O_TRANS) ? 'x' : '-',
		  (chp->flags & CHAN_FLAG_O_RECV) ? 'r' : '-',
		  (chp->join_lock) ? 'J' : '-',
		  (chp->trans_lock) ? 'X' : '-',
		  (chp->recv_lock) ? 'R' : '-',
		  chp->charge,
		  chp->charge_collected,
		  chp->num_who,
		  chp->num_sent,
		  chp->owner));
	    } else {
		notify(player,
		       tprintf("%-20s %-18s %-38.38s",
			       chp->name, Name(chp->owner),
			       ((key & CLIST_HEADER) ? chp->header :
				(chp->descrip ? chp->descrip : " "))));
	    }
	    count++;
	}
    }

    if (Comm_All(player)) {
	notify(player, tprintf("There %s %d %s.",
			       (count == 1) ? "is" : "are",
			       count,
			       (count == 1) ? "channel" : "channels"));
    } else {
	notify(player, tprintf("There %s %d %s visible to you.",
			       (count == 1) ? "is" : "are",
			       count,
			       (count == 1) ? "channel" : "channels"));
    }
}


void do_comlist(player, cause, key)
    dbref player, cause;
    int key;
{
    COMLIST *clist, *cl_ptr;
    int count = 0;

    clist = lookup_clist(player);
    if (!clist) {
	notify(player, "You are not on any channels.");
	return;
    }

    notify(player, "Alias      Channel              Title");

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next) {

	/* We are guaranteed alias and channel lengths that are not truncated.
	 * We need to truncate title.
	 */
	notify(player,
	       tprintf("%-10s %-20s %-40.40s  %s",
		       cl_ptr->alias_ptr->alias,
		       cl_ptr->alias_ptr->channel->name,
		       (cl_ptr->alias_ptr->title) ? cl_ptr->alias_ptr->title :
		       (char *) "                                        ",
		       (is_listenchannel(player, cl_ptr->alias_ptr->channel)) ?
		       "[on]" : " "));
	count++;
    }

    notify(player, tprintf("You have %d channel %s.",
			   count,
			   (count == 1) ? "alias" : "aliases"));
}
			

void do_allcom(player, cause, key, cmd)
    dbref player, cause;
    int key;
    char *cmd;
{
    COMLIST *clist, *cl_ptr;

    clist = lookup_clist(player);
    if (!clist) {
	notify(player, "You are not on any channels.");
	return;
    }

    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next)
	process_comsys(player, cmd, cl_ptr->alias_ptr);
}


int mod_comsys_process_command(player, cause, interactive, in_cmd,
			       args, nargs)
    dbref player, cause;
    int interactive;
    char *in_cmd, *args[];
    int nargs;
{
    /* Return 1 if we got something, 0 if we didn't. */

    char *arg;
    COMALIAS *cap;
    char cmd[LBUF_SIZE];	/* temp -- can't nibble our input */

    if (!in_cmd || !*in_cmd || Slave(player))
	return 0;

    strcpy(cmd, in_cmd);

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
    if (!*arg) {
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
{(char *)"quiet",	1,	CA_PUBLIC,	CBOOT_QUIET},
{ NULL,			0,	0,		0}};

NAMETAB cemit_sw[] = {
{(char *)"noheader",	1,	CA_PUBLIC,	CEMIT_NOHEADER},
{ NULL,			0,	0,		0}};

NAMETAB channel_sw[] = {
{(char *)"charge",	1,	CA_PUBLIC,	CHANNEL_CHARGE},
{(char *)"desc",	1,	CA_PUBLIC,	CHANNEL_DESC},
{(char *)"header",	1,	CA_PUBLIC,	CHANNEL_HEADER},
{(char *)"lock",	1,	CA_PUBLIC,	CHANNEL_LOCK},
{(char *)"owner",	1,	CA_PUBLIC,	CHANNEL_OWNER},
{(char *)"set",		1,	CA_PUBLIC,	CHANNEL_SET},
{(char *)"join",	1,	CA_PUBLIC,	CHANNEL_JOIN | SW_MULTIPLE},
{(char *)"transmit",	1,	CA_PUBLIC,	CHANNEL_TRANS | SW_MULTIPLE},
{(char *)"receive",	1,	CA_PUBLIC,	CHANNEL_RECV | SW_MULTIPLE},
{ NULL,			0,	0,		0}};

NAMETAB clist_sw[] = {
{(char *)"full",        1,      CA_PUBLIC,      CLIST_FULL},
{(char *)"header",	1,	CA_PUBLIC,	CLIST_HEADER},
{ NULL,                 0,      0,              0}};

NAMETAB cwho_sw[] = {
{(char *)"all",         1,      CA_PUBLIC,      CWHO_ALL},
{ NULL,                 0,      0,              0}};

CMDENT mod_comsys_cmdtable[] = {
{(char *)"@cboot",              cboot_sw,       CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_TWO_ARG,          
	NULL,		NULL,	NULL,		{do_cboot}},
{(char *)"@ccreate",            NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_ONE_ARG,          
	NULL,		NULL,	NULL,		{do_ccreate}},
{(char *)"@cdestroy",           NULL,           CA_NO_SLAVE|CA_NO_GUEST,
        0,               CS_ONE_ARG,          
	NULL,		NULL,	NULL,		{do_cdestroy}},
{(char *)"@cemit",		cemit_sw,	CA_NO_SLAVE|CA_NO_GUEST,
	0,		 CS_TWO_ARG,		
	NULL,		NULL,	NULL,		{do_cemit}},
{(char *)"@channel",		channel_sw,	CA_NO_SLAVE|CA_NO_GUEST,
	0,		 CS_TWO_ARG|CS_INTERP,		
	NULL,		NULL,	NULL,		{do_channel}},
{(char *)"@clist",              clist_sw,       CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,		NULL,	NULL,		{do_clist}},
{(char *)"@cwho",               cwho_sw,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,		NULL,	NULL,		{do_cwho}},
{(char *)"addcom",              NULL,           CA_NO_SLAVE,
        0,              CS_TWO_ARG|CS_ARGV,           
	NULL,		NULL,	NULL,		{do_addcom}},
{(char *)"allcom",              NULL,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,		NULL,	NULL,		{do_allcom}},
{(char *)"comlist",             NULL,           CA_NO_SLAVE,
        0,              CS_NO_ARGS,           
	NULL,		NULL,	NULL,		{do_comlist}},
{(char *)"comtitle",            NULL,           CA_NO_SLAVE,
        0,              CS_TWO_ARG,          
	NULL,		NULL,	NULL,		{do_comtitle}},
{(char *)"clearcom",            NULL,           CA_NO_SLAVE,
        0,              CS_NO_ARGS,           
	NULL,		NULL,	NULL,		{do_clearcom}},
{(char *)"delcom",              NULL,           CA_NO_SLAVE,
        0,              CS_ONE_ARG,           
	NULL,		NULL,	NULL,		{do_delcom}},
{(char *)NULL,			NULL,		0,
	0,		0,				
	NULL,		NULL,	NULL,		{NULL}}};

/* --------------------------------------------------------------------------
 * Initialization, and other fun with files.
 */

void mod_comsys_dump_database(fp)
FILE *fp;
{
    CHANNEL *chp;
    COMALIAS *cap;

    fprintf(fp, "+V4\n");

    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
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

    for (cap = (COMALIAS *) hash_firstentry(&mod_comsys_calias_htab);
	 cap != NULL;
	 cap = (COMALIAS *) hash_nextentry(&mod_comsys_calias_htab)) {
	putref(fp, cap->player);
	putstring(fp, cap->channel->name);
	putstring(fp, cap->alias);
	putstring(fp, cap->title);
	putref(fp, is_listening_disconn(cap->player, cap->channel));
	fprintf(fp, "<\n");
    }

    fprintf(fp, "*** END OF DUMP ***\n");
}

static void comsys_flag_convert(chp)
    CHANNEL *chp;
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

static void comsys_data_update(chp, obj)
    CHANNEL *chp;
    dbref obj;
{
    /* Copy data from a MUX channel object to a new-style channel. */

    char *key;
    dbref aowner;
    int aflags, alen;

    key = atr_get(obj, A_LOCK, &aowner, &aflags, &alen);
    chp->join_lock = parse_boolexp(obj, key, 1);
    free_lbuf(key);

    key = atr_get(obj, A_LUSE, &aowner, &aflags, &alen);
    chp->trans_lock = parse_boolexp(obj, key, 1);
    free_lbuf(key);

    key = atr_get(obj, A_LENTER, &aowner, &aflags, &alen);
    chp->recv_lock = parse_boolexp(obj, key, 1);
    free_lbuf(key);

    key = atr_pget(obj, A_DESC, &aowner, &aflags, &alen);
    if (*key)
	chp->descrip = XSTRDUP(key, "comsys_data_update.desc");
    else
	chp->descrip = NULL;
    free_lbuf(key);
}

static void read_comsys(fp, com_ver)
    FILE *fp;
    int com_ver;
{
    CHANNEL *chp;
    COMALIAS *cap;
    COMLIST *clist;
    CHANWHO *wp;
    char c, *s, buf[LBUF_SIZE];
    int done;

    done = 0;
    c = getc(fp);
    if (c == '+')		/* do we have any channels? */
	done = 1;
    ungetc(c, fp);

    /* Load up the channels */

    while (!done) {

	chp = (CHANNEL *) XMALLOC(sizeof(CHANNEL), "load_comsys.channel");
	chp->name = XSTRDUP(getstring_noalloc(fp, 1), "load_comsys.name");
	chp->owner = getref(fp);
	if (!Good_obj(chp->owner) || !isPlayer(chp->owner))
	    chp->owner = GOD;	/* sanitize */
	chp->flags = getref(fp);
	if (com_ver == 1)
	    comsys_flag_convert(chp);
	chp->charge = getref(fp);
	chp->charge_collected = getref(fp);
	chp->num_sent = getref(fp);
	chp->header = NULL;
	if (com_ver == 1) {
	    comsys_data_update(chp, getref(fp));
	} else {
	    s = (char *) getstring_noalloc(fp, 1);
	    if (s && *s)
		chp->descrip = XSTRDUP(s, "read_comsys.desc");
	    else
		chp->descrip = NULL;

	    if (com_ver > 3) {
		s = (char *) getstring_noalloc(fp, 1);
		if (s && *s)
		    chp->header = XSTRDUP(s, "read_comsys.header");
	    }

	    if (com_ver == 2) {
		/* Inherently broken behavior. Can't deal with eval locks,
		 * among other things.
		 */
		chp->join_lock = getboolexp1(fp);
		getc(fp);		/* eat newline */
		chp->trans_lock = getboolexp1(fp);
		getc(fp);		/* eat newline */
		chp->recv_lock = getboolexp1(fp);
		getc(fp);		/* eat newline */
	    } else {
		chp->join_lock = getboolexp1(fp);
		if (getc(fp) != '\n') {
		    /* Uh oh. Format error. Trudge on valiantly... probably
		     * won't work, but we can try.
		     */
		    fprintf(mainlog_fp,
	    "Missing newline while reading join lock for channel %s\n",
			    chp->name);
		}
		c = getc(fp);
		if (c == '\n') {
		    getc(fp);	/* eat the dash on the next line */
		    getc(fp);	/* eat the newline on the next line */
		} else if (c == '-') {
		    getc(fp);	/* eat the next newline */
		} else {
		    fprintf(mainlog_fp,
    "Expected termination sequence while reading join lock for channel %s\n",
			    chp->name);
		}
		chp->trans_lock = getboolexp1(fp);
		if (getc(fp) != '\n') {
		    fprintf(mainlog_fp,
	    "Missing newline while reading transmit lock for channel %s\n",
			    chp->name);
		}
		c = getc(fp);
		if (c == '\n') {
		    getc(fp);	/* eat the dash on the next line */
		    getc(fp);	/* eat the newline on the next line */
		} else if (c == '-') {
		    getc(fp);	/* eat the next newline */
		} else {
		    fprintf(mainlog_fp,
 "Expected termination sequence while reading transmit lock for channel %s\n",
			    chp->name);
		}
		chp->recv_lock = getboolexp1(fp);
		if (getc(fp) != '\n') {
		    fprintf(mainlog_fp,
	    "Missing newline while reading receive lock for channel %s\n",
			    chp->name);
		}
		c = getc(fp);
		if (c == '\n') {
		    getc(fp);	/* eat the dash on the next line */
		    getc(fp);	/* eat the newline on the next line */
		} else if (c == '-') {
		    getc(fp);	/* eat the next newline */
		} else {
		    fprintf(mainlog_fp,
  "Expected termination sequence while reading receive lock for channel %s\n",
			    chp->name);
		}
	    }
		    
	}

	if (!chp->header) {
	    sprintf(buf, "[%s]", chp->name);
	    chp->header = XSTRDUP(buf, "read_comsys.header");
	}

	chp->who = NULL;
	chp->num_who = 0;
	chp->connect_who = NULL;
	chp->num_connected = 0;
	hashadd(chp->name, (int *) chp, &mod_comsys_comsys_htab, 0);
	getstring_noalloc(fp, 0);	/* discard the < */
	c = getc(fp);
	if (c == '+')		/* look ahead for the end of the channels */
	    done = 1;
	ungetc(c, fp);
    }

    getstring_noalloc(fp, 0);	/* discard the version string */

    done = 0;
    c = getc(fp);
    if (c == '*')		/* do we have any aliases? */
	done = 1;
    ungetc(c, fp);

    /* Load up the aliases */

    while (!done) {
	cap = (COMALIAS *) XMALLOC(sizeof(COMALIAS), "load_comsys.alias");
	cap->player = getref(fp);
	cap->channel = lookup_channel((char *) getstring_noalloc(fp, 1));
	cap->alias = XSTRDUP(getstring_noalloc(fp, 1), "load_comsys.alias_str");
	s = (char *) getstring_noalloc(fp, 1);
	if (s && *s)
	    cap->title = XSTRDUP(s, "load_comsys.title");
	else
	    cap->title = NULL;
	hashadd(tprintf("%d.%s", cap->player, cap->alias), (int *) cap,
		&mod_comsys_calias_htab, 0);
	clist = (COMLIST *) XMALLOC(sizeof(COMLIST), "load_comsys.clist");
	clist->alias_ptr = cap;
	clist->next = lookup_clist(cap->player);
	if (clist->next == NULL)
	    nhashadd((int) cap->player, (int *) clist, &mod_comsys_comlist_htab);
	else
	    nhashrepl((int) cap->player, (int *) clist,
		      &mod_comsys_comlist_htab);
	if (!is_onchannel(cap->player, cap->channel)) {
	    wp = (CHANWHO *) XMALLOC(sizeof(CHANWHO), "load_comsys.who");
	    wp->player = cap->player;
	    wp->is_listening = getref(fp);
	    if ((cap->channel->num_who == 0) || (cap->channel->who == NULL)) {
		wp->next = NULL;
	    } else {
		wp->next = cap->channel->who;
	    }
	    cap->channel->who = wp;
	    cap->channel->num_who++;
	} else {
	    getref(fp);		/* toss the value */
	}
	getstring_noalloc(fp, 0);	/* discard the < */
	c = getc(fp);
	if (c == '*')		/* look ahead for the end of the aliases */
	    done = 1;
	ungetc(c, fp);
    }

    s = (char *) getstring_noalloc(fp, 0);
    if (strcmp(s, (char *) "*** END OF DUMP ***")) {
	STARTLOG(LOG_STARTUP, "INI", "COM")
	    log_printf("Aborted load on unexpected line: %s", s);
	ENDLOG
    }
}

static void sanitize_comsys()
{
    /* Because we can run into situations where the comsys db and
     * regular database are not in sync (ex: restore from backup),
     * we need to sanitize the comsys data structures at load time.
     * The comlists we have represent the dbrefs of objects on channels.
     * Thus we can just look at what objects are there, and work
     * accordingly.
     */

    int i, count;
    NHSHTAB *htab;
    NHSHENT *hptr;
    int *ptab;

    count = 0;
    htab = &mod_comsys_comlist_htab;
    ptab = (int *) XCALLOC(htab->entries, sizeof(int), "sanitize_comsys");

    for (i = 0; i < htab->hashsize; i++) {
	for (hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next) {
	    if (!Good_obj(hptr->target.i)) {
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

    XFREE(ptab, "sanitize_comsys");
}

void mod_comsys_make_minimal()
{
    CHANNEL *chp;

    do_ccreate(GOD, GOD, 0, mod_comsys_config.public_channel);
    chp = lookup_channel(mod_comsys_config.public_channel);
    if (chp) {		/* should always be true, but be safe */
	chp->flags |= CHAN_FLAG_PUBLIC;
    }

    do_ccreate(GOD, GOD, 0, mod_comsys_config.guests_channel);
    chp = lookup_channel(mod_comsys_config.guests_channel);
    if (chp) {		/* should always be true, but be safe */
	chp->flags |= CHAN_FLAG_PUBLIC;
    }
}

void mod_comsys_load_database(fp)
FILE *fp;
{
    char buffer[2 * MBUF_SIZE + 8];	/* depends on max length of params */

    fgets(buffer, sizeof(buffer), fp);
    if (!strncmp(buffer, (char *) "+V", 2)) {
	read_comsys(fp, atoi(buffer + 2));
	sanitize_comsys();
    } else {
	STARTLOG(LOG_STARTUP, "INI", "COM")
	    log_printf("Unrecognized comsys format.");
	ENDLOG
	mod_comsys_make_minimal();
    }
}

/* --------------------------------------------------------------------------
 * User functions.
 */

#define Grab_Channel(p) \
    chp = lookup_channel(fargs[0]); \
    if (!chp) { \
	safe_str((char *) "#-1 CHANNEL NOT FOUND", buff, bufc); \
	return; \
    } \
    if ((!Comm_All(p) && ((p) != chp->owner))) { \
	safe_str((char *) "#-1 NO PERMISSION TO USE", buff, bufc); \
	return; \
    }

#define Comsys_User(p,t) \
    t = lookup_player(p, fargs[0], 1); \
    if (!Good_obj(t) || (!Controls(p,t) && !Comm_All(p))) { \
	safe_str((char *) "#-1 NO PERMISSION TO USE", buff, bufc); \
	return; \
    }

#define Grab_Alias(p,n) \
    cap = lookup_calias(p,n); \
    if (!cap) { \
        safe_str((char *) "#-1 NO SUCH ALIAS", buff, bufc); \
        return; \
    }

FUNCTION(fun_comlist)
{
    CHANNEL *chp;
    char *bb_p;
    Delim osep;

    VaChk_Only_Out(1);

    bb_p = *bufc;
    for (chp = (CHANNEL *) hash_firstentry(&mod_comsys_comsys_htab);
	 chp != NULL;
	 chp = (CHANNEL *) hash_nextentry(&mod_comsys_comsys_htab)) {
	if ((chp->flags & CHAN_FLAG_PUBLIC) ||
	    Comm_All(player) || (chp->owner == player)) {
	    if (*bufc != bb_p) {
		print_sep(&osep, buff, bufc);
	    }
	    safe_str(chp->name, buff, bufc);
	}
    }
}

FUNCTION(fun_cwho)
{
    CHANNEL *chp;
    char *bb_p;
    int i;

    Grab_Channel(player);
    bb_p = *bufc;
    for (i = 0; i < chp->num_connected; i++) {
	if (chp->connect_who[i]->is_listening &&
	    (!isPlayer(chp->connect_who[i]->player) ||
	     (Connected(chp->connect_who[i]->player) &&
	      (!Hidden(chp->connect_who[i]->player) || See_Hidden(player))))) {
	    if (*bufc != bb_p)
		safe_chr(' ', buff, bufc);
	    safe_dbref(buff, bufc, chp->connect_who[i]->player);
	}
    }
}

FUNCTION(fun_cwhoall)
{
    CHANNEL *chp;
    CHANWHO *wp;
    char *bb_p;

    Grab_Channel(player);
    bb_p = *bufc;
    for (wp = chp->who; wp != NULL; wp = wp->next) {
	if (*bufc != bb_p)
	    safe_chr(' ', buff, bufc);
	safe_dbref(buff, bufc, wp->player);
    }
}

FUNCTION(fun_comowner)
{
    CHANNEL *chp;

    Grab_Channel(player);
    safe_dbref(buff, bufc, chp->owner);
}

FUNCTION(fun_comdesc)
{
    CHANNEL *chp;

    Grab_Channel(player);
    if (chp->descrip)
	safe_str(chp->descrip, buff, bufc);
}

FUNCTION(fun_comheader)
{
    CHANNEL *chp;

    Grab_Channel(player);
    if (chp->header)
	safe_str(chp->header, buff, bufc);
}

FUNCTION(fun_comalias)
{
    dbref target;
    COMLIST *clist, *cl_ptr;
    char *bb_p;

    Comsys_User(player, target);

    clist = lookup_clist(target);
    if (!clist)
	return;
    bb_p = *bufc;
    for (cl_ptr = clist; cl_ptr != NULL; cl_ptr = cl_ptr->next) {
	if (*bufc != bb_p)
	    safe_chr(' ', buff, bufc);
	safe_str(cl_ptr->alias_ptr->alias, buff, bufc);
    }
}

FUNCTION(fun_cominfo)
{
    dbref target;
    COMALIAS *cap;

    Comsys_User(player, target);
    Grab_Alias(target, fargs[1]);
    safe_str(cap->channel->name, buff, bufc);
}

FUNCTION(fun_comtitle)
{
    dbref target;
    COMALIAS *cap;

    Comsys_User(player, target);
    Grab_Alias(target, fargs[1]);
    if (cap->title)
	safe_str(cap->title, buff, bufc);
}

FUNCTION(fun_cemit)
{
    CHANNEL *chp;

    Grab_Channel(player);
    com_message(chp, fargs[1], player);
}

FUN mod_comsys_functable[] = {
{"CEMIT",	fun_cemit,	2,  0,		CA_PUBLIC,	NULL},
{"COMALIAS",	fun_comalias,	1,  0,		CA_PUBLIC,	NULL},
{"COMDESC",	fun_comdesc,	1,  0,		CA_PUBLIC,	NULL},
{"COMHEADER",	fun_comheader,	1,  0,		CA_PUBLIC,	NULL},
{"COMINFO",	fun_cominfo,	2,  0,		CA_PUBLIC,	NULL},
{"COMLIST",	fun_comlist,	0,  FN_VARARGS, CA_PUBLIC,	NULL},
{"COMOWNER",	fun_comowner,	1,  0,		CA_PUBLIC,	NULL},
{"COMTITLE",	fun_comtitle,	2,  0,		CA_PUBLIC,	NULL},
{"CWHO",        fun_cwho,       1,  0,          CA_PUBLIC,	NULL},
{"CWHOALL",     fun_cwhoall,    1,  0,          CA_PUBLIC,	NULL},
{NULL,		NULL,		0,  0,		0,		NULL}};

/* --------------------------------------------------------------------------
 * Initialization.
 */

void mod_comsys_cleanup_startup()
{
    update_comwho_all();
}

void mod_comsys_create_player(creator, player, isrobot, isguest)
    dbref creator, player;
    int isrobot, isguest;
{
    if (isguest && (player != 1)) {
	if (*mod_comsys_config.guests_channel)
	    join_channel(player, mod_comsys_config.guests_channel,
			 mod_comsys_config.guests_calias, NULL);
    } else if (player != 1) { /* avoid problems with minimal db */
	if (*mod_comsys_config.public_channel)
	    join_channel(player, mod_comsys_config.public_channel,
			 mod_comsys_config.public_calias, NULL);
    }
}

void mod_comsys_destroy_obj(player, obj)
    dbref player, obj;
{
    channel_clr(obj);
}

void mod_comsys_destroy_player(player, victim)
    dbref player, victim;
{
    comsys_chown(victim, Owner(player));
}

void mod_comsys_init()
{
    mod_comsys_config.public_channel = XSTRDUP("Public", "cf_string");
    mod_comsys_config.guests_channel = XSTRDUP("Guests", "cf_string");
    mod_comsys_config.public_calias = XSTRDUP("pub", "cf_string");
    mod_comsys_config.guests_calias = XSTRDUP("g", "cf_string");

    register_hashtables(mod_comsys_hashtable, mod_comsys_nhashtable);
    register_commands(mod_comsys_cmdtable);
    register_functions(mod_comsys_functable);
}
