/* flags.h - object flags */

#include "copyright.h"

#ifndef __FLAGS_H
#define __FLAGS_H

#define FLAG_WORD1  0x0 /* 1st word of flags. */
#define FLAG_WORD2  0x1 /* 2nd word of flags. */
#define FLAG_WORD3  0x2 /* 3rd word of flags. */

/* Object types */
#define TYPE_ROOM   0x0
#define TYPE_THING  0x1
#define TYPE_EXIT   0x2
#define TYPE_PLAYER     0x3
#define TYPE_ZONE       0x4
#define TYPE_GARBAGE    0x5
#define GOODTYPE    0x5 /* Values less than this pass Good_obj check */
#define NOTYPE      0x7
#define TYPE_MASK   0x7

/* First word of flags */
#define SEETHRU     0x00000008  /* Can see through to the other side */
#define WIZARD      0x00000010  /* gets automatic control */
#define LINK_OK     0x00000020  /* anybody can link to this room */
#define DARK        0x00000040  /* Don't show contents or presence */
#define JUMP_OK     0x00000080  /* Others may @tel here */
#define STICKY      0x00000100  /* Object goes home when dropped */
#define DESTROY_OK  0x00000200  /* Others may @destroy */
#define HAVEN       0x00000400  /* No killing here, or no pages */
#define QUIET       0x00000800  /* Prevent 'feelgood' messages */
#define HALT        0x00001000  /* object cannot perform actions */
#define TRACE       0x00002000  /* Generate evaluation trace output */
#define GOING       0x00004000  /* object is available for recycling */
#define MONITOR     0x00008000  /* Process ^x:action listens on obj? */
#define MYOPIC      0x00010000  /* See things as nonowner/nonwizard */
#define PUPPET      0x00020000  /* Relays ALL messages to owner */
#define CHOWN_OK    0x00040000  /* Object may be @chowned freely */
#define ENTER_OK    0x00080000  /* Object may be ENTERed */
#define VISUAL      0x00100000  /* Everyone can see properties */
#define IMMORTAL    0x00200000  /* Object can't be killed */
#define HAS_STARTUP 0x00400000  /* Load some attrs at startup */
#define OPAQUE      0x00800000  /* Can't see inside */
#define VERBOSE     0x01000000  /* Tells owner everything it does. */
#define INHERIT     0x02000000  /* Gets owner's privs. (i.e. Wiz) */
#define NOSPOOF     0x04000000  /* Report originator of all actions. */
#define ROBOT       0x08000000  /* Player is a ROBOT */
#define SAFE        0x10000000  /* Need /override to @destroy */
#define ROYALTY     0x20000000  /* Sees like a wiz, but ca't modify */
#define HEARTHRU    0x40000000  /* Can hear out of this obj or exit */
#define TERSE       0x80000000  /* Only show room name on look */

/* Second word of flags */
#define KEY     0x00000001  /* No puppets */
#define ABODE       0x00000002  /* May @set home here */
#define FLOATING    0x00000004  /* -- Legacy -- */
#define UNFINDABLE  0x00000008  /* Can't loc() from afar */
#define PARENT_OK   0x00000010  /* Others may @parent to me */
#define LIGHT       0x00000020  /* Visible in dark places */
#define HAS_LISTEN  0x00000040  /* Internal: LISTEN attr set */
#define HAS_FWDLIST 0x00000080  /* Internal: FORWARDLIST attr set */
#define AUDITORIUM  0x00000100  /* Should we check the SpeechLock? */
#define ANSI            0x00000200
#define HEAD_FLAG       0x00000400
#define FIXED           0x00000800
#define UNINSPECTED     0x00001000
#define ZONE_PARENT 0x00002000  /* Check as local master room */
#define DYNAMIC         0x00004000
#define NOBLEED         0x00008000
#define STAFF           0x00010000
#define HAS_DAILY   0x00020000
#define GAGGED      0x00040000
#define HAS_COMMANDS    0x00080000  /* Check it for $commands */
#define STOP_MATCH  0x00100000  /* Stop matching commands if found */
#define BOUNCE      0x00200000  /* Forward messages to contents */
#define CONTROL_OK  0x00400000  /* ControlLk specifies who ctrls me */
#define CONSTANT_ATTRS  0x00800000  /* Can't set attrs on this object */
#define VACATION    0x01000000
#define PLAYER_MAILS    0x02000000  /* Mail message in progress */
#define HTML        0x04000000  /* Player supports HTML */
#define BLIND       0x08000000  /* Suppress has arrived / left msgs */
#define SUSPECT     0x10000000  /* Report some activities to wizards */
#define WATCHER     0x20000000  /* Watch logins */
#define CONNECTED   0x40000000  /* Player is connected */
#define SLAVE       0x80000000  /* Disallow most commands */

/* Third word of flags */
#define REDIR_OK    0x00000001  /* Can be victim of @redirect */
#define HAS_REDIRECT    0x00000002  /* Victim of @redirect */
#define ORPHAN      0x00000004  /* Don't check parent chain for $cmd */
#define HAS_DARKLOCK    0x00000008  /* Has a DarkLock */
#define DIRTY       0x00000010  /* Temporary flag: object is dirty */
#define NODEFAULT   0x00000020  /* Not subject to attr defaults */
#define PRESENCE    0x00000040  /* Check presence-related locks */
#define HAS_SPEECHMOD   0x00000080  /* Check @speechmod attr */
#define HAS_PROPDIR 0X00000100  /* Internal: has Propdir attr */
#define COLOR256    0x00000200  /* Player support XTERM 256 colors */
#define FLAG_RES02  0x00000400
#define FLAG_RES03  0x00000800
#define FLAG_RES04  0x00001000
#define FLAG_RES05  0x00002000
#define FLAG_RES06  0x00004000
#define FLAG_RES07  0x00008000
#define FLAG_RES08  0x00010000
#define FLAG_RES09  0x00020000
#define FLAG_RES10  0x00040000
#define FLAG_RES11  0x00080000
#define FLAG_RES12  0x00100000
#define FLAG_RES13  0x00200000
/* FREE FREE FREE */
#define MARK_0      0x00400000  /* User-defined flags */
#define MARK_1      0x00800000
#define MARK_2      0x01000000
#define MARK_3      0x02000000
#define MARK_4      0x04000000
#define MARK_5      0x08000000
#define MARK_6      0x10000000
#define MARK_7      0x20000000
#define MARK_8      0x40000000
#define MARK_9      0x80000000
#define MARK_FLAGS  0xffc00000  /* Bitwise-or of all marker flags */

/*
 * ---------------------------------------------------------------------------
 * FLAGENT: Information about object flags.
 */

typedef struct flag_entry {
    const char     *flagname;   /* Name of the flag */
    int     flagvalue;  /* Which bit in the object is the
                     * flag */
    char        flaglett;   /* Flag letter for listing */
    int     flagflag;   /* Ctrl flags for this flag
                     * (recursive? :-) */
    int     listperm;   /* Who sees this flag when set */
    int ( *handler ) (); /* Handler for setting/clearing this flag */
}       FLAGENT;

/*
 * ---------------------------------------------------------------------------
 * OBJENT: Fundamental object types
 */

typedef struct object_entry {
    const char     *name;
    char        lett;
    int     perm;
    int     flags;
}       OBJENT;
extern OBJENT   object_types[8];

#define OF_CONTENTS 0x0001  /* Object has contents: Contents() */
#define OF_LOCATION 0x0002  /* Object has a location: Location() */
#define OF_EXITS    0x0004  /* Object has exits: Exits() */
#define OF_HOME     0x0008  /* Object has a home: Home() */
#define OF_DROPTO   0x0010  /* Object has a dropto: Dropto() */
#define OF_OWNER    0x0020  /* Object can own other objects */
#define OF_SIBLINGS 0x0040  /* Object has siblings: Next() */

typedef struct flagset {
    FLAG        word1;
    FLAG        word2;
    FLAG        word3;
}       FLAGSET;

extern void init_flagtab ( void );
extern void display_flagtab ( dbref );
extern void flag_set ( dbref, dbref, char *, int );
extern char    *flag_description ( dbref, dbref );
extern FLAGENT *find_flag ( dbref, char * );
extern char    *decode_flags ( dbref, FLAGSET );
extern char    *unparse_flags ( dbref, dbref );
extern int  has_flag ( dbref, dbref, char * );
extern char    *unparse_object ( dbref, dbref, int );
extern char    *unparse_object_numonly ( dbref );
extern int  convert_flags ( dbref, char *, FLAGSET *, FLAG * );
extern void decompile_flags ( dbref, dbref, char * );

#define GOD ((dbref) 1)

/* ---------------------- Object Permission/Attribute Macros */
/* IS(X,T,F)        - Is X of type T and have flag F set? */
/* Typeof(X)        - What object type is X */
/* God(X)       - Is X player #1 */
/* Robot(X)     - Is X a robot player */
/* Wizard(X)        - Does X have wizard privs */
/* Immortal(X)      - Is X unkillable */
/* Alive(X)     - Is X a player or a puppet */
/* Dark(X)      - Is X dark */
/* Quiet(X)     - Should 'Set.' messages et al from X be disabled */
/* Verbose(X)       - Should owner receive all commands executed? */
/* Trace(X)     - Should owner receive eval trace output? */
/* Player_haven(X)  - Is the owner of X no-page */
/* Haven(X)     - Is X no-kill(rooms) or no-page(players) */
/* Halted(X)        - Is X halted (not allowed to run commands)? */
/* Suspect(X)       - Is X someone the wizzes should keep an eye on */
/* Slave(X)     - Should X be prevented from db-changing commands */
/* Safe(X,P)        - Does P need the /OVERRIDE switch to @destroy X? */
/* Monitor(X)       - Should we check for ^xxx:xxx listens on player? */
/* Terse(X)     - Should we only show the room name on a look? */
/* Myopic(X)        - Should things as if we were nonowner/nonwiz */
/* Audible(X)       - Should X forward messages? */
/* Findroom(X)      - Can players in room X be found via @whereis? */
/* Unfindroom(X)    - Is @whereis blocked for players in room X? */
/* Findable(X)      - Can @whereis find X */
/* Unfindable(X)    - Is @whereis blocked for X */
/* No_robots(X)     - Does X disallow robot players from using */
/* Has_location(X)  - Is X something with a location (ie plyr or obj) */
/* Has_home(X)      - Is X something with a home (ie plyr or obj) */
/* Has_contents(X)  - Is X something with contents (ie plyr/obj/room) */
/* Good_dbref(X)    - Is X inside the DB? */
/* Good_obj(X)      - Is X inside the DB and have a valid type? */
/* Good_owner(X)    - Is X a good owner value? */
/* Good_home(X)     - Is X a good home value? */
/* Good_loc(X)      - Is X a good location value? */
/* Going(X)     - Is X marked GOING? */
/* Inherits(X)      - Does X inherit the privs of its owner */
/* Examinable(P,X)  - Can P look at attribs of X */
/* MyopicExam(P,X)  - Can P look at attribs of X (obeys MYOPIC) */
/* Controls(P,X)    - Can P force X to do something */
/* Darkened(P,X)    - Is X dark, and does P pass its DarkLock? */
/* Can_See(P,X,L)   - Can P see thing X, depending on location dark (L)? */
/* Can_See_Exit(P,X,L)  - Can P see exit X, depending on loc dark (L)? */
/* Abode(X)     - Is X an ABODE room */
/* Link_exit(P,X)   - Can P link from exit X */
/* Linkable(P,X)    - Can P link to X */
/* Mark(x)      - Set marked flag on X */
/* Unmark(x)        - Clear marked flag on X */
/* Marked(x)        - Check marked flag on X */
/* See_attr(P,X,A,O,F)  - Can P see text attr A on X if attr has owner O */
/* Set_attr(P,X,A,F)    - Can P set/change text attr A (with flags F) on X */
/* Read_attr(P,X,A,O,F) - Can P see attr A on X if attr has owner O */
/* Write_attr(P,X,A,F)  - Can P set/change attr A (with flags F) on X */
/* Lock_attr(P,X,A,O)   - Can P lock/unlock attr A (with owner O) on X */

#define IS(thing,type,flag) ((Typeof(thing)==(type)) && (Flags(thing) & (flag)))
#define Typeof(x)   (Flags(x) & TYPE_MASK)
#define God(x)      ((x) == GOD)
#define Robot(x)    (isPlayer(x) && ((Flags(x) & ROBOT) != 0))
#define Alive(x)    (isPlayer(x) || (Puppet(x) && Has_contents(x)))
#define OwnsOthers(x)   ((object_types[Typeof(x)].flags & OF_OWNER) != 0)
#define Has_location(x) ((object_types[Typeof(x)].flags & OF_LOCATION) != 0)
#define Has_contents(x) ((object_types[Typeof(x)].flags & OF_CONTENTS) != 0)
#define Has_exits(x)    ((object_types[Typeof(x)].flags & OF_EXITS) != 0)
#define Has_siblings(x) ((object_types[Typeof(x)].flags & OF_SIBLINGS) != 0)
#define Has_home(x) ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define Has_dropto(x)   ((object_types[Typeof(x)].flags & OF_DROPTO) != 0)
#define Home_ok(x)  ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define isPlayer(x) (Typeof(x) == TYPE_PLAYER)
#define isRoom(x)   (Typeof(x) == TYPE_ROOM)
#define isExit(x)   (Typeof(x) == TYPE_EXIT)
#define isThing(x)  (Typeof(x) == TYPE_THING)
#define isGarbage(x)    (Typeof(x) == TYPE_GARBAGE)

#define Good_dbref(x)   (((x) >= 0) && ((x) < mudstate.db_top))
#define Good_obj(x) (Good_dbref(x) && (Typeof(x) < GOODTYPE))
#define Good_owner(x)   (Good_obj(x) && OwnsOthers(x))
#define Good_home(x)    (Good_obj(x) && Home_ok(x))
#define Good_loc(x) (Good_obj(x) && Has_contents(x))

#define Royalty(x)      ((Flags(x) & ROYALTY) != 0)
#define WizRoy(x)       (Royalty(x) || Wizard(x))
#define Staff(x)    ((Flags2(x) & STAFF) != 0)
#define Head(x)     ((Flags2(x) & HEAD_FLAG) != 0)
#define Fixed(x)        ((Flags2(x) & FIXED) != 0)
#define Uninspected(x)  ((Flags2(x) & UNINSPECTED) != 0)
#define Ansi(x)         ((Flags2(x) & ANSI) != 0)
#define Color256(x) ((Flags3(x) & COLOR256) != 0)
#define NoBleed(x)      ((Flags2(x) & NOBLEED) != 0)

#define Transparent(x)  ((Flags(x) & SEETHRU) != 0)
#define Link_ok(x)  (((Flags(x) & LINK_OK) != 0) && Has_contents(x))
#define Wizard(x)   ((Flags(x) & WIZARD) || \
             ((Flags(Owner(x)) & WIZARD) && Inherits(x)))
#define Dark(x)     (((Flags(x) & DARK) != 0) && \
             (!Alive(x) || \
              (Wizard(x) && !mudconf.visible_wizzes) || \
              Can_Cloak(x)))
#define DarkMover(x)    ((Wizard(x) || Can_Cloak(x)) && Dark(x))
#define Jump_ok(x)  (((Flags(x) & JUMP_OK) != 0) && Has_contents(x))
#define Sticky(x)   ((Flags(x) & STICKY) != 0)
#define Destroy_ok(x)   ((Flags(x) & DESTROY_OK) != 0)
#define Haven(x)    ((Flags(x) & HAVEN) != 0)
#define Player_haven(x) ((Flags(Owner(x)) & HAVEN) != 0)
#define Quiet(x)    ((Flags(x) & QUIET) != 0)
#define Halted(x)   ((Flags(x) & HALT) != 0)
#define Trace(x)    ((Flags(x) & TRACE) != 0)
#define Going(x)    ((Flags(x) & GOING) != 0)
#define Monitor(x)  ((Flags(x) & MONITOR) != 0)
#define Myopic(x)   ((Flags(x) & MYOPIC) != 0)
#define Puppet(x)   ((Flags(x) & PUPPET) != 0)
#define Chown_ok(x) ((Flags(x) & CHOWN_OK) != 0)
#define Enter_ok(x) (((Flags(x) & ENTER_OK) != 0) && \
                Has_location(x) && Has_contents(x))
#define Visual(x)   ((Flags(x) & VISUAL) != 0)
#define Immortal(x) ((Flags(x) & IMMORTAL) || \
             ((Flags(Owner(x)) & IMMORTAL) && Inherits(x)))
#define Opaque(x)   ((Flags(x) & OPAQUE) != 0)
#define Verbose(x)  ((Flags(x) & VERBOSE) != 0)
#define Inherits(x) (((Flags(x) & INHERIT) != 0) || \
             ((Flags(Owner(x)) & INHERIT) != 0) || \
             ((x) == Owner(x)))
#define Nospoof(x)  ((Flags(x) & NOSPOOF) != 0)
#define Safe(x,p)   (OwnsOthers(x) || \
             (Flags(x) & SAFE) || \
             (mudconf.safe_unowned && (Owner(x) != Owner(p))))
#define Control_ok(x)   ((Flags2(x) & CONTROL_OK) != 0)
#define Constant_Attrs(x)  ((Flags2(x) & CONSTANT_ATTRS) != 0)
#define Audible(x)  ((Flags(x) & HEARTHRU) != 0)
#define Terse(x)    ((Flags(x) & TERSE) != 0)

#define Gagged(x)   ((Flags2(x) & GAGGED) != 0)
#define Vacation(x) ((Flags2(x) & VACATION) != 0)
#define Sending_Mail(x) ((Flags2(x) & PLAYER_MAILS) != 0)
#define Key(x)      ((Flags2(x) & KEY) != 0)
#define Abode(x)    (((Flags2(x) & ABODE) != 0) && Home_ok(x))
#define Auditorium(x)   ((Flags2(x) & AUDITORIUM) != 0)
#define Findable(x) ((Flags2(x) & UNFINDABLE) == 0)
#define Hideout(x)  ((Flags2(x) & UNFINDABLE) != 0)
#define Parent_ok(x)    ((Flags2(x) & PARENT_OK) != 0)
#define Light(x)    ((Flags2(x) & LIGHT) != 0)
#define Suspect(x)  ((Flags2(Owner(x)) & SUSPECT) != 0)
#define Watcher(x)  ((Flags2(x) & WATCHER) != 0)
#define Connected(x)    (((Flags2(x) & CONNECTED) != 0) && \
             (Typeof(x) == TYPE_PLAYER))
#define Slave(x)    ((Flags2(Owner(x)) & SLAVE) != 0)
#define ParentZone(x)   ((Flags2(x) & ZONE_PARENT) != 0)
#define Stop_Match(x)   ((Flags2(x) & STOP_MATCH) != 0)
#define Has_Commands(x) ((Flags2(x) & HAS_COMMANDS) != 0)
#define Bouncer(x)      ((Flags2(x) & BOUNCE) != 0)
#define Hidden(x)   ((Flags(x) & DARK) != 0)
#define Blind(x)    ((Flags2(x) & BLIND) != 0)
#define Redir_ok(x) ((Flags3(x) & REDIR_OK) != 0)
#define Orphan(x)   ((Flags3(x) & ORPHAN) != 0)
#define NoDefault(x)    ((Flags3(x) & NODEFAULT) != 0)
#define Unreal(x)   ((Flags3(x) & PRESENCE) != 0)

#define H_Startup(x)    ((Flags(x) & HAS_STARTUP) != 0)
#define H_Fwdlist(x)    ((Flags2(x) & HAS_FWDLIST) != 0)
#define H_Listen(x) ((Flags2(x) & HAS_LISTEN) != 0)
#define H_Redirect(x)   ((Flags3(x) & HAS_REDIRECT) != 0)
#define H_Darklock(x)   ((Flags3(x) & HAS_DARKLOCK) != 0)
#define H_Speechmod(x)  ((Flags3(x) & HAS_SPEECHMOD) != 0)
#define H_Propdir(x)    ((Flags3(x) & HAS_PROPDIR) != 0)

#define H_Marker0(x)    ((Flags3(x) & MARK_0) != 0)
#define H_Marker1(x)    ((Flags3(x) & MARK_1) != 0)
#define H_Marker2(x)    ((Flags3(x) & MARK_2) != 0)
#define H_Marker3(x)    ((Flags3(x) & MARK_3) != 0)
#define H_Marker4(x)    ((Flags3(x) & MARK_4) != 0)
#define H_Marker5(x)    ((Flags3(x) & MARK_5) != 0)
#define H_Marker6(x)    ((Flags3(x) & MARK_6) != 0)
#define H_Marker7(x)    ((Flags3(x) & MARK_7) != 0)
#define H_Marker8(x)    ((Flags3(x) & MARK_8) != 0)
#define H_Marker9(x)    ((Flags3(x) & MARK_9) != 0)
#define isMarkerFlag(fp)    (((fp)->flagflag & FLAG_WORD3) && \
                 ((fp)->flagvalue & MARK_FLAGS))

#define s_Halted(x) s_Flags((x), Flags(x) | HALT)
#define s_Going(x)  s_Flags((x), Flags(x) | GOING)
#define s_Connected(x)  s_Flags2((x), Flags2(x) | CONNECTED)
#define c_Connected(x)  s_Flags2((x), Flags2(x) & ~CONNECTED)
#define isConnFlag(fp)  (((fp)->flagflag & FLAG_WORD2) && \
             ((fp)->flagvalue & CONNECTED))
#define s_Has_Darklock(x)   s_Flags3((x), Flags3(x) | HAS_DARKLOCK)
#define c_Has_Darklock(x)   s_Flags3((x), Flags3(x) & ~HAS_DARKLOCK)
#define s_Trace(x)  s_Flags((x), Flags(x) | TRACE)
#define c_Trace(x)  s_Flags((x), Flags(x) & ~TRACE)

#define Html(x) ((Flags2(x) & HTML) != 0)
#define s_Html(x) s_Flags2((x), Flags2(x) | HTML)
#define c_Html(x) s_Flags2((x), Flags2(x) & ~HTML)

/*
 * ---------------------------------------------------------------------------
 * Base control-oriented predicates.
 */

#define Parentable(p,x) (Controls(p,x) || \
             (Parent_ok(x) && could_doit(p,x,A_LPARENT)))

#define OnControlLock(p,x) (check_zone(p,x))

#define Controls(p,x)   (Good_obj(x) && \
             (!(God(x) && !God(p))) && \
             (Control_All(p) || \
              ((Owner(p) == Owner(x)) && \
               (Inherits(p) || !Inherits(x))) || \
              OnControlLock(p,x)))

#define Cannot_Objeval(p,x)  ((x == NOTHING) || God(x) || \
                  (mudconf.fascist_objeval ? \
                   !Controls(p, x) : \
                   ((Owner(x) != Owner(p)) && !Wizard(p))))

#define Has_power(p,x)  (check_access((p),powers_nametab[x].flag))

#define Mark(x)     (mudstate.markbits->chunk[(x)>>3] |= \
             mudconf.markdata[(x)&7])
#define Unmark(x)   (mudstate.markbits->chunk[(x)>>3] &= \
             ~mudconf.markdata[(x)&7])
#define Marked(x)   (mudstate.markbits->chunk[(x)>>3] & \
             mudconf.markdata[(x)&7])
#define Mark_all(i) for ((i)=0; (i)<((mudstate.db_top+7)>>3); (i)++) \
                mudstate.markbits->chunk[i]=(char)0xff
#define Unmark_all(i)   for ((i)=0; (i)<((mudstate.db_top+7)>>3); (i)++) \
                mudstate.markbits->chunk[i]=(char)0x0

/*
 * ---------------------------------------------------------------------------
 * Visibility constraints.
 */

#define Examinable(p,x) (((Flags(x) & VISUAL) != 0) || \
             (See_All(p)) || \
                         (Owner(p) == Owner(x)) || \
             OnControlLock(p,x))

#define MyopicExam(p,x) (((Flags(x) & VISUAL) != 0) || \
             (!Myopic(p) && (See_All(p) || \
             (Owner(p) == Owner(x)) || \
                     OnControlLock(p,x))))

/*
 * An object is considered Darkened if: It is set Dark. It does not have a
 * DarkLock set (no HAS_DARKLOCK flag on the object; an empty lock means the
 * player would pass the DarkLock), OR the player passes the DarkLock.
 * DarkLocks only apply when we are checking if we can see something on a
 * 'look'. They are not checked when matching, when looking at lexits(), when
 * determining whether a move is seen, on @sweep, etc.
 */

#define Darkened(p,x)   (Dark(x) && \
             (!H_Darklock(x) || could_doit(p,x,A_LDARK)))

/*
 * For an object in a room, don't show if all of the following apply:
 * Sleeping players should not be seen. The thing is a disconnected player.
 * The player is not a puppet. You don't see yourself or exits. If a location
 * is not dark, you see it if it's not dark or you control it. If the
 * location is dark, you see it if you control it. Seeing your own dark
 * objects is controlled by mudconf.see_own_dark. In dark locations, you also
 * see things that are LIGHT and !DARK.
 */

#define Sees(p,x) \
    (!Darkened(p,x) || (mudconf.see_own_dark && MyopicExam(p,x)))

#define Sees_Always(p,x) \
    (!Darkened(p,x) || (mudconf.see_own_dark && Examinable(p,x)))

#define Sees_In_Dark(p,x)   ((Light(x) && !Darkened(p,x)) || \
                 (mudconf.see_own_dark && MyopicExam(p,x)))

#define Can_See(p,x,l)  (!(((p) == (x)) || isExit(x) || \
               (mudconf.dark_sleepers && isPlayer(x) && \
                !Connected(x) && !Puppet(x))) && \
             ((l) ? Sees(p,x) : Sees_In_Dark(p,x)) && \
             ((!Unreal(x) || Check_Known(p,x)) && \
              (!Unreal(p) || Check_Knows(x,p))))

#define Can_See_Exit(p,x,l) (!Darkened(p,x) && (!(l) || Light(x)) && \
                 ((!Unreal(x) || Check_Known(p,x)) && \
                  (!Unreal(p) || Check_Knows(x,p))))


/*
 * For exits visible (for lexits(), etc.), this is true if we can examine the
 * exit's location, examine the exit, or the exit is LIGHT. It is also true
 * if neither the location or base or exit is dark.
 */

#define VE_LOC_XAM  0x01    /* Location is examinable */
#define VE_LOC_DARK 0x02    /* Location is dark */
#define VE_BASE_DARK    0x04    /* Base location (pre-parent) is dark */

#define Exit_Visible(x,p,k) \
    (((k) & VE_LOC_XAM) || Examinable(p,x) || Light(x) || \
     (!((k) & (VE_LOC_DARK | VE_BASE_DARK)) && !Dark(x)))

/*
 * ---------------------------------------------------------------------------
 * Linking.
 */

/* Can I link this exit to something else? */
#define Link_exit(p,x)  ((Typeof(x) == TYPE_EXIT) && \
             ((Location(x) == NOTHING) || Controls(p,x)))

/*
 * Is this something I can link to? - It must be a valid object, and be able
 * to have contents. - I must control it, or have it be Link_ok, or I must
 * the link_to_any power and not have the destination be God.
 */
#define Linkable(p,x)   (Good_obj(x) && Has_contents(x) &&  \
             (Controls(p,x) || Link_ok(x) ||    \
              (LinkToAny(p) && !God(x))))

/*
 * Can I pass the linklock check on this? - I must have link_to_any (or be a
 * wizard) and wizards must ignore linklocks, OR - I must be able to pass the
 * linklock.
 */
#define Passes_Linklock(p,x)  \
((LinkToAny(p) && !mudconf.wiz_obey_linklock) ||  \
 could_doit(p,x,A_LLINK))

/*
 * ---------------------------------------------------------------------------
 * Attribute visibility and write permissions.
 */

#define AttrFlags(a,f)  ((f) | (a)->flags)

#define Visible_desc(p,x,a) \
(((a)->number != A_DESC) || mudconf.read_rem_desc || nearby(p,x))

#define Invisible_attr(p,x,a,o,f) \
((!Examinable(p,x) && (Owner(p) != o)) || \
   ((AttrFlags(a,f) & AF_MDARK) && !Sees_Hidden_Attrs(p)) || \
   ((AttrFlags(a,f) & AF_DARK) && !God(p)))

#define Visible_attr(p,x,a,o,f) \
(((AttrFlags(a,f) & AF_VISUAL) && Visible_desc(p,x,a)) || \
 !Invisible_attr(p,x,a,o,f))

#define See_attr(p,x,a,o,f) \
(!((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) && !(f & AF_STRUCTURE) && \
 Visible_attr(p,x,a,o,f))

#define Read_attr(p,x,a,o,f) \
(!((a)->flags & AF_INTERNAL) && !(f & AF_STRUCTURE) && \
 Visible_attr(p,x,a,o,f))

#define See_attr_all(p,x,a,o,f,y) \
(!((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) && \
 ((y) || !(f &AF_STRUCTURE)) && \
 Visible_attr(p,x,a,o,f))

#define Read_attr_all(p,x,a,o,f,y) \
(!((a)->flags & AF_INTERNAL) && \
 ((y) || !(f &AF_STRUCTURE)) && \
 Visible_attr(p,x,a,o,f))

/*
 * We can set it if: The (master) attribute is not internal or a lock AND
 * we're God OR we meet the following criteria: - The object is not God. -
 * The attribute on the object is not locked. - The object is not set
 * Constant. - We control the object, and the attribute and master attribute
 * do not have the Wizard or God flags, OR We are a Wizard and the attribute
 * and master attribute do not have the God flags.
 */

#define Set_attr(p,x,a,f) \
(!((a)->flags & (AF_INTERNAL|AF_IS_LOCK|AF_CONST)) && \
 (God(p) || \
  (!God(x) && !((f) & AF_LOCK) && !Constant_Attrs(x) && \
   ((Controls(p,x) && \
     !((a)->flags & (AF_WIZARD|AF_GOD)) && \
     !((f) & (AF_WIZARD|AF_GOD))) || \
    (Sets_Wiz_Attrs(p) && !((a)->flags & AF_GOD) && !((f) & AF_GOD))))))

/*
 * Write_attr() is only used by atr_cpy(), and thus is not subject to the
 * effects of the Constant flag.
 */

#define Write_attr(p,x,a,f) \
            (!((a)->flags & (AF_INTERNAL|AF_NOCLONE)) && \
             (God(p) || \
              (!God(x) && !(f & AF_LOCK) && \
               ((Controls(p,x) && \
                 !((a)->flags & (AF_WIZARD|AF_GOD)) && \
                 !((f) & (AF_WIZARD|AF_GOD))) || \
                (Sets_Wiz_Attrs(p) && \
                 !((a)->flags & AF_GOD))))))

/*
 * We can lock/unlock it if: we're God OR we meet the following criteria: -
 * The (master) attribute is not internal or a lock AND - The object is not
 * God. - The object is not set Constant. - The master attribute does not
 * have the Wizard or God flags, OR We are a Wizard and the master attribute
 * does not have the God flag. - We are a Wizard OR we own the attribute.
 */

#define Lock_attr(p,x,a,o) \
(God(p) || \
 (!God(x) && \
  !((a)->flags & (AF_INTERNAL|AF_IS_LOCK|AF_CONST)) && \
  !Constant_Attrs(x) && \
  (!((a)->flags & (AF_WIZARD|AF_GOD)) || \
   (Sets_Wiz_Attrs(p) && !((a)->flags & AF_GOD))) && \
  (Wizard(p) || (o) == Owner(p))))

/* Visibility abstractions */

#define Are_Real(p,t)       (!(Unreal(p) || Unreal(t)))
#define Check_Heard(t,p)    (could_doit((t),(p),A_LHEARD))
#define Check_Noticed(t,p)  (could_doit((t),(p),A_LMOVED))
#define Check_Known(t,p)    (could_doit((t),(p),A_LKNOWN))
#define Check_Hears(p,t)    (could_doit((p),(t),A_LHEARS))
#define Check_Notices(p,t)  (could_doit((p),(t),A_LMOVES))
#define Check_Knows(p,t)    (could_doit((p),(t),A_LKNOWS))

#endif  /* __FLAGS_H */
