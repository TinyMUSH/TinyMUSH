/**
 * @file conf_core.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core configuration defaults, logging, and scalar handlers
 * @version 4.0
 *
 * Holds initialization of global configuration/state, logging helpers, and
 * the common scalar configuration interpreters (int/bool/string/dbref/bitset).
 * Also contains shared helpers for extended access parsing.
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

CONFDATA mushconf;
STATEDATA mushstate;

void cf_init(void)
{
	mushstate.modules_list = NULL;
	mushstate.modloaded = XMALLOC(MBUF_SIZE, "mushstate.modloaded");
	mushconf.rng_seed = -1;
	mushconf.port = 6250;
	mushconf.conc_port = 6251;
	mushconf.init_size = 1000;
	mushconf.output_block_size = 16384;
	mushconf.use_global_aconn = 1;
	mushconf.global_aconn_uselocks = 0;
	mushconf.guest_char = NOTHING;
	mushconf.guest_nuker = GOD;
	mushconf.number_guests = 30;
	mushconf.guest_basename = XSTRDUP("Guest", "mushconf.guest_basename");
	mushconf.guest_password = XSTRDUP("guest", "mushconf.guest_password");
	mushconf.guest_prefixes = XSTRDUP("", "mushconf.guest_prefixes");
	mushconf.guest_suffixes = XSTRDUP("", "mushconf.guest_suffixes");
	mushconf.backup_exec = XSTRDUP(DEFAULT_BACKUP_UTIL, "mushconf.backup_exec");
	mushconf.backup_compress = XSTRDUP(DEFAULT_BACKUP_COMPRESS, "mushconf.backup_compress");
	mushconf.backup_extract = XSTRDUP(DEFAULT_BACKUP_EXTRACT, "mushconf.backup_extract");
	mushconf.backup_ext = XSTRDUP(DEFAULT_BACKUP_EXT, "mushconf.backup_ext");
	mushconf.mush_owner = XSTRDUP("", "mushconf.mush_owner");
	mushconf.binhome = XSTRDUP(DEFAULT_BINARY_HOME, "mushconf.binhome");
	mushconf.dbhome = XSTRDUP(DEFAULT_DATABASE_HOME, "mushconf.dbhome");
	mushconf.txthome = XSTRDUP(DEFAULT_TEXT_HOME, "mushconf.txthome");
	mushconf.bakhome = XSTRDUP(DEFAULT_BACKUP_HOME, "mushconf.bakhome");
	mushconf.modules_home = XSTRDUP(DEFAULT_MODULES_HOME, "mushconf.modules_home");
	mushconf.scripts_home = XSTRDUP(DEFAULT_SCRIPTS_HOME, "mushconf.scripts_home");
	mushconf.log_home = XSTRDUP(DEFAULT_LOG_HOME, "mushconf.log_home");
	mushconf.pid_home = XSTRDUP(DEFAULT_PID_HOME, "mushconf.pid_home");
	/* We can make theses NULL because we are going to define default values later if they are still NULL. */
	mushconf.help_users = NULL;
	mushconf.help_wizards = NULL;
	mushconf.help_quick = NULL;
	mushconf.guest_file = NULL;
	mushconf.conn_file = NULL;
	mushconf.creg_file = NULL;
	mushconf.regf_file = NULL;
	mushconf.motd_file = NULL;
	mushconf.wizmotd_file = NULL;
	mushconf.quit_file = NULL;
	mushconf.down_file = NULL;
	mushconf.full_file = NULL;
	mushconf.site_file = NULL;
	mushconf.crea_file = NULL;
	mushconf.htmlconn_file = NULL;
	mushconf.motd_msg = NULL;
	mushconf.wizmotd_msg = NULL;
	mushconf.downmotd_msg = NULL;
	mushconf.fullmotd_msg = NULL;
	mushconf.dump_msg = NULL;
	mushconf.postdump_msg = NULL;
	mushconf.fixed_home_msg = NULL;
	mushconf.fixed_tel_msg = NULL;
	mushconf.huh_msg = XSTRDUP("Huh?  (Type \"help\" for help.)", "mushconf.huh_msg");
	mushconf.pueblo_msg = XSTRDUP("</xch_mudtext><img xch_mode=html><tt>", "mushconf.pueblo_msg");
	mushconf.pueblo_version = XSTRDUP("This world is Pueblo 1.0 enhanced", "mushconf.pueblo_version");
	mushconf.infotext_list = NULL;
	mushconf.indent_desc = 0;
	mushconf.name_spaces = 1;
	mushconf.fork_dump = 0;
	mushconf.dbopt_interval = 0;
	mushconf.have_pueblo = 1;
	mushconf.have_zones = 1;
	mushconf.sig_action = SA_DFLT;
	mushconf.max_players = -1;
	mushconf.dump_interval = 3600;
	mushconf.check_interval = 600;
	mushconf.events_daily_hour = 7;
	mushconf.dump_offset = 0;
	mushconf.check_offset = 300;
	mushconf.idle_timeout = 3600;
	mushconf.conn_timeout = 120;
	mushconf.idle_interval = 60;
	mushconf.retry_limit = 3;
	mushconf.output_limit = 16384;
	mushconf.paycheck = 0;
	mushconf.paystart = 0;
	mushconf.paylimit = 10000;
	mushconf.start_quota = 20;
	mushconf.start_room_quota = 20;
	mushconf.start_exit_quota = 20;
	mushconf.start_thing_quota = 20;
	mushconf.start_player_quota = 20;
	mushconf.site_chars = 25;
	mushconf.payfind = 0;
	mushconf.digcost = 10;
	mushconf.linkcost = 1;
	mushconf.opencost = 1;
	mushconf.createmin = 10;
	mushconf.createmax = 505;
	mushconf.killmin = 10;
	mushconf.killmax = 100;
	mushconf.killguarantee = 100;
	mushconf.robotcost = 1000;
	mushconf.pagecost = 10;
	mushconf.searchcost = 100;
	mushconf.waitcost = 10;
	mushconf.machinecost = 64;
	mushconf.building_limit = 50000;
	mushconf.exit_quota = 1;
	mushconf.player_quota = 1;
	mushconf.room_quota = 1;
	mushconf.thing_quota = 1;
	mushconf.queuemax = 100;
	mushconf.queue_chunk = 10;
	mushconf.active_q_chunk = 10;
	mushconf.sacfactor = 5;
	mushconf.sacadjust = -1;
	mushconf.use_hostname = 1;
	mushconf.quotas = 0;
	mushconf.typed_quotas = 0;
	mushconf.ex_flags = 1;
	mushconf.robot_speak = 1;
	mushconf.clone_copy_cost = 0;
	mushconf.pub_flags = 1;
	mushconf.quiet_look = 1;
	mushconf.exam_public = 1;
	mushconf.read_rem_desc = 0;
	mushconf.read_rem_name = 0;
	mushconf.sweep_dark = 0;
	mushconf.player_listen = 0;
	mushconf.quiet_whisper = 1;
	mushconf.dark_sleepers = 1;
	mushconf.see_own_dark = 1;
	mushconf.idle_wiz_dark = 0;
	mushconf.visible_wizzes = 0;
	mushconf.pemit_players = 0;
	mushconf.pemit_any = 0;
	mushconf.addcmd_match_blindly = 1;
	mushconf.addcmd_obey_stop = 0;
	mushconf.addcmd_obey_uselocks = 0;
	mushconf.lattr_oldstyle = 0;
	mushconf.bools_oldstyle = 0;
	mushconf.match_mine = 0;
	mushconf.match_mine_pl = 0;
	mushconf.switch_df_all = 1;
	mushconf.fascist_objeval = 0;
	mushconf.fascist_tport = 0;
	mushconf.terse_look = 1;
	mushconf.terse_contents = 1;
	mushconf.terse_exits = 1;
	mushconf.terse_movemsg = 1;
	mushconf.trace_topdown = 1;
	mushconf.trace_limit = 200;
	mushconf.safe_unowned = 0;
	mushconf.wiz_obey_linklock = 0;
	mushconf.wiz_obey_openlock = 0;
	mushconf.local_masters = 1;
	mushconf.match_zone_parents = 1;
	mushconf.req_cmds_flag = 1;
	mushconf.ansi_colors = 1;
	mushconf.safer_passwords = 0;
	mushconf.instant_recycle = 1;
	mushconf.dark_actions = 0;
	mushconf.no_ambiguous_match = 0;
	mushconf.exit_calls_move = 0;
	mushconf.move_match_more = 0;
	mushconf.autozone = 1;
	mushconf.page_req_equals = 0;
	mushconf.comma_say = 0;
	mushconf.you_say = 1;
	mushconf.c_cmd_subst = 1;
	mushconf.player_name_min = 0;
	mushconf.register_limit = 50;
	mushconf.max_qpid = 10000;
	mushconf.space_compress = 1; /*!< ??? Running SC on a non-SC DB may cause problems */
	mushconf.start_room = 0;
	mushconf.guest_start_room = NOTHING; /* default, use start_room */
	mushconf.start_home = NOTHING;
	mushconf.default_home = NOTHING;
	mushconf.master_room = NOTHING;
	mushconf.player_proto = NOTHING;
	mushconf.room_proto = NOTHING;
	mushconf.exit_proto = NOTHING;
	mushconf.thing_proto = NOTHING;
	mushconf.player_defobj = NOTHING;
	mushconf.room_defobj = NOTHING;
	mushconf.thing_defobj = NOTHING;
	mushconf.exit_defobj = NOTHING;
	mushconf.player_parent = NOTHING;
	mushconf.room_parent = NOTHING;
	mushconf.exit_parent = NOTHING;
	mushconf.thing_parent = NOTHING;
	mushconf.player_flags.word1 = 0;
	mushconf.player_flags.word2 = 0;
	mushconf.player_flags.word3 = 0;
	mushconf.room_flags.word1 = 0;
	mushconf.room_flags.word2 = 0;
	mushconf.room_flags.word3 = 0;
	mushconf.exit_flags.word1 = 0;
	mushconf.exit_flags.word2 = 0;
	mushconf.exit_flags.word3 = 0;
	mushconf.thing_flags.word1 = 0;
	mushconf.thing_flags.word2 = 0;
	mushconf.thing_flags.word3 = 0;
	mushconf.robot_flags.word1 = ROBOT;
	mushconf.robot_flags.word2 = 0;
	mushconf.robot_flags.word3 = 0;
	mushconf.stripped_flags.word1 = IMMORTAL | INHERIT | ROYALTY | WIZARD;
	mushconf.stripped_flags.word2 = BLIND | CONNECTED | GAGGED | HEAD_FLAG | SLAVE | STOP_MATCH | SUSPECT | UNINSPECTED;
	mushconf.stripped_flags.word3 = 0;
	mushconf.vattr_flags = 0;
	mushconf.vattr_flag_list = NULL;
	mushconf.flag_sep = XSTRDUP("_", "mushconf.flag_sep");
	mushconf.mush_name = XSTRDUP("TinyMUSH", "mushconf.mush_name");
	mushconf.one_coin = XSTRDUP("penny", "mushconf.one_coin");
	mushconf.many_coins = XSTRDUP("pennies", "mushconf.many_coins");
	mushconf.struct_dstr = XSTRDUP("\r\n", "mushconf.struct_dstr");
	mushconf.timeslice = 1000;
	mushconf.cmd_quota_max = 100;
	mushconf.cmd_quota_incr = 1;
	mushconf.lag_check = 1;
	mushconf.lag_check_clk = 1;
	mushconf.lag_check_cpu = 1;
	mushconf.malloc_logger = 0;
	mushconf.max_global_regs = 36;
	mushconf.max_command_args = 100;
	mushconf.player_name_length = 22;
	mushconf.hash_factor = 2;
	mushconf.max_cmdsecs = 120;
	mushconf.control_flags = 0xffffffff;      /* Everything for now... */
	mushconf.control_flags &= ~CF_GODMONITOR; /* Except for monitoring... */
	mushconf.log_options = LOG_ALWAYS | LOG_BUGS | LOG_SECURITY | LOG_NET | LOG_LOGIN | LOG_DBSAVES | LOG_CONFIGMODS | LOG_SHOUTS | LOG_STARTUP | LOG_WIZARD | LOG_PROBLEMS | LOG_PCREATES | LOG_TIMEUSE | LOG_LOCAL | LOG_MALLOC;
	mushconf.log_info = LOGOPT_TIMESTAMP | LOGOPT_LOC;
	mushconf.log_diversion = 0;
	mushconf.markdata[0] = 0x01;
	mushconf.markdata[1] = 0x02;
	mushconf.markdata[2] = 0x04;
	mushconf.markdata[3] = 0x08;
	mushconf.markdata[4] = 0x10;
	mushconf.markdata[5] = 0x20;
	mushconf.markdata[6] = 0x40;
	mushconf.markdata[7] = 0x80;
	mushconf.wild_times_lim = 25000;
	mushconf.cmd_nest_lim = 50;
	mushconf.cmd_invk_lim = 2500;
	mushconf.func_nest_lim = 50;
	mushconf.func_invk_lim = 2500;
	mushconf.parse_stack_limit = 64;
	mushconf.func_cpu_lim_secs = 60;
	mushconf.func_cpu_lim = 60 * CLOCKS_PER_SEC;
	mushconf.ntfy_nest_lim = 20;
	mushconf.fwdlist_lim = 100;
	mushconf.propdir_lim = 10;
	mushconf.lock_nest_lim = 20;
	mushconf.parent_nest_lim = 10;
	mushconf.zone_nest_lim = 20;
	mushconf.numvars_lim = 50;
	mushconf.stack_lim = 50;
	mushconf.struct_lim = 100;
	mushconf.instance_lim = 100;
	mushconf.max_grid_size = 1000;
	mushconf.max_player_aliases = 10;
	mushconf.cache_width = CACHE_WIDTH;
	mushconf.cache_size = CACHE_SIZE;
	mushstate.loading_db = 0;
	mushstate.panicking = 0;
	mushstate.standalone = 0;
	mushstate.logstderr = 1;
	mushstate.dumping = 0;
	mushstate.dumper = 0;
	mushstate.logging = 0;
	mushstate.epoch = 0;
	mushstate.generation = 0;
	mushstate.reboot_nums = 0;
	mushstate.mush_lognum = 0;
	mushstate.helpfiles = 0;
	mushstate.hfiletab = NULL;
	mushstate.hfiletab_size = 0;
	mushstate.cfiletab = NULL;
	mushstate.configfiles = 0;
	mushstate.hfile_hashes = NULL;
	mushstate.curr_player = NOTHING;
	mushstate.curr_enactor = NOTHING;
	mushstate.curr_cmd = (char *)"< none >";
	mushstate.shutdown_flag = 0;
	mushstate.flatfile_flag = 0;
	mushstate.backup_flag = 0;
	mushstate.attr_next = A_USER_START;
	mushstate.debug_cmd = (char *)"< init >";
	mushstate.doing_hdr = XSTRDUP("Doing", "mushstate.doing_hdr");
	mushstate.access_list = NULL;
	mushstate.suspect_list = NULL;
	mushstate.qfirst = NULL;
	mushstate.qlast = NULL;
	mushstate.qlfirst = NULL;
	mushstate.qllast = NULL;
	mushstate.qwait = NULL;
	mushstate.qsemfirst = NULL;
	mushstate.qsemlast = NULL;
	mushstate.badname_head = NULL;
	mushstate.mstat_ixrss[0] = 0;
	mushstate.mstat_ixrss[1] = 0;
	mushstate.mstat_idrss[0] = 0;
	mushstate.mstat_idrss[1] = 0;
	mushstate.mstat_isrss[0] = 0;
	mushstate.mstat_isrss[1] = 0;
	mushstate.mstat_secs[0] = 0;
	mushstate.mstat_secs[1] = 0;
	mushstate.mstat_curr = 0;
	mushstate.iter_alist.data = NULL;
	mushstate.iter_alist.len = 0;
	mushstate.iter_alist.next = NULL;
	mushstate.mod_alist = NULL;
	mushstate.mod_size = 0;
	mushstate.mod_al_id = NOTHING;
	mushstate.olist = NULL;
	mushstate.min_size = 0;
	mushstate.db_top = 0;
	mushstate.db_size = 0;
	mushstate.moduletype_top = DBTYPE_RESERVED;
	mushstate.freelist = NOTHING;
	mushstate.markbits = NULL;
	mushstate.cmd_nest_lev = 0;
	mushstate.cmd_invk_ctr = 0;
	mushstate.func_nest_lev = 0;
	mushstate.func_invk_ctr = 0;
	mushstate.wild_times_lev = 0;
	mushstate.cputime_base = clock();
	mushstate.ntfy_nest_lev = 0;
	mushstate.lock_nest_lev = 0;
	mushstate.zone_nest_num = 0;
	mushstate.in_loop = 0;
	mushstate.loop_token[0] = NULL;
	mushstate.loop_token2[0] = NULL;
	mushstate.loop_number[0] = 0;
	mushstate.loop_break[0] = 0;
	mushstate.in_switch = 0;
	mushstate.switch_token = NULL;
	mushstate.break_called = 0;
	mushstate.f_limitmask = 0;
	mushstate.inpipe = 0;
	mushstate.pout = NULL;
	mushstate.poutnew = NULL;
	mushstate.poutbufc = NULL;
	mushstate.poutobj = -1;
	mushstate.dbm_fd = -1;
	mushstate.rdata = NULL;
}

void cf_log(dbref player, const char *primary, const char *secondary, char *cmd, const char *format, ...)
{
	char *prefixed_format = XASPRINTF("prefixed_format", "%s: %s", cmd, format);
	va_list ap;
	va_start(ap, format);

	if (mushstate.initializing)
	{
		_log_write_va(__FILE__, __LINE__, LOG_STARTUP, primary, secondary, prefixed_format, ap);
	}
	else
	{
		_notify_check_va(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, prefixed_format, ap);
	}

	va_end(ap);
	XFREE(prefixed_format);
}

static CF_Result _cf_status_from_succfail(dbref player, char *cmd, int success, int failure)
{
	if (success > 0)
	{
		return (failure == 0) ? CF_Success : CF_Partial;
	}

	if (failure == 0)
	{
		cf_log(player, "CNF", "NDATA", cmd, "Nothing to set");
	}

	return CF_Failure;
}

CF_Result cf_const(int *vp, char *str, long extra, dbref player, char *cmd)
{
	(void)vp;
	(void)str;
	(void)extra;

	cf_log(player, "CNF", "SYNTX", cmd, "Cannot change a constant value");
	return CF_Failure;
}

CF_Result cf_int(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *endptr = NULL;
	long val = 0;

	errno = 0;
	val = strtol(str, &endptr, 10);

	if ((errno == ERANGE) || (val > INT_MAX) || (val < INT_MIN))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Value out of range or too large");
		return CF_Failure;
	}

	while (*endptr != '\0' && isspace((unsigned char)*endptr))
	{
		endptr++;
	}

	if (*endptr != '\0')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid numeric format: %s", str);
		return CF_Failure;
	}

	if ((extra > 0) && (val > extra))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %ld", extra);
		return CF_Failure;
	}

	*vp = (int)val;
	return CF_Success;
}

CF_Result cf_int_factor(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *endptr = NULL;
	long num = 0;

	errno = 0;
	num = strtol(str, &endptr, 10);

	if ((errno == ERANGE) || (num > INT_MAX) || (num < INT_MIN))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Value out of range or too large");
		return CF_Failure;
	}

	while (*endptr != '\0' && isspace((unsigned char)*endptr))
	{
		endptr++;
	}

	if (*endptr != '\0')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid numeric format: %s", str);
		return CF_Failure;
	}

	if ((extra > 0) && (num > extra))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %ld", extra);
		return CF_Failure;
	}

	if (num == 0)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Value cannot be 0. You may want a value of 1.");
		return CF_Failure;
	}

	*vp = (int)num;
	return CF_Success;
}

CF_Result cf_dbref(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *endptr = NULL;
	char *parse_start = (*str == '#') ? (str + 1) : str;
	long num = 0;

	errno = 0;
	num = strtol(parse_start, &endptr, 10);

	if ((errno == ERANGE) || (num > INT_MAX) || (num < INT_MIN))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "DBref value out of range");
		return CF_Failure;
	}

	while ((*endptr != '\0') && isspace((unsigned char)*endptr))
	{
		endptr++;
	}

	if (*endptr != '\0')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid DBref format: %s", str);
		return CF_Failure;
	}

	if (mushstate.initializing)
	{
		*vp = (int)num;
		return CF_Success;
	}

	if (((extra == NOTHING) && (num == NOTHING)) || (Good_obj(num) && !Going(num)))
	{
		*vp = (int)num;
		return CF_Success;
	}

	if (extra == NOTHING)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "A valid dbref, or -1, is required.");
	}
	else
	{
		cf_log(player, "CNF", "SYNTX", cmd, "A valid dbref is required.");
	}

	return CF_Failure;
}

CF_Result cf_bool(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *start = str;
	char *end = NULL;

	(void)extra;
	(void)player;
	(void)cmd;

	if (str == NULL)
	{
		*vp = 0;
		return CF_Success;
	}

	while ((*start != '\0') && isspace((unsigned char)*start))
	{
		start++;
	}

	end = start + strlen(start);
	while ((end > start) && isspace((unsigned char)end[-1]))
	{
		end--;
	}
	*end = '\0';

	*vp = (int)search_nametab(GOD, bool_names, start);

	if (*vp < 0)
	{
		*vp = 0;
	}

	return CF_Success;
}

CF_Result cf_option(int *vp, char *str, long extra, dbref player, char *cmd)
{
	NAMETAB *options = (NAMETAB *)extra;
	char *start = str;
	char *end = NULL;
	int i = 0;

	if ((str == NULL) || (options == NULL))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Option value is required");
		return CF_Failure;
	}

	while ((*start != '\0') && isspace((unsigned char)*start))
	{
		start++;
	}

	end = start + strlen(start);
	while ((end > start) && isspace((unsigned char)end[-1]))
	{
		end--;
	}
	*end = '\0';

	if (*start == '\0')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Option value is required");
		return CF_Failure;
	}

	i = search_nametab(GOD, options, start);

	if (i < 0)
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Value", start);
		return CF_Failure;
	}

	*vp = i;
	return CF_Success;
}

CF_Result cf_string(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *start = str;
	char *end = NULL;
	int retval = CF_Success;
	size_t len = 0;

	if (str == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "String value is required");
		return CF_Failure;
	}

	if (extra <= 0)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid string length limit");
		return CF_Failure;
	}

	while ((*start != '\0') && isspace((unsigned char)*start))
	{
		start++;
	}

	end = start + strlen(start);
	while ((end > start) && isspace((unsigned char)end[-1]))
	{
		end--;
	}
	*end = '\0';

	len = strlen(start);
	if (len >= (size_t)extra)
	{
		start[extra - 1] = '\0';
		cf_log(player, "CNF", "NFND", cmd, "String truncated");
		retval = CF_Failure;
	}

	XFREE(*(char **)vp);
	*(char **)vp = XSTRDUP(start, "vp");
	return retval;
}

CF_Result cf_modify_bits(int *vp, char *str, long extra, dbref player, char *cmd)
{
	NAMETAB *ntab = (NAMETAB *)extra;
	char *sp = NULL, *tokst = NULL;
	int f = 0, negate = 0, success = 0, failure = 0;

	if ((str == NULL) || (ntab == NULL))
	{
		return _cf_status_from_succfail(player, cmd, 0, 0);
	}

	sp = strtok_r(str, " \t", &tokst);

	while (sp != NULL)
	{
		negate = 0;

		if (*sp == '!')
		{
			negate = 1;
			sp++;

			if (*sp == '\0')
			{
				sp = strtok_r(NULL, " \t", &tokst);
				continue;
			}
		}

		f = search_nametab(GOD, ntab, sp);

		if (f > 0)
		{
			if (negate)
			{
				*vp &= ~f;
			}
			else
			{
				*vp |= f;
			}

			success++;
		}
		else
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", sp);
			failure++;
		}

		sp = strtok_r(NULL, " \t", &tokst);
	}

	return _cf_status_from_succfail(player, cmd, success, failure);
}

static bool _cf_modify_xfuncs(char *fn_name, int (*fn_ptr)(dbref), EXTFUNCS **xfuncs, bool negate)
{
	NAMEDFUNC *np = NULL, **tp = NULL;
	int i = 0;
	EXTFUNCS *xfp = *xfuncs;

	if (negate)
	{
		if (!xfp)
		{
			return false;
		}

		for (i = 0; i < xfp->num_funcs; i++)
		{
			if (!strcmp(xfp->ext_funcs[i]->fn_name, fn_name))
			{
				xfp->ext_funcs[i] = NULL;
				return true;
			}
		}

		return false;
	}

	np = NULL;

	for (i = 0; i < xfunctions.count; i++)
	{
		if (!strcmp(xfunctions.func[i]->fn_name, fn_name))
		{
			np = xfunctions.func[i];
			break;
		}
	}

	if (!np)
	{
		np = (NAMEDFUNC *)XMALLOC(sizeof(NAMEDFUNC), "np");
		np->fn_name = (char *)XSTRDUP(fn_name, "np->fn_name");
		np->handler = fn_ptr;
	}

	if (xfunctions.count == 0)
	{
		xfunctions.func = (NAMEDFUNC **)XMALLOC(sizeof(NAMEDFUNC *), "xfunctions.func");
		xfunctions.count = 1;
		xfunctions.func[0] = np;
	}
	else
	{
		tp = (NAMEDFUNC **)XREALLOC(xfunctions.func, (xfunctions.count + 1) * sizeof(NAMEDFUNC *), "tp");
		tp[xfunctions.count] = np;
		xfunctions.func = tp;
		xfunctions.count++;
	}

	if (!xfp)
	{
		xfp = (EXTFUNCS *)XMALLOC(sizeof(EXTFUNCS), "xfp");
		xfp->num_funcs = 1;
		xfp->ext_funcs = (NAMEDFUNC **)XMALLOC(sizeof(NAMEDFUNC *), "xfp->ext_funcs");
		xfp->ext_funcs[0] = np;
		*xfuncs = xfp;
		return true;
	}

	for (i = 0; i < xfp->num_funcs; i++)
	{
		if (!xfp->ext_funcs[i])
		{
			xfp->ext_funcs[i] = np;
			return true;
		}
	}

	tp = (NAMEDFUNC **)XREALLOC(xfp->ext_funcs, (xfp->num_funcs + 1) * sizeof(NAMEDFUNC *), "tp");
	tp[xfp->num_funcs] = np;
	xfp->ext_funcs = tp;
	xfp->num_funcs++;
	return true;
}

CF_Result cf_parse_ext_access(int *perms, EXTFUNCS **xperms, char *str, NAMETAB *ntab, dbref player, char *cmd)
{
	char *sp = NULL, *tokst = NULL, *cp = NULL, *ostr = NULL, *s = NULL;
	int f = 0, negate = 0, success = 0, failure = 0, got_one = 0;
	MODULE *mp = NULL;
	int (*hp)(dbref) = NULL;

	success = failure = 0;
	sp = strtok_r(str, " \t", &tokst);

	while (sp != NULL)
	{
		negate = 0;

		if (*sp == '!')
		{
			negate = 1;
			sp++;
		}

		f = search_nametab(GOD, ntab, sp);

		if (f > 0)
		{
			if (negate)
			{
				*perms &= ~f;
			}
			else
			{
				*perms |= f;
			}

			success++;
		}
		else
		{
			got_one = 0;

			if (!strncmp(sp, "mod_", 4))
			{
				s = XMALLOC(MBUF_SIZE, "s");
				ostr = (char *)XSTRDUP(sp, "ostr");

				if (*(sp + 4) != '\0')
				{
					cp = strchr(sp + 4, '_');

					if (cp)
					{
						*cp++ = '\0';

						for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
						{
							got_one = 1;

							if (!strcmp(sp + 4, mp->modname))
							{
								XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, cp);
								hp = (int (*)(dbref))dlsym(mp->handle, s);

								if (!hp)
								{
									cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Module function", str);
									failure++;
								}
								else
								{
									if (_cf_modify_xfuncs(ostr, hp, xperms, negate))
									{
										success++;
									}
									else
									{
										failure++;
									}
								}

								break;
							}
						}

						if (!got_one)
						{
							cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Loaded module", str);
							got_one = 1;
						}
					}
				}

				XFREE(ostr);
				XFREE(s);
			}

			if (!got_one)
			{
				cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
				failure++;
			}
		}

		sp = strtok_r(NULL, " \t", &tokst);
	}

	return _cf_status_from_succfail(player, cmd, success, failure);
}

CF_Result cf_set_flags(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *sp = NULL, *tokst = NULL;
	FLAGENT *fp = NULL;
	FLAGSET *fset = (FLAGSET *)vp;
	int success = 0, failure = 0;

	(void)extra;

	if (str == NULL)
	{
		fset->word1 = 0;
		fset->word2 = 0;
		fset->word3 = 0;
		return CF_Success;
	}

	success = failure = 0;
	sp = strtok_r(str, " \t", &tokst);

	while (sp != NULL)
	{
		for (char *p = sp; *p; ++p)
		{
			*p = toupper((unsigned char)*p);
		}

		fp = (FLAGENT *)hashfind(sp, &mushstate.flags_htab);

		if (fp != NULL)
		{
			if (success == 0)
			{
				fset->word1 = 0;
				fset->word2 = 0;
				fset->word3 = 0;
			}

			if (fp->flagflag & FLAG_WORD3)
			{
				fset->word3 |= fp->flagvalue;
			}
			else if (fp->flagflag & FLAG_WORD2)
			{
				fset->word2 |= fp->flagvalue;
			}
			else
			{
				fset->word1 |= fp->flagvalue;
			}

			success++;
		}
		else
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", sp);
			failure++;
		}

		sp = strtok_r(NULL, " \t", &tokst);
	}

	if ((success == 0) && (failure == 0))
	{
		fset->word1 = 0;
		fset->word2 = 0;
		fset->word3 = 0;
		return CF_Success;
	}

	if (success > 0)
	{
		return ((failure == 0) ? CF_Success : CF_Partial);
	}

	return CF_Failure;
}

CF_Result cf_badname(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *start = str;
	char *end = NULL;

	(void)vp;
	(void)player;
	(void)cmd;

	if (str == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Missing name to add/remove.");
		return CF_Failure;
	}

	while ((*start != '\0') && isspace((unsigned char)*start))
	{
		start++;
	}

	end = start + strlen(start);
	while ((end > start) && isspace((unsigned char)end[-1]))
	{
		end--;
	}
	*end = '\0';

	if (*start == '\0')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Empty name not allowed.");
		return CF_Failure;
	}

	if (extra)
	{
		badname_remove(start);
	}
	else
	{
		badname_add(start);
	}

	return CF_Success;
}
