/**
 * @file cque_wait.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Wait queue management and timed command execution
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

#include <string.h>
/* Local prototype for helper defined in cque_halt.c */
bool _cque_parse_pid_string(const char *pidstr, int *qpid);
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

/**
 * @brief Queue a command for delayed or semaphore-controlled execution.
 *
 * Creates a queue entry via setup_que() and routes it to the appropriate queue based on
 * wait time and semaphore parameters. Supports three execution modes: immediate (wait <= 0,
 * no semaphore), time-delayed (wait > 0, no semaphore), and semaphore-blocked (semaphore
 * specified). The wait queue is maintained in sorted order by execution time for efficient
 * processing by do_second().
 *
 * Queue routing logic:
 * - No semaphore + wait <= 0: Immediate execution via give_que()
 * - No semaphore + wait > 0: Time-sorted insertion into wait queue (mushstate.qwait)
 * - Semaphore specified: Append to semaphore queue (mushstate.qsemfirst/qsemlast)
 *
 * Wait time handling includes overflow protection: values exceeding INT_MAX/INT_MIN are
 * clamped to prevent timestamp wraparound. The wait queue is sorted by absolute execution
 * time (now + wait), enabling efficient O(1) pop operations by do_second().
 *
 * If CF_INTERP flag is disabled or setup_que() fails (halted player, insufficient funds,
 * quota exceeded, PID exhaustion), the command is silently discarded.
 *
 * @param player  DBref of the player queuing the command
 * @param cause   DBref of the object that caused this command (for attribution)
 * @param wait    Delay in seconds before execution (0 = immediate, negative treated as 0)
 * @param sem     DBref of semaphore object to wait on (NOTHING = no semaphore)
 * @param attr    Attribute number for semaphore counter (0 = A_SEMAPHORE, ignored if sem = NOTHING)
 * @param command Command text string to execute (may be NULL)
 * @param args    Array of environment variable strings (%0-%9) - may contain NULLs
 * @param nargs   Number of arguments in args array
 * @param gargs   Global and extended register data (q-registers, x-registers) or NULL
 *
 * @note Thread-safe: No (modifies global queue structures and calls non-thread-safe functions)
 * @note If setup_que() fails, function returns silently without notification
 * @note Semaphore queue is unsorted (FIFO); wait queue is sorted by execution time
 * @attention Wait time overflow is silently clamped to INT_MAX/INT_MIN
 * @attention Requires CF_INTERP flag to be set or command is ignored
 *
 * @see setup_que() for queue entry creation and validation
 * @see give_que() for immediate execution queue
 * @see do_second() for wait queue processing
 * @see nfy_que() for semaphore release
 */
void wait_que(dbref player, dbref cause, int wait, dbref sem, int attr, char *command, char *args[], int nargs, GDATA *gargs)
{
	BQUE *tmp = NULL, *point = NULL, **pptr = NULL;

	if (!(mushconf.control_flags & CF_INTERP))
	{
		return;
	}

	tmp = setup_que(player, cause, command, args, nargs, gargs);

	if (tmp == NULL)
	{
		return;
	}

	/* Set wait time, and check for integer overflow before the addition */
	if (wait > 0)
	{
		time_t now = time(NULL);
		/* Check for overflow before performing the addition */
		if ((time_t)wait > INT_MAX - now)
		{
			tmp->waittime = INT_MAX;
		}
		else
		{
			tmp->waittime = now + wait;
		}
	}
	else if (wait < 0)
	{
		time_t now = time(NULL);
		if ((time_t)wait < INT_MIN - now)
		{
			tmp->waittime = INT_MIN;
		}
		else
		{
			tmp->waittime = now + wait;
		}
	}

	tmp->sem = sem;
	tmp->attr = attr;

	if (sem == NOTHING)
	{
		/* No semaphore, put on wait queue if wait value specified. Otherwise put on the normal queue. */
		if (wait <= 0)
		{
			give_que(tmp);
		}
		else
		{
			/* Thread into sorted wait queue using pointer-to-pointer technique */
			pptr = &mushstate.qwait;
			while (*pptr && (*pptr)->waittime <= tmp->waittime)
			{
				pptr = &((*pptr)->next);
			}

			tmp->next = *pptr;
			*pptr = tmp;
		}
	}
	else
	{
		/* Append to semaphore queue (unsorted, FIFO) */
		tmp->next = NULL;

		if (mushstate.qsemlast != NULL)
		{
			mushstate.qsemlast->next = tmp;
		}
		else
		{
			mushstate.qsemfirst = tmp;
		}

		mushstate.qsemlast = tmp;
	}
}

/**
 * @brief Adjust the wait time of a specific queue entry identified by PID.
 *
 * Validates and parses both PID and time strings, locates the queue entry, and modifies
 * its execution time after permission checks. Supports two time specification modes:
 * absolute (WAIT_UNTIL) and relative (default). For wait queue entries, automatically
 * re-threads the entry to maintain sorted order by execution time. Semaphore queue
 * entries remain in place as that queue is unsorted.
 *
 * Time specification modes:
 * - WAIT_UNTIL: Absolute Unix timestamp (negative values = execute now)
 * - Relative (default): Offset from current time or entry's existing time
 *   * Prefixed with +/-: Adjust existing waittime by offset
 *   * No prefix: Set to (current_time + offset)
 *
 * Validation sequence:
 * 1. Verify both timestr and pidstr are valid integers
 * 2. Confirm PID in valid range [1, max_qpid]
 * 3. Locate entry in PID hash table
 * 4. Check entry not halted (player != NOTHING)
 * 5. Verify Controls permission on entry owner
 * 6. For semaphore entries, ensure waittime != 0 (must be timed-wait)
 *
 * Overflow protection: Time calculations check for INT_MAX/INT_MIN overflow and clamp
 * results. Negative waittimes (except from WAIT_UNTIL) are corrected to either current
 * time (if decremented) or INT_MAX (if incremented).
 *
 * @param player  DBref of the player adjusting the wait time
 * @param key     Command flags: WAIT_UNTIL for absolute time mode
 * @param pidstr  String representation of the PID to modify
 * @param timestr String representation of new time (seconds or Unix timestamp)
 *
 * @note Thread-safe: No (modifies global queue structures)
 * @note Notifies player of all validation errors with specific error messages
 * @note Wait queue entries are automatically re-sorted after time adjustment
 * @attention Cannot adjust semaphore entries with waittime = 0 (non-timed waits)
 * @attention Time overflow is silently clamped to INT_MAX/INT_MIN
 *
 * @see wait_que() for initial queue entry creation
 * @see remove_waitq() for queue removal before re-threading
 * @see do_wait() for the wait command that creates timed waits
 */
void do_wait_pid(dbref player, int key, char *pidstr, char *timestr)
{
	int qpid = 0;
	BQUE *qptr = NULL, **pptr = NULL;
	char *endptr = NULL;
	long val = 0;

	/* Validate and parse PID */
	if (!_cque_parse_pid_string(pidstr, &qpid))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	if ((qpid < 1) || (qpid > mushconf.max_qpid))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	/* Validate and parse time value */
	errno = 0;
	val = strtol(timestr, &endptr, 10);

	if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == timestr || *endptr != '\0')
	{
		notify(player, "That is not a valid wait time.");
		return;
	}

	/* Locate queue entry by PID */
	qptr = (BQUE *)nhashfind(qpid, &mushstate.qpid_htab);

	if (!qptr)
	{
		notify(player, "That PID is not associated with an active queue entry.");
		return;
	}

	if (qptr->player == NOTHING)
	{
		notify(player, "That queue entry has been halted.");
		return;
	}

	if (!Controls(player, qptr->player))
	{
		notify(player, "Permission denied.");
		return;
	}

	if ((qptr->sem != NOTHING) && (qptr->waittime == 0))
	{
		notify(player, "That semaphore does not have a wait time.");
		return;
	}

	/* Calculate new wait time based on mode */
	if (key & WAIT_UNTIL)
	{
		qptr->waittime = (val < 0) ? time(NULL) : (int)val;
	}
	else
	{
		time_t base_time = (timestr[0] == '+' || timestr[0] == '-') ? qptr->waittime : time(NULL);

		/* Check for overflow before addition */
		if ((val > 0 && base_time > INT_MAX - val) || (val < 0 && base_time < INT_MIN - val))
		{
			qptr->waittime = (val > 0) ? INT_MAX : INT_MIN;
		}
		else
		{
			qptr->waittime = base_time + (int)val;
		}

		/* Correct negative waittimes */
		if (qptr->waittime < 0)
		{
			qptr->waittime = (timestr[0] == '-') ? time(NULL) : INT_MAX;
		}
	}

	/* Re-thread wait queue entry if necessary (queue is sorted by waittime) */
	if (qptr->sem == NOTHING)
	{
		remove_waitq(qptr);

		/* Use pointer-to-pointer technique for clean insertion */
		pptr = &mushstate.qwait;
		while (*pptr && (*pptr)->waittime <= qptr->waittime)
		{
			pptr = &((*pptr)->next);
		}

		qptr->next = *pptr;
		*pptr = qptr;
	}

	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Adjusted wait time for queue entry PID %d.", qpid);
}

/**
 * @brief Command interface for queuing commands with time delays or semaphore blocking.
 *
 * Parses event specification to determine execution mode and delegates to wait_que() for
 * actual queuing. Supports two primary modes: simple timed delay (numeric event) and
 * semaphore-based blocking (object[/attribute] event). The WAIT_PID flag provides access
 * to do_wait_pid() for adjusting existing queue entries instead of creating new ones.
 *
 * Execution modes:
 * 1. PID adjustment (WAIT_PID flag): Delegates to do_wait_pid() to modify existing entry
 * 2. Timed delay (numeric event): Queues command with specified delay in seconds
 *    - WAIT_UNTIL flag: Treats value as absolute Unix timestamp
 *    - Default: Treats value as relative delay from current time
 * 3. Semaphore wait (object event): Increments semaphore counter and blocks until notified
 *    - Format: "object" uses A_SEMAPHORE attribute
 *    - Format: "object/attribute" uses custom attribute for semaphore counter
 *    - Optional timeout after "/" separator (numeric, seconds)
 *
 * Event parsing logic:
 * - If numeric: Timed delay mode, parse as seconds (or timestamp if WAIT_UNTIL)
 * - If non-numeric: Semaphore mode, parse as "object" or "object/attribute[/timeout]"
 * - Split on '/' to extract object name, optional attribute name, optional timeout
 *
 * Semaphore behavior: Increments attribute counter via add_to(). If counter becomes <= 0
 * (over-notification), command executes immediately without blocking. Otherwise, command
 * is queued on semaphore until nfy_que() or do_notify() releases it.
 *
 * Permission requirements: For semaphore mode, player must either control the target
 * object or object must have Link_ok flag. For custom attributes, player must have
 * Set_attr permission on that attribute.
 *
 * @param player DBref of the player issuing the wait command
 * @param cause  DBref of the cause object (for attribution in queued command)
 * @param key    Command flags: WAIT_PID for PID adjustment, WAIT_UNTIL for absolute time
 * @param event  Event specification: PID (if WAIT_PID), seconds, or object[/attr[/timeout]]
 * @param cmd    Command text string to execute after delay/notification
 * @param cargs  Array of environment variable strings (%0-%9) for command execution
 * @param ncargs Number of arguments in cargs array
 *
 * @note Thread-safe: No (calls wait_que which modifies global queue state)
 * @note Invalid numeric values result in error notification and command is not queued
 * @note Over-notified semaphores (counter <= 0) execute command immediately
 * @attention Semaphore counter increment occurs before queuing - potential race condition
 * @attention Timeout values exceeding INT_MAX are clamped to INT_MAX
 *
 * @see wait_que() for the underlying queue insertion implementation
 * @see do_wait_pid() for PID-based wait time adjustment
 * @see do_notify() for semaphore notification/release
 * @see nfy_que() for semaphore queue processing
 */
void do_wait(dbref player, dbref cause, int key, char *event, char *cmd, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int howlong = 0, num = 0, attr = A_SEMAPHORE, aflags = 0;
	char *what = NULL, *endptr = NULL;
	long val = 0;
	ATTR *ap = NULL;

	/* PID adjustment mode */
	if (key & WAIT_PID)
	{
		do_wait_pid(player, key, event, cmd);
		return;
	}

	/* Numeric event: simple timed wait (no semaphore) */
	if (is_number(event))
	{
		errno = 0;
		val = strtol(event, &endptr, 10);

		if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == event || *endptr != '\0')
		{
			notify(player, "Invalid wait time.");
			return;
		}

		/* Calculate wait duration based on mode */
		if (key & WAIT_UNTIL)
		{
			time_t now = time(NULL);
			howlong = (val < (long)now) ? 0 : ((val - now > INT_MAX) ? INT_MAX : (int)(val - now));
		}
		else
		{
			howlong = (int)val;
		}

		wait_que(player, cause, howlong, NOTHING, 0, cmd, cargs, ncargs, mushstate.rdata);
		return;
	}

	/* Semaphore wait with optional timeout and custom attribute */
	what = parse_to(&event, '/', 0);
	init_match(player, what, NOTYPE);
	match_everything(0);
	thing = noisy_match_result();

	if (!Good_obj(thing))
	{
		notify(player, "No match.");
		return;
	}

	if (!controls(player, thing) && !Link_ok(thing))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/* Parse optional timeout (numeric) or attribute name (non-numeric) */
	if (event && *event)
	{
		if (is_number(event))
		{
			/* Numeric: parse as timeout value */
			errno = 0;
			val = strtol(event, &endptr, 10);

			if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == event || *endptr != '\0')
			{
				notify(player, "Invalid wait time.");
				return;
			}

			/* Calculate wait duration based on mode */
			howlong = (key & WAIT_UNTIL) ? ((val < (long)time(NULL)) ? 0 : ((val - time(NULL) > INT_MAX) ? INT_MAX : (int)(val - time(NULL)))) : (int)val;
		}
		else
		{
			/* Non-numeric: parse as custom attribute name */
			ap = atr_str(event);

			if (!ap)
			{
				attr = mkattr(event);

				if (attr <= 0)
				{
					notify_quiet(player, "Invalid attribute.");
					return;
				}

				ap = atr_num(attr);
			}

			atr_pget_info(thing, ap->number, &aowner, &aflags);

			if (!Set_attr(player, thing, ap, aflags))
			{
				notify_quiet(player, NOPERM_MESSAGE);
				return;
			}

			attr = ap->number;
			howlong = 0;
		}
	}
	else
	{
		howlong = 0;
	}

	/* Increment semaphore counter */
	num = add_to(player, thing, 1, attr);

	if (num <= 0)
	{
		/* Over-notified semaphore: execute immediately without blocking */
		thing = NOTHING;
		howlong = 0;
	}

	wait_que(player, cause, howlong, thing, attr, cmd, cargs, ncargs, mushstate.rdata);
}
