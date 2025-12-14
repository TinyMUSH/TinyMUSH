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

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <errno.h>
#include <string.h>

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
        if (!mainlog_fp)
        {
            fprintf(stderr, "Could not create temporary logfile %s.\n", filename);
            mainlog_fp = stderr;
            return (NULL);
        }
    }
    else
    {
        mainlog_fp = fopen(filename, "a");
        if (!mainlog_fp)
        {
            fprintf(stderr, "Could not open logfile %s for writing.\n", filename);
            mainlog_fp = stderr;
            return (NULL);
        }
    }

    /* Only unbuffer if not using stderr (don't modify system stderr) */
    if (mainlog_fp != stderr)
    {
        setbuf(mainlog_fp, NULL); /* unbuffered */
    }

    return (filename);
}

void logfile_move(char *oldfn, char *newfn)
{
    if (!oldfn || !newfn)
    {
        fprintf(stderr, "Error: Invalid parameters to logfile_move\n");
        return;
    }

    if (mainlog_fp != NULL && mainlog_fp != stderr)
    {
        fclose(mainlog_fp);
        mainlog_fp = NULL;
    }

    if (copy_file(oldfn, newfn, 1) != 0)
    {
        fprintf(stderr, "Warning: Failed to copy log file from %s to %s\n", oldfn, newfn);
    }

    if (logfile_init(newfn) == NULL)
    {
        fprintf(stderr, "Error: Failed to initialize new log file %s, logging to stderr\n", newfn);
    }
}

/* ---------------------------------------------------------------------------
 * start_log: see if it is OK to log something, and if so, start writing the
 * log entry.
 */

int start_log(const char *primary, const char *secondary, int key)
{
    time_t now;
    LOGFILETAB *lp;
    static int last_key = 0;
    char *pri = NULL, *sec = NULL;

    if (!primary)
    {
        return 0;
    }

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
                log_fp = NULL;

                if (logfds_table != NULL)
                {
                    for (lp = logfds_table; lp->log_flag; lp++)
                    {
                        /*
                         * Though keys can be OR'd, use the first one
                         * * matched
                         */
                        if (lp->log_flag & key)
                        {
                            if (lp->fileptr != NULL)
                            {
                                log_fp = lp->fileptr;
                            }
                            break;
                        }
                    }
                }

                if (!log_fp)
                {
                    log_fp = mainlog_fp;
                }
            }
            /* else: key == last_key, use cached log_fp but verify it's still valid */
            else if (!log_fp)
            {
                /* Cache is invalid, reset to mainlog */
                log_fp = mainlog_fp;
            }
        }
        else
        {
            last_key = 0; /* Not in diversion mode, clear cache */
            log_fp = mainlog_fp;
        }
    }
    else
    {
        log_fp = mainlog_fp;
    }

    /* Sanity check: ensure we have a valid log_fp */
    if (!log_fp)
    {
        fprintf(stderr, "Error: Invalid log file pointer in start_log\n");
        return 0;
    }

    mushstate.logging++;

    if (mushstate.logging > 1)
    {
        if (key & LOG_FORCE)
        {
            /*
             * Log even if we are recursing and
             * don't complain about it. This should
             * never happens with the new logger.
             * Note: We don't decrement here, we'll let end_log do it
             */
        }
        else
        {
            /*
             * Recursive call, log it and return indicating no log
             */
            log_write_raw(0, "Recursive logging request.\n");
            mushstate.logging--;
            return (0);
        }
    }

    if (!mushstate.standalone)
    {
        /*
         * Format the timestamp
         */
        if ((mushconf.log_info & LOGOPT_TIMESTAMP) != 0)
        {
            now = time(NULL);
            struct tm tp;
            localtime_r(&now, &tp);
            log_write_raw(0, "%02d%02d%02d.%02d%02d%02d ", (tp.tm_year % 100), tp.tm_mon + 1, tp.tm_mday, tp.tm_hour, tp.tm_min, tp.tm_sec);
        }

        /*
         * Write the header to the log
         */
        const char *mush_name = (*(mushconf.mush_shortname) ? (mushconf.mush_shortname) : (mushconf.mush_name));

        if (!mush_name)
        {
            mush_name = "MUSH";
        }

        if (secondary && *secondary)
        {
            pri = strndup(primary, 3);
            sec = strndup(secondary, 5);

            if (pri == NULL || sec == NULL)
            {
                free(pri);
                free(sec);
                end_log();
                return 0;
            }

            if (log_pos == NULL)
            {
                log_write_raw(0, "%s %3s/%-5s: ", mush_name, pri, sec);
            }
            else
            {
                log_write_raw(0, "%s %3s/%-5s (%s): ", mush_name, pri, sec, log_pos);
            }

            free(sec);
            free(pri);
        }
        else
        {
            pri = strndup(primary, 9);

            if (pri == NULL)
            {
                end_log();
                return 0;
            }

            if (log_pos == NULL)
            {
                log_write_raw(0, "%s %-9s: ", mush_name, pri);
            }
            else
            {
                log_write_raw(0, "%s %-9s (%s): ", mush_name, pri, log_pos);
            }

            free(pri);
        }
    }

    return (1);
}

/* ---------------------------------------------------------------------------
 * end_log: Finish up writing a log entry
 */

void end_log(void)
{
    log_write_raw(0, "\n");

    if (log_fp != NULL)
    {
        if (fflush(log_fp) == EOF)
        {
            /* Don't use log_write_raw to avoid recursion */
            if (log_fp != stderr)
            {
                fprintf(stderr, "Error: Failed to flush log file\n");
            }
        }
    }

    mushstate.logging--;

    if (mushstate.logging < 0)
    {
        /* Use fprintf directly to avoid recursion */
        if (mainlog_fp != NULL)
        {
            fprintf(mainlog_fp, "Log was closed too many times (%d)\n", mushstate.logging);
        }
        mushstate.logging = 0;
    }
}

/* ---------------------------------------------------------------------------
 * log_perror: Write perror message to the log
 */

void _log_perror(const char *file, int line, const char *primary, const char *secondary, const char *extra, const char *failing_object)
{
    int my_errno = errno;
    char errbuf[256];

    /* Use XSI-compliant strerror_r (returns int) */
    if (strerror_r(my_errno, errbuf, sizeof(errbuf)) != 0)
    {
        snprintf(errbuf, sizeof(errbuf), "Unknown error %d", my_errno);
    }

    if (!failing_object)
    {
        failing_object = "(null)";
    }

    if (extra && *extra)
    {
        _log_write(file, line, LOG_ALWAYS, primary, secondary, "(%s) %s: %s", extra, failing_object, errbuf);
    }
    else
    {
        _log_write(file, line, LOG_ALWAYS, primary, secondary, "%s: %s", failing_object, errbuf);
    }
}

/* ---------------------------------------------------------------------------
 * log_write: Format text and print to the log file.
 */

void _log_write(const char *file, int line, int key, const char *primary, const char *secondary, const char *format, ...)
{
    if (!format)
    {
        fprintf(stderr, "Error: NULL format string in _log_write\n");
        return;
    }

    if ((((key)&mushconf.log_options) != 0) && start_log(primary, secondary, key))
    {
        int size = 0;
        char *str = NULL, *str1 = NULL;
        va_list ap;

        if (!file)
        {
            file = "<unknown>";
        }

        va_start(ap, format);
        size = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if (size < 0)
        {
            end_log();
            return;
        }

        size++;
        str = calloc(size, sizeof(char)); // Don't track the logger...

        if (str == NULL)
        {
            end_log();
            return;
        }

        va_start(ap, format);
        vsnprintf(str, size, format, ap);
        va_end(ap);

        if (mushstate.debug)
        {
            str1 = XNASPRINTF("%s:%d %s", file, line, str);
        }

        /* Do we have a logfile to write to... */
        if ((log_fp != NULL))
        {
            if (mushstate.debug && str1 != NULL)
            {
                if (fputs(str1, log_fp) == EOF)
                {
                    fprintf(stderr, "Error writing to log file\n");
                }
            }
            else
            {
                if (fputs(str, log_fp) == EOF)
                {
                    fprintf(stderr, "Error writing to log file\n");
                }
            }
        }

        /* If we are starting up, log to stderr too.. */

        if ((log_fp != stderr) && (mushstate.logstderr))
        {
            if (mushstate.debug && str1 != NULL)
            {
                fputs(str1, stderr); /* Ignore errors on stderr */
            }
            else
            {
                fputs(str, stderr); /* Ignore errors on stderr */
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
    int size = 0;
    char *str = NULL;
    va_list ap;
    FILE *lfp;

    if (!format)
    {
        fprintf(stderr, "Error: NULL format string in log_write_raw\n");
        return;
    }

    va_start(ap, format);
    size = vsnprintf(NULL, 0, format, ap);
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
    vsnprintf(str, size, format, ap);
    va_end(ap);

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
        if (fputs(str, lfp) == EOF)
        {
            /* Only report error if not writing to stderr (avoid recursion) */
            if (lfp != stderr)
            {
                fprintf(stderr, "Error writing to log file\n");
            }
        }
    }

    /* If we are starting up, log to stderr too (but avoid duplicating if already writing to stderr) */
    if ((lfp != stderr) && (mushstate.logstderr))
    {
        fputs(str, stderr); /* Ignore errors on stderr */
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

    if (!name)
    {
        return XSTRDUP("<error>", "log_getname");
    }

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

    if (mushstate.logging > 0)
    {
        notify(player, "Error: Cannot rotate logs while logging is in progress.");
        return;
    }

    ts = mktimestamp();

    if (!ts)
    {
        notify(player, "Error: Failed to generate timestamp for log rotation.");
        return;
    }

    mushstate.mush_lognum++;

    if (mainlog_fp == stderr)
    {
        notify(player, "Warning: can't rotate main log when logging to stderr.");
    }
    else
    {
        /* Log the rotation before closing files */
        pname = log_getname(player);
        log_write(LOG_ALWAYS, "WIZ", "LOGROTATE", "%s: logfile rotation %d", pname, mushstate.mush_lognum);
        XFREE(pname);

        fclose(mainlog_fp);
        mainlog_fp = NULL;
        s = XASPRINTF("s", "%s.%s", mushconf.log_file, ts);

        if (s)
        {
            copy_file(mushconf.log_file, s, 1);
            XFREE(s);
        }
        else
        {
            fprintf(stderr, "Error: Failed to allocate memory for log rotation\n");
        }

        if (logfile_init(mushconf.log_file) == NULL)
        {
            fprintf(stderr, "Error: Failed to reinitialize main log after rotation\n");
        }
    }

    notify(player, "Logs rotated.");

    /*
     * Any additional special ones
     */

    if (logfds_table != NULL)
    {
        for (lp = logfds_table; lp->log_flag; lp++)
        {
            if (lp->filename && lp->fileptr)
            {
                /* Reset log_fp if it points to this file */
                if (log_fp == lp->fileptr)
                {
                    log_fp = mainlog_fp;
                }

                fclose(lp->fileptr);
                s = XASPRINTF("s", "%s.%s", lp->filename, ts);

                if (s)
                {
                    copy_file(lp->filename, s, 1);
                    XFREE(s);
                }

                lp->fileptr = fopen(lp->filename, "a");

                if (lp->fileptr)
                {
                    setbuf(lp->fileptr, NULL);
                }
                else
                {
                    lp->fileptr = NULL;
                    fprintf(stderr, "Error: Failed to reopen log file %s after rotation\n", lp->filename);
                }
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

    if (mushstate.logging > 0)
    {
        fprintf(stderr, "Warning: Closing log files while logging is in progress (count=%d)\n", mushstate.logging);
        /* Continue anyway, we're shutting down */
    }

    ts = mktimestamp();

    if (!ts)
    {
        fprintf(stderr, "Error: Failed to generate timestamp for log closure, skipping file archiving\n");
        /* Still close files even if we can't archive them */
        if (logfds_table != NULL)
        {
            for (lp = logfds_table; lp->log_flag; lp++)
            {
                if (lp->fileptr)
                {
                    fclose(lp->fileptr);
                    lp->fileptr = NULL;
                }
            }
        }
        if (mainlog_fp != stderr)
        {
            fclose(mainlog_fp);
            mainlog_fp = NULL;
        }
        return;
    }

    if (logfds_table != NULL)
    {
        for (lp = logfds_table; lp->log_flag; lp++)
        {
            if (lp->filename && lp->fileptr)
            {
                fclose(lp->fileptr);
                lp->fileptr = NULL;
                s = XASPRINTF("s", "%s.%s", lp->filename, ts);

                if (s)
                {
                    copy_file(lp->filename, s, 1);
                    XFREE(s);
                }
            }
        }
    }

    if (mainlog_fp != stderr)
    {
        fclose(mainlog_fp);
        mainlog_fp = NULL;
        s = XASPRINTF("s", "%s.%s", mushconf.log_file, ts);

        if (s)
        {
            copy_file(mushconf.log_file, s, 1);
            XFREE(s);
        }
    }

    XFREE(ts);
}
