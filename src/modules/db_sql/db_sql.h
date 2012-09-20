#ifndef __DB_SQL_H
#define __DB_SQL_H

#include "../../copyright.h"
#include "../../api.h"

#ifdef HAVE_MSQL

#define SQL_DRIVER "mSQL"

#include <msql.h>		/* required by code */

/* See db_sql.h for details of what each of these functions do. */

/*
 * Because we cannot trap error codes in mSQL, just error messages, we have
 * to compare against error messages to find out what really happened with
 * something. This particular error message is SERVER_GONE_ERROR in mSQL's
 * errmsg.h -- this needs to change if that API definition changes.
 */

#define MSQL_SERVER_GONE_ERROR "MSQL server has gone away"

/*
 * Number of times to retry a connection if we fail in the middle of a query.
 */

#define MSQL_RETRY_TIMES 3
#endif

#ifdef HAVE_MYSQL

#define SQL_DRIVER "MySQL"

#include <mysql.h>
#include <errmsg.h>

/*
 * Number of times to retry a connection if we fail in the middle of a query.
 */
#define MYSQL_RETRY_TIMES 3

static MYSQL *mysql_struct = NULL;
#endif

#ifdef HAVE_PGSQL

#define SQL_DRIVER "PostgreSQL"

#include <libpq-fe.h>

/* See db_sql.h for details of what each of these functions do. */


/*
 * Number of times to retry a connection if we fail in the middle of a query.
 */

#define PGSQL_RETRY_TIMES 3
#define CONNECT_STRING_SIZE 512

static PGconn *pgsql_struct = NULL;
#endif

#ifdef HAVE_SQLITE3

#define SQL_DRIVER "SQLite3"

#include <sqlite3.h>

/* See db_sql.h for details of what each of these functions do. */

/*
 * Number of times to retry a connection if we fail in the middle of a query.
 */

#define SQLITE_RETRY_TIMES 3

static sqlite3 *sqlite3_struct = NULL;
#endif

CMD_NO_ARG(mod_db_sql_do_init);
CMD_NO_ARG(mod_db_sql_do_connect);
CMD_NO_ARG(mod_db_sql_do_disconnect);
CMD_ONE_ARG(mod_db_sql_do_query);

XFUNCTION(mod_db_sql_fun_init);
XFUNCTION(mod_db_sql_fun_connect);
XFUNCTION(mod_db_sql_fun_shutdown);
XFUNCTION(mod_db_sql_fun_query);

void sql_shutdown(dbref, dbref, char *, char **);
int sql_init(dbref, dbref, char *, char **);
int sql_query(dbref, char *, char *, char **, const Delim *, const Delim *);

#if defined(__STDC__) && defined(STDC_HEADERS)
void mod_db_sql_notify(dbref player, char *buff, char **bufc, const char *format, ...);
#else
void mod_db_sql_notify(dbref player, char *buff, char **bufc, va_dlc va_alist);
#endif

typedef struct {
	char	*host;		/* IP address of SQL database */
	char	*db;		/* Database to use */
	char	*username;	/* Username for database */
	char	*password;	/* Password for database */
	int	reconnect;	/* Auto-reconnect if connection dropped? */
	int	port;		/* Port of SQL database */
	int	socket;		/* Socket fd for SQL database connection */
} mod_db_sql_confstorage;

extern mod_db_sql_confstorage mod_db_sql_config;

#endif /* __DB_SQL_H */
