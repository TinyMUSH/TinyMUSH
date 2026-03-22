/**
 * @file funlist_utils.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List utility functions: type detection, argument validation, and helpers
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

/**
 * List management utilities.
 *
 */

/**
 * @brief Auto-Detect what kind of list we are dealing with
 *
 * @param ptrs List to check
 * @param nitems Number of items
 * @return int List type (NUMERIC_LIST, FLOAT_LIST, DBREF_LIST, or ALPHANUM_LIST)
 *
 * @details Analyzes the list items to determine the appropriate sort type.
 * Starts by assuming numeric, then checks for floats, DBrefs, or falls back
 * to alphanumeric. DBref detection is based on the first item starting with '#'
 * followed by digits.
 */
int autodetect_list(char *ptrs[], int nitems)
{
	int sort_type, i;
	char *p;
	sort_type = NUMERIC_LIST;

	for (i = 0; i < nitems; i++)
	{
		switch (sort_type)
		{
		case NUMERIC_LIST:
			if (!is_number(ptrs[i]))
			{
				/**
				 * If non-numeric, switch to alphanum sort.
				 * Exception: if this is the first element
				 * and it is a good dbref, switch to a dbref
				 * sort. We're a little looser than the
				 * normal 'good dbref' rules, any number
				 * following the #-sign is  accepted.
				 */
				if (i == 0)
				{
					p = ptrs[i];

					if (*p++ != NUMBER_TOKEN)
					{
						return ALPHANUM_LIST;
					}
					else if (is_integer(p))
					{
						sort_type = DBREF_LIST;
					}
					else
					{
						return ALPHANUM_LIST;
					}
				}
				else
				{
					return ALPHANUM_LIST;
				}
			}
			else if (strchr(ptrs[i], '.'))
			{
				sort_type = FLOAT_LIST;
			}

			break;

		case FLOAT_LIST:
			if (!is_number(ptrs[i]))
			{
				sort_type = ALPHANUM_LIST;
				return ALPHANUM_LIST;
			}

			break;

		case DBREF_LIST:
			p = ptrs[i];

			if (*p++ != NUMBER_TOKEN)
			{
				return ALPHANUM_LIST;
			}

			if (!is_integer(p))
			{
				return ALPHANUM_LIST;
			}

			break;

		default:
			return ALPHANUM_LIST;
		}
	}

	return sort_type;
}

/**
 * @brief Detect the list type
 *
 * @param fargs Function Arguments
 * @param nfargs Number of arguments
 * @param type_pos Which argument hold the list type
 * @param ptrs List
 * @param nitems Number of items in the list
 * @return int List type (NUMERIC_LIST, FLOAT_LIST, DBREF_LIST, ALPHANUM_LIST, or NOCASE_LIST)
 *
 * @details If type_pos is within bounds, uses the specified type from fargs[type_pos-1]:
 * 'd' for DBREF_LIST, 'n' for NUMERIC_LIST, 'f' for FLOAT_LIST, 'i' for NOCASE_LIST.
 * Otherwise, auto-detects based on the list content.
 */
int get_list_type(char *fargs[], int nfargs, int type_pos, char *ptrs[], int nitems)
{
	if (nfargs >= type_pos)
	{
		switch (tolower(*fargs[type_pos - 1]))
		{
		case 'd':
			return DBREF_LIST;

		case 'n':
			return NUMERIC_LIST;

		case 'f':
			return FLOAT_LIST;

		case 'i':
			return NOCASE_LIST;

		case '\0':
			return autodetect_list(ptrs, nitems);

		default:
			return ALPHANUM_LIST;
		}
	}

	return autodetect_list(ptrs, nitems);
}

/**
 * @brief Validate function arguments and delimiter for list functions
 * @param func_name Name of the function (for error messages)
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Command's argument count
 * @param min_args Minimum number of arguments
 * @param max_args Maximum number of arguments
 * @param delim_pos Position of delimiter argument (-1 if none, or uses 1-based index)
 * @param delim_flags Delimiter flags (DELIM_STRING, etc.)
 * @param isep Pointer to Delim structure to populate
 * @return int 1 if valid, 0 if error (already printed to buff)
 */
int validate_list_args(const char *func_name, char *buff, char **bufc, dbref player, dbref caller, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs, int min_args, int max_args,
					   int delim_pos, int delim_flags, Delim *isep)
{
	if (!fn_range_check(func_name, nfargs, min_args, max_args, buff, bufc))
	{
		return 0;
	}

	if (delim_pos > 0 && !delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, delim_pos, isep, delim_flags))
	{
		return 0;
	}

	return 1;
}

/**
 * @brief Convert a DBref (#db) to its numerical value (db)
 *
 * @param dbr Text DBref value
 * @return int DBref numerical value
 */
int dbnum(char *dbr)
{
	if ((*dbr != '#') || (dbr[1] == '\0'))
	{
		return 0;
	}
	else
	{
		return (int)strtol(dbr + 1, (char **)NULL, 10);
	}
}
