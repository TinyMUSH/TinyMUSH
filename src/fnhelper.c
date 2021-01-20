/**
 * @file fnhelper.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Helper functions for MUSH functions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 * @copyright PCG Random Number Generation for C, Copyright 2014 Melissa 
 *            O'Neill <oneill@pcg-random.org>. You may distribute under 
 *            the terms of the Apache License, Version 2.0 as specified 
 *            in the COPYING file.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

/*
 * ---------------------------------------------------------------------------
 * Trim off leading and trailing spaces if the separator char is a space
 */

char *trim_space_sep(char *str, const Delim *sep)
{
	char *p;

	if ((sep->len > 1) || (sep->str[0] != ' ') || !*str)
	{
		return str;
	}

	while (*str == ' ')
	{
		str++;
	}

	for (p = str; *p; p++)
		;

	for (p--; *p == ' ' && p > str; p--)
		;

	p++;
	*p = '\0';
	return str;
}

/*
 * ---------------------------------------------------------------------------
 * Tokenizer functions. next_token: Point at start of next token in string.
 * split_token: Get next token from string as null-term string.  String is
 * destructively modified. next_token_ansi: Point at start of next token, and
 * tell what color it is.
 */

char *next_token(char *str, const Delim *sep)
{
	char *p;

	if (sep->len == 1)
	{
		while (*str == ESC_CHAR)
		{
			skip_esccode(&str);
		}

		while (*str && (*str != sep->str[0]))
		{
			++str;

			while (*str == ESC_CHAR)
			{
				skip_esccode(&str);
			}
		}

		if (!*str)
		{
			return NULL;
		}

		++str;

		if (sep->str[0] == ' ')
		{
			while (*str == ' ')
			{
				++str;
			}
		}
	}
	else
	{
		if ((p = strstr(str, sep->str)) == NULL)
		{
			return NULL;
		}

		str = p + sep->len;
	}

	return str;
}

char *split_token(char **sp, const Delim *sep)
{
	char *str, *save, *p;
	save = str = *sp;

	if (!str)
	{
		*sp = NULL;
		return NULL;
	}

	if (sep->len == 1)
	{
		while (*str == ESC_CHAR)
		{
			skip_esccode(&str);
		}

		while (*str && (*str != sep->str[0]))
		{
			++str;

			while (*str == ESC_CHAR)
			{
				skip_esccode(&str);
			}
		}

		if (*str)
		{
			*str++ = '\0';

			if (sep->str[0] == ' ')
			{
				while (*str == ' ')
				{
					++str;
				}
			}
		}
		else
		{
			str = NULL;
		}
	}
	else
	{
		if ((p = strstr(str, sep->str)) != NULL)
		{
			*p = '\0';
			str = p + sep->len;
		}
		else
		{
			str = NULL;
		}
	}

	*sp = str;
	return save;
}

char *next_token_ansi(char *str, const Delim *sep, int *ansi_state_ptr)
{
	int ansi_state = *ansi_state_ptr;
	char *p;

	if (sep->len == 1)
	{
		while (*str == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(str);
				if (*(str) == ANSI_CSI)
				{
					while ((*(++(str)) & 0xf0) == 0x30)
					{
						if (*(str) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(str)&0x0f);
						}
						else
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							param_val = 0;
						}
					}
				}
				while ((*(str)&0xf0) == 0x20)
				{
					++(str);
				}
				if (*(str) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(str);
				}
				else if (*(str))
				{
					++(str);
				}
			} while (0);
		}

		while (*str && (*str != sep->str[0]))
		{
			++str;

			while (*str == ESC_CHAR)
			{
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++(str);
					if (*(str) == ANSI_CSI)
					{
						while ((*(++(str)) & 0xf0) == 0x30)
						{
							if (*(str) < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*(str)&0x0f);
							}
							else
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								param_val = 0;
							}
						}
					}
					while ((*(str)&0xf0) == 0x20)
					{
						++(str);
					}
					if (*(str) == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
						++(str);
					}
					else if (*(str))
					{
						++(str);
					}
				} while (0);
			}
		}

		if (!*str)
		{
			*ansi_state_ptr = ansi_state;
			return NULL;
		}

		++str;

		if (sep->str[0] == ' ')
		{
			while (*str == ' ')
			{
				++str;
			}
		}
	}
	else
	{
		/*
	 * ansi tracking not supported yet in multichar delims
	 */
		if ((p = strstr(str, sep->str)) == NULL)
		{
			return NULL;
		}

		str = p + sep->len;
	}

	*ansi_state_ptr = ansi_state;
	return str;
}

/*
 * ---------------------------------------------------------------------------
 * Count the words in a delimiter-separated list.
 */

int countwords(char *str, const Delim *sep)
{
	int n;
	str = trim_space_sep(str, sep);

	if (!*str)
	{
		return 0;
	}

	for (n = 0; str; str = next_token(str, sep), n++)
		;

	return n;
}

/*
 * ---------------------------------------------------------------------------
 * list2arr, arr2list: Convert lists to arrays and vice versa.
 */

int list2arr(char ***arr, int maxtok, char *list, const Delim *sep)
{
	unsigned char *tok_starts = XMALLOC((LBUF_SIZE >> 3) + 1, "tok_starts");
	int initted = 0;
	char *tok, *liststart;
	int ntok, tokpos, i, bits;

	/*
     * Mark token starting points in a 1k bitstring, then go back
     * and collect them into an array of just the right number of
     * pointers.
     */

	if (!initted)
	{
		XMEMSET(tok_starts, 0, sizeof(tok_starts));
		initted = 1;
	}

	liststart = list = trim_space_sep(list, sep);
	tok = split_token(&list, sep);

	for (ntok = 0, tokpos = 0; tok && ntok < maxtok; ++ntok, tok = split_token(&list, sep))
	{
		tokpos = tok - liststart;
		tok_starts[tokpos >> 3] |= (1 << (tokpos & 0x7));
	}

	if (ntok == 0)
	{
		++ntok; //So we don't try to XMALLOC(0).
	}

	/*
     * Caller must free this array of pointers later. Validity of the
     * pointers is dependent upon the original list string having not
     * been freed yet.
     */
	*(arr) = (char **)XMALLOC(ntok + 1, "arr");
	tokpos >>= 3;
	ntok = 0;

	for (i = 0; i <= tokpos; ++i)
	{
		if (tok_starts[i])
		{
			/*
	     * There's at least one token starting in this byte
	     * of the bitstring, so we scan the bits.
	     */
			bits = tok_starts[i];
			tok_starts[i] = 0;
			tok = liststart + (i << 3);

			do
			{
				if (bits & 0x1)
				{
					(*arr)[ntok] = tok;
					++ntok;
				}

				bits >>= 1;
				++tok;
			} while (bits);
		}
	}

	XFREE(tok_starts);
	return ntok;
}

void print_separator(const Delim *sep, char *list, char **bufc)
{
	if (sep->len == 1)
	{
		if (sep->str[0] == '\r')
		{
			SAFE_CRLF(list, bufc);
		}
		else if (sep->str[0] != '\0')
		{
			SAFE_LB_CHR(sep->str[0], list, bufc);
		}
	}
	else
	{
		SAFE_STRNCAT(list, bufc, sep->str, sep->len, LBUF_SIZE);
	}
}

void arr2list(char **arr, int alen, char *list, char **bufc, const Delim *sep)
{
	int i;

	if (alen)
	{
		SAFE_LB_STR(arr[0], list, bufc);
	}

	for (i = 1; i < alen; i++)
	{
		print_separator(sep, list, bufc);

		SAFE_LB_STR(arr[i], list, bufc);
	}
}

/*
 * ---------------------------------------------------------------------------
 * Find the ANSI states at the beginning and end of each word of a list.
 * NOTE! Needs one more array slot than list2arr (think fenceposts) but still
 * takes the same maxlen and returns the same number of words.
 */

int list2ansi(int *arr, int *prior_state, int maxlen, char *list, const Delim *sep)
{
	int i, ansi_state;

	if (maxlen <= 0)
	{
		return 0;
	}

	ansi_state = arr[0] = *prior_state;
	list = trim_space_sep(list, sep);

	for (i = 1; list && i < maxlen; list = next_token_ansi(list, sep, &ansi_state), ++i)
	{
		arr[i - 1] = ansi_state;
	}

	arr[i] = ANST_NONE;
	return i - 1;
}

/*
 * ---------------------------------------------------------------------------
 * Quick-matching for function purposes.
 */

dbref match_thing(dbref player, char *name)
{
	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	return (noisy_match_result());
}

/*
 * ---------------------------------------------------------------------------
 * fn_range_check: Check # of args to a function with an optional argument
 * for validity.
 */

int fn_range_check(const char *fname, int nfargs, int minargs, int maxargs, char *result, char **bufc)
{
	if ((nfargs >= minargs) && (nfargs <= maxargs))
	{
		return 1;
	}

	if (maxargs == (minargs + 1))
	{
		SAFE_SPRINTF(result, bufc, "#-1 FUNCTION (%s) EXPECTS %d OR %d ARGUMENTS BUT GOT %d", fname, minargs, maxargs, nfargs);
	}
	else
	{
		SAFE_SPRINTF(result, bufc, "#-1 FUNCTION (%s) EXPECTS BETWEEN %d AND %d ARGUMENTS BUT GOT %d", fname, minargs, maxargs, nfargs);
	}

	return 0;
}

/*
 * ---------------------------------------------------------------------------
 * delim_check: obtain delimiter
 */

int delim_check(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs, int sep_arg, Delim *sep, int dflags)
{
	char *tstr, *bp, *str;
	int tlen;

	if (nfargs < sep_arg)
	{
		sep->str[0] = ' ';
		sep->len = 1;
		return 1;
	}

	tlen = strlen(fargs[sep_arg - 1]);

	if (tlen <= 1)
	{
		dflags &= ~DELIM_EVAL;
	}

	if (dflags & DELIM_EVAL)
	{
		tstr = bp = XMALLOC(LBUF_SIZE, "tstr");
		str = fargs[sep_arg - 1];
		exec(tstr, &bp, player, caller, cause, EV_EVAL | EV_FCHECK, &str, cargs, ncargs);
		tlen = strlen(tstr);
	}
	else
	{
		tstr = fargs[sep_arg - 1];
	}

	sep->len = 1;

	if (tlen == 0)
	{
		sep->str[0] = ' ';
	}
	else if (tlen == 1)
	{
		sep->str[0] = *tstr;
	}
	else
	{
		if ((dflags & DELIM_NULL) && !strcmp(tstr, (char *)NULL_DELIM_VAR))
		{
			sep->str[0] = '\0';
		}
		else if ((dflags & DELIM_CRLF) && !strcmp(tstr, (char *)"\r\n"))
		{
			sep->str[0] = '\r';
		}
		else if (dflags & DELIM_STRING)
		{
			if (tlen > MAX_DELIM_LEN)
			{
				SAFE_LB_STR("#-1 SEPARATOR TOO LONG", buff, bufc);
				sep->len = 0;
			}
			else
			{
				XSTRCPY(sep->str, tstr);
				sep->len = tlen;
			}
		}
		else
		{
			SAFE_LB_STR("#-1 SEPARATOR MUST BE ONE CHARACTER", buff, bufc);
			sep->len = 0;
		}
	}

	if (dflags & DELIM_EVAL)
	{
		XFREE(tstr);
	}

	return (sep->len);
}

/*
 * ---------------------------------------------------------------------------
 * Boolean true/false check.
 */

bool xlate(char *arg)
{
	char *temp2;

	if (arg[0] == '#')
	{
		arg++;

		if (is_integer(arg))
		{
			if (mushconf.bools_oldstyle)
			{
				switch ((int)strtol(arg, (char **)NULL, 10))
				{
				case -1:
					return false;

				case 0:
					return false;

				default:
					return true;
				}
			}
			else
			{
				return ((int)strtol(arg, (char **)NULL, 10) >= 0) ? true : false;
			}
		}

		if (mushconf.bools_oldstyle)
		{
			return false;
		}
		else
		{
			/*
	     * Case of '#-1 <string>'
	     */
			return !((arg[0] == '-') && (arg[1] == '1') && (arg[2] == ' ')) ? true : false;
		}
	}

	temp2 = Eat_Spaces(arg);

	if (!*temp2)
	{
		return false;
	}

	if (is_integer(temp2))
	{
		return (int)strtol(temp2, (char **)NULL, 10) ? true : false;
	}

	return true;
}

/*
 * ---------------------------------------------------------------------------
 * used by fun_reverse and fun_revwords to reverse things
 */

void do_reverse(char *from, char *to)
{
	char *tp;
	tp = to + strlen(from);
	*tp-- = '\0';

	while (*from)
	{
		*tp-- = *from++;
	}
}

/*
 * ---------------------------------------------------------------------------
 * Random number generator, from PennMUSH's get_random_long(), which is turn
 * based on MUX2's RandomINT32().
 */

uint32_t random_range(uint32_t low, uint32_t high)
{
	uint32_t x;
	pcg32_random_t rng1;

	/*
     * Validate parameters.
     */

	if (high < low)
	{
		return 0;
	}
	else if (high == low)
	{
		return low;
	}

	x = high - low;

	if (UINT32_MAX < x)
	{
		return -1;
	}

	x++;

	pcg32_srandom_r(&rng1, time(NULL), (intptr_t)&rng1);
	return pcg32_boundedrand_r(&rng1, x) + low;
}

/**
 * @brief Seed the rng. Specified in two parts, state initializer and a
 * sequence selection constant (a.k.a. stream id)
 * 
 * @param rng		Address of a pcg32_random_t value previously declared
 * @param initstate	Starting state for the RNG, you can pass any 64-bit value
 * @param initseq	Selects the output sequence for the RNG, you can pass any
 * 					64-bit value, although only the low 63 bits are significant
 */
void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

/**
 * @brief Generate a uniformly distributed 32-bit random number
 * 
 * @param rng		Address of a pcg32_random_t value previously declared
 * @return uint32_t	Uniformly distributed 32-bit random number
 */
uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

/**
 * @brief Generate a uniformly distributed number, r, where 0 <= r < bound
 * 
 * @param rng		Address of a pcg32_random_t value previously declared
 * @param bound		Upper limit for the generated number
 * @return uint32_t Uniformly distributed 32-bit random number
 */
uint32_t pcg32_boundedrand_r(pcg32_random_t* rng, uint32_t bound)
{
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In 
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = pcg32_random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}
