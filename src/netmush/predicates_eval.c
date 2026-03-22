/**
 * @file predicates_eval.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Attribute evaluation, verb execution, and server control
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
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

/* ---------------------------------------------------------------------------
 * do_restart: Restarts the game.
 */

void do_restart(dbref player, dbref cause, int key)
{
	MODULE *mp = NULL;
	char *name = NULL;
	int err = 0;

	if (mushstate.dumping)
	{
		notify(player, "Dumping. Please try again later.");
		return;
	}

	/*
	 * Make sure what follows knows we're restarting. No need to clear
	 * * this, since this process is going away-- this is also set on
	 * * startup when the restart.db is read.
	 */
	mushstate.restarting = 1;
	raw_broadcast(0, "GAME: Restart by %s, please wait.", Name(Owner(player)));
	name = log_getname(player);
	log_write(LOG_ALWAYS, "WIZ", "RSTRT", "Restart by %s", name);
	XFREE(name);
	/*
	 * Do a dbck first so we don't end up with an inconsistent state.
	 * * Otherwise, since we don't write out GOING objects, the initial
	 * * dbck at startup won't have valid data to work with in order to
	 * * clean things out.
	 */
	do_dbck(NOTHING, NOTHING, 0);
	/*
	 * Dump databases, etc.
	 */
	al_store(); /* Persist any in-memory attribute list before restart */
	dump_database_internal(DUMP_DB_RESTART);

	db_sync_attributes();
	dddb_close();

	logfile_close();
	alarm(0);
	dump_restart_db();

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		dlclose(mp->handle);
	}

	err = execl(mushconf.game_exec, mushconf.game_exec, mushconf.config_file, (char *)NULL);

	if (err == -1)
	{
		int saved_errno = errno;
		char errbuf[256];
		if (strerror_r(saved_errno, errbuf, sizeof(errbuf)) != 0)
		{
			snprintf(errbuf, sizeof(errbuf), "Unknown error %d", saved_errno);
		}
		log_write(LOG_ALWAYS, "WIZ", "RSTRT", "execl report an error %s", errbuf);
	}
}

/* ---------------------------------------------------------------------------
 * do_comment: Implement the @@ (comment) command. Very cpu-intensive :-)
 * do_eval is similar, except it gets passed on arg.
 */

void do_comment(dbref player, dbref cause, int key)
{
}

void do_eval(dbref player, dbref cause, int key, char *str)
{
}

/* ---------------------------------------------------------------------------
 * master_attr: Get the evaluated text string of a master attribute.
 *              Note that this returns an lbuf, which must be freed by
 *              the caller.
 */

char *master_attr(dbref player, dbref thing, int what, char **sargs, int nsargs, int *f_ptr)
{
	/*
	 * If the attribute exists, evaluate it and return pointer to lbuf.
	 * * If not, return NULL.
	 * * Respect global overrides.
	 * * what is assumed to be more than 0.
	 */
	char *d, *m, *buff, *bp, *str, *tbuf, *tp, *sp, *list, *bb_p, *lp;
	int t, aflags, alen, is_ok, lev;
	dbref aowner, master, parent, obj;
	ATTR *ap;
	GDATA *preserve;

	if (NoDefault(thing))
	{
		master = NOTHING;
	}
	else
	{
		switch (Typeof(thing))
		{
		case TYPE_ROOM:
			master = mushconf.room_defobj;
			break;

		case TYPE_EXIT:
			master = mushconf.exit_defobj;
			break;

		case TYPE_PLAYER:
			master = mushconf.player_defobj;
			break;

		case TYPE_GARBAGE:
			return NULL;
			break; /* NOTREACHED */

		default:
			master = mushconf.thing_defobj;
		}

		if (master == thing)
		{
			master = NOTHING;
		}
	}

	m = NULL;
	d = atr_pget(thing, what, &aowner, &aflags, &alen);

	if (Good_obj(master))
	{
		ap = atr_num(what);
		t = (ap && (ap->flags & AF_DEFAULT)) ? 1 : 0;
	}
	else
	{
		t = 0;
	}

	if (t)
	{
		m = atr_pget(master, what, &aowner, &aflags, &alen);
	}

	if (f_ptr)
	{
		*f_ptr = aflags;
	}

	const int has_d = (d && *d);
	const int has_m = (t && m && *m);

	if (!(has_d || has_m))
	{
		if (d)
		{
			XFREE(d);
		}

		if (m)
		{
			XFREE(m);
		}

		return NULL;
	}

	/*
	 * Construct any arguments that we're going to pass along on the
	 * * stack.
	 */

	switch (what)
	{
	case A_LEXITS_FMT:
		list = XMALLOC(LBUF_SIZE, "list");
		bb_p = lp = list;
		is_ok = Darkened(player, thing);

		for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (!Has_exits(parent))
			{
				continue;
			}

			int exit_count = 0;
			for (obj = Exits(parent); (obj != NOTHING) && (Next(obj) != obj) && (exit_count < mushstate.db_top); obj = Next(obj), exit_count++)
			{
				if (Can_See_Exit(player, obj, is_ok))
				{
					if (lp != bb_p)
					{
						XSAFELBCHR(' ', list, &lp);
					}

					XSAFELBCHR('#', list, &lp);
					XSAFELTOS(list, &lp, obj, LBUF_SIZE);
				}
			}
		}
		*lp = '\0';
		is_ok = 1;
		break;

	case A_LCON_FMT:
		list = XMALLOC(LBUF_SIZE, "list");
		bb_p = lp = list;
		is_ok = Sees_Always(player, thing);

		int cont_count = 0;
		for (obj = Contents(thing); (obj != NOTHING) && (Next(obj) != obj) && (cont_count < mushstate.db_top); obj = Next(obj), cont_count++)
		{
			if (Can_See(player, obj, is_ok))
			{
				if (lp != bb_p)
				{
					XSAFELBCHR(' ', list, &lp);
				}

				XSAFELBCHR('#', list, &lp);
				XSAFELTOS(list, &lp, obj, LBUF_SIZE);
			}
		}
		*lp = '\0';
		is_ok = 1;
		break;

	default:
		list = NULL;
		is_ok = nsargs;
		break;
	}

	/*
	 * Go do it.
	 */
	preserve = save_global_regs("master_attr_save");
	buff = bp = XMALLOC(LBUF_SIZE, "bp");

	if (has_m)
	{
		str = m;

		if (has_d)
		{
			sp = d;
			tbuf = tp = XMALLOC(LBUF_SIZE, "tp");
			eval_expression_string(tbuf, &tp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &sp, ((list == NULL) ? sargs : &list), is_ok);
			eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, &tbuf, 1);
			XFREE(tbuf);
		}
		else
		{
			eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, ((list == NULL) ? sargs : &list), is_ok);
		}
	}
	else if (has_d)
	{
		str = d;
		eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, ((list == NULL) ? sargs : &list), is_ok);
	}

	*bp = '\0';
	if (d)
	{
		XFREE(d);
	}

	if (m)
	{
		XFREE(m);
	}

	if (list)
	{
		XFREE(list);
	}

	restore_global_regs("master_attr_restore", preserve);
	return buff;
}

/* ---------------------------------------------------------------------------
 * did_it: Have player do something to/with thing
 */

void did_it(dbref player, dbref thing, int what, const char *def, int owhat, const char *odef, int awhat, int ctrl_flags, char *args[], int nargs, int msg_key)
{
	GDATA *preserve = NULL;
	dbref loc = NOTHING, aowner = NOTHING, master = NOTHING;
	ATTR *ap = NULL;
	char *d = NULL, *m = NULL, *buff = NULL, *act = NULL, *charges = NULL, *bp = NULL, *str = NULL;
	char *tbuf = NULL, *tp = NULL, *sp = NULL;
	int t = 0, num = 0, aflags = 0, alen = 0, need_pres = 0, retval = 0;


	/*
	 * If we need to call eval_expression_string() from within this function, we first save
	 * * the state of the global registers, in order to avoid munging them
	 * * inappropriately. Do note that the restoration to their original
	 * * values occurs BEFORE the execution of the @a-attribute. Therefore,
	 * * any changing of setq() values done in the @-attribute and @o-attribute
	 * * will NOT be passed on. This prevents odd behaviors that result from
	 * * odd @verbs and so forth (the idea is to preserve the caller's control
	 * * of the global register values).
	 */
	need_pres = 0;

	if (NoDefault(thing))
	{
		master = NOTHING;
	}
	else
	{
		switch (Typeof(thing))
		{
		case TYPE_ROOM:
			master = mushconf.room_defobj;
			break;

		case TYPE_EXIT:
			master = mushconf.exit_defobj;
			break;

		case TYPE_PLAYER:
			master = mushconf.player_defobj;
			break;

		default:
			master = mushconf.thing_defobj;
		}

		if (master == thing || !Good_obj(master))
		{
			master = NOTHING;
		}
	}

	/*
	 * Module call. Modules can return a negative number, zero, or a
	 * * positive number.
	 * * Positive: Stop calling modules. Return; do not execute normal did_it().
	 * * Zero: Continue calling modules. Execute normal did_it() if we get
	 * *       to the end of the modules and nothing has returned non-zero.
	 * * Negative: Stop calling modules. Execute normal did_it().
	 */

	retval = 0;
	for (MODULE *csm__mp = mushstate.modules_list; (csm__mp != NULL) && !retval; csm__mp = csm__mp->next)
	{
		if (csm__mp->did_it)
		{
			retval = (*(csm__mp->did_it))(player, thing, master, what, def, owhat, odef, awhat, ctrl_flags, args, nargs, msg_key);
		}
	}

	if (retval > 0)
	{
		return;
	}

	/*
	 * message to player
	 */
	m = NULL;

	if (what > 0)
	{
		/*
		 * Check for global attribute format override. If it exists,
		 * * use that. The actual attribute text we were provided
		 * * will be passed to that as %0. (Note that if a global
		 * * override exists, we never use a supplied server default.)
		 * *
		 * * Otherwise, we just go evaluate what we've got, and
		 * * if that's nothing, we go do the default.
		 */
		d = atr_pget(thing, what, &aowner, &aflags, &alen);

		if (Good_obj(master))
		{
			ap = atr_num(what);
			t = (ap && (ap->flags & AF_DEFAULT)) ? 1 : 0;
		}
		else
		{
			t = 0;
		}

		if (t)
		{
			m = atr_pget(master, what, &aowner, &aflags, &alen);
		}

		const int has_d = (d && *d);
		const int has_m = (t && m && *m);

		if (has_d || has_m)
		{
			need_pres = 1;
			preserve = save_global_regs("did_it_save");
			buff = bp = XMALLOC(LBUF_SIZE, "bp");

			if (has_m)
			{
				str = m;

				if (has_d)
				{
					sp = d;
					tbuf = tp = XMALLOC(LBUF_SIZE, "tp");
					eval_expression_string(tbuf, &tp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &sp, args, nargs);
					eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, &tbuf, 1);
					XFREE(tbuf);
				}
				else
				{
					eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, (char **)NULL, 0);
				}
			}
			else if (has_d)
			{
				str = d;
				eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, args, nargs);
			}

			*bp = '\0';

			if (mushconf.have_pueblo == 1)
			{
				if ((aflags & AF_HTML) && Html(player))
				{
					char *buff_cp = buff + strlen(buff);
					XSAFECRLF(buff, &buff_cp);
					notify_html(player, buff);
				}
				else
				{
					notify(player, buff);
				}
			}
			else
			{
				notify(player, buff);
			}

			XFREE(buff);
		}
		else if (def)
		{
			notify(player, def);
		}

		if (d)
		{
			XFREE(d);
		}
	}
	else if ((what < 0) && def)
	{
		notify(player, def);
	}

	if (m)
	{
		XFREE(m);
	}

	/*
	 * message to neighbors
	 */
	m = NULL;

	if ((owhat > 0) && Has_location(player) && Good_obj(loc = Location(player)))
	{
		d = atr_pget(thing, owhat, &aowner, &aflags, &alen);

		if (Good_obj(master))
		{
			ap = atr_num(owhat);
			t = (ap && (ap->flags & AF_DEFAULT)) ? 1 : 0;
		}
		else
		{
			t = 0;
		}

		if (t)
		{
			m = atr_pget(master, owhat, &aowner, &aflags, &alen);
		}

		const int has_d = (d && *d);
		const int has_m = (t && m && *m);

		if (has_d || has_m)
		{
			if (!need_pres)
			{
				need_pres = 1;
				preserve = save_global_regs("did_it_save");
			}

			buff = bp = XMALLOC(LBUF_SIZE, "bp");

			if (has_m)
			{
				str = m;

				if (has_d)
				{
					sp = d;
					tbuf = tp = XMALLOC(LBUF_SIZE, "tp");
					eval_expression_string(tbuf, &tp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &sp, args, nargs);
					eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, &tbuf, 1);
					XFREE(tbuf);
				}
				else if (odef)
				{
					eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, (char **)&odef, 1);
				}
				else
				{
					eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, (char **)NULL, 0);
				}
			}
			else if (has_d)
			{
				str = d;
				eval_expression_string(buff, &bp, thing, player, player, EV_EVAL | EV_FIGNORE | EV_TOP, &str, args, nargs);
			}

			*bp = '\0';

			if (*buff)
			{
				if (aflags & AF_NONAME)
				{
					notify_except2(loc, player, player, thing, msg_key, NULL, buff);
				}
				else
				{
					notify_except2(loc, player, player, thing, msg_key, "%s %s", Name(player), buff);
				}
			}

			XFREE(buff);
		}
		else if (odef)
		{
			if (ctrl_flags & VERB_NONAME)
			{
				notify_except2(loc, player, player, thing, msg_key, NULL, odef);
			}
			else
			{
				notify_except2(loc, player, player, thing, msg_key, "%s %s", Name(player), odef);
			}
		}

		if (d)
		{
			XFREE(d);
		}
	}
	else if ((owhat < 0) && odef && Has_location(player) && Good_obj(loc = Location(player)))
	{
		if (ctrl_flags & VERB_NONAME)
		{
			notify_except2(loc, player, player, thing, msg_key, NULL, odef);
		}
		else
		{
			notify_except2(loc, player, player, thing, msg_key, "%s %s", Name(player), odef);
		}
	}

	if (m)
	{
		XFREE(m);
	}

	/*
	 * If we preserved the state of the global registers, restore them.
	 */

	if (need_pres)
	{
		restore_global_regs("did_it_restore", preserve);
	}

	/*
	 * do the action attribute
	 */

	if (awhat > 0)
	{
		act = atr_pget(thing, awhat, &aowner, &aflags, &alen);

		if (!act)
		{
			log_write(LOG_ALWAYS, "TRIG", "TRACE", "did_it: !act, return");
			return;
		}

		if (*act)
		{
			charges = atr_pget(thing, A_CHARGES, &aowner, &aflags, &alen);
			char *runout = NULL;

			if (charges && *charges)
			{
				errno = 0;
				long temp_charges = strtol(charges, (char **)NULL, 10);
				if (errno == ERANGE || temp_charges > INT_MAX || temp_charges < INT_MIN)
				{
					/* Invalid charge value, skip */
					XFREE(charges);
					XFREE(act);
					return;
				}
				num = (int)temp_charges;
				buff = XMALLOC(LBUF_SIZE, "buff");
				XSPRINTF(buff, "%d", num - 1);
				atr_add_raw(thing, A_CHARGES, buff);
				XFREE(buff);
			}
			else if ((runout = atr_pget(thing, A_RUNOUT, &aowner, &aflags, &alen)) && *runout)
			{
				XFREE(act);
				act = runout;
				runout = NULL;
			}
			       else
			       {
				       /* On ne retourne plus ici : on laisse act tel quel pour exécution */
			       }

			if (runout)
			{
				XFREE(runout);
			}
		}

		if (charges)
		{
			XFREE(charges);
		}

		/*
		 * Skip any leading $<command>: or ^<monitor>: pattern.
		 * * If we run off the end of string without finding an unescaped
		 * * ':' (or there's nothing after it), go back to the beginning
		 * * of the string and just use that.
		 */

		if ((*act == '$') || (*act == '^'))
		{
			for (tp = act + 1; *tp && ((*tp != ':') || (*(tp - 1) == '\\')); tp++)
				;

			if (!*tp)
			{
				tp = act;
			}
			else
			{
				tp++; /* must advance past the ':' */
			}
		}
		else
		{
			tp = act;
		}

		/*
		 * Go do it.
		 */

		if (ctrl_flags & (VERB_NOW | TRIG_NOW))
		{
			preserve = save_global_regs("did_it_save2");
			process_cmdline(thing, player, tp, args, nargs, NULL);
			restore_global_regs("did_it_restore2", preserve);
		}
		else
		{
			cque_wait_que(thing, player, 0, NOTHING, 0, tp, args, nargs, mushstate.rdata);
		}
	}

	XFREE(act);
}

/*
 * ---------------------------------------------------------------------------
 * * do_verb: Command interface to did_it.
 */

void do_verb(dbref player, dbref cause, int key, char *victim_str, char *args[], int nargs)
{
	dbref actor, victim, aowner;
	int what, owhat, awhat, nxargs, restriction, aflags, i;
	ATTR *ap;
	const char *whatd, *owhatd;
	char *xargs[NUM_ENV_VARS];

	/*
	 * Look for the victim
	 */

	if (!victim_str || !*victim_str)
	{
		notify(player, "Nothing to do.");
		return;
	}

	/*
	 * Get the victim
	 */
	init_match(player, victim_str, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	victim = noisy_match_result();

	if (!Good_obj(victim))
	{
		return;
	}

	/*
	 * Get the actor.  Default is my cause
	 */

	if ((nargs >= 1) && args[0] && *args[0])
	{
		init_match(player, args[0], NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		actor = noisy_match_result();

		if (!Good_obj(actor))
		{
			return;
		}
	}
	else
	{
		actor = cause;
	}

	/*
	 * Check permissions.  There are two possibilities * 1: Player * * *
	 * controls both victim and actor.  In this case victim runs *    his
	 *
	 * *  * *  * * action list. * 2: Player controls actor.  In this case
	 * * victim * does  * not run his *    action list and any attributes
	 * * that * player cannot  * read from *    victim are defaulted.
	 */

	if (!controls(player, actor))
	{
		notify_quiet(player, NOPERM_MESSAGE);
		return;
	}

	restriction = !controls(player, victim);
	what = -1;
	owhat = -1;
	awhat = -1;
	whatd = NULL;
	owhatd = NULL;
	nxargs = 0;

	/*
	 * Get invoker message attribute
	 */

	if (nargs >= 2)
	{
		ap = atr_str(args[1]);

		if (ap && (ap->number > 0))
		{
			what = ap->number;
		}
	}

	/*
	 * Get invoker message default
	 */

	if ((nargs >= 3) && args[2] && *args[2])
	{
		whatd = args[2];
	}

	/*
	 * Get others message attribute
	 */

	if (nargs >= 4)
	{
		ap = atr_str(args[3]);

		if (ap && (ap->number > 0))
		{
			owhat = ap->number;
		}
	}

	/*
	 * Get others message default
	 */

	if ((nargs >= 5) && args[4] && *args[4])
	{
		owhatd = args[4];
	}

	/*
	 * Get action attribute
	 */

	if (nargs >= 6)
	{
		ap = atr_str(args[5]);

		if (ap)
		{
			awhat = ap->number;
		}
	}

	/*
	 * Get arguments
	 */

	if (nargs >= 7)
	{
		parse_arglist(victim, actor, actor, args[6], '\0', EV_STRIP_LS | EV_STRIP_TS, xargs, NUM_ENV_VARS, (char **)NULL, 0);

		for (nxargs = 0; (nxargs < NUM_ENV_VARS) && xargs[nxargs]; nxargs++)
			;
	}

	/*
	 * If player doesn't control both, enforce visibility restrictions.
	 * Regardless of control we still check if the player can read the
	 * attribute, since we don't want him getting wiz-readable-only attrs.
	 */
	atr_get_info(victim, what, &aowner, &aflags);

	if (what != -1)
	{
		ap = atr_num(what);

		if (!ap || !Read_attr(player, victim, ap, aowner, aflags) || (restriction && ((ap->number == A_DESC) && !mushconf.read_rem_desc && !Examinable(player, victim) && !nearby(player, victim))))
		{
			what = -1;
		}
	}

	atr_get_info(victim, owhat, &aowner, &aflags);

	if (owhat != -1)
	{
		ap = atr_num(owhat);

		if (!ap || !Read_attr(player, victim, ap, aowner, aflags) || (restriction && ((ap->number == A_DESC) && !mushconf.read_rem_desc && !Examinable(player, victim) && !nearby(player, victim))))
		{
			owhat = -1;
		}
	}

	if (restriction)
	{
		awhat = 0;
	}

	/*
	 * Go do it
	 */
	did_it(actor, victim, what, whatd, owhat, owhatd, awhat, key & (VERB_NOW | VERB_NONAME), xargs, nxargs, (((key & VERB_SPEECH) ? MSG_SPEECH : 0) | ((key & VERB_MOVE) ? MSG_MOVE : 0) | ((key & VERB_PRESENT) ? MSG_PRESENCE : 0)));

	/*
	 * Free user args
	 */

	for (i = 0; i < nxargs; i++)
	{
		XFREE(xargs[i]);
	}
}

/* ---------------------------------------------------------------------------
 * do_include: Run included text. This is very similar to a @trigger/now,
 * except that we only need to be able to read the attr, not control the
 * object, and it uses the original stack by default. If we get passed args,
 * we replace the entirety of the included text, just like a @trigger/now.
 * Also, unlike a trigger, we remain the thing running the action, and we
 * do not deplete charges.
 */

void do_include(dbref player, dbref cause, int key, char *object, char *argv[], int nargs, char *cargs[], int ncargs)
{
	dbref thing, aowner;
	int attrib, aflags, alen;
	char *act, *tp;
	char *s;
	/*
	 * Get the attribute. Default to getting it off ourselves.
	 */
	s = XASPRINTF("s", "me/%s", object);

	if (!((parse_attrib(player, object, &thing, &attrib, 0) && (attrib != NOTHING)) || (parse_attrib(player, s, &thing, &attrib, 0) && (attrib != NOTHING))))
	{
		notify_quiet(player, "No match.");
		XFREE(s);
		return;
	}
	XFREE(s);

	act = atr_pget(thing, attrib, &aowner, &aflags, &alen);

	if (!act)
	{
		return;
	}

	if (*act)
	{
		/*
		 * Skip leading $command: or ^monitor:
		 */
		if ((*act == '$') || (*act == '^'))
		{
			for (tp = act + 1; *tp && ((*tp != ':') || (*(tp - 1) == '\\')); tp++)
				;

			if (!*tp)
			{
				tp = act;
			}
			else
			{
				tp++; /* must advance past the ':' */
			}
		}
		else
		{
			tp = act;
		}

		/*
		 * Go do it. Use stack if we have it, otherwise use command stack.
		 * * Note that an empty stack is still one arg but it's empty.
		 */

		if ((nargs > 1) || ((nargs == 1) && *argv[0]))
		{
			process_cmdline(player, cause, tp, argv, nargs, NULL);
		}
		else
		{
			process_cmdline(player, cause, tp, cargs, ncargs, NULL);
		}
	}

	XFREE(act);
}

/* ---------------------------------------------------------------------------
 * do_redirect: Redirect PUPPET, TRACE, VERBOSE output to another player.
 */

void do_redirect(dbref player, dbref cause, int key, char *from_name, char *to_name)
{
	dbref from_ref, to_ref;
	NUMBERTAB *np;
	/*
	 * Find what object we're redirecting from. We must either control it,
	 * * or it must be REDIR_OK.
	 */
	init_match(player, from_name, NOTYPE);
	match_everything(0);
	from_ref = noisy_match_result();

	if (!Good_obj(from_ref))
	{
		return;
	}

	/*
	 * If we have no second argument, we are un-redirecting something
	 * * which is already redirected. We can get rid of it if we control
	 * * the object being redirected, or we control the target of the
	 * * redirection.
	 */

	if (!to_name || !*to_name)
	{
		if (!H_Redirect(from_ref))
		{
			notify(player, "That object is not being redirected.");
			return;
		}

		np = (NUMBERTAB *)nhashfind(from_ref, &mushstate.redir_htab);

		if (np)
		{
			/*
			 * This should always be true -- if we have the flag the
			 * * hashtable lookup should succeed -- but just in case,
			 * * we check. (We clear the flag upon startup.)
			 * * If we have a weird situation, we don't care whether
			 * * or not the control criteria gets met; we just fix it.
			 */
			if (!Controls(player, from_ref) && (np->num != player))
			{
				notify(player, NOPERM_MESSAGE);
				return;
			}

			if (np->num != player)
			{
				notify_check(np->num, np->num, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Output from %s(#%d) is no longer being redirected to you.", Name(from_ref), from_ref);
			}

			XFREE(np);
			nhashdelete(from_ref, &mushstate.redir_htab);
		}

		s_Flags3(from_ref, Flags3(from_ref) & ~HAS_REDIRECT);
		notify(player, "Redirection stopped.");

		if (from_ref != player)
		{
			notify(from_ref, "You are no longer being redirected.");
		}

		return;
	}

	/*
	 * If the object is already being redirected, we cannot do so again.
	 */

	if (H_Redirect(from_ref))
	{
		notify(player, "That object is already being redirected.");
		return;
	}

	/*
	 * To redirect something, it needs to either be REDIR_OK or we
	 * * need to control it.
	 */

	if (!Controls(player, from_ref) && !Redir_ok(from_ref))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/*
	 * Find the player that we're redirecting to. We must control the
	 * * player.
	 */
	to_ref = lookup_player(player, to_name, 1);

	if (!Good_obj(to_ref))
	{
		notify(player, "No such player.");
		return;
	}

	if (!Controls(player, to_ref))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/*
	 * Insert it into the hashtable.
	 */
	np = (NUMBERTAB *)XMALLOC(sizeof(NUMBERTAB), "np");
	np->num = to_ref;
	nhashadd(from_ref, (int *)np, &mushstate.redir_htab);
	s_Flags3(from_ref, Flags3(from_ref) | HAS_REDIRECT);

	if (from_ref != player)
	{
		notify_check(from_ref, from_ref, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You have been redirected to %s.", Name(to_ref));
	}

	if (to_ref != player)
	{
		notify_check(to_ref, to_ref, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Output from %s(#%d) has been redirected to you.", Name(from_ref), from_ref);
	}

	notify(player, "Redirected.");
}

/* ---------------------------------------------------------------------------
 * do_reference: Manipulate nrefs.
 */

void do_reference(dbref player, dbref cause, int key, char *ref_name, char *obj_name)
{
	HASHENT *hptr;
	HASHTAB *htab;
	int i, len, total, is_global;
	char *tbuf = XMALLOC(LBUF_SIZE, "tbuf");
	char *outbuf = XMALLOC(LBUF_SIZE, "outbuf");
	char *tp, *bp, *buff, *s;
	dbref target, *np;

	if (key & NREF_LIST)
	{
		htab = &mushstate.nref_htab;

		if (!ref_name || !*ref_name)
		{
			/*
			 * Global only.
			 */
			is_global = 1;
			tbuf[0] = '_';
			tbuf[1] = '\0';
			len = 1;
		}
		else
		{
			is_global = 0;

			if (!string_compare(ref_name, "me"))
			{
				target = player;
			}
			else
			{
				target = lookup_player(player, ref_name, 1);

				if (target == NOTHING)
				{
					notify(player, "No such player.");
					XFREE(outbuf);
					XFREE(tbuf);
					return;
				}

				if (!Controls(player, target))
				{
					notify(player, NOPERM_MESSAGE);
					XFREE(outbuf);
					XFREE(tbuf);
					return;
				}
			}

			tp = tbuf;
			XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
			XSAFELBCHR('.', tbuf, &tp);
			*tp = '\0';
			len = strlen(tbuf);
		}

		total = 0;

		for (i = 0; i < htab->hashsize; i++)
		{
			int entry_count = 0;
			for (hptr = htab->entry[i]; hptr != NULL && entry_count < mushstate.db_top; hptr = hptr->next, entry_count++)
			{
				if (!strncmp(tbuf, hptr->target.s, len))
				{
					total++;
					bp = outbuf;
					char *dot_pos = strchr(hptr->target.s, '.');
					XSAFESPRINTF(outbuf, &bp, "%s:  ", ((is_global) ? hptr->target.s : (dot_pos ? dot_pos + 1 : hptr->target.s)));
					buff = unparse_object(player, *(hptr->data), 0);
					XSAFELBSTR(buff, outbuf, &bp);
					XFREE(buff);

					if (Owner(player) != Owner(*(hptr->data)))
					{
						XSAFELBSTR((char *)" [owner: ", outbuf, &bp);
						buff = unparse_object(player, Owner(*(hptr->data)), 0);
						XSAFELBSTR(buff, outbuf, &bp);
						XFREE(buff);
						XSAFELBCHR(']', outbuf, &bp);
					}

					*bp = '\0';
					notify(player, outbuf);
				}
			}
		}

		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Total references: %d", total);
		XFREE(outbuf);
		XFREE(tbuf);
		return;
	}

	/*
	 * We can only reference objects that we can examine.
	 */

	if (obj_name && *obj_name)
	{
		target = match_thing(player, obj_name);

		if (!Good_obj(target))
		{
			XFREE(outbuf);
			XFREE(tbuf);
			return;
		}

		if (!Examinable(player, target))
		{
			notify(player, NOPERM_MESSAGE);
			XFREE(outbuf);
			XFREE(tbuf);
			return;
		}
	}
	else
	{
		target = NOTHING; /* indicates clear */
	}

	/*
	 * If the reference name starts with an underscore, it's global.
	 * * Only wizards can do that.
	 */
	tp = tbuf;

	if (*ref_name == '_')
	{
		if (!Wizard(player))
		{
			notify(player, NOPERM_MESSAGE);
			XFREE(outbuf);
			XFREE(tbuf);
			return;
		}
	}
	else
	{
		XSAFELTOS(tbuf, &tp, player, LBUF_SIZE);
		XSAFELBCHR('.', tbuf, &tp);
	}

	for (s = ref_name; *s; s++)
	{
		XSAFELBCHR(tolower(*s), tbuf, &tp);
	}

	*tp = '\0';
	/*
	 * Does this reference name exist already?
	 */
	np = (dbref *)hashfind(tbuf, &mushstate.nref_htab);

	if (np)
	{
		if (target == NOTHING)
		{
			XFREE(np);
			hashdelete(tbuf, &mushstate.nref_htab);
			notify(player, "Reference cleared.");
		}
		else if (*np == target)
		{
			/*
			 * Already got it.
			 */
			notify(player, "That reference has already been made.");
		}
		else
		{
			/*
			 * Replace it.
			 */
			XFREE(np);
			np = (dbref *)XMALLOC(sizeof(dbref), "np");
			*np = target;
			hashrepl(tbuf, np, &mushstate.nref_htab);
			notify(player, "Reference updated.");
		}
		XFREE(outbuf);
		XFREE(tbuf);
		return;
	}

	/*
	 * Didn't find it. We've got a new one (or an error if we have no
	 * * target but the reference didn't exist).
	 */

	if (target == NOTHING)
	{
		notify(player, "No such reference to clear.");
		XFREE(outbuf);
		XFREE(tbuf);
		return;
	}

	np = (dbref *)XMALLOC(sizeof(dbref), "np");
	*np = target;
	hashadd(tbuf, np, &mushstate.nref_htab, 0);
	notify(player, "Referenced.");
	XFREE(outbuf);
	XFREE(tbuf);
}
