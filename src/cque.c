/* cque.c - commands and functions for manipulating the command queue */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"	/* required by mudconf */
#include "mushconf.h"	/* required by code */
#include "db.h"			/* required by externs */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by interface */
#include "match.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "command.h"	/* required by code */
#include "stringutil.h" /* required by code */

extern int a_Queue(dbref, int);

extern void s_Queue(dbref, int);

extern int QueueMax(dbref);

int qpid_top = 1;

/*
 * ---------------------------------------------------------------------------
 * Delete and free a queue entry.
 */

void delete_qentry(BQUE *qptr)
{
	nhashdelete(qptr->pid, &mudstate.qpid_htab);
	Free_QData(qptr);
	XFREE(qptr);
}

/*
 * ---------------------------------------------------------------------------
 * add_to: Adjust an object's queue or semaphore count.
 */

int add_to(dbref doer, dbref player, int am, int attrnum)
{
	int num, aflags, alen;
	dbref aowner;
	char *buff;
	char *atr_gotten;
	num = (int)strtol(atr_gotten = atr_get(player, attrnum, &aowner, &aflags, &alen), (char **)NULL, 10);
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
		atr_add(player, attrnum, (char *)'\0', Owner(doer), aflags);
	}

	return (num);
}

/*
 * ---------------------------------------------------------------------------
 * give_que: Thread a queue block onto the high or low priority queue
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
		if (mudstate.qlast != NULL)
		{
			mudstate.qlast->next = tmp;
			mudstate.qlast = tmp;
		}
		else
		{
			mudstate.qlast = mudstate.qfirst = tmp;
		}
	}
	else
	{
		if (mudstate.qllast)
		{
			mudstate.qllast->next = tmp;
			mudstate.qllast = tmp;
		}
		else
		{
			mudstate.qllast = mudstate.qlfirst = tmp;
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * que_want: Do we want this queue entry?
 */

int que_want(BQUE *entry, dbref ptarg, dbref otarg)
{
	if (!Good_obj(entry->player))
	{
		return 0;
	}

	if ((ptarg != NOTHING) && (ptarg != Owner(entry->player)))
	{
		return 0;
	}

	if ((otarg != NOTHING) && (otarg != entry->player))
	{
		return 0;
	}

	return 1;
}

/*
 * ---------------------------------------------------------------------------
 * halt_que: Remove all queued commands from a certain player
 */

int halt_que(dbref player, dbref object)
{
	BQUE *trail, *point, *next;
	int numhalted, halt_all, i;
	int *dbrefs_array;
	numhalted = 0;
	halt_all = ((player == NOTHING) && (object == NOTHING)) ? 1 : 0;

	if (halt_all)
		dbrefs_array = (int *)XCALLOC(mudstate.db_top, sizeof(int), "dbrefs_array");

	/*
     * Player queue
     */

	for (point = mudstate.qfirst; point; point = point->next)
		if (que_want(point, player, object))
		{
			numhalted++;

			if (halt_all && Good_obj(point->player))
			{
				dbrefs_array[Owner(point->player)] += 1;
			}

			point->player = NOTHING;
		}

	/*
     * Object queue
     */

	for (point = mudstate.qlfirst; point; point = point->next)
		if (que_want(point, player, object))
		{
			numhalted++;

			if (halt_all && Good_obj(point->player))
			{
				dbrefs_array[Owner(point->player)] += 1;
			}

			point->player = NOTHING;
		}

	/*
     * Wait queue
     */

	for (point = mudstate.qwait, trail = NULL; point; point = next)
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
				mudstate.qwait = next = point->next;
			}

			delete_qentry(point);
		}
		else
		{
			next = (trail = point)->next;
		}

	/*
     * Semaphore queue
     */

	for (point = mudstate.qsemfirst, trail = NULL; point; point = next)
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
				mudstate.qsemfirst = next = point->next;
			}

			if (point == mudstate.qsemlast)
			{
				mudstate.qsemlast = trail;
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
		for (i = 0; i < mudstate.db_top; i++)
		{
			if (dbrefs_array[i])
			{
				giveto(i, (mudconf.waitcost * dbrefs_array[i]));
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

	giveto(player, (mudconf.waitcost * numhalted));

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

/*
 * ---------------------------------------------------------------------------
 * remove_waitq: Remove an entry from the wait queue.
 */

void remove_waitq(BQUE *qptr)
{
	BQUE *point, *trail;

	if (qptr == mudstate.qwait)
	{
		/*
	 * Head of the queue. Just remove it and relink.
	 */
		mudstate.qwait = qptr->next;
	}
	else
	{
		/*
	 * Go find it somewhere in the queue and take it out.
	 */
		for (point = mudstate.qwait, trail = NULL; point != NULL; point = point->next)
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

/*
 * ---------------------------------------------------------------------------
 * do_halt_pid: Halt a single queue entry.
 */

void do_halt_pid(dbref player, dbref cause, int key, char *pidstr)
{
	dbref victim;
	int qpid;
	BQUE *qptr, *last, *tmp;

	if (!is_integer(pidstr))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	qpid = (int)strtol(pidstr, (char **)NULL, 10);

	if ((qpid < 1) || (qpid > mudconf.max_qpid))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	qptr = (BQUE *)nhashfind(qpid, &mudstate.qpid_htab);

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

	/*
     * Changing the player to NOTHING will flag this as halted, but we
     * may have to delete it from the wait queue as well as the semaphore
     * queue.
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
		for (tmp = mudstate.qsemfirst, last = NULL; tmp != NULL; last = tmp, tmp = tmp->next)
		{
			if (tmp == qptr)
			{
				if (last)
				{
					last->next = tmp->next;
				}
				else
				{
					mudstate.qsemfirst = tmp->next;
				}

				if (mudstate.qsemlast == tmp)
				{
					mudstate.qsemlast = last;
				}

				break;
			}
		}

		add_to(player, qptr->sem, -1, qptr->attr);
		delete_qentry(qptr);
	}

	giveto(victim, mudconf.waitcost);
	a_Queue(victim, -1);
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Halted queue entry PID %d.", qpid);
}

/*
 * ---------------------------------------------------------------------------
 * do_halt: Command interface to halt_que.
 */

void do_halt(dbref player, dbref cause, int key, char *target)
{
	dbref player_targ, obj_targ;
	int numhalted;

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

	/*
     * Figure out what to halt
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

/*
 * ---------------------------------------------------------------------------
 * nfy_que: Notify commands from the queue and perform or discard them.
 */

int nfy_que(dbref player, dbref sem, int attr, int key, int count)
{
	BQUE *point, *trail, *next;
	int num, aflags, alen;
	dbref aowner;
	char *str;

	if (attr)
	{
		str = atr_get(sem, attr, &aowner, &aflags, &alen);
		num = (int)strtol(str, (char **)NULL, 10);
		;
		XFREE(str);
	}
	else
	{
		num = 1;
	}

	if (num > 0)
	{
		num = 0;

		for (point = mudstate.qsemfirst, trail = NULL; point; point = next)
		{
			if ((point->sem == sem) && ((point->attr == attr) || !attr))
			{
				num++;

				if (trail)
				{
					trail->next = next = point->next;
				}
				else
					mudstate.qsemfirst = next = point->next;

				if (point == mudstate.qsemlast)
				{
					mudstate.qsemlast = trail;
				}

				/*
		 * Either run or discard the command
		 */

				if (key != NFY_DRAIN)
				{
					give_que(point);
				}
				else
				{
					giveto(point->player, mudconf.waitcost);
					a_Queue(Owner(point->player), -1);
					delete_qentry(point);
				}
			}
			else
			{
				next = (trail = point)->next;
			}

			/*
	     * If we've notified enough, exit
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

	/*
     * Update the sem waiters count
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

/*
 * ---------------------------------------------------------------------------
 * do_notify: Command interface to nfy_que
 */

void do_notify(dbref player, dbref cause, int key, char *what, char *count)
{
	dbref thing, aowner;
	int loccount, attr, aflags;
	ATTR *ap;
	char *obj;
	obj = parse_to(&what, '/', 0);
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
			/*
	     * Do they have permission to set this attribute?
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
			loccount = (int)strtol(count, (char **)NULL, 10);
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

/*
 * ---------------------------------------------------------------------------
 * Get the next available queue PID.
 */

int qpid_next(void)
{
	int i;
	int qpid = qpid_top;

	for (i = 0; i < mudconf.max_qpid; i++)
	{
		if (qpid > mudconf.max_qpid)
		{
			qpid = 1;
		}

		if (nhashfind(qpid, &mudstate.qpid_htab) != NULL)
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

/*
 * ---------------------------------------------------------------------------
 * setup_que: Set up a queue entry.
 */

BQUE *setup_que(dbref player, dbref cause, char *command, char *args[], int nargs, GDATA *gargs)
{
	int a, tlen, qpid;
	BQUE *tmp;
	char *tptr;

	/*
     * Can we run commands at all?
     */

	if (Halted(player))
	{
		return NULL;
	}

	/*
     * make sure player can afford to do it
     */
	a = mudconf.waitcost;

	if (a && mudconf.machinecost && (Randomize(mudconf.machinecost) == 0))
	{
		a++;
	}

	if (!payfor(player, a))
	{
		notify(Owner(player), "Not enough money to queue command.");
		return NULL;
	}

	/*
     * Wizards and their objs may queue up to db_top+1 cmds. Players are
     * limited to QUEUE_QUOTA. -mnp
     */
	a = QueueMax(Owner(player));

	if (a_Queue(Owner(player), 1) > a)
	{
		notify(Owner(player), "Run away objects: too many commands queued.  Halted.");
		halt_que(Owner(player), NOTHING);
		/*
	 * halt also means no command execution allowed
	 */
		s_Halted(player);
		return NULL;
	}

	/*
     * Generate a PID
     */
	qpid = qpid_next();

	if (qpid == 0)
	{
		notify(Owner(player), "Could not queue command. The queue is full.");
		return NULL;
	}

	/*
     * We passed all the tests
     */
	/*
     * Calculate the length of the save string
     */
	tlen = 0;

	if (command)
	{
		tlen = strlen(command) + 1;
	}

	if (nargs > NUM_ENV_VARS)
	{
		nargs = NUM_ENV_VARS;
	}

	for (a = 0; a < nargs; a++)
	{
		if (args[a])
		{
			tlen += (strlen(args[a]) + 1);
		}
	}

	if (gargs)
	{
		for (a = 0; a < gargs->q_alloc; a++)
		{
			if (gargs->q_regs[a])
			{
				tlen += gargs->q_lens[a] + 1;
			}
		}

		for (a = 0; a < gargs->xr_alloc; a++)
		{
			if (gargs->x_names[a] && gargs->x_regs[a])
			{
				tlen += strlen(gargs->x_names[a]) + gargs->x_lens[a] + 2;
			}
		}
	}

	/*
     * Create the queue entry and load the save string
     */
	tmp = XMALLOC(sizeof(BQUE), "tmp");

	if (!(tptr = tmp->text = (char *)XMALLOC(tlen, "tmp->text")))
	{
		XFREE(tmp);
		return (BQUE *)NULL;
	}

	/*
     * Set up registers and whatnot
     */
	tmp->comm = NULL;

	for (a = 0; a < NUM_ENV_VARS; a++)
	{
		tmp->env[a] = NULL;
	}

	Alloc_RegData("setup_que", gargs, tmp->gdata);

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

	/*
     * Load the rest of the queue block
     */
	tmp->pid = qpid;
	nhashadd(qpid, (int *)tmp, &mudstate.qpid_htab);
	tmp->player = player;
	tmp->waittime = 0;
	tmp->next = NULL;
	tmp->sem = NOTHING;
	tmp->attr = 0;
	tmp->cause = cause;
	tmp->nargs = nargs;
	return tmp;
}

/*
 * ---------------------------------------------------------------------------
 * wait_que: Add commands to the wait or semaphore queues.
 */

void wait_que(dbref player, dbref cause, int wait, dbref sem, int attr, char *command, char *args[], int nargs, GDATA *gargs)
{
	BQUE *tmp, *point, *trail;

	if (mudconf.control_flags & CF_INTERP)
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

	/*
     * Set wait time, and check for integer overflow
     */

	if (wait != 0)
	{
		tmp->waittime = time(NULL) + wait;
	}

	if ((wait > 0) && (tmp->waittime < 0))
	{
		tmp->waittime = INT_MAX;
	}

	tmp->sem = sem;
	tmp->attr = attr;

	if (sem == NOTHING)
	{
		/*
	 * No semaphore, put on wait queue if wait value specified.
	 * Otherwise put on the normal queue.
	 */
		if (wait <= 0)
		{
			give_que(tmp);
		}
		else
		{
			for (point = mudstate.qwait, trail = NULL; point && point->waittime <= tmp->waittime; point = point->next)
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
				mudstate.qwait = tmp;
			}
		}
	}
	else
	{
		tmp->next = NULL;

		if (mudstate.qsemlast != NULL)
		{
			mudstate.qsemlast->next = tmp;
		}
		else
		{
			mudstate.qsemfirst = tmp;
		}

		mudstate.qsemlast = tmp;
	}
}

/*
 * ---------------------------------------------------------------------------
 * do_wait_pid: Adjust the wait time on an existing entry.
 */

void do_wait_pid(dbref player, int key, char *pidstr, char *timestr)
{
	int qpid, wsecs;
	BQUE *qptr, *point, *trail;

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

	qpid = (int)strtol(pidstr, (char **)NULL, 10);

	if ((qpid < 1) || (qpid > mudconf.max_qpid))
	{
		notify(player, "That is not a valid PID.");
		return;
	}

	qptr = (BQUE *)nhashfind(qpid, &mudstate.qpid_htab);

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
		wsecs = (int)strtol(timestr, (char **)NULL, 10);

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
		if ((timestr[0] == '+') || (timestr[0] == '-'))
		{
			qptr->waittime += (int)strtol(timestr, (char **)NULL, 10);
		}
		else
		{
			qptr->waittime = time(NULL) + (int)strtol(timestr, (char **)NULL, 10);
			;
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

	/*
     * The semaphore queue is unsorted, but the main wait queue is
     * sorted. So we may have to go rethread.
     */

	if (qptr->sem == NOTHING)
	{
		remove_waitq(qptr);

		/*
	 * Re-insert
	 */
		for (point = mudstate.qwait, trail = NULL; point && point->waittime <= qptr->waittime; point = point->next)
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
			mudstate.qwait = qptr;
		}
	}

	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Adjusted wait time for queue entry PID %d.", qpid);
}

/*
 * ---------------------------------------------------------------------------
 * do_wait: Command interface to wait_que
 */

void do_wait(dbref player, dbref cause, int key, char *event, char *cmd, char *cargs[], int ncargs)
{
	dbref thing, aowner;
	int howlong, num, attr, aflags;
	char *what;
	ATTR *ap;

	if (key & WAIT_PID)
	{
		do_wait_pid(player, key, event, cmd);
		return;
	}

	/*
     * If arg1 is all numeric, do simple (non-sem) timed wait.
     */

	if (is_number(event))
	{
		if (key & WAIT_UNTIL)
		{
			howlong = (int)strtol(event, (char **)NULL, 10) - time(NULL);

			if (howlong < 0)
			{
				howlong = 0;
			}
		}
		else
		{
			howlong = (int)strtol(event, (char **)NULL, 10);
		}

		wait_que(player, cause, howlong, NOTHING, 0, cmd, cargs, ncargs, mudstate.rdata);
		return;
	}

	/*
     * Semaphore wait with optional timeout
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
		/*
	 * Get timeout, default 0
	 */
		if (event && *event && is_number(event))
		{
			attr = A_SEMAPHORE;

			if (key & WAIT_UNTIL)
			{
				howlong = (int)strtol(event, (char **)NULL, 10) - time(NULL);

				if (howlong < 0)
				{
					howlong = 0;
				}
			}
			else
			{
				howlong = (int)strtol(event, (char **)NULL, 10);
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
			/*
	     * thing over-notified, run the command immediately
	     */
			thing = NOTHING;
			howlong = 0;
		}

		wait_que(player, cause, howlong, thing, attr, cmd, cargs, ncargs, mudstate.rdata);
	}
}

/*
 * ---------------------------------------------------------------------------
 * que_next: Return the time in seconds until the next command should be run
 * from the queue.
 */

int que_next(void)
{
	int min, this;
	BQUE *point;

	/*
     * If there are commands in the player queue, we want to run them
     * immediately.
     */

	if (test_top())
	{
		return 0;
	}

	/*
     * If there are commands in the object queue, we want to run them
     * after a one-second pause.
     */

	if (mudstate.qlfirst != NULL)
	{
		return 1;
	}

	/*
     * Walk the wait and semaphore queues, looking for the smallest wait
     * value.  Return the smallest value - 1, because the command gets
     * moved to the player queue when it has 1 second to go.
     */
	min = 1000;

	for (point = mudstate.qwait; point; point = point->next)
	{
		this = point->waittime - mudstate.now;

		if (this <= 2)
		{
			return 1;
		}

		if (this < min)
		{
			min = this;
		}
	}

	for (point = mudstate.qsemfirst; point; point = point->next)
	{
		if (point->waittime == 0)
		{ /* Skip if no timeout */
			continue;
		}

		this = point->waittime - mudstate.now;

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

/*
 * ---------------------------------------------------------------------------
 * do_second: Check the wait and semaphore queues for commands to remove.
 */

void do_second(void)
{
	BQUE *trail, *point, *next;
	char *cmdsave;

	/*
     * move contents of low priority queue onto end of normal one this
     * helps to keep objects from getting out of control since its
     * affects on other objects happen only after one second  this should
     * allow @halt to be type before getting blown away  by scrolling
     * text
     */

	if ((mudconf.control_flags & CF_DEQUEUE) == 0)
	{
		return;
	}

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *)"< do_second >";

	if (mudstate.qlfirst)
	{
		if (mudstate.qlast)
		{
			mudstate.qlast->next = mudstate.qlfirst;
		}
		else
		{
			mudstate.qfirst = mudstate.qlfirst;
		}

		mudstate.qlast = mudstate.qllast;
		mudstate.qllast = mudstate.qlfirst = NULL;
	}

	/*
     * Note: the point->waittime test would be 0 except the command is
     * being put in the low priority queue to be done in one second
     * anyway
     */

	/*
     * Do the wait queue
     */

	while (mudstate.qwait && mudstate.qwait->waittime <= mudstate.now)
	{
		point = mudstate.qwait;
		mudstate.qwait = point->next;
		give_que(point);
	}

	/*
     * Check the semaphore queue for expired timed-waits
     */

	for (point = mudstate.qsemfirst, trail = NULL; point; point = next)
	{
		if (point->waittime == 0)
		{
			next = (trail = point)->next;
			continue; /* Skip if not timed-wait */
		}

		if (point->waittime <= mudstate.now)
		{
			if (trail != NULL)
			{
				trail->next = next = point->next;
			}
			else
			{
				mudstate.qsemfirst = next = point->next;
			}

			if (point == mudstate.qsemlast)
			{
				mudstate.qsemlast = trail;
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

	mudstate.debug_cmd = cmdsave;
	return;
}

/*
 * ---------------------------------------------------------------------------
 * do_top: Execute the command at the top of the queue
 */

int do_top(int ncmds)
{
	BQUE *tmp;
	dbref player;
	int count;
	char *cmdsave;

	if ((mudconf.control_flags & CF_DEQUEUE) == 0)
	{
		return 0;
	}

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *)"< do_top >";

	for (count = 0; count < ncmds; count++)
	{
		if (!test_top())
		{
			mudstate.debug_cmd = cmdsave;
			Free_RegData(mudstate.rdata);
			mudstate.rdata = NULL;
			return count;
		}

		player = mudstate.qfirst->player;

		if ((player >= 0) && !Going(player))
		{
			giveto(player, mudconf.waitcost);
			mudstate.curr_enactor = mudstate.qfirst->cause;
			mudstate.curr_player = player;
			a_Queue(Owner(player), -1);
			mudstate.qfirst->player = NOTHING;

			if (!Halted(player))
			{
				/*
		 * Load scratch args
		 */
				if (mudstate.qfirst->gdata)
				{
					Free_RegData(mudstate.rdata);
					Alloc_RegData("do_top", mudstate.qfirst->gdata, mudstate.rdata);
					Copy_RegData("do_top", mudstate.qfirst->gdata, mudstate.rdata);
				}
				else
				{
					Free_RegData(mudstate.rdata);
					mudstate.rdata = NULL;
				}

				mudstate.cmd_invk_ctr = 0;
				process_cmdline(player, mudstate.qfirst->cause, mudstate.qfirst->comm, mudstate.qfirst->env, mudstate.qfirst->nargs, mudstate.qfirst);
			}
		}

		if (mudstate.qfirst)
		{
			tmp = mudstate.qfirst;
			mudstate.qfirst = mudstate.qfirst->next;
			delete_qentry(tmp);
		}

		if (!mudstate.qfirst)
		{ /* gotta check this, as the value's
				 * changed */
			mudstate.qlast = NULL;
		}
	}

	Free_RegData(mudstate.rdata);
	mudstate.rdata = NULL;
	mudstate.debug_cmd = cmdsave;
	return count;
}

/*
 * ---------------------------------------------------------------------------
 * do_ps: tell player what commands they have pending in the queue
 */

void show_que(dbref player, int key, BQUE *queue, int *qtot, int *qent, int *qdel, dbref player_targ, dbref obj_targ, const char *header)
{
	BQUE *tmp;
	char *bp, *bufp;
	int i;
	ATTR *ap;
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
				/*
		 * A minor shortcut. We can never
		 * timeout-wait on a non-Semaphore attribute.
		 */
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[#%d/%d] %d:%s:%s", tmp->sem, tmp->waittime - mudstate.now, tmp->pid, bufp, tmp->comm);
			}
			else if (tmp->waittime > 0)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%d] %d:%s:%s", tmp->waittime - mudstate.now, tmp->pid, bufp, tmp->comm);
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
						SAFE_LB_STR((char *)"; Arg", bufp, &bp);
						SAFE_LB_CHR(i + '0', bufp, &bp);
						SAFE_LB_STR((char *)"='", bufp, &bp);
						SAFE_LB_STR(tmp->env[i], bufp, &bp);
						SAFE_LB_CHR('\'', bufp, &bp);
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

void do_ps(dbref player, dbref cause, int key, char *target)
{
	char *bufp;
	dbref player_targ, obj_targ;
	int pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent, sqtot, i;

	/*
     * Figure out what to list the queue for
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

	/*
     * Go do it
     */
	show_que(player, key, mudstate.qfirst, &pqtot, &pqent, &pqdel, player_targ, obj_targ, "Player");
	show_que(player, key, mudstate.qlfirst, &oqtot, &oqent, &oqdel, player_targ, obj_targ, "Object");
	show_que(player, key, mudstate.qwait, &wqtot, &wqent, &i, player_targ, obj_targ, "Wait");
	show_que(player, key, mudstate.qsemfirst, &sqtot, &sqent, &i, player_targ, obj_targ, "Semaphore");
	/*
     * Display stats
     */
	bufp = XMALLOC(MBUF_SIZE, "bufp");

	if (See_Queue(player))
		XSPRINTF(bufp, "Totals: Player...%d/%d[%ddel]  Object...%d/%d[%ddel]  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent, sqtot);
	else
		XSPRINTF(bufp, "Totals: Player...%d/%d  Object...%d/%d  Wait...%d/%d  Semaphore...%d/%d", pqent, pqtot, oqent, oqtot, wqent, wqtot, sqent, sqtot);

	notify(player, bufp);
	XFREE(bufp);
}

/*
 * ---------------------------------------------------------------------------
 * do_queue: Queue management
 */

void do_queue(dbref player, dbref cause, int key, char *arg)
{
	BQUE *point;
	int i, ncmds, was_disabled;
	was_disabled = 0;

	if (key == QUEUE_KICK)
	{
		i = (int)strtol(arg, (char **)NULL, 10);
		;

		if ((mudconf.control_flags & CF_DEQUEUE) == 0)
		{
			was_disabled = 1;
			mudconf.control_flags |= CF_DEQUEUE;
			notify(player, "Warning: automatic dequeueing is disabled.");
		}

		ncmds = do_top(i);

		if (was_disabled)
		{
			mudconf.control_flags &= ~CF_DEQUEUE;
		}

		if (!Quiet(player))
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d commands processed.", ncmds);
		}
	}
	else if (key == QUEUE_WARP)
	{
		i = (int)strtol(arg, (char **)NULL, 10);
		;

		if ((mudconf.control_flags & CF_DEQUEUE) == 0)
		{
			was_disabled = 1;
			mudconf.control_flags |= CF_DEQUEUE;
			notify(player, "Warning: automatic dequeueing is disabled.");
		}

		/*
	 * Handle the wait queue
	 */

		for (point = mudstate.qwait; point; point = point->next)
		{
			point->waittime = -i;
		}

		/*
	 * Handle the semaphore queue
	 */

		for (point = mudstate.qsemfirst; point; point = point->next)
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
			mudconf.control_flags &= ~CF_DEQUEUE;
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
