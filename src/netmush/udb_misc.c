/**
 * @file udb_misc.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Misc support routines for unter style error management.
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @todo Convert to standard logging functions
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <errno.h>
#include <string.h>

/*
 * Log database errors
 */

void log_db_err(int obj, int attr, const char *txt)
{
	if (!mushstate.standalone)
	{
		if (attr != NOTHING)
		{
			log_write(LOG_ALWAYS, "DBM", "ERROR", "Could not %s object #%d attr #%d", txt, obj, attr);
		}
		else
		{
			log_write(LOG_ALWAYS, "DBM", "ERROR", "Could not %s object #%d", txt, obj);
		}
	}
	else
	{
		log_write_raw(1, "Could not %s object #%d", txt, obj);

		if (attr != NOTHING)
		{
			log_write_raw(1, " attr #%d", attr);
		}

		log_write_raw(1, "\n");
	}
}

/*
 * print a series of warnings - do not exit
 */
/*
 * VARARGS
 */
void warning(const char *p, ...)
{
	va_list ap;
	va_start(ap, p);

	while (1)
	{
		if (p == (const char *)0)
		{
			break;
		}

		if (p == (const char *)-1)
		{
			p = (const char *)safe_strerror(errno);
		}

		log_write_raw(1, "%s", p);
		p = va_arg(ap, const char *);
	}

	va_end(ap);
	log_write_raw(1, "\n");
}

/*
 * print a series of warnings - exit
 */
/*
 * VARARGS
 */
void fatal(const char *p, ...)
{
	va_list ap;
	va_start(ap, p);

	while (1)
	{
		if (p == (const char *)0)
		{
			break;
		}

		if (p == (const char *)-1)
		{
			static char errbuf[256];
			strerror_r(errno, errbuf, sizeof(errbuf));
			p = errbuf;
		}

		log_write_raw(1, "%s", p);
		p = va_arg(ap, const char *);
	}

	va_end(ap);
	log_write_raw(1, "\n");
	exit(EXIT_FAILURE);
}
