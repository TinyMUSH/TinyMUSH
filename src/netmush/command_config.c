/**
 * @file command_config.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration functions for commands, attributes, and aliases
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

int cf_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
	if (!str || !*str)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "No command name provided");
		return -1;
	}

	char *ap = str;
	int set_switch = 0;

	/* Split on whitespace or '/'; the command token ends there. */
	while (*ap && !isspace(*ap) && (*ap != '/'))
	{
		ap++;
	}

	if (*ap == '/')
	{
		set_switch = 1;
		*ap++ = '\0'; /* Terminate the command portion */
	}
	else
	{
		if (*ap)
		{
			*ap++ = '\0'; /* Trim trailing token separator */
		}
	}

	/* Skip any whitespace before the switch/permission string */
	ap = (char *)skip_whitespace(ap);

	CMDENT *cmdp = (CMDENT *)hashfind(str, &mushstate.command_htab);

	if (!cmdp)
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Command", str);
		return -1;
	}

	if (set_switch)
	{
		/* Apply permissions to a specific switch entry */
		return cf_ntab_access((int *)cmdp->switches, ap, extra, player, cmd);
	}

	/* Apply permissions to the command itself */
	return cf_modify_bits(&(cmdp->perms), ap, extra, player, cmd);
}

/**
 * @brief Apply a permission change to every attribute-setter command.
 *
 * Iterates all defined attributes, derives their setter command name (e.g.,
 * "\@name"), and applies the requested bitmask change to each matching
 * command's permissions. If any update fails, the first failed command is
 * restored to its original permissions and the function returns -1.
 *
 * @param vp     Unused
 * @param str    Permission string to apply (cf_modify_bits syntax)
 * @param extra  Bitmask operation selector
 * @param player DBref requesting the change
 * @param cmd    Config directive name (for logging)
 * @return 0 on success, -1 on failure
 */
int cf_acmd_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
	if (!str || !*str)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "No permission string provided");
		return -1;
	}

	char buff[SBUF_SIZE];

	for (ATTR *ap = attr; ap->name; ap++)
	{
		/* Build the setter command name: "@" + lowercase attribute name */
		char *p = buff;
		*p++ = '@';

		for (char *q = (char *)ap->name; *q && (p < (buff + SBUF_SIZE - 1)); p++, q++)
		{
			*p = tolower(*q);
		}

		*p = '\0';

		CMDENT *cmdp = (CMDENT *)hashfind(buff, &mushstate.command_htab);

		if (!cmdp)
		{
			continue; /* Attribute has no associated command */
		}

		int save = cmdp->perms;
		int failure = cf_modify_bits(&(cmdp->perms), str, extra, player, cmd);

		if (failure != 0)
		{
			/* Revert on first failure to avoid partial updates */
			cmdp->perms = save;
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Modify the access flags of a specific attribute.
 *
 * Parses "name perms" where `name` is the attribute to adjust and `perms`
 * follows `cf_modify_bits()` syntax. Looks up the attribute by name and
 * applies the requested bitmask change; logs an error if the attribute is
 * unknown.
 *
 * @param vp     Unused
 * @param str    Attribute name followed by a permission string
 * @param extra  Bitmask operation selector
 * @param player DBref requesting the change
 * @param cmd    Config directive name (for logging)
 * @return 0 on success, -1 on failure
 */
int cf_attr_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
	if (!str || !*str)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "No attribute name provided");
		return -1;
	}

	/* Split into attribute name and permission string */
	char *sp = str;
	while (*sp && !isspace(*sp))
	{
		sp++;
	}

	if (*sp)
	{
		*sp++ = '\0';
	}

	sp = (char *)skip_whitespace(sp);

	ATTR *ap = atr_str(str);

	if (!ap)
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Attribute", str);
		return -1;
	}

	return cf_modify_bits(&(ap->flags), sp, extra, player, cmd);
}

/**
 * @brief Register wildcard attribute patterns and their default flags.
 *
 * Accepts "PATTERN privs" where PATTERN is uppercased and truncated to the
 * maximum attribute name length, and `privs` is a cf_modify_bits() mask to
 * apply when creating attributes that match the pattern. On success, the
 * pattern is prepended to mushconf.vattr_flag_list so later lookups can
 * inherit the configured flags.
 *
 * @param vp     Unused
 * @param str    In-place buffer containing "PATTERN privs"
 * @param extra  Bitmask operation selector
 * @param player DBref requesting the change
 * @param cmd    Config directive name (for logging)
 * @return 0 on success, -1 on failure
 */
int cf_attr_type(int *vp, char *str, long extra, dbref player, char *cmd)
{
	if (!str || !*str)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "No attribute pattern provided");
		return -1;
	}

	char *privs = str;

	/* Uppercase the pattern while scanning for the privilege string. */
	while (*privs && !isspace(*privs))
	{
		*privs = toupper(*privs);
		privs++;
	}

	if (*privs)
	{
		*privs++ = '\0';
		privs = (char *)skip_whitespace(privs);
	}

	if (!*privs)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "No privilege string provided for %s", str);
		return -1;
	}

	/* Enforce maximum attribute-name length. */
	if (strlen(str) >= VNAME_SIZE)
	{
		str[VNAME_SIZE - 1] = '\0';
	}

	/* Evaluate privileges before allocating list node to avoid churn on failure. */
	int data = 0;
	int succ = cf_modify_bits(&data, privs, extra, player, cmd);

	if (succ < 0)
	{
		return -1;
	}

	KEYLIST *kp = (KEYLIST *)XMALLOC(sizeof(KEYLIST), "kp");
	kp->data = data;
	kp->name = XSTRDUP(str, "kp->name");
	kp->next = mushconf.vattr_flag_list;
	mushconf.vattr_flag_list = kp;
	return succ;
}

/**
 * @brief Add a new alias for an existing command (optionally for a specific switch).
 *
 * Accepts two tokens in `str`: `alias` and `original[/switch]`. If a switch is
 * provided, a new CMDENT is created that mirrors the original command and
 * applies the switch's flags; otherwise an alias entry is inserted that points
 * to the existing command.
 *
 * @param vp     Hash table of commands (treated as opaque by callers)
 * @param str    Mutable buffer containing "alias original[/switch]"
 * @param extra  Unused
 * @param player DBref requesting the alias
 * @param cmd    Config directive name (for logging)
 * @return 0 on success, -1 on failure
 */
int cf_cmd_alias(int *vp, char *str, long extra, dbref player, char *cmd)
{
	CMDENT *cmdp = NULL, *cmd2 = NULL;
	NAMETAB *nt = NULL;
	int *hp = NULL;
	char *tokst = NULL;
	char *alias = strtok_r(str, " \t=,", &tokst);
	char *orig = strtok_r(NULL, " \t=,", &tokst);

	if (!alias || !*alias)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "No alias name provided");
		return -1;
	}

	if (!orig || !*orig)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid original for alias %s", alias);
		return -1;
	}

	if (alias[0] == '_' && alias[1] == '_')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias %s would cause @addcommand conflict", alias);
		return -1;
	}

	char *slash = strchr(orig, '/');

	if (slash)
	{
		/* Switch-specific alias: split the command/switch tokens. */
		*slash++ = '\0';

		cmdp = (CMDENT *)hashfind(orig, (HASHTAB *)vp);
		if (!cmdp)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Command", orig);
			return -1;
		}

		nt = find_nametab_ent(player, (NAMETAB *)cmdp->switches, slash);
		if (!nt)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s/%s not found", "Switch", orig, slash);
			return -1;
		}

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

		/* Hook pointers are intentionally not inherited; see note below. */
		cmd2->pre_hook = NULL;
		cmd2->post_hook = NULL;
		cmd2->userperms = NULL;
		cmd2->info.handler = cmdp->info.handler;

		if (hashadd(cmd2->cmdname, (int *)cmd2, (HASHTAB *)vp, 0))
		{
			/* Insertion failed, drop the allocated alias entry. */
			XFREE(cmd2->cmdname);
			XFREE(cmd2);
		}
	}
	else
	{
		/* Simple alias: point the new name at the existing command entry. */
		hp = hashfind(orig, (HASHTAB *)vp);
		if (!hp)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", orig);
			return -1;
		}

		hashadd(alias, hp, (HASHTAB *)vp, HASH_ALIAS);
	}

	return 0;
}

/**
 * @brief List the default flag sets applied when new objects are created.
 *
 * Decodes the configured default flags for each object type (player, room,
 * exit, thing, robot, stripped) and emits a compact table showing what flags
 * new instances receive.
 *
 * @param player DBref of the viewer (used for decode_flags visibility)
 */
