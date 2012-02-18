dnl Useful functions for configure

AC_DEFUN([PRETTY_PRINT], [
 if test -z "$2"; then
  ${ac_aux_dir}/shtool echo -e "$1"
 elif test -z "$3"; then
  ${ac_aux_dir}/shtool echo -e "$1: %B$2%b"
 else
  echo
  ${ac_aux_dir}/shtool echo -e "$1: %B$2%b"
  echo
 fi
])

AC_DEFUN([CONFIG_MODULES], [
 for dir in `echo modules/*`
 do 
  if test -f "$dir/Makefile.in"; then
   AC_CONFIG_FILES([$dir/Makefile])
   MOD_SUBDIR="$MOD_SUBDIR $dir"
   PRETTY_PRINT([configure:] [$dir found.])
  fi
 done 
])


AC_DEFUN([AX_SPLIT_VERSION],[
 AC_REQUIRE([AC_PROG_SED])
 awk '{split($PACKAGE_VERSION,a,"." ); for (i=1; i<=10; i++) print a[i]}'
 AC_MSG_CHECKING([Major version])
 AC_MSG_RESULT([$a\[1\]])
 AC_MSG_CHECKING([Minor version])
 AC_MSG_RESULT([$a\[2\]])
 AC_MSG_CHECKING([Status version])
 AC_MSG_RESULT([$a\[3\]])
 AC_MSG_CHECKING([Revision version])
 AC_MSG_RESULT([$a\[4\]])
])
                                        