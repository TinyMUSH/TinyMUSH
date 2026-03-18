/**
 * @file funobj_flags.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object-focused built-ins: flags, powers, zones, and timestamps
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
 * @brief Returns the flags on an object. Because \@switch is case-insensitive,
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
void fun_flags(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it, aowner;
	int atr, aflags;
	char *buff2, *xbuf, *xbufp;

	if (parse_attrib(player, fargs[0], &it, &atr, 1))
	{
		if (atr == NOTHING)
		{
			XSAFENOTHING(buff, bufc);
		}
		else
		{
			atr_pget_info(it, atr, &aowner, &aflags);
			char xbuf[16];

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

			XSAFELBSTR(xbuf, buff, bufc);
		}
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (Good_obj(it) && (mushconf.pub_flags || Examinable(player, it) || (it == cause)))
		{
			buff2 = unparse_flags(player, it);
			XSAFELBSTR(buff2, buff, bufc);
			XFREE(buff2);
		}
		else
		{
			XSAFENOTHING(buff, bufc);
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
void handle_flaglists(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	char flagletter[2];
	FLAGSET fset;
	FLAG p_type;
	int negate, temp, type;
	dbref it = match_thing(player, fargs[0]);
	type = Is_Func(LOGIC_OR);
	negate = temp = 0;

	if (!Good_obj(it) || (!(mushconf.pub_flags || Examinable(player, it) || (it == cause))))
	{
		XSAFELBCHR('0', buff, bufc);
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
			XSAFELBCHR('0', buff, bufc);
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
				XSAFELBCHR('0', buff, bufc);
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
				XSAFEBOOL(buff, bufc, type);
				XFREE(flagletter);
				return;
			}

			/* Otherwise, move on to check the next flag. */
		}
	}

	XSAFEBOOL(buff, bufc, !type);
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
 * @brief   Returns true if object &lt;object&gt; has the flag named &lt;flag&gt; set on
 *          it, or, if &lt;flag&gt; is PLAYER, THING, ROOM, or EXIT, if &lt;object&gt; is
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
void fun_hasflag(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = NOTHING, aowner = NOTHING;
	int atr = 0, aflags = 0;
	ATTR *ap = NULL;

	if (parse_attrib(player, fargs[0], &it, &atr, 1))
	{
		if (atr == NOTHING)
		{
			XSAFELBSTR("#-1 NOT FOUND", buff, bufc);
		}
		else
		{
			ap = atr_num(atr);
			atr_pget_info(it, atr, &aowner, &aflags);
			XSAFEBOOL(buff, bufc, atr_has_flag(player, it, ap, aowner, aflags, fargs[1]));
		}
	}
	else
	{
		it = match_thing(player, fargs[0]);

		if (!Good_obj(it))
		{
			XSAFENOMATCH(buff, bufc);
			return;
		}

		int exam = Examinable(player, it);
		if (mushconf.pub_flags || exam || (it == cause))
		{
			XSAFEBOOL(buff, bufc, has_flag(player, it, fargs[1]));
		}
		else
		{
			XSAFENOPERM(buff, bufc);
		}
	}
}

/**
 * @brief Returns true if object &lt;object&gt; has the flag named &lt;flag&gt; set on
 *        it, or, if &lt;flag&gt; is PLAYER, THING, ROOM, or EXIT, if &lt;object&gt; is
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
void fun_haspower(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		XSAFENOMATCH(buff, bufc);
		return;
	}

	int exam = Examinable(player, it);
	if (mushconf.pub_flags || exam || (it == cause))
	{
		XSAFEBOOL(buff, bufc, has_power(player, it, fargs[1]));
	}
	else
	{
		XSAFENOPERM(buff, bufc);
	}
}

/**
 * @brief This function returns 1 if &lt;object&gt; has all the flags in
 *        &lt;flag list 1&gt;, or all the flags in &lt;flag list 2&gt;, and so
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
void fun_hasflags(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	char **elems;
	int n_elems, i, j, result;

	if (nfargs < 2)
	{
		XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (HASFLAGS) EXPECTS AT LEAST 2 ARGUMENTS BUT GOT %d", nfargs);
		return;
	}

	it = match_thing(player, fargs[0]);

	if (!Good_obj(it))
	{
		XSAFENOMATCH(buff, bufc);
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

	XSAFEBOOL(buff, bufc, result);
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
void handle_timestamp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	int exam = Good_obj(it) ? Examinable(player, it) : 0;
	if (!exam)
	{
		XSAFESTRNCAT(buff, bufc, "-1", 2, LBUF_SIZE);
	}
	else
	{
		XSAFELTOS(buff, bufc, Is_Func(TIMESTAMP_MOD) ? ModTime(it) : (Is_Func(TIMESTAMP_ACC) ? AccessTime(it) : CreateTime(it)), LBUF_SIZE);
	}
}

/**
 * @brief This function returns 1 if &lt;object&gt; has all the flags in
 *        &lt;flag list 1&gt;, or all the flags in &lt;flag list 2&gt;, and so
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
void fun_parent(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = match_thing(player, fargs[0]);

	int exam = Good_obj(it) ? Examinable(player, it) : 0;
	if (exam || (it == cause))
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, Parent(it), LBUF_SIZE);
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}

	return;
}

/**
 * @brief This function returns the list of dbrefs of the object's "parent
 *        chain", including itself: i.e., its own dbref, the dbref of the
 *        object it is \@parent'd to, its parent's parent (grandparent),
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
		XSAFENOMATCH(buff, bufc);
		return;
	}
	int exam = Examinable(player, it);
	if (!exam)
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, it, LBUF_SIZE);

	par = Parent(it);
	i = 1;

	while (Good_obj(par) && exam && (i < mushconf.parent_nest_lim))
	{
		print_separator(&osep, buff, bufc);
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, par, LBUF_SIZE);
		it = par;
		exam = Examinable(player, it);
		par = Parent(par);
		i++;
	}
}

/**
 * @brief This function returns a list of objects that are parented to &lt;object&gt;.
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
			XSAFENOMATCH(buff, bufc);
			return;
		}
	}

	if (!Controls(player, it) && !See_All(player))
	{
		XSAFENOPERM(buff, bufc);
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

			XSAFELBCHR('#', buff, bufc);
			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
		}
	}
}

/**
 * @brief Returns the dbref of &lt;object&gt;'s zone (the dbref of the master object
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
void fun_zone(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it = NOTHING;

	if (!mushconf.have_zones)
	{
		XSAFELBSTR("#-1 ZONES DISABLED", buff, bufc);
		return;
	}

	it = match_thing(player, fargs[0]);

	int exam = Good_obj(it) ? Examinable(player, it) : 0;
	if (!exam)
	{
		XSAFENOTHING(buff, bufc);
		return;
	}

	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, Zone(it), LBUF_SIZE);
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
void scan_zone(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref i = NOTHING, it = NOTHING;
	char *bb_p = NULL;
	int type = Func_Mask(TYPE_MASK);

	if (!mushconf.have_zones)
	{
		XSAFELBSTR("#-1 ZONES DISABLED", buff, bufc);
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
			XSAFENOMATCH(buff, bufc);
			return;
		}
	}

	if (!Controls(player, it) && !WizRoy(player))
	{
		XSAFENOPERM(buff, bufc);
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
					XSAFELBCHR(' ', buff, bufc);
				}

				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, i, LBUF_SIZE);
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
void fun_zfun(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;
	ATTR *ap = NULL;
	char *tbuf1 = NULL, *str = NULL;
	dbref zone = Zone(player);

	if (!mushconf.have_zones)
	{
		XSAFELBSTR("#-1 ZONES DISABLED", buff, bufc);
		return;
	}

	if (zone == NOTHING)
	{
		XSAFELBSTR("#-1 INVALID ZONE", buff, bufc);
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
		XSAFELBSTR("#-1 NO SUCH USER FUNCTION", buff, bufc);
		return;
	}

	tbuf1 = atr_pget(zone, ap->number, &aowner, &aflags, &alen);

	if (!See_attr(player, zone, ap, aowner, aflags))
	{
		XSAFELBSTR("#-1 NO PERMISSION TO GET ATTRIBUTE", buff, bufc);
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
