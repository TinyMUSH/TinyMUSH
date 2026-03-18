/**
 * @file funvars.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Q-register index helpers (qidx_chartab, qidx_str)
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

#include <ctype.h>
#include <string.h>
#include <pcre2.h>


/*
 * ---------------------------------------------------------------------------
 * setq, setr, r: set and read global registers.
 */

/**
 * @brief Convert ascii characters to global register (%q?) id
 *
 * @param ch    ascii character to convert
 * @return char global register id
 */
char qidx_chartab(int ch)
{
    if (ch > (86 + mushconf.max_global_regs))
    { // > z
        return -1;
    }
    else if (ch >= 97)
    { // >= a
        return ch - 87;
    }
    else if (ch > (54 + mushconf.max_global_regs))
    { // > Z
        return -1;
    }
    else if (ch >= 65)
    { // >= A
        return ch - 55;
    }
    else if (ch > 57)
    { // > 9
        return -1;
    }
    else if (ch >= 48)
    { // >= 0
        return ch - 48;
    }
    return -1;
}

/**
 * @brief convert global register (%q?) id to ascii character
 *
 * @param id    global register id
 * @return char ascii code of the register
 */
char qidx_str(int id)
{
    if (id > 35)
    {
        return 0;
    }
    else if (id >= 10)
    { // > z
        return id + 87;
    }
    else if (id >= 0)
    {
        return id + 48;
    }
    return 0;
}

