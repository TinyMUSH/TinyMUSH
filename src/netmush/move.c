/**
 * @file move.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Movement commands, teleportation, and location transition handling
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

/* ---------------------------------------------------------------------------
 * process_leave_loc: Generate messages and actions resulting from leaving
 * a place.
 */

void process_leave_loc(dbref thing, dbref dest, dbref cause, int canhear, int hush)
{
    dbref loc;
    int quiet, pattr, oattr, aattr;
    loc = Location(thing);

    if ((loc == NOTHING) || (loc == dest))
    {
        return;
    }

    if (dest == HOME)
    {
        dest = Home(thing);
        if (!Good_obj(dest))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "process_leave_loc: Invalid HOME destination for object #%d", thing);
            return;
        }
    }
    else if (!Good_obj(dest))
    {
        log_write(LOG_PROBLEMS, "BUG", "MOVE", "process_leave_loc: Invalid destination #%d from object #%d", dest, thing);
        return;
    }

    if (mushconf.have_pueblo == 1)
    {
        if (Html(thing))
        {
            notify_html(thing, "<xch_page clear=links>");
        }
    }

    /*
     * Hook support first
     */
    call_move_hook(thing, cause, false);
    /*
     * Run the LEAVE attributes in the current room if we meet any of
     * * following criteria:
     * * - The current room has wizard privs.
     * * - Neither the current room nor the moving object are dark.
     * * - The moving object can hear and does not hav wizard
     * *   privs. EXCEPT if we were called with the HUSH_LEAVE key.
     */
    quiet = (!(Wizard(loc) || (!Dark(thing) && !Dark(loc)) || (canhear && !DarkMover(thing)))) || (hush & HUSH_LEAVE);
    oattr = quiet ? A_NULL : A_OLEAVE;
    aattr = (!quiet || (mushconf.dark_actions && !(hush & HUSH_LEAVE))) ? A_ALEAVE : A_NULL;
    pattr = (!mushconf.terse_movemsg && Terse(thing)) ? A_NULL : A_LEAVE;
    did_it(thing, loc, pattr, NULL, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);

    /*
     * Do OXENTER for receiving room
     */

    if ((dest != NOTHING) && !quiet)
        did_it(thing, dest, A_NULL, NULL, A_OXENTER, NULL, A_NULL, 0, (char **)NULL, 0, MSG_MOVE);

    /*
     * Display the 'has left' message if we meet any of the following
     * * criteria:
     * * - Neither the current room nor the moving object are dark.
     * * - The object can hear and is not a dark wizard.
     */

    if (!quiet && !Blind(thing) && !Blind(loc))
    {
        if ((!Dark(thing) && !Dark(loc)) || (canhear && !DarkMover(thing)))
        {
            notify_except2(loc, thing, thing, cause, MSG_MOVE, "%s has left.", Name(thing));
        }
    }
}

/* ---------------------------------------------------------------------------
 * process_enter_loc: Generate messages and actions resulting from entering
 * a place.
 */

void process_enter_loc(dbref thing, dbref src, dbref cause, int canhear, int hush)
{
    dbref loc;
    int quiet, pattr, oattr, aattr;
    loc = Location(thing);

    if ((loc == NOTHING) || (loc == src))
    {
        return;
    }

    if (mushconf.have_pueblo == 1)
    {
        show_vrml_url(thing, loc);
    }

    /*
     * Hook support first
     */
    call_move_hook(thing, cause, true);
    /*
     * Run the ENTER attributes in the current room if we meet any of
     * * following criteria:
     * * - The current room has wizard privs.
     * * - Neither the current room nor the moving object are dark.
     * * - The moving object can hear and does not hav wizard
     * *   privs. EXCEPT if we were called with the HUSH_ENTER key.
     */
    quiet = (!(Wizard(loc) || (!Dark(thing) && !Dark(loc)) || (canhear && !DarkMover(thing)))) || (hush & HUSH_ENTER);
    oattr = quiet ? A_NULL : A_OENTER;
    aattr = (!quiet || (mushconf.dark_actions && !(hush & HUSH_ENTER))) ? A_AENTER : A_NULL;
    pattr = (!mushconf.terse_movemsg && Terse(thing)) ? A_NULL : A_ENTER;
    did_it(thing, loc, pattr, NULL, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);

    /*
     * Do OXLEAVE for sending room
     */

    if ((src != NOTHING) && !quiet && Good_obj(src))
        did_it(thing, src, A_NULL, NULL, A_OXLEAVE, NULL, A_NULL, 0, (char **)NULL, 0, MSG_MOVE);

    /*
     * Display the 'has arrived' message if we meet all of the following
     * * criteria:
     * * - The moving object can hear.
     * * - The object is not a dark wizard.
     */

    if (!quiet && canhear && !Blind(thing) && !Blind(loc) && !DarkMover(thing))
    {
        notify_except2(loc, thing, thing, cause, MSG_MOVE, "%s has arrived.", Name(thing));
    }
}

/* ---------------------------------------------------------------------------
 * move_object: Physically move an object from one place to another.
 * Does not generate any messages or actions.
 */

void move_object(dbref thing, dbref dest)
{
    dbref src;
    /*
     * Validate the object being moved
     */
    if (!Good_obj(thing))
    {
        return;
    }
    /*
     * Remove from the source location
     */
    src = Location(thing);

    if (src != NOTHING)
    {
        s_Contents(src, remove_first(Contents(src), thing));
    }

    /*
     * Special check for HOME
     */

    if (dest == HOME)
    {
        dest = Home(thing);
        if (!Good_obj(dest))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "move_object: Invalid HOME destination for object #%d", thing);
            return;
        }
    }

    /*
     * Add to destination location
     */

    if (dest != NOTHING)
    {
        s_Contents(dest, insert_first(Contents(dest), thing));
    }
    else
    {
        s_Next(thing, NOTHING);
    }

    s_Location(thing, dest);
    /*
     * Look around and do the penny check
     */
    look_in(thing, dest, (LK_SHOWEXIT | LK_OBEYTERSE));

    if (isPlayer(thing) && (mushconf.payfind > 0) && (Pennies(thing) < mushconf.paylimit) && (!Controls(thing, dest)) && (random_range(0, (mushconf.payfind) - 1) == 0))
    {
        giveto(thing, 1);
        notify_check(thing, thing, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You found a %s!", mushconf.one_coin);
    }
}

/* ---------------------------------------------------------------------------
 * send_dropto, process_sticky_dropto, process_dropped_dropto,
 * process_sacrifice_dropto: Check for and process droptos.
 */

/* send_dropto: Send an object through the dropto of a room */

void send_dropto(dbref thing, dbref player)
{
    dbref dest, loc;

    if (!Sticky(thing))
    {
        loc = Location(thing);
        dest = Dropto(loc);
        if (!Good_obj(dest))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "send_dropto: Invalid dropto destination #%d from object #%d at location #%d", dest, thing, loc);
            dest = HOME;
        }
        move_via_generic(thing, dest, player, 0);
    }
    else
    {
        move_via_generic(thing, HOME, player, 0);
    }

    divest_object(thing);
}

/* process_sticky_dropto: Call when an object leaves the room to see if
 * we should empty the room
 */

void process_sticky_dropto(dbref loc, dbref player)
{
    dbref dropto, thing, next;

    /*
     * Do nothing if checking anything but a sticky room
     */

    if (!Good_obj(loc) || !Has_dropto(loc) || !Sticky(loc))
    {
        return;
    }

    /*
     * Make sure dropto loc is valid
     */
    dropto = Dropto(loc);

    if ((dropto == NOTHING) || (dropto == loc))
    {
        return;
    }

    /*
     * Make sure no players hanging out
     */
    for (thing = Contents(loc); (thing != NOTHING) && (Next(thing) != thing); thing = Next(thing))
    {
        if (Dropper(thing))
        {
            return;
        }
    }

    /*
     * Check for corrupted object chain (cycle detected)
     */
    if ((thing != NOTHING) && (Next(thing) == thing))
    {
        log_write(LOG_PROBLEMS, "BUG", "MOVE", "Corrupted object chain detected in process_sticky_dropto: object #%d has Next(thing)==thing", thing);
        return;
    }

    /*
     * Send everything through the dropto
     */
    s_Contents(loc, reverse_list(Contents(loc)));

    for (thing = Contents(loc), next = (thing == NOTHING ? NOTHING : Next(thing)); (thing != NOTHING) && (Next(thing) != thing); thing = next, next = Next(next))
    {
        send_dropto(thing, player);
    }

    /*
     * Check for corrupted object chain in final loop
     */
    if ((thing != NOTHING) && (Next(thing) == thing))
    {
        log_write(LOG_PROBLEMS, "BUG", "MOVE", "Corrupted object chain detected in process_sticky_dropto final loop: object #%d has Next(thing)==thing", thing);
    }
}

/* process_dropped_dropto: Check what to do when someone drops an object. */

void process_dropped_dropto(dbref thing, dbref player)
{
    dbref loc, dropto;

    /*
     * If STICKY, send home
     */

    if (Sticky(thing))
    {
        move_via_generic(thing, HOME, player, 0);
        divest_object(thing);
        return;
    }

    /*
     * Process the dropto if location is a room and is not STICKY
     */
    loc = Location(thing);

    /*
     * Validate location before accessing dropto macros
     */
    if (Good_obj(loc) && Has_dropto(loc))
    {
        dropto = Dropto(loc);
        if ((dropto != NOTHING) && !Sticky(loc))
        {
            send_dropto(thing, player);
        }
    }
}

/* ---------------------------------------------------------------------------
 * move_via_generic: Generic move routine, generates standard messages and
 * actions.
 */

void move_via_generic(dbref thing, dbref dest, dbref cause, int hush)
{
    dbref src;
    int canhear;

    if (dest == HOME)
    {
        dest = Home(thing);
        if (!Good_obj(dest))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "Invalid HOME destination for object #%d", thing);
            return;
        }
    }

    src = Location(thing);
    canhear = Hearer(thing);
    process_leave_loc(thing, dest, cause, canhear, hush);
    move_object(thing, dest);
    did_it(thing, thing, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE, 0, (char **)NULL, 0, MSG_MOVE);
    process_enter_loc(thing, src, cause, canhear, hush);
}

/* ---------------------------------------------------------------------------
 * move_via_exit: Exit move routine, generic + exit messages + dropto check.
 */

void move_via_exit(dbref thing, dbref dest, dbref cause, dbref exit, int hush)
{
    dbref src;
    int canhear, darkwiz, quiet, pattr, oattr, aattr;

    if (dest == HOME)
    {
        dest = Home(thing);
        if (!Good_obj(dest))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "move_via_exit: Invalid HOME destination for object #%d", thing);
            return;
        }
    }

    src = Location(thing);
    canhear = Hearer(thing);
    /*
     * Dark wizzes and Cloak powered things don't trigger OSUCC/ASUCC
     */
    darkwiz = DarkMover(thing);
    quiet = darkwiz || (hush & HUSH_EXIT);
    oattr = quiet ? A_NULL : A_OSUCC;
    aattr = (!quiet || (mushconf.dark_actions && !(hush & HUSH_EXIT))) ? A_ASUCC : A_NULL;
    pattr = (!mushconf.terse_movemsg && Terse(thing)) ? A_NULL : A_SUCC;
    did_it(thing, exit, pattr, NULL, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
    process_leave_loc(thing, dest, cause, canhear, hush);
    move_object(thing, dest);
    /*
     * Dark wizards don't trigger ODROP/ADROP
     */
    oattr = quiet ? A_NULL : A_ODROP;
    aattr = (!quiet || (mushconf.dark_actions && !(hush & HUSH_EXIT))) ? A_ADROP : A_NULL;
    pattr = (!mushconf.terse_movemsg && Terse(thing)) ? A_NULL : A_DROP;
    did_it(thing, exit, pattr, NULL, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
    did_it(thing, thing, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE, 0, (char **)NULL, 0, MSG_MOVE);
    process_enter_loc(thing, src, cause, canhear, hush);

    /*
     * Process sticky dropto on source location (validate src is valid)
     */
    if (Good_obj(src))
    {
        process_sticky_dropto(src, thing);
    }
}

/* ---------------------------------------------------------------------------
 * move_via_teleport: Teleport move routine, generic + teleport messages +
 * divestiture + dropto check.
 */

int move_via_teleport(dbref thing, dbref dest, dbref cause, int hush)
{
    dbref src, curr;
    int canhear, count;
    char *failmsg;
    src = Location(thing);

    if ((dest != HOME) && Good_obj(src))
    {
        curr = src;

        for (count = mushconf.ntfy_nest_lim; count > 0; count--)
        {
            if (!could_doit(thing, curr, A_LTELOUT))
            {
                if ((thing == cause) || (cause == NOTHING))
                    failmsg = (char *)"You can't teleport out!";
                else
                {
                    failmsg = (char *)"You can't be teleported out!";
                    notify_quiet(cause, "You can't teleport that out!");
                }

                did_it(thing, src, A_TOFAIL, failmsg, A_OTOFAIL, NULL, A_ATOFAIL, 0, (char **)NULL, 0, MSG_MOVE);
                return 0;
            }

            if (isRoom(curr))
            {
                break;
            }

            curr = Location(curr);
        }
    }

    if (dest == HOME)
    {
        dest = Home(thing);
        if (!Good_obj(dest))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "move_via_teleport: Invalid HOME destination for object #%d", thing);
            return 0;
        }
    }

    /*
     * Validate destination is accessible
     */
    if (!Good_obj(dest))
    {
        log_write(LOG_PROBLEMS, "BUG", "MOVE", "move_via_teleport: Invalid destination #%d for object #%d", dest, thing);
        return 0;
    }

    canhear = Hearer(thing);

    if (!(hush & HUSH_LEAVE))
        did_it(thing, thing, A_NULL, NULL, A_OXTPORT, NULL, A_NULL, 0, (char **)NULL, 0, MSG_MOVE);

    process_leave_loc(thing, dest, NOTHING, canhear, hush);
    move_object(thing, dest);

    if (!(hush & HUSH_ENTER))
        did_it(thing, thing, A_TPORT, NULL, A_OTPORT, NULL, A_ATPORT, 0, (char **)NULL, 0, MSG_MOVE);

    did_it(thing, thing, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE, 0, (char **)NULL, 0, MSG_MOVE);
    process_enter_loc(thing, src, NOTHING, canhear, hush);
    divest_object(thing);

    /*
     * Process sticky dropto on source location (validate src is valid)
     */
    if (Good_obj(src))
    {
        process_sticky_dropto(src, thing);
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * find_var_dest: Find a variable exit destination (DESTINATION attr).
 */

dbref find_var_dest(dbref player, dbref exit)
{
    char *buf, *ebuf, *ep, *str;
    dbref aowner, dest_room;
    int aflags, alen;
    GDATA *preserve;
    buf = atr_pget(exit, A_EXITVARDEST, &aowner, &aflags, &alen);

    if (!*buf)
    {
        XFREE(buf);
        return NOTHING;
    }

    preserve = save_global_regs("find_var_dest_save");
    ebuf = ep = XMALLOC(LBUF_SIZE, "ep");
    str = buf;
    eval_expression_string(ebuf, &ep, exit, player, player, EV_FCHECK | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
    XFREE(buf);
    restore_global_regs("find_var_dest_save", preserve);

    if (*ebuf == '#')
    {
        dest_room = parse_dbref(ebuf + 1);
        if (!Good_obj(dest_room))
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "find_var_dest: Invalid destination #%d from exit #%d", dest_room, exit);
            dest_room = NOTHING;
        }
    }
    else
    {
        dest_room = NOTHING;
    }

    XFREE(ebuf);
    return dest_room;
}

/* ---------------------------------------------------------------------------
 * move_exit: Try to move a player through an exit.
 */

void move_exit(dbref player, dbref exit, int divest, const char *failmsg, int hush)
{
    dbref loc;
    int oattr, aattr;
    loc = Location(exit);

    switch (loc)
    {
    case HOME:
        loc = Home(player);
        if (!Good_obj(loc))
        {
            oattr = (Dark(player) || (hush & HUSH_EXIT)) ? A_NULL : A_OFAIL;
            aattr = ((hush & HUSH_EXIT) || (Dark(player) && !mushconf.dark_actions)) ? A_NULL : A_AFAIL;
            did_it(player, exit, A_FAIL, "That exit doesn't lead anywhere.", oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
            return;
        }
        break;

    case AMBIGUOUS:
        loc = find_var_dest(player, exit);
        break;

    case NOTHING:
        /*
         * No valid destination found
         */
        oattr = (Dark(player) || (hush & HUSH_EXIT)) ? A_NULL : A_OFAIL;
        aattr = ((hush & HUSH_EXIT) || (Dark(player) && !mushconf.dark_actions)) ? A_NULL : A_AFAIL;
        did_it(player, exit, A_FAIL, "That exit doesn't lead anywhere.", oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
        return;

    default:
        /*
         * EMPTY
         */
        break;
    }

    if (Good_obj(loc) && could_doit(player, exit, A_LOCK))
    {
        /*
         * Check if destination is going away before processing movement
         */
        if (Going(loc))
        {
            notify(player, "You can't go that way.");
            return;
        }

        switch (Typeof(loc))
        {
        case TYPE_ROOM:
            move_via_exit(player, loc, NOTHING, exit, hush);

            if (divest)
            {
                divest_object(player);
            }

            break;

        case TYPE_PLAYER:
        case TYPE_THING:
            move_via_exit(player, loc, NOTHING, exit, hush);
            divest_object(player);
            break;

        case TYPE_EXIT:
            notify(player, "You can't go that way.");
            return;
        }
    }
    else
    {
        oattr = (Dark(player) || (hush & HUSH_EXIT)) ? A_NULL : A_OFAIL;
        aattr = ((hush & HUSH_EXIT) || (Dark(player) && !mushconf.dark_actions)) ? A_NULL : A_AFAIL;
        did_it(player, exit, A_FAIL, failmsg, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
    }
}

/* ---------------------------------------------------------------------------
 * do_move: Move from one place to another via exits or 'home'.
 */

void do_move(dbref player, dbref cause, int key, char *direction)
{
    dbref exit, loc;
    int i, quiet;

    if (!string_compare(direction, "home"))
    { /* go home w/o stuff */
        if (((Fixed(player)) || (Fixed(Owner(player)))) && !(WizRoy(player)))
        {
            notify(player, mushconf.fixed_home_msg);
            return;
        }

        if ((loc = Location(player)) != NOTHING && !Dark(player) && !Dark(loc))
        {
            /*
             * tell all
             */
            notify_except(loc, player, player, MSG_MOVE, "%s goes home.", Name(player));
        }

        /*
         * give the player the messages
         */

        for (i = 0; i < 3; i++)
        {
            notify(player, "There's no place like home...");
        }

        move_via_generic(player, HOME, NOTHING, 0);
        divest_object(player);

        /*
         * Process sticky dropto on source location (check loc is valid)
         */
        if (Good_obj(loc))
        {
            process_sticky_dropto(loc, player);
        }
        return;
    }

    /*
     * find the exit - init once and reuse across multiple match attempts
     * Use init_match_check_keys once, then try different matchers in order
     */

    init_match_check_keys(player, direction, TYPE_EXIT);

    if (mushconf.move_match_more)
    {
        /*
         * Try increasingly broad match scopes
         */
        match_exit_with_parents();
        exit = last_match_result();

        if (exit == NOTHING)
        {
            match_master_exit();
            exit = last_match_result();
        }

        if (exit == NOTHING)
        {
            match_zone_exit();
            exit = last_match_result();
        }
    }
    else
    {
        match_exit();
        exit = match_result();
    }

    switch (exit)
    {
    case NOTHING: /* try to force the object */
        notify(player, "You can't go that way.");
        break;

    case AMBIGUOUS:
        notify(player, "I don't know which way you mean!");
        break;

    default:
        quiet = 0;

        if ((key & MOVE_QUIET) && Controls(player, exit))
        {
            quiet = HUSH_EXIT;
        }

        move_exit(player, exit, 0, "You can't go that way.", quiet);
    }
}

/* ---------------------------------------------------------------------------
 * do_get: Get an object.
 */

void do_get(dbref player, dbref cause, int key, char *what)
{
    dbref thing, playerloc, thingloc;
    char *failmsg;
    int oattr, aattr, quiet;
    playerloc = Location(player);

    if (!Good_obj(playerloc))
    {
        return;
    }

    /*
     * You can only pick up things in rooms and ENTER_OK objects/players
     */

    if (!isRoom(playerloc) && !Enter_ok(playerloc) && !controls(player, playerloc))
    {
        notify(player, NOPERM_MESSAGE);
        return;
    }

    /*
     * Look for the thing locally
     */
    init_match_check_keys(player, what, TYPE_THING);
    match_neighbor();
    match_exit();

    if (Long_Fingers(player))
    {
        match_absolute(); /* long fingers */
    }

    thing = match_result();

    /*
     * Look for the thing in other people's inventories
     */

    if (!Good_obj(thing))
        thing = match_status(player, match_possessed(player, player, what, thing, 1));

    if (!Good_obj(thing))
    {
        return;
    }

    /*
     * If we found it, get it
     */
    quiet = 0;

    switch (Typeof(thing))
    {
    case TYPE_PLAYER:
    case TYPE_THING:
        /*
         * You can't take what you already have
         */
        thingloc = Location(thing);

        if (thingloc == player)
        {
            notify(player, "You already have that!");
            break;
        }

        if ((key & GET_QUIET) && Controls(player, thing))
        {
            quiet = 1;
        }

        if (thing == player)
        {
            notify(player, "You cannot get yourself!");
        }
        else if (could_doit(player, thing, A_LOCK))
        {
            if (thingloc != playerloc)
            {
                notify_check(thingloc, thingloc, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s was taken from you.", Name(thing));
            }

            move_via_generic(thing, player, player, 0);
            notify(thing, "Taken.");
            oattr = quiet ? 0 : A_OSUCC;
            aattr = quiet ? 0 : A_ASUCC;
            did_it(player, thing, A_SUCC, "Taken.", oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
        }
        else
        {
            oattr = quiet ? 0 : A_OFAIL;
            aattr = quiet ? 0 : A_AFAIL;

            if (thingloc != playerloc)
                failmsg = (char *)"You can't take that from there.";
            else
            {
                failmsg = (char *)"You can't pick that up.";
            }

            did_it(player, thing, A_FAIL, failmsg, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
        }

        break;

    case TYPE_EXIT:
        /*
         * You can't take what you already have
         */
        thingloc = Exits(thing);

        if (thingloc == player)
        {
            notify(player, "You already have that!");
            break;
        }

        /*
         * You must control either the exit or the location
         */

        if (!Controls(player, thing) && !Controls(player, playerloc))
        {
            notify(player, NOPERM_MESSAGE);
            break;
        }

        /*
         * Do it
         */
        s_Exits(thingloc, remove_first(Exits(thingloc), thing));
        s_Exits(player, insert_first(Exits(player), thing));
        s_Exits(thing, player);

        if (!Quiet(player))
        {
            notify(player, "Exit taken.");
        }

        break;

    default:
        notify(player, "You can't take that!");
        break;
    }
}

/* ---------------------------------------------------------------------------
 * do_drop: Drop an object.
 */

void do_drop(dbref player, dbref cause, int key, char *name)
{
    dbref loc, exitloc, thing;
    char *buf, *bp;
    int quiet, oattr, aattr;
    loc = Location(player);

    if (!Good_obj(loc))
    {
        return;
    }

    init_match(player, name, TYPE_THING);
    match_possession();
    match_carried_exit();

    switch (thing = match_result())
    {
    case NOTHING:
        notify(player, "You don't have that!");
        return;

    case AMBIGUOUS:
        notify(player, "I don't know which you mean!");
        return;
    }

    switch (Typeof(thing))
    {
    case TYPE_THING:
    case TYPE_PLAYER:
    {
        dbref thingloc = Location(thing);

        /*
         * You have to be carrying it
         */
        if (((thingloc != player) && !Wizard(player)) || (!could_doit(player, thing, A_LDROP)))
        {
            did_it(player, thing, A_DFAIL, "You can't drop that.", A_ODFAIL, NULL, A_ADFAIL, 0, (char **)NULL, 0, MSG_MOVE);
            return;
        }

        /*
         * Move it
         */
        move_via_generic(thing, loc, player, 0);
        notify(thing, "Dropped.");
        quiet = 0;

        if ((key & DROP_QUIET) && Controls(player, thing))
        {
            quiet = 1;
        }

        bp = buf = XMALLOC(SBUF_SIZE, "buf");
        XSAFESPRINTF(buf, &bp, "dropped %s.", Name(thing));
        oattr = quiet ? 0 : A_ODROP;
        aattr = quiet ? 0 : A_ADROP;
        did_it(player, thing, A_DROP, "Dropped.", oattr, buf, aattr, 0, (char **)NULL, 0, MSG_MOVE);
        XFREE(buf);
        /*
         * Process droptos
         */
        process_dropped_dropto(thing, player);
    }
    break;

    case TYPE_EXIT:

        /*
         * You have to be carrying it
         */
        if ((Exits(thing) != player) && !Wizard(player))
        {
            notify(player, "You can't drop that.");
            return;
        }

        /**
        * Make sure we can open here: - We must control the destination or it must
        * be OPEN_OK or we must have Open_Anywhere and the location's not God. - We
        * must be able to pass the openlock, or we must be able to Open_Anywhere
        * (power, or be a wizard) and be config'd so wizards ignore openlocks
        *
        */
        if (!(Openable(player, loc) && Passes_Openlock(player, loc)))
        {
            notify(player, NOPERM_MESSAGE);
            return;
        }

        /*
         * Do it
         */
        exitloc = Exits(thing);
        s_Exits(exitloc, remove_first(Exits(exitloc), thing));
        s_Exits(loc, insert_first(Exits(loc), thing));
        s_Exits(thing, loc);

        if (!Quiet(player))
        {
            notify(player, "Exit dropped.");
        }

        break;

    default:
        notify(player, "You can't drop that.");
    }
}

/* ---------------------------------------------------------------------------
 * do_enter, do_leave: The enter and leave commands.
 */

void do_enter_internal(dbref player, dbref thing, int quiet)
{
    dbref loc;
    int oattr, aattr;

    if (!Enter_ok(thing) && !controls(player, thing))
    {
        oattr = quiet ? 0 : A_OEFAIL;
        aattr = quiet ? 0 : A_AEFAIL;
        did_it(player, thing, A_EFAIL, NOPERM_MESSAGE, oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
    }
    else if (player == thing)
    {
        notify(player, "You can't enter yourself!");
    }
    else if (could_doit(player, thing, A_LENTER))
    {
        loc = Location(player);
        if (Good_obj(loc))
        {
            oattr = quiet ? HUSH_ENTER : 0;
            move_via_generic(player, thing, NOTHING, oattr);
            divest_object(player);
            process_sticky_dropto(loc, player);
        }
    }
    else
    {
        oattr = quiet ? 0 : A_OEFAIL;
        aattr = quiet ? 0 : A_AEFAIL;
        did_it(player, thing, A_EFAIL, "You can't enter that.", oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
    }
}

void do_enter(dbref player, dbref cause, int key, char *what)
{
    dbref thing;
    int quiet;
    init_match(player, what, TYPE_THING);
    match_neighbor();

    if (Long_Fingers(player))
    {
        match_absolute(); /* the wizard has long fingers */
    }

    if ((thing = noisy_match_result()) == NOTHING)
    {
        return;
    }

    switch (Typeof(thing))
    {
    case TYPE_PLAYER:
    case TYPE_THING:
        quiet = 0;

        if ((key & MOVE_QUIET) && Controls(player, thing))
        {
            quiet = 1;
        }

        do_enter_internal(player, thing, quiet);
        break;

    default:
        notify(player, NOPERM_MESSAGE);
    }

    return;
}

void do_leave(dbref player, dbref cause, int key)
{
    dbref loc;
    int quiet, oattr, aattr;
    loc = Location(player);

    if (!Good_obj(loc) || isRoom(loc) || Going(loc))
    {
        notify(player, "You can't leave.");
        return;
    }

    quiet = 0;

    if ((key & MOVE_QUIET) && Controls(player, loc))
    {
        quiet = HUSH_LEAVE;
    }

    if (could_doit(player, loc, A_LLEAVE))
    {
        dbref dest = Location(loc);
        if (Good_obj(dest))
        {
            move_via_generic(player, dest, NOTHING, quiet);
        }
        else
        {
            log_write(LOG_PROBLEMS, "BUG", "MOVE", "do_leave: Invalid destination #%d for player #%d", dest, player);
            notify(player, "You can't leave.");
        }
    }
    else
    {
        oattr = quiet ? 0 : A_OLFAIL;
        aattr = quiet ? 0 : A_ALFAIL;
        did_it(player, loc, A_LFAIL, "You can't leave.", oattr, NULL, aattr, 0, (char **)NULL, 0, MSG_MOVE);
    }
}
