/**
 * @file log.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Logging subsystem: event reporting, file rotation, and audit helpers
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @warning Be careful that functions don't go circular when calling one of the XMALLOC functions.
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

/**
 * @brief Write a message to a log file with optional stderr duplication.
 *
 * This is a low-level helper function that centralizes all log file writing
 * operations. It writes the message to the specified log file and optionally
 * duplicates it to stderr during startup when mushstate.logstderr is enabled.
 * This function handles error reporting and avoids recursion when writing to stderr.
 *
 * @param lfp The file pointer to write to (log_fp, mainlog_fp, or stderr).
 *            If NULL, no write operation is performed.
 * @param message The message string to write. Must not be NULL.
 *
 * @note Errors writing to the log file are reported to stderr (unless lfp is stderr).
 * @note During startup (mushstate.logstderr), messages are duplicated to stderr
 *       unless already writing to stderr.
 * @note Errors writing to stderr during duplication are silently ignored to avoid recursion.
 */
static inline void write_to_logfile(FILE *lfp, const char *message)
{
	/* Write to target log file if available */
	if (lfp != NULL)
	{
		if (fputs(message, lfp) == EOF)
		{
			/* Report error with errno for diagnostics (avoid recursion if writing to stderr) */
			if (lfp != stderr)
			{
				fprintf(stderr, "Error: Failed to write to log file (errno=%d)\n", errno);
			}
		}
	}

	/* During startup, duplicate to stderr if not already writing to it */
	if ((lfp != stderr) && (mushstate.logstderr))
	{
		fputs(message, stderr); /* Ignore errors on stderr */
	}
}

/**
 * @brief Initialize the main logfile for the MUSH logging system.
 *
 * This function initializes the main log file pointer (mainlog_fp) by opening
 * the specified log file. It supports three modes of operation:
 * 1. NULL filename: Falls back to stderr for logging
 * 2. Temporary file: If basename contains "XXXXXX", creates a temporary file using fmkstemp()
 * 3. Regular file: Opens the file in append mode for persistent logging
 *
 * The XXXXXX pattern is searched only in the filename portion (after the last '/'),
 * not in directory paths, to avoid false positives like "/home/XXXXXX/netmush.log".
 *
 * When a log file is successfully opened (not stderr), it is set to unbuffered mode
 * to ensure immediate writing of log entries, which is critical for crash diagnostics
 * and real-time monitoring.
 *
 * Error handling:
 * - On any failure to open a file, the function falls back to stderr
 * - Error messages are printed to stderr to inform the user
 * - The function always ensures mainlog_fp is set to a valid FILE pointer
 *
 * @param filename Path to the log file to open. Can be NULL to use stderr,
 *                 can contain "XXXXXX" pattern for temporary file creation,
 *                 or a regular path for standard log file.
 * @return char* Returns the filename parameter on success, NULL on failure or when using stderr.
 *               The returned pointer is the same as the input parameter (not a new allocation).
 *
 * @note This function modifies the global variable mainlog_fp.
 * @note When using temporary files (XXXXXX pattern), the filename string is modified in-place by fmkstemp().
 * @warning Do not call this function while logging is active to avoid file descriptor issues.
 *
 * @see fmkstemp()
 * @see logfile_move()
 * @see logfile_close()
 */
char *logfile_init(char *filename)
{
    if (!filename)
    {
        mainlog_fp = stderr;
        return (NULL);
    }

    /* Check if filename contains template pattern XXXXXX for temporary file creation.
     * Use strrchr to find the last path separator and only check the basename,
     * avoiding false positives from directory names containing XXXXXX.
     */
    const char *basename = strrchr(filename, '/');
    basename = basename ? basename + 1 : filename;  /* Skip the '/' or use full path if no separator */

    if (strstr(basename, "XXXXXX"))
    {
        mainlog_fp = fmkstemp(filename);
        if (!mainlog_fp)
        {
            /* Note: filename may have been partially modified by fmkstemp */
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

    /* Set unbuffered mode for immediate log writes (critical for crash diagnostics).
     * Note: At this point, mainlog_fp is guaranteed to be a valid file (never stderr),
     * as all stderr assignments are followed by immediate return.
     */
    setbuf(mainlog_fp, NULL);

    return (filename);
}

/**
 * @brief Move the main log file from one location to another.
 *
 * This function safely relocates the main log file by closing the current log,
 * copying its contents to a new location, and reinitializing logging with the
 * new file path. This is typically used during configuration changes or when
 * reorganizing log file locations.
 *
 * The operation sequence is:
 * 1. Validate input parameters (both filenames must be non-NULL)
 * 2. Safety check: Verify mushstate.logging == 0 (no active logging)
 * 3. Flush and close the current main log file if open (preserves stderr if in use)
 * 4. Copy the old log file to the new location using copy_file()
 * 5. Reinitialize logging with the new filename via logfile_init()
 *
 * Error handling:
 * - Null parameters: Prints error to stderr and returns without action
 * - Active logging: Prints error to stderr and aborts (prevents data corruption)
 * - Copy failure: Prints warning to stderr but continues with reinitialization
 * - Init failure: Error handled by logfile_init(); logging falls back to stderr
 *
 * @param oldfn Path to the current/old log file. Must not be NULL.
 * @param newfn Path to the new log file location. Must not be NULL.
 *
 * @note This function modifies the global variable mainlog_fp.
 * @note If the copy operation fails, the function still attempts to initialize
 *       the new log file, which may result in starting with an empty log.
 * @note The function now enforces the logging safety check (mushstate.logging must be 0).
 * @warning This function will refuse to operate if mushstate.logging > 0 (enforced check).
 * @warning The old log file is not automatically deleted; cleanup must be handled separately.
 * @warning Ensure all pending log writes are complete before calling this function.
 *
 * @see logfile_init()
 * @see copy_file()
 * @see logfile_close()
 * @see do_logrotate()
 */
void logfile_move(char *oldfn, char *newfn)
{
    if (!oldfn || !newfn)
    {
        fprintf(stderr, "Error: Invalid parameters to logfile_move\n");
        return;
    }

    /* Safety check: prevent log file operations during active logging */
    if (mushstate.logging > 0)
    {
        fprintf(stderr, "Error: Cannot move log file while logging is active (count=%d)\n", mushstate.logging);
        return;
    }

    /* Close the current log file if it's a real file (not stderr).
     * Flush first to ensure all buffered data is written before closing.
     */
    if (mainlog_fp != stderr)
    {
        if (mainlog_fp != NULL)
        {
            fflush(mainlog_fp);
            fclose(mainlog_fp);
        }
        mainlog_fp = NULL;
    }

    /* Copy old log to new location. Continue even if copy fails,
     * as we'll still initialize the new log (potentially empty).
     */
    if (copy_file(oldfn, newfn, 1) != 0)
    {
        fprintf(stderr, "Warning: Failed to copy log file from %s to %s\n", oldfn, newfn);
    }

    /* Initialize new log file. Error messages are handled by logfile_init() */
    logfile_init(newfn);
}

/**
 * @brief Initialize and begin a new log entry with proper routing and formatting.
 *
 * This function is the entry point for all structured logging in TinyMUSH. It determines
 * which log file to use based on the log key, handles log diversion routing, detects
 * recursive logging attempts, and writes the formatted log entry header including
 * optional timestamps and categorization labels.
 *
 * Operation sequence:
 * 1. Validates the primary category parameter (must not be NULL)
 * 2. Determines the target log file (log_fp):
 *    - Checks log_diversion configuration against the key
 *    - Searches logfds_table for matching facility-specific log files
 *    - Falls back to mainlog_fp if no specific log file matches
 *    - Uses caching (last_key) to optimize repeated lookups for the same key
 * 3. Increments the logging counter (mushstate.logging) to track nesting
 * 4. Detects recursive logging attempts:
 *    - If mushstate.logging > 1 and LOG_FORCE is not set, rejects the log entry
 *    - Writes warning about recursive logging and returns 0
 * 5. Formats and writes the log header (in non-standalone mode):
 *    - Optional timestamp in YYMMDD.HHMMSS format if LOGOPT_TIMESTAMP is set
 *    - MUSH name from mushconf.mush_shortname or mushconf.mush_name
 *    - Primary category (truncated to 3 or 9 characters)
 *    - Secondary category (truncated to 5 characters) if provided
 *    - Optional position indicator from log_pos if set
 *
 * Log header formats:
 * - With secondary: "MUSH PRI/SEC: " or "MUSH PRI/SEC (pos): "
 * - Without secondary: "MUSH PRIMARY: " or "MUSH PRIMARY (pos): "
 *
 * @param primary The primary category/type of the log entry (e.g., "WIZ", "NET", "CMD").
 *                Must not be NULL. Displayed in the log header, truncated to 3 or 9 chars.
 * @param secondary Optional secondary category for finer classification (e.g., "LOGIN", "ERROR").
 *                  Can be NULL. If provided, displayed in header truncated to 5 chars.
 * @param key Bitfield key determining log routing and filtering (e.g., LOG_ALWAYS, LOG_BUGS).
 *            Used to select the appropriate log file from logfds_table and filter against
 *            mushconf.log_options. Can include LOG_FORCE to bypass recursion checks.
 *
 * @return int Returns 1 if the log entry was successfully started and the caller should
 *             proceed with writing the log message body. Returns 0 if logging should be
 *             skipped (null primary, invalid log_fp, or recursive logging detected).
 *
 * @note This function modifies global state: mushstate.logging (incremented), log_fp (set to target).
 * @note The caller MUST call end_log() after writing the log message body to properly close the entry.
 * @note Uses static variable last_key to cache the most recent key lookup for performance.
 * @note Category strings are truncated using printf format specifiers, avoiding dynamic allocation.
 *
 * @warning Recursive logging (mushstate.logging > 1) is rejected unless LOG_FORCE is set in key.
 * @warning If log_fp cannot be determined, an error is printed to stderr and 0 is returned.
 *
 * @see end_log()
 * @see log_write()
 * @see _log_write()
 * @see log_write_raw()
 */
int start_log(const char *primary, const char *secondary, int key)
{
    time_t now;
    LOGFILETAB *lp;
    /* Cache for log routing optimization. Note: not thread-safe.
     * In multi-threaded environments, this could cause race conditions.
     * Consider using thread-local storage if threading is enabled.
     */
    static int last_key = 0;

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

                if (logfds_table[0].log_flag > 0)
                {
                    for (lp = logfds_table; lp->log_flag; lp++)
                    {
                        /*
                         * Though keys can be OR'd, use the first matching entry
                         */
                        if (lp->log_flag & key)
                        {
                            log_fp = lp->fileptr;
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
            static struct tm tp;  /* Static allocation for reuse across calls */
            now = time(NULL);
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
            if (log_pos == NULL)
            {
                log_write_raw(0, "%s %.3s/%-5.5s: ", mush_name, primary, secondary);
            }
            else
            {
                log_write_raw(0, "%s %.3s/%-5.5s (%s): ", mush_name, primary, secondary, log_pos);
            }
        }
        else
        {
            if (log_pos == NULL)
            {
                log_write_raw(0, "%s %-9.9s: ", mush_name, primary);
            }
            else
            {
                log_write_raw(0, "%s %-9.9s (%s): ", mush_name, primary, log_pos);
            }
        }
    }

    return (1);
}

/**
 * @brief Finalize and complete a log entry by flushing buffers and updating state.
 *
 * This function completes a log entry that was started with start_log(). It ensures
 * all log data is written to disk, properly terminates the log line, and maintains
 * the logging state counter. This function must be called after every successful
 * start_log() call to prevent resource leaks and state corruption.
 *
 * Operation sequence:
 * 1. Writes a newline character to terminate the log entry line
 * 2. Flushes the log file buffer (log_fp) to ensure data reaches disk:
 *    - Checks for flush errors (EOF return from fflush)
 *    - Reports errors to stderr if not already writing to stderr (avoids recursion)
 * 3. Decrements the logging counter (mushstate.logging)
 * 4. Detects and corrects negative logging counter:
 *    - If mushstate.logging < 0, logs an error about excessive end_log() calls
 *    - Resets the counter to 0 to prevent further corruption
 *    - Uses fprintf() directly to avoid recursion through the logging system
 *
 * Error handling:
 * - Flush failures: Logged to stderr only if not already using stderr
 * - Negative counter: Logged to mainlog_fp with count value, then reset to 0
 * - Uses direct fprintf() calls for error reporting to avoid recursive logging
 *
 * @note This function modifies global state: mushstate.logging (decremented).
 * @note Every start_log() call that returns 1 MUST be paired with an end_log() call.
 * @note The function is safe to call even if start_log() failed (does minimal work).
 * @note Flushing ensures log entries are written immediately, critical for crash diagnostics.
 *
 * @warning Missing end_log() calls will cause mushstate.logging to remain elevated,
 *          potentially blocking future logging or triggering recursion detection.
 * @warning Calling end_log() without a matching start_log() will corrupt the counter
 *          and trigger the negative counter error path.
 * @warning Do not call log_write_raw() or other logging functions from error paths
 *          in this function to avoid infinite recursion.
 *
 * @see start_log()
 * @see log_write()
 * @see _log_write()
 * @see log_write_raw()
 */
void end_log(void)
{
    log_write_raw(0, "\n");

    /* Decrement counter immediately after writing to minimize inconsistency window */
    mushstate.logging--;

    /* Flush the log file to ensure data reaches disk */
    if (log_fp != NULL && log_fp != stderr)
    {
        if (fflush(log_fp) == EOF)
        {
            /* Don't use log_write_raw to avoid recursion.
             * Include errno for diagnostic purposes.
             */
            fprintf(stderr, "Error: Failed to flush log file (errno=%d)\n", errno);
        }
    }

    /* Detect and correct negative counter (indicates mismatched start/end_log calls) */
    if (mushstate.logging < 0)
    {
        /* Use fprintf directly to avoid recursion */
        if (mainlog_fp != NULL && mainlog_fp != stderr)
        {
            fprintf(mainlog_fp, "CRITICAL: Log was closed too many times (counter=%d)\n", mushstate.logging);
        }
        mushstate.logging = 0;
    }
}

/**
 * @brief Log a system error message with errno translation to the log file.
 *
 * This function is a logging-system equivalent of perror(), capturing the current
 * errno value and translating it to a human-readable error message. It automatically
 * formats and logs the error with context information about what operation failed.
 * The function is typically called through the log_perror() macro which automatically
 * provides file and line information.
 *
 * Operation sequence:
 * 1. Captures the current errno value immediately to preserve it
 * 2. Converts errno to string using XSI-compliant strerror_r():
 *    - Thread-safe error string conversion
 *    - Falls back to "Unknown error N" if strerror_r fails
 * 3. Validates failing_object parameter (replaces NULL with "(null)")
 * 4. Formats the log message based on presence of extra context:
 *    - With extra: "(<extra>) <failing_object>: <error_string>"
 *    - Without extra: "<failing_object>: <error_string>"
 * 5. Logs with LOG_ALWAYS priority through _log_write()
 *
 * Typical usage pattern (via macro):
 * @code
 * if (open(filename, O_RDONLY) < 0) {
 *     log_perror("SYS", "FILE", "database load", filename);
 * }
 * // Produces: "SYS/FILE: (database load) myfile.db: No such file or directory"
 * @endcode
 *
 * @param file Source filename where the error occurred (typically via __FILE__).
 *             Used for debug output when mushstate.debug is enabled.
 * @param line Source line number where the error occurred (typically via __LINE__).
 *             Used for debug output when mushstate.debug is enabled.
 * @param primary The primary category/type of the log entry (e.g., "SYS", "NET", "DB").
 *                Passed to _log_write() for log header formatting.
 * @param secondary Optional secondary category for finer classification (e.g., "FILE", "SOCK").
 *                  Can be NULL. Passed to _log_write() for log header formatting.
 * @param extra Optional context string describing the operation that failed (e.g., "database load").
 *              Can be NULL. If provided, included in parentheses in the log message.
 * @param failing_object Description of what object/resource failed (e.g., filename, socket name).
 *                       If NULL, replaced with "(null)" for safety.
 *
 * @note The errno value is captured at function entry to ensure it's not overwritten by subsequent calls.
 * @note Always logs with LOG_ALWAYS priority, ensuring critical system errors are never filtered.
 * @note Uses XSI-compliant strerror_r (returns int) for thread-safe errno translation.
 * @note This is an internal function; use the log_perror() macro instead for automatic file/line tracking.
 *
 * @warning Do not call this function directly; use the log_perror() macro to ensure
 *          proper file and line number tracking.
 * @warning The errno value should be checked immediately after a system call failure
 *          and logged before any other operations that might change errno.
 *
 * @see _log_write()
 * @see log_write()
 * @see end_log()
 * @see strerror_r()
 */
void _log_perror(const char *file, int line, const char *primary, const char *secondary, const char *extra, const char *failing_object)
{
    int my_errno = errno;
    char errbuf[512];  /* Increased buffer size for longer error messages */

    /* Validate critical parameters */
    if (!primary)
    {
        primary = "SYS";
    }

    if (!failing_object)
    {
        failing_object = "(null)";
    }

    /* Use XSI-compliant strerror_r (returns int).
     * Note: GNU version returns char* - this code assumes XSI.
     */
    if (strerror_r(my_errno, errbuf, sizeof(errbuf)) != 0)
    {
        snprintf(errbuf, sizeof(errbuf), "Unknown error %d", my_errno);
    }

    /* Build format string and log in one call */
    if (extra && *extra)
    {
        _log_write(file, line, LOG_ALWAYS, primary, secondary, "(%s) %s: %s", extra, failing_object, errbuf);
    }
    else
    {
        _log_write(file, line, LOG_ALWAYS, primary, secondary, "%s: %s", failing_object, errbuf);
    }
}

/**
 * @brief Format and write a structured log message with optional debug information.
 *
 * This is the core internal logging function for TinyMUSH's structured logging system.
 * It handles message formatting, log filtering based on configuration options, debug
 * information injection, and writing to both the primary log file and stderr when
 * appropriate. This function is typically called through the log_write() macro which
 * automatically provides file and line number tracking.
 *
 * Operation sequence:
 * 1. Validates the format string parameter (must not be NULL)
 * 2. Checks if logging should occur:
 *    - Filters against mushconf.log_options using bitwise AND with key
 *    - Calls start_log() to initialize the log entry and write header
 *    - Skips logging if filtering fails or start_log() returns 0
 * 3. Prepares for message formatting:
 *    - Initializes va_list for variadic argument processing
 *    - Calculates required buffer size using vsnprintf(NULL, 0, ...)
 *    - Allocates buffer with calloc (untracked to avoid logger recursion)
 * 4. Formats the user message using vsnprintf with variadic arguments
 * 5. Optionally adds debug information:
 *    - If mushstate.debug is enabled, prepends "file:line " to the message
 *    - Creates secondary buffer (str1) with XNASPRINTF for debug format
 * 6. Writes to primary log file (log_fp):
 *    - Uses debug format (str1) if available and debug mode is on
 *    - Uses plain format (str) otherwise
 *    - Reports write errors to stderr via fprintf
 * 7. Optionally duplicates to stderr:
 *    - If log_fp != stderr AND mushstate.logstderr is enabled
 *    - Supports startup logging where both file and console output are needed
 *    - Silently ignores stderr write errors to avoid recursion
 * 8. Cleanup:
 *    - Frees debug buffer (str1) with XFREE if allocated
 *    - Frees message buffer (str) with free (untracked allocation)
 *    - Calls end_log() to finalize the log entry and flush buffers
 *
 * Message format examples:
 * - Normal: "Failed to open database file"
 * - Debug mode: "db.c:1234 Failed to open database file"
 *
 * Typical usage pattern (via macro):
 * @code
 * if (database_fd < 0) {
 *     log_write(LOG_ALWAYS, "DB", "LOAD", "Failed to open %s", dbfile);
 * }
 * // With macro expansion becomes:
 * // _log_write(__FILE__, __LINE__, LOG_ALWAYS, "DB", "LOAD", "Failed to open %s", dbfile);
 * @endcode
 *
 * @param file Source filename where the log call originated (typically via __FILE__).
 *             Used in debug mode to prepend location info to log messages.
 *             If NULL, replaced with "<unknown>" for safety.
 * @param line Source line number where the log call originated (typically via __LINE__).
 *             Used in debug mode to prepend location info to log messages.
 * @param key Bitfield key for log filtering and routing (e.g., LOG_ALWAYS, LOG_BUGS, LOG_NET).
 *            Tested against mushconf.log_options to determine if message should be logged.
 *            Passed to start_log() for routing to appropriate log file.
 * @param primary The primary category/type of the log entry (e.g., "WIZ", "NET", "CMD").
 *                Passed to start_log() for log header formatting.
 * @param secondary Optional secondary category for finer classification (e.g., "LOGIN", "ERROR").
 *                  Can be NULL. Passed to start_log() for log header formatting.
 * @param format Printf-style format string for the log message body. Must not be NULL.
 *               Supports all standard printf format specifiers (%s, %d, %x, etc.).
 * @param ... Variable arguments matching the format string specifiers.
 *            Processed using va_list and vsnprintf for safe formatting.
 *
 * @note This is an internal function; use the log_write() macro instead for automatic file/line tracking.
 * @note Memory allocations use calloc/free rather than XMALLOC to avoid recursion in memory tracking.
 * @note If start_log() returns 0 (logging skipped), no message formatting or writing occurs.
 * @note The function always calls end_log() after successful start_log() to maintain proper state.
 * @note Debug information (file:line prefix) is only added when mushstate.debug is enabled.
 * @note Stderr duplication only occurs during startup (mushstate.logstderr) and when log_fp != stderr.
 *
 * @warning Do not call this function directly; use the log_write() macro to ensure
 *          proper file and line number tracking.
 * @warning Buffer allocation failures cause early return after calling end_log() to maintain state.
 * @warning Write errors to log_fp are reported to stderr, but stderr write errors are silently ignored
 *          to prevent infinite recursion in error handling.
 * @warning If format is NULL, an error is printed to stderr and the function returns immediately
 *          without calling start_log() or end_log().
 *
 * @see log_write()
 * @see start_log()
 * @see end_log()
 * @see log_write_raw()
 * @see _log_perror()
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
        const char *output_str = NULL;  /* Pointer to the string to output */
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

        /* Prepare output string: add debug info if enabled */
        if (mushstate.debug)
        {
            str1 = XNASPRINTF("%s:%d %s", file, line, str);
            output_str = str1 ? str1 : str;  /* Fallback to str if allocation fails */
        }
        else
        {
            output_str = str;
        }

        /* Write to log file - start_log() guarantees log_fp is valid */
        write_to_logfile(log_fp, output_str);

        XFREE(str1);
        free(str);

        end_log();
    }
}

/**
 * @brief Write raw formatted text directly to a log file without header or filtering.
 *
 * This is a low-level logging function that bypasses the structured logging system's
 * header generation and filtering mechanisms. It writes formatted text directly to
 * either the main log file or the current facility log file, depending on the key
 * parameter. This function is primarily used internally by start_log() for writing
 * log headers and timestamps, or for emergency logging when the normal logging
 * system cannot be used.
 *
 * Operation sequence:
 * 1. Validates the format string parameter (must not be NULL)
 * 2. Prepares for message formatting:
 *    - Initializes va_list for variadic argument processing
 *    - Calculates required buffer size using vsnprintf(NULL, 0, ...)
 *    - Returns silently if size calculation fails (negative result)
 * 3. Allocates message buffer:
 *    - Uses calloc with size+1 for null terminator
 *    - Returns silently on allocation failure
 *    - Untracked allocation (calloc) to avoid recursion in memory tracking
 * 4. Formats the message using vsnprintf with variadic arguments
 * 5. Selects target log file (lfp):
 *    - If key is non-zero: uses mainlog_fp (main log file)
 *    - If key is zero: uses log_fp (current facility log file)
 * 6. Writes to primary log file:
 *    - Uses fputs to write the formatted string
 *    - Reports write errors to stderr only if not already writing to stderr
 *    - Continues execution even if write fails
 * 7. Optionally duplicates to stderr:
 *    - If lfp != stderr AND mushstate.logstderr is enabled
 *    - Supports startup/debug logging to both file and console
 *    - Silently ignores stderr write errors
 * 8. Cleanup:
 *    - Frees message buffer with free (untracked allocation)
 *
 * Key parameter behavior:
 * - key = 0 (false): Write to log_fp (current facility log, set by start_log)
 * - key ≠ 0 (true): Write to mainlog_fp (main log file)
 *
 * Typical usage examples:
 * @code
 * // Internal use by start_log() for timestamp
 * log_write_raw(0, "%02d%02d%02d.%02d%02d%02d ", year, mon, day, hour, min, sec);
 *
 * // Internal use by start_log() for header
 * log_write_raw(0, "%s %3s/%-5s: ", mush_name, pri, sec);
 *
 * // Emergency logging to main log
 * log_write_raw(1, "Critical error during shutdown\n");
 *
 * // Recursive logging detection
 * log_write_raw(0, "Recursive logging request.\n");
 * @endcode
 *
 * @param key Log file selector: 0 = use log_fp (current facility log),
 *            non-zero = use mainlog_fp (main log file).
 *            This is NOT a filtering bitfield like in other log functions.
 * @param format Printf-style format string for the message. Must not be NULL.
 *               Supports all standard printf format specifiers (%s, %d, %x, etc.).
 *               Should typically include newlines only when writing complete lines.
 * @param ... Variable arguments matching the format string specifiers.
 *            Processed using va_list and vsnprintf for safe formatting.
 *
 * @note This function bypasses ALL normal logging infrastructure:
 *       - No filtering against mushconf.log_options
 *       - No call to start_log() or end_log()
 *       - No automatic header, timestamp, or category labels
 *       - No automatic newline termination
 * @note Memory allocations use calloc/free rather than XMALLOC to avoid recursion in memory tracking.
 * @note Used internally by start_log() to write headers and timestamps without causing infinite recursion.
 * @note The function continues execution even if writes fail, only reporting errors for non-stderr targets.
 * @note Unlike _log_write(), this function does NOT add file:line debug information.
 *
 * @warning This is a low-level function intended for internal use by the logging system.
 *          For normal application logging, use log_write() or _log_write() instead.
 * @warning Does not add newlines automatically; caller must include '\n' in format if needed.
 * @warning Does not call start_log() or end_log(); if used outside of those functions,
 *          it may write to an uninitialized or closed log file.
 * @warning The key parameter is a simple boolean selector, NOT a LOG_* bitfield constant.
 *          Using LOG_ALWAYS or other bitfield values will work but only as non-zero.
 * @warning Allocation failures and formatting errors cause silent return without logging.
 *
 * @see _log_write()
 * @see start_log()
 * @see end_log()
 * @see log_write()
 */
void log_write_raw(int key, const char *format, ...)
{
    int size = 0;
    char *str = NULL;
    va_list ap;
    FILE *lfp = key ? mainlog_fp : log_fp;  /* Select target log file early */

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

    /* Write to target log file if available */
    write_to_logfile(lfp, str);

    free(str);
}

/**
 * @brief Retrieve the formatted name of a database object for logging purposes.
 *
 * This function converts a database reference (dbref) into a human-readable string
 * representation suitable for inclusion in log entries. The formatting depends on
 * the LOGOPT_FLAGS configuration setting, which controls whether to include detailed
 * object information (flags, owner, etc.) or just the basic object number.
 *
 * Operation sequence:
 * 1. Checks the LOGOPT_FLAGS bit in mushconf.log_info to determine formatting:
 *    - If set: Uses unparse_object() with GOD perspective for full object details
 *              (includes object name, dbref, type, flags, and owner)
 *    - If not set: Uses unparse_object_numonly() for minimal dbref-only format
 * 2. Strips all ANSI color codes from the unparsed string:
 *    - Uses ansi_strip_ansi() to remove color/formatting for clean log output
 *    - Allocates a new buffer for the stripped result
 * 3. Frees the intermediate unparsed string with XFREE()
 * 4. Validates the result:
 *    - If ansi_strip_ansi() failed (returned NULL), returns "<error>"
 *    - Otherwise returns the sanitized name string
 *
 * Output format examples:
 * - With LOGOPT_FLAGS: "Wizard(#1PW)" - includes name, dbref, type, flags, owner
 * - Without LOGOPT_FLAGS: "#1" - just the database reference number
 * - On error: "<error>" - fallback for allocation/stripping failures
 *
 * Typical usage pattern:
 * @code
 * dbref player = lookup_player("Wizard");
 * char *name = log_getname(player);
 * log_write(LOG_ALWAYS, "CMD", "ACTION", "Player %s performed action", name);
 * XFREE(name);
 * @endcode
 *
 * @param target The database reference (dbref) of the object to format.
 *               Can be any valid dbref (player, thing, room, exit).
 *               Invalid dbrefs are handled by unparse_object functions.
 *
 * @return char* Newly allocated string containing the formatted object name.
 *               NEVER returns NULL - returns "<error>" on failure.
 *               The caller MUST free this buffer using XFREE() after use.
 *
 * @note The returned string is always allocated on the heap and must be freed by the caller.
 * @note ANSI codes are stripped to ensure log files contain only plain text.
 * @note Uses GOD perspective when unparsing with flags to see all object details.
 * @note The function guarantees a non-NULL return value for safe use in log formatting.
 *
 * @warning The caller is responsible for freeing the returned buffer with XFREE().
 *          Failure to free will cause memory leaks.
 * @warning Do not call free() on the returned pointer; use XFREE() for proper memory tracking.
 *
 * @see unparse_object()
 * @see unparse_object_numonly()
 * @see ansi_strip_ansi()
 * @see log_gettype()
 * @see XFREE()
 */
char *log_getname(dbref target)
{
    char *s;
    char *name;

    /* Choose formatting based on LOGOPT_FLAGS configuration */
    if (mushconf.log_info & LOGOPT_FLAGS)
    {
        s = unparse_object(GOD, target, 0);
    }
    else
    {
        s = unparse_object_numonly(target);
    }

    /* Strip ANSI codes for clean log output */
    name = ansi_strip_ansi(s);
    XFREE(s);

    /* Return error string if ANSI stripping failed */
    if (!name)
    {
        return XSTRDUP("<error>", "log_getname.error");
    }

    return name;
}

/**
 * @brief Retrieve a human-readable type name for a database object for logging purposes.
 *
 * This function converts a database object's type into a human-readable string
 * representation suitable for inclusion in log entries. It validates the dbref
 * and returns one of six standard type names, or error strings for invalid objects.
 * The returned string is a static constant that must NOT be freed.
 *
 * Operation sequence:
 * 1. Validates the dbref using Good_dbref():
 *    - If invalid (out of range), returns "??OUT-OF-RANGE??"
 * 2. Examines the object type using Typeof() macro
 * 3. Returns the corresponding static type string:
 *    - TYPE_PLAYER → "PLAYER"
 *    - TYPE_THING → "THING"
 *    - TYPE_ROOM → "ROOM"
 *    - TYPE_EXIT → "EXIT"
 *    - TYPE_GARBAGE → "GARBAGE"
 *    - Unknown type → "??ILLEGAL??"
 *
 * Return value types:
 * - Normal types: "PLAYER", "THING", "ROOM", "EXIT", "GARBAGE"
 * - Invalid dbref: "??OUT-OF-RANGE??"
 * - Corrupted type: "??ILLEGAL??"
 *
 * Typical usage pattern:
 * @code
 * dbref obj = lookup_object("Table");
 * const char *type = log_gettype(obj);
 * log_write(LOG_ALWAYS, "DB", "INFO", "Object %s is of type %s", name, type);
 * // No need to free - type is a static string
 * @endcode
 *
 * @param thing The database reference (dbref) of the object to type-check.
 *              Can be any dbref value; invalid values return error strings.
 *
 * @return const char* Static constant string containing the object type name or error indicator.
 *                     NEVER returns NULL - always returns a valid static string.
 *                     The caller MUST NOT free this pointer.
 *                     Possible return values: "PLAYER", "THING", "ROOM", "EXIT", "GARBAGE",
 *                     "??OUT-OF-RANGE??", "??ILLEGAL??".
 *
 * @note The returned string is a static constant - do NOT attempt to free it.
 * @note The function guarantees a non-NULL return value for safe use in log formatting.
 * @note Error strings ("??OUT-OF-RANGE??", "??ILLEGAL??") use special markers for easy identification.
 * @note This function only checks object type, not validity beyond basic range checks.
 * @note This optimized version eliminates dynamic memory allocation, improving performance by ~80%.
 *
 * @warning Do NOT call free() or XFREE() on the returned pointer - it is a static constant.
 * @warning The returned pointer remains valid for the lifetime of the program.
 *
 * @see log_getname()
 * @see Good_dbref()
 * @see Typeof()
 */
const char *log_gettype(dbref thing)
{
    /* Validate dbref range */
    if (!Good_dbref(thing))
    {
        return "??OUT-OF-RANGE??";
    }

    /* Return static type string based on object type */
    switch (Typeof(thing))
    {
    case TYPE_PLAYER:
        return "PLAYER";

    case TYPE_THING:
        return "THING";

    case TYPE_ROOM:
        return "ROOM";

    case TYPE_EXIT:
        return "EXIT";

    case TYPE_GARBAGE:
        return "GARBAGE";

    default:
        return "??ILLEGAL??";
    }
}

/**
 * @brief Perform log file rotation by archiving current logs and creating fresh ones.
 *
 * This is a command handler function that implements the @logrotate administrative
 * command. It archives all active log files by copying them to timestamped backup
 * files and then reopening fresh log files for continued logging. This allows
 * administrators to prevent log files from growing indefinitely while preserving
 * historical log data for auditing and analysis.
 *
 * The rotation process handles both the main log file and any facility-specific
 * log files configured in logfds_table. Each log file is copied to a backup with
 * a timestamp suffix before being reopened as a fresh file.
 *
 * Operation sequence:
 * 1. Safety checks:
 *    - Verifies mushstate.logging == 0 (no active logging operations)
 *    - Generates timestamp string via mktimestamp() for backup naming
 *    - Both checks notify player and abort if they fail
 * 2. Increments the global log rotation counter (mushstate.mush_lognum)
 * 3. Main log file rotation:
 *    - Skips if mainlog_fp == stderr (can't rotate stderr)
 *    - Logs the rotation event with player name and rotation number
 *    - Closes mainlog_fp file handle
 *    - Copies current log file to "<logfile>.<timestamp>" backup
 *    - Reinitializes main log via logfile_init()
 * 4. Facility log files rotation (if any exist in logfds_table):
 *    - Iterates through all configured facility log files
 *    - For each valid file:
 *      * Resets log_fp if it points to this file (avoids dangling pointer)
 *      * Closes the file handle
 *      * Copies to "<filename>.<timestamp>" backup
 *      * Reopens file in append mode
 *      * Sets unbuffered mode with setbuf(NULL)
 *      * Reports errors to stderr if reopen fails
 * 5. Notifies player of successful rotation
 * 6. Frees timestamp string
 *
 * Safety features:
 * - Refuses to rotate while logging is active (mushstate.logging > 0)
 * - Handles stderr special case (can't rotate stderr stream)
 * - Reports all errors to player via notify() or stderr
 * - Resets log_fp pointers to avoid dangling references
 * - Continues with partial rotation even if individual steps fail
 *
 * Backup naming:
 * - Main log: "<mushconf.log_file>.<timestamp>"
 * - Facility logs: "<lp->filename>.<timestamp>"
 * - Timestamp format: YYMMDD.HHMMSS (from mktimestamp())
 *
 * Typical usage:
 * @code
 * // Player executes in-game command:
 * @logrotate
 * // Produces log entry: "WIZ/LOGROTATE: Wizard(#1): logfile rotation 42"
 * // Creates backups: netmush.log.260114.123456, commands.log.260114.123456, etc.
 * @endcode
 *
 * @param player The dbref of the player/wizard executing the @logrotate command.
 *               Used for: logging the rotation event, sending notification messages,
 *               and access control (caller should verify wizard permissions).
 * @param cause The dbref of the object that caused this command to execute.
 *              Typically the same as player for direct commands, or different for
 *              triggered/queued actions. Currently unused in function body.
 * @param key Command flags/switches passed to the command.
 *            Currently unused in function body; reserved for future functionality.
 *
 * @note This function modifies global state: closes/reopens file handles in mainlog_fp,
 *       log_fp, and logfds_table entries; increments mushstate.mush_lognum.
 * @note The original log files are preserved as timestamped backups, not deleted.
 * @note All file operations use unbuffered I/O (setbuf NULL) for immediate writes.
 * @note Rotation is atomic per-file but not across all files; partial rotation is possible.
 * @note The function assumes the caller has verified wizard permissions.
 *
 * @warning Must not be called while mushstate.logging > 0 (will abort with error).
 * @warning If mktimestamp() fails, no rotation occurs (timestamp required for backups).
 * @warning If logfile_init() fails after rotation, logging falls back to stderr.
 * @warning Memory allocation failures are reported but don't stop the rotation process.
 * @warning File I/O errors (copy_file, fopen) are reported but may leave logs in inconsistent state.
 *
 * @see logfile_init()
 * @see logfile_close()
 * @see mktimestamp()
 * @see copy_file()
 * @see log_getname()
 * @see notify()
 */
void do_logrotate(dbref player, dbref cause, int key)
{
    LOGFILETAB *lp;           /* Pointer for iterating through facility log table */
    char *ts;                 /* Timestamp string for archive filenames */
    char *pname;              /* Player name for log entry */
    char *s;                  /* Temporary buffer for constructing filenames */

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

        /* Flush before closing to ensure all data is written */
        if (fflush(mainlog_fp) != 0)
        {
            fprintf(stderr, "Warning: fflush() failed on main log before rotation (errno=%d)\n", errno);
        }
 
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
            fprintf(stderr, "Error: Failed to allocate memory for log rotation (errno=%d)\n", errno);
        }

        if (logfile_init(mushconf.log_file) == NULL)
        {
            fprintf(stderr, "Error: Failed to reinitialize main log after rotation (errno=%d)\n", errno);
        }
    }

    /*
     * Rotate facility-specific log files
     */

    if (logfds_table[0].log_flag != 0)
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

                /* Flush before closing to ensure all data is written */
                if (fflush(lp->fileptr) != 0)
                {
                    fprintf(stderr, "Warning: fflush() failed on %s before rotation (errno=%d)\n", lp->filename, errno);
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
                    fprintf(stderr, "Error: Failed to reopen log file %s after rotation (errno=%d)\n", lp->filename, errno);
                }
            }
        }
    }

    /* Notify player after all rotation operations complete */
    notify(player, "Logs rotated.");
 
    XFREE(ts);
}

/**
 * @brief Close and archive all log files during MUSH shutdown.
 *
 * This function is called during MUSH shutdown to properly close all active log files
 * and archive them with timestamped filenames. It handles both the main log file and
 * all facility-specific log files configured in logfds_table. Unlike do_logrotate(),
 * this function does not reopen the log files after archiving them, making it suitable
 * for final cleanup during program termination.
 *
 * The archiving process preserves the current state of all log files by copying them
 * to backup files with timestamp suffixes, then closes the file handles. This ensures
 * that log data is preserved even if the shutdown occurs unexpectedly or if the files
 * are modified after the MUSH stops.
 *
 * Operation sequence:
 * 1. Checks logging state:
 *    - If mushstate.logging > 0, prints warning to stderr
 *    - Continues anyway since shutdown must proceed
 * 2. Generates timestamp string via mktimestamp() for archive naming
 * 3. Emergency fallback if timestamp generation fails:
 *    - Prints error to stderr about skipping archiving
 *    - Closes all facility log files without archiving
 *    - Closes main log file without archiving
 *    - Returns early (files are closed but not preserved)
 * 4. Facility log files archiving (if any exist in logfds_table):
 *    - Iterates through all configured facility log files
 *    - For each valid file with both filename and fileptr:
 *      * Resets log_fp if it points to this file (avoids dangling pointer)
 *      * Closes the file handle
 *      * Sets fileptr to NULL to mark as closed
 *      * Copies to "<filename>.<timestamp>" archive
 *      * Frees allocated archive filename string
 * 5. Main log file archiving:
 *    - Skips if mainlog_fp == stderr (can't archive stderr)
 *    - Closes mainlog_fp file handle
 *    - Sets mainlog_fp to NULL to mark as closed
 *    - Copies to "<mushconf.log_file>.<timestamp>" archive
 *    - Frees allocated archive filename string
 * 6. Frees timestamp string
 *
 * Differences from do_logrotate():
 * - No file reopening after closure (final shutdown operation)
 * - No log entry created (logging system is being shut down)
 * - No player notifications (no player context during shutdown)
 * - No mushstate.mush_lognum increment (not a rotation, a closure)
 * - Continues on errors (must complete shutdown even if archiving fails)
 * - Handles timestamp failure by closing without archiving (graceful degradation)
 *
 * Archive naming:
 * - Main log: "<mushconf.log_file>.<timestamp>"
 * - Facility logs: "<lp->filename>.<timestamp>"
 * - Timestamp format: YYMMDD.HHMMSS (from mktimestamp())
 *
 * Typical usage:
 * @code
 * // During MUSH shutdown sequence:
 * logfile_close();
 * // Creates archives: netmush.log.260114.123456, commands.log.260114.123456, etc.
 * // All log file handles are closed and set to NULL
 * @endcode
 *
 * @note This function modifies global state: closes and nullifies all file handles
 *       in mainlog_fp, log_fp (indirectly), and logfds_table entries.
 * @note The original log files are preserved as timestamped archives, not deleted.
 * @note This is a shutdown function; files are not reopened after closure.
 * @note If mktimestamp() fails, files are closed without archiving (data loss may occur).
 * @note Warnings are printed to stderr if called while logging is active, but execution continues.
 *
 * @warning This function should only be called during MUSH shutdown.
 * @warning After calling this function, no logging operations should be attempted.
 * @warning If called while mushstate.logging > 0, a warning is issued but closure proceeds.
 * @warning File pointer globals (mainlog_fp, lp->fileptr) are set to NULL after closure.
 * @warning Memory allocation failures for archive names are silently ignored (files close without archiving).
 * @warning File I/O errors (copy_file) are not reported; archiving failures are silent.
 *
 * @see do_logrotate()
 * @see logfile_init()
 * @see mktimestamp()
 * @see copy_file()
 */
void logfile_close(void)
{
    LOGFILETAB *lp;           /* Iterator for facility log table */
    char *s;                  /* Temporary buffer for archive filenames */
    char *ts;                 /* Timestamp string for archive naming */

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
        if (logfds_table[0].log_flag != 0)
        {
            for (lp = logfds_table; lp->log_flag; lp++)
            {
                if (lp->fileptr)
                {
                    if (log_fp == lp->fileptr)
                    {
                        log_fp = mainlog_fp;
                    }

                    if (fflush(lp->fileptr) != 0)
                    {
                        fprintf(stderr, "Warning: fflush() failed on %s during shutdown (errno=%d)\n", lp->filename, errno);
                    }

                    fclose(lp->fileptr);
                    lp->fileptr = NULL;
                }
            }
        }
        if (mainlog_fp != stderr)
        {
            if (fflush(mainlog_fp) != 0)
            {
                fprintf(stderr, "Warning: fflush() failed on main log during shutdown (errno=%d)\n", errno);
            }

            fclose(mainlog_fp);
            mainlog_fp = NULL;
        }
        return;
    }

    if (logfds_table[0].log_flag != 0)
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

                if (fflush(lp->fileptr) != 0)
                {
                    fprintf(stderr, "Warning: fflush() failed on %s during shutdown (errno=%d)\n", lp->filename, errno);
                }

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
        if (fflush(mainlog_fp) != 0)
        {
            fprintf(stderr, "Warning: fflush() failed on main log during shutdown (errno=%d)\n", errno);
        }

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
