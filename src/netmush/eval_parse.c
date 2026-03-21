/**
 * @file eval_parse.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief String tokenization and argument-list parsing for the expression evaluator
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * This module implements the two low-level parsing primitives used throughout
 * the evaluator and command processor:
 *
 *  - parse_to()       – split a string on a single delimiter while respecting
 *                       brace/bracket nesting and backslash/percent escapes.
 *  - parse_arglist()  – split a parenthesised argument list into an array of
 *                       individually evaluated strings.
 *
 * Both functions operate in-place on the source buffer so no extra allocation
 * is needed for the token itself.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

/**
 * @brief Final cleanup step for parse_to() before returning a token.
 *
 * Trims spaces and optional outer braces on the just-parsed segment according
 * to the active eval flags. It assumes `cstr` points at the delimiter we
 * stopped on, `rstr` is the start of the token, and `zstr` is the current
 * write cursor. Space compression and brace stripping mirror the behavior of
 * parse_to() so callers get a normalized substring.
 *
 * @param eval   Evaluation flags (EV_STRIP_TS, EV_STRIP_AROUND, EV_NO_COMPRESS)
 * @param first  True if no non-space chars were copied before this cleanup
 * @param cstr   Pointer at the delimiter that ended the token
 * @param rstr   Pointer to the start of the token in the source buffer
 * @param zstr   Pointer to the current end of the destination buffer
 * @return char* Pointer to the start of the cleaned token (usually rstr)
 */
static char *parse_to_cleanup(int eval, bool first, char *cstr, char *rstr, char *zstr)
{
	/* Cache frequently used flag combinations to avoid redundant checks */
	bool do_compress = mushconf.space_compress && !(eval & EV_NO_COMPRESS);

	/* Strip trailing space if compression enabled or EV_STRIP_TS set */
	if ((do_compress || (eval & EV_STRIP_TS)) && !first && (cstr[-1] == ' '))
	{
		zstr--;
	}

	/* Strip outer braces and surrounding whitespace if requested */
	if ((eval & EV_STRIP_AROUND) && (*rstr == '{') && (zstr[-1] == '}'))
	{
		rstr++;

		/* Strip leading spaces after opening brace */
		if (do_compress || (eval & EV_STRIP_LS))
		{
			while (*rstr && isspace(*rstr))
			{
				rstr++;
			}
		}

		rstr[-1] = '\0';
		zstr--;

		/* Strip trailing spaces before closing brace */
		if (do_compress || (eval & EV_STRIP_TS))
		{
			while (zstr[-1] && isspace(zstr[-1]))
			{
				zstr--;
			}
		}

		*zstr = '\0';
	}

	*zstr = '\0';
	return rstr;
}

/**
 * @brief Advance both a source and destination pointer by one character.
 *
 * When source and destination pointers are equal we are working in-place:
 * simply advance both. Otherwise copy the character and advance both. Used
 * in the hot path of parse_to() to collapse escape sequences and brace
 * content in-place.
 *
 * @param c  Pointer to the source cursor (updated in place)
 * @param z  Pointer to the destination cursor (updated in place)
 */
static inline void copy_char(char **c, char **z)
{
	if (*c == *z)
	{
		(*c)++;
		(*z)++;
	}
	else
	{
		**z = **c;
		(*z)++;
		(*c)++;
	}
}

/**
 * @brief Split a string on a delimiter while respecting nesting and escapes.
 *
 * Consumes characters from `*dstr`, copying in-place to collapse escapes and
 * optional space compression. Bracket and parenthesis depth is tracked so the
 * delimiter only triggers at depth 0. On success, the delimiter is replaced by
 * NUL, `*dstr` is advanced past it, and the start of the token is returned.
 * `parse_to_cleanup()` applies brace stripping and trailing-space handling
 * according to `eval`. If the delimiter is never seen, `*dstr` is set to NULL
 * and the token to the end of the string is returned.
 *
 * @param dstr   Pointer to the source string pointer (updated in-place)
 * @param delim  Delimiter to split on when not nested
 * @param eval   Evaluation flags for compression/stripping/escapes
 * @return char* Start of the token inside the original buffer, or NULL
 */
char *parse_to(char **dstr, char delim, int eval)
{
	int sp = 0, tp = 0, bracketlev = 0;
	int stack_limit = mushconf.parse_stack_limit;
	char *rstr = NULL, *cstr = NULL, *zstr = NULL;

	if (stack_limit <= 0)
	{
		stack_limit = 1;
	}

	char *stack = XMALLOC(stack_limit, "stack");
	bool first = true;

	if ((dstr == NULL) || (*dstr == NULL))
	{
		XFREE(stack);
		return NULL;
	}

	if (**dstr == '\0')
	{
		rstr = *dstr;
		*dstr = NULL;
		XFREE(stack);
		return rstr;
	}

	sp = 0;
	rstr = *dstr;

	/* Cache frequently checked flag combinations */
	bool do_compress = mushconf.space_compress && !(eval & EV_NO_COMPRESS);
	bool do_strip_esc = (eval & EV_STRIP_ESC);
	bool do_strip = (eval & EV_STRIP);

	if ((do_compress || (eval & EV_STRIP_LS)))
	{
		while (*rstr && isspace(*rstr))
		{
			rstr++;
		}

		*dstr = rstr;
	}

	zstr = cstr = rstr;

	while (*cstr)
	{
		switch (*cstr)
		{
		case '\\': /* general escape */
		case '%':  /* also escapes chars */
			if ((*cstr == '\\') && do_strip_esc)
			{
				cstr++;
			}
			else
			{
				copy_char(&cstr, &zstr);
			}

			if (*cstr)
			{
				copy_char(&cstr, &zstr);
			}

			first = false;
			break;

		case ']':
		case ')':
			for (tp = sp - 1; (tp >= 0) && (stack[tp] != *cstr); tp--)
				;

			/* If we hit something on the stack, unwind to it. Otherwise (it's
			   not on stack), if it's our delim we are done, and we convert the
			   delim to a null and return a ptr to the char after the null. If
			   it's not our delimiter, skip over it normally. */
			if (tp >= 0)
			{
				sp = tp;
			}
			else if (*cstr == delim)
			{
				rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
				*dstr = ++cstr;
				XFREE(stack);
				return rstr;
			}

			first = false;
			copy_char(&cstr, &zstr);
			break;

		case '{':
			bracketlev = 1;

			if (do_strip)
			{
				cstr++;
			}
			else
			{
				copy_char(&cstr, &zstr);
			}

			while (*cstr && (bracketlev > 0))
			{
				switch (*cstr)
				{
				case '\\':
				case '%':
					if (cstr[1])
					{
						if ((*cstr == '\\') && do_strip_esc)
						{
							cstr++;
						}
						else
						{
							copy_char(&cstr, &zstr);
						}
					}

					break;

				case '{':
					bracketlev++;
					break;

				case '}':
					bracketlev--;
					break;
				}

				if (bracketlev > 0)
				{
					copy_char(&cstr, &zstr);
				}
			}

			if (do_strip && (bracketlev == 0))
			{
				cstr++;
			}
			else if (bracketlev == 0)
			{
				copy_char(&cstr, &zstr);
			}

			first = false;
			break;

		default:
			if ((*cstr == delim) && (sp == 0))
			{
				rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
				*dstr = ++cstr;
				XFREE(stack);
				return rstr;
			}

			switch (*cstr)
			{
			case ' ': /* space */
				if (do_compress)
				{
					if (first)
					{
						rstr++;
					}
					else if (cstr[-1] == ' ')
					{
						zstr--;
					}
				}

				copy_char(&cstr, &zstr);
				break;

			case '[':
				if (sp < stack_limit)
				{
					stack[sp++] = ']';
				}

				copy_char(&cstr, &zstr);
				first = false;
				break;

			case '(':
				if (sp < stack_limit)
				{
					stack[sp++] = ')';
				}

				copy_char(&cstr, &zstr);
				first = false;
				break;

			case C_ANSI_ESC:
				copy_char(&cstr, &zstr);

				if (*cstr == C_ANSI_CSI)
				{
					do
					{
						copy_char(&cstr, &zstr);
					} while ((*cstr & 0xf0) == 0x30);
				}

				while ((*cstr & 0xf0) == 0x20)
				{
					copy_char(&cstr, &zstr);
				}

				if (*cstr)
				{
					copy_char(&cstr, &zstr);
				}

				first = false;
				break;

			default:
				first = false;
				copy_char(&cstr, &zstr);
				break;
			}
		}
	}

	rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
	*dstr = NULL;
	XFREE(stack);
	return rstr;
}

/**
 * @brief Parse a delimited argument list into an array of strings.
 *
 * Splits `dstr` on commas into up to `nfargs` arguments, evaluating each
 * according to `eval`. Each argument is stored in `fargs[]`, which must
 * be preallocated to hold `nfargs` pointers. If `eval` includes
 * EV_EVAL, each argument is further evaluated as an expression
 * before being stored. The source string pointer `dstr` is updated
 * to point after the parsed arguments or set to NULL if fully consumed.
 * Returns the updated `dstr` pointer.
 *
 * @param player	DBref of the executor
 * @param caller	DBref of the immediate caller
 * @param cause		DBref of the enactor
 * @param dstr		Pointer to the source string pointer (updated in-place)
 * @param delim		Delimiter character to split on
 * @param eval		Evaluation flags for argument parsing and evaluation
 * @param fargs		Array of string pointers to store parsed arguments
 * @param nfargs	Maximum number of arguments to parse
 * @param cargs		Array of string pointers for context arguments
 * @param ncargs	Number of context arguments
 * @return char*	Updated source string pointer after parsing
 */
char *parse_arglist(dbref player, dbref caller, dbref cause, char *dstr, char delim, int eval, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *rstr = NULL, *tstr = NULL, *bp = NULL, *str = NULL;
	int arg = 0;

	/* Early exit if no input string – still initialize output array */
	if (dstr == NULL)
	{
		for (arg = 0; arg < nfargs; arg++)
		{
			fargs[arg] = NULL;
		}
		return NULL;
	}

	/* Initialize output array */
	for (arg = 0; arg < nfargs; arg++)
	{
		fargs[arg] = NULL;
	}

	/* Cache frequently used flag combinations */
	int peval = (eval & ~EV_EVAL);
	bool do_eval = (eval & EV_EVAL);

	rstr = parse_to(&dstr, delim, 0);
	arg = 0;

	while ((arg < nfargs) && rstr)
	{
		/* Parse current argument – use '\0' for last arg, ',' for others */
		if (arg < (nfargs - 1))
		{
			tstr = parse_to(&rstr, ',', peval);
		}
		else
		{
			tstr = parse_to(&rstr, '\0', peval);
		}

		/* Allocate and populate argument */
		if (do_eval)
		{
			bp = fargs[arg] = XMALLOC(LBUF_SIZE, "fargs[arg]");
			str = tstr;
			eval_expression_string(fargs[arg], &bp, player, caller, cause, eval | EV_FCHECK, &str, cargs, ncargs);
		}
		else
		{
			fargs[arg] = XMALLOC(LBUF_SIZE, "fargs[arg]");
			XSTRCPY(fargs[arg], tstr);
		}

		arg++;
	}

	return dstr;
}
