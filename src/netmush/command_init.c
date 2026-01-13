/**
 * @file command_init.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command table initialization and prefix command management
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

#include <ctype.h>

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
