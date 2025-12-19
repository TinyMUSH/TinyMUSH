/**
 * @file quota.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Quota tracking and commands that enforce building/resource limits
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

#include <string.h>
#include <limits.h>

/* Error message constants for better code reuse and localization */
static const char *MSG_INVALID_QUOTA = "Invalid quota value (must be a number).";
static const char *MSG_QUOTA_OUT_OF_RANGE = "Quota value out of range.";
static const char *MSG_ILLEGAL_QUOTA = "Illegal quota value.";

/* ---------------------------------------------------------------------------
 * load_quota: Load a quota into an array. Expects A_QUOTA or A_RQUOTA.
 */

void load_quota(int q_list[], dbref player, int qtype)
{
	int i, aowner, aflags, alen;
	char *quota_str, *p, *tokst;

	/* Validate player object to prevent crash on invalid dbrefs */
	if (!Good_obj(player))
	{
		memset(q_list, 0, 5 * sizeof(int));
		return;
	}

	quota_str = atr_get(player, qtype, &aowner, &aflags, &alen);

	if (!quota_str || !*quota_str)
	{
		memset(q_list, 0, 5 * sizeof(int));

		if (quota_str)
		{
			XFREE(quota_str);
		}
		return;
	}

	for (p = strtok_r(quota_str, " ", &tokst), i = 0; p && (i < 5); p = strtok_r(NULL, " ", &tokst), i++)
	{
		char *endptr;
		long val = strtol(p, &endptr, 10);

		/* Use 0 if conversion failed */
		q_list[i] = (endptr != p && *endptr == '\0') ? (int)val : 0;
	}

	/*
	 * Initialize remaining slots to 0 if less than 5 values were present
	 */
	if (i < 5)
	{
		memset(&q_list[i], 0, (5 - i) * sizeof(int));
	}

	XFREE(quota_str);
}

/* ---------------------------------------------------------------------------
 * save_quota: turns a quota array into an attribute.
 */
void save_quota(int q_list[], dbref player, int qtype)
{
	/* Validate player to prevent database corruption */
	if (!Good_obj(player))
	{
		return;
	}

	/* Use stack buffer instead of heap allocation for better performance */
	char buf[64];
	snprintf(buf, sizeof(buf), "%d %d %d %d %d", q_list[0], q_list[1], q_list[2], q_list[3], q_list[4]);
	atr_add_raw(player, qtype, buf);
}

/* ---------------------------------------------------------------------------
 * mung_quota, show_quota, do_quota: Manage quotas.
 */

void count_objquota(dbref player, int *aq, int *rq, int *eq, int *tq, int *pq)
{
	int a = 0, r = 0, e = 0, t = 0, p = 0;

	/* Validate player parameter */
	if (!Good_obj(player))
	{
		*aq = *rq = *eq = *tq = *pq = 0;
		return;
	}

	/*
	 * Validate database size to prevent infinite loops or invalid access
	 */
	if (mushstate.db_top < 0)
	{
		*aq = *rq = *eq = *tq = *pq = 0;
		return;
	}

	/* Cache quota values to avoid repeated memory accesses in loop */
	const int room_q = mushconf.room_quota;
	const int exit_q = mushconf.exit_quota;
	const int thing_q = mushconf.thing_quota;
	const int player_q = mushconf.player_quota;

	for (dbref i = 0; i < mushstate.db_top; i++)
	{
		/*
		 * Skip invalid objects and objects not owned by player.
		 * Also skip Going objects except rooms (rooms count until truly destroyed).
		 */
		if (!Good_obj(i) || (Owner(i) != player) || (Going(i) && !isRoom(i)))
		{
			continue;
		}

		switch (Typeof(i))
		{
		case TYPE_ROOM:
			/* Check for integer overflow before adding */
			if (a <= INT_MAX - room_q)
			{
				a += room_q;
			}
			r++;
			break;

		case TYPE_EXIT:
			if (a <= INT_MAX - exit_q)
			{
				a += exit_q;
			}
			e++;
			break;

		case TYPE_THING:
			if (a <= INT_MAX - thing_q)
			{
				a += thing_q;
			}
			t++;
			break;

		case TYPE_PLAYER:
			if (a <= INT_MAX - player_q)
			{
				a += player_q;
			}
			p++;
			break;

		default:
			/* Invalid/corrupted object type - skip without counting */
			break;
		}
	}
	*aq = a;
	*rq = r;
	*eq = e;
	*tq = t;
	*pq = p;
}

void adjust_quota(dbref player, int qtype, int value, int key)
{
	int aq, rq;
	int q_list[5], rq_list[5];

	/* Validate qtype to prevent array bounds violations */
	if (qtype < 0 || qtype >= 5)
	{
		return;
	}

	/* Validate player to prevent corruption cascade */
	if (!Good_obj(player))
	{
		return;
	}

	load_quota(q_list, player, A_QUOTA);
	load_quota(rq_list, player, A_RQUOTA);
	aq = q_list[qtype];
	rq = rq_list[qtype];

	/*
	 * Adjust values:
	 * - QUOTA_REM: Set relative quota to 'value', adjust absolute by delta
	 * - Otherwise: Set absolute quota to 'value', adjust relative by delta
	 * This maintains the invariant: available = absolute - relative
	 */

	if (key & QUOTA_REM)
	{
		/* Protect against overflow when adjusting absolute quota */
		int delta = value - rq;
		if (delta > 0 && aq > INT_MAX - delta)
		{
			aq = INT_MAX; /* Cap at maximum */
		}
		else if (delta < 0 && aq < INT_MIN - delta)
		{
			aq = INT_MIN; /* Cap at minimum */
		}
		else
		{
			aq += delta; /* Adjust absolute by change in relative */
		}
		rq = value; /* Set new relative quota */
	}
	else
	{
		/* Protect against overflow when adjusting relative quota */
		int delta = value - aq;
		if (delta > 0 && rq > INT_MAX - delta)
		{
			rq = INT_MAX; /* Cap at maximum */
		}
		else if (delta < 0 && rq < INT_MIN - delta)
		{
			rq = INT_MIN; /* Cap at minimum */
		}
		else
		{
			rq += delta; /* Adjust relative by change in absolute */
		}
		aq = value; /* Set new absolute quota */
	}

	/*
	 * Set both abs and relative quota
	 */
	q_list[qtype] = aq;
	rq_list[qtype] = rq;
	save_quota(q_list, player, A_QUOTA);
	save_quota(rq_list, player, A_RQUOTA);
}

void mung_quotas(dbref player, int key, int value)
{
	int xq, rooms, exits, things, players;
	int q_list[5], rq_list[5];

	if (!Good_obj(player))
	{
		return;
	}

	if (key & QUOTA_FIX)
	{
		/*
		 * Get value of stuff owned and current quota value,
		 * then adjust other quota values from that.
		 *
		 * QUOTA_TOT: Increment relative quota by used amount (grant ownership)
		 * Otherwise: Decrement absolute quota by used amount (charge for ownership)
		 */
		count_objquota(player, &xq, &rooms, &exits, &things, &players);

		if (key & QUOTA_TOT)
		{
			load_quota(rq_list, player, A_RQUOTA);
			/* Protect against overflow when adding used quota to relative */
			if (rq_list[QTYPE_ALL] <= INT_MAX - xq)
			{
				rq_list[QTYPE_ALL] += xq;
			}
			else
			{
				rq_list[QTYPE_ALL] = INT_MAX; /* Cap at maximum */
			}
			save_quota(rq_list, player, A_RQUOTA);
		}
		else
		{
			load_quota(q_list, player, A_QUOTA);
			/* Subtract used quota, ensuring values don't go negative */
			q_list[QTYPE_ALL] = (q_list[QTYPE_ALL] > xq) ? (q_list[QTYPE_ALL] - xq) : 0;
			q_list[QTYPE_ROOM] = (q_list[QTYPE_ROOM] > rooms) ? (q_list[QTYPE_ROOM] - rooms) : 0;
			q_list[QTYPE_EXIT] = (q_list[QTYPE_EXIT] > exits) ? (q_list[QTYPE_EXIT] - exits) : 0;
			q_list[QTYPE_THING] = (q_list[QTYPE_THING] > things) ? (q_list[QTYPE_THING] - things) : 0;
			q_list[QTYPE_PLAYER] = (q_list[QTYPE_PLAYER] > players) ? (q_list[QTYPE_PLAYER] - players) : 0;
			save_quota(q_list, player, A_QUOTA);
		}

		/* QUOTA_FIX is terminal, don't process other flags */
		return;
	}

	if (key & QUOTA_ROOM)
	{
		adjust_quota(player, QTYPE_ROOM, value, key);
	}
	else if (key & QUOTA_EXIT)
	{
		adjust_quota(player, QTYPE_EXIT, value, key);
	}
	else if (key & QUOTA_THING)
	{
		adjust_quota(player, QTYPE_THING, value, key);
	}
	else if (key & QUOTA_PLAYER)
	{
		adjust_quota(player, QTYPE_PLAYER, value, key);
	}
	else
	{
		adjust_quota(player, QTYPE_ALL, value, key);
	}
}

void show_quota(dbref player, dbref victim)
{
	int q_list[5], rq_list[5], dq_list[5], i;

	if (!Good_obj(player) || !Good_obj(victim))
	{
		return;
	}

	load_quota(q_list, victim, A_QUOTA);
	load_quota(rq_list, victim, A_RQUOTA);

	const char *vname = Name(victim) ? Name(victim) : "?INVALID?";
	const int typed = mushconf.typed_quotas;
	const int free_quota = Free_Quota(victim);

	/* Calculate available quota - optimize for Free_Quota case */
	if (free_quota && !typed)
	{
		/* Free quota, untyped: only need QTYPE_ALL */
		dq_list[QTYPE_ALL] = q_list[QTYPE_ALL] - rq_list[QTYPE_ALL];
	}
	else
	{
		/* Calculate all quota types (absolute - relative) */
		for (i = 0; i < 5; i++)
		{
			dq_list[i] = q_list[i] - rq_list[i];
		}
	}

	if (free_quota)
	{
		if (typed)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - N/A  %4d - N/A  %4d - N/A  %4d - N/A  %4d - N/A", vname, dq_list[QTYPE_ALL], dq_list[QTYPE_ROOM], dq_list[QTYPE_EXIT], dq_list[QTYPE_THING], dq_list[QTYPE_PLAYER]);
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - N/A", vname, dq_list[QTYPE_ALL]);
		}
	}
	else
	{
		if (typed)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - %3d  %4d - %3d  %4d - %3d  %4d - %3d  %4d - %3d", vname, dq_list[QTYPE_ALL], q_list[QTYPE_ALL], dq_list[QTYPE_ROOM], q_list[QTYPE_ROOM], dq_list[QTYPE_EXIT], q_list[QTYPE_EXIT], dq_list[QTYPE_THING], q_list[QTYPE_THING], dq_list[QTYPE_PLAYER], q_list[QTYPE_PLAYER]);
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - %3d", vname, dq_list[QTYPE_ALL], q_list[QTYPE_ALL]);
		}
	}
}

void show_quota_header(dbref player)
{
	if (!Good_obj(player))
	{
		return;
	}

	if (mushconf.typed_quotas)
		notify_quiet(player, "Name            : Quot - Lim  Room - Lim  Exit - Lim  Thng - Lim  Play - Lim");
	else
	{
		notify_quiet(player, "Name            : Quot - Lim");
	}
}

void do_quota(dbref player, __attribute__((unused)) dbref cause, int key, char *arg1, char *arg2)
{
	dbref who = NOTHING, i = NOTHING;
	char *name = NULL, *target = NULL;
	int set = 0, value = 0;

	if (!(mushconf.quotas || Can_Set_Quota(player)))
	{
		notify_quiet(player, "Quotas are not enabled.");
		return;
	}

	/* Validate switch combinations */
	if ((key & QUOTA_TOT) && (key & QUOTA_REM))
	{
		notify_quiet(player, "Illegal combination of switches.");
		return;
	}

	if ((key & QUOTA_FIX) && (key & QUOTA_SET))
	{
		notify_quiet(player, "Cannot use /fix and /set together.");
		return;
	}

	/* Check for multiple type flags - count number of active type bits */
	int type_count = 0;
	if (key & QUOTA_ROOM)
		type_count++;
	if (key & QUOTA_EXIT)
		type_count++;
	if (key & QUOTA_THING)
		type_count++;
	if (key & QUOTA_PLAYER)
		type_count++;

	if (type_count > 1)
	{
		notify_quiet(player, "Cannot specify multiple quota types.");
		return;
	}

	/*
	 * Show or set all quotas if requested
	 */

	if (key & QUOTA_ALL)
	{
		if (arg1 && *arg1)
		{
			char *endptr;
			long lval = strtol(arg1, &endptr, 10);

			if (endptr == arg1 || *endptr != '\0')
			{
				notify(player, MSG_INVALID_QUOTA);
				return;
			}

			if (lval > INT_MAX || lval < INT_MIN)
			{
				notify(player, MSG_QUOTA_OUT_OF_RANGE);
				return;
			}

			value = (int)lval;
			set = 1;

			if (value < 0)
			{
				notify(player, MSG_ILLEGAL_QUOTA);
				return;
			}
		}
		else if (key & (QUOTA_SET | QUOTA_FIX))
		{
			value = 0;
			set = 1;
		}

		if (set)
		{
			/* Verify permission to modify all quotas */
			if (!Can_Set_Quota(player))
			{
				notify_quiet(player, NOPERM_MESSAGE);
				return;
			}

			name = log_getname(player);
			log_write(LOG_WIZARD, "WIZ", "QUOTA", "%s changed everyone's quota.", name ? name : "?UNKNOWN?");
			if (name)
			{
				XFREE(name);
			}
		}

		show_quota_header(player);

		for (i = 0; i < mushstate.db_top; i++)
		{
			if (Good_obj(i) && isPlayer(i))
			{
				if (set)
				{
					mung_quotas(i, key, value);
				}

				show_quota(player, i);
			}
		}
		return;
	}

	/*
	 * Find out whose quota to show or set
	 */

	if (!arg1 || *arg1 == '\0')
	{
		/* Default to player's owner - validate player first */
		if (!Good_obj(player))
		{
			return;
		}
		who = Owner(player);
	}
	else
	{
		who = lookup_player(player, arg1, 1);

		if (!Good_obj(who) || !isPlayer(who))
		{
			notify_quiet(player, "Not found.");
			return;
		}
	}

	/*
	 * Make sure we have permission to do it
	 */

	if (!Can_Set_Quota(player))
	{
		if (arg2 && *arg2)
		{
			notify_quiet(player, NOPERM_MESSAGE);
			return;
		}

		/* Cache Owner(player) to avoid redundant call */
		dbref player_owner = Owner(player);
		if (player_owner != who)
		{
			notify_quiet(player, NOPERM_MESSAGE);
			return;
		}
	}

	if (arg2 && *arg2)
	{
		char *endptr;
		long lval = strtol(arg2, &endptr, 10);

		if (endptr == arg2 || *endptr != '\0')
		{
			notify(player, MSG_INVALID_QUOTA);
			return;
		}

		if (lval > INT_MAX || lval < INT_MIN)
		{
			notify(player, MSG_QUOTA_OUT_OF_RANGE);
			return;
		}

		set = 1;
		value = (int)lval;

		if (value < 0)
		{
			notify(player, MSG_ILLEGAL_QUOTA);
			return;
		}
	}
	else if (key & QUOTA_FIX)
	{
		set = 1;
		value = 0;
	}

	if (set)
	{
		name = log_getname(player);
		target = log_getname(who);
		log_write(LOG_WIZARD, "WIZ", "QUOTA", "%s changed the quota of %s", name ? name : "?UNKNOWN?", target ? target : "?UNKNOWN?");
		if (name)
		{
			XFREE(name);
		}
		if (target)
		{
			XFREE(target);
		}

		/* Revalidate object before modifying (race condition protection) */
		if (!Good_obj(who) || !isPlayer(who))
		{
			notify_quiet(player, "Target player no longer exists.");
			return;
		}

		mung_quotas(who, key, value);
	}

	show_quota_header(player);
	show_quota(player, who);
}
