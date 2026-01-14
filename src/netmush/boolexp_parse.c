/**
 * @file boolexp_parse.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Parsing boolean expressions into tree structures
 * @version 4.0
 *
 * This module implements a recursive descent parser that converts infix boolean expressions
 * into BOOLEXP tree structures for use in MUSH object locks.
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

#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

// Error messages
static const char *ERR_ATTR_NUM_OVERFLOW = "ERROR: boolexp.c attribute number overflow or invalid\n";
static const char *ERR_PARSE_DEPTH_EXCEEDED = "ERROR: boolexp.c parse depth exceeded limit\n";

/**
 * @brief Global recursion depth counter for parsing operations
 *
 * Prevents stack overflow attacks by limiting expression complexity.
 * Shared across all parsing operations in this module.
 */
static int boolexp_parse_depth = 0;

/**
 * @defgroup boolexp_parsing Parsing Functions
 * @brief Functions for parsing boolean expression strings into trees
 *
 * These functions implement a recursive descent parser that converts
 * infix boolean expressions into BOOLEXP tree structures.
 */

/**
 * @ingroup boolexp_internal
 * @brief Parses an attribute reference for boolean expressions.
 *
 * This function parses a string representing an attribute reference in the format "attribute:value" or "attribute/value"
 * for use in boolean expressions. It determines whether the attribute is a standard named attribute or a numeric reference,
 * and creates the appropriate BOOLEXP node. The function supports both attribute checks (BOOLEXP_ATR) and evaluation locks (BOOLEXP_EVAL).
 *
 * For named attributes, it looks up the attribute by name. For numeric attributes, it allows direct specification by number,
 * but restricts this to internal parsing or God-level players to prevent abuse.
 *
 * @param s The string containing the attribute reference to parse (format: "attr:value" or "attr/value").
 * @param parse_player The DBref of the player performing the parsing (used for permission checks on numeric attributes).
 * @param parsing_internal True if parsing is done internally (allows numeric attributes), false for user input.
 * @return A pointer to the created BOOLEXP structure representing the attribute check, or NULL if parsing fails.
 */
static inline BOOLEXP *test_atr(char *s, dbref parse_player, bool parsing_internal)
{
	ATTR *attrib = NULL;
	BOOLEXP *b = NULL;
	char *buff = NULL;
	int anum = 0, locktype = 0;

	// Create a modifiable copy of the input string
	buff = XMALLOC(LBUF_SIZE, "buff");
	XSTRCPY(buff, s);

	// Find the separator (: for attribute check, / for evaluation)
	char *sep = buff;
	while (*sep && (*sep != ':') && (*sep != '/'))
	{
		sep++;
	}

	if (!*sep)
	{
		XFREE(buff);
		return NULL;
	}

	// Determine lock type based on separator
	locktype = (*sep == '/') ? BOOLEXP_EVAL : BOOLEXP_ATR;

	// Null-terminate the attribute name and move to the value part
	*sep++ = '\0';

	// Try to find the attribute by name
	if ((attrib = atr_str(buff)) != NULL)
	{
		anum = attrib->number;
	}
	else
	{
		// Numeric attribute reference - only allowed for internal parsing or God
		if (!parsing_internal && !God(parse_player))
		{
			XFREE(buff);
			return NULL;
		}

		// Convert string to number with overflow detection
		errno = 0;
		long temp = strtol(buff, NULL, 10);

		if (errno == ERANGE || temp <= 0 || temp > INT_MAX)
		{
			log_write_raw(1, ERR_ATTR_NUM_OVERFLOW);
			XFREE(buff);
			return NULL;
		}

		anum = (int)temp;
	}

	// Create the BOOLEXP node
	b = alloc_boolexp();
	b->type = locktype;
	b->thing = (dbref)anum;
	b->sub1 = (BOOLEXP *)XSTRDUP(sep, "test_atr.sub1");

	XFREE(buff);
	return b;
}

/**
 * @defgroup boolexp_parsing_internal Internal Parsing Functions
 * @ingroup boolexp_parsing
 * @brief Low-level recursive descent parsing functions
 *
 * These functions implement the recursive descent parser for boolean expressions.
 * Each function corresponds to a grammar rule and parses a specific level of the expression hierarchy.
 */

/* Forward declarations needed for mutual recursion (L calls E, E calls T, T calls F, F calls L) */
static inline BOOLEXP *parse_boolexp_E(char **pBuf, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_T(char **pBuf, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_F(char **pBuf, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_L(char **pBuf, dbref parse_player, bool parsing_internal);

/**
 * @ingroup boolexp_parsing_internal
 * @brief Parses a leaf node in boolean expressions: L -> (E) | object_identifier
 *
 * This function parses the leaf nodes of boolean expressions according to the grammar rule L.
 * It handles two cases:
 * 1. Parenthesized subexpressions: (E) where E is a full boolean expression
 * 2. Object identifiers: attribute references (attr:value or attr/value) or object references (#dbref)
 *
 * For object identifiers, it first attempts to parse as an attribute reference using test_atr().
 * If that fails, it treats the input as an object reference, resolving it to a database reference
 * either by direct parsing (for internal locks) or by name matching (for user input).
 *
 * @param pBuf A pointer to the current position in the input buffer string. Updated as parsing progresses.
 * @param parse_player The DBref of the player performing the parsing (used for permissions and notifications).
 * @param parsing_internal True if parsing is done internally (allows direct dbref parsing), false for user input.
 * @return A pointer to the parsed BOOLEXP structure, or TRUE_BOOLEXP if parsing fails.
 */
static inline BOOLEXP *parse_boolexp_L(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL;
	char *buf = NULL;
	MSTATE mstate;

	*pBuf = (char *)skip_whitespace(*pBuf);

	switch (**pBuf)
	{
	case '(':
		(*pBuf)++;
		b = parse_boolexp_E(pBuf, parse_player, parsing_internal);
		*pBuf = (char *)skip_whitespace(*pBuf);

		if (b == TRUE_BOOLEXP || **pBuf != ')')
		{
			free_boolexp(b);
			return TRUE_BOOLEXP;
		}

		(*pBuf)++; // Consume the closing parenthesis
		break;

	default:
		// Parse object identifier: attribute or object reference
		buf = XMALLOC(LBUF_SIZE, "buf");
		char *p = buf;

		// Copy until delimiter
		while (**pBuf && **pBuf != AND_TOKEN && **pBuf != OR_TOKEN && **pBuf != ')')
		{
			*p++ = **pBuf;
			(*pBuf)++;
		}

		*p = '\0';

		// Trim trailing whitespace
		while (p > buf && isspace(*(p - 1)))
		{
			*--p = '\0';
		}

		// First, try to parse as attribute reference
		if ((b = test_atr(buf, parse_player, parsing_internal)) != NULL)
		{
			XFREE(buf);
			return b;
		}

		// Not an attribute, treat as object reference
		b = alloc_boolexp();
		b->type = BOOLEXP_CONST;

		if (!mushstate.standalone)
		{
			if (parsing_internal)
			{
				// Internal parsing: expect #dbref format
				if (buf[0] != '#')
				{
					XFREE(buf);
					free_boolexp(b);
					return TRUE_BOOLEXP;
				}

				b->thing = (int)strtol(&buf[1], NULL, 10);

				if (!Good_obj(b->thing))
				{
					XFREE(buf);
					free_boolexp(b);
					return TRUE_BOOLEXP;
				}
			}
			else
			{
				// User input: perform name matching
				save_match_state(&mstate);
				init_match(parse_player, buf, TYPE_THING);
				match_everything(MAT_EXIT_PARENTS);
				b->thing = match_result();
				restore_match_state(&mstate);

				if (b->thing == NOTHING)
				{
					notify_check(parse_player, parse_player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN,
					             "I don't see %s here.", buf);
					XFREE(buf);
					free_boolexp(b);
					return TRUE_BOOLEXP;
				}

				if (b->thing == AMBIGUOUS)
				{
					notify_check(parse_player, parse_player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN,
					             "I don't know which %s you mean!", buf);
					XFREE(buf);
					free_boolexp(b);
					return TRUE_BOOLEXP;
				}
			}
		}
		else
		{
			// Standalone mode: simple #dbref parsing
			if (buf[0] != '#')
			{
				XFREE(buf);
				free_boolexp(b);
				return TRUE_BOOLEXP;
			}

			b->thing = (int)strtol(&buf[1], NULL, 10);

			if (b->thing < 0)
			{
				XFREE(buf);
				free_boolexp(b);
				return TRUE_BOOLEXP;
			}
		}

		XFREE(buf);
		break;
	}

	return b;
}

/**
 * @ingroup boolexp_parsing_internal
 * @brief Parses a factor in boolean expressions: F -> !F | @L | =L | +L | $L | L
 *
 * This function parses factors in boolean expressions according to the grammar rule F.
 * Factors can be unary operators applied to other factors or leaf nodes (L).
 * Supported operators:
 * - ! (NOT): Logical negation of a factor
 * - @ (INDIR): Indirection operator, references another object's lock (requires BOOLEXP_CONST subexpression)
 * - = (IS): Checks if the player has the specified attribute or object
 * - + (CARRY): Checks if the player carries the specified attribute or object
 * - $ (OWNER): Checks if the player owns the specified object
 * - Default: Falls back to parsing a leaf node (L)
 *
 * @param pBuf A pointer to the current position in the input buffer string. Updated as parsing progresses.
 * @param parse_player The DBref of the player performing the parsing (used for permissions).
 * @param parsing_internal True if parsing is done internally, false for user input.
 * @return A pointer to the parsed BOOLEXP structure, or TRUE_BOOLEXP if parsing fails.
 */
static inline BOOLEXP *parse_boolexp_F(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL;

	// Check parse depth to prevent stack overflow
	if (++boolexp_parse_depth > MAX_BOOLEXP_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	*pBuf = (char *)skip_whitespace(*pBuf);

	switch (**pBuf)
	{
	case NOT_TOKEN:
		(*pBuf)++;
		b = alloc_boolexp();
		b->type = BOOLEXP_NOT;
		b->sub1 = parse_boolexp_F(pBuf, parse_player, parsing_internal);
		b->sub2 = NULL;

		if (b->sub1 == TRUE_BOOLEXP)
		{
			free_boolexp(b);
			boolexp_parse_depth--;
			return TRUE_BOOLEXP;
		}
		break;

	case INDIR_TOKEN:
		(*pBuf)++;
		b = alloc_boolexp();
		b->type = BOOLEXP_INDIR;
		b->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b->sub2 = NULL;

		if (b->sub1 == TRUE_BOOLEXP || b->sub1->type != BOOLEXP_CONST)
		{
			free_boolexp(b);
			boolexp_parse_depth--;
			return TRUE_BOOLEXP;
		}
		break;

	case IS_TOKEN:
		(*pBuf)++;
		b = alloc_boolexp();
		b->type = BOOLEXP_IS;
		b->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b->sub2 = NULL;

		if (b->sub1 == TRUE_BOOLEXP || (b->sub1->type != BOOLEXP_CONST && b->sub1->type != BOOLEXP_ATR))
		{
			free_boolexp(b);
			boolexp_parse_depth--;
			return TRUE_BOOLEXP;
		}
		break;

	case CARRY_TOKEN:
		(*pBuf)++;
		b = alloc_boolexp();
		b->type = BOOLEXP_CARRY;
		b->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b->sub2 = NULL;

		if (b->sub1 == TRUE_BOOLEXP || (b->sub1->type != BOOLEXP_CONST && b->sub1->type != BOOLEXP_ATR))
		{
			free_boolexp(b);
			boolexp_parse_depth--;
			return TRUE_BOOLEXP;
		}
		break;

	case OWNER_TOKEN:
		(*pBuf)++;
		b = alloc_boolexp();
		b->type = BOOLEXP_OWNER;
		b->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b->sub2 = NULL;

		if (b->sub1 == TRUE_BOOLEXP || b->sub1->type != BOOLEXP_CONST)
		{
			free_boolexp(b);
			boolexp_parse_depth--;
			return TRUE_BOOLEXP;
		}
		break;

	default:
		b = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		break;
	}

	boolexp_parse_depth--;
	return b;
}

/**
 * @ingroup boolexp_parsing_internal
 * @brief Parses a term in boolean expressions: T -> F & T | F
 *
 * This function parses terms in boolean expressions according to the grammar rule T.
 * Terms are factors (F) optionally combined with the logical AND operator (&).
 * The parsing follows left-associative AND operations, allowing expressions like:
 * - Single factor: F
 * - Multiple factors: F & F & F (parsed as (F & (F & F)))
 *
 * @param pBuf A pointer to the current position in the input buffer string. Updated as parsing progresses.
 * @param parse_player The DBref of the player performing the parsing (used for permissions).
 * @param parsing_internal True if parsing is done internally, false for user input.
 * @return A pointer to the parsed BOOLEXP structure, or TRUE_BOOLEXP if parsing fails.
 */
static inline BOOLEXP *parse_boolexp_T(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL, *b2 = NULL;

	// Check parse depth to prevent stack overflow
	if (++boolexp_parse_depth > MAX_BOOLEXP_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Parse the first factor
	b = parse_boolexp_F(pBuf, parse_player, parsing_internal);

	if (b == TRUE_BOOLEXP)
	{
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Skip whitespace and check for AND operator
	*pBuf = (char *)skip_whitespace(*pBuf);

	if (**pBuf != AND_TOKEN)
	{
		boolexp_parse_depth--;
		return b;
	}

	// Consume the AND token and parse the right-hand side
	(*pBuf)++;
	b2 = alloc_boolexp();
	b2->type = BOOLEXP_AND;
	b2->sub1 = b;
	b2->sub2 = parse_boolexp_T(pBuf, parse_player, parsing_internal);

	if (b2->sub2 == TRUE_BOOLEXP)
	{
		free_boolexp(b2);
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	boolexp_parse_depth--;
	return b2;
}

/**
 * @ingroup boolexp_parsing_internal
 * @brief Parses an expression in boolean expressions: E -> T | E | T
 *
 * This function parses expressions in boolean expressions according to the grammar rule E.
 * Expressions are terms (T) optionally combined with the logical OR operator (|).
 * The parsing follows left-associative OR operations, allowing expressions like:
 * - Single term: T
 * - Multiple terms: T | T | T (parsed as (T | (T | T)))
 *
 * @param pBuf A pointer to the current position in the input buffer string. Updated as parsing progresses.
 * @param parse_player The DBref of the player performing the parsing (used for permissions).
 * @param parsing_internal True if parsing is done internally, false for user input.
 * @return A pointer to the parsed BOOLEXP structure, or TRUE_BOOLEXP if parsing fails.
 */
static inline BOOLEXP *parse_boolexp_E(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL, *b2 = NULL;

	// Check parse depth to prevent stack overflow
	if (++boolexp_parse_depth > MAX_BOOLEXP_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Parse the first term
	b = parse_boolexp_T(pBuf, parse_player, parsing_internal);

	if (b == TRUE_BOOLEXP)
	{
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Skip whitespace and check for OR operator
	*pBuf = (char *)skip_whitespace(*pBuf);

	if (**pBuf != OR_TOKEN)
	{
		boolexp_parse_depth--;
		return b;
	}

	// Consume the OR token and parse the right-hand side
	(*pBuf)++;
	b2 = alloc_boolexp();
	b2->type = BOOLEXP_OR;
	b2->sub1 = b;
	b2->sub2 = parse_boolexp_E(pBuf, parse_player, parsing_internal);

	if (b2->sub2 == TRUE_BOOLEXP)
	{
		free_boolexp(b2);
		boolexp_parse_depth--;
		return TRUE_BOOLEXP;
	}

	boolexp_parse_depth--;
	return b2;
}

/**
 * @ingroup boolexp_parsing
 * @brief Parses a complete boolean expression string into a BOOLEXP tree.
 *
 * This function serves as the main entry point for parsing boolean expressions used in MUSH object locks.
 * It implements a recursive descent parser that follows the grammar:
 * - E (Expression): T | E | T  (terms combined with OR)
 * - T (Term): F & T | F        (factors combined with AND)
 * - F (Factor): !F | @L | =L | +L | $L | L  (unary operators or leaf nodes)
 * - L (Leaf): (E) | object_identifier  (parenthesized expressions or identifiers)
 *
 * The parser supports various lock types including object references (#dbref), attribute checks (attr:value),
 * evaluation locks (attr/value), and special operators like indirection (@), IS (=), CARRY (+), OWNER ($).
 *
 * For user input (internal=false), the function performs security validation:
 * - Rejects control characters (tab, CR, LF, ANSI escape)
 * - Ensures balanced parentheses
 * For internal parsing (internal=true), fewer restrictions apply.
 *
 * @param player The DBref of the player performing the parsing (used for permissions and notifications).
 * @param buf The null-terminated string containing the boolean expression to parse.
 * @param internal True if parsing is done internally (allows more syntax), false for user input.
 * @return A pointer to the root of the parsed BOOLEXP tree, or TRUE_BOOLEXP if parsing fails.
 *         TRUE_BOOLEXP represents a constant true value and indicates parsing errors.
 */
BOOLEXP *parse_boolexp(dbref player, const char *buf, bool internal)
{
	char *p = NULL, *pBuf = NULL, *pStore = NULL;
	int num_opens = 0;
	BOOLEXP *ret = NULL;
	bool parsing_internal = false;

	if (!internal)
	{
		// Security validation for user input: reject control characters and check parentheses balance
		for (p = (char *)buf; *p; p++)
		{
			if ((*p == '\t') || (*p == '\r') || (*p == '\n') || (*p == C_ANSI_ESC))
			{
				return (TRUE_BOOLEXP);
			}

			if (*p == '(')
			{
				num_opens++;
			}
			else if (*p == ')')
			{
				if (num_opens > 0)
				{
					num_opens--;
				}
				else
				{
					return (TRUE_BOOLEXP);
				}
			}
		}

		if (num_opens != 0)
		{
			return (TRUE_BOOLEXP);
		}
	}

	// Handle null or empty input
	if ((buf == NULL) || (*buf == '\0'))
	{
		return (TRUE_BOOLEXP);
	}

	// Create a working copy of the input buffer
	pStore = pBuf = XMALLOC(LBUF_SIZE, "parsestore");
	XSTRCPY(pBuf, buf);

	// Determine parsing mode based on standalone state
	if (!mushstate.standalone)
	{
		parsing_internal = internal;
	}

	// Reset parse depth counter for each top-level parse
	boolexp_parse_depth = 0;

	// Parse the expression starting from the top-level rule E
	ret = parse_boolexp_E(&pBuf, player, parsing_internal);

	// Clean up the working buffer
	XFREE(pStore);
	return ret;
}
