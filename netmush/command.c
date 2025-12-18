/**
 * @file command.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command parser and support routines
 * @version 3.3
 * @date 2020-12-25
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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
#include <string.h>
#include <pcre.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <libgen.h>

void (*handler_cs_no_args)(dbref, dbref, int);
void (*handler_cs_one_args)(dbref, dbref, int, char *);
void (*handler_cs_one_args_unparse)(dbref, char *);
void (*handler_cs_one_args_cmdargs)(dbref, dbref, int, char *, char *[], int);
void (*handler_cs_two_args)(dbref, dbref, int, char *, char *);
void (*handler_cs_two_args_cmdargs)(dbref, dbref, int, char *, char *, char *[], int);
void (*handler_cs_two_args_argv)(dbref, dbref, int, char *, char *[], int);
void (*handler_cs_two_args_cmdargs_argv)(dbref, dbref, int, char *, char *[], int, char *[], int);

CMDENT *prefix_cmds[256];
CMDENT *goto_cmdp, *enter_cmdp, *leave_cmdp, *internalgoto_cmdp;

/**
 * @brief Initialize the command table.
 *
 */
void init_cmdtab(void)
{
	CMDENT *cp;
	ATTR *ap;
	int i = 0;
	char *p, *q, *s;
	char *cbuff = XMALLOC(SBUF_SIZE, "cbuff");

	hashinit(&mushstate.command_htab, 250 * mushconf.hash_factor, HT_STR);

	/**
	 * Load attribute-setting commands
	 *
	 */
	for (ap = attr; ap->name; ap++)
	{
		if ((ap->flags & AF_NOCMD) == 0)
		{
			p = cbuff;
			*p++ = '@';

			for (q = (char *)ap->name; *q; p++, q++)
			{
				*p = tolower(*q);
			}

			*p = '\0';
			cp = (CMDENT *)XMALLOC(sizeof(CMDENT), "cp");
			cp->cmdname = XSTRDUP(cbuff, "cp->cmdname");
			cp->perms = CA_NO_GUEST | CA_NO_SLAVE;
			cp->switches = NULL;

			if (ap->flags & (AF_WIZARD | AF_MDARK))
			{
				cp->perms |= CA_WIZARD;
			}

			cp->extra = ap->number;
			cp->callseq = CS_TWO_ARG;
			cp->pre_hook = NULL;
			cp->post_hook = NULL;
			cp->userperms = NULL;
			cp->info.handler = do_setattr;

			if (hashadd(cp->cmdname, (int *)cp, &mushstate.command_htab, 0))
			{
				XFREE(cp->cmdname);
				XFREE(cp);
			}
			else
			{
				/**
				 * also add the __ alias form
				 *
				 */
				s = XASPRINTF("s", "__%s", cp->cmdname);
				hashadd(s, (int *)cp, &mushstate.command_htab, HASH_ALIAS);
				XFREE(s);
			}
		}
	}

	XFREE(cbuff);

	/**
	 * Load the builtin commands, plus __ aliases
	 *
	 */
	for (cp = command_table; cp->cmdname; cp++)
	{
		hashadd(cp->cmdname, (int *)cp, &mushstate.command_htab, 0);
		s = XASPRINTF("s", "__%s", cp->cmdname);
		hashadd(s, (int *)cp, &mushstate.command_htab, HASH_ALIAS);
		XFREE(s);
	}

	/**
	 * Set the builtin prefix commands
	 *
	 */
	for (i = 0; i < 256; i++)
	{
		prefix_cmds[i] = NULL;
	}

	register_prefix_cmds("\":;\\#&"); /* ":;\#&  */
	goto_cmdp = (CMDENT *)hashfind("goto", &mushstate.command_htab);
	enter_cmdp = (CMDENT *)hashfind("enter", &mushstate.command_htab);
	leave_cmdp = (CMDENT *)hashfind("leave", &mushstate.command_htab);
	internalgoto_cmdp = (CMDENT *)hashfind("internalgoto", &mushstate.command_htab);
}

/**
 * @brief Reset prefix's commands
 *
 */
void reset_prefix_cmds(void)
{
	int i = 0;
	char *cn = XSTRDUP("x", "cn");

	for (i = 0; i < 256; i++)
	{
		if (prefix_cmds[i])
		{
			cn[0] = i;
			prefix_cmds[i] = (CMDENT *)hashfind(cn, &mushstate.command_htab);
		}
	}

	XFREE(cn);
}

/**
 * @brief Check if player has access to function. Note that the calling function
 * may also give permission denied messages on failure.
 *
 * @param player	Player doing the command
 * @param mask		Permission's mask
 * @return bool		Permission granted or not
 */
bool check_access(dbref player, int mask)
{
	int mval = 0, nval = 0;

	/**
	 * Check if we have permission to execute
	 *
	 */
	if (mask & (CA_DISABLED | CA_STATIC))
	{
		return false;
	}

	if (God(player) || mushstate.initializing)
	{
		return true;
	}

	/**
	 * Check for bits that we have to have. Since we know that we're not
	 * God at this point, if it is God-only, it fails. (God in combination with
	 * other stuff is implicitly checked, since we return false if we don't find
	 * the other bits.)
	 *
	 */
	if ((mval = mask & (CA_ISPRIV_MASK | CA_MARKER_MASK)) == CA_GOD)
	{
		return false;
	}

	if (mval)
	{
		mval = mask & CA_ISPRIV_MASK;
		nval = mask & CA_MARKER_MASK;

		if (mval && !nval)
		{
			if (!(((mask & CA_WIZARD) && Wizard(player)) || ((mask & CA_ADMIN) && WizRoy(player)) || ((mask & CA_BUILDER) && Builder(player)) || ((mask & CA_STAFF) && Staff(player)) || ((mask & CA_HEAD) && Head(player)) || ((mask & CA_IMMORTAL) && Immortal(player)) || ((mask & CA_MODULE_OK) && Can_Use_Module(player))))
			{
				return false;
			}
		}
		else if (!mval && nval)
		{
			if (!(((mask & CA_MARKER0) && H_Marker0(player)) || ((mask & CA_MARKER1) && H_Marker1(player)) || ((mask & CA_MARKER2) && H_Marker2(player)) || ((mask & CA_MARKER3) && H_Marker3(player)) || ((mask & CA_MARKER4) && H_Marker4(player)) || ((mask & CA_MARKER5) && H_Marker5(player)) || ((mask & CA_MARKER6) && H_Marker6(player)) || ((mask & CA_MARKER7) && H_Marker7(player)) || ((mask & CA_MARKER8) && H_Marker8(player)) || ((mask & CA_MARKER9) && H_Marker9(player))))
			{
				return false;
			}
		}
		else
		{
			if (!(((mask & CA_WIZARD) && Wizard(player)) || ((mask & CA_ADMIN) && WizRoy(player)) || ((mask & CA_BUILDER) && Builder(player)) || ((mask & CA_STAFF) && Staff(player)) || ((mask & CA_HEAD) && Head(player)) || ((mask & CA_IMMORTAL) && Immortal(player)) || ((mask & CA_MODULE_OK) && Can_Use_Module(player)) || ((mask & CA_MARKER0) && H_Marker0(player)) || ((mask & CA_MARKER1) && H_Marker1(player)) || ((mask & CA_MARKER2) && H_Marker2(player)) || ((mask & CA_MARKER3) && H_Marker3(player)) || ((mask & CA_MARKER4) && H_Marker4(player)) || ((mask & CA_MARKER5) && H_Marker5(player)) || ((mask & CA_MARKER6) && H_Marker6(player)) || ((mask & CA_MARKER7) && H_Marker7(player)) || ((mask & CA_MARKER8) && H_Marker8(player)) || ((mask & CA_MARKER9) && H_Marker9(player))))
			{
				return false;
			}
		}
	}
	/**
	 * Check the things that we can't be.
	 *
	 */
	if (((mask & CA_ISNOT_MASK) && !Wizard(player)) && (((mask & CA_NO_HAVEN) && Player_haven(player)) || ((mask & CA_NO_ROBOT) && Robot(player)) || ((mask & CA_NO_SLAVE) && Slave(player)) || ((mask & CA_NO_SUSPECT) && Suspect(player)) || ((mask & CA_NO_GUEST) && Guest(player))))
	{
		return false;
	}

	return true;
}

/**
 * @brief Go through sequence of module call-outs, treating all of them like permission checks.
 *
 * @param player	Player doing the command
 * @param xperms	Extended functions list
 * @return bool		Permission granted or not
 *
 */
bool check_mod_access(dbref player, EXTFUNCS *xperms)
{
	for (int i = 0; i < xperms->num_funcs; i++)
	{
		if (!xperms->ext_funcs[i])
		{
			continue;
		}

		if (!((xperms->ext_funcs[i]->handler)(player)))
		{
			return false;
		}
	}

	return true;
}

/**
 * @brief Check if user has access to command with user-def'd permissions.
 *
 * @param player	Player doing the command
 * @param hookp		Hook entry point
 * @param cargs		Command argument list
 * @param ncargs	Number of arguments
 * @return bool		Permission granted or not
 */
bool check_userdef_access(dbref player, HOOKENT *hookp, char *cargs[], int ncargs)
{
	char *buf, *bp, *tstr, *str;
	int result = 0, aflags = 0, alen = 0;
	dbref aowner;
	GDATA *preserve;

	/**
	 * We have user-defined command permissions. Go evaluate the obj/attr
	 * pair that we've been given. If that result is nonexistent, we consider it
	 * a failure. We use boolean truth here.
	 *
	 * Note that unlike before and after hooks, we always preserve the registers.
	 * (When you get right down to it, this thing isn't really a hook. It's just
	 * convenient to re-use the same code that we use with hooks.)
	 *
	 */
	tstr = atr_get(hookp->thing, hookp->atr, &aowner, &aflags, &alen);

	if (!tstr)
	{
		return false;
	}

	if (!*tstr)
	{
		XFREE(tstr);
		return false;
	}

	str = tstr;
	preserve = save_global_regs("check_userdef_access");
	bp = buf = XMALLOC(LBUF_SIZE, "buf");
	eval_expression_string(buf, &bp, hookp->thing, player, player, EV_EVAL | EV_FCHECK | EV_TOP, &str, cargs, ncargs);
	restore_global_regs("check_userdef_access", preserve);
	result = xlate(buf);
	XFREE(buf);
	XFREE(tstr);
	return result;
}

/**
 * @brief Evaluate a hook.
 *
 * @param hp			Hook Entry
 * @param save_globs	Save globals?
 * @param player		Player being evaluated
 * @param cause			Cause of the evaluation
 * @param cargs			Command arguments
 * @param ncargs		Number of arguments
 */
void process_hook(HOOKENT *hp, int save_globs, dbref player, dbref cause __attribute__((unused)), char *cargs[], int ncargs)
{
	char *buf = NULL, *bp = NULL, *tstr = NULL, *str = NULL;
	int aflags = 0, alen = 0;
	dbref aowner = NOTHING;
	GDATA *preserve = NULL;

	str = tstr = atr_get(hp->thing, hp->atr, &aowner, &aflags, &alen);

	/**
	 * We know we have a non-null hook. We want to evaluate the obj/attr
	 * pair of that hook. We consider the enactor to be the player who executed
	 * the command that caused this hook to be called.
	 *
	 */
	if (save_globs & CS_PRESERVE)
	{
		preserve = save_global_regs("process_hook");
	}
	else if (save_globs & CS_PRIVATE)
	{
		preserve = mushstate.rdata;
		mushstate.rdata = NULL;
	}

	buf = bp = XMALLOC(LBUF_SIZE, "buf");
	eval_expression_string(buf, &bp, hp->thing, player, player, EV_EVAL | EV_FCHECK | EV_TOP, &str, cargs, ncargs);
	XFREE(buf);

	if (save_globs & CS_PRESERVE)
	{
		restore_global_regs("process_hook", preserve);
	}
	else if (save_globs & CS_PRIVATE)
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

		mushstate.rdata = preserve;
	}

	XFREE(tstr);
}

/**
 * @brief Call the hooks before and after leaving a room
 *
 * @param player	DBref of Player leaving
 * @param cause		DBref of what caused the action
 * @param state		True: before, False: after
 */
void call_move_hook(dbref player, dbref cause, bool state)
{
	if (internalgoto_cmdp)
	{
		if (!state) /* before move */
		{
			if ((internalgoto_cmdp->pre_hook != NULL) && !(internalgoto_cmdp->callseq & CS_ADDED))
			{
				process_hook(internalgoto_cmdp->pre_hook, internalgoto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, NULL, 0);
			}
		}
		else /* after move */
		{
			if ((internalgoto_cmdp->post_hook != NULL) && !(internalgoto_cmdp->callseq & CS_ADDED))
			{
				process_hook(internalgoto_cmdp->post_hook, internalgoto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, NULL, 0);
			}
		}
	}
}

/**
 * @brief Check if user has access to command
 *
 * @param player	DBref of playuer
 * @param cmdp		Command entry
 * @param cargs		Command Arguments
 * @param ncargs	Number of arguments
 * @return Bool		User has access or not
 */
bool check_cmd_access(dbref player, CMDENT *cmdp, char *cargs[], int ncargs)
{
	return check_access(player, cmdp->perms) && (!cmdp->userperms || check_userdef_access(player, cmdp->userperms, cargs, ncargs) || God(player)) ? true : false;
}

/**
 * @brief Perform indicated command with passed args.
 *
 * @param cmdp			Command
 * @param switchp		Switches
 * @param player		DBref of player doing the command
 * @param cause			DBref of what caused the action
 * @param interactive	Is the command interractive?
 * @param arg			Raw Arguments
 * @param unp_command	Raw Commands
 * @param cargs			Arguments
 * @param ncargs		Number of arguments
 */
void process_cmdent(CMDENT *cmdp, char *switchp, dbref player, dbref cause, bool interactive, char *arg, char *unp_command, char *cargs[], int ncargs)
{
	int nargs = 0, i = 0, interp = 0, key = 0, xkey = 0, aflags = 0, alen = 0;
	int cmd_matches = 0;
	char *buf1 = NULL, *buf2 = NULL, *bp = NULL, *str = NULL, *buff = NULL;
	char *s = NULL, *j = NULL, *new = NULL, *pname = NULL, *lname = NULL;
	char *args[mushconf.max_command_args], *aargs[NUM_ENV_VARS];
	char tchar = 0;
	dbref aowner = NOTHING;
	ADDENT *add = NULL;
	GDATA *preserve = NULL;
	/**
	 * Validate arguments count to prevent buffer overflow
	 *
	 */
	if (ncargs < 0 || ncargs > NUM_ENV_VARS)
	{
		return;
	}

	/**
	 * Perform object type checks.
	 *
	 */
	if (Invalid_Objtype(player))
	{
		notify(player, "Command incompatible with invoker type.");
		return;
	}

	/**
	 * Check if we have permission to execute the command
	 *
	 */

	if (!check_cmd_access(player, cmdp, cargs, ncargs))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/**
	 * Check global flags
	 *
	 */
	if ((!Builder(player)) && Protect(CA_GBL_BUILD) && !(mushconf.control_flags & CF_BUILD))
	{
		notify(player, "Sorry, building is not allowed now.");
		return;
	}

	if (Protect(CA_GBL_INTERP) && !(mushconf.control_flags & CF_INTERP))
	{
		notify(player, "Sorry, queueing and triggering are not allowed now.");
		return;
	}

	key = cmdp->extra & ~SW_MULTIPLE;

	if (key & SW_GOT_UNIQUE)
	{
		i = 1;
		key = key & ~SW_GOT_UNIQUE;
	}
	else
	{
		i = 0;
	}

	/**
	 * Check command switches.  Note that there may be more than one, and
	 * that we OR all of them together along with the extra value from the command
	 * table to produce the key value in the handler call.
	 *
	 */
	if (switchp && cmdp->switches)
	{
		do
		{
			buf1 = strchr(switchp, '/');

			if (buf1)
			{
				*buf1++ = '\0';
			}

			xkey = search_nametab(player, cmdp->switches, switchp);

			if (xkey == -1)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unrecognized switch '%s' for command '%s'.", switchp, cmdp->cmdname);
				return;
			}
			else if (xkey == -2)
			{
				notify(player, NOPERM_MESSAGE);
				return;
			}
			else if (!(xkey & SW_MULTIPLE))
			{
				if (i == 1)
				{
					notify(player, "Illegal combination of switches.");
					return;
				}

				i = 1;
			}
			else
			{
				xkey &= ~SW_MULTIPLE;
			}

			key |= xkey;
			switchp = buf1;
		} while (buf1);
	}
	else if (switchp && !(cmdp->callseq & CS_ADDED))
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Command %s does not take switches.", cmdp->cmdname);
		return;
	}

	/**
	 * At this point we're guaranteed we're going to execute something.
	 * Let's check to see if we have a pre-command hook.
	 */
	if (((cmdp)->pre_hook != NULL) && !((cmdp)->callseq & CS_ADDED))
	{
		process_hook((cmdp)->pre_hook, (cmdp)->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, (cargs), (ncargs));
	}

	/**
	 * If the command normally has interpreted args, but the user
	 * specified, /noeval, just do EV_STRIP.
	 *
	 * If the command is interpreted, or we're interactive (and the command
	 * isn't specified CS_NOINTERP), eval the args.
	 *
	 * The others are obvious.
	 *
	 */
	if ((cmdp->callseq & CS_INTERP) && (key & SW_NOEVAL))
	{
		interp = EV_STRIP;
		key &= ~SW_NOEVAL; /* Remove SW_NOEVAL from 'key' */
	}
	else if ((cmdp->callseq & CS_INTERP) || !(interactive || (cmdp->callseq & CS_NOINTERP)))
	{
		interp = EV_EVAL | EV_STRIP;
	}
	else if (cmdp->callseq & CS_STRIP)
	{
		interp = EV_STRIP;
	}
	else if (cmdp->callseq & CS_STRIP_AROUND)
	{
		interp = EV_STRIP_AROUND;
	}
	else
	{
		interp = 0;
	}

	switch (cmdp->callseq & CS_NARG_MASK)
	{
	case CS_NO_ARGS: /*!< <cmd> (no args) */
	{
		handler_cs_no_args = cmdp->info.handler;
		(*(handler_cs_no_args))(player, cause, key);
	}
	//(*(cmdp->info.handler))(player, cause, key);
	break;

	case CS_ONE_ARG: /*!< <cmd> <arg> */
		/**
		 * If an unparsed command, just give it to the handler
		 *
		 */
		if (cmdp->callseq & CS_UNPARSE)
		{
			handler_cs_one_args_unparse = cmdp->info.handler;
			(*(handler_cs_one_args_unparse))(player, unp_command);
			break;
		}
		/**
		 * Interpret if necessary, but not twice for CS_ADDED
		 *
		 */
		if ((interp & EV_EVAL) && !(cmdp->callseq & CS_ADDED))
		{
			buf1 = bp = XMALLOC(LBUF_SIZE, "buf1");
			str = arg;
			eval_expression_string(buf1, &bp, player, cause, cause, interp | EV_FCHECK | EV_TOP, &str, cargs, ncargs);
		}
		else
		{
			buf1 = parse_to(&arg, '\0', interp | EV_TOP);
		}
		/**
		 * Call the correct handler
		 *
		 */
		if (cmdp->callseq & CS_CMDARG)
		{
			handler_cs_one_args_cmdargs = cmdp->info.handler;
			(*(handler_cs_one_args_cmdargs))(player, cause, key, buf1, cargs, ncargs);
		}
		else
		{
			if (cmdp->callseq & CS_ADDED)
			{
				preserve = save_global_regs("process_cmdent_added");
				/**
				 * Construct the matching buffer.
				 *
				 * In the case of a single-letter prefix, we want to just skip
				 * past that first letter. want to just skip past that first
				 * letter. Otherwise we want to go past the first word.
				 *
				 */
				if (!(cmdp->callseq & CS_LEADIN))
				{
					for (j = unp_command; *j && (*j != ' '); j++)
						;
				}
				else
				{
					j = unp_command;
					j++;
				}

				new = XMALLOC(LBUF_SIZE, "new");
				bp = new;

				if (!*j)
				{
					/**
					 * No args
					 *
					 */
					if (!(cmdp->callseq & CS_LEADIN))
					{
						XSAFELBSTR(cmdp->cmdname, new, &bp);
					}
					else
					{
						XSAFELBSTR(unp_command, new, &bp);
					}

					if (switchp)
					{
						XSAFELBCHR('/', new, &bp);
						XSAFELBSTR(switchp, new, &bp);
					}

					*bp = '\0';
				}
				else
				{
					if (!(cmdp->callseq & CS_LEADIN))
					{
						j++;
					}

					XSAFELBSTR(cmdp->cmdname, new, &bp);

					if (switchp)
					{
						XSAFELBCHR('/', new, &bp);
						XSAFELBSTR(switchp, new, &bp);
					}

					if (!(cmdp->callseq & CS_LEADIN))
					{
						XSAFELBCHR(' ', new, &bp);
					}

					XSAFELBSTR(j, new, &bp);
					*bp = '\0';
				}
				/**
				 * Now search against the attributes, unless we can't
				 * pass the uselock.
				 *
				 */
				for (add = (ADDENT *)cmdp->info.added; add != NULL; add = add->next)
				{
					buff = atr_get(add->thing, add->atr, &aowner, &aflags, &alen);
					/**
					 * Skip the '$' character, and the next
					 *
					 */
					for (s = buff + 2; *s && ((*s != ':') || (*(s - 1) == '\\')); s++)
						;

					if (!*s)
					{
						XFREE(buff);
						break;
					}

					*s++ = '\0';

					if (((!(aflags & AF_REGEXP) && wild(buff + 1, new, aargs, NUM_ENV_VARS)) || ((aflags & AF_REGEXP) && regexp_match(buff + 1, new, ((aflags & AF_CASE) ? 0 : PCRE_CASELESS), aargs, NUM_ENV_VARS))) && (!mushconf.addcmd_obey_uselocks || could_doit(player, add->thing, A_LUSE)))
					{
						process_cmdline(((!(cmdp->callseq & CS_ACTOR) || God(player)) ? add->thing : player), player, s, aargs, NUM_ENV_VARS, NULL);

						for (i = 0; i < NUM_ENV_VARS; i++)
						{
							if (aargs[i])
								XFREE(aargs[i]);
						}

						cmd_matches++;
					}

					XFREE(buff);

					if (cmd_matches && mushconf.addcmd_obey_stop && Stop_Match(add->thing))
					{
						break;
					}
				}

				if (!cmd_matches && !mushconf.addcmd_match_blindly)
				{
					/**
					 * The command the player typed didn't match any of
					 * the wildcard patterns we have for that addcommand. We
					 * should raise an error. We DO NOT go back into trying to
					 * match other stuff -- this is a 'Huh?' situation.
					 *
					 */
					notify(player, mushconf.huh_msg);
					pname = log_getname(player);

					if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
					{
						lname = log_getname(Location(player));
						log_write(LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s", pname, lname, new);
						XFREE(lname);
					}
					else
					{
						log_write(LOG_BADCOMMANDS, "CMD", "BAD", "%s entered: %s", pname, new);
					}

					XFREE(pname);
				}

				XFREE(new);
				restore_global_regs("process_cmdent", preserve);
			}
			else
			{
				handler_cs_one_args = cmdp->info.handler;
				(*handler_cs_one_args)(player, cause, key, buf1);
			}
		}
		/**
		 * Free the buffer if one was allocated
		 *
		 */
		if ((interp & EV_EVAL) && !(cmdp->callseq & CS_ADDED))
		{
			XFREE(buf1);
		}

		break;

	case CS_TWO_ARG: /* <cmd> <arg1> = <arg2> */
		/**
		 * Interpret ARG1
		 *
		 */
		buf2 = parse_to(&arg, '=', EV_STRIP_TS);
		/**
		 * Handle when no '=' was specified
		 *
		 */
		if (!arg || (arg && !*arg))
		{
			arg = &tchar;
			*arg = '\0';
		}

		buf1 = bp = XMALLOC(LBUF_SIZE, "buf1");
		str = buf2;
		eval_expression_string(buf1, &bp, player, cause, cause, EV_STRIP | EV_FCHECK | EV_EVAL | EV_TOP, &str, cargs, ncargs);

		if (cmdp->callseq & CS_ARGV)
		{
			/**
			 * Arg2 is ARGV style. Go get the args
			 *
			 */
			parse_arglist(player, cause, cause, arg, '\0', interp | EV_STRIP_LS | EV_STRIP_TS, args, mushconf.max_command_args, cargs, ncargs);

			for (nargs = 0; (nargs < mushconf.max_command_args) && args[nargs]; nargs++)
				;
			/**
			 * Call the correct command handler
			 *
			 */
			if (cmdp->callseq & CS_CMDARG)
			{
				handler_cs_two_args_cmdargs_argv = cmdp->info.handler;
				(*handler_cs_two_args_cmdargs_argv)(player, cause, key, buf1, args, nargs, cargs, ncargs);
			}
			else
			{
				handler_cs_two_args_argv = cmdp->info.handler;
				(*handler_cs_two_args_argv)(player, cause, key, buf1, args, nargs);
			}
			/**
			 * Free the argument buffers
			 *
			 */
			for (i = 0; i < nargs; i++)
				if (args[i])
				{
					XFREE(args[i]);
				}
		}
		else
		{
			/**
			 * Arg2 is normal style. Interpret if needed
			 *
			 */
			if (interp & EV_EVAL)
			{
				buf2 = bp = XMALLOC(LBUF_SIZE, "buf2");
				str = arg;
				eval_expression_string(buf2, &bp, player, cause, cause, interp | EV_FCHECK | EV_TOP, &str, cargs, ncargs);
			}
			else if (cmdp->callseq & CS_UNPARSE)
			{
				buf2 = parse_to(&arg, '\0', interp | EV_TOP | EV_NO_COMPRESS);
			}
			else
			{
				buf2 = parse_to(&arg, '\0', interp | EV_STRIP_LS | EV_STRIP_TS | EV_TOP);
			}
			/**
			 * Call the correct command handler
			 *
			 */
			if (cmdp->callseq & CS_CMDARG)
			{
				handler_cs_two_args_cmdargs = cmdp->info.handler;
				(*handler_cs_two_args_cmdargs)(player, cause, key, buf1, buf2, cargs, ncargs);
			}
			else
			{
				handler_cs_two_args = cmdp->info.handler;
				(*handler_cs_two_args)(player, cause, key, buf1, buf2);
			}
			/**
			 * Free the buffer, if needed
			 *
			 */
			if (interp & EV_EVAL)
			{
				XFREE(buf2);
			}
		}
		/**
		 * Free the buffer obtained by evaluating Arg1
		 *
		 */
		XFREE(buf1);
		break;
	}
	/**
	 * And now we go do the posthook, if we have one.
	 *
	 */
	if ((cmdp->post_hook != NULL) && !(cmdp->callseq & CS_ADDED))
	{
		process_hook(cmdp->post_hook, cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, cargs, ncargs);
	}
	return;
}

/**
 * @brief Execute a command.
 *
 * @param player		DBref of player doing the command
 * @param cause			DBref of what caused the action
 * @param interactive	Is the command interractive?
 * @param command		Command
 * @param args			Arguments
 * @param nargs			Number of arguments
 * @return char*
 */
char *process_command(dbref player, dbref cause, int interactive, char *command, char *args[], int nargs)
{

	dbref exit = NOTHING, aowner = NOTHING, parent = NOTHING;
	CMDENT *cmdp = NULL;
	MODULE *csm__mp = NULL;
	NUMBERTAB *np = NULL;
	int succ = 0, aflags = 0, alen = 0, i = 0, got_stop = 0, pcount = 0, retval = 0;
	char *p = NULL, *q = NULL, *arg = NULL, *lcbuf = NULL, *slashp = NULL, *cmdsave = NULL, *bp = NULL;
	char *str = NULL, *evcmd = NULL, *gbuf = NULL, *gc = NULL, *pname = NULL, *lname = NULL;
	char *preserve_cmd = XMALLOC(LBUF_SIZE, "preserve_cmd");

	if (mushstate.cmd_invk_ctr == mushconf.cmd_invk_lim)
	{
		return command;
	}

	mushstate.cmd_invk_ctr++;

	/**
	 * Robustify player
	 *
	 */
	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = XSTRDUP("< process_command >", "mushstate.debug_cmd");

	if (!command)
	{
		log_write_raw(1, "ABORT! command.c, null command in process_command().\n");
		abort();
	}

	if (!Good_obj(player))
	{
		log_write(LOG_BUGS, "CMD", "PLYR", "Bad player in process_command: %d", player);
		mushstate.debug_cmd = cmdsave;
		return command;
	}

	/**
	 * Make sure player isn't going or halted
	 *
	 */
	if (Going(player) || (Halted(player) && !((Typeof(player) == TYPE_PLAYER) && interactive)))
	{
		notify_check(Owner(player), Owner(player), MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Attempt to execute command by halted object #%d", player);
		mushstate.debug_cmd = cmdsave;
		return command;
	}

	pname = log_getname(player);

	if (Suspect(player))
	{
		if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
		{
			lname = log_getname(Location(player));
			log_write(LOG_SUSPECTCMDS, "CMD", "SUSP", "%s in %s entered: %s", pname, lname, command);
			XFREE(lname);
		}
		else
		{
			log_write(LOG_SUSPECTCMDS, "CMD", "SUSP", "%s entered: %s", pname, command);
		}
	}
	else
	{
		if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
		{
			lname = log_getname(Location(player));
			log_write(LOG_SUSPECTCMDS, "CMD", "ALL", "%s in %s entered: %s", pname, lname, command);
			XFREE(lname);
		}
		else
		{
			log_write(LOG_SUSPECTCMDS, "CMD", "ALL", "%s entered: %s", pname, command);
		}
	}

	XFREE(pname);
	s_Accessed(player);

	/**
	 * Reset recursion and other limits. Baseline the CPU counter.
	 *
	 */
	mushstate.func_nest_lev = 0;
	mushstate.func_invk_ctr = 0;
	mushstate.f_limitmask = 0;
	mushstate.ntfy_nest_lev = 0;
	mushstate.lock_nest_lev = 0;

	if (mushconf.func_cpu_lim > 0)
	{
		mushstate.cputime_base = clock();
	}

	if (Verbose(player))
	{
		if (H_Redirect(player))
		{
			np = (NUMBERTAB *)nhashfind(player, &mushstate.redir_htab);

			if (np)
			{
				notify_check(np->num, np->num, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s] %s", Name(player), command);
			}
			else
			{
				/**
				 * We have no pointer, we should have no flags.
				 *
				 */
				s_Flags3(player, Flags3(player) & ~HAS_REDIRECT);
			}
		}
		else
		{
			notify_check(Owner(player), Owner(player), MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s] %s", Name(player), command);
		}
	}

	/**
	 * @warning NOTE THAT THIS WILL BREAK IF "GOD" IS NOT A DBREF.
	 *
	 */
	if (mushconf.control_flags & CF_GODMONITOR)
	{
		raw_notify(GOD, "%s(#%d)%c %s", Name(player), player, (interactive) ? '|' : ':', command);
	}

	/**
	 * Eat leading whitespace, and space-compress if configured
	 *
	 */
	while (*command && isspace(*command))
	{
		command++;
	}

	XSTRCPY(preserve_cmd, command);
	mushstate.debug_cmd = command;
	mushstate.curr_cmd = preserve_cmd;

	if (mushconf.space_compress)
	{
		p = q = command;

		while (*p)
		{
			while (*p && !isspace(*p))
			{
				*q++ = *p++;
			}

			while (*p && isspace(*p))
			{
				p++;
			}

			if (*p)
			{
				*q++ = ' ';
			}
		}

		*q = '\0';
	}

	/**
	 * Allow modules to intercept command strings.
	 *
	 */
	for (csm__mp = mushstate.modules_list, retval = 0; (csm__mp != NULL) && !retval; csm__mp = csm__mp->next)
	{
		if (csm__mp->process_command)
		{
			retval = (*(csm__mp->process_command))(player, cause, interactive, command, args, nargs);
		}
	}

	if (retval > 0)
	{
		mushstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

	/**
	 * Now comes the fun stuff.  First check for single-letter leadins.
	 * We check these before checking HOME because they are among the most
	 * frequently executed commands, and they can never be the HOME command.
	 */
	i = command[0] & 0xff;

	if ((prefix_cmds[i] != NULL) && command[0])
	{
		process_cmdent(prefix_cmds[i], NULL, player, cause, interactive, command, command, args, nargs);
		mushstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

	/**
	 * Check for the HOME command. You cannot do hooks on this because
	 * home is not part of the traditional command table.
	 */
	if (Has_location(player) && string_compare(command, "home") == 0)
	{
		if (((Fixed(player)) || (Fixed(Owner(player)))) && !(WizRoy(player)))
		{
			notify(player, mushconf.fixed_home_msg);
			mushstate.debug_cmd = cmdsave;
			return preserve_cmd;
		}

		do_move(player, cause, 0, "home");
		mushstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

	/**
	 * Only check for exits if we may use the goto command
	 *
	 */
	if (check_cmd_access(player, goto_cmdp, args, nargs))
	{
		/**
		 * Check for an exit name
		 *
		 */
		init_match_check_keys(player, command, TYPE_EXIT);
		match_exit_with_parents();
		exit = last_match_result();

		if (exit != NOTHING)
		{
			if (mushconf.exit_calls_move)
			{
				/**
				 * Exits literally call the 'move' command. Note that,
				 * later, when we go to matching master-room and other
				 * global-ish exits, that we also need to have move_match_more
				 * set to 'yes', or we'll match here only to encounter dead
				 * silence when we try to find the exit inside the move routine.
				 * We also need to directly find what the pointer for the move
				 * (goto) command is, since we could have @addcommand'd it
				 * (and probably did, if this conf option is on). Finally, we've
				 * got to make this look like we really did type 'goto <exit>',
				 * or the @addcommand will just skip over the string.
				 */
				cmdp = (CMDENT *)hashfind("goto", &mushstate.command_htab);

				if (cmdp)
				{
					gbuf = XMALLOC(LBUF_SIZE, "gbuf");
					gc = gbuf;
					XSAFELBSTR(cmdp->cmdname, gbuf, &gc);
					XSAFELBCHR(' ', gbuf, &gc);
					XSAFELBSTR(command, gbuf, &gc);
					*gc = '\0';
					process_cmdent(cmdp, NULL, player, cause, interactive, command, gbuf, args, nargs);
					XFREE(gbuf);
				}
			}
			else
			{
				/**
				 * Execute the pre-hook for the goto command
				 *
				 */

				if ((goto_cmdp->pre_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
				{
					process_hook(goto_cmdp->pre_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
				}
				move_exit(player, exit, 0, NOGO_MESSAGE, 0);
				/**
				 * Execute the post-hook for the goto command
				 *
				 */
				if ((goto_cmdp->post_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
				{
					process_hook(goto_cmdp->post_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
				}
			}

			mushstate.debug_cmd = cmdsave;
			return preserve_cmd;
		}

		/**
		 * Check for an exit in the master room
		 *
		 */
		init_match_check_keys(player, command, TYPE_EXIT);
		match_master_exit();
		exit = last_match_result();

		if (exit != NOTHING)
		{
			if (mushconf.exit_calls_move)
			{
				cmdp = (CMDENT *)hashfind("goto", &mushstate.command_htab);

				if (cmdp)
				{
					gbuf = XMALLOC(LBUF_SIZE, "gbuf");
					gc = gbuf;
					XSAFELBSTR(cmdp->cmdname, gbuf, &gc);
					XSAFELBCHR(' ', gbuf, &gc);
					XSAFELBSTR(command, gbuf, &gc);
					*gc = '\0';
					process_cmdent(cmdp, NULL, player, cause, interactive, command, gbuf, args, nargs);
					XFREE(gbuf);
				}
			}
			else
			{
				if ((goto_cmdp->pre_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
				{
					process_hook(goto_cmdp->pre_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
				}
				move_exit(player, exit, 1, NOGO_MESSAGE, 0);
				if ((goto_cmdp->post_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
				{
					process_hook(goto_cmdp->post_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
				}
			}

			mushstate.debug_cmd = cmdsave;
			return preserve_cmd;
		}
	}

	/**
	 * Set up a lowercase command and an arg pointer for the hashed
	 * command check.  Since some types of argument processing destroy
	 * the arguments, make a copy so that we keep the original command
	 * line intact.  Store the edible copy in lcbuf after the lowercased
	 * command.
	 *
	 * Removed copy of the rest of the command, since it's ok to allow it
	 * to be trashed.  -dcm
	 *
	 */
	lcbuf = XMALLOC(LBUF_SIZE, "lcbuf");

	for (p = command, q = lcbuf; *p && !isspace(*p); p++, q++)
	{
		*q = tolower(*p);
	}

	*q++ = '\0';

	while (*p && isspace(*p))
	{
		p++;
	}

	arg = p;

	slashp = strchr(lcbuf, '/');

	if (slashp)
	{
		*slashp++ = '\0';
	}

	/**
	 * Check for a builtin command (or an alias of a builtin command)
	 *
	 */
	cmdp = (CMDENT *)hashfind(lcbuf, &mushstate.command_htab);

	if (cmdp != NULL)
	{
		if (mushconf.space_compress && (cmdp->callseq & CS_NOSQUISH))
		{
			/**
			 * We handle this specially -- there is no space
			 * compression involved, so we must go back to the
			 * preserved command.
			 *
			 */
			XSTRCPY(command, preserve_cmd);
			arg = command;

			while (*arg && !isspace(*arg))
			{
				arg++;
			}

			if (*arg)
			{
				arg++;
			}
		}

		process_cmdent(cmdp, slashp, player, cause, interactive, arg, command, args, nargs);
		XFREE(lcbuf);
		mushstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

	/**
	 * Check for enter and leave aliases, user-defined commands on the
	 * player, other objects where the player is, on objects in the
	 * player's inventory, and on the room that holds the player. We
	 * evaluate the command line here to allow chains of $-commands to
	 * work.
	 *
	 */
	str = evcmd = XMALLOC(LBUF_SIZE, "evcmd");
	XSTRCPY(evcmd, command);
	bp = lcbuf;
	eval_expression_string(lcbuf, &bp, player, cause, cause, EV_EVAL | EV_FCHECK | EV_STRIP | EV_TOP, &str, args, nargs);
	XFREE(evcmd);
	succ = 0;

	/**
	 * Idea for enter/leave aliases from R'nice@TinyTIM
	 *
	 */
	if (Has_location(player) && Good_obj(Location(player)))
	{
		/**
		 * Check for a leave alias, if we have permissions to use the
		 * 'leave' command.
		 *
		 */
		if (check_cmd_access(player, leave_cmdp, args, nargs))
		{
			p = atr_pget(Location(player), A_LALIAS, &aowner, &aflags, &alen);

			if (*p)
			{
				if (matches_exit_from_list(lcbuf, p))
				{
					XFREE(lcbuf);
					XFREE(p);

					if ((leave_cmdp->pre_hook != NULL) && !(leave_cmdp->callseq & CS_ADDED))
					{
						process_hook(leave_cmdp->pre_hook, leave_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
					}

					do_leave(player, player, 0);

					if ((leave_cmdp->post_hook != NULL) && !(leave_cmdp->callseq & CS_ADDED))
					{
						process_hook(leave_cmdp->post_hook, leave_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
					}

					return preserve_cmd;
				}
			}

			XFREE(p);
		}

		/**
		 * Check for enter aliases, if we have permissions to use the
		 * 'enter' command.
		 */
		if (check_cmd_access(player, enter_cmdp, args, nargs))
		{
			for (exit = Contents(Location(player)); (exit != NOTHING) && (Next(exit) != exit); exit = Next(exit))
			{
				p = atr_pget(exit, A_EALIAS, &aowner, &aflags, &alen);

				if (*p)
				{
					if (matches_exit_from_list(lcbuf, p))
					{
						XFREE(lcbuf);
						XFREE(p);
						if ((enter_cmdp->pre_hook != NULL) && !(enter_cmdp->callseq & CS_ADDED))
						{
							process_hook(enter_cmdp->pre_hook, enter_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
						}
						do_enter_internal(player, exit, 0);
						if ((enter_cmdp->post_hook != NULL) && !(enter_cmdp->callseq & CS_ADDED))
						{
							process_hook(enter_cmdp->post_hook, enter_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
						}
						return preserve_cmd;
					}
				}

				XFREE(p);
			}
		}
	}

	/**
	 * At each of the following stages, we check to make sure that we
	 * haven't hit a match on a STOP-set object.
	 *
	 */
	got_stop = 0;

	/**
	 * Check for $-command matches on me
	 *
	 */
	if (mushconf.match_mine)
	{
		if (((Typeof(player) != TYPE_PLAYER) || mushconf.match_mine_pl) && (atr_match(player, player, AMATCH_CMD, lcbuf, preserve_cmd, 1) > 0))
		{
			succ++;
			got_stop = Stop_Match(player);
		}
	}

	/**
	 * Check for $-command matches on nearby things and on my room
	 *
	 */
	if (!got_stop && Has_location(player))
	{
		succ += list_check(Contents(Location(player)), player, AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);

		if (!got_stop && atr_match(Location(player), player, AMATCH_CMD, lcbuf, preserve_cmd, 1) > 0)
		{
			succ++;
			got_stop = Stop_Match(Location(player));
		}
	}

	/**
	 * Check for $-command matches in my inventory
	 */
	if (!got_stop && Has_contents(player))
		succ += list_check(Contents(player), player, AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);

	/**
	 * If we didn't find anything, and we're checking local masters, do
	 * those checks. Do it for the zone of the player's location first,
	 * and then, if nothing is found, on the player's personal zone.
	 * Walking back through the parent tree stops when a match is found.
	 * Also note that these matches are done in the style of the master
	 * room: parents of the contents of the rooms aren't checked for
	 * commands. We try to maintain 2.2/MUX compatibility here, putting
	 * both sets of checks together.
	 *
	 */

	if (Has_location(player) && Good_obj(Location(player)))
	{
		/**
		 * 2.2 style location
		 *
		 */
		if (!succ && mushconf.local_masters)
		{
			pcount = 0;
			parent = Parent(Location(player));

			while (!succ && !got_stop && Good_obj(parent) && ParentZone(parent) && (pcount < mushconf.parent_nest_lim))
			{
				if (Has_contents(parent))
				{
					succ += list_check(Contents(parent), player, AMATCH_CMD, lcbuf, preserve_cmd, mushconf.match_zone_parents, &got_stop);
				}

				parent = Parent(parent);
				pcount++;
			}
		}

		/**
		 * MUX style location
		 *
		 */
		if ((!succ) && mushconf.have_zones && (Zone(Location(player)) != NOTHING))
		{
			if (Typeof(Zone(Location(player))) == TYPE_ROOM)
			{
				/**
				 * zone of player's location is a parent room
				 *
				 */
				if (Location(player) != Zone(player))
				{
					/**
					 * check parent room exits
					 *
					 */
					init_match_check_keys(player, command, TYPE_EXIT);
					match_zone_exit();
					exit = last_match_result();

					if (exit != NOTHING)
					{
						if (mushconf.exit_calls_move)
						{
							cmdp = (CMDENT *)
								hashfind("goto", &mushstate.command_htab);

							if (cmdp)
							{
								gbuf = XMALLOC(LBUF_SIZE, "gbuf");
								gc = gbuf;
								XSAFELBSTR(cmdp->cmdname, gbuf, &gc);
								XSAFELBCHR(' ', gbuf, &gc);
								XSAFELBSTR(command, gbuf, &gc);
								*gc = '\0';
								process_cmdent(cmdp, NULL, player, cause, interactive, command, gbuf, args, nargs);
								XFREE(gbuf);
							}
						}
						else
						{
							if ((goto_cmdp->pre_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
							{
								process_hook(goto_cmdp->pre_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
							}
							move_exit(player, exit, 1, NOGO_MESSAGE, 0);
							if ((goto_cmdp->post_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
							{
								process_hook(goto_cmdp->post_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
							}
						}

						mushstate.debug_cmd = cmdsave;
						return preserve_cmd;
					}

					if (!got_stop)
					{
						succ += list_check(Contents(Zone(Location(player))), player, AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);
					}
				}
			}
			else
				/**
				 * try matching commands on area zone object
				 *
				 */
				if (!got_stop && !succ && mushconf.have_zones && (Zone(Location(player)) != NOTHING))
				{
					succ += atr_match(Zone(Location(player)), player, AMATCH_CMD, lcbuf, preserve_cmd, 1);
				}
		}
	}

	/**
	 * 2.2 style player
	 *
	 */
	if (!succ && mushconf.local_masters)
	{
		parent = Parent(player);

		if (!Has_location(player) || !Good_obj(Location(player)) || ((parent != Location(player)) && (parent != Parent(Location(player)))))
		{
			pcount = 0;

			while (!succ && !got_stop && Good_obj(parent) && ParentZone(parent) && (pcount < mushconf.parent_nest_lim))
			{
				if (Has_contents(parent))
				{
					succ += list_check(Contents(parent), player, AMATCH_CMD, lcbuf, preserve_cmd, 0, &got_stop);
				}

				parent = Parent(parent);
				pcount++;
			}
		}
	}

	/**
	 * MUX style player
	 *
	 * if nothing matched with parent room/zone object, try matching zone
	 * commands on the player's personal zone
	 *
	 */
	if (!got_stop && !succ && mushconf.have_zones && (Zone(player) != NOTHING) && (!Has_location(player) || !Good_obj(Location(player)) || (Zone(Location(player)) != Zone(player))))
	{
		succ += atr_match(Zone(player), player, AMATCH_CMD, lcbuf, preserve_cmd, 1);
	}

	/**
	 * If we didn't find anything, try in the master room
	 *
	 */
	if (!got_stop && !succ)
	{
		if (Good_loc(mushconf.master_room))
		{
			succ += list_check(Contents(mushconf.master_room), player, AMATCH_CMD, lcbuf, preserve_cmd, 0, &got_stop);

			if (!got_stop && atr_match(mushconf.master_room, player, AMATCH_CMD, lcbuf, preserve_cmd, 0) > 0)
			{
				succ++;
			}
		}
	}

	/**
	 * Allow modules to intercept, if still no match. This time we pass
	 * both the lower-cased evaluated buffer and the preserved command.
	 *
	 */
	if (!succ)
	{
		for (csm__mp = mushstate.modules_list, succ = 0; (csm__mp != NULL) && !succ; csm__mp = csm__mp->next)
		{
			if (csm__mp->process_no_match)
			{
				succ = (*(csm__mp->process_no_match))(player, cause, interactive, lcbuf, preserve_cmd, args, nargs);
			}
		}
	}

	XFREE(lcbuf);

	/**
	 * If we still didn't find anything, tell how to get help.
	 *
	 */
	if (!succ)
	{
		notify(player, mushconf.huh_msg);
		pname = log_getname(player);

		if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
		{
			lname = log_getname(Location(player));
			log_write(LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s", pname, lname, command);
			XFREE(lname);
		}
		else
		{
			log_write(LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s", pname, command);
		}

		XFREE(pname);
	}

	mushstate.debug_cmd = cmdsave;
	return preserve_cmd;
}

/**
 * @brief Execute a semicolon/pipe-delimited series of commands.
 *
 * @param player	DBref of the player
 * @param cause		DBref of the cause
 * @param cmdline	Command
 * @param args		Arguments
 * @param nargs		Number of arguments
 * @param qent		Command queue.
 */
void process_cmdline(dbref player, dbref cause, char *cmdline, char *args[], int nargs, BQUE *qent)
{
	char *cmdsave = NULL, *save_poutnew = NULL, *save_poutbufc = NULL, *save_pout = NULL;
	char *cp = NULL, *log_cmdbuf = NULL, *pname = NULL, *lname = NULL;
	int save_inpipe = 0, numpipes = 0, used_time = 0;
	dbref save_poutobj = NOTHING, save_enactor = NOTHING, save_player = NOTHING;
	struct timeval begin_time, end_time, obj_time;
	struct rusage b_usage, e_usage;

	if (mushstate.cmd_nest_lev == mushconf.cmd_nest_lim)
	{
		return;
	}

	mushstate.cmd_nest_lev++;
	cmdsave = mushstate.debug_cmd;
	save_enactor = mushstate.curr_enactor;
	save_player = mushstate.curr_player;
	mushstate.curr_enactor = cause;
	mushstate.curr_player = player;
	save_inpipe = mushstate.inpipe;
	save_poutobj = mushstate.poutobj;
	save_poutnew = mushstate.poutnew;
	save_poutbufc = mushstate.poutbufc;
	save_pout = mushstate.pout;
	mushstate.break_called = 0;

	while (cmdline && (!qent || qent == mushstate.qfirst) && !mushstate.break_called)
	{
		cp = parse_to(&cmdline, ';', 0);

		if (cp && *cp)
		{
			numpipes = 0;
			while (cmdline && (*cmdline == '|') && (!qent || qent == mushstate.qfirst) && (numpipes < mushconf.ntfy_nest_lim))
			{
				cmdline++;
				numpipes++;
				mushstate.inpipe = 1;
				mushstate.poutnew = XMALLOC(LBUF_SIZE, "mushstate.poutnew");
				mushstate.poutbufc = mushstate.poutnew;
				mushstate.poutobj = player;
				mushstate.debug_cmd = cp;
				/**
				 * No lag check on piped commands
				 *
				 */
				process_command(player, cause, 0, cp, args, nargs);

				if (mushstate.pout && mushstate.pout != save_pout)
				{
					XFREE(mushstate.pout);
				}

				*mushstate.poutbufc = '\0';
				mushstate.pout = mushstate.poutnew;
				cp = parse_to(&cmdline, ';', 0);
			}

			mushstate.inpipe = save_inpipe;
			mushstate.poutnew = save_poutnew;
			mushstate.poutbufc = save_poutbufc;
			mushstate.poutobj = save_poutobj;
			mushstate.debug_cmd = cp;
			/**
			 * Is the queue still linked like we think it is?
			 *
			 */
			if (qent && qent != mushstate.qfirst)
			{
				if (mushstate.pout && mushstate.pout != save_pout)
				{
					XFREE(mushstate.pout);
				}

				break;
			}

			if (mushconf.lag_check)
			{
				gettimeofday(&begin_time, NULL);

				if (mushconf.lag_check_cpu)
				{
					getrusage(RUSAGE_SELF, &b_usage);
				}
			}

			log_cmdbuf = process_command(player, cause, 0, cp, args, nargs);

			if (mushstate.pout && mushstate.pout != save_pout)
			{
				XFREE(mushstate.pout);
				mushstate.pout = save_pout;
			}

			save_poutbufc = mushstate.poutbufc;

			if (mushconf.lag_check)
			{
				gettimeofday(&end_time, NULL);

				if (mushconf.lag_check_cpu)
				{
					getrusage(RUSAGE_SELF, &e_usage);
				}

				used_time = msec_diff(end_time, begin_time);

				if ((used_time / 1000) >= mushconf.max_cmdsecs)
				{
					pname = log_getname(player);

					if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
					{
						lname = log_getname(Location(player));
						log_write(LOG_PROBLEMS, "CMD", "CPU", "%s in %s queued command taking %.2f secs (enactor #%d): %s", pname, lname, (double)(used_time / 1000), mushstate.qfirst->cause, log_cmdbuf);
						XFREE(lname);
					}
					else
					{
						log_write(LOG_PROBLEMS, "CMD", "CPU", "%s queued command taking %.2f secs (enactor #%d): %s", pname, (double)(used_time / 1000), mushstate.qfirst->cause, log_cmdbuf);
					}

					XFREE(pname);
				}
				/**
				 * Don't use msec_add(), this is more accurate
				 *
				 */
				if (mushconf.lag_check_clk)
				{
					obj_time = Time_Used(player);

					if (mushconf.lag_check_cpu)
					{
						obj_time.tv_usec += e_usage.ru_utime.tv_usec;
						obj_time.tv_sec += e_usage.ru_utime.tv_sec - b_usage.ru_utime.tv_sec;
					}
					else
					{
						obj_time.tv_usec += end_time.tv_usec - begin_time.tv_usec;
						obj_time.tv_sec += end_time.tv_sec - begin_time.tv_sec;
					}

					if (obj_time.tv_usec < 0)
					{
						obj_time.tv_usec += 1000000;
						obj_time.tv_sec--;
					}
					else if (obj_time.tv_usec >= 1000000)
					{
						obj_time.tv_sec += obj_time.tv_usec / 1000000;
						obj_time.tv_usec = obj_time.tv_usec % 1000000;
					}

					db[player].cpu_time_used.tv_sec = obj_time.tv_sec;
					db[player].cpu_time_used.tv_usec = obj_time.tv_usec;
				}
			}
		}
	}

	mushstate.debug_cmd = cmdsave;
	mushstate.curr_enactor = save_enactor;
	mushstate.curr_player = save_player;
	mushstate.cmd_nest_lev--;

	if (log_cmdbuf)
	{
		XFREE(log_cmdbuf);
	}
}

/**
 * @brief List internal commands. Note that user-defined command permissions are
 * ignored in this context.
 *
 * @param player DBref of the player
 */
void list_cmdtable(dbref player)
{
	CMDENT *cmdp = NULL, *modcmds = NULL;
	MODULE *mp = NULL;
	char *buf = XMALLOC(LBUF_SIZE, "buf");

	XSPRINTF(buf, "Built-in commands:");
	for (cmdp = command_table; cmdp->cmdname; cmdp++)
	{
		if (check_access(player, cmdp->perms))
		{
			if (!(cmdp->perms & CF_DARK))
			{
				XSPRINTFCAT(buf, " %s", cmdp->cmdname);
			}
		}
	}

	/**
	 * Players get the list of logged-out cmds too
	 *
	 */
	if (isPlayer(player))
	{
		display_nametab(player, logout_cmdtable, true, buf);
	}
	else
	{
		notify(player, buf);
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		char *modname = XASPRINTF("modname", "mod_%s_%s", mp->modname, "cmdtable");

		if ((modcmds = (CMDENT *)dlsym(mp->handle, modname)) != NULL)
		{
			XSPRINTF(buf, "Module %s commands:", mp->modname);
			for (cmdp = modcmds; cmdp->cmdname; cmdp++)
			{
				if (check_access(player, cmdp->perms))
				{
					if (!(cmdp->perms & CF_DARK))
					{
						XSPRINTFCAT(buf, " %s", cmdp->cmdname);
					}
				}
			}
			notify(player, buf);
		}

		XFREE(modname);
	}

	XFREE(buf);
}

/**
 * @brief List available attributes.
 *
 * @param player
 */
void list_attrtable(dbref player)
{
	ATTR *ap = NULL;
	char *cp = NULL, *bp = NULL, *buf = NULL;

	bp = buf = XMALLOC(LBUF_SIZE, "buf");

	for (cp = (char *)"Attributes:"; *cp; cp++)
	{
		*bp++ = *cp;
	}

	for (ap = attr; ap->name; ap++)
	{
		if (See_attr(player, player, ap, player, 0))
		{
			*bp++ = ' ';

			for (cp = (char *)(ap->name); *cp; cp++)
			{
				*bp++ = *cp;
			}
		}
	}

	*bp = '\0';
	raw_notify(player, NULL, buf);
	XFREE(buf);
}

/**
 * @brief Helper for the list access commands.
 *
 * @param player	DBref of the player
 * @param ctab		Command table
 * @param buff		Buffer for the list
 */
void helper_list_cmdaccess(dbref player, CMDENT *ctab)
{
	CMDENT *cmdp = NULL;
	ATTR *ap = NULL;

	for (cmdp = ctab; cmdp->cmdname; cmdp++)
	{
		if (check_access(player, cmdp->perms))
		{
			if (!(cmdp->perms & CF_DARK))
			{
				if (cmdp->userperms)
				{
					ap = atr_num(cmdp->userperms->atr);

					if (!ap)
					{
						listset_nametab(player, access_nametab, cmdp->perms, true, "%-26.26s user(#%d/?BAD?)", cmdp->cmdname, cmdp->userperms->thing);
					}
					else
					{
						listset_nametab(player, access_nametab, cmdp->perms, true, "%-26.26s user(#%d/%s)", cmdp->cmdname, cmdp->userperms->thing, ap->name);
					}
				}
				else
				{
					listset_nametab(player, access_nametab, cmdp->perms, true, "%-26.26s ", cmdp->cmdname);
				}
			}
		}
	}
}

/**
 * @brief List access commands.
 *
 * @param player DBref of the player
 */
void list_cmdaccess(dbref player)
{
	CMDENT *cmdp = NULL, *ctab = NULL;
	ATTR *ap = NULL;
	MODULE *mp = NULL;
	char *p = NULL, *q = NULL, *buff = XMALLOC(SBUF_SIZE, "buff");

	notify(player, "Command                    Permissions");
	notify(player, "-------------------------- ----------------------------------------------------");

	helper_list_cmdaccess(player, command_table);

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		p = XASPRINTF("p", "mod_%s_%s", mp->modname, "cmdtable");

		if ((ctab = (CMDENT *)dlsym(mp->handle, p)) != NULL)
		{
			helper_list_cmdaccess(player, ctab);
		}
		XFREE(p);
	}

	for (ap = attr; ap->name; ap++)
	{
		p = buff;
		*p++ = '@';

		for (q = (char *)ap->name; *q; p++, q++)
		{
			*p = tolower(*q);
		}

		if (ap->flags & AF_NOCMD)
		{
			continue;
		}

		*p = '\0';
		cmdp = (CMDENT *)hashfind(buff, &mushstate.command_htab);

		if (cmdp == NULL)
		{
			continue;
		}

		if (!check_access(player, cmdp->perms))
		{
			continue;
		}

		if (!(cmdp->perms & CF_DARK))
		{
			XSPRINTF(buff, "%-26.26s ", cmdp->cmdname);
			listset_nametab(player, access_nametab, cmdp->perms, true, buff);
		}
	}

	notify(player, "-------------------------------------------------------------------------------");

	XFREE(buff);
}

/**
 * @brief List switches for commands.
 *
 * @param player DBref of the player
 */
void list_cmdswitches(dbref player)
{
	CMDENT *cmdp = NULL, *ctab = NULL;
	MODULE *mp = NULL;
	char *s = NULL, *buff = XMALLOC(SBUF_SIZE, "buff");

	notify(player, "Command          Switches");
	notify(player, "---------------- ---------------------------------------------------------------");

	for (cmdp = command_table; cmdp->cmdname; cmdp++)
	{
		if (cmdp->switches)
		{
			if (check_access(player, cmdp->perms))
			{
				if (!(cmdp->perms & CF_DARK))
				{
					display_nametab(player, cmdp->switches, false, "%-16.16s", cmdp->cmdname);
				}
			}
		}
	}

	s = XMALLOC(MBUF_SIZE, "s");

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, "cmdtable");

		if ((ctab = (CMDENT *)dlsym(mp->handle, s)) != NULL)
		{
			for (cmdp = ctab; cmdp->cmdname; cmdp++)
			{
				if (cmdp->switches)
				{
					if (check_access(player, cmdp->perms))
					{
						if (!(cmdp->perms & CF_DARK))
						{
							display_nametab(player, cmdp->switches, false, "%-16.16s", cmdp->cmdname);
						}
					}
				}
			}
		}
	}

	notify(player, "--------------------------------------------------------------------------------");

	XFREE(s);
	XFREE(buff);
}

/**
 * @brief List access to attributes.
 *
 * @param player DBref of player
 */
void list_attraccess(dbref player)
{
	ATTR *ap = NULL;
	notify(player, "Attribute                  Permissions");
	notify(player, "-------------------------- ----------------------------------------------------");
	for (ap = attr; ap->name; ap++)
	{
		if (Read_attr(player, player, ap, player, 0))
		{
			listset_nametab(player, attraccess_nametab, ap->flags, true, "%-26.26s ", ap->name);
		}
	}
	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List attribute "types" (wildcards and permissions)
 *
 * @param player DBref of player
 */
void list_attrtypes(dbref player)
{
	KEYLIST *kp = NULL;

	if (!mushconf.vattr_flag_list)
	{
		notify(player, "No attribute type patterns defined.");
		return;
	}

	notify(player, "Attribute                  Permissions");
	notify(player, "-------------------------- ----------------------------------------------------");

	for (kp = mushconf.vattr_flag_list; kp != NULL; kp = kp->next)
	{
		char *buff = XASPRINTF("buff", "%-26.26s ", kp->name);
		listset_nametab(player, attraccess_nametab, kp->data, true, buff);
		XFREE(buff);
	}

	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Change command or switch permissions.
 *
 * @param vp
 * @param str
 * @param extra
 * @param player
 * @param cmd
 * @return int
 */
int cf_access(int *vp __attribute__((unused)), char *str, long extra, dbref player, char *cmd)
{
	CMDENT *cmdp = NULL;
	char *ap = NULL;
	int set_switch = 0;

	for (ap = str; *ap && !isspace(*ap) && (*ap != '/'); ap++)
		;

	if (*ap == '/')
	{
		set_switch = 1;
		*ap++ = '\0';
	}
	else
	{
		set_switch = 0;

		if (*ap)
		{
			*ap++ = '\0';
		}

		while (*ap && isspace(*ap))
		{
			ap++;
		}
	}

	cmdp = (CMDENT *)hashfind(str, &mushstate.command_htab);

	if (cmdp != NULL)
	{
		if (set_switch)
			return cf_ntab_access((int *)cmdp->switches, ap, extra, player, cmd);
		else
			return cf_modify_bits(&(cmdp->perms), ap, extra, player, cmd);
	}
	else
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Command", str);
		return -1;
	}
}

/**
 * @brief Change command permissions for all attr-setting cmds.
 *
 * @param vp
 * @param str
 * @param extra
 * @param player
 * @param cmd
 * @return int
 */
int cf_acmd_access(int *vp __attribute__((unused)), char *str, long extra, dbref player, char *cmd)
{
	CMDENT *cmdp = NULL;
	ATTR *ap = NULL;
	int failure = 0, save = 0;
	char *p = NULL, *q = NULL, *buff = XMALLOC(SBUF_SIZE, "buff");

	for (ap = attr; ap->name; ap++)
	{
		p = buff;
		*p++ = '@';

		for (q = (char *)ap->name; *q; p++, q++)
		{
			*p = tolower(*q);
		}

		*p = '\0';
		cmdp = (CMDENT *)hashfind(buff, &mushstate.command_htab);

		if (cmdp != NULL)
		{
			save = cmdp->perms;
			failure = cf_modify_bits(&(cmdp->perms), str, extra, player, cmd);

			if (failure != 0)
			{
				cmdp->perms = save;
				XFREE(buff);
				return -1;
			}
		}
	}

	XFREE(buff);
	return 0;
}

/**
 * @brief Change access on an attribute.
 *
 * @param vp
 * @param str
 * @param extra
 * @param player
 * @param cmd
 * @return int
 */
int cf_attr_access(int *vp __attribute__((unused)), char *str, long extra, dbref player, char *cmd)
{
	ATTR *ap = NULL;
	char *sp = NULL;

	for (sp = str; *sp && !isspace(*sp); sp++)
		;

	if (*sp)
	{
		*sp++ = '\0';
	}

	while (*sp && isspace(*sp))
	{
		sp++;
	}

	ap = atr_str(str);

	if (ap != NULL)
	{
		return cf_modify_bits(&(ap->flags), sp, extra, player, cmd);
	}
	else
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Attribute", str);
		return -1;
	}
}

/**
 * @brief Define attribute flags for new user-named attributes whose
 * names match a certain pattern.
 *
 * @param vp
 * @param str
 * @param extra
 * @param player
 * @param cmd
 * @return int
 */
int cf_attr_type(int *vp __attribute__((unused)), char *str, long extra, dbref player, char *cmd)
{
	char *privs = NULL;
	KEYLIST *kp = NULL;
	int succ = 0;

	/**
	 * Split our string into the attribute pattern and privileges. Also
	 * uppercase it, while we're at it. Make sure it's not longer than an
	 * attribute name can be.
	 *
	 */
	for (privs = str; *privs && !isspace(*privs); privs++)
	{
		*privs = toupper(*privs);
	}

	if (*privs)
	{
		*privs++ = '\0';
	}

	while (*privs && isspace(*privs))
	{
		privs++;
	}

	if (strlen(str) >= VNAME_SIZE)
	{
		str[VNAME_SIZE - 1] = '\0';
	}

	/**
	 * Create our new data blob. Make sure that we're setting the privs
	 * to something reasonable before trying to link it in. (If we're
	 * not, an error will have been logged; we don't need to do it.)
	 */
	kp = (KEYLIST *)XMALLOC(sizeof(KEYLIST), "kp");
	kp->data = 0;
	succ = cf_modify_bits(&(kp->data), privs, extra, player, cmd);

	if (succ < 0)
	{
		XFREE(kp);
		return -1;
	}

	kp->name = XSTRDUP(str, "kp->name");
	kp->next = mushconf.vattr_flag_list;
	mushconf.vattr_flag_list = kp;
	return (succ);
}

/**
 * @brief Add a command alias.
 *
 * @param vp
 * @param str
 * @param extra
 * @param player
 * @param cmd
 * @return int
 */
int cf_cmd_alias(int *vp, char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{

	CMDENT *cmdp = NULL, *cmd2 = NULL;
	NAMETAB *nt = NULL;
	int *hp = NULL;
	char *ap = NULL, *tokst = NULL;
	char *alias = strtok_r(str, " \t=,", &tokst);
	char *orig = strtok_r(NULL, " \t=,", &tokst);

	if (!orig)
	{
		/**
		 * we only got one argument to @alias. Bad.
		 *
		 */
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid original for alias %s", alias);
		return -1;
	}

	if (alias[0] == '_' && alias[1] == '_')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias %s would cause @addcommand conflict", alias);
		return -1;
	}

	for (ap = orig; *ap && (*ap != '/'); ap++)
		;

	if (*ap == '/')
	{
		/**
		 * Switch form of command aliasing: create an alias for a
		 * command + a switch
		 */
		*ap++ = '\0';
		/**
		 * Look up the command
		 *
		 */
		cmdp = (CMDENT *)hashfind(orig, (HASHTAB *)vp);

		if (cmdp == NULL)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Command", str);
			return -1;
		}
		/**
		 * Look up the switch
		 *
		 */
		nt = find_nametab_ent(player, (NAMETAB *)cmdp->switches, ap);

		if (!nt)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Switch", str);
			return -1;
		}
		/**
		 * Got it, create the new command table entry
		 *
		 */
		cmd2 = (CMDENT *)XMALLOC(sizeof(CMDENT), "cmd2");
		cmd2->cmdname = XSTRDUP(alias, "cmd2->cmdname");
		cmd2->switches = cmdp->switches;
		cmd2->perms = cmdp->perms | nt->perm;
		cmd2->extra = (cmdp->extra | nt->flag) & ~SW_MULTIPLE;

		if (!(nt->flag & SW_MULTIPLE))
		{
			cmd2->extra |= SW_GOT_UNIQUE;
		}

		cmd2->callseq = cmdp->callseq;

		/**
		 * KNOWN PROBLEM: We are not inheriting the hook that the
		 * 'original' command had -- we will have to add it manually
		 * (whereas an alias of a non-switched command is just
		 * another hashtable entry for the same command pointer and
		 * therefore gets the hook). This is preferable to having to
		 * search the hashtable for hooks when a hook is deleted,
		 * though.
		 *
		 */
		cmd2->pre_hook = NULL;
		cmd2->post_hook = NULL;
		cmd2->userperms = NULL;
		cmd2->info.handler = cmdp->info.handler;

		if (hashadd(cmd2->cmdname, (int *)cmd2, (HASHTAB *)vp, 0))
		{
			XFREE(cmd2->cmdname);
			XFREE(cmd2);
		}
	}
	else
	{
		/**
		 * A normal (non-switch) alias
		 *
		 */
		hp = hashfind(orig, (HASHTAB *)vp);

		if (hp == NULL)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
			return -1;
		}

		hashadd(alias, hp, (HASHTAB *)vp, HASH_ALIAS);
	}

	return 0;
}

/**
 * @brief List default flags at create time.
 *
 * @param player DBref of player
 */
void list_df_flags(dbref player)
{
	char *playerb = decode_flags(player, mushconf.player_flags);
	char *roomb = decode_flags(player, mushconf.room_flags);
	char *exitb = decode_flags(player, mushconf.exit_flags);
	char *thingb = decode_flags(player, mushconf.thing_flags);
	char *robotb = decode_flags(player, mushconf.robot_flags);
	char *stripb = decode_flags(player, mushconf.stripped_flags);
	notify(player, "Type           Default flags");
	notify(player, "-------------- ----------------------------------------------------------------");
	raw_notify(player, "Players        P%s", playerb);
	raw_notify(player, "Rooms          R%s", roomb);
	raw_notify(player, "Exits          E%s", exitb);
	raw_notify(player, "Things         %s", thingb);
	raw_notify(player, "Robots         P%s", robotb);
	raw_notify(player, "Stripped       %s", stripb);
	notify(player, "-------------------------------------------------------------------------------");
	XFREE(playerb);
	XFREE(roomb);
	XFREE(exitb);
	XFREE(thingb);
	XFREE(robotb);
	XFREE(stripb);
}

/**
 * @brief List the costs of things.
 *
 * @param player DBref of player
 */
void list_costs(dbref player)
{
	char *buff = XMALLOC(MBUF_SIZE, "buff");

	notify(player, "Action                                            Minimum   Maximum   Quota");
	notify(player, "------------------------------------------------- --------- --------- ---------");

	if (mushconf.quotas)
	{
		raw_notify(player, "%-49.49s %-9d           %-9d", "Digging Room", mushconf.digcost, mushconf.room_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Digging Room", mushconf.digcost);
	}

	if (mushconf.quotas)
	{
		raw_notify(player, "%-49.49s %-9d           %-9d", "Opening Exit", mushconf.opencost, mushconf.exit_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Opening Exit", mushconf.opencost);
	}

	raw_notify(player, "%-49.49s %-9d", "Linking Exit or DropTo", mushconf.linkcost);

	if (mushconf.quotas)
	{
		raw_notify(player, "%-49.49s %-9d %-9d %-9d", "Creating Thing", mushconf.createmin, mushconf.createmax, mushconf.exit_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d %-9d", "Creating Thing", mushconf.createmin, mushconf.createmax);
	}

	if (mushconf.quotas)
	{
		raw_notify(player, "%-49.49s %-9d           %-9d", "Creating Robot", mushconf.robotcost, mushconf.player_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Creating Robot", mushconf.robotcost);
	}

	raw_notify(player, "%-49.49s %-9d %-9d", "Killing Player", mushconf.killmin, mushconf.killmax);
	if (mushconf.killmin == mushconf.killmax)
	{
		raw_notify(player, "  Chance of success: %d%%", (mushconf.killmin * 100) / mushconf.killguarantee);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Guaranted Kill Success", mushconf.killguarantee);
	}

	raw_notify(player, "%-49.49s %-9d", "Computationally expensive commands or functions", mushconf.searchcost);
	raw_notify(player, "  @entrances, @find, @search, @stats,");
	raw_notify(player, "  search() and stats()");

	if (mushconf.machinecost > 0)
	{
		raw_notify(player, "%-49.49s 1/%-7d", "Command run from Queue", mushconf.machinecost);
	}

	if (mushconf.waitcost > 0)
	{
		raw_notify(player, "%-49.49s %-9d", "Deposit for putting command in Queue", mushconf.waitcost);
		raw_notify(player, "  Deposit refund when command is run or cancel");
	}

	if (mushconf.sacfactor == 0)
	{
		raw_notify(player, "%-49.49s %-9d", "Object Value", mushconf.sacadjust);
	}
	else if (mushconf.sacfactor == 1)
	{
		if (mushconf.sacadjust < 0)
		{
			raw_notify(player, "%-49.49s Creation Cost - %d", "Object Value", -mushconf.sacadjust);
		}
		else if (mushconf.sacadjust > 0)
		{
			raw_notify(player, "%-49.49s Creation Cost + %d", "Object Value", mushconf.sacadjust);
		}
		else
		{
			raw_notify(player, "%-49.49s Creation Cost");
		}
	}
	else
	{
		if (mushconf.sacadjust < 0)
		{
			raw_notify(player, "%-49.49s (Creation Cost / %d) - %d", "Object Value", mushconf.sacfactor, -mushconf.sacadjust);
		}
		else if (mushconf.sacadjust > 0)
		{
			raw_notify(player, "%-49.49s (Creation Cost / %d) + %d", "Object Value", mushconf.sacfactor, mushconf.sacadjust);
		}
		else
		{
			raw_notify(player, "%-49.49s Creation Cost / %d", "Object Value", mushconf.sacfactor);
		}
	}

	if (mushconf.clone_copy_cost)
	{
		raw_notify(player, "%-49.49s Value Original Object", "Cloned Object Value");
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Cloned Object Value", mushconf.createmin);
	}

	notify(player, "-------------------------------------------------------------------------------");
	raw_notify(player, "All costs are in %s", mushconf.many_coins);

	XFREE(buff);
}

/**
 * @brief List boolean game options from mushconf. list_config: List non-boolean game options.
 *
 * @param player DBref of player
 */
void list_params(dbref player)
{
	time_t now = time(NULL);

	notify(player, "Prototype           Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Room                #%d", mushconf.room_proto);
	raw_notify(player, "Exit                #%d", mushconf.exit_proto);
	raw_notify(player, "Thing               #%d", mushconf.thing_proto);
	raw_notify(player, "Player              #%d", mushconf.player_proto);
	notify(player, "\nAttr Default        Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Room                #%d", mushconf.room_defobj);
	raw_notify(player, "Exit                #%d", mushconf.exit_defobj);
	raw_notify(player, "Thing               #%d", mushconf.thing_defobj);
	raw_notify(player, "Player              #%d", mushconf.player_defobj);
	notify(player, "\nDefault Parents     Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Room                #%d", mushconf.room_parent);
	raw_notify(player, "Exit                #%d", mushconf.exit_parent);
	raw_notify(player, "Thing               #%d", mushconf.thing_parent);
	raw_notify(player, "Player              #%d", mushconf.player_parent);
	notify(player, "\nLimits              Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Function recursion  %d", mushconf.func_nest_lim);
	raw_notify(player, "Function invocation %d", mushconf.func_invk_lim);
	raw_notify(player, "Command recursion   %d", mushconf.cmd_nest_lim);
	raw_notify(player, "Command invocation  %d", mushconf.cmd_invk_lim);
	raw_notify(player, "Output              %d", mushconf.output_limit);
	raw_notify(player, "Queue               %d", mushconf.queuemax);
	raw_notify(player, "CPU                 %d", mushconf.func_cpu_lim_secs);
	raw_notify(player, "Wild                %d", mushconf.wild_times_lim);
	raw_notify(player, "Aliases             %d", mushconf.max_player_aliases);
	raw_notify(player, "Forwardlist         %d", mushconf.fwdlist_lim);
	raw_notify(player, "Propdirs            %d", mushconf.propdir_lim);
	raw_notify(player, "Registers           %d", mushconf.register_limit);
	raw_notify(player, "Stacks              %d", mushconf.stack_lim);
	raw_notify(player, "Variables           %d", mushconf.numvars_lim);
	raw_notify(player, "Structures          %d", mushconf.struct_lim);
	raw_notify(player, "Instances           %d", mushconf.instance_lim);
	raw_notify(player, "Objects             %d", mushconf.building_limit);
	raw_notify(player, "Allowance           %d", mushconf.paylimit);
	raw_notify(player, "Trace levels        %d", mushconf.trace_limit);
	raw_notify(player, "Connect tries       %d", mushconf.retry_limit);

	if (mushconf.max_players >= 0)
	{
		raw_notify(player, "Logins              %d", mushconf.max_players);
	}

	notify(player, "\nNesting             Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Locks               %d", mushconf.lock_nest_lim);
	raw_notify(player, "Parents             %d", mushconf.parent_nest_lim);
	raw_notify(player, "Messages            %d", mushconf.ntfy_nest_lim);
	raw_notify(player, "Zones               %d", mushconf.zone_nest_lim);
	notify(player, "\nTimeouts            Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Idle                %d", mushconf.idle_timeout);
	raw_notify(player, "Connect             %d", mushconf.conn_timeout);
	raw_notify(player, "Tries               %d", mushconf.retry_limit);
	raw_notify(player, "Lag                 %d", mushconf.max_cmdsecs);
	notify(player, "\nMoney               Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Start               %d", mushconf.paystart);
	raw_notify(player, "Daily               %d", mushconf.paycheck);
	raw_notify(player, "Singular            %s", mushconf.one_coin);
	raw_notify(player, "Plural              %s", mushconf.many_coins);

	if (mushconf.payfind > 0)
	{
		raw_notify(player, "Find money          1 chance in %d", mushconf.payfind);
	}

	notify(player, "\nStart Quotas        Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Total               %d", mushconf.start_quota);
	raw_notify(player, "Rooms               %d", mushconf.start_room_quota);
	raw_notify(player, "Exits               %d", mushconf.start_exit_quota);
	raw_notify(player, "Things              %d", mushconf.start_thing_quota);
	raw_notify(player, "Players             %d", mushconf.start_player_quota);

	notify(player, "\nDbrefs              Value");
	notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "Master Room         #%d", mushconf.master_room);
	raw_notify(player, "Start Room          #%d", mushconf.start_room);
	raw_notify(player, "Start Home          #%d", mushconf.start_home);
	raw_notify(player, "Default Home        #%d", mushconf.default_home);

	if (Wizard(player))
	{
		raw_notify(player, "Guest Char          #%d", mushconf.guest_char);
		raw_notify(player, "GuestStart          #%d", mushconf.guest_start_room);
		raw_notify(player, "Freelist            #%d", mushstate.freelist);

		notify(player, "\nQueue run sizes     Value");
		notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "No net activity     %d", mushconf.queue_chunk);
		raw_notify(player, "Activity            %d", mushconf.active_q_chunk);
		notify(player, "\nIntervals           Value");
		notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "Dump                %d", mushconf.dump_interval);
		raw_notify(player, "Clean               %d", mushconf.check_interval);
		raw_notify(player, "Idle Check          %d", mushconf.idle_interval);
		raw_notify(player, "Optimize            %d", mushconf.dbopt_interval);
		notify(player, "\nTimers              Value");
		notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "Dump                %d", (int)(mushstate.dump_counter - now));
		raw_notify(player, "Clean               %d", (int)(mushstate.check_counter - now));
		raw_notify(player, "Idle Check          %d", (int)(mushstate.idle_counter - now));
		notify(player, "\nScheduling          Value");
		notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "Timeslice           %d", mushconf.timeslice);
		raw_notify(player, "Max_Quota           %d", mushconf.cmd_quota_max);
		raw_notify(player, "Increment           %d", mushconf.cmd_quota_incr);
		notify(player, "\nAttribute cache     Value");
		notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "Width               %d", mushconf.cache_width);
		raw_notify(player, "Size                %d", mushconf.cache_size);
	}
	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List user-defined attributes
 *
 * @param player DBref of player
 */
void list_vattrs(dbref player)
{
	VATTR *va = NULL;
	int na = 0;

	notify(player, "User-Defined Attributes    Attr ID  Permissions");
	notify(player, "-------------------------- -------- -------------------------------------------");

	for (va = vattr_first(), na = 0; va; va = vattr_next(va), na++)
	{
		if (!(va->flags & AF_DELETED))
		{
			listset_nametab(player, attraccess_nametab, va->flags, true, "%-26.26s %-8d ", va->name, va->number);
		}
	}

	notify(player, "-------------------------------------------------------------------------------");
	raw_notify(player, "%d attributes, next=%d", na, mushstate.attr_next);
}

/**
 * @brief Helper for listing information from an hash table
 *
 * @param player	DBref of player
 * @param tab_name	Hash table name
 * @param htab		Hash table
 */
void list_hashstat(dbref player, const char *tab_name, HASHTAB *htab)
{
	char *buff = hashinfo(tab_name, htab);

	notify(player, buff);

	XFREE(buff);
}

/**
 * @brief Helper for listing information from an nhash table
 *
 * @param player	DBref of player
 * @param tab_name	NHash table name
 * @param htab		NHash table
 */
void list_nhashstat(dbref player, const char *tab_name, HASHTAB *htab)
{
	char *buff = nhashinfo(tab_name, htab);

	notify(player, buff);

	XFREE(buff);
}

/**
 * @brief List informations from Hash/NHash tables
 *
 * @param player DBref of player
 */
void list_hashstats(dbref player)
{
	MODULE *mp = NULL;
	MODHASHES *m_htab = NULL, *hp = NULL, *m_ntab = NULL, *np = NULL;
	char *s = NULL;

	notify(player, "Hash Stats         Size Entries Deleted   Empty Lookups    Hits  Checks Longest");
	notify(player, "--------------- ------- ------- ------- ------- ------- ------- ------- -------");
	list_hashstat(player, "Commands", &mushstate.command_htab);
	list_hashstat(player, "Logged-out Cmds", &mushstate.logout_cmd_htab);
	list_hashstat(player, "Functions", &mushstate.func_htab);
	list_hashstat(player, "User Functions", &mushstate.ufunc_htab);
	list_hashstat(player, "Flags", &mushstate.flags_htab);
	list_hashstat(player, "Powers", &mushstate.powers_htab);
	list_hashstat(player, "Attr names", &mushstate.attr_name_htab);
	list_hashstat(player, "Vattr names", &mushstate.vattr_name_htab);
	list_hashstat(player, "Player Names", &mushstate.player_htab);
	list_hashstat(player, "References", &mushstate.nref_htab);
	list_nhashstat(player, "Net Descriptors", &mushstate.desc_htab);
	list_nhashstat(player, "Queue Entries", &mushstate.qpid_htab);
	list_nhashstat(player, "Forwardlists", &mushstate.fwdlist_htab);
	list_nhashstat(player, "Propdirs", &mushstate.propdir_htab);
	list_nhashstat(player, "Redirections", &mushstate.redir_htab);
	list_nhashstat(player, "Overlaid $-cmds", &mushstate.parent_htab);
	list_nhashstat(player, "Object Stacks", &mushstate.objstack_htab);
	list_nhashstat(player, "Object Grids", &mushstate.objgrid_htab);
	list_hashstat(player, "Variables", &mushstate.vars_htab);
	list_hashstat(player, "Structure Defs", &mushstate.structs_htab);
	list_hashstat(player, "Component Defs", &mushstate.cdefs_htab);
	list_hashstat(player, "Instances", &mushstate.instance_htab);
	list_hashstat(player, "Instance Data", &mushstate.instdata_htab);
	list_hashstat(player, "Module APIs", &mushstate.api_func_htab);

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		s = XASPRINTF("s", "mod_%s_%s", mp->modname, "hashtable");
		m_htab = (MODHASHES *)dlsym(mp->handle, s);
		XFREE(s);

		if (m_htab)
		{
			for (hp = m_htab; hp->htab != NULL; hp++)
			{
				list_hashstat(player, hp->tabname, hp->htab);
			}
		}

		s = XASPRINTF("s", "mod_%s_%s", mp->modname, "nhashtable");
		m_ntab = (MODHASHES *)dlsym(mp->handle, s);
		XFREE(s);

		if (m_ntab)
		{
			for (np = m_ntab; np->tabname != NULL; np++)
			{
				list_nhashstat(player, np->tabname, np->htab);
			}
		}
	}
	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List textfiles
 *
 * @param player DBref of player
 */
void list_textfiles(dbref player)
{
	raw_notify(player, NULL, "Help File       Size    Entries Deleted Empty   Lookups Hits    Checks  Longest");
	raw_notify(player, NULL, "--------------- ------- ------- ------- ------- ------- ------- ------- -------");

	for (int i = 0; i < mushstate.helpfiles; i++)
	{
		raw_notify(player, "%-15.15s %7d %7d %7d %7d %7d %7d %7d %7d", basename(mushstate.hfiletab[i]), mushstate.hfile_hashes[i].hashsize, mushstate.hfile_hashes[i].entries, mushstate.hfile_hashes[i].deletes, mushstate.hfile_hashes[i].nulls, mushstate.hfile_hashes[i].scans, mushstate.hfile_hashes[i].hits, mushstate.hfile_hashes[i].checks, mushstate.hfile_hashes[i].max_scan);
	}

	raw_notify(player, NULL, "-------------------------------------------------------------------------------");
}

/**
 * @brief Get useful info from the DB layer about hash stats, etc.
 *
 * @param player DBref of player
 */
void list_db_stats(dbref player)
{
	notify(player, "DB Cache Stats              Writes                    Reads");
	notify(player, "--------------------------- ------------------------- -------------------------");
	raw_notify(player, "Calls                       %-25d %-25d", cs_writes, cs_reads);
	raw_notify(player, "Cache Hits                  %-25d %-25d", cs_whits, cs_rhits);
	raw_notify(player, "I/O                         %-25d %-25d", cs_dbwrites, cs_dbreads);
	raw_notify(player, "Failed                                                %-25d", cs_fails);
	raw_notify(player, "Hit ratio                   %-3.0f%%                      %-3.0f%%", (cs_writes ? (float)cs_whits / cs_writes * 100 : 0.0), (cs_reads ? (float)cs_rhits / cs_reads * 100 : 0.0));
	raw_notify(player, "Deletes                     %d", cs_dels);
	raw_notify(player, "Checks                      %d", cs_checks);
	raw_notify(player, "Syncs                       %d", cs_syncs);
	notify(player, "-------------------------------------------------------------------------------");
	raw_notify(player, "Cache size: %d bytes. Cache time: %d seconds.", cs_size, (int)(time(NULL) - cs_ltime));
}

/**
 * @brief List local resource usage stats of the MUSH process. Adapted
 * from code by Claudius@PythonMUCK, posted to the net by Howard/Dark_Lord.
 *
 * @param player DBref of player
 */
void list_process(dbref player)
{
	int pid = 0, psize = 0, maxfds = 0;
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	maxfds = getdtablesize();
	pid = getpid();
	psize = getpagesize();
	/**
	 * Go display everything
	 *
	 */
	raw_notify(player, "      Process ID: %10d        %10d bytes per page", pid, psize);
	raw_notify(player, "       Time used: %10d user   %10d sys", usage.ru_utime.tv_sec, usage.ru_stime.tv_sec);
	raw_notify(player, " Integral memory: %10d shared %10d private %10d stack", usage.ru_ixrss, usage.ru_idrss, usage.ru_isrss);
	raw_notify(player, "  Max res memory: %10d pages  %10d bytes", usage.ru_maxrss, (usage.ru_maxrss * psize));
	raw_notify(player, "     Page faults: %10d hard   %10d soft    %10d swapouts", usage.ru_majflt, usage.ru_minflt, usage.ru_nswap);
	raw_notify(player, "        Disk I/O: %10d reads  %10d writes", usage.ru_inblock, usage.ru_oublock);
	raw_notify(player, "     Network I/O: %10d in     %10d out", usage.ru_msgrcv, usage.ru_msgsnd);
	raw_notify(player, "  Context switch: %10d vol    %10d forced  %10d sigs", usage.ru_nvcsw, usage.ru_nivcsw, usage.ru_nsignals);
	raw_notify(player, " Descs available: %10d", maxfds);
}

void print_memory(dbref player, const char *item, size_t size)
{

	if (size < 1024)
	{
		raw_notify(player, "%-30.30s %0.2fB", item, size);
	}
	else if (size < 1048576)
	{
		raw_notify(player, "%-30.30s %0.2fK", item, size / 1024.0);
	}
	else
	{
		raw_notify(player, "%-30.30s %0.2fM", item, size / 1048576.0);
	}
}

/**
 * @brief Breaks down memory usage of the process
 *
 * @param player DBref of player
 */
void list_memory(dbref player)
{
	double total = 0, each = 0, each2 = 0;
	int i = 0, j = 0;
	CMDENT *cmd = NULL;
	ADDENT *add = NULL;
	NAMETAB *name = NULL;
	ATTR *attr = NULL;
	UFUN *ufunc = NULL;
	UDB_CACHE *cp = NULL;
	UDB_CHAIN *sp = NULL;
	HASHENT *htab = NULL;
	OBJSTACK *stack = NULL;
	OBJGRID *grid = NULL;
	VARENT *xvar = NULL;
	STRUCTDEF *this_struct = NULL;
	INSTANCE *inst_ptr = NULL;
	STRUCTDATA *data_ptr = NULL;

	/**
	 * Calculate size of object structures
	 *
	 */
	each = mushstate.db_top * sizeof(OBJ);
	// raw_notify(player, "%-20s %12.2fk", "Object structures", each / 1024);
	raw_notify(player, "Item                          Size");
	raw_notify(player, "------------------------------ ------------------------------------------------");
	print_memory(player, "Object structures", each);
	total += each;
	/**
	 * Calculate size of mushstate and mushconf structures
	 *
	 */
	each = sizeof(CONFDATA) + sizeof(STATEDATA);
	print_memory(player, "mushconf/mushstate", each);
	total += each;
	/**
	 * Calculate size of cache
	 *
	 */
	each = cs_size;
	print_memory(player, "Cache data", each);
	total += each;
	each = sizeof(UDB_CHAIN) * mushconf.cache_width;

	for (i = 0; i < mushconf.cache_width; i++)
	{
		sp = &sys_c[i];

		for (cp = sp->head; cp != NULL; cp = cp->nxt)
		{
			each += sizeof(UDB_CACHE);
			each2 += cp->keylen;
		}
	}

	print_memory(player, "Cache keys", each2);
	print_memory(player, "Cache overhead", each);
	total += each + each2;
	/**
	 * Calculate size of object pipelines
	 *
	 */
	each = 0;

	for (i = 0; i < NUM_OBJPIPES; i++)
	{
		if (mushstate.objpipes[i])
		{
			each += obj_siz(mushstate.objpipes[i]);
		}
	}

	print_memory(player, "Object pipelines", each);
	total += each;
	/**
	 * Calculate size of name caches
	 */
	each = sizeof(NAME *) * mushstate.db_top * 2;

	for (i = 0; i < mushstate.db_top; i++)
	{
		if (purenames[i])
		{
			each += strlen(purenames[i]) + 1;
		}

		if (names[i])
		{
			each += strlen(names[i]) + 1;
		}
	}

	print_memory(player, "Name caches", each);
	total += each;
	/**
	 * Calculate size of Raw Memory allocations
	 *
	 */
	each = total_rawmemory();

	print_memory(player, "Raw Memory", each);
	total += each;
	/**
	 * Calculate size of command hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.command_htab.hashsize;

	for (i = 0; i < mushstate.command_htab.hashsize; i++)
	{
		htab = mushstate.command_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(mushstate.command_htab.entry[i]->target.s) + 1;

			/**
			 * Add up all the little bits in the CMDENT.
			 *
			 */

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(CMDENT);
				cmd = (CMDENT *)htab->data;
				each += strlen(cmd->cmdname) + 1;

				if ((name = cmd->switches) != NULL)
				{
					for (j = 0; name[j].name != NULL; j++)
					{
						each += sizeof(NAMETAB);
						each += strlen(name[j].name) + 1;
					}
				}

				if (cmd->callseq & CS_ADDED)
				{
					add = cmd->info.added;

					while (add != NULL)
					{
						each += sizeof(ADDENT);
						each += strlen(add->name) + 1;
						add = add->next;
					}
				}
			}

			htab = htab->next;
		}
	}

	print_memory(player, "Command table", each);
	total += each;
	/**
	 * Calculate size of logged-out commands hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.logout_cmd_htab.hashsize;

	for (i = 0; i < mushstate.logout_cmd_htab.hashsize; i++)
	{
		htab = mushstate.logout_cmd_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				name = (NAMETAB *)htab->data;
				each += sizeof(NAMETAB);
				each += strlen(name->name) + 1;
			}

			htab = htab->next;
		}
	}

	print_memory(player, "Logout cmd htab", each);
	total += each;
	/**
	 * Calculate size of functions hashtable
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.func_htab.hashsize;

	for (i = 0; i < mushstate.func_htab.hashsize; i++)
	{
		htab = mushstate.func_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(FUN);
			}

			/**
			 * We don't count func->name because we already got
			 * it with htab->target.s
			 *
			 */
			htab = htab->next;
		}
	}

	print_memory(player, "Functions htab", each);
	total += each;
	/**
	 * Calculate size of user-defined functions hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.ufunc_htab.hashsize;

	for (i = 0; i < mushstate.ufunc_htab.hashsize; i++)
	{
		htab = mushstate.ufunc_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				ufunc = (UFUN *)htab->data;

				while (ufunc != NULL)
				{
					each += sizeof(UFUN);
					each += strlen(ufunc->name) + 1;
					ufunc = ufunc->next;
				}
			}

			htab = htab->next;
		}
	}

	print_memory(player, "U-functions htab", each);
	total += each;
	/**
	 * Calculate size of flags hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.flags_htab.hashsize;

	for (i = 0; i < mushstate.flags_htab.hashsize; i++)
	{
		htab = mushstate.flags_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(FLAGENT);
			}

			/**
			 * We don't count flag->flagname because we already
			 * got it with htab->target.s
			 *
			 */
			htab = htab->next;
		}
	}

	print_memory(player, "Flags htab", each);
	total += each;
	/**
	 * Calculate size of powers hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.powers_htab.hashsize;

	for (i = 0; i < mushstate.powers_htab.hashsize; i++)
	{
		htab = mushstate.powers_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(POWERENT);
			}

			/**
			 * We don't count power->powername because we already
			 * got it with htab->target.s
			 *
			 */
			htab = htab->next;
		}
	}

	print_memory(player, "Powers htab", each);
	total += each;
	/**
	 * Calculate size of helpfile hashtables
	 *
	 */
	each = 0;

	for (j = 0; j < mushstate.helpfiles; j++)
	{
		each += sizeof(HASHENT *) * mushstate.hfile_hashes[j].hashsize;

		for (i = 0; i < mushstate.hfile_hashes[j].hashsize; i++)
		{
			htab = mushstate.hfile_hashes[j].entry[i];

			while (htab != NULL)
			{
				each += sizeof(HASHENT);
				each += strlen(htab->target.s) + 1;

				if (!(htab->flags & HASH_ALIAS))
				{
					each += sizeof(struct help_entry);
				}

				htab = htab->next;
			}
		}
	}

	print_memory(player, "Helpfiles htabs", each);
	total += each;
	/**
	 * Calculate size of vattr name hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.vattr_name_htab.hashsize;

	for (i = 0; i < mushstate.vattr_name_htab.hashsize; i++)
	{
		htab = mushstate.vattr_name_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;
			each += sizeof(VATTR);
			htab = htab->next;
		}
	}

	print_memory(player, "Vattr name htab", each);
	total += each;
	/**
	 * Calculate size of attr name hashtable
	 *
	 */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.attr_name_htab.hashsize;

	for (i = 0; i < mushstate.attr_name_htab.hashsize; i++)
	{
		htab = mushstate.attr_name_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				attr = (ATTR *)htab->data;
				each += sizeof(ATTR);
				each += strlen(attr->name) + 1;
			}

			htab = htab->next;
		}
	}

	print_memory(player, "Attr name htab", each);
	total += each;
	/**
	 * Calculate the size of anum_table
	 *
	 */
	each = sizeof(ATTR *) * anum_alc_top;
	print_memory(player, "Attr num table", each);
	total += each;

	/**
	 * After this point, we only report if it's non-zero.
	 *
	 */

	/**
	 * Calculate size of object stacks
	 *
	 */
	each = 0;

	for (stack = (OBJSTACK *)hash_firstentry((HASHTAB *)&mushstate.objstack_htab); stack != NULL; stack = (OBJSTACK *)hash_nextentry((HASHTAB *)&mushstate.objstack_htab))
	{
		each += sizeof(OBJSTACK);
		each += strlen(stack->data) + 1;
	}

	if (each)
	{
		print_memory(player, "Object stacks", each);
	}

	total += each;
	/**
	 * Calculate the size of grids
	 *
	 */
	each = 0;

	for (grid = (OBJGRID *)hash_firstentry((HASHTAB *)&mushstate.objgrid_htab); grid != NULL; grid = (OBJGRID *)hash_nextentry((HASHTAB *)&mushstate.objgrid_htab))
	{
		each += sizeof(OBJGRID);
		each += sizeof(char **) * grid->rows * grid->cols;

		for (i = 0; i < grid->rows; i++)
		{
			for (j = 0; j < grid->cols; j++)
			{
				if (grid->data[i][j] != NULL)
				{
					each += strlen(grid->data[i][j]) + 1;
				}
			}
		}
	}

	if (each)
	{
		print_memory(player, "Object grids", each);
	}

	total += each;
	/**
	 * Calculate the size of xvars.
	 */
	each = 0;

	for (xvar = (VARENT *)hash_firstentry(&mushstate.vars_htab); xvar != NULL; xvar = (VARENT *)hash_nextentry(&mushstate.vars_htab))
	{
		each += sizeof(VARENT);
		each += strlen(xvar->text) + 1;
	}

	if (each)
	{
		raw_notify(player, "X-Variables      : %12.2fk", each / 1024);
	}

	total += each;
	/**
	 * Calculate the size of overhead associated with structures.
	 *
	 */
	each = 0;

	for (this_struct = (STRUCTDEF *)hash_firstentry(&mushstate.structs_htab); this_struct != NULL; this_struct = (STRUCTDEF *)hash_nextentry(&mushstate.structs_htab))
	{
		each += sizeof(STRUCTDEF);
		each += strlen(this_struct->s_name) + 1;

		for (i = 0; i < this_struct->c_count; i++)
		{
			each += strlen(this_struct->c_names[i]) + 1;
			each += sizeof(COMPONENT);
			each += strlen(this_struct->c_array[i]->def_val) + 1;
		}
	}

	for (inst_ptr = (INSTANCE *)hash_firstentry(&mushstate.instance_htab); inst_ptr != NULL; inst_ptr = (INSTANCE *)hash_nextentry(&mushstate.instance_htab))
	{
		each += sizeof(INSTANCE);
	}

	if (each)
	{
		print_memory(player, "Struct var defs", each);
	}

	total += each;
	/**
	 * Calculate the size of data associated with structures.
	 *
	 */
	each = 0;

	for (data_ptr = (STRUCTDATA *)hash_firstentry(&mushstate.instdata_htab); data_ptr != NULL; data_ptr = (STRUCTDATA *)hash_nextentry(&mushstate.instdata_htab))
	{
		each += sizeof(STRUCTDATA);

		if (data_ptr->text)
		{
			each += strlen(data_ptr->text) + 1;
		}
	}

	if (each)
	{
		print_memory(player, "Struct var data", each);
	}

	total += each;
	/**
	 * Report end total.
	 *
	 */
	raw_notify(player, "-------------------------------------------------------------------------------");
	print_memory(player, "Total", total);
}

/**
 * @brief List information stored in internal structures.
 *
 * @param player	DBref of player
 * @param cause		DBref of cause
 * @param extra		Not used.
 * @param arg		Arguments
 */
void do_list(dbref player, dbref cause __attribute__((unused)), int extra __attribute__((unused)), char *arg)
{
	int flagvalue = search_nametab(player, list_names, arg);

	switch (flagvalue)
	{
	case LIST_ALLOCATOR:
		list_bufstats(player);
		break;

	case LIST_BUFTRACE:
		list_buftrace(player);
		break;

	case LIST_ATTRIBUTES:
		list_attrtable(player);
		break;

	case LIST_COMMANDS:
		list_cmdtable(player);
		break;

	case LIST_SWITCHES:
		list_cmdswitches(player);
		break;

	case LIST_COSTS:
		list_costs(player);
		break;

	case LIST_OPTIONS:
		list_options(player);
		break;

	case LIST_HASHSTATS:
		list_hashstats(player);
		break;

	case LIST_SITEINFO:
		list_siteinfo(player);
		break;

	case LIST_FLAGS:
		display_flagtab(player);
		break;

	case LIST_FUNCPERMS:
		list_funcaccess(player);
		break;

	case LIST_FUNCTIONS:
		list_functable(player);
		break;

	case LIST_GLOBALS:
		interp_nametab(player, enable_names, mushconf.control_flags, (char *)"Global parameters", (char *)"Status", (char *)"enabled", (char *)"disabled", true);
		break;

	case LIST_DF_FLAGS:
		list_df_flags(player);
		break;

	case LIST_PERMS:
		list_cmdaccess(player);
		break;

	case LIST_CONF_PERMS:
		list_cf_access(player);
		break;

	case LIST_CF_RPERMS:
		list_cf_read_access(player);
		break;

	case LIST_POWERS:
		display_powertab(player);
		break;

	case LIST_ATTRPERMS:
		list_attraccess(player);
		break;

	case LIST_VATTRS:
		list_vattrs(player);
		break;

	case LIST_LOGGING:
		interp_nametab(player, logoptions_nametab, mushconf.log_options, (char *)"Events Logged", (char *)"Status", (char *)"enabled", (char *)"disabled", true);
		notify(player, "");
		interp_nametab(player, logdata_nametab, mushconf.log_info, (char *)"Information Type", (char *)"Logged", (char *)"yes", (char *)"no", true);
		break;

	case LIST_DB_STATS:
		list_db_stats(player);
		break;

	case LIST_PROCESS:
		list_process(player);
		break;

	case LIST_BADNAMES:
		badname_list(player, "Disallowed names:");
		break;

	case LIST_CACHEOBJS:
		list_cached_objs(player);
		break;

	case LIST_TEXTFILES:
		list_textfiles(player);
		break;

	case LIST_PARAMS:
		list_params(player);
		break;

	case LIST_ATTRTYPES:
		list_attrtypes(player);
		break;

	case LIST_MEMORY:
		list_memory(player);
		break;

	case LIST_CACHEATTRS:
		list_cached_attrs(player);
		break;

	case LIST_RAWMEM:
		list_rawmemory(player);
		break;

	default:
		display_nametab(player, list_names, true, (char *)"Unknown option.  Use one of:");
	}
}
