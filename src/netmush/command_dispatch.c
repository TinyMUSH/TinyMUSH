/**
 * @file command_dispatch.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command parsing, resolution, and main execution pipeline
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
#include "command_internal.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <pcre2.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>

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

	/* Validate argument count to prevent buffer overflow attacks */
	if (ncargs < 0 || ncargs > NUM_ENV_VARS)
	{
		return;
	}

	/* Validate player object type compatibility */
	if (Invalid_Objtype(player))
	{
		notify(player, "Command incompatible with invoker type.");
		return;
	}

	/* Perform comprehensive permission check (core/module/user-defined) */
	if (!check_cmd_access(player, cmdp, cargs, ncargs))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	/* Check global building restriction flag */
	if (!Builder(player) && Protect(CA_GBL_BUILD) && !(mushconf.control_flags & CF_BUILD))
	{
		notify(player, "Sorry, building is not allowed now.");
		return;
	}

	/* Check global queueing/triggering restriction flag */
	if (Protect(CA_GBL_INTERP) && !(mushconf.control_flags & CF_INTERP))
	{
		notify(player, "Sorry, queueing and triggering are not allowed now.");
		return;
	}

	/* Initialize key with command's extra flags, masking out SW_MULTIPLE */
	key = cmdp->extra & ~SW_MULTIPLE;

	/* Track whether we've seen a non-SW_MULTIPLE switch (for exclusivity checking) */
	bool seen_exclusive_switch = (key & SW_GOT_UNIQUE) != 0;
	if (seen_exclusive_switch)
	{
		key &= ~SW_GOT_UNIQUE; /* Remove marker bit from key */
	}

	/* Parse and validate command switches (e.g., /quiet/force) */
	if (switchp && cmdp->switches)
	{
		do
		{
			/* Find next switch separator or end of string */
			buf1 = strchr(switchp, '/');
			if (buf1)
			{
				*buf1++ = '\0'; /* Terminate current switch, advance to next */
			}

			/* Look up switch in command's switch table */
			xkey = search_nametab(player, cmdp->switches, switchp);

			if (xkey == -1) /* Unrecognized switch */
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN,
							 "Unrecognized switch '%s' for command '%s'.", switchp, cmdp->cmdname);
				return;
			}
			else if (xkey == -2) /* Permission denied for this switch */
			{
				notify(player, NOPERM_MESSAGE);
				return;
			}
			else if (!(xkey & SW_MULTIPLE)) /* Exclusive switch (cannot combine with others) */
			{
				if (seen_exclusive_switch)
				{
					notify(player, "Illegal combination of switches.");
					return;
				}
				seen_exclusive_switch = true;
			}
			else /* SW_MULTIPLE flag set - strip it before ORing into key */
			{
				xkey &= ~SW_MULTIPLE;
			}

			key |= xkey;	/* Accumulate switch flags */
			switchp = buf1; /* Move to next switch */
		} while (buf1);
	}
	else if (switchp && !(cmdp->callseq & CS_ADDED))
	{
		/* Switches specified but command doesn't support them */
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN,
					 "Command %s does not take switches.", cmdp->cmdname);
		return;
	}

	/* Execute pre-command hook if registered (not for CS_ADDED commands) */
	if (cmdp->pre_hook && !(cmdp->callseq & CS_ADDED))
	{
		process_hook(cmdp->pre_hook, cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, cargs, ncargs);
	}

	/* Determine argument interpretation mode based on command flags and switches */
	if ((cmdp->callseq & CS_INTERP) && (key & SW_NOEVAL))
	{
		/* Command normally evaluates, but /noeval switch specified */
		interp = EV_STRIP;
		key &= ~SW_NOEVAL; /* Remove switch from key (already processed) */
	}
	else if ((cmdp->callseq & CS_INTERP) || (!(cmdp->callseq & CS_NOINTERP) && !interactive))
	{
		/* If CS_INTERP is set, always interpret the arg. */
		/* Else if CS_NOINTERP is set, never interpret the arg. */
		/* If neither flag is set, interpret the arg in non-interactive mode only. */
		/* Therefore: If CS_NOINTERP is NOT set, then we must check for interactivity, and only interpret if we're NOT interactive. */
		interp = EV_EVAL | EV_STRIP;
	}
	else if (cmdp->callseq & CS_STRIP)
	{
		/* Strip leading/trailing whitespace only */
		interp = EV_STRIP;
	}
	else if (cmdp->callseq & CS_STRIP_AROUND)
	{
		/* Strip whitespace around delimiters */
		interp = EV_STRIP_AROUND;
	}
	else
	{
		/* No interpretation - raw text */
		interp = 0;
	}

	/* Dispatch command based on argument structure (calling sequence) */
	switch (cmdp->callseq & CS_NARG_MASK)
	{
	case CS_NO_ARGS: /* Commands with no arguments (e.g., WHO, QUIT, INVENTORY) */
	{
		handler_cs_no_args = cmdp->info.handler;
		(*handler_cs_no_args)(player, cause, key);
		break;
	}

	case CS_ONE_ARG: /* Commands with single argument (e.g., LOOK <object>) */
		/* Handle unparsed commands (raw text passed directly to handler) */
		if (cmdp->callseq & CS_UNPARSE)
		{
			handler_cs_one_args_unparse = cmdp->info.handler;
			(*handler_cs_one_args_unparse)(player, unp_command);
			break;
		}

		/* Interpret/parse argument based on evaluation mode */
		if ((interp & EV_EVAL) && !(cmdp->callseq & CS_ADDED))
		{
			/* Evaluate argument with full softcode interpretation */
			buf1 = XMALLOC(LBUF_SIZE, "buf1");
			bp = buf1;
			str = arg;
			eval_expression_string(buf1, &bp, player, cause, cause,
								   interp | EV_FCHECK | EV_TOP, &str, cargs, ncargs);
		}
		else
		{
			/* Parse argument without evaluation (or CS_ADDED handles it separately) */
			buf1 = parse_to(&arg, '\0', interp | EV_TOP);
		}

		/* Dispatch to handler based on CS_CMDARG and CS_ADDED flags */
		if (cmdp->callseq & CS_CMDARG)
		{
			/* Pass original command arguments (%0-%9) to handler */
			handler_cs_one_args_cmdargs = cmdp->info.handler;
			(*handler_cs_one_args_cmdargs)(player, cause, key, buf1, cargs, ncargs);
		}
		else if (cmdp->callseq & CS_ADDED)
		{
			/* Handle dynamic softcode commands registered via \@addcommand */
			preserve = save_global_regs("process_cmdent_added");

			/* Determine where command arguments start (skip prefix/command name) */
			if (cmdp->callseq & CS_LEADIN)
			{
				/* Single-character prefix command (e.g., ":", ";") - skip 1 char */
				j = unp_command + 1;
			}
			else
			{
				/* Multi-character command - skip to first space or end */
				for (j = unp_command; *j && (*j != ' '); j++)
					;
			}

			/* Reconstruct command with switches for pattern matching */
			new = XMALLOC(LBUF_SIZE, "new");
			bp = new;

			if (!*j) /* No arguments after command name */
			{
				/* Build: <cmdname>[/switches] */
				char *cmdname_str = (cmdp->callseq & CS_LEADIN) ? unp_command : cmdp->cmdname;
				XSAFELBSTR(cmdname_str, new, &bp);
				if (switchp)
				{
					XSAFELBCHR('/', new, &bp);
					XSAFELBSTR(switchp, new, &bp);
				}
			}
			else /* Arguments present */
			{
				/* Build: <cmdname>[/switches] <args> */
				if (!(cmdp->callseq & CS_LEADIN))
				{
					j++; /* Skip space separator */
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
			}
			*bp = '\0';

			/* Match command against registered attribute patterns */
			for (add = (ADDENT *)cmdp->info.added; add != NULL; add = add->next)
			{
				/* Retrieve attribute containing pattern:action */
				buff = atr_get(add->thing, add->atr, &aowner, &aflags, &alen);

				/* Skip '$' marker and find ':' separator (handle escaped colons) */
				for (s = buff + 2; *s && ((*s != ':') || (*(s - 1) == '\\')); s++)
					;

				if (!*s) /* Malformed attribute (no separator) */
				{
					XFREE(buff);
					break;
				}

				*s++ = '\0'; /* Terminate pattern, s now points to action code */

				/* Match pattern against reconstructed command */
				bool pattern_matches = false;
				if (aflags & AF_REGEXP)
				{
					/* Regex matching with optional case-insensitivity */
					pattern_matches = regexp_match(buff + 1, new,
												   (aflags & AF_CASE) ? 0 : PCRE2_CASELESS,
												   aargs, NUM_ENV_VARS);
				}
				else
				{
					/* Wildcard matching */
					pattern_matches = wild(buff + 1, new, aargs, NUM_ENV_VARS);
				}

				/* Check uselock if configured */
				bool has_permission = !mushconf.addcmd_obey_uselocks ||
									  could_doit(player, add->thing, A_LUSE);

				if (pattern_matches && has_permission)
				{
					/* Execute action softcode with captured wildcard/regex groups */
					dbref executor = (!(cmdp->callseq & CS_ACTOR) || God(player)) ? add->thing : player;
					process_cmdline(executor, player, s, aargs, NUM_ENV_VARS, NULL);

					/* Free captured argument strings */
					for (i = 0; i < NUM_ENV_VARS; i++)
					{
						XFREE(aargs[i]);
					}

					cmd_matches++;
				}

				XFREE(buff);

				/* Stop processing if match found and STOP_MATCH flag set */
				if (cmd_matches && mushconf.addcmd_obey_stop && Stop_Match(add->thing))
				{
					break;
				}
			}

			/* Handle no matches (if not configured to match blindly) */
			if (!cmd_matches && !mushconf.addcmd_match_blindly)
			{
				notify(player, mushconf.huh_msg);

				/* Log failed command attempt */
				pname = log_getname(player);
				if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
				{
					lname = log_getname(Location(player));
					log_write(LOG_BADCOMMANDS, "CMD", "BAD", "%s in %s entered: %s",
							  pname, lname, new);
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
			/* Standard one-argument command handler */
			handler_cs_one_args = cmdp->info.handler;
			(*handler_cs_one_args)(player, cause, key, buf1);
		}

		/* Free allocated buffer if we performed evaluation */
		if ((interp & EV_EVAL) && !(cmdp->callseq & CS_ADDED))
		{
			XFREE(buf1);
		}
		break;

	case CS_TWO_ARG: /* Commands with two arguments: cmd arg1=arg2 */
		/* Parse and interpret first argument (before '=') */
		buf2 = parse_to(&arg, '=', EV_STRIP_TS);

		/* Handle missing '=' separator */
		if (!arg || !*arg)
		{
			arg = &tchar;
			*arg = '\0';
		}

		/* Evaluate first argument */
		buf1 = XMALLOC(LBUF_SIZE, "buf1");
		bp = buf1;
		str = buf2;
		eval_expression_string(buf1, &bp, player, cause, cause,
							   EV_STRIP | EV_FCHECK | EV_EVAL | EV_TOP, &str, cargs, ncargs);

		if (cmdp->callseq & CS_ARGV)
		{
			/* Second argument is ARGV-style (space-separated list) */
			parse_arglist(player, cause, cause, arg, '\0',
						  interp | EV_STRIP_LS | EV_STRIP_TS,
						  args, mushconf.max_command_args, cargs, ncargs);

			/* Count arguments */
			for (nargs = 0; (nargs < mushconf.max_command_args) && args[nargs]; nargs++)
				;

			/* Dispatch to handler with or without cmdargs */
			if (cmdp->callseq & CS_CMDARG)
			{
				handler_cs_two_args_cmdargs_argv = cmdp->info.handler;
				(*handler_cs_two_args_cmdargs_argv)(player, cause, key, buf1, args, nargs,
													cargs, ncargs);
			}
			else
			{
				handler_cs_two_args_argv = cmdp->info.handler;
				(*handler_cs_two_args_argv)(player, cause, key, buf1, args, nargs);
			}

			/* Free ARGV argument buffers */
			for (i = 0; i < nargs; i++)
			{
				XFREE(args[i]);
			}
		}
		else
		{
			/* Second argument is normal style (single string) */
			if (interp & EV_EVAL)
			{
				/* Evaluate second argument */
				buf2 = XMALLOC(LBUF_SIZE, "buf2");
				bp = buf2;
				str = arg;
				eval_expression_string(buf2, &bp, player, cause, cause,
									   interp | EV_FCHECK | EV_TOP, &str, cargs, ncargs);
			}
			else if (cmdp->callseq & CS_UNPARSE)
			{
				/* Preserve whitespace for unparsed commands */
				buf2 = parse_to(&arg, '\0', interp | EV_TOP | EV_NO_COMPRESS);
			}
			else
			{
				/* Strip whitespace from parsed argument */
				buf2 = parse_to(&arg, '\0', interp | EV_STRIP_LS | EV_STRIP_TS | EV_TOP);
			}

			/* Dispatch to handler with or without cmdargs */
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

			/* Free evaluated buffer if allocated */
			if (interp & EV_EVAL)
			{
				XFREE(buf2);
			}
		}

		/* Free first argument buffer (always allocated) */
		XFREE(buf1);
		break;
	}

	/* Execute post-command hook if registered (not for CS_ADDED commands) */
	if (cmdp->post_hook && !(cmdp->callseq & CS_ADDED))
	{
		process_hook(cmdp->post_hook, cmdp->callseq & (CS_PRESERVE | CS_PRIVATE),
					 player, cause, cargs, ncargs);
	}
}

/**
 * @brief Parse, normalize, resolve, and dispatch a raw command string.
 *
 * This is the high-level entrypoint for command execution. It receives the
 * raw input line, performs normalization and logging, lets modules intercept
 * the command, resolves quick lead-in commands and special cases (like HOME
 * and exits/goto), looks up builtins and aliases, then delegates execution to
 * `process_cmdent()` for the matched command entry.
 *
 * Responsibilities (execution flow):
 * - Input guardrails and player validation (halted/going objects)
 * - Logging of user input, including suspect logging and optional God monitor
 * - Whitespace normalization and optional space compression
 * - Module interception via `MODULE.process_command` (early consume/transform)
 * - Single-character lead-in dispatch (fast path via `prefix_cmds`)
 * - Special command: `home` movement (outside the regular table)
 * - Exit matching and `goto` permission check for room navigation
 * - Builtin command/alias resolution from `command_htab`
 * - Alias evaluation (softcode) to produce the final command tokens
 * - Enter/leave alias handling based on player location
 * - Delegation to `process_cmdent()` with parsed switches and arguments
 *
 * Parameters:
 * - player: DBref du joueur qui exécute la commande (enactor)
 * - cause:  DBref de l'entité qui a causé l'exécution (souvent le joueur)
 * - interactive: Vrai si la commande provient d'une entrée directe, faux si
 *   issue de la file/trigger (affecte l'interprétation des arguments)
 * - command: Chaîne brute saisie par l'utilisateur (sera normalisée)
 * - args: Tableau d'arguments d'environnement (%0-%9) transmis en aval
 * - nargs: Taille du tableau `args`
 *
 * Return value:
 * - Retourne un pointeur vers une copie préservée de la chaîne de commande;
 *   utilisée par l'infrastructure de debug et d'observabilité.
 *
 * Notes et interactions:
 * - Réinitialise plusieurs compteurs/limites (fonctions, notifications, locks)
 *   et bascule le `cputime_base` si la limite CPU des fonctions est active.
 * - Les modules peuvent consommer la commande et empêcher l'exécution standard.
 * - La commande `home` est gérée en dehors de la table de commandes.
 * - Les hooks de mouvement associés à `internalgoto` sont déclenchés ailleurs
 *   via `call_move_hook()`.
 *
 * Sécurité et permission:
 * - Les vérifications de permission spécifiques à un builtin se font au moment
 *   de la délégation dans `process_cmdent()`.
 *
 * @warning Les side-effects (journalisation, compteurs, reset de registres)
 *          se produisent même si un module intercepte et consomme la commande.
 *
 * @see process_cmdent() pour la résolution des switches, l'interprétation des
 *      arguments et l'invocation des hooks de commandes
 * @see call_move_hook() pour les hooks de mouvement sur les transitions de
 *      salle
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

	/* Robustify player */
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

	/* Make sure player isn't going or halted */
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

	/* Reset recursion and other limits. Baseline the CPU counter. */
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
				/* We have no pointer, we should have no flags. */
				s_Flags3(player, Flags3(player) & ~HAS_REDIRECT);
			}
		}
		else
		{
			notify_check(Owner(player), Owner(player), MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s] %s", Name(player), command);
		}
	}

	/* NOTE THAT THIS WILL BREAK IF "GOD" IS NOT A DBREF. */
	if (mushconf.control_flags & CF_GODMONITOR)
	{
		raw_notify(GOD, "%s(#%d)%c %s", Name(player), player, (interactive) ? '|' : ':', command);
	}

	/* Eat leading whitespace, and space-compress if configured */
	/* Normaliser les espaces en tête; évite les "commandes vides" accidentelles */
	command = (char *)skip_whitespace(command);

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

	/* Allow modules to intercept command strings. */
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

	/*
	 * Fast path: commandes à préfixe d’un caractère (lead-ins). On les traite
	 * avant HOME car elles sont fréquentes et ne peuvent pas correspondre à HOME.
	 */
	int leadin_index = command[0] & 0xff;

	if ((prefix_cmds[leadin_index] != NULL) && command[0])
	{
		process_cmdent(prefix_cmds[leadin_index], NULL, player, cause, interactive, command, command, args, nargs);
		mushstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

	/*
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

	/* Only check for exits if we may use the goto command */
	if (check_cmd_access(player, goto_cmdp, args, nargs))
	{
		/* Check for an exit name */
		init_match_check_keys(player, command, TYPE_EXIT);
		match_exit_with_parents();
		exit = last_match_result();

		if (exit != NOTHING)
		{
			if (mushconf.exit_calls_move)
			{
				/*
				 * Exits literally call the 'move' command. Note that,
				 * later, when we go to matching master-room and other
				 * global-ish exits, that we also need to have move_match_more
				 * set to 'yes', or we'll match here only to encounter dead
				 * silence when we try to find the exit inside the move routine.
				 * We also need to directly find what the pointer for the move
				 * (goto) command is, since we could have \@addcommand'd it
				 * (and probably did, if this conf option is on). Finally, we've
				 * got to make this look like we really did type 'goto exit',
				 * or the \@addcommand will just skip over the string.
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
				/* Execute the pre-hook for the goto command */

				if ((goto_cmdp->pre_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
				{
					process_hook(goto_cmdp->pre_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
				}
				move_exit(player, exit, 0, NOGO_MESSAGE, 0);
				/* Execute the post-hook for the goto command */
				if ((goto_cmdp->post_hook != NULL) && !(goto_cmdp->callseq & CS_ADDED))
				{
					process_hook(goto_cmdp->post_hook, goto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE), player, cause, args, nargs);
				}
			}

			mushstate.debug_cmd = cmdsave;
			return preserve_cmd;
		}

		/* Check for an exit in the master room */
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

	/*
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

	p = (char *)skip_whitespace(p);

	arg = p;

	slashp = strchr(lcbuf, '/');

	if (slashp)
	{
		*slashp++ = '\0';
	}

	/* Check for a builtin command (or an alias of a builtin command) */
	cmdp = (CMDENT *)hashfind(lcbuf, &mushstate.command_htab);

	if (cmdp != NULL)
	{
		if (mushconf.space_compress && (cmdp->callseq & CS_NOSQUISH))
		{
			/*
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

	/*
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

	/* Idea for enter/leave aliases from R'nice\@TinyTIM */
	if (Has_location(player) && Good_obj(Location(player)))
	{
		/* Check for a leave alias, if permitted ('leave' command). */
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

		/* Check for enter aliases, if permitted ('enter' command). */
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

	/*
	 * At each of the following stages, we check to make sure that we
	 * haven't hit a match on a STOP-set object.
	 *
	 */
	got_stop = 0;

	/* Check for $-command matches on me */
	if (mushconf.match_mine)
	{
		if (((Typeof(player) != TYPE_PLAYER) || mushconf.match_mine_pl) && (atr_match(player, player, AMATCH_CMD, lcbuf, preserve_cmd, 1) > 0))
		{
			succ++;
			got_stop = Stop_Match(player);
		}
	}

	/* Check for $-command matches on nearby things and on my room */
	if (!got_stop && Has_location(player))
	{
		succ += list_check(Contents(Location(player)), player, AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);

		if (!got_stop && atr_match(Location(player), player, AMATCH_CMD, lcbuf, preserve_cmd, 1) > 0)
		{
			succ++;
			got_stop = Stop_Match(Location(player));
		}
	}

	/* Check for $-command matches in my inventory */
	if (!got_stop && Has_contents(player))
		succ += list_check(Contents(player), player, AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);

	/*
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
		/* 2.2 style location */
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

		/* MUX style location */
		if ((!succ) && mushconf.have_zones && (Zone(Location(player)) != NOTHING))
		{
			if (Typeof(Zone(Location(player))) == TYPE_ROOM)
			{
				/* zone of player's location is a parent room */
				if (Location(player) != Zone(player))
				{
					/* check parent room exits */
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
				/* try matching commands on area zone object */
				if (!got_stop && !succ && mushconf.have_zones && (Zone(Location(player)) != NOTHING))
				{
					succ += atr_match(Zone(Location(player)), player, AMATCH_CMD, lcbuf, preserve_cmd, 1);
				}
		}
	}

	/* 2.2 style player */
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

	/*
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

	/* If we didn't find anything, try in the master room */
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

	/*
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

	/* If we still didn't find anything, tell how to get help. */
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
			log_write(LOG_BADCOMMANDS, "CMD", "BAD", "%s entered: %s", pname, command);
		}

		XFREE(pname);
	}

	mushstate.debug_cmd = cmdsave;
	return preserve_cmd;
}

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
			/* Log the current segment once (avoid repeated strlen) */
			int cp_len = (int)strlen(cp);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, NULL,
						 "[DEBUG process_cmdline] RAW cp='%s' (len=%d)",
						 cp, cp_len);
			log_write(LOG_ALWAYS, "TRIG", "CMDLINE", "[DEBUG process_cmdline] RAW cp='%s' (len=%d) (player=#%d, cause=#%d)",
					  cp, cp_len, player, cause);
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, NULL,
						 "[DEBUG process_cmdline] about to call process_command: '%s'",
						 cp);
			numpipes = 0;
			/* Scan consecutive pipes for this segment */
			while (cmdline && (*cmdline == '|') && (!qent || qent == mushstate.qfirst) && (numpipes < mushconf.ntfy_nest_lim))
			{
				cmdline++;
				numpipes++;
				mushstate.inpipe = 1;
				mushstate.poutnew = XMALLOC(LBUF_SIZE, "mushstate.poutnew");
				mushstate.poutbufc = mushstate.poutnew;
				mushstate.poutobj = player;
				mushstate.debug_cmd = cp;
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
 * @brief Execute a semicolon-delimited command line and handle pipe segments.
 *
 * Splits the incoming `cmdline` on ';' to obtain individual commands, scans
 * any trailing '|' segments to set up piped output (`pout*`, `mushstate.inpipe`),
 * preserves and restores global state around each segment, and delegates
 * execution to `process_command()` with `interactive` set to false.
 *
 * Responsibilities:
 * - Split by ';' and iterate over each command segment.
 * - Handle consecutive pipes '|' up to `mushconf.ntfy_nest_lim`.
 * - Save/restore enactor/player and pipe/output state across segments.
 * - Log segments and, when enabled, capture lag metrics.
 * - Call `process_command()` for each segment.
 *
 * Execution notes:
 * - `mushstate.inpipe`, `pout*`, and `poutobj` are saved then restored around
 *   each segment execution.
 * - When `mushconf.lag_check` is on, timing/CPU is measured via
 *   `gettimeofday`/`getrusage`.
 * - Loop stops on `mushstate.break_called` or when the queue entry is no longer
 *   at the head of the queue.
 * - Empty segments are ignored.
 *
 * @param player  DBref of the player executing the command line
 * @param cause   DBref of the cause (often the same as player)
 * @param cmdline Raw command line to parse (modified in place during parsing)
 * @param args    Environment argument array (%0-%9)
 * @param nargs   Number of entries in `args`
 * @param qent    Command queue entry; only the head is allowed to execute
 *
 * @return void
 *
 * @note Nesting is limited by `mushconf.cmd_nest_lim`.
 * @note `process_command()` handles permissions and hooks for each segment.
 *
 * @see process_command()
 * @see mushstate.inpipe
 */
