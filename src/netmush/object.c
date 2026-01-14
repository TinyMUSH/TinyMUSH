/**
 * @file object.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object lifecycle helpers: creation, ownership, locks, attributes, and linkage
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
 * can_set_home, new_home, clone_home:
 * Routines for validating and determining homes.
 */

bool can_set_home(dbref player, dbref thing, dbref home)
{
	if (!Good_obj(player) || !Good_obj(home) || (thing == home))
	{
		return false;
	}

	switch (Typeof(home))
	{
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_THING:
		if (Going(home))
		{
			return false;
		}

		if (Controls(player, home) || Abode(home) || LinkAnyHome(player))
		{
			return true;
		}
	}

	return false;
}

dbref new_home(dbref player)
{
	dbref loc;
	loc = Location(player);

	if (can_set_home(Owner(player), player, loc))
	{
		return loc;
	}

	loc = Home(Owner(player));

	if (can_set_home(Owner(player), player, loc))
	{
		return loc;
	}

	return (Good_home(mushconf.default_home) ? mushconf.default_home : (Good_home(mushconf.start_home) ? mushconf.start_home : (Good_home(mushconf.start_room) ? mushconf.start_room : 0)));
}

dbref clone_home(dbref player, dbref thing)
{
	dbref loc;
	loc = Home(thing);

	if (can_set_home(Owner(player), player, loc))
	{
		return loc;
	}

	return new_home(player);
}

/* ---------------------------------------------------------------------------
 * update_newobjs: Update a player's most-recently-created objects.
 */

void update_newobjs(dbref player, dbref obj_num, int obj_type)
{
	int i, aowner, aflags, alen;
	char *newobj_str, *p, *tbuf, *tokst;
	int obj_list[4];
	newobj_str = atr_get(player, A_NEWOBJS, &aowner, &aflags, &alen);

	if (!*newobj_str)
	{
		for (i = 0; i < 4; i++)
		{
			obj_list[i] = -1;
		}
	}
	else
	{
		for (p = strtok_r(newobj_str, " ", &tokst), i = 0; p && (i < 4); p = strtok_r(NULL, " ", &tokst), i++)
		{
			obj_list[i] = (int)strtol(p, (char **)NULL, 10);
		}
	}

	XFREE(newobj_str);

	switch (obj_type)
	{
	case TYPE_ROOM:
		obj_list[0] = obj_num;
		break;

	case TYPE_EXIT:
		obj_list[1] = obj_num;
		break;

	case TYPE_THING:
		obj_list[2] = obj_num;
		break;

	case TYPE_PLAYER:
		obj_list[3] = obj_num;
		break;
	}

	tbuf = XASPRINTF("tbuf", "%d %d %d %d", obj_list[0], obj_list[1], obj_list[2], obj_list[3]);
	atr_add_raw(player, A_NEWOBJS, tbuf);
	XFREE(tbuf);
}

/* ---------------------------------------------------------------------------
 * ok_exit_name: Make sure an exit name contains no blank components.
 */

int ok_exit_name(char *name)
{
	char *p, *lastp, *s;
	char *buff = XSTRDUP(name, "buff"); /* munchable buffer */

	/*
	 * walk down the string, checking lengths. skip leading spaces.
	 */

	for (p = buff, lastp = buff; (p = strchr(lastp, ';')) != NULL; lastp = p)
	{
		*p++ = '\0';
		s = lastp;

		while (isspace(*s))
		{
			s++;
		}

		if (strlen(s) < 1)
		{
			return 0;
		}
	}

	XFREE(buff);

	/*
	 * check last component
	 */

	while (isspace(*lastp))
	{
		lastp++;
	}

	if (strlen(lastp) < 1)
	{
		return 0;
	}

	return 1;
}

/* ---------------------------------------------------------------------------
 * create_obj: Create an object of the indicated type IF the player can
 * afford it.
 */

dbref create_obj(dbref player, int objtype, char *name, int cost)
{
	dbref obj, owner;
	dbref parent = NOTHING;
	dbref proto = NOTHING;
	int quota, okname = 0, value, self_owned, require_inherit;
	FLAG f1, f2, f3;
	time_t tt;
	char *buff;
	const char *tname;
	struct timeval obj_time;

	/*
	 * First check to see whether or not we're allowed to grow the
	 * * database any further (we must either have an object in the
	 * * freelist, or we have to be under the limit).
	 */

	if ((mushstate.db_top + 1 >= mushconf.building_limit) && (mushstate.freelist == NOTHING))
	{
		notify(player, "The database building limit has been reached.");
		return NOTHING;
	}

	value = 0;
	quota = 0;
	self_owned = 0;
	require_inherit = 0;

	switch (objtype)
	{
	case TYPE_ROOM:
		cost = mushconf.digcost;
		quota = mushconf.room_quota;
		f1 = mushconf.room_flags.word1;
		f2 = mushconf.room_flags.word2;
		f3 = mushconf.room_flags.word3;
		okname = ok_name(name);
		tname = "a room";

		if (Good_obj(mushconf.room_parent))
		{
			parent = mushconf.room_parent;
		}

		if (Good_obj(mushconf.room_proto))
		{
			proto = mushconf.room_proto;
		}

		break;

	case TYPE_THING:
		if (cost < mushconf.createmin)
		{
			cost = mushconf.createmin;
		}

		if (cost > mushconf.createmax)
		{
			cost = mushconf.createmax;
		}

		quota = mushconf.thing_quota;
		f1 = mushconf.thing_flags.word1;
		f2 = mushconf.thing_flags.word2;
		f3 = mushconf.thing_flags.word3;
		value = OBJECT_ENDOWMENT(cost);
		okname = ok_name(name);
		tname = "a thing";

		if (Good_obj(mushconf.thing_parent))
		{
			parent = mushconf.thing_parent;
		}

		if (Good_obj(mushconf.thing_proto))
		{
			proto = mushconf.thing_proto;
		}

		break;

	case TYPE_EXIT:
		cost = mushconf.opencost;
		quota = mushconf.exit_quota;
		f1 = mushconf.exit_flags.word1;
		f2 = mushconf.exit_flags.word2;
		f3 = mushconf.exit_flags.word3;
		okname = ok_name(name) && ok_exit_name(name);
		tname = "an exit";

		if (Good_obj(mushconf.exit_parent))
		{
			parent = mushconf.exit_parent;
		}

		if (Good_obj(mushconf.exit_proto))
		{
			proto = mushconf.exit_proto;
		}

		break;

	case TYPE_PLAYER:
		if (cost)
		{
			cost = mushconf.robotcost;
			quota = mushconf.player_quota;
			f1 = mushconf.robot_flags.word1;
			f2 = mushconf.robot_flags.word2;
			f3 = mushconf.robot_flags.word3;
			value = 0;
			tname = "a robot";
			require_inherit = 1;
		}
		else
		{
			cost = 0;
			quota = 0;
			f1 = mushconf.player_flags.word1;
			f2 = mushconf.player_flags.word2;
			f3 = mushconf.player_flags.word3;
			value = mushconf.paystart;
			quota = mushconf.start_quota;
			self_owned = 1;
			tname = "a player";
		}

		if (Good_obj(mushconf.player_parent))
		{
			parent = mushconf.player_parent;
		}

		if (Good_obj(mushconf.player_proto))
		{
			proto = mushconf.player_proto;
		}

		buff = munge_space(name);

		if (!badname_check(buff))
		{
			notify(player, "That name is not allowed.");
			XFREE(buff);
			return NOTHING;
		}

		if (!ok_player_name(buff))
		{
			XFREE(buff);
			break;
		}

		if (lookup_player(NOTHING, buff, 0) != NOTHING)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "The name %s is already taken.", name);
			XFREE(buff);
			return NOTHING;
		}

		++okname;
		XFREE(buff);
		break;

	default:
		log_write(LOG_BUGS, "BUG", "OTYPE", "Bad object type in create_obj: %d.", objtype);
		return NOTHING;
	}

	if (!okname)
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "That's a silly name for %s!", tname);
		return NOTHING;
	}

	if (!self_owned)
	{
		if (!Good_obj(player))
		{
			return NOTHING;
		}

		owner = Owner(player);

		if (!Good_obj(owner))
		{
			return NOTHING;
		}
	}
	else
	{
		owner = NOTHING;
	}

	if (require_inherit)
	{
		if (!Inherits(player))
		{
			notify(player, NOPERM_MESSAGE);
			return NOTHING;
		}
	}

	/*
	 * Make sure the creator can pay for the object.
	 */

	if ((player != NOTHING) && !canpayfees(player, player, cost, quota, objtype))
	{
		return NOTHING;
	}
	else if (player != NOTHING)
	{
		payfees(player, cost, quota, objtype);
	}

	/*
	 * Get the first object from the freelist.  If the object is not
	 * * clean, discard the remainder of the freelist and go get a
	 * * completely new object.
	 */
	obj = NOTHING;

	if (mushstate.freelist != NOTHING)
	{
		obj = mushstate.freelist;

		if (Good_dbref(obj) && IS_CLEAN(obj))
		{
			mushstate.freelist = Link(obj);
		}
		else
		{
			log_write(LOG_PROBLEMS, "FRL", "DAMAG", "Freelist damaged, bad object #%d.", obj);
			obj = NOTHING;
			mushstate.freelist = NOTHING;
		}
	}

	if (obj == NOTHING)
	{
		obj = mushstate.db_top;
		db_grow(mushstate.db_top + 1);
	}

	atr_free(obj); /* just in case... */
	/*
	 * Set things up according to the object type
	 */
	s_Location(obj, NOTHING);
	s_Contents(obj, NOTHING);
	s_Exits(obj, NOTHING);
	s_Next(obj, NOTHING);
	s_Link(obj, NOTHING);

	/*
	 * We do not autozone players to their creators.
	 */

	if (mushconf.autozone && (player != NOTHING) && (objtype != TYPE_PLAYER))
	{
		s_Zone(obj, Zone(player));
	}
	else
	{
		if (proto != NOTHING)
		{
			s_Zone(obj, Zone(proto));
		}
		else
		{
			s_Zone(obj, NOTHING);
		}
	}

	if (proto != NOTHING)
	{
		s_Parent(obj, Parent(proto));
		s_Flags(obj, objtype | (Flags(proto) & ~TYPE_MASK));
		s_Flags2(obj, Flags2(proto));
		s_Flags3(obj, Flags3(proto));
	}
	else
	{
		s_Parent(obj, parent);
		s_Flags(obj, objtype | f1);
		s_Flags2(obj, f2);
		s_Flags3(obj, f3);
	}

	s_Owner(obj, (self_owned ? obj : owner));
	s_Pennies(obj, value);
	Unmark(obj);
	buff = munge_space((char *)name);
	s_Name(obj, buff);
	XFREE(buff);

	if (mushconf.lag_check_clk)
	{
		obj_time.tv_sec = obj_time.tv_usec = 0;
		db[obj].cpu_time_used.tv_sec = obj_time.tv_sec;
		db[obj].cpu_time_used.tv_usec = obj_time.tv_usec;
	}

	s_Created(obj);
	s_Accessed(obj);
	s_Modified(obj);
	s_StackCount(obj, 0);
	s_VarsCount(obj, 0);
	s_StructCount(obj, 0);
	s_InstanceCount(obj, 0);

	if (proto != NOTHING)
	{
		atr_cpy(GOD, obj, proto);
	}

	if (objtype == TYPE_PLAYER)
	{
		time(&tt);
		char timebuf[26];
		struct tm tm_buf;
		localtime_r(&tt, &tm_buf);
		strftime(timebuf, sizeof(timebuf), "%a %b %d %H:%M:%S %Y", &tm_buf);
		atr_add_raw(obj, A_LAST, timebuf);
		buff = XMALLOC(SBUF_SIZE, "buff");
		XSPRINTF(buff, "%d %d %d %d %d", quota, mushconf.start_room_quota, mushconf.start_exit_quota, mushconf.start_thing_quota, mushconf.start_player_quota);
		atr_add_raw(obj, A_QUOTA, buff);
		atr_add_raw(obj, A_RQUOTA, buff);
		add_player_name(obj, Name(obj));
		XFREE(buff);

		if (!cost)
		{
			payfees(obj, 0, mushconf.player_quota, TYPE_PLAYER);
		}
	}

	if (player != NOTHING)
	{
		update_newobjs(player, obj, objtype);
	}

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->create_obj)
		{
			(*(cam__mp->create_obj))(player, obj);
		}
	}

	return obj;
}

/* ---------------------------------------------------------------------------
 * destroy_obj: Destroy an object.  Assumes it has already been removed from
 * all lists and has no contents or exits.
 */

void destroy_obj(dbref player, dbref obj)
{
	dbref owner;
	int good_owner, val, quota;
	char *tname;

	if (!Good_obj(obj))
	{
		return;
	}

	/*
	 * Validate the owner
	 */
	owner = Owner(obj);
	good_owner = Good_owner(owner);

	/*
	 * Halt any pending commands (waiting or semaphore)
	 */

	if (halt_que(NOTHING, obj) > 0)
	{
		if (good_owner && !Quiet(obj) && !Quiet(owner))
		{
			notify(owner, "Halted.");
		}
	}

	nfy_que(GOD, obj, 0, NFY_DRAIN, 0);
	cron_clr(obj, NOTHING);
	/*
	 * Remove forwardlists, stacks, etc. from the hash tables.
	 */
	fwdlist_clr(obj);
	propdir_clr(obj);
	stack_clr(obj);
	xvars_clr(obj);
	structure_clr(obj);

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->destroy_obj)
		{
			(*(cam__mp->destroy_obj))(player, obj);
		}
	}

	/*
	 * Compensate the owner for the object
	 */
	val = 1;
	quota = 1;

	if (good_owner && (owner != obj))
	{
		switch (Typeof(obj))
		{
		case TYPE_ROOM:
			val = mushconf.digcost;
			quota = mushconf.room_quota;
			break;

		case TYPE_THING:
			val = OBJECT_DEPOSIT(Pennies(obj));
			quota = mushconf.thing_quota;
			break;

		case TYPE_EXIT:
			val = mushconf.opencost;
			quota = mushconf.exit_quota;
			break;

		case TYPE_PLAYER:
			if (Robot(obj))
			{
				val = mushconf.robotcost;
			}
			else
			{
				val = 0;
			}

			quota = mushconf.player_quota;
		}

		payfees(owner, -val, -quota, Typeof(obj));

		if (!Quiet(owner) && !Quiet(obj))
		{
			notify_check(owner, owner, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You get back your %d %s deposit for %s(#%d).", val, mushconf.one_coin, Name(obj), obj);
		}
	}

	if ((player != NOTHING) && !Quiet(player))
	{
		if (good_owner && Owner(player) != owner)
		{
			if (owner == obj)
			{
				notify_check(owner, owner, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Destroyed. %s(#%d)", Name(obj), obj);
			}
			else
			{
				tname = XMALLOC(SBUF_SIZE, "tname");
				XSTRCPY(tname, Name(owner));
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Destroyed. %s's %s(#%d)", tname, Name(obj), obj);
				XFREE(tname);
			}
		}
		else if (!Quiet(obj))
		{
			notify(player, "Destroyed.");
		}
	}

	atr_free(obj);
	s_Name(obj, NULL);
	s_Flags(obj, (TYPE_GARBAGE | GOING));
	s_Flags2(obj, 0);
	s_Flags3(obj, 0);
	s_Powers(obj, 0);
	s_Powers2(obj, 0);
	s_Location(obj, NOTHING);
	s_Contents(obj, NOTHING);
	s_Exits(obj, NOTHING);
	s_Next(obj, NOTHING);
	s_Link(obj, NOTHING);
	s_Owner(obj, GOD);
	s_Pennies(obj, 0);
	s_Parent(obj, NOTHING);
	s_Zone(obj, NOTHING);
}

/* ---------------------------------------------------------------------------
 * do_freelist: Grab a garbage object, and move it to the top of the freelist.
 */

void do_freelist(dbref player, dbref cause, int key, char *str)
{
	dbref i, thing;

	/*
	 * We can only take a dbref; don't bother calling match_absolute() even,
	 * * since we're dealing with the garbage pile anyway.
	 */
	if (*str != '#')
	{
		notify(player, NOMATCH_MESSAGE);
		return;
	}

	str++;

	if (!*str)
	{
		notify(player, NOMATCH_MESSAGE);
		return;
	}

	thing = (int)strtol(str, (char **)NULL, 10);

	if (!Good_dbref(thing))
	{
		notify(player, NOMATCH_MESSAGE);
		return;
	}

	/*
	 * The freelist is a linked list going from the lowest-numbered
	 * * objects to the highest-numbered objects. We need to make sure an
	 * * object is clean before we muck with it.
	 */

	if (IS_CLEAN(thing))
	{
		if (mushstate.freelist == thing)
		{
			notify(player, "That object is already at the head of the freelist.");
			return;
		}

		/*
		 * We've got to find this thing's predecessor so we avoid
		 * * circular chaining.
		 */
		for (i = 0; i < mushstate.db_top; i++)
		{
			if (Link(i) == thing)
			{
				if (IS_CLEAN(i))
				{
					s_Link(i, Link(thing));
					break; /* shouldn't have more than one linkage */
				}
				else
				{
					notify(player, "Unable to relink freelist at this time.");
					return;
				}
			}
		}
		s_Link(thing, mushstate.freelist);
		mushstate.freelist = thing;
		notify(player, "Object placed at the head of the freelist.");
	}
	else
	{
		notify(player, "That object is not clean garbage.");
	}
}

/* ---------------------------------------------------------------------------
 * make_freelist: Build a freelist
 */

void make_freelist(void)
{
	mushstate.freelist = NOTHING;

	for (dbref i = mushstate.db_top - 1; i >= 0; i--)
	{
		if (IS_CLEAN(i))
		{
			/*
			 * If there's clean garbage at the end of the db, just
			 * * trim it off. Memory will be reused if new objects are
			 * * needed, but can be eliminated by restarting.
			 */
			mushstate.db_top--;
		}
		else
		{
			break;
		}
	}

	for (dbref i = mushstate.db_top - 1; i >= 0; i--)
	{
		if (IS_CLEAN(i))
		{
			s_Link(i, mushstate.freelist);
			mushstate.freelist = i;
		}
	}
}

/* ---------------------------------------------------------------------------
 * divest_object: Get rid of KEY contents of object.
 */

void divest_object(dbref thing)
{
	dbref curr, temp;

	for (curr = Contents(thing); curr != NOTHING; curr = temp)
	{
		temp = Next(curr);

		if (temp == curr)
		{
			break;
		}

		if (!Controls(thing, curr) && Has_location(curr) && Key(curr))
		{
			move_via_generic(curr, HOME, NOTHING, 0);
		}
	}
}

/* ---------------------------------------------------------------------------
 * empty_obj, purge_going: Get rid of GOING objects in the db.
 */

void empty_obj(dbref obj)
{
	dbref targ, next;
	/*
	 * Send the contents home
	 */
	for (targ = Contents(obj); targ != NOTHING; targ = next)
	{
		next = Next(targ);

		if (next == targ)
		{
			break;
		}

		if (!Has_location(targ))
		{
			Log_simple_err(targ, obj, "Funny object type in contents list of GOING location. Flush terminated.");
			break;
		}
		else if (Location(targ) != obj)
		{
			Log_header_err(targ, obj, Location(targ), 1, "Location", "indicates object really in another location during cleanup of GOING location.  Flush terminated.");
			break;
		}
		else
		{
			s_Location(targ, NOTHING);
			s_Next(targ, NOTHING);

			if (Home(targ) == obj)
			{
				s_Home(targ, new_home(targ));
			}

			move_via_generic(targ, HOME, NOTHING, 0);
			divest_object(targ);
		}
	}
	/*
	 * Destroy the exits
	 */
	for (targ = Exits(obj); targ != NOTHING; targ = next)
	{
		next = Next(targ);

		if (next == targ)
		{
			break;
		}

		if (!isExit(targ))
		{
			Log_simple_err(targ, obj, "Funny object type in exit list of GOING location. Flush terminated.");
			break;
		}
		else if (Exits(targ) != obj)
		{
			Log_header_err(targ, obj, Exits(targ), 1, "Location", "indicates exit really in another location during cleanup of GOING location.  Flush terminated.");
			break;
		}
		else
		{
			destroy_obj(NOTHING, targ);
		}
	}
}

/* ---------------------------------------------------------------------------
 * destroy_exit, destroy_thing, destroy_player
 */

void destroy_exit(dbref exit)
{
	dbref loc;
	loc = Exits(exit);
	s_Exits(loc, remove_first(Exits(loc), exit));
	destroy_obj(NOTHING, exit);
}

void destroy_thing(dbref thing)
{
	move_via_generic(thing, NOTHING, Owner(thing), 0);
	empty_obj(thing);
	destroy_obj(NOTHING, thing);
}

void destroy_player(dbref victim)
{
	dbref aowner, player;
	int count, aflags, alen;
	char *buf, *a_dest, *cpa__p, *cpa__tokp;
	/*
	 * Bye bye...
	 */
	a_dest = atr_get_raw(victim, A_DESTROYER);

	if (a_dest)
	{
		player = (int)strtol(a_dest, (char **)NULL, 10);
		XFREE(a_dest);

		if (!Good_owner(player))
		{
			player = GOD;
		}
	}
	else
	{
		player = GOD;
	}

	boot_off(victim, (char *)"You have been destroyed!");
	halt_que(victim, NOTHING);
	count = chown_all(victim, player, player, 0);
	/*
	 * Remove the name from the name hash table
	 */
	delete_player_name(victim, Name(victim));
	buf = atr_pget(victim, A_ALIAS, &aowner, &aflags, &alen);

	for (cpa__p = strtok_r((buf), ";", &cpa__tokp); cpa__p; cpa__p = strtok_r(NULL, ";", &cpa__tokp))
	{
		delete_player_name((victim), cpa__p);
	}

	XFREE(buf);
	move_via_generic(victim, NOTHING, player, 0);

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->destroy_player)
		{
			(*(cam__mp->destroy_player))(player, victim);
		}
	}

	destroy_obj(NOTHING, victim);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "(%d objects @chowned to you)", count);
}

void purge_going(void)
{
	dbref i;
	for (i = 0; i < mushstate.db_top; i++)
	{
		if (!Going(i))
		{
			continue;
		}

		switch (Typeof(i))
		{
		case TYPE_PLAYER:
			destroy_player(i);
			break;

		case TYPE_ROOM:
			/*
			 * Room scheduled for destruction... do it
			 */
			empty_obj(i);
			destroy_obj(NOTHING, i);
			break;

		case TYPE_THING:
			destroy_thing(i);
			break;

		case TYPE_EXIT:
			destroy_exit(i);
			break;

		case TYPE_GARBAGE:
			break;

		default:
			/*
			 * Something else... How did this happen?
			 */
			Log_simple_err(i, NOTHING, "GOING object with unexpected type.  Destroyed.");
			destroy_obj(NOTHING, i);
		}
	}
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
