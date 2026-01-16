/**
 * @file command_hooks.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command pre/post hook execution and movement hooks
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
