/**
 * @file db_gdbm.c
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
#include "db_backend.h"

#include <stdbool.h>
#include <limits.h>
#include <dlfcn.h>
#include <getopt.h>
#include <gdbm.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
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
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_setsync: cannot set GDBM_SYNCMODE to %d on %s. GDBM Error %s", flag, gdbm_dbfile, gdbm_strerror(gdbm_errno));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_init: cannot open %s. GDBM Error %s", tmpfile, gdbm_strerror(gdbm_errno));
        XFREE(tmpfile);
        return (1);
    }
    XFREE(tmpfile);

    /* Set cache size */
    i = mushstate.standalone ? 400 : 2;
    if (gdbm_setopt(gdbm_dbp, GDBM_CACHESIZE, &i, sizeof(int)) == -1)
    {
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_init: cannot set cache size to %d on %s. GDBM Error %s", i, gdbm_dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /* Set GDBM to manage a global free space table */
    i = 1;
    if (gdbm_setopt(gdbm_dbp, GDBM_CENTFREE, &i, sizeof(int)) == -1)
    {
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_init: cannot set GDBM_CENTFREE to %d on %s. GDBM Error %s", i, gdbm_dbfile, gdbm_strerror(gdbm_errno));
        return (1);
    }

    /* Set GDBM to coalesce free blocks */
    i = 1;
    if (gdbm_setopt(gdbm_dbp, GDBM_COALESCEBLKS, &i, sizeof(int)) == -1)
    {
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_init: cannot set GDBM_COALESCEBLKS to %d on %s. GDBM Error %s", i, gdbm_dbfile, gdbm_strerror(gdbm_errno));
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
            log_write(LOG_ALWAYS, "DB", "WARN", "mushgdbm_close: gdbm_sync error on %s. GDBM Error %s", gdbm_dbfile, gdbm_strerror(gdbm_errno));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_put: gdbm_store failed. GDBM Error %s", gdbm_strerror(gdbm_errno));
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
        log_write(LOG_ALWAYS, "DB", "WARN", "gdbm_del: gdbm_delete failed. GDBM Error %s", gdbm_strerror(gdbm_errno));
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

#ifdef USE_GDBM

void usage_dbconvert(void)
{
    fprintf(stderr, "  -f, --config=<filename>   config file\n");
    fprintf(stderr, "  -C, --check               perform consistency check\n");
    fprintf(stderr, "  -d, --data=<path>         data directory\n");
    fprintf(stderr, "  -D, --dbfile=<filename>   database file\n");
    fprintf(stderr, "  -q, --cleanattr           clean attribute table\n");
    fprintf(stderr, "  -G, --gdbm                write in GDBM format (default)\n");
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

void usage_dbrecover(void)
{
    fprintf(stderr, "  -i, --input               dbm file to recover\n");
    fprintf(stderr, "  -o, --output              recovered db file\n\n");
}

/*
 * GDBM-specific dbconvert implementation
 * Converts between GDBM database and flat text formats.
 * GDBM creates single-file databases (e.g., game.gdbm)
 */
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
    int do_output_gdbm = 1;  /* Default to GDBM output */

    struct option long_options[] = {
        {"config", required_argument, 0, 'f'},
        {"check", no_argument, 0, 'C'},
        {"data", required_argument, 0, 'd'},
        {"dbfile", required_argument, 0, 'D'},
        {"cleanattr", no_argument, 0, 'q'},
        {"gdbm", no_argument, 0, 'G'},
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

    while ((c = getopt_long(argc, argv, "f:Cd:D:q:gGKkLlMmNHPpWwXxZzo:?", long_options, &option_index)) != -1)
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
            do_output_gdbm = 1;
            break;

        case 'g':
            do_output_gdbm = 0;
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

    /* Determine input format based on flags or default behavior */
    /* Try reading as GDBM first */
    if (fileexist(argv[optind]))
    {
        db_read();
        call_all_modules_nocache("db_read");
        db_format = F_TINYMUSH;
        db_ver = OUTPUT_VERSION;
        db_flags = OUTPUT_FLAGS;
    }
    else
    {
        /* Fall back to reading from stdin as flatfile */
        db_read_flatfile(stdin, &db_format, &db_ver, &db_flags);

        /* Call modules to load their flatfiles */
        s1 = XMALLOC(MBUF_SIZE, "s1");

        for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
        {
            XSNPRINTF(s1, MBUF_SIZE, "mod_%s_%s", mp->modname, "db_read_flatfile");

            if ((modfunc = (void (*)(FILE *))dlsym(mp->handle, s1)) != NULL)
            {
                s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);
                f = db_module_flatfile(s, 0);

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

        if (do_output_gdbm)
        {
            /* Write to GDBM database */
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

#endif /* USE_GDBM */
