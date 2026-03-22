/**
 * @file look_pretty.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Pretty-print and paren-colorisation helpers for look output
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

/* ColorState constants */
static const ColorState color_normal = {
    .foreground = { .is_set = ColorStatusReset },
    .background = { .is_set = ColorStatusReset }
};
static const ColorState color_hilite = {.highlight = ColorStatusSet};
static const ColorState color_red = {.foreground = {.is_set = ColorStatusSet, .ansi_index = 1, .xterm_index = 196, .truecolor = {255, 0, 0}}};
static const ColorState color_magenta = {.foreground = {.is_set = ColorStatusSet, .ansi_index = 5, .xterm_index = 201, .truecolor = {255, 0, 255}}};
static const ColorState color_green = {.foreground = {.is_set = ColorStatusSet, .ansi_index = 2, .xterm_index = 46, .truecolor = {0, 255, 0}}};
static const ColorState color_yellow = {.foreground = {.is_set = ColorStatusSet, .ansi_index = 3, .xterm_index = 226, .truecolor = {255, 255, 0}}};
static const ColorState color_cyan = {.foreground = {.is_set = ColorStatusSet, .ansi_index = 6, .xterm_index = 51, .truecolor = {0, 255, 255}}};
static const ColorState color_blue = {.foreground = {.is_set = ColorStatusSet, .ansi_index = 4, .xterm_index = 21, .truecolor = {0, 0, 255}}};


static ColorState pair_color_states[5] = {
	color_magenta,
	color_green,
	color_yellow,
	color_cyan,
	color_blue};

static ColorState pair_rev_color_states[5] = {
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 5, .xterm_index = 201, .truecolor = {255, 0, 255}}}, /* magenta inverse */
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 2, .xterm_index = 46, .truecolor = {0, 255, 0}}},	/* green inverse */
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 3, .xterm_index = 226, .truecolor = {255, 255, 0}}}, /* yellow inverse */
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 6, .xterm_index = 51, .truecolor = {0, 255, 255}}},	/* cyan inverse */
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 4, .xterm_index = 21, .truecolor = {0, 0, 255}}}		/* blue inverse */
};

void pairs_print(dbref player, char *atext, char *buff, char **bufc)
{
	int pos, depth;
	char *str, *strbuf, *parenlist, *endp;
	ColorType color_type = ColorTypeTrueColor;
	ColorState normal = color_normal;
	ColorState hilite_state = color_hilite;
	ColorState red_state = color_red;
	ColorState reverse_normal_state = {.inverse = ColorStatusSet};
	ColorState reverse_hired_state = {.inverse = ColorStatusSet, .highlight = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 1, .xterm_index = 196, .truecolor = {255, 0, 0}}}; /* red fg */

	/* pair_color_states is now statically initialized */

	if (!atext || !buff || !bufc)
	{
		return;
	}

	str = ansi_strip_ansi(atext);
	if (!str)
	{
		return;
	}

	strbuf = XMALLOC(LBUF_SIZE, "strbuf");
	if (!strbuf)
	{
		XFREE(str);
		return;
	}

	parenlist = XMALLOC(LBUF_SIZE, "parenlist");
	if (!parenlist)
	{
		XFREE(strbuf);
		XFREE(str);
		return;
	}

	endp = strbuf;
	parenlist[0] = 0;
	depth = 0;

	for (pos = 0; pos < (int)strlen(str); pos++)
	{
		switch (str[pos])
		{
		case '(':
		case '{':
		case '[':
			if (pos == 0 || str[pos - 1] != '\\')
			{
				depth++;
				if (depth < LBUF_SIZE)
				{
					parenlist[depth] = str[pos];
				}
				else
				{
					depth--;
					break;
				}
				append_color_transition(&normal, &pair_color_states[depth % 5], color_type, strbuf, &endp);
				XSAFELBCHR(str[pos], strbuf, &endp);
				append_color_transition(&pair_color_states[depth % 5], &normal, color_type, strbuf, &endp);
			}
			else
			{
				XSAFELBCHR(str[pos], strbuf, &endp);
			}

			break;

		case ']':
		case '}':
		case ')':

			/*
			 * ASCII hack to check for matching parens.
			 * Since parenlist[0] is 0, this also catches
			 * the too many close parens bug.
			 */
			if (pos == 0 || str[pos - 1] != '\\')
			{
				if ((parenlist[depth] & 96) == (str[pos] & 96))
				{
					append_color_transition(&normal, &pair_color_states[depth % 5], color_type, strbuf, &endp);
					XSAFELBCHR(str[pos], strbuf, &endp);
					append_color_transition(&pair_color_states[depth % 5], &normal, color_type, strbuf, &endp);
					depth--;
				}
				else
				{
					append_color_transition(&normal, &hilite_state, color_type, strbuf, &endp);
					append_color_transition(&hilite_state, &red_state, color_type, strbuf, &endp);
					XSAFELBCHR(str[pos], strbuf, &endp);
					append_color_transition(&red_state, &normal, color_type, strbuf, &endp);
					*endp = '\0';
					XSAFELBSTR(strbuf, buff, bufc);
					XSAFELBSTR(str + pos + 1, buff, bufc);
					XFREE(str);
					XFREE(strbuf);
					XFREE(parenlist);
					return;
				}
			}
			else
			{
				XSAFELBCHR(str[pos], strbuf, &endp);
			}

			break;

		default:
			XSAFELBCHR(str[pos], strbuf, &endp);
			break;
		}
	}

	if (depth == 0)
	{
		*endp = '\0';
		XSAFELBSTR(strbuf, buff, bufc);
		XFREE(str);
		XFREE(strbuf);
		XFREE(parenlist);
		return;
	}

	/*
	 * If we reach this point there were too many left parens.
	 * We've gotta go back.
	 */
	endp = strbuf;
	parenlist[0] = 0;
	depth = 0;

	if (strlen(str) == 0)
	{
		*endp = '\0';
		XSAFELBSTR(strbuf, buff, bufc);
		XFREE(str);
		XFREE(strbuf);
		XFREE(parenlist);
		return;
	}

	for (pos = (int)strlen(str) - 1; pos >= 0; pos--)
	{
		switch (str[pos])
		{
		case ']':
		case '}':
		case ')':
			depth++;
			if (depth < LBUF_SIZE)
			{
				parenlist[depth] = str[pos];
			}
			append_color_transition(&normal, &pair_rev_color_states[depth % 5], color_type, strbuf, &endp);
			XSAFELBCHR(str[pos], strbuf, &endp);
			append_color_transition(&pair_rev_color_states[depth % 5], &normal, color_type, strbuf, &endp);
			break;

		case '(':
		case '{':
		case '[':

			/*
			 * ASCII hack to check for matching parens.
			 * Since parenlist[0] is 0, this also catches
			 * the too many close parens bug.
			 */
			if (depth > 0 && (parenlist[depth] & 96) == (str[pos] & 96))
			{
				append_color_transition(&normal, &pair_rev_color_states[depth % 5], color_type, strbuf, &endp);
				XSAFELBCHR(str[pos], strbuf, &endp);
				append_color_transition(&pair_rev_color_states[depth % 5], &normal, color_type, strbuf, &endp);
				depth--;
			}
			else
			{
				append_color_transition(&normal, &reverse_hired_state, color_type, strbuf, &endp);
				XSAFELBCHR(str[pos], strbuf, &endp);
				append_color_transition(&reverse_hired_state, &normal, color_type, strbuf, &endp);
				str[pos] = '\0';
				XSAFELBSTR(str, buff, bufc);

				while (endp >= strbuf)
				{
					endp--;
					XSAFELBCHR(*endp, buff, bufc);
				}

				**bufc = '\0';
				XFREE(str);
				XFREE(strbuf);
				XFREE(parenlist);
				return;
			}

			break;

		default:
			XSAFELBCHR(str[pos], strbuf, &endp);
			break;
		}
	}

	/*
	 * We won't get here, but what the hell.
	 */

	while (endp >= strbuf)
	{
		endp--;
		XSAFELBCHR(*endp, buff, bufc);
	}

	**bufc = '\0';
	XFREE(str);
	XFREE(strbuf);
	XFREE(parenlist);
}

void pretty_format(char *dest, char **cp, char *p)
{
	/*
	 * Pretty-print an attribute into a buffer (assumed to be an lbuf).
	 */
	int indent_lev, i;
	indent_lev = 0;
	XSAFECRLF(dest, cp);

	while (*p)
	{
		switch (*p)
		{
		case '\\':
			/*
			 * Skip escaped chars
			 */
			XSAFELBCHR(*p, dest, cp);
			p++;
			if (!*p)
			{
				return; /* we're done */
			}
			XSAFELBCHR(*p, dest, cp);

			break;

		case '{':
			XSAFECRLF(dest, cp);

			for (i = 0; i < indent_lev; i++)
			{
				XSAFELBSTR((char *)INDENT_STR, dest, cp);
			}

			XSAFELBCHR(*p, dest, cp);
			XSAFECRLF(dest, cp);
			indent_lev++;

			for (i = 0; i < indent_lev; i++)
			{
				XSAFELBSTR((char *)INDENT_STR, dest, cp);
			}

			while (p[1] == ' ')
			{
				p++;
			}

			break;

		case '}':
			if (indent_lev > 0)
			{
				indent_lev--;
			}

			XSAFECRLF(dest, cp);

			for (i = 0; i < indent_lev; i++)
			{
				XSAFELBSTR((char *)INDENT_STR, dest, cp);
			}

			XSAFELBCHR(*p, dest, cp);
			XSAFECRLF(dest, cp);

			for (i = 0; i < indent_lev; i++)
			{
				XSAFELBSTR((char *)INDENT_STR, dest, cp);
			}

			while (p[1] == ' ')
			{
				p++;
			}

			break;

		case ';':
			XSAFELBCHR(*p, dest, cp);
			XSAFECRLF(dest, cp);

			for (i = 0; i < indent_lev; i++)
			{
				XSAFELBSTR((char *)INDENT_STR, dest, cp);
			}

			while (p[1] == ' ')
			{
				p++;
			}

			break;

		default:
			XSAFELBCHR(*p, dest, cp);
			break;
		}

		p++;
	}

	if (cp && *cp && dest && (*cp > dest) && (*(*cp - 1) != '\n'))
	{
		XSAFECRLF(dest, cp);
	}
}

void pretty_print(char *dest, char *name, char *text)
{
	char *cp, *p, *word;
	cp = dest;
	p = text;
	XSAFELBSTR(name, dest, &cp);

	/*
	 * Pretty-print contents of text into dest.
	 */

	switch (*text)
	{
	case '$':
	case '^':
		/*
		 * Do:  $command:<text to format>
		 * * Nibble up the first bit then fall through to format the rest.
		 */
		while (*p && (*p != ':'))
		{
			XSAFELBCHR(*p, dest, &cp);
			p++;
		}

		/*
		 * Do the ':'
		 */
		if (*p == ':')
		{
			XSAFELBCHR(':', dest, &cp);

			do
			{
				p++;
			} while (isspace(*p));
		}
		else
		{
			return;
		}
		/* [[fallthrough]] */
		__attribute__((fallthrough));
	case '@':
	case '&':
		/*
		 * Normal formatting
		 */
		pretty_format(dest, &cp, p);
		break;

	case '#':
		/*
		 * Special case: If the first word starts with #, there is a
		 * * second word, and it does NOT start with a #, this is a
		 * * @force command.
		 */
		word = p;

		while (word && *word && !isspace(*word))
		{
			word++;
		}

		if (!word)
		{
			XSAFELBSTR(p, dest, &cp);
			return;
		}

		word = (char *) skip_whitespace(word);

		if (!*word || (*word == '#'))
		{
			/*
			 * This is a list of dbrefs, probably. Bail.
			 */
			XSAFELBSTR(p, dest, &cp);
			return;
		}

		pretty_format(dest, &cp, p);
		break;

	default:
		/*
		 * Ordinary text
		 */
		XSAFELBSTR(p, dest, &cp);
	}

	if (cp && dest && (cp > dest) && (*(cp - 1) != '\n'))
	{
		XSAFECRLF(dest, &cp);
	}

	XSAFELBCHR('-', dest, &cp);
}

