/**
 * @file player_c.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Player cache definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __PLAYER_C_H
#define __PLAYER_C_H

typedef struct player_cache
{
    dbref player;
    int money;
    int queue;
    int qmax;
    int cflags;
    struct player_cache *next;
} PCACHE;

#endif /* __PLAYER_C_H */
