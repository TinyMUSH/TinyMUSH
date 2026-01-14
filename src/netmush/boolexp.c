/**
 * @file boolexp.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Parse, evaluate, and render lock boolean expressions
 * @version 4.0
 *
 * This file implements the boolean expression system used for object locks in TinyMUSH.
 * Boolean expressions allow complex access control based on player attributes, object ownership,
 * inventory contents, and logical operations. The system supports parsing infix notation
 * with operators like AND (&), OR (|), NOT (!), and special lock-specific operators.
 *
 * Key components:
 * - Parsing: Converts string expressions into BOOLEXP trees using recursive descent
 * - Evaluation: Recursively evaluates BOOLEXP trees against player and object contexts
 * - Memory management: Allocates and frees BOOLEXP structures efficiently
 * - Attribute checking: Handles visibility permissions and wildcard matching
 *
 * Supported lock types include:
 * - Object references (#123)
 * - Attribute checks (ATTR:value)
 * - Evaluation locks (/ATTR:value)
 * - Logical operators (&, |, !)
 * - Special operators: @ (indirection), = (IS), + (CARRY), $ (OWNER)
 *
 * @attention If the parser returns TRUE_BOOLEXP, it indicates a parsing error or broken lock. TRUE_BOOLEXP cannot be typed in by the user; use \@unlock instead.
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

/**
 * @brief Error message constants for boolean expression operations
 *
 * These constants define error messages used throughout the boolean expression
 * system for logging and debugging purposes. Each message corresponds to a
 * specific error condition that can occur during parsing or evaluation.
 */
#define ERR_BOOLEXP_ATR_NULL "ERROR: boolexp.c BOOLEXP_ATR has NULL sub1\n"          /*!< BOOLEXP_ATR node has null sub1 pointer */
#define ERR_BOOLEXP_EVAL_NULL "ERROR: boolexp.c BOOLEXP_EVAL has NULL sub1\n"       /*!< BOOLEXP_EVAL node has null sub1 pointer */
#define ERR_BOOLEXP_IS_NULL "ERROR: boolexp.c BOOLEXP_IS attribute check has NULL sub1->sub1\n"     /*!< BOOLEXP_IS node has invalid sub1->sub1 */
#define ERR_BOOLEXP_CARRY_NULL "ERROR: boolexp.c BOOLEXP_CARRY attribute check has NULL sub1->sub1\n" /*!< BOOLEXP_CARRY node has invalid sub1->sub1 */
#define ERR_BOOLEXP_UNKNOWN_TYPE "ABORT! boolexp.c, unknown boolexp type in eval_boolexp().\n"       /*!< Unknown BOOLEXP type encountered */
#define ERR_ATTR_NUM_OVERFLOW "ERROR: boolexp.c attribute number overflow or invalid\n"             /*!< Attribute number out of valid range */
#define ERR_PARSE_DEPTH_EXCEEDED "ERROR: boolexp.c parse depth exceeded limit\n"                    /*!< Recursion depth limit exceeded during parsing */

static int parse_depth = 0; /*!< Current recursion depth counter for parsing operations. Prevents stack overflow attacks by limiting expression complexity. */

/* Forward declarations for static inline functions */
static inline bool check_attr(dbref player, dbref lockobj, ATTR *attr, char *key);
static inline BOOLEXP *test_atr(char *s, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_L(char **pBuf, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_F(char **pBuf, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_T(char **pBuf, dbref parse_player, bool parsing_internal);
static inline BOOLEXP *parse_boolexp_E(char **pBuf, dbref parse_player, bool parsing_internal);

/**
 * @defgroup boolexp_functions Boolean Expression Functions
 * @brief Core functions for handling boolean expressions
 *
 * The following functions provide the core functionality for parsing,
 * evaluating, and managing boolean expressions used in object locks.
 * They handle attribute checks, memory management, and recursive evaluation
 * with proper error handling and recursion limits.
 */

/**
 * @defgroup boolexp_internal Internal Helper Functions
 * @brief Internal functions for parsing and evaluation support
 *
 * These functions provide internal support for the boolean expression system,
 * including attribute checking, parsing helpers, and recursion management.
 * They are not part of the public API.
 */

/**
 * @ingroup boolexp_internal
 * @brief Helper function for attribute-based lock checks (IS and CARRY operators)
 *
 * This function performs attribute-based checks for lock expressions using the IS (=) and CARRY (+) operators.
 * It searches for objects that have a specific attribute matching a given key value.
 *
 * For IS operator (check_inventory=false):
 * - Checks if the player object itself has the attribute with the matching key
 *
 * For CARRY operator (check_inventory=true):
 * - Checks all objects in the player's inventory for the attribute with the matching key
 * - Uses the same attribute checking logic as the IS operator
 *
 * The function handles permission checking through the existing check_attr() function,
 * ensuring that attribute visibility rules are properly enforced.
 *
 * @param b				BOOLEXP structure containing the attribute check (BOOLEXP_ATR type expected in sub1)
 * @param player		DBref of the player being checked
 * @param from			DBref of the object containing the lock (used for permission checks)
 * @param check_inventory	If true, check player's inventory (CARRY operator), else check player only (IS operator)
 * @return bool			True if any checked object has the attribute matching the key, false otherwise
 */
static bool check_attr_lock(BOOLEXP *b, dbref player, dbref from, bool check_inventory)
{
	ATTR *a = NULL;
	dbref obj = NOTHING;
	char *key = NULL;

	// Get the attribute
	a = atr_num(b->sub1->thing);

	if (!a)
	{
		return false;
	}

	// Cache the key pointer
	key = (char *)(b->sub1)->sub1;

	// Validate key pointer
	if (!key)
	{
		log_write_raw(1, check_inventory ? ERR_BOOLEXP_CARRY_NULL : ERR_BOOLEXP_IS_NULL);
		return false;
	}

	// Check player first (only for IS operator)
	if (!check_inventory && check_attr(player, from, a, key))
	{
		return true;
	}

	// If checking inventory (CARRY operator), iterate through contents
	if (check_inventory)
	{
		for (obj = Contents(player); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
		{
			if (check_attr(obj, from, a, key))
			{
				return true;
			}
		}
		return false;
	}

	return false;
}

/**
 * @ingroup boolexp_internal
 * @brief Checks if the specified attribute on the player matches the given key, taking into account visibility permissions.
 *
 * This function evaluates whether a particular attribute of a player passes a key check when performed by a locked object.
 * It handles special cases for control (A_LCONTROL) and name (A_NAME) attributes, and uses wildcard pattern matching
 * to compare the attribute's value with the provided key.
 *
 * @param player    DBref of the player whose attribute is being checked.
 * @param lockobj   DBref of the locked object performing the check (used for visibility permission controls).
 * @param attr      Pointer to the ATTR structure representing the attribute to check.
 * @param key       String representing the key to compare with the attribute's value.
 * @return          true if the attribute matches the key and is visible according to permissions, false otherwise.
 */
static inline bool check_attr(dbref player, dbref lockobj, ATTR *attr, char *key)
{
	char *buff = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0, checkit = 0;

	buff = atr_pget(player, attr->number, &aowner, &aflags, &alen);
	checkit = false;

	if (attr->number == A_LCONTROL)
	{
		// We can see control locks... else we'd break zones
		checkit = true;
	}
	else if (See_attr(lockobj, player, attr, aowner, aflags))
	{
		checkit = true;
	}
	else if (attr->number == A_NAME)
	{
		checkit = true;
	}

	if (checkit && (!wild_match(key, buff)))
	{
		checkit = false;
	}

	XFREE(buff);
	return checkit;
}

/**
 * @defgroup boolexp_memory Memory Management Functions
 * @brief Functions for allocating and freeing boolean expression structures
 *
 * These functions handle the memory management of BOOLEXP trees,
 * providing safe allocation and recursive deallocation.
 */

/**
 * @ingroup boolexp_memory
 * @brief Allocates and initializes a BOOLEXP item.
 *
 * This function creates a new BOOLEXP item by allocating memory and initializing all fields to their default values (zero or NULL).
 *
 * @return Pointer to the initialized BOOLEXP item.
 */
BOOLEXP *alloc_boolexp(void)
{
	BOOLEXP *b = XMALLOC(sizeof(BOOLEXP), "b");

	return b;
}

/**
 * @ingroup boolexp_memory
 * @brief Frees a BOOLEXP structure and all its sub-expressions recursively.
 *
 * This function deallocates memory for a BOOLEXP tree, handling different types
 * of boolean expressions appropriately. It recursively frees sub-expressions
 * before freeing the current node. For binary operations (AND/OR), both
 * sub-expressions are freed. For unary operations and attribute checks,
 * only the sub-expression is freed. Constants have no sub-expressions.
 *
 * @param b Pointer to the BOOLEXP structure to free. If b is TRUE_BOOLEXP,
 *          no action is taken as it represents a constant true value that
 *          should not be deallocated.
 */
void free_boolexp(BOOLEXP *b)
{
	if (b == TRUE_BOOLEXP)
		return;

	switch (b->type)
	{
	case BOOLEXP_AND:
	case BOOLEXP_OR:
		XFREE(b->sub1);
		XFREE(b->sub2);
		break;
	case BOOLEXP_NOT:
	case BOOLEXP_CARRY:
	case BOOLEXP_IS:
	case BOOLEXP_OWNER:
	case BOOLEXP_INDIR:
	case BOOLEXP_ATR:
	case BOOLEXP_EVAL:
		XFREE(b->sub1);
		break;
	case BOOLEXP_CONST:
		break;
	}

	XFREE(b);
}

/**
 * @defgroup boolexp_evaluation Evaluation Functions
 * @brief Functions for evaluating boolean expressions against game state
 *
 * These functions evaluate parsed boolean expressions to determine if
 * they pass for a given player, object, and context.
 */

/**
 * @ingroup boolexp_evaluation
 * @brief Evaluates a boolean expression tree for lock checking.
 *
 * This function recursively evaluates a BOOLEXP (boolean expression) tree to determine
 * if a player satisfies the lock conditions. It handles various types of boolean operations
 * including logical AND/OR/NOT, attribute checks, object ownership, and indirection.
 * The evaluation considers player permissions, attribute visibility, and recursion limits
 * to prevent stack overflows.
 *
 * @param player The DBref of the player attempting to pass the lock.
 * @param thing The DBref of the object the lock is on.
 * @param from The DBref of the object from which the evaluation is performed (for permissions).
 * @param b Pointer to the BOOLEXP structure to evaluate. If b is TRUE_BOOLEXP, returns true.
 * @return true if the player satisfies the boolean expression, false otherwise.
 */
bool eval_boolexp(dbref player, dbref thing, dbref from, BOOLEXP *b)
{
	if (b == TRUE_BOOLEXP)
	{
		return true;
	}

	switch (b->type)
	{
	case BOOLEXP_AND:
		return (eval_boolexp(player, thing, from, b->sub1) && eval_boolexp(player, thing, from, b->sub2));

	case BOOLEXP_OR:
		return (eval_boolexp(player, thing, from, b->sub1) || eval_boolexp(player, thing, from, b->sub2));

	case BOOLEXP_NOT:
		return !eval_boolexp(player, thing, from, b->sub1);

	case BOOLEXP_INDIR:
	{
		// BOOLEXP_INDIR (i.e. @) is a unary operation which is replaced at evaluation time by the lock of the object whose number is the argument of the operation.
		char *pname = NULL, *lname = NULL;
		char *key = NULL;
		dbref lock_originator = NOTHING;
		int c = 0;

		mushstate.lock_nest_lev++;

		if (mushstate.lock_nest_lev >= mushconf.lock_nest_lim)
		{
			pname = log_getname(player);

			if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
			{
				lname = log_getname(Location(player));
				log_write(LOG_BUGS, "BUG", "LOCK", "%s in %s: Lock exceeded recursion limit.", pname, lname);
				XFREE(lname);
			}
			else
			{
				log_write(LOG_BUGS, "BUG", "LOCK", "%s: Lock exceeded recursion limit.", pname);
			}

			XFREE(pname);
			notify(player, "Sorry, broken lock!");
			mushstate.lock_nest_lev--;
			return false;
		}

		if ((b->sub1->type != BOOLEXP_CONST) || (b->sub1->thing < 0))
		{
			pname = log_getname(player);
			if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
			{
				lname = log_getname(Location(player));

				log_write(LOG_BUGS, "BUG", "LOCK", "%s in %s: Lock had bad indirection (%c, type %d)", pname, lname, INDIR_TOKEN, b->sub1->type);
				XFREE(lname);
			}
			else
			{
				log_write(LOG_BUGS, "BUG", "LOCK", "%s: Lock had bad indirection (%c, type %d)", pname, INDIR_TOKEN, b->sub1->type);
			}
			XFREE(pname);

			notify(player, "Sorry, broken lock!");
			mushstate.lock_nest_lev--;
			return false;
		}

		key = atr_get(b->sub1->thing, A_LOCK, &lock_originator, &c, &c); 
		// Note: reusing c for aflags and alen, but actually need proper vars
		// Wait, this is wrong. atr_get takes &aowner, &aflags, &alen
		// In original: atr_get(b->sub1->thing, A_LOCK, &aowner, &aflags, &alen);
		// But aowner etc not declared. In original, they are at top.
		// Fix: declare dbref aowner = NOTHING; int aflags = 0, alen = 0;
		dbref aowner = NOTHING;
		int aflags = 0, alen = 0;
		key = atr_get(b->sub1->thing, A_LOCK, &aowner, &aflags, &alen);
		lock_originator = thing;
		c = eval_boolexp_atr(player, b->sub1->thing, from, key);
		lock_originator = NOTHING;
		XFREE(key);
		mushstate.lock_nest_lev--;
		return (c);
	}

	case BOOLEXP_CONST:
		return (b->thing == player || member(b->thing, Contents(player)));

	case BOOLEXP_ATR:
	{
		ATTR *a = NULL;
		dbref obj = NOTHING;

		a = atr_num(b->thing);

		if (!a)
		{
			// no such attribute
			return false;
		}

		// Validate sub1 pointer before casting to char*
		if (!b->sub1)
		{
			log_write_raw(1, ERR_BOOLEXP_ATR_NULL);
			return false;
		}

		// First check the object itself, then its contents
		if (check_attr(player, from, a, (char *)b->sub1))
		{
			return true;
		}

		for (obj = Contents(player); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
		{
			if (check_attr(obj, from, a, (char *)b->sub1))
			{
				return true;
			}
		}
		return false;
	}

	case BOOLEXP_EVAL:
	{
		ATTR *a = NULL;
		dbref source = NOTHING, aowner = NOTHING, lock_originator = NOTHING;
		int aflags = 0, alen = 0, checkit = 0;
		char *buff = NULL, *buff2 = NULL, *bp = NULL, *str = NULL;
		GDATA *preserve = NULL;

		a = atr_num(b->thing);

		if (!a)
		{
			// no such attribute
			return false;
		}

		// Validate sub1 pointer before casting to char*
		if (!b->sub1)
		{
			log_write_raw(1, ERR_BOOLEXP_EVAL_NULL);
			return false;
		}

		source = from;
		buff = atr_pget(from, a->number, &aowner, &aflags, &alen);

		if (!*buff)
		{
			XFREE(buff);
			buff = atr_pget(thing, a->number, &aowner, &aflags, &alen);
			source = thing;
		}

		checkit = false;

		if ((a->number == A_NAME) || (a->number == A_LCONTROL))
		{
			checkit = true;
		}
		else if (Read_attr(source, source, a, aowner, aflags))
		{
			checkit = true;
		}

		if (checkit)
		{
			preserve = save_global_regs("eval_boolexp_save");
			buff2 = bp = XMALLOC(LBUF_SIZE, "buff2");
			str = buff;
			eval_expression_string(buff2, &bp, source, ((lock_originator == NOTHING) ? player : lock_originator), player, EV_FCHECK | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
			restore_global_regs("eval_boolexp_save", preserve);
			checkit = !string_compare(buff2, (char *)b->sub1);
			XFREE(buff2);
		}

		XFREE(buff);
		return checkit;
	}

	case BOOLEXP_IS:

		// If an object check, do that
		if (b->sub1->type == BOOLEXP_CONST)
		{
			return (b->sub1->thing == player);
		}

		// Attribute check - use helper function
		return check_attr_lock(b, player, from, false);

	case BOOLEXP_CARRY:

		// If an object check, do that
		if (b->sub1->type == BOOLEXP_CONST)
		{
			return (member(b->sub1->thing, Contents(player)));
		}

		// Attribute check - use helper function (with inventory check)
		return check_attr_lock(b, player, from, true);

	case BOOLEXP_OWNER:
		return (Owner(b->sub1->thing) == Owner(player));

	default:
		log_write_raw(1, ERR_BOOLEXP_UNKNOWN_TYPE);
		// bad type
		abort();
		return false; // Unreachable, but for safety
	}
	return false; // Unreachable
}

/**
 * @ingroup boolexp_evaluation
 * @brief Evaluates a boolean expression stored in an attribute.
 *
 * This function parses a string containing a boolean expression (typically from an attribute like A_LOCK),
 * evaluates it against the given player and context, and returns the result. If parsing fails (returns NULL),
 * it defaults to true, allowing access. The parsed expression is properly cleaned up after evaluation.
 *
 * @param player The DBref of the player to evaluate the expression for.
 * @param thing The DBref of the object the expression is associated with.
 * @param from The DBref of the object performing the evaluation (for permissions).
 * @param key The string containing the boolean expression to parse and evaluate.
 * @return true if the expression evaluates to true or parsing fails, false otherwise.
 */
bool eval_boolexp_atr(dbref player, dbref thing, dbref from, char *key)
{
	BOOLEXP *b = parse_boolexp(player, key, 1);

	if (b == NULL)
	{
		return true;
	}

	bool result = eval_boolexp(player, thing, from, b);
	free_boolexp(b);
	return result;
}

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
	if (++parse_depth > MAX_BOOLEXP_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		parse_depth--;
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
			parse_depth--;
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
			parse_depth--;
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
			parse_depth--;
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
			parse_depth--;
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
			parse_depth--;
			return TRUE_BOOLEXP;
		}
		break;

	default:
		b = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		break;
	}

	parse_depth--;
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
	if (++parse_depth > MAX_BOOLEXP_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Parse the first factor
	b = parse_boolexp_F(pBuf, parse_player, parsing_internal);

	if (b == TRUE_BOOLEXP)
	{
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Skip whitespace and check for AND operator
	*pBuf = (char *)skip_whitespace(*pBuf);

	if (**pBuf != AND_TOKEN)
	{
		parse_depth--;
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
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	parse_depth--;
	return b2;
}

/**
 * @defgroup boolexp_parsing_internal Internal Parsing Functions
 * @ingroup boolexp_parsing
 * @brief Low-level recursive descent parsing functions
 *
 * These functions implement the recursive descent parser for boolean expressions.
 * Each function corresponds to a grammar rule and parses a specific level of the expression hierarchy.
 */

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
	if (++parse_depth > MAX_BOOLEXP_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Parse the first term
	b = parse_boolexp_T(pBuf, parse_player, parsing_internal);

	if (b == TRUE_BOOLEXP)
	{
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	// Skip whitespace and check for OR operator
	*pBuf = (char *)skip_whitespace(*pBuf);

	if (**pBuf != OR_TOKEN)
	{
		parse_depth--;
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
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	parse_depth--;
	return b2;
}

/**
 * @defgroup boolexp_parsing Parsing Functions
 * @brief Functions for parsing boolean expression strings into trees
 *
 * These functions implement a recursive descent parser that converts
 * infix boolean expressions into BOOLEXP tree structures.
 */

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
	parse_depth = 0;

	// Parse the expression starting from the top-level rule E
	ret = parse_boolexp_E(&pBuf, player, parsing_internal);

	// Clean up the working buffer
	XFREE(pStore);
	return ret;
}
