/**
 * @file db_sql.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Public API for the SQL abstraction layer and backend drivers
 * @version 4.0
 * 
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 * @bug This code is currently not working. Won't be fixed until we go Beta
 * 
 */

#ifndef __DB_SQL_H
#define __DB_SQL_H

#ifdef HAVE_MSQL

#define SQL_DRIVER "mSQL"

#include <msql.h>       /* required by code */

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

typedef struct {
    char    *host;      /* IP address of SQL database */
    char    *db;        /* Database to use */
    char    *username;  /* Username for database */
    char    *password;  /* Password for database */
    int reconnect;  /* Auto-reconnect if connection dropped? */
    int port;       /* Port of SQL database */
    int socket;     /* Socket fd for SQL database connection */
} mod_db_sql_confstorage;

extern mod_db_sql_confstorage mod_db_sql_config;

#endif /* __DB_SQL_H */
