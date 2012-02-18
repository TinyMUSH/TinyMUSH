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
