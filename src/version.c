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

void do_version ( dbref player, dbref cause, int extra ) {
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname bpInfo;
#endif

    MODVER *mver;

    char string[MBUF_SIZE], *ptr;

    sprintf ( string, "\nTinyMUSH version %d.%d", mudstate.version.major, mudstate.version.minor );

    switch ( mudstate.version.status ) {
    case 0:
        sprintf ( string, "%s, Alpha %d", string, mudstate.version.revision );
        break;
    case 1:
        sprintf ( string, "%s, Beta %d", string, mudstate.version.revision );
        break;
    case 2:
        sprintf ( string,"%s, Release Candidate %d", string, mudstate.version.revision );
        break;
    default:
        if ( mudstate.version.revision > 0 ) {
            sprintf ( string, "%s, Patch Level %d", string, mudstate.version.revision );
        } else {
            sprintf ( string, "%s, Gold Release", string );
        }
    }
    sprintf ( string, "%s (%s)", string, PACKAGE_RELEASE_DATE );
    ptr = repeatchar ( strlen ( string ), '-' );
    notify ( player, tmprintf ( "%s\n%s\n", string, ptr ) );
    XFREE ( ptr, "repeatchar" );
    notify ( player, tmprintf ( "     Build date: %s", MUSH_BUILD_DATE ) );
    if ( Wizard ( player ) ) {
#ifdef HAVE_SYS_UTSNAME_H
        uname ( &bpInfo );
        notify ( player, tmprintf ( " Build platform: %s %s %s", bpInfo.sysname, bpInfo.release, bpInfo.machine ) );
#endif
        notify ( player, tmprintf ( "Configure Flags: %s", mudstate.configureinfo ) );
        notify ( player, tmprintf ( " Compiler Flags: %s", mudstate.compilerinfo ) );
        notify ( player, tmprintf ( "   Linker Flags: %s", mudstate.linkerinfo ) );
        notify ( player, tmprintf ( "     DBM driver: %s\n", mudstate.dbmdriver ) );

    }
    if ( mudstate.modloaded[0] ) {
        MODULE *mp;
        WALK_ALL_MODULES ( mp ) {
            ptr = repeatchar ( strlen ( mp->modname ) + 8, '-' );
            notify ( player, tmprintf ( "Module %s\n%s\n", mp->modname, ptr ) );
            XFREE ( ptr, "repeatchar" );
            if ( ( mver = DLSYM_VAR ( mp->handle, mp->modname, "version", MODVER * ) ) != NULL ) {

                notify ( player, tmprintf ( "        Version: %s", mver->version ) );
                notify ( player, tmprintf ( "         Author: %s", mver->author ) );
                notify ( player, tmprintf ( "          Email: %s", mver->email ) );
                notify ( player, tmprintf ( "        Website: %s", mver->url ) );
                notify ( player, tmprintf ( "      Copyright: %s", mver->copyright ) );
                notify ( player, tmprintf ( "    Description: %s\n", mver->description ) );
            } else {
                notify ( player, tmprintf ( "module %s: no version information", mp->modname ) );
            }
        }
    }
}

void init_version ( void ) {

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

    string = XSTRDUP ( PACKAGE_VERSION, "init_version" );

    if ( string != NULL ) {
        mudstate.version.major =    strtoimax ( strsep ( &string, "." ), ( char * ) NULL, 10 );
        mudstate.version.minor =    strtoimax ( strsep ( &string, "." ), ( char * ) NULL, 10 );
        mudstate.version.status =   strtoimax ( strsep ( &string, "." ), ( char * ) NULL, 10 );
        mudstate.version.revision = strtoimax ( strsep ( &string, "." ), ( char * ) NULL, 10 );
    } else {
        /* If we hit that, we have a serious problem... */
        mudstate.version.major = 0;
        mudstate.version.minor = 0;
        mudstate.version.status = 0;
        mudstate.version.revision = 0;
    }

    XFREE ( string, "init_version" );

    mudstate.configureinfo = munge_space ( PACKAGE_CONFIG );
    mudstate.compilerinfo = munge_space ( MUSH_BUILD_COMPILE );
    mudstate.linkerinfo = munge_space ( MUSH_BUILD_LTCOMPILE );

    mudstate.dbmdriver = XSTRDUP ( tmprintf ( "%s", MUSH_DBM ), "init_version" );
    log_printf2 ( LOG_ALWAYS, "INI", "START", "       Starting: TinyMUSH %d.%d.%d.%d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.status, mudstate.version.revision, PACKAGE_RELEASE_DATE );
    log_printf2 ( LOG_ALWAYS, "INI", "START", "     Build date: %s", MUSH_BUILD_DATE );
    log_printf2 ( LOG_ALWAYS, "INI", "START", "Configure Flags: %s", mudstate.configureinfo );
    log_printf2 ( LOG_ALWAYS, "INI", "START", " Compiler Flags: %s", mudstate.compilerinfo );
    log_printf2 ( LOG_ALWAYS, "INI", "START", "   Linker Flags: %s", mudstate.linkerinfo );
    log_printf2 ( LOG_ALWAYS, "INI", "START", "     DBM driver: %s", mudstate.dbmdriver );
}
