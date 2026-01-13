/**
 * @file command_list.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Admin command table inspection and attribute management displays
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
#include <string.h>
#include <pcre2.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>

void list_cmdtable(dbref player)
{
	CMDENT *cmdp = NULL, *modcmds = NULL;
	MODULE *mp = NULL;
	char *buf = XMALLOC(LBUF_SIZE, "buf");

	XSPRINTF(buf, "Built-in commands:");
	for (cmdp = command_table; cmdp->cmdname; cmdp++)
	{
		if (!check_access(player, cmdp->perms) || (cmdp->perms & CF_DARK))
		{
			continue;
		}

		XSPRINTFCAT(buf, " %s", cmdp->cmdname);
	}

	/* Players get the list of logged-out cmds too */
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
				if (!check_access(player, cmdp->perms) || (cmdp->perms & CF_DARK))
				{
					continue;
				}

				XSPRINTFCAT(buf, " %s", cmdp->cmdname);
			}
			notify(player, buf);
		}

		XFREE(modname);
	}

	XFREE(buf);
}

/**
 * @brief Show attribute names the player is allowed to see.
 *
 * Builds a single line beginning with "Attributes:" followed by each attribute
 * name the caller can see (filtered via `See_attr`). Hidden attributes are
 * skipped; the list is truncated if it would exceed the notification buffer.
 *
 * @param player Database reference of the requesting player
 */
void list_attrtable(dbref player)
{
	ATTR *ap = NULL;
	char *cp = NULL, *bp = NULL, *buf = NULL;
	char *buf_end = NULL;

	bp = buf = XMALLOC(LBUF_SIZE, "buf");
	buf_end = buf + LBUF_SIZE - 1; /* Keep space for terminator */

	for (cp = (char *)"Attributes:"; *cp; cp++)
	{
		*bp++ = *cp;
	}

	for (ap = attr; ap->name; ap++)
	{
		if (See_attr(player, player, ap, player, 0))
		{
			/* Ensure we never overrun the output buffer */
			size_t needed = 1 + strlen(ap->name); /* leading space + name */
			if ((bp + needed) >= buf_end)
			{
				break;
			}

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
 * @brief Emit visible command permissions from a command table.
 *
 * Iterates a command table (core or module) and prints each command the caller
 * can access, skipping dark entries. If a command uses user-defined permissions
 * (`userperms`), the output annotates which object/attribute provides them.
 *
 * @param player Database reference of the requesting player
 * @param ctab   Pointer to the command table to list
 */
void helper_list_cmdaccess(dbref player, CMDENT *ctab)
{
	CMDENT *cmdp = NULL;
	ATTR *ap = NULL;

	for (cmdp = ctab; cmdp->cmdname; cmdp++)
	{
		if (!check_access(player, cmdp->perms) || (cmdp->perms & CF_DARK))
		{
			continue;
		}

		if (cmdp->userperms)
		{
			ap = atr_num(cmdp->userperms->atr);
			/* Annotate the source of user-defined permissions; fallback if missing */
			const char *attr_name = ap ? ap->name : "?BAD?";
			listset_nametab(player, access_nametab, cmdp->perms, true, "%-26.26s user(#%d/%s)", cmdp->cmdname, cmdp->userperms->thing, attr_name);
		}
		else
		{
			listset_nametab(player, access_nametab, cmdp->perms, true, "%-26.26s ", cmdp->cmdname);
		}
	}
}

/**
 * @brief Display command permission masks the caller can see.
 *
 * Prints a header row, then lists:
 * - All built-in commands visible to the caller
 * - Commands exported by loaded modules
 * - Attribute-setter commands ("@attr") that exist in the command table
 *
 * Entries hidden by `CF_DARK` or failing `check_access()` are skipped so the
 * output only shows commands the player can actually run or inspect.
 *
 * @param player Database reference of the requesting player
 */
void list_cmdaccess(dbref player)
{
	CMDENT *cmdp = NULL, *ctab = NULL;
	ATTR *ap = NULL;
	MODULE *mp = NULL;
	char *p = NULL, *q = NULL, *buff = XMALLOC(SBUF_SIZE, "buff");

	notify(player, "Command                    Permissions");
	notify(player, "-------------------------- ----------------------------------------------------");

	/* Core command table */
	helper_list_cmdaccess(player, command_table);

	/* Module command tables (if exported) */
	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		p = XASPRINTF("p", "mod_%s_%s", mp->modname, "cmdtable");

		if ((ctab = (CMDENT *)dlsym(mp->handle, p)) != NULL)
		{
			helper_list_cmdaccess(player, ctab);
		}
		XFREE(p);
	}

	/* Attribute-setter commands ("\@name", "\@desc", etc.) */
	for (ap = attr; ap->name; ap++)
	{
		if (ap->flags & AF_NOCMD)
		{
			continue; /* Attribute is not exposed as a command */
		}

		size_t name_len = strlen(ap->name);
		if ((name_len + 2) >= SBUF_SIZE)
		{
			continue; /* Avoid buffer overflow on extremely long names */
		}

		p = buff;
		*p++ = '@';

		for (q = (char *)ap->name; *q; p++, q++)
		{
			*p = tolower(*q);
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
 * @brief Print visible switches for a command table.
 *
 * Walks a command table and displays each command's switch set if the caller
 * can access it, skipping entries that define no switches or are marked dark.
 * Used by `list_cmdswitches()` for both built-in and module command tables.
 *
 * @param player Database reference of the requesting player
 * @param ctab   Null-terminated command table to enumerate
 */
static void emit_cmdswitches_for_table(dbref player, CMDENT *ctab)
{
	if (!ctab)
	{
		return; /* Nothing to list */
	}

	for (CMDENT *cmdp = ctab; cmdp->cmdname; cmdp++)
	{
		/* Skip commands without switches first to avoid deeper checks */
		if (!cmdp->switches)
		{
			continue;
		}

		/* Enforce permission and visibility filters */
		if ((cmdp->perms & CF_DARK) || !check_access(player, cmdp->perms))
		{
			continue;
		}

		/* Emit aligned command name followed by its switch list */
		display_nametab(player, cmdp->switches, false, "%-16.16s", cmdp->cmdname);
	}
}

/**
 * @brief List switches for every command visible to the caller.
 *
 * Prints switch names for all built-in commands and any module-exported
 * command tables the player can access. Entries hidden by `CF_DARK` or failing
 * `check_access()` are omitted so the output only shows runnable commands.
 *
 * @param player Database reference of the requesting player
 */
void list_cmdswitches(dbref player)
{
	CMDENT *ctab = NULL;
	MODULE *mp = NULL;
	char symname[MBUF_SIZE];

	notify(player, "Command          Switches");
	notify(player, "---------------- ---------------------------------------------------------------");

	/* Built-in command table */
	emit_cmdswitches_for_table(player, command_table);

	/* Module command tables (if they export one) */
	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		/* Symbol name: mod_<module>_cmdtable */
		XSNPRINTF(symname, MBUF_SIZE, "mod_%s_%s", mp->modname, "cmdtable");

		if ((ctab = (CMDENT *)dlsym(mp->handle, symname)) != NULL)
		{
			emit_cmdswitches_for_table(player, ctab);
		}
	}

	notify(player, "--------------------------------------------------------------------------------");
}

/**
 * @brief List attribute visibility and flags for the caller.
 *
 * Shows each attribute the player may read and the associated flag bitmask,
 * skipping hidden attributes. Output is bracketed by a header/footer for
 * readability.
 *
 * @param player Database reference of the requesting player
 */
void list_attraccess(dbref player)
{
	notify(player, "Attribute                  Permissions");
	notify(player, "-------------------------- ----------------------------------------------------");

	for (ATTR *ap = attr; ap->name; ap++)
	{
		/* Only display attributes visible to the caller */
		if (!Read_attr(player, player, ap, player, 0))
		{
			continue;
		}

		listset_nametab(player, attraccess_nametab, ap->flags, true, "%-26.26s ", ap->name);
	}

	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List wildcard attribute patterns and their flags.
 *
 * Displays all configured vattr flag patterns (e.g., NAME*, DESC*) and the
 * permissions attached to each. If none are defined, notifies the caller and
 * returns early.
 *
 * @param player Database reference of the requesting player
 */
void list_attrtypes(dbref player)
{
	if (!mushconf.vattr_flag_list)
	{
		notify(player, "No attribute type patterns defined.");
		return;
	}

	notify(player, "Attribute                  Permissions");
	notify(player, "-------------------------- ----------------------------------------------------");

	for (KEYLIST *kp = mushconf.vattr_flag_list; kp != NULL; kp = kp->next)
	{
		char buff[MBUF_SIZE];

		/* Format the pattern name once, avoiding heap churn per entry */
		XSNPRINTF(buff, sizeof(buff), "%-26.26s ", kp->name);
		listset_nametab(player, attraccess_nametab, kp->data, true, buff);
	}

	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Update permissions on a command or one of its switches.
 *
 * Accepts a token of the form "command" or "command/switch", looks up the
 * command in the global hash, and applies `cf_modify_bits()` (for commands)
 * or `cf_ntab_access()` (for switches). Missing commands are logged.
 *
 * @param vp     Unused
 * @param str    Command name, optionally suffixed with "/switch"
 * @param extra  Bitmask operation selector
 * @param player DBref requesting the change
 * @param cmd    Config directive name (for logging)
 * @return 0 on success, -1 on failure
 */
