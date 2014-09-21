#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])

dnl AC_INIT and AX_VERSION need constants to work. By defining the version here we can avoid
dnl to have multiple places to update when
m4_define(TINYMUSH_VERSION, [3.3.0.6])
m4_define(TINYMUSH_RELEASE_DATE, ["2014-09-21"])
m4_define(TINYMUSH_COPYRIGHT, [Copyright (C) 2014 TinyMUSH development team.])

AC_INIT([TinyMUSH],[TINYMUSH_VERSION],[tinymush-bugs@lists.sourceforge.net],[tinymush],[http://tinymush.sourceforge.net])

AX_VERSION()

dnl AC_INIT define the following itself:
dnl PACKAGE_NAME
dnl PACKAGE_TARNAME
dnl PACKAGE_VERSION
dnl PACKAGE_STRING
dnl PACKAGE_BUGREPORT
dnl PACKAGE_URL
dnl Aith AX_VERSION, We define the following.
dnl PACKAGE_VERSION_MAJOR
dnl PACKAGE_VERSION_MINOR
dnl PACKAGE_RELEASE_STATUS
dnl PACKAGE_RELEASE_NAME
dnl PACKAGE_RELEASE_REVISION
dnl And we manually create a few other things that need to be updated from time to time.
dnl PACKAGE_RELEASE_DATE
dnl PACKAGE_COPYRIGHT

dnl Some default values
dnl
with_included_ltdl="yes"

AC_COPYRIGHT([TINYMUSH_COPYRIGHT

This software is release under the terms of the Artistic License 2.0.

The Artistic License 2.0  establishes  the  terms  under  which a given free
software Package may be copied, modified, distributed, and/or redistributed.
The intent is that the Copyright Holder maintains some artistic control over
the development of that Package while still keeping the Package available as
open source and free software.

You are always permitted to make arrangements wholly outside of this license
directly with the Copyright Holder of a given Package.  If the terms of this
license do not permit  the full use that you propose to make of the Package,
you should contact  the Copyright Holder  and  seek  a  different  licensing 
arrangement.])
AC_CONFIG_SRCDIR([src/game.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([src/libltdl/config])
AC_CONFIG_MACRO_DIR([src/libltdl/m4])

AX_PRINT_PACKAGE_TITLE()

AX_BOX([Checks for programs])
# Checks for programs.

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CPP_WERROR
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_INSTALL
LT_INIT

AC_PATH_PROGS([with_tar_prog], [tar], AC_MSG_ERROR([not found]))
AC_PATH_PROGS([with_compress_prog], [gzip], AC_MSG_ERROR([not found]))

AX_BOX([Initializing Automake])

AM_INIT_AUTOMAKE([subdir-objects])

AX_BOX([Initializing Libtools])

LT_PREREQ([2.4])
LT_CONFIG_LTDL_DIR([src/libltdl])
LT_INIT([dlopen])
AC_SUBST([LIBTOOL_DEPS])
LTDL_INIT([convenience])

AX_BOX([$ac_default_prefix])

AX_BOX([Checks for Prefix])

dnl XXX // This really need to be check...
dnl AC_PREFIX_DEFAULT([`pwd -P | sed 's:/[[^/]]*$::'`/game])
AC_PREFIX_DEFAULT([`pwd -P`/game])
AC_MSG_CHECKING([for installation suffix])
AC_ARG_WITH([install-suffix], 
 [AS_HELP_STRING([--with-install-suffix@<:@=suffix@:>@], [Select an installation suffix for includes, libraries and text files, if no installation suffix is given, configure will use "tinymush" if installed in the default location, else, no prefix will be used])],
 [AS_IF([test "x$withval" = "x"], [with_install_suffix="tinymush"], [with_install_suffix="$withval"])],
 [AS_CASE(["$prefix"], ["NONE"], [with_install_suffix="tinymush"],[$ac_default_prefix], [with_install_suffix="tinymush"], [with_install_suffix=""])])
with_install_suffix="/`echo "$with_install_suffix" | sed "s/\///g"`"
AS_IF([test "x$with_install_suffix" = "x/"], [
 with_install_suffix=""
 AC_MSG_RESULT([none])
], [
AC_MSG_RESULT([$with_install_suffix])
])

AC_SUBST([with_install_suffix])
	
AX_BOX([Checks for Options])

AH_TOP([#ifndef __AUTOCONF_H
#define __AUTOCONF_H

#include "copyright.h"])

AH_BOTTOM([
#endif /* __AUTOCONF_H */])

AC_MSG_CHECKING([dbm engine to use])
AC_ARG_WITH([dbm],
 [AS_HELP_STRING([--with-dbm=gdbm|qdbm], [Select the DBM driver used to save game data, valid values are : gdbm Use the traditional GDBM hash database driver, or qdbm : Use the new QDBM hash database driver. A newer implementation of the DBM engine. @<:@default=qdbm@:>@])],
 [AS_CASE([$withval], [gdbm], [with_dbm="gdbm"], [qdbm], [with_dbm="qdbm"], [with_dbm="qdbm"])], 
 [with_dbm="qdbm"])
AC_MSG_RESULT([$with_dbm])

AS_IF([test "x$with_dbm" = "xqdbm"], 
 [
  AC_DEFINE([MUSH_DBM], ["QDBM"], [TinyQDBM dbm used])
  AC_DEFINE([HAVE_LIBTINYQDBM_H], [1], [Define to 1 if you have <libtinyqdbm.h>.])
 ], 
 [
  AC_DEFINE([MUSH_DBM], ["GDBM"], [TinyGDBM dbm used])
  AC_DEFINE([HAVE_LIBTINYGDBM_H], [1], [Define to 1 if you have <libtinygdbm.h>.])
 ]
)

AM_CONDITIONAL([WANT_DBM_GDBM], [test "x$with_dbm" = "xgdbm"])
AM_CONDITIONAL([WANT_DBM_QDBM], [test "x$with_dbm" = "xqdbm"])

AC_MSG_CHECKING([whether to enable disk-based database])
AC_ARG_ENABLE([disk-based], [AS_HELP_STRING([--enable-disk-based], [store the database on disk using a DBM hashfile, and cache some attributes in a limited amount of memory. The alternative is memory-based, where the database is kept in memory. Disabling the disk-based cache can improve performance if enough memory is available. @<:@default=yes@:>@])],
 [AS_IF([test "x$enableval" = "xno"],
  [
   AC_DEFINE([MEMORY_BASED], [1], [Enable memory-based DBM hashDB])
   AC_MSG_RESULT([no])
  ],
  [
   AC_MSG_RESULT([yes])
  ]
 )],
 [AC_MSG_RESULT([yes])]
)

AC_MSG_CHECKING([whether the comsys module is enable])
AC_ARG_ENABLE([module-comsys], [AS_HELP_STRING([--enable-module-comsys], [Enable support for the comsys module @<:@default=yes@:>@])], 
 [
  AX_ENABLE_YESNO([$enableval])
 ], [
  AC_MSG_RESULT([yes])
  enable_module_comsys="yes"
 ]
)
AM_CONDITIONAL([WANT_MODULE_COMSYS], [test "x$enable_module_comsys" = "xyes"])

AC_MSG_CHECKING([whether the mail module is enable])
AC_ARG_ENABLE([module-mail], [AS_HELP_STRING([--enable-module-mail], [Enable support for the mail module @<:@default=yes@:>@])], 
 [
  AX_ENABLE_YESNO([$enableval])
 ], [
 AC_MSG_RESULT([yes])
 enable_module_mail="yes"
 ]
)
AM_CONDITIONAL([WANT_MODULE_MAIL], [test "x$enable_module_mail" = "xyes"])

AC_MSG_CHECKING([whether the dbsql module is enable])
AC_ARG_ENABLE([module-dbsql], [AS_HELP_STRING([--enable-module-dbsql], [Enable support for the dbsql module @<:@default=no@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [
  AC_MSG_RESULT([no])
  enable_module_dbsql="no"
 ]
)
AM_CONDITIONAL([WANT_MODULE_DB_SQL], [test "x$enable_module_dbsql" = "xyes"])

AC_MSG_CHECKING([dbsql module driver to use])
AC_ARG_WITH([module-dbsql],
 [AS_HELP_STRING([--with-module-dbsql=DRIVER], [Select the SQL driver used by the db_sql module, valid values are : none : no database driver, msql : use mSQL database, mysql : use MySQL database driver, pgsql : use PostgreSQL database driver or sqlite : use SQLite database driver. Of course support for the database must be avalable before trying to use anything but none. @<:@default=none@:>@])],
 [AS_CASE([$withval], [msql], [with_db_sql="msql"], [pgsql], [with_db_sql="pgsql"], [sqlite], [with_db_sql="sqlite"], [with_db_sql="none"])], 
 [with_module_dbsql="none"])
AC_MSG_RESULT([$with_module_dbsql])
AS_CASE([$with_module_dbsql],
	[msql], 	[
			AX_CHECK_MSQL([yes])
			],
	[mysql],	[
			AX_CHECK_MYSQL([yes])
			AC_SUBST([MYSQL_LIBS])
			AC_SUBST([MYSQL_CFLAGS])
			AC_DEFINE([HAVE_MYSQL], [1], [DB_SQL use the MySQL driver])
			],
	[pgsql],	[
			AX_CHECK_PGSQL([yes])
			AC_SUBST([PGSQL_LIBS])  
			AC_SUBST([PGSQL_CFLAGS])
			AC_DEFINE([HAVE_PGSQL], [1], [DB_SQL use the PostgreSQL driver])
			],
	[sqlite],	[
			AX_LIB_SQLITE3([3.0.0])
			],
	[AC_DEFINE([DB_SQL_DRIVER],[NONE], [DB_SQL use no driver driver])]
)
	
AM_CONDITIONAL([WANT_MODULE_DB_SQL_DRIVER_NONE], [test "x$with_module_dbsql" = "xnone"])
AM_CONDITIONAL([WANT_MODULE_DB_SQL_DRIVER_MSQL], [test "x$with_module_dbsql" = "xmsql"])
AM_CONDITIONAL([WANT_MODULE_DB_SQL_DRIVER_MYSQL], [test "x$with_module_dbsql" = "xmysql"])
AM_CONDITIONAL([WANT_MODULE_DB_SQL_DRIVER_PGSQL], [test "x$with_module_dbsql" = "xpgsql"])
AM_CONDITIONAL([WANT_MODULE_DB_SQL_DRIVER_SQLITE], [test "x$with_module_dbsql" = "xsqlite"])

AC_MSG_CHECKING([whether to enable pueblo support])
AC_ARG_ENABLE([pueblo-support], [AS_HELP_STRING([--enable-pueblo-support], [Enable support for Pueblo clients @<:@default=yes@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_pueblo_support="yes"
 ]
)
AS_IF([test "x$enable_pueblo_support" = "xyes"], [AC_DEFINE([PUEBLO_SUPPORT],[1], [Support for Pueblo Clients])])

AC_MSG_CHECKING([whether to enable command lag detection support])
AC_ARG_ENABLE([lag-checking], [AS_HELP_STRING([--enable-lag-checking], [Enable warnings that a certain command has exceeded a certain amount of wall-clock time to execute. Disable this if you have severe problems with computational resources on your host machine. @<:@default=yes@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_lag_checking="yes"
 ]
)
AS_IF([test "x$enable_lag_checking" = "xno"], [AC_DEFINE([NO_LAG_CHECK],[1], [Support for command lag detection support])])

AC_MSG_CHECKING([whether to enable per-object time usage tracking])
AC_ARG_ENABLE([time-checking], [AS_HELP_STRING([--enable-time-checking], [Enable tracking of the amount of wall-clock time that individual objects are using. Disable this if you have severe problems with computational resources on your host machine. (This is assumed to be disabled if lag-checking is disabled) @<:@default=yes@:>@])], 
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_time_checking="yes"
 ]
)
AS_IF([test "x$enable_time_checking" = "xno"], [AC_DEFINE([NO_TIMECHECKING],[1], [Support for per-object time usage tracking])])

AC_MSG_CHECKING([whether to enable cpu time checking])
AC_ARG_ENABLE([cpu-checking], [AS_HELP_STRING([--enable-cpu-checking], [Enable CPU time reported with getrusage() instead of tracking wall-clock time for lag-checking and time-checking features. This is more accurate but also more computationally expensive. @<:@default=no@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([no])
  enable_cpu_checking="no"
 ]
)
AS_IF([test "x$enable_cpu_checking" = "xyes"], [AC_DEFINE([TRACK_USER_TIME],[1], [Support for cpu time checking])])

AC_MSG_CHECKING([which malloc-wrapper macros to use])
AC_ARG_WITH([malloc-wrapper], [AS_HELP_STRING([--with-malloc-wrapper=@<:@track|log@:>@], [Enable use of wrapper macros for malloc(), free(), and related functions to do one of the following, which can help identify memory leaks: track - maintain a tagged list of memory allocated for purposes other than the buffer pool. (see @list raw_memory) log - print log messages when the function are called. This is spammy; try other debugging tools first.])],
  AS_IF([ test "x$withval" = "xtrack" ], AC_DEFINE([RAW_MEMTRACKING],[1], [Enable malloc wrapper tracker]), AS_IF([ test "x$withval" = "xlog" ], AC_DEFINE([TEST_MALLOC],[1], [Enable malloc wrapper logger]), [with_malloc_wrapper="none"])),
  [with_malloc_wrapper="none"]
)

AC_MSG_CHECKING([how many global registers to use])
AC_ARG_WITH([global-regs], [AS_HELP_STRING([--with-global-regs=N], [use N global registers (%q0-%q9,%qa-%qz) N must be a number from 10 to 36. Set lower to conserve memory if you don't need all the possible registers  @<:@default=36@:>@])],
  AS_IF([ test "$withval" -lt "10" ], [with_global_regs=10], AS_IF([ test "$withval" -gt "36" ], [with_global_regs=36], [with_global_regs=$withval])),
  [with_global_regs=36]
)
AC_MSG_RESULT([$with_global_regs])
AC_DEFINE_UNQUOTED([MAX_GLOBAL_REGS], [${with_global_regs}], [How many global registers to use])

AC_MSG_CHECKING([maximum length for player names])
AC_ARG_WITH([pname-length], [AS_HELP_STRING([--with-pname-length=LENGTH], [maximum length for player names @<:@default=22@:>@])], [with_pname_length=$withval], [with_pname_length=22])
AC_MSG_RESULT([$with_pname_length])
AC_DEFINE_UNQUOTED([PLAYER_NAME_LIMIT], [${with_pname_length}], [Max length for player names])

AC_MSG_CHECKING([number of env vars])
AC_ARG_WITH([env-vars], [AS_HELP_STRING([--with-env-vars=NUMBER], [Number of env vars (%0 et al) @<:@default=10@:>@])], [with_env_vars=$withval], [with_env_vars=10])
AC_MSG_RESULT([$with_env_vars])
AC_DEFINE_UNQUOTED([NUM_ENV_VARS], [${with_env_vars}], [Number of env vars (%0 et al)])

AC_MSG_CHECKING([maximum number of args for command processor])
AC_ARG_WITH([max-args], [AS_HELP_STRING([--with-max-args=MAX], [Maximum number of args for command processor @<:@default=100@:>@])], [with_max_args=$withval], [with_max_args=100])
AC_MSG_RESULT([$with_max_args])
AC_DEFINE_UNQUOTED([MAX_ARG], [${with_max_args}], [max # args from command processor])

AC_MSG_CHECKING([maximum number of iter levels])
AC_ARG_WITH([max-iter], [AS_HELP_STRING([--with-max-iter=MAX], [Maximum number of iter levels @<:@default=1024@:>@])], [with_max_iter=$withval], [with_max_iter=1024])
AC_MSG_RESULT([$with_max_iter])
AC_DEFINE_UNQUOTED([MAX_ITER_NESTING], [${with_max_iter}], [max # of iter levels])

AC_MSG_CHECKING([maximum length of a delimiter])
AC_ARG_WITH([max-delim], [AS_HELP_STRING([--with-max-delim=MAX], [Maximum lenght of a delimiter @<:@default=128@:>@])], [with_max_delim=$withval], [with_max_delim=128])
AC_MSG_RESULT([$with_max_delim])
AC_DEFINE_UNQUOTED([MAX_DELIM_LEN], [${with_max_delim}], [Maximum length of a delimiter])

AC_MSG_CHECKING([separator of dbref from marker flags])
AC_ARG_WITH([flag-sep], [AS_HELP_STRING([--with-flag-sep=CHAR], [separator of dbref from marker flags @<:@default=_@:>@])], [with_flag_sep="$withval"], [with_flag_sep="_"])
AC_MSG_RESULT([$with_flag_sep])
AC_DEFINE_UNQUOTED([MARK_FLAG_SEP], ['${with_flag_sep}'], [sep. of dbref from marker flags])

AC_MSG_CHECKING([for hash factor])
AC_ARG_WITH([hash-factor], [AS_HELP_STRING([--with-hash-factor=NUMBER], [Hash factor @<:@default=2@:>@])], [with_hash_factor=$withval], [with_hash_factor=2])
AC_MSG_RESULT([$with_hash_factor])
AC_DEFINE_UNQUOTED([HASH_FACTOR], [${with_hash_factor}], [How much hashing you want])

AC_MSG_CHECKING([for output block size])
AC_ARG_WITH([max-output], [AS_HELP_STRING([--with-max-output=NUMBER], [Maximum output block size @<:@default=16384@:>@])], [with_max_output=$withval], [with_max_output=16384])
AC_MSG_RESULT([$with_max_output])
AC_DEFINE_UNQUOTED([OUTPUT_BLOCK_SIZE], [${with_max_output}], [Maximum output block size])

AC_MSG_CHECKING([length of the DOING field in WHO])
AC_ARG_WITH([doing-length], [AS_HELP_STRING([--with-doing-length=NUMBER], [length of the DOING field in WHO @<:@default=41@:>@])], [with_doing_length=$withval], [with_doing_length=41])
AC_MSG_RESULT([$with_doing_length])
AC_DEFINE_UNQUOTED([DOING_LEN], [${with_doing_length}], [length of the DOING field in WHO])

AC_MSG_CHECKING([for Pueblo support message])
AC_ARG_WITH([pueblo-msg], [AS_HELP_STRING([--with-pueblo-msg="TEXT"], [Text that indicate Pueblo Support @<:@default="This world is Pueblo 1.0 enhanced\r\n\r\n"@:>@])], [with_pueblo_msg="$withval"], [with_pueblo_msg="This world is Pueblo 1.0 enhanced\r\n\r\n"])
AC_MSG_RESULT([$with_pueblo_msg])
AC_DEFINE_UNQUOTED([PUEBLO_SUPPORT_MSG], ["${with_pueblo_msg}"], [Pueblo support message])

AX_BOX([Checks for platform])
# Checks for platform.

AX_BOX([Checks for libraries])
# Checks for libraries.

dnl AC_CHECK_LIB(crypt, main)
AC_SEARCH_LIBS(crypt, crypt)
dnl AC_CHECK_LIB(resolv, main)
AC_SEARCH_LIBS(herror, resolv)
dnl AC_CHECK_LIB(socket, main)
AC_SEARCH_LIBS(socket, c socket)
dnl AC_CHECK_LIB(nsl, main)
dnl AC_CHECK_LIB(inet, gethostbyaddr)
AC_SEARCH_LIBS(gethostbyaddr, nsl inet)
dnl AC_CHECK_LIB(m, main)
AC_SEARCH_LIBS(sqrt, m)
dnl AC_CHECK_LIB(dbm, main)
dnl AC_CHECK_LIB(ndbm, main)
AC_SEARCH_LIBS(pcre_exec, pcre)

AX_BOX([Checks for header files.])
# Checks for header files.

AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h ctype.h dlfcn.h dl.h dld.h errno.h fcntl.h limits.h math.h malloc.h memory.h netdb.h netinet/in.h signal.h stddef.h stdlib.h stdio.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/select.h sys/socket.h sys/stat.h sys/time.h sys/types.h sys/utsname.h unistd.h pcre.h])
AS_IF([test "x$with_dbm" = "xgdbm"],[AC_CHECK_HEADERS([gdbm.h])]) 

AX_BOX([Checks for typedefs, structures, and compiler characteristics.])
# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_C_CHAR_UNSIGNED
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_DIAGNOSE([obsolete],[your code may safely assume C89 semantics that RETSIGTYPE is void.
Remove this warning and the `AC_CACHE_CHECK' when you adjust the code.])dnl
AC_CACHE_CHECK([return type of signal handlers],[ac_cv_type_signal],[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
],
		 [return *(signal (0, 0)) (0) == 1;])],
		   [ac_cv_type_signal=int],
		   [ac_cv_type_signal=void])])
AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],[Define as the return type of signal handlers
		    (`int' or `void').])

AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_STRUCT_TIMEZONE

AC_MSG_CHECKING([for struct linger in sys/socket.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/socket.h> ]], [[struct linger ling; ling.l_onoff = 1;]])],[AC_DEFINE(HAVE_LINGER, 1, Define if your <sys/socket.h> declares `struct linger'.) AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for union wait in sys/wait.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/wait.h>]], [[union wait stat;]])],[AC_DEFINE(HAVE_UNION_WAIT, 1, Define if your <sys/wait.h> declares `union wait'.) AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for struct rusage completeness in sys/resource.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/resource.h>]], [[struct rusage usage; usage.ru_ixrss = 1]])],[AC_DEFINE(STRUCT_RUSAGE_COMPLETE, 1, Define if your <sys/resource.h> declares everything we need in `struct rusage'.) AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for signal SIGCHLD braindamage in signal.h])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <signal.h>

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

int rlev;

RETSIGTYPE sighand(sig)
int sig;
{
  int stat;

  if (rlev++ > 2)
    exit(1);
  signal(SIGCHLD, sighand);
  wait(&stat);
  return;
}

main(argc, argv)
int argc;
char **argv;
{

  rlev = 0;
  signal(SIGCHLD, sighand);
  if (fork()) {
    sleep(10);
  } else {
    sleep(2);
    exit(1);
  }
  exit(0);
}]])],[AC_MSG_RESULT(no)],[AC_DEFINE(SIGNAL_SIGCHLD_BRAINDAMAGE, 1, Define if your <sys/signal> handle signals correctly.) AC_MSG_RESULT(yes)],[AC_DEFINE(SIGNAL_SIGCHLD_BRAINDAMAGE, 1, Define if your <sys/signal> handle signals correctly.) AC_MSG_RESULT(unable to test... assuming yes)])

AC_MSG_CHECKING([for broken small struct padding])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

main(argc, argv)
int argc;
char **argv;
{
  struct in_addr addr;
  char *dq;

  addr.s_addr = htonl(0x01020304);
  dq = inet_ntoa(addr);
  if (dq[0] != '1' || dq[1] != '.' || dq[2] != '2' || dq[3] != '.' ||
      dq[4] != '3' || dq[5] != '.' || dq[6] != '4' || dq[7] != '\0')
    return 1;
  return 0;
}]])],[AC_MSG_RESULT(no)],[AC_DEFINE(BROKEN_GCC_PADDING, 1, Define if your GCC break small struct padding) AC_MSG_RESULT(yes)],[AC_DEFINE(BROKEN_GCC_PADDING, 1, Define if your GCC break small struct padding) AC_MSG_RESULT(unable to test... assuming yes)])
    
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_MEMBERS([struct stat.st_blksize])

AX_BOX([Check for Declarations.])
# Check for Declarations

AC_CHECK_DECLS([sys_siglist],[],[],[#include <signal.h>
/* NetBSD declares sys_siglist in unistd.h.  */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
])


AC_MSG_CHECKING(for sys_signame in signal.h)
AC_EGREP_HEADER(sys_signame,signal.h,AC_DEFINE([HAVE_SYS_SIGNAME], [1], [Define if your <signal.h> doesn't declares `sys_signame'.]) AC_MSG_RESULT(yes),AC_MSG_RESULT(no))

AX_BOX([Checks for library functions.])
# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_FORK
AC_FUNC_VPRINTF
AC_FUNC_MMAP

AC_CHECK_FUNCS([alarm gethostbyaddr gethostbyname inet_ntoa isascii localtime_r munmap setrlimit srandom strcasecmp strchr strrchr strspn])

includedir=$includedir$with_install_suffix
libdir=$libdir$with_install_suffix
datarootdir=$datarootdir$with_install_suffix
sysconfdir=$sysconfdir$with_install_suffix

AC_SUBST([ac_aux_dir])
AC_DEFINE_UNQUOTED([PACKAGE_CONFIG], ["$ac_configure_args"], [Switches used with ./configure])
AC_DEFINE_UNQUOTED([PACKAGE_RELEASE_DATE], TINYMUSH_RELEASE_DATE, [Date of Release])
AC_DEFINE_UNQUOTED([PACKAGE_COPYRIGHT], TINYMUSH_COPYRIGHT, [Copyright Notice])

AX_BOX([Configuring files.])
AC_CONFIG_FILES([src/Makefile src/dbm/gdbm/Makefile src/dbm/qdbm/Makefile src/modules/comsys/Makefile src/modules/mail/Makefile src/modules/db_sql/Makefile Makefile])

AX_BOX([Generating config.status.])
AC_OUTPUT

AX_PRINT_PACKAGE_TITLE()
AX_PRINT([
                           %BPath
-------------------------------%b
         Installation directory : %B$prefix%b
             Binaries directory : %B$prefix/bin%b
             Includes directory : %B$includedir%b
            Libraries directory : %B$libdir%b
            Help Text directory : %B$datarootdir%b
                 Data directory : %B$sysconfdir%b

                         %BLimits
-------------------------------%b
        Length for player names : %B$with_pname_length%b
             Number of env vars : %B$with_env_vars%b
         Args for cmd processor : %B$with_max_args%b
          Number of iter levels : %B$with_max_iter%b
        global registers to use : %B$with_global_regs%b
          Lenght of a delimiter : %B$with_max_delim%b
                    Hash factor : %B$with_hash_factor%b
              Output block size : %B$with_max_output%b
             DOING field length : %B$with_doing_length%b
             
                        %BMarkers
---------------------------------%b
     separator for marker flags : %B$with_flag_sep%b
         Pueblo support message : %B$with_pueblo_msg%b

                       %BDatabase
-------------------------------%b
                     DBM Engine : %B$with_dbm%b

                        %BModules
-------------------------------%b
                  Module ComSYS : %B$enable_module_comsys%b
                    Module Mail : %B$enable_module_mail%b
                   Module DBSQL : %B$enable_module_dbsql%b with driver %B$with_module_dbsql%b

                           %BMisc
-------------------------------%b
                 Pueblo clients : %B$enable_pueblo_support%b
IEEE 754 floating point support	: %B$enable_floating_point%b
          Command lag detection : %B$enable_lag_checking%b
 Per-object time usage tracking : %B$enable_time_checking%b
              CPU time checking : %B$enable_cpu_checking%b
                 Malloc Wrapper : %B$with_malloc_wrapper%b

If everything is ok, you can now type %Bmake install%b to install the engine.])
