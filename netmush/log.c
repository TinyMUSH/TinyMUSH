/**
 * @file log.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Handle log files and log events
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 * @warning Be carefull that functions doesn't go circural when calling one of the XMALLOC functions.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

FILE *mainlog_fp = NULL; /*!< Pointer to the main log file */
FILE *log_fp = NULL;     /*!< Pointer to the facility's log file */

char *log_pos = NULL;


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

    if (!mushstate.standalone)
    {
        if (mushconf.log_diversion & key)
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

    mushstate.logging++;

    if (mushstate.logging)
    {
        if (key & LOG_FORCE)
        {
            /*
	     * Log even if we are recursing and
	     * don't complain about it. This should
	     * never happens with the new logger.
	     */
            mushstate.logging--;
        }

        if (!mushstate.standalone)
        {
            /*
	     * Format the timestamp
	     */
            if ((mushconf.log_info & LOGOPT_TIMESTAMP) != 0)
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
                    log_write_raw(0, "%s %3s/%-5s: ", (*(mushconf.mush_shortname) ? (mushconf.mush_shortname) : (mushconf.mush_name)), pri, sec);
                }
                else
                {
                    log_write_raw(0, "%s %3s/%-5s (%s): ", (*(mushconf.mush_shortname) ? (mushconf.mush_shortname) : (mushconf.mush_name)), pri, sec, log_pos);
                }

                free(sec);
                free(pri);
            }
            else
            {
                pri = strndup(primary, 9);

                if (log_pos == NULL)
                {
                    log_write_raw(0, "%s %-9s: ", (*(mushconf.mush_shortname) ? (mushconf.mush_shortname) : (mushconf.mush_name)), pri);
                }
                else
                {
                    log_write_raw(0, "%s %-9s (%s): ", (*(mushconf.mush_shortname) ? (mushconf.mush_shortname) : (mushconf.mush_name)), pri, log_pos);
                }

                free(pri);
            }
        }

        /*
	 * If a recursive call, log it and return indicating no log
	 */

        if (mushstate.logging != 1)
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

    mushstate.logging--;

    if (mushstate.logging < 0)
    {
        log_write_raw(1, "Log was closed too many times (%d)\n", mushstate.logging);
        mushstate.logging = 0;
    }
}

/* ---------------------------------------------------------------------------
 * log_perror: Write perror message to the log
 */

void _log_perror(const char *file, int line, const char *primary, const char *secondary, const char *extra, const char *failing_object)
{
    int my_errno = errno;

    if (extra && *extra)
    {
        _log_write(file, line, LOG_ALWAYS, primary, secondary, "(%s) %s: %s", extra, failing_object, strerror(my_errno));
    }
    else
    {
        _log_write(file, line, LOG_ALWAYS, primary, secondary, "%s: %s", failing_object, strerror(my_errno));
    }
}

/* ---------------------------------------------------------------------------
 * log_write: Format text and print to the log file.
 */

void _log_write(const char *file, int line, int key, const char *primary, const char *secondary, const char *format, ...)
{
    if ((((key)&mushconf.log_options) != 0) && start_log(primary, secondary, key))
    {
        int size = 0, vsize = 0;
        char *str = NULL, *str1 = NULL;
        va_list ap;

        va_start(ap, format);
        size = vsnprintf(str, size, format, ap);
        va_end(ap);

        if (size < 0)
        {
            return;
        }

        size++;
        str = calloc(size, sizeof(char)); // Don't track the logger...

        if (str == NULL)
        {
            return;
        }

        va_start(ap, format);
        vsize = vsnprintf(str, size, format, ap);
        va_end(ap);

        if (vsize < 0)
        {
            free(str);
            return;
        }

        if (mushstate.debug)
        {
            str1 = XNASPRINTF("%s:%d %s", file, line, str);
        }

        /* Do we have a logfile to write to... */
        if ((log_fp != NULL))
        {
            if (mushstate.debug)
            {
                fputs(str1, log_fp);
            }
            else
            {
                fputs(str, log_fp);
            }
        }

        /* If we are starting up, log to stderr too.. */

        if ((log_fp != stderr) && (mushstate.logstderr))
        {
            if (mushstate.debug)
            {
                fputs(str1, stderr);
            }
            else
            {
                fputs(str, stderr);
            }
        }

        XFREE(str1);
        free(str);
        
        end_log();
    }
}

/* ---------------------------------------------------------------------------
 * log_write_raw: Print text to the log or mainlog file.
 */
void log_write_raw(int key, const char *format, ...)
{
    int size = 0, vsize = 0;
    char *str = NULL;
    va_list ap;
    FILE *lfp;

    va_start(ap, format);
    size = vsnprintf(str, size, format, ap);
    va_end(ap);

    if (size < 0)
    {
        return;
    }

    size++;
    str = calloc(size, sizeof(char)); // Don't track the logger...

    if (str == NULL)
    {
        return;
    }

    va_start(ap, format);
    vsize = vsnprintf(str, size, format, ap);
    va_end(ap);

    if (vsize < 0)
    {
        free(str);
        return;
    }

    if (key)
    {
        lfp = mainlog_fp;
    }
    else
    {
        lfp = log_fp;
    }

    /* Do we have a logfile to write to... */
    if (lfp != NULL)
    {
        fputs(str, lfp);
    }

    /* If we are starting up, log to stderr too.. */
    if ((log_fp != stderr) && (mushstate.logstderr))
    {
        fputs(str, stderr);
    }

    free(str);
}

/* ---------------------------------------------------------------------------
 * log_getname : return the name of <target>. It is the responsibilty of
 * the caller to free the buffer with XFREE().
 */

char *log_getname(dbref target)
{
    char *name, *s;

    if ((mushconf.log_info & LOGOPT_FLAGS) != 0)
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

void do_logrotate(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
    LOGFILETAB *lp;
    char *ts, *pname;
    char *s;
    ts = mktimestamp();
    mushstate.mush_lognum++;

    if (mainlog_fp == stderr)
    {
        notify(player, "Warning: can't rotate main log when logging to stderr.");
    }
    else
    {
        fclose(mainlog_fp);
        s = XASPRINTF("s", "%s.%s", mushconf.log_file, ts);
        copy_file(mushconf.log_file, s, 1);
        XFREE(s);
        logfile_init(mushconf.log_file);
    }

    notify(player, "Logs rotated.");
    pname = log_getname(player);
    log_write(LOG_ALWAYS, "WIZ", "LOGROTATE", "%s: logfile rotation %d", pname, mushstate.mush_lognum);
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
    char *s;
    char *ts;
    ts = mktimestamp();

    for (lp = logfds_table; lp->log_flag; lp++)
    {
        if (lp->filename && lp->fileptr)
        {
            fclose(lp->fileptr);
            s = XASPRINTF("s", "%s.%s", lp->filename, ts);
            copy_file(lp->filename, s, 1);
            XFREE(s);
        }
    }

    if (mainlog_fp != stderr)
    {
        fclose(mainlog_fp);
        s = XASPRINTF("s", "%s.%s", mushconf.log_file, ts);
        copy_file(mushconf.log_file, s, 1);
        XFREE(s);
    }

    XFREE(ts);
}
