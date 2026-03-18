/**
 * @file funobj_speak.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object-focused built-ins: speech transformation and formatting
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
 * @brief Transform spoken text via u-functions and emit formatted speech/pose output
 *
 * @param speaker DBref of speaker
 * @param sname Speaker's name
 * @param str Text being said
 * @param key Say or pose key
 * @param say_str Default say string
 * @param trans_str Transform u-function name (optional)
 * @param empty_str Fallback text when transform returns empty
 * @param open_sep Opening delimiter for transform chunks
 * @param close_sep Closing delimiter for transform chunks
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
	char tstack_buf1[SBUF_SIZE], tstack_buf2[SBUF_SIZE];
	char estack_buf0[SBUF_SIZE], estack_buf1[SBUF_SIZE];
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
		XSAFELBSTR(save, buff, bufc);

		if (!sp)
		{
			XFREE(tbuf);
			return;
		}

		spos = 1;
	}

	tstack[1] = tstack_buf1;
	tstack[2] = tstack_buf2;

	if (empty_str && *empty_str)
	{
		empty_len = strlen(empty_str);
		estack[0] = estack_buf0;
		estack[1] = estack_buf1;
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
				XSAFESPRINTF(buff, bufc, "%s %s %s", sname, say_str, result);
			}
			else
			{
				XSAFELBSTR(result, buff, bufc);
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
				XSAFELBSTR(result, buff, bufc);
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
			XSAFELBSTR(save, buff, bufc);

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

	XFREE(trans_str);

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
	 * We have three possible cases for the speaker: &lt;thing string&gt;&amp;&lt;name
	 * string&gt; &amp;&lt;name string&gt; (speaker defaults to player) &lt;thing string&gt;
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
			XSAFENOMATCH(buff, bufc);
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
			XSAFENOMATCH(buff, bufc);
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
				XSAFESPRINTF(buff, bufc, "%s%s", tname, fargs[1] + 2);
			}
			else
			{
				XSAFESPRINTF(buff, bufc, "%s %s", tname, fargs[1] + 1);
			}

			break;

		case ';':
			XSAFESPRINTF(buff, bufc, "%s%s", tname, fargs[1] + 1);
			break;

		case '|':
			XSAFESPRINTF(buff, bufc, "%s", fargs[1] + 1);
			break;

		case '"':
			XSAFESPRINTF(buff, bufc, "%s %s \"%s\"", tname, say_str, fargs[1] + 1);
			break;

		default:
			XSAFESPRINTF(buff, bufc, "%s %s \"%s\"", tname, say_str, fargs[1]);
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
		XSAFELBSTR(tname, buff, bufc);

		if (*(fargs[1] + 1) != ' ')
		{
			XSAFELBCHR(' ', buff, bufc);
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
		XSAFELBSTR(tname, buff, bufc);
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
