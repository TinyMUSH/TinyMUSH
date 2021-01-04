/**
 * @file nametabs.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Nametables definitions
 * @version 3.3
 * @date 2020-12-25
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */
#include "copyright.h"

#ifndef __NAMETABS_H
#define __NAMETABS_H

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

/**
 * @brief Command Tables
 * 
 */

extern CMDENT command_table[];

/**
 * @brief Config Tables
 * 
 */
extern CONF conftable[];

/**
 * @brief Logfile Tables
 * 
 */
extern LOGFILETAB logfds_table[];

/**
 * @brief List of built-in attributes
 *
 */
extern ATTR attr[];

#endif /* __NAMETABS_H */
