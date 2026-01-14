/**
 * @file conf_access.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration access control
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

/**
 * @brief Set write or read access on config directives kludge
 *
 * this cf handler uses vp as an extra extra field since the first extra field
 * is taken up with the access nametab.
 *
 * @param tp        Config Parameter
 * @param player    DBrief of player
 * @param vp        Extra field
 * @param ap        String buffer
 * @param cmd       Command
 * @param extra     Extra field
 * @return CF_Result
 */
CF_Result helper_cf_cf_access(CONF *tp, dbref player, int *vp, char *ap, char *cmd, long extra)
{
    /**
     * Cannot modify parameters set STATIC
     *
     */
    char *name = NULL;

    if (tp->flags & CA_STATIC)
    {
        notify(player, NOPERM_MESSAGE);

        if (db)
        {
            name = log_getname(player);
            log_write(LOG_CONFIGMODS, "CFG", "PERM", "%s tried to change %s access to static param: %s", name, (((long)vp) ? "read" : "write"), tp->pname);
            XFREE(name);
        }
        else
        {
            log_write(LOG_CONFIGMODS, "CFG", "PERM", "System tried to change %s access to static param: %s", (((long)vp) ? "read" : "write"), tp->pname);
        }

        return CF_Failure;
    }

    if ((long)vp)
    {
        return (cf_modify_bits(&tp->rperms, ap, extra, player, cmd));
    }
    else
    {
        return (cf_modify_bits(&tp->flags, ap, extra, player, cmd));
    }
}

/**
 * @brief Set configuration parameter access
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_cf_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *ap = NULL;

    for (ap = str; *ap && !isspace(*ap); ap++)
        ;

    if (*ap)
    {
        *ap++ = '\0';
    }

    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcmp(tp->pname, str))
        {
            return (helper_cf_cf_access(tp, player, vp, ap, cmd, extra));
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcmp(tp->pname, str))
                {
                    return (helper_cf_cf_access(tp, player, vp, ap, cmd, extra));
                }
            }
        }
    }

    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config directive", str);
    return CF_Failure;
}
