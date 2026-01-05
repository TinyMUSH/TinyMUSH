/**
 * @file udb_backend.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Database backend dispatcher - delegates operations to GDBM or LMDB
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "udb_backend.h"

#include <stdbool.h>
#include <sys/file.h>
#include <unistd.h>

/* Current active backend (chosen at compile time) */
#if defined(USE_LMDB) && !defined(USE_GDBM)
db_backend_t *current_backend = &lmdb_backend;
#elif defined(USE_GDBM) && !defined(USE_LMDB)
db_backend_t *current_backend = &gdbm_backend;
#else
#error "Exactly one database backend must be enabled (USE_LMDB xor USE_GDBM)"
#endif

/* File lock structure for GDBM backend */
static struct flock fl;

/**
 * @brief Configure sync mode
 * @param flag Sync mode flag
 */
void dddb_setsync(int flag)
{
    if (current_backend && current_backend->setsync) {
        current_backend->setsync(flag);
    }
}

/**
 * @brief Optimize/reorganize database
 * @return 0 on success, -1 on error
 */
int dddb_optimize(void)
{
    if (current_backend && current_backend->optimize) {
        return current_backend->optimize();
    }
    return -1;
}

/**
 * @brief Initialize database
 * @return 0 on success, -1 on error
 */
int dddb_init(void)
{
    if (current_backend && current_backend->init) {
        return current_backend->init();
    }
    return -1;
}

/**
 * @brief Set database filename
 * @param fil Filename
 * @return 0 on success, -1 on error
 */
int dddb_setfile(char *fil)
{
    if (current_backend && current_backend->setfile) {
        return current_backend->setfile(fil);
    }
    return -1;
}

/**
 * @brief Close database
 * @return true on success, false on error
 */
bool dddb_close(void)
{
    if (current_backend && current_backend->close) {
        return current_backend->close();
    }
    return false;
}

/**
 * @brief Fetch record from database
 * @param gamekey Key to fetch
 * @param type Record type
 * @return Data structure with record, or empty on failure
 */
UDB_DATA db_get(UDB_DATA gamekey, unsigned int type)
{
    if (current_backend && current_backend->get) {
        return current_backend->get(gamekey, type);
    }
    
    /* Return empty result on failure */
    UDB_DATA empty = {NULL, 0};
    return empty;
}

/**
 * @brief Store record in database
 * @param gamekey Key to store
 * @param gamedata Data to store
 * @param type Record type
 * @return 0 on success, -1 on error
 */
int db_put(UDB_DATA gamekey, UDB_DATA gamedata, unsigned int type)
{
    if (current_backend && current_backend->put) {
        return current_backend->put(gamekey, gamedata, type);
    }
    return -1;
}

/**
 * @brief Delete record from database
 * @param gamekey Key to delete
 * @param type Record type
 * @return 0 on success, -1 on error
 */
int db_del(UDB_DATA gamekey, unsigned int type)
{
    if (current_backend && current_backend->del) {
        return current_backend->del(gamekey, type);
    }
    return -1;
}

/**
 * @brief Lock the database file
 * 
 * Used primarily by GDBM backend for manual locking.
 * LMDB uses transactions and doesn't need this.
 */
void db_lock(void)
{
    if (mushstate.dbm_fd == -1)
    {
        return;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (fcntl(mushstate.dbm_fd, F_SETLKW, &fl) == -1)
    {
        log_perror("DMP", "LOCK", NULL, "fcntl()");
        return;
    }
}

/**
 * @brief Unlock the database file
 * 
 * Used primarily by GDBM backend for manual locking.
 * LMDB uses transactions and doesn't need this.
 */
void db_unlock(void)
{
    if (mushstate.dbm_fd == -1)
    {
        return;
    }

    fl.l_type = F_UNLCK;

    if (fcntl(mushstate.dbm_fd, F_SETLK, &fl) == -1)
    {
        log_perror("DMP", "LOCK", NULL, "fcntl()");
        return;
    }
}
