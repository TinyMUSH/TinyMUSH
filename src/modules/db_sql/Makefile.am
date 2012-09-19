ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = db_sql.la
db_sql_la_SOURCES = db_sql.c db_sql.h
db_sql_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -module -avoid-version -shared

if WANT_MODULE_DB_SQL_DRIVER_NONE
 db_sql_la_SOURCES +=db_sql_none.c
endif

if WANT_MODULE_DB_SQL_DRIVER_MSQL
 db_sql_la_SOURCES +=db_sql_msql.c
endif

if WANT_MODULE_DB_SQL_DRIVER_MYSQL
 db_sql_la_SOURCES +=db_sql_mysql.c
endif

if WANT_MODULE_DB_SQL_DRIVER_PGSQL
 db_sql_la_SOURCES +=db_sql_pgsqp.c
endif

if WANT_MODULE_DB_SQL_DRIVER_SQLITE
 db_sql_la_SOURCES +=sqlite3
endif
