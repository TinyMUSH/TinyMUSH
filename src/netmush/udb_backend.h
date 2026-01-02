/**
 * @file udb_backend.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Abstract database backend interface for GDBM and LMDB
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#ifndef __UDB_BACKEND_H
#define __UDB_BACKEND_H

#include "typedefs.h"
#include <stdbool.h>

/**
 * @brief Abstract database backend operations
 * 
 * This structure defines the interface that both GDBM and LMDB backends
 * must implement. It allows runtime selection of the database engine.
 */
typedef struct db_backend {
    const char *name;                                                   /**< Backend name (e.g., "GDBM", "LMDB") */
    
    /* Initialization and configuration */
    void (*setsync)(int flag);                                          /**< Configure sync mode */
    int (*init)(void);                                                  /**< Initialize database */
    int (*setfile)(char *fil);                                          /**< Set database filename */
    bool (*close)(void);                                                /**< Close database */
    int (*optimize)(void);                                              /**< Optimize/reorganize database */
    
    /* Core operations */
    UDB_DATA (*get)(UDB_DATA gamekey, unsigned int type);              /**< Fetch record */
    int (*put)(UDB_DATA gamekey, UDB_DATA gamedata, unsigned int type); /**< Store record */
    int (*del)(UDB_DATA gamekey, unsigned int type);                   /**< Delete record */
    
    /* Backend-specific data */
    void *private_data;                                                 /**< Backend-specific state */
} db_backend_t;

/* Backend selection */
extern db_backend_t *current_backend;

/* Available backends */
#ifdef USE_GDBM
extern db_backend_t gdbm_backend;
#endif

#ifdef USE_LMDB
extern db_backend_t lmdb_backend;
#endif

/* Wrapper functions that delegate to current_backend */
void dddb_setsync(int flag);
int dddb_optimize(void);
int dddb_init(void);
int dddb_setfile(char *fil);
bool dddb_close(void);
UDB_DATA db_get(UDB_DATA gamekey, unsigned int type);
int db_put(UDB_DATA gamekey, UDB_DATA gamedata, unsigned int type);
int db_del(UDB_DATA gamekey, unsigned int type);

#endif /* __UDB_BACKEND_H */
