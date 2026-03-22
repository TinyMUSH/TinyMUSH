/**
 * @file funstring_search.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief String search and position functions
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

/*
 * ---------------------------------------------------------------------------
 * fun_after, fun_before: Return substring after or before a specified
 * string.
 */

/**
 * @brief Return substring after a specified string.
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
void fun_after(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *haystack = NULL;
	char *needle = NULL;
	ColorState *hay_states = NULL;
	ColorState *needle_states = NULL;
	char *hay_text = NULL;
	char *needle_text = NULL;
	ColorState normal = color_normal;
	ColorType color_type = ColorTypeTrueColor;
	ColorState zero_state = {0};
	int match_pos = -1;
	bool needle_has_color = false;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	haystack = fargs[0];
	needle = fargs[1];

	if (!haystack)
	{
		haystack = "";
	}

	if (needle == NULL)
	{
		needle = " ";
	}

	if (!needle || !*needle)
	{
		needle = (char *)" ";
	}

	if ((needle[0] == ' ') && (needle[1] == '\0'))
	{
		haystack = Eat_Spaces(haystack);
	}

	int hay_len = ansi_map_states_colorstate(haystack, &hay_states, &hay_text);
	int needle_len = ansi_map_states_colorstate(needle, &needle_states, &needle_text);

	if (needle_len == 0 || hay_len < needle_len)
	{
		goto cleanup_after;
	}

	for (int i = 0; i < needle_len; ++i)
	{
		if (!colorstate_equal(&needle_states[i], &zero_state))
		{
			needle_has_color = true;
			break;
		}
	}

	for (int i = 0; i <= hay_len - needle_len; ++i)
	{
		if (memcmp(hay_text + i, needle_text, (size_t)needle_len) != 0)
		{
			continue;
		}

		if (needle_has_color)
		{
			bool states_match = true;

			for (int j = 0; j < needle_len; ++j)
			{
				if (!colorstate_equal(&hay_states[i + j], &needle_states[j]))
				{
					states_match = false;
					break;
				}
			}

			if (!states_match)
			{
				continue;
			}
		}

		match_pos = i + needle_len;
		break;
	}

	if (match_pos != -1)
	{
		emit_colored_range(hay_text, hay_states, match_pos, hay_len, normal, hay_states[hay_len], color_type, buff, bufc);
	}

cleanup_after:
	if (hay_states)
	{
		XFREE(hay_states);
	}

	if (hay_text)
	{
		XFREE(hay_text);
	}

	if (needle_states)
	{
		XFREE(needle_states);
	}

	if (needle_text)
	{
		XFREE(needle_text);
	}
}

/**
 * @brief Return substring before a specified string.
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
void fun_before(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *haystack = NULL;
	char *needle = NULL;
	ColorState *hay_states = NULL;
	ColorState *needle_states = NULL;
	char *hay_text = NULL;
	char *needle_text = NULL;
	ColorState normal = color_normal;
	ColorType color_type = ColorTypeTrueColor;
	ColorState zero_state = {0};
	int match_pos = -1;
	bool needle_has_color = false;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	haystack = fargs[0]; /*!< haystack */
	needle = fargs[1];	 /*!< needle */

	if (haystack == NULL)
	{
		haystack = "";
	}

	if (needle == NULL)
	{
		needle = " ";
	}

	if (!needle || !*needle)
	{
		needle = (char *)" ";
	}

	if ((needle[0] == ' ') && (needle[1] == '\0'))
	{
		haystack = Eat_Spaces(haystack);
	}

	int hay_len = ansi_map_states_colorstate(haystack, &hay_states, &hay_text);
	int needle_len = ansi_map_states_colorstate(needle, &needle_states, &needle_text);

	if (needle_len == 0 || hay_len < needle_len)
	{
		goto cleanup_before;
	}

	for (int i = 0; i < needle_len; ++i)
	{
		if (!colorstate_equal(&needle_states[i], &zero_state))
		{
			needle_has_color = true;
			break;
		}
	}

	for (int i = 0; i <= hay_len - needle_len; ++i)
	{
		if (memcmp(hay_text + i, needle_text, (size_t)needle_len) != 0)
		{
			continue;
		}

		if (needle_has_color)
		{
			bool states_match = true;

			for (int j = 0; j < needle_len; ++j)
			{
				if (!colorstate_equal(&hay_states[i + j], &needle_states[j]))
				{
					states_match = false;
					break;
				}
			}

			if (!states_match)
			{
				continue;
			}
		}

		match_pos = i;
		break;
	}

	if (match_pos != -1)
	{
		emit_colored_range(hay_text, hay_states, 0, match_pos, normal, normal, color_type, buff, bufc);
	}
	else
	{
		emit_colored_range(hay_text, hay_states, 0, hay_len, normal, hay_states ? hay_states[hay_len] : normal, color_type, buff, bufc);
	}

cleanup_before:
	if (hay_states)
	{
		XFREE(hay_states);
	}

	if (hay_text)
	{
		XFREE(hay_text);
	}

	if (needle_states)
	{
		XFREE(needle_states);
	}

	if (needle_text)
	{
		XFREE(needle_text);
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
	b1 = b = ansi_strip_ansi(fargs[0]);
	s1 = s = ansi_strip_ansi(fargs[1]);

	if (*b && !*(b + 1))
	{ /* single character */
		t = strchr(s, *b);

		if (t)
		{
			XSAFELTOS(buff, bufc, (int)(t - s + 1), LBUF_SIZE);
		}
		else
		{
			XSAFENOTHING(buff, bufc);
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
			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
			XFREE(s1);
			XFREE(b1);
			return;
		}

		++i, ++s;
	}

	XSAFENOTHING(buff, bufc);
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

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

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
	buf = s = ansi_strip_ansi(fargs[0]);

	for (i = 0; *s; i++, s++)
	{
		if (scratch_chartab[(unsigned char)*s])
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
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
		while (*s1 == C_ANSI_ESC)
		{
			skip_esccode(&s1);
		}

		while (*s2 == C_ANSI_ESC)
		{
			skip_esccode(&s2);
		}

		if (*s1 != *s2)
		{
			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
			return;
		}
	}

	XSAFELTOS(buff, bufc, -1, LBUF_SIZE);
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

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	charpos = (int)strtol(fargs[1], (char **)NULL, 10);
	buf = cp = ansi_strip_ansi(fargs[0]);

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

		XSAFELTOS(buff, bufc, i, LBUF_SIZE);
		XFREE(buf);
		return;
	}

	XSAFENOTHING(buff, bufc);
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
	ColorSequence sequences;
	char *result = NULL;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	int charpos = (int)strtol(fargs[1], (char **)NULL, 10);
	if (charpos < 0)
	{
		return;
	}

	// Parse the string for ANSI escape sequences
	if (!ansi_parse_ansi_to_sequences(fargs[0], &sequences))
	{
		// Parsing failed, return empty
		return;
	}

	// Map charpos to position in stripped text
	char *stripped = ansi_strip_ansi(fargs[0]);
	int stripped_len = strlen(stripped);
	if (charpos >= stripped_len)
	{
		XFREE(stripped);
		// Clean up
		if (sequences.text)
			XFREE(sequences.text);
		if (sequences.data)
			XFREE(sequences.data);
		return;
	}
	int stripped_pos = charpos; // 0-based
	XFREE(stripped);

	// Find the ColorSequence that applies at the given character position
	ColorState color_state = {0};
	bool found = false;

	for (size_t i = 0; i < sequences.count; i++)
	{
		if (stripped_pos >= sequences.data[i].position &&
			(i + 1 >= sequences.count || stripped_pos < sequences.data[i + 1].position))
		{
			color_state = sequences.data[i].color;
			found = true;
			break;
		}
	}

	// If no sequence found, use default (no color)
	if (!found)
	{
		color_state.reset = ColorStatusReset;
	}

	// Determine output format
	if (nfargs > 2 && (fargs[2][0] == 'e' || fargs[2][0] == '0'))
	{
		// Escape sequence format
		ColorType color_type = ColorTypeAnsi;
		dbref color_target = (cause != NOTHING) ? cause : player;

		if (color_target != NOTHING && Color24Bit(color_target))
		{
			color_type = ColorTypeTrueColor;
		}
		else if (color_target != NOTHING && Color256(color_target))
		{
			color_type = ColorTypeXTerm;
		}
		else if (color_target != NOTHING && Ansi(color_target))
		{
			color_type = ColorTypeAnsi;
		}
		else
		{
			color_type = ColorTypeNone;
		}
		result = color_state_to_escape(&color_state, color_type);
	}
	else if (nfargs > 2 && (fargs[2][0] == 'p' || fargs[2][0] == '1'))
	{
		// Mushcode format
		result = color_state_to_mush_code(&color_state);
	}
	else
	{
		// Letter format (default)
		result = color_state_to_letters(&color_state);
	}

	if (result)
	{
		XSAFELBSTR(result, buff, bufc);
		XFREE(result);
	}

	// Clean up
	if (sequences.text)
		XFREE(sequences.text);
	if (sequences.data)
		XFREE(sequences.data);
}
