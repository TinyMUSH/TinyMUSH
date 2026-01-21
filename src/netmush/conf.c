/**
 * @file conf.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration parsing, defaults, and runtime reload support
 * @version 4.0
 *
 * Provides the core configuration loader for TinyMUSH, including default
 * initialization, directive parsing, runtime updates, and validation helpers.
 * This module centralizes all configuration handlers and bridges startup
 * logging, runtime notification, and module-provided configuration tables.
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

#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

CONFDATA mushconf;
STATEDATA mushstate;

/**
 * @brief Initialize mushconf and mushstate to default values
 *
 * Populates global configuration and state structures with startup defaults,
 * allocates initial buffers, and seeds runtime counters. This must be called
 * exactly once during process startup before any configuration parsing or
 * database loading occurs.
 *
 * @return void
 * @note Thread-safe: No; mutates global state.
 */
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
    mushconf.stripped_flags.word2 = BLIND | CONNECTED | GAGGED | HEAD_FLAG | SLAVE | STAFF | STOP_MATCH | SUSPECT | UNINSPECTED;
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

/**
 * @brief Log a configuration message with contextual prefix
 *
 * Creates a prefixed format string that embeds the configuration directive
 * name, then routes the variadic message either to the startup log (during
 * initialization) or to the requesting player (at runtime). Uses a single
 * allocation for the prefixed format string and forwards the variadic list to
 * the appropriate logging backend without additional formatting passes.
 *
 * @param player    DBref of player (ignored during startup)
 * @param primary   Primary error type (e.g., "CNF")
 * @param secondary Secondary error type (e.g., "SYNTX")
 * @param cmd       Configuration directive name for context
 * @param format    Printf-style format string
 * @param ...       Variable parameters for format string
 *
 * @return void
 * @note Thread-safe: No (mutates global mushstate during logging).
 * @note Allocation: Exactly one heap allocation for the prefixed format.
 */
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

/**
 * @brief Convert success/failure tallies into a CF_Result
 *
 * Summarizes the outcome of configuration sub-operations based on counts of
 * successes and failures. When both counts are zero, logs "Nothing to set"
 * via `cf_log()` to give feedback without changing the result status.
 *
 * Result mapping:
 * - success > 0 and failure == 0  => CF_Success
 * - success > 0 and failure > 0   => CF_Partial
 * - success == 0                  => CF_Failure (logs NDATA when failure == 0)
 *
 * @param player    DBref of the requesting player (ignored at startup)
 * @param cmd       Configuration directive name (context for logs)
 * @param success   Number of successful sub-operations
 * @param failure   Number of failed sub-operations
 * @return CF_Result Aggregated status derived from the counts
 *
 * @note Message routing handled by `cf_log()`.
 * @see cf_log()
 */
CF_Result cf_status_from_succfail(dbref player, char *cmd, int success, int failure)
{
    /* Success present: full success if no errors, else partial */
    if (success > 0)
    {
        return (failure == 0) ? CF_Success : CF_Partial;
    }

    /* No successes. If no failures either, emit informational message */
    if (failure == 0)
    {
        cf_log(player, "CNF", "NDATA", cmd, "Nothing to set");
    }

    /* Always failure in the remaining cases */
    return CF_Failure;
}

/**
 * @brief Reject attempts to modify read-only configuration parameters
 *
 * Acts as a handler for immutable directives. Immediately logs a syntax error
 * through `cf_log()` and returns `CF_Failure`, leaving the parameter unchanged
 * during both startup and runtime.
 *
 * @param vp        Unused value pointer (required by handler signature)
 * @param str       Unused attempted value
 * @param extra     Unused extra metadata
 * @param player    DBref of the requesting player (for runtime messages)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result Always `CF_Failure`
 *
 * @note Message routing handled by `cf_log()`.
 * @see cf_log()
 */
CF_Result cf_const(int *vp, char *str, long extra, dbref player, char *cmd)
{
    /* Explicitly mark unused parameters to avoid warnings */
    (void)vp;
    (void)str;
    (void)extra;

    /* Fail on any attempt to change the value */
    cf_log(player, "CNF", "SYNTX", cmd, "Cannot change a constant value");
    return CF_Failure;
}

/**
 * @brief Parse and store a plain integer configuration value
 *
 * Converts `str` with `strtol`, validates numeric range, rejects trailing
 * non-space characters, and enforces an optional upper bound (`extra`). Logs
 * descriptive errors through `cf_log()` and returns `CF_Failure` on any
 * validation issue; otherwise writes the parsed value into `vp`.
 *
 * Validations performed:
 * - Range: rejects `ERANGE` from `strtol` or values outside the `int` domain.
 * - Format: rejects trailing characters other than whitespace.
 * - Upper bound: when `extra > 0`, rejects values greater than `extra`.
 *
 * @param vp        Output pointer to store the parsed integer
 * @param str       Input string to parse
 * @param extra     Optional upper bound (ignored when <= 0)
 * @param player    DBref of the requesting player (for runtime messages)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on acceptance, otherwise `CF_Failure`
 *
 * @note Errors route through `cf_log()` (startup log vs runtime notify).
 * @see cf_log()
 */
CF_Result cf_int(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *endptr = NULL;
    long val = 0;

    /* Copy the numeric value to the parameter. Use strtol with proper error handling. */
    errno = 0;
    val = strtol(str, &endptr, 10);

    /* Validate conversion: check for range errors and invalid input */
    if ((errno == ERANGE) || (val > INT_MAX) || (val < INT_MIN))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value out of range or too large");
        return CF_Failure;
    }

    /* Skip any trailing whitespace before final format validation */
    while (*endptr != '\0' && isspace((unsigned char)*endptr))
    {
        endptr++;
    }

    /* Verify entire string was consumed (no trailing garbage) */
    if (*endptr != '\0')
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid numeric format: %s", str);
        return CF_Failure;
    }

    /* Check against upper limit if specified */
    if ((extra > 0) && (val > extra))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %ld", extra);
        return CF_Failure;
    }

    *vp = (int)val;
    return CF_Success;
}

/**
 * @brief Parse and store an integer factor (must be non-zero)
 *
 * Converts `str` with `strtol`, validates range, format, optional upper bound
 * (`extra`), and enforces that the resulting value is non-zero. On any
 * validation failure, emits an explanatory message via `cf_log()` and returns
 * `CF_Failure`; otherwise writes the parsed value to `vp`.
 *
 * @param vp        Output pointer to store the parsed integer factor
 * @param str       Input string to parse
 * @param extra     Optional upper bound (ignored when <= 0)
 * @param player    DBref of the requesting player (for runtime messages)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on acceptance, otherwise `CF_Failure`
 *
 * @note Errors route through `cf_log()` (startup log vs runtime notify).
 * @see cf_log()
 */
CF_Result cf_int_factor(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *endptr = NULL;
    long num = 0;

    /* Parse integer with proper error handling. Factors cannot be 0, so we check this explicitly. */
    errno = 0;
    num = strtol(str, &endptr, 10);

    /* Validate conversion: check for range errors and invalid input */
    if ((errno == ERANGE) || (num > INT_MAX) || (num < INT_MIN))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value out of range or too large");
        return CF_Failure;
    }

    /* Skip any trailing whitespace before final format validation */
    while (*endptr != '\0' && isspace((unsigned char)*endptr))
    {
        endptr++;
    }

    /* Verify entire string was consumed (no trailing garbage) */
    if (*endptr != '\0')
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid numeric format: %s", str);
        return CF_Failure;
    }

    /* Check against upper limit if specified */
    if ((extra > 0) && (num > extra))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %ld", extra);
        return CF_Failure;
    }

    /* Factor cannot be zero */
    if (num == 0)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value cannot be 0. You may want a value of 1.");
        return CF_Failure;
    }

    *vp = (int)num;
    return CF_Success;
}

/**
 * @brief Parse and store a dbref configuration value
 *
 * Parses a dbref string (optional leading '#'), validates numeric range and
 * trailing format, and, outside initialization, confirms the object exists or
 * is explicitly allowed to be `NOTHING` when `extra == NOTHING`. Logs
 * descriptive errors through `cf_log()` on failure.
 *
 * @param vp        Output pointer to store the parsed dbref
 * @param str       Input string to parse (may start with '#')
 * @param extra     Allows `NOTHING` when set to `NOTHING`; otherwise requires a valid object
 * @param player    DBref of the requesting player (for runtime messages)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on acceptance, otherwise `CF_Failure`
 *
 * @note Errors route through `cf_log()` (startup log vs runtime notify).
 * @see cf_log()
 */
CF_Result cf_dbref(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *endptr = NULL;
    char *parse_start = (*str == '#') ? (str + 1) : str;
    long num = 0;

    errno = 0;
    num = strtol(parse_start, &endptr, 10);

    /* Validate conversion: check for range errors and invalid input */
    if ((errno == ERANGE) || (num > INT_MAX) || (num < INT_MIN))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "DBref value out of range");
        return CF_Failure;
    }

    /* Skip trailing whitespace and verify entire string was consumed */
    while ((*endptr != '\0') && isspace((unsigned char)*endptr))
    {
        endptr++;
    }

    if (*endptr != '\0')
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid DBref format: %s", str);
        return CF_Failure;
    }

    /* No consistency check on initialization. Accept any value during startup. */
    if (mushstate.initializing)
    {
        *vp = (int)num;
        return CF_Success;
    }

    /* Validate the dbref is either NOTHING (if allowed) or a valid object */
    if (((extra == NOTHING) && (num == NOTHING)) || (Good_obj(num) && !Going(num)))
    {
        *vp = (int)num;
        return CF_Success;
    }

    /* Report appropriate error message */
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

/**
 * @brief Load a shared module and cache its entry points
 *
 * Opens `lib<modname>.so` from the configured modules directory, allocates a
 * MODULE node, links it into `mushstate.modules_list`, and resolves all module
 * entry points needed by the server. If not running standalone, it also calls
 * the module's optional `init` entry point.
 *
 * @param vp        Unused value pointer (handler signature compliance)
 * @param modname   Module base name (without lib prefix or suffix)
 * @param extra     Unused extra metadata
 * @param player    DBref of the requesting player (ignored during startup)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on successful load, otherwise `CF_Failure`
 *
 * @note Thread-safe: No; mutates global module registry and resolves symbols.
 * @attention Caller does not own the returned handles; they remain cached in mushstate.
 */
CF_Result cf_module(int *vp, char *modname, long extra, dbref player, char *cmd)
{
    MODULE *mp = NULL;
    MODULE *iter = NULL;
    void *handle = NULL;
    void (*initptr)(void) = NULL;
    char *name = modname;
    char *end = NULL;

    if (modname == NULL)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Module name is required");
        return CF_Failure;
    }

    /* Trim leading and trailing whitespace from module name */
    while (*name != '\0' && isspace((unsigned char)*name))
    {
        name++;
    }

    end = name + strlen(name);
    while ((end > name) && isspace((unsigned char)end[-1]))
    {
        end--;
    }
    *end = '\0';

    /* Reject empty names */
    if (*name == '\0')
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Module name is required");
        return CF_Failure;
    }

    /* Skip load if already present */
    for (iter = mushstate.modules_list; iter != NULL; iter = iter->next)
    {
        if (strcmp(iter->modname, name) == 0)
        {
            cf_log(player, "CNF", "MOD", cmd, "Module %s already loaded", name);
            return CF_Success;
        }
    }

    handle = dlopen_format("%s/lib%s.so", mushconf.modules_home, name);

    if (!handle)
    {
        cf_log(player, "CNF", "MOD", cmd, "Loading of %s/lib%s.so failed: %s", mushconf.modules_home, name, dlerror());
        return CF_Failure;
    }

    mp = (MODULE *)XMALLOC(sizeof(MODULE), "mp");
    mp->modname = XSTRDUP(name, "mp->modname");
    mp->handle = handle;
    mp->next = mushstate.modules_list;
    mushstate.modules_list = mp;

    /* Look up our symbols now, and cache the pointers. They're not going to change from here on out. */
    mp->process_command = (int (*)(dbref, dbref, int, char *, char *[], int))dlsym_format(handle, "mod_%s_%s", name, "process_command");
    mp->process_no_match = (int (*)(dbref, dbref, int, char *, char *, char *[], int))dlsym_format(handle, "mod_%s_%s", name, "process_no_match");
    mp->did_it = (int (*)(dbref, dbref, dbref, int, const char *, int, const char *, int, int, char *[], int, int))dlsym_format(handle, "mod_%s_%s", name, "did_it");
    mp->create_obj = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", name, "create_obj");
    mp->destroy_obj = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", name, "destroy_obj");
    mp->create_player = (void (*)(dbref, dbref, int, int))dlsym_format(handle, "mod_%s_%s", name, "create_player");
    mp->destroy_player = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", name, "destroy_player");
    mp->announce_connect = (void (*)(dbref, const char *, int))dlsym_format(handle, "mod_%s_%s", name, "announce_connect");
    mp->announce_disconnect = (void (*)(dbref, const char *, int))dlsym_format(handle, "mod_%s_%s", name, "announce_disconnect");
    mp->examine = (void (*)(dbref, dbref, dbref, int, int))dlsym_format(handle, "mod_%s_%s", name, "examine");
    mp->dump_database = (void (*)(FILE *))dlsym_format(handle, "mod_%s_%s", name, "dump_database");
    mp->db_grow = (void (*)(int, int))dlsym_format(handle, "mod_%s_%s", name, "db_grow");
    mp->db_write = (void (*)(void))dlsym_format(handle, "mod_%s_%s", name, "db_write");
    mp->db_write_flatfile = (void (*)(FILE *))dlsym_format(handle, "mod_%s_%s", name, "db_write_flatfile");
    mp->do_second = (void (*)(void))dlsym_format(handle, "mod_%s_%s", name, "do_second");
    mp->cache_put_notify = (void (*)(UDB_DATA, unsigned int))dlsym_format(handle, "mod_%s_%s", name, "cache_put_notify");
    mp->cache_del_notify = (void (*)(UDB_DATA, unsigned int))dlsym_format(handle, "mod_%s_%s", name, "cache_del_notify");

    if (!mushstate.standalone)
    {
        if ((initptr = (void (*)(void))dlsym_format(handle, "mod_%s_%s", modname, "init")) != NULL)
        {
            (*initptr)();
        }
    }

    log_write(LOG_STARTUP, "CNF", "MOD", "Loaded module: %s", modname);
    return CF_Success;
}

/**
 * @brief Parse and set a boolean configuration value
 *
 * Looks up `str` in the boolean name table and stores the resulting value into
 * `vp`. Unknown values resolve to false but still return `CF_Success` to
 * preserve legacy behavior.
 *
 * @param vp        Output pointer to store the boolean result
 * @param str       Input string to parse
 * @param extra     Unused (handler signature compliance)
 * @param player    DBref of the requesting player
 * @param cmd       Configuration directive name
 * @return CF_Result Always `CF_Success`
 *
 * @note Unknown values are accepted and mapped to false for legacy behavior.
 */
CF_Result cf_bool(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *start = str;
    char *end = NULL;

    if (str == NULL)
    {
        *vp = 0;
        return CF_Success;
    }

    /* Trim leading and trailing whitespace before lookup */
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

/**
 * @brief Parse a single option from a name table
 *
 * Resolves `str` against the provided `NAMETAB` (passed in `extra`) and stores
 * the matching index into `vp`. Logs a not-found error and returns
 * `CF_Failure` if the option is unknown.
 *
 * @param vp        Output pointer to store the resolved option index
 * @param str       Input string to parse
 * @param extra     Pointer to a `NAMETAB` containing valid options
 * @param player    DBref of the requesting player (for runtime messages)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on match, otherwise `CF_Failure`
 */
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

	/* Trim leading and trailing whitespace before lookup */
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

/**
 * @brief Set a string configuration value with optional truncation
 *
 * Duplicates `str` into the location pointed to by `vp`, truncating the input
 * when it exceeds the maximum length specified in `extra`. Truncation is
 * logged to startup logs or notified to the requesting player. Frees the
 * previous string before replacement.
 *
 * @param vp        Pointer to the destination string slot
 * @param str       Input string to store
 * @param extra     Maximum allowed length (including terminator)
 * @param player    DBref of player (for runtime notifications)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on store, `CF_Failure` if truncation occurred
 */
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

	/* Trim leading and trailing whitespace */
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

	/* Check for truncation */
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

/**
 * @brief Ensure an alias target hash table is initialized
 *
 * Lazily initializes well-known hash tables before alias manipulation so
 * lookup operations run against populated tables. Unknown tables are treated
 * as unsupported and return false without side effects.
 *
 * @param htab     Target hash table pointer (required)
 * @return bool    true if the table is initialized; false on NULL or
 *                 unsupported tables
 *
 * @note Thread-safe: No; may invoke init_* routines that modify globals.
 */
static bool _cf_alias_ensure_hashtab(HASHTAB *htab)
{
    bool initialized = false;

    if (htab == NULL)
    {
        return false;
    }

    /* HT_STR tables have flags == 0, so use entry/hashsize to detect init */
    initialized = ((htab->entry != NULL) && (htab->hashsize > 0));

    if (initialized)
    {
        return true;
    }

    if (htab == &mushstate.command_htab)
    {
        init_cmdtab();
    }
    else if (htab == &mushstate.logout_cmd_htab)
    {
        init_logout_cmdtab();
    }
    else if (htab == &mushstate.flags_htab)
    {
        init_flagtab();
    }
    else if (htab == &mushstate.powers_htab)
    {
        init_powertab();
    }
    else if (htab == &mushstate.func_htab)
    {
        init_functab();
    }
    else if (htab == &mushstate.attr_name_htab)
    {
        init_attrtab();
    }

    return ((htab->entry != NULL) && (htab->hashsize > 0));
}

/**
 * @brief Define an alias entry inside a hash table
 *
 * Creates or replaces an alias for an existing hash entry, matching the
 * original key case-insensitively. When `HT_KEYREF` is set on the hash table,
 * duplicates the alias key to preserve ownership expectations. Logs an error
 * through `cf_log()` when the original key is missing.
 *
 * @param vp        Pointer to target hash table cast to int * (handler signature)
 * @param str       Input string containing alias and original, separated by whitespace or commas
 * @param extra     Name used for not-found logging (cast from const char *)
 * @param player    DBref of the requesting player (for runtime messages)
 * @param cmd       Configuration directive name (context in logs)
 * @return CF_Result `CF_Success` on alias creation, otherwise `CF_Failure`
 */
CF_Result cf_alias(int *vp, char *str, long extra, dbref player, char *cmd)
{
	HASHTAB *htab = (HASHTAB *)vp;
	int *cp = NULL;
	char *p = NULL;
	char *tokst = NULL;
	char *alias_start = NULL;
	char *alias_end = NULL;
	char *orig_start = NULL;
	char *orig_end = NULL;
	int upcase = 0;

    if ((str == NULL) || (htab == NULL))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias configuration requires valid input");
		return CF_Failure;
	}

    if (!_cf_alias_ensure_hashtab(htab))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid hash table for alias");
		return CF_Failure;
	}

	/* Parse first token as alias */
	alias_start = strtok_r(str, " \t=,", &tokst);

	if (alias_start == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias requires name");
		return CF_Failure;
	}

	/* Parse second token as original */
	orig_start = strtok_r(NULL, " \t=,", &tokst);

	if (orig_start == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias %s requires original entry", alias_start);
		return CF_Failure;
	}

	/* Trim trailing whitespace from both alias and orig */
	alias_end = alias_start + strlen(alias_start);
	while ((alias_end > alias_start) && isspace((unsigned char)alias_end[-1]))
	{
		alias_end--;
	}
	*alias_end = '\0';

	orig_end = orig_start + strlen(orig_start);
	while ((orig_end > orig_start) && isspace((unsigned char)orig_end[-1]))
	{
		orig_end--;
	}
	*orig_end = '\0';

	/* Reject empty names */
	if ((*alias_start == '\0') || (*orig_start == '\0'))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias and original names cannot be empty");
		return CF_Failure;
	}

	/* Convert orig to lowercase and search */
	upcase = 0;

	for (p = orig_start; *p; p++)
	{
		*p = tolower(*p);
	}

    cp = hashfind(orig_start, htab);

	/* If not found, try uppercase */
	if (cp == NULL)
	{
		upcase++;

		for (p = orig_start; *p; p++)
		{
			*p = toupper(*p);
		}

        cp = hashfind(orig_start, htab);

		if (cp == NULL)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", (char *)extra, orig_start);
			return CF_Failure;
		}
	}

	/* Convert alias to match case of orig */
	if (upcase)
	{
		for (p = alias_start; *p; p++)
		{
			*p = toupper(*p);
		}
	}
	else
	{
		for (p = alias_start; *p; p++)
		{
			*p = tolower(*p);
		}
	}

	/* Handle key reference flag */
    if (htab->flags & HT_KEYREF)
	{
		p = alias_start;
		alias_start = XSTRDUP(p, "alias");
	}

    return hashadd(alias_start, cp, htab, HASH_ALIAS);
}

/**
 * @brief Locate an INFO field and optionally its predecessor
 *
 * Walks the global `mushconf.infotext_list` to find a case-insensitive match
 * for `name`. Optionally returns the previous node pointer to simplify
 * removal in callers.
 *
 * @param name        Field name to search for (case-insensitive)
 * @param prev_out    Optional output for the node preceding the match; set to
 *                    NULL when the match is at the head or not found
 * @return LINKEDLIST* Matching node pointer, or NULL when no match is found
 *
 * @note Thread-safe: No; iterates shared configuration state.
 */
static LINKEDLIST *_conf_infotext_find(const char *name, LINKEDLIST **prev_out)
{
    LINKEDLIST *prev = NULL;
    LINKEDLIST *cur = mushconf.infotext_list;

    while (cur != NULL)
    {
        if (!strcasecmp(name, cur->name))
        {
            if (prev_out)
            {
                *prev_out = prev;
            }
            return cur;
        }

        prev = cur;
        cur = cur->next;
    }

    if (prev_out)
    {
        *prev_out = NULL;
    }

    return NULL;
}

/**
 * @brief Add, update, or remove an INFO field
 *
 * Parses a field name and optional value from the directive payload. If no
 * value is present, the named INFO entry is removed. When a value is present,
 * the existing entry is replaced or a new entry is pushed to the head of the
 * list. This handler always reports partial success to indicate work was
 * attempted regardless of add/update/delete direction.
 *
 * @param vp        Unused (required by handler signature)
 * @param str       Field name followed by optional value content
 * @param extra     Unused
 * @param player    DBref of player (ignored; no runtime messaging)
 * @param cmd       Configuration directive name
 * @return CF_Result Always `CF_Partial` to indicate the handler performed work
 *
 * @note Thread-safe: No; directly mutates the global INFO list.
 */
CF_Result cf_infotext(int *vp, char *str, long extra, dbref player, char *cmd)
{
	LINKEDLIST *prev = NULL;
	LINKEDLIST *itp = NULL;
	char *fvalue = NULL, *tokst = NULL;
	char *fname = NULL;

	/* Paramètres inutilisés par ce handler */
	(void)vp;
	(void)extra;
	(void)player;
	(void)cmd;

	/* Nom du champ */
	fname = strtok_r(str, " \t=,", &tokst);

	if (!fname || (*fname == '\0'))
	{
		return CF_Partial;
	}

	/* Valeur brute: avancer au premier caractère non-espace */
	if (tokst)
	{
		for (fvalue = tokst; *fvalue && ((*fvalue == ' ') || (*fvalue == '\t')); fvalue++)
		{
			/* noop */
		}
	}
	else
	{
		fvalue = NULL;
	}

	/* Recherche du noeud existant (un seul passage) */
	itp = _conf_infotext_find(fname, &prev);

	/* Suppression si valeur absente */
	if (!fvalue || !*fvalue)
	{
		if (itp)
		{
			if (prev)
			{
				prev->next = itp->next;
			}
			else
			{
				mushconf.infotext_list = itp->next;
			}

			XFREE(itp->name);
			XFREE(itp->value);
			XFREE(itp);
		}

		return CF_Partial;
	}

	/* Mise à jour en place */
	if (itp)
	{
		XFREE(itp->value);
		itp->value = XSTRDUP(fvalue, "itp->value");
		return CF_Partial;
	}

	/* Ajout en tête */
	itp = (LINKEDLIST *)XMALLOC(sizeof(LINKEDLIST), "itp");
	itp->name = XSTRDUP(fname, "itp->name");
	itp->value = XSTRDUP(fvalue, "itp->value");
	itp->next = mushconf.infotext_list;
	mushconf.infotext_list = itp;
	return CF_Partial;
}

/**
 * @brief Divert a log category to a specific file
 *
 * Parses a log type and pathname, opens (or reuses) the target file, marks the
 * log type as diverted, and records the file handle for subsequent logging.
 * Prevents duplicate diversions and validates that the log category exists.
 *
 * @param vp        Pointer to log option bitfield
 * @param str       Input string with "<logtype> <path>"
 * @param extra     Pointer to a `NAMETAB` of valid log types
 * @param player    DBref of player (ignored; always logs to startup)
 * @param cmd       Configuration directive name
 * @return CF_Result `CF_Success` on diversion, otherwise `CF_Failure`
 */
CF_Result cf_divert_log(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *type_str = NULL, *file_str = NULL, *tokst = NULL;
    int f = 0, fd = 0;
    FILE *fptr = NULL;
    LOGFILETAB *tp = NULL, *lp = NULL, *target = NULL;

    /* Deux arguments requis */
    type_str = strtok_r(str, " \t", &tokst);
    file_str = strtok_r(NULL, " \t", &tokst);

    if (!type_str || !file_str)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Missing pathname to log to.");
        return CF_Failure;
    }

    /* Résoudre le type de log */
    f = search_nametab(GOD, (NAMETAB *)extra, type_str);

    if (f <= 0)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Log diversion", str);
        return CF_Failure;
    }

    /* Un seul passage: trouver la cible et un fileptr réutilisable si existant */
    for (lp = logfds_table; lp->log_flag; lp++)
    {
        if (lp->log_flag == f)
        {
            target = lp;
        }

        if (lp->filename && !strcmp(file_str, lp->filename))
        {
            fptr = lp->fileptr;
        }
    }

    /* Vérifier que la cible existe */
    if (!target)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Logfile table corruption", type_str);
        return CF_Failure;
    }

    /* Déjà détourné ? */
    if (target->filename != NULL)
    {
        log_write(LOG_STARTUP, "CNF", "DIVT", "Log type %s already diverted: %s", type_str, target->filename);
        return CF_Failure;
    }

    /* Ouvrir si nécessaire */
    if (!fptr)
    {
        fptr = fopen(file_str, "w");

        if (!fptr)
        {
            log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot open logfile: %s", file_str);
            return CF_Failure;
        }

        fd = fileno(fptr);
        if (fd == -1)
        {
            log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot get fd for logfile: %s", file_str);
            return CF_Failure;
        }
#ifdef FNDELAY
        if (fcntl(fd, F_SETFL, FNDELAY) == -1)
        {
            log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot make nonblocking: %s", file_str);
            return CF_Failure;
        }
#else
        if (fcntl(fd, F_SETFL, O_NDELAY) == -1)
        {
            log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot make nonblocking: %s", file_str);
            return CF_Failure;
        }
#endif
    }

    /* Marquer le détournement */
    target->fileptr = fptr;
    target->filename = XSTRDUP(file_str, "tp->filename");
    *vp |= f;
    return CF_Success;
}

/**
 * @brief Set or clear bits from a name list
 *
 * Parses tokens in `str`, optionally prefixed with '!' to clear bits, and
 * updates the flag word referenced by `vp` using entries from the provided
 * name table. Aggregates results with `cf_status_from_succfail()`.
 *
 * @param vp        Pointer to the flag word to modify
 * @param str       Space-separated list of flag names (prefix with '!' to clear)
 * @param extra     Pointer to a `NAMETAB` describing valid flags
 * @param player    DBref of player (context for error reporting)
 * @param cmd       Configuration directive name
 * @return CF_Result Aggregated success status
 */
CF_Result cf_modify_bits(int *vp, char *str, long extra, dbref player, char *cmd)
{
    NAMETAB *ntab = (NAMETAB *)extra;
    char *sp = NULL, *tokst = NULL;
    int f = 0, negate = 0, success = 0, failure = 0;

    if ((str == NULL) || (ntab == NULL))
    {
        return cf_status_from_succfail(player, cmd, 0, 0);
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

    return cf_status_from_succfail(player, cmd, success, failure);
}

/**
 * @brief Add or remove an external function entry
 *
 * Maintains the shared list of named external functions and attaches or detaches
 * a function pointer from the provided `xfuncs` array. Adds new named entries
 * to the global `xfunctions` registry when first encountered.
 *
 * @param fn_name   Name of the external function
 * @param fn_ptr    Function pointer to register
 * @param xfuncs    Pointer to the destination `EXTFUNCS` structure
 * @param negate    true to remove the function; false to add
 * @return bool     true on update, false if the removal target is missing
 */
bool modify_xfuncs(char *fn_name, int (*fn_ptr)(dbref), EXTFUNCS **xfuncs, bool negate)
{
    NAMEDFUNC *np = NULL, **tp = NULL;
    int i = 0;
    EXTFUNCS *xfp = *xfuncs;

    /* If we're negating, just remove it from the list of functions. */
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

    /* Have we encountered this function before? */
    np = NULL;

    for (i = 0; i < xfunctions.count; i++)
    {
        if (!strcmp(xfunctions.func[i]->fn_name, fn_name))
        {
            np = xfunctions.func[i];
            break;
        }
    }

    /* If not, we need to allocate it. */
    if (!np)
    {
        np = (NAMEDFUNC *)XMALLOC(sizeof(NAMEDFUNC), "np");
        np->fn_name = (char *)XSTRDUP(fn_name, "np->fn_name");
        np->handler = fn_ptr;
    }

    /* Add it to the ones we know about. */
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

    /* Do we have an existing list of functions? If not, this is easy. */
    if (!xfp)
    {
        xfp = (EXTFUNCS *)XMALLOC(sizeof(EXTFUNCS), "xfp");
        xfp->num_funcs = 1;
        xfp->ext_funcs = (NAMEDFUNC **)XMALLOC(sizeof(NAMEDFUNC *), "xfp->ext_funcs");
        xfp->ext_funcs[0] = np;
        *xfuncs = xfp;
        return true;
    }

    /* See if we have an empty slot to insert into. */
    for (i = 0; i < xfp->num_funcs; i++)
    {
        if (!xfp->ext_funcs[i])
        {
            xfp->ext_funcs[i] = np;
            return true;
        }
    }

    /* Guess not. Tack it onto the end. */
    tp = (NAMEDFUNC **)XREALLOC(xfp->ext_funcs, (xfp->num_funcs + 1) * sizeof(NAMEDFUNC *), "tp");
    tp[xfp->num_funcs] = np;
    xfp->ext_funcs = tp;
    xfp->num_funcs++;
    return true;
}

/**
 * @brief Parse an access list supporting module callouts
 *
 * Parses tokens that set or clear permission bits and optionally reference
 * module-provided callbacks (`mod_<module>_<func>`). Populates bitmask
 * permissions and extends the callable functions list as requested, logging
 * missing entries via `cf_log()`.
 *
 * @param perms     Pointer to permission bitmask to modify
 * @param xperms    Pointer to extended function list to update
 * @param str       Input string of permissions/callouts
 * @param ntab      Name table describing permission bits
 * @param player    DBref of player (context for logging)
 * @param cmd       Configuration directive name
 * @return CF_Result Aggregated success status
 */
CF_Result parse_ext_access(int *perms, EXTFUNCS **xperms, char *str, NAMETAB *ntab, dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL, *cp = NULL, *ostr = NULL, *s = NULL;
    int f = 0, negate = 0, success = 0, failure = 0, got_one = 0;
    MODULE *mp = NULL;
    int (*hp)(dbref) = NULL;

    /* Walk through the tokens */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);

    while (sp != NULL)
    {
        /* Check for negation */
        negate = 0;

        if (*sp == '!')
        {
            negate = 1;
            sp++;
        }

        /* Set or clear the appropriate bit */
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
            /* Is this a module callout? */
            got_one = 0;

            if (!strncmp(sp, "mod_", 4))
            {
                /* Split it apart, see if we have anything. */
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
                                    if (modify_xfuncs(ostr, hp, xperms, negate))
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

        /* Get the next token */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    return cf_status_from_succfail(player, cmd, success, failure);
}

/**
 * @brief Reset and populate a flagset from the flags hash table
 *
 * Clears the target flagset on first successful match, then sets bits for each
 * token found in the global flags hash. Returns partial success when some
 * tokens fail to resolve.
 *
 * @param vp        Pointer to a FLAGSET structure to update
 * @param str       Space-separated list of flag names
 * @param extra     Unused (handler signature compliance)
 * @param player    DBref of player (context for logging)
 * @param cmd       Configuration directive name
 * @return CF_Result Success/partial/failure based on resolved tokens
 */
CF_Result cf_set_flags(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL;
    FLAGENT *fp = NULL;
    FLAGSET *fset = (FLAGSET *)vp;
    int success = 0, failure = 0;

    /* Traiter chaîne nulle comme "aucun token": vider le flagset */
    if (str == NULL)
    {
        fset->word1 = 0;
        fset->word2 = 0;
        fset->word3 = 0;
        return CF_Success;
    }

    /* Parcours des tokens */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);

    while (sp != NULL)
    {
        /* Normaliser en majuscules le token uniquement */
        for (char *p = sp; *p; ++p)
        {
            *p = toupper((unsigned char)*p);
        }

        /* Résoudre le flag via la table globale */
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

        /* Token suivant */
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

/**
 * @brief Add or remove a forbidden player name
 *
 * Adds a name to the badname list when `extra` is false, or removes it when
 * `extra` is true. Used to enforce disallowed player names/aliases.
 *
 * @param vp        Unused (handler signature compliance)
 * @param str       Name to add or remove
 * @param extra     Non-zero to remove; zero to add
 * @param player    DBref of player (ignored)
 * @param cmd       Configuration directive name
 * @return CF_Result Always `CF_Success`
 */
CF_Result cf_badname(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *start = str;
    char *end = NULL;

    /* Valider l'entrée */
    if (str == NULL)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Missing name to add/remove.");
        return CF_Failure;
    }

    /* Trim des espaces en tête et en queue */
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

    /* Rejeter les noms vides après normalisation */
    if (*start == '\0')
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Empty name not allowed.");
        return CF_Failure;
    }

    /* Appliquer l'ajout ou la suppression */
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

/**
 * @brief Safe wrapper around inet_pton() for IPv4 dotted-quad strings
 *
 * Validates input and parses IPv4 address strings using `inet_pton`, which
 * handles all validation. Returns `INADDR_NONE` on NULL input or malformed
 * address.
 *
 * @param str       IPv4 address string
 * @return in_addr_t Parsed address or INADDR_NONE on error
 */
in_addr_t sane_inet_addr(char *str)
{
    struct in_addr addr;

    /* Validate input and parse with inet_pton */
    if ((str == NULL) || (inet_pton(AF_INET, str, &addr) != 1))
    {
        return INADDR_NONE;
    }

    return addr.s_addr;
}

/**
 * @brief Add a site access entry (allow/deny)
 *
 * Parses CIDR or netmask notation, constructs a SITE entry with the specified
 * flag (allow/deny), and links it into the site list. On startup, entries are
 * appended to preserve file order; at runtime, entries are prepended to take
 * precedence.
 *
 * @param vp        Pointer to the SITE list head storage
 *                   (cast from `long **` per handler convention)
 * @param str       Address and mask ("a.b.c.d m.m.m.m" or "a.b.c.d/len")
 * @param extra     Flag stored in the SITE entry (allow/deny)
 * @param player    DBref of player (context for logging)
 * @param cmd       Configuration directive name
 * @return CF_Result `CF_Success` on acceptance, otherwise `CF_Failure`
 */
CF_Result cf_site(long **vp, char *str, long extra, dbref player, char *cmd)
{
    SITE *site = NULL, *last = NULL, *head = NULL;
    char *addr_txt = NULL, *mask_txt = NULL, *tokst = NULL, *endp = NULL;
    struct in_addr addr_num, mask_num;
    int mask_bits = 0;

    /* Validate input */
    if (str == NULL)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Missing site address and mask.");
        return CF_Failure;
    }

    /* Initialize mask_num to avoid undefined state */
    memset(&mask_num, 0, sizeof(mask_num));

    /* Detect CIDR notation by '/' */
    if ((mask_txt = strchr(str, '/')) == NULL)
    {
        /* Standard netmask notation: addr mask */
        addr_txt = strtok_r(str, " \t=,", &tokst);
        if (addr_txt)
        {
            mask_txt = strtok_r(NULL, " \t=,", &tokst);
        }

        if (!addr_txt || !*addr_txt || !mask_txt || !*mask_txt)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Missing host address or mask.");
            return CF_Failure;
        }

        /* Validate address */
        if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
            return CF_Failure;
        }

        /* Validate netmask: special case for 255.255.255.255, else parse as IP */
        if (strcmp(mask_txt, "255.255.255.255") != 0)
        {
            if ((mask_num.s_addr = sane_inet_addr(mask_txt)) == INADDR_NONE)
            {
                cf_log(player, "CNF", "SYNTX", cmd, "Malformed mask address: %s", mask_txt);
                return CF_Failure;
            }
        }
        else
        {
            mask_num.s_addr = htonl(0xFFFFFFFFU);
        }
    }
    else
    {
        /* CIDR notation: addr/bits */
        addr_txt = str;
        *mask_txt++ = '\0';

        /* Parse mask bits */
        mask_bits = (int)strtol(mask_txt, &endp, 10);

        /* Validate mask_bits range and endp for garbage */
        if ((mask_bits < 0) || (mask_bits > 32) || (*endp != '\0'))
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Invalid CIDR mask: %s (expected 0-32)", mask_txt);
            return CF_Failure;
        }

        /* Calculate netmask from bits; mask_bits == 0 gives 0.0.0.0 */
        if (mask_bits == 0)
        {
            mask_num.s_addr = htonl(0);
        }
        else if (mask_bits == 32)
        {
            mask_num.s_addr = htonl(0xFFFFFFFFU);
        }
        else
        {
            mask_num.s_addr = htonl(0xFFFFFFFFU << (32 - mask_bits));
        }

        /* Validate address */
        if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
            return CF_Failure;
        }
    }

    head = (SITE *)*vp;

    /* Allocate site entry */
    site = (SITE *)XMALLOC(sizeof(SITE), "site");
    if (!site)
    {
        return CF_Failure;
    }

    /* Initialize the site entry */
    site->address.s_addr = addr_num.s_addr;
    site->mask.s_addr = mask_num.s_addr;
    site->flag = (long)extra;
    site->next = NULL;

    /* Link entry: append during init, prepend at runtime */
    if (mushstate.initializing)
    {
        if (head == NULL)
        {
            *vp = (long *)site;
        }
        else
        {
            /* Find last in chain */
            for (last = head; last->next != NULL; last = last->next)
                ;
            last->next = site;
        }
    }
    else
    {
        site->next = head;
        *vp = (long *)site;
    }

    return CF_Success;
}

/**
 * @brief Helper to adjust read/write access on a config directive
 *
 * Validates that the directive is not static, then routes to `cf_modify_bits`
 * to set either read permissions (`rperms`) or write permissions (`flags`)
 * based on the `vp` flag. Logs permission violations for static directives.
 *
 * @param tp        Target config table entry
 * @param player    DBref of player requesting the change
 * @param vp        Non-zero to edit read perms; zero to edit write perms
 * @param ap        Parsed argument list of permissions
 * @param cmd       Configuration directive name
 * @param extra     Pointer to access name table
 * @return CF_Result Result from `cf_modify_bits`, or `CF_Failure` on denial
 */
CF_Result helper_cf_cf_access(CONF *tp, dbref player, int *vp, char *ap, char *cmd, long extra)
{
    const char *access_type = ((long)vp) ? "read" : "write";

    /* Reject attempts to modify STATIC directives */
    if (tp->flags & CA_STATIC)
    {
        notify(player, NOPERM_MESSAGE);

        if (db)
        {
            char *name = log_getname(player);
            log_write(LOG_CONFIGMODS, "CFG", "PERM", "%s tried to change %s access to static param: %s", name, access_type, tp->pname);
            XFREE(name);
        }
        else
        {
            log_write(LOG_CONFIGMODS, "CFG", "PERM", "System tried to change %s access to static param: %s", access_type, tp->pname);
        }

        return CF_Failure;
    }

    /* Delegate to cf_modify_bits with appropriate target */
    if ((long)vp)
    {
        return cf_modify_bits(&tp->rperms, ap, extra, player, cmd);
    }
    else
    {
        return cf_modify_bits(&tp->flags, ap, extra, player, cmd);
    }
}

/**
 * @brief Configure read/write access for a named directive
 *
 * Locates the configuration directive (core or module) by name and delegates
 * to `helper_cf_cf_access` to apply read or write access changes as requested.
 * Logs an error when the directive is not found.
 *
 * @param vp        Non-zero to edit read perms; zero to edit write perms
 * @param str       Target configuration directive name and permission tokens
 * @param extra     Pointer to access name table
 * @param player    DBref of player (context for logging)
 * @param cmd       Configuration directive name
 * @return CF_Result Result of applying access changes or `CF_Failure` on miss
 */
CF_Result cf_cf_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *directive_name = NULL, *perms_str = NULL;
    char *str_copy = NULL;

    /* Validate input */
    if (str == NULL)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Missing directive name and permissions.");
        return CF_Failure;
    }

    /* Parse directive name and permissions: extract name, split on whitespace */
    str_copy = XSTRDUP(str, "str_copy");
    directive_name = str_copy;

    /* Find end of directive name (first whitespace) */
    for (perms_str = directive_name; *perms_str && !isspace((unsigned char)*perms_str); perms_str++)
        ;

    /* Null-terminate directive name and advance to permissions */
    if (*perms_str)
    {
        *perms_str++ = '\0';
        /* Skip leading whitespace in permissions */
        while (*perms_str && isspace((unsigned char)*perms_str))
            perms_str++;
    }
    else
    {
        perms_str = "";
    }

    /* Search in core configuration table */
    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcmp(tp->pname, directive_name))
        {
            CF_Result result = helper_cf_cf_access(tp, player, vp, perms_str, cmd, extra);
            XFREE(str_copy);
            return result;
        }
    }

    /* Search in module configuration tables */
    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcmp(tp->pname, directive_name))
                {
                    CF_Result result = helper_cf_cf_access(tp, player, vp, perms_str, cmd, extra);
                    XFREE(str_copy);
                    return result;
                }
            }
        }
    }

    /* Not found in core or modules */
    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config directive", directive_name);
    XFREE(str_copy);
    return CF_Failure;
}

/**
 * @brief Load and register a help/news file during startup
 *
 * Validates and locates the requested helpfile (absolute or relative to
 * `txthome`), rebuilds its index, and registers the associated command and
 * alias. Rejects names that would collide with @addcommand prefixes and logs
 * detailed errors on missing files or index failures. Intended for use during
 * initialization.
 *
 * @param player        DBref of the requester (logging context)
 * @param confcmd       Configuration directive name invoking the load
 * @param str           Filename and optional path for the helpfile
 * @param is_raw        true to mark as raw help (no formatting), false otherwise
 * @return CF_Result    CF_Success (0) when the helpfile loads; CF_Failure (-1)
 *                      when any validation or indexing step fails
 *
 * @note Thread-safe: No; mutates command/help registries and global tables.
 */
CF_Result add_helpfile(dbref player, char *confcmd, char *str, bool is_raw)
{
    CMDENT *cmdp = NULL;
    HASHTAB *hashes = NULL;
    FILE *fp = NULL;
    char *fcmd = NULL, *fpath = NULL, *newstr = NULL, *tokst = NULL;
    char **ftab = NULL;
    char *s = NULL;
    char *full_fpath = NULL;

    /* Validate inputs early */
    if ((str == NULL) || (confcmd == NULL))
    {
        cf_log(player, "CNF", "SYNTX", confcmd ? confcmd : "add_helpfile", "Missing input parameters");
        return -1;
    }

    /* Parse command and path; make copy to preserve original */
    newstr = XMALLOC(MBUF_SIZE, "newstr");
    XSTRCPY(newstr, str);
    fcmd = strtok_r(newstr, " \t=,", &tokst);
    fpath = strtok_r(NULL, " \t=,", &tokst);

    /* Validate parsed tokens */
    if ((fcmd == NULL) || (*fcmd == '\0') || (fpath == NULL) || (*fpath == '\0'))
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Missing command name or file path");
        XFREE(newstr);
        return -1;
    }

    /* Reject __* collision with @addcommand */
    if ((fcmd[0] == '_') && (fcmd[1] == '_'))
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s would cause @addcommand conflict", fcmd);
        XFREE(newstr);
        return -1;
    }

    /* Allocate path buffer now that we have validated fpath */
    s = XMALLOC(MAXPATHLEN, "s");

    /* Try to open file: first with given path, then with txthome prefix */
    XSNPRINTF(s, MAXPATHLEN, "%s.txt", fpath);
    fp = fopen(s, "r");

    if (fp == NULL)
    {
        full_fpath = XASPRINTF("full_fpath", "%s/%s", mushconf.txthome, fpath);
        XSNPRINTF(s, MAXPATHLEN, "%s.txt", full_fpath);
        fp = fopen(s, "r");

        if (fp == NULL)
        {
            cf_log(player, "HLP", "LOAD", confcmd, "Helpfile %s not found", fcmd);
            XFREE(newstr);
            XFREE(s);
            XFREE(full_fpath);
            return -1;
        }

        /* Use full path for remaining operations */
        fpath = full_fpath;
    }

    fclose(fp);

    /* Validate filename length before index rebuild */
    if (strlen(fpath) > SBUF_SIZE)
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s filename too long", fcmd);
        XFREE(newstr);
        XFREE(s);
        XFREE(full_fpath);
        return -1;
    }

    /* Log load attempt with basename for clarity */
    cf_log(player, "HLP", "LOAD", confcmd, "Loading helpfile %s", basename(fpath));

    /* Rebuild index; abort on failure */
    if (helpmkindx(player, confcmd, fpath))
    {
        cf_log(player, "HLP", "LOAD", confcmd, "Could not create index for helpfile %s, not loaded.", basename(fpath));
        XFREE(newstr);
        XFREE(s);
        XFREE(full_fpath);
        return -1;
    }

    /* Allocate and initialize command entry */
    cmdp = (CMDENT *)XMALLOC(sizeof(CMDENT), "cmdp");
    cmdp->cmdname = XSTRDUP(fcmd, "cmdp->cmdname");
    cmdp->switches = NULL;
    cmdp->perms = 0;
    cmdp->pre_hook = NULL;
    cmdp->post_hook = NULL;
    cmdp->userperms = NULL;
    cmdp->callseq = CS_ONE_ARG;
    cmdp->info.handler = do_help;
    cmdp->extra = mushstate.helpfiles;

    if (is_raw)
    {
        cmdp->extra |= HELP_RAWHELP;
    }

    /* Register command and alias in hash table */
    hashdelete(cmdp->cmdname, &mushstate.command_htab);
    hashadd(cmdp->cmdname, (int *)cmdp, &mushstate.command_htab, 0);
    XSNPRINTF(s, MAXPATHLEN, "__%s", cmdp->cmdname);
    hashdelete(s, &mushstate.command_htab);
    hashadd(s, (int *)cmdp, &mushstate.command_htab, HASH_ALIAS);

    /* Allocate or grow helpfiles table as needed */
    if (!mushstate.hfiletab)
    {
        mushstate.hfiletab = (char **)XCALLOC(4, sizeof(char *), "mushstate.hfiletab");
        mushstate.hfile_hashes = (HASHTAB *)XCALLOC(4, sizeof(HASHTAB), "mushstate.hfile_hashes");
        mushstate.hfiletab_size = 4;
    }
    else if (mushstate.helpfiles >= mushstate.hfiletab_size)
    {
        ftab = (char **)XREALLOC(mushstate.hfiletab, (mushstate.hfiletab_size + 4) * sizeof(char *), "ftab");
        hashes = (HASHTAB *)XREALLOC(mushstate.hfile_hashes, (mushstate.hfiletab_size + 4) * sizeof(HASHTAB), "hashes");
        ftab[mushstate.hfiletab_size] = NULL;
        ftab[mushstate.hfiletab_size + 1] = NULL;
        ftab[mushstate.hfiletab_size + 2] = NULL;
        ftab[mushstate.hfiletab_size + 3] = NULL;
        mushstate.hfiletab_size += 4;
        mushstate.hfiletab = ftab;
        mushstate.hfile_hashes = hashes;
    }

    /* Store helpfile path, replacing old one if present */
    if (mushstate.hfiletab[mushstate.helpfiles] != NULL)
    {
        XFREE(mushstate.hfiletab[mushstate.helpfiles]);
    }

    mushstate.hfiletab[mushstate.helpfiles] = XSTRDUP(fpath, "mushstate.hfiletab[mushstate.helpfiles]");

    /* Initialize hash table for this helpfile */
    hashinit(&mushstate.hfile_hashes[mushstate.helpfiles], 30 * mushconf.hash_factor, HT_STR);
    mushstate.helpfiles++;

    /* Log successful load */
    cf_log(player, "HLP", "LOAD", confcmd, "Successfully loaded helpfile %s", basename(fpath));

    /* Cleanup */
    XFREE(s);
    XFREE(newstr);
    XFREE(full_fpath);

    return 0;
}

/**
 * @brief Add a help/news file during startup
 *
 * Loads a helpfile, rebuilds its index, and registers the associated command
 * handler (and alias). Expects to run during initialization; logs and aborts on
 * missing files or index failures.
 *
 * @param vp            Unused (handler signature compliance)
 * @param str           Base filename (with or without path)
 * @param extra         Unused
 * @param player        DBref of player (for logging context)
 * @param cmd           Configuration directive name
 * @return CF_Result    `CF_Success` on load, `CF_Failure` on error
 */
CF_Result cf_helpfile(int *vp, char *str, long extra, dbref player, char *cmd)
{
    return add_helpfile(player, cmd, str, 0);
}

/**
 * @brief Add a raw (unformatted) helpfile during startup
 *
 * Identical to `cf_helpfile` but marks the entry as raw text, bypassing
 * formatting expectations when served to players.
 *
 * @param vp            Unused (handler signature compliance)
 * @param str           Base filename (with or without path)
 * @param extra         Unused
 * @param player        DBref of player (for logging context)
 * @param cmd           Configuration directive name
 * @return CF_Result    `CF_Success` on load, `CF_Failure` on error
 */
CF_Result cf_raw_helpfile(int *vp, char *str, long extra, dbref player, char *cmd)
{
    return add_helpfile(player, cmd, str, 1);
}

/**
 * @brief Include and parse another configuration file (startup only)
 *
 * Opens the requested file (or resolves it relative to `config_home`), logs
 * the include, and feeds each directive line to `cf_set`. Rejects use outside
 * initialization. Trims comments, handles whitespace, and reports read errors
 * via `cf_log()`.
 *
 * @param vp        Unused (handler signature compliance)
 * @param filename  Path to configuration file to include
 * @param extra     Unused (handler signature compliance)
 * @param player    DBref of player executing the include
 * @param cmd       Configuration directive name ("include")
 * @return CF_Result `CF_Success` on completion, otherwise `CF_Failure`
 */
CF_Result cf_include(int *vp, char *filename, long extra, dbref player, char *cmd)
{
    FILE *fp = NULL;
    char *filepath = NULL, *buf = NULL, *line_ptr = NULL, *cmd_ptr = NULL;
    char *arg_ptr = NULL, *comment_ptr = NULL, *trim_ptr = NULL;
    int line_num = 0;

    /* Validate inputs and startup context */
    if (!mushstate.initializing)
    {
        return CF_Failure;
    }

    if ((filename == NULL) || (cmd == NULL))
    {
        cf_log(player, "CNF", "SYNTX", cmd ? cmd : "include", "Missing filename parameter");
        return CF_Failure;
    }

    /* Try to open file with given path; fall back to config_home if needed */
    filepath = XSTRDUP(filename, "filepath");
    fp = fopen(filepath, "r");

    if (fp == NULL)
    {
        char *full_path = XASPRINTF("full_path", "%s/%s", mushconf.config_home, filename);
        fp = fopen(full_path, "r");

        if (fp == NULL)
        {
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config file", filename);
            XFREE(filepath);
            XFREE(full_path);
            return CF_Failure;
        }

        /* Use full path for logging and tracking */
        XFREE(filepath);
        filepath = full_path;
    }

    /* Log and track file inclusion */
    log_write(LOG_ALWAYS, "CNF", "INFO", "Reading configuration file : %s", filepath);
    mushstate.cfiletab = add_array(mushstate.cfiletab, filepath, &mushstate.configfiles);
    XFREE(filepath);

    buf = XMALLOC(LBUF_SIZE, "buf");

    /* Read and process configuration lines */
    while (fgets(buf, LBUF_SIZE, fp) != NULL)
    {
        line_num++;
        line_ptr = buf;

        /* Strip trailing newline */
        comment_ptr = strchr(line_ptr, '\n');
        if (comment_ptr)
        {
            *comment_ptr = '\0';
        }

        /* Skip leading whitespace and empty/comment lines */
        while (*line_ptr && isspace((unsigned char)*line_ptr))
        {
            line_ptr++;
        }

        if ((!*line_ptr) || (*line_ptr == '#'))
        {
            continue;
        }

        /* Extract command token (stop at first whitespace) */
        cmd_ptr = line_ptr;
        while (*cmd_ptr && !isspace((unsigned char)*cmd_ptr))
        {
            cmd_ptr++;
        }

        /* Separate command from arguments */
        if (*cmd_ptr)
        {
            *cmd_ptr++ = '\0';

            /* Skip spaces between command and arguments */
            while (*cmd_ptr && isspace((unsigned char)*cmd_ptr))
            {
                cmd_ptr++;
            }

            arg_ptr = cmd_ptr;
        }
        else
        {
            arg_ptr = cmd_ptr;
        }

        /* Find and remove inline comment from arguments (not digit-prefixed) */
        comment_ptr = strchr(arg_ptr, '#');
        if (comment_ptr)
        {
            /* Allow # only if preceded by whitespace and followed by digit (e.g., "#1 to 10") */
            if ((comment_ptr == arg_ptr) || (isspace((unsigned char)*(comment_ptr - 1)) && isdigit((unsigned char)*(comment_ptr + 1))))
            {
                /* This looks like a range or special syntax; keep it */
            }
            else
            {
                /* Treat as comment start; truncate */
                *comment_ptr = '\0';
            }
        }

        /* Trim trailing whitespace from arguments */
        trim_ptr = arg_ptr + strlen(arg_ptr);
        while ((trim_ptr > arg_ptr) && isspace((unsigned char)*(trim_ptr - 1)))
        {
            trim_ptr--;
        }
        *trim_ptr = '\0';

        /* Process the configuration directive if command is present */
        if (*line_ptr)
        {
            cf_set(line_ptr, arg_ptr, player);
        }
    }

    /* Verify successful read (check for errors vs. EOF) */
    if (!feof(fp))
    {
        cf_log(player, "CNF", "ERROR", cmd, "Line %d: Error reading configuration file", line_num);
        XFREE(buf);
        fclose(fp);
        return CF_Failure;
    }

    XFREE(buf);
    fclose(fp);
    return CF_Success;
}

int (*cf_interpreter)(int *, char *, long, dbref, char *);
/**
 * @brief Execute a configuration handler and log the attempt
 *
 * Invokes the interpreter for a specific config table entry, logging the
 * caller, arguments, and status during runtime. Preserves the current handler
 * pointer in `cf_interpreter` for downstream reference.
 *
 * @param cp        Configuration directive name
 * @param ap        Arguments string
 * @param player    DBref of player making the change
 * @param tp        Config table entry describing the directive
 * @return CF_Result Result from the interpreter (success/partial/failure)
 */
CF_Result helper_cf_set(char *cp, char *ap, dbref player, CONF *tp)
{
    CF_Result result = CF_Failure;
    const char *status_msg = "Strange.";
    char *buff = NULL, *buf = NULL, *name = NULL;
    int interp_result = 0;

    /* Check permissions; deny if not standalone and not initializing */
    if (!mushstate.standalone && !mushstate.initializing && !check_access(player, tp->flags))
    {
        notify(player, NOPERM_MESSAGE);
        return CF_Failure;
    }

    /* Save arguments for logging (only at runtime) */
    if (!mushstate.initializing)
    {
        buff = XSTRDUP(ap, "buff");
    }

    /* Invoke the configuration handler */
    cf_interpreter = tp->interpreter;
    interp_result = cf_interpreter(tp->loc, ap, tp->extra, player, cp);

    /* Map interpreter result to status and return code; log at runtime */
    if (mushstate.initializing)
    {
        return interp_result;
    }

    /* Determine status message and result code based on interpreter output */
    switch (interp_result)
    {
    case 0:
        result = CF_Success;
        status_msg = "Success.";
        break;

    case 1:
        result = CF_Partial;
        status_msg = "Partial success.";
        break;

    case -1:
        result = CF_Failure;
        status_msg = "Failure.";
        break;

    default:
        result = CF_Failure;
        status_msg = "Strange.";
    }

    /* Log the directive execution with arguments and result */
    name = log_getname(player);
    buf = ansi_strip_ansi(buff);
    log_write(LOG_CONFIGMODS, "CFG", "UPDAT", "%s entered config directive: %s with args '%s'. Status: %s", name, cp, buf, status_msg);

    /* Cleanup */
    XFREE(buf);
    XFREE(name);
    XFREE(buff);

    return result;
}

/**
 * @brief Dispatch a configuration directive to the appropriate handler
 *
 * Looks up the directive in core and module tables, invokes its handler via
 * `helper_cf_set`, and logs not-found errors for runtime callers. Standalone
 * mode restricts processing to module-loading essentials.
 *
 * @param cp        Configuration directive name
 * @param ap        Argument string
 * @param player    DBref of player (for logging and permissions)
 * @return CF_Result Result from the handler or `CF_Failure` if missing
 */
CF_Result cf_set(char *cp, char *ap, dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    bool is_essential = false;

    /* Validate directive name */
    if (cp == NULL)
    {
        cf_log(player, "CNF", "SYNTX", (char *)"Set", "Missing configuration directive name");
        return CF_Failure;
    }

    /* Check if directive is essential for standalone mode (module loading, database paths) */
    is_essential = (!strcmp(cp, "module") || !strcmp(cp, "database_home"));

    /* In standalone mode, only allow essential directives */
    if (mushstate.standalone && !is_essential)
    {
        return CF_Success;
    }

    /* Search in core configuration table */
    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcmp(tp->pname, cp))
        {
            return helper_cf_set(cp, ap, player, tp);
        }
    }

    /* Search in module configuration tables */
    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcmp(tp->pname, cp))
                {
                    return helper_cf_set(cp, ap, player, tp);
                }
            }
        }
    }

    /* Config directive not found; log error if not in standalone mode */
    if (!mushstate.standalone)
    {
        cf_log(player, "CNF", "NFND", (char *)"Set", "%s %s not found", "Config directive", cp);
    }

    return CF_Failure;
}

/**
 * @brief Runtime command to set configuration parameters
 *
 * Wraps `cf_set` for the @admin command, validating the directive name and
 * returning a confirmation to the caller when the change executes successfully.
 * Respects the player's `Quiet` flag to suppress confirmation messages.
 *
 * @param player    DBref of player issuing the command
 * @param cause     DBref of the cause (unused)
 * @param extra     Unused command extra
 * @param kw        Directive keyword (required)
 * @param value     Argument string (may be empty or NULL)
 * @return void
 */
void do_admin(dbref player, dbref cause, int extra, char *kw, char *value)
{
    /* Validate directive name before invoking handler */
    if (kw == NULL)
    {
        notify(player, "Syntax: @admin <directive> <value>");
        return;
    }

    /* Invoke configuration handler; notify on success unless quiet */
    int result = cf_set(kw, value, player);

    if ((result >= 0) && !Quiet(player))
    {
        notify(player, "Set.");
    }
}

/**
 * @brief Convenience wrapper to read configuration from a file
 *
 * Loads and parses a configuration file during initialization. Validates input
 * and delegates to `cf_include` with standard parameters (no player context,
 * directive name "init").
 *
 * @param fn    Path to configuration file (required)
 * @return CF_Result Result from `cf_include`, or `CF_Failure` if fn is NULL
 *
 * @note Intended for initialization paths; respects `cf_include` startup guard.
 */
CF_Result cf_read(char *fn)
{
    /* Validate input */
    if (fn == NULL)
    {
        log_write(LOG_ALWAYS, "CNF", "ERROR", "cf_read: NULL filename provided");
        return CF_Failure;
    }

    /* Delegate to cf_include with standard initialization parameters */
    return cf_include(NULL, fn, 0, 0, "init");
}

/**
 * @brief List write access for all configuration directives
 *
 * Displays directives and their write permissions to the requesting player,
 * including module directives. Respects access checks per entry. Early-validates
 * the player to prevent notification spam on invalid dbrefs.
 *
 * @param player DBref of player requesting the list
 * @return void
 */
void list_cf_access(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    /* Validate player object early */
    if (!Good_obj(player))
    {
        return;
    }

    notify(player, "Attribute                      Permission");
    notify(player, "------------------------------ ------------------------------------------------");

    /* Display core configuration directives */
    for (tp = conftable; tp->pname; tp++)
    {
        if (God(player) || check_access(player, tp->flags))
        {
            listset_nametab(player, access_nametab, tp->flags, true, "%-30.30s ", tp->pname);
        }
    }

    /* Display module configuration directives */
    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
        if (ctab != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (God(player) || check_access(player, tp->flags))
                {
                    listset_nametab(player, access_nametab, tp->flags, true, "%-30.30s ", tp->pname);
                }
            }
        }
    }
    notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List read access for all configuration directives
 *
 * Displays directives and their read permissions to the requesting player,
 * including module directives. Respects access checks per entry. Early-validates
 * the player and allocates format buffer once for efficiency.
 *
 * @param player DBref of player requesting the list
 * @return void
 */
void list_cf_read_access(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *buff = NULL;

    /* Validate player object early */
    if (!Good_obj(player))
    {
        return;
    }

    notify(player, "Attribute                      Permission");
    notify(player, "------------------------------ ------------------------------------------------");

    /* Allocate format buffer once for all core entries */
    buff = XMALLOC(SBUF_SIZE, "buff");

    /* Display core configuration directives */
    for (tp = conftable; tp->pname; tp++)
    {
        if (God(player) || check_access(player, tp->rperms))
        {
            XSNPRINTF(buff, SBUF_SIZE, "%-30.30s ", tp->pname);
            listset_nametab(player, access_nametab, tp->rperms, true, buff);
        }
    }

    /* Display module configuration directives */
    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
        if (ctab != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (God(player) || check_access(player, tp->rperms))
                {
                    XSNPRINTF(buff, SBUF_SIZE, "%-30.30s ", tp->pname);
                    listset_nametab(player, access_nametab, tp->rperms, true, buff);
                }
            }
        }
    }

    /* Cleanup */
    XFREE(buff);
    notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Validate a configuration table's dbref entries
 *
 * Scans a configuration table for `cf_dbref` parameters, verifies referenced
 * objects still exist (or are permitted `NOTHING` defaults), and resets any
 * invalid values to their declared defaults while logging corrections.
 *
 * @param ctab   Configuration table to validate (core or module)
 * @return void
 *
 * @note Thread-safe: No; reads and writes global configuration storage.
 */
static void _cf_verify_table(CONF *ctab)
{
    CONF *tp = NULL;
    dbref current = NOTHING;
    dbref default_ref = NOTHING;
    bool valid_ref = false;

    if (ctab == NULL)
    {
        return;
    }

    for (tp = ctab; tp->pname; tp++)
    {
        if (tp->interpreter != cf_dbref)
        {
            continue;
        }

        if (tp->loc == NULL)
        {
            continue;
        }

        current = *(tp->loc);
        default_ref = (dbref)tp->extra;
        valid_ref = (((default_ref == NOTHING) && (current == NOTHING)) || (Good_obj(current) && !Going(current)));

        if (!valid_ref)
        {
            log_write(LOG_ALWAYS, "CNF", "VRFY", "%s #%d is invalid. Reset to #%d.", tp->pname, current, default_ref);
            *(tp->loc) = default_ref;
        }
    }
}

/**
 * @brief Validate dbref configuration values after loading
 *
 * Iterates through core and module configuration tables, ensuring each dbref
 * parameter references a valid object or permitted `NOTHING` value. Resets
 * invalid entries to their specified defaults and logs corrections.
 *
 * @return void
 */
void cf_verify(void)
{
    CONF *ctab = NULL;
    MODULE *mp = NULL;

    _cf_verify_table(conftable);

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
        _cf_verify_table(ctab);
    }
}

/**
 * @brief Format a configuration value into the provided buffer
 *
 * Renders the value for a specific configuration entry into `buff`, honoring
 * read permissions and formatting based on the interpreter type (bool, int,
 * string, dbref, option). Writes a no-permission marker when access is denied.
 *
 * @param player    DBref of player requesting the value
 * @param buff      Output buffer
 * @param bufc      Current buffer cursor pointer
 * @param tp        Configuration table entry to display
 * @return void
 */
void helper_cf_display(dbref player, char *buff, char **bufc, CONF *tp)
{
    NAMETAB *opt = NULL;

    /* Validate inputs early; fall back to no-permission marker on bad state */
    if ((tp == NULL) || (buff == NULL) || (bufc == NULL) || (tp->loc == NULL))
    {
        XSAFENOPERM(buff, bufc);
        return;
    }

    if (!check_access(player, tp->rperms))
    {
        XSAFENOPERM(buff, bufc);
        return;
    }

    if ((tp->interpreter == cf_bool) || (tp->interpreter == cf_int) || (tp->interpreter == cf_int_factor) || (tp->interpreter == cf_const))
    {
        XSAFELTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
        return;
    }

    if (tp->interpreter == cf_string)
    {
        XSAFELBSTR(*((char **)tp->loc), buff, bufc);
        return;
    }

    if (tp->interpreter == cf_dbref)
    {
        XSAFELBCHR('#', buff, bufc);
        XSAFELTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
        return;
    }

    if (tp->interpreter == cf_option)
    {
        opt = find_nametab_ent_flag(GOD, (NAMETAB *)tp->extra, *(tp->loc));
        XSAFELBSTR((opt ? opt->name : "*UNKNOWN*"), buff, bufc);
        return;
    }

    XSAFENOPERM(buff, bufc);
    return;
}

/**
 * @brief Display a configuration parameter by name
 *
 * Finds the named parameter in core or module tables and delegates formatting
 * to `helper_cf_display`. Emits a no-match marker when the parameter is not
 * found.
 *
 * @param player        DBref of player requesting the value
 * @param param_name    Name of the configuration parameter
 * @param buff          Output buffer
 * @param bufc          Current buffer cursor pointer
 * @return void
 */
void cf_display(dbref player, char *param_name, char *buff, char **bufc)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    /* Validate inputs early */
    if ((param_name == NULL) || (buff == NULL) || (bufc == NULL))
    {
        if ((buff != NULL) && (bufc != NULL))
        {
            XSAFENOMATCH(buff, bufc);
        }
        return;
    }

    if (*param_name == '\0')
    {
        XSAFENOMATCH(buff, bufc);
        return;
    }

    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcasecmp(tp->pname, param_name))
        {
            helper_cf_display(player, buff, bufc, tp);
            return;
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
        if (ctab == NULL)
        {
            continue;
        }

        for (tp = ctab; tp->pname; tp++)
        {
            if (!strcasecmp(tp->pname, param_name))
            {
                helper_cf_display(player, buff, bufc, tp);
                return;
            }
        }
    }

    XSAFENOMATCH(buff, bufc);
}

/**
 * @brief Emit one option entry in the options listing
 *
 * Renders a single boolean/constant configuration option for display, showing
 * the directive name, current value (Y/N), and any description stored in the
 * `extra` field. Intended for use by `list_options()` when iterating over
 * core and module configuration tables.
 *
 * @param player DBref of the player receiving the listing
 * @param tp     Configuration table entry to display
 */
static void _list_option_entry(dbref player, CONF *tp)
{
    char status = (*(tp->loc) ? 'Y' : 'N');
    char *desc = (tp->extra ? (char *)tp->extra : "");

    raw_notify(player, "%-25s %c %s?", tp->pname, status, desc);
}

/**
 * @brief List boolean/constant options available to a player
 *
 * Shows readable boolean and constant configuration options, grouping module
 * options under module headers. Respects per-entry read permissions.
 *
 * @param player DBref of player requesting the list
 * @return void
 */
void list_options(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    bool draw_header = false;
    bool is_option = false;

    /* Validate player early */
    if (!Good_obj(player))
    {
        return;
    }

    notify(player, "Global Options            S Description");
    notify(player, "------------------------- - ---------------------------------------------------");
    for (tp = conftable; tp->pname; tp++)
    {
        is_option = ((tp->interpreter == cf_const) || (tp->interpreter == cf_bool));
        if (is_option && check_access(player, tp->rperms))
        {
            _list_option_entry(player, tp);
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
        if (ctab == NULL)
        {
            continue;
        }

        draw_header = false;
        for (tp = ctab; tp->pname; tp++)
        {
            is_option = ((tp->interpreter == cf_const) || (tp->interpreter == cf_bool));
            if (is_option && check_access(player, tp->rperms))
            {
                if (!draw_header)
                {
                    raw_notify(player, "\nModule %-18.18s S Description", mp->modname);
                    notify(player, "------------------------- - ---------------------------------------------------");
                    draw_header = true;
                }
                _list_option_entry(player, tp);
            }
        }
    }
    notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief Open a shared library using a formatted path
 *
 * Formats the provided path template with variadic arguments and opens the
 * resulting shared library via `dlopen` using `RTLD_LAZY`. A small stack
 * buffer handles typical paths; a heap buffer is allocated only when the
 * formatted path exceeds the initial capacity. The caller owns the returned
 * handle and must release it with `dlclose` when no longer needed.
 *
 * @param filename Format string for the library path (required)
 * @param ...      Arguments corresponding to the format string
 * @return void*   Handle from `dlopen` on success; NULL on formatting errors,
 *                 allocation failure, or `dlopen` failure
 *
 * @note Thread-safe: Yes (no shared state beyond libc `dlopen`)
 * @attention Caller must pair the returned handle with `dlclose`.
 */
void *dlopen_format(const char *filename, ...)
{
    char stackbuf[256];
    char *buf = stackbuf;
    size_t size = sizeof(stackbuf);
    int n = 0;
    va_list ap;
    void *dlhandle = NULL;

    if (filename == NULL)
    {
        return NULL;
    }

    va_start(ap, filename);
    n = vsnprintf(buf, size, filename, ap);
    va_end(ap);

    if (n < 0)
    {
        return NULL;
    }

    if ((size_t)n >= size)
    {
        size = (size_t)n + 1;
        buf = malloc(size);
        if (buf == NULL)
        {
            return NULL;
        }

        va_start(ap, filename);
        n = vsnprintf(buf, size, filename, ap);
        va_end(ap);

        if ((n < 0) || ((size_t)n >= size))
        {
            free(buf);
            return NULL;
        }
    }

    dlhandle = dlopen(buf, RTLD_LAZY);

    if (buf != stackbuf)
    {
        free(buf);
    }

    return dlhandle;
}

/**
 * @brief Resolve a symbol name using formatted input
 *
 * Formats the symbol name with variadic arguments, then resolves it from the
 * provided library handle using `dlsym`. Uses a stack buffer for common symbol
 * names and allocates dynamically only when the formatted name exceeds the
 * initial buffer size. Returns NULL on formatting errors, allocation failure,
 * or when `dlsym` cannot find the symbol.
 *
 * @param place   Handle returned by `dlopen` (required)
 * @param symbol  Format string for the symbol name (required)
 * @param ...     Arguments corresponding to the format string
 * @return void*  Address of the resolved symbol, or NULL on error
 *
 * @note Thread-safe: Yes (no shared state beyond libc `dlsym`)
 * @attention The caller must ensure `place` stays valid for the lifetime of
 *            the returned symbol pointer.
 */
void *dlsym_format(void *place, const char *symbol, ...)
{
    char stackbuf[256];
    char *buf = stackbuf;
    size_t size = sizeof(stackbuf);
    int n = 0;
    va_list ap;
    void *_dlsym = NULL;

    if ((place == NULL) || (symbol == NULL))
    {
        return NULL;
    }

    va_start(ap, symbol);
    n = vsnprintf(buf, size, symbol, ap);
    va_end(ap);

    if (n < 0)
    {
        return NULL;
    }

    if ((size_t)n >= size)
    {
        size = (size_t)n + 1;
        buf = malloc(size);
        if (buf == NULL)
        {
            return NULL;
        }

        va_start(ap, symbol);
        n = vsnprintf(buf, size, symbol, ap);
        va_end(ap);

        if ((n < 0) || ((size_t)n >= size))
        {
            free(buf);
            return NULL;
        }
    }

    _dlsym = dlsym(place, buf);

    if (buf != stackbuf)
    {
        free(buf);
    }

    return _dlsym;
}