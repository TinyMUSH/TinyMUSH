/* db_rw.c - flatfile implementation */
/* $Id: db_rw.c,v 1.97 2009/02/21 20:06:11 lwl Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "vattr.h"		/* required by code */
#include "attrs.h"		/* required by code */

#include "powers.h"		/* required by code */

extern void FDECL(db_grow, (dbref));

extern struct object *db;

static int g_version;

static int g_format;

static int g_flags;

extern int anum_alc_top;

static int *used_attrs_table;

/*
 * ---------------------------------------------------------------------------
 * getboolexp1: Get boolean subexpression from file.
 */

BOOLEXP *
getboolexp1(f)
FILE *f;
{
    BOOLEXP *b;

    char *buff, *s;

    int c, d, anum;

    c = getc(f);
    switch (c)
    {
    case '\n':
        ungetc(c, f);
        return TRUE_BOOLEXP;
        /*
         * break;
         */
    case EOF:
        fprintf(mainlog_fp,
                "ABORT! db_rw.c, unexpected EOF in boolexp in getboolexp1().\n");
        abort();
        break;
    case '(':
        b = alloc_bool("getboolexp1.openparen");
        switch (c = getc(f))
        {
        case NOT_TOKEN:
            b->type = BOOLEXP_NOT;
            b->sub1 = getboolexp1(f);
            if ((d = getc(f)) == '\n')
                d = getc(f);
            if (d != ')')
                goto error;
            return b;
        case INDIR_TOKEN:
            b->type = BOOLEXP_INDIR;
            b->sub1 = getboolexp1(f);
            if ((d = getc(f)) == '\n')
                d = getc(f);
            if (d != ')')
                goto error;
            return b;
        case IS_TOKEN:
            b->type = BOOLEXP_IS;
            b->sub1 = getboolexp1(f);
            if ((d = getc(f)) == '\n')
                d = getc(f);
            if (d != ')')
                goto error;
            return b;
        case CARRY_TOKEN:
            b->type = BOOLEXP_CARRY;
            b->sub1 = getboolexp1(f);
            if ((d = getc(f)) == '\n')
                d = getc(f);
            if (d != ')')
                goto error;
            return b;
        case OWNER_TOKEN:
            b->type = BOOLEXP_OWNER;
            b->sub1 = getboolexp1(f);
            if ((d = getc(f)) == '\n')
                d = getc(f);
            if (d != ')')
                goto error;
            return b;
        default:
            ungetc(c, f);
            b->sub1 = getboolexp1(f);
            if ((c = getc(f)) == '\n')
                c = getc(f);
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
                d = getc(f);
            if (d != ')')
                goto error;
            return b;
        }
    case '-':		/* obsolete NOTHING key, eat it */
        while ((c = getc(f)) != '\n')
        {
            if (c == EOF)
            {
                fprintf(mainlog_fp,
                        "ABORT! db_rw.c, unexpected EOF in getboolexp1().\n");
                abort();
            }
        }
        ungetc(c, f);
        return TRUE_BOOLEXP;
    case '"':
        ungetc(c, f);
        buff = alloc_lbuf("getboolexp.quoted");
        strcpy(buff, getstring_noalloc(f, 1));
        c = fgetc(f);
        if (c == EOF)
        {
            free_lbuf(buff);
            return TRUE_BOOLEXP;
        }
        b = alloc_bool("getboolexp1.quoted");
        anum = mkattr(buff);
        if (anum <= 0)
        {
            free_bool(b);
            free_lbuf(buff);
            goto error;
        }
        free_lbuf(buff);
        b->thing = anum;

        /*
         * if last character is : then this is an attribute lock. A
         * last character of / means an eval lock
         */

        if ((c == ':') || (c == '/'))
        {
            if (c == '/')
                b->type = BOOLEXP_EVAL;
            else
                b->type = BOOLEXP_ATR;
            b->sub1 =
                (BOOLEXP *) XSTRDUP(getstring_noalloc(f, 1),
                                    "getboolexp1.attr_lock");
        }
        return b;
    default:		/* dbref or attribute */
        ungetc(c, f);
        b = alloc_bool("getboolexp1.default");
        b->type = BOOLEXP_CONST;
        b->thing = 0;

        /*
         * This is either an attribute, eval, or constant lock.
         * Constant locks are of the form <num>, while attribute and
         * eval locks are of the form <anam-or-anum>:<string> or
         * <aname-or-anum>/<string> respectively. The characters
         * <nl>, |, and & terminate the string.
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
            buff = alloc_lbuf("getboolexp1.atr_name");
            for (s = buff;
                    ((c = getc(f)) != EOF) && (c != '\n') &&
                    (c != ':') && (c != '/'); *s++ = c);
            if (c == EOF)
            {
                free_lbuf(buff);
                free_bool(b);
                goto error;
            }
            *s = '\0';

            /*
             * Look the name up as an attribute.  If not found,
             * create a new attribute.
             */

            anum = mkattr(buff);
            if (anum <= 0)
            {
                free_bool(b);
                free_lbuf(buff);
                goto error;
            }
            free_lbuf(buff);
            b->thing = anum;
        }
        else
        {
            free_bool(b);
            goto error;
        }

        /*
         * if last character is : then this is an attribute lock. A
         * last character of / means an eval lock
         */

        if ((c == ':') || (c == '/'))
        {
            if (c == '/')
                b->type = BOOLEXP_EVAL;
            else
                b->type = BOOLEXP_ATR;
            buff = alloc_lbuf("getboolexp1.attr_lock");
            for (s = buff;
                    ((c = getc(f)) != EOF) && (c != '\n') && (c != ')')
                    && (c != OR_TOKEN) && (c != AND_TOKEN); *s++ = c);
            if (c == EOF)
                goto error;
            *s++ = 0;
            b->sub1 =
                (BOOLEXP *) XSTRDUP(buff, "getboolexp1.attr_lock");
            free_lbuf(buff);
        }
        ungetc(c, f);
        return b;
    }

error:
    fprintf(mainlog_fp,
            "ABORT! db_rw.c, reached error case in getboolexp1().\n");
    abort();		/* bomb out */
    return TRUE_BOOLEXP;	/* NOTREACHED */
}

/*
 * ---------------------------------------------------------------------------
 * getboolexp: Read a boolean expression from the flat file.
 */

static BOOLEXP *
getboolexp(f)
FILE *f;
{
    BOOLEXP *b;

    char c;

    b = getboolexp1(f);
    if (getc(f) != '\n')
    {
        fprintf(mainlog_fp,
                "ABORT! db_rw.c, parse error in getboolexp().\n");
        abort();	/* parse error, we lose */
    }
    if ((c = getc(f)) != '\n')
        ungetc(c, f);

    return b;
}

/*
 * ---------------------------------------------------------------------------
 * unscramble_attrnum: Fix up attribute numbers from foreign muds
 */

static int
unscramble_attrnum(attrnum)
int attrnum;
{
    switch (g_format)
    {
    case F_MUSH:
        /*
         * TinyMUSH 2.2:  Deal with different attribute numbers.
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

/*
 * ---------------------------------------------------------------------------
 * get_list: Read attribute list from flat file.
 */

static int
get_list(f, i, new_strings)
FILE *f;

dbref i;

int new_strings;
{
    dbref atr;

    int c;

    char *buff;

    buff = alloc_lbuf("get_list");
    while (1)
    {
        switch (c = getc(f))
        {
        case '>':	/* read # then string */
            if (mudstate.standalone)
                atr = unscramble_attrnum(getref(f));
            else
                atr = getref(f);
            if (atr > 0)
            {
                /*
                 * Store the attr
                 */

                atr_add_raw(i, atr,
                            (char *)getstring_noalloc(f, new_strings));
            }
            else
            {
                /*
                 * Silently discard
                 */

                getstring_noalloc(f, new_strings);
            }
            break;
        case '\n':	/* ignore newlines. They're due to v(r). */
            break;
        case '<':	/* end of list */
            free_lbuf(buff);
            c = getc(f);
            if (c != '\n')
            {
                ungetc(c, f);
                fprintf(mainlog_fp,
                        "No line feed on object %d\n", i);
                return 1;
            }
            return 1;
        default:
            fprintf(mainlog_fp,
                    "Bad character '%c' when getting attributes on object %d\n",
                    c, i);
            /*
             * We've found a bad spot.  I hope things aren't too
             * bad.
             */

            (void)getstring_noalloc(f, new_strings);
        }
    }
    return 1;		/* NOTREACHED */
}

/*
 * ---------------------------------------------------------------------------
 * putbool_subexp: Write a boolean sub-expression to the flat file.
 */
static void
putbool_subexp(f, b)
FILE *f;

BOOLEXP *b;
{
    ATTR *va;

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
        fprintf(mainlog_fp,
                "Unknown boolean type in putbool_subexp: %d\n", b->type);
    }
}

/*
 * ---------------------------------------------------------------------------
 * putboolexp: Write boolean expression to the flat file.
 */

void
putboolexp(f, b)
FILE *f;

BOOLEXP *b;
{
    if (b != TRUE_BOOLEXP)
    {
        putbool_subexp(f, b);
    }
    putc('\n', f);
}

/*
 * ---------------------------------------------------------------------------
 * upgrade_flags: Convert foreign flags to MUSH format.
 */

static void
upgrade_flags(flags1, flags2, flags3, thing, db_format, db_version)
FLAG *flags1, *flags2, *flags3;

dbref thing;

int db_format, db_version;
{
    FLAG f1, f2, f3, newf1, newf2, newf3;

    f1 = *flags1;
    f2 = *flags2;
    f3 = *flags3;
    newf1 = 0;
    newf2 = 0;
    newf3 = 0;
    if ((db_format == F_MUSH) && (db_version >= 3))
    {
        newf1 = f1;
        newf2 = f2;
        newf3 = 0;

        /*
         * Then we have to do the 2.2 to 3.0 flag conversion
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
            newf2 &= ~HAS_DAILY;
            /*
             * This is the unimplemented TICKLER flag.
             */
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

        /*
         * TinyMUX to 3.0 flag conversion
         */

        newf1 = f1;
        newf2 = f2;
        newf3 = f3;

        if (newf2 & ZONE_PARENT)
        {
            /*
             * This used to be an object set NO_COMMAND. We unset
             * the flag.
             */
            newf2 &= ~ZONE_PARENT;
        }
        else
        {
            /*
             * And if it wasn't NO_COMMAND, then it should be
             * COMMANDS.
             */
            newf2 |= HAS_COMMANDS;
        }

        if (newf2 & WATCHER)
        {
            /*
             * This used to be the COMPRESS flag, which didn't do
             * anything.
             */
            newf2 &= ~WATCHER;
        }
        if ((newf1 & MONITOR) && ((newf1 & TYPE_MASK) == TYPE_PLAYER))
        {
            /*
             * Players set MONITOR should be set WATCHER as well.
             */
            newf2 |= WATCHER;
        }
    }
    else if (db_format == F_TINYMUSH)
    {
        /*
         * Native TinyMUSH 3.0 database. The only thing we have to do
         * is clear the redirection flag, as nothing is ever
         * redirected at startup.
         */
        newf1 = f1;
        newf2 = f2;
        newf3 = f3 & ~HAS_REDIRECT;
    }
    newf2 = newf2 & ~FLOATING;	/* this flag is now obsolete */

    *flags1 = newf1;
    *flags2 = newf2;
    *flags3 = newf3;
    return;
}

/*
 * ---------------------------------------------------------------------------
 * efo_convert: Fix things up for Exits-From-Objects
 */

void
NDECL(efo_convert)
{
    int i;

    dbref link;

    DO_WHOLE_DB(i)
    {
        switch (Typeof(i))
        {
        case TYPE_PLAYER:
        case TYPE_THING:

            /*
             * swap Exits and Link
             */

            link = Link(i);
            s_Link(i, Exits(i));
            s_Exits(i, link);
            break;
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * fix_mux_zones: Convert MUX-style zones to 3.0-style zones.
 */

static void
fix_mux_zones()
{
    /*
     * For all objects in the database where Zone(thing) != NOTHING, set
     * the CONTROL_OK flag on them.
     *
     * For all objects in the database that are ZMOs (that have other
     * objects zoned to them), copy the EnterLock of those objects to the
     * ControlLock.
     */

    int i;

    int *zmarks;

    char *astr;

    zmarks = (int *)XCALLOC(mudstate.db_top, sizeof(int), "fix_mux_zones");

    DO_WHOLE_DB(i)
    {
        if (Zone(i) != NOTHING)
        {
            s_Flags2(i, Flags2(i) | CONTROL_OK);
            zmarks[Zone(i)] = 1;
        }
    }

    DO_WHOLE_DB(i)
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

    XFREE(zmarks, "fix_mux_zones");
}

/*
 * ---------------------------------------------------------------------------
 * fix_typed_quotas: Explode standard quotas into typed quotas
 */

static void
fix_typed_quotas()
{
    /*
     * If we have a pre-2.2 or MUX database, only the QUOTA and RQUOTA
     * attributes  exist. For simplicity's sake, we assume that players
     * will have the  same quotas for all types, equal to the current
     * value. This is  going to produce incorrect values for RQUOTA; this
     * is easily fixed  by a @quota/fix done from within-game.
     *
     * If we have a early beta 2.2 release, we have quotas which are spread
     * out over ten attributes. We're going to have to grab those, make
     * the new quotas, and then delete the old attributes.
     */

    int i;

    char *qbuf, *rqbuf;

    DO_WHOLE_DB(i)
    {
        if (isPlayer(i))
        {
            qbuf = atr_get_raw(i, A_QUOTA);
            rqbuf = atr_get_raw(i, A_RQUOTA);
            if (!qbuf || !*qbuf)
                qbuf = (char *)"1";
            if (!rqbuf || !*rqbuf)
                rqbuf = (char *)"0";
            atr_add_raw(i, A_QUOTA,
                        tprintf("%s %s %s %s %s",
                                qbuf, qbuf, qbuf, qbuf, qbuf));
            atr_add_raw(i, A_RQUOTA,
                        tprintf("%s %s %s %s %s",
                                rqbuf, rqbuf, rqbuf, rqbuf, rqbuf));
        }
    }
}

dbref
db_read_flatfile(f, db_format, db_version, db_flags)
FILE *f;

int *db_format, *db_version, *db_flags;
{
    dbref i, anum;

    char ch;

    const char *tstr;

    int header_gotten, size_gotten, nextattr_gotten;

    int read_attribs, read_name, read_zone, read_link, read_key,
        read_parent;
    int read_extflags, read_3flags, read_money, read_timestamps,
        read_createtime, read_new_strings;
    int read_powers, read_powers_player, read_powers_any;

    int has_typed_quotas, has_visual_attrs;

    int deduce_version, deduce_name, deduce_zone, deduce_timestamps;

    int aflags, f1, f2, f3;

    BOOLEXP *tempbool;

    time_t tmptime;

#ifndef NO_TIMECHECKING
    struct timeval obj_time;

#endif

    header_gotten = 0;
    size_gotten = 0;
    nextattr_gotten = 0;
    g_format = F_UNKNOWN;
    g_version = 0;
    g_flags = 0;
    read_attribs = 1;
    read_name = 1;
    read_zone = 0;
    read_link = 0;
    read_key = 1;
    read_parent = 0;
    read_money = 1;
    read_extflags = 0;
    read_3flags = 0;
    has_typed_quotas = 0;
    has_visual_attrs = 0;
    read_timestamps = 0;
    read_createtime = 0;
    read_new_strings = 0;
    read_powers = 0;
    read_powers_player = 0;
    read_powers_any = 0;
    deduce_version = 1;
    deduce_zone = 1;
    deduce_name = 1;
    deduce_timestamps = 1;
    fprintf(mainlog_fp, "Reading ");
    db_free();
    for (i = 0;; i++)
    {

        if (!(i % 100))
        {
            fputc('.', mainlog_fp);
        }
        switch (ch = getc(f))
        {
        case '-':	/* Misc tag */
            switch (ch = getc(f))
            {
            case 'R':	/* Record number of players */
                mudstate.record_players = getref(f);
                break;
            default:
                (void)getstring_noalloc(f, 0);
            }
            break;
        case '+':	/* MUX and MUSH header */

            ch = getc(f);	/* 2nd char selects type */

            if ((ch == 'V') || (ch == 'X') || (ch == 'T'))
            {

                /*
                 * The following things are common across
                 * 2.x, MUX, and 3.0.
                 */

                if (header_gotten)
                {
                    fprintf(mainlog_fp,
                            "\nDuplicate MUSH version header entry at object %d, ignored.\n",
                            i);
                    tstr = getstring_noalloc(f, 0);
                    break;
                }
                header_gotten = 1;
                deduce_version = 0;
                g_version = getref(f);

                /*
                 * Otherwise extract feature flags
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
            /*
             * More generic switch.
             */

            switch (ch)
            {
            case 'T':	/* 3.0 VERSION */
                g_format = F_TINYMUSH;
                read_3flags = (g_version & V_3FLAGS);
                read_powers = (g_version & V_POWERS);
                read_new_strings = (g_version & V_QUOTED);
                g_version &= V_MASK;
                break;

            case 'V':	/* 2.0 VERSION */
                g_format = F_MUSH;
                g_version &= V_MASK;
                break;

            case 'X':	/* MUX VERSION */
                g_format = F_MUX;
                read_3flags = (g_version & V_3FLAGS);
                read_powers = (g_version & V_POWERS);
                read_new_strings = (g_version & V_QUOTED);
                g_version &= V_MASK;
                break;

            case 'S':	/* SIZE */
                if (size_gotten)
                {
                    fprintf(mainlog_fp,
                            "\nDuplicate size entry at object %d, ignored.\n",
                            i);
                    tstr = getstring_noalloc(f, 0);
                }
                else
                {
                    mudstate.min_size = getref(f);
                }
                size_gotten = 1;
                break;
            case 'A':	/* USER-NAMED ATTRIBUTE */
                anum = getref(f);
                tstr = getstring_noalloc(f, read_new_strings);
                if (isdigit(*tstr))
                {
                    aflags = 0;
                    while (isdigit(*tstr))
                        aflags = (aflags * 10) +
                                 (*tstr++ - '0');
                    tstr++;	/* skip ':' */
                    if (!has_visual_attrs)
                    {
                        /*
                         * If not AF_ODARK, is
                         * AF_VISUAL. Strip AF_ODARK.
                         */
                        if (aflags & AF_ODARK)
                            aflags &= ~AF_ODARK;
                        else
                            aflags |= AF_VISUAL;
                    }
                }
                else
                {
                    aflags = mudconf.vattr_flags;
                }
                vattr_define((char *)tstr, anum, aflags);
                break;
            case 'F':	/* OPEN USER ATTRIBUTE SLOT */
                anum = getref(f);
                break;
            case 'N':	/* NEXT ATTR TO ALLOC WHEN NO
					 * FREELIST */
                if (nextattr_gotten)
                {
                    fprintf(mainlog_fp,
                            "\nDuplicate next free vattr entry at object %d, ignored.\n",
                            i);
                    tstr = getstring_noalloc(f, 0);
                }
                else
                {
                    mudstate.attr_next = getref(f);
                    nextattr_gotten = 1;
                }
                break;
            default:
                fprintf(mainlog_fp,
                        "\nUnexpected character '%c' in MUSH header near object #%d, ignored.\n",
                        ch, i);
                tstr = getstring_noalloc(f, 0);
            }
            break;
        case '!':	/* MUX entry/MUSH entry */
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

#ifndef NO_TIMECHECKING
            obj_time.tv_sec = obj_time.tv_usec = 0;
            s_Time_Used(i, obj_time);
#endif
            s_StackCount(i, 0);
            s_VarsCount(i, 0);
            s_StructCount(i, 0);
            s_InstanceCount(i, 0);

            if (read_name)
            {
                tstr = getstring_noalloc(f, read_new_strings);
                if (deduce_name)
                {
                    if (isdigit(*tstr))
                    {
                        read_name = 0;
                        s_Location(i, atoi(tstr));
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
                s_Zone(i, getref(f));
            /*
             * else s_Zone(i, NOTHING);
             */

            /*
             * CONTENTS and EXITS
             */

            s_Contents(i, getref(f));
            s_Exits(i, getref(f));

            /*
             * LINK
             */

            if (read_link)
            {
                s_Link(i, getref(f));
            }
            else
            {
                s_Link(i, NOTHING);
            }

            /*
             * NEXT
             */

            s_Next(i, getref(f));

            /*
             * LOCK
             */

            if (read_key)
            {
                tempbool = getboolexp(f);
                atr_add_raw(i, A_LOCK,
                            unparse_boolexp_quiet(1, tempbool));
                free_boolexp(tempbool);
            }
            /*
             * OWNER
             */

            s_Owner(i, getref(f));

            /*
             * PARENT
             */

            if (read_parent)
            {
                s_Parent(i, getref(f));
            }
            else
            {
                s_Parent(i, NOTHING);
            }

            /*
             * PENNIES
             */

            if (read_money)
                s_Pennies(i, getref(f));

            /*
             * FLAGS
             */

            f1 = getref(f);
            if (read_extflags)
                f2 = getref(f);
            else
                f2 = 0;

            if (read_3flags)
                f3 = getref(f);
            else
                f3 = 0;

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
                tmptime = (time_t) getlong(f);
                s_AccessTime(i, tmptime);
                tmptime = (time_t) getlong(f);
                s_ModTime(i, tmptime);
            }
            else
            {
                AccessTime(i) = ModTime(i) = time(NULL);
            }

            if (read_createtime)
            {
                tmptime = (time_t) getlong(f);
                s_CreateTime(i, tmptime);
            }
            else
            {
                s_CreateTime(i, AccessTime(i));
            }

            /*
             * ATTRIBUTES
             */

            if (read_attribs)
            {
                if (!get_list(f, i, read_new_strings))
                {
                    fprintf(mainlog_fp,
                            "\nError reading attrs for object #%d\n",
                            i);
                    return -1;
                }
            }
            /*
             * check to see if it's a player
             */

            if (Typeof(i) == TYPE_PLAYER)
            {
                c_Connected(i);
            }
            break;
        case '*':	/* EOF marker */
            tstr = getstring_noalloc(f, 0);
            if (strcmp(tstr, "**END OF DUMP***"))
            {
                fprintf(mainlog_fp,
                        "\nBad EOF marker at object #%d\n", i);
                return -1;
            }
            else
            {
                fprintf(mainlog_fp, "\n");
                *db_version = g_version;
                *db_format = g_format;
                *db_flags = g_flags;
                if (!has_typed_quotas)
                    fix_typed_quotas();
                if (g_format == F_MUX)
                    fix_mux_zones();
                return mudstate.db_top;
            }
        default:
            fprintf(mainlog_fp,
                    "\nIllegal character '%c' near object #%d\n", ch,
                    i);
            return -1;
        }


    }
}

int
db_read()
{
    DBData key, data;

    int *c, vattr_flags, i, j, blksize, num;

    char *s;

#ifndef NO_TIMECHECKING
    struct timeval obj_time;

#endif

    /*
     * Fetch the database info
     */

    key.dptr = "TM3";
    key.dsize = strlen("TM3") + 1;
    data = db_get(key, DBTYPE_DBINFO);

    if (!data.dptr)
    {
        fprintf(mainlog_fp, "\nCould not open main record");
        return -1;
    }
    /*
     * Unroll the data returned
     */

    c = data.dptr;
    memcpy((void *)&mudstate.min_size, (void *)c, sizeof(int));
    c++;
    memcpy((void *)&mudstate.attr_next, (void *)c, sizeof(int));
    c++;
    memcpy((void *)&mudstate.record_players, (void *)c, sizeof(int));
    c++;
    memcpy((void *)&mudstate.moduletype_top, (void *)c, sizeof(int));
    RAW_FREE(data.dptr, "db_get");

    /*
     * Load the attribute numbers
     */

    blksize = ATRNUM_BLOCK_SIZE;

    for (i = 0; i <= ENTRY_NUM_BLOCKS(mudstate.attr_next, blksize); i++)
    {
        key.dptr = &i;
        key.dsize = sizeof(int);
        data = db_get(key, DBTYPE_ATRNUM);

        if (data.dptr)
        {
            /*
             * Unroll the data into flags and name
             */

            s = data.dptr;

            while ((s - (char *)data.dptr) < data.dsize)
            {
                memcpy((void *)&j, (void *)s, sizeof(int));
                s += sizeof(int);
                memcpy((void *)&vattr_flags, (void *)s,
                       sizeof(int));
                s += sizeof(int);
                vattr_define(s, j, vattr_flags);
                s = strchr((const char *)s, '\0');

                if (!s)
                {
                    /*
                     * Houston, we have a problem
                     */
                    fprintf(mainlog_fp,
                            "\nError reading attribute number %d\n",
                            j + ENTRY_BLOCK_STARTS(i,
                                                   blksize));
                }
                s++;
            }
            RAW_FREE(data.dptr, "db_get");
        }
    }

    /*
     * Load the object structures
     */

    if (mudstate.standalone)
        fprintf(mainlog_fp, "Reading ");

    blksize = OBJECT_BLOCK_SIZE;

    for (i = 0; i <= ENTRY_NUM_BLOCKS(mudstate.min_size, blksize); i++)
    {
        key.dptr = &i;
        key.dsize = sizeof(int);
        data = db_get(key, DBTYPE_OBJECT);

        if (data.dptr)
        {
            /*
             * Unroll the data into objnum and object
             */

            s = data.dptr;

            while ((s - (char *)data.dptr) < data.dsize)
            {
                memcpy((void *)&num, (void *)s, sizeof(int));
                s += sizeof(int);
                db_grow(num + 1);

                if (mudstate.standalone && !(num % 100))
                {
                    fputc('.', mainlog_fp);
                }
                /*
                 * We read the entire object structure in and
                 * copy it into place
                 */

                memcpy((void *)&(db[num]), (void *)s,
                       sizeof(DUMPOBJ));
                s += sizeof(DUMPOBJ);
#ifndef NO_TIMECHECKING
                obj_time.tv_sec = obj_time.tv_usec = 0;
                s_Time_Used(num, obj_time);
#endif
                s_StackCount(num, 0);
                s_VarsCount(num, 0);
                s_StructCount(num, 0);
                s_InstanceCount(num, 0);
#ifdef MEMORY_BASED
                db[num].attrtext.at_count = 0;
                db[num].attrtext.atrs = NULL;
#endif
                /*
                 * Check to see if it's a player
                 */

                if (Typeof(num) == TYPE_PLAYER)
                {
                    c_Connected(num);
                }
                s_Clean(num);
            }

            RAW_FREE(data.dptr, "db_get");
        }
    }

    if (!mudstate.standalone)
        load_player_names();

    if (mudstate.standalone)
        fprintf(mainlog_fp, "\n");

    return (0);
}

static int
db_write_object_out(f, i, db_format, flags, n_atrt)
FILE *f;

dbref i;

int db_format, flags;

int *n_atrt;
{
    ATTR *a;

    char *got, *as;

    dbref aowner;

    int ca, aflags, alen, save, j, changed;

    BOOLEXP *tempbool;

    if (Going(i))
    {
        return (0);
    }
    fprintf(f, "!%d\n", i);
    if (!(flags & V_ATRNAME))
        putstring(f, Name(i));
    putref(f, Location(i));
    if (flags & V_ZONE)
        putref(f, Zone(i));
    putref(f, Contents(i));
    putref(f, Exits(i));
    if (flags & V_LINK)
        putref(f, Link(i));
    putref(f, Next(i));
    if (!(flags & V_ATRKEY))
    {
        got = atr_get(i, A_LOCK, &aowner, &aflags, &alen);
        tempbool = parse_boolexp(GOD, got, 1);
        free_lbuf(got);
        putboolexp(f, tempbool);
        if (tempbool)
            free_boolexp(tempbool);
    }
    putref(f, Owner(i));
    if (flags & V_PARENT)
        putref(f, Parent(i));
    if (!(flags & V_ATRMONEY))
        putref(f, Pennies(i));
    putref(f, Flags(i));
    if (flags & V_XFLAGS)
        putref(f, Flags2(i));
    if (flags & V_3FLAGS)
        putref(f, Flags3(i));
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
    /*
     * write the attribute list
     */

    changed = 0;

    for (ca = atr_head(i, &as); ca; ca = atr_next(&as))
    {
        save = 0;
        if (!mudstate.standalone)
        {
            a = atr_num(ca);
            if (a)
                j = a->number;
            else
                j = -1;
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
                    save = 1;
                break;
            case A_LOCK:
                if (flags & V_ATRKEY)
                    save = 1;
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

dbref
db_write_flatfile(f, format, version)
FILE *f;

int format, version;
{
    dbref i;

    int flags;

    VATTR *vp;

    int n, end, ca, n_oldtotal, n_oldtop, n_deleted, n_renumbered;

    int n_objt, n_atrt, anxt, dbclean;

    int *old_attrs_table;

    char *as;

    al_store();

    dbclean = (version & V_DBCLEAN) ? 1 : 0;
    version &= ~V_DBCLEAN;

    switch (format)
    {
    case F_TINYMUSH:
        flags = version;
        break;
    default:
        fprintf(mainlog_fp, "Can only write TinyMUSH 3 format.\n");
        return -1;
    }
    if (mudstate.standalone)
        fprintf(mainlog_fp, "Writing ");

    /*
     * Attribute cleaning, if standalone.
     */

    if (mudstate.standalone && dbclean)
    {

        used_attrs_table = (int *)XCALLOC(mudstate.attr_next,
                                          sizeof(int), "flatfile.used_attrs_table");
        old_attrs_table = (int *)XCALLOC(mudstate.attr_next,
                                         sizeof(int), "flatfile.old_attrs_table");
        n_oldtotal = mudstate.attr_next;
        n_oldtop = anum_alc_top;
        n_deleted = n_renumbered = n_objt = n_atrt = 0;

        /*
         * Non-user defined attributes are always considered used.
         */

        for (n = 0; n < A_USER_START; n++)
            used_attrs_table[n] = n;

        /*
         * Walk the database. Mark all the attribute numbers in use.
         */

        atr_push();
        DO_WHOLE_DB(i)
        {
            for (ca = atr_head(i, &as); ca; ca = atr_next(&as))
                used_attrs_table[ca] = old_attrs_table[ca] =
                                           ca;
        }
        atr_pop();

        /*
         * Count up how many attributes we're deleting.
         */

        vp = vattr_first();
        while (vp)
        {
            if (used_attrs_table[vp->number] == 0)
                n_deleted++;
            vp = vattr_next(vp);
        }

        /*
         * Walk the table we've created of used statuses. When we
         * find free slots, walk backwards to the first used slot at
         * the end of the table. Write the number of the free slot
         * into that used slot. Keep a mapping of what things used to
         * be.
         */

        for (n = A_USER_START, end = mudstate.attr_next - 1;
                (n < mudstate.attr_next) && (n < end); n++)
        {
            if (used_attrs_table[n] == 0)
            {
                while ((end > n)
                        && (used_attrs_table[end] == 0))
                    end--;
                if (end > n)
                {
                    old_attrs_table[n] = end;
                    used_attrs_table[end] =
                        used_attrs_table[n] = n;
                    end--;
                }
            }
        }

        /*
         * Count up our renumbers.
         */

        for (n = A_USER_START; n < mudstate.attr_next; n++)
        {
            if ((used_attrs_table[n] != n)
                    && (used_attrs_table[n] != 0))
            {
                vp = (VATTR *) anum_get(n);
                if (vp)
                    n_renumbered++;
            }
        }

        /*
         * The new end of the attribute table is the first thing
         * we've renumbered.
         */

        for (anxt = A_USER_START;
                ((anxt == used_attrs_table[anxt]) &&
                 (anxt < mudstate.attr_next)); anxt++);

    }
    else
    {
        used_attrs_table = NULL;
        anxt = mudstate.attr_next;
    }

    /*
     * Write database information. TinyMUSH 2 wrote '+V', MUX wrote '+X',
     * 3.0 writes '+T'.
     */
    fprintf(f, "+T%d\n+S%d\n+N%d\n", flags, mudstate.db_top, anxt);
    fprintf(f, "-R%d\n", mudstate.record_players);

    /*
     * Dump user-named attribute info
     */

    if (mudstate.standalone && dbclean)
    {
        for (i = A_USER_START; i < anxt; i++)
        {
            if (used_attrs_table[i] == 0)
                continue;
            vp = (VATTR *) anum_get(old_attrs_table[i]);
            if (vp)
            {
                if (!(vp->flags & AF_DELETED))
                {
                    fprintf(f, "+A%d\n\"%d:%s\"\n",
                            i, vp->flags, vp->name);
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
                fprintf(f, "+A%d\n\"%d:%s\"\n",
                        vp->number, vp->flags, vp->name);
            }
            vp = vattr_next(vp);
        }
    }

    /*
     * Dump object and attribute info
     */

    n_objt = n_atrt = 0;

    DO_WHOLE_DB(i)
    {

        if (mudstate.standalone && !(i % 100))
        {
            fputc('.', mainlog_fp);
        }
        n_objt += db_write_object_out(f, i, format, flags, &n_atrt);
    }

    fputs("***END OF DUMP***\n", f);
    fflush(f);

    if (mudstate.standalone)
    {
        fprintf(mainlog_fp, "\n");
        if (dbclean)
        {
            if (n_objt)
            {
                fprintf(mainlog_fp,
                        "Cleaned %d attributes (now %d): %d deleted, %d renumbered (%d objects and %d individual attrs touched).\n",
                        n_oldtotal, anxt, n_deleted, n_renumbered,
                        n_objt, n_atrt);
            }
            else if (n_deleted || n_renumbered)
            {
                fprintf(mainlog_fp,
                        "Cleaned %d attributes (now %d): %d deleted, %d renumbered (no objects touched).\n",
                        n_oldtotal, anxt, n_deleted, n_renumbered);
            }
            XFREE(used_attrs_table, "flatfile.used_attrs_table");
            XFREE(old_attrs_table, "flatfile.old_attrs_table");
        }
    }
    return (mudstate.db_top);
}

dbref
db_write()
{
    VATTR *vp;

    DBData key, data;

    int *c, blksize, num, i, j, k, dirty, len;

    char *s;

    al_store();

    if (mudstate.standalone)
        fprintf(mainlog_fp, "Writing ");

    /*
     * Lock the database
     */

    db_lock();

    /*
     * Write database information
     */

    i = mudstate.attr_next;

    /*
     * Roll up various paramaters needed for startup into one record.
     * This should be the only data record of its type
     */

    c = data.dptr = (int *)XMALLOC(4 * sizeof(int), "db_write");
    memcpy((void *)c, (void *)&mudstate.db_top, sizeof(int));
    c++;
    memcpy((void *)c, (void *)&i, sizeof(int));
    c++;
    memcpy((void *)c, (void *)&mudstate.record_players, sizeof(int));
    c++;
    memcpy((void *)c, (void *)&mudstate.moduletype_top, sizeof(int));

    /*
     * "TM3" is our unique key
     */

    key.dptr = "TM3";
    key.dsize = strlen("TM3") + 1;
    data.dsize = 4 * sizeof(int);
    db_put(key, data, DBTYPE_DBINFO);
    XFREE(data.dptr, "db_write");

    /*
     * Dump user-named attribute info
     */

    /*
     * First, calculate the number of attribute entries we can fit in a
     * block, allowing for some minor DBM key overhead. This should not
     * change unless the size of VNAME_SIZE or LBUF_SIZE changes, in
     * which case you'd have to reload anyway
     */

    blksize = ATRNUM_BLOCK_SIZE;

    /*
     * Step through the attribute number array, writing stuff in 'num'
     * sized chunks
     */

    data.dptr = (char *)XMALLOC(ATRNUM_BLOCK_BYTES, "db_write.cdata");

    for (i = 0; i <= ENTRY_NUM_BLOCKS(mudstate.attr_next, blksize); i++)
    {
        dirty = 0;
        num = 0;
        s = data.dptr;

        for (j = ENTRY_BLOCK_STARTS(i, blksize);
                (j <= ENTRY_BLOCK_ENDS(i, blksize)) &&
                (j < mudstate.attr_next); j++)
        {

            if (j < A_USER_START)
            {
                continue;
            }
            vp = (VATTR *) anum_table[j];

            if (vp && !(vp->flags & AF_DELETED))
            {
                if (!mudstate.standalone)
                {
                    if (vp->flags & AF_DIRTY)
                    {
                        /*
                         * Only write the dirty
                         * attribute numbers and
                         * clear the flag
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
            /*
             * No valid attributes in this block, delete it
             */
            key.dptr = &i;
            key.dsize = sizeof(int);
            db_del(key, DBTYPE_ATRNUM);
        }
        if (dirty)
        {
            /*
             * Something is dirty in this block, write all of the
             * attribute numbers in this block
             */

            for (j = 0; (j < blksize) &&
                    ((ENTRY_BLOCK_STARTS(i,
                                         blksize) + j) < mudstate.attr_next);
                    j++)
            {
                /*
                 * j is an offset of attribute numbers into
                 * the current block
                 */

                if ((ENTRY_BLOCK_STARTS(i,
                                        blksize) + j) < A_USER_START)
                {
                    continue;
                }
                vp = (VATTR *) anum_table[ENTRY_BLOCK_STARTS(i,
                                          blksize) + j];

                if (vp && !(vp->flags & AF_DELETED))
                {
                    len = strlen(vp->name) + 1;
                    memcpy((void *)s, (void *)&vp->number,
                           sizeof(int));
                    s += sizeof(int);
                    memcpy((void *)s, (void *)&vp->flags,
                           sizeof(int));
                    s += sizeof(int);
                    memcpy((void *)s, (void *)vp->name,
                           len);
                    s += len;
                }
            }

            /*
             * Write the block: Block number is our key
             */

            key.dptr = &i;
            key.dsize = sizeof(int);
            data.dsize = s - (char *)data.dptr;
            db_put(key, data, DBTYPE_ATRNUM);
        }
    }
    XFREE(data.dptr, "db_write.cdata");

    /*
     * Dump object structures using the same block-based method we use to
     * dump attribute numbers
     */

    blksize = OBJECT_BLOCK_SIZE;

    /*
     * Step through the object structure array, writing stuff in 'num'
     * sized chunks
     */

    data.dptr = (char *)XMALLOC(OBJECT_BLOCK_BYTES, "db_write.cdata");

    for (i = 0; i <= ENTRY_NUM_BLOCKS(mudstate.db_top, blksize); i++)
    {
        dirty = 0;
        num = 0;
        s = data.dptr;

        for (j = ENTRY_BLOCK_STARTS(i, blksize);
                (j <= ENTRY_BLOCK_ENDS(i, blksize)) &&
                (j < mudstate.db_top); j++)
        {

            if (mudstate.standalone && !(j % 100))
            {
                fputc('.', mainlog_fp);
            }
            /*
             * We assume you always do a dbck before dump, and
             * Going objects are really destroyed!
             */

            if (!Going(j))
            {
                if (!mudstate.standalone)
                {
                    if (Flags3(j) & DIRTY)
                    {
                        /*
                         * Only write the dirty
                         * objects and clear the flag
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
            /*
             * No valid objects in this block, delete it
             */

            key.dptr = &i;
            key.dsize = sizeof(int);
            db_del(key, DBTYPE_OBJECT);
        }
        if (dirty)
        {
            /*
             * Something is dirty in this block, write all of the
             * objects in this block
             */

            for (j = 0; (j < blksize) &&
                    ((ENTRY_BLOCK_STARTS(i,
                                         blksize) + j) < mudstate.db_top);
                    j++)
            {
                /*
                 * j is an offset of object numbers into the
                 * current block
                 */

                k = ENTRY_BLOCK_STARTS(i, blksize) + j;

                if (!Going(k))
                {
                    memcpy((void *)s, (void *)&k,
                           sizeof(int));
                    s += sizeof(int);
                    memcpy((void *)s, (void *)&(db[k]),
                           sizeof(DUMPOBJ));
                    s += sizeof(DUMPOBJ);
                }
            }

            /*
             * Write the block: Block number is our key
             */

            key.dptr = &i;
            key.dsize = sizeof(int);
            data.dsize = s - (char *)data.dptr;
            db_put(key, data, DBTYPE_OBJECT);
        }
    }

    XFREE(data.dptr, "db_write.cdata");

    /*
     * Unlock the database
     */

    db_unlock();

    if (mudstate.standalone)
        fprintf(mainlog_fp, "\n");
    return (mudstate.db_top);
}

/* Open a file pointer for a module to use when writing a flatfile */

FILE *
db_module_flatfile(modname, wrflag)
char *modname;

int wrflag;
{
    char filename[256];

    FILE *f = NULL;

    sprintf(filename, "%s/mod_%s.db", mudconf.dbhome, modname);

    if (wrflag)
    {
        f = tf_fopen(filename, O_WRONLY | O_CREAT | O_TRUNC);
        STARTLOG(LOG_ALWAYS, "DMP", "DUMP")
        log_printf("Writing db: %s", filename);
        ENDLOG
    }
    else
    {
        f = tf_fopen(filename, O_RDONLY);
        STARTLOG(LOG_ALWAYS, "INI", "LOAD")
        log_printf("Loading db: %s", filename);
        ENDLOG
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
