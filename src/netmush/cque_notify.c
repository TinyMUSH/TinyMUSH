/**
 * @file cque_notify.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Semaphore notification and queue release operations
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
	int attrnum = attr ? attr : A_SEMAPHORE;

	/* Get semaphore counter from attribute */
	if (attr)
	{
		str = atr_get(sem, attr, &aowner, &aflags, &alen);

		errno = 0;
		val = strtol(str ? str : "0", &endptr, 10);

		if (errno != ERANGE && val >= INT_MIN && val <= INT_MAX && endptr != str && (str && (*endptr == '\0' || !*str)))
		{
			num = (int)val;
		}

		XFREE(str);
	}
	else
	{
		num = 1;
	}

	/* Process matching semaphore entries */
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
				{
					mushstate.qsemfirst = next = point->next;
				}

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

				/* If we've notified enough, exit */
				if ((key == NFY_NFY) && (num >= count))
				{
					next = NULL;
				}
			}
			else
			{
				next = (trail = point)->next;
			}
		}
	}

	/* Update the semaphore waiters count */
	if (key == NFY_NFY)
	{
		add_to(player, sem, -count, attrnum);
	}
	else
	{
		atr_clr(sem, attrnum);
	}

	return num;
}

/**
 * @brief Parse and validate a count string into an integer value.
 *
 * Validates the count string format and range, ensuring it represents a valid
 * positive integer count for semaphore notifications or drain operations.
 *
 * @param countstr String representation of the count
 * @param cnt Pointer to store the parsed count value (only valid if returns true)
 * @return true if countstr is valid and parsed successfully, false otherwise
 *
 * @note Thread-safe: Yes (read-only, no side effects beyond output parameter)
 * @note Valid count range: [1, INT_MAX]
 * @attention cnt pointer must be valid when calling this function
 */
static bool _cque_parse_count_string(const char *countstr, int *cnt)
{
	char *endptr = NULL;
	long val = 0;

	errno = 0;
	val = strtol((char *)countstr, &endptr, 10);

	if (errno == ERANGE || val > INT_MAX || val < INT_MIN || endptr == countstr || *endptr != '\0')
	{
		return false;
	}

	*cnt = (int)val;
	return true;
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
		return;
	}

	if (!controls(player, thing) && !Link_ok(thing))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/* Resolve semaphore attribute */
	ap = (what && *what) ? atr_str(what) : NULL;

	if (ap)
	{
		/* Do they have permission to set this attribute? */
		atr_pget_info(thing, ap->number, &aowner, &aflags);

		if (!Set_attr(player, thing, ap, aflags))
		{
			notify_quiet(player, NOPERM_MESSAGE);
			return;
		}

		attr = ap->number;
	}
	else
	{
		attr = A_SEMAPHORE;
	}

	/* Parse notification count */
	if (count && *count)
	{
		if (!_cque_parse_count_string(count, &loccount))
		{
			notify_quiet(player, "Invalid count value.");
			return;
		}
	}
	else
	{
		loccount = 1;
	}

	/* Process semaphore queue if count is positive */
	if (loccount > 0)
	{
		nfy_que(player, thing, attr, key, loccount);

		if (!(Quiet(player) || Quiet(thing)))
		{
			notify_quiet(player, (key == NFY_DRAIN) ? "Drained." : "Notified.");
		}
	}
}
