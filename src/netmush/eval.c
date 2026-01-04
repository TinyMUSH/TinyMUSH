/**
 * @file eval.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Expression parsing, evaluation, and command/function argument processing
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
#include "ansi.h"

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
	// Cache frequently used flag combinations to avoid redundant checks
	bool do_compress = mushconf.space_compress && !(eval & EV_NO_COMPRESS);

	// Strip trailing space if compression enabled or EV_STRIP_TS set
	if ((do_compress || (eval & EV_STRIP_TS)) && !first && (cstr[-1] == ' '))
	{
		zstr--;
	}

	// Strip outer braces and surrounding whitespace if requested
	if ((eval & EV_STRIP_AROUND) && (*rstr == '{') && (zstr[-1] == '}'))
	{
		rstr++;

		// Strip leading spaces after opening brace
		if (do_compress || (eval & EV_STRIP_LS))
		{
			while (*rstr && isspace(*rstr))
			{
				rstr++;
			}
		}

		rstr[-1] = '\0';
		zstr--;

		// Strip trailing spaces before closing brace
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

// Inline function to avoid repetitive pointer advancement code (used 17+ times)
// Using inline function instead of macro for better type safety and debugging
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

	// Cache frequently checked flag combinations
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
		case '\\': // general escape
		case '%':  // also escapes chars
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

			// If we hit something on the stack, unwind to it Otherwise (it's
			// not on stack), if it's our delim we are done, and we convert the
			// delim to a null and return a ptr to the char after the null. If
			// it's not our delimiter, skip over it normally
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
			case ' ': // space
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

			case ESC_CHAR:
				copy_char(&cstr, &zstr);

				if (*cstr == ANSI_CSI)
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
 *
 */
char *parse_arglist(dbref player, dbref caller, dbref cause, char *dstr, char delim, int eval, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *rstr = NULL, *tstr = NULL, *bp = NULL, *str = NULL;
	int arg = 0;

	// Early exit if no input string
	if (dstr == NULL)
	{
		// Still initialize output array
		for (arg = 0; arg < nfargs; arg++)
		{
			fargs[arg] = NULL;
		}
		return NULL;
	}

	// Initialize output array
	for (arg = 0; arg < nfargs; arg++)
	{
		fargs[arg] = NULL;
	}

	// Cache frequently used flag combinations
	int peval = (eval & ~EV_EVAL);
	bool do_eval = (eval & EV_EVAL);

	rstr = parse_to(&dstr, delim, 0);
	arg = 0;

	while ((arg < nfargs) && rstr)
	{
		// Parse current argument - use '\0' for last arg, ',' for others
		if (arg < (nfargs - 1))
		{
			tstr = parse_to(&rstr, ',', peval);
		}
		else
		{
			tstr = parse_to(&rstr, '\0', peval);
		}

		// Allocate and populate argument
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

/**
 * @brief Resolve a player's gender flag into the internal pronoun code.
 *
 * Looks up the A_SEX attribute and maps its first character to the pronoun
 * set used by %S/%O/%P/%A substitutions: 1=neuter/it, 2=feminine/she,
 * 3=masculine/he, 4=plural/they. Any unknown or missing value returns the
 * neutral form.
 *
 * @param player	DBref of the player to inspect
 * @return int	Pronoun code 1-4 as described above
 */
int get_gender(dbref player)
{
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;
	char *atr_gotten = atr_pget(player, A_SEX, &aowner, &aflags, &alen);
	
	// Handle NULL or empty attribute safely
	if (!atr_gotten || !*atr_gotten)
	{
		if (atr_gotten)
		{
			XFREE(atr_gotten);
		}
		return 1; // default neuter
	}
	
	char first = tolower(*atr_gotten);
	XFREE(atr_gotten);

	switch (first)
	{
	case 'p': // plural
		return 4;

	case 'm': // masculine
		return 3;

	case 'f': // feminine
	case 'w':
		return 2;

	default: // neuter/unknown
		return 1;
	}
}

typedef struct tcache_ent
{
	char *orig;
	char *result;
	struct tcache_ent *next;
} tcache_ent; // Trace cache entry

tcache_ent *tcache_head;	  // Head of trace cache linked list
int tcache_top, tcache_count; // Trace cache status indicators

/**
 * @brief Initialize the expression trace cache system.
 *
 * Resets the trace cache to its initial empty state. The trace cache records
 * input-to-output transformations during expression evaluation when tracing is
 * enabled on a player. Each transformation pair (original expression → evaluated
 * result) is stored as a linked list entry for later display via tcache_finish().
 *
 * This function should be called at server startup and whenever beginning a new
 * trace session.
 *
 * @see tcache_add(), tcache_finish(), tcache_empty()
 */
void tcache_init(void)
{
	tcache_head = NULL;
	tcache_top = 1;
	tcache_count = 0;
}

/**
 * @brief Check if trace cache is at top level and mark as active if so.
 *
 * This function serves dual purposes:
 * 1. Detects if we're at the top level of a trace session (no nested traces)
 * 2. Marks the cache as "in use" by setting tcache_top to 0
 *
 * Called at the start of expression evaluation when tracing is enabled to determine
 * if this is the outermost traced evaluation. If true, this evaluation will be
 * responsible for outputting the complete trace via tcache_finish().
 *
 * @return bool true if this is the top-level trace (cache was empty), false if nested
 *
 * @see tcache_init(), tcache_finish()
 */
static bool tcache_empty(void)
{
	if (tcache_top)
	{
		tcache_top = 0;
		tcache_count = 0;
		return true;
	}

	return false;
}

/**
 * @brief Add an expression evaluation pair to the trace cache.
 *
 * Records a transformation from original expression to evaluated result for later
 * display to the player. Only records transformations where the result differs from
 * the input (optimized to skip identity transformations). Entries are stored in a
 * linked list up to mushconf.trace_limit.
 *
 * The `orig` buffer ownership is transferred to this function - it will be freed
 * either when added to the cache (and later by tcache_finish()) or immediately if
 * the entry is rejected (identical strings or over limit).
 *
 * @param orig    Original expression string (ownership transferred, will be freed)
 * @param result  Evaluated result string (copied into cache if added)
 *
 * @note If orig == result or strcmp(orig, result) == 0, orig is freed and nothing added
 * @note If tcache_count > mushconf.trace_limit, orig is freed and entry discarded
 *
 * @see tcache_finish(), mushconf.trace_limit
 */
static void tcache_add(char *orig, char *result)
{
	// Quick check: if pointers are equal or strings identical, nothing to trace
	if (orig == result || !strcmp(orig, result))
	{
		XFREE(orig);
		return;
	}

	tcache_count++;

	if (tcache_count <= mushconf.trace_limit)
	{
		// Allocate correctly sized structure (24 bytes on 64-bit, not 64)
		tcache_ent *xp = XMALLOC(sizeof(tcache_ent), "xp");
		char *tp = XMALLOC(LBUF_SIZE, "tp");
		XSTRCPY(tp, result);
		xp->orig = orig;
		xp->result = tp;
		xp->next = tcache_head;
		tcache_head = xp;
	}
	else
	{
		XFREE(orig);
	}
}

/**
 * @brief Output and clear all cached trace entries.
 *
 * Displays all accumulated expression transformations (original → result) to the
 * appropriate player via notify_check(). The notification target is determined by:
 * 1. If player has H_Redirect flag: sends to redirected target
 * 2. Otherwise: sends to Owner(player)
 *
 * Each trace entry is formatted as:
 *   "PlayerName(#dbref)} 'original' -> 'result'"
 *
 * After displaying all entries, frees all memory and resets the cache to empty state
 * (tcache_top=1, tcache_count=0). Should be called at the end of a top-level traced
 * evaluation to complete the trace session.
 *
 * @param player  DBref of the player being traced (used for name/dbref in output)
 *
 * @note Handles redirect flag cleanup if H_Redirect is set but no redirect pointer exists
 * @note All tcache entries are freed and pointers nulled after output
 *
 * @see tcache_add(), tcache_empty(), H_Redirect()
 */
static void tcache_finish(dbref player)
{
	// Determine notification target (redirected player or owner)
	dbref target = Owner(player);

	if (H_Redirect(player))
	{
		NUMBERTAB *np = (NUMBERTAB *)nhashfind(player, &mushstate.redir_htab);

		if (np)
		{
			target = np->num;
		}
		else
		{
			// Ick. If we have no pointer, we should have no flag.
			s_Flags3(player, Flags3(player) & ~HAS_REDIRECT);
			// target already set to Owner(player) above
		}
	}

	// Output and free all cached trace entries
	while (tcache_head != NULL)
	{
		tcache_ent *xp = tcache_head;
		tcache_head = xp->next;
		notify_check(target, target, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s(#%d)} '%s' -> '%s'", Name(player), player, xp->orig, xp->result);
		XFREE(xp->orig);
		XFREE(xp->result);
		XFREE(xp);
	}

	tcache_top = 1;
	tcache_count = 0;
}

/**
 * @brief Lookup table for mundane character detection (hot path optimization).
 *
 * Inverted logic to eliminate negation in hot loop:
 * Values: 1 = mundane (not special), 0 = always special, 2 = conditionally mundane
 */
static unsigned char mundane_char_table[256];

/**
 * @brief Initialize the mundane character lookup table.
 *
 * Must be called during server initialization before processing any expressions.
 * All characters default to mundane (1), then special chars are marked as 0 or 2.
 */
void mundane_char_table_init(void)
{
	// Initialize all characters as mundane (1)
	memset(mundane_char_table, 1, sizeof(mundane_char_table));

	// Mark special characters as 0 (always special)
	mundane_char_table[0x00] = 0; // NUL
	mundane_char_table[0x1b] = 0; // ESC
	mundane_char_table[' '] = 0;
	mundane_char_table['%'] = 0;
	mundane_char_table['('] = 0;
	mundane_char_table['['] = 0;
	mundane_char_table['\\'] = 0;
	mundane_char_table['{'] = 0;
	mundane_char_table['#'] = 2; // Conditionally mundane (special in loop/switch)
}

/**
 * @brief Check if a character is mundane (not special) - optimized with lookup table.
 *
 * Returns true for mundane chars, false for special chars (NUL, ESC, ' ', '%', '(',
 * '[', '\', '{', and '#' in loop/switch context). Inverted logic eliminates negation
 * in the hot loop where it's called. Uses pre-computed lookup table for O(1) performance.
 *
 * @param ch	Character to check
 * @return bool	true if mundane (not special), false if special
 */
static bool mundane_char(unsigned char ch)
{
	return mundane_char_table[ch] == 2 ? !(mushstate.in_loop || mushstate.in_switch) : mundane_char_table[ch];
}

/**
 * @brief Lookup table for pronoun substitutions indexed by [pronoun_type][gender-1]
 *
 * Pronoun types: 0='o' (object), 1='p' (possessive), 2='s' (subject), 3='a' (absolute)
 * Gender values: 0=neuter/it, 1=feminine/she, 2=masculine/he, 3=plural/they
 */
static const char *pronoun_table[4][4] = {
	// Object pronouns (o): it, her, him, them
	{"it", "her", "him", "them"},
	// Possessive adjectives (p): its, her, his, their
	{"its", "her", "his", "their"},
	// Subject pronouns (s): it, she, he, they
	{"it", "she", "he", "they"},
	// Absolute possessives (a): its, hers, his, theirs
	{"its", "hers", "his", "theirs"}
};

/**
 * @brief Emit pronoun substitution for %O/%P/%S/%A (and lowercase variants)
 *
 * Uses lookup table instead of nested switches for O(1) performance with
 * better cache locality. Centralizes gender lookup and output logic.
 *
 * @param code    Pronoun code ('o', 'p', 's', 'a' - case insensitive)
 * @param gender  Pointer to cached gender (lazy-loaded if < 0)
 * @param cause   DBref to get name/gender from
 * @param buff    Output buffer start
 * @param bufc    Output buffer cursor
 */
static void emit_pronoun_substitution(char code, int *gender, dbref cause, char *buff, char **bufc)
{
	code = tolower(code);

	if (*gender < 0)
	{
		*gender = get_gender(cause);
	}

	if (!*gender) // Non-player, use name as fallback
	{
		safe_name(cause, buff, bufc);
		if (code == 'p' || code == 'a')
		{
			XSAFELBCHR('s', buff, bufc);
		}
		return;
	}

	// Map pronoun code to table index
	int pronoun_idx = -1;
	switch (code)
	{
	case 'o': pronoun_idx = 0; break;
	case 'p': pronoun_idx = 1; break;
	case 's': pronoun_idx = 2; break;
	case 'a': pronoun_idx = 3; break;
	default: return;
	}

	// Bounds check gender (should be 1-4)
	if (*gender < 1 || *gender > 4)
	{
		return;
	}

	// Lookup and output pronoun string
	XSAFELBSTR(pronoun_table[pronoun_idx][*gender - 1], buff, bufc);
}

/**
 * @brief Evaluate an expression string in place, expanding substitutions and functions.
 *
 * This is the core interpreter for TinyMUSH command lines. It walks the input string
 * referenced by `dstr`, copying plain text to `buff`, expanding %-codes, executing
 * functions inside brackets, and recursively evaluating nested expressions. The
 * cursor `*bufc` is advanced as characters are written. The input pointer `*dstr`
 * is moved forward as segments are consumed so callers can continue parsing after
 * a nested invocation.
 *
 * @param buff		Start of the destination buffer (LBUF-sized)
 * @param bufc		Pointer to the current write cursor inside `buff` (updated in place)
 * @param player	DBref of the executor (permissions and flags)
 * @param caller	DBref of the immediate caller in the call chain
 * @param cause		DBref of the enactor (used for pronouns, name, dbref output)
 * @param eval		Evaluation flags controlling compression, stripping, function checks, etc.
 * @param dstr		Pointer to the input string pointer; advanced past what was consumed
 * @param cargs	    Command argument array for numeric %-subs (%0-%9)
 * @param ncargs	Number of entries in `cargs`
 */
void eval_expression_string(char *buff, char **bufc, dbref player, dbref caller, dbref cause, int eval, char **dstr, char *cargs[], int ncargs)
{
	char *real_fargs[MAX_NFARGS + 1];
	char **fargs = real_fargs + 1;
	char *tstr = NULL, *tbuf = NULL, *savepos = NULL, *atr_gotten = NULL, *start = NULL, *oldp = NULL;
	char *savestr = NULL, *str = NULL, *xptr = NULL, *mundane = NULL, *p = NULL, *xtp = NULL;
	char savec = 0, ch = 0;
	char *realbuff = NULL, *realbp = NULL;
	char *xtbuf = XMALLOC(SBUF_SIZE, "xtbuf");
	dbref aowner = NOTHING;
	int at_space = 1, nfargs = 0, gender = -1, i = 0, j = 0, alldone = 0, aflags = 0, alen = 0, feval = 0;
	int is_trace = Trace(player) && !(eval & EV_NOTRACE), is_top = 0, save_count = 0;
	int ansi = 0, nchar = 0, navail = 0;
	bool hilite_mode = false; /*!< Track hilite/bright mode for color codes */
	FUN *fp = NULL;
	UFUN *ufp = NULL;
	VARENT *xvar = NULL;
	ATTR *ap = NULL;
	GDATA *preserve = NULL;

	if (*dstr == NULL)
	{
		**bufc = '\0';
		XFREE(xtbuf);
		return;
	}

	// Extend the buffer if we need to.
	if (((*bufc) - buff) > (LBUF_SIZE - SBUF_SIZE))
	{
		realbuff = buff;
		realbp = *bufc;
		buff = (char *)XMALLOC(LBUF_SIZE, "buff");
		*bufc = buff;
	}

	oldp = start = *bufc;

	// If we are tracing, save a copy of the starting buffer
	savestr = NULL;

	if (is_trace)
	{
		is_top = tcache_empty();
		savestr = XMALLOC(LBUF_SIZE, "savestr");
		XSTRCPY(savestr, *dstr);
	}

	while (**dstr && !alldone)
	{
		if (mundane_char((unsigned char)**dstr))
		{
			// Mundane characters are the most common. There are usually a
			// bunch in a row. We should just copy them.
			mundane = *dstr;
			nchar = 0;

			do
			{
				nchar++;
			} while (mundane_char((unsigned char)*(++mundane)));

			p = *bufc;
			navail = LBUF_SIZE - 1 - (p - buff);
			nchar = (nchar > navail) ? navail : nchar;
			XMEMCPY(p, *dstr, nchar);
			*bufc = p + nchar;
			*dstr = mundane;
			at_space = 0;
		}

		// We must have a special character at this point.
		if (**dstr == '\0')
		{
			break;
		}

		switch (**dstr)
		{
		case ' ': // A space.  Add a space if not compressing or if previous char was not a space
			if (!(mushconf.space_compress && at_space) || (eval & EV_NO_COMPRESS))
			{
				XSAFELBCHR(' ', buff, bufc);
				at_space = 1;
			}

			break;

		case '\\': // General escape. Add the following char without special processing
			at_space = 0;
			(*dstr)++;

			if (**dstr)
			{
				XSAFELBCHR(**dstr, buff, bufc);
			}
			else
			{
				(*dstr)--;
			}

			break;

		case '[': // Function start.  Evaluate the contents of the square brackets as a function.
			at_space = 0;
			tstr = (*dstr)++;

			if (eval & EV_NOFCHECK)
			{
				XSAFELBCHR('[', buff, bufc);
				*dstr = tstr;
				break;
			}

			// Parse to the matching closing ]
			tbuf = parse_to(dstr, ']', 0);

			if (*dstr == NULL)
			{
				// If no closing bracket, insert the [ and continue.
				XSAFELBCHR('[', buff, bufc);
				*dstr = tstr;
			}
			else
			{
				// We have a complete function.  Parse out the function name and arguments.
				str = tbuf;
				eval_expression_string(buff, bufc, player, caller, cause, (eval | EV_FCHECK | EV_FMAND), &str, cargs, ncargs);
				(*dstr)--;
			}

			break;

		case '{': // Literal start.  Insert everything up to the terminating } without parsing.  If no closing brace, insert the { and continue.
			at_space = 0;
			tstr = (*dstr)++;
			tbuf = parse_to(dstr, '}', 0);

			if (*dstr == NULL)
			{
				XSAFELBCHR('{', buff, bufc);
				*dstr = tstr;
			}
			else
			{
				if (!(eval & EV_STRIP))
				{
					XSAFELBCHR('{', buff, bufc);
				}

				// Preserve leading spaces (Felan)
				if (*tbuf == ' ')
				{
					XSAFELBCHR(' ', buff, bufc);
					tbuf++;
				}

				str = tbuf;
				eval_expression_string(buff, bufc, player, caller, cause, (eval & ~(EV_STRIP | EV_FCHECK)), &str, cargs, ncargs);

				if (!(eval & EV_STRIP))
				{
					XSAFELBCHR('}', buff, bufc);
				}

				(*dstr)--;
			}

			break;

		case '%': // Percent-replace start.  Evaluate the chars following and perform the appropriate substitution.
			at_space = 0;
			(*dstr)++;
			savec = **dstr;
			savepos = *bufc;

			switch (savec)
			{
			case '\0': // Null - all done
				(*dstr)--;
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': // Command argument number N
				i = (**dstr - '0');

				if ((i < ncargs) && (cargs[i] != NULL))
				{
					XSAFELBSTR(cargs[i], buff, bufc);
				}

				break;

			case 'r':
			case 'R': // Carriage return
				XSAFECRLF(buff, bufc);
				break;

			case 't':
			case 'T': // Tab
				XSAFELBCHR('\t', buff, bufc);
				break;

			case 'B':
			case 'b': // Blank
				XSAFELBCHR(' ', buff, bufc);
				break;

			case 'C':
			case 'c': // %c can be last command executed or legacy color (legacy color is deprecated)
				if (mushconf.c_cmd_subst)
				{

					XSAFELBSTR(mushstate.curr_cmd, buff, bufc);
					break;
				}
				//[[fallthrough]];
				__attribute__((fallthrough)); // Legacy color code

			case 'x':
			case 'X': // ANSI color - Use centralized parsing from ansi.c

				(*dstr)++;

				if (!**dstr)
				{
					// End of string after %x - back up and break
					(*dstr)--;
					break;
				}

				if (!mushconf.ansi_colors)
				{
					// ANSI colors disabled - skip the code
					break;
				}

				// Parse the color code using ansi_parse_single_x_code().
				// This handles all formats: <color>, <fg/bg>, +<color>, simple letters, etc.
				{
					ColorState color = {0};
					int consumed = ansi_parse_single_x_code(dstr, &color, &hilite_mode);

					if (consumed > 0)
					{
						// Successfully parsed a color code - generate ANSI escape sequence.
						// Determine color type based on player capabilities.
						ColorType color_type = ColorTypeNone;
						dbref color_target = (cause != NOTHING) ? cause : player; // Prefer enactor/viewer flags for ANSI output

						if (color_target != NOTHING && Color24Bit(color_target))
						{
							color_type = ColorTypeTrueColor;
						}
						else if (color_target != NOTHING && Color256(color_target))
						{
							color_type = ColorTypeXTerm;
						}
						else if (color_target != NOTHING && Ansi(color_target))
						{
							color_type = ColorTypeAnsi;
						}

						char ansi_buf[256];
						size_t ansi_offset = 0;
						ColorStatus result = to_ansi_escape_sequence(ansi_buf, sizeof(ansi_buf), &ansi_offset, &color, color_type);

						if (result != ColorStatusNone)
						{
							XSAFELBSTR(ansi_buf, buff, bufc);
							ansi = (result == ColorStatusReset) ? 0 : 1;
						}

						// dstr has already been advanced by ansi_parse_single_x_code(),
						// compensate for the (*dstr)++ that will happen at the end of the switch
						(*dstr)--;
					}
					else
					{
						// Failed to parse - copy the character literally.
						// This maintains backward compatibility for invalid codes.
						XSAFELBCHR(**dstr, buff, bufc);
					}
				}

				break;

				if (**dstr != '<')
				{
					(*dstr)--;
					break;
				}

				xptr = *dstr;
				(*dstr)++;

				if (!**dstr)
				{
					*dstr = xptr;
					break;
				}

				xtp = xtbuf;

				while (**dstr && (**dstr != '>'))
				{
					XSAFESBCHR(**dstr, xtbuf, &xtp);
					(*dstr)++;
				}

				if (**dstr != '>')
				{
					// Ran off the end. Back up.
					*dstr = xptr;
					break;
				}

				*xtp = '\0';
				ap = atr_str(xtbuf);

				if (!ap)
				{
					break;
				}

				atr_pget_info(player, ap->number, &aowner, &aflags);

				if (See_attr(player, player, ap, aowner, aflags))
				{
					atr_gotten = atr_pget(player, ap->number, &aowner, &aflags, &alen);
					XSAFESTRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
					XFREE(atr_gotten);
				}

				break;

			case '_': // x-variable
				(*dstr)++;

				// Check for %_<varname>
				if (**dstr != '<')
				{
					ch = tolower(**dstr);

					if (!**dstr)
					{
						(*dstr)--;
					}

					if (!isalnum(ch))
					{
						break;
					}

					xtp = xtbuf;
					XSAFELTOS(xtbuf, &xtp, player, LBUF_SIZE);
					XSAFECOPYCHR('.', xtbuf, &xtp, SBUF_SIZE - 1);
					XSAFECOPYCHR(ch, xtbuf, &xtp, SBUF_SIZE - 1);
				}
				else
				{
					xptr = *dstr;
					(*dstr)++;

					if (!**dstr)
					{
						*dstr = xptr;
						break;
					}

					xtp = xtbuf;
					XSAFELTOS(xtbuf, &xtp, player, LBUF_SIZE);
					XSAFECOPYCHR('.', xtbuf, &xtp, SBUF_SIZE - 1);

					while (**dstr && (**dstr != '>'))
					{
						// Copy. No interpretation.
						ch = tolower(**dstr);
						XSAFESBCHR(ch, xtbuf, &xtp);
						(*dstr)++;
					}

					if (**dstr != '>')
					{
						// We ran off the end of the string without finding a termination condition. Go back.
						*dstr = xptr;
						break;
					}
				}

				*xtp = '\0';

				if (!(mushstate.f_limitmask & FN_VARFX) && (xvar = (VARENT *)hashfind(xtbuf, &mushstate.vars_htab)))
				{
					XSAFELBSTR(xvar->text, buff, bufc);
				}

				break;

			case 'V':
			case 'v': // Variable attribute
				(*dstr)++;
				ch = toupper(**dstr);

				if (!**dstr)
				{
					(*dstr)--;
				}

				if ((ch < 'A') || (ch > 'Z'))
				{
					break;
				}

				i = A_VA + ch - 'A';
				atr_gotten = atr_pget(player, i, &aowner, &aflags, &alen);
				XSAFESTRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
				XFREE(atr_gotten);
				break;

			case 'Q':
			case 'q': // Local registers
				(*dstr)++;

				if (!**dstr)
				{
					(*dstr)--;
					break;
				}

				if (**dstr != '<')
				{
					i = qidx_chartab((unsigned char)**dstr);

					if ((i < 0) || (i >= mushconf.max_global_regs))
					{
						break;
					}

					if (mushstate.rdata && mushstate.rdata->q_alloc > i)
					{
						char *qreg = mushstate.rdata->q_regs[i];
						size_t qlen = mushstate.rdata->q_lens[i];
						XSAFESTRNCAT(buff, bufc, qreg, qlen, LBUF_SIZE);
					}

					if (!**dstr)
					{
						(*dstr)--;
					}

					break;
				}

				xptr = *dstr;
				(*dstr)++;

				if (!**dstr)
				{
					*dstr = xptr;
					break;
				}

				if (!mushstate.rdata || !mushstate.rdata->xr_alloc)
				{
					// We know there's no result, so we just advance past.
					while (**dstr && (**dstr != '>'))
					{
						(*dstr)++;
					}

					if (**dstr != '>')
					{
						// Whoops, no end. Go back.
						*dstr = xptr;
						break;
					}

					break;
				}

				xtp = xtbuf;

				while (**dstr && (**dstr != '>'))
				{
					XSAFESBCHR(tolower(**dstr), xtbuf, &xtp);
					(*dstr)++;
				}

				if (**dstr != '>')
				{
					// Ran off the end. Back up.
					*dstr = xptr;
					break;
				}

				*xtp = '\0';

				for (i = 0; i < mushstate.rdata->xr_alloc; i++)
				{
					if (mushstate.rdata->x_names[i] && !strcmp(xtbuf, mushstate.rdata->x_names[i]))
					{
						XSAFESTRNCAT(buff, bufc, mushstate.rdata->x_regs[i], mushstate.rdata->x_lens[i], LBUF_SIZE);
						break;
					}
				}

				break;

			case 'O':
			case 'o':
			case 'P':
			case 'p':
			case 'S':
			case 's':
			case 'A':
			case 'a': // Objective pronoun / Possessive adjective / Subjective / Absolute possessive
				emit_pronoun_substitution(savec, &gender, player, buff, bufc);
				break;

			case '#': // Invoker DB number
				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, cause, LBUF_SIZE);
				break;

			case '!': // Executor DB number
				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, player, LBUF_SIZE);
				break;

			case 'N':
			case 'n': // Invoker name
				safe_name(cause, buff, bufc);
				break;

			case 'L':
			case 'l': // Invoker location db#
				if (!(eval & EV_NO_LOCATION))
				{
					XSAFELBCHR('#', buff, bufc);
					XSAFELTOS(buff, bufc, where_is(cause), LBUF_SIZE);
				}

				break;

			case '@': // Caller dbref
				XSAFELBCHR('#', buff, bufc);
				XSAFELTOS(buff, bufc, caller, LBUF_SIZE);
				break;

			case ':': // Enactor's objID
				XSAFELBCHR(':', buff, bufc);
				XSAFELTOS(buff, bufc, CreateTime(cause), LBUF_SIZE);
				break;

			case 'M':
			case 'm':
				XSAFELBSTR(mushstate.curr_cmd, buff, bufc);
				break;

			case 'I':
			case 'i': // itext() equivalent
			case 'J':
			case 'j': // itext2() equivalent
				xtp = *dstr;
				(*dstr)++;

				if (!**dstr)
				{
					(*dstr)--;
				}

				if (**dstr == '-')
				{
					// use absolute level number
					(*dstr)++;

					if (!**dstr)
					{
						(*dstr)--;
					}

					if (!isdigit(**dstr))
					{
						break;
					}

					i = (**dstr - '0');
				}
				else
				{
					// use number as delta back from current
					if (!mushstate.in_loop || !isdigit(**dstr))
					{
						break;
					}

					i = mushstate.in_loop - 1 - (**dstr - '0');

					if (i < 0)
					{
						break;
					}
				}

				if (i > mushstate.in_loop - 1)
				{
					break;
				}

				if ((*xtp == 'i') || (*xtp == 'I'))
				{
					// itext()
					XSAFELBSTR(mushstate.loop_token[i], buff, bufc);
				}
				else
				{
					// itext2()
					XSAFELBSTR(mushstate.loop_token2[i], buff, bufc);
				}

				break;

			case '+': // Arguments to function
				XSAFELTOS(buff, bufc, ncargs, LBUF_SIZE);
				break;

			case '|': // Piped command output
				XSAFELBSTR(mushstate.pout, buff, bufc);
				break;

			case '%': // Percent - a literal %
				XSAFELBCHR('%', buff, bufc);
				break;

			default: // Just copy
				XSAFELBCHR(**dstr, buff, bufc);
			}

			if (isupper(savec))
			{
				*savepos = toupper(*savepos);
			}

			break;

		case '(': // Arglist start.  See if what precedes is a function. If so, execute it if we should.
			at_space = 0;

			if (!(eval & EV_FCHECK))
			{
				XSAFELBCHR('(', buff, bufc);
				break;
			}
			// Load an sbuf with an uppercase version of the func name, and see if the func exists.
			// Trim trailing spaces from the name if configured.
			**bufc = '\0';
			xtp = xtbuf;
			XSAFESBSTR(oldp, xtbuf, &xtp);
			*xtp = '\0';

			if (mushconf.space_compress && (eval & EV_FMAND))
			{
				while ((--xtp >= xtbuf) && isspace(*xtp))
					;

				xtp++;
				*xtp = '\0';
			}

			for (xtp = xtbuf; *xtp; xtp++)
			{
				*xtp = toupper(*xtp);
			}

			fp = (FUN *)hashfind(xtbuf, &mushstate.func_htab);
			// If not a builtin func, check for global function
			ufp = NULL;

			if (fp == NULL)
			{
				ufp = (UFUN *)hashfind(xtbuf, &mushstate.ufunc_htab);
			}

			// Do the right thing if it doesn't exist
			if (!fp && !ufp)
			{
				if (eval & EV_FMAND)
				{
					*bufc = oldp;
					XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (%s) NOT FOUND", xtbuf);
				}

				eval &= ~EV_FCHECK;
				break;
			}

			// Get the arglist and count the number of args Negative # of args means join subsequent args.
			if (ufp)
			{
				nfargs = MAX_NFARGS;
			}
			else if (fp->nargs < 0)
			{
				nfargs = -fp->nargs;
			}
			else
			{
				nfargs = MAX_NFARGS;
			}

			tstr = *dstr;

			if ((fp && (fp->flags & FN_NO_EVAL)) || (ufp && (ufp->flags & FN_NO_EVAL)))
			{
				feval = (eval & ~EV_EVAL) | EV_STRIP_ESC;
			}
			else
			{
				feval = eval;
			}

			*dstr = parse_arglist(player, caller, cause, *dstr + 1, ')', feval, fargs, nfargs, cargs, ncargs);

			// If no closing delim, just insert the '(' and continue normally
			if (!*dstr)
			{
				*dstr = tstr;
				XSAFELBCHR(**dstr, buff, bufc);

				for (i = 0; i < nfargs; i++)
					if (fargs[i] != NULL)
					{
						XFREE(fargs[i]);
					}

				eval &= ~EV_FCHECK;
				break;
			}

			// Count number of args returned
			(*dstr)--;
			j = 0;

			for (i = 0; i < nfargs; i++)
				if (fargs[i] != NULL)
				{
					j = i + 1;
				}

			nfargs = j;
			// We've got function(args) now, so back up over function name in output buffer.
			*bufc = oldp;

			// If it's a user-defined function, perform it now.
			if (ufp)
			{
				mushstate.func_nest_lev++;
				mushstate.func_invk_ctr++;

				if (mushstate.func_nest_lev >= mushconf.func_nest_lim)
				{
					XSAFELBSTR("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
				}
				else if (mushstate.func_invk_ctr >= mushconf.func_invk_lim)
				{
					XSAFELBSTR("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
				}
				else if (Too_Much_CPU())
				{
					XSAFELBSTR("#-1 FUNCTION CPU LIMIT EXCEEDED", buff, bufc);
				}
				else if (Going(player))
				{
					XSAFELBSTR("#-1 BAD INVOKER", buff, bufc);
				}
				else if (!check_access(player, ufp->perms))
				{
					XSAFENOPERM(buff, bufc);
				}
				else
				{
					tstr = atr_get(ufp->obj, ufp->atr, &aowner, &aflags, &alen);

					if (ufp->flags & FN_PRIV)
					{
						i = ufp->obj;
					}
					else
					{
						i = player;
					}

					str = tstr;

					if (ufp->flags & FN_NOREGS)
					{
						preserve = mushstate.rdata;
						mushstate.rdata = NULL;
					}
					else if (ufp->flags & FN_PRES)
					{
						preserve = save_global_regs("eval.save");
					}

					eval_expression_string(buff, bufc, i, player, cause, ((ufp->flags & FN_NO_EVAL) ? (EV_FCHECK | EV_EVAL) : feval), &str, fargs, nfargs);

					if (ufp->flags & FN_NOREGS)
					{
						if (mushstate.rdata)
						{
							for (int z = 0; z < mushstate.rdata->q_alloc; z++)
							{
								if (mushstate.rdata->q_regs[z])
									XFREE(mushstate.rdata->q_regs[z]);
							}
							for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
							{
								if (mushstate.rdata->x_names[z])
									XFREE(mushstate.rdata->x_names[z]);
								if (mushstate.rdata->x_regs[z])
									XFREE(mushstate.rdata->x_regs[z]);
							}

							if (mushstate.rdata->q_regs)
							{
								XFREE(mushstate.rdata->q_regs);
							}
							if (mushstate.rdata->q_lens)
							{
								XFREE(mushstate.rdata->q_lens);
							}
							if (mushstate.rdata->x_names)
							{
								XFREE(mushstate.rdata->x_names);
							}
							if (mushstate.rdata->x_regs)
							{
								XFREE(mushstate.rdata->x_regs);
							}
							if (mushstate.rdata->x_lens)
							{
								XFREE(mushstate.rdata->x_lens);
							}
							XFREE(mushstate.rdata);
						}

						mushstate.rdata = preserve;
					}
					else if (ufp->flags & FN_PRES)
					{
						restore_global_regs("eval.restore", preserve);
					}

					XFREE(tstr);
				}

				// Return the space allocated for the args
				mushstate.func_nest_lev--;

				for (i = 0; i < nfargs; i++)
					if (fargs[i] != NULL)
					{
						XFREE(fargs[i]);
					}

				eval &= ~EV_FCHECK;
				break;
			}

			// If the number of args is right, perform the func. Otherwise return an error message.
			// Note that parse_arglist returns zero args as one null arg, so we have to handle that case specially.
			if ((fp->nargs == 0) && (nfargs == 1))
			{
				if (!*fargs[0])
				{
					XFREE(fargs[0]);
					fargs[0] = NULL;
					nfargs = 0;
				}
			}

			if ((nfargs == fp->nargs) || (nfargs == -fp->nargs) || (fp->flags & FN_VARARGS))
			{
				// Check recursion limit
				mushstate.func_nest_lev++;
				mushstate.func_invk_ctr++;

				if (mushstate.func_nest_lev >= mushconf.func_nest_lim)
				{
					XSAFELBSTR("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
				}
				else if (mushstate.func_invk_ctr >= mushconf.func_invk_lim)
				{
					XSAFELBSTR("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
				}
				else if (Too_Much_CPU())
				{
					XSAFELBSTR("#-1 FUNCTION CPU LIMIT EXCEEDED", buff, bufc);
				}
				else if (Going(player))
				{
					// Deal with the peculiar case of the calling object being destroyed
					// mid-function sequence, such as with a command()/@destroy combo...
					XSAFELBSTR("#-1 BAD INVOKER", buff, bufc);
				}
				else if (!Check_Func_Access(player, fp))
				{
					XSAFENOPERM(buff, bufc);
				}
				else if (mushstate.f_limitmask & fp->flags)
				{
					XSAFENOPERM(buff, bufc);
				}
				else
				{
					fargs[-1] = (char *)fp;
					fp->fun(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs);
				}

				mushstate.func_nest_lev--;
			}
			else
			{
				XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (%s) EXPECTS %d ARGUMENTS BUT GOT %d", fp->name, fp->nargs, nfargs);
			}

			// Return the space allocated for the arguments
			for (i = 0; i < nfargs; i++)
			{
				XFREE(fargs[i]);
			}

			eval &= ~EV_FCHECK;
			break;

		case '#':
			// We should never reach this point unless we're in a loop or switch, thanks to the table lookup.
			at_space = 0;
			(*dstr)++;

			if (!strchr("!#$+@", **dstr))
			{
				(*dstr)--;
				XSAFELBCHR(**dstr, buff, bufc);
			}
			else
			{
				if ((**dstr == '#') && mushstate.in_loop)
				{
					XSAFELBSTR(mushstate.loop_token[mushstate.in_loop - 1], buff, bufc);
				}
				else if ((**dstr == '@') && mushstate.in_loop)
				{
					XSAFELTOS(buff, bufc, mushstate.loop_number[mushstate.in_loop - 1], LBUF_SIZE);
				}
				else if ((**dstr == '+') && mushstate.in_loop)
				{
					XSAFELBSTR(mushstate.loop_token2[mushstate.in_loop - 1], buff, bufc);
				}
				else if ((**dstr == '$') && mushstate.in_switch)
				{
					XSAFELBSTR(mushstate.switch_token, buff, bufc);
				}
				else if (**dstr == '!')
				{
					// Nesting level of loop takes precedence over switch nesting level.
					XSAFELTOS(buff, bufc, ((mushstate.in_loop) ? (mushstate.in_loop - 1) : mushstate.in_switch), LBUF_SIZE);
				}
				else
				{
					(*dstr)--;
					XSAFELBCHR(**dstr, buff, bufc);
				}
			}

			break;

		case ESC_CHAR:
		{
			char *escape_start = *dstr;
			skip_esccode(dstr);
			for (char *seq = escape_start; seq < *dstr; ++seq)
			{
				XSAFELBCHR(*seq, buff, bufc);
			}
			(*dstr)--;
			break;
		}
		}

		(*dstr)++;
	}

	// If we're eating spaces, and the last thing was a space, eat it up. Complicated by the fact
	// that at_space is initially true. So check to see if we actually put something in the buffer, too.
	if (mushconf.space_compress && at_space && !(eval & EV_NO_COMPRESS) && (start != *bufc))
	{
		(*bufc)--;
	}

	// The ansi() function knows how to take care of itself. However, if the player used a %x sub
	// in the string, and hasn't yet terminated the color with a %xn yet, we'll have to do it for them.
	if (ansi)
	{
		xsafe_ansi_normal(buff, bufc);
	}

	**bufc = '\0';

	// Report trace information
	if (is_trace)
	{
		tcache_add(savestr, start);
		save_count = tcache_count - mushconf.trace_limit;
		;

		if (is_top || !mushconf.trace_topdown)
		{
			tcache_finish(player);
		}

		if (is_top && (save_count > 0))
		{
			tbuf = XMALLOC(MBUF_SIZE, "tbuff");
			XSPRINTF(tbuf, "%d lines of trace output discarded.", save_count);
			notify(player, tbuf);
			XFREE(tbuf);
		}
	}

	if (realbuff)
	{
		*bufc = realbp;
		XSAFELBSTR(buff, realbuff, bufc);
		**bufc = '\0';
		XFREE(buff);
		buff = realbuff;
	}
	XFREE(xtbuf);
}

/**
 * @brief Save the global registers to protect them from various sorts of munging.
 *
 * Creates a deep copy of the current global register state (q-registers and x-registers)
 * so that nested function calls can safely modify registers without affecting the caller's
 * state. Returns NULL if there are no registers to save.
 *
 * @param funcname Function name (used for memory allocation tracking)
 * @return GDATA*  Pointer to saved register state, or NULL if no registers exist
 */
GDATA *save_global_regs(const char *funcname)
{
	// Early exit if no registers exist
	if (!mushstate.rdata || (!mushstate.rdata->q_alloc && !mushstate.rdata->xr_alloc))
	{
		return NULL;
	}

	// Cache pointer to avoid repeated dereferences
	GDATA *rdata = mushstate.rdata;
	
	// Allocate and initialize the preservation structure
	GDATA *preserve = (GDATA *)XMALLOC(sizeof(GDATA), funcname);
	preserve->q_alloc = rdata->q_alloc;
	preserve->xr_alloc = rdata->xr_alloc;
	preserve->dirty = rdata->dirty;

	// Allocate q-register arrays
	if (rdata->q_alloc)
	{
		preserve->q_regs = XCALLOC(rdata->q_alloc, sizeof(char *), "q_regs");
		preserve->q_lens = XCALLOC(rdata->q_alloc, sizeof(int), "q_lens");
		
		// Copy non-empty q-registers
		for (int z = 0; z < rdata->q_alloc; z++)
		{
			if (rdata->q_regs[z] && *rdata->q_regs[z])
			{
				preserve->q_regs[z] = XMALLOC(LBUF_SIZE, funcname);
				XMEMCPY(preserve->q_regs[z], rdata->q_regs[z], rdata->q_lens[z] + 1);
				preserve->q_lens[z] = rdata->q_lens[z];
			}
		}
	}
	else
	{
		preserve->q_regs = NULL;
		preserve->q_lens = NULL;
	}

	// Allocate x-register arrays
	if (rdata->xr_alloc)
	{
		preserve->x_names = XCALLOC(rdata->xr_alloc, sizeof(char *), "x_names");
		preserve->x_regs = XCALLOC(rdata->xr_alloc, sizeof(char *), "x_regs");
		preserve->x_lens = XCALLOC(rdata->xr_alloc, sizeof(int), "x_lens");
		
		// Copy non-empty x-registers
		for (int z = 0; z < rdata->xr_alloc; z++)
		{
			if (rdata->x_names[z] && *rdata->x_names[z] && rdata->x_regs[z] && *rdata->x_regs[z])
			{
				size_t name_len = strlen(rdata->x_names[z]);
				if (name_len >= SBUF_SIZE)
				{
					continue; // Skip overlong names to avoid overflow
				}
				preserve->x_names[z] = XMALLOC(SBUF_SIZE, "glob.x_name");
				XSTRNCPY(preserve->x_names[z], rdata->x_names[z], SBUF_SIZE - 1);
				preserve->x_names[z][SBUF_SIZE - 1] = '\0';
				preserve->x_regs[z] = XMALLOC(LBUF_SIZE, "glob.x_reg");
				XMEMCPY(preserve->x_regs[z], rdata->x_regs[z], rdata->x_lens[z] + 1);
				preserve->x_lens[z] = rdata->x_lens[z];
			}
		}
	}
	else
	{
		preserve->x_names = NULL;
		preserve->x_regs = NULL;
		preserve->x_lens = NULL;
	}

	return preserve;
}

/**
 * @brief Free a GDATA structure and all its allocated contents.
 *
 * Helper function to avoid code duplication. Frees all q-registers, x-registers,
 * their arrays, and the GDATA structure itself.
 *
 * @param gdata  Pointer to GDATA to free (can be NULL)
 */
static void free_gdata(GDATA *gdata)
{
	if (!gdata)
	{
		return;
	}

	// Free q-register contents
	for (int z = 0; z < gdata->q_alloc; z++)
	{
		if (gdata->q_regs[z])
		{
			XFREE(gdata->q_regs[z]);
		}
	}

	// Free x-register contents
	for (int z = 0; z < gdata->xr_alloc; z++)
	{
		if (gdata->x_names[z])
		{
			XFREE(gdata->x_names[z]);
		}
		if (gdata->x_regs[z])
		{
			XFREE(gdata->x_regs[z]);
		}
	}

	// Free arrays
	if (gdata->q_regs)
	{
		XFREE(gdata->q_regs);
	}
	if (gdata->q_lens)
	{
		XFREE(gdata->q_lens);
	}
	if (gdata->x_names)
	{
		XFREE(gdata->x_names);
	}
	if (gdata->x_regs)
	{
		XFREE(gdata->x_regs);
	}
	if (gdata->x_lens)
	{
		XFREE(gdata->x_lens);
	}

	XFREE(gdata);
}

/**
 * @brief Restore the global registers from a previously saved state.
 *
 * Restores global register state (q-registers and x-registers) from a snapshot
 * created by save_global_regs(). If the dirty flag hasn't changed, assumes no
 * modifications were made and simply frees the preserved state. Otherwise, replaces
 * the current register state with the preserved values.
 *
 * @param funcname Function name (used for memory allocation tracking)
 * @param preserve Saved register state to restore (can be NULL to clear registers)
 */
void restore_global_regs(const char *funcname, GDATA *preserve)
{
	// Early exit if nothing to do
	if (!mushstate.rdata && !preserve)
	{
		return;
	}

	// Fast path: No changes made, just free the preserved state
	if (mushstate.rdata && preserve && (mushstate.rdata->dirty == preserve->dirty))
	{
		free_gdata(preserve);
		return;
	}

	// Free current register state
	if (mushstate.rdata)
	{
		free_gdata(mushstate.rdata);
		mushstate.rdata = NULL;
	}

	// If no preserved state, we're done (registers cleared)
	if (!preserve || (!preserve->q_alloc && !preserve->xr_alloc))
	{
		free_gdata(preserve);
		return;
	}

	// Allocate new register structure and copy preserved state
	mushstate.rdata = (GDATA *)XMALLOC(sizeof(GDATA), funcname);
	mushstate.rdata->q_alloc = preserve->q_alloc;
	mushstate.rdata->xr_alloc = preserve->xr_alloc;
	mushstate.rdata->dirty = preserve->dirty;

	// Allocate and copy q-register arrays
	if (preserve->q_alloc)
	{
		mushstate.rdata->q_regs = XCALLOC(preserve->q_alloc, sizeof(char *), "q_regs");
		mushstate.rdata->q_lens = XCALLOC(preserve->q_alloc, sizeof(int), "q_lens");

		for (int z = 0; z < preserve->q_alloc; z++)
		{
			if (preserve->q_regs[z] && *preserve->q_regs[z])
			{
				mushstate.rdata->q_regs[z] = XMALLOC(LBUF_SIZE, funcname);
				XMEMCPY(mushstate.rdata->q_regs[z], preserve->q_regs[z], preserve->q_lens[z] + 1);
				mushstate.rdata->q_lens[z] = preserve->q_lens[z];
			}
		}
	}
	else
	{
		mushstate.rdata->q_regs = NULL;
		mushstate.rdata->q_lens = NULL;
	}

	// Allocate and copy x-register arrays
	if (preserve->xr_alloc)
	{
		mushstate.rdata->x_names = XCALLOC(preserve->xr_alloc, sizeof(char *), "x_names");
		mushstate.rdata->x_regs = XCALLOC(preserve->xr_alloc, sizeof(char *), "x_regs");
		mushstate.rdata->x_lens = XCALLOC(preserve->xr_alloc, sizeof(int), "x_lens");

		for (int z = 0; z < preserve->xr_alloc; z++)
		{
			if (preserve->x_names[z] && *preserve->x_names[z] && preserve->x_regs[z] && *preserve->x_regs[z])
			{
				size_t name_len = strlen(preserve->x_names[z]);
				if (name_len >= SBUF_SIZE)
				{
					continue; // Skip overlong names to avoid overflow
				}
				mushstate.rdata->x_names[z] = XMALLOC(SBUF_SIZE, "glob.x_name");
				XSTRNCPY(mushstate.rdata->x_names[z], preserve->x_names[z], SBUF_SIZE - 1);
				mushstate.rdata->x_names[z][SBUF_SIZE - 1] = '\0';
				mushstate.rdata->x_regs[z] = XMALLOC(LBUF_SIZE, "glob.x_reg");
				XMEMCPY(mushstate.rdata->x_regs[z], preserve->x_regs[z], preserve->x_lens[z] + 1);
				mushstate.rdata->x_lens[z] = preserve->x_lens[z];
			}
		}
	}
	else
	{
		mushstate.rdata->x_names = NULL;
		mushstate.rdata->x_regs = NULL;
		mushstate.rdata->x_lens = NULL;
	}

	// Free the preserved state now that we've copied it
	free_gdata(preserve);
}
