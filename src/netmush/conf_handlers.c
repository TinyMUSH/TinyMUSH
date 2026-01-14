/**
 * @file conf_handlers.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration parameter type handlers
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
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>
#include <string.h>

extern CONFDATA mushconf;
extern STATEDATA mushstate;

/**
 * @brief Read-only integer or boolean parameter.
 *
 * @param vp        Not used
 * @param str       Not used
 * @param extra     Not used
 * @param player    Dbref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_const(int *vp __attribute__((unused)), char *str __attribute__((unused)), long extra __attribute__((unused)), dbref player, char *cmd)
{
    /**
     * Fail on any attempt to change the value
     *
     */
    cf_log(player, "CNF", "SYNTX", cmd, "Cannot change a constant value");
    return CF_Failure;
}

/**
 * @brief Set integer parameter.
 *
 * @param vp        Numeric value
 * @param str       String buffer
 * @param extra     Maximum limit
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_int(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *endptr = NULL;
    long val = 0;

    /**
     * Copy the numeric value to the parameter.
     * Use strtol with proper error handling.
     *
     */
    errno = 0;
    val = strtol(str, &endptr, 10);

    /**
     * Validate conversion: check for range errors and invalid input
     *
     */
    if (errno == ERANGE || val > INT_MAX || val < INT_MIN)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value out of range or too large");
        return CF_Failure;
    }

    /**
     * Verify entire string was consumed (no trailing garbage)
     *
     */
    if (*endptr != '\0' && !isspace(*endptr))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid numeric format: %s", str);
        return CF_Failure;
    }

    /**
     * Check against upper limit if specified
     *
     */
    if ((extra > 0) && (val > extra))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %ld", extra);
        return CF_Failure;
    }

    *vp = (int)val;
    return CF_Success;
}

/**
 * @brief Set integer parameter that will be used as a factor (ie. cannot be set to 0)
 *
 * @param vp        Numeric value
 * @param str       String buffer
 * @param extra     Maximum limit
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_int_factor(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *endptr = NULL;
    long num = 0;

    /**
     * Parse integer with proper error handling.
     * Factors cannot be 0, so we check this explicitly.
     *
     */
    errno = 0;
    num = strtol(str, &endptr, 10);

    /**
     * Validate conversion: check for range errors and invalid input
     *
     */
    if (errno == ERANGE || num > INT_MAX || num < INT_MIN)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value out of range or too large");
        return CF_Failure;
    }

    /**
     * Verify entire string was consumed
     *
     */
    if (*endptr != '\0' && !isspace(*endptr))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid numeric format: %s", str);
        return CF_Failure;
    }

    /**
     * Check against upper limit if specified
     *
     */
    if ((extra > 0) && (num > extra))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value exceeds limit of %ld", extra);
        return CF_Failure;
    }

    /**
     * Factor cannot be zero
     *
     */
    if (num == 0)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Value cannot be 0. You may want a value of 1.");
        return CF_Failure;
    }

    *vp = (int)num;
    return CF_Success;
}

/**
 * @brief Set dbref parameter.
 *
 * @param vp        Numeric value
 * @param str       String buffer
 * @param extra     Maximum limit
 * @param player    DBref of the player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_dbref(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *endptr = NULL;
    long num = 0;

    /**
     * No consistency check on initialization.
     * Accept any value during startup.
     *
     */
    if (mushstate.initializing)
    {
        if (*str == '#')
        {
            errno = 0;
            num = strtol(str + 1, &endptr, 10);
            if (errno == ERANGE || num > INT_MAX || num < INT_MIN)
            {
                cf_log(player, "CNF", "SYNTX", cmd, "DBref value out of range");
                return CF_Failure;
            }
        }
        else
        {
            errno = 0;
            num = strtol(str, &endptr, 10);
            if (errno == ERANGE || num > INT_MAX || num < INT_MIN)
            {
                cf_log(player, "CNF", "SYNTX", cmd, "DBref value out of range");
                return CF_Failure;
            }
        }

        *vp = (int)num;
        return CF_Success;
    }

    /**
     * Otherwise we have to validate this. If 'extra' is non-zero (NOTHING),
     * the dbref is allowed to be NOTHING (-1).
     *
     */
    if (*str == '#')
    {
        errno = 0;
        num = strtol(str + 1, &endptr, 10);
    }
    else
    {
        errno = 0;
        num = strtol(str, &endptr, 10);
    }

    /**
     * Validate conversion: check for range errors and invalid input
     *
     */
    if (errno == ERANGE || num > INT_MAX || num < INT_MIN)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "DBref value out of range");
        return CF_Failure;
    }

    /**
     * Verify entire string was consumed (except for leading # if present)
     *
     */
    if (*endptr != '\0' && !isspace(*endptr))
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid DBref format: %s", str);
        return CF_Failure;
    }

    /**
     * Validate the dbref is either NOTHING (if allowed) or a valid object
     *
     */
    if (((extra == NOTHING) && (num == NOTHING)) || (Good_obj(num) && !Going(num)))
    {
        *vp = (int)num;
        return CF_Success;
    }

    /**
     * Report appropriate error message
     *
     */
    if (extra == NOTHING)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "A valid dbref, or -1, is required.");
    }
    else
    {
        cf_log(player, "CNF", "SYNTX", cmd, "A valid dbref is required.");
    }

    return CF_Failure;
}

/**
 * @brief Open a loadable module. Modules are initialized later in the startup.
 *
 * @param vp
 * @param modname
 * @param extra
 * @param player
 * @param cmd
 * @return int
 */
CF_Result cf_module(int *vp __attribute__((unused)), char *modname, long extra __attribute__((unused)), dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    void *handle;
    void (*initptr)(void) = NULL;
    MODULE *mp = NULL;

    handle = dlopen_format("%s/lib%s.so", mushconf.modules_home, modname);

    if (!handle)
    {
        log_write(LOG_STARTUP, "CNF", "MOD", "Loading of %s/lib%s.so module failed: %s", mushconf.modules_home, modname, dlerror());
        return CF_Failure;
    }

    mp = (MODULE *)XMALLOC(sizeof(MODULE), "mp");
    mp->modname = XSTRDUP(modname, "mp->modname");
    mp->handle = handle;
    mp->next = mushstate.modules_list;
    mushstate.modules_list = mp;

    /**
     * Look up our symbols now, and cache the pointers. They're not going
     * to change from here on out.
     *
     */
    mp->process_command = (int (*)(dbref, dbref, int, char *, char *[], int))dlsym_format(handle, "mod_%s_%s", modname, "process_command");
    mp->process_no_match = (int (*)(dbref, dbref, int, char *, char *, char *[], int))dlsym_format(handle, "mod_%s_%s", modname, "process_no_match");
    mp->did_it = (int (*)(dbref, dbref, dbref, int, const char *, int, const char *, int, int, char *[], int, int))dlsym_format(handle, "mod_%s_%s", modname, "did_it");
    mp->create_obj = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", modname, "create_obj");
    mp->destroy_obj = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", modname, "destroy_obj");
    mp->create_player = (void (*)(dbref, dbref, int, int))dlsym_format(handle, "mod_%s_%s", modname, "create_player");
    mp->destroy_player = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", modname, "destroy_player");
    mp->announce_connect = (void (*)(dbref, const char *, int))dlsym_format(handle, "mod_%s_%s", modname, "announce_connect");
    mp->announce_disconnect = (void (*)(dbref, const char *, int))dlsym_format(handle, "mod_%s_%s", modname, "announce_disconnect");
    mp->examine = (void (*)(dbref, dbref, dbref, int, int))dlsym_format(handle, "mod_%s_%s", modname, "examine");
    mp->dump_database = (void (*)(FILE *))dlsym_format(handle, "mod_%s_%s", modname, "dump_database");
    mp->db_grow = (void (*)(int, int))dlsym_format(handle, "mod_%s_%s", modname, "db_grow");
    mp->db_write = (void (*)(void))dlsym_format(handle, "mod_%s_%s", modname, "db_write");
    mp->db_write_flatfile = (void (*)(FILE *))dlsym_format(handle, "mod_%s_%s", modname, "db_write_flatfile");
    mp->do_second = (void (*)(void))dlsym_format(handle, "mod_%s_%s", modname, "do_second");
    mp->cache_put_notify = (void (*)(UDB_DATA, unsigned int))dlsym_format(handle, "mod_%s_%s", modname, "cache_put_notify");
    mp->cache_del_notify = (void (*)(UDB_DATA, unsigned int))dlsym_format(handle, "mod_%s_%s", modname, "cache_del_notify");

    if (!mushstate.standalone)
    {
        if ((initptr = (void (*)(void))dlsym_format(handle, "mod_%s_%s", modname, "init")) != NULL)
        {
            (*initptr)();
        }
    }

    log_write(LOG_STARTUP, "CNF", "MOD", "Loaded module: %s", modname);
    return CF_Success;
}

/**
 * @brief Set boolean parameter.
 *
 * @param vp        Boolean value (0 or 1)
 * @param str       String buffer ("yes"/"no", "true"/"false", etc)
 * @param extra     Not used
 * @param player    DBref of player (not used)
 * @param cmd       Command (not used)
 * @return CF_Result
 */
CF_Result cf_bool(int *vp, char *str, long extra __attribute__((unused)), dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    *vp = (int)search_nametab(GOD, bool_names, str);

    if (*vp < 0)
    {
        *vp = (long)0;
    }

    return CF_Success;
}

/**
 * @brief Select one option from many choices.
 *
 * @param vp        Index of the option
 * @param str       String buffer
 * @param extra     Name Table
 * @param player    DBref of the player
 * @param cmd       Command
 * @return int      -1 on Failure, 0 on Success
 */
CF_Result cf_option(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int i = search_nametab(GOD, (NAMETAB *)extra, str);

    if (i < 0)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Value", str);
        return CF_Failure;
    }

    *vp = i;
    return CF_Success;
}

/**
 * @brief Set string parameter.
 *
 * @param vp        Variable type
 * @param str       String buffer
 * @param extra     Maximum string length
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_string(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int retval = CF_Success;
    /**
     * Make a copy of the string if it is not too big
     *
     */

    if (strlen(str) >= (unsigned int)extra)
    {
        str[extra - 1] = '\0';

        if (mushstate.initializing)
        {
            log_write(LOG_STARTUP, "CNF", "NFND", "%s: String truncated", cmd);
        }
        else
        {
            notify(player, "String truncated");
        }

        retval = CF_Failure;
    }

    XFREE(*(char **)vp);
    *(char **)vp = XSTRDUP(str, "vp");
    return retval;
}

/**
 * @brief Define a generic hash table alias.
 *
 * @param vp        Hash table pointer
 * @param str       Alias definition string (alias=original)
 * @param extra     Error message string
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result indicating success or failure
 */
CF_Result cf_alias(int *vp, char *str, long extra, dbref player, char *cmd)
{
    int *cp = NULL, upcase = 0;
    char *p = NULL, *tokst = NULL;
    char *alias = strtok_r(str, " \t=,", &tokst);
    char *orig = strtok_r(NULL, " \t=,", &tokst);

    if (orig)
    {
        upcase = 0;

        for (p = orig; *p; p++)
        {
            *p = tolower(*p);
        }

        cp = hashfind(orig, (HASHTAB *)vp);

        if (cp == NULL)
        {
            upcase++;

            for (p = orig; *p; p++)
            {
                *p = toupper(*p);
            }

            cp = hashfind(orig, (HASHTAB *)vp);

            if (cp == NULL)
            {
                cf_log(player, "CNF", "NFND", cmd, "%s %s not found", (char *)extra, str);
                return CF_Failure;
            }
        }

        if (upcase)
        {
            for (p = alias; *p; p++)
            {
                *p = toupper(*p);
            }
        }
        else
        {
            for (p = alias; *p; p++)
            {
                *p = tolower(*p);
            }
        }

        if (((HASHTAB *)vp)->flags & HT_KEYREF)
        {
            /**
             * hashadd won't copy it, so we do that here
             *
             */
            p = alias;
            alias = XSTRDUP(p, "alias");
        }

        return hashadd(alias, cp, (HASHTAB *)vp, HASH_ALIAS);
    }
    else
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Invalid original for alias %s", alias);
        return CF_Failure;
    }
}
