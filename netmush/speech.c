/**
 * @file speech.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Commands which involve speaking
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

#include <string.h>

int sp_ok(dbref player)
{
	if (Gagged(player) && (!(Wizard(player))))
	{
		notify(player, "Sorry. Gagged players cannot speak.");
		return 0;
	}

	if (!mushconf.robot_speak)
	{
		if (Robot(player) && !controls(player, Location(player)))
		{
			notify(player, "Sorry robots may not speak in public.");
			return 0;
		}
	}

	if (Auditorium(Location(player)))
	{
		if (!could_doit(player, Location(player), A_LSPEECH))
		{
			notify(player, "Sorry, you may not speak in this place.");
			return 0;
		}
	}

	return 1;
}

void say_shout(int target, const char *prefix, int flags, dbref player, char *message)
{
	if (flags & SAY_NOTAG)
	{
		raw_broadcast(target, "%s%s", Name(player), message);
	}
	else
	{
		raw_broadcast(target, "%s%s%s", prefix, Name(player), message);
	}
}

const char *announce_msg = "Announcement: ";

const char *broadcast_msg = "Broadcast: ";

const char *admin_msg = "Admin: ";

void do_think(dbref player, dbref cause, __attribute__((unused)) int key, char *message)
{
	char *str, *buf, *bp;
	buf = bp = XMALLOC(LBUF_SIZE, "bp");
	str = message;
	exec(buf, &bp, player, cause, cause, EV_FCHECK | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
	notify(player, buf);
	XFREE(buf);
}

int check_speechformat(dbref player, dbref speaker, dbref loc, dbref thing, char *message, int key)
{
	char *sargs[2], *buff;
	int aflags;
	/*
	 * We have to make a copy of our arguments, because the exec() we
	 * * pass it through later can nibble those arguments, and we may
	 * * need to call this function more than once on the same message.
	 */

	sargs[0] = XSTRDUP(message, "sargs[0]");

	switch (key)
	{
	case SAY_SAY:
		sargs[1] = XSTRDUP("\"", "sargs[1]");
		break;

	case SAY_POSE:
		sargs[1] = XSTRDUP(":", "sargs[1]");
		break;

	case SAY_POSE_NOSPC:
		sargs[1] = XSTRDUP(";", "sargs[1]");
		break;

	default:
		sargs[1] = XSTRDUP("|", "sargs[1]");
	}

	/*
	 * Go get it. An empty evaluation is considered equivalent to no
	 * * attribute, unless the attribute has a no_name flag.
	 */
	buff = master_attr(speaker, thing, A_SPEECHFMT, sargs, 2, &aflags);

	if (buff)
	{
		if (*buff)
		{
			notify_all_from_inside_speech(loc, player, buff);
			XFREE(buff);
			XFREE(sargs[1]);
			XFREE(sargs[0]);
			return 1;
		}
		else if (aflags & AF_NONAME)
		{
			XFREE(buff);
			XFREE(sargs[1]);
			XFREE(sargs[0]);
			return 1;
		}
		XFREE(buff);
	}

	XFREE(sargs[1]);
	XFREE(sargs[0]);
	return 0;
}

void format_speech(dbref player, dbref speaker, dbref loc, char *message, int key)
{
	if (H_Speechmod(speaker) && check_speechformat(player, speaker, loc, speaker, message, key))
	{
		return;
	}

	if (H_Speechmod(loc) && check_speechformat(player, speaker, loc, loc, message, key))
	{
		return;
	}

	switch (key)
	{
	case SAY_SAY:
		if (mushconf.you_say)
		{
			notify_check(speaker, speaker, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You %s \"%s\"", mushconf.comma_say ? "say," : "say", message);

			if (loc != NOTHING)
			{
				notify_except(loc, player, speaker, MSG_SPEECH, "%s %s \"%s\"", Name(speaker), mushconf.comma_say ? "says," : "says", message);
			}
		}
		else
		{
			notify_check(loc, player, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_SPEECH, "%s %s \"%s\"", Name(speaker), mushconf.comma_say ? "says," : "says", message);
		}

		break;

	case SAY_POSE:
		notify_check(loc, player, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_SPEECH, "%s %s", Name(speaker), message);
		break;

	case SAY_POSE_NOSPC:
		notify_check(loc, player, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_SPEECH, "%s%s", Name(speaker), message);
		break;

	default:
		/*
		 * NOTREACHED
		 */
		notify_all_from_inside_speech(loc, player, message);
	}
}

void do_say(dbref player, __attribute__((unused)) dbref cause, int key, char *message)
{
	dbref loc;
	char *buf2, *bp, *name;
	int say_flags, depth;

	/*
	 * Check for shouts. Need to have Announce power.
	 */

	if ((key & SAY_SHOUT) && !Announce(player))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/*
	 * Convert prefix-coded messages into the normal type
	 */
	say_flags = key & (SAY_NOTAG | SAY_HERE | SAY_ROOM | SAY_HTML);
	key &= ~(SAY_NOTAG | SAY_HERE | SAY_ROOM | SAY_HTML);

	if (key & SAY_PREFIX)
	{
		key &= ~SAY_PREFIX;

		switch (key)
		{
		case SAY_POSE:
			message++;

			if (*message == ' ')
			{
				message++;
				key = SAY_POSE_NOSPC;
			}

			break;

		case SAY_SAY:
		case SAY_POSE_NOSPC:
			message++;
			break;

		case SAY_EMIT:

			/*
			 * if they doubled the backslash, remove it. Otherwise
			 * * it's already been removed by evaluation.
			 */
			if (*message == '\\')
			{
				message++;
			}

			break;

		default:
			return;
		}
	}

	/*
	 * Make sure speaker is somewhere if speaking in a place
	 */
	loc = where_is(player);

	switch (key)
	{
	case SAY_SAY:
	case SAY_POSE:
	case SAY_POSE_NOSPC:
	case SAY_EMIT:
		if (loc == NOTHING)
		{
			return;
		}

		if (!sp_ok(player))
		{
			return;
		}
	}

	/*
	 * Send the message on its way
	 */

	switch (key)
	{
	case SAY_SAY:
		format_speech(player, player, loc, message, SAY_SAY);
		break;

	case SAY_POSE:
		format_speech(player, player, loc, message, SAY_POSE);
		break;

	case SAY_POSE_NOSPC:
		format_speech(player, player, loc, message, SAY_POSE_NOSPC);
		break;

	case SAY_EMIT:
		if (!say_flags || (say_flags & SAY_HERE) || ((say_flags & SAY_HTML) && !(say_flags & SAY_ROOM)))
		{
			if (say_flags & SAY_HTML)
			{
				notify_all_from_inside_html_speech(loc, player, message);
			}
			else
			{
				notify_all_from_inside_speech(loc, player, message);
			}
		}

		if (say_flags & SAY_ROOM)
		{
			if ((Typeof(loc) == TYPE_ROOM) && (say_flags & SAY_HERE))
			{
				return;
			}

			depth = 0;

			while ((Typeof(loc) != TYPE_ROOM) && (depth++ < 20))
			{
				loc = Location(loc);

				if ((loc == NOTHING) || (loc == Location(loc)))
				{
					return;
				}
			}

			if (Typeof(loc) == TYPE_ROOM)
			{
				if (say_flags & SAY_HTML)
				{
					notify_all_from_inside_html_speech(loc, player, message);
				}
				else
				{
					notify_all_from_inside_speech(loc, player, message);
				}
			}
		}

		break;

	case SAY_SHOUT:
		switch (*message)
		{
		case ':':
			message[0] = ' ';
			say_shout(0, announce_msg, say_flags, player, message);
			break;

		case ';':
			message++;
			say_shout(0, announce_msg, say_flags, player, message);
			break;

		case '"':
			message++;
			//[[fallthrough]];
			__attribute__((fallthrough));
		default:
			buf2 = XMALLOC(LBUF_SIZE, "buf2");
			bp = buf2;
			SAFE_LB_STR((char *)" shouts, \"", buf2, &bp);
			SAFE_LB_STR(message, buf2, &bp);
			SAFE_LB_CHR('"', buf2, &bp);
			say_shout(0, announce_msg, say_flags, player, buf2);
			XFREE(buf2);
		}

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "SHOUT", "%s shouts: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;

	case SAY_WIZSHOUT:
		switch (*message)
		{
		case ':':
			message[0] = ' ';
			say_shout(WIZARD, broadcast_msg, say_flags, player, message);
			break;

		case ';':
			message++;
			say_shout(WIZARD, broadcast_msg, say_flags, player, message);
			break;

		case '"':
			message++;
			//[[fallthrough]];
			__attribute__((fallthrough));
		default:
			buf2 = XMALLOC(LBUF_SIZE, "buf2");
			bp = buf2;
			SAFE_LB_STR((char *)" says, \"", buf2, &bp);
			SAFE_LB_STR(message, buf2, &bp);
			SAFE_LB_CHR('"', buf2, &bp);
			say_shout(WIZARD, broadcast_msg, say_flags, player, buf2);
			XFREE(buf2);
		}

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "BCAST", "%s broadcasts: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;

	case SAY_ADMINSHOUT:
		switch (*message)
		{
		case ':':
			message[0] = ' ';
			say_shout(WIZARD, admin_msg, say_flags, player, message);
			say_shout(ROYALTY, admin_msg, say_flags, player, message);
			break;

		case ';':
			message++;
			say_shout(WIZARD, admin_msg, say_flags, player, message);
			say_shout(ROYALTY, admin_msg, say_flags, player, message);
			break;

		case '"':
			message++;
			//[[fallthrough]];
			__attribute__((fallthrough));
		default:
			buf2 = XMALLOC(LBUF_SIZE, "buf2");
			bp = buf2;
			SAFE_LB_STR((char *)" says, \"", buf2, &bp);
			SAFE_LB_STR(message, buf2, &bp);
			SAFE_LB_CHR('"', buf2, &bp);
			*bp = '\0';
			say_shout(WIZARD, admin_msg, say_flags, player, buf2);
			say_shout(ROYALTY, admin_msg, say_flags, player, buf2);
			XFREE(buf2);
		}

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "ASHOUT", "%s yells: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;

	case SAY_WALLPOSE:
		if (say_flags & SAY_NOTAG)
		{
			raw_broadcast(0, "%s %s", Name(player), message);
		}
		else
		{
			raw_broadcast(0, "Announcement: %s %s", Name(player), message);
		}

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "SHOUT", "%s WALLposes: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;

	case SAY_WIZPOSE:
		if (say_flags & SAY_NOTAG)
		{
			raw_broadcast(WIZARD, "%s %s", Name(player), message);
		}
		else
			raw_broadcast(WIZARD, "Broadcast: %s %s", Name(player), message);

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "BCAST", "%s WIZposes: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;

	case SAY_WALLEMIT:
		if (say_flags & SAY_NOTAG)
		{
			raw_broadcast(0, "%s", message);
		}
		else
		{
			raw_broadcast(0, "Announcement: %s", message);
		}

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "SHOUT", "%s WALLemits: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;

	case SAY_WIZEMIT:
		if (say_flags & SAY_NOTAG)
		{
			raw_broadcast(WIZARD, "%s", message);
		}
		else
		{
			raw_broadcast(WIZARD, "Broadcast: %s", message);
		}

		name = log_getname(player);
		buf2 = strip_ansi(message);
		log_write(LOG_SHOUTS, "WIZ", "BCAST", "%s WIZemits: '%s'", name, buf2);
		XFREE(buf2);
		XFREE(name);
		break;
	}
}

/*
 * ---------------------------------------------------------------------------
 * * do_page: Handle the page command.
 * * Page-pose code from shadow@prelude.cc.purdue.
 */
// void page_return( dbref player, dbref target, const char *tag, int anum, const char *dflt )
void page_return(dbref player, dbref target, const char *tag, int anum, const char *format, ...)
{
	dbref aowner;
	int aflags, alen;
	char *str, *str2, *buf, *bp;
	struct tm *tp;
	time_t t;
	char *dflt = XMALLOC(LBUF_SIZE, "dflt");
	char *s;
	va_list ap;
	va_start(ap, format);

	if ((!format || !*format) && format != NULL)
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(dflt, s, LBUF_SIZE);
		}
		else
		{
			dflt[0] = 0x00;
		}
	}
	else
	{
		XVSNPRINTF(dflt, LBUF_SIZE, format, ap);
	}

	va_end(ap);
	str = atr_pget(target, anum, &aowner, &aflags, &alen);

	if (*str)
	{
		str2 = bp = XMALLOC(LBUF_SIZE, "str2");
		buf = str;
		exec(str2, &bp, target, player, player, EV_FCHECK | EV_EVAL | EV_TOP | EV_NO_LOCATION, &buf, (char **)NULL, 0);

		if (*str2)
		{
			t = time(NULL);
			tp = localtime(&t);
			notify_check(player, target, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s message from %s: %s", tag, Name(target), str2);
			notify_check(player, target, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%d:%02d] %s message sent to %s.", tp->tm_hour, tp->tm_min, tag, Name(player));
		}

		XFREE(str2);
	}
	else if (*dflt)
	{
		notify_with_cause(player, target, dflt);
	}

	XFREE(str);
	XFREE(dflt);
}

int page_check(dbref player, dbref target)
{
	if (!payfor(player, Guest(player) ? 0 : mushconf.pagecost))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
	}
	else if (!Connected(target))
	{
		page_return(player, target, "Away", A_AWAY, "Sorry, %s is not connected.", Name(target));
	}
	else if (!could_doit(player, target, A_LPAGE))
	{
		if (Can_Hide(target) && Hidden(target) && !See_Hidden(player))
		{
			page_return(player, target, "Away", A_AWAY, "Sorry, %s is not connected.", Name(target));
		}
		else
		{
			page_return(player, target, "Reject", A_REJECT, "Sorry, %s is not accepting pages.", Name(target));
		}
	}
	else if (!could_doit(target, player, A_LPAGE))
	{
		if (Wizard(player))
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Warning: %s can't return your page.", Name(target));
			return 1;
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Sorry, %s can't return your page.", Name(target));
			return 0;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

void do_page(dbref player, __attribute__((unused)) dbref cause, int key, char *tname, char *message)
{
	/* key is 1 if this is a reply page */
	char *dbref_list, *ddp;
	char *clean_tname, *tnp;
	char *omessage, *omp, *imessage, *imp;
	int count = 0;
	int aowner, aflags, alen;
	dbref target;
	int n_dbrefs, i;
	int *dbrefs_array = NULL;
	char *str, *tokst;

	/*
	 * If we have to have an equals sign in the page command, if
	 * * there's no message, it's an error (otherwise tname would
	 * * be null and message would contain text).
	 * * Otherwise we handle repage by swapping args.
	 * * Unfortunately, we have no way of differentiating
	 * * 'page foo=' from 'page foo' -- both result in a valid tname.
	 */

	if (!key && !*message)
	{
		if (mushconf.page_req_equals)
		{
			notify(player, "No one to page.");
			return;
		}

		tnp = message;
		message = tname;
		tname = tnp;
	}

	if (!tname || !*tname)
	{ /* no recipient list; use lastpaged */

		/*
		 * Clean junk objects out of their lastpaged dbref list
		 */
		if (key)
			dbref_list = atr_get(player, A_PAGEGROUP, &aowner, &aflags, &alen);
		else
			dbref_list = atr_get(player, A_LASTPAGE, &aowner, &aflags, &alen);

		/*
		 * How many words in the list of targets?
		 */

		if (!*dbref_list)
		{
			count = 0;
		}
		else
		{
			for (n_dbrefs = 1, str = dbref_list;; n_dbrefs++)
			{
				if ((str = strchr(str, ' ')) != NULL)
				{
					str++;
				}
				else
				{
					break;
				}
			}

			dbrefs_array = (int *)XCALLOC(n_dbrefs, sizeof(int), "dbrefs_array");

			/*
			 * Convert the list into an array of targets. Validate.
			 */

			for (ddp = strtok_r(dbref_list, " ", &tokst); ddp; ddp = strtok_r(NULL, " ", &tokst))
			{
				target = (int)strtol(ddp, (char **)NULL, 10);

				if (!Good_obj(target) || !isPlayer(target))
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "I don't recognize #%d.", target);
					continue;
				}

				/*
				 * Eliminate ourselves from repeat and reply pages
				 */
				if (target == player)
				{
					continue;
				}

				dbrefs_array[count] = target;
				count++;
			}
		}

		XFREE(dbref_list);
	}
	else
	{ /* normal page; build new recipient list */
		if ((target = lookup_player(player, tname, 1)) != NOTHING)
		{
			dbrefs_array = (int *)XCALLOC(1, sizeof(int), "dbrefs_array");
			dbrefs_array[0] = target;
			count++;
		}
		else
		{
			/*
			 * How many words in the list of targets? Note that we separate
			 * * with either a comma or a space!
			 */
			n_dbrefs = 1;

			for (str = tname; *str; str++)
			{
				if ((*str == ' ') || (*str == ','))
				{
					n_dbrefs++;
				}
			}

			dbrefs_array = (int *)XCALLOC(n_dbrefs, sizeof(int), "dbrefs_array");

			/*
			 * Go look 'em up
			 */

			for (tnp = strtok_r(tname, ", ", &tokst); tnp; tnp = strtok_r(NULL, ", ", &tokst))
			{
				if ((target = lookup_player(player, tnp, 1)) != NOTHING)
				{
					dbrefs_array[count] = target;
					count++;
				}
				else
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "I don't recognize %s.", tnp);
				}
			}
		}
	}

	n_dbrefs = count;

	/*
	 * Filter out disconnected and pagelocked, if we're actually sending
	 * * a message.
	 */

	if (*message)
	{
		for (i = 0; i < n_dbrefs; i++)
		{
			if (!page_check(player, dbrefs_array[i]))
			{
				dbrefs_array[i] = NOTHING;
				count--;
			}
		}
	}

	/*
	 * Write back the lastpaged attribute.
	 */
	dbref_list = ddp = XMALLOC(LBUF_SIZE, "dbref_list");

	for (i = 0; i < n_dbrefs; i++)
	{
		if (dbrefs_array[i] != NOTHING)
		{
			if (ddp != dbref_list)
			{
				SAFE_LB_CHR(' ', dbref_list, &ddp);
			}

			SAFE_LTOS(dbref_list, &ddp, dbrefs_array[i], LBUF_SIZE);
		}
	}

	*ddp = '\0';
	atr_add_raw(player, A_LASTPAGE, dbref_list);
	XFREE(dbref_list);

	/*
	 * Check to make sure we have something.
	 */

	if (count == 0)
	{
		if (!*message)
		{
			if (key)
				notify(player, "You have not been paged by anyone.");
			else
			{
				notify(player, "You have not paged anyone.");
			}
		}
		else
		{
			notify(player, "No one to page.");
		}

		if (dbrefs_array)
		{
			XFREE(dbrefs_array);
		}

		return;
	}

	/*
	 * Each person getting paged is included in the pagegroup, as is the
	 * * person doing the paging. This lets us construct one list rather than
	 * * individual ones for each player. Self-paging is automatically
	 * * eliminated when the pagegroup is used.
	 */
	dbref_list = ddp = XMALLOC(LBUF_SIZE, "dbref_list");
	SAFE_LTOS(dbref_list, &ddp, player, LBUF_SIZE);

	for (i = 0; i < n_dbrefs; i++)
	{
		if (dbrefs_array[i] != NOTHING)
		{
			SAFE_LB_CHR(' ', dbref_list, &ddp);
			SAFE_LTOS(dbref_list, &ddp, dbrefs_array[i], LBUF_SIZE);
		}
	}

	*ddp = '\0';

	for (i = 0; i < n_dbrefs; i++)
		if (dbrefs_array[i] != NOTHING)
		{
			atr_add_raw(dbrefs_array[i], A_PAGEGROUP, dbref_list);
		}

	XFREE(dbref_list);
	/*
	 * Build name list. Even if we only have one name, we have to go
	 * * through the array, because the first entries might be invalid.
	 */
	clean_tname = tnp = XMALLOC(LBUF_SIZE, "clean_tname");

	if (count == 1)
	{
		for (i = 0; i < n_dbrefs; i++)
		{
			if (dbrefs_array[i] != NOTHING)
			{
				safe_name(dbrefs_array[i], clean_tname, &tnp);
				break;
			}
		}
	}
	else
	{
		SAFE_LB_CHR('(', clean_tname, &tnp);

		for (i = 0; i < n_dbrefs; i++)
		{
			if (dbrefs_array[i] != NOTHING)
			{
				if (tnp != clean_tname + 1)
					SAFE_STRNCAT(clean_tname, &tnp, ", ", 2, LBUF_SIZE);

				safe_name(dbrefs_array[i], clean_tname, &tnp);
			}
		}

		SAFE_LB_CHR(')', clean_tname, &tnp);
	}

	*tnp = '\0';
	/*
	 * Mess with message
	 */
	omessage = omp = XMALLOC(LBUF_SIZE, "omessage");
	imessage = imp = XMALLOC(LBUF_SIZE, ".imessage");

	switch (*message)
	{
	case '\0':
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You last paged %s.", clean_tname);
		XFREE(clean_tname);
		XFREE(omessage);
		XFREE(imessage);
		XFREE(dbrefs_array);
		return;

	case ':':
		message++;
		SAFE_LB_STR("From afar, ", omessage, &omp);

		if (count != 1)
		{
			SAFE_SPRINTF(omessage, &omp, "to %s: ", clean_tname);
		}

		SAFE_SPRINTF(omessage, &omp, "%s %s", Name(player), message);
		SAFE_SPRINTF(imessage, &imp, "Long distance to %s: %s %s", clean_tname, Name(player), message);
		break;

	case ';':
		message++;
		SAFE_LB_STR("From afar, ", omessage, &omp);

		if (count != 1)
		{
			SAFE_SPRINTF(omessage, &omp, "to %s: ", clean_tname);
		}

		SAFE_SPRINTF(omessage, &omp, "%s%s", Name(player), message);
		SAFE_SPRINTF(imessage, &imp, "Long distance to %s: %s%s", clean_tname, Name(player), message);
		break;

	case '"':
		message++;
		//[[fallthrough]];
		__attribute__((fallthrough));
	default:
		if (count != 1)
		{
			SAFE_SPRINTF(omessage, &omp, "To %s, ", clean_tname);
		}

		SAFE_SPRINTF(omessage, &omp, "%s pages: %s", Name(player), message);
		SAFE_SPRINTF(imessage, &imp, "You paged %s with '%s'.", clean_tname, message);
	}

	XFREE(clean_tname);

	/*
	 * Send the message out, checking for idlers
	 */

	for (i = 0; i < n_dbrefs; i++)
	{
		if (dbrefs_array[i] != NOTHING)
		{
			notify_with_cause(dbrefs_array[i], player, omessage);
			page_return(player, dbrefs_array[i], "Idle", A_IDLE, NULL);
		}
	}

	XFREE(omessage);
	XFREE(dbrefs_array);
	/*
	 * Tell the sender
	 */
	notify(player, imessage);
	XFREE(imessage);
}

void do_reply_page(dbref player, dbref cause, __attribute__((unused)) int key, char *msg)
{
	do_page(player, cause, 1, NULL, msg);
}

/*
 * ---------------------------------------------------------------------------
 * * do_pemit: Messages to specific players, or to all but specific players.
 */

void whisper_pose(dbref player, dbref target, char *message)
{
	char *buff;
	buff = XMALLOC(LBUF_SIZE, "buff");
	XSTRCPY(buff, Name(player));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s senses \"%s%s\"", Name(target), buff, message);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You sense %s%s", buff, message);
	XFREE(buff);
}

void do_pemit_list(dbref player, char *list, const char *message, int do_contents)
{
	/*
	 * Send a message to a list of dbrefs. To avoid repeated generation *
	 * of the NOSPOOF string, we set it up the first time we
	 * encounter something Nospoof, and then check for it
	 * thereafter. The list is destructively modified.
	 */
	char *p, *tokst;
	dbref who, *recips;
	int n_recips, r, ok_to_do;

	if (!message || !*message || !list || !*list)
	{
		return;
	}

	n_recips = 1;

	for (p = list; *p; ++p)
	{
		if (*p == ' ')
		{
			++n_recips;
		}
	}

	recips = (dbref *)XCALLOC(n_recips, sizeof(dbref), "recips");
	n_recips = 0;

	for (p = strtok_r(list, " ", &tokst); p != NULL; p = strtok_r(NULL, " ", &tokst))
	{
		init_match(player, p, TYPE_PLAYER);
		match_everything(0);
		who = match_result();

		switch (who)
		{
		case NOTHING:
			notify(player, "Emit to whom?");
			break;

		case AMBIGUOUS:
			notify(player, "I don't know who you mean!");
			break;

		default:
			if (!Good_obj(who))
			{
				continue;
			}

			/*
			 * avoid pemitting to this dbref if already done
			 */
			for (r = 0; r < n_recips; ++r)
			{
				if (recips[r] == who)
				{
					break;
				}
			}

			if (r < n_recips)
			{
				continue;
			}

			/*
			 * see if player can pemit to this dbref
			 */
			ok_to_do = mushconf.pemit_any;

			if (!ok_to_do && (Long_Fingers(player) || nearby(player, who) || Controls(player, who)))
			{
				ok_to_do = 1;
			}

			if (!ok_to_do && (isPlayer(who)) && mushconf.pemit_players)
			{
				if (!page_check(player, who))
				{
					continue;
				}

				ok_to_do = 1;
			}

			if (do_contents && !mushconf.pemit_any && !Controls(player, who))
			{
				ok_to_do = 0;
			}

			if (!ok_to_do)
			{
				notify(player, "You cannot do that.");
				continue;
			}

			/*
			 * fine, send the message
			 */
			if (do_contents && Has_contents(who))
			{
				notify_all_from_inside(who, player, message);
			}
			else
			{
				notify_with_cause(who, player, message);
			}

			/*
			 * avoid pemitting to this dbref again
			 */
			recips[n_recips] = who;
			++n_recips;
		}
	}

	XFREE(recips);
}

void do_pemit(dbref player, __attribute__((unused)) dbref cause, int key, char *recipient, char *message)
{
	dbref target, loc;
	char *buf2, *bp;
	int do_contents, ok_to_do, depth, pemit_flags;

	if (key & PEMIT_CONTENTS)
	{
		do_contents = 1;
		key &= ~PEMIT_CONTENTS;
	}
	else
	{
		do_contents = 0;
	}

	if (key & PEMIT_LIST)
	{
		do_pemit_list(player, recipient, message, do_contents);
		return;
	}

	pemit_flags = key & (PEMIT_HERE | PEMIT_ROOM | PEMIT_SPEECH | PEMIT_MOVE | PEMIT_HTML | PEMIT_SPOOF);
	key &= ~(PEMIT_HERE | PEMIT_ROOM | PEMIT_SPEECH | PEMIT_MOVE | PEMIT_HTML | PEMIT_SPOOF);
	ok_to_do = 0;

	switch (key)
	{
	case PEMIT_FSAY:
	case PEMIT_FPOSE:
	case PEMIT_FPOSE_NS:
	case PEMIT_FEMIT:
		target = match_affected(player, recipient);

		if (target == NOTHING)
		{
			return;
		}

		ok_to_do = 1;
		break;

	default:
		init_match(player, recipient, TYPE_PLAYER);
		match_everything(0);
		target = match_result();
	}

	switch (target)
	{
	case NOTHING:
		switch (key)
		{
		case PEMIT_WHISPER:
			notify(player, "Whisper to whom?");
			break;

		case PEMIT_PEMIT:
			notify(player, "Emit to whom?");
			break;

		case PEMIT_OEMIT:
			notify(player, "Emit except to whom?");
			break;

		default:
			notify(player, "Sorry.");
		}

		break;

	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;

	default:

		/*
		 * Enforce locality constraints
		 */
		if (!ok_to_do && (nearby(player, target) || Long_Fingers(player) || Controls(player, target)))
		{
			ok_to_do = 1;
		}

		if (!ok_to_do && (key == PEMIT_PEMIT) && (Typeof(target) == TYPE_PLAYER) && mushconf.pemit_players)
		{
			if (!page_check(player, target))
			{
				return;
			}

			ok_to_do = 1;
		}

		if (!ok_to_do && (!mushconf.pemit_any || (key != PEMIT_PEMIT)))
		{
			notify(player, "You are too far away to do that.");
			return;
		}

		if (do_contents && !Controls(player, target) && !mushconf.pemit_any)
		{
			notify(player, NOPERM_MESSAGE);
			return;
		}

		loc = where_is(target);

		switch (key)
		{
		case PEMIT_PEMIT:
			if (do_contents)
			{
				if (Has_contents(target))
				{
					if (pemit_flags & PEMIT_SPEECH)
					{
						notify_all_from_inside_speech(target, player, message);
					}
					else if (pemit_flags & PEMIT_MOVE)
					{
						notify_all_from_inside_move(target, player, message);
					}
					else
					{
						notify_all_from_inside(target, player, message);
					}
				}
			}
			else
			{
				notify_with_cause_extra(target, player, message, ((pemit_flags & PEMIT_HTML) ? MSG_HTML : 0) | ((pemit_flags & PEMIT_SPEECH) ? MSG_SPEECH : 0));
			}

			break;

		case PEMIT_OEMIT:
			notify_except(Location(target), player, target, ((pemit_flags & PEMIT_SPEECH) ? MSG_SPEECH : 0) | ((pemit_flags & PEMIT_MOVE) ? MSG_MOVE : 0), NULL, message);
			break;

		case PEMIT_WHISPER:
			if ((Unreal(player) && !Check_Heard(target, player)) || (Unreal(target) && !Check_Hears(player, target)))
			{
				notify(player, CANNOT_HEAR_MSG);
			}
			else
			{
				switch (*message)
				{
				case ':':
					message[0] = ' ';
					whisper_pose(player, target, message);
					break;

				case ';':
					message++;
					whisper_pose(player, target, message);
					break;

				case '"':
					message++;
					//[[fallthrough]];
					__attribute__((fallthrough));
				default:
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You whisper \"%s\" to %s.", message, Name(target));
					notify_check(target, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s whispers \"%s\"", Name(player), message);
				}

				if ((!mushconf.quiet_whisper) && !Wizard(player))
				{
					loc = where_is(player);

					if (loc != NOTHING)
					{
						buf2 = XMALLOC(LBUF_SIZE, "buf2");
						bp = buf2;
						safe_name(player, buf2, &bp);
						SAFE_LB_STR((char *)" whispers something to ", buf2, &bp);
						safe_name(target, buf2, &bp);
						*bp = '\0';
						notify_except2(loc, player, player, target, MSG_SPEECH, NULL, buf2);
						XFREE(buf2);
					}
				}
			}

			break;

		case PEMIT_FSAY:
			format_speech(((pemit_flags & PEMIT_SPOOF) ? target : player), target, loc, message, SAY_SAY);
			break;

		case PEMIT_FPOSE:
			format_speech(((pemit_flags & PEMIT_SPOOF) ? target : player), target, loc, message, SAY_POSE);
			break;

		case PEMIT_FPOSE_NS:
			format_speech(((pemit_flags & PEMIT_SPOOF) ? target : player), target, loc, message, SAY_POSE_NOSPC);
			break;

		case PEMIT_FEMIT:
			if ((pemit_flags & PEMIT_HERE) || (!(pemit_flags & ~PEMIT_SPOOF)))
				notify_all_from_inside_speech(loc, ((pemit_flags & PEMIT_SPOOF) ? target : player), message);

			if (pemit_flags & PEMIT_ROOM)
			{
				if ((Typeof(loc) == TYPE_ROOM) && (pemit_flags & PEMIT_HERE))
				{
					return;
				}

				depth = 0;

				while ((Typeof(loc) != TYPE_ROOM) && (depth++ < 20))
				{
					loc = Location(loc);

					if ((loc == NOTHING) || (loc == Location(loc)))
					{
						return;
					}
				}

				if (Typeof(loc) == TYPE_ROOM)
				{
					notify_all_from_inside_speech(loc, ((pemit_flags & PEMIT_SPOOF) ? target : player), message);
				}
			}

			break;
		}
	}
}
