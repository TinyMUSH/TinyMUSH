/* create.c - Commands that create new objects */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "match.h"		/* required by code */
#include "command.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "ansi.h"		/* required by code */

extern dbref FDECL(match_controlled_quiet, (dbref, const char *));

extern dbref FDECL(clone_home, (dbref, dbref));

extern int FDECL(can_set_home, (dbref, dbref, dbref));

extern CONF conftable[];

extern CF_HDCL(cf_dbref);

/*
 * ---------------------------------------------------------------------------
 * parse_linkable_room: Get a location to link to.
 */

static dbref
parse_linkable_room(player, room_name)
dbref player;

char *room_name;
{
    dbref room;

    init_match(player, room_name, NOTYPE);
    match_everything(MAT_NO_EXITS | MAT_NUMERIC | MAT_HOME);
    room = match_result();

    /*
     * HOME is always linkable
     */

    if (room == HOME)
        return HOME;

    /*
     * Make sure we can link to it
     */

    if (!Good_obj(room))
    {
        notify_quiet(player, "That's not a valid object.");
        return NOTHING;
    }
    else if (!Linkable(player, room))
    {
        notify_quiet(player, "You can't link to that.");
        return NOTHING;
    }
    else
    {
        return room;
    }
}

/*
 * ---------------------------------------------------------------------------
 * open_exit, do_open: Open a new exit and optionally link it somewhere.
 */

static void
open_exit(player, loc, direction, linkto)
dbref player, loc;

char *direction, *linkto;
{
    dbref exit;

    if (!Good_obj(loc))
        return;

    if (!direction || !*direction)
    {
        notify_quiet(player, "Open where?");
        return;
    }
    else if (!(controls(player, loc) ||
               (Open_Anywhere(player) && !God(loc))))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    exit = create_obj(player, TYPE_EXIT, direction, 0);
    if (exit == NOTHING)
        return;

    /*
     * Initialize everything and link it in.
     */

    s_Exits(exit, loc);
    s_Next(exit, Exits(loc));
    s_Exits(loc, exit);

    /*
     * and we're done
     */

    notify_quiet(player, "Opened.");

    /*
     * See if we should do a link
     */

    if (!linkto || !*linkto)
        return;

    loc = parse_linkable_room(player, linkto);
    if (loc != NOTHING)
    {

        /*
         * Make sure the player passes the link lock
         */

        if (loc != HOME && (!Good_obj(loc) ||
                            !Passes_Linklock(player, loc)))
        {
            notify_quiet(player, "You can't link to there.");
            return;
        }
        /*
         * Link it if the player can pay for it
         */

        if (!payfor(player, mudconf.linkcost))
        {
            notify_quiet(player,
                         tprintf("You don't have enough %s to link.",
                                 mudconf.many_coins));
        }
        else
        {
            s_Location(exit, loc);
            notify_quiet(player, "Linked.");
        }
    }
}

void
do_open(player, cause, key, direction, links, nlinks)
dbref player, cause;

int key, nlinks;

char *direction, *links[];
{
    dbref loc, destnum;

    char *dest;

    /*
     * Create the exit and link to the destination, if there is one
     */

    if (nlinks >= 1)
        dest = links[0];
    else
        dest = NULL;

    if (key == OPEN_INVENTORY)
        loc = player;
    else
        loc = Location(player);

    open_exit(player, loc, direction, dest);

    /*
     * Open the back link if we can
     */

    if (nlinks >= 2)
    {
        destnum = parse_linkable_room(player, dest);
        if (destnum != NOTHING)
        {
            open_exit(player, destnum, links[1],
                      tprintf("%d", loc));
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * link_exit, do_link: Set destination(exits), dropto(rooms) or
 * home(player,thing)
 */

void
link_exit(player, exit, dest)
dbref player, exit, dest;
{
    int cost, quot;

    /*
     * Make sure we can link there: Our destination is HOME Our
     * destination is AMBIGUOUS and we can link to variable exits Normal
     * destination check: - We must control the destination or it must be
     * LINK_OK or we must have LinkToAny and the destination's not God. -
     * We must be able to pass the linklock, or we must be able to
     * LinkToAny (power, or be a wizard) and be config'd so wizards
     * ignore linklocks
     */

    if (!((dest == HOME) ||
            ((dest == AMBIGUOUS) && LinkVariable(player)) ||
            (Linkable(player, dest) && Passes_Linklock(player, dest))))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    /*
     * Exit must be unlinked or controlled by you
     */

    if ((Location(exit) != NOTHING) && !controls(player, exit))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    /*
     * handle costs
     */

    cost = mudconf.linkcost;
    quot = 0;
    if (Owner(exit) != Owner(player))
    {
        cost += mudconf.opencost;
        quot += mudconf.exit_quota;
    }
    if (!canpayfees(player, player, cost, quot, TYPE_EXIT))
        return;
    else
        payfees(player, cost, quot, TYPE_EXIT);

    /*
     * Pay the owner for his loss
     */

    if (Owner(exit) != Owner(player))
    {
        payfees(Owner(exit), -mudconf.opencost, -quot, TYPE_EXIT);
        s_Owner(exit, Owner(player));
        s_Flags(exit, (Flags(exit) & ~(INHERIT | WIZARD)) | HALT);
    }
    /*
     * link has been validated and paid for, do it and tell the player
     */

    s_Location(exit, dest);
    if (!Quiet(player))
        notify_quiet(player, "Linked.");
    s_Modified(exit);
}

void
do_link(player, cause, key, what, where)
dbref player, cause;

int key;

char *what, *where;
{
    dbref thing, room;

    /*
     * Find the thing to link
     */

    init_match(player, what, TYPE_EXIT);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
        return;

    /*
     * Allow unlink if where is not specified
     */

    if (!where || !*where)
    {
        do_unlink(player, cause, key, what);
        return;
    }
    switch (Typeof(thing))
    {
    case TYPE_EXIT:

        /*
         * Set destination
         */
        if (!strcasecmp(where, "variable"))
        {
            room = AMBIGUOUS;
        }
        else
        {
            room = parse_linkable_room(player, where);
        }
        if (room != NOTHING)
            link_exit(player, thing, room);
        break;
    case TYPE_PLAYER:
    case TYPE_THING:

        /*
         * Set home
         */

        if (!Controls(player, thing))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            break;
        }
        init_match(player, where, NOTYPE);
        match_everything(MAT_NO_EXITS);
        room = noisy_match_result();
        if (!Good_obj(room))
            break;
        if (!Has_contents(room))
        {
            notify_quiet(player, "Can't link to an exit.");
            break;
        }
        if (!can_set_home(player, thing, room) ||
                !Passes_Linklock(player, room))
        {
            notify_quiet(player, NOPERM_MESSAGE);
        }
        else if (room == HOME)
        {
            notify_quiet(player, "Can't set home to home.");
        }
        else
        {
            s_Home(thing, room);
            if (!Quiet(player))
                notify_quiet(player, "Home set.");
            s_Modified(thing);
        }
        break;
    case TYPE_ROOM:

        /*
         * Set dropto
         */

        if (!Controls(player, thing))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            break;
        }
        room = parse_linkable_room(player, where);

        if (room == HOME)
        {
            s_Dropto(thing, room);
            if (!Quiet(player))
                notify_quiet(player, "Dropto set.");
            s_Modified(thing);
        }
        else if (!(Good_obj(room)))
        {
            break;
        }
        else if (!isRoom(room))
        {
            notify_quiet(player, "That is not a room!");
        }
        else if (!(Linkable(player, room) &&
                   Passes_Linklock(player, room)))
        {
            notify_quiet(player, NOPERM_MESSAGE);
        }
        else
        {
            s_Dropto(thing, room);
            if (!Quiet(player))
                notify_quiet(player, "Dropto set.");
            s_Modified(thing);
        }
        break;
    case TYPE_GARBAGE:
        notify_quiet(player, NOPERM_MESSAGE);
        break;
    default:
        STARTLOG(LOG_BUGS, "BUG", "OTYPE")
        log_printf("Strange object type: object #%d = %d",
                   thing, Typeof(thing));
        ENDLOG
    }
}

/*
 * ---------------------------------------------------------------------------
 * do_parent: Set an object's parent field.
 */

void
do_parent(player, cause, key, tname, pname)
dbref player, cause;

int key;

char *tname, *pname;
{
    dbref thing, parent, curr;

    int lev;

    /*
     * get victim
     */

    init_match(player, tname, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
        return;

    /*
     * Make sure we can do it
     */

    if (!Controls(player, thing))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    /*
     * Find out what the new parent is
     */

    if (*pname)
    {
        init_match(player, pname, Typeof(thing));
        match_everything(0);
        parent = noisy_match_result();
        if (parent == NOTHING)
            return;

        /*
         * Make sure we have rights to set parent
         */

        if (!Parentable(player, parent))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
        /*
         * Verify no recursive reference
         */

        ITER_PARENTS(parent, curr, lev)
        {
            if (curr == thing)
            {
                notify_quiet(player,
                             "You can't have yourself as a parent!");
                return;
            }
        }
    }
    else
    {
        parent = NOTHING;
    }

    s_Parent(thing, parent);
    s_Modified(thing);
    if (!Quiet(thing) && !Quiet(player))
    {
        if (parent == NOTHING)
            notify_quiet(player, "Parent cleared.");
        else
            notify_quiet(player, "Parent set.");
    }
}

/*
 * ---------------------------------------------------------------------------
 * do_dig: Create a new room.
 */

void
do_dig(player, cause, key, name, args, nargs)
dbref player, cause;

int key, nargs;

char *name, *args[];
{
    dbref room;

    char *buff;

    /*
     * we don't need to know player's location!  hooray!
     */

    if (!name || !*name)
    {
        notify_quiet(player, "Dig what?");
        return;
    }
    room = create_obj(player, TYPE_ROOM, name, 0);
    if (room == NOTHING)
        return;

    notify(player, tprintf("%s created with room number %d.", name, room));

    buff = alloc_sbuf("do_dig");
    if ((nargs >= 1) && args[0] && *args[0])
    {
        sprintf(buff, "%d", room);
        open_exit(player, Location(player), args[0], buff);
    }
    if ((nargs >= 2) && args[1] && *args[1])
    {
        sprintf(buff, "%d", Location(player));
        open_exit(player, room, args[1], buff);
    }
    free_sbuf(buff);
    if (key == DIG_TELEPORT)
        (void)move_via_teleport(player, room, cause, 0);
}

/*
 * ---------------------------------------------------------------------------
 * do_create: Make a new object.
 */

void
do_create(player, cause, key, name, coststr)
dbref player, cause;

int key;

char *name, *coststr;
{
    dbref thing;

    int cost;

    cost = atoi(coststr);
    if (!name || !*name || (strip_ansi_len(name) == 0))
    {
        notify_quiet(player, "Create what?");
        return;
    }
    else if (cost < 0)
    {
        notify_quiet(player,
                     "You can't create an object for less than nothing!");
        return;
    }
    thing = create_obj(player, TYPE_THING, name, cost);
    if (thing == NOTHING)
        return;

    move_via_generic(thing, player, NOTHING, 0);
    s_Home(thing, new_home(player));
    if (!Quiet(player))
    {
        notify(player,
               tprintf("%s created as object #%d", Name(thing), thing));
    }
}


/*
 * ---------------------------------------------------------------------------
 * do_clone: Create a copy of an object.
 */

void
do_clone(player, cause, key, name, arg2)
dbref player, cause;

int key;

char *name, *arg2;
{
    dbref clone, thing, new_owner, loc;

    FLAG rmv_flags;

    int cost;

    const char *clone_name;

    if ((key & CLONE_INVENTORY) || !Has_location(player))
        loc = player;
    else
        loc = Location(player);

    if (!Good_obj(loc))
        return;

    init_match(player, name, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();
    if ((thing == NOTHING) || (thing == AMBIGUOUS))
        return;

    /*
     * Let players clone things set VISUAL.  It's easier than retyping in
     * all that data
     */

    if (!Examinable(player, thing))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    if (isPlayer(thing))
    {
        notify_quiet(player, "You cannot clone players!");
        return;
    }
    /*
     * You can only make a parent link to what you control
     */

    if (!Controls(player, thing) && !Parent_ok(thing) &&
            (key & CLONE_FROM_PARENT))
    {
        notify_quiet(player,
                     tprintf("You don't control %s, ignoring /parent.",
                             Name(thing)));
        key &= ~CLONE_FROM_PARENT;
    }
    /*
     * You can only preserve the owner on the clone of an object owned by
     * another player, if you control that player.
     */

    new_owner = (key & CLONE_PRESERVE) ? Owner(thing) : Owner(player);
    if ((new_owner != Owner(player)) && !Controls(player, new_owner))
    {
        notify_quiet(player,
                     tprintf
                     ("You don't control the owner of %s, ignoring /preserve.",
                      Name(thing)));
        new_owner = Owner(player);
    }
    /*
     * Determine the cost of cloning. We have to do limits enforcement
     * here, because we're going to wipe out the attribute for money set
     * by create_obj() and need to set this ourselves. Note that you
     * can't change the cost of objects other than things.
     */

    if (key & CLONE_SET_COST)
    {
        cost = atoi(arg2);
        arg2 = NULL;
    }
    else
    {
        cost = 0;
    }

    switch (Typeof(thing))
    {
    case TYPE_THING:
        if (key & CLONE_SET_COST)
        {
            if (cost < mudconf.createmin)
                cost = mudconf.createmin;
            if (cost > mudconf.createmax)
                cost = mudconf.createmax;
        }
        else
        {
            cost = OBJECT_DEPOSIT((mudconf.clone_copy_cost) ?
                                  Pennies(thing) : 1);
        }
        break;
    case TYPE_ROOM:
        cost = mudconf.digcost;
        break;
    case TYPE_EXIT:
        if (!Controls(player, loc))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
        cost = mudconf.opencost;
        break;
    }

    /*
     * Go make the clone object
     */

    if ((arg2 && *arg2) && ok_name(arg2))
        clone = create_obj(new_owner, Typeof(thing), arg2, cost);
    else
        clone =
            create_obj(new_owner, Typeof(thing), Name(thing), cost);
    if (clone == NOTHING)
        return;

    /*
     * Wipe out any old attributes and copy in the new data
     */

    atr_free(clone);
    if (key & CLONE_FROM_PARENT)
    {
        s_Parent(clone, thing);
    }
    else
    {
        atr_cpy(player, clone, thing);
    }

    /*
     * Reset the name, since we cleared the attributes.
     */

    if ((arg2 && *arg2) && ok_name(arg2))
        clone_name = arg2;
    else
        clone_name = Name(thing);

    s_Name(clone, (char *)clone_name);

    /*
     * Reset the cost, since this also got wiped when we cleared
     * attributes. Note that only things have a value, though you pay a
     * cost for creating everything.
     */

    if (isThing(clone))
        s_Pennies(clone, OBJECT_ENDOWMENT(cost));

    /*
     * Clear out problem flags from the original. Don't strip the INHERIT
     * bit if we got the Inherit switch. Don't strip other flags if we
     * got the NoStrip switch EXCEPT for the Wizard flag, unless we're
     * God. (Powers are not cloned, ever.)
     */

    if (key & CLONE_NOSTRIP)
    {
        if (God(player))
        {
            s_Flags(clone, Flags(thing));
        }
        else
        {
            s_Flags(clone, Flags(thing) & ~WIZARD);
        }
        s_Flags2(clone, Flags2(thing));
        s_Flags3(clone, Flags3(thing));
    }
    else
    {
        rmv_flags = mudconf.stripped_flags.word1;
        if ((key & CLONE_INHERIT) && Inherits(player))
            rmv_flags &= ~INHERIT;
        s_Flags(clone, Flags(thing) & ~rmv_flags);
        s_Flags2(clone, Flags2(thing) & ~mudconf.stripped_flags.word2);
        s_Flags3(clone, Flags3(thing) & ~mudconf.stripped_flags.word3);
    }

    /*
     * Tell creator about it
     */

    if (!Quiet(player))
    {
        if (arg2 && *arg2)
            notify(player,
                   tprintf("%s cloned as %s, new copy is object #%d.",
                           Name(thing), clone_name, clone));
        else
            notify(player,
                   tprintf("%s cloned, new copy is object #%d.",
                           Name(thing), clone));
    }
    /*
     * Put the new thing in its new home.  Break any dropto or link, then
     * try to re-establish it.
     */

    switch (Typeof(thing))
    {
    case TYPE_THING:
        s_Home(clone, clone_home(player, thing));
        move_via_generic(clone, loc, player, 0);
        break;
    case TYPE_ROOM:
        s_Dropto(clone, NOTHING);
        if (Dropto(thing) != NOTHING)
            link_exit(player, clone, Dropto(thing));
        break;
    case TYPE_EXIT:
        s_Exits(loc, insert_first(Exits(loc), clone));
        s_Exits(clone, loc);
        s_Location(clone, NOTHING);
        if (Location(thing) != NOTHING)
            link_exit(player, clone, Location(thing));
        break;
    }

    /*
     * If same owner run Aclone, else halt it.  Also copy parent if we
     * can
     */

    if (new_owner == Owner(thing))
    {
        if (!(key & CLONE_FROM_PARENT))
            s_Parent(clone, Parent(thing));
        did_it(player, clone, A_NULL, NULL, A_NULL, NULL, A_ACLONE, 0,
               (char **)NULL, 0, MSG_MOVE);
    }
    else
    {
        if (!(key & CLONE_FROM_PARENT) &&
                (Controls(player, thing) || Parent_ok(thing)))
            s_Parent(clone, Parent(thing));
        s_Halted(clone);
    }
}

/*
 * ---------------------------------------------------------------------------
 * do_pcreate: Create new players and robots.
 */

void
do_pcreate(player, cause, key, name, pass)
dbref player, cause;

int key;

char *name, *pass;
{
    int isrobot;

    dbref newplayer;

    char *newname;

    isrobot = (key == PCRE_ROBOT) ? 1 : 0;
    newplayer = create_player(name, pass, player, isrobot, 0);
    newname = munge_space(name);
    if (newplayer == NOTHING)
    {
        notify_quiet(player, tprintf("Failure creating '%s'",
                                     newname));
        free_lbuf(newname);
        return;
    }
    if (isrobot)
    {
        move_object(newplayer, Location(player));
        notify_quiet(player,
                     tprintf("New robot '%s' (#%d) created with password '%s'",
                             newname, newplayer, pass));
        notify_quiet(player, "Your robot has arrived.");
        STARTLOG(LOG_PCREATES, "CRE", "ROBOT")
        log_name(newplayer);
        log_printf(" created by ");
        log_name(player);
        ENDLOG
    }
    else
    {
        move_object(newplayer, (Good_loc(mudconf.start_room) ?
                                mudconf.start_room : 0));
        notify_quiet(player,
                     tprintf("New player '%s' (#%d) created with password '%s'",
                             newname, newplayer, pass));
        STARTLOG(LOG_PCREATES | LOG_WIZARD, "WIZ", "PCREA")
        log_name(newplayer);
        log_printf(" created by ");
        log_name(player);
        ENDLOG
    }
    free_lbuf(newname);
}

/*
 * ---------------------------------------------------------------------------
 * can_destroy_exit, can_destroy_player, do_destroy: Destroy things.
 */

static int
can_destroy_exit(player, exit)
dbref player, exit;
{
    dbref loc;

    loc = Exits(exit);
    if (!((Has_location(player) && (loc == Location(player))) ||
            (player == loc) || (player == exit) || Wizard(player)))
    {
        notify_quiet(player,
                     "You cannot destroy exits in another room.");
        return 0;
    }
    return 1;
}

/*
 *
 * --------------------------------------------------------------------------- *
 * destroyable: Indicates if target of a @destroy is a 'special' object in *
 * the database.
 */

int
destroyable(victim)
dbref victim;
{
    CONF *tp, *ctab;

    MODULE *mp;

    if ((victim == (dbref) 0) || (God(victim)))
        return 0;

    for (tp = conftable; tp->pname; tp++)
    {
        if (tp->interpreter == cf_dbref &&
                victim == *((dbref *) (tp->loc)))
            return 0;
    }

    WALK_ALL_MODULES(mp)
    {
        if ((ctab = DLSYM_VAR(mp->handle, mp->modname, "conftable",
                              CONF *)) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (tp->interpreter == cf_dbref &&
                        victim == *((dbref *) (tp->loc)))
                    return 0;
            }
        }
    }

    return 1;
}


static int
can_destroy_player(player, victim)
dbref player, victim;
{
    if (!Wizard(player))
    {
        notify_quiet(player, "Sorry, no suicide allowed.");
        return 0;
    }
    if (Wizard(victim))
    {
        notify_quiet(player, "Even you can't do that!");
        return 0;
    }
    return 1;
}

void
do_destroy(player, cause, key, what)
dbref player, cause;

int key;

char *what;
{
    dbref thing;

    int can_doit;

    const char *typename;

    char *t, *tbuf;

    /*
     * You can destroy anything you control.
     */

    thing = match_controlled_quiet(player, what);

    /*
     * If you own a location, you can destroy its exits.
     */

    if ((thing == NOTHING) && controls(player, Location(player)))
    {
        init_match(player, what, TYPE_EXIT);
        match_exit();
        thing = last_match_result();
    }
    /*
     * You can destroy DESTROY_OK things in your inventory.
     */

    if (thing == NOTHING)
    {
        init_match(player, what, TYPE_THING);
        match_possession();
        thing = last_match_result();
        if ((thing != NOTHING) && !(isThing(thing)
                                    && Destroy_ok(thing)))
        {
            thing = NOPERM;
        }
    }
    /*
     * Return an error if we didn't find anything to destroy.
     */

    if (match_status(player, thing) == NOTHING)
    {
        return;
    }
    /*
     * Check SAFE and DESTROY_OK flags
     */

    if (Safe(thing, player) && !(key & DEST_OVERRIDE) &&
            !(isThing(thing) && Destroy_ok(thing)))
    {
        notify_quiet(player,
                     "Sorry, that object is protected.  Use @destroy/override to destroy it.");
        return;
    }
    /*
     * Make sure we're not trying to destroy a special object
     */

    if (!destroyable(thing))
    {
        notify_quiet(player, "You can't destroy that!");
        return;
    }
    /*
     * Make sure we can do it, on a type-specific basis
     */

    switch (Typeof(thing))
    {
    case TYPE_EXIT:
        typename = "exit";
        can_doit = can_destroy_exit(player, thing);
        break;
    case TYPE_PLAYER:
        typename = "player";
        can_doit = can_destroy_player(player, thing);
        break;
    case TYPE_ROOM:
        typename = "room";
        can_doit = 1;
        break;
    case TYPE_THING:
        typename = "thing";
        can_doit = 1;
        break;
    case TYPE_GARBAGE:
        typename = "garbage";
        can_doit = 1;
        break;
    default:
        typename = "weird object";
        can_doit = 1;
        break;
    }
    if (!can_doit)
        return;

    /*
     * We can use @destroy/instant to immediately blow up an object that
     * was already queued for destruction -- that object is unmodified
     * except for being Going.
     */

    if (Going(thing) &&
            !((key & DEST_INSTANT) && (Typeof(thing) != TYPE_GARBAGE)))
    {
        notify_quiet(player,
                     tprintf("That %s has already been destroyed.", typename));
        return;
    }
    /*
     * If we specified the instant switch, or we're configured to
     * immediately make Destroy_Ok things (or things owned by Destroy_Ok
     * owners) go away, we do instant destruction.
     */

    if ((key & DEST_INSTANT) ||
            (mudconf.instant_recycle &&
             (Destroy_ok(thing) || Destroy_ok(Owner(thing)))))
    {

        switch (Typeof(thing))
        {
        case TYPE_EXIT:
            destroy_exit(thing);
            break;
        case TYPE_PLAYER:
            atr_add_raw(thing, A_DESTROYER, tprintf("%d", player));
            destroy_player(thing);
            break;
        case TYPE_ROOM:
            empty_obj(thing);
            destroy_obj(NOTHING, thing);
            break;
        case TYPE_THING:
            destroy_thing(thing);
            break;
        default:
            notify(player,
                   "Weird object type cannot be destroyed.");
            break;
        }
        return;
    }
    /*
     * Otherwise we queue things up for destruction.
     */

    if (!isRoom(thing))
    {
        notify(player,
               tprintf("The %s shakes and begins to crumble.", typename));
    }
    else
    {
        notify_all(thing, player,
                   "The room shakes and begins to crumble.");
    }

    if (!Quiet(thing) && !Quiet(Owner(thing)))
    {
        notify_quiet(Owner(thing),
                     tprintf("You will be rewarded shortly for %s(#%d).",
                             Name(thing), thing));
    }
    if ((Owner(thing) != player) && !Quiet(player))
    {
        t = tbuf = alloc_sbuf("do_destroy.owner");
        safe_sb_str(Name(Owner(thing)), tbuf, &t);
        notify_quiet(player, tprintf("Destroyed. %s's %s(#%d)",
                                     tbuf, Name(thing), thing));
        free_sbuf(tbuf);
    }
    if (isPlayer(thing))
        atr_add_raw(thing, A_DESTROYER, tprintf("%d", player));

    s_Going(thing);
}
