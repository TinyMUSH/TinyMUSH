/**
 * @file look.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Look/examine commands and rendering of room, object, and player descriptions
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

// ColorState constants
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
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 5, .xterm_index = 201, .truecolor = {255, 0, 255}}}, // magenta inverse
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 2, .xterm_index = 46, .truecolor = {0, 255, 0}}},	// green inverse
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 3, .xterm_index = 226, .truecolor = {255, 255, 0}}}, // yellow inverse
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 6, .xterm_index = 51, .truecolor = {0, 255, 255}}},	// cyan inverse
	{.inverse = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 4, .xterm_index = 21, .truecolor = {0, 0, 255}}}		// blue inverse
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
	ColorState reverse_hired_state = {.inverse = ColorStatusSet, .highlight = ColorStatusSet, .foreground = {.is_set = ColorStatusSet, .ansi_index = 1, .xterm_index = 196, .truecolor = {255, 0, 0}}}; // red fg

	// pair_color_states is now statically initialized

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
		//[[fallthrough]];
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

void view_atr(dbref player, dbref thing, ATTR *ap, char *raw_text, dbref aowner, int aflags, int skip_tag, int is_special)
{
	char *text = NULL, *buf = NULL, *bp = NULL, *bb_p = NULL;
	char *xbuf = NULL, *gbuf = NULL, *flag_buf = NULL;
	char *xbufp, *gbufp, *fbp;
	char *s = NULL;
	BOOLEXP *bexp = NULL;

	/*
	 * Validate input parameters
	 */
	if (!ap || !raw_text)
	{
		return;
	}

	xbuf = XMALLOC(16, "xbuf");
	if (!xbuf)
	{
		return;
	}

	gbuf = XMALLOC(16, "gbuf");
	if (!gbuf)
	{
		XFREE(xbuf);
		return;
	}

	flag_buf = XMALLOC(34, "flag_buf");
	if (!flag_buf)
	{
		XFREE(xbuf);
		XFREE(gbuf);
		return;
	}

	s = XMALLOC(GBUF_SIZE, "s");
	if (!s)
	{
		XFREE(xbuf);
		XFREE(gbuf);
		XFREE(flag_buf);
		return;
	}

	ColorType color_type = resolve_color_type(player, player);
	ColorState normal_state = color_normal;
	ColorState hilite_state = color_hilite;

	if (ap->flags & AF_IS_LOCK)
	{
		BOOLEXP *bexp = NULL;
		bexp = boolexp_parse(player, raw_text, 1);
		if (bexp)
		{
			text = unparse_boolexp(player, bexp);
			boolexp_free(bexp);
		}
		else
		{
			text = raw_text;
		}
	}
	else if (aflags & AF_STRUCTURE)
	{
		char *replaced = replace_string(GENERIC_STRUCT_STRDELIM, mushconf.struct_dstr, raw_text);
		if (replaced)
		{
			text = replaced;
		}
		else
		{
			text = raw_text;
		}
	}
	else
	{
		text = raw_text;
	}

	/*
	 * If we don't control the object or own the attribute, hide the
	 * * attr owner and flag info.
	 */

	if (!Controls(player, thing) && (Owner(player) != aowner))
	{
		if (skip_tag && (ap->number == A_DESC))
		{
			buf = text;
			notify(player, buf);
		}
		else
		{
			if (is_special == 0)
			{
				char *temp_buf = XMALLOC(LBUF_SIZE, "temp_buf");
				char *tp = temp_buf;
				append_color_transition(&normal_state, &hilite_state, color_type, temp_buf, &tp);
				XSAFELBSTR(ap->name, temp_buf, &tp);
				append_color_transition(&hilite_state, &normal_state, color_type, temp_buf, &tp);
				XSAFELBSTR(": ", temp_buf, &tp);
				XSAFELBSTR(text, temp_buf, &tp);
				*tp = '\0';
				notify(player, temp_buf);
				XFREE(temp_buf);
			}
			else if (is_special == 1)
			{
				buf = XMALLOC(LBUF_SIZE, "buf");
				char *temp_s = XMALLOC(LBUF_SIZE, "temp_s");
				char *sp = temp_s;
				append_color_transition(&normal_state, &hilite_state, color_type, temp_s, &sp);
				XSAFELBSTR(ap->name, temp_s, &sp);
				append_color_transition(&hilite_state, &normal_state, color_type, temp_s, &sp);
				XSAFELBSTR(": ", temp_s, &sp);
				*sp = '\0';
				pretty_print(buf, temp_s, text);
				notify(player, buf);
				XFREE(buf);
				XFREE(temp_s);
			}
			else
			{
				buf = XMALLOC(LBUF_SIZE, "buf");
				bp = buf;
				append_color_transition(&normal_state, &hilite_state, color_type, buf, &bp);
				XSAFELBSTR(ap->name, buf, &bp);
				append_color_transition(&hilite_state, &normal_state, color_type, buf, &bp);
				XSAFELBSTR(": ", buf, &bp);
				pairs_print(player, text, buf, &bp);
				*bp = '\0';
				notify(player, buf);
				XFREE(buf);
			}

			XFREE(s);
			XFREE(xbuf);
			XFREE(gbuf);
			XFREE(flag_buf);
			return;
		}
	}

	/*
	 * Generate flags
	 */

	xbufp = xbuf;
	if (aflags & AF_LOCK)
		*xbufp++ = '+';
	if (aflags & AF_NOPROG)
		*xbufp++ = '$';
	if (aflags & AF_CASE)
		*xbufp++ = 'C';
	if (aflags & AF_DEFAULT)
		*xbufp++ = 'D';
	if (aflags & AF_HTML)
		*xbufp++ = 'H';
	if (aflags & AF_PRIVATE)
		*xbufp++ = 'I';
	if (aflags & AF_RMATCH)
		*xbufp++ = 'M';
	if (aflags & AF_NONAME)
		*xbufp++ = 'N';
	if (aflags & AF_NOPARSE)
		*xbufp++ = 'P';
	if (aflags & AF_NOW)
		*xbufp++ = 'Q';
	if (aflags & AF_REGEXP)
		*xbufp++ = 'R';
	if (aflags & AF_STRUCTURE)
		*xbufp++ = 'S';
	if (aflags & AF_TRACE)
		*xbufp++ = 'T';
	if (aflags & AF_VISUAL)
		*xbufp++ = 'V';
	if (aflags & AF_NOCLONE)
		*xbufp++ = 'c';
	if (aflags & AF_DARK)
		*xbufp++ = 'd';
	if (aflags & AF_GOD)
		*xbufp++ = 'g';
	if (aflags & AF_CONST)
		*xbufp++ = 'k';
	if (aflags & AF_MDARK)
		*xbufp++ = 'm';
	if (aflags & AF_WIZARD)
		*xbufp++ = 'w';
	*xbufp = '\0';

	gbufp = gbuf;
	if (ap->flags & AF_LOCK)
		*gbufp++ = '+';
	if (ap->flags & AF_NOPROG)
		*gbufp++ = '$';
	if (ap->flags & AF_CASE)
		*gbufp++ = 'C';
	if (ap->flags & AF_DEFAULT)
		*gbufp++ = 'D';
	if (ap->flags & AF_HTML)
		*gbufp++ = 'H';
	if (ap->flags & AF_PRIVATE)
		*gbufp++ = 'I';
	if (ap->flags & AF_RMATCH)
		*gbufp++ = 'M';
	if (ap->flags & AF_NONAME)
		*gbufp++ = 'N';
	if (ap->flags & AF_NOPARSE)
		*gbufp++ = 'P';
	if (ap->flags & AF_NOW)
		*gbufp++ = 'Q';
	if (ap->flags & AF_REGEXP)
		*gbufp++ = 'R';
	if (ap->flags & AF_STRUCTURE)
		*gbufp++ = 'S';
	if (ap->flags & AF_TRACE)
		*gbufp++ = 'T';
	if (ap->flags & AF_VISUAL)
		*gbufp++ = 'V';
	if (ap->flags & AF_NOCLONE)
		*gbufp++ = 'c';
	if (ap->flags & AF_DARK)
		*gbufp++ = 'd';
	if (ap->flags & AF_GOD)
		*gbufp++ = 'g';
	if (ap->flags & AF_CONST)
		*gbufp++ = 'k';
	if (ap->flags & AF_MDARK)
		*gbufp++ = 'm';
	if (ap->flags & AF_WIZARD)
		*gbufp++ = 'w';
	*gbufp = '\0';

	fbp = xbuf;

	if (*xbuf && *gbuf)
	{
		XSNPRINTF(flag_buf, 34, "%s(%s)", xbuf, gbuf);
		fbp = flag_buf;
	}
	else if (*gbuf)
	{
		XSNPRINTF(flag_buf, 34, "(%s)", gbuf);
		fbp = flag_buf;
	}

	if (is_special == 1)
	{
		char *temp_s = XMALLOC(LBUF_SIZE, "temp_s");
		char *sp = temp_s;
		append_color_transition(&normal_state, &hilite_state, color_type, temp_s, &sp);
		XSAFELBSTR(ap->name, temp_s, &sp);
		if ((aowner != Owner(thing)) && (aowner != NOTHING))
		{
			XSAFELBSTR(" [#", temp_s, &sp);
			char num[16];
			XSNPRINTF(num, 16, "%d", aowner);
			XSAFELBSTR(num, temp_s, &sp);
			XSAFELBSTR(fbp, temp_s, &sp);
			XSAFELBSTR("]:", temp_s, &sp);
		}
		else if (*fbp)
		{
			XSAFELBSTR(" [", temp_s, &sp);
			XSAFELBSTR(fbp, temp_s, &sp);
			XSAFELBSTR("]:", temp_s, &sp);
		}
		else if (!skip_tag || (ap->number != A_DESC))
		{
			XSAFELBSTR(":", temp_s, &sp);
		}
		append_color_transition(&hilite_state, &normal_state, color_type, temp_s, &sp);
		XSAFELBSTR(" ", temp_s, &sp);
		*sp = '\0';
		buf = XMALLOC(LBUF_SIZE, "buf");
		pretty_print(buf, temp_s, text);
		notify(player, buf);
		XFREE(buf);
		XFREE(temp_s);
	}
	else if (is_special == 2)
	{
		buf = XMALLOC(LBUF_SIZE, "buf");
		if (buf)
		{
			bb_p = buf;

			if ((aowner != Owner(thing)) && (aowner != NOTHING))
			{
				char temp[16];
				sprintf(temp, "%d", aowner);
				append_color_transition(&normal_state, &hilite_state, color_type, buf, &bb_p);
				XSAFELBSTR(ap->name, buf, &bb_p);
				XSAFELBSTR(" [#", buf, &bb_p);
				XSAFELBSTR(temp, buf, &bb_p);
				XSAFELBSTR(fbp, buf, &bb_p);
				XSAFELBSTR("]: ", buf, &bb_p);
				append_color_transition(&hilite_state, &normal_state, color_type, buf, &bb_p);
			}
			else if (*fbp)
			{
				append_color_transition(&normal_state, &hilite_state, color_type, buf, &bb_p);
				XSAFELBSTR(ap->name, buf, &bb_p);
				XSAFELBSTR(" [", buf, &bb_p);
				XSAFELBSTR(fbp, buf, &bb_p);
				XSAFELBSTR("]: ", buf, &bb_p);
				append_color_transition(&hilite_state, &normal_state, color_type, buf, &bb_p);
			}
			else if (!skip_tag || (ap->number != A_DESC))
			{
				append_color_transition(&normal_state, &hilite_state, color_type, buf, &bb_p);
				XSAFELBSTR(ap->name, buf, &bb_p);
				XSAFELBSTR(": ", buf, &bb_p);
				append_color_transition(&hilite_state, &normal_state, color_type, buf, &bb_p);
			}
			else
			{
				/*
				 * Just fine the way it is
				 */
			}

			pairs_print(player, text, buf, &bb_p);
			*bb_p = '\0';
			notify(player, buf);
			XFREE(buf);
		}
	}
	else
	{
		char *sp = s;
		if ((aowner != Owner(thing)) && (aowner != NOTHING))
		{
			char temp[16];
			sprintf(temp, "%d", aowner);
			append_color_transition(&normal_state, &hilite_state, color_type, s, &sp);
			XSAFELBSTR(ap->name, s, &sp);
			XSAFELBSTR(" [#", s, &sp);
			XSAFELBSTR(temp, s, &sp);
			XSAFELBSTR(fbp, s, &sp);
			XSAFELBSTR("]: ", s, &sp);
			append_color_transition(&hilite_state, &normal_state, color_type, s, &sp);
			XSAFELBSTR(text, s, &sp);
		}
		else if (*fbp)
		{
			append_color_transition(&normal_state, &hilite_state, color_type, s, &sp);
			XSAFELBSTR(ap->name, s, &sp);
			XSAFELBSTR(" [", s, &sp);
			XSAFELBSTR(fbp, s, &sp);
			XSAFELBSTR("]: ", s, &sp);
			append_color_transition(&hilite_state, &normal_state, color_type, s, &sp);
			XSAFELBSTR(text, s, &sp);
		}
		else if (!skip_tag || (ap->number != A_DESC))
		{
			append_color_transition(&normal_state, &hilite_state, color_type, s, &sp);
			XSAFELBSTR(ap->name, s, &sp);
			XSAFELBSTR(": ", s, &sp);
			append_color_transition(&hilite_state, &normal_state, color_type, s, &sp);
			XSAFELBSTR(text, s, &sp);
		}
		else
		{
			XSAFELBSTR(text, s, &sp);
		}
		*sp = '\0';
		notify(player, s);
	}

	if (((aflags & AF_STRUCTURE) || (ap->flags & AF_IS_LOCK)) && text)
	{
		XFREE(text);
	}

	XFREE(s);
	XFREE(xbuf);
	XFREE(gbuf);
	XFREE(flag_buf);
}

void look_atrs1(dbref player, dbref thing, dbref othing, int check_exclude, int hash_insert, int is_special)
{
	dbref aowner = NOTHING;
	int ca = 0, aflags = 0, alen = 0;
	ATTR *attr = NULL, *cattr = NULL;
	char *as = NULL, *buf = NULL;
	cattr = (ATTR *)XMALLOC(sizeof(ATTR), "cattr");
	if (!cattr)
	{
		return;
	}

	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as))
	{
		if ((ca == A_DESC) || (ca == A_LOCK))
		{
			continue;
		}

		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		XMEMCPY((char *)cattr, (char *)attr, sizeof(ATTR));

		/*
		 * Should we exclude this attr?
		 * * We have a couple of things we exclude:
		 * * Attributes explicitly marked no_inherit.
		 * * Locks. Note that UseLock is NOT, strictly speaking, an
		 * *   inherited lock, since it's just checked when the child
		 * *   tries to inherit $commands from the parent; the child
		 * *   itself doesn't acquire the parent's uselock.
		 * * Attributes already slurped by upper-level objects.
		 */

		if (check_exclude && ((cattr->flags & AF_PRIVATE) || (cattr->flags & AF_IS_LOCK) || nhashfind(ca, &mushstate.parent_htab)))
		{
			continue;
		}

		buf = atr_get(thing, ca, &aowner, &aflags, &alen);

		if (buf && Read_attr_all(player, othing, cattr, aowner, aflags, 1))
		{
			if (!(check_exclude && (aflags & AF_PRIVATE)))
			{
				if (hash_insert)
					nhashadd(ca, (int *)cattr, &mushstate.parent_htab);

				view_atr(player, thing, cattr, buf, aowner, aflags, 0, is_special);
			}
		}

		if (buf)
			XFREE(buf);
	}

	XFREE(cattr);
}

void look_atrs(dbref player, dbref thing, int check_parents, int is_special)
{
	dbref parent = NOTHING;
	int lev = 0, check_exclude = 0, hash_insert = 0;

	if (!check_parents)
	{
		look_atrs1(player, thing, thing, 0, 0, is_special);
	}
	else
	{
		hash_insert = 1;
		check_exclude = 0;
		nhashflush(&mushstate.parent_htab, 0);

		for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (!Good_obj(Parent(parent)))
			{
				hash_insert = 0;
			}

			look_atrs1(player, parent, thing, check_exclude, hash_insert, is_special);
			check_exclude = 1;
		}
	}
}

void look_simple(dbref player, dbref thing, int obey_terse)
{
	char *buff = NULL;

	/*
	 * Only makes sense for things that can hear
	 */

	if (!Hearer(player))
	{
		return;
	}

	/*
	 * Get the name and db-number if we can examine it.
	 */

	if (Examinable(player, thing))
	{
		buff = unparse_object(player, thing, 1);
		if (buff)
		{
			notify(player, buff);
			XFREE(buff);
		}
	}

	if (obey_terse && Terse(player))
		did_it(player, thing, A_NULL, "You see nothing special.", A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);
	else if (mushconf.have_pueblo == 1)
	{
		show_a_desc(player, thing, "You see nothing special.");
	}
	else
	{
		did_it(player, thing, A_DESC, "You see nothing special.", A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);
	}

	if (!mushconf.quiet_look && (!Terse(player) || mushconf.terse_look))
	{
		look_atrs(player, thing, 0, 0);
	}
}

void show_a_desc(dbref player, dbref loc, const char *msg)
{
	char *got2 = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0, indent = 0;
	char *raw_desc = NULL;

	if (!msg)
	{
		msg = "You see nothing special.";
	}

	raw_desc = atr_get_raw(loc, A_DESC);
	indent = (isRoom(loc) && mushconf.indent_desc && raw_desc && *raw_desc);

	if (Html(player))
	{
		got2 = atr_pget(loc, A_HTDESC, &aowner, &aflags, &alen);

		if (got2 && *got2)
			did_it(player, loc, A_HTDESC, msg, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);
		else
		{
			if (indent)
			{
				raw_notify_newline(player);
			}

			did_it(player, loc, A_DESC, msg, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);

			if (indent)
			{
				raw_notify_newline(player);
			}
		}

		if (got2)
			XFREE(got2);
	}
	else
	{
		if (indent)
		{
			raw_notify_newline(player);
		}

		did_it(player, loc, A_DESC, msg, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);

		if (indent)
		{
			raw_notify_newline(player);
		}
	}

	if (raw_desc)
	{
		XFREE(raw_desc);
	}
}

void show_desc(dbref player, dbref loc, int key)
{
	char *got = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0, indent = 0;
	char *raw_desc = NULL;
	const char *msg = NULL;

	msg = "You see nothing special.";

	raw_desc = atr_get_raw(loc, A_DESC);
	indent = (isRoom(loc) && mushconf.indent_desc && raw_desc && *raw_desc);

	if ((key & LK_OBEYTERSE) && Terse(player))
		did_it(player, loc, A_NULL, NULL, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);
	else if ((Typeof(loc) != TYPE_ROOM) && (key & LK_IDESC))
	{
		got = atr_pget(loc, A_IDESC, &aowner, &aflags, &alen);
		if (got && *got)
			did_it(player, loc, A_IDESC, NULL, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);
		else
		{
			if (mushconf.have_pueblo == 1)
			{
				show_a_desc(player, loc, NULL);
			}
			else
			{
				if (indent)
				{
					raw_notify_newline(player);
				}

				did_it(player, loc, A_DESC, NULL, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);

				if (indent)
				{
					raw_notify_newline(player);
				}
			}

			if (raw_desc)
			{
				XFREE(raw_desc);
			}
		}

		if (got)
			XFREE(got);
	}
	else
	{
		if (mushconf.have_pueblo == 1)
		{
			show_a_desc(player, loc, NULL);
		}
		else
		{
			if (indent)
			{
				raw_notify_newline(player);
			}

			did_it(player, loc, A_DESC, NULL, A_ODESC, NULL, A_ADESC, 0, (char **)NULL, 0, MSG_PRESENCE);

			if (indent)
			{
				raw_notify_newline(player);
			}
		}
	}
}

void look_in(dbref player, dbref loc, int key)
{
	int pattr = 0, oattr = 0, aattr = 0, is_terse = 0, showkey = 0;
	char *buff = NULL;
	is_terse = (key & LK_OBEYTERSE) ? Terse(player) : 0;

	/*
	 * Only makes sense for things that can hear
	 */

	if (!Hearer(player))
	{
		return;
	}

	/*
	 * If he needs the VRML URL, send it:
	 */
	if (mushconf.have_pueblo == 1)
	{
		if (key & LK_SHOWVRML)
		{
			show_vrml_url(player, loc);
		}
	}

	/*
	 * If we can't format it in a player-specified way, then show
	 * * the name (and unparse info, if relevant). We only invoke
	 * * the Pueblo formatting if we weren't given a @nameformat.
	 */

	if (!did_attr(player, loc, A_NAME_FMT))
	{
		buff = unparse_object(player, loc, 1);

		if (buff)
		{
			if (mushconf.have_pueblo == 1)
			{
				if (Html(player))
				{
					notify_html(player, "<center><h3>");
					notify(player, buff);
					notify_html(player, "</h3></center>");
				}
				else
				{
					notify(player, buff);
				}
			}
			else
			{
				notify(player, buff);
			}

			XFREE(buff);
		}
	}

	if (!Good_obj(loc))
	{
		return;
	}
	/* If we went to NOTHING et al, skip the

	 * rest */
	/*
	 * tell him the description
	 */
	showkey = 0;

	if (loc == Location(player))
	{
		showkey |= LK_IDESC;
	}

	if (key & LK_OBEYTERSE)
	{
		showkey |= LK_OBEYTERSE;
	}

	show_desc(player, loc, showkey);

	/*
	 * tell him the appropriate messages if he has the key
	 */

	if (Typeof(loc) == TYPE_ROOM)
	{
		if (could_doit(player, loc, A_LOCK))
		{
			pattr = A_SUCC;
			oattr = A_OSUCC;
			aattr = A_ASUCC;
		}
		else
		{
			pattr = A_FAIL;
			oattr = A_OFAIL;
			aattr = A_AFAIL;
		}

		if (is_terse)
		{
			pattr = 0;
		}

		did_it(player, loc, pattr, NULL, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_PRESENCE);
	}

	/*
	 * tell him the attributes, contents and exits
	 */

	if ((key & LK_SHOWATTR) && !mushconf.quiet_look && !is_terse)
	{
		look_atrs(player, loc, 0, 0);
	}

	if (!is_terse || mushconf.terse_contents)
	{
		look_contents(player, loc, "Contents:", CONTENTS_LOCAL);
	}

	if ((key & LK_SHOWEXIT) && (!is_terse || mushconf.terse_exits))
	{
		look_exits(player, loc, "Obvious exits:");
	}
}

void look_here(dbref player, dbref thing, int key, int look_key)
{
	if (!Good_obj(thing))
	{
		return;
	}

	if (key & LOOK_OUTSIDE)
	{
		if ((isRoom(thing)) || Opaque(thing))
		{
			notify_quiet(player, "You can't look outside.");
			return;
		}

		thing = Location(thing);
		if (!Good_obj(thing))
		{
			return;
		}
	}

	look_in(player, thing, look_key);
}

void do_look(dbref player, dbref cause, int key, char *name)
{
	dbref thing = NOTHING, loc = NOTHING, look_key = 0;
	look_key = LK_SHOWATTR | LK_SHOWEXIT;

	if (!mushconf.terse_look)
	{
		look_key |= LK_OBEYTERSE;
	}

	loc = Location(player);

	if (!name || !*name)
	{
		look_here(player, loc, key, look_key);
		return;
	}

	/*
	 * Look for the target locally
	 */
	thing = (key & LOOK_OUTSIDE) ? loc : player;
	init_match(thing, name, NOTYPE);
	match_exit_with_parents();
	match_neighbor();
	match_possession();

	if (Long_Fingers(player))
	{
		match_absolute();
		match_player();
	}

	match_here();
	match_me();
	match_master_exit();
	thing = match_result();

	/*
	 * Not found locally, check possessive
	 */

	if (!Good_obj(thing))
	{
		thing = match_status(player, match_possessed(player, ((key & LOOK_OUTSIDE) ? loc : player), (char *)name, thing, 0));
	}

	/*
	 * First make sure that we aren't looking at our own location, since
	 * * that gets handled a little differently.
	 */
	if (thing == loc)
	{
		look_here(player, thing, key, look_key);
		return;
	}

	/*
	 * If we found something, go handle it
	 */

	if (Good_obj(thing))
	{
		switch (Typeof(thing))
		{
		case TYPE_ROOM:
			look_in(player, thing, look_key);
			break;

		case TYPE_THING:
		case TYPE_PLAYER:
			look_simple(player, thing, !mushconf.terse_look);

			if (!Opaque(thing) && (!Terse(player) || mushconf.terse_contents))
			{
				look_contents(player, thing, "Carrying:", CONTENTS_NESTED);
			}

			break;

		case TYPE_EXIT:
			look_simple(player, thing, !mushconf.terse_look);

			if (Transparent(thing) && Good_obj(Location(thing)))
			{
				look_key &= ~LK_SHOWATTR;
				look_in(player, Location(thing), look_key);
			}

			break;

		default:
			look_simple(player, thing, !mushconf.terse_look);
			break;
		}
	}
}

