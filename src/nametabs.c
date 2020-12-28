/**
 * @file cmdtabs.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command and other supporting tables
 * @version 3.3
 * @date 2020-12-25
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"
#include "game.h"
#include "alloc.h"
#include "flags.h"
#include "htab.h"
#include "ltdl.h"
#include "udb.h"
#include "udb_defs.h"
#include "mushconf.h"
#include "attrs.h"
#include "command.h"
#include "interface.h"
#include "db.h"
#include "externs.h"

/**
 * @brief Switch tables for the various commands.
 * @attention  Make sure that all of your command and switch names are lowercase!
 * 
 */
NAMETAB addcmd_sw[] = {
    {(char *)"preserve", 1, CA_GOD, ADDCMD_PRESERVE},
    {NULL, 0, 0, 0}};

NAMETAB attrib_sw[] = {
    {(char *)"access", 1, CA_GOD, ATTRIB_ACCESS},
    {(char *)"delete", 1, CA_GOD, ATTRIB_DELETE},
    {(char *)"info", 1, CA_WIZARD, ATTRIB_INFO},
    {(char *)"rename", 1, CA_GOD, ATTRIB_RENAME},
    {NULL, 0, 0, 0}};

NAMETAB boot_sw[] = {
    {(char *)"port", 1, CA_WIZARD, BOOT_PORT | SW_MULTIPLE},
    {(char *)"quiet", 1, CA_WIZARD, BOOT_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB chown_sw[] = {
    {(char *)"nostrip", 1, CA_WIZARD, CHOWN_NOSTRIP},
    {NULL, 0, 0, 0}};

NAMETAB chzone_sw[] = {
    {(char *)"nostrip", 1, CA_WIZARD, CHZONE_NOSTRIP},
    {NULL, 0, 0, 0}};

NAMETAB clone_sw[] = {
    {(char *)"cost", 1, CA_PUBLIC, CLONE_SET_COST | SW_MULTIPLE},
    {(char *)"inherit", 3, CA_PUBLIC, CLONE_INHERIT | SW_MULTIPLE},
    {(char *)"inventory", 3, CA_PUBLIC, CLONE_INVENTORY},
    {(char *)"location", 1, CA_PUBLIC, CLONE_LOCATION},
    {(char *)"nostrip", 1, CA_WIZARD, CLONE_NOSTRIP | SW_MULTIPLE},
    {(char *)"parent", 2, CA_PUBLIC, CLONE_FROM_PARENT | SW_MULTIPLE},
    {(char *)"preserve", 2, CA_PUBLIC, CLONE_PRESERVE | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB decomp_sw[] = {
    {(char *)"pretty", 1, CA_PUBLIC, DECOMP_PRETTY},
    {NULL, 0, 0, 0}};

NAMETAB destroy_sw[] = {
    {(char *)"instant", 4, CA_PUBLIC, DEST_INSTANT | SW_MULTIPLE},
    {(char *)"override", 8, CA_PUBLIC, DEST_OVERRIDE | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB dig_sw[] = {
    {(char *)"teleport", 1, CA_PUBLIC, DIG_TELEPORT},
    {NULL, 0, 0, 0}};

NAMETAB doing_sw[] = {
    {(char *)"header", 1, CA_PUBLIC, DOING_HEADER | SW_MULTIPLE},
    {(char *)"message", 1, CA_PUBLIC, DOING_MESSAGE | SW_MULTIPLE},
    {(char *)"poll", 1, CA_PUBLIC, DOING_POLL},
    {(char *)"quiet", 1, CA_PUBLIC, DOING_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB dolist_sw[] = {
    {(char *)"delimit", 1, CA_PUBLIC, DOLIST_DELIMIT},
    {(char *)"space", 1, CA_PUBLIC, DOLIST_SPACE},
    {(char *)"notify", 1, CA_PUBLIC, DOLIST_NOTIFY | SW_MULTIPLE},
    {(char *)"now", 1, CA_PUBLIC, DOLIST_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB drop_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, DROP_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB dump_sw[] = {
    {(char *)"structure", 1, CA_WIZARD, DUMP_STRUCT | SW_MULTIPLE},
    {(char *)"text", 1, CA_WIZARD, DUMP_TEXT | SW_MULTIPLE},
    {(char *)"flatfile", 1, CA_WIZARD, DUMP_FLATFILE | SW_MULTIPLE},
    {(char *)"optimize", 1, CA_WIZARD, DUMP_OPTIMIZE | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB emit_sw[] = {
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"here", 1, CA_PUBLIC, SAY_HERE | SW_MULTIPLE},
    {(char *)"room", 1, CA_PUBLIC, SAY_ROOM | SW_MULTIPLE},
    {(char *)"html", 1, CA_PUBLIC, SAY_HTML | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB end_sw[] = {
    {(char *)"assert", 1, CA_PUBLIC, ENDCMD_ASSERT},
    {(char *)"break", 1, CA_PUBLIC, ENDCMD_BREAK},
    {NULL, 0, 0, 0}};

NAMETAB enter_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, MOVE_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB examine_sw[] = {
    {(char *)"brief", 1, CA_PUBLIC, EXAM_BRIEF},
    {(char *)"debug", 1, CA_WIZARD, EXAM_DEBUG},
    {(char *)"full", 1, CA_PUBLIC, EXAM_LONG},
    {(char *)"owner", 1, CA_PUBLIC, EXAM_OWNER},
    {(char *)"pairs", 3, CA_PUBLIC, EXAM_PAIRS},
    {(char *)"parent", 1, CA_PUBLIC, EXAM_PARENT | SW_MULTIPLE},
    {(char *)"pretty", 2, CA_PUBLIC, EXAM_PRETTY},
    {NULL, 0, 0, 0}};

NAMETAB femit_sw[] = {
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"here", 1, CA_PUBLIC, PEMIT_HERE | SW_MULTIPLE},
    {(char *)"room", 1, CA_PUBLIC, PEMIT_ROOM | SW_MULTIPLE},
    {(char *)"spoof", 1, CA_PUBLIC, PEMIT_SPOOF | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB fixdb_sw[] = {
    {(char *)"contents", 1, CA_GOD, FIXDB_CON},
    {(char *)"exits", 1, CA_GOD, FIXDB_EXITS},
    {(char *)"location", 1, CA_GOD, FIXDB_LOC},
    {(char *)"next", 1, CA_GOD, FIXDB_NEXT},
    {(char *)"owner", 1, CA_GOD, FIXDB_OWNER},
    {(char *)"pennies", 1, CA_GOD, FIXDB_PENNIES},
    {(char *)"rename", 1, CA_GOD, FIXDB_NAME},
    {NULL, 0, 0, 0}};

NAMETAB floaters_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, FLOATERS_ALL},
    {NULL, 0, 0, 0}};

NAMETAB force_sw[] = {
    {(char *)"now", 1, CA_PUBLIC, FRC_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB fpose_sw[] = {
    {(char *)"default", 1, CA_PUBLIC, 0},
    {(char *)"noeval", 3, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"nospace", 1, CA_PUBLIC, SAY_NOSPACE},
    {(char *)"spoof", 1, CA_PUBLIC, PEMIT_SPOOF | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB fsay_sw[] = {
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"spoof", 1, CA_PUBLIC, PEMIT_SPOOF | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB function_sw[] = {
    {(char *)"list", 1, CA_WIZARD, FUNCT_LIST},
    {(char *)"noeval", 1, CA_WIZARD, FUNCT_NO_EVAL | SW_MULTIPLE},
    {(char *)"privileged", 3, CA_WIZARD, FUNCT_PRIV | SW_MULTIPLE},
    {(char *)"private", 5, CA_WIZARD, FUNCT_NOREGS | SW_MULTIPLE},
    {(char *)"preserve", 3, CA_WIZARD, FUNCT_PRES | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB get_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, GET_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB give_sw[] = {
    {(char *)"quiet", 1, CA_WIZARD, GIVE_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB goto_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, MOVE_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB halt_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, HALT_ALL},
    {(char *)"pid", 1, CA_PUBLIC, HALT_PID},
    {NULL, 0, 0, 0}};

NAMETAB help_sw[] = {
    {(char *)"fine", 1, CA_PUBLIC, HELP_FIND},
    {NULL, 0, 0, 0}};

NAMETAB hook_sw[] = {
    {(char *)"before", 1, CA_GOD, HOOK_BEFORE},
    {(char *)"after", 1, CA_GOD, HOOK_AFTER},
    {(char *)"permit", 1, CA_GOD, HOOK_PERMIT},
    {(char *)"preserve", 3, CA_GOD, HOOK_PRESERVE},
    {(char *)"nopreserve", 1, CA_GOD, HOOK_NOPRESERVE},
    {(char *)"private", 3, CA_GOD, HOOK_PRIVATE},
    {NULL, 0, 0, 0}};

NAMETAB leave_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, MOVE_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB listmotd_sw[] = {
    {(char *)"brief", 1, CA_WIZARD, MOTD_BRIEF},
    {NULL, 0, 0, 0}};

NAMETAB lock_sw[] = {
    {(char *)"chownlock", 2, CA_PUBLIC, A_LCHOWN},
    {(char *)"controllock", 2, CA_PUBLIC, A_LCONTROL},
    {(char *)"defaultlock", 1, CA_PUBLIC, A_LOCK},
    {(char *)"darklock", 2, CA_PUBLIC, A_LDARK},
    {(char *)"droplock", 2, CA_PUBLIC, A_LDROP},
    {(char *)"enterlock", 1, CA_PUBLIC, A_LENTER},
    {(char *)"givelock", 2, CA_PUBLIC, A_LGIVE},
    {(char *)"heardlock", 5, CA_PUBLIC, A_LHEARD},
    {(char *)"hearslock", 5, CA_PUBLIC, A_LHEARS},
    {(char *)"knownlock", 5, CA_PUBLIC, A_LKNOWN},
    {(char *)"knowslock", 5, CA_PUBLIC, A_LKNOWS},
    {(char *)"leavelock", 2, CA_PUBLIC, A_LLEAVE},
    {(char *)"linklock", 2, CA_PUBLIC, A_LLINK},
    {(char *)"movedlock", 5, CA_PUBLIC, A_LMOVED},
    {(char *)"moveslock", 5, CA_PUBLIC, A_LMOVES},
    {(char *)"pagelock", 3, CA_PUBLIC, A_LPAGE},
    {(char *)"parentlock", 3, CA_PUBLIC, A_LPARENT},
    {(char *)"receivelock", 1, CA_PUBLIC, A_LRECEIVE},
    {(char *)"teloutlock", 2, CA_PUBLIC, A_LTELOUT},
    {(char *)"tportlock", 2, CA_PUBLIC, A_LTPORT},
    {(char *)"uselock", 1, CA_PUBLIC, A_LUSE},
    {(char *)"userlock", 4, CA_PUBLIC, A_LUSER},
    {(char *)"speechlock", 1, CA_PUBLIC, A_LSPEECH},
    {NULL, 0, 0, 0}};

NAMETAB look_sw[] = {
    {(char *)"outside", 1, CA_PUBLIC, LOOK_OUTSIDE},
    {NULL, 0, 0, 0}};

NAMETAB mark_sw[] = {
    {(char *)"set", 1, CA_PUBLIC, MARK_SET},
    {(char *)"clear", 1, CA_PUBLIC, MARK_CLEAR},
    {NULL, 0, 0, 0}};

NAMETAB markall_sw[] = {
    {(char *)"set", 1, CA_PUBLIC, MARK_SET},
    {(char *)"clear", 1, CA_PUBLIC, MARK_CLEAR},
    {NULL, 0, 0, 0}};

NAMETAB motd_sw[] = {
    {(char *)"brief", 1, CA_WIZARD, MOTD_BRIEF | SW_MULTIPLE},
    {(char *)"connect", 1, CA_WIZARD, MOTD_ALL},
    {(char *)"down", 1, CA_WIZARD, MOTD_DOWN},
    {(char *)"full", 1, CA_WIZARD, MOTD_FULL},
    {(char *)"list", 1, CA_PUBLIC, MOTD_LIST},
    {(char *)"wizard", 1, CA_WIZARD, MOTD_WIZ},
    {NULL, 0, 0, 0}};

NAMETAB notify_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, NFY_NFYALL},
    {(char *)"first", 1, CA_PUBLIC, NFY_NFY},
    {NULL, 0, 0, 0}};

NAMETAB oemit_sw[] = {
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"speech", 1, CA_PUBLIC, PEMIT_SPEECH | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB open_sw[] = {
    {(char *)"inventory", 1, CA_PUBLIC, OPEN_INVENTORY},
    {(char *)"location", 1, CA_PUBLIC, OPEN_LOCATION},
    {NULL, 0, 0, 0}};

NAMETAB pemit_sw[] = {
    {(char *)"contents", 1, CA_PUBLIC, PEMIT_CONTENTS | SW_MULTIPLE},
    {(char *)"object", 1, CA_PUBLIC, 0},
    {(char *)"silent", 2, CA_PUBLIC, 0},
    {(char *)"speech", 2, CA_PUBLIC, PEMIT_SPEECH | SW_MULTIPLE},
    {(char *)"list", 1, CA_PUBLIC, PEMIT_LIST | SW_MULTIPLE},
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"html", 1, CA_PUBLIC, PEMIT_HTML | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB pose_sw[] = {
    {(char *)"default", 1, CA_PUBLIC, 0},
    {(char *)"noeval", 3, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"nospace", 1, CA_PUBLIC, SAY_NOSPACE},
    {NULL, 0, 0, 0}};

NAMETAB ps_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, PS_ALL | SW_MULTIPLE},
    {(char *)"brief", 1, CA_PUBLIC, PS_BRIEF},
    {(char *)"long", 1, CA_PUBLIC, PS_LONG},
    {(char *)"summary", 1, CA_PUBLIC, PS_SUMM},
    {NULL, 0, 0, 0}};

NAMETAB quota_sw[] = {
    {(char *)"all", 1, CA_GOD, QUOTA_ALL | SW_MULTIPLE},
    {(char *)"fix", 1, CA_WIZARD, QUOTA_FIX},
    {(char *)"remaining", 1, CA_WIZARD, QUOTA_REM | SW_MULTIPLE},
    {(char *)"set", 1, CA_WIZARD, QUOTA_SET},
    {(char *)"total", 1, CA_WIZARD, QUOTA_TOT | SW_MULTIPLE},
    {(char *)"room", 1, CA_WIZARD, QUOTA_ROOM | SW_MULTIPLE},
    {(char *)"exit", 1, CA_WIZARD, QUOTA_EXIT | SW_MULTIPLE},
    {(char *)"thing", 1, CA_WIZARD, QUOTA_THING | SW_MULTIPLE},
    {(char *)"player", 1, CA_WIZARD, QUOTA_PLAYER | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB reference_sw[] = {
    {(char *)"list", 1, CA_PUBLIC, NREF_LIST},
    {NULL, 0, 0, 0}};

NAMETAB set_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, SET_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB shutdown_sw[] = {
    {(char *)"abort", 1, CA_WIZARD, SHUTDN_COREDUMP},
    {NULL, 0, 0, 0}};

NAMETAB stats_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, STAT_ALL},
    {(char *)"me", 1, CA_PUBLIC, STAT_ME},
    {(char *)"player", 1, CA_PUBLIC, STAT_PLAYER},
    {NULL, 0, 0, 0}};

NAMETAB sweep_sw[] = {
    {(char *)"commands", 3, CA_PUBLIC, SWEEP_COMMANDS | SW_MULTIPLE},
    {(char *)"connected", 3, CA_PUBLIC, SWEEP_CONNECT | SW_MULTIPLE},
    {(char *)"exits", 1, CA_PUBLIC, SWEEP_EXITS | SW_MULTIPLE},
    {(char *)"here", 1, CA_PUBLIC, SWEEP_HERE | SW_MULTIPLE},
    {(char *)"inventory", 1, CA_PUBLIC, SWEEP_ME | SW_MULTIPLE},
    {(char *)"listeners", 1, CA_PUBLIC, SWEEP_LISTEN | SW_MULTIPLE},
    {(char *)"players", 1, CA_PUBLIC, SWEEP_PLAYER | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB switch_sw[] = {
    {(char *)"all", 1, CA_PUBLIC, SWITCH_ANY},
    {(char *)"default", 1, CA_PUBLIC, SWITCH_DEFAULT},
    {(char *)"first", 1, CA_PUBLIC, SWITCH_ONE},
    {(char *)"now", 1, CA_PUBLIC, SWITCH_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB teleport_sw[] = {
    {(char *)"loud", 1, CA_PUBLIC, TELEPORT_DEFAULT},
    {(char *)"quiet", 1, CA_PUBLIC, TELEPORT_QUIET},
    {NULL, 0, 0, 0}};

NAMETAB timecheck_sw[] = {
    {(char *)"log", 1, CA_WIZARD, TIMECHK_LOG | SW_MULTIPLE},
    {(char *)"reset", 1, CA_WIZARD, TIMECHK_RESET | SW_MULTIPLE},
    {(char *)"screen", 1, CA_WIZARD, TIMECHK_SCREEN | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB toad_sw[] = {
    {(char *)"no_chown", 1, CA_WIZARD, TOAD_NO_CHOWN | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB trig_sw[] = {
    {(char *)"quiet", 1, CA_PUBLIC, TRIG_QUIET},
    {(char *)"now", 1, CA_PUBLIC, TRIG_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB verb_sw[] = {
    {(char *)"known", 1, CA_PUBLIC, VERB_PRESENT | SW_MULTIPLE},
    {(char *)"move", 1, CA_PUBLIC, VERB_MOVE | SW_MULTIPLE},
    {(char *)"now", 3, CA_PUBLIC, VERB_NOW | SW_MULTIPLE},
    {(char *)"no_name", 3, CA_PUBLIC, VERB_NONAME | SW_MULTIPLE},
    {(char *)"speech", 1, CA_PUBLIC, VERB_SPEECH | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB wall_sw[] = {
    {(char *)"emit", 1, CA_PUBLIC, SAY_WALLEMIT},
    {(char *)"no_prefix", 1, CA_PUBLIC, SAY_NOTAG | SW_MULTIPLE},
    {(char *)"pose", 1, CA_PUBLIC, SAY_WALLPOSE},
    {(char *)"wizard", 1, CA_PUBLIC, SAY_WIZSHOUT | SW_MULTIPLE},
    {(char *)"admin", 1, CA_ADMIN, SAY_ADMINSHOUT},
    {NULL, 0, 0, 0}};

NAMETAB warp_sw[] = {
    {(char *)"check", 1, CA_WIZARD, TWARP_CLEAN | SW_MULTIPLE},
    {(char *)"dump", 1, CA_WIZARD, TWARP_DUMP | SW_MULTIPLE},
    {(char *)"idle", 1, CA_WIZARD, TWARP_IDLE | SW_MULTIPLE},
    {(char *)"queue", 1, CA_WIZARD, TWARP_QUEUE | SW_MULTIPLE},
    {(char *)"events", 1, CA_WIZARD, TWARP_EVENTS | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB wait_sw[] = {
    {(char *)"pid", 1, CA_PUBLIC, WAIT_PID | SW_MULTIPLE},
    {(char *)"until", 1, CA_PUBLIC, WAIT_UNTIL | SW_MULTIPLE},
    {NULL, 0, 0, 0}};

NAMETAB noeval_sw[] = {
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL},
    {NULL, 0, 0, 0}};

/**
 * @brief Command, function, etc. access name table.
 * 
 */
NAMETAB access_nametab[] = {
    {(char *)"admin", 2, CA_WIZARD, CA_ADMIN},
    {(char *)"builder", 6, CA_WIZARD, CA_BUILDER},
    {(char *)"dark", 4, CA_GOD, CF_DARK},
    {(char *)"disabled", 4, CA_GOD, CA_DISABLED},
    {(char *)"global_build", 8, CA_PUBLIC, CA_GBL_BUILD},
    {(char *)"global_interp", 8, CA_PUBLIC, CA_GBL_INTERP},
    {(char *)"god", 2, CA_GOD, CA_GOD},
    {(char *)"head", 2, CA_WIZARD, CA_HEAD},
    {(char *)"immortal", 3, CA_WIZARD, CA_IMMORTAL},
    {(char *)"marker0", 7, CA_WIZARD, CA_MARKER0},
    {(char *)"marker1", 7, CA_WIZARD, CA_MARKER1},
    {(char *)"marker2", 7, CA_WIZARD, CA_MARKER2},
    {(char *)"marker3", 7, CA_WIZARD, CA_MARKER3},
    {(char *)"marker4", 7, CA_WIZARD, CA_MARKER4},
    {(char *)"marker5", 7, CA_WIZARD, CA_MARKER5},
    {(char *)"marker6", 7, CA_WIZARD, CA_MARKER6},
    {(char *)"marker7", 7, CA_WIZARD, CA_MARKER7},
    {(char *)"marker8", 7, CA_WIZARD, CA_MARKER8},
    {(char *)"marker9", 7, CA_WIZARD, CA_MARKER9},
    {(char *)"need_location", 6, CA_PUBLIC, CA_LOCATION},
    {(char *)"need_contents", 6, CA_PUBLIC, CA_CONTENTS},
    {(char *)"need_player", 6, CA_PUBLIC, CA_PLAYER},
    {(char *)"no_haven", 4, CA_PUBLIC, CA_NO_HAVEN},
    {(char *)"no_robot", 4, CA_WIZARD, CA_NO_ROBOT},
    {(char *)"no_slave", 5, CA_PUBLIC, CA_NO_SLAVE},
    {(char *)"no_suspect", 5, CA_WIZARD, CA_NO_SUSPECT},
    {(char *)"no_guest", 5, CA_WIZARD, CA_NO_GUEST},
    {(char *)"staff", 3, CA_WIZARD, CA_STAFF},
    {(char *)"static", 3, CA_GOD, CA_STATIC},
    {(char *)"wizard", 3, CA_WIZARD, CA_WIZARD},
    {NULL, 0, 0, 0}};

/**
 * @brief Attribute access name tables.
 * 
 */
NAMETAB attraccess_nametab[] = {
    {(char *)"const", 2, CA_PUBLIC, AF_CONST},
    {(char *)"dark", 2, CA_WIZARD, AF_DARK},
    {(char *)"default", 3, CA_WIZARD, AF_DEFAULT},
    {(char *)"deleted", 3, CA_WIZARD, AF_DELETED},
    {(char *)"god", 1, CA_PUBLIC, AF_GOD},
    {(char *)"hidden", 1, CA_WIZARD, AF_MDARK},
    {(char *)"ignore", 2, CA_WIZARD, AF_NOCMD},
    {(char *)"internal", 2, CA_WIZARD, AF_INTERNAL},
    {(char *)"is_lock", 4, CA_PUBLIC, AF_IS_LOCK},
    {(char *)"locked", 1, CA_PUBLIC, AF_LOCK},
    {(char *)"no_clone", 5, CA_PUBLIC, AF_NOCLONE},
    {(char *)"no_command", 5, CA_PUBLIC, AF_NOPROG},
    {(char *)"no_inherit", 4, CA_PUBLIC, AF_PRIVATE},
    {(char *)"visual", 1, CA_PUBLIC, AF_VISUAL},
    {(char *)"wizard", 1, CA_PUBLIC, AF_WIZARD},
    {NULL, 0, 0, 0}};

NAMETAB indiv_attraccess_nametab[] = {
    {(char *)"case", 1, CA_PUBLIC, AF_CASE},
    {(char *)"hidden", 1, CA_WIZARD, AF_MDARK},
    {(char *)"wizard", 1, CA_WIZARD, AF_WIZARD},
    {(char *)"no_command", 4, CA_PUBLIC, AF_NOPROG},
    {(char *)"no_inherit", 4, CA_PUBLIC, AF_PRIVATE},
    {(char *)"no_name", 4, CA_PUBLIC, AF_NONAME},
    {(char *)"no_parse", 4, CA_PUBLIC, AF_NOPARSE},
    {(char *)"now", 3, CA_PUBLIC, AF_NOW},
    {(char *)"regexp", 2, CA_PUBLIC, AF_REGEXP},
    {(char *)"rmatch", 2, CA_PUBLIC, AF_RMATCH},
    {(char *)"structure", 1, CA_GOD, AF_STRUCTURE},
    {(char *)"trace", 1, CA_PUBLIC, AF_TRACE},
    {(char *)"visual", 1, CA_PUBLIC, AF_VISUAL},
    {(char *)"html", 2, CA_PUBLIC, AF_HTML},
    {NULL, 0, 0, 0}};

/**
 * @brief All available lists for the list command.
 * 
 */
NAMETAB list_names[] = {
    {(char *)"allocations", 2, CA_WIZARD, LIST_ALLOCATOR},
    {(char *)"attr_permissions", 6, CA_WIZARD, LIST_ATTRPERMS},
    {(char *)"attr_types", 6, CA_PUBLIC, LIST_ATTRTYPES},
    {(char *)"attributes", 2, CA_PUBLIC, LIST_ATTRIBUTES},
    {(char *)"bad_names", 2, CA_WIZARD, LIST_BADNAMES},
    {(char *)"buffers", 2, CA_WIZARD, LIST_BUFTRACE},
    {(char *)"cache", 2, CA_WIZARD, LIST_CACHEOBJS},
    {(char *)"cache_attrs", 6, CA_WIZARD, LIST_CACHEATTRS},
    {(char *)"commands", 3, CA_PUBLIC, LIST_COMMANDS},
    {(char *)"config_permissions", 8, CA_GOD, LIST_CONF_PERMS},
    {(char *)"config_read_perms", 4, CA_PUBLIC, LIST_CF_RPERMS},
    {(char *)"costs", 3, CA_PUBLIC, LIST_COSTS},
    {(char *)"db_stats", 2, CA_WIZARD, LIST_DB_STATS},
    {(char *)"default_flags", 1, CA_PUBLIC, LIST_DF_FLAGS},
    {(char *)"flags", 2, CA_PUBLIC, LIST_FLAGS},
    {(char *)"func_permissions", 5, CA_WIZARD, LIST_FUNCPERMS},
    {(char *)"functions", 2, CA_PUBLIC, LIST_FUNCTIONS},
    {(char *)"globals", 1, CA_WIZARD, LIST_GLOBALS},
    {(char *)"hashstats", 1, CA_WIZARD, LIST_HASHSTATS},
    {(char *)"logging", 1, CA_GOD, LIST_LOGGING},
    {(char *)"memory", 1, CA_WIZARD, LIST_MEMORY},
    {(char *)"options", 1, CA_PUBLIC, LIST_OPTIONS},
    {(char *)"params", 2, CA_PUBLIC, LIST_PARAMS},
    {(char *)"permissions", 2, CA_WIZARD, LIST_PERMS},
    {(char *)"powers", 2, CA_WIZARD, LIST_POWERS},
    {(char *)"process", 2, CA_WIZARD, LIST_PROCESS},
    {(char *)"raw_memory", 1, CA_WIZARD, LIST_RAWMEM},
    {(char *)"site_information", 2, CA_WIZARD, LIST_SITEINFO},
    {(char *)"switches", 2, CA_PUBLIC, LIST_SWITCHES},
    {(char *)"textfiles", 1, CA_WIZARD, LIST_TEXTFILES},
    {(char *)"user_attributes", 1, CA_WIZARD, LIST_VATTRS},
    {NULL, 0, 0, 0}};

/**
 * @brief Boolean nametable
 * 
 */
NAMETAB bool_names[] = {
    {(char *)"true", 1, 0, 1},
    {(char *)"false", 1, 0, 0},
    {(char *)"yes", 1, 0, 1},
    {(char *)"no", 1, 0, 0},
    {(char *)"1", 1, 0, 1},
    {(char *)"0", 1, 0, 0},
    {NULL, 0, 0, 0}};

/**
 * @brief File nametable
 * 
 */
NAMETAB list_files[] = {
    {(char *)"badsite_connect", 1, CA_WIZARD, FC_CONN_SITE},
    {(char *)"connect", 2, CA_WIZARD, FC_CONN},
    {(char *)"create_register", 2, CA_WIZARD, FC_CREA_REG},
    {(char *)"down", 1, CA_WIZARD, FC_CONN_DOWN},
    {(char *)"full", 1, CA_WIZARD, FC_CONN_FULL},
    {(char *)"guest_motd", 1, CA_WIZARD, FC_CONN_GUEST},
    {(char *)"html_connect", 1, CA_WIZARD, FC_CONN_HTML},
    {(char *)"motd", 1, CA_WIZARD, FC_MOTD},
    {(char *)"newuser", 1, CA_WIZARD, FC_CREA_NEW},
    {(char *)"quit", 1, CA_WIZARD, FC_QUIT},
    {(char *)"register_connect", 1, CA_WIZARD, FC_CONN_REG},
    {(char *)"wizard_motd", 1, CA_WIZARD, FC_WIZMOTD},
    {NULL, 0, 0, 0}};

/**
 * @brief Logging nametables
 * 
 */
NAMETAB logdata_nametab[] = {
    {(char *)"flags", 1, 0, LOGOPT_FLAGS},
    {(char *)"location", 1, 0, LOGOPT_LOC},
    {(char *)"owner", 1, 0, LOGOPT_OWNER},
    {(char *)"timestamp", 1, 0, LOGOPT_TIMESTAMP},
    {NULL, 0, 0, 0}};

NAMETAB logoptions_nametab[] = {
    {(char *)"accounting", 2, 0, LOG_ACCOUNTING},
    {(char *)"all_commands", 2, 0, LOG_ALLCOMMANDS},
    {(char *)"bad_commands", 2, 0, LOG_BADCOMMANDS},
    {(char *)"buffer_alloc", 3, 0, LOG_ALLOCATE},
    {(char *)"bugs", 3, 0, LOG_BUGS},
    {(char *)"checkpoints", 2, 0, LOG_DBSAVES},
    {(char *)"config_changes", 2, 0, LOG_CONFIGMODS},
    {(char *)"create", 2, 0, LOG_PCREATES},
    {(char *)"keyboard_commands", 2, 0, LOG_KBCOMMANDS},
    {(char *)"killing", 1, 0, LOG_KILLS},
    {(char *)"local", 3, 0, LOG_LOCAL},
    {(char *)"logins", 3, 0, LOG_LOGIN},
    {(char *)"network", 1, 0, LOG_NET},
    {(char *)"problems", 1, 0, LOG_PROBLEMS},
    {(char *)"security", 2, 0, LOG_SECURITY},
    {(char *)"shouts", 2, 0, LOG_SHOUTS},
    {(char *)"startup", 2, 0, LOG_STARTUP},
    {(char *)"suspect_commands", 2, 0, LOG_SUSPECTCMDS},
    {(char *)"time_usage", 1, 0, LOG_TIMEUSE},
    {(char *)"wizard", 1, 0, LOG_WIZARD},
    {(char *)"malloc", 1, 0, LOG_MALLOC},
    {NULL, 0, 0, 0}};

/**
 * @brief Global control flags nametable
 * 
 */
NAMETAB enable_names[] = {
    {(char *)"building", 1, CA_PUBLIC, CF_BUILD},
    {(char *)"checkpointing", 2, CA_PUBLIC, CF_CHECKPOINT},
    {(char *)"cleaning", 2, CA_PUBLIC, CF_DBCHECK},
    {(char *)"dequeueing", 1, CA_PUBLIC, CF_DEQUEUE},
    {(char *)"god_monitoring", 1, CA_PUBLIC, CF_GODMONITOR},
    {(char *)"idlechecking", 2, CA_PUBLIC, CF_IDLECHECK},
    {(char *)"interpret", 2, CA_PUBLIC, CF_INTERP},
    {(char *)"logins", 3, CA_PUBLIC, CF_LOGIN},
    {(char *)"eventchecking", 2, CA_PUBLIC, CF_EVENTCHECK},
    {NULL, 0, 0, 0}};

/**
 * @brief Signal actions nametable
 * 
 */
NAMETAB sigactions_nametab[] = {
    {(char *)"exit", 3, 0, SA_EXIT},
    {(char *)"default", 1, 0, SA_DFLT},
    {NULL, 0, 0, 0}};

/**
 * @brief Logged out command tablename
 * 
 */
NAMETAB logout_cmdtable[] = {
	{(char *)"DOING", 5, CA_PUBLIC, CMD_DOING},
	{(char *)"LOGOUT", 6, CA_PUBLIC, CMD_LOGOUT},
	{(char *)"OUTPUTPREFIX", 12, CA_PUBLIC, CMD_PREFIX | CMD_NOxFIX},
	{(char *)"OUTPUTSUFFIX", 12, CA_PUBLIC, CMD_SUFFIX | CMD_NOxFIX},
	{(char *)"QUIT", 4, CA_PUBLIC, CMD_QUIT},
	{(char *)"SESSION", 7, CA_PUBLIC, CMD_SESSION},
	{(char *)"WHO", 3, CA_PUBLIC, CMD_WHO},
	{(char *)"PUEBLOCLIENT", 12, CA_PUBLIC, CMD_PUEBLOCLIENT},
	{(char *)"INFO", 4, CA_PUBLIC, CMD_INFO},
	{NULL, 0, 0, 0}};

/**
 * @brief Command table: Definitions for builtin commands, used to build the command hash table.
 * 
 */
CMDENT command_table[] = {
    {(char *)"@@", NULL, CA_PUBLIC, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_comment}},
    {(char *)"@addcommand", addcmd_sw, CA_GOD, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_addcommand}},
    {(char *)"@admin", NULL, CA_WIZARD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_admin}},
    {(char *)"@alias", NULL, CA_NO_GUEST | CA_NO_SLAVE, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_alias}},
    {(char *)"@apply_marked", NULL, CA_WIZARD | CA_GBL_INTERP, 0, CS_ONE_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, NULL, NULL, NULL, {do_apply_marked}},
    {(char *)"@attribute", attrib_sw, CA_WIZARD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_attribute}},
    {(char *)"@boot", boot_sw, CA_NO_GUEST | CA_NO_SLAVE, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_boot}},
    {(char *)"@chown", chown_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, CHOWN_ONE, CS_TWO_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_chown}},
    {(char *)"@chownall", chown_sw, CA_WIZARD | CA_GBL_BUILD, CHOWN_ALL, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_chownall}},
    {(char *)"@chzone", chzone_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_chzone}},
    {(char *)"@clone", clone_sw, CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_clone}},
    {(char *)"@colormap", NULL, CA_PUBLIC, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_colormap}},
    {(char *)"@cpattr", NULL, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, 0, CS_TWO_ARG | CS_ARGV, NULL, NULL, NULL, {do_cpattr}},
    {(char *)"@create", NULL, CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_create}},
    {(char *)"@cron", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_cron}},
    {(char *)"@crondel", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_crondel}},
    {(char *)"@crontab", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_crontab}},
    {(char *)"@cut", NULL, CA_WIZARD | CA_LOCATION, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_cut}},
    {(char *)"@dbck", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_dbck}},
    {(char *)"@backup", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_backup_mush}},
    {(char *)"@decompile", decomp_sw, CA_PUBLIC, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_decomp}},
    {(char *)"@delcommand", NULL, CA_GOD, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_delcommand}},
    {(char *)"@destroy", destroy_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, DEST_ONE, CS_ONE_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_destroy}},
    {(char *)"@dig", dig_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, 0, CS_TWO_ARG | CS_ARGV | CS_INTERP, NULL, NULL, NULL, {do_dig}},
    {(char *)"@disable", NULL, CA_WIZARD, GLOB_DISABLE, CS_ONE_ARG, NULL, NULL, NULL, {do_global}},
    {(char *)"@doing", doing_sw, CA_PUBLIC, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_doing}},
    {(char *)"@dolist", dolist_sw, CA_GBL_INTERP, 0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, NULL, NULL, NULL, {do_dolist}},
    {(char *)"@drain", NULL, CA_GBL_INTERP | CA_NO_SLAVE | CA_NO_GUEST, NFY_DRAIN, CS_TWO_ARG, NULL, NULL, NULL, {do_notify}},
    {(char *)"@dump", dump_sw, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_dump}},
    {(char *)"@edit", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG | CS_ARGV | CS_STRIP_AROUND, NULL, NULL, NULL, {do_edit}},
    {(char *)"@emit", emit_sw, CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE, SAY_EMIT, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_say}},
    {(char *)"@enable", NULL, CA_WIZARD, GLOB_ENABLE, CS_ONE_ARG, NULL, NULL, NULL, {do_global}},
    {(char *)"@end", end_sw, CA_GBL_INTERP, 0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, NULL, NULL, NULL, {do_end}},
    {(char *)"@entrances", NULL, CA_NO_GUEST, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_entrances}},
    {(char *)"@eval", NULL, CA_NO_SLAVE, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_eval}},
    {(char *)"@femit", femit_sw, CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE, PEMIT_FEMIT, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_pemit}},
    {(char *)"@find", NULL, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_find}},
    {(char *)"@fixdb", fixdb_sw, CA_GOD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_fixdb}},
    {(char *)"@floaters", floaters_sw, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_floaters}},
    {(char *)"@force", force_sw, CA_NO_SLAVE | CA_GBL_INTERP | CA_NO_GUEST, FRC_COMMAND, CS_TWO_ARG | CS_INTERP | CS_CMDARG, NULL, NULL, NULL, {do_force}},
    {(char *)"@fpose", fpose_sw, CA_LOCATION | CA_NO_SLAVE, PEMIT_FPOSE, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_pemit}},
    {(char *)"@fsay", fsay_sw, CA_LOCATION | CA_NO_SLAVE, PEMIT_FSAY, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_pemit}},
    {(char *)"@freelist", NULL, CA_WIZARD, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_freelist}},
    {(char *)"@function", function_sw, CA_GOD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_function}},
    {(char *)"@halt", halt_sw, CA_NO_SLAVE, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_halt}},
    {(char *)"@hashresize", NULL, CA_GOD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_hashresize}},
    {(char *)"@hook", hook_sw, CA_GOD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_hook}},
    {(char *)"@include", NULL, CA_GBL_INTERP, 0, CS_TWO_ARG | CS_ARGV | CS_CMDARG, NULL, NULL, NULL, {do_include}},
    {(char *)"@kick", NULL, CA_WIZARD, QUEUE_KICK, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_queue}},
    {(char *)"@last", NULL, CA_NO_GUEST, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_last}},
    {(char *)"@link", NULL, CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_link}},
    {(char *)"@list", NULL, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_list}},
    {(char *)"@listcommands", NULL, CA_GOD, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_listcommands}},
    {(char *)"@list_file", NULL, CA_WIZARD, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_list_file}},
    {(char *)"@listmotd", listmotd_sw, CA_PUBLIC, MOTD_LIST, CS_ONE_ARG, NULL, NULL, NULL, {do_motd}},
    {(char *)"@lock", lock_sw, CA_NO_SLAVE, 0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_lock}},
    {(char *)"@log", NULL, CA_WIZARD, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_logwrite}},
    {(char *)"@logrotate", NULL, CA_GOD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_logrotate}},
    {(char *)"@mark", mark_sw, CA_WIZARD, SRCH_MARK, CS_ONE_ARG | CS_NOINTERP, NULL, NULL, NULL, {do_search}},
    {(char *)"@mark_all", markall_sw, CA_WIZARD, MARK_SET, CS_NO_ARGS, NULL, NULL, NULL, {do_markall}},
    {(char *)"@motd", motd_sw, CA_WIZARD, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_motd}},
    {(char *)"@mvattr", NULL, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, 0, CS_TWO_ARG | CS_ARGV, NULL, NULL, NULL, {do_mvattr}},
    {(char *)"@name", NULL, CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_name}},
    {(char *)"@newpassword", NULL, CA_WIZARD, PASS_ANY, CS_TWO_ARG, NULL, NULL, NULL, {do_newpassword}},
    {(char *)"@notify", notify_sw, CA_GBL_INTERP | CA_NO_SLAVE | CA_NO_GUEST, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_notify}},
    {(char *)"@oemit", oemit_sw, CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE, PEMIT_OEMIT, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_pemit}},
    {(char *)"@open", open_sw, CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST, 0, CS_TWO_ARG | CS_ARGV | CS_INTERP, NULL, NULL, NULL, {do_open}},
    {(char *)"@parent", NULL, CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST, 0, CS_TWO_ARG | CS_FUNCTION, NULL, NULL, NULL, {do_parent}},
    {(char *)"@password", NULL, CA_NO_GUEST, PASS_MINE, CS_TWO_ARG, NULL, NULL, NULL, {do_password}},
    {(char *)"@pcreate", NULL, CA_WIZARD | CA_GBL_BUILD, PCRE_PLAYER, CS_TWO_ARG, NULL, NULL, NULL, {do_pcreate}},
    {(char *)"@pemit", pemit_sw, CA_NO_GUEST | CA_NO_SLAVE, PEMIT_PEMIT, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_pemit}},
    {(char *)"@npemit", pemit_sw, CA_NO_GUEST | CA_NO_SLAVE, PEMIT_PEMIT, CS_TWO_ARG | CS_UNPARSE | CS_NOSQUISH, NULL, NULL, NULL, {do_pemit}},
    {(char *)"@poor", NULL, CA_GOD, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_poor}},
    {(char *)"@power", NULL, CA_PUBLIC, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_power}},
    {(char *)"@program", NULL, CA_PUBLIC, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_prog}},
    {(char *)"@ps", ps_sw, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_ps}},
    {(char *)"@quota", quota_sw, CA_PUBLIC, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_quota}},
    {(char *)"@quitprogram", NULL, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_quitprog}},
    {(char *)"@readcache", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_readcache}},
    {(char *)"@redirect", NULL, CA_PUBLIC, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_redirect}},
    {(char *)"@reference", reference_sw, CA_PUBLIC, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_reference}},
    {(char *)"@restart", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_restart}},
    {(char *)"@robot", NULL, CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST | CA_PLAYER, PCRE_ROBOT, CS_TWO_ARG, NULL, NULL, NULL, {do_pcreate}},
    {(char *)"@search", NULL, CA_PUBLIC, SRCH_SEARCH, CS_ONE_ARG | CS_NOINTERP, NULL, NULL, NULL, {do_search}},
    {(char *)"@set", set_sw, CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST, 0, CS_TWO_ARG, NULL, NULL, NULL, {do_set}},
    {(char *)"@shutdown", shutdown_sw, CA_WIZARD, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_shutdown}},
    {(char *)"@stats", stats_sw, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_stats}},
    {(char *)"@startslave", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {boot_slave}},
    {(char *)"@sweep", sweep_sw, CA_PUBLIC, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_sweep}},
    {(char *)"@switch", switch_sw, CA_GBL_INTERP, 0, CS_TWO_ARG | CS_ARGV | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, NULL, NULL, NULL, {do_switch}},
    {(char *)"@teleport", teleport_sw, CA_NO_GUEST, TELEPORT_DEFAULT, CS_TWO_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_teleport}},
    {(char *)"@timecheck", timecheck_sw, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_timecheck}},
    {(char *)"@timewarp", warp_sw, CA_WIZARD, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_timewarp}},
    {(char *)"@toad", toad_sw, CA_WIZARD, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_toad}},
    {(char *)"@trigger", trig_sw, CA_GBL_INTERP, 0, CS_TWO_ARG | CS_ARGV, NULL, NULL, NULL, {do_trigger}},
    {(char *)"@unlink", NULL, CA_NO_SLAVE | CA_GBL_BUILD, 0, CS_ONE_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_unlink}},
    {(char *)"@unlock", lock_sw, CA_NO_SLAVE, 0, CS_ONE_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_unlock}},
    {(char *)"@verb", verb_sw, CA_GBL_INTERP | CA_NO_SLAVE, 0, CS_TWO_ARG | CS_ARGV | CS_INTERP | CS_STRIP_AROUND, NULL, NULL, NULL, {do_verb}},
    {(char *)"@wait", wait_sw, CA_GBL_INTERP, 0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, NULL, NULL, NULL, {do_wait}},
    {(char *)"@wall", wall_sw, CA_PUBLIC, SAY_SHOUT, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_say}},
    {(char *)"@wipe", NULL, CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD, 0, CS_ONE_ARG | CS_INTERP | CS_FUNCTION, NULL, NULL, NULL, {do_wipe}},
    {(char *)"drop", drop_sw, CA_NO_SLAVE | CA_CONTENTS | CA_LOCATION | CA_NO_GUEST, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_drop}},
    {(char *)"enter", enter_sw, CA_LOCATION, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_enter}},
    {(char *)"examine", examine_sw, CA_PUBLIC, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_examine}},
    {(char *)"get", get_sw, CA_LOCATION | CA_NO_GUEST, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_get}},
    {(char *)"give", give_sw, CA_LOCATION | CA_NO_GUEST, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_give}},
    {(char *)"goto", goto_sw, CA_LOCATION, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_move}},
    {(char *)"internalgoto", NULL, CA_GOD, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_comment}},
    {(char *)"inventory", NULL, CA_PUBLIC, LOOK_INVENTORY, CS_NO_ARGS, NULL, NULL, NULL, {do_inventory}},
    {(char *)"kill", NULL, CA_NO_GUEST | CA_NO_SLAVE, KILL_KILL, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_kill}},
    {(char *)"leave", leave_sw, CA_LOCATION, 0, CS_NO_ARGS | CS_INTERP, NULL, NULL, NULL, {do_leave}},
    {(char *)"look", look_sw, CA_LOCATION, LOOK_LOOK, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_look}},
    {(char *)"page", noeval_sw, CA_NO_SLAVE, 0, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_page}},
    {(char *)"pose", pose_sw, CA_LOCATION | CA_NO_SLAVE, SAY_POSE, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_say}},
    {(char *)"reply", noeval_sw, CA_NO_SLAVE, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_reply_page}},
    {(char *)"say", noeval_sw, CA_LOCATION | CA_NO_SLAVE, SAY_SAY, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_say}},
    {(char *)"score", NULL, CA_PUBLIC, LOOK_SCORE, CS_NO_ARGS, NULL, NULL, NULL, {do_score}},
    {(char *)"slay", NULL, CA_WIZARD, KILL_SLAY, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_kill}},
    {(char *)"think", NULL, CA_NO_SLAVE, 0, CS_ONE_ARG, NULL, NULL, NULL, {do_think}},
    {(char *)"use", NULL, CA_NO_SLAVE | CA_GBL_INTERP, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, {do_use}},
    {(char *)"version", NULL, CA_PUBLIC, 0, CS_NO_ARGS, NULL, NULL, NULL, {do_version}},
    {(char *)"whisper", NULL, CA_LOCATION | CA_NO_SLAVE, PEMIT_WHISPER, CS_TWO_ARG | CS_INTERP, NULL, NULL, NULL, {do_pemit}},
    {(char *)"doing", NULL, CA_PUBLIC, CMD_DOING, CS_ONE_ARG, NULL, NULL, NULL, {logged_out}},
    {(char *)"quit", NULL, CA_PUBLIC, CMD_QUIT, CS_NO_ARGS, NULL, NULL, NULL, {logged_out}},
    {(char *)"logout", NULL, CA_PUBLIC, CMD_LOGOUT, CS_NO_ARGS, NULL, NULL, NULL, {logged_out}},
    {(char *)"who", NULL, CA_PUBLIC, CMD_WHO, CS_ONE_ARG, NULL, NULL, NULL, {logged_out}},
    {(char *)"session", NULL, CA_PUBLIC, CMD_SESSION, CS_ONE_ARG, NULL, NULL, NULL, {logged_out}},
    {(char *)"info", NULL, CA_PUBLIC, CMD_INFO, CS_NO_ARGS, NULL, NULL, NULL, {logged_out}},
    {(char *)"outputprefix", NULL, CA_PUBLIC, CMD_PREFIX, CS_ONE_ARG, NULL, NULL, NULL, {logged_out}},
    {(char *)"outputsuffix", NULL, CA_PUBLIC, CMD_SUFFIX, CS_ONE_ARG, NULL, NULL, NULL, {logged_out}},
    {(char *)"puebloclient", NULL, CA_PUBLIC, CMD_PUEBLOCLIENT, CS_ONE_ARG, NULL, NULL, NULL, {logged_out}},
    {(char *)"\\", NULL, CA_NO_GUEST | CA_LOCATION | CF_DARK | CA_NO_SLAVE, SAY_PREFIX | SAY_EMIT, CS_ONE_ARG | CS_INTERP | CS_LEADIN, NULL, NULL, NULL, {do_say}},
    {(char *)"#", NULL, CA_NO_SLAVE | CA_GBL_INTERP | CF_DARK, 0, CS_ONE_ARG | CS_INTERP | CS_CMDARG | CS_LEADIN, NULL, NULL, NULL, {do_force_prefixed}},
    {(char *)":", NULL, CA_LOCATION | CF_DARK | CA_NO_SLAVE, SAY_PREFIX | SAY_POSE, CS_ONE_ARG | CS_INTERP | CS_LEADIN, NULL, NULL, NULL, {do_say}},
    {(char *)";", NULL, CA_LOCATION | CF_DARK | CA_NO_SLAVE, SAY_PREFIX | SAY_POSE_NOSPC, CS_ONE_ARG | CS_INTERP | CS_LEADIN, NULL, NULL, NULL, {do_say}},
    {(char *)"\"", NULL, CA_LOCATION | CF_DARK | CA_NO_SLAVE, SAY_PREFIX | SAY_SAY, CS_ONE_ARG | CS_INTERP | CS_LEADIN, NULL, NULL, NULL, {do_say}},
    {(char *)"&", NULL, CA_NO_GUEST | CA_NO_SLAVE | CF_DARK, 0, CS_TWO_ARG | CS_LEADIN, NULL, NULL, NULL, {do_setvattr}},
    {(char *)NULL, NULL, 0, 0, 0, NULL, NULL, NULL, {NULL}}};

/**
 * @brief Table for parsing the configuration file.
 * 
 */
CONF conftable[] = {
    {(char *)"access", cf_access, CA_GOD, CA_DISABLED, NULL, (long)access_nametab},
    {(char *)"addcommands_match_blindly", cf_bool, CA_GOD, CA_WIZARD, &mudconf.addcmd_match_blindly, (long)"@addcommands don't error if no match is found"},
    {(char *)"addcommands_obey_stop", cf_bool, CA_GOD, CA_WIZARD, &mudconf.addcmd_obey_stop, (long)"@addcommands obey STOP"},
    {(char *)"addcommands_obey_uselocks", cf_bool, CA_GOD, CA_WIZARD, &mudconf.addcmd_obey_uselocks, (long)"@addcommands obey UseLocks"},
    {(char *)"alias", cf_cmd_alias, CA_GOD, CA_DISABLED, (int *)&mudstate.command_htab, 0},
    {(char *)"ansi_colors", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.ansi_colors, (long)"ANSI color codes enabled"},
    {(char *)"attr_access", cf_attr_access, CA_GOD, CA_DISABLED, NULL, (long)attraccess_nametab},
    {(char *)"attr_alias", cf_alias, CA_GOD, CA_DISABLED, (int *)&mudstate.attr_name_htab, (long)"Attribute"},
    {(char *)"attr_cmd_access", cf_acmd_access, CA_GOD, CA_DISABLED, NULL, (long)access_nametab},
    {(char *)"attr_type", cf_attr_type, CA_GOD, CA_DISABLED, NULL, (long)attraccess_nametab},
    {(char *)"autozone", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.autozone, (long)"New objects are @chzoned to their creator's zone"},
    {(char *)"bad_name", cf_badname, CA_GOD, CA_DISABLED, NULL, 0},
    {(char *)"badsite_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.site_file, MBUF_SIZE},
    {(char *)"backup_compress", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.backup_compress, MBUF_SIZE},
    {(char *)"backup_extension", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.backup_ext, MBUF_SIZE},
    {(char *)"backup_extract", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.backup_extract, MBUF_SIZE},
    {(char *)"backup_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.bakhome, MBUF_SIZE},
    {(char *)"backup_util", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.backup_exec, MBUF_SIZE},
    {(char *)"binary_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.binhome, MBUF_SIZE},
    {(char *)"booleans_oldstyle", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.bools_oldstyle, (long)"Dbrefs #0 and #-1 are boolean false, all other\n\t\t\t\tdbrefs are boolean true"},
    {(char *)"building_limit", cf_int, CA_GOD, CA_PUBLIC, (int *)&mudconf.building_limit, 0},
    {(char *)"c_is_command", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.c_cmd_subst, (long)"%c substitution is last command rather than ANSI"},
    {(char *)"cache_size", cf_int, CA_GOD, CA_GOD, &mudconf.cache_size, 0},
    {(char *)"cache_width", cf_int, CA_STATIC, CA_GOD, &mudconf.cache_width, 0},
    {(char *)"check_interval", cf_int, CA_GOD, CA_WIZARD, &mudconf.check_interval, 0},
    {(char *)"check_offset", cf_int, CA_GOD, CA_WIZARD, &mudconf.check_offset, 0},
    {(char *)"clone_copies_cost", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.clone_copy_cost, (long)"@clone copies object cost"},
    {(char *)"command_invocation_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.cmd_invk_lim, 0},
    {(char *)"command_quota_increment", cf_int, CA_GOD, CA_WIZARD, &mudconf.cmd_quota_incr, 0},
    {(char *)"command_quota_max", cf_int, CA_GOD, CA_WIZARD, &mudconf.cmd_quota_max, 0},
    {(char *)"command_recursion_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.cmd_nest_lim, 0},
    {(char *)"concentrator_port", cf_int, CA_STATIC, CA_WIZARD, &mudconf.conc_port, 0},
    {(char *)"config_access", cf_cf_access, CA_GOD, CA_DISABLED, NULL, (long)access_nametab},
    {(char *)"config_read_access", cf_cf_access, CA_GOD, CA_DISABLED, (int *)1, (long)access_nametab},
    {(char *)"conn_timeout", cf_int, CA_GOD, CA_WIZARD, &mudconf.conn_timeout, 0},
    {(char *)"connect_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.conn_file, MBUF_SIZE},
    {(char *)"connect_reg_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.creg_file, MBUF_SIZE},
    {(char *)"create_max_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.createmax, 0},
    {(char *)"create_min_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.createmin, 0},
    {(char *)"dark_actions", cf_bool, CA_GOD, CA_WIZARD, &mudconf.dark_actions, (long)"Dark objects still trigger @a-actions when moving"},
    {(char *)"dark_sleepers", cf_bool, CA_GOD, CA_WIZARD, &mudconf.dark_sleepers, (long)"Disconnected players not shown in room contents"},
    {(char *)"database_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.dbhome, MBUF_SIZE},
    {(char *)"default_home", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.default_home, NOTHING},
    {(char *)"dbref_flag_sep", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mudconf.flag_sep, 1},
    {(char *)"dig_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.digcost, 0},
    {(char *)"divert_log", cf_divert_log, CA_STATIC, CA_DISABLED, &mudconf.log_diversion, (long)logoptions_nametab},
    {(char *)"down_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.down_file, MBUF_SIZE},
    {(char *)"down_motd_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.downmotd_msg, GBUF_SIZE},
    {(char *)"dump_interval", cf_int, CA_GOD, CA_WIZARD, &mudconf.dump_interval, 0},
    {(char *)"dump_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.dump_msg, MBUF_SIZE},
    {(char *)"postdump_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.postdump_msg, MBUF_SIZE},
    {(char *)"dump_offset", cf_int, CA_GOD, CA_WIZARD, &mudconf.dump_offset, 0},
    {(char *)"earn_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.paylimit, 0},
    {(char *)"examine_flags", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.ex_flags, (long)"examine shows names of flags"},
    {(char *)"examine_public_attrs", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.exam_public, (long)"examine shows public attributes"},
    {(char *)"exit_flags", cf_set_flags, CA_GOD, CA_DISABLED, (int *)&mudconf.exit_flags, 0},
    {(char *)"exit_calls_move", cf_bool, CA_GOD, CA_WIZARD, &mudconf.exit_calls_move, (long)"Using an exit calls the move command"},
    {(char *)"exit_parent", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.exit_parent, NOTHING},
    {(char *)"exit_proto", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.exit_proto, NOTHING},
    {(char *)"exit_attr_defaults", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.exit_defobj, NOTHING},
    {(char *)"exit_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.exit_quota, 0},
    {(char *)"events_daily_hour", cf_int, CA_GOD, CA_PUBLIC, &mudconf.events_daily_hour, 0},
    {(char *)"fascist_teleport", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.fascist_tport, (long)"@teleport source restricted to control or JUMP_OK"},
    {(char *)"fixed_home_message", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mudconf.fixed_home_msg, MBUF_SIZE},
    {(char *)"fixed_tel_message", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mudconf.fixed_tel_msg, MBUF_SIZE},
    {(char *)"find_money_chance", cf_int, CA_GOD, CA_WIZARD, &mudconf.payfind, 0},
    {(char *)"flag_alias", cf_alias, CA_GOD, CA_DISABLED, (int *)&mudstate.flags_htab, (long)"Flag"},
    {(char *)"flag_access", cf_flag_access, CA_GOD, CA_DISABLED, NULL, 0},
    {(char *)"flag_name", cf_flag_name, CA_GOD, CA_DISABLED, NULL, 0},
    {(char *)"forbid_site", cf_site, CA_GOD, CA_DISABLED, (int *)&mudstate.access_list, H_FORBIDDEN},
    {(char *)"fork_dump", cf_bool, CA_GOD, CA_WIZARD, &mudconf.fork_dump, (long)"Dumps are performed using a forked process"},
    {(char *)"fork_vfork", cf_bool, CA_GOD, CA_WIZARD, &mudconf.fork_vfork, (long)"Forks are done using vfork()"},
    {(char *)"forwardlist_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.fwdlist_lim, 0},
    {(char *)"full_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.full_file, MBUF_SIZE},
    {(char *)"full_motd_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.fullmotd_msg, GBUF_SIZE},
    {(char *)"function_access", cf_func_access, CA_GOD, CA_DISABLED, NULL, (long)access_nametab},
    {(char *)"function_alias", cf_alias, CA_GOD, CA_DISABLED, (int *)&mudstate.func_htab, (long)"Function"},
    {(char *)"function_invocation_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.func_invk_lim, 0},
    {(char *)"function_recursion_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.func_nest_lim, 0},
    {(char *)"function_cpu_limit", cf_int, CA_STATIC, CA_PUBLIC, &mudconf.func_cpu_lim_secs, 0},
    {(char *)"global_aconn_uselocks", cf_bool, CA_GOD, CA_WIZARD, &mudconf.global_aconn_uselocks, (long)"Obey UseLocks on global @Aconnect and @Adisconnect"},
    {(char *)"good_name", cf_badname, CA_GOD, CA_DISABLED, NULL, 1},
    {(char *)"gridsize_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.max_grid_size, 0},
    {(char *)"guest_basename", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mudconf.guest_basename, 22},
    {(char *)"guest_char_num", cf_dbref, CA_GOD, CA_WIZARD, &mudconf.guest_char, NOTHING},
    {(char *)"guest_nuker", cf_dbref, CA_GOD, CA_WIZARD, &mudconf.guest_nuker, GOD},
    {(char *)"guest_password", cf_string, CA_GOD, CA_GOD, (int *)&mudconf.guest_password, SBUF_SIZE},
    {(char *)"guest_prefixes", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.guest_prefixes, LBUF_SIZE},
    {(char *)"guest_suffixes", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.guest_suffixes, LBUF_SIZE},
    {(char *)"number_guests", cf_int, CA_STATIC, CA_WIZARD, &mudconf.number_guests, 0},
    {(char *)"guest_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.guest_file, MBUF_SIZE},
    {(char *)"guest_site", cf_site, CA_GOD, CA_DISABLED, (int *)&mudstate.access_list, H_GUEST},
    {(char *)"guest_starting_room", cf_dbref, CA_GOD, CA_WIZARD, &mudconf.guest_start_room, NOTHING},

    {(char *)"have_pueblo", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.have_pueblo, (long)"Pueblo client extensions are supported"},
    {(char *)"have_zones", cf_bool, CA_STATIC, CA_PUBLIC, &mudconf.have_zones, (long)"Multiple control via ControlLocks is permitted"},

    {(char *)"helpfile", cf_helpfile, CA_STATIC, CA_DISABLED, NULL, 0},
    {(char *)"help_users", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.help_users, MBUF_SIZE},
    {(char *)"help_wizards", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.help_wizards, MBUF_SIZE},
    {(char *)"help_quick", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.help_quick, MBUF_SIZE},

    {(char *)"hostnames", cf_bool, CA_GOD, CA_WIZARD, &mudconf.use_hostname, (long)"DNS lookups are done on hostnames"},
    {(char *)"html_connect_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.htmlconn_file, MBUF_SIZE},
    {(char *)"pueblo_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.pueblo_msg, GBUF_SIZE},
    {(char *)"pueblo_version", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.pueblo_version, GBUF_SIZE},
    {(char *)"hash_factor", cf_int, CA_STATIC, CA_WIZARD, &mudconf.hash_factor, (long)"Hash Factor"},
    {(char *)"huh_message", cf_string, CA_GOD, CA_PUBLIC, (int *)&mudconf.huh_msg, MBUF_SIZE},
    {(char *)"idle_wiz_dark", cf_bool, CA_GOD, CA_WIZARD, &mudconf.idle_wiz_dark, (long)"Wizards who idle are set DARK"},
    {(char *)"idle_interval", cf_int, CA_GOD, CA_WIZARD, &mudconf.idle_interval, 0},
    {(char *)"idle_timeout", cf_int, CA_GOD, CA_PUBLIC, &mudconf.idle_timeout, 0},
    {(char *)"include", cf_include, CA_STATIC, CA_DISABLED, NULL, 0},
    {(char *)"indent_desc", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.indent_desc, (long)"Descriptions are indented"},
    {(char *)"info_text", cf_infotext, CA_GOD, CA_DISABLED, NULL, 0},
    {(char *)"initial_size", cf_int, CA_STATIC, CA_WIZARD, &mudconf.init_size, 0},
    {(char *)"instance_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.instance_lim, 0},
    {(char *)"instant_recycle", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.instant_recycle, (long)"@destroy instantly recycles objects set DESTROY_OK"},
    {(char *)"kill_guarantee_cost", cf_int_factor, CA_GOD, CA_PUBLIC, &mudconf.killguarantee, 0},
    {(char *)"kill_max_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.killmax, 0},
    {(char *)"kill_min_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.killmin, 0},
    {(char *)"lag_check", cf_bool, CA_STATIC, CA_PUBLIC, &mudconf.lag_check, (long)"CPU usage warnings are enabled"},
    {(char *)"lag_check_clk", cf_bool, CA_STATIC, CA_PUBLIC, &mudconf.lag_check_clk, (long)"Track CPU usage using wall-clock"},
    {(char *)"lag_check_cpu", cf_bool, CA_STATIC, CA_PUBLIC, &mudconf.lag_check_cpu, (long)"Track CPU usage using getrusage()"},
    {(char *)"lag_maximum", cf_int, CA_GOD, CA_WIZARD, &mudconf.max_cmdsecs, 0},
    {(char *)"lattr_default_oldstyle", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.lattr_oldstyle, (long)"Empty lattr() returns blank, not #-1 NO MATCH"},
    {(char *)"link_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.linkcost, 0},
    {(char *)"list_access", cf_ntab_access, CA_GOD, CA_DISABLED, (int *)list_names, (long)access_nametab},
    {(char *)"local_master_rooms", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.local_masters, (long)"Objects set ZONE act as local master rooms"},
    {(char *)"local_master_parents", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.match_zone_parents, (long)"Objects in local master rooms inherit\n\t\t\t\tcommands from their parent"},
    {(char *)"lock_recursion_limit", cf_int, CA_WIZARD, CA_PUBLIC, &mudconf.lock_nest_lim, 0},
    {(char *)"log", cf_modify_bits, CA_GOD, CA_DISABLED, &mudconf.log_options, (long)logoptions_nametab},
    {(char *)"log_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.log_home, MBUF_SIZE},
    {(char *)"log_options", cf_modify_bits, CA_GOD, CA_DISABLED, &mudconf.log_info, (long)logdata_nametab},
    {(char *)"logout_cmd_access", cf_ntab_access, CA_GOD, CA_DISABLED, (int *)logout_cmdtable, (long)access_nametab},
    {(char *)"logout_cmd_alias", cf_alias, CA_GOD, CA_DISABLED, (int *)&mudstate.logout_cmd_htab, (long)"Logged-out command"},
    {(char *)"look_obey_terse", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.terse_look, (long)"look obeys the TERSE flag"},
    {(char *)"machine_command_cost", cf_int_factor, CA_GOD, CA_PUBLIC, &mudconf.machinecost, 0},
    {(char *)"malloc_logger", cf_bool, CA_STATIC, CA_PUBLIC, &mudconf.malloc_logger, (long)"log allocation of memory"},
    {(char *)"master_room", cf_dbref, CA_GOD, CA_WIZARD, &mudconf.master_room, NOTHING},
    {(char *)"match_own_commands", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.match_mine, (long)"Non-players can match $-commands on themselves"},
    {(char *)"max_command_arguments", cf_int, CA_STATIC, CA_WIZARD, &mudconf.max_command_args, (long)"Maximum number of arguments a command may have"},
    {(char *)"max_global_registers", cf_int, CA_STATIC, CA_WIZARD, &mudconf.max_global_regs, (long)"Maximum length of a player name"},
    {(char *)"max_player_name_length", cf_int, CA_STATIC, CA_WIZARD, &mudconf.player_name_length, (long)"Maximum length of a player name"},
    {(char *)"max_players", cf_int, CA_GOD, CA_WIZARD, &mudconf.max_players, 0},
    {(char *)"module", cf_module, CA_STATIC, CA_WIZARD, NULL, 0},
    {(char *)"modules_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.modules_home, MBUF_SIZE},
    {(char *)"money_name_plural", cf_string, CA_GOD, CA_PUBLIC, (int *)&mudconf.many_coins, SBUF_SIZE},
    {(char *)"money_name_singular", cf_string, CA_GOD, CA_PUBLIC, (int *)&mudconf.one_coin, SBUF_SIZE},
    {(char *)"motd_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.motd_file, MBUF_SIZE},
    {(char *)"motd_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.motd_msg, GBUF_SIZE},
    {(char *)"move_match_more", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.move_match_more, (long)"Move command checks for global and zone exits,\n\t\t\t\tresolves ambiguity"},
    {(char *)"mud_name", cf_string, CA_GOD, CA_PUBLIC, (int *)&mudconf.mud_name, SBUF_SIZE},
    {(char *)"mud_shortname", cf_string, CA_STATIC, CA_PUBLIC, (int *)&mudconf.mud_shortname, SBUF_SIZE},
    {(char *)"mud_owner", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.mudowner, MBUF_SIZE},
    {(char *)"newuser_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.crea_file, MBUF_SIZE},
    {(char *)"no_ambiguous_match", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.no_ambiguous_match, (long)"Ambiguous matches resolve to the last match"},
    {(char *)"notify_recursion_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.ntfy_nest_lim, 0},
    {(char *)"objeval_requires_control", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.fascist_objeval, (long)"Control of victim required by objeval()"},
    {(char *)"open_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.opencost, 0},
    {(char *)"opt_frequency", cf_int, CA_GOD, CA_WIZARD, &mudconf.dbopt_interval, 0},
    {(char *)"output_block_size", cf_int, CA_STATIC, CA_PUBLIC, &mudconf.output_block_size, (long)"block size of output buffer"},
    {(char *)"output_limit", cf_int, CA_GOD, CA_WIZARD, &mudconf.output_limit, 0},
    {(char *)"page_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.pagecost, 0},
    {(char *)"page_requires_equals", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.page_req_equals, (long)"page command always requires an equals sign"},
    {(char *)"parent_recursion_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.parent_nest_lim, 0},
    {(char *)"paycheck", cf_int, CA_GOD, CA_PUBLIC, &mudconf.paycheck, 0},
    {(char *)"pemit_far_players", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.pemit_players, (long)"@pemit targets can be players in other locations"},
    {(char *)"pemit_any_object", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.pemit_any, (long)"@pemit targets can be objects in other locations"},
    {(char *)"permit_site", cf_site, CA_GOD, CA_DISABLED, (int *)&mudstate.access_list, 0},
    {(char *)"pid_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.pid_home, MBUF_SIZE},
    {(char *)"player_aliases_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.max_player_aliases, 0},
    {(char *)"player_flags", cf_set_flags, CA_GOD, CA_DISABLED, (int *)&mudconf.player_flags, 0},
    {(char *)"player_listen", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.player_listen, (long)"@listen and ^-monitors are checked on players"},
    {(char *)"player_match_own_commands", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.match_mine_pl, (long)"Players can match $-commands on themselves"},
    {(char *)"player_name_spaces", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.name_spaces, (long)"Player names can contain spaces"},
    {(char *)"player_name_minlength", cf_int, CA_GOD, CA_GOD, &mudconf.player_name_min, 0},
    {(char *)"player_parent", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.player_parent, NOTHING},
    {(char *)"player_proto", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.player_proto, NOTHING},
    {(char *)"player_attr_defaults", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.player_defobj, NOTHING},
    {(char *)"player_queue_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.queuemax, 0},
    {(char *)"player_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.player_quota, 0},
    {(char *)"player_starting_home", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.start_home, NOTHING},
    {(char *)"player_starting_room", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.start_room, 0},
    {(char *)"port", cf_int, CA_STATIC, CA_PUBLIC, &mudconf.port, 0},
    {(char *)"power_access", cf_power_access, CA_GOD, CA_DISABLED, NULL, 0},
    {(char *)"power_alias", cf_alias, CA_GOD, CA_DISABLED, (int *)&mudstate.powers_htab, (long)"Power"},
    {(char *)"propdir_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.propdir_lim, 0},
    {(char *)"public_flags", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.pub_flags, (long)"Flag information is public"},
    {(char *)"queue_active_chunk", cf_int, CA_GOD, CA_PUBLIC, &mudconf.active_q_chunk, 0},
    {(char *)"queue_idle_chunk", cf_int, CA_GOD, CA_PUBLIC, &mudconf.queue_chunk, 0},
    {(char *)"queue_max_size", cf_int, CA_GOD, CA_PUBLIC, &mudconf.max_qpid, 0},
    {(char *)"quiet_look", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.quiet_look, (long)"look shows public attributes in addition to @Desc"},
    {(char *)"quiet_whisper", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.quiet_whisper, (long)"whisper is quiet"},
    {(char *)"quit_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.quit_file, MBUF_SIZE},
    {(char *)"quotas", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.quotas, (long)"Quotas are enforced"},
    {(char *)"raw_helpfile", cf_raw_helpfile, CA_STATIC, CA_DISABLED, NULL, 0},
    {(char *)"read_remote_desc", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.read_rem_desc, (long)"@Desc is public, even to players not nearby"},
    {(char *)"read_remote_name", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.read_rem_name, (long)"Names are public, even to players not nearby"},
    {(char *)"register_create_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.regf_file, MBUF_SIZE},
    {(char *)"register_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.register_limit, 0},
    {(char *)"register_site", cf_site, CA_GOD, CA_DISABLED, (int *)&mudstate.access_list, H_REGISTRATION},
    {(char *)"require_cmds_flag", cf_bool, CA_GOD, CA_PUBLIC, (int *)&mudconf.req_cmds_flag, (long)"Only objects with COMMANDS flag are searched\n\t\t\t\tfor $-commands"},
    {(char *)"retry_limit", cf_int, CA_GOD, CA_WIZARD, &mudconf.retry_limit, 0},
    {(char *)"robot_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.robotcost, 0},
    {(char *)"robot_flags", cf_set_flags, CA_GOD, CA_DISABLED, (int *)&mudconf.robot_flags, 0},
    {(char *)"robot_speech", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.robot_speak, (long)"Robots can speak in locations their owners do not\n\t\t\t\tcontrol"},
    {(char *)"room_flags", cf_set_flags, CA_GOD, CA_DISABLED, (int *)&mudconf.room_flags, 0},
    {(char *)"room_parent", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.room_parent, NOTHING},
    {(char *)"room_proto", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.room_proto, NOTHING},
    {(char *)"room_attr_defaults", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.room_defobj, NOTHING},
    {(char *)"room_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.room_quota, 0},
    {(char *)"sacrifice_adjust", cf_int, CA_GOD, CA_PUBLIC, &mudconf.sacadjust, 0},
    {(char *)"sacrifice_factor", cf_int_factor, CA_GOD, CA_PUBLIC, &mudconf.sacfactor, 0},
    {(char *)"safer_passwords", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.safer_passwords, (long)"Passwords must satisfy minimum security standards"},
    {(char *)"say_uses_comma", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.comma_say, (long)"Say uses a grammatically-correct comma"},
    {(char *)"say_uses_you", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.you_say, (long)"Say uses You rather than the player name"},
    {(char *)"scripts_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.scripts_home, MBUF_SIZE},
    {(char *)"search_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.searchcost, 0},
    {(char *)"see_owned_dark", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.see_own_dark, (long)"look shows DARK objects owned by you"},
    {(char *)"signal_action", cf_option, CA_STATIC, CA_GOD, &mudconf.sig_action, (long)sigactions_nametab},
    {(char *)"site_chars", cf_int, CA_GOD, CA_WIZARD, &mudconf.site_chars, MBUF_SIZE - 2},
    {(char *)"space_compress", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.space_compress, (long)"Multiple spaces are compressed to a single space"},
    {(char *)"stack_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.stack_lim, 0},
    {(char *)"starting_money", cf_int, CA_GOD, CA_PUBLIC, &mudconf.paystart, 0},
    {(char *)"starting_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.start_quota, 0},
    {(char *)"starting_exit_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.start_exit_quota, 0},
    {(char *)"starting_player_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.start_player_quota, 0},
    {(char *)"starting_room_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.start_room_quota, 0},
    {(char *)"starting_thing_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.start_thing_quota, 0},
    {(char *)"status_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.status_file, MBUF_SIZE},
    {(char *)"stripped_flags", cf_set_flags, CA_GOD, CA_DISABLED, (int *)&mudconf.stripped_flags, 0},
    {(char *)"structure_delimiter_string", cf_string, CA_GOD, CA_PUBLIC, (int *)&mudconf.struct_dstr, 0},
    {(char *)"structure_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.struct_lim, 0},
    {(char *)"suspect_site", cf_site, CA_GOD, CA_DISABLED, (int *)&mudstate.suspect_list, H_SUSPECT},
    {(char *)"sweep_dark", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.sweep_dark, (long)"@sweep works on Dark locations"},
    {(char *)"switch_default_all", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.switch_df_all, (long)"@switch default is /all, not /first"},

    {(char *)"terse_shows_contents", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.terse_contents, (long)"TERSE suppresses the contents list of a location"},
    {(char *)"terse_shows_exits", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.terse_exits, (long)"TERSE suppresses the exit list of a location"},
    {(char *)"terse_shows_move_messages", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.terse_movemsg, (long)"TERSE suppresses movement messages"},
    {(char *)"text_home", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.txthome, MBUF_SIZE},
    {(char *)"thing_flags", cf_set_flags, CA_GOD, CA_DISABLED, (int *)&mudconf.thing_flags, 0},
    {(char *)"thing_parent", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.thing_parent, NOTHING},
    {(char *)"thing_proto", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.thing_proto, NOTHING},
    {(char *)"thing_attr_defaults", cf_dbref, CA_GOD, CA_PUBLIC, &mudconf.thing_defobj, NOTHING},
    {(char *)"thing_quota", cf_int, CA_GOD, CA_PUBLIC, &mudconf.thing_quota, 0},
    {(char *)"timeslice", cf_int_factor, CA_GOD, CA_PUBLIC, &mudconf.timeslice, 0},
    {(char *)"trace_output_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.trace_limit, 0},
    {(char *)"trace_topdown", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.trace_topdown, (long)"Trace output is top-down"},
    {(char *)"trust_site", cf_site, CA_GOD, CA_DISABLED, (int *)&mudstate.suspect_list, 0},
    {(char *)"typed_quotas", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.typed_quotas, (long)"Quotas are enforced per object type"},
    {(char *)"unowned_safe", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.safe_unowned, (long)"Objects not owned by you are considered SAFE"},
    {(char *)"user_attr_access", cf_modify_bits, CA_GOD, CA_DISABLED, &mudconf.vattr_flags, (long)attraccess_nametab},
    {(char *)"use_global_aconn", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.use_global_aconn, (long)"Global @Aconnects and @Adisconnects are used"},
    {(char *)"variables_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.numvars_lim, 0},
    {(char *)"visible_wizards", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.visible_wizzes, (long)"DARK Wizards are hidden from WHO but not invisible"},
    {(char *)"wait_cost", cf_int, CA_GOD, CA_PUBLIC, &mudconf.waitcost, 0},
    {(char *)"wildcard_match_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.wild_times_lim, 0},
    {(char *)"wizard_obeys_linklock", cf_bool, CA_GOD, CA_PUBLIC, &mudconf.wiz_obey_linklock, (long)"Check LinkLock even if player can link to anything"},
    {(char *)"wizard_motd_file", cf_string, CA_STATIC, CA_GOD, (int *)&mudconf.wizmotd_file, MBUF_SIZE},
    {(char *)"wizard_motd_message", cf_string, CA_GOD, CA_WIZARD, (int *)&mudconf.wizmotd_msg, GBUF_SIZE},
    {(char *)"zone_recursion_limit", cf_int, CA_GOD, CA_PUBLIC, &mudconf.zone_nest_lim, 0},
    {NULL, NULL, 0, 0, NULL, 0}};

/**
 * @brief Log file descriptor table
 * 
 */
LOGFILETAB logfds_table[] = {
    {LOG_ACCOUNTING, NULL, NULL},
    {LOG_ALLCOMMANDS, NULL, NULL},
    {LOG_BADCOMMANDS, NULL, NULL},
    {LOG_ALLOCATE, NULL, NULL},
    {LOG_BUGS, NULL, NULL},
    {LOG_DBSAVES, NULL, NULL},
    {LOG_CONFIGMODS, NULL, NULL},
    {LOG_PCREATES, NULL, NULL},
    {LOG_KBCOMMANDS, NULL, NULL},
    {LOG_KILLS, NULL, NULL},
    {LOG_LOCAL, NULL, NULL},
    {LOG_LOGIN, NULL, NULL},
    {LOG_NET, NULL, NULL},
    {LOG_PROBLEMS, NULL, NULL},
    {LOG_SECURITY, NULL, NULL},
    {LOG_SHOUTS, NULL, NULL},
    {LOG_STARTUP, NULL, NULL},
    {LOG_SUSPECTCMDS, NULL, NULL},
    {LOG_TIMEUSE, NULL, NULL},
    {LOG_WIZARD, NULL, NULL},
    {LOG_MALLOC, NULL, NULL},
    {0, NULL, NULL}};

/**
 * @brief List of built-in attributes
 *
 */
ATTR attr[] = {
    {"Aahear", A_AAHEAR, AF_DEFAULT | AF_NOPROG, NULL},
    {"Aclone", A_ACLONE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Aconnect", A_ACONNECT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Adesc", A_ADESC, AF_DEFAULT | AF_NOPROG, NULL},
    {"Adfail", A_ADFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Adisconnect", A_ADISCONNECT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Adrop", A_ADROP, AF_DEFAULT | AF_NOPROG, NULL},
    {"Aefail", A_AEFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Aenter", A_AENTER, AF_DEFAULT | AF_NOPROG, NULL},
    {"Afail", A_AFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Agfail", A_AGFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Ahear", A_AHEAR, AF_DEFAULT | AF_NOPROG, NULL},
    {"Akill", A_AKILL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Aleave", A_ALEAVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Alfail", A_ALFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Alias", A_ALIAS, AF_NOPROG | AF_NOCMD | AF_NOCLONE | AF_PRIVATE | AF_CONST, NULL},
    {"Allowance", A_ALLOWANCE, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
    {"Amail", A_AMAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Amhear", A_AMHEAR, AF_DEFAULT | AF_NOPROG, NULL},
    {"Amove", A_AMOVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Apay", A_APAY, AF_DEFAULT | AF_NOPROG, NULL},
    {"Arfail", A_ARFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Asucc", A_ASUCC, AF_DEFAULT | AF_NOPROG, NULL},
    {"Atfail", A_ATFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Atport", A_ATPORT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Atofail", A_ATOFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Aufail", A_AUFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Ause", A_AUSE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Away", A_AWAY, AF_DEFAULT | AF_NOPROG, NULL},
    {"Charges", A_CHARGES, AF_NOPROG, NULL},
    {"ChownLock", A_LCHOWN, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Comment", A_COMMENT, AF_NOPROG | AF_MDARK | AF_WIZARD, NULL},
    {"Conformat", A_LCON_FMT, AF_DEFAULT | AF_NOPROG, NULL},
    {"ControlLock", A_LCONTROL, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Cost", A_COST, AF_NOPROG, NULL},
    {"Daily", A_DAILY, AF_NOPROG, NULL},
    {"DarkLock", A_LDARK, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Desc", A_DESC, AF_DEFAULT | AF_VISUAL | AF_NOPROG, NULL},
    {"DefaultLock", A_LOCK, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Destroyer", A_DESTROYER, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
    {"Dfail", A_DFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Drop", A_DROP, AF_DEFAULT | AF_NOPROG, NULL},
    {"DropLock", A_LDROP, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Ealias", A_EALIAS, AF_NOPROG, NULL},
    {"Efail", A_EFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Enter", A_ENTER, AF_DEFAULT | AF_NOPROG, NULL},
    {"EnterLock", A_LENTER, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Exitformat", A_LEXITS_FMT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Exitto", A_EXITVARDEST, AF_NOPROG, NULL},
    {"Fail", A_FAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Filter", A_FILTER, AF_NOPROG, NULL},
    {"Forwardlist", A_FORWARDLIST, AF_NOPROG, fwdlist_ck},
    {"Gfail", A_GFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"GiveLock", A_LGIVE, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"HeardLock", A_LHEARD, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"HearsLock", A_LHEARS, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Idesc", A_IDESC, AF_DEFAULT | AF_NOPROG, NULL},
    {"Idle", A_IDLE, AF_NOPROG, NULL},
    {"Infilter", A_INFILTER, AF_NOPROG, NULL},
    {"Inprefix", A_INPREFIX, AF_NOPROG, NULL},
    {"Kill", A_KILL, AF_DEFAULT | AF_NOPROG, NULL},
    {"KnownLock", A_LKNOWN, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"KnowsLock", A_LKNOWS, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Lalias", A_LALIAS, AF_NOPROG, NULL},
    {"Last", A_LAST, AF_VISUAL | AF_WIZARD | AF_NOCMD | AF_NOPROG | AF_NOCLONE, NULL},
    {"Lastip", A_LASTIP, AF_NOPROG | AF_NOCMD | AF_NOCLONE | AF_GOD, NULL},
    {"Lastpage", A_LASTPAGE, AF_INTERNAL | AF_NOCMD | AF_NOPROG | AF_GOD | AF_PRIVATE, NULL},
    {"Lastsite", A_LASTSITE, AF_NOPROG | AF_NOCMD | AF_NOCLONE | AF_GOD, NULL},
    {"Leave", A_LEAVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"LeaveLock", A_LLEAVE, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Lfail", A_LFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"LinkLock", A_LLINK, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Listen", A_LISTEN, AF_NOPROG, NULL},
    {"Logindata", A_LOGINDATA, AF_MDARK | AF_NOPROG | AF_NOCMD | AF_CONST, NULL},
    {"Mailcurf", A_MAILCURF, AF_MDARK | AF_WIZARD | AF_NOPROG | AF_NOCLONE, NULL},
    {"Mailflags", A_MAILFLAGS, AF_MDARK | AF_WIZARD | AF_NOPROG | AF_NOCLONE, NULL},
    {"Mailfolders", A_MAILFOLDERS, AF_MDARK | AF_WIZARD | AF_NOPROG | AF_NOCLONE, NULL},
    {"Mailmsg", A_MAILMSG, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"Mailsub", A_MAILSUB, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"Mailsucc", A_MAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Mailto", A_MAILTO, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"MovedLock", A_LMOVED, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"MovesLock", A_LMOVES, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Move", A_MOVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Name", A_NAME, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"Nameformat", A_NAME_FMT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Newobjs", A_NEWOBJS, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOCMD | AF_NOCLONE, NULL},
    {"Odesc", A_ODESC, AF_DEFAULT | AF_NOPROG, NULL},
    {"Odfail", A_ODFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Odrop", A_ODROP, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oefail", A_OEFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oenter", A_OENTER, AF_DEFAULT | AF_NOPROG, NULL},
    {"Ofail", A_OFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Ogfail", A_OGFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Okill", A_OKILL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oleave", A_OLEAVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Olfail", A_OLFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Omove", A_OMOVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Opay", A_OPAY, AF_DEFAULT | AF_NOPROG, NULL},
    {"Orfail", A_ORFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Osucc", A_OSUCC, AF_DEFAULT | AF_NOPROG, NULL},
    {"Otfail", A_OTFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Otport", A_OTPORT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Otofail", A_OTOFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oufail", A_OUFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Ouse", A_OUSE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oxenter", A_OXENTER, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oxleave", A_OXLEAVE, AF_DEFAULT | AF_NOPROG, NULL},
    {"Oxtport", A_OXTPORT, AF_DEFAULT | AF_NOPROG, NULL},
    {"Pagegroup", A_PAGEGROUP, AF_INTERNAL | AF_NOCMD | AF_NOPROG | AF_GOD | AF_PRIVATE, NULL},
    {"PageLock", A_LPAGE, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"ParentLock", A_LPARENT, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Pay", A_PAY, AF_NOPROG, NULL},
    {"Prefix", A_PREFIX, AF_NOPROG, NULL},
    {"Progcmd", A_PROGCMD, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"Propdir", A_PROPDIR, AF_NOPROG, propdir_ck},
    {"Queuemax", A_QUEUEMAX, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
    {"Quota", A_QUOTA, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOCMD | AF_NOCLONE, NULL},
    {"ReceiveLock", A_LRECEIVE, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Reject", A_REJECT, AF_NOPROG, NULL},
    {"Rfail", A_RFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Rquota", A_RQUOTA, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOCMD | AF_NOCLONE, NULL},
    {"Runout", A_RUNOUT, AF_NOPROG, NULL},
    {"Semaphore", A_SEMAPHORE, AF_NOPROG | AF_WIZARD | AF_NOCMD | AF_NOCLONE, NULL},
    {"Sex", A_SEX, AF_VISUAL | AF_NOPROG, NULL},
    {"Signature", A_SIGNATURE, AF_NOPROG, NULL},
    {"Speechformat", A_SPEECHFMT, AF_DEFAULT | AF_NOPROG, NULL},
    {"SpeechLock", A_LSPEECH, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Startup", A_STARTUP, AF_NOPROG, NULL},
    {"Succ", A_SUCC, AF_DEFAULT | AF_NOPROG, NULL},
    {"TeloutLock", A_LTELOUT, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Tfail", A_TFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Timeout", A_TIMEOUT, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
    {"Tport", A_TPORT, AF_DEFAULT | AF_NOPROG, NULL},
    {"TportLock", A_LTPORT, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Tofail", A_TOFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Ufail", A_UFAIL, AF_DEFAULT | AF_NOPROG, NULL},
    {"Use", A_USE, AF_DEFAULT | AF_NOPROG, NULL},
    {"UseLock", A_LUSE, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"UserLock", A_LUSER, AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"Va", A_VA, 0, NULL},
    {"Vb", A_VA + 1, 0, NULL},
    {"Vc", A_VA + 2, 0, NULL},
    {"Vd", A_VA + 3, 0, NULL},
    {"Ve", A_VA + 4, 0, NULL},
    {"Vf", A_VA + 5, 0, NULL},
    {"Vg", A_VA + 6, 0, NULL},
    {"Vh", A_VA + 7, 0, NULL},
    {"Vi", A_VA + 8, 0, NULL},
    {"Vj", A_VA + 9, 0, NULL},
    {"Vk", A_VA + 10, 0, NULL},
    {"Vl", A_VA + 11, 0, NULL},
    {"Vm", A_VA + 12, 0, NULL},
    {"Vn", A_VA + 13, 0, NULL},
    {"Vo", A_VA + 14, 0, NULL},
    {"Vp", A_VA + 15, 0, NULL},
    {"Vq", A_VA + 16, 0, NULL},
    {"Vr", A_VA + 17, 0, NULL},
    {"Vs", A_VA + 18, 0, NULL},
    {"Vt", A_VA + 19, 0, NULL},
    {"Vu", A_VA + 20, 0, NULL},
    {"Vv", A_VA + 21, 0, NULL},
    {"Vw", A_VA + 22, 0, NULL},
    {"Vx", A_VA + 23, 0, NULL},
    {"Vy", A_VA + 24, 0, NULL},
    {"Vz", A_VA + 25, 0, NULL},
    {"Vrml_url", A_VRML_URL, AF_NOPROG, NULL},
    {"Htdesc", A_HTDESC, AF_DEFAULT | AF_VISUAL | AF_NOPROG, NULL},
    {"*Atrlist", A_LIST, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"*Password", A_PASS, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"*Money", A_MONEY, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"*Invalid", A_TEMP, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {NULL, 0, 0, NULL}};

