/**
 * @file object_utils.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object helper utilities for homes, naming checks, and new-object tracking
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
#include <stdlib.h>
#include <string.h>

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
			XFREE(buff);
			return 0;
		}
	}

	/*
	 * check last component
	 */

	while (isspace(*lastp))
	{
		lastp++;
	}

	if (strlen(lastp) < 1)
	{
		XFREE(buff);
		return 0;
	}

	XFREE(buff);
	return 1;
}
