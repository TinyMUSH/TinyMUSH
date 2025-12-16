/**
 * @file wild.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Wildcard routines
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * @copyright Copyright (c) 1993 by T. Alexander Popiel
 *            This code is hereby placed under GNU copyleft,
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

char **arglist; /* argument return space */

int numargs; /* argument return size */

/* ---------------------------------------------------------------------------
 * check_literals: All literals in a wildcard pattern must appear in the
 *                 data string, or no match is possible.
 */

int check_literals(char *tstr, char *dstr)
{
	char pattern[LBUF_SIZE], data[LBUF_SIZE], *p, *dp, *ep, *xp;
	int len;

	/*
	 * Fast match the beginning of the string.
	 */

	while ((*tstr != '*') && (*tstr != '?'))
	{
		if (*tstr == '\\')
		{
			tstr++;
		}

		if ((*dstr != *tstr) && (tolower(*dstr) != tolower(*tstr)))
		{
			return 0;
		}

		if (!*dstr)
		{
			return 1;
		}

		tstr++;
		dstr++;
	}

	/*
	 * Make a lower-case copy of the data. Bounds-check to prevent overflow.
	 */
	ep = data;

	while (*dstr && (ep - data < LBUF_SIZE - 1))
	{
		*ep = tolower(*dstr);
		ep++;
		dstr++;
	}

	*ep = '\0';
	/* If dstr was truncated, fail (data too long) */
	if (*dstr)
	{
		return 0;
	}
	/*
	 * Fast match the end of the string.
	 * * When we're done with this, we'll also have established a better
	 * * end point for the remainder of the literals match.
	 * * ep will point to the null terminator at the end of the data string
	 * * we need to worry about (i.e., that which has not already been
	 * * taken care of by our backwards match).
	 * * xp will point to the last character of the pattern string we need
	 * * to worry about.
	 */
	ep--;
	xp = tstr + strlen(tstr) - 1;

	while ((ep >= dstr) && (xp >= tstr) && (*xp != '*') && (*xp != '?'))
	{
		/* Handle escape: if xp is escape, check next char (xp-1), then decrement xp only once */
		if (*xp == '\\')
		{
			if (xp > tstr)
			{
				xp--; /* Move to escaped char */
				if ((*ep != *xp) && (tolower(*ep) != tolower(*xp)))
				{
					return 0;
				}
			}
			else
			{
				return 0; /* Escape at start of pattern is invalid */
			}
		}
		else if ((*ep != *xp) && (tolower(*ep) != tolower(*xp)))
		{
			return 0;
		}

		ep--;
		xp--;
	}

	ep++;
	/* Validate ep is still in bounds */
	if (ep < data || ep >= data + LBUF_SIZE)
	{
		return 0;
	}
	*ep = '\0';
	/*
	 * Walk the pattern string. Use the wildcard characters as delimiters,
	 * * to extract the literal strings that we need to match sequentially.
	 */
	dp = data;

	while (*tstr && (tstr <= xp))
	{
		while ((*tstr == '*') || (*tstr == '?'))
		{
			tstr++;
		}

		if (!*tstr || (tstr > xp))
		{
			return 1;
		}

		p = pattern;
		len = 0;

		while (*tstr && (*tstr != '*') && (*tstr != '?') && (tstr <= xp))
		{
			if (*tstr == '\\')
			{
				tstr++;
			}

			/* Bounds check: prevent pattern overflow */
			if (p - pattern >= LBUF_SIZE - 1)
			{
				return 0; /* Pattern literal too long */
			}

			*p = tolower(*tstr);
			p++;
			tstr++;
			len++;
		}

		*p = '\0';

		if (len)
		{
			if ((dp = strstr(dp, pattern)) == NULL)
			{
				return 0;
			}

			dp += len;
		}

		if (dp >= ep)
		{
			return 1;
		}
	}

	return 1;
}

/* ---------------------------------------------------------------------------
 * quick_wild: do a wildcard match, without remembering the wild data.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 */

int real_quick_wild(char *tstr, char *dstr)
{
	int st;

	if (!tstr || !dstr)
	{
		return 0;
	}

	if (mushstate.wild_times_lev > mushconf.wild_times_lim)
	{
		return -1;
	}

	mushstate.wild_times_lev++;

	while (*tstr != '*')
	{
		switch (*tstr)
		{
		case '?':

			/*
			 * Single character match.  Return false if at end
			 * * of data.
			 */
			if (!*dstr)
			{
				return 0;
			}

			break;

		case '\\':
			/*
			 * Escape character.  Move up, and force literal
			 * * match of next character.
			 */
			tstr++;
			/* Check for end of pattern after escape */
			if (!*tstr)
			{
				return 0; /* Invalid escape at end */
			}
			__attribute__((fallthrough));
		default:

			/*
			 * Literal character.  Check for a match. If
			 * * matching end of data, return true.
			 */
			if ((*dstr != *tstr) && (tolower(*dstr) != tolower(*tstr)))
			{
				return 0;
			}

			if (!*dstr)
			{
				return 1;
			}
		}

		tstr++;
		dstr++;
	}

	/*
	 * Skip over '*'.
	 */
	tstr++;

	/*
	 * Return true on trailing '*'.
	 */

	if (!*tstr)
	{
		return 1;
	}

	/*
	 * Skip over wildcards.
	 */

	while ((*tstr == '?') || (*tstr == '*'))
	{
		if (*tstr == '?')
		{
			if (!*dstr)
			{
				return 0;
			}

			dstr++;
		}

		tstr++;
	}

	/*
	 * Skip over a backslash in the pattern string if it is there.
	 */

	if (*tstr == '\\')
	{
		tstr++;
		if (*tstr)
		{
			tstr++; /* Skip the escaped character */
		}
	}

	/*
	 * Return true on trailing '*'.
	 */

	if (!*tstr)
	{
		return 1;
	}

	/*
	 * Scan for possible matches.
	 */

	while (*dstr)
	{
		if ((*dstr == *tstr) || (tolower(*dstr) == tolower(*tstr)))
		{
			if ((st = real_quick_wild(tstr + 1, dstr + 1)) != 0)
			{
				return st;
			}
		}

		dstr++;
	}

	return 0;
}

int quick_wild(char *tstr, char *dstr)
{
	int st;

	if (!check_literals(tstr, dstr))
	{
		return 0;
	}

	mushstate.wild_times_lev = 0;
	st = real_quick_wild(tstr, dstr);

	if ((st < 0) && (mushstate.wild_times_lev > mushconf.wild_times_lim))
	{
		return 0;
	}

	return st;
}

/* ---------------------------------------------------------------------------
 * wild1: INTERNAL: do a wildcard match, remembering the wild data.
 *
 * DO NOT CALL THIS FUNCTION DIRECTLY - DOING SO MAY RESULT IN
 * SERVER CRASHES AND IMPROPER ARGUMENT RETURN.
 *
 * Side Effect: this routine modifies the 'arglist' global
 * variable.
 */

int real_wild1(char *tstr, char *dstr, int arg)
{
	char *datapos;
	int argpos, numextra, st;

	if (!tstr || !dstr)
	{
		return 0;
	}

	if (mushstate.wild_times_lev > mushconf.wild_times_lim)
	{
		return -1;
	}

	/* Guard: prevent excessive recursion depth (additional safeguard) */
	if (mushstate.wild_times_lev > 100)
	{
		return -1;
	}

	mushstate.wild_times_lev++;

	while (*tstr != '*')
	{
		switch (*tstr)
		{
		case '?':

			/*
			 * Single character match.  Return false if at end
			 * * of data.
			 */
			if (!*dstr)
			{
				return 0;
			}

			arglist[arg][0] = *dstr;
			arglist[arg][1] = '\0';
			arg++;

			/*
			 * Jump to the fast routine if we can.
			 */

			if (arg >= numargs)
			{
				return real_quick_wild(tstr + 1, dstr + 1);
			}

			break;

		case '\\':
			/*
			 * Escape character.  Move up, and force literal
			 * * match of next character.
			 */
			tstr++;
			/* Check for end of pattern after escape */
			if (!*tstr)
			{
				return 0; /* Invalid escape at end */
			}
			__attribute__((fallthrough));
			/*
			 * FALL THROUGH
			 */
		default:

			/*
			 * Literal character.  Check for a match. If
			 * * matching end of data, return true.
			 */
			if ((*dstr != *tstr) && (tolower(*dstr) != tolower(*tstr)))
			{
				return 0;
			}

			if (!*dstr)
			{
				return 1;
			}
		}

		tstr++;
		dstr++;
	}

	/*
	 * If at end of pattern, slurp the rest, and leave.
	 */

	if (!tstr[1])
	{
		XSTRNCPY(arglist[arg], dstr, LBUF_SIZE - 1);
		arglist[arg][LBUF_SIZE - 1] = '\0';
		return 1;
	}

	/*
	 * Remember current position for filling in the '*' return.
	 */
	datapos = dstr;
	argpos = arg;

	/*
	 * Scan forward until we find a non-wildcard.
	 */

	do
	{
		if (argpos < arg)
		{
			/*
			 * Fill in arguments if someone put another '*'
			 * * before a fixed string.
			 */
			arglist[argpos][0] = '\0';
			argpos++;

			/*
			 * Jump to the fast routine if we can.
			 */

			if (argpos >= numargs)
			{
				return real_quick_wild(tstr, dstr);
			}

			/*
			 * Fill in any intervening '?'s
			 */

			while (argpos < arg)
			{
				arglist[argpos][0] = *datapos;
				arglist[argpos][1] = '\0';
				datapos++;
				argpos++;

				/*
				 * Jump to the fast routine if we can.
				 */

				if (argpos >= numargs)
				{
					return real_quick_wild(tstr, dstr);
				}
			}
		}

		/*
		 * Skip over the '*' for now...
		 */
		tstr++;
		arg++;
		/*
		 * Skip over '?'s for now...
		 */
		numextra = 0;

		while (*tstr == '?')
		{
			if (!*dstr)
			{
				return 0;
			}

			tstr++;
			dstr++;
			arg++;
			numextra++;
		}
	} while (*tstr == '*');

	/*
	 * Skip over a backslash in the pattern string if it is there.
	 */

	if (*tstr == '\\')
	{
		tstr++;
	}

	/*
	 * Check for possible matches.  This loop terminates either at end
	 * * of data (resulting in failure), or at a successful match.
	 */
	while (1)
	{
		/*
		 * Scan forward until first character matches.
		 */
		if (*tstr)
			while ((*dstr != *tstr) && (tolower(*dstr) != tolower(*tstr)))
			{
				if (!*dstr)
				{
					return 0;
				}

				dstr++;
			}
		else
			while (*dstr)
			{
				dstr++;
			}

		/*
		 * The first character matches, now.  Check if the rest
		 * * does, using the fastest method, as usual.
		 */

		if (*dstr)
		{
			st = (arg < numargs) ? real_wild1(tstr + 1, dstr + 1, arg) : real_quick_wild(tstr + 1, dstr + 1);

			if (st < 0)
			{
				return st;
			}
		}
		else
		{
			st = 0;
		}

		if (!*dstr || st)
		{
			/*
			 * Found a match!  Fill in all remaining arguments.
			 * * First do the '*'...
			 */
			int copy_len = (dstr - datapos) - numextra;
			if (copy_len < 0)
			{
				copy_len = 0; /* Guard against underflow */
			}
			XSTRNCPY(arglist[argpos], datapos, copy_len);
			arglist[argpos][copy_len] = '\0';
			datapos = dstr - numextra;
			argpos++;

			/*
			 * Fill in any trailing '?'s that are left.
			 */

			while (numextra)
			{
				if (argpos >= numargs)
				{
					return 1;
				}

				arglist[argpos][0] = *datapos;
				arglist[argpos][1] = '\0';
				datapos++;
				argpos++;
				numextra--;
			}

			/*
			 * It's done!
			 */
			return 1;
		}
		else
		{
			dstr++;
		}
	}
}

int wild1(char *tstr, char *dstr, int arg)
{
	int st;

	if (!check_literals(tstr, dstr))
	{
		return 0;
	}

	mushstate.wild_times_lev = 0;
	st = real_wild1(tstr, dstr, arg);
	mushstate.wild_times_lev = 0; /* Always reset */

	if (st < 0)
	{
		return 0;
	}

	return st;
}

/* ---------------------------------------------------------------------------
 * wild: do a wildcard match, remembering the wild data.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 *
 * This function may crash if XMALLOC(LBUF_SIZE, ) fails.
 *
 * Side Effect: this routine modifies the 'arglist' and 'numargs'
 * global variables.
 */
int wild(char *tstr, char *dstr, char *args[], int nargs)
{
	int i, value;
	char *scan;

	/* Guard against null inputs */
	if (!tstr || !dstr || !args || nargs <= 0)
	{
		return 0;
	}

	/*
	 * Initialize the return array.
	 */

	for (i = 0; i < nargs; i++)
	{
		args[i] = NULL;
	}

	/*
	 * Do fast match.
	 */

	while ((*tstr != '*') && (*tstr != '?'))
	{
		if (*tstr == '\\')
		{
			tstr++;
		}

		if ((*dstr != *tstr) && (tolower(*dstr) != tolower(*tstr)))
		{
			return 0;
		}

		if (!*dstr)
		{
			return 1;
		}

		tstr++;
		dstr++;
	}

	/*
	 * Allocate space for the return args.
	 */
	i = 0;
	scan = tstr;

	while (*scan && (i < nargs))
	{
		switch (*scan)
		{
		case '?':
			args[i] = XMALLOC(LBUF_SIZE, "args[i]");
			i++;
			break;

		case '*':
			args[i] = XMALLOC(LBUF_SIZE, "args[i]");
			i++;
		}

		scan++;
	}

	/*
	 * Put stuff in globals for quick recursion.
	 */
	arglist = args;
	numargs = nargs;
	/*
	 * Do the match.
	 */
	value = nargs ? wild1(tstr, dstr, 0) : quick_wild(tstr, dstr);

	/*
	 * Clean out any fake match data left by wild1.
	 */

	for (i = 0; i < nargs; i++)
		if ((args[i] != NULL) && (!*args[i] || !value))
		{
			XFREE(args[i]);
			args[i] = NULL;
		}

	return value;
}

/* ---------------------------------------------------------------------------
 * wild_match: do either an order comparison or a wildcard match,
 * remembering the wild data, if wildcard match is done.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 */
int wild_match(char *tstr, char *dstr)
{
	switch (*tstr)
	{
	case '>':
		tstr++;

		if (isdigit(*tstr) || (*tstr == '-'))
		{
			return ((int)strtol(tstr, (char **)NULL, 10) < (int)strtol(dstr, (char **)NULL, 10));
		}
		else
		{
			return (strcmp(tstr, dstr) < 0);
		}

	case '<':
		tstr++;

		if (isdigit(*tstr) || (*tstr == '-'))
		{
			return ((int)strtol(tstr, (char **)NULL, 10) > (int)strtol(dstr, (char **)NULL, 10));
		}
		else
		{
			return (strcmp(tstr, dstr) > 0);
		}
	}

	return quick_wild(tstr, dstr);
}

/* ----------------------------------------------------------------------
 * register_match: Do a wildcard match, setting the wild data into the
 * global registers.
 */

int register_match(char *tstr, char *dstr, char *args[], int nargs)
{
	int i, value;
	char *buff, *scan, *p, *end, *q_names[NUM_ENV_VARS];

	/*
	 * Initialize return array.
	 */

	for (i = 0; i < nargs; i++)
	{
		args[i] = q_names[i] = NULL;
	}

	/*
	 * Do fast match.
	 */

	while ((*tstr != '*') && (*tstr != '?'))
	{
		if (*tstr == '\\')
		{
			tstr++;
		}

		if ((*dstr != *tstr) && (tolower(*dstr) != tolower(*tstr)))
		{
			return 0;
		}

		if (!*dstr)
		{
			return 1;
		}

		tstr++;
		dstr++;
	}

	/*
	 * Convert string, allocate space for the return args.
	 */
	i = 0;
	scan = tstr;
	buff = XMALLOC(LBUF_SIZE, "buff");
	if (!buff)
	{
		return 0; /* Allocation failed */
	}
	p = buff;

	while (*scan)
	{
		*p++ = *scan;

		switch (*scan)
		{
		case '?':

			/*
			 * FALLTHRU
			 */
		case '*':
			args[i] = XMALLOC(LBUF_SIZE, "args[i]");
			scan++;

			if (*scan == '{')
			{
				if ((end = strchr(scan + 1, '}')) != NULL)
				{
					*end = '\0';

					if (*(scan + 1))
					{
						q_names[i] = XSTRDUP(scan + 1, "q_names[i]");
					}

					scan = end + 1;
				}
			}

			i++;
			break;

		default:
			scan++;
		}
	}

	*p = '\0';
	/*
	 * Go do it.
	 */
	arglist = args;
	numargs = nargs;
	value = nargs ? wild1(buff, dstr, 0) : quick_wild(buff, dstr);

	/*
	 * Copy things into registers. Clean fake match data from wild1().
	 */

	for (i = 0; i < nargs; i++)
	{
		if ((args[i] != NULL) && (!*args[i] || !value))
		{
			XFREE(args[i]);
			args[i] = NULL;
		}

		if (args[i] && q_names[i])
		{
			set_register("rmatch", q_names[i], args[i]);
		}

		if (q_names[i])
		{
			XFREE(q_names[i]);
		}
	}

	XFREE(buff);
	return value;
}
