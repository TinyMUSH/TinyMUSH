/**
 * @file look_content.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Score, inventory, entrances, sweep, decomp and VRML helpers
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

int did_attr(dbref player, dbref thing, int what)
{
	/*
	 * If the attribute exists, get it, notify the player, return 1.
	 * If not, return 0.
	 */
	char *buff;
	buff = master_attr(player, thing, what, NULL, 0, NULL);

	if (buff)
	{
		notify(player, buff);
		XFREE(buff);
		return 1;
	}

	return 0;
}

void look_exits(dbref player, dbref loc, const char *exit_name)
{
	dbref thing = NOTHING, parent = NOTHING;
	char *buff = NULL, *e = NULL, *buff1 = NULL, *e1 = NULL, *buf = NULL;
	int foundany = 0, lev = 0, isdark = 0;

	/*
	 * make sure location has exits
	 */

	if (!Good_obj(loc) || !Has_exits(loc))
	{
		return;
	}

	/*
	 * exit_name must be valid for notification
	 */
	if (!exit_name)
	{
		return;
	}

	/*
	 * Check to see if we're formatting exits in a player-specified way.
	 */

	if (did_attr(player, loc, A_LEXITS_FMT))
	{
		return;
	}

	/*
	 * make sure there is at least one visible exit
	 */
	foundany = 0;
	isdark = Darkened(player, loc);

	for (lev = 0, parent = loc; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
	{
		if (!Has_exits(parent))
		{
			continue;
		}

		for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
		{
			if (Can_See_Exit(player, thing, isdark))
			{
				foundany = 1;
				break;
			}
		}

		if (foundany)
		{
			break;
		}
	}

	if (!foundany)
	{
		return;
	}

	/*
	 * Display the list of exit names
	 */
	notify(player, exit_name);
	buff = XMALLOC(LBUF_SIZE, "buff");
	if (!buff)
	{
		return;
	}
	e = buff;
	buff1 = XMALLOC(LBUF_SIZE, "buff1");
	if (!buff1)
	{
		XFREE(buff);
		return;
	}
	e1 = buff1;

	for (lev = 0, parent = loc; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
	{
		if (Transparent(loc))
		{
			for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (Can_See_Exit(player, thing, isdark))
				{
					e = buff;
					safe_exit_name(thing, buff, &e);

					if (Location(thing) == NOTHING)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads nowhere.", buff);
					}
					else if (Location(thing) == AMBIGUOUS)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads somewhere.", buff);
					}
					else if (Location(thing) == HOME)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads home.", buff);
					}
					else if (Good_obj(Location(thing)))
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads to %s.", buff, Name(Location(thing)));
					}
					else
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads elsewhere.", buff);
					}
				}
			}
		}
		else
		{
			for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (Can_See_Exit(player, thing, isdark))
				{
					if (e != buff)
						XSAFESTRCAT(buff, &e, "  ", LBUF_SIZE);

					if (Html(player) && (mushconf.have_pueblo == 1))
					{
						e1 = buff1;
						safe_exit_name(thing, buff1, &e1);
						XSAFELBSTR((char *)"<a xch_cmd=\"", buff, &e);
						/*
						 * XXX Just stripping ansi isn't really enough.
						 */
						buf = ansi_strip_ansi(buff1);
						if (buf)
						{
							XSAFELBSTR(buf, buff, &e);
							XFREE(buf);
						}
						else
						{
							XSAFELBSTR(buff1, buff, &e);
						}
						XSAFELBSTR((char *)"\">", buff, &e);
						/*
						 * XXX The exit name needs to be HTML escaped.
						 */
						html_escape(buff1, buff, &e);
						XSAFELBSTR((char *)"</a>", buff, &e);
					}
					else
					{
						/*
						 * Append this exit to the list
						 */
						safe_exit_name(thing, buff, &e);
					}
				}
			}
		}
	}

	if (!(Transparent(loc)))
	{
		*e = '\0';
		if (mushconf.have_pueblo == 1 && Html(player))
		{
			XSAFELBSTR("\r\n", buff, &e);
			*e = '\0';
			notify_html(player, buff);
		}
		else
		{
			notify(player, buff);
		}
	}

	if (buff)
		XFREE(buff);
	if (buff1)
		XFREE(buff1);
}

void look_contents(dbref player, dbref loc, const char *contents_name, int style)
{
	dbref thing = NOTHING, can_see_loc = NOTHING;
	char *buff = NULL, *html_buff = NULL, *html_cp = NULL, *remote_num = NULL;

	/*
	 * Validate input parameters
	 */
	if (!Good_obj(loc) || !contents_name)
	{
		return;
	}

	/*
	 * Check if we're formatting contents in a player-specified way.
	 */

	if (did_attr(player, loc, A_LCON_FMT))
	{
		return;
	}

	if (mushconf.have_pueblo == 1)
	{
		html_buff = XMALLOC(LBUF_SIZE, "html_cp");
		if (!html_buff)
		{
			return;
		}
		html_cp = html_buff;
	}

	/*
	 * check to see if he can see the location
	 */
	can_see_loc = Sees_Always(player, loc);
	/*
	 * check to see if there is anything there
	 */
	for (thing = Contents(loc); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
	{
		if (Can_See(player, thing, can_see_loc))
		{
			/*
			 * something exists!  show him everything
			 */
			notify(player, contents_name);
			for (thing = Contents(loc); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (Can_See(player, thing, can_see_loc))
				{
					buff = unparse_object(player, thing, 1);
					if (!buff)
					{
						continue;
					}
					html_cp = html_buff;

					if (Html(player) && (mushconf.have_pueblo == 1))
					{
						XSAFELBSTR("<a xch_cmd=\"look ", html_buff, &html_cp);

						/**
						 * @bug Just stripping ansi isn't enough.
						 *
						 */

						switch (style)
						{
						case CONTENTS_LOCAL:
							XSAFELBSTR(PureName(thing), html_buff, &html_cp);
							break;

						case CONTENTS_NESTED:
							XSAFELBSTR(PureName(Location(thing)), html_buff, &html_cp);
							XSAFELBSTR("'s ", html_buff, &html_cp);
							XSAFELBSTR(PureName(thing), html_buff, &html_cp);
							break;

						case CONTENTS_REMOTE:
							remote_num = XASPRINTF("remote_num", "#%d", thing);
							XSAFELBSTR(remote_num, html_buff, &html_cp);
							XFREE(remote_num);
							break;

						default:
							break;
						}

						XSAFELBSTR("\">", html_buff, &html_cp);
						html_escape(buff, html_buff, &html_cp);
						XSAFELBSTR("</a>\r\n", html_buff, &html_cp);
						*html_cp = 0;
						notify_html(player, html_buff);
					}
					else
					{
						notify(player, buff);
					}

					XFREE(buff);
				}
			}
			break; /* we're done */
		}
	}

	if (html_buff)
	{
		XFREE(html_buff);
	}
}

void do_score(dbref player, dbref cause, int key)
{
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You have %d %s.", Pennies(player), (Pennies(player) == 1) ? mushconf.one_coin : mushconf.many_coins);
}

void do_inventory(dbref player, dbref cause, int key)
{
	char *buff = NULL, *e = NULL;
	dbref thing;

	if (Contents(player) == NOTHING)
	{
		notify(player, "You aren't carrying anything.");
	}
	else
	{
		notify(player, "You are carrying:");
		for (thing = Contents(player); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
		{
			buff = unparse_object(player, thing, 1);
			if (buff)
			{
				notify(player, buff);
				XFREE(buff);
			}
		}
	}

	if (Exits(player) != NOTHING)
	{
		notify(player, "Exits:");
		buff = XMALLOC(LBUF_SIZE, "buff");
		if (buff)
		{
			e = buff;
			for (thing = Exits(player); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (e != buff && e && *e)
				{
					if ((e - buff + 2) < LBUF_SIZE)
					{
						XSAFESTRNCAT(buff, &e, (char *)"  ", 2, LBUF_SIZE);
					}
				}

				safe_exit_name(thing, buff, &e);
			}
			if (e && buff)
			{
				*e = '\0';
			}
			notify(player, buff);
			XFREE(buff);
		}
	}

	do_score(player, player, 0);
}

void do_entrances(dbref player, dbref cause, int key, char *name)
{
	dbref thing = NOTHING, i = NOTHING, j = NOTHING;
	char *exit = NULL, *message = NULL;
	int control_thing = 0, count = 0, low_bound = 0, high_bound = 0;
	FWDLIST *fp = NULL;
	PROPDIR *pp = NULL;
	parse_range(&name, &low_bound, &high_bound);

	if (!name || !*name)
	{
		if (Has_location(player))
		{
			thing = Location(player);
		}
		else
		{
			thing = player;
		}

		if (!Good_obj(thing))
		{
			return;
		}
	}
	else
	{
		init_match(player, name, NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();

		if (!Good_obj(thing))
		{
			return;
		}
	}

	if (!payfor(player, mushconf.searchcost))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
		return;
	}

	message = XMALLOC(LBUF_SIZE, "message");
	control_thing = Examinable(player, thing);
	count = 0;

	for (i = low_bound; i <= high_bound; i++)
	{
		if (control_thing || Examinable(player, i))
		{
			switch (Typeof(i))
			{
			case TYPE_EXIT:
				if (Location(i) == thing)
				{
					exit = unparse_object(player, Exits(i), 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s (%s)", exit, Name(i));
					XFREE(exit);
					count++;
				}

				break;

			case TYPE_ROOM:
				if (Dropto(i) == thing)
				{
					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [dropto]", exit);
					XFREE(exit);
					count++;
				}

				break;

			case TYPE_THING:
			case TYPE_PLAYER:
				if (Home(i) == thing)
				{
					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [home]", exit);
					XFREE(exit);
					count++;
				}

				break;
			}

			/*
			 * Check for parents
			 */

			if (Parent(i) == thing)
			{
				exit = unparse_object(player, i, 0);
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [parent]", exit);
				XFREE(exit);
				count++;
			}

			/*
			 * Check for propdir
			 */

			if (H_Propdir(i))
			{
				pp = propdir_get(i);

				if (!pp)
				{
					continue;
				}

				for (j = 0; j < pp->count; j++)
				{
					if (pp->data[j] != thing)
					{
						continue;
					}

					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [propdir]", exit);
					XFREE(exit);
					count++;
				}
			}

			/*
			 * Check for forwarding
			 */

			if (H_Fwdlist(i))
			{
				fp = fwdlist_get(i);

				if (!fp)
				{
					continue;
				}

				for (j = 0; j < fp->count; j++)
				{
					if (fp->data[j] != thing)
					{
						continue;
					}

					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [forward]", exit);
					XFREE(exit);
					count++;
				}
			}
		}
	}

	if (message)
	{
		XFREE(message);
	}
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d entrance%s found.", count, (count == 1) ? "" : "s");
}

/* check the current location for bugs */

void sweep_check(dbref player, dbref what, int key, int is_loc)
{
	dbref aowner = NOTHING, parent = NOTHING;
	int canhear = 0, cancom = 0, isplayer = 0, ispuppet = 0, isconnected = 0, is_audible = 0, attr = 0, aflags = 0, alen = 0;
	int is_parent = 0, lev = 0;
	char *buf = NULL, *buf2 = NULL, *bp = NULL, *as = NULL, *buff = NULL, *s = NULL;
	ATTR *ap = NULL;

	if ((key & SWEEP_LISTEN) && ((isExit(what) || is_loc) && Audible(what)))
	{
		is_audible = 1;
	}

	if (key & SWEEP_LISTEN)
	{
		if (H_Listen(what) || Bouncer(what))
		{
			canhear = 1;
		}
		else if (Monitor(what))
		{
			buff = XMALLOC(LBUF_SIZE, "buff");
			if (!buff)
			{
				return;
			}

			for (attr = atr_head(what, &as); attr; attr = atr_next(&as))
			{
				ap = atr_num(attr);

				if (!ap || (ap->flags & AF_NOPROG))
				{
					continue;
				}

				atr_get_str(buff, what, attr, &aowner, &aflags, &alen);

				/*
				 * Make sure we can execute it
				 */

				if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
				{
					continue;
				}

				/*
				 * Make sure there's a : in it
				 */

				for (s = buff + 1; *s && (*s != ':'); s++)
					;

				if (*s == ':')
				{
					canhear = 1;
					break;
				}
			}

			XFREE(buff);
		}
	}

	if ((key & SWEEP_COMMANDS) && !isExit(what))
	{
		/*
		 * Look for commands on the object and parents too
		 */
		for (lev = 0, parent = what; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (Commer(parent))
			{
				cancom = 1;

				if (lev)
				{
					is_parent = 1;
					break;
				}
			}
		}
	}

	if (key & SWEEP_CONNECT)
	{
		if (Connected(what) || (Puppet(what) && Connected(Owner(what))) || (mushconf.player_listen && (Typeof(what) == TYPE_PLAYER) && canhear && Connected(Owner(what))))
		{
			isconnected = 1;
		}
	}

	if (key & SWEEP_PLAYER || isconnected)
	{
		if (Typeof(what) == TYPE_PLAYER)
		{
			isplayer = 1;
		}

		if (Puppet(what))
		{
			ispuppet = 1;
		}
	}

	if (canhear || cancom || isplayer || ispuppet || isconnected || is_audible || is_parent)
	{
		buf = XMALLOC(LBUF_SIZE, "buf");
		if (!buf)
		{
			return;
		}
		bp = buf;

		if (cancom)
		{
			XSAFELBSTR((char *)"commands ", buf, &bp);
		}

		if (canhear)
		{
			XSAFELBSTR((char *)"messages ", buf, &bp);
		}

		if (is_audible)
		{
			XSAFELBSTR((char *)"audible ", buf, &bp);
		}

		if (isplayer)
		{
			XSAFELBSTR((char *)"player ", buf, &bp);
		}

		if (ispuppet)
		{
			XSAFELBSTR((char *)"puppet(", buf, &bp);
			safe_name(Owner(what), buf, &bp);
			XSAFELBSTR((char *)") ", buf, &bp);
		}

		if (isconnected)
		{
			XSAFELBSTR((char *)"connected ", buf, &bp);
		}

		if (is_parent)
		{
			XSAFELBSTR((char *)"parent ", buf, &bp);
		}

		*--bp = '\0'; /* nuke the space at the end */

		if (!isExit(what))
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "  %s is listening. [%s]", Name(what), buf);
		}
		else
		{
			bp = buf2 = XMALLOC(LBUF_SIZE, "buf2");
			if (!buf2)
			{
				XFREE(buf);
				return;
			}
			safe_exit_name(what, buf2, &bp);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "  %s is listening. [%s]", buf2, buf);
			XFREE(buf2);
		}

		XFREE(buf);
	}
}

void do_sweep(dbref player, dbref cause, int key, char *where)
{
	dbref here, sweeploc;
	int where_key, what_key;
	where_key = key & (SWEEP_ME | SWEEP_HERE | SWEEP_EXITS);
	what_key = key & (SWEEP_COMMANDS | SWEEP_LISTEN | SWEEP_PLAYER | SWEEP_CONNECT);

	if (where && *where)
	{
		sweeploc = match_controlled(player, where);

		if (!Good_obj(sweeploc))
		{
			return;
		}
	}
	else
	{
		sweeploc = player;
	}

	if (!where_key)
	{
		where_key = -1;
	}

	if (!what_key)
	{
		what_key = -1;
	}
	else if (what_key == SWEEP_VERBOSE)
	{
		what_key = SWEEP_VERBOSE | SWEEP_COMMANDS;
	}

	/*
	 * Check my location.  If I have none or it is dark, check just me.
	 */

	if (where_key & SWEEP_HERE)
	{
		notify(player, "Sweeping location...");

		if (Has_location(sweeploc))
		{
			here = Location(sweeploc);

			if ((here == NOTHING) || (Dark(here) && !mushconf.sweep_dark && !Examinable(player, here)))
			{
				notify_quiet(player, "Sorry, it is dark here and you can't search for bugs");
				sweep_check(player, sweeploc, what_key, 0);
			}
			else
			{
				sweep_check(player, here, what_key, 1);

				for (here = Contents(here); here != NOTHING; here = Next(here))
				{
					sweep_check(player, here, what_key, 0);
				}
			}
		}
		else
		{
			sweep_check(player, sweeploc, what_key, 0);
		}
	}

	/*
	 * Check exits in my location
	 */

	if ((where_key & SWEEP_EXITS) && Has_location(sweeploc))
	{
		notify(player, "Sweeping exits...");

		for (here = Exits(Location(sweeploc)); (here != NOTHING) && (Next(here) != here); here = Next(here))
		{
			sweep_check(player, here, what_key, 0);
		}
	}

	/*
	 * Check my inventory
	 */

	if ((where_key & SWEEP_ME) && Has_contents(sweeploc))
	{
		notify(player, "Sweeping inventory...");

		for (here = Contents(sweeploc); (here != NOTHING) && (Next(here) != here); here = Next(here))
		{
			sweep_check(player, here, what_key, 0);
		}
	}

	/*
	 * Check carried exits
	 */

	if ((where_key & SWEEP_EXITS) && Has_exits(sweeploc))
	{
		notify(player, "Sweeping carried exits...");

		for (here = Exits(sweeploc); (here != NOTHING) && (Next(here) != here); here = Next(here))
		{
			sweep_check(player, here, what_key, 0);
		}
	}

	notify(player, "Sweep complete.");
}

/* Output the sequence of commands needed to duplicate the specified
 * object.  If you're moving things to another system, your mileage
 * will almost certainly vary.  (i.e. different flags, etc.)
 */

void do_decomp(dbref player, dbref cause, int key, char *name, char *qual)
{
	BOOLEXP *bexp = NULL;
	char *got = NULL, *thingname = NULL, *as = NULL, *ltext = NULL, *buff = NULL, *tbuf = NULL, *tmp = NULL, *buf = NULL;
	dbref aowner = NOTHING, thing = NOTHING;
	int val = 0, aflags = 0, alen = 0, ca = 0, wild_decomp = 0;
	ATTR *attr = NULL;
	NAMETAB *np = NULL;
	/*
	 * Check for obj/attr first
	 */
	olist_push();

	if (parse_attrib_wild(player, name, &thing, 0, 1, 0, 1))
	{
		wild_decomp = 1;
	}
	else
	{
		wild_decomp = 0;
		init_match(player, name, TYPE_THING);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();
	}

	/*
	 * get result
	 */
	if (thing == NOTHING)
	{
		olist_pop();
		return;
	}

	if (!Examinable(player, thing))
	{
		notify_quiet(player, "You can only decompile things you can examine.");
		olist_pop();
		return;
	}

	thingname = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
	bexp = boolexp_parse(player, thingname, 1);

	/*
	 * Determine the name of the thing to use in reporting and then
	 * * report the command to make the thing.
	 */

	if (qual && *qual)
	{
		XSTRCPY(thingname, qual);
	}
	else
	{
		switch (Typeof(thing))
		{
		case TYPE_THING:
			XSTRCPY(thingname, Name(thing));
			val = OBJECT_DEPOSIT(Pennies(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@create %s=%d", buf, val);
			XFREE(buf);
			break;

		case TYPE_ROOM:
			XSTRCPY(thingname, Name(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@dig/teleport %s", buf);
			XFREE(buf);
			XSTRCPY(thingname, "here");
			break;

		case TYPE_EXIT:
			XSTRCPY(thingname, Name(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@open %s", buf);
			XFREE(buf);
			got = thingname;
			safe_exit_name(thing, thingname, &got);
			break;

		case TYPE_PLAYER:
			XSTRCPY(thingname, "me");
			break;
		}
	}

	buf = ansi_strip_ansi(thingname);
	XSTRCPY(thingname, buf);
	XFREE(buf);

	/*
	 * Report the lock (if any)
	 */

	if (!wild_decomp && (bexp != TRUE_BOOLEXP))
	{
		buf = unparse_boolexp_decompile(player, bexp);
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock %s=%s", thingname, buf);
		XFREE(buf);
	}

	boolexp_free(bexp);
	/*
	 * Report attributes
	 */
	buff = XMALLOC(MBUF_SIZE, "buff");

	for (ca = (wild_decomp ? olist_first() : atr_head(thing, &as)); (wild_decomp) ? (ca != NOTHING) : (ca != (int)0); ca = (wild_decomp ? olist_next() : atr_next(&as)))
	{
		if ((ca == A_NAME) || (ca == A_LOCK))
		{
			continue;
		}

		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		if ((attr->flags & AF_NOCMD) && !(attr->flags & AF_IS_LOCK))
		{
			continue;
		}

		got = atr_get(thing, ca, &aowner, &aflags, &alen);

		if (aflags & AF_STRUCTURE)
		{
			tmp = replace_string(GENERIC_STRUCT_STRDELIM, mushconf.struct_dstr, got);
			XFREE(got);
			got = tmp;
		}

		if (Read_attr_all(player, thing, attr, aowner, aflags, 1))
		{
			if (attr->flags & AF_IS_LOCK)
			{
				bexp = boolexp_parse(player, got, 1);
				ltext = unparse_boolexp_decompile(player, bexp);
				boolexp_free(bexp);
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock/%s %s=%s", attr->name, thingname, ltext);
				XFREE(ltext);
			}
			else
			{
				XSTRCPY(buff, attr->name);

				if (key & DECOMP_PRETTY)
				{
					tbuf = XMALLOC(LBUF_SIZE, "tbuf");
					buf = XMALLOC(GBUF_SIZE, "buf");
					if (tbuf && buf)
					{
						XSNPRINTF(buf, GBUF_SIZE, "%c%s %s=", ((ca < A_USER_START) ? '@' : '&'), buff, thingname);
						pretty_print(tbuf, buf, got);
						notify(player, tbuf);
					}
					if (tbuf)
						XFREE(tbuf);
					if (buf)
						XFREE(buf);
				}
				else
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%c%s %s=%s", ((ca < A_USER_START) ? '@' : '&'), buff, thingname, got);
				}

				for (np = indiv_attraccess_nametab; np->name; np++)
				{
					if ((aflags & np->flag) && check_access(player, np->perm) && (!(np->perm & CA_NO_DECOMP)))
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@set %s/%s = %s", thingname, buff, np->name);
					}
				}

				if (aflags & AF_LOCK)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock %s/%s", thingname, buff);
				}
			}
		}

		XFREE(got);
	}

	XFREE(buff);

	if (!wild_decomp)
	{
		decompile_flags(player, thing, thingname);
		decompile_powers(player, thing, thingname);
	}

	/*
	 * If the object has a parent, report it
	 */

	if (!wild_decomp && (Parent(thing) != NOTHING))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@parent %s=#%d", thingname, Parent(thing));
	}

	/*
	 * If the object has a zone, report it
	 */

	if (!wild_decomp && (Zone(thing) != NOTHING))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@chzone %s=#%d", thingname, Zone(thing));
	}

	XFREE(thingname);
	olist_pop();
}

void show_vrml_url(dbref thing, dbref loc)
{
	char *vrml_url = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;

	/*
	 * If they don't care about HTML, just return.
	 */
	if (!Html(thing))
	{
		return;
	}

	vrml_url = atr_pget(loc, A_VRML_URL, &aowner, &aflags, &alen);

	if (vrml_url && *vrml_url)
	{
		char *vrml_message = NULL, *vrml_cp = NULL;
		vrml_message = XMALLOC(LBUF_SIZE, "vrml_message");
		if (vrml_message)
		{
			vrml_cp = vrml_message;
			XSAFELBSTR("<img xch_graph=load href=\"", vrml_message, &vrml_cp);
			XSAFELBSTR(vrml_url, vrml_message, &vrml_cp);
			XSAFELBSTR("\">", vrml_message, &vrml_cp);
			*vrml_cp = 0;
			notify_html(thing, vrml_message);
			XFREE(vrml_message);
		}
	}
	else
	{
		notify_html(thing, "<img xch_graph=hide>");
	}

	if (vrml_url)
		XFREE(vrml_url);
}
