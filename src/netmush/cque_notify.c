/**
 * @file cque_notify.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue notification and wait time adjustment operations
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

extern int a_Queue(dbref, int);
extern int add_to(dbref, dbref, int, int);
extern void give_que(BQUE *);
extern void remove_waitq(BQUE *);

/**
 * @brief Notify commands from the queue and perform or discard them.
 *
 * @param player DBref of player
 * @param sem    DBref of semaphore object
 * @param attr   Attribute number (0 for default A_SEMAPHORE)
 * @param key    Operation mode (NFY_NFY to notify, NFY_DRAIN to drain)
 * @param count  Number of entries to notify
 * @return int   Number of entries actually notified
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

				/**
				 * Either run or discard the command
				 *
				 */
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
			/**
			 * If we've notified enough, exit
			 *
			 */

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

	/**
	 * Update the sem waiters count
	 *
	 */
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
 * @brief Command interface to nfy_que
 *
 * @param player DBref of player
 * @param cause  DBref of cause
 * @param key    Key
 * @param what   Command
 * @param count  Counter
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
			/**
			 * Do they have permission to set this attribute?
			 *
			 */
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
 * @brief Adjust the wait time on an existing entry.
 *
 * @param player  DBref of player
 * @param key     Mode flags (WAIT_UNTIL for absolute time)
 * @param pidstr  PID of the queue entry to adjust
 * @param timestr Wait time (seconds or absolute time if WAIT_UNTIL)
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

	/**
	 * The semaphore queue is unsorted, but the main wait queue is
	 * sorted. So we may have to go rethread.
	 *
	 */
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
