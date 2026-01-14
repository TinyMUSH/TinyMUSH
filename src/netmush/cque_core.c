/**
 * @file cque_core.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core queue utilities and helper functions
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

extern int a_Queue(dbref, int);
extern void s_Queue(dbref, int);
extern int QueueMax(dbref);

int qpid_top = 1;

/**
 * @brief Delete and free a queue entry.
 *
 * @param qptr Queue entry
 */
void delete_qentry(BQUE *qptr)
{
	nhashdelete(qptr->pid, &mushstate.qpid_htab);

	XFREE(qptr->text);

	if (qptr->gdata)
	{
		if ((qptr->gdata)->q_regs)
		{
			XFREE((qptr->gdata)->q_regs);
		}
		if ((qptr->gdata)->q_lens)
		{
			XFREE((qptr->gdata)->q_lens);
		}
		if ((qptr->gdata)->x_names)
		{
			XFREE((qptr->gdata)->x_names);
		}
		if ((qptr->gdata)->x_regs)
		{
			XFREE((qptr->gdata)->x_regs);
		}
		if ((qptr->gdata)->x_lens)
		{
			XFREE((qptr->gdata)->x_lens);
		}
		XFREE(qptr->gdata);
	}

	XFREE(qptr);
}

/**
 * @brief Adjust an object's queue or semaphore count.
 *
 * @param doer		Dbref of doer
 * @param player	DBref of player
 * @param am		Attribute map
 * @param attrnum	Attribute number
 * @return int
 */
int add_to(dbref doer, dbref player, int am, int attrnum)
{
	long val = 0;
	int num = 0, aflags = 0, alen = 0;
	dbref aowner = NOTHING;
	char *buff = NULL;
	char *atr_gotten = NULL;
	char *endptr = NULL;

	/**
	 * Get attribute value and parse it safely
	 *
	 */
	atr_gotten = atr_get(player, attrnum, &aowner, &aflags, &alen);

	errno = 0;
	val = strtol(atr_gotten, &endptr, 10);

	/**
	 * Validate conversion and range
	 *
	 */
	if (errno == ERANGE || val > INT_MAX || val < INT_MIN || (*endptr != '\0' && !isspace(*endptr)))
	{
		num = 0;
	}
	else
	{
		num = (int)val;
	}

	XFREE(atr_gotten);
	num += am;

	if (num)
	{
		buff = ltos(num);
		atr_add(player, attrnum, buff, Owner(doer), aflags);
		XFREE(buff);
	}
	else
	{
		atr_add(player, attrnum, NULL, Owner(doer), aflags);
	}

	return (num);
}

/**
 * @brief Do we want this queue entry?
 *
 * @param entry Queue Entry
 * @param ptarg Player target
 * @param otarg	Object target
 * @return bool
 */
bool que_want(BQUE *entry, dbref ptarg, dbref otarg)
{
	if (!Good_obj(entry->player))
	{
		return false;
	}

	if ((ptarg != NOTHING) && (ptarg != Owner(entry->player)))
	{
		return false;
	}

	if ((otarg != NOTHING) && (otarg != entry->player))
	{
		return false;
	}

	return true;
}

/**
 * @brief Get the next available queue PID.
 *
 * @return int
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
