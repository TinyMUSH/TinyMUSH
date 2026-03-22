/**
 * @file funmisc.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Miscellaneous built-in functions: control flow, system info, and evaluation
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
#include <sys/time.h>

static void handle_switch_wild(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs, bool return_all)
{
	int i = 0;
	bool got_one = false;
	char *mbuff = NULL, *tbuff = NULL, *bp = NULL, *str = NULL, *save_token = NULL;

	if (nfargs < 2)
	{
		return;
	}

	mbuff = bp = XMALLOC(LBUF_SIZE, "bp");
	str = fargs[0];
	eval_expression_string(mbuff, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

	mushstate.in_switch++;
	save_token = mushstate.switch_token;

	for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2)
	{
		tbuff = bp = XMALLOC(LBUF_SIZE, "bp");
		str = fargs[i];
		eval_expression_string(tbuff, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

		if (quick_wild(tbuff, mbuff))
		{
			got_one = true;
			XFREE(tbuff);
			mushstate.switch_token = mbuff;
			str = fargs[i + 1];
			eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

			if (!return_all)
			{
				mushstate.in_switch--;
				mushstate.switch_token = save_token;
				XFREE(mbuff);
				return;
			}
			continue;
		}

		XFREE(tbuff);
	}

	if (!got_one && (i < nfargs) && fargs[i])
	{
		mushstate.switch_token = mbuff;
		str = fargs[i];
		eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	}

	XFREE(mbuff);
	mushstate.in_switch--;
	mushstate.switch_token = save_token;
}

static void handle_switch_exact(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i = 0;
	char *mbuff = NULL, *tbuff = NULL, *bp = NULL, *str = NULL;

	if (nfargs < 2)
	{
		return;
	}

	mbuff = bp = XMALLOC(LBUF_SIZE, "bp");
	str = fargs[0];
	eval_expression_string(mbuff, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

	for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2)
	{
		tbuff = bp = XMALLOC(LBUF_SIZE, "bp");
		str = fargs[i];
		eval_expression_string(tbuff, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

		if (!strcmp(tbuff, mbuff))
		{
			XFREE(tbuff);
			str = fargs[i + 1];
			eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
			XFREE(mbuff);
			return;
		}

		XFREE(tbuff);
	}

	XFREE(mbuff);

	if ((i < nfargs) && fargs[i])
	{
		str = fargs[i];
		eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	}
}

static void emit_clock_time(char *buff, char **bufc, int width, int cdays, int chours, int cmins, int csecs, bool zero_pad, bool hidezero)
{
	if (!hidezero || (cdays != 0))
	{
		XSAFESPRINTF(buff, bufc, zero_pad ? "%0*d:%0*d:%0*d:%0*d" : "%*d:%*d:%*d:%*d", width, cdays, width, chours, width, cmins, width, csecs);
		return;
	}

	if (chours != 0)
	{
		XSAFESPRINTF(buff, bufc, zero_pad ? "%0*d:%0*d:%0*d" : "%*d:%*d:%*d", width, chours, width, cmins, width, csecs);
		return;
	}

	if (cmins != 0)
	{
		XSAFESPRINTF(buff, bufc, zero_pad ? "%0*d:%0*d" : "%*d:%*d", width, cmins, width, csecs);
		return;
	}

	XSAFESPRINTF(buff, bufc, zero_pad ? "%0*d" : "%*d", width, csecs);
}

static char parse_etimefmt_flags(char **p, int *width, int *hidezero, int *hideearly, int *showsuffix, int *clockfmt, int *usecap)
{
	char *s = *p;

	*hidezero = *hideearly = *showsuffix = *clockfmt = *usecap = 0;

	*width = 0;

	for (; *s && isdigit((unsigned char)*s); s++)
	{
		*width = (*width * 10) + (*s - '0');
	}

	for (; (*s == 'z') || (*s == 'Z') || (*s == 'x') || (*s == 'X') || (*s == 'c') || (*s == 'C'); s++)
	{
		if (*s == 'z')
		{
			*hidezero = 1;
		}
		else if (*s == 'Z')
		{
			*hideearly = 1;
		}
		else if ((*s == 'x') || (*s == 'X'))
		{
			*showsuffix = 1;
		}
		else if (*s == 'c')
		{
			*clockfmt = 1;
		}
		else if (*s == 'C')
		{
			*usecap = 1;
		}
	}

	*p = s;
	return *s;
}

/**
 * @brief   The switchall() function compares &lt;str&gt; against &lt;pat1&gt;, &lt;pat2&gt;, etc.
 *          (allowing * to match any number of characters and ? to match any 1
 *          character), and returns the corresponding &lt;resN&gt; parameters (without
 *          any delimiters) for all &lt;patN&gt; patterns that match. If none match,
 *          then the default result &lt;dflt&gt; is returned. The evaluated value of
 *          &lt;str&gt; can be obtained as '\#$'. If switchall() and switch() are
 *          nested, the nest level can be obtained with '#!'.
 *
 * @note This functions expect that its arguments has not been evaluated.
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
void fun_switchall(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	handle_switch_wild(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, true);
}

/**
 * @brief   The switch() function compares &lt;str&gt; against &lt;pat1&gt;, &lt;pat2&gt;, etc (allowing
 *          to match any number of characters and ? to match any 1 character), and
 *          returns the corresponding &lt;resN&gt; parameter for the first &lt;patN&gt; pattern
 *          that matches. If none match, then the default result &lt;dflt&gt; is returned.
 *          The evaluated value of &lt;str&gt; can be obtained as '\#$'. If switch() and
 *          switchall() are nested, the nest level can be obtained with '#!'.
 *
 * @note This functions expect that its arguments has not been evaluated.
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
void fun_switch(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	handle_switch_wild(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, false);
}

/**
 * @brief   This function is similar to switch(), save that it looks for an exact
 *          match between the patterns and the string, rather than doing a 'wildcard'
 *          match (case-insensitive match with '*' and '?'), and the `#$` token
 *          replacement is not done. It performs marginally faster than switch().
 *
 * @note This functions expect that its arguments has not been evaluated.
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
void fun_case(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	handle_switch_exact(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs);
}

/**
 * @brief Handle if else cases.
 *
 * @note This functions expect that its arguments has not been evaluated.
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
void handle_ifelse(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str = NULL, *mbuff = NULL, *bp = NULL, *save_token = NULL, *tbuf = NULL;
	int n = 0, flag = Func_Flags(fargs);

	if (flag & IFELSE_DEFAULT)
	{
		if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
		{
			return;
		}
	}
	else
	{
		if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
		{
			return;
		}
	}

	mbuff = bp = XMALLOC(LBUF_SIZE, "bp");
	str = fargs[0];
	eval_expression_string(mbuff, &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);

	/**
	 * We default to bool-style, but we offer the option of the MUX-style
	 * nonzero -- it's true if it's not empty or zero.
	 *
	 */
	if (!mbuff || !*mbuff)
	{
		n = 0;
	}
	else if (flag & IFELSE_BOOL)
	{
		/**
		 * xlate() destructively modifies the string
		 *
		 */
		tbuf = XSTRDUP(mbuff, "tbuf");
		n = xlate(tbuf);
		XFREE(tbuf);
	}
	else
	{
		n = !(((int)strtol(mbuff, (char **)NULL, 10) == 0) && is_number(mbuff));
	}

	if (flag & IFELSE_FALSE)
	{
		n = !n;
	}

	if (flag & IFELSE_DEFAULT)
	{
		/*
		 * If we got our condition, return the string, otherwise
		 * return our 'else' default clause.
		 *
		 */
		if (n)
		{
			XSAFELBSTR(mbuff, buff, bufc);
		}
		else
		{
			str = fargs[1];
			eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
		}

		XFREE(mbuff);
		return;
	}

	/**
	 * Not default mode: Use our condition to execute result clause
	 *
	 */
	if (!n)
	{
		if (nfargs != 3)
		{
			XFREE(mbuff);
			return;
		}

		/**
		 * Do 'false' clause
		 *
		 */
		str = fargs[2];
	}
	else
	{
		/**
		 * Do 'true' clause
		 *
		 */
		str = fargs[1];
	}

	if (flag & IFELSE_TOKEN)
	{
		mushstate.in_switch++;
		save_token = mushstate.switch_token;
		mushstate.switch_token = mbuff;
	}

	eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	XFREE(mbuff);

	if (flag & IFELSE_TOKEN)
	{
		mushstate.in_switch--;
		mushstate.switch_token = save_token;
	}
}

/**
 * @brief Return a list of numbers.
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
void fun_lnum(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim osep;
	int bot = 0, top = 0, over = 0, i = 0;
	char *bb_p = NULL, *startp = NULL, *endp = NULL, *tbuf = NULL;
	int lnum_init = 0;
	char *lnum_buff = XMALLOC(290, "lnum_buff");

	if (nfargs == 0)
	{
		XFREE(lnum_buff);
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 3, 3, DELIM_STRING | DELIM_NULL | DELIM_CRLF, &osep))
	{
		return;
	}

	if (nfargs >= 2)
	{
		bot = (int)strtol(fargs[0], (char **)NULL, 10);
		top = (int)strtol(fargs[1], (char **)NULL, 10);
	}
	else
	{
		bot = 0;
		top = (int)strtol(fargs[0], (char **)NULL, 10);

		if (top-- < 1)
		{
			/**
			 * still want to generate if arg is 1
			 *
			 */
			XFREE(lnum_buff);
			return;
		}
	}

	/**
	 * We keep 0-100 pre-generated so we can do quick copies.
	 *
	 */
	if (!lnum_init)
	{
		XSTRCPY(lnum_buff, (char *)"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99");
		lnum_init = 1;
	}

	/**
	 * If it's an ascending sequence crossing from negative numbers into
	 * positive, get the negative numbers out of the way first.
	 *
	 */
	bb_p = *bufc;
	over = 0;

	if ((bot < 0) && (top >= 0) && (osep.len == 1) && (osep.str[0] == ' '))
	{
		while ((bot < 0) && !over)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			tbuf = ltos(bot);
			over = XSAFELBSTR(tbuf, buff, bufc);
			XFREE(tbuf);
			bot++;
		}

		if (over)
		{
			XFREE(lnum_buff);
			return;
		}
	}

	/**
	 * Copy as much out of the pre-gen as we can.
	 *
	 */
	if ((bot >= 0) && (bot < 100) && (top > bot) && (osep.len == 1) && (osep.str[0] == ' '))
	{
		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		startp = lnum_buff + (bot < 10 ? 2 * bot : (3 * bot) - 10);

		if (top >= 99)
		{
			XSAFELBSTR(startp, buff, bufc);
		}
		else
		{
			endp = lnum_buff + (top + 1 < 10 ? 2 * top + 1 : (3 * top + 1) - 10) - 1;
			*endp = '\0';
			XSAFELBSTR(startp, buff, bufc);
			*endp = ' ';
		}

		if (top < 100)
		{
			XFREE(lnum_buff);
			return;
		}
		else
		{
			bot = 100;
		}
	}

	/**
	 * Print a new list.
	 *
	 */
	if (top == bot)
	{
		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		XSAFELTOS(buff, bufc, bot, LBUF_SIZE);
		XFREE(lnum_buff);
		return;
	}
	else if (top > bot)
	{
		for (i = bot; (i <= top) && !over; i++)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			tbuf = ltos(i);
			over = XSAFELBSTR(tbuf, buff, bufc);
			XFREE(tbuf);
		}
	}
	else
	{
		for (i = bot; (i >= top) && !over; i--)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			tbuf = ltos(i);
			over = XSAFELBSTR(tbuf, buff, bufc);
			XFREE(tbuf);
		}
	}
	XFREE(lnum_buff);
}


/*
 * ---------------------------------------------------------------------------
 * fun_version:
 */

/**
 * @brief Return the MUSH version.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_version(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELBSTR(mushstate.version.versioninfo, buff, bufc);
}

/**
 * @brief Return the name of the Mush.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_mushname(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELBSTR(mushconf.mush_name, buff, bufc);
}

/**
 * @brief Return a list of modules.
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
void fun_modules(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	MODULE *mp = NULL;
	bool got_one = false;

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		if (got_one)
		{
			if (nfargs >= 1)
			{
				if (fargs[0] || *fargs[0])
				{
					XSAFELBSTR(fargs[0], buff, bufc);
				}
			}
			else
			{
				XSAFELBCHR(' ', buff, bufc);
			}
		}
		XSAFELBSTR(mp->modname, buff, bufc);
		got_one = true;
	}
}

/**
 * @brief Return 1 if a module is installed, 0 if it is not.
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
void fun_hasmodule(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	MODULE *mp = NULL;

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		if (!strcasecmp(fargs[0], mp->modname))
		{
			XSAFELBCHR('1', buff, bufc);
			return;
		}
	}

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Get max number of simultaneous connects.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_connrecord(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.record_players, LBUF_SIZE);
}

/**
 * @brief State of the function invocation counters
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_fcount(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.func_invk_ctr, LBUF_SIZE);
}

/**
 * @brief State of the function recursion counters
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_fdepth(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.func_nest_lev, LBUF_SIZE);
}

/**
 * @brief State of the command invocation counters
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ccount(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.cmd_invk_ctr, LBUF_SIZE);
}

/**
 * @brief State of the command recursion counters
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_cdepth(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.cmd_nest_lev, LBUF_SIZE);
}

/**
 * @brief Benchmark softcode.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_benchmark(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	struct timeval bt, et;
	int i = 0, times = 0;
	double min = 0.0, max = 0.0, total = 0.0, ut = 0.0;
	char *tp = NULL, *nstr = NULL, *s = NULL;
	char *ebuf = XMALLOC(LBUF_SIZE, "ebuf");
	char *tbuf = XMALLOC(LBUF_SIZE, "ebuf");

	/**
	 * Evaluate our times argument
	 *
	 */
	tp = nstr = XMALLOC(LBUF_SIZE, "nstr");
	s = fargs[1];
	eval_expression_string(nstr, &tp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &s, cargs, ncargs);
	times = (int)strtol(nstr, (char **)NULL, 10);
	XFREE(nstr);

	if (times < 1)
	{
		XSAFELBSTR("#-1 TOO FEW TIMES", buff, bufc);
		XFREE(ebuf);
		XFREE(tbuf);
		return;
	}

	if (times > mushconf.func_invk_lim)
	{
		XSAFELBSTR("#-1 TOO MANY TIMES", buff, bufc);
		XFREE(ebuf);
		XFREE(tbuf);
		return;
	}

	min = max = total = 0;

	for (i = 0; i < times; i++)
	{
		XSTRCPY(ebuf, fargs[0]);
		s = ebuf;
		tp = tbuf;

		gettimeofday(&bt, NULL);

		eval_expression_string(tbuf, &tp, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &s, cargs, ncargs);

		gettimeofday(&et, NULL);

		ut = ((et.tv_sec - bt.tv_sec) * 1000000) + (et.tv_usec - bt.tv_usec);

		if ((ut < min) || (min == 0))
		{
			min = ut;
		}

		if (ut > max)
		{
			max = ut;
		}

		total += ut;

		if ((mushstate.func_invk_ctr >= mushconf.func_invk_lim) || (Too_Much_CPU()))
		{
			/**
			 * Abort
			 *
			 */
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Limits exceeded at benchmark iteration %d.", i + 1);
			times = i + 1;
		}
	}

	XSAFESPRINTF(buff, bufc, "%.2f %.0f %.0f", total / (double)times, min, max);
	XFREE(ebuf);
	XFREE(tbuf);
}

/*
 * ---------------------------------------------------------------------------
 * fun_s: Force substitution to occur. fun_subeval: Like s(), but don't do
 * function evaluations.
 */

/**
 * @brief Force substitution to occur.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_s(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str;
	str = fargs[0];
	eval_expression_string(buff, bufc, player, caller, cause, EV_FIGNORE | EV_EVAL, &str, cargs, ncargs);
}

/**
 * @brief Like s(), but don't do function evaluations.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_subeval(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str;
	str = fargs[0];
	eval_expression_string(buff, bufc, player, caller, cause, EV_NO_LOCATION | EV_NOFCHECK | EV_FIGNORE | EV_NO_COMPRESS, &str, (char **)NULL, 0);
}
