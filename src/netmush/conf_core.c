/**
 * @file conf_core.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core configuration initialization and logging
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
#include <stdarg.h>

/* Global configuration and state data */
CONFDATA mushconf;
STATEDATA mushstate;

/* Global configuration interpreter function pointer */
int (*cf_interpreter)(int *, char *, long, dbref, char *);

/**
 * @brief Initialize mushconf to default values.
 *
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
    /**
     * We can make theses NULL because we are going to define
     * default values later if they are still NULL.
     *
     */
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
    mushconf.max_global_regs = DEFAULT_GLOBAL_REGS;
    mushconf.max_command_args = 100;
    mushconf.player_name_length = 22;
    mushconf.hash_factor = 2;
    mushconf.max_qpid = 10000;
    mushconf.timeslice = 1000;
    mushconf.cmd_quota_max = 100;
    mushconf.cmd_quota_incr = 1;
    mushconf.cmd_invk_lim = 2500;
    mushconf.cmd_nest_lim = 50;
    mushconf.func_invk_lim = 2500;
    mushconf.func_nest_lim = 50;
    mushconf.func_cpu_lim_secs = 60;
    mushconf.func_cpu_lim = 60 * CLOCKS_PER_SEC;
    mushconf.ntfy_nest_lim = 20;
    mushconf.lock_nest_lim = 20;
    mushconf.parent_nest_lim = 10;
    mushconf.zone_nest_lim = 20;
    mushconf.stack_lim = 50;
    mushconf.struct_lim = 100;
    mushconf.instance_lim = 100;
    mushconf.numvars_lim = 50;
    mushconf.struct_dstr = XSTRDUP("\r\n", "mushconf.struct_dstr");
    mushconf.wild_times_lim = 25000;
    mushconf.max_cmdsecs = 120;
    mushconf.max_grid_size = 1000;
    mushconf.max_player_aliases = 10;
    mushconf.propdir_lim = 10;
    mushconf.fwdlist_lim = 100;
    mushconf.parse_stack_limit = 64;
    mushconf.lag_check = 1;
    mushconf.lag_check_clk = 1;
    mushconf.lag_check_cpu = 1;
    mushconf.malloc_logger = 0;
    mushconf.cache_size = CACHE_SIZE;
    mushconf.cache_width = CACHE_WIDTH;
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
    mushconf.many_coins = XSTRDUP("pennies", "mushconf.many_coins");
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
}

/**
 * @brief Log or notify messages based on initialization state.
 *
 * @param player        DBref of player
 * @param primary       Primary log category
 * @param secondary     Secondary log category
 * @param cmd           Command name
 * @param format        Printf-style format string
 * @param ...           Variable arguments
 */
void cf_log(dbref player, const char *primary, const char *secondary, char *cmd, const char *format, ...)
{
    va_list ap;
    char *buff = XMALLOC(LBUF_SIZE, "buff");

    va_start(ap, format);
    XVSNPRINTF(buff, LBUF_SIZE, format, ap);
    va_end(ap);

    if (mushstate.initializing)
    {
        log_write(LOG_STARTUP, primary, secondary, "%s: %s", cmd, buff);
    }
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: %s", cmd, buff);
    }

    XFREE(buff);
}

/**
 * @brief Return command status from succ and fail info
 *
 * @param player    DBref of player
 * @param cmd       Command
 * @param success   Did it succeed?
 * @param failure   Did it fail?
 * @return CF_Result
 */
CF_Result cf_status_from_succfail(dbref player, char *cmd, int success, int failure)
{
    /**
     * If any successes, return SUCCESS(0) if no failures or
     * PARTIAL_SUCCESS(1) if any failures.
     *
     */
    if (success > 0)
    {
        return ((failure == 0) ? CF_Success : CF_Partial);
    }

    /**
     * No successes. If no failures indicate nothing done. Always return
     * FAILURE(-1)
     */

    if (failure == 0)
    {
        if (mushstate.initializing)
        {
            log_write(LOG_STARTUP, "CNF", "NDATA", "%s: Nothing to set", cmd);
        }
        else
        {
            notify(player, "Nothing to set");
        }
    }

    return CF_Failure;
}
