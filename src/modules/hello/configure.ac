#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Hello module for TinyMUSH 3.3], [1.1.0.0], [tinymush-support@list.sourceforge.net])
AC_CONFIG_SRCDIR([hello.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([libltdl/config])
AC_CONFIG_MACRO_DIR([libltdl/m4])
AC_CONFIG_MACRO_DIR([libltdl/m4/])
LT_PREREQ([2.4])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
AC_SUBST([LIBTOOL_DEPS])
LTDL_INIT([convenience])
AM_INIT_AUTOMAKE()




# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.

AC_MSG_CHECKING([for include path needed to find tinymushapi.h])
AC_ARG_WITH([api-include], [AS_HELP_STRING([--with-api-include=<path>], [Specify the include path needed to find tinymushapi.h @<:@default=none@:>@])], [API_CFLAG="-I$with_api_include"], []) 
AC_MSG_RESULT([${API_CFLAG}])

SAVECFLAGS="$CFLAGS"
SAVECPPFLAGS="$CPPFLAGS"
CFLAGS="$CFLAGS -I./libltdl/ $API_CFLAG"
CPPFLAGS="$CPPFLAGS -I./libltdl/ $API_CFLAG"
AC_CHECK_HEADERS([stdlib.h stdio.h limits.h string.h memory.h unistd.h time.h ltdl.h sys/socket.h netinet/in.h])

AC_CHECK_HEADER([tinymushapi.h], [AC_MSG_RESULT([yes])], [AC_MSG_ERROR([cannot find tinymushapi.h], [1])])
CPPFLAGS="$SAVECPPFLAGS"
CFLAGS="$SAVECFLAGS"

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_SUBST([API_CFLAG])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
