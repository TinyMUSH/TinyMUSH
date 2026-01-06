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


static inline bool colorstate_equal(const ColorState *a, const ColorState *b)
{
	return memcmp(a, b, sizeof(ColorState)) == 0;
}

static void append_color_transition(const ColorState *from, const ColorState *to, ColorType type, char *buff, char **bufc)
{
	if (type == ColorTypeNone || colorstate_equal(from, to))
	{
		return;
	}

	char *seq = ansi_transition_colorstate(*from, *to, type, false);

	if (seq)
	{
		XSAFELBSTR(seq, buff, bufc);
		XFREE(seq);
	}
}

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

int did_attr(dbref player, dbref thing, int what)
{
	/*
	 * If the attribute exists, get it, notify the player, return 1.
	 * * If not, return 0.
	 */
	char *buff;
	buff = master_attr(player, thing, what, NULL, 0, NULL);

	if (buff)
	{
		notify(player, buff);
		XFREE(buff);
		return 1;
	}

	return 0;
}

void look_exits(dbref player, dbref loc, const char *exit_name)
{
	dbref thing = NOTHING, parent = NOTHING;
	char *buff = NULL, *e = NULL, *buff1 = NULL, *e1 = NULL, *buf = NULL;
	int foundany = 0, lev = 0, isdark = 0;

	/*
	 * make sure location has exits
	 */

	if (!Good_obj(loc) || !Has_exits(loc))
	{
		return;
	}

	/*
	 * exit_name must be valid for notification
	 */
	if (!exit_name)
	{
		return;
	}

	/*
	 * Check to see if we're formatting exits in a player-specified way.
	 */

	if (did_attr(player, loc, A_LEXITS_FMT))
	{
		return;
	}

	/*
	 * make sure there is at least one visible exit
	 */
	foundany = 0;
	isdark = Darkened(player, loc);

	for (lev = 0, parent = loc; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
	{
		if (!Has_exits(parent))
		{
			continue;
		}

		for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
		{
			if (Can_See_Exit(player, thing, isdark))
			{
				foundany = 1;
				break;
			}
		}

		if (foundany)
		{
			break;
		}
	}

	if (!foundany)
	{
		return;
	}

	/*
	 * Display the list of exit names
	 */
	notify(player, exit_name);
	buff = XMALLOC(LBUF_SIZE, "buff");
	if (!buff)
	{
		return;
	}
	e = buff;
	buff1 = XMALLOC(LBUF_SIZE, "buff1");
	if (!buff1)
	{
		XFREE(buff);
		return;
	}
	e1 = buff1;

	for (lev = 0, parent = loc; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
	{
		if (Transparent(loc))
		{
			for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (Can_See_Exit(player, thing, isdark))
				{
					e = buff;
					safe_exit_name(thing, buff, &e);

					if (Location(thing) == NOTHING)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads nowhere.", buff);
					}
					else if (Location(thing) == AMBIGUOUS)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads somewhere.", buff);
					}
					else if (Location(thing) == HOME)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads home.", buff);
					}
					else if (Good_obj(Location(thing)))
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads to %s.", buff, Name(Location(thing)));
					}
					else
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s leads elsewhere.", buff);
					}
				}
			}
		}
		else
		{
			for (thing = Exits(parent); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (Can_See_Exit(player, thing, isdark))
				{
					if (e != buff)
						XSAFESTRCAT(buff, &e, "  ", LBUF_SIZE);

					if (Html(player) && (mushconf.have_pueblo == 1))
					{
						e1 = buff1;
						safe_exit_name(thing, buff1, &e1);
						XSAFELBSTR((char *)"<a xch_cmd=\"", buff, &e);
						/*
						 * XXX Just stripping ansi isn't really enough.
						 */
						buf = ansi_strip_ansi(buff1);
						if (buf)
						{
							XSAFELBSTR(buf, buff, &e);
							XFREE(buf);
						}
						else
						{
							XSAFELBSTR(buff1, buff, &e);
						}
						XSAFELBSTR((char *)"\">", buff, &e);
						/*
						 * XXX The exit name needs to be HTML escaped.
						 */
						html_escape(buff1, buff, &e);
						XSAFELBSTR((char *)"</a>", buff, &e);
					}
					else
					{
						/*
						 * Append this exit to the list
						 */
						safe_exit_name(thing, buff, &e);
					}
				}
			}
		}
	}

	if (!(Transparent(loc)))
	{
		*e = '\0';
		if (mushconf.have_pueblo == 1 && Html(player))
		{
			XSAFELBSTR("\r\n", buff, &e);
			*e = '\0';
			notify_html(player, buff);
		}
		else
		{
			notify(player, buff);
		}
	}

	if (buff)
		XFREE(buff);
	if (buff1)
		XFREE(buff1);
}

void look_contents(dbref player, dbref loc, const char *contents_name, int style)
{
	dbref thing = NOTHING, can_see_loc = NOTHING;
	char *buff = NULL, *html_buff = NULL, *html_cp = NULL, *remote_num = NULL;

	/*
	 * Validate input parameters
	 */
	if (!Good_obj(loc) || !contents_name)
	{
		return;
	}

	/*
	 * Check if we're formatting contents in a player-specified way.
	 */

	if (did_attr(player, loc, A_LCON_FMT))
	{
		return;
	}

	if (mushconf.have_pueblo == 1)
	{
		html_buff = XMALLOC(LBUF_SIZE, "html_cp");
		if (!html_buff)
		{
			return;
		}
		html_cp = html_buff;
	}

	/*
	 * check to see if he can see the location
	 */
	can_see_loc = Sees_Always(player, loc);
	/*
	 * check to see if there is anything there
	 */
	for (thing = Contents(loc); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
	{
		if (Can_See(player, thing, can_see_loc))
		{
			/*
			 * something exists!  show him everything
			 */
			notify(player, contents_name);
			for (thing = Contents(loc); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (Can_See(player, thing, can_see_loc))
				{
					buff = unparse_object(player, thing, 1);
					if (!buff)
					{
						continue;
					}
					if (!buff)
					{
						continue;
					}
					html_cp = html_buff;

					if (Html(player) && (mushconf.have_pueblo == 1))
					{
						XSAFELBSTR("<a xch_cmd=\"look ", html_buff, &html_cp);

						/**
						 * @bug Just stripping ansi isn't enough.
						 *
						 */

						switch (style)
						{
						case CONTENTS_LOCAL:
							XSAFELBSTR(PureName(thing), html_buff, &html_cp);
							break;

						case CONTENTS_NESTED:
							XSAFELBSTR(PureName(Location(thing)), html_buff, &html_cp);
							XSAFELBSTR("'s ", html_buff, &html_cp);
							XSAFELBSTR(PureName(thing), html_buff, &html_cp);
							break;

						case CONTENTS_REMOTE:
							remote_num = XASPRINTF("remote_num", "#%d", thing);
							XSAFELBSTR(remote_num, html_buff, &html_cp);
							XFREE(remote_num);
							break;

						default:
							break;
						}

						XSAFELBSTR("\">", html_buff, &html_cp);
						html_escape(buff, html_buff, &html_cp);
						XSAFELBSTR("</a>\r\n", html_buff, &html_cp);
						*html_cp = 0;
						notify_html(player, html_buff);
					}
					else
					{
						notify(player, buff);
					}

					XFREE(buff);
				}
			}
			break; /* we're done */
		}
	}

	if (html_buff)
	{
		XFREE(html_buff);
	}
}

void pairs_print(dbref player, char *atext, char *buff, char **bufc)
{
	int pos, depth;
	char *str, *strbuf, *parenlist, *endp;
	ColorType color_type = resolve_color_type(player, player);
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

		while (*word && isspace(*word))
		{
			word++;
		}

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
		bexp = parse_boolexp(player, raw_text, 1);
		if (bexp)
		{
			text = unparse_boolexp(player, bexp);
			free_boolexp(bexp);
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

void do_look(dbref player, dbref cause __attribute__((unused)), int key, char *name)
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

void debug_examine(dbref player, dbref thing)
{
	dbref aowner;
	char *buf;
	int aflags, alen, ca;
	BOOLEXP *bexp;
	ATTR *attr;
	char *as, *cp, *nbuf;
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Number  = %d", thing);

	if (!Good_obj(thing))
	{
		return;
	}

	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Name    = %s", Name(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Location= %d", Location(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Contents= %d", Contents(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Exits   = %d", Exits(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Link    = %d", Link(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Next    = %d", Next(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owner   = %d", Owner(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Pennies = %d", Pennies(thing));
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Zone    = %d", Zone(thing));
	buf = flag_description(player, thing);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Flags   = %s", buf);
	XFREE(buf);
	buf = power_description(player, thing);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Powers  = %s", buf);
	XFREE(buf);
	buf = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
	bexp = parse_boolexp(player, buf, 1);
	XFREE(buf);
	buf = unparse_boolexp(player, bexp);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Lock    = %s", buf);
	XFREE(buf);
	free_boolexp(bexp);
	buf = XMALLOC(LBUF_SIZE, "buf");
	cp = buf;
	XSAFELBSTR((char *)"Attr list: ", buf, &cp);

	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as))
	{
		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		atr_get_info(thing, ca, &aowner, &aflags);

		if (Read_attr(player, thing, attr, aowner, aflags))
		{
			if (attr)
			{ /* Valid attr. */
				XSAFELBSTR((char *)attr->name, buf, &cp);
				XSAFELBCHR(' ', buf, &cp);
			}
			else
			{
				nbuf = ltos(ca);
				XSAFELBSTR(nbuf, buf, &cp);
				XFREE(nbuf);
				XSAFELBCHR(' ', buf, &cp);
			}
		}
	}

	notify(player, buf);
	XFREE(buf);

	for (ca = atr_head(thing, &as); ca; ca = atr_next(&as))
	{
		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		buf = atr_get(thing, ca, &aowner, &aflags, &alen);

		if (Read_attr_all(player, thing, attr, aowner, aflags, 1))
			view_atr(player, thing, attr, buf, aowner, aflags, 0, 0);

		XFREE(buf);
	}
}

void exam_wildattrs(dbref player, dbref thing, int do_parent, int is_special)
{
	int atr = NOTHING, aflags = 0, alen = 0, got_any = 0;
	char *buf = NULL;
	dbref aowner = NOTHING;
	ATTR *ap = NULL;

	for (atr = olist_first(); atr != NOTHING; atr = olist_next())
	{
		ap = atr_num(atr);

		if (!ap)
		{
			continue;
		}

		if (do_parent && !(ap->flags & AF_PRIVATE))
		{
			buf = atr_pget(thing, atr, &aowner, &aflags, &alen);
		}
		else
		{
			buf = atr_get(thing, atr, &aowner, &aflags, &alen);
		}

		/*
		 * Decide if the player should see the attr:
		 * If obj is Examinable and has rights to see, yes.
		 * If a player and has rights to see, yes...
		 *   except if faraway, attr=DESC, and
		 *   remote DESC-reading is not turned on.
		 * If I own the attrib and have rights to see, yes...
		 *   except if faraway, attr=DESC, and
		 *   remote DESC-reading is not turned on.
		 */

		if (ap && Examinable(player, thing) && Read_attr_all(player, thing, ap, aowner, aflags, 1))
		{
			got_any = 1;
			view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
		}
		else if (ap && (Typeof(thing) == TYPE_PLAYER) && Read_attr_all(player, thing, ap, aowner, aflags, 1))
		{
			got_any = 1;

			if (aowner == Owner(player))
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else if ((atr == A_DESC) && (mushconf.read_rem_desc || nearby(player, thing)))
			{
				show_desc(player, thing, 0);
			}
			else if (atr != A_DESC)
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else
			{
				notify(player, "<Too far away to get a good look>");
			}
		}
		else if (ap && Read_attr_all(player, thing, ap, aowner, aflags, 1))
		{
			got_any = 1;

			if (aowner == Owner(player))
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else if ((atr == A_DESC) && (mushconf.read_rem_desc || nearby(player, thing)))
			{
				show_desc(player, thing, 0);
			}
			else if (nearby(player, thing))
			{
				view_atr(player, thing, ap, buf, aowner, aflags, 0, is_special);
			}
			else
			{
				notify(player, "<Too far away to get a good look>");
			}
		}

		XFREE(buf);
	}

	if (!got_any)
	{
		notify_quiet(player, "No matching attributes found.");
	}
}

void do_examine(dbref player, dbref cause, int key, char *name)
{
	dbref thing = NOTHING, content = NOTHING, exit = NOTHING, aowner = NOTHING, loc = NOTHING;
	char savec = '\0';
	char *temp = NULL, *buf = NULL, *buf2 = NULL;
	char *timebuf = NULL;
	BOOLEXP *bexp = NULL;
	int control = 0, aflags = 0, alen = 0, do_parent = 0, is_special = 0;
	time_t save_access_time = 0;
	char created_str[26] = {0}, accessed_str[26] = {0}, modified_str[26] = {0};
	struct tm tm_created = {0}, tm_accessed = {0}, tm_modified = {0};

	/*
	 * This command is pointless if the player can't hear.
	 */

	if (!Hearer(player))
	{
		return;
	}

	timebuf = XMALLOC(32, "timebuf");
	if (!timebuf)
	{
		return;
	}

	do_parent = key & EXAM_PARENT;
	is_special = 0;

	if (key & EXAM_PRETTY)
	{
		is_special = 1;
	}

	if (key & EXAM_PAIRS)
	{
		is_special = 2;
	}

	thing = NOTHING;

	if (!name || !*name)
	{
		if ((thing = Location(player)) == NOTHING)
		{
			XFREE(timebuf);
			return;
		}
	}
	else
	{
		/*
		 * Check for obj/attr first.
		 */
		olist_push();

		if (parse_attrib_wild(player, name, &thing, do_parent, 1, 0, 1))
		{
			exam_wildattrs(player, thing, do_parent, is_special);
			olist_pop();
			XFREE(timebuf);
			return;
		}

		olist_pop();
		/*
		 * Look it up
		 */
		init_match(player, name, NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();

		if (!Good_obj(thing))
		{
			XFREE(timebuf);
			return;
		}
	}

	/*
	 * We have to save our access time, because the very act of
	 * * trying to examine the object will have touched it.
	 */
	save_access_time = AccessTime(thing);

	/*
	 * Check for the /debug switch
	 */

	if (key & EXAM_DEBUG)
	{
		if (!Examinable(player, thing))
		{
			notify_quiet(player, NOPERM_MESSAGE);
		}
		else
		{
			debug_examine(player, thing);
		}

		XFREE(timebuf);
		return;
	}

	control = (Examinable(player, thing) || Link_exit(player, thing));

	if (control && !(key & EXAM_OWNER))
	{
		buf2 = unparse_object(player, thing, 0);
		if (buf2)
		{
			notify(player, buf2);
			XFREE(buf2);
		}

		if (mushconf.ex_flags)
		{
			buf2 = flag_description(player, thing);
			if (buf2)
			{
				notify(player, buf2);
				XFREE(buf2);
			}
		}
	}
	else
	{
		if ((key & EXAM_OWNER) || ((key & EXAM_DEFAULT) && !mushconf.exam_public))
		{
			if (mushconf.read_rem_name)
			{
				buf2 = XMALLOC(LBUF_SIZE, "buf2");
				if (buf2)
				{
					XSTRCPY(buf2, Name(thing));
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s is owned by %s", buf2, Name(Owner(thing)));
					XFREE(buf2);
				}
			}
			else
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owned by %s", Name(Owner(thing)));
			}

			XFREE(timebuf);
			return;
		}
	}

	temp = XMALLOC(LBUF_SIZE, "temp");
	if (!temp)
	{
		XFREE(timebuf);
		return;
	}

	if (control || mushconf.read_rem_desc || nearby(player, thing))
	{
		temp = atr_get_str(temp, thing, A_DESC, &aowner, &aflags, &alen);

		if (temp && *temp)
		{
			if (Examinable(player, thing) || (aowner == Owner(player)))
			{
				view_atr(player, thing, atr_num(A_DESC), temp, aowner, aflags, 1, is_special);
			}
			else
			{
				show_desc(player, thing, 0);
			}
		}
	}
	else
	{
		notify(player, "<Too far away to get a good look>");
	}

	if (control)
	{
		/*
		 * print owner, key, and value
		 */
		savec = mushconf.many_coins[0];
		mushconf.many_coins[0] = (islower(savec) ? toupper(savec) : savec);
		buf2 = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
		// An empty lock or NULL lock still constitutes a valid lock (meaning the thing is unlocked)
		// The parser can handle these cases and correctly returns TRUE_BOOLEXP (aka NULL), thus no null check is required here.
		bexp = parse_boolexp(player, buf2, 1);
		// A NULL boolexp is a valid boolexp (meaning the thing has no lock); the unparser can safely handle those, thus no null check is required here.
		buf = unparse_boolexp(player, bexp);
		if (buf)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owner: %s  Key: %s %s: %d", Name(Owner(thing)), buf, mushconf.many_coins, Pennies(thing));
			XFREE(buf);
		}
		free_boolexp(bexp);
		XFREE(buf2);
		mushconf.many_coins[0] = savec;
		if (localtime_r(&CreateTime(thing), &tm_created) != NULL)
		{
			if (strftime(created_str, sizeof(created_str), "%a %b %d %H:%M:%S %Y", &tm_created) > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Created: %s", created_str);
			}
		}
		if (localtime_r(&save_access_time, &tm_accessed) != NULL)
		{
			if (strftime(accessed_str, sizeof(accessed_str), "%a %b %d %H:%M:%S %Y", &tm_accessed) > 0)
			{
				if (timebuf)
				{
					XSTRCPY(timebuf, accessed_str);
				}
			}
		}
		if (localtime_r(&ModTime(thing), &tm_modified) != NULL)
		{
			if (strftime(modified_str, sizeof(modified_str), "%a %b %d %H:%M:%S %Y", &tm_modified) > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Accessed: %s    Modified: %s", timebuf, modified_str);
			}
		}

		/*
		 * Print the zone
		 */

		if (mushconf.have_zones)
		{
			buf2 = unparse_object(player, Zone(thing), 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Zone: %s", buf2);
				XFREE(buf2);
			}
		}

		/*
		 * print parent
		 */
		loc = Parent(thing);

		if (loc != NOTHING)
		{
			buf2 = unparse_object(player, loc, 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Parent: %s", buf2);
				XFREE(buf2);
			}
		}

		/*
		 * Show the powers
		 */
		buf2 = power_description(player, thing);
		if (buf2)
		{
			notify(player, buf2);
			XFREE(buf2);
		}
	}

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->examine)
		{
			(*(cam__mp->examine))(player, cause, thing, control, key);
		}
	}

	if (!((key & EXAM_OWNER) || (key & EXAM_BRIEF)))
	{
		look_atrs(player, thing, do_parent, is_special);
	}

	/*
	 * show him interesting stuff
	 */

	if (control)
	{
		/*
		 * Contents
		 */
		if (Contents(thing) != NOTHING)
		{
			notify(player, "Contents:");
			for (content = Contents(thing); (content != NOTHING) && (Next(content) != content); content = Next(content))
			{
				buf2 = unparse_object(player, content, 0);
				if (buf2)
				{
					notify(player, buf2);
					XFREE(buf2);
				}
			}
		}

		/*
		 * Show stuff that depends on the object type
		 */

		switch (Typeof(thing))
		{
		case TYPE_ROOM:

			/*
			 * tell him about exits
			 */
			if (Exits(thing) != NOTHING)
			{
				notify(player, "Exits:");
				for (exit = Exits(thing); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
				{
					buf2 = unparse_object(player, exit, 0);
					if (buf2)
					{
						notify(player, buf2);
						XFREE(buf2);
					}
				}
			}
			else
			{
				notify(player, "No exits.");
			}

			/*
			 * print dropto if present
			 */

			if (Dropto(thing) != NOTHING)
			{
				buf2 = unparse_object(player, Dropto(thing), 0);
				if (buf2)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Dropped objects go to: %s", buf2);
					XFREE(buf2);
				}
			}

			break;

		case TYPE_THING:
		case TYPE_PLAYER:

			/*
			 * tell him about exits
			 */
			if (Exits(thing) != NOTHING)
			{
				notify(player, "Exits:");
				for (exit = Exits(thing); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
				{
					buf2 = unparse_object(player, exit, 0);
					if (buf2)
					{
						notify(player, buf2);
						XFREE(buf2);
					}
				}
			}
			else
			{
				notify(player, "No exits.");
			}

			/*
			 * print home
			 */
			loc = Home(thing);
			buf2 = unparse_object(player, loc, 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Home: %s", buf2);
				XFREE(buf2);
			}
			/*
			 * print location if player can link to it
			 */
			loc = Location(thing);

			if ((Location(thing) != NOTHING) && (Examinable(player, loc) || Examinable(player, thing) || Linkable(player, loc)))
			{
				buf2 = unparse_object(player, loc, 0);
				if (buf2)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Location: %s", buf2);
					XFREE(buf2);
				}
			}

			break;

		case TYPE_EXIT:
			buf2 = unparse_object(player, Exits(thing), 0);
			if (buf2)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Source: %s", buf2);
				XFREE(buf2);
			}

			/*
			 * print destination
			 */

			switch (Location(thing))
			{
			case NOTHING:
				/*
				 * Special case. unparse_object() normally
				 * * returns -1 as '*NOTHING*'.
				 */
				notify(player, "Destination: *UNLINKED*");
				break;

			default:
				buf2 = unparse_object(player, Location(thing), 0);
				if (buf2)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Destination: %s", buf2);
					XFREE(buf2);
				}
				break;
			}

			break;

		default:
			break;
		}
	}
	else if (!Opaque(thing) && nearby(player, thing))
	{
		if (Has_contents(thing))
		{
			look_contents(player, thing, "Contents:", CONTENTS_REMOTE);
		}

		if (!isExit(thing))
		{
			look_exits(player, thing, "Obvious exits:");
		}
	}

	XFREE(temp);

	if (!control)
	{
		if (mushconf.read_rem_name)
		{
			buf2 = XMALLOC(LBUF_SIZE, "buf2");
			if (buf2)
			{
				XSTRCPY(buf2, Name(thing));
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s is owned by %s", buf2, Name(Owner(thing)));
				XFREE(buf2);
			}
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owned by %s", Name(Owner(thing)));
		}
	}
	XFREE(timebuf);
}

void do_score(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You have %d %s.", Pennies(player), (Pennies(player) == 1) ? mushconf.one_coin : mushconf.many_coins);
}

void do_inventory(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
	char *buff = NULL, *e = NULL;
	dbref thing;

	if (Contents(player) == NOTHING)
	{
		notify(player, "You aren't carrying anything.");
	}
	else
	{
		notify(player, "You are carrying:");
		for (thing = Contents(player); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
		{
			buff = unparse_object(player, thing, 1);
			if (buff)
			{
				notify(player, buff);
				XFREE(buff);
			}
		}
	}

	if (Exits(player) != NOTHING)
	{
		notify(player, "Exits:");
		buff = XMALLOC(LBUF_SIZE, "buff");
		if (buff)
		{
			e = buff;
			for (thing = Exits(player); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
			{
				if (e != buff && e && *e)
				{
					if ((e - buff + 2) < LBUF_SIZE)
					{
						XSAFESTRNCAT(buff, &e, (char *)"  ", 2, LBUF_SIZE);
					}
				}

				safe_exit_name(thing, buff, &e);
			}
			if (e && buff)
			{
				*e = '\0';
			}
			notify(player, buff);
			XFREE(buff);
		}
	}

	do_score(player, player, 0);
}

void do_entrances(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *name)
{
	dbref thing = NOTHING, i = NOTHING, j = NOTHING;
	char *exit = NULL, *message = NULL;
	int control_thing = 0, count = 0, low_bound = 0, high_bound = 0;
	FWDLIST *fp = NULL;
	PROPDIR *pp = NULL;
	parse_range(&name, &low_bound, &high_bound);

	if (!name || !*name)
	{
		if (Has_location(player))
		{
			thing = Location(player);
		}
		else
		{
			thing = player;
		}

		if (!Good_obj(thing))
		{
			return;
		}
	}
	else
	{
		init_match(player, name, NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();

		if (!Good_obj(thing))
		{
			return;
		}
	}

	if (!payfor(player, mushconf.searchcost))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s.", mushconf.many_coins);
		return;
	}

	message = XMALLOC(LBUF_SIZE, "message");
	control_thing = Examinable(player, thing);
	count = 0;

	for (i = low_bound; i <= high_bound; i++)
	{
		if (control_thing || Examinable(player, i))
		{
			switch (Typeof(i))
			{
			case TYPE_EXIT:
				if (Location(i) == thing)
				{
					exit = unparse_object(player, Exits(i), 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s (%s)", exit, Name(i));
					XFREE(exit);
					count++;
				}

				break;

			case TYPE_ROOM:
				if (Dropto(i) == thing)
				{
					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [dropto]", exit);
					XFREE(exit);
					count++;
				}

				break;

			case TYPE_THING:
			case TYPE_PLAYER:
				if (Home(i) == thing)
				{
					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [home]", exit);
					XFREE(exit);
					count++;
				}

				break;
			}

			/*
			 * Check for parents
			 */

			if (Parent(i) == thing)
			{
				exit = unparse_object(player, i, 0);
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [parent]", exit);
				XFREE(exit);
				count++;
			}

			/*
			 * Check for propdir
			 */

			if (H_Propdir(i))
			{
				pp = propdir_get(i);

				if (!pp)
				{
					continue;
				}

				for (j = 0; j < pp->count; j++)
				{
					if (pp->data[j] != thing)
					{
						continue;
					}

					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [propdir]", exit);
					XFREE(exit);
					count++;
				}
			}

			/*
			 * Check for forwarding
			 */

			if (H_Fwdlist(i))
			{
				fp = fwdlist_get(i);

				if (!fp)
				{
					continue;
				}

				for (j = 0; j < fp->count; j++)
				{
					if (fp->data[j] != thing)
					{
						continue;
					}

					exit = unparse_object(player, i, 0);
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s [forward]", exit);
					XFREE(exit);
					count++;
				}
			}
		}
	}

	if (message)
	{
		XFREE(message);
	}
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d entrance%s found.", count, (count == 1) ? "" : "s");
}

/* check the current location for bugs */

void sweep_check(dbref player, dbref what, int key, int is_loc)
{
	dbref aowner = NOTHING, parent = NOTHING;
	int canhear = 0, cancom = 0, isplayer = 0, ispuppet = 0, isconnected = 0, is_audible = 0, attr = 0, aflags = 0, alen = 0;
	int is_parent = 0, lev = 0;
	char *buf = NULL, *buf2 = NULL, *bp = NULL, *as = NULL, *buff = NULL, *s = NULL;
	ATTR *ap = NULL;

	if ((key & SWEEP_LISTEN) && ((isExit(what) || is_loc) && Audible(what)))
	{
		is_audible = 1;
	}

	if (key & SWEEP_LISTEN)
	{
		if (H_Listen(what) || Bouncer(what))
		{
			canhear = 1;
		}
		else if (Monitor(what))
		{
			buff = XMALLOC(LBUF_SIZE, "buff");
			if (!buff)
			{
				return;
			}

			for (attr = atr_head(what, &as); attr; attr = atr_next(&as))
			{
				ap = atr_num(attr);

				if (!ap || (ap->flags & AF_NOPROG))
				{
					continue;
				}

				atr_get_str(buff, what, attr, &aowner, &aflags, &alen);

				/*
				 * Make sure we can execute it
				 */

				if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
				{
					continue;
				}

				/*
				 * Make sure there's a : in it
				 */

				for (s = buff + 1; *s && (*s != ':'); s++)
					;

				if (*s == ':')
				{
					canhear = 1;
					break;
				}
			}

			XFREE(buff);
		}
	}

	if ((key & SWEEP_COMMANDS) && !isExit(what))
	{
		/*
		 * Look for commands on the object and parents too
		 */
		for (lev = 0, parent = what; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (Commer(parent))
			{
				cancom = 1;

				if (lev)
				{
					is_parent = 1;
					break;
				}
			}
		}
	}

	if (key & SWEEP_CONNECT)
	{
		if (Connected(what) || (Puppet(what) && Connected(Owner(what))) || (mushconf.player_listen && (Typeof(what) == TYPE_PLAYER) && canhear && Connected(Owner(what))))
		{
			isconnected = 1;
		}
	}

	if (key & SWEEP_PLAYER || isconnected)
	{
		if (Typeof(what) == TYPE_PLAYER)
		{
			isplayer = 1;
		}

		if (Puppet(what))
		{
			ispuppet = 1;
		}
	}

	if (canhear || cancom || isplayer || ispuppet || isconnected || is_audible || is_parent)
	{
		buf = XMALLOC(LBUF_SIZE, "buf");
		if (!buf)
		{
			return;
		}
		bp = buf;

		if (cancom)
		{
			XSAFELBSTR((char *)"commands ", buf, &bp);
		}

		if (canhear)
		{
			XSAFELBSTR((char *)"messages ", buf, &bp);
		}

		if (is_audible)
		{
			XSAFELBSTR((char *)"audible ", buf, &bp);
		}

		if (isplayer)
		{
			XSAFELBSTR((char *)"player ", buf, &bp);
		}

		if (ispuppet)
		{
			XSAFELBSTR((char *)"puppet(", buf, &bp);
			safe_name(Owner(what), buf, &bp);
			XSAFELBSTR((char *)") ", buf, &bp);
		}

		if (isconnected)
		{
			XSAFELBSTR((char *)"connected ", buf, &bp);
		}

		if (is_parent)
		{
			XSAFELBSTR((char *)"parent ", buf, &bp);
		}

		*--bp = '\0'; /* nuke the space at the end */

		if (!isExit(what))
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "  %s is listening. [%s]", Name(what), buf);
		}
		else
		{
			bp = buf2 = XMALLOC(LBUF_SIZE, "buf2");
			if (!buf2)
			{
				XFREE(buf);
				return;
			}
			safe_exit_name(what, buf2, &bp);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "  %s is listening. [%s]", buf2, buf);
			XFREE(buf2);
		}

		XFREE(buf);
	}
}

void do_sweep(dbref player, dbref cause __attribute__((unused)), int key, char *where)
{
	dbref here, sweeploc;
	int where_key, what_key;
	where_key = key & (SWEEP_ME | SWEEP_HERE | SWEEP_EXITS);
	what_key = key & (SWEEP_COMMANDS | SWEEP_LISTEN | SWEEP_PLAYER | SWEEP_CONNECT);

	if (where && *where)
	{
		sweeploc = match_controlled(player, where);

		if (!Good_obj(sweeploc))
		{
			return;
		}
	}
	else
	{
		sweeploc = player;
	}

	if (!where_key)
	{
		where_key = -1;
	}

	if (!what_key)
	{
		what_key = -1;
	}
	else if (what_key == SWEEP_VERBOSE)
	{
		what_key = SWEEP_VERBOSE | SWEEP_COMMANDS;
	}

	/*
	 * Check my location.  If I have none or it is dark, check just me.
	 */

	if (where_key & SWEEP_HERE)
	{
		notify(player, "Sweeping location...");

		if (Has_location(sweeploc))
		{
			here = Location(sweeploc);

			if ((here == NOTHING) || (Dark(here) && !mushconf.sweep_dark && !Examinable(player, here)))
			{
				notify_quiet(player, "Sorry, it is dark here and you can't search for bugs");
				sweep_check(player, sweeploc, what_key, 0);
			}
			else
			{
				sweep_check(player, here, what_key, 1);

				for (here = Contents(here); here != NOTHING; here = Next(here))
				{
					sweep_check(player, here, what_key, 0);
				}
			}
		}
		else
		{
			sweep_check(player, sweeploc, what_key, 0);
		}
	}

	/*
	 * Check exits in my location
	 */

	if ((where_key & SWEEP_EXITS) && Has_location(sweeploc))
	{
		notify(player, "Sweeping exits...");

		for (here = Exits(Location(sweeploc)); (here != NOTHING) && (Next(here) != here); here = Next(here))
		{
			sweep_check(player, here, what_key, 0);
		}
	}

	/*
	 * Check my inventory
	 */

	if ((where_key & SWEEP_ME) && Has_contents(sweeploc))
	{
		notify(player, "Sweeping inventory...");

		for (here = Contents(sweeploc); (here != NOTHING) && (Next(here) != here); here = Next(here))
		{
			sweep_check(player, here, what_key, 0);
		}
	}

	/*
	 * Check carried exits
	 */

	if ((where_key & SWEEP_EXITS) && Has_exits(sweeploc))
	{
		notify(player, "Sweeping carried exits...");

		for (here = Exits(sweeploc); (here != NOTHING) && (Next(here) != here); here = Next(here))
		{
			sweep_check(player, here, what_key, 0);
		}
	}

	notify(player, "Sweep complete.");
}

/* Output the sequence of commands needed to duplicate the specified
 * object.  If you're moving things to another system, your mileage
 * will almost certainly vary.  (i.e. different flags, etc.)
 */

void do_decomp(dbref player, dbref cause __attribute__((unused)), int key, char *name, char *qual)
{
	BOOLEXP *bexp = NULL;
	char *got = NULL, *thingname = NULL, *as = NULL, *ltext = NULL, *buff = NULL, *tbuf = NULL, *tmp = NULL, *buf = NULL;
	dbref aowner = NOTHING, thing = NOTHING;
	int val = 0, aflags = 0, alen = 0, ca = 0, wild_decomp = 0;
	ATTR *attr = NULL;
	NAMETAB *np = NULL;
	/*
	 * Check for obj/attr first
	 */
	olist_push();

	if (parse_attrib_wild(player, name, &thing, 0, 1, 0, 1))
	{
		wild_decomp = 1;
	}
	else
	{
		wild_decomp = 0;
		init_match(player, name, TYPE_THING);
		match_everything(MAT_EXIT_PARENTS);
		thing = noisy_match_result();
	}

	/*
	 * get result
	 */
	if (thing == NOTHING)
	{
		olist_pop();
		return;
	}

	if (!Examinable(player, thing))
	{
		notify_quiet(player, "You can only decompile things you can examine.");
		olist_pop();
		return;
	}

	thingname = atr_get(thing, A_LOCK, &aowner, &aflags, &alen);
	bexp = parse_boolexp(player, thingname, 1);

	/*
	 * Determine the name of the thing to use in reporting and then
	 * * report the command to make the thing.
	 */

	if (qual && *qual)
	{
		XSTRCPY(thingname, qual);
	}
	else
	{
		switch (Typeof(thing))
		{
		case TYPE_THING:
			XSTRCPY(thingname, Name(thing));
			val = OBJECT_DEPOSIT(Pennies(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@create %s=%d", buf, val);
			XFREE(buf);
			break;

		case TYPE_ROOM:
			XSTRCPY(thingname, Name(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@dig/teleport %s", buf);
			XFREE(buf);
			XSTRCPY(thingname, "here");
			break;

		case TYPE_EXIT:
			XSTRCPY(thingname, Name(thing));
			buf = translate_string_ansi(thingname, 1);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@open %s", buf);
			XFREE(buf);
			got = thingname;
			safe_exit_name(thing, thingname, &got);
			break;

		case TYPE_PLAYER:
			XSTRCPY(thingname, "me");
			break;
		}
	}

	buf = ansi_strip_ansi(thingname);
	XSTRCPY(thingname, buf);
	XFREE(buf);

	/*
	 * Report the lock (if any)
	 */

	if (!wild_decomp && (bexp != TRUE_BOOLEXP))
	{
		buf = unparse_boolexp_decompile(player, bexp);
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock %s=%s", thingname, buf);
		XFREE(buf);
	}

	free_boolexp(bexp);
	/*
	 * Report attributes
	 */
	buff = XMALLOC(MBUF_SIZE, "buff");

	for (ca = (wild_decomp ? olist_first() : atr_head(thing, &as)); (wild_decomp) ? (ca != NOTHING) : (ca != (int)0); ca = (wild_decomp ? olist_next() : atr_next(&as)))
	{
		if ((ca == A_NAME) || (ca == A_LOCK))
		{
			continue;
		}

		attr = atr_num(ca);

		if (!attr)
		{
			continue;
		}

		if ((attr->flags & AF_NOCMD) && !(attr->flags & AF_IS_LOCK))
		{
			continue;
		}

		got = atr_get(thing, ca, &aowner, &aflags, &alen);

		if (aflags & AF_STRUCTURE)
		{
			tmp = replace_string(GENERIC_STRUCT_STRDELIM, mushconf.struct_dstr, got);
			XFREE(got);
			got = tmp;
		}

		if (Read_attr_all(player, thing, attr, aowner, aflags, 1))
		{
			if (attr->flags & AF_IS_LOCK)
			{
				bexp = parse_boolexp(player, got, 1);
				ltext = unparse_boolexp_decompile(player, bexp);
				free_boolexp(bexp);
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock/%s %s=%s", attr->name, thingname, ltext);
				XFREE(ltext);
			}
			else
			{
				XSTRCPY(buff, attr->name);

				if (key & DECOMP_PRETTY)
				{
					tbuf = XMALLOC(LBUF_SIZE, "tbuf");
					buf = XMALLOC(GBUF_SIZE, "buf");
					if (tbuf && buf)
					{
						XSNPRINTF(buf, GBUF_SIZE, "%c%s %s=", ((ca < A_USER_START) ? '@' : '&'), buff, thingname);
						pretty_print(tbuf, buf, got);
						notify(player, tbuf);
					}
					if (tbuf)
						XFREE(tbuf);
					if (buf)
						XFREE(buf);
				}
				else
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%c%s %s=%s", ((ca < A_USER_START) ? '@' : '&'), buff, thingname, got);
				}

				for (np = indiv_attraccess_nametab; np->name; np++)
				{
					if ((aflags & np->flag) && check_access(player, np->perm) && (!(np->perm & CA_NO_DECOMP)))
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@set %s/%s = %s", thingname, buff, np->name);
					}
				}

				if (aflags & AF_LOCK)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@lock %s/%s", thingname, buff);
				}
			}
		}

		XFREE(got);
	}

	XFREE(buff);

	if (!wild_decomp)
	{
		decompile_flags(player, thing, thingname);
		decompile_powers(player, thing, thingname);
	}

	/*
	 * If the object has a parent, report it
	 */

	if (!wild_decomp && (Parent(thing) != NOTHING))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@parent %s=#%d", thingname, Parent(thing));
	}

	/*
	 * If the object has a zone, report it
	 */

	if (!wild_decomp && (Zone(thing) != NOTHING))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "@chzone %s=#%d", thingname, Zone(thing));
	}

	XFREE(thingname);
	olist_pop();
}

void show_vrml_url(dbref thing, dbref loc)
{
	char *vrml_url = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;

	/*
	 * If they don't care about HTML, just return.
	 */
	if (!Html(thing))
	{
		return;
	}

	vrml_url = atr_pget(loc, A_VRML_URL, &aowner, &aflags, &alen);

	if (vrml_url && *vrml_url)
	{
		char *vrml_message = NULL, *vrml_cp = NULL;
		vrml_message = XMALLOC(LBUF_SIZE, "vrml_message");
		if (vrml_message)
		{
			vrml_cp = vrml_message;
			XSAFELBSTR("<img xch_graph=load href=\"", vrml_message, &vrml_cp);
			XSAFELBSTR(vrml_url, vrml_message, &vrml_cp);
			XSAFELBSTR("\">", vrml_message, &vrml_cp);
			*vrml_cp = 0;
			notify_html(thing, vrml_message);
			XFREE(vrml_message);
		}
	}
	else
	{
		notify_html(thing, "<img xch_graph=hide>");
	}

	if (vrml_url)
		XFREE(vrml_url);
}
