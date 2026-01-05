/**
 * @file db_backend_lmdb.c
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
#include "db_storage.h"

#include <stdbool.h>
#include <limits.h>
#include <dlfcn.h>
#include <getopt.h>
#include <lmdb.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stddef.h>

/* LMDB-specific state */
static char *lmdb_dbfile = DEFAULT_DBMCHUNKFILE;
static int lmdb_db_initted = 0;
static MDB_env *lmdb_env = NULL;
static MDB_dbi lmdb_dbi = 0;

/* LMDB map sizing (default 1 GB, grows as needed up to 16 GB) */
#define LMDB_DEFAULT_MAPSIZE (1UL * 1024UL * 1024UL * 1024UL)
#define LMDB_MAX_MAPSIZE (16UL * 1024UL * 1024UL * 1024UL)
#define LMDB_MAP_GROWTH_FACTOR 2UL
static size_t lmdb_mapsize = LMDB_DEFAULT_MAPSIZE;

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
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_setsync: cannot get env flags: %s", mdb_strerror(rc));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_setsync: cannot set sync mode: %s", mdb_strerror(rc));
    } else {
        log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: set sync mode to %s on %s.", 
                  flag ? "sync" : "async", lmdb_dbfile);
    }
}

/* Grow the LMDB mapsize to accommodate additional writes */
static int lmdb_grow_mapsize(size_t minimum)
{
    MDB_envinfo info;
    size_t current;
    size_t target;
    int rc;

    if (!lmdb_env)
    {
        return 1;
    }

    rc = mdb_env_info(lmdb_env, &info);
    if (rc != 0)
    {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_grow_mapsize: mdb_env_info failed: %s", mdb_strerror(rc));
        return 1;
    }

    current = (size_t)info.me_mapsize;
    target = current;

    if (target < LMDB_DEFAULT_MAPSIZE)
    {
        target = LMDB_DEFAULT_MAPSIZE;
    }

    if (minimum > target)
    {
        target = minimum;
    }

    while (target < minimum && target <= (LMDB_MAX_MAPSIZE / LMDB_MAP_GROWTH_FACTOR))
    {
        target *= LMDB_MAP_GROWTH_FACTOR;
    }

    if (target > LMDB_MAX_MAPSIZE)
    {
        target = LMDB_MAX_MAPSIZE;
    }

    if (target <= current)
    {
        return 1;
    }

    rc = mdb_env_set_mapsize(lmdb_env, target);
    if (rc != 0)
    {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_grow_mapsize: mdb_env_set_mapsize failed: %s", mdb_strerror(rc));
        return 1;
    }

    lmdb_mapsize = target;
    log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: grew map to %zu bytes", target);
    return 0;
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
    MDB_envinfo info;
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
            log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: cannot create directory %s", tmpdir);
            XFREE(tmpfile);
            XFREE(tmpdir);
            return (1);
        }
    }
    
    log_write(LOG_ALWAYS, "DB", "INFO", "LMDB: opening %s", tmpdir);

    /* Create LMDB environment */
    rc = mdb_env_create(&lmdb_env);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_env_create failed: %s", mdb_strerror(rc));
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    /* Set map size (1 GB default, can grow) */
    rc = mdb_env_set_mapsize(lmdb_env, LMDB_DEFAULT_MAPSIZE);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_env_set_mapsize failed: %s", mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    lmdb_mapsize = LMDB_DEFAULT_MAPSIZE;

    /* Set max databases (we only use 1, but allow expansion) */
    rc = mdb_env_set_maxdbs(lmdb_env, 1);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_env_set_maxdbs failed: %s", mdb_strerror(rc));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_env_open failed on %s: %s", tmpdir, mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    rc = mdb_env_info(lmdb_env, &info);
    if (rc == 0) {
        lmdb_mapsize = (size_t)info.me_mapsize;
    } else {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_env_info failed: %s", mdb_strerror(rc));
    }

    /* Open main database */
    rc = mdb_txn_begin(lmdb_env, NULL, 0, &txn);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_txn_begin failed: %s", mdb_strerror(rc));
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    rc = mdb_dbi_open(txn, NULL, MDB_CREATE, &lmdb_dbi);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_dbi_open failed: %s", mdb_strerror(rc));
        mdb_txn_abort(txn);
        mdb_env_close(lmdb_env);
        lmdb_env = NULL;
        XFREE(tmpfile);
        XFREE(tmpdir);
        return (1);
    }

    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_init: mdb_txn_commit failed: %s", mdb_strerror(rc));
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
    lmdb_mapsize = LMDB_DEFAULT_MAPSIZE;
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
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_get: mdb_txn_begin failed: %s", mdb_strerror(rc));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_get: mdb_get failed: %s", mdb_strerror(rc));
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

retry_put:
    rc = mdb_txn_begin(lmdb_env, NULL, 0, &txn);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_put: mdb_txn_begin failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return (1);
    }

    rc = mdb_put(txn, lmdb_dbi, &key, &data, 0);
    if (rc == MDB_MAP_FULL) {
        mdb_txn_abort(txn);

        if (lmdb_grow_mapsize(lmdb_mapsize * LMDB_MAP_GROWTH_FACTOR) == 0) {
            goto retry_put;
        }

        XFREE(keybuf);
        return (1);
    }

    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_put: mdb_put failed: %s", mdb_strerror(rc));
        mdb_txn_abort(txn);
        XFREE(keybuf);
        return (1);
    }

    rc = mdb_txn_commit(txn);
    if (rc == MDB_MAP_FULL) {
        if (lmdb_grow_mapsize(lmdb_mapsize * LMDB_MAP_GROWTH_FACTOR) == 0) {
            goto retry_put;
        }

        XFREE(keybuf);
        return (1);
    }

    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_put: mdb_txn_commit failed: %s", mdb_strerror(rc));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_del: mdb_txn_begin failed: %s", mdb_strerror(rc));
        XFREE(keybuf);
        return (1);
    }

    /* Delete record */
    rc = mdb_del(txn, lmdb_dbi, &key, NULL);
    if (rc != 0 && rc != MDB_NOTFOUND) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_del: mdb_del failed: %s", mdb_strerror(rc));
        mdb_txn_abort(txn);
        XFREE(keybuf);
        return (1);
    }

    /* Commit transaction */
    rc = mdb_txn_commit(txn);
    if (rc != 0) {
        log_write(LOG_ALWAYS, "DB", "WARN", "lmdb_del: mdb_txn_commit failed: %s", mdb_strerror(rc));
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

#ifdef USE_LMDB

void usage_dbconvert(void)
{
    fprintf(stderr, "  -f, --config=<filename>   config file\n");
    fprintf(stderr, "  -C, --check               perform consistency check\n");
    fprintf(stderr, "  -d, --data=<path>         data directory\n");
    fprintf(stderr, "  -D, --dbdir=<path>        database directory\n");
    fprintf(stderr, "  -q, --cleanattr           clean attribute table\n");
    fprintf(stderr, "  -G, --lmdb                write in LMDB format (default)\n");
    fprintf(stderr, "  -g, --flat                write in flat text format\n");
    fprintf(stderr, "  -K, --keyattr             store keys as object attributes\n");
    fprintf(stderr, "  -k, --keyhdr              store keys in object header\n");
    fprintf(stderr, "  -L, --links               include link information\n");
    fprintf(stderr, "  -l, --nolinks             don't include link information\n");
    fprintf(stderr, "  -M, --maps                include attribute maps\n");
    fprintf(stderr, "  -m, --nomaps              don't include attribute maps\n");
    fprintf(stderr, "  -N, --nameattr            store names as object attributes\n");
    fprintf(stderr, "  -H, --namehdr             store names in object header\n");
    fprintf(stderr, "  -P, --parents             include parent information\n");
    fprintf(stderr, "  -p, --noparents           don't include parent information\n");
    fprintf(stderr, "  -W, --write               write database to output\n");
    fprintf(stderr, "  -w, --nowrite             don't write database\n");
    fprintf(stderr, "  -X, --mindb               create minimal database\n");
    fprintf(stderr, "  -x, --minflat             create minimal flat file\n");
    fprintf(stderr, "  -Z, --zones               include zone information\n");
    fprintf(stderr, "  -z, --nozones             don't include zone information\n");
    fprintf(stderr, "  -o, --output=<number>     set output version number\n\n");
}

int dbconvert(int argc, char *argv[])
{
    int ver;
    int db_ver, db_format, db_flags, do_check, do_write;
    int c, dbclean, errflg = 0;
    int setflags, clrflags;
    char *opt_conf = (char *)DEFAULT_CONFIG_FILE;
    char *opt_datadir = (char *)DEFAULT_DATABASE_HOME;
    char *opt_dbfile = (char *)DEFAULT_CONFIG_FILE;
    char *s, *s1;
    FILE *f;
    MODULE *mp;
    void (*modfunc)(FILE *);
    int option_index = 0;
    int do_output_lmdb = 1;  /* Default to LMDB output */

    struct option long_options[] = {
        {"config", required_argument, 0, 'f'},
        {"check", no_argument, 0, 'C'},
        {"data", required_argument, 0, 'd'},
        {"dbdir", required_argument, 0, 'D'},
        {"cleanattr", no_argument, 0, 'q'},
        {"lmdb", no_argument, 0, 'G'},
        {"flat", no_argument, 0, 'g'},
        {"keyattr", no_argument, 0, 'K'},
        {"keyhdr", no_argument, 0, 'k'},
        {"links", no_argument, 0, 'L'},
        {"nolinks", no_argument, 0, 'l'},
        {"maps", no_argument, 0, 'M'},
        {"nomaps", no_argument, 0, 'm'},
        {"nameattr", no_argument, 0, 'N'},
        {"namehdr", no_argument, 0, 'H'},
        {"parents", no_argument, 0, 'P'},
        {"noparents", no_argument, 0, 'p'},
        {"write", no_argument, 0, 'W'},
        {"nowrite", no_argument, 0, 'w'},
        {"mindb", no_argument, 0, 'X'},
        {"minflat", no_argument, 0, 'x'},
        {"zones", no_argument, 0, 'Z'},
        {"nozones", no_argument, 0, 'z'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, '?'},
        {0, 0, 0, 0}};

    logfile_init(NULL);

    /* Initialize conversion parameters */
    ver = do_check = 0;
    do_write = 1;
    dbclean = V_DBCLEAN;
    setflags = 0;
    clrflags = 0;

    while ((c = getopt_long(argc, argv, "f:Cd:D:q:GgKkLlMmNHPpWwXxZzo:?", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'f':
            opt_conf = optarg;
            break;

        case 'd':
            opt_datadir = optarg;
            break;

        case 'D':
            opt_dbfile = optarg;
            break;

        case 'C':
            do_check = 1;
            break;

        case 'q':
            dbclean = 0;
            break;

        case 'G':
            do_output_lmdb = 1;
            break;

        case 'g':
            do_output_lmdb = 0;
            break;

        case 'K':
            setflags |= V_ATRNAME;
            clrflags &= ~V_ATRNAME;
            break;

        case 'k':
            clrflags |= V_ATRNAME;
            setflags &= ~V_ATRNAME;
            break;

        case 'L':
            setflags |= V_LINK;
            clrflags &= ~V_LINK;
            break;

        case 'l':
            clrflags |= V_LINK;
            setflags &= ~V_LINK;
            break;

        case 'M':
            setflags |= V_ATRKEY;
            clrflags &= ~V_ATRKEY;
            break;

        case 'm':
            clrflags |= V_ATRKEY;
            setflags &= ~V_ATRKEY;
            break;

        case 'N':
            setflags |= V_ATRNAME;
            clrflags &= ~V_ATRNAME;
            break;

        case 'H':
            clrflags |= V_ATRNAME;
            setflags &= ~V_ATRNAME;
            break;

        case 'P':
            setflags |= V_PARENT;
            clrflags &= ~V_PARENT;
            break;

        case 'p':
            clrflags |= V_PARENT;
            setflags &= ~V_PARENT;
            break;

        case 'W':
            do_write = 1;
            break;

        case 'w':
            do_write = 0;
            break;

        case 'X':
            dbclean = V_DBCLEAN;
            break;

        case 'x':
            dbclean = 0;
            break;

        case 'Z':
            setflags |= V_ZONE;
            clrflags &= ~V_ZONE;
            break;

        case 'z':
            clrflags |= V_ZONE;
            setflags &= ~V_ZONE;
            break;

        case 'o':
            ver = (int)strtol(optarg, NULL, 10);
            break;

        default:
            errflg++;
        }
    }

    if (errflg || optind >= argc)
    {
        usage(basename(argv[0]), 1);
        exit(EXIT_FAILURE);
    }

    mushconf.dbhome = XSTRDUP(opt_datadir, "argv");
    mushconf.db_file = XSTRDUP(opt_dbfile, "argv");
    cf_init();
    mushstate.standalone = 1;
    cf_read(opt_conf);
    mushstate.initializing = 0;

    vattr_init();

    if (init_database(argv[optind]) < 0)
    {
        log_write_raw(1, "Can't open database file\n");
        exit(EXIT_FAILURE);
    }

    db_lock();

    /* Read from current LMDB database */
    db_read();
    call_all_modules_nocache("db_read");
    db_format = F_TINYMUSH;
    db_ver = OUTPUT_VERSION;
    db_flags = OUTPUT_FLAGS;

    /* Apply conversion flags from command line */
    db_flags = (db_flags & ~clrflags) | setflags;

    log_write_raw(1, "Input: ");
    info(db_format, db_flags, db_ver);

    if (do_check)
    {
        do_dbck(NOTHING, NOTHING, DBCK_FULL);
    }

    if (do_write)
    {
        if (ver != 0)
        {
            db_ver = ver;
        }
        else
        {
            db_ver = 3;
        }

        log_write_raw(1, "Output: ");

        if (do_output_lmdb)
        {
            /* Write to LMDB database */
            info(F_TINYMUSH, db_flags, db_ver);
            db_write();
            db_lock();
            call_all_modules_nocache("db_write");
            db_unlock();
        }
        else
        {
            /* Write to flat text file */
            info(F_TINYMUSH, UNLOAD_OUTFLAGS, db_ver);
            db_write_flatfile(stdout, F_TINYMUSH, db_ver | UNLOAD_OUTFLAGS | dbclean);
			
            /* Call all modules to write to flatfile */
            s1 = XMALLOC(MBUF_SIZE, "s1");

            for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
            {
                XSNPRINTF(s1, MBUF_SIZE, "mod_%s_%s", mp->modname, "db_write_flatfile");

                if ((modfunc = (void (*)(FILE *))dlsym(mp->handle, s1)) != NULL)
                {
                    s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);
                    f = db_module_flatfile(s, 1);

                    if (f)
                    {
                        (*modfunc)(f);
                        tf_fclose(f);
                    }

                    XFREE(s);
                }
            }

            XFREE(s1);
        }
    }

    db_unlock();
    db_sync_attributes();
    dddb_close();
    exit(EXIT_SUCCESS);
}

#endif /* USE_LMDB */
