/**
 * @file cque_execution.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue execution, monitoring, and command interface operations
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
extern bool que_want(BQUE *, dbref, dbref);
extern int add_to(dbref, dbref, int, int);
extern void give_que(BQUE *);
extern void do_wait_pid(dbref, int, char *, char *);

/**
 * @brief Return the time in seconds until the next command should be run from the queue.
 *
 * @return int
 */
int que_next(void)
{
	int min = 0, this = 0;
	BQUE *point = NULL;

	/**
	 * If there are commands in the player queue, we want to run them
	 * immediately.
	 *
	 */
	if (test_top())
	{
		return 0;
	}

	/**
	 * If there are commands in the object queue, we want to run them
	 * after a one-second pause.
	 *
	 */
	if (mushstate.qlfirst != NULL)
	{
		return 1;
	}

	/*
	 * Walk the wait and semaphore queues, looking for the smallest wait
	 * value.  Return the smallest value - 1, because the command gets
	 * moved to the player queue when it has 1 second to go.
	 *
	 */
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
			continue; /*!< Skip if no timeout */
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
 * @brief Check the wait and semaphore queues for commands to remove.
 *
 */
void do_second(void)
{
	BQUE *trail = NULL, *point = NULL, *next = NULL;
	char *cmdsave = NULL;

	/**
	 * move contents of low priority queue onto end of normal one this
	 * helps to keep objects from getting out of control since its
	 * affects on other objects happen only after one second  this should
	* allow \@halt to be type before getting blown away  by scrolling text
	 *
	 */
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

	/**
	 * the point->waittime test would be 0 except the command is
	 * being put in the low priority queue to be done in one second
	 * anyway
	 *
	 */
	while (mushstate.qwait && mushstate.qwait->waittime <= mushstate.now)
	{
		/**
		 * Do the wait queue
		 *
		 */
		point = mushstate.qwait;
		mushstate.qwait = point->next;
		give_que(point);
	}

	/**
	 * Check the semaphore queue for expired timed-waits
	 *
	 */
	for (point = mushstate.qsemfirst, trail = NULL; point; point = next)
	{
		if (point->waittime == 0)
		{
			next = (trail = point)->next;
			continue; /*!< Skip if not timed-wait */
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
 * @brief Execute the command at the top of the queue
 *
 * @param ncmds Number of commands to execute
 * @return int
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
				/**
				 * Load scratch args
				 *
				 */
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
			mushstate.qlast = NULL; /*!< gotta check this, as the value's * changed */
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
 * @brief tell player what commands they have pending in the queue
 *
 * @param player      DBref of player
 * @param key         Key
 * @param queue       Queue Entry
 * @param qtot        Queue Total
 * @param qent        Queue Entries
 * @param qdel        Queue Deleted
 * @param player_targ DBref of player target
 * @param obj_targ    DBref of object target
 * @param header      Header
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
				/**
				 * A minor shortcut. We can never
				 * timeout-wait on a non-Semaphore attribute.
				 *
				 */
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
 * @brief List queue
 *
 * @param player DBref of player
 * @param cause  DBref of cause
 * @param key    Key
 * @param target Target
 */
void do_ps(dbref player, dbref cause, int key, char *target)
{
	char *bufp = NULL;
	dbref player_targ = NOTHING, obj_targ = NOTHING;
	int pqent = 0, pqtot = 0, pqdel = 0, oqent = 0, oqtot = 0, oqdel = 0, wqent = 0, wqtot = 0, sqent = 0, sqtot = 0, i = 0;

	/**
	 * Figure out what to list the queue for
	 *
	 */
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

	/**
	 * Go do it
	 *
	 */
	show_que(player, key, mushstate.qfirst, &pqtot, &pqent, &pqdel, player_targ, obj_targ, "Player");
	show_que(player, key, mushstate.qlfirst, &oqtot, &oqent, &oqdel, player_targ, obj_targ, "Object");
	show_que(player, key, mushstate.qwait, &wqtot, &wqent, &i, player_targ, obj_targ, "Wait");
	show_que(player, key, mushstate.qsemfirst, &sqtot, &sqent, &i, player_targ, obj_targ, "Semaphore");
	/**
	 * Display stats
	 *
	 */
	bufp = XMALLOC(MBUF_SIZE, "bufp");

	if (See_Queue(player))
		XSPRINTF(bufp, "Totals: Player...%d/%d[%ddel]  Object...%d/%d[%ddel]  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent, sqtot);
	else
		XSPRINTF(bufp, "Totals: Player...%d/%d  Object...%d/%d  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, oqent, oqtot, wqent, wqtot, sqent, sqtot);

	notify(player, bufp);
	XFREE(bufp);
}

/**
 * @brief Queue management
 *
 * @param player DBref of player
 * @param cause  DBref of cause
 * @param key    Key
 * @param arg    Arguments
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

		/**
		 * Handle the wait queue
		 *
		 */
		for (point = mushstate.qwait; point; point = point->next)
		{
			point->waittime = -i;
		}

		/**
		 * Handle the semaphore queue
		 *
		 */
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

/**
 * @brief Command interface to wait_que
 *
 * @param player DBref of player
 * @param cause  DBref of cause
 * @param key    Key
 * @param event  Event
 * @param cmd    Command
 * @param cargs  Command arguments
 * @param ncargs Number of commands
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

	/**
	 * If arg1 is all numeric, do simple (non-sem) timed wait.
	 *
	 */
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

	/**
	 * Semaphore wait with optional timeout
	 *
	 */
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
		/**
		 * Get timeout, default 0
		 *
		 */
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
			/**
			 * thing over-notified, run the command immediately
			 *
			 */
			thing = NOTHING;
			howlong = 0;
		}

		wait_que(player, cause, howlong, thing, attr, cmd, cargs, ncargs, mushstate.rdata);
	}
}
