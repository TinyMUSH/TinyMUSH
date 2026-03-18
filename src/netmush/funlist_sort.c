/**
 * @file funlist_sort.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List sort functions
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
#include <stdlib.h>

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
 * @brief qsort helper for case-sensitive string comparison.
 *
 * Casts @p s1 and @p s2 to `const char **` and returns the result of
 * `strcmp(*s1, *s2)`. Suitable for use with `qsort` over an array of `char *`
 * pointers.
 *
 * @param s1 Pointer to first string pointer
 * @param s2 Pointer to second string pointer
 * @return int Negative if `*s1` < `*s2`, zero if equal, positive if `*s1` > `*s2`
 */
int a_comp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

/**
 * @brief qsort helper for case-insensitive string comparison.
 *
 * Casts @p s1 and @p s2 to `const char **` and returns the result of
 * `strcasecmp(*s1, *s2)`. Suitable for use with `qsort` over an array of
 * `char *` pointers when ASCII case folding is desired.
 *
 * @param s1 Pointer to first string pointer
 * @param s2 Pointer to second string pointer
 * @return int Negative if `*s1` < `*s2`, zero if equal, positive if `*s1` > `*s2`
 */
int c_comp(const void *s1, const void *s2)
{
	return strcasecmp(*(char **)s1, *(char **)s2);
}

/**
 * @brief qsort helper for case-sensitive alphanumeric record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `A_RECORD` and compares their
 * `str` fields with `strcmp`. Designed for `qsort` over arrays of `A_RECORD`.
 *
 * @param s1 Pointer to first `A_RECORD`
 * @param s2 Pointer to second `A_RECORD`
 * @return int Negative if `s1->str` < `s2->str`, zero if equal, positive if `s1->str` > `s2->str`
 */
int arec_comp(const void *s1, const void *s2)
{
	return strcmp(((A_RECORD *)s1)->str, ((A_RECORD *)s2)->str);
}

/**
 * @brief qsort helper for case-insensitive alphanumeric record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `A_RECORD` and compares their
 * `str` fields with `strcasecmp` for ASCII case folding. Designed for `qsort`
 * over arrays of `A_RECORD`.
 *
 * @param s1 Pointer to first `A_RECORD`
 * @param s2 Pointer to second `A_RECORD`
 * @return int Negative if `s1->str` < `s2->str`, zero if equal, positive if `s1->str` > `s2->str`
 */
int crec_comp(const void *s1, const void *s2)
{
	return strcasecmp(((A_RECORD *)s1)->str, ((A_RECORD *)s2)->str);
}

/**
 * @brief qsort helper for floating-point record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `F_RECORD` and compares their
 * `data` fields. Returns 1 when the first value is greater, -1 when smaller,
 * and 0 when equal. Suitable for use with `qsort` over arrays of `F_RECORD`.
 *
 * @param s1 Pointer to first `F_RECORD`
 * @param s2 Pointer to second `F_RECORD`
 * @return int 1 if `s1->data` > `s2->data`, -1 if less, 0 if equal
 */
int f_comp(const void *s1, const void *s2)
{
	if (((F_RECORD *)s1)->data > ((F_RECORD *)s2)->data)
	{
		return 1;
	}

	if (((F_RECORD *)s1)->data < ((F_RECORD *)s2)->data)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief qsort helper for integer record comparison.
 *
 * Interprets @p s1 and @p s2 as pointers to `I_RECORD` and compares their
 * `data` fields as integers. Returns 1 when the first value is greater, -1
 * when smaller, and 0 when equal. Intended for use with `qsort` over arrays
 * of `I_RECORD`.
 *
 * @param s1 Pointer to first `I_RECORD`
 * @param s2 Pointer to second `I_RECORD`
 * @return int 1 if `s1->data` > `s2->data`, -1 if less, 0 if equal
 */
int i_comp(const void *s1, const void *s2)
{
	if (((I_RECORD *)s1)->data > ((I_RECORD *)s2)->data)
	{
		return 1;
	}

	if (((I_RECORD *)s1)->data < ((I_RECORD *)s2)->data)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief Sort a list of strings by the requested type and optionally return positions.
 *
 * Sorts the array @p s of length @p n using @p sort_type: ALPHANUM_LIST and
 * NOCASE_LIST apply string comparison (case-sensitive/insensitive), NUMERIC_LIST
 * parses integers, DBREF_LIST sorts by @c dbnum(), and FLOAT_LIST parses doubles.
 * When @p listpos_only is zero, @p s is sorted in place. When non-zero, a 1-based
 * position array is returned indicating each element's original index in sorted
 * order. For ALPHANUM_LIST and NOCASE_LIST the input array is left untouched when
 * @p listpos_only is non-zero; other modes still reorder @p s while also returning
 * positions. Caller must free the returned array (if non-NULL) with @c XFREE.
 *
 * @param s Array of list elements to sort (modified unless ALPHANUM/NOCASE with listpos_only set)
 * @param n Number of elements in @p s
 * @param sort_type Sort strategy selector (ALPHANUM_LIST, NOCASE_LIST, NUMERIC_LIST, DBREF_LIST, FLOAT_LIST)
 * @param listpos_only Non-zero to return original positions instead of (or in addition to) an in-place sort
 * @return int* Position array mapping sorted order to original 1-based indices, or NULL if not requested
 */
int *do_asort(char *s[], int n, int sort_type, int listpos_only)
{
	int i;
	F_RECORD *fp = NULL;
	I_RECORD *ip = NULL;
	A_RECORD *ap = NULL;
	int *poslist = NULL;

	switch (sort_type)
	{
	case ALPHANUM_LIST:
		if (!listpos_only)
		{
			qsort((void *)s, n, sizeof(char *), (int (*)(const void *, const void *))a_comp);
		}
		else
		{
			ap = (A_RECORD *)XCALLOC(n, sizeof(A_RECORD), "ap");

			for (i = 0; i < n; i++)
			{
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}

			qsort((void *)ap, n, sizeof(A_RECORD), (int (*)(const void *, const void *))arec_comp);

			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ap[i].pos;
			}

			XFREE(ap);
		}

		break;

	case NOCASE_LIST:
		if (!listpos_only)
		{
			qsort((void *)s, n, sizeof(char *), (int (*)(const void *, const void *))c_comp);
		}
		else
		{
			ap = (A_RECORD *)XCALLOC(n, sizeof(A_RECORD), "ap");

			for (i = 0; i < n; i++)
			{
				ap[i].str = s[i];
				ap[i].pos = i + 1;
			}

			qsort((void *)ap, n, sizeof(A_RECORD), (int (*)(const void *, const void *))crec_comp);

			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ap[i].pos;
			}

			XFREE(ap);
		}

		break;

	case NUMERIC_LIST:
		ip = (I_RECORD *)XCALLOC(n, sizeof(I_RECORD), "ip");

		for (i = 0; i < n; i++)
		{
			ip[i].str = s[i];
			ip[i].data = (int)strtol(s[i], (char **)NULL, 10);
			ip[i].pos = i + 1;
		}

		qsort((void *)ip, n, sizeof(I_RECORD), (int (*)(const void *, const void *))i_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = ip[i].str;
		}

		if (listpos_only)
		{
			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ip[i].pos;
			}
		}

		XFREE(ip);
		break;

	case DBREF_LIST:
		ip = (I_RECORD *)XCALLOC(n, sizeof(I_RECORD), "ip");

		for (i = 0; i < n; i++)
		{
			ip[i].str = s[i];
			ip[i].data = dbnum(s[i]);
			ip[i].pos = i + 1;
		}

		qsort((void *)ip, n, sizeof(I_RECORD), (int (*)(const void *, const void *))i_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = ip[i].str;
		}

		if (listpos_only)
		{
			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = ip[i].pos;
			}
		}

		XFREE(ip);
		break;

	case FLOAT_LIST:
		fp = (F_RECORD *)XCALLOC(n, sizeof(F_RECORD), "fp");

		for (i = 0; i < n; i++)
		{
			fp[i].str = s[i];
			fp[i].data = strtod(s[i], (char **)NULL);
			fp[i].pos = i + 1;
		}

		qsort((void *)fp, n, sizeof(F_RECORD), (int (*)(const void *, const void *))f_comp);

		for (i = 0; i < n; i++)
		{
			s[i] = fp[i].str;
		}

		if (listpos_only)
		{
			poslist = (int *)XCALLOC(n, sizeof(int), "l");
			for (i = 0; i < n; i++)
			{
				poslist[i] = fp[i].pos;
			}
		}

		XFREE(fp);
		break;
	}

	return poslist;
}

/**
 * @brief Entry point for the `sort()` family of functions.
 *
 * Validates the function arguments and delimiters, converts the input list
 * (from `fargs[0]`) into an array, determines the requested sort type via
 * `get_list_type()`, and delegates the actual sorting to `do_asort()`.
 *
 * Behavior:
 * - If the caller requested positions (via the function mask `SORT_POS`),
 *   this function emits a space-separated list of 1-based positions
 *   corresponding to the sorted order (the positions array is produced by
 *   `do_asort()` and freed after use).
 * - Otherwise, it reconstructs the sorted array back into a delimited string
 *   using the output delimiter (or the input delimiter when unspecified) and
 *   writes that to the output buffer.
 *
 * The function returns early for empty argument lists. Delimiter validation
 * and error reporting are handled by `validate_list_args()` and
 * `delim_check()`; any error messages are written into @p buff.
 *
 * @param buff Output buffer where results or error strings are written
 * @param bufc Pointer into @p buff for appending output
 * @param player DBref of player invoking the function (unused by sorting)
 * @param caller DBref of the caller (unused by sorting)
 * @param cause DBref of the cause (unused by sorting)
 * @param fargs Function arguments: at minimum [0]=list; additional args
 *              control sort type and delimiters as accepted by
 *              `get_list_type()` and the surrounding `sort()` API.
 * @param nfargs Number of function arguments in @p fargs
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void handle_sort(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int nitems, sort_type, oper, i;
	char *list, **ptrs;
	int *poslist;
	Delim isep, osep;

	/**
	 * If we are passed an empty arglist return a null string
	 *
	 */
	if (nfargs == 0)
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 4, 3, DELIM_STRING, &isep))
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

	oper = Func_Mask(SORT_POS);

	/**
	 * Convert the list to an array
	 *
	 */
	list = XMALLOC(LBUF_SIZE, "list");
	XSTRCPY(list, fargs[0]);
	nitems = list2arr(&ptrs, LBUF_SIZE / 2, list, &isep);
	sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
	poslist = do_asort(ptrs, nitems, sort_type, oper);

	if (oper == SORT_POS)
	{
		for (i = 0; i < nitems; i++)
		{
			if (i > 0)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELTOS(buff, bufc, poslist[i], LBUF_SIZE);
		}
	}
	else
	{
		arr2list(ptrs, nitems, buff, bufc, &osep);
	}

	if (poslist)
	{
		XFREE(poslist);
	}

	XFREE(list);
	XFREE(ptrs);
}

/*
 * ---------------------------------------------------------------------------
 * sortby:
 */

/**
 * @brief Invoke a user-provided comparison expression for sorting.
 *
 * Evaluates the expression in @p cbuff with @p s1 and @p s2 bound as the
 * two input parameters. The expression is executed via
 * `eval_expression_string()` and must yield a numeric result interpreted as
 * follows: negative if `s1 < s2`, zero if equal, positive if `s1 > s2`.
 *
 * Important: this function is designed to be used with the internal
 * `sane_qsort()` routine (which accepts extra context parameters) and is
 * NOT compatible with the standard library `qsort()` comparator signature.
 *
 * Resource and safety notes:
 * - The function guards against excessive resource use by checking
 *   `mushstate.func_invk_ctr`, `mushstate.func_nest_lev` and `Too_Much_CPU()`;
 *   if limits are exceeded it returns 0 to avoid further evaluation.
 * - `cbuff` is copied into a temporary buffer before evaluation; no caller
 *   ownership changes are performed by this function.
 *
 * @param s1 Pointer to the first element (passed to the user expression)
 * @param s2 Pointer to the second element (passed to the user expression)
 * @param cbuff Null-terminated user comparison expression (copied internally)
 * @param thing DBref used as evaluation context (e.g. attribute owner)
 * @param player DBref of the player performing the evaluation
 * @param cause DBref of the cause of the evaluation
 * @return int &lt;0 if `s1 &lt; s2`, 0 if equal, &gt;0 if `s1 &gt; s2` as produced by the expression
 */
int u_comp(const void *s1, const void *s2, char *cbuff, dbref thing, dbref player, dbref cause)
{
	/**
	 * Note that this function is for use in conjunction with our own
	 * sane_qsort routine, NOT with the standard library qsort!
	 *
	 */
	char *result, *tbuf, *elems[2], *bp;
	int n;

	if ((mushstate.func_invk_ctr > mushconf.func_invk_lim) || (mushstate.func_nest_lev > mushconf.func_nest_lim) || Too_Much_CPU())
	{
		return 0;
	}

	tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	elems[0] = (char *)s1;
	elems[1] = (char *)s2;
	XSTRCPY(tbuf, cbuff);
	result = bp = XMALLOC(LBUF_SIZE, "result");
	eval_expression_string(result, &bp, thing, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &tbuf, elems, 2);
	n = (int)strtol(result, (char **)NULL, 10);
	XFREE(result);
	XFREE(tbuf);
	return n;
}

/**
 * @brief Andrew Molitor's quicksort variant tolerant of non-transitive comparators.
 *
 * This routine sorts an array of pointers in-place using a randomized
 * pivot-based partitioning algorithm that does not require the comparator to
 * satisfy strict transitivity. It is intended for use when the comparison
 * function may depend on external context and therefore can produce
 * comparisons that violate a>b => b<a. The algorithm partitions elements
 * around a pivot chosen at random, swaps pointers in-place, and recursively
 * sorts partitions. Tail recursion is converted to iteration to reduce stack
 * usage.
 *
 * Comparator contract:
 * - The provided @p compare callback must have the signature
 *   `int compare(const void *a, const void *b, char *cbuff, dbref, dbref, dbref)`
 *   and should return negative if `a < b`, zero if `a == b`, positive if
 *   `a > b` (semantics are determined by the caller-provided comparator).
 * - The additional parameters `cbuff`, `thing`, `player`, and `cause` are
 *   forwarded to @p compare unchanged and may be used by user-defined
 *   comparison expressions.
 *
 * Notes and limitations:
 * - Sorting is performed by swapping `void *` pointers; element data is not
 *   copied. The caller is responsible for pointer validity for the duration
 *   of the call.
 * - The routine is not inherently thread-safe; concurrent calls must ensure
 *   the comparator and context are safe for parallel use.
 * - Average complexity is O(n log n); worst-case is O(n^2), but randomized
 *   pivot selection reduces the likelihood of adversarial worst-cases.
 *
 * @param array Array of pointers to sort (modified in-place)
 * @param left Index of the leftmost element to sort (0-based)
 * @param right Index of the rightmost element to sort (0-based)
 * @param compare Function pointer to comparator with extended context
 * @param cbuff User-supplied comparison buffer (forwarded to @p compare)
 * @param thing DBref passed to @p compare as context
 * @param player DBref passed to @p compare as context
 * @param cause DBref passed to @p compare as context
 */
void sane_qsort(void *array[], int left, int right, int (*compare)(const void *, const void *, char *, dbref, dbref, dbref), char *cbuff, dbref thing, dbref player, dbref cause)
{
	/*
	 *
	 */
	int i, last;
	void *tmp;
loop:

	if (left >= right)
	{
		return;
	}

	/**
	 * Pick something at random at swap it into the leftmost slot
	 * This is the pivot, we'll put it back in the right spot later
	 *
	 */
	i = random_range(0, (1 + (right - left)) - 1);
	tmp = array[left + i];
	array[left + i] = array[left];
	array[left] = tmp;
	last = left;

	for (i = left + 1; i <= right; i++)
	{
		/**
		 * Walk the array, looking for stuff that's less than our
		 * pivot. If it is, swap it with the next thing along
		 *
		 */
		if ((*compare)(array[i], array[left], cbuff, thing, player, cause) < 0)
		{
			last++;

			if (last == i)
			{
				continue;
			}

			tmp = array[last];
			array[last] = array[i];
			array[i] = tmp;
		}
	}

	/**
	 * Now we put the pivot back, it's now in the right spot, we never
	 * need to look at it again, trust me.
	 *
	 */
	tmp = array[last];
	array[last] = array[left];
	array[left] = tmp;

	/**
	 * At this point everything underneath the 'last' index is < the
	 * entry at 'last' and everything above it is not < it.
	 *
	 */
	if ((last - left) < (right - last))
	{
		sane_qsort(array, left, last - 1, compare, cbuff, thing, player, cause);
		left = last + 1;
		goto loop;
	}
	else
	{
		sane_qsort(array, last + 1, right, compare, cbuff, thing, player, cause);
		right = last - 1;
		goto loop;
	}
}

/**
 * @brief Sort a list using a user-supplied comparison expression.
 *
 * Sorts the list provided in @p fargs[1] using a user-defined comparison
 * expression supplied either as an attribute name in @p fargs[0] or as a
 * lambda string beginning with "#lambda/". The attribute text (or lambda
 * body) is retrieved and passed as the comparison buffer to `sane_qsort()`
 * via the `u_comp()` wrapper. The user expression will be evaluated with the
 * two elements under comparison bound as the evaluation arguments; it must
 * return a numeric result interpreted as negative if the first element is
 * less than the second, zero if equal, and positive if greater.
 *
 * Argument validation and delimiter handling are performed via
 * `validate_list_args()` and `delim_check()`; any error messages are written
 * into @p buff and the function returns early. The input list is split using
 * the input delimiter (default space) and the sorted result is written back
 * to the output buffer joined with the output delimiter (or input delimiter
 * when the output delimiter is not provided).
 *
 * Resource notes:
 * - The attribute text (or lambda body) is copied into a temporary buffer
 *   and freed before returning.
 * - Sorting uses `sane_qsort()` which swaps pointer values in-place; the
 *   elements pointed to must remain valid during the call.
 * - The comparator evaluation respects function invocation and nesting
 *   limits; excessive usage may cause the comparator to return 0.
 *
 * @param buff Output buffer where the sorted list or error is written
 * @param bufc Output buffer cursor for appending
 * @param player DBref of the player invoking the function
 * @param caller DBref of the caller (unused)
 * @param cause DBref of the cause (unused)
 * @param fargs Function arguments: [0]=attribute name or "#lambda/..." comparator, [1]=list to sort, [2]=optional input delimiter, [3]=optional output delimiter
 * @param nfargs Number of function arguments in @p fargs
 * @param cargs Command arguments (unused)
 * @param ncargs Number of command arguments (unused)
 */
void fun_sortby(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *atext, **ptrs;
	char *list = XMALLOC(LBUF_SIZE, "list");
	Delim isep, osep;
	int nptrs, aflags, alen, anum;
	dbref thing, aowner;
	ATTR *ap;

	if ((nfargs == 0) || !fargs[0] || !*fargs[0])
	{
		return;
	}

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

	if (string_prefix(fargs[0], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen(fargs[0] + 8);
		__xstrcpy(atext, fargs[0] + 8);
		atext[alen] = '\0';
		aowner = player;
		aflags = 0;
	}
	else
	{
		if (parse_attrib(player, fargs[0], &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(fargs[0]);
		}
		if (!ap)
		{
			return;
		}
		atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
		if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
		{
			XFREE(atext);
			return;
		}
	}

	XSTRCPY(list, fargs[1]);
	nptrs = list2arr(&ptrs, LBUF_SIZE / 2, list, &isep);

	if (nptrs > 1)
	{
		/**
		 * pointless to sort less than 2 elements
		 *
		 */
		sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp, atext, thing, player, cause);
	}

	arr2list(ptrs, nptrs, buff, bufc, &osep);
	XFREE(list);
	XFREE(atext);
	XFREE(ptrs);
}

