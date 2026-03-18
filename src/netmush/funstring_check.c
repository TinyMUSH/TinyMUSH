/**
 * @file funstring_check.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief String validation built-ins.
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

/**
 * @brief Is every character in the argument a letter?
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
void fun_isword(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = NULL;

	for (p = fargs[0]; *p; p++)
	{
		if (!isalpha(*p))
		{
			XSAFELBCHR('0', buff, bufc);
			return;
		}
	}

	XSAFELBCHR('1', buff, bufc);
}

/**
 * @brief isalnum: is every character in the argument a letter or number?
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
void fun_isalnum(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = NULL;

	for (p = fargs[0]; *p; p++)
	{
		if (!isalnum(*p))
		{
			XSAFELBCHR('0', buff, bufc);
			return;
		}
	}

	XSAFELBCHR('1', buff, bufc);
}

/**
 * @brief Is the argument a number?
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
void fun_isnum(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	XSAFELBCHR((is_number(fargs[0]) ? '1' : '0'), buff, bufc);
}

/**
 * @brief Is the argument a valid dbref?
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
void fun_isdbref(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = fargs[0];
	dbref dbitem = NOTHING;

	if (*p++ == NUMBER_TOKEN)
	{
		if (*p)
		{
			/**
			 * just the string '#' won't do!
			 *
			 */
			dbitem = parse_dbref_only(p);

			if (Good_obj(dbitem))
			{
				XSAFELBCHR('1', buff, bufc);
				return;
			}
		}
	}

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Is the argument a valid objid?
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
void fun_isobjid(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = fargs[0];
	dbref dbitem = NOTHING;

	if (*p++ == NUMBER_TOKEN)
	{
		if (*p)
		{
			/**
			 * just the string '#' won't do!
			 *
			 */
			dbitem = parse_objid(p, NULL);

			if (Good_obj(dbitem))
			{
				XSAFELBCHR('1', buff, bufc);
				return;
			}
		}
	}

	XSAFELBCHR('0', buff, bufc);
}
