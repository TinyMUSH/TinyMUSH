/*  logwatch.c - A very basic log watcher */
/* $Id: logwatch.c,v 1.1 2004/10/12 19:42:53 tyrspace Exp $ */

#include "copyright.h"
#include "autoconf.h"

extern char *optarg;

int
main(argc, argv)
int argc;

char *argv[];
{
    char *searchstr = (char *)NULL;

    char *logfile = (char *)NULL;

    char *s = (char *)NULL;

    FILE *fptr;

    int c = 0, delay = 0, timeout = 30;

    long pos = 0, newpos;

    /*
     * Parse the command line
     */

    while ((c = getopt(argc, argv, "s:l:t:")) != -1)
    {
        switch (c)
        {
        case 's':
            searchstr = optarg;
            break;
        case 'l':
            logfile = optarg;
            break;
        case 't':
            timeout = atoi(optarg);
            if (timeout < 1)
            {
                fprintf(stderr,
                        "Warning - Invalid timeout specified.\n"
                        "Using default value of 30 seconds\n");
                timeout = 30;
            }
            break;
        }

    }

    /*
     * Die if we don't have everything we need
     */

    if ((searchstr == (char *)NULL) || (logfile == (char *)NULL))
    {
        fprintf(stderr,
                "Usage : %s -l <logfile> -s <searchstring> [-t <timeout>]\n",
                argv[0]);
        exit(1);
    }

    c = 0;

    /*
     * Open the logfile, die if we can't
     */

    if ((fptr = fopen(logfile, "r")) == NULL)
    {
        fprintf(stderr, "Error - Unable to open %s.\n", logfile);
        exit(1);
    }

    s = (char *)malloc(1024);

    while (c == 0)
    {
        /*
         * Find the length of the logfile
         */

        fseek(fptr, (long)0, SEEK_END);
        newpos = ftell(fptr);

        /*
         * file grew? print the new content
         */
        if (newpos > pos)
        {
            fseek(fptr, pos, SEEK_SET);
            while (fgets(s, 1024, fptr) != NULL)
            {
                fprintf(stdout, "%s", s);
                if (strstr(s, searchstr) != NULL)
                {
                    c++;
                    break;
                }
            }
            pos = ftell(fptr);
        }
        else if (delay < timeout)
        {
            sleep(1);
            delay++;
        }
        else
        {
            fprintf(stderr,
                    "Timeout - String '%s' not found in '%s'. Giving up.\n",
                    searchstr, logfile);
            break;
        }
    }

    free(s);

    fclose(fptr);
    exit(0);
}
