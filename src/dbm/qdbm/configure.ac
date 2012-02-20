#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TinyMUSH:libTinyQDBM], [3.3.2.0], [tinymush-bugs@lists.sourceforge.net],,[http://tinymush.sourceforge.net])
AC_COPYRIGHT([Copyright (c) 2011 TinyMUSH development team.])
AC_CONFIG_AUX_DIR([libltdl/config])
AC_CONFIG_MACRO_DIR([libltdl/m4])
AC_CONFIG_SRCDIR([hovel.c])
AC_CONFIG_HEADERS([autoconf.h])

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

PRETTY_PRINT([configure], [Checks for header files.], [1])
# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h])

PRETTY_PRINT([configure], [Checks for typedefs, structures, and compiler characteristics.], [1])
# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

PRETTY_PRINT([configure], [Checks for library functions.], [1])
# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit ftruncate localtime_r memset mkdir munmap rmdir strchr strrchr strstr strtol])

PRETTY_PRINT([configure], [writting results], [1])
AC_SUBST([ac_aux_dir])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
