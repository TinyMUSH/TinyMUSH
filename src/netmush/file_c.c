/**
 * @file file_c.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief File cache management
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

#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Text files cache.
 *
 */
FCACHE fcache[] = {
    {&mushconf.conn_file, NULL, "Conn"},
    {&mushconf.site_file, NULL, "Conn/Badsite"},
    {&mushconf.down_file, NULL, "Conn/Down"},
    {&mushconf.full_file, NULL, "Conn/Full"},
    {&mushconf.guest_file, NULL, "Conn/Guest"},
    {&mushconf.creg_file, NULL, "Conn/Reg"},
    {&mushconf.crea_file, NULL, "Crea/Newuser"},
    {&mushconf.regf_file, NULL, "Crea/RegFail"},
    {&mushconf.motd_file, NULL, "Motd"},
    {&mushconf.wizmotd_file, NULL, "Wizmotd"},
    {&mushconf.quit_file, NULL, "Quit"},
    {&mushconf.htmlconn_file, NULL, "Conn/Html"},
    {NULL, NULL, NULL}};

/**
 * @brief Show text file.
 *
 * @param player DBref of player
 * @param cause  Not used
 * @param extra  Not used
 * @param arg    File to show
 */
void do_list_file(dbref player, dbref cause __attribute__((unused)), int extra __attribute__((unused)), char *arg)
{
    int flagvalue;
    flagvalue = search_nametab(player, list_files, arg);

    if (flagvalue < 0)
    {
        display_nametab(player, list_files, true, (char *)"Unknown file.  Use one of:");
        return;
    }

    fcache_send(player, flagvalue);
}
/**
 * @brief Add date to a file block
 *
 * @param fp File block
 * @param ch character to add
 * @return FBLOCK* Updated file block.
 */
FBLOCK *fcache_fill(FBLOCK *fp, char ch)
{
    FBLOCK *tfp;

    if (fp->hdr.nchars >= (int)(MBUF_SIZE - sizeof(FBLKHDR)))
    {
        /**
         * We filled the current buffer.  Go get a new one.
         *
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

/**
 * @brief Read a file into cache
 *
 * @param cp Cache buffer
 * @param filename File to read
 * @return int Size of cached text.
 */
int fcache_read(FBLOCK **cp, char *filename)
{
    int n = 0, tchars = 0, fd = 0;
    ssize_t nmax = 0;
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

    if (filename)
    {
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
                return -1;
            }

            buff = XMALLOC(LBUF_SIZE, "buff");

            /**
             * Process the file, one lbuf at a time
             *
             */
            nmax = read(fd, buff, LBUF_SIZE);

            while (nmax != 0)
            {
                if (nmax < 0)
                {
                    if (errno == EINTR || errno == EAGAIN)
                    {
                        nmax = read(fd, buff, LBUF_SIZE);
                        continue;
                    }

                    log_write(LOG_PROBLEMS, "FIL", "READ", "Error reading file '%s': %s", filename, safe_strerror(errno));
                    XFREE(buff);
                    tf_close(fd);
                    return -1;
                }

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

            if (fd >= 0)
            {
                tf_close(fd);
            }
        }
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

/**
 * @brief Raw dump a cache file to a file descriptor
 *
 * @param fd File descriptor
 * @param num Index of the file in the file cache
 */
void fcache_rawdump(int fd, int num)
{
    ssize_t cnt, remaining;
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
                if (errno == EINTR)
                {
                    continue;
                }

                log_write(LOG_PROBLEMS, "FIL", "WRITE", "Error writing cached file %d: %s", num, safe_strerror(errno));
                return;
            }

            if (cnt == 0)
            {
                log_write(LOG_PROBLEMS, "FIL", "WRITE", "Zero-length write while dumping cached file %d", num);
                return;
            }

            remaining -= cnt;
            start += cnt;
        }

        fp = fp->hdr.nxt;
    }

    return;
}

/**
 * @brief Dump a file to a descriptor
 *
 * @param d Descriptor
 * @param num Index of the file in the file cache
 */
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

/**
 * @brief Send the content of a file cache to a player
 *
 * @param player DBref of player
 * @param num Index of the file in the file cache
 */
void fcache_send(dbref player, int num)
{
    DESC *d;

    for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
    {
        fcache_dump(d, num);
    }
}

/**
 * @brief Load all files into cache
 *
 * @param player DBref of player
 */
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

        if (i < 0)
        {
            log_write(LOG_PROBLEMS, "FIL", "LOAD", "Failed to load cached file '%s'", *(fp->filename));

            if ((player != NOTHING) && !Quiet(player))
            {
                XSPRINTF(sbuf, "cache load failed: %s", *(fp->filename));
                notify(player, sbuf);
            }

            continue;
        }

        if ((player != NOTHING) && !Quiet(player))
        {
            XSPRINTF(sbuf, "%d", i);

            if (fp == fcache)
            {
                XSAFELBSTR((char *)"File sizes: ", buff, &bufc);
            }
            else
            {
                XSAFELBSTR((char *)"  ", buff, &bufc);
            }

            XSAFELBSTR((char *)fp->desc, buff, &bufc);
            XSAFELBSTR((char *)"...", buff, &bufc);
            XSAFELBSTR(sbuf, buff, &bufc);
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

/**
 * @brief Initialize the file cache.
 *
 */
void fcache_init(void)
{
    FCACHE *fp;

    for (fp = fcache; fp->filename; fp++)
    {
        fp->fileblock = NULL;
    }

    fcache_load(NOTHING);
}
