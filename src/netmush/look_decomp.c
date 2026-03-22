/**
 * @file look_decomp.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object decomposition and VRML URL helpers (do_decomp, show_vrml_url)
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

void do_decomp(dbref player, dbref cause, int key, char *name, char *qual)
{
	BOOLEXP *bexp = NULL;
	char *got = NULL, *thingname = NULL, *as = NULL, *ltext = NULL, *buff = NULL, *tbuf = NULL, *tmp = NULL, *buf = NULL;
	dbref aowner = NOTHING, thing = NOTHING;
	int val = 0, aflags = 0, alen = 0, ca = 0, wild_decomp = 0;
	ATTR *attr = NULL;
	NAMETAB *np = NULL;
	/*
	 * Check for obj/attr first
	 */
	olist_push();

	if (parse_attrib_wild(player, name, &thing, 0, 1, 0, 1))
	{
		wild_decomp = 1;
	}
	else
	{
		wild_decomp = 0;
		init_match(player, name, TYPE_THING);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();
	}

	/*
	 * get result
	 */
	if (thing == NOTHING)
	{
		olist_pop();
		return;
	}

	if (!Examinable(player, thing))
	{
		notify_quiet(player, "You can only decompile things you can examine.");
		olist_pop();
		return;
	}

	thingname = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
	bexp = boolexp_parse(player, thingname, 1);

	/*
	 * Determine the name of the thing to use in reporting and then
	 * * report the command to make the thing.
	 */

	if (qual && *qual)
	{
		XSTRCPY(thingname, qual);
	}
	else
	{
		switch (Typeof(thing))
		{
		case TYPE_THING:
			XSTRCPY(thingname, Name(thing));
			val = OBJECT_DEPOSIT(Pennies(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@create %s=%d", buf, val);
			XFREE(buf);
			break;

		case TYPE_ROOM:
			XSTRCPY(thingname, Name(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@dig/teleport %s", buf);
			XFREE(buf);
			XSTRCPY(thingname, "here");
			break;

		case TYPE_EXIT:
			XSTRCPY(thingname, Name(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@open %s", buf);
			XFREE(buf);
			got = thingname;
			safe_exit_name(thing, thingname, &got);
			break;

		case TYPE_PLAYER:
			XSTRCPY(thingname, "me");
			break;
		}
	}

	buf = ansi_strip_ansi(thingname);
	XSTRCPY(thingname, buf);
	XFREE(buf);

	/*
	 * Report the lock (if any)
	 */

	if (!wild_decomp && (bexp != TRUE_BOOLEXP))
	{
		buf = unparse_boolexp_decompile(player, bexp);
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock %s=%s", thingname, buf);
		XFREE(buf);
	}

	boolexp_free(bexp);
	/*
	 * Report attributes
	 */
	buff = XMALLOC(MBUF_SIZE, "buff");

	for (ca = (wild_decomp ? olist_first() : atr_head(thing, &as)); (wild_decomp) ? (ca != NOTHING) : (ca != (int)0); ca = (wild_decomp ? olist_next() : atr_next(&as)))
	{
		if ((ca == A_NAME) || (ca == A_LOCK))
		{
			continue;
		}

		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		if ((attr->flags & AF_NOCMD) && !(attr->flags & AF_IS_LOCK))
		{
			continue;
		}

		got = atr_get(thing, ca, &aowner, &aflags, &alen);

		if (aflags & AF_STRUCTURE)
		{
			tmp = replace_string(GENERIC_STRUCT_STRDELIM, mushconf.struct_dstr, got);
			XFREE(got);
			got = tmp;
		}

		if (Read_attr_all(player, thing, attr, aowner, aflags, 1))
		{
			if (attr->flags & AF_IS_LOCK)
			{
				bexp = boolexp_parse(player, got, 1);
				ltext = unparse_boolexp_decompile(player, bexp);
				boolexp_free(bexp);
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock/%s %s=%s", attr->name, thingname, ltext);
				XFREE(ltext);
			}
			else
			{
				XSTRCPY(buff, attr->name);

				if (key & DECOMP_PRETTY)
				{
					tbuf = XMALLOC(LBUF_SIZE, "tbuf");
					buf = XMALLOC(GBUF_SIZE, "buf");
					if (tbuf && buf)
					{
						XSNPRINTF(buf, GBUF_SIZE, "%c%s %s=", ((ca < A_USER_START) ? '@' : '&'), buff, thingname);
						pretty_print(tbuf, buf, got);
						notify(player, tbuf);
					}
					if (tbuf)
						XFREE(tbuf);
					if (buf)
						XFREE(buf);
				}
				else
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%c%s %s=%s", ((ca < A_USER_START) ? '@' : '&'), buff, thingname, got);
				}

				for (np = indiv_attraccess_nametab; np->name; np++)
				{
					if ((aflags & np->flag) && check_access(player, np->perm) && (!(np->perm & CA_NO_DECOMP)))
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@set %s/%s = %s", thingname, buff, np->name);
					}
				}

				if (aflags & AF_LOCK)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock %s/%s", thingname, buff);
				}
			}
		}

		XFREE(got);
	}

	XFREE(buff);

	if (!wild_decomp)
	{
		decompile_flags(player, thing, thingname);
		decompile_powers(player, thing, thingname);
	}

	/*
	 * If the object has a parent, report it
	 */

	if (!wild_decomp && (Parent(thing) != NOTHING))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@parent %s=#%d", thingname, Parent(thing));
	}

	/*
	 * If the object has a zone, report it
	 */

	if (!wild_decomp && (Zone(thing) != NOTHING))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@chzone %s=#%d", thingname, Zone(thing));
	}

	XFREE(thingname);
	olist_pop();
}

void show_vrml_url(dbref thing, dbref loc)
{
	char *vrml_url = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;

	/*
	 * If they don't care about HTML, just return.
	 */
	if (!Html(thing))
	{
		return;
	}

	vrml_url = atr_pget(loc, A_VRML_URL, &aowner, &aflags, &alen);

	if (vrml_url && *vrml_url)
	{
		char *vrml_message = NULL, *vrml_cp = NULL;
		vrml_message = XMALLOC(LBUF_SIZE, "vrml_message");
		if (vrml_message)
		{
			vrml_cp = vrml_message;
			XSAFELBSTR("<img xch_graph=load href=\"", vrml_message, &vrml_cp);
			XSAFELBSTR(vrml_url, vrml_message, &vrml_cp);
			XSAFELBSTR("\">", vrml_message, &vrml_cp);
			*vrml_cp = 0;
			notify_html(thing, vrml_message);
			XFREE(vrml_message);
		}
	}
	else
	{
		notify_html(thing, "<img xch_graph=hide>");
	}

	if (vrml_url)
		XFREE(vrml_url);
}
