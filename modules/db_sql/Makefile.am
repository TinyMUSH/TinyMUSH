ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = db_sql.la
db_sql_la_SOURCES = db_sql.c db_sql.h
db_sql_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -module -avoid-version -shared

if WANT_MODULE_DB_SQL_DRIVER_NONE
 db_sql_la_SOURCES +=db_sql_none.c
endif

if WANT_MODULE_DB_SQL_DRIVER_MSQL
 db_sql_la_SOURCES	+=db_sql_msql.c
 AM_CPPFLAGS		+=$(MSQL_CFLAGS)
 db_sql_la_LDFLAGS	+=$(MSQL_LIBS) 
endif

if WANT_MODULE_DB_SQL_DRIVER_MYSQL
 db_sql_la_SOURCES	+=db_sql_mysql.c
 AM_CPPFLAGS		+=$(MYSQL_CFLAGS)
 db_sql_la_LDFLAGS	+=$(MYSQL_LIBS)
endif

if WANT_MODULE_DB_SQL_DRIVER_PGSQL
 db_sql_la_SOURCES	+=db_sql_pgsql.c
 AM_CPPFLAGS		+=$(PGSQL_CFLAGS)
 db_sql_la_LDFLAGS	+=$(PGSQL_LIBS)
endif

if WANT_MODULE_DB_SQL_DRIVER_SQLITE
 db_sql_la_SOURCES	+=db_sql_sqlite3.c
 AM_CPPFLAGS		+=$(SQLITE3_CFLAGS)
 db_sql_la_LDFLAGS	+=$(SQLITE3_LDFLAGS)
endif
