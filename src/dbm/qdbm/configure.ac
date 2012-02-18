#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TinyMUSH:libTinyQDBM], [3.3.3.0], [tinymush-bugs@lists.sourceforge.net],,[http://tinymush.sourceforge.net])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([hovel.c])
AC_CONFIG_HEADERS([autoconf.h])

PRETTY_PRINT([
%B${PACKAGE_NAME}%b version %B${PACKAGE_VERSION}%b.
  Copyright (c) 2011, TinyMUSH development team. All rights reserved.
  See %Bhttp://tinymush.sourceforge.net%b for more informations.
])

PRETTY_PRINT([configure], [Initializing Automake.], [1])

AM_INIT_AUTOMAKE()

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
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
