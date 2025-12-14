/**
 * @file boolexp.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Handle boolean expressions
 * @version 3.3
 * @date 2020-12-24
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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
		 * Nope, do an attribute check
		 *
		 */
		a = atr_num(b->sub1->thing);

		if (!a)
		{
			return false;
		}

		return (check_attr(player, from, a, (char *)(b->sub1)->sub1));

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
		 * Nope, do an attribute check
		 *
		 */
		a = atr_num(b->sub1->thing);

		if (!a)
		{
			return false;
		}

		for (obj = Contents(player); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
		{
			if (check_attr(obj, from, a, (char *)(b->sub1)->sub1))
			{
				return true;
			}
		}
		return false;

	case BOOLEXP_OWNER:
		return (Owner(b->sub1->thing) == Owner(player));

	default:
		log_write_raw(1, "ABORT! boolexp.c, unknown boolexp type in eval_boolexp().\n");
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
BOOLEXP *test_atr(char *s, dbref parse_player)
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
		if (!God(parse_player))
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

		anum = (int)strtol(buff, (char **)NULL, 10);

		if (anum <= 0)
		{
			XFREE(buff);
			return ((BOOLEXP *)NULL);
		}
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

		if (b == TRUE_BOOLEXP || (**pBuf)++ != ')')
		{
			free_boolexp(b);
			return TRUE_BOOLEXP;
		}

		break;
	default:
		/**
		 * must have hit an object ref.  Load the name into our buffer
		 *
		 */
		buf = XMALLOC(LBUF_SIZE, "buf");
		//buf = XSTRDUP(*pBuf, "buf");
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
		if ((b = test_atr(buf, parse_player)) != NULL)
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
			return (TRUE_BOOLEXP);
		}
		else
		{
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
			return (TRUE_BOOLEXP);
		}
		else if ((b2->sub1->type) != BOOLEXP_CONST)
		{
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		}
		else
		{
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
			return (TRUE_BOOLEXP);
		}
		else if (((b2->sub1->type) != BOOLEXP_CONST) && ((b2->sub1->type) != BOOLEXP_ATR))
		{
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		}
		else
		{
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
			return (TRUE_BOOLEXP);
		}
		else if (((b2->sub1->type) != BOOLEXP_CONST) && ((b2->sub1->type) != BOOLEXP_ATR))
		{
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		}
		else
		{
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
			return (TRUE_BOOLEXP);
		}
		else if ((b2->sub1->type) != BOOLEXP_CONST)
		{
			free_boolexp(b2);
			return (TRUE_BOOLEXP);
		}
		else
		{
			return (b2);
		}

	default:
		return (parse_boolexp_L(pBuf, parse_player, parsing_internal));
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
				return TRUE_BOOLEXP;
			}

			b = b2;
		}
	}

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
				return TRUE_BOOLEXP;
			}

			b = b2;
		}
	}

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

	ret = parse_boolexp_E(&pBuf, player, parsing_internal);
	XFREE(pStore);
	return ret;
}