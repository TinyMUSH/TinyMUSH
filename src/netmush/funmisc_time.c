/**
 * @file funmisc_time.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Time and date functions
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

MONTHDAYS mdtab[] = {
	{(char *)"Jan", 31},
	{(char *)"Feb", 29},
	{(char *)"Mar", 31},
	{(char *)"Apr", 30},
	{(char *)"May", 31},
	{(char *)"Jun", 30},
	{(char *)"Jul", 31},
	{(char *)"Aug", 31},
	{(char *)"Sep", 30},
	{(char *)"Oct", 31},
	{(char *)"Nov", 30},
	{(char *)"Dec", 31}};


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
 * @brief Returns nicely-formatted time.
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
void fun_time(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char temp[26];
	struct tm tm_buf;
	localtime_r(&mushstate.now, &tm_buf);
	strftime(temp, sizeof(temp), "%a %b %d %H:%M:%S %Y", &tm_buf);
	XSAFELBSTR(temp, buff, bufc);
}

/**
 * @brief Seconds since 0:00 1/1/70
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
void fun_secs(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.now, LBUF_SIZE);
}

/**
 * @brief Converts seconds to time string, based off 0:00 1/1/70
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
void fun_convsecs(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	time_t tt = strtol(fargs[0], (char **)NULL, 10);
	char temp[26];
	struct tm tm_buf;
	localtime_r(&tt, &tm_buf);
	strftime(temp, sizeof(temp), "%a %b %d %H:%M:%S %Y", &tm_buf);
	XSAFELBSTR(temp, buff, bufc);
}

/**
 * @brief Converts time string to a struct tm.
 *
 * @note Time string format is always 24 characters long,
 *       in format Ddd Mmm DD HH:MM:SS YYYY
 *
 * @param str Time string
 * @param ttm Pointer to a tm struct
 * @return true Converted
 * @return false Something went wrong
 */

bool do_convtime(char *str, struct tm *ttm)
{
	char *buf = NULL, *p = NULL, *q = NULL;
	int i = 0;

	if (!str || !ttm)
	{
		return false;
	}

	while (*str == ' ')
	{
		str++;
	}

	buf = p = XMALLOC(SBUF_SIZE, "p");
	XSAFESBSTR(str, buf, &p);
	*p = '\0';

	/**
	 * day-of-week or month
	 *
	 */
	p = strchr(buf, ' ');
	if (p)
	{
		*p++ = '\0';
		while (*p == ' ')
			p++;
	}

	if (!p || strlen(buf) != 3)
	{
		XFREE(buf);
		return false;
	}

	for (i = 0; (i < 12) && string_compare(mdtab[i].month, p); i++)
		;

	if (i == 12)
	{
		/**
		 * month
		 *
		 */
		q = strchr(p, ' ');
		if (q)
		{
			*q++ = '\0';
			while (*q == ' ')
				q++;
		}

		if (!q || strlen(p) != 3)
		{
			XFREE(buf);
			return false;
		}

		for (i = 0; (i < 12) && string_compare(mdtab[i].month, p); i++)
			;

		if (i == 12)
		{
			XFREE(buf);
			return false;
		}

		p = q;
	}

	ttm->tm_mon = i;

	/**
	 * day of month
	 *
	 */
	q = strchr(p, ' ');
	if (q)
	{
		*q++ = '\0';
		while (*q == ' ')
			q++;
	}

	if (!q || (ttm->tm_mday = (int)strtol(p, (char **)NULL, 10)) < 1 || ttm->tm_mday > mdtab[i].day)
	{
		XFREE(buf);
		return false;
	}

	/**
	 * hours
	 *
	 */
	p = strchr(q, ':');

	if (!p)
	{
		XFREE(buf);
		return false;
	}

	*p++ = '\0';

	if ((ttm->tm_hour = (int)strtol(q, (char **)NULL, 10)) > 23 || ttm->tm_hour < 0)
	{
		XFREE(buf);
		return false;
	}

	if (ttm->tm_hour == 0)
	{
		while (isspace(*q))
		{
			q++;
		}

		if (*q != '0')
		{
			XFREE(buf);
			return false;
		}
	}

	/**
	 * minutes
	 *
	 */
	q = strchr(p, ':');

	if (!q)
	{
		XFREE(buf);
		return false;
	}

	*q++ = '\0';

	if ((ttm->tm_min = (int)strtol(p, (char **)NULL, 10)) > 59 || ttm->tm_min < 0)
	{
		XFREE(buf);
		return false;
	}

	if (ttm->tm_min == 0)
	{
		while (isspace(*p))
		{
			p++;
		}

		if (*p != '0')
		{
			XFREE(buf);
			return false;
		}
	}

	/**
	 * seconds
	 *
	 */
	p = strchr(q, ' ');
	if (p)
	{
		*p++ = '\0';
		while (*p == ' ')
			p++;
	}

	if (!p || (ttm->tm_sec = (int)strtol(q, (char **)NULL, 10)) > 59 || ttm->tm_sec < 0)
	{
		XFREE(buf);
		return false;
	}

	if (ttm->tm_sec == 0)
	{
		while (isspace(*q))
		{
			q++;
		}

		if (*q != '0')
		{
			XFREE(buf);
			return false;
		}
	}

	/**
	 * year
	 *
	 */
	q = strchr(p, ' ');
	if (q)
	{
		*q++ = '\0';
		while (*q == ' ')
			q++;
	}

	if ((ttm->tm_year = (int)strtol(p, (char **)NULL, 10)) == 0)
	{
		while (isspace(*p))
		{
			p++;
		}

		if (*p != '0')
		{
			XFREE(buf);
			return false;
		}
	}

	XFREE(buf);

	if (ttm->tm_year > 100)
	{
		ttm->tm_year -= 1900;
	}

	if (ttm->tm_year < 0)
	{
		return false;
	}

	/**
	 * We don't whether or not it's daylight savings time.
	 *
	 */
	ttm->tm_isdst = -1;
	return (ttm->tm_mday != 29 || i != 1 || ((ttm->tm_year) % 400 == 100 || ((ttm->tm_year) % 100 != 0 && (ttm->tm_year) % 4 == 0))) ? true : false;
}

/**
 * @brief converts time string to seconds, based off 0:00 1/1/70
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
void fun_convtime(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	struct tm ttm;
	localtime_r(&mushstate.now, &ttm);

	if (do_convtime(fargs[0], &ttm))
	{
		XSAFELTOS(buff, bufc, mktime(&ttm), LBUF_SIZE);
	}
	else
	{
		XSAFESTRNCAT(buff, bufc, "-1", 2, LBUF_SIZE);
	}
}

/**
 * @brief Interface to strftime().
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
void fun_timefmt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	time_t tt = 0L;
	struct tm *ttm = NULL;
	char *str = XMALLOC(LBUF_SIZE, "str");
	char *tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	char *tp = NULL, *p = NULL;
	int len = 0;

	/**
	 * Check number of arguments.
	 *
	 */
	if ((nfargs < 1) || !fargs[0] || !*fargs[0])
	{
		XFREE(str);
		XFREE(tbuf);
		return;
	}

	if (nfargs == 1)
	{
		tt = mushstate.now;
	}
	else if (nfargs == 2)
	{
		tt = (time_t)strtol(fargs[1], (char **)NULL, 10);
		;

		if (tt < 0)
		{
			XSAFELBSTR("#-1 INVALID TIME", buff, bufc);
			XFREE(str);
			XFREE(tbuf);
			return;
		}
	}
	else
	{
		XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (TIMEFMT) EXPECTS 1 OR 2 ARGUMENTS BUT GOT %d", nfargs);
		XFREE(str);
		XFREE(tbuf);
		return;
	}
	/**
	 * Construct the format string. We need to convert instances of '$'
	 * into percent signs for strftime(), unless we get a '$$', which we
	 * treat as a literal '$'. Step on '$n' as invalid (output literal
	 * '%n'), because some strftime()s use it to insert a newline.
	 *
	 */
	for (tp = tbuf, p = fargs[0], len = 0; *p && (len < LBUF_SIZE - 2); tp++, p++)
	{
		if (*p == '%')
		{
			*tp++ = '%';
			*tp = '%';
		}
		else if (*p == '$')
		{
			if (*(p + 1) == '$')
			{
				*tp = '$';
				p++;
			}
			else if (*(p + 1) == 'n')
			{
				*tp++ = '%';
				*tp++ = '%';
				*tp = 'n';
				p++;
			}
			else
			{
				*tp = '%';
			}
		}
		else
		{
			*tp = *p;
		}
	}

	*tp = '\0';

	/**
	 * Get the time and format it. We do this using the local timezone.
	 *
	 */
	struct tm time_tm;
	localtime_r(&tt, &time_tm);
	strftime(str, LBUF_SIZE - 1, tbuf, &time_tm);
	XSAFELBSTR(str, buff, bufc);
	XFREE(str);
	XFREE(tbuf);
}

/**
 * @brief Format a number of seconds into a human-readable time.
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
void fun_etimefmt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = NULL, *mark = NULL, padc = 0, timec = 0;
	int raw_secs = 0, secs = 0, mins = 0, hours = 0, days = 0, csecs = 0, cmins = 0, chours = 0, cdays = 0;
	int max = 0, n = 0, x = 0, width = 0, hidezero = 0, hideearly = 0, showsuffix = 0, clockfmt = 0, usecap = 0;

	/**
	 * Figure out time values
	 *
	 */
	raw_secs = secs = (int)strtol(fargs[1], (char **)NULL, 10);

	if (secs < 0)
	{
		/**
		 * Try to be semi-useful. Keep value of secs; zero out the
		 * rest
		 *
		 */
		mins = hours = days = 0;
	}
	else
	{
		days = secs / 86400;
		secs %= 86400;
		hours = secs / 3600;
		secs %= 3600;
		mins = secs / 60;
		secs %= 60;
	}

	/**
	 * Parse and print format string
	 *
	 */
	p = fargs[0];

	while (*p)
	{
		if (*p == '$')
		{
			/**
			 * save place in case we need to go back
			 *
			 */
			mark = p;

			p++;

			if (!*p)
			{
				XSAFELBCHR('$', buff, bufc);
				break;
			}
			else if (*p == '$')
			{
				XSAFELBCHR('$', buff, bufc);
				p++;
			}
			else
			{
				char spec = parse_etimefmt_flags(&p, &width, &hidezero, &hideearly, &showsuffix, &clockfmt, &usecap);

				if (clockfmt && (raw_secs < 0))
				{
					cdays = chours = cmins = 0;
					csecs = raw_secs;
					emit_clock_time(buff, bufc, width, cdays, chours, cmins, csecs, isupper(spec), hidezero);
					p++;
					continue;
				}

				switch (spec)
				{
				case 's':
				case 'S':
					if (usecap)
					{
						n = raw_secs;
					}
					else
					{
						n = secs;
					}

					timec = 's';
					break;

				case 'm':
				case 'M':
					if (usecap)
						n = mins + (hours * 60) + (days * 24 * 60);
					else
					{
						n = mins;
					}

					timec = 'm';
					break;

				case 'h':
				case 'H':
					if (usecap)
					{
						n = hours + (days * 24);
					}
					else
					{
						n = hours;
					}

					timec = 'h';
					break;

				case 'd':
				case 'D':
					n = days;
					timec = 'd';
					break;

				case 'a':
				case 'A':
					/**
					 * Show the first non-zero thing
					 *
					 */
					if (days > 0)
					{
						n = days;
						timec = 'd';
					}
					else if (hours > 0)
					{
						n = hours;
						timec = 'h';
					}
					else if (mins > 0)
					{
						n = mins;
						timec = 'm';
					}
					else
					{
						n = secs;
						timec = 's';
					}

					break;

				default:
					timec = ' ';
				}

				if (timec == ' ')
				{
					while (*p && (*p != '$'))
					{
						p++;
					}

					XSAFESTRNCAT(buff, bufc, mark, p - mark, LBUF_SIZE);
				}
				else if (!clockfmt)
				{
					/**
					 * If it's 0 and we're hidezero, just
					 * hide it. If it's 0 and we're
					 * hideearly, we only hide it if we
					 * haven't got some bigger increment
					 * that's non-zero.
					 *
					 */
					if ((n == 0) && (hidezero || (hideearly && !(((timec == 's') && (raw_secs > 0)) || ((timec == 'm') && (raw_secs >= 60)) || ((timec == 'h') && (raw_secs >= 3600))))))
					{
						if (width > 0)
						{
							padc = isupper(spec) ? '0' : ' ';

							if (showsuffix)
							{
								x = width + 1;

								if (x > 0)
								{
									max = LBUF_SIZE - 1 - (*bufc - buff);
									x = (x > max) ? max : x;
									XMEMSET(*bufc, padc, x);
									*bufc += x;
									**bufc = '\0';
								}
							}
							else
							{
								if (width > 0)
								{
									max = LBUF_SIZE - 1 - (*bufc - buff);
									width = (width > max) ? max : width;
									XMEMSET(*bufc, padc, width);
									*bufc += width;
									**bufc = '\0';
								}
							}
						}
					}
					else if (width > 0)
					{
						if (isupper(spec))
						{
							XSAFESPRINTF(buff, bufc, "%0*d", width, n);
						}
						else
						{
							XSAFESPRINTF(buff, bufc, "%*d", width, n);
						}

						if (showsuffix)
						{
							XSAFELBCHR(timec, buff, bufc);
						}
					}
					else
					{
						XSAFELTOS(buff, bufc, n, LBUF_SIZE);

						if (showsuffix)
						{
							XSAFELBCHR(timec, buff, bufc);
						}
					}

					p++;
				}
				else
				{
					/*
					 * In clock format, we show
					 * <d>:<h>:<m>:<s>. The field
					 * specifier tells us where our
					 * division stops.
					 *
					 */
					if (timec == 'd')
					{
						cdays = days;
						chours = hours;
						cmins = mins;
						csecs = secs;
					}
					else if (timec == 'h')
					{
						cdays = 0;
						chours = hours + (days * 24);
						cmins = mins;
						csecs = secs;
					}
					else if (timec == 'm')
					{
						cdays = 0;
						chours = 0;
						cmins = mins + (hours * 60) + (days * 1440);
						csecs = secs;
					}
					else if (timec == 's')
					{
						cdays = 0;
						chours = 0;
						cmins = 0;
						csecs = raw_secs;
					}

					emit_clock_time(buff, bufc, width, cdays, chours, cmins, csecs, isupper(spec), hidezero);
					p++;
				}
			}
		}
		else
		{
			mark = p;

			while (*p && (*p != '$'))
			{
				p++;
			}

			XSAFESTRNCAT(buff, bufc, mark, p - mark, LBUF_SIZE);
		}
	}
}

/**
 * @brief What time did this system last reboot?
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
void fun_starttime(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char temp[26];
	struct tm tm_buf;
	localtime_r(&mushstate.start_time, &tm_buf);
	strftime(temp, sizeof(temp), "%a %b %d %H:%M:%S %Y", &tm_buf);
	XSAFELBSTR(temp, buff, bufc);
}

/**
 * @brief How many times have we restarted?
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
void fun_restarts(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELTOS(buff, bufc, mushstate.reboot_nums, LBUF_SIZE);
}

/**
 * @brief When did we last restart?
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
void fun_restarttime(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char temp[26];
	struct tm tm_buf;
	localtime_r(&mushstate.restart_time, &tm_buf);
	strftime(temp, sizeof(temp), "%a %b %d %H:%M:%S %Y", &tm_buf);
	XSAFELBSTR(temp, buff, bufc);
}
