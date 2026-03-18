/**
 * @file funobj_locks.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object-focused built-ins: lock evaluation and content/exit listing
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

void fun_lock(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = NOTHING, aowner = NOTHING;
	int aflags = 0, alen = 0;
	char *tbuf = NULL;
	ATTR *attr = NULL;
	struct boolexp *bexp = NULL;

	/**
	 * Parse the argument into obj + lock
	 *
	 */
	if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufc))
	{
		return;
	}

	/**
	 * Get the attribute and decode it if we can read it
	 *
	 */
	tbuf = atr_get(it, attr->number, &aowner, &aflags, &alen);

	if (Read_attr(player, it, attr, aowner, aflags))
	{
		bexp = boolexp_parse(player, tbuf, 1);
		XFREE(tbuf);
		tbuf = (char *)unparse_boolexp_function(player, bexp);
		boolexp_free(bexp);
		XSAFELBSTR(tbuf, buff, bufc);
		XFREE(tbuf);
	}
	else
	{
		XFREE(tbuf);
	}
}

/**
 * @brief Checks if &lt;actor&gt; would pass the named lock on &lt;object&gt;.
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
void fun_elock(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = NOTHING, victim = NOTHING, aowner = NOTHING;
	int aflags = 0, alen = 0;
	char *tbuf = NULL;
	ATTR *attr = NULL;
	struct boolexp *bexp = NULL;

	/**
	 * Parse lock supplier into obj + lock
	 *
	 */
	if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufc))
	{
		return;
	}

	/**
	 * Get the victim and ensure we can do it
	 *
	 */
	victim = match_thing(player, fargs[1]);

	if (!Good_obj(victim))
	{
		XSAFENOMATCH(buff, bufc);
	}
	else if (!nearby_or_control(player, victim) && !nearby_or_control(player, it))
	{
		XSAFELBSTR("#-1 TOO FAR AWAY", buff, bufc);
	}
	else
	{
		tbuf = atr_get(it, attr->number, &aowner, &aflags, &alen);

		if ((attr->flags & AF_IS_LOCK) || Read_attr(player, it, attr, aowner, aflags))
		{
			if (Pass_Locks(victim))
			{
				XSAFELBCHR('1', buff, bufc);
			}
			else
			{
				bexp = boolexp_parse(player, tbuf, 1);
				XSAFEBOOL(buff, bufc, boolexp_eval(victim, it, it, bexp));
				boolexp_free(bexp);
			}
		}
		else
		{
			XSAFELBCHR('0', buff, bufc);
		}

		XFREE(tbuf);
	}
}

/**
 * @brief   Checks if &lt;actor&gt; would pass a lock on &lt;locked object&gt; with syntax
 *          &lt;lock string&gt;
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
void fun_elockstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref locked_obj = match_thing(player, fargs[0]), actor_obj = match_thing(player, fargs[1]);

	if (!Good_obj(locked_obj) || !Good_obj(actor_obj))
	{
		XSAFENOMATCH(buff, bufc);
	}
	else if (!nearby_or_control(player, actor_obj))
	{
		XSAFELBSTR("#-1 TOO FAR AWAY", buff, bufc);
	}
	else if (!Controls(player, locked_obj))
	{
		XSAFENOPERM(buff, bufc);
	}
	else
	{
		BOOLEXP *okey = boolexp_parse(player, fargs[2], 0);

		if (okey == TRUE_BOOLEXP)
		{
			XSAFELBSTR("#-1 INVALID KEY", buff, bufc);
		}
		else if (Pass_Locks(actor_obj))
		{
			XSAFELBCHR('1', buff, bufc);
		}
		else
		{
			XSAFELTOS(buff, bufc, boolexp_eval(actor_obj, locked_obj, locked_obj, okey), LBUF_SIZE);
		}

		boolexp_free(okey);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_xcon:
 */

/**
 * @brief Return a partial list of contents of an object, starting from a
 *        specified element in the list and copying a specified number of
 *        elements.
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
void fun_xcon(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, it = NOTHING;
	char *bb_p = NULL;
	int i = 0, first = 0, last = 0;
	Delim osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	it = match_thing(player, fargs[0]);
	bb_p = *bufc;

	if (Good_loc(it) && (Examinable(player, it) || (Location(player) == it) || (it == cause)))
	{
		first = (int)strtol(fargs[1], (char **)NULL, 10);
		last = (int)strtol(fargs[2], (char **)NULL, 10);

		if ((first > 0) && (last > 0))
		{
			/**
			 * Move to the first object that we want
			 *
			 */
			for (thing = Contents(it), i = 1; (i < first) && (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing), i++)
				;

			/**
			 * Grab objects until we reach the last one we want
			 *
			 */
			for (i = 0; (i < last) && (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing), i++)
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
			}
		}
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}
}

/**
 * @brief Return a list of contents.
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
void fun_lcon(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, it = NOTHING;
	char *bb_p = NULL;
	int exam = 0;
	Delim osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	it = match_thing(player, fargs[0]);
	bb_p = *bufc;

	exam = Good_loc(it) ? Examinable(player, it) : 0;
	if (Good_loc(it) && (exam || (Location(player) == it) || (it == cause)))
	{
		for (thing = Contents(it); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}
			XSAFELBCHR('#', buff, bufc);
			XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
		}
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}
}

/**
 * @brief Return a list of exits.
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
void fun_lexits(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, it = NOTHING, parent = NOTHING;
	char *bb_p = NULL;
	int exam = 0, lev = 0, key = 0;
	Delim osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	it = match_thing(player, fargs[0]);

	if (!Good_obj(it) || !Has_exits(it))
	{
		XSAFENOTHING(buff, bufc);
		return;
	}

	exam = Examinable(player, it);

	if (!exam && (where_is(player) != it) && (it != cause))
	{
		XSAFENOTHING(buff, bufc);
		return;
	}

	/**
	 * Return info for all parent levels
	 *
	 */
	bb_p = *bufc;
	/* Cache base dark once for the original object */
	int base_dark_flag = Dark(it) ? VE_BASE_DARK : 0;
	for (lev = 0, parent = it; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
	{
		/**
		 * Look for exits at each level
		 *
		 */
		if (!Has_exits(parent))
		{
			continue;
		}

		key = 0;
		if (Examinable(player, parent))
			key |= VE_LOC_XAM;
		if (Dark(parent))
			key |= VE_LOC_DARK;
		/* Reuse cached base dark flag for the starting room/object */
		key |= base_dark_flag;

		for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
		{
			if (Exit_Visible(thing, player, key))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
			}
		}
	}
	return;
}

/**
 * @brief Approximate equivalent of \@entrances command.
 *        Borrowed in part from PennMUSH.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_entrances(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing, i;
	char *bb_p;
	int low_bound, high_bound, control_thing;
	int find_ex, find_th, find_pl, find_rm;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 4, buff, bufc))
	{
		return;
	}

	if (nfargs >= 3)
	{
		low_bound = (int)strtol(fargs[2] + (fargs[2][0] == NUMBER_TOKEN), (char **)NULL, 10);

		if (!Good_dbref(low_bound))
		{
			low_bound = 0;
		}
	}
	else
	{
		low_bound = 0;
	}

	if (nfargs == 4)
	{
		high_bound = (int)strtol(fargs[3] + (fargs[3][0] == NUMBER_TOKEN), (char **)NULL, 10);

		if (!Good_dbref(high_bound))
		{
			high_bound = mushstate.db_top - 1;
		}
	}
	else
	{
		high_bound = mushstate.db_top - 1;
	}

	find_ex = find_th = find_pl = find_rm = 0;

	if (nfargs >= 2)
	{
		for (bb_p = fargs[1]; *bb_p; ++bb_p)
		{
			switch (*bb_p)
			{
			case 'a':
			case 'A':
				find_ex = find_th = find_pl = find_rm = 1;
				break;

			case 'e':
			case 'E':
				find_ex = 1;
				break;

			case 't':
			case 'T':
				find_th = 1;
				break;

			case 'p':
			case 'P':
				find_pl = 1;
				break;

			case 'r':
			case 'R':
				find_rm = 1;
				break;

			default:
				XSAFELBSTR("#-1 INVALID TYPE", buff, bufc);
				return;
			}
		}
	}

	if (!find_ex && !find_th && !find_pl && !find_rm)
	{
		find_ex = find_th = find_pl = find_rm = 1;
	}

	if (!fargs[0] || !*fargs[0])
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
			XSAFENOTHING(buff, bufc);
			return;
		}
	}
	else
	{
		init_match(player, fargs[0], NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();

		if (!Good_obj(thing))
		{
			XSAFENOTHING(buff, bufc);
			return;
		}
	}

	if (!payfor(player, mushconf.searchcost))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
		XSAFENOTHING(buff, bufc);
		return;
	}

	control_thing = Examinable(player, thing);
	bb_p = *bufc;

	for (i = low_bound; i <= high_bound; i++)
	{
		/* Cheap type/location checks first, defer Examinable unless needed */
		int match_exit = find_ex && isExit(i) && (Location(i) == thing);
		int match_room = find_rm && isRoom(i) && (Dropto(i) == thing);
		int match_thing = find_th && isThing(i) && (Home(i) == thing);
		int match_player = find_pl && isPlayer(i) && (Home(i) == thing);

		if (!(match_exit || match_room || match_thing || match_player))
		{
			continue;
		}

		if (!control_thing && !Examinable(player, i))
		{
			continue;
		}

		if (*bufc != bb_p)
		{
			XSAFELBCHR(' ', buff, bufc);
		}

		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, i, LBUF_SIZE);
	}
}

/**
 * @brief Return an object's home
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_home(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	int exam = Good_obj(it) ? Examinable(player, it) : 0;
	if (!exam)
	{
		XSAFENOTHING(buff, bufc);
	}
	else if (Has_home(it))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, Link(it), LBUF_SIZE);
	}
	else if (Has_dropto(it))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, Location(it), LBUF_SIZE);
	}
	else if (isExit(it))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, where_is(it), LBUF_SIZE);
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}

	return;
}

/**
 * @brief Return an object's value
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_money(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	int exam = Good_obj(it) ? Examinable(player, it) : 0;
	if (!exam)
	{
		XSAFENOTHING(buff, bufc);
	}
	else
	{
		XSAFELTOS(buff, bufc, Pennies(it), LBUF_SIZE);
	}
}

/**
 * @brief Can X locate Y
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
void fun_findable(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref obj = match_thing(player, fargs[0]);
	dbref victim = match_thing(player, fargs[1]);

	if (!Good_obj(obj))
	{
		XSAFELBSTR("#-1 ARG1 NOT FOUND", buff, bufc);
	}
	else if (!Good_obj(victim))
	{
		XSAFELBSTR("#-1 ARG2 NOT FOUND", buff, bufc);
	}
	else
	{
		XSAFEBOOL(buff, bufc, locatable(obj, victim, obj));
	}
}

/**
 * @brief Can X examine Y. If X does not exist, 0 is returned. If Y, the
 *        object, does not exist, 0 is returned. If Y the object exists, but
 *        the optional attribute does not, X's ability to return Y the object
 *        is returned.
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
void fun_visible(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]), thing = NOTHING, aowner = NOTHING;
	int aflags = 0, atr = 0;
	ATTR *ap = NULL;

	if (!Good_obj(it))
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	if (parse_attrib(player, fargs[1], &thing, &atr, 1))
	{
		if (atr == NOTHING)
		{
			XSAFEBOOL(buff, bufc, Examinable(it, thing));
		}
		else
		{
			ap = atr_num(atr);
			atr_pget_info(thing, atr, &aowner, &aflags);
			XSAFEBOOL(buff, bufc, See_attr_all(it, thing, ap, aowner, aflags, 1));
		}
		return;
	}

	thing = match_thing(player, fargs[1]);
	int good = Good_obj(thing);
	XSAFEBOOL(buff, bufc, good ? Examinable(it, thing) : 0);
}

/**
 * @brief Returns 1 if player could set &lt;obj&gt;/&lt;attr&gt;.
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
void fun_writable(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]), thing = NOTHING, aowner = NOTHING;
	int aflags = 0, atr = 0, retval = 0;
	ATTR *ap = NULL;
	char *s = NULL;

	if (!Good_obj(it))
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	retval = parse_attrib(player, fargs[1], &thing, &atr, 1);

	/**
	 * Possibilities: retval is 0, which means we didn't match a thing.
	 * retval is NOTHING, which means we matched a thing but have a
	 * non-existent attribute. retval is 1; atr is either NOTHING
	 * (non-existent attribute or no permission to see), or a valid attr
	 * number. In the case of NOTHING we can't tell which it is, so must
	 * continue.
	 *
	 */
	if (retval == 0)
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	if ((retval == 1) && (atr != NOTHING))
	{
		ap = atr_num(atr);
		atr_pget_info(thing, atr, &aowner, &aflags);
		XSAFEBOOL(buff, bufc, Set_attr(it, thing, ap, aflags));
		return;
	}

	/**
	 * Non-existent attribute. Go see if it's settable.
	 *
	 */
	if (!fargs[1] || !*fargs[1])
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	if (((s = strchr(fargs[1], '/')) == NULL) || !*s++)
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	atr = mkattr(s);

	if ((atr <= 0) || ((ap = atr_num(atr)) == NULL))
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	atr_pget_info(thing, atr, &aowner, &aflags);
	XSAFEBOOL(buff, bufc, Set_attr(it, thing, ap, aflags));
}
