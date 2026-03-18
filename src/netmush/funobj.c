/**
 * @file funobj.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object-focused built-ins: basic navigation, names, and proximity
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
void fun_objid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	if (Good_obj(it))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, it, LBUF_SIZE);

		XSAFELBCHR(':', buff, bufc);
		XSAFELTOS(buff, bufc, CreateTime(it), LBUF_SIZE);
	}
	else
	{
		XSAFENOTHING(buff, bufc);
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
void fun_con(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	if (Good_loc(it) && (Examinable(player, it) || (where_is(player) == it) || (it == cause)))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, Contents(it), LBUF_SIZE);
		return;
	}

	XSAFENOTHING(buff, bufc);
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
void fun_exit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
			/* Base dark of starting location can affect visibility */
			key |= VE_BASE_DARK;
		}

		for (exit = Exits(it); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
		{
			if (Exit_Visible(exit, player, key))
			{
				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, exit, LBUF_SIZE);
				return;
			}
		}
	}

	XSAFENOTHING(buff, bufc);
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
void fun_next(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, Next(it), LBUF_SIZE);
				return;
			}
			else
			{
				key = 0;

				if (ex_here)
				{
					key |= VE_LOC_XAM;
				}

				int loc_is_dark = Dark(loc);
				if (loc_is_dark)
				{
					key |= VE_LOC_DARK;
				}

				for (exit = Next(it); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
				{
					if (Exit_Visible(exit, player, key))
					{
						XSAFELBCHR('#', buff, bufc);
						XSAFELTOS(buff, bufc, exit, LBUF_SIZE);
						return;
					}
				}
			}
		}
	}

	XSAFENOTHING(buff, bufc);
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
void handle_loc(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	it = match_thing(player, fargs[0]);

	if (locatable(player, it, cause))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, ((FUN *)fargs[-1])->flags & (0x01) ? where_is(it) : Location(it), LBUF_SIZE);
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}
}

/**
 * @brief Returns the recursed location of something (specifying \#levels)
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
void fun_rloc(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, it, LBUF_SIZE);
		return;
	}

	XSAFENOTHING(buff, bufc);
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
void fun_room(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, it, LBUF_SIZE);
				return;
			}
		}

		XSAFENOTHING(buff, bufc);
	}
	else if (isRoom(it))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, it, LBUF_SIZE);
	}
	else
	{
		XSAFENOTHING(buff, bufc);
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
void fun_owner(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, it, LBUF_SIZE);
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
void fun_controls(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref x = match_thing(player, fargs[0]), y = match_thing(player, fargs[1]);

	if (!Good_obj(x))
	{
		XSAFELBSTR("#-1 ARG1 NOT FOUND", buff, bufc);
		return;
	}

	if (!Good_obj(y))
	{
		XSAFELBSTR("#-1 ARG2 NOT FOUND", buff, bufc);
		return;
	}

	XSAFEBOOL(buff, bufc, Controls(x, y));
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
void fun_sees(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]), thing = match_thing(player, fargs[1]);
	dbref loc = NOTHING;

	if (!Good_obj(it) || !Good_obj(thing))
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	loc = Location(thing);
	XSAFEBOOL(buff, bufc, (isExit(thing) ? Can_See_Exit(it, thing, Darkened(it, loc)) : Can_See(it, thing, Sees_Always(it, loc))));
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
void fun_nearby(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref obj1 = match_thing(player, fargs[0]), obj2 = match_thing(player, fargs[1]);
	bool nb1 = nearby_or_control(player, obj1);
	bool nb2 = nearby_or_control(player, obj2);

	if (!(nb1 || nb2))
	{
		XSAFELBCHR('0', buff, bufc);
	}
	else
	{
		XSAFEBOOL(buff, bufc, nearby(obj1, obj2));
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
 *        hears(&lt;object&gt;, &lt;speaker&gt;): Can &lt;object&gt; hear &lt;speaker&gt; speak?
 *        knows(&lt;object&gt;, &lt;target&gt;): Can &lt;object&gt; know about &lt;target&gt;?
 *        moves(&lt;object&gt;, &lt;mover&gt;): Can &lt;object&gt; see &lt;mover&gt; move?
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
void handle_okpres(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int oper = Func_Mask(PRESFN_OPER);
	dbref object = match_thing(player, fargs[0]), actor = match_thing(player, fargs[1]);

	if (!Good_obj(object) || !Good_obj(actor))
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	if (oper == PRESFN_HEARS)
	{
		XSAFEBOOL(buff, bufc, ((Unreal(actor) && !Check_Heard(object, actor)) || (Unreal(object) && !Check_Hears(actor, object))) ? 0 : 1);
	}
	else if (oper == PRESFN_MOVES)
	{
		XSAFEBOOL(buff, bufc, ((Unreal(actor) && !Check_Noticed(object, actor)) || (Unreal(object) && !Check_Notices(actor, object))) ? 0 : 1);
	}
	else if (oper == PRESFN_KNOWS)
	{
		XSAFEBOOL(buff, bufc, ((Unreal(actor) && !Check_Known(object, actor)) || (Unreal(object) && !Check_Knows(actor, object))) ? 0 : 1);
	}
	else
	{
		XSAFELBCHR('0', buff, bufc);
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
void handle_name(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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
			XSAFELBSTR("#-1 TOO FAR AWAY TO SEE", buff, bufc);
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
void handle_pronoun(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);
	char *str = NULL, *pronouns[4] = {"%o", "%p", "%s", "%a"};

	if (!Good_obj(it) || (!isPlayer(it) && !nearby_or_control(player, it)))
	{
		XSAFENOMATCH(buff, bufc);
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
