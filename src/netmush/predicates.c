/**
 * @file predicates.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Predicate helpers that gate command execution and permission checks
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
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

/* ---------------------------------------------------------------------------
 * insert_first, remove_first: Insert or remove objects from lists.
 */

dbref insert_first(dbref head, dbref thing)
{
	s_Next(thing, head);
	return thing;
}

dbref remove_first(dbref head, dbref thing)
{
	dbref prev;
	int count = 0;

	if (head == thing)
	{
		return (Next(thing));
	}

	for (prev = head; (prev != NOTHING) && (Next(prev) != prev) && (count < mushstate.db_top); prev = Next(prev), count++)
	{
		if (Next(prev) == thing)
		{
			s_Next(prev, Next(thing));
			return head;
		}
	}
	return head;
}

/* ---------------------------------------------------------------------------
 * reverse_list: Reverse the order of members in a list.
 */

dbref reverse_list(dbref list)
{
	dbref newlist, rest;
	int count = 0;
	newlist = NOTHING;

	while (list != NOTHING)
	{
		if (++count > mushstate.db_top)
		{
			/* Circular list detected */
			break;
		}
		rest = Next(list);
		s_Next(list, newlist);
		newlist = list;
		list = rest;
	}

	return newlist;
}

/* ---------------------------------------------------------------------------
 * member - indicate if thing is in list
 */

int member(dbref thing, dbref list)
{
	int count = 0;
	for (; (list != NOTHING) && (Next(list) != list) && (count < mushstate.db_top); list = Next(list), count++)
	{
		if (list == thing)
		{
			return 1;
		}
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * is_integer, is_number: see if string contains just a number.
 */

bool is_integer(char *str)
{
	str = (char *) skip_whitespace(str);

	if (!*str)
	{
		return false; /* Empty string or only spaces */
	}

	if ((*str == '-') || (*str == '+'))
	{ /* Leading minus or plus */
		str++;

		if (!*str)
		{
			return false; /* but not if just a minus or plus */
		}
	}

	if (!isdigit(*str))
	{ /* Need at least 1 integer */
		return false;
	}

	while (*str && isdigit(*str))
	{
		str++; /* The number (int) */
	}

	while (*str && isspace(*str))
	{
		str++; /* Trailing spaces */
	}

	return *str ? false : true;
}

int is_number(char *str)
{
	int got_one;

	str = (char *) skip_whitespace(str);

	if (!*str)
	{
		return 0; /* Empty string or only spaces */
	}

	if ((*str == '-') || (*str == '+'))
	{ /* Leading minus or plus */
		str++;

		if (!*str)
		{
			return 0; /* but not if just a minus or plus */
		}
	}

	got_one = 0;

	if (isdigit(*str))
	{
		got_one = 1; /* Need at least one digit */
	}

	while (*str && isdigit(*str))
	{
		str++; /* The number (int) */
	}

	if (*str == '.')
	{
		str++; /* decimal point */
	}

	if (isdigit(*str))
	{
		got_one = 1; /* Need at least one digit */
	}

	while (*str && isdigit(*str))
	{
		str++; /* The number (fract) */
	}

	while (*str && isspace(*str))
	{
		str++; /* Trailing spaces */
	}

	return ((*str || !got_one) ? 0 : 1);
}

int could_doit(dbref player, dbref thing, int locknum)
{
	char *key;
	dbref aowner;
	int aflags, alen, doit;

	/*
	 * no if nonplayer tries to get key
	 */

	if (!isPlayer(player) && Key(thing))
	{
		return 0;
	}

	if (Pass_Locks(player))
	{
		return 1;
	}

	key = atr_get(thing, locknum, &aowner, &aflags, &alen);
	doit = boolexp_eval_atr(player, thing, thing, key);
	XFREE(key);
	return doit;
}

int canpayquota(dbref player, dbref who, int cost, int objtype)
{
	register int quota;
	int q_list[5];

	/*
	 * If no cost, succeed
	 */

	if (cost <= 0)
	{
		return 1;
	}

	/*
	 * determine basic quota
	 */
	load_quota(q_list, Owner(who), A_RQUOTA);
	quota = q_list[QTYPE_ALL];
	/*
	 * enough to build?  Wizards always have enough.
	 */
	quota -= cost;

	if ((quota < 0) && !Free_Quota(who) && !Free_Quota(Owner(who)))
	{
		return 0;
	}

	if (mushconf.typed_quotas)
	{
		quota = q_list[type_quota(objtype)];

		if ((quota <= 0) && !Free_Quota(who) && !Free_Quota(Owner(who)))
		{
			return 0;
		}
	}

	return 1;
}

int pay_quota(dbref who, int cost, int objtype)
{
	/*
	 * If no cost, succeed.  Negative costs /must/ be managed, however
	 */
	if (cost == 0)
	{
		return 1;
	}

	add_quota(who, -cost, type_quota(objtype));
	return 1;
}

int canpayfees(dbref player, dbref who, int pennies, int quota, int objtype)
{
	if (!Wizard(who) && !Wizard(Owner(who)) && !Free_Money(who) && !Free_Money(Owner(who)) && (Pennies(Owner(who)) < pennies))
	{
		if (player == who)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Sorry, you don't have enough %s.", mushconf.many_coins);
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Sorry, that player doesn't have enough %s.", mushconf.many_coins);
		}

		return 0;
	}

	if (mushconf.quotas)
	{
		if (!canpayquota(player, who, quota, objtype))
		{
			if (player == who)
			{
				notify(player, "Sorry, your building contract has run out.");
			}
			else
			{
				notify(player, "Sorry, that player's building contract has run out.");
			}

			return 0;
		}
	}

	return 1;
}

int type_quota(int objtype)
{
	int qtype;

	/*
	 * determine typed quota
	 */

	switch (objtype)
	{
	case TYPE_ROOM:
		qtype = QTYPE_ROOM;
		break;

	case TYPE_EXIT:
		qtype = QTYPE_EXIT;
		break;

	case TYPE_PLAYER:
		qtype = QTYPE_PLAYER;
		break;

	default:
		qtype = QTYPE_THING;
	}

	return (qtype);
}

int payfor(dbref who, dbref cost)
{
	dbref tmp;

	if (Wizard(who) || Wizard(Owner(who)) || Free_Money(who) || Free_Money(Owner(who)) || Immortal(who) || Immortal(Owner(who)))
	{
		return 1;
	}

	who = Owner(who);
	tmp = Pennies(who);

	/* Validate Pennies() returned a reasonable value */
	if (tmp < 0)
	{
		return 0;
	}

	if (tmp >= cost)
	{
		s_Pennies(who, tmp - cost);
		return 1;
	}

	return 0;
}

int payfees(dbref who, int pennies, int quota, int objtype)
{
	/*
	 * You /must/ have called canpayfees() first.  If not, your
	 * * database will be eaten by rabid squirrels.
	 */
	if (mushconf.quotas)
	{
		pay_quota(who, quota, objtype);
	}

	return payfor(who, pennies);
}

void add_quota(dbref who, int payment, int type)
{
	int q_list[5];
	load_quota(q_list, Owner(who), A_RQUOTA);
	q_list[QTYPE_ALL] += payment;

	if (mushconf.typed_quotas)
	{
		q_list[type] += payment;
	}

	save_quota(q_list, Owner(who), A_RQUOTA);
}

void giveto(dbref who, int pennies)
{
	if (Wizard(who) || Wizard(Owner(who)) || Free_Money(who) || Free_Money(Owner(who)) || Immortal(who) || Immortal(Owner(who)))
	{
		return;
	}

	who = Owner(who);
	s_Pennies(who, Pennies(who) + pennies);
}

int ok_name(const char *name)
{
	const char *cp;
	char *purename = ansi_strip_ansi(name);
	int i;

	/*
	 * Disallow pure ANSI names
	 */

	if (strlen(purename) == 0)
	{
		XFREE(purename);
		return (0);
	}

	/*
	 * Disallow leading spaces
	 */

	if (isspace(*purename))
	{
		XFREE(purename);
		return (0);
	}

	/*
	 * Only printable characters outside of escape codes
	 */

	for (cp = purename; *cp; ++cp)
	{
		if (!isprint(*cp))
		{
			XFREE(purename);
			return (0);
		}
	}

	/*
	 * Disallow trailing spaces
	 */
	if (cp > purename)
	{
		cp--;

		if (isspace(*cp))
		{
			XFREE(purename);
			return (0);
		}
	}

	/*
	 * Exclude names that start with or contain certain magic cookies
	 */
	i = (*purename != LOOKUP_TOKEN && *purename != NUMBER_TOKEN && *purename != NOT_TOKEN && !strchr(name, ARG_DELIMITER) && !strchr(name, AND_TOKEN) && !strchr(name, OR_TOKEN) && string_compare(purename, "me") && string_compare(purename, "home") && string_compare(purename, "here"));
	XFREE(purename);
	return (i);
}

int ok_player_name(const char *name)
{
	const char *cp, *good_chars;

	/*
	 * Good name for a thing, not too long, and we either don't have a
	 * * minimum player name length, or we're sufficiently long.
	 */

	if (!ok_name(name) || (strlen(name) >= (size_t)mushconf.max_command_args) || (mushconf.player_name_min && (strlen(name) < (size_t)mushconf.player_name_min)))
	{
		return 0;
	}

	if (mushconf.name_spaces || mushstate.standalone)
	{
		good_chars = " `$_-.,'";
	}
	else
	{
		good_chars = "`$_-.,'";
	}

	/*
	 * Make sure name only contains legal characters
	 */

	for (cp = name; cp && *cp; cp++)
	{
		if (isalnum(*cp))
		{
			continue;
		}

		if (!strchr(good_chars, *cp))
		{
			return 0;
		}
	}

	return 1;
}

int ok_attr_name(const char *attrname)
{
	const char *scan;

	if (!isalpha(*attrname) && (*attrname != '_'))
	{
		return 0;
	}

	for (scan = attrname; *scan; scan++)
	{
		if (isalnum(*scan))
		{
			continue;
		}

		if (!strchr("'?!`/-_.@#$^&~=+<>()%", *scan))
		{
			return 0;
		}
	}

	return 1;
}

int ok_password(const char *password, dbref player)
{
	const char *scan;
	int num_upper = 0;
	int num_special = 0;
	int num_lower = 0;

	if (*password == '\0')
	{
		if (!mushstate.standalone)
			notify_quiet(player, "Null passwords are not allowed.");

		return 0;
	}

	for (scan = password; *scan; scan++)
	{
		if (!(isprint(*scan) && !isspace(*scan)))
		{
			if (!mushstate.standalone)
				notify_quiet(player, "Illegal character in password.");

			return 0;
		}

		if (isupper(*scan))
		{
			num_upper++;
		}
		else if (islower(*scan))
		{
			num_lower++;
		}
		else if ((*scan != '\'') && (*scan != '-'))
		{
			num_special++;
		}
	}

	/*
	 * Needed.  Change it if you like, but be sure yours is the same.
	 */
	if ((strlen(password) == 13) && (password[0] == 'X') && (password[1] == 'X'))
	{
		if (!mushstate.standalone)
			notify_quiet(player, "Please choose another password.");

		return 0;
	}

	if (!mushstate.standalone && mushconf.safer_passwords)
	{
		if (num_upper < 1)
		{
			notify_quiet(player, "The password must contain at least one capital letter.");
			return 0;
		}

		if (num_lower < 1)
		{
			notify_quiet(player, "The password must contain at least one lowercase letter.");
			return 0;
		}

		if (num_special < 1)
		{
			notify_quiet(player, "The password must contain at least one number or a symbol other than the apostrophe or dash.");
			return 0;
		}
	}

	return 1;
}

/* ---------------------------------------------------------------------------
 * handle_ears: Generate the 'grows ears' and 'loses ears' messages.
 */

void handle_ears(dbref thing, int could_hear, int can_hear)
{
	char *buff, *bp;
	int gender;

	if (could_hear != can_hear)
	{
		bp = buff = XMALLOC(LBUF_SIZE, "buff");

		if (isExit(thing))
		{
			safe_exit_name(thing, buff, &bp);
		}
		else
		{
			safe_name(thing, buff, &bp);
		}

		gender = get_gender(thing);
		notify_check(thing, thing, (MSG_ME | MSG_NBR | MSG_LOC | MSG_INV), "%s %s %s listening.", buff, ((gender == 4) ? "are" : "is"), (can_hear ? "now" : "no longer"));
		XFREE(buff);
	}
}
