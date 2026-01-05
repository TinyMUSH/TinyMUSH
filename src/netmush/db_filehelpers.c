/**
 * @file db_filehelpers.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief File and pipe manipulation helpers for database I/O
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

#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

FILE *t_fd;             /*!< Main BD file descriptor */
bool t_is_pipe = false; /*!< Are we piping from stdin? */

/**
 * @brief Close file/stream
 *
 * @param fd
 */
void tf_xclose(FILE *fd)
{
    if (fd)
    {
        if (t_is_pipe)
        {
            pclose(fd);
        }
        else
        {
            fclose(fd);
        }
    }
    else
    {
        close(0);
    }

    t_fd = NULL;
    t_is_pipe = false;
}

/**
 * @brief Fiddle file/stream
 *
 * @param tfd   Descriptor
 * @return int
 */
int tf_fiddle(int tfd)
{
    if (tfd < 0)
    {
        tfd = open(DEV_NULL, O_RDONLY, 0);
        return -1;
    }

    if (tfd != 0)
    {
        dup2(tfd, 0);
        close(tfd);
    }

    return 0;
}

/**
 * @brief Open a file
 *
 * @param fname Filename
 * @param mode  Mode
 * @return int
 */
int tf_xopen(char *fname, int mode)
{
    int fd = open(fname, mode, 0600);

    fd = tf_fiddle(fd);

    return fd;
}

/**
 * @brief Convert mode to char mode
 *
 * @param mode
 * @return const char*
 */
const char *mode_txt(int mode)
{
    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR))
    {
    case O_RDONLY:
        return "r";

    case O_WRONLY:
        return "w";
    }

    return "r+";
}

/**
 * @brief Initialize tf file handler
 *
 */
void tf_init(void)
{
    fclose(stdin);
    tf_xopen(DEV_NULL, O_RDONLY);
    t_fd = NULL;
    t_is_pipe = false;
}

/**
 * @brief Open file
 *
 * @param fname Filename
 * @param mode  Mode
 * @return int
 */
int tf_open(char *fname, int mode)
{
    tf_xclose(t_fd);

    return tf_xopen(fname, mode);
}

/**
 * @brief Close file
 *
 * @param fdes  File descriptor
 */
void tf_close(int fdes __attribute__((unused)))
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

/**
 * @brief Open file
 *
 * @param fname File
 * @param mode  Mode
 * @return FILE*
 */
FILE *tf_fopen(char *fname, int mode)
{
    tf_xclose(t_fd);

    if (tf_xopen(fname, mode) >= 0)
    {
        t_fd = fdopen(0, mode_txt(mode));
        return t_fd;
    }

    return NULL;
}

/**
 * @brief Close file
 *
 * @param fd File descriptor
 */
void tf_fclose(FILE *fd __attribute__((unused)))
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

/**
 * @brief Open file
 *
 * @param fname File
 * @param mode  File mode
 * @return FILE*
 */
FILE *tf_popen(char *fname, int mode)
{
    tf_xclose(t_fd);
    t_fd = popen(fname, mode_txt(mode));

    if (t_fd != NULL)
    {
        t_is_pipe = true;
    }

    return t_fd;
}
