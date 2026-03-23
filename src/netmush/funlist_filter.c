/**
 * @file funlist_filter.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List filter functions: element selection and exclusion
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
 * @brief Copy delimiter structure (osep = isep)
 *
 * @param dest Destination Delim structure
 * @param src Source Delim structure
 *
 * @details Safely copies the delimiter information from src to dest,
 * handling the variable-length string field.
 */
static inline void copy_delim(Delim *dest, const Delim *src)
{
	if (!dest || !src)
	{
		return;
	}

	size_t copy_len = sizeof(Delim) - MAX_DELIM_LEN + 1 + src->len;
	XMEMCPY(dest, src, copy_len);
}


/**
 * @brief given a list of numbers, get corresponding elements from the list.
 *
 * elements(ack bar eep foof yay,2 4) ==> bar foof The function takes
 * a separator, but the separator only applies to the first list.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_elements(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nwords, cur, start, end, stepn;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	char *end_p, *step_p;
	Delim isep, osep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
	{
		return;
	}

	if (nfargs < 4)
	{
		copy_delim(&osep, &isep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	oldp = *bufc;

	/**
	 * Turn the first list into an array.
	 *
	 */
	wordlist = XMALLOC(LBUF_SIZE, "wordlist");
	XSTRCPY(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);
	s = Eat_Spaces(fargs[1]);

	/**
	 * Go through the second list, grabbing the numbers and finding the
	 * corresponding elements.
	 *
	 */
	do
	{
		r = split_token(&s, &SPACE_DELIM);

		if ((end_p = strchr(r, ':')) == NULL)
		{
			/**
			 * Just a number. If negative, count back from end of
			 * list.
			 *
			 */
			cur = (int)strtol(r, (char **)NULL, 10);

			if (cur < 0)
			{
				cur += nwords;
			}
			else
			{
				cur--;
			}

			if ((cur >= 0) && (cur < nwords) && ptrs[cur])
			{
				if (oldp != *bufc)
				{
					print_separator(&osep, buff, bufc);
				}

				XSAFELBSTR(ptrs[cur], buff, bufc);
			}
		}
		else
		{
			/**
			 * Support Python-style slicing syntax:
			 * &lt;start&gt;:&lt;end&gt;:&lt;step&gt; If start is empty, start from
			 * element 0. If start is positive, start from that
			 * number. If start is negative, start from that
			 * number back from the end (-1 is the last item, -2
			 * is second to last, etc.) If end is empty, stop at
			 * the last element. If end is positive, stop there.
			 * If end is negative, skip the last end elements.
			 * Note that Python numbers arrays from 0, and we
			 * number word lists from 1, so the syntax isn't
			 * Python-identical!
			 */

			/**
			 * r points to our start
			 *
			 */
			*end_p++ = '\0';

			if ((step_p = strchr(end_p, ':')) != NULL)
			{
				*step_p++ = '\0';
			}

			if (!step_p)
			{
				stepn = 1;
			}
			else
			{
				stepn = (int)strtol(step_p, (char **)NULL, 10);
			}

			if (stepn > 0)
			{
				if (*r == '\0')
				{
					/**
					 * Empty start
					 *
					 */
					start = 0;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/**
					 * Empty end
					 *
					 */
					end = nwords;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur) : (cur);
				}

				if (start <= end)
				{
					for (cur = start; cur < end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords) && ptrs[cur])
						{
							if (oldp != *bufc)
							{
								print_separator(&osep, buff, bufc);
							}

							XSAFELBSTR(ptrs[cur], buff, bufc);
						}
					}
				}
			}
			else if (stepn < 0)
			{
				if (*r == '\0')
				{
					/**
					 * Empty start, goes to the LAST element
					 *
					 */
					start = nwords - 1;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/**
					 * Empty end
					 *
					 */
					end = 0;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur - 1) : (cur - 1);
				}

				if (start >= end)
				{
					for (cur = start; cur >= end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords) && ptrs[cur])
						{
							if (oldp != *bufc)
							{
								print_separator(&osep, buff, bufc);
							}

							XSAFELBSTR(ptrs[cur], buff, bufc);
						}
					}
				}
			}
		}
	} while (s);

	XFREE(wordlist);
	XFREE(ptrs);
}

/**
 * @brief Return the elements of a list EXCEPT the numbered items.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_exclude(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nwords, cur, start, end, stepn;
	char **ptrs;
	char *wordlist, *s, *r, *oldp;
	char *end_p, *step_p;
	int *mapper;
	Delim isep, osep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
	{
		return;
	}

	if (nfargs < 4)
	{
		copy_delim(&osep, &isep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	oldp = *bufc;

	/**
	 * Turn the first list into an array.
	 *
	 */
	wordlist = XMALLOC(LBUF_SIZE, "wordlist");
	XSTRCPY(wordlist, fargs[0]);
	nwords = list2arr(&ptrs, LBUF_SIZE / 2, wordlist, &isep);
	s = Eat_Spaces(fargs[1]);

	/**
	 * Go through the second list, grabbing the numbers and mapping the
	 * corresponding elements.
	 *
	 */
	mapper = (int *)XCALLOC(nwords, sizeof(int), "mapper");

	do
	{
		r = split_token(&s, &SPACE_DELIM);

		if ((end_p = strchr(r, ':')) == NULL)
		{
			/**
			 * Just a number. If negative, count back from end of list.
			 *
			 */
			cur = (int)strtol(r, (char **)NULL, 10);

			if (cur < 0)
			{
				cur += nwords;
			}
			else
			{
				cur--;
			}

			if ((cur >= 0) && (cur < nwords))
			{
				mapper[cur] = 1;
			}
		}
		else
		{
			/**
			 * Slicing syntax
			 *
			 */

			/**
			 * r points to our start
			 *
			 */
			*end_p++ = '\0';

			if ((step_p = strchr(end_p, ':')) != NULL)
			{
				*step_p++ = '\0';
			}

			if (!step_p)
			{
				stepn = 1;
			}
			else
			{
				stepn = (int)strtol(step_p, (char **)NULL, 10);
			}

			if (stepn > 0)
			{
				if (*r == '\0')
				{
					/**
					 * Empty start
					 *
					 */
					start = 0;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/**
					 * Empty end
					 *
					 */
					end = nwords;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur) : (cur);
				}

				if (start <= end)
				{
					for (cur = start; cur < end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords))
						{
							mapper[cur] = 1;
						}
					}
				}
			}
			else if (stepn < 0)
			{
				if (*r == '\0')
				{
					/**
					 * Empty start, goes to the LAST
					 * element
					 *
					 */
					start = nwords - 1;
				}
				else
				{
					cur = (int)strtol(r, (char **)NULL, 10);
					start = (cur < 0) ? (nwords + cur) : (cur - 1);
				}

				if (*end_p == '\0')
				{
					/**
					 * Empty end
					 *
					 */
					end = 0;
				}
				else
				{
					cur = (int)strtol(end_p, (char **)NULL, 10);
					end = (cur < 0) ? (nwords + cur - 1) : (cur - 1);
				}

				if (start >= end)
				{
					for (cur = start; cur >= end; cur += stepn)
					{
						if ((cur >= 0) && (cur < nwords))
						{
							mapper[cur] = 1;
						}
					}
				}
			}
		}
	} while (s);

	for (cur = 0; cur < nwords; cur++)
	{
		if (!mapper[cur])
		{
			if (oldp != *bufc)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELBSTR(ptrs[cur], buff, bufc);
		}
	}

	XFREE(wordlist);
	XFREE(ptrs);
	XFREE(mapper);
}
