/**
 * @file cque_entry.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue entry management and lifecycle operations
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
	BQUE **pptr = &mushstate.qwait;

	while (*pptr && *pptr != qptr)
	{
		pptr = &((*pptr)->next);
	}

	if (*pptr)
	{
		*pptr = qptr->next;
	}
}

/**
 * @brief Clean up and free global register data (GDATA) structure.
 *
 * Frees all allocated memory in a GDATA structure including q-registers, x-registers,
 * and their associated length arrays. Handles NULL pointers gracefully at all levels.
 * Used during queue command execution to manage global register context cleanup.
 *
 * @param gdata Pointer to GDATA structure to free (may be NULL)
 *
 * @note Thread-safe: Yes (no side effects beyond deallocation)
 * @note Sets gdata pointer to NULL after cleanup (caller responsibility)
 * @attention Does not set input pointer to NULL - caller must handle
 */
void _cque_free_gdata(GDATA *gdata)
{
	if (!gdata)
	{
		return;
	}

	/* Free q-register contents */
	for (int z = 0; z < gdata->q_alloc; z++)
	{
		if (gdata->q_regs[z])
		{
			XFREE(gdata->q_regs[z]);
		}
	}

	/* Free x-register names and contents */
	for (int z = 0; z < gdata->xr_alloc; z++)
	{
		if (gdata->x_names[z])
		{
			XFREE(gdata->x_names[z]);
		}

		if (gdata->x_regs[z])
		{
			XFREE(gdata->x_regs[z]);
		}
	}

	/* Free register arrays */
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
