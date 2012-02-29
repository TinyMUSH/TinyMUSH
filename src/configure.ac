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
AC_ARG_ENABLE([disk-based],
 [AS_HELP_STRING([--enable-disk-based], [store the database on disk using a DBM hashfile, and cache some attributes in a limited amount of memory. The alternative is memory-based, where the database is kept in memory. Disabling the disk-based cache can improve performance if enough memory is available. @<:@default=yes@:>@])],
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
AC_ARG_ENABLE([module-comsys], [ 
  AS_HELP_STRING([--enable-module-comsys], [Enable support for the comsys module @<:@default=yes@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [
  AC_MSG_RESULT([yes])
  enable_module_comsys="yes"
 ]
)
AM_CONDITIONAL([WANT_MODULE_COMSYS], [test "x$enable_module_comsys" = "xyes"])

AC_MSG_CHECKING([whether the mail module is enable])
AC_ARG_ENABLE([module-mail], [ 
  AS_HELP_STRING([--enable-module-mail], [Enable support for the mail module @<:@default=yes@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [
  AC_MSG_RESULT([yes])
  enable_module_mail="yes"
 ]
)
AM_CONDITIONAL([WANT_MODULE_MAIL], [test "x$enable_module_mail" = "xyes"])

AC_MSG_CHECKING([whether the hello module is enable])
AC_ARG_ENABLE([module-hello], [ 
  AS_HELP_STRING([--enable-module-hello], [Enable support for the hello module @<:@default=no@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [
  AC_MSG_RESULT([no])
  enable_module_hello="no"
 ]
)
AM_CONDITIONAL([WANT_MODULE_HELLO], [test "x$enable_module_hello" = "xyes"])

dnl XXX Modukles below this line need to be checked XXX
AC_MSG_CHECKING([whether to enable pueblo support])
AC_ARG_ENABLE([pueblo-support], [ 
  AS_HELP_STRING([--enable-pueblo-support], [Enable support for Pueblo clients @<:@default=yes@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_pueblo_support="yes"
 ]
)
AS_IF([test "x$enable_pueblo_support" = "xyes"], [AC_DEFINE([PUEBLO_SUPPORT],[], [Support for Pueblo Clients])])

AC_MSG_CHECKING([whether to enable IEEE 754 floating point support])
AC_ARG_ENABLE([floating-point], [
  AS_HELP_STRING([--enable-floating-point], [Enable support for math functions (lt, gt, add, etc.) with IEEE 754 floating-point support @<:@default=yes@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_floating_point="yes"
 ]
)
AS_IF([test "x$enable_floating_point" = "xyes"], [AC_DEFINE([FLOATING_POINTS],[], [Support for IEEE 754 floating-point])])

AC_MSG_CHECKING([whether to enable command lag detection support])
AC_ARG_ENABLE([lag-checking], [
  AS_HELP_STRING([--enable-lag-checking], [Enable warnings that a certain command has exceeded a certain amount of wall-clock time to execute. Disable this if you have severe problems with computational resources on your host machine. @<:@default=yes@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_lag_checking="yes"
 ]
)
AS_IF([test "x$enable_lag_checking" = "xno"], [AC_DEFINE([NO_LAG_CHECK],[], [Support for command lag detection support])])

AC_MSG_CHECKING([whether to enable per-object time usage tracking])
AC_ARG_ENABLE([time-checking], [
  AS_HELP_STRING([--enable-time-checking], [Enable tracking of the amount of wall-clock time that individual objects are using. Disable this if you have severe problems with computational resources on your host machine. (This is assumed to be disabled if lag-checking is disabled) @<:@default=yes@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([yes])
  enable_time_checking="yes"
 ]
)
AS_IF([test "x$enable_time_checking" = "xno"], [AC_DEFINE([NO_TIMECHECKING],[], [Support for per-object time usage tracking])])

AC_MSG_CHECKING([whether to enable cpu time checking])
AC_ARG_ENABLE([cpu-checking], [
  AS_HELP_STRING([--enable-cpu-checking], [Enable CPU time reported with getrusage() instead of tracking wall-clock time for lag-checking and time-checking features. This is more accurate but also more computationally expensive. @<:@default=no@:>@])
 ], [
  AX_ENABLE_YESNO([$enableval])
 ], [ 
  AC_MSG_RESULT([no])
  enable_cpu_checking="no"
 ]
)
AS_IF([test "x$enable_cpu_checking" = "xyes"], [AC_DEFINE([TRACK_USER_TIME],[], [Support for cpu time checking])])

AC_MSG_CHECKING([which malloc-wrapper macros to use])
AC_ARG_WITH([malloc-wrapper],
  AS_HELP_STRING([--with-malloc-wrapper=@<:@track|log@:>@], [Enable use of wrapper macros for malloc(), free(), and related functions to do one of the following, which can help identify memory leaks: track - maintain a tagged list of memory allocated for purposes other than the buffer pool. (see @list raw_memory) log - print log messages when the function are called. This is spammy; try other debugging tools first.]),
  AS_IF([ test "x$withval" = "xtrack" ], AC_DEFINE([RAW_MEMTRACKING],[], [Support for cpu time checking]), AS_IF([ test "x$withval" = "xlog" ], AC_DEFINE([TEST_MALLOC],[], [Support for cpu time checking]), [with_malloc_wrapper="none"])),
  [with_malloc_wrapper="none"]
)

AC_MSG_CHECKING([how many global registers to use])
AC_ARG_WITH([global-regs],
  AS_HELP_STRING([--with-global-regs=N], [use N global registers (%q0-%q9,%qa-%qz) N must be a number from 10 to 36. Set lower to conserve memory if you don't need all the possible registers  @<:@default=36@:>@]),
  AS_IF([ test "$withval" -lt "10" ], [with_global_regs=10], AS_IF([ test "$withval" -gt "36" ], [with_global_regs=36], [with_global_regs=$withval])),
  [with_global_regs=36]
)

AC_MSG_RESULT([$with_global_regs])
AC_DEFINE_UNQUOTED([MAX_GLOBAL_REGS], [$with_global_regs], [How many global registers to use])

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
AC_CHECK_HEADERS([arpa/inet.h ctype.h dlfcn.h dl.h dld.h errno.h fcntl.h limits.h math.h malloc.h memory.h netdb.h netinet/in.h signal.h stddef.h stdlib.h stdio.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/select.h sys/stat.h sys/time.h sys/types.h sys/utsname.h unistd.h])

AC_MSG_CHECKING(for struct linger)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>] , [struct linger ling; ling.l_onoff = 1;], AC_DEFINE(HAVE_LINGER, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
AC_MSG_CHECKING(for union wait)
AC_TRY_COMPILE([#include <sys/wait.h>],[union wait stat;],
  AC_DEFINE(HAVE_UNION_WAIT, [], [--- ADD DESCRIPTION ---])
    AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no))

AC_MSG_CHECKING(for struct rusage completeness)
AC_TRY_COMPILE([#include <sys/resource.h>],
[struct rusage usage; usage.ru_ixrss = 1],
  AC_DEFINE(STRUCT_RUSAGE_COMPLETE, [], [--- ADD DESCRIPTION ---])
    AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no))


AC_MSG_CHECKING(for signal SIGCHLD braindamage)
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
}
], AC_MSG_RESULT(no),
  AC_DEFINE(SIGNAL_SIGCHLD_BRAINDAMAGE, [], [--- ADD DESCRIPTION ---])
    AC_MSG_RESULT(yes),
  AC_DEFINE(SIGNAL_SIGCHLD_BRAINDAMAGE, [], [--- ADD DESCRIPTION ---])
    AC_MSG_RESULT(unable to test... assuming yes))

AC_MSG_CHECKING(for broken small struct padding)
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
}],AC_MSG_RESULT(no),
  AC_DEFINE([BROKEN_GCC_PADDING], [], [--- ADD DESCRIPTION ---])
    AC_MSG_RESULT(yes),
  AC_DEFINE([BROKEN_GCC_PADDING], [], [--- ADD DESCRIPTION ---])
    AC_MSG_RESULT(unable to test... assuming yes))
    
AX_BOX([Checks for typedefs, structures, and compiler characteristics.])
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_C_CONST
AC_C_CHAR_UNSIGNED
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_ST_BLKSIZE
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_MEMBERS([struct stat.st_blksize])

AX_BOX([Check for Declarations.])
# Check for Declarations
AC_DECL_SYS_SIGLIST

AC_MSG_CHECKING(for sys_signame decl)
AC_EGREP_HEADER(sys_signame,signal.h,AC_DEFINE(HAVE_SYS_SIGNAME, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(yes),AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getpagesize dcl)
AC_EGREP_HEADER(getpagesize,unistd.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_GETPAGESIZE_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for malloc dcl)
AC_EGREP_HEADER(realloc,stdlib.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_MALLOC_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for vsprintf dcl)
AC_EGREP_HEADER(vsprintf,stdio.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_VSPRINTF_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for sprintf dcl)
AC_EGREP_HEADER(\<sprintf|\<*sprintf,stdio.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_SPRINTF_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for extended stdio dcls)
AC_EGREP_HEADER(fread,stdio.h,AC_DEFINE(EXTENDED_STDIO_DCLS, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING(for extended socket dcls)
AC_EGREP_HEADER(setsockopt,sys/socket.h,AC_DEFINE(EXTENDED_SOCKET_DCLS, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

AC_MSG_CHECKING(for gettimeofday dcl)
if test $ac_cv_header_time = yes && test $ac_cv_header_sys_time_h = yes ; then
AC_EGREP_HEADER(gettimeofday,sys/time.h,AC_MSG_RESULT(yes),AC_EGREP_HEADER(gettimeofday,time.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_GETTIMEOFDAY_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no)))
elif test $ac_cv_header_sys_time_h = yes ; then
AC_EGREP_HEADER(gettimeofday,sys/time.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_GETTIMEOFDAY_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))
else
AC_EGREP_HEADER(gettimeofday,time.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_GETTIMEOFDAY_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))
fi
if test $ac_cv_header_sys_wait_h = yes ; then

AC_MSG_CHECKING(for wait3 dcl)
AC_EGREP_HEADER(wait3,sys/wait.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_WAIT3_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for wait4 dcl)
AC_EGREP_HEADER(wait4,sys/wait.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_WAIT4_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))
fi

AC_MSG_CHECKING(for srandom dcl)
AC_EGREP_HEADER(srandom,math.h,AC_MSG_RESULT(yes),AC_EGREP_HEADER(srandom,stdlib.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_SRANDOM_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no)))

AC_MSG_CHECKING(for random dcl)
AC_EGREP_HEADER(extern * random,stdlib.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_RANDOM_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for perror dcl)
AC_EGREP_HEADER(perror,errno.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_PERROR_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getrusage dcl)
AC_EGREP_HEADER(getrusage,sys/resource.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_GETRUSAGE_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getrlimit dcl)
AC_EGREP_HEADER(getrlimit,sys/resource.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_GETRLIMIT_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for strtok_r dcl)
AC_EGREP_HEADER(strtok_r,string.h,AC_MSG_RESULT(yes),AC_DEFINE(NEED_STRTOK_R_DCL, [], [--- ADD DESCRIPTION ---]) AC_MSG_RESULT(no))

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

AX_BOX([Writing configuration.])
# Write results.
AC_SUBST([ac_aux_dir])
AC_DEFINE_UNQUOTED([PACKAGE_CONFIG], ["$ac_configure_args"], [Switches used with ./configure])
AC_DEFINE_UNQUOTED([PACKAGE_RELEASE_DATE], ["$PACKAGE_RELEASE_DATE"], [Date of Release])

AC_CONFIG_FILES([dbm/gdbm/Makefile dbm/qdbm/Makefile modules/comsys/Makefile modules/mail/Makefile modules/hello/Makefile Makefile])

AC_OUTPUT

AX_PRINT_PACKAGE_TITLE()
AX_PRINT([
         Installation directory : %B$prefix%b
                     DBM Engine : %B$with_dbm%b
                  Module ComSYS : %B$enable_module_comsys%b
                    Module Mail : %B$enable_module_mail%b
                   Module Hello : %B$enable_module_hello%b
                 Pueblo clients : %B$enable_pueblo_support%b
IEEE 754 floating point support	: %B$enable_floating_point%b
          Command lag detection : %B$enable_lag_checking%b
 Per-object time usage tracking : %B$enable_time_checking%b
              CPU time checking : %B$enable_cpu_checking%b
                 Malloc Wrapper : %B$with_malloc_wrapper%b
        global registers to use : %B$with_global_regs%b

If everything is ok, you can now type %Bmake install%b to install the engine.])
