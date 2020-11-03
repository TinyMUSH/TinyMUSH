#include "copyright.h"

#ifndef __PLAYER_H
#define __PLAYER_H

#define NUM_GOOD 4 /* # of successful logins to save data for */
#define NUM_BAD 3  /* # of failed logins to save data for */

typedef struct hostdtm HOSTDTM;

struct hostdtm
{
    char *host;
    char *dtm;
};

typedef struct logindata LDATA;

struct logindata
{
    HOSTDTM good[NUM_GOOD];
    HOSTDTM bad[NUM_BAD];
    int tot_good;
    int tot_bad;
    int new_bad;
};

#endif /* __PLAYER_H */
