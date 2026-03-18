/**
 * @file funlist_sets.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List set operations
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


/*
 * Set management: SETUNION, SETDIFF, SETINTER. Also LUNION,
 * LDIFF, LINTER: Same thing, but takes a sort type like sort() does. There's
 * an unavoidable PennMUSH conflict, as setunion() and friends have a 4th-arg
 * output delimiter in TM3, but the 4th arg is used for the sort type in
 * PennMUSH. Also, adding the sort type as a fifth arg for setunion(), etc.
 * would be confusing, since the last two args are, by convention,
 * delimiters. So we add new funcs.
 *
 * Parameters: buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs
 */

/**
 * @brief Compare two items by sort type
 * @param s1 First item string
 * @param s2 Second item string
 * @param sort_type Sort type (ALPHANUM_LIST, NOCASE_LIST, FLOAT_LIST, INT_LIST)
 * @param ip1 Pointer to first integer value (for INT_LIST)
 * @param ip2 Pointer to second integer value (for INT_LIST)
 * @param fp1 Pointer to first float value (for FLOAT_LIST)
 * @param fp2 Pointer to second float value (for FLOAT_LIST)
 * @return Comparison result: &lt;0, 0, or &gt;0
 */
static int compare_items(const char *s1, const char *s2, int sort_type, int *ip1, int *ip2, double *fp1, double *fp2)
{
	if (sort_type == ALPHANUM_LIST)
	{
		return strcmp(s1, s2);
	}
	else if (sort_type == NOCASE_LIST)
	{
		return strcasecmp(s1, s2);
	}
	else if (sort_type == FLOAT_LIST)
	{
		return (*fp1 > *fp2) ? 1 : ((*fp1 < *fp2) ? -1 : 0);
	}
	else
	{
		return (*ip1 > *ip2) ? 1 : ((*ip1 < *ip2) ? -1 : 0);
	}
}

void handle_sets(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	int oper, type_arg;
	char *list1, *list2, *bb_p;
	char oldstr[LBUF_SIZE];
	char **ptrs1, **ptrs2;
	int i1, i2, n1, n2, val, sort_type;
	int *ip1, *ip2;
	double *fp1, *fp2;
	oper = Func_Mask(SET_OPER);
	type_arg = Func_Mask(SET_TYPE);

	if (type_arg)
	{
		if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 5, 4, DELIM_STRING, &isep))
		{
			return;
		}

		if (nfargs < 5)
		{
			copy_delim(&osep, &isep);
		}
		else
		{
			if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
			{
				return;
			}
		}
	}
	else
	{
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
	}

	list1 = XMALLOC(LBUF_SIZE, "list1");
	XSTRCPY(list1, fargs[0]);
	n1 = list2arr(&ptrs1, LBUF_SIZE, list1, &isep);
	list2 = XMALLOC(LBUF_SIZE, "list2");
	XSTRCPY(list2, fargs[1]);
	n2 = list2arr(&ptrs2, LBUF_SIZE, list2, &isep);

	if (type_arg)
	{
		if (*fargs[0])
		{
			sort_type = get_list_type(fargs, nfargs, 3, ptrs1, n1);
		}
		else
		{
			sort_type = get_list_type(fargs, nfargs, 3, ptrs2, n2);
		}
	}
	else
	{
		sort_type = ALPHANUM_LIST;
	}

	do_asort(ptrs1, n1, sort_type, SORT_ITEMS);
	do_asort(ptrs2, n2, sort_type, SORT_ITEMS);

	/**
	 * This conversion is inefficient, since it's already happened once
	 * in do_asort().
	 *
	 */
	ip1 = ip2 = NULL;
	fp1 = fp2 = NULL;

	if (sort_type == NUMERIC_LIST)
	{
		ip1 = (int *)XCALLOC(n1, sizeof(int), "ip1");
		ip2 = (int *)XCALLOC(n2, sizeof(int), "ip2");

		for (val = 0; val < n1; val++)
		{
			ip1[val] = (int)strtol(ptrs1[val], (char **)NULL, 10);
		}

		for (val = 0; val < n2; val++)
		{
			ip2[val] = (int)strtol(ptrs2[val], (char **)NULL, 10);
		}
	}
	else if (sort_type == DBREF_LIST)
	{
		ip1 = (int *)XCALLOC(n1, sizeof(int), "ip1");
		ip2 = (int *)XCALLOC(n2, sizeof(int), "ip2");

		for (val = 0; val < n1; val++)
		{
			ip1[val] = dbnum(ptrs1[val]);
		}

		for (val = 0; val < n2; val++)
		{
			ip2[val] = dbnum(ptrs2[val]);
		}
	}
	else if (sort_type == FLOAT_LIST)
	{
		fp1 = (double *)XCALLOC(n1, sizeof(double), "fp1");
		fp2 = (double *)XCALLOC(n2, sizeof(double), "fp2");

		for (val = 0; val < n1; val++)
		{
			fp1[val] = strtod(ptrs1[val], (char **)NULL);
		}

		for (val = 0; val < n2; val++)
		{
			fp2[val] = strtod(ptrs2[val], (char **)NULL);
		}
	}

	i1 = i2 = 0;
	bb_p = *bufc;
	*oldstr = '\0';
	**bufc = '\0';

	switch (oper)
	{
	case SET_UNION: /*!< Copy elements common to both lists */

		/**
		 * Handle case of two identical single-element lists
		 *
		 */
		if ((n1 == 1) && (n2 == 1) && (!strcmp(ptrs1[0], ptrs2[0])))
		{
			XSAFELBSTR(ptrs1[0], buff, bufc);
			break;
		}

		/**
		 * Process until one list is empty
		 *
		 */
		while ((i1 < n1) && (i2 < n2))
		{
			/**
			 * Skip over duplicates
			 *
			 */
			if ((i1 > 0) || (i2 > 0))
			{
				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}

			/**
			 * Compare and copy
			 *
			 */
			if ((i1 < n1) && (i2 < n2))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				if (compare_items(ptrs1[i1], ptrs2[i2], sort_type, &ip1[i1], &ip2[i2], &fp1[i1], &fp2[i2]) < 0)
				{
					XSAFELBSTR(ptrs1[i1], buff, bufc);
					XSTRCPY(oldstr, ptrs1[i1]);
					i1++;
				}
				else
				{
					XSAFELBSTR(ptrs2[i2], buff, bufc);
					XSTRCPY(oldstr, ptrs2[i2]);
					i2++;
				}

				**bufc = '\0';
			}
		}

		/**
		 * Copy rest of remaining list, stripping duplicates
		 *
		 */
		for (; i1 < n1; i1++)
		{
			if (strcmp(oldstr, ptrs1[i1]))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSTRCPY(oldstr, ptrs1[i1]);
				XSAFELBSTR(ptrs1[i1], buff, bufc);
				**bufc = '\0';
			}
		}

		for (; i2 < n2; i2++)
		{
			if (strcmp(oldstr, ptrs2[i2]))
			{
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSTRCPY(oldstr, ptrs2[i2]);
				XSAFELBSTR(ptrs2[i2], buff, bufc);
				**bufc = '\0';
			}
		}

		break;

	case SET_INTERSECT: /*!< Copy elements not in both lists */
		while ((i1 < n1) && (i2 < n2))
		{
			val = compare_items(ptrs1[i1], ptrs2[i2], sort_type, &ip1[i1], &ip2[i2], &fp1[i1], &fp2[i2]);

			if (!val)
			{
				/**
				 * Got a match, copy it
				 *
				 */
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSTRCPY(oldstr, ptrs1[i1]);
				XSAFELBSTR(ptrs1[i1], buff, bufc);
				i1++;
				i2++;

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}
			else if (val < 0)
			{
				i1++;
			}
			else
			{
				i2++;
			}
		}

		break;

	case SET_DIFF: /*!< Copy elements unique to list1 */
		while ((i1 < n1) && (i2 < n2))
		{
			val = compare_items(ptrs1[i1], ptrs2[i2], sort_type, &ip1[i1], &ip2[i2], &fp1[i1], &fp2[i2]);

			if (!val)
			{
				/**
				 * Got a match, increment pointers
				 *
				 */
				XSTRCPY(oldstr, ptrs1[i1]);

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}
			else if (val < 0)
			{
				/**
				 * Item in list1 not in list2, copy
				 *
				 */
				if (*bufc != bb_p)
				{
					print_separator(&osep, buff, bufc);
				}

				XSAFELBSTR(ptrs1[i1], buff, bufc);
				XSTRCPY(oldstr, ptrs1[i1]);
				i1++;

				while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
				{
					i1++;
				}
			}
			else
			{
				/**
				 * Item in list2 but not in list1, discard
				 *
				 */
				XSTRCPY(oldstr, ptrs2[i2]);
				i2++;

				while ((i2 < n2) && !strcmp(ptrs2[i2], oldstr))
				{
					i2++;
				}
			}
		}

		/**
		 * Copy remainder of list1
		 *
		 */
		while (i1 < n1)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELBSTR(ptrs1[i1], buff, bufc);
			XSTRCPY(oldstr, ptrs1[i1]);
			i1++;

			while ((i1 < n1) && !strcmp(ptrs1[i1], oldstr))
			{
				i1++;
			}
		}
	}

	if ((sort_type == NUMERIC_LIST) || (sort_type == DBREF_LIST))
	{
		XFREE(ip1);
		XFREE(ip2);
	}
	else if (sort_type == FLOAT_LIST)
	{
		XFREE(fp1);
		XFREE(fp2);
	}

	XFREE(ptrs1);
	XFREE(ptrs2);
	XFREE(list1);
	XFREE(list2);
}

