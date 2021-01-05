/**
 * @file walkdb.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Structures used in commands that walk the entire db
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __WALKDB_H
#define __WALKDB_H

/** 
 * Search structure, used by @search and search(). 
 * 
 */
typedef struct search_type
{
    int s_wizard;
    dbref s_owner;
    dbref s_rst_owner;
    int s_rst_type;
    FLAGSET s_fset;
    POWERSET s_pset;
    dbref s_parent;
    dbref s_zone;
    char *s_rst_name;
    char *s_rst_eval;
    char *s_rst_ufuntxt;
    int low_bound;
    int high_bound;
} SEARCH;

/** 
 * Stats structure, used by @stats and stats(). 
 * 
 */
typedef struct stats_type
{
    int s_total;
    int s_rooms;
    int s_exits;
    int s_things;
    int s_players;
    int s_going;
    int s_garbage;
    int s_unknown;
} STATS;

#endif /* __WALKDB_H */
