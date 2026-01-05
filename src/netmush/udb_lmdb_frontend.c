/**
 * @file udb_lmdb_frontend.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief LMDB-only command-line helpers (dbconvert usage and implementation)
 * @version 4.0
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <dlfcn.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

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

/**
 * LMDB-specific dbconvert implementation
 * Converts between LMDB database and flat text formats.
 * LMDB creates directory-based databases (e.g., game.gdbm.lmdb/)
 * with internal data.mdb and lock.mdb files.
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
