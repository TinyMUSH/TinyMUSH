/* cmdtabs.h - command and other supporting tables */
/* $Id: cmdtabs.h,v 1.50 2010/05/31 18:29:18 lwl Exp $ */

#include "copyright.h"

/* Make sure that all of your command and switch names are lowercase! */

/* *INDENT-OFF* */

/*
 * ---------------------------------------------------------------------------
 * Switch tables for the various commands.
 */

NAMETAB		addcmd_sw[] =
{
    {(char *)"preserve", 1, CA_GOD, ADDCMD_PRESERVE},
    {NULL, 0, 0, 0}
};

NAMETAB		attrib_sw[] =
{
    {(char *)"access", 1, CA_GOD, ATTRIB_ACCESS},
    {(char *)"delete", 1, CA_GOD, ATTRIB_DELETE},
    {(char *)"info", 1, CA_WIZARD, ATTRIB_INFO},
    {(char *)"rename", 1, CA_GOD, ATTRIB_RENAME},
    {NULL, 0, 0, 0}
};

NAMETAB		boot_sw[] =
{
    {(char *)"port", 1, CA_WIZARD, BOOT_PORT | SW_MULTIPLE},
    {(char *)"quiet", 1, CA_WIZARD, BOOT_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		chown_sw[] =
{
    {(char *)"nostrip", 1, CA_WIZARD, CHOWN_NOSTRIP},
    {NULL, 0, 0, 0}
};

NAMETAB		chzone_sw[] =
{
    {(char *)"nostrip", 1, CA_WIZARD, CHZONE_NOSTRIP},
    {NULL, 0, 0, 0}
};

NAMETAB		clone_sw[] =
{
    {(char *)"cost", 1, CA_PUBLIC, CLONE_SET_COST | SW_MULTIPLE},
    {(char *)"inherit", 3, CA_PUBLIC, CLONE_INHERIT | SW_MULTIPLE},
    {(char *)"inventory", 3, CA_PUBLIC, CLONE_INVENTORY},
    {(char *)"location", 1, CA_PUBLIC, CLONE_LOCATION},
    {(char *)"nostrip", 1, CA_WIZARD, CLONE_NOSTRIP | SW_MULTIPLE},
    {(char *)"parent", 2, CA_PUBLIC, CLONE_FROM_PARENT | SW_MULTIPLE},
    {(char *)"preserve", 2, CA_PUBLIC, CLONE_PRESERVE | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		decomp_sw[] =
{
    {(char *)"pretty", 1, CA_PUBLIC, DECOMP_PRETTY},
    {NULL, 0, 0, 0}
};

NAMETAB		destroy_sw[] =
{
    {(char *)"instant", 4, CA_PUBLIC, DEST_INSTANT | SW_MULTIPLE},
    {(char *)"override", 8, CA_PUBLIC, DEST_OVERRIDE | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		dig_sw [] =
{
    {(char *)"teleport", 1, CA_PUBLIC, DIG_TELEPORT},
    {NULL, 0, 0, 0}
};

NAMETAB		doing_sw[] =
{
    {(char *)"header", 1, CA_PUBLIC, DOING_HEADER | SW_MULTIPLE},
    {(char *)"message", 1, CA_PUBLIC, DOING_MESSAGE | SW_MULTIPLE},
    {(char *)"poll", 1, CA_PUBLIC, DOING_POLL},
    {(char *)"quiet", 1, CA_PUBLIC, DOING_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		dolist_sw[] =
{
    {(char *)"delimit", 1, CA_PUBLIC, DOLIST_DELIMIT},
    {(char *)"space", 1, CA_PUBLIC, DOLIST_SPACE},
    {(char *)"notify", 1, CA_PUBLIC, DOLIST_NOTIFY | SW_MULTIPLE},
    {(char *)"now", 1, CA_PUBLIC, DOLIST_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0,}
};

NAMETAB		drop_sw[] =
{
    {(char *)"quiet", 1, CA_PUBLIC, DROP_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		dump_sw[] =
{
    {(char *)"structure", 1, CA_WIZARD, DUMP_STRUCT | SW_MULTIPLE},
    {(char *)"text", 1, CA_WIZARD, DUMP_TEXT | SW_MULTIPLE},
    {(char *)"flatfile", 1, CA_WIZARD, DUMP_FLATFILE | SW_MULTIPLE},
    {(char *)"optimize", 1, CA_WIZARD, DUMP_OPTIMIZE | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		emit_sw[] =
{
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"here", 1, CA_PUBLIC, SAY_HERE | SW_MULTIPLE},
    {(char *)"room", 1, CA_PUBLIC, SAY_ROOM | SW_MULTIPLE},
#ifdef PUEBLO_SUPPORT
    {(char *)"html", 1, CA_PUBLIC, SAY_HTML | SW_MULTIPLE},
#endif
    {NULL, 0, 0, 0}
};

NAMETAB		end_sw [] =
{
    {(char *)"assert", 1, CA_PUBLIC, ENDCMD_ASSERT},
    {(char *)"break", 1, CA_PUBLIC, ENDCMD_BREAK},
    {NULL, 0, 0, 0}
};

NAMETAB		enter_sw[] =
{
    {(char *)"quiet", 1, CA_PUBLIC, MOVE_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		examine_sw[] =
{
    {(char *)"brief", 1, CA_PUBLIC, EXAM_BRIEF},
    {(char *)"debug", 1, CA_WIZARD, EXAM_DEBUG},
    {(char *)"full", 1, CA_PUBLIC, EXAM_LONG},
    {(char *)"owner", 1, CA_PUBLIC, EXAM_OWNER},
    {(char *)"pairs", 3, CA_PUBLIC, EXAM_PAIRS},
    {(char *)"parent", 1, CA_PUBLIC, EXAM_PARENT | SW_MULTIPLE},
    {(char *)"pretty", 2, CA_PUBLIC, EXAM_PRETTY},
    {NULL, 0, 0, 0}
};

NAMETAB		femit_sw[] =
{
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"here", 1, CA_PUBLIC, PEMIT_HERE | SW_MULTIPLE},
    {(char *)"room", 1, CA_PUBLIC, PEMIT_ROOM | SW_MULTIPLE},
    {(char *)"spoof", 1, CA_PUBLIC, PEMIT_SPOOF | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		fixdb_sw[] =
{
    /* {(char *)"add_pname",1,	CA_GOD,		FIXDB_ADD_PN}, */
    {(char *)"contents", 1, CA_GOD, FIXDB_CON},
    {(char *)"exits", 1, CA_GOD, FIXDB_EXITS},
    {(char *)"location", 1, CA_GOD, FIXDB_LOC},
    {(char *)"next", 1, CA_GOD, FIXDB_NEXT},
    {(char *)"owner", 1, CA_GOD, FIXDB_OWNER},
    {(char *)"pennies", 1, CA_GOD, FIXDB_PENNIES},
    {(char *)"rename", 1, CA_GOD, FIXDB_NAME},
    /* {(char *)"rm_pname",	1,	CA_GOD,		FIXDB_DEL_PN}, */
    {NULL, 0, 0, 0}
};

NAMETAB		floaters_sw[] =
{
    {(char *)"all", 1, CA_PUBLIC, FLOATERS_ALL},
    {NULL, 0, 0, 0}
};

NAMETAB		force_sw[] =
{
    {(char *)"now", 1, CA_PUBLIC, FRC_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		fpose_sw[] =
{
    {(char *)"default", 1, CA_PUBLIC, 0},
    {(char *)"noeval", 3, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"nospace", 1, CA_PUBLIC, SAY_NOSPACE},
    {(char *)"spoof", 1, CA_PUBLIC, PEMIT_SPOOF | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		fsay_sw[] =
{
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"spoof", 1, CA_PUBLIC, PEMIT_SPOOF | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		function_sw[] =
{
    {(char *)"list", 1, CA_WIZARD, FUNCT_LIST},
    {(char *)"noeval", 1, CA_WIZARD, FUNCT_NO_EVAL | SW_MULTIPLE},
    {(char *)"privileged", 3, CA_WIZARD, FUNCT_PRIV | SW_MULTIPLE},
    {(char *)"private", 5, CA_WIZARD, FUNCT_NOREGS | SW_MULTIPLE},
    {(char *)"preserve", 3, CA_WIZARD, FUNCT_PRES | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		get_sw [] =
{
    {(char *)"quiet", 1, CA_PUBLIC, GET_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		give_sw[] =
{
    {(char *)"quiet", 1, CA_WIZARD, GIVE_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		goto_sw[] =
{
    {(char *)"quiet", 1, CA_PUBLIC, MOVE_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		halt_sw[] =
{
    {(char *)"all", 1, CA_PUBLIC, HALT_ALL},
    {(char *)"pid", 1, CA_PUBLIC, HALT_PID},
    {NULL, 0, 0, 0}
};

NAMETAB		help_sw[] =
{
    {(char *)"fine", 1, CA_PUBLIC, HELP_FIND},
    {NULL, 0, 0, 0}
};

NAMETAB		hook_sw[] =
{
    {(char *)"before", 1, CA_GOD, HOOK_BEFORE},
    {(char *)"after", 1, CA_GOD, HOOK_AFTER},
    {(char *)"permit", 1, CA_GOD, HOOK_PERMIT},
    {(char *)"preserve", 3, CA_GOD, HOOK_PRESERVE},
    {(char *)"nopreserve", 1, CA_GOD, HOOK_NOPRESERVE},
    {(char *)"private", 3, CA_GOD, HOOK_PRIVATE},
    {NULL, 0, 0, 0}
};

NAMETAB		leave_sw[] =
{
    {(char *)"quiet", 1, CA_PUBLIC, MOVE_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		listmotd_sw[] =
{
    {(char *)"brief", 1, CA_WIZARD, MOTD_BRIEF},
    {NULL, 0, 0, 0}
};

NAMETAB		lock_sw[] =
{
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
    {NULL, 0, 0, 0}
};

NAMETAB		look_sw[] =
{
    {(char *)"outside", 1, CA_PUBLIC, LOOK_OUTSIDE},
    {NULL, 0, 0, 0}
};

NAMETAB		mark_sw[] =
{
    {(char *)"set", 1, CA_PUBLIC, MARK_SET},
    {(char *)"clear", 1, CA_PUBLIC, MARK_CLEAR},
    {NULL, 0, 0, 0}
};

NAMETAB		markall_sw[] =
{
    {(char *)"set", 1, CA_PUBLIC, MARK_SET},
    {(char *)"clear", 1, CA_PUBLIC, MARK_CLEAR},
    {NULL, 0, 0, 0}
};

NAMETAB		motd_sw[] =
{
    {(char *)"brief", 1, CA_WIZARD, MOTD_BRIEF | SW_MULTIPLE},
    {(char *)"connect", 1, CA_WIZARD, MOTD_ALL},
    {(char *)"down", 1, CA_WIZARD, MOTD_DOWN},
    {(char *)"full", 1, CA_WIZARD, MOTD_FULL},
    {(char *)"list", 1, CA_PUBLIC, MOTD_LIST},
    {(char *)"wizard", 1, CA_WIZARD, MOTD_WIZ},
    {NULL, 0, 0, 0}
};

NAMETAB		notify_sw[] =
{
    {(char *)"all", 1, CA_PUBLIC, NFY_NFYALL},
    {(char *)"first", 1, CA_PUBLIC, NFY_NFY},
    {NULL, 0, 0, 0}
};

NAMETAB		oemit_sw[] =
{
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"speech", 1, CA_PUBLIC, PEMIT_SPEECH | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		open_sw[] =
{
    {(char *)"inventory", 1, CA_PUBLIC, OPEN_INVENTORY},
    {(char *)"location", 1, CA_PUBLIC, OPEN_LOCATION},
    {NULL, 0, 0, 0}
};

NAMETAB		pemit_sw[] =
{
    {(char *)"contents", 1, CA_PUBLIC, PEMIT_CONTENTS | SW_MULTIPLE},
    {(char *)"object", 1, CA_PUBLIC, 0},
    {(char *)"silent", 2, CA_PUBLIC, 0},
    {(char *)"speech", 2, CA_PUBLIC, PEMIT_SPEECH | SW_MULTIPLE},
    {(char *)"list", 1, CA_PUBLIC, PEMIT_LIST | SW_MULTIPLE},
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
#ifdef PUEBLO_SUPPORT
    {(char *)"html", 1, CA_PUBLIC, PEMIT_HTML | SW_MULTIPLE},
#endif
    {NULL, 0, 0, 0}
};

NAMETAB		pose_sw[] =
{
    {(char *)"default", 1, CA_PUBLIC, 0},
    {(char *)"noeval", 3, CA_PUBLIC, SW_NOEVAL | SW_MULTIPLE},
    {(char *)"nospace", 1, CA_PUBLIC, SAY_NOSPACE},
    {NULL, 0, 0, 0}
};

NAMETAB		ps_sw  [] =
{
    {(char *)"all", 1, CA_PUBLIC, PS_ALL | SW_MULTIPLE},
    {(char *)"brief", 1, CA_PUBLIC, PS_BRIEF},
    {(char *)"long", 1, CA_PUBLIC, PS_LONG},
    {(char *)"summary", 1, CA_PUBLIC, PS_SUMM},
    {NULL, 0, 0, 0}
};

NAMETAB		quota_sw[] =
{
    {(char *)"all", 1, CA_GOD, QUOTA_ALL | SW_MULTIPLE},
    {(char *)"fix", 1, CA_WIZARD, QUOTA_FIX},
    {(char *)"remaining", 1, CA_WIZARD, QUOTA_REM | SW_MULTIPLE},
    {(char *)"set", 1, CA_WIZARD, QUOTA_SET},
    {(char *)"total", 1, CA_WIZARD, QUOTA_TOT | SW_MULTIPLE},
    {(char *)"room", 1, CA_WIZARD, QUOTA_ROOM | SW_MULTIPLE},
    {(char *)"exit", 1, CA_WIZARD, QUOTA_EXIT | SW_MULTIPLE},
    {(char *)"thing", 1, CA_WIZARD, QUOTA_THING | SW_MULTIPLE},
    {(char *)"player", 1, CA_WIZARD, QUOTA_PLAYER | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		reference_sw[] =
{
    {(char *)"list", 1, CA_PUBLIC, NREF_LIST},
    {NULL, 0, 0, 0}
};

NAMETAB		set_sw [] =
{
    {(char *)"quiet", 1, CA_PUBLIC, SET_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		shutdown_sw[] =
{
    {(char *)"abort", 1, CA_WIZARD, SHUTDN_COREDUMP},
    {NULL, 0, 0, 0}
};

NAMETAB		stats_sw[] =
{
    {(char *)"all", 1, CA_PUBLIC, STAT_ALL},
    {(char *)"me", 1, CA_PUBLIC, STAT_ME},
    {(char *)"player", 1, CA_PUBLIC, STAT_PLAYER},
    {NULL, 0, 0, 0}
};

NAMETAB		sweep_sw[] =
{
    {(char *)"commands", 3, CA_PUBLIC, SWEEP_COMMANDS | SW_MULTIPLE},
    {(char *)"connected", 3, CA_PUBLIC, SWEEP_CONNECT | SW_MULTIPLE},
    {(char *)"exits", 1, CA_PUBLIC, SWEEP_EXITS | SW_MULTIPLE},
    {(char *)"here", 1, CA_PUBLIC, SWEEP_HERE | SW_MULTIPLE},
    {(char *)"inventory", 1, CA_PUBLIC, SWEEP_ME | SW_MULTIPLE},
    {(char *)"listeners", 1, CA_PUBLIC, SWEEP_LISTEN | SW_MULTIPLE},
    {(char *)"players", 1, CA_PUBLIC, SWEEP_PLAYER | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		switch_sw[] =
{
    {(char *)"all", 1, CA_PUBLIC, SWITCH_ANY},
    {(char *)"default", 1, CA_PUBLIC, SWITCH_DEFAULT},
    {(char *)"first", 1, CA_PUBLIC, SWITCH_ONE},
    {(char *)"now", 1, CA_PUBLIC, SWITCH_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		teleport_sw[] =
{
    {(char *)"loud", 1, CA_PUBLIC, TELEPORT_DEFAULT},
    {(char *)"quiet", 1, CA_PUBLIC, TELEPORT_QUIET},
    {NULL, 0, 0, 0}
};

NAMETAB		timecheck_sw[] =
{
    {(char *)"log", 1, CA_WIZARD, TIMECHK_LOG | SW_MULTIPLE},
    {(char *)"reset", 1, CA_WIZARD, TIMECHK_RESET | SW_MULTIPLE},
    {(char *)"screen", 1, CA_WIZARD, TIMECHK_SCREEN | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		toad_sw[] =
{
    {(char *)"no_chown", 1, CA_WIZARD, TOAD_NO_CHOWN | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		trig_sw[] =
{
    {(char *)"quiet", 1, CA_PUBLIC, TRIG_QUIET},
    {(char *)"now", 1, CA_PUBLIC, TRIG_NOW | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		verb_sw[] =
{
    {(char *)"known", 1, CA_PUBLIC, VERB_PRESENT | SW_MULTIPLE},
    {(char *)"move", 1, CA_PUBLIC, VERB_MOVE | SW_MULTIPLE},
    {(char *)"now", 3, CA_PUBLIC, VERB_NOW | SW_MULTIPLE},
    {(char *)"no_name", 3, CA_PUBLIC, VERB_NONAME | SW_MULTIPLE},
    {(char *)"speech", 1, CA_PUBLIC, VERB_SPEECH | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		wall_sw[] =
{
    {(char *)"emit", 1, CA_PUBLIC, SAY_WALLEMIT},
    {(char *)"no_prefix", 1, CA_PUBLIC, SAY_NOTAG | SW_MULTIPLE},
    {(char *)"pose", 1, CA_PUBLIC, SAY_WALLPOSE},
    {(char *)"wizard", 1, CA_PUBLIC, SAY_WIZSHOUT | SW_MULTIPLE},
    {(char *)"admin", 1, CA_ADMIN, SAY_ADMINSHOUT},
    {NULL, 0, 0, 0}
};

NAMETAB		warp_sw[] =
{
    {(char *)"check", 1, CA_WIZARD, TWARP_CLEAN | SW_MULTIPLE},
    {(char *)"dump", 1, CA_WIZARD, TWARP_DUMP | SW_MULTIPLE},
    {(char *)"idle", 1, CA_WIZARD, TWARP_IDLE | SW_MULTIPLE},
    {(char *)"queue", 1, CA_WIZARD, TWARP_QUEUE | SW_MULTIPLE},
    {(char *)"events", 1, CA_WIZARD, TWARP_EVENTS | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		wait_sw[] =
{
    {(char *)"pid", 1, CA_PUBLIC, WAIT_PID | SW_MULTIPLE},
    {(char *)"until", 1, CA_PUBLIC, WAIT_UNTIL | SW_MULTIPLE},
    {NULL, 0, 0, 0}
};

NAMETAB		noeval_sw[] =
{
    {(char *)"noeval", 1, CA_PUBLIC, SW_NOEVAL},
    {NULL, 0, 0, 0}
};

/*
 * ---------------------------------------------------------------------------
 * Command table: Definitions for builtin commands, used to build the command
 * hash table.
 *
 * Format:  Name		Switches	Permissions Needed Key (if any)
 * Calling Seq			Handler
 */

CMDENT		command_table[] =
{
    {
        (char *)"@@", NULL, CA_PUBLIC,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_comment}
    },
    {
        (char *)"@addcommand", addcmd_sw, CA_GOD,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_addcommand}
    },
    {
        (char *)"@admin", NULL, CA_WIZARD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_admin}
    },
    {
        (char *)"@alias", NULL, CA_NO_GUEST | CA_NO_SLAVE,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_alias}
    },
    {
        (char *)"@apply_marked", NULL, CA_WIZARD | CA_GBL_INTERP,
        0, CS_ONE_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_apply_marked}
    },
    {
        (char *)"@attribute", attrib_sw, CA_WIZARD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_attribute}
    },
    {
        (char *)"@boot", boot_sw, CA_NO_GUEST | CA_NO_SLAVE,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_boot}
    },
    {
        (char *)"@chown", chown_sw,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        CHOWN_ONE, CS_TWO_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_chown}
    },
    {
        (char *)"@chownall", chown_sw, CA_WIZARD | CA_GBL_BUILD,
        CHOWN_ALL, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_chownall}
    },
    {
        (char *)"@chzone", chzone_sw,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_chzone}
    },
    {
        (char *)"@clone", clone_sw,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_clone}
    },
    {
        (char *)"@colormap", NULL, CA_PUBLIC,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_colormap}
    },
    {
        (char *)"@cpattr", NULL,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        0, CS_TWO_ARG | CS_ARGV,
        NULL, NULL, NULL, {do_cpattr}
    },
    {
        (char *)"@create", NULL,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_create}
    },
    {
        (char *)"@cron", NULL, CA_NO_SLAVE | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_cron}
    },
    {
        (char *)"@crondel", NULL, CA_NO_SLAVE | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_crondel}
    },
    {
        (char *)"@crontab", NULL, CA_NO_SLAVE | CA_NO_GUEST,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_crontab}
    },
    {
        (char *)"@cut", NULL, CA_WIZARD | CA_LOCATION,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_cut}
    },
    {
        (char *)"@dbck", NULL, CA_WIZARD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_dbck}
    },
    {
        (char *)"@decompile", decomp_sw, CA_PUBLIC,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_decomp}
    },
    {
        (char *)"@delcommand", NULL, CA_GOD,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_delcommand}
    },
    {
        (char *)"@destroy", destroy_sw,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        DEST_ONE, CS_ONE_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_destroy}
    },
    /*
     * {(char *)"@destroyall",	NULL,
     * , DEST_ALL,	CS_ONE_ARG, NULL,		NULL,	NULL,
     * do_destroy}},
     */
    {
        (char *)"@dig", dig_sw,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        0, CS_TWO_ARG | CS_ARGV | CS_INTERP,
        NULL, NULL, NULL, {do_dig}
    },
    {
        (char *)"@disable", NULL, CA_WIZARD,
        GLOB_DISABLE, CS_ONE_ARG,
        NULL, NULL, NULL, {do_global}
    },
    {
        (char *)"@doing", doing_sw, CA_PUBLIC,
        0, CS_ONE_ARG,
        NULL, NULL, NULL, {do_doing}
    },
    {
        (char *)"@dolist", dolist_sw, CA_GBL_INTERP,
        0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_dolist}
    },
    {
        (char *)"@drain", NULL,
        CA_GBL_INTERP | CA_NO_SLAVE | CA_NO_GUEST,
        NFY_DRAIN, CS_TWO_ARG,
        NULL, NULL, NULL, {do_notify}
    },
    {
        (char *)"@dump", dump_sw, CA_WIZARD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_dump}
    },
    {
        (char *)"@edit", NULL, CA_NO_SLAVE | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_ARGV | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_edit}
    },
    {
        (char *)"@emit", emit_sw,
        CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE,
        SAY_EMIT, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"@enable", NULL, CA_WIZARD,
        GLOB_ENABLE, CS_ONE_ARG,
        NULL, NULL, NULL, {do_global}
    },
    {
        (char *)"@end", end_sw, CA_GBL_INTERP,
        0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_end}
    },
    {
        (char *)"@entrances", NULL, CA_NO_GUEST,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_entrances}
    },
    {
        (char *)"@eval", NULL, CA_NO_SLAVE,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_eval}
    },
    {
        (char *)"@femit", femit_sw,
        CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE,
        PEMIT_FEMIT, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"@find", NULL, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_find}
    },
    {
        (char *)"@fixdb", fixdb_sw, CA_GOD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_fixdb}
    },
    {
        (char *)"@floaters", floaters_sw, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_floaters}
    },
    {
        (char *)"@force", force_sw,
        CA_NO_SLAVE | CA_GBL_INTERP | CA_NO_GUEST,
        FRC_COMMAND, CS_TWO_ARG | CS_INTERP | CS_CMDARG,
        NULL, NULL, NULL, {do_force}
    },
    {
        (char *)"@fpose", fpose_sw, CA_LOCATION | CA_NO_SLAVE,
        PEMIT_FPOSE, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"@fsay", fsay_sw, CA_LOCATION | CA_NO_SLAVE,
        PEMIT_FSAY, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"@freelist", NULL, CA_WIZARD,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_freelist}
    },
    {
        (char *)"@function", function_sw, CA_GOD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_function}
    },
    {
        (char *)"@halt", halt_sw, CA_NO_SLAVE,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_halt}
    },
    {
        (char *)"@hashresize", NULL, CA_GOD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_hashresize}
    },
    {
        (char *)"@hook", hook_sw, CA_GOD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_hook}
    },
    {
        (char *)"@include", NULL, CA_GBL_INTERP,
        0, CS_TWO_ARG | CS_ARGV | CS_CMDARG,
        NULL, NULL, NULL, {do_include}
    },
    {
        (char *)"@kick", NULL, CA_WIZARD,
        QUEUE_KICK, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_queue}
    },
    {
        (char *)"@last", NULL, CA_NO_GUEST,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_last}
    },
    {
        (char *)"@link", NULL,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_link}
    },
    {
        (char *)"@list", NULL, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_list}
    },
    {
        (char *)"@listcommands", NULL, CA_GOD,
        0, CS_ONE_ARG,
        NULL, NULL, NULL, {do_listcommands}
    },
    {
        (char *)"@list_file", NULL, CA_WIZARD,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_list_file}
    },
    {
        (char *)"@listmotd", listmotd_sw, CA_PUBLIC,
        MOTD_LIST, CS_ONE_ARG,
        NULL, NULL, NULL, {do_motd}
    },
    {
        (char *)"@lock", lock_sw, CA_NO_SLAVE,
        0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_lock}
    },
    {
        (char *)"@log", NULL, CA_WIZARD,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_logwrite}
    },
    {
        (char *)"@logrotate", NULL, CA_GOD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_logrotate}
    },
    {
        (char *)"@mark", mark_sw, CA_WIZARD,
        SRCH_MARK, CS_ONE_ARG | CS_NOINTERP,
        NULL, NULL, NULL, {do_search}
    },
    {
        (char *)"@mark_all", markall_sw, CA_WIZARD,
        MARK_SET, CS_NO_ARGS,
        NULL, NULL, NULL, {do_markall}
    },
    {
        (char *)"@motd", motd_sw, CA_WIZARD,
        0, CS_ONE_ARG,
        NULL, NULL, NULL, {do_motd}
    },
    {
        (char *)"@mvattr", NULL,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        0, CS_TWO_ARG | CS_ARGV,
        NULL, NULL, NULL, {do_mvattr}
    },
    {
        (char *)"@name", NULL,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_name}
    },
    {
        (char *)"@newpassword", NULL, CA_WIZARD,
        PASS_ANY, CS_TWO_ARG,
        NULL, NULL, NULL, {do_newpassword}
    },
    {
        (char *)"@notify", notify_sw,
        CA_GBL_INTERP | CA_NO_SLAVE | CA_NO_GUEST,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_notify}
    },
    {
        (char *)"@oemit", oemit_sw,
        CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE,
        PEMIT_OEMIT, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"@open", open_sw,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_ARGV | CS_INTERP,
        NULL, NULL, NULL, {do_open}
    },
    {
        (char *)"@parent", NULL,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_FUNCTION,
        NULL, NULL, NULL, {do_parent}
    },
    {
        (char *)"@password", NULL, CA_NO_GUEST,
        PASS_MINE, CS_TWO_ARG,
        NULL, NULL, NULL, {do_password}
    },
    {
        (char *)"@pcreate", NULL, CA_WIZARD | CA_GBL_BUILD,
        PCRE_PLAYER, CS_TWO_ARG,
        NULL, NULL, NULL, {do_pcreate}
    },
    {
        (char *)"@pemit", pemit_sw, CA_NO_GUEST | CA_NO_SLAVE,
        PEMIT_PEMIT, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"@npemit", pemit_sw, CA_NO_GUEST | CA_NO_SLAVE,
        PEMIT_PEMIT, CS_TWO_ARG | CS_UNPARSE | CS_NOSQUISH,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"@poor", NULL, CA_GOD,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_poor}
    },
    {
        (char *)"@power", NULL, CA_PUBLIC,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_power}
    },
    {
        (char *)"@program", NULL, CA_PUBLIC,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_prog}
    },
    {
        (char *)"@ps", ps_sw, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_ps}
    },
    {
        (char *)"@quota", quota_sw, CA_PUBLIC,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_quota}
    },
    {
        (char *)"@quitprogram", NULL, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_quitprog}
    },
    {
        (char *)"@readcache", NULL, CA_WIZARD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_readcache}
    },
    {
        (char *)"@redirect", NULL, CA_PUBLIC,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_redirect}
    },
    {
        (char *)"@reference", reference_sw, CA_PUBLIC,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_reference}
    },
    {
        (char *)"@restart", NULL, CA_WIZARD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_restart}
    },
    {
        (char *)"@robot", NULL,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST | CA_PLAYER,
        PCRE_ROBOT, CS_TWO_ARG,
        NULL, NULL, NULL, {do_pcreate}
    },
    {
        (char *)"@search", NULL, CA_PUBLIC,
        SRCH_SEARCH, CS_ONE_ARG | CS_NOINTERP,
        NULL, NULL, NULL, {do_search}
    },
    {
        (char *)"@set", set_sw,
        CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST,
        0, CS_TWO_ARG,
        NULL, NULL, NULL, {do_set}
    },
    {
        (char *)"@shutdown", shutdown_sw, CA_WIZARD,
        0, CS_ONE_ARG,
        NULL, NULL, NULL, {do_shutdown}
    },
    {
        (char *)"@stats", stats_sw, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_stats}
    },
    {
        (char *)"@startslave", NULL, CA_WIZARD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {boot_slave}
    },
    {
        (char *)"@sweep", sweep_sw, CA_PUBLIC,
        0, CS_ONE_ARG,
        NULL, NULL, NULL, {do_sweep}
    },
    {
        (char *)"@switch", switch_sw, CA_GBL_INTERP,
        0, CS_TWO_ARG | CS_ARGV | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_switch}
    },
    {
        (char *)"@teleport", teleport_sw, CA_NO_GUEST,
        TELEPORT_DEFAULT, CS_TWO_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_teleport}
    },
    {
        (char *)"@timecheck", timecheck_sw, CA_WIZARD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_timecheck}
    },
    {
        (char *)"@timewarp", warp_sw, CA_WIZARD,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_timewarp}
    },
    {
        (char *)"@toad", toad_sw, CA_WIZARD,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_toad}
    },
    {
        (char *)"@trigger", trig_sw, CA_GBL_INTERP,
        0, CS_TWO_ARG | CS_ARGV,
        NULL, NULL, NULL, {do_trigger}
    },
    {
        (char *)"@unlink", NULL, CA_NO_SLAVE | CA_GBL_BUILD,
        0, CS_ONE_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_unlink}
    },
    {
        (char *)"@unlock", lock_sw, CA_NO_SLAVE,
        0, CS_ONE_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_unlock}
    },
    {
        (char *)"@verb", verb_sw, CA_GBL_INTERP | CA_NO_SLAVE,
        0, CS_TWO_ARG | CS_ARGV | CS_INTERP | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_verb}
    },
    {
        (char *)"@wait", wait_sw, CA_GBL_INTERP,
        0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
        NULL, NULL, NULL, {do_wait}
    },
    {
        (char *)"@wall", wall_sw, CA_PUBLIC,
        SAY_SHOUT, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"@wipe", NULL,
        CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD,
        0, CS_ONE_ARG | CS_INTERP | CS_FUNCTION,
        NULL, NULL, NULL, {do_wipe}
    },
    {
        (char *)"drop", drop_sw,
        CA_NO_SLAVE | CA_CONTENTS | CA_LOCATION | CA_NO_GUEST,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_drop}
    },
    {
        (char *)"enter", enter_sw, CA_LOCATION,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_enter}
    },
    {
        (char *)"examine", examine_sw, CA_PUBLIC,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_examine}
    },
    {
        (char *)"get", get_sw, CA_LOCATION | CA_NO_GUEST,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_get}
    },
    {
        (char *)"give", give_sw, CA_LOCATION | CA_NO_GUEST,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_give}
    },
    {
        (char *)"goto", goto_sw, CA_LOCATION,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_move}
    },
    {
        (char *)"internalgoto", NULL, CA_GOD,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_comment}
    },
    {
        (char *)"inventory", NULL, CA_PUBLIC,
        LOOK_INVENTORY, CS_NO_ARGS,
        NULL, NULL, NULL, {do_inventory}
    },
    {
        (char *)"kill", NULL, CA_NO_GUEST | CA_NO_SLAVE,
        KILL_KILL, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_kill}
    },
    {
        (char *)"leave", leave_sw, CA_LOCATION,
        0, CS_NO_ARGS | CS_INTERP,
        NULL, NULL, NULL, {do_leave}
    },
    {
        (char *)"look", look_sw, CA_LOCATION,
        LOOK_LOOK, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_look}
    },
    {
        (char *)"page", noeval_sw, CA_NO_SLAVE,
        0, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_page}
    },
    {
        (char *)"pose", pose_sw, CA_LOCATION | CA_NO_SLAVE,
        SAY_POSE, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"reply", noeval_sw, CA_NO_SLAVE,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_reply_page}
    },
    {
        (char *)"say", noeval_sw, CA_LOCATION | CA_NO_SLAVE,
        SAY_SAY, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"score", NULL, CA_PUBLIC,
        LOOK_SCORE, CS_NO_ARGS,
        NULL, NULL, NULL, {do_score}
    },
    {
        (char *)"slay", NULL, CA_WIZARD,
        KILL_SLAY, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_kill}
    },
    {
        (char *)"think", NULL, CA_NO_SLAVE,
        0, CS_ONE_ARG,
        NULL, NULL, NULL, {do_think}
    },
    {
        (char *)"use", NULL, CA_NO_SLAVE | CA_GBL_INTERP,
        0, CS_ONE_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_use}
    },
    {
        (char *)"version", NULL, CA_PUBLIC,
        0, CS_NO_ARGS,
        NULL, NULL, NULL, {do_version}
    },
    {
        (char *)"whisper", NULL, CA_LOCATION | CA_NO_SLAVE,
        PEMIT_WHISPER, CS_TWO_ARG | CS_INTERP,
        NULL, NULL, NULL, {do_pemit}
    },
    {
        (char *)"doing", NULL, CA_PUBLIC,
        CMD_DOING, CS_ONE_ARG,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"quit", NULL, CA_PUBLIC,
        CMD_QUIT, CS_NO_ARGS,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"logout", NULL, CA_PUBLIC,
        CMD_LOGOUT, CS_NO_ARGS,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"who", NULL, CA_PUBLIC,
        CMD_WHO, CS_ONE_ARG,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"session", NULL, CA_PUBLIC,
        CMD_SESSION, CS_ONE_ARG,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"info", NULL, CA_PUBLIC,
        CMD_INFO, CS_NO_ARGS,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"outputprefix", NULL, CA_PUBLIC,
        CMD_PREFIX, CS_ONE_ARG,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"outputsuffix", NULL, CA_PUBLIC,
        CMD_SUFFIX, CS_ONE_ARG,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"puebloclient", NULL, CA_PUBLIC,
        CMD_PUEBLOCLIENT, CS_ONE_ARG,
        NULL, NULL, NULL, {logged_out}
    },
    {
        (char *)"\\", NULL,
        CA_NO_GUEST | CA_LOCATION | CF_DARK | CA_NO_SLAVE,
        SAY_PREFIX | SAY_EMIT, CS_ONE_ARG | CS_INTERP | CS_LEADIN,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"#", NULL,
        CA_NO_SLAVE | CA_GBL_INTERP | CF_DARK,
        0, CS_ONE_ARG | CS_INTERP | CS_CMDARG | CS_LEADIN,
        NULL, NULL, NULL, {do_force_prefixed}
    },
    {
        (char *)":", NULL,
        CA_LOCATION | CF_DARK | CA_NO_SLAVE,
        SAY_PREFIX | SAY_POSE, CS_ONE_ARG | CS_INTERP | CS_LEADIN,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)";", NULL,
        CA_LOCATION | CF_DARK | CA_NO_SLAVE,
        SAY_PREFIX | SAY_POSE_NOSPC, CS_ONE_ARG | CS_INTERP | CS_LEADIN,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"\"", NULL,
        CA_LOCATION | CF_DARK | CA_NO_SLAVE,
        SAY_PREFIX | SAY_SAY, CS_ONE_ARG | CS_INTERP | CS_LEADIN,
        NULL, NULL, NULL, {do_say}
    },
    {
        (char *)"&", NULL,
        CA_NO_GUEST | CA_NO_SLAVE | CF_DARK,
        0, CS_TWO_ARG | CS_LEADIN,
        NULL, NULL, NULL, {do_setvattr}
    },
    {
        (char *)NULL, NULL, 0,
        0, 0,
        NULL, NULL, NULL, {NULL}
    }
};

/*
 * ---------------------------------------------------------------------------
 * Command, function, etc. access name table.
 */

NAMETAB		access_nametab[] =
{
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
    {NULL, 0, 0, 0}
};

/*
 * ---------------------------------------------------------------------------
 * Attribute access name tables.
 */

NAMETAB		attraccess_nametab[] =
{
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
    {NULL, 0, 0, 0}
};

NAMETAB		indiv_attraccess_nametab[] =
{
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
    {NULL, 0, 0, 0}
};

/* *INDENT-ON* */
