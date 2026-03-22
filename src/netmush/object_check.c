/**
 * @file object_check.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Database integrity checking: consistency validation and repair of object references, chains, and structure
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
#include <unistd.h>

int check_type;

/* ---------------------------------------------------------------------------
 * Log_pointer_err, Log_header_err, Log_simple_damage: Write errors to the
 * log file.
 */

void Log_pointer_err(dbref prior, dbref obj, dbref loc, dbref ref, const char *reftype, const char *errtype)
{
	const char *obj_type, *ref_type;
	char *obj_name, *obj_loc;
	char *ref_name;
	obj_type = log_gettype(obj);
	obj_name = log_getname(obj);
	ref_type = log_gettype(ref);
	ref_name = log_getname(ref);

	if (loc != NOTHING)
	{
		obj_loc = log_getname(Location(obj));
		log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s in %s: %s %s %s %s", obj_type, obj_name, obj_loc, ((prior == NOTHING) ? reftype : "Next pointer"), ref_type, ref_name, errtype);
		XFREE(obj_loc);
	}
	else
	{
		log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s: %s %s %s %s", obj_type, obj_name, ((prior == NOTHING) ? reftype : "Next pointer"), ref_type, ref_name, errtype);
	}

	/* obj_type and ref_type are now static strings - do not free */
	XFREE(obj_name);
	XFREE(ref_name);
}

void Log_header_err(dbref obj, dbref loc, dbref val, int is_object, const char *valtype, const char *errtype)
{
	const char *obj_type, *val_type;
	char *obj_name, *obj_loc;
	char *val_name;
	obj_type = log_gettype(obj);
	obj_name = log_getname(obj);

	if (loc != NOTHING)
	{
		obj_loc = log_getname(Location(obj));

		if (is_object)
		{
			val_type = log_gettype(val);
			val_name = log_getname(val);
			log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s in %s: %s %s %s", obj_type, obj_name, obj_loc, val_type, val_name, errtype);
			/* val_type is now a static string - do not free */
			XFREE(val_name);
		}
		else
		{
			log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s in %s: %d %s", obj_type, obj_name, obj_loc, val, errtype);
		}

		XFREE(obj_loc);
	}
	else
	{
		if (is_object)
		{
			val_type = log_gettype(val);
			val_name = log_getname(val);
			log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s: %s %s %s", obj_type, obj_name, val_type, val_name, errtype);
			/* val_type is now a static string - do not free */
			XFREE(val_name);
		}
		else
		{
			log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s: %d %s", obj_type, obj_name, val, errtype);
		}
	}

	/* obj_type is now a static string - do not free */
	XFREE(obj_name);
}

void Log_simple_err(dbref obj, dbref loc, const char *errtype)
{
	const char *obj_type;
	char *obj_name, *obj_loc;
	obj_type = log_gettype(obj);
	obj_name = log_getname(obj);

	if (loc != NOTHING)
	{
		obj_loc = log_getname(Location(obj));
		log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s in %s: %s", obj_type, obj_name, obj_loc, errtype);
		XFREE(obj_loc);
	}
	else
	{
		log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "%s %s: %s", obj_type, obj_name, errtype);
	}

	/* obj_type is now a static string - do not free */
	XFREE(obj_name);
}

/* ---------------------------------------------------------------------------
 * check_dead_refs: Look for references to GOING or illegal objects.
 */

void check_pennies(dbref thing, int limit, const char *qual)
{
	int j;

	if (Going(thing))
	{
		return;
	}

	j = Pennies(thing);

	if (isRoom(thing) || isExit(thing))
	{
		if (j)
		{
			Log_header_err(thing, NOTHING, j, 0, qual, "is strange.  Reset.");
			s_Pennies(thing, 0);
		}
	}
	else if (j == 0)
	{
		Log_header_err(thing, NOTHING, j, 0, qual, "is zero.");
	}
	else if (j < 0)
	{
		Log_header_err(thing, NOTHING, j, 0, qual, "is negative.");
	}
	else if (j > limit)
	{
		Log_header_err(thing, NOTHING, j, 0, qual, "is excessive.");
	}
}

void check_dead_refs(void)
{
	dbref targ, owner, i, j;
	int aflags, dirty;
	char *str;
	FWDLIST *fp;
	PROPDIR *pp;
	for (i = 0; i < mushstate.db_top; i++)
	{
		/*
		 * Check the parent
		 */
		targ = Parent(i);
		do
		{
			if (Good_obj(targ))
			{
				if (Going(targ))
				{
					s_Parent(i, (-1));
					if (!mushstate.standalone)
					{
						owner = Owner(i);
						if (Good_owner(owner) && !Quiet(i) && !Quiet(owner))
						{
							notify_check(owner, owner, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s cleared on %s(#%d)", "Parent", Name(i), i);
						}
					}
					else
					{
						Log_header_err(i, Location(i), targ, 1, "Parent", "is invalid.  Cleared.");
					}
				}
			}
			else if (targ != NOTHING)
			{
				Log_header_err(i, Location(i), targ, 1, "Parent", "is invalid.  Cleared.");
				s_Parent(i, (-1));
			}
		} while (0);
		/*
		 * Check the zone
		 */
		targ = Zone(i);
		do
		{
			if (Good_obj(targ))
			{
				if (Going(targ))
				{
					s_Zone(i, (-1));
					if (!mushstate.standalone)
					{
						owner = Owner(i);
						if (Good_owner(owner) && !Quiet(i) && !Quiet(owner))
						{
							notify_check(owner, owner, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s cleared on %s(#%d)", "Zone", Name(i), i);
						}
					}
					else
					{
						Log_header_err(i, Location(i), targ, 1, "Zone", "is invalid.  Cleared.");
					}
				}
			}
			else if (targ != NOTHING)
			{
				Log_header_err(i, Location(i), targ, 1, "Zone", "is invalid.  Cleared.");
				s_Zone(i, (-1));
			}
		} while (0);

		switch (Typeof(i))
		{
		case TYPE_PLAYER:
		case TYPE_THING:
			if (Going(i))
			{
				break;
			}

			/*
			 * Check the home
			 */
			targ = Home(i);
			do
			{
				if (Good_obj(targ))
				{
					if (Going(targ))
					{
						s_Home(i, new_home(i));
						if (!mushstate.standalone)
						{
							owner = Owner(i);
							if (Good_owner(owner) && !Quiet(i) && !Quiet(owner))
							{
								notify_check(owner, owner, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s cleared on %s(#%d)", "Home", Name(i), i);
							}
						}
						else
						{
							Log_header_err(i, Location(i), targ, 1, "Home", "is invalid.  Cleared.");
						}
					}
				}
				else if (targ != NOTHING)
				{
					Log_header_err(i, Location(i), targ, 1, "Home", "is invalid.  Cleared.");
					s_Home(i, new_home(i));
				}
			} while (0);
			/*
			 * Check the location
			 */
			targ = Location(i);

			if (!Good_obj(targ))
			{
				Log_pointer_err(NOTHING, i, NOTHING, targ, "Location", "is invalid.  Moved to home.");
				s_Location(i, NOTHING);
				s_Next(i, NOTHING);
				move_object(i, HOME);
			}

			/*
			 * Check for self-referential Next()
			 */

			if (Next(i) == i)
			{
				Log_simple_err(i, NOTHING, "Next points to self.  Next cleared.");
				s_Next(i, NOTHING);
			}

			if (check_type & DBCK_FULL)
			{
				/*
				 * Check wealth or value
				 */
				targ = OBJECT_ENDOWMENT(mushconf.createmax);

				if (OwnsOthers(i))
				{
					targ += mushconf.paylimit;
					check_pennies(i, targ, "Wealth");
				}
				else
				{
					check_pennies(i, targ, "Value");
				}
			}

			break;

		case TYPE_ROOM:
			/*
			 * Check the dropto
			 */
			targ = Dropto(i);

			if (targ != HOME)
			{
				do
				{
					if (Good_obj(targ))
					{
						if (Going(targ))
						{
							s_Dropto(i, (-1));
							if (!mushstate.standalone)
							{
								owner = Owner(i);
								if (Good_owner(owner) && !Quiet(i) && !Quiet(owner))
								{
									notify_check(owner, owner, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s cleared on %s(#%d)", "Dropto", Name(i), i);
								}
							}
							else
							{
								Log_header_err(i, Location(i), targ, 1, "Dropto", "is invalid.  Cleared.");
							}
						}
					}
					else if (targ != NOTHING)
					{
						Log_header_err(i, Location(i), targ, 1, "Dropto", "is invalid.  Cleared.");
						s_Dropto(i, (-1));
					}
				} while (0);
			}

			if (check_type & DBCK_FULL)
			{
				/*
				 * NEXT should be null
				 */
				if (Next(i) != NOTHING)
				{
					Log_header_err(i, NOTHING, Next(i), 1, "Next pointer", "should be NOTHING.  Reset.");
					s_Next(i, NOTHING);
				}

				/*
				 * LINK should be null
				 */

				if (Link(i) != NOTHING)
				{
					Log_header_err(i, NOTHING, Link(i), 1, "Link pointer ", "should be NOTHING.  Reset.");
					s_Link(i, NOTHING);
				}

				/*
				 * Check value
				 */
				check_pennies(i, 1, "Value");
			}

			break;

		case TYPE_EXIT:
			/*
			 * If it points to something GOING, set it going
			 */
			targ = Location(i);

			if (Good_obj(targ))
			{
				if (Going(targ))
				{
					s_Going(i);
				}
			}
			else if ((targ == HOME) || (targ == AMBIGUOUS) || (targ == NOTHING))
			{
				/*
				 * null case, always valid
				 */
			}
			else if (targ != NOTHING)
			{
				Log_header_err(i, Exits(i), targ, 1, "Destination", "is invalid.  Exit destroyed.");
				s_Going(i);
			}
			else
			{
				Log_header_err(i, Exits(i), targ, 1, "Destination", "is not a valid type.  Exit destroyed.");
				s_Going(i);
			}

			/*
			 * Check for self-referential Next()
			 */

			if (Next(i) == i)
			{
				Log_simple_err(i, NOTHING, "Next points to self.  Next cleared.");
				s_Next(i, NOTHING);
			}

			if (check_type & DBCK_FULL)
			{
				/*
				 * CONTENTS should be null
				 */
				if (Contents(i) != NOTHING)
				{
					Log_header_err(i, Exits(i), Contents(i), 1, "Contents", "should be NOTHING.  Reset.");
					s_Contents(i, NOTHING);
				}

				/*
				 * LINK should be null
				 */

				if (Link(i) != NOTHING)
				{
					Log_header_err(i, Exits(i), Link(i), 1, "Link", "should be NOTHING.  Reset.");
					s_Link(i, NOTHING);
				}

				/*
				 * Check value
				 */
				check_pennies(i, 1, "Value");
			}

			break;

		case TYPE_GARBAGE:
			break;

		default:
			/*
			 * Funny object type, destroy it
			 */
			Log_simple_err(i, NOTHING, "Funny object type.  Destroyed.");
			destroy_obj(NOTHING, i);
		}

		/*
		 * Check forwardlist
		 */
		dirty = 0;

		if (H_Fwdlist(i) && ((fp = fwdlist_get(i)) != NULL))
		{
			for (j = 0; j < fp->count; j++)
			{
				targ = fp->data[j];

				if (Good_obj(targ) && Going(targ))
				{
					fp->data[j] = NOTHING;
					dirty = 1;
				}
				else if (!Good_obj(targ) && (targ != NOTHING))
				{
					fp->data[j] = NOTHING;
					dirty = 1;
				}
			}
		}

		if (dirty)
		{
			str = XMALLOC(LBUF_SIZE, "str");
			(void)fwdlist_rewrite(fp, str);
			atr_get_info(i, A_FORWARDLIST, &owner, &aflags);
			atr_add(i, A_FORWARDLIST, str, owner, aflags);
			XFREE(str);
		}

		/*
		 * Check propdir
		 */
		dirty = 0;

		if (H_Propdir(i) && ((pp = propdir_get(i)) != NULL))
		{
			for (j = 0; j < pp->count; j++)
			{
				targ = pp->data[j];

				if (Good_obj(targ) && Going(targ))
				{
					pp->data[j] = NOTHING;
					dirty = 1;
				}
				else if (!Good_obj(targ) && (targ != NOTHING))
				{
					pp->data[j] = NOTHING;
					dirty = 1;
				}
			}
		}

		if (dirty)
		{
			str = XMALLOC(LBUF_SIZE, "str");
			(void)propdir_rewrite(pp, str);
			atr_get_info(i, A_PROPDIR, &owner, &aflags);
			atr_add(i, A_PROPDIR, str, owner, aflags);
			XFREE(str);
		}

		/*
		 * Check owner
		 */
		owner = Owner(i);

		if (!Good_obj(owner))
		{
			Log_header_err(i, NOTHING, owner, 1, "Owner", "is invalid.  Set to GOD.");
			owner = GOD;
			s_Owner(i, owner);

			if (!mushstate.standalone)
			{
				halt_que(NOTHING, i);
			}

			s_Halted(i);
		}
		else if (check_type & DBCK_FULL)
		{
			if (Going(owner))
			{
				Log_header_err(i, NOTHING, owner, 1, "Owner", "is set GOING.  Set to GOD.");
				s_Owner(i, owner);

				if (!mushstate.standalone)
				{
					halt_que(NOTHING, i);
				}

				s_Halted(i);
			}
			else if (!OwnsOthers(owner))
			{
				Log_header_err(i, NOTHING, owner, 1, "Owner", "is not a valid owner type.");
			}
			else if (isPlayer(i) && (owner != i))
			{
				Log_header_err(i, NOTHING, owner, 1, "Player", "is the owner instead of the player.");
			}
		}

		if (check_type & DBCK_FULL)
		{
			/*
			 * Check for wizards
			 */
			if (Wizard(i))
			{
				if (isPlayer(i))
				{
					Log_simple_err(i, NOTHING, "Player is a WIZARD.");
				}

				if (!Wizard(Owner(i)))
				{
					Log_header_err(i, NOTHING, Owner(i), 1, "Owner", "of a WIZARD object is not a wizard");
				}
			}
		}
	}
}

#undef check_ref_targ

/* ---------------------------------------------------------------------------
 * check_loc_exits, check_exit_chains: Validate the exits chains
 * of objects and attempt to correct problems.  The following errors are
 * found and corrected:
 *      Location not in database                        - skip it.
 *      Location GOING                                  - skip it.
 *      Location not a PLAYER, ROOM, or THING           - skip it.
 *      Location already visited                        - skip it.
 *      Exit/next pointer not in database               - NULL it.
 *      Member is not an EXIT                           - terminate chain.
 *      Member is GOING                                 - destroy exit.
 *      Member already checked (is in another list)     - terminate chain.
 *      Member in another chain (recursive check)       - terminate chain.
 *      Location of member is not specified location    - reset it.
 */

void check_loc_exits(dbref loc)
{
	dbref exit, back, temp, exitloc, dest;

	if (!Good_obj(loc))
	{
		return;
	}

	/*
	 * Only check players, rooms, and things that aren't GOING
	 */

	if (isExit(loc) || Going(loc))
	{
		return;
	}

	/*
	 * If marked, we've checked here already
	 */

	if (Marked(loc))
	{
		return;
	}

	Mark(loc);
	/*
	 * Check all the exits
	 */
	back = NOTHING;
	exit = Exits(loc);

	while (exit != NOTHING)
	{
		exitloc = NOTHING;
		dest = NOTHING;

		if (Good_obj(exit))
		{
			exitloc = Exits(exit);
			dest = Location(exit);
		}

		if (!Good_obj(exit))
		{
			/*
			 * A bad pointer - terminate chain
			 */
			Log_pointer_err(back, loc, NOTHING, exit, "Exit list", "is invalid.  List nulled.");

			if (back != NOTHING)
			{
				s_Next(back, NOTHING);
			}
			else
			{
				s_Exits(loc, NOTHING);
			}

			exit = NOTHING;
		}
		else if (!isExit(exit))
		{
			/*
			 * Not an exit - terminate chain
			 */
			Log_pointer_err(back, loc, NOTHING, exit, "Exitlist member", "is not an exit.  List terminated.");

			if (back != NOTHING)
			{
				s_Next(back, NOTHING);
			}
			else
			{
				s_Exits(loc, NOTHING);
			}

			exit = NOTHING;
		}
		else if (Going(exit))
		{
			/*
			 * Going - silently filter out
			 */
			temp = Next(exit);

			if (back != NOTHING)
			{
				s_Next(back, temp);
			}
			else
			{
				s_Exits(loc, temp);
			}

			destroy_obj(NOTHING, exit);
			exit = temp;
			continue;
		}
		else if (Marked(exit))
		{
			/*
			 * Already in another list - terminate chain
			 */
			Log_pointer_err(back, loc, NOTHING, exit, "Exitlist member", "is in another exitlist.  Cleared.");

			if (back != NOTHING)
			{
				s_Next(back, NOTHING);
			}
			else
			{
				s_Exits(loc, NOTHING);
			}

			exit = NOTHING;
		}
		else if (!Good_obj(dest) && (dest != HOME) && (dest != AMBIGUOUS) && (dest != NOTHING))
		{
			/*
			 * Destination is not in the db.  Null it.
			 */
			Log_pointer_err(back, loc, NOTHING, exit, "Destination", "is invalid.  Cleared.");
			s_Location(exit, NOTHING);
		}
		else if (exitloc != loc)
		{
			/*
			 * Exit thinks it's in another place.  Check the
			 * * exitlist there and see if it contains this
			 * * exit. If it does, then our exitlist
			 * * somehow pointed into the middle of their
			 * * exitlist. If not, assume we own the exit.
			 */
			check_loc_exits(exitloc);

			if (Marked(exit))
			{
				/*
				 * It's in the other list, give it up
				 */
				Log_pointer_err(back, loc, NOTHING, exit, "", "is in another exitlist.  List terminated.");

				if (back != NOTHING)
				{
					s_Next(back, NOTHING);
				}
				else
				{
					s_Exits(loc, NOTHING);
				}

				exit = NOTHING;
			}
			else
			{
				/*
				 * Not in the other list, assume in ours
				 */
				Log_header_err(exit, loc, exitloc, 1, "Not on chain for location", "Reset.");
				s_Exits(exit, loc);
			}
		}

		if (exit != NOTHING)
		{
			/*
			 * All OK (or all was made OK)
			 */
			if (check_type & DBCK_FULL)
			{
				/*
				 * Make sure exit owner owns at least one of
				 * * the source or destination.  Just
				 * * warn if he doesn't.
				 */
				temp = Owner(exit);

				if ((temp != Owner(loc)) && (temp != Owner(Location(exit))))
				{
					Log_header_err(exit, loc, temp, 1, "Owner", "does not own either the source or destination.");
				}
			}

			Mark(exit);
			back = exit;
			temp = Next(exit);

			if (temp == exit)
			{
				Log_simple_err(exit, loc, "Next points to self in exit chain. Next cleared.");
				s_Next(exit, NOTHING);
				break;
			}

			exit = temp;
		}
	}

	return;
}

void check_exit_chains(void)
{
	for (dbref i = 0; i < ((mushstate.db_top + 7) >> 3); i++)
	{
		mushstate.markbits[i] = (char)0x0;
	}

	for (dbref i = 0; i < mushstate.db_top; i++)
	{
		check_loc_exits(i);
	}

	for (dbref i = 0; i < mushstate.db_top; i++)
	{
		if (isExit(i) && !Marked(i))
		{
			Log_simple_err(i, NOTHING, "Disconnected exit.  Destroyed.");
			destroy_obj(NOTHING, i);
		}
	}
}

/* ---------------------------------------------------------------------------
 * check_misplaced_obj, check_loc_contents, check_contents_chains: Validate
 * the contents chains of objects and attempt to correct problems.  The
 * following errors are found and corrected:
 *      Location not in database                        - skip it.
 *      Location GOING                                  - skip it.
 *      Location not a PLAYER, ROOM, or THING           - skip it.
 *      Location already visited                        - skip it.
 *      Contents/next pointer not in database           - NULL it.
 *      Member is not an PLAYER or THING                - terminate chain.
 *      Member is GOING                                 - destroy exit.
 *      Member already checked (is in another list)     - terminate chain.
 *      Member in another chain (recursive check)       - terminate chain.
 *      Location of member is not specified location    - reset it.
 */

void check_loc_contents(dbref);

void check_misplaced_obj(dbref *obj, dbref back, dbref loc)
{
	dbref claimed_loc;

	/*
	 * Object thinks it's in another place.  Check the contents list
	 * * there and see if it contains this object.  If it does, then
	 * * our contents list somehow pointed into the middle of their
	 * * contents list and we should truncate our list. If not,
	 * * assume we own the object.
	 */
	if (!Good_obj(*obj))
	{
		return;
	}

	claimed_loc = Location(*obj);
	Unmark(*obj);

	if (Good_obj(claimed_loc))
	{
		check_loc_contents(claimed_loc);
	}

	if (Marked(*obj))
	{
		/*
		 * It's in the other list, give it up
		 */
		Log_pointer_err(back, loc, NOTHING, *obj, "", "is in another contents list.  Cleared.");

		if (back != NOTHING)
		{
			s_Next(back, NOTHING);
		}
		else
		{
			s_Contents(loc, NOTHING);
		}

		*obj = NOTHING;
	}
	else
	{
		/*
		 * Not in the other list, assume in ours
		 */
		Log_header_err(*obj, claimed_loc, claimed_loc, 1, "Location", "is invalid.  Reset.");
		s_Location(*obj, loc);
	}

	return;
}

void check_loc_contents(dbref loc)
{
	dbref obj, back, temp;

	if (!Good_obj(loc))
	{
		return;
	}

	/*
	 * Only check players, rooms, and things that aren't GOING
	 */

	if (isExit(loc) || Going(loc))
	{
		return;
	}

	/*
	 * Check all the exits
	 */
	back = NOTHING;
	obj = Contents(loc);

	while (obj != NOTHING)
	{
		if (!Good_obj(obj))
		{
			/*
			 * A bad pointer - terminate chain
			 */
			Log_pointer_err(back, loc, NOTHING, obj, "Contents list", "is invalid.  Cleared.");

			if (back != NOTHING)
			{
				s_Next(back, NOTHING);
			}
			else
			{
				s_Contents(loc, NOTHING);
			}

			obj = NOTHING;
		}
		else if (!Has_location(obj))
		{
			/*
			 * Not a player or thing - terminate chain
			 */
			Log_pointer_err(back, loc, NOTHING, obj, "Contents list member", "is not a player or thing.  Cleared.");

			if (back != NOTHING)
			{
				s_Next(back, NOTHING);
			}
			else
			{
				s_Contents(loc, NOTHING);
			}

			obj = NOTHING;
		}
		else if (Going(obj) && (Typeof(obj) == TYPE_GARBAGE))
		{
			/*
			 * Going - silently filter out
			 */
			temp = Next(obj);

			if (back != NOTHING)
			{
				s_Next(back, temp);
			}
			else
			{
				s_Contents(loc, temp);
			}

			destroy_obj(NOTHING, obj);
			obj = temp;
			continue;
		}
		else if (Marked(obj))
		{
			/*
			 * Already visited - either truncate or ignore
			 */
			if (Location(obj) != loc)
			{
				/*
				 * Location wrong - either truncate or fix
				 */
				check_misplaced_obj(&obj, back, loc);
			}
			else
			{
				/*
				 * Location right - recursive contents
				 */
			}
		}
		else if (Location(obj) != loc)
		{
			/*
			 * Location wrong - either truncate or fix
			 */
			check_misplaced_obj(&obj, back, loc);
		}

		if (obj != NOTHING)
		{
			/*
			 * All OK (or all was made OK)
			 */
			if (check_type & DBCK_FULL)
			{
				/*
				 * Check for wizard command-handlers inside
				 * * nonwiz. Just warn if we find one.
				 */
				if (Wizard(obj) && !Wizard(loc))
				{
					if (Commer(obj))
					{
						Log_simple_err(obj, loc, "Wizard command handling object inside nonwizard.");
					}
				}

				/*
				 * Check for nonwizard objects inside wizard
				 * * objects.
				 */

				if (Wizard(loc) && !Wizard(obj) && !Wizard(Owner(obj)))
				{
					Log_simple_err(obj, loc, "Nonwizard object inside wizard.");
				}
			}

			Mark(obj);
			back = obj;
			temp = Next(obj);

			if (temp == obj)
			{
				Log_simple_err(obj, loc, "Next points to self in contents chain. Next cleared.");
				s_Next(obj, NOTHING);
				break;
			}

			obj = temp;
		}
	}

	return;
}

void check_contents_chains(void)
{
	for (dbref i = 0; i < ((mushstate.db_top + 7) >> 3); i++)
	{
		mushstate.markbits[i] = (char)0x0;
	}

	for (dbref i = 0; i < mushstate.db_top; i++)
	{
		check_loc_contents(i);
	}

	for (dbref i = 0; i < mushstate.db_top; i++)
	{
		if (!Going(i) && !Marked(i) && Has_location(i))
		{
			Log_simple_err(i, Location(i), "Orphaned object, moved home.");
			s_Location(i, NOTHING);
			s_Next(i, NOTHING);
			move_via_generic(i, HOME, NOTHING, 0);
		}
	}
}

/* ---------------------------------------------------------------------------
 * do_dbck: Perform a database consistency check and clean up damage.
 */

void do_dbck(dbref player, dbref cause, int key)
{
	check_type = key;
	make_freelist();

	if (!mushstate.standalone)
	{
		cf_verify();
	}

	check_dead_refs();
	check_exit_chains();
	check_contents_chains();
	purge_going();

	if (!mushstate.standalone && (player != NOTHING))
	{
		alarm(1);

		if (!Quiet(player))
		{
			notify(player, "Done");
		}
	}
}
