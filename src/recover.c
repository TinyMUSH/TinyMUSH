/*
 * A tool to traverse a corrupted GDBM database, look for special tags, and
 * rebuild a consistent database
 */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mudconf */
#include "game.h"	   /* required by mudconf */
#include "alloc.h"	   /* required by mudconf */
#include "flags.h"	   /* required by mudconf */
#include "htab.h"	   /* required by mudconf */
#include "ltdl.h"	   /* required by mudconf */
#include "udb.h"	   /* required by mudconf */
#include "udb_defs.h"  /* required by mudconf */
#include "mushconf.h"  /* required by code */
#include "db.h"		   /* required by externs */
#include "udb.h"	   /* required by code */
#include "udb_defs.h"  /* required by code */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by interface */
#include "file_c.h"	   /* required by code */
#include "command.h"   /* required by code */
#include "powers.h"	   /* required by code */
#include "attrs.h"	   /* required by code */
#include "defaults.h"  /* required by code */

#ifdef HAVE_GDBM_H
#include <gdbm.h> /* required by code */
#endif

#ifdef HAVE_QDBM_H
#include <qdbm.h> /* required by code */
#endif

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
	FILE *f;
	int numbytes, filesize;
	long filepos;
	struct stat buf;
	bucket_element be;
	int c;
	char cp;
	char *infile, *outfile;
	int errflg = 0;
	int option_index = 0;
	struct option long_options[] = {
		{"input", required_argument, 0, 'i'},
		{"output", required_argument, 0, 'o'},
		{"help", no_argument, 0, '?'},
		{0, 0, 0, 0}};
	infile = outfile = NULL;
	GDBM_FILE dbp = (GDBM_FILE)0;

	while ((c = getopt_long(argc, argv, "i:o:?", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'i':
			infile = optarg;
			break;

		case 'o':
			outfile = optarg;
			break;

		default:
			errflg++;
		}
	}

	if (errflg || !infile || !outfile)
	{
		usage(basename(argv[0]), 2);
		exit(EXIT_FAILURE);
	}

	/*
     * Open files
     */

	if ((dbp = gdbm_open(outfile, 8192, GDBM_WRCREAT, 0600, gdbm_panic)) == NULL)
	{
		fprintf(stderr, "Fatal error in gdbm_open (%s): %s\n", outfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (stat(infile, &buf))
	{
		fprintf(stderr, "Fatal error in stat (%s): %s\n", infile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	filesize = buf.st_size;
	f = fopen(infile, "r");

	while (fread((void *)&cp, 1, 1, f) != 0)
	{
		/*
	 * Quick and dirty
	 */
		if (cp == 'T')
		{
			filepos = ftell(f);
			/*
	     * Rewind one byte
	     */
			fseek(f, -1, SEEK_CUR);

			if (fread((void *)&be, sizeof(bucket_element), 1, f) == 0)
			{
				fprintf(stderr, "Fatal error at file position %ld.\n", filepos);
				exit(EXIT_FAILURE);
			}

			/*
	     * Check the tag to make sure it's correct, and
	     * * make sure the pointer and sizes are sane
	     */

			if (!memcmp((void *)(be.start_tag), (void *)"TM3S", 4) && be.data_pointer < filesize && be.key_size < filesize && be.data_size < filesize)
			{
				filepos = ftell(f);
				/*
		 * Seek to where the data begins
		 */
				fseek(f, be.data_pointer, SEEK_SET);
				key.dptr = (char *)malloc(be.key_size);
				key.dsize = be.key_size;
				dat.dptr = (char *)malloc(be.data_size);
				dat.dsize = be.data_size;

				if ((numbytes = fread((void *)(key.dptr), 1, key.dsize, f)) == 0)
				{
					fprintf(stderr, "Fatal error at file position %ld.\n", filepos);
					exit(EXIT_FAILURE);
				}

				if (fread((void *)(dat.dptr), dat.dsize, 1, f) == 0)
				{
					fprintf(stderr, "Fatal error at file position %ld.\n", filepos);
					exit(EXIT_FAILURE);
				}

				if (gdbm_store(dbp, key, dat, GDBM_REPLACE))
				{
					fprintf(stderr, "Fatal error in gdbm_store (%s): %s\n", outfile, strerror(errno));
					exit(EXIT_FAILURE);
				}

				free(key.dptr);
				free(dat.dptr);
				/*
		 * Seek back to where we left off
		 */
				fseek(f, filepos, SEEK_SET);
			}
			else
			{
				/*
		 * Seek back to one byte after we started
		 * * and continue
		 */
				fseek(f, filepos, SEEK_SET);
			}
		}
	}

	fclose(f);
	gdbm_close(dbp);
	exit(EXIT_SUCCESS);
}
