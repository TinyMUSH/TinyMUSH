/**
 * @file udb_lmdb.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief LMDB database backend implementation
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
#include <limits.h>
#include <lmdb.h>
#include <sys/stat.h>

/* LMDB-specific state */
static char *lmdb_dbfile = DEFAULT_DBMCHUNKFILE;
static int lmdb_db_initted = 0;
static MDB_env *lmdb_env = NULL;
static MDB_dbi lmdb_dbi = 0;

/* LMDB default map size: 1 GB, expandable */
#define LMDB_DEFAULT_MAPSIZE (1UL * 1024UL * 1024UL * 1024UL)

static void lmdb_setsync(int flag)
{
    unsigned int env_flags = 0;
    int rc;
    
    if (!lmdb_env) {
        return;
    }
    
    /* Get current flags */
    rc = mdb_env_get_flags(lmdb_env, &env_flags);
    if (rc != 0) {
        warning("lmdb_setsync: cannot get env flags: %s", mdb_strerror(rc));
        return;
    }
    
    /* Toggle MDB_NOSYNC based on flag */
    if (flag == 0) {
        env_flags |= MDB_NOSYNC;  /* Async mode */
    } else {
        env_flags &= ~MDB_NOSYNC; /* Sync mode */
    }
    
    /* Note: mdb_env_set_flags can only change certain flags after open */
    rc = mdb_env_set_flags(lmdb_env, MDB_NOSYNC, flag == 0);
    if (rc != 0) {
        warning("lmdb_setsync: cannot set sync mode: %s", mdb_strerror(rc));
    } else {
        log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: set sync mode to %s on %s.", 
                  flag ? "sync" : "async", lmdb_dbfile);
    }
}

static int lmdb_optimize(void)
{
    /* LMDB doesn't require manual optimization like GDBM */
    /* It automatically manages space and coalesces pages */
    log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: optimization not required (automatic)");
    return 0;
}

static int lmdb_init(void)
{
    char *tmpfile;
    char *tmpdir;
    int rc;
    MDB_txn *txn = NULL;
    struct stat st;

    if (!mushstate.standalone)
    {
        tmpfile = XASPRINTF("tmpfile", "%s/%s", mushconf.dbhome, lmdb_dbfile);
    }
    else
    {
        tmpfile = XSTRDUP(lmdb_dbfile, "tmpfile");
    }

    /* LMDB needs a directory, not a file */
    tmpdir = XASPRINTF("tmpdir", "%s.lmdb", tmpfile);
    
    /* Create directory if it doesn't exist */
    if (stat(tmpdir, &st) != 0) {
        if (mkdir(tmpdir, 0700) != 0) {
            warning("lmdb_init: cannot create directory %s", tmpdir);
            XFREE(tmpfile);
            XFREE(tmpdir);
            return (1);
        }
    }
    
    log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: opening %s", tmpdir);

    /* Create LMDB environment */
    rc = mdb_env_create(&lmdb_env);
    if (rc != 0) {
        warning("lmdb_init: mdb_env_create failed: %s", mdb_strerror(rc));
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    /* Set map size (1 GB default, can grow) */
    rc = mdb_env_set_mapsize(lmdb_env, LMDB_DEFAULT_MAPSIZE);
    if (rc != 0) {
        warning("lmdb_init: mdb_env_set_mapsize failed: %s", mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    /* Set max databases (we only use 1, but allow expansion) */
    rc = mdb_env_set_maxdbs(lmdb_env, 1);
    if (rc != 0) {
        warning("lmdb_init: mdb_env_set_maxdbs failed: %s", mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    /* Open environment - use MDB_NOSYNC for standalone mode. LMDB expects a directory. */
    unsigned int env_flags = 0;
    if (mushstate.standalone) {
        env_flags |= MDB_NOSYNC;
    }
    
    rc = mdb_env_open(lmdb_env, tmpdir, env_flags, 0600);
    if (rc != 0) {
        warning("lmdb_init: mdb_env_open failed on %s: %s", tmpdir, mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    /* Open main database */
    rc = mdb_txn_begin(lmdb_env, NULL, 0, &txn);
    if (rc != 0) {
        warning("lmdb_init: mdb_txn_begin failed: %s", mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    rc = mdb_dbi_open(txn, NULL, MDB_CREATE, &lmdb_dbi);
    if (rc != 0) {
        warning("lmdb_init: mdb_dbi_open failed: %s", mdb_strerror(rc));
        mdb_txn_abort(txn);
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        warning("lmdb_init: mdb_txn_commit failed: %s", mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    /* LMDB doesn't use file descriptors in the same way as GDBM */
    mushstate.dbm_fd = -1;
    lmdb_db_initted = 1;
    
    XFREE(tmpfile);
    XFREE(tmpdir);
    return (0);
}

static int lmdb_setfile(char *fil)
{
    char *xp;

    if (lmdb_db_initted)
    {
        return (1);
    }

    xp = XSTRDUP(fil, "xp");
    if (xp == (char *)0)
    {
        return (1);
    }

    lmdb_dbfile = xp;
    return (0);
}

static bool lmdb_close(void)
{
    log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: closing %s", lmdb_dbfile);

    if (lmdb_env != NULL)
    {
        /* LMDB automatically syncs on close */
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
    }

    lmdb_db_initted = 0;
    return true;
}

static UDB_DATA lmdb_get(UDB_DATA gamekey, unsigned int type)
{
    UDB_DATA gamedata;
    MDB_txn *txn = NULL;
    MDB_val key, data;
    char *keybuf;
    size_t keylen;
    int rc;

    gamedata.dptr = NULL;
    gamedata.dsize = 0;

    if (!lmdb_db_initted || !gamekey.dptr || gamekey.dsize < 0)
    {
        return gamedata;
    }

    keylen = (size_t)gamekey.dsize + sizeof(unsigned int);
    if (keylen > INT_MAX)
    {
        return gamedata;
    }

    /* Construct composite key */
    keybuf = (char *)XMALLOC(keylen, "keybuf");
    XMEMCPY((void *)keybuf, gamekey.dptr, gamekey.dsize);
    XMEMCPY((void *)(keybuf + gamekey.dsize), (void *)&type, sizeof(unsigned int));

    key.mv_data = keybuf;
    key.mv_size = keylen;

    /* Start read-only transaction */
    rc = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn);
    if (rc != 0) {
        warning("lmdb_get: mdb_txn_begin failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return gamedata;
    }

    /* Fetch data */
    rc = mdb_get(txn, lmdb_dbi, &key, &data);
    if (rc == 0) {
        /* Found - copy data (caller must free) */
        gamedata.dptr = (char *)XMALLOC(data.mv_size, "gamedata.dptr");
        XMEMCPY(gamedata.dptr, data.mv_data, data.mv_size);
        gamedata.dsize = (int)data.mv_size;
    } else if (rc != MDB_NOTFOUND) {
        warning("lmdb_get: mdb_get failed: %s", mdb_strerror(rc));
    }

    mdb_txn_abort(txn);  /* Read-only, so abort is fine */
    XFREE(keybuf);
    return gamedata;
}

static int lmdb_put(UDB_DATA gamekey, UDB_DATA gamedata, unsigned int type)
{
    MDB_txn *txn = NULL;
    MDB_val key, data;
    char *keybuf;
    size_t keylen;
    int rc;

    if (!lmdb_db_initted || !gamekey.dptr || gamekey.dsize < 0)
    {
        return (1);
    }

    keylen = (size_t)gamekey.dsize + sizeof(unsigned int);
    if (keylen > INT_MAX)
    {
        return (1);
    }

    /* Construct composite key */
    keybuf = (char *)XMALLOC(keylen, "keybuf");
    XMEMCPY((void *)keybuf, gamekey.dptr, gamekey.dsize);
    XMEMCPY((void *)(keybuf + gamekey.dsize), (void *)&type, sizeof(unsigned int));

    key.mv_data = keybuf;
    key.mv_size = keylen;
    data.mv_data = gamedata.dptr;
    data.mv_size = gamedata.dsize;

    /* Start write transaction */
    rc = mdb_txn_begin(lmdb_env, NULL, 0, &txn);
    if (rc != 0) {
        warning("lmdb_put: mdb_txn_begin failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return (1);
    }

    /* Store data (MDB_NOOVERWRITE = 0 means replace) */
    rc = mdb_put(txn, lmdb_dbi, &key, &data, 0);
    if (rc != 0) {
        warning("lmdb_put: mdb_put failed: %s", mdb_strerror(rc));
        mdb_txn_abort(txn);
        XFREE(keybuf);
        return (1);
    }

    /* Commit transaction */
    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        warning("lmdb_put: mdb_txn_commit failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return (1);
    }

    XFREE(keybuf);
    return (0);
}

static int lmdb_del(UDB_DATA gamekey, unsigned int type)
{
    MDB_txn *txn = NULL;
    MDB_val key;
    char *keybuf;
    size_t keylen;
    int rc;

    if (!lmdb_db_initted || !gamekey.dptr || gamekey.dsize < 0)
    {
        return (-1);
    }

    keylen = (size_t)gamekey.dsize + sizeof(unsigned int);
    if (keylen > INT_MAX)
    {
        return (1);
    }

    /* Construct composite key */
    keybuf = (char *)XMALLOC(keylen, "keybuf");
    XMEMCPY((void *)keybuf, gamekey.dptr, gamekey.dsize);
    XMEMCPY((void *)(keybuf + gamekey.dsize), (void *)&type, sizeof(unsigned int));

    key.mv_data = keybuf;
    key.mv_size = keylen;

    /* Start write transaction */
    rc = mdb_txn_begin(lmdb_env, NULL, 0, &txn);
    if (rc != 0) {
        warning("lmdb_del: mdb_txn_begin failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return (1);
    }

    /* Delete record */
    rc = mdb_del(txn, lmdb_dbi, &key, NULL);
    if (rc != 0 && rc != MDB_NOTFOUND) {
        warning("lmdb_del: mdb_del failed: %s", mdb_strerror(rc));
        mdb_txn_abort(txn);
        XFREE(keybuf);
        return (1);
    }

    /* Commit transaction */
    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        warning("lmdb_del: mdb_txn_commit failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return (1);
    }

    XFREE(keybuf);
    return (0);
}

/* LMDB backend structure */
db_backend_t lmdb_backend = {
    .name = "LMDB",
    .setsync = lmdb_setsync,
    .init = lmdb_init,
    .setfile = lmdb_setfile,
    .close = lmdb_close,
    .optimize = lmdb_optimize,
    .get = lmdb_get,
    .put = lmdb_put,
    .del = lmdb_del,
    .private_data = NULL
};
