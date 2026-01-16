/**
 * @file command_core.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command initialization, parsing, dispatch, and execution engine
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * This module handles the core command execution pipeline: table initialization,
 * argument parsing, call sequence dispatch, and hook invocation. It contains no
 * permission-related functions (see command_access.c) or administrative reporting
 * (see command_admin.c).
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
#include <pcre2.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <libgen.h>

/**
 * @brief Command handler function pointers for various call sequences
 *
 */
void (*handler_cs_no_args)(dbref, dbref, int);													   /*!< Handler for no-arg commands */
void (*handler_cs_one_args)(dbref, dbref, int, char *);											   /*!< Handler for one-arg commands */
void (*handler_cs_one_args_unparse)(dbref, char *);												   /*!< Handler for one-arg unparsed commands */
void (*handler_cs_one_args_cmdargs)(dbref, dbref, int, char *, char *[], int);					   /*!< Handler for one-arg commands with cmdargs */
void (*handler_cs_two_args)(dbref, dbref, int, char *, char *);									   /*!< Handler for two-arg commands */
void (*handler_cs_two_args_cmdargs)(dbref, dbref, int, char *, char *, char *[], int);			   /*!< Handler for two-arg commands with cmdargs */
void (*handler_cs_two_args_argv)(dbref, dbref, int, char *, char *[], int);						   /*!< Handler for two-arg commands with argv */
void (*handler_cs_two_args_cmdargs_argv)(dbref, dbref, int, char *, char *[], int, char *[], int); /*!< Handler for two-arg commands with cmdargs and argv */

/**
 * @brief Command hash table and prefix command array
 */
CMDENT *prefix_cmds[256];										 /*!< Builtin prefix commands */
CMDENT *goto_cmdp, *enter_cmdp, *leave_cmdp, *internalgoto_cmdp; /*!< Commonly used command pointers */

/**
 * @brief Initialize the command hash table and populate it with all available commands.
 *
 * This function performs a complete initialization of the MUSH command system by:
 * 1. Creating the command hash table with appropriate sizing based on configuration
 * 2. Generating attribute-setter commands (\@name, \@desc, etc.) from the attribute table
 * 3. Registering all builtin commands from the static command_table
 * 4. Setting up prefix command dispatch array for single-character command leaders
 * 5. Caching frequently-used command pointers for performance optimization
 *
 * Each attribute-setter command is dynamically allocated and configured with:
 * - Lowercased "\@attribute" naming convention
 * - Standard permission mask (no guests/slaves, wizard-only if attribute requires it)
 * - CS_TWO_ARG calling sequence (command arg1=arg2)
 * - Double-underscore alias (__\@name) for programmatic access
 *
 * Builtin commands are registered directly from command_table with their aliases.
 * Prefix commands (:, ;, #, &, ", \) enable single-character command dispatch.
 *
 * @note This function must be called during server initialization before any
 *       command processing occurs. Hash collisions are handled by freeing the
 *       duplicate command entry.
 * @see reset_prefix_cmds() for re-synchronizing prefix command pointers
 * @see command_table for the static builtin command definitions
 */
void init_cmdtab(void)
{
	CMDENT *cp = NULL;
	ATTR *ap = NULL;
	char *alias = NULL;
	size_t cmdname_len = 0;
	char cbuff[SBUF_SIZE]; /* Stack allocation for temporary command name buffer */

	/* Initialize hash table with size based on configuration factor */
	hashinit(&mushstate.command_htab, 250 * mushconf.hash_factor, HT_STR);

	/* Dynamically create attribute-setter commands (@name, @desc, @flags, etc.) */
	for (ap = attr; ap->name; ap++)
	{
		/* Skip attributes marked as non-command (AF_NOCMD flag) */
		if (ap->flags & AF_NOCMD)
		{
			continue;
		}

		/* Construct lowercased "@attributename" command string */
		cbuff[0] = '@';
		cmdname_len = 1;

		for (const char *src = ap->name; *src && cmdname_len < SBUF_SIZE - 1; src++)
		{
			cbuff[cmdname_len++] = tolower(*src);
		}

		cbuff[cmdname_len] = '\0';

		/* Allocate and initialize command entry structure */
		cp = (CMDENT *)XMALLOC(sizeof(CMDENT), "cp");
		cp->cmdname = XSTRDUP(cbuff, "cp->cmdname");
		cp->perms = CA_NO_GUEST | CA_NO_SLAVE; /* Base permissions: no guests or slaves */
		cp->switches = NULL;

		/* Elevate to wizard permission if attribute requires it */
		if (ap->flags & (AF_WIZARD | AF_MDARK))
		{
			cp->perms |= CA_WIZARD;
		}

		/* Configure command entry for attribute setting */
		cp->extra = ap->number;	  /* Store attribute number for handler */
		cp->callseq = CS_TWO_ARG; /* Standard cmd obj=value format */
		cp->pre_hook = NULL;
		cp->post_hook = NULL;
		cp->userperms = NULL;
		cp->info.handler = do_setattr; /* All attribute setters use same handler */

		/* Add primary command to hash table; if collision occurs, free the duplicate */
		if (hashadd(cp->cmdname, (int *)cp, &mushstate.command_htab, 0))
		{
			XFREE(cp->cmdname);
			XFREE(cp);
		}
		else
		{
			/* Register double-underscore alias for programmatic command execution */
			alias = XASPRINTF("alias", "__%s", cp->cmdname);
			hashadd(alias, (int *)cp, &mushstate.command_htab, HASH_ALIAS);
			XFREE(alias);
		}
	}

	/* Register all builtin commands from static command_table with __ aliases */
	for (cp = command_table; cp->cmdname; cp++)
	{
		hashadd(cp->cmdname, (int *)cp, &mushstate.command_htab, 0);

		alias = XASPRINTF("alias", "__%s", cp->cmdname);
		hashadd(alias, (int *)cp, &mushstate.command_htab, HASH_ALIAS);
		XFREE(alias);
	}

	/* Initialize prefix command dispatch array (256 entries for all ASCII+extended chars) */
	for (int i = 0; i < 256; i++)
	{
		prefix_cmds[i] = NULL;
	}

	/* Register single-character command leaders: " : ; \ # & */
	register_prefix_cmds("\":;\\#&");

	/* Cache frequently-used command pointers to avoid repeated hash lookups */
	goto_cmdp = (CMDENT *)hashfind("goto", &mushstate.command_htab);
	enter_cmdp = (CMDENT *)hashfind("enter", &mushstate.command_htab);
	leave_cmdp = (CMDENT *)hashfind("leave", &mushstate.command_htab);
	internalgoto_cmdp = (CMDENT *)hashfind("internalgoto", &mushstate.command_htab);
}

/**
 * @brief Re-synchronize prefix command pointers after hash table modifications.
 *
 * This function refreshes the prefix_cmds[] dispatch array by re-querying the
 * command hash table for each registered prefix command. It ensures that prefix
 * command pointers remain valid after operations that may invalidate or relocate
 * hash table entries (e.g., rehashing, dynamic command registration/removal).
 *
 * The prefix_cmds array provides O(1) dispatch for single-character command
 * leaders (", :, ;, \, #, &) without requiring hash lookups during command
 * processing. This function maintains that optimization after structural changes.
 *
 * @note This should be called after any operation that modifies the command hash
 *       table structure, such as adding/removing commands dynamically or after
 *       table rehashing. It is NOT needed after initial initialization via
 *       register_prefix_cmds() since those pointers are already fresh.
 *
 * @see init_cmdtab() for initial prefix command registration
 * @see register_prefix_cmds() for setting up prefix command entries
 * @see prefix_cmds[] for the global prefix dispatch array (256 entries)
 */
void reset_prefix_cmds(void)
{
	char lookup_key[2] = {0, 0}; /* Single-character lookup string + null terminator */

	/* Re-query hash table for each registered prefix command */
	for (int i = 0; i < 256; i++)
	{
		/* Skip empty slots in the prefix dispatch array */
		if (!prefix_cmds[i])
		{
			continue;
		}

		/* Build single-character key and refresh pointer from hash table */
		lookup_key[0] = (char)i;
		prefix_cmds[i] = (CMDENT *)hashfind(lookup_key, &mushstate.command_htab);
	}
}

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

	/* Fast path: single-character lead-in commands */
	int leadin_index = command[0] & 0xff;

	if ((prefix_cmds[leadin_index] != NULL) && command[0])
	{
		process_cmdent(prefix_cmds[leadin_index], NULL, player, cause, interactive, command, command, args, nargs);
		mushstate.debug_cmd = cmdsave;
		return preserve_cmd;
	}

	/* Check for the HOME command */
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
				move_exit(player, exit, 0, NOGO_MESSAGE, 0);
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

	/* Set up a lowercase command and an arg pointer for the hashed command check */
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

	/* Check for enter and leave aliases, user-defined commands on the player, etc. */
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

	/* At each of the following stages, we check to make sure that we haven't hit a match on a STOP-set object. */
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
				if (Location(player) != Zone(player))
				{
					init_match_check_keys(player, command, TYPE_EXIT);
					match_zone_exit();
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

					if (!got_stop)
					{
						succ += list_check(Contents(Zone(Location(player))), player, AMATCH_CMD, lcbuf, preserve_cmd, 1, &got_stop);
					}
				}
			}
			else if (!got_stop && !succ && mushconf.have_zones && (Zone(Location(player)) != NOTHING))
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

	/* MUX style player */
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

	/* Allow modules to intercept, if still no match. */
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
