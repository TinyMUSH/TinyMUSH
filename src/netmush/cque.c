/**
 * @file cque.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command queue scheduling, throttling, and execution utilities (main module)
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * This module provides the main setup_que() function for queue entry creation and
 * routes to specialized queue management modules for halt, notify, wait, execution,
 * and administrative commands.
 *
 * Queue modules:
 * - cque_entry.c: Queue entry lifecycle management
 * - cque_halt.c: Halt and kill command handlers
 * - cque_notify.c: Semaphore notification and release
 * - cque_wait.c: Wait and timed delay commands
 * - cque_exec.c: Command execution and scheduling engine
 * - cque_queue.c: Queue inspection and administration commands
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

/* ==================== Queue Entry Setup ==================== */

/**
 * @brief Create and initialize a queue entry for command execution.
 *
 * Allocates and initializes a queue entry (BQUE structure) with command text, environment
 * variables, and optional global register context. Performs comprehensive validation:
 * checks player is not halted, deducts wait cost, enforces queue quotas, allocates PID,
 * validates memory requirements, and handles all string allocation in a single contiguous
 * buffer (tptr).
 *
 * Register context is copied into the queue entry to preserve state across command
 * execution boundaries. Both q-registers (0-9) and x-registers (named) are supported.
 * Memory overflow protection prevents integer wraparound during accumulation.
 *
 * Resource management:
 * - Deducts mushconf.waitcost from player's money
 * - Allocates queue entry structure
 * - Allocates single text buffer for all strings (command, args, registers)
 * - Increments queue counter via a_Queue(Owner(player), 1)
 * - Assigns unique PID via qpid_next()
 * - Registers entry in qpid hash table
 *
 * Quota enforcement: Wizards and their objects may queue up to db_top+1 commands.
 * Players limited to QUEUE_QUOTA. If exceeded, player is halted and command is rejected.
 *
 * @param player DBref of player owning the command
 * @param cause  DBref of object that triggered the command (for attribution)
 * @param command Command text to queue (may be NULL)
 * @param args   Array of environment variable strings (%0-%9) - may contain NULLs
 * @param nargs  Number of arguments in args array (clamped to NUM_ENV_VARS)
 * @param gargs  Global/extended register data to preserve (may be NULL)
 * @return Allocated and initialized queue entry, or NULL on failure
 *
 * @note Thread-safe: No (modifies global state: player money, queue counter, PID table)
 * @note Player must not be halted or command fails silently
 * @note Insufficient funds results in notification to player's owner
 * @note Queue quota exceeded results in notification and player being halted
 * @note PID exhaustion results in notification and failure
 * @note All arguments are clamped to prevent overflow (nargs, string lengths)
 * @attention Caller must use give_que(), wait_que(), or nfy_que() to route entry to queue
 * @attention Caller must release entry via delete_qentry() on failure or cleanup
 * @attention Memory buffer is single allocation - pointers are internal offsets
 *
 * @see give_que() for normal queue insertion
 * @see wait_que() for wait queue insertion (timed or semaphore)
 * @see qpid_next() for PID allocation
 * @see delete_qentry() for cleanup on failure
 * @see halt_que() for quota enforcement halt
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
		tmp->gdata = (GDATA *)XCALLOC(1, sizeof(GDATA), "setup_que");
		tmp->gdata->q_alloc = gargs->q_alloc;
		if (gargs->q_alloc)
		{
			tmp->gdata->q_regs = XCALLOC(gargs->q_alloc, sizeof(char *), "q_regs");
			tmp->gdata->q_lens = XCALLOC(gargs->q_alloc, sizeof(int), "q_lens");
		}
		tmp->gdata->xr_alloc = gargs->xr_alloc;
		if (gargs->xr_alloc)
		{
			tmp->gdata->x_names = XCALLOC(gargs->xr_alloc, sizeof(char *), "x_names");
			tmp->gdata->x_regs = XCALLOC(gargs->xr_alloc, sizeof(char *), "x_regs");
			tmp->gdata->x_lens = XCALLOC(gargs->xr_alloc, sizeof(int), "x_lens");
		}
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
