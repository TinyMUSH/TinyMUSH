/**
 * @file db_rw.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief flatfile implementation
 * @version 3.3
 * @date 2020-12-28
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
#include <fcntl.h>
#include <string.h>

extern struct object *db;
int g_version;
int g_format;
int g_flags;
extern int anum_alc_top;
int *used_attrs_table;

/**
 * @brief Get boolean subexpression from file.
 *
 * @param f			File
 * @return BOOLEXP*
 */
BOOLEXP *getboolexp1(FILE *f)
{
	BOOLEXP *b = NULL;
	char *buff = NULL, *s = NULL;
	int d = 0, anum = 0, c = getc(f);

	switch (c)
	{
	case '\n':
		ungetc(c, f);
		return TRUE_BOOLEXP;
	case EOF:
		log_write_raw(1, "ABORT! db_rw.c, unexpected EOF in boolexp in getboolexp1().\n");
		abort();
		break;
	case '(':
		b = alloc_boolexp();
		switch (c = getc(f))
		{
		case NOT_TOKEN:
			b->type = BOOLEXP_NOT;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
			{
				d = getc(f);
			}
			if (d != ')')
			{
				goto error;
			}
			return b;
		case INDIR_TOKEN:
			b->type = BOOLEXP_INDIR;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
			{
				d = getc(f);
			}
			if (d != ')')
			{
				goto error;
			}
			return b;
		case IS_TOKEN:
			b->type = BOOLEXP_IS;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
			{
				d = getc(f);
			}
			if (d != ')')
			{
				goto error;
			}
			return b;
		case CARRY_TOKEN:
			b->type = BOOLEXP_CARRY;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
			{
				d = getc(f);
			}
			if (d != ')')
			{
				goto error;
			}
			return b;
		case OWNER_TOKEN:
			b->type = BOOLEXP_OWNER;
			b->sub1 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
			{
				d = getc(f);
			}
			if (d != ')')
			{
				goto error;
			}
			return b;
		default:
			ungetc(c, f);
			b->sub1 = getboolexp1(f);
			if ((c = getc(f)) == '\n')
			{
				c = getc(f);
			}
			switch (c)
			{
			case AND_TOKEN:
				b->type = BOOLEXP_AND;
				break;
			case OR_TOKEN:
				b->type = BOOLEXP_OR;
				break;
			default:
				goto error;
			}
			b->sub2 = getboolexp1(f);
			if ((d = getc(f)) == '\n')
			{
				d = getc(f);
			}
			if (d != ')')
			{
				goto error;
			}
			return b;
		}
	case '-':
		/**
		 * obsolete NOTHING key, eat it
		 *
		 */
		while ((c = getc(f)) != '\n')
		{
			if (c == EOF)
			{
				log_write_raw(1, "ABORT! db_rw.c, unexpected EOF in getboolexp1().\n");
				abort();
			}
		}
		ungetc(c, f);
		return TRUE_BOOLEXP;
	case '"':
		ungetc(c, f);
		buff = getstring(f, 1);
		c = fgetc(f);
		if (c == EOF)
		{
			XFREE(buff);
			return TRUE_BOOLEXP;
		}
		b = alloc_boolexp();
		anum = mkattr(buff);
		if (anum <= 0)
		{
			free_boolexp(b);
			XFREE(buff);
			goto error;
		}
		XFREE(buff);
		b->thing = anum;
		/**
		 * if last character is : then this is an attribute lock. A
		 * last character of / means an eval lock
		 *
		 */
		if ((c == ':') || (c == '/'))
		{
			if (c == '/')
			{
				b->type = BOOLEXP_EVAL;
			}
			else
			{
				b->type = BOOLEXP_ATR;
			}
			b->sub1 = (BOOLEXP *)getstring(f, 1);
		}
		return b;
	default:
		/**
		 * dbref or attribute
		 *
		 */
		ungetc(c, f);
		b = alloc_boolexp();
		b->type = BOOLEXP_CONST;
		b->thing = 0;
		/**
		 * This is either an attribute, eval, or constant lock.
		 * Constant locks are of the form <num>, while attribute and
		 * eval locks are of the form <anam-or-anum>:<string> or
		 * <aname-or-anum>/<string> respectively. The characters
		 * <nl>, |, and & terminate the string.
		 *
		 */
		if (isdigit(c))
		{
			while (isdigit(c = getc(f)))
			{
				b->thing = b->thing * 10 + c - '0';
			}
		}
		else if (isalpha(c))
		{
			buff = XMALLOC(LBUF_SIZE, "buff");
			for (s = buff; ((c = getc(f)) != EOF) && (c != '\n') && (c != ':') && (c != '/'); *s++ = c)
				;
			if (c == EOF)
			{
				XFREE(buff);
				free_boolexp(b);
				goto error;
			}
			*s = '\0';
			/**
			 * Look the name up as an attribute.  If not found,
			 * create a new attribute.
			 *
			 */
			anum = mkattr(buff);
			if (anum <= 0)
			{
				free_boolexp(b);
				XFREE(buff);
				goto error;
			}
			XFREE(buff);
			b->thing = anum;
		}
		else
		{
			free_boolexp(b);
			goto error;
		}
		/**
		 * if last character is : then this is an attribute lock. A
		 * last character of / means an eval lock
		 *
		 */
		if ((c == ':') || (c == '/'))
		{
			if (c == '/')
			{
				b->type = BOOLEXP_EVAL;
			}
			else
			{
				b->type = BOOLEXP_ATR;
			}
			buff = XMALLOC(LBUF_SIZE, "buff");
			for (s = buff; ((c = getc(f)) != EOF) && (c != '\n') && (c != ')') && (c != OR_TOKEN) && (c != AND_TOKEN); *s++ = c)
				;
			if (c == EOF)
			{
				goto error;
			}
			*s++ = 0;
			b->sub1 = (BOOLEXP *)XSTRDUP(buff, "b->sub1");
			XFREE(buff);
		}
		ungetc(c, f);
		return b;
	}
error:
	log_write_raw(1, "ABORT! db_rw.c, reached error case in getboolexp1().\n");
	/**
	 * bomb out
	 *
	 */
	abort();
	return TRUE_BOOLEXP;
}

/**
 * @brief Read a boolean expression from the flat file.
 *
 * @param f			File
 * @return BOOLEXP*
 */
BOOLEXP *getboolexp(FILE *f)
{
	char c = 0;
	BOOLEXP *b = getboolexp1(f);

	if (getc(f) != '\n')
	{
		/**
		 * parse error, we lose
		 *
		 */
		log_write_raw(1, "ABORT! db_rw.c, parse error in getboolexp().\n");
		abort();
	}

	if ((c = getc(f)) != '\n')
	{
		ungetc(c, f);
	}

	return b;
}

/**
 * @brief Fix up attribute numbers from foreign muds
 *
 * @param attrnum	attribute numbers
 * @return int
 */
int unscramble_attrnum(int attrnum)
{
	switch (g_format)
	{
	case F_MUSH:
		/**
		 * TinyMUSH 2.2:  Deal with different attribute numbers.
		 *
		 */
		switch (attrnum)
		{
		case 208:
			return A_NEWOBJS;
			break;
		case 209:
			return A_LCON_FMT;
			break;
		case 210:
			return A_LEXITS_FMT;
			break;
		case 211:
			return A_PROGCMD;
			break;
		default:
			return attrnum;
		}
	default:
		return attrnum;
	}
}

/**
 * @brief Read attribute list from flat file.
 *
 * @param f				File
 * @param i				DBref
 * @param new_strings	New string
 */
void get_list(FILE *f, dbref i, int new_strings)
{
	dbref atr = NOTHING;
	int c = 0;
	char *buff = NULL;
	bool loopend = false;

	while (!loopend)
	{
		switch (c = getc(f))
		{
		case '>':
			/**
			 * read # then string
			 *
			 */
			if (mushstate.standalone)
			{
				atr = unscramble_attrnum(getref(f));
			}
			else
			{
				atr = getref(f);
			}

			if (atr > 0)
			{
				/**
				 * Store the attr
				 *
				 */
				buff = getstring(f, new_strings);
				atr_add_raw(i, atr, buff);
				XFREE(buff);
			}
			else
			{
				/**
				 * Silently discard
				 *
				 */
				XFREE(getstring(f, new_strings));
			}

			break;

		case '\n':
			/**
			 * ignore newlines. They're due to v(r).
			 *
			 */
			break;

		case '<':
			/**
			 * end of list
			 *
			 */
			c = getc(f);

			if (c != '\n')
			{
				ungetc(c, f);
				log_write_raw(1, "No line feed on object %d\n", i);
			}
			loopend = true;
			break;
		default:
			/**
			 * We've found a bad spot.  I hope things aren't too bad.
			 *
			 */
			log_write_raw(1, "Bad character '%c' when getting attributes on object %d\n", c, i);

			XFREE(getstring(f, new_strings));
		}
	}
}

/**
 * @brief Write a boolean sub-expression to the flat file.
 *
 * @param f File
 * @param b Boolean sub-expression
 */
void putbool_subexp(FILE *f, BOOLEXP *b)
{
	ATTR *va = NULL;

	switch (b->type)
	{
	case BOOLEXP_IS:
		putc('(', f);
		putc(IS_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;

	case BOOLEXP_CARRY:
		putc('(', f);
		putc(CARRY_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;

	case BOOLEXP_INDIR:
		putc('(', f);
		putc(INDIR_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;

	case BOOLEXP_OWNER:
		putc('(', f);
		putc(OWNER_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;

	case BOOLEXP_AND:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(AND_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;

	case BOOLEXP_OR:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(OR_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;

	case BOOLEXP_NOT:
		putc('(', f);
		putc(NOT_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;

	case BOOLEXP_CONST:
		fprintf(f, "%d", b->thing);
		break;

	case BOOLEXP_ATR:
		va = atr_num(b->thing);

		if (va)
		{
			fprintf(f, "%s:%s", va->name, (char *)b->sub1);
		}
		else
		{
			fprintf(f, "%d:%s\n", b->thing, (char *)b->sub1);
		}

		break;

	case BOOLEXP_EVAL:
		va = atr_num(b->thing);

		if (va)
		{
			fprintf(f, "%s/%s\n", va->name, (char *)b->sub1);
		}
		else
		{
			fprintf(f, "%d/%s\n", b->thing, (char *)b->sub1);
		}

		break;

	default:
		log_write_raw(1, "Unknown boolean type in putbool_subexp: %d\n", b->type);
	}
}

/**
 * @brief Write boolean expression to the flat file.
 *
 * @param f File
 * @param b Boolean expression
 */
void putboolexp(FILE *f, BOOLEXP *b)
{
	if (b != TRUE_BOOLEXP)
	{
		putbool_subexp(f, b);
	}

	putc('\n', f);
}

/**
 * @brief Convert foreign flags to MUSH format.
 *
 * @param flags1		Flag
 * @param flags2		Flag
 * @param flags3		Flag
 * @param thing			DBref of thing
 * @param db_format		DB Format
 * @param db_version	DB Version
 */
void upgrade_flags(FLAG *flags1, FLAG *flags2, FLAG *flags3, dbref thing, int db_format, int db_version)
{
	FLAG f1 = *flags1, f2 = *flags2, f3 = *flags3;
	FLAG newf1 = 0, newf2 = 0, newf3 = 0;

	if ((db_format == F_MUSH) && (db_version >= 3))
	{
		newf1 = f1;
		newf2 = f2;
		newf3 = 0;

		/**
		 * TinyMUSH 2.2 to 3.0 flag conversion
		 *
		 */
		if (newf1 & ROYALTY)
		{
			newf1 &= ~ROYALTY;
			newf2 |= CONTROL_OK;
		}

		if (newf2 & HAS_COMMANDS)
		{
			newf2 &= ~HAS_COMMANDS;
			newf2 |= NOBLEED;
		}

		if (newf2 & AUDITORIUM)
		{
			newf2 &= ~AUDITORIUM;
			newf2 |= ZONE_PARENT;
		}

		if (newf2 & ANSI)
		{
			newf2 &= ~ANSI;
			newf2 |= STOP_MATCH;
		}

		if (newf2 & HEAD_FLAG)
		{
			newf2 &= ~HEAD_FLAG;
			newf2 |= HAS_COMMANDS;
		}

		if (newf2 & FIXED)
		{
			newf2 &= ~FIXED;
			newf2 |= BOUNCE;
		}

		if (newf2 & STAFF)
		{
			newf2 &= STAFF;
			newf2 |= HTML;
		}

		if (newf2 & HAS_DAILY)
		{
			/**
			 * This is the unimplemented TICKLER flag.
			 *
			 */
			newf2 &= ~HAS_DAILY;
		}

		if (newf2 & GAGGED)
		{
			newf2 &= ~GAGGED;
			newf2 |= ANSI;
		}

		if (newf2 & WATCHER)
		{
			newf2 &= ~WATCHER;
			s_Powers(thing, Powers(thing) | POW_BUILDER);
		}
	}
	else if (db_format == F_MUX)
	{
		/**
		 * TinyMUX to 3.0 flag conversion
		 *
		 */
		newf1 = f1;
		newf2 = f2;
		newf3 = f3;

		if (newf2 & ZONE_PARENT)
		{
			/**
			 * This used to be an object set NO_COMMAND. We unset the flag.
			 *
			 */
			newf2 &= ~ZONE_PARENT;
		}
		else
		{
			/**
			 * And if it wasn't NO_COMMAND, then it should be COMMANDS.
			 *
			 */
			newf2 |= HAS_COMMANDS;
		}

		if (newf2 & WATCHER)
		{
			/**
			 * This used to be the COMPRESS flag, which didn't do anything.
			 *
			 */
			newf2 &= ~WATCHER;
		}

		if ((newf1 & MONITOR) && ((newf1 & TYPE_MASK) == TYPE_PLAYER))
		{
			/**
			 * Players set MONITOR should be set WATCHER as well.
			 *
			 */
			newf2 |= WATCHER;
		}
	}
	else if (db_format == F_TINYMUSH)
	{
		/**
		 * Native TinyMUSH 3.0 database. The only thing we have to do is
		 * clear the redirection flag, as nothing is ever redirected at startup.
		 *
		 */
		newf1 = f1;
		newf2 = f2;
		newf3 = f3 & ~HAS_REDIRECT;
	}

	newf2 = newf2 & ~FLOATING; /*!< this flag is now obsolete */
	*flags1 = newf1;
	*flags2 = newf2;
	*flags3 = newf3;
	return;
}

/**
 * @brief Fix things up for Exits-From-Objects
 *
 */
void efo_convert(void)
{
	int i = 0;
	dbref link = NOTHING;

	for (i = 0; i < mushstate.db_top; i++)
	{
		switch (Typeof(i))
		{
		case TYPE_PLAYER:
		case TYPE_THING:
			/**
			 * swap Exits and Link
			 *
			 */
			link = Link(i);
			s_Link(i, Exits(i));
			s_Exits(i, link);
			break;
		}
	}
}

/**
 * @brief Convert MUX-style zones to 3.0-style zones.
 *
 */
void fix_mux_zones(void)
{
	/**
	 * For all objects in the database where Zone(thing) != NOTHING, set
	 * the CONTROL_OK flag on them.
	 *
	 * For all objects in the database that are ZMOs (that have other objects
	 * zoned to them), copy the EnterLock of those objects to the ControlLock.
	 *
	 */
	int *zmarks;
	char *astr;
	zmarks = (int *)XCALLOC(mushstate.db_top, sizeof(int), "zmarks");
	for (int i = 0; i < mushstate.db_top; i++)
	{
		if (Zone(i) != NOTHING)
		{
			s_Flags2(i, Flags2(i) | CONTROL_OK);
			zmarks[Zone(i)] = 1;
		}
	}

	for (int i = 0; i < mushstate.db_top; i++)
	{
		if (zmarks[i])
		{
			astr = atr_get_raw(i, A_LENTER);

			if (astr)
			{
				atr_add_raw(i, A_LCONTROL, astr);
			}
		}
	}
	XFREE(zmarks);
}

/**
 * @brief Explode standard quotas into typed quotas
 *
 */
void fix_typed_quotas(void)
{
	/**
	 * If we have a pre-2.2 or MUX database, only the QUOTA and RQUOTA
	 * attributes  exist. For simplicity's sake, we assume that players will
	 * have the  same quotas for all types, equal to the current value. This
	 * is  going to produce incorrect values for RQUOTA; this is easily fixed
	 * by a @quota/fix done from within-game.
	 *
	 * If we have a early beta 2.2 release, we have quotas which are spread
	 * out over ten attributes. We're going to have to grab those, make the
	 * new quotas, and then delete the old attributes.
	 *
	 */
	char *qbuf = NULL, *rqbuf = NULL;
	char *s = XMALLOC(LBUF_SIZE, "s");

	for (int i = 0; i < mushstate.db_top; i++)
	{
		if (isPlayer(i))
		{
			qbuf = atr_get_raw(i, A_QUOTA);
			rqbuf = atr_get_raw(i, A_RQUOTA);

			if (!qbuf || !*qbuf)
			{
				qbuf = (char *)"1";
			}

			if (!rqbuf || !*rqbuf)
			{
				rqbuf = (char *)"0";
			}

			XSNPRINTF(s, LBUF_SIZE, "%s %s %s %s %s", qbuf, qbuf, qbuf, qbuf, qbuf);
			atr_add_raw(i, A_QUOTA, s);
			XSNPRINTF(s, LBUF_SIZE, "%s %s %s %s %s", rqbuf, rqbuf, rqbuf, rqbuf, rqbuf);
			atr_add_raw(i, A_RQUOTA, s);
		}
	}
	XFREE(s);
}

/**
 * @brief Read a flatfile
 *
 * @param f				File
 * @param db_format		DB Format
 * @param db_version	DB Version
 * @param db_flags		DB Flags
 * @return dbref
 */
dbref db_read_flatfile(FILE *f, int *db_format, int *db_version, int *db_flags)
{
	dbref i = NOTHING, anum = NOTHING;
	char ch = 0;
	char *tstr = NULL, *s = NULL;
	int header_gotten = 0, size_gotten = 0, nextattr_gotten = 0;
	int read_attribs = 1, read_name = 1, read_zone = 0, read_link = 0, read_key = 1, read_parent = 0;
	int read_extflags = 0, read_3flags = 0, read_money = 1, read_timestamps = 0, read_createtime = 0, read_new_strings = 0;
	int read_powers = 0;
	int has_typed_quotas = 0, has_visual_attrs = 0;
	int deduce_version = 1, deduce_name = 1, deduce_zone = 1;
	int aflags = 0, f1 = 0, f2 = 0, f3 = 0;
	BOOLEXP *tempbool = NULL;
	time_t tmptime = 0L;
	struct timeval obj_time;

	g_format = F_UNKNOWN;
	g_version = 0;
	g_flags = 0;

	if (mushstate.standalone)
	{
		log_write_raw(1, "Reading ");
	}

	db_free();

	for (i = 0;; i++)
	{
		if (tstr)
		{
			XFREE(tstr);
		}
		if (mushstate.standalone && !(i % 100))
		{
			log_write_raw(1, ".");
		}

		switch (ch = getc(f))
		{
		case '-':
			/**
			 * Misc tag
			 *
			 */
			switch (ch = getc(f))
			{
			case 'R':
				/**
				 * Record number of players
				 *
				 */
				mushstate.record_players = getref(f);
				break;

			default:
				XFREE(getstring(f, 0));
			}

			break;

		case '+':
			/**
			 * MUX and MUSH header, 2nd char selects type
			 *
			 */
			ch = getc(f);

			if ((ch == 'V') || (ch == 'X') || (ch == 'T'))
			{
				/**
				 * The following things are common across 2.x, MUX, and 3.0.
				 *
				 */
				if (header_gotten)
				{
					if (mushstate.standalone)
					{
						log_write_raw(1, "\nDuplicate MUSH version header entry at object %d, ignored.\n", i);
					}

					if (tstr)
					{
						XFREE(tstr);
					}
					tstr = getstring(f, 0);
					break;
				}

				header_gotten = 1;
				deduce_version = 0;
				g_version = getref(f);

				/**
				 * Otherwise extract feature flags
				 *
				 */
				if (g_version & V_GDBM)
				{
					read_attribs = 0;
					read_name = !(g_version & V_ATRNAME);
				}

				read_zone = (g_version & V_ZONE);
				read_link = (g_version & V_LINK);
				read_key = !(g_version & V_ATRKEY);
				read_parent = (g_version & V_PARENT);
				read_money = !(g_version & V_ATRMONEY);
				read_extflags = (g_version & V_XFLAGS);
				has_typed_quotas = (g_version & V_TQUOTAS);
				read_timestamps = (g_version & V_TIMESTAMPS);
				has_visual_attrs = (g_version & V_VISUALATTRS);
				read_createtime = (g_version & V_CREATETIME);
				g_flags = g_version & ~V_MASK;
				deduce_name = 0;
				deduce_version = 0;
				deduce_zone = 0;
			}

			/**
			 * More generic switch.
			 *
			 */
			switch (ch)
			{
			case 'T':
				/**
				 * 3.0 VERSION
				 *
				 */
				g_format = F_TINYMUSH;
				read_3flags = (g_version & V_3FLAGS);
				read_powers = (g_version & V_POWERS);
				read_new_strings = (g_version & V_QUOTED);
				g_version &= V_MASK;
				break;

			case 'V':
				/**
				 * 2.0 VERSION
				 *
				 */
				g_format = F_MUSH;
				g_version &= V_MASK;
				break;

			case 'X':
				/**
				 * MUX VERSION
				 *
				 */
				g_format = F_MUX;
				read_3flags = (g_version & V_3FLAGS);
				read_powers = (g_version & V_POWERS);
				read_new_strings = (g_version & V_QUOTED);
				g_version &= V_MASK;
				break;

			case 'S':
				/**
				 * SIZE
				 *
				 */
				if (size_gotten)
				{
					if (mushstate.standalone)
					{
						log_write_raw(1, "\nDuplicate size entry at object %d, ignored.\n", i);
					}

					if (tstr)
					{
						XFREE(tstr);
					}
					tstr = getstring(f, 0);
				}
				else
				{
					mushstate.min_size = getref(f);
				}

				size_gotten = 1;
				break;

			case 'A':
				/**
				 * USER-NAMED ATTRIBUTE
				 *
				 */
				anum = getref(f);

				if (tstr)
				{
					XFREE(tstr);
				}
				tstr = getstring(f, read_new_strings);

				if (isdigit(*tstr))
				{
					aflags = 0;

					while (isdigit(*tstr))
						aflags = (aflags * 10) + (*tstr++ - '0');

					tstr++; /*!< skip ':' */

					if (!has_visual_attrs)
					{
						/**
						 * If not AF_ODARK, is AF_VISUAL. Strip AF_ODARK.
						 *
						 */
						if (aflags & AF_ODARK)
						{
							aflags &= ~AF_ODARK;
						}
						else
						{
							aflags |= AF_VISUAL;
						}
					}
				}
				else
				{
					aflags = mushconf.vattr_flags;
				}

				vattr_define((char *)tstr, anum, aflags);
				break;

			case 'F':
				/**
				 * OPEN USER ATTRIBUTE SLOT
				 *
				 */
				anum = getref(f);
				break;

			case 'N':
				/**
				 * NEXT ATTR TO ALLOC WHEN NO FREELIST
				 *
				 */
				if (nextattr_gotten)
				{
					if (mushstate.standalone)
					{
						log_write_raw(1, "\nDuplicate next free vattr entry at object %d, ignored.\n", i);
					}

					if (tstr)
					{
						XFREE(tstr);
					}
					tstr = getstring(f, 0);
				}
				else
				{
					mushstate.attr_next = getref(f);
					nextattr_gotten = 1;
				}

				break;

			default:
				if (mushstate.standalone)
				{
					log_write_raw(1, "\nUnexpected character '%c' in MUSH header near object #%d, ignored.\n", ch, i);
				}

				if (tstr)
				{
					XFREE(tstr);
				}
				tstr = getstring(f, 0);
			}

			break;

		case '!':
			/**
			 * MUX and MUSH entries
			 *
			 */
			if (deduce_version)
			{
				g_format = F_TINYMUSH;
				g_version = 1;
				deduce_name = 0;
				deduce_zone = 0;
				deduce_version = 0;
			}
			else if (deduce_zone)
			{
				deduce_zone = 0;
				read_zone = 0;
			}

			i = getref(f);
			db_grow(i + 1);

			if (mushconf.lag_check_clk)
			{
				obj_time.tv_sec = obj_time.tv_usec = 0;
				db[i].cpu_time_used.tv_sec = obj_time.tv_sec;
				db[i].cpu_time_used.tv_usec = obj_time.tv_usec;
			}

			s_StackCount(i, 0);
			s_VarsCount(i, 0);
			s_StructCount(i, 0);
			s_InstanceCount(i, 0);

			if (read_name)
			{
				if (tstr)
				{
					XFREE(tstr);
				}
				tstr = getstring(f, read_new_strings);

				if (deduce_name)
				{
					if (isdigit(*tstr))
					{
						read_name = 0;
						s_Location(i, (int)strtol(tstr, (char **)NULL, 10));
					}
					else
					{
						s_Name(i, (char *)tstr);
						s_Location(i, getref(f));
					}

					deduce_name = 0;
				}
				else
				{
					s_Name(i, (char *)tstr);
					s_Location(i, getref(f));
				}
			}
			else
			{
				s_Location(i, getref(f));
			}

			if (read_zone)
			{
				s_Zone(i, getref(f));
			}

			/**
			 * CONTENTS and EXITS
			 *
			 */
			s_Contents(i, getref(f));

			/**
			 * EXITS
			 *
			 */
			s_Exits(i, getref(f));

			/**
			 * LINK
			 *
			 */
			if (read_link)
			{
				s_Link(i, getref(f));
			}
			else
			{
				s_Link(i, NOTHING);
			}

			/**
			 * NEXT
			 *
			 */
			s_Next(i, getref(f));

			/**
			 * LOCK
			 *
			 */
			if (read_key)
			{
				tempbool = getboolexp(f);
				s = unparse_boolexp_quiet(1, tempbool);
				atr_add_raw(i, A_LOCK, s);
				XFREE(s);
				free_boolexp(tempbool);
			}

			/**
			 * OWNER
			 *
			 */
			s_Owner(i, getref(f));

			/**
			 * PARENT
			 *
			 */
			if (read_parent)
			{
				s_Parent(i, getref(f));
			}
			else
			{
				s_Parent(i, NOTHING);
			}

			/**
			 * PENNIES
			 *
			 */
			if (read_money)
			{
				s_Pennies(i, getref(f));
			}

			/**
			 * FLAGS
			 *
			 */
			f1 = getref(f);

			if (read_extflags)
			{
				f2 = getref(f);
			}
			else
			{
				f2 = 0;
			}

			if (read_3flags)
			{
				f3 = getref(f);
			}
			else
			{
				f3 = 0;
			}

			upgrade_flags(&f1, &f2, &f3, i, g_format, g_version);
			s_Flags(i, f1);
			s_Flags2(i, f2);
			s_Flags3(i, f3);

			if (read_powers)
			{
				f1 = getref(f);
				f2 = getref(f);
				s_Powers(i, f1);
				s_Powers2(i, f2);
			}

			if (read_timestamps)
			{
				tmptime = (time_t)getlong(f);
				s_AccessTime(i, tmptime);
				tmptime = (time_t)getlong(f);
				s_ModTime(i, tmptime);
			}
			else
			{
				AccessTime(i) = ModTime(i) = time(NULL);
			}

			if (read_createtime)
			{
				tmptime = (time_t)getlong(f);
				s_CreateTime(i, tmptime);
			}
			else
			{
				s_CreateTime(i, AccessTime(i));
			}

			/**
			 * ATTRIBUTES
			 *
			 */
			if (read_attribs)
			{
				get_list(f, i, read_new_strings);
			}

			/**
			 * check to see if it's a player
			 *
			 */
			if (Typeof(i) == TYPE_PLAYER)
			{
				c_Connected(i);
			}

			break;

		case '*':
			/**
			 * EOF marker
			 *
			 */
			if (tstr)
			{
				XFREE(tstr);
			}
			tstr = getstring(f, 0);

			if (strcmp(tstr, "**END OF DUMP***"))
			{
				if (mushstate.standalone)
				{
					log_write_raw(1, "\nBad EOF marker at object #%d\n", i);
				}

				return -1;
			}
			else
			{
				if (mushstate.standalone)
				{
					log_write_raw(1, "\n");
				}

				*db_version = g_version;
				*db_format = g_format;
				*db_flags = g_flags;

				if (!has_typed_quotas)
				{
					fix_typed_quotas();
				}

				if (g_format == F_MUX)
				{
					fix_mux_zones();
				}

				return mushstate.db_top;
			}

		default:
			if (mushstate.standalone)
			{
				log_write_raw(1, "\nIllegal character '%c' near object #%d\n", ch, i);
			}

			return -1;
		}
		if (tstr)
		{
			XFREE(tstr);
		}
	}
	if (tstr)
	{
		XFREE(tstr);
	}
}

/**
 * @brief Read a DB
 *
 * @return int
 */
int db_read(void)
{
	UDB_DATA key, data;
	int *c = NULL, vattr_flags = 0, i = 0, j = 0, blksize = 0, num = 0;
	char *s = NULL;
	struct timeval obj_time;

	/**
	 * Fetch the database info
	 *
	 */
	key.dptr = "TM3";
	key.dsize = strlen("TM3") + 1;
	data = db_get(key, DBTYPE_DBINFO);

	if (!data.dptr)
	{
		log_write(LOG_ALWAYS, "DBR", "LOAD", "Could not open main record");
		return -1;
	}

	/**
	 * Unroll the data returned
	 *
	 */
	c = data.dptr;
	XMEMCPY((void *)&mushstate.min_size, (void *)c, sizeof(int));
	c++;
	XMEMCPY((void *)&mushstate.attr_next, (void *)c, sizeof(int));
	c++;
	XMEMCPY((void *)&mushstate.record_players, (void *)c, sizeof(int));
	c++;
	XMEMCPY((void *)&mushstate.moduletype_top, (void *)c, sizeof(int));
	XFREE(data.dptr);

	/**
	 * Load the attribute numbers
	 *
	 */
	blksize = ATRNUM_BLOCK_SIZE;

	for (i = 0; i <= ENTRY_NUM_BLOCKS(mushstate.attr_next, blksize); i++)
	{
		key.dptr = &i;
		key.dsize = sizeof(int);
		data = db_get(key, DBTYPE_ATRNUM);

		if (data.dptr)
		{
			/**
			 * Unroll the data into flags and name
			 *
			 */
			s = data.dptr;

			while ((s - (char *)data.dptr) < data.dsize)
			{
				XMEMCPY((void *)&j, (void *)s, sizeof(int));
				s += sizeof(int);
				XMEMCPY((void *)&vattr_flags, (void *)s, sizeof(int));
				s += sizeof(int);
				vattr_define(s, j, vattr_flags);
				s = strchr((const char *)s, '\0');

				if (!s)
				{

					/**
					 * Houston, we have a problem
					 *
					 */
					log_write(LOG_ALWAYS, "DBR", "LOAD", "Error reading attribute number %d", j + ENTRY_BLOCK_STARTS(i, blksize));
				}

				s++;
			}

			XFREE(data.dptr);
		}
	}

	/**
	 * Load the object structures
	 *
	 */
	if (mushstate.standalone)
	{
		log_write(LOG_ALWAYS, "DBR", "LOAD", "Reading ");
	}

	blksize = OBJECT_BLOCK_SIZE;

	for (i = 0; i <= ENTRY_NUM_BLOCKS(mushstate.min_size, blksize); i++)
	{
		key.dptr = &i;
		key.dsize = sizeof(int);
		data = db_get(key, DBTYPE_OBJECT);

		if (data.dptr)
		{

			/**
			 * Unroll the data into objnum and object
			 *
			 */
			s = data.dptr;

			while ((s - (char *)data.dptr) < data.dsize)
			{
				XMEMCPY((void *)&num, (void *)s, sizeof(int));
				s += sizeof(int);
				db_grow(num + 1);

				if (mushstate.standalone && !(num % 100))
				{
					log_write_raw(1, ".");
				}

				/**
				 * We read the entire object structure in and copy it into place
				 *
				 */
				XMEMCPY((void *)&(db[num]), (void *)s, sizeof(DUMPOBJ));
				s += sizeof(DUMPOBJ);

				if (mushconf.lag_check_clk)
				{
					obj_time.tv_sec = obj_time.tv_usec = 0;
					db[num].cpu_time_used.tv_sec = obj_time.tv_sec;
					db[num].cpu_time_used.tv_usec = obj_time.tv_usec;
				}

				s_StackCount(num, 0);
				s_VarsCount(num, 0);
				s_StructCount(num, 0);
				s_InstanceCount(num, 0);
				/**
				 * Check to see if it's a player
				 *
				 */
				if (Typeof(num) == TYPE_PLAYER)
				{
					c_Connected(num);
				}

				s_Clean(num);
			}

			XFREE(data.dptr);
		}
	}

	if (!mushstate.standalone)
	{
		load_player_names();
	}

	if (mushstate.standalone)
	{
		log_write_raw(1, "\n");
	}

	return (0);
}

/**
 * @brief Write an object to a DB
 *
 * @param f			File
 * @param i			DBref of object
 * @param db_format	DB Format
 * @param flags		Flags
 * @param n_atrt	Number of attributes
 * @return int
 */
int db_write_object_out(FILE *f, dbref i, int db_format __attribute__((unused)), int flags, int *n_atrt)
{
	ATTR *a = NULL;
	char *got = NULL, *as = NULL;
	dbref aowner = NOTHING;
	int ca = 0, aflags = 0, alen = 0, save = 0, j = 0, changed = 0;
	BOOLEXP *tempbool = NULL;

	if (Going(i))
	{
		return (0);
	}

	fprintf(f, "!%d\n", i);

	if (!(flags & V_ATRNAME))
	{
		putstring(f, Name(i));
	}

	putref(f, Location(i));

	if (flags & V_ZONE)
	{
		putref(f, Zone(i));
	}

	putref(f, Contents(i));
	putref(f, Exits(i));

	if (flags & V_LINK)
	{
		putref(f, Link(i));
	}

	putref(f, Next(i));

	if (!(flags & V_ATRKEY))
	{
		got = atr_get(i, A_LOCK, &aowner, &aflags, &alen);
		tempbool = parse_boolexp(GOD, got, 1);
		XFREE(got);
		putboolexp(f, tempbool);

		if (tempbool)
		{
			free_boolexp(tempbool);
		}
	}

	putref(f, Owner(i));

	if (flags & V_PARENT)
	{
		putref(f, Parent(i));
	}

	if (!(flags & V_ATRMONEY))
	{
		putref(f, Pennies(i));
	}

	putref(f, Flags(i));

	if (flags & V_XFLAGS)
	{
		putref(f, Flags2(i));
	}

	if (flags & V_3FLAGS)
	{
		putref(f, Flags3(i));
	}

	if (flags & V_POWERS)
	{
		putref(f, Powers(i));
		putref(f, Powers2(i));
	}

	if (flags & V_TIMESTAMPS)
	{
		putlong(f, AccessTime(i));
		putlong(f, ModTime(i));
	}

	if (flags & V_CREATETIME)
	{
		putlong(f, CreateTime(i));
	}

	/**
	 * write the attribute list
	 *
	 */
	changed = 0;

	for (ca = atr_head(i, &as); ca; ca = atr_next(&as))
	{
		save = 0;

		if (!mushstate.standalone)
		{
			a = atr_num(ca);

			if (a)
			{
				j = a->number;
			}
			else
			{
				j = -1;
			}
		}
		else
		{
			j = ca;
		}

		if (j > 0)
		{
			switch (j)
			{
			case A_NAME:
				if (flags & V_ATRNAME)
				{
					save = 1;
				}

				break;

			case A_LOCK:
				if (flags & V_ATRKEY)
				{
					save = 1;
				}

				break;

			case A_LIST:
			case A_MONEY:
				break;

			default:
				save = 1;
			}
		}

		if (save)
		{
			got = atr_get_raw(i, j);

			if (used_attrs_table != NULL)
			{
				fprintf(f, ">%d\n", used_attrs_table[j]);

				if (used_attrs_table[j] != j)
				{
					changed = 1;
					*n_atrt += 1;
				}
			}
			else
			{
				fprintf(f, ">%d\n", j);
			}

			putstring(f, got);
		}
	}

	fprintf(f, "<\n");
	return (changed);
}

/**
 * @brief Write a db to Flat File
 *
 * @param f			File
 * @param format	Format of the flatfile
 * @param version	Version of the flatfile
 * @return dbref
 */
dbref db_write_flatfile(FILE *f, int format, int version)
{
	dbref i = NOTHING;
	int flags = 0;
	VATTR *vp = NULL;
	int n = 0, end = 0, ca = 0, n_oldtotal = 0, n_deleted = 0, n_renumbered = 0;
	int n_objt = 0, n_atrt = 0, anxt = 0, dbclean = (version & V_DBCLEAN) ? 1 : 0;
	int *old_attrs_table = NULL;
	char *as = NULL;

	al_store();
	version &= ~V_DBCLEAN;

	switch (format)
	{
	case F_TINYMUSH:
		flags = version;
		break;

	default:
		log_write_raw(1, "Can only write TinyMUSH 3 format.\n");
		return -1;
	}

	if (mushstate.standalone)
	{
		log_write_raw(1, "Writing ");
	}

	/**
	 * Attribute cleaning, if standalone.
	 *
	 */
	if (mushstate.standalone && dbclean)
	{
		used_attrs_table = (int *)XCALLOC(mushstate.attr_next, sizeof(int), "used_attrs_table");
		old_attrs_table = (int *)XCALLOC(mushstate.attr_next, sizeof(int), "old_attrs_table");
		n_oldtotal = mushstate.attr_next;
		n_deleted = n_renumbered = n_objt = n_atrt = 0;

		/**
		 * Non-user defined attributes are always considered used.
		 *
		 */
		for (n = 0; n < A_USER_START; n++)
		{
			used_attrs_table[n] = n;
		}

		/**
		 * Walk the database. Mark all the attribute numbers in use.
		 *
		 */
		atr_push();
		for (i = 0; i < mushstate.db_top; i++)
		{
			for (ca = atr_head(i, &as); ca; ca = atr_next(&as))
			{
				used_attrs_table[ca] = old_attrs_table[ca] = ca;
			}
		}
		atr_pop();

		/**
		 * Count up how many attributes we're deleting.
		 *
		 */
		vp = vattr_first();

		while (vp)
		{
			if (used_attrs_table[vp->number] == 0)
			{
				n_deleted++;
			}

			vp = vattr_next(vp);
		}

		/**
		 * Walk the table we've created of used statuses. When we find
		 * free slots, walk backwards to the first used slot at the end of the
		 * table. Write the number of the free slot into that used slot. Keep
		 * a mapping of what things used to be.
		 *
		 */
		for (n = A_USER_START, end = mushstate.attr_next - 1; (n < mushstate.attr_next) && (n < end); n++)
		{
			if (used_attrs_table[n] == 0)
			{
				while ((end > n) && (used_attrs_table[end] == 0))
				{
					end--;
				}

				if (end > n)
				{
					old_attrs_table[n] = end;
					used_attrs_table[end] = used_attrs_table[n] = n;
					end--;
				}
			}
		}

		/**
		 * Count up our renumbers.
		 *
		 */
		for (n = A_USER_START; n < mushstate.attr_next; n++)
		{
			if ((used_attrs_table[n] != n) && (used_attrs_table[n] != 0))
			{
				vp = (VATTR *)anum_get(n);

				if (vp)
				{
					n_renumbered++;
				}
			}
		}

		/**
		 * The new end of the attribute table is the first thing
		 * we've renumbered.
		 *
		 */
		for (anxt = A_USER_START; ((anxt == used_attrs_table[anxt]) && (anxt < mushstate.attr_next)); anxt++)
			;
	}
	else
	{
		used_attrs_table = NULL;
		anxt = mushstate.attr_next;
	}

	/**
	 * Write database information. TinyMUSH 2 wrote '+V', MUX wrote '+X',
	 * 3.0 writes '+T'.
	 *
	 */
	fprintf(f, "+T%d\n+S%d\n+N%d\n", flags, mushstate.db_top, anxt);
	fprintf(f, "-R%d\n", mushstate.record_players);

	/**
	 * Dump user-named attribute info
	 *
	 */
	if (mushstate.standalone && dbclean)
	{
		for (i = A_USER_START; i < anxt; i++)
		{
			if (used_attrs_table[i] == 0)
			{
				continue;
			}

			vp = (VATTR *)anum_get(old_attrs_table[i]);

			if (vp)
			{
				if (!(vp->flags & AF_DELETED))
				{
					fprintf(f, "+A%d\n\"%d:%s\"\n", i, vp->flags, vp->name);
				}
			}
		}
	}
	else
	{
		vp = vattr_first();

		while (vp != NULL)
		{
			if (!(vp->flags & AF_DELETED))
			{
				fprintf(f, "+A%d\n\"%d:%s\"\n", vp->number, vp->flags, vp->name);
			}

			vp = vattr_next(vp);
		}
	}

	/**
	 * Dump object and attribute info
	 *
	 */
	n_objt = n_atrt = 0;
	for (i = 0; i < mushstate.db_top; i++)
	{
		if (mushstate.standalone && !(i % 100))
		{
			log_write_raw(1, ".");
		}

		n_objt += db_write_object_out(f, i, format, flags, &n_atrt);
	}
	fputs("***END OF DUMP***\n", f);
	fflush(f);

	if (mushstate.standalone)
	{
		log_write_raw(1, "\n");

		if (dbclean)
		{
			if (n_objt)
			{
				log_write_raw(1, "Cleaned %d attributes (now %d): %d deleted, %d renumbered (%d objects and %d individual attrs touched).\n", n_oldtotal, anxt, n_deleted, n_renumbered, n_objt, n_atrt);
			}
			else if (n_deleted || n_renumbered)
			{
				log_write_raw(1, "Cleaned %d attributes (now %d): %d deleted, %d renumbered (no objects touched).\n", n_oldtotal, anxt, n_deleted, n_renumbered);
			}

			XFREE(used_attrs_table);
			XFREE(old_attrs_table);
		}
	}

	return (mushstate.db_top);
}

/**
 * @brief Write DB to file
 *
 * @return dbref
 */
dbref db_write(void)
{
	VATTR *vp = NULL;
	UDB_DATA key, data;
	int *c = NULL, blksize = 0, num = 0, i = 0, j = 0, k = 0, dirty = 0, len = 0;
	char *s = NULL;

	al_store();

	if (mushstate.standalone)
	{
		log_write_raw(1, "Writing ");
	}

	/**
	 * Lock the database
	 *
	 */
	db_lock();

	/**
	 * Write database information
	 *
	 */
	i = mushstate.attr_next;

	/**
	 * Roll up various paramaters needed for startup into one record.
	 * This should be the only data record of its type
	 *
	 */
	c = data.dptr = (int *)XMALLOC(4 * sizeof(int), "c");
	XMEMCPY((void *)c, (void *)&mushstate.db_top, sizeof(int));
	c++;
	XMEMCPY((void *)c, (void *)&i, sizeof(int));
	c++;
	XMEMCPY((void *)c, (void *)&mushstate.record_players, sizeof(int));
	c++;
	XMEMCPY((void *)c, (void *)&mushstate.moduletype_top, sizeof(int));

	/**
	 * "TM3" is our unique key
	 *
	 */
	key.dptr = "TM3";
	key.dsize = strlen("TM3") + 1;
	data.dsize = 4 * sizeof(int);
	db_put(key, data, DBTYPE_DBINFO);
	XFREE(data.dptr);

	/**
	 * Dump user-named attribute info
	 *
	 * First, calculate the number of attribute entries we can fit in a
	 * block, allowing for some minor DBM key overhead. This should not
	 * change unless the size of VNAME_SIZE or LBUF_SIZE changes, in
	 * which case you'd have to reload anyway
	 *
	 */
	blksize = ATRNUM_BLOCK_SIZE;

	/**
	 * Step through the attribute number array, writing stuff in 'num'
	 * sized chunks
	 *
	 */
	data.dptr = (char *)XMALLOC(ATRNUM_BLOCK_BYTES, "data.dptr");

	for (i = 0; i <= ENTRY_NUM_BLOCKS(mushstate.attr_next, blksize); i++)
	{
		dirty = 0;
		num = 0;
		s = data.dptr;

		for (j = ENTRY_BLOCK_STARTS(i, blksize); (j <= ENTRY_BLOCK_ENDS(i, blksize)) && (j < mushstate.attr_next); j++)
		{
			if (j < A_USER_START)
			{
				continue;
			}

			vp = (VATTR *)anum_table[j];

			if (vp && !(vp->flags & AF_DELETED))
			{
				if (!mushstate.standalone)
				{
					if (vp->flags & AF_DIRTY)
					{
						/**
						 * Only write the dirty attribute numbers and clear the flag
						 *
						 */
						vp->flags &= ~AF_DIRTY;
						dirty = 1;
					}
				}
				else
				{
					dirty = 1;
				}

				num++;
			}
		}

		if (!num)
		{
			/**
			 * No valid attributes in this block, delete it
			 *
			 */
			key.dptr = &i;
			key.dsize = sizeof(int);
			db_del(key, DBTYPE_ATRNUM);
		}

		if (dirty)
		{
			/**
			 * Something is dirty in this block, write all of the
			 * attribute numbers in this block
			 *
			 */
			for (j = 0; (j < blksize) && ((ENTRY_BLOCK_STARTS(i, blksize) + j) < mushstate.attr_next); j++)
			{
				/**
				 * j is an offset of attribute numbers into
				 * the current block
				 *
				 */
				if ((ENTRY_BLOCK_STARTS(i, blksize) + j) < A_USER_START)
				{
					continue;
				}

				vp = (VATTR *)anum_table[ENTRY_BLOCK_STARTS(i, blksize) + j];

				if (vp && !(vp->flags & AF_DELETED))
				{
					len = strlen(vp->name) + 1;
					XMEMCPY((void *)s, (void *)&vp->number, sizeof(int));
					s += sizeof(int);
					XMEMCPY((void *)s, (void *)&vp->flags, sizeof(int));
					s += sizeof(int);
					XMEMCPY((void *)s, (void *)vp->name, len);
					s += len;
				}
			}

			/**
			 * Write the block: Block number is our key
			 *
			 */
			key.dptr = &i;
			key.dsize = sizeof(int);
			data.dsize = s - (char *)data.dptr;
			db_put(key, data, DBTYPE_ATRNUM);
		}
	}

	XFREE(data.dptr);

	/**
	 * Dump object structures using the same block-based method we use to
	 * dump attribute numbers
	 *
	 */
	blksize = OBJECT_BLOCK_SIZE;

	/**
	 * Step through the object structure array, writing stuff in 'num'
	 * sized chunks
	 *
	 */
	data.dptr = (char *)XMALLOC(OBJECT_BLOCK_BYTES, "data.dptr");

	for (i = 0; i <= ENTRY_NUM_BLOCKS(mushstate.db_top, blksize); i++)
	{
		dirty = 0;
		num = 0;
		s = data.dptr;

		for (j = ENTRY_BLOCK_STARTS(i, blksize); (j <= ENTRY_BLOCK_ENDS(i, blksize)) && (j < mushstate.db_top); j++)
		{
			if (mushstate.standalone && !(j % 100))
			{
				log_write_raw(1, ".");
			}

			/**
			 * We assume you always do a dbck before dump, and
			 * Going objects are really destroyed!
			 *
			 */
			if (!Going(j))
			{
				if (!mushstate.standalone)
				{
					if (Flags3(j) & DIRTY)
					{
						/**
						 * Only write the dirty objects and clear the flag
						 *
						 */
						s_Clean(j);
						dirty = 1;
					}
				}
				else
				{
					dirty = 1;
				}

				num++;
			}
		}

		if (!num)
		{
			/**
			 * No valid objects in this block, delete it
			 *
			 */
			key.dptr = &i;
			key.dsize = sizeof(int);
			db_del(key, DBTYPE_OBJECT);
		}

		if (dirty)
		{
			/**
			 * Something is dirty in this block, write all of the
			 * objects in this block
			 *
			 */
			for (j = 0; (j < blksize) && ((ENTRY_BLOCK_STARTS(i, blksize) + j) < mushstate.db_top); j++)
			{
				/**
				 * j is an offset of object numbers into the current block
				 *
				 */
				k = ENTRY_BLOCK_STARTS(i, blksize) + j;

				if (!Going(k))
				{
					XMEMCPY((void *)s, (void *)&k, sizeof(int));
					s += sizeof(int);
					XMEMCPY((void *)s, (void *)&(db[k]), sizeof(DUMPOBJ));
					s += sizeof(DUMPOBJ);
				}
			}

			/**
			 * Write the block: Block number is our key
			 *
			 */
			key.dptr = &i;
			key.dsize = sizeof(int);
			data.dsize = s - (char *)data.dptr;
			db_put(key, data, DBTYPE_OBJECT);
		}
	}

	XFREE(data.dptr);

	/**
	 * Unlock the database
	 *
	 */
	db_unlock();

	if (mushstate.standalone)
	{
		log_write_raw(1, "\n");
	}

	return (mushstate.db_top);
}

/**
 * @brief Open a file pointer for a module to use when writing a flatfile
 *
 * @param filename	Filename
 * @param wrflag	Open for write or read access.
 * @return FILE*
 */
FILE *db_module_flatfile(char *filename, bool wrflag)
{
	FILE *f = NULL;

	if (wrflag)
	{
		f = tf_fopen(filename, O_WRONLY | O_CREAT | O_TRUNC);
		log_write(LOG_ALWAYS, "DMP", "DUMP", "Writing db: %s", filename);
	}
	else
	{
		f = tf_fopen(filename, O_RDONLY);
		log_write(LOG_ALWAYS, "INI", "LOAD", "Loading db: %s", filename);
	}

	if (f != NULL)
	{
		return f;
	}
	else
	{
		log_perror("DMP", "FAIL", "Opening flatfile", filename);
		return NULL;
	}
}
