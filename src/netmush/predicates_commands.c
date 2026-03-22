/**
 * @file predicates_commands.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command flow control and hook management
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

/* for lack of better place the @switch code is here */

void do_switch(dbref player, dbref cause, int key, char *expr, char *args[], int nargs, char *cargs[], int ncargs)
{
	int a, any, now;
	char *buff, *tbuf, *bp, *str;

	if (!expr || (nargs <= 0))
	{
		return;
	}

	now = key & SWITCH_NOW;
	key &= ~SWITCH_NOW;

	if (key == SWITCH_DEFAULT)
	{
		if (mushconf.switch_df_all)
		{
			key = SWITCH_ANY;
		}
		else
		{
			key = SWITCH_ONE;
		}
	}

	/*
	 * now try a wild card match of buff with stuff in args
	 */
	any = 0;

	/* Only allocate if we have arguments to process */
	if (nargs < 2 || !args[0] || !args[1])
	{
		/* No switch cases, check for default */
		if (nargs > 0 && args[0])
		{
			tbuf = replace_string(SWITCH_VAR, expr, args[0]);

			if (now)
			{
				process_cmdline(player, cause, tbuf, cargs, ncargs, NULL);
			}
			else
			{
				cque_wait_que(player, cause, 0, NOTHING, 0, tbuf, cargs, ncargs, mushstate.rdata);
			}

			XFREE(tbuf);
		}
		return;
	}

	buff = bp = XMALLOC(LBUF_SIZE, "bp");

	for (a = 0; (a < (nargs - 1)) && args[a] && args[a + 1]; a += 2)
	{
		bp = buff;
		str = args[a];
		eval_expression_string(buff, &bp, player, cause, cause, EV_FCHECK | EV_EVAL | EV_TOP, &str, cargs, ncargs);

		if (wild_match(buff, expr))
		{
			tbuf = replace_string(SWITCH_VAR, expr, args[a + 1]);

			if (now)
			{
				process_cmdline(player, cause, tbuf, cargs, ncargs, NULL);
			}
			else
			{
				cque_wait_que(player, cause, 0, NOTHING, 0, tbuf, cargs, ncargs, mushstate.rdata);
			}

			XFREE(tbuf);

			if (key == SWITCH_ONE)
			{
				XFREE(buff);
				return;
			}

			any = 1;
		}
	}

	XFREE(buff);

	if ((a < nargs) && !any && args[a])
	{
		tbuf = replace_string(SWITCH_VAR, expr, args[a]);

		if (now)
		{
			process_cmdline(player, cause, tbuf, cargs, ncargs, NULL);
		}
		else
		{
			cque_wait_que(player, cause, 0, NOTHING, 0, tbuf, cargs, ncargs, mushstate.rdata);
		}

		XFREE(tbuf);
	}
}

/* ---------------------------------------------------------------------------
 * do_end: Stop processing an action list, based on a conditional.
 */

void do_end(dbref player, dbref cause, int key, char *condstr, char *cmdstr, char *args[], int nargs)
{
	int k = key & ENDCMD_ASSERT;
	int n = xlate(condstr);

	if ((!k && n) || (k && !n))
	{
		mushstate.break_called = 1;

		if (cmdstr && *cmdstr)
		{
			cque_wait_que(player, cause, 0, NOTHING, 0, cmdstr, args, nargs, mushstate.rdata);
		}
	}
}

/* ---------------------------------------------------------------------------
 * Command hooks.
 */

void do_hook(dbref player, dbref cause, int key, char *cmdname, char *target)
{
	CMDENT *cmdp;
	char *p;
	ATTR *ap;
	HOOKENT *hp;
	dbref thing, aowner;
	int atr, aflags;

	for (p = cmdname; p && *p; p++)
	{
		*p = tolower(*p);
	}

	if (!cmdname || ((cmdp = (CMDENT *)hashfind(cmdname, &mushstate.command_htab)) == NULL) || (cmdp->callseq & CS_ADDED))
	{
		notify(player, "That is not a valid built-in command.");
		return;
	}

	if (key == 0)
	{
		/*
		 * List hooks only
		 */
		if (cmdp->pre_hook)
		{
			ap = atr_num(cmdp->pre_hook->atr);

			if (!ap)
			{
				notify(player, "Before Hook contains bad attribute number.");
			}
			else
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Before Hook: #%d/%s", cmdp->pre_hook->thing, ap->name);
			}
		}
		else
		{
			notify(player, "Before Hook: none");
		}

		if (cmdp->post_hook)
		{
			ap = atr_num(cmdp->post_hook->atr);

			if (!ap)
			{
				notify(player, "After Hook contains bad attribute number.");
			}
			else
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "After Hook: #%d/%s", cmdp->post_hook->thing, ap->name);
			}
		}
		else
		{
			notify(player, "After Hook: none");
		}

		if (cmdp->userperms)
		{
			ap = atr_num(cmdp->userperms->atr);

			if (!ap)
			{
				notify(player, "User Permissions contains bad attribute number.");
			}
			else
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "User Permissions: #%d/%s", cmdp->userperms->thing, ap->name);
			}
		}
		else
		{
			notify(player, "User Permissions: none");
		}

		return;
	}

	/*
	 * Check for the hook flags.
	 */

	if (key & HOOK_PRESERVE)
	{
		cmdp->callseq &= ~CS_PRIVATE;
		cmdp->callseq |= CS_PRESERVE;
		notify(player, "Hooks will preserve the state of the global registers.");
		return;
	}

	if (key & HOOK_NOPRESERVE)
	{
		cmdp->callseq &= ~(CS_PRESERVE | CS_PRIVATE);
		notify(player, "Hooks will not preserve the state of the global registers.");
		return;
	}

	if (key & HOOK_PRIVATE)
	{
		cmdp->callseq &= ~CS_PRESERVE;
		cmdp->callseq |= CS_PRIVATE;
		notify(player, "Hooks will use private global registers.");
		return;
	}

	/*
	 * If we didn't get a target, this is a hook deletion.
	 */

	if (!target || !*target)
	{
		if (key & HOOK_BEFORE)
		{
			if (cmdp->pre_hook)
			{
				XFREE(cmdp->pre_hook);
				cmdp->pre_hook = NULL;
			}

			notify(player, "Hook removed.");
		}
		else if (key & HOOK_AFTER)
		{
			if (cmdp->post_hook)
			{
				XFREE(cmdp->post_hook);
				cmdp->post_hook = NULL;
			}

			notify(player, "Hook removed.");
		}
		else if (key & HOOK_PERMIT)
		{
			if (cmdp->userperms)
			{
				XFREE(cmdp->userperms);
				cmdp->userperms = NULL;
			}

			notify(player, "User-defined permissions removed.");
		}
		else
		{
			notify(player, "Unknown command switch.");
		}

		return;
	}

	/*
	 * Find target object and attribute. Make sure it can be read, and
	 * * that we control the object.
	 */

	if (!parse_attrib(player, target, &thing, &atr, 0))
	{
		notify(player, NOMATCH_MESSAGE);
		return;
	}

	if (!Controls(player, thing))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	if (atr == NOTHING)
	{
		notify(player, "No such attribute.");
		return;
	}

	ap = atr_num(atr);

	if (!ap)
	{
		notify(player, "No such attribute.");
		return;
	}

	atr_get_info(thing, atr, &aowner, &aflags);

	if (!See_attr(player, thing, ap, aowner, aflags))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/*
	 * All right, we have what we need. Go allocate a hook.
	 */
	hp = (HOOKENT *)XMALLOC(sizeof(HOOKENT), "hp");
	hp->thing = thing;
	hp->atr = atr;

	/*
	 * If that kind of hook already existed, get rid of it. Put in the
	 * * new one.
	 */

	if (key & HOOK_BEFORE)
	{
		if (cmdp->pre_hook)
		{
			XFREE(cmdp->pre_hook);
		}

		cmdp->pre_hook = hp;
		notify(player, "Hook added.");
	}
	else if (key & HOOK_AFTER)
	{
		if (cmdp->post_hook)
		{
			XFREE(cmdp->post_hook);
		}

		cmdp->post_hook = hp;
		notify(player, "Hook added.");
	}
	else if (key & HOOK_PERMIT)
	{
		if (cmdp->userperms)
		{
			XFREE(cmdp->userperms);
		}

		cmdp->userperms = hp;
		notify(player, "User-defined permissions will now be checked.");
	}
	else
	{
		XFREE(hp);
		notify(player, "Unknown command switch.");
	}
}

/* ---------------------------------------------------------------------------
 * Command overriding and friends.
 */

void do_addcommand(dbref player, dbref cause, int key, char *name, char *command)
{
	CMDENT *old, *cmd;
	ADDENT *add, *nextp;
	dbref thing;
	int atr;
	char *s;
	char *s1;

	/*
	 * Sanity-check the command name and make it case-insensitive.
	 */

	if (!*name || (name[0] == '_' && name[1] == '_'))
	{
		notify(player, "That is not a valid command name.");
		return;
	}

	for (s = name; *s; s++)
	{
		if (isspace(*s) || (*s == C_ANSI_ESC))
		{
			notify(player, "That is not a valid command name.");
			return;
		}

		*s = tolower(*s);
	}

	if (!parse_attrib(player, command, &thing, &atr, 0) || (atr == NOTHING))
	{
		notify(player, "No such attribute.");
		return;
	}

	old = (CMDENT *)hashfind(name, &mushstate.command_htab);

	if (old && (old->callseq & CS_ADDED))
	{
		/*
		 * If it's already found in the hash table, and it's being
		 * added using the same object and attribute...
		 */
		for (nextp = (ADDENT *)old->info.added; nextp != NULL; nextp = nextp->next)
		{
			if ((nextp->thing == thing) && (nextp->atr == atr))
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s already added.", name);
				return;
			}
		}

		/*
		 * else tack it on to the existing entry...
		 */
		add = (ADDENT *)XMALLOC(sizeof(ADDENT), "add");
		add->thing = thing;
		add->atr = atr;
		add->name = XSTRDUP(name, "add->name");
		add->next = (ADDENT *)old->info.added;

		if (key & ADDCMD_PRESERVE)
		{
			old->callseq |= CS_ACTOR;
		}
		else
		{
			old->callseq &= ~CS_ACTOR;
		}

		old->info.added = add;
	}
	else
	{
		if (old)
		{
			/*
			 * Delete the old built-in
			 */
			hashdelete(name, &mushstate.command_htab);
		}

		cmd = (CMDENT *)XMALLOC(sizeof(CMDENT), "cmd");
		cmd->cmdname = XSTRDUP(name, "cmd->cmdname");
		cmd->switches = NULL;
		cmd->perms = 0;
		cmd->extra = 0;
		cmd->pre_hook = NULL;
		cmd->post_hook = NULL;
		cmd->userperms = NULL;
		cmd->callseq = CS_ADDED | CS_ONE_ARG | ((old && (old->callseq & CS_LEADIN)) ? CS_LEADIN : 0) | ((key & ADDCMD_PRESERVE) ? CS_ACTOR : 0);
		add = (ADDENT *)XMALLOC(sizeof(ADDENT), "add");
		add->thing = thing;
		add->atr = atr;
		add->name = XSTRDUP(name, "add->name");
		add->next = NULL;
		cmd->info.added = add;
		hashadd(cmd->cmdname, (int *)cmd, &mushstate.command_htab, 0);

		if (old)
		{
			/*
			 * If this command was the canonical form of the
			 * * command (not an alias), point its aliases to
			 * * the added command, while keeping the __ alias.
			 */
			if (!strcmp(name, old->cmdname))
			{
				s1 = XASPRINTF("s1", "__%s", old->cmdname);
				hashdelete(s1, &mushstate.command_htab);
				hashreplall((int *)old, (int *)cmd, &mushstate.command_htab);
				hashadd(s1, (int *)old, &mushstate.command_htab, 0);
				XFREE(s1);
			}
		}
	}

	/*
	 * We reset the one letter commands here so you can overload them
	 */
	reset_prefix_cmds();
	notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Command %s added.", name);
}

void do_listcommands(dbref player, dbref cause, int key, char *name)
{
	CMDENT *old;
	ADDENT *nextp;
	int didit = 0;
	char *s, *keyname;

	/*
	 * Let's make this case insensitive...
	 */

	for (s = name; *s; s++)
	{
		*s = tolower(*s);
	}

	if (*name)
	{
		old = (CMDENT *)hashfind(name, &mushstate.command_htab);

		if (old && (old->callseq & CS_ADDED))
		{
			if (strcmp(name, old->cmdname))
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: alias for %s", name, old->cmdname);
				return;
			}

			for (nextp = (ADDENT *)old->info.added; nextp != NULL; nextp = nextp->next)
			{
				ATTR *ap_check = (ATTR *)atr_num(nextp->atr);
				if (ap_check)
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/%s", nextp->name, nextp->thing, ap_check->name);
				}
				else
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/<invalid>", nextp->name, nextp->thing);
				}
			}
		}
		else
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s not found in command table.", name);
		}

		return;
	}
	else
	{
		for (keyname = hash_firstkey(&mushstate.command_htab); keyname != NULL; keyname = hash_nextkey(&mushstate.command_htab))
		{
			old = (CMDENT *)hashfind(keyname, &mushstate.command_htab);

			if (old && (old->callseq & CS_ADDED))
			{
				if (strcmp(keyname, old->cmdname))
				{
					notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: alias for %s", keyname, old->cmdname);
					continue;
				}

				for (nextp = (ADDENT *)old->info.added; nextp != NULL; nextp = nextp->next)
				{
					ATTR *ap_check = (ATTR *)atr_num(nextp->atr);
					if (ap_check)
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/%s", nextp->name, nextp->thing, ap_check->name);
					}
					else
					{
						notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/<invalid>", nextp->name, nextp->thing);
					}
					didit = 1;
				}
			}
		}
	}

	if (!didit)
	{
		notify(player, "No added commands found in command table.");
	}
}

void do_delcommand(dbref player, dbref cause, int key, char *name, char *command)
{
	CMDENT *old, *cmd;
	ADDENT *prev = NULL, *nextp;
	dbref thing;
	int atr;
	char *s;
	char *s1 = XMALLOC(MBUF_SIZE, "s1");

	if (!*name)
	{
		notify(player, "Sorry.");
		XFREE(s1);
		return;
	}

	if (*command)
	{
		if (!parse_attrib(player, command, &thing, &atr, 0) || (atr == NOTHING))
		{
			notify(player, "No such attribute.");
			XFREE(s1);
			return;
		}
	}

	/*
	 * Let's make this case insensitive...
	 */

	for (s = name; *s; s++)
	{
		*s = tolower(*s);
	}

	old = (CMDENT *)hashfind(name, &mushstate.command_htab);

	if (old && (old->callseq & CS_ADDED))
	{
		if (!*command)
		{
			for (prev = (ADDENT *)old->info.added; prev != NULL; prev = nextp)
			{
				nextp = prev->next;
				/*
				 * Delete it!
				 */
				XFREE(prev->name);
				XFREE(prev);
			}

			hashdelete(name, &mushstate.command_htab);
			XSNPRINTF(s1, MBUF_SIZE, "__%s", old->cmdname);

			if ((cmd = (CMDENT *)hashfind(s1, &mushstate.command_htab)) != NULL)
			{
				hashadd(cmd->cmdname, (int *)cmd, &mushstate.command_htab, 0);

				/*
				 * in case we deleted by alias
				 */
				if (strcmp(name, cmd->cmdname))
				{
					hashadd(name, (int *)cmd, &mushstate.command_htab, HASH_ALIAS);
				}

				/*
				 * the __ alias may have been temporarily
				 * * marked as the original hash entry
				 */
				XSNPRINTF(s1, MBUF_SIZE, "__%s", cmd->cmdname);
				hashdelete(s1, &mushstate.command_htab);
				hashadd(s1, (int *)cmd, &mushstate.command_htab, HASH_ALIAS);
				hashreplall((int *)old, (int *)cmd, &mushstate.command_htab);
			}
			else
			{
				hashdelall((int *)old, &mushstate.command_htab);
			}

			XFREE(old->cmdname);
			XFREE(old);
			reset_prefix_cmds();
			notify(player, "Done");
			XFREE(s1);
			return;
		}
		else
		{
			for (nextp = (ADDENT *)old->info.added; nextp != NULL; nextp = nextp->next)
			{
				if ((nextp->thing == thing) && (nextp->atr == atr))
				{
					/*
					 * Delete it!
					 */
					XFREE(nextp->name);

					if (!prev)
					{
						if (!nextp->next)
						{
							hashdelete(name, &mushstate.command_htab);
							XSNPRINTF(s1, MBUF_SIZE, "__%s", name);

							if ((cmd = (CMDENT *)hashfind(s1, &mushstate.command_htab)) != NULL)
							{
								hashadd(cmd->cmdname, (int *)cmd, &mushstate.command_htab, 0);

								/*
								 * in case we deleted by alias
								 */
								if (strcmp(name, cmd->cmdname))
								{
									hashadd(name, (int *)cmd, &mushstate.command_htab, HASH_ALIAS);
								}

								/*
								 * the __ alias may have been temporarily
								 * * marked as the original hash entry
								 */
								XSNPRINTF(s1, MBUF_SIZE, "__%s", cmd->cmdname);
								hashdelete(s1, &mushstate.command_htab);
								hashadd(s1, (int *)cmd, &mushstate.command_htab, HASH_ALIAS);
								hashreplall((int *)old, (int *)cmd, &mushstate.command_htab);
							}
							else
							{
								hashdelall((int *)old, &mushstate.command_htab);
							}

							XFREE(old->cmdname);
							XFREE(old);
						}
						else
						{
							old->info.added = nextp->next;
							XFREE(nextp);
						}
					}
					else
					{
						prev->next = nextp->next;
						XFREE(nextp);
					}

					reset_prefix_cmds();
					notify(player, "Done");
					XFREE(s1);
					return;
				}

				prev = nextp;
			}

			notify(player, "Command not found in command table.");
		}
	}
	else
	{
		notify(player, "Command not found in command table.");
	}
	XFREE(s1);
}
