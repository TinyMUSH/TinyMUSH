#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

AC_INIT([TinyMUSH], [3.3.0.0], [tinymush-bugs@lists.sourceforge.net], [tinymush], [http://tinymush.sourceforge.net])
dnl AC_INIT define the following itself:
dnl PACKAGE_NAME
dnl PACKAGE_TARNAME
dnl PACKAGE_VERSION
dnl PACKAGE_STRING
dnl PACKAGE_BUGREPORT
dnl PACKAGE_URL
dnl With AX_SPLIT_VERSION, we define the following based on autoconf's values:
dnl PACKAGE_VERSION_MAJOR
dnl PACKAGE_VERSION_MINOR
dnl PACKAGE_RELEASE_STATUS
dnl PACKAGE_RELEASE_NAME
dnl PACKAGE_RELEASE_REVISION
AX_SPLIT_VERSION()
dnl And we manually create a few other things tha need to be updated from time to time.
dnl
PACKAGE_RELEASE_DATE="2012-02-19"
PACKAGE_COPYRIGHT="Copyright (c) 2012 TinyMUSH development team"

dnl Some default values
dnl
with_included_ltdl="yes"

AC_COPYRIGHT([$PACKAGE_COPYRIGHT])
AC_CONFIG_AUX_DIR([libltdl/config])
AC_CONFIG_SRCDIR([game.c])
AC_CONFIG_HEADERS([autoconf.h])
AC_CONFIG_MACRO_DIR([libltdl/m4])

AX_PRINT_PACKAGE_TITLE()

AX_BOX([Initializing Automake])

AM_INIT_AUTOMAKE()

AX_BOX([Initializing Libtools])

LT_PREREQ([2.4])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
AC_SUBST([LIBTOOL_DEPS])
LTDL_INIT([convenience])

AX_BOX([Checks for Options])

dnl XXX // This really need to be check...

AC_MSG_CHECKING([configuration prefix])
 case "$prefix" in
  NONE) prefix=`pwd -P | sed 's:/[[^/]]*$::'`/game;;
 esac
AC_MSG_RESULT([$prefix])

AH_TOP([#ifndef __AUTOCONF_H
#define __AUTOCONF_H

#include "copyright.h"])

AH_BOTTOM([#include "autoinclude.h"

#endif /* __AUTOCONF_H */])

AC_MSG_CHECKING([dbm engine to use])
AC_ARG_WITH([dbm],
 [AS_HELP_STRING([--with-dbm=gdbm|qdbm], [Select the DBM driver used to save game data, valid values are : gdbm Use TinyMUSH-GDBM hash database driver, This is the historical driver used by most games or qdbm : Use TinyMUSH-QDBM hash database driver. A newer implementation of the DBM engine, a compromise between gdbm and memory-based cache. @<:@default=gdbm@:>@])],
 [AS_CASE([$withval], [gdbm], [with_dbm="gdbm"], [qdbm], [with_dbm="qdbm"], [with_dbm="gdbm"])], 
 [with_dbm="gdbm"])
AC_MSG_RESULT([$with_dbm])

AS_IF([test "x$with_dbm" = "xqdbm"], 
 [
  AC_DEFINE([MUSH_DBM], ["QDBM"], [TinyQDBM dbm used])
  AC_DEFINE([HAVE_LIBTINYQDBM_H], [1], [Define to 1 if you have <libtinyqdbm.h>.])
 ], 
 [
  AC_DEFINE([MUSH_DBM], ["GDBM"], [TinyGDBM dbm used])
  AC_DEFINE([HAVE_LIBTINYGDBM_H], [1], [Define to 1 if you have <libtinyqdbm.h>.])
 ]
)

AM_CONDITIONAL([WANT_DBM_GDBM], [test "x$with_dbm" = "xgdbm"])
AM_CONDITIONAL([WANT_DBM_QDBM], [test "x$with_dbm" = "xqdbm"])

AC_MSG_CHECKING([whether to enable disk-based database])
AC_ARG_ENABLE([disk-based], [AS_HELP_STRING([--enable-disk-based], [store the database on disk using a DBM hashfile, and cache some attributes in a limited amount of memory. The alternative is memory-based, where the database is kept in memory. Disabling the disk-based cache can improve performance if enough memory is available. @<:@default=yes@:>@])],
 [AS_IF([test "x$enableval" = "xno"],
  [
   AC_DEFINE([MEMORY_BASED], [], [Enable memory-based DBM hashDB])
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

AC_MSG_CHECKING([whether the hello module is enable])
AC_ARG_ENABLE([module-hello], [AS_HELP_STRING([--enable-module-hello], [Enable support for the hello module @<:@default=no@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [
  AC_MSG_RESULT([no])
  enable_module_hello="no"
 ]
)
AM_CONDITIONAL([WANT_MODULE_HELLO], [test "x$enable_module_hello" = "xyes"])

AC_MSG_CHECKING([whether to enable pueblo support])
AC_ARG_ENABLE([pueblo-support], [AS_HELP_STRING([--enable-pueblo-support], [Enable support for Pueblo clients @<:@default=yes@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_pueblo_support="yes"
 ]
)
AS_IF([test "x$enable_pueblo_support" = "xyes"], [AC_DEFINE([PUEBLO_SUPPORT],[], [Support for Pueblo Clients])])

AC_MSG_CHECKING([whether to enable IEEE 754 floating point support])
AC_ARG_ENABLE([floating-point], [AS_HELP_STRING([--enable-floating-point], [Enable support for math functions (lt, gt, add, etc.) with IEEE 754 floating-point support @<:@default=yes@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_floating_point="yes"
 ]
)
AS_IF([test "x$enable_floating_point" = "xyes"], [AC_DEFINE([FLOATING_POINTS],[], [Support for IEEE 754 floating-point])])

AC_MSG_CHECKING([whether to enable command lag detection support])
AC_ARG_ENABLE([lag-checking], [AS_HELP_STRING([--enable-lag-checking], [Enable warnings that a certain command has exceeded a certain amount of wall-clock time to execute. Disable this if you have severe problems with computational resources on your host machine. @<:@default=yes@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_lag_checking="yes"
 ]
)
AS_IF([test "x$enable_lag_checking" = "xno"], [AC_DEFINE([NO_LAG_CHECK],[], [Support for command lag detection support])])

AC_MSG_CHECKING([whether to enable per-object time usage tracking])
AC_ARG_ENABLE([time-checking], [AS_HELP_STRING([--enable-time-checking], [Enable tracking of the amount of wall-clock time that individual objects are using. Disable this if you have severe problems with computational resources on your host machine. (This is assumed to be disabled if lag-checking is disabled) @<:@default=yes@:>@])], 
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_time_checking="yes"
 ]
)
AS_IF([test "x$enable_time_checking" = "xno"], [AC_DEFINE([NO_TIMECHECKING],[], [Support for per-object time usage tracking])])

AC_MSG_CHECKING([whether to enable cpu time checking])
AC_ARG_ENABLE([cpu-checking], [AS_HELP_STRING([--enable-cpu-checking], [Enable CPU time reported with getrusage() instead of tracking wall-clock time for lag-checking and time-checking features. This is more accurate but also more computationally expensive. @<:@default=no@:>@])],
 [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([no])
  enable_cpu_checking="no"
 ]
)
AS_IF([test "x$enable_cpu_checking" = "xyes"], [AC_DEFINE([TRACK_USER_TIME],[], [Support for cpu time checking])])

AC_MSG_CHECKING([which malloc-wrapper macros to use])
AC_ARG_WITH([malloc-wrapper], [AS_HELP_STRING([--with-malloc-wrapper=@<:@track|log@:>@], [Enable use of wrapper macros for malloc(), free(), and related functions to do one of the following, which can help identify memory leaks: track - maintain a tagged list of memory allocated for purposes other than the buffer pool. (see @list raw_memory) log - print log messages when the function are called. This is spammy; try other debugging tools first.])],
  AS_IF([ test "x$withval" = "xtrack" ], AC_DEFINE([RAW_MEMTRACKING],[], [Support for cpu time checking]), AS_IF([ test "x$withval" = "xlog" ], AC_DEFINE([TEST_MALLOC],[], [Support for cpu time checking]), [with_malloc_wrapper="none"])),
  [with_malloc_wrapper="none"]
)

AC_MSG_CHECKING([how many global registers to use])
AC_ARG_WITH([global-regs], [AS_HELP_STRING([--with-global-regs=N], [use N global registers (%q0-%q9,%qa-%qz) N must be a number from 10 to 36. Set lower to conserve memory if you don't need all the possible registers  @<:@default=36@:>@])],
  AS_IF([ test "$withval" -lt "10" ], [with_global_regs=10], AS_IF([ test "$withval" -gt "36" ], [with_global_regs=36], [with_global_regs=$withval])),
  [with_global_regs=36]
)
AC_MSG_RESULT([$with_global_regs])
AC_DEFINE_UNQUOTED([MAX_GLOBAL_REGS], [${with_global_regs}], [How many global registers to use])

AC_MSG_CHECKING([default filename for the configuration])
AC_ARG_WITH([conf-file], [AS_HELP_STRING([--with-conf-file=NAME], [default name of the config file @<:@default=netmush.conf@:>@])], [with_conf_file="$withval"], [with_conf_file="netmush.conf"])
AC_MSG_RESULT([$with_conf_file])
AC_DEFINE_UNQUOTED([CONF_FILE], ["${with_conf_file}"], [default config filename])

AC_MSG_CHECKING([default filename for logs])
AC_ARG_WITH([log-file], [AS_HELP_STRING([--with-log-file=NAME], [default name of the log file @<:@default=netmush.log@:>@])], [with_log_file="$withval"], [with_log_file="netmush.log"])
AC_MSG_RESULT([$with_log_file])
AC_DEFINE_UNQUOTED([LOG_FILE], ["${with_log_file}"], [default log filename])

AC_MSG_CHECKING([default filename for the pid])
AC_ARG_WITH([pid-file], [AS_HELP_STRING([--with-pid-file=NAME], [default name of the pid file @<:@default=netmush.pid@:>@])], [with_pid_file="$withval"], [with_pid_file="netmush.pid"])
AC_MSG_RESULT([$with_pid_file])
AC_DEFINE_UNQUOTED([PID_FILE], ["${with_pid_file}"], [default pid filename])

AC_MSG_CHECKING([default filename for the db])
AC_ARG_WITH([db-file], [AS_HELP_STRING([--with-db-file=NAME], [default name of the db file @<:@default=netmush.db@:>@])], [with_db_file="$withval"], [with_db_file="netmush.db"])
AC_MSG_RESULT([$with_db_file])
AC_DEFINE_UNQUOTED([DB_FILE], ["${with_db_file}"], [default db filename])

AC_MSG_CHECKING([default filename for the crash db])
AC_ARG_WITH([crash-file], [AS_HELP_STRING([--with-crash-file=NAME], [default name of the crash db file @<:@default=netmush.db.crash@:>@])], [with_crash_file="$withval"], [with_crash_file="netmush.db.crash"])
AC_MSG_RESULT([$with_crash_file])
AC_DEFINE_UNQUOTED([CRASH_FILE], ["${with_crash_file}"], [default crash db filename])

AC_MSG_CHECKING([default name for the bin directory])
AC_ARG_WITH([bin-dir], [AS_HELP_STRING([--with-bin-dir=NAME], [default name of the bin directory @<:@default=$bindir@:>@])], [with_bin_dir="$withval"], [with_bin_dir="$prefix/bin"])
AC_MSG_RESULT([$with_bin_dir])
AC_DEFINE_UNQUOTED([BIN_DIR], ["${with_bin_dir}"], [default bin dirname])

AC_MSG_CHECKING([default name for the txt directory])
AC_ARG_WITH([txt-dir], [AS_HELP_STRING([--with-txt-dir=NAME], [default name of the txt directory @<:@default=$datadir@:>@])], [with_txt_dir="$withval"], [with_txt_dir="$prefix/txt"])
AC_MSG_RESULT([$with_txt_dir])
AC_DEFINE_UNQUOTED([TXT_DIR], ["${with_txt_dir}"], [default txt dirname])

AC_MSG_CHECKING([default name for the data directory])
AC_ARG_WITH([data-dir], [AS_HELP_STRING([--with-data-dir=NAME], [default name of the data directory @<:@default=$localstatedir@:>@])], [with_data_dir="$withval"], [with_data_dir="$prefix/data"])
AC_MSG_RESULT([$with_data_dir])
AC_DEFINE_UNQUOTED([DATA_DIR], ["${with_data_dir}"], [default data dirname])

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

dnl #define MARK_FLAG_SEP           '_'     /* sep. of dbref from marker flags */

dnl #define HASH_FACTOR             2       /* How much hashing you want. */

dnl #define OUTPUT_BLOCK_SIZE       16384

dnl #define DOING_LEN               41      /* length of the DOING field in WHO */

dnl #define PUEBLO_SUPPORT_MSG "This world is Pueblo 1.0 enhanced\r\n\r\n"

AX_BOX([Checks for programs])
# Checks for programs.

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CPP_WERROR
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AX_BOX([Checks for platform])
# Checks for platform.

AX_BOX([Checks for libraries])
# Checks for libraries.

AC_CHECK_LIB(crypt, main)
AC_CHECK_LIB(resolv, main)
AC_CHECK_LIB(socket, main)
AC_CHECK_LIB(nsl, main)
AC_CHECK_LIB(inet, gethostbyaddr)
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(dbm, main)
AC_CHECK_LIB(ndbm, main)

AX_BOX([Checks for header files.])
# Checks for header files.

AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h ctype.h dlfcn.h dl.h dld.h errno.h fcntl.h limits.h math.h malloc.h memory.h netdb.h netinet/in.h signal.h stddef.h stdlib.h stdio.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/select.h sys/stat.h sys/time.h sys/types.h sys/utsname.h unistd.h])

AX_BOX([Checks for typedefs, structures, and compiler characteristics.])
# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_C_CHAR_UNSIGNED
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
#AC_ST_BLKSIZE

AC_MSG_CHECKING([for struct linger in sys/socket.h])
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>] , [struct linger ling; ling.l_onoff = 1;], AC_DEFINE([HAVE_LINGER], [], [Define if your <sys/socket.h> declares `struct linger'.]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING([for union wait in sys/wait.h])
AC_TRY_COMPILE([#include <sys/wait.h>],[union wait stat;], AC_DEFINE([HAVE_UNION_WAIT], [], [Define if your <sys/wait.h> declares `union wait'.]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING([for struct rusage completeness in sys/resource.h])
AC_TRY_COMPILE([#include <sys/resource.h>], [struct rusage usage; usage.ru_ixrss = 1], AC_DEFINE([STRUCT_RUSAGE_COMPLETE], [], [Define if your <sys/resource.h> declares everything we need in `struct rusage'.]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING([for signal SIGCHLD braindamage in signal.h])
AC_TRY_RUN([
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
}], AC_MSG_RESULT([no]), AC_DEFINE([SIGNAL_SIGCHLD_BRAINDAMAGE], [], [Define if your <sys/signal> handle signals correctly.]) AC_MSG_RESULT([yes]), AC_DEFINE([SIGNAL_SIGCHLD_BRAINDAMAGE], [], [Define if your <sys/signal> handle signals correctly.]) AC_MSG_RESULT([unable to test... assuming yes]))

AC_MSG_CHECKING([for broken small struct padding])
AC_TRY_RUN([
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
}],AC_MSG_RESULT([no]), AC_DEFINE([BROKEN_GCC_PADDING], [], [Define if your GCC break small struct padding]) AC_MSG_RESULT([yes]), AC_DEFINE([BROKEN_GCC_PADDING], [], [Define if your GCC break small struct padding]) AC_MSG_RESULT([unable to test... assuming yes]))
    
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_MEMBERS([struct stat.st_blksize])

AX_BOX([Check for Declarations.])
# Check for Declarations

AC_DECL_SYS_SIGLIST

AC_MSG_CHECKING(for sys_signame in signal.h)
AC_EGREP_HEADER(sys_signame,signal.h,AC_DEFINE([HAVE_SYS_SIGNAME], [], [Define if your <signal.h> doesn't declares `sys_signame'.]) AC_MSG_RESULT(yes),AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getpagesize in unistd.h)
AC_EGREP_HEADER(getpagesize,unistd.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_GETPAGESIZE_DCL], [], [Define if your <unistd.h> doesn't declares getpagesize]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for realloc in stdlib.h)
AC_EGREP_HEADER(realloc,stdlib.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_MALLOC_DCL], [], [Define if your <stdlib.h> doesn't declares realloc]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for vsprintf in stdio.h)
AC_EGREP_HEADER(vsprintf,stdio.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_VSPRINTF_DCL], [], [Define if your <stdio.h> doesn't declares vsprintf]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for sprintf in stdio.h)
AC_EGREP_HEADER(\<sprintf|\<*sprintf,stdio.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_SPRINTF_DCL], [], [Define if your <stdio.h> doesn't declares sprintf]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for fread in stdio)
AC_EGREP_HEADER(fread,stdio.h,AC_DEFINE([EXTENDED_STDIO_DCLS], [], [Define if your <stdio.h> doesn't declares fread]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING(for setsockopts in socket.h)
AC_EGREP_HEADER(setsockopt,sys/socket.h,AC_DEFINE([EXTENDED_SOCKET_DCLS], [], [Define if your <sys/socket.h> doesn't declares setsockopt]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING(for gettimeofday in time.h or sys/time.h)
if test $ac_cv_header_time = yes && test $ac_cv_header_sys_time_h = yes ; then
 AC_EGREP_HEADER(gettimeofday,sys/time.h,AC_MSG_RESULT(yes),AC_EGREP_HEADER(gettimeofday,time.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_GETTIMEOFDAY_DCL], [], [Define if your <time.h> or <sys/time.h> doesn't declares gettimeofday]) AC_MSG_RESULT(no)))
elif test $ac_cv_header_sys_time_h = yes ; then
 AC_EGREP_HEADER(gettimeofday,sys/time.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_GETTIMEOFDAY_DCL], [], [Define if your <sys/time.h> doesn't declares gettimeofday]) AC_MSG_RESULT(no))
else
 AC_EGREP_HEADER(gettimeofday,time.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_GETTIMEOFDAY_DCL], [], [Define if your <time.h> doesn't declares gettimeofday]) AC_MSG_RESULT(no))
fi

if test $ac_cv_header_sys_wait_h = yes ; then
 AC_MSG_CHECKING(for wait3 in wait.h)
 AC_EGREP_HEADER(wait3,sys/wait.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_WAIT3_DCL], [], [Define if your <wait.h> doesn't declares wait3]) AC_MSG_RESULT(no))

 AC_MSG_CHECKING(for wait4 in wait.h)
 AC_EGREP_HEADER(wait4,sys/wait.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_WAIT4_DCL], [], [Define if your <wait.h> doesn't declares wait3]) AC_MSG_RESULT(no))
fi

AC_MSG_CHECKING(for srandom in math.h or stdlib.h)
AC_EGREP_HEADER(srandom,math.h,AC_MSG_RESULT(yes),AC_EGREP_HEADER(srandom,stdlib.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_SRANDOM_DCL], [], [Define if your <math.h> or <stdlib.h> doesn't declares srandom]) AC_MSG_RESULT(no)))

AC_MSG_CHECKING(for random in stdlib.h)
AC_EGREP_HEADER(extern * random,stdlib.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_RANDOM_DCL], [], [Define if your <stdlib.h> doesn't declares random]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for perror in errno.h)
AC_EGREP_HEADER(perror,errno.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_PERROR_DCL], [], [Define if your <errno.h> doesn't declares perror]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getrusage in sys/resource.h)
AC_EGREP_HEADER(getrusage,sys/resource.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_GETRUSAGE_DCL], [], [Define if your <sys/resource.h> doesn't declares getrusage]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getrlimit sys/resource.h)
AC_EGREP_HEADER(getrlimit,sys/resource.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_GETRLIMIT_DCL], [], [Define if your <sys/resource.h> doesn't declares getrlimit]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for strtok_r dcl)
AC_EGREP_HEADER(strtok_r,string.h,AC_MSG_RESULT(yes),AC_DEFINE([NEED_STRTOK_R_DCL], [], [Define if your <string.h> doesn't declares strtok_r]) AC_MSG_RESULT(no))

AX_BOX([Checks for library functions.])
# Checks for library functions.
AC_FUNC_FORK
dnl AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MMAP
dnl AC_FUNC_STRTOD
dnl AC_FUNC_MALLOC
dnl AC_FUNC_REALLOC
AC_FUNC_VFORK
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm atexit bcopy dup2 floor ftruncate flock fsync getdtablesize gethostbyaddr gethostbyname getpagesize getrusage gettimeofday inet_ntoa isascii localtime_r memmove memset mkdir mktime munmap pow rename rmdir select setrlimit socket sqrt srandom strcasecmp strchr strcspn strdup strerror strpbrk strrchr strspn strstr strtok_r strtol strtoul timelocal tzset vsnprintf wait3 wait4])

AC_SUBST([ac_aux_dir])
AC_DEFINE_UNQUOTED([PACKAGE_CONFIG], ["$ac_configure_args"], [Switches used with ./configure])
AC_DEFINE_UNQUOTED([PACKAGE_RELEASE_DATE], ["$PACKAGE_RELEASE_DATE"], [Date of Release])

AX_BOX([Configuring files.])
AC_CONFIG_FILES([dbm/gdbm/Makefile dbm/qdbm/Makefile modules/comsys/Makefile modules/mail/Makefile modules/hello/Makefile Makefile])

AX_BOX([Generating config.status.])
AC_OUTPUT

AX_PRINT_PACKAGE_TITLE()
AX_PRINT([
                           %BPath
-------------------------------%b
         Installation directory : %B$prefix%b
             Binaries directory : %B$with_bin_dir%b
            Help Text directory : %B$with_txt_dir%b
                 Data directory : %B$with_data_dir%b

                      %BFilenames
-------------------------------%b
 default configuration filename : %B$with_conf_file%b
           default log filename : %B$with_log_file%b
           default pid filename : %B$with_pid_file%b
            default db filename : %B$with_db_file%b
         default crash filename : %B$with_crash_file%b
         
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
                   Module Hello : %B$enable_module_hello%b

                           %BMisc
-------------------------------%b
                 Pueblo clients : %B$enable_pueblo_support%b
IEEE 754 floating point support	: %B$enable_floating_point%b
          Command lag detection : %B$enable_lag_checking%b
 Per-object time usage tracking : %B$enable_time_checking%b
              CPU time checking : %B$enable_cpu_checking%b
                 Malloc Wrapper : %B$with_malloc_wrapper%b

If everything is ok, you can now type %Bmake install%b to install the engine.])
