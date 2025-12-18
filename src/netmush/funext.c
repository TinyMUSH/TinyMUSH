/**
 * @file funext.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Functions that rely on external call-outs
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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
 * @brief Parse port number or player name with visibility checks.
 *
 * @param viewer DBref of player doing the lookup
 * @param arg Argument string (port number or player name)
 * @param target Output: resolved player dbref (or NOTHING)
 * @param port Output: port number (or NOTHING)
 */
static void parse_port_or_player(dbref viewer, char *arg, dbref *target, int *port)
{
	*target = NOTHING;
	*port = NOTHING;

	if (is_integer(arg))
	{
		*port = (int)strtol(arg, (char **)NULL, 10);
	}
	else
	{
		*target = lookup_player(viewer, arg, 1);
		if (Good_obj(*target) && Can_Hide(*target) && Hidden(*target) && !See_Hidden(viewer))
		{
			*target = NOTHING;
		}
	}
}

/**
 * @brief Return a MUSH config parameter.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Functions arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_config(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	cf_display(player, fargs[0], buff, bufc);
}

/**
 * @brief Return list of connected users.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_lwho(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	make_ulist(player, buff, bufc);
}

/**
 * @brief Returns a list of ports for a user.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ports(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref target = NOTHING;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 1, buff, bufc))
	{
		return;
	}

	if (fargs[0] && *fargs[0])
	{
		target = lookup_player(player, fargs[0], 1);

		if (!Good_obj(target) || !Connected(target))
		{
			return;
		}

		make_portlist(player, target, buff, bufc);
	}
	else
	{
		make_portlist(player, NOTHING, buff, bufc);
	}
}

/**
 * @brief Returns a user's doing.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_doing(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref target;
	int port;
	char *str = NULL;

	parse_port_or_player(player, fargs[0], &target, &port);

	if ((port < 0) && (target == NOTHING))
	{
		return;
	}

	str = get_doing(target, port);

	if (str)
	{
		XSAFELBSTR(str, buff, bufc);
	}
}

/**
 * @brief Return seconds idle or connected (IDLE, CONN).
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void handle_conninfo(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref target;
	int port;

	parse_port_or_player(player, fargs[0], &target, &port);

	if ((port < 0) && (target == NOTHING))
	{
		XSAFESTRNCAT(buff, bufc, (char *)"-1", 2, LBUF_SIZE);
		return;
	}

	SAFE_LTOS(buff, bufc, Is_Func(CONNINFO_IDLE) ? fetch_idle(target, port) : fetch_connect(target, port), LBUF_SIZE);
}

/**
 * @brief Return session info about a port.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_session(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref target;
	int port;

	parse_port_or_player(player, fargs[0], &target, &port);

	if ((port < 0) && (target == NOTHING))
	{
		XSAFELBSTR((char *)"-1 -1 -1", buff, bufc);
		return;
	}

	make_sessioninfo(player, target, port, buff, bufc);
}

/**
 * @brief Returns the dbref or #1- of an object in a @program.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_programmer(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	dbref target = lookup_player(player, fargs[0], 1);

	if (!Good_obj(target) || !Connected(target) || !Examinable(player, target))
	{
		XSAFENOTHING(buff, bufc);
		return;
	}

	XSAFELBCHR('#', buff, bufc);
	SAFE_LTOS(buff, bufc, get_programmer(target), LBUF_SIZE);
}

/**
 * @brief Read an entry from a helpfile.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_helptext(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[], int ncargs)
{
	CMDENT *cmdp = NULL;
	char *p = NULL;
	char *cmd_lower = NULL;

	if (!fargs[0] || !*fargs[0])
	{
		XSAFELBSTR((char *)"#-1 NOT FOUND", buff, bufc);
		return;
	}

	/**
	 * Copy to temp buffer to avoid mutating fargs.
	 */
	cmd_lower = XSTRDUP(fargs[0], "cmd_lower");
	for (p = cmd_lower; *p; p++)
	{
		*p = tolower(*p);
	}

	cmdp = (CMDENT *)hashfind(cmd_lower, &mushstate.command_htab);

	if (!cmdp || (cmdp->info.handler != do_help))
	{
		XFREE(cmd_lower);
		XSAFELBSTR((char *)"#-1 NOT FOUND", buff, bufc);
		return;
	}

	if (!check_cmd_access(player, cmdp, cargs, ncargs))
	{
		XFREE(cmd_lower);
		XSAFENOPERM(buff, bufc);
		return;
	}

	XFREE(cmd_lower);
	help_helper(player, (cmdp->extra & ~HELP_RAWHELP), (cmdp->extra & HELP_RAWHELP) ? 0 : 1, fargs[1], buff, bufc);
}

/**
 * @brief Pueblo HTML-related functions.
 *
 */

/**
 * @brief Escape HTML
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_html_escape(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	html_escape(fargs[0], buff, bufc);
}

/**
 * @brief Un-escape HTML
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_html_unescape(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	const char *msg_orig = NULL;
	int ret = 0;

	for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++)
	{
		switch (*msg_orig)
		{
		case '&':
			if (!strncmp(msg_orig, "&quot;", 6))
			{
				ret = XSAFELBCHR('\"', buff, bufc);
				msg_orig += 5;
			}
			else if (!strncmp(msg_orig, "&lt;", 4))
			{
				ret = XSAFELBCHR('<', buff, bufc);
				msg_orig += 3;
			}
			else if (!strncmp(msg_orig, "&gt;", 4))
			{
				ret = XSAFELBCHR('>', buff, bufc);
				msg_orig += 3;
			}
			else if (!strncmp(msg_orig, "&amp;", 5))
			{
				ret = XSAFELBCHR('&', buff, bufc);
				msg_orig += 4;
			}
			else
			{
				ret = XSAFELBCHR('&', buff, bufc);
			}

			break;

		default:
			ret = XSAFELBCHR(*msg_orig, buff, bufc);
			break;
		}
	}
}

/**
 * @brief Check if a characters should be converted to %<hex>
 *
 * @param ch Character to check
 * @return true Convert to hex
 * @return false Keep as is.
 */
bool escaped_chars(unsigned char ch)
{
	switch (ch)
	{
	case '<':
	case '>':
	case '#':
	case '%':
	case '{':
	case '}':
	case '|':
	case '\\':
	case '^':
	case '~':
	case '[':
	case ']':
	case '\'':
	case ';':
	case '/':
	case '?':
	case ':':
	case '@':
	case '=':
	case '&':
	case '\"':
	case '+':
		return true;
	}
	return false;
}

/**
 * @brief Escape URL
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_url_escape(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{

	const char *msg_orig = NULL;
	int ret = 0;
	char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");

	for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++)
	{
		if (escaped_chars(*msg_orig))
		{
			XSPRINTF(tbuf, "%%02X", (unsigned char)*msg_orig);
			ret = XSAFELBSTR(tbuf, buff, bufc);
		}
		else if (*msg_orig == ' ')
		{
			ret = XSAFELBCHR('+', buff, bufc);
		}
		else
		{
			ret = XSAFELBCHR(*msg_orig, buff, bufc);
		}
	}
	XFREE(tbuf);
}

/**
 * @brief Un-escape URL
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_url_unescape(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	const char *msg_orig;
	int ret = 0;
	unsigned int tempchar = 0;
	char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");

	for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret;)
	{
		switch (*msg_orig)
		{
		case '+':
			ret = XSAFELBCHR(' ', buff, bufc);
			msg_orig++;
			break;

		case '%':
			XSTRNCPY(tbuf, msg_orig + 1, 2);
			tbuf[2] = '\0';

			if ((sscanf(tbuf, "%x", &tempchar) == 1) && (tempchar > 0x1F) && (tempchar < 0x7F))
			{
				ret = XSAFELBCHR((char)tempchar, buff, bufc);
			}

			if (*msg_orig)
			{
				/**
				 * Skip the '%'
				 *
				 */
				msg_orig++;
			}

			if (*msg_orig)
			{
				/**
				 * Skip the 1st hex character.
				 *
				 */
				msg_orig++;
			}

			if (*msg_orig)
			{
				/**
				 * Skip the 2nd hex character.
				 *
				 */
				msg_orig++;
			}

			break;

		default:
			ret = XSAFELBCHR(*msg_orig, buff, bufc);
			msg_orig++;
			break;
		}
	}

	XFREE(tbuf);
	return;
}
