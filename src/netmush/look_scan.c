/**
 * @file look_scan.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Debug examine and do_examine commands
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

void debug_examine(dbref player, dbref thing)
{
	dbref aowner;
	char *buf;
	int aflags, alen, ca;
	BOOLEXP *bexp;
	ATTR *attr;
	char *as, *cp, *nbuf;
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Number  = %d", thing);

	if (!Good_obj(thing))
	{
		return;
	}

	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Name    = %s", Name(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Location= %d", Location(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Contents= %d", Contents(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Exits   = %d", Exits(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Link    = %d", Link(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Next    = %d", Next(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owner   = %d", Owner(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Pennies = %d", Pennies(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Zone    = %d", Zone(thing));
	buf = flag_description(player, thing);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Flags   = %s", buf);
	XFREE(buf);
	buf = power_description(player, thing);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Powers  = %s", buf);
	XFREE(buf);
	buf = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
	bexp = boolexp_parse(player, buf, 1);
	XFREE(buf);
	buf = unparse_boolexp(player, bexp);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Lock    = %s", buf);
	XFREE(buf);
	boolexp_free(bexp);
	buf = XMALLOC(LBUF_SIZE, "buf");
	cp = buf;
	XSAFELBSTR((char *)"Attr list: ", buf, &cp);

	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as))
	{
		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		atr_get_info(thing, ca, &aowner, &aflags);

		if (Read_attr(player, thing, attr, aowner, aflags))
		{
			if (attr)
			{ /* Valid attr. */
				XSAFELBSTR((char *)attr->name, buf, &cp);
				XSAFELBCHR(' ', buf, &cp);
			}
			else
			{
				nbuf = ltos(ca);
				XSAFELBSTR(nbuf, buf, &cp);
				XFREE(nbuf);
				XSAFELBCHR(' ', buf, &cp);
			}
		}
	}

	notify(player, buf);
	XFREE(buf);

	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as))
	{
		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		buf = atr_get(thing, ca, &aowner, &aflags, &alen);

		if (Read_attr_all(player, thing, attr, aowner, aflags, 1))
			view_atr(player, thing, attr, buf, aowner, aflags, 0, 0);

		XFREE(buf);
	}
}

void exam_wildattrs(dbref player, dbref thing, int do_parent, int is_special)
{
	int atr = NOTHING, aflags = 0, alen = 0, got_any = 0;
	char *buf = NULL;
	dbref aowner = NOTHING;
	ATTR *ap = NULL;

	for (atr = olist_first(); atr != NOTHING; atr = olist_next())
	{
		ap = atr_num(atr);

		if (!ap)
		{
			continue;
		}

		if (do_parent && !(ap->flags & AF_PRIVATE))
		{
			buf = atr_pget(thing, atr, &aowner, &aflags, &alen);
		}
		else
		{
			buf = atr_get(thing, atr, &aowner, &aflags, &alen);
		}

		/*
		 * Decide if the player should see the attr:
		 * If obj is Examinable and has rights to see, yes.
		 * If a player and has rights to see, yes...
		 *   except if faraway, attr=DESC, and
		 *   remote DESC-reading is not turned on.
		 * If I own the attrib and have rights to see, yes...
		 *   except if faraway, attr=DESC, and
		 *   remote DESC-reading is not turned on.
		 */

		if (ap && Examinable(player, thing) && Read_attr_all(player, thing, ap, aowner, aflags, 1))
		{
			got_any = 1;
			view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
		}
		else if (ap && (Typeof(thing) == TYPE_PLAYER) && Read_attr_all(player, thing, ap, aowner, aflags, 1))
		{
			got_any = 1;

			if (aowner == Owner(player))
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else if ((atr == A_DESC) && (mushconf.read_rem_desc || nearby(player, thing)))
			{
				show_desc(player, thing, 0);
			}
			else if (atr != A_DESC)
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else
			{
				notify(player, "<Too far away to get a good look>");
			}
		}
		else if (ap && Read_attr_all(player, thing, ap, aowner, aflags, 1))
		{
			got_any = 1;

			if (aowner == Owner(player))
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else if ((atr == A_DESC) && (mushconf.read_rem_desc || nearby(player, thing)))
			{
				show_desc(player, thing, 0);
			}
			else if (nearby(player, thing))
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else
			{
				notify(player, "<Too far away to get a good look>");
			}
		}

		XFREE(buf);
	}

	if (!got_any)
	{
		notify_quiet(player, "No matching attributes found.");
	}
}

void do_examine(dbref player, dbref cause, int key, char *name)
{
	dbref thing = NOTHING, content = NOTHING, exit = NOTHING, aowner = NOTHING, loc = NOTHING;
	char savec = '\0';
	char *temp = NULL, *buf = NULL, *buf2 = NULL;
	char *timebuf = NULL;
	BOOLEXP *bexp = NULL;
	int control = 0, aflags = 0, alen = 0, do_parent = 0, is_special = 0;
	time_t save_access_time = 0;
	char created_str[26] = {0}, accessed_str[26] = {0}, modified_str[26] = {0};
	struct tm tm_created = {0}, tm_accessed = {0}, tm_modified = {0};

	/*
	 * This command is pointless if the player can't hear.
	 */

	if (!Hearer(player))
	{
		return;
	}

	timebuf = XMALLOC(32, "timebuf");
	if (!timebuf)
	{
		return;
	}

	do_parent = key & EXAM_PARENT;
	is_special = 0;

	if (key & EXAM_PRETTY)
	{
		is_special = 1;
	}

	if (key & EXAM_PAIRS)
	{
		is_special = 2;
	}

	thing = NOTHING;

	if (!name || !*name)
	{
		if ((thing = Location(player)) == NOTHING)
		{
			XFREE(timebuf);
			return;
		}
	}
	else
	{
		/*
		 * Check for obj/attr first.
		 */
		olist_push();

		if (parse_attrib_wild(player, name, &thing, do_parent, 1, 0, 1))
		{
			exam_wildattrs(player, thing, do_parent, is_special);
			olist_pop();
			XFREE(timebuf);
			return;
		}

		olist_pop();
		/*
		 * Look it up
		 */
		init_match(player, name, NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();

		if (!Good_obj(thing))
		{
			XFREE(timebuf);
			return;
		}
	}

	/*
	 * We have to save our access time, because the very act of
	 * * trying to examine the object will have touched it.
	 */
	save_access_time = AccessTime(thing);

	/*
	 * Check for the /debug switch
	 */

	if (key & EXAM_DEBUG)
	{
		if (!Examinable(player, thing))
		{
			notify_quiet(player, NOPERM_MESSAGE);
		}
		else
		{
			debug_examine(player, thing);
		}

		XFREE(timebuf);
		return;
	}

	control = (Examinable(player, thing) || Link_exit(player, thing));

	if (control && !(key & EXAM_OWNER))
	{
		buf2 = unparse_object(player, thing, 0);
		if (buf2)
		{
			notify(player, buf2);
			XFREE(buf2);
		}

		if (mushconf.ex_flags)
		{
			buf2 = flag_description(player, thing);
			if (buf2)
			{
				notify(player, buf2);
				XFREE(buf2);
			}
		}
	}
	else
	{
		if ((key & EXAM_OWNER) || ((key & EXAM_DEFAULT) && !mushconf.exam_public))
		{
			if (mushconf.read_rem_name)
			{
				buf2 = XMALLOC(LBUF_SIZE, "buf2");
				if (buf2)
				{
					XSTRCPY(buf2, Name(thing));
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s is owned by %s", buf2, Name(Owner(thing)));
					XFREE(buf2);
				}
			}
			else
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owned by %s", Name(Owner(thing)));
			}

			XFREE(timebuf);
			return;
		}
	}

	temp = XMALLOC(LBUF_SIZE, "temp");
	if (!temp)
	{
		XFREE(timebuf);
		return;
	}

	if (control || mushconf.read_rem_desc || nearby(player, thing))
	{
		temp = atr_get_str(temp, thing, A_DESC, &aowner, &aflags, &alen);

		if (temp && *temp)
		{
			if (Examinable(player, thing) || (aowner == Owner(player)))
			{
				view_atr(player, thing, atr_num(A_DESC), temp, aowner, aflags, 1, is_special);
			}
			else
			{
				show_desc(player, thing, 0);
			}
		}
	}
	else
	{
		notify(player, "<Too far away to get a good look>");
	}

	if (control)
	{
		/*
		 * print owner, key, and value
		 */
		savec = mushconf.many_coins[0];
		mushconf.many_coins[0] = (islower(savec) ? toupper(savec) : savec);
		buf2 = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
		// An empty lock or NULL lock still constitutes a valid lock (meaning the thing is unlocked)
		// The parser can handle these cases and correctly returns TRUE_BOOLEXP (aka NULL), thus no null check is required here.
		bexp = boolexp_parse(player, buf2, 1);
		// A NULL boolexp is a valid boolexp (meaning the thing has no lock); the unparser can safely handle those, thus no null check is required here.
		buf = unparse_boolexp(player, bexp);
		if (buf)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owner: %s  Key: %s %s: %d", Name(Owner(thing)), buf, mushconf.many_coins, Pennies(thing));
			XFREE(buf);
		}
		boolexp_free(bexp);
		XFREE(buf2);
		mushconf.many_coins[0] = savec;
		if (localtime_r(&CreateTime(thing), &tm_created) != NULL)
		{
			if (strftime(created_str, sizeof(created_str), "%a %b %d %H:%M:%S %Y", &tm_created) > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Created: %s", created_str);
			}
		}
		if (localtime_r(&save_access_time, &tm_accessed) != NULL)
		{
			if (strftime(accessed_str, sizeof(accessed_str), "%a %b %d %H:%M:%S %Y", &tm_accessed) > 0)
			{
				if (timebuf)
				{
					XSTRCPY(timebuf, accessed_str);
				}
			}
		}
		if (localtime_r(&ModTime(thing), &tm_modified) != NULL)
		{
			if (strftime(modified_str, sizeof(modified_str), "%a %b %d %H:%M:%S %Y", &tm_modified) > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Accessed: %s    Modified: %s", timebuf, modified_str);
			}
		}

		/*
		 * Print the zone
		 */

		if (mushconf.have_zones)
		{
			buf2 = unparse_object(player, Zone(thing), 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Zone: %s", buf2);
				XFREE(buf2);
			}
		}

		/*
		 * print parent
		 */
		loc = Parent(thing);

		if (loc != NOTHING)
		{
			buf2 = unparse_object(player, loc, 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Parent: %s", buf2);
				XFREE(buf2);
			}
		}

		/*
		 * Show the powers
		 */
		buf2 = power_description(player, thing);
		if (buf2)
		{
			notify(player, buf2);
			XFREE(buf2);
		}
	}

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->examine)
		{
			(*(cam__mp->examine))(player, cause, thing, control, key);
		}
	}

	if (!((key & EXAM_OWNER) || (key & EXAM_BRIEF)))
	{
		look_atrs(player, thing, do_parent, is_special);
	}

	/*
	 * show him interesting stuff
	 */

	if (control)
	{
		/*
		 * Contents
		 */
		if (Contents(thing) != NOTHING)
		{
			notify(player, "Contents:");
			for (content = Contents(thing); (content != NOTHING) && (Next(content) != content); content = Next(content))
			{
				buf2 = unparse_object(player, content, 0);
				if (buf2)
				{
					notify(player, buf2);
					XFREE(buf2);
				}
			}
		}

		/*
		 * Show stuff that depends on the object type
		 */

		switch (Typeof(thing))
		{
		case TYPE_ROOM:

			/*
			 * tell him about exits
			 */
			if (Exits(thing) != NOTHING)
			{
				notify(player, "Exits:");
				for (exit = Exits(thing); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
				{
					buf2 = unparse_object(player, exit, 0);
					if (buf2)
					{
						notify(player, buf2);
						XFREE(buf2);
					}
				}
			}
			else
			{
				notify(player, "No exits.");
			}

			/*
			 * print dropto if present
			 */

			if (Dropto(thing) != NOTHING)
			{
				buf2 = unparse_object(player, Dropto(thing), 0);
				if (buf2)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Dropped objects go to: %s", buf2);
					XFREE(buf2);
				}
			}

			break;

		case TYPE_THING:
		case TYPE_PLAYER:

			/*
			 * tell him about exits
			 */
			if (Exits(thing) != NOTHING)
			{
				notify(player, "Exits:");
				for (exit = Exits(thing); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
				{
					buf2 = unparse_object(player, exit, 0);
					if (buf2)
					{
						notify(player, buf2);
						XFREE(buf2);
					}
				}
			}
			else
			{
				notify(player, "No exits.");
			}

			/*
			 * print home
			 */
			loc = Home(thing);
			buf2 = unparse_object(player, loc, 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Home: %s", buf2);
				XFREE(buf2);
			}
			/*
			 * print location if player can link to it
			 */
			loc = Location(thing);

			if ((Location(thing) != NOTHING) && (Examinable(player, loc) || Examinable(player, thing) || Linkable(player, loc)))
			{
				buf2 = unparse_object(player, loc, 0);
				if (buf2)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Location: %s", buf2);
					XFREE(buf2);
				}
			}

			break;

		case TYPE_EXIT:
			buf2 = unparse_object(player, Exits(thing), 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Source: %s", buf2);
				XFREE(buf2);
			}

			/*
			 * print destination
			 */

			switch (Location(thing))
			{
			case NOTHING:
				/*
				 * Special case. unparse_object() normally
				 * * returns -1 as '*NOTHING*'.
				 */
				notify(player, "Destination: *UNLINKED*");
				break;

			default:
				buf2 = unparse_object(player, Location(thing), 0);
				if (buf2)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Destination: %s", buf2);
					XFREE(buf2);
				}
				break;
			}

			break;

		default:
			break;
		}
	}
	else if (!Opaque(thing) && nearby(player, thing))
	{
		if (Has_contents(thing))
		{
			look_contents(player, thing, "Contents:", CONTENTS_REMOTE);
		}

		if (!isExit(thing))
		{
			look_exits(player, thing, "Obvious exits:");
		}
	}

	XFREE(temp);

	if (!control)
	{
		if (mushconf.read_rem_name)
		{
			buf2 = XMALLOC(LBUF_SIZE, "buf2");
			if (buf2)
			{
				XSTRCPY(buf2, Name(thing));
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s is owned by %s", buf2, Name(Owner(thing)));
				XFREE(buf2);
			}
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owned by %s", Name(Owner(thing)));
		}
	}
	XFREE(timebuf);
}

