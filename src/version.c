/**
 * @file version.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Version information
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

void do_version(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int extra)
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname bpInfo;
#endif
    MODVER *mver;
    char *ptr;
    char string[MBUF_SIZE];
    XSNPRINTF(string, MBUF_SIZE, "%s [%s]", mushstate.version.name, PACKAGE_RELEASE_DATE);
    ptr = repeatchar(strlen(string), '-');
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "\n%s\n%s\n", string, ptr);
    XFREE(ptr);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "     Build date: %s, %s", __DATE__, __TIME__);

    if (Wizard(player))
    {
#ifdef HAVE_SYS_UTSNAME_H
        uname(&bpInfo);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, " Build platform: %s %s %s %s %s", bpInfo.sysname, bpInfo.nodename, bpInfo.release, bpInfo.version, bpInfo.machine);
#endif
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Configure Flags: %s", mushstate.configureinfo);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, " Compiler Flags: %s", mushstate.compilerinfo);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "   Linker Flags: %s\n", mushstate.linkerinfo);
    }

    if (mushstate.modloaded[0])
    {
        MODULE *mp;

        for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
        {
            XSNPRINTF(string, MBUF_SIZE, "Module %s", mp->modname);
            ptr = repeatchar(strlen(string), '-');
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s\n%s\n", string, ptr);
            XFREE(ptr);
            XSNPRINTF(string, MBUF_SIZE, "mod_%s_%s", mp->modname, "version");

            if ((mver = (MODVER *)lt_dlsym(mp->handle, string)) != NULL)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "        Version: %s", mver->version);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "         Author: %s", mver->author);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "          Email: %s", mver->email);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "        Website: %s", mver->url);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "      Copyright: %s", mver->copyright);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "    Description: %s\n", mver->description);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "module %s: no version information", mp->modname);
            }
        }
    }
}

void init_version(void)
{
    /* TinyMUSH 3.3 version scheme : Major Version.Minor Version.Status.Revision
       Major version   : The main branch.
       Minor version   : The minor version.
       Revision    : Patch Level.
       Status      : 0 - Alpha Release.
       1 - Beta Release.
       2 - Release Candidate.
       3 - Release Version (Gamma).

       Everything is now set from the configure script. No need to edit this file anymore.
     */
    char *version;
    version = XSTRDUP(PACKAGE_VERSION, "version");

    if (version != NULL)
    {
        mushstate.version.major = strtoimax(strsep(&version, "."), (char **)NULL, 10);
        mushstate.version.minor = strtoimax(strsep(&version, "."), (char **)NULL, 10);
        mushstate.version.status = strtoimax(strsep(&version, "."), (char **)NULL, 10);
        mushstate.version.revision = strtoimax(strsep(&version, "."), (char **)NULL, 10);
    }
    else
    {
        /* If we hit that, we have a serious problem... */
        mushstate.version.major = 0;
        mushstate.version.minor = 0;
        mushstate.version.status = 0;
        mushstate.version.revision = 0;
    }

    XFREE(version);
    version = munge_space(PACKAGE_CONFIG);
    mushstate.configureinfo = XSTRDUP(version, "mushstate.configureinfo");
    XFREE(version);

    version = munge_space(MUSH_BUILD_COMPILE);
    mushstate.compilerinfo = XSTRDUP(version, "mushstate.compilerinfo");
    XFREE(version);

    version = munge_space(MUSH_BUILD_LTCOMPILE);
    mushstate.linkerinfo = XSTRDUP(version, "mushstate.linkerinfo");
    XFREE(version);

    version = XMALLOC(LBUF_SIZE, "version");
    XSPRINTFCAT(version, "TinyMUSH version %d.%d", mushstate.version.major, mushstate.version.minor);

    switch (mushstate.version.status)
    {
    case 0:
        XSPRINTFCAT(version, ", Alpha %d", mushstate.version.revision);
        break;

    case 1:
        XSPRINTFCAT(version, ", Beta %d", mushstate.version.revision);
        break;

    case 2:
        XSPRINTFCAT(version, ", Release Candidate %d", mushstate.version.revision);
        break;

    default:
        if (mushstate.version.revision > 0)
        {
            XSPRINTFCAT(version, ", Patch Level %d", mushstate.version.revision);
        }
        else
        {
            XSPRINTFCAT(version, ", Gold Release");
        }

        break;
    }

    mushstate.version.name = XSTRDUP(version, "mushstate.version.name");
    XFREE(version);
}

void log_version(void)
{
    log_write(LOG_ALWAYS, "INI", "START", "       Starting: %s (%s)", mushstate.version.name, PACKAGE_RELEASE_DATE);
    log_write(LOG_ALWAYS, "INI", "START", "     Build date: %s, %s", __DATE__, __TIME__);
    log_write(LOG_ALWAYS, "INI", "START", "Configure Flags: %s", mushstate.configureinfo);
    log_write(LOG_ALWAYS, "INI", "START", " Compiler Flags: %s", mushstate.compilerinfo);
    log_write(LOG_ALWAYS, "INI", "START", "   Linker Flags: %s", mushstate.linkerinfo);
}
