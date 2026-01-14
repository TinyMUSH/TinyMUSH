/**
 * @file cque_halt.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue halting and removal operations
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
extern void s_Queue(dbref, int);
extern bool que_want(BQUE *, dbref, dbref);
extern int add_to(dbref, dbref, int, int);

/**
 * @brief Remove all queued commands from a certain player
 *
 * @param player DBref of player
 * @param object DBref of object
 * @return int
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

	/**
	 * Player queue
	 *
	 */
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

	/**
	 * Object queue
	 *
	 */
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

	/**
	 * Wait queue
	 *
	 */
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

	/**
	 * Semaphore queue
	 *
	 */
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
 * @brief Remove an entry from the wait queue.
 *
 * @param qptr Queue Entry
 */
void remove_waitq(BQUE *qptr)
{
	BQUE *point = NULL, *trail = NULL;

	if (qptr == mushstate.qwait)
	{
		/**
		 * Head of the queue. Just remove it and relink.
		 *
		 */
		mushstate.qwait = qptr->next;
	}
	else
	{
		/**
		 * Go find it somewhere in the queue and take it out.
		 *
		 */
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
 * @brief Halt a single queue entry.
 *
 * @param player DBref of player
 * @param cause  DBref of cause
 * @param key    Key
 * @param pidstr Pîd of the queue
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

	/**
	 * Changing the player to NOTHING will flag this as halted, but we
	 * may have to delete it from the wait queue as well as the semaphore
	 * queue.
	 *
	 */
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
 * @brief Command interface to halt_que.
 *
 * @param player DBref of player
 * @param cause  DBref of cause
 * @param key 	 Key
 * @param target Target to halt
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

	/**
	 * Figure out what to halt
	 *
	 */
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
