/**
 * @file conf.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration functions and defaults
 * @version 3.3
 * @date 2020-12-25
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

CONFDATA mushconf;
STATEDATA mushstate;

/**
 * @brief Initialize mushconf to default values.
 * 
 */
void cf_init(void)
{
    mushstate.modules_list = NULL;
    mushstate.modloaded = XMALLOC(MBUF_SIZE, "mushstate.modloaded");
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
    mushconf.fork_vfork = 0;
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
 * @brief Log a configuration error
 * 
 * @param player    DBref of player
 * @param primary   Primary error type
 * @param secondary Secondary error type
 * @param cmd       Command
 * @param format    Format string
 * @param ...       Variable parameter for format
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
 * @param success   Dit it success?
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

/**
 * @brief Read-only integer or boolean parameter.
 * 
 * @param vp        Not used
 * @param str       Not used 
 * @param extra     Not used 
 * @param player    Dbref of the player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_const(int *vp __attribute__((unused)), char *str __attribute__((unused)), long extra __attribute__((unused)), dbref player, char *cmd)
{
    /**
     * Fail on any attempt to change the value
     * 
     */
    cf_log(player, "CNF", "SYNTX", cmd, "Cannot change a constant value");
    return CF_Failure;
}

/**
 * @brief Set integer parameter.
 * 
 * @param vp        Numeric value
 * @param str       String buffer
 * @param extra     Maximum limit
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_int(int *vp, char *str, long extra, dbref player, char *cmd)
{
    /**
     * Copy the numeric value to the parameter
     * 
     */
    if ((extra > 0) && ((int)strtol(str, (char **)NULL, 10) > extra))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %d", extra);
        return CF_Failure;
    }

    sscanf(str, "%d", vp);
    return CF_Success;
}

/**
 * @brief Set integer parameter that will be used as a factor (ie. cannot be set to 0)
 * 
 * @param vp        Numeric value
 * @param str       String buffer
 * @param extra     Maximum limit
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_int_factor(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int num = (int)strtol(str, (char **)NULL, 10);

    if ((extra > 0) && (num > extra))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %d", extra);
        return CF_Failure;
    }

    if (num == 0)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value cannot be 0. You may want a value of 1.");
        return CF_Failure;
    }

    sscanf(str, "%d", vp);
    return CF_Success;
}

/**
 * @brief Set dbref parameter.
 * 
 * @param vp        Numeric value
 * @param str       String buffer
 * @param extra     Maximum limit
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_dbref(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int num = 0;

    /**
     * No consistency check on initialization.
     * 
     */
    if (mushstate.initializing)
    {
        if (*str == '#')
        {
            sscanf(str, "#%d", vp);
        }
        else
        {
            sscanf(str, "%d", vp);
        }

        return CF_Success;
    }

    /**
     * Otherwise we have to validate this. If 'extra' is non-zero, the
     * dbref is allowed to be NOTHING.
     * 
     */
    if (*str == '#')
    {
        num = (int)strtol(str + 1, (char **)NULL, 10);
    }
    else
    {
        num = (int)strtol(str, (char **)NULL, 10);
    }

    if (((extra == NOTHING) && (num == NOTHING)) || (Good_obj(num) && !Going(num)))
    {
        if (*str == '#')
        {
            sscanf(str, "#%d", vp);
        }
        else
        {
            sscanf(str, "%d", vp);
        }

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

/**
 * @brief Open a loadable module. Modules are initialized later in the startup.
 * 
 * @param vp 
 * @param modname 
 * @param extra 
 * @param player 
 * @param cmd 
 * @return int 
 */
CF_Result cf_module(int *vp __attribute__((unused)), char *modname, long extra __attribute__((unused)), dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    lt_dlhandle handle;
    void (*initptr)(void) = NULL;
    MODULE *mp = NULL;

    handle = lt_dlopen_format("%s/%s.la", mushconf.modules_home, modname);

    if (!handle)
    {
        log_write(LOG_STARTUP, "CNF", "MOD", "Loading of %s module failed: %s", modname, lt_dlerror());
        return CF_Failure;
    }

    mp = (MODULE *)XMALLOC(sizeof(MODULE), "mp");
    mp->modname = XSTRDUP(modname, "mp->modname");
    mp->handle = handle;
    mp->next = mushstate.modules_list;
    mushstate.modules_list = mp;

    /**
     * Look up our symbols now, and cache the pointers. They're not going
     * to change from here on out.
     * 
     */
    mp->process_command = (int (*)(dbref, dbref, int, char *, char *[], int))lt_dlsym_format(handle, "mod_%s_%s", modname, "process_command");
    mp->process_no_match = (int (*)(dbref, dbref, int, char *, char *, char *[], int))lt_dlsym_format(handle, "mod_%s_%s", modname, "process_no_match");
    mp->did_it = (int (*)(dbref, dbref, dbref, int, const char *, int, const char *, int, int, char *[], int, int))lt_dlsym_format(handle, "mod_%s_%s", modname, "did_it");
    mp->create_obj = (void (*)(dbref, dbref))lt_dlsym_format(handle, "mod_%s_%s", modname, "create_obj");
    mp->destroy_obj = (void (*)(dbref, dbref))lt_dlsym_format(handle, "mod_%s_%s", modname, "destroy_obj");
    mp->create_player = (void (*)(dbref, dbref, int, int))lt_dlsym_format(handle, "mod_%s_%s", modname, "create_player");
    mp->destroy_player = (void (*)(dbref, dbref))lt_dlsym_format(handle, "mod_%s_%s", modname, "destroy_player");
    mp->announce_connect = (void (*)(dbref, const char *, int))lt_dlsym_format(handle, "mod_%s_%s", modname, "announce_connect");
    mp->announce_disconnect = (void (*)(dbref, const char *, int))lt_dlsym_format(handle, "mod_%s_%s", modname, "announce_disconnect");
    mp->examine = (void (*)(dbref, dbref, dbref, int, int))lt_dlsym_format(handle, "mod_%s_%s", modname, "examine");
    mp->dump_database = (void (*)(FILE *))lt_dlsym_format(handle, "mod_%s_%s", modname, "dump_database");
    mp->db_grow = (void (*)(int, int))lt_dlsym_format(handle, "mod_%s_%s", modname, "db_grow");
    mp->db_write = (void (*)(void))lt_dlsym_format(handle, "mod_%s_%s", modname, "db_write");
    mp->db_write_flatfile = (void (*)(FILE *))lt_dlsym_format(handle, "mod_%s_%s", modname, "db_write_flatfile");
    mp->do_second = (void (*)(void))lt_dlsym_format(handle, "mod_%s_%s", modname, "do_second");
    mp->cache_put_notify = (void (*)(UDB_DATA, unsigned int))lt_dlsym_format(handle, "mod_%s_%s", modname, "cache_put_notify");
    mp->cache_del_notify = (void (*)(UDB_DATA, unsigned int))lt_dlsym_format(handle, "mod_%s_%s", modname, "cache_del_notify");

    if (!mushstate.standalone)
    {
        if ((initptr = (void (*)(void))lt_dlsym_format(handle, "mod_%s_%s", modname, "init")) != NULL)
        {
            (*initptr)();
        }
    }

    log_write(LOG_STARTUP, "CNF", "MOD", "Loaded module: %s", modname);
    return CF_Success;
}

/**
 * @brief Set boolean parameter.
 * 
 * @param vp        Boolean value
 * @param str       String buffer
 * @param extra     Not used
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_bool(int *vp, char *str, long extra __attribute__((unused)), dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    *vp = (int)search_nametab(GOD, bool_names, str);

    if (*vp < 0)
    {
        *vp = (long)0;
    }

    return CF_Success;
}

/**
 * @brief Select one option from many choices.
 * 
 * @param vp        Index of the option
 * @param str       String buffer
 * @param extra     Name Table
 * @param player    DBref of the player
 * @param cmd       Command
 * @return int      -1 on Failure, 0 on Success
 */
CF_Result cf_option(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int i = search_nametab(GOD, (NAMETAB *)extra, str);

    if (i < 0)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Value", str);
        return CF_Failure;
    }

    *vp = i;
    return CF_Success;
}

/**
 * @brief Set string parameter.
 * 
 * @param vp        Variable type
 * @param str       String buffer
 * @param extra     Maximum string length
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_string(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int retval = CF_Success;
    /**
     * Make a copy of the string if it is not too big
     * 
     */

    if (strlen(str) >= (unsigned int)extra)
    {
        str[extra - 1] = '\0';

        if (mushstate.initializing)
        {
            log_write(LOG_STARTUP, "CNF", "NFND", "%s: String truncated", cmd);
        }
        else
        {
            notify(player, "String truncated");
        }

        retval = CF_Failure;
    }

    XFREE(*(char **)vp);
    *(char **)vp = XSTRDUP(str, "vp");
    return retval;
}

/**
 * @brief define a generic hash table alias.
 * 
 * @param vp 
 * @param str 
 * @param extra 
 * @param player 
 * @param cmd 
 * @return CF_Result 
 */
CF_Result cf_alias(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int *cp = NULL, upcase = 0;
    char *p = NULL, *tokst = NULL;
    char *alias = strtok_r(str, " \t=,", &tokst);
    char *orig = strtok_r(NULL, " \t=,", &tokst);

    if (orig)
    {
        upcase = 0;

        for (p = orig; *p; p++)
        {
            *p = tolower(*p);
        }

        cp = hashfind(orig, (HASHTAB *)vp);

        if (cp == NULL)
        {
            upcase++;

            for (p = orig; *p; p++)
            {
                *p = toupper(*p);
            }

            cp = hashfind(orig, (HASHTAB *)vp);

            if (cp == NULL)
            {
                cf_log(player, "CNF", "NFND", cmd, "%s %s not found", (char *)extra, str);
                return CF_Failure;
            }
        }

        if (upcase)
        {
            for (p = alias; *p; p++)
            {
                *p = toupper(*p);
            }
        }
        else
        {
            for (p = alias; *p; p++)
            {
                *p = tolower(*p);
            }
        }

        if (((HASHTAB *)vp)->flags & HT_KEYREF)
        {
            /**
             * hashadd won't copy it, so we do that here
             * 
             */
            p = alias;
            alias = XSTRDUP(p, "alias");
        }

        return hashadd(alias, cp, (HASHTAB *)vp, HASH_ALIAS);
    }
    else
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid original for alias %s", alias);
        return CF_Failure;
    }
}

/**
 * @brief Add an arbitrary field to INFO output.
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Not used
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_infotext(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    LINKEDLIST *itp = NULL, *prev = NULL;
    char *fvalue = NULL, *tokst = NULL;
    char *fname = strtok_r(str, " \t=,", &tokst);

    if (tokst)
    {
        for (fvalue = tokst; *fvalue && ((*fvalue == ' ') || (*fvalue == '\t')); fvalue++)
        {
            /**
             * Empty loop
             * 
             */
        }
    }
    else
    {
        fvalue = NULL;
    }

    if (!fvalue || !*fvalue)
    {
        for (itp = mushconf.infotext_list, prev = NULL; itp != NULL; itp = itp->next)
        {
            if (!strcasecmp(fname, itp->name))
            {
                XFREE(itp->name);
                XFREE(itp->value);

                if (prev)
                {
                    prev->next = itp->next;
                }
                else
                {
                    mushconf.infotext_list = itp->next;
                }

                XFREE(itp);
                return CF_Partial;
            }
            else
            {
                prev = itp;
            }
        }

        return CF_Partial;
    }

    /**
     * Otherwise we're setting. Replace if we had a previous value.
     * 
     */
    for (itp = mushconf.infotext_list; itp != NULL; itp = itp->next)
    {
        if (!strcasecmp(fname, itp->name))
        {
            XFREE(itp->value);
            itp->value = XSTRDUP(fvalue, "itp->value");
            return CF_Partial;
        }
    }

    /**
     * No previous value. Add a node.
     * 
     */
    itp = (LINKEDLIST *)XMALLOC(sizeof(LINKEDLIST), "itp");
    itp->name = XSTRDUP(fname, "itp->name");
    itp->value = XSTRDUP(fvalue, "itp->value");
    itp->next = mushconf.infotext_list;
    mushconf.infotext_list = itp;
    return CF_Partial;
}

/**
 * @brief Redirect a log type.
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_divert_log(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *type_str = NULL, *file_str = NULL, *tokst = NULL;
    int f = 0, fd = 0;
    FILE *fptr = NULL;
    LOGFILETAB *tp = NULL, *lp = NULL;

    /**
     * Two args, two args only
     * 
     */
    type_str = strtok_r(str, " \t", &tokst);
    file_str = strtok_r(NULL, " \t", &tokst);

    if (!type_str || !file_str)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Missing pathname to log to.");
        return CF_Failure;
    }

    /**
     * Find the log.
     * 
     */
    f = search_nametab(GOD, (NAMETAB *)extra, type_str);

    if (f <= 0)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Log diversion", str);
        return CF_Failure;
    }

    for (tp = logfds_table; tp->log_flag; tp++)
    {
        if (tp->log_flag == f)
        {
            break;
        }
    }

    if (tp == NULL)
    {
        /**
         * This should never happen!
         * 
         */
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Logfile table corruption", str);
        return CF_Failure;
    }

    /**
     * We shouldn't have a file open already.
     * 
     */
    if (tp->filename != NULL)
    {
        log_write(LOG_STARTUP, "CNF", "DIVT", "Log type %s already diverted: %s", type_str, tp->filename);
        return CF_Failure;
    }

    /**
     * Check to make sure that we don't have this filename open already.
     * 
     */
    fptr = NULL;

    for (lp = logfds_table; lp->log_flag; lp++)
    {
        if (lp->filename && !strcmp(file_str, lp->filename))
        {
            fptr = lp->fileptr;
            break;
        }
    }

    /**
     * We don't have this filename yet. Open the logfile.
     * 
     */
    if (!fptr)
    {
        fptr = fopen(file_str, "w");

        if (!fptr)
        {
            log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot open logfile: %s", file_str);
            return CF_Failure;
        }

        if ((fd = fileno(fptr)) == -1)
        {
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
            return -1;
        }
#endif
    }

    /**
     * Indicate that this is being diverted.
     * 
     */
    tp->fileptr = fptr;
    tp->filename = XSTRDUP(file_str, "tp->filename");
    *vp |= f;
    return CF_Success;
}

/**
 * @brief set or clear bits in a flag word from a namelist.
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_modify_bits(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL;
    int f = 0, negate = 0, success = 0, failure = 0;
    /**
     * Walk through the tokens
     * 
     */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);

    while (sp != NULL)
    {
        /**
    	 * Check for negation
         * 
    	 */
        negate = 0;

        if (*sp == '!')
        {
            negate = 1;
            sp++;
        }

        /**
    	 * Set or clear the appropriate bit
         * 
    	 */
        f = search_nametab(GOD, (NAMETAB *)extra, sp);

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
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
            failure++;
        }

        /**
    	 * Get the next token
         * 
    	 */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    return cf_status_from_succfail(player, cmd, success, failure);
}

/**
 * @brief Helper function to change xfuncs.
 * 
 * @param fn_name   Function name
 * @param fn_ptr    Function pointer
 * @param xfuncs    External functions
 * @param negate    true: remove, false add:
 * @return bool 
 */
bool modify_xfuncs(char *fn_name, int (*fn_ptr)(dbref), EXTFUNCS **xfuncs, bool negate)
{
    NAMEDFUNC *np = NULL, **tp = NULL;
    int i = 0;
    EXTFUNCS *xfp = *xfuncs;

    /**
     * If we're negating, just remove it from the list of functions.
     * 
     */
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

    /**
     * Have we encountered this function before?
     * 
     */
    np = NULL;

    for (i = 0; i < xfunctions.count; i++)
    {
        if (!strcmp(xfunctions.func[i]->fn_name, fn_name))
        {
            np = xfunctions.func[i];
            break;
        }
    }

    /**
     * If not, we need to allocate it.
     * 
     */
    if (!np)
    {
        np = (NAMEDFUNC *)XMALLOC(sizeof(NAMEDFUNC), "np");
        np->fn_name = (char *)XSTRDUP(fn_name, "np->fn_name");
        np->handler = fn_ptr;
    }

    /**
     * Add it to the ones we know about.
     * 
     */
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

    /**
     * Do we have an existing list of functions? If not, this is easy.
     * 
     */
    if (!xfp)
    {
        xfp = (EXTFUNCS *)XMALLOC(sizeof(EXTFUNCS), "xfp");
        xfp->num_funcs = 1;
        xfp->ext_funcs = (NAMEDFUNC **)XMALLOC(sizeof(NAMEDFUNC *), "xfp->ext_funcs");
        xfp->ext_funcs[0] = np;
        *xfuncs = xfp;
        return true;
    }

    /**
     * See if we have an empty slot to insert into.
     * 
     */
    for (i = 0; i < xfp->num_funcs; i++)
    {
        if (!xfp->ext_funcs[i])
        {
            xfp->ext_funcs[i] = np;
            return true;
        }
    }

    /**
     * Guess not. Tack it onto the end.
     * 
     */
    tp = (NAMEDFUNC **)XREALLOC(xfp->ext_funcs, (xfp->num_funcs + 1) * sizeof(NAMEDFUNC *), "tp");
    tp[xfp->num_funcs] = np;
    xfp->ext_funcs = tp;
    xfp->num_funcs++;
    return true;
}

/**
 * @brief Parse an extended access list with module callouts.
 * 
 * @param perms     Permissions
 * @param xperms    Extendes permissions
 * @param str       String buffer
 * @param ntab      Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result parse_ext_access(int *perms, EXTFUNCS **xperms, char *str, NAMETAB *ntab, dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL, *cp = NULL, *ostr = NULL, *s = NULL;
    int f = 0, negate = 0, success = 0, failure = 0, got_one = 0;
    MODULE *mp  = NULL;
    int (*hp)(dbref) = NULL;

    /**
     * Walk through the tokens
     * 
     */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);

    while (sp != NULL)
    {
        /**
         * Check for negation
         * 
         */
        negate = 0;

        if (*sp == '!')
        {
            negate = 1;
            sp++;
        }

        /**
         * Set or clear the appropriate bit
         * 
         */
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
            /**
             * Is this a module callout?
             * 
             */
            got_one = 0;

            if (!strncmp(sp, "mod_", 4))
            {
                /**
                 * Split it apart, see if we have anything.
                 * 
                 */
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
                                hp = (int (*)(dbref))lt_dlsym(mp->handle, s);

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

        /**
         * Get the next token
         * 
         */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    return cf_status_from_succfail(player, cmd, success, failure);
}

/**
 * @brief Clear flag word and then set from a flags htab.
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_set_flags(int *vp, char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL;
    FLAGENT *fp = NULL;
    FLAGSET *fset = NULL;
    int success = 0, failure = 0;

    for (sp = str; *sp; sp++)
    {
        *sp = toupper(*sp);
    }

    /**
     * Walk through the tokens
     * 
     */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);
    fset = (FLAGSET *)vp;

    while (sp != NULL)
    {
        /**
         * Set the appropriate bit
         * 
         */
        fp = (FLAGENT *)hashfind(sp, &mushstate.flags_htab);

        if (fp != NULL)
        {
            if (success == 0)
            {
                (*fset).word1 = 0;
                (*fset).word2 = 0;
                (*fset).word3 = 0;
            }

            if (fp->flagflag & FLAG_WORD3)
            {
                (*fset).word3 |= fp->flagvalue;
            }
            else if (fp->flagflag & FLAG_WORD2)
            {
                (*fset).word2 |= fp->flagvalue;
            }
            else
            {
                (*fset).word1 |= fp->flagvalue;
            }

            success++;
        }
        else
        {
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
            failure++;
        }

        /**
         * Get the next token
         * 
         */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    if ((success == 0) && (failure == 0))
    {
        (*fset).word1 = 0;
        (*fset).word2 = 0;
        (*fset).word3 = 0;
        return CF_Success;
    }

    if (success > 0)
    {
        return ((failure == 0) ? CF_Success : CF_Partial);
    }

    return CF_Failure;
}

/**
 * @brief Disallow use of player name/alias.
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_badname(int *vp __attribute__((unused)), char *str, long extra, dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    if (extra)
    {
        badname_remove(str);
    }
    else
    {
        badname_add(str);
    }

    return CF_Success;
}

/**
 * @brief Replacement for inet_addr()
 * 
 * inet_addr() does not necessarily do reasonable checking for sane syntax.
 * On certain operating systems, if passed less than four octets, it will
 * cause a segmentation violation. This is unfriendly. We take steps here
 * to deal with it.
 * 
 * @param str       IP Address
 * @return in_addr_t
 */
in_addr_t sane_inet_addr(char *str)
{
    int i = 0;
    char *p = str;

    for (i = 1; (p = strchr(p, '.')) != NULL; i++, p++)
        ;

    if (i < 4)
    {
        return INADDR_NONE;
    }
    else
    {
        return inet_addr(str);
    }
}

/**
 * @brief Update site information
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_site(long **vp, char *str, long extra, dbref player, char *cmd)
{
    SITE *site = NULL, *last = NULL, *head = NULL;
    char *addr_txt = NULL, *mask_txt = NULL, *tokst = NULL;
    struct in_addr addr_num, mask_num;
    int mask_bits = 0;

    if ((mask_txt = strchr(str, '/')) == NULL)
    {
        /**
         * Standard IP range and netmask notation.
         * 
         */
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

        if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
            return CF_Failure;
        }

        if (((mask_num.s_addr = sane_inet_addr(mask_txt)) == INADDR_NONE) && strcmp(mask_txt, "255.255.255.255"))
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed mask address: %s", mask_txt);
            return CF_Failure;
        }
    }
    else
    {
        /**
         * RFC 1517, 1518, 1519, 1520: CIDR IP prefix notation
         * 
         */
        addr_txt = str;
        *mask_txt++ = '\0';
        mask_bits = (int)strtol(mask_txt, (char **)NULL, 10);

        if ((mask_bits > 32) || (mask_bits < 0))
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Mask bits (%d) in CIDR IP prefix out of range.", mask_bits);
            return CF_Failure;
        }
        else if (mask_bits == 0)
        {  
            /** 
             * can't shift by 32 
             * 
             */
            mask_num.s_addr = htonl(0);
        }
        else
        {
            mask_num.s_addr = htonl(0xFFFFFFFFU << (32 - mask_bits));
        }

        if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
            return CF_Failure;
        }
    }

    head = (SITE *)*vp;

    /**
     * Parse the access entry and allocate space for it
     * 
     */
    if (!(site = (SITE *)XMALLOC(sizeof(SITE), "site")))
    {
        return CF_Failure;
    }

    /**
     * Initialize the site entry
     * 
     */
    site->address.s_addr = addr_num.s_addr;
    site->mask.s_addr = mask_num.s_addr;
    site->flag = (long)extra;
    site->next = NULL;

    /**
     * Link in the entry. Link it at the start if not initializing, at
     * the end if initializing. This is so that entries in the config 
     * file are processed as you would think they would be, while entries
     * made while running are processed first.
     * 
     */
    if (mushstate.initializing)
    {
        if (head == NULL)
        {
            *vp = (long *)site;
        }
        else
        {
            for (last = head; last->next; last = last->next)
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
 * @brief Set write or read access on config directives kludge
 * 
 * this cf handler uses vp as an extra extra field since the first extra field
 * is taken up with the access nametab.
 * 
 * @param tp        Config Parameter
 * @param player    DBrief of player
 * @param vp        Extra field
 * @param ap        String buffer
 * @param cmd       Command
 * @param extra     Extra field
 * @return CF_Result 
 */
CF_Result helper_cf_cf_access(CONF *tp, dbref player, int *vp, char *ap, char *cmd, long extra)
{
    /**
     * Cannot modify parameters set STATIC
     * 
     */
    char *name  = NULL;

    if (tp->flags & CA_STATIC)
    {
        notify(player, NOPERM_MESSAGE);

        if (db)
        {
            name = log_getname(player);
            log_write(LOG_CONFIGMODS, "CFG", "PERM", "%s tried to change %s access to static param: %s", name, (((long)vp) ? "read" : "write"), tp->pname);
            XFREE(name);
        }
        else
        {
            log_write(LOG_CONFIGMODS, "CFG", "PERM", "System tried to change %s access to static param: %s", (((long)vp) ? "read" : "write"), tp->pname);
        }

        return CF_Failure;
    }

    if ((long)vp)
    {
        return (cf_modify_bits(&tp->rperms, ap, extra, player, cmd));
    }
    else
    {
        return (cf_modify_bits(&tp->flags, ap, extra, player, cmd));
    }
}

/**
 * @brief Set configuration parameter access
 * 
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_cf_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *ap = NULL;

    for (ap = str; *ap && !isspace(*ap); ap++)
        ;

    if (*ap)
    {
        *ap++ = '\0';
    }

    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcmp(tp->pname, str))
        {
            return (helper_cf_cf_access(tp, player, vp, ap, cmd, extra));
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcmp(tp->pname, str))
                {
                    return (helper_cf_cf_access(tp, player, vp, ap, cmd, extra));
                }
            }
        }
    }

    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config directive", str);
    return CF_Failure;
}

/**
 * @brief Add a help/news-style file. Only valid during startup.
 * 
 * @param player        Dbref of player
 * @param confcmd       Command
 * @param str           Filename
 * @param is_raw        Raw textfile?
 * @return CF_Result 
 */
CF_Result add_helpfile(dbref player, char *confcmd, char *str, bool is_raw)
{
    
    CMDENT *cmdp = NULL;
    HASHTAB *hashes = NULL;
    FILE *fp = NULL;
    char *fcmd = NULL, *fpath = NULL, *newstr = NULL, *tokst = NULL;
    char **ftab = NULL;
    char *s = XMALLOC(MAXPATHLEN, "s");

    /**
     * Make a new string so we won't SEGV if given a constant string
     * 
     */
    newstr = XMALLOC(MBUF_SIZE, "newstr");
    XSTRCPY(newstr, str);
    fcmd = strtok_r(newstr, " \t=,", &tokst);
    fpath = strtok_r(NULL, " \t=,", &tokst);
    cf_log(player, "HLP", "LOAD", confcmd, "Loading helpfile %s", basename(fpath));

    if (fpath == NULL)
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Missing path for helpfile %s", fcmd);
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

    if (fcmd[0] == '_' && fcmd[1] == '_')
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s would cause @addcommand conflict", fcmd);
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

    /**
     * Check if file exists in given and standard path
     * 
     */
    XSNPRINTF(s, MAXPATHLEN, "%s.txt", fpath);
    fp = fopen(s, "r");

    if (fp == NULL)
    {
        fpath = XASPRINTF("fpath", "%s/%s", mushconf.txthome, fpath);
        XSNPRINTF(s, MAXPATHLEN, "%s.txt", fpath);
        fp = fopen(s, "r");

        if (fp == NULL)
        {
            cf_log(player, "HLP", "LOAD", confcmd, "Helpfile %s not found", fcmd);
            XFREE(newstr);
            XFREE(s);
            return -1;
        }
    }

    fclose(fp);

    /**
     * Rebuild Index
     * 
     */
    if (helpmkindx(player, confcmd, fpath))
    {
        cf_log(player, "HLP", "LOAD", confcmd, "Could not create index for helpfile %s, not loaded.", basename(fpath));
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

    if (strlen(fpath) > SBUF_SIZE)
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s filename too long", fcmd);
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

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

    hashdelete(cmdp->cmdname, &mushstate.command_htab);
    hashadd(cmdp->cmdname, (int *)cmdp, &mushstate.command_htab, 0);
    XSNPRINTF(s, MAXPATHLEN, "__%s", cmdp->cmdname);
    hashdelete(s, &mushstate.command_htab);
    hashadd(s, (int *)cmdp, &mushstate.command_htab, HASH_ALIAS);

    /**
     * We may need to grow the helpfiles table, or create it.
     * 
     */
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

    /**
     * Add or replace the path to the file.
     * 
     */
    if (mushstate.hfiletab[mushstate.helpfiles] != NULL)
    {
        XFREE(mushstate.hfiletab[mushstate.helpfiles]);
    }

    mushstate.hfiletab[mushstate.helpfiles] = XSTRDUP(fpath, "mushstate.hfiletab[mushstate.helpfiles]");

    /**
     * Initialize the associated hashtable.
     * 
     */
    hashinit(&mushstate.hfile_hashes[mushstate.helpfiles], 30 * mushconf.hash_factor, HT_STR);
    mushstate.helpfiles++;
    cf_log(player, "HLP", "LOAD", confcmd, "Successfully loaded helpfile %s", basename(fpath));
    XFREE(s);
    XFREE(newstr);
    return 0;
}

/**
 * @brief Add an helpfile
 * 
 * @param player        Dbref of player
 * @param confcmd       Command
 * @param str           Filename
 * @param is_raw        Raw textfile?
 * @return CF_Result 
 */
CF_Result cf_helpfile(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    return add_helpfile(player, cmd, str, 0);
}

/**
 * @brief Add a raw helpfile
 * 
 * @param player        Dbref of player
 * @param confcmd       Command
 * @param str           Filename
 * @param is_raw        Raw textfile?
 * @return CF_Result 
 */
CF_Result cf_raw_helpfile(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    return add_helpfile(player, cmd, str, 1);
}

/**
 * @brief Read another config file. Only valid during startup.
 * 
 * @param vp        Variable buffer
 * @param str       String buffer
 * @param extra     Extra data
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result 
 */
CF_Result cf_include(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    FILE *fp = NULL;
    char *cp = NULL, *ap = NULL, *zp = NULL, *buf = NULL;
    int line = 0;

    /**
     * @todo TODO Add stuff to fill
     * 
     * **cfiletab;     // Array of config files
     * cfiletab_size;  // Size of the table storing config pointers
     */

    if (!mushstate.initializing)
    {
        return CF_Failure;
    }

    buf = XSTRDUP(str, "buf");
    fp = fopen(buf, "r");

    if (fp == NULL)
    {
        XFREE(buf);
        buf = XASPRINTF("buf", "%s/%s", mushconf.config_home, str);
        fp = fopen(buf, "r");

        if (fp == NULL)
        {
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config file", str);
            return CF_Failure;
        }
    }

    log_write(LOG_ALWAYS, "CNF", "INFO", "Reading configuration file : %s", buf);
    mushstate.cfiletab = add_array(mushstate.cfiletab, buf, &mushstate.configfiles);
    XFREE(buf);
    buf = XMALLOC(LBUF_SIZE, "buf");

    if (fgets(buf, LBUF_SIZE, fp) == NULL)
    {
        if (!feof(fp))
        {
            cf_log(player, "CNF", "ERROR", "Line:", "%d - %s", line + 1, "Error while reading configuration file.");
        }

        XFREE(buf);
        fclose(fp);
        return CF_Failure;
    }

    line++;

    while (!feof(fp))
    {
        cp = buf;

        if (*cp == '#')
        {
            if (fgets(buf, LBUF_SIZE, fp) == NULL)
            {
                if (!feof(fp))
                {
                    cf_log(player, "CNF", "ERROR", "Line:", "%d - %s", line + 1, "Error while reading configuration file.");
                }

                XFREE(buf);
                fclose(fp);
                return CF_Failure;
            }

            line++;
            continue;
        }

        /**
    	 * Not a comment line.  Strip off the NL and any characters
    	 * following it.  Then, split the line into the command and
    	 * argument portions (separated by a space).  Also, trim off
    	 * the trailing comment, if any (delimited by #)
    	 */

        for (cp = buf; *cp && *cp != '\n'; cp++)
            ;

        /** 
         * strip \n 
         * 
         */
        *cp = '\0'; 

        for (cp = buf; *cp && isspace(*cp); cp++)
            ; 
            
        /** 
         * strip spaces 
         * 
         */
        for (ap = cp; *ap && !isspace(*ap); ap++)
            ; 
        
        /** 
         * skip over command 
         * 
         */
        if (*ap)
        {
            /** 
             * trim command 
             * 
             */
            *ap++ = '\0';
        }

        for (; *ap && isspace(*ap); ap++)
            ; 
        
        /** 
         * skip spaces 
         * 
         */
        for (zp = ap; *zp && (*zp != '#'); zp++)
            ;
            
        /** 
         * find comment 
         * 
         */
        if (*zp && !(isdigit(*(zp + 1)) && isspace(*(zp - 1))))
        {
            *zp = '\0';
        }

        /** zap comment, but only if it's not sitting between whitespace and a 
         * digit, which traps a case like 'master_room #2'
         * 
         */
        for (zp = zp - 1; zp >= ap && isspace(*zp); zp--)
        {
            /** 
             * zap trailing spaces 
             * 
             */
            *zp = '\0';
        }

        cf_set(cp, ap, player);

        if (fgets(buf, LBUF_SIZE, fp) == NULL)
        {
            if (!feof(fp))
            {
                cf_log(player, "CNF", "ERROR", "Line:", "%d - %s", line + 1, "Error while reading configuration file.");
            }

            XFREE(buf);
            fclose(fp);
            return CF_Failure;
        }

        line++;
    }

    XFREE(buf);
    fclose(fp);
    return CF_Success;
}

/**
 * @brief Set config parameter.
 * 
 * @param cp 
 * @param ap 
 * @param player 
 * @param tp 
 * @return int 
 */
CF_Result helper_cf_set(char *cp, char *ap, dbref player, CONF *tp)
{
    int i = 0, r = CF_Failure;
    char *buf = NULL, *buff = NULL, *name = NULL, *status = NULL;

    if (!mushstate.standalone && !mushstate.initializing && !check_access(player, tp->flags))
    {
        notify(player, NOPERM_MESSAGE);
    }
    else
    {
        if (!mushstate.initializing)
        {
            buff = XSTRDUP(ap, "buff");
        }

        i = tp->interpreter(tp->loc, ap, tp->extra, player, cp);

        if (!mushstate.initializing)
        {
            name = log_getname(player);

            switch (i)
            {
            case 0:
                r = CF_Success;
                status = XSTRDUP("Success.", "status");
                break;

            case 1:
                r = CF_Partial;
                status = XSTRDUP("Partial success.", "status");
                break;

            case -1:
                r = CF_Failure;
                status = XSTRDUP("Failure.", "status");
                break;

            default:
                r = CF_Failure;
                status = XSTRDUP("Strange.", "status");
            }

            buf = strip_ansi(buff);
            log_write(LOG_CONFIGMODS, "CFG", "UPDAT", "%s entered config directive: %s with args '%s'. Status: %s", name, cp, buf, status);
            XFREE(buf);
            XFREE(name);
            XFREE(status);
            XFREE(buff);
        }
    }

    return r;
}

/**
 * @brief Set a config directive
 * 
 * @param cp 
 * @param ap 
 * @param player 
 * @return CF_Result 
 */
CF_Result cf_set(char *cp, char *ap, dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    /**
     * Search the config parameter table for the command. If we find it,
     * call the handler to parse the argument. Make sure that if we're standalone,
     * the paramaters we need to load module flatfiles are loaded
     */

    if (mushstate.standalone && strcmp(cp, "module") && strcmp(cp, "database_home"))
    {
        return CF_Success;
    }

    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcmp(tp->pname, cp))
        {
            return (helper_cf_set(cp, ap, player, tp));
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcmp(tp->pname, cp))
                {
                    return (helper_cf_set(cp, ap, player, tp));
                }
            }
        }
    }

    /**
     * Config directive not found.  Complain about it.
     * 
     */
    if (!mushstate.standalone)
    {
        cf_log(player, "CNF", "NFND", (char *)"Set", "%s %s not found", "Config directive", cp);
    }

    return CF_Failure;
}

/**
 * @brief Command handler to set config params at runtime
 * 
 * @param player    DBref of player
 * @param cause     DBref of cause
 * @param extra     Extra data
 * @param kw        Keyword
 * @param value     Value
 */
void do_admin(dbref player, dbref cause __attribute__((unused)), int extra __attribute__((unused)), char *kw, char *value)
{
    int i = cf_set(kw, value, player);

    if ((i >= 0) && !Quiet(player))
    {
        notify(player, "Set.");
    }

    return;
}

/**
 * @brief Read in config parameters from named file
 * 
 * @param fn    Filename
 * @return int 
 */
CF_Result cf_read(char *fn)
{
    int retval = cf_include(NULL, fn, 0, 0, (char *)"init");

    return retval;
}

/**
 * @brief List write or read access to config directives.
 * 
 * @param player DBref of player
 */
void list_cf_access(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *buff = NULL;

    for (tp = conftable; tp->pname; tp++)
    {
        if (God(player) || check_access(player, tp->flags))
        {
            buff = XASPRINTF("buff", "%s:", tp->pname);
            listset_nametab(player, access_nametab, tp->flags, buff, 1);
            XFREE(buff);
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (God(player) || check_access(player, tp->flags))
                {
                    buff = XASPRINTF("buff", "%s:", tp->pname);
                    listset_nametab(player, access_nametab, tp->flags, buff, 1);
                    XFREE(buff);
                }
            }
        }
    }
}

/**
 * @brief List read access to config directives.
 * 
 * @param player DBref of player
 */
void list_cf_read_access(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *buff = NULL;

    for (tp = conftable; tp->pname; tp++)
    {
        if (God(player) || check_access(player, tp->rperms))
        {
            buff = XASPRINTF("buff", "%s:", tp->pname);
            listset_nametab(player, access_nametab, tp->rperms, buff, 1);
            XFREE(buff);
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (God(player) || check_access(player, tp->rperms))
                {
                    buff = XASPRINTF("buff", "%s:", tp->pname);
                    listset_nametab(player, access_nametab, tp->rperms, buff, 1);
                    XFREE(buff);
                }
            }
        }
    }
}

/**
 * @brief Walk all configuration tables and validate any dbref values.
 * 
 */
void cf_verify(void)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    for (tp = conftable; tp->pname; tp++)
    {
        if (tp->interpreter == cf_dbref)
        {
            if (!(((tp->extra == NOTHING) && (*(tp->loc) == NOTHING)) || (Good_obj(*(tp->loc)) && !Going(*(tp->loc)))))
            {
                log_write(LOG_ALWAYS, "CNF", "VRFY", "%s #%d is invalid. Reset to #%d.", tp->pname, *(tp->loc), tp->extra);
                *(tp->loc) = (dbref)tp->extra;
            }
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (tp->interpreter == cf_dbref)
                {
                    if (!(((tp->extra == NOTHING) && (*(tp->loc) == NOTHING)) || (Good_obj(*(tp->loc)) && !Going(*(tp->loc)))))
                    {
                        log_write(LOG_ALWAYS, "CNF", "VRFY", "%s #%d is invalid. Reset to #%d.", tp->pname, *(tp->loc), tp->extra);
                        *(tp->loc) = (dbref)tp->extra;
                    }
                }
            }
        }
    }
}

/**
 * @brief Helper function for cf_display
 * 
 * @param player    DBref of player
 * @param buff      Output buffer
 * @param bufc      Output buffer tracker
 * @param tp        Config parameter
 */
void helper_cf_display(dbref player, char *buff, char **bufc, CONF *tp)
{
    NAMETAB *opt = NULL;

    if (!check_access(player, tp->rperms))
    {
        SAFE_NOPERM(buff, bufc);
        return;
    }

    if ((tp->interpreter == cf_bool) || (tp->interpreter == cf_int) || (tp->interpreter == cf_int_factor) || (tp->interpreter == cf_const))
    {
        SAFE_LTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
        return;
    }

    if (tp->interpreter == cf_string)
    {
        SAFE_LB_STR(*((char **)tp->loc), buff, bufc);
        return;
    }

    if (tp->interpreter == cf_dbref)
    {
        SAFE_LB_CHR('#', buff, bufc);
        SAFE_LTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
        return;
    }

    if (tp->interpreter == cf_option)
    {
        opt = find_nametab_ent_flag(GOD, (NAMETAB *)tp->extra, *(tp->loc));
        SAFE_LB_STR((opt ? opt->name : "*UNKNOWN*"), buff, bufc);
        return;
    }

    SAFE_NOPERM(buff, bufc);
    return;
}

/**
 * @brief Given a config parameter by name, return its value in some sane fashion.
 * 
 * @param player        DBref of player
 * @param param_name    Parameter name
 * @param buff          Output buffer
 * @param bufc          Output buffer tracker
 */
void cf_display(dbref player, char *param_name, char *buff, char **bufc)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

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
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcasecmp(tp->pname, param_name))
                {
                    helper_cf_display(player, buff, bufc, tp);
                    return;
                }
            }
        }
    }

    SAFE_NOMATCH(buff, bufc);
}

/**
 * @brief List options to player
 * 
 * @param player DBref of player
 */
void list_options(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    for (tp = conftable; tp->pname; tp++)
    {
        if (((tp->interpreter == cf_const) || (tp->interpreter == cf_bool)) && (check_access(player, tp->rperms)))
        {
            raw_notify(player, "%-25s %c %s?", tp->pname, (*(tp->loc) ? 'Y' : 'N'), (tp->extra ? (char *)tp->extra : ""));
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)lt_dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (((tp->interpreter == cf_const) || (tp->interpreter == cf_bool)) && (check_access(player, tp->rperms)))
                {
                    raw_notify(player, "%-25s %c %s?", tp->pname, (*(tp->loc) ? 'Y' : 'N'), (tp->extra ? (char *)tp->extra : ""));
                }
            }
        }
    }
}

/**
 * @brief Wrapper around lt_dlopen that can format filename.
 * 
 * @param filename	filename to load (or construct if a format string is given)
 * @param ...		arguments for the format string
 * @return void*    Handler for the module.
 */
lt_dlhandle lt_dlopen_format(const char *filename, ...)
{
    int n = 0;
    size_t size = 0;
    char *p = NULL;
    va_list ap;
    lt_dlhandle dlhandle = NULL;

    va_start(ap, filename);
    n = vsnprintf(p, size, filename, ap);
    va_end(ap);

    if (n < 0)
    {
        return NULL;
    }

    size = (size_t)n + 1;
    p = malloc(size);
    if (p == NULL)
    {
        return NULL;
    }

    va_start(ap, filename);
    n = vsnprintf(p, size, filename, ap);
    va_end(ap);

    if (n < 0)
    {
        free(p);
        return NULL;
    }

    dlhandle = lt_dlopen(p);
    free(p);

    return dlhandle;
}

/**
 * @brief Wrapper for lt_dlsym that format symbol string.
 * 
 * @param place		Module handler
 * @param symbol	Symbol to load (or construct if a format string is given)
 * @param ...		arguments for the format string
 * @return void*	Return the address in the module handle, where the symbol is loaded
 */
void *lt_dlsym_format(lt_dlhandle place, const char *symbol, ...)
{
    int n = 0;
    size_t size = 0;
    char *p = NULL;
    va_list ap;
    void *dlsym = NULL;

    va_start(ap, symbol);
    n = vsnprintf(p, size, symbol, ap);
    va_end(ap);

    if (n < 0)
    {
        return NULL;
    }

    size = (size_t)n + 1;
    p = malloc(size);
    if (p == NULL)
    {
        return NULL;
    }

    va_start(ap, symbol);
    n = vsnprintf(p, size, symbol, ap);
    va_end(ap);

    if (n < 0)
    {
        free(p);
        return NULL;
    }

    dlsym = lt_dlsym(place, p);
    free(p);

    return dlsym;
}
