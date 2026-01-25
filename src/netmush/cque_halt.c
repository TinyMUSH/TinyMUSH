/**
 * @file cque_halt.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue halt operations and command processing
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

/* Local prototype for helper shared with wait module */
bool _cque_parse_pid_string(const char *pidstr, int *qpid);

/**
 * @brief Filter queue entries by owner and/or object criteria.
 *
 * Determines if a queue entry matches specified filtering criteria based on the
 * entry's player and owner. Used by queue operations (halt, display, etc.) to
 * select which entries to process. If both targets are NOTHING, all valid entries
 * match. If only one target is specified, entries must match that criterion.
 *
 * @param entry Queue entry to evaluate
 * @param ptarg Player target filter (NOTHING = don't filter by player owner)
 * @param otarg Object target filter (NOTHING = don't filter by object)
 * @return true if entry matches filtering criteria, false otherwise
 *
 * @note Thread-safe: Yes (read-only operation)
 * @note Entries with invalid player dbrefs (halted entries) never match
 *
 * @see halt_que() for primary usage example
 * @see cque_show_que() for display filtering
 */
bool cque_que_want(BQUE *entry, dbref ptarg, dbref otarg)
{
	return Good_obj(entry->player) &&
	       ((ptarg == NOTHING) || (ptarg == Owner(entry->player))) &&
	       ((otarg == NOTHING) || (otarg == entry->player));
}

/**
 * @brief Record a halted queue entry for accounting and tracking.
 *
 * Helper function that increments the halt counter and tracks owner costs when
 * performing a global halt-all operation. Checks ownership validity before updating
 * the per-owner accounting array.
 *
 * @param numhalted Pointer to halt counter to increment
 * @param dbrefs_array Owner accounting array (NULL if not halt-all mode)
 * @param entry Queue entry being halted
 * @param halt_all Non-zero if in global halt-all mode
 *
 * @note Thread-safe: No (modifies the dbrefs_array parameter)
 * @note dbrefs_array must be allocated by caller if halt_all is non-zero
 */
static void _cque_halt_record(int *numhalted, int *dbrefs_array, BQUE *entry, int halt_all)
{
	(*numhalted)++;
	if (halt_all && Good_obj(entry->player))
	{
		dbrefs_array[Owner(entry->player)] += 1;
	}
}

/**
 * @brief Halt and remove queued commands matching specified player/object criteria.
 *
 * Traverses all four queue types (player, object, wait, semaphore) and halts entries
 * matching the specified player owner and/or object. Halted entries in execution queues
 * (player/object) are flagged but not immediately deleted. Entries in wait/semaphore
 * queues are removed and freed. Refunds wait costs and adjusts queue counters.
 *
 * Special case: When both player and object are NOTHING, performs a global halt-all
 * operation that tracks and refunds costs per owner.
 *
 * @param player Player owner to match (NOTHING = don't filter by owner, or halt-all mode)
 * @param object Object to match (NOTHING = don't filter by object)
 * @return Number of queue entries halted
 *
 * @note Thread-safe: No (modifies global queue state)
 * @note Halted entries are marked with player=NOTHING to prevent execution
 * @note Wait/semaphore entries are immediately deleted; execution queue entries remain
 * @attention Global halt-all (both params NOTHING) requires special permission checks
 *
 * @see cque_do_halt() for command interface
 * @see cque_que_want() for filtering logic
 */
int halt_que(dbref player, dbref object)
{
	BQUE *trail = NULL, *point = NULL, *next = NULL;
	int numhalted = 0, halt_all = 0, i = 0;
	int *dbrefs_array = NULL;

	numhalted = 0;
	halt_all = ((player == NOTHING) && (object == NOTHING)) ? 1 : 0;

	if (halt_all)
	{
		dbrefs_array = (int *)XCALLOC(mushstate.db_top, sizeof(int), "dbrefs_array");
	}

	/* Player queue */
	for (point = mushstate.qfirst; point; point = point->next)
		if (cque_que_want(point, player, object))
		{
			_cque_halt_record(&numhalted, dbrefs_array, point, halt_all);
			point->player = NOTHING;
		}

	/* Object queue */
	for (point = mushstate.qlfirst; point; point = point->next)
		if (cque_que_want(point, player, object))
		{
			_cque_halt_record(&numhalted, dbrefs_array, point, halt_all);
			point->player = NOTHING;
		}

	/* Wait queue */
	for (point = mushstate.qwait, trail = NULL; point; point = next)
		if (cque_que_want(point, player, object))
		{
			_cque_halt_record(&numhalted, dbrefs_array, point, halt_all);
			if (trail)
			{
				trail->next = next = point->next;
			}
			else
			{
				mushstate.qwait = next = point->next;
			}

			cque_delete_qentry(point);
		}
		else
		{
			next = (trail = point)->next;
		}

	/* Semaphore queue */
	for (point = mushstate.qsemfirst, trail = NULL; point; point = next)
		if (cque_que_want(point, player, object))
		{
			_cque_halt_record(&numhalted, dbrefs_array, point, halt_all);
			if (trail)
			{
				trail->next = next = point->next;
			}
			else
			{
				mushstate.qsemfirst = next = point->next;
			}

			if (point == mushstate.qsemlast)
			{
				mushstate.qsemlast = trail;
			}

			cque_add_to(player, point->sem, -1, point->attr);
			cque_delete_qentry(point);
		}
		else
		{
			next = (trail = point)->next;
		}

	if (halt_all)
	{
		for (i = 0; i < mushstate.db_top; i++)
		{
			if (dbrefs_array[i])
			{
				giveto(i, (mushconf.waitcost * dbrefs_array[i]));
				s_Queue(i, 0);
			}
		}

		XFREE(dbrefs_array);
		return numhalted;
	}

	if (player == NOTHING)
	{
		player = Owner(object);
	}

	giveto(player, (mushconf.waitcost * numhalted));

	if (object == NOTHING)
	{
		s_Queue(player, 0);
	}
	else
	{
		a_Queue(player, -numhalted);
	}

	return numhalted;
}

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
bool _cque_parse_pid_string(const char *pidstr, int *qpid)
{
	char *endptr = NULL;
	long val = 0;

	if (!is_integer((char *)pidstr))
	{
		return false;
	}

	errno = 0;
	val = strtol((char *)pidstr, &endptr, 10);

	if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == pidstr || *endptr != '\0')
	{
		return false;
	}

	*qpid = (int)val;

	if ((*qpid < 1) || (*qpid > mushconf.max_qpid))
	{
		return false;
	}

	return true;
}

/**
 * @brief Halt a specific queue entry identified by its process ID (PID).
 *
 * Validates and parses the PID string, locates the corresponding queue entry in the
 * PID hash table, and halts it after performing permission checks. The entry is removed
 * from its queue (wait or semaphore), all resources are freed, and the wait cost is
 * refunded to the entry owner. This provides targeted control over individual queued
 * commands without affecting other entries.
 *
 * Validation steps include: integer format checking, range validation against max_qpid,
 * existence verification in hash table, halt status checking, and permission verification
 * (Controls or Can_Halt).
 *
 * @param player DBref of the player issuing the halt command
 * @param cause  DBref of the cause object (unused in this function)
 * @param key    Command key flags (unused in this function)
 * @param pidstr String representation of the PID to halt
 *
 * @note Thread-safe: No (modifies global queue state and hash table)
 * @note Notifies player of validation errors with specific error messages
 * @attention PID must be in range [1, max_qpid] to be considered valid
 * @attention Requires Controls permission on entry owner or Can_Halt privilege
 *
 * @see halt_que() for halting multiple entries by player/object criteria
 * @see cque_remove_waitq() for wait queue removal
 * @see cque_delete_qentry() for resource cleanup
 */
void cque_do_halt_pid(dbref player, dbref cause, int key, char *pidstr)
{
	dbref victim = NOTHING;
	int qpid = 0;
	BQUE *qptr = NULL;

	if (!_cque_parse_pid_string(pidstr, &qpid))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	qptr = (BQUE *)nhashfind(qpid, &mushstate.qpid_htab);

	if (!qptr)
	{
		notify(player, "That PID is not associated with an active queue entry.");
		return;
	}

	if (qptr->player == NOTHING)
	{
		notify(player, "That queue entry has already been halted.");
		return;
	}

	if (!(Controls(player, qptr->player) || Can_Halt(player)))
	{
		notify(player, "Permission denied.");
		return;
	}

	/* Flag as halted and remove from wait/semaphore queues if needed */
	victim = Owner(qptr->player);
	qptr->player = NOTHING;

	if (qptr->sem == NOTHING)
	{
		cque_remove_waitq(qptr);
	}
	else
	{
		/* Remove from semaphore queue using pointer-to-pointer technique */
		BQUE **pptr = &mushstate.qsemfirst, *prev = NULL;
		
		while (*pptr && *pptr != qptr)
		{
			prev = *pptr;
			pptr = &((*pptr)->next);
		}

		if (*pptr)
		{
			*pptr = qptr->next;

			if (mushstate.qsemlast == qptr)
			{
				mushstate.qsemlast = prev;
			}
		}

		cque_add_to(player, qptr->sem, -1, qptr->attr);
	}

	cque_delete_qentry(qptr);
	giveto(victim, mushconf.waitcost);
	a_Queue(victim, -1);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Halted queue entry PID %d.", qpid);
}

/**
 * @brief Parse and validate halt command target specification.
 *
 * Resolves the target string into player and object references for halting operations.
 * Handles empty targets (halts caller's entries), player targets, and object targets.
 * Validates that the resolved target is accessible based on the player's privileges.
 *
 * @param player DBref of the player issuing the halt command
 * @param key Command flags (HALT_ALL controls permission requirement)
 * @param target Target specification string (object name or empty)
 * @param player_targ Pointer to store resolved player target (NOTHING = no player target)
 * @param obj_targ Pointer to store resolved object target (NOTHING = no object target)
 * @return true if target parsed successfully, false on error (with notify() already called)
 *
 * @note Thread-safe: No (calls match_* functions which may modify game state)
 * @note Notifies player of errors (no match, permission denied, conflicting flags)
 * @note Sets player_targ and obj_targ to appropriate values for halt_que()
 */
static bool _cque_parse_halt_target(dbref player, int key, const char *target,
                                dbref *player_targ, dbref *obj_targ)
{
	/* Empty target: halt caller's own entries (or all if HALT_ALL) */
	if (!target || !*target)
	{
		*obj_targ = NOTHING;
		*player_targ = (key & HALT_ALL) ? NOTHING : Owner(player);

		if (Typeof(player) != TYPE_PLAYER)
		{
			*obj_targ = player;
		}

		return true;
	}

	/* Specified target: resolve it */
	if (Can_Halt(player))
	{
		*obj_targ = match_thing(player, (char *)target);
	}
	else
	{
		*obj_targ = match_controlled(player, (char *)target);
	}

	if (!Good_obj(*obj_targ))
	{
		return false;
	}

	if (key & HALT_ALL)
	{
		notify(player, "Can't specify a target and /all");
		return false;
	}

	/* Distinguish players from objects for filtering */
	if (Typeof(*obj_targ) == TYPE_PLAYER)
	{
		*player_targ = *obj_targ;
		*obj_targ = NOTHING;
	}
	else
	{
		*player_targ = NOTHING;
	}

	return true;
}

/**
 * @brief Command interface for halting queued commands by various criteria.
 *
 * Provides flexible queue halting capabilities through multiple modes:
 * - PID mode (HALT_PID): Halts a specific queue entry by process ID
 * - Target mode (default): Halts entries owned by or associated with specified object
 * - All mode (HALT_ALL): Halts all entries owned by caller (or globally if privileged)
 *
 * Target parsing determines halt scope: empty target halts caller's own entries
 * (and entries run by non-player objects owned by caller), specified target halts
 * that player's or object's entries. Players are distinguished from objects to
 * determine correct ownership filtering.
 *
 * Permission requirements vary by mode: HALT_ALL requires Can_Halt privilege,
 * target mode requires either Can_Halt (for any target) or Controls permission
 * (for specific target). Reports number of halted entries unless player is Quiet.
 *
 * @param player DBref of the player issuing the halt command
 * @param cause  DBref of the cause object (passed to do_halt_pid if needed)
 * @param key    Command flags: HALT_PID for PID mode, HALT_ALL for global halt
 * @param target Target specification: PID string (if HALT_PID), object name, or empty
 *
 * @note Thread-safe: No (delegates to functions that modify global queue state)
 * @note HALT_ALL without target and with Can_Halt privilege performs global halt
 * @attention Cannot combine HALT_ALL flag with a specific target
 * @attention Permission checks occur before any queue modification
 *
 * @see halt_que() for the underlying halt implementation
 * @see cque_do_halt_pid() for PID-based halting
 * @see cque_que_want() for filtering logic
 */
void cque_do_halt(dbref player, dbref cause, int key, char *target)
{
	dbref player_targ = NOTHING, obj_targ = NOTHING;
	int numhalted = 0;

	if (key & HALT_PID)
	{
		cque_do_halt_pid(player, cause, key, target);
		return;
	}

	if ((key & HALT_ALL) && !Can_Halt(player))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	if (!_cque_parse_halt_target(player, key, target, &player_targ, &obj_targ))
	{
		return;
	}

	numhalted = halt_que(player_targ, obj_targ);

	if (!Quiet(player))
	{
		if (numhalted == 1)
		{
			notify(Owner(player), "1 queue entries removed.");
		}
		else
		{
			notify_check(Owner(player), Owner(player), MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN,
						 "%d queue entries removed.", numhalted);
		}
	}
}
