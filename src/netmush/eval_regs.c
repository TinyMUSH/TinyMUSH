/**
 * @file eval_regs.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Global register save/restore for the expression evaluator
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * Nested function calls inside the evaluator need to preserve the caller's
 * q-register (%q0-%qZ) and named x-register (%q<name>) state so that
 * functions marked FN_PRES or FN_NOREGS do not corrupt the outer context.
 *
 * Public API:
 *  - save_global_regs()    – snapshot the current register state
 *  - restore_global_regs() – restore a previously snapshotted state
 *
 * The internal helper free_gdata() is used by both functions and is kept
 * static (private to this translation unit).
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <string.h>

/**
 * @brief Free a GDATA structure and all its allocated contents.
 *
 * Helper function to avoid code duplication in save_global_regs() and
 * restore_global_regs(). Frees all q-registers, x-registers, their arrays,
 * and the GDATA structure itself. Safe to call with NULL.
 *
 * @param gdata  Pointer to GDATA to free (can be NULL)
 */
static void free_gdata(GDATA *gdata)
{
	if (!gdata)
	{
		return;
	}

	/* Free q-register contents */
	for (int z = 0; z < gdata->q_alloc; z++)
	{
		if (gdata->q_regs[z])
		{
			XFREE(gdata->q_regs[z]);
		}
	}

	/* Free x-register contents */
	for (int z = 0; z < gdata->xr_alloc; z++)
	{
		if (gdata->x_names[z])
		{
			XFREE(gdata->x_names[z]);
		}
		if (gdata->x_regs[z])
		{
			XFREE(gdata->x_regs[z]);
		}
	}

	/* Free arrays */
	if (gdata->q_regs)
	{
		XFREE(gdata->q_regs);
	}
	if (gdata->q_lens)
	{
		XFREE(gdata->q_lens);
	}
	if (gdata->x_names)
	{
		XFREE(gdata->x_names);
	}
	if (gdata->x_regs)
	{
		XFREE(gdata->x_regs);
	}
	if (gdata->x_lens)
	{
		XFREE(gdata->x_lens);
	}

	XFREE(gdata);
}

/**
 * @brief Save the global registers to protect them from various sorts of munging.
 *
 * Creates a deep copy of the current global register state (q-registers and
 * x-registers) so that nested function calls can safely modify registers
 * without affecting the caller's state. Returns NULL if there are no registers
 * to save.
 *
 * @param funcname Function name (used for memory allocation tracking)
 * @return GDATA*  Pointer to saved register state, or NULL if no registers exist
 */
GDATA *save_global_regs(const char *funcname)
{
	/* Early exit if no registers exist */
	if (!mushstate.rdata || (!mushstate.rdata->q_alloc && !mushstate.rdata->xr_alloc))
	{
		return NULL;
	}

	/* Cache pointer to avoid repeated dereferences */
	GDATA *rdata = mushstate.rdata;

	/* Allocate and initialize the preservation structure */
	GDATA *preserve = (GDATA *)XMALLOC(sizeof(GDATA), funcname);
	preserve->q_alloc = rdata->q_alloc;
	preserve->xr_alloc = rdata->xr_alloc;
	preserve->dirty = rdata->dirty;

	/* Allocate q-register arrays */
	if (rdata->q_alloc)
	{
		preserve->q_regs = XCALLOC(rdata->q_alloc, sizeof(char *), "q_regs");
		preserve->q_lens = XCALLOC(rdata->q_alloc, sizeof(int), "q_lens");

		/* Copy non-empty q-registers */
		for (int z = 0; z < rdata->q_alloc; z++)
		{
			if (rdata->q_regs[z] && *rdata->q_regs[z])
			{
				preserve->q_regs[z] = XMALLOC(LBUF_SIZE, funcname);
				XMEMCPY(preserve->q_regs[z], rdata->q_regs[z], rdata->q_lens[z] + 1);
				preserve->q_lens[z] = rdata->q_lens[z];
			}
		}
	}
	else
	{
		preserve->q_regs = NULL;
		preserve->q_lens = NULL;
	}

	/* Allocate x-register arrays */
	if (rdata->xr_alloc)
	{
		preserve->x_names = XCALLOC(rdata->xr_alloc, sizeof(char *), "x_names");
		preserve->x_regs = XCALLOC(rdata->xr_alloc, sizeof(char *), "x_regs");
		preserve->x_lens = XCALLOC(rdata->xr_alloc, sizeof(int), "x_lens");

		/* Copy non-empty x-registers */
		for (int z = 0; z < rdata->xr_alloc; z++)
		{
			if (rdata->x_names[z] && *rdata->x_names[z] && rdata->x_regs[z] && *rdata->x_regs[z])
			{
				size_t name_len = strlen(rdata->x_names[z]);
				if (name_len >= SBUF_SIZE)
				{
					continue; /* Skip overlong names to avoid overflow */
				}
				preserve->x_names[z] = XMALLOC(SBUF_SIZE, "glob.x_name");
				XSTRNCPY(preserve->x_names[z], rdata->x_names[z], SBUF_SIZE - 1);
				preserve->x_names[z][SBUF_SIZE - 1] = '\0';
				preserve->x_regs[z] = XMALLOC(LBUF_SIZE, "glob.x_reg");
				XMEMCPY(preserve->x_regs[z], rdata->x_regs[z], rdata->x_lens[z] + 1);
				preserve->x_lens[z] = rdata->x_lens[z];
			}
		}
	}
	else
	{
		preserve->x_names = NULL;
		preserve->x_regs = NULL;
		preserve->x_lens = NULL;
	}

	return preserve;
}

/**
 * @brief Restore the global registers from a previously saved state.
 *
 * Restores global register state (q-registers and x-registers) from a
 * snapshot created by save_global_regs(). If the dirty flag has not changed,
 * assumes no modifications were made and simply frees the preserved state.
 * Otherwise, replaces the current register state with the preserved values.
 *
 * @param funcname Function name (used for memory allocation tracking)
 * @param preserve Saved register state to restore (can be NULL to clear registers)
 */
void restore_global_regs(const char *funcname, GDATA *preserve)
{
	/* Early exit if nothing to do */
	if (!mushstate.rdata && !preserve)
	{
		return;
	}

	/* Fast path: No changes made, just free the preserved state */
	if (mushstate.rdata && preserve && (mushstate.rdata->dirty == preserve->dirty))
	{
		free_gdata(preserve);
		return;
	}

	/* Free current register state */
	if (mushstate.rdata)
	{
		free_gdata(mushstate.rdata);
		mushstate.rdata = NULL;
	}

	/* If no preserved state, we are done (registers cleared) */
	if (!preserve || (!preserve->q_alloc && !preserve->xr_alloc))
	{
		free_gdata(preserve);
		return;
	}

	/* Allocate new register structure and copy preserved state */
	mushstate.rdata = (GDATA *)XMALLOC(sizeof(GDATA), funcname);
	mushstate.rdata->q_alloc = preserve->q_alloc;
	mushstate.rdata->xr_alloc = preserve->xr_alloc;
	mushstate.rdata->dirty = preserve->dirty;

	/* Allocate and copy q-register arrays */
	if (preserve->q_alloc)
	{
		mushstate.rdata->q_regs = XCALLOC(preserve->q_alloc, sizeof(char *), "q_regs");
		mushstate.rdata->q_lens = XCALLOC(preserve->q_alloc, sizeof(int), "q_lens");

		for (int z = 0; z < preserve->q_alloc; z++)
		{
			if (preserve->q_regs[z] && *preserve->q_regs[z])
			{
				mushstate.rdata->q_regs[z] = XMALLOC(LBUF_SIZE, funcname);
				XMEMCPY(mushstate.rdata->q_regs[z], preserve->q_regs[z], preserve->q_lens[z] + 1);
				mushstate.rdata->q_lens[z] = preserve->q_lens[z];
			}
		}
	}
	else
	{
		mushstate.rdata->q_regs = NULL;
		mushstate.rdata->q_lens = NULL;
	}

	/* Allocate and copy x-register arrays */
	if (preserve->xr_alloc)
	{
		mushstate.rdata->x_names = XCALLOC(preserve->xr_alloc, sizeof(char *), "x_names");
		mushstate.rdata->x_regs = XCALLOC(preserve->xr_alloc, sizeof(char *), "x_regs");
		mushstate.rdata->x_lens = XCALLOC(preserve->xr_alloc, sizeof(int), "x_lens");

		for (int z = 0; z < preserve->xr_alloc; z++)
		{
			if (preserve->x_names[z] && *preserve->x_names[z] && preserve->x_regs[z] && *preserve->x_regs[z])
			{
				size_t name_len = strlen(preserve->x_names[z]);
				if (name_len >= SBUF_SIZE)
				{
					continue; /* Skip overlong names to avoid overflow */
				}
				mushstate.rdata->x_names[z] = XMALLOC(SBUF_SIZE, "glob.x_name");
				XSTRNCPY(mushstate.rdata->x_names[z], preserve->x_names[z], SBUF_SIZE - 1);
				mushstate.rdata->x_names[z][SBUF_SIZE - 1] = '\0';
				mushstate.rdata->x_regs[z] = XMALLOC(LBUF_SIZE, "glob.x_reg");
				XMEMCPY(mushstate.rdata->x_regs[z], preserve->x_regs[z], preserve->x_lens[z] + 1);
				mushstate.rdata->x_lens[z] = preserve->x_lens[z];
			}
		}
	}
	else
	{
		mushstate.rdata->x_names = NULL;
		mushstate.rdata->x_regs = NULL;
		mushstate.rdata->x_lens = NULL;
	}

	/* Free the preserved state now that we have copied it */
	free_gdata(preserve);
}
