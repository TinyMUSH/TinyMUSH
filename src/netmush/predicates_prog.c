/**
 * @file predicates_prog.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Interactive programming (@prog) support
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
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

/* @program 'glues' a user's input to a command. Once executed, the first
 * string input from any of the doer's logged in descriptors will be
 * substituted in <command> as %0. Commands already queued by the doer
 * will be processed normally.
 */

void handle_prog(DESC *d, char *message)
{
	DESC *all, *dsave;
	char *cmd;
	dbref aowner;
	int aflags, alen;

	/*
	 * Allow the player to pipe a command while in interactive mode.
	 * * Use telnet protocol's GOAHEAD command to show prompt
	 */

	if (*message == '|')
	{
		dsave = d;
		do_command(d, message + 1, 1);

		if (dsave == d)
		{
			/*
			 * We MUST check if we still have a descriptor, and it's
			 * * the same one, since we could have piped a LOGOUT or
			 * * QUIT!
			 */

			/*
			 * Use telnet protocol's GOAHEAD command to show prompt, make
			 * sure that we haven't been issues an @quitprogram
			 */
			if (d->program_data != NULL)
			{
				queue_rawstring(d, NULL, "> \377\371");
			}

			return;
		}
	}

	cmd = atr_get(d->player, A_PROGCMD, &aowner, &aflags, &alen);

	if (!cmd)
	{
		return;
	}

	all = (DESC *)nhashfind((int)d->player, &mushstate.desc_htab);

	if (!all)
	{
		XFREE(cmd);
		return;
	}

	if (!all->program_data)
	{
		XFREE(cmd);
		return;
	}

	if (all->program_data->wait_data)
	{
		for (int z = 0; z < all->program_data->wait_data->q_alloc; z++)
		{
			if (all->program_data->wait_data->q_regs[z])
				XFREE(all->program_data->wait_data->q_regs[z]);
		}

		for (int z = 0; z < all->program_data->wait_data->xr_alloc; z++)
		{
			if (all->program_data->wait_data->x_names[z])
				XFREE(all->program_data->wait_data->x_names[z]);

			if (all->program_data->wait_data->x_regs[z])
				XFREE(all->program_data->wait_data->x_regs[z]);
		}

		if (all->program_data->wait_data->q_regs)
		{
			XFREE(all->program_data->wait_data->q_regs);
		}

		if (all->program_data->wait_data->q_lens)
		{
			XFREE(all->program_data->wait_data->q_lens);
		}

		if (all->program_data->wait_data->x_names)
		{
			XFREE(all->program_data->wait_data->x_names);
		}

		if (all->program_data->wait_data->x_regs)
		{
			XFREE(all->program_data->wait_data->x_regs);
		}

		if (all->program_data->wait_data->x_lens)
		{
			XFREE(all->program_data->wait_data->x_lens);
		}

		XFREE(all->program_data->wait_data);
	}

	XFREE(all->program_data);
	/*
	 * Set info for all player descriptors to NULL
	 */
	for (all = (DESC *)nhashfind((int)d->player, &mushstate.desc_htab); all; all = all->hashnext)
	{
		all->program_data = NULL;
	}
	atr_clr(d->player, A_PROGCMD);
	XFREE(cmd);
}

int ok_program(dbref player, dbref doer)
{
	if ((!(Prog(player) || Prog(Owner(player))) && !Controls(player, doer)) || (God(doer) && !God(player)))
	{
		notify(player, NOPERM_MESSAGE);
		return 0;
	}

	if (!isPlayer(doer) || !Good_obj(doer))
	{
		notify(player, "No such player.");
		return 0;
	}

	if (!Connected(doer))
	{
		notify(player, "Sorry, that player is not connected.");
		return 0;
	}

	return 1;
}

void do_quitprog(dbref player, dbref cause, int key, char *name)
{
	DESC *d;
	dbref doer;
	int isprog = 0;

	if (*name)
	{
		doer = match_thing(player, name);
	}
	else
	{
		doer = player;
	}

	if (!ok_program(player, doer))
	{
		return;
	}

	for (d = (DESC *)nhashfind((int)doer, &mushstate.desc_htab); d; d = d->hashnext)
	{
		if (d->program_data != NULL)
		{
			isprog = 1;
		}
	}

	if (!isprog)
	{
		notify(player, "Player is not in an @program.");
		return;
	}

	d = (DESC *)nhashfind(doer, &mushstate.desc_htab);

	if (!d)
	{
		notify(player, "Player descriptor not found.");
		return;
	}

	if (!d->program_data)
	{
		notify(player, "Player has no program data.");
		return;
	}

	if (d->program_data->wait_data)
	{
		for (int z = 0; z < d->program_data->wait_data->q_alloc; z++)
		{
			if (d->program_data->wait_data->q_regs[z])
				XFREE(d->program_data->wait_data->q_regs[z]);
		}

		for (int z = 0; z < d->program_data->wait_data->xr_alloc; z++)
		{
			if (d->program_data->wait_data->x_names[z])
				XFREE(d->program_data->wait_data->x_names[z]);

			if (d->program_data->wait_data->x_regs[z])
				XFREE(d->program_data->wait_data->x_regs[z]);
		}

		if (d->program_data->wait_data->q_regs)
		{
			XFREE(d->program_data->wait_data->q_regs);
		}
		if (d->program_data->wait_data->q_lens)
		{
			XFREE(d->program_data->wait_data->q_lens);
		}
		if (d->program_data->wait_data->x_names)
		{
			XFREE(d->program_data->wait_data->x_names);
		}
		if (d->program_data->wait_data->x_regs)
		{
			XFREE(d->program_data->wait_data->x_regs);
		}
		if (d->program_data->wait_data->x_lens)
		{
			XFREE(d->program_data->wait_data->x_lens);
		}
		XFREE(d->program_data->wait_data);
	}

	XFREE(d->program_data);
	/*
	 * Set info for all player descriptors to NULL
	 */
	for (d = (DESC *)nhashfind((int)doer, &mushstate.desc_htab); d; d = d->hashnext)
	{
		d->program_data = NULL;
	}
	atr_clr(doer, A_PROGCMD);
	notify(player, "@program cleared.");
	notify(doer, "Your @program has been terminated.");
}

void do_prog(dbref player, dbref cause, int key, char *name, char *command)
{
	DESC *d;
	PROG *program;
	int atr, aflags, lev, found;
	dbref doer, thing, aowner, parent;
	ATTR *ap;
	char *attrib, *msg;

	if (!name || !*name)
	{
		notify(player, "No players specified.");
		return;
	}

	doer = match_thing(player, name);

	if (!ok_program(player, doer))
	{
		return;
	}

	msg = command;
	attrib = parse_to(&msg, ':', 1);

	if (msg && *msg)
	{
		notify(doer, msg);
	}

	parse_attrib(player, attrib, &thing, &atr, 0);

	if (atr != NOTHING)
	{
		if (!atr_pget_info(thing, atr, &aowner, &aflags))
		{
			notify(player, "Attribute not present on object.");
			return;
		}

		ap = atr_num(atr);

		if (!ap)
		{
			notify(player, "No such attribute.");
			return;
		}

		found = 0;

		for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (atr_get_info(parent, atr, &aowner, &aflags))
			{
				found = 1;
				break;
			}
		}

		if (!found)
		{
			notify(player, "Attribute not present on object.");
			return;
		}

		if (God(player) || (!God(thing) && See_attr(player, thing, ap, aowner, aflags) && (Wizard(player) || (aowner == Owner(player)))))
		{
			/*
			 * Check if cause already has an @prog input pending
			 */
			int has_pending = 0;
			for (d = (DESC *)nhashfind((int)doer, &mushstate.desc_htab); d; d = d->hashnext)
			{
				if (d->program_data != NULL)
				{
					has_pending = 1;
					break;
				}
			}
			if (has_pending)
			{
				notify(player, "Input already pending.");
				return;
			}
			char *prog_cmd = atr_get_raw(parent, atr);

			if (prog_cmd)
			{
				atr_add_raw(doer, A_PROGCMD, prog_cmd);
				XFREE(prog_cmd);
			}
		}
		else
		{
			notify(player, NOPERM_MESSAGE);
			return;
		}
	}
	else
	{
		notify(player, "No such attribute.");
		return;
	}

	program = (PROG *)XMALLOC(sizeof(PROG), "program");
	program->wait_cause = player;

	if (mushstate.rdata)
	{
		/**
		 * Alloc_RegData
		 *
		 */
		if (mushstate.rdata && (mushstate.rdata->q_alloc || mushstate.rdata->xr_alloc))
		{
			program->wait_data = (GDATA *)XMALLOC(sizeof(GDATA), "do_prog.gdata");
			program->wait_data->q_alloc = mushstate.rdata->q_alloc;

			if (mushstate.rdata->q_alloc)
			{
				program->wait_data->q_regs = XCALLOC(mushstate.rdata->q_alloc, sizeof(char *), "q_regs");
				program->wait_data->q_lens = XCALLOC(mushstate.rdata->q_alloc, sizeof(int), "q_lens");
			}
			else
			{
				program->wait_data->q_regs = NULL;
				program->wait_data->q_lens = NULL;
			}

			program->wait_data->xr_alloc = mushstate.rdata->xr_alloc;

			if (mushstate.rdata->xr_alloc)
			{
				program->wait_data->x_names = XCALLOC(mushstate.rdata->xr_alloc, sizeof(char *), "x_names");
				program->wait_data->x_regs = XCALLOC(mushstate.rdata->xr_alloc, sizeof(char *), "x_regs");
				program->wait_data->x_lens = XCALLOC(mushstate.rdata->xr_alloc, sizeof(int), "x_lens");
			}
			else
			{
				program->wait_data->x_names = NULL;
				program->wait_data->x_regs = NULL;
				program->wait_data->x_lens = NULL;
			}

			program->wait_data->dirty = 0;
		}
		else
		{
			program->wait_data = NULL;
		}

		/**
		 * Copy_RegData
		 *
		 */
		if (mushstate.rdata && mushstate.rdata->q_alloc)
		{
			for (int z = 0; z < mushstate.rdata->q_alloc; z++)
			{
				if (mushstate.rdata->q_regs[z] && *(mushstate.rdata->q_regs[z]))
				{
					program->wait_data->q_regs[z] = XMALLOC(LBUF_SIZE, "do_prog.regs");
					XMEMCPY(program->wait_data->q_regs[z], mushstate.rdata->q_regs[z], mushstate.rdata->q_lens[z] + 1);
					program->wait_data->q_lens[z] = mushstate.rdata->q_lens[z];
				}
			}
		}

		if (mushstate.rdata && mushstate.rdata->xr_alloc)
		{
			for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
			{
				if (mushstate.rdata->x_names[z] && *(mushstate.rdata->x_names[z]) && mushstate.rdata->x_regs[z] && *(mushstate.rdata->x_regs[z]))
				{
					program->wait_data->x_names[z] = XMALLOC(SBUF_SIZE, "glob.x_name");
					strncpy(program->wait_data->x_names[z], mushstate.rdata->x_names[z], SBUF_SIZE - 1);
					program->wait_data->x_names[z][SBUF_SIZE - 1] = '\0';
					program->wait_data->x_regs[z] = XMALLOC(LBUF_SIZE, "glob.x_reg");
					XMEMCPY(program->wait_data->x_regs[z], mushstate.rdata->x_regs[z], mushstate.rdata->x_lens[z] + 1);
					program->wait_data->x_lens[z] = mushstate.rdata->x_lens[z];
				}
			}
		}

		if (mushstate.rdata)
		{
			program->wait_data->dirty = mushstate.rdata->dirty;
		}
		else
		{
			program->wait_data->dirty = 0;
		}
	}
	else
	{
		program->wait_data = NULL;
	}

	/*
	 * Now, start waiting.
	 */
	int desc_count = 0;
	for (d = (DESC *)nhashfind((int)doer, &mushstate.desc_htab); d && desc_count < 1000; d = d->hashnext, desc_count++)
	{
		d->program_data = program;
		/*
		 * Use telnet protocol's GOAHEAD command to show prompt
		 */
		queue_rawstring(d, NULL, "> \377\371");
	}
}
