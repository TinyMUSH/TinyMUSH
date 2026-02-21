/**
 * @file cque_queue.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue display and administrative commands
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
#include <limits.h>
#include <errno.h>
#include <ctype.h>

/**
 * @brief Display queue entries matching filter criteria with configurable detail level.
 *
 * Iterates through a queue (player, object, wait, or semaphore) and displays entries
 * matching player_targ/obj_targ filters. Supports three detail modes: summary (count only),
 * brief (one line per entry), and long (multi-line with arguments and enactor). Used by
 * cque_do_ps() to implement the @ps command for queue inspection and monitoring.
 *
 * Display modes (key parameter):
 * - PS_SUMM: Count matching entries without displaying individual commands
 * - PS_BRIEF: Display one line per entry with PID, player, and command text
 * - PS_LONG: Display multi-line entries including arguments (%0-%9) and enactor
 *
 * Entry filtering: Only displays entries where cque_que_want() returns true, filtering by:
 * - player_targ: Match entries owned by specific player (NOTHING = all players)
 * - obj_targ: Match entries from specific object (NOTHING = all objects)
 *
 * Output formats vary by queue entry type:
 * - Timed wait on semaphore: "[#sem/seconds] pid:player:command"
 * - Timed wait (no semaphore): "[seconds] pid:player:command"
 * - Semaphore wait (no timeout): "[#sem] pid:player:command" or "[#sem/attr] pid:player:command"
 * - Normal queue entry: "pid:player:command"
 *
 * Counter updates: Function updates three output counters via pointer parameters:
 * - *qtot: Total entries scanned in queue
 * - *qent: Entries matching filter criteria (displayed or counted)
 * - *qdel: Entries marked as halted (player == NOTHING)
 *
 * Header display: When first matching entry is found (qent == 1), displays header line
 * "----- {header} Queue -----" to identify queue type. Header is passed by caller
 * (typically "Player", "Object", "Wait", or "Semaphore").
 *
 * @param player      DBref of player viewing the queue (for permission checks and output)
 * @param key         Display mode: PS_SUMM (summary), PS_BRIEF (brief), or PS_LONG (detailed)
 * @param queue       Head of queue to scan (qfirst, qlfirst, qwait, or qsemfirst)
 * @param qtot        Output: Total number of entries scanned in queue
 * @param qent        Output: Number of entries matching filter criteria
 * @param qdel        Output: Number of halted entries (player == NOTHING) encountered
 * @param player_targ DBref of player to filter by (NOTHING = match all players)
 * @param obj_targ    DBref of object to filter by (NOTHING = match all objects)
 * @param header      Queue type name for display header ("Player", "Object", "Wait", "Semaphore")
 *
 * @note Thread-safe: Yes (read-only operation on queue structures)
 * @note Time remaining for wait/semaphore entries computed as (waittime - mushstate.now)
 * @note Halted entries (player == NOTHING) are counted but never displayed
 * @note Long mode displays enactor separately from player (shows command attribution)
 * @attention Assumes mushstate.now is current time for accurate time remaining calculation
 * @attention Memory allocated for unparse_object() strings must be freed after use
 *
 * @see cque_do_ps() for command interface that calls this function
 * @see cque_que_want() for entry filtering logic
 * @see unparse_object() for player/object name formatting
 */
void cque_show_que(dbref player, int key, BQUE *queue, int *qtot, int *qent, int *qdel, dbref player_targ, dbref obj_targ, const char *header)
{
	BQUE *tmp = NULL;
	char *bp = NULL, *bufp = NULL, *enactor_name = NULL;
	int msg_flags = MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN;
	ATTR *ap = NULL;

	*qtot = *qent = *qdel = 0;

	for (tmp = queue; tmp; tmp = tmp->next)
	{
		(*qtot)++;

		/* Track deleted entries */
		if (tmp->player == NOTHING)
		{
			(*qdel)++;
			continue;
		}

		/* Filter entries by target criteria */
		if (!cque_que_want(tmp, player_targ, obj_targ))
		{
			continue;
		}

		(*qent)++;

		/* Skip display for summary mode */
		if (key == PS_SUMM)
		{
			continue;
		}

		/* Display header on first matching entry */
		if (*qent == 1)
		{
			notify_check(player, player, msg_flags, "----- %s Queue -----", header);
		}

		bufp = unparse_object(player, tmp->player, 0);

		/* Display entry based on type */
		if ((tmp->waittime > 0) && Good_obj(tmp->sem))
		{
			/* Timed wait on semaphore */
			notify_check(player, player, msg_flags, "[#%d/%d] %d:%s:%s", tmp->sem, tmp->waittime - mushstate.now, tmp->pid, bufp, tmp->comm);
		}
		else if (tmp->waittime > 0)
		{
			/* Timed wait without semaphore */
			notify_check(player, player, msg_flags, "[%d] %d:%s:%s", tmp->waittime - mushstate.now, tmp->pid, bufp, tmp->comm);
		}
		else if (Good_obj(tmp->sem))
		{
			/* Semaphore wait without timeout */
			if (tmp->attr == A_SEMAPHORE)
			{
				notify_check(player, player, msg_flags, "[#%d] %d:%s:%s", tmp->sem, tmp->pid, bufp, tmp->comm);
			}
			else
			{
				ap = atr_num(tmp->attr);

				if (ap && ap->name)
				{
					notify_check(player, player, msg_flags, "[#%d/%s] %d:%s:%s", tmp->sem, ap->name, tmp->pid, bufp, tmp->comm);
				}
				else
				{
					notify_check(player, player, msg_flags, "[#%d] %d:%s:%s", tmp->sem, tmp->pid, bufp, tmp->comm);
				}
			}
		}
		else
		{
			/* Normal queue entry */
			notify_check(player, player, msg_flags, "%d:%s:%s", tmp->pid, bufp, tmp->comm);
		}

		/* Display detailed information in long mode */
		if (key == PS_LONG)
		{
			bp = bufp;

			for (int i = 0; i < tmp->nargs; i++)
			{
				if (tmp->env[i])
				{
					XSAFELBSTR((char *)"; Arg", bufp, &bp);
					XSAFELBCHR(i + '0', bufp, &bp);
					XSAFELBSTR((char *)"='", bufp, &bp);
					XSAFELBSTR(tmp->env[i], bufp, &bp);
					XSAFELBCHR('\'', bufp, &bp);
				}
			}

			*bp = '\0';
			enactor_name = unparse_object(player, tmp->cause, 0);
			notify_check(player, player, msg_flags, "   Enactor: %s%s", enactor_name, bufp);
			XFREE(enactor_name);
		}

		XFREE(bufp);
	}
}

/**
 * @brief Command interface for displaying queue status and entries (@ps command).
 *
 * Implements the @ps command that displays pending commands across all four queue types
 * (player, object, wait, semaphore) with filtering by player/object ownership. Supports
 * three detail levels (brief, summary, long) and optional "all queues" mode for wizards.
 * Delegates to cque_show_que() for each queue type, then displays aggregate statistics.
 *
 * Display modes (key parameter):
 * - PS_BRIEF (default): One line per entry with PID, player, and command
 * - PS_SUMM: Count only, no individual entries displayed
 * - PS_LONG: Multi-line format including arguments (%0-%9) and enactor
 * - PS_ALL flag: View all queues (requires See_Queue permission)
 *
 * Target resolution logic:
 * 1. No target specified:
 *    - PS_ALL: Display all players' queues (player_targ = NOTHING)
 *    - Default: Display issuer's queues (player_targ = Owner(player))
 *    - If issuer is not TYPE_PLAYER, also filter by issuer object (obj_targ = player)
 * 2. Target specified:
 *    - Must match controlled object (or any object if See_Queue permission)
 *    - If target is TYPE_PLAYER: Display that player's queues
 *    - Otherwise: Display that object's queues (player_targ = Owner(player))
 *
 * Permission requirements: PS_ALL flag requires See_Queue permission (typically wizard).
 * Target matching uses match_controlled() for normal users, match_thing() for wizards.
 * Invalid combinations (PS_ALL + target) are rejected with error message.
 *
 * Output format: Calls cque_show_que() four times (player, object, wait, semaphore queues),
 * then displays summary line with totals. Wizard view includes deleted entry counts
 * ("[Xdel]" suffix) for player and object queues. Normal users see entry/total counts.
 *
 * Error handling: Invalid target match returns silently after noisy_match_result().
 * Invalid switch combinations notify player with "Illegal combination of switches."
 * Permission denied on PS_ALL without See_Queue shows NOPERM_MESSAGE.
 *
 * @param player DBref of player issuing the @ps command
 * @param cause  DBref of cause object (unused, for command interface consistency)
 * @param key    Command flags: PS_BRIEF/PS_SUMM/PS_LONG for detail, PS_ALL for global view
 * @param target Target specification: player/object name to filter, or empty for self
 *
 * @note Thread-safe: Yes (read-only operation via cque_show_que() calls)
 * @note PS_ALL stripped from key before switch validation to allow combining with detail modes
 * @note Deleted entry counts (qdel) only collected/displayed for player and object queues
 * @note Wait and semaphore queues do not track deleted entries in summary statistics
 * @attention Requires See_Queue permission for PS_ALL flag or viewing other players' queues
 * @attention Target + PS_ALL combination is explicitly forbidden and returns error
 *
 * @see cque_show_que() for individual queue display implementation
 * @see cque_que_want() for entry filtering logic
 * @see match_controlled() for target resolution with permission checks
 */
void cque_do_ps(dbref player, dbref cause, int key, char *target)
{
	char *bufp = NULL;
	dbref player_targ = NOTHING, obj_targ = NOTHING;
	int pqent = 0, pqtot = 0, pqdel = 0, oqent = 0, oqtot = 0, oqdel = 0, wqent = 0, wqtot = 0, wqdel = 0, sqent = 0, sqtot = 0, sqdel = 0;

	/* Check permission for PS_ALL flag */
	if ((key & PS_ALL) && !See_Queue(player))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/* Determine target objects for queue filtering */
	if (!target || !*target)
	{
		/* No target specified: default to player's own queues */
		if (!(key & PS_ALL))
		{
			player_targ = Owner(player);

			if (Typeof(player) != TYPE_PLAYER)
			{
				obj_targ = player;
			}
		}
	}
	else
	{
		/* Target specified: resolve and validate */
		player_targ = Owner(player);
		obj_targ = See_Queue(player) ? match_thing(player, target) : match_controlled(player, target);

		if (!Good_obj(obj_targ))
		{
			return;
		}

		if (key & PS_ALL)
		{
			notify(player, "Can't specify a target and /all");
			return;
		}

		if (Typeof(obj_targ) == TYPE_PLAYER)
		{
			player_targ = obj_targ;
			obj_targ = NOTHING;
		}
	}

	/* Validate display mode */
	switch (key & ~PS_ALL)
	{
	case PS_BRIEF:
	case PS_SUMM:
	case PS_LONG:
		break;

	default:
		notify(player, "Illegal combination of switches.");
		return;
	}

	/* Display all four queues */
	cque_show_que(player, key & ~PS_ALL, mushstate.qfirst, &pqtot, &pqent, &pqdel, player_targ, obj_targ, "Player");
	cque_show_que(player, key & ~PS_ALL, mushstate.qlfirst, &oqtot, &oqent, &oqdel, player_targ, obj_targ, "Object");
	cque_show_que(player, key & ~PS_ALL, mushstate.qwait, &wqtot, &wqent, &wqdel, player_targ, obj_targ, "Wait");
	cque_show_que(player, key & ~PS_ALL, mushstate.qsemfirst, &sqtot, &sqent, &sqdel, player_targ, obj_targ, "Semaphore");

	/* Display summary statistics */
	bufp = XMALLOC(MBUF_SIZE, "bufp");

	if (See_Queue(player))
	{
		XSPRINTF(bufp, "Totals: Player...%d/%d[%ddel]  Object...%d/%d[%ddel]  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent, sqtot);
	}
	else
	{
		XSPRINTF(bufp, "Totals: Player...%d/%d  Object...%d/%d  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, oqent, oqtot, wqent, wqtot, sqent, sqtot);
	}

	notify(player, bufp);
	XFREE(bufp);
}

/**
 * @brief Parse and validate an integer argument for queue operations.
 *
 * Parses a string argument into an integer within INT_MIN to INT_MAX range.
 * Validates for ERANGE errors (overflow/underflow) and out-of-range values.
 *
 * @param arg    String argument to parse
 * @param val    Output: parsed integer value (valid only if function returns true)
 * @return true if parsing succeeded, false on any error (overflow, underflow, non-numeric)
 *
 * @note Thread-safe: Yes (no global state modification)
 */
static bool _cque_parse_queue_arg(const char *arg, int *val)
{
	char *endptr = NULL;
	long lval = 0;

	if (!arg || !*arg || !val)
	{
		return false;
	}

	errno = 0;
	lval = strtol(arg, &endptr, 10);

	if (errno == ERANGE || lval > INT_MAX || lval < INT_MIN || endptr == arg || *endptr != '\0')
	{
		return false;
	}

	*val = (int)lval;
	return true;
}

/**
 * @brief Administrative command interface for manual queue manipulation (@queue command).
 *
 * Implements the @queue command with two operational modes: QUEUE_KICK for forced command
 * execution, and QUEUE_WARP for time manipulation of wait/semaphore queues. Provides
 * wizard-level control over queue processing for debugging, performance testing, and
 * emergency queue management. Temporarily enables CF_DEQUEUE flag if disabled to ensure
 * operations succeed even when automatic processing is paused.
 *
 * Operational modes:
 * 1. QUEUE_KICK: Manually execute specified number of commands from player queue
 *    - Calls cque_do_top(i) to process i commands from mushstate.qfirst
 *    - Returns count of commands actually executed (may be less than requested)
 *    - Useful for draining queue during high load or testing command processing
 *    - Temporarily enables CF_DEQUEUE if disabled (warns player)
 *
 * 2. QUEUE_WARP: Adjust wait times by time offset (positive = advance, negative = rewind)
 *    - Modifies all wait queue entries: sets waittime = -i (forces immediate execution)
 *    - Modifies semaphore timeouts: decrements waittime by i (clamps negative to -1)
 *    - Calls cque_do_second() to process newly-expired entries
 *    - Special case: i = 0 promotes object queue to player queue without time change
 *    - Used for testing time-based features or recovering from clock issues
 *
 * CF_DEQUEUE flag handling: Both modes check if automatic dequeueing is disabled
 * (CF_DEQUEUE flag clear). If so, temporarily enables flag, displays warning to player,
 * executes operation, then restores original disabled state. This ensures manual
 * operations work regardless of automatic processing state.
 *
 * Input validation: Argument must be valid integer within INT_MIN to INT_MAX range.
 * Invalid values (non-numeric, overflow, empty) result in error notification and
 * early return without modifying queue state.
 *
 * Output messages: Unless player is Quiet, displays operation result:
 * - QUEUE_KICK: "X commands processed." where X is actual execution count
 * - QUEUE_WARP positive: "WaitQ timer advanced X seconds."
 * - QUEUE_WARP negative: "WaitQ timer set back X seconds."
 * - QUEUE_WARP zero: "Object queue appended to player queue."
 *
 * @param player DBref of player issuing the @queue command (typically wizard)
 * @param cause  DBref of cause object (unused, for command interface consistency)
 * @param key    Operation mode: QUEUE_KICK for forced execution, QUEUE_WARP for time adjustment
 * @param arg    String argument: command count for QUEUE_KICK, time offset for QUEUE_WARP
 *
 * @note Thread-safe: No (modifies global queue structures and CF_DEQUEUE flag)
 * @note QUEUE_WARP with negative offset does not restore original wait times precisely
 * @note Semaphore entries with waittime = 0 (non-timed waits) are unaffected by QUEUE_WARP
 * @note CF_DEQUEUE state is preserved across operation if temporarily enabled
 * @attention QUEUE_WARP can cause commands to execute out of intended order
 * @attention QUEUE_KICK may process fewer commands than requested if queue depletes
 * @attention Temporary CF_DEQUEUE enable triggers warning notification
 *
 * @see cque_do_top() for command execution implementation used by QUEUE_KICK
 * @see cque_do_second() for wait queue processing triggered by QUEUE_WARP
 * @see mushconf.control_flags for CF_DEQUEUE flag controlling automatic processing
 */
void cque_do_queue(dbref player, dbref cause, int key, char *arg)
{
	BQUE *point = NULL;
	int i = 0, ncmds = 0;
	int was_disabled = !(mushconf.control_flags & CF_DEQUEUE);

	/* Parse and validate the integer argument */
	if (!_cque_parse_queue_arg(arg, &i))
	{
		notify(player, (key == QUEUE_KICK) ? "Invalid number of commands." : "Invalid time value.");
		return;
	}

	/* Temporarily enable CF_DEQUEUE if needed */
	if (was_disabled)
	{
		mushconf.control_flags |= CF_DEQUEUE;
		notify(player, "Warning: automatic dequeueing is disabled.");
	}

	if (key == QUEUE_KICK)
	{
		ncmds = cque_do_top(i);

		if (!Quiet(player))
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d commands processed.", ncmds);
		}
	}
	else if (key == QUEUE_WARP)
	{
		/* Adjust wait queue: set all entries to negative of time offset */
		for (point = mushstate.qwait; point; point = point->next)
		{
			point->waittime = -i;
		}

		/* Adjust semaphore queue: decrement timeouts, clamp to -1 if negative */
		for (point = mushstate.qsemfirst; point; point = point->next)
		{
			if (point->waittime > 0)
			{
				point->waittime -= i;

				if (point->waittime <= 0)
				{
					point->waittime = -1;
				}
			}
		}

		cque_do_second();

		if (!Quiet(player))
		{
			if (i > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "WaitQ timer advanced %d seconds.", i);
			}
			else if (i < 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "WaitQ timer set back %d seconds.", i);
			}
			else
			{
				notify(player, "Object queue appended to player queue.");
			}
		}
	}

	/* Restore original CF_DEQUEUE state */
	if (was_disabled)
	{
		mushconf.control_flags &= ~CF_DEQUEUE;
	}
}
