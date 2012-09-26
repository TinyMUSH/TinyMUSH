/* command.h - declarations used by the command processor */

#include "copyright.h"

#ifndef __COMMAND_H
#define __COMMAND_H

#define CMD_NO_ARG(name) extern void name(dbref, dbref, int)
#define CMD_ONE_ARG(name) extern void name(dbref, dbref, int, char *)
#define CMD_ONE_ARG_CMDARG(name) extern void name(dbref, dbref, int, char *, char *[], int)
#define CMD_TWO_ARG(name) extern void name(dbref, dbref, int, char *, char *)
#define CMD_TWO_ARG_CMDARG(name) extern void name(dbref, dbref, int, char *, char *, char*[], int)
#define CMD_TWO_ARG_ARGV(name) extern void name(dbref, dbref, int, char *, char *[], int)
#define CMD_TWO_ARG_ARGV_CMDARG(name) extern void name(dbref, dbref, int, char *, char *[], int, char*[], int)

/* Command function handlers */

CMD_ONE_ARG_CMDARG(do_apply_marked);	/* Apply command to marked objects */
CMD_TWO_ARG(do_admin);		/* Change config parameters */
CMD_TWO_ARG(do_alias);		/* Change the alias of something */
CMD_TWO_ARG(do_attribute);	/* Manage user-named attributes */
CMD_ONE_ARG(do_boot);		/* Force-disconnect a player */
CMD_TWO_ARG(do_chown);		/* Change object or attribute owner */
CMD_TWO_ARG(do_chownall);	/* Give away all of someone's objs */
CMD_TWO_ARG(do_chzone);		/* Change an object's zone. */
CMD_TWO_ARG(do_clone);		/* Create a copy of an object */
CMD_TWO_ARG(do_colormap);	/* Remap ANSI colors in output */
CMD_NO_ARG(do_comment);		/* Ignore argument and do nothing */
CMD_TWO_ARG_ARGV(do_cpattr);	/* Copy attributes */
CMD_TWO_ARG(do_create);		/* Create a new object */
CMD_TWO_ARG(do_cron);		/* Add a new cron entry */
CMD_ONE_ARG(do_crondel);	/* Delete cron entries */
CMD_ONE_ARG(do_crontab);	/* List out cron entries */
CMD_ONE_ARG(do_cut);		/* Truncate contents or exits list */
CMD_NO_ARG(do_dbck);		/* Consistency check */
CMD_TWO_ARG(do_decomp);		/* Show commands to recreate obj */
CMD_ONE_ARG(do_destroy);	/* Destroy an object */
CMD_TWO_ARG_ARGV(do_dig);	/* Dig a new room */
CMD_ONE_ARG(do_doing);		/* Set doing string in WHO report */
CMD_TWO_ARG_CMDARG(do_dolist);	/* Iterate command on list members */
CMD_ONE_ARG(do_drop);		/* Drop an object */
CMD_NO_ARG(do_dump);		/* Dump the database */
CMD_TWO_ARG_ARGV(do_edit);	/* Edit one or more attributes */
CMD_TWO_ARG_CMDARG(do_end);	/* End action list on a conditional */
CMD_ONE_ARG(do_enter);		/* Enter an object */
CMD_ONE_ARG(do_entrances);	/* List exits and links to loc */
CMD_ONE_ARG(do_eval);		/* Like @@ comment, but takes an arg */
CMD_ONE_ARG(do_examine);	/* Examine an object */
CMD_ONE_ARG(do_find);		/* Search for name in database */
CMD_TWO_ARG(do_fixdb);		/* Database repair functions */
CMD_ONE_ARG(do_floaters);	/* Display floating rooms */
CMD_TWO_ARG_CMDARG(do_force);	/* Force someone to do something */
CMD_ONE_ARG_CMDARG(do_force_prefixed);	/* #<num> <cmd> variant of FORCE */
CMD_ONE_ARG(do_freelist);	/* Move obj to head of freelist */
CMD_TWO_ARG(do_function);	/* Make user-def global function */
CMD_ONE_ARG(do_get);		/* Get an object */
CMD_TWO_ARG(do_give);		/* Give something away */
CMD_ONE_ARG(do_global);		/* Enable/disable global flags */
CMD_ONE_ARG(do_halt);		/* Remove commands from the queue */
CMD_NO_ARG(do_hashresize);	/* Resize the hash tables */
CMD_TWO_ARG(do_hook);		/* Command hooks */
CMD_ONE_ARG(do_help);		/* Print info from help files */
CMD_TWO_ARG_ARGV_CMDARG(do_include);	/* Include text of attribute */
CMD_NO_ARG(do_inventory);	/* Print what I am carrying */
CMD_TWO_ARG(do_prog);		/* Interactive input */
CMD_ONE_ARG(do_quitprog);	/* Quits @prog */
CMD_TWO_ARG(do_kill);		/* Kill something */
CMD_ONE_ARG(do_last);		/* Get recent login info */
CMD_NO_ARG(do_leave);		/* Leave the current object */
CMD_TWO_ARG(do_link);		/* Set home, dropto, or dest */
CMD_ONE_ARG(do_list);		/* List contents of internal tables */
CMD_ONE_ARG(do_list_file);	/* List contents of message files */
CMD_TWO_ARG(do_lock);		/* Set a lock on an object */
CMD_ONE_ARG(do_look);		/* Look here or at something */
CMD_NO_ARG(do_markall);		/* Mark or unmark all objects */
CMD_ONE_ARG(do_motd);		/* Set/list MOTD messages */
CMD_ONE_ARG(do_move);		/* Move about using exits */
CMD_TWO_ARG_ARGV(do_mvattr);	/* Move attributes on object */
CMD_TWO_ARG(do_name);		/* Change the name of something */
CMD_TWO_ARG(do_newpassword);	/* Change passwords */
CMD_TWO_ARG(do_notify);		/* Notify or drain semaphore */
CMD_TWO_ARG_ARGV(do_open);	/* Open an exit */
CMD_TWO_ARG(do_page);		/* Send message to faraway player */
CMD_TWO_ARG(do_parent);		/* Set parent field */
CMD_TWO_ARG(do_password);	/* Change my password */
CMD_TWO_ARG(do_pcreate);	/* Create new characters */
CMD_TWO_ARG(do_pemit);		/* Messages to specific player */
CMD_ONE_ARG(do_poor);		/* Reduce wealth of all players */
CMD_TWO_ARG(do_power);		/* Sets powers */
CMD_ONE_ARG(do_ps);		/* List contents of queue */
CMD_ONE_ARG(do_queue);		/* Force queue processing */
CMD_TWO_ARG(do_quota);		/* Set or display quotas */
CMD_NO_ARG(do_readcache);	/* Reread text file cache */
CMD_TWO_ARG(do_redirect);	/* Redirect output */
CMD_TWO_ARG(do_reference);	/* Manipulate nrefs */
CMD_ONE_ARG(do_reply_page);	/* Reply to a page */
CMD_NO_ARG(do_restart);		/* Restart the game. */
CMD_ONE_ARG(do_say);		/* Messages to all */
CMD_NO_ARG(do_score);		/* Display my wealth */
CMD_ONE_ARG(do_search);		/* Search for objs matching criteria */
CMD_TWO_ARG(do_set);		/* Set flags or attributes */
CMD_TWO_ARG(do_setattr);	/* Set object attribute */
CMD_TWO_ARG(do_setvattr);	/* Set variable attribute */
CMD_ONE_ARG(do_shutdown);	/* Stop the game */
CMD_ONE_ARG(do_stats);		/* Display object type breakdown */
CMD_ONE_ARG(do_sweep);		/* Check for listeners */
CMD_TWO_ARG_ARGV_CMDARG(do_switch);	/* Execute cmd based on match */
CMD_TWO_ARG(do_teleport);	/* Teleport elsewhere */
CMD_ONE_ARG(do_think);		/* Think command */
CMD_NO_ARG(do_timecheck);	/* Check time used by objects */
CMD_ONE_ARG(do_timewarp);	/* Warp various timers */
CMD_TWO_ARG(do_toad);		/* Turn a tinyjerk into a tinytoad */
CMD_TWO_ARG_ARGV(do_trigger);	/* Trigger an attribute */
CMD_ONE_ARG(do_unlock);		/* Remove a lock from an object */
CMD_ONE_ARG(do_unlink);		/* Unlink exit or remove dropto */
CMD_ONE_ARG(do_use);		/* Use object */
CMD_NO_ARG(do_version);		/* List MUSH version number */
CMD_TWO_ARG_ARGV(do_verb);	/* Execute a user-created verb */
CMD_TWO_ARG_CMDARG(do_wait);	/* Perform command after a wait */
CMD_ONE_ARG(do_wipe);		/* Mass-remove attrs from obj */
CMD_TWO_ARG(do_addcommand);	/* Add or replace a global command */
CMD_TWO_ARG(do_delcommand);	/* Delete an added global command */
CMD_ONE_ARG(do_listcommands);	/* List added global commands */
CMD_TWO_ARG(do_logwrite);	/* Write to the logfile */
CMD_NO_ARG(do_logrotate);	/* Rotate the logfile */

typedef struct addedentry ADDENT;
struct addedentry {
    dbref		thing;
    int		atr;
    char           *name;
    struct addedentry *next;
};

typedef struct cmdentry CMDENT;
struct cmdentry {
    char           *cmdname;
    NAMETAB        *switches;
    int		perms;
    int		extra;
    int		callseq;
    /* int	(*modperms)(); */
    HOOKENT        *userperms;
    HOOKENT        *pre_hook;
    HOOKENT        *post_hook;
    union {
        void            (*handler) ();
        ADDENT         *added;
    }		info;
};

/* Command handler call conventions */

#define CS_NO_ARGS	0x00000	/* No arguments */
#define CS_ONE_ARG	0x00001	/* One argument */
#define CS_TWO_ARG	0x00002	/* Two arguments */
#define CS_NARG_MASK	0x00003	/* Argument count mask */
#define CS_ARGV		0x00004	/* ARG2 is in ARGV form */
#define CS_INTERP	0x00010	/* Interpret ARG2 if 2 args, ARG1 if 1 */
#define CS_NOINTERP	0x00020	/* Never interp ARG2 if 2 or ARG1 if 1 */
#define CS_CAUSE	0x00040	/* Pass cause to old command handler */
#define CS_UNPARSE	0x00080	/* Pass unparsed cmd to old-style handler */
#define CS_CMDARG	0x00100	/* Pass in given command args */
#define CS_STRIP	0x00200	/* Strip braces even when not interpreting */
#define	CS_STRIP_AROUND	0x00400	/* Strip braces around entire string only */
#define CS_ADDED	0x00800	/* Command has been added by @addcommand */
#define CS_LEADIN	0x01000	/* Command is a single-letter lead-in */
#define CS_PRESERVE	0x02000	/* For hooks, preserve global registers */
#define CS_NOSQUISH	0x04000	/* Do not space-compress */
#define CS_FUNCTION	0x08000	/* Can call with command() */
#define CS_ACTOR	0x10000	/* @addcommand executed by player, not obj */
#define CS_PRIVATE	0x20000	/* For hooks, use private global registers */

/* Command permission flags */

#define CA_PUBLIC	0x00000000	/* No access restrictions */
#define CA_GOD		0x00000001	/* GOD only... */
#define CA_WIZARD	0x00000002	/* Wizards only */
#define CA_BUILDER	0x00000004	/* Builders only */
#define CA_IMMORTAL	0x00000008	/* Immortals only */
#define CA_STAFF	0x00000010	/* Must have STAFF flag */
#define CA_HEAD		0x00000020	/* Must have HEAD flag */
#define CA_MODULE_OK	0x00000040	/* Must have MODULE_OK power */
#define CA_ADMIN	0x00000080	/* Wizard or royal */

#define CA_ISPRIV_MASK  (CA_GOD|CA_WIZARD|CA_BUILDER|CA_IMMORTAL|CA_STAFF|CA_HEAD|CA_ADMIN|CA_MODULE_OK)

#define CA_NO_HAVEN	0x00000100	/* Not by HAVEN players */
#define CA_NO_ROBOT	0x00000200	/* Not by ROBOT players */
#define CA_NO_SLAVE	0x00000400	/* Not by SLAVE players */
#define CA_NO_SUSPECT	0x00000800	/* Not by SUSPECT players */
#define CA_NO_GUEST	0x00001000	/* Not by GUEST players */

#define CA_ISNOT_MASK   (CA_NO_HAVEN|CA_NO_ROBOT|CA_NO_SLAVE|CA_NO_SUSPECT|CA_NO_GUEST)

#define CA_MARKER0	0x00002000
#define CA_MARKER1	0x00004000
#define CA_MARKER2	0x00008000
#define CA_MARKER3	0x00010000
#define CA_MARKER4	0x00020000
#define CA_MARKER5	0x00040000
#define CA_MARKER6	0x00080000
#define CA_MARKER7	0x00100000
#define CA_MARKER8	0x00200000
#define CA_MARKER9	0x00400000

#define CA_MARKER_MASK  (CA_MARKER0|CA_MARKER1|CA_MARKER2|CA_MARKER3|CA_MARKER4|CA_MARKER5|CA_MARKER6|CA_MARKER7|CA_MARKER8|CA_MARKER9)

#define CA_GBL_BUILD	0x00800000	/* Requires the global BUILDING flag */
#define CA_GBL_INTERP	0x01000000	/* Requires the global INTERP flag */
#define CA_DISABLED	0x02000000	/* Command completely disabled */
#define CA_STATIC	0x04000000	/* Cannot be changed at runtime */
#define	CA_NO_DECOMP	0x08000000	/* Don't include in @decompile */

#define CA_LOCATION	0x10000000	/* Invoker must have location */
#define CA_CONTENTS	0x20000000	/* Invoker must have contents */
#define CA_PLAYER	0x40000000	/* Invoker must be a player */
#define CF_DARK		0x80000000	/* Command doesn't show up in list */

#define	SW_MULTIPLE	0x80000000	/* This sw may be spec'd w/others */
#define	SW_GOT_UNIQUE	0x40000000	/* Already have a unique option */
#define SW_NOEVAL       0x20000000	/* Don't parse args before calling
* handler */
/* (typically via a switch alias) */

/*
 * #define Check_Cmd_Access(p,c,a,n) \ (check_access(p,(c)->perms) && \
 * (!((c)->xperms) || check_mod_access(p,(c)->xperms)) && \
 * (!((c)->userperms) || check_userdef_access(p,(c)->userperms,a,n) ||
 * God(p)))
 */

#define Check_Cmd_Access(p,c,a,n) \
(check_access(p,(c)->perms) && \
 (!((c)->userperms) || check_userdef_access(p,(c)->userperms,a,n) || God(p)))

extern int	check_userdef_access(dbref, HOOKENT *, char *[], int);
extern char    *process_command(dbref, dbref, int, char *, char *[], int);

#endif				/* __COMMAND_H */
