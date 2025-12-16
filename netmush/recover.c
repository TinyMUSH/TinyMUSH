/**
 * @file recover.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Tool to traverse a corrupted GDBM database, look for special
 *        tags, and rebuild a consistent database.
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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

#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <gdbm.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>

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
