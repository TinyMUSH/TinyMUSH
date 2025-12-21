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

/**
 * @brief Thread-safe wrapper for strerror
 *
 * @param errnum Error number
 * @return const char* Error message string
 */
const char *safe_strerror(int errnum)
{
	static __thread char errbuf[256];
	strerror_r(errnum, errbuf, sizeof(errbuf));
	return errbuf;
}

/**
 * @brief High-resolution timer using clock_gettime
 * Returns struct timeval for compatibility with existing code
 *
 * @param tv Pointer to timeval structure to fill
 * @param tz Unused (for gettimeofday compatibility)
 * @return int 0 on success, -1 on error
 */
int safe_gettimeofday(struct timeval *tv, void *tz __attribute__((unused)))
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
 * \fn char *translate_string ( char *str, int type )
 * \brief Convert raw ansi to mushcode or strip it.
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param str Pointer to the string to be translated
 * \param type 1 = Convert to mushcode, 0 strip ansi.
 *
 * \return Pointer to the translatted string.
 */

char *translate_string(char *str, int type)
{
	char *buff, *bp;
	bp = buff = XMALLOC(LBUF_SIZE, "buff");

	if (type)
	{
		int ansi_state = ANST_NORMAL;
		int ansi_state_prev = ANST_NORMAL;

		while (*str)
		{
			switch (*str)
			{
			case ESC_CHAR:
				while (*str == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(str);
						if (*(str) == ANSI_CSI)
						{
							while ((*(++(str)) & 0xf0) == 0x30)
							{
								if (*(str) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(str) & 0x0f);
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
						while ((*(str) & 0xf0) == 0x20)
						{
							++(str);
						}
						if (*(str) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++(str);
						}
						else if (*(str))
						{
							++(str);
						}
					} while (0);
				}

				char *transition = ansi_transition_mushcode(ansi_state_prev, ansi_state);
				XSAFELBSTR(transition, buff, &bp);
				XFREE(transition);
				ansi_state_prev = ansi_state;
				continue;

			case ' ':
				if (str[1] == ' ')
				{
					XSAFESTRNCAT(buff, &bp, "%b", 2, LBUF_SIZE);
				}
				else
				{
					XSAFELBCHR(' ', buff, &bp);
				}

				break;

			case '\\':
			case '%':
			case '[':
			case ']':
			case '{':
			case '}':
			case '(':
			case ')':
				XSAFELBCHR('%', buff, &bp);
				XSAFELBCHR(*str, buff, &bp);
				break;

			case '\r':
				break;

			case '\n':
				XSAFESTRNCAT(buff, &bp, "%r", 2, LBUF_SIZE);
				break;

			case '\t':
				XSAFESTRNCAT(buff, &bp, "%t", 2, LBUF_SIZE);
				break;

			default:
				XSAFELBCHR(*str, buff, &bp);
			}

			str++;
		}
	}
	else
	{
		while (*str)
		{
			switch (*str)
			{
			case ESC_CHAR:
				skip_esccode(&str);
				continue;

			case '\r':
				break;

			case '\n':
			case '\t':
				XSAFELBCHR(' ', buff, &bp);
				break;

			default:
				XSAFELBCHR(*str, buff, &bp);
			}

			str++;
		}
	}

	*bp = '\0';
	return buff;
}

/*
 * ---------------------------------------------------------------------------
 * rgb2xterm -- Convert an RGB value to xterm color
 */

/**
 * \fn int rgb2xterm ( long rgb )
 * \brief Convert an RGB value to xterm value
 *
 * \param rgb Color to convert to xterm value
 *
 * \return The xterm value of the color.
 */

int rgb2xterm(long rgb)
{
	int xterm, r, g, b;

	/* First, handle standard colors */
	if (rgb == 0x000000)
	{
		return (0);
	}

	if (rgb == 0x800000)
	{
		return (1);
	}

	if (rgb == 0x008000)
	{
		return (2);
	}

	if (rgb == 0x808000)
	{
		return (3);
	}

	if (rgb == 0x000080)
	{
		return (4);
	}

	if (rgb == 0x800080)
	{
		return (5);
	}

	if (rgb == 0x008080)
	{
		return (6);
	}

	if (rgb == 0xc0c0c0)
	{
		return (7);
	}

	if (rgb == 0x808080)
	{
		return (8);
	}

	if (rgb == 0xff0000)
	{
		return (9);
	}

	if (rgb == 0x00ff00)
	{
		return (10);
	}

	if (rgb == 0xffff00)
	{
		return (11);
	}

	if (rgb == 0x0000ff)
	{
		return (12);
	}

	if (rgb == 0xff00ff)
	{
		return (13);
	}

	if (rgb == 0x00ffff)
	{
		return (14);
	}

	if (rgb == 0xffffff)
	{
		return (15);
	}

	r = (rgb & 0xFF0000) >> 16;
	g = (rgb & 0x00FF00) >> 8;
	b = rgb & 0x0000FF;

	/* Next, handle grayscales */

	if ((r == g) && (r == b))
	{
		if (rgb <= 0x080808)
		{
			return (232);
		}

		if (rgb <= 0x121212)
		{
			return (233);
		}

		if (rgb <= 0x1c1c1c)
		{
			return (234);
		}

		if (rgb <= 0x262626)
		{
			return (235);
		}

		if (rgb <= 0x303030)
		{
			return (236);
		}

		if (rgb <= 0x3a3a3a)
		{
			return (237);
		}

		if (rgb <= 0x444444)
		{
			return (238);
		}

		if (rgb <= 0x4e4e4e)
		{
			return (239);
		}

		if (rgb <= 0x585858)
		{
			return (240);
		}

		if (rgb <= 0x606060)
		{
			return (241);
		}

		if (rgb <= 0x666666)
		{
			return (242);
		}

		if (rgb <= 0x767676)
		{
			return (243);
		}

		if (rgb <= 0x808080)
		{
			return (244);
		}

		if (rgb <= 0x8a8a8a)
		{
			return (245);
		}

		if (rgb <= 0x949494)
		{
			return (246);
		}

		if (rgb <= 0x9e9e9e)
		{
			return (247);
		}

		if (rgb <= 0xa8a8a8)
		{
			return (248);
		}

		if (rgb <= 0xb2b2b2)
		{
			return (249);
		}

		if (rgb <= 0xbcbcbc)
		{
			return (250);
		}

		if (rgb <= 0xc6c6c6)
		{
			return (251);
		}

		if (rgb <= 0xd0d0d0)
		{
			return (252);
		}

		if (rgb <= 0xdadada)
		{
			return (253);
		}

		if (rgb <= 0xe4e4e4)
		{
			return (254);
		}

		if (rgb <= 0xeeeeee)
		{
			return (255);
		}
	}

	/* It's an RGB, convert it */
	xterm = (((r / 51) * 36) + ((g / 51) * 6) + (b / 51)) + 16;

	/* Just in case... */

	if (xterm < 16)
	{
		xterm = 16;
	}

	if (xterm > 231)
	{
		xterm = 231;
	}

	return (xterm);
}

/**
 * \fn int str2xterm ( char *str )
 * \brief Convert a value to xterm color
 *
 * \param str A string representing the color to be convert into xterm value.
 *            The value can be express as hex (#rrggbb), decimal (r g b). a 24
 *            bit integer value, or the actual xterm value.
 *
 * \return The xterm value of the color.
 */

int str2xterm(char *str)
{
	long rgb;
	int r, g, b;
	char *p, *t;
	p = str;

	if (*p == '#')
	{ /* Ok, it's a RGB in hex */
		p++;
		rgb = strtol(p, &t, 16);

		if (p == t)
		{
			return (-1);
		}
		else
		{

			return (rgb2xterm(rgb));
		}
	}
	else
	{ /* Then it must be decimal */
		r = strtol(p, &t, 10);

		if (p == t)
		{
			return (-1);
		}
		else if (*t == 0)
		{
			if (r < 256)
			{
				return (r); /* It's the color index */
			}

			return (rgb2xterm(r)); /* It's a RGB, fetch the index */
		}
		else
		{
			p = t;

			while (!isdigit((unsigned char)*p) && (*p != 0))
			{
				p++;
			}

			g = strtol(p, &t, 10);

			if ((p == t) || (*p == 0))
			{
				return (-1);
			}

			p = t;

			while (!isdigit((unsigned char)*p) && (*p != 0))
			{
				p++;
			}

			b = strtol(p, &t, 10);

			if ((p == t) || (*p == 0))
			{
				return (-1);
			}

			rgb = (r << 16) + (g << 8) + b;
			return (rgb2xterm(rgb));
		}
	}

	return (-1); /* Something is terribly wrong... */
}

/**
 * \fn char *upcasestr ( char *s )
 * \brief Capitalizes an entire string
 *
 * \param s The string that need to be capitalized.
 *
 * \return The string capitalized.
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
 * \fn char *munge_space ( char *string )
 * \brief Compress multiple spaces into one and remove leading/trailing spaces.
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param s The string that need to be munge
 *
 * \return The string munged
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
 * \fn char *trim_spaces ( char *string )
 * \brief Remove leading and trailing spaces.
 *
 * \param s The string that need to be trimmed.
 *
 * \return The trimmed string.
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
 * \fn char *grabto ( char **str, char targ )
 * \brief Return portion of a string up to the indicated character.
 *
 * Note that str will be move to the position following the searched
 * character. If you need the original string, save it before calling
 * this function.
 *
 * \param str The string you want to search
 * \param targ The caracter you want to search for,
 *
 * \return The string up to the character you've search for.
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
 * \fn int string_compare ( const char *s1, const char *s2 )
 * \brief Compare two string. Treat multiple spaces as one.
 *
 * \param s1 The first string to compare
 * \param s2 The second string to compare
 *
 * \return 0 if the string are different.
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
 * \fn int string_prefix ( const char *string, const char *prefix )
 * \brief Compare a string with a prefix
 *
 * \param string The string you want to search
 * \param prefix The prefix you are searching for.
 *
 * \return 0 if the prefix isn't found.
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
 * \fn const char *string_match ( const char *src, const char *sub )
 * \brief Compare a string with a substring, accepts only nonempty matches starting at the beginning of a word
 *
 * \param src The string you want to search
 * \param sub The search term.
 *
 * \return The position of the search term. 0 if not found.
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
 * \fn char *replace_string ( const char *old, const char *new, const char *string )
 * \brief Replace all occurences of a substring with a new substring.
 *
 * \param old The string you want to replace.
 * \param new The string you want to replace with.
 * \param string The string that is modified.
 *
 * \return A pointer to the modified string.
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
 * \fn void edit_string ( char *src, char **dst, char *from, char *to )
 * \brief Replace all occurences of a substring with a new substring.
 *        Sensitive about ANSI codes, and handles special ^ and $ cases.
 *
 * \param src Pointer to the original string.
 * \param dst Pointer to the new string.
 * \param from Pointer to the string to be replaced
 * \param to Pointer to the string to be replaced with.
 *
 * \return None
 */

void edit_string(char *src, char **dst, char *from, char *to)
{
	char *cp, *p;
	int ansi_state, to_ansi_set, to_ansi_clr, tlen, flen;
	/*
	 * We may have gotten an ANSI_NORMAL termination to OLD and NEW,
	 * that the user probably didn't intend to be there. (If the
	 * user really did want it there, he simply has to put a double
	 * ANSI_NORMAL in; this is non-intuitive but without it we can't
	 * let users swap one ANSI code for another using this.)  Thus,
	 * we chop off the terminating ANSI_NORMAL on both, if there is
	 * one.
	 */
	p = from + strlen(from) - 4;

	if (p >= from && !strcmp(p, ANSI_NORMAL))
	{
		*p = '\0';
	}

	p = to + strlen(to) - 4;

	if (p >= to && !strcmp(p, ANSI_NORMAL))
	{
		*p = '\0';
	}

	/*
	 * Scan the contents of the TO string. Figure out whether we
	 * have any embedded ANSI codes.
	 */
	ansi_state = ANST_NONE;

	do
	{
		p = to;
		while (*p)
		{
			if (*p == ESC_CHAR)
			{
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++p;
					if (*p == ANSI_CSI)
					{
						while ((*(++p) & 0xf0) == 0x30)
						{
							if (*p < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*p & 0x0f);
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
					while ((*p & 0xf0) == 0x20)
					{
						++p;
					}
					if (*p == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
						++p;
					}
					else if (*p)
					{
						++p;
					}
				} while (0);
			}
			else
			{
				++p;
			}
		}
	} while (0);

	to_ansi_set = (~ANST_NONE) & ansi_state;
	to_ansi_clr = ANST_NONE & (~ansi_state);
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
				if (*p == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++p;
						if (*p == ANSI_CSI)
						{
							while ((*(++p) & 0xf0) == 0x30)
							{
								if (*p < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*p & 0x0f);
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
						while ((*p & 0xf0) == 0x20)
						{
							++p;
						}
						if (*p == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++p;
						}
						else if (*p)
						{
							++p;
						}
					} while (0);
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
		ansi_state = ANST_NONE;

		do
		{
			p = src;
			while (*p)
			{
				if (*p == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++p;
						if (*p == ANSI_CSI)
						{
							while ((*(++p) & 0xf0) == 0x30)
							{
								if (*p < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*p & 0x0f);
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
						while ((*p & 0xf0) == 0x20)
						{
							++p;
						}
						if (*p == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++p;
						}
						else if (*p)
						{
							++p;
						}
					} while (0);
				}
				else
				{
					++p;
				}
			}
		} while (0);

		XSAFESTRNCAT(*dst, &cp, src, p - src, LBUF_SIZE);
		ansi_state |= to_ansi_set;
		ansi_state &= ~to_ansi_clr;
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
		ansi_state = ANST_NONE;

		while (*src)
		{
			/* Copy up to the next occurrence of the first char of FROM. */
			p = src;

			while (*src && (*src != *from))
			{
				if (*src == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(src);
						if (*(src) == ANSI_CSI)
						{
							while ((*(++(src)) & 0xf0) == 0x30)
							{
								if (*(src) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(src) & 0x0f);
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
						while ((*(src) & 0xf0) == 0x20)
						{
							++(src);
						}
						if (*(src) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++(src);
						}
						else if (*(src))
						{
							++(src);
						}
					} while (0);
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
					ansi_state |= to_ansi_set;
					ansi_state &= ~to_ansi_clr;
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
					if (*from == ESC_CHAR)
					{
						p = src;
						do
						{
							int ansi_mask = 0;
							int ansi_diff = 0;
							unsigned int param_val = 0;
							++(src);
							if (*(src) == ANSI_CSI)
							{
								while ((*(++(src)) & 0xf0) == 0x30)
								{
									if (*(src) < 0x3a)
									{
										param_val <<= 1;
										param_val += (param_val << 2) + (*(src) & 0x0f);
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
							while ((*(src) & 0xf0) == 0x20)
							{
								++(src);
							}
							if (*(src) == ANSI_END)
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
								++(src);
							}
							else if (*(src))
							{
								++(src);
							}
						} while (0);
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

	p = ansi_transition_esccode(ansi_state, ANST_NONE);
	XSAFELBSTR(p, *dst, &cp);
	XFREE(p);
}

/**
 * \fn int minmatch ( char *str, char *target, int min )
 * \brief Find if a substring at the start of a string exist.
 *
 * At least MIN characters must match. This is how flags are
 * match by the command queue.
 *
 * \param str Pointer to the string that will be search.
 * \param target Pointer to the string being search.
 * \param min Minimum length of match.
 *
 * \return 1 if found, 0 if not.
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
 * \fn int matches_exit_from_list ( char *exit_list, char *pattern )
 * \brief Find if pattern is found in the exit list.
 *
 * \param exit_list Pointer to the list of names of an exit
 * \param pattern Pointer to the pattern we are searching
 *
 * \return 1 if the pattern is in the list, 0 if not.
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
 * \fn char *ltos ( char long num )
 * \brief Convert a long signed number into string.
 *
 * It is the caller's responsibility to free the returned buffer with
 * XFREE();
 *
 * \param num Number to convert.
 *
 * \return A pointer to the resulting string.
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
 * \fn char *repeatchar ( int count, char ch )
 * \brief Return a string with 'count' number of 'ch' characters.
 *
 * It is the responsibility of the caller to free the resulting
 * buffer.
 *
 * \param count Length of the string to build
 * \param ch Character used to fill the string.
 *
 * \return A Pointer to the new string.
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
 * \fn void skip_esccode ( char **s )
 * \brief Move the pointer after an ansi escape sequence.
 *
 * \param s Pointer that need to be modified.
 */

