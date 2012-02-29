/* version.c - version information */
/* $Id: version.c,v 1.16 2010/12/29 04:58:38 tyrspace Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "patchlevel.h"		/* required by code */
#include "version.h"		/* required by code */

void
do_version(player, cause, extra)
dbref player, cause;

int extra;
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname bpInfo;
#endif

    char string[MBUF_SIZE];

    sprintf(string, "\nTinyMUSH version %d.%d", mudstate.version.major, mudstate.version.minor);
    
    switch(mudstate.version.status)
    {
        case 0: sprintf(string, "%s, Alpha %d", string, mudstate.version.status);
                break;
        case 1: sprintf(string, "%s, Beta %d", string, mudstate.version.status);
                break;
        case 2: sprintf(string,"%s, Release Candidate %d", string, mudstate.version.status);
                break;
    }
    if(mudstate.version.revision > 0)
        sprintf(string, "%s, Patch Level %d", string, mudstate.version.revision);
    notify(player, tprintf("%s (%s)", string, PACKAGE_RELEASE_DATE));
    if (Wizard(player))
    {
#ifdef HAVE_SYS_UTSNAME_H
        uname(&bpInfo);
        notify(player, tprintf("Build platform: %s %s %s", bpInfo.sysname, bpInfo.release, bpInfo.machine));
#endif
        notify(player, tprintf("    DBM driver: %s", MUSH_DBM));
    if (mudstate.modloaded[0])
    {
        notify(player, tprintf("Modules loaded: %s",
                               mudstate.modloaded));
    }
        notify(player, tprintf("    Build info: %s", mudstate.buildinfo));
    }
}

void
NDECL(init_version)
{

/* TinyMUSH 3.3 version scheme : Major Version.Minor Version.Status.Revision
    Major version	: The main branch.
    Minor version	: The minor version.
    Status		: 0 - Alpha Release.
                          1 - Beta Release.
                          2 - Release Candidate.
                          3 - Release Version (Gamma).
    Revision 		: Patch Level.
    
    Everything is now set from the configure script. No need to edit this file anymore.
*/
    char *string, *token;
    
    string = XSTRDUP(PACKAGE_VERSION, "init_version");
    
    if(string != NULL)
    {
    mudstate.version.major = strtoimax(strsep(&string, "."), (char *) NULL, 10);
    mudstate.version.minor = strtoimax(strsep(&string, "."), (char *) NULL, 10);
    mudstate.version.status = strtoimax(strsep(&string, "."), (char *) NULL, 10);
    mudstate.version.revision = strtoimax(strsep(&string, "."), (char *) NULL, 10);
    }
    else
    {
    /* If we hit that, we have a serious problem... */
    mudstate.version.major = 0;
    mudstate.version.minor = 0;
    mudstate.version.status = 0;
    mudstate.version.revision = 0;
    }
    
    XFREE(string, "init_version");

    mudstate.dbmdriver = XSTRDUP(tprintf("%s", MUSH_DBM), "init_version");
    mudstate.buildinfo =
        XSTRDUP(tprintf("%s\n                %s\n                %s",
                        PACKAGE_CONFIG, MUSH_BUILD_COMPILE, MUSH_BUILD_LTCOMPILE), "init_version");
    STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("  Starting: TinyMUSH %d.%d.%d.%d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.status, mudstate.version.revision, PACKAGE_RELEASE_DATE);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("Build date: %s", MUSH_BUILD_DATE);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("Build info: %s", mudstate.buildinfo);
    ENDLOG STARTLOG(LOG_ALWAYS, "INI", "START")
    log_printf("DBM driver: %s", mudstate.dbmdriver);
    ENDLOG
}
