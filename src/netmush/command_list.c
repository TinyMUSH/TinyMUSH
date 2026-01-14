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

/**
 * @brief Display all built-in and module commands visible to the player.
 *
 * Lists command names from the core command table and any loaded modules,
 * filtered by the caller's permissions. Commands marked CF_DARK are hidden.
 * Players also see the logout command table.
 *
 * @param player DBref of the requesting player
 */
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
 * @brief List all attributes visible to the player in a single line.
 *
 * Builds output starting with "Attributes:" followed by space-separated
 * attribute names. Filtering via `See_attr` hides restricted attributes;
 * truncation prevents buffer overrun.
 *
 * @param player DBref of the requesting player
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
 * @brief Print command permissions from a command table for visible entries.
 *
 * Iterates the command table and displays each accessible command's permission
 * mask, skipping CF_DARK entries. User-defined permissions are annotated with
 * the source object and attribute.
 *
 * @param player DBref of the requesting player
 * @param ctab   Pointer to the command table to enumerate
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
 * @brief Display comprehensive command permission list for the player.
 *
 * Emits a formatted table showing permission masks for:
 * - Built-in commands
 * - Module-exported commands
 * - Attribute-setter commands (@name, @desc, etc.)
 *
 * Only shows commands the player can access; CF_DARK entries are hidden.
 *
 * @param player DBref of the requesting player
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
 * @brief Helper to emit switches for one command table.
 *
 * Iterates the table and displays switch lists for accessible commands,
 * skipping entries without switches or marked CF_DARK. Called by
 * `list_cmdswitches()` for core and module tables.
 *
 * @param player DBref of the requesting player
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
 * @brief Display switches for all accessible commands.
 *
 * Emits a formatted table of switch names for built-in and module commands
 * the player can access. CF_DARK entries and permission-failed commands are
 * hidden.
 *
 * @param player DBref of the requesting player
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
 * @brief Display attribute names and permission flags.
 *
 * Emits a formatted table showing each readable attribute and its flag bitmask,
 * filtered by `Read_attr` to hide restricted entries. Includes header and footer.
 *
 * @param player DBref of the requesting player
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
 * @brief Display wildcard attribute type patterns and flags.
 *
 * Emits a formatted table of vattr flag patterns (e.g., NAME*, DESC*) with
 * their associated permissions. Notifies the player if no patterns are defined.
 *
 * @param player DBref of the requesting player
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
