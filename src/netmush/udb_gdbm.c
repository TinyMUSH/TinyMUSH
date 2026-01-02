/**
 * @file udb_gdbm.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief GDBM database backend implementation
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
#include <gdbm.h>
#include <sys/file.h>
#include <unistd.h>

/* GDBM-specific state */
static char *gdbm_dbfile = DEFAULT_DBMCHUNKFILE;
static int gdbm_db_initted = 0;
static GDBM_FILE gdbm_dbp = (GDBM_FILE)0;

static void mushgdbm_error_handler(const char *msg)
{
    log_write(LOG_ALWAYS, "DB", "ERROR", "GDBM error: %s\n", msg);
}

static void gdbm_setsync(int flag)
{
    if (gdbm_setopt(gdbm_dbp, GDBM_SYNCMODE, &flag, sizeof(int)) == -1)
    {
        warning("gdbm_setsync: cannot set GDBM_SYNCMODE to %d on %s. GDBM Error %s", flag, gdbm_dbfile, gdbm_strerror(gdbm_errno));
    }
    else
    {
        log_write(LOG_ALWAYS, "DB", "INFO", "GDBM: set GDBM_SYNCMODE to %d on %s.", flag, gdbm_dbfile);
    }
}

static int gdbm_optimize(void)
{
    int rc;
    log_write(LOG_ALWAYS, "DB", "INFO", "GDBM: optimizing %s", gdbm_dbfile);

    db_lock();
    rc = gdbm_reorganize(gdbm_dbp);
    if (rc == 0) {
        (void)gdbm_sync(gdbm_dbp);
    }
    db_unlock();
    
    return rc;
}

static int gdbm_init(void)
{
    char *tmpfile;
    int i;

    if (!mushstate.standalone)
    {
        tmpfile = XASPRINTF("tmpfile", "%s/%s", mushconf.dbhome, gdbm_dbfile);
    }
    else
    {
        tmpfile = XSTRDUP(gdbm_dbfile, "tmpfile");
    }

    log_write(LOG_ALWAYS, "DB", "INFO", "GDBM: opening %s", tmpfile);

    if ((gdbm_dbp = gdbm_open(tmpfile, mushstate.db_block_size, GDBM_WRCREAT | GDBM_SYNC | GDBM_NOLOCK, 0600, mushgdbm_error_handler)) == (GDBM_FILE)0)
    {
        warning("gdbm_init: cannot open %s. GDBM Error %s", tmpfile, gdbm_strerror(gdbm_errno));
        XFREE(tmpfile);
        return (1);
    }
    XFREE(tmpfile);

    /* Set cache size */
    i = mushstate.standalone ? 400 : 2;
    if (gdbm_setopt(gdbm_dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1)
    {
        warning("gdbm_init: cannot set cache size to %d on %s. GDBM Error %s", i, gdbm_dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /* Set GDBM to manage a global free space table */
    i = 1;
    if (gdbm_setopt(gdbm_dbp, GDBM_CENTFREE, &i, sizeof(int)) == -1)
    {
        warning("gdbm_init: cannot set GDBM_CENTFREE to %d on %s. GDBM Error %s", i, gdbm_dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /* Set GDBM to coalesce free blocks */
    i = 1;
    if (gdbm_setopt(gdbm_dbp, GDBM_COALESCEBLKS, &i, sizeof(int)) == -1)
    {
        warning("gdbm_init: cannot set GDBM_COALESCEBLKS to %d on %s. GDBM Error %s", i, gdbm_dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /* If standalone, run non-synchronous */
    if (mushstate.standalone)
    {
        gdbm_setsync(0);
    }

    mushstate.dbm_fd = gdbm_fdesc(gdbm_dbp);
    gdbm_db_initted = 1;
    return (0);
}

static int gdbm_setfile(char *fil)
{
    char *xp;

    if (gdbm_db_initted)
    {
        return (1);
    }

    xp = XSTRDUP(fil, "xp");
    if (xp == (char *)0)
    {
        return (1);
    }

    gdbm_dbfile = xp;
    return (0);
}

static bool mushgdbm_close(void)
{
    log_write(LOG_ALWAYS, "DB", "INFO", "GDBM: closing %s", gdbm_dbfile);

    if (gdbm_dbp != (GDBM_FILE)0)
    {
        if (gdbm_sync(gdbm_dbp) == -1)
        {
            warning("mushgdbm_close: gdbm_sync error on %s. GDBM Error %s", gdbm_dbfile, gdbm_strerror(gdbm_errno));
            return false;
        }
        gdbm_close(gdbm_dbp);
        gdbm_dbp = (GDBM_FILE)0;
    }

    gdbm_db_initted = 0;
    return true;
}

static UDB_DATA gdbm_get(UDB_DATA gamekey, unsigned int type)
{
    UDB_DATA gamedata;
    datum dat;
    datum key;
    char *s;
    size_t keylen;

    if (!gdbm_db_initted || !gamekey.dptr || gamekey.dsize < 0)
    {
        gamedata.dptr = NULL;
        gamedata.dsize = 0;
        return gamedata;
    }

    keylen = (size_t)gamekey.dsize + sizeof(unsigned int);
    if (keylen > INT_MAX)
    {
        gamedata.dptr = NULL;
        gamedata.dsize = 0;
        return gamedata;
    }

    /* Construct composite key */
    s = key.dptr = (char *)XMALLOC(keylen, "key.dptr");
    XMEMCPY((void *)s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    XMEMCPY((void *)s, (void *)&type, sizeof(unsigned int));
    key.dsize = (int)keylen;
    
    dat = gdbm_fetch(gdbm_dbp, key);
    gamedata.dptr = dat.dptr;
    gamedata.dsize = dat.dsize;
    XFREE(key.dptr);
    return gamedata;
}

static int gdbm_put(UDB_DATA gamekey, UDB_DATA gamedata, unsigned int type)
{
    datum dat;
    datum key;
    char *s;
    size_t keylen;

    if (!gdbm_db_initted || !gamekey.dptr || gamekey.dsize < 0)
    {
        return (1);
    }

    keylen = (size_t)gamekey.dsize + sizeof(unsigned int);
    if (keylen > INT_MAX)
    {
        return (1);
    }

    /* Construct composite key */
    s = key.dptr = (char *)XMALLOC(keylen, "key.dptr");
    XMEMCPY((void *)s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    XMEMCPY((void *)s, (void *)&type, sizeof(unsigned int));
    key.dsize = (int)keylen;
    
    dat.dptr = gamedata.dptr;
    dat.dsize = gamedata.dsize;

    if (gdbm_store(gdbm_dbp, key, dat, GDBM_REPLACE))
    {
        warning("gdbm_put: gdbm_store failed. GDBM Error %s", gdbm_strerror(gdbm_errno));
        XFREE(key.dptr);
        return (1);
    }

    XFREE(key.dptr);
    return (0);
}

static int gdbm_del(UDB_DATA gamekey, unsigned int type)
{
    datum dat;
    datum key;
    char *s;
    size_t keylen;

    if (!gdbm_db_initted || !gamekey.dptr || gamekey.dsize < 0)
    {
        return (-1);
    }

    keylen = (size_t)gamekey.dsize + sizeof(unsigned int);
    if (keylen > INT_MAX)
    {
        return (1);
    }

    /* Construct composite key */
    s = key.dptr = (char *)XMALLOC(keylen, "key.dptr");
    XMEMCPY((void *)s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    XMEMCPY((void *)s, (void *)&type, sizeof(unsigned int));
    key.dsize = (int)keylen;
    
    dat = gdbm_fetch(gdbm_dbp, key);
    if (dat.dptr == NULL)
    {
        XFREE(key.dptr);
        return (0);
    }
    XFREE(dat.dptr);

    if (gdbm_delete(gdbm_dbp, key))
    {
        warning("gdbm_del: gdbm_delete failed. GDBM Error %s", gdbm_strerror(gdbm_errno));
        XFREE(key.dptr);
        return (1);
    }

    XFREE(key.dptr);
    return (0);
}

/* GDBM backend structure */
db_backend_t gdbm_backend = {
    .name = "GDBM",
    .setsync = gdbm_setsync,
    .init = gdbm_init,
    .setfile = gdbm_setfile,
    .close = mushgdbm_close,
    .optimize = gdbm_optimize,
    .get = gdbm_get,
    .put = gdbm_put,
    .del = gdbm_del,
    .private_data = NULL
};
