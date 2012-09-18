ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = db_sql.la
db_sql_la_SOURCES = db_sql.c
db_sql_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -dlopen self -module -avoid-version -shared
