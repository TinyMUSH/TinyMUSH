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

	/* Get attribute value and parse it safely */
	atr_gotten = atr_get(player, attrnum, &aowner, &aflags, &alen);

	errno = 0;
	val = strtol(atr_gotten, &endptr, 10);

	/* Validate conversion and range */
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
 * @brief Thread a queue block onto the high or low priority queue
 *
 * @param tmp Queue Entry
 */
void give_que(BQUE *tmp)
{
	tmp->next = NULL;
	tmp->waittime = 0;

	/* Thread the command into the correct queue */
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
 * @brief Remove an entry from the wait queue.
 *
 * @param qptr Queue Entry
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

	/* Changing the player to NOTHING will flag this as halted, but we may have to delete it from the wait queue as well as the semaphore queue. */
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
 * @brief Notify commands from the queue and perform or discard them.
 *
 * @param player DBref of player
 * @param sem    DBref of sem
 * @param attr	 Attributes
 * @param key 	 Key
 * @param count  Counter
 * @return int
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
 * @brief Adjust the wait time on an existing entry.
 *
 * @param player  DBref of player
 * @param key     Key
 * @param pidstr  PID of the entry
 * @param timestr Wait time
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
 * @brief Return the time in seconds until the next command should be run from the queue.
 *
 * @return int
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

	/* Walk the wait and semaphore queues, looking for the smallest wait value.  Return the smallest value - 1, because the command gets moved to the player queue when it has 1 second to go. */
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

/* Check the wait and semaphore queues for commands to remove. */
void do_second(void)
{
	BQUE *trail = NULL, *point = NULL, *next = NULL;
	char *cmdsave = NULL;

	/* move contents of low priority queue onto end of normal one this helps to keep objects from getting out of control since its affects on other objects happen only after one second  this should allow \@halt to be type before getting blown away  by scrolling text */
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
