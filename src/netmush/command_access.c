/**
 * @file command_access.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command access control and permission validation
 * @details This module is part of the command subsystem modularization
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
#include <dlfcn.h>
#include <unistd.h>
#include <libgen.h>

/**
 * @brief Validate player permissions against a command's core permission mask.
 *
 * This is the foundational permission check that evaluates a player's eligibility
 * to execute a command based on its core permission mask. The mask encodes multiple
 * permission dimensions that are checked hierarchically:
 *
 * **Permission Mask Components (bitwise-OR combined):**
 *
 * 1. **Static Checks** (evaluated first):
 *    - CA_DISABLED: Command is administratively disabled
 *    - CA_STATIC: Command exists for table purposes only (no runtime execution)
 *
 * 2. **God Bypass**:
 *    - God players and initialization phase bypass ALL subsequent checks
 *    - This allows God to execute any non-disabled command regardless of flags
 *
 * 3. **Privilege Bits** (CA_ISPRIV_MASK: require specific privilege level):
 *    - CA_WIZARD: Full administrative access
 *    - CA_ADMIN: Administrative powers without wizard restrictions
 *    - CA_BUILDER: Object creation and modification rights
 *    - CA_STAFF: Extended player privileges
 *    - CA_HEAD: Group leadership abilities
 *    - CA_IMMORTAL: Unkillable player status
 *    - CA_MODULE_OK: Module-specific capabilities
 *    - CA_GOD: God-only (non-combinable, already handled in bypass)
 *
 * 4. **Marker Bits** (CA_MARKER_MASK: require specific markers 0-9):
 *    - CA_MARKER0 through CA_MARKER9: Custom game-specific flags
 *    - Allows fine-grained permission control without consuming privilege bits
 *
 * 5. **Exclusion Bits** (CA_ISNOT_MASK: deny if player has these):
 *    - CA_NO_HAVEN: Deny players in HAVEN mode
 *    - CA_NO_ROBOT: Deny robot players
 *    - CA_NO_SLAVE: Deny slaved objects
 *    - CA_NO_SUSPECT: Deny suspect-flagged players
 *    - CA_NO_GUEST: Deny guest accounts
 *    - Wizards bypass all exclusion checks
 *
 * **Permission Evaluation Logic:**
 * - Disabled/static commands always fail (even for God)
 * - God and initialization bypass all other checks
 * - Privilege bits and marker bits use OR semantics (match any)
 * - Exclusion bits use OR semantics (fail if any match, unless wizard)
 *
 * **Common Use Cases:**
 * - CA_WIZARD: \@shutdown, \@pcreate, \@sql
 * - CA_BUILDER: \@dig, \@create, \@link
 * - CA_NO_GUEST: \@page, \@mail, \@chown
 * - CA_MARKER0 | CA_BUILDER: Custom command requiring marker OR builder
 *
 * @param player Database reference of the player requesting command execution
 * @param mask   Permission bitmask encoding privilege requirements, markers,
 *               and exclusions (combined via bitwise OR)
 *
 * @return true if player has permission to execute the command (all checks passed),
 *         false if any check failed or command is disabled
 *
 * @note This function implements only core permission checking. Module-specific
 *       and user-defined permission checks are handled separately.
 *
 * @note The CA_GOD check is performed early as an optimization; if God-only
 *       permission is set and the player is not God (already bypassed), we
 *       can immediately deny access without evaluating other bits.
 *
 * @warning Modifying permission semantics affects game security. Core permissions
 *          cannot be circumvented by user-defined permissions.
 *
 * @see check_mod_access() for module permission validation
 * @see check_userdef_access() for user-defined softcode permission checking
 * @see check_cmd_access() for combined permission validation
 * @see CA_* constants in constants.h for permission bit definitions
 */
bool check_access(dbref player, int mask)
{
	/* Early exits for disabled commands and God-level bypass */
	if (mask & (CA_DISABLED | CA_STATIC))
	{
		return false; /* Command is disabled or static (no runtime execution) */
	}

	if (God(player) || mushstate.initializing)
	{
		return true; /* God and initialization phase bypass all checks */
	}

	/* Extract privilege and marker components from permission mask */
	int combined_mask = mask & (CA_ISPRIV_MASK | CA_MARKER_MASK);

	/* God-only permission check (non-combinable with other privileges) */
	if (combined_mask == CA_GOD)
	{
		return false; /* Only God can pass this check, and we know player is not God */
	}

	/* Check privilege bits and/or marker bits if either is present */
	if (combined_mask)
	{
		int priv_mask = mask & CA_ISPRIV_MASK;
		int marker_mask = mask & CA_MARKER_MASK;

		/* Case 1: Only privilege bits set (no markers) */
		if (priv_mask && !marker_mask)
		{
			if (!(((mask & CA_WIZARD) && Wizard(player)) ||
				  ((mask & CA_ADMIN) && WizRoy(player)) ||
				  ((mask & CA_BUILDER) && Builder(player)) ||
				  ((mask & CA_STAFF) && Staff(player)) ||
				  ((mask & CA_HEAD) && Head(player)) ||
				  ((mask & CA_IMMORTAL) && Immortal(player)) ||
				  ((mask & CA_MODULE_OK) && Can_Use_Module(player))))
			{
				return false; /* Player lacks required privilege level */
			}
		}
		/* Case 2: Only marker bits set (no privileges) */
		else if (!priv_mask && marker_mask)
		{
			if (!(((mask & CA_MARKER0) && H_Marker0(player)) ||
				  ((mask & CA_MARKER1) && H_Marker1(player)) ||
				  ((mask & CA_MARKER2) && H_Marker2(player)) ||
				  ((mask & CA_MARKER3) && H_Marker3(player)) ||
				  ((mask & CA_MARKER4) && H_Marker4(player)) ||
				  ((mask & CA_MARKER5) && H_Marker5(player)) ||
				  ((mask & CA_MARKER6) && H_Marker6(player)) ||
				  ((mask & CA_MARKER7) && H_Marker7(player)) ||
				  ((mask & CA_MARKER8) && H_Marker8(player)) ||
				  ((mask & CA_MARKER9) && H_Marker9(player))))
			{
				return false; /* Player lacks required marker flags */
			}
		}
		/* Case 3: Both privilege and marker bits set (OR semantics) */
		else
		{
			if (!(((mask & CA_WIZARD) && Wizard(player)) ||
				  ((mask & CA_ADMIN) && WizRoy(player)) ||
				  ((mask & CA_BUILDER) && Builder(player)) ||
				  ((mask & CA_STAFF) && Staff(player)) ||
				  ((mask & CA_HEAD) && Head(player)) ||
				  ((mask & CA_IMMORTAL) && Immortal(player)) ||
				  ((mask & CA_MODULE_OK) && Can_Use_Module(player)) ||
				  ((mask & CA_MARKER0) && H_Marker0(player)) ||
				  ((mask & CA_MARKER1) && H_Marker1(player)) ||
				  ((mask & CA_MARKER2) && H_Marker2(player)) ||
				  ((mask & CA_MARKER3) && H_Marker3(player)) ||
				  ((mask & CA_MARKER4) && H_Marker4(player)) ||
				  ((mask & CA_MARKER5) && H_Marker5(player)) ||
				  ((mask & CA_MARKER6) && H_Marker6(player)) ||
				  ((mask & CA_MARKER7) && H_Marker7(player)) ||
				  ((mask & CA_MARKER8) && H_Marker8(player)) ||
				  ((mask & CA_MARKER9) && H_Marker9(player))))
			{
				return false; /* Player must match at least one privilege OR marker */
			}
		}
	}

	/* Check exclusion bits (things player must NOT have, unless they're a wizard) */
	if ((mask & CA_ISNOT_MASK) && !Wizard(player))
	{
		if (((mask & CA_NO_HAVEN) && Player_haven(player)) ||
			((mask & CA_NO_ROBOT) && Robot(player)) ||
			((mask & CA_NO_SLAVE) && Slave(player)) ||
			((mask & CA_NO_SUSPECT) && Suspect(player)) ||
			((mask & CA_NO_GUEST) && Guest(player)))
		{
			return false; /* Player has a forbidden flag/state */
		}
	}

	return true; /* All permission checks passed */
}

/**
 * @brief Validate player permissions through dynamically loaded module permission handlers.
 *
 * This function extends the core permission system by allowing modules to register
 * custom permission validation handlers. It iterates through all registered module
 * permission functions and executes them sequentially as a permission check chain.
 * All handlers must approve access for the check to succeed (logical AND).
 *
 * Each handler receives the player dbref and returns a boolean indicating whether
 * permission is granted. The first handler to deny access short-circuits the chain
 * and causes the entire check to fail.
 *
 * @param player The database reference of the player being validated
 * @param xperms Pointer to EXTFUNCS structure containing the array of module
 *               permission handler function pointers and handler count
 *
 * @return true if all module handlers approve access (or no handlers registered),
 *         false if any handler denies access or a handler pointer is invalid
 *
 * @note NULL handler pointers in the array are skipped silently (not an error).
 *       This allows modules to be unloaded without disrupting the handler chain.
 *
 * @note If xperms->num_funcs is 0 or all handlers are NULL, returns true by default.
 *       Empty handler chains impose no additional restrictions.
 *
 * @warning Module handlers must not have side effects beyond permission checking.
 *          They should be fast, reentrant, and avoid blocking operations.
 *
 * @see check_access() for core permission validation
 * @see check_cmd_access() for combining core and module permission checks
 * @see EXTFUNCS structure definition for handler registration interface
 */
bool check_mod_access(dbref player, EXTFUNCS *xperms)
{
	/* Iterate through all registered module permission handlers */
	for (int i = 0; i < xperms->num_funcs; i++)
	{
		/* Skip NULL handler slots (unloaded modules or registration gaps) */
		if (!xperms->ext_funcs[i])
		{
			continue;
		}

		/* Invoke module handler; fail immediately if permission denied */
		if (!(xperms->ext_funcs[i]->handler)(player))
		{
			return false; /* Module denied access - short-circuit chain */
		}
	}

	return true; /* All module handlers approved or no handlers registered */
}

/**
 * @brief Evaluate user-defined command permissions through in-game attribute softcode.
 *
 * This function implements custom permission checking by evaluating arbitrary MUSH
 * softcode stored in an attribute. It enables game administrators to define complex,
 * context-aware permission rules without modifying C code, using the full power of
 * the MUSH scripting language.
 *
 * The function retrieves an attribute from a specified object, evaluates its contents
 * as softcode with the player as the enactor, and interprets the result as a boolean
 * permission check. This allows for dynamic permissions based on:
 * - Player statistics (level, experience, inventory)
 * - Time-of-day restrictions or event schedules
 * - Complex relationships between objects
 * - Quest completion or achievement status
 * - Zone permissions or area restrictions
 *
 * **Evaluation Context:**
 * - Enactor: The player requesting permission
 * - Executor: The object holding the permission attribute (hookp->thing)
 * - Command args: Available to the softcode via %0, %1, etc.
 * - Global registers: Preserved across the evaluation (unlike pre/post hooks)
 *
 * **Return Logic:**
 * - Missing attribute → false (deny access)
 * - Empty attribute → false (deny access)
 * - Non-zero result → true (grant access)
 * - Zero/"" result → false (deny access)
 *
 * @param player The database reference of the player requesting command access
 * @param hookp  Pointer to HOOKENT structure specifying the object (thing) and
 *               attribute (atr) containing the permission evaluation softcode
 * @param cargs  Array of command argument strings passed to the softcode (%0, %1, ...)
 * @param ncargs Number of command arguments in the cargs array
 *
 * @return true if the softcode evaluates to a non-zero/non-empty value (boolean true),
 *         false if attribute is missing, empty, or evaluates to zero/empty
 *
 * @note This function always preserves global registers (unlike pre/post command hooks
 *       which may use CS_PRESERVE or CS_PRIVATE). This ensures the permission check
 *       doesn't corrupt the calling context's register state.
 *
 * @note The softcode is evaluated with EV_EVAL | EV_FCHECK | EV_TOP flags, enabling
 *       full function evaluation, permission checking, and top-level parsing.
 *
 * @warning The softcode runs in the security context of hookp->thing (not the player),
 *          so the attribute owner controls the permission logic's privilege level.
 *
 * @see check_cmd_access() for combining user-defined and core permission checks
 * @see check_access() for core permission mask validation
 * @see HOOKENT structure for hook registration interface
 */
bool check_userdef_access(dbref player, HOOKENT *hookp, char *cargs[], int ncargs)
{
	char *buf = NULL, *bp = NULL, *tstr = NULL, *str = NULL;
	int aflags = 0, alen = 0;
	dbref aowner = NOTHING;
	GDATA *preserve = NULL;
	bool result = false;

	/* Retrieve permission attribute from the specified object */
	tstr = atr_get(hookp->thing, hookp->atr, &aowner, &aflags, &alen);

	/* Missing attribute denies access */
	if (!tstr)
	{
		return false;
	}

	/* Empty attribute denies access */
	if (!*tstr)
	{
		XFREE(tstr);
		return false;
	}

	/* Preserve global registers during permission evaluation */
	preserve = save_global_regs("check_userdef_access");

	/* Evaluate the attribute softcode with player as enactor */
	buf = XMALLOC(LBUF_SIZE, "buf");
	bp = buf;
	str = tstr;
	eval_expression_string(buf, &bp, hookp->thing, player, player,
						   EV_EVAL | EV_FCHECK | EV_TOP, &str, cargs, ncargs);

	/* Restore global registers to pre-evaluation state */
	restore_global_regs("check_userdef_access", preserve);

	/* Convert softcode result to boolean (0/"" → false, anything else → true) */
	result = xlate(buf);

	/* Clean up allocated buffers */
	XFREE(buf);
	XFREE(tstr);

	return result;
}

/**
 * @brief Validate command execution permission by combining core, module, and user-defined checks.
 *
 * This is the primary command permission validation function that orchestrates all
 * permission checking subsystems. It performs a multi-stage authorization check:
 *
 * 1. **Core Permission Check**: Validates against the command's base permission mask
 *    (privileges, markers, exclusions) via check_access()
 *
 * 2. **User-Defined Permission Check** (if configured): Evaluates custom softcode
 *    permission attribute via check_userdef_access() when cmdp->userperms is set
 *
 * 3. **God Override**: God players always bypass user-defined permission checks
 *    (but still must pass core permission checks)
 *
 * The permission logic follows this evaluation order:
 * - If core permissions fail → deny access immediately
 * - If core permissions pass AND no user-defined perms set → grant access
 * - If core permissions pass AND user-defined perms set:
 *   - Check user-defined permission softcode
 *   - If softcode denies AND player is not God → deny access
 *   - If softcode approves OR player is God → grant access
 *
 * This design allows game administrators to layer custom permission logic on top of
 * the core permission system without bypassing fundamental access controls. Core
 * permissions act as a baseline security gate that cannot be circumvented by softcode.
 *
 * **Common Use Cases:**
 * - Commands with fixed privilege requirements (wizard-only, builder-only, etc.)
 * - Commands with dynamic requirements (quest status, faction membership, etc.)
 * - Commands with time-based restrictions or resource costs
 * - Commands requiring complex multi-condition authorization
 *
 * @param player Database reference of the player requesting command execution
 * @param cmdp   Pointer to command entry containing permission mask and optional
 *               user-defined permission hook
 * @param cargs  Array of command argument strings passed to user-defined permission
 *               softcode for context-aware authorization (%0, %1, ...)
 * @param ncargs Number of command arguments in the cargs array
 *
 * @return true if player has permission to execute the command (all checks passed),
 *         false if any permission check failed
 *
 * @note This function is the single point of entry for all command permission checks.
 *       Direct calls to check_access() or check_userdef_access() bypass the integrated
 *       permission model and should be avoided.
 *
 * @note God bypass only applies to user-defined permissions, not core permissions.
 *       Even God must have the appropriate privilege flags for core-restricted commands.
 *
 * @warning Calling code is responsible for displaying appropriate permission denied
 *          messages to the user. This function only returns authorization status.
 *
 * @see check_access() for core permission mask validation
 * @see check_userdef_access() for user-defined softcode permission evaluation
 * @see process_cmdent() for command dispatch and permission enforcement
 * @see CMDENT.perms for core permission mask structure
 * @see CMDENT.userperms for user-defined permission hook pointer
 */
bool check_cmd_access(dbref player, CMDENT *cmdp, char *cargs[], int ncargs)
{
	/* Core permission check (privilege/marker/exclusion mask validation) */
	if (!check_access(player, cmdp->perms))
	{
		return false; /* Core permissions failed - deny access */
	}

	/* User-defined permission check (if configured) with God override */
	if (cmdp->userperms)
	{
		/* God bypasses user-defined permissions, non-God must pass softcode check */
		if (!God(player) && !check_userdef_access(player, cmdp->userperms, cargs, ncargs))
		{
			return false; /* User-defined permission check failed - deny access */
		}
	}

	return true; /* All permission checks passed - grant access */
}
