/**
 * @file cque_management.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Queue entry setup, threading, and wait queue management
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
extern int QueueMax(dbref);
extern int qpid_next(void);

/**
 * @brief Thread a queue block onto the high or low priority queue
 *
 * @param tmp Queue Entry
 */
void give_que(BQUE *tmp)
{
	tmp->next = NULL;
	tmp->waittime = 0;

	/*
	 * Thread the command into the correct queue
	 */

	if (Typeof(tmp->cause) == TYPE_PLAYER)
	{
		if (mushstate.qlast != NULL)
		{
			mushstate.qlast->next = tmp;
			mushstate.qlast = tmp;
		}
		else
		{
			mushstate.qlast = mushstate.qfirst = tmp;
		}
	}
	else
	{
		if (mushstate.qllast)
		{
			mushstate.qllast->next = tmp;
			mushstate.qllast = tmp;
		}
		else
		{
			mushstate.qllast = mushstate.qlfirst = tmp;
		}
	}
}

/**
 * @brief Set up a queue entry.
 *
 * @param player  DBref of player
 * @param cause   DBref of cause
 * @param command Command
 * @param args    Arguments
 * @param nargs   Number of arguments
 * @param gargs   Global registers
 * @return BQUE*
 */
BQUE *setup_que(dbref player, dbref cause, char *command, char *args[], int nargs, GDATA *gargs)
{
	int a = 0, tlen = 0, qpid = 0;
	BQUE *tmp = NULL;
	char *tptr = NULL;

	/**
	 * Can we run commands at all?
	 *
	 */
	if (Halted(player))
	{
		return NULL;
	}

	/**
	 * make sure player can afford to do it
	 *
	 */
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

	/**
	 * Wizards and their objs may queue up to db_top+1 cmds. Players are
	 * limited to QUEUE_QUOTA. -mnp
	 *
	 */
	a = QueueMax(Owner(player));

	if (a_Queue(Owner(player), 1) > a)
	{
		notify(Owner(player), "Run away objects: too many commands queued.  Halted.");
		halt_que(Owner(player), NOTHING);
		/**
		 * halt also means no command execution allowed
		 *
		 */
		s_Halted(player);
		return NULL;
	}

	/**
	 * Generate a PID
	 *
	 */
	qpid = qpid_next();

	if (qpid == 0)
	{
		notify(Owner(player), "Could not queue command. The queue is full.");
		return NULL;
	}

	/**
	 * We passed all the tests, calculate the length of the save string.
	 * Check for integer overflow during accumulation.
	 *
	 */
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

	/**
	 * Create the queue entry and load the save string
	 *
	 */
	tmp = XMALLOC(sizeof(BQUE), "tmp");

	if (!(tptr = tmp->text = (char *)XMALLOC(tlen, "tmp->text")))
	{
		XFREE(tmp);
		return (BQUE *)NULL;
	}

	/**
	 * Set up registers and whatnot
	 *
	 */
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

	/**
	 * Load the rest of the queue block
	 *
	 */
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
 * @brief Add commands to the wait or semaphore queues.
 *
 * @param player  DBref of player
 * @param cause   DBref of cause
 * @param wait    Delay
 * @param sem     Semaphores
 * @param attr    Attributes
 * @param command Command
 * @param args    Arguments
 * @param nargs   Number of arguments
 * @param gargs   Global registers
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

	/**
	 * Set wait time, and check for integer overflow before the addition
	 *
	 */
	if (wait != 0)
	{
		time_t now = time(NULL);
		/**
		 * Check for overflow before performing the addition
		 */
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
		/**
		 * No semaphore, put on wait queue if wait value specified.
		 * Otherwise put on the normal queue.
		 *
		 */
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
