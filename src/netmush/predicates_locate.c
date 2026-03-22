/**
 * @file predicates_locate.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object location and matching utilities
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
 */

dbref promote_dflt(dbref old, dbref new)
{
	switch (new)
	{
	case NOPERM:
		return NOPERM;

	case AMBIGUOUS:
		if (old == NOPERM)
		{
			return old;
		}
		else
		{
			return new;
		}
	}

	if ((old == NOPERM) || (old == AMBIGUOUS))
	{
		return old;
	}

	return NOTHING;
}

dbref match_possessed(dbref player, dbref thing, char *target, dbref dflt, int check_enter)
{
	dbref result, result1;
	int control;
	char *buff, *start, *place, *s1, *d1, *temp;

	/*
	 * First, check normally
	 */

	if (Good_obj(dflt))
	{
		return dflt;
	}

	/*
	 * Didn't find it directly.  Recursively do a contents check
	 */
	start = target;

	while (*target)
	{
		/*
		 * Fail if no ' characters
		 */
		place = target;
		target = strchr(place, '\'');

		if ((target == NULL) || !*target)
		{
			return dflt;
		}

		/*
		 * If string started with a ', skip past it
		 */

		if (place == target)
		{
			target++;
			continue;
		}

		/*
		 * If next character is not an s or a space, skip past
		 */
		temp = target++;

		if (!*target)
		{
			return dflt;
		}

		if ((*target != 's') && (*target != 'S') && (*target != ' '))
		{
			continue;
		}

		/*
		 * If character was not a space make sure the following
		 * * character is a space.
		 */

		if (*target != ' ')
		{
			target++;

			if (!*target)
			{
				return dflt;
			}

			if (*target != ' ')
			{
				continue;
			}
		}

		/*
		 * Copy the container name to a new buffer so we can
		 * * terminate it.
		 */
		buff = XMALLOC(LBUF_SIZE, "buff");

		for (s1 = start, d1 = buff; *s1 && (s1 < temp); *d1++ = (*s1++))
			;

		*d1 = '\0';
		/*
		 * Look for the container here and in our inventory.  Skip
		 * * past if we can't find it.
		 */
		init_match(thing, buff, NOTYPE);

		if (player == thing)
		{
			match_neighbor();
			match_possession();
		}
		else
		{
			match_possession();
		}

		result1 = match_result();
		XFREE(buff);

		if (!Good_obj(result1))
		{
			dflt = promote_dflt(dflt, result1);
			continue;
		}

		/*
		 * If we don't control it and it is either dark or opaque,
		 * * skip past.
		 */
		control = Controls(player, result1);

		if ((Dark(result1) || Opaque(result1)) && !control)
		{
			dflt = promote_dflt(dflt, NOTHING);
			continue;
		}

		/*
		 * Validate object has the ENTER bit set, if requested
		 */

		if ((check_enter) && !Enter_ok(result1) && !control)
		{
			dflt = promote_dflt(dflt, NOPERM);
			continue;
		}

		/*
		 * Look for the object in the container
		 */
		init_match(result1, target, NOTYPE);
		match_possession();
		result = match_result();

		/* Limit recursion depth to prevent stack overflow */
		if (--check_enter >= -mushconf.parent_nest_lim)
		{
			result = match_possessed(player, result1, target, result, check_enter);
		}

		if (Good_obj(result))
		{
			return result;
		}

		dflt = promote_dflt(dflt, result);
	}

	return dflt;
}

/* ---------------------------------------------------------------------------
 * parse_range: break up <what>,<low>,<high> syntax
 */

void parse_range(char **name, dbref *low_bound, dbref *high_bound)
{
	char *buff1, *buff2;
	buff1 = *name;

	if (buff1 && *buff1)
	{
		*name = parse_to(&buff1, ',', EV_STRIP_TS);
	}

	if (buff1 && *buff1)
	{
		buff2 = parse_to(&buff1, ',', EV_STRIP_TS);

		if (buff1 && *buff1)
		{
			buff1 = (char *) skip_whitespace(buff1);

			if (*buff1 == NUMBER_TOKEN)
			{
				buff1++;
			}

			errno = 0;
			long temp_val = strtol(buff1, (char **)NULL, 10);
			if (errno == ERANGE || temp_val > INT_MAX || temp_val < INT_MIN)
			{
				*high_bound = mushstate.db_top - 1;
			}
			else
			{
				*high_bound = (int)temp_val;
				if (*high_bound >= mushstate.db_top)
				{
					*high_bound = mushstate.db_top - 1;
				}
			}
		}
		else
		{
			*high_bound = mushstate.db_top - 1;
		}

		buff2 = (char *) skip_whitespace(buff2);

		if (*buff2 == NUMBER_TOKEN)
		{
			buff2++;
		}

		errno = 0;
		long temp_low = strtol(buff2, (char **)NULL, 10);
		if (errno == ERANGE || temp_low > INT_MAX || temp_low < INT_MIN)
		{
			*low_bound = 0;
		}
		else
		{
			*low_bound = (int)temp_low;
			if (*low_bound < 0)
			{
				*low_bound = 0;
			}
		}
	}
	else
	{
		*low_bound = 0;
		*high_bound = mushstate.db_top - 1;
	}
}

int parse_thing_slash(dbref player, char *thing, char **after, dbref *it)
{
	char *str;

	/*
	 * get name up to /
	 */
	for (str = thing; *str && (*str != '/'); str++)
		;

	/*
	 * If no / in string, return failure
	 */

	if (!*str)
	{
		*after = NULL;
		*it = NOTHING;
		return 0;
	}

	*str++ = '\0';
	*after = str;
	/*
	 * Look for the object
	 */
	init_match(player, thing, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	*it = match_result();
	/*
	 * Return status of search
	 */
	return (Good_obj(*it));
}

int get_obj_and_lock(dbref player, char *what, dbref *it, ATTR **attr, char *errmsg, char **bufc)
{
	char *str, *tbuf;
	int anum;
	tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	XSTRCPY(tbuf, what);

	if (parse_thing_slash(player, tbuf, &str, it))
	{
		/*
		 * <obj>/<lock> syntax, use the named lock
		 */
		anum = search_nametab(player, lock_sw, str);

		if (anum == -1)
		{
			XFREE(tbuf);
			XSAFELBSTR("#-1 LOCK NOT FOUND", errmsg, bufc);
			return 0;
		}
	}
	else
	{
		/*
		 * Not <obj>/<lock>, do a normal get of the default lock
		 */
		*it = match_thing(player, what);

		if (!Good_obj(*it))
		{
			XFREE(tbuf);
			XSAFELBSTR("#-1 NOT FOUND", errmsg, bufc);
			return 0;
		}

		anum = A_LOCK;
	}

	/*
	 * Get the attribute definition, fail if not found
	 */
	XFREE(tbuf);
	*attr = atr_num(anum);

	if (!(*attr))
	{
		XSAFELBSTR("#-1 LOCK NOT FOUND", errmsg, bufc);
		return 0;
	}

	return 1;
}

/* ---------------------------------------------------------------------------
 * where_is: Returns place where obj is linked into a list.
 * ie. location for players/things, source for exits, NOTHING for rooms.
 */

dbref where_is(dbref what)
{
	dbref loc;

	if (!Good_obj(what))
	{
		return NOTHING;
	}

	switch (Typeof(what))
	{
	case TYPE_PLAYER:
	case TYPE_THING:
	case TYPE_ZONE:
		loc = Location(what);
		break;

	case TYPE_EXIT:
		loc = Exits(what);
		break;

	default:
		loc = NOTHING;
		break;
	}

	return loc;
}

/* ---------------------------------------------------------------------------
 * where_room: Return room containing player, or NOTHING if no room or
 * recursion exceeded.  If player is a room, returns itself.
 */

dbref where_room(dbref what)
{
	int count;
	dbref loc;

	for (count = mushconf.ntfy_nest_lim; count > 0; count--)
	{
		if (!Good_obj(what))
		{
			break;
		}

		if (isRoom(what))
		{
			return what;
		}

		if (!Has_location(what))
		{
			break;
		}

		loc = Location(what);
		if (loc == what)
		{
			break;
		}
		what = loc;
	}

	return NOTHING;
}

int locatable(dbref player, dbref it, dbref cause)
{
	dbref loc_it, room_it, player_loc;
	int findable_room;

	/*
	 * No sense if trying to locate a bad object
	 */

	if (!Good_obj(it))
	{
		return 0;
	}

	loc_it = where_is(it);
	player_loc = where_is(player);

	/*
	 * Succeed if we can examine the target, if we are the target, if
	 * * we can examine the location, if a wizard caused the lookup,
	 * * or if the target caused the lookup.
	 */

	if (Examinable(player, it) || Find_Unfindable(player) || (loc_it == player) || ((loc_it != NOTHING) && (Examinable(player, loc_it) || loc_it == player_loc)) || Wizard(cause) || (it == cause))
	{
		return 1;
	}

	room_it = where_room(it);

	if (Good_obj(room_it))
	{
		findable_room = !Hideout(room_it);
	}
	else
	{
		findable_room = 1;
	}

	/*
	 * Succeed if we control the containing room or if the target is
	 * * findable and the containing room is not unfindable.
	 */

	if (((room_it != NOTHING) && Examinable(player, room_it)) || Find_Unfindable(player) || (Findable(it) && findable_room))
	{
		return 1;
	}

	/*
	 * We can't do it.
	 */
	return 0;
}

/* ---------------------------------------------------------------------------
 * nearby: Check if thing is nearby player (in inventory, in same room, or
 * IS the room.
 */

int nearby(dbref player, dbref thing)
{
	int thing_loc, player_loc;

	if (!Good_obj(player) || !Good_obj(thing))
	{
		return 0;
	}

	thing_loc = where_is(thing);

	if (thing_loc == player)
	{
		return 1;
	}

	player_loc = where_is(player);

	if ((thing_loc == player_loc) || (thing == player_loc))
	{
		return 1;
	}

	return 0;
}
