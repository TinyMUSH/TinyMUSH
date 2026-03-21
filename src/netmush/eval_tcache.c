/**
 * @file eval_tcache.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Expression trace-cache subsystem for the evaluator
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * When a player has the Trace flag set, every expression evaluation records
 * the original input and its expanded output. This module owns that cache:
 *
 *  - tcache_init()         – reset the cache at server start-up
 *  - _eval_tcache_empty()  – test whether we are at top level and mark active
 *  - _eval_tcache_add()    – append one (original, result) pair
 *  - _eval_tcache_finish() – flush the cache to the traced player and free it
 *
 * The three _eval_tcache_* helpers are module-internal: they are called only
 * from eval_expression_string() in eval.c. They are not declared in
 * prototypes.h but are declared extern in eval.c where they are used.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
#include <string.h>

/**
 * @brief Single entry in the trace cache linked list.
 */
typedef struct tcache_ent
{
	char *orig;               /*!< Original (unevaluated) expression string */
	char *result;             /*!< Evaluated result string */
	struct tcache_ent *next;  /*!< Next entry in the list */
} tcache_ent;

tcache_ent *tcache_head;     /*!< Head of the trace cache linked list */
int tcache_top;              /*!< Non-zero when cache is at top level (no nesting) */
int tcache_count;            /*!< Number of entries currently in the cache */

/**
 * @brief Initialize the expression trace cache system.
 *
 * Resets the trace cache to its initial empty state. The trace cache records
 * input-to-output transformations during expression evaluation when tracing is
 * enabled on a player. Each transformation pair (original expression → evaluated
 * result) is stored as a linked list entry for later display via
 * _eval_tcache_finish().
 *
 * This function should be called at server startup and whenever beginning a
 * new trace session.
 *
 * @see _eval_tcache_add(), _eval_tcache_finish(), _eval_tcache_empty()
 */
void tcache_init(void)
{
	tcache_head = NULL;
	tcache_top = 1;
	tcache_count = 0;
}

/**
 * @brief Check if trace cache is at top level and mark as active if so.
 *
 * This function serves dual purposes:
 * 1. Detects if we are at the top level of a trace session (no nested traces)
 * 2. Marks the cache as "in use" by setting tcache_top to 0
 *
 * Called at the start of expression evaluation when tracing is enabled to
 * determine if this is the outermost traced evaluation. If true, this
 * evaluation will be responsible for flushing the complete trace via
 * _eval_tcache_finish().
 *
 * @return bool true if this is the top-level trace (cache was empty), false if nested
 *
 * @see tcache_init(), _eval_tcache_finish()
 */
bool _eval_tcache_empty(void)
{
	if (tcache_top)
	{
		tcache_top = 0;
		tcache_count = 0;
		return true;
	}

	return false;
}

/**
 * @brief Add an expression evaluation pair to the trace cache.
 *
 * Records a transformation from original expression to evaluated result for
 * later display to the player. Only records transformations where the result
 * differs from the input (optimized to skip identity transformations). Entries
 * are stored in a linked list up to mushconf.trace_limit.
 *
 * The `orig` buffer ownership is transferred to this function – it will be
 * freed either when added to the cache (and later by _eval_tcache_finish())
 * or immediately if the entry is rejected (identical strings or over limit).
 *
 * @param orig    Original expression string (ownership transferred, will be freed)
 * @param result  Evaluated result string (copied into cache if added)
 *
 * @note If orig == result or strcmp(orig, result) == 0, orig is freed and nothing added
 * @note If tcache_count > mushconf.trace_limit, orig is freed and entry discarded
 *
 * @see _eval_tcache_finish(), mushconf.trace_limit
 */
void _eval_tcache_add(char *orig, char *result)
{
	/* Quick check: if pointers are equal or strings identical, nothing to trace */
	if (orig == result || !strcmp(orig, result))
	{
		XFREE(orig);
		return;
	}

	tcache_count++;

	if (tcache_count <= mushconf.trace_limit)
	{
		tcache_ent *xp = XMALLOC(sizeof(tcache_ent), "xp");
		char *tp = XMALLOC(LBUF_SIZE, "tp");
		XSTRCPY(tp, result);
		xp->orig = orig;
		xp->result = tp;
		xp->next = tcache_head;
		tcache_head = xp;
	}
	else
	{
		XFREE(orig);
	}
}

/**
 * @brief Output and clear all cached trace entries.
 *
 * Displays all accumulated expression transformations (original → result) to
 * the appropriate player via notify_check(). The notification target is
 * determined by:
 * 1. If player has H_Redirect flag: sends to redirected target
 * 2. Otherwise: sends to Owner(player)
 *
 * Each trace entry is formatted as:
 *   "PlayerName(#dbref)} 'original' -> 'result'"
 *
 * After displaying all entries, frees all memory and resets the cache to
 * empty state (tcache_top=1, tcache_count=0). Should be called at the end
 * of a top-level traced evaluation to complete the trace session.
 *
 * @param player  DBref of the player being traced (used for name/dbref in output)
 *
 * @note Handles redirect flag cleanup if H_Redirect is set but no redirect pointer exists
 * @note All tcache entries are freed and pointers nulled after output
 *
 * @see _eval_tcache_add(), _eval_tcache_empty(), H_Redirect()
 */
void _eval_tcache_finish(dbref player)
{
	/* Determine notification target (redirected player or owner) */
	dbref target = Owner(player);

	if (H_Redirect(player))
	{
		NUMBERTAB *np = (NUMBERTAB *)nhashfind(player, &mushstate.redir_htab);

		if (np)
		{
			target = np->num;
		}
		else
		{
			/* Ick. If we have no pointer, we should have no flag. */
			s_Flags3(player, Flags3(player) & ~HAS_REDIRECT);
			/* target already set to Owner(player) above */
		}
	}

	/* Output and free all cached trace entries */
	while (tcache_head != NULL)
	{
		tcache_ent *xp = tcache_head;
		tcache_head = xp->next;
		notify_check(target, target, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s(#%d)} '%s' -> '%s'", Name(player), player, xp->orig, xp->result);
		XFREE(xp->orig);
		XFREE(xp->result);
		XFREE(xp);
	}

	tcache_top = 1;
	tcache_count = 0;
}
