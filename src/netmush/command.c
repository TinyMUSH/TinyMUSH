/**
 * @file command.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command table registration, parsing, dispatch, and execution pipeline
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
/* init_cmdtab moved to command_init.c */

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
/* reset_prefix_cmds moved to command_init.c */

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
/* check_access moved to command_access.c */

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
/* check_mod_access moved to command_access.c */

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
/* check_userdef_access moved to command_access.c */

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
/* process_hook moved to command_access.c */

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
/* call_move_hook moved to command_access.c */

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
/* check_cmd_access moved to command_access.c */

/**
 * @brief Execute a validated command with argument parsing, switch processing, and hook invocation.
 *
 * This function is the core command execution engine that handles the complete lifecycle
 * of a command after it has been matched and validated. It performs:
 *
 * **Pre-Execution Validation:**
 * 1. Argument count validation (prevent buffer overflows)
 * 2. Object type compatibility checking
 * 3. Permission validation (core, module, user-defined)
 * 4. Global system flag checks (building disabled, queueing disabled)
 *
 * **Switch Processing:**
 * - Parses switch strings (e.g., "/quiet/force")
 * - Validates switches against command's allowed switch table
 * - Handles SW_MULTIPLE for combinable switches vs exclusive switches
 * - Accumulates switch flags via bitwise OR into the key parameter
 *
 * **Argument Interpretation:**
 * - Determines evaluation mode based on CS_INTERP, CS_NOINTERP, /noeval
 * - Handles EV_STRIP, EV_STRIP_AROUND, EV_EVAL modes
 * - Preserves raw text for CS_UNPARSE commands
 *
 * **Command Dispatch (by calling sequence):**
 * - **CS_NO_ARGS**: Commands with no arguments (e.g., WHO, QUIT)
 * - **CS_ONE_ARG**: Single-argument commands (e.g., LOOK object)
 *   - CS_UNPARSE: Pass raw command text without parsing
 *   - CS_ADDED: Dynamic softcode commands (user-defined via \@addcommand)
 *   - CS_CMDARG: Pass original command args (%0-%9) to handler
 * - **CS_TWO_ARG**: Two-argument commands with = separator (e.g., \@name obj=value)
 *   - CS_ARGV: Parse arg2 as space-separated list
 *   - CS_CMDARG: Pass original command args to handler
 *
 * **Hook Invocation:**
 * - Pre-hook: Executed before command handler (logging, validation, cost deduction)
 * - Post-hook: Executed after command handler (achievement tracking, notifications)
 *
 * **Dynamic Command Handling (CS_ADDED):**
 * For \@addcommand-registered softcode commands:
 * - Reconstructs command with switches
 * - Matches against wildcard/regex patterns in attributes
 * - Executes matching attribute softcode
 * - Handles multiple matches and \@addcommand_obey_stop setting
 * - Preserves/restores global registers during execution
 *
 * @param cmdp        Pointer to command entry containing handler, switches, and metadata
 * @param switchp     Slash-separated switch string (e.g., "quiet/force"), or NULL if no switches
 * @param player      Database reference of the player executing the command (enactor)
 * @param cause       Database reference of the object that caused this command execution
 * @param interactive Boolean flag: true if command is interactive (direct player input),
 *                    false if from queue/trigger (affects argument interpretation)
 * @param arg         Argument string to be parsed (may be modified during parsing)
 * @param unp_command Raw unparsed command string (preserved for CS_UNPARSE and logging)
 * @param cargs       Array of original command arguments (%0-%9 context)
 * @param ncargs      Number of command arguments in cargs array
 *
 * @note This function modifies the 'arg' and 'switchp' parameters during parsing.
 *       Callers should pass copies if they need to preserve the original strings.
 *
 * @note For CS_ADDED commands, this function may execute multiple attribute matches
 *       depending on \@addcommand_match_blindly and \@addcommand_obey_stop settings.
 *
 * @warning Command handlers are called with cause as both the enactor and executor
 *          for evaluation context. This is intentional for maintaining causality chains.
 *
 * @see check_cmd_access() for permission validation logic
 * @see process_hook() for pre/post hook execution
 * @see process_cmdline() for recursive command execution (CS_ADDED)
 * @see eval_expression_string() for argument interpretation
 * @see CMDENT structure for command entry definition
 */
void process_cmdent__moved(CMDENT *cmdp, char *switchp, dbref player, dbref cause, bool interactive, char *arg, char *unp_command, char *cargs[], int ncargs)
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
			/* process_cmdent and process_command moved to command_dispatch.c */
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
char *process_command__moved(dbref player, dbref cause, int interactive, char *command, char *args[], int nargs)
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
void process_cmdline__moved(dbref player, dbref cause, char *cmdline, char *args[], int nargs, BQUE *qent)
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
/* process_cmdline moved to command_dispatch.c */

/**
 * @brief Display the built-in and module-provided commands visible to a player.
 *
 * Lists all compiled-in commands and any commands exposed by loaded modules,
 * filtered by the caller's permissions and omitting `CF_DARK` entries.
 *
 * @param player Database reference of the requesting player
 *
 * @see display_nametab() for printing logged-out command names
 */
/* list_cmdtable moved to command_list.c */

/**
 * @brief Show attribute names the player is allowed to see.
 *
 * Builds a single line beginning with "Attributes:" followed by each attribute
 * name the caller can see (filtered via `See_attr`). Hidden attributes are
 * skipped; the list is truncated if it would exceed the notification buffer.
 *
 * @param player Database reference of the requesting player
 */
/* list_attrtable moved to command_list.c */

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
/* helper_list_cmdaccess moved to command_list.c */

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
/* list_cmdaccess moved to command_list.c */

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
/* list_cmdswitches moved to command_list.c */

/**
 * @brief List attribute visibility and flags for the caller.
 *
 * Shows each attribute the player may read and the associated flag bitmask,
 * skipping hidden attributes. Output is bracketed by a header/footer for
 * readability.
 *
 * @param player Database reference of the requesting player
 */
/* list_attraccess moved to command_list.c */

/**
 * @brief List wildcard attribute patterns and their flags.
 *
 * Displays all configured vattr flag patterns (e.g., NAME*, DESC*) and the
 * permissions attached to each. If none are defined, notifies the caller and
 * returns early.
 *
 * @param player Database reference of the requesting player
 */
/* list_attrtypes moved to command_list.c */

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
/* cf_access moved to command_config.c */

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
/* cf_acmd_access moved to command_config.c */

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
/* cf_attr_access moved to command_config.c */

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
/* cf_attr_type moved to command_config.c */

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
/* cf_cmd_alias moved to command_config.c */

/**
 * @brief List the default flag sets applied when new objects are created.
 *
 * Decodes the configured default flags for each object type (player, room,
 * exit, thing, robot, stripped) and emits a compact table showing what flags
 * new instances receive.
 *
 * @param player DBref of the viewer (used for decode_flags visibility)
 */
/* list_df_flags moved to command_info.c */

/**
 * @brief List per-action creation/operation costs and related quotas.
 *
 * Emits a table of common game actions with their configured minimum/maximum
 * costs and, when quotas are enabled, the associated quota consumption. Also
 * shows search/queue costs, sacrifice value rules, and clone value policy.
 *
 * @param player DBref of the viewer (used for decode/visibility)
 */
/* list_costs moved to command_info.c */

/**
 * @brief Display key configuration parameters for game setup and limits.
 *
 * Emits a structured overview of prototype objects, defaults, limits,
 * quotas, currency, and timers so admins can confirm current runtime
 * configuration. Adds wizard-only sections for queue sizing, dump/clean
 * intervals, and cache sizing.
 *
 * @param player Database reference of the viewer (visibility gates wizard-only sections)
 */
/* list_params moved to command_info.c */

/**
 * @brief List non-deleted user-defined attributes (vattrs) and their flags.
 *
 * Emits a table of user-created attributes, showing each attribute's name,
 * internal numeric ID, and its permission/behavior flags decoded via
 * `attraccess_nametab`. Attributes marked with `AF_DELETED` are skipped.
 *
 * The output is bracketed by a header/footer for readability and ends with a
 * short summary line indicating how many attributes were listed and what the
 * next automatically assigned attribute ID will be.
 *
 * @param player Database reference of the viewer
 */
/* list_vattrs moved to command_info.c */

/**
 * @brief Display one-line statistics for a hash table.
 *
 * Retrieves a formatted, heap-allocated summary from `hashinfo(tab_name, htab)`,
 * sends it to the viewer, and frees the buffer afterward. If `hashinfo()`
 * returns `NULL`, emits a simple fallback line indicating that statistics are
 * unavailable.
 *
 * @param player   Database reference of the viewer
 * @param tab_name Label used by `hashinfo()` to identify the table in output
 * @param htab     Pointer to the hash table to summarize
 */
/* list_hashstat moved to command_info.c */

/**
 * @brief Display statistics for all core and module-provided hash tables.
 *
 * Outputs a formatted table showing performance metrics (size, entries, deletes,
 * nulls, scans, hits, checks, max_scan) for all compiled-in hash tables managed
 * by `mushstate`, followed by any hash/nhash tables exported by loaded modules via
 * the `mod_<name>_hashtable` and `mod_<name>_nhashtable` symbols.
 *
 * The output is bracketed by a header and footer for readability.
 *
 * @param player Database reference of the viewer
 */
/* list_hashstats moved to command_info.c */

/**
 * @brief List hash statistics for all loaded helpfiles.
 *
 * Produces a compact table with one row per helpfile, reporting the same
 * metrics as other hash-stat listings: size, entries, deletes, empty buckets,
 * scans/lookups, hits, checks, and longest probe chain. This helps admins
 * understand help index performance and load characteristics.
 *
 * The output uses `raw_notify()` for consistent CRLF line endings and includes
 * a header and footer. If no helpfiles are loaded, a short notice is emitted.
 *
 * @param player Database reference of the viewer
 */
/* list_textfiles moved to command_info.c */

/**
 * @brief Report local resource usage of the running MUSH process.
 *
 * Prints a concise snapshot of process-level metrics obtained via
 * `getrusage(RUSAGE_SELF)` and related system queries. The report includes
 * CPU time, memory usage, page faults, disk and IPC counters, context switches,
 * and the current limit on open file descriptors.
 *
 * Notes on portability and units:
 * - Most `struct rusage` fields are of type `long`; we cast and format them
 *   with width-stable specifiers to avoid UB from mismatched printf formats.
 * - `ru_maxrss` is platform-dependent (pages on some BSDs, kilobytes on Linux).
 *   We display both the raw value and an approximate byte size assuming pages;
 *   this approximation may overstate on Linux where the unit is kilobytes.
 *
 * @param player Database reference of the viewer
 */
/* list_process moved to command_info.c */

/**
 * @brief Format and print a human-readable memory size.
 *
 * Renders the provided count using binary multiples with two decimal places,
 * selecting among B/KiB/MiB/GiB The output aligns with the table produced 
 * by `list_memory()`.
 *
 * @param player  Database reference of the viewer
 * @param item    Left-column label (padded/truncated to 30 characters)
 * @param size    Size in bytes to format
 */
/* print_memory moved to command_info.c */

/**
 * @brief Report a breakdown of in-memory structures used by the process.
 *
 * Walks key runtime data structures (objects, tables, caches, hash tables,
 * user vars, grids, structures) and emits a per-category size line plus a
 * final total. Sizes are computed in bytes and formatted via `print_memory()`
 * using binary units (B/KiB/MiB/GiB).
 *
 * @param player Database reference of the viewer
 */
/* list_memory moved to command_info.c */

/**
 * @brief Dispatch \@list to the appropriate reporting helper.
 *
 * Parses the subcommand from arg, resolves it against list_names with
 * search_nametab(), and invokes the matching reporting routine (allocator
 * stats, hash tables, parameters, memory, process info, etc.). When the input
 * is missing or unknown, the valid options are displayed. Permission failures
 * are reported explicitly. The cause and extra parameters are kept for command
 * dispatcher compatibility and are otherwise unused.
 *
 * @param player Database reference of the viewer.
 * @param cause  Command cause (unused).
 * @param extra  Extra argument (unused).
 * @param arg    Subcommand to dispatch.
 */
/* do_list moved to command_info.c */
