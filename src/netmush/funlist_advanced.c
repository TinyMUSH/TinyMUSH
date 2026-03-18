/**
 * @file funlist_advanced.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Advanced list operations
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

/*
 * ---------------------------------------------------------------------------
 * fun_grab:
 */

/**
 * @brief a combination of extract() and match(), sortof. We grab the
 * single element that we match.
 *
 * grab(Test:1 Ack:2 Foof:3,*:2)    => Ack:2
 * grab(Test-1+Ack-2+Foof-3,*o*,+)  => Ack:2
 *
 * fun_graball: Ditto, but like matchall() rather than match(). We grab all the
 * elements that match, and we can take an output delimiter.
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
void fun_grab(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *r, *s;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	/**
	 * Walk the wordstring, until we find the word we want.
	 *
	 */
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			XSAFELBSTR(r, buff, bufc);
			return;
		}
	} while (s);
}

/**
 * @brief Like grab with matchall() rather than match(). We grab all the
 * elements that match, and we can take an output delimiter.
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
void fun_graball(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *r, *s, *bb_p;
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

	s = trim_space_sep(fargs[0], &isep);
	bb_p = *bufc;

	do
	{
		r = split_token(&s, &isep);

		if (quick_wild(fargs[1], r))
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELBSTR(r, buff, bufc);
		}
	} while (s);
}

/**
 * @brief swaps two points to strings
 *
 * @param p First points to strings
 * @param q Second points to strings
 */
void swap(char **p, char **q)
{
	char *temp;
	temp = *p;
	*p = *q;
	*q = temp;
}

/**
 * @brief randomize order of words in a list.
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
void fun_shuffle(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **words;
	int n, i, j;
	Delim isep, osep;

	if (!nfargs || !fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, 3, 2, DELIM_STRING, &isep))
	{
		return;
	}

	if (nfargs < 3)
	{
		copy_delim(&osep, &isep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	n = list2arr(&words, LBUF_SIZE, fargs[0], &isep);

	for (i = 0; i < n; i++)
	{
		j = random_range(i, n - 1);
		swap(&words[i], &words[j]);
	}

	arr2list(words, n, buff, bufc, &osep);
	XFREE(words);
}

/**
 * @brief If a &lt;word&gt; in &lt;list of words&gt; is in &lt;old words&gt;, replace it with the
 * corresponding word from &lt;new words&gt;. This is basically a mass-edit. This
 * is an EXACT, not a case-insensitive or wildcarded, match.
 *
 * ledit(<list of words>,<old words>,<new words>[,<delim>[,<output delim>]])
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
void fun_ledit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	char *old_list, *new_list;
	int nptrs_old, nptrs_new;
	char **ptrs_old, **ptrs_new;
	char *r, *s, *bb_p;
	int i;
	int got_it;

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

	old_list = XMALLOC(LBUF_SIZE, "old_list");
	new_list = XMALLOC(LBUF_SIZE, "new_list");
	XSTRCPY(old_list, fargs[1]);
	XSTRCPY(new_list, fargs[2]);
	nptrs_old = list2arr(&ptrs_old, LBUF_SIZE / 2, old_list, &isep);
	nptrs_new = list2arr(&ptrs_new, LBUF_SIZE / 2, new_list, &isep);

	/**
	 * Iterate through the words.
	 *
	 */
	bb_p = *bufc;
	s = trim_space_sep(fargs[0], &isep);

	do
	{
		if (*bufc != bb_p)
		{
			print_separator(&osep, buff, bufc);
		}

		r = split_token(&s, &isep);

		for (i = 0, got_it = 0; i < nptrs_old; i++)
		{
			if (!strcmp(r, ptrs_old[i]))
			{
				got_it = 1;

				if ((i < nptrs_new) && *ptrs_new[i])
				{
					/**
					 * If we specify more old words than
					 * we have new words, we assume we
					 * want to just nullify.
					 *
					 */
					XSAFELBSTR(ptrs_new[i], buff, bufc);
				}

				break;
			}
		}

		if (!got_it)
		{
			XSAFELBSTR(r, buff, bufc);
		}
	} while (s);

	XFREE(old_list);
	XFREE(new_list);
	XFREE(ptrs_old);
	XFREE(ptrs_new);
}
/**
 * @brief Turn a list into a punctuated list.
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
void fun_itemize(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep, osep;
	int n_elems, i;
	char *conj_str, **elems;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 4, buff, bufc))
	{
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, DELIM_STRING))
	{
		return;
	}

	conj_str = (nfargs < 3) ? (char *)"and" : fargs[2];

	if (nfargs < 4)
	{
		osep.str[0] = ',';
		osep.len = 1;
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);

	if (n_elems == 1)
	{
		XSAFELBSTR(elems[0], buff, bufc);
	}
	else if (n_elems == 2)
	{
		XSAFELBSTR(elems[0], buff, bufc);

		if (*conj_str)
		{
			XSAFELBCHR(' ', buff, bufc);
			XSAFELBSTR(conj_str, buff, bufc);
		}

		XSAFELBCHR(' ', buff, bufc);
		XSAFELBSTR(elems[1], buff, bufc);
	}
	else
	{
		for (i = 0; i < (n_elems - 1); i++)
		{
			XSAFELBSTR(elems[i], buff, bufc);
			print_separator(&osep, buff, bufc);
			XSAFELBCHR(' ', buff, bufc);
		}

		if (*conj_str)
		{
			XSAFELBSTR(conj_str, buff, bufc);
			XSAFELBCHR(' ', buff, bufc);
		}

		XSAFELBSTR(elems[i], buff, bufc);
	}

	XFREE(elems);
}

/**
 * @brief Weighted random choice from a list.
 *
 * choose(<list of items>,<list of weights>,<input delim>)
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
void fun_choose(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim isep;
	char **elems, **weights;
	int i, num, n_elems, n_weights, *ip;
	int sum = 0;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, DELIM_STRING, &isep))
	{
		return;
	}

	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);
	n_weights = list2arr(&weights, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);

	if (n_elems != n_weights)
	{
		XSAFELBSTR("#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc);
		XFREE(elems);
		XFREE(weights);
		return;
	}

	/**
	 * Store the breakpoints, not the choose weights themselves.
	 *
	 */
	ip = (int *)XCALLOC(n_weights, sizeof(int), "ip");

	for (i = 0; i < n_weights; i++)
	{
		num = (int)strtol(weights[i], (char **)NULL, 10);

		if (num < 0)
		{
			num = 0;
		}

		if (num == 0)
		{
			ip[i] = 0;
		}
		else
		{
			sum += num;
			ip[i] = sum;
		}
	}

	num = (int)random_range(0, (sum)-1);

	for (i = 0; i < n_weights; i++)
	{
		if ((ip[i] != 0) && (num < ip[i]))
		{
			XSAFELBSTR(elems[i], buff, bufc);
			break;
		}
	}

	XFREE(ip);
	XFREE(elems);
	XFREE(weights);
}

/**
 * @brief Sort a list by numerical-size group, i.e., take every Nth
 * element. Useful for passing to a column-type function where you want the
 * list to go down rather than across, for instance.
 *
 * group(&lt;list&gt;, &lt;number of groups&gt;, &lt;idelim&gt;, &lt;odelim&gt;, &lt;gdelim&gt;)
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
void fun_group(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *bb_p, **elems;
	Delim isep, osep, gsep;
	int n_elems, n_groups, i, j;

	/**
	 * Separator checking is weird in this, since we can delimit by
	 * group, too, as well as the element delimiter. The group delimiter
	 * defaults to the output delimiter.
	 *
	 */
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 5, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
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

	if (nfargs < 5)
	{
		copy_delim(&gsep, &osep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &gsep, DELIM_NULL | DELIM_CRLF | DELIM_STRING))
		{
			return;
		}
	}

	/**
	 * Go do it, unless the group size doesn't make sense.
	 *
	 */
	n_groups = (int)strtol(fargs[1], (char **)NULL, 10);
	n_elems = list2arr(&elems, LBUF_SIZE / 2, fargs[0], &isep);

	if (n_groups < 2)
	{
		arr2list(elems, n_elems, buff, bufc, &osep);
		XFREE(elems);
		return;
	}

	if (n_groups >= n_elems)
	{
		arr2list(elems, n_elems, buff, bufc, &gsep);
		XFREE(elems);
		return;
	}

	bb_p = *bufc;

	for (i = 0; i < n_groups; i++)
	{
		for (j = 0; i + j < n_elems; j += n_groups)
		{
			if (*bufc != bb_p)
			{
				if (j == 0)
				{
					print_separator(&gsep, buff, bufc);
				}
				else
				{
					print_separator(&osep, buff, bufc);
				}
			}

			XSAFELBSTR(elems[i + j], buff, bufc);
		}
	}

	XFREE(elems);
}

/**
 * @brief Take a string such as 'this "Joe Bloggs" John' and turn it
 * into an output delim-separated list.
 *
 * tokens(&lt;string&gt;[,&lt;obj&gt;/&lt;attr&gt;][,&lt;open&gt;][,&lt;close&gt;][,&lt;sep&gt;][,&lt;osep&gt;])
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
void fun_tokens(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *t, *bb_p, *atext, *atextbuf, *objs[1], *str;
	int anum, aflags, alen;
	dbref aowner, thing;
	ATTR *ap;
	Delim omark, cmark, isep, osep;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 0, 6, buff, bufc))
	{
		return;
	}

	if (nfargs < 3)
	{
		omark.str[0] = '"';
		omark.len = 1;
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &omark, DELIM_STRING))
		{
			return;
		}
	}

	if (nfargs < 4)
	{
		copy_delim(&cmark, &omark);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &cmark, DELIM_STRING))
		{
			return;
		}
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &isep, DELIM_STRING))
	{
		return;
	}

	if (nfargs < 6)
	{
		copy_delim(&osep, &isep);
	}
	else
	{
		if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
		{
			return;
		}
	}

	if ((nfargs > 1) && fargs[1] && *fargs[1])
	{
		if (string_prefix(fargs[1], "#lambda/"))
		{
			thing = player;
			anum = NOTHING;
			ap = NULL;
			atext = XMALLOC(LBUF_SIZE, "lambda.atext");
			alen = strlen(fargs[1] + 8);
			__xstrcpy(atext, fargs[1] + 8);
			atext[alen] = '\0';
			aowner = player;
			aflags = 0;
		}
		else
		{
			if (parse_attrib(player, fargs[1], &thing, &anum, 0))
			{
				if ((anum == NOTHING) || !(Good_obj(thing)))
					ap = NULL;
				else
					ap = atr_num(anum);
			}
			else
			{
				thing = player;
				ap = atr_str(fargs[1]);
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

		atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");
	}
	else
	{
		atextbuf = NULL;
	}

	bb_p = *bufc;
	s = trim_space_sep(fargs[0], &isep);

	while (s && *s)
	{
		if ((omark.len == 1) ? (*s == omark.str[0]) : !strncmp(s, omark.str, omark.len))
		{
			/**
			 * Now we're inside quotes. Find the end quote, and
			 * copy the token inside of it. If we run off the end
			 * of the string, we ignore the literal opening
			 * marker that we've skipped.
			 *
			 */
			s += omark.len;

			if (*s)
			{
				t = split_token(&s, &cmark);
			}
			else
			{
				break;
			}
		}
		else
		{
			/**
			 * We are at a bare word. Split it off.
			 *
			 */
			t = split_token(&s, &isep);
		}

		/**
		 * Pass the token through the transformation function if we
		 * have one, or just copy it, if not.
		 *
		 */
		if (t)
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			if (!atextbuf)
			{
				XSAFELBSTR(t, buff, bufc);
			}
			else if ((mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU())
			{
				objs[0] = t;
				XMEMCPY(atextbuf, atext, alen);
				atextbuf[alen] = '\0';
				str = atextbuf;
				eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 1);
			}
		}

		/**
		 * Skip to start of next token, ignoring input separators.
		 *
		 */
		if (s && *s)
		{
			if ((isep.len == 1) && (isep.str[0] == ' '))
			{
				s = trim_space_sep(s, &isep);
			}
			else
			{
				if (isep.len == 1)
				{
					while (*s == isep.str[0])
					{
						s++;
					}
				}
				else
				{
					while (*s && !strncmp(s, isep.str, isep.len))
					{
						s += isep.len;
					}
				}
			}
		}
	}

	if (atextbuf)
	{
		XFREE(atextbuf);
	}
}
