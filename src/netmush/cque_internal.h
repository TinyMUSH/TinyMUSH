/**
 * @file cque_internal.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Internal header for command queue modules
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * This header provides shared declarations and internal interfaces for the queue
 * management modules (cque_entry.c, cque_halt.c, cque_notify.c, cque_wait.c,
 * cque_exec.c, cque_queue.c).
 */

#pragma once

/* ==================== Global Variables ==================== */

/**
 * @brief Next queue PID to allocate (internal optimization)
 * 
 * Shared variable tracking the next available queue process ID.
 * Access protected by queue management functions.
 */
extern int qpid_top;

/* ==================== Entry Management ==================== */

/**
 * @brief Delete and free a queue entry, releasing all associated resources.
 *
 * Removes the entry from the PID hash table and frees all allocated memory including
 * command text, global registers (q_regs), extended registers (x_regs), and the queue
 * entry structure itself.
 *
 * @param qptr Queue entry to delete
 *
 * @note Thread-safe: No (must be called with appropriate locking)
 * @attention Caller must ensure qptr has been removed from any linked lists before calling
 */
void delete_qentry(BQUE *qptr);

/**
 * @brief Add a semaphore counter value and return the new total.
 *
 * Implements semaphore increment/decrement operation with range clamping.
 * Used for @notify and timed semaphore timeout handling.
 *
 * @param doer       DBref of player performing the add
 * @param player     DBref of the semaphore owner
 * @param am         Amount to add (positive or negative)
 * @param attrnum    Attribute number for semaphore counter
 * @return New semaphore counter value (clamped to INT_MAX/INT_MIN)
 */
int add_to(dbref doer, dbref player, int am, int attrnum);

/**
 * @brief Insert a queue entry into the normal (player) queue for immediate execution.
 *
 * Adds entry to the end of mushstate.qfirst queue, or becomes the head if queue is empty.
 * Used by do_second() to promote expired wait/semaphore entries to normal queue.
 *
 * @param tmp Queue entry to insert
 */
void give_que(BQUE *tmp);

/**
 * @brief Remove a queue entry from the wait queue without deletion.
 *
 * Extracts entry from mushstate.qwait sorted by waittime. Used before re-inserting
 * with different waittime or moving to different queue.
 *
 * @param qptr Queue entry to remove (must be in wait queue)
 */
void remove_waitq(BQUE *qptr);

/**
 * @brief Clean up and free global register data (GDATA) structure.
 *
 * Frees all allocated memory in a GDATA structure including q-registers, x-registers,
 * and their associated length arrays. Handles NULL pointers gracefully.
 *
 * @param gdata Pointer to GDATA structure to free (may be NULL)
 *
 * @note Thread-safe: Yes (no side effects beyond deallocation)
 */
void _free_gdata(GDATA *gdata);

/**
 * @brief Parse and validate a PID string into an integer value.
 *
 * Validates the PID string format and range, ensuring it represents a valid
 * process ID within the configured queue limits. Performs comprehensive error
 * checking for parsing and range validation.
 *
 * @param pidstr String representation of the PID
 * @param qpid Pointer to store the parsed PID value (only valid if returns true)
 * @return true if pidstr is valid and parsed successfully, false otherwise
 *
 * @note Thread-safe: Yes (read-only, no side effects beyond output parameter)
 * @note Valid PID range: [1, max_qpid]
 * @attention qpid pointer must be valid when calling this function
 */
bool _parse_pid_string(const char *pidstr, int *qpid);

/* ==================== Wait Queue Management ==================== */

/**
 * @brief Create and queue a command to execute after specified delay or semaphore release.
 *
 * Main queueing function that handles both timed delays and semaphore-based blocking.
 * Allocates queue entry, configures wait conditions, inserts into appropriate queue.
 *
 * @param player   DBref of player to queue command for
 * @param cause    DBref of cause object
 * @param wait     Seconds to delay before execution (0 for semaphore waits)
 * @param sem      Semaphore object DBref (NOTHING for timed waits)
 * @param attr     Attribute for semaphore counter
 * @param command  Command text to queue
 * @param args     Environment variable arguments (%0-%9)
 * @param nargs    Number of arguments
 * @param gargs    Global register context to preserve
 */
void wait_que(dbref player, dbref cause, int wait, dbref sem, int attr, char *command, char *args[], int nargs, GDATA *gargs);

/* ==================== Execution Engine ==================== */

/**
 * @brief Calculate seconds until next queue command is ready for execution.
 *
 * Scans all four queue types and returns minimum time until any command becomes ready.
 * Used by main event loop for sleep time calculation.
 *
 * @return Seconds to sleep before next processing cycle
 */
int que_next(void);

/**
 * @brief Process expired wait queue and semaphore queue entries for execution.
 *
 * Called once per second by main event loop. Promotes object queue, processes expired
 * waits, handles semaphore timeouts.
 *
 * @note Thread-safe: No (modifies global queue structures)
 */
void do_second(void);

/**
 * @brief Execute up to ncmds commands from the player queue (normal priority).
 *
 * Main command execution engine that dequeues and runs commands from mushstate.qfirst.
 *
 * @param ncmds Maximum number of commands to execute
 * @return Number of commands actually executed
 *
 * @note Thread-safe: No (modifies global queue structures and player state)
 */
int do_top(int ncmds);

/**
 * @brief Allocate and return next available queue process ID.
 *
 * Thread-safe atomic operation for PID allocation to track queue entries.
 *
 * @return New unique queue PID
 */
int qpid_next(void);
