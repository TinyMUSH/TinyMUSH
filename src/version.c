/* version.c - version information */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */
#include "typedefs.h"           /* required by mudconf */
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "version.h"		/* required by code */

void do_version(dbref player, dbref cause, int extra) {
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname bpInfo;
#endif

    char string[MBUF_SIZE], *ptr;

    sprintf(string, "\nTinyMUSH version %d.%d", mudstate.version.major, mudstate.version.minor);

    switch(mudstate.version.status) {
    case 0:
        sprintf(string, "%s, Alpha %d", string, mudstate.version.revision);
        break;
    case 1:
        sprintf(string, "%s, Beta %d", string, mudstate.version.revision);
        break;
    case 2:
        sprintf(string,"%s, Release Candidate %d", string, mudstate.version.revision);
        break;
    default:
        if(mudstate.version.revision > 0) {
            sprintf(string, "%s, Patch Level %d", string, mudstate.version.revision);
        } else {
            sprintf(string, "%s, Gold Release", string);
        }
    }
    sprintf(string, "%s (%s).", string, PACKAGE_RELEASE_DATE);
    ptr = repeatchar(strlen(string), '-');
    notify(player, tprintf("%s\n%s\n", string, ptr));
    XFREE(ptr, "repeatchar");
    notify(player, tprintf("   Build number: %s (%s)", MUSH_BUILD_NUM, MUSH_BUILD_DATE));
    if (Wizard(player)) {
#ifdef HAVE_SYS_UTSNAME_H
        uname(&bpInfo);
        notify(player, tprintf(" Build platform: %s %s %s", bpInfo.sysname, bpInfo.release, bpInfo.machine));
#endif
        notify(player, tprintf("Configure Flags: %s", mudstate.configureinfo));
        notify(player, tprintf(" Compiler Flags: %s", mudstate.compilerinfo));
        notify(player, tprintf("   Linker Flags: %s", mudstate.linkerinfo));
        notify(player, tprintf("     DBM driver: %s", mudstate.dbmdriver));
    }
    if (mudstate.modloaded[0]) {
        MODULE *cam__mp;
        void (*cam__ip)(dbref, dbref, int);
        notify(player, tprintf("\nModules version\n---------------\n"));
        WALK_ALL_MODULES(cam__mp) {
            if ((cam__ip = DLSYM(cam__mp->handle, cam__mp->modname, "version", (dbref, dbref, int))) != NULL) {
                (*cam__ip)(player, cause, extra);
            } else {
                notify(player, tprintf("module %s: no version information", cam__mp->modname));
            }
        }

    }
}

void init_version(void) {

    /* TinyMUSH 3.3 version scheme : Major Version.Minor Version.Revision.Status
        Major version	: The main branch.
        Minor version	: The minor version.
        Revision 	: Patch Level.
        Status		: 0 - Alpha Release.
                              1 - Beta Release.
                              2 - Release Candidate.
                              3 - Release Version (Gamma).

        Everything is now set from the configure script. No need to edit this file anymore.
    */
    char *string, *pc, *bc, *bl;

    string = XSTRDUP(PACKAGE_VERSION, "init_version");

    if(string != NULL) {
        mudstate.version.major =    strtoimax(strsep(&string, "."), (char *) NULL, 10);
        mudstate.version.minor =    strtoimax(strsep(&string, "."), (char *) NULL, 10);
        mudstate.version.status =   strtoimax(strsep(&string, "."), (char *) NULL, 10);
        mudstate.version.revision = strtoimax(strsep(&string, "."), (char *) NULL, 10);
    } else {
        /* If we hit that, we have a serious problem... */
        mudstate.version.major = 0;
        mudstate.version.minor = 0;
        mudstate.version.status = 0;
        mudstate.version.revision = 0;
    }

    XFREE(string, "init_version");

    mudstate.configureinfo = munge_space(PACKAGE_CONFIG);
    mudstate.compilerinfo = munge_space(MUSH_BUILD_COMPILE);
    mudstate.linkerinfo = munge_space(MUSH_BUILD_LTCOMPILE);

    mudstate.dbmdriver = XSTRDUP(tprintf("%s", MUSH_DBM), "init_version");
    STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("       Starting: TinyMUSH %d.%d.%d.%d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.status, mudstate.version.revision, PACKAGE_RELEASE_DATE);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("   Build number: %s", MUSH_BUILD_NUM);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("     Build date: %s", MUSH_BUILD_DATE);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("Configure Flags: %s", mudstate.configureinfo);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf(" Compiler Flags: %s", mudstate.compilerinfo);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("   Linker Flags: %s", mudstate.linkerinfo);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("     DBM driver: %s", mudstate.dbmdriver);
    ENDLOG
}
