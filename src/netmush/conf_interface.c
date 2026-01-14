/**
 * @file conf_interface.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration interface and main command handlers
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
#include <string.h>
#include <dlfcn.h>

extern CONFDATA mushconf;
extern STATEDATA mushstate;
extern int (*cf_interpreter)(int *, char *, long, dbref, char *);

/**
 * @brief Set config parameter with access and logging.
 *
 * @param cp       Configuration parameter string
 * @param ap       Argument parameter string
 * @param player   DBref of player executing command
 * @param tp       Configuration table entry
 * @return CF_Result indicating success or failure
 */
CF_Result helper_cf_set(char *cp, char *ap, dbref player, CONF *tp)
{
    int i = 0, r = CF_Failure;
    char *buf = NULL, *buff = NULL, *name = NULL, *status = NULL;

    if (!mushstate.standalone && !mushstate.initializing && !check_access(player, tp->flags))
    {
        notify(player, NOPERM_MESSAGE);
    }
    else
    {
        if (!mushstate.initializing)
        {
            buff = XSTRDUP(ap, "buff");
        }

        cf_interpreter = tp->interpreter;
        i = cf_interpreter(tp->loc, ap, tp->extra, player, cp);

        if (!mushstate.initializing)
        {
            name = log_getname(player);

            switch (i)
            {
            case 0:
                r = CF_Success;
                status = XSTRDUP("Success.", "status");
                break;

            case 1:
                r = CF_Partial;
                status = XSTRDUP("Partial success.", "status");
                break;

            case -1:
                r = CF_Failure;
                status = XSTRDUP("Failure.", "status");
                break;

            default:
                r = CF_Failure;
                status = XSTRDUP("Strange.", "status");
            }

            buf = ansi_strip_ansi(buff);
            log_write(LOG_CONFIGMODS, "CFG", "UPDAT", "%s entered config directive: %s with args '%s'. Status: %s", name, cp, buf, status);
            XFREE(buf);
            XFREE(name);
            XFREE(status);
            XFREE(buff);
        }
    }

    return r;
}

/**
 * @brief Set a configuration directive.
 *
 * @param cp       Configuration parameter name
 * @param ap       Argument value
 * @param player   DBref of player
 * @return CF_Result indicating success or failure
 */
CF_Result cf_set(char *cp, char *ap, dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    /**
     * Search the config parameter table for the command. If we find it,
     * call the handler to parse the argument. Make sure that if we're standalone,
     * the paramaters we need to load module flatfiles are loaded
     */

    if (mushstate.standalone && strcmp(cp, "module") && strcmp(cp, "database_home"))
    {
        return CF_Success;
    }

    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcmp(tp->pname, cp))
        {
            return (helper_cf_set(cp, ap, player, tp));
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcmp(tp->pname, cp))
                {
                    return (helper_cf_set(cp, ap, player, tp));
                }
            }
        }
    }

    /**
     * Config directive not found.  Complain about it.
     *
     */
    if (!mushstate.standalone)
    {
        cf_log(player, "CNF", "NFND", (char *)"Set", "%s %s not found", "Config directive", cp);
    }

    return CF_Failure;
}

/**
 * @brief Read in configuration parameters from named file.
 *
 * @param fn        Filename to read
 * @return CF_Result indicating success or failure
 */
CF_Result cf_read(char *fn)
{
    int retval = cf_include(NULL, fn, 0, 0, (char *)"init");

    return retval;
}

/**
 * @brief Command handler to set configuration parameters at runtime.
 *
 * @param player    DBref of player executing command
 * @param cause     DBref of cause (not used)
 * @param extra     Extra flags (not used)
 * @param kw        Keyword (parameter name)
 * @param value     New value for parameter
 * @return void
 */
void do_admin(dbref player, dbref cause __attribute__((unused)), int extra __attribute__((unused)), char *kw, char *value)
{
    int i = cf_set(kw, value, player);

    if ((i >= 0) && !Quiet(player))
    {
        notify(player, "Set.");
    }

    return;
}
