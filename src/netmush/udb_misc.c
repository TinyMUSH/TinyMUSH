/**
 * @file udb_misc.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Support routines for the UDBM backend: error handling and maintenance helpers
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
	char *buffer = NULL;
	int size = 0;
	const char *str = NULL;
	int first = 1;

	va_start(ap, p);

	/* Calculate total size needed for concatenation */
	str = p;
	va_list ap_copy;
	va_copy(ap_copy, ap);

	while (str != (const char *)0)
	{
		if (str == (const char *)-1)
		{
			const char *err = safe_strerror(errno);
			size += strlen(err);
		}
		else
		{
			size += strlen(str);
		}
		str = va_arg(ap_copy, const char *);
	}

	va_end(ap_copy);

	/* Allocate buffer and concatenate strings */
	buffer = calloc(size + 1, 1);
	if (buffer == NULL)
	{
		/* Fallback if allocation fails */
		log_write(LOG_ALWAYS, "UDB", "ERROR", "Memory allocation failed in warning()");
		va_end(ap);
		return;
	}

	str = p;
	while (str != (const char *)0)
	{
		if (str == (const char *)-1)
		{
			const char *err = safe_strerror(errno);
			if (first)
			{
				strcpy(buffer, err);
				first = 0;
			}
			else
			{
				strcat(buffer, err);
			}
		}
		else
		{
			if (first)
			{
				strcpy(buffer, str);
				first = 0;
			}
			else
			{
				strcat(buffer, str);
			}
		}
		str = va_arg(ap, const char *);
	}

	va_end(ap);

	/* Use standard logging */
	log_write(LOG_ALWAYS, "UDB", "WARN", "%s", buffer);
	free(buffer);
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
	char *buffer = NULL;
	int size = 0;
	const char *str = NULL;
	int first = 1;

	va_start(ap, p);

	/* Calculate total size needed for concatenation */
	str = p;
	va_list ap_copy;
	va_copy(ap_copy, ap);

	while (str != (const char *)0)
	{
		if (str == (const char *)-1)
		{
			const char *err = safe_strerror(errno);
			size += strlen(err);
		}
		else
		{
			size += strlen(str);
		}
		str = va_arg(ap_copy, const char *);
	}

	va_end(ap_copy);

	/* Allocate buffer and concatenate strings */
	buffer = calloc(size + 1, 1);
	if (buffer == NULL)
	{
		/* Fallback if allocation fails - use minimal logging and exit */
		log_write(LOG_ALWAYS, "UDB", "FATAL", "Memory allocation failed in fatal(), exiting");
		va_end(ap);
		exit(EXIT_FAILURE);
	}

	str = p;
	while (str != (const char *)0)
	{
		if (str == (const char *)-1)
		{
			const char *err = safe_strerror(errno);
			if (first)
			{
				strcpy(buffer, err);
				first = 0;
			}
			else
			{
				strcat(buffer, err);
			}
		}
		else
		{
			if (first)
			{
				strcpy(buffer, str);
				first = 0;
			}
			else
			{
				strcat(buffer, str);
			}
		}
		str = va_arg(ap, const char *);
	}

	va_end(ap);

	/* Use standard logging */
	log_write(LOG_ALWAYS, "UDB", "FATAL", "%s", buffer);
	free(buffer);
	exit(EXIT_FAILURE);
}
