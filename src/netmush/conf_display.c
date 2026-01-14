/**
 * @file conf_display.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration display and verification functions
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
 * @brief Walk all configuration tables and validate any dbref values.
 *
 */
void cf_verify(void)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    for (tp = conftable; tp->pname; tp++)
    {
        if (tp->interpreter == cf_dbref)
        {
            if (!(((tp->extra == NOTHING) && (*(tp->loc) == NOTHING)) || (Good_obj(*(tp->loc)) && !Going(*(tp->loc)))))
            {
                log_write(LOG_ALWAYS, "CNF", "VRFY", "%s #%d is invalid. Reset to #%d.", tp->pname, *(tp->loc), tp->extra);
                *(tp->loc) = (dbref)tp->extra;
            }
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (tp->interpreter == cf_dbref)
                {
                    if (!(((tp->extra == NOTHING) && (*(tp->loc) == NOTHING)) || (Good_obj(*(tp->loc)) && !Going(*(tp->loc)))))
                    {
                        log_write(LOG_ALWAYS, "CNF", "VRFY", "%s #%d is invalid. Reset to #%d.", tp->pname, *(tp->loc), tp->extra);
                        *(tp->loc) = (dbref)tp->extra;
                    }
                }
            }
        }
    }
}

/**
 * @brief Helper function for cf_display
 *
 * @param player    DBref of player
 * @param buff      Output buffer
 * @param bufc      Output buffer tracker
 * @param tp        Config parameter
 */
void helper_cf_display(dbref player, char *buff, char **bufc, CONF *tp)
{
    NAMETAB *opt = NULL;

    if (!check_access(player, tp->rperms))
    {
        XSAFENOPERM(buff, bufc);
        return;
    }

    if ((tp->interpreter == cf_bool) || (tp->interpreter == cf_int) || (tp->interpreter == cf_int_factor) || (tp->interpreter == cf_const))
    {
        XSAFELTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
        return;
    }

    if (tp->interpreter == cf_string)
    {
        XSAFELBSTR(*((char **)tp->loc), buff, bufc);
        return;
    }

    if (tp->interpreter == cf_dbref)
    {
        XSAFELBCHR('#', buff, bufc);
        XSAFELTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
        return;
    }

    if (tp->interpreter == cf_option)
    {
        opt = find_nametab_ent_flag(GOD, (NAMETAB *)tp->extra, *(tp->loc));
        XSAFELBSTR((opt ? opt->name : "*UNKNOWN*"), buff, bufc);
        return;
    }

    XSAFENOPERM(buff, bufc);
    return;
}

/**
 * @brief Given a config parameter by name, return its value in some sane fashion.
 *
 * @param player        DBref of player
 * @param param_name    Parameter name
 * @param buff          Output buffer
 * @param bufc          Output buffer tracker
 */
void cf_display(dbref player, char *param_name, char *buff, char **bufc)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    for (tp = conftable; tp->pname; tp++)
    {
        if (!strcasecmp(tp->pname, param_name))
        {
            helper_cf_display(player, buff, bufc, tp);
            return;
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (!strcasecmp(tp->pname, param_name))
                {
                    helper_cf_display(player, buff, bufc, tp);
                    return;
                }
            }
        }
    }

    XSAFENOMATCH(buff, bufc);
}

/**
 * @brief List write or read access to config directives.
 *
 * @param player DBref of player
 */
void list_cf_access(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;

    notify(player, "Attribute                      Permission");
    notify(player, "------------------------------ ------------------------------------------------");
    for (tp = conftable; tp->pname; tp++)
    {
        if (God(player) || check_access(player, tp->flags))
        {
            listset_nametab(player, access_nametab, tp->flags, true, "%-30.30s ", tp->pname);
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (God(player) || check_access(player, tp->flags))
                {
                    listset_nametab(player, access_nametab, tp->flags, true, "%-30.30s ", tp->pname);
                }
            }
        }
    }
    notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List read access to config directives.
 *
 * @param player DBref of player
 */
void list_cf_read_access(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    char *buff = NULL;

    notify(player, "Attribute                      Permission");
    notify(player, "------------------------------ ------------------------------------------------");
    for (tp = conftable; tp->pname; tp++)
    {
        if (God(player) || check_access(player, tp->rperms))
        {
            buff = XASPRINTF("buff", "%-30.30s ", tp->pname);
            listset_nametab(player, access_nametab, tp->rperms, true, buff);
            XFREE(buff);
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            for (tp = ctab; tp->pname; tp++)
            {
                if (God(player) || check_access(player, tp->rperms))
                {
                    buff = XASPRINTF("buff", "%-30.30s ", tp->pname);
                    listset_nametab(player, access_nametab, tp->rperms, true, buff);
                    XFREE(buff);
                }
            }
        }
    }
    notify(player, "-------------------------------------------------------------------------------");
}

/**
 * @brief List options to player
 *
 * @param player DBref of player
 */
void list_options(dbref player)
{
    CONF *tp = NULL, *ctab = NULL;
    MODULE *mp = NULL;
    bool draw_header = false;

    notify(player, "Global Options            S Description");
    notify(player, "------------------------- - ---------------------------------------------------");
    for (tp = conftable; tp->pname; tp++)
    {
        if (((tp->interpreter == cf_const) || (tp->interpreter == cf_bool)) && (check_access(player, tp->rperms)))
        {
            raw_notify(player, "%-25s %c %s?", tp->pname, (*(tp->loc) ? 'Y' : 'N'), (tp->extra ? (char *)tp->extra : ""));
        }
    }

    for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
        {
            draw_header = false;
            for (tp = ctab; tp->pname; tp++)
            {
                if (((tp->interpreter == cf_const) || (tp->interpreter == cf_bool)) && (check_access(player, tp->rperms)))
                {
                    if (!draw_header)
                    {
                        raw_notify(player, "\nModule %-18.18s S Description", mp->modname);
                        notify(player, "------------------------- - ---------------------------------------------------");
                        draw_header = true;
                    }
                    raw_notify(player, "%-25s %c %s?", tp->pname, (*(tp->loc) ? 'Y' : 'N'), (tp->extra ? (char *)tp->extra : ""));
                }
            }
        }
    }
    notify(player, "-------------------------------------------------------------------------------");
}
