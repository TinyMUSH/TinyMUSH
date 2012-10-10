/* wiz.c - Wizard-only commands */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */
#include "typedefs.h"           /* required by mudconf */
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by interface */
#include "interface.h"		/* required by code */

#include "file_c.h"		/* required by code */
#include "match.h"		/* required by code */
#include "command.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */


void do_teleport(dbref player, dbref cause, int key, char *arg1, char *arg2) {
    dbref victim, destination, loc, exitloc;

    char *to;

    int hush = 0;

    if (((Fixed(player)) || (Fixed(Owner(player)))) &&
            !(Tel_Anywhere(player))) {
        notify(player, mudconf.fixed_tel_msg);
        return;
    }

    /*
     * get victim
     */

    if (*arg2 == '\0') {
        victim = player;
        to = arg1;
    } else {
        init_match(player, arg1, NOTYPE);
        match_everything(0);
        victim = noisy_match_result();

        if (victim == NOTHING)
            return;
        to = arg2;
    }

    /*
     * Validate type of victim
     */

    if (!Has_location(victim) && !isExit(victim)) {
        notify_quiet(player, "You can't teleport that.");
        return;
    }

    /*
     * If this is an exit, we need to control it, or it must be
     * * unlinked. (Same permissions as @link.)  Or, we can control
     * * the room. (Same permissions as get.)
     * * Otherwise, fail if we're not Tel_Anything, and we don't control
     * * the victim or the victim's location.
     */

    if (isExit(victim)) {
        if ((Location(victim) != NOTHING) && !Controls(player, victim)
                && !Controls(player, Exits(victim))) {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    } else if (!Controls(player, victim) &&
               !Controls(player, Location(victim)) && !Tel_Anything(player)) {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }
    /*
     * Check for teleporting home. Exits don't have homes.
     */

    if (!string_compare(to, "home")) {
        if (isExit(victim))
            notify_quiet(player, NOPERM_MESSAGE);
        else
            move_via_teleport(victim, HOME, cause, 0);
        return;
    }
    /*
     * Find out where to send the victim
     */

    init_match(player, to, NOTYPE);
    match_everything(0);
    destination = match_result();

    switch (destination) {
    case NOTHING:
        notify_quiet(player, "No match.");
        return;
    case AMBIGUOUS:
        notify_quiet(player,
                     "I don't know which destination you mean!");
        return;
    default:
        if ((victim == destination) || Going(destination)) {
            notify_quiet(player, "Bad destination.");
            return;
        }
    }

    /*
     * If fascist teleport is on, you must control the victim's ultimate
     * location (after LEAVEing any objects) or it must be JUMP_OK.
     */

    if (mudconf.fascist_tport) {
        loc = where_room(victim);
        if (!Good_obj(loc) || !isRoom(loc) ||
                !(Controls(player, loc) || Jump_ok(loc) ||
                  Tel_Anywhere(player))) {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
    }

    /*
     * If this is an exit, the same privs involved as @open apply.
     */

    if (isExit(victim)) {
        if (!Has_exits(destination) ||
                (!Controls(player, destination) && !Open_Anywhere(player))) {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }
        exitloc = Exits(victim);
        s_Exits(exitloc, remove_first(Exits(exitloc), victim));
        s_Exits(destination, insert_first(Exits(destination), victim));
        s_Exits(victim, destination);
        s_Modified(victim);
        notify_quiet(player, "Teleported.");
        return;
    }

    if (Has_contents(destination)) {

        /*
         * You must control the destination, or it must be a JUMP_OK
         * room where the victim passes its TELEPORT lock, or you
         * must be Tel_Anywhere.
         */

        if (!(Controls(player, destination) ||
                (Jump_ok(destination) &&
                 could_doit(victim, destination, A_LTPORT)) ||
                Tel_Anywhere(player))) {

            /*
             * Nope, report failure
             */

            if (player != victim)
                notify_quiet(player, NOPERM_MESSAGE);
            did_it(victim, destination,
                   A_TFAIL, "You can't teleport there!",
                   A_OTFAIL, NULL, A_ATFAIL, 0, (char **)NULL, 0,
                   MSG_MOVE);
            return;
        }
        /*
         * We're OK, do the teleport
         */

        if (key & TELEPORT_QUIET)
            hush = HUSH_ENTER | HUSH_LEAVE;

        if (move_via_teleport(victim, destination, cause, hush)) {
            if (player != victim) {
                if (!Quiet(player))
                    notify_quiet(player, "Teleported.");
            }
        }
    } else if (isExit(destination)) {
        if (Exits(destination) == Location(victim)) {
            move_exit(victim, destination, 0,
                      "You can't go that way.", 0);
        } else {
            notify_quiet(player, "I can't find that exit.");
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * do_force_prefixed: Interlude to do_force for the # command
 */

void do_force_prefixed(dbref player, dbref cause, int key, char *command, char *args[], int nargs) {
    char *cp;

    cp = parse_to(&command, ' ', 0);
    if (!command)
        return;
    while (*command && isspace(*command))
        command++;
    if (*command)
        do_force(player, cause, key, cp, command, args, nargs);
}

/*
 * ---------------------------------------------------------------------------
 * do_force: Force an object to do something.
 */

void do_force(dbref player, dbref cause, int key, char *what, char *command, char *args[], int nargs) {
    dbref victim;

    if ((victim = match_controlled(player, what)) == NOTHING)
        return;

    /*
     * force victim to do command
     */

    if (key & FRC_NOW)
        process_cmdline(victim, player, command, args, nargs, NULL);
    else
        wait_que(victim, player, 0, NOTHING, 0, command, args, nargs,
                 mudstate.rdata);
}

/*
 * ---------------------------------------------------------------------------
 * do_toad: Turn a player into an object.
 */

void do_toad(dbref player, dbref cause, int key, char *toad, char *newowner) {
    dbref victim, recipient, loc, aowner;

    char *buf;

    int count, aflags, alen;

    init_match(player, toad, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();
    if ((victim = noisy_match_result()) == NOTHING)
        return;

    if (!isPlayer(victim)) {
        notify_quiet(player, "Try @destroy instead.");
        return;
    }
    if (No_Destroy(victim)) {
        notify_quiet(player, "You can't toad that player.");
        return;
    }
    if ((newowner != NULL) && *newowner) {
        init_match(player, newowner, TYPE_PLAYER);
        match_neighbor();
        match_absolute();
        match_player();
        if ((recipient = noisy_match_result()) == NOTHING)
            return;
    } else {
        recipient = player;
    }

    STARTLOG(LOG_WIZARD, "WIZ", "TOAD")
    log_name_and_loc(victim);
    log_printf(" was @toaded by ");
    log_name(player);
    ENDLOG
    /*
     * Clear everything out
     */
    if (key & TOAD_NO_CHOWN) {
        count = -1;
    } else {
        count = chown_all(victim, recipient, player, 0);
        s_Owner(victim, recipient);	/*
						 * you get it
						 */
    }
    s_Flags(victim, TYPE_THING | HALT);
    s_Flags2(victim, 0);
    s_Flags3(victim, 0);
    s_Pennies(victim, 1);

    /*
     * notify people
     */

    loc = Location(victim);
    buf = alloc_mbuf("do_toad");
    sprintf(buf, "%s has been turned into a slimy toad!", Name(victim));
    notify_except2(loc, player, victim, player, buf, 0);
    sprintf(buf, "You toaded %s! (%d objects @chowned)", Name(victim),
            count + 1);
    notify_quiet(player, buf);

    /*
     * Zap the name from the name hash table
     */

    delete_player_name(victim, Name(victim));
    sprintf(buf, "a slimy toad named %s", Name(victim));
    s_Name(victim, buf);
    free_mbuf(buf);

    /*
     * Zap the alias too
     */

    buf = atr_pget(victim, A_ALIAS, &aowner, &aflags, &alen);
    delete_player_name(victim, buf);
    free_lbuf(buf);

    count = boot_off(victim,
                     (char *)"You have been turned into a slimy toad!");
    notify_quiet(player, tmprintf("%d connection%s closed.",
                                 count, (count == 1 ? "" : "s")));
}

void do_newpassword(dbref player, dbref cause, int key, char *name, char *password) {
    dbref victim;

    char *buf, *bp;

    if ((victim = lookup_player(player, name, 0)) == NOTHING) {
        notify_quiet(player, "No such player.");
        return;
    }
    if (*password != '\0' && !ok_password(password, player)) {
        /*
         * Can set null passwords, but not bad passwords.
         * * Notification of reason done by ok_password().
         */
        return;
    }
    if (God(victim)) {
        notify_quiet(player,
                     "You cannot change that player's password.");
        return;
    }
    STARTLOG(LOG_WIZARD, "WIZ", "PASS")
    log_name(player);
    log_printf(" changed the password of ");
    log_name(victim);
    ENDLOG
    /*
     * it's ok, do it
     */
    s_Pass(victim, crypt((const char *)password, "XX"));
    notify_quiet(player, "Password changed.");
    buf = alloc_lbuf("do_newpassword");
    bp = buf;
    safe_tmprintf_str(buf, &bp,
                     "Your password has been changed by %s.", Name(player));
    notify_quiet(victim, buf);
    free_lbuf(buf);
}

void do_boot(dbref player, dbref cause, int key, char *name) {
    dbref victim;

    char *buf, *bp;

    int count;

    if (!(Can_Boot(player))) {
        notify(player, NOPERM_MESSAGE);
        return;
    }
    if (key & BOOT_PORT) {
        if (is_number(name)) {
            victim = (int)strtol(name, (char **)NULL, 10);
        } else {
            notify_quiet(player, "That's not a number!");
            return;
        }
        STARTLOG(LOG_WIZARD, "WIZ", "BOOT")
        log_printf("Port %d was @booted by ", victim);
        log_name(player);
        ENDLOG
    } else {
        init_match(player, name, TYPE_PLAYER);
        match_neighbor();
        match_absolute();
        match_player();
        if ((victim = noisy_match_result()) == NOTHING)
            return;

        if (God(victim)) {
            notify_quiet(player, "You cannot boot that player!");
            return;
        }
        if ((!isPlayer(victim) && !God(player)) || (player == victim)) {
            notify_quiet(player,
                         "You can only boot off other players!");
            return;
        }
        STARTLOG(LOG_WIZARD, "WIZ", "BOOT")
        log_name_and_loc(victim);
        log_printf(" was @booted by ");
        log_name(player);
        ENDLOG
        notify_quiet(player, tmprintf("You booted %s off!",
                                     Name(victim)));
    }
    if (key & BOOT_QUIET) {
        buf = NULL;
    } else {
        bp = buf = alloc_lbuf("do_boot.msg");
        safe_name(player, buf, &bp);
        safe_str((char *)" gently shows you the door.", buf, &bp);
        *bp = '\0';
    }

    if (key & BOOT_PORT)
        count = boot_by_port(victim, !God(player), buf);
    else
        count = boot_off(victim, buf);
    notify_quiet(player, tmprintf("%d connection%s closed.",
                                 count, (count == 1 ? "" : "s")));
    if (buf)
        free_lbuf(buf);
}

/*
 * ---------------------------------------------------------------------------
  do_poor: Reduce the wealth of anyone over a specified amount.
 */

void do_poor(dbref player, dbref cause, int key, char *arg1) {
    dbref a;

    int amt, curamt;

    if (!is_number(arg1))
        return;
    amt = (int)strtol(arg1, (char **)NULL, 10);
    DO_WHOLE_DB(a) {
        if (isPlayer(a)) {
            curamt = Pennies(a);
            if (amt < curamt)
                s_Pennies(a, amt);
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * do_cut: Chop off a contents or exits chain after the named item.
 */

void do_cut(dbref player, dbref cause, int key, char *thing) {
    dbref object;

    object = match_controlled(player, thing);
    switch (object) {
    case NOTHING:
        notify_quiet(player, "No match.");
        break;
    case AMBIGUOUS:
        notify_quiet(player, "I don't know which one");
        break;
    default:
        s_Next(object, NOTHING);
        notify_quiet(player, "Cut.");
    }
}

/* --------------------------------------------------------------------------
 * do_motd: Wizard-settable message of the day (displayed on connect)
 */

void do_motd(dbref player, dbref cause, int key, char *message) {
    int is_brief;

    is_brief = 0;
    if (key & MOTD_BRIEF) {
        is_brief = 1;
        key = key & ~MOTD_BRIEF;
        if (key == MOTD_ALL)
            key = MOTD_LIST;
        else if (key != MOTD_LIST)
            key |= MOTD_BRIEF;
    }
    message[GBUF_SIZE - 1] = '\0';
    switch (key) {
    case MOTD_ALL:
        if (mudconf.motd_msg) {
            XFREE(mudconf.motd_msg, "do_motd.motd");
        }
        mudconf.motd_msg = XSTRDUP(message, "do_motd.motd");
        if (!Quiet(player))
            notify_quiet(player, "Set: MOTD.");
        break;
    case MOTD_WIZ:
        if (mudconf.wizmotd_msg) {
            XFREE(mudconf.wizmotd_msg, "do_motd.wizmotd");
        }
        mudconf.wizmotd_msg = XSTRDUP(message, "do_motd.wizmotd");
        if (!Quiet(player))
            notify_quiet(player, "Set: Wizard MOTD.");
        break;
    case MOTD_DOWN:
        if (mudconf.downmotd_msg) {
            XFREE(mudconf.downmotd_msg, "do_motd.downmotd");
        }
        mudconf.downmotd_msg = XSTRDUP(message, "do_motd.downmotd");
        if (!Quiet(player))
            notify_quiet(player, "Set: Down MOTD.");
        break;
    case MOTD_FULL:
        if (mudconf.fullmotd_msg) {
            XFREE(mudconf.fullmotd_msg, "do_motd.fullmotd");
        }
        mudconf.fullmotd_msg = XSTRDUP(message, "do_motd.fullmotd");
        if (!Quiet(player))
            notify_quiet(player, "Set: Full MOTD.");
        break;
    case MOTD_LIST:
        if (Wizard(player)) {
            if (!is_brief) {
                notify_quiet(player, "----- motd file -----");
                fcache_send(player, FC_MOTD);
                notify_quiet(player,
                             "----- wizmotd file -----");
                fcache_send(player, FC_WIZMOTD);
                notify_quiet(player,
                             "----- motd messages -----");
            }
            if (mudconf.motd_msg && *mudconf.motd_msg) {
                notify_quiet(player,
                             tmprintf("MOTD: %s", mudconf.motd_msg));
            } else {
                notify_quiet(player, "No MOTD.");
            }
            if (mudconf.wizmotd_msg && *mudconf.wizmotd_msg) {
                notify_quiet(player,
                             tmprintf("Wizard MOTD: %s",
                                     mudconf.wizmotd_msg));
            } else {
                notify_quiet(player, "No Wizard MOTD.");
            }
            if (mudconf.downmotd_msg && *mudconf.downmotd_msg) {
                notify_quiet(player,
                             tmprintf("Down MOTD: %s",
                                     mudconf.downmotd_msg));
            } else {
                notify_quiet(player, "No Down MOTD.");
            }
            if (mudconf.fullmotd_msg && *mudconf.fullmotd_msg) {
                notify_quiet(player,
                             tmprintf("Full MOTD: %s",
                                     mudconf.fullmotd_msg));
            } else {
                notify_quiet(player, "No Full MOTD.");
            }
        } else {
            if (Guest(player))
                fcache_send(player, FC_CONN_GUEST);
            else
                fcache_send(player, FC_MOTD);
            if (mudconf.motd_msg && *mudconf.motd_msg) {
                notify_quiet(player, mudconf.motd_msg);
            } else {
                notify_quiet(player, "No MOTD.");
            }
        }
        break;
    default:
        notify_quiet(player, "Illegal combination of switches.");
    }
}

/* ---------------------------------------------------------------------------
 * do_global: enable or disable global control flags
 */
/* *INDENT-OFF* */

NAMETAB enable_names[] = {
    {(char *)"building",		1,	CA_PUBLIC,	CF_BUILD},
    {(char *)"checkpointing",	2,	CA_PUBLIC,	CF_CHECKPOINT},
    {(char *)"cleaning",		2,	CA_PUBLIC,	CF_DBCHECK},
    {(char *)"dequeueing",		1,	CA_PUBLIC,	CF_DEQUEUE},
    {(char *)"god_monitoring",	1,	CA_PUBLIC,	CF_GODMONITOR},
    {(char *)"idlechecking",	2,	CA_PUBLIC,	CF_IDLECHECK},
    {(char *)"interpret",		2,	CA_PUBLIC,	CF_INTERP},
    {(char *)"logins",		3,	CA_PUBLIC,	CF_LOGIN},
    {(char *)"eventchecking",	2,	CA_PUBLIC,	CF_EVENTCHECK},
    { NULL,				0,	0,		0}
};

/* *INDENT-ON* */


void do_global(dbref player, dbref cause, int key, char *flag) {
    int flagvalue;

    /*
     * Set or clear the indicated flag
     */

    flagvalue = search_nametab(player, enable_names, flag);
    if (flagvalue < 0) {
        notify_quiet(player, "I don't know about that flag.");
    } else if (key == GLOB_ENABLE) {
        mudconf.control_flags |= flagvalue;
        STARTLOG(LOG_CONFIGMODS, "CFG", "GLOBAL")
        log_name(player);
        log_printf(" enabled: %s", flag);
        ENDLOG if (!Quiet(player))
            notify_quiet(player, "Enabled.");
    } else if (key == GLOB_DISABLE) {
        mudconf.control_flags &= ~flagvalue;
        STARTLOG(LOG_CONFIGMODS, "CFG", "GLOBAL")
        log_name(player);
        log_printf(" disabled: %s", flag);
        ENDLOG if (!Quiet(player))
            notify_quiet(player, "Disabled.");
    } else {
        notify_quiet(player, "Illegal combination of switches.");
    }
}
