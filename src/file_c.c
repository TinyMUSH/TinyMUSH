/* file_c.c - File cache management */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"   /* required by mudconf */
#include "game.h"       /* required by mudconf */
#include "alloc.h"      /* required by mudconf */
#include "flags.h"      /* required by mudconf */
#include "htab.h"       /* required by mudconf */
#include "ltdl.h"       /* required by mudconf */
#include "udb.h"        /* required by mudconf */
#include "udb_defs.h"   /* required by mudconf */
#include "mushconf.h"   /* required by code */
#include "db.h"         /* required by externs */
#include "interface.h"  /* required by code */
#include "externs.h"    /* required by interface */
#include "command.h"    /* required by code */
#include "file_c.h"     /* required by code */
#include "stringutil.h" /* required by code */
#include "nametabs.h"

#define FBLOCK_SIZE (MBUF_SIZE - sizeof(FBLKHDR))
/* *INDENT-OFF* */

FCACHE fcache[] = {
    {&mudconf.conn_file, NULL, "Conn"},
    {&mudconf.site_file, NULL, "Conn/Badsite"},
    {&mudconf.down_file, NULL, "Conn/Down"},
    {&mudconf.full_file, NULL, "Conn/Full"},
    {&mudconf.guest_file, NULL, "Conn/Guest"},
    {&mudconf.creg_file, NULL, "Conn/Reg"},
    {&mudconf.crea_file, NULL, "Crea/Newuser"},
    {&mudconf.regf_file, NULL, "Crea/RegFail"},
    {&mudconf.motd_file, NULL, "Motd"},
    {&mudconf.wizmotd_file, NULL, "Wizmotd"},
    {&mudconf.quit_file, NULL, "Quit"},
    {&mudconf.htmlconn_file, NULL, "Conn/Html"},
    {NULL, NULL, NULL}};

/* *INDENT-ON* */

void do_list_file(dbref player, dbref cause __attribute__((unused)), int extra __attribute__((unused)), char *arg)
{
    int flagvalue;
    flagvalue = search_nametab(player, list_files, arg);

    if (flagvalue < 0)
    {
        display_nametab(player, list_files, (char *)"Unknown file.  Use one of:", 1);
        return;
    }

    fcache_send(player, flagvalue);
}

FBLOCK *fcache_fill(FBLOCK *fp, char ch)
{
    FBLOCK *tfp;

    if (fp->hdr.nchars >= (int)(MBUF_SIZE - sizeof(FBLKHDR)))
    {
        /*
	 * We filled the current buffer.  Go get a new one.
	 */
        tfp = fp;
        fp = (FBLOCK *)XMALLOC(MBUF_SIZE, "fp");
        fp->hdr.nxt = NULL;
        fp->hdr.nchars = 0;
        tfp->hdr.nxt = fp;
    }

    fp->data[fp->hdr.nchars++] = ch;
    return fp;
}

int fcache_read(FBLOCK **cp, char *filename)
{
    int n = 0, nmax = 0, tchars = 0, fd = 0;
    char *buff = NULL;
    FBLOCK *fp = *cp, *tfp = NULL;

    /**
     * Free a prior buffer chain
     * 
     */

    while (fp != NULL)
    {
        tfp = fp->hdr.nxt;
        XFREE(fp);
        fp = tfp;
    }

    *cp = NULL;

    /**
     * Set up the initial cache buffer to make things easier
     * 
     */
    fp = (FBLOCK *)XMALLOC(MBUF_SIZE, "fp");
    fp->hdr.nxt = NULL;
    fp->hdr.nchars = 0;
    *cp = fp;
    tchars = 0;

    if (*filename)
    {
        /**
         * Read the text file into a new chain
         * 
         */
        if ((fd = tf_open(filename, O_RDONLY)) == -1)
        {
            /**
        	 * Failure: log the event
             * 
        	 */
            log_write(LOG_PROBLEMS, "FIL", "OPEN", "Couldn't open file '%s'.", filename);
            tf_close(fd);
            return -1;
        }

        buff = XMALLOC(LBUF_SIZE, "buff");

        /**
         * Process the file, one lbuf at a time
         * 
         */
        nmax = read(fd, buff, LBUF_SIZE);

        while (nmax > 0)
        {
            for (n = 0; n < nmax; n++)
            {
                switch (buff[n])
                {
                case '\n':
                    fp = fcache_fill(fp, '\r');
                    fp = fcache_fill(fp, '\n');
                    tchars += 2;
                    break;
                case '\0':
                case '\r':
                    break;
                default:
                    fp = fcache_fill(fp, buff[n]);
                    tchars++;
                    break;
                }
            }

            nmax = read(fd, buff, LBUF_SIZE);
        }

        XFREE(buff);
        tf_close(fd);
    }

    /**
     * If we didn't read anything in, toss the initial buffer
     * 
     */
    if (fp->hdr.nchars == 0)
    {
        *cp = NULL;
        XFREE(fp);
    }

    return tchars;
}

void fcache_rawdump(int fd, int num)
{
    int cnt, remaining;
    char *start;
    FBLOCK *fp;

    if ((num < 0) || (num > FC_LAST))
    {
        return;
    }

    fp = fcache[num].fileblock;

    while (fp != NULL)
    {
        start = fp->data;
        remaining = fp->hdr.nchars;

        while (remaining > 0)
        {
            cnt = write(fd, start, remaining);

            if (cnt < 0)
            {
                return;
            }

            remaining -= cnt;
            start += cnt;
        }

        fp = fp->hdr.nxt;
    }

    return;
}

void fcache_dump(DESC *d, int num)
{
    FBLOCK *fp;

    if ((num < 0) || (num > FC_LAST))
    {
        return;
    }

    fp = fcache[num].fileblock;

    while (fp != NULL)
    {
        queue_write(d, fp->data, fp->hdr.nchars);
        fp = fp->hdr.nxt;
    }
}

void fcache_send(dbref player, int num)
{
    DESC *d;
    DESC_ITER_PLAYER(player, d)
    {
        fcache_dump(d, num);
    }
}

void fcache_load(dbref player)
{
    FCACHE *fp;
    char *buff, *bufc, *sbuf;
    int i;
    buff = bufc = XMALLOC(LBUF_SIZE, "lbuf");
    sbuf = XMALLOC(SBUF_SIZE, "sbuf");

    for (fp = fcache; fp->filename; fp++)
    {
        i = fcache_read(&fp->fileblock, *(fp->filename));

        if ((player != NOTHING) && !Quiet(player))
        {
            XSPRINTF(sbuf, "%d", i);

            if (fp == fcache)
            {
                SAFE_LB_STR((char *)"File sizes: ", buff, &bufc);
            }
            else
            {
                SAFE_LB_STR((char *)"  ", buff, &bufc);
            }

            SAFE_LB_STR((char *)fp->desc, buff, &bufc);
            SAFE_LB_STR((char *)"...", buff, &bufc);
            SAFE_LB_STR(sbuf, buff, &bufc);
        }
    }

    *bufc = '\0';

    if ((player != NOTHING) && !Quiet(player))
    {
        notify(player, buff);
    }

    XFREE(buff);
    XFREE(sbuf);
}

void fcache_init(void)
{
    FCACHE *fp;

    for (fp = fcache; fp->filename; fp++)
    {
        fp->fileblock = NULL;
    }

    fcache_load(NOTHING);
}
