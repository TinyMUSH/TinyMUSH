/* db.h */
/* $Id: db.h,v 1.53 2009/02/21 20:58:58 lwl Exp $ */

#include "copyright.h"

#ifndef	__DB_H
#define	__DB_H

#define SYNC			cache_sync()
#define CLOSE			{ cache_sync(); dddb_close(); }
#define DBCLOSE			dddb_close()
#define OPTIMIZE                (void) dddb_optimize()

/* Macros to help deal with batch writes of attribute numbers and objects */

#define ATRNUM_BLOCK_SIZE	(int) ((mudstate.db_block_size - 32) / \
					(2 * sizeof (int) + VNAME_SIZE))
#define ATRNUM_BLOCK_BYTES	(int) ((ATRNUM_BLOCK_SIZE) * \
					(2 * sizeof (int) + VNAME_SIZE))
#define OBJECT_BLOCK_SIZE	(int) ((mudstate.db_block_size - 32) / \
					(sizeof(int) + sizeof(DUMPOBJ)))
#define OBJECT_BLOCK_BYTES	(int) ((OBJECT_BLOCK_SIZE) * \
					(sizeof(int) + sizeof(DUMPOBJ)))
#define ENTRY_NUM_BLOCKS(total, blksize)	(int) (total / blksize)
#define ENTRY_BLOCK_STARTS(blk, blksize)	(int) (blk * blksize)
#define ENTRY_BLOCK_ENDS(blk, blksize)	(int) (blk * blksize) + (blksize - 1)


#include "udb.h"
#include "udb_defs.h" 

#define	ITER_PARENTS(t,p,l)	for ((l)=0, (p)=(t); \
				     (Good_obj(p) && \
				      ((l) < mudconf.parent_nest_lim)); \
				     (p)=Parent(p), (l)++)

#define Hasprivs(x)      (Royalty(x) || Wizard(x))

typedef struct attr ATTR;
struct attr
{
    const char     *name;	/* This has to be first.  braindeath. */
    int		number;	/* attr number */
    int		flags;
    int		FDECL      ((*check), (int, dbref, dbref, int, char *));
};

extern ATTR    *FDECL(atr_num, (int anum));
extern ATTR    *FDECL(atr_str, (char *s));

extern ATTR	attr[];

extern ATTR   **anum_table;

#define anum_get(x)	(anum_table[(x)])
#define anum_set(x,v)	anum_table[(x)] = v
extern void	FDECL(anum_extend, (int));

#define	ATR_INFO_CHAR	'\1'	/* Leadin char for attr control data */

/* Boolean expressions, for locks */
#define	BOOLEXP_AND	0
#define	BOOLEXP_OR	1
#define	BOOLEXP_NOT	2
#define	BOOLEXP_CONST	3
#define	BOOLEXP_ATR	4
#define	BOOLEXP_INDIR	5
#define	BOOLEXP_CARRY	6
#define	BOOLEXP_IS	7
#define	BOOLEXP_OWNER	8
#define	BOOLEXP_EVAL	9

typedef struct boolexp BOOLEXP;
struct boolexp
{
    boolexp_type	type;
    struct boolexp *sub1;
    struct boolexp *sub2;
    dbref		thing;	/* thing refers to an object */
};

#define	TRUE_BOOLEXP ((BOOLEXP *) 0)

/* Database format information */

#define	F_UNKNOWN	0	/* Unknown database format */
#define	F_MUSH		1	/* MUSH format (many variants) */
#define	F_MUSE		2	/* MUSE format */
#define	F_MUD		3	/* Old TinyMUD format */
#define	F_MUCK		4	/* TinyMUCK format */
#define F_MUX		5	/* TinyMUX format */
#define F_TINYMUSH	6	/* TinyMUSH 3.0 format */

#define	V_MASK		0x000000ff	/* Database version */
#define	V_ZONE		0x00000100	/* ZONE/DOMAIN field */
#define	V_LINK		0x00000200	/* LINK field (exits from objs) */
#define	V_GDBM		0x00000400	/* attrs are in a gdbm db, not here */
#define	V_ATRNAME	0x00000800	/* NAME is an attr, not in the hdr */
#define	V_ATRKEY	0x00001000	/* KEY is an attr, not in the hdr */
#define	V_PERNKEY	0x00001000	/* PERN: Extra locks in object hdr */
#define	V_PARENT	0x00002000	/* db has the PARENT field */
#define	V_COMM		0x00004000	/* PERN: Comm status in header */
#define	V_ATRMONEY	0x00008000	/* Money is kept in an attribute */
#define	V_XFLAGS	0x00010000	/* An extra word of flags */
#define V_POWERS        0x00020000	/* Powers? */
#define V_3FLAGS	0x00040000	/* Adding a 3rd flag word */
#define V_QUOTED	0x00080000	/* Quoted strings, ala PennMUSH */
#define V_TQUOTAS       0x00100000	/* Typed quotas */
#define V_TIMESTAMPS	0x00200000	/* Timestamps */
#define V_VISUALATTRS	0x00400000	/* ODark-to-Visual attr flags */
#define V_CREATETIME	0x00800000	/* Create time */
#define V_DBCLEAN	0x80000000	/* Option to clean attr table */

/* special dbref's */
#define	NOTHING		(-1)	/* null dbref */
#define	AMBIGUOUS	(-2)	/* multiple possibilities, for matchers */
#define	HOME		(-3)	/* virtual room, represents mover's home */
#define	NOPERM		(-4)	/* Error status, no permission */

typedef struct object OBJ;
struct object
{
    dbref		location;	/* PLAYER, THING: where it is */
    /* ROOM: dropto: */
    /* EXIT: where it goes to */
    dbref		contents;	/* PLAYER, THING, ROOM: head of
					 * contentslist */
    /* EXIT: unused */
    dbref		exits;	/* PLAYER, THING, ROOM: head of exitslist */
    /* EXIT: where it is */
    dbref		next;	/* PLAYER, THING: next in contentslist */
    /* EXIT: next in exitslist */
    /* ROOM: unused */
    dbref		link;	/* PLAYER, THING: home location */
    /* ROOM, EXIT: unused */
    dbref		parent;	/* ALL: defaults for attrs, exits, $cmds, */
    dbref		owner;	/* PLAYER: domain number + class + moreflags */
    /* THING, ROOM, EXIT: owning player number */

    dbref		zone;	/* Whatever the object is zoned to. */

    FLAG		flags;	/* ALL: Flags set on the object */
    FLAG		flags2;	/* ALL: even more flags */
    FLAG		flags3;	/* ALL: yet _more_ flags */

    POWER		powers;	/* ALL: Powers on object */
    POWER		powers2;/* ALL: even more powers */

    time_t		create_time;	/* ALL: Time created (used in ObjID) */
    time_t		last_access;	/* ALL: Time last accessed */
    time_t		last_mod;	/* ALL: Time last modified */

    /*
     * Make sure everything you want to write to the DBM database is in
     * the first part of the structure and included in DUMPOBJ
     */

    int		name_length;	/* ALL: Length of name string */

    int		stack_count;	/* ALL: number of things on the stack */
    int		vars_count;	/* ALL: number of variables */
    int		struct_count;	/* ALL: number of structures */
    int		instance_count;	/* ALL: number of struct instances */

#ifndef NO_TIMECHECKING
    struct timeval	cpu_time_used;	/* ALL: CPU time eaten */
#endif

#ifdef MEMORY_BASED
    Obj		attrtext;	/* Array of attribute text */
#endif
};

/*
 * The DUMPOBJ structure exists for use during database writes. It is a
 * duplicate of the OBJ structure except for items we don't need to write
 */

typedef struct dump_object DUMPOBJ;
struct dump_object
{
    dbref		location;	/* PLAYER, THING: where it is */
    /* ROOM: dropto: */
    /* EXIT: where it goes to */
    dbref		contents;	/* PLAYER, THING, ROOM: head of
					 * contentslist */
    /* EXIT: unused */
    dbref		exits;	/* PLAYER, THING, ROOM: head of exitslist */
    /* EXIT: where it is */
    dbref		next;	/* PLAYER, THING: next in contentslist */
    /* EXIT: next in exitslist */
    /* ROOM: unused */
    dbref		link;	/* PLAYER, THING: home location */
    /* ROOM, EXIT: unused */
    dbref		parent;	/* ALL: defaults for attrs, exits, $cmds, */
    dbref		owner;	/* PLAYER: domain number + class + moreflags */
    /* THING, ROOM, EXIT: owning player number */

    dbref		zone;	/* Whatever the object is zoned to. */

    FLAG		flags;	/* ALL: Flags set on the object */
    FLAG		flags2;	/* ALL: even more flags */
    FLAG		flags3;	/* ALL: yet _more_ flags */

    POWER		powers;	/* ALL: Powers on object */
    POWER		powers2;/* ALL: even more powers */

    time_t		create_time;	/* ALL: Time created (used in ObjID) */
    time_t		last_access;	/* ALL: Time last accessed */
    time_t		last_mod;	/* ALL: Time last modified */
};

typedef char   *NAME;

extern OBJ     *db;
extern NAME    *names;

#define	Location(t)		db[t].location

#define	Zone(t)			db[t].zone

#define	Contents(t)		db[t].contents
#define	Exits(t)		db[t].exits
#define	Next(t)			db[t].next
#define	Link(t)			db[t].link
#define	Owner(t)		db[t].owner
#define	Parent(t)		db[t].parent
#define	Flags(t)		db[t].flags
#define	Flags2(t)		db[t].flags2
#define Flags3(t)		db[t].flags3
#define Powers(t)		db[t].powers
#define Powers2(t)		db[t].powers2
#define NameLen(t)		db[t].name_length
#define	Home(t)			Link(t)
#define	Dropto(t)		Location(t)

#define AccessTime(t)		db[t].last_access
#define ModTime(t)		db[t].last_mod
#define CreateTime(t)           db[t].create_time

#define VarsCount(t)		db[t].vars_count
#define StackCount(t)		db[t].stack_count
#define StructCount(t)		db[t].struct_count
#define InstanceCount(t)	db[t].instance_count

#ifndef NO_TIMECHECKING
#define Time_Used(t)		db[t].cpu_time_used
#define s_Time_Used(t,n)	db[t].cpu_time_used.tv_sec = n.tv_sec; \
				db[t].cpu_time_used.tv_usec = n.tv_usec
#endif

/*
 * If we modify something on the db object that needs to be written at dump
 * time, set the object DIRTY
 */

#define	s_Location(t,n)		db[t].location = (n); \
				db[t].flags3 |= DIRTY
#define	s_Zone(t,n)		db[t].zone = (n); \
				db[t].flags3 |= DIRTY
#define	s_Contents(t,n)		db[t].contents = (n); \
				db[t].flags3 |= DIRTY
#define	s_Exits(t,n)		db[t].exits = (n); \
				db[t].flags3 |= DIRTY
#define	s_Next(t,n)		db[t].next = (n); \
				db[t].flags3 |= DIRTY
#define	s_Link(t,n)		db[t].link = (n); \
				db[t].flags3 |= DIRTY
#define	s_Owner(t,n)		db[t].owner = (n); \
				db[t].flags3 |= DIRTY
#define	s_Parent(t,n)		db[t].parent = (n); \
				db[t].flags3 |= DIRTY
#define	s_Flags(t,n)		db[t].flags = (n); \
				db[t].flags3 |= DIRTY
#define	s_Flags2(t,n)		db[t].flags2 = (n); \
				db[t].flags3 |= DIRTY
#define s_Flags3(t,n)		db[t].flags3 = (n); \
				db[t].flags3 |= DIRTY
#define s_Powers(t,n)		db[t].powers = (n); \
				db[t].flags3 |= DIRTY
#define s_Powers2(t,n)		db[t].powers2 = (n); \
				db[t].flags3 |= DIRTY
#define s_AccessTime(t,n)	db[t].last_access = (n); \
				db[t].flags3 |= DIRTY
#define s_ModTime(t,n)		db[t].last_mod = (n); \
				db[t].flags3 |= DIRTY
#define s_CreateTime(t,n)	db[t].create_time = (n); \
				db[t].flags3 |= DIRTY
#define s_Accessed(t)		db[t].last_access = mudstate.now; \
				db[t].flags3 |= DIRTY
#define s_Modified(t)		db[t].last_mod = mudstate.now; \
				db[t].flags3 |= DIRTY
#define s_Created(t)		db[t].create_time = mudstate.now; \
				db[t].flags3 |= DIRTY
#define s_Clean(t)		db[t].flags3 = db[t].flags3 & ~DIRTY
#define s_NameLen(t,n)		db[t].name_length = (n)
#define	s_Home(t,n)		s_Link(t,n)
#define	s_Dropto(t,n)		s_Location(t,n)
#define s_VarsCount(t,n)	db[t].vars_count = n;
#define s_StackCount(t,n)	db[t].stack_count = n;
#define s_StructCount(t,n)	db[t].struct_count = n;
#define s_InstanceCount(t,n)	db[t].instance_count = n;

extern int	FDECL(Pennies, (dbref));
extern void	FDECL(s_Pennies, (dbref, int));

extern void	NDECL(tf_init);
extern int	FDECL(tf_open, (char *, int));
extern void	FDECL(tf_close, (int));
extern FILE    *FDECL(tf_fopen, (char *, int));
extern void	FDECL(tf_fclose, (FILE *));
extern FILE    *FDECL(tf_popen, (char *, int));

#define tf_pclose(f)	tf_fclose(f)

#define putref(pr__f,pr__ref)	fprintf(pr__f, "%d\n", (int)pr__ref)
#define putlong(pr__f,pr__i)	fprintf(pr__f, "%ld\n", (long)pr__i)

extern INLINE dbref FDECL(getref, (FILE *));
extern INLINE long FDECL(getlong, (FILE *));
extern BOOLEXP *FDECL(dup_bool, (BOOLEXP *));
extern void	FDECL(free_boolexp, (BOOLEXP *));
extern dbref	FDECL(parse_dbref, (const char *));
extern dbref	FDECL(parse_dbref_only, (const char *));
extern dbref	FDECL(parse_objid, (const char *, const char *));
extern int	FDECL(mkattr, (char *));
extern void	FDECL(al_add, (dbref, int));
extern void	FDECL(al_delete, (dbref, int));
extern void	FDECL(al_destroy, (dbref));
extern void	NDECL(al_store);
extern void	FDECL(db_grow, (dbref));
extern void	NDECL(db_free);
extern void	NDECL(db_make_minimal);
extern dbref	FDECL(db_convert, (FILE *, int *, int *, int *));
extern dbref	NDECL(db_read);
extern dbref	FDECL(db_write_flatfile, (FILE *, int, int));
extern dbref	NDECL(db_write);
extern FILE    *FDECL(db_module_flatfile, (char *, int));
extern void	FDECL(destroy_thing, (dbref));
extern void	FDECL(destroy_exit, (dbref));

#define	DOLIST(thing,list) \
	for ((thing)=(list); \
	     ((thing)!=NOTHING) && (Next(thing)!=(thing)); \
	     (thing)=Next(thing))
#define	SAFE_DOLIST(thing,next,list) \
	for ((thing)=(list),(next)=((thing)==NOTHING ? NOTHING: Next(thing)); \
	     (thing)!=NOTHING && (Next(thing)!=(thing)); \
	     (thing)=(next), (next)=Next(next))
#define	DO_WHOLE_DB(thing) \
	for ((thing)=0; (thing)<mudstate.db_top; (thing)++)
#define	DO_WHOLE_DB_BACKWARDS(thing) \
	for ((thing)=mudstate.db_top-1; (thing)>=0; (thing)--)

#define	Dropper(thing)	(Connected(Owner(thing)) && Hearer(thing))

/* Clear a player's aliases, given x (player dbref) and b (alias buffer). */
#define Clear_Player_Aliases(x,b) \
{ \
    char *cpa__p, *cpa__tokp; \
    for (cpa__p = strtok_r((b), ";", &cpa__tokp); cpa__p; \
	 cpa__p = strtok_r(NULL, ";", &cpa__tokp)) { \
	delete_player_name((x), cpa__p); \
    } \
}

typedef struct logfiletable LOGFILETAB;
struct logfiletable
{
    int		log_flag;
    FILE           *fileptr;
    char           *filename;
};

typedef struct numbertable NUMBERTAB;
struct numbertable
{
    int		num;
};

#endif				/* __DB_H */
