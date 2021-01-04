/**
 * @file api.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Functions called only by modules - must be included by all dynamically loaded modules
 * @version 3.3
 * @date 2020-12-24
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"

#include <ltdl.h>

#ifndef __API_H
#define __API_H

#define SIZE_HACK 1

/**
 * @bug Need to be convert into a function in api.c.
 * 
 */
#define DB_GROW_MODULE(old_db, new_size, new_top, proto)                                                                  \
    {                                                                                                                     \
        char *dg__cp;                                                                                                     \
        proto *dg__new_db;                                                                                                \
        int dg__i;                                                                                                        \
        dg__new_db = (proto *)XMALLOC((new_size + SIZE_HACK) * sizeof(proto), "db_grow.mod_db");                          \
        if (!dg__new_db)                                                                                                  \
        {                                                                                                                 \
            log_write(LOG_ALWAYS, "ALC", "DB", "Could not allocate space for %d item module struct database.", new_size); \
            abort();                                                                                                      \
        }                                                                                                                 \
        if (old_db)                                                                                                       \
        {                                                                                                                 \
            old_db -= SIZE_HACK;                                                                                          \
            memcpy((char *)dg__new_db, (char *)old_db,                                                                    \
                   (mudstate.db_top + SIZE_HACK) * sizeof(proto));                                                        \
            dg__cp = (char *)old_db;                                                                                      \
            XFREE(dg__cp);                                                                                                \
        }                                                                                                                 \
        else                                                                                                              \
        {                                                                                                                 \
            old_db = dg__new_db;                                                                                          \
            for (dg__i = 0; dg__i < SIZE_HACK; dg__i++)                                                                   \
            {                           /* 'NOTHING' fill */                                                              \
                OBJ_INIT_MODULE(dg__i); /* define macro in module file */                                                 \
            }                                                                                                             \
        }                                                                                                                 \
        old_db = dg__new_db + SIZE_HACK;                                                                                  \
        for (dg__i = mudstate.db_top; dg__i < new_size; dg__i++)                                                          \
        {                                                                                                                 \
            OBJ_INIT_MODULE(dg__i);                                                                                       \
        }                                                                                                                 \
    }

#endif /* __API_H */
