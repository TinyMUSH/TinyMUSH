/* unparse.c - convert boolexps to printable form */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"	/* required by mudconf */
#include "mushconf.h"	/* required by code */
#include "db.h"			/* required by externs */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by code */
#include "stringutil.h" /* required by code */

/*
 * Boolexp decompile formats
 */

#define F_EXAMINE 1	  /*        \
					   * Normal \
					   */
#define F_QUIET 2	  /*                     \
					   * Binary for db dumps \
					   */
#define F_DECOMPILE 3 /*                   \
					   * @decompile output \
					   */
#define F_FUNCTION 4  /*                 \
					   * [lock()] output \
					   */

/*
 * Take a dbref (loc) and generate a string.  -1, -3, or (#loc) Note, this
 * will give players object numbers of stuff they don't control, but it's
 * only internal currently, so it's not a problem.
 */

char *unparse_object_quiet(__attribute__((unused)) dbref player, dbref loc)
{
	static char buf[SBUF_SIZE]; // XXX Should return a buffer instead of a static pointer

	switch (loc)
	{
	case NOTHING:
		return (char *)"-1";

	case HOME:
		return (char *)"-3";

	default:
		XSPRINTF(buf, "(#%d)", loc);
		return buf;
	}
}

char boolexp_buf[LBUF_SIZE];

char *buftop;

void unparse_boolexp1(dbref player, BOOLEXP *b, char outer_type, int format)
{
	ATTR *ap;
	char sep_ch;
	char *buff;

	if (b == TRUE_BOOLEXP)
	{
		if (format == F_EXAMINE)
		{
			SAFE_LB_STR((char *)"*UNLOCKED*", boolexp_buf, &buftop);
		}

		return;
	}

	switch (b->type)
	{
	case BOOLEXP_AND:
		if (outer_type == BOOLEXP_NOT)
		{
			SAFE_LB_CHR('(', boolexp_buf, &buftop);
		}

		unparse_boolexp1(player, b->sub1, b->type, format);
		SAFE_LB_CHR(AND_TOKEN, boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub2, b->type, format);

		if (outer_type == BOOLEXP_NOT)
		{
			SAFE_LB_CHR(')', boolexp_buf, &buftop);
		}

		break;

	case BOOLEXP_OR:
		if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND)
		{
			SAFE_LB_CHR('(', boolexp_buf, &buftop);
		}

		unparse_boolexp1(player, b->sub1, b->type, format);
		SAFE_LB_CHR(OR_TOKEN, boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub2, b->type, format);

		if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND)
		{
			SAFE_LB_CHR(')', boolexp_buf, &buftop);
		}

		break;

	case BOOLEXP_NOT:
		SAFE_LB_CHR('!', boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub1, b->type, format);
		break;

	case BOOLEXP_INDIR:
		SAFE_LB_CHR(INDIR_TOKEN, boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub1, b->type, format);
		break;

	case BOOLEXP_IS:
		SAFE_LB_CHR(IS_TOKEN, boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub1, b->type, format);
		break;

	case BOOLEXP_CARRY:
		SAFE_LB_CHR(CARRY_TOKEN, boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub1, b->type, format);
		break;

	case BOOLEXP_OWNER:
		SAFE_LB_CHR(OWNER_TOKEN, boolexp_buf, &buftop);
		unparse_boolexp1(player, b->sub1, b->type, format);
		break;

	case BOOLEXP_CONST:
		if (!mudstate.standalone)
		{
			switch (format)
			{
			case F_QUIET:
				/*
		 * Quiet output - for dumps and internal use.
		 * Always #Num
		 */
				SAFE_LB_STR((char *)unparse_object_quiet(player, b->thing), boolexp_buf, &buftop);
				break;

			case F_EXAMINE:
				/*
		 * Examine output - informative. *
		 * Name(#Num) or Name
		 */
				buff = unparse_object(player, b->thing, 0);
				SAFE_LB_STR(buff, boolexp_buf, &buftop);
				XFREE(buff);
				break;

			case F_DECOMPILE:

				/*
		 * Decompile output - should be usable on
		 * other MUSHes. *Name if player, Name if
		 * thing, else #Num
		 */
				switch (Typeof(b->thing))
				{
				case TYPE_PLAYER:
					SAFE_LB_CHR('*', boolexp_buf, &buftop);

				case TYPE_THING:
					safe_name(b->thing, boolexp_buf, &buftop);
					break;

				default:
					safe_dbref(boolexp_buf, &buftop, b->thing);
					break;
				}

				break;

			case F_FUNCTION:

				/*
		 * Function output - must be usable by @lock
		 * cmd.  *Name if player, else #Num
		 */
				switch (Typeof(b->thing))
				{
				case TYPE_PLAYER:
					SAFE_LB_CHR('*', boolexp_buf, &buftop);
					safe_name(b->thing, boolexp_buf, &buftop);
					break;

				default:
					safe_dbref(boolexp_buf, &buftop, b->thing);
					break;
				}
			}
		}
		else
		{
			SAFE_LB_STR((char *)unparse_object_quiet(player, b->thing), boolexp_buf, &buftop);
		}

		break;

	case BOOLEXP_ATR:
	case BOOLEXP_EVAL:
		if (b->type == BOOLEXP_EVAL)
		{
			sep_ch = '/';
		}
		else
		{
			sep_ch = ':';
		}

		ap = atr_num(b->thing);

		if (ap && ap->number)
		{
			SAFE_LB_STR((char *)ap->name, boolexp_buf, &buftop);
		}
		else
		{
			SAFE_LTOS(boolexp_buf, &buftop, b->thing, LBUF_SIZE);
		}

		SAFE_LB_CHR(sep_ch, boolexp_buf, &buftop);
		SAFE_LB_STR((char *)b->sub1, boolexp_buf, &buftop);
		break;

	default:
		log_write_raw(1, "ABORT! unparse.c, bad boolexp type in unparse_boolexp1().\n");
		abort();
		break;
	}
}

char *unparse_boolexp_quiet(dbref player, BOOLEXP *b)
{
	buftop = boolexp_buf;
	unparse_boolexp1(player, b, BOOLEXP_CONST, F_QUIET);
	*buftop = '\0';
	return boolexp_buf;
}

char *unparse_boolexp(dbref player, BOOLEXP *b)
{
	buftop = boolexp_buf;
	unparse_boolexp1(player, b, BOOLEXP_CONST, F_EXAMINE);
	*buftop = '\0';
	return boolexp_buf;
}

char *unparse_boolexp_decompile(dbref player, BOOLEXP *b)
{
	buftop = boolexp_buf;
	unparse_boolexp1(player, b, BOOLEXP_CONST, F_DECOMPILE);
	*buftop = '\0';
	return boolexp_buf;
}

char *unparse_boolexp_function(dbref player, BOOLEXP *b)
{
	buftop = boolexp_buf;
	unparse_boolexp1(player, b, BOOLEXP_CONST, F_FUNCTION);
	*buftop = '\0';
	return boolexp_buf;
}
