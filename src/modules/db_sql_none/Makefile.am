ACLOCAL_AMFLAGS = -I libltdl/m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = db_sql_none.la
comsys_la_SOURCES = db_sql_none.c db_sql_none.h
comsys_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -dlopen self -module -avoid-version -shared
