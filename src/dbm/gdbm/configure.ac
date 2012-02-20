#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TinyMUSH:libTinyGDBM], [3.3.2.0], [tinymush-bugs@lists.sourceforge.net],,[http://tinymush.sourceforge.net/])
AC_COPYRIGHT([Copyright (c) 2011 TinyMUSH development team.])
AC_CONFIG_AUX_DIR([libltdl/config])
AC_CONFIG_SRCDIR([gdbmopen.c])
AC_CONFIG_HEADERS([autoconf.h])
AC_CONFIG_MACRO_DIR([libltdl/m4])

AX_SPLIT_VERSION()
PACKAGE_RELEASE_DATE="2012-02-19"
PACKAGE_COPYRIGHT="Copyright (c) 2012 TinyMUSH development team"
AX_PRINT_PACKAGE_TITLE()

PRETTY_PRINT([configure], [Initializing Automake.], [1])

AM_INIT_AUTOMAKE()

PRETTY_PRINT([configure], [Initializing Libtools.], [1])

LT_PREREQ([2.4])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
LTDL_INIT([convenience])

PRETTY_PRINT([configure], [Checks for programs.], [1])
# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PRETTY_PRINT([configure], [Checks for libraries.], [1])
# Checks for libraries.
AC_CHECK_LIB(dbm, main)
AC_CHECK_LIB(ndbm, main)

PRETTY_PRINT([configure], [Checks for header files.], [1])
# Checks for header files.
AH_TOP([#include "gdbmcopyright.h"])
AC_CHECK_HEADERS([fcntl.h memory.h stdlib.h string.h strings.h sys/file.h unistd.h sys/types.h])
AH_BOTTOM([#include "gdbmsystems.h"
#include "gdbmconst.h"
#include "gdbmerrno.h"
#include "gdbmdefs.h"
#include "gdbmproto.h"
#include "gdbmextern.h"])

PRETTY_PRINT([configure], [Checks for typedefs, structures, and compiler characteristics.], [1])
# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_CHECK_MEMBERS([struct stat.st_blksize])

PRETTY_PRINT([configure], [Checks for library functions.], [1])
# Checks for library functions.
AC_CHECK_FUNCS([rename ftruncate flock bcopy fsync])
AC_FUNC_MALLOC

PRETTY_PRINT([configure], [writting results], [1])
AC_SUBST([ac_aux_dir])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
