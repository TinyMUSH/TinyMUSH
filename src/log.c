/* log.c - Handle log files and log events */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mudconf */
#include "game.h"      /* required by mudconf */
#include "alloc.h"     /* required by mudconf */
#include "flags.h"     /* required by mudconf */
#include "htab.h"      /* required by mudconf */
#include "ltdl.h"      /* required by mudconf */
#include "udb.h"       /* required by mudconf */
#include "udb_defs.h"  /* required by mudconf */
#include "mushconf.h"  /* required by code */
#include "db.h"        /* required by externs */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by code */
#include "ansi.h"      /* required by code */

FILE *mainlog_fp = NULL; /*!< Pointer to the main log file */
FILE *log_fp = NULL;     /*!< Pointer to the facility's log file */

char *log_pos = NULL;

NAMETAB logdata_nametab[] = {
    {(char *)"flags", 1, 0, LOGOPT_FLAGS},
    {(char *)"location", 1, 0, LOGOPT_LOC},
    {(char *)"owner", 1, 0, LOGOPT_OWNER},
    {(char *)"timestamp", 1, 0, LOGOPT_TIMESTAMP},
    {NULL, 0, 0, 0}};

NAMETAB logoptions_nametab[] = {
    {(char *)"accounting", 2, 0, LOG_ACCOUNTING},
    {(char *)"all_commands", 2, 0, LOG_ALLCOMMANDS},
    {(char *)"bad_commands", 2, 0, LOG_BADCOMMANDS},
    {(char *)"buffer_alloc", 3, 0, LOG_ALLOCATE},
    {(char *)"bugs", 3, 0, LOG_BUGS},
    {(char *)"checkpoints", 2, 0, LOG_DBSAVES},
    {(char *)"config_changes", 2, 0, LOG_CONFIGMODS},
    {(char *)"create", 2, 0, LOG_PCREATES},
    {(char *)"keyboard_commands", 2, 0, LOG_KBCOMMANDS},
    {(char *)"killing", 1, 0, LOG_KILLS},
    {(char *)"local", 3, 0, LOG_LOCAL},
    {(char *)"logins", 3, 0, LOG_LOGIN},
    {(char *)"network", 1, 0, LOG_NET},
    {(char *)"problems", 1, 0, LOG_PROBLEMS},
    {(char *)"security", 2, 0, LOG_SECURITY},
    {(char *)"shouts", 2, 0, LOG_SHOUTS},
    {(char *)"startup", 2, 0, LOG_STARTUP},
    {(char *)"suspect_commands", 2, 0, LOG_SUSPECTCMDS},
    {(char *)"time_usage", 1, 0, LOG_TIMEUSE},
    {(char *)"wizard", 1, 0, LOG_WIZARD},
    {(char *)"malloc", 1, 0, LOG_MALLOC},
    {NULL, 0, 0, 0}};

LOGFILETAB logfds_table[] = {
    {LOG_ACCOUNTING, NULL, NULL},
    {LOG_ALLCOMMANDS, NULL, NULL},
    {LOG_BADCOMMANDS, NULL, NULL},
    {LOG_ALLOCATE, NULL, NULL},
    {LOG_BUGS, NULL, NULL},
    {LOG_DBSAVES, NULL, NULL},
    {LOG_CONFIGMODS, NULL, NULL},
    {LOG_PCREATES, NULL, NULL},
    {LOG_KBCOMMANDS, NULL, NULL},
    {LOG_KILLS, NULL, NULL},
    {LOG_LOCAL, NULL, NULL},
    {LOG_LOGIN, NULL, NULL},
    {LOG_NET, NULL, NULL},
    {LOG_PROBLEMS, NULL, NULL},
    {LOG_SECURITY, NULL, NULL},
    {LOG_SHOUTS, NULL, NULL},
    {LOG_STARTUP, NULL, NULL},
    {LOG_SUSPECTCMDS, NULL, NULL},
    {LOG_TIMEUSE, NULL, NULL},
    {LOG_WIZARD, NULL, NULL},
    {LOG_MALLOC, NULL, NULL},
    {0, NULL, NULL}};

/* ---------------------------------------------------------------------------
 * logfile_init: Initialize the main logfile.
 */

char *logfile_init(char *filename)
{
    if (!filename)
    {
        mainlog_fp = stderr;
        return (NULL);
    }
    else if (strstr(filename, "XXXXXX") != NULL)
    {
        mainlog_fp = fmkstemp(filename);
    }
    else
    {
        mainlog_fp = fopen(filename, "a");
    }

    if (!mainlog_fp)
    {
        fprintf(stderr, "Could not open logfile %s for writing.\n", filename);
        mainlog_fp = stderr;
        return (NULL);
    }

    setbuf(mainlog_fp, NULL); /* unbuffered */
    return (filename);
}

void logfile_move(char *oldfn, char *newfn)
{
    fclose(mainlog_fp);
    copy_file(oldfn, newfn, 1);
    logfile_init(newfn);
}

/* ---------------------------------------------------------------------------
 * start_log: see if it is OK to log something, and if so, start writing the
 * log entry.
 */

int start_log(const char *primary, const char *secondary, int key)
{
    struct tm *tp;
    time_t now;
    LOGFILETAB *lp;
    int last_key = 0;
    char *pri, *sec;

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
                {
                    log_fp = mainlog_fp;
                }
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

    if (mudstate.logging)
    {
        if (key & LOG_FORCE)
        {
            /*
	     * Log even if we are recursing and
	     * don't complain about it. This should
	     * never happens with the new logger.
	     */
            mudstate.logging--;
        }

        if (!mudstate.standalone)
        {
            /*
	     * Format the timestamp
	     */
            if ((mudconf.log_info & LOGOPT_TIMESTAMP) != 0)
            {
                time((time_t *)(&now));
                tp = localtime((time_t *)(&now));
                log_write_raw(0, "%02d%02d%02d.%02d%02d%02d ", (tp->tm_year % 100), tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
            }

            /*
	     * Write the header to the log
	     */

            if (secondary && *secondary)
            {
                pri = strndup(primary, 3);
                sec = strndup(secondary, 5);

                if (log_pos == NULL)
                {
                    log_write_raw(0, "%s %3s/%-5s: ", (*(mudconf.mud_shortname) ? (mudconf.mud_shortname) : (mudconf.mud_name)), pri, sec);
                }
                else
                {
                    log_write_raw(0, "%s %3s/%-5s (%s): ", (*(mudconf.mud_shortname) ? (mudconf.mud_shortname) : (mudconf.mud_name)), pri, sec, log_pos);
                }

                free(sec);
                free(pri);
            }
            else
            {
                pri = strndup(primary, 9);

                if (log_pos == NULL)
                {
                    log_write_raw(0, "%s %-9s: ", (*(mudconf.mud_shortname) ? (mudconf.mud_shortname) : (mudconf.mud_name)), pri);
                }
                else
                {
                    log_write_raw(0, "%s %-9s (%s): ", (*(mudconf.mud_shortname) ? (mudconf.mud_shortname) : (mudconf.mud_name)), pri, log_pos);
                }

                free(pri);
            }
        }

        /*
	 * If a recursive call, log it and return indicating no log
	 */

        if (mudstate.logging != 1)
        {
            log_write_raw(0, "Recursive logging request.\n");
        }

        return (1);
    }

    return 0;
}

/* ---------------------------------------------------------------------------
 * end_log: Finish up writing a log entry
 */

void end_log(void)
{
    log_write_raw(0, "\n");

    if (log_fp != NULL)
    {
        fflush(log_fp);
    }

    mudstate.logging--;

    if (mudstate.logging < 0)
    {
        log_write_raw(1, "Log was closed too many times (%d)\n", mudstate.logging);
        mudstate.logging = 0;
    }
}

/* ---------------------------------------------------------------------------
 * log_perror: Write perror message to the log
 */

void _log_perror(const char *primary, const char *secondary, const char *extra, const char *failing_object)
{
    int my_errno = errno;

    if (extra && *extra)
    {
        log_write(LOG_ALWAYS, primary, secondary, "(%s) %s: %s", extra, failing_object, strerror(my_errno));
    }
    else
    {
        log_write(LOG_ALWAYS, primary, secondary, "%s: %s", failing_object, strerror(my_errno));
    }
}

/* ---------------------------------------------------------------------------
 * log_write: Format text and print to the log file.
 */

void _log_write(int key, const char *primary, const char *secondary, const char *format, ...)
{
    va_list ap;
    char s[MBUF_SIZE];

    /*
     * Since the malloc functions now call this,
     * we should avoid doing malloc stuff in
     * the logger...
     */

    if ((((key)&mudconf.log_options) != 0) && start_log(primary, secondary, key))
    {
        va_start(ap, format);
        vsnprintf(s, MBUF_SIZE - 1, format, ap);
        va_end(ap);

        /*
	 * Do we have a logfile to write to...
	 */

        if ((log_fp != NULL))
        {
            fputs(s, log_fp);
        }

        /*
	 * If we are starting up, log to stderr too..
	 */

        if ((log_fp != stderr) && (mudstate.logstderr))
        {
            fputs(s, stderr);
        }

        end_log();
    }
}

/* ---------------------------------------------------------------------------
 * log_write_raw: Print text to the log or mainlog file.
 */

void log_write_raw(int key, const char *format, ...)
{
    va_list ap;
    char s[MBUF_SIZE];
    FILE *lfp;

    if (key)
    {
        lfp = mainlog_fp;
    }
    else
    {
        lfp = log_fp;
    }

    va_start(ap, format);
    vsnprintf(s, MBUF_SIZE - 1, format, ap);

    /*
     * Do we have a logfile to write to...
     */

    if (lfp != NULL)
    {
        fputs(s, lfp);
    }

    /*
     * If we are starting up, log to stderr too..
     */

    if ((log_fp != stderr) && (mudstate.logstderr))
    {
        fputs(s, stderr);
    }

    va_end(ap);
}

/* ---------------------------------------------------------------------------
 * log_getname : return the name of <target>. It is the responsibilty of
 * the caller to free the buffer with XFREE().
 */

char *log_getname(dbref target)
{
    char *name, *s;

    if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
    {
        s = unparse_object((dbref)GOD, target, 0);
    }
    else
    {
        s = unparse_object_numonly(target);
    }

    name = strip_ansi(s);
    XFREE(s);
    return (name);
}

char *log_gettype(dbref thing, char *d)
{
    if (!Good_dbref(thing))
    {
        return (XSTRDUP("??OUT-OF-RANGE??", d));
    }

    switch (Typeof(thing))
    {
    case TYPE_PLAYER:
        return (XSTRDUP("PLAYER", d));

    case TYPE_THING:
        return (XSTRDUP("THING", d));

    case TYPE_ROOM:
        return (XSTRDUP("ROOM", d));

    case TYPE_EXIT:
        return (XSTRDUP("EXIT", d));

    case TYPE_GARBAGE:
        return (XSTRDUP("GARBAGE", d));

    default:
        return (XSTRDUP("??ILLEGAL??", d));
    }
}

/*
 * ----------------------------------------------------------------------
 * Log rotation.
 */

void do_logrotate(dbref player, dbref cause, int key)
{
    LOGFILETAB *lp;
    char *ts, *pname;
    char s[MBUF_SIZE];
    ts = mktimestamp();
    mudstate.mudlognum++;

    if (mainlog_fp == stderr)
    {
        notify(player, "Warning: can't rotate main log when logging to stderr.");
    }
    else
    {
        fclose(mainlog_fp);
        snprintf(s, MBUF_SIZE, "%s.%s", mudconf.log_file, ts);
        copy_file(mudconf.log_file, s, 1);
        logfile_init(mudconf.log_file);
    }

    notify(player, "Logs rotated.");
    pname = log_getname(player);
    log_write(LOG_ALWAYS, "WIZ", "LOGROTATE", "%s: logfile rotation %d", pname, mudstate.mudlognum);
    XFREE(pname);

    /*
     * Any additional special ones
     */

    for (lp = logfds_table; lp->log_flag; lp++)
    {
        if (lp->filename && lp->fileptr)
        {
            fclose(lp->fileptr);
            snprintf(s, MBUF_SIZE, "%s.%s", lp->filename, ts);
            copy_file(lp->filename, s, 1);
            lp->fileptr = fopen(lp->filename, "a");

            if (lp->fileptr)
            {
                setbuf(lp->fileptr, NULL);
            }
        }
    }

    XFREE(ts);
}

void logfile_close(void)
{
    LOGFILETAB *lp;
    char s[MBUF_SIZE];
    char *ts;
    ts = mktimestamp();

    for (lp = logfds_table; lp->log_flag; lp++)
    {
        if (lp->filename && lp->fileptr)
        {
            fclose(lp->fileptr);
            snprintf(s, MBUF_SIZE, "%s.%s", lp->filename, ts);
            copy_file(lp->filename, s, 1);
        }
    }

    if (mainlog_fp != stderr)
    {
        fclose(mainlog_fp);
        snprintf(s, MBUF_SIZE, "%s.%s", mudconf.log_file, ts);
        copy_file(mudconf.log_file, s, 1);
    }

    XFREE(ts);
}
