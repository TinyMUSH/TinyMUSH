/**
 * @file funmisc_action.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Game action side-effect functions
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
#include <sys/time.h>

void (*handler_fun_command_no_args)(dbref, dbref, int);
void (*handler_fun_command_one_args)(dbref, dbref, int, char *);
void (*handler_fun_command_two_args)(dbref, dbref, int, char *, char *);


/*------------------------------------------------------------------------
 * Side-effect functions.
 */

/**
 * @brief Check if the player can execute a command
 *
 * @param player DBref of player
 * @param name Name of command
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 * @return true User can execute the command
 * @return false User is not allowed to used the command
 */

bool check_command(dbref player, char *name, char *buff, char **bufc, char *cargs[], int ncargs)
{
	CMDENT *cmdp;

	if ((cmdp = (CMDENT *)hashfind(name, &mushstate.command_htab)))
	{
		/**
		 * Note that these permission checks are NOT identical to the
		 * ones in process_cmdent(). In particular, side-effects are
		 * NOT subject to the CA_GBL_INTERP flag. This is a design
		 * decision based on the concept that these are functions and
		 * not commands, even though they behave like commands in
		 * many respects. This is also the same reason why
		 * side-effects don't trigger hooks.
		 *
		 */
		if (Invalid_Objtype(player) || !check_cmd_access(player, cmdp, cargs, ncargs) || (!Builder(player) && Protect(CA_GBL_BUILD) && !(mushconf.control_flags & CF_BUILD)))
		{
			XSAFENOPERM(buff, bufc);
			return false;
		}
	}

	return true;
}

/**
 * @brief This side-effect function links an object to a location,
 *        behaving identically to the command:
 *        '\@link &lt;object&gt;=&lt;destination&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_link(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@link", buff, bufc, cargs, ncargs))
	{
		return;
	}

	create_do_link(player, cause, 0, fargs[0], fargs[1]);
}

/**
 * @brief This side-effect function teleports an object from one place to
 *        another, behaving identically to the command:
 *        '\@tel &lt;object&gt;=&lt;destination&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_tel(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@teleport", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_teleport(player, cause, 0, fargs[0], fargs[1]);
}

/**
 * @brief This side-effect function erases some or all attributes from an
 *        object, behaving identically to the command:
 *        '\@wipe &lt;object&gt;[&lt;/wild-attr&gt;]'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_wipe(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@wipe", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_wipe(player, cause, 0, fargs[0]);
}

/**
 * @brief This side-effect function sends a message to the list of dbrefs,
 *        behaving identically to the command:
 *        '\@pemit/list &lt;list of dbrefs&gt;=&lt;string&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_pemit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@pemit", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_pemit_list(player, fargs[0], fargs[1], 0);
}

/**
 * @brief This side-effect function sends a message to the list of dbrefs,
 *        behaving identically to the command:
 *        '\@pemit/list/contents &lt;list of dbrefs&gt;=&lt;string&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_remit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@pemit", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_pemit_list(player, fargs[0], fargs[1], 1);
}

/**
 * @brief This side-effect function sends a message to everyone in &lt;target&gt;'s
 *        location with the exception of &lt;target&gt;,
 *        behaving identically to the command:
 *        '\@oemit &lt;target&gt; = &lt;string&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_oemit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@oemit", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_pemit(player, cause, PEMIT_OEMIT, fargs[0], fargs[1]);
}

/**
 * @brief This side-effect function behaves identically to the command
 *        '\@force &lt;object&gt;=&lt;action&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_force(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!check_command(player, "@force", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_force(player, cause, FRC_NOW, fargs[0], fargs[1], cargs, ncargs);
}

/**
 * @brief This side-effect function behaves identically to the command
 *        '\@trigger &lt;object&gt;/&lt;attribute&gt;=&lt;arg 0&gt;,&lt;arg 1&gt;,...&lt;arg N&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_trigger(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (nfargs < 1)
	{
		XSAFELBSTR("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}

	if (!check_command(player, "@trigger", buff, bufc, cargs, ncargs))
	{
		return;
	}

	do_trigger(player, cause, TRIG_NOW, fargs[0], &(fargs[1]), nfargs - 1);
}

/**
 * @brief This side-effect function behaves identically to the command
 *        '\@wait &lt;timer&gt;=&lt;command&gt;'
 *
 * @param buff Not used
 * @param bufc Not used
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_wait(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	cque_do_wait(player, cause, 0, fargs[0], fargs[1], cargs, ncargs);
}

/**
 * @brief This function executes &lt;command&gt; with the given arguments.
 *        &lt;command&gt; is presently limited to \@chown, \@clone, \@destroy,
 *        \@link, \@lock, \@name, \@parent, \@teleport, \@unlink, \@unlock,
 *        and \@wipe.
 *
 * @param buff Not used
 * @param bufc Not used
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_command(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	CMDENT *cmdp = NULL;
	char *tbuf1 = NULL, *tbuf2 = NULL, *p = NULL;
	int key = 0;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	for (p = fargs[0]; *p; p++)
	{
		*p = tolower(*p);
	}

	cmdp = (CMDENT *)hashfind(fargs[0], &mushstate.command_htab);

	if (!cmdp)
	{
		notify(player, "Command not found.");
		return;
	}

	if (Invalid_Objtype(player) || !check_cmd_access(player, cmdp, cargs, ncargs) || (!Builder(player) && Protect(CA_GBL_BUILD) && !(mushconf.control_flags & CF_BUILD)))
	{
		notify(player, NOPERM_MESSAGE);
		return;
	}

	if (!(cmdp->callseq & CS_FUNCTION) || (cmdp->callseq & CS_ADDED))
	{
		notify(player, "Cannot call that command.");
		return;
	}

	/**
	 * Strip command flags that are irrelevant.
	 *
	 */
	key = cmdp->extra;
	key &= ~(SW_GOT_UNIQUE | SW_MULTIPLE | SW_NOEVAL);
	switch (cmdp->callseq & CS_NARG_MASK)
	{
	case CS_NO_ARGS:
	{
		handler_fun_command_no_args = (void (*)(dbref, dbref, int))cmdp->info.handler;
		(*(handler_fun_command_no_args))(player, cause, key);
	}
	break;

	case CS_ONE_ARG:
	{
		tbuf1 = XMALLOC(1, "tbuf1");
		handler_fun_command_one_args = (void (*)(dbref, dbref, int, char *))cmdp->info.handler;
		(*(handler_fun_command_one_args))(player, cause, key, ((fargs[1]) ? (fargs[1]) : tbuf1));
		XFREE(tbuf1);
	}
	break;

	case CS_TWO_ARG:
	{
		tbuf1 = XMALLOC(1, "tbuf1");
		tbuf2 = XMALLOC(1, "tbuf2");
		handler_fun_command_two_args = (void (*)(dbref, dbref, int, char *, char *))cmdp->info.handler;
		(*(handler_fun_command_two_args))(player, cause, key, ((fargs[1]) ? (fargs[1]) : tbuf1), ((fargs[2]) ? (fargs[2]) : tbuf2));
		XFREE(tbuf2);
		XFREE(tbuf1);
	}
	break;

	default:
		notify(player, "Invalid command handler.");
		return;
	}
}

/**
 * @brief Creates a room, thing or exit
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_create(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing;
	int cost;
	char *name;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 3, 3, 0, &isep))
	{
		return;
	}

	name = fargs[0];

	if (!name || !*name)
	{
		XSAFELBSTR("#-1 ILLEGAL NAME", buff, bufc);
		return;
	}

	switch (isep.str[0])
	{
	case 'r':
		if (!check_command(player, "@dig", buff, bufc, cargs, ncargs))
		{
			return;
		}

		thing = create_obj(player, TYPE_ROOM, name, 0);
		break;

	case 'e':
		if (!check_command(player, "@open", buff, bufc, cargs, ncargs))
		{
			return;
		}

		thing = create_obj(player, TYPE_EXIT, name, 0);

		if (thing != NOTHING)
		{
			s_Exits(thing, player);
			s_Next(thing, Exits(player));
			s_Exits(player, thing);
		}

		break;

	default:
		if (!check_command(player, "@create", buff, bufc, cargs, ncargs))
		{
			return;
		}

		if (fargs[1] && *fargs[1])
		{
			cost = (int)strtol(fargs[1], (char **)NULL, 10);

			if (cost < mushconf.createmin || cost > mushconf.createmax)
			{
				XSAFELBSTR("#-1 COST OUT OF RANGE", buff, bufc);
				return;
			}
		}
		else
		{
			cost = mushconf.createmin;
		}

		thing = create_obj(player, TYPE_THING, name, cost);

		if (thing != NOTHING)
		{
			move_via_generic(thing, player, NOTHING, 0);
			s_Home(thing, new_home(player));
		}

		break;
	}

	XSAFELBCHR('#', buff, bufc);
	XSAFELTOS(buff, bufc, thing, LBUF_SIZE);
}

/*---------------------------------------------------------------------------
 * fun_set:
 */

/**
 * @brief Sets an attribute on an object
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_set(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing, thing2, aowner;
	char *p, *buff2;
	int atr, atr2, aflags, alen, clear, flagvalue;
	ATTR *attr, *attr2;

	/**
	 * obj/attr form?
	 *
	 */
	if (!check_command(player, "@set", buff, bufc, cargs, ncargs))
	{
		return;
	}

	if (parse_attrib(player, fargs[0], &thing, &atr, 0))
	{
		if (atr != NOTHING)
		{
			/**
			 * must specify flag name
			 *
			 */
			if (!fargs[1] || !*fargs[1])
			{
				XSAFELBSTR("#-1 UNSPECIFIED PARAMETER", buff, bufc);
			}

			/**
			 * are we clearing?
			 *
			 */
			clear = 0;
			p = fargs[1];

			if (*fargs[1] == NOT_TOKEN)
			{
				p++;
				clear = 1;
			}

			/**
			 * valid attribute flag?
			 *
			 */
			flagvalue = search_nametab(player, indiv_attraccess_nametab, p);

			if (flagvalue < 0)
			{
				XSAFELBSTR("#-1 CAN NOT SET", buff, bufc);
				return;
			}

			/**
			 * make sure attribute is present
			 *
			 */
			if (!atr_get_info(thing, atr, &aowner, &aflags))
			{
				XSAFELBSTR("#-1 ATTRIBUTE NOT PRESENT ON OBJECT", buff, bufc);
				return;
			}

			/**
			 * can we write to attribute?
			 *
			 */
			attr = atr_num(atr);

			if (!attr || !Set_attr(player, thing, attr, aflags))
			{
				XSAFENOPERM(buff, bufc);
				return;
			}

			/**
			 * just do it!
			 *
			 */
			if (clear)
			{
				aflags &= ~flagvalue;
			}
			else
			{
				aflags |= flagvalue;
			}

			Hearer(thing);
			atr_set_flags(thing, atr, aflags);
			return;
		}
	}

	/**
	 * find thing
	 *
	 */
	if ((thing = match_controlled(player, fargs[0])) == NOTHING)
	{
		XSAFENOTHING(buff, bufc);
		return;
	}

	/**
	 * check for attr set first
	 *
	 */
	for (p = fargs[1]; *p && (*p != ':'); p++)
		;

	if (*p)
	{
		*p++ = 0;
		atr = mkattr(fargs[1]);

		if (atr <= 0)
		{
			XSAFELBSTR("#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc);
			return;
		}

		attr = atr_num(atr);

		if (!attr)
		{
			XSAFENOPERM(buff, bufc);
			return;
		}

		atr_get_info(thing, atr, &aowner, &aflags);

		if (!Set_attr(player, thing, attr, aflags))
		{
			XSAFENOPERM(buff, bufc);
			return;
		}

		buff2 = XMALLOC(LBUF_SIZE, "buff2");

		/**
		 * check for _
		 *
		 */
		if (*p == '_')
		{
			XSTRCPY(buff2, p + 1);

			if (!parse_attrib(player, p + 1, &thing2, &atr2, 0) || (atr == NOTHING))
			{
				XFREE(buff2);
				XSAFENOMATCH(buff, bufc);
				return;
			}

			attr2 = atr_num(atr);
			p = buff2;
			atr_pget_str(buff2, thing2, atr2, &aowner, &aflags, &alen);

			if (!attr2 || !See_attr(player, thing2, attr2, aowner, aflags))
			{
				XFREE(buff2);
				XSAFENOPERM(buff, bufc);
				return;
			}
		}

		/**
		 * set it
		 *
		 */
		set_attr_internal(player, thing, atr, p, 0, buff, bufc);
		XFREE(buff2);
		return;
	}

	/**
	 * set/clear a flag
	 *
	 */
	flag_set(thing, player, fargs[1], 0);
}

/*---------------------------------------------------------------------------
 * fun_ps: Gets details about the queue.
 *   ps(): Lists everything on the queue by PID
 *   ps(<object or player>): Lists PIDs enqueued by object or player's stuff
 *   ps(<PID>): Results in '<PID>:<wait status> <command>'
 */

/**
 * @brief List the queue pids
 *
 * @param player_targ DBref of target player
 * @param obj_targ DBref of target object
 * @param queue Queue
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param bb_p Original value of output buffer tracher
 */
void list_qpids(dbref player_targ, dbref obj_targ, BQUE *queue, char *buff, char **bufc, char *bb_p)
{
	BQUE *tmp;

	for (tmp = queue; tmp; tmp = tmp->next)
	{
		if (cque_que_want(tmp, player_targ, obj_targ))
		{
			if (*bufc != bb_p)
			{
				print_separator(&SPACE_DELIM, buff, bufc);
			}

			XSAFELTOS(buff, bufc, tmp->pid, LBUF_SIZE);
		}
	}
}

/**
 * @brief Gets details about the queue.
 *        ps(): Lists everything on the queue by PID
 *        ps(&lt;object or player&gt;): Lists PIDs enqueued by object or player's stuff
 *        ps(&lt;PID&gt;): Results in '&lt;PID&gt;:&lt;wait status&gt; &lt;command&gt;'
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ps(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int qpid;
	dbref player_targ, obj_targ;
	BQUE *qptr;
	ATTR *ap;
	char *bb_p;

	/**
	 * Check for the PID case first.
	 *
	 */
	if (fargs[0] && is_integer(fargs[0]))
	{
		qpid = (int)strtol(fargs[0], (char **)NULL, 10);
		qptr = (BQUE *)nhashfind(qpid, &mushstate.qpid_htab);

		if (qptr == NULL)
		{
			return;
		}

		if ((qptr->waittime > 0) && (Good_obj(qptr->sem)))
		{
			XSAFESPRINTF(buff, bufc, "#%d:#%d/%d %s", qptr->player, qptr->sem, qptr->waittime - mushstate.now, qptr->comm);
		}
		else if (qptr->waittime > 0)
		{
			XSAFESPRINTF(buff, bufc, "#%d:%d %s", qptr->player, qptr->waittime - mushstate.now, qptr->comm);
		}
		else if (Good_obj(qptr->sem))
		{
			if (qptr->attr == A_SEMAPHORE)
			{
				XSAFESPRINTF(buff, bufc, "#%d:#%d %s", qptr->player, qptr->sem, qptr->comm);
			}
			else
			{
				ap = atr_num(qptr->attr);

				if (ap && ap->name)
				{
					XSAFESPRINTF(buff, bufc, "#%d:#%d/%s %s", qptr->player, qptr->sem, ap->name, qptr->comm);
				}
				else
				{
					XSAFESPRINTF(buff, bufc, "#%d:#%d %s", qptr->player, qptr->sem, qptr->comm);
				}
			}
		}
		else
		{
			XSAFESPRINTF(buff, bufc, "#%d: %s", qptr->player, qptr->comm);
		}

		return;
	}

	/**
	 * We either have nothing specified, or an object or player.
	 *
	 */
	if (!fargs[0] || !*fargs[0])
	{
		if (!See_Queue(player))
		{
			return;
		}

		obj_targ = NOTHING;
		player_targ = NOTHING;
	}
	else
	{
		player_targ = Owner(player);

		if (See_Queue(player))
		{
			obj_targ = match_thing(player, fargs[0]);
		}
		else
		{
			obj_targ = match_controlled(player, fargs[0]);
		}

		if (!Good_obj(obj_targ))
		{
			return;
		}

		if (isPlayer(obj_targ))
		{
			player_targ = obj_targ;
			obj_targ = NOTHING;
		}
	}

	/**
	 * List all the PIDs that match.
	 *
	 */
	bb_p = *bufc;
	list_qpids(player_targ, obj_targ, mushstate.qfirst, buff, bufc, bb_p);
	list_qpids(player_targ, obj_targ, mushstate.qlfirst, buff, bufc, bb_p);
	list_qpids(player_targ, obj_targ, mushstate.qwait, buff, bufc, bb_p);
	list_qpids(player_targ, obj_targ, mushstate.qsemfirst, buff, bufc, bb_p);
}
