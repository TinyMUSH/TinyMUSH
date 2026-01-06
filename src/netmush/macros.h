/**
 * @file macros.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Engine-wide macros for memory tracking, flag checks, logging, and utilities
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#pragma once

/**
 * @brief XMALLOC related macros
 *
 */

// Allocation Functions
#define XMALLOC(s, v) __xmalloc(s, __FILE__, __LINE__, __func__, v)
#define XNMALLOC(s) __xmalloc(s, NULL, 0, NULL, NULL)
#define XCALLOC(n, s, v) __xcalloc(n, s, __FILE__, __LINE__, __func__, v)
#define XNCALLOC(n, s) __xcalloc(n, s, NULL, 0, NULL, NULL)
#define XREALLOC(p, s, v) __xrealloc((void *)(p), s, __FILE__, __LINE__, __func__, v)
#define XNREALLOC(p, s) __xrealloc((void *)(p), s, NULL, 0, NULL, NULL)
#define XFREE(p) __xfree((void *)(p))
#define XHEADER(p) (MEMTRACK *)((char *)(p) - sizeof(MEMTRACK));

// String Functions
#define XASPRINTF(v, f, ...) __xasprintf(__FILE__, __LINE__, __func__, v, (const char *)(f), ##__VA_ARGS__)
#define XAVSPRINTF(v, f, a) __xavsprintf(__FILE__, __LINE__, __func__, v, (const char *)(f), a)
#define XNASPRINTF(f, ...) __xasprintf(NULL, 0, NULL, NULL, (const char *)(f), ##__VA_ARGS__);
#define XSPRINTF(s, f, ...) __xsprintf((char *)(s), (const char *)(f), ##__VA_ARGS__)
#define XSPRINTFCAT(s, f, ...) __xsprintfcat((char *)(s), (const char *)(f), ##__VA_ARGS__)
#define XVSPRINTF(s, f, a) __xvsprintf((char *)(s), (const char *)(f), a)
#define XSNPRINTF(s, n, f, ...) __xsnprintf((char *)(s), n, (const char *)(f), ##__VA_ARGS__)
#define XVSNPRINTF(s, n, f, a) __xvsnprintf((char *)(s), n, (const char *)(f), a)
#define XSTRDUP(s, v) __xstrdup((const char *)(s), __FILE__, __LINE__, __func__, v)
#define XNSTRDUP(s) __xstrdup((const char *)(s), NULL, 0, NULL, NULL)
#define XSTRCAT(d, s) __xstrcat((char *)(d), (const char *)(s))
#define XSTRNCAT(d, s, n) __xstrncat((char *)(d), (const char *)(s), n)
#define XSTRCCAT(d, c) __xstrccat((char *)(d), (const char)(c))
#define XSTRNCCAT(d, c, n) __xstrnccat((char *)(d), (const char)(c), n)
#define XSTRCPY(d, s) __xstrcpy((char *)(d), (const char *)(s))
#define XSTRNCPY(d, s, n) __xstrncpy((char *)(d), (const char *)(s), n)
#define XMEMMOVE(d, s, n) __xmemmove((void *)(d), (void *)(s), n)
#define XMEMCPY(d, s, n) __xmemmove((void *)(d), (void *)(s), n)
#define XMEMSET(s, c, n) __xmemset((void *)(s), c, n)

// Replacement for the safe_* functions.
#define XSAFESPRINTF(s, p, f, ...) __xsafesprintf(s, p, f, ##__VA_ARGS__)
#define XSAFECOPYCHR(c, b, p, s) __xsafestrcatchr(b, p, c, s)

#define XSAFESBCHR(c, b, p) __xsafestrcatchr(b, p, c, SBUF_SIZE - 1)
#define XSAFEMBCHR(c, b, p) __xsafestrcatchr(b, p, c, MBUF_SIZE - 1)
#define XSAFELBCHR(c, b, p) __xsafestrcatchr(b, p, c, LBUF_SIZE - 1)
#define XSAFESBSTR(s, b, p) (s ? __xsafestrncpy(b, p, s, strlen(s), SBUF_SIZE - 1) : 0)
#define XSAFEMBSTR(s, b, p) (s ? __xsafestrncpy(b, p, s, strlen(s), MBUF_SIZE - 1) : 0)
#define XSAFELBSTR(s, b, p) (s ? __xsafestrncpy(b, p, s, strlen(s), LBUF_SIZE - 1) : 0)
#define XSAFESTRCAT(b, p, s, n) (s ? __xsafestrncat(b, p, s, strlen(s), n) : 0)
#define XSAFESTRNCAT(b, p, s, n, z) __xsafestrncat(b, p, s, n, z)

#define XSAFECRLF(b, p) __xsafestrncat(b, p, "\r\n", 2, LBUF_SIZE - 1)
#define XSAFENOTHING(b, p) __xsafestrncat(b, p, "#-1", 3, LBUF_SIZE - 1)
#define XSAFENOPERM(b, p) __xsafestrncat(b, p, "#-1 PERMISSION DENIED", 21, LBUF_SIZE - 1)
#define XSAFENOMATCH(b, p) __xsafestrncat(b, p, "#-1 NO MATCH", 12, LBUF_SIZE - 1)
#define XSAFEBOOL(b, p, n) __xsafestrcatchr((b), (p), ((n) ? '1' : '0'), LBUF_SIZE - 1)

#define XSAFELTOS(d, p, n, s) __xsafeltos(d, p, n, s)


// Misc Macros
#define XGETSIZE(p) (((char *)__xfind(p)->magic - p) - sizeof(char))
#define XCALSIZE(s, d, p) ((s - ((*p) - d)))
#define XLTOS(n) __xasprintf(__FILE__, __LINE__, __func__, "XLTOS", "%ld", n)

/**
 * @brief DB related macros
 *
 */

/**
 * @brief Macros to help deal with batch writes of attribute numbers and objects
 *
 */
#define ATRNUM_BLOCK_SIZE (int)((mushstate.db_block_size - 32) / (2 * sizeof(int) + VNAME_SIZE))
#define ATRNUM_BLOCK_BYTES (int)((ATRNUM_BLOCK_SIZE) * (2 * sizeof(int) + VNAME_SIZE))
#define OBJECT_BLOCK_SIZE (int)((mushstate.db_block_size - 32) / (sizeof(int) + sizeof(DUMPOBJ)))
#define OBJECT_BLOCK_BYTES (int)((OBJECT_BLOCK_SIZE) * (sizeof(int) + sizeof(DUMPOBJ)))
#define ENTRY_NUM_BLOCKS(total, blksize) (int)(total / blksize)
#define ENTRY_BLOCK_STARTS(blk, blksize) (int)(blk * blksize)
#define ENTRY_BLOCK_ENDS(blk, blksize) (int)(blk * blksize) + (blksize - 1)

#define Hasprivs(x) (Royalty(x) || Wizard(x))

#define anum_get(x) (anum_table[(x)])
#define anum_set(x, v) anum_table[(x)] = v

#define TRUE_BOOLEXP ((BOOLEXP *)NULL)

#define Location(t) db[t].location
#define Zone(t) db[t].zone
#define Contents(t) db[t].contents
#define Exits(t) db[t].exits
#define Next(t) db[t].next
#define Link(t) db[t].link
#define Owner(t) db[t].owner
#define Parent(t) db[t].parent
#define Flags(t) db[t].flags
#define Flags2(t) db[t].flags2
#define Flags3(t) db[t].flags3
#define Powers(t) db[t].powers
#define Powers2(t) db[t].powers2
#define NameLen(t) db[t].name_length
#define Home(t) db[t].link
#define Dropto(t) db[t].location
#define AccessTime(t) db[t].last_access
#define ModTime(t) db[t].last_mod
#define CreateTime(t) db[t].create_time
#define VarsCount(t) db[t].vars_count
#define StackCount(t) db[t].stack_count
#define StructCount(t) db[t].struct_count
#define InstanceCount(t) db[t].instance_count
#define Time_Used(t) db[t].cpu_time_used

/**
 * @brief If we modify something on the db object that needs to be written at dump
 * time, set the object DIRTY
 *
 */
#define s_Location(t, n)  \
    db[t].location = (n); \
    db[t].flags3 |= DIRTY
#define s_Zone(t, n)  \
    db[t].zone = (n); \
    db[t].flags3 |= DIRTY
#define s_Contents(t, n)  \
    db[t].contents = (n); \
    db[t].flags3 |= DIRTY
#define s_Exits(t, n)  \
    db[t].exits = (n); \
    db[t].flags3 |= DIRTY
#define s_Next(t, n)  \
    db[t].next = (n); \
    db[t].flags3 |= DIRTY
#define s_Link(t, n)  \
    db[t].link = (n); \
    db[t].flags3 |= DIRTY
#define s_Owner(t, n)  \
    db[t].owner = (n); \
    db[t].flags3 |= DIRTY
#define s_Parent(t, n)  \
    db[t].parent = (n); \
    db[t].flags3 |= DIRTY
#define s_Flags(t, n)  \
    db[t].flags = (n); \
    db[t].flags3 |= DIRTY
#define s_Flags2(t, n)  \
    db[t].flags2 = (n); \
    db[t].flags3 |= DIRTY
#define s_Flags3(t, n)  \
    db[t].flags3 = (n); \
    db[t].flags3 |= DIRTY
#define s_Powers(t, n)  \
    db[t].powers = (n); \
    db[t].flags3 |= DIRTY
#define s_Powers2(t, n)  \
    db[t].powers2 = (n); \
    db[t].flags3 |= DIRTY
#define s_AccessTime(t, n)   \
    db[t].last_access = (n); \
    db[t].flags3 |= DIRTY
#define s_ModTime(t, n)   \
    db[t].last_mod = (n); \
    db[t].flags3 |= DIRTY
#define s_CreateTime(t, n)   \
    db[t].create_time = (n); \
    db[t].flags3 |= DIRTY
#define s_Accessed(t)                  \
    db[t].last_access = mushstate.now; \
    db[t].flags3 |= DIRTY
#define s_Modified(t)               \
    db[t].last_mod = mushstate.now; \
    db[t].flags3 |= DIRTY
#define s_Created(t)                   \
    db[t].create_time = mushstate.now; \
    db[t].flags3 |= DIRTY

#define s_Clean(t) db[t].flags3 = db[t].flags3 & ~DIRTY
#define s_NameLen(t, n) db[t].name_length = (n)
#define s_Home(t, n) s_Link(t, n)
#define s_Dropto(t, n) s_Location(t, n)
#define s_VarsCount(t, n) db[t].vars_count = n;
#define s_StackCount(t, n) db[t].stack_count = n;
#define s_StructCount(t, n) db[t].struct_count = n;
#define s_InstanceCount(t, n) db[t].instance_count = n;

#define Dropper(thing) (Connected(Owner(thing)) && Hearer(thing))

/**
 * Sizes, on disk, of Object and (within the object) Attribute headers
 *
 */
#define OBJ_HEADER_SIZE (sizeof(unsigned int) + sizeof(int))
#define ATTR_HEADER_SIZE (sizeof(int) * 2)

/**
 * @brief Messages related macros.
 *
 */
#define MSG_ME_ALL (MSG_ME | MSG_INV_EXITS | MSG_FWDLIST)
#define MSG_F_CONTENTS (MSG_INV)
#define MSG_F_UP (MSG_NBR_A | MSG_LOC_A)
#define MSG_F_DOWN (MSG_INV_L)
/**
 * @brief Log related macros.
 *
 */
#define log_write(i, p, s, f, ...) _log_write(__FILE__, __LINE__, i, p, s, f, ##__VA_ARGS__)
#define log_perror(p, s, e, o) _log_perror(__FILE__, __LINE__, p, s, e, o)
/**
 * A zillion ways to notify things.
 *
 */
#define notify(p, m, ...) notify_check(p, p, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, NULL, m, ##__VA_ARGS__)
#define notify_html(p, m, ...) notify_check(p, p, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN | MSG_HTML, NULL, m, ##__VA_ARGS__)
#define notify_quiet(p, m, ...) notify_check(p, p, MSG_PUP_ALWAYS | MSG_ME, NULL, m, ##__VA_ARGS__)
#define notify_with_cause(p, c, m, ...) notify_check(p, c, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, NULL, m, ##__VA_ARGS__)
#define notify_with_cause_html(p, c, m, ...) notify_check(p, c, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN | MSG_HTML, NULL, m, ##__VA_ARGS__)
#define notify_with_cause_extra(p, c, m, f, ...) notify_check(p, c, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN | (f), NULL, m, ##__VA_ARGS__)
#define notify_quiet_with_cause(p, c, m, ...) notify_check(p, c, MSG_PUP_ALWAYS | MSG_ME, NULL, m, ##__VA_ARGS__)
#define notify_puppet(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_F_DOWN, NULL, m, ##__VA_ARGS__)
#define notify_quiet_puppet(p, c, m, ...) notify_check(p, c, MSG_ME, NULL, m, ##__VA_ARGS__)
#define notify_all(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS | MSG_F_UP | MSG_F_CONTENTS, NULL, m, ##__VA_ARGS__)
#define notify_all_from_inside(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE, NULL, m, ##__VA_ARGS__)
#define notify_all_from_inside_speech(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_SPEECH, NULL, m, ##__VA_ARGS__)
#define notify_all_from_inside_move(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_MOVE, NULL, m, ##__VA_ARGS__)
#define notify_all_from_inside_html(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_HTML, NULL, m, ##__VA_ARGS__)
#define notify_all_from_inside_html_speech(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS_A | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | MSG_HTML | MSG_SPEECH, NULL, m, ##__VA_ARGS__)
#define notify_all_from_outside(p, c, m, ...) notify_check(p, c, MSG_ME_ALL | MSG_NBR_EXITS | MSG_F_UP | MSG_F_CONTENTS | MSG_S_OUTSIDE, NULL, m, ##__VA_ARGS__)
/**
 * General macros.
 *
 */
// #define Randomize(n) (random_range(0, (n)-1))
#define Protect(f) (cmdp->perms & f)
#define Invalid_Objtype(x) ((Protect(CA_LOCATION) && !Has_location(x)) || (Protect(CA_CONTENTS) && !Has_contents(x)) || (Protect(CA_PLAYER) && (Typeof(x) != TYPE_PLAYER)))
#define test_top() ((mushstate.qfirst != NULL) ? 1 : 0)
#define controls(p, x) Controls(p, x)

/**
 * @brief Flags related macros
 *
 */

#define GOD ((dbref)1)

/**
 * Object Permission/Attribute Macros:
 *
 * IS(X,T,F)            - Is X of type T and have flag F set?
 * Typeof(X)            - What object type is X
 * God(X)               - Is X player #1
 * Robot(X)             - Is X a robot player
 * Wizard(X)            - Does X have wizard privs
 * Immortal(X)          - Is X unkillable
 * Alive(X)             - Is X a player or a puppet
 * Dark(X)              - Is X dark
 * Quiet(X)             - Should 'Set.' messages et al from X be disabled
 * Verbose(X)           - Should owner receive all commands executed?
 * Trace(X)             - Should owner receive eval trace output?
 * Player_haven(X)      - Is the owner of X no-page
 * Haven(X)             - Is X no-kill(rooms) or no-page(players)
 * Halted(X)            - Is X halted (not allowed to run commands)?
 * Suspect(X)           - Is X someone the wizzes should keep an eye on
 * Slave(X)             - Should X be prevented from db-changing commands
 * Safe(X,P)            - Does P need the /OVERRIDE switch to \@destroy X?
 * Monitor(X)           - Should we check for ^xxx:xxx listens on player?
 * Terse(X)             - Should we only show the room name on a look?
 * Myopic(X)            - Should things as if we were nonowner/nonwiz
 * Audible(X)           - Should X forward messages?
 * Findroom(X)          - Can players in room X be found via \@whereis?
 * Unfindroom(X)        - Is \@whereis blocked for players in room X?
 * Findable(X)          - Can \@whereis find X
 * Unfindable(X)        - Is \@whereis blocked for X
 * No_robots(X)         - Does X disallow robot players from using
 * Has_location(X)      - Is X something with a location (ie plyr or obj)
 * Has_home(X)          - Is X something with a home (ie plyr or obj)
 * Has_contents(X)      - Is X something with contents (ie plyr/obj/room)
 * Good_dbref(X)        - Is X inside the DB?
 * Good_obj(X)          - Is X inside the DB and have a valid type?
 * Good_owner(X)        - Is X a good owner value?
 * Good_home(X)         - Is X a good home value?
 * Good_loc(X)          - Is X a good location value?
 * Going(X)             - Is X marked GOING?
 * Inherits(X)          - Does X inherit the privs of its owner
 * Examinable(P,X)      - Can P look at attribs of X
 * MyopicExam(P,X)      - Can P look at attribs of X (obeys MYOPIC)
 * Controls(P,X)        - Can P force X to do something
 * Darkened(P,X)        - Is X dark, and does P pass its DarkLock?
 * Can_See(P,X,L)       - Can P see thing X, depending on location dark (L)?
 * Can_See_Exit(P,X,L)  - Can P see exit X, depending on loc dark (L)?
 * Abode(X)             - Is X an ABODE room
 * Link_exit(P,X)       - Can P link from exit X
 * Linkable(P,X)        - Can P link to X
 * Mark(x)              - Set marked flag on X
 * Unmark(x)            - Clear marked flag on X
 * Marked(x)            - Check marked flag on X
 * See_attr(P,X,A,O,F)  - Can P see text attr A on X if attr has owner O
 * Set_attr(P,X,A,F)    - Can P set/change text attr A (with flags F) on X
 * Read_attr(P,X,A,O,F) - Can P see attr A on X if attr has owner O
 * Write_attr(P,X,A,F)  - Can P set/change attr A (with flags F) on X
 * Lock_attr(P,X,A,O)   - Can P lock/unlock attr A (with owner O) on X
 *
 */

#define IS(thing, type, flag) ((Typeof(thing) == (type)) && (Flags(thing) & (flag)))
#define Typeof(x) (Flags(x) & TYPE_MASK)
#define God(x) ((x) == GOD)
#define Robot(x) (isPlayer(x) && ((Flags(x) & ROBOT) != 0))
#define Alive(x) (isPlayer(x) || (Puppet(x) && Has_contents(x)))
#define OwnsOthers(x) ((object_types[Typeof(x)].flags & OF_OWNER) != 0)
#define Has_location(x) ((object_types[Typeof(x)].flags & OF_LOCATION) != 0)
#define Has_contents(x) ((object_types[Typeof(x)].flags & OF_CONTENTS) != 0)
#define Has_exits(x) ((object_types[Typeof(x)].flags & OF_EXITS) != 0)
#define Has_siblings(x) ((object_types[Typeof(x)].flags & OF_SIBLINGS) != 0)
#define Has_home(x) ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define Has_dropto(x) ((object_types[Typeof(x)].flags & OF_DROPTO) != 0)
#define Home_ok(x) ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define isPlayer(x) (Typeof(x) == TYPE_PLAYER)
#define isRoom(x) (Typeof(x) == TYPE_ROOM)
#define isExit(x) (Typeof(x) == TYPE_EXIT)
#define isThing(x) (Typeof(x) == TYPE_THING)
#define isGarbage(x) (Typeof(x) == TYPE_GARBAGE)

#define Good_dbref(x) (((x) >= 0) && ((x) < mushstate.db_top))
#define Good_obj(x) (Good_dbref(x) && (Typeof(x) < GOODTYPE))
#define Good_owner(x) (Good_obj(x) && OwnsOthers(x))
#define Good_home(x) (Good_obj(x) && Home_ok(x))
#define Good_loc(x) (Good_obj(x) && Has_contents(x))

#define Royalty(x) ((Flags(x) & ROYALTY) != 0)
#define WizRoy(x) (Royalty(x) || Wizard(x))
#define Staff(x) ((Flags2(x) & STAFF) != 0)
#define Head(x) ((Flags2(x) & HEAD_FLAG) != 0)
#define Fixed(x) ((Flags2(x) & FIXED) != 0)
#define Uninspected(x) ((Flags2(x) & UNINSPECTED) != 0)
#define Ansi(x) ((Flags2(x) & ANSI) != 0)
#define Color256(x) ((Flags3(x) & COLOR256) != 0)
#define Color24Bit(x) ((Flags3(x) & COLOR24BIT) != 0)
#define NoBleed(x) ((Flags2(x) & NOBLEED) != 0)

#define Transparent(x) ((Flags(x) & SEETHRU) != 0)
#define Link_ok(x) (((Flags(x) & LINK_OK) != 0) && Has_contents(x))
#define Open_ok(x) (((Flags3(x) & OPEN_OK) != 0) && Has_contents(x))
#define Wizard(x) ((Flags(x) & WIZARD) || ((Flags(Owner(x)) & WIZARD) && Inherits(x)))
#define Dark(x) (((Flags(x) & DARK) != 0) && (!Alive(x) || (Wizard(x) && !mushconf.visible_wizzes) || Can_Cloak(x)))
#define DarkMover(x) ((Wizard(x) || Can_Cloak(x)) && Dark(x))
#define Jump_ok(x) (((Flags(x) & JUMP_OK) != 0) && Has_contents(x))
#define Sticky(x) ((Flags(x) & STICKY) != 0)
#define Destroy_ok(x) ((Flags(x) & DESTROY_OK) != 0)
#define Haven(x) ((Flags(x) & HAVEN) != 0)
#define Player_haven(x) ((Flags(Owner(x)) & HAVEN) != 0)
#define Quiet(x) ((Flags(x) & QUIET) != 0)
#define Halted(x) ((Flags(x) & HALT) != 0)
#define Trace(x) ((Flags(x) & TRACE) != 0)
#define Going(x) ((Flags(x) & GOING) != 0)
#define Monitor(x) ((Flags(x) & MONITOR) != 0)
#define Myopic(x) ((Flags(x) & MYOPIC) != 0)
#define Puppet(x) ((Flags(x) & PUPPET) != 0)
#define Chown_ok(x) ((Flags(x) & CHOWN_OK) != 0)
#define Enter_ok(x) (((Flags(x) & ENTER_OK) != 0) && Has_location(x) && Has_contents(x))
#define Visual(x) ((Flags(x) & VISUAL) != 0)
#define Immortal(x) ((Flags(x) & IMMORTAL) || ((Flags(Owner(x)) & IMMORTAL) && Inherits(x)))
#define Opaque(x) ((Flags(x) & OPAQUE) != 0)
#define Verbose(x) ((Flags(x) & VERBOSE) != 0)
#define Inherits(x) (((Flags(x) & INHERIT) != 0) || ((Flags(Owner(x)) & INHERIT) != 0) || ((x) == Owner(x)))
#define Nospoof(x) ((Flags(x) & NOSPOOF) != 0)
#define Safe(x, p) (OwnsOthers(x) || (Flags(x) & SAFE) || (mushconf.safe_unowned && (Owner(x) != Owner(p))))
#define Control_ok(x) ((Flags2(x) & CONTROL_OK) != 0)
#define Constant_Attrs(x) ((Flags2(x) & CONSTANT_ATTRS) != 0)
#define Audible(x) ((Flags(x) & HEARTHRU) != 0)
#define Terse(x) ((Flags(x) & TERSE) != 0)

#define Gagged(x) ((Flags2(x) & GAGGED) != 0)
#define Vacation(x) ((Flags2(x) & VACATION) != 0)
#define Sending_Mail(x) ((Flags2(x) & PLAYER_MAILS) != 0)
#define Key(x) ((Flags2(x) & KEY) != 0)
#define Abode(x) (((Flags2(x) & ABODE) != 0) && Home_ok(x))
#define Auditorium(x) ((Flags2(x) & AUDITORIUM) != 0)
#define Findable(x) ((Flags2(x) & UNFINDABLE) == 0)
#define Hideout(x) ((Flags2(x) & UNFINDABLE) != 0)
#define Parent_ok(x) ((Flags2(x) & PARENT_OK) != 0)
#define Light(x) ((Flags2(x) & LIGHT) != 0)
#define Suspect(x) ((Flags2(Owner(x)) & SUSPECT) != 0)
#define Watcher(x) ((Flags2(x) & WATCHER) != 0)
#define Connected(x) (((Flags2(x) & CONNECTED) != 0) && (Typeof(x) == TYPE_PLAYER))
#define Slave(x) ((Flags2(Owner(x)) & SLAVE) != 0)
#define ParentZone(x) ((Flags2(x) & ZONE_PARENT) != 0)
#define Stop_Match(x) ((Flags2(x) & STOP_MATCH) != 0)
#define Has_Commands(x) ((Flags2(x) & HAS_COMMANDS) != 0)
#define Bouncer(x) ((Flags2(x) & BOUNCE) != 0)
#define Hidden(x) ((Flags(x) & DARK) != 0)
#define Blind(x) ((Flags2(x) & BLIND) != 0)
#define Redir_ok(x) ((Flags3(x) & REDIR_OK) != 0)
#define Orphan(x) ((Flags3(x) & ORPHAN) != 0)
#define NoDefault(x) ((Flags3(x) & NODEFAULT) != 0)
#define Unreal(x) ((Flags3(x) & PRESENCE) != 0)

#define H_Startup(x) ((Flags(x) & HAS_STARTUP) != 0)
#define H_Fwdlist(x) ((Flags2(x) & HAS_FWDLIST) != 0)
#define H_Listen(x) ((Flags2(x) & HAS_LISTEN) != 0)
#define H_Redirect(x) ((Flags3(x) & HAS_REDIRECT) != 0)
#define H_Darklock(x) ((Flags3(x) & HAS_DARKLOCK) != 0)
#define H_Speechmod(x) ((Flags3(x) & HAS_SPEECHMOD) != 0)
#define H_Propdir(x) ((Flags3(x) & HAS_PROPDIR) != 0)

#define H_Marker0(x) ((Flags3(x) & MARK_0) != 0)
#define H_Marker1(x) ((Flags3(x) & MARK_1) != 0)
#define H_Marker2(x) ((Flags3(x) & MARK_2) != 0)
#define H_Marker3(x) ((Flags3(x) & MARK_3) != 0)
#define H_Marker4(x) ((Flags3(x) & MARK_4) != 0)
#define H_Marker5(x) ((Flags3(x) & MARK_5) != 0)
#define H_Marker6(x) ((Flags3(x) & MARK_6) != 0)
#define H_Marker7(x) ((Flags3(x) & MARK_7) != 0)
#define H_Marker8(x) ((Flags3(x) & MARK_8) != 0)
#define H_Marker9(x) ((Flags3(x) & MARK_9) != 0)
#define isMarkerFlag(fp) (((fp)->flagflag & FLAG_WORD3) && ((fp)->flagvalue & MARK_FLAGS))

#define s_Halted(x) s_Flags((x), Flags(x) | HALT)
#define s_Going(x) s_Flags((x), Flags(x) | GOING)
#define s_Connected(x) s_Flags2((x), Flags2(x) | CONNECTED)
#define c_Connected(x) s_Flags2((x), Flags2(x) & ~CONNECTED)
#define isConnFlag(fp) (((fp)->flagflag & FLAG_WORD2) && ((fp)->flagvalue & CONNECTED))
#define s_Has_Darklock(x) s_Flags3((x), Flags3(x) | HAS_DARKLOCK)
#define c_Has_Darklock(x) s_Flags3((x), Flags3(x) & ~HAS_DARKLOCK)
#define s_Trace(x) s_Flags((x), Flags(x) | TRACE)
#define c_Trace(x) s_Flags((x), Flags(x) & ~TRACE)

#define Html(x) ((Flags2(x) & HTML) != 0)
#define s_Html(x) s_Flags2((x), Flags2(x) | HTML)
#define c_Html(x) s_Flags2((x), Flags2(x) & ~HTML)

/**
 * @brief Base control-oriented predicates.
 *
 */
#define Parentable(p, x) (Controls(p, x) || (Parent_ok(x) && could_doit(p, x, A_LPARENT)))
#define OnControlLock(p, x) (check_zone(p, x))
#define Controls(p, x) (Good_obj(x) && (!(God(x) && !God(p))) && (Control_All(p) || ((Owner(p) == Owner(x)) && (Inherits(p) || !Inherits(x))) || OnControlLock(p, x)))
#define Cannot_Objeval(p, x) ((x == NOTHING) || God(x) || (mushconf.fascist_objeval ? !Controls(p, x) : ((Owner(x) != Owner(p)) && !Wizard(p))))
#define Has_power(p, x) (check_access((p), powers_nametab[x].flag))
#define Mark(x) (mushstate.markbits[(x) >> 3] |= mushconf.markdata[(x) & 7])
#define Unmark(x) (mushstate.markbits[(x) >> 3] &= ~mushconf.markdata[(x) & 7])
#define Marked(x) (mushstate.markbits[(x) >> 3] & mushconf.markdata[(x) & 7])

/**
 * @brief Visibility constraints.
 *
 */
#define Examinable(p, x) (((Flags(x) & VISUAL) != 0) || (See_All(p)) || (Owner(p) == Owner(x)) || OnControlLock(p, x))
#define MyopicExam(p, x) (((Flags(x) & VISUAL) != 0) || (!Myopic(p) && (See_All(p) || (Owner(p) == Owner(x)) || OnControlLock(p, x))))

/**
 * @brief An object is considered Darkened if: It is set Dark. It does not have a
 * DarkLock set (no HAS_DARKLOCK flag on the object; an empty lock means the
 * player would pass the DarkLock), OR the player passes the DarkLock.
 * DarkLocks only apply when we are checking if we can see something on a
 * 'look'. They are not checked when matching, when looking at lexits(), when
 * determining whether a move is seen, on \@sweep, etc.
 *
 */
#define Darkened(p, x) (Dark(x) && (!H_Darklock(x) || could_doit(p, x, A_LDARK)))

/**
 * @brief For an object in a room, don't show if all of the following apply:
 * Sleeping players should not be seen. The thing is a disconnected player.
 * The player is not a puppet. You don't see yourself or exits. If a location
 * is not dark, you see it if it's not dark or you control it. If the
 * location is dark, you see it if you control it. Seeing your own dark
 * objects is controlled by mushconf.see_own_dark. In dark locations, you also
 * see things that are LIGHT and !DARK.
 *
 */
#define Sees(p, x) (!Darkened(p, x) || (mushconf.see_own_dark && MyopicExam(p, x)))
#define Sees_Always(p, x) (!Darkened(p, x) || (mushconf.see_own_dark && Examinable(p, x)))
#define Sees_In_Dark(p, x) ((Light(x) && !Darkened(p, x)) || (mushconf.see_own_dark && MyopicExam(p, x)))
#define Can_See(p, x, l) (!(((p) == (x)) || isExit(x) || (mushconf.dark_sleepers && isPlayer(x) && !Connected(x) && !Puppet(x))) && ((l) ? Sees(p, x) : Sees_In_Dark(p, x)) && ((!Unreal(x) || Check_Known(p, x)) && (!Unreal(p) || Check_Knows(x, p))))
#define Can_See_Exit(p, x, l) (!Darkened(p, x) && (!(l) || Light(x)) && ((!Unreal(x) || Check_Known(p, x)) && (!Unreal(p) || Check_Knows(x, p))))

/**
 * @brief nearby_or_control: Check if player is near or controls thing
 *
 */
#define nearby_or_control(p, t) (Good_obj(p) && Good_obj(t) && (Controls(p, t) || nearby(p, t)))

/**
 * @brief For exits visible (for lexits(), etc.), this is true if we can examine the
 * exit's location, examine the exit, or the exit is LIGHT. It is also true
 * if neither the location or base or exit is dark.
 *
 */
#define Exit_Visible(x, p, k) (((k) & VE_LOC_XAM) || Examinable(p, x) || Light(x) || (!((k) & (VE_LOC_DARK | VE_BASE_DARK)) && !Dark(x)))

/**
 * @brief Linking.
 *
 */

/**
 * @brief Can I link this exit to something else?
 *
 */
#define Link_exit(p, x) ((Typeof(x) == TYPE_EXIT) && ((Location(x) == NOTHING) || Controls(p, x)))

/**
 * @brief Is this something I can link to? - It must be a valid object, and be able
 * to have contents. - I must control it, or have it be Link_ok, or I must
 * the link_to_any power and not have the destination be God.
 *
 */
#define Linkable(p, x) (Good_obj(x) && Has_contents(x) && (Controls(p, x) || Link_ok(x) || (LinkToAny(p) && !God(x))))

/**
 * @brief Can I pass the linklock check on this? - I must have link_to_any (or be a
 * wizard) and wizards must ignore linklocks, OR - I must be able to pass the
 * linklock.
 *
 */
#define Passes_Linklock(p, x) ((LinkToAny(p) && !mushconf.wiz_obey_linklock) || could_doit(p, x, A_LLINK))

/**
 * @brief Is this somewhere I can open exits from? - It must be a valid object, and be able
 * to have contents. - I must control it, or have it be Open_ok, or I must have
 * the open_anywhere power and not have the location be God.
 *
 */
#define Openable(p, x) (Good_obj(x) && Has_contents(x) && (Controls(p, x) || Open_ok(x) || (Open_Anywhere(p) && !God(x))))

/**
 * @brief Can I pass the openlock check on this? - I must have open_anywhere (or be a
 * wizard) and wizards must ignore openlocks, OR - I must be able to pass the
 * openlock.
 *
 */
#define Passes_Openlock(p, x) ((Open_Anywhere(p) && !mushconf.wiz_obey_openlock) || could_doit(p, x, A_LOPEN))

/**
 * @brief Attribute visibility and write permissions.
 *
 */
#define AttrFlags(a, f) ((f) | (a)->flags)
#define Visible_desc(p, x, a) (((a)->number != A_DESC) || mushconf.read_rem_desc || nearby(p, x))
#define Invisible_attr(p, x, a, o, f) ((!Examinable(p, x) && (Owner(p) != o)) || ((AttrFlags(a, f) & AF_MDARK) && !Sees_Hidden_Attrs(p)) || ((AttrFlags(a, f) & AF_DARK) && !God(p)))
#define Visible_attr(p, x, a, o, f) (((AttrFlags(a, f) & AF_VISUAL) && Visible_desc(p, x, a)) || !Invisible_attr(p, x, a, o, f))
#define See_attr(p, x, a, o, f) (!((a)->flags & (AF_INTERNAL | AF_IS_LOCK)) && !(f & AF_STRUCTURE) && Visible_attr(p, x, a, o, f))
#define Read_attr(p, x, a, o, f) (!((a)->flags & AF_INTERNAL) && !(f & AF_STRUCTURE) && Visible_attr(p, x, a, o, f))
#define See_attr_all(p, x, a, o, f, y) (!((a)->flags & (AF_INTERNAL | AF_IS_LOCK)) && ((y) || !(f & AF_STRUCTURE)) && Visible_attr(p, x, a, o, f))
#define Read_attr_all(p, x, a, o, f, y) (!((a)->flags & AF_INTERNAL) && ((y) || !(f & AF_STRUCTURE)) && Visible_attr(p, x, a, o, f))

/**
 * @brief We can set it if: The (master) attribute is not internal or a lock AND
 * we're God OR we meet the following criteria: - The object is not God. -
 * The attribute on the object is not locked. - The object is not set
 * Constant. - We control the object, and the attribute and master attribute
 * do not have the Wizard or God flags, OR We are a Wizard and the attribute
 * and master attribute do not have the God flags.
 *
 */
#define Set_attr(p, x, a, f) (!((a)->flags & (AF_INTERNAL | AF_IS_LOCK | AF_CONST)) && (God(p) || (!God(x) && !((f) & AF_LOCK) && !Constant_Attrs(x) && ((Controls(p, x) && !((a)->flags & (AF_WIZARD | AF_GOD)) && !((f) & (AF_WIZARD | AF_GOD))) || (Sets_Wiz_Attrs(p) && !((a)->flags & AF_GOD) && !((f) & AF_GOD))))))

/**
 * @brief Write_attr() is only used by atr_cpy(), and thus is not subject to the
 * effects of the Constant flag.
 *
 */
#define Write_attr(p, x, a, f) (!((a)->flags & (AF_INTERNAL | AF_NOCLONE)) && (God(p) || (!God(x) && !(f & AF_LOCK) && ((Controls(p, x) && !((a)->flags & (AF_WIZARD | AF_GOD)) && !((f) & (AF_WIZARD | AF_GOD))) || (Sets_Wiz_Attrs(p) && !((a)->flags & AF_GOD))))))

/**
 * @brief We can lock/unlock it if: we're God OR we meet the following criteria: -
 * The (master) attribute is not internal or a lock AND - The object is not
 * God. - The object is not set Constant. - The master attribute does not
 * have the Wizard or God flags, OR We are a Wizard and the master attribute
 * does not have the God flag. - We are a Wizard OR we own the attribute.
 *
 */
#define Lock_attr(p, x, a, o) (God(p) || (!God(x) && !((a)->flags & (AF_INTERNAL | AF_IS_LOCK | AF_CONST)) && !Constant_Attrs(x) && (!((a)->flags & (AF_WIZARD | AF_GOD)) || (Sets_Wiz_Attrs(p) && !((a)->flags & AF_GOD))) && (Wizard(p) || (o) == Owner(p))))

/**
 * @brief Visibility abstractions
 *
 */
#define Are_Real(p, t) (!(Unreal(p) || Unreal(t)))
#define Check_Heard(t, p) (could_doit((t), (p), A_LHEARD))
#define Check_Noticed(t, p) (could_doit((t), (p), A_LMOVED))
#define Check_Known(t, p) (could_doit((t), (p), A_LKNOWN))
#define Check_Hears(p, t) (could_doit((p), (t), A_LHEARS))
#define Check_Notices(p, t) (could_doit((p), (t), A_LMOVES))
#define Check_Knows(p, t) (could_doit((p), (t), A_LKNOWS))

/**
 * @brief Functions related macros
 *
 */

/**
 * @brief Miscellaneous macros.
 *
 * Get function flags. Note that Is_Func() and Func_Mask() are identical;
 * they are given specific names for code clarity.
 *
 */
#define Func_Flags(x) (((FUN *)(x)[-1])->flags)
#define Is_Func(x) (((FUN *)fargs[-1])->flags & (x))
#define Func_Mask(x) (((FUN *)fargs[-1])->flags & (x))

#define IS_CLEAN(i) (IS(i, TYPE_GARBAGE, GOING) && (Location(i) == NOTHING) && (Contents(i) == NOTHING) && (Exits(i) == NOTHING) && (Next(i) == NOTHING) && (Owner(i) == GOD))
/**
 * Check access to built-in function.
 *
 */
#define Check_Func_Access(p, f) (check_access(p, (f)->perms) && (!((f)->xperms) || check_mod_access(p, (f)->xperms)))
/**
 * Trim spaces.
 *
 */
#define Eat_Spaces(x) trim_space_sep((x), &SPACE_DELIM)
/**
 * @brief Handling CPU time checking.
 *
 * @note CPU time "clock()" compatibility notes:
 *
 * Linux clock() doesn't necessarily start at 0. BSD clock() does appear to
 * always start at 0.
 *
 * Linux sets CLOCKS_PER_SEC to 1000000, citing POSIX, so its clock() will wrap
 * around from (32-bit) INT_MAX to INT_MIN every 72 cpu-minutes or so. The
 * actual clock resolution is low enough that, for example, it probably never
 * returns odd numbers.
 *
 * BSD sets CLOCKS_PER_SEC to 100, so theoretically I could hose a cpu for 250
 * days and see what it does when it hits INT_MAX. Any bets? Any possible
 * reason to care?
 *
 * NetBSD clock() can occasionally decrease as the scheduler's estimate of how
 * much cpu the mush will use during the current timeslice is revised, so we
 * can't use subtraction.
 *
 * BSD clock() returns -1 if there is an error.
 *
 * @note CPU time logic notes:
 *
 * B = mushstate.cputime_base L = mushstate.cputime_base + mushconf.func_cpu_lim N = mushstate.cputime_now
 *
 * Assuming B != -1 and N != -1 to catch errors on BSD, the possible
 * combinations of these values are as follows (note >> means "much greater
 * than", not right shift):
 *
 * 1. B <  L  normal    -- limit should be checked, and is not wrapped yet
 * 2. B == L  disabled  -- limit should not be checked
 * 3. B >  L  strange   -- probably misconfigured
 * 4. B >> L  wrapped   -- limit should be checked, and note L wrapped
 *
 * 1.  normal:
 *  1a. N << B          -- too much, N wrapped
 *  1b. N <  B          -- fine, NetBSD counted backwards
 *  1c. N >= B, N <= L  -- fine
 *  1d. N >  L          -- too much
 *
 * 2.  disabled:
 *  2a. always          -- fine, not checking
 *
 * 3.  strange:
 *  3a. always          -- fine, I guess we shouldn't check
 *
 * 4.  wrapped:
 *  4a. N <= L          -- fine, N wrapped but not over limit yet
 *  4b. N >  L, N << B  -- too much, N wrapped
 *  4c. N <  B          -- fine, NetBSD counted backwards
 *  4d. N >= B          -- fine
 *
 * Note that 1a, 1d, and 4b are the cases where we can be certain that too much
 * cpu has been used. The code below only checks for 1d. The other two are
 * corner cases that require some distinction between "x > y" and "x >> y".
 */
#define Too_Much_CPU() ((mushstate.cputime_now = clock()), ((mushstate.cputime_now > mushstate.cputime_base + mushconf.func_cpu_lim) && (mushstate.cputime_base + mushconf.func_cpu_lim > mushstate.cputime_base) && (mushstate.cputime_base != -1) && (mushstate.cputime_now != -1)))

/**
 * @brief Database macros
 *
 */

#define MANDFLAGS (V_LINK | V_PARENT | V_XFLAGS | V_ZONE | V_POWERS | V_3FLAGS | V_QUOTED | V_TQUOTAS | V_TIMESTAMPS | V_VISUALATTRS | V_CREATETIME)
#define OFLAGS1 (V_GDBM | V_ATRKEY)                  /*!< GDBM has these */
#define OFLAGS2 (V_ATRNAME | V_ATRMONEY)             /*!< */
#define OUTPUT_FLAGS (MANDFLAGS | OFLAGS1 | OFLAGS2) /*!< format for dumps */
#define UNLOAD_OUTFLAGS (MANDFLAGS)                  /*!< format for export */

/**
 * @brief HTab macros
 *
 */

#define nhashinit(h, sz) hashinit((h), (sz), HT_NUM)
#define nhashreset(h) hashreset((h))
#define hashfind(s, h) hashfind_generic((HASHKEY)(s), (h))
#define nhashfind(n, h) hashfind_generic((HASHKEY)(n), (h))
#define hashfindflags(s, h) hashfindflags_generic((HASHKEY)(s), (h))
#define hashadd(s, d, h, f) hashadd_generic((HASHKEY)(s), (d), (h), (f))
#define nhashadd(n, d, h) hashadd_generic((HASHKEY)(n), (d), (h), 0)
#define hashdelete(s, h) hashdelete_generic((HASHKEY)(s), (h))
#define nhashdelete(n, h) hashdelete_generic((HASHKEY)(n), (h))
#define nhashflush(h, sz) hashflush((h), (sz))
#define hashrepl(s, d, h) hashrepl_generic((HASHKEY)(s), (d), (h))
#define nhashrepl(n, d, h) hashrepl_generic((HASHKEY)(n), (d), (h))
#define nhashinfo(t, h) hashinfo((t), (h))
#define hash_firstkey(h) (hash_firstkey_generic((h)).s)
#define hash_nextkey(h) (hash_nextkey_generic((h)).s)
#define nhashresize(h, sz) hashresize((h), (sz))

/**
 * @brief MUSH macros
 *
 */

#define OBLOCK_SIZE (dbref)((LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref))

/**
 * @brief Powers macros
 *
 */

#define s_Change_Quotas(c) s_Powers((c), Powers(c) | POW_CHG_QUOTAS)
#define s_Chown_Any(c) s_Powers((c), Powers(c) | POW_CHOWN_ANY)
#define s_Announce(c) s_Powers((c), Powers(c) | POW_ANNOUNCE)
#define s_Can_Boot(c) s_Powers((c), Powers(c) | POW_BOOT)
#define s_Can_Halt(c) s_Powers((c), Powers(c) | POW_HALT)
#define s_Control_All(c) s_Powers((c), Powers(c) | POW_CONTROL_ALL)
#define s_Wizard_Who(c) s_Powers((c), Powers(c) | POW_WIZARD_WHO)
#define s_See_All(c) s_Powers((c), Powers(c) | POW_EXAM_ALL)
#define s_Find_Unfindable(c) s_Powers((c), Powers(c) | POW_FIND_UNFIND)
#define s_Free_Money(c) s_Powers((c), Powers(c) | POW_FREE_MONEY)
#define s_Free_Quota(c) s_Powers((c), Powers(c) | POW_FREE_QUOTA)
#define s_Can_Hide(c) s_Powers((c), Powers(c) | POW_HIDE)
#define s_Can_Idle(c) s_Powers((c), Powers(c) | POW_IDLE)
#define s_Search(c) s_Powers((c), Powers(c) | POW_SEARCH)
#define s_Long_Fingers(c) s_Powers((c), Powers(c) | POW_LONGFINGERS)
#define s_Prog(c) s_Powers((c), Powers(c) | POW_PROG)
#define s_Comm_All(c) s_Powers((c), Powers(c) | POW_COMM_ALL)
#define s_See_Queue(c) s_Powers((c), Powers(c) | POW_SEE_QUEUE)
#define s_See_Hidden(c) s_Powers((c), Powers(c) | POW_SEE_HIDDEN)
#define s_Can_Watch(c) s_Powers((c), Powers(c) | POW_WATCH)
#define s_Can_Poll(c) s_Powers((c), Powers(c) | POW_POLL)
#define s_No_Destroy(c) s_Powers((c), Powers(c) | POW_NO_DESTROY)
#define s_Guest(c) s_Powers((c), Powers(c) | POW_GUEST)
#define s_Set_Maint_Flags(c) s_Powers((c), Powers(c) | POW_SET_MFLAGS)
#define s_Stat_Any(c) s_Powers((c), Powers(c) | POW_STAT_ANY)
#define s_Steal(c) s_Powers((c), Powers(c) | POW_STEAL)
#define s_Tel_Anywhere(c) s_Powers((c), Powers(c) | POW_TEL_ANYWHR)
#define s_Tel_Anything(c) s_Powers((c), Powers(c) | POW_TEL_UNRST)
#define s_Unkillable(c) s_Powers((c), Powers(c) | POW_UNKILLABLE)
#define s_Builder(c) s_Powers2((c), Powers2(c) | POW_BUILDER)

#define Can_Set_Quota(c) (((Powers(c) & POW_CHG_QUOTAS) != 0) || Wizard(c))
#define Chown_Any(c) (((Powers(c) & POW_CHOWN_ANY) != 0) || Wizard(c))
#define Announce(c) (((Powers(c) & POW_ANNOUNCE) != 0) || Wizard(c))
#define Can_Boot(c) (((Powers(c) & POW_BOOT) != 0) || Wizard(c))
#define Can_Halt(c) (((Powers(c) & POW_HALT) != 0) || Wizard(c))
#define Control_All(c) (((Powers(c) & POW_CONTROL_ALL) != 0) || Wizard(c))
#define Wizard_Who(c) (((Powers(c) & POW_WIZARD_WHO) != 0) || WizRoy(c))
#define See_All(c) (((Powers(c) & POW_EXAM_ALL) != 0) || WizRoy(c))
#define Find_Unfindable(c) ((Powers(c) & POW_FIND_UNFIND) != 0)
#define Free_Money(c) (((Powers(c) & POW_FREE_MONEY) != 0) || Immortal(c))
#define Free_Quota(c) (((Powers(c) & POW_FREE_QUOTA) != 0) || Wizard(c))
#define Can_Hide(c) (((Powers(c) & POW_HIDE) != 0) || Wizard(c))
#define Can_Idle(c) (((Powers(c) & POW_IDLE) != 0) || Wizard(c))
#define Search(c) (((Powers(c) & POW_SEARCH) != 0) || WizRoy(c))
#define Long_Fingers(c) (((Powers(c) & POW_LONGFINGERS) != 0) || Wizard(c))
#define Comm_All(c) (((Powers(c) & POW_COMM_ALL) != 0) || Wizard(c))
#define See_Queue(c) (((Powers(c) & POW_SEE_QUEUE) != 0) || WizRoy(c))
#define See_Hidden(c) (((Powers(c) & POW_SEE_HIDDEN) != 0) || WizRoy(c))
#define Can_Watch(c) (((Powers(c) & POW_WATCH) != 0) || Wizard(c))
#define Can_Poll(c) (((Powers(c) & POW_POLL) != 0) || Wizard(c))
#define No_Destroy(c) (((Powers(c) & POW_NO_DESTROY) != 0) || Wizard(c))
#define Guest(c) ((Powers(c) & POW_GUEST) != 0)
#define Set_Maint_Flags(c) ((Powers(c) & POW_SET_MFLAGS) != 0)
#define Stat_Any(c) ((Powers(c) & POW_STAT_ANY) != 0)
#define Steal(c) (((Powers(c) & POW_STEAL) != 0) || Wizard(c))
#define Tel_Anywhere(c) (((Powers(c) & POW_TEL_ANYWHR) != 0) || Tel_Anything(c))
#define Tel_Anything(c) (((Powers(c) & POW_TEL_UNRST) != 0) || WizRoy(c))
#define Unkillable(c) (((Powers(c) & POW_UNKILLABLE) != 0) || Immortal(c))
#define Prog(c) (((Powers(c) & POW_PROG) != 0) || Wizard(c))
#define Sees_Hidden_Attrs(c) (((Powers(c) & POW_MDARK_ATTR) != 0) || WizRoy(c))
#define Sets_Wiz_Attrs(c) (((Powers(c) & POW_WIZ_ATTR) != 0) || Wizard(c))
#define Pass_Locks(c) ((Powers(c) & POW_PASS_LOCKS) != 0)
#define Builder(c) (((Powers2(c) & POW_BUILDER) != 0) || WizRoy(c))
#define LinkVariable(c) (((Powers2(c) & POW_LINKVAR) != 0) || Wizard(c))
#define LinkToAny(c) (((Powers2(c) & POW_LINKTOANY) != 0) || Wizard(c))
#define LinkAnyHome(c) (((Powers2(c) & POW_LINKHOME) != 0) || Wizard(c))
#define Open_Anywhere(c) ((Powers2(c) & POW_OPENANYLOC) != 0)
#define Can_Cloak(c) ((Powers2(c) & POW_CLOAK) != 0)
#define Can_Use_Module(c) ((Powers2(c) & POW_USE_MODULE) != 0)

#define OK_To_Send(p, t) (!herekey || ((!Unreal(p) || ((key & MSG_SPEECH) && Check_Heard((t), (p))) || ((key & MSG_MOVE) && Check_Noticed((t), (p))) || ((key & MSG_PRESENCE) && Check_Known((t), (p)))) && (!Unreal(t) || ((key & MSG_SPEECH) && Check_Hears((p), (t))) || ((key & MSG_MOVE) && Check_Notices((p), (t))) || ((key & MSG_PRESENCE) && Check_Knows((p), (t))))))

/**
 * @brief Cron macros
 *
 */
#define MINUTE_COUNT (LAST_MINUTE - FIRST_MINUTE + 1)
#define HOUR_COUNT (LAST_HOUR - FIRST_HOUR + 1)
#define DOM_COUNT (LAST_DOM - FIRST_DOM + 1)
#define MONTH_COUNT (LAST_MONTH - FIRST_MONTH + 1)
#define DOW_COUNT (LAST_DOW - FIRST_DOW + 1)