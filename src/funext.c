/* funext.c - Functions that rely on external call-outs */
/* $Id: funext.c,v 1.22 2007/07/14 02:02:04 lwl Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "functions.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "command.h"		/* required by code */

extern void FDECL(make_ulist, (dbref, char *, char **));

extern void FDECL(make_portlist, (dbref, dbref, char *, char **));

extern int FDECL(fetch_idle, (dbref, int));

extern int FDECL(fetch_connect, (dbref, int));

extern char *FDECL(get_doing, (dbref, int));

extern void FDECL(make_sessioninfo, (dbref, dbref, int, char *, char **));

extern dbref FDECL(get_programmer, (dbref));

extern void FDECL(cf_display, (dbref, char *, char *, char **));

extern void FDECL(help_helper, (dbref, int, int, char *, char *, char **));

extern INLINE int FDECL(safe_chr_real_fn, (char, char *, char **, int));

#define Find_Connection(x,s,t,p) \
	p = t = NOTHING;	\
	if (is_integer(s)) {	\
		p = atoi(s);	\
	} else {	\
		t = lookup_player(x, s, 1);	\
		if (Good_obj(t) && Can_Hide(t) && Hidden(t) && \
		    !See_Hidden(x))	\
			t = NOTHING;	\
	}

/*
 * ---------------------------------------------------------------------------
 * config: Display a MUSH config parameter.
 */

FUNCTION(fun_config)
{
    cf_display(player, fargs[0], buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_lwho: Return list of connected users.
 */

FUNCTION(fun_lwho)
{
    make_ulist(player, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_ports: Returns a list of ports for a user.
 */

FUNCTION(fun_ports)
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

FUNCTION(fun_doing)
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
        safe_str(str, buff, bufc);
    }
}

/*
 * ---------------------------------------------------------------------------
 * handle_conninfo: return seconds idle or connected (IDLE, CONN).
 */

FUNCTION(handle_conninfo)
{
    dbref target;

    int port;

    Find_Connection(player, fargs[0], target, port);
    if ((port < 0) && (target == NOTHING))
    {
        safe_known_str((char *)"-1", 2, buff, bufc);
        return;
    }
    safe_ltos(buff, bufc, Is_Func(CONNINFO_IDLE) ?
              fetch_idle(target, port) : fetch_connect(target, port));
}

/*
 * ---------------------------------------------------------------------------
 * fun_session: Return session info about a port.
 */

FUNCTION(fun_session)
{
    dbref target;

    int port;

    Find_Connection(player, fargs[0], target, port);
    if ((port < 0) && (target == NOTHING))
    {
        safe_str((char *)"-1 -1 -1", buff, bufc);
        return;
    }
    make_sessioninfo(player, target, port, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_programmer: Returns the dbref or #1- of an object in a @program.
 */

FUNCTION(fun_programmer)
{
    dbref target;

    target = lookup_player(player, fargs[0], 1);
    if (!Good_obj(target) || !Connected(target) ||
            !Examinable(player, target))
    {
        safe_nothing(buff, bufc);
        return;
    }
    safe_dbref(buff, bufc, get_programmer(target));
}

/*---------------------------------------------------------------------------
 * fun_helptext: Read an entry from a helpfile.
 */

FUNCTION(fun_helptext)
{
    CMDENT *cmdp;

    char *p;

    if (!fargs[0] || !*fargs[0])
    {
        safe_str((char *)"#-1 NOT FOUND", buff, bufc);
        return;
    }
    for (p = fargs[0]; *p; p++)
        *p = tolower(*p);

    cmdp = (CMDENT *) hashfind(fargs[0], &mudstate.command_htab);
    if (!cmdp || (cmdp->info.handler != do_help))
    {
        safe_str((char *)"#-1 NOT FOUND", buff, bufc);
        return;
    }
    if (!Check_Cmd_Access(player, cmdp, cargs, ncargs))
    {
        safe_noperm(buff, bufc);
        return;
    }
    help_helper(player, (cmdp->extra & ~HELP_RAWHELP),
                (cmdp->extra & HELP_RAWHELP) ? 0 : 1, fargs[1], buff, bufc);
}

/*---------------------------------------------------------------------------
 * Pueblo HTML-related functions.
 */

#ifdef PUEBLO_SUPPORT

FUNCTION(fun_html_escape)
{
    html_escape(fargs[0], buff, bufc);
}

FUNCTION(fun_html_unescape)
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
                ret = safe_chr_fn('\"', buff, bufc);
                msg_orig += 5;
            }
            else if (!strncmp(msg_orig, "&lt;", 4))
            {
                ret = safe_chr_fn('<', buff, bufc);
                msg_orig += 3;
            }
            else if (!strncmp(msg_orig, "&gt;", 4))
            {
                ret = safe_chr_fn('>', buff, bufc);
                msg_orig += 3;
            }
            else if (!strncmp(msg_orig, "&amp;", 5))
            {
                ret = safe_chr_fn('&', buff, bufc);
                msg_orig += 4;
            }
            else
            {
                ret = safe_chr_fn('&', buff, bufc);
            }
            break;
        default:
            ret = safe_chr_fn(*msg_orig, buff, bufc);
            break;
        }
    }
}

FUNCTION(fun_url_escape)
{
    /*
     * These are the characters which are converted to %<hex>
     */
    char *escaped_chars = "<>#%{}|\\^~[]';/?:@=&\"+";

    const char *msg_orig;

    int ret = 0;

    char tbuf[10];

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret; msg_orig++)
    {
        if (strchr(escaped_chars, *msg_orig))
        {
            sprintf(tbuf, "%%%2x", *msg_orig);
            ret = safe_str_fn(tbuf, buff, bufc);
        }
        else if (*msg_orig == ' ')
        {
            ret = safe_chr_fn('+', buff, bufc);
        }
        else
        {
            ret = safe_chr_fn(*msg_orig, buff, bufc);
        }
    }
}

FUNCTION(fun_url_unescape)
{
    const char *msg_orig;

    int ret = 0;

    unsigned int tempchar;

    char tempstr[10];

    for (msg_orig = fargs[0]; msg_orig && *msg_orig && !ret;)
    {
        switch (*msg_orig)
        {
        case '+':
            ret = safe_chr_fn(' ', buff, bufc);
            msg_orig++;
            break;
        case '%':
            strncpy(tempstr, msg_orig + 1, 2);
            tempstr[2] = '\0';
            if ((sscanf(tempstr, "%x", &tempchar) == 1) &&
                    (tempchar > 0x1F) && (tempchar < 0x7F))
            {
                ret = safe_chr_fn((char)tempchar, buff, bufc);
            }
            if (*msg_orig)
                msg_orig++;	/* Skip the '%' */
            if (*msg_orig)	/* Skip the 1st hex character. */
                msg_orig++;
            if (*msg_orig)	/* Skip the 2nd hex character. */
                msg_orig++;
            break;
        default:
            ret = safe_chr_fn(*msg_orig, buff, bufc);
            msg_orig++;
            break;
        }
    }
    return;
}

#endif				/* PUEBLO_SUPPORT */
