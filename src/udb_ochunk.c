/* udb_ochunk.c */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"		/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"		/* required by mudconf */

#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "interface.h"
#include "externs.h"		/* required by code */

#ifdef HAVE_LIBTINYGDBM_H
#include "libtinygdbm.h"	/* required by code */
#else
#ifdef HAVE_LIBTINYQDBM_H
#include "libtinyqdbm.h"	/* required by code */
#endif
#endif

#include "udb.h"		/* required by code */
#include "udb_defs.h"

#define DEFAULT_DBMCHUNKFILE "mudDB"

static char *dbfile = DEFAULT_DBMCHUNKFILE;

static int db_initted = 0;

static GDBM_FILE dbp = (GDBM_FILE) 0;

static datum dat;

static datum key;

static struct flock fl;

extern void fatal(char *, ...);

extern void warning(char *, ...);

extern void log_db_err(int, int, const char *);

void dddb_setsync(int flag)
{
    char *gdbm_error;

    if (gdbm_setopt(dbp, GDBM_SYNCMODE, &flag, sizeof(int)) == -1) {
	gdbm_error = (char *) gdbm_strerror(gdbm_errno);
	warning("setsync: cannot toggle sync flag", dbfile, " ", (char *) -1, "\n", gdbm_error, "\n", (char *) 0);
    }
}

static void dbm_error(char *msg)
{
    log_write(LOG_ALWAYS, "DB", "ERROR", "Database error: %s\n", msg);
}

/* gdbm_reorganize compresses unused space in the db */

int dddb_optimize(void)
{
    int i;
    i = gdbm_reorganize(dbp);
    return i;
}

int dddb_init(void)
{
    static char *copen = "db_init cannot open ";
    char tmpfile[256];
    char *gdbm_error;
    int i;

    if (!mudstate.standalone) {
	sprintf(tmpfile, "%s/%s", mudconf.dbhome, dbfile);
    } else {
	strcpy(tmpfile, dbfile);
    }

    if ((dbp = gdbm_open(tmpfile, mudstate.db_block_size, GDBM_WRCREAT | GDBM_SYNC | GDBM_NOLOCK, 0600, dbm_error)) == (GDBM_FILE) 0) {
	gdbm_error = (char *) gdbm_strerror(gdbm_errno);
	warning(copen, tmpfile, " ", (char *) -1, "\n", gdbm_error, "\n", (char *) 0);
	return (1);
    }

    if (mudstate.standalone) {
	/*
	 * Set the cache size to be 400 hash buckets for GDBM.
	 */
	i = 400;

	if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1) {
	    gdbm_error = (char *) gdbm_strerror(gdbm_errno);
	    warning(copen, dbfile, " ", (char *) -1, "\n", gdbm_error, "\n", (char *) 0);
	    return (1);
	}
    } else {
	/*
	 * This would set the cache size to be 2 hash buckets
	 * * for GDBM, except that the library imposes a minimum
	 * * of 10.
	 */
	i = 2;

	if (gdbm_setopt(dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1) {
	    gdbm_error = (char *) gdbm_strerror(gdbm_errno);
	    warning(copen, dbfile, " ", (char *) -1, "\n", gdbm_error, "\n", (char *) 0);
	    return (1);
	}
    }

    /*
     * Set GDBM to manage a global free space table.
     */
    i = 1;

    if (gdbm_setopt(dbp, GDBM_CENTFREE, &i, sizeof(int)) == -1) {
	gdbm_error = (char *) gdbm_strerror(gdbm_errno);
	warning(copen, dbfile, " ", (char *) -1, "\n", gdbm_error, "\n", (char *) 0);
	return (1);
    }

    /*
     * Set GDBM to coalesce free blocks.
     */
    i = 1;

    if (gdbm_setopt(dbp, GDBM_COALESCEBLKS, &i, sizeof(int)) == -1) {
	gdbm_error = (char *) gdbm_strerror(gdbm_errno);
	warning(copen, dbfile, " ", (char *) -1, "\n", gdbm_error, "\n", (char *) 0);
	return (1);
    }

    /*
     * If we're standalone, having GDBM wait for each write is a
     * * performance no-no; run non-synchronous
     */

    if (mudstate.standalone) {
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

    if (db_initted) {
	return (1);
    }

    /*
     * KNOWN memory leak. can't help it. it's small
     */
    xp = xstrdup(fil, "dddb_setfile");

    if (xp == (char *) 0) {
	return (1);
    }

    dbfile = xp;
    return (0);
}

int dddb_close(void)
{
    if (dbp != (GDBM_FILE) 0) {
	gdbm_close(dbp);
	dbp = (GDBM_FILE) 0;
    }

    db_initted = 0;
    return (0);
}

/* Pass db_get a key, and it returns data. Type is used as part of the GDBM
 * key to guard against namespace conflicts in different MUSH subsystems.
 * It is the caller's responsibility to free the data returned by db_get */

DBData db_get(DBData gamekey, unsigned int type)
{
    DBData gamedata;
    char *s;
    char *newdat;

    if (!db_initted) {
	gamedata.dptr = NULL;
	gamedata.dsize = 0;
	return gamedata;
    }

    /*
     * Construct a key (GDBM likes first 4 bytes to be unique)
     */
    s = key.dptr = (char *) malloc(sizeof(int) + gamekey.dsize);
    memcpy((void *) s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    memcpy((void *) s, (void *) &type, sizeof(unsigned int));
    key.dsize = sizeof(int) + gamekey.dsize;
    dat = gdbm_fetch(dbp, key);

    if (mudconf.malloc_logger) {
	/*
	 * We must xmalloc() our own memory
	 */
	if (dat.dptr != NULL) {
	    newdat = (char *) xmalloc(dat.dsize, "db_get.newdat");
	    memcpy(newdat, dat.dptr, dat.dsize);
	    free(dat.dptr);
	    dat.dptr = newdat;
	}
    }

    gamedata.dptr = dat.dptr;
    gamedata.dsize = dat.dsize;
    free(key.dptr);
    return gamedata;
}

/* Pass db_put a key, data and the type of entry you are storing */

int db_put(DBData gamekey, DBData gamedata, unsigned int type)
{
    char *s;

    if (!db_initted) {
	return (1);
    }

    /*
     * Construct a key (GDBM likes first 4 bytes to be unique)
     */
    s = key.dptr = (char *) malloc(sizeof(int) + gamekey.dsize);
    memcpy((void *) s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    memcpy((void *) s, (void *) &type, sizeof(unsigned int));
    key.dsize = sizeof(int) + gamekey.dsize;
    /*
     * make table entry
     */
    dat.dptr = gamedata.dptr;
    dat.dsize = gamedata.dsize;

    if (gdbm_store(dbp, key, dat, GDBM_REPLACE)) {
	warning("db_put: can't gdbm_store ", " ", (char *) -1, "\n", (char *) 0);
	free(dat.dptr);
	free(key.dptr);
	return (1);
    }

    free(key.dptr);
    return (0);
}

/* Pass db_del a key and the type of entry you are deleting */

int db_del(DBData gamekey, unsigned int type)
{
    char *s;

    if (!db_initted) {
	return (-1);
    }

    /*
     * Construct a key (GDBM likes first 4 bytes to be unique)
     */
    s = key.dptr = (char *) malloc(sizeof(int) + gamekey.dsize);
    memcpy((void *) s, gamekey.dptr, gamekey.dsize);
    s += gamekey.dsize;
    memcpy((void *) s, (void *) &type, sizeof(unsigned int));
    key.dsize = sizeof(int) + gamekey.dsize;
    dat = gdbm_fetch(dbp, key);

    /*
     * not there?
     */
    if (dat.dptr == NULL) {
	free(key.dptr);
	return (0);
    }

    free(dat.dptr);

    /*
     * drop key from db
     */
    if (gdbm_delete(dbp, key)) {
	warning("db_del: can't delete key\n", (char *) NULL);
	free(key.dptr);
	return (1);
    }

    free(key.dptr);
    return (0);
}

void db_lock(void)
{
    /*
     * Attempt to lock the DBM file. Block until the lock is cleared,
     * then set it.
     */
    if (mudstate.dbm_fd == -1) {
	return;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (fcntl(mudstate.dbm_fd, F_SETLKW, &fl) == -1) {
	log_perror("DMP", "LOCK", NULL, "fcntl()");
	return;
    }
}

void db_unlock(void)
{
    if (mudstate.dbm_fd == -1) {
	return;
    }

    fl.l_type = F_UNLCK;

    if (fcntl(mudstate.dbm_fd, F_SETLK, &fl) == -1) {
	log_perror("DMP", "LOCK", NULL, "fcntl()");
	return;
    }
}
