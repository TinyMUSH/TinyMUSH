/**
 * @file boolexp.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Parse, evaluate, and render lock boolean expressions
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

#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/**
 * @brief Maximum parse depth to prevent stack overflow
 */
#define MAX_PARSE_DEPTH 100

/**
 * @brief Error message constants
 */
#define ERR_BOOLEXP_ATR_NULL "ERROR: boolexp.c BOOLEXP_ATR has NULL sub1\n"
#define ERR_BOOLEXP_EVAL_NULL "ERROR: boolexp.c BOOLEXP_EVAL has NULL sub1\n"
#define ERR_BOOLEXP_IS_NULL "ERROR: boolexp.c BOOLEXP_IS attribute check has NULL sub1->sub1\n"
#define ERR_BOOLEXP_CARRY_NULL "ERROR: boolexp.c BOOLEXP_CARRY attribute check has NULL sub1->sub1\n"
#define ERR_BOOLEXP_UNKNOWN_TYPE "ABORT! boolexp.c, unknown boolexp type in eval_boolexp().\n"
#define ERR_ATTR_NUM_OVERFLOW "ERROR: boolexp.c attribute number overflow or invalid\n"
#define ERR_PARSE_DEPTH_EXCEEDED "ERROR: boolexp.c parse depth exceeded limit\n"

/**
 * @brief Parser state structure
 */
typedef struct parse_state
{
	dbref parse_player;	   /*!< Player doing the parsing */
	bool parsing_internal; /*!< Internal parse (stored lock) or user input */
	int depth;			   /*!< Current recursion depth */
} PARSE_STATE;

/**
 * @brief Current parse depth (for recursion limiting)
 */
static int parse_depth = 0;

/**
 * @brief Helper function for attribute-based lock checks (IS and CARRY operators)
 *
 * @param b			BOOLEXP structure containing attribute check
 * @param player	DBref being checked
 * @param from		Object containing the lock
 * @param check_inventory	If true, check player's inventory (CARRY), else check player only (IS)
 * @return bool		True if attribute check passes
 */
static bool check_attr_lock(BOOLEXP *b, dbref player, dbref from, bool check_inventory)
{
	ATTR *a = NULL;
	dbref obj = NOTHING;

	/**
	 * Get the attribute
	 */
	a = atr_num(b->sub1->thing);

	if (!a)
	{
		return false;
	}

	/**
	 * Validate sub1->sub1 pointer before casting to char*
	 */
	if (!(b->sub1)->sub1)
	{
		log_write_raw(1, check_inventory ? ERR_BOOLEXP_CARRY_NULL : ERR_BOOLEXP_IS_NULL);
		return false;
	}

	/**
	 * Check player first
	 */
	if (!check_inventory && check_attr(player, from, a, (char *)(b->sub1)->sub1))
	{
		return true;
	}

	/**
	 * If checking inventory, iterate through contents
	 */
	if (check_inventory)
	{
		for (obj = Contents(player); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
		{
			if (check_attr(obj, from, a, (char *)(b->sub1)->sub1))
			{
				return true;
			}
		}
		return false;
	}

	return false;
}

/**
 * @brief indicate if attribute ATTR on player passes key when checked by the object lockobj
 *
 * @param player	DBref of player
 * @param lockobj	DBref of locked object
 * @param attr		Attribute
 * @param key		Key
 * @return bool
 */
bool check_attr(dbref player, dbref lockobj, ATTR *attr, char *key)
{
	char *buff = NULL;
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0, checkit = 0;

	buff = atr_pget(player, attr->number, &aowner, &aflags, &alen);
	checkit = false;

	if (attr->number == A_LCONTROL)
	{
		/**
		 * We can see control locks... else we'd break zones
		 *
		 */
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
 * @brief Prepare a BOOLEXP item.
 *
 * @return BOOLEXP* Initialized BOOLEXP item.
 */
BOOLEXP *alloc_boolexp(void)
{
	BOOLEXP *b = XMALLOC(sizeof(BOOLEXP), "b");

	b->type = 0;
	b->thing = 0;
	b->sub1 = NULL;
	b->sub2 = NULL;

	return (b);
}

/**
 * @brief Free a BOOLEXP item.
 *
 * @param b BOOLEXP item.
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
		XFREE(b);
		break;
	case BOOLEXP_NOT:
	case BOOLEXP_CARRY:
	case BOOLEXP_IS:
	case BOOLEXP_OWNER:
	case BOOLEXP_INDIR:
		XFREE(b->sub1);
		XFREE(b);
		break;
	case BOOLEXP_CONST:
		XFREE(b);
		break;
	case BOOLEXP_ATR:
	case BOOLEXP_EVAL:
		XFREE(b->sub1);
		XFREE(b);
		break;
	}
}

/**
 * @brief Evaluate a boolean expression
 *
 * @param player	DBref of player
 * @param thing		DBref of thing
 * @param from		DBref from
 * @param b			Boolean expression
 * @return bool
 */
bool eval_boolexp(dbref player, dbref thing, dbref from, BOOLEXP *b)
{
	dbref aowner = NOTHING, obj = NOTHING, source = NOTHING, lock_originator = NOTHING;
	int aflags = 0, alen = 0, c = 0, checkit = 0;
	char *key = NULL, *buff = NULL, *buff2 = NULL, *bp = NULL, *str = NULL, *pname = NULL, *lname = NULL;
	ATTR *a = NULL;
	GDATA *preserve = NULL;

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
		/**
		 * BOOLEXP_INDIR (i.e. @) is a unary operation which is
		 * replaced at evaluation time by the lock of the object
		 * whose number is the argument of the operation.
		 *
		 */
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
				log_write(LOG_BUGS, "BUG", "LOCK", "%s in %s: Lock had bad indirection (%c, type %d)", pname, INDIR_TOKEN, b->sub1->type);
			}
			XFREE(pname);

			notify(player, "Sorry, broken lock!");
			mushstate.lock_nest_lev--;
			return false;
		}

		key = atr_get(b->sub1->thing, A_LOCK, &aowner, &aflags, &alen);
		lock_originator = thing;
		c = eval_boolexp_atr(player, b->sub1->thing, from, key);
		lock_originator = NOTHING;
		XFREE(key);
		mushstate.lock_nest_lev--;
		return (c);

	case BOOLEXP_CONST:
		return (b->thing == player || member(b->thing, Contents(player)));

	case BOOLEXP_ATR:
		a = atr_num(b->thing);

		if (!a)
		{
			/**
			 * no such attribute
			 *
			 */
			return false;
		}

		/**
		 * Validate sub1 pointer before casting to char*
		 */
		if (!b->sub1)
		{
			log_write_raw(1, ERR_BOOLEXP_ATR_NULL);
			return false;
		}

		/**
		 * First check the object itself, then its contents
		 *
		 */
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

	case BOOLEXP_EVAL:
		a = atr_num(b->thing);

		if (!a)
		{
			/**
			 * no such attribute
			 *
			 */
			return false;
		}

		/**
		 * Validate sub1 pointer before casting to char*
		 */
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

	case BOOLEXP_IS:

		/**
		 * If an object check, do that
		 *
		 */
		if (b->sub1->type == BOOLEXP_CONST)
		{
			return (b->sub1->thing == player);
		}

		/**
		 * Attribute check - use helper function
		 */
		return check_attr_lock(b, player, from, false);

	case BOOLEXP_CARRY:

		/**
		 * If an object check, do that
		 *
		 */
		if (b->sub1->type == BOOLEXP_CONST)
		{
			return (member(b->sub1->thing, Contents(player)));
		}

		/**
		 * Attribute check - use helper function (with inventory check)
		 */
		return check_attr_lock(b, player, from, true);

	case BOOLEXP_OWNER:
		return (Owner(b->sub1->thing) == Owner(player));

	default:
		log_write_raw(1, ERR_BOOLEXP_UNKNOWN_TYPE);
		/**
		 * bad type
		 */
		abort();
	}
	return false;
}

/**
 * @brief Evaluate attribute's boolean expression
 *
 * @param player	DBref of player
 * @param thing		DBref of thing
 * @param from		DBref from
 * @param key		Attribute's key
 * @return bool
 */
bool eval_boolexp_atr(dbref player, dbref thing, dbref from, char *key)
{
	BOOLEXP *b = NULL;
	int ret_value = false;

	b = parse_boolexp(player, key, 1);

	if (b == NULL)
	{
		ret_value = true;
	}
	else
	{
		ret_value = eval_boolexp(player, thing, from, b);
		free_boolexp(b);
	}

	return ret_value;
}

/**
 * @attention If the parser returns TRUE_BOOLEXP, you lose TRUE_BOOLEXP cannot be typed in by the user; use @unlock instead
 *
 */

/**
 * @brief Skip over whitespace in a string
 *
 * @param pBuf Buffer to skip whitespace in
 */
void skip_whitespace(char **pBuf)
{
	while ((**pBuf) && isspace((**pBuf)))
	{
		(*pBuf)++;
	}
}

/**
 * @brief Test attriutes
 *
 * @param s				Attribute to test
 * @param parse_player	DBref of player triggering the test
 * @return BOOLEXP*		Boolean expression with the result of the test
 */
BOOLEXP *test_atr(char *s, dbref parse_player, bool parsing_internal)
{
	ATTR *attrib = NULL;
	BOOLEXP *b = NULL;
	char *buff = NULL, *s1 = NULL;
	int anum = 0, locktype = 0;

	buff = XMALLOC(LBUF_SIZE, "buff");
	XSTRCPY(buff, s);

	for (s = buff; *s && (*s != ':') && (*s != '/'); s++)
		;

	if (!*s)
	{
		XFREE(buff);
		return ((BOOLEXP *)NULL);
	}

	if (*s == '/')
	{
		locktype = BOOLEXP_EVAL;
	}
	else
	{
		locktype = BOOLEXP_ATR;
	}

	*s++ = '\0';
	/**
	 * See if left side is valid attribute.  Access to attr is checked
	 * on eval. Also allow numeric references to attributes.  It can't hurt us,
	 * and lets us import stuff that stores attr locks by number instead of by
	 * name.
	 *
	 */
	if (!(attrib = atr_str(buff)))
	{
		/**
		 * Only #1 can lock on numbers
		 *
		 */
		if (!parsing_internal && !God(parse_player))
		{
			XFREE(buff);
			return ((BOOLEXP *)NULL);
		}

		s1 = buff;

		for (s1 = buff; isdigit(*s1); s1++)
			;

		if (*s1)
		{
			XFREE(buff);
			return ((BOOLEXP *)NULL);
		}

		/**
		 * Convert to long with overflow detection
		 */
		errno = 0;
		long temp = strtol(buff, (char **)NULL, 10);

		if (errno == ERANGE || temp <= 0 || temp > INT_MAX)
		{
			log_write_raw(1, ERR_ATTR_NUM_OVERFLOW);
			XFREE(buff);
			return ((BOOLEXP *)NULL);
		}

		anum = (int)temp;
	}
	else
	{
		anum = attrib->number;
	}

	/**
	 * made it now make the parse tree node
	 *
	 */
	b = alloc_boolexp();
	b->type = locktype;
	b->thing = (dbref)anum;
	b->sub1 = (BOOLEXP *)XSTRDUP(s, "test_atr.sub1");
	XFREE(buff);
	return (b);
}

/**
 * @brief L -> (E); L -> object identifier
 *
 * @param pBuf				Attribute buffer
 * @param parse_player		DBref of player
 * @param parsing_internal	Engine or player triggered?
 * @return BOOLEXP*			Boolean expression result
 */
BOOLEXP *parse_boolexp_L(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL;
	char *p = NULL, *buf = NULL;
	MSTATE mstate;
	buf = NULL;

	skip_whitespace(pBuf);

	switch ((**pBuf))
	{
	case '(':
		(*pBuf)++;
		b = parse_boolexp_E(pBuf, parse_player, parsing_internal);
		skip_whitespace(pBuf);

		if (b == TRUE_BOOLEXP || **pBuf != ')')
		{
			free_boolexp(b);
			return TRUE_BOOLEXP;
		}

		// Consume the closing parenthesis
		(*pBuf)++;

		break;
	default:
		/**
		 * must have hit an object ref.  Load the name into our buffer
		 *
		 */
		buf = XMALLOC(LBUF_SIZE, "buf");
		// buf = XSTRDUP(*pBuf, "buf");
		p = buf;

		while ((**pBuf) && ((**pBuf) != AND_TOKEN) && ((**pBuf) != OR_TOKEN) && ((**pBuf) != ')'))
		{
			*p++ = (**pBuf);
			(*pBuf)++;
		}

		*p-- = '\0';
		while (isspace(*p))
		{
			*p-- = '\0';
		}

		/**
		 * check for an attribute
		 *
		 */
		if ((b = test_atr(buf, parse_player, parsing_internal)) != NULL)
		{
			XFREE(buf);
			return (b);
		}

		b = alloc_boolexp();
		b->type = BOOLEXP_CONST;

		/**
		 * Do the match. If we are parsing a boolexp that was a
		 * stored lock then we know that object refs are all dbrefs,
		 * so we skip the expensive match code.
		 *
		 */
		if (!mushstate.standalone)
		{
			if (parsing_internal)
			{
				if (buf[0] != '#')
				{
					XFREE(buf);
					free_boolexp(b);
					return TRUE_BOOLEXP;
				}

				b->thing = (int)strtol(&buf[1], (char **)NULL, 10);

				if (!Good_obj(b->thing))
				{
					XFREE(buf);
					free_boolexp(b);
					return TRUE_BOOLEXP;
				}
			}
			else
			{
				save_match_state(&mstate);
				init_match(parse_player, buf, TYPE_THING);
				match_everything(MAT_EXIT_PARENTS);
				b->thing = match_result();
				restore_match_state(&mstate);
			}

			if (b->thing == NOTHING)
			{
				notify_check(parse_player, parse_player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "I don't see %s here.", buf);
				XFREE(buf);
				free_boolexp(b);
				return TRUE_BOOLEXP;
			}

			if (b->thing == AMBIGUOUS)
			{
				notify_check(parse_player, parse_player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "I don't know which %s you mean!", buf);
				XFREE(buf);
				free_boolexp(b);
				return TRUE_BOOLEXP;
			}
		}
		else
		{
			if (buf[0] != '#')
			{
				XFREE(buf);
				free_boolexp(b);
				return TRUE_BOOLEXP;
			}

			b->thing = (int)strtol(&buf[1], (char **)NULL, 10);

			if (b->thing < 0)
			{
				XFREE(buf);
				free_boolexp(b);
				return TRUE_BOOLEXP;
			}
		}

		XFREE(buf);
	}

	return b;
}

/**
 * @brief F -> !F; F -> @L; F -> =L; F -> +L; F -> $L
 *
 * The argument L must be type BOOLEXP_CONST
 *
 * @param pBuf				Attribute buffer
 * @param parse_player		DBref of player
 * @param parsing_internal	Engine or player triggered?
 * @return BOOLEXP*			Boolean expression result
 */
BOOLEXP *parse_boolexp_F(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b2 = NULL;

	/**
	 * Check parse depth to prevent stack overflow
	 */
	if (++parse_depth > MAX_PARSE_DEPTH)
	{
		log_write_raw(1, "ERROR: boolexp.c parse depth exceeded limit\n");
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	skip_whitespace(pBuf);

	switch ((**pBuf))
	{
	case NOT_TOKEN:
		(*pBuf)++;
		b2 = alloc_boolexp();
		b2->type = BOOLEXP_NOT;

		if ((b2->sub1 = parse_boolexp_F(pBuf, parse_player, parsing_internal)) == TRUE_BOOLEXP)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else
		{
			parse_depth--;
			return (b2);
		}

	case INDIR_TOKEN:
		(*pBuf)++;
		b2 = alloc_boolexp();
		b2->type = BOOLEXP_INDIR;
		b2->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b2->sub2 = NULL;

		if ((b2->sub1) == TRUE_BOOLEXP)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else if ((b2->sub1->type) != BOOLEXP_CONST)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else
		{
			parse_depth--;
			return (b2);
		}

	case IS_TOKEN:
		(*pBuf)++;
		b2 = alloc_boolexp();
		b2->type = BOOLEXP_IS;
		b2->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b2->sub2 = NULL;

		if ((b2->sub1) == TRUE_BOOLEXP)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else if (((b2->sub1->type) != BOOLEXP_CONST) && ((b2->sub1->type) != BOOLEXP_ATR))
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else
		{
			parse_depth--;
			return (b2);
		}

	case CARRY_TOKEN:
		(*pBuf)++;
		b2 = alloc_boolexp();
		b2->type = BOOLEXP_CARRY;
		b2->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b2->sub2 = NULL;

		if ((b2->sub1) == TRUE_BOOLEXP)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else if (((b2->sub1->type) != BOOLEXP_CONST) && ((b2->sub1->type) != BOOLEXP_ATR))
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else
		{
			parse_depth--;
			return (b2);
		}

	case OWNER_TOKEN:
		(*pBuf)++;
		b2 = alloc_boolexp();
		b2->type = BOOLEXP_OWNER;
		b2->sub1 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		b2->sub2 = NULL;

		if ((b2->sub1) == TRUE_BOOLEXP)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else if ((b2->sub1->type) != BOOLEXP_CONST)
		{
			free_boolexp(b2);
			parse_depth--;
			return (TRUE_BOOLEXP);
		}
		else
		{
			parse_depth--;
			return (b2);
		}

	default:
		b2 = parse_boolexp_L(pBuf, parse_player, parsing_internal);
		parse_depth--;
		return b2;
	}
}

/**
 * @brief T -> F; T -> F & T
 *
 * @param pBuf				Attribute buffer
 * @param parse_player		DBref of player
 * @param parsing_internal	Engine or player triggered?
 * @return BOOLEXP*			Boolean expression result
 */
BOOLEXP *parse_boolexp_T(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL, *b2 = NULL;

	/**
	 * Check parse depth to prevent stack overflow
	 */
	if (++parse_depth > MAX_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	if ((b = parse_boolexp_F(pBuf, parse_player, parsing_internal)) != TRUE_BOOLEXP)
	{
		skip_whitespace(pBuf);

		if ((**pBuf) == AND_TOKEN)
		{
			(*pBuf)++;
			b2 = alloc_boolexp();
			b2->type = BOOLEXP_AND;
			b2->sub1 = b;
			b2->sub2 = NULL;

			if ((b2->sub2 = parse_boolexp_T(pBuf, parse_player, parsing_internal)) == TRUE_BOOLEXP)
			{
				free_boolexp(b2);
				parse_depth--;
				return TRUE_BOOLEXP;
			}

			b = b2;
		}
	}

	parse_depth--;
	return b;
}

/**
 * @brief E -> T; E -> T | E
 *
 * @param pBuf				Attribute buffer
 * @param parse_player		DBref of player
 * @param parsing_internal	Engine or player triggered?
 * @return BOOLEXP*			Boolean expression result
 */
BOOLEXP *parse_boolexp_E(char **pBuf, dbref parse_player, bool parsing_internal)
{
	BOOLEXP *b = NULL, *b2 = NULL;

	/**
	 * Check parse depth to prevent stack overflow
	 */
	if (++parse_depth > MAX_PARSE_DEPTH)
	{
		log_write_raw(1, ERR_PARSE_DEPTH_EXCEEDED);
		parse_depth--;
		return TRUE_BOOLEXP;
	}

	if ((b = parse_boolexp_T(pBuf, parse_player, parsing_internal)) != TRUE_BOOLEXP)
	{
		skip_whitespace(pBuf);

		if ((**pBuf) == OR_TOKEN)
		{
			(*pBuf)++;
			b2 = alloc_boolexp();
			b2->type = BOOLEXP_OR;
			b2->sub1 = b;
			b2->sub2 = NULL;

			if ((b2->sub2 = parse_boolexp_E(pBuf, parse_player, parsing_internal)) == TRUE_BOOLEXP)
			{
				free_boolexp(b2);
				parse_depth--;
				return TRUE_BOOLEXP;
			}

			b = b2;
		}
	}

	parse_depth--;
	return b;
}

/**
 * @brief Parse a boolean expression
 *
 * @param player	DBref of player
 * @param buf		Attribute buffer
 * @param internal	Internal check?
 * @return BOOLEXP*	Boolean expression result
 */
BOOLEXP *parse_boolexp(dbref player, const char *buf, bool internal)
{
	char *p = NULL, *pBuf = NULL, *pStore = NULL;
	int num_opens = 0;
	BOOLEXP *ret = NULL;
	bool parsing_internal = false;

	if (!internal)
	{
		/*
		 * Don't allow funky characters in locks. Don't allow
		 * unbalanced parentheses.
		 */
		for (p = (char *)buf; *p; p++)
		{
			if ((*p == '\t') || (*p == '\r') || (*p == '\n') || (*p == ESC_CHAR))
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

	if ((buf == NULL) || (*buf == '\0'))
	{
		return (TRUE_BOOLEXP);
	}

	pStore = pBuf = XMALLOC(LBUF_SIZE, "parsestore");
	XSTRCPY(pBuf, buf);

	if (!mushstate.standalone)
	{
		parsing_internal = internal;
	}

	/**
	 * Reset parse depth counter for each top-level parse
	 */
	parse_depth = 0;

	ret = parse_boolexp_E(&pBuf, player, parsing_internal);
	XFREE(pStore);
	return ret;
}
