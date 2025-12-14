/**
 * @file funobj.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object functions
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

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

/**
 * @brief Returns an object's objectID.
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
void fun_objid(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (Good_obj(it))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, it, LBUF_SIZE);

		SAFE_LB_CHR(':', buff, bufc);
		SAFE_LTOS(buff, bufc, CreateTime(it), LBUF_SIZE);
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
	}
}

/**
 * @brief Returns first item in contents list of object/room
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
void fun_con(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (Good_loc(it) && (Examinable(player, it) || (where_is(player) == it) || (it == cause)))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, Contents(it), LBUF_SIZE);
		return;
	}

	SAFE_NOTHING(buff, bufc);
	return;
}

/**
 * @brief Returns first exit in exits list of room.
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
void fun_exit(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]), exit = NOTHING;
	int key = 0;

	if (Good_obj(it) && Has_exits(it) && Good_obj(Exits(it)))
	{
		key = 0;

		if (Examinable(player, it))
		{
			key |= VE_LOC_XAM;
		}

		if (Dark(it))
		{
			key |= VE_LOC_DARK;
		}

		for (exit = Exits(it); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
		{
			if (Exit_Visible(exit, player, key))
			{
				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, exit, LBUF_SIZE);
				return;
			}
		}
	}

	SAFE_NOTHING(buff, bufc);
	return;
}

/**
 * @brief return next thing in contents or exits chain
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
void fun_next(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]), loc = NOTHING, exit = NOTHING, ex_here = NOTHING;
	int key = 0;

	if (Good_obj(it) && Has_siblings(it))
	{
		loc = where_is(it);
		ex_here = Good_obj(loc) ? Examinable(player, loc) : 0;

		if (ex_here || (loc == player) || (loc == where_is(player)))
		{
			if (!isExit(it))
			{
				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, Next(it), LBUF_SIZE);
				return;
			}
			else
			{
				key = 0;

				if (ex_here)
				{
					key |= VE_LOC_XAM;
				}

				if (Dark(loc))
				{
					key |= VE_LOC_DARK;
				}

				for (exit = Next(it); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
				{
					if (Exit_Visible(exit, player, key))
					{
						SAFE_LB_CHR('#', buff, bufc);
						SAFE_LTOS(buff, bufc, exit, LBUF_SIZE);
						return;
					}
				}
			}
		}
	}

	SAFE_NOTHING(buff, bufc);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * handle_loc: Locate an object (LOC, WHERE).  where(): Returns the "true" location of something
 */

/**
 * @brief loc(): Returns the location of something.
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
void handle_loc(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it;
	it = match_thing(player, fargs[0]);

	if (locatable(player, it, cause))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, ((FUN *)fargs[-1])->flags & (0x01) ? where_is(it) : Location(it), LBUF_SIZE);
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
	}
}

/**
 * @brief Returns the recursed location of something (specifying #levels)
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
void fun_rloc(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int i = 0, levels = (int)strtol(fargs[1], (char **)NULL, 10);
	dbref it = match_thing(player, fargs[0]);

	if (levels > mushconf.ntfy_nest_lim)
	{
		levels = mushconf.ntfy_nest_lim;
	}

	if (locatable(player, it, cause))
	{
		for (i = 0; i < levels; i++)
		{
			if (Good_obj(it) && (Has_location(it) || isExit(it)))
			{
				it = Location(it);
			}
			else
			{
				break;
			}
		}

		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, it, LBUF_SIZE);
		return;
	}

	SAFE_NOTHING(buff, bufc);
}

/**
 * @brief Find the room an object is ultimately in.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref oc cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_room(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (locatable(player, it, cause))
	{
		for (int count = mushconf.ntfy_nest_lim; count > 0; count--)
		{
			it = Location(it);

			if (!Good_obj(it))
			{
				break;
			}

			if (isRoom(it))
			{
				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, it, LBUF_SIZE);
				return;
			}
		}

		SAFE_NOTHING(buff, bufc);
	}
	else if (isRoom(it))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, it, LBUF_SIZE);
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
	}

	return;
}

/**
 * @brief Return the owner of an object.
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
void fun_owner(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = NOTHING, aowner = NOTHING;
	int atr = 0, aflags = 0;

	if (parse_attrib(player, fargs[0], &it, &atr, 1))
	{
		if (atr == NOTHING)
		{
			it = NOTHING;
		}
		else
		{
			atr_pget_info(it, atr, &aowner, &aflags);
			it = aowner;
		}
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (Good_obj(it))
		{
			it = Owner(it);
		}
	}

	SAFE_LB_CHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, it, LBUF_SIZE);
}

/**
 * @brief  Does x control y?
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
void fun_controls(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref x = match_thing(player, fargs[0]), y = match_thing(player, fargs[1]);

	if (!Good_obj(x))
	{
		SAFE_LB_STR("#-1 ARG1 NOT FOUND", buff, bufc);
		return;
	}

	if (!Good_obj(y))
	{
		SAFE_LB_STR("#-1 ARG2 NOT FOUND", buff, bufc);
		return;
	}

	SAFE_BOOL(buff, bufc, Controls(x, y));
}

/**
 * @brief  Can X see Y in the normal Contents list of the room. If X or Y
 *         do not exist, 0 is returned.
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
void fun_sees(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]), thing = match_thing(player, fargs[1]);
	dbref loc = NOTHING;

	if (!Good_obj(it) || !Good_obj(thing))
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	loc = Location(thing);
	SAFE_BOOL(buff, bufc, (isExit(thing) ? Can_See_Exit(it, thing, Darkened(it, loc)) : Can_See(it, thing, Sees_Always(it, loc))));
}

/**
 * @brief Return whether or not obj1 is near obj2.
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
void fun_nearby(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref obj1 = match_thing(player, fargs[0]), obj2 = match_thing(player, fargs[1]);
	bool nb1 = nearby_or_control(player, obj1);
	bool nb2 = nearby_or_control(player, obj2);

	if (!(nb1 || nb2))
	{
		SAFE_LB_CHR('0', buff, bufc);
	}
	else
	{
		SAFE_BOOL(buff, bufc, nearby(obj1, obj2));
	}
}

/*
 * ---------------------------------------------------------------------------
 * Presence functions. hears(<object>, <speaker>): Can <object> hear
 * <speaker> speak? knows(<object>, <target>): Can <object> know about
 * <target>? moves(<object>, <mover>): Can <object> see <mover> move?
 */

/**
 * @brief Handle presence functions.
 *        hears(<object>, <speaker>): Can <object> hear <speaker> speak?
 *        knows(<object>, <target>): Can <object> know about <target>?
 *        moves(<object>, <mover>): Can <object> see <mover> move?
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
void handle_okpres(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int oper = Func_Mask(PRESFN_OPER);
	dbref object = match_thing(player, fargs[0]), actor = match_thing(player, fargs[1]);

	if (!Good_obj(object) || !Good_obj(actor))
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	if (oper == PRESFN_HEARS)
	{
		SAFE_BOOL(buff, bufc, ((Unreal(actor) && !Check_Heard(object, actor)) || (Unreal(object) && !Check_Hears(actor, object))) ? 0 : 1);
	}
	else if (oper == PRESFN_MOVES)
	{
		SAFE_BOOL(buff, bufc, ((Unreal(actor) && !Check_Noticed(object, actor)) || (Unreal(object) && !Check_Notices(actor, object))) ? 0 : 1);
	}
	else if (oper == PRESFN_KNOWS)
	{
		SAFE_BOOL(buff, bufc, ((Unreal(actor) && !Check_Known(object, actor)) || (Unreal(object) && !Check_Knows(actor, object))) ? 0 : 1);
	}
	else
	{
		SAFE_LB_CHR('0', buff, bufc);
	}
}

/**
 * @brief Get object name (NAME, FULLNAME). name(): Return the name of
 *        an object. fullname(): Return the fullname of an object
 *        (good for exits).
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
void handle_name(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		return;
	}

	if (!mushconf.read_rem_name)
	{
		if (!nearby_or_control(player, it) && !isPlayer(it) && !Long_Fingers(player))
		{
			SAFE_LB_STR("#-1 TOO FAR AWAY TO SEE", buff, bufc);
			return;
		}
	}

	if (!Is_Func(NAMEFN_FULLNAME) && isExit(it))
	{
		safe_exit_name(it, buff, bufc);
	}
	else
	{
		safe_name(it, buff, bufc);
	}
}

/**
 * @brief Perform pronoun sub for object (OBJ, POSS, SUBJ, APOSS).
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
void handle_pronoun(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);
	char *str = NULL, *pronouns[4] = {"%o", "%p", "%s", "%a"};

	if (!Good_obj(it) || (!isPlayer(it) && !nearby_or_control(player, it)))
	{
		SAFE_NOMATCH(buff, bufc);
	}
	else
	{
		str = pronouns[Func_Flags(fargs)];
		eval_expression_string(buff, bufc, it, it, it, 0, &str, (char **)NULL, 0);
	}
}

/**
 * @brief Handle locks
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
void fun_lock(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
		bexp = parse_boolexp(player, tbuf, 1);
		XFREE(tbuf);
		tbuf = (char *)unparse_boolexp_function(player, bexp);
		free_boolexp(bexp);
		SAFE_LB_STR(tbuf, buff, bufc);
		XFREE(tbuf);
	}
	else
	{
		XFREE(tbuf);
	}
}

/**
 * @brief Checks if <actor> would pass the named lock on <object>.
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
void fun_elock(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
		SAFE_NOMATCH(buff, bufc);
	}
	else if (!nearby_or_control(player, victim) && !nearby_or_control(player, it))
	{
		SAFE_LB_STR("#-1 TOO FAR AWAY", buff, bufc);
	}
	else
	{
		tbuf = atr_get(it, attr->number, &aowner, &aflags, &alen);

		if ((attr->flags & AF_IS_LOCK) || Read_attr(player, it, attr, aowner, aflags))
		{
			if (Pass_Locks(victim))
			{
				SAFE_LB_CHR('1', buff, bufc);
			}
			else
			{
				bexp = parse_boolexp(player, tbuf, 1);
				SAFE_BOOL(buff, bufc, eval_boolexp(victim, it, it, bexp));
				free_boolexp(bexp);
			}
		}
		else
		{
			SAFE_LB_CHR('0', buff, bufc);
		}

		XFREE(tbuf);
	}
}

/**
 * @brief   Checks if <actor> would pass a lock on <locked object> with syntax
 *          <lock string>
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
void fun_elockstr(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref locked_obj = match_thing(player, fargs[0]), actor_obj = match_thing(player, fargs[1]);

	if (!Good_obj(locked_obj) || !Good_obj(actor_obj))
	{
		SAFE_NOMATCH(buff, bufc);
	}
	else if (!nearby_or_control(player, actor_obj))
	{
		SAFE_LB_STR("#-1 TOO FAR AWAY", buff, bufc);
	}
	else if (!Controls(player, locked_obj))
	{
		SAFE_NOPERM(buff, bufc);
	}
	else
	{
		BOOLEXP *okey = parse_boolexp(player, fargs[2], 0);

		if (okey == TRUE_BOOLEXP)
		{
			SAFE_LB_STR("#-1 INVALID KEY", buff, bufc);
		}
		else if (Pass_Locks(actor_obj))
		{
			SAFE_LB_CHR('1', buff, bufc);
		}
		else
		{
			SAFE_LTOS(buff, bufc, eval_boolexp(actor_obj, locked_obj, locked_obj, okey), LBUF_SIZE);
		}

		free_boolexp(okey);
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

				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, thing, LBUF_SIZE);
			}
		}
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
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
			SAFE_LB_CHR('#', buff, bufc);
			SAFE_LTOS(buff, bufc, thing, LBUF_SIZE);
		}
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
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
		SAFE_NOTHING(buff, bufc);
		return;
	}

	exam = Examinable(player, it);

	if (!exam && (where_is(player) != it) && (it != cause))
	{
		SAFE_NOTHING(buff, bufc);
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

				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, thing, LBUF_SIZE);
			}
		}
	}
	return;
}

/**
 * @brief Approximate equivalent of @entrances command.
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
void fun_entrances(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
				SAFE_LB_STR("#-1 INVALID TYPE", buff, bufc);
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
			SAFE_NOTHING(buff, bufc);
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
			SAFE_NOTHING(buff, bufc);
			return;
		}
	}

	if (!payfor(player, mushconf.searchcost))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
		SAFE_NOTHING(buff, bufc);
		return;
	}

	control_thing = Examinable(player, thing);
	bb_p = *bufc;

	for (i = low_bound; i <= high_bound; i++)
	{
		if (control_thing || Examinable(player, i))
		{
			if ((find_ex && isExit(i) && (Location(i) == thing)) || (find_rm && isRoom(i) && (Dropto(i) == thing)) || (find_th && isThing(i) && (Home(i) == thing)) || (find_pl && isPlayer(i) && (Home(i) == thing)))
			{
				if (*bufc != bb_p)
				{
					SAFE_LB_CHR(' ', buff, bufc);
				}

				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, i, LBUF_SIZE);
			}
		}
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
void fun_home(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (!Good_obj(it) || !Examinable(player, it))
	{
		SAFE_NOTHING(buff, bufc);
	}
	else if (Has_home(it))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, Link(it), LBUF_SIZE);
	}
	else if (Has_dropto(it))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, Location(it), LBUF_SIZE);
	}
	else if (isExit(it))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, where_is(it), LBUF_SIZE);
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
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
void fun_money(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (!Good_obj(it) || !Examinable(player, it))
	{
		SAFE_NOTHING(buff, bufc);
	}
	else
	{
		SAFE_LTOS(buff, bufc, Pennies(it), LBUF_SIZE);
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
void fun_findable(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref obj = match_thing(player, fargs[0]);
	dbref victim = match_thing(player, fargs[1]);

	if (!Good_obj(obj))
	{
		SAFE_LB_STR("#-1 ARG1 NOT FOUND", buff, bufc);
	}
	else if (!Good_obj(victim))
	{
		SAFE_LB_STR("#-1 ARG2 NOT FOUND", buff, bufc);
	}
	else
	{
		SAFE_BOOL(buff, bufc, locatable(obj, victim, obj));
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
void fun_visible(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]), thing = NOTHING, aowner = NOTHING;
	int aflags = 0, atr = 0;
	ATTR *ap = NULL;

	if (!Good_obj(it))
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	if (parse_attrib(player, fargs[1], &thing, &atr, 1))
	{
		if (atr == NOTHING)
		{
			SAFE_BOOL(buff, bufc, Examinable(it, thing));
			return;
		}
		ap = atr_num(atr);
		atr_pget_info(thing, atr, &aowner, &aflags);
		SAFE_BOOL(buff, bufc, See_attr_all(it, thing, ap, aowner, aflags, 1));
		return;
	}

	thing = match_thing(player, fargs[1]);
	SAFE_BOOL(buff, bufc, Good_obj(thing) ? Examinable(it, thing) : 0);
}

/**
 * @brief Returns 1 if player could set <obj>/<attr>.
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
void fun_writable(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]), thing = NOTHING, aowner = NOTHING;
	int aflags = 0, atr = 0, retval = 0;
	ATTR *ap = NULL;
	char *s = NULL;

	if (!Good_obj(it))
	{
		SAFE_LB_CHR('0', buff, bufc);
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
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	if ((retval == 1) && (atr != NOTHING))
	{
		ap = atr_num(atr);
		atr_pget_info(thing, atr, &aowner, &aflags);
		SAFE_BOOL(buff, bufc, Set_attr(it, thing, ap, aflags));
		return;
	}

	/**
	 * Non-existent attribute. Go see if it's settable.
	 *
	 */
	if (!fargs[1] || !*fargs[1])
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	if (((s = strchr(fargs[1], '/')) == NULL) || !*s++)
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	atr = mkattr(s);

	if ((atr <= 0) || ((ap = atr_num(atr)) == NULL))
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	atr_pget_info(thing, atr, &aowner, &aflags);
	SAFE_BOOL(buff, bufc, Set_attr(it, thing, ap, aflags));
}

/**
 * @brief Returns the flags on an object. Because @switch is case-insensitive,
 *        not quite as useful as it could be.
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
void fun_flags(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it, aowner;
	int atr, aflags;
	char *buff2, *xbuf, *xbufp;

	if (parse_attrib(player, fargs[0], &it, &atr, 1))
	{
		if (atr == NOTHING)
		{
			SAFE_NOTHING(buff, bufc);
		}
		else
		{
			atr_pget_info(it, atr, &aowner, &aflags);
			xbuf = XMALLOC(16, "xbuf");

			xbufp = xbuf;
			if (aflags & AF_LOCK)
				*xbufp++ = '+';
			if (aflags & AF_NOPROG)
				*xbufp++ = '$';
			if (aflags & AF_CASE)
				*xbufp++ = 'C';
			if (aflags & AF_DEFAULT)
				*xbufp++ = 'D';
			if (aflags & AF_HTML)
				*xbufp++ = 'H';
			if (aflags & AF_PRIVATE)
				*xbufp++ = 'I';
			if (aflags & AF_RMATCH)
				*xbufp++ = 'M';
			if (aflags & AF_NONAME)
				*xbufp++ = 'N';
			if (aflags & AF_NOPARSE)
				*xbufp++ = 'P';
			if (aflags & AF_NOW)
				*xbufp++ = 'Q';
			if (aflags & AF_REGEXP)
				*xbufp++ = 'R';
			if (aflags & AF_STRUCTURE)
				*xbufp++ = 'S';
			if (aflags & AF_TRACE)
				*xbufp++ = 'T';
			if (aflags & AF_VISUAL)
				*xbufp++ = 'V';
			if (aflags & AF_NOCLONE)
				*xbufp++ = 'c';
			if (aflags & AF_DARK)
				*xbufp++ = 'd';
			if (aflags & AF_GOD)
				*xbufp++ = 'g';
			if (aflags & AF_CONST)
				*xbufp++ = 'k';
			if (aflags & AF_MDARK)
				*xbufp++ = 'm';
			if (aflags & AF_WIZARD)
				*xbufp++ = 'w';
			*xbufp = '\0';

			SAFE_LB_STR(xbuf, buff, bufc);
			XFREE(xbuf);
		}
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (Good_obj(it) && (mushconf.pub_flags || Examinable(player, it) || (it == cause)))
		{
			buff2 = unparse_flags(player, it);
			SAFE_LB_STR(buff2, buff, bufc);
			XFREE(buff2);
		}
		else
		{
			SAFE_NOTHING(buff, bufc);
		}
	}
}

/**
 * @brief andflags, orflags: Check a list of flags.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref oc cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void handle_flaglists(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s;
	char *flagletter = XMALLOC(2, "flagletter");
	FLAGSET fset;
	FLAG p_type;
	int negate, temp, type;
	dbref it = match_thing(player, fargs[0]);
	type = Is_Func(LOGIC_OR);
	negate = temp = 0;

	if (!Good_obj(it) || (!(mushconf.pub_flags || Examinable(player, it) || (it == cause))))
	{
		SAFE_LB_CHR('0', buff, bufc);
		XFREE(flagletter);
		return;
	}

	for (s = fargs[1]; *s; s++)
	{
		/**
		 * Check for a negation sign. If we find it, we note it and
		 * increment the pointer to the next character.
		 *
		 */
		if (*s == '!')
		{
			negate = 1;
			s++;
		}
		else
		{
			negate = 0;
		}

		if (!*s)
		{
			SAFE_LB_CHR('0', buff, bufc);
			XFREE(flagletter);
			return;
		}

		flagletter[0] = *s;
		flagletter[1] = '\0';

		if (!convert_flags(player, flagletter, &fset, &p_type))
		{
			/**
			 * Either we got a '!' that wasn't followed by a
			 * letter, or we couldn't find that flag. For AND,
			 * since we've failed a check, we can return false.
			 * Otherwise we just go on.
			 *
			 */
			if (!type)
			{
				SAFE_LB_CHR('0', buff, bufc);
				XFREE(flagletter);
				return;
			}
			else
			{
				continue;
			}
		}
		else
		{
			/* Determine if the object has this flag once */
			int has_flag = ((Flags(it) & fset.word1) || (Flags2(it) & fset.word2) || (Flags3(it) & fset.word3) || (Typeof(it) == p_type)) ? 1 : 0;
			if (has_flag && (p_type == TYPE_PLAYER) && (fset.word2 == CONNECTED) && Can_Hide(it) && Hidden(it) && !See_Hidden(player))
			{
				temp = 0;
			}
			else
			{
				temp = has_flag;
			}

			if (!(type ^ negate ^ temp))
			{
				/**
				 * Four ways to satisfy that test: AND, don't
				 * want flag but we have it; AND, do want
				 * flag but don't have it; OR, don't want
				 * flag and don't have it; OR, do want flag
				 * and do have it.
				 *
				 */
				SAFE_BOOL(buff, bufc, type);
				XFREE(flagletter);
				return;
			}

			/* Otherwise, move on to check the next flag. */
		}
	}

	SAFE_BOOL(buff, bufc, !type);
	XFREE(flagletter);
}

/**
 * @brief Auxiliary function for fun_hasflag
 *
 * @param player DBref of player
 * @param thing DBref of thing
 * @param attr Attributes
 * @param aowner Attribute owner
 * @param aflags Attribute flags
 * @param flagname Flag Name
 * @return true Attribute found
 * @return false Attribute not found
 */
bool atr_has_flag(dbref player, dbref thing, ATTR *attr, int aowner, int aflags, char *flagname)
{
	int flagval = 0;

	if (!See_attr(player, thing, attr, aowner, aflags))
	{
		return false;
	}

	flagval = search_nametab(player, indiv_attraccess_nametab, flagname);

	if (flagval < 0)
	{
		flagval = search_nametab(player, attraccess_nametab, flagname);
	}

	if (flagval < 0)
	{
		return false;
	}

	return (aflags & flagval) ? true : false;
}

/**
 * @brief   Returns true if object <object> has the flag named <flag> set on
 *          it, or, if <flag> is PLAYER, THING, ROOM, or EXIT, if <object> is
 *          of the specified type.
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
void fun_hasflag(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = NOTHING, aowner = NOTHING;
	int atr = 0, aflags = 0;
	ATTR *ap = NULL;

	if (parse_attrib(player, fargs[0], &it, &atr, 1))
	{
		if (atr == NOTHING)
		{
			SAFE_LB_STR("#-1 NOT FOUND", buff, bufc);
		}
		else
		{
			ap = atr_num(atr);
			atr_pget_info(it, atr, &aowner, &aflags);
			SAFE_BOOL(buff, bufc, atr_has_flag(player, it, ap, aowner, aflags, fargs[1]));
		}
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (!Good_obj(it))
		{
			SAFE_NOMATCH(buff, bufc);
			return;
		}

		if (mushconf.pub_flags || Examinable(player, it) || (it == cause))
		{
			SAFE_BOOL(buff, bufc, has_flag(player, it, fargs[1]));
		}
		else
		{
			SAFE_NOPERM(buff, bufc);
		}
	}
}

/**
 * @brief Returns true if object <object> has the flag named <flag> set on
 *        it, or, if <flag> is PLAYER, THING, ROOM, or EXIT, if <object> is
 *        of the specified type.
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
void fun_haspower(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it;
	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		SAFE_NOMATCH(buff, bufc);
		return;
	}

	if (mushconf.pub_flags || Examinable(player, it) || (it == cause))
	{
		SAFE_BOOL(buff, bufc, has_power(player, it, fargs[1]));
	}
	else
	{
		SAFE_NOPERM(buff, bufc);
	}
}

/**
 * @brief This function returns 1 if <object> has all the flags in
 *        <flag list 1>, or all the flags in <flag list 2>, and so
 *        forth (up to eight lists). Otherwise, it returns 0.
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
void fun_hasflags(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it;
	char **elems;
	int n_elems, i, j, result;

	if (nfargs < 2)
	{
		SAFE_SPRINTF(buff, bufc, "#-1 FUNCTION (HASFLAGS) EXPECTS AT LEAST 2 ARGUMENTS BUT GOT %d", nfargs);
		return;
	}

	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		SAFE_NOMATCH(buff, bufc);
		return;
	}

	/**
	 * Walk through each of the lists we've been passed. We need to have
	 * all the flags in a particular list (AND) in order to consider that
	 * list true. We return 1 if any of the lists are true. (i.e., we OR
	 * the list results).
	 *
	 */
	result = 0;

	for (i = 1; !result && (i < nfargs); i++)
	{
		n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[i], &SPACE_DELIM);

		if (n_elems > 0)
		{
			result = 1;

			for (j = 0; result && (j < n_elems); j++)
			{
				if (*elems[j] == '!')
					result = (has_flag(player, it, elems[j] + 1)) ? 0 : 1;
				else
					result = has_flag(player, it, elems[j]);
			}
		}

		XFREE(elems);
	}

	SAFE_BOOL(buff, bufc, result);
}

/**
 * @brief Get timestamps (LASTACCESS, LASTMOD, CREATION).
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
void handle_timestamp(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (!Good_obj(it) || !Examinable(player, it))
	{
		SAFE_STRNCAT(buff, bufc, "-1", 2, LBUF_SIZE);
	}
	else
	{
		SAFE_LTOS(buff, bufc, Is_Func(TIMESTAMP_MOD) ? ModTime(it) : (Is_Func(TIMESTAMP_ACC) ? AccessTime(it) : CreateTime(it)), LBUF_SIZE);
	}
}

/**
 * @brief This function returns 1 if <object> has all the flags in
 *        <flag list 1>, or all the flags in <flag list 2>, and so
 *        forth (up to eight lists). Otherwise, it returns 0.
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
void fun_parent(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (Good_obj(it) && (Examinable(player, it) || (it == cause)))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, Parent(it), LBUF_SIZE);
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
	}

	return;
}

/**
 * @brief This function returns the list of dbrefs of the object's "parent
 *        chain", including itself: i.e., its own dbref, the dbref of the
 *        object it is @parent'd to, its parent's parent (grandparent),
 *        and so forth. Note that the function will always return at least one
 *        element, the dbref of the object itself.
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
void fun_lparent(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = NOTHING, par = NOTHING;
	int i = 0;
	Delim osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	};

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		SAFE_NOMATCH(buff, bufc);
		return;
	}
	else if (!(Examinable(player, it)))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	SAFE_LB_CHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, it, LBUF_SIZE);

	par = Parent(it);
	i = 1;

	while (Good_obj(par) && Examinable(player, it) && (i < mushconf.parent_nest_lim))
	{
		print_separator(&osep, buff, bufc);
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, par, LBUF_SIZE);
		it = par;
		par = Parent(par);
		i++;
	}
}

/**
 * @brief This function returns a list of objects that are parented to <object>.
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
void fun_children(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref i = NOTHING, it = NOTHING;
	char *bb_p = NULL;
	Delim osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (!strcmp(fargs[0], "#-1"))
	{
		it = NOTHING;
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (!Good_obj(it))
		{
			SAFE_NOMATCH(buff, bufc);
			return;
		}
	}

	if (!Controls(player, it) && !See_All(player))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	bb_p = *bufc;
	for (i = 0; i < mushstate.db_top; i++)
	{
		if (Parent(i) == it)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LB_CHR('#', buff, bufc);
			SAFE_LTOS(buff, bufc, i, LBUF_SIZE);
		}
	}
}

/**
 * @brief Returns the dbref of <object>'s zone (the dbref of the master object
 *        which defines the zone).
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
void fun_zone(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = NOTHING;

	if (!mushconf.have_zones)
	{
		SAFE_LB_STR("#-1 ZONES DISABLED", buff, bufc);
		return;
	}

	it = match_thing(player, fargs[0]);

	if (!Good_obj(it) || !Examinable(player, it))
	{
		SAFE_NOTHING(buff, bufc);
		return;
	}

	SAFE_LB_CHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, Zone(it), LBUF_SIZE);
}

/**
 * @brief Scan zone for content
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
void scan_zone(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref i = NOTHING, it = NOTHING;
	char *bb_p = NULL;
	int type = Func_Mask(TYPE_MASK);

	if (!mushconf.have_zones)
	{
		SAFE_LB_STR("#-1 ZONES DISABLED", buff, bufc);
		return;
	}

	if (!strcmp(fargs[0], "#-1"))
	{
		it = NOTHING;
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (!Good_obj(it))
		{
			SAFE_NOMATCH(buff, bufc);
			return;
		}
	}

	if (!Controls(player, it) && !WizRoy(player))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	bb_p = *bufc;
	for (i = 0; i < mushstate.db_top; i++)
	{
		if (Typeof(i) == type)
		{
			if (Zone(i) == it)
			{
				if (*bufc != bb_p)
				{
					SAFE_LB_CHR(' ', buff, bufc);
				}

				SAFE_LB_CHR('#', buff, bufc);
				SAFE_LTOS(buff, bufc, i, LBUF_SIZE);
			}
		}
	}
}

/**
 * @brief Evaluates an attribute on the caller's Zone object
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_zfun(char *buff, char **bufc, dbref player, dbref caller, dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;
	ATTR *ap = NULL;
	char *tbuf1 = NULL, *str = NULL;
	dbref zone = Zone(player);

	if (!mushconf.have_zones)
	{
		SAFE_LB_STR("#-1 ZONES DISABLED", buff, bufc);
		return;
	}

	if (zone == NOTHING)
	{
		SAFE_LB_STR("#-1 INVALID ZONE", buff, bufc);
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	/**
	 * find the user function attribute
	 *
	 */
	ap = atr_str(upcasestr(fargs[0]));

	if (!ap)
	{
		SAFE_LB_STR("#-1 NO SUCH USER FUNCTION", buff, bufc);
		return;
	}

	tbuf1 = atr_pget(zone, ap->number, &aowner, &aflags, &alen);

	if (!See_attr(player, zone, ap, aowner, aflags))
	{
		SAFE_LB_STR("#-1 NO PERMISSION TO GET ATTRIBUTE", buff, bufc);
		XFREE(tbuf1);
		return;
	}

	str = tbuf1;

	/**
	 * Behavior here is a little wacky. The enactor was always the
	 * player, not the cause. You can still get the caller, though.
	 *
	 */
	eval_expression_string(buff, bufc, zone, caller, player, EV_EVAL | EV_STRIP | EV_FCHECK, &str, &(fargs[1]), nfargs - 1);
	XFREE(tbuf1);
}

/**
 * @brief Does object X have attribute Y.
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
void fun_hasattr(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	ATTR *attr = NULL;
	char *tbuf = NULL;
	int aflags = 0, alen = 0, check_parents = Is_Func(CHECK_PARENTS);
	dbref thing = match_thing(player, fargs[0]), aowner = NOTHING;

	if (!Good_obj(thing))
	{
		SAFE_NOMATCH(buff, bufc);
		return;
	}
	else if (!Examinable(player, thing))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	attr = atr_str(fargs[1]);

	if (!attr)
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	if (check_parents)
	{
		atr_pget_info(thing, attr->number, &aowner, &aflags);
	}
	else
	{
		atr_get_info(thing, attr->number, &aowner, &aflags);
	}

	if (!See_attr(player, thing, attr, aowner, aflags))
	{
		SAFE_LB_CHR('0', buff, bufc);
	}
	else
	{
		if (check_parents)
		{
			tbuf = atr_pget(thing, attr->number, &aowner, &aflags, &alen);
		}
		else
		{
			tbuf = atr_get(thing, attr->number, &aowner, &aflags, &alen);
		}

		if (*tbuf)
		{
			SAFE_LB_CHR('1', buff, bufc);
		}
		else
		{
			SAFE_LB_CHR('0', buff, bufc);
		}

		XFREE(tbuf);
	}
}

/**
 * @brief Function form of %-substitution
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_v(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	dbref aowner;
	int aflags, alen;
	char *sbuf, *sbufc, *tbuf, *str;
	ATTR *ap;
	tbuf = fargs[0];

	if (isalpha(tbuf[0]) && tbuf[1])
	{
		/**
		 * Fetch an attribute from me.  First see if it exists,
		 * returning a null string if it does not.
		 *
		 */
		ap = atr_str(fargs[0]);

		if (!ap)
		{
			return;
		}

		/**
		 * If we can access it, return it, otherwise return a null string
		 *
		 */
		tbuf = atr_pget(player, ap->number, &aowner, &aflags, &alen);

		if (See_attr(player, player, ap, aowner, aflags))
		{
			SAFE_STRNCAT(buff, bufc, tbuf, alen, LBUF_SIZE);
		}

		XFREE(tbuf);
		return;
	}

	/**
	 * Not an attribute, process as %<arg>
	 *
	 */
	sbuf = XMALLOC(SBUF_SIZE, "sbuf");
	sbufc = sbuf;
	SAFE_SB_CHR('%', sbuf, &sbufc);
	SAFE_SB_STR(fargs[0], sbuf, &sbufc);
	*sbufc = '\0';
	str = sbuf;
	eval_expression_string(buff, bufc, player, caller, cause, EV_FIGNORE, &str, cargs, ncargs);
	XFREE(sbuf);
}

/**
 * @brief Get attribute from object: GET, XGET, GET_EVAL, EVAL(obj,atr)
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
void perform_get(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref thing = NOTHING, aowner = NOTHING;
	char *atr_gotten = NULL, *str = NULL;
	int attrib = 0, aflags = 0, alen, eval_it = Is_Func(GET_EVAL);

	if (Is_Func(GET_XARGS))
	{
		if (!*fargs[0] || !*fargs[1])
		{
			return;
		}

		str = XASPRINTF("str", "%s/%s", fargs[0], fargs[1]);
	}
	else
	{
		str = XASPRINTF("str", "%s", fargs[0]);
	}

	if (!parse_attrib(player, str, &thing, &attrib, 0))
	{
		SAFE_NOMATCH(buff, bufc);
		XFREE(str);
		return;
	}

	XFREE(str);

	if (attrib == NOTHING)
	{
		return;
	}

	/**
	 * There used to be code here to handle AF_IS_LOCK attributes, but
	 * parse_attrib can never return one of those. Use fun_lock instead.
	 *
	 */
	atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);

	if (eval_it)
	{
		str = atr_gotten;
		eval_expression_string(buff, bufc, thing, player, player, EV_FIGNORE | EV_EVAL, &str, (char **)NULL, 0);
	}
	else
	{
		SAFE_STRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
	}

	XFREE(atr_gotten);
}

/**
 * @brief The first form of this function is identical to get_eval(), but
 *        splits the <object> and <attribute> into two arguments, rather
 *        than having an <object>/<attribute> pair. It is provided for
 *        PennMUSH compatibility.
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
void fun_eval(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (nfargs == 1)
	{
		char *str = fargs[0];
		eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_FCHECK, &str, (char **)NULL, 0);
		return;
	}

	perform_get(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs);
}

/**
 * @brief Call a user-defined function: U, ULOCAL, UPRIVATE
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void do_ufun(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref aowner = NOTHING, thing = NOTHING;
	int aflags = 0, alen = 0, anum = 0, trace_flag = 0;
	ATTR *ap = NULL;
	char *atext = NULL, *str = NULL;
	GDATA *preserve = NULL;
	int is_local = Is_Func(U_LOCAL), is_private = Is_Func(U_PRIVATE);

	/**
	 * We need at least one argument
	 *
	 */
	if (nfargs < 1)
	{
		SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
		return;
	}

	/**
	 * First arg: <obj>/<attr> or <attr> or #lambda/<code>
	 *
	 */
	if (string_prefix(fargs[0], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen((fargs[0]) + 8);
		__xstrcpy(atext, fargs[0] + 8);
		atext[alen] = '\0';
		aowner = player;
		aflags = 0;
	}
	else
	{
		if (parse_attrib(player, fargs[0], &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(fargs[0]);
		}
		if (!ap)
		{
			return;
		}
		atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
		if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
		{
			XFREE(atext);
			return;
		}
	}

	/**
	 * If we're evaluating locally, preserve the global registers. If
	 * we're evaluating privately, preserve and wipe out.
	 *
	 */
	if (is_local)
	{
		preserve = save_global_regs("fun_ulocal.save");
	}
	else if (is_private)
	{
		preserve = mushstate.rdata;
		mushstate.rdata = NULL;
	}

	/**
	 * If the trace flag is on this attr, set the object Trace
	 *
	 */
	if (!Trace(thing) && (aflags & AF_TRACE))
	{
		trace_flag = 1;
		s_Trace(thing);
	}
	else
	{
		trace_flag = 0;
	}

	/**
	 * Evaluate it using the rest of the passed function args
	 *
	 */
	str = atext;
	eval_expression_string(buff, bufc, thing, player, cause, EV_FCHECK | EV_EVAL, &str, &(fargs[1]), nfargs - 1);
	XFREE(atext);

	/**
	 * Reset the trace flag if we need to
	 *
	 */
	if (trace_flag)
	{
		c_Trace(thing);
	}

	/**
	 * If we're evaluating locally, restore the preserved registers. If
	 * we're evaluating privately, free whatever data we had and restore.
	 *
	 */
	if (is_local)
	{
		restore_global_regs("fun_ulocal.restore", preserve);
	}
	else if (is_private)
	{
		if (mushstate.rdata)
		{
			for (int z = 0; z < mushstate.rdata->q_alloc; z++)
			{
				if (mushstate.rdata->q_regs[z])
					XFREE(mushstate.rdata->q_regs[z]);
			}

			for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
			{
				if (mushstate.rdata->x_names[z])
					XFREE(mushstate.rdata->x_names[z]);

				if (mushstate.rdata->x_regs[z])
					XFREE(mushstate.rdata->x_regs[z]);
			}

			if (mushstate.rdata->q_regs)
			{
				XFREE(mushstate.rdata->q_regs);
			}

			if (mushstate.rdata->q_lens)
			{
				XFREE(mushstate.rdata->q_lens);
			}

			if (mushstate.rdata->x_names)
			{
				XFREE(mushstate.rdata->x_names);
			}

			if (mushstate.rdata->x_regs)
			{
				XFREE(mushstate.rdata->x_regs);
			}

			if (mushstate.rdata->x_lens)
			{
				XFREE(mushstate.rdata->x_lens);
			}

			XFREE(mushstate.rdata);
		}

		mushstate.rdata = preserve;
	}
}

/**
 * @brief  Call the text of a u-function from a specific object's perspective.
 *         (i.e., get the text as the player, but execute it as the specified
 *         object.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_objcall(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref obj = NOTHING, aowner = NOTHING, thing = NOTHING;
	int aflags = 0, alen = 0, anum = 0;
	ATTR *ap = NULL;
	char *atext = NULL, *str = NULL;

	if (nfargs < 2)
	{
		SAFE_STRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
		return;
	}

	/**
	 * First arg: <obj>/<attr> or <attr> or #lambda/<code>
	 *
	 */
	if (string_prefix(fargs[1], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen((fargs[1]) + 8);
		__xstrcpy(atext, fargs[1] + 8);
		atext[alen] = '\0';
		aowner = player;
		aflags = 0;
	}
	else
	{
		if (parse_attrib(player, fargs[1], &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(fargs[1]);
		}
		if (!ap)
		{
			return;
		}
		atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
		if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
		{
			XFREE(atext);
			return;
		}
	}
	/**
	 * Find our perspective.
	 *
	 */
	obj = match_thing(player, fargs[0]);

	if (Cannot_Objeval(player, obj))
	{
		obj = player;
	}

	/**
	 * Evaluate using the rest of the passed function args.
	 *
	 */
	str = atext;
	eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str, &(fargs[2]), nfargs - 2);
	XFREE(atext);
}

/**
 * @brief Evaluate a function with local scope (i.e., preserve and restore the
 *        r-registers). Essentially like calling ulocal() but with the function
 *        string directly.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_localize(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	char *str = fargs[0];
	;
	GDATA *preserve = save_global_regs("fun_localize_save");

	eval_expression_string(buff, bufc, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	restore_global_regs("fun_localize_restore", preserve);
}

/**
 * @brief Evaluate a function with a strictly local scope -- do not pass global
 *        registers and discard any changes made to them. Like calling
 *        uprivate() but with the function string directly.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_private(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	char *str = fargs[0];
	GDATA *preserve = mushstate.rdata;
	mushstate.rdata = NULL;

	eval_expression_string(buff, bufc, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);

	if (mushstate.rdata)
	{
		for (int z = 0; z < mushstate.rdata->q_alloc; z++)
		{
			if (mushstate.rdata->q_regs[z])
				XFREE(mushstate.rdata->q_regs[z]);
		}

		for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
		{
			if (mushstate.rdata->x_names[z])
				XFREE(mushstate.rdata->x_names[z]);

			if (mushstate.rdata->x_regs[z])
				XFREE(mushstate.rdata->x_regs[z]);
		}

		if (mushstate.rdata->q_regs)
		{
			XFREE(mushstate.rdata->q_regs);
		}

		if (mushstate.rdata->q_lens)
		{
			XFREE(mushstate.rdata->q_lens);
		}

		if (mushstate.rdata->x_names)
		{
			XFREE(mushstate.rdata->x_names);
		}

		if (mushstate.rdata->x_regs)
		{
			XFREE(mushstate.rdata->x_regs);
		}

		if (mushstate.rdata->x_lens)
		{
			XFREE(mushstate.rdata->x_lens);
		}

		XFREE(mushstate.rdata);
	}

	mushstate.rdata = preserve;
}

/**
 * @brief This function returns the value of <obj>/<attr>, as if retrieved via
 *        the get() function, if the attribute exists and is readable by Player
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_default(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int attrib = 0, aflags = 0, alen = 0;
	ATTR *attr = NULL;
	char *objname = NULL, *atr_gotten = NULL, *bp = NULL, *str = fargs[0];
	objname = bp = XMALLOC(LBUF_SIZE, "objname");

	eval_expression_string(objname, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);

	/**
	 * First we check to see that the attribute exists on the object. If
	 * so, we grab it and use it.
	 *
	 */
	if (objname != NULL)
	{
		if (parse_attrib(player, objname, &thing, &attrib, 0) && (attrib != NOTHING))
		{
			attr = atr_num(attrib);

			if (attr && !(attr->flags & AF_IS_LOCK))
			{
				atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);

				if (*atr_gotten)
				{
					SAFE_STRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
					XFREE(atr_gotten);
					XFREE(objname);
					return;
				}

				XFREE(atr_gotten);
			}
		}

		XFREE(objname);
	}

	/**
	 * If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default.
	 *
	 */
	str = fargs[1];
	eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/**
 * @brief This function returns the evaluated value of <obj>/<attr>, as if
 *        retrieved via the get_eval() function, if the attribute exists and
 *        is readable by you. Otherwise, it evaluates the default case, and
 *        returns that. The default case is only evaluated if the attribute
 *        does not exist or cannot be read.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_edefault(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int attrib = 0, aflags = 0, alen = 0;
	ATTR *attr = NULL;
	char *objname = NULL, *atr_gotten = NULL, *bp = NULL, *str = fargs[0];

	objname = bp = XMALLOC(LBUF_SIZE, "objname");

	eval_expression_string(objname, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);

	/**
	 * First we check to see that the attribute exists on the object. If
	 * so, we grab it and use it.
	 *
	 */
	if (objname != NULL)
	{
		if (parse_attrib(player, objname, &thing, &attrib, 0) && (attrib != NOTHING))
		{
			attr = atr_num(attrib);

			if (attr && !(attr->flags & AF_IS_LOCK))
			{
				atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);

				if (*atr_gotten)
				{
					str = atr_gotten;
					eval_expression_string(buff, bufc, thing, player, player, EV_FIGNORE | EV_EVAL, &str, (char **)NULL, 0);
					XFREE(atr_gotten);
					XFREE(objname);
					return;
				}

				XFREE(atr_gotten);
			}
		}

		XFREE(objname);
	}

	/**
	 * If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default.
	 *
	 */
	str = fargs[1];
	eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/**
 * @brief This function returns the value of the user-defined function as
 *        defined by <attr> (or <obj>/<attr>), as if retrieved via the u()
 *        function, with <args>, if the attribute exists and is readable
 *        by you.
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
void fun_udefault(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int aflags = 0, alen = 0, anum = 0, i = 0, j = 0, trace_flag = 0;
	ATTR *ap = NULL;
	char *objname = NULL, *atext = NULL, *str = NULL, *bp = NULL, *xargs[NUM_ENV_VARS];

	if (nfargs < 2)
	{
		/**
		 * must have at least two arguments
		 *
		 */
		return;
	}

	str = fargs[0];
	objname = bp = XMALLOC(LBUF_SIZE, "objname");
	eval_expression_string(objname, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);

	/**
	 * First we check to see that the attribute exists on the object. If
	 * so, we grab it and use it.
	 *
	 */
	if (objname != NULL)
	{
		if (parse_attrib(player, objname, &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(objname);
		}

		if (ap)
		{
			atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);

			if (*atext)
			{
				/**
				 * Now we have a problem -- we've got to go
				 * eval all of those arguments to the
				 * function.
				 *
				 */
				for (i = 2, j = 0; j < NUM_ENV_VARS; i++, j++)
				{
					if ((i < nfargs) && fargs[i])
					{
						bp = xargs[j] = XMALLOC(LBUF_SIZE, "xargs[j]");
						str = fargs[i];
						eval_expression_string(xargs[j], &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
					}
					else
					{
						xargs[j] = NULL;
					}
				}

				/**
				 * We have the args, now call the ufunction.
				 * Obey the trace flag on the attribute if
				 * there is one.
				 */
				if (!Trace(thing) && (aflags & AF_TRACE))
				{
					trace_flag = 1;
					s_Trace(thing);
				}
				else
				{
					trace_flag = 0;
				}

				str = atext;
				eval_expression_string(buff, bufc, thing, player, cause, EV_FCHECK | EV_EVAL, &str, xargs, nfargs - 2);

				if (trace_flag)
				{
					c_Trace(thing);
				}

				/**
				 * Then clean up after ourselves.
				 *
				 */
				for (j = 0; j < NUM_ENV_VARS; j++)
					if (xargs[j])
					{
						XFREE(xargs[j]);
					}

				XFREE(atext);
				XFREE(objname);
				return;
			}

			XFREE(atext);
		}

		XFREE(objname);
	}

	/**
	 * If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default.
	 *
	 */
	str = fargs[1];
	eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/**
 * @brief Evaluate from a specific object's perspective.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_objeval(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	dbref obj = NOTHING;
	char *name = NULL, *bp = NULL, *str = NULL;

	if (!*fargs[0])
	{
		return;
	}

	name = bp = XMALLOC(LBUF_SIZE, "bp");
	str = fargs[0];
	eval_expression_string(name, &bp, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	obj = match_thing(player, name);

	/**
	 * In order to evaluate from something else's viewpoint, you must
	 * have the same owner as it, or be a wizard (unless
	 * objeval_requires_control is turned on, in which case you must
	 * control it, period). Otherwise, we default to evaluating from our
	 * own viewpoint. Also, you cannot evaluate things from the point of
	 * view of God.
	 *
	 */
	if (Cannot_Objeval(player, obj))
	{
		obj = player;
	}

	str = fargs[1];
	eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	XFREE(name);
}

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
void fun_num(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	SAFE_LB_CHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, match_thing(player, fargs[0]), LBUF_SIZE);
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
void fun_pmatch(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
			SAFE_LB_CHR('#', buff, bufc);
			SAFE_LTOS(buff, bufc, thing, LBUF_SIZE);
		}
		else
		{
			SAFE_NOTHING(buff, bufc);
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
	SAFE_LB_STR(name, temp, &tp);

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
			SAFE_LB_CHR('#', buff, bufc);
			SAFE_LTOS(buff, bufc, (int)*p_ptr, LBUF_SIZE);
		}
		else
		{
			SAFE_NOTHING(buff, bufc);
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
		SAFE_STRNCAT(buff, bufc, "#-2", 3, LBUF_SIZE);
	}
	else if (Good_obj(thing) && isPlayer(thing))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, thing, LBUF_SIZE);
	}
	else
	{
		SAFE_NOTHING(buff, bufc);
	}
}

/**
 * @brief If <object> is a dbref, if the dbref is valid, that dbref will be
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
void fun_pfind(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref thing;

	if (*fargs[0] == '#')
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, match_thing(player, fargs[0]), LBUF_SIZE);
		return;
	}

	if (!((thing = lookup_player(player, fargs[0], 1)) == NOTHING))
	{
		SAFE_LB_CHR('#', buff, bufc);
		SAFE_LTOS(buff, bufc, thing, LBUF_SIZE);
		return;
	}
	else
	{
		SAFE_NOMATCH(buff, bufc);
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
void fun_locate(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
		SAFE_NOPERM(buff, bufc);
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

	SAFE_LB_CHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, what, LBUF_SIZE);
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
				SAFE_LB_STR("#-1 ARGUMENT OUT OF RANGE", buff, bufc);
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

					SAFE_LB_STR((char *)attr->name, buff, bufc);
					got++;
				}
			}
		}

		if (count_only)
		{
			SAFE_LTOS(buff, bufc, total, LBUF_SIZE);
		}
	}
	else
	{
		if (!mushconf.lattr_oldstyle)
		{
			SAFE_NOMATCH(buff, bufc);
		}
		else if (count_only)
		{
			SAFE_LB_CHR('0', buff, bufc);
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
void fun_search(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause, char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref thing = NOTHING;
	char *bp = NULL, *nbuf = NULL;
	SEARCH searchparm;

	/**
	 * Set up for the search.  If any errors, abort.
	 *
	 */
	if (!search_setup(player, fargs[0], &searchparm))
	{
		SAFE_LB_STR("#-1 ERROR DURING SEARCH", buff, bufc);
		return;
	}

	/**
	 * Do the search and report the results
	 *
	 */
	olist_push();
	search_perform(player, cause, &searchparm);
	bp = *bufc;
	nbuf = XMALLOC(SBUF_SIZE, "nbuf");

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

		SAFE_LB_STR(nbuf, buff, bufc);
	}

	XFREE(nbuf);
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
void fun_stats(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
			SAFE_LB_STR("#-1 NOT FOUND", buff, bufc);
			return;
		}
	}

	if (!get_stats(player, who, &statinfo))
	{
		SAFE_LB_STR("#-1 ERROR GETTING STATS", buff, bufc);
		return;
	}

	SAFE_SPRINTF(buff, bufc, "%d %d %d %d %d %d %d %d", statinfo.s_total, statinfo.s_rooms, statinfo.s_exits, statinfo.s_things, statinfo.s_players, statinfo.s_unknown, statinfo.s_going, statinfo.s_garbage);
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

		attr = atr_num(ca);

		if (attr)
		{
			str = (char *)attr->name;

			if (str && *str)
			{
				k += strlen(((ATTR *)atr_num(ca))->name);
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
			if (Examinable(player, thing) && Read_attr(player, thing, ap, aowner, aflags))
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
 *          attributes on <object>. To just get a count of the number of bytes
 *          used by all attribute text on an object, use 'objmem(<object> / *)'.
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
void fun_objmem(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref thing = NOTHING;

	if (strchr(fargs[0], '/'))
	{
		SAFE_LTOS(buff, bufc, mem_usage_attr(player, fargs[0]), LBUF_SIZE);
		return;
	}

	thing = match_thing(player, fargs[0]);

	if (!Good_obj(thing) || !Examinable(player, thing))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	SAFE_LTOS(buff, bufc, mem_usage(thing), LBUF_SIZE);
}

/**
 * @brief   Returns the sum total of the size, in bytes, of all objects in the
 *          database that are owned by <player> (equivalent to doing an objmem()
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
void fun_playmem(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int tot = 0;
	dbref thing = match_thing(player, fargs[0]);
	dbref j = NOTHING;

	if (!Good_obj(thing) || !Examinable(player, thing))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	for (j = 0; j < mushstate.db_top; j++)
	{
		if (Owner(j) == thing)
		{
			tot += mem_usage(j);
		}
	}

	SAFE_LTOS(buff, bufc, tot, LBUF_SIZE);
}

/**
 * @brief Returns a string indicating the object type of <object>, either EXIT,
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
void fun_type(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it;
	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		SAFE_NOMATCH(buff, bufc);
		return;
	}

	switch (Typeof(it))
	{
	case TYPE_ROOM:
		SAFE_STRNCAT(buff, bufc, "ROOM", 4, LBUF_SIZE);
		break;

	case TYPE_EXIT:
		SAFE_STRNCAT(buff, bufc, "EXIT", 4, LBUF_SIZE);
		break;

	case TYPE_PLAYER:
		SAFE_STRNCAT(buff, bufc, "PLAYER", 6, LBUF_SIZE);
		break;

	case TYPE_THING:
		SAFE_STRNCAT(buff, bufc, "THING", 5, LBUF_SIZE);
		break;

	default:
		SAFE_LB_STR("#-1 ILLEGAL TYPE", buff, bufc);
	}

	return;
}

/**
 * @brief Returns 1 if <object> is of type <type>, and 0 otherwise. Valid
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
void fun_hastype(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		SAFE_NOMATCH(buff, bufc);
		return;
	}

	if (!fargs[1] || !*fargs[1])
	{
		SAFE_LB_STR("#-1 NO SUCH TYPE", buff, bufc);
		return;
	}

	switch (*fargs[1])
	{
	case 'r':
	case 'R':
		SAFE_BOOL(buff, bufc, isRoom(it));
		break;

	case 'e':
	case 'E':
		SAFE_BOOL(buff, bufc, isExit(it));
		break;

	case 'p':
	case 'P':
		SAFE_BOOL(buff, bufc, isPlayer(it));
		break;

	case 't':
	case 'T':
		SAFE_BOOL(buff, bufc, isThing(it));
		break;

	default:
		SAFE_LB_STR("#-1 NO SUCH TYPE", buff, bufc);
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
void fun_lastcreate(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
		SAFE_NOTHING(buff, bufc);
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
		SAFE_NOTHING(buff, bufc);
		return;
	}

	obj_str = atr_get(obj, A_NEWOBJS, &aowner, &aflags, &alen);

	if (!*obj_str)
	{
		XFREE(obj_str);
		SAFE_NOTHING(buff, bufc);
		return;
	}

	for (p = strtok_r(obj_str, " ", &tokst), i = 0; p && (i < 4); p = strtok_r(NULL, " ", &tokst), i++)
	{
		obj_list[i] = (int)strtol(p, (char **)NULL, 10);
	}

	XFREE(obj_str);
	SAFE_LB_CHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, obj_list[obj_type], LBUF_SIZE);
}

/*
 * ---------------------------------------------------------------------------
 * fun_speak: Complex say-format-processing function.
 *
 * speak(<speaker>, <string>[, <substitute for "says,"> [, <transform>[,
 * <empty>[, <open>[, <close>]]]]])
 *
 * <string> can be a plain string (treated like say), :<foo> (pose), : <foo>
 * (pose/nospace), ;<foo> (pose/nospace), |<foo> (emit), or "<foo> (also
 * treated like say).
 *
 * If we are given <transform>, we parse through the string, pulling out the
 * things that fall between <open in> and <close in> (which default to
 * double-quotes). We pass the speech between those things to the u-function
 * (or everything, if a plain say string), with the speech as %0, <object>'s
 * resolved dbref as %1, and the speech part number (plain say begins
 * numbering at 0, others at 1) as %2.
 *
 * We rely on the u-function to do any necessary formatting of the return
 * string, including putting quotes (or whatever) back around it if
 * necessary.
 *
 * If the return string is null, we call <empty>, with <object>'s resolved dbref
 * as %0, and the speech part number as %1.
 */

/**
 * @brief
 *
 * @param speaker DBref of speaker
 * @param sname Speaker's name
 * @param str Text being said
 * @param key Say or pose key
 * @param say_str Say string
 * @param trans_str Transform string
 * @param empty_str Buffer String
 * @param open_sep Open Separator
 * @param close_sep Close Separator
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 */
void transform_say(dbref speaker, char *sname, char *str, int key, char *say_str, char *trans_str, char *empty_str, const Delim *open_sep, const Delim *close_sep, dbref player, dbref caller, dbref cause, char *buff, char **bufc)
{
	char *sp, *ep, *save, *tp, *bp;
	char *result, *tstack[3], *estack[2];
	char *tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	int spos, trans_len, empty_len;
	int done = 0;

	if (trans_str && *trans_str)
	{
		trans_len = strlen(trans_str);
	}
	else
	{
		/**
		 * should never happen; caller should check
		 *
		 */
		XFREE(tbuf);
		return;
	}

	/**
	 * Find the start of the speech string. Copy up to it.
	 *
	 */
	sp = str;

	if (key == SAY_SAY)
	{
		spos = 0;
	}
	else
	{
		save = split_token(&sp, open_sep);
		SAFE_LB_STR(save, buff, bufc);

		if (!sp)
		{
			XFREE(tbuf);
			return;
		}

		spos = 1;
	}

	tstack[1] = XMALLOC(SBUF_SIZE, "tstack[1]");
	tstack[2] = XMALLOC(SBUF_SIZE, "tstack[2]");

	if (empty_str && *empty_str)
	{
		empty_len = strlen(empty_str);
		estack[0] = XMALLOC(SBUF_SIZE, "estack[0]");
		estack[1] = XMALLOC(SBUF_SIZE, "estack[1]");
	}
	else
	{
		empty_len = 0;
	}

	result = XMALLOC(LBUF_SIZE, "result");

	while (!done)
	{
		/**
		 * Find the end of the speech string.
		 *
		 */
		ep = sp;
		sp = split_token(&ep, close_sep);

		/*
		 * Pass the stuff in-between through the u-functions.
		 *
		 */
		tstack[0] = sp;
		XSPRINTF(tstack[1], "#%d", speaker);
		XSPRINTF(tstack[2], "%d", spos);
		XMEMCPY(tbuf, trans_str, trans_len);
		tbuf[trans_len] = '\0';
		tp = tbuf;
		bp = result;
		eval_expression_string(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &tp, tstack, 3);

		if (result && *result)
		{
			if ((key == SAY_SAY) && (spos == 0))
			{
				SAFE_SPRINTF(buff, bufc, "%s %s %s", sname, say_str, result);
			}
			else
			{
				SAFE_LB_STR(result, buff, bufc);
			}
		}
		else if (empty_len > 0)
		{
			XSPRINTF(estack[0], "#%d", speaker);
			XSPRINTF(estack[1], "%d", spos);
			XMEMCPY(tbuf, empty_str, empty_len);
			tbuf[empty_len] = '\0';

			tp = tbuf;
			bp = result;
			eval_expression_string(result, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &tp, estack, 2);

			if (result && *result)
			{
				SAFE_LB_STR(result, buff, bufc);
			}
		}

		/**
		 * If there's more, find it and copy it. sp will point to the
		 * beginning of the next speech string.
		 *
		 */
		if (ep && *ep)
		{
			sp = ep;
			save = split_token(&sp, open_sep);
			SAFE_LB_STR(save, buff, bufc);

			if (!sp)
			{
				done = 1;
			}
		}
		else
		{
			done = 1;
		}

		spos++;
	}

	XFREE(result);
	XFREE(tstack[1]);
	XFREE(tstack[2]);

	if (empty_len > 0)
	{
		XFREE(estack[0]);
		XFREE(estack[1]);
	}

	if (trans_str)
	{
		XFREE(trans_str);
	}

	if (empty_str)
	{
		XFREE(empty_str);
	}
	XFREE(tbuf);
}

/**
 * @brief This function is used to format speech-like constructs, and is
 *        capable of transforming text within a speech string; it is useful for
 *        implementing "language code" and the like.
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
void fun_speak(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, obj1 = NOTHING, obj2 = NOTHING, aowner1 = NOTHING, aowner2 = NOTHING;
	Delim isep, osep; /* really open and close separators */
	int aflags1 = 0, alen1 = 0, anum1 = 0, aflags2 = 0, alen2 = 0, anum2 = 0, is_transform = 0, key = 0;
	ATTR *ap1 = NULL, *ap2 = NULL;
	char *atext1 = NULL, *atext2 = NULL, *str = NULL, *say_str = NULL, *s = NULL, *tname = NULL;

	/**
	 * Delimiter processing here is different. We have to do some funky
	 * stuff to make sure that a space delimiter is really an intended
	 * space, not delim_check() defaulting.
	 *
	 */
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 7, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &isep, DELIM_STRING))
	{
		return;
	}

	if ((isep.len == 1) && (isep.str[0] == ' '))
	{
		if ((nfargs < 6) || !fargs[5] || !*fargs[5])
		{
			isep.str[0] = '"';
		}
	}

	if (nfargs < 7)
	{
		XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 7, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	/**
	 * We have three possible cases for the speaker: <thing string>&<name
	 * string> &<name string> (speaker defaults to player) <thing string>
	 * (name string defaults to name of thing)
	 *
	 */
	if (*fargs[0] == '&')
	{
		/**
		 * name only
		 *
		 */
		thing = player;
		tname = fargs[0];
		tname++;
	}
	else if (((s = strchr(fargs[0], '&')) == NULL) || !*s++)
	{
		/**
		 * thing only
		 *
		 */
		thing = match_thing(player, fargs[0]);

		if (!Good_obj(thing))
		{
			SAFE_NOMATCH(buff, bufc);
			return;
		}

		tname = Name(thing);
	}
	else
	{
		/**
		 * thing and name
		 *
		 */
		*(s - 1) = '\0';
		thing = match_thing(player, fargs[0]);

		if (!Good_obj(thing))
		{
			SAFE_NOMATCH(buff, bufc);
			return;
		}

		tname = s;
	}

	/**
	 * Must have an input string. Otherwise silent fail.
	 *
	 */
	if (!fargs[1] || !*fargs[1])
	{
		return;
	}

	/**
	 * Check if there's a string substituting for "says,".
	 *
	 */
	if ((nfargs >= 3) && fargs[2] && *fargs[2])
	{
		say_str = fargs[2];
	}
	else
	{
		say_str = (char *)(mushconf.comma_say ? "says," : "says");
	}

	/**
	 * Find the u-function. If we have a problem with it, we just default
	 * to no transformation.
	 *
	 */
	atext1 = atext2 = NULL;
	is_transform = 0;

	if (nfargs >= 4)
	{
		if (parse_attrib(player, fargs[3], &obj1, &anum1, 0))
		{
			if ((anum1 == NOTHING) || !(Good_obj(obj1)))
				ap1 = NULL;
			else
				ap1 = atr_num(anum1);
		}
		else
		{
			obj1 = player;
			ap1 = atr_str(fargs[3]);
		}

		if (ap1)
		{
			atext1 = atr_pget(obj1, ap1->number, &aowner1, &aflags1, &alen1);

			if (!*atext1 || !(See_attr(player, obj1, ap1, aowner1, aflags1)))
			{
				XFREE(atext1);
				atext1 = NULL;
			}
			else
			{
				is_transform = 1;
			}
		}
		else
		{
			atext1 = NULL;
		}
	}

	/**
	 * Do some up-front work on the empty-case u-function, too.
	 *
	 */
	if (nfargs >= 5)
	{
		if (parse_attrib(player, fargs[4], &obj2, &anum2, 0))
		{
			if ((anum2 == NOTHING) || !(Good_obj(obj2)))
				ap2 = NULL;
			else
				ap2 = atr_num(anum2);
		}
		else
		{
			obj2 = player;
			ap2 = atr_str(fargs[4]);
		}

		if (ap2)
		{
			atext2 = atr_pget(obj2, ap2->number, &aowner2, &aflags2, &alen2);

			if (!*atext2 || !(See_attr(player, obj2, ap2, aowner2, aflags2)))
			{
				XFREE(atext2);
				atext2 = NULL;
			}
		}
		else
		{
			atext2 = NULL;
		}
	}

	/**
	 * Take care of the easy case, no u-function.
	 *
	 */
	if (!is_transform)
	{
		switch (*fargs[1])
		{
		case ':':
			if (*(fargs[1] + 1) == ' ')
			{
				SAFE_SPRINTF(buff, bufc, "%s%s", tname, fargs[1] + 2);
			}
			else
			{
				SAFE_SPRINTF(buff, bufc, "%s %s", tname, fargs[1] + 1);
			}

			break;

		case ';':
			SAFE_SPRINTF(buff, bufc, "%s%s", tname, fargs[1] + 1);
			break;

		case '|':
			SAFE_SPRINTF(buff, bufc, "%s", fargs[1] + 1);
			break;

		case '"':
			SAFE_SPRINTF(buff, bufc, "%s %s \"%s\"", tname, say_str, fargs[1] + 1);
			break;

		default:
			SAFE_SPRINTF(buff, bufc, "%s %s \"%s\"", tname, say_str, fargs[1]);
			break;
		}

		return;
	}

	/**
	 * Now for the nasty stuff.
	 *
	 */
	switch (*fargs[1])
	{
	case ':':
		SAFE_LB_STR(tname, buff, bufc);

		if (*(fargs[1] + 1) != ' ')
		{
			SAFE_LB_CHR(' ', buff, bufc);
			str = fargs[1] + 1;
			key = SAY_POSE;
		}
		else
		{
			str = fargs[1] + 2;
			key = SAY_POSE_NOSPC;
		}

		break;

	case ';':
		SAFE_LB_STR(tname, buff, bufc);
		str = fargs[1] + 1;
		key = SAY_POSE_NOSPC;
		break;

	case '|':
		str = fargs[1] + 1;
		key = SAY_EMIT;
		break;

	case '"':
		str = fargs[1] + 1;
		key = SAY_SAY;
		break;

	default:
		str = fargs[1];
		key = SAY_SAY;
	}

	transform_say(thing, tname, str, key, say_str, atext1, atext2, &isep, &osep, player, caller, cause, buff, bufc);
}
