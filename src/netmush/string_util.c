/**
 * @file string_util.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core string utilities: safe buffer operations, munging, and helper routines
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

static const ColorState color_none = {0};

/**
 * @brief Consume an ANSI escape sequence and update ColorState
 *
 * @param cursor Pointer to the current position in the string
 * @param state Pointer to ColorState to update
 */
static inline void consume_ansi_sequence_state(char **cursor, ColorState *state)
{
	const char *ptr = *cursor;

	if (ansi_apply_sequence(&ptr, state))
	{
		*cursor = (char *)ptr;
	}
}

/**
 * @brief Thread-safe wrapper for strerror
 *
 * @param errnum Error number
 * @return const char* Error message string
 */
const char *safe_strerror(int errnum)
{
	static __thread char errbuf[256];
	if (strerror_r(errnum, errbuf, sizeof(errbuf)) != 0)
	{
		snprintf(errbuf, sizeof(errbuf), "Unknown error %d", errnum);
	}
	return errbuf;
}

/**
 * @brief High-resolution timer using clock_gettime
 * Returns struct timeval for compatibility with existing code
 *
 * @param tv Pointer to timeval structure to fill
 * @return int 0 on success, -1 on error
 */
int safe_gettimeofday(struct timeval *tv)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
	{
		return -1;
	}
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / 1000;
	return 0;
}

/**
 * @brief Capitalize an entire string
 *
 * @param s The string to capitalize
 * @return char* The capitalized string (same pointer as input)
 */
char *upcasestr(char *s)
{
	char *p;

	for (p = s; p && *p; p++)
	{
		*p = toupper((unsigned char)*p);
	}

	return s;
}

/**
 * @brief Compress multiple spaces into one and remove leading/trailing spaces
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param string The string to munge
 * @return char* The munged string (newly allocated)
 */
char *munge_space(char *string)
{
	char *buffer, *p, *q;
	buffer = XMALLOC(LBUF_SIZE, "buffer");
	p = string;
	q = buffer;

	while (p && *p && isspace((unsigned char)*p))
	{
		p++;
	}

	/* remove initial spaces */

	while (p && *p)
	{
		while (*p && !isspace((unsigned char)*p))
		{
			XSAFELBCHR(*p, buffer, &q);
			++p;
		}

		while (*p && isspace((unsigned char)*++p))
			;

		if (*p)
		{
			XSAFELBCHR(' ', buffer, &q);
		}
	}

	*q = '\0';
	/* remove terminal spaces and terminate string */
	return (buffer);
}

/**
 * @brief Remove leading and trailing spaces
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param string The string to trim
 * @return char* The trimmed string (newly allocated)
 */
char *trim_spaces(char *string)
{
	char *buffer, *p, *q;
	buffer = XMALLOC(LBUF_SIZE, "buffer");
	p = string;
	q = buffer;

	while (p && *p && isspace((unsigned char)*p))
	{
		p++; /* remove inital spaces */
	}

	while (p && *p)
	{
		while (*p && !isspace((unsigned char)*p))
		{
			XSAFELBCHR(*p, buffer, &q); /* copy nonspace chars */
			++p;
		}

		while (*p && isspace((unsigned char)*p))
		{
			p++; /* compress spaces */
		}

		if (*p)
		{
			XSAFELBCHR(' ', buffer, &q); /* leave one space */
		}
	}

	*q = '\0'; /* terminate string */
	return (buffer);
}

/**
 * @brief Return portion of a string up to the indicated character
 *
 * Note that str will be moved to the position following the searched
 * character. If you need the original string, save it before calling
 * this function.
 *
 * @param str Pointer to the string pointer to search
 * @param targ The character to search for
 * @return char* The string up to the character, or NULL if str is invalid
 */
char *grabto(char **str, char targ)
{
	char *savec, *cp;

	if (!str || !*str || !**str)
	{
		return NULL;
	}

	savec = cp = *str;

	while (*cp && *cp != targ)
	{
		cp++;
	}

	if (*cp)
	{
		*cp++ = '\0';
	}

	*str = cp;
	return savec;
}

/**
 * @brief Compare two strings case-insensitively, treating multiple spaces as one
 *
 * @param s1 The first string to compare
 * @param s2 The second string to compare
 * @return int 0 if strings match, non-zero otherwise
 */
int string_compare(const char *s1, const char *s2)
{
	if (mushstate.standalone || mushconf.space_compress)
	{
		while (isspace((unsigned char)*s1))
		{
			s1++;
		}

		while (isspace((unsigned char)*s2))
		{
			s2++;
		}

		while (*s1 && *s2 && ((tolower((unsigned char)*s1) == tolower((unsigned char)*s2)) || (isspace((unsigned char)*s1) && isspace((unsigned char)*s2))))
		{
			if (isspace((unsigned char)*s1) && isspace((unsigned char)*s2))
			{
				/* skip all other spaces */
				while (isspace((unsigned char)*s1))
				{
					s1++;
				}

				while (isspace((unsigned char)*s2))
				{
					s2++;
				}
			}
			else
			{
				s1++;
				s2++;
			}
		}

		if ((*s1) && (*s2))
		{
			return (1);
		}

		if (isspace((unsigned char)*s1))
		{
			while (isspace((unsigned char)*s1))
			{
				s1++;
			}

			return (*s1);
		}

		if (isspace((unsigned char)*s2))
		{
			while (isspace((unsigned char)*s2))
			{
				s2++;
			}

			return (*s2);
		}

		if ((*s1) || (*s2))
		{
			return (1);
		}

		return (0);
	}
	else
	{
		while (*s1 && *s2 && tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
		{
			s1++, s2++;
		}

		return (tolower((unsigned char)*s1) - tolower((unsigned char)*s2));
	}
}

/**
 * @brief Compare a string with a prefix case-insensitively
 *
 * @param string The string to search
 * @param prefix The prefix to search for
 * @return int Number of matching characters if prefix found, 0 otherwise
 */
int string_prefix(const char *string, const char *prefix)
{
	int count = 0;

	while (*string && *prefix && tolower((unsigned char)*string) == tolower((unsigned char)*prefix))
	{
		string++, prefix++, count++;
	}

	if (*prefix == '\0')
	{ /* Matched all of prefix */
		return (count);
	}
	else
	{
		return (0);
	}
}

/**
 * @brief Find a substring within a string, matching only at word boundaries
 *
 * Accepts only nonempty matches starting at the beginning of a word.
 *
 * @param src The string to search
 * @param sub The substring to find
 * @return const char* Pointer to the match position, or NULL if not found
 */
const char *string_match(const char *src, const char *sub)
{
	if ((*sub != '\0') && (src))
	{
		while (*src)
		{
			if (string_prefix(src, sub))
			{
				return src;
			}

			/*  else scan to beginning of next word */

			while (*src && isalnum((unsigned char)*src))
			{
				src++;
			}

			while (*src && !isalnum((unsigned char)*src))
			{
				src++;
			}
		}
	}

	return 0;
}

/**
 * @brief Replace all occurrences of a substring with a new substring
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param old The string to replace
 * @param new The replacement string
 * @param string The string to modify
 * @return char* The modified string (newly allocated)
 */
char *replace_string(const char *old, const char *new, const char *string)
{
	char *result, *r, *s;
	int olen;
	r = result = XMALLOC(LBUF_SIZE, "result");

	if (string != NULL)
	{
		s = (char *)string;
		olen = strlen(old);

		while (*s)
		{
			/* Copy up to the next occurrence of the first char of OLD */
			while (*s && *s != *old)
			{
				XSAFELBCHR(*s, result, &r);
				s++;
			}

			/*
			 * If we are really at an OLD, append NEW to the result and
			 * bump the input string past the occurrence of
			 * OLD. Otherwise, copy the char and try again.
			 */

			if (*s)
			{
				if (!strncmp(old, s, olen))
				{
					XSAFELBSTR((char *)new, result, &r);
					s += olen;
				}
				else
				{
					XSAFELBCHR(*s, result, &r);
					s++;
				}
			}
		}
	}

	*r = '\0';
	return result;
}

/**
 * @brief Replace all occurrences of a substring, handling ANSI codes and special ^ and $ cases
 *
 * Special cases:
 * - from="^" prepends 'to' to the string
 * - from="$" appends 'to' to the string
 * - from="\^" or from="\$" matches literal ^ or $ characters
 *
 * Caller is responsible for freeing *dst with XFREE().
 *
 * @param src The original string
 * @param dst Pointer to receive the new string (newly allocated)
 * @param from The substring to replace
 * @param to The replacement substring
 * @param player DBref of the player for color type resolution
 * @param cause DBref of the cause for color type resolution
 */
void edit_string(char *src, char **dst, char *from, char *to, dbref player, dbref cause)
{
	char *cp, *p;
	ColorState ansi_state = {0};
	ColorState to_color_state = {0};
	int tlen, flen;
	/*
	 * We may have gotten an C_ANSI_NORMAL_SEQ termination to OLD and NEW,
	 * that the user probably didn't intend to be there. (If the
	 * user really did want it there, he simply has to put a double
	 * C_ANSI_NORMAL_SEQ in; this is non-intuitive but without it we can't
	 * let users swap one ANSI code for another using this.)  Thus,
	 * we chop off the terminating C_ANSI_NORMAL_SEQ on both, if there is
	 * one.
	 */
	p = from + strlen(from) - 4;

	if (p >= from && !strcmp(p, C_ANSI_NORMAL_SEQ))
	{
		*p = '\0';
	}

	p = to + strlen(to) - 4;

	if (p >= to && !strcmp(p, C_ANSI_NORMAL_SEQ))
	{
		*p = '\0';
	}

	/*
	 * Scan the contents of the TO string. Figure out whether we
	 * have any embedded ANSI codes.
	 */
	to_color_state = (ColorState){0};
	ColorType color_type = resolve_color_type(player, cause);

	do
	{
		p = to;
		while (*p)
		{
			if (*p == C_ANSI_ESC)
			{
				consume_ansi_sequence_state(&p, &to_color_state);
			}
			else
			{
				++p;
			}
		}
	} while (0);

	tlen = p - to;
	/* Do the substitution.  Idea for prefix/suffix from R'nice@TinyTIM */
	cp = *dst = XMALLOC(LBUF_SIZE, "edit_string_cp");

	if (!strcmp(from, "^"))
	{
		/* Prepend 'to' to string */
		XSAFESTRNCAT(*dst, &cp, to, tlen, LBUF_SIZE);

		do
		{
			p = src;
			while (*p)
			{
				if (*p == C_ANSI_ESC)
				{
					consume_ansi_sequence_state(&p, &ansi_state);
				}
				else
				{
					++p;
				}
			}
		} while (0);

		XSAFESTRNCAT(*dst, &cp, src, p - src, LBUF_SIZE);
	}
	else if (!strcmp(from, "$"))
	{
		/* Append 'to' to string */
		ansi_state = (ColorState){0};

		do
		{
			p = src;
			while (*p)
			{
				if (*p == C_ANSI_ESC)
				{
					consume_ansi_sequence_state(&p, &ansi_state);
				}
				else
				{
					++p;
				}
			}
		} while (0);

		XSAFESTRNCAT(*dst, &cp, src, p - src, LBUF_SIZE);
		// Apply to_color_state to ansi_state
		ansi_state.highlight = to_color_state.highlight;
		ansi_state.underline = to_color_state.underline;
		ansi_state.flash = to_color_state.flash;
		ansi_state.inverse = to_color_state.inverse;
		if (to_color_state.foreground.is_set)
		{
			ansi_state.foreground = to_color_state.foreground;
		}
		if (to_color_state.background.is_set)
		{
			ansi_state.background = to_color_state.background;
		}
		XSAFESTRNCAT(*dst, &cp, to, tlen, LBUF_SIZE);
	}
	else
	{
		/*
		 * Replace all occurances of 'from' with 'to'.  Handle the
		 * special cases of from = \$ and \^.
		 */
		if (((from[0] == '\\') || (from[0] == '%')) && ((from[1] == '$') || (from[1] == '^')) && (from[2] == '\0'))
		{
			from++;
		}

		flen = strlen(from);
		ansi_state = (ColorState){0};

		while (*src)
		{
			/* Copy up to the next occurrence of the first char of FROM. */
			p = src;

			while (*src && (*src != *from))
			{
				if (*src == C_ANSI_ESC)
				{
					consume_ansi_sequence_state(&src, &ansi_state);
				}
				else
				{
					++src;
				}
			}

			XSAFESTRNCAT(*dst, &cp, p, src - p, LBUF_SIZE);

			/*
			 * If we are really at a FROM, append TO to the result
			 * and bump the input string past the occurrence of
			 * FROM. Otherwise, copy the char and try again.
			 */

			if (*src)
			{
				if (!strncmp(from, src, flen))
				{
					/* Apply whatever ANSI transition happens in TO */
					// Apply to_color_state to ansi_state
					ansi_state.highlight = to_color_state.highlight;
					ansi_state.underline = to_color_state.underline;
					ansi_state.flash = to_color_state.flash;
					ansi_state.inverse = to_color_state.inverse;
					if (to_color_state.foreground.is_set)
					{
						ansi_state.foreground = to_color_state.foreground;
					}
					if (to_color_state.background.is_set)
					{
						ansi_state.background = to_color_state.background;
					}
					XSAFESTRNCAT(*dst, &cp, to, tlen, LBUF_SIZE);
					src += flen;
				}
				else
				{
					/*
					 * We have to handle the case where
					 * the first character in FROM is the
					 * ANSI escape character. In that case
					 * we move over and copy the entire
					 * ANSI code. Otherwise we just copy
					 * the character.
					 */
					if (*from == C_ANSI_ESC)
					{
						p = src;
						consume_ansi_sequence_state(&src, &ansi_state);
						XSAFESTRNCAT(*dst, &cp, p, src - p, LBUF_SIZE);
					}
					else
					{
						XSAFELBCHR(*src, *dst, &cp);
						++src;
					}
				}
			}
		}
	}

	p = ansi_transition_colorstate(ansi_state, color_none, color_type, false);
	XSAFELBSTR(p, *dst, &cp);
	XFREE(p);
}

/**
 * @brief Find if a substring matches at the start of a string with minimum length
 *
 * At least min characters must match. This is how flags are matched
 * by the command queue.
 *
 * @param str The string to search
 * @param target The target substring
 * @param min Minimum number of characters that must match
 * @return int 1 if found, 0 if not
 */
int minmatch(char *str, char *target, int min)
{
	while (*str && *target && (tolower((unsigned char)*str) == tolower((unsigned char)*target)))
	{
		str++;
		target++;
		min--;
	}

	if (*str)
	{
		return 0;
	}

	if (!*target)
	{
		return 1;
	}

	return ((min <= 0) ? 1 : 0);
}

/**
 * @brief Check if a pattern is found in the exit list
 *
 * @param exit_list Semicolon-separated list of exit names
 * @param pattern The pattern to search for
 * @return int 1 if the pattern is found, 0 otherwise
 */
int matches_exit_from_list(char *exit_list, char *pattern)
{
	char *s;

	if (*exit_list == '\0')
	{ /* never match empty */
		return 0;
	}

	while (*pattern)
	{
		/* check out this one */
		for (s = exit_list; (*s && (tolower((unsigned char)*s) == tolower((unsigned char)*pattern)) && *pattern && (*pattern != EXIT_DELIMITER)); s++, pattern++)
			;

		/* Did we match it all? */

		if (*s == '\0')
		{
			/* Make sure nothing afterwards */
			while (*pattern && isspace((unsigned char)*pattern))
			{
				pattern++;
			}

			/* Did we get it? */

			if (!*pattern || (*pattern == EXIT_DELIMITER))
			{
				return 1;
			}
		}

		/* We didn't get it, find next string to test */

		while (*pattern && *pattern++ != EXIT_DELIMITER)
			;

		while (isspace((unsigned char)*pattern))
		{
			pattern++;
		}
	}

	return 0;
}

/**
 * @brief Convert a long signed number to string
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 * Uses Mark Vasoll's long int to string converter algorithm.
 *
 * @param num Number to convert
 * @return char* String representation of the number (newly allocated)
 */
char *ltos(long num)
{
	/* Mark Vasoll's long int to string converter. */
	char *buf, *p, *dest, *destp;
	unsigned long anum;
	p = buf = XMALLOC(SBUF_SIZE, "ltos_buf");
	destp = dest = XMALLOC(SBUF_SIZE, "ltos_dest");
	/* absolute value (avoid undefined behavior on LONG_MIN) */
	if (num < 0)
	{
		anum = (unsigned long)(-(num + 1)) + 1;
	}
	else
	{
		anum = (unsigned long)num;
	}

	/* build up the digits backwards by successive division */
	while (anum > 9)
	{
		*p++ = '0' + (anum % 10);
		anum /= 10;
	}

	/* put in the sign if needed */
	if (num < 0)
	{
		*destp++ = '-';
	}

	/* put in the last digit, this makes very fast single digits numbers */
	*destp++ = '0' + (char)anum;

	/* reverse the rest of the digits (if any) into the provided buf */
	while (p-- > buf)
	{
		*destp++ = *p;
	}

	/* terminate the resulting string */
	*destp = '\0';
	XFREE(buf);
	return (dest);
}

/**
 * @brief Create a string filled with a repeated character
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param count Number of characters (clamped to 0..LBUF_SIZE-1)
 * @param ch Character to repeat
 * @return char* String of repeated characters (newly allocated)
 */
char *repeatchar(int count, char ch)
{
	char *str, *ptr;
	ptr = str = XMALLOC(LBUF_SIZE, "repeatchar_ptr");

	if (count < 0)
	{
		count = 0;
	}
	else if (count >= LBUF_SIZE)
	{
		count = LBUF_SIZE - 1;
	}

	if (count > 0)
	{
		for (; str < ptr + count; str++)
		{
			*str = ch;
		}
	}

	*str = '\0';
	return ptr;
}

/**
 * @brief Advances a string pointer past any leading whitespace characters.
 *
 * This function modifies the provided pointer to skip over consecutive whitespace
 * characters (as defined by isspace()) at the beginning of the string. It stops
 * when a non-whitespace character is encountered or the end of the string is reached.
 *
 * @param pBuf Pointer to the string buffer.
 * @return const char* Pointer to the first non-whitespace character.
 */
const char *skip_whitespace(const char *pBuf)
{
	while (*pBuf && isspace(*pBuf))
	{
		pBuf++;
	}
	return pBuf;
}