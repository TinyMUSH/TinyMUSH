/**
 * @file db_backend_gdbm.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief GDBM database backend implementation and recovery tools
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

/*
 * GDBM Database Recovery Functions
 * (Formerly db_gdbm_recover.c)
 */

/* The dbm hash bucket element contains the full 31 bit hash value, the
   "pointer" to the key and data (stored together) with their sizes.  It also
   has a small part of the actual key value.  It is used to verify the first
   part of the key has the correct value without having to read the actual
   key. */

#define SMALL 4

typedef struct
{
	char start_tag[4];
	int hash_value;		   /* The complete 31 bit value. */
	char key_start[SMALL]; /* Up to the first SMALL bytes of the
							* key.  */
	off_t data_pointer;	   /* The file address of the key record.
							* The data record directly follows
							* the key.  */
	int key_size;		   /* Size of key data in the file. */
	int data_size;		   /* Size of associated data in the
							* file. */
} bucket_element;

void gdbm_panic(const char *mesg)
{
	fprintf(stderr, "GDBM panic: %s\n", mesg);
}

int dbrecover(int argc, char *argv[])
{
	datum key, dat;
	FILE *f = NULL; /* Initialize to NULL for safety */
	size_t numbytes;
	off_t filesize;
	long filepos;
	struct stat buf;
	bucket_element be;
	int c;
	char cp;
	char *infile, *outfile;
	int errflg = 0;
	int option_index = 0;
	unsigned long records_recovered = 0; /* Progress counter */
	struct option long_options[] = {
		{"input", required_argument, 0, 'i'},
		{"output", required_argument, 0, 'o'},
		{"help", no_argument, 0, '?'},
		{0, 0, 0, 0}};
	infile = outfile = NULL;
	GDBM_FILE dbp = (GDBM_FILE)0;

	/* Initialize datum pointers to NULL for safety */
	key.dptr = NULL;
	key.dsize = 0;
	dat.dptr = NULL;
	dat.dsize = 0;

	while ((c = getopt_long(argc, argv, "i:o:?", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'i':
			if (!optarg)
			{
				fprintf(stderr, "Error: -i/--input requires an argument.\n");
				errflg++;
				break;
			}
			infile = optarg;
			break;

		case 'o':
			if (!optarg)
			{
				fprintf(stderr, "Error: -o/--output requires an argument.\n");
				errflg++;
				break;
			}
			outfile = optarg;
			break;

		default:
			errflg++;
		}
	}

	if (errflg || !infile || !outfile)
	{
		char *progname = (argc > 0 && argv[0]) ? basename(argv[0]) : (char *)"dbrecover";
		usage(progname ? progname : (char *)"dbrecover", 2);
		exit(EXIT_FAILURE);
	}

	/* Ensure input and output files are different to avoid corruption */
	if (strcmp(infile, outfile) == 0)
	{
		fprintf(stderr, "Fatal error: Input and output files must be different.\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Get input file size first (before opening output database)
	 */
	if (stat(infile, &buf))
	{
		char errbuf[256];
		int err = errno;
		if (strerror_r(err, errbuf, sizeof(errbuf)) != 0)
		{
			snprintf(errbuf, sizeof(errbuf), "Unknown error %d", err);
		}
		fprintf(stderr, "Fatal error in stat (%s): %s\n", infile, errbuf);
		exit(EXIT_FAILURE);
	}

	filesize = buf.st_size;

	/* Validate filesize is reasonable */
	if (filesize <= 0)
	{
		fprintf(stderr, "Input file (%s) is empty or invalid (size: %lld).\n", infile, (long long)filesize);
		exit(EXIT_FAILURE);
	}

	if (filesize > (off_t)(1024LL * 1024LL * 1024LL * 10LL)) /* Max 10GB */
	{
		fprintf(stderr, "Input file (%s) is too large (size: %lld, max: 10GB).\n", infile, (long long)filesize);
		exit(EXIT_FAILURE);
	}

	/*
	 * Open output database
	 */
	if ((dbp = gdbm_open(outfile, 8192, GDBM_WRCREAT, 0600, gdbm_panic)) == NULL)
	{
		if (gdbm_errno)
		{
			fprintf(stderr, "Fatal error in gdbm_open (%s): %s\n", outfile, gdbm_strerror(gdbm_errno));
		}
		else
		{
			char errbuf[256];
			int err = errno;
			if (strerror_r(err, errbuf, sizeof(errbuf)) != 0)
			{
				snprintf(errbuf, sizeof(errbuf), "Unknown error %d", err);
			}
			fprintf(stderr, "Fatal error in gdbm_open (%s): %s\n", outfile, errbuf);
		}
		exit(EXIT_FAILURE);
	}
	f = fopen(infile, "rb"); /* Binary mode for portability */

	if (!f)
	{
		char errbuf[256];
		int err = errno;
		if (strerror_r(err, errbuf, sizeof(errbuf)) != 0)
		{
			snprintf(errbuf, sizeof(errbuf), "Unknown error %d", err);
		}
		fprintf(stderr, "Fatal error opening input file (%s): %s\n", infile, errbuf);
		gdbm_close(dbp);
		exit(EXIT_FAILURE);
	}

	while (fread((void *)&cp, 1, 1, f) != 0)
	{
		/*
		 * Quick and dirty scan for 'T' marker
		 */
		if (cp == 'T')
		{
			filepos = ftell(f);
			if (filepos == -1L || filepos < 0)
			{
				fprintf(stderr, "Fatal error: ftell() failed or returned invalid position.\n");
				fclose(f);
				gdbm_close(dbp);
				exit(EXIT_FAILURE);
			}

			/*
			 * Rewind one byte
			 */
			if (fseek(f, -1, SEEK_CUR) != 0)
			{
				fprintf(stderr, "Fatal error: fseek() failed at position %ld.\n", filepos);
				fclose(f);
				gdbm_close(dbp);
				exit(EXIT_FAILURE);
			}

			/* Clear bucket_element to avoid uninitialized data */
			memset(&be, 0, sizeof(bucket_element));

			/* Read bucket_element and check for both error and EOF */
			if (fread(&be, sizeof(bucket_element), 1, f) != 1)
			{
				if (feof(f))
				{
					/* EOF reached - not an error, just continue scanning */
					break;
				}
				fprintf(stderr, "Fatal error reading bucket_element at file position %ld.\n", filepos);
				fclose(f);
				gdbm_close(dbp);
				exit(EXIT_FAILURE);
			}

			/*
			 * Check the tag to make sure it's correct, and
			 * * make sure the pointer and sizes are sane
			 */

			/*
			 * Validate: tag matches, sizes are positive and reasonable,
			 * pointer is within file bounds, and total allocation is sane
			 */
			if (memcmp(be.start_tag, "TM3S", 4) == 0 &&
				be.data_pointer >= 0 &&
				be.data_pointer < filesize &&
				be.key_size > 0 && be.key_size <= INT_MAX &&
				be.data_size > 0 && be.data_size <= INT_MAX &&
				be.key_size < filesize &&
				be.data_size < filesize &&
				be.key_size < (1024 * 1024 * 100) &&  /* Max 100MB per key */
				be.data_size < (1024 * 1024 * 100) && /* Max 100MB per data */
				/* Ensure data_pointer + key_size + data_size doesn't overflow or exceed file */
				be.data_pointer <= (filesize - be.key_size - be.data_size))
			{
				filepos = ftell(f);
				if (filepos == -1L || filepos < 0)
				{
					fprintf(stderr, "Fatal error: ftell() failed before seeking to data.\n");
					fclose(f);
					gdbm_close(dbp);
					exit(EXIT_FAILURE);
				}

				/* Validate filepos is within file bounds */
				if (filepos >= filesize)
				{
					fprintf(stderr, "Warning: File position %ld exceeds file size %lld, skipping entry.\n", filepos, (long long)filesize);
					continue;
				}

				/*
				 * Seek to where the data begins
				 */
				if (fseek(f, be.data_pointer, SEEK_SET) != 0)
				{
					fprintf(stderr, "Fatal error: fseek() to data_pointer %lld failed.\n", (long long)be.data_pointer);
					fclose(f);
					gdbm_close(dbp);
					exit(EXIT_FAILURE);
				}
				key.dptr = (char *)XMALLOC(be.key_size, "key.dptr");
				key.dsize = be.key_size;
				dat.dptr = (char *)XMALLOC(be.data_size, "dat.dptr");
				dat.dsize = be.data_size;

				/* Read key data and verify exact size */
				if ((numbytes = fread(key.dptr, 1, key.dsize, f)) != (size_t)key.dsize)
				{
					fprintf(stderr, "Fatal error reading key at file position %ld (read %zu, expected %d).\n", filepos, numbytes, key.dsize);
					XFREE(key.dptr);
					key.dptr = NULL;
					XFREE(dat.dptr);
					dat.dptr = NULL;
					fclose(f);
					gdbm_close(dbp);
					exit(EXIT_FAILURE);
				}

				/* Read data and verify exact size */
				if ((numbytes = fread(dat.dptr, 1, dat.dsize, f)) != (size_t)dat.dsize)
				{
					fprintf(stderr, "Fatal error reading data at file position %ld (read %zu, expected %d).\n", filepos, numbytes, dat.dsize);
					XFREE(key.dptr);
					key.dptr = NULL;
					XFREE(dat.dptr);
					dat.dptr = NULL;
					fclose(f);
					gdbm_close(dbp);
					exit(EXIT_FAILURE);
				}

				/* Try to insert first to detect duplicates */
				int store_result = gdbm_store(dbp, key, dat, GDBM_INSERT);
				if (store_result != 0)
				{
					if (store_result == 1)
					{
						/* Key already exists - replace it with warning */
						fprintf(stderr, "Warning: Duplicate key found at position %ld, replacing...\n", filepos);
						store_result = gdbm_store(dbp, key, dat, GDBM_REPLACE);
					}
					if (store_result != 0)
					{
						if (gdbm_errno)
						{
							fprintf(stderr, "Fatal error in gdbm_store (%s): %s\n", outfile, gdbm_strerror(gdbm_errno));
						}
						else
						{
							char errbuf[256];
							int err = errno;
							if (strerror_r(err, errbuf, sizeof(errbuf)) != 0)
							{
								snprintf(errbuf, sizeof(errbuf), "Unknown error %d", err);
							}
							fprintf(stderr, "Fatal error in gdbm_store (%s): %s\n", outfile, errbuf);
						}
						XFREE(key.dptr);
						key.dptr = NULL;
						XFREE(dat.dptr);
						dat.dptr = NULL;
						fclose(f);
						gdbm_close(dbp);
						exit(EXIT_FAILURE);
					}
				}

				/* Increment recovery counter with overflow check */
				if (records_recovered < ULONG_MAX)
				{
					records_recovered++;
				}
				else
				{
					fprintf(stderr, "Warning: Record counter overflow (reached ULONG_MAX).\n");
				}

				/* Print progress every 1000 records */
				if (records_recovered % 1000 == 0)
				{
					fprintf(stderr, "Progress: %lu records recovered...\n", records_recovered);
				}

				XFREE(key.dptr);
				key.dptr = NULL;
				XFREE(dat.dptr);
				dat.dptr = NULL;
				/*
				 * Seek back to where we left off
				 */
				if (fseek(f, filepos, SEEK_SET) != 0)
				{
					fprintf(stderr, "Fatal error: fseek() back to position %ld failed.\n", filepos);
					fclose(f);
					gdbm_close(dbp);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				/*
				 * Seek back to one byte after we started
				 * * and continue
				 */
				if (fseek(f, filepos, SEEK_SET) != 0)
				{
					fprintf(stderr, "Fatal error: fseek() back to position %ld failed.\n", filepos);
					fclose(f);
					gdbm_close(dbp);
					exit(EXIT_FAILURE);
				}
			}
		}
	}

	/* Check if loop ended due to error vs normal EOF */
	if (ferror(f))
	{
		fprintf(stderr, "Fatal error: I/O error occurred while reading input file.\n");
		fclose(f);
		gdbm_close(dbp);
		exit(EXIT_FAILURE);
	}

	fclose(f);

	/* Sync database to ensure all data is written to disk */
	if (gdbm_sync(dbp) != 0)
	{
		if (gdbm_errno)
		{
			fprintf(stderr, "Warning: gdbm_sync failed (%s): %s\n", outfile, gdbm_strerror(gdbm_errno));
		}
		else
		{
			fprintf(stderr, "Warning: gdbm_sync failed (%s).\n", outfile);
		}
	}

	gdbm_close(dbp);

	/* Print final summary */
	fprintf(stderr, "Recovery complete: %lu records successfully recovered.\n", records_recovered);
	exit(EXIT_SUCCESS);
}

#endif /* USE_GDBM */
