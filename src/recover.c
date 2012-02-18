/*
 * A tool to traverse a corrupted GDBM database, look for special tags, and
 * rebuild a consistent database
 */

#include "copyright.h"
#include "autoconf.h"
#include "db.h"

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>

#ifdef MUSH_DBM_GDBM
#include "libtinygdbm.h"	/* required by code */
#else
#ifdef MUSH_DBM_QDBM
#include "libtinyqdbm.h"	/* required by code */
#endif
#endif

static GDBM_FILE dbp = (GDBM_FILE) 0;
//static gdbm_file_info *dbp = NULL;

static void
gdbm_panic(mesg)
char *mesg;
{
    fprintf(stderr, "GDBM panic: %s\n", mesg);
}

extern char *optarg;

extern int optind;

int
main(argc, argv)
int argc;

char *argv[];
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

    /*
     * Parse options
     */

    infile = outfile = NULL;

    while ((c = getopt(argc, argv, "i:o:")) != -1)
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
        fprintf(stderr, "Usage: %s -i input_file -o output_file\n",
                argv[0]);
        exit(1);
    }

    /*
     * Open files
     */

    if ((dbp = gdbm_open(outfile, 8192, GDBM_WRCREAT, 0600,
                         gdbm_panic)) == NULL)
    {
        fprintf(stderr, "Fatal error in gdbm_open (%s): %s\n",
                outfile, strerror(errno));
        exit(1);
    }

    if (stat(infile, &buf))
    {
        fprintf(stderr, "Fatal error in stat (%s): %s\n",
                infile, strerror(errno));
        exit(1);
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

            if (fread((void *)&be, sizeof(bucket_element),
                      1, f) == 0)
            {
                fprintf(stderr,
                        "Fatal error at file position %ld.\n",
                        filepos);
                exit(1);
            }

            /*
             * Check the tag to make sure it's correct, and
             * * make sure the pointer and sizes are sane
             */

            if (!memcmp((void *)(be.start_tag),
                        (void *)"TM3S", 4) &&
                    be.data_pointer < filesize &&
                    be.key_size < filesize &&
                    be.data_size < filesize)
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

                if ((numbytes = fread((void *)(key.dptr), 1,
                                      key.dsize, f)) == 0)
                {
                    fprintf(stderr,
                            "Fatal error at file position %ld.\n",
                            filepos);
                    exit(1);
                }

                if (fread((void *)(dat.dptr), dat.dsize,
                          1, f) == 0)
                {
                    fprintf(stderr,
                            "Fatal error at file position %ld.\n",
                            filepos);
                    exit(1);
                }

                if (gdbm_store(dbp, key, dat, GDBM_REPLACE))
                {
                    fprintf(stderr,
                            "Fatal error in gdbm_store (%s): %s\n",
                            outfile, strerror(errno));
                    exit(1);
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
    exit(0);
}
