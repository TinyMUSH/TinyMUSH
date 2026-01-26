/**
 * @file create.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object creation commands and initial setup of rooms, exits, things, and players
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
#include <string.h>
#include <errno.h>
#include <limits.h>

/* Internal function prototypes */
static dbref _create_parse_linkable_room(dbref player, char *room_name);
static void _create_open_exit(dbref player, dbref loc, char *direction, char *linkto);
static void _create_link_exit(dbref player, dbref exit, dbref dest);
static bool _create_can_destroy_exit(dbref player, dbref exit);
static bool _create_can_destroy_player(dbref player, dbref victim);

/**
 * @brief Parse and validate a room name to ensure the player can link to it.
 *
 * Resolves a room name into a valid DBref that the player has permission to link to.
 * Supports numeric DBrefs, object names/aliases, and the special HOME keyword. Performs
 * comprehensive permission checking to ensure the target room is linkable by the player.
 *
 * Matching logic:
 * - Searches for exits, numeric DBrefs, and HOME keyword (MAT_NO_EXITS | MAT_NUMERIC | MAT_HOME)
 * - HOME is always linkable (special return value)
 * - Validates that targets are linkable objects (LINK_OK flag or player controls it)
 * - Checks Linkable() predicate to ensure permission
 *
 * @param player    DBref of player attempting to link (used for permission checks)
 * @param room_name Room specification string (object name, alias, numeric DBref, or \"home\")
 * @return HOME if target is the special HOME location, valid DBref if room is linkable,
 *         NOTHING if room_name is invalid, not linkable, or permission denied
 *
 * @note Thread-safe: No (modifies match state via init_match/match_result)
 * @note HOME is always linkable regardless of other conditions
 * @attention Notifies player on permission errors but returns NOTHING on invalid objects
 *
 * @see Linkable() for permission predicate
 * @see match_everything() for name resolution rules
 */
static dbref _create_parse_linkable_room(dbref player, char *room_name)
{
    dbref room = NOTHING;

    init_match(player, room_name, NOTYPE);
    match_everything(MAT_NO_EXITS | MAT_NUMERIC | MAT_HOME);
    room = match_result();

    /* HOME is always linkable */
    if (room == HOME)
    {
        return HOME;
    }

    /* Make sure we can link to it */
    if (!Good_obj(room))
    {
        notify_quiet(player, "That's not a valid object.");
        return NOTHING;
    }
    
    if (!Linkable(player, room))
    {
        notify_quiet(player, "You can't link to that.");
        return NOTHING;
    }
    
    return room;
}

/**
 * @brief Create a new exit in a location and optionally link it to a destination.
 *
 * Creates a new exit object with the specified name and direction, inserts it into
 * the location's exit list, and optionally links it to a destination room. Validates
 * permissions to ensure the player can open exits in the target location and can
 * afford the link cost if provided.
 *
 * Permission checks:
 * - Player must control the location or location must have OPEN_OK flag
 * - Player must pass the location's openlock
 * - If linking, player must pass destination's linklock and afford linkcost
 *
 * Exit creation process:
 * 1. Validates location is valid and direction is non-empty
 * 2. Checks Openable() and Passes_Openlock() permissions
 * 3. Creates new TYPE_EXIT object with specified direction name
 * 4. Inserts exit into location's exit list
 * 5. If linkto specified: resolves destination via parse_linkable_room(), validates linklock, charges cost
 *
 * @param player    DBref of player creating the exit (used for permission checks and cost)
 * @param loc       DBref of location where exit is being created
 * @param direction Direction/name of exit (e.g., "north", "out", "portal"); must be non-empty
 * @param linkto    Optional destination room specification (object name, alias, DBref, or NULL);
 *                  if NULL or empty, exit is created unlinked
 *
 * @note Thread-safe: No (modifies location's exit list, charges pennies, calls create_obj)
 * @note Notifies player quietly on success ("Opened.") and on error conditions
 * @note Creates exit even if linking fails; player can link later with @link command
 * @attention Validates permissions but does not check SAFE flag (not SAFE-protected)
 * @attention linkcost charged if linking succeeds, but not if exit creation fails
 *
 * @see _create_parse_linkable_room() for destination resolution rules
 * @see Openable() for location permission predicate
 * @see Passes_Openlock() for openlock validation
 * @see _create_link_exit() for explicit exit linking
 */
static void _create_open_exit(dbref player, dbref loc, char *direction, char *linkto)
{
    dbref exit = NOTHING;

    if (!Good_obj(loc))
    {
        return;
    }

    if (!direction || !*direction)
    {
        notify_quiet(player, "Open where?");
        return;
    }
    
    /* Verify player can open exits here (location permissions and openlock) */
    if (!(Openable(player, loc) && Passes_Openlock(player, loc)))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }

    exit = create_obj(player, TYPE_EXIT, direction, 0);

    if (exit == NOTHING)
    {
        return;
    }

    /* Initialize everything and link it in. */
    s_Exits(exit, loc);
    s_Next(exit, Exits(loc));
    s_Exits(loc, exit);

    /* and we're done */
    notify_quiet(player, "Opened.");

    /* See if we should do a link */
    if (!linkto || !*linkto)
    {
        return;
    }

    loc = _create_parse_linkable_room(player, linkto);

    if (loc != NOTHING)
    {
        /* Make sure the player passes the link lock */
        if (loc != HOME && (!Good_obj(loc) || !Passes_Linklock(player, loc)))
        {
            notify_quiet(player, "You can't link to there.");
            return;
        }
        /* Link it if the player can pay for it */
        if (!payfor(player, mushconf.linkcost))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "You don't have enough %s to link.", mushconf.many_coins);
        }
        else
        {
            s_Location(exit, loc);
            notify_quiet(player, "Linked.");
        }
    }
}

/**
 * @brief Command handler for @open: create new exits with optional back-linking.
 *
 * Implements the @open command to create one or two linked exits. This is the top-level
 * command dispatcher that determines whether to create an exit in the current room or
 * player's inventory, and optionally creates a corresponding back-link in the destination.
 *
 * Behavior:
 * - /INVENTORY flag: Create exit in player's inventory (loc = player)
 * - Normal: Create exit in player's current location (loc = Location(player))
 * - Creates primary exit with open_exit(player, loc, direction, dest)
 * - If nlinks >= 2: Creates back-link exit in destination with exit number as destination
 *
 * Link handling:
 * - nlinks == 0: Exit created unlinked (no destination)
 * - nlinks == 1: Exit created with single link to dest (forward exit only)
 * - nlinks >= 2: Forward exit to dest, back-link from dest back to loc using exit number
 * - Back-link direction taken from links[1], destination stringified as room number
 *
 * @param player     DBref of player issuing @open command (must be able to open in location)
 * @param cause      DBref of cause (usually equals player; used in logging/triggers)
 * @param key        Command switches: OPEN_INVENTORY creates exit in inventory instead of location
 * @param direction  Name/direction for primary exit (e.g., "north", "out", "portal")
 * @param links[]    Array of destination specifiers: links[0]=destination, links[1]=back-link direction
 * @param nlinks     Number of elements in links array (0, 1, or 2)
 *
 * @note Thread-safe: No (creates objects, modifies exit lists, charges coins)
 * @note Back-links only created if destination is linkable and nlinks >= 2
 * @note Notifies player of success/failure through notify_quiet() via open_exit()
 * @note Uses XASPRINTF to convert location number to string for back-link destination
 * @attention Both forward and back-link creation share cost; cost charged for each via _create_open_exit()
 * @attention If back-link creation fails, forward exit remains (not rolled back)
 *
 * @see _create_open_exit() for actual exit creation and linking logic
 * @see _create_parse_linkable_room() for destination resolution
 * @see OPEN_INVENTORY flag for inventory vs. room mode
 */
void do_open(dbref player, dbref cause, int key, char *direction, char *links[], int nlinks)
{
    dbref loc = NOTHING, destnum = NOTHING;
    char *dest = NULL, *s = NULL;

    /* Create the exit and link to the destination, if there is one */
    dest = (nlinks >= 1) ? links[0] : NULL;
    loc = (key == OPEN_INVENTORY) ? player : Location(player);

    _create_open_exit(player, loc, direction, dest);

    /* Open the back link if we can */
    if (nlinks >= 2)
    {
        destnum = _create_parse_linkable_room(player, dest);

        if (destnum != NOTHING)
        {
            s = XASPRINTF("s", "%d", loc);
            _create_open_exit(player, destnum, links[1], s);
            XFREE(s);
        }
    }
}

/**
 * @brief Link an exit to a destination, validating permissions and handling ownership/cost.
 *
 * Performs all necessary validation, permission checks, cost calculations, and state
 * management to link an exit object to a destination. Handles special cases like HOME,
 * variable exits (AMBIGUOUS), and ownership changes. Updates exit owner if destination
 * is owned by different player and charges appropriate fees.
 *
 * Destination validation:
 * - HOME always allowed (special return value)
 * - AMBIGUOUS allowed if player has LinkVariable power (variable exits)
 * - Normal destinations must pass Linkable() and Passes_Linklock() checks
 * - Returns with NOPERM_MESSAGE if destination not linkable
 *
 * Exit state validation:
 * - Exit must be unlinked (Location(exit) == NOTHING) OR player must control it
 * - Returns with NOPERM_MESSAGE if linked exit not controlled by player
 *
 * Cost calculation:
 * - Base cost: linkcost (charged to player)
 * - If Owner(exit) != Owner(player): add opencost and exit_quota (ownership change fee)
 * - Validates sufficient funds via canpayfees(); returns early if insufficient
 * - Deducts fees from player; pays owner if ownership changes
 * - Ownership change sets HALT flag and clears INHERIT/WIZARD flags
 *
 * @param player    DBref of player linking the exit (permission and cost validation)
 * @param exit      DBref of exit object to link (must be TYPE_EXIT)
 * @param dest      Destination DBref or special value (HOME, AMBIGUOUS, valid room)
 *
 * @note Thread-safe: No (modifies exit object, transfers currency, checks/updates ownership)
 * @note Ownership may change if exit owner differs from player
 * @note Halts exit if ownership changes (flag management prevents new owner issues)
 * @note Notifies player on success ("Linked.") unless player has Quiet flag
 * @note Does NOT notify on permission failures (uses NOPERM_MESSAGE only in code, not sent)
 * @attention HOME and AMBIGUOUS bypass linklock validation; normal destinations do not
 * @attention linkcost charged regardless of current owner; opencost only if owner changes
 * @attention Exit remains modified (s_Modified called) to update database timestamp
 *
 * @see Linkable() for destination permission predicate
 * @see Passes_Linklock() for linklock validation
 * @see LinkVariable() power check for AMBIGUOUS (variable) exit support
 * @see canpayfees() for fee validation including quotas
 * @see payfees() for currency deduction
 */
static void _create_link_exit(dbref player, dbref exit, dbref dest)
{
    int cost = 0, quot = 0;
    bool ownership_change = false;

    /* Make sure we can link there: Our destination is HOME Our
       destination is AMBIGUOUS and we can link to variable exits Normal
       destination check: - We must control the destination or it must be
       LINK_OK or we must have LinkToAny and the destination's not God. -
       We must be able to pass the linklock, or we must be able to
       LinkToAny (power, or be a wizard) and be config'd so wizards
       ignore linklocks */
    if (!((dest == HOME) || ((dest == AMBIGUOUS) && LinkVariable(player)) || (Linkable(player, dest) && Passes_Linklock(player, dest))))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }

    /* Exit must be unlinked or controlled by you */
    if ((Location(exit) != NOTHING) && !controls(player, exit))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }

    /* Calculate cost (linkcost + opencost/quota if ownership changes) */
    cost = mushconf.linkcost;
    ownership_change = (Owner(exit) != Owner(player));

    if (ownership_change)
    {
        cost += mushconf.opencost;
        quot = mushconf.exit_quota;
    }

    if (!canpayfees(player, player, cost, quot, TYPE_EXIT))
    {
        return;
    }
    
    payfees(player, cost, quot, TYPE_EXIT);

    /* Pay the owner for his loss */
    if (ownership_change)
    {
        payfees(Owner(exit), -mushconf.opencost, -quot, TYPE_EXIT);
        s_Owner(exit, Owner(player));
        s_Flags(exit, (Flags(exit) & ~(INHERIT | WIZARD)) | HALT);
    }

    /* Apply validated link */
    s_Location(exit, dest);

    if (!Quiet(player))
    {
        notify_quiet(player, "Linked.");
    }

    s_Modified(exit);
}

/**
 * @brief Command handler for @link: set exit destinations, player/thing homes, or room droptos.
 *
 * Unified command dispatcher that handles linking for three different object types with
 * distinct behaviors. Resolves target object, validates permissions, and applies appropriate
 * state changes based on object type. Supports unlinking when destination is unspecified.
 *
 * Type-specific behaviors:
 *
 * **TYPE_EXIT**: Links exit to destination via _create_link_exit()
 * - Special case: "variable" keyword creates variable/AMBIGUOUS exit (requires LinkVariable power)
 * - Normal destinations resolved via _create_parse_linkable_room()
 * - Calls _create_link_exit() which validates permissions, handles costs, potential ownership changes
 *
 * **TYPE_PLAYER / TYPE_THING**: Sets home location (s_Home)
 * - Requires player to control the object
 * - Destination must be linkable room (not exit) with Has_contents()
 * - Validates can_set_home() and Passes_Linklock() before applying
 * - Notifies player unless player has Quiet flag
 * - Sets object as modified
 *
 * **TYPE_ROOM**: Sets dropto location (s_Dropto)
 * - Requires player to control the room
 * - HOME always allowed (no linklock check)
 * - Normal destinations must be rooms (isRoom check) and pass Linkable/Passes_Linklock
 * - Notifies player unless player has Quiet flag
 * - Sets room as modified
 *
 * **Unlinking**: When where is empty/NULL, delegates to do_unlink() to remove links
 *
 * Permission and validation:
 * - Finds target via noisy_match_result() (returns NOTHING if not found)
 * - TYPE_GARBAGE objects rejected with NOPERM_MESSAGE
 * - Unknown types logged with BUG level
 *
 * @param player    DBref of player issuing @link command (must control target object)
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches (not used in current implementation)
 * @param what      Target object specification (name, alias, DBref to link/unlink)
 * @param where     Destination specification (room name/alias/DBref, "variable" for exits, or empty to unlink)
 *
 * @note Thread-safe: No (modifies objects, validates permissions, potential cost implications via link_exit)
 * @note Unlinking creates separate command dispatch to do_unlink()
 * @note Notifies player only for successful changes (HOME set, dropto set, etc.)
 * @note Notifies player of permission failures via NOPERM_MESSAGE
 * @attention For exits: "variable" is case-insensitive special keyword, not a room name
 * @attention For players/things: destination must have contents (can't link to exits)
 * @attention For rooms: HOME is always linkable but shows different behavior
 * @attention Objects of GARBAGE type always rejected regardless of control/permissions
 *
 * @see _create_link_exit() for exit linking logic, cost calculation, ownership handling
 * @see _create_parse_linkable_room() for room name resolution and permission checking
 * @see do_unlink() for unlinking implementation when destination unspecified
 * @see can_set_home() for home location permission predicate
 * @see isRoom() for room type validation
 */
void do_link(dbref player, dbref cause, int key, char *what, char *where)
{
    dbref thing = NOTHING, room = NOTHING;

    /* Find the thing to link */
    init_match(player, what, TYPE_EXIT);
    match_everything(0);
    thing = noisy_match_result();

    if (thing == NOTHING)
    {
        return;
    }

    /* Allow unlink if where is not specified */
    if (!where || !*where)
    {
        do_unlink(player, cause, key, what);
        return;
    }

    switch (Typeof(thing))
    {
    case TYPE_EXIT:
        /* Set destination */
        room = (!strcasecmp(where, "variable")) ? AMBIGUOUS : _create_parse_linkable_room(player, where);

        if (room != NOTHING)
        {
            _create_link_exit(player, thing, room);
        }

        break;

    case TYPE_PLAYER:
    case TYPE_THING:
        /* Set home */
        if (!Controls(player, thing))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            break;
        }

        init_match(player, where, NOTYPE);
        match_everything(MAT_NO_EXITS);
        room = noisy_match_result();

        if (!Good_obj(room))
        {
            break;
        }

        if (!Has_contents(room))
        {
            notify_quiet(player, "Can't link to an exit.");
            break;
        }

        if (!can_set_home(player, thing, room) || !Passes_Linklock(player, room))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            break;
        }
        
        if (room == HOME)
        {
            notify_quiet(player, "Can't set home to home.");
            break;
        }

        s_Home(thing, room);

        if (!Quiet(player))
        {
            notify_quiet(player, "Home set.");
        }

        s_Modified(thing);
        break;

    case TYPE_ROOM:
        /* Set dropto */
        if (!Controls(player, thing))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            break;
        }

        room = _create_parse_linkable_room(player, where);

        if (room == HOME)
        {
            s_Dropto(thing, room);

            if (!Quiet(player))
            {
                notify_quiet(player, "Dropto set.");
            }

            s_Modified(thing);
            break;
        }
        
        if (!Good_obj(room))
        {
            break;
        }
        
        if (!isRoom(room))
        {
            notify_quiet(player, "That is not a room!");
            break;
        }
        
        if (!(Linkable(player, room) && Passes_Linklock(player, room)))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            break;
        }

        s_Dropto(thing, room);

        if (!Quiet(player))
        {
            notify_quiet(player, "Dropto set.");
        }

        s_Modified(thing);
        break;

    case TYPE_GARBAGE:
        notify_quiet(player, NOPERM_MESSAGE);
        break;

    default:
        log_write(LOG_BUGS, "BUG", "OTYPE", "Strange object type: object #%d = %d", thing, Typeof(thing));
    }
}

/**
 * @brief Command handler for @parent: set or clear an object's parent for attribute inheritance.
 *
 * Implements the @parent command to establish parent-child relationships for attribute
 * inheritance. Validates that the player controls the target object, has permission to
 * set the specified parent, and prevents circular parent chain references. Supports
 * clearing parent by specifying empty pname.
 *
 * Permission and validation flow:
 * 1. Finds target object via noisy_match_result() (must exist)
 * 2. Verifies player Controls() target object
 * 3. If pname non-empty: resolves parent object, verifies Parentable() permission
 * 4. Prevents circular references: walks parent chain up to parent_nest_lim levels
 * 5. Sets parent via s_Parent(); marks object as modified
 *
 * Circular reference detection:
 * - Walks current parent chain (curr = Parent(curr)) up to parent_nest_lim iterations
 * - Returns error if target object found in ancestor chain
 * - Prevents logical impossibilities (object as its own ancestor)
 *
 * Parent clearing:
 * - pname empty/NULL sets parent to NOTHING (clears existing parent)
 * - Does not require Parentable() check (always allowed)
 * - Notifies player "Parent cleared." unless both object and player have Quiet flag
 *
 * @param player    DBref of player issuing @parent command (must control target)
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches (not used in current implementation)
 * @param tname     Target object specification (name, alias, DBref to set parent for)
 * @param pname     Parent object specification (name, alias, DBref), or empty to clear
 *
 * @note Thread-safe: No (modifies object parent chain, updates modification timestamp)
 * @note Quiet flags checked on both object and player before notifications
 * @note Parent chain limit enforced by mushconf.parent_nest_lim (prevents deep chains)
 * @note Always marks object as modified (s_Modified) even if parent unchanged
 * @attention Circular reference check compares actual object numbers, not names
 * @attention Both object and player must lack Quiet flag to receive notification
 * @attention Parentable() predicate checked even for parent clearing (see code flow)
 *
 * @see Parentable() for parent permission predicate
 * @see Parent() macro for accessing object parent
 * @see s_Parent() setter for parent modification
 * @see s_Modified() to update object's modification timestamp
 * @see mushconf.parent_nest_lim configuration limit
 */
void do_parent(dbref player, dbref cause, int key, char *tname, char *pname)
{
    dbref thing = NOTHING, parent = NOTHING, curr = NOTHING;
    int lev = 0;

    /* get victim */
    init_match(player, tname, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();

    if (thing == NOTHING)
    {
        return;
    }

    /* Make sure we can do it */
    if (!Controls(player, thing))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        return;
    }

    /* Find out what the new parent is */
    parent = NOTHING;
    
    if (*pname)
    {
        init_match(player, pname, Typeof(thing));
        match_everything(0);
        parent = noisy_match_result();

        if (parent == NOTHING)
        {
            return;
        }

        /* Make sure we have rights to set parent */
        if (!Parentable(player, parent))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }

        /* Verify no recursive reference */
        for (lev = 0, curr = parent; (Good_obj(curr) && (lev < mushconf.parent_nest_lim)); curr = Parent(curr), lev++)
        {
            if (curr == thing)
            {
                notify_quiet(player, "You can't have yourself as a parent!");
                return;
            }
        }
    }

    s_Parent(thing, parent);
    s_Modified(thing);

    if (!Quiet(thing) && !Quiet(player))
    {
        notify_quiet(player, (parent == NOTHING) ? "Parent cleared." : "Parent set.");
    }
}

/**
 * @brief Command handler for @dig: create a new room with optional exits and automatic teleport.
 *
 * Implements the @dig command to create a new room in the game world. Supports optional
 * creation of forward/back-link exits connecting the player's current location to the new
 * room, and optional automatic teleportation to the newly created room. Note that rooms
 * can be created without requiring knowledge of player's current location.
 *
 * Room creation process:
 * 1. Validates room name is non-empty (returns error "Dig what?" if empty)
 * 2. Creates new TYPE_ROOM object with create_obj() (returns NOTHING if quota exceeded)
 * 3. Notifies player of creation: "{name} created with room number {dbref}."
 * 4. If nargs >= 1: Creates forward exit from current location to new room using args[0] as direction
 * 5. If nargs >= 2: Creates back-link exit from new room to current location using args[1] as direction
 * 6. If DIG_TELEPORT flag: Teleports player into newly created room
 *
 * Exit creation:
 * - Forward exit (nargs >= 1): open_exit(player, Location(player), args[0], "{room_number}")
 *   Connects player's location to new room via direction specified in args[0]
 * - Back-link (nargs >= 2): open_exit(player, room, args[1], "{location_number}")
 *   Connects new room back to player's original location via direction in args[1]
 * - Both exits share cost (linkcost); created via open_exit() which handles validation
 * - Exit creation failures do not prevent room creation or subsequent exit attempts
 *
 * Teleportation behavior:
 * - Only executed if DIG_TELEPORT flag set (e.g., @dig/teleport)
 * - Uses move_via_teleport() to transport player to new room
 * - Teleport respects normal movement restrictions (e.g., JUMP_OK, control checks)
 *
 * @param player    DBref of player issuing @dig command (becomes owner of new room)
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches: DIG_TELEPORT teleports player to room after creation
 * @param name      Name of room to create (non-empty required; must pass ok_name() checks)
 * @param args[]    Array of exit direction specifications: args[0]=forward exit name, args[1]=back-link name
 * @param nargs     Number of elements in args array (0=room only, 1=forward exit only, 2+=both exits)
 *
 * @note Thread-safe: No (creates objects, modifies exit lists, transfers player, charges coins)
 * @note Room creation not dependent on player's current location (no location validation needed)
 * @note Exits created via open_exit() inherit that function's cost/permission model
 * @note XASPRINTF used to convert DBref numbers to string destinations for exits
 * @note Notifies player of room creation regardless of exit or teleport success
 * @attention If room creation fails (quota exceeded), function returns early; no exits created
 * @attention Exit creation failures do not roll back room (room persists even if exits fail)
 * @attention Both forward and back-link creation charge player separately; costs not combined
 * @attention DIG_TELEPORT applied after exits; player teleported even if exit creation fails
 *
 * @see _create_open_exit() for exit creation logic, permission validation, cost calculation
 * @see create_obj() for room object creation and quota enforcement
 * @see move_via_teleport() for teleportation logic and restrictions
 * @see DIG_TELEPORT flag definition
 * @see ok_name() for room name validation
 */
void do_dig(dbref player, dbref cause, int key, char *name, char *args[], int nargs)
{
    dbref room = NOTHING;
    char *s = NULL;

    /* we don't need to know player's location!  hooray! */
    if (!name || !*name)
    {
        notify_quiet(player, "Dig what?");
        return;
    }

    room = create_obj(player, TYPE_ROOM, name, 0);

    if (room == NOTHING)
    {
        return;
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s created with room number %d.", name, room);

    if ((nargs >= 1) && args[0] && *args[0])
    {
        s = XASPRINTF("s", "%d", room);
        _create_open_exit(player, Location(player), args[0], s);
        XFREE(s);
    }

    if ((nargs >= 2) && args[1] && *args[1])
    {
        s = XASPRINTF("s", "%d", Location(player));
        _create_open_exit(player, room, args[1], s);
        XFREE(s);
    }

    if (key == DIG_TELEPORT)
    {
        (void)move_via_teleport(player, room, cause, 0);
    }
}

/**
 * @brief Command handler for @create: make a new tangible object (TYPE_THING) with optional cost.
 *
 * Implements the @create command to create a new thing object with specified name and optional
 * initial cost (represented as pennies/endowment). Validates input parameters, creates the object,
 * places it in player's inventory, sets default home location, and notifies player of success.
 * The cost parameter determines initial money value the thing possesses.
 *
 * Input validation:
 * - Name must be non-empty and not strip to empty via ansi_strip_ansi_len()
 * - Returns "Create what?" if name is empty/invalid
 * - coststr optional; if provided, parses as integer via strtol()
 * - Cost must be within INT range (checked via errno ERANGE, INT_MAX/INT_MIN)
 * - Returns "Invalid cost value." if parsing fails or out of range
 * - Negative costs rejected with error message (no creation at negative cost)
 *
 * Object creation process:
 * 1. Validates name and cost parameters
 * 2. Creates new TYPE_THING object with create_obj(player, TYPE_THING, name, cost)
 * 3. Returns early if creation fails (quota exceeded, returns NOTHING)
 * 4. Moves object to player's inventory via move_via_generic()
 * 5. Sets object's home to player's default home via new_home(player)
 * 6. Notifies player unless player has Quiet flag
 *
 * Cost semantics:
 * - Cost parameter becomes initial penny endowment for the thing
 * - Uses OBJECT_ENDOWMENT() macro to calculate actual starting pennies
 * - Cost within system limits (enforced by create_obj or configuration)
 * - Zero cost allowed (creates "worthless" thing)
 *
 * @param player    DBref of player issuing @create command (becomes owner)
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches (not used in current implementation)
 * @param name      Thing name specification (non-empty, valid characters required)
 * @param coststr   Optional cost string (integer pennies); NULL or empty for default cost
 *
 * @note Thread-safe: No (creates objects, modifies player inventory, stores in database)
 * @note Cost parsing uses strtol() with errno checking; does not validate endptr format initially
 * @note ANSI stripping applied to name to check if valid after color code removal
 * @note Object placed in player's inventory immediately after creation
 * @note Notifies player of object number only if not Quiet
 * @attention Negative costs explicitly rejected before object creation
 * @attention Cost values > INT_MAX or < INT_MIN rejected as out of range
 * @attention Object creation failures (quota, permission) return early; no inventory placement
 * @attention Home location determined by new_home(player), not player's current location
 *
 * @see create_obj() for thing object creation and cost enforcement
 * @see move_via_generic() for placing object in player inventory
 * @see new_home() for default home location determination
 * @see OBJECT_ENDOWMENT() for cost-to-pennies conversion
 * @see ansi_strip_ansi_len() for ANSI color code stripping validation
 * @see strtol() for cost string parsing
 */
void do_create(dbref player, dbref cause, int key, char *name, char *coststr)
{
    dbref thing = NOTHING;
    int cost = 0;
    char *endptr = NULL;
    long val = 0;

    if (!name || !*name || (ansi_strip_ansi_len(name) == 0))
    {
        notify_quiet(player, "Create what?");
        return;
    }

    if (coststr && *coststr)
    {
        errno = 0;
        val = strtol(coststr, &endptr, 10);

        if (errno == ERANGE || val > INT_MAX || val < INT_MIN)
        {
            notify_quiet(player, "Invalid cost value.");
            return;
        }

        if (endptr == coststr || *endptr != '\0')
        {
            notify_quiet(player, "Invalid cost value.");
            return;
        }

        cost = (int)val;
    }

    if (cost < 0)
    {
        notify_quiet(player, "You can't create an object for less than nothing!");
        return;
    }

    thing = create_obj(player, TYPE_THING, name, cost);

    if (thing == NOTHING)
    {
        return;
    }

    move_via_generic(thing, player, NOTHING, 0);
    s_Home(thing, new_home(player));

    if (!Quiet(player))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s created as object #%d", Name(thing), thing);
    }
}

/**
 * @brief Command handler for @clone: duplicate an existing object with optional customizations.
 *
 * Implements the @clone command to create an exact or customized copy of an existing object.
 * Supports cloning of different object types (THING, ROOM, EXIT) with extensive control over
 * ownership, attributes, flags, and cost. Handles complex permission checks, parent linking,
 * and object relocation. Created clone placed in player's location or inventory based on type.
 *
 * Cloning eligibility and restrictions:
 * - Player must be Examinable to clone target object (View() permission checks)
 * - Cannot clone players (rejected with error "You cannot clone players!")
 * - Parent linking requires Controls() on source or Parent_ok() flag (with /parent switch)
 * - Ownership preservation (/preserve) requires Controls() on target's owner
 * - New owner defaults to player; may be overridden by /preserve switch
 *
 * Switch behaviors:
 * - /INVENTORY: Place clone in player inventory instead of player's location
 * - /SET_COST: Override default cost via arg2 parameter; validated within config limits
 * - /PRESERVE: Keep original owner (requires Controls on owner); deprecated behavior
 * - /FROM_PARENT: Link clone to source parent instead of copying attributes
 * - /INHERIT: Preserve INHERIT flag on clone (requires Inherits() power)
 * - /NOSTRIP: Keep original flags except WIZARD (unless player is God)
 *
 * Cost calculation by type:
 * - TYPE_THING: Default clone_copy_cost (copy pennies) or fixed value; respects createmin/createmax
 * - TYPE_ROOM: Uses digcost from configuration
 * - TYPE_EXIT: Requires Controls on location; uses opencost; fails if location uncontrollable
 *
 * Attribute handling:
 * - Default (/FROM_PARENT not set): Copies all attributes via atr_cpy(player, clone, source)
 * - With /FROM_PARENT: Links clone parent to source (s_Parent); no attribute copy
 * - Clears all attributes first via atr_free(), then copies or sets parent
 * - Resets name after attribute wipe (name cleared by atr_free)
 *
 * Flag management:
 * - Default (/NOSTRIP): Strips flags per mushconf.stripped_flags (word1, word2, word3)
 * - With /INHERIT and Inherits(): Preserves INHERIT flag
 * - With /NOSTRIP: Copies all flags/flags2/flags3 from source (unless God required)
 * - If clone owner differs from source, source WIZARD flag always stripped
 *
 * Object-specific post-processing:
 * - TYPE_THING: Sets home via clone_home(player, source); places in location/inventory
 * - TYPE_ROOM: Clears dropto; re-establishes via link_exit if source has Dropto
 * - TYPE_EXIT: Inserts into location exit list; clears location; re-links via link_exit if needed
 *
 * Aclone/halt behavior:
 * - Same owner: Triggers A_ACLONE attribute handler (did_it call)
 * - Different owner: Sets HALTED flag (prevents execution)
 *
 * @param player    DBref of player issuing @clone command
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches: bitmask of CLONE_INVENTORY, CLONE_SET_COST, CLONE_PRESERVE, etc.
 * @param name      Source object specification (name, alias, DBref to clone)
 * @param arg2      Optional cost override for /SET_COST switch; parsed as integer or ignored
 *
 * @note Thread-safe: No (creates objects, modifies attributes, transfers ownership, charges coins)
 * @note Examinable() check is permission boundary; Controls() not required for basic cloning
 * @note Clone name defaults to source name; may be customized via arg2 if /SET_COST not used
 * @note Attribute wipe (atr_free) clears name; must re-set via s_Name after copying/parent-linking
 * @note Parent chain limit enforced during parent linking (parent_nest_lim via Parent() macro)
 * @note Ownership changes trigger flag alterations (HALT set, WIZARD/INHERIT cleared)
 * @attention Players cannot be cloned regardless of permissions or switches
 * @attention /SET_COST and name customization share arg2; cannot use both simultaneously
 * @attention Quota enforcement occurs during create_obj; no rollback if quota exceeded mid-clone
 * @attention Different ownership prevents Aclone trigger; clone will be halted instead
 * @attention Dropto/exit re-linking may fail silently if destination uncontrollable/unlinkable
 *
 * @see Examinable() for clone permission predicate
 * @see create_obj() for object creation and cost enforcement
 * @see atr_cpy() for attribute copying
 * @see atr_free() for attribute clearing
 * @see _create_link_exit() for re-establishing exit links
 * @see clone_home() for destination home determination
 * @see did_it() for A_ACLONE attribute trigger
 * @see OBJECT_ENDOWMENT() for cost-to-pennies conversion
 * @see mushconf.stripped_flags for flag stripping configuration
 */
void do_clone(dbref player, dbref cause, int key, char *name, char *arg2)
{
    dbref clone = NOTHING, thing = NOTHING, new_owner = NOTHING, loc = NOTHING;
    FLAG rmv_flags = 0;
    int cost = 0;
    const char *clone_name = NULL;

    loc = ((key & CLONE_INVENTORY) || !Has_location(player)) ? player : Location(player);

    if (!Good_obj(loc))
    {
        return;
    }

    init_match(player, name, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();

    if ((thing == NOTHING) || (thing == AMBIGUOUS))
    {
        return;
    }

    /* Let players clone things set VISUAL.  It's easier than retyping in all that data */
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

    /* You can only make a parent link to what you control */
    if (!Controls(player, thing) && !Parent_ok(thing) && (key & CLONE_FROM_PARENT))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "You don't control %s, ignoring /parent.", Name(thing));
        key &= ~CLONE_FROM_PARENT;
    }

    /* You can only preserve the owner on the clone of an object owned by another player, if you control that player. */
    new_owner = (key & CLONE_PRESERVE) ? Owner(thing) : Owner(player);

    if ((new_owner != Owner(player)) && !Controls(player, new_owner))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "You don't control the owner of %s, ignoring /preserve.", Name(thing));
        new_owner = Owner(player);
    }

    /* Calculate clone cost (attributes wiped, so must enforce limits here) */
    cost = 0;
    
    if (key & CLONE_SET_COST)
    {
        if (arg2 && *arg2)
        {
            char *endptr = NULL;
            long val = 0;

            errno = 0;
            val = strtol(arg2, &endptr, 10);

            if (errno == ERANGE || val > INT_MAX || val < INT_MIN)
            {
                notify_quiet(player, "Invalid cost value.");
                return;
            }

            if (endptr == arg2 || *endptr != '\0')
            {
                notify_quiet(player, "Invalid cost value.");
                return;
            }

            cost = (int)val;
        }
        arg2 = NULL;
    }

    switch (Typeof(thing))
    {
    case TYPE_THING:
        if (key & CLONE_SET_COST)
        {
            if (cost < mushconf.createmin)
            {
                cost = mushconf.createmin;
            }

            if (cost > mushconf.createmax)
            {
                cost = mushconf.createmax;
            }
        }
        else
        {
            cost = OBJECT_DEPOSIT((mushconf.clone_copy_cost) ? Pennies(thing) : 1);
        }

        break;

    case TYPE_ROOM:
        cost = mushconf.digcost;
        break;

    case TYPE_EXIT:
        if (!Controls(player, loc))
        {
            notify_quiet(player, NOPERM_MESSAGE);
            return;
        }

        cost = mushconf.opencost;
        break;
    }

    /* Create clone object */
    clone_name = ((arg2 && *arg2) && ok_name(arg2)) ? arg2 : Name(thing);
    clone = create_obj(new_owner, Typeof(thing), clone_name, cost);

    if (clone == NOTHING)
    {
        return;
    }

    /* Clear attributes, then copy from source or set parent */
    atr_free(clone);

    if (key & CLONE_FROM_PARENT)
    {
        s_Parent(clone, thing);
    }
    else
    {
        atr_cpy(player, clone, thing);
    }

    /* Restore name (cleared by atr_free) */
    s_Name(clone, (char *)clone_name);

    /* Set cost (only things have pennies value) */
    if (isThing(clone))
    {
        s_Pennies(clone, OBJECT_ENDOWMENT(cost));
    }

    /* Apply flags: /nostrip preserves all (except WIZARD unless God), default strips per config */
    if (key & CLONE_NOSTRIP)
    {
        s_Flags(clone, God(player) ? Flags(thing) : (Flags(thing) & ~WIZARD));
        s_Flags2(clone, Flags2(thing));
        s_Flags3(clone, Flags3(thing));
    }
    else
    {
        rmv_flags = mushconf.stripped_flags.word1;

        if ((key & CLONE_INHERIT) && Inherits(player))
        {
            rmv_flags &= ~INHERIT;
        }

        s_Flags(clone, Flags(thing) & ~rmv_flags);
        s_Flags2(clone, Flags2(thing) & ~mushconf.stripped_flags.word2);
        s_Flags3(clone, Flags3(thing) & ~mushconf.stripped_flags.word3);
    }

    /* Notify creator of successful cloning */
    if (!Quiet(player))
    {
        if (arg2 && *arg2)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s cloned as %s, new copy is object #%d.", Name(thing), clone_name, clone);
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s cloned, new copy is object #%d.", Name(thing), clone);
        }
    }

    /* Place clone and re-establish links/dropto as appropriate */
    switch (Typeof(thing))
    {
    case TYPE_THING:
        s_Home(clone, clone_home(player, thing));
        move_via_generic(clone, loc, player, 0);
        break;

    case TYPE_ROOM:
        s_Dropto(clone, NOTHING);

        if (Dropto(thing) != NOTHING)
        {
            _create_link_exit(player, clone, Dropto(thing));
        }

        break;

    case TYPE_EXIT:
        s_Exits(loc, insert_first(Exits(loc), clone));
        s_Exits(clone, loc);
        s_Location(clone, NOTHING);

        if (Location(thing) != NOTHING)
        {
            _create_link_exit(player, clone, Location(thing));
        }

        break;
    }

    /* Trigger Aclone if same owner, otherwise halt; set parent as appropriate */
    if (new_owner == Owner(thing))
    {
        if (!(key & CLONE_FROM_PARENT))
        {
            s_Parent(clone, Parent(thing));
        }

        did_it(player, clone, A_NULL, NULL, A_NULL, NULL, A_ACLONE, 0, (char **)NULL, 0, MSG_MOVE);
    }
    else
    {
        if (!(key & CLONE_FROM_PARENT) && (Controls(player, thing) || Parent_ok(thing)))
        {
            s_Parent(clone, Parent(thing));
        }

        s_Halted(clone);
    }
}

/**
 * @brief Command handler for @pcreate: create new player characters or robot objects.
 *
 * Implements the @pcreate command to create new player characters or robot objects with
 * specified name and password. Distinguishes between regular players and robots via command
 * switches, placing them in appropriate starting locations. Logs all creation attempts
 * (success or failure) for administrative audit trails.
 *
 * Player vs Robot distinction:
 * - **Regular players** (key != PCRE_ROBOT): Full player characters with login capability
 *   - Placed in mushconf.start_room (or dbref 0 if start_room invalid)
 *   - Logged to LOG_PCREATES | LOG_WIZARD with "WIZ" prefix
 *   - Notification: "New player '{name}' (#{dbref}) created with password '{pass}'"
 *
 * - **Robots** (key == PCRE_ROBOT): Player-controlled puppet objects
 *   - Placed at creator's current location via Location(player)
 *   - Logged to LOG_PCREATES with "CRE" prefix (no WIZARD flag)
 *   - Notification: "New robot '{name}' (#{dbref}) created with password '{pass}'"
 *   - Additional notification: "Your robot has arrived."
 *
 * Creation process:
 * 1. Determines type via key comparison (PCRE_ROBOT check)
 * 2. Obtains creator name via log_getname() for logging
 * 3. Creates player/robot object via create_player(name, pass, creator, isrobot, 0)
 * 4. Munges name with munge_space() for logging consistency
 * 5. On failure: Notifies creator, logs failure, frees memory, returns early
 * 6. On success: Places in location, notifies creator, logs creation, frees memory
 *
 * Failure handling:
 * - create_player() returns NOTHING on failure (quota exceeded, invalid name, duplicate name)
 * - Notifies player with "Failure creating '{name}'"
 * - Logs failure with creator name for administrative tracking
 * - Separate log entries for robots vs players (different log flags/prefixes)
 *
 * Memory management:
 * - Allocates strings via log_getname() and munge_space()
 * - All allocations freed via XFREE() in both success and failure paths
 * - Three allocated strings: cname (creator), nname (new player), newname (munged)
 *
 * @param player    DBref of player issuing @pcreate command (becomes creator)
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches: PCRE_ROBOT creates robot, otherwise creates player
 * @param name      Name for new player/robot (must be valid, non-duplicate)
 * @param pass      Password for new player/robot (stored for authentication)
 *
 * @note Thread-safe: No (creates player objects, modifies database, writes logs)
 * @note Wizard-level command; requires appropriate permissions (enforced by command dispatcher)
 * @note Password stored in plaintext or hashed depending on create_player() implementation
 * @note Robots count against creator's quota; players do not (system-level objects)
 * @note Start room fallback to dbref 0 if mushconf.start_room invalid (Good_loc check)
 * @note Notifies player unless player has Quiet flag (via notify_check with MSG_PUP_ALWAYS)
 * @attention Both success and failure paths log to administrative logs (audit trail)
 * @attention Robot creation logs differ from player creation (different flags/prefixes)
 * @attention Memory leaks prevented via XFREE() in all exit paths
 * @attention Name validation occurs in create_player(); this function does not pre-validate
 *
 * @see create_player() for actual player/robot object creation and validation
 * @see move_object() for placing new player/robot in starting location
 * @see log_getname() for obtaining printable name strings
 * @see munge_space() for normalizing names for logging
 * @see Good_loc() for validating start room location
 * @see mushconf.start_room for default player starting location
 * @see LOG_PCREATES for player creation log flag
 * @see LOG_WIZARD for wizard action log flag
 */
void do_pcreate(dbref player, dbref cause, int key, char *name, char *pass)
{
    dbref newplayer = NOTHING;
    char *newname = NULL, *cname = NULL, *nname = NULL;
    bool isrobot = (key == PCRE_ROBOT);

    cname = log_getname(player);
    newplayer = create_player(name, pass, player, isrobot, 0);
    newname = munge_space(name);

    if (newplayer == NOTHING)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Failure creating '%s'", newname);
        log_write(isrobot ? LOG_PCREATES : (LOG_PCREATES | LOG_WIZARD), 
                  isrobot ? "CRE" : "WIZ", 
                  isrobot ? "ROBOT" : "PCREA", 
                  "Failure creating '%s' by %s", newname, cname);
        XFREE(cname);
        XFREE(newname);
        return;
    }

    nname = log_getname(newplayer);

    if (isrobot)
    {
        move_object(newplayer, Location(player));
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "New robot '%s' (#%d) created with password '%s'", newname, newplayer, pass);
        notify_quiet(player, "Your robot has arrived.");
        log_write(LOG_PCREATES, "CRE", "ROBOT", "%s created by %s", nname, cname);
    }
    else
    {
        move_object(newplayer, Good_loc(mushconf.start_room) ? mushconf.start_room : 0);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "New player '%s' (#%d) created with password '%s'", newname, newplayer, pass);
        log_write(LOG_PCREATES | LOG_WIZARD, "WIZ", "PCREA", "%s created by %s", nname, cname);
    }

    XFREE(cname);
    XFREE(nname);
    XFREE(newname);
}

/**
 * @brief Validates whether a player can destroy a specific exit object.
 *
 * Implements permission checking for exit destruction, enforcing location-based access control
 * and wizard override privileges. Players can destroy exits in their current location, exits
 * attached to themselves, or any exit if they possess wizard privileges. Prevents remote exit
 * destruction to maintain spatial integrity and prevent abuse.
 *
 * Permission validation logic (OR conditions):
 * - **Location match**: Player has valid location AND exit's location matches player's location
 *   - Checked via Has_location(player) && (Exits(exit) == Location(player))
 *   - Most common case: destroying exits in current room
 *   - Has_location() ensures player is not a ROOM or other locationless type
 *
 * - **Direct ownership**: Player DBref equals exit's location DBref
 *   - Handles exits attached directly to player object (rare case)
 *   - Allows exit cleanup on player-carried containers
 *
 * - **Self-reference**: Player DBref equals exit DBref
 *   - Edge case: player attempting to destroy themselves as exit
 *   - Prevents premature rejection before type-specific checks
 *
 * - **Wizard override**: Player has WIZARD flag set
 *   - Bypasses all location restrictions
 *   - Allows global exit administration
 *
 * Failure behavior:
 * - If no permission conditions met: notifies player with "You cannot destroy exits in another room."
 * - Returns false to abort destruction attempt
 * - No logging performed (called before actual destruction)
 *
 * @param player    DBref of player attempting exit destruction
 * @param exit      DBref of exit object to validate for destruction
 * @return true if player permitted to destroy exit (location match, ownership, or wizard privilege)
 * @return false if permission denied (exit in remote room without wizard privilege)
 *
 * @note Thread-safe: Yes (read-only database access, no state modification)
 * @note Called by do_destroy() before actual exit destruction occurs
 * @note Location validation prevents remote exit griefing across rooms
 * @note Exits(exit) macro returns exit's parent location (room containing exit)
 * @note Wizard check bypasses location restrictions entirely
 * @attention Does not validate exit existence (assumes valid DBref passed from caller)
 * @attention Does not check SAFE flag or Controls() predicate (handled by do_destroy)
 * @attention Notification sent only on failure; success returns silently
 * @attention Location(player) may be NOTHING for locationless objects (checked via Has_location)
 *
 * @see do_destroy() for complete destruction logic and SAFE flag validation
 * @see Exits() macro for retrieving exit's parent location
 * @see Has_location() for validating player has valid location
 * @see Wizard() predicate for wizard privilege check
 * @see destroyable() for special object protection checks
 */
static bool _create_can_destroy_exit(dbref player, dbref exit)
{
    dbref loc = Exits(exit);

    if (!((Has_location(player) && (loc == Location(player))) || (player == loc) || (player == exit) || Wizard(player)))
    {
        notify_quiet(player, "You cannot destroy exits in another room.");
        return false;
    }

    return true;
}

/**
 * @brief Validates whether an object is eligible for destruction or protected as special system object.
 *
 * Implements protection checking for critical system objects referenced in configuration tables,
 * preventing accidental or malicious destruction of essential database objects. Scans both core
 * configuration table and all loaded module configuration tables for DBref references matching
 * the target victim. Special handling for dbref 0 (master room) and God player.
 *
 * Protection criteria (returns false if any condition met):
 * - **DBref 0**: Master room/object 0 is foundational; destruction would corrupt database
 *   - Checked via `victim == (dbref)0` comparison
 *   - Unconditionally protected regardless of permissions
 *
 * - **God player**: Highest privilege level player; destruction would break administrative chain
 *   - Checked via God(victim) predicate
 *   - Even wizards cannot destroy God
 *
 * - **Core conftable references**: Objects referenced in core conftable as cf_dbref type
 *   - Iterates conftable[] entries checking tp->interpreter == cf_dbref
 *   - Compares victim against *((dbref *)(tp->loc)) for match
 *   - Examples: default_home, master_room, start_room, player_start, etc.
 *   - Protects system-configured special locations and objects
 *
 * - **Module conftable references**: Objects referenced in loaded module configuration tables
 *   - Iterates mushstate.modules_list for all loaded modules
 *   - Uses dlsym_format() to locate "mod_{modname}_conftable" symbol in module
 *   - Scans module conftable for cf_dbref entries matching victim
 *   - Allows modules to protect their own system objects
 *
 * Configuration table scanning logic:
 * 1. Core table scan via conftable[] iteration until tp->pname is NULL
 * 2. Module scan via mushstate.modules_list linked list traversal
 * 3. Each module's conftable loaded dynamically via dlsym_format()
 * 4. Module tables scanned identically to core table (tp->pname termination)
 * 5. First match in any table returns false (non-destroyable)
 *
 * Destruction eligibility:
 * - Returns true if no protection criteria matched (normal objects)
 * - Returns false if any special object condition detected
 * - Called by do_destroy() before type-specific validation
 * - Complements but does not replace Controls() or SAFE flag checks
 *
 * @param victim    DBref of object to validate for destruction eligibility
 * @return true if object is destroyable (not special, not protected by configuration)
 * @return false if object is protected (dbref 0, God, or configuration table reference)
 *
 * @note Thread-safe: Yes (read-only access to conftable, modules_list, configuration data)
 * @note Called early in destruction validation pipeline before permission checks
 * @note Does not check SAFE flag, Controls(), or ownership (handled by do_destroy)
 * @note Configuration table protection applies to objects of any type (ROOM, THING, PLAYER, EXIT)
 * @note Module system integration allows extensibility for custom protection rules
 * @note dlsym_format() returns NULL if module lacks conftable (not an error; skipped)
 * @attention DBref 0 and God protection cannot be overridden by any flags or permissions
 * @attention Configuration table matches prevent destruction even by God with /override
 * @attention Module unloading does not remove protection; references persist until server restart
 * @attention No notification sent by this function; do_destroy() handles user feedback
 *
 * @see do_destroy() for complete destruction logic and permission validation
 * @see cf_dbref for configuration table DBref interpreter type
 * @see conftable for core configuration table structure
 * @see mushstate.modules_list for loaded module list
 * @see dlsym_format() for dynamic module symbol lookup
 * @see God() predicate for God player check
 * @see CONF structure for configuration table entry format
 */
bool destroyable(dbref victim)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    if ((victim == (dbref)0) || God(victim))
    {
        return false;
    }

    for (tp = conftable; tp->pname; tp++)
    {
        if (tp->interpreter == cf_dbref && victim == *((dbref *)(tp->loc)))
        {
            return false;
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
        
        if (ctab != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (tp->interpreter == cf_dbref && victim == *((dbref *)(tp->loc)))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * @brief Validates whether a player can destroy another player character (including self-destruction).
 *
 * Implements permission checking for player destruction, enforcing wizard privilege requirements
 * and protecting wizard characters from destruction. Restricts player destruction to wizard-level
 * administrators while preventing even wizards from destroying each other. Provides safeguards
 * against accidental or malicious player account deletion.
 *
 * Permission validation logic (sequential checks):
 * - **Wizard requirement**: Player attempting destruction must have WIZARD flag set
 *   - Checked via Wizard(player) predicate
 *   - Prevents non-wizards from destroying any player (including themselves)
 *   - Notification on failure: "Sorry, no suicide allowed."
 *   - Blocks self-destruction by non-wizard players (prevents account suicide)
 *
 * - **Wizard protection**: Victim player must NOT have WIZARD flag set
 *   - Checked via Wizard(victim) predicate
 *   - Prevents wizards from destroying other wizards
 *   - Notification on failure: "Even you can't do that!"
 *   - Protects administrative hierarchy from lateral attacks
 *
 * Destruction eligibility by scenario:
 * - **Non-wizard destroys self**: Rejected (no suicide allowed)
 * - **Non-wizard destroys other non-wizard**: Rejected (requires wizard privileges)
 * - **Wizard destroys non-wizard**: Allowed (normal administrative action)
 * - **Wizard destroys wizard**: Rejected (wizard protection)
 * - **Wizard destroys self**: Rejected (wizard victim check prevents wizard suicide)
 *
 * Failure notifications:
 * - "Sorry, no suicide allowed." - Player lacks wizard privileges for any player destruction
 * - "Even you can't do that!" - Wizard attempting to destroy another wizard
 * - Both notifications use notify_quiet() (no room broadcast)
 *
 * @param player    DBref of player attempting player destruction
 * @param victim    DBref of player character targeted for destruction
 * @return true if player permitted to destroy victim (wizard destroying non-wizard)
 * @return false if permission denied (non-wizard player, or wizard victim)
 *
 * @note Thread-safe: Yes (read-only flag checks, no state modification)
 * @note Called by do_destroy() after general permission checks and before actual destruction
 * @note "No suicide allowed" message applies to both self-destruction and destroying others
 * @note Wizard protection absolute; no override switch bypasses wizard-vs-wizard restriction
 * @note Does not check SAFE flag, Controls(), or destroyable() (handled by do_destroy)
 * @note God player check performed elsewhere; this function treats God as wizard
 * @attention Non-wizard players cannot destroy any player character including themselves
 * @attention Wizards cannot destroy themselves (blocked by wizard victim check)
 * @attention Wizard mutual destruction prevention maintains administrative stability
 * @attention Notifications sent on failure; success returns silently for do_destroy to handle
 *
 * @see do_destroy() for complete destruction logic including instant/queued destruction
 * @see Wizard() predicate for wizard privilege check
 * @see destroyable() for special object protection (God player handled there)
 * @see _create_can_destroy_exit() for exit-specific destruction validation
 * @see destroy_player() for actual player destruction implementation
 */
static bool _create_can_destroy_player(dbref player, dbref victim)
{
    if (!Wizard(player))
    {
        notify_quiet(player, "Sorry, no suicide allowed.");
        return false;
    }

    if (Wizard(victim))
    {
        notify_quiet(player, "Even you can't do that!");
        return false;
    }

    return true;
}

/**
 * @brief Command handler for @destroy: remove objects from the database with permission validation.
 *
 * Implements the @destroy command to permanently remove or queue for removal objects from the
 * database. Performs multi-stage permission validation including control checks, SAFE flag
 * protection, special object protection, and type-specific validation. Supports both instant
 * destruction and queued destruction modes. Handles exits, players, rooms, things, and garbage
 * with type-appropriate cleanup and notifications.
 *
 * Object matching and permission hierarchy (sequential attempts):
 * 1. **Controlled objects**: Match any object controlled by player via match_controlled_quiet()
 *    - Primary permission check: player Controls() the object
 *    - Most common case for destruction
 *
 * 2. **Location exits**: If first match fails AND player controls their location
 *    - Matches exits in player's current location via match_exit()
 *    - Allows room owners to destroy exits without controlling them directly
 *    - Validated further by can_destroy_exit() for location restrictions
 *
 * 3. **DESTROY_OK inventory items**: If previous matches fail
 *    - Matches things in player's inventory with DESTROY_OK flag set
 *    - Type must be TYPE_THING (not players, rooms, or exits)
 *    - Provides mechanism for items to allow player destruction without Controls()
 *
 * Protection validation stages (sequential checks, early return on failure):
 * 1. Match status validation: Returns early if no valid object matched
 * 2. SAFE flag protection: Blocks destruction unless DEST_OVERRIDE switch used
 *    - Exception: DESTROY_OK things bypass SAFE protection
 *    - Notification: "Sorry, that object is protected. Use @destroy/override to destroy it."
 * 3. Special object protection: Calls destroyable() to check system objects
 *    - Protects dbref 0, God, configuration table references
 *    - Notification: "You can't destroy that!"
 * 4. Type-specific validation: Calls can_destroy_exit() or can_destroy_player()
 *    - Exit: Location-based permission checks
 *    - Player: Wizard privilege requirements
 *    - Room/Thing/Garbage: Always permitted after reaching this stage
 *
 * Destruction modes:
 * - **Instant destruction** (immediate database removal):
 *   - Triggered by: DEST_INSTANT switch OR mushconf.instant_recycle enabled
 *   - instant_recycle applies to: DESTROY_OK objects OR objects owned by DESTROY_OK owner
 *   - Type-specific handlers: destroy_exit(), destroy_player(), empty_obj()+destroy_obj(), destroy_thing()
 *   - Players: A_DESTROYER attribute set to player DBref before destruction
 *   - No queue delay; object immediately removed from database
 *
 * - **Queued destruction** (delayed removal via Going flag):
 *   - Default mode when instant conditions not met
 *   - Sets Going flag via s_Going(thing) for later cleanup
 *   - Notifications: "The {typename} shakes and begins to crumble."
 *   - Owner notified: "You will be rewarded shortly for {name}(#{dbref})."
 *   - Players: A_DESTROYER attribute recorded for audit trail
 *   - Actual destruction occurs during periodic database maintenance
 *
 * Already-Going object handling:
 * - If object already has Going flag: "That {typename} has already been destroyed."
 * - Exception: /instant switch can immediately destroy Going objects (except TYPE_GARBAGE)
 * - Prevents duplicate destruction queue entries
 *
 * Notification behavior:
 * - Success notifications respect player Quiet flag
 * - Owner notifications respect owner and object Quiet flags
 * - Room destruction notifies all contents via notify_all()
 * - Other types notify player with MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN
 * - Different owner than player: Additional notification with owner name
 *
 * @param player    DBref of player issuing @destroy command
 * @param cause     DBref of cause (usually equals player; used in logging/triggers)
 * @param key       Command switches: bitmask of DEST_OVERRIDE (ignore SAFE), DEST_INSTANT (immediate)
 * @param what      Object name/alias/DBref specification to destroy
 *
 * @note Thread-safe: No (modifies database, destroys objects, sets Going flag, sends notifications)
 * @note Multiple matching attempts allow flexible destruction without requiring explicit object specifications
 * @note SAFE flag provides protection layer; requires explicit /override to bypass
 * @note instant_recycle configuration allows automatic immediate destruction of DESTROY_OK objects
 * @note A_DESTROYER attribute tracking provides audit trail for player destruction
 * @note Queued destruction (Going flag) allows objects to trigger cleanup code before removal
 * @note Empty rooms notified via notify_all() to inform all occupants of destruction
 * @attention DEST_OVERRIDE bypasses SAFE flag but not special object protection or type-specific checks
 * @attention Already-Going objects cannot be destroyed again except with /instant switch
 * @attention Player destruction records destroyer DBref in A_DESTROYER before removal
 * @attention Exit destruction requires location permission validation via can_destroy_exit()
 * @attention Wizard/God protection enforced via can_destroy_player() and destroyable()
 * @attention TYPE_GARBAGE can be queued but not instantly destroyed (protection against corruption)
 *
 * @see match_controlled_quiet() for primary object matching and control validation
 * @see controls() for ownership/control predicate
 * @see destroyable() for special object protection (dbref 0, God, conftable references)
 * @see _create_can_destroy_exit() for exit-specific location permission validation
 * @see _create_can_destroy_player() for player-specific wizard privilege validation
 * @see destroy_exit() for instant exit removal implementation
 * @see destroy_player() for instant player removal implementation
 * @see destroy_thing() for instant thing removal implementation
 * @see destroy_obj() for generic object removal implementation
 * @see s_Going() for queued destruction flag setter
 * @see Safe() predicate for SAFE flag check
 * @see Destroy_ok() predicate for DESTROY_OK flag check
 * @see mushconf.instant_recycle for instant destruction configuration
 * @see DEST_OVERRIDE flag for SAFE override switch
 * @see DEST_INSTANT flag for instant destruction switch
 */
void do_destroy(dbref player, dbref cause, int key, char *what)
{
    dbref thing = NOTHING;
    bool can_doit = false;
    const char *typename = NULL;
    char *t = NULL, *tbuf = NULL, *s = NULL;

    /* You can destroy anything you control. */
    thing = match_controlled_quiet(player, what);

    /* If you own a location, you can destroy its exits. */
    if ((thing == NOTHING) && controls(player, Location(player)))
    {
        init_match(player, what, TYPE_EXIT);
        match_exit();
        thing = last_match_result();
    }

    /* You can destroy DESTROY_OK things in your inventory. */
    if (thing == NOTHING)
    {
        init_match(player, what, TYPE_THING);
        match_possession();
        thing = last_match_result();

        if (thing != NOTHING && !(isThing(thing) && Destroy_ok(thing)))
        {
            thing = NOPERM;
        }
    }

    /* Return an error if we didn't find anything to destroy. */
    if (match_status(player, thing) == NOTHING)
    {
        return;
    }

    /* Check SAFE and DESTROY_OK flags */
    if (Safe(thing, player) && !(key & DEST_OVERRIDE) && !(isThing(thing) && Destroy_ok(thing)))
    {
        notify_quiet(player, "Sorry, that object is protected.  Use @destroy/override to destroy it.");
        return;
    }

    /* Make sure we're not trying to destroy a special object */
    if (!destroyable(thing))
    {
        notify_quiet(player, "You can't destroy that!");
        return;
    }

    /* Make sure we can do it, on a type-specific basis */
    switch (Typeof(thing))
    {
    case TYPE_EXIT:
        typename = "exit";
        can_doit = _create_can_destroy_exit(player, thing);
        break;

    case TYPE_PLAYER:
        typename = "player";
        can_doit = _create_can_destroy_player(player, thing);
        break;

    case TYPE_ROOM:
        typename = "room";
        can_doit = true;
        break;

    case TYPE_THING:
        typename = "thing";
        can_doit = true;
        break;

    case TYPE_GARBAGE:
        typename = "garbage";
        can_doit = true;
        break;

    default:
        typename = "weird object";
        can_doit = true;
        break;
    }

    if (!can_doit)
    {
        return;
    }

    /* Allow /instant to destroy already-Going objects (except GARBAGE) */
    if (Going(thing) && !((key & DEST_INSTANT) && (Typeof(thing) != TYPE_GARBAGE)))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "That %s has already been destroyed.", typename);
        return;
    }

    /* Instant destruction: /instant switch or instant_recycle for DESTROY_OK objects */
    if ((key & DEST_INSTANT) || (mushconf.instant_recycle && (Destroy_ok(thing) || Destroy_ok(Owner(thing)))))
    {
        switch (Typeof(thing))
        {
        case TYPE_EXIT:
            destroy_exit(thing);
            break;

        case TYPE_PLAYER:
            s = XASPRINTF("s", "%d", player);
            atr_add_raw(thing, A_DESTROYER, s);
            destroy_player(thing);
            XFREE(s);
            break;

        case TYPE_ROOM:
            empty_obj(thing);
            destroy_obj(NOTHING, thing);
            break;

        case TYPE_THING:
            destroy_thing(thing);
            break;

        default:
            notify(player, "Weird object type cannot be destroyed.");
            break;
        }

        return;
    }

    /* Queue for destruction (set Going flag after notifications) */
    if (!isRoom(thing))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "The %s shakes and begins to crumble.", typename);
    }
    else
    {
        notify_all(thing, player, "The room shakes and begins to crumble.");
    }

    if (!Quiet(thing) && !Quiet(Owner(thing)))
    {
        notify_check(Owner(thing), Owner(thing), MSG_PUP_ALWAYS | MSG_ME, "You will be rewarded shortly for %s(#%d).", Name(thing), thing);
    }

    if (Owner(thing) != player && !Quiet(player))
    {
        t = tbuf = XMALLOC(SBUF_SIZE, "t");
        XSAFESBSTR(Name(Owner(thing)), tbuf, &t);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Destroyed. %s's %s(#%d)", tbuf, Name(thing), thing);
        XFREE(tbuf);
    }

    if (isPlayer(thing))
    {
        s = XASPRINTF("s", "%d", player);
        atr_add_raw(thing, A_DESTROYER, s);
        XFREE(s);
    }

    s_Going(thing);
}
