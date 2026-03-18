/**
 * @file funobj_search.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object-focused built-ins: object search, memory stats, and type inspection
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
#include <string.h>

/**
 * @brief Returns the dbref number of the object, which must be in the same
 *         room as the object executing num.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_num(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, match_thing(player, fargs[0]), LBUF_SIZE);
}

/**
 * @brief Given the partial name of a player, it returns that player's dbref
 *        number.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_pmatch(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *name = NULL, *temp = NULL, *tp = NULL;
	dbref thing = NOTHING;
	dbref *p_ptr = NULL;

	/**
	 * If we have a valid dbref, it's okay if it's a player.
	 *
	 */
	if ((*fargs[0] == NUMBER_TOKEN) && fargs[0][1])
	{
		thing = parse_dbref(fargs[0] + 1);

		if (Good_obj(thing) && isPlayer(thing))
		{
			XSAFELBCHR('#', buff, bufc);
			XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
		}
		else
		{
			XSAFENOTHING(buff, bufc);
		}

		return;
	}

	/**
	 * If we have *name, just advance past the *; it doesn't matter
	 *
	 */
	name = fargs[0];

	if (*fargs[0] == LOOKUP_TOKEN)
	{
		do
		{
			name++;
		} while (isspace(*name));
	}

	/**
	 * Look up the full name
	 *
	 */
	tp = temp = XMALLOC(LBUF_SIZE, "temp");
	XSAFELBSTR(name, temp, &tp);

	for (tp = temp; *tp; tp++)
	{
		*tp = tolower(*tp);
	}

	p_ptr = (int *)hashfind(temp, &mushstate.player_htab);
	XFREE(temp);

	if (p_ptr)
	{
		/**
		 * We've got it. Check to make sure it's a good object.
		 *
		 */
		if (Good_obj(*p_ptr) && isPlayer(*p_ptr))
		{
			XSAFELBCHR('#', buff, bufc);
			XSAFELTOS(buff, bufc, (int)*p_ptr, LBUF_SIZE);
		}
		else
		{
			XSAFENOTHING(buff, bufc);
		}

		return;
	}

	/**
	 * We haven't found anything. Now we try a partial match.
	 *
	 */
	thing = find_connected_ambiguous(player, name);

	if (thing == AMBIGUOUS)
	{
		XSAFESTRNCAT(buff, bufc, "#-2", 3, LBUF_SIZE);
	}
	else if (Good_obj(thing) && isPlayer(thing))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}
}

/**
 * @brief If &lt;object&gt; is a dbref, if the dbref is valid, that dbref will be
 *        returned. If the dbref is not valid, #-1 will be returned.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_pfind(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing;

	if (*fargs[0] == '#')
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, match_thing(player, fargs[0]), LBUF_SIZE);
		return;
	}

	if (!((thing = lookup_player(player, fargs[0], 1)) == NOTHING))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
		return;
	}
	else
	{
		XSAFENOMATCH(buff, bufc);
	}
}

/**
 * @brief Search for things with the perspective of another obj.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_locate(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int pref_type = NOTYPE, check_locks = 0, verbose = 0, multiple = 0;
	dbref thing = NOTHING, what = NOTHING;
	char *cp = NULL;

	/**
	 * Find the thing to do the looking, make sure we control it.
	 *
	 */
	if (See_All(player))
	{
		thing = match_thing(player, fargs[0]);
	}
	else
	{
		thing = match_controlled(player, fargs[0]);
	}

	if (!Good_obj(thing))
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	/**
	 * Get pre- and post-conditions and modifiers
	 *
	 */
	for (cp = fargs[2]; *cp; cp++)
	{
		switch (*cp)
		{
		case 'E':
			pref_type = TYPE_EXIT;
			break;

		case 'L':
			check_locks = 1;
			break;

		case 'P':
			pref_type = TYPE_PLAYER;
			break;

		case 'R':
			pref_type = TYPE_ROOM;
			break;

		case 'T':
			pref_type = TYPE_THING;
			break;

		case 'V':
			verbose = 1;
			break;

		case 'X':
			multiple = 1;
			break;
		}
	}

	/**
	 * Set up for the search
	 *
	 */
	if (check_locks)
	{
		init_match_check_keys(thing, fargs[1], pref_type);
	}
	else
	{
		init_match(thing, fargs[1], pref_type);
	}

	/**
	 * Search for each requested thing
	 *
	 */
	for (cp = fargs[2]; *cp; cp++)
	{
		switch (*cp)
		{
		case 'a':
			match_absolute();
			break;

		case 'c':
			match_carried_exit_with_parents();
			break;

		case 'e':
			match_exit_with_parents();
			break;

		case 'h':
			match_here();
			break;

		case 'i':
			match_possession();
			break;

		case 'm':
			match_me();
			break;

		case 'n':
			match_neighbor();
			break;

		case 'p':
			match_player();
			break;

		case '*':
			match_everything(MAT_EXIT_PARENTS);
			break;
		}
	}

	/**
	 * Get the result and return it to the caller
	 *
	 */
	if (multiple)
	{
		what = last_match_result();
	}
	else
	{
		what = match_result();
	}

	if (verbose)
	{
		(void)match_status(player, what);
	}

	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, what, LBUF_SIZE);
}

/**
 * @brief Handler for lattr/nattr
 *        lattr: Return list of attributes I can see on the object.
 *        nattr: Ditto, but just count 'em up.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void handle_lattr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING;
	ATTR *attr = NULL;
	char *bb_p = NULL;
	int ca = 0, total = 0, count_only = Is_Func(LATTR_COUNT), start = 0, count = 0, i = 0, got = 0;
	Delim osep;

	if (!count_only)
	{
		/**
		 * We have two possible syntaxes:
		 * lattr(<whatever>[,<odelim>])
		 * lattr(<whatever>,<start>,<count>[,<odelim>])
		 *
		 */
		if (nfargs > 2)
		{
			if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
			{
				return;
			}

			if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
			{
				return;
			}

			start = (int)strtol(fargs[1], (char **)NULL, 10);
			count = (int)strtol(fargs[2], (char **)NULL, 10);

			if ((start < 1) || (count < 1))
			{
				XSAFELBSTR("#-1 ARGUMENT OUT OF RANGE", buff, bufc);
				return;
			}
		}
		else
		{
			if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
			{
				return;
			}

			if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
			{
				return;
			}

			start = 1;
			count = 0;
		}
	}

	/**
	 * Check for wildcard matching.  parse_attrib_wild checks for read
	 * permission, so we don't have to.  Have p_a_w assume the slash-star
	 * if it is missing.
	 *
	 */
	olist_push();

	if (parse_attrib_wild(player, fargs[0], &thing, 0, 0, 1, 1))
	{
		bb_p = *bufc;

		for (ca = olist_first(), i = 1, got = 0; (ca != NOTHING) && (!count || (got < count)); ca = olist_next(), i++)
		{
			attr = atr_num(ca);

			if (attr)
			{
				if (count_only)
				{
					total++;
				}
				else if (i >= start)
				{
					if (*bufc != bb_p)
					{
						print_separator(&osep, buff, bufc);
					}

					XSAFELBSTR((char *)attr->name, buff, bufc);
					got++;
				}
			}
		}

		if (count_only)
		{
			XSAFELTOS(buff, bufc, total, LBUF_SIZE);
		}
	}
	else
	{
		if (!mushconf.lattr_oldstyle)
		{
			XSAFENOMATCH(buff, bufc);
		}
		else if (count_only)
		{
			XSAFELBCHR('0', buff, bufc);
		}
	}

	olist_pop();
}

/**
 * @brief Search the db for things, returning a list of what matches
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_search(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING;
	char *bp = NULL;
	SEARCH searchparm;

	/**
	 * Set up for the search.  If any errors, abort.
	 *
	 */
	if (!search_setup(player, fargs[0], &searchparm))
	{
		XSAFELBSTR("#-1 ERROR DURING SEARCH", buff, bufc);
		return;
	}

	/**
	 * Do the search and report the results
	 *
	 */
	olist_push();
	search_perform(player, cause, &searchparm);
	bp = *bufc;
	char nbuf[SBUF_SIZE];

	for (thing = olist_first(); thing != NOTHING; thing = olist_next())
	{
		if (bp == *bufc)
		{
			XSPRINTF(nbuf, "#%d", thing);
		}
		else
		{
			XSPRINTF(nbuf, " #%d", thing);
		}

		XSAFELBSTR(nbuf, buff, bufc);
	}
	olist_pop();
}

/**
 * @brief Get database size statistics.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_stats(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref who;
	STATS statinfo;

	if ((!fargs[0]) || !*fargs[0] || !string_compare(fargs[0], "all"))
	{
		who = NOTHING;
	}
	else
	{
		who = lookup_player(player, fargs[0], 1);

		if (who == NOTHING)
		{
			XSAFELBSTR("#-1 NOT FOUND", buff, bufc);
			return;
		}
	}

	if (!get_stats(player, who, &statinfo))
	{
		XSAFELBSTR("#-1 ERROR GETTING STATS", buff, bufc);
		return;
	}

	XSAFESPRINTF(buff, bufc, "%d %d %d %d %d %d %d %d", statinfo.s_total, statinfo.s_rooms, statinfo.s_exits, statinfo.s_things, statinfo.s_players, statinfo.s_unknown, statinfo.s_going, statinfo.s_garbage);
}

/*
 * ---------------------------------------------------------------------------
 * Memory usage.
 */

/**
 * @brief Memory usage of an object
 *
 * @param thing
 * @return int
 */
size_t mem_usage(dbref thing)
{
	char *as = NULL, *str = NULL;
	ATTR *attr = NULL;
	size_t k = sizeof(OBJ) + strlen(Name(thing)) + 1;

	for (int ca = atr_head(thing, &as); ca; ca = atr_next(&as))
	{
		str = atr_get_raw(thing, ca);

		if (str && *str)
		{
			k += strlen(str);
		}

		if (str)
		{
			XFREE(str);
			str = NULL;
		}

		attr = atr_num(ca);

		if (attr)
		{
			str = (char *)attr->name;

			if (str && *str)
			{
				k += strlen(str);
			}
		}
	}

	return k;
}

/**
 * @brief Memory usage of an attribute
 *
 * @param player DBref of the player
 * @param str Attribute
 * @return size_t
 */
size_t mem_usage_attr(dbref player, char *str)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int atr = 0, aflags = 0, alen = 0;
	ATTR *ap = NULL;
	char *abuf = XMALLOC(LBUF_SIZE, "abuf");
	size_t bytes_atext = 0;

	olist_push();

	if (parse_attrib_wild(player, str, &thing, 0, 0, 1, 1))
	{
		int exam = Examinable(player, thing);
		for (atr = olist_first(); atr != NOTHING; atr = olist_next())
		{
			ap = atr_num(atr);

			if (!ap)
			{
				continue;
			}

			abuf = atr_get_str(abuf, thing, atr, &aowner, &aflags, &alen);

			/**
			 * Player must be able to read attribute with 'examine'
			 *
			 */
			if (exam && Read_attr(player, thing, ap, aowner, aflags))
			{
				bytes_atext += alen;
			}
		}
	}

	olist_pop();
	XFREE(abuf);
	return bytes_atext;
}

/**
 * @brief   If no attribute pattern is specified, this returns the number of
 *          bytes of memory consumed by an object, including both attribute
 *          text and object overhead.
 *
 *          If an attribute wildcard pattern is specified, this returns the
 *          number of bytes of memory consumed by attribute text for those
 *          attributes on &lt;object&gt;. To just get a count of the number of bytes
 *          used by all attribute text on an object, use 'objmem(&lt;object&gt; / *)'.
 *          You must be able to read an attribute in order to check its size.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_objmem(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING;

	if (strchr(fargs[0], '/'))
	{
		XSAFELTOS(buff, bufc, mem_usage_attr(player, fargs[0]), LBUF_SIZE);
		return;
	}

	thing = match_thing(player, fargs[0]);

	int exam = Good_obj(thing) ? Examinable(player, thing) : 0;
	if (!exam)
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	XSAFELTOS(buff, bufc, mem_usage(thing), LBUF_SIZE);
}

/**
 * @brief   Returns the sum total of the size, in bytes, of all objects in the
 *          database that are owned by &lt;player&gt; (equivalent to doing an objmem()
 *          on everything that player owns). You must be a Wizard, or have the
 *          Search power, in order to use this on another player.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_playmem(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int tot = 0;
	dbref thing = match_thing(player, fargs[0]);
	dbref j = NOTHING;

	int exam = Good_obj(thing) ? Examinable(player, thing) : 0;
	if (!exam)
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	for (j = 0; j < mushstate.db_top; j++)
	{
		if (Owner(j) == thing)
		{
			tot += mem_usage(j);
		}
	}

	XSAFELTOS(buff, bufc, tot, LBUF_SIZE);
}

/**
 * @brief Returns a string indicating the object type of &lt;object&gt;, either EXIT,
 *        PLAYER, ROOM, or THING.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_type(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		XSAFENOMATCH(buff, bufc);
		return;
	}

	switch (Typeof(it))
	{
	case TYPE_ROOM:
		XSAFESTRNCAT(buff, bufc, "ROOM", 4, LBUF_SIZE);
		break;

	case TYPE_EXIT:
		XSAFESTRNCAT(buff, bufc, "EXIT", 4, LBUF_SIZE);
		break;

	case TYPE_PLAYER:
		XSAFESTRNCAT(buff, bufc, "PLAYER", 6, LBUF_SIZE);
		break;

	case TYPE_THING:
		XSAFESTRNCAT(buff, bufc, "THING", 5, LBUF_SIZE);
		break;

	default:
		XSAFELBSTR("#-1 ILLEGAL TYPE", buff, bufc);
	}

	return;
}

/**
 * @brief Returns 1 if &lt;object&gt; is of type &lt;type&gt;, and 0 otherwise. Valid
 *        types are: ROOM, EXIT, THING, and PLAYER. If an invalid type is
 *        given, the function returns #-1.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_hastype(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		XSAFENOMATCH(buff, bufc);
		return;
	}

	if (!fargs[1] || !*fargs[1])
	{
		XSAFELBSTR("#-1 NO SUCH TYPE", buff, bufc);
		return;
	}

	switch (*fargs[1])
	{
	case 'r':
	case 'R':
		XSAFEBOOL(buff, bufc, isRoom(it));
		break;

	case 'e':
	case 'E':
		XSAFEBOOL(buff, bufc, isExit(it));
		break;

	case 'p':
	case 'P':
		XSAFEBOOL(buff, bufc, isPlayer(it));
		break;

	case 't':
	case 'T':
		XSAFEBOOL(buff, bufc, isThing(it));
		break;

	default:
		XSAFELBSTR("#-1 NO SUCH TYPE", buff, bufc);
		break;
	};
}

/**
 * @brief Return the last object of type Y that X created.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_lastcreate(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i = 0, aowner = 0, aflags = 0, alen = 0, obj_list[4], obj_type = 0;
	char *obj_str = NULL, *p = NULL, *tokst = NULL;
	dbref obj = match_thing(player, fargs[0]);

	if (!controls(player, obj))
	{
		/**
		 * Automatically checks for GoodObj
		 *
		 */
		XSAFENOTHING(buff, bufc);
		return;
	}

	switch (*fargs[1])
	{
	case 'R':
	case 'r':
		obj_type = 0;
		break;

	case 'E':
	case 'e':
		obj_type = 1;
		;
		break;

	case 'T':
	case 't':
		obj_type = 2;
		break;

	case 'P':
	case 'p':
		obj_type = 3;
		break;

	default:
		notify_quiet(player, "Invalid object type.");
		XSAFENOTHING(buff, bufc);
		return;
	}

	obj_str = atr_get(obj, A_NEWOBJS, &aowner, &aflags, &alen);

	if (!*obj_str)
	{
		XFREE(obj_str);
		XSAFENOTHING(buff, bufc);
		return;
	}

	for (p = strtok_r(obj_str, " ", &tokst), i = 0; p && (i < 4); p = strtok_r(NULL, " ", &tokst), i++)
	{
		obj_list[i] = (int)strtol(p, (char **)NULL, 10);
	}

	XFREE(obj_str);
	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, obj_list[obj_type], LBUF_SIZE);
}
