/**
 * @file udb_ochunk.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Database handling functions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

char *dbfile = DEFAULT_DBMCHUNKFILE;
int db_initted = 0;
GDBM_FILE dbp = (GDBM_FILE)0;

struct flock fl;

void dddb_setsync(int flag)
{
    if (gdbm_setopt(dbp, GDBM_SYNCMODE, &flag, sizeof(int)) == -1)
    {
        warning("dddb_setsync: cannot set GDBM_SYNCMODE to %d on %s. GDBM Error %s", flag, dbfile, gdbm_strerror(gdbm_errno));
    }
    else
    {
        log_write(LOG_ALWAYS, "DB", "INFO", "set GDBM_SYNCMODE to %d on %s.", flag, dbfile);
    }
}

void dbm_error(const char *msg)
{
    log_write(LOG_ALWAYS, "DB", "ERROR", "Database error: %s\n", msg);
}

/* gdbm_reorganize compresses unused space in the db */

int dddb_optimize(void)
{
    log_write(LOG_ALWAYS, "DB", "INFO", "Optimizing %s", dbfile);

    return gdbm_reorganize(dbp);
}

int dddb_init(void)
{
    char *tmpfile;
    int i;

    if (!mudstate.standalone)
    {

        tmpfile = XASPRINTF("tmpfile", "%s/%s", mudconf.dbhome, dbfile);
    }
    else
    {
        tmpfile = XSTRDUP(dbfile, "tmpfile");
    }

    log_write(LOG_ALWAYS, "DB", "INFO", "Opening %s", tmpfile);

    if ((dbp = gdbm_open(tmpfile, mudstate.db_block_size, GDBM_WRCREAT | GDBM_SYNC | GDBM_NOLOCK, 0600, dbm_error)) == (GDBM_FILE)0)
    {
        warning("dddb_init: cannot open %s. GDBM Error %s", tmpfile, gdbm_strerror(gdbm_errno));
        XFREE(tmpfile);
        return (1);
    }
    XFREE(tmpfile);

    if (mudstate.standalone)
    {
        /*
	 * Set the cache size to be 400 hash buckets for GDBM.
	 */
        i = 400;

        if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1)
        {
            warning("dddb_init: cannot set cache size to %d on %s. GDBM Error %s", i, dbfile, gdbm_strerror(gdbm_errno));
            return (1);
        }
    }
    else
    {
        /*
	 * This would set the cache size to be 2 hash buckets
	 * * for GDBM, except that the library imposes a minimum
	 * * of 10.
	 */
        i = 2;

        if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1)
        {
            warning("dddb_init: cannot set GDBM_CACHESIZE to %d on %s. GDBM Error %s", i, dbfile, gdbm_strerror(gdbm_errno));
            return (1);
        }
    }

    /*
     * Set GDBM to manage a global free space table.
     */
    i = 1;

    if (gdbm_setopt(dbp, GDBM_CENTFREE, &i, sizeof(int)) == -1)
    {
        warning("dddb_init: cannot set GDBM_CENTFREE to %d on %s. GDBM Error %s", i, dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /*
     * Set GDBM to coalesce free blocks.
     */
    i = 1;

    if (gdbm_setopt(dbp, GDBM_COALESCEBLKS, &i, sizeof(int)) == -1)
    {
        warning("dddb_init: cannot set GDBM_COALESCEBLKS to %d on %s. GDBM Error %s", i, dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /*
     * If we're standalone, having GDBM wait for each write is a
     * * performance no-no; run non-synchronous
     */

    if (mudstate.standalone)
    {
        dddb_setsync(0);
    }

    /*
     * Grab the file descriptor for locking
     */
    mudstate.dbm_fd = gdbm_fdesc(dbp);
    db_initted = 1;
    return (0);
}

int dddb_setfile(char *fil)
{
    char *xp;

    if (db_initted)
    {
        return (1);
    }

    /*
     * KNOWN memory leak. can't help it. it's small
     */
    xp = XSTRDUP(fil, "xp");

    if (xp == (char *)0)
    {
        return (1);
    }

    dbfile = xp;
    return (0);
}

bool dddb_close(void)
{
    log_write(LOG_ALWAYS, "DB", "INFO", "closing %s", dbfile);

    if (dbp != (GDBM_FILE)0)
    {
        if (gdbm_sync(dbp) == -1)
        {
            warning("dddb_close: gdbm_sync error on %s. GDBM Error %s", dbfile, gdbm_strerror(gdbm_errno));
            return false;
        }
        if (gdbm_close(dbp) == -1)
        {
            warning("dddb_close: gdbm_close error on %s. GDBM Error %s", dbfile, gdbm_strerror(gdbm_errno));
            return false;
        }
        dbp = (GDBM_FILE)0;
    }

    db_initted = 0;
    return true;
}

/* Pass db_get a key, and it returns data. Type is used as part of the GDBM
 * key to guard against namespace conflicts in different MUSH subsystems.
 * It is the caller's responsibility to free the data returned by db_get */

UDB_DATA db_get(UDB_DATA gamekey, unsigned int type)
{
    UDB_DATA gamedata;
    datum dat;
    datum key;
    char *s;

    if (!db_initted)
    {
        gamedata.dptr = NULL;
        gamedata.dsize = 0;
        return gamedata;
    }

    /*
     * Construct a key (GDBM likes first 4 bytes to be unique)
     */
    s = key.dptr = (char *)XMALLOC(sizeof(int) + gamekey.dsize, "key.dptr");
    XMEMCPY((void *)s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    XMEMCPY((void *)s, (void *)&type, sizeof(unsigned int));
    key.dsize = sizeof(int) + gamekey.dsize;
    dat = gdbm_fetch(dbp, key);
    gamedata.dptr = dat.dptr;
    gamedata.dsize = dat.dsize;
    XFREE(key.dptr);
    return gamedata;
}

/* Pass db_put a key, data and the type of entry you are storing */

int db_put(UDB_DATA gamekey, UDB_DATA gamedata, unsigned int type)
{
    datum dat;
    datum key;
    char *s;

    if (!db_initted)
    {
        return (1);
    }

    /*
     * Construct a key (GDBM likes first 4 bytes to be unique)
     */
    s = key.dptr = (char *)XMALLOC(sizeof(int) + gamekey.dsize, "key.dptr");
    XMEMCPY((void *)s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    XMEMCPY((void *)s, (void *)&type, sizeof(unsigned int));
    key.dsize = sizeof(int) + gamekey.dsize;
    /*
     * make table entry
     */
    dat.dptr = gamedata.dptr;
    dat.dsize = gamedata.dsize;

    if (gdbm_store(dbp, key, dat, GDBM_REPLACE))
    {
        warning("db_put: gdbm_store failed. GDBM Error %s", gdbm_strerror(gdbm_errno));
        XFREE(key.dptr);
        return (1);
    }

    XFREE(key.dptr);
    return (0);
}

/* Pass db_del a key and the type of entry you are deleting */

int db_del(UDB_DATA gamekey, unsigned int type)
{
    datum dat;
    datum key;
    char *s;

    if (!db_initted)
    {
        return (-1);
    }

    /*
     * Construct a key (GDBM likes first 4 bytes to be unique)
     */
    s = key.dptr = (char *)XMALLOC(sizeof(int) + gamekey.dsize, "key.dptr");
    XMEMCPY((void *)s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    XMEMCPY((void *)s, (void *)&type, sizeof(unsigned int));
    key.dsize = sizeof(int) + gamekey.dsize;
    dat = gdbm_fetch(dbp, key);

    /* not there? */
    if (dat.dptr == NULL)
    {
        XFREE(key.dptr);
        return (0);
    }

    XFREE(dat.dptr);

    /*
     * drop key from db
     */
    if (gdbm_delete(dbp, key))
    {
        warning("db_del: gdbm_delete failed. GDBM Error %s", gdbm_strerror(gdbm_errno));
        XFREE(key.dptr);
        return (1);
    }

    XFREE(key.dptr);
    return (0);
}

void db_lock(void)
{
    /*
     * Attempt to lock the DBM file. Block until the lock is cleared,
     * then set it.
     */
    if (mudstate.dbm_fd == -1)
    {
        return;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (fcntl(mudstate.dbm_fd, F_SETLKW, &fl) == -1)
    {
        log_perror("DMP", "LOCK", NULL, "fcntl()");
        return;
    }
}

void db_unlock(void)
{
    if (mudstate.dbm_fd == -1)
    {
        return;
    }

    fl.l_type = F_UNLCK;

    if (fcntl(mudstate.dbm_fd, F_SETLK, &fl) == -1)
    {
        log_perror("DMP", "LOCK", NULL, "fcntl()");
        return;
    }
}
