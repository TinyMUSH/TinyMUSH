/**
 * @file rob.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Commands dealing with giving/taking/killing things or money
 * @version 4.0
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
#include <limits.h>

void do_kill(dbref player, __attribute__((unused)) dbref cause, int key, char *what, char *costchar)
{
	dbref victim;
	char *buf1, *buf2, *bp;
	int cost;

	/* Cache frequently accessed values */
	const char *pname = Name(player);
	const char *safe_pname = pname ? pname : "Someone";
	const int killmin = mushconf.killmin;
	const int killmax = mushconf.killmax;
	const int killguarantee = mushconf.killguarantee;
	const char *many_coins = mushconf.many_coins;
	const int paylimit = mushconf.paylimit;

	init_match(player, what, TYPE_PLAYER);
	match_neighbor();
	match_me();
	match_here();

	if (Long_Fingers(player))
	{
		match_player();
		match_absolute();
	}

	victim = match_result();

	switch (victim)
	{
	case NOTHING:
		notify(player, "I don't see that player here.");
		break;

	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;

	default:
		/* Validate victim is a valid object */
		if (!Good_obj(victim))
		{
			notify(player, "Invalid target.");
			break;
		}

		/* Cache victim type and owner */
		int victim_type = Typeof(victim);
		dbref victim_owner = Owner(victim);
		const char *vname = Name(victim);
		const char *safe_vname = vname ? vname : "?Unknown?";

		if ((victim_type != TYPE_PLAYER) && (victim_type != TYPE_THING))
		{
			notify(player, "Sorry, you can only kill players and things.");
			break;
		}

		dbref victim_loc = Location(victim);
		if ((Haven(victim_loc) && !Wizard(player)) || (controls(victim, victim_loc) && !controls(player, victim_loc)) || Unkillable(victim))
		{
			notify(player, "Sorry.");
			break;
		}

		/*
		 * go for it - validate cost conversion
		 */
		char *endptr;
		long lval = strtol(costchar, &endptr, 10);
		if (endptr == costchar || *endptr != '\0' || lval < 0 || lval > INT_MAX)
		{
			cost = 0; /* Invalid conversion, use 0 */
		}
		else
		{
			cost = (int)lval;
		}

		if (key == KILL_KILL)
		{
			if (cost < killmin)
			{
				cost = killmin;
			}

			if (cost > killmax)
			{
				cost = killmax;
			}

			/*
			 * see if it works
			 */

			if (!payfor(player, cost))
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", many_coins);
				return;
			}
		}
		else
		{
			cost = 0;
		}

		/* Early exit if victim is wizard - no need for random calculation */
		if (Wizard(victim))
		{
			/* Wizard victims cannot be killed - go to failure path */
			goto kill_failed;
		}

		/* Protect against division by zero in killguarantee and validate random_range */
		int kill_success = 0;
		if (killguarantee > 0)
		{
			kill_success = ((int)(random_range(0, (killguarantee)-1)) < cost);
		}

		if (!(((killguarantee > 0) && kill_success) || (key == KILL_SLAY)))
		{
		kill_failed: /*
					  * Failure: notify player and victim only
					  */
			notify(player, "Your murder attempt failed.");
			buf1 = XMALLOC(LBUF_SIZE, "buf1");
			bp = buf1;
			XSAFESPRINTF(buf1, &bp, "%s tried to kill you!", safe_pname);
			notify_with_cause(victim, player, buf1);

			int is_suspect = Suspect(player);
			if (is_suspect)
			{
				XSTRCPY(buf1, safe_pname);
				dbref player_owner = Owner(player);

				if (player == player_owner)
				{
					raw_broadcast(WIZARD, "[Suspect] %s tried to kill %s(#%d).", buf1, safe_vname, victim);
				}
				else
				{
					buf2 = XMALLOC(LBUF_SIZE, "buf2");
					const char *oname = Name(player_owner);
					XSTRCPY(buf2, oname ? oname : "?Unknown?");
					raw_broadcast(WIZARD, "[Suspect] %s <via %s(#%d)> tried to kill %s(#%d).", buf2, buf1, player, safe_vname, victim);
					XFREE(buf2);
				}
			}

			XFREE(buf1);
			break;
		}

		/*
		 * Success!  You killed him
		 */
		buf1 = XMALLOC(LBUF_SIZE, "buf1");
		buf2 = XMALLOC(LBUF_SIZE, "buf2");

		int is_suspect = Suspect(player);
		if (is_suspect)
		{
			XSTRCPY(buf1, safe_pname);
			dbref player_owner = Owner(player);

			if (player == player_owner)
			{
				raw_broadcast(WIZARD, "[Suspect] %s killed %s(#%d).", buf1, safe_vname, victim);
			}
			else
			{
				const char *oname = Name(player_owner);
				XSTRCPY(buf2, oname ? oname : "?Unknown?");
				raw_broadcast(WIZARD, "[Suspect] %s <via %s(#%d)> killed %s(#%d).", buf2, buf1, player, safe_vname, victim);
			}
		}

		bp = buf1;
		XSAFESPRINTF(buf1, &bp, "You killed %s!", safe_vname);
		bp = buf2;
		XSAFESPRINTF(buf2, &bp, "killed %s!", safe_vname);

		if (victim_type != TYPE_PLAYER)
			if (halt_que(NOTHING, victim) > 0)
				if (!Quiet(victim))
				{
					notify(victim_owner, "Halted.");
				}

		did_it(player, victim, A_KILL, buf1, A_OKILL, buf2, A_AKILL, 0, (char **)NULL, 0, MSG_PRESENCE);
		/*
		 * notify victim
		 */
		bp = buf1;
		XSAFESPRINTF(buf1, &bp, "%s killed you!", safe_pname);
		notify_with_cause(victim, player, buf1);

		/*
		 * Pay off the bonus
		 */

		if (key == KILL_KILL)
		{
			cost /= 2; /*
						* victim gets half
						*/

			if (Pennies(victim_owner) < paylimit)
			{
				XSPRINTF(buf1, "Your insurance policy pays %d %s.", cost, many_coins);
				notify(victim, buf1);
				giveto(victim_owner, cost);
			}
			else
			{
				notify(victim, "Your insurance policy has been revoked.");
			}
		}

		XFREE(buf1);
		XFREE(buf2);
		/*
		 * send him home
		 */
		move_via_generic(victim, HOME, NOTHING, 0);
		divest_object(victim);
		break;
	}
}

/*
 * ---------------------------------------------------------------------------
 * * give_thing, give_money, do_give: Give away money or things.
 */

void give_thing(dbref giver, dbref recipient, int key, char *what)
{
	dbref thing;
	char *str, *sp;
	init_match(giver, what, TYPE_THING);
	match_possession();
	match_me();
	thing = match_result();

	switch (thing)
	{
	case NOTHING:
		notify(giver, "You don't have that!");
		return;

	case AMBIGUOUS:
		notify(giver, "I don't know which you mean!");
		return;
	}

	/* Validate thing is a valid object */
	if (!Good_obj(thing))
	{
		notify(giver, "Invalid object.");
		return;
	}

	if (thing == giver)
	{
		notify(giver, "You can't give yourself away!");
		return;
	}

	/* Validate recipient is a valid object */
	if (!Good_obj(recipient))
	{
		notify(giver, "Invalid recipient.");
		return;
	}

	int thing_type = Typeof(thing);
	if (((thing_type != TYPE_THING) && (thing_type != TYPE_PLAYER)) || !(Enter_ok(recipient) || controls(giver, recipient)))
	{
		notify(giver, NOPERM_MESSAGE);
		return;
	}

	if (!could_doit(giver, thing, A_LGIVE))
	{
		sp = str = XMALLOC(LBUF_SIZE, "str");
		XSAFELBSTR((char *)"You can't give ", str, &sp);
		safe_name(thing, str, &sp);
		XSAFELBSTR((char *)" away.", str, &sp);
		*sp = '\0';
		did_it(giver, thing, A_GFAIL, str, A_OGFAIL, NULL, A_AGFAIL, 0, (char **)NULL, 0, MSG_MOVE);
		XFREE(str);
		return;
	}

	if (!could_doit(thing, recipient, A_LRECEIVE))
	{
		sp = str = XMALLOC(LBUF_SIZE, "str");
		safe_name(recipient, str, &sp);
		XSAFELBSTR((char *)" doesn't want ", str, &sp);
		safe_name(thing, str, &sp);
		XSAFELBCHR('.', str, &sp);
		*sp = '\0';
		did_it(giver, recipient, A_RFAIL, str, A_ORFAIL, NULL, A_ARFAIL, 0, (char **)NULL, 0, MSG_MOVE);
		XFREE(str);
		return;
	}

	move_via_generic(thing, recipient, giver, 0);
	divest_object(thing);

	if (!(key & GIVE_QUIET))
	{
		const char *gname = Name(giver);
		const char *tname = Name(thing);
		const char *rname = Name(recipient);
		const char *safe_gname = gname ? gname : "Someone";
		const char *safe_tname = tname ? tname : "something";
		const char *safe_rname = rname ? rname : "someone";
		notify_check(recipient, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gave you %s.", safe_gname, safe_tname);
		notify(giver, "Given.");
		notify_check(thing, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gave you to %s.", safe_gname, safe_rname);
	}

	did_it(giver, thing, A_DROP, NULL, A_ODROP, NULL, A_ADROP, 0, (char **)NULL, 0, MSG_MOVE);
	did_it(recipient, thing, A_SUCC, NULL, A_OSUCC, NULL, A_ASUCC, 0, (char **)NULL, 0, MSG_MOVE);
}

void give_money(dbref giver, dbref recipient, int key, int amount)
{
	dbref aowner;
	int cost, aflags, alen;
	char *str;

	/* Validate recipient is a valid object */
	if (!Good_obj(recipient))
	{
		notify(giver, "Invalid recipient.");
		return;
	}

	/* Cache frequently accessed values to reduce global lookups */
	int recipient_type = Typeof(recipient);
	const char *many_coins = mushconf.many_coins;
	const char *one_coin = mushconf.one_coin;
	int paylimit = mushconf.paylimit;

	/*
	 * do amount consistency check
	 */

	if (amount < 0 && !Steal(giver))
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You look through your pockets. Nope, no negative %s.", many_coins);
		return;
	}

	if (!amount)
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You must specify a positive number of %s.", many_coins);
		return;
	}

	/* Protect against integer overflow with INT_MIN */
	if (amount == INT_MIN)
	{
		notify(giver, "Invalid amount.");
		return;
	}

	if (!Wizard(giver))
	{
		/* Protect against integer overflow in addition */
		if (recipient_type == TYPE_PLAYER && amount > 0)
		{
			int recipient_pennies = Pennies(recipient);
			if (recipient_pennies > paylimit - amount)
			{
				notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "That player doesn't need that many %s!", many_coins);
				return;
			}
		}

		if (!could_doit(giver, recipient, A_LRECEIVE))
		{
			const char *rname = Name(recipient);
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s won't take your money.", rname ? rname : "Someone");
			return;
		}
	}

	/*
	 * try to do the give
	 */

	if (!payfor(giver, amount))
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have that many %s to give!", many_coins);
		return;
	}

	/*
	 * Find out cost if an object
	 */

	if (recipient_type == TYPE_THING)
	{
		str = atr_pget(recipient, A_COST, &aowner, &aflags, &alen);
		char *endptr = NULL;
		long lval = 0;
		if (str && *str)
		{
			lval = strtol(str, &endptr, 10);
		}

		if (!str || endptr == str || (endptr && *endptr != '\0') || lval < INT_MIN || lval > INT_MAX)
		{
			cost = 0; /* Missing or invalid conversion */
		}
		else
		{
			cost = (int)lval;
		}
		if (str)
		{
			XFREE(str);
		}

		/*
		 * Can't afford it?
		 */

		if (amount < cost)
		{
			notify(giver, "Feeling poor today?");
			giveto(giver, amount);
			return;
		}

		/*
		 * Negative cost: refund and abort to avoid charging the giver
		 */

		if (cost < 0)
		{
			notify(giver, "That item cannot be purchased.");
			giveto(giver, amount);
			return;
		}
	}
	else
	{
		cost = amount;
	}

	if (!(key & GIVE_QUIET))
	{
		const char *gname = Name(giver);
		const char *rname = Name(recipient);
		const char *safe_gname = gname ? gname : "Someone";
		const char *safe_rname = rname ? rname : "someone";

		if (amount == 1)
		{
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You give a %s to %s.", one_coin, safe_rname);
			notify_check(recipient, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gives you a %s.", safe_gname, one_coin);
		}
		else
		{
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You give %d %s to %s.", amount, many_coins, safe_rname);
			notify_check(recipient, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gives you %d %s.", safe_gname, amount, many_coins);
		}
	}

	/*
	 * Report change given
	 */
	int change = amount - cost;
	if (change == 1)
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You get 1 %s in change.", one_coin);
		giveto(giver, 1);
	}
	else if (change > 0)
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You get %d %s in change.", change, many_coins);
		giveto(giver, change);
	}

	/*
	 * Transfer the money and run PAY attributes
	 */
	giveto(recipient, cost);
	did_it(giver, recipient, A_PAY, NULL, A_OPAY, NULL, A_APAY, 0, (char **)NULL, 0, MSG_PRESENCE);
	return;
}

void do_give(dbref player, __attribute__((unused)) dbref cause, int key, char *who, char *amnt)
{
	dbref recipient;
	int has_long_fingers = Long_Fingers(player);

	/*
	 * check recipient
	 */
	init_match(player, who, TYPE_PLAYER);
	match_neighbor();
	match_possession();
	match_me();

	if (has_long_fingers)
	{
		match_player();
		match_absolute();
	}

	recipient = match_result();

	switch (recipient)
	{
	case NOTHING:
		notify(player, "Give to whom?");
		return;

	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		return;
	}

	/* Validate recipient is a valid object */
	if (!Good_obj(recipient))
	{
		notify(player, "Invalid recipient.");
		return;
	}

	if (isExit(recipient))
	{
		notify(player, "You can't give anything to an exit.");
		return;
	}

	if (Guest(recipient))
	{
		notify(player, "You can't give anything to a Guest.");
		return;
	}

	if (is_number(amnt))
	{
		char *endptr;
		long lval = strtol(amnt, &endptr, 10);
		if (endptr == amnt || *endptr != '\0' || lval < INT_MIN || lval > INT_MAX)
		{
			notify(player, "Invalid amount.");
			return;
		}
		give_money(player, recipient, key, (int)lval);
	}
	else
	{
		give_thing(player, recipient, key, amnt);
	}
}
