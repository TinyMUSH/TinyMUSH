
/**
 * @file funstring.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief String manipulation built-ins: core string operations, case handling, comparison, and utility functions
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
#include "ansi.h"

#include <ctype.h>
#include <string.h>

static const ColorState color_normal = {
	.foreground = {.is_set = ColorStatusReset},
	.background = {.is_set = ColorStatusReset}};

static const ColorState color_none = {0};

/**
 * @brief Just eat the contents of the string. Handy for those times when
 *        you've output a bunch of junk in a function call and just want to
 *        dispose of the output (like if you've done an iter() that just did
 *        a bunch of side-effects, and now you have bunches of spaces you need
 *        to get rid of.
 *
 * @param buff Not used
 * @param bufc Not used
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_null(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
}

/**
 * @brief Squash occurrences of a given character down to 1. We do this
 *        both on leading and trailing chars, as well as internal ones; if the
 *        player wants to trim off the leading and trailing as well, they can
 *        always call trim().
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
void fun_squish(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tp = NULL, *bp = NULL;
	Delim isep;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, 0))
	{
		return;
	}

	bp = tp = fargs[0];

	while (*tp)
	{
		/**
		 * Move over and copy the non-sep characters
		 *
		 */
		while (*tp && *tp != isep.str[0])
		{
			if (*tp == C_ANSI_ESC)
			{
				char *esc_start = tp;
				skip_esccode(&tp);
				size_t esc_len = (size_t)(tp - esc_start);
				memmove(bp, esc_start, esc_len);
				bp += esc_len;
			}
			else
			{
				*bp++ = *tp++;
			}
		}

		/**
		 * If we've reached the end of the string, leave the loop.
		 *
		 */
		if (!*tp)
		{
			break;
		}

		/**
		 * Otherwise, we've hit a sep char. Move over it, and then
		 * move on to the next non-separator. Note that we're
		 * overwriting our own string as we do this. However, the
		 * other pointer will always be ahead of our current copy
		 * pointer.
		 *
		 */
		*bp++ = *tp++;

		while (*tp && (*tp == isep.str[0]))
		{
			tp++;
		}
	}

	/**
	 * Must terminate the string
	 *
	 */
	*bp = '\0';
	XSAFELBSTR(fargs[0], buff, bufc);
}

/**
 * @brief Trim off unwanted white space.
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
void fun_trim(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = NULL, *q = NULL, *endchar = NULL, *ep = NULL;
	int trim = 0;
	Delim isep;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

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

	/**
	 * Single-character delimiters are easy.
	 *
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
				if (*q == C_ANSI_ESC)
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

		XSAFELBSTR(p, buff, bufc);
		return;
	}

	/**
	 * Multi-character delimiters take more work.
	 *
	 */
	ep = p + strlen(fargs[0]) - 1; /*!< last char in string */

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
			if (*q == C_ANSI_ESC)
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

	XSAFELBSTR(p, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

/**
 * @brief Lowercase string
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_lcstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *ap = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	while (*ap)
	{
		if (*ap == C_ANSI_ESC)
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

/**
 * @brief Uppercase string
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ucstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *ap = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	while (*ap)
	{
		if (*ap == C_ANSI_ESC)
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

/**
 * @brief Capitalize string
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_capstr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *ap = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	while (*ap == C_ANSI_ESC)
	{
		skip_esccode(&ap);
	}

	*ap = toupper(*ap);
}

/**
 * @brief If the line ends with CRLF, CR, or LF, chop it off
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_chomp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	if (*bufc != bb_p && (*bufc)[-1] == '\n')
	{
		(*bufc)--;
	}

	if (*bufc != bb_p && (*bufc)[-1] == '\r')
	{
		(*bufc)--;
	}
}

/**
 * @brief Exact-string compare
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_comp(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int x = strcmp(fargs[0], fargs[1]);

	if (x > 0)
	{
		XSAFELBCHR('1', buff, bufc);
	}
	else if (x < 0)
	{
		XSAFELBSTR("-1", buff, bufc);
	}
	else
	{
		XSAFELBCHR('0', buff, bufc);
	}
}

/**
 * @brief non-case-sensitive string compare
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_streq(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFEBOOL(buff, bufc, !string_compare(fargs[0], fargs[1]));
}

/**
 * @brief wildcard string compare
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_strmatch(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/**
	 * Check if we match the whole string.  If so, return 1
	 *
	 */
	XSAFEBOOL(buff, bufc, quick_wild(fargs[1], fargs[0]));
}

/**
 * @brief Edit text.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_edit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tstr = NULL;

	edit_string(fargs[0], &tstr, fargs[1], fargs[2], player, cause);
	XSAFELBSTR(tstr, buff, bufc);
	XFREE(tstr);
}

/**
 * @brief Given two strings and a character, merge the two strings by
 *        replacing characters in string1 that are the same as the given
 *        character by the corresponding character in string2 (by position).
 *        The strings must be of the same length.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_merge(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str = NULL, *rep = NULL, c = 0;

	/**
	 * do length checks first
	 *
	 */
	if (strlen(fargs[0]) != strlen(fargs[1]))
	{
		XSAFELBSTR("#-1 STRING LENGTHS MUST BE EQUAL", buff, bufc);
		return;
	}

	if (strlen(fargs[2]) > 1)
	{
		XSAFELBSTR("#-1 TOO MANY CHARACTERS", buff, bufc);
		return;
	}

	/**
	 * find the character to look for. null character is considered a space
	 *
	 */
	if (!*fargs[2])
	{
		c = ' ';
	}
	else
	{
		c = *fargs[2];
	}

	/**
	 * walk strings, copy from the appropriate string
	 *
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
 * fun_reverse: reverse a string
 */

void fun_reverse(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int n, i, num_transitions;
	ColorState *color_states;
	char *stripped;
	char escape_buffer[256];
	size_t escape_offset;
	ColorType color_type = ColorTypeNone;
	dbref color_target = (cause != NOTHING) ? cause : player;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	// Determine color type for output
	// Always process colors at highest fidelity: they get converted to player-appropriate level at send time.
	color_type = ColorTypeTrueColor;

	n = ansi_map_states_colorstate(fargs[0], &color_states, &stripped);
	num_transitions = 0;

	// Reverse iteration through characters
	for (i = n - 1; i >= 0; i--)
	{
		// Output color transition if state changed
		ColorState before = (i < n - 1) ? color_states[i + 1] : (ColorState){0};
		if (memcmp(&before, &color_states[i], sizeof(ColorState)) != 0)
		{
			num_transitions++;
			escape_offset = 0;
			to_ansi_escape_sequence(escape_buffer, sizeof(escape_buffer), &escape_offset, &color_states[i], color_type);
			if (escape_offset > 0)
			{
				XSAFELBSTR(escape_buffer, buff, bufc);
			}
		}

		// Output the reversed character
		XSAFELBCHR(stripped[i], buff, bufc);
	}

	// Reset to normal at the end, if required
	escape_offset = 0;
	ColorState reset_state = (ColorState){.reset = ColorStatusReset};
	to_ansi_escape_sequence(escape_buffer, sizeof(escape_buffer), &escape_offset, &reset_state, color_type);
	if (escape_offset > 0 && num_transitions > 0)
	{
		XSAFELBSTR(escape_buffer, buff, bufc);
	}

	XFREE(color_states);
	XFREE(stripped);
}

/*
 * ---------------------------------------------------------------------------
 * fun_mid: mid(foobar,2,3) returns oba
 */

void fun_mid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	ColorState *states = NULL;
	char *stripped = NULL;
	ColorState normal = color_normal;
	ColorType color_type = ColorTypeTrueColor;
	int start = (int)strtol(fargs[1], (char **)NULL, 10);
	int nchars = (int)strtol(fargs[2], (char **)NULL, 10);

	if (nchars <= 0)
	{
		return;
	}

	int len = ansi_map_states_colorstate(fargs[0], &states, &stripped);

	if (start < 0)
	{
		nchars += start;
		start = 0;
	}

	if (nchars <= 0 || start >= len)
	{
		goto cleanup_mid;
	}

	int end = start + nchars;
	if (end > len)
	{
		end = len;
	}

	emit_colored_range(stripped, states, start, end, normal, normal, color_type, buff, bufc);

cleanup_mid:
	if (states)
	{
		XFREE(states);
	}

	if (stripped)
	{
		XFREE(stripped);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_translate: Takes a string and a second argument. If the second
 * argument is 0 or s, control characters are converted to spaces. If it's 1
 * or p, they're converted to percent substitutions.
 */

void fun_translate(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	/*
	 * Strictly speaking, we're just checking the first char
	 */

	if (nfargs > 1 && (fargs[1][0] == 's' || fargs[1][0] == '0'))
	{
		char *s = translate_string_ansi(fargs[0], 0);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
	else if (nfargs > 1 && fargs[1][0] == 'p')
	{
		char *s = translate_string_ansi(fargs[0], 1);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
	else
	{
		char *s = translate_string_ansi(fargs[0], 1);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
}

/*
 * ---------------------------------------------------------------------------
 * String concatenation.
 */

void fun_cat(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i;
	XSAFELBSTR(fargs[0], buff, bufc);

	for (i = 1; i < nfargs; i++)
	{
		XSAFELBCHR(' ', buff, bufc);
		XSAFELBSTR(fargs[i], buff, bufc);
	}
}

void fun_strcat(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i;
	XSAFELBSTR(fargs[0], buff, bufc);

	for (i = 1; i < nfargs; i++)
	{
		XSAFELBSTR(fargs[i], buff, bufc);
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

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	bb_p = *bufc;

	for (i = 1; i < nfargs; i++)
	{
		if (*fargs[i])
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELBSTR(fargs[i], buff, bufc);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Misc functions.
 */

void fun_strlen(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, ansi_strip_ansi_len(fargs[0]), LBUF_SIZE);
}

void fun_delete(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	ColorState *states = NULL;
	char *stripped = NULL;
	ColorState normal = color_normal;
	ColorType color_type = ColorTypeTrueColor;
	int start = (int)strtol(fargs[1], (char **)NULL, 10);
	int nchars = (int)strtol(fargs[2], (char **)NULL, 10);

	if ((nchars <= 0) || (start + nchars <= 0))
	{
		XSAFELBSTR(fargs[0], buff, bufc);
		return;
	}

	int len = ansi_map_states_colorstate(fargs[0], &states, &stripped);

	int start_idx = (start < 0) ? 0 : start;
	if (start_idx > len)
	{
		start_idx = len;
	}

	int delete_len = (start < 0) ? start + nchars : nchars;
	if (delete_len < 0)
	{
		delete_len = 0;
	}

	int end_idx = start_idx + delete_len;
	if (end_idx > len)
	{
		end_idx = len;
	}

	emit_colored_range(stripped, states, 0, start_idx, normal, states ? states[start_idx] : normal, color_type, buff, bufc);
	emit_colored_range(stripped, states, end_idx, len, states ? states[start_idx] : normal, normal, color_type, buff, bufc);

	if (states)
	{
		XFREE(states);
	}

	if (stripped)
	{
		XFREE(stripped);
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
	XSAFELBSTR(fargs[0], buff, bufc);
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
		if (*s == C_ANSI_ESC)
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
		XSAFESTRNCAT(buff, bufc, "an", 2, LBUF_SIZE);
	}
	else
	{
		XSAFELBCHR('a', buff, bufc);
	}
}

void fun_alphamax(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *amax;
	int i = 1;

	if (!fargs[0])
	{
		XSAFELBSTR("#-1 TOO FEW ARGUMENTS", buff, bufc);
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

	XSAFELBSTR(amax, buff, bufc);
}

void fun_alphamin(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *amin;
	int i = 1;

	if (!fargs[0])
	{
		XSAFELBSTR("#-1 TOO FEW ARGUMENTS", buff, bufc);
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

	XSAFELBSTR(amin, buff, bufc);
}

void fun_valid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	 * Checks to see if a given <something> is valid as a parameter of a
	 * given type (such as an object name).
	 */
	if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1])
	{
		XSAFELBCHR('0', buff, bufc);
	}
	else if (!strcasecmp(fargs[0], "name"))
	{
		XSAFEBOOL(buff, bufc, ok_name(fargs[1]));
	}
	else if (!strcasecmp(fargs[0], "attrname"))
	{
		XSAFEBOOL(buff, bufc, ok_attr_name(fargs[1]));
	}
	else if (!strcasecmp(fargs[0], "playername"))
	{
		XSAFEBOOL(buff, bufc, (ok_player_name(fargs[1]) && badname_check(fargs[1])));
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}
}

void fun_beep(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELBCHR(BEEP_CHAR, buff, bufc);
}
