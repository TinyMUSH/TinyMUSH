/**
 * @file look_content.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Score, inventory, entrances, and look helper functions
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
							/*
							 * Escape the ANSI-stripped command for HTML attribute context.
							 */
							html_escape(buf, buff, &e);
							XFREE(buf);
						}
						else
						{
							/*
							 * Fallback to escaping the original exit name.
							 */
							html_escape(buff1, buff, &e);
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

						switch (style)
						{
						case CONTENTS_LOCAL:
							html_escape(PureName(thing), html_buff, &html_cp);
							break;

						case CONTENTS_NESTED:
							html_escape(PureName(Location(thing)), html_buff, &html_cp);
							XSAFELBSTR("'s ", html_buff, &html_cp);
							html_escape(PureName(thing), html_buff, &html_cp);
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

