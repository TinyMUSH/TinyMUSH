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
 * @brief Compare two ColorState structures for equality.
 *
 * @param a Pointer to first ColorState
 * @param b Pointer to second ColorState
 * @return bool True if equal, false otherwise
 */
static inline bool colorstate_equal(const ColorState *a, const ColorState *b)
{
	return memcmp(a, b, sizeof(ColorState)) == 0;
}

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
 * @brief Validate multiple delimiters for table functions
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Command arguments
 * @param ncargs Number of command arguments
 * @param list_sep Delimiter for list separation
 * @param field_sep Delimiter for field separation
 * @param pad_char Padding character
 * @param list_pos Position of list delimiter argument
 * @param field_pos Position of field delimiter argument
 * @param pad_pos Position of padding character argument
 * @return int 1 if valid, 0 if error
 *
 * @details Validates the list separator, field separator, and padding character
 * for table-related functions like fun_table.
 */
static int
validate_table_delims(char *buff, char **bufc, dbref player, dbref caller, dbref cause,
					  char *fargs[], int nfargs, char *cargs[], int ncargs,
					  Delim *list_sep, Delim *field_sep, Delim *pad_char,
					  int list_pos, int field_pos, int pad_pos)
{
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, list_pos, list_sep, DELIM_STRING))
	{
		return 0;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, field_pos, field_sep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return 0;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, pad_pos, pad_char, 0))
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
	while (**s == ESC_CHAR)
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
 * @brief Consume a single ANSI escape sequence and update a ColorState.
 *
 * Advances the input cursor past a single ANSI escape sequence if one is
 * present at the current position, updating the provided `ColorState` to
 * reflect the effect of that sequence. If no valid escape sequence is
 * recognized, the cursor is advanced by one byte to avoid stalling.
 *
 * @param cursor Pointer to the input cursor (will be advanced)
 * @param state Pointer to the current `ColorState` to update
 */
static inline void consume_ansi_sequence_state(char **cursor, ColorState *state)
{
	const char *ptr = *cursor;

	if (ansi_apply_sequence(&ptr, state))
	{
		*cursor = (char *)ptr;
	}
	else
	{
		++(*cursor);
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
 * ldelete(<list>,<position>[,<separator>])
 *
 * insert: insert a word into a string by place
 * insert(<list>,<position>,<new item> [,<separator>])
 *
 * replace: replace a word into a string by place
 * replace(<list>,<position>,<new item>[,<separator>])
 *
 * lreplace: replace multiple words into a string by places
 * lreplace(<list>,<replacement words>,<positions>[,<isep>,<osep>])
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

/**
 * @brief Remove a single element from a delimited list.
 *
 * Splits @p fargs[0] on the delimiter (default space) and drops the element at
 * 1-based position @p fargs[1]. If @p pos < 1, the original list is returned.
 * If @p pos is beyond the end, the original list is returned unchanged. An
 * empty input list yields an empty result. Delimiter validation errors are
 * written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=position (1-based), [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_ldelete(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), NULL, &isep, IF_DELETE);
}

/**
 * @brief Replace a single list element at a given position.
 *
 * Splits @p fargs[0] on the delimiter (default space) and substitutes
 * @p fargs[2] for the element at 1-based position @p fargs[1]. If @p pos < 1,
 * or @p pos is beyond the end of the list, the original list is returned
 * unchanged. An empty input list yields an empty result. Delimiter validation
 * errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=position (1-based), [2]=replacement text, [3]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_replace(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 4, 4, DELIM_STRING, &isep))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), fargs[2], &isep, IF_REPLACE);
}

/**
 * @brief Insert a single element into a delimited list.
 *
 * Splits @p fargs[0] on the delimiter (default space) and inserts @p fargs[2]
 * at 1-based position @p fargs[1]. When @p pos == 1 and the input list is
 * empty, the new element becomes the list. If @p pos < 1, the original list is
 * returned unchanged. If @p pos is beyond the end, the element is appended.
 * Delimiter validation errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=position (1-based), [2]=text to insert, [3]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_insert(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 4, 4, DELIM_STRING, &isep))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), fargs[2], &isep, IF_INSERT);
}

/**
 * @brief Replace multiple positions in a list in one call.
 *
 * Parses three lists: the source list @p fargs[0], replacement values
 * @p fargs[1], and 1-based positions @p fargs[2]. The input delimiter
 * (default space or @p fargs[3]) splits source and replacement lists;
 * positions are always split on space. Output is joined with @p fargs[4] if
 * provided, otherwise the input delimiter is reused. The replacement list and
 * position list must contain the same number of elements or an error string
 * is returned. Positions outside the source range are ignored. When no
 * positions are supplied, the original list is returned unchanged. Delimiter
 * validation errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=source list, [1]=replacement list, [2]=positions list, [3]=optional input delimiter, [4]=optional output delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_lreplace(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;
	Delim osep;
	char *origlist, *replist, *poslist;
	char **orig_p, **rep_p, **pos_p;
	int norig, npos, i, cpos;

	/**
	 * We're generous with the argument checking, in case the replacement
	 * list is blank, and/or the position list is blank.
	 *
	 */
	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 5, 4, DELIM_STRING, &isep))
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

	/**
	 * If there are no positions to replace, then we just return the
	 * original list.
	 *
	 */
	if ((nfargs < 3) || !fargs[2])
	{
		XSAFELBSTR(fargs[0], buff, bufc);
		return;
	}

	/**
	 * The number of elements we have in our replacement list must equal
	 * the number of elements in our position list.
	 *
	 */
	if (!fargs[1] || (countwords(fargs[1], &isep) != countwords(fargs[2], &SPACE_DELIM)))
	{
		XSAFELBSTR("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}

	/**
	 * Turn out lists into arrays for ease of manipulation.
	 *
	 */
	origlist = fargs[0];
	replist = fargs[1];
	poslist = fargs[2];
	norig = list2arr(&orig_p, LBUF_SIZE / 2, origlist, &isep);
	list2arr(&rep_p, LBUF_SIZE / 2, replist, &isep);
	npos = list2arr(&pos_p, LBUF_SIZE / 2, poslist, &SPACE_DELIM);

	/**
	 * The positions we have aren't necessarily sequential, so we can't
	 * just walk through the list. We have to replace position by
	 * position. If we get an invalid position number, just ignore it.
	 *
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
}

/**
 * @brief Remove the first occurrence of a word from a delimited list.
 *
 * Validates arguments and delimiter (default space). If the target word
 * contains the delimiter, returns the error string "#-1 CAN ONLY DELETE ONE
 * ELEMENT". Walks the list token by token, copying all elements to @p buff
 * except the first one that equals @p fargs[1]; subsequent matches are kept.
 * If the word is not found, the original list is returned. An empty input list
 * yields an empty result. Delimiter validation errors are written to @p buff
 * by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=word to remove, [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_remove(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *sp, *word, *bb_p;
	Delim isep;
	int found;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	if (((isep.len == 1) && strchr(fargs[1], isep.str[0])) || ((isep.len > 1) && strstr(fargs[1], isep.str)))
	{
		XSAFELBSTR("#-1 CAN ONLY DELETE ONE ELEMENT", buff, bufc);
		return;
	}

	s = fargs[0];
	word = fargs[1];

	/**
	 * Walk through the string copying words until (if ever) we get to
	 * one that matches the target word.
	 *
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

			XSAFELBSTR(sp, buff, bufc);
		}
		else
		{
			found = 1;
		}
	}
}

/**
 * @brief Return the 1-based position of a word in a delimited list.
 *
 * Validates arguments and delimiter (default space). Trims leading separators
 * then walks the list token by token. Returns the index of the first element
 * equal to @p fargs[1], or "0" if the word does not occur. Delimiter
 * validation errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=word to find, [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_member(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int wcount;
	char *r, *s;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
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
			XSAFELTOS(buff, bufc, wcount, LBUF_SIZE);
			return;
		}

		wcount++;
	} while (s);

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Reverse all elements in a delimited list.
 *
 * Validates arguments and delimiter (default space). An empty argument list
 * returns an empty result. Applies a bounds check to avoid overrunning the
 * output buffer, truncating the input string if needed. Splits the list into
 * an array with @p fargs[0] and writes elements in reverse order, reusing the
 * input delimiter between items. Delimiter validation errors are written to
 * @p buff by the argument check.
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
void fun_revwords(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p, **elems;
	Delim isep;
	int n_elems, i;

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

	/**
	 * Nasty bounds checking
	 *
	 */
	if ((int)strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff) - 1)
	{
		*(fargs[0] + (LBUF_SIZE - (*bufc - buff) - 1)) = '\0';
	}

	/**
	 * Chop it up into an array of words and reverse them.
	 *
	 */
	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	bb_p = *bufc;

	for (i = n_elems - 1; i >= 0; i--)
	{
		if (*bufc != bb_p)
		{
			print_separator(&isep, buff, bufc);
		}

		XSAFELBSTR(elems[i], buff, bufc);
	}

	XFREE(elems);
}

/**
 * @brief Merge two lists by substituting matching elements from a second list.
 *
 * Validates arguments and delimiter (default space). Requires that @p fargs[0]
 * and @p fargs[1] have the same number of elements; otherwise returns
 * "#-1 NUMBER OF WORDS MUST BE EQUAL". If @p fargs[2] contains more than one
 * word, returns "#-1 TOO MANY WORDS". Uses @p fargs[2] as the match token: for
 * each position, if the element from list1 equals that token, the element from
 * the same position in list2 is emitted; otherwise the original element from
 * list1 is emitted. Output uses @p fargs[4] when provided, else reuses the
 * input delimiter. Delimiter validation errors are written to @p buff by the
 * argument checks.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list1, [1]=list2, [2]=match word, [3]=optional input delimiter, [4]=optional output delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_splice(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p1, *p2, *q1, *q2, *bb_p;
	Delim isep, osep;
	int words, i;

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

	/**
	 * length checks
	 *
	 */
	if (countwords(fargs[2], &isep) > 1)
	{
		XSAFELBSTR("#-1 TOO MANY WORDS", buff, bufc);
		return;
	}

	words = countwords(fargs[0], &isep);

	if (words != countwords(fargs[1], &isep))
	{
		XSAFELBSTR("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}

	/**
	 * loop through the two lists
	 *
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
			XSAFELBSTR(q2, buff, bufc); /*!< replace */
		}
		else
		{
			XSAFELBSTR(p2, buff, bufc); /*!< copy */
		}
	}
}

/**
 * @brief qsort helper for case-sensitive string comparison.
 *
 * Casts @p s1 and @p s2 to `const char **` and returns the result of
 * `strcmp(*s1, *s2)`. Suitable for use with `qsort` over an array of `char *`
 * pointers.
 *
 * @param s1 Pointer to first string pointer
 * @param s2 Pointer to second string pointer
 * @return int Negative if `*s1` < `*s2`, zero if equal, positive if `*s1` > `*s2`
 */
int a_comp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

/**
 * @brief qsort helper for case-insensitive string comparison.
 *
 * Casts @p s1 and @p s2 to `const char **` and returns the result of
 * `strcasecmp(*s1, *s2)`. Suitable for use with `qsort` over an array of
 * `char *` pointers when ASCII case folding is desired.
 *
 * @param s1 Pointer to first string pointer
 * @param s2 Pointer to second string pointer
 * @return int Negative if `*s1` < `*s2`, zero if equal, positive if `*s1` > `*s2`
 */
int c_comp(const void *s1, const void *s2)
{
	return strcasecmp(*(char **)s1, *(char **)s2);
}

/**
 * @brief qsort helper for case-sensitive alphanumeric record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `A_RECORD` and compares their
 * `str` fields with `strcmp`. Designed for `qsort` over arrays of `A_RECORD`.
 *
 * @param s1 Pointer to first `A_RECORD`
 * @param s2 Pointer to second `A_RECORD`
 * @return int Negative if `s1->str` < `s2->str`, zero if equal, positive if `s1->str` > `s2->str`
 */
int arec_comp(const void *s1, const void *s2)
{
	return strcmp(((A_RECORD *)s1)->str, ((A_RECORD *)s2)->str);
}

/**
 * @brief qsort helper for case-insensitive alphanumeric record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `A_RECORD` and compares their
 * `str` fields with `strcasecmp` for ASCII case folding. Designed for `qsort`
 * over arrays of `A_RECORD`.
 *
 * @param s1 Pointer to first `A_RECORD`
 * @param s2 Pointer to second `A_RECORD`
 * @return int Negative if `s1->str` < `s2->str`, zero if equal, positive if `s1->str` > `s2->str`
 */
int crec_comp(const void *s1, const void *s2)
{
	return strcasecmp(((A_RECORD *)s1)->str, ((A_RECORD *)s2)->str);
}

/**
 * @brief qsort helper for floating-point record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `F_RECORD` and compares their
 * `data` fields. Returns 1 when the first value is greater, -1 when smaller,
 * and 0 when equal. Suitable for use with `qsort` over arrays of `F_RECORD`.
 *
 * @param s1 Pointer to first `F_RECORD`
 * @param s2 Pointer to second `F_RECORD`
 * @return int 1 if `s1->data` > `s2->data`, -1 if less, 0 if equal
 */
int f_comp(const void *s1, const void *s2)
{
	if (((F_RECORD *)s1)->data > ((F_RECORD *)s2)->data)
	{
		return 1;
	}

	if (((F_RECORD *)s1)->data < ((F_RECORD *)s2)->data)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief qsort helper for integer record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `I_RECORD` and compares their
 * `data` fields as integers. Returns 1 when the first value is greater, -1
 * when smaller, and 0 when equal. Intended for use with `qsort` over arrays
 * of `I_RECORD`.
 *
 * @param s1 Pointer to first `I_RECORD`
 * @param s2 Pointer to second `I_RECORD`
 * @return int 1 if `s1->data` > `s2->data`, -1 if less, 0 if equal
 */
int i_comp(const void *s1, const void *s2)
{
	if (((I_RECORD *)s1)->data > ((I_RECORD *)s2)->data)
	{
		return 1;
	}

	if (((I_RECORD *)s1)->data < ((I_RECORD *)s2)->data)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief Sort a list of strings by the requested type and optionally return positions.
 *
 * Sorts the array @p s of length @p n using @p sort_type: ALPHANUM_LIST and
 * NOCASE_LIST apply string comparison (case-sensitive/insensitive), NUMERIC_LIST
 * parses integers, DBREF_LIST sorts by @c dbnum(), and FLOAT_LIST parses doubles.
 * When @p listpos_only is zero, @p s is sorted in place. When non-zero, a 1-based
 * position array is returned indicating each element's original index in sorted
 * order. For ALPHANUM_LIST and NOCASE_LIST the input array is left untouched when
 * @p listpos_only is non-zero; other modes still reorder @p s while also returning
 * positions. Caller must free the returned array (if non-NULL) with @c XFREE.
 *
 * @param s Array of list elements to sort (modified unless ALPHANUM/NOCASE with listpos_only set)
 * @param n Number of elements in @p s
 * @param sort_type Sort strategy selector (ALPHANUM_LIST, NOCASE_LIST, NUMERIC_LIST, DBREF_LIST, FLOAT_LIST)
 * @param listpos_only Non-zero to return original positions instead of (or in addition to) an in-place sort
 * @return int* Position array mapping sorted order to original 1-based indices, or NULL if not requested
 */
int *do_asort(char *s[], int n, int sort_type, int listpos_only)
{
	int i;
	F_RECORD *fp = NULL;
	I_RECORD *ip = NULL;
	A_RECORD *ap = NULL;
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
			ap = (A_RECORD *)XCALLOC(n, sizeof(A_RECORD), "ap");

			for (i = 0; i < n; i++)
			{
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}

			qsort((void *)ap, n, sizeof(A_RECORD), (int (*)(const void *, const void *))arec_comp);

			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ap[i].pos;
			}

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
			ap = (A_RECORD *)XCALLOC(n, sizeof(A_RECORD), "ap");

			for (i = 0; i < n; i++)
			{
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}

			qsort((void *)ap, n, sizeof(A_RECORD), (int (*)(const void *, const void *))crec_comp);

			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ap[i].pos;
			}

			XFREE(ap);
		}

		break;

	case NUMERIC_LIST:
		ip = (I_RECORD *)XCALLOC(n, sizeof(I_RECORD), "ip");

		for (i = 0; i < n; i++)
		{
			ip[i].str = s[i];
			ip[i].data = (int)strtol(s[i], (char **)NULL, 10);
			ip[i].pos = i + 1;
		}

		qsort((void *)ip, n, sizeof(I_RECORD), (int (*)(const void *, const void *))i_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = ip[i].str;
		}

		if (listpos_only)
		{
			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ip[i].pos;
			}
		}

		XFREE(ip);
		break;

	case DBREF_LIST:
		ip = (I_RECORD *)XCALLOC(n, sizeof(I_RECORD), "ip");

		for (i = 0; i < n; i++)
		{
			ip[i].str = s[i];
			ip[i].data = dbnum(s[i]);
			ip[i].pos = i + 1;
		}

		qsort((void *)ip, n, sizeof(I_RECORD), (int (*)(const void *, const void *))i_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = ip[i].str;
		}

		if (listpos_only)
		{
			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ip[i].pos;
			}
		}

		XFREE(ip);
		break;

	case FLOAT_LIST:
		fp = (F_RECORD *)XCALLOC(n, sizeof(F_RECORD), "fp");

		for (i = 0; i < n; i++)
		{
			fp[i].str = s[i];
			fp[i].data = strtod(s[i], (char **)NULL);
			fp[i].pos = i + 1;
		}

		qsort((void *)fp, n, sizeof(F_RECORD), (int (*)(const void *, const void *))f_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = fp[i].str;
		}

		if (listpos_only)
		{
			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = fp[i].pos;
			}
		}

		XFREE(fp);
		break;
	}

	return poslist;
}

/**
 * @brief Entry point for the `sort()` family of functions.
 *
 * Validates the function arguments and delimiters, converts the input list
 * (from `fargs[0]`) into an array, determines the requested sort type via
 * `get_list_type()`, and delegates the actual sorting to `do_asort()`.
 *
 * Behavior:
 * - If the caller requested positions (via the function mask `SORT_POS`),
 *   this function emits a space-separated list of 1-based positions
 *   corresponding to the sorted order (the positions array is produced by
 *   `do_asort()` and freed after use).
 * - Otherwise, it reconstructs the sorted array back into a delimited string
 *   using the output delimiter (or the input delimiter when unspecified) and
 *   writes that to the output buffer.
 *
 * The function returns early for empty argument lists. Delimiter validation
 * and error reporting are handled by `validate_list_args()` and
 * `delim_check()`; any error messages are written into @p buff.
 *
 * @param buff Output buffer where results or error strings are written
 * @param bufc Pointer into @p buff for appending output
 * @param player DBref of player invoking the function (unused by sorting)
 * @param caller DBref of the caller (unused by sorting)
 * @param cause DBref of the cause (unused by sorting)
 * @param fargs Function arguments: at minimum [0]=list; additional args
 *              control sort type and delimiters as accepted by
 *              `get_list_type()` and the surrounding `sort()` API.
 * @param nfargs Number of function arguments in @p fargs
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void handle_sort(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nitems, sort_type, oper, i;
	char *list, **ptrs;
	int *poslist;
	Delim isep, osep;

	/**
	 * If we are passed an empty arglist return a null string
	 *
	 */
	if (nfargs == 0)
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 4, 3, DELIM_STRING, &isep))
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

	oper = Func_Mask(SORT_POS);

	/**
	 * Convert the list to an array
	 *
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

			XSAFELTOS(buff, bufc, poslist[i], LBUF_SIZE);
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
 * sortby:
 */

/**
 * @brief Invoke a user-provided comparison expression for sorting.
 *
 * Evaluates the expression in @p cbuff with @p s1 and @p s2 bound as the
 * two input parameters. The expression is executed via
 * `eval_expression_string()` and must yield a numeric result interpreted as
 * follows: negative if `s1 < s2`, zero if equal, positive if `s1 > s2`.
 *
 * Important: this function is designed to be used with the internal
 * `sane_qsort()` routine (which accepts extra context parameters) and is
 * NOT compatible with the standard library `qsort()` comparator signature.
 *
 * Resource and safety notes:
 * - The function guards against excessive resource use by checking
 *   `mushstate.func_invk_ctr`, `mushstate.func_nest_lev` and `Too_Much_CPU()`;
 *   if limits are exceeded it returns 0 to avoid further evaluation.
 * - `cbuff` is copied into a temporary buffer before evaluation; no caller
 *   ownership changes are performed by this function.
 *
 * @param s1 Pointer to the first element (passed to the user expression)
 * @param s2 Pointer to the second element (passed to the user expression)
 * @param cbuff Null-terminated user comparison expression (copied internally)
 * @param thing DBref used as evaluation context (e.g. attribute owner)
 * @param player DBref of the player performing the evaluation
 * @param cause DBref of the cause of the evaluation
 * @return int <0 if `s1 < s2`, 0 if equal, >0 if `s1 > s2` as produced by the expression
 */
int u_comp(const void *s1, const void *s2, char *cbuff, dbref thing, dbref player, dbref cause)
{
	/**
	 * Note that this function is for use in conjunction with our own
	 * sane_qsort routine, NOT with the standard library qsort!
	 *
	 */
	char *result, *tbuf, *elems[2], *bp;
	int n;

	if ((mushstate.func_invk_ctr > mushconf.func_invk_lim) || (mushstate.func_nest_lev > mushconf.func_nest_lim) || Too_Much_CPU())
	{
		return 0;
	}

	tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	elems[0] = (char *)s1;
	elems[1] = (char *)s2;
	XSTRCPY(tbuf, cbuff);
	result = bp = XMALLOC(LBUF_SIZE, "result");
	eval_expression_string(result, &bp, thing, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &tbuf, elems, 2);
	n = (int)strtol(result, (char **)NULL, 10);
	XFREE(result);
	XFREE(tbuf);
	return n;
}

/**
 * @brief Andrew Molitor's quicksort variant tolerant of non-transitive comparators.
 *
 * This routine sorts an array of pointers in-place using a randomized
 * pivot-based partitioning algorithm that does not require the comparator to
 * satisfy strict transitivity. It is intended for use when the comparison
 * function may depend on external context and therefore can produce
 * comparisons that violate a>b => b<a. The algorithm partitions elements
 * around a pivot chosen at random, swaps pointers in-place, and recursively
 * sorts partitions. Tail recursion is converted to iteration to reduce stack
 * usage.
 *
 * Comparator contract:
 * - The provided @p compare callback must have the signature
 *   `int compare(const void *a, const void *b, char *cbuff, dbref, dbref, dbref)`
 *   and should return negative if `a < b`, zero if `a == b`, positive if
 *   `a > b` (semantics are determined by the caller-provided comparator).
 * - The additional parameters `cbuff`, `thing`, `player`, and `cause` are
 *   forwarded to @p compare unchanged and may be used by user-defined
 *   comparison expressions.
 *
 * Notes and limitations:
 * - Sorting is performed by swapping `void *` pointers; element data is not
 *   copied. The caller is responsible for pointer validity for the duration
 *   of the call.
 * - The routine is not inherently thread-safe; concurrent calls must ensure
 *   the comparator and context are safe for parallel use.
 * - Average complexity is O(n log n); worst-case is O(n^2), but randomized
 *   pivot selection reduces the likelihood of adversarial worst-cases.
 *
 * @param array Array of pointers to sort (modified in-place)
 * @param left Index of the leftmost element to sort (0-based)
 * @param right Index of the rightmost element to sort (0-based)
 * @param compare Function pointer to comparator with extended context
 * @param cbuff User-supplied comparison buffer (forwarded to @p compare)
 * @param thing DBref passed to @p compare as context
 * @param player DBref passed to @p compare as context
 * @param cause DBref passed to @p compare as context
 */
void sane_qsort(void *array[], int left, int right, int (*compare)(const void *, const void *, char *, dbref, dbref, dbref), char *cbuff, dbref thing, dbref player, dbref cause)
{
	/*
	 *
	 */
	int i, last;
	void *tmp;
loop:

	if (left >= right)
	{
		return;
	}

	/**
	 * Pick something at random at swap it into the leftmost slot
	 * This is the pivot, we'll put it back in the right spot later
	 *
	 */
	i = random_range(0, (1 + (right - left)) - 1);
	tmp = array[left + i];
	array[left + i] = array[left];
	array[left] = tmp;
	last = left;

	for (i = left + 1; i <= right; i++)
	{
		/**
		 * Walk the array, looking for stuff that's less than our
		 * pivot. If it is, swap it with the next thing along
		 *
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

	/**
	 * Now we put the pivot back, it's now in the right spot, we never
	 * need to look at it again, trust me.
	 *
	 */
	tmp = array[last];
	array[last] = array[left];
	array[left] = tmp;

	/**
	 * At this point everything underneath the 'last' index is < the
	 * entry at 'last' and everything above it is not < it.
	 *
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

/**
 * @brief Sort a list using a user-supplied comparison expression.
 *
 * Sorts the list provided in @p fargs[1] using a user-defined comparison
 * expression supplied either as an attribute name in @p fargs[0] or as a
 * lambda string beginning with "#lambda/". The attribute text (or lambda
 * body) is retrieved and passed as the comparison buffer to `sane_qsort()`
 * via the `u_comp()` wrapper. The user expression will be evaluated with the
 * two elements under comparison bound as the evaluation arguments; it must
 * return a numeric result interpreted as negative if the first element is
 * less than the second, zero if equal, and positive if greater.
 *
 * Argument validation and delimiter handling are performed via
 * `validate_list_args()` and `delim_check()`; any error messages are written
 * into @p buff and the function returns early. The input list is split using
 * the input delimiter (default space) and the sorted result is written back
 * to the output buffer joined with the output delimiter (or input delimiter
 * when the output delimiter is not provided).
 *
 * Resource notes:
 * - The attribute text (or lambda body) is copied into a temporary buffer
 *   and freed before returning.
 * - Sorting uses `sane_qsort()` which swaps pointer values in-place; the
 *   elements pointed to must remain valid during the call.
 * - The comparator evaluation respects function invocation and nesting
 *   limits; excessive usage may cause the comparator to return 0.
 *
 * @param buff Output buffer where the sorted list or error is written
 * @param bufc Output buffer cursor for appending
 * @param player DBref of the player invoking the function
 * @param caller DBref of the caller (unused)
 * @param cause DBref of the cause (unused)
 * @param fargs Function arguments: [0]=attribute name or "#lambda/..." comparator, [1]=list to sort, [2]=optional input delimiter, [3]=optional output delimiter
 * @param nfargs Number of function arguments in @p fargs
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
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

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
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

	if (string_prefix(fargs[0], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen(fargs[0] + 8);
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
	{
		/**
		 * pointless to sort less than 2 elements
		 *
		 */
		sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp, atext, thing, player, cause);
	}

	arr2list(ptrs, nptrs, buff, bufc, &osep);
	XFREE(list);
	XFREE(atext);
	XFREE(ptrs);
}

/**
 * @brief Set management: SETUNION, SETDIFF, SETINTER. Also LUNION,
 * LDIFF, LINTER: Same thing, but takes a sort type like sort() does. There's
 * an unavoidable PennMUSH conflict, as setunion() and friends have a 4th-arg
 * output delimiter in TM3, but the 4th arg is used for the sort type in
 * PennMUSH. Also, adding the sort type as a fifth arg for setunion(), etc.
 * would be confusing, since the last two args are, by convention,
 * delimiters. So we add new funcs.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */

/**
 * @brief Compare two items by sort type
 * @param s1 First item string
 * @param s2 Second item string
 * @param sort_type Sort type (ALPHANUM_LIST, NOCASE_LIST, FLOAT_LIST, INT_LIST)
 * @param ip1 Pointer to first integer value (for INT_LIST)
 * @param ip2 Pointer to second integer value (for INT_LIST)
 * @param fp1 Pointer to first float value (for FLOAT_LIST)
 * @param fp2 Pointer to second float value (for FLOAT_LIST)
 * @return Comparison result: <0, 0, or >0
 */
static int compare_items(const char *s1, const char *s2, int sort_type, int *ip1, int *ip2, double *fp1, double *fp2)
{
	if (sort_type == ALPHANUM_LIST)
	{
		return strcmp(s1, s2);
	}
	else if (sort_type == NOCASE_LIST)
	{
		return strcasecmp(s1, s2);
	}
	else if (sort_type == FLOAT_LIST)
	{
		return (*fp1 > *fp2) ? 1 : ((*fp1 < *fp2) ? -1 : 0);
	}
	else
	{
		return (*ip1 > *ip2) ? 1 : ((*ip1 < *ip2) ? -1 : 0);
	}
}

void handle_sets(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	int oper, type_arg;
	char *list1, *list2, *bb_p;
	char oldstr[LBUF_SIZE];
	char **ptrs1, **ptrs2;
	int i1, i2, n1, n2, val, sort_type;
	int *ip1, *ip2;
	double *fp1, *fp2;
	oper = Func_Mask(SET_OPER);
	type_arg = Func_Mask(SET_TYPE);

	if (type_arg)
	{
		if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 5, 4, DELIM_STRING, &isep))
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
	}
	else
	{
		if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
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

	/**
	 * This conversion is inefficient, since it's already happened once
	 * in do_asort().
	 *
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
	bb_p = *bufc;
	*oldstr = '\0';
	**bufc = '\0';

	switch (oper)
	{
	case SET_UNION: /*!< Copy elements common to both lists */

		/**
		 * Handle case of two identical single-element lists
		 *
		 */
		if ((n1 == 1) && (n2 == 1) && (!strcmp(ptrs1[0], ptrs2[0])))
		{
			XSAFELBSTR(ptrs1[0], buff, bufc);
			break;
		}

		/**
		 * Process until one list is empty
		 *
		 */
		while ((i1 < n1) && (i2 < n2))
		{
			/**
			 * Skip over duplicates
			 *
			 */
			if ((i1 > 0) || (i2 > 0))
			{
				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}

			/**
			 * Compare and copy
			 *
			 */
			if ((i1 < n1) && (i2 < n2))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				if (compare_items(ptrs1[i1], ptrs2[i2], sort_type, &ip1[i1], &ip2[i2], &fp1[i1], &fp2[i2]) < 0)
				{
					XSAFELBSTR(ptrs1[i1], buff, bufc);
					XSTRCPY(oldstr, ptrs1[i1]);
					i1++;
				}
				else
				{
					XSAFELBSTR(ptrs2[i2], buff, bufc);
					XSTRCPY(oldstr, ptrs2[i2]);
					i2++;
				}

				**bufc = '\0';
			}
		}

		/**
		 * Copy rest of remaining list, stripping duplicates
		 *
		 */
		for (; i1 < n1; i1++)
		{
			if (strcmp(oldstr, ptrs1[i1]))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSTRCPY(oldstr, ptrs1[i1]);
				XSAFELBSTR(ptrs1[i1], buff, bufc);
				**bufc = '\0';
			}
		}

		for (; i2 < n2; i2++)
		{
			if (strcmp(oldstr, ptrs2[i2]))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSTRCPY(oldstr, ptrs2[i2]);
				XSAFELBSTR(ptrs2[i2], buff, bufc);
				**bufc = '\0';
			}
		}

		break;

	case SET_INTERSECT: /*!< Copy elements not in both lists */
		while ((i1 < n1) && (i2 < n2))
		{
			val = compare_items(ptrs1[i1], ptrs2[i2], sort_type, &ip1[i1], &ip2[i2], &fp1[i1], &fp2[i2]);

			if (!val)
			{
				/**
				 * Got a match, copy it
				 *
				 */
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSTRCPY(oldstr, ptrs1[i1]);
				XSAFELBSTR(ptrs1[i1], buff, bufc);
				i1++;
				i2++;

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
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

	case SET_DIFF: /*!< Copy elements unique to list1 */
		while ((i1 < n1) && (i2 < n2))
		{
			val = compare_items(ptrs1[i1], ptrs2[i2], sort_type, &ip1[i1], &ip2[i2], &fp1[i1], &fp2[i2]);

			if (!val)
			{
				/**
				 * Got a match, increment pointers
				 *
				 */
				XSTRCPY(oldstr, ptrs1[i1]);

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}
			else if (val < 0)
			{
				/**
				 * Item in list1 not in list2, copy
				 *
				 */
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSAFELBSTR(ptrs1[i1], buff, bufc);
				XSTRCPY(oldstr, ptrs1[i1]);
				i1++;

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}
			}
			else
			{
				/**
				 * Item in list2 but not in list1, discard
				 *
				 */
				XSTRCPY(oldstr, ptrs2[i2]);
				i2++;

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}
		}

		/**
		 * Copy remainder of list1
		 *
		 */
		while (i1 < n1)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELBSTR(ptrs1[i1], buff, bufc);
			XSTRCPY(oldstr, ptrs1[i1]);
			i1++;

			while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
			{
				i1++;
			}
		}
	}

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
	XFREE(list1);
	XFREE(list2);
}

/**
 * @brief Format a list into fixed-width columns, preserving ANSI color state.
 *
 * Splits the input list on the supplied delimiter, truncates or pads each
 * element to the requested width, and emits ANSI transitions so colored text
 * stays intact across column boundaries.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments: [0]=list, [1]=column width, [2]=optional delimiter
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 *
 * @details Each element is padded or truncated to fit the specified width,
 * and ANSI color states are preserved between elements.
 */
void fun_columns(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	unsigned int spaces, number, striplen;
	unsigned int count, i, indent = 0;
	ColorState ansi_state = color_none;
	int rturn = 1;
	size_t remaining;
	char *p, *q, *buf, *objstring, *cp, *cr = NULL;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
	{
		return;
	}

	number = (int)strtol(fargs[1], (char **)NULL, 10);
	indent = (nfargs >= 4) ? (int)strtol(fargs[3], (char **)NULL, 10) : 0;

	if (indent > 77)
	{
		/**
		 * unsigned int, always a positive number
		 *
		 */
		indent = 1;
	}

	/**
	 * Must check number separately, since number + indent can result in
	 * an integer overflow.
	 *
	 */
	if ((number < 1) || (number > 77) || ((unsigned int)(number + indent) > 78))
	{
		XSAFELBSTR("#-1 OUT OF RANGE", buff, bufc);
		return;
	}

	cp = trim_space_sep(fargs[0], &isep);

	if (!*cp)
	{
		return;
	}

	for (i = 0; i < indent; i++)
	{
		XSAFELBCHR(' ', buff, bufc);
	}

	buf = XMALLOC(LBUF_SIZE, "buf");

	while (cp)
	{
		objstring = split_token(&cp, &isep);
		striplen = ansi_strip_ansi_len(objstring);

		p = objstring;
		q = buf;
		count = 0;
		ansi_state = color_none;

		while (p && *p)
		{
			if (count == number)
			{
				break;
			}

			if (*p == ESC_CHAR)
			{
				char *seq_start = p;
				char *seq_end;
				ColorState prev_state = ansi_state;
				size_t seq_len;

				consume_ansi_sequence_state(&p, &ansi_state);
				seq_end = p;
				remaining = LBUF_SIZE - 1 - (size_t)(q - buf);

				if (!remaining)
				{
					break;
				}

				seq_len = seq_end - seq_start;

				if (seq_len > remaining)
				{
					p = seq_start;
					ansi_state = prev_state;
					break;
				}

				XMEMCPY(q, seq_start, seq_len);
				q += seq_len;
			}
			else
			{
				if ((size_t)(q - buf) >= LBUF_SIZE - 1)
				{
					break;
				}

				*q++ = *p++;
				count++;
			}
		}

		if (memcmp(&ansi_state, &color_none, sizeof(ColorState)) != 0)
		{
			char *reset_seq = ansi_transition_colorstate(ansi_state, color_none, ColorTypeTrueColor, false);
			size_t reset_len = strlen(reset_seq);
			remaining = LBUF_SIZE - 1 - (size_t)(q - buf);

			if (reset_len > remaining)
			{
				reset_len = remaining;
			}

			XMEMCPY(q, reset_seq, reset_len);
			q += reset_len;
			XFREE(reset_seq);
		}

		*q = '\0';
		XSAFELBSTR(buf, buff, bufc);

		if (striplen < number)
		{
			/**
			 * We only need spaces if we need to pad out.
			 * Sanitize the number of spaces, too.
			 *
			 */
			spaces = number - striplen;

			if (spaces > LBUF_SIZE)
			{
				spaces = LBUF_SIZE;
			}

			for (i = 0; i < spaces; i++)
			{
				XSAFELBCHR(' ', buff, bufc);
			}
		}

		if (!(rturn % (int)((78 - indent) / number)))
		{
			XSAFECRLF(buff, bufc);
			cr = *bufc;

			for (i = 0; i < indent; i++)
			{
				XSAFELBCHR(' ', buff, bufc);
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
		XSAFECRLF(buff, bufc);
	}

	XFREE(buf);
}

/**
 * @brief Helper function for perform_tables
 *
 * @param list Input list to format into table
 * @param last_state Current ANSI color state
 * @param n_cols Number of columns in the table
 * @param col_widths Array of column widths
 * @param lead_str Leading string for each row
 * @param trail_str Trailing string for each row
 * @param list_sep Separator between list items
 * @param field_sep Separator between fields in items
 * @param pad_char Padding character for alignment
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param just Justification flag
 *
 * @details Formats the list into a table with specified columns, preserving ANSI colors.
 */
void tables_helper(char *list, ColorState *last_state, int n_cols, int col_widths[], char *lead_str, char *trail_str, const Delim *list_sep, const Delim *field_sep, const Delim *pad_char, char *buff, char **bufc, int just)
{
	int i = 0, nwords = 0, nstates = 0, cpos = 0, wcount = 0, over = 0;
	int max = 0, nleft = 0, lead_chrs = 0, lens[LBUF_SIZE / 2];
	ColorState ansi_state = color_none;
	ColorState states[LBUF_SIZE / 2 + 1];
	char *s = NULL, **words = NULL, *buf = NULL;
	/**
	 * Split apart the list. We need to find the length of each
	 * de-ansified word, as well as keep track of the state of each word.
	 * Overly-long words eventually get truncated, but the correct ANSI
	 * state is preserved nonetheless.
	 *
	 */
	nstates = list2ansi(states, last_state, LBUF_SIZE / 2, list, list_sep);
	nwords = list2arr(&words, LBUF_SIZE / 2, list, list_sep);

	if (nstates != nwords)
	{
		XSAFESPRINTF(buff, bufc, "#-1 STATE/WORD COUNT OFF: %d/%d", nstates, nwords);
		XFREE(words);
		return;
	}

	for (i = 0; i < nwords; i++)
	{
		lens[i] = ansi_strip_ansi_len(words[i]);
	}

	over = wcount = 0;

	while ((wcount < nwords) && !over)
	{
		/**
		 * Beginning of new line. Insert newline if this isn't the
		 * first thing we're writing. Write left margin, if
		 * appropriate.
		 *
		 */
		if (wcount != 0)
		{
			XSAFECRLF(buff, bufc);
		}

		if (lead_str)
		{
			over = XSAFELBSTR(lead_str, buff, bufc);
		}

		/**
		 * Do each column in the line.
		 *
		 */
		for (cpos = 0; (cpos < n_cols) && (wcount < nwords) && !over; cpos++, wcount++)
		{
			/**
			 * Write leading padding if we need it.
			 *
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

			/**
			 * If we had a previous state, we have to write it.
			 *
			 */
			buf = ansi_transition_colorstate(color_none, states[wcount], ColorTypeTrueColor, false);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);

			/**
			 * Copy in the word.
			 *
			 */
			if (lens[wcount] <= col_widths[cpos])
			{
				over = XSAFELBSTR(words[wcount], buff, bufc);
				buf = ansi_transition_colorstate(states[wcount + 1], color_none, ColorTypeTrueColor, false);
				XSAFELBSTR(buf, buff, bufc);
				XFREE(buf);
			}
			else
			{
				/**
				 * Bleah. We have a string that's too long.
				 * Truncate it. Write an ANSI normal at the
				 * end at the end if we need one (we'll
				 * restore the correct ANSI code with the
				 * next word, if need be).
				 *
				 */
				ansi_state = states[wcount];

				for (s = words[wcount], i = 0; *s && (i < col_widths[cpos]);)
				{
					if (*s == ESC_CHAR)
					{
						consume_ansi_sequence_state(&s, &ansi_state);
					}
					else
					{
						s++;
						i++;
					}
				}

				XSAFESTRNCAT(buff, bufc, words[wcount], s - words[wcount], LBUF_SIZE);
				buf = ansi_transition_colorstate(ansi_state, color_none, ColorTypeTrueColor, false);
				XSAFELBSTR(buf, buff, bufc);
				XFREE(buf);
			}

			/**
			 * Writing trailing padding if we need it.
			 *
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

			/**
			 * Insert the field separator if this isn't the last
			 * column AND this is not the very last word in the
			 * list.
			 *
			 */
			if ((cpos < n_cols - 1) && (wcount < nwords - 1))
			{
				print_separator(field_sep, buff, bufc);
			}
		}

		if (!over && trail_str)
		{
			/**
			 * If we didn't get enough columns to fill out a
			 * line, and this is the last line, then we have to
			 * pad it out.
			 *
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

			/**
			 * Write the right margin.
			 *
			 */
			over = XSAFELBSTR(trail_str, buff, bufc);
		}
	}

	/**
	 * Save the ANSI state of the last word.
	 *
	 */
	*last_state = states[nstates - 1];

	/**
	 * Clean up.
	 *
	 */
	XFREE(words);
}

/**
 * @brief Draw a table
 *
 * @param player DBref of player (unused)
 * @param list List to draw in table
 * @param n_cols Number of columns
 * @param col_widths Array of column widths
 * @param lead_str Leading string for each row
 * @param trail_str Trailing string for each row
 * @param list_sep Separator between list items
 * @param field_sep Separator between fields in items
 * @param pad_char Padding character
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param just Justification flag for the table
 *
 * @details Formats the input list into a multi-column table with specified widths and separators.
 */
void perform_tables(dbref player, char *list, int n_cols, int col_widths[], char *lead_str, char *trail_str, const Delim *list_sep, const Delim *field_sep, const Delim *pad_char, char *buff, char **bufc, int just)
{
	char *p, *savep, *bb_p;
	ColorState ansi_state = color_none;

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
			XSAFECRLF(buff, bufc);
		}

		tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str, list_sep, field_sep, pad_char, buff, bufc, just);
		savep = p + 2; /*!< must skip '\n' too */
		p = strchr(savep, '\r');
	}

	if (*bufc != bb_p)
	{
		XSAFECRLF(buff, bufc);
	}

	tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str, list_sep, field_sep, pad_char, buff, bufc, just);
}

/**
 * @brief Validate that we have everything to draw the table
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments: [0]=list, [1]=column specs, [2]=optional delimiters
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 *
 * @details Parses column specifications and calls perform_tables to format the table.
 */
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

	if (!validate_table_delims(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, &list_sep, &field_sep, &pad_char, 5, 6, 7))
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

/*---------------------------------------------------------------------------
 * fun_table:
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

/**
 * @brief Turn a list into a table.
 *
 *   table(<list>,<field width>,<line length>,<list delim>,<field sep>,<pad>)
 *     Only the <list> parameter is mandatory.
 *   tables(<list>,<field widths>,<lead str>,<trail str>,<list delim>,<field sep str>,<pad>)
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
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
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

	if (!validate_table_delims(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, &list_sep, &field_sep, &pad_char, 4, 5, 6))
	{
		return;
	}

	/**
	 * Get line length and column width. All columns are the same width.
	 * Calculate what we need to.
	 *
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
		field_sep_width = ansi_strip_ansi_len(field_sep.str);
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

/**
 * @brief given a list of numbers, get corresponding elements from the list.
 *
 * elements(ack bar eep foof yay,2 4) ==> bar foof The function takes
 * a separator, but the separator only applies to the first list.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_elements(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nwords, cur, start, end, stepn;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	char *end_p, *step_p;
	Delim isep, osep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
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

	oldp = *bufc;

	/**
	 * Turn the first list into an array.
	 *
	 */
	wordlist = XMALLOC(LBUF_SIZE, "wordlist");
	XSTRCPY(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);
	s = Eat_Spaces(fargs[1]);

	/**
	 * Go through the second list, grabbing the numbers and finding the
	 * corresponding elements.
	 *
	 */
	do
	{
		r = split_token(&s, &SPACE_DELIM);

		if ((end_p = strchr(r, ':')) == NULL)
		{
			/**
			 * Just a number. If negative, count back from end of
			 * list.
			 *
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

				XSAFELBSTR(ptrs[cur], buff, bufc);
			}
		}
		else
		{
			/**
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

			/**
			 * r points to our start
			 *
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
					/**
					 * Empty start
					 *
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
					/**
					 * Empty end
					 *
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

							XSAFELBSTR(ptrs[cur], buff, bufc);
						}
					}
				}
			}
			else if (stepn < 0)
			{
				if (*r == '\0')
				{
					/**
					 * Empty start, goes to the LAST element
					 *
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
					/**
					 * Empty end
					 *
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

							XSAFELBSTR(ptrs[cur], buff, bufc);
						}
					}
				}
			}
		}
	} while (s);

	XFREE(wordlist);
	XFREE(ptrs);
}

/**
 * @brief Return the elements of a list EXCEPT the numbered items.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_exclude(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nwords, cur, start, end, stepn;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	char *end_p, *step_p;
	int *mapper;
	Delim isep, osep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
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

	oldp = *bufc;

	/**
	 * Turn the first list into an array.
	 *
	 */
	wordlist = XMALLOC(LBUF_SIZE, "wordlist");
	XSTRCPY(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);
	s = Eat_Spaces(fargs[1]);

	/**
	 * Go through the second list, grabbing the numbers and mapping the
	 * corresponding elements.
	 *
	 */
	mapper = (int *)XCALLOC(nwords, sizeof(int), "mapper");

	do
	{
		r = split_token(&s, &SPACE_DELIM);

		if ((end_p = strchr(r, ':')) == NULL)
		{
			/**
			 * Just a number. If negative, count back from end of list.
			 *
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
			/**
			 * Slicing syntax
			 *
			 */

			/**
			 * r points to our start
			 *
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
					/**
					 * Empty start
					 *
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
					/**
					 * Empty end
					 *
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
					/**
					 * Empty start, goes to the LAST
					 * element
					 *
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
					/**
					 * Empty end
					 *
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

			XSAFELBSTR(ptrs[cur], buff, bufc);
		}
	}

	XFREE(wordlist);
	XFREE(ptrs);
	XFREE(mapper);
}

/*
 * ---------------------------------------------------------------------------
 * fun_grab:
 */

/**
 * @brief a combination of extract() and match(), sortof. We grab the
 * single element that we match.
 *
 * grab(Test:1 Ack:2 Foof:3,*:2)    => Ack:2
 * grab(Test-1+Ack-2+Foof-3,*o*,+)  => Ack:2
 *
 * fun_graball: Ditto, but like matchall() rather than match(). We grab all the
 * elements that match, and we can take an output delimiter.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_grab(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *r, *s;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	/**
	 * Walk the wordstring, until we find the word we want.
	 *
	 */
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			XSAFELBSTR(r, buff, bufc);
			return;
		}
	} while (s);
}

/**
 * @brief Like grab with matchall() rather than match(). We grab all the
 * elements that match, and we can take an output delimiter.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_graball(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *r, *s, *bb_p;
	Delim isep, osep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
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

			XSAFELBSTR(r, buff, bufc);
		}
	} while (s);
}

/**
 * @brief swaps two points to strings
 *
 * @param p First points to strings
 * @param q Second points to strings
 */
void swap(char **p, char **q)
{
	char *temp;
	temp = *p;
	*p = *q;
	*q = temp;
}

/**
 * @brief randomize order of words in a list.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_shuffle(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **words;
	int n, i, j;
	Delim isep, osep;

	if (!nfargs || !fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 3, 2, DELIM_STRING, &isep))
	{
		return;
	}

	if (nfargs < 3)
	{
		copy_delim(&osep, &isep);
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

/**
 * @brief If a <word> in <list of words> is in <old words>, replace it with the
 * corresponding word from <new words>. This is basically a mass-edit. This
 * is an EXACT, not a case-insensitive or wildcarded, match.
 *
 * ledit(<list of words>,<old words>,<new words>[,<delim>[,<output delim>]])
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
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

	old_list = XMALLOC(LBUF_SIZE, "old_list");
	new_list = XMALLOC(LBUF_SIZE, "new_list");
	XSTRCPY(old_list, fargs[1]);
	XSTRCPY(new_list, fargs[2]);
	nptrs_old = list2arr(&ptrs_old, LBUF_SIZE / 2, old_list, &isep);
	nptrs_new = list2arr(&ptrs_new, LBUF_SIZE / 2, new_list, &isep);

	/**
	 * Iterate through the words.
	 *
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
					/**
					 * If we specify more old words than
					 * we have new words, we assume we
					 * want to just nullify.
					 *
					 */
					XSAFELBSTR(ptrs_new[i], buff, bufc);
				}

				break;
			}
		}

		if (!got_it)
		{
			XSAFELBSTR(r, buff, bufc);
		}
	} while (s);

	XFREE(old_list);
	XFREE(new_list);
	XFREE(ptrs_old);
	XFREE(ptrs_new);
}
/**
 * @brief Turn a list into a punctuated list.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
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

	conj_str = (nfargs < 3) ? (char *)"and" : fargs[2];

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
		XSAFELBSTR(elems[0], buff, bufc);
	}
	else if (n_elems == 2)
	{
		XSAFELBSTR(elems[0], buff, bufc);

		if (*conj_str)
		{
			XSAFELBCHR(' ', buff, bufc);
			XSAFELBSTR(conj_str, buff, bufc);
		}

		XSAFELBCHR(' ', buff, bufc);
		XSAFELBSTR(elems[1], buff, bufc);
	}
	else
	{
		for (i = 0; i < (n_elems - 1); i++)
		{
			XSAFELBSTR(elems[i], buff, bufc);
			print_separator(&osep, buff, bufc);
			XSAFELBCHR(' ', buff, bufc);
		}

		if (*conj_str)
		{
			XSAFELBSTR(conj_str, buff, bufc);
			XSAFELBCHR(' ', buff, bufc);
		}

		XSAFELBSTR(elems[i], buff, bufc);
	}

	XFREE(elems);
}

/**
 * @brief Weighted random choice from a list.
 *
 * choose(<list of items>,<list of weights>,<input delim>)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_choose(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;
	char **elems, **weights;
	int i, num, n_elems, n_weights, *ip;
	int sum = 0;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	n_weights = list2arr(&weights, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);

	if (n_elems != n_weights)
	{
		XSAFELBSTR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
		XFREE(elems);
		XFREE(weights);
		return;
	}

	/**
	 * Store the breakpoints, not the choose weights themselves.
	 *
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

	num = (int)random_range(0, (sum)-1);

	for (i = 0; i < n_weights; i++)
	{
		if ((ip[i] != 0) && (num < ip[i]))
		{
			XSAFELBSTR(elems[i], buff, bufc);
			break;
		}
	}

	XFREE(ip);
	XFREE(elems);
	XFREE(weights);
}

/**
 * @brief Sort a list by numerical-size group, i.e., take every Nth
 * element. Useful for passing to a column-type function where you want the
 * list to go down rather than across, for instance.
 *
 * group(<list>, <number of groups>, <idelim>, <odelim>, <gdelim>)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_group(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p, **elems;
	Delim isep, osep, gsep;
	int n_elems, n_groups, i, j;

	/**
	 * Separator checking is weird in this, since we can delimit by
	 * group, too, as well as the element delimiter. The group delimiter
	 * defaults to the output delimiter.
	 *
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
		copy_delim(&osep, &isep);
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
		copy_delim(&gsep, &osep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &gsep, DELIM_NULL | DELIM_CRLF | DELIM_STRING))
		{
			return;
		}
	}

	/**
	 * Go do it, unless the group size doesn't make sense.
	 *
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

			XSAFELBSTR(elems[i + j], buff, bufc);
		}
	}

	XFREE(elems);
}

/**
 * @brief Take a string such as 'this "Joe Bloggs" John' and turn it
 * into an output delim-separated list.
 *
 * tokens(<string>[,<obj>/<attr>][,<open>][,<close>][,<sep>][,<osep>])
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
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
		copy_delim(&cmark, &omark);
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
		copy_delim(&osep, &isep);
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
			alen = strlen(fargs[1] + 8);
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
			/**
			 * Now we're inside quotes. Find the end quote, and
			 * copy the token inside of it. If we run off the end
			 * of the string, we ignore the literal opening
			 * marker that we've skipped.
			 *
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
			/**
			 * We are at a bare word. Split it off.
			 *
			 */
			t = split_token(&s, &isep);
		}

		/**
		 * Pass the token through the transformation function if we
		 * have one, or just copy it, if not.
		 *
		 */
		if (t)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			if (!atextbuf)
			{
				XSAFELBSTR(t, buff, bufc);
			}
			else if ((mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
			{
				objs[0] = t;
				XMEMCPY(atextbuf, atext, alen);
				atextbuf[alen] = '\0';
				str = atextbuf;
				eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 1);
			}
		}

		/**
		 * Skip to start of next token, ignoring input separators.
		 *
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
