/* funstring.c - string functions */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"	/* required by mudconf */
#include "mushconf.h"	/* required by code */
#include "db.h"			/* required by externs */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "powers.h"		/* required by code */
#include "ansi.h"		/* required by code */
#include "stringutil.h" /* required by code */

/*
 * ---------------------------------------------------------------------------
 * isword: is every character in the argument a letter? isalnum: is every
 * character in the argument a letter or number?
 */

void fun_isword(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p;

	for (p = fargs[0]; *p; p++)
	{
		if (!isalpha(*p))
		{
			safe_chr('0', buff, bufc);
			return;
		}
	}

	safe_chr('1', buff, bufc);
}

void fun_isalnum(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p;

	for (p = fargs[0]; *p; p++)
	{
		if (!isalnum(*p))
		{
			safe_chr('0', buff, bufc);
			return;
		}
	}

	safe_chr('1', buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * isnum: is the argument a number?
 */

void fun_isnum(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	safe_chr((is_number(fargs[0]) ? '1' : '0'), buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * isdbref: is the argument a valid dbref?
 */

void fun_isdbref(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p;
	dbref dbitem;
	p = fargs[0];

	if (*p++ == NUMBER_TOKEN)
	{
		if (*p)
		{ /* just the string '#' won't do! */
			dbitem = parse_dbref_only(p);

			if (Good_obj(dbitem))
			{
				safe_chr('1', buff, bufc);
				return;
			}
		}
	}

	safe_chr('0', buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * isobjid: is the argument a valid objid?
 */

void fun_isobjid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p;
	dbref dbitem;
	p = fargs[0];

	if (*p++ == NUMBER_TOKEN)
	{
		if (*p)
		{ /* just the string '#' won't do! */
			dbitem = parse_objid(p, NULL);

			if (Good_obj(dbitem))
			{
				safe_chr('1', buff, bufc);
				return;
			}
		}
	}

	safe_chr('0', buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_null: Just eat the contents of the string. Handy for those times when
 * you've output a bunch of junk in a function call and just want to dispose
 * of the output (like if you've done an iter() that just did a bunch of
 * side-effects, and now you have bunches of spaces you need to get rid of.
 */

void fun_null(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
}

/*
 * ---------------------------------------------------------------------------
 * fun_squish: Squash occurrences of a given character down to 1. We do this
 * both on leading and trailing chars, as well as internal ones; if the
 * player wants to trim off the leading and trailing as well, they can always
 * call trim().
 */

void fun_squish(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tp, *bp;
	Delim isep;

	if (nfargs == 0)
	{
		return;
	}

	VaChk_Only_InPure(2);
	bp = tp = fargs[0];

	while (*tp)
	{
		/*
	 * Move over and copy the non-sep characters
	 */
		while (*tp && *tp != isep.str[0])
		{
			if (*tp == ESC_CHAR)
			{
				copy_esccode(&tp, &bp);
			}
			else
			{
				*bp++ = *tp++;
			}
		}

		/*
	 * If we've reached the end of the string, leave the loop.
	 */

		if (!*tp)
		{
			break;
		}

		/*
	 * Otherwise, we've hit a sep char. Move over it, and then
	 * move on to the next non-separator. Note that we're
	 * overwriting our own string as we do this. However, the
	 * other pointer will always be ahead of our current copy
	 * pointer.
	 */
		*bp++ = *tp++;

		while (*tp && (*tp == isep.str[0]))
		{
			tp++;
		}
	}

	/*
     * Must terminate the string
     */
	*bp = '\0';
	safe_str(fargs[0], buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * trim: trim off unwanted white space.
 */
#define TRIM_L 0x1
#define TRIM_R 0x2

void fun_trim(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p, *q, *endchar, *ep;
	Delim isep;
	int trim;

	if (nfargs == 0)
	{
		return;
	}

	VaChk_In(1, 3);

	if (nfargs >= 2)
	{
		switch (tolower(*fargs[1]))
		{
		case 'l':
			trim = TRIM_L;
			break;

		case 'r':
			trim = TRIM_R;
			break;

		default:
			trim = TRIM_L | TRIM_R;
			break;
		}
	}
	else
	{
		trim = TRIM_L | TRIM_R;
	}

	p = fargs[0];

	/*
     * Single-character delimiters are easy.
     */

	if (isep.len == 1)
	{
		if (trim & TRIM_L)
		{
			while (*p == isep.str[0])
			{
				p++;
			}
		}

		if (trim & TRIM_R)
		{
			q = endchar = p;

			while (*q != '\0')
			{
				if (*q == ESC_CHAR)
				{
					skip_esccode(&q);
					endchar = q;
				}
				else if (*q++ != isep.str[0])
				{
					endchar = q;
				}
			}

			*endchar = '\0';
		}

		safe_str(p, buff, bufc);
		return;
	}

	/*
     * Multi-character delimiters take more work.
     */
	ep = p + strlen(fargs[0]) - 1; /* last char in string */

	if (trim & TRIM_L)
	{
		while (!strncmp(p, isep.str, isep.len) && (p <= ep))
		{
			p = p + isep.len;
		}

		if (p > ep)
		{
			return;
		}
	}

	if (trim & TRIM_R)
	{
		q = endchar = p;

		while (q <= ep)
		{
			if (*q == ESC_CHAR)
			{
				skip_esccode(&q);
				endchar = q;
			}
			else if (!strncmp(q, isep.str, isep.len))
			{
				q = q + isep.len;
			}
			else
			{
				q++;
				endchar = q;
			}
		}

		*endchar = '\0';
	}

	safe_str(p, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_after, fun_before: Return substring after or before a specified
 * string.
 */

void fun_after(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bp, *cp, *mp, *np, *buf;
	int ansi_needle, ansi_needle2, ansi_haystack, ansi_haystack2;

	if (nfargs == 0)
	{
		return;
	}

	VaChk_Range(1, 2);
	bp = fargs[0]; /* haystack */
	mp = fargs[1]; /* needle */

	/*
     * Sanity-check arg1 and arg2
     */

	if (bp == NULL)
	{
		bp = "";
	}

	if (mp == NULL)
	{
		mp = " ";
	}

	if (!mp || !*mp)
	{
		mp = (char *)" ";
	}

	if ((mp[0] == ' ') && (mp[1] == '\0'))
	{
		bp = Eat_Spaces(bp);
	}

	/*
     * Get ansi state of the first needle char
     */
	ansi_needle = ANST_NONE;

	while (*mp == ESC_CHAR)
	{
		track_esccode(&mp, &ansi_needle);

		if (!*mp)
		{
			mp = (char *)" ";
		}
	}

	ansi_haystack = ANST_NORMAL;

	/*
     * Look for the needle string
     */

	while (*bp)
	{
		while (*bp == ESC_CHAR)
		{
			track_esccode(&bp, &ansi_haystack);
		}

		if ((*bp == *mp) && (ansi_needle == ANST_NONE || ansi_haystack == ansi_needle))
		{
			/*
	     * See if what follows is what we are looking for
	     */
			ansi_needle2 = ansi_needle;
			ansi_haystack2 = ansi_haystack;
			cp = bp;
			np = mp;

			while (1)
			{
				while (*cp == ESC_CHAR)
				{
					track_esccode(&cp, &ansi_haystack2);
				}

				while (*np == ESC_CHAR)
				{
					track_esccode(&np, &ansi_needle2);
				}

				if ((*cp != *np) || (ansi_needle2 != ANST_NONE && ansi_haystack2 != ansi_needle2) || !*cp || !*np)
				{
					break;
				}

				++cp, ++np;
			}

			if (!*np)
			{
				/*
		 * Yup, return what follows
		 */
				buf = ansi_transition_esccode(ANST_NORMAL, ansi_haystack2);
				safe_str(buf, buff, bufc);
				XFREE(buf);
				safe_str(cp, buff, bufc);
				return;
			}
		}

		/*
	 * Nope, continue searching
	 */
		if (*bp)
		{
			++bp;
		}
	}

	/*
     * Ran off the end without finding it
     */
	return;
}

void fun_before(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *haystack, *bp, *cp, *mp, *np, *buf;
	int ansi_needle, ansi_needle2, ansi_haystack, ansi_haystack2;

	if (nfargs == 0)
	{
		return;
	}

	VaChk_Range(1, 2);
	haystack = fargs[0]; /* haystack */
	mp = fargs[1];		 /* needle */

	/*
     * Sanity-check arg1 and arg2
     */

	if (haystack == NULL)
	{
		haystack = "";
	}

	if (mp == NULL)
	{
		mp = " ";
	}

	if (!mp || !*mp)
	{
		mp = (char *)" ";
	}

	if ((mp[0] == ' ') && (mp[1] == '\0'))
	{
		haystack = Eat_Spaces(haystack);
	}

	bp = haystack;
	/*
     * Get ansi state of the first needle char
     */
	ansi_needle = ANST_NONE;

	while (*mp == ESC_CHAR)
	{
		track_esccode(&mp, &ansi_needle);

		if (!*mp)
		{
			mp = (char *)" ";
		}
	}

	ansi_haystack = ANST_NORMAL;

	/*
     * Look for the needle string
     */

	while (*bp)
	{
		/*
	 * See if what follows is what we are looking for
	 */
		ansi_needle2 = ansi_needle;
		ansi_haystack2 = ansi_haystack;
		cp = bp;
		np = mp;

		while (1)
		{
			while (*cp == ESC_CHAR)
			{
				track_esccode(&cp, &ansi_haystack2);
			}

			while (*np == ESC_CHAR)
			{
				track_esccode(&np, &ansi_needle2);
			}

			if ((*cp != *np) || (ansi_needle2 != ANST_NONE && ansi_haystack2 != ansi_needle2) || !*cp || !*np)
			{
				break;
			}

			++cp, ++np;
		}

		if (!*np)
		{
			/*
	     * Yup, return what came before this
	     */
			*bp = '\0';
			safe_str(haystack, buff, bufc);
			buf = ansi_transition_esccode(ansi_haystack, ANST_NORMAL);
			safe_str(buf, buff, bufc);
			XFREE(buf);
			return;
		}

		/*
	 * Nope, continue searching
	 */
		while (*bp == ESC_CHAR)
		{
			track_esccode(&bp, &ansi_haystack);
		}

		if (*bp)
		{
			++bp;
		}
	}

	/*
     * Ran off the end without finding it
     */
	safe_str(haystack, buff, bufc);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

void fun_lcstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *ap;
	ap = *bufc;
	safe_str(fargs[0], buff, bufc);

	while (*ap)
	{
		if (*ap == ESC_CHAR)
		{
			skip_esccode(&ap);
		}
		else
		{
			*ap = tolower(*ap);
			ap++;
		}
	}
}

void fun_ucstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *ap;
	ap = *bufc;
	safe_str(fargs[0], buff, bufc);

	while (*ap)
	{
		if (*ap == ESC_CHAR)
		{
			skip_esccode(&ap);
		}
		else
		{
			*ap = toupper(*ap);
			ap++;
		}
	}
}

void fun_capstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *ap;
	ap = *bufc;
	safe_str(fargs[0], buff, bufc);

	while (*ap == ESC_CHAR)
	{
		skip_esccode(&ap);
	}

	*ap = toupper(*ap);
}

/*
 * ---------------------------------------------------------------------------
 * fun_space: Make spaces.
 */

void fun_space(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int num, max;

	if (!fargs[0] || !(*fargs[0]))
	{
		num = 1;
	}
	else
	{
		num = (int)strtol(fargs[0], (char **)NULL, 10);
	}

	if (num < 1)
	{
		/*
	 * If negative or zero spaces return a single space, -except-
	 * allow 'space(0)' to return "" for calculated padding
	 */
		if (!is_integer(fargs[0]) || (num != 0))
		{
			num = 1;
		}
	}

	max = LBUF_SIZE - 1 - (*bufc - buff);
	num = (num > max) ? max : num;
	memset(*bufc, ' ', num);
	*bufc += num;
	**bufc = '\0';
}

/*
 * ---------------------------------------------------------------------------
 * rjust, ljust, center: Justify or center text, specifying fill character
 */

void fun_ljust(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int spaces, max, i, slen;
	char *tp, *fillchars;
	VaChk_Range(2, 3);
	spaces = (int)strtol(fargs[1], (char **)NULL, 10) - strip_ansi_len(fargs[0]);
	safe_str(fargs[0], buff, bufc);

	/*
     * Sanitize number of spaces
     */
	if (spaces <= 0)
	{
		return; /* no padding needed, just return string */
	}

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	spaces = (spaces > max) ? max : spaces;

	if (fargs[2])
	{
		fillchars = strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > spaces) ? spaces : slen;

		if (slen == 0)
		{
			/*
	     * NULL character fill
	     */
			memset(tp, ' ', spaces);
			tp += spaces;
		}
		else if (slen == 1)
		{
			/*
	     * single character fill
	     */
			memset(tp, *fillchars, spaces);
			tp += spaces;
		}
		else
		{
			/*
	     * multi character fill
	     */
			for (i = spaces; i >= slen; i -= slen)
			{
				memcpy(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/*
		 * we have a remainder here
		 */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}

		XFREE(fillchars);
	}
	else
	{
		/*
	 * no fill character specified
	 */
		memset(tp, ' ', spaces);
		tp += spaces;
	}

	*tp = '\0';
	*bufc = tp;
}

void fun_rjust(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int spaces, max, i, slen;
	char *tp, *fillchars;
	VaChk_Range(2, 3);
	spaces = (int)strtol(fargs[1], (char **)NULL, 10) - strip_ansi_len(fargs[0]);

	/*
     * Sanitize number of spaces
     */

	if (spaces <= 0)
	{
		/*
	 * no padding needed, just return string
	 */
		safe_str(fargs[0], buff, bufc);
		return;
	}

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	spaces = (spaces > max) ? max : spaces;

	if (fargs[2])
	{
		fillchars = strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > spaces) ? spaces : slen;

		if (slen == 0)
		{
			/*
	     * NULL character fill
	     */
			memset(tp, ' ', spaces);
			tp += spaces;
		}
		else if (slen == 1)
		{
			/*
	     * single character fill
	     */
			memset(tp, *fillchars, spaces);
			tp += spaces;
		}
		else
		{
			/*
	     * multi character fill
	     */
			for (i = spaces; i >= slen; i -= slen)
			{
				memcpy(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/*
		 * we have a remainder here
		 */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}

		XFREE(fillchars);
	}
	else
	{
		/*
	 * no fill character specified
	 */
		memset(tp, ' ', spaces);
		tp += spaces;
	}

	*bufc = tp;
	safe_str(fargs[0], buff, bufc);
}

void fun_center(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tp, *fillchars;
	int len, lead_chrs, trail_chrs, width, max, i, slen;
	VaChk_Range(2, 3);
	width = (int)strtol(fargs[1], (char **)NULL, 10);
	len = strip_ansi_len(fargs[0]);
	width = (width > LBUF_SIZE - 1) ? LBUF_SIZE - 1 : width;

	if (len >= width)
	{
		safe_str(fargs[0], buff, bufc);
		return;
	}

	lead_chrs = (int)((width / 2) - (len / 2) + .5);
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /* chars left in buffer */
	lead_chrs = (lead_chrs > max) ? max : lead_chrs;

	if (fargs[2])
	{
		fillchars = (char *)strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > lead_chrs) ? lead_chrs : slen;

		if (slen == 0)
		{
			/*
	     * NULL character fill
	     */
			memset(tp, ' ', lead_chrs);
			tp += lead_chrs;
		}
		else if (slen == 1)
		{
			/*
	     * single character fill
	     */
			memset(tp, *fillchars, lead_chrs);
			tp += lead_chrs;
		}
		else
		{
			/*
	     * multi character fill
	     */
			for (i = lead_chrs; i >= slen; i -= slen)
			{
				memcpy(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/*
		 * we have a remainder here
		 */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}

		XFREE(fillchars);
	}
	else
	{
		/*
	 * no fill character specified
	 */
		memset(tp, ' ', lead_chrs);
		tp += lead_chrs;
	}

	*bufc = tp;
	safe_str(fargs[0], buff, bufc);
	trail_chrs = width - lead_chrs - len;
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff);
	trail_chrs = (trail_chrs > max) ? max : trail_chrs;

	if (fargs[2])
	{
		if (slen == 0)
		{
			/*
	     * NULL character fill
	     */
			memset(tp, ' ', trail_chrs);
			tp += trail_chrs;
		}
		else if (slen == 1)
		{
			/*
	     * single character fill
	     */
			memset(tp, *fillchars, trail_chrs);
			tp += trail_chrs;
		}
		else
		{
			/*
	     * multi character fill
	     */
			for (i = trail_chrs; i >= slen; i -= slen)
			{
				memcpy(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/*
		 * we have a remainder here
		 */
				memcpy(tp, fillchars, i);
				tp += i;
			}
		}
	}
	else
	{
		/*
	 * no fill character specified
	 */
		memset(tp, ' ', trail_chrs);
		tp += trail_chrs;
	}

	*tp = '\0';
	*bufc = tp;
}

/*
 * ---------------------------------------------------------------------------
 * fun_left: Returns first n characters in a string fun_right: Returns last n
 * characters in a string strtrunc: now an alias for left
 */

void fun_left(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	int count, nchars;
	int ansi_state = ANST_NORMAL;
	s = fargs[0];
	nchars = (int)strtol(fargs[1], (char **)NULL, 10);

	if (nchars <= 0)
	{
		return;
	}

	for (count = 0; (count < nchars) && *s; count++)
	{
		while (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state);
		}

		if (*s)
		{
			++s;
		}
	}

	safe_strncat(buff, bufc, fargs[0], s - fargs[0], LBUF_SIZE);
	s = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	safe_str(s, buff, bufc);
	XFREE(s);
}

void fun_right(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *buf;
	int count, start, nchars;
	int ansi_state = ANST_NORMAL;
	s = fargs[0];
	nchars = (int)strtol(fargs[1], (char **)NULL, 10);
	start = strip_ansi_len(s) - nchars;

	if (nchars <= 0)
	{
		return;
	}

	if (start < 0)
	{
		nchars += start;

		if (nchars <= 0)
		{
			return;
		}

		start = 0;
	}

	while (*s == ESC_CHAR)
	{
		track_esccode(&s, &ansi_state);
	}

	for (count = 0; (count < start) && *s; count++)
	{
		++s;

		while (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state);
		}
	}

	if (*s)
	{
		buf = ansi_transition_esccode(ANST_NORMAL, ansi_state);
		safe_str(buf, buff, bufc);
		XFREE(buf);
	}

	safe_str(s, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_chomp: If the line ends with CRLF, CR, or LF, chop it off.
 */

void fun_chomp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p = *bufc;
	safe_str(fargs[0], buff, bufc);

	if (*bufc != bb_p && (*bufc)[-1] == '\n')
	{
		(*bufc)--;
	}

	if (*bufc != bb_p && (*bufc)[-1] == '\r')
	{
		(*bufc)--;
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_comp: exact-string compare. fun_streq: non-case-sensitive string
 * compare. fun_strmatch: wildcard string compare.
 */

void fun_comp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int x;
	x = strcmp(fargs[0], fargs[1]);

	if (x > 0)
	{
		safe_chr('1', buff, bufc);
	}
	else if (x < 0)
	{
		safe_str("-1", buff, bufc);
	}
	else
	{
		safe_chr('0', buff, bufc);
	}
}

void fun_streq(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	safe_bool(buff, bufc, !string_compare(fargs[0], fargs[1]));
}

void fun_strmatch(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * Check if we match the whole string.  If so, return 1
     */
	safe_bool(buff, bufc, quick_wild(fargs[1], fargs[0]));
}

/*
 * ---------------------------------------------------------------------------
 * fun_edit: Edit text.
 */

void fun_edit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tstr;
	edit_string(fargs[0], &tstr, fargs[1], fargs[2]);
	safe_str(tstr, buff, bufc);
	XFREE(tstr);
}

/*
 * ---------------------------------------------------------------------------
 * fun_merge:  given two strings and a character, merge the two strings by
 * replacing characters in string1 that are the same as the given character
 * by the corresponding character in string2 (by position). The strings must
 * be of the same length.
 */

void fun_merge(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str, *rep;
	char c;

	/*
     * do length checks first
     */

	if (strlen(fargs[0]) != strlen(fargs[1]))
	{
		safe_str("#-1 STRING LENGTHS MUST BE EQUAL", buff, bufc);
		return;
	}

	if (strlen(fargs[2]) > 1)
	{
		safe_str("#-1 TOO MANY CHARACTERS", buff, bufc);
		return;
	}

	/*
     * find the character to look for. null character is considered a
     * space
     */

	if (!*fargs[2])
	{
		c = ' ';
	}
	else
	{
		c = *fargs[2];
	}

	/*
     * walk strings, copy from the appropriate string
     */

	for (str = fargs[0], rep = fargs[1]; *str && *rep && ((*bufc - buff) < (LBUF_SIZE - 1)); str++, rep++, (*bufc)++)
	{
		if (*str == c)
		{
			**bufc = *rep;
		}
		else
		{
			**bufc = *str;
		}
	}

	return;
}

/*
 * ---------------------------------------------------------------------------
 * fun_secure, fun_escape: escape [, ], %, \, and the beginning of the
 * string. fun_esc: more limited escape, intended for instances where you
 * only care about string evaluation, not a forced command.
 */

void fun_secure(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	s = fargs[0];

	while (*s)
	{
		switch (*s)
		{
		case ESC_CHAR:
			safe_copy_esccode(&s, buff, bufc);
			continue;

		case '%':
		case '$':
		case '\\':
		case '[':
		case ']':
		case '(':
		case ')':
		case '{':
		case '}':
		case ',':
		case ';':
			safe_chr(' ', buff, bufc);
			break;

		default:
			safe_chr(*s, buff, bufc);
		}

		++s;
	}
}

void fun_escape(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *d;
	s = fargs[0];

	if (!*s)
	{
		return;
	}

	safe_chr('\\', buff, bufc);
	d = *bufc;

	while (*s)
	{
		switch (*s)
		{
		case ESC_CHAR:
			safe_copy_esccode(&s, buff, bufc);
			continue;

		case '%':
		case '\\':
		case '[':
		case ']':
		case '{':
		case '}':
		case ';':
			if (*bufc != d)
			{
				safe_chr('\\', buff, bufc);
			}

			/*
	     * FALLTHRU
	     */
		default:
			safe_chr(*s, buff, bufc);
		}

		++s;
	}
}

void fun_esc(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	s = fargs[0];

	if (!*s)
	{
		return;
	}

	while (*s)
	{
		switch (*s)
		{
		case ESC_CHAR:
			safe_copy_esccode(&s, buff, bufc);
			continue;

		case '%':
		case '\\':
		case '[':
		case ']':
			safe_chr('\\', buff, bufc);

			/*
	     * FALLTHRU
	     */
		default:
			safe_chr(*s, buff, bufc);
		}

		++s;
	}
}

/*
 * ---------------------------------------------------------------------------
 * stripchars: Remove all of a set of characters from a string.
 */

void fun_stripchars(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	char stab[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	Delim osep;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	/*
     * Output delimiter should default to null, not a space
     */
	VaChk_Only_Out(3);

	if (nfargs < 3)
	{
		osep.str[0] = '\0';
	}

	for (s = fargs[1]; *s; s++)
	{
		stab[(unsigned char)*s] = 1;
	}

	for (s = fargs[0]; *s; s++)
	{
		if (stab[(unsigned char)*s] == 0)
		{
			safe_chr(*s, buff, bufc);
		}
		else if (nfargs > 2)
		{
			print_sep(&osep, buff, bufc);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * ANSI handlers.
 */

void fun_ansi(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	int ansi_state;
	int xterm = 0;

	if (!mudconf.ansi_colors)
	{
		safe_str(fargs[1], buff, bufc);
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		safe_str(fargs[1], buff, bufc);
		return;
	}

	track_ansi_letters(fargs[0], &ansi_state);
	s = ansi_transition_esccode(ANST_NONE, ansi_state);
	safe_str(s, buff, bufc);
	XFREE(s);
	/* Now that normal ansi has been done, time for xterm */
	s = fargs[0];

	while (*s)
	{
		if (*s == '<' || *s == '/')
		{ /* Xterm colors */
			int xterm_isbg = 0, i;
			char xtbuf[SBUF_SIZE], *xtp;

			if (*s == '/')
			{ /* We are dealing with background */
				s++;

				if (!*s)
				{
					break;
				}
				else
				{
					xterm_isbg = 1;
				}
			}
			else
			{
				xterm_isbg = 0; /* We are dealing with foreground */
			}

			if (*s == '<')
			{ /* Ok we got a color to process */
				s++;

				if (!*s)
				{
					break;
				}

				xtp = xtbuf;

				while (*s && (*s != '>'))
				{
					safe_sb_chr(*s, xtbuf, &xtp);
					s++;
				}

				if (*s != '>')
				{
					break;
				}

				*xtp = '\0';
				/* Now we have the color string... Time to handle it */
				i = str2xterm(xtbuf);

				if (xterm_isbg)
				{
					snprintf(xtbuf, SBUF_SIZE, "%s%d%c", ANSI_XTERM_BG, i, ANSI_END);
				}
				else
				{
					snprintf(xtbuf, SBUF_SIZE, "%s%d%c", ANSI_XTERM_FG, i, ANSI_END);
				}

				safe_str(xtbuf, buff, bufc);
				xterm = 1;
			}
			else
			{
				break;
			}

			/* Shall we continue? */
			s++;

			if (*s != '<' && *s != '/')
			{
				break;
			}
		}
		else
		{
			s++;
		}
	}

	s = fargs[1];

	while (*s)
	{
		if (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state);
		}
		else
		{
			++s;
		}
	}

	safe_str(fargs[1], buff, bufc);
	s = ansi_transition_esccode(ansi_state, ANST_NONE);
	safe_str(s, buff, bufc);
	XFREE(s);

	if (xterm)
	{
		safe_ansi_normal(buff, bufc);
	}
}

void fun_stripansi(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *buf;
	buf = strip_ansi(fargs[0]);
	safe_str(buf, buff, bufc);
	XFREE(buf);
}

/*---------------------------------------------------------------------------
 * encrypt() and decrypt(): From DarkZone.
 */

/*
 * Copy over only alphanumeric chars
 */
#define CRYPTCODE_LO 32	 /* space */
#define CRYPTCODE_HI 126 /* tilde */
#define CRYPTCODE_MOD 95 /* count of printable ascii chars */

void crunch_code(char *code)
{
	char *in, *out;
	in = out = code;

	while (*in)
	{
		if ((*in >= CRYPTCODE_LO) && (*in <= CRYPTCODE_HI))
		{
			*out++ = *in++;
		}
		else if (*in == ESC_CHAR)
		{
			skip_esccode(&in);
		}
		else
		{
			++in;
		}
	}

	*out = '\0';
}

void crypt_code(char *buff, char **bufc, char *code, char *text, int type)
{
	char *p, *q;

	if (!*text)
	{
		return;
	}

	crunch_code(code);

	if (!*code)
	{
		safe_str(text, buff, bufc);
		return;
	}

	q = code;
	p = *bufc;
	safe_str(text, buff, bufc);

	/*
     * Encryption: Simply go through each character of the text, get its
     * ascii value, subtract LO, add the ascii value (less LO) of the
     * code, mod the result, add LO. Continue
     */
	while (*p)
	{
		if ((*p >= CRYPTCODE_LO) && (*p <= CRYPTCODE_HI))
		{
			if (type)
			{
				*p = (((*p - CRYPTCODE_LO) + (*q - CRYPTCODE_LO)) % CRYPTCODE_MOD) + CRYPTCODE_LO;
			}
			else
			{
				*p = (((*p - *q) + 2 * CRYPTCODE_MOD) % CRYPTCODE_MOD) + CRYPTCODE_LO;
			}

			++p, ++q;

			if (!*q)
			{
				q = code;
			}
		}
		else if (*p == ESC_CHAR)
		{
			skip_esccode(&p);
		}
		else
		{
			++p;
		}
	}
}

void fun_encrypt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 1);
}

void fun_decrypt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 0);
}

/*
 * ---------------------------------------------------------------------------
 * fun_scramble:  randomizes the letters in a string.
 */

/* Borrowed from PennMUSH 1.50 */
void fun_scramble(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int n, i, j, ansi_state, *ansi_map;
	char *stripped, *buf;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	n = ansi_map_states(fargs[0], &ansi_map, &stripped);
	ansi_state = ANST_NORMAL;

	for (i = 0; i < n; i++)
	{
		j = random_range(i, n - 1);

		if (ansi_state != ansi_map[j])
		{
			buf = ansi_transition_esccode(ansi_state, ansi_map[j]);
			safe_str(buf, buff, bufc);
			XFREE(buf);
			ansi_state = ansi_map[j];
		}

		safe_chr(stripped[j], buff, bufc);
		ansi_map[j] = ansi_map[i];
		stripped[j] = stripped[i];
	}

	buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	safe_str(buf, buff, bufc);
	XFREE(buf);
	XFREE(ansi_map);
	XFREE(stripped);
}

/*
 * ---------------------------------------------------------------------------
 * fun_reverse: reverse a string
 */

void fun_reverse(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int n, *ansi_map, ansi_state;
	char *stripped, *buf;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	n = ansi_map_states(fargs[0], &ansi_map, &stripped);
	ansi_state = ansi_map[n];

	while (n--)
	{
		if (ansi_state != ansi_map[n])
		{
			buf = ansi_transition_esccode(ansi_state, ansi_map[n]);
			safe_str(buf, buff, bufc);
			XFREE(buf);
			ansi_state = ansi_map[n];
		}

		safe_chr(stripped[n], buff, bufc);
	}

	buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	safe_str(buf, buff, bufc);
	XFREE(buf);
	XFREE(ansi_map);
	XFREE(stripped);
}

/*
 * ---------------------------------------------------------------------------
 * fun_mid: mid(foobar,2,3) returns oba
 */

void fun_mid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *savep, *buf;
	int count, start, nchars;
	int ansi_state = ANST_NORMAL;
	s = fargs[0];
	start = (int)strtol(fargs[1], (char **)NULL, 10);
	nchars = (int)strtol(fargs[2], (char **)NULL, 10);

	if (nchars <= 0)
	{
		return;
	}

	if (start < 0)
	{
		nchars += start;

		if (nchars <= 0)
		{
			return;
		}

		start = 0;
	}

	while (*s == ESC_CHAR)
	{
		track_esccode(&s, &ansi_state);
	}

	for (count = 0; (count < start) && *s; ++count)
	{
		++s;

		while (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state);
		}
	}

	if (*s)
	{
		buf = ansi_transition_esccode(ANST_NORMAL, ansi_state);
		safe_str(buf, buff, bufc);
		XFREE(buf);
	}

	savep = s;

	for (count = 0; (count < nchars) && *s; ++count)
	{
		while (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state);
		}

		if (*s)
		{
			++s;
		}
	}

	safe_strncat(buff, bufc, savep, s - savep, LBUF_SIZE);
	buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	safe_str(buf, buff, bufc);
	XFREE(buf);
}

/*
 * ---------------------------------------------------------------------------
 * fun_translate: Takes a string and a second argument. If the second
 * argument is 0 or s, control characters are converted to spaces. If it's 1
 * or p, they're converted to percent substitutions.
 */

void fun_translate(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s;
	VaChk_Range(1, 2);

	/*
     * Strictly speaking, we're just checking the first char
     */

	if (nfargs > 1 && (fargs[1][0] == 's' || fargs[1][0] == '0'))
	{
		s = translate_string(fargs[0], 0);
		safe_str(s, buff, bufc);
		XFREE(s);
	}
	else if (nfargs > 1 && fargs[1][0] == 'p')
	{
		s = translate_string(fargs[0], 1);
		safe_str(s, buff, bufc);
		XFREE(s);
	}
	else
	{
		s = translate_string(fargs[0], 1);
		safe_str(s, buff, bufc);
		XFREE(s);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_pos: Find a word in a string
 */

void fun_pos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i = 1;
	char *b, *b1, *s, *s1, *t, *u;
	i = 1;
	b1 = b = strip_ansi(fargs[0]);
	s1 = s = strip_ansi(fargs[1]);

	if (*b && !*(b + 1))
	{ /* single character */
		t = strchr(s, *b);

		if (t)
		{
			safe_ltos(buff, bufc, (int)(t - s + 1), LBUF_SIZE);
		}
		else
		{
			safe_nothing(buff, bufc);
		}

		XFREE(s1);
		XFREE(b1);
		return;
	}

	while (*s)
	{
		u = s;
		t = b;

		while (*t && *t == *u)
		{
			++t, ++u;
		}

		if (*t == '\0')
		{
			safe_ltos(buff, bufc, i, LBUF_SIZE);
			XFREE(s1);
			XFREE(b1);
			return;
		}

		++i, ++s;
	}

	safe_nothing(buff, bufc);
	XFREE(s1);
	XFREE(b1);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * fun_lpos: Find all occurrences of a character in a string, and return a
 * space-separated list of the positions, starting at 0. i.e.,
 * lpos(a-bc-def-g,-) ==> 1 4 8
 */

void fun_lpos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *bb_p, *scratch_chartab, *buf;
	;
	int i;
	Delim osep;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	VaChk_Only_Out(3);
	scratch_chartab = (char *)XCALLOC(256, sizeof(char), "scratch_chartab");

	if (!fargs[1] || !*fargs[1])
	{
		scratch_chartab[(unsigned char)' '] = 1;
	}
	else
	{
		for (s = fargs[1]; *s; s++)
		{
			scratch_chartab[(unsigned char)*s] = 1;
		}
	}

	bb_p = *bufc;
	buf = s = strip_ansi(fargs[0]);

	for (i = 0; *s; i++, s++)
	{
		if (scratch_chartab[(unsigned char)*s])
		{
			if (*bufc != bb_p)
			{
				print_sep(&osep, buff, bufc);
			}

			safe_ltos(buff, bufc, i, LBUF_SIZE);
		}
	}

	XFREE(buf);
	XFREE(scratch_chartab);
}

/*
 * ---------------------------------------------------------------------------
 * diffpos: Return the position of the first character where s1 and s2 are
 * different.
 */

void fun_diffpos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i;
	char *s1, *s2;

	for (i = 0, s1 = fargs[0], s2 = fargs[1]; *s1 && *s2; i++, s1++, s2++)
	{
		while (*s1 == ESC_CHAR)
		{
			skip_esccode(&s1);
		}

		while (*s2 == ESC_CHAR)
		{
			skip_esccode(&s2);
		}

		if (*s1 != *s2)
		{
			safe_ltos(buff, bufc, i, LBUF_SIZE);
			return;
		}
	}

	safe_ltos(buff, bufc, -1, LBUF_SIZE);
}

/*
 * ---------------------------------------------------------------------------
 * Take a character position and return which word that char is in.
 * wordpos(<string>, <charpos>)
 */

void fun_wordpos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int charpos, i;
	char *buf, *cp, *tp, *xp;
	Delim isep;
	VaChk_Only_In(3);
	charpos = (int)strtol(fargs[1], (char **)NULL, 10);
	buf = cp = strip_ansi(fargs[0]);

	if ((charpos > 0) && (charpos <= (int)strlen(cp)))
	{
		tp = &(cp[charpos - 1]);
		cp = trim_space_sep(cp, &isep);
		xp = split_token(&cp, &isep);

		for (i = 1; xp; i++)
		{
			if (tp < (xp + strlen(xp)))
			{
				break;
			}

			xp = split_token(&cp, &isep);
		}

		safe_ltos(buff, bufc, i, LBUF_SIZE);
		XFREE(buf);
		return;
	}

	safe_nothing(buff, bufc);
	XFREE(buf);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * Take a character position and return what color that character would be.
 * ansipos(<string>, <charpos>[, <type>])
 */

void fun_ansipos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int charpos, i, ansi_state;
	char *s;
	VaChk_Range(2, 3);
	s = fargs[0];
	charpos = (int)strtol(fargs[1], (char **)NULL, 10);
	ansi_state = ANST_NORMAL;
	i = 0;

	while (*s && i < charpos)
	{
		if (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state);
		}
		else
		{
			++s, ++i;
		}
	}

	if (nfargs > 2 && (fargs[2][0] == 'e' || fargs[2][0] == '0'))
	{
		s = ansi_transition_esccode(ANST_NONE, ansi_state);
		safe_str(s, buff, bufc);
		XFREE(s);
	}
	else if (nfargs > 2 && (fargs[2][0] == 'p' || fargs[2][0] == '1'))
	{
		s = ansi_transition_mushcode(ANST_NONE, ansi_state);
		safe_str(s, buff, bufc);
		XFREE(s);
	}
	else
	{
		s = ansi_transition_letters(ANST_NONE, ansi_state);
		safe_str(s, buff, bufc);
		XFREE(s);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_repeat: repeats a string
 */

void fun_repeat(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int times, len, i, maxtimes;
	times = (int)strtol(fargs[1], (char **)NULL, 10);

	if ((times < 1) || (fargs[0] == NULL) || (!*fargs[0]))
	{
		return;
	}
	else if (times == 1)
	{
		safe_str(fargs[0], buff, bufc);
	}
	else
	{
		len = strlen(fargs[0]);
		maxtimes = (LBUF_SIZE - 1 - (*bufc - buff)) / len;
		maxtimes = (times > maxtimes) ? maxtimes : times;

		for (i = 0; i < maxtimes; i++)
		{
			memcpy(*bufc, fargs[0], len);
			*bufc += len;
		}

		if (times > maxtimes)
		{
			safe_strncat(buff, bufc, fargs[0], len, LBUF_SIZE);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * perform_border: Turn a string of words into a bordered paragraph: BORDER,
 * CBORDER, RBORDER border(<words>,<width without margins>[,<L margin
 * fill>[,<R margin fill>]])
 */

void perform_border(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int width, just;
	char *l_fill, *r_fill, *bb_p;
	char *sl, *el, *sw, *ew, *buf;
	int sl_ansi_state, el_ansi_state, sw_ansi_state, ew_ansi_state;
	int sl_pos, el_pos, sw_pos, ew_pos;
	int nleft, max, lead_chrs;
	just = Func_Mask(JUST_TYPE);
	VaChk_Range(2, 4);

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	width = (int)strtol(fargs[1], (char **)NULL, 10);

	if (width < 1)
	{
		width = 1;
	}

	if (nfargs > 2)
	{
		l_fill = fargs[2];
	}
	else
	{
		l_fill = NULL;
	}

	if (nfargs > 3)
	{
		r_fill = fargs[3];
	}
	else
	{
		r_fill = NULL;
	}

	bb_p = *bufc;
	sl = el = sw = NULL;
	ew = fargs[0];
	sl_ansi_state = el_ansi_state = ANST_NORMAL;
	sw_ansi_state = ew_ansi_state = ANST_NORMAL;
	sl_pos = el_pos = sw_pos = ew_pos = 0;

	while (1)
	{
		/*
	 * Locate the next start-of-word (SW)
	 */
		for (sw = ew, sw_ansi_state = ew_ansi_state, sw_pos = ew_pos; *sw; ++sw)
		{
			switch (*sw)
			{
			case ESC_CHAR:
				track_esccode(&sw, &sw_ansi_state);
				--sw;
				continue;

			case '\t':
			case '\r':
				*sw = ' ';

			/*
		 * FALLTHRU
		 */
			case ' ':
				++sw_pos;
				continue;

			case BEEP_CHAR:
				continue;

			default:
				break;
			}

			break;
		}

		/*
	 * Three ways out of that loop: end-of-string (ES),
	 * end-of-line (EL), and start-of-word (SW)
	 */
		if (!*sw && sl == NULL)
		{ /* ES, and nothing left to output */
			break;
		}

		/*
	 * Decide where start-of-line (SL) was
	 */
		if (sl == NULL)
		{
			if (ew == fargs[0] || ew[-1] == '\n')
			{
				/*
		 * Preserve indentation at SS or after
		 * explicit EL
		 */
				sl = ew;
				sl_ansi_state = ew_ansi_state;
				sl_pos = ew_pos;
			}
			else
			{
				/*
		 * Discard whitespace if previous line
		 * wrapped
		 */
				sl = sw;
				sl_ansi_state = sw_ansi_state;
				sl_pos = sw_pos;
			}
		}

		if (*sw == '\n')
		{ /* EL, so we have to output */
			ew = sw;
			ew_ansi_state = sw_ansi_state;
			ew_pos = sw_pos;
		}
		else
		{
			/*
	     * Locate the next end-of-word (EW)
	     */
			for (ew = sw, ew_ansi_state = sw_ansi_state, ew_pos = sw_pos; *ew; ++ew)
			{
				switch (*ew)
				{
				case ESC_CHAR:
					track_esccode(&ew, &ew_ansi_state);
					--ew;
					continue;

				case '\r':
				case '\t':
					*ew = ' ';

					/*
		     * FALLTHRU
		     */
				case ' ':
				case '\n':
					break;

				case BEEP_CHAR:
					continue;

				default:

					/*
		     * Break up long words
		     */
					if (ew_pos - sw_pos == width)
					{
						break;
					}

					++ew_pos;
					continue;
				}

				break;
			}

			/*
	     * Three ways out of that loop: ES, EL, EW
	     */

			/*
	     * If it fits on the line, add it
	     */
			if (ew_pos - sl_pos <= width)
			{
				el = ew;
				el_ansi_state = ew_ansi_state;
				el_pos = ew_pos;
			}

			/*
	     * If it's just EW, not ES or EL, and the line isn't
	     * too long, keep adding words to the line
	     */
			if (*ew && *ew != '\n' && (ew_pos - sl_pos <= width))
			{
				continue;
			}

			/*
	     * So now we definitely need to output a line
	     */
		}

		/*
	 * Could be a blank line, no words fit
	 */
		if (el == NULL)
		{
			el = sw;
			el_ansi_state = sw_ansi_state;
			el_pos = sw_pos;
		}

		/*
	 * Newline if this isn't the first line
	 */
		if (*bufc != bb_p)
		{
			safe_crlf(buff, bufc);
		}

		/*
	 * Left border text
	 */
		safe_str(l_fill, buff, bufc);

		/*
	 * Left space padding if needed
	 */
		if (just == JUST_RIGHT)
		{
			nleft = width - el_pos + sl_pos;
			print_padding(nleft, max, ' ');
		}
		else if (just == JUST_CENTER)
		{
			lead_chrs = (int)((width / 2) - ((el_pos - sl_pos) / 2) + .5);
			print_padding(lead_chrs, max, ' ');
		}

		/*
	 * Restore previous ansi state
	 */
		buf = ansi_transition_esccode(ANST_NORMAL, sl_ansi_state);
		safe_str(buf, buff, bufc);
		XFREE(buf);
		/*
	 * Print the words
	 */
		safe_strncat(buff, bufc, sl, el - sl, LBUF_SIZE);
		/*
	 * Back to ansi normal
	 */
		buf = ansi_transition_esccode(el_ansi_state, ANST_NORMAL);
		safe_str(buf, buff, bufc);
		XFREE(buf);

		/*
	 * Right space padding if needed
	 */
		if (just == JUST_LEFT)
		{
			nleft = width - el_pos + sl_pos;
			print_padding(nleft, max, ' ');
		}
		else if (just == JUST_CENTER)
		{
			nleft = width - lead_chrs - el_pos + sl_pos;
			print_padding(nleft, max, ' ');
		}

		/*
	 * Right border text
	 */
		safe_str(r_fill, buff, bufc);

		/*
	 * Update pointers for the next line
	 */
		if (!*el)
		{
			/*
	     * ES, and nothing left to output
	     */
			break;
		}
		else if (*ew == '\n' && sw == ew)
		{
			/*
	     * EL already handled on this line, and no new word
	     * yet
	     */
			++ew;
			sl = el = NULL;
		}
		else if (sl == sw)
		{
			/*
	     * No new word yet
	     */
			sl = el = NULL;
		}
		else
		{
			/*
	     * ES with more to output, EL for next line, or just
	     * a full line
	     */
			sl = sw;
			sl_ansi_state = sw_ansi_state;
			sl_pos = sw_pos;
			el = ew;
			el_ansi_state = ew_ansi_state;
			el_pos = ew_pos;
		}
	}
}

/*---------------------------------------------------------------------------
 * fun_align: Turn a set of lists into newspaper-like columns.
 *   align(<widths>,<col1>,...,<colN>,<fill char>,<col sep>,<row sep>)
 *   lalign(<widths>,<col data>,<delim>,<fill char>,<col sep>,<row sep>)
 *   Only <widths> and the column data parameters are mandatory.
 *
 * This is mostly PennMUSH-compatible, but not 100%.
 *   - ANSI is not stripped out of the column text. States will be correctly
 *     preserved, and will not bleed.
 *   - ANSI states are not supported in the widths, as they are unnecessary.
 */

void perform_align(int n_cols, char **raw_colstrs, char **data, char fillc, Delim col_sep, Delim row_sep, char *buff, char **bufc)
{
	int i, n;
	int *col_widths, *col_justs, *col_done;
	char *p, *bb_p, *l_p;
	char **xsl, **xel, **xsw, **xew;
	int *xsl_a, *xel_a, *xsw_a, *xew_a;
	int *xsl_p, *xel_p, *xsw_p, *xew_p;
	char *sl, *el, *sw, *ew, *buf;
	int sl_ansi_state, el_ansi_state, sw_ansi_state, ew_ansi_state;
	int sl_pos, el_pos, sw_pos, ew_pos;
	int width, just, nleft, max, lead_chrs;
	int n_done = 0, pending_coaright = 0;
	/*
     * Parse the column widths and justifications
     */
	col_widths = (int *)XCALLOC(n_cols, sizeof(int), "col_widths");
	col_justs = (int *)XCALLOC(n_cols, sizeof(int), "col_justs");

	for (i = 0; i < n_cols; i++)
	{
		p = raw_colstrs[i];

		switch (*p)
		{
		case '<':
			col_justs[i] = JUST_LEFT;
			p++;
			break;

		case '>':
			col_justs[i] = JUST_RIGHT;
			p++;
			break;

		case '-':
			col_justs[i] = JUST_CENTER;
			p++;
			break;

		default:
			col_justs[i] = JUST_LEFT;
		}

		for (n = 0; *p && isdigit((unsigned char)*p); p++)
		{
			n *= 10;
			n += *p - '0';
		}

		if (n < 1)
		{
			safe_str("#-1 INVALID COLUMN WIDTH", buff, bufc);
			XFREE(col_widths);
			XFREE(col_justs);
			return;
		}

		col_widths[i] = n;

		switch (*p)
		{
		case '.':
			col_justs[i] |= JUST_REPEAT;
			p++;
			break;

		case '`':
			col_justs[i] |= JUST_COALEFT;
			p++;
			break;

		case '\'':
			col_justs[i] |= JUST_COARIGHT;
			p++;
			break;
		}

		if (*p)
		{
			safe_str("#1 INVALID ALIGN STRING", buff, bufc);
			XFREE(col_widths);
			XFREE(col_justs);
			return;
		}
	}

	col_done = (int *)XCALLOC(n_cols, sizeof(int), "col_done");
	xsl = (char **)XCALLOC(n_cols, sizeof(char *), "xsl");
	xel = (char **)XCALLOC(n_cols, sizeof(char *), "xel");
	xsw = (char **)XCALLOC(n_cols, sizeof(char *), "xsw");
	xew = (char **)XCALLOC(n_cols, sizeof(char *), "xew");
	xsl_a = (int *)XCALLOC(n_cols, sizeof(int), "xsl_a");
	xel_a = (int *)XCALLOC(n_cols, sizeof(int), "xel_a");
	xsw_a = (int *)XCALLOC(n_cols, sizeof(int), "xsw_a");
	xew_a = (int *)XCALLOC(n_cols, sizeof(int), "xew_a");
	xsl_p = (int *)XCALLOC(n_cols, sizeof(int), "xsl_p");
	xel_p = (int *)XCALLOC(n_cols, sizeof(int), "xel_p");
	xsw_p = (int *)XCALLOC(n_cols, sizeof(int), "xsw_p");
	xew_p = (int *)XCALLOC(n_cols, sizeof(int), "xew_p");

	/*
     * calloc() auto-initializes things to 0, so just do the other things
     */

	for (i = 0; i < n_cols; i++)
	{
		xew[i] = data[i];
		xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ANST_NORMAL;
	}

	bb_p = *bufc;
	l_p = *bufc;

	while (n_done < n_cols)
	{
		for (i = 0; i < n_cols; i++)
		{
			/*
	     * If this is the first column, and it's not our
	     * first line, output a row separator.
	     */
			if ((i == 0) && (*bufc != bb_p))
			{
				print_sep(&row_sep, buff, bufc);
				l_p = *bufc;
			}

			/*
	     * If our column width is 0, we've coalesced and we
	     * can safely continue.
	     */
			if (col_widths[i] == 0)
			{
				continue;
			}

			/*
	     * If this is not the first column of this line,
	     * output a column separator.
	     */
			if (*bufc != l_p)
			{
				print_sep(&col_sep, buff, bufc);
			}

			/*
	     * If we have a pending right-coalesce, we take care
	     * of it now, though we save our previous width at
	     * this stage, since we're going to output at that
	     * width during this pass. We know we're not at width
	     * 0 ourselves at this point, so we don't have to
	     * worry about a cascading coalesce; it'll get taken
	     * care of by the loop, automatically. If we have a
	     * pending coalesce-right and we're at the first
	     * column, we know that we overflowed and should just
	     * clear it.
	     */
			width = col_widths[i];

			if (pending_coaright)
			{
				if (i > 0)
					col_widths[i] += pending_coaright + col_sep.len;

				pending_coaright = 0;
			}

			/*
	     * If we're done and our column width is not zero,
	     * and we are not repeating, we must fill in spaces
	     * before we continue.
	     */

			if (col_done[i] && !(col_justs[i] & JUST_REPEAT))
			{
				print_padding(width, max, fillc);
				continue;
			}

			/*
	     * Restore our state variables
	     */
			sl = xsl[i];
			el = xel[i];
			sw = xsw[i];
			ew = xew[i];
			sl_ansi_state = xsl_a[i];
			el_ansi_state = xel_a[i];
			sw_ansi_state = xsw_a[i];
			ew_ansi_state = xew_a[i];
			sl_pos = xsl_p[i];
			el_pos = xel_p[i];
			sw_pos = xsw_p[i];
			ew_pos = xew_p[i];
			just = col_justs[i];

			while (1)
			{
				/*
		 * Locate the next start-of-word (SW)
		 */
				for (sw = ew, sw_ansi_state = ew_ansi_state, sw_pos = ew_pos; *sw; ++sw)
				{
					switch (*sw)
					{
					case ESC_CHAR:
						track_esccode(&sw, &sw_ansi_state);
						--sw;
						continue;

					case '\t':
					case '\r':
						*sw = ' ';

					/*
			 * FALLTHRU
			 */
					case ' ':
						++sw_pos;
						continue;

					case BEEP_CHAR:
						continue;

					default:
						break;
					}

					break;
				}

				/*
		 * Three ways out of that locator loop:
		 * end-of-string (ES), end-of-line (EL), and
		 * start-of-word (SW)
		 */

				if (!*sw && sl == NULL)
				{
					/*
		     * ES, and nothing left
		     * * to output
		     */
					/*
		     * If we're coalescing left, we set
		     * this column to 0 width, and
		     * increase the width of the left
		     * column. If we're coalescing right,
		     * we can't widen that column yet,
		     * because otherwise it'll throw off
		     * its width for this pass, so we've
		     * got to do that later. If we're
		     * repeating, we reset our pointer
		     * state, but we keep track of our
		     * done-ness. Don't increment done
		     * more than once, since we might
		     * repeat several times.
		     */
					if (!col_done[i])
					{
						n_done++;
						col_done[i] = 1;
					}

					if (i && (just & JUST_COALEFT))
					{
						/*
			 * Find the next-left column
			 * with a nonzero width,
			 * since we can have
			 * casdading coalescing.
			 */
						for (n = i - 1; (n > 0) && (col_widths[n] == 0); n--)
							;

						/*
			 * We have to add not only
			 * the width of the column,
			 * but the column separator
			 * length.
			 */
						col_widths[n] += col_widths[i] + col_sep.len;
						col_widths[i] = 0;
					}
					else if ((just & JUST_COARIGHT) && (i + 1 < n_cols))
					{
						pending_coaright = col_widths[i];
						col_widths[i] = 0;
					}
					else if (just & JUST_REPEAT)
					{
						xsl[i] = xel[i] = xsw[i] = NULL;
						xew[i] = data[i];
						xsl_a[i] = xel_a[i] = ANST_NORMAL;
						xsw_a[i] = xew_a[i] = ANST_NORMAL;
						xsl_p[i] = xel_p[i] = xsw_p[i] = xew_p[i] = 0;
					}

					break; /* get out of our infinite
				 * while */
				}

				/*
		 * Decide where start-of-line (SL) was
		 */
				if (sl == NULL)
				{
					if (ew == data[i] || ew[-1] == '\n')
					{
						/*
			 * Preserve indentation at SS
			 * or after explicit EL
			 */
						sl = ew;
						sl_ansi_state = ew_ansi_state;
						sl_pos = ew_pos;
					}
					else
					{
						/*
			 * Discard whitespace if
			 * previous line wrapped
			 */
						sl = sw;
						sl_ansi_state = sw_ansi_state;
						sl_pos = sw_pos;
					}
				}

				if (*sw == '\n')
				{
					/*
		     * EL, so we have to
		     * * output
		     */
					ew = sw;
					ew_ansi_state = sw_ansi_state;
					ew_pos = sw_pos;
					break;
				}
				else
				{
					/*
		     * Locate the next end-of-word (EW)
		     */
					for (ew = sw, ew_ansi_state = sw_ansi_state, ew_pos = sw_pos; *ew; ++ew)
					{
						switch (*ew)
						{
						case ESC_CHAR:
							track_esccode(&ew, &ew_ansi_state);
							--ew;
							continue;

						case '\r':
						case '\t':
							*ew = ' ';

							/*
			     * FALLTHRU
			     */
						case ' ':
						case '\n':
							break;

						case BEEP_CHAR:
							continue;

						default:

							/*
			     * Break up long
			     * words
			     */
							if (ew_pos - sw_pos == width)
							{
								break;
							}

							++ew_pos;
							continue;
						}

						break;
					}

					/*
		     * Three ways out of that previous
		     * for loop: ES, EL, EW
		     */

					/*
		     * If it fits on the line, add it
		     */
					if (ew_pos - sl_pos <= width)
					{
						el = ew;
						el_ansi_state = ew_ansi_state;
						el_pos = ew_pos;
					}

					/*
		     * If it's just EW, not ES or EL, and
		     * the line isn't too long, keep
		     * adding words to the line
		     */
					if (*ew && *ew != '\n' && (ew_pos - sl_pos <= width))
					{
						continue;
					}

					/*
		     * So now we definitely need to
		     * output a line
		     */
					break;
				}
			}

			/*
	     * Could be a blank line, no words fit
	     */
			if (el == NULL)
			{
				el = sw;
				el_ansi_state = sw_ansi_state;
				el_pos = sw_pos;
			}

			/*
	     * Left space padding if needed
	     */
			if (just & JUST_RIGHT)
			{
				nleft = width - el_pos + sl_pos;
				print_padding(nleft, max, fillc);
			}
			else if (just & JUST_CENTER)
			{
				lead_chrs = (int)((width / 2) - ((el_pos - sl_pos) / 2) + .5);
				print_padding(lead_chrs, max, fillc);
			}

			/*
	     * Restore previous ansi state
	     */
			buf = ansi_transition_esccode(ANST_NORMAL, sl_ansi_state);
			safe_str(buf, buff, bufc);
			XFREE(buf);
			/*
	     * Print the words
	     */
			safe_strncat(buff, bufc, sl, el - sl, LBUF_SIZE);
			/*
	     * Back to ansi normal
	     */
			buf = ansi_transition_esccode(el_ansi_state, ANST_NORMAL);
			safe_str(buf, buff, bufc);
			XFREE(buf);

			/*
	     * Right space padding if needed
	     */
			if (just & JUST_LEFT)
			{
				nleft = width - el_pos + sl_pos;
				print_padding(nleft, max, fillc);
			}
			else if (just & JUST_CENTER)
			{
				nleft = width - lead_chrs - el_pos + sl_pos;
				print_padding(nleft, max, fillc);
			}

			/*
	     * Update pointers for the next line
	     */

			if (!*el)
			{
				/*
		 * ES, and nothing left to output
		 */
				if (!col_done[i])
				{
					n_done++;
					col_done[i] = 1;
				}

				if ((just & JUST_COALEFT) && (i - 1 >= 0))
				{
					for (n = i - 1; (n > 0) && (col_widths[n] == 0); n--)
						;

					col_widths[n] += col_widths[i] + col_sep.len;
					col_widths[i] = 0;
				}
				else if ((just & JUST_COARIGHT) && (i + 1 < n_cols))
				{
					pending_coaright = col_widths[i];
					col_widths[i] = 0;
				}
				else if (just & JUST_REPEAT)
				{
					xsl[i] = xel[i] = xsw[i] = NULL;
					xew[i] = data[i];
					xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ANST_NORMAL;
					xsl_p[i] = xel_p[i] = xsw_p[i] = xew_p[i] = 0;
				}
			}
			else
			{
				if (*ew == '\n' && sw == ew)
				{
					/*
		     * EL already handled on this line,
		     * and no new word yet
		     */
					++ew;
					sl = el = NULL;
				}
				else if (sl == sw)
				{
					/*
		     * No new word yet
		     */
					sl = el = NULL;
				}
				else
				{
					/*
		     * ES with more to output, EL for
		     * next line, or just a full line
		     */
					sl = sw;
					sl_ansi_state = sw_ansi_state;
					sl_pos = sw_pos;
					el = ew;
					el_ansi_state = ew_ansi_state;
					el_pos = ew_pos;
				}

				/*
		 * Save state
		 */
				xsl[i] = sl;
				xel[i] = el;
				xsw[i] = sw;
				xew[i] = ew;
				xsl_a[i] = sl_ansi_state;
				xel_a[i] = el_ansi_state;
				xsw_a[i] = sw_ansi_state;
				xew_a[i] = ew_ansi_state;
				xsl_p[i] = sl_pos;
				xel_p[i] = el_pos;
				xsw_p[i] = sw_pos;
				xew_p[i] = ew_pos;
			}
		}
	}

	XFREE(col_widths);
	XFREE(col_justs);
	XFREE(col_done);
	XFREE(xsl);
	XFREE(xel);
	XFREE(xsw);
	XFREE(xew);
	XFREE(xsl_a);
	XFREE(xel_a);
	XFREE(xsw_a);
	XFREE(xew_a);
	XFREE(xsl_p);
	XFREE(xel_p);
	XFREE(xsw_p);
	XFREE(xew_p);
}

void fun_align(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **raw_colstrs;
	int n_cols;
	Delim filler, col_sep, row_sep;

	if (nfargs < 2)
	{
		safe_str("#-1 FUNCTION (ALIGN) EXPECTS AT LEAST 2 ARGUMENTS", buff, bufc);
		return;
	}

	/*
     * We need to know how many columns we have, so we know where the
     * column arguments stop and where the optional arguments start.
     */
	n_cols = list2arr(&raw_colstrs, LBUF_SIZE / 2, fargs[0], &SPACE_DELIM);

	if (nfargs < n_cols + 1)
	{
		safe_str("#-1 NOT ENOUGH COLUMNS FOR ALIGN", buff, bufc);
		XFREE(raw_colstrs);
		return;
	}

	if (nfargs > n_cols + 4)
	{
		safe_str("#-1 TOO MANY COLUMNS FOR ALIGN", buff, bufc);
		XFREE(raw_colstrs);
		return;
	}

	/*
     * Note that the VaChk macros number arguments from 1.
     */
	VaChk_Sep(&filler, n_cols + 2, 0);
	VaChk_SepOut(col_sep, n_cols + 3, 0);
	VaChk_SepOut(row_sep, n_cols + 4, 0);

	if (nfargs < n_cols + 4)
	{
		row_sep.str[0] = '\r';
	}

	perform_align(n_cols, raw_colstrs, fargs + 1, filler.str[0], col_sep, row_sep, buff, bufc);
	XFREE(raw_colstrs);
}

void fun_lalign(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **raw_colstrs, **data;
	int n_cols, n_data;
	Delim isep, filler, col_sep, row_sep;
	VaChk_Range(2, 6);
	n_cols = list2arr(&raw_colstrs, LBUF_SIZE / 2, fargs[0], &SPACE_DELIM);
	VaChk_InSep(3, 0);
	n_data = list2arr(&data, LBUF_SIZE / 2, fargs[1], &isep);

	if (n_cols > n_data)
	{
		safe_str("#-1 NOT ENOUGH COLUMNS FOR LALIGN", buff, bufc);
		XFREE(raw_colstrs);
		XFREE(data);
		return;
	}

	if (n_cols < n_data)
	{
		safe_str("#-1 TOO MANY COLUMNS FOR LALIGN", buff, bufc);
		XFREE(raw_colstrs);
		XFREE(data);
		return;
	}

	VaChk_Sep(&filler, 4, 0);
	VaChk_SepOut(col_sep, 5, 0);
	VaChk_SepOut(row_sep, 6, 0);

	if (nfargs < 6)
	{
		row_sep.str[0] = '\r';
	}

	perform_align(n_cols, raw_colstrs, data, filler.str[0], col_sep, row_sep, buff, bufc);
	XFREE(raw_colstrs);
	XFREE(data);
}

/*
 * ---------------------------------------------------------------------------
 * String concatenation.
 */

void fun_cat(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i;
	safe_str(fargs[0], buff, bufc);

	for (i = 1; i < nfargs; i++)
	{
		safe_chr(' ', buff, bufc);
		safe_str(fargs[i], buff, bufc);
	}
}

void fun_strcat(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i;
	safe_str(fargs[0], buff, bufc);

	for (i = 1; i < nfargs; i++)
	{
		safe_str(fargs[i], buff, bufc);
	}
}

void fun_join(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim osep;
	char *bb_p;
	int i;

	if (nfargs < 1)
	{
		return;
	}

	VaChk_OutSep(1, 0);
	bb_p = *bufc;

	for (i = 1; i < nfargs; i++)
	{
		if (*fargs[i])
		{
			if (*bufc != bb_p)
			{
				print_sep(&osep, buff, bufc);
			}

			safe_str(fargs[i], buff, bufc);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Misc functions.
 */

void fun_strlen(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	safe_ltos(buff, bufc, strip_ansi_len(fargs[0]), LBUF_SIZE);
}

void fun_delete(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *savep, *buf;
	int count, start, nchars;
	int ansi_state_l = ANST_NORMAL;
	int ansi_state_r = ANST_NORMAL;
	s = fargs[0];
	start = (int)strtol(fargs[1], (char **)NULL, 10);
	nchars = (int)strtol(fargs[2], (char **)NULL, 10);

	if ((nchars <= 0) || (start + nchars <= 0))
	{
		safe_str(s, buff, bufc);
		return;
	}

	savep = s;

	for (count = 0; (count < start) && *s; ++count)
	{
		while (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state_l);
		}

		if (*s)
		{
			++s;
		}
	}

	safe_strncat(buff, bufc, savep, s - savep, LBUF_SIZE);
	ansi_state_r = ansi_state_l;

	while (*s == ESC_CHAR)
	{
		track_esccode(&s, &ansi_state_r);
	}

	for (; (count < start + nchars) && *s; ++count)
	{
		++s;

		while (*s == ESC_CHAR)
		{
			track_esccode(&s, &ansi_state_r);
		}
	}

	if (*s)
	{
		buf = ansi_transition_esccode(ansi_state_l, ansi_state_r);
		safe_str(buf, buff, bufc);
		XFREE(buf);
		safe_str(s, buff, bufc);
	}
	else
	{
		buf = ansi_transition_esccode(ansi_state_l, ANST_NORMAL);
		safe_str(buf, buff, bufc);
		XFREE(buf);
	}
}

/*
 * ---------------------------------------------------------------------------
 * Misc PennMUSH-derived functions.
 */

void fun_lit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * Just returns the argument, literally
     */
	safe_str(fargs[0], buff, bufc);
}

void fun_art(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * checks a word and returns the appropriate article, "a" or "an"
     */
	char *s = fargs[0];
	char c;

	while (*s && (isspace(*s) || iscntrl(*s)))
	{
		if (*s == ESC_CHAR)
		{
			skip_esccode(&s);
		}
		else
		{
			++s;
		}
	}

	c = tolower(*s);

	if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
	{
		safe_strncat(buff, bufc, "an", 2, LBUF_SIZE);
	}
	else
	{
		safe_chr('a', buff, bufc);
	}
}

void fun_alphamax(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *amax;
	int i = 1;

	if (!fargs[0])
	{
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}
	else
	{
		amax = fargs[0];
	}

	while ((i < nfargs) && fargs[i])
	{
		amax = (strcmp(amax, fargs[i]) > 0) ? amax : fargs[i];
		i++;
	}

	safe_str(amax, buff, bufc);
}

void fun_alphamin(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *amin;
	int i = 1;

	if (!fargs[0])
	{
		safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}
	else
	{
		amin = fargs[0];
	}

	while ((i < nfargs) && fargs[i])
	{
		amin = (strcmp(amin, fargs[i]) < 0) ? amin : fargs[i];
		i++;
	}

	safe_str(amin, buff, bufc);
}

void fun_valid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * Checks to see if a given <something> is valid as a parameter of a
     * given type (such as an object name).
     */
	if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1])
	{
		safe_chr('0', buff, bufc);
	}
	else if (!strcasecmp(fargs[0], "name"))
	{
		safe_bool(buff, bufc, ok_name(fargs[1]));
	}
	else if (!strcasecmp(fargs[0], "attrname"))
	{
		safe_bool(buff, bufc, ok_attr_name(fargs[1]));
	}
	else if (!strcasecmp(fargs[0], "playername"))
	{
		safe_bool(buff, bufc, (ok_player_name(fargs[1]) && badname_check(fargs[1])));
	}
	else
	{
		safe_nothing(buff, bufc);
	}
}

void fun_beep(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	safe_chr(BEEP_CHAR, buff, bufc);
}
