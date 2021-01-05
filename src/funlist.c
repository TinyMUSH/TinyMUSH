/**
 * @file funlist.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List functions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#include "system.h"

#include "typedefs.h"	/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "mushconf.h"	/* required by code */
#include "db.h"			/* required by externs */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "stringutil.h" /* required by code */

/*
 * ---------------------------------------------------------------------------
 * List management utilities.
 */

#define ALPHANUM_LIST 1
#define NUMERIC_LIST 2
#define DBREF_LIST 3
#define FLOAT_LIST 4
#define NOCASE_LIST 5

int autodetect_list(char *ptrs[], int nitems)
{
	int sort_type, i;
	char *p;
	sort_type = NUMERIC_LIST;

	for (i = 0; i < nitems; i++)
	{
		switch (sort_type)
		{
		case NUMERIC_LIST:
			if (!is_number(ptrs[i]))
			{
				/*
		 * If non-numeric, switch to alphanum sort.
		 * Exception: if this is the first element
		 * and it is a good dbref, switch to a dbref
		 * sort. We're a little looser than the
		 * normal 'good dbref' rules, any number
		 * following the #-sign is  accepted.
		 */
				if (i == 0)
				{
					p = ptrs[i];

					if (*p++ != NUMBER_TOKEN)
					{
						return ALPHANUM_LIST;
					}
					else if (is_integer(p))
					{
						sort_type = DBREF_LIST;
					}
					else
					{
						return ALPHANUM_LIST;
					}
				}
				else
				{
					return ALPHANUM_LIST;
				}
			}
			else if (strchr(ptrs[i], '.'))
			{
				sort_type = FLOAT_LIST;
			}

			break;

		case FLOAT_LIST:
			if (!is_number(ptrs[i]))
			{
				sort_type = ALPHANUM_LIST;
				return ALPHANUM_LIST;
			}

			break;

		case DBREF_LIST:
			p = ptrs[i];

			if (*p++ != NUMBER_TOKEN)
			{
				return ALPHANUM_LIST;
			}

			if (!is_integer(p))
			{
				return ALPHANUM_LIST;
			}

			break;

		default:
			return ALPHANUM_LIST;
		}
	}

	return sort_type;
}

int get_list_type(char *fargs[], int nfargs, int type_pos, char *ptrs[], int nitems)
{
	if (nfargs >= type_pos)
	{
		switch (tolower(*fargs[type_pos - 1]))
		{
		case 'd':
			return DBREF_LIST;

		case 'n':
			return NUMERIC_LIST;

		case 'f':
			return FLOAT_LIST;

		case 'i':
			return NOCASE_LIST;

		case '\0':
			return autodetect_list(ptrs, nitems);

		default:
			return ALPHANUM_LIST;
		}
	}

	return autodetect_list(ptrs, nitems);
}

int dbnum(char *dbr)
{
	if ((*dbr != '#') || (dbr[1] == '\0'))
	{
		return 0;
	}
	else
	{
		return (int)strtol(dbr + 1, (char **)NULL, 10);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_words: Returns number of words in a string. Aka vdim. Added 1/28/91
 * Philip D. Wasson
 */

void fun_words(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (nfargs == 0)
	{
		SAFE_LB_CHR('0', buff, bufc);
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	SAFE_LTOS(buff, bufc, countwords(fargs[0], &isep), LBUF_SIZE);
}

/*
 * ---------------------------------------------------------------------------
 * fun_first: Returns first word in a string
 */

void fun_first(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *first;
	Delim isep;

	/*
     * If we are passed an empty arglist return a null string
     */

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	s = trim_space_sep(fargs[0], &isep); /* leading spaces */
	first = split_token(&s, &isep);

	if (first)
	{
		SAFE_LB_STR(first, buff, bufc);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_rest: Returns all but the first word in a string
 */

void fun_rest(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *rest;
	Delim isep;
	int ansi_state = ANST_NONE;

	/*
     * If we are passed an empty arglist return a null string
     */

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	s = trim_space_sep(fargs[0], &isep); /* leading spaces */
	rest = next_token_ansi(s, &isep, &ansi_state);

	if (rest)
	{
		s = ansi_transition_esccode(ANST_NORMAL, ansi_state);
		SAFE_LB_STR(s, buff, bufc);
		XFREE(s);
		SAFE_LB_STR(rest, buff, bufc);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_last: Returns last word in a string
 */

void fun_last(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *last, *buf;
	Delim isep;
	int ansi_state = ANST_NONE;

	/*
     * If we are passed an empty arglist return a null string
     */

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	if (isep.len == 1)
	{
		last = s = trim_space_sep(fargs[0], &isep);

		do
		{
			/*
	     * this is like next_token(), but tracking ansi
	     */
			while (*s == ESC_CHAR)
			{
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++(s);
					if (*(s) == ANSI_CSI)
					{
						while ((*(++(s)) & 0xf0) == 0x30)
						{
							if (*(s) < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*(s)&0x0f);
							}
							else
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								param_val = 0;
							}
						}
					}
					while ((*(s)&0xf0) == 0x20)
					{
						++(s);
					}
					if (*(s) == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
						++(s);
					}
					else if (*(s))
					{
						++(s);
					}
				} while (0);
			}

			while (*s && (*s != isep.str[0]))
			{
				++s;

				while (*s == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(s);
						if (*(s) == ANSI_CSI)
						{
							while ((*(++(s)) & 0xf0) == 0x30)
							{
								if (*(s) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(s)&0x0f);
								}
								else
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									param_val = 0;
								}
							}
						}
						while ((*(s)&0xf0) == 0x20)
						{
							++(s);
						}
						if (*(s) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++(s);
						}
						else if (*(s))
						{
							++(s);
						}
					} while (0);
				}
			}

			if (*s)
			{
				++s;

				if (isep.str[0] == ' ')
				{
					while (*s == ' ')
					{
						++s;
					}
				}

				last = s;
			}
		} while (*s);

		buf = ansi_transition_esccode(ANST_NORMAL, ansi_state);
		SAFE_LB_STR(buf, buff, bufc);
		XFREE(buf);
		SAFE_STRNCAT(buff, bufc, last, s - last, LBUF_SIZE);
	}
	else
	{
		s = fargs[0];

		/*
	 * Walk backwards through the string to find the separator.
	 * Find the last character, and compare the previous
	 * characters, to find the separator. If we can't find the
	 * last character or we know we're going to fall off the
	 * string, return the original string.
	 */

		if ((last = strrchr(s, isep.str[isep.len - 1])) == NULL)
		{
			SAFE_LB_STR(s, buff, bufc);
			return;
		}

		while (last >= s + isep.len - 1)
		{
			if ((*last == isep.str[isep.len - 1]) && !strncmp(isep.str, last - isep.len + 1, isep.len))
			{
				++last;
				SAFE_LB_STR(last, buff, bufc);
				return;
			}

			last--;
		}

		SAFE_LB_STR(s, buff, bufc);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_match: Match arg2 against each word of arg1, returning index of first
 * match.
 */

void fun_match(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int wcount;
	char *r, *s;
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	/*
     * Check each word individually, returning the word number of the
     * first one that matches.  If none match, return 0.
     */
	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			SAFE_LTOS(buff, bufc, wcount, LBUF_SIZE);
			return;
		}

		wcount++;
	} while (s);

	SAFE_LB_CHR('0', buff, bufc);
}

void fun_matchall(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int wcount, flag;
	char *r, *s, *old;
	Delim isep, osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	flag = Func_Flags(fargs);

	/*
     * SPECIAL CASE: If there's no output delimiter specified, we use a
     * space, NOT the delimiter given for the list!
     */
	if (nfargs < 4)
	{
		osep.str[0] = ' ';
		osep.len = 1;
	}

	old = *bufc;
	/*
     * Check each word individually, returning the word number of all
     * that match (or don't match, in the case of unmatchall). If none,
     * return a null string.
     */
	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r) ? !(flag & IFELSE_FALSE) : (flag & IFELSE_FALSE))
		{
			if (old != *bufc)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LTOS(buff, bufc, wcount, LBUF_SIZE);
		}

		wcount++;
	} while (s);
}

/*
 * ---------------------------------------------------------------------------
 * fun_extract: extract words from string: extract(foo bar baz,1,2) returns
 * 'foo bar' extract(foo bar baz,2,1) returns 'bar' extract(foo bar baz,2,2)
 * returns 'bar baz'
 *
 * Now takes optional separator extract(foo-bar-baz,1,2,-) returns 'foo-bar'
 */

void fun_extract(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int start, len;
	char *r, *s, *t;
	Delim isep, osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 5, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 5)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	s = fargs[0];
	start = (int)strtol(fargs[1], (char **)NULL, 10);
	len = (int)strtol(fargs[2], (char **)NULL, 10);

	if ((start < 1) || (len < 1))
	{
		return;
	}

	/*
     * Skip to the start of the string to save
     */
	start--;
	s = trim_space_sep(s, &isep);

	while (start && s)
	{
		s = next_token(s, &isep);
		start--;
	}

	/*
     * If we ran of the end of the string, return nothing
     */

	if (!s || !*s)
	{
		return;
	}

	/*
     * If our delimiter is the same, we have an easy task. Otherwise we
     * have to go token by token.
     */

	if (!strcmp((&isep)->str, (&osep)->str))
	{
		/*
	 * Count off the words in the string to save
	 */
		r = s;
		len--;

		while (len && s)
		{
			s = next_token(s, &isep);
			len--;
		}

		/*
	 * Chop off the rest of the string, if needed
	 */
		if (s && *s)
		{
			t = split_token(&s, &isep);
		}

		SAFE_LB_STR(r, buff, bufc);
	}
	else
	{
		r = *bufc;

		do
		{
			t = split_token(&s, &isep);

			if (r != *bufc)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LB_STR(t, buff, bufc);
			len--;
		} while (len && s && *s);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_index:  like extract(), but it works with an arbitrary separator.
 * index(a b | c d e | f gh | ij k, |, 2, 1) => c d e index(a b | c d e | f
 * gh | ij k, |, 2, 2) => c d e | f g h
 */

void fun_index(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int start, end;
	char c, *s, *p;
	s = fargs[0];
	c = *fargs[1];
	start = (int)strtol(fargs[2], (char **)NULL, 10);
	end = (int)strtol(fargs[3], (char **)NULL, 10);

	if ((start < 1) || (end < 1) || (*s == '\0'))
	{
		return;
	}

	if (c == '\0')
	{
		c = ' ';
	}

	/*
     * move s to point to the start of the item we want
     */
	start--;

	while (start && s && *s)
	{
		if ((s = strchr(s, c)) != NULL)
		{
			s++;
		}

		start--;
	}

	/*
     * skip over just spaces
     */

	while (s && (*s == ' '))
	{
		s++;
	}

	if (!s || !*s)
	{
		return;
	}

	/*
     * figure out where to end the string
     */
	p = s;

	while (end && p && *p)
	{
		if ((p = strchr(p, c)) != NULL)
		{
			if (--end == 0)
			{
				do
				{
					p--;
				} while ((*p == ' ') && (p > s));

				*(++p) = '\0';
				SAFE_LB_STR(s, buff, bufc);
				return;
			}
			else
			{
				p++;
			}
		}
	}

	/*
     * if we've gotten this far, we've run off the end of the string
     */
	SAFE_LB_STR(s, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * ldelete: Remove a word from a string by place
 * ldelete(<list>,<position>[,<separator>])
 *
 * insert: insert a word into a string by place insert(<list>,<position>,<new
 * item> [,<separator>])
 *
 * replace: replace a word into a string by place replace(<list>,<position>,<new
 * item>[,<separator>])
 *
 * lreplace: replace multiple words into a string by places
 * lreplace(<list>,<replacement words>,<positions>[,<isep>,<osep>])
 */

#define IF_DELETE 0
#define IF_REPLACE 1
#define IF_INSERT 2

void do_itemfuns(char *buff, char **bufc, char *str, int el, char *word, const Delim *sep, int flag)
{
	int ct, overrun;
	char *sptr, *iptr, *eptr;

	/*
     * If passed a null string return an empty string, except that we are
     * allowed to append to a null string.
     */

	if ((!str || !*str) && ((flag != IF_INSERT) || (el != 1)))
	{
		return;
	}

	/*
     * we can't fiddle with anything before the first position
     */

	if (el < 1)
	{
		SAFE_LB_STR(str, buff, bufc);
		return;
	}

	/*
     * Split the list up into 'before', 'target', and 'after' chunks
     * pointed to by sptr, iptr, and eptr respectively.
     */

	if (el == 1)
	{
		/*
	 * No 'before' portion, just split off element 1
	 */
		sptr = NULL;

		if (!str || !*str)
		{
			eptr = NULL;
			iptr = NULL;
		}
		else
		{
			eptr = trim_space_sep(str, sep);
			iptr = split_token(&eptr, sep);
		}
	}
	else
	{
		/*
	 * Break off 'before' portion
	 */
		sptr = eptr = trim_space_sep(str, sep);
		overrun = 1;

		for (ct = el; ct > 2 && eptr; eptr = next_token(eptr, sep), ct--)
			;

		if (eptr)
		{
			overrun = 0;
			iptr = split_token(&eptr, sep);
		}

		/*
	 * If we didn't make it to the target element, just return
	 * the string.  Insert is allowed to continue if we are
	 * exactly at the end of the string, but replace and delete
	 * are not.
	 */

		if (!(eptr || ((flag == IF_INSERT) && !overrun)))
		{
			SAFE_LB_STR(str, buff, bufc);
			return;
		}

		/*
	 * Split the 'target' word from the 'after' portion.
	 */

		if (eptr)
		{
			iptr = split_token(&eptr, sep);
		}
		else
		{
			iptr = NULL;
		}
	}

	switch (flag)
	{
	case IF_DELETE: /* deletion */
		if (sptr)
		{
			SAFE_LB_STR(sptr, buff, bufc);

			if (eptr)
			{
				print_separator(sep, buff, bufc);
			}
		}

		if (eptr)
		{
			SAFE_LB_STR(eptr, buff, bufc);
		}

		break;

	case IF_REPLACE: /* replacing */
		if (sptr)
		{
			SAFE_LB_STR(sptr, buff, bufc);
			print_separator(sep, buff, bufc);
		}

		SAFE_LB_STR(word, buff, bufc);

		if (eptr)
		{
			print_separator(sep, buff, bufc);
			SAFE_LB_STR(eptr, buff, bufc);
		}

		break;

	case IF_INSERT: /* insertion */
		if (sptr)
		{
			SAFE_LB_STR(sptr, buff, bufc);
			print_separator(sep, buff, bufc);
		}

		SAFE_LB_STR(word, buff, bufc);

		if (iptr)
		{
			print_separator(sep, buff, bufc);
			SAFE_LB_STR(iptr, buff, bufc);
		}

		if (eptr)
		{
			print_separator(sep, buff, bufc);
			SAFE_LB_STR(eptr, buff, bufc);
		}

		break;
	}
}

void fun_ldelete(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * delete a word at position X of a list
     */
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	};

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), NULL, &isep, IF_DELETE);
}

void fun_replace(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * replace a word at position X of a list
     */
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), fargs[2], &isep, IF_REPLACE);
}

void fun_insert(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
     * insert a word at position X of a list
     */
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), fargs[2], &isep, IF_INSERT);
}

void fun_lreplace(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;
	Delim osep;
	char *origlist, *replist, *poslist;
	char **orig_p, **rep_p, **pos_p;
	int norig, npos, i, cpos;
	/*
     * We're generous with the argument checking, in case the replacement
     * list is blank, and/or the position list is blank.
     */
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 5, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 5)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	/*
     * If there are no positions to replace, then we just return the
     * original list.
     */

	if ((nfargs < 3) || !fargs[2])
	{
		SAFE_LB_STR(fargs[0], buff, bufc);
		return;
	}

	/*
     * The number of elements we have in our replacement list must equal
     * the number of elements in our position list.
     */

	if (!fargs[1] || (countwords(fargs[1], &isep) != countwords(fargs[2], &SPACE_DELIM)))
	{
		SAFE_LB_STR("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}

	/*
     * Turn out lists into arrays for ease of manipulation.
     */
	origlist = XMALLOC(LBUF_SIZE, "origlist");
	replist = XMALLOC(LBUF_SIZE, "replist");
	poslist = XMALLOC(LBUF_SIZE, "poslist");
	XSTRCPY(origlist, fargs[0]);
	XSTRCPY(replist, fargs[1]);
	XSTRCPY(poslist, fargs[2]);
	norig = list2arr(&orig_p, LBUF_SIZE / 2, origlist, &isep);
	list2arr(&rep_p, LBUF_SIZE / 2, replist, &isep);
	npos = list2arr(&pos_p, LBUF_SIZE / 2, poslist, &SPACE_DELIM);

	/*
     * The positions we have aren't necessarily sequential, so we can't
     * just walk through the list. We have to replace position by
     * position. If we get an invalid position number, just ignore it.
     */

	for (i = 0; i < npos; i++)
	{
		cpos = (int)strtol(pos_p[i], (char **)NULL, 10);

		if ((cpos > 0) && (cpos <= norig))
		{
			orig_p[cpos - 1] = rep_p[i];
		}
	}

	arr2list(orig_p, norig, buff, bufc, &osep);
	XFREE(orig_p);
	XFREE(rep_p);
	XFREE(pos_p);
	XFREE(origlist);
	XFREE(replist);
	XFREE(poslist);
}

/*
 * ---------------------------------------------------------------------------
 * fun_remove: Remove a word from a string
 */

void fun_remove(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *sp, *word, *bb_p;
	Delim isep;
	int found;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	};

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	if (((isep.len == 1) && strchr(fargs[1], isep.str[0])) || ((isep.len > 1) && strstr(fargs[1], isep.str)))
	{
		SAFE_LB_STR("#-1 CAN ONLY DELETE ONE ELEMENT", buff, bufc);
		return;
	}

	s = fargs[0];
	word = fargs[1];
	/*
     * Walk through the string copying words until (if ever) we get to
     * one that matches the target word.
     */
	sp = s;
	found = 0;
	bb_p = *bufc;

	while (s)
	{
		sp = split_token(&s, &isep);

		if (found || strcmp(sp, word))
		{
			if (*bufc != bb_p)
			{
				print_separator(&isep, buff, bufc);
			}

			SAFE_LB_STR(sp, buff, bufc);
		}
		else
		{
			found = 1;
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_member: Is a word in a string
 */

void fun_member(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int wcount;
	char *r, *s;
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (!strcmp(fargs[1], r))
		{
			SAFE_LTOS(buff, bufc, wcount, LBUF_SIZE);
			return;
		}

		wcount++;
	} while (s);

	SAFE_LB_CHR('0', buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_revwords: Reverse the order of words in a list.
 */

void fun_revwords(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p, **elems;
	Delim isep;
	int n_elems, i;

	/*
     * If we are passed an empty arglist return a null string
     */

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	/*
     * Nasty bounds checking
     */

	if ((int)strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff) - 1)
	{
		*(fargs[0] + (LBUF_SIZE - (*bufc - buff) - 1)) = '\0';
	}

	/*
     * Chop it up into an array of words and reverse them.
     */
	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	bb_p = *bufc;

	for (i = n_elems - 1; i >= 0; i--)
	{
		if (*bufc != bb_p)
		{
			print_separator(&isep, buff, bufc);
		}

		SAFE_LB_STR(elems[i], buff, bufc);
	}

	XFREE(elems);
}

/*
 * ---------------------------------------------------------------------------
 * fun_splice: given two lists and a word, merge the two lists by replacing
 * words in list1 that are the same as the given word by the corresponding
 * word in list2 (by position). The lists must have the same number of words.
 * Compare to MERGE().
 */

void fun_splice(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p1, *p2, *q1, *q2, *bb_p;
	Delim isep, osep;
	int words, i;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 5, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 5)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	/*
     * length checks
     */

	if (countwords(fargs[2], &isep) > 1)
	{
		SAFE_LB_STR("#-1 TOO MANY WORDS", buff, bufc);
		return;
	}

	words = countwords(fargs[0], &isep);

	if (words != countwords(fargs[1], &isep))
	{
		SAFE_LB_STR("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}

	/*
     * loop through the two lists
     */
	p1 = fargs[0];
	q1 = fargs[1];
	bb_p = *bufc;

	for (i = 0; i < words; i++)
	{
		p2 = split_token(&p1, &isep);
		q2 = split_token(&q1, &isep);

		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		if (!strcmp(p2, fargs[2]))
		{
			SAFE_LB_STR(q2, buff, bufc); /* replace */
		}
		else
		{
			SAFE_LB_STR(p2, buff, bufc); /* copy */
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * handle_sort: Sort lists.
 */

typedef struct f_record f_rec;

typedef struct i_record i_rec;

typedef struct a_record a_rec;

struct f_record
{
	double data;
	char *str;
	int pos;
};

struct i_record
{
	long data;
	char *str;
	int pos;
};

struct a_record
{
	char *str;
	int pos;
};

int a_comp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

int c_comp(const void *s1, const void *s2)
{
	return strcasecmp(*(char **)s1, *(char **)s2);
}

int arec_comp(const void *s1, const void *s2)
{
	return strcmp(((a_rec *)s1)->str, ((a_rec *)s2)->str);
}

int crec_comp(const void *s1, const void *s2)
{
	return strcasecmp(((a_rec *)s1)->str, ((a_rec *)s2)->str);
}

int f_comp(const void *s1, const void *s2)
{
	if (((f_rec *)s1)->data > ((f_rec *)s2)->data)
	{
		return 1;
	}

	if (((f_rec *)s1)->data < ((f_rec *)s2)->data)
	{
		return -1;
	}

	return 0;
}

int i_comp(const void *s1, const void *s2)
{
	if (((i_rec *)s1)->data > ((i_rec *)s2)->data)
	{
		return 1;
	}

	if (((i_rec *)s1)->data < ((i_rec *)s2)->data)
	{
		return -1;
	}

	return 0;
}

#define Get_Poslist(p, n, l)                 \
	l = (int *)XCALLOC(n, sizeof(int), "l"); \
	for (i = 0; i < n; i++)                  \
		l[i] = p[i].pos;

int *do_asort(char *s[], int n, int sort_type, int listpos_only)
{
	int i;
	f_rec *fp = NULL;
	i_rec *ip = NULL;
	a_rec *ap = NULL;
	int *poslist = NULL;

	switch (sort_type)
	{
	case ALPHANUM_LIST:
		if (!listpos_only)
		{
			qsort((void *)s, n, sizeof(char *), (int (*)(const void *, const void *))a_comp);
		}
		else
		{
			ap = (a_rec *)XCALLOC(n, sizeof(a_rec), "ap");

			for (i = 0; i < n; i++)
			{
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}

			qsort((void *)ap, n, sizeof(a_rec), (int (*)(const void *, const void *))arec_comp);
			Get_Poslist(ap, n, poslist);
			XFREE(ap);
		}

		break;

	case NOCASE_LIST:
		if (!listpos_only)
		{
			qsort((void *)s, n, sizeof(char *), (int (*)(const void *, const void *))c_comp);
		}
		else
		{
			ap = (a_rec *)XCALLOC(n, sizeof(a_rec), "ap");

			for (i = 0; i < n; i++)
			{
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}

			qsort((void *)ap, n, sizeof(a_rec), (int (*)(const void *, const void *))crec_comp);
			Get_Poslist(ap, n, poslist);
			XFREE(ap);
		}

		break;

	case NUMERIC_LIST:
		ip = (i_rec *)XCALLOC(n, sizeof(i_rec), "ip");

		for (i = 0; i < n; i++)
		{
			ip[i].str = s[i];
			ip[i].data = (int)strtol(s[i], (char **)NULL, 10);
			ip[i].pos = i + 1;
		}

		qsort((void *)ip, n, sizeof(i_rec), (int (*)(const void *, const void *))i_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = ip[i].str;
		}

		if (listpos_only)
		{
			Get_Poslist(ip, n, poslist);
		}

		XFREE(ip);
		break;

	case DBREF_LIST:
		ip = (i_rec *)XCALLOC(n, sizeof(i_rec), "ip");

		for (i = 0; i < n; i++)
		{
			ip[i].str = s[i];
			ip[i].data = dbnum(s[i]);
			ip[i].pos = i + 1;
		}

		qsort((void *)ip, n, sizeof(i_rec), (int (*)(const void *, const void *))i_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = ip[i].str;
		}

		if (listpos_only)
		{
			Get_Poslist(ip, n, poslist);
		}

		XFREE(ip);
		break;

	case FLOAT_LIST:
		fp = (f_rec *)XCALLOC(n, sizeof(f_rec), "fp");

		for (i = 0; i < n; i++)
		{
			fp[i].str = s[i];
			fp[i].data = strtod(s[i], (char **)NULL);
			fp[i].pos = i + 1;
		}

		qsort((void *)fp, n, sizeof(f_rec), (int (*)(const void *, const void *))f_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = fp[i].str;
		}

		if (listpos_only)
		{
			Get_Poslist(fp, n, poslist);
		}

		XFREE(fp);
		break;
	}

	return poslist;
}

void handle_sort(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nitems, sort_type, oper, i;
	char *list, **ptrs;
	int *poslist;
	Delim isep, osep;

	/*
     * If we are passed an empty arglist return a null string
     */

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 4, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	oper = Func_Mask(SORT_POS);
	/*
     * Convert the list to an array
     */
	list = XMALLOC(LBUF_SIZE, "list");
	XSTRCPY(list, fargs[0]);
	nitems = list2arr(&ptrs, LBUF_SIZE / 2, list, &isep);
	sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
	poslist = do_asort(ptrs, nitems, sort_type, oper);

	if (oper == SORT_POS)
	{
		for (i = 0; i < nitems; i++)
		{
			if (i > 0)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LTOS(buff, bufc, poslist[i], LBUF_SIZE);
		}
	}
	else
	{
		arr2list(ptrs, nitems, buff, bufc, &osep);
	}

	if (poslist)
	{
		XFREE(poslist);
	}

	XFREE(list);
	XFREE(ptrs);
}

/*
 * ---------------------------------------------------------------------------
 * sortby: Sort using a user-defined function.
 */

int u_comp(const void *s1, const void *s2, char *cbuff, dbref thing, dbref player, dbref cause)
{
	/*
     * Note that this function is for use in conjunction with our own
     * sane_qsort routine, NOT with the standard library qsort!
     */
	char *result, *tbuf, *elems[2], *bp;
	int n;

	if ((mudstate.func_invk_ctr > mudconf.func_invk_lim) || (mudstate.func_nest_lev > mudconf.func_nest_lim) || Too_Much_CPU())
	{
		return 0;
	}

	tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	elems[0] = (char *)s1;
	elems[1] = (char *)s2;
	XSTRCPY(tbuf, cbuff);
	result = bp = XMALLOC(LBUF_SIZE, "result");
	exec(result, &bp, thing, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &tbuf, elems, 2);
	n = (int)strtol(result, (char **)NULL, 10);
	XFREE(result);
	XFREE(tbuf);
	return n;
}

void sane_qsort(void *array[], int left, int right, int (*compare)(const void *, const void *, char *, dbref, dbref, dbref), char *cbuff, dbref thing, dbref player, dbref cause)
{
	/*
     * Andrew Molitor's qsort, which doesn't require transitivity between
     * comparisons (essential for preventing crashes due to boneheads who
     * write comparison functions where a > b doesn't mean b < a).
     */
	int i, last;
	void *tmp;
loop:

	if (left >= right)
	{
		return;
	}
	/*
     * Pick something at random at swap it into the leftmost slot
	 * This is the pivot, we'll put it back in the right spot later
     */
	i = Randomize(1 + (right - left));
	tmp = array[left + i];
	array[left + i] = array[left];
	array[left] = tmp;
	last = left;

	for (i = left + 1; i <= right; i++)
	{
		/*
	 * Walk the array, looking for stuff that's less than our
	 * pivot. If it is, swap it with the next thing along
	 */
		if ((*compare)(array[i], array[left], cbuff, thing, player, cause) < 0)
		{
			last++;

			if (last == i)
			{
				continue;
			}

			tmp = array[last];
			array[last] = array[i];
			array[i] = tmp;
		}
	}
	/*
     * Now we put the pivot back, it's now in the right spot, we never
	 * need to look at it again, trust me.
     */
	tmp = array[last];
	array[last] = array[left];
	array[left] = tmp;
	/*
     * At this point everything underneath the 'last' index is < the
	 * entry at 'last' and everything above it is not < it.
     */
	if ((last - left) < (right - last))
	{
		sane_qsort(array, left, last - 1, compare, cbuff, thing, player, cause);
		left = last + 1;
		goto loop;
	}
	else
	{
		sane_qsort(array, last + 1, right, compare, cbuff, thing, player, cause);
		right = last - 1;
		goto loop;
	}
}

void fun_sortby(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *atext, **ptrs;
	char *list = XMALLOC(LBUF_SIZE, "list");
	Delim isep, osep;
	int nptrs, aflags, alen, anum;
	dbref thing, aowner;
	ATTR *ap;

	if ((nfargs == 0) || !fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4 - 1, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	if (string_prefix(fargs[0], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen((fargs[0]) + 8);
		__xstrcpy(atext, fargs[0] + 8);
		atext[alen] = '\0';
		aowner = player;
		aflags = 0;
	}
	else
	{
		if (parse_attrib(player, fargs[0], &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(fargs[0]);
		}
		if (!ap)
		{
			return;
		}
		atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
		if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
		{
			XFREE(atext);
			return;
		}
	}

	XSTRCPY(list, fargs[1]);
	nptrs = list2arr(&ptrs, LBUF_SIZE / 2, list, &isep);

	if (nptrs > 1)
	{ /* pointless to sort less than 2 elements */
		sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp, atext, thing, player, cause);
	}

	arr2list(ptrs, nptrs, buff, bufc, &osep);
	XFREE(list);
	XFREE(atext);
	XFREE(ptrs);
}

/*
 * ---------------------------------------------------------------------------
 * handle_sets: Set management: SETUNION, SETDIFF, SETINTER. Also LUNION,
 * LDIFF, LINTER: Same thing, but takes a sort type like sort() does. There's
 * an unavoidable PennMUSH conflict, as setunion() and friends have a 4th-arg
 * output delimiter in TM3, but the 4th arg is used for the sort type in
 * PennMUSH. Also, adding the sort type as a fifth arg for setunion(), etc.
 * would be confusing, since the last two args are, by convention,
 * delimiters. So we add new funcs.
 */

#define NUMCMP(f1, f2) \
	((f1 > f2) ? 1 : ((f1 < f2) ? -1 : 0))

#define GENCMP(x1, x2, s) \
	((s == ALPHANUM_LIST) ? strcmp(ptrs1[x1], ptrs2[x2]) : ((s == NOCASE_LIST) ? strcasecmp(ptrs1[x1], ptrs2[x2]) : ((s == FLOAT_LIST) ? NUMCMP(fp1[x1], fp2[x2]) : NUMCMP(ip1[x1], ip2[x2]))))

void handle_sets(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	int oper, type_arg;
	char *list1, *list2, *oldp, *bb_p;
	char **ptrs1, **ptrs2;
	int i1, i2, n1, n2, val, sort_type;
	int *ip1, *ip2;
	double *fp1, *fp2;
	oper = Func_Mask(SET_OPER);
	type_arg = Func_Mask(SET_TYPE);

	if (type_arg)
	{
		if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 5, buff, bufc))
		{
			return;
		}
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
		{
			return;
		}
		if (nfargs < 5)
		{
			XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
		}
		else
		{
			if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
			{
				return;
			}
		}
	}
	else
	{
		if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
		{
			return;
		}
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
		{
			return;
		}
		if (nfargs < 4)
		{
			XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
		}
		else
		{
			if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
			{
				return;
			}
		}
	}

	list1 = XMALLOC(LBUF_SIZE, "list1");
	XSTRCPY(list1, fargs[0]);
	n1 = list2arr(&ptrs1, LBUF_SIZE, list1, &isep);
	list2 = XMALLOC(LBUF_SIZE, "list2");
	XSTRCPY(list2, fargs[1]);
	n2 = list2arr(&ptrs2, LBUF_SIZE, list2, &isep);

	if (type_arg)
	{
		if (*fargs[0])
		{
			sort_type = get_list_type(fargs, nfargs, 3, ptrs1, n1);
		}
		else
		{
			sort_type = get_list_type(fargs, nfargs, 3, ptrs2, n2);
		}
	}
	else
	{
		sort_type = ALPHANUM_LIST;
	}

	do_asort(ptrs1, n1, sort_type, SORT_ITEMS);
	do_asort(ptrs2, n2, sort_type, SORT_ITEMS);
	/*
     * This conversion is inefficient, since it's already happened once
     * in do_asort().
     */
	ip1 = ip2 = NULL;
	fp1 = fp2 = NULL;

	if (sort_type == NUMERIC_LIST)
	{
		ip1 = (int *)XCALLOC(n1, sizeof(int), "ip1");
		ip2 = (int *)XCALLOC(n2, sizeof(int), "ip2");

		for (val = 0; val < n1; val++)
		{
			ip1[val] = (int)strtol(ptrs1[val], (char **)NULL, 10);
		}

		for (val = 0; val < n2; val++)
		{
			ip2[val] = (int)strtol(ptrs2[val], (char **)NULL, 10);
		}
	}
	else if (sort_type == DBREF_LIST)
	{
		ip1 = (int *)XCALLOC(n1, sizeof(int), "ip1");
		ip2 = (int *)XCALLOC(n2, sizeof(int), "ip2");

		for (val = 0; val < n1; val++)
		{
			ip1[val] = dbnum(ptrs1[val]);
		}

		for (val = 0; val < n2; val++)
		{
			ip2[val] = dbnum(ptrs2[val]);
		}
	}
	else if (sort_type == FLOAT_LIST)
	{
		fp1 = (double *)XCALLOC(n1, sizeof(double), "fp1");
		fp2 = (double *)XCALLOC(n2, sizeof(double), "fp2");

		for (val = 0; val < n1; val++)
		{
			fp1[val] = strtod(ptrs1[val], (char **)NULL);
		}

		for (val = 0; val < n2; val++)
		{
			fp2[val] = strtod(ptrs2[val], (char **)NULL);
		}
	}

	i1 = i2 = 0;
	bb_p = oldp = *bufc;
	**bufc = '\0';

	switch (oper)
	{
	case SET_UNION: /* Copy elements common to both lists */

		/*
	 * Handle case of two identical single-element lists
	 */
		if ((n1 == 1) && (n2 == 1) && (!strcmp(ptrs1[0], ptrs2[0])))
		{
			SAFE_LB_STR(ptrs1[0], buff, bufc);
			break;
		}

		/*
	 * Process until one list is empty
	 */

		while ((i1 < n1) && (i2 < n2))
		{
			/*
	     * Skip over duplicates
	     */
			if ((i1 > 0) || (i2 > 0))
			{
				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
				{
					i2++;
				}
			}

			/*
	     * Compare and copy
	     */

			if ((i1 < n1) && (i2 < n2))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				oldp = *bufc;

				if (GENCMP(i1, i2, sort_type) < 0)
				{
					SAFE_LB_STR(ptrs1[i1], buff, bufc);
					i1++;
				}
				else
				{
					SAFE_LB_STR(ptrs2[i2], buff, bufc);
					i2++;
				}

				**bufc = '\0';
			}
		}

		/*
	 * Copy rest of remaining list, stripping duplicates
	 */

		for (; i1 < n1; i1++)
		{
			if (strcmp(oldp, ptrs1[i1]))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				oldp = *bufc;
				SAFE_LB_STR(ptrs1[i1], buff, bufc);
				**bufc = '\0';
			}
		}

		for (; i2 < n2; i2++)
		{
			if (strcmp(oldp, ptrs2[i2]))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				oldp = *bufc;
				SAFE_LB_STR(ptrs2[i2], buff, bufc);
				**bufc = '\0';
			}
		}

		break;

	case SET_INTERSECT: /* Copy elements not in both lists */
		while ((i1 < n1) && (i2 < n2))
		{
			val = GENCMP(i1, i2, sort_type);

			if (!val)
			{
				/*
		 * Got a match, copy it
		 */
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				oldp = *bufc;
				SAFE_LB_STR(ptrs1[i1], buff, bufc);
				i1++;
				i2++;

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
				{
					i2++;
				}
			}
			else if (val < 0)
			{
				i1++;
			}
			else
			{
				i2++;
			}
		}

		break;

	case SET_DIFF: /* Copy elements unique to list1 */
		while ((i1 < n1) && (i2 < n2))
		{
			val = GENCMP(i1, i2, sort_type);

			if (!val)
			{
				/*
		 * Got a match, increment pointers
		 */
				oldp = ptrs1[i1];

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
				{
					i2++;
				}
			}
			else if (val < 0)
			{
				/*
		 * Item in list1 not in list2, copy
		 */
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				SAFE_LB_STR(ptrs1[i1], buff, bufc);
				oldp = ptrs1[i1];
				i1++;

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
				{
					i1++;
				}
			}
			else
			{
				/*
		 * Item in list2 but not in list1, discard
		 */
				oldp = ptrs2[i2];
				i2++;

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
				{
					i2++;
				}
			}
		}

		/*
	 * Copy remainder of list1
	 */

		while (i1 < n1)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LB_STR(ptrs1[i1], buff, bufc);
			oldp = ptrs1[i1];
			i1++;

			while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
			{
				i1++;
			}
		}
	}

	//XFREE(plist1);
	//XFREE(plist2);

	if ((sort_type == NUMERIC_LIST) || (sort_type == DBREF_LIST))
	{
		XFREE(ip1);
		XFREE(ip2);
	}
	else if (sort_type == FLOAT_LIST)
	{
		XFREE(fp1);
		XFREE(fp2);
	}

	XFREE(ptrs1);
	XFREE(ptrs2);
}

/*---------------------------------------------------------------------------
 * Format a list into columns.
 */

void fun_columns(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	unsigned int spaces, number, ansinumber, striplen;
	unsigned int count, i, indent = 0;
	int isansi = 0, rturn = 1;
	char *p, *q, *buf, *objstring, *cp, *cr = NULL;
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	number = (int)strtol(fargs[1], (char **)NULL, 10);
	indent = (int)strtol(fargs[3], (char **)NULL, 10);

	if (indent > 77)
	{ /* unsigned int, always a positive number */
		indent = 1;
	}

	/*
     * Must check number separately, since number + indent can result in
     * an integer overflow.
     */

	if ((number < 1) || (number > 77) || ((unsigned int)(number + indent) > 78))
	{
		SAFE_LB_STR("#-1 OUT OF RANGE", buff, bufc);
		return;
	}

	cp = trim_space_sep(fargs[0], &isep);

	if (!*cp)
	{
		return;
	}

	for (i = 0; i < indent; i++)
	{
		SAFE_LB_CHR(' ', buff, bufc);
	}

	buf = XMALLOC(LBUF_SIZE, "buf");

	while (cp)
	{
		objstring = split_token(&cp, &isep);
		ansinumber = number;
		striplen = strip_ansi_len(objstring);

		if (ansinumber > striplen)
		{
			ansinumber = striplen;
		}

		p = objstring;
		q = buf;
		count = 0;

		while (p && *p)
		{
			if (count == number)
			{
				break;
			}

			if (*p == ESC_CHAR)
			{
				/*
		 * Start of ANSI code. Skip to end.
		 */
				isansi = 1;

				while (*p && !isalpha(*p))
				{
					*q++ = *p++;
				}

				if (*p)
				{
					*q++ = *p++;
				}
			}
			else
			{
				*q++ = *p++;
				count++;
			}
		}

		if (isansi)
		{
			SAFE_ANSI_NORMAL(buf, &q);
		}

		*q = '\0';
		isansi = 0;
		SAFE_LB_STR(buf, buff, bufc);

		if (striplen < number)
		{
			/*
	     * We only need spaces if we need to pad out.
	     * Sanitize the number of spaces, too.
	     */
			spaces = number - striplen;

			if (spaces > LBUF_SIZE)
			{
				spaces = LBUF_SIZE;
			}

			for (i = 0; i < spaces; i++)
			{
				SAFE_LB_CHR(' ', buff, bufc);
			}
		}

		if (!(rturn % (int)((78 - indent) / number)))
		{
			SAFE_CRLF(buff, bufc);
			cr = *bufc;

			for (i = 0; i < indent; i++)
			{
				SAFE_LB_CHR(' ', buff, bufc);
			}
		}
		else
		{
			cr = NULL;
		}

		rturn++;
	}

	if (cr)
	{
		*bufc = cr;
		**bufc = '\0';
	}
	else
	{
		SAFE_CRLF(buff, bufc);
	}

	XFREE(buf);
}

/*---------------------------------------------------------------------------
 * fun_table: Turn a list into a table.
 *   table(<list>,<field width>,<line length>,<list delim>,<field sep>,<pad>)
 *     Only the <list> parameter is mandatory.
 *   tables(<list>,<field widths>,<lead str>,<trail str>,
 *          <list delim>,<field sep str>,<pad>)
 *     Only the <list> and <field widths> parameters are mandatory.
 *
 * There are a couple of PennMUSH incompatibilities. The handling here is
 * more complex and probably more desirable behavior. The issues are:
 *   - ANSI states are preserved even if a word is truncated. Thus, the
 *     next word will start with the correct color.
 *   - ANSI does not bleed into the padding or field separators.
 *   - Having a '%r' embedded in the list will start a new set of columns.
 *     This allows a series of %r-separated lists to be table-ified
 *     correctly, and doesn't mess up the character count.
 */

void tables_helper(char *list, int *last_state, int n_cols, int col_widths[], char *lead_str, char *trail_str, const Delim *list_sep, const Delim *field_sep, const Delim *pad_char, char *buff, char **bufc, int just)
{
	int i = 0, nwords = 0, nstates = 0, cpos = 0, wcount = 0, over = 0, ansi_state = 0;
	int max = 0, nleft = 0, lead_chrs = 0, lens[LBUF_SIZE / 2], states[LBUF_SIZE / 2 + 1];
	char *s = NULL, **words = NULL, *buf = NULL;
	/*
     * Split apart the list. We need to find the length of each
     * de-ansified word, as well as keep track of the state of each word.
     * Overly-long words eventually get truncated, but the correct ANSI
     * state is preserved nonetheless.
     */
	nstates = list2ansi(states, last_state, LBUF_SIZE / 2, list, list_sep);
	nwords = list2arr(&words, LBUF_SIZE / 2, list, list_sep);

	if (nstates != nwords)
	{
		SAFE_SPRINTF(buff, bufc, "#-1 STATE/WORD COUNT OFF: %d/%d", nstates, nwords);
		XFREE(words);
		return;
	}

	for (i = 0; i < nwords; i++)
	{
		lens[i] = strip_ansi_len(words[i]);
	}

	over = wcount = 0;

	while ((wcount < nwords) && !over)
	{
		/*
	 * Beginning of new line. Insert newline if this isn't the
	 * first thing we're writing. Write left margin, if
	 * appropriate.
	 */
		if (wcount != 0)
		{
			SAFE_CRLF(buff, bufc);
		}

		if (lead_str)
		{
			over = SAFE_LB_STR(lead_str, buff, bufc);
		}

		/*
	 * Do each column in the line.
	 */

		for (cpos = 0; (cpos < n_cols) && (wcount < nwords) && !over; cpos++, wcount++)
		{
			/*
	     * Write leading padding if we need it.
	     */
			if (just == JUST_RIGHT)
			{
				nleft = col_widths[cpos] - lens[wcount];

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, pad_char->str[0], nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}
			else if (just == JUST_CENTER)
			{
				lead_chrs = (int)((col_widths[cpos] / 2) - (lens[wcount] / 2) + .5);

				if (lead_chrs > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					lead_chrs = (lead_chrs > max) ? max : lead_chrs;
					XMEMSET(*bufc, pad_char->str[0], lead_chrs);
					*bufc += lead_chrs;
					**bufc = '\0';
				}
			}

			/*
	     * If we had a previous state, we have to write it.
	     */
			buf = ansi_transition_esccode(ANST_NONE, states[wcount]);
			SAFE_LB_STR(buf, buff, bufc);
			XFREE(buf);

			/*
	     * Copy in the word.
	     */

			if (lens[wcount] <= col_widths[cpos])
			{
				over = SAFE_LB_STR(words[wcount], buff, bufc);
				buf = ansi_transition_esccode(states[wcount + 1], ANST_NONE);
				SAFE_LB_STR(buf, buff, bufc);
				XFREE(buf);
			}
			else
			{
				/*
		 * Bleah. We have a string that's too long.
		 * Truncate it. Write an ANSI normal at the
		 * end at the end if we need one (we'll
		 * restore the correct ANSI code with the
		 * next word, if need be).
		 */
				ansi_state = states[wcount];

				for (s = words[wcount], i = 0; *s && (i < col_widths[cpos]);)
				{
					if (*s == ESC_CHAR)
					{
						do
						{
							int ansi_mask = 0;
							int ansi_diff = 0;
							unsigned int param_val = 0;
							++(s);
							if (*(s) == ANSI_CSI)
							{
								while ((*(++(s)) & 0xf0) == 0x30)
								{
									if (*(s) < 0x3a)
									{
										param_val <<= 1;
										param_val += (param_val << 2) + (*(s)&0x0f);
									}
									else
									{
										if (param_val < I_ANSI_LIM)
										{
											ansi_mask |= ansiBitsMask(param_val);
											ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
										}
										param_val = 0;
									}
								}
							}
							while ((*(s)&0xf0) == 0x20)
							{
								++(s);
							}
							if (*(s) == ANSI_END)
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
								++(s);
							}
							else if (*(s))
							{
								++(s);
							}
						} while (0);
					}
					else
					{
						s++;
						i++;
					}
				}

				SAFE_STRNCAT(buff, bufc, words[wcount], s - words[wcount], LBUF_SIZE);
				buf = ansi_transition_esccode(ansi_state, ANST_NONE);
				SAFE_LB_STR(buf, buff, bufc);
				XFREE(buf);
			}

			/*
	     * Writing trailing padding if we need it.
	     */

			if (just & JUST_LEFT)
			{
				nleft = col_widths[cpos] - lens[wcount];

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, pad_char->str[0], nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}
			else if (just & JUST_CENTER)
			{
				nleft = col_widths[cpos] - lead_chrs - lens[wcount];

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, pad_char->str[0], nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}

			/*
	     * Insert the field separator if this isn't the last
	     * column AND this is not the very last word in the
	     * list.
	     */

			if ((cpos < n_cols - 1) && (wcount < nwords - 1))
			{
				print_separator(field_sep, buff, bufc);
			}
		}

		if (!over && trail_str)
		{
			/*
	     * If we didn't get enough columns to fill out a
	     * line, and this is the last line, then we have to
	     * pad it out.
	     */
			if ((wcount == nwords) && ((nleft = nwords % n_cols) > 0))
			{
				for (cpos = nleft; (cpos < n_cols) && !over; cpos++)
				{
					print_separator(field_sep, buff, bufc);

					if (col_widths[cpos] > 0)
					{
						max = LBUF_SIZE - 1 - (*bufc - buff);
						col_widths[cpos] = (col_widths[cpos] > max) ? max : col_widths[cpos];
						XMEMSET(*bufc, pad_char->str[0], col_widths[cpos]);
						*bufc += col_widths[cpos];
						**bufc = '\0';
					}
				}
			}

			/*
	     * Write the right margin.
	     */
			over = SAFE_LB_STR(trail_str, buff, bufc);
		}
	}

	/*
     * Save the ANSI state of the last word.
     */
	*last_state = states[nstates - 1];
	/*
     * Clean up.
     */
	XFREE(words);
}

void perform_tables(dbref player __attribute__((unused)), char *list, int n_cols, int col_widths[], char *lead_str, char *trail_str, const Delim *list_sep, const Delim *field_sep, const Delim *pad_char, char *buff, char **bufc, int just)
{
	char *p, *savep, *bb_p;
	int ansi_state = ANST_NONE;

	if (!list || !*list)
	{
		return;
	}

	bb_p = *bufc;
	savep = list;
	p = strchr(list, '\r');

	while (p)
	{
		*p = '\0';

		if (*bufc != bb_p)
		{
			SAFE_CRLF(buff, bufc);
		}

		tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str, list_sep, field_sep, pad_char, buff, bufc, just);
		savep = p + 2; /* must skip '\n' too */
		p = strchr(savep, '\r');
	}

	if (*bufc != bb_p)
	{
		SAFE_CRLF(buff, bufc);
	}

	tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str, list_sep, field_sep, pad_char, buff, bufc, just);
}

void process_tables(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int just;
	int i, num, n_columns, *col_widths;
	Delim list_sep, field_sep, pad_char;
	char **widths;
	just = Func_Mask(JUST_TYPE);

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 7, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &list_sep, DELIM_STRING))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &field_sep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 7, &pad_char, 0))
	{
		return;
	}

	n_columns = list2arr(&widths, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);

	if (n_columns < 1)
	{
		XFREE(widths);
		return;
	}

	col_widths = (int *)XCALLOC(n_columns, sizeof(int), "col_widths");

	for (i = 0; i < n_columns; i++)
	{
		num = (int)strtol(widths[i], (char **)NULL, 10);
		col_widths[i] = (num < 1) ? 1 : num;
	}

	perform_tables(player, fargs[0], n_columns, col_widths, ((nfargs > 2) && *fargs[2]) ? fargs[2] : NULL, ((nfargs > 3) && *fargs[3]) ? fargs[3] : NULL, &list_sep, &field_sep, &pad_char, buff, bufc, just);
	XFREE(col_widths);
	XFREE(widths);
}

void fun_table(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int line_length = 78;
	int field_width = 10;
	int just = JUST_LEFT;
	int i, field_sep_width, n_columns, *col_widths;
	char *p;
	Delim list_sep, field_sep, pad_char;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 6, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &list_sep, DELIM_STRING))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &field_sep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &pad_char, 0))
	{
		return;
	}

	/*
     * Get line length and column width. All columns are the same width.
     * Calculate what we need to.
     */

	if (nfargs > 2)
	{
		line_length = (int)strtol(fargs[2], (char **)NULL, 10);

		if (line_length < 2)
		{
			line_length = 2;
		}
	}

	if (nfargs > 1)
	{
		p = fargs[1];

		switch (*p)
		{
		case '<':
			just = JUST_LEFT;
			p++;
			break;

		case '>':
			just = JUST_RIGHT;
			p++;
			break;

		case '-':
			just = JUST_CENTER;
			p++;
			break;
		}

		field_width = (int)strtol(p, (char **)NULL, 10);

		if (field_width < 1)
		{
			field_width = 1;
		}
		else if (field_width > LBUF_SIZE - 1)
		{
			field_width = LBUF_SIZE - 1;
		}
	}

	if (field_width >= line_length)
	{
		field_width = line_length - 1;
	}

	if (field_sep.len == 1)
	{
		switch (field_sep.str[0])
		{
		case '\r':
		case '\0':
		case '\n':
		case '\a':
			field_sep_width = 0;
			break;

		default:
			field_sep_width = 1;
			break;
		}
	}
	else
	{
		field_sep_width = strip_ansi_len(field_sep.str);
	}

	n_columns = (int)(line_length / (field_width + field_sep_width));
	col_widths = (int *)XCALLOC(n_columns, sizeof(int), "col_widths");

	for (i = 0; i < n_columns; i++)
	{
		col_widths[i] = field_width;
	}

	perform_tables(player, fargs[0], n_columns, col_widths, NULL, NULL, &list_sep, &field_sep, &pad_char, buff, bufc, just);
	XFREE(col_widths);
}

/*
 * ---------------------------------------------------------------------------
 * fun_elements: given a list of numbers, get corresponding elements from the
 * list.  elements(ack bar eep foof yay,2 4) ==> bar foof The function takes
 * a separator, but the separator only applies to the first list.
 */

void fun_elements(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nwords, cur, start, end, stepn;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	char *end_p, *step_p;
	Delim isep, osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	oldp = *bufc;
	/*
     * Turn the first list into an array.
     */
	wordlist = XMALLOC(LBUF_SIZE, "wordlist");
	XSTRCPY(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);
	s = Eat_Spaces(fargs[1]);

	/*
     * Go through the second list, grabbing the numbers and finding the
     * corresponding elements.
     */

	do
	{
		r = split_token(&s, &SPACE_DELIM);

		if ((end_p = strchr(r, ':')) == NULL)
		{
			/*
	     * Just a number. If negative, count back from end of
	     * list.
	     */
			cur = (int)strtol(r, (char **)NULL, 10);

			if (cur < 0)
			{
				cur += nwords;
			}
			else
			{
				cur--;
			}

			if ((cur >= 0) && (cur < nwords) && ptrs[cur])
			{
				if (oldp != *bufc)
				{
					print_separator(&osep, buff, bufc);
				}

				SAFE_LB_STR(ptrs[cur], buff, bufc);
			}
		}
		else
		{
			/*
	     * Support Python-style slicing syntax:
	     * <start>:<end>:<step> If start is empty, start from
	     * element 0. If start is positive, start from that
	     * number. If start is negative, start from that
	     * number back from the end (-1 is the last item, -2
	     * is second to last, etc.) If end is empty, stop at
	     * the last element. If end is positive, stop there.
	     * If end is negative, skip the last end elements.
	     * Note that Python numbers arrays from 0, and we
	     * number word lists from 1, so the syntax isn't
	     * Python-identical!
	     */
			/*
	     * r points to our start
	     */
			*end_p++ = '\0';

			if ((step_p = strchr(end_p, ':')) != NULL)
			{
				*step_p++ = '\0';
			}

			if (!step_p)
			{
				stepn = 1;
			}
			else
			{
				stepn = (int)strtol(step_p, (char **)NULL, 10);
			}

			if (stepn > 0)
			{
				if (*r == '\0')
				{
					/*
		     * Empty start
		     */
					start = 0;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/*
		     * Empty end
		     */
					end = nwords;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur) : (cur);
				}

				if (start <= end)
				{
					for (cur = start; cur < end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords) && ptrs[cur])
						{
							if (oldp != *bufc)
							{
								print_separator(&osep, buff, bufc);
							}

							SAFE_LB_STR(ptrs[cur], buff, bufc);
						}
					}
				}
			}
			else if (stepn < 0)
			{
				if (*r == '\0')
				{
					/*
		     * Empty start, goes to the LAST
		     * element
		     */
					start = nwords - 1;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/*
		     * Empty end
		     */
					end = 0;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur - 1) : (cur - 1);
				}

				if (start >= end)
				{
					for (cur = start; cur >= end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords) && ptrs[cur])
						{
							if (oldp != *bufc)
							{
								print_separator(&osep, buff, bufc);
							}

							SAFE_LB_STR(ptrs[cur], buff, bufc);
						}
					}
				}
			}
		}
	} while (s);

	XFREE(wordlist);
	XFREE(ptrs);
}

/*
 * ---------------------------------------------------------------------------
 * fun_exclude: Return the elements of a list EXCEPT the numbered items.
 */

void fun_exclude(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nwords, cur, start, end, stepn;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	char *end_p, *step_p;
	int *mapper;
	Delim isep, osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	oldp = *bufc;
	/*
     * Turn the first list into an array.
     */
	wordlist = XMALLOC(LBUF_SIZE, "wordlist");
	XSTRCPY(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);
	s = Eat_Spaces(fargs[1]);
	/*
     * Go through the second list, grabbing the numbers and mapping the
     * corresponding elements.
     */
	mapper = (int *)XCALLOC(nwords, sizeof(int), "mapper");

	do
	{
		r = split_token(&s, &SPACE_DELIM);

		if ((end_p = strchr(r, ':')) == NULL)
		{
			/*
	     * Just a number. If negative, count back from end of
	     * list.
	     */
			cur = (int)strtol(r, (char **)NULL, 10);

			if (cur < 0)
			{
				cur += nwords;
			}
			else
			{
				cur--;
			}

			if ((cur >= 0) && (cur < nwords))
			{
				mapper[cur] = 1;
			}
		}
		else
		{
			/*
	     * Slicing syntax
	     */
			/*
	     * r points to our start
	     */
			*end_p++ = '\0';

			if ((step_p = strchr(end_p, ':')) != NULL)
			{
				*step_p++ = '\0';
			}

			if (!step_p)
			{
				stepn = 1;
			}
			else
			{
				stepn = (int)strtol(step_p, (char **)NULL, 10);
			}

			if (stepn > 0)
			{
				if (*r == '\0')
				{
					/*
		     * Empty start
		     */
					start = 0;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/*
		     * Empty end
		     */
					end = nwords;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur) : (cur);
				}

				if (start <= end)
				{
					for (cur = start; cur < end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords))
						{
							mapper[cur] = 1;
						}
					}
				}
			}
			else if (stepn < 0)
			{
				if (*r == '\0')
				{
					/*
		     * Empty start, goes to the LAST
		     * element
		     */
					start = nwords - 1;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/*
		     * Empty end
		     */
					end = 0;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur - 1) : (cur - 1);
				}

				if (start >= end)
				{
					for (cur = start; cur >= end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords))
						{
							mapper[cur] = 1;
						}
					}
				}
			}
		}
	} while (s);

	for (cur = 0; cur < nwords; cur++)
	{
		if (!mapper[cur])
		{
			if (oldp != *bufc)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LB_STR(ptrs[cur], buff, bufc);
		}
	}

	XFREE(wordlist);
	XFREE(ptrs);
	XFREE(mapper);
}

/*
 * ---------------------------------------------------------------------------
 * fun_grab: a combination of extract() and match(), sortof. We grab the
 * single element that we match.
 *
 * grab(Test:1 Ack:2 Foof:3,*:2)    => Ack:2 grab(Test-1+Ack-2+Foof-3,*o*,+)  =>
 * Ack:2
 *
 * fun_graball: Ditto, but like matchall() rather than match(). We grab all the
 * elements that match, and we can take an output delimiter.
 */

void fun_grab(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *r, *s;
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	};

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	/*
     * Walk the wordstring, until we find the word we want.
     */
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			SAFE_LB_STR(r, buff, bufc);
			return;
		}
	} while (s);
}

void fun_graball(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *r, *s, *bb_p;
	Delim isep, osep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	s = trim_space_sep(fargs[0], &isep);
	bb_p = *bufc;

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			SAFE_LB_STR(r, buff, bufc);
		}
	} while (s);
}

/*
 * ---------------------------------------------------------------------------
 * fun_shuffle: randomize order of words in a list.
 */

/* Borrowed from PennMUSH 1.50 */
void swap(char **p, char **q)
{
	/*
     * swaps two points to strings
     */
	char *temp;
	temp = *p;
	*p = *q;
	*q = temp;
}

void fun_shuffle(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **words;
	int n, i, j;
	Delim isep, osep;

	if (!nfargs || !fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 3, buff, bufc))
	{
		return;
	}
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}
	if (nfargs < 3)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	n = list2arr(&words, LBUF_SIZE, fargs[0], &isep);

	for (i = 0; i < n; i++)
	{
		j = random_range(i, n - 1);
		swap(&words[i], &words[j]);
	}

	arr2list(words, n, buff, bufc, &osep);
	XFREE(words);
}

/*
 * ---------------------------------------------------------------------------
 * ledit(<list of words>,<old words>,<new words>[,<delim>[,<output delim>]])
 * If a <word> in <list of words> is in <old words>, replace it with the
 * corresponding word from <new words>. This is basically a mass-edit. This
 * is an EXACT, not a case-insensitive or wildcarded, match.
 */

void fun_ledit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	char *old_list, *new_list;
	int nptrs_old, nptrs_new;
	char **ptrs_old, **ptrs_new;
	char *r, *s, *bb_p;
	int i;
	int got_it;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 5, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &isep, DELIM_STRING))
	{
		return;
	}

	if (nfargs < 5)
	{
		XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	old_list = XMALLOC(LBUF_SIZE, "old_list");
	new_list = XMALLOC(LBUF_SIZE, "new_list");
	XSTRCPY(old_list, fargs[1]);
	XSTRCPY(new_list, fargs[2]);
	nptrs_old = list2arr(&ptrs_old, LBUF_SIZE / 2, old_list, &isep);
	nptrs_new = list2arr(&ptrs_new, LBUF_SIZE / 2, new_list, &isep);
	/*
     * Iterate through the words.
     */
	bb_p = *bufc;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		r = split_token(&s, &isep);

		for (i = 0, got_it = 0; i < nptrs_old; i++)
		{
			if (!strcmp(r, ptrs_old[i]))
			{
				got_it = 1;

				if ((i < nptrs_new) && *ptrs_new[i])
				{
					/*
		     * If we specify more old words than
		     * we have new words, we assume we
		     * want to just nullify.
		     */
					SAFE_LB_STR(ptrs_new[i], buff, bufc);
				}

				break;
			}
		}

		if (!got_it)
		{
			SAFE_LB_STR(r, buff, bufc);
		}
	} while (s);

	XFREE(old_list);
	XFREE(new_list);
	XFREE(ptrs_old);
	XFREE(ptrs_new);
}

/*
 * ---------------------------------------------------------------------------
 * fun_itemize: Turn a list into a punctuated list.
 */

void fun_itemize(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	int n_elems, i;
	char *conj_str, **elems;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 4, buff, bufc))
	{
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	if (nfargs < 3)
	{
		conj_str = (char *)"and";
	}
	else
	{
		conj_str = fargs[2];
	}

	if (nfargs < 4)
	{
		osep.str[0] = ',';
		osep.len = 1;
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);

	if (n_elems == 1)
	{
		SAFE_LB_STR(elems[0], buff, bufc);
	}
	else if (n_elems == 2)
	{
		SAFE_LB_STR(elems[0], buff, bufc);

		if (*conj_str)
		{
			SAFE_LB_CHR(' ', buff, bufc);
			SAFE_LB_STR(conj_str, buff, bufc);
		}

		SAFE_LB_CHR(' ', buff, bufc);
		SAFE_LB_STR(elems[1], buff, bufc);
	}
	else
	{
		for (i = 0; i < (n_elems - 1); i++)
		{
			SAFE_LB_STR(elems[i], buff, bufc);
			print_separator(&osep, buff, bufc);
			SAFE_LB_CHR(' ', buff, bufc);
		}

		if (*conj_str)
		{
			SAFE_LB_STR(conj_str, buff, bufc);
			SAFE_LB_CHR(' ', buff, bufc);
		}

		SAFE_LB_STR(elems[i], buff, bufc);
	}

	XFREE(elems);
}

/*
 * ---------------------------------------------------------------------------
 * fun_choose: Weighted random choice from a list. choose(<list of
 * items>,<list of weights>,<input delim>)
 */

void fun_choose(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;
	char **elems, **weights;
	int i, num, n_elems, n_weights, *ip;
	int sum = 0;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	};

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	n_weights = list2arr(&weights, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);

	if (n_elems != n_weights)
	{
		SAFE_LB_STR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
		XFREE(elems);
		XFREE(weights);
		return;
	}

	/*
     * Store the breakpoints, not the choose weights themselves.
     */
	ip = (int *)XCALLOC(n_weights, sizeof(int), "ip");

	for (i = 0; i < n_weights; i++)
	{
		num = (int)strtol(weights[i], (char **)NULL, 10);

		if (num < 0)
		{
			num = 0;
		}

		if (num == 0)
		{
			ip[i] = 0;
		}
		else
		{
			sum += num;
			ip[i] = sum;
		}
	}

	num = (int)Randomize(sum);

	for (i = 0; i < n_weights; i++)
	{
		if ((ip[i] != 0) && (num < ip[i]))
		{
			SAFE_LB_STR(elems[i], buff, bufc);
			break;
		}
	}

	XFREE(ip);
	XFREE(elems);
	XFREE(weights);
}

/*
 * ---------------------------------------------------------------------------
 * fun_group:  group(<list>, <number of groups>, <idelim>, <odelim>,
 * <gdelim>) Sort a list by numerical-size group, i.e., take every Nth
 * element. Useful for passing to a column-type function where you want the
 * list to go down rather than across, for instance.
 */

void fun_group(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p, **elems;
	Delim isep, osep, gsep;
	int n_elems, n_groups, i, j;
	/*
     * Separator checking is weird in this, since we can delimit by
     * group, too, as well as the element delimiter. The group delimiter
     * defaults to the output delimiter.
     */
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 5, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	if (nfargs < 4)
	{
		XMEMCPY(&osep, &isep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	if (nfargs < 5)
	{
		XMEMCPY(&gsep, &osep, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&osep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &gsep, DELIM_NULL | DELIM_CRLF | DELIM_STRING))
		{
			return;
		}
	}

	/*
     * Go do it, unless the group size doesn't make sense.
     */
	n_groups = (int)strtol(fargs[1], (char **)NULL, 10);
	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);

	if (n_groups < 2)
	{
		arr2list(elems, n_elems, buff, bufc, &osep);
		XFREE(elems);
		return;
	}

	if (n_groups >= n_elems)
	{
		arr2list(elems, n_elems, buff, bufc, &gsep);
		XFREE(elems);
		return;
	}

	bb_p = *bufc;

	for (i = 0; i < n_groups; i++)
	{
		for (j = 0; i + j < n_elems; j += n_groups)
		{
			if (*bufc != bb_p)
			{
				if (j == 0)
				{
					print_separator(&gsep, buff, bufc);
				}
				else
				{
					print_separator(&osep, buff, bufc);
				}
			}

			SAFE_LB_STR(elems[i + j], buff, bufc);
		}
	}

	XFREE(elems);
}

/*
 * ---------------------------------------------------------------------------
 * fun_tokens: Take a string such as 'this "Joe Bloggs" John' and turn it
 * into an output delim-separated list.
 * tokens(<string>[,<obj>/<attr>][,<open>][,<close>][,<sep>][,<osep>])
 */

void fun_tokens(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *t, *bb_p, *atext, *atextbuf, *objs[1], *str;
	int anum, aflags, alen;
	dbref aowner, thing;
	ATTR *ap;
	Delim omark, cmark, isep, osep;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 6, buff, bufc))
	{
		return;
	}

	if (nfargs < 3)
	{
		omark.str[0] = '"';
		omark.len = 1;
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &omark, DELIM_STRING))
		{
			return;
		}
	}

	if (nfargs < 4)
	{
		XMEMCPY(&cmark, &omark, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&omark)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &cmark, DELIM_STRING))
		{
			return;
		}
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &isep, DELIM_STRING))
	{
		return;
	}

	if (nfargs < 6)
	{
		XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	if ((nfargs > 1) && fargs[1] && *fargs[1])
	{
		if (string_prefix(fargs[1], "#lambda/"))
		{
			thing = player;
			anum = NOTHING;
			ap = NULL;
			atext = XMALLOC(LBUF_SIZE, "lambda.atext");
			alen = strlen((fargs[1]) + 8);
			__xstrcpy(atext, fargs[1] + 8);
			atext[alen] = '\0';
			aowner = player;
			aflags = 0;
		}
		else
		{
			if (parse_attrib(player, fargs[1], &thing, &anum, 0))
			{
				if ((anum == NOTHING) || !(Good_obj(thing)))
					ap = NULL;
				else
					ap = atr_num(anum);
			}
			else
			{
				thing = player;
				ap = atr_str(fargs[1]);
			}
			if (!ap)
			{
				return;
			}
			atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
			if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
			{
				XFREE(atext);
				return;
			}
		}

		atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
	}
	else
	{
		atextbuf = NULL;
	}

	bb_p = *bufc;
	s = trim_space_sep(fargs[0], &isep);

	while (s && *s)
	{
		if ((omark.len == 1) ? (*s == omark.str[0]) : !strncmp(s, omark.str, omark.len))
		{
			/*
	     * Now we're inside quotes. Find the end quote, and
	     * copy the token inside of it. If we run off the end
	     * of the string, we ignore the literal opening
	     * marker that we've skipped.
	     */
			s += omark.len;

			if (*s)
			{
				t = split_token(&s, &cmark);
			}
			else
			{
				break;
			}
		}
		else
		{
			/*
	     * We are at a bare word. Split it off.
	     */
			t = split_token(&s, &isep);
		}

		/*
	 * Pass the token through the transformation function if we
	 * have one, or just copy it, if not.
	 */
		if (t)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			if (!atextbuf)
			{
				SAFE_LB_STR(t, buff, bufc);
			}
			else if ((mudstate.func_invk_ctr < mudconf.func_invk_lim) && !Too_Much_CPU())
			{
				objs[0] = t;
				XMEMCPY(atextbuf, atext, alen);
				atextbuf[alen] = '\0';
				str = atextbuf;
				exec(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 1);
			}
		}

		/*
	 * Skip to start of next token, ignoring input separators.
	 */
		if (s && *s)
		{
			if ((isep.len == 1) && (isep.str[0] == ' '))
			{
				s = trim_space_sep(s, &isep);
			}
			else
			{
				if (isep.len == 1)
				{
					while (*s == isep.str[0])
					{
						s++;
					}
				}
				else
				{
					while (*s && !strncmp(s, isep.str, isep.len))
					{
						s += isep.len;
					}
				}
			}
		}
	}

	if (atextbuf)
	{
		XFREE(atextbuf);
	}
}
