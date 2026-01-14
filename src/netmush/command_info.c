/**
 * @file command_info.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief System information and statistics display functions
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
#include <dlfcn.h>
#include <pcre2.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>

/**
 * @brief Display default flag sets for each object type.
 *
 * Emits a formatted table showing the default flags assigned to newly created
 * objects of each type (players, rooms, exits, things, robots, and stripped).
 * Uses `decode_flags()` to convert bitmasks to human-readable flag strings.
 *
 * @param player DBref of the viewer
 */
void list_df_flags(dbref player)
{
	char *flags[6];

	/* Decode defaults for each object category so we can present a concise table. */
	flags[0] = decode_flags(player, mushconf.player_flags);
	flags[1] = decode_flags(player, mushconf.room_flags);
	flags[2] = decode_flags(player, mushconf.exit_flags);
	flags[3] = decode_flags(player, mushconf.thing_flags);
	flags[4] = decode_flags(player, mushconf.robot_flags);
	flags[5] = decode_flags(player, mushconf.stripped_flags);

	raw_notify(player, "%-14s %s", "Type", "Default flags");
	raw_notify(player, "-------------- ----------------------------------------------------------------");
	raw_notify(player, "%-14s P%s", "Players", flags[0]);
	raw_notify(player, "%-14s R%s", "Rooms", flags[1]);
	raw_notify(player, "%-14s E%s", "Exits", flags[2]);
	raw_notify(player, "%-14s %s", "Things", flags[3]);
	raw_notify(player, "%-14s P%s", "Robots", flags[4]);
	raw_notify(player, "%-14s %s", "Stripped", flags[5]);
	raw_notify(player, "-------------------------------------------------------------------------------");

	for (int i = 0; i < 6; i++)
	{
		XFREE(flags[i]);
	}
}

/**
 * @brief Display action costs, quotas, and economic configuration.
 *
 * Emits a formatted table of creation/operation costs (min/max) for common actions,
 * quota consumption (when enabled), search/queue costs, sacrifice rules, and clone
 * value policy. All costs are displayed in the configured currency.
 *
 * @param player DBref of the viewer
 */
void list_costs(dbref player)
{
	const bool show_quota = mushconf.quotas;

	notify(player, "Action                                            Minimum   Maximum   Quota");
	notify(player, "------------------------------------------------- --------- --------- ---------");

	/* Basic creation costs (quota-aware). */
	if (show_quota)
	{
		raw_notify(player, "%-49.49s %-9d           %-9d", "Digging Room", mushconf.digcost, mushconf.room_quota);
		raw_notify(player, "%-49.49s %-9d           %-9d", "Opening Exit", mushconf.opencost, mushconf.exit_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Digging Room", mushconf.digcost);
		raw_notify(player, "%-49.49s %-9d", "Opening Exit", mushconf.opencost);
	}
	raw_notify(player, "%-49.49s %-9d", "Linking Exit or DropTo", mushconf.linkcost);
	if (show_quota)
	{
		raw_notify(player, "%-49.49s %-9d %-9d %-9d", "Creating Thing", mushconf.createmin, mushconf.createmax, mushconf.exit_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d %-9d", "Creating Thing", mushconf.createmin, mushconf.createmax);
	}
	if (show_quota)
	{
		raw_notify(player, "%-49.49s %-9d           %-9d", "Creating Robot", mushconf.robotcost, mushconf.player_quota);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Creating Robot", mushconf.robotcost);
	}

	/* Killing and success chance. */
	raw_notify(player, "%-49.49s %-9d %-9d", "Killing Player", mushconf.killmin, mushconf.killmax);
	if (mushconf.killmin == mushconf.killmax)
	{
		raw_notify(player, "  Chance of success: %d%%", (mushconf.killmin * 100) / mushconf.killguarantee);
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Guaranted Kill Success", mushconf.killguarantee);
	}

	/* Miscellaneous CPU/search and queue-related costs. */
	raw_notify(player, "%-49.49s %-9d", "Computationally expensive commands or functions", mushconf.searchcost);
	raw_notify(player, "  @entrances, @find, @search, @stats,");
	raw_notify(player, "  search() and stats()");

	if (mushconf.machinecost > 0)
	{
		raw_notify(player, "%-49.49s 1/%-7d", "Command run from Queue", mushconf.machinecost);
	}

	if (mushconf.waitcost > 0)
	{
		raw_notify(player, "%-49.49s %-9d", "Deposit for putting command in Queue", mushconf.waitcost);
		raw_notify(player, "  Deposit refund when command is run or cancel");
	}

	/* Sacrifice value math depends on sacfactor/sacadjust. */
	if (mushconf.sacfactor == 0)
	{
		raw_notify(player, "%-49.49s %-9d", "Object Value", mushconf.sacadjust);
	}
	else if (mushconf.sacfactor == 1)
	{
		if (mushconf.sacadjust < 0)
		{
			raw_notify(player, "%-49.49s Creation Cost - %d", "Object Value", -mushconf.sacadjust);
		}
		else if (mushconf.sacadjust > 0)
		{
			raw_notify(player, "%-49.49s Creation Cost + %d", "Object Value", mushconf.sacadjust);
		}
		else
		{
			raw_notify(player, "%-49.49s Creation Cost", "Object Value");
		}
	}
	else
	{
		if (mushconf.sacadjust < 0)
		{
			raw_notify(player, "%-49.49s (Creation Cost / %d) - %d", "Object Value", mushconf.sacfactor, -mushconf.sacadjust);
		}
		else if (mushconf.sacadjust > 0)
		{
			raw_notify(player, "%-49.49s (Creation Cost / %d) + %d", "Object Value", mushconf.sacfactor, mushconf.sacadjust);
		}
		else
		{
			raw_notify(player, "%-49.49s Creation Cost / %d", "Object Value", mushconf.sacfactor);
		}
	}

	if (mushconf.clone_copy_cost)
	{
		raw_notify(player, "%-49.49s Value Original Object", "Cloned Object Value");
	}
	else
	{
		raw_notify(player, "%-49.49s %-9d", "Cloned Object Value", mushconf.createmin);
	}

	notify(player, "-------------------------------------------------------------------------------");
	raw_notify(player, "All costs are in %s", mushconf.many_coins);
}

/**
 * @brief Display comprehensive configuration parameters and limits.
 *
 * Emits structured sections covering prototype objects, defaults, system limits,
 * quotas, currency settings, and timers. Wizard-only sections show queue sizing,
 * dump/clean intervals, and cache configuration.
 *
 * @param player DBref of the viewer (wizard check gates restricted sections)
 */
void list_params(dbref player)
{
	time_t now = time(NULL); /* Capture current time once for timer deltas below. */

	raw_notify(player, "%-19s %s", "Prototype", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s #%d", "Room", mushconf.room_proto);
	raw_notify(player, "%-19s #%d", "Exit", mushconf.exit_proto);
	raw_notify(player, "%-19s #%d", "Thing", mushconf.thing_proto);
	raw_notify(player, "%-19s #%d", "Player", mushconf.player_proto);

	raw_notify(player, "\r\n%-19s %s", "Attr Default", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s #%d", "Room", mushconf.room_defobj);
	raw_notify(player, "%-19s #%d", "Exit", mushconf.exit_defobj);
	raw_notify(player, "%-19s #%d", "Thing", mushconf.thing_defobj);
	raw_notify(player, "%-19s #%d", "Player", mushconf.player_defobj);

	raw_notify(player, "\r\n%-19s %s", "Default Parents", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s #%d", "Room", mushconf.room_parent);
	raw_notify(player, "%-19s #%d", "Exit", mushconf.exit_parent);
	raw_notify(player, "%-19s #%d", "Thing", mushconf.thing_parent);
	raw_notify(player, "%-19s #%d", "Player", mushconf.player_parent);

	raw_notify(player, "\r\n%-19s %s", "Limits", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s %d", "Function recursion", mushconf.func_nest_lim);
	raw_notify(player, "%-19s %d", "Function invocation", mushconf.func_invk_lim);
	raw_notify(player, "%-19s %d", "Command recursion", mushconf.cmd_nest_lim);
	raw_notify(player, "%-19s %d", "Command invocation", mushconf.cmd_invk_lim);
	raw_notify(player, "%-19s %d", "Output", mushconf.output_limit);
	raw_notify(player, "%-19s %d", "Queue", mushconf.queuemax);
	raw_notify(player, "%-19s %d", "CPU", mushconf.func_cpu_lim_secs);
	raw_notify(player, "%-19s %d", "Wild", mushconf.wild_times_lim);
	raw_notify(player, "%-19s %d", "Aliases", mushconf.max_player_aliases);
	raw_notify(player, "%-19s %d", "Forwardlist", mushconf.fwdlist_lim);
	raw_notify(player, "%-19s %d", "Propdirs", mushconf.propdir_lim);
	raw_notify(player, "%-19s %d", "Registers", mushconf.register_limit);
	raw_notify(player, "%-19s %d", "Stacks", mushconf.stack_lim);
	raw_notify(player, "%-19s %d", "Variables", mushconf.numvars_lim);
	raw_notify(player, "%-19s %d", "Structures", mushconf.struct_lim);
	raw_notify(player, "%-19s %d", "Instances", mushconf.instance_lim);
	raw_notify(player, "%-19s %d", "Objects", mushconf.building_limit);
	raw_notify(player, "%-19s %d", "Allowance", mushconf.paylimit);
	raw_notify(player, "%-19s %d", "Trace levels", mushconf.trace_limit);
	raw_notify(player, "%-19s %d", "Connect tries", mushconf.retry_limit);
	if (mushconf.max_players >= 0)
	{
		raw_notify(player, "%-19s %d", "Logins", mushconf.max_players);
	}

	raw_notify(player, "\r\n%-19s %s", "Nesting", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s %d", "Locks", mushconf.lock_nest_lim);
	raw_notify(player, "%-19s %d", "Parents", mushconf.parent_nest_lim);
	raw_notify(player, "%-19s %d", "Messages", mushconf.ntfy_nest_lim);
	raw_notify(player, "%-19s %d", "Zones", mushconf.zone_nest_lim);

	raw_notify(player, "\r\n%-19s %s", "Timeouts", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s %d", "Idle", mushconf.idle_timeout);
	raw_notify(player, "%-19s %d", "Connect", mushconf.conn_timeout);
	raw_notify(player, "%-19s %d", "Tries", mushconf.retry_limit);
	raw_notify(player, "%-19s %d", "Lag", mushconf.max_cmdsecs);

	raw_notify(player, "\r\n%-19s %s", "Money", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s %d", "Start", mushconf.paystart);
	raw_notify(player, "%-19s %d", "Daily", mushconf.paycheck);
	raw_notify(player, "%-19s %s", "Singular", mushconf.one_coin);
	raw_notify(player, "%-19s %s", "Plural", mushconf.many_coins);

	if (mushconf.payfind > 0)
	{
		raw_notify(player, "%-19s 1 chance in %d", "Find money", mushconf.payfind);
	}

	raw_notify(player, "\r\n%-19s %s", "Start Quotas", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s %d", "Total", mushconf.start_quota);
	raw_notify(player, "%-19s %d", "Rooms", mushconf.start_room_quota);
	raw_notify(player, "%-19s %d", "Exits", mushconf.start_exit_quota);
	raw_notify(player, "%-19s %d", "Things", mushconf.start_thing_quota);
	raw_notify(player, "%-19s %d", "Players", mushconf.start_player_quota);

	raw_notify(player, "\r\n%-19s %s", "Dbrefs", "Value");
	raw_notify(player, "------------------- -----------------------------------------------------------");
	raw_notify(player, "%-19s #%d", "Master Room", mushconf.master_room);
	raw_notify(player, "%-19s #%d", "Start Room", mushconf.start_room);
	raw_notify(player, "%-19s #%d", "Start Home", mushconf.start_home);
	raw_notify(player, "%-19s #%d", "Default Home", mushconf.default_home);

	if (Wizard(player))
	{
		raw_notify(player, "%-19s #%d", "Guest Char", mushconf.guest_char);
		raw_notify(player, "%-19s #%d", "GuestStart", mushconf.guest_start_room);
		raw_notify(player, "%-19s #%d", "Freelist", mushstate.freelist);

		raw_notify(player, "\r\n%-19s %s", "Queue run sizes", "Value");
		raw_notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "%-19s %d", "No net activity", mushconf.queue_chunk);
		raw_notify(player, "%-19s %d", "Activity", mushconf.active_q_chunk);

		raw_notify(player, "\r\n%-19s %s", "Intervals", "Value");
		raw_notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "%-19s %d", "Dump", mushconf.dump_interval);
		raw_notify(player, "%-19s %d", "Clean", mushconf.check_interval);
		raw_notify(player, "%-19s %d", "Idle Check", mushconf.idle_interval);
		raw_notify(player, "%-19s %d", "Optimize", mushconf.dbopt_interval);

		raw_notify(player, "\r\n%-19s %s", "Timers", "Value");
		raw_notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "%-19s %d", "Dump", (int)(mushstate.dump_counter - now));
		raw_notify(player, "%-19s %d", "Clean", (int)(mushstate.check_counter - now));
		raw_notify(player, "%-19s %d", "Idle Check", (int)(mushstate.idle_counter - now));

		raw_notify(player, "\r\n%-19s %s", "Scheduling", "Value");
		raw_notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "%-19s %d", "Timeslice", mushconf.timeslice);
		raw_notify(player, "%-19s %d", "Max_Quota", mushconf.cmd_quota_max);
		raw_notify(player, "%-19s %d", "Increment", mushconf.cmd_quota_incr);

		raw_notify(player, "\r\n%-19s %s", "Attribute cache", "Value");
		raw_notify(player, "------------------- -----------------------------------------------------------");
		raw_notify(player, "%-19s %d", "Width", mushconf.cache_width);
		raw_notify(player, "%-19s %d", "Size", mushconf.cache_size);
	}

	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Display user-defined attributes with IDs and permission flags.
 *
 * Emits a table of non-deleted vattrs showing name, numeric ID, and decoded
 * permission flags. Includes a summary line with the count of listed attributes
 * and the next available attribute ID.
 *
 * @param player DBref of the viewer
 */
void list_vattrs(dbref player)
{
	VATTR *va = NULL;
	int listed = 0; /* Count of attributes actually displayed (non-deleted) */

	raw_notify(player, "%-26.26s %-8s %s", "User-Defined Attributes", "Attr ID", "Permissions");
	raw_notify(player, "-------------------------- -------- -------------------------------------------");

	/*
	 * Walk the vattr registry and print only entries that are not marked
	 * deleted. We keep a count of displayed entries for the summary.
	 */
	for (va = vattr_first(); va; va = vattr_next(va))
	{
		if (!(va->flags & AF_DELETED))
		{
			listset_nametab(player, attraccess_nametab, va->flags, true, "%-26.26s %-8d ", va->name, va->number);
			listed++;
		}
	}

	raw_notify(player, "-------------------------------------------------------------------------------");
	/*
	 * Report how many were listed and the next attribute ID that will be
	 * assigned on creation. `raw_notify()` terminates the line for us.
	 */
	raw_notify(player, "%d attributes, next=%d", listed, mushstate.attr_next);
}

/**
 * @brief Display statistics for a single hash table.
 *
 * Formats and emits one line of hash table metrics (size, entries, deletes, nulls,
 * scans, hits, checks, max_scan). Falls back to "No stats available" if formatting
 * fails.
 *
 * @param player   DBref of the viewer
 * @param tab_name Label for the hash table
 * @param htab     Pointer to the hash table
 */
void list_hashstat(dbref player, const char *tab_name, HASHTAB *htab)
{
	/* Get a formatted summary line; ownership of the returned buffer is ours. */
	char *buff = XASPRINTF("hashinfo", "%-15s%8d%8d%8d%8d%8d%8d%8d%8d", tab_name, htab->hashsize, htab->entries, htab->deletes, htab->nulls, htab->scans, htab->hits, htab->checks, htab->max_scan);

	if (!buff)
	{
		/* Defensive fallback: avoid NULL dereference and still inform the user. */
		raw_notify(player, "%-15.15s %s", (tab_name ? tab_name : "(unknown)"), "No stats available");
		return;
	}

	/* Send the summary; raw_notify() appends CRLF automatically. */
	raw_notify(player, buff);

	/* Free the heap buffer provided by hashinfo(). */
	XFREE(buff);
}

/**
 * @brief Display statistics for all hash tables (core and modules).
 *
 * Emits a comprehensive table of hash metrics for all built-in hash tables plus
 * any exported by loaded modules (via `mod_<name>_hashtable` and `mod_<name>_nhashtable`
 * symbols). Includes header and footer.
 *
 * @param player DBref of the viewer
 */
void list_hashstats(dbref player)
{
	MODULE *mp = NULL;
	MODHASHES *m_htab = NULL, *hp = NULL;
	MODHASHES *m_ntab = NULL, *np = NULL;
	char *s = NULL;

	/* Output header with column labels. */
	notify(player, "Hash Stats         Size Entries Deleted   Empty Lookups    Hits  Checks Longest");
	notify(player, "--------------- ------- ------- ------- ------- ------- ------- ------- -------");

	/* Display statistics for all core hash tables. */
	list_hashstat(player, "Commands", &mushstate.command_htab);
	list_hashstat(player, "Logged-out Cmds", &mushstate.logout_cmd_htab);
	list_hashstat(player, "Functions", &mushstate.func_htab);
	list_hashstat(player, "User Functions", &mushstate.ufunc_htab);
	list_hashstat(player, "Flags", &mushstate.flags_htab);
	list_hashstat(player, "Powers", &mushstate.powers_htab);
	list_hashstat(player, "Attr names", &mushstate.attr_name_htab);
	list_hashstat(player, "Vattr names", &mushstate.vattr_name_htab);
	list_hashstat(player, "Player Names", &mushstate.player_htab);
	list_hashstat(player, "References", &mushstate.nref_htab);
	list_hashstat(player, "Net Descriptors", &mushstate.desc_htab);
	list_hashstat(player, "Queue Entries", &mushstate.qpid_htab);
	list_hashstat(player, "Forwardlists", &mushstate.fwdlist_htab);
	list_hashstat(player, "Propdirs", &mushstate.propdir_htab);
	list_hashstat(player, "Redirections", &mushstate.redir_htab);
	list_hashstat(player, "Overlaid $-cmds", &mushstate.parent_htab);
	list_hashstat(player, "Object Stacks", &mushstate.objstack_htab);
	list_hashstat(player, "Object Grids", &mushstate.objgrid_htab);
	list_hashstat(player, "Variables", &mushstate.vars_htab);
	list_hashstat(player, "Structure Defs", &mushstate.structs_htab);
	list_hashstat(player, "Component Defs", &mushstate.cdefs_htab);
	list_hashstat(player, "Instances", &mushstate.instance_htab);
	list_hashstat(player, "Instance Data", &mushstate.instdata_htab);
	list_hashstat(player, "Module APIs", &mushstate.api_func_htab);

	/*
	 * Iterate through loaded modules and look up their exported hash table
	 * arrays via dynamic symbol resolution (dlsym). Each module may provide
	 * up to two symbol exports: "mod_<name>_hashtable" and "mod_<name>_nhashtable".
	 * These are arrays of MODHASHES structs terminated by a null entry.
	 */
	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		/*
		 * Look up regular hash tables exported by this module.
		 * The symbol name is dynamically constructed as "mod_<name>_hashtable".
		 */
		s = XASPRINTF("s", "mod_%s_%s", mp->modname, "hashtable");
		m_htab = (MODHASHES *)dlsym(mp->handle, s);
		XFREE(s);

		if (m_htab)
		{
			/* Iterate through the MODHASHES array until the null terminator. */
			for (hp = m_htab; hp->htab != NULL; hp++)
			{
				list_hashstat(player, hp->tabname, hp->htab);
			}
		}

		/*
		 * Look up nhash (numeric hash) tables exported by this module.
		 * The symbol name is dynamically constructed as "mod_<name>_nhashtable".
		 */
		s = XASPRINTF("s", "mod_%s_%s", mp->modname, "nhashtable");
		m_ntab = (MODHASHES *)dlsym(mp->handle, s);
		XFREE(s);

		if (m_ntab)
		{
			/* Iterate through the MODHASHES array until the null terminator. */
			for (np = m_ntab; np->tabname != NULL; np++)
			{
				list_hashstat(player, np->tabname, np->htab);
			}
		}
	}

	/* Output footer separator. */
	notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Display hash statistics for all loaded help files.
 *
 * Emits a formatted table showing hash metrics for each help file index (size,
 * entries, deletes, empty buckets, lookups, hits, checks, longest probe). Helps
 * admins monitor help system performance. Notifies if no help files are loaded.
 *
 * @param player DBref of the viewer
 */
void list_textfiles(dbref player)
{
	/* Early exit when the build has no helpfiles configured/loaded. */
	if (mushstate.helpfiles <= 0 || !mushstate.hfiletab || !mushstate.hfile_hashes)
	{
		raw_notify(player, "No help files are loaded.");
		return;
	}

	/* Column headers aligned to match other hash statistics listings. */
	raw_notify(player, "%-15s %7s %7s %7s %7s %7s %7s %7s %7s",
			   "Help File", "Size", "Entries", "Deleted", "Empty",
			   "Lookups", "Hits", "Checks", "Longest");
	raw_notify(player, "--------------- ------- ------- ------- ------- ------- ------- ------- -------");

	/* Walk each loaded helpfile and report its hash table stats in one line. */
	for (int i = 0; i < mushstate.helpfiles; i++)
	{
		/* Resolve a human-friendly filename. We duplicate before basename()
		 * because some implementations may modify the input buffer. */
		const char *path = mushstate.hfiletab[i];
		const char *name = "(unknown)";
		if (path && *path)
		{
			char *dup = XSTRDUP(path, "helpfile_basename");
			char *base = basename(dup);
			name = base ? base : path;
			XFREE(dup);
		}

		const HASHTAB *stats = &mushstate.hfile_hashes[i];
		raw_notify(player, "%-15.15s %7d %7d %7d %7d %7d %7d %7d %7d",
				   name,
				   stats->hashsize,
				   stats->entries,
				   stats->deletes,
				   stats->nulls,
				   stats->scans,
				   stats->hits,
				   stats->checks,
				   stats->max_scan);
	}

	/* Footer separator for readability. */
	raw_notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Display process resource usage and limits.
 *
 * Reports CPU time, memory (with platform-specific `ru_maxrss` handling), page faults,
 * I/O, IPC counters, context switches, and file descriptor limit via `getrusage()` and
 * system queries. Handles failed `getrusage()` gracefully by zeroing metrics.
 *
 * @param player DBref of the viewer
 */
void list_process(dbref player)
{
	struct rusage usage;
	int rstat = getrusage(RUSAGE_SELF, &usage);

	/* Gather basic process/environment details. */
	const long pid = (long)getpid();
	const long psize = (long)getpagesize();
	const long maxfds = (long)getdtablesize();

	/* If getrusage fails, zero out the metrics to keep output predictable. */
	if (rstat != 0)
	{
		memset(&usage, 0, sizeof(usage));
	}

	/* Display identifiers and basic system parameters. */
	raw_notify(player, "      Process ID: %10ld        %10ld bytes per page", pid, psize);

	/* CPU time used in seconds (user and system). */
	raw_notify(player, "       Time used: %10ld user   %10ld sys",
			   (long)usage.ru_utime.tv_sec, (long)usage.ru_stime.tv_sec);

	/* Integral memory usage counters (platform-dependent semantics). */
	raw_notify(player, " Integral memory: %10ld shared %10ld private %10ld stack",
			   (long)usage.ru_ixrss, (long)usage.ru_idrss, (long)usage.ru_isrss);

	/* Resident set size: raw value and an approximate bytes figure. */
	{
		const long long maxrss_raw = (long long)usage.ru_maxrss;
		const long long maxrss_bytes = maxrss_raw * (long long)psize; /* may be kB on Linux */
		raw_notify(player, "  Max res memory: %10lld raw    %10lld bytes", maxrss_raw, maxrss_bytes);
	}

	/* Page fault counts: major (hard) vs minor (soft) and swapouts. */
	raw_notify(player, "     Page faults: %10ld hard   %10ld soft    %10ld swapouts",
			   (long)usage.ru_majflt, (long)usage.ru_minflt, (long)usage.ru_nswap);

	/* Block I/O counters (may be filesystem dependent). */
	raw_notify(player, "        Disk I/O: %10ld reads  %10ld writes",
			   (long)usage.ru_inblock, (long)usage.ru_oublock);

	/* IPC message counters (typically zero for this process type). */
	raw_notify(player, "     Network I/O: %10ld in     %10ld out",
			   (long)usage.ru_msgrcv, (long)usage.ru_msgsnd);

	/* Context switches and signals received. */
	raw_notify(player, "  Context switch: %10ld vol    %10ld forced  %10ld sigs",
			   (long)usage.ru_nvcsw, (long)usage.ru_nivcsw, (long)usage.ru_nsignals);

	/* Current soft limit on open file descriptors. */
	raw_notify(player, " Descs available: %10ld", maxfds);
}

/**
 * @brief Format and display a memory size with binary units.
 *
 * Converts byte count to human-readable format (B/KiB/MiB/GiB) with two decimal
 * places and emits aligned output (30-char label, value, unit).
 *
 * @param player DBref of the viewer
 * @param item   Left-column label (padded/truncated to 30 chars)
 * @param size   Size in bytes
 */
void print_memory(dbref player, const char *item, size_t size)
{
	/* Choose units and divisor thresholds using binary multiples. */
	const char *unit;
	double value;

	if (size < 1024) /* < 1 KiB */
	{
		unit = "B";
		value = (double)size;;
	}
	else if (size < 1048576) /* < 1 MiB */
	{
		unit = "KiB";
		value = (double)size / 1024.0;
	}
	else if (size < 1073741824) /* < 1 GiB */
	{
		unit = "MiB";
		value = (double)size / 1048576.0;
	}
	else
	{
		unit = "GiB";
		value = (double)size / 1073741824.0;
	}

	/* Emit aligned label and value with unit; raw_notify appends CRLF. */
	raw_notify(player, "%-30s %0.2f%s", item, value, unit);
}

/**
 * @brief Display detailed memory usage breakdown by category.
 *
 * Analyzes and reports memory consumption for objects, attributes, hash tables,
 * caches, user variables, grids, structures, and other runtime data. Emits per-category
 * sizes and a final total via `print_memory()` with binary units.
 *
 * @param player DBref of the viewer
 */
void list_memory(dbref player)
{
	double total = 0, each = 0;
	int i = 0, j = 0;
	CMDENT *cmd = NULL;
	ADDENT *add = NULL;
	NAMETAB *name = NULL;
	ATTR *attr = NULL;
	UFUN *ufunc = NULL;
	HASHENT *htab = NULL;
	OBJSTACK *stack = NULL;
	OBJGRID *grid = NULL;
	VARENT *xvar = NULL;
	STRUCTDEF *this_struct = NULL;
	INSTANCE *inst_ptr = NULL;
	STRUCTDATA *data_ptr = NULL;

	/* Calculate size of object structures */
	each = mushstate.db_top * sizeof(OBJ);
	raw_notify(player, "Item                          Size");
	raw_notify(player, "----------------------------- ------------------------------------------------");
	print_memory(player, "Object structures", each);
	total += each;
	/* Calculate size of mushstate and mushconf structures */
	each = sizeof(CONFDATA) + sizeof(STATEDATA);
	print_memory(player, "mushconf/mushstate", each);
	total += each;
	/* Calculate size of object pipelines */
	each = 0;

	for (i = 0; i < NUM_OBJPIPES; i++)
	{
		if (mushstate.objpipes[i])
		{
			each += obj_siz(mushstate.objpipes[i]);
		}
	}

	print_memory(player, "Object pipelines", each);
	total += each;
	/* Calculate size of name caches */
	each = sizeof(NAME *) * mushstate.db_top * 2;

	for (i = 0; i < mushstate.db_top; i++)
	{
		if (purenames[i])
		{
			each += strlen(purenames[i]) + 1;
		}

		if (names[i])
		{
			each += strlen(names[i]) + 1;
		}
	}

	print_memory(player, "Name caches", each);
	total += each;
	/* Calculate size of Raw Memory allocations */
	each = total_rawmemory();

	print_memory(player, "Raw Memory", each);
	total += each;
	/* Calculate size of command hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.command_htab.hashsize;

	for (i = 0; i < mushstate.command_htab.hashsize; i++)
	{
		htab = mushstate.command_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1; /* Use the current entry, not the bucket head. */

			/* Add up all the little bits in the CMDENT. */

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(CMDENT);
				cmd = (CMDENT *)htab->data;
				each += strlen(cmd->cmdname) + 1;

				if ((name = cmd->switches) != NULL)
				{
					for (j = 0; name[j].name != NULL; j++)
					{
						each += sizeof(NAMETAB);
						each += strlen(name[j].name) + 1;
					}
				}

				if (cmd->callseq & CS_ADDED)
				{
					add = cmd->info.added;

					while (add != NULL)
					{
						each += sizeof(ADDENT);
						each += strlen(add->name) + 1;
						add = add->next;
					}
				}
			}

			htab = htab->next;
		}
	}

	print_memory(player, "Command table", each);
	total += each;
	/* Calculate size of logged-out commands hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.logout_cmd_htab.hashsize;

	for (i = 0; i < mushstate.logout_cmd_htab.hashsize; i++)
	{
		htab = mushstate.logout_cmd_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				name = (NAMETAB *)htab->data;
				each += sizeof(NAMETAB);
				each += strlen(name->name) + 1;
			}

			htab = htab->next;
		}
	}

	print_memory(player, "Logout cmd htab", each);
	total += each;
	/* Calculate size of functions hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.func_htab.hashsize;

	for (i = 0; i < mushstate.func_htab.hashsize; i++)
	{
		htab = mushstate.func_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(FUN);
			}

			/*
			 * We don't count func->name because we already got
			 * it with htab->target.s
			 *
			 */
			htab = htab->next;
		}
	}

	print_memory(player, "Functions htab", each);
	total += each;
	/* Calculate size of user-defined functions hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.ufunc_htab.hashsize;

	for (i = 0; i < mushstate.ufunc_htab.hashsize; i++)
	{
		htab = mushstate.ufunc_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				ufunc = (UFUN *)htab->data;

				while (ufunc != NULL)
				{
					each += sizeof(UFUN);
					each += strlen(ufunc->name) + 1;
					ufunc = ufunc->next;
				}
			}

			htab = htab->next;
		}
	}

	print_memory(player, "U-functions htab", each);
	total += each;
	/* Calculate size of flags hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.flags_htab.hashsize;

	for (i = 0; i < mushstate.flags_htab.hashsize; i++)
	{
		htab = mushstate.flags_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(FLAGENT);
			}

			/*
			 * We don't count flag->flagname because we already
			 * got it with htab->target.s
			 *
			 */
			htab = htab->next;
		}
	}

	print_memory(player, "Flags htab", each);
	total += each;
	/* Calculate size of powers hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.powers_htab.hashsize;

	for (i = 0; i < mushstate.powers_htab.hashsize; i++)
	{
		htab = mushstate.powers_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				each += sizeof(POWERENT);
			}

			/*
			 * We don't count power->powername because we already
			 * got it with htab->target.s
			 *
			 */
			htab = htab->next;
		}
	}

	print_memory(player, "Powers htab", each);
	total += each;
	/* Calculate size of helpfile hashtables */
	each = 0;

	for (j = 0; j < mushstate.helpfiles; j++)
	{
		each += sizeof(HASHENT *) * mushstate.hfile_hashes[j].hashsize;

		for (i = 0; i < mushstate.hfile_hashes[j].hashsize; i++)
		{
			htab = mushstate.hfile_hashes[j].entry[i];

			while (htab != NULL)
			{
				each += sizeof(HASHENT);
				each += strlen(htab->target.s) + 1;

				if (!(htab->flags & HASH_ALIAS))
				{
					each += sizeof(struct help_entry);
				}

				htab = htab->next;
			}
		}
	}

	print_memory(player, "Helpfiles htabs", each);
	total += each;
	/* Calculate size of vattr name hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.vattr_name_htab.hashsize;

	for (i = 0; i < mushstate.vattr_name_htab.hashsize; i++)
	{
		htab = mushstate.vattr_name_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;
			each += sizeof(VATTR);
			htab = htab->next;
		}
	}

	print_memory(player, "Vattr name htab", each);
	total += each;
	/* Calculate size of attr name hashtable */
	each = 0;
	each += sizeof(HASHENT *) * mushstate.attr_name_htab.hashsize;

	for (i = 0; i < mushstate.attr_name_htab.hashsize; i++)
	{
		htab = mushstate.attr_name_htab.entry[i];

		while (htab != NULL)
		{
			each += sizeof(HASHENT);
			each += strlen(htab->target.s) + 1;

			if (!(htab->flags & HASH_ALIAS))
			{
				attr = (ATTR *)htab->data;
				each += sizeof(ATTR);
				each += strlen(attr->name) + 1;
			}

			htab = htab->next;
		}
	}

	print_memory(player, "Attr name htab", each);
	total += each;
	/* Calculate the size of anum_table */
	each = sizeof(ATTR *) * anum_alc_top;
	print_memory(player, "Attr num table", each);
	total += each;

	/* After this point, we only report if it's non-zero. */

	/* Calculate size of object stacks */
	each = 0;

	for (stack = (OBJSTACK *)hash_firstentry((HASHTAB *)&mushstate.objstack_htab); stack != NULL; stack = (OBJSTACK *)hash_nextentry((HASHTAB *)&mushstate.objstack_htab))
	{
		each += sizeof(OBJSTACK);
		each += strlen(stack->data) + 1;
	}

	if (each)
	{
		print_memory(player, "Object stacks", each);
	}

	total += each;
	/* Calculate the size of grids */
	each = 0;

	for (grid = (OBJGRID *)hash_firstentry((HASHTAB *)&mushstate.objgrid_htab); grid != NULL; grid = (OBJGRID *)hash_nextentry((HASHTAB *)&mushstate.objgrid_htab))
	{
		each += sizeof(OBJGRID);
		each += sizeof(char **) * grid->rows * grid->cols;

		for (i = 0; i < grid->rows; i++)
		{
			for (j = 0; j < grid->cols; j++)
			{
				if (grid->data[i][j] != NULL)
				{
					each += strlen(grid->data[i][j]) + 1;
				}
			}
		}
	}

	if (each)
	{
		print_memory(player, "Object grids", each);
	}

	total += each;
	/* Calculate the size of xvars. */
	each = 0;

	for (xvar = (VARENT *)hash_firstentry(&mushstate.vars_htab); xvar != NULL; xvar = (VARENT *)hash_nextentry(&mushstate.vars_htab))
	{
		each += sizeof(VARENT);
		each += strlen(xvar->text) + 1;
	}

	if (each)
	{
		print_memory(player, "X-Variables", each);
	}

	total += each;
	/* Calculate the size of overhead associated with structures. */
	each = 0;

	for (this_struct = (STRUCTDEF *)hash_firstentry(&mushstate.structs_htab); this_struct != NULL; this_struct = (STRUCTDEF *)hash_nextentry(&mushstate.structs_htab))
	{
		each += sizeof(STRUCTDEF);
		each += strlen(this_struct->s_name) + 1;

		for (i = 0; i < this_struct->c_count; i++)
		{
			each += strlen(this_struct->c_names[i]) + 1;
			each += sizeof(COMPONENT);
			each += strlen(this_struct->c_array[i]->def_val) + 1;
		}
	}

	for (inst_ptr = (INSTANCE *)hash_firstentry(&mushstate.instance_htab); inst_ptr != NULL; inst_ptr = (INSTANCE *)hash_nextentry(&mushstate.instance_htab))
	{
		each += sizeof(INSTANCE);
	}

	if (each)
	{
		print_memory(player, "Struct var defs", each);
	}

	total += each;
	/* Calculate the size of data associated with structures. */
	each = 0;

	for (data_ptr = (STRUCTDATA *)hash_firstentry(&mushstate.instdata_htab); data_ptr != NULL; data_ptr = (STRUCTDATA *)hash_nextentry(&mushstate.instdata_htab))
	{
		each += sizeof(STRUCTDATA);

		if (data_ptr->text)
		{
			each += strlen(data_ptr->text) + 1;
		}
	}

	if (each)
	{
		print_memory(player, "Struct var data", each);
	}

	total += each;
	/* Report end total. */
	raw_notify(player, "-------------------------------------------------------------------------------");
	print_memory(player, "Total", total);
}

/**
 * @brief Dispatch @list subcommands to appropriate reporting functions.
 *
 * Resolves subcommand from `arg` via `search_nametab()` and invokes the matching
 * reporting function (flags, costs, params, memory, process, etc.). Displays valid
 * options if input is missing/unknown. Reports permission failures explicitly.
 *
 * @param player DBref of the viewer
 * @param cause  Command cause (unused)
 * @param extra  Extra argument (unused)
 * @param arg    Subcommand to dispatch
 */
void do_list(dbref player, dbref cause, int extra, char *arg)
{
	/* Resolve the subcommand; show choices on missing/unknown input. */
	if (!arg || !*arg)
	{
		display_nametab(player, list_names, true, (char *)"Unknown option.  Use one of:");
		return;
	}

	const int flagvalue = search_nametab(player, list_names, arg);

	if (flagvalue == -2)
	{
		notify(player, "Permission denied.");
		return;
	}

	if (flagvalue < 0)
	{
		display_nametab(player, list_names, true, (char *)"Unknown option.  Use one of:");
		return;
	}

	/* Dispatch to the specific listing helper. */
	switch (flagvalue)
	{
	case LIST_ALLOCATOR:
		list_bufstats(player);
		break;

	case LIST_BUFTRACE:
		list_buftrace(player);
		break;

	case LIST_ATTRIBUTES:
		list_attrtable(player);
		break;

	case LIST_COMMANDS:
		list_cmdtable(player);
		break;

	case LIST_SWITCHES:
		list_cmdswitches(player);
		break;

	case LIST_COSTS:
		list_costs(player);
		break;

	case LIST_OPTIONS:
		list_options(player);
		break;

	case LIST_HASHSTATS:
		list_hashstats(player);
		break;

	case LIST_SITEINFO:
		list_siteinfo(player);
		break;

	case LIST_FLAGS:
		display_flagtab(player);
		break;

	case LIST_FUNCPERMS:
		list_funcaccess(player);
		break;

	case LIST_FUNCTIONS:
		list_functable(player);
		break;

	case LIST_GLOBALS:
		interp_nametab(player, enable_names, mushconf.control_flags, (char *)"Global parameters", (char *)"Status", (char *)"enabled", (char *)"disabled", true);
		break;

	case LIST_DF_FLAGS:
		list_df_flags(player);
		break;

	case LIST_PERMS:
		list_cmdaccess(player);
		break;

	case LIST_CONF_PERMS:
		list_cf_access(player);
		break;

	case LIST_CF_RPERMS:
		list_cf_read_access(player);
		break;

	case LIST_POWERS:
		display_powertab(player);
		break;

	case LIST_ATTRPERMS:
		list_attraccess(player);
		break;

	case LIST_VATTRS:
		list_vattrs(player);
		break;

	case LIST_LOGGING:
		/* Two tables: event toggles, then data toggles. */
		interp_nametab(player, logoptions_nametab, mushconf.log_options, (char *)"Events Logged", (char *)"Status", (char *)"enabled", (char *)"disabled", true);
		notify(player, "");
		interp_nametab(player, logdata_nametab, mushconf.log_info, (char *)"Information Type", (char *)"Logged", (char *)"yes", (char *)"no", true);
		break;

	case LIST_DB_STATS:
		notify(player, "Database cache layer removed: database is accessed directly.");
		break;

	case LIST_PROCESS:
		list_process(player);
		break;

	case LIST_BADNAMES:
		badname_list(player, "Disallowed names:");
		break;

	case LIST_CACHEOBJS:
		notify(player, "Object cache removed: database is accessed directly.");
		break;

	case LIST_TEXTFILES:
		list_textfiles(player);
		break;

	case LIST_PARAMS:
		list_params(player);
		break;

	case LIST_ATTRTYPES:
		list_attrtypes(player);
		break;

	case LIST_MEMORY:
		list_memory(player);
		break;

	case LIST_CACHEATTRS:
		notify(player, "Attribute cache removed: database is accessed directly.");
		break;

	case LIST_RAWMEM:
		list_rawmemory(player);
		break;

	default:
		display_nametab(player, list_names, true, (char *)"Unknown option.  Use one of:");
	}
}
