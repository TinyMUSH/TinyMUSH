/**
 * @file externs.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Shared extern declarations and globals used across the game engine
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#pragma once

extern char *log_pos;

extern OBJ *db;          /*!< struct database */
extern NAME *names;      /*!< Name buffer */
extern NAME *purenames;  /*!< Pure Name Buffer */
extern int anum_alc_top; /*!< Top of attr num lookup table */

extern OBJENT object_types[8]; /*!< Object types flags */

extern const Delim SPACE_DELIM;
extern OBJXFUNCS xfunctions;
extern FUN flist[];

extern DESC *descriptor_list;
extern int sock;
extern int maxd;
extern int ndescriptors;
extern int msgq_Id;

extern CONFDATA mushconf;
extern STATEDATA mushstate;

/**
 * @brief Name Tables
 *
 */
extern NAMETAB addcmd_sw[];
extern NAMETAB attrib_sw[];
extern NAMETAB boot_sw[];
extern NAMETAB chown_sw[];
extern NAMETAB chzone_sw[];
extern NAMETAB clone_sw[];
extern NAMETAB decomp_sw[];
extern NAMETAB destroy_sw[];
extern NAMETAB dig_sw[];
extern NAMETAB doing_sw[];
extern NAMETAB dolist_sw[];
extern NAMETAB drop_sw[];
extern NAMETAB dump_sw[];
extern NAMETAB emit_sw[];
extern NAMETAB end_sw[];
extern NAMETAB enter_sw[];
extern NAMETAB examine_sw[];
extern NAMETAB femit_sw[];
extern NAMETAB fixdb_sw[];
extern NAMETAB floaters_sw[];
extern NAMETAB force_sw[];
extern NAMETAB fpose_sw[];
extern NAMETAB fsay_sw[];
extern NAMETAB function_sw[];
extern NAMETAB get_sw[];
extern NAMETAB give_sw[];
extern NAMETAB goto_sw[];
extern NAMETAB halt_sw[];
extern NAMETAB help_sw[];
extern NAMETAB hook_sw[];
extern NAMETAB leave_sw[];
extern NAMETAB listmotd_sw[];
extern NAMETAB lock_sw[];
extern NAMETAB look_sw[];
extern NAMETAB mark_sw[];
extern NAMETAB markall_sw[];
extern NAMETAB motd_sw[];
extern NAMETAB notify_sw[];
extern NAMETAB oemit_sw[];
extern NAMETAB open_sw[];
extern NAMETAB pemit_sw[];
extern NAMETAB pose_sw[];
extern NAMETAB ps_sw[];
extern NAMETAB quota_sw[];
extern NAMETAB reference_sw[];
extern NAMETAB set_sw[];
extern NAMETAB shutdown_sw[];
extern NAMETAB stats_sw[];
extern NAMETAB sweep_sw[];
extern NAMETAB switch_sw[];
extern NAMETAB teleport_sw[];
extern NAMETAB timecheck_sw[];
extern NAMETAB toad_sw[];
extern NAMETAB trig_sw[];
extern NAMETAB verb_sw[];
extern NAMETAB wall_sw[];
extern NAMETAB warp_sw[];
extern NAMETAB wait_sw[];
extern NAMETAB noeval_sw[];
extern NAMETAB access_nametab[];
extern NAMETAB attraccess_nametab[];
extern NAMETAB indiv_attraccess_nametab[];
extern NAMETAB list_names[];
extern NAMETAB bool_names[];
extern NAMETAB list_files[];
extern NAMETAB logdata_nametab[];
extern NAMETAB logoptions_nametab[];
extern NAMETAB enable_names[];
extern NAMETAB sigactions_nametab[];
extern NAMETAB logout_cmdtable[];

extern CMDENT command_table[];    /*!< Command Tables */
extern CONF conftable[];          /*!< Config Tables */
extern LOGFILETAB logfds_table[]; /*!< Logfile Tables */
extern ATTR attr[];               /*!< List of built-in attributes*/


/**
 */

