/**
 * @file version.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Version information
 * @version 4.0
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

#include <inttypes.h>
#include <string.h>
#include <sys/utsname.h>
#include <dlfcn.h>

void do_version(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int extra)
{
    MODVER *mver;
    char *ptr;
    char string[MBUF_SIZE];
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "\nTinyMUSH Engine\n---------------");
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Version : %s", mushstate.version.versioninfo);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "     Author : %s", TINYMUSH_AUTHOR);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "      Email : %s", TINYMUSH_CONTACT);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Website : %s", TINYMUSH_HOMEPAGE_URL);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "  Copyright : %s", TINYMUSH_COPYRIGHT);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Licence : %s", TINYMUSH_LICENSE);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, " Build date : %s", TINYMUSH_BUILD_DATE);

    if (Wizard(player))
    {
        struct utsname bpInfo;

        if (uname(&bpInfo) == 0)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "   Platform : %s %s %s %s %s", bpInfo.sysname, bpInfo.nodename, bpInfo.release, bpInfo.version, bpInfo.machine);
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "   Platform : unavailable (uname failed)");
        }
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, " ");

    if (mushstate.modloaded[0])
    {
        MODULE *mp;

        for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
        {
            XSNPRINTF(string, MBUF_SIZE, "Module %s", mp->modname);
            ptr = repeatchar(strlen(string), '-');
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s\n%s", string, ptr);
            XFREE(ptr);
            XSNPRINTF(string, MBUF_SIZE, "mod_%s_%s", mp->modname, "version");

            if ((mver = (MODVER *)dlsym(mp->handle, string)) != NULL)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Version : %s", mver->version);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "     Author : %s", mver->author);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "      Email : %s", mver->email);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Website : %s", mver->url);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "  Copyright : %s", mver->copyright);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Description : %s\n", mver->description);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Version : No version information for module %s", mp->modname);
            }
        }
    }
}

void format_version(void)
{
    static char buf[MBUF_SIZE];
    XSNPRINTF(buf, MBUF_SIZE, "%d.%d %s", mushstate.version.major, mushstate.version.minor, mushstate.version.status == 0 ? "alpha" : mushstate.version.status == 1 ? "beta"
                                                                                                                                  : mushstate.version.status == 2   ? "rc"
                                                                                                                                                                    : "stable");
    if (mushstate.version.patch > 0)
        XSPRINTFCAT(buf, " Patch %d", mushstate.version.patch);
    if (mushstate.version.tweak > 0)
        XSPRINTFCAT(buf, " Revision %d", mushstate.version.tweak);
    XSPRINTFCAT(buf, " (%s%s %s)", mushstate.version.git_hash, mushstate.version.git_dirty ? "-dirty" : "", mushstate.version.git_date);

    if (mushstate.version.versioninfo)
    {
        XFREE(mushstate.version.versioninfo);
    }

    mushstate.version.versioninfo = XSTRDUP(buf, "mushstate.version.versioninfo");
}

void init_version(void)
{
    /* TinyMUSH 4.0 version scheme : Major.Minor.Patch.Tweak-Status
       Major  : The main branch.
       Minor  : The minor version.
       Patch  : Patch Level.
       Tweak  : Revision Level.
       Status : 0 - Alpha Release.
                1 - Beta Release.
                2 - Release Candidate.
                3 - Stable.

       Everything is now set from the configure script. No need to edit this file anymore.
     */

    mushstate.version.major = TINYMUSH_VERSION_MAJOR;
    mushstate.version.minor = TINYMUSH_VERSION_MINOR;
    mushstate.version.patch = TINYMUSH_VERSION_PATCH;
    mushstate.version.tweak = TINYMUSH_VERSION_TWEAK;
    mushstate.version.status = TINYMUSH_RELEASE_STATUS;
    mushstate.version.git_hash = XSTRDUP(TINYMUSH_GIT_HASH, "mushstate.version.name");
    mushstate.version.git_dirty = (bool)TINYMUSH_GIT_DIRTY;
    mushstate.version.git_date = XSTRDUP(TINYMUSH_GIT_RELEASE_DATE, "mushstate.version.name");

    format_version();
}

void log_version(void)
{
    log_write(LOG_ALWAYS, "INI", "START", "TinyMUSH version %s", mushstate.version.versioninfo);
    log_write(LOG_ALWAYS, "INI", "START", "Build date: %s", TINYMUSH_BUILD_DATE);
}
