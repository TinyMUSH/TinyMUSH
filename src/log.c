/* log.c - logging routines */
/* $Id: log.c,v 1.28 2008/01/12 06:53:52 lwl Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "ansi.h"		/* required by code */

FILE *mainlog_fp;

static FILE *log_fp = NULL;

#define MUDLOGNAME (*(mudconf.mud_shortname) ? (mudconf.mud_shortname) : (mudconf.mud_name))

/* *INDENT-OFF* */

NAMETAB logdata_nametab[] =
{
    {(char *)"flags",		1,	0,	LOGOPT_FLAGS},
    {(char *)"location",		1,	0,	LOGOPT_LOC},
    {(char *)"owner",		1,	0,	LOGOPT_OWNER},
    {(char *)"timestamp",		1,	0,	LOGOPT_TIMESTAMP},
    { NULL,				0,	0,	0}
};

NAMETAB logoptions_nametab[] =
{
    {(char *)"accounting",		2,	0,	LOG_ACCOUNTING},
    {(char *)"all_commands",	2,	0,	LOG_ALLCOMMANDS},
    {(char *)"bad_commands",	2,	0,	LOG_BADCOMMANDS},
    {(char *)"buffer_alloc",	3,	0,	LOG_ALLOCATE},
    {(char *)"bugs",		3,	0,	LOG_BUGS},
    {(char *)"checkpoints",		2,	0,	LOG_DBSAVES},
    {(char *)"config_changes",	2,	0,	LOG_CONFIGMODS},
    {(char *)"create",		2,	0,	LOG_PCREATES},
    {(char *)"keyboard_commands",	2,	0,	LOG_KBCOMMANDS},
    {(char *)"killing",		1,	0,	LOG_KILLS},
    {(char *)"local",		3,	0,	LOG_LOCAL},
    {(char *)"logins",		3,	0,	LOG_LOGIN},
    {(char *)"network",		1,	0,	LOG_NET},
    {(char *)"problems",		1,	0,	LOG_PROBLEMS},
    {(char *)"security",		2,	0,	LOG_SECURITY},
    {(char *)"shouts",		2,	0,	LOG_SHOUTS},
    {(char *)"startup",		2,	0,	LOG_STARTUP},
    {(char *)"suspect_commands",	2,	0,	LOG_SUSPECTCMDS},
    {(char *)"time_usage",		1,	0,	LOG_TIMEUSE},
    {(char *)"wizard",		1,	0,	LOG_WIZARD},
    { NULL,				0,	0,	0}
};

LOGFILETAB logfds_table[] =
{
    { LOG_ACCOUNTING,	NULL,		NULL},
    { LOG_ALLCOMMANDS,	NULL,		NULL},
    { LOG_BADCOMMANDS,	NULL,		NULL},
    { LOG_ALLOCATE,		NULL,		NULL},
    { LOG_BUGS,		NULL,		NULL},
    { LOG_DBSAVES,		NULL,		NULL},
    { LOG_CONFIGMODS,	NULL,		NULL},
    { LOG_PCREATES,		NULL,		NULL},
    { LOG_KBCOMMANDS,	NULL,		NULL},
    { LOG_KILLS,		NULL,		NULL},
    { LOG_LOCAL,		NULL,		NULL},
    { LOG_LOGIN,		NULL,		NULL},
    { LOG_NET,		NULL,		NULL},
    { LOG_PROBLEMS,		NULL,		NULL},
    { LOG_SECURITY,		NULL,		NULL},
    { LOG_SHOUTS,		NULL,		NULL},
    { LOG_STARTUP,		NULL,		NULL},
    { LOG_SUSPECTCMDS,	NULL,		NULL},
    { LOG_TIMEUSE,		NULL,		NULL},
    { LOG_WIZARD,		NULL,		NULL},
    { 0,			NULL,		NULL}
};

/* *INDENT-ON* */

/* ---------------------------------------------------------------------------
 * logfile_init: Initialize the main logfile.
 */

void
logfile_init(filename)
char *filename;
{
    if (!filename)
    {
        mainlog_fp = stderr;
        return;
    }

    mainlog_fp = fopen(filename, "w");
    if (!mainlog_fp)
    {
        fprintf(stderr, "Could not open logfile %s for writing.\n",
                filename);
        mainlog_fp = stderr;
        return;
    }

    setbuf(mainlog_fp, NULL);	/* unbuffered */
}


/* ---------------------------------------------------------------------------
 * start_log: see if it is OK to log something, and if so, start writing the
 * log entry.
 */

int
start_log(primary, secondary, key)
const char *primary, *secondary;

int key;
{
    struct tm *tp;

    time_t now;

    LOGFILETAB *lp;

    static int last_key = 0;

    if (!mudstate.standalone)
    {
        if (mudconf.log_diversion & key)
        {
            if (key != last_key)
            {
                /*
                 * Try to save ourselves some lookups
                 */
                last_key = key;
                for (lp = logfds_table; lp->log_flag; lp++)
                {
                    /*
                     * Though keys can be OR'd, use the first one
                     * * matched
                     */

                    if (lp->log_flag & key)
                    {
                        log_fp = lp->fileptr;
                        break;
                    }
                }
                if (!log_fp)
                    log_fp = mainlog_fp;
            }
        }
        else
        {
            last_key = key;
            log_fp = mainlog_fp;
        }
    }
    else
    {
        log_fp = mainlog_fp;
    }

    mudstate.logging++;
    switch (mudstate.logging)
    {
    case 1:
    case 2:

        if (!mudstate.standalone)
        {
            /*
             * Format the timestamp
             */

            if ((mudconf.log_info & LOGOPT_TIMESTAMP) != 0)
            {
                time((time_t *) (&now));
                tp = localtime((time_t *) (&now));
                fprintf(log_fp, "%02d%02d%02d.%02d%02d%02d ",
                        (tp->tm_year % 100), tp->tm_mon + 1,
                        tp->tm_mday, tp->tm_hour,
                        tp->tm_min, tp->tm_sec);
            }

            /*
             * Write the header to the log
             */

            if (secondary && *secondary)
                fprintf(log_fp, "%s %3s/%-5s: ",
                        MUDLOGNAME, primary, secondary);
            else
                fprintf(log_fp, "%s %-9s: ",
                        MUDLOGNAME, primary);
        }

        /*
         * If a recursive call, log it and return indicating no log
         */

        if (mudstate.logging == 1)
            return 1;
        fprintf(mainlog_fp, "Recursive logging request.\r\n");
    default:
        mudstate.logging--;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * end_log: Finish up writing a log entry
 */

void
NDECL(end_log)
{
    fprintf(log_fp, "\n");
    fflush(log_fp);
    mudstate.logging--;
}

/* ---------------------------------------------------------------------------
 * log_perror: Write perror message to the log
 */

void
log_perror(primary, secondary, extra, failing_object)
const char *primary, *secondary, *extra, *failing_object;
{
    int my_errno = errno;

    start_log(primary, secondary, LOG_ALWAYS);
    if (extra && *extra)
    {
        log_printf("(%s) ", extra);
    }
    log_printf("%s: %s", failing_object, strerror(my_errno));
    end_log();
}

/* ---------------------------------------------------------------------------
 * log_printf: Format text and print to the log file.
 */

#if defined(__STDC__) && defined(STDC_HEADERS)
void
log_printf(const char *format, ...)
#else
void
log_printf(va_alist)
va_dcl
#endif
{
    va_list ap;

#if defined(__STDC__) && defined(STDC_HEADERS)
    va_start(ap, format);
#else
    char *format;

    va_start(ap);
    format = va_arg(ap, char *);

#endif
    vfprintf(log_fp, format, ap);
    va_end(ap);
}

void
log_vprintf(format, ap)
const char *format;

va_list ap;
{
    vfprintf(log_fp, format, ap);
}

/* ---------------------------------------------------------------------------
 * log_name: write the name, db number, and flags of an object to the log.
 * If the object does not own itself, append the name, db number, and flags
 * of the owner.
 */

void
log_name(target)
dbref target;
{
    char *tp;

    if (mudstate.standalone)
    {
        fprintf(stderr, "%s(#%d)", Name(target), target);
        return;
    }

    if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
        tp = unparse_object((dbref) GOD, target, 0);
    else
        tp = unparse_object_numonly(target);
    fprintf(log_fp, "%s", strip_ansi(tp));
    free_lbuf(tp);
    if (((mudconf.log_info & LOGOPT_OWNER) != 0) &&
            (target != Owner(target)))
    {
        if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
            tp = unparse_object((dbref) GOD, Owner(target), 0);
        else
            tp = unparse_object_numonly(Owner(target));
        fprintf(log_fp, "[%s]", strip_ansi(tp));
        free_lbuf(tp);
    }
    return;
}

/* ---------------------------------------------------------------------------
 * log_name_and_loc: Log both the name and location of an object
 */

void
log_name_and_loc(player)
dbref player;
{
    log_name(player);
    if ((mudconf.log_info & LOGOPT_LOC) && Has_location(player))
    {
        log_printf(" in ");
        log_name(Location(player));
    }
}

char *
OBJTYP(thing)
dbref thing;
{
    if (!Good_dbref(thing))
    {
        return (char *)"??OUT-OF-RANGE??";
    }
    switch (Typeof(thing))
    {
    case TYPE_PLAYER:
        return (char *)"PLAYER";
    case TYPE_THING:
        return (char *)"THING";
    case TYPE_ROOM:
        return (char *)"ROOM";
    case TYPE_EXIT:
        return (char *)"EXIT";
    case TYPE_GARBAGE:
        return (char *)"GARBAGE";
    default:
        return (char *)"??ILLEGAL??";
    }
}

void
log_type_and_name(thing)
dbref thing;
{
    log_printf("%s ", OBJTYP(thing));
    log_name(thing);
}
