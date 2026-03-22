/**
 * @file look_sweep.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Sweep command helpers (sweep_check, do_sweep)
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

