/**
 * @file cque.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command queue scheduling, throttling, and execution utilities
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

static int qpid_top = 1; /*!< Next queue PID to allocate (internal optimization) */

/**
 * @brief Delete and free a queue entry, releasing all associated resources.
 *
 * Removes the entry from the PID hash table and frees all allocated memory including
 * command text, global registers (q_regs), extended registers (x_regs), and the queue
 * entry structure itself. This function must be called for proper cleanup of queue entries
 * to prevent memory leaks.
 *
 * @param qptr Queue entry to delete
 *
 * @note Thread-safe: No (must be called with appropriate locking)
 * @attention Caller must ensure qptr has been removed from any linked lists before calling
 */
void delete_qentry(BQUE *qptr)
{
	GDATA *gdata = NULL;
	int i = 0;

	nhashdelete(qptr->pid, &mushstate.qpid_htab);
	XFREE(qptr->text);

	if ((gdata = qptr->gdata) != NULL)
	{
		/* Free individual q-register strings */
		for (i = 0; i < gdata->q_alloc; i++)
		{
			if (gdata->q_regs[i])
			{
				XFREE(gdata->q_regs[i]);
			}
		}

		/* Free individual x-register names and values */
		for (i = 0; i < gdata->xr_alloc; i++)
		{
			if (gdata->x_names[i])
			{
				XFREE(gdata->x_names[i]);
			}
			if (gdata->x_regs[i])
			{
				XFREE(gdata->x_regs[i]);
			}
		}

		/* Free the register arrays themselves */
		if (gdata->q_regs)
		{
			XFREE(gdata->q_regs);
		}
		if (gdata->q_lens)
		{
			XFREE(gdata->q_lens);
		}
		if (gdata->x_names)
		{
			XFREE(gdata->x_names);
		}
		if (gdata->x_regs)
		{
			XFREE(gdata->x_regs);
		}
		if (gdata->x_lens)
		{
			XFREE(gdata->x_lens);
		}
		XFREE(gdata);
	}

	XFREE(qptr);
}

/**
 * @brief Adjust an object's queue or semaphore count attribute.
 *
 * Reads the specified attribute from the player object, interprets it as an integer
 * count, adds the adjustment value (am), and writes the result back. If the resulting
 * count is zero, the attribute is cleared. This function is used to track semaphore
 * wait counts and queue throttling limits.
 *
 * @param doer    DBref of the actor performing the adjustment (for ownership tracking)
 * @param player  DBref of the object whose attribute is being adjusted
 * @param am      Amount to add (positive) or subtract (negative) from current value
 * @param attrnum Attribute number to read/modify (e.g., A_SEMAPHORE)
 * @return New attribute value after adjustment, or 0 if attribute was invalid/cleared
 *
 * @note Thread-safe: No (modifies database attributes)
 * @note Invalid or non-numeric attribute values are treated as 0
 */
int add_to(dbref doer, dbref player, int am, int attrnum)
{
	int num = 0, aflags = 0, alen = 0;
	dbref aowner = NOTHING;
	char *buff = NULL;
	char *atr_gotten = NULL;
	char *endptr = NULL;
	long val = 0;

	/* Get attribute value and parse it safely */
	atr_gotten = atr_get(player, attrnum, &aowner, &aflags, &alen);

	errno = 0;
	val = strtol(atr_gotten, &endptr, 10);

	/* Validate conversion and range - if successful, use the value */
	if (errno != ERANGE && val <= INT_MAX && val >= INT_MIN && (*endptr == '\0' || isspace(*endptr)))
	{
		num = (int)val;
	}

	XFREE(atr_gotten);
	num += am;

	buff = num ? ltos(num) : NULL;
	atr_add(player, attrnum, buff, Owner(doer), aflags);
	XFREE(buff);

	return num;
}

/**
 * @brief Thread a queue entry onto the appropriate priority queue for execution.
 *
 * Adds a queue entry to either the high-priority (player) queue or low-priority
 * (object) queue based on the cause type. Player-caused commands are queued with
 * higher priority to ensure responsive gameplay. The entry is appended to the end
 * of the appropriate queue. Resets waittime to 0 and next pointer to NULL before
 * queueing.
 *
 * @param tmp Queue entry to add to execution queue
 *
 * @note Thread-safe: No (modifies global queue state)
 * @note Entry must be fully initialized before calling this function
 * @attention Does not check for null pointer - caller must validate
 *
 * @see wait_que() for delayed queue entries
 * @see do_top() for queue execution
 */
void give_que(BQUE *tmp)
{
	BQUE **qhead = NULL, **qtail = NULL;

	tmp->next = NULL;
	tmp->waittime = 0;

	/* Determine which queue to use based on cause type */
	if (Typeof(tmp->cause) == TYPE_PLAYER)
	{
		qhead = &mushstate.qfirst;
		qtail = &mushstate.qlast;
	}
	else
	{
		qhead = &mushstate.qlfirst;
		qtail = &mushstate.qllast;
	}

	/* Add to end of queue */
	if (*qtail != NULL)
	{
		(*qtail)->next = tmp;
		*qtail = tmp;
	}
	else
	{
		*qhead = *qtail = tmp;
	}
}

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
 * @see show_que() for display filtering
 */
bool que_want(BQUE *entry, dbref ptarg, dbref otarg)
{
	return Good_obj(entry->player) &&
	       ((ptarg == NOTHING) || (ptarg == Owner(entry->player))) &&
	       ((otarg == NOTHING) || (otarg == entry->player));
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
 * @see do_halt() for command interface
 * @see que_want() for filtering logic
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
		if (que_want(point, player, object))
		{
			numhalted++;

			if (halt_all && Good_obj(point->player))
			{
				dbrefs_array[Owner(point->player)] += 1;
			}

			point->player = NOTHING;
		}

	/* Object queue */
	for (point = mushstate.qlfirst; point; point = point->next)
		if (que_want(point, player, object))
		{
			numhalted++;

			if (halt_all && Good_obj(point->player))
			{
				dbrefs_array[Owner(point->player)] += 1;
			}

			point->player = NOTHING;
		}

	/* Wait queue */
	for (point = mushstate.qwait, trail = NULL; point; point = next)
		if (que_want(point, player, object))
		{
			numhalted++;

			if (halt_all && Good_obj(point->player))
			{
				dbrefs_array[Owner(point->player)] += 1;
			}

			if (trail)
			{
				trail->next = next = point->next;
			}
			else
			{
				mushstate.qwait = next = point->next;
			}

			delete_qentry(point);
		}
		else
		{
			next = (trail = point)->next;
		}

	/* Semaphore queue */
	for (point = mushstate.qsemfirst, trail = NULL; point; point = next)
		if (que_want(point, player, object))
		{
			numhalted++;

			if (halt_all && Good_obj(point->player))
			{
				dbrefs_array[Owner(point->player)] += 1;
			}

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

			add_to(player, point->sem, -1, point->attr);
			delete_qentry(point);
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
 * @brief Remove a specific entry from the time-sorted wait queue.
 *
 * Searches for and unlinks the specified queue entry from the wait queue linked list
 * without freeing it. Handles both head-of-queue and mid-queue removal cases. This
 * function only removes the entry from the linked list structure; the caller is
 * responsible for freeing the entry's memory if needed.
 *
 * @param qptr Queue entry to remove from wait queue
 *
 * @note Thread-safe: No (modifies global wait queue structure)
 * @note Does not free the entry - caller must call delete_qentry() separately
 * @attention Entry must actually be in the wait queue or behavior is undefined
 * @attention If entry is not found, queue remains unchanged (silent failure)
 *
 * @see wait_que() for adding entries to wait queue
 * @see do_wait_pid() for wait time adjustment that uses this function
 */
void remove_waitq(BQUE *qptr)
{
	BQUE *point = NULL, *trail = NULL;

	if (qptr == mushstate.qwait)
	{
		/* Head of the queue. Just remove it and relink. */
		mushstate.qwait = qptr->next;
	}
	else
	{
		/* Go find it somewhere in the queue and take it out. */
		for (point = mushstate.qwait, trail = NULL; point != NULL; point = point->next)
		{
			if (qptr == point)
			{
				trail->next = qptr->next;
				break;
			}

			trail = point;
		}
	}
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
 * @see remove_waitq() for wait queue removal
 * @see delete_qentry() for resource cleanup
 */
void do_halt_pid(dbref player, dbref cause, int key, char *pidstr)
{
	dbref victim = NOTHING;
	int qpid = 0;
	BQUE *qptr = NULL, *last = NULL, *tmp = NULL;
	char *endptr = NULL;
	long val = 0;

	if (!is_integer(pidstr))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	errno = 0;
	val = strtol(pidstr, &endptr, 10);

	if (errno == ERANGE || val > INT_MAX || val < INT_MIN)
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	if (endptr == pidstr || *endptr != '\0')
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	qpid = (int)val;

	if ((qpid < 1) || (qpid > mushconf.max_qpid))
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
		remove_waitq(qptr);
		delete_qentry(qptr);
	}
	else
	{
		for (tmp = mushstate.qsemfirst, last = NULL; tmp != NULL; last = tmp, tmp = tmp->next)
		{
			if (tmp == qptr)
			{
				if (last)
				{
					last->next = tmp->next;
				}
				else
				{
					mushstate.qsemfirst = tmp->next;
				}

				if (mushstate.qsemlast == tmp)
				{
					mushstate.qsemlast = last;
				}

				break;
			}
		}

		add_to(player, qptr->sem, -1, qptr->attr);
		delete_qentry(qptr);
	}

	giveto(victim, mushconf.waitcost);
	a_Queue(victim, -1);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Halted queue entry PID %d.", qpid);
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
 * @see do_halt_pid() for PID-based halting
 * @see que_want() for filtering logic
 */
void do_halt(dbref player, dbref cause, int key, char *target)
{
	dbref player_targ = NOTHING, obj_targ = NOTHING;
	int numhalted = 0;

	if (key & HALT_PID)
	{
		do_halt_pid(player, cause, key, target);
		return;
	}

	if ((key & HALT_ALL) && !(Can_Halt(player)))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/* Figure out what to halt */
	if (!target || !*target)
	{
		obj_targ = NOTHING;

		if (key & HALT_ALL)
		{
			player_targ = NOTHING;
		}
		else
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
		if (Can_Halt(player))
		{
			obj_targ = match_thing(player, target);
		}
		else
		{
			obj_targ = match_controlled(player, target);
		}

		if (!Good_obj(obj_targ))
		{
			return;
		}

		if (key & HALT_ALL)
		{
			notify(player, "Can't specify a target and /all");
			return;
		}

		if (Typeof(obj_targ) == TYPE_PLAYER)
		{
			player_targ = obj_targ;
			obj_targ = NOTHING;
		}
		else
		{
			player_targ = NOTHING;
		}
	}

	numhalted = halt_que(player_targ, obj_targ);

	if (Quiet(player))
	{
		return;
	}

	if (numhalted == 1)
	{
		notify(Owner(player), "1 queue entries removed.");
	}
	else
	{
		notify_check(Owner(player), Owner(player), MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d queue entries removed.", numhalted);
	}
}

/**
 * @brief Release and process commands waiting on a semaphore.
 *
 * Reads the semaphore counter from the specified attribute (or A_SEMAPHORE if none specified),
 * and if positive, removes up to 'count' matching entries from the semaphore queue. Entries
 * are either executed (queued to execution queue) or discarded (with refund) based on the key.
 * The semaphore counter is then decremented by the notification count.
 *
 * Processing modes:
 * - NFY_NFY (notify): Removes up to 'count' entries and queues them for execution
 * - NFY_DRAIN (drain): Removes all matching entries and discards them with refunds
 *
 * If the semaphore counter is <= 0, no entries are processed. When attr is 0, uses
 * A_SEMAPHORE and treats counter as 1. Invalid or missing attribute values are treated as 0.
 *
 * @param player DBref of the player performing the notification (for attribute updates)
 * @param sem    DBref of the semaphore object whose waiters should be notified
 * @param attr   Attribute number containing semaphore counter (0 = use A_SEMAPHORE)
 * @param key    Operation mode: NFY_NFY to execute commands, NFY_DRAIN to discard
 * @param count  Maximum number of entries to notify (NFY_NFY mode only)
 * @return Number of queue entries actually processed/notified
 *
 * @note Thread-safe: No (modifies global semaphore queue and database attributes)
 * @note Semaphore counter must be positive for any processing to occur
 * @note NFY_DRAIN mode processes all matching entries regardless of count parameter
 * @attention Clears the semaphore attribute entirely in NFY_DRAIN mode
 *
 * @see do_notify() for the command interface
 * @see wait_que() for adding entries to semaphore queue
 * @see give_que() for queuing entries for execution
 */
int nfy_que(dbref player, dbref sem, int attr, int key, int count)
{
	BQUE *point = NULL, *trail = NULL, *next = NULL;
	int num = 0, aflags = 0, alen = 0;
	dbref aowner = NOTHING;
	char *str = NULL;
	char *endptr = NULL;
	long val = 0;

	if (attr)
	{
		str = atr_get(sem, attr, &aowner, &aflags, &alen);

		if (str && *str)
		{
			errno = 0;
			val = strtol(str, &endptr, 10);

			if (errno != ERANGE && val >= INT_MIN && val <= INT_MAX && endptr != str && *endptr == '\0')
			{
				num = (int)val;
			}
			else
			{
				num = 0;
			}
		}
		else
		{
			num = 0;
		}

		XFREE(str);
	}
	else
	{
		num = 1;
	}

	if (num > 0)
	{
		num = 0;

		for (point = mushstate.qsemfirst, trail = NULL; point; point = next)
		{
			if ((point->sem == sem) && ((point->attr == attr) || !attr))
			{
				num++;

				if (trail)
				{
					trail->next = next = point->next;
				}
				else
					mushstate.qsemfirst = next = point->next;

				if (point == mushstate.qsemlast)
				{
					mushstate.qsemlast = trail;
				}

				/* Either run or discard the command */
				if (key != NFY_DRAIN)
				{
					give_que(point);
				}
				else
				{
					giveto(point->player, mushconf.waitcost);
					a_Queue(Owner(point->player), -1);
					delete_qentry(point);
				}
			}
			else
			{
				next = (trail = point)->next;
			}
			/* If we've notified enough, exit */
			if ((key == NFY_NFY) && (num >= count))
			{
				next = NULL;
			}
		}
	}
	else
	{
		num = 0;
	}

	/* Update the sem waiters count */
	if (key == NFY_NFY)
	{
		add_to(player, sem, -count, (attr ? attr : A_SEMAPHORE));
	}
	else
	{
		atr_clr(sem, (attr ? attr : A_SEMAPHORE));
	}

	return num;
}

/**
 * @brief Command interface for notifying and releasing semaphore-blocked commands.
 *
 * Parses target specification (object[/attribute]) to identify the semaphore object and
 * optional attribute containing the semaphore counter. Validates permissions (controls or
 * Link_ok), parses the count parameter, and delegates to nfy_que() to process waiting
 * commands. Provides user feedback on completion unless both player and target are Quiet.
 *
 * Target format: "object" uses A_SEMAPHORE attribute, "object/attribute" uses specified
 * attribute. The attribute must exist and player must have Set_attr permission on it.
 * Count defaults to 1 if not specified. Key determines operation mode (NFY_NFY to
 * execute commands, NFY_DRAIN to discard them).
 *
 * Permission requirements: player must either control the semaphore object or the object
 * must have Link_ok flag set. For custom attributes, player must have Set_attr permission.
 * Notifies player of "Notified." or "Drained." on success unless Quiet flag is set.
 *
 * @param player DBref of the player issuing the notification command
 * @param cause  DBref of the cause object (unused in this function)
 * @param key    Operation mode: NFY_NFY to execute commands, NFY_DRAIN to discard
 * @param what   Target specification: "object" or "object/attribute" string
 * @param count  String representation of notification count (parsed to integer)
 *
 * @note Thread-safe: No (calls nfy_que which modifies global queue state)
 * @note Count must be positive integer; invalid values result in error notification
 * @attention Requires controls or Link_ok permission on target object
 * @attention Custom attribute requires Set_attr permission
 *
 * @see nfy_que() for the underlying notification implementation
 * @see wait_que() for queuing commands on semaphores
 * @see do_wait() for the wait command that blocks on semaphores
 */
void do_notify(dbref player, dbref cause, int key, char *what, char *count)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int loccount = 0, attr = 0, aflags = 0;
	ATTR *ap = NULL;
	char *obj = parse_to(&what, '/', 0);

	init_match(player, obj, NOTYPE);
	match_everything(0);

	if ((thing = noisy_match_result()) < 0)
	{
		notify(player, "No match.");
	}
	else if (!controls(player, thing) && !Link_ok(thing))
	{
		notify(player, NOPERM_MESSAGE);
	}
	else
	{
		if (!what || !*what)
		{
			ap = NULL;
		}
		else
		{
			ap = atr_str(what);
		}

		if (!ap)
		{
			attr = A_SEMAPHORE;
		}
		else
		{
			/* Do they have permission to set this attribute? */
			atr_pget_info(thing, ap->number, &aowner, &aflags);

			if (Set_attr(player, thing, ap, aflags))
			{
				attr = ap->number;
			}
			else
			{
				notify_quiet(player, NOPERM_MESSAGE);
				return;
			}
		}

		if (count && *count)
		{
			char *endptr = NULL;
			long val = 0;

			errno = 0;
			val = strtol(count, &endptr, 10);

			if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == count || *endptr != '\0')
			{
				notify_quiet(player, "Invalid count value.");
				return;
			}

			loccount = (int)val;
		}
		else
		{
			loccount = 1;
		}

		if (loccount > 0)
		{
			nfy_que(player, thing, attr, key, loccount);

			if (!(Quiet(player) || Quiet(thing)))
			{
				if (key == NFY_DRAIN)
				{
					notify_quiet(player, "Drained.");
				}
				else
				{
					notify_quiet(player, "Notified.");
				}
			}
		}
	}
}

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
		if (qpid > mushconf.max_qpid)
		{
			qpid = 1;
		}

		if (nhashfind(qpid, &mushstate.qpid_htab) != NULL)
		{
			qpid++;
		}
		else
		{
			qpid_top = qpid + 1;
			return qpid;
		}
	}

	return 0;
}

/**
 * @brief Create and initialize a new queue entry with command, arguments, and registers.
 *
 * Constructs a fully initialized queue entry (BQUE) after performing comprehensive validation:
 * checks player halt status, verifies payment for queue cost, enforces queue quota limits,
 * allocates a unique PID, and carefully calculates total memory requirements to prevent
 * integer overflow. All data (command text, arguments, global registers, extended registers)
 * is copied into a single contiguous memory block for cache efficiency.
 *
 * Validation sequence:
 * 1. Check if player is halted (cannot queue commands)
 * 2. Charge waitcost (with occasional machinecost penalty)
 * 3. Verify queue quota not exceeded (triggers auto-halt if over limit)
 * 4. Allocate available PID from pool
 * 5. Calculate total memory size with overflow detection
 * 6. Allocate and populate queue entry structure
 *
 * Memory layout: Single allocation containing command string, nargs argument strings,
 * q_alloc global register contents, and xr_alloc extended register name/value pairs,
 * all null-terminated and referenced via pointers in the BQUE structure.
 *
 * @param player  DBref of the player queuing the command
 * @param cause   DBref of the object that caused this command (for attribution)
 * @param command Command text string to execute (may be NULL)
 * @param args    Array of environment variable strings (%0-%9) - may contain NULLs
 * @param nargs   Number of arguments in args array (clamped to NUM_ENV_VARS)
 * @param gargs   Global and extended register data (q-registers, x-registers) or NULL
 * @return Newly allocated and initialized queue entry, or NULL on failure
 *
 * @note Thread-safe: No (modifies global queue counters, hash table, charges player)
 * @note Caller must add returned entry to appropriate queue (via give_que or wait_que)
 * @note On failure (NULL return), player is notified with specific error message
 * @attention Player is charged waitcost immediately; refund required if entry not queued
 * @attention Returns NULL if player halted, insufficient funds, quota exceeded, or PID exhausted
 *
 * @see wait_que() for queuing entry with delay or semaphore
 * @see give_que() for immediate execution queue
 * @see qpid_next() for PID allocation
 * @see delete_qentry() for cleanup and deallocation
 */
BQUE *setup_que(dbref player, dbref cause, char *command, char *args[], int nargs, GDATA *gargs)
{
	int a = 0, tlen = 0, qpid = 0;
	BQUE *tmp = NULL;
	char *tptr = NULL;

	/* Can we run commands at all? */
	if (Halted(player))
	{
		return NULL;
	}

	/* make sure player can afford to do it */
	a = mushconf.waitcost;

	if (a && mushconf.machinecost && (random_range(0, (mushconf.machinecost) - 1) == 0))
	{
		a++;
	}

	if (!payfor(player, a))
	{
		notify(Owner(player), "Not enough money to queue command.");
		return NULL;
	}

	/* Wizards and their objs may queue up to db_top+1 cmds. Players are limited to QUEUE_QUOTA. -mnp */
	a = QueueMax(Owner(player));

	if (a_Queue(Owner(player), 1) > a)
	{
		notify(Owner(player), "Run away objects: too many commands queued.  Halted.");
		halt_que(Owner(player), NOTHING);
		/* halt also means no command execution allowed */
		s_Halted(player);
		return NULL;
	}

	/* Generate a PID */
	qpid = qpid_next();

	if (qpid == 0)
	{
		notify(Owner(player), "Could not queue command. The queue is full.");
		return NULL;
	}

	/* We passed all the tests, calculate the length of the save string. Check for integer overflow during accumulation. */
	tlen = 0;

	if (command)
	{
		size_t cmd_len = strlen(command) + 1;
		if (cmd_len > INT_MAX || tlen > INT_MAX - cmd_len)
		{
			notify(Owner(player), "Command too large to queue.");
			return NULL;
		}
		tlen += cmd_len;
	}

	if (nargs > NUM_ENV_VARS)
	{
		nargs = NUM_ENV_VARS;
	}
	else if (nargs < 0)
	{
		nargs = 0;
	}

	for (a = 0; a < nargs; a++)
	{
		if (args[a])
		{
			size_t arg_len = strlen(args[a]) + 1;
			if (arg_len > INT_MAX || tlen > INT_MAX - arg_len)
			{
				notify(Owner(player), "Arguments too large to queue.");
				return NULL;
			}
			tlen += arg_len;
		}
	}

	if (gargs)
	{
		for (a = 0; a < gargs->q_alloc; a++)
		{
			if (gargs->q_regs[a])
			{
				size_t reg_len = gargs->q_lens[a] + 1;
				if (reg_len > INT_MAX || tlen > INT_MAX - reg_len)
				{
					notify(Owner(player), "Global registers too large to queue.");
					return NULL;
				}
				tlen += reg_len;
			}
		}

		for (a = 0; a < gargs->xr_alloc; a++)
		{
			if (gargs->x_names[a] && gargs->x_regs[a])
			{
				size_t xr_len = strlen(gargs->x_names[a]) + gargs->x_lens[a] + 2;
				if (xr_len > INT_MAX || tlen > INT_MAX - xr_len)
				{
					notify(Owner(player), "Extended registers too large to queue.");
					return NULL;
				}
				tlen += xr_len;
			}
		}
	}

	/* Create the queue entry and load the save string */
	tmp = XMALLOC(sizeof(BQUE), "tmp");

	if (!(tptr = tmp->text = (char *)XMALLOC(tlen, "tmp->text")))
	{
		XFREE(tmp);
		return (BQUE *)NULL;
	}

	/* Set up registers and whatnot */
	tmp->comm = NULL;

	for (a = 0; a < NUM_ENV_VARS; a++)
	{
		tmp->env[a] = NULL;
	}

	if (gargs && (gargs->q_alloc || gargs->xr_alloc))
	{
		tmp->gdata = (GDATA *)XMALLOC(sizeof(GDATA), "setup_que");
		tmp->gdata->q_alloc = gargs->q_alloc;
		if (gargs->q_alloc)
		{
			tmp->gdata->q_regs = XCALLOC(gargs->q_alloc, sizeof(char *), "q_regs");
			tmp->gdata->q_lens = XCALLOC(gargs->q_alloc, sizeof(int), "q_lens");
		}
		else
		{
			tmp->gdata->q_regs = NULL;
			tmp->gdata->q_lens = NULL;
		}
		tmp->gdata->xr_alloc = gargs->xr_alloc;
		if (gargs->xr_alloc)
		{
			tmp->gdata->x_names = XCALLOC(gargs->xr_alloc, sizeof(char *), "x_names");
			tmp->gdata->x_regs = XCALLOC(gargs->xr_alloc, sizeof(char *), "x_regs");
			tmp->gdata->x_lens = XCALLOC(gargs->xr_alloc, sizeof(int), "x_lens");
		}
		else
		{
			tmp->gdata->x_names = NULL;
			tmp->gdata->x_regs = NULL;
			tmp->gdata->x_lens = NULL;
		}
		tmp->gdata->dirty = 0;
	}
	else
	{
		tmp->gdata = NULL;
	}

	if (command)
	{
		XSTRCPY(tptr, command);
		tmp->comm = tptr;
		tptr += (strlen(command) + 1);
	}

	for (a = 0; a < nargs; a++)
	{
		if (args[a])
		{
			XSTRCPY(tptr, args[a]);
			tmp->env[a] = tptr;
			tptr += (strlen(args[a]) + 1);
		}
	}

	if (gargs && gargs->q_alloc)
	{
		for (a = 0; a < gargs->q_alloc; a++)
		{
			if (gargs->q_regs[a])
			{
				tmp->gdata->q_lens[a] = gargs->q_lens[a];
				XMEMCPY(tptr, gargs->q_regs[a], gargs->q_lens[a] + 1);
				tmp->gdata->q_regs[a] = tptr;
				tptr += gargs->q_lens[a] + 1;
			}
		}
	}

	if (gargs && gargs->xr_alloc)
	{
		for (a = 0; a < gargs->xr_alloc; a++)
		{
			if (gargs->x_names[a] && gargs->x_regs[a])
			{
				XSTRCPY(tptr, gargs->x_names[a]);
				tmp->gdata->x_names[a] = tptr;
				tptr += strlen(gargs->x_names[a]) + 1;
				tmp->gdata->x_lens[a] = gargs->x_lens[a];
				XMEMCPY(tptr, gargs->x_regs[a], gargs->x_lens[a] + 1);
				tmp->gdata->x_regs[a] = tptr;
				tptr += gargs->x_lens[a] + 1;
			}
		}
	}

	/* Load the rest of the queue block */
	tmp->pid = qpid;
	nhashadd(qpid, (int *)tmp, &mushstate.qpid_htab);
	tmp->player = player;
	tmp->waittime = 0;
	tmp->next = NULL;
	tmp->sem = NOTHING;
	tmp->attr = 0;
	tmp->cause = cause;
	tmp->nargs = nargs;
	return tmp;
}

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
	BQUE *tmp = NULL, *point = NULL, *trail = NULL;

	if (mushconf.control_flags & CF_INTERP)
	{
		tmp = setup_que(player, cause, command, args, nargs, gargs);
	}
	else
	{
		tmp = NULL;
	}

	if (tmp == NULL)
	{
		return;
	}

	/* Set wait time, and check for integer overflow before the addition */
	if (wait != 0)
	{
		time_t now = time(NULL);
		/* Check for overflow before performing the addition */
		if (wait > 0 && (time_t)wait > INT_MAX - now)
		{
			tmp->waittime = INT_MAX;
		}
		else if (wait < 0 && (time_t)wait < INT_MIN - now)
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
			for (point = mushstate.qwait, trail = NULL; point && point->waittime <= tmp->waittime; point = point->next)
			{
				trail = point;
			}

			tmp->next = point;

			if (trail != NULL)
			{
				trail->next = tmp;
			}
			else
			{
				mushstate.qwait = tmp;
			}
		}
	}
	else
	{
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
	int qpid = 0, wsecs = 0;
	BQUE *qptr = NULL, *point = NULL, *trail = NULL;
	char *endptr = NULL;
	long val = 0;

	if (!is_integer(timestr))
	{
		notify(player, "That is not a valid wait time.");
		return;
	}

	if (!is_integer(pidstr))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	errno = 0;
	val = strtol(pidstr, &endptr, 10);

	if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == pidstr || *endptr != '\0')
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	qpid = (int)val;

	if ((qpid < 1) || (qpid > mushconf.max_qpid))
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

	if (key & WAIT_UNTIL)
	{
		char *endptr_time = NULL;
		long val_time = 0;

		errno = 0;
		val_time = strtol(timestr, &endptr_time, 10);

		if (errno == ERANGE || val_time > INT_MAX || val_time < INT_MIN || endptr_time == timestr || *endptr_time != '\0')
		{
			notify(player, "That is not a valid wait time.");
			return;
		}

		wsecs = (int)val_time;

		if (wsecs < 0)
		{
			qptr->waittime = time(NULL);
		}
		else
		{
			qptr->waittime = wsecs;
		}
	}
	else
	{
		char *endptr_time = NULL;
		long val_time = 0;

		errno = 0;
		val_time = strtol(timestr, &endptr_time, 10);

		if (errno == ERANGE || val_time > INT_MAX || val_time < INT_MIN || endptr_time == timestr || *endptr_time != '\0')
		{
			notify(player, "That is not a valid wait time.");
			return;
		}

		if ((timestr[0] == '+') || (timestr[0] == '-'))
		{
			time_t old_time = qptr->waittime;
			if ((val_time > 0 && old_time > INT_MAX - val_time) || (val_time < 0 && old_time < INT_MIN - val_time))
			{
				qptr->waittime = (val_time > 0) ? INT_MAX : INT_MIN;
			}
			else
			{
				qptr->waittime = old_time + (int)val_time;
			}
		}
		else
		{
			time_t now = time(NULL);
			if ((val_time > 0 && now > INT_MAX - val_time) || (val_time < 0 && now < INT_MIN - val_time))
			{
				qptr->waittime = (val_time > 0) ? INT_MAX : INT_MIN;
			}
			else
			{
				qptr->waittime = now + (int)val_time;
			}
		}

		if (qptr->waittime < 0)
		{
			if (timestr[0] == '-')
			{
				qptr->waittime = time(NULL);
			}
			else
			{
				qptr->waittime = INT_MAX;
			}
		}
	}

	/* The semaphore queue is unsorted, but the main wait queue is sorted. So we may have to go rethread. */
	if (qptr->sem == NOTHING)
	{
		remove_waitq(qptr);

		for (point = mushstate.qwait, trail = NULL; point && point->waittime <= qptr->waittime; point = point->next)
		{
			trail = point;
		}

		qptr->next = point;

		if (trail != NULL)
		{
			trail->next = qptr;
		}
		else
		{
			mushstate.qwait = qptr;
		}
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
	int howlong = 0, num = 0, attr = 0, aflags = 0;
	char *what = NULL;
	ATTR *ap = NULL;

	if (key & WAIT_PID)
	{
		do_wait_pid(player, key, event, cmd);
		return;
	}

	/* If arg1 is all numeric, do simple (non-sem) timed wait. */
	if (is_number(event))
	{
		char *endptr = NULL;
		long val = 0;

		errno = 0;
		val = strtol(event, &endptr, 10);

		if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == event || *endptr != '\0')
		{
			notify(player, "Invalid wait time.");
			return;
		}

		if (key & WAIT_UNTIL)
		{
			time_t now = time(NULL);
			if (val < (long)now)
			{
				howlong = 0;
			}
			else if (val - now > INT_MAX)
			{
				howlong = INT_MAX;
			}
			else
			{
				howlong = (int)(val - now);
			}
		}
		else
		{
			howlong = (int)val;
		}

		wait_que(player, cause, howlong, NOTHING, 0, cmd, cargs, ncargs, mushstate.rdata);
		return;
	}

	/* Semaphore wait with optional timeout */
	what = parse_to(&event, '/', 0);
	init_match(player, what, NOTYPE);
	match_everything(0);
	thing = noisy_match_result();

	if (!Good_obj(thing))
	{
		notify(player, "No match.");
	}
	else if (!controls(player, thing) && !Link_ok(thing))
	{
		notify(player, NOPERM_MESSAGE);
	}
	else
	{
		/* Get timeout, default 0 */
		if (event && *event && is_number(event))
		{
			char *endptr = NULL;
			long val = 0;

			attr = A_SEMAPHORE;

			errno = 0;
			val = strtol(event, &endptr, 10);

			if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == event || *endptr != '\0')
			{
				notify(player, "Invalid wait time.");
				return;
			}

			if (key & WAIT_UNTIL)
			{
				time_t now = time(NULL);
				if (val < (long)now)
				{
					howlong = 0;
				}
				else if (val - now > INT_MAX)
				{
					howlong = INT_MAX;
				}
				else
				{
					howlong = (int)(val - now);
				}
			}
			else
			{
				howlong = (int)val;
			}
		}
		else
		{
			attr = A_SEMAPHORE;
			howlong = 0;
		}

		if (event && *event && !is_number(event))
		{
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

			if (attr && Set_attr(player, thing, ap, aflags))
			{
				attr = ap->number;
				howlong = 0;
			}
			else
			{
				notify_quiet(player, NOPERM_MESSAGE);
				return;
			}
		}

		num = add_to(player, thing, 1, attr);

		if (num <= 0)
		{
			/* thing over-notified, run the command immediately */
			thing = NOTHING;
			howlong = 0;
		}

		wait_que(player, cause, howlong, thing, attr, cmd, cargs, ncargs, mushstate.rdata);
	}
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
	int min = 0, this = 0;
	BQUE *point = NULL;

	/* If there are commands in the player queue, we want to run them immediately. */
	if (test_top())
	{
		return 0;
	}

	/* If there are commands in the object queue, we want to run them after a one-second pause. */
	if (mushstate.qlfirst != NULL)
	{
		return 1;
	}

	/* Find minimum wait time among queued commands */
	min = 1000;

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

	for (point = mushstate.qsemfirst; point; point = point->next)
	{
		if (point->waittime == 0)
		{
			continue; /* Skip if no timeout */
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

	/* Move low-priority queue to end of normal queue, delaying object effects by 1 second */
	if ((mushconf.control_flags & CF_DEQUEUE) == 0)
	{
		return;
	}

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< do_second >";

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

	/* the point->waittime test would be 0 except the command is being put in the low priority queue to be done in one second anyway */
	while (mushstate.qwait && mushstate.qwait->waittime <= mushstate.now)
	{
		/* Do the wait queue, move the command to the normal queue */
		point = mushstate.qwait;
		mushstate.qwait = point->next;
		give_que(point);
	}

	/* Check the semaphore queue for expired timed-waits */
	for (point = mushstate.qsemfirst, trail = NULL; point; point = next)
	{
		if (point->waittime == 0)
		{
			next = (trail = point)->next;
			continue; /* Skip if not timed-wait */
		}

		if (point->waittime <= mushstate.now)
		{
			if (trail != NULL)
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

			add_to(point->player, point->sem, -1, (point->attr ? point->attr : A_SEMAPHORE));
			point->sem = NOTHING;
			give_que(point);
		}
		else
		{
			next = (trail = point)->next;
		}
	}

	mushstate.debug_cmd = cmdsave;
	return;
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

	if ((mushconf.control_flags & CF_DEQUEUE) == 0)
	{
		return 0;
	}

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< do_top >";

	for (count = 0; count < ncmds; count++)
	{
		if (!test_top())
		{
			mushstate.debug_cmd = cmdsave;

			if (mushstate.rdata)
			{
				for (int z = 0; z < mushstate.rdata->q_alloc; z++)
				{
					if (mushstate.rdata->q_regs[z])
						XFREE(mushstate.rdata->q_regs[z]);
				}

				for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
				{
					if (mushstate.rdata->x_names[z])
						XFREE(mushstate.rdata->x_names[z]);

					if (mushstate.rdata->x_regs[z])
						XFREE(mushstate.rdata->x_regs[z]);
				}

				if (mushstate.rdata->q_regs)
				{
					XFREE(mushstate.rdata->q_regs);
				}

				if (mushstate.rdata->q_lens)
				{
					XFREE(mushstate.rdata->q_lens);
				}

				if (mushstate.rdata->x_names)
				{
					XFREE(mushstate.rdata->x_names);
				}

				if (mushstate.rdata->x_regs)
				{
					XFREE(mushstate.rdata->x_regs);
				}

				if (mushstate.rdata->x_lens)
				{
					XFREE(mushstate.rdata->x_lens);
				}

				XFREE(mushstate.rdata);
			}

			mushstate.rdata = NULL;
			return count;
		}

		player = mushstate.qfirst->player;

		if ((player >= 0) && !Going(player))
		{
			giveto(player, mushconf.waitcost);
			mushstate.curr_enactor = mushstate.qfirst->cause;
			mushstate.curr_player = player;
			a_Queue(Owner(player), -1);
			mushstate.qfirst->player = NOTHING;

			if (!Halted(player))
			{
				/* Load scratch args */
				if (mushstate.qfirst->gdata)
				{
					if (mushstate.rdata)
					{
						for (int z = 0; z < mushstate.rdata->q_alloc; z++)
						{
							if (mushstate.rdata->q_regs[z])
								XFREE(mushstate.rdata->q_regs[z]);
						}
						for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
						{
							if (mushstate.rdata->x_names[z])
								XFREE(mushstate.rdata->x_names[z]);
							if (mushstate.rdata->x_regs[z])
								XFREE(mushstate.rdata->x_regs[z]);
						}

						if (mushstate.rdata->q_regs)
						{
							XFREE(mushstate.rdata->q_regs);
						}
						if (mushstate.rdata->q_lens)
						{
							XFREE(mushstate.rdata->q_lens);
						}
						if (mushstate.rdata->x_names)
						{
							XFREE(mushstate.rdata->x_names);
						}
						if (mushstate.rdata->x_regs)
						{
							XFREE(mushstate.rdata->x_regs);
						}
						if (mushstate.rdata->x_lens)
						{
							XFREE(mushstate.rdata->x_lens);
						}
						XFREE(mushstate.rdata);
					}

					if (mushstate.qfirst->gdata && (mushstate.qfirst->gdata->q_alloc || mushstate.qfirst->gdata->xr_alloc))
					{
						mushstate.rdata = (GDATA *)XMALLOC(sizeof(GDATA), "do_top");
						mushstate.rdata->q_alloc = mushstate.qfirst->gdata->q_alloc;
						if (mushstate.qfirst->gdata->q_alloc)
						{
							mushstate.rdata->q_regs = XCALLOC(mushstate.qfirst->gdata->q_alloc, sizeof(char *), "q_regs");
							mushstate.rdata->q_lens = XCALLOC(mushstate.qfirst->gdata->q_alloc, sizeof(int), "q_lens");
						}
						else
						{
							mushstate.rdata->q_regs = NULL;
							mushstate.rdata->q_lens = NULL;
						}
						mushstate.rdata->xr_alloc = mushstate.qfirst->gdata->xr_alloc;
						if (mushstate.qfirst->gdata->xr_alloc)
						{
							mushstate.rdata->x_names = XCALLOC(mushstate.qfirst->gdata->xr_alloc, sizeof(char *), "x_names");
							mushstate.rdata->x_regs = XCALLOC(mushstate.qfirst->gdata->xr_alloc, sizeof(char *), "x_regs");
							mushstate.rdata->x_lens = XCALLOC(mushstate.qfirst->gdata->xr_alloc, sizeof(int), "x_lens");
						}
						else
						{
							mushstate.rdata->x_names = NULL;
							mushstate.rdata->x_regs = NULL;
							mushstate.rdata->x_lens = NULL;
						}
						mushstate.rdata->dirty = 0;
					}
					else
					{
						mushstate.rdata = NULL;
					}

					if (mushstate.qfirst->gdata && mushstate.qfirst->gdata->q_alloc)
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

					if (mushstate.qfirst->gdata && mushstate.qfirst->gdata->xr_alloc)
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

					if (mushstate.qfirst->gdata)
					{
						mushstate.rdata->dirty = mushstate.qfirst->gdata->dirty;
					}
					else
					{
						mushstate.rdata->dirty = 0;
					}
				}
				else
				{
					if (mushstate.rdata)
					{
						for (int z = 0; z < mushstate.rdata->q_alloc; z++)
						{
							if (mushstate.rdata->q_regs[z])
								XFREE(mushstate.rdata->q_regs[z]);
						}
						for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
						{
							if (mushstate.rdata->x_names[z])
								XFREE(mushstate.rdata->x_names[z]);
							if (mushstate.rdata->x_regs[z])
								XFREE(mushstate.rdata->x_regs[z]);
						}

						if (mushstate.rdata->q_regs)
						{
							XFREE(mushstate.rdata->q_regs);
						}
						if (mushstate.rdata->q_lens)
						{
							XFREE(mushstate.rdata->q_lens);
						}
						if (mushstate.rdata->x_names)
						{
							XFREE(mushstate.rdata->x_names);
						}
						if (mushstate.rdata->x_regs)
						{
							XFREE(mushstate.rdata->x_regs);
						}
						if (mushstate.rdata->x_lens)
						{
							XFREE(mushstate.rdata->x_lens);
						}
						XFREE(mushstate.rdata);
					}
					mushstate.rdata = NULL;
				}

				mushstate.cmd_invk_ctr = 0;
				process_cmdline(player, mushstate.qfirst->cause, mushstate.qfirst->comm, mushstate.qfirst->env, mushstate.qfirst->nargs, mushstate.qfirst);
			}
		}

		if (mushstate.qfirst)
		{
			tmp = mushstate.qfirst;
			mushstate.qfirst = mushstate.qfirst->next;
			delete_qentry(tmp);
		}

		if (!mushstate.qfirst)
		{
			mushstate.qlast = NULL; /* gotta check this, as the value's * changed */
		}
	}

	if (mushstate.rdata)
	{
		for (int z = 0; z < mushstate.rdata->q_alloc; z++)
		{
			if (mushstate.rdata->q_regs[z])
				XFREE(mushstate.rdata->q_regs[z]);
		}

		for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
		{
			if (mushstate.rdata->x_names[z])
				XFREE(mushstate.rdata->x_names[z]);

			if (mushstate.rdata->x_regs[z])
				XFREE(mushstate.rdata->x_regs[z]);
		}

		if (mushstate.rdata->q_regs)
		{
			XFREE(mushstate.rdata->q_regs);
		}

		if (mushstate.rdata->q_lens)
		{
			XFREE(mushstate.rdata->q_lens);
		}

		if (mushstate.rdata->x_names)
		{
			XFREE(mushstate.rdata->x_names);
		}

		if (mushstate.rdata->x_regs)
		{
			XFREE(mushstate.rdata->x_regs);
		}

		if (mushstate.rdata->x_lens)
		{
			XFREE(mushstate.rdata->x_lens);
		}

		XFREE(mushstate.rdata);
	}

	mushstate.rdata = NULL;
	mushstate.debug_cmd = cmdsave;
	return count;
}

/**
 * @brief Display queue entries matching filter criteria with configurable detail level.
 *
 * Iterates through a queue (player, object, wait, or semaphore) and displays entries
 * matching player_targ/obj_targ filters. Supports three detail modes: summary (count only),
 * brief (one line per entry), and long (multi-line with arguments and enactor). Used by
 * do_ps() to implement the @ps command for queue inspection and monitoring.
 *
 * Display modes (key parameter):
 * - PS_SUMM: Count matching entries without displaying individual commands
 * - PS_BRIEF: Display one line per entry with PID, player, and command text
 * - PS_LONG: Display multi-line entries including arguments (%0-%9) and enactor
 *
 * Entry filtering: Only displays entries where que_want() returns true, filtering by:
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
 * @see do_ps() for command interface that calls this function
 * @see que_want() for entry filtering logic
 * @see unparse_object() for player/object name formatting
 */
void show_que(dbref player, int key, BQUE *queue, int *qtot, int *qent, int *qdel, dbref player_targ, dbref obj_targ, const char *header)
{
	BQUE *tmp = NULL;
	char *bp = NULL, *bufp = NULL;
	int i = 0;
	ATTR *ap = NULL;

	*qtot = 0;
	*qent = 0;
	*qdel = 0;

	for (tmp = queue; tmp; tmp = tmp->next)
	{
		(*qtot)++;

		if (que_want(tmp, player_targ, obj_targ))
		{
			(*qent)++;

			if (key == PS_SUMM)
			{
				continue;
			}

			if (*qent == 1)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "----- %s Queue -----", header);
			}

			bufp = unparse_object(player, tmp->player, 0);

			if ((tmp->waittime > 0) && (Good_obj(tmp->sem)))
			{
				/* A minor shortcut. We can never timeout-wait on a non-Semaphore attribute. */
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[#%d/%d] %d:%s:%s", tmp->sem, tmp->waittime - mushstate.now, tmp->pid, bufp, tmp->comm);
			}
			else if (tmp->waittime > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%d] %d:%s:%s", tmp->waittime - mushstate.now, tmp->pid, bufp, tmp->comm);
			}
			else if (Good_obj(tmp->sem))
			{
				if (tmp->attr == A_SEMAPHORE)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[#%d] %d:%s:%s", tmp->sem, tmp->pid, bufp, tmp->comm);
				}
				else
				{
					ap = atr_num(tmp->attr);

					if (ap && ap->name)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[#%d/%s] %d:%s:%s", tmp->sem, ap->name, tmp->pid, bufp, tmp->comm);
					}
					else
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[#%d] %d:%s:%s", tmp->sem, tmp->pid, bufp, tmp->comm);
					}
				}
			}
			else
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d:%s:%s", tmp->pid, bufp, tmp->comm);
			}

			bp = bufp;

			if (key == PS_LONG)
			{
				for (i = 0; i < (tmp->nargs); i++)
				{
					if (tmp->env[i] != NULL)
					{
						XSAFELBSTR((char *)"; Arg", bufp, &bp);
						XSAFELBCHR(i + '0', bufp, &bp);
						XSAFELBSTR((char *)"='", bufp, &bp);
						XSAFELBSTR(tmp->env[i], bufp, &bp);
						XSAFELBCHR('\'', bufp, &bp);
					}
				}

				*bp = '\0';
				bp = unparse_object(player, tmp->cause, 0);
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "   Enactor: %s%s", bp, bufp);
				XFREE(bp);
			}

			XFREE(bufp);
		}
		else if (tmp->player == NOTHING)
		{
			(*qdel)++;
		}
	}

	return;
}

/**
 * @brief Command interface for displaying queue status and entries (@ps command).
 *
 * Implements the @ps command that displays pending commands across all four queue types
 * (player, object, wait, semaphore) with filtering by player/object ownership. Supports
 * three detail levels (brief, summary, long) and optional "all queues" mode for wizards.
 * Delegates to show_que() for each queue type, then displays aggregate statistics.
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
 * Output format: Calls show_que() four times (player, object, wait, semaphore queues),
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
 * @note Thread-safe: Yes (read-only operation via show_que() calls)
 * @note PS_ALL stripped from key before switch validation to allow combining with detail modes
 * @note Deleted entry counts (qdel) only collected/displayed for player and object queues
 * @note Wait and semaphore queues do not track deleted entries in summary statistics
 * @attention Requires See_Queue permission for PS_ALL flag or viewing other players' queues
 * @attention Target + PS_ALL combination is explicitly forbidden and returns error
 *
 * @see show_que() for individual queue display implementation
 * @see que_want() for entry filtering logic
 * @see match_controlled() for target resolution with permission checks
 */
void do_ps(dbref player, dbref cause, int key, char *target)
{
	char *bufp = NULL;
	dbref player_targ = NOTHING, obj_targ = NOTHING;
	int pqent = 0, pqtot = 0, pqdel = 0, oqent = 0, oqtot = 0, oqdel = 0, wqent = 0, wqtot = 0, sqent = 0, sqtot = 0, i = 0;

	/* Figure out what to list the queue for */
	if ((key & PS_ALL) && !(See_Queue(player)))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	if (!target || !*target)
	{
		obj_targ = NOTHING;

		if (key & PS_ALL)
		{
			player_targ = NOTHING;
		}
		else
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
		player_targ = Owner(player);

		if (See_Queue(player))
		{
			obj_targ = match_thing(player, target);
		}
		else
		{
			obj_targ = match_controlled(player, target);
		}

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

	key = key & ~PS_ALL;

	switch (key)
	{
	case PS_BRIEF:
	case PS_SUMM:
	case PS_LONG:
		break;

	default:
		notify(player, "Illegal combination of switches.");
		return;
	}

	/* Go do it */
	show_que(player, key, mushstate.qfirst, &pqtot, &pqent, &pqdel, player_targ, obj_targ, "Player");
	show_que(player, key, mushstate.qlfirst, &oqtot, &oqent, &oqdel, player_targ, obj_targ, "Object");
	show_que(player, key, mushstate.qwait, &wqtot, &wqent, &i, player_targ, obj_targ, "Wait");
	show_que(player, key, mushstate.qsemfirst, &sqtot, &sqent, &i, player_targ, obj_targ, "Semaphore");

	/* Display stats */
	bufp = XMALLOC(MBUF_SIZE, "bufp");

	if (See_Queue(player))
		XSPRINTF(bufp, "Totals: Player...%d/%d[%ddel]  Object...%d/%d[%ddel]  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent, sqtot);
	else
		XSPRINTF(bufp, "Totals: Player...%d/%d  Object...%d/%d  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, oqent, oqtot, wqent, wqtot, sqent, sqtot);

	notify(player, bufp);
	XFREE(bufp);
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
 *    - Calls do_top(i) to process i commands from mushstate.qfirst
 *    - Returns count of commands actually executed (may be less than requested)
 *    - Useful for draining queue during high load or testing command processing
 *    - Temporarily enables CF_DEQUEUE if disabled (warns player)
 *
 * 2. QUEUE_WARP: Adjust wait times by time offset (positive = advance, negative = rewind)
 *    - Modifies all wait queue entries: sets waittime = -i (forces immediate execution)
 *    - Modifies semaphore timeouts: decrements waittime by i (clamps negative to -1)
 *    - Calls do_second() to process newly-expired entries
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
 * @see do_top() for command execution implementation used by QUEUE_KICK
 * @see do_second() for wait queue processing triggered by QUEUE_WARP
 * @see mushconf.control_flags for CF_DEQUEUE flag controlling automatic processing
 */
void do_queue(dbref player, dbref cause, int key, char *arg)
{
	BQUE *point = NULL;
	int i = 0, ncmds = 0, was_disabled = 0;

	if (key == QUEUE_KICK)
	{
		char *endptr = NULL;
		long val = 0;

		errno = 0;
		val = strtol(arg, &endptr, 10);

		if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == arg || *endptr != '\0')
		{
			notify(player, "Invalid number of commands.");
			return;
		}

		i = (int)val;

		if ((mushconf.control_flags & CF_DEQUEUE) == 0)
		{
			was_disabled = 1;
			mushconf.control_flags |= CF_DEQUEUE;
			notify(player, "Warning: automatic dequeueing is disabled.");
		}

		ncmds = do_top(i);

		if (was_disabled)
		{
			mushconf.control_flags &= ~CF_DEQUEUE;
		}

		if (!Quiet(player))
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d commands processed.", ncmds);
		}
	}
	else if (key == QUEUE_WARP)
	{
		char *endptr = NULL;
		long val = 0;

		errno = 0;
		val = strtol(arg, &endptr, 10);

		if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == arg || *endptr != '\0')
		{
			notify(player, "Invalid time value.");
			return;
		}

		i = (int)val;

		if ((mushconf.control_flags & CF_DEQUEUE) == 0)
		{
			was_disabled = 1;
			mushconf.control_flags |= CF_DEQUEUE;
			notify(player, "Warning: automatic dequeueing is disabled.");
		}

		/* Handle the wait queue */
		for (point = mushstate.qwait; point; point = point->next)
		{
			point->waittime = -i;
		}

		/* Handle the semaphore queue */
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

		do_second();

		if (was_disabled)
		{
			mushconf.control_flags &= ~CF_DEQUEUE;
		}

		if (Quiet(player))
		{
			return;
		}

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
