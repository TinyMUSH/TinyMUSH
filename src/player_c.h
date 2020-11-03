#include "copyright.h"

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
