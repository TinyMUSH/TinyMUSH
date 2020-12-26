/* funext.c - Functions that rely on external call-outs */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"	/* required by mudconf */
#include "mushconf.h"	/* required by code */
#include "db.h"			/* required by externs */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by code */
#include "functions.h"	/* required by code */
#include "powers.h"		/* required by code */
#include "command.h"	/* required by code */
#include "stringutil.h" /* required by code */

#define Find_Connection(x, s, t, p)                    \
	p = t = NOTHING;                                   \
	if (is_integer(s))                                 \
	{                                                  \
		p = (int)strtol(s, (char **)NULL, 10);         \
	}                                                  \
	else                                               \
	{                                                  \
		t = lookup_player(x, s, 1);                    \
		if (Good_obj(t) && Can_Hide(t) && Hidden(t) && \
			!See_Hidden(x))                            \
			t = NOTHING;                               \
	}

/*
 * ---------------------------------------------------------------------------
 * config: Display a MUSH config parameter.
 */

void fun_config(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	cf_display(player, fargs[0], buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_lwho: Return list of connected users.
 */

void fun_lwho(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	make_ulist(player, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_ports: Returns a list of ports for a user.
 */

void fun_ports(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref target;
	VaChk_Range(0, 1);

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

/*
 * ---------------------------------------------------------------------------
 * fun_doing: Returns a user's doing.
 */

void fun_doing(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref target;
	int port;
	char *str;
	Find_Connection(player, fargs[0], target, port);

	if ((port < 0) && (target == NOTHING))
	{
		return;
	}

	str = get_doing(target, port);

	if (str)
	{
		SAFE_LB_STR(str, buff, bufc);
	}
}

/*
 * ---------------------------------------------------------------------------
 * handle_conninfo: return seconds idle or connected (IDLE, CONN).
 */

void handle_conninfo(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref target;
	int port;
	Find_Connection(player, fargs[0], target, port);

	if ((port < 0) && (target == NOTHING))
	{
		SAFE_STRNCAT(buff, bufc, (char *)"-1", 2, LBUF_SIZE);
		return;
	}

	SAFE_LTOS(buff, bufc, Is_Func(CONNINFO_IDLE) ? fetch_idle(target, port) : fetch_connect(target, port), LBUF_SIZE);
}

/*
 * ---------------------------------------------------------------------------
 * fun_session: Return session info about a port.
 */

void fun_session(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref target;
	int port;
	Find_Connection(player, fargs[0], target, port);

	if ((port < 0) && (target == NOTHING))
	{
		SAFE_LB_STR((char *)"-1 -1 -1", buff, bufc);
		return;
	}

	make_sessioninfo(player, target, port, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_programmer: Returns the dbref or #1- of an object in a @program.
 */

void fun_programmer(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref target;
	target = lookup_player(player, fargs[0], 1);

	if (!Good_obj(target) || !Connected(target) || !Examinable(player, target))
	{
		SAFE_NOTHING(buff, bufc);
		return;
	}

	safe_dbref(buff, bufc, get_programmer(target));
}

/*---------------------------------------------------------------------------
 * fun_helptext: Read an entry from a helpfile.
 */

void fun_helptext(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	CMDENT *cmdp;
	char *p;

	if (!fargs[0] || !*fargs[0])
	{
		SAFE_LB_STR((char *)"#-1 NOT FOUND", buff, bufc);
		return;
	}

	for (p = fargs[0]; *p; p++)
	{
		*p = tolower(*p);
	}

	cmdp = (CMDENT *)hashfind(fargs[0], &mudstate.command_htab);

	if (!cmdp || (cmdp->info.handler != do_help))
	{
		SAFE_LB_STR((char *)"#-1 NOT FOUND", buff, bufc);
		return;
	}

	if (!check_cmd_access(player, cmdp, cargs, ncargs))
	{
		SAFE_NOPERM(buff, bufc);
		return;
	}

	help_helper(player, (cmdp->extra & ~HELP_RAWHELP), (cmdp->extra & HELP_RAWHELP) ? 0 : 1, fargs[1], buff, bufc);
}

/*---------------------------------------------------------------------------
 * Pueblo HTML-related functions.
 */

void fun_html_escape(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	html_escape(fargs[0], buff, bufc);
}

void fun_html_unescape(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	const char *msg_orig;
	int ret = 0;

	for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++)
	{
		switch (*msg_orig)
		{
		case '&':
			if (!strncmp(msg_orig, "&quot;", 6))
			{
				ret = SAFE_LB_CHR('\"', buff, bufc);
				msg_orig += 5;
			}
			else if (!strncmp(msg_orig, "&lt;", 4))
			{
				ret = SAFE_LB_CHR('<', buff, bufc);
				msg_orig += 3;
			}
			else if (!strncmp(msg_orig, "&gt;", 4))
			{
				ret = SAFE_LB_CHR('>', buff, bufc);
				msg_orig += 3;
			}
			else if (!strncmp(msg_orig, "&amp;", 5))
			{
				ret = SAFE_LB_CHR('&', buff, bufc);
				msg_orig += 4;
			}
			else
			{
				ret = SAFE_LB_CHR('&', buff, bufc);
			}

			break;

		default:
			ret = SAFE_LB_CHR(*msg_orig, buff, bufc);
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
bool escaped_chars(unsigned char ch) {
	switch(ch) {
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

void fun_url_escape(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{

	//char *escaped_chars = "<>#%{}|\\^~[]';/?:@=&\"+";
	const char *msg_orig;
	int ret = 0;
	char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");

	for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++)
	{
		if (escaped_chars(*msg_orig))
		{
			XSPRINTF(tbuf, "%%%2x", *msg_orig);
			ret = SAFE_LB_STR(tbuf, buff, bufc);
		}
		else if (*msg_orig == ' ')
		{
			ret = SAFE_LB_CHR('+', buff, bufc);
		}
		else
		{
			ret = SAFE_LB_CHR(*msg_orig, buff, bufc);
		}
	}
	XFREE(tbuf);
}

void fun_url_unescape(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	const char *msg_orig;
	int ret = 0;
	unsigned int tempchar;
	char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");;

	for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret;)
	{
		switch (*msg_orig)
		{
		case '+':
			ret = SAFE_LB_CHR(' ', buff, bufc);
			msg_orig++;
			break;

		case '%':
			XSTRNCPY(tbuf, msg_orig + 1, 2);
			tbuf[2] = '\0';

			if ((sscanf(tbuf, "%x", &tempchar) == 1) && (tempchar > 0x1F) && (tempchar < 0x7F))
			{
				ret = SAFE_LB_CHR((char)tempchar, buff, bufc);
			}

			if (*msg_orig)
			{
				msg_orig++; /* Skip the '%' */
			}

			if (*msg_orig)
			{ /* Skip the 1st hex character. */
				msg_orig++;
			}

			if (*msg_orig)
			{ /* Skip the 2nd hex character. */
				msg_orig++;
			}

			break;

		default:
			ret = SAFE_LB_CHR(*msg_orig, buff, bufc);
			msg_orig++;
			break;
		}
	}

	XFREE(tbuf);
	return;
}
