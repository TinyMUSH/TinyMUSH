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

/* Command handler function pointers for various call sequences */
void (*handler_cs_no_args)(dbref, dbref, int);													   /*!< Handler for no-arg commands */
void (*handler_cs_one_args)(dbref, dbref, int, char *);											   /*!< Handler for one-arg commands */
void (*handler_cs_one_args_unparse)(dbref, char *);												   /*!< Handler for one-arg unparsed commands */
void (*handler_cs_one_args_cmdargs)(dbref, dbref, int, char *, char *[], int);					   /*!< Handler for one-arg commands with cmdargs */
void (*handler_cs_two_args)(dbref, dbref, int, char *, char *);									   /*!< Handler for two-arg commands */
void (*handler_cs_two_args_cmdargs)(dbref, dbref, int, char *, char *, char *[], int);			   /*!< Handler for two-arg commands with cmdargs */
void (*handler_cs_two_args_argv)(dbref, dbref, int, char *, char *[], int);						   /*!< Handler for two-arg commands with argv */
void (*handler_cs_two_args_cmdargs_argv)(dbref, dbref, int, char *, char *[], int, char *[], int); /*!< Handler for two-arg commands with cmdargs and argv */

/* Command hash table and prefix command array */
CMDENT *prefix_cmds[256];										 /*!< Builtin prefix commands */
CMDENT *goto_cmdp, *enter_cmdp, *leave_cmdp, *internalgoto_cmdp; /*!< Commonly used command pointers */

/**
 * @brief Initialize command hash table and register all available commands.
 *
 * Performs complete MUSH command system initialization:
 * - Creates command hash table sized by `mushconf.hash_factor`
 * - Generates attribute-setter commands (@name, @desc, etc.) from the attribute table
 * - Registers all builtin commands from `command_table`
 * - Populates prefix command dispatch array for single-char leaders (", :, ;, \, #, &)
 * - Caches frequently-used command pointers (goto, enter, leave, internalgoto)
 *
 * Attribute-setters are dynamically allocated with lowercased names, CA_NO_GUEST | CA_NO_SLAVE
 * permissions (plus CA_WIZARD if AF_WIZARD/AF_MDARK), CS_TWO_ARG call sequence, and __@attr aliases.
 * All commands get double-underscore aliases for programmatic invocation.
 *
 * @note Must be called during server initialization before command processing begins.
 *
 * @see reset_prefix_cmds() to refresh prefix pointers after hash modifications
 * @see command_table for static builtin command definitions
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
 * @brief Refresh prefix command pointers after hash table modifications.
 *
 * Re-queries the command hash for each registered prefix in the `prefix_cmds[]` array,
 * ensuring pointers remain valid after rehashing or dynamic command changes. Maintains
 * O(1) dispatch for single-character leaders (", :, ;, \, #, &) without runtime lookups.
 *
 * @note Call after adding/removing commands dynamically or rehashing. Not needed after
 *       initial `init_cmdtab()` since `register_prefix_cmds()` sets fresh pointers.
 *
 * @see init_cmdtab() for initial prefix command registration
 * @see prefix_cmds for the global 256-entry dispatch array
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
