/**
 * @file typedefs.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Type definitions for variables
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>

typedef int dbref;
typedef int FLAG;
typedef int POWER;

typedef struct tracemem_header
{
    size_t size;
    void *bptr;
    const char *file;
    int line;
    const char *function;
    const char *var;
    uint64_t *magic;
    struct tracemem_header *next;
} MEMTRACK;

typedef struct hookentry
{
    dbref thing;
    int atr;
} HOOKENT;

typedef struct key_linked_list
{
    char *name;
    int data;
    struct key_linked_list *next;
} KEYLIST;

typedef struct str_linked_list
{
    char *name;
    char *value;
    struct str_linked_list *next;
} LINKEDLIST;

typedef struct named_function
{
    char *fn_name;
    int (*handler)(dbref);
} NAMEDFUNC;

typedef struct external_funcs
{
    int num_funcs;
    NAMEDFUNC **ext_funcs;
} EXTFUNCS;

typedef struct global_register_data
{
    int q_alloc;
    char **q_regs;
    int *q_lens;
    int xr_alloc;
    char **x_names;
    char **x_regs;
    int *x_lens;
    int dirty;
} GDATA;

typedef struct bque BQUE; /*!< Command queue */
struct bque
{
    BQUE *next;              /*!< pointer to next command */
    dbref player;            /*!< player who will do command - halt is #-1 */
    dbref cause;             /*!< player causing command (for %N) */
    int pid;                 /*!< internal process ID */
    int waittime;            /*!< time to run command */
    dbref sem;               /*!< blocking semaphore */
    int attr;                /*!< blocking attribute */
    char *text;              /*!< buffer for comm, env, and scr text */
    char *comm;              /*!< command */
    char *env[NUM_ENV_VARS]; /*!< environment vars */
    GDATA *gdata;            /*!< temp vars */
    int nargs;               /*!< How many args I have */
};

/**
 * Return values for cf_ functions.
 *
 */
typedef enum CF_RESULT
{
    CF_Failure = -1,
    CF_Success,
    CF_Partial
} CF_Result;

/**
 * DB Related typedefs
 *
 */

typedef char boolexp_type;

typedef struct attr
{
    const char *name;                             /*!< This has to be first.  braindeath. */
    int number;                                   /*!< attr number */
    int flags;                                    /*!< attr flags */
    int (*check)(int, dbref, dbref, int, char *); /*!< check function */
} ATTR;

extern ATTR attr[];

extern ATTR **anum_table;

typedef struct boolexp BOOLEXP;
struct boolexp
{
    boolexp_type type;
    BOOLEXP *sub1;
    BOOLEXP *sub2;
    dbref thing; /*!< thing refers to an object */
};

typedef struct object
{
    dbref location;     /*!< PLAYER, THING: where it is */
                        /*!< ROOM: dropto: */
                        /*!< EXIT: where it goes to */
    dbref contents;     /*!< PLAYER, THING, ROOM: head of contentslist */
                        /*!< EXIT: unused */
    dbref exits;        /*!< PLAYER, THING, ROOM: head of exitslist */
                        /*!< EXIT: where it is */
    dbref next;         /*!< PLAYER, THING: next in contentslist */
                        /*!< EXIT: next in exitslist */
                        /*!< ROOM: unused */
    dbref link;         /*!< PLAYER, THING: home location */
                        /*!< ROOM, EXIT: unused */
    dbref parent;       /*!< ALL: defaults for attrs, exits, $cmds, */
    dbref owner;        /*!< PLAYER: domain number + class + moreflags */
                        /*!< THING, ROOM, EXIT: owning player number */
    dbref zone;         /*!< Whatever the object is zoned to. */
    FLAG flags;         /*!< ALL: Flags set on the object */
    FLAG flags2;        /*!< ALL: even more flags */
    FLAG flags3;        /*!< ALL: yet _more_ flags */
    POWER powers;       /*!< ALL: Powers on object */
    POWER powers2;      /*!< ALL: even more powers */
    time_t create_time; /*!< ALL: Time created (used in ObjID) */
    time_t last_access; /*!< ALL: Time last accessed */
    time_t last_mod;    /*!< ALL: Time last modified */
    /**
     * Make sure everything you want to write to the DBM database is in
     * the first part of the structure and included in DUMPOBJ
     *
     */
    int name_length;              /*!< ALL: Length of name string */
    int stack_count;              /*!< ALL: number of things on the stack */
    int vars_count;               /*!< ALL: number of variables */
    int struct_count;             /*!< ALL: number of structures */
    int instance_count;           /*!< ALL: number of struct instances */
    struct timeval cpu_time_used; /*!< ALL: CPU time eaten */
} OBJ;

/**
 * @brief The DUMPOBJ structure exists for use during database writes. It is a
 * duplicate of the OBJ structure except for items we don't need to write
 *
 */
// typedef struct dump_object DUMPOBJ;
typedef struct dump_object
{
    dbref location;     /*!< PLAYER, THING: where it is */
                        /*!< ROOM: dropto: */
                        /*!< EXIT: where it goes to */
    dbref contents;     /*!< PLAYER, THING, ROOM: head of contentslist */
                        /*!< EXIT: unused */
    dbref exits;        /*!< PLAYER, THING, ROOM: head of exitslist */
                        /*!< EXIT: where it is */
    dbref next;         /*!< PLAYER, THING: next in contentslist */
                        /*!< EXIT: next in exitslist */
                        /*!< ROOM: unused */
    dbref link;         /*!< PLAYER, THING: home location */
                        /*!< ROOM, EXIT: unused */
    dbref parent;       /*!< ALL: defaults for attrs, exits, $cmds, */
    dbref owner;        /*!< PLAYER: domain number + class + moreflags */
                        /*!< THING, ROOM, EXIT: owning player number */
    dbref zone;         /*!< Whatever the object is zoned to. */
    FLAG flags;         /*!< ALL: Flags set on the object */
    FLAG flags2;        /*!< ALL: even more flags */
    FLAG flags3;        /*!< ALL: yet _more_ flags */
    POWER powers;       /*!< ALL: Powers on object */
    POWER powers2;      /*!< ALL: even more powers */
    time_t create_time; /*!< ALL: Time created (used in ObjID) */
    time_t last_access; /*!< ALL: Time last accessed */
    time_t last_mod;    /*!< ALL: Time last modified */
} DUMPOBJ;

typedef char *NAME; /*!< Name type */

typedef struct logfiletable
{
    int log_flag;
    FILE *fileptr;
    char *filename;
} LOGFILETAB;

typedef struct numbertable
{
    int num;
} NUMBERTAB;

/**
 * @brief File cache related typedefs
 *
 */

typedef struct filecache_hdr FCACHE;
typedef struct filecache_block_hdr FBLKHDR;
typedef struct filecache_block FBLOCK;

struct filecache_hdr
{
    char **filename;
    FBLOCK *fileblock;
    const char *desc;
};

struct filecache_block
{
    struct filecache_block_hdr
    {
        struct filecache_block *nxt;
        int nchars;
    } hdr;
    char data[MBUF_SIZE - sizeof(FBLKHDR)];
};

/**
 * @brief Power related typedefs
 *
 */

/**
 * Information about object powers.
 *
 */
typedef struct power_entry
{
    const char *powername; /*!< Name of the flag */
    int powervalue;        /*!< Which bit in the object is the flag */
    int powerpower;        /*!< Ctrl flags for this power (recursive? :-) */
    int listperm;          /*!< Who sees this flag when set */
    int (*handler)();      /*!< Handler for setting/clearing this flag */
} POWERENT;

typedef struct powerset
{
    POWER word1; /*!< First word for power */
    POWER word2; /*!< Second word for power */
} POWERSET;

/**
 * @brief Flags related typedefs
 *
 */

/**
 * @brief Information about object flags.
 *
 */
typedef struct flag_entry
{
    const char *flagname; /*!< Name of the flag */
    int flagvalue;        /*!< Which bit in the object is the flag */
    char flaglett;        /*!< Flag letter for listing */
    int flagflag;         /*!< Ctrl flags for this flag (recursive? :-) */
    int listperm;         /*!< Who sees this flag when set */
    int (*handler)();     /*!< Handler for setting/clearing this flag */
} FLAGENT;

/**
 * @brief Fundamental object types
 *
 */
typedef struct object_entry
{
    const char *name;
    char lett;
    int perm;
    int flags;
} OBJENT;

typedef struct flagset
{
    FLAG word1;
    FLAG word2;
    FLAG word3;
} FLAGSET;

/**
 * @brief Functions related typedefs
 *
 */

typedef struct fun
{
    const char *name;   /*!< Function name */
    void (*fun)();      /*!< Handler */
    int nargs;          /*!< Number of args needed or expected */
    unsigned int flags; /*!< Function flags */
    int perms;          /*!< Access to function */
    EXTFUNCS *xperms;   /*!< Extended access to function */
} FUN;

typedef struct ufun
{
    char *name;         /*!< Function name */
    dbref obj;          /*!< Object ID */
    int atr;            /*!< Attribute ID */
    unsigned int flags; /*!< Function flags */
    int perms;          /*!< Access to function */
    struct ufun *next;  /*!< Next ufun in chain */
} UFUN;

typedef struct delim
{
    size_t len;
    char str[MAX_DELIM_LEN];
} Delim;

typedef struct var_entry
{
    char *text; /*!< variable text */
} VARENT;

typedef struct component_def
{
    int (*typer_func)(); /*!< type-checking handler */
    char *def_val;       /*!< default value */
} COMPONENT;

typedef struct structure_def
{
    char *s_name;        /*!< name of the structure */
    char **c_names;      /*!< array of component names */
    COMPONENT **c_array; /*!< array of pointers to components */
    int c_count;         /*!< number of components */
    char delim;          /*!< output delimiter when unloading */
    int need_typecheck;  /*!< any components without types of any? */
    int n_instances;     /*!< number of instances out there */
    char *names_base;    /*!< pointer for later freeing */
    char *defs_base;     /*!< pointer for later freeing */
} STRUCTDEF;

typedef struct instance_def
{
    STRUCTDEF *datatype; /*!< pointer to structure data type def */
} INSTANCE;

typedef struct data_def
{
    char *text;
} STRUCTDATA;

typedef struct object_stack OBJSTACK;
struct object_stack
{
    char *data;
    OBJSTACK *next;
};

typedef struct object_grid
{
    int rows;
    int cols;
    char ***data;
} OBJGRID;

typedef struct object_xfuncs
{
    NAMEDFUNC **func;
    int count;
} OBJXFUNCS;

/**
 * @brief Search structure, used by @search and search().
 *
 */
typedef struct search_type
{
    int s_wizard;
    dbref s_owner;
    dbref s_rst_owner;
    int s_rst_type;
    FLAGSET s_fset;
    POWERSET s_pset;
    dbref s_parent;
    dbref s_zone;
    char *s_rst_name;
    char *s_rst_eval;
    char *s_rst_ufuntxt;
    int low_bound;
    int high_bound;
} SEARCH;

/**
 * * @brief Stats structure, used by @stats and stats().
 *
 */
typedef struct stats_type
{
    int s_total;
    int s_rooms;
    int s_exits;
    int s_things;
    int s_players;
    int s_going;
    int s_garbage;
    int s_unknown;
} STATS;

/**
 * @brief Help related typedefs
 *
 */

typedef struct
{
    long pos;                       /*!< index into help file */
    int len;                        /*!< length of help entry */
    char topic[TOPIC_NAME_LEN + 1]; /*!< topic of help entry */
} help_indx;

/**
 * Pointers to this struct is what gets stored in the help_htab's
 *
 */
struct help_entry
{
    int pos; /*!< Position, copied from help_indx */
    int len; /*!< Length of entry, copied from help_indx */
};

typedef struct _help_indx_list
{
    help_indx entry;
    struct _help_indx_list *next;
} help_indx_list;

/**
 * @brief htab related typedefs
 *
 */

typedef union
{
    char *s;
    int i;
} HASHKEY;

typedef struct hashentry
{
    HASHKEY target;
    int *data;
    int flags;
    struct hashentry *next;
} HASHENT;

typedef struct hashtable
{
    int hashsize;
    int mask;
    int checks;
    int scans;
    int max_scan;
    int hits;
    int entries;
    int deletes;
    int nulls;
    int flags;
    HASHENT **entry;
    int last_hval;       /*!< Used for hashfirst & hashnext. */
    HASHENT *last_entry; /*!< like last_hval */
} HASHTAB;

typedef struct mod_hashes
{
    char *tabname;
    HASHTAB *htab;
    int size_factor;
    int min_size;
} MODHASHES;

/**
 * Definititon of a name table
 *
 */
typedef struct name_table
{
    char *name; //!< Name of the entry
    int minlen; //!< Minimum length of the entry
    int perm;   //!< Permissions
    int flag;   //!< Flags
} NAMETAB;

/**
 * Command related typedefs
 *
 */

typedef struct addedentry
{
    dbref thing;
    int atr;
    char *name;
    struct addedentry *next;
} ADDENT;

typedef struct cmdentry
{
    char *cmdname;
    NAMETAB *switches;
    int perms;
    int extra;
    int callseq;
    HOOKENT *userperms;
    HOOKENT *pre_hook;
    HOOKENT *post_hook;
    union
    {
        void (*handler)();
        ADDENT *added;
    } info;
} CMDENT;

/**
 * @brief Interface related typedefs
 *
 */

typedef struct cmd_block_hdr
{
    struct cmd_block *nxt;
} CBLKHDR;

typedef struct cmd_block
{
    CBLKHDR hdr;
    char cmd[LBUF_SIZE - sizeof(CBLKHDR)];
} CBLK;

typedef struct text_block_hdr
{
    struct text_block *nxt;
    char *start;
    char *end;
    int nchars;
} TBLKHDR;

typedef struct text_block
{
    TBLKHDR hdr;
    char *data;
} TBLOCK;

typedef struct prog_data
{
    dbref wait_cause;
    GDATA *wait_data;
} PROG;

typedef struct descriptor_data
{
    int descriptor;
    int flags;
    int retries_left;
    int command_count;
    int timeout;
    int host_info;
    char addr[51];
    char username[11];
    char *doing;
    dbref player;
    int *colormap;
    char *output_prefix;
    char *output_suffix;
    int output_size;
    int output_tot;
    int output_lost;
    TBLOCK *output_head;
    TBLOCK *output_tail;
    int input_size;
    int input_tot;
    int input_lost;
    CBLK *input_head;
    CBLK *input_tail;
    CBLK *raw_input;
    char *raw_input_at;
    time_t connected_at;
    time_t last_time;
    int quota;
    PROG *program_data;
    struct sockaddr_in address;
    struct descriptor_data *hashnext;
    struct descriptor_data *next;
    struct descriptor_data **prev;
} DESC;

/**
 * @brief UDB related typedefs
 *
 */

/**
 * For MUSH, an int works great as an object ID And attributes are zero
 * terminated strings, so we heave the size out. We hand around attribute
 * identifiers in the last things.
 *
 */
typedef struct udb_aname
{
    unsigned int object;
    unsigned int attrnum;
} UDB_ANAME;

/**
 * In general, we want binary attributes, so we do this.
 *
 */
typedef struct udb_attrib
{
    int attrnum; /*!< MUSH specific identifier */
    int size;
    char *data;
} UDB_ATTRIB;

/**
 * An object is a name, an attribute count, and a vector of attributes which
 * Attr's are stowed in a contiguous array pointed at by atrs.
 *
 */
typedef struct udb_object
{
    unsigned int name;
    time_t counter;
    int dirty;
    int at_count;
    UDB_ATTRIB *atrs;
} UDB_OBJECT;

typedef struct udb_cache
{
    void *keydata;
    int keylen;
    void *data;
    int datalen;
    unsigned int type;
    unsigned int flags;
    struct udb_cache *nxt;
    struct udb_cache *prvfree;
    struct udb_cache *nxtfree;
} UDB_CACHE;

typedef struct udb_chain
{
    UDB_CACHE *head;
    UDB_CACHE *tail;
} UDB_CHAIN;

typedef struct udb_data
{
    void *dptr;
    int dsize;
} UDB_DATA;

/**
 * @brief Match related typedefs
 *
 */

typedef struct match_state
{
    int confidence;      /*!< How confident are we?  CON_xx */
    int count;           /*!< # of matches at this confidence */
    int pref_type;       /*!< The preferred object type */
    int check_keys;      /*!< Should we test locks? */
    dbref absolute_form; /*!< If #num, then the number */
    dbref match;         /*!< What I've found so far */
    dbref player;        /*!< Who is performing match */
    char *string;        /*!< The string to search for */
} MSTATE;

/**
 * @brief MUSH Configuration related typedefs
 *
 */

/**
 * Modules and related things.
 *
 */
typedef struct module_version_info
{
    char *version;
    char *author;
    char *email;
    char *url;
    char *description;
    char *copyright;
} MODVER;

typedef struct module_linked_list
{
    char *modname;
    void *handle;
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
    void (*cache_put_notify)(UDB_DATA, unsigned int);
    void (*cache_del_notify)(UDB_DATA, unsigned int);
    MODVER(*version)
    (dbref, dbref, int);
} MODULE;

typedef struct api_function_data
{
    const char *name;
    const char *param_fmt;
    void (*handler)(void *, void *);
} API_FUNCTION;

typedef union
{
    long double d;
    unsigned int u[FP_SIZE];
} fp_union_uint;

/**
 * Runtime configurable parameters
 *
 */
typedef struct confparm
{
    char *pname;          /*!< parm name */
    int (*interpreter)(); /*!< routine to interp parameter */
    int flags;            /*!< control flags */
    int rperms;           /*!< read permission flags */
    int *loc;             /*!< where to store value */
    long extra;           /*!< extra data for interpreter */
} CONF;

typedef struct confdata
{
    int cache_size;            /*!< Maximum size of cache */
    int cache_width;           /*!< Number of cache cells */
    int paylimit;              /*!< getting money gets hard over this much */
    int digcost;               /*!< cost of @dig command */
    int linkcost;              /*!< cost of @link command */
    int opencost;              /*!< cost of @open command */
    int robotcost;             /*!< cost of @robot command */
    int createmin;             /*!< default (and minimum) cost of @create cmd */
    int createmax;             /*!< max cost of @create command */
    int quotas;                /*!< TRUE = have building quotas */
    int room_quota;            /*!< quota needed to make a room */
    int exit_quota;            /*!< quota needed to make an exit */
    int thing_quota;           /*!< quota needed to make a thing */
    int player_quota;          /*!< quota needed to make a robot player */
    int sacfactor;             /*!< sacrifice earns (obj_cost/sfactor) + sadj */
    int sacadjust;             /*!< ... */
    dbref start_room;          /*!< initial location for non-Guest players */
    dbref start_home;          /*!< initial HOME for players */
    dbref default_home;        /*!< HOME when home is inaccessible */
    dbref guest_start_room;    /*!< initial location for Guests */
    int vattr_flags;           /*!< Attr flags for all user-defined attrs */
    KEYLIST *vattr_flag_list;  /*!< Linked list, for attr_type conf */
    int log_options;           /*!< What gets logged */
    int log_info;              /*!< Info that goes into log entries */
    int log_diversion;         /*!< What logs get diverted? */
    uint8_t markdata[8];       /*!< Masks for marking/unmarking */
    int ntfy_nest_lim;         /*!< Max nesting of notifys */
    int fwdlist_lim;           /*!< Max objects in @forwardlist */
    int propdir_lim;           /*!< Max objects in @propdir */
    int dbopt_interval;        /*!< Optimize db every N dumps */
    char *dbhome;              /*!< Database home directory */
    char *txthome;             /*!< Text files home directory */
    char *binhome;             /*!< Binary home directory */
    char *bakhome;             /*!< Backup home directory */
    char *status_file;         /*!< Where to write arg to @shutdown */
    char *config_file;         /*!< MUSH's config file */
    char *config_home;         /*!< MUSH's config directory */
    char *log_file;            /*!< MUSH's log file */
    char *log_home;            /*!< MUSH's log directory */
    char *pid_file;            /*!< MUSH's pid file */
    char *pid_home;            /*!< MUSH's pid directory */
    char *db_file;             /*!< MUSH's db file */
    char *backup_exec;         /*!< Executable run to tar files */
    char *backup_compress;     /*!< Flags used to compress */
    char *backup_extract;      /*!< Flags used to extract */
    char *backup_ext;          /*!< Filename extension for backup */
    char *mush_owner;          /*!< Email of the game owner */
    char *modules_home;        /*!< Base path for modules */
    char *game_exec;           /*!< MUSH's executable full path and name */
    char *game_home;           /*!< MUSH's working directory */
    char *scripts_home;        /*!< MUSH's scripts directory */
    int have_pueblo;           /*!< Is Pueblo support compiled in? */
    int have_zones;            /*!< Should zones be active? */
    int port;                  /*!< user port */
    int conc_port;             /*!< concentrator port */
    int init_size;             /*!< initial db size */
    int output_block_size;     /*!< Block size of output */
    int use_global_aconn;      /*!< Do we want to use global @aconn code? */
    int global_aconn_uselocks; /*!< global @aconn obeys uselocks? */
    int have_guest;            /*!< Do we wish to allow a GUEST character? */
    int guest_char;            /*!< player num of prototype GUEST character */
    int guest_nuker;           /*!< Wiz who nukes the GUEST characters. */
    int number_guests;         /*!< number of guest characters allowed */
    char *guest_basename;      /*!< Base name or alias for guest char */
    char *guest_prefixes;      /*!< Prefixes for the guest char's name */
    char *guest_suffixes;      /*!< Suffixes for the guest char's name */
    char *guest_password;      /*!< Default password for guests */
    char *help_users;          /*!< Help file for users */
    char *help_wizards;        /*!< Help file for wizards and god */
    char *help_quick;          /*!< Quick help file */
    char *guest_file;          /*!< display if guest connects */
    char *conn_file;           /*!< display on connect if no registration */
    char *creg_file;           /*!< display on connect if registration */
    char *regf_file;           /*!< display on (failed) create if reg is on */
    char *motd_file;           /*!< display this file on login */
    char *wizmotd_file;        /*!< display this file on login to wizards */
    char *quit_file;           /*!< display on quit */
    char *down_file;           /*!< display this file if no logins */
    char *full_file;           /*!< display when max users exceeded */
    char *site_file;           /*!< display if conn from bad site */
    char *crea_file;           /*!< display this on login for new users */
    char *motd_msg;            /*!< Wizard-settable login message */
    char *wizmotd_msg;         /*!< Login message for wizards only */
    char *downmotd_msg;        /*!< Settable 'logins disabled' message */
    char *fullmotd_msg;        /*!< Settable 'Too many players' message */
    char *dump_msg;            /*!< Message displayed when @dump-ing */
    char *postdump_msg;        /*!< Message displayed after @dump-ing */
    char *fixed_home_msg;      /*!< Message displayed when going home and FIXED */
    char *fixed_tel_msg;       /*!< Message displayed when teleporting and FIXED */
    char *huh_msg;             /*!< Message displayed when a Huh? is gotten */
    char *pueblo_msg;          /*!< Message displayed to Pueblo clients */
    char *pueblo_version;      /*!< Message displayed to indicate Pueblo's capabilities */
    char *htmlconn_file;       /*!< display on PUEBLOCLIENT message */
    char *exec_path;           /*!< argv[0] */
    LINKEDLIST *infotext_list; /*!< Linked list of INFO fields and values */
    int indent_desc;           /*!< Newlines before and after descs? */
    int name_spaces;           /*!< allow player names to have spaces */
    int site_chars;            /*!< where to truncate site name */
    int fork_dump;             /*!< perform dump in a forked process */
    int sig_action;            /*!< What to do with fatal signals */
    int max_players;           /*!< Max # of connected players */
    int dump_interval;         /*!< interval between checkpoint dumps in secs */
    int check_interval;        /*!< interval between db check/cleans in secs */
    int events_daily_hour;     /*!< At what hour should @daily be executed? */
    int dump_offset;           /*!< when to take first checkpoint dump */
    int check_offset;          /*!< when to perform first check and clean */
    int idle_timeout;          /*!< Boot off players idle this long in secs */
    int conn_timeout;          /*!< Allow this long to connect before booting */
    int idle_interval;         /*!< when to check for idle users */
    int retry_limit;           /*!< close conn after this many bad logins */
    int output_limit;          /*!< Max # chars queued for output */
    int paycheck;              /*!< players earn this much each day connected */
    int paystart;              /*!< new players start with this much money */
    int start_quota;           /*!< Quota for new players */
    int start_room_quota;      /*!< Room quota for new players */
    int start_exit_quota;      /*!< Exit quota for new players */
    int start_thing_quota;     /*!< Thing quota for new players */
    int start_player_quota;    /*!< Player quota for new players */
    int payfind;               /*!< chance to find a penny with wandering */
    int killmin;               /*!< default (and minimum) cost of kill cmd */
    int killmax;               /*!< max cost of kill command */
    int killguarantee;         /*!< cost of kill cmd that guarantees success */
    int pagecost;              /*!< cost of @page command */
    int searchcost;            /*!< cost of commands that search the whole DB */
    int waitcost;              /*!< cost of @wait (refunded when finishes) */
    int building_limit;        /*!< max number of objects in the db */
    int queuemax;              /*!< max commands a player may have in queue */
    int queue_chunk;           /*!< # cmds to run from queue when idle */
    int active_q_chunk;        /*!< # cmds to run from queue when active */
    int machinecost;           /*!< One in mc+1 cmds costs 1 penny (POW2-1) */
    int clone_copy_cost;       /*!< Does @clone copy value? */
    int use_hostname;          /*!< TRUE = use machine NAME rather than quad */
    int typed_quotas;          /*!< TRUE = use quotas by type */
    int ex_flags;              /*!< TRUE = show flags on examine */
    int robot_speak;           /*!< TRUE = allow robots to speak in public */
    int pub_flags;             /*!< TRUE = flags() works on anything */
    int quiet_look;            /*!< TRUE = don't see attribs when looking */
    int exam_public;           /*!< Does EXAM show public attrs by default? */
    int read_rem_desc;         /*!< Can the DESCs of nonlocal objs be read? */
    int read_rem_name;         /*!< Can the NAMEs of nonlocal objs be read? */
    int sweep_dark;            /*!< Can you sweep dark places? */
    int player_listen;         /*!< Are AxHEAR triggered on players? */
    int quiet_whisper;         /*!< Can others tell when you whisper? */
    int dark_sleepers;         /*!< Are sleeping players 'dark'? */
    int see_own_dark;          /*!< Do you see your own dark stuff? */
    int idle_wiz_dark;         /*!< Do idling wizards get set dark? */
    int visible_wizzes;        /*!< Do dark wizards show up on contents? */
    int pemit_players;         /*!< Can you @pemit to faraway players? */
    int pemit_any;             /*!< Can you @pemit to ANY remote object? */
    int addcmd_match_blindly;  /*!< Does @addcommand produce a Huh? if syntax issues mean no wildcard is matched? */
    int addcmd_obey_stop;      /*!< Does @addcommand still multiple match on STOP objs? */
    int addcmd_obey_uselocks;  /*!< Does @addcommand obey uselocks? */
    int lattr_oldstyle;        /*!< Bad lattr() return empty or #-1 NO MATCH? */
    int bools_oldstyle;        /*!< TinyMUSH 2.x and TinyMUX bools */
    int match_mine;            /*!< Should you check yourself for $-commands? */
    int match_mine_pl;         /*!< Should players check selves for $-cmds? */
    int switch_df_all;         /*!< Should @switch match all by default? */
    int fascist_objeval;       /*!< Does objeval() require victim control? */
    int fascist_tport;         /*!< Src of teleport must be owned/JUMP_OK */
    int terse_look;            /*!< Does manual look obey TERSE */
    int terse_contents;        /*!< Does TERSE look show exits */
    int terse_exits;           /*!< Does TERSE look show obvious exits */
    int terse_movemsg;         /*!< Show move msgs (SUCC/LEAVE/etc) if TERSE? */
    int trace_topdown;         /*!< Is TRACE output top-down or bottom-up? */
    int safe_unowned;          /*!< Are objects not owned by you safe? */
    int trace_limit;           /*!< Max lines of trace output if top-down */
    int wiz_obey_linklock;     /*!< Do wizards obey linklocks? */
    int local_masters;         /*!< Do we check Zone rooms as local masters? */
    int match_zone_parents;    /*!< Objects in local master rooms inherit commands from their parents, just like normal? */
    int req_cmds_flag;         /*!< COMMANDS flag required to check $-cmds? */
    int ansi_colors;           /*!< allow ANSI colors? */
    int safer_passwords;       /*!< enforce reasonably good password choices? */
    int space_compress;        /*!< Convert multiple spaces into one space */
    int instant_recycle;       /*!< Do destroy_ok objects get insta-nuke? */
    int dark_actions;          /*!< Trigger @a-actions even when dark? */
    int no_ambiguous_match;    /*!< match_result() -> last_match_result() */
    int exit_calls_move;       /*!< Matching an exit in the main command parser invokes the 'move' command. */
    int move_match_more;       /*!< Exit matches in 'move' parse like the main command parser (local, global, zone; pick random on ambiguous). */
    int autozone;              /*!< New objs are zoned to creator's zone */
    int page_req_equals;       /*!< page command must always contain '=' */
    int comma_say;             /*!< Use grammatically-correct comma in says */
    int you_say;               /*!< Show 'You say' to the player */
    int c_cmd_subst;           /*!< Is %c last command or ansi? */
    int player_name_min;       /*!< Minimum length of a player name */
    dbref master_room;         /*!< Room containing default cmds/exits/etc */
    dbref player_proto;        /*!< Player prototype to clone */
    dbref room_proto;          /*!< Room prototype to clone */
    dbref exit_proto;          /*!< Exit prototype to clone */
    dbref thing_proto;         /*!< Thing prototype to clone */
    dbref player_defobj;       /*!< Players use this for attr template */
    dbref room_defobj;         /*!< Rooms use this for attr template */
    dbref exit_defobj;         /*!< Exits use this for attr template */
    dbref thing_defobj;        /*!< Things use this for attr template */
    dbref player_parent;       /*!< Parent that players start with */
    dbref room_parent;         /*!< Parent that rooms start with */
    dbref exit_parent;         /*!< Parent that exits start with */
    dbref thing_parent;        /*!< Parent that things start with */
    FLAGSET player_flags;      /*!< Flags players start with */
    FLAGSET room_flags;        /*!< Flags rooms start with */
    FLAGSET exit_flags;        /*!< Flags exits start with */
    FLAGSET thing_flags;       /*!< Flags things start with */
    FLAGSET robot_flags;       /*!< Flags robots start with */
    FLAGSET stripped_flags;    /*!< Flags stripped by @clone and @chown */
    char *flag_sep;            /*!< Separator of dbref from marker flags */
    char *mush_name;           /*!< Name of the Mush */
    char *mush_shortname;      /*!< Shorter name, for log */
    char *one_coin;            /*!< name of one coin (ie. "penny") */
    char *many_coins;          /*!< name of many coins (ie. "pennies") */
    int timeslice;             /*!< How often do we bump people's cmd quotas? */
    int cmd_quota_max;         /*!< Max commands at one time */
    int cmd_quota_incr;        /*!< Bump #cmds allowed by this each timeslice */
    int lag_check;             /*!< Is CPU usage checking compiled in? */
    int lag_check_clk;         /*!< track object use time with wall-clock (need lag_check == true) */
    int lag_check_cpu;         /*!< track object use time with getrusage() instead of wall-clock (need lag_check_clk == true) */
    int malloc_logger;         /*!< log allocation of memory */
    int max_global_regs;       /*!< How many global register are avalable (min 10, max 36) */
    int max_command_args;      /*!< Maximum arguments a command may have */
    int player_name_length;    /*!< Maximum length of a player name */
    int hash_factor;           /*!< Hash factor */
    int max_cmdsecs;           /*!< Threshhold for real time taken by command */
    int control_flags;         /*!< Global runtime control flags */
    int wild_times_lim;        /*!< Max recursions in wildcard match */
    int cmd_nest_lim;          /*!< Max nesting of commands like @switch/now */
    int cmd_invk_lim;          /*!< Max commands in one queue entry */
    int func_nest_lim;         /*!< Max nesting of functions */
    int func_invk_lim;         /*!< Max funcs invoked by a command */
    int func_cpu_lim_secs;     /*!< Max secs of func CPU time by a cmd */
    clock_t func_cpu_lim;      /*!< Max clock ticks of func CPU time by a cmd */
    int lock_nest_lim;         /*!< Max nesting of lock evals */
    int parent_nest_lim;       /*!< Max levels of parents */
    int zone_nest_lim;         /*!< Max nesting of zones */
    int numvars_lim;           /*!< Max number of variables per object */
    int stack_lim;             /*!< Max number of items on an object stack */
    int struct_lim;            /*!< Max number of defined structures for obj */
    int instance_lim;          /*!< Max number of struct instances for obj */
    int max_grid_size;         /*!< Maximum cells in a grid */
    int max_player_aliases;    /*!< Max number of aliases for a player */
    int register_limit;        /*!< Max number of named q-registers */
    int max_qpid;              /*!< Max total number of queue entries */
    char *struct_dstr;         /*!< Delim string used for struct 'examine' */
} CONFDATA;

typedef struct site_data
{
    struct site_data *next; /*!< Next site in chain */
    struct in_addr address; /*!< Host or network address */
    struct in_addr mask;    /*!< Mask to apply before comparing */
    int flag;               /*!< Value to return on match */
} SITE;

typedef struct objlist_block OBLOCK;
struct objlist_block
{
    struct objlist_block *next;
    dbref data[(LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref)];
};

typedef struct objlist_stack
{
    struct objlist_stack *next; /*!< Next object list in stack */
    OBLOCK *head;               /*!< Head of object list */
    OBLOCK *tail;               /*!< Tail of object list */
    OBLOCK *cblock;             /*!< Current block for scan */
    int count;                  /*!< Number of objs in last obj list block */
    dbref citm;                 /*!< Current item for scan */
} OLSTK;

typedef struct markbuf
{
    char chunk[5000];
} MARKBUF;

typedef struct alist
{
    char *data;
    int len;
    struct alist *next;
} ALIST;

typedef struct badname_struc
{
    char *name;
    struct badname_struc *next;
} BADNAME;

typedef struct forward_list
{
    int count;
    int *data;
} FWDLIST;

typedef struct propdir_list
{
    int count;
    int *data;
} PROPDIR;

/**
 * Version number is Major.Minor.Status.PatchLevel
 *
 */
typedef struct
{
    char *name;   /*!< Complete version string */
    int major;    /*!< Major Version */
    int minor;    /*!< Minor Version */
    int status;   /*!< Status : 0 - Alpha, 1 - Beta, 2 - Release Candidate, 3 - Gamma */
    int revision; /*!< Patch Level */
} versioninfo;

typedef struct statedata
{
    int record_players;                 /*!< The maximum # of player logged on */
    int db_block_size;                  /*!< Block size of database */
    UDB_OBJECT *objpipes[NUM_OBJPIPES]; /*!< Number of object pipelines */
    unsigned int objc;                  /*!< Object reference counter */
    versioninfo version;                /*!< MUSH version info */
    // char *configureinfo;                /*!< Configure switches */
    // char *compilerinfo;                 /*!< Compiler command line */
    // char *linkerinfo;                   /*!< Linker command line */
    char *modloaded;         /*!< Modules loaded */
    char **cfiletab;         /*!< Array of config files */
    int configfiles;         /*!< Number of config files */
    int initializing;        /*!< Are we reading config file at startup? */
    int loading_db;          /*!< Are we loading the db? */
    int standalone;          /*!< Are we converting the database? */
    int panicking;           /*!< Are we in the middle of dying horribly? */
    int restarting;          /*!< Are we restarting? */
    int dumping;             /*!< Are we dumping? */
    int logstderr;           /*!< Echo log to stderr too? */
    int debug;               /*!< Are we being debug? */
    pid_t dumper;            /*!< If forked-dumping, with what pid? */
    int logging;             /*!< Are we in the middle of logging? */
    int epoch;               /*!< Generation number for dumps */
    int generation;          /*!< DB global generation number */
    int mush_lognum;         /*!< Number of logfile */
    int helpfiles;           /*!< Number of external indexed helpfiles */
    int hfiletab_size;       /*!< Size of the table storing path pointers */
    char **hfiletab;         /*!< Array of path pointers */
    HASHTAB *hfile_hashes;   /*!< Pointer to an array of index hashtables */
    dbref curr_enactor;      /*!< Who initiated the current command */
    dbref curr_player;       /*!< Who is running the current command */
    char *curr_cmd;          /*!< The current command */
    int alarm_triggered;     /*!< Has periodic alarm signal occurred? */
    time_t now;              /*!< What time is it now? */
    time_t dump_counter;     /*!< Countdown to next db dump */
    time_t check_counter;    /*!< Countdown to next db check */
    time_t idle_counter;     /*!< Countdown to next idle check */
    time_t mstats_counter;   /*!< Countdown to next mstats snapshot */
    time_t events_counter;   /*!< Countdown to next events check */
    int shutdown_flag;       /*!< Should interface be shut down? */
    int flatfile_flag;       /*!< Dump a flatfile when we have the chance */
    time_t start_time;       /*!< When was MUSH started */
    time_t restart_time;     /*!< When did we last restart? */
    int reboot_nums;         /*!< How many times have we restarted? */
    time_t cpu_count_from;   /*!< When did we last reset CPU counters? */
    char *debug_cmd;         /*!< The command we are executing (if any) */
    char *doing_hdr;         /*!< Doing column header in WHO display */
    SITE *access_list;       /*!< Access states for sites */
    SITE *suspect_list;      /*!< Sites that are suspect */
    HASHTAB command_htab;    /*!< Commands hashtable */
    HASHTAB logout_cmd_htab; /*!< Logged-out commands hashtable (WHO, etc) */
    HASHTAB func_htab;       /*!< Functions hashtable */
    HASHTAB ufunc_htab;      /*!< Local functions hashtable */
    HASHTAB powers_htab;     /*!< Powers hashtable */
    HASHTAB flags_htab;      /*!< Flags hashtable */
    HASHTAB attr_name_htab;  /*!< Attribute names hashtable */
    HASHTAB vattr_name_htab; /*!< User attribute names hashtable */
    HASHTAB player_htab;     /*!< Player name->number hashtable */
    HASHTAB nref_htab;       /*!< Object name reference #_name_ mapping */
    HASHTAB desc_htab;       /*!< Socket descriptor hashtable */
    HASHTAB fwdlist_htab;    /*!< Room forwardlists */
    HASHTAB propdir_htab;    /*!< Propdir lists */
    HASHTAB qpid_htab;       /*!< Queue process IDs */
    HASHTAB redir_htab;      /*!< Redirections */
    HASHTAB objstack_htab;   /*!< Object stacks */
    HASHTAB objgrid_htab;    /*!< Object grids */
    HASHTAB parent_htab;     /*!< Parent $-command exclusion */
    HASHTAB vars_htab;       /*!< Persistent variables hashtable */
    HASHTAB structs_htab;    /*!< Structure hashtable */
    HASHTAB cdefs_htab;      /*!< Components hashtable */
    HASHTAB instance_htab;   /*!< Instances hashtable */
    HASHTAB instdata_htab;   /*!< Structure data hashtable */
    HASHTAB api_func_htab;   /*!< Registered module API functions */
    MODULE *modules_list;    /*!< Loadable modules hashtable */
    int max_structs;
    int max_cdefs;
    int max_instance;
    int max_instdata;
    int max_stacks;
    int max_vars;
    int attr_next;                       /*!< Next attr to alloc when freelist is empty */
    BQUE *qfirst;                        /*!< Head of player queue */
    BQUE *qlast;                         /*!< Tail of player queue */
    BQUE *qlfirst;                       /*!< Head of object queue */
    BQUE *qllast;                        /*!< Tail of object queue */
    BQUE *qwait;                         /*!< Head of wait queue */
    BQUE *qsemfirst;                     /*!< Head of semaphore queue */
    BQUE *qsemlast;                      /*!< Tail of semaphore queue */
    BADNAME *badname_head;               /*!< List of disallowed names */
    int mstat_ixrss[2];                  /*!< Summed shared size */
    int mstat_idrss[2];                  /*!< Summed private data size */
    int mstat_isrss[2];                  /*!< Summed private stack size */
    int mstat_secs[2];                   /*!< Time of samples */
    int mstat_curr;                      /*!< Which sample is latest */
    ALIST iter_alist;                    /*!< Attribute list for iterations */
    char *mod_alist;                     /*!< Attribute list for modifying */
    int mod_size;                        /*!< Length of modified buffer */
    dbref mod_al_id;                     /*!< Where did mod_alist come from? */
    OLSTK *olist;                        /*!< Stack of object lists for nested searches */
    dbref freelist;                      /*!< Head of object freelist */
    int min_size;                        /*!< Minimum db size (from file header) */
    int db_top;                          /*!< Number of items in the db */
    int db_size;                         /*!< Allocated size of db structure */
    unsigned int moduletype_top;         /*!< Highest module DBTYPE */
    int *guest_free;                     /*!< Table to keep track of free guests */
    MARKBUF *markbits;                   /*!< temp storage for marking/unmarking */
    int in_loop;                         /*!< In a loop() statement? */
    char *loop_token[MAX_ITER_NESTING];  /*!< Value of ## */
    char *loop_token2[MAX_ITER_NESTING]; /*!< Value of #? */
    int loop_number[MAX_ITER_NESTING];   /*!< Value of #@ */
    int loop_break[MAX_ITER_NESTING];    /*!< Kill this iter() loop? */
    int in_switch;                       /*!< In a switch() statement? */
    char *switch_token;                  /*!< Value of #$ */
    int func_nest_lev;                   /*!< Current nesting of functions */
    int func_invk_ctr;                   /*!< Functions invoked so far by this command */
    int ntfy_nest_lev;                   /*!< Current nesting of notifys */
    int lock_nest_lev;                   /*!< Current nesting of lock evals */
    int cmd_nest_lev;                    /*!< Current nesting of commands like @sw/now */
    int cmd_invk_ctr;                    /*!< Commands invoked so far by this qentry */
    int wild_times_lev;                  /*!< Wildcard matching tries. */
    GDATA *rdata;                        /*!< Global register data */
    int zone_nest_num;                   /*!< Global current zone nest position */
    int break_called;                    /*!< Boolean flag for @break and @assert */
    int f_limitmask;                     /*!< Flagword for limiter for functions */
    int inpipe;                          /*!< Boolean flag for command piping */
    char *pout;                          /*!< The output of the pipe used in %| */
    char *poutnew;                       /*!< The output being build by the current command */
    char *poutbufc;                      /*!< Buffer position for poutnew */
    dbref poutobj;                       /*!< Object doing the piping */
    clock_t cputime_base;                /*!< CPU baselined at beginning of command */
    clock_t cputime_now;                 /*!< CPU time recorded during command */
    const unsigned char *retabs;         /*!< PCRE regexp tables */
    MEMTRACK *raw_allocs;                /*!< Tracking of raw memory allocations */
    int dbm_fd;                          /*!< File descriptor of our DBM database */
} STATEDATA;

/**
 * @brief Player related typedefs
 *
 */

typedef struct hostdtm
{
    char *host;
    char *dtm;
} HOSTDTM;

typedef struct logindata
{
    HOSTDTM good[NUM_GOOD];
    HOSTDTM bad[NUM_BAD];
    int tot_good;
    int tot_bad;
    int new_bad;
} LDATA;

typedef struct player_cache
{
    dbref player;
    int money;
    int queue;
    int qmax;
    int cflags;
    struct player_cache *next;
} PCACHE;

/**
 * @brief User attributes related typedefs
 *
 */

typedef struct user_attribute
{
    char *name; /*!< Name of user attribute */
    int number; /*!< Assigned attribute number */
    int flags;  /*!< Attribute flags */
} VATTR;

/**
 * @brief PGC related typedefs
 *
 */
typedef struct pcg_state_setseq_64
{
    uint64_t state; /*!< RNG state.  All values are possible. */
    uint64_t inc;   /*!< Controls which RNG sequence (stream) is */
                    /*!< selected. Must *always* be odd. */
} pcg32_random_t;

/**
 * @brief Message queue related typedefs
 *
 */

/**
 * @brief Type of messages for the task handler
 *
 */
typedef enum msgq_destination
{
    MSGQ_DEST_DNSRESOLVER = 1L, /*!< DNS Resolver */
    MSGQ_DEST_REPLY = __LONG_MAX__
} MSGQ_DESTINATION;

typedef struct msgq_dnsresolver
{
    long destination;
    struct
    {
        union
        {
            struct in_addr v4;
            struct in6_addr v6;
        } ip;
        int addrf;
        in_port_t port;
        char *hostname;
    } payload;
} MSGQ_DNSRESOLVER;

/**
 * @brief Sort list related typedefs
 *
 */

typedef struct f_record
{
    double data;
    char *str;
    int pos;
} F_RECORD;

typedef struct i_record
{
    long data;
    char *str;
    int pos;
} I_RECORD;

typedef struct a_record
{
    char *str;
    int pos;
} A_RECORD;

/**
 * @brief Time conversion related typedefs
 *
 */
typedef struct monthdays
{
    char *month;
    int day;
} MONTHDAYS;

/**
 * @brief Ansi and color conversion typedefs
 *
 */

typedef struct
{
    float x;
    float y;
    float z;
} xyzColor;

typedef struct
{
    float l;
    float a;
    float b;
} CIELABColor;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgbColor;

typedef struct
{
    char *name;
    rgbColor rgb;
} COLORINFO;

typedef struct
{
    float deltaE;
    COLORINFO color;
} COLORMATCH;

#endif /* __TYPEDEFS_H */
