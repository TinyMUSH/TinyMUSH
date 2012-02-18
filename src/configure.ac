#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TinyMUSH], [3.3.0.0], [tinymush-bugs@lists.sourceforge.net])
package_config=$@
package_release_date="YYYY/MM/DD"
AC_COPYRIGHT([Copyright (c) 2011 TinyMUSH development team.])
AC_CONFIG_AUX_DIR([libltdl/config])
AC_CONFIG_SRCDIR([game.c])
AC_CONFIG_HEADERS([autoconf.h])
AC_CONFIG_MACRO_DIR([libltdl/m4])

AX_SPLIT_VERSION()

PRETTY_PRINT([
%B${PACKAGE_NAME}%b version %B${PACKAGE_VERSION}%b.
  Copyright (c) 2011, TinyMUSH development team. All rights reserved.
  See %Bhttp://tinymush.sourceforge.net%b for more informations.
])

PRETTY_PRINT([configure], [Initializing Automake.], [1])

AM_INIT_AUTOMAKE()

PRETTY_PRINT([configure], [Initializing Libtools.], [1])

LT_PREREQ([2.4])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
LTDL_INIT([convenience])

PRETTY_PRINT([configure], [Checks for Options.], [1])

AC_MSG_CHECKING(for configuration prefix)
 case "$prefix" in
  NONE) prefix=`pwd -P | sed 's:/[[^/]]*$::'`/game;;
 esac
AC_MSG_RESULT($prefix)

AH_TOP([#ifndef __AUTOCONF_H
#define __AUTOCONF_H

#include "copyright.h"])

AH_BOTTOM([#include "autoinclude.h"

#endif /* __AUTOCONF_H */])

dbm_gdbm=false
dbm_qdbm=false

AC_MSG_CHECKING(dbm engine to use)
AC_ARG_WITH(dbm,
  [  --with-dbm=TYPE         Select the DBM driver used to save game data:
                            gdbm : Use TinyMUSH-GDBM hash database driver.
                                   This is the historical driver used by
                                   most games. No it is not bogus. 
                            qdbm : Use TinyMUSH-QDBM hash database driver.
                                   A newer implementation of the DBM
                                   engine, a compromise between gdbm and
                                   memory-based cache.
                          @<:@default=gdbm@:>@],
  [
    case "$withval" in
      gdbm) dbm_gdbm=true
            AC_MSG_RESULT([TinyMUSH-GDBM]);;
      qdbm) dbm_qdbm=true
            AC_MSG_RESULT([TinyMUSH-QDBM]);;
      *)    dbm_gdbm=true
            AC_MSG_RESULT([TinyMUSH-GDBM]);;
    esac
  ], dbm_gdbm=true
     AC_MSG_RESULT([TinyMUSH-GDBM]))

if [test x$dbm_gdbm = xtrue]; then
 AC_DEFINE([MUSH_DBM_GDBM], [], [TinyMUSH-GDBM engine used])
else 
 if [test x$dbm_qdbm = xtrue]; then
  AC_DEFINE([MUSH_DBM_QDBM], [], [TinyMUSH-QDBM engine used])
 fi
fi

AM_CONDITIONAL([WANT_DBM_GDBM], [test x$dbm_gdbm = xtrue])
AM_CONDITIONAL([WANT_DBM_QDBM], [test x$dbm_qdbm = xtrue])

disk_based=true

AC_MSG_CHECKING(whether to enable disk-based database)
AC_ARG_ENABLE(disk-based,
  [  --enable-disk-based     store the database on disk using a DBM hashfile,
                          and cache some attributes in a limited amount of
                          memory. The alternative is memory-based, where the
                          database is kept in memory. Disk-based uses less
                          memory. Disabling the disk-based cache can improve
                          performance if enough memory is available.
                          @<:@default=yesyes@:>@],
  [
    if test "x$enableval" = "xno"; then
      AC_DEFINE([MEMORY_BASED], [], [Enable memory-based DBM hashDB])
      disk_based=false
      AC_MSG_RESULT(no)
    else
      AC_MSG_RESULT(yes)
    fi
  ], [
    AC_MSG_RESULT(yes)
  ])

AM_CONDITIONAL([WANT_DISK_BASED], [test x$disk_based = xtrue])

mod_comsys=false
AC_MSG_CHECKING(for module comsys)
AC_ARG_WITH(module-comsys,
 [  --with-module-comsys    Enable module comsys. @<:@default=yes@:>@],
 [
  case "$withval" in
   no) AC_MSG_RESULT(no);;
    *) if test -d "modules/comsys"; then
        mod_comsys=true
        AC_MSG_RESULT(yes)
        package_config="$package_config --with-module-comsys=yes"
       else
        AC_MSG_ERROR(not found)
       fi;;
  esac
 ], [
  if test -d "modules/comsys"; then
   mod_comsys=true
   AC_MSG_RESULT(yes)
   package_config="$package_config --with-module-comsys=yes"
  else
   AC_MSG_ERROR(not found)
  fi
  ])
AM_CONDITIONAL([WANT_MODULE_COMSYS], [test x$mod_comsys = xtrue])

mod_mail=false
AC_MSG_CHECKING(for module mail)
AC_ARG_WITH(module-mail,
 [  --with-module-mail      Enable module mail. @<:@default=yes@:>@],
 [
  case "$withval" in
   no) AC_MSG_RESULT(no);;
    *) if test -d "modules/mail"; then
        mod_mail=true
        AC_MSG_RESULT(yes)
        package_config="$package_config --with-module-mail=yes"
       else
        AC_MSG_ERROR(not found)
       fi;;
  esac
 ], [
  if test -d "modules/comsys"; then
   mod_mail=true
   AC_MSG_RESULT(yes)
   package_config="$package_config --with-module-mail=yes"
  else
   AC_MSG_ERROR(not found)
  fi
 ])
AM_CONDITIONAL([WANT_MODULE_MAIL], [test x$mod_mail = xtrue])

mod_hello=false
AC_MSG_CHECKING(for module hello)
AC_ARG_WITH(module-hello,
 [  --with-module-hello     Enable module hello. @<:@default=no@:>@],
 [
  case "$withval" in
   no) AC_MSG_RESULT(no);;
    *) if test -d "modules/hello"; then
        mod_hello=true
        AC_MSG_RESULT(yes)
        package_config="$package_config --with-module-hello=yes"
       else
        AC_MSG_ERROR(not found)
       fi;;
  esac
 ], [
  AC_MSG_RESULT(no)
 ])
AM_CONDITIONAL([WANT_MODULE_HELLO], [test x$mod_hello = xtrue])

dnl XXX Modules below this line need to be checked XXX

AC_MSG_CHECKING(whether to build with SQL support)
AC_ARG_WITH(sql,
  [  --with-sql=TYPE         build with support for SQL database connection
                    (TYPE can be msql, mysql, pgsql, or sqlite3) [default=no]],
  [
    case "$withval" in
      msql)    SQL_O="db_sql_msql.o"
               AC_MSG_RESULT(msql) ;;
      mysql)   SQL_O="db_sql_mysql.o"
               AC_MSG_RESULT(mysql) ;;
      pgsql)   SQL_O="db_sql_pgsql.o"
               AC_MSG_RESULT(pgsql) ;;
      sqlite3) SQL_O="db_sql_sqlite3.o"
               AC_MSG_RESULT(sqlite3) ;;
      *)       SQL_O="db_sql_empty.o"
               AC_MSG_RESULT(no) ;;
    esac
  ], SQL_O="db_sql_empty.o"
     AC_MSG_RESULT(no))
AC_SUBST(SQL_O)

AC_MSG_CHECKING(for location of SQL header files)
AC_ARG_WITH(sql-inc,
  [  --with-sql-inc=PATH     set the include path for SQL header files
                          [default for msql=/usr/local/Hughes/include]
                          [default for mysql=/usr/local/include/mysql]
                          [default for pgsql=/usr/local/pgsql/include]
                          [default for sqlite3=/usr/include]],
  [
    case "$with_sql" in
      msql | mysql | pgsql | sqlite3) SQL_I="-I$withval"
                      AC_MSG_RESULT($withval) ;;
      *) AC_MSG_RESULT(none) ;;
    esac
  ], [
    case "$with_sql" in
      msql) SQL_I="-I/usr/local/Hughes/include"
              AC_MSG_RESULT(/usr/local/Hughes/include) ;;
      mysql) SQL_I="-I/usr/local/include/mysql"
               AC_MSG_RESULT(/usr/local/include/mysql) ;;
      pgsql) SQL_I="-I/usr/local/pgsql/include"
               AC_MSG_RESULT(/usr/local/pgsql/include) ;;
      sqlite3) SQL_I="-I/usr/include"
               AC_MSG_RESULT(/usr/include) ;;
      *) AC_MSG_RESULT(none) ;;
    esac
  ])
AC_SUBST(SQL_I)

AC_MSG_CHECKING(for location of SQL library files)
AC_ARG_WITH(sql-libs,
  [  --with-sql-libs=PATH    set the library path for SQL library files
                          [default for msql=/usr/local/Hughes/lib]
                          [default for mysql=/usr/local/lib/mysql]
                          [default for pgsql=/usr/local/pgsql/lib]
                          [default for sqlite3=/usr/lib]],
  [
    case "$with_sql" in
      msql | mysql | pgsql | sqlite3) SQL_LIB="-L$withval"
                      AC_MSG_RESULT($withval) ;;
      *) AC_MSG_RESULT(none) ;;
    esac
  ], [
    case "$with_sql" in
      msql) SQL_LIB="-L/usr/local/Hughes/lib"
              AC_MSG_RESULT(/usr/local/Hughes/lib) ;;
      mysql) SQL_LIB="-L/usr/local/lib/mysql"
               AC_MSG_RESULT(/usr/local/lib/mysql) ;;
      pgsql) SQL_LIB="-L/usr/local/pgsql/lib"
               AC_MSG_RESULT(/usr/local/pgsql/lib) ;;
      sqlite3) SQL_LIB="-L/usr/lib"
               AC_MSG_RESULT(/usr/lib) ;;
      *) AC_MSG_RESULT(none) ;;
    esac
  ])

AC_MSG_CHECKING(whether to build with DEBUG support)
AC_ARG_WITH(debug,
  [  --with-debug=FLAGS      set debug flags for the compiler. After we
                          finish beta testing, we'll take away the debugging
                          -g flag. If you don't use GCC, don't use -g. Add
                          -pg for profiling (gprof netmush gmon.out)
                          [default=-g]],
  [
	if test -z "$withval"; then
	  AC_MSG_RESULT(none)
	else
          OPTIM_F=$withval
	  AC_MSG_RESULT($OPTIM_F)
	fi

  ],
  [
	OPTIM_F="-g"
	AC_MSG_RESULT($OPTIM_F)
  ]
)
AC_SUBST(OPTIM_F)

AC_MSG_CHECKING(for which version of malloc to use)
AC_ARG_WITH(malloc,
  [  --with-malloc=TYPE      build with a particular malloc package.
                          TYPE can be sys (use your system libraries),
                          gnu (use our copy of GNU malloc), or trace (use
                          our GNU malloc with mtrace turned on). If your
                          system malloc library is broken for some reason,
                          try gnu. Don't use trace unless you're debugging
                          a memory leak or corruption problem and you're
                          willing to put up with the performance hit. Try
                          using Purify (make netmush-pure) if you have it.
                          [default=sys]],
  [
     case "$withval" in
       gnu)
         MALLOC_S="malloc.c"
         MALLOC_O="malloc.o"
         AC_MSG_RESULT(gnu)
         ;;
       trace)
         MALLOC_S="malloc.c mcheck.c mstats.c mtrace.c"
         MALLOC_O="malloc.o mcheck.o mstats.o mtrace.o"
         AC_DEFINE(MCHECK, [], [--- ADD DESCRIPTION ---])
         AC_DEFINE(MTRACE_FILE, "mtrace.out", [--- ADD DESCRIPTION ---])
         AC_MSG_RESULT(trace)
         ;;
      *)
         AC_MSG_RESULT(sys)
         ;;
     esac 
  ], AC_MSG_RESULT(sys))
AC_SUBST(MALLOC_S)
AC_SUBST(MALLOC_O)

AC_MSG_CHECKING(whether to build with Pueblo support)
AC_ARG_ENABLE(pueblo,
  [  --enable-pueblo         build with support for Pueblo client
                          [default=yes]],
  [
    if test "x$enableval" = "xno"; then
      AC_MSG_RESULT(no)
    else
      AC_DEFINE(PUEBLO_SUPPORT, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(yes)
    fi
  ], AC_DEFINE(PUEBLO_SUPPORT, [], [--- ADD DESCRIPTION ---])
       AC_MSG_RESULT(yes))

AC_MSG_CHECKING(whether to use floating-point math)
AC_ARG_ENABLE(fpmath,
  [  --enable-fpmath         build math functions (lt, gt, add, etc.) with
                          IEEE 754 floating-point support instead of allowing
                          only integer calculations [default=yes]],
  [
    if test "x$enableval" = "xno"; then
      AC_MSG_RESULT(no)
    else
      AC_DEFINE(FLOATING_POINTS, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(yes)
    fi
  ], AC_DEFINE(FLOATING_POINTS, [], [--- ADD DESCRIPTION ---])
       AC_MSG_RESULT(yes))

AC_MSG_CHECKING(whether to enable command lag detection)
AC_ARG_ENABLE(lag-checking,
  [  --enable-lag-checking   enable warnings that a certain command has
                          exceeded a certain amount of wall-clock time to
                          execute. Disable this if you have severe problems
                          with computational resources on your host machine.
                          [default=yes]],
  [
    if test "x$enableval" = "xno"; then
      AC_DEFINE(NO_LAG_CHECK, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(no)
    else
      AC_MSG_RESULT(yes)
    fi
  ], AC_MSG_RESULT(yes))

AC_MSG_CHECKING(whether to enable per-object time usage tracking)
AC_ARG_ENABLE(time-checking,
  [  --enable-time-checking  enable tracking of the amount of wall-clock time
                          that individual objects are using. Disable this
                          if you have severe problems with computational
                          resources on your host machine. (This is assumed to
                          be disabled if lag-checking is disabled)
                          [default=yes]],
  [
    if test "x$enableval" = "xno"; then
      AC_DEFINE(NO_TIMECHECKING, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(no)
    else
      AC_MSG_RESULT(yes)
    fi
  ], AC_MSG_RESULT(yes))

AC_MSG_CHECKING(whether to enable cpu time checking)
AC_ARG_ENABLE(cpu-checking,
  [  --enable-cpu-checking   for lag-checking and time-checking features,
                          use CPU time reported with getrusage() instead
                          of tracking wall-clock time. This is more accurate
                          but also more computationally expensive
                          [default=no]],
  [
    if test "x$enableval" = "xno"; then
      AC_MSG_RESULT(no)
    else
      AC_DEFINE(TRACK_USER_TIME, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(yes)
    fi
  ], AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether to enable malloc-wrapper macros)
AC_ARG_ENABLE(malloc-wrapper,
  [  --enable-malloc-wrapper=TYPE
                          use wrapper macros for malloc(), free(), and
                          related functions to do one of the following,
                          which can help identify memory leaks:
                          track - maintain a tagged list of memory
                                  allocated for purposes other than the
                                  buffer pool (see @list raw_memory)
                          log   - print log messages when the functions
                                  are called. This is spammy; try other
                                  debugging tools first
                          [default=no]],
  [
    case "$enableval" in
    trac*)
      AC_DEFINE(RAW_MEMTRACKING, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(tracking) ;;
    log*)
      AC_DEFINE(TEST_MALLOC, [], [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(logging) ;;
    *)
      AC_MSG_RESULT(no) ;;
    esac
  ], AC_MSG_RESULT(no))

AC_MSG_CHECKING(how many global registers to use)
AC_ARG_WITH(global-regs,
  [  --with-global-regs=N    use N global registers (%q0-%q9,%qa-%qz)
                          N must be a number from 10 to 36. Set lower
                          to conserve memory if you don't need all the
                          possible registers [default=36]],
  [
    if test "$withval" -lt "10"; then
      AC_DEFINE(MAX_GLOBAL_REGS, 10, [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(10)
    elif test "$withval" -gt "36"; then
      AC_DEFINE(MAX_GLOBAL_REGS, 36, [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT(36)
    else
      AC_DEFINE_UNQUOTED(MAX_GLOBAL_REGS, $withval, [--- ADD DESCRIPTION ---])
        AC_MSG_RESULT($withval)
    fi
  ], AC_DEFINE(MAX_GLOBAL_REGS, 36, [--- ADD DESCRIPTION ---])
       AC_MSG_RESULT(36))

PRETTY_PRINT([configure], [Checks for programs.], [1])
# Checks for programs.
AC_PROG_CC
AC_PROG_CPP_WERROR
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_INSTALL

PRETTY_PRINT([configure], [Checks for platform.], [1])
# Checks for platform.
dnl AC_ISC_POSIX
dnl AC_MINIX
dnl AC_AIX

dnl Figure out preprocessor args to do 'make depend' stuff
dnl MKDEP=""
dnl AC_MSG_CHECKING(for preprocessor flag to list dependencies)
dnl SAVECPPFLAGS="$CPPFLAGS"
dnl CPPFLAGS="$SAVECPPFLAGS -xM1"
dnl AC_TRY_CPP(,
dnl   AC_MSG_RESULT(-xM1)
dnl     MKDEP="$CPP -xM1",
dnl   CPPFLAGS="$SAVECPPFLAGS -MM"
dnl   AC_TRY_CPP(,
dnl     AC_MSG_RESULT(-MM)
dnl       MKDEP="$CPP -MM",
dnl     CPPFLAGS="$SAVECPPFLAGS -M"
dnl     AC_TRY_CPP(,
dnl       AC_MSG_RESULT(-M)
dnl         MKDEP="CPP -M",
dnl       AC_MSG_RESULT(unknown))))
dnl if test -z "$MKDEP"; then
dnl   AC_MSG_CHECKING(for compiler flag to list dependencies)
dnl   CPPFLAGS="$SAVECPPFLAGS -MM"
dnl   AC_TRY_COMPILE(,,
dnl     AC_MSG_RESULT(-MM)
dnl       MKDEP="$CC -MM",
dnl     CPPFLAGS="$SAVECPPFLAGS -M"
dnl     AC_TRY_COMPILE(,,
dnl       AC_MSG_RESULT(-M)
dnl         MKDEP="$CC -M",
dnl       AC_MSG_RESULT(unknown)))
dnl fi
dnl CPPFLAGS="$SAVECPPFLAGS"
dnl if test -z "$MKDEP"; then
dnl   AC_PATH_PROG(MKDEP, mkdep, :)
dnl   MKDEP="$MKDEP -f -"
dnl fi
dnl AC_SUBST(MKDEP)

PRETTY_PRINT([configure], [Checks for libraries.], [1])
# Checks for libraries.

dnl case "$with_sql" in
dnl   msql)
dnl     LDFLAGS="$SQL_LIB $LDFLAGS"
dnl     AC_CHECK_LIB(msql, main,
dnl       LIBS="$SQL_LIB -lmsql $LIBS",
dnl       AC_MSG_ERROR(unable to build with msql), ) ;;
dnl   mysql)
dnl     LDFLAGS="$SQL_LIB $LDFLAGS"
dnl     AC_CHECK_LIB(z, compress)
dnl     AC_CHECK_LIB(mysqlclient, mysql_real_connect,
dnl       LIBS="$SQL_LIB -lmysqlclient $LIBS",
dnl       AC_MSG_ERROR(unable to build with mysql), ) ;;
dnl   pgsql)
dnl     LDFLAGS="$SQL_LIB $LDFLAGS"
dnl     AC_CHECK_LIB(pq, PQconnectdb,
dnl       LIBS="$SQL_LIB -lpq $LIBS",
dnl       AC_MSG_ERROR(unable to build with pgsql), ) ;;
dnl   sqlite3)
dnl     LDFLAGS="$SQL_LIB $LDFLAGS"
dnl     AC_CHECK_LIB(sqlite3, sqlite3_open,
dnl       LIBS="$SQL_LIB -lsqlite3 $LIBS",
dnl       AC_MSG_ERROR(unable to build with sqlite3), ) ;;
dnl esac

dnl How to build GDBM if needed.
dnl AC_DEFUN([BUILD_GDBM], [
dnl
dnl  PRETTY_PRINT([configure], [TinyGDBM library not found, building it.], 1)
dnl  
dnl   cd gdbm-1.8.3/
dnl   make distclean
dnl  ./configure --disable-shared --enable-static
dnl  make
dnl  cd ..
dnl
dnl  PRETTY_PRINT([configure], [TinyGDBM build done], 1)
dnl
dnl  LIBS="-L./gdbm-1.8.3/.libs -ltinygdbm $LIBS"
dnl  LIBSINC="-I./gdbm-1.8.3"
dnl
dnl ])

dnl INCDBM="@srcdir@/dbm/gdbm"

dnl LDFLAGS="-L./dbm/gdbm/.libs $LDFLAGS"
dnl LIBS="-L./dbm/gdbm/.libs -llibtinygdbm $LIBS"
dnl  AC_CHECK_LIB(tinygdbm, main, LIBS="-L./gdbm-1.8.3/.libs -ltinygdbm $LIBS", LIBSINC="-I./gdbm-1.8.3" [BUILD_GDBM], )

AC_CHECK_LIB(crypt, main)
AC_CHECK_LIB(resolv, main)
AC_CHECK_LIB(socket, main)
AC_CHECK_LIB(nsl, main)
AC_CHECK_LIB(inet, gethostbyaddr)
AC_CHECK_LIB(m, main)
dnl AC_CHECK_LIB(ltdl, lt_dlopen, LIBS="-lltdl $LIBS", AC_MSG_ERROR(unable to build with ltdl), )

PRETTY_PRINT([configure], [Checks for header files.], [1])
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
    
PRETTY_PRINT([configure], [Checks for typedefs, structures, and compiler characteristics.], [1])
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

PRETTY_PRINT([configure], [Check for Declarations.], [1])
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

PRETTY_PRINT([configure], [Checks for library functions.], [1])
# Checks for library functions.
AC_FUNC_FORK
dnl AC_FUNC_MALLOC
dnl AC_FUNC_REALLOC
AC_FUNC_VFORK
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm dup2 floor getdtablesize gethostbyaddr gethostbyname getpagesize getrusage gettimeofday inet_ntoa isascii memmove memset mktime pow select setrlimit socket sqrt srandom strcasecmp strchr strcspn strdup strerror strpbrk strrchr strspn strstr strtok_r strtoul timelocal tzset vsnprintf wait3 wait4])

PRETTY_PRINT([configure], [Writing configuration.], [1])
# Write results.
AC_DEFINE_UNQUOTED([PACKAGE_CONFIG], ["$package_config"], [Switches used with ./configure])
AC_DEFINE_UNQUOTED([PACKAGE_RELEASE_DATE], ["$package_release_date"], [Date of Release])
AC_CONFIG_SUBDIRS([dbm/gdbm dbm/qdbm])
AC_CONFIG_FILES([Makefile modules/comsys/Makefile modules/mail/Makefile modules/hello/Makefile])
AC_OUTPUT
