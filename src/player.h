/**
 * @file player.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Player handing and processing definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __PLAYER_H
#define __PLAYER_H

#define NUM_GOOD 4 /*!< # of successful logins to save data for */
#define NUM_BAD 3  /*!< # of failed logins to save data for */

typedef struct hostdtm
{
    char *host;
    char *dtm;
} HOSTDTM;

typedef struct logindata
{
    HOSTDTM good[NUM_GOOD];
    HOSTDTM bad[NUM_BAD];
    int tot_good;
    int tot_bad;
    int new_bad;
} LDATA;

typedef struct player_cache
{
    dbref player;
    int money;
    int queue;
    int qmax;
    int cflags;
    struct player_cache *next;
} PCACHE;

#endif /* __PLAYER_H */
