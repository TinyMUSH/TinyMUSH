m4_include([src/libltdl/m4/argz.m4])
m4_include([src/libltdl/m4/libtool.m4])
m4_include([src/libltdl/m4/ltdl.m4])
m4_include([src/libltdl/m4/ltoptions.m4])
m4_include([src/libltdl/m4/ltsugar.m4])
m4_include([src/libltdl/m4/ltversion.m4])
m4_include([src/libltdl/m4/lt~obsolete.m4])
m4_include([src/modules/db_sql/m4/ax_check_msql.m4])
m4_include([src/modules/db_sql/m4/ax_check_mysql.m4])
m4_include([src/modules/db_sql/m4/ax_check_pgsql.m4])
m4_include([src/modules/db_sql/m4/ax_lib_sqlite3.m4])

AC_DEFUN([AX_PRINT],AS_IF([test -z $2], [${ac_aux_dir}/shtool echo -e "$1"], [${ac_aux_dir}/shtool echo -e "$1: %B$2%b"]))

AC_DEFUN([AX_VERSION], [
	m4_define([count], [0])
	m4_foreach([myvar], m4_split(TINYMUSH_VERSION, [\.]),[
		AS_CASE([count], [0], [PACKAGE_VERSION_MAJOR=myvar], [1], [PACKAGE_VERSION_MINOR=myvar], [2], [PACKAGE_RELEASE_STATUS=myvar], [3], [PACKAGE_RELEASE_REVISION=myvar])
		m4_define([count], m4_eval(count + 1))
	])
	
	AS_CASE(${PACKAGE_RELEASE_STATUS}, [0], [PACKAGE_RELEASE_NAME="Alpha ${PACKAGE_RELEASE_REVISION}"], [1], [PACKAGE_RELEASE_NAME="Beta ${PACKAGE_RELEASE_REVISION}"], [2], [PACKAGE_RELEASE_NAME="Release Candidate ${PACKAGE_RELEASE_REVISION}"], AS_IF([ test ${PACKAGE_RELEASE_REVISION} -gt 0 ], [PACKAGE_RELEASE_NAME="PatchLevel ${PACKAGE_RELEASE_REVISION}"], [PACKAGE_RELEASE_NAME="Gold"]))
])

AC_DEFUN([AX_BOX],
AS_ECHO([])
AS_BOX([$1])
AS_ECHO([])
)

AC_DEFUN([AX_PRINT_PACKAGE_TITLE], AX_PRINT([
%B${PACKAGE_NAME}%b version %B${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR} ${PACKAGE_RELEASE_NAME} (TINYMUSH_RELEASE_DATE)%b

  TINYMUSH_COPYRIGHT
  See %B${PACKAGE_URL}%b for more informations. 
  Report bugs to <%B${PACKAGE_BUGREPORT}%b>.
])
)

AC_DEFUN([AX_ENABLE_YESNO], [AS_IF([test "x$1" = "xyes"], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])])
