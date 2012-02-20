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
 tmp=(`echo $PACKAGE_VERSION | tr "." "\n"`)

 PACKAGE_VERSION_MAJOR=${tmp[[0]]}
 PACKAGE_VERSION_MINOR=${tmp[[1]]}
 PACKAGE_RELEASE_STATUS=${tmp[[2]]}
 PACKAGE_RELEASE_REVISION=${tmp[[3]]}
 
 case ${tmp[[2]]} in
 	0)	PACKAGE_RELEASE_NAME="Alpha ${tmp[[3]]}"
 		;;
 	1)	PACKAGE_RELEASE_NAME="Beta ${tmp[[3]]}"
 		;;
 	2)	if [[ ${tmp[3]} -gt 0 ]]; then
 			PACKAGE_RELEASE_NAME="PatchLevel ${tmp[[3]]}"
 		else
 			PACKAGE_RELEASE_NAME="Gold"
		fi
		;;
 esac
])

AC_DEFUN([AX_PRINT_PACKAGE_TITLE],[
PRETTY_PRINT([
%B${PACKAGE_NAME}%b version %B${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR} ${PACKAGE_RELEASE_NAME} (${PACKAGE_RELEASE_DATE})%b

  $PACKAGE_COPYRIGHT.
  See %B$PACKAGE_URL%b for more informations. 
  Report bugs to <%B$PACKAGE_BUGREPORT%b>.])
])

                                        