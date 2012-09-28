/* mushconf.h */

#include "copyright.h"

#ifndef __MUSHCONF_H
#define __MUSHCONF_H


/* ---------------------------------------------------------------------------
 * Modules and related things.
 */

typedef struct module_version_info MODVER;
struct module_version_info {
    char *version;
    char *author;
    char *email;
    char *url;
    char *description;
    char *copyright;
};

typedef struct module_linked_list MODULE;
struct module_linked_list {
    char *modname;
    lt_dlhandle handle;
    struct module_linked_list *next;
    int (*process_command)(dbref, dbref, int, char *, char *[], int);
    int (*process_no_match)(dbref, dbref, int, char *, char *, char *[], int);
    int (*did_it)(dbref, dbref, dbref, int, const char *, int, const char *, int, int, char *[], int, int);
    void (*create_obj)(dbref, dbref);
    void (*destroy_obj)(dbref, dbref);
    void (*create_player)(dbref, dbref, int, int);
    void (*destroy_player)(dbref, dbref);
    void (*announce_connect)(dbref, const char *, int);
    void (*announce_disconnect)(dbref, const char *, int);
    void (*examine)(dbref, dbref, dbref, int, int);
    void (*dump_database)(FILE *);
    void (*db_write)(void);
    void (*db_grow)(int, int);
    void (*db_write_flatfile)(FILE *);
    void (*do_second)(void);
    void (*cache_put_notify)(DBData, unsigned int);
    void (*cache_del_notify)(DBData, unsigned int);
    MODVER (*version)(dbref, dbref, int);
};

typedef struct api_function_data API_FUNCTION;
struct api_function_data {
    const char *name;
    const char *param_fmt;
    void (*handler)(void *, void *);
};

/* ---------------------------------------------------------------------------
 * CONFDATA:	runtime configurable parameters
 */

typedef struct confparm CONF;
struct confparm {
    char *pname;		/* parm name */
    int (*interpreter)();	/* routine to interp parameter */
    int flags;		/* control flags */
    int rperms;		/* read permission flags */
    int *loc;		/* where to store value */
    long extra;		/* extra data for interpreter */
};

typedef struct confdata CONFDATA;
struct confdata {
    int	cache_size;	/* Maximum size of cache */
    int	cache_width;	/* Number of cache cells */
    int	paylimit;	/* getting money gets hard over this much */
    int	digcost;	/* cost of @dig command */
    int	linkcost;	/* cost of @link command */
    int	opencost;	/* cost of @open command */
    int	robotcost;	/* cost of @robot command */
    int	createmin;	/* default (and minimum) cost of @create cmd */
    int	createmax;	/* max cost of @create command */
    int	quotas;		/* TRUE = have building quotas */
    int	room_quota;	/* quota needed to make a room */
    int	exit_quota;	/* quota needed to make an exit */
    int	thing_quota;	/* quota needed to make a thing */
    int	player_quota;	/* quota needed to make a robot player */
    int	sacfactor;	/* sacrifice earns (obj_cost/sfactor) + sadj */
    int	sacadjust;	/* ... */
    dbref	start_room;	/* initial location for non-Guest players */
    dbref	start_home;	/* initial HOME for players */
    dbref	default_home;	/* HOME when home is inaccessible */
    dbref	guest_start_room; /* initial location for Guests */
    int	vattr_flags;	/* Attr flags for all user-defined attrs */
    KEYLIST *vattr_flag_list; /* Linked list, for attr_type conf */
    int	log_options;	/* What gets logged */
    int	log_info;	/* Info that goes into log entries */
    int	log_diversion;	/* What logs get diverted? */
    Uchar	markdata[8];	/* Masks for marking/unmarking */
    int	ntfy_nest_lim;	/* Max nesting of notifys */
    int	fwdlist_lim;	/* Max objects in @forwardlist */
    int	propdir_lim;	/* Max objects in @propdir */
    int	dbopt_interval; /* Optimize db every N dumps */
    char	*dbhome;	/* Database home directory */
    char	*txthome;	/* Text files home directory */
    char	*binhome;	/* Binary home directory */
    char	*bakhome;	/* Backup home directory */
    char	*status_file;	/* Where to write arg to @shutdown */
    char 	*config_file;	/* MUSH's config file */
    char 	*log_file;	/* MUSH's log file */
    char 	*pid_file;	/* MUSH's pid file */
    char 	*db_file;	/* MUSH's db file */
    char	*compressexe;	/* Executable run to compress file */
    char	*mudowner;	/* Email of the game owner */
    int	have_pueblo;	/* Is Pueblo support compiled in? */
    int	have_zones;	/* Should zones be active? */
    int	port;		/* user port */
    int	conc_port;	/* concentrator port */
    int	init_size;	/* initial db size */
    int	use_global_aconn;     /* Do we want to use global @aconn code? */
    int	global_aconn_uselocks; /* global @aconn obeys uselocks? */
    int	have_guest;	/* Do we wish to allow a GUEST character? */
    int	guest_char;	/* player num of prototype GUEST character */
    int     guest_nuker;    /* Wiz who nukes the GUEST characters. */
    int     number_guests;  /* number of guest characters allowed */
    char	*guest_basename; /* Base name or alias for guest char */
    char    *guest_prefixes; /* Prefixes for the guest char's name */
    char    *guest_suffixes; /* Suffixes for the guest char's name */
    char	*guest_password; /* Default password for guests */
    char	*guest_file;	/* display if guest connects */
    char	*conn_file;	/* display on connect if no registration */
    char	*creg_file;	/* display on connect if registration */
    char	*regf_file;	/* display on (failed) create if reg is on */
    char	*motd_file;	/* display this file on login */
    char	*wizmotd_file;	/* display this file on login to wizards */
    char	*quit_file;	/* display on quit */
    char	*down_file;	/* display this file if no logins */
    char	*full_file;	/* display when max users exceeded */
    char	*site_file;	/* display if conn from bad site */
    char	*crea_file;	/* display this on login for new users */
    char	*motd_msg;	/* Wizard-settable login message */
    char	*wizmotd_msg;  /* Login message for wizards only */
    char	*downmotd_msg;  /* Settable 'logins disabled' message */
    char	*fullmotd_msg;  /* Settable 'Too many players' message */
    char	*dump_msg;	/* Message displayed when @dump-ing */
    char	*postdump_msg;  /* Message displayed after @dump-ing */
    char	*fixed_home_msg; /* Message displayed when going home and FIXED */
    char	*fixed_tel_msg; /* Message displayed when teleporting and FIXED */
    char	*huh_msg;	/* Message displayed when a Huh? is gotten */
#ifdef PUEBLO_SUPPORT
    char    *pueblo_msg;	/* Message displayed to Pueblo clients */
    char	*htmlconn_file;	/* display on PUEBLOCLIENT message */
#endif
    char	*exec_path;	/* argv[0] */
    LINKEDLIST *infotext_list; /* Linked list of INFO fields and values */
    int	indent_desc;	/* Newlines before and after descs? */
    int	name_spaces;	/* allow player names to have spaces */
    int	site_chars;	/* where to truncate site name */
    int	fork_dump;	/* perform dump in a forked process */
    int	fork_vfork;	/* use vfork to fork */
    int	sig_action;	/* What to do with fatal signals */
    int	paranoid_alloc;	/* Rigorous buffer integrity checks */
    int	max_players;	/* Max # of connected players */
    int	dump_interval;	/* interval between checkpoint dumps in secs */
    int	check_interval;	/* interval between db check/cleans in secs */
    int	events_daily_hour; /* At what hour should @daily be executed? */
    int	dump_offset;	/* when to take first checkpoint dump */
    int	check_offset;	/* when to perform first check and clean */
    int	idle_timeout;	/* Boot off players idle this long in secs */
    int	conn_timeout;	/* Allow this long to connect before booting */
    int	idle_interval;	/* when to check for idle users */
    int	retry_limit;	/* close conn after this many bad logins */
    int	output_limit;	/* Max # chars queued for output */
    int	paycheck;	/* players earn this much each day connected */
    int	paystart;	/* new players start with this much money */
    int	start_quota;	/* Quota for new players */
    int	start_room_quota;     /* Room quota for new players */
    int	start_exit_quota;     /* Exit quota for new players */
    int	start_thing_quota;    /* Thing quota for new players */
    int	start_player_quota;   /* Player quota for new players */
    int	payfind;	/* chance to find a penny with wandering */
    int	killmin;	/* default (and minimum) cost of kill cmd */
    int	killmax;	/* max cost of kill command */
    int	killguarantee;	/* cost of kill cmd that guarantees success */
    int	pagecost;	/* cost of @page command */
    int	searchcost;	/* cost of commands that search the whole DB */
    int	waitcost;	/* cost of @wait (refunded when finishes) */
    int	building_limit;	/* max number of objects in the db */
    int	queuemax;	/* max commands a player may have in queue */
    int	queue_chunk;	/* # cmds to run from queue when idle */
    int	active_q_chunk;	/* # cmds to run from queue when active */
    int	machinecost;	/* One in mc+1 cmds costs 1 penny (POW2-1) */
    int	clone_copy_cost;/* Does @clone copy value? */
    int	use_hostname;	/* TRUE = use machine NAME rather than quad */
    int	typed_quotas;	/* TRUE = use quotas by type */
    int	ex_flags;	/* TRUE = show flags on examine */
    int	robot_speak;	/* TRUE = allow robots to speak in public */
    int	pub_flags;	/* TRUE = flags() works on anything */
    int	quiet_look;	/* TRUE = don't see attribs when looking */
    int	exam_public;	/* Does EXAM show public attrs by default? */
    int	read_rem_desc;	/* Can the DESCs of nonlocal objs be read? */
    int	read_rem_name;	/* Can the NAMEs of nonlocal objs be read? */
    int	sweep_dark;	/* Can you sweep dark places? */
    int	player_listen;	/* Are AxHEAR triggered on players? */
    int	quiet_whisper;	/* Can others tell when you whisper? */
    int	dark_sleepers;	/* Are sleeping players 'dark'? */
    int	see_own_dark;	/* Do you see your own dark stuff? */
    int	idle_wiz_dark;	/* Do idling wizards get set dark? */
    int	visible_wizzes;	/* Do dark wizards show up on contents? */
    int	pemit_players;	/* Can you @pemit to faraway players? */
    int	pemit_any;	/* Can you @pemit to ANY remote object? */
    int	addcmd_match_blindly;   /* Does @addcommand produce a Huh?
					 * if syntax issues mean no wildcard
					 * is matched?
					 */
    int	addcmd_obey_stop;	/* Does @addcommand still multiple
					 * match on STOP objs?
					 */
    int	addcmd_obey_uselocks;	/* Does @addcommand obey uselocks? */
    int	lattr_oldstyle; /* Bad lattr() return empty or #-1 NO MATCH? */
    int	bools_oldstyle; /* TinyMUSH 2.x and TinyMUX bools */
    int	match_mine;	/* Should you check yourself for $-commands? */
    int	match_mine_pl;	/* Should players check selves for $-cmds? */
    int	switch_df_all;	/* Should @switch match all by default? */
    int	fascist_objeval; /* Does objeval() require victim control? */
    int	fascist_tport;	/* Src of teleport must be owned/JUMP_OK */
    int	terse_look;	/* Does manual look obey TERSE */
    int	terse_contents;	/* Does TERSE look show exits */
    int	terse_exits;	/* Does TERSE look show obvious exits */
    int	terse_movemsg;	/* Show move msgs (SUCC/LEAVE/etc) if TERSE? */
    int	trace_topdown;	/* Is TRACE output top-down or bottom-up? */
    int	safe_unowned;	/* Are objects not owned by you safe? */
    int	trace_limit;	/* Max lines of trace output if top-down */
    int	wiz_obey_linklock;	/* Do wizards obey linklocks? */
    int	local_masters;	/* Do we check Zone rooms as local masters? */
    int	match_zone_parents;	/* Objects in local master rooms
					 * inherit commands from their
					 * parents, just like normal?
					 */
    int	req_cmds_flag;	/* COMMANDS flag required to check $-cmds? */
    int	ansi_colors;	/* allow ANSI colors? */
    int	safer_passwords;/* enforce reasonably good password choices? */
    int	space_compress;	/* Convert multiple spaces into one space */
    int	instant_recycle;/* Do destroy_ok objects get insta-nuke? */
    int	dark_actions;	/* Trigger @a-actions even when dark? */
    int	no_ambiguous_match; /* match_result() -> last_match_result() */
    int	exit_calls_move; /* Matching an exit in the main command
				  * parser invokes the 'move' command.
				  */
    int	move_match_more; /* Exit matches in 'move' parse like the
				  * main command parser (local, global, zone;
				  * pick random on ambiguous).
				  */
    int	autozone;	/* New objs are zoned to creator's zone */
    int	page_req_equals; /* page command must always contain '=' */
    int	comma_say;	/* Use grammatically-correct comma in says */
    int	you_say;	/* Show 'You say' to the player */
    int	c_cmd_subst;	/* Is %c last command or ansi? */
    int	player_name_min; /* Minimum length of a player name */
    dbref	master_room;	/* Room containing default cmds/exits/etc */
    dbref	player_proto;	/* Player prototype to clone */
    dbref	room_proto;	/* Room prototype to clone */
    dbref	exit_proto;	/* Exit prototype to clone */
    dbref	thing_proto;	/* Thing prototype to clone */
    dbref	player_defobj;	/* Players use this for attr template */
    dbref	room_defobj;	/* Rooms use this for attr template */
    dbref	exit_defobj;	/* Exits use this for attr template */
    dbref	thing_defobj;	/* Things use this for attr template */
    dbref	player_parent;	/* Parent that players start with */
    dbref	room_parent;	/* Parent that rooms start with */
    dbref	exit_parent;	/* Parent that exits start with */
    dbref	thing_parent;	/* Parent that things start with */
    FLAGSET	player_flags;	/* Flags players start with */
    FLAGSET	room_flags;	/* Flags rooms start with */
    FLAGSET	exit_flags;	/* Flags exits start with */
    FLAGSET	thing_flags;	/* Flags things start with */
    FLAGSET	robot_flags;	/* Flags robots start with */
    FLAGSET stripped_flags; /* Flags stripped by @clone and @chown */
    char	*mud_name;	/* Name of the mud */
    char	*mud_shortname;	/* Shorter name, for log */
    char	*one_coin;	/* name of one coin (ie. "penny") */
    char	*many_coins;	/* name of many coins (ie. "pennies") */
    int	timeslice;	/* How often do we bump people's cmd quotas? */
    int	cmd_quota_max;	/* Max commands at one time */
    int	cmd_quota_incr;	/* Bump #cmds allowed by this each timeslice */
    int	lag_check;	/* Is CPU usage checking compiled in? */
    int	max_cmdsecs;	/* Threshhold for real time taken by command */
    int	control_flags;	/* Global runtime control flags */
    int	wild_times_lim;	/* Max recursions in wildcard match */
    int	cmd_nest_lim;	/* Max nesting of commands like @switch/now */
    int	cmd_invk_lim;	/* Max commands in one queue entry */
    int	func_nest_lim;	/* Max nesting of functions */
    int	func_invk_lim;	/* Max funcs invoked by a command */
    int	func_cpu_lim_secs; /* Max secs of func CPU time by a cmd */
    clock_t	func_cpu_lim;	/* Max clock ticks of func CPU time by a cmd */
    int	lock_nest_lim;	/* Max nesting of lock evals */
    int	parent_nest_lim;/* Max levels of parents */
    int	zone_nest_lim;	/* Max nesting of zones */
    int	numvars_lim;	/* Max number of variables per object */
    int	stack_lim;	/* Max number of items on an object stack */
    int	struct_lim;	/* Max number of defined structures for obj */
    int	instance_lim;	/* Max number of struct instances for obj */
    int	max_grid_size;  /* Maximum cells in a grid */
    int	max_player_aliases; /* Max number of aliases for a player */
    int	register_limit;	/* Max number of named q-registers */
    int     max_qpid;       /* Max total number of queue entries */
    char	*struct_dstr;	/* Delim string used for struct 'examine' */
};

extern CONFDATA mudconf;

/* ---------------------------------------------------------------------------
 * Various types.
 */

typedef struct site_data SITE;
struct site_data {
    struct site_data *next;		/* Next site in chain */
    struct in_addr address;		/* Host or network address */
    struct in_addr mask;		/* Mask to apply before comparing */
    int	flag;			/* Value to return on match */
};

typedef struct objlist_block OBLOCK;
struct objlist_block {
    struct objlist_block *next;
    dbref	data[(LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref)];
};

#define OBLOCK_SIZE ((LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref))

typedef struct objlist_stack OLSTK;
struct objlist_stack {
    struct objlist_stack *next;	/* Next object list in stack */
    OBLOCK	*head;		/* Head of object list */
    OBLOCK	*tail;		/* Tail of object list */
    OBLOCK	*cblock;	/* Current block for scan */
    int	count;		/* Number of objs in last obj list block */
    int	citm;		/* Current item for scan */
};

typedef struct markbuf MARKBUF;
struct markbuf {
    char	chunk[5000];
};

typedef struct alist ALIST;
struct alist {
    char	*data;
    int	len;
    struct alist *next;
};

typedef struct badname_struc BADNAME;
struct badname_struc {
    char	*name;
    struct badname_struc	*next;
};

typedef struct forward_list FWDLIST;
struct forward_list {
    int	count;
    int	*data;
};

typedef struct propdir_list PROPDIR;
struct propdir_list {
    int	count;
    int	*data;
};

/* ---------------------------------------------------------------------------
 * State data.
 */

typedef struct {
    int major;		/* Major Version */
    int minor;		/* Minor Version */
    int status;		/* Status : 0 - Alpha, 1 - Beta, 2 - Release Candidate, 3 - Gamma */
    int revision;		/* Patch Level */
} versioninfo;

typedef struct statedata STATEDATA;
struct statedata {
    int	record_players; /* The maximum # of player logged on */
    int	db_block_size;	/* Block size of database */
    Obj	*objpipes[NUM_OBJPIPES];
    /* Number of object pipelines */
    unsigned int objc;	/* Object reference counter */
    versioninfo	version;	/* MUSH version info */
    char	*configureinfo;	/* Configure switches */
    char	*compilerinfo;	/* Compiler command line */
    char	*linkerinfo;	/* Linker command line */
    char	*dbmdriver;	/* DBM Driver */
    char	modloaded[MBUF_SIZE];	/* Modules loaded */
    int	initializing;	/* Are we reading config file at startup? */
    int	loading_db;	/* Are we loading the db? */
    int	standalone;	/* Are we converting the database? */
    int	panicking;	/* Are we in the middle of dying horribly? */
    int	restarting;	/* Are we restarting? */
    int	dumping;	/* Are we dumping? */
    int running;	/* Are we running? */
    pid_t	dumper;		/* If forked-dumping, with what pid? */
    int	logging;	/* Are we in the middle of logging? */
    int	epoch;		/* Generation number for dumps */
    int	generation;	/* DB global generation number */
    int	mudlognum;	/* Number of logfile */
    int	helpfiles;	/* Number of external indexed helpfiles */
    int	hfiletab_size;	/* Size of the table storing path pointers */
    char	**hfiletab;	/* Array of path pointers */
    HASHTAB *hfile_hashes;	/* Pointer to an array of index hashtables */
    dbref	curr_enactor;	/* Who initiated the current command */
    dbref	curr_player;	/* Who is running the current command */
    char	*curr_cmd;	/* The current command */
    int	alarm_triggered;/* Has periodic alarm signal occurred? */
    time_t	now;		/* What time is it now? */
    time_t	dump_counter;	/* Countdown to next db dump */
    time_t	check_counter;	/* Countdown to next db check */
    time_t	idle_counter;	/* Countdown to next idle check */
    time_t	mstats_counter;	/* Countdown to next mstats snapshot */
    time_t  events_counter; /* Countdown to next events check */
    int	shutdown_flag;	/* Should interface be shut down? */
    int	flatfile_flag;	/* Dump a flatfile when we have the chance */
    time_t	start_time;	/* When was MUSH started */
    time_t	restart_time;	/* When did we last restart? */
    int	reboot_nums;	/* How many times have we restarted? */
    time_t	cpu_count_from; /* When did we last reset CPU counters? */
    char	*debug_cmd;	/* The command we are executing (if any) */
    char	doing_hdr[DOING_LEN]; /* Doing column header in WHO display */
    SITE	*access_list;	/* Access states for sites */
    SITE	*suspect_list;	/* Sites that are suspect */
    HASHTAB	command_htab;	/* Commands hashtable */
    HASHTAB	logout_cmd_htab;/* Logged-out commands hashtable (WHO, etc) */
    HASHTAB func_htab;	/* Functions hashtable */
    HASHTAB ufunc_htab;	/* Local functions hashtable */
    HASHTAB powers_htab;    /* Powers hashtable */
    HASHTAB flags_htab;	/* Flags hashtable */
    HASHTAB	attr_name_htab;	/* Attribute names hashtable */
    HASHTAB vattr_name_htab;/* User attribute names hashtable */
    HASHTAB player_htab;	/* Player name->number hashtable */
    HASHTAB nref_htab;	/* Object name reference #_name_ mapping */
    NHSHTAB	desc_htab;	/* Socket descriptor hashtable */
    NHSHTAB	fwdlist_htab;	/* Room forwardlists */
    NHSHTAB	propdir_htab;	/* Propdir lists */
    NHSHTAB	qpid_htab;	/* Queue process IDs */
    NHSHTAB redir_htab;	/* Redirections */
    NHSHTAB objstack_htab;	/* Object stacks */
    NHSHTAB objgrid_htab;	/* Object grids */
    NHSHTAB	parent_htab;	/* Parent $-command exclusion */
    HASHTAB vars_htab;	/* Persistent variables hashtable */
    HASHTAB structs_htab;	/* Structure hashtable */
    HASHTAB cdefs_htab;	/* Components hashtable */
    HASHTAB instance_htab;	/* Instances hashtable */
    HASHTAB instdata_htab;	/* Structure data hashtable */
    HASHTAB api_func_htab;	/* Registered module API functions */
    MODULE *modules_list;	/* Loadable modules hashtable */
    int	max_structs;
    int	max_cdefs;
    int	max_instance;
    int	max_instdata;
    int	max_stacks;
    int	max_vars;
    int	attr_next;	/* Next attr to alloc when freelist is empty */
    BQUE	*qfirst;	/* Head of player queue */
    BQUE	*qlast;		/* Tail of player queue */
    BQUE	*qlfirst;	/* Head of object queue */
    BQUE	*qllast;	/* Tail of object queue */
    BQUE	*qwait;		/* Head of wait queue */
    BQUE	*qsemfirst;	/* Head of semaphore queue */
    BQUE	*qsemlast;	/* Tail of semaphore queue */
    BADNAME	*badname_head;	/* List of disallowed names */
    int	mstat_ixrss[2];	/* Summed shared size */
    int	mstat_idrss[2];	/* Summed private data size */
    int	mstat_isrss[2];	/* Summed private stack size */
    int	mstat_secs[2];	/* Time of samples */
    int	mstat_curr;	/* Which sample is latest */
    ALIST	iter_alist;	/* Attribute list for iterations */
    char	*mod_alist;	/* Attribute list for modifying */
    int	mod_size;	/* Length of modified buffer */
    dbref	mod_al_id;	/* Where did mod_alist come from? */
    OLSTK	*olist;		/* Stack of object lists for nested searches */
    dbref	freelist;	/* Head of object freelist */
    int	min_size;	/* Minimum db size (from file header) */
    int	db_top;		/* Number of items in the db */
    int	db_size;	/* Allocated size of db structure */
    unsigned int moduletype_top;	/* Highest module DBTYPE */
    int	*guest_free;	/* Table to keep track of free guests */
    MARKBUF	*markbits;	/* temp storage for marking/unmarking */
    int	in_loop;	/* In a loop() statement? */
    char	*loop_token[MAX_ITER_NESTING];	/* Value of ## */
    char	*loop_token2[MAX_ITER_NESTING];	/* Value of #? */
    int	loop_number[MAX_ITER_NESTING];	/* Value of #@ */
    int	loop_break[MAX_ITER_NESTING];	/* Kill this iter() loop? */
    int	in_switch;	/* In a switch() statement? */
    char	*switch_token;	/* Value of #$ */
    int	func_nest_lev;	/* Current nesting of functions */
    int	func_invk_ctr;	/* Functions invoked so far by this command */
    int	ntfy_nest_lev;	/* Current nesting of notifys */
    int	lock_nest_lev;	/* Current nesting of lock evals */
    int	cmd_nest_lev;	/* Current nesting of commands like @sw/now */
    int	cmd_invk_ctr;	/* Commands invoked so far by this qentry */
    int	wild_times_lev;	/* Wildcard matching tries. */
    GDATA	*rdata;		/* Global register data */
    int	zone_nest_num;  /* Global current zone nest position */
    int	break_called;	/* Boolean flag for @break and @assert */
    int	f_limitmask;	/* Flagword for limiter for functions */
    int	inpipe;		/* Boolean flag for command piping */
    char	*pout;		/* The output of the pipe used in %| */
    char	*poutnew;	/* The output being build by the current command */
    char	*poutbufc; 	/* Buffer position for poutnew */
    dbref	poutobj;	/* Object doing the piping */
    clock_t	cputime_base;	/* CPU baselined at beginning of command */
    clock_t	cputime_now;	/* CPU time recorded during command */
    const unsigned char *retabs; /* PCRE regexp tables */
#if !defined(TEST_MALLOC) && defined(RAW_MEMTRACKING)
    MEMTRACK *raw_allocs;	/* Tracking of raw memory allocations */
#endif
    int	dbm_fd;		/* File descriptor of our DBM database */
};

extern STATEDATA mudstate;

/* ---------------------------------------------------------------------------
 * Configuration parameter handler definition
 */

/*
#define CF_HAND(proc)	int proc (vp, str, extra, player, cmd) \
			int	*vp; \
			char	*str, *cmd; \
			long	extra; \
			dbref	player;
*/
/* This is for the DEC Alpha, which can't cast a pointer to an int. */
/*
#define CF_AHAND(proc)	int proc (vp, str, extra, player, cmd) \
			long	**vp; \
			char	*str, *cmd; \
			long	extra; \
			dbref	player;

#define CF_HDCL(proc)	int proc(int *, char *, long, dbref, char *)
*/

/* ---------------------------------------------------------------------------
 * Misc. constants.
 */

/* Global flags */

/* Game control flags in mudconf.control_flags */

#define	CF_LOGIN	0x0001		/* Allow nonwiz logins to the MUSH */
#define CF_BUILD	0x0002		/* Allow building commands */
#define CF_INTERP	0x0004		/* Allow object triggering */
#define CF_CHECKPOINT	0x0008		/* Perform auto-checkpointing */
#define	CF_DBCHECK	0x0010		/* Periodically check/clean the DB */
#define CF_IDLECHECK	0x0020		/* Periodically check for idle users */
/* empty		0x0040 */
/* empty		0x0080 */
#define CF_DEQUEUE	0x0100		/* Remove entries from the queue */
#define CF_GODMONITOR   0x0200  /* Display commands to the God. */
#define CF_EVENTCHECK   0x0400		/* Allow events checking */
/* Host information codes */

#define H_REGISTRATION	0x0001	/* Registration ALWAYS on */
#define H_FORBIDDEN	0x0002	/* Reject all connects */
#define H_SUSPECT	0x0004	/* Notify wizards of connects/disconnects */
#define H_GUEST         0x0008  /* Don't permit guests from here */

/* Logging options */

#define LOG_ALLCOMMANDS	0x00000001	/* Log all commands */
#define LOG_ACCOUNTING	0x00000002	/* Write accounting info on logout */
#define LOG_BADCOMMANDS	0x00000004	/* Log bad commands */
#define LOG_BUGS	0x00000008	/* Log program bugs found */
#define LOG_DBSAVES	0x00000010	/* Log database dumps */
#define LOG_CONFIGMODS	0x00000020	/* Log changes to configuration */
#define LOG_PCREATES	0x00000040	/* Log character creations */
#define LOG_KILLS	0x00000080	/* Log KILLs */
#define LOG_LOGIN	0x00000100	/* Log logins and logouts */
#define LOG_NET		0x00000200	/* Log net connects and disconnects */
#define LOG_SECURITY	0x00000400	/* Log security-related events */
#define LOG_SHOUTS	0x00000800	/* Log shouts */
#define LOG_STARTUP	0x00001000	/* Log nonfatal errors in startup */
#define LOG_WIZARD	0x00002000	/* Log dangerous things */
#define LOG_ALLOCATE	0x00004000	/* Log alloc/free from buffer pools */
#define LOG_PROBLEMS	0x00008000	/* Log runtime problems */
#define LOG_KBCOMMANDS	0x00010000	/* Log keyboard commands */
#define LOG_SUSPECTCMDS	0x00020000	/* Log SUSPECT player keyboard cmds */
#define LOG_TIMEUSE	0x00040000	/* Log CPU time usage */
#define LOG_LOCAL	0x00080000	/* Log user stuff via @log */
#define LOG_ALWAYS	0x80000000	/* Always log it */

#define LOGOPT_FLAGS		0x01	/* Report flags on object */
#define LOGOPT_LOC		0x02	/* Report loc of obj when requested */
#define LOGOPT_OWNER		0x04	/* Report owner of obj if not obj */
#define LOGOPT_TIMESTAMP	0x08	/* Timestamp log entries */

#endif	/* __MUDCONF_H */
