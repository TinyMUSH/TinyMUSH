/**
 * @file funlist_edit.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List editing functions
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
 * @brief Remove a single element from a delimited list.
 *
 * Splits @p fargs[0] on the delimiter (default space) and drops the element at
 * 1-based position @p fargs[1]. If @p pos < 1, the original list is returned.
 * If @p pos is beyond the end, the original list is returned unchanged. An
 * empty input list yields an empty result. Delimiter validation errors are
 * written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=position (1-based), [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_ldelete(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), NULL, &isep, IF_DELETE);
}

/**
 * @brief Replace a single list element at a given position.
 *
 * Splits @p fargs[0] on the delimiter (default space) and substitutes
 * @p fargs[2] for the element at 1-based position @p fargs[1]. If @p pos < 1,
 * or @p pos is beyond the end of the list, the original list is returned
 * unchanged. An empty input list yields an empty result. Delimiter validation
 * errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=position (1-based), [2]=replacement text, [3]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_replace(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 4, 4, DELIM_STRING, &isep))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), fargs[2], &isep, IF_REPLACE);
}

/**
 * @brief Insert a single element into a delimited list.
 *
 * Splits @p fargs[0] on the delimiter (default space) and inserts @p fargs[2]
 * at 1-based position @p fargs[1]. When @p pos == 1 and the input list is
 * empty, the new element becomes the list. If @p pos < 1, the original list is
 * returned unchanged. If @p pos is beyond the end, the element is appended.
 * Delimiter validation errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=position (1-based), [2]=text to insert, [3]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_insert(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 4, 4, DELIM_STRING, &isep))
	{
		return;
	}

	do_itemfuns(buff, bufc, fargs[0], (int)strtol(fargs[1], (char **)NULL, 10), fargs[2], &isep, IF_INSERT);
}

/**
 * @brief Replace multiple positions in a list in one call.
 *
 * Parses three lists: the source list @p fargs[0], replacement values
 * @p fargs[1], and 1-based positions @p fargs[2]. The input delimiter
 * (default space or @p fargs[3]) splits source and replacement lists;
 * positions are always split on space. Output is joined with @p fargs[4] if
 * provided, otherwise the input delimiter is reused. The replacement list and
 * position list must contain the same number of elements or an error string
 * is returned. Positions outside the source range are ignored. When no
 * positions are supplied, the original list is returned unchanged. Delimiter
 * validation errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=source list, [1]=replacement list, [2]=positions list, [3]=optional input delimiter, [4]=optional output delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_lreplace(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;
	Delim osep;
	char *origlist, *replist, *poslist;
	char **orig_p, **rep_p, **pos_p;
	int norig, npos, i, cpos;

	/**
	 * We're generous with the argument checking, in case the replacement
	 * list is blank, and/or the position list is blank.
	 *
	 */
	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 5, 4, DELIM_STRING, &isep))
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

	/**
	 * If there are no positions to replace, then we just return the
	 * original list.
	 *
	 */
	if ((nfargs < 3) || !fargs[2])
	{
		XSAFELBSTR(fargs[0], buff, bufc);
		return;
	}

	/**
	 * The number of elements we have in our replacement list must equal
	 * the number of elements in our position list.
	 *
	 */
	if (!fargs[1] || (countwords(fargs[1], &isep) != countwords(fargs[2], &SPACE_DELIM)))
	{
		XSAFELBSTR("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}

	/**
	 * Turn out lists into arrays for ease of manipulation.
	 *
	 */
	origlist = fargs[0];
	replist = fargs[1];
	poslist = fargs[2];
	norig = list2arr(&orig_p, LBUF_SIZE / 2, origlist, &isep);
	list2arr(&rep_p, LBUF_SIZE / 2, replist, &isep);
	npos = list2arr(&pos_p, LBUF_SIZE / 2, poslist, &SPACE_DELIM);

	/**
	 * The positions we have aren't necessarily sequential, so we can't
	 * just walk through the list. We have to replace position by
	 * position. If we get an invalid position number, just ignore it.
	 *
	 */
	for (i = 0; i < npos; i++)
	{
		cpos = (int)strtol(pos_p[i], (char **)NULL, 10);

		if ((cpos > 0) && (cpos <= norig))
		{
			orig_p[cpos - 1] = rep_p[i];
		}
	}

	arr2list(orig_p, norig, buff, bufc, &osep);
	XFREE(orig_p);
	XFREE(rep_p);
	XFREE(pos_p);
}

/**
 * @brief Remove the first occurrence of a word from a delimited list.
 *
 * Validates arguments and delimiter (default space). If the target word
 * contains the delimiter, returns the error string "#-1 CAN ONLY DELETE ONE
 * ELEMENT". Walks the list token by token, copying all elements to @p buff
 * except the first one that equals @p fargs[1]; subsequent matches are kept.
 * If the word is not found, the original list is returned. An empty input list
 * yields an empty result. Delimiter validation errors are written to @p buff
 * by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=word to remove, [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_remove(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *sp, *word, *bb_p;
	Delim isep;
	int found;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	if (((isep.len == 1) && strchr(fargs[1], isep.str[0])) || ((isep.len > 1) && strstr(fargs[1], isep.str)))
	{
		XSAFELBSTR("#-1 CAN ONLY DELETE ONE ELEMENT", buff, bufc);
		return;
	}

	s = fargs[0];
	word = fargs[1];

	/**
	 * Walk through the string copying words until (if ever) we get to
	 * one that matches the target word.
	 *
	 */
	sp = s;
	found = 0;
	bb_p = *bufc;

	while (s)
	{
		sp = split_token(&s, &isep);

		if (found || strcmp(sp, word))
		{
			if (*bufc != bb_p)
			{
				print_separator(&isep, buff, bufc);
			}

			XSAFELBSTR(sp, buff, bufc);
		}
		else
		{
			found = 1;
		}
	}
}

/**
 * @brief Return the 1-based position of a word in a delimited list.
 *
 * Validates arguments and delimiter (default space). Trims leading separators
 * then walks the list token by token. Returns the index of the first element
 * equal to @p fargs[1], or "0" if the word does not occur. Delimiter
 * validation errors are written to @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=word to find, [2]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_member(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int wcount;
	char *r, *s;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	wcount = 1;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (!strcmp(fargs[1], r))
		{
			XSAFELTOS(buff, bufc, wcount, LBUF_SIZE);
			return;
		}

		wcount++;
	} while (s);

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Reverse all elements in a delimited list.
 *
 * Validates arguments and delimiter (default space). An empty argument list
 * returns an empty result. Applies a bounds check to avoid overrunning the
 * output buffer, truncating the input string if needed. Splits the list into
 * an array with @p fargs[0] and writes elements in reverse order, reusing the
 * input delimiter between items. Delimiter validation errors are written to
 * @p buff by the argument check.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list, [1]=optional delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_revwords(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p, **elems;
	Delim isep;
	int n_elems, i;

	/**
	 * If we are passed an empty arglist return a null string
	 *
	 */
	if (nfargs == 0)
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 2, 2, DELIM_STRING, &isep))
	{
		return;
	}

	/**
	 * Nasty bounds checking
	 *
	 */
	if ((int)strlen(fargs[0]) >= LBUF_SIZE - (*bufc - buff) - 1)
	{
		*(fargs[0] + (LBUF_SIZE - (*bufc - buff) - 1)) = '\0';
	}

	/**
	 * Chop it up into an array of words and reverse them.
	 *
	 */
	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	bb_p = *bufc;

	for (i = n_elems - 1; i >= 0; i--)
	{
		if (*bufc != bb_p)
		{
			print_separator(&isep, buff, bufc);
		}

		XSAFELBSTR(elems[i], buff, bufc);
	}

	XFREE(elems);
}

/**
 * @brief Merge two lists by substituting matching elements from a second list.
 *
 * Validates arguments and delimiter (default space). Requires that @p fargs[0]
 * and @p fargs[1] have the same number of elements; otherwise returns
 * "#-1 NUMBER OF WORDS MUST BE EQUAL". If @p fargs[2] contains more than one
 * word, returns "#-1 TOO MANY WORDS". Uses @p fargs[2] as the match token: for
 * each position, if the element from list1 equals that token, the element from
 * the same position in list2 is emitted; otherwise the original element from
 * list1 is emitted. Output uses @p fargs[4] when provided, else reuses the
 * input delimiter. Delimiter validation errors are written to @p buff by the
 * argument checks.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player (unused)
 * @param caller DBref of caller (unused)
 * @param cause DBref of cause (unused)
 * @param fargs Function arguments: [0]=list1, [1]=list2, [2]=match word, [3]=optional input delimiter, [4]=optional output delimiter
 * @param nfargs Number of function arguments
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_splice(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p1, *p2, *q1, *q2, *bb_p;
	Delim isep, osep;
	int words, i;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, 5, 4, DELIM_STRING, &isep))
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

	/**
	 * length checks
	 *
	 */
	if (countwords(fargs[2], &isep) > 1)
	{
		XSAFELBSTR("#-1 TOO MANY WORDS", buff, bufc);
		return;
	}

	words = countwords(fargs[0], &isep);

	if (words != countwords(fargs[1], &isep))
	{
		XSAFELBSTR("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
		return;
	}

	/**
	 * loop through the two lists
	 *
	 */
	p1 = fargs[0];
	q1 = fargs[1];
	bb_p = *bufc;

	for (i = 0; i < words; i++)
	{
		p2 = split_token(&p1, &isep);
		q2 = split_token(&q1, &isep);

		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		if (!strcmp(p2, fargs[2]))
		{
			XSAFELBSTR(q2, buff, bufc); /*!< replace */
		}
		else
		{
			XSAFELBSTR(p2, buff, bufc); /*!< copy */
		}
	}
}

