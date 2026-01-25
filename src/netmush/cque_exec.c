/**
 * @file cque_exec.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command execution and scheduling operations
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
#include "cque_internal.h"

#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

/* ==================== Global Variables ==================== */

/** @brief Next queue PID to allocate (internal optimization hint for qpid_next) */
int qpid_top = 1;

/**
 * @brief Allocate the next available queue process ID (PID) from the PID pool.
 *
 * Searches for an unused PID in the range [1, max_qpid] using an optimized allocation
 * strategy. Maintains qpid_top as a hint to the next likely available PID, avoiding
 * repeated scans from 1. If the search space is exhausted (all PIDs in use), returns 0
 * to indicate queue exhaustion.
 *
 * Allocation algorithm:
 * 1. Start search from qpid_top (last allocated PID + 1)
 * 2. Wrap around to 1 if search exceeds max_qpid
 * 3. Check PID availability via hash table lookup
 * 4. If available, update qpid_top hint and return PID
 * 5. If unavailable, increment and continue
 * 6. After max_qpid attempts, queue is full - return 0
 *
 * The qpid_top optimization reduces allocation time from O(n²) to approximately O(1)
 * for typical usage patterns where PIDs are allocated and freed sequentially.
 *
 * @return Next available PID in range [1, max_qpid], or 0 if queue is full
 *
 * @note Thread-safe: No (modifies static qpid_top and checks global hash table)
 * @note Return value 0 indicates all PIDs exhausted - caller must handle gracefully
 * @attention PIDs wrap around from max_qpid to 1, ensuring bounded search space
 *
 * @see setup_que() for PID allocation usage
 * @see delete_qentry() for PID deallocation (via nhashdelete)
 */
int qpid_next(void)
{
	int qpid = qpid_top;

	for (int i = 0; i < mushconf.max_qpid; i++)
	{
		if (!nhashfind(qpid, &mushstate.qpid_htab))
		{
			qpid_top = (qpid < mushconf.max_qpid) ? (qpid + 1) : 1;
			return qpid;
		}

		qpid = (qpid < mushconf.max_qpid) ? (qpid + 1) : 1;
	}

	return 0;
}

/**
 * @brief Calculate seconds until next queue command is ready for execution.
 *
 * Implements a priority-based scheduling algorithm to determine optimal sleep time before
 * the next queue processing cycle. Scans all four queue types (player, object, wait,
 * semaphore) and returns the minimum time until any command becomes ready, implementing
 * a three-tier priority system for responsive gameplay.
 *
 * Priority tiers and return values:
 * 1. Player queue (highest): Returns 0 for immediate execution
 * 2. Object queue: Returns 1 for one-second delay
 * 3. Wait/semaphore queues: Returns minimum time until next command (min - 1)
 *
 * Scanning algorithm:
 * - Checks test_top() for player queue readiness (immediate return 0)
 * - Checks mushstate.qlfirst for object queue presence (return 1)
 * - Scans wait queue: Computes (waittime - now) for each entry
 * - Scans semaphore queue: Only considers timed-waits (waittime != 0)
 * - Returns (minimum_time - 1) with floor of 1 second for imminent commands
 *
 * The algorithm prioritizes player commands over object commands to maintain responsive
 * user experience. Commands within 2 seconds of execution time are treated as "imminent"
 * and scheduled for immediate processing (return 1). The default maximum of 1000 seconds
 * serves as a safety ceiling for empty queues.
 *
 * @return Seconds to sleep before next queue processing cycle:
 *         - 0: Player queue has ready commands (immediate processing)
 *         - 1: Object queue or imminent wait/semaphore commands (short delay)
 *         - 2+: Time until next scheduled command minus 1 second
 *         - 999: No commands in any queue (default 1000 - 1)
 *
 * @note Thread-safe: Yes (read-only operation on queue structures)
 * @note Return value of 0 triggers immediate do_top() execution
 * @note Imminent threshold (2 seconds) provides buffer for processing latency
 * @attention Depends on mushstate.now being current time (updated by main loop)
 * @attention Semaphore entries with waittime = 0 are skipped (no timeout)
 *
 * @see do_second() for wait queue processing when time expires
 * @see do_top() for command execution triggered by return value 0
 * @see test_top() for player queue readiness check
 */
int que_next(void)
{
	int min = 1000, this = 0;
	BQUE *point = NULL;

	/* Player queue has highest priority - execute immediately */
	if (test_top())
	{
		return 0;
	}

	/* Object queue - execute after one-second delay */
	if (mushstate.qlfirst != NULL)
	{
		return 1;
	}

	/* Scan wait queue for minimum time until next command */
	for (point = mushstate.qwait; point; point = point->next)
	{
		this = point->waittime - mushstate.now;

		if (this <= 2)
		{
			return 1;
		}

		if (this < min)
		{
			min = this;
		}
	}

	/* Scan semaphore queue for timed-waits only */
	for (point = mushstate.qsemfirst; point; point = point->next)
	{
		/* Skip non-timed semaphore waits */
		if (point->waittime == 0)
		{
			continue;
		}

		this = point->waittime - mushstate.now;

		if (this <= 2)
		{
			return 1;
		}

		if (this < min)
		{
			min = this;
		}
	}

	return min - 1;
}

/**
 * @brief Process expired wait queue and semaphore queue entries for execution.
 *
 * Called once per second by the main event loop to check wait and semaphore queues for
 * commands ready to execute. Performs three queue management operations in order:
 * low-priority queue promotion, wait queue expiration processing, and semaphore timeout
 * handling. This function implements the core time-based command scheduling mechanism
 * that enables @wait, timed semaphores, and object action throttling.
 *
 * Processing sequence:
 * 1. Low-priority queue promotion: Moves entire object queue (qlfirst/qllast) to end of
 *    normal queue (qfirst/qlast), implementing one-second delay for object actions
 * 2. Wait queue processing: Scans mushstate.qwait in sorted order, moving all entries
 *    with waittime <= now to normal queue via give_que() for immediate execution
 * 3. Semaphore timeout processing: Scans mushstate.qsemfirst for timed-wait entries
 *    (waittime != 0), decrements semaphore counter via add_to(), and moves expired
 *    entries to normal queue
 *
 * Queue promotion logic maintains responsive gameplay by prioritizing player commands
 * (immediate execution) over object commands (one-second delay). The wait queue is
 * sorted by execution time, enabling efficient O(1) early-exit when first entry is
 * not ready. Semaphore queue is unsorted and requires full scan.
 *
 * Timed semaphore behavior: When timeout expires, decrements semaphore counter and
 * executes command regardless of semaphore state. This prevents deadlock when notification
 * never arrives. Non-timed semaphores (waittime = 0) remain queued until explicit
 * notification via nfy_que()/do_notify().
 *
 * Early exit: If CF_DEQUEUE flag is disabled, function returns immediately without
 * processing any queues, effectively pausing command execution system.
 *
 * @note Thread-safe: No (modifies global queue structures mushstate.qfirst/qlast/qwait/qsemfirst)
 * @note Called by main event loop approximately once per second (see que_next() return values)
 * @note Low-priority queue promotion occurs before wait processing for consistent timing
 * @note Wait queue early-exit optimization assumes sorted order (maintained by wait_que())
 * @attention Requires mushstate.now to be current Unix timestamp (updated by caller)
 * @attention CF_DEQUEUE flag must be set or no command processing occurs
 * @attention Semaphore counter decremented even if command fails to execute
 *
 * @see que_next() for sleep time calculation that triggers this function
 * @see wait_que() for wait queue insertion and sorting
 * @see give_que() for normal queue insertion (immediate execution)
 * @see nfy_que() for semaphore notification (non-timeout release)
 * @see do_top() for command execution from normal queue
 */
void do_second(void)
{
	BQUE *trail = NULL, *point = NULL, *next = NULL;
	char *cmdsave = NULL;

	/* Early exit if command processing is disabled */
	if (!(mushconf.control_flags & CF_DEQUEUE))
	{
		return;
	}

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< do_second >";

	/* Promote low-priority (object) queue to end of normal queue */
	if (mushstate.qlfirst)
	{
		if (mushstate.qlast)
		{
			mushstate.qlast->next = mushstate.qlfirst;
		}
		else
		{
			mushstate.qfirst = mushstate.qlfirst;
		}

		mushstate.qlast = mushstate.qllast;
		mushstate.qllast = mushstate.qlfirst = NULL;
	}

	/* Process wait queue: move expired entries to normal queue */
	while (mushstate.qwait && mushstate.qwait->waittime <= mushstate.now)
	{
		point = mushstate.qwait;
		mushstate.qwait = point->next;
		give_que(point);
	}

	/* Process semaphore queue: handle expired timed-waits */
	for (point = mushstate.qsemfirst, trail = NULL; point; point = next)
	{
		/* Skip non-timed semaphore waits */
		if (point->waittime == 0)
		{
			next = (trail = point)->next;
			continue;
		}

		/* Check if timed-wait has expired */
		if (point->waittime <= mushstate.now)
		{
			/* Unlink from semaphore queue */
			next = point->next;

			if (trail)
			{
				trail->next = next;
			}
			else
			{
				mushstate.qsemfirst = next;
			}

			if (point == mushstate.qsemlast)
			{
				mushstate.qsemlast = trail;
			}

			/* Decrement semaphore and execute command */
			add_to(point->player, point->sem, -1, point->attr ? point->attr : A_SEMAPHORE);
			point->sem = NOTHING;
			give_que(point);
		}
		else
		{
			next = (trail = point)->next;
		}
	}

	mushstate.debug_cmd = cmdsave;
}

/**
 * @brief Execute up to ncmds commands from the player queue (normal priority).
 *
 * Main command execution engine that dequeues and runs commands from mushstate.qfirst
 * (player/normal priority queue). Processes commands in FIFO order, handling resource
 * refunds, register context setup, command parsing, and queue entry cleanup. Executes
 * a maximum of ncmds commands per invocation to prevent CPU starvation, returning the
 * actual count executed for scheduling feedback.
 *
 * Execution sequence per command:
 * 1. Check test_top() for available commands (early exit if queue empty)
 * 2. Extract player from queue head (mushstate.qfirst)
 * 3. Refund waitcost to player (paid during setup_que())
 * 4. Set execution context (curr_player, curr_enactor)
 * 5. Decrement player's queue counter via a_Queue()
 * 6. Mark entry as processed (player = NOTHING prevents double-execution)
 * 7. Load scratch registers (q-registers, x-registers) from queue entry gdata
 * 8. Parse and execute command via process_cmdent()
 * 9. Clean up queue entry via delete_qentry()
 * 10. Advance queue head to next entry
 *
 * Register context management: Global registers (mushstate.rdata) are preserved across
 * the execution batch to maintain continuity. Queue entry registers (gdata) are loaded
 * into mushstate.global_regs before command execution. Both are cleaned up when queue
 * becomes empty or ncmds limit is reached.
 *
 * Player validation: Commands for non-existent (player < 0) or Going players are
 * silently discarded with queue entry cleanup. Halted players have entries removed
 * but commands are not executed (wasteful waitcost refund still occurs).
 *
 * Early termination: If CF_DEQUEUE flag is disabled, returns 0 immediately without
 * processing any commands, effectively pausing the command execution system. Queue
 * state remains unchanged for later processing.
 *
 * @param ncmds Maximum number of commands to execute in this invocation (throttling limit)
 * @return Number of commands actually executed (0 to ncmds inclusive)
 *
 * @note Thread-safe: No (modifies global queue structures and player state)
 * @note Commands from Going players are discarded without execution
 * @note Halted players receive waitcost refund but command is not executed
 * @note Register cleanup (mushstate.rdata) occurs when queue empties or batch completes
 * @attention Requires CF_DEQUEUE flag set or no commands are processed
 * @attention Queue entries marked processed (player = NOTHING) before execution prevents retry on error
 * @attention Global register context (mushstate.rdata) shared across batch execution
 *
 * @see test_top() for queue readiness check
 * @see give_que() for normal queue insertion
 * @see process_cmdent() for command parsing and execution
 * @see delete_qentry() for queue entry cleanup and deallocation
 * @see do_second() for wait/semaphore queue processing that feeds this queue
 */
int do_top(int ncmds)
{
	BQUE *tmp = NULL;
	dbref player = NOTHING;
	int count = 0;
	char *cmdsave = NULL;

	/* Early exit if command processing is disabled */
	if (!(mushconf.control_flags & CF_DEQUEUE))
	{
		return 0;
	}

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< do_top >";

	for (count = 0; count < ncmds; count++)
	{
		/* Check if queue has commands */
		if (!test_top())
		{
			mushstate.debug_cmd = cmdsave;
			_free_gdata(mushstate.rdata);
			mushstate.rdata = NULL;
			return count;
		}

		player = mushstate.qfirst->player;

		/* Process valid player commands */
		if ((player >= 0) && !Going(player))
		{
			giveto(player, mushconf.waitcost);
			mushstate.curr_enactor = mushstate.qfirst->cause;
			mushstate.curr_player = player;
			a_Queue(Owner(player), -1);
			mushstate.qfirst->player = NOTHING;

			if (!Halted(player))
			{
				/* Load scratch registers from queue entry */
				if (mushstate.qfirst->gdata)
				{
					/* Clean up existing register data */
					_free_gdata(mushstate.rdata);

					/* Allocate new register structure if needed */
					if (mushstate.qfirst->gdata->q_alloc || mushstate.qfirst->gdata->xr_alloc)
					{
						mushstate.rdata = (GDATA *)XMALLOC(sizeof(GDATA), "do_top");
						mushstate.rdata->q_alloc = mushstate.qfirst->gdata->q_alloc;
						mushstate.rdata->q_regs = mushstate.qfirst->gdata->q_alloc ? XCALLOC(mushstate.qfirst->gdata->q_alloc, sizeof(char *), "q_regs") : NULL;
						mushstate.rdata->q_lens = mushstate.qfirst->gdata->q_alloc ? XCALLOC(mushstate.qfirst->gdata->q_alloc, sizeof(int), "q_lens") : NULL;
						mushstate.rdata->xr_alloc = mushstate.qfirst->gdata->xr_alloc;
						mushstate.rdata->x_names = mushstate.qfirst->gdata->xr_alloc ? XCALLOC(mushstate.qfirst->gdata->xr_alloc, sizeof(char *), "x_names") : NULL;
						mushstate.rdata->x_regs = mushstate.qfirst->gdata->xr_alloc ? XCALLOC(mushstate.qfirst->gdata->xr_alloc, sizeof(char *), "x_regs") : NULL;
						mushstate.rdata->x_lens = mushstate.qfirst->gdata->xr_alloc ? XCALLOC(mushstate.qfirst->gdata->xr_alloc, sizeof(int), "x_lens") : NULL;
						mushstate.rdata->dirty = 0;
					}
					else
					{
						mushstate.rdata = NULL;
					}

					/* Copy q-register contents */
					if (mushstate.qfirst->gdata->q_alloc)
					{
						for (int z = 0; z < mushstate.qfirst->gdata->q_alloc; z++)
						{
							if (mushstate.qfirst->gdata->q_regs[z] && *(mushstate.qfirst->gdata->q_regs[z]))
							{
								mushstate.rdata->q_regs[z] = XMALLOC(LBUF_SIZE, "do_top");
								XMEMCPY(mushstate.rdata->q_regs[z], mushstate.qfirst->gdata->q_regs[z], mushstate.qfirst->gdata->q_lens[z] + 1);
								mushstate.rdata->q_lens[z] = mushstate.qfirst->gdata->q_lens[z];
							}
						}
					}

					/* Copy x-register contents */
					if (mushstate.qfirst->gdata->xr_alloc)
					{
						for (int z = 0; z < mushstate.qfirst->gdata->xr_alloc; z++)
						{
							if (mushstate.qfirst->gdata->x_names[z] && *(mushstate.qfirst->gdata->x_names[z]) && mushstate.qfirst->gdata->x_regs[z] && *(mushstate.qfirst->gdata->x_regs[z]))
							{
								mushstate.rdata->x_names[z] = XMALLOC(SBUF_SIZE, "glob.x_name");
								XSTRNCPY(mushstate.rdata->x_names[z], mushstate.qfirst->gdata->x_names[z], SBUF_SIZE);
								mushstate.rdata->x_regs[z] = XMALLOC(LBUF_SIZE, "glob.x_reg");
								XMEMCPY(mushstate.rdata->x_regs[z], mushstate.qfirst->gdata->x_regs[z], mushstate.qfirst->gdata->x_lens[z] + 1);
								mushstate.rdata->x_lens[z] = mushstate.qfirst->gdata->x_lens[z];
							}
						}
					}

					if (mushstate.rdata)
					{
						mushstate.rdata->dirty = mushstate.qfirst->gdata->dirty;
					}
				}
				else
				{
					/* No register data in queue entry - clean up existing */
					_free_gdata(mushstate.rdata);
					mushstate.rdata = NULL;
				}

				mushstate.cmd_invk_ctr = 0;
				process_cmdline(player, mushstate.qfirst->cause, mushstate.qfirst->comm, mushstate.qfirst->env, mushstate.qfirst->nargs, mushstate.qfirst);
			}
		}

		/* Remove processed entry from queue */
		if (mushstate.qfirst)
		{
			tmp = mushstate.qfirst;
			mushstate.qfirst = mushstate.qfirst->next;
			delete_qentry(tmp);
		}

		if (!mushstate.qfirst)
		{
			mushstate.qlast = NULL;
		}
	}

	/* Final cleanup */
	_free_gdata(mushstate.rdata);
	mushstate.rdata = NULL;
	mushstate.debug_cmd = cmdsave;
	return count;
}
