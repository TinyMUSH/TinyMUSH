/* version.c - version information */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mudconf */
#include "game.h"      /* required by mudconf */
#include "alloc.h"     /* required by mudconf */
#include "flags.h"     /* required by mudconf */
#include "htab.h"      /* required by mudconf */
#include "ltdl.h"      /* required by mudconf */
#include "udb.h"       /* required by mudconf */
#include "udb_defs.h"  /* required by mudconf */
#include "mushconf.h"  /* required by code */
#include "db.h"        /* required by externs */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by code */
#include "version.h"   /* required by code */

void do_version(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int extra)
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname bpInfo;
#endif
    MODVER *mver;
    char *ptr;
    char string[MBUF_SIZE];
    XSNPRINTF(string, MBUF_SIZE, "%s (%s)", mudstate.version.name, PACKAGE_RELEASE_DATE);
    ptr = repeatchar(strlen(string), '-');
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "\n%s\n%s\n", string, ptr);
    XFREE(ptr);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "     Build date: %s", MUSH_BUILD_DATE);

    if (Wizard(player))
    {
#ifdef HAVE_SYS_UTSNAME_H
        uname(&bpInfo);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, " Build platform: %s %s %s", bpInfo.sysname, bpInfo.release, bpInfo.machine);
#endif
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Configure Flags: %s", mudstate.configureinfo);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, " Compiler Flags: %s", mudstate.compilerinfo);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "   Linker Flags: %s\n", mudstate.linkerinfo);
    }

    if (mudstate.modloaded[0])
    {
        MODULE *mp;

        for (mp = mudstate.modules_list; mp != NULL; mp = mp->next)
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
    char *version, *bp;
    version = XSTRDUP(PACKAGE_VERSION, "version");

    if (version != NULL)
    {
        mudstate.version.major = strtoimax(strsep(&version, "."), (char **)NULL, 10);
        mudstate.version.minor = strtoimax(strsep(&version, "."), (char **)NULL, 10);
        mudstate.version.status = strtoimax(strsep(&version, "."), (char **)NULL, 10);
        mudstate.version.revision = strtoimax(strsep(&version, "."), (char **)NULL, 10);
    }
    else
    {
        /* If we hit that, we have a serious problem... */
        mudstate.version.major = 0;
        mudstate.version.minor = 0;
        mudstate.version.status = 0;
        mudstate.version.revision = 0;
    }

    XFREE(version);
    version = munge_space(PACKAGE_CONFIG);
    mudstate.configureinfo = XSTRDUP(version, "mudstate.configureinfo");
    XFREE(version);

    version = munge_space(MUSH_BUILD_COMPILE);
    mudstate.compilerinfo = XSTRDUP(version, "mudstate.compilerinfo");
    XFREE(version);

    version = munge_space(MUSH_BUILD_LTCOMPILE);
    mudstate.linkerinfo = XSTRDUP(version, "mudstate.linkerinfo");
    XFREE(version);

    bp = version = XMALLOC(LBUF_SIZE, "version");
    XSPRINTFCAT(version, "TinyMUSH version %d.%d", mudstate.version.major, mudstate.version.minor);

    switch (mudstate.version.status)
    {
    case 0:
        XSPRINTFCAT(version, ", Alpha %d", mudstate.version.revision);
        break;

    case 1:
        XSPRINTFCAT(version, ", Beta %d", mudstate.version.revision);
        break;

    case 2:
        XSPRINTFCAT(version, ", Release Candidate %d", mudstate.version.revision);
        break;

    default:
        if (mudstate.version.revision > 0)
        {
            XSPRINTFCAT(version, ", Patch Level %d", mudstate.version.revision);
        }
        else
        {
            XSPRINTFCAT(version, ", Gold Release");
        }

        break;
    }

    mudstate.version.name = XSTRDUP(version, "mudstate.version.name");
    XFREE(version);
}

void log_version(void)
{
    log_write(LOG_ALWAYS, "INI", "START", "       Starting: %s (%s)", mudstate.version.name, PACKAGE_RELEASE_DATE);
    log_write(LOG_ALWAYS, "INI", "START", "     Build date: %s", MUSH_BUILD_DATE);
    log_write(LOG_ALWAYS, "INI", "START", "Configure Flags: %s", mudstate.configureinfo);
    log_write(LOG_ALWAYS, "INI", "START", " Compiler Flags: %s", mudstate.compilerinfo);
    log_write(LOG_ALWAYS, "INI", "START", "   Linker Flags: %s", mudstate.linkerinfo);
}
