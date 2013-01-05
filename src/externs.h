/* externs.h - Prototypes for externs not defined elsewhere */

#include "copyright.h"

#ifndef __EXTERNS_H
#define	__EXTERNS_H

/*
 * --------------------------------------------------------------------------
 * External function declarations.
 */

/* From external sources */
extern char    *crypt( const char *, const char * );

/* From alloc.c */
extern void    *xstrprintf(const char *, const char *, ...);

/* From boolexp.c */
extern int	eval_boolexp( dbref, dbref, dbref, BOOLEXP * );
extern int	eval_boolexp_atr( dbref, dbref, dbref, char * );
extern BOOLEXP *parse_boolexp( dbref, const char *, int );

/* From bsd.c */
extern void	boot_slave( void );
extern void	emergency_shutdown( void );
extern void	shutdownsock( DESC *, int );
extern void	shovechars( int );
extern void	set_signals( void );

/* From command.c */
extern int	check_access( dbref, int );
extern int	check_mod_access( dbref, EXTFUNCS * );
extern void	reset_prefix_cmds( void );
extern void	process_cmdline( dbref, dbref, char *, char *[], int, BQUE * );
extern void	call_move_hook( dbref, dbref, int );

/* From conf.c */
extern void	cf_log_syntax( dbref, char *, const char *,... );
extern void	cf_log_help( dbref, char *, const char *, ... );
extern void	cf_log_help_mkindx( dbref, char *, const char *, ... );
extern void	cf_log_notfound( dbref, char *, const char *, char * );
extern int	cf_modify_bits( int *, char *, long, dbref, char * );

/* From cque.c */
extern int	nfy_que( dbref, dbref, int, int, int );
extern int	halt_que( dbref, dbref );
extern void	wait_que( dbref, dbref, int, dbref, int, char *, char *[], int, GDATA * );
extern int	que_next( void );
extern int	do_top( int ncmds );
extern void	do_second( void );

/* From create.c */
extern int	destroyable( dbref );

/* From db.c */
extern char *getstring_noalloc( FILE *, int );
extern void	putstring( FILE *, const char * );
extern void	dump_restart_db( void );
extern int	Commer( dbref );
extern void	s_Pass( dbref, const char * );
extern void s_Name( dbref, char * );
extern char *Name( dbref );
extern char *PureName( dbref );
extern void safe_name( dbref, char *, char ** );
extern void	safe_exit_name( dbref, char *, char ** );
extern int	fwdlist_load( FWDLIST *, dbref, char * );
extern void	fwdlist_set( dbref, FWDLIST * );
extern void	fwdlist_clr( dbref );
extern int	fwdlist_rewrite( FWDLIST *, char * );
extern FWDLIST *fwdlist_get( dbref );
extern int	propdir_load( PROPDIR *, dbref, char * );
extern void	propdir_set( dbref, PROPDIR * );
extern void	propdir_clr( dbref );
extern int	propdir_rewrite( PROPDIR *, char * );
extern PROPDIR *propdir_get( dbref );
extern void	atr_push( void );
extern void	atr_pop( void );
extern int	atr_head( dbref, char ** );
extern int	atr_next( char ** );
extern int	init_gdbm_db( char * );
extern void	atr_cpy( dbref, dbref, dbref );
extern void	atr_chown( dbref );
extern void	atr_clr( dbref, int );
extern void	atr_add_raw( dbref, int, char * );
extern void	atr_add( dbref, int, char *, dbref, int );
extern void	atr_set_owner( dbref, int, dbref );
extern void	atr_set_flags( dbref, int, int );
extern char    *atr_get_raw( dbref, int );
extern char    *atr_get( dbref, int, dbref *, int *, int * );
extern char    *atr_pget( dbref, int, dbref *, int *, int * );
extern char    *atr_get_str( char *, dbref, int, dbref *, int *, int * );
extern char    *atr_pget_str( char *, dbref, int, dbref *, int *, int * );
extern int	atr_get_info( dbref, int, dbref *, int * );
extern int	atr_pget_info( dbref, int, dbref *, int * );
extern void	atr_free( dbref );
extern int	check_zone( dbref, dbref );
extern int	check_zone_for_player( dbref, dbref );
extern void	toast_player( dbref );

/* from db_rw.c */
extern BOOLEXP *getboolexp1( FILE * );
extern void putboolexp( FILE *, BOOLEXP * );

/* From eval.c */
extern char    *parse_to( char **, char, int );
extern char    *parse_arglist( dbref, dbref, dbref, char *, char, int, char *[], int, char *[], int );
extern void	exec( char *, char **, dbref, dbref, dbref, int, char **, char *[], int );
extern GDATA   *save_global_regs( const char * );
extern void	restore_global_regs( const char *, GDATA * );

/* From fnhelper.c */
extern dbref	match_thing( dbref, char * );
extern int	xlate( char * );
extern long	random_range( long, long );

/* From funmisc.c */

extern int do_convtime( char *, struct tm * );

/* From game.c */
extern void	notify_except( dbref, dbref, dbref, int, const char  *, ... );
extern void	notify_except2( dbref, dbref, dbref, dbref, int, const char *, ... );
extern void	notify_check( dbref, dbref, int, const char *, ... );
extern int	Hearer( dbref );
extern void	html_escape( const char *, char *, char ** );
extern void	dump_database_internal( int );
extern void	fork_and_dump( dbref, dbref, int );
extern int	copy_file( char *,char *, int );
extern char 	**add_array( char **, char *, int *, char * );
extern void	write_status_file( dbref, char * );
extern char	*mktimestamp( char *, size_t );
extern void	do_backup_mush( dbref player, dbref cause, int key );

/* From help.c */
extern int	helpmkindx( dbref, char *, char * );

/* From htab.c */
extern int	cf_ntab_access( int *, char *, long, dbref, char * );

/* From log.c */
extern void	logfile_init( char * );
extern void	log_perror( const char *, const char *, const char *, const char  * );
extern void	log_write( int, const char *, const char *, const char *, ... );
extern void	log_write_raw( int, const char *, ... );
extern char 	*log_getname( dbref, char * );
extern char	*log_gettype( dbref, char * );
extern void	do_logrotate( dbref, dbref, int );
extern void	logfile_close( void );

/* From look.c */
extern void	look_in( dbref, dbref, int );
extern void	show_vrml_url( dbref, dbref );

/* From object.c */
extern dbref	new_home( dbref );
extern void	divest_object( dbref );
extern dbref	create_obj( dbref, int, char *, int );
extern void	destroy_obj( dbref, dbref );
extern void	empty_obj( dbref );

/* From netcommon.c */
extern void	raw_broadcast( int, char *,... );
extern struct timeval	timeval_sub( struct timeval, struct timeval );
extern int	msec_diff( struct timeval now, struct timeval then );
extern struct timeval	msec_add( struct timeval, int );
extern struct timeval	update_quotas( struct timeval, struct timeval );
extern void 	raw_notify_html( dbref, const char *, ... );
extern void	raw_notify( dbref, const char *, ... );
extern void	raw_notify_newline( dbref );
extern void	clearstrings( DESC * );
extern void	queue_write( DESC *, const char *, int );
extern void	queue_string( DESC *, const char *, ... );
extern void	queue_rawstring( DESC *, const char *, ... );
extern void	freeqs( DESC * );
extern void	welcome_user( DESC * );
extern void	save_command( DESC *, CBLK * );
extern void	announce_disconnect( dbref, DESC *, const char * );
extern int	boot_off( dbref, char * );
extern int	boot_by_port( int, int, char * );
extern void	check_idle( void );
extern void	process_commands( void );
extern int	site_check( struct in_addr, SITE * );
extern dbref	find_connected_name( dbref, char * );

/* From move.c */
extern void	move_object( dbref, dbref );
extern void	move_via_generic( dbref, dbref, dbref, int );
extern void	move_via_exit( dbref, dbref, dbref, dbref, int );
extern int	move_via_teleport( dbref, dbref, dbref, int );
extern void	move_exit( dbref, dbref, int, const char *, int );

/* From object.c */
extern void	destroy_player( dbref );

/* From player.c */
extern dbref	create_player( char *, char *, dbref, int, int );
extern int	add_player_name( dbref, char * );
extern int	delete_player_name( dbref, char * );
extern dbref	lookup_player( dbref, char *, int );
extern void	load_player_names( void );
extern void	badname_add( char * );
extern void	badname_remove( char * );
extern int	badname_check( char * );
extern void	badname_list( dbref, const char * );

/* From predicates.c */
extern char    *safe_snprintf(char *,  size_t, const char *, ... );
extern char    *safe_vsnprintf(char *, size_t, const char *, va_list );
//extern char    *tmprintf( const char *,... );
extern void	safe_sprintf( char *, char **, const char *, ... );
extern dbref	insert_first( dbref, dbref );
extern dbref	remove_first( dbref, dbref );
extern dbref	reverse_list( dbref );
extern int	member( dbref, dbref );
extern int	is_integer( char * );
extern int	is_number( char * );
extern int	could_doit( dbref, dbref, int );
extern void	add_quota( dbref, int, int );
extern int	canpayfees( dbref, dbref, int, int, int );
extern int	payfees( dbref, int, int, int );
extern void	giveto( dbref, int );
extern int	payfor( dbref, int );
extern int	ok_name( const char * );
extern int	ok_player_name( const char * );
extern int	ok_attr_name( const char * );
extern int	ok_password( const char *, dbref );
extern void	handle_ears( dbref, int, int );
extern dbref	match_possessed( dbref, dbref, char *, dbref, int );
extern void	parse_range( char **, dbref *, dbref * );
extern int	parse_thing_slash( dbref, char *, char **, dbref * );
extern int	get_obj_and_lock( dbref, char *, dbref *, ATTR **, char *, char ** );
extern dbref	where_is( dbref );
extern dbref	where_room( dbref );
extern int	locatable( dbref, dbref, dbref );
extern int	nearby( dbref, dbref );
extern char    *master_attr( dbref, dbref, int, char **, int, int * );
extern void	did_it( dbref, dbref, int, const char *, int, const char  *, int, int, char *[], int, int );

/* From set.c */
extern int	parse_attrib( dbref, char *, dbref *, int *, int );
extern int	parse_attrib_wild( dbref, char *, dbref *, int, int, int, int );
extern void	edit_string( char *, char **, char *, char * );
extern dbref	match_controlled( dbref, const char * );
extern dbref	match_affected( dbref, const char * );

/* From stringutil.c */
extern char    *upcasestr( char * );
extern char    *munge_space( char * );
extern char    *trim_spaces( char * );
extern char    *grabto( char **, char );
extern int	string_compare( const char *, const char * );
extern int	string_prefix( const char *, const char * );
extern const char *string_match( const char *, const char * );
extern char    *replace_string( const char *, const char *, const char  * );
extern void	edit_string( char *, char **, char *, char * );
extern char    *skip_space( const char * );
extern int	minmatch( char *, char *, int );
extern void safe_copy_str( const char *, char *, char **, int );
extern int safe_copy_str_fn( const char *, char *, char **, int );
extern int	safe_copy_long_str( char *, char *, char **, int );
extern void safe_known_str( const char *, int, char *, char ** );
extern int	matches_exit_from_list( char *, char * );
extern char    *translate_string( char *, int );
extern int	ltos( char *, long );
extern void safe_ltos( char *, char **, long );
extern char    *repeatchar( int, char );
extern char    *strip_ansi( const char * );
extern int	strip_ansi_len( const char * );
extern char    *normal_to_white( const char * );
extern char    *ansi_transition_esccode( int, int );
extern char    *ansi_transition_mushcode( int, int );
extern char    *ansi_transition_letters( int, int );
extern int	ansi_map_states( const char *, int **, char ** );
extern char     *remap_colors( const char *, int * );
extern void	track_ansi_letters(char *, int *);
extern void	track_esccode(char **, int *);
extern void	track_all_esccodes(char **, char **, int *);
extern void	skip_esccode(char **);
extern void	copy_esccode(char **, char **);
extern void	safe_copy_esccode(char **, char *, char **);

/* From timer.c */
extern int	call_cron( dbref, dbref, int, char * );
extern int	cron_clr( dbref, int );

/* From udb_achunk.c */
extern int	dddb_close( void );
extern int	dddb_setfile( char * );
extern int	dddb_init( void );

/* From unparse.c */
extern char    *unparse_boolexp( dbref, BOOLEXP * );
extern char    *unparse_boolexp_quiet( dbref, BOOLEXP * );
extern char    *unparse_boolexp_decompile( dbref, BOOLEXP * );
extern char    *unparse_boolexp_function( dbref, BOOLEXP * );

/* From walkdb.c */
extern int	chown_all( dbref, dbref, dbref, int );
extern void	olist_push( void );
extern void	olist_pop( void );
extern void	olist_add( dbref );
extern dbref	olist_first( void );
extern dbref	olist_next( void );

/* From wild.c */
extern int	wild( char *, char *, char *[], int );
extern int	wild_match( char *, char * );
extern int	quick_wild( char *, char * );
extern int	register_match( char *, char *, char *[], int );

/*
 * --------------------------------------------------------------------------
 * Constants.
 */

/* Command handler keys */

#define ADDCMD_PRESERVE 1	/* Use player rather than addcommand thing */
#define	ATTRIB_ACCESS	1	/* Change access to attribute */
#define	ATTRIB_RENAME	2	/* Rename attribute */
#define	ATTRIB_DELETE	4	/* Delete attribute */
#define ATTRIB_INFO	8	/* Info (number, flags) about attribute */
#define	BOOT_QUIET	1	/* Inhibit boot message to victim */
#define	BOOT_PORT	2	/* Boot by port number */
#define	CHOWN_ONE	1	/* item = new_owner */
#define	CHOWN_ALL	2	/* old_owner = new_owner */
#define CHOWN_NOSTRIP	4	/* Don't strip (most) flags from object */
#define CHZONE_NOSTRIP	1	/* Don't strip (most) flags from object */
#define	CLONE_LOCATION	0	/* Create cloned object in my location */
#define	CLONE_INHERIT	1	/* Keep INHERIT bit if set */
#define	CLONE_PRESERVE	2	/* Preserve the owner of the object */
#define	CLONE_INVENTORY	4	/* Create cloned object in my inventory */
#define	CLONE_SET_COST	8	/* ARG2 is cost of cloned object */
#define	CLONE_FROM_PARENT 16	/* Set parent instead of cloning attrs */
#define CLONE_NOSTRIP	32	/* Don't strip (most) flags from clone */
#define	DBCK_FULL	1	/* Do all tests */
#define DECOMP_PRETTY	1	/* pretty-format output */
#define	DEST_ONE	1	/* object */
#define	DEST_ALL	2	/* owner */
#define	DEST_OVERRIDE	4	/* override Safe() */
#define DEST_INSTANT	8	/* instantly destroy */
#define	DIG_TELEPORT	1	/* teleport to room after @digging */
#define DOLIST_SPACE    0	/* expect spaces as delimiter */
#define DOLIST_DELIMIT  1	/* expect custom delimiter */
#define DOLIST_NOTIFY   2	/* queue a '@notify me' at the end */
#define DOLIST_NOW	4	/* Run commands immediately, no queueing */
#define	DOING_MESSAGE	0	/* Set my DOING message */
#define	DOING_HEADER	1	/* Set the DOING header */
#define	DOING_POLL	2	/* List DOING header */
#define DOING_QUIET	4	/* Inhibit 'Set.' message */
#define	DROP_QUIET	1	/* Don't do Odrop/Adrop if control */
#define	DUMP_STRUCT	1	/* Dump flat structure file */
#define	DUMP_TEXT	2	/* Dump text (gdbm) file */
#define DUMP_FLATFILE	8	/* Dump to flatfile */
#define DUMP_OPTIMIZE	16	/* Optimized dump */
#define ENDCMD_BREAK	0	/* @break - end action list on true */
#define ENDCMD_ASSERT	1	/* @assert - end action list on false */
#define	EXAM_DEFAULT	0	/* Default */
#define	EXAM_BRIEF	1	/* Don't show attributes */
#define	EXAM_LONG	2	/* Nonowner sees public attrs too */
#define	EXAM_DEBUG	4	/* Display more info for finding db problems */
#define	EXAM_PARENT	8	/* Get attr from parent when exam obj/attr */
#define EXAM_PRETTY	16	/* Pretty-format output */
#define EXAM_PAIRS	32	/* Print paren matches in color */
#define	EXAM_OWNER	64	/* Nonowner sees just owner */
#define	FIXDB_OWNER	1	/* Fix OWNER field */
#define	FIXDB_LOC	2	/* Fix LOCATION field */
#define	FIXDB_CON	4	/* Fix CONTENTS field */
#define	FIXDB_EXITS	8	/* Fix EXITS field */
#define	FIXDB_NEXT	16	/* Fix NEXT field */
#define	FIXDB_PENNIES	32	/* Fix PENNIES field */
#define	FIXDB_NAME	64	/* Set NAME attribute */
#define	FLOATERS_ALL	1	/* Display all floating rooms in db */
#define FUNCT_LIST	1	/* List the user-defined functions */
#define FUNCT_NO_EVAL	2	/* Don't evaluate args to function */
#define FUNCT_PRIV	4	/* Perform ufun as holding obj */
#define FUNCT_PRES	8	/* Preserve r-regs before ufun */
#define FUNCT_NOREGS	16	/* Private r-regs for ufun */
#define	FRC_COMMAND	1	/* what=command */
#define FRC_NOW		2	/* run command immediately, no queueing */
#define	GET_QUIET	1	/* Don't do osucc/asucc if control */
#define	GIVE_QUIET	1	/* Inhibit give messages */
#define	GLOB_ENABLE	1	/* key to enable */
#define	GLOB_DISABLE	2	/* key to disable */
#define	HALT_ALL	1	/* halt everything */
#define HALT_PID	2	/* halt a particular PID */
#define HELP_FIND	1	/* do a wildcard search through help subjects */
#define HELP_RAWHELP	0x08000000	/* A high bit. Don't eval text. */
#define HOOK_BEFORE	1	/* pre-command hook */
#define HOOK_AFTER	2	/* post-command hook */
#define HOOK_PRESERVE	4	/* preserve global regs */
#define HOOK_NOPRESERVE	8	/* don't preserve global regs */
#define HOOK_PERMIT	16	/* user-defined permissions */
#define HOOK_PRIVATE	32	/* private global regs */
#define	KILL_KILL	1	/* gives victim insurance */
#define	KILL_SLAY	2	/* no insurance */
#define	LOOK_LOOK	1	/* list desc (and succ/fail if room) */
#define	LOOK_INVENTORY	2	/* list inventory of object */
#define	LOOK_SCORE	4	/* list score (# coins) */
#define	LOOK_OUTSIDE    8	/* look for object in container of player */
#define	MARK_SET	0	/* Set mark bits */
#define	MARK_CLEAR	1	/* Clear mark bits */
#define	MOTD_ALL	0	/* login message for all */
#define	MOTD_WIZ	1	/* login message for wizards */
#define	MOTD_DOWN	2	/* login message when logins disabled */
#define	MOTD_FULL	4	/* login message when too many players on */
#define	MOTD_LIST	8	/* Display current login messages */
#define	MOTD_BRIEF	16	/* Suppress motd file display for wizards */
#define	MOVE_QUIET	1	/* Dont do Osucc/Ofail/Asucc/Afail if ctrl */
#define	NFY_NFY		0	/* Notify first waiting command */
#define	NFY_NFYALL	1	/* Notify all waiting commands */
#define	NFY_DRAIN	2	/* Delete waiting commands */
#define NREF_LIST	1	/* List rather than set nrefs */
#define	OPEN_LOCATION	0	/* Open exit in my location */
#define	OPEN_INVENTORY	1	/* Open exit in me */
#define	PASS_ANY	1	/* name=newpass */
#define	PASS_MINE	2	/* oldpass=newpass */
#define	PCRE_PLAYER	1	/* create new player */
#define	PCRE_ROBOT	2	/* create robot player */
#define	PEMIT_PEMIT	1	/* emit to named player */
#define	PEMIT_OEMIT	2	/* emit to all in current room except named */
#define	PEMIT_WHISPER	3	/* whisper to player in current room */
#define	PEMIT_FSAY	4	/* force controlled obj to say */
#define	PEMIT_FEMIT	5	/* force controlled obj to emit */
#define	PEMIT_FPOSE	6	/* force controlled obj to pose */
#define	PEMIT_FPOSE_NS	7	/* force controlled obj to pose w/o space */
#define	PEMIT_CONTENTS	8	/* Send to contents (additive) */
#define	PEMIT_HERE	16	/* Send to location (@femit, additive) */
#define	PEMIT_ROOM	32	/* Send to containing rm (@femit, additive) */
#define PEMIT_LIST      64	/* Send to a list */
#define PEMIT_SPEECH	128	/* Explicitly tag this as speech */
#define PEMIT_HTML	256	/* HTML escape, and no newline */
#define PEMIT_MOVE	512	/* Explicitly tag this as a movement message */
#define PEMIT_SPOOF	1024	/* change enactor to target object */
#define	PS_BRIEF	0	/* Short PS report */
#define	PS_LONG		1	/* Long PS report */
#define	PS_SUMM		2	/* Queue counts only */
#define	PS_ALL		4	/* List entire queue */
#define	QUEUE_KICK	1	/* Process commands from queue */
#define	QUEUE_WARP	2	/* Advance or set back wait queue clock */
#define	QUOTA_SET	1	/* Set a quota */
#define	QUOTA_FIX	2	/* Repair a quota */
#define	QUOTA_TOT	4	/* Operate on total quota */
#define	QUOTA_REM	8	/* Operate on remaining quota */
#define	QUOTA_ALL	16	/* Operate on all players */
#define QUOTA_ROOM      32	/* Room quota set */
#define QUOTA_EXIT      64	/* Exit quota set */
#define QUOTA_THING     128	/* Thing quota set */
#define QUOTA_PLAYER    256	/* Player quota set */
#define	SAY_SAY		1	/* say in current room */
#define	SAY_NOSPACE	1	/* OR with xx_EMIT to get nospace form */
#define	SAY_POSE	2	/* pose in current room */
#define	SAY_POSE_NOSPC	3	/* pose w/o space in current room */
#define	SAY_EMIT	5	/* emit in current room */
#define	SAY_SHOUT	8	/* shout to all logged-in players */
#define	SAY_WALLPOSE	9	/* Pose to all logged-in players */
#define	SAY_WALLEMIT	10	/* Emit to all logged-in players */
#define	SAY_WIZSHOUT	12	/* shout to all logged-in wizards */
#define	SAY_WIZPOSE	13	/* Pose to all logged-in wizards */
#define	SAY_WIZEMIT	14	/* Emit to all logged-in wizards */
#define SAY_ADMINSHOUT	15	/* Emit to all wizards or royalty */
#define	SAY_NOTAG	32	/* Don't put Broadcast: in front (additive) */
#define	SAY_HERE	64	/* Output to current location */
#define	SAY_ROOM	128	/* Output to containing room */
#define SAY_HTML	256	/* Don't output a newline */
#define	SAY_PREFIX	512	/* first char indicates formatting */
#define	SET_QUIET	1	/* Don't display 'Set.' message. */
#define	SHUTDN_COREDUMP	1	/* Produce a coredump */
#define	SRCH_SEARCH	1	/* Do a normal search */
#define	SRCH_MARK	2	/* Set mark bit for matches */
#define	SRCH_UNMARK	3	/* Clear mark bit for matches */
#define	STAT_PLAYER	0	/* Display stats for one player or tot objs */
#define	STAT_ALL	1	/* Display global stats */
#define	STAT_ME		2	/* Display stats for me */
#define	SWITCH_DEFAULT	0	/* Use the configured default for switch */
#define	SWITCH_ANY	1	/* Execute all cases that match */
#define	SWITCH_ONE	2	/* Execute only first case that matches */
#define SWITCH_NOW	4	/* Execute case(s) immediately, no queueing */
#define	SWEEP_ME	1	/* Check my inventory */
#define	SWEEP_HERE	2	/* Check my location */
#define	SWEEP_COMMANDS	4	/* Check for $-commands */
#define	SWEEP_LISTEN	8	/* Check for @listen-ers */
#define	SWEEP_PLAYER	16	/* Check for players and puppets */
#define	SWEEP_CONNECT	32	/* Search for connected players/puppets */
#define	SWEEP_EXITS	64	/* Search the exits for audible flags */
#define	SWEEP_VERBOSE	256	/* Display what pattern matches */
#define TELEPORT_DEFAULT 1	/* Emit all messages */
#define TELEPORT_QUIET   2	/* Teleport in quietly */
#define TIMECHK_RESET	1	/* Reset all counters to zero */
#define TIMECHK_SCREEN	2	/* Write info to screen */
#define TIMECHK_LOG	4	/* Write info to log */
#define	TOAD_NO_CHOWN	1	/* Don't change ownership */
#define	TRIG_QUIET	1	/* Don't display 'Triggered.' message. */
#define TRIG_NOW	2	/* Run immediately, no queueing */
#define	TWARP_QUEUE	1	/* Warp the wait and sem queues */
#define	TWARP_DUMP	2	/* Warp the dump interval */
#define	TWARP_CLEAN	4	/* Warp the cleaning interval */
#define	TWARP_IDLE	8	/* Warp the idle check interval */
/* twarp empty		16 */
#define TWARP_EVENTS	32	/* Warp the events checking interval */
#define VERB_NOW	1	/* Run @afoo immediately, no queueing */
#define VERB_MOVE	2	/* Treat like movement message */
#define VERB_SPEECH	4	/* Treat like speech message */
#define VERB_PRESENT	8	/* Treat like presence message */
#define VERB_NONAME	16	/* Do not prepend name to odefault */
#define WAIT_UNTIL	1	/* Absolute time wait */
#define WAIT_PID	2	/* Change the wait on a PID */

/* Hush codes for movement messages */

#define	HUSH_ENTER	1	/* xENTER/xEFAIL */
#define	HUSH_LEAVE	2	/* xLEAVE/xLFAIL */
#define	HUSH_EXIT	4	/* xSUCC/xDROP/xFAIL from exits */

/* Evaluation directives */

#define	EV_FIGNORE	0x00000000	/* Don't look for func if () found */
#define	EV_FMAND	0x00000100	/* Text before () must be func name */
#define	EV_FCHECK	0x00000200	/* Check text before () for function */
#define	EV_STRIP	0x00000400	/* Strip one level of brackets */
#define	EV_EVAL		0x00000800	/* Evaluate results before returning */
#define	EV_STRIP_TS	0x00001000	/* Strip trailing spaces */
#define	EV_STRIP_LS	0x00002000	/* Strip leading spaces */
#define	EV_STRIP_ESC	0x00004000	/* Strip one level of \ characters */
#define	EV_STRIP_AROUND	0x00008000	/* Strip {} only at ends of string */
#define	EV_TOP		0x00010000	/* This is a toplevel call to eval() */
#define	EV_NOTRACE	0x00020000	/* Don't trace this call to eval */
#define EV_NO_COMPRESS  0x00040000	/* Don't compress spaces. */
#define EV_NO_LOCATION	0x00080000	/* Suppresses %l */
#define EV_NOFCHECK	0x00100000	/* Do not evaluate functions! */

/* Function flags */

#define	FN_VARARGS	0x80000000	/* allows variable # of args */
#define	FN_NO_EVAL	0x40000000	/* Don't evaluate args to function */
#define	FN_PRIV		0x20000000	/* Perform ufun as holding obj */
#define FN_PRES		0x10000000	/* Preserve r-regs before ufun */
#define FN_NOREGS	0x08000000	/* Private r-regs for ufun */
#define FN_DBFX         0x04000000	/* DB-affecting side effects */
#define FN_QFX          0x02000000	/* Queue-affecting side effects */
#define FN_OUTFX        0x01000000	/* Output-affecting side effects */
#define FN_STACKFX      0x00800000	/* All stack functions */
#define FN_VARFX	0x00400000	/* All xvars functions */

/* Lower flag values are used for function-specific switches */

/* Message forwarding directives */

#define	MSG_PUP_ALWAYS	0x00001	/* Always forward msg to puppet own */
#define	MSG_INV		0x00002	/* Forward msg to contents */
#define	MSG_INV_L	0x00004	/* ... only if msg passes my @listen */
#define	MSG_INV_EXITS	0x00008	/* Forward through my audible exits */
#define	MSG_NBR		0x00010	/* Forward msg to neighbors */
#define	MSG_NBR_A	0x00020	/* ... only if I am audible */
#define	MSG_NBR_EXITS	0x00040	/* Also forward to neighbor exits */
#define	MSG_NBR_EXITS_A	0x00080	/* ... only if I am audible */
#define	MSG_LOC		0x00100	/* Send to my location */
#define	MSG_LOC_A	0x00200	/* ... only if I am audible */
#define	MSG_FWDLIST	0x00400	/* Forward to my fwdlist members if audible */
#define	MSG_ME		0x00800	/* Send to me */
#define	MSG_S_INSIDE	0x01000	/* Originator is inside target */
#define	MSG_S_OUTSIDE	0x02000	/* Originator is outside target */
#define MSG_HTML	0x04000	/* Don't send \r\n */
#define MSG_SPEECH	0x08000	/* This message is speech. */
#define MSG_MOVE	0x10000	/* This message is movement. */
#define MSG_PRESENCE	0x20000	/* This message is related to presence. */
#define	MSG_ME_ALL	(MSG_ME|MSG_INV_EXITS|MSG_FWDLIST)
#define	MSG_F_CONTENTS	(MSG_INV)
#define	MSG_F_UP	(MSG_NBR_A|MSG_LOC_A)
#define	MSG_F_DOWN	(MSG_INV_L)

/* Look primitive directives */

#define	LK_IDESC	0x0001
#define	LK_OBEYTERSE	0x0002
#define	LK_SHOWATTR	0x0004
#define	LK_SHOWEXIT	0x0008
#define LK_SHOWVRML	0x0010

/* Quota types */
#define QTYPE_ALL 0
#define QTYPE_ROOM 1
#define QTYPE_EXIT 2
#define QTYPE_THING 3
#define QTYPE_PLAYER 4

/* Signal handling directives */

#define	SA_EXIT		1	/* Exit, and dump core */
#define	SA_DFLT		2	/* Try to restart on a fatal error */

/* Database dumping directives for dump_database_internal() */

#define DUMP_DB_NORMAL		0
#define DUMP_DB_CRASH		1
#define DUMP_DB_RESTART		2
#define DUMP_DB_FLATFILE	3
#define DUMP_DB_KILLED		4

/*
 * --------------------------------------------------------------------------
 * A zillion ways to notify things.
 */

#define	notify(p,m)					notify_check(p,p,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL,m)
#define notify_html(p,m)				notify_check(p,p,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|MSG_HTML,NULL,m)
#define	notify_quiet(p,m)				notify_check(p,p,MSG_PUP_ALWAYS|MSG_ME,NULL,m)
#define	notify_with_cause(p,c,m)			notify_check(p,c,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN,NULL,m)
#define notify_with_cause_html(p,c,m)			notify_check(p,c,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|MSG_HTML,NULL,m)
#define notify_with_cause_extra(p,c,m,f)		notify_check(p,c,MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|(f),NULL,m)
#define	notify_quiet_with_cause(p,c,m)			notify_check(p,c,MSG_PUP_ALWAYS|MSG_ME,NULL,m)
#define	notify_puppet(p,c,m)				notify_check(p,c,MSG_ME_ALL|MSG_F_DOWN,NULL,m)
#define	notify_quiet_puppet(p,c,m)			notify_check(p,c,MSG_ME,NULL,m)
#define	notify_all(p,c,m)				notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS|MSG_F_UP|MSG_F_CONTENTS,NULL,m)
#define	notify_all_from_inside(p,c,m)			notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE,NULL,m)
#define	notify_all_from_inside_speech(p,c,m)		notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE|MSG_SPEECH,NULL,m)
#define	notify_all_from_inside_move(p,c,m)		notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE|MSG_MOVE,NULL,m)
#define notify_all_from_inside_html(p,c,m)		notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE|MSG_HTML,NULL,m)
#define notify_all_from_inside_html_speech(p,c,m)	notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE|MSG_HTML|MSG_SPEECH,NULL,m)
#define	notify_all_from_outside(p,c,m)			notify_check(p,c,MSG_ME_ALL|MSG_NBR_EXITS|MSG_F_UP|MSG_F_CONTENTS|MSG_S_OUTSIDE,NULL,m)

#define CANNOT_HEAR_MSG  "That target cannot hear you."
#define NOT_PRESENT_MSG  "That target is not present."

/*
 * --------------------------------------------------------------------------
 * General macros.
 */

#define Randomize(n)  (random_range(0, (n) - 1))

#define	Protect(f) (cmdp->perms & f)

#define Invalid_Objtype(x) ((Protect(CA_LOCATION) && !Has_location(x)) || (Protect(CA_CONTENTS) && !Has_contents(x)) || (Protect(CA_PLAYER) && (Typeof(x) != TYPE_PLAYER)))

#define safe_atoi(s)	((s == NULL) ? 0 : (int)strtol(s, (char **)NULL, 10))

#define	test_top()		((mudstate.qfirst != NULL) ? 1 : 0)
#define	controls(p,x)		Controls(p,x)

/*
 * --------------------------------------------------------------------------
 * Global data things.
 */

#define Free_RegDataStruct(d) \
    if (d) { \
	if ((d)->q_regs) { \
	    XFREE((d)->q_regs, "q_regs"); \
	} \
	if ((d)->q_lens) { \
	    XFREE((d)->q_lens, "q_lens"); \
	} \
	if ((d)->x_names) { \
	    XFREE((d)->x_names, "x_names"); \
	} \
	if ((d)->x_regs) { \
	    XFREE((d)->x_regs, "x_regs"); \
	} \
	if ((d)->x_lens) { \
	    XFREE((d)->x_lens, "x_lens"); \
	} \
	XFREE(d, "gdata"); \
    }

#define Free_RegData(d) \
{ \
    int z; \
    if (d) { \
        for (z = 0; z < (d)->q_alloc; z++) { \
           if ((d)->q_regs[z]) \
                free_lbuf((d)->q_regs[z]); \
        } \
        for (z = 0; z < (d)->xr_alloc; z++) { \
           if ((d)->x_names[z]) \
                free_sbuf((d)->x_names[z]); \
           if ((d)->x_regs[z]) \
                free_lbuf((d)->x_regs[z]); \
        } \
        Free_RegDataStruct(d); \
    } \
}

#define Free_QData(q) \
    XFREE((q)->text, "queue.text"); \
    Free_RegDataStruct((q)->gdata);

#define Init_RegData(f, t) \
    (t) = (GDATA *) XMALLOC(sizeof(GDATA), (f)); \
    (t)->q_alloc = (t)->xr_alloc = 0; \
    (t)->q_regs = (t)->x_names = (t)->x_regs = NULL; \
    (t)->q_lens = (t)->x_lens = NULL; \
    (t)->dirty = 0;

#define Alloc_RegData(f, g, t) \
    if ((g) && ((g)->q_alloc || (g)->xr_alloc)) { \
	(t) = (GDATA *) XMALLOC(sizeof(GDATA), (f)); \
	(t)->q_alloc = (g)->q_alloc; \
	if ((g)->q_alloc) { \
	    (t)->q_regs = XCALLOC((g)->q_alloc, sizeof(char *), "q_regs"); \
	    (t)->q_lens = XCALLOC((g)->q_alloc, sizeof(int), "q_lens"); \
	} else { \
	    (t)->q_regs = NULL; \
	    (t)->q_lens = NULL; \
	} \
	(t)->xr_alloc = (g)->xr_alloc; \
	if ((g)->xr_alloc) { \
	    (t)->x_names = XCALLOC((g)->xr_alloc, sizeof(char *), "x_names"); \
	    (t)->x_regs = XCALLOC((g)->xr_alloc, sizeof(char *), "x_regs"); \
	    (t)->x_lens = XCALLOC((g)->xr_alloc, sizeof(int), "x_lens"); \
	} else { \
	    (t)->x_names = NULL; \
	    (t)->x_regs = NULL; \
	    (t)->x_lens = NULL; \
	} \
	(t)->dirty = 0; \
    } else { \
	(t) = NULL; \
    }

/* Copy global data from g to t */

#define Copy_RegData(f, g, t) \
{ \
    int z; \
    if ((g) && (g)->q_alloc) { \
	for (z = 0; z < (g)->q_alloc; z++) {  \
	    if ((g)->q_regs[z] && *((g)->q_regs[z])) { \
		(t)->q_regs[z] = alloc_lbuf(f); \
		memcpy((t)->q_regs[z], (g)->q_regs[z], (g)->q_lens[z] + 1); \
		(t)->q_lens[z] = (g)->q_lens[z]; \
	    } \
	} \
    } \
    if ((g) && (g)->xr_alloc) { \
	for (z = 0; z < (g)->xr_alloc; z++) { \
	    if ((g)->x_names[z] && *((g)->x_names[z]) && \
		(g)->x_regs[z] && *((g)->x_regs[z])) { \
		(t)->x_names[z] = alloc_sbuf("glob.x_name"); \
		strcpy((t)->x_names[z], (g)->x_names[z]); \
		(t)->x_regs[z] = alloc_lbuf("glob.x_reg"); \
		memcpy((t)->x_regs[z], (g)->x_regs[z], (g)->x_lens[z] + 1); \
		(t)->x_lens[z] = (g)->x_lens[z]; \
	    } \
	} \
    } \
    if (g) \
        (t)->dirty = (g)->dirty; \
    else \
        (t)->dirty = 0; \
}

/*
 * --------------------------------------------------------------------------
 * Module things.
 */

/*
 * Syntax: CALL_ALL_MODULES(<name of function>, (<args>)) Call all modules
 * defined for this symbol.
 */

#define CALL_ALL_MODULES(xfn,args) \
{ \
    MODULE *cam__mp; \
    for (cam__mp = mudstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next) { \
        if (cam__mp->xfn) { \
            (*(cam__mp->xfn))args; \
        } \
    } \
}

/*
 * Syntax: CALL_SOME_MODULES(<return value variable>, <name of function>,
 * (<args>)) Call modules in sequence until we get back a non-zero value.
 */

#define CALL_SOME_MODULES(rv,xfn,args) \
{ \
    MODULE *csm__mp; \
    for (csm__mp = mudstate.modules_list, rv = 0; (csm__mp != NULL) && !rv; csm__mp = csm__mp->next) { \
        if (csm__mp->xfn) { \
            rv = (*(csm__mp->xfn))args; \
        } \
    } \
}

/*
 * --------------------------------------------------------------------------
 * String things.
 */

/*
 * Copies a string, and sets its length, not including the terminating null
 * character, in another variable. Note that it is assumed that the source
 * string is null-terminated. Takes: string to copy to, string to copy from,
 * length (pointer to int)
 */

#define StrCopyLen(scl__dest,scl__src,scl__len) \
*(scl__len) = strlen(scl__src); \
memcpy(scl__dest,scl__src,(int) *(scl__len) + 1);

/*
 * Copies a string of known length, and null-terminates it. Takes: pointer to
 * copy to, pointer to copy from, length
 */

#define StrCopyKnown(scl__dest,scl__src,scl__len) \
memcpy(scl__dest,scl__src,scl__len); \
scl__dest[scl__len] = '\0';

/* Various macros for writing common string sequences. */

#define safe_crlf(b,p)		safe_known_str("\r\n",2,(b),(p))
#define safe_ansi_normal(b,p)	safe_known_str(ANSI_NORMAL,4,(b),(p))
#define safe_nothing(b,p)	safe_known_str("#-1",3,(b),(p))
#define safe_noperm(b,p)	safe_known_str("#-1 PERMISSION DENIED",21,(b),(p))
#define safe_nomatch(b,p)	safe_known_str("#-1 NO MATCH",12,(b),(p))
#define safe_bool(b,p,n)	safe_chr(((n) ? '1' : '0'),(b),(p))

#define safe_dbref(b,p,n) \
safe_chr('#',(b),(p)); \
safe_ltos((b),(p),(n));

#endif	/* __EXTERNS_H */
