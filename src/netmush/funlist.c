/**
 * @file funlist.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List manipulation built-ins: set operations, joins, sorting, and selection
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

static const ColorState color_normal = {.foreground = {.is_set = ColorStatusReset}, .background = {.is_set = ColorStatusReset}};
static const ColorState color_none = {0};

/**
 * List management utilities.
 *
 */

/**
 * @brief Auto-Detect what kind of list we are dealing with
 *
 * @param ptrs List to check
 * @param nitems Number of items
 * @return int List type (NUMERIC_LIST, FLOAT_LIST, DBREF_LIST, or ALPHANUM_LIST)
 *
 * @details Analyzes the list items to determine the appropriate sort type.
 * Starts by assuming numeric, then checks for floats, DBrefs, or falls back
 * to alphanumeric. DBref detection is based on the first item starting with '#'
 * followed by digits.
 */
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
				/**
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

/**
 * @brief Detect the list type
 *
 * @param fargs Function Arguments
 * @param nfargs Number of arguments
 * @param type_pos Which argument hold the list type
 * @param ptrs List
 * @param nitems Number of items in the list
 * @return int List type (NUMERIC_LIST, FLOAT_LIST, DBREF_LIST, ALPHANUM_LIST, or NOCASE_LIST)
 *
 * @details If type_pos is within bounds, uses the specified type from fargs[type_pos-1]:
 * 'd' for DBREF_LIST, 'n' for NUMERIC_LIST, 'f' for FLOAT_LIST, 'i' for NOCASE_LIST.
 * Otherwise, auto-detects based on the list content.
 */
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

/**
 * @brief Validate function arguments and delimiter for list functions
 * @param func_name Name of the function (for error messages)
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Command's argument count
 * @param min_args Minimum number of arguments
 * @param max_args Maximum number of arguments
 * @param delim_pos Position of delimiter argument (-1 if none, or uses 1-based index)
 * @param delim_flags Delimiter flags (DELIM_STRING, etc.)
 * @param isep Pointer to Delim structure to populate
 * @return int 1 if valid, 0 if error (already printed to buff)
 */
int validate_list_args(const char *func_name, char *buff, char **bufc, dbref player, dbref caller, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs, int min_args, int max_args,
					   int delim_pos, int delim_flags, Delim *isep)
{
	if (!fn_range_check(func_name, nfargs, min_args, max_args, buff, bufc))
	{
		return 0;
	}

	if (delim_pos > 0 && !delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, delim_pos, isep, delim_flags))
	{
		return 0;
	}

	return 1;
}

/**
 * @brief Generate a list of random integers within an inclusive range.
 *
 * Produces @p n_times values between @p r_bot and @p r_top (inclusive),
 * separated by the optional output delimiter. The count is clamped to
 * LBUF_SIZE. When @p r_top == @p r_bot, the constant value repeats; when
 * @p r_top < @p r_bot, the call returns an empty string. A zero or negative
 * @p n_times also yields an empty result.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=lower bound, [1]=upper bound, [2]=count, [3]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_lrand(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim osep;
	int n_times = 0, r_bot = 0, r_top = 0, i = 0;
	double n_range = 0.0;
	unsigned int tmp = 0;
	char *bb_p = NULL;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 4, 4, DELIM_STRING | DELIM_NULL | DELIM_CRLF, &osep))
	{
		return;
	}

	n_times = (int)strtol(fargs[2], (char **)NULL, 10);

	if (n_times < 1)
	{
		return;
	}

	if (n_times > LBUF_SIZE)
	{
		n_times = LBUF_SIZE;
	}

	r_bot = (int)strtol(fargs[0], (char **)NULL, 10);
	r_top = (int)strtol(fargs[1], (char **)NULL, 10);

	if (r_top < r_bot)
	{
		return;
	}
	else if (r_bot == r_top)
	{
		bb_p = *bufc;

		for (i = 0; i < n_times; i++)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELTOS(buff, bufc, r_bot, LBUF_SIZE);
		}

		return;
	}

	n_range = (double)r_top - r_bot + 1;
	bb_p = *bufc;

	for (i = 0; i < n_times; i++)
	{
		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		tmp = (unsigned int)random_range(0, (n_range)-1);

		XSAFELTOS(buff, bufc, r_bot + tmp, LBUF_SIZE);
	}
}

/**
 * @brief Copy delimiter structure (osep = isep)
 *
 * @param dest Destination Delim structure
 * @param src Source Delim structure
 *
 * @details Safely copies the delimiter information from src to dest,
 * handling the variable-length string field.
 */
static inline void copy_delim(Delim *dest, const Delim *src)
{
	if (!dest || !src)
	{
		return;
	}

	size_t copy_len = sizeof(Delim) - MAX_DELIM_LEN + 1 + src->len;
	XMEMCPY(dest, src, copy_len);
}

/**
 * @brief Parse ANSI escape sequences and update full ColorState
 *
 * This function processes ANSI escape sequences (color codes) at the current
 * position in the string, advancing the pointer past the escape sequence and
 * updating the ANSI state accordingly.
 *
 * @param s Pointer to string pointer (will be advanced past ANSI sequences)
 * @param state Pointer to current ColorState (will be updated)
 *
 * Implementation note: This is a static inline function rather than a macro
 * for type safety and maintainability, while preserving performance.
 */
static inline void parse_ansi_escapes(char **s, ColorState *state)
{
	while (**s == C_ANSI_ESC)
	{
		const char *cursor = *s;
		if (ansi_apply_sequence(&cursor, state))
		{
			*s = (char *)cursor;
		}
		else
		{
			++(*s);
		}
	}
}

/**
 * @brief Convert a DBref (#db) to its numerical value (db)
 *
 * @param dbr Text DBref value
 * @return int DBref numerical value
 */
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

/**
 * @brief Count the words in a list using the specified delimiter.
 *
 * Trims leading separators, splits @p fargs[0] on the delimiter (default space
 * when none provided), and writes the resulting count to @p buff. A missing
 * argument returns 0 without error output.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_words(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (nfargs == 0)
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
	{
		return;
	}

	XSAFELTOS(buff, bufc, countwords(fargs[0], &isep), LBUF_SIZE);
}

/**
 * @brief Return the first word from a list using the given delimiter.
 *
 * Trims leading separators, splits @p fargs[0] on the delimiter (default
 * space if none specified), and copies the first element to @p buff. With no
 * arguments, returns an empty string without emitting an error.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_first(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *first;
	Delim isep;

	/**
	 * If we are passed an empty arglist return a null string
	 *
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

	s = trim_space_sep(fargs[0], &isep);
	first = split_token(&s, &isep);

	if (first)
	{
		XSAFELBSTR(first, buff, bufc);
	}
}

/**
 * @brief Return all words except the first, preserving ANSI state.
 *
 * Trims leading separators, consumes the first element (tracking ANSI escape
 * sequences), then outputs any remaining text while restoring the active ANSI
 * state so colors remain correct. With no arguments, returns an empty string
 * without error output.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_rest(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *rest;
	Delim isep;
	ColorState ansi_state = color_none;

	/*
	 * If we are passed an empty arglist return a null string
	 */

	if (nfargs == 0)
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
	{
		return;
	}

	s = trim_space_sep(fargs[0], &isep); /* leading spaces */
	rest = next_token_colorstate(s, &isep, &ansi_state);

	if (rest)
	{
		s = ansi_transition_colorstate(color_normal, ansi_state, ColorTypeTrueColor, false);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
		XSAFELBSTR(rest, buff, bufc);
	}
}

/**
 * @brief Return the last word from a list while preserving ANSI state.
 *
 * Trims leading separators, walks the list using the input delimiter (default
 * space), and copies the final element to @p buff. For single-character
 * delimiters, ANSI escape sequences are tracked so the color state active at
 * the end of the list is reapplied before emitting the last token. For
 * multi-character delimiters, the function scans from the end of the string to
 * locate the final separator. With no arguments, returns an empty string
 * without emitting an error.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_last(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *last, *buf;
	Delim isep;
	ColorState ansi_state = color_none;

	/**
	 * If we are passed an empty arglist return a null string
	 *
	 */
	if (nfargs == 0)
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
	{
		return;
	}

	if (isep.len == 1)
	{
		last = s = trim_space_sep(fargs[0], &isep);

		do
		{
			/**
			 * this is like next_token(), but tracking ansi
			 *
			 */
			parse_ansi_escapes(&s, &ansi_state);

			while (*s && (*s != isep.str[0]))
			{
				++s;
				parse_ansi_escapes(&s, &ansi_state);
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

		buf = ansi_transition_colorstate(color_normal, ansi_state, ColorTypeTrueColor, false);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
		XSAFESTRNCAT(buff, bufc, last, s - last, LBUF_SIZE);
	}
	else
	{
		s = fargs[0];

		/**
		 * Walk backwards through the string to find the separator.
		 * Find the last character, and compare the previous
		 * characters, to find the separator. If we can't find the
		 * last character or we know we're going to fall off the
		 * string, return the original string.
		 *
		 */
		if ((last = strrchr(s, isep.str[isep.len - 1])) == NULL)
		{
			XSAFELBSTR(s, buff, bufc);
			return;
		}

		while (last >= s + isep.len - 1)
		{
			if ((*last == isep.str[isep.len - 1]) && !strncmp(isep.str, last - isep.len + 1, isep.len))
			{
				++last;
				XSAFELBSTR(last, buff, bufc);
				return;
			}

			last--;
		}

		XSAFELBSTR(s, buff, bufc);
	}
}

/**
 * @brief Find the 1-based index of the first list element matching a pattern.
 *
 * Trims leading separators, splits @p fargs[0] on the delimiter (default
 * space if none provided), and compares each token to @p fargs[1] using
 * wildcard matching (`quick_wild`). Returns the index of the first match, or
 * 0 if no elements match. Argument validation errors are written to @p buff by
 * the range and delimiter checks.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=pattern, [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
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

	/**
	 * Check each word individually, returning the word number of the
	 * first one that matches.  If none match, return 0.
	 *
	 */
	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			XSAFELTOS(buff, bufc, wcount, LBUF_SIZE);
			return;
		}

		wcount++;
	} while (s);

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Return the indices of all list elements that match (or don't match) a pattern.
 *
 * Trims leading separators, splits @p fargs[0] on the input delimiter (default
 * space), and tests each token against @p fargs[1] using wildcard matching
 * (`quick_wild`). The IFELSE_FALSE flag (set by `unmatchall`) inverts the
 * match test. Matching indices are joined with the output delimiter: if none
 * is supplied, a space is used (even if the input delimiter differs).
 * Returns an empty string when no elements qualify. Argument validation errors
 * are written to @p buff by the range and delimiter checks.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=pattern, [2]=optional input delimiter, [3]=optional output delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
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
		copy_delim(&osep, &isep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	flag = Func_Flags(fargs);

	/**
	 * SPECIAL CASE: If there's no output delimiter specified, we use a
	 * space, NOT the delimiter given for the list!
	 *
	 */
	if (nfargs < 4)
	{
		osep.str[0] = ' ';
		osep.len = 1;
	}

	old = *bufc;

	/**
	 * Check each word individually, returning the word number of all
	 * that match (or don't match, in the case of unmatchall). If none,
	 * return a null string.
	 *
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

			XSAFELTOS(buff, bufc, wcount, LBUF_SIZE);
		}

		wcount++;
	} while (s);
}

/**
 * @brief Extract a slice of words from a list, with optional output delimiter.
 *
 * Splits @p fargs[0] on the input delimiter (default space). Starting at
 * 1-based position @p fargs[1], copies @p fargs[2] words to @p buff. If
 * @p fargs[4] is given, it is used as the output delimiter; otherwise the
 * input delimiter is reused. When input and output delimiters match, the
 * function copies the substring directly for efficiency; otherwise it rebuilds
 * the slice token by token. If @p start or @p len is less than 1, or the start
 * position is past the end of the list, an empty string is returned. Argument
 * validation errors are written to @p buff by the range and delimiter checks.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=start (1-based), [2]=length, [3]=optional input delimiter, [4]=optional output delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_extract(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int start, len;
	char *r, *s, *t;
	Delim isep, osep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 5, 4, DELIM_STRING, &isep))
	{
		return;
	}

	if (nfargs < 5)
	{
		copy_delim(&osep, &isep);
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

	/**
	 * Skip to the start of the string to save
	 *
	 */
	start--;
	s = trim_space_sep(s, &isep);

	while (start && s)
	{
		s = next_token(s, &isep);
		start--;
	}

	/**
	 * If we ran of the end of the string, return nothing
	 *
	 */
	if (!s || !*s)
	{
		return;
	}

	/**
	 * If our delimiter is the same, we have an easy task. Otherwise we
	 * have to go token by token.
	 *
	 */
	if (!strcmp(isep.str, osep.str))
	{
		/**
		 * Count off the words in the string to save
		 *
		 */
		r = s;
		len--;

		while (len && s)
		{
			s = next_token(s, &isep);
			len--;
		}

		/**
		 * Chop off the rest of the string, if needed
		 *
		 */
		if (s && *s)
		{
			t = split_token(&s, &isep);
		}

		XSAFELBSTR(r, buff, bufc);
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

			XSAFELBSTR(t, buff, bufc);
			len--;
		} while (len && s && *s);
	}
}

/**
 * @brief Extract a substring between occurrences of a single-character separator.
 *
 * Treats @p fargs[1] as a single-character delimiter (default space if empty),
 * and returns the portion of @p fargs[0] starting at the @p fargs[2]-th field
 * and ending before the @p fargs[3]-th delimiter. Leading spaces after the
 * starting delimiter are skipped when the separator is space; trailing spaces
 * before the ending delimiter are trimmed. If @p start or @p end is less than 1,
 * or the start position is past the end of the string, returns an empty string.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=string, [1]=separator char (optional), [2]=start field (1-based), [3]=end field (1-based)
 * @param nfargs Number of function arguments (unused)
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_index(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
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

	/**
	 * move s to point to the start of the item we want
	 *
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

	/**
	 * skip over just spaces
	 *
	 */
	while (s && (*s == ' '))
	{
		s++;
	}

	if (!s || !*s)
	{
		return;
	}

	/**
	 * figure out where to end the string
	 *
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
				XSAFELBSTR(s, buff, bufc);
				return;
			}
			else
			{
				p++;
			}
		}
	}

	/**
	 * if we've gotten this far, we've run off the end of the string
	 *
	 */
	XSAFELBSTR(s, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * ldelete: Remove a word from a string by place
 * ldelete(&lt;list&gt;,&lt;position&gt;[,&lt;separator&gt;])
 *
 * insert: insert a word into a string by place
 * insert(&lt;list&gt;,&lt;position&gt;,&lt;new item&gt; [,&lt;separator&gt;])
 *
 * replace: replace a word into a string by place
 * replace(&lt;list&gt;,&lt;position&gt;,&lt;new item&gt;[,&lt;separator&gt;])
 *
 * lreplace: replace multiple words into a string by places
 * lreplace(&lt;list&gt;,&lt;replacement words&gt;,&lt;positions&gt;[,&lt;isep&gt;,&lt;osep&gt;])
 */

/**
 * @brief Shared worker for ldelete/replace/insert operations on a delimited list.
 *
 * Splits @p str into before/target/after chunks using @p sep, then performs one of:
 * - IF_DELETE: drop the target element and join the rest.
 * - IF_REPLACE: substitute @p word for the target element.
 * - IF_INSERT: insert @p word at the target position (allowing append when past end).
 *
 * If @p str is empty, only IF_INSERT at position 1 produces output. Positions <1
 * emit the original string unchanged. If the requested element is beyond the end
 * (and not an allowed insert-at-end), the original string is returned. Results are
 * written to @p buff via @p bufc.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param str Input list to modify
 * @param el 1-based target position
 * @param word Replacement/insert text (ignored for delete)
 * @param sep Input/output delimiter
 * @param flag Operation selector: IF_DELETE, IF_REPLACE, or IF_INSERT
 */
void do_itemfuns(char *buff, char **bufc, char *str, int el, char *word, const Delim *sep, int flag)
{
	int ct, overrun;
	char *sptr, *iptr, *eptr;

	/**
	 * If passed a null string return an empty string, except that we are
	 * allowed to append to a null string.
	 *
	 */
	if ((!str || !*str) && ((flag != IF_INSERT) || (el != 1)))
	{
		return;
	}

	/**
	 * we can't fiddle with anything before the first position
	 */
	if (el < 1)
	{
		XSAFELBSTR(str, buff, bufc);
		return;
	}

	/**
	 * Split the list up into 'before', 'target', and 'after' chunks
	 * pointed to by sptr, iptr, and eptr respectively.
	 */
	if (el == 1)
	{
		/**
		 * No 'before' portion, just split off element 1
		 *
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
		/**
		 * Break off 'before' portion
		 *
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

		/**
		 * If we didn't make it to the target element, just return
		 * the string.  Insert is allowed to continue if we are
		 * exactly at the end of the string, but replace and delete
		 * are not.
		 *
		 */
		if (!(eptr || ((flag == IF_INSERT) && !overrun)))
		{
			XSAFELBSTR(str, buff, bufc);
			return;
		}

		/**
		 * Split the 'target' word from the 'after' portion.
		 *
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
	case IF_DELETE:
		if (sptr)
		{
			XSAFELBSTR(sptr, buff, bufc);

			if (eptr)
			{
				print_separator(sep, buff, bufc);
			}
		}

		if (eptr)
		{
			XSAFELBSTR(eptr, buff, bufc);
		}

		break;

	case IF_REPLACE:
		if (sptr)
		{
			XSAFELBSTR(sptr, buff, bufc);
			print_separator(sep, buff, bufc);
		}

		XSAFELBSTR(word, buff, bufc);

		if (eptr)
		{
			print_separator(sep, buff, bufc);
			XSAFELBSTR(eptr, buff, bufc);
		}

		break;

	case IF_INSERT:
		if (sptr)
		{
			XSAFELBSTR(sptr, buff, bufc);
			print_separator(sep, buff, bufc);
		}

		XSAFELBSTR(word, buff, bufc);

		if (iptr)
		{
			print_separator(sep, buff, bufc);
			XSAFELBSTR(iptr, buff, bufc);
		}

		if (eptr)
		{
			print_separator(sep, buff, bufc);
			XSAFELBSTR(eptr, buff, bufc);
		}

		break;
	}
}

