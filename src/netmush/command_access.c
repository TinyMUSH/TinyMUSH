/**
 * @file command_access.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command permission validation, hooks, and access control
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

/**
 * @brief Validate whether a player has permission to execute a command or access a resource.
 *
 * This function implements a hierarchical permission checking system for MUSH commands,
 * evaluating multiple permission tiers in the following order:
 *
 * 1. **Disabled/Static Check**: Commands marked CA_DISABLED or CA_STATIC are never accessible
 * 2. **God/Init Bypass**: God players and server initialization always succeed
 * 3. **God-Only Check**: CA_GOD mask requires the God player specifically (non-combinable)
 * 4. **Privilege Bits**: Checks player privilege level against mask requirements:
 *    - CA_WIZARD, CA_ADMIN, CA_BUILDER, CA_STAFF, CA_HEAD, CA_IMMORTAL, CA_MODULE_OK
 * 5. **Marker Bits**: Checks player marker flags (CA_MARKER0..CA_MARKER9) for custom permissions
 * 6. **Combined Privileges+Markers**: When both are set, player must match at least one from either category
 * 7. **Exclusion Bits**: Checks CA_ISNOT_MASK restrictions (NO_HAVEN, NO_ROBOT, NO_SLAVE, NO_SUSPECT, NO_GUEST)
 *
 * Permission masks are bitfield combinations allowing complex access control patterns.
 * The function uses short-circuit evaluation to minimize flag checking overhead.
 *
 * @param player The database reference of the player requesting access
 * @param mask   Bitfield permission mask combining CA_* flags from the command entry
 *
 * @return true if player has sufficient permissions, false otherwise
 *
 * @note Calling functions are responsible for displaying permission denied messages.
 *       This function only performs the authorization check.
 *
 * @warning God-only permissions (CA_GOD alone) cannot be combined with other privilege
 *          requirements. The check will fail for any non-God player regardless of other flags.
 *
 * @see check_cmd_access() for command-specific permission validation
 * @see check_userdef_access() for user-defined permission hooks
 * @see CMDENT.perms for command permission mask structure
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
 * @brief Execute pre-command or post-command hook softcode with register context management.
 *
 * This function implements the command hook system that allows in-game softcode to be
 * triggered before (pre-hook) or after (post-hook) command execution. Hooks enable
 * game administrators to intercept, log, modify, or react to command execution without
 * modifying C code.
 *
 * The function retrieves a hook attribute from a specified object and evaluates it as
 * MUSH softcode with the player as the enactor. The key distinction from user-defined
 * permissions is the register context management strategy:
 *
 * **Register Management Modes:**
 * - **CS_PRESERVE**: Saves and restores global registers (q0-q9, %0-%9)
 *   - Hook sees and can modify registers
 *   - Changes are discarded after hook execution
 *   - Used for hooks that need to read register state but shouldn't persist changes
 *
 * - **CS_PRIVATE**: Creates isolated register context for the hook
 *   - Hook gets a fresh, empty register set
 *   - Original registers are completely hidden during hook execution
 *   - Private register allocations are cleaned up after hook completes
 *   - Used for hooks that should not see or affect the calling context's registers
 *
 * - **Neither flag**: No register preservation (direct modification)
 *   - Hook modifies registers in place
 *   - Changes persist after hook execution
 *   - Rarely used due to side-effect risks
 *
 * **Evaluation Context:**
 * - Enactor: The player who executed the command that triggered the hook
 * - Executor: The object holding the hook attribute (hp->thing)
 * - Command args: Available to softcode via %0, %1, etc.
 * - Evaluation flags: EV_EVAL | EV_FCHECK | EV_TOP (full function evaluation)
 *
 * **Common Use Cases:**
 * - Pre-hooks: Access control, prerequisite validation, cost deduction, logging
 * - Post-hooks: Achievement tracking, state updates, notifications, auditing
 *
 * @param hp         Pointer to HOOKENT structure specifying the object (thing) and
 *                   attribute (atr) containing the hook softcode to execute
 * @param save_globs Register context mode: CS_PRESERVE (save/restore) or CS_PRIVATE
 *                   (isolated context), or 0 (direct modification)
 * @param player     Database reference of the player who triggered the command (enactor)
 * @param cause      Database reference of the entity that caused the command execution
 *                   (currently unused but reserved for future causality tracking)
 * @param cargs      Array of command argument strings passed to hook softcode (%0, %1, ...)
 * @param ncargs     Number of command arguments in the cargs array
 *
 * @note This function does not return a value. Hook evaluation results are discarded
 *       after execution completes (hooks produce side effects, not permission results).
 *
 * @note CS_PRIVATE mode performs extensive cleanup: deallocates all q-registers,
 *       x-registers (named registers), and their associated name/length arrays.
 *
 * @warning Hooks without CS_PRESERVE or CS_PRIVATE will modify the calling context's
 *          registers directly, potentially causing unexpected side effects in the
 *          command implementation.
 *
 * @see check_userdef_access() for permission checking hooks (always uses CS_PRESERVE)
 * @see process_cmdent() for hook invocation during command execution
 * @see HOOKENT structure for hook registration interface
 */
void process_hook(HOOKENT *hp, int save_globs, dbref player, dbref cause, char *cargs[], int ncargs)
{
	char *buf = NULL, *bp = NULL, *tstr = NULL, *str = NULL;
	int aflags = 0, alen = 0;
	dbref aowner = NOTHING;
	GDATA *preserve = NULL;

	/* Retrieve hook attribute from the specified object */
	tstr = atr_get(hp->thing, hp->atr, &aowner, &aflags, &alen);
	str = tstr;

	/* CS_PRESERVE: Save current register state for restoration after hook execution */
	if (save_globs & CS_PRESERVE)
	{
		preserve = save_global_regs("process_hook");
	}
	/* CS_PRIVATE: Create isolated register context by hiding current registers */
	else if (save_globs & CS_PRIVATE)
	{
		preserve = mushstate.rdata; /* Save current register context pointer */
		mushstate.rdata = NULL;		/* Clear context to force fresh allocation */
	}

	/* Evaluate hook softcode with player as enactor */
	buf = XMALLOC(LBUF_SIZE, "buf");
	bp = buf;
	eval_expression_string(buf, &bp, hp->thing, player, player,
						   EV_EVAL | EV_FCHECK | EV_TOP, &str, cargs, ncargs);

	/* Hook result is discarded (hooks produce side effects only) */
	XFREE(buf);

	/* CS_PRESERVE: Restore original register state, discarding hook modifications */
	if (save_globs & CS_PRESERVE)
	{
		restore_global_regs("process_hook", preserve);
	}
	/* CS_PRIVATE: Deallocate private register context and restore original */
	else if (save_globs & CS_PRIVATE)
	{
		/* Clean up private register allocations if hook created any */
		if (mushstate.rdata)
		{
			/* Free all q-register contents (q0-q9 style registers) */
			for (int z = 0; z < mushstate.rdata->q_alloc; z++)
			{
				XFREE(mushstate.rdata->q_regs[z]);
			}

			/* Free all x-register contents (named registers) */
			for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
			{
				XFREE(mushstate.rdata->x_names[z]); /* Register name strings */
				XFREE(mushstate.rdata->x_regs[z]);	/* Register value strings */
			}

			/* Free register array structures */
			XFREE(mushstate.rdata->q_regs);	 /* Q-register value array */
			XFREE(mushstate.rdata->q_lens);	 /* Q-register length array */
			XFREE(mushstate.rdata->x_names); /* X-register name array */
			XFREE(mushstate.rdata->x_regs);	 /* X-register value array */
			XFREE(mushstate.rdata->x_lens);	 /* X-register length array */

			/* Free register context structure itself */
			XFREE(mushstate.rdata);
		}

		/* Restore original register context */
		mushstate.rdata = preserve;
	}

	/* Clean up hook attribute buffer */
	XFREE(tstr);
}

/**
 * @brief Trigger pre-movement and post-movement hooks during room transitions.
 *
 * This function invokes registered hooks on the internalgoto command to allow
 * in-game softcode to intercept and respond to player movement between rooms.
 * It supports both pre-movement hooks (before location change) and post-movement
 * hooks (after location change), enabling game logic such as:
 * - Exit/entrance announcements and custom messages
 * - Movement cost deduction (energy, stamina, currency)
 * - Access validation and movement restrictions
 * - Environmental effects and status changes
 * - Activity logging and zone tracking
 * - Achievement/quest progress updates
 *
 * The function uses the internalgoto command entry's hooks rather than the
 * user-visible "goto" command, ensuring hooks are triggered for all internal
 * movement operations (teleport, home, follow, etc.) not just explicit goto.
 *
 * Hooks are skipped for CS_ADDED commands (dynamically added commands) since
 * those may not have properly initialized hook structures. The register
 * management mode (CS_PRESERVE or CS_PRIVATE) is extracted from the command's
 * callseq flags to determine register context handling during hook evaluation.
 *
 * @param player Database reference of the player being moved between rooms
 * @param cause  Database reference of the entity that initiated the movement
 *               (could be the player, another object, or the system)
 * @param state  Movement phase: false = before location change (pre-hook),
 *               true = after location change (post-hook)
 *
 * @note This function has no effect if internalgoto_cmdp is NULL (command not
 *       initialized), if the relevant hook (pre/post) is NULL, or if the
 *       command is marked CS_ADDED.
 *
 * @note No command arguments are passed to movement hooks (cargs = NULL, ncargs = 0).
 *       Hooks can query player location and destination through database functions.
 *
 * @warning Pre-hooks execute before the location change, so player location
 *          is still the source room. Post-hooks execute after the change, so
 *          player location is the destination room.
 *
 * @see process_hook() for hook evaluation and register management
 * @see internalgoto_cmdp for the internal movement command entry
 * @see init_cmdtab() for internalgoto_cmdp initialization
 */
void call_move_hook(dbref player, dbref cause, bool state)
{
	/* Early exit if internalgoto command not initialized */
	if (!internalgoto_cmdp)
	{
		return;
	}

	/* Extract register management mode from command callseq flags */
	int register_mode = internalgoto_cmdp->callseq & (CS_PRESERVE | CS_PRIVATE);

	/* Execute appropriate hook based on movement phase */
	if (!state) /* Pre-movement: before location change */
	{
		/* Trigger pre-hook if registered and not a dynamically added command */
		if (internalgoto_cmdp->pre_hook && !(internalgoto_cmdp->callseq & CS_ADDED))
		{
			process_hook(internalgoto_cmdp->pre_hook, register_mode, player, cause, NULL, 0);
		}
	}
	else /* Post-movement: after location change */
	{
		/* Trigger post-hook if registered and not a dynamically added command */
		if (internalgoto_cmdp->post_hook && !(internalgoto_cmdp->callseq & CS_ADDED))
		{
			process_hook(internalgoto_cmdp->post_hook, register_mode, player, cause, NULL, 0);
		}
	}
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
