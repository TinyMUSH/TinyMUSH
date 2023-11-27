dnl ---------------------------------------------------------------------------
dnl Macro: AX_CHECK_MSQL
dnl Check for custom mSQL paths in --with-msql-* options.
dnl If some paths are missing,  check if msql_config exists. 
dnl ---------------------------------------------------------------------------

AC_DEFUN([AX_CHECK_MSQL],[
	# check well-known include paths
	for MSQL_INCLUDE in "/usr/include" "/usr/local/include" "/usr/local/msql3" "/usr/local/msql3/include" "/usr/local/Hughes" "/usr/local/Hughes/include"
	do
		if test [ -r "$MSQL_INCLUDE/msql.h" ]
			then
			ac_cv_msql_includes=$MSQL_INCLUDE
			
		fi
	done
	
	# check well-known library paths
	for MSQL_LIB in "/usr/lib" "/usr/local/lib" "/usr/local/msql3" "/usr/local/msql3/lib" "/usr/local/Hughes" "/usr/local/Hughes/lib"
	do
		if test [ -r "$MSQL_LIB/libmsql.a" ]
		then
			ac_cv_msql_libs=$MSQL_LIB
		fi
	done

	# check for custom user-specified mSQL root directory
	if test [ x$1 != xyes -a x$1 != xno ] 
	then
		ac_cv_msql_root=`echo $1 | sed -e 's+/$++'`
		if test [ -d "$ac_cv_msql_root/include" -a -d "$ac_cv_msql_root/lib" ]
		then
			ac_cv_msql_includes="$ac_cv_msql_root/include"
			ac_cv_msql_libs="$ac_cv_msql_root/lib"
		else 
			AC_MSG_ERROR([invalid mSQL root directory: $ac_cv_msql_root])
		fi
	fi
	
	# check for custom includes path
	if test [ -z "$ac_cv_msql_includes" ] 
	then 
		AC_ARG_WITH([msql-includes], AC_HELP_STRING([--with-msql-includes], [path to mSQL header files]), [ac_cv_msql_includes=$withval])
	fi

	if test [ -n "$ac_cv_msql_includes" ]
	then
		AC_CACHE_CHECK([mSQL includes], [ac_cv_msql_includes], [ac_cv_msql_includes=""])
		MSQL_CFLAGS="-I$ac_cv_msql_includes"
		AC_MSG_RESULT([$ac_cv_msql_includes])
	fi

	# check for custom library path
	if test [ -z "$ac_cv_msql_libs" ]
	then
		AC_ARG_WITH([msql-libs], AC_HELP_STRING([--with-msql-libs], [path to mSQL libraries]), [ac_cv_msql_libs=$withval])
	fi	

	if test [ -n "$ac_cv_msql_libs" ]
	then
		# Trim trailing '.libs' if user passed it in --with-msql-libs option
		ac_cv_msql_libs=`echo ${ac_cv_msql_libs} | sed -e 's/.libs$//' -e 's+.libs/$++'`
		AC_CACHE_CHECK([mSQL libraries], [ac_cv_msql_libs], [ac_cv_msql_libs=""])
		MSQL_LIBS="-L$ac_cv_msql_libs -lmsql"
		AC_MSG_RESULT([$ac_cv_msql_libs])
	fi

	if test [ -z "$ac_cv_msql_includes" ]
	then	
		AC_MSG_ERROR([Unable to find mSQL include path.])
	fi

	if test [ -z "$ac_cv_msql_libs" ]
	then	
		AC_MSG_ERROR([Unable to find mSQL library path.])
	fi

	AC_SUBST([MSQL_CFLAGS])
	AC_SUBST([MSQL_LIBS])
	AC_DEFINE([HAVE_MSQL], [1], [DB_SQL use the mSQL driver])
])
