/**
 * @file rob.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Commands dealing with giving/taking/killing things or money
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

void do_kill(dbref player, __attribute__((unused)) dbref cause, int key, char *what, char *costchar)
{
	dbref victim;
	char *buf1, *buf2, *bp;
	int cost;
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
		if ((Typeof(victim) != TYPE_PLAYER) && (Typeof(victim) != TYPE_THING))
		{
			notify(player, "Sorry, you can only kill players and things.");
			break;
		}

		if ((Haven(Location(victim)) && !Wizard(player)) || (controls(victim, Location(victim)) && !controls(player, Location(victim))) || Unkillable(victim))
		{
			notify(player, "Sorry.");
			break;
		}

		/*
		 * go for it
		 */
		cost = (int)strtol(costchar, (char **)NULL, 10);

		if (key == KILL_KILL)
		{
			if (cost < mushconf.killmin)
			{
				cost = mushconf.killmin;
			}

			if (cost > mushconf.killmax)
			{
				cost = mushconf.killmax;
			}

			/*
			 * see if it works
			 */

			if (!payfor(player, cost))
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
				return;
			}
		}
		else
		{
			cost = 0;
		}

		if (!((mushconf.killguarantee && ((int)(random_range(0, (mushconf.killguarantee) - 1)) < cost)) || (key == KILL_SLAY)) || Wizard(victim))
		{
			/*
			 * Failure: notify player and victim only
			 */
			notify(player, "Your murder attempt failed.");
			buf1 = XMALLOC(LBUF_SIZE, "buf1");
			bp = buf1;
			SAFE_SPRINTF(buf1, &bp, "%s tried to kill you!", Name(player));
			notify_with_cause(victim, player, buf1);

			if (Suspect(player))
			{
				XSTRCPY(buf1, Name(player));

				if (player == Owner(player))
				{
					raw_broadcast(WIZARD, "[Suspect] %s tried to kill %s(#%d).", buf1, Name(victim), victim);
				}
				else
				{
					buf2 = XMALLOC(LBUF_SIZE, "buf2");
					XSTRCPY(buf2, Name(Owner(player)));
					raw_broadcast(WIZARD, "[Suspect] %s <via %s(#%d)> tried to kill %s(#%d).", buf2, buf1, player, Name(victim), victim);
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

		if (Suspect(player))
		{
			XSTRCPY(buf1, Name(player));

			if (player == Owner(player))
			{
				raw_broadcast(WIZARD, "[Suspect] %s killed %s(#%d).", buf1, Name(victim), victim);
			}
			else
			{
				XSTRCPY(buf2, Name(Owner(player)));
				raw_broadcast(WIZARD, "[Suspect] %s <via %s(#%d)> killed %s(#%d).", buf2, buf1, player, Name(victim), victim);
			}
		}

		bp = buf1;
		SAFE_SPRINTF(buf1, &bp, "You killed %s!", Name(victim));
		bp = buf2;
		SAFE_SPRINTF(buf2, &bp, "killed %s!", Name(victim));

		if (Typeof(victim) != TYPE_PLAYER)
			if (halt_que(NOTHING, victim) > 0)
				if (!Quiet(victim))
				{
					notify(Owner(victim), "Halted.");
				}

		did_it(player, victim, A_KILL, buf1, A_OKILL, buf2, A_AKILL, 0, (char **)NULL, 0, MSG_PRESENCE);
		/*
		 * notify victim
		 */
		bp = buf1;
		SAFE_SPRINTF(buf1, &bp, "%s killed you!", Name(player));
		notify_with_cause(victim, player, buf1);

		/*
		 * Pay off the bonus
		 */

		if (key == KILL_KILL)
		{
			cost /= 2; /*
						* victim gets half
						*/

			if (Pennies(Owner(victim)) < mushconf.paylimit)
			{
				XSPRINTF(buf1, "Your insurance policy pays %d %s.", cost, mushconf.many_coins);
				notify(victim, buf1);
				giveto(Owner(victim), cost);
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

	if (thing == giver)
	{
		notify(giver, "You can't give yourself away!");
		return;
	}

	if (((Typeof(thing) != TYPE_THING) && (Typeof(thing) != TYPE_PLAYER)) || !(Enter_ok(recipient) || controls(giver, recipient)))
	{
		notify(giver, NOPERM_MESSAGE);
		return;
	}

	if (!could_doit(giver, thing, A_LGIVE))
	{
		sp = str = XMALLOC(LBUF_SIZE, "str");
		SAFE_LB_STR((char *)"You can't give ", str, &sp);
		safe_name(thing, str, &sp);
		SAFE_LB_STR((char *)" away.", str, &sp);
		*sp = '\0';
		did_it(giver, thing, A_GFAIL, str, A_OGFAIL, NULL, A_AGFAIL, 0, (char **)NULL, 0, MSG_MOVE);
		XFREE(str);
		return;
	}

	if (!could_doit(thing, recipient, A_LRECEIVE))
	{
		sp = str = XMALLOC(LBUF_SIZE, "str");
		safe_name(recipient, str, &sp);
		SAFE_LB_STR((char *)" doesn't want ", str, &sp);
		safe_name(thing, str, &sp);
		SAFE_LB_CHR('.', str, &sp);
		*sp = '\0';
		did_it(giver, recipient, A_RFAIL, str, A_ORFAIL, NULL, A_ARFAIL, 0, (char **)NULL, 0, MSG_MOVE);
		XFREE(str);
		return;
	}

	move_via_generic(thing, recipient, giver, 0);
	divest_object(thing);

	if (!(key & GIVE_QUIET))
	{
		str = XMALLOC(LBUF_SIZE, "str");
		XSTRCPY(str, Name(giver));
		notify_check(recipient, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gave you %s.", str, Name(thing));
		notify(giver, "Given.");
		notify_check(thing, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gave you to %s.", str, Name(recipient));
		XFREE(str);
	}

	did_it(giver, thing, A_DROP, NULL, A_ODROP, NULL, A_ADROP, 0, (char **)NULL, 0, MSG_MOVE);
	did_it(recipient, thing, A_SUCC, NULL, A_OSUCC, NULL, A_ASUCC, 0, (char **)NULL, 0, MSG_MOVE);
}

void give_money(dbref giver, dbref recipient, int key, int amount)
{
	dbref aowner;
	int cost, aflags, alen;
	char *str;

	/*
	 * do amount consistency check
	 */

	if (amount < 0 && !Steal(giver))
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You look through your pockets. Nope, no negative %s.", mushconf.many_coins);
		return;
	}

	if (!amount)
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You must specify a positive number of %s.", mushconf.many_coins);
		return;
	}

	if (!Wizard(giver))
	{
		if ((Typeof(recipient) == TYPE_PLAYER) && (Pennies(recipient) + amount > mushconf.paylimit))
		{
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "That player doesn't need that many %s!", mushconf.many_coins);
			return;
		}

		if (!could_doit(giver, recipient, A_LRECEIVE))
		{
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s won't take your money.", Name(recipient));
			return;
		}
	}

	/*
	 * try to do the give
	 */

	if (!payfor(giver, amount))
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have that many %s to give!", mushconf.many_coins);
		return;
	}

	/*
	 * Find out cost if an object
	 */

	if (Typeof(recipient) == TYPE_THING)
	{
		str = atr_pget(recipient, A_COST, &aowner, &aflags, &alen);
		cost = (int)strtol(str, (char **)NULL, 10);
		XFREE(str);

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
		 * Negative cost
		 */

		if (cost < 0)
		{
			return;
		}
	}
	else
	{
		cost = amount;
	}

	if (!(key & GIVE_QUIET))
	{
		if (amount == 1)
		{
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You give a %s to %s.", mushconf.one_coin, Name(recipient));
			notify_check(recipient, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gives you a %s.", Name(giver), mushconf.one_coin);
		}
		else
		{
			notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You give %d %s to %s.", amount, mushconf.many_coins, Name(recipient));
			notify_check(recipient, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s gives you %d %s.", Name(giver), amount, mushconf.many_coins);
		}
	}

	/*
	 * Report change given
	 */

	if ((amount - cost) == 1)
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You get 1 %s in change.", mushconf.one_coin);
		giveto(giver, 1);
	}
	else if (amount != cost)
	{
		notify_check(giver, giver, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You get %d %s in change.", (amount - cost), mushconf.many_coins);
		giveto(giver, (amount - cost));
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
	/*
	 * check recipient
	 */
	init_match(player, who, TYPE_PLAYER);
	match_neighbor();
	match_possession();
	match_me();

	if (Long_Fingers(player))
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
		give_money(player, recipient, key, (int)strtol(amnt, (char **)NULL, 10));
	}
	else
	{
		give_thing(player, recipient, key, amnt);
	}
}
