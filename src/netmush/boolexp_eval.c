/**
 * @file boolexp_eval.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Evaluation, memory management, and attribute checking for boolean expressions
 * @version 4.0
 *
 * This module handles evaluation of parsed boolean expression trees, memory management
 * for BOOLEXP structures, and attribute visibility checking for lock evaluation.
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
#include <string.h>

// Error messages
static const char *ERR_BOOLEXP_ATR_NULL = "ERROR: boolexp.c BOOLEXP_ATR has NULL sub1\n";
static const char *ERR_BOOLEXP_EVAL_NULL = "ERROR: boolexp.c BOOLEXP_EVAL has NULL sub1\n";
static const char *ERR_BOOLEXP_IS_NULL = "ERROR: boolexp.c BOOLEXP_IS attribute check has NULL sub1->sub1\n";
static const char *ERR_BOOLEXP_CARRY_NULL = "ERROR: boolexp.c BOOLEXP_CARRY attribute check has NULL sub1->sub1\n";
static const char *ERR_BOOLEXP_UNKNOWN_TYPE = "ABORT! boolexp.c, unknown boolexp type in eval_boolexp().\n";

/**
 * @defgroup boolexp_memory Memory Management Functions
 * @ingroup boolexp_internal
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

		{
			dbref aowner = NOTHING;
			int aflags = 0, alen = 0;
			key = atr_get(b->sub1->thing, A_LOCK, &aowner, &aflags, &alen);
		}
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
