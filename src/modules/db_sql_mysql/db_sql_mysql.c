/* db_mysql.c - Implements accessing a mySQL 3.22+ database. */
/* $Id: db_mysql.c,v 1.11 2004/09/13 14:12:54 tyrspace Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */
#include "functions.h"		/* required by code */

#include "mysql.h"
#include "errmsg.h"

/* See db_sql.h for details of what each of these functions do. */


/*
 * Number of times to retry a connection if we fail in the middle of a query.
 */

#define MYSQL_RETRY_TIMES 3

static MYSQL *mysql_struct = NULL;

void
sql_shutdown()
{
    MYSQL *mysql;

    if (!mysql_struct)
        return;
    mysql = mysql_struct;
    STARTLOG(LOG_ALWAYS, "SQL", "DISC")
    log_printf
    ("Disconnected from SQL server %s, SQL database selected: %s",
     mysql->host, mysql->db);
    ENDLOG mysql_close(mysql);
    XFREE(mysql, "mysql");
    mysql_struct = NULL;
    mudstate.sql_socket = -1;
}

int
sql_init()
{
    MYSQL *mysql, *result;

    /*
     * Make sure we have valid config options.
     */

    if (!mudconf.sql_host || !*mudconf.sql_host)
        return -1;
    if (!mudconf.sql_db || !*mudconf.sql_db)
        return -1;

    /*
     * If we are already connected, drop and retry the connection, in
     * case for some reason the server went away.
     */

    if (mysql_struct)
        sql_shutdown();

    /*
     * Try to connect to the database host. If we have specified
     * localhost, use the Unix domain socket instead.
     */
    mysql = (MYSQL *) XMALLOC(sizeof(MYSQL), "mysql");
    mysql_init(mysql);

    result = mysql_real_connect(mysql, mudconf.sql_host,
                                mudconf.sql_username, mudconf.sql_password,
                                mudconf.sql_db, 0, NULL, 0);
    if (!result)
    {
        STARTLOG(LOG_ALWAYS, "SQL", "CONN")
        log_printf("Failed connection to SQL server %s: %s",
                   mudconf.sql_host, mysql_error(mysql));
        ENDLOG XFREE(mysql, "mysql");
        return -1;
    }
    STARTLOG(LOG_ALWAYS, "SQL", "CONN")
    log_printf("Connected to SQL server %s, SQL database selected: %s",
               mysql->host, mysql->db);
    ENDLOG mysql_struct = mysql;
    mudstate.sql_socket = mysql->net.fd;
    return 1;
}

int
sql_query(player, q_string, buff, bufc, row_delim, field_delim)
dbref player;

char *q_string;

char *buff;

char **bufc;

const Delim *row_delim, *field_delim;
{
    MYSQL_RES *qres;

    MYSQL_ROW row_p;

    MYSQL *mysql;

    int num_rows, got_rows, got_fields;

    int i, j;

    int retries;

    /*
     * If we have no connection, and we don't have auto-reconnect on (or
     * we try to auto-reconnect and we fail), this is an error generating
     * a #-1. Notify the player, too, and set the return code.
     */

    mysql = mysql_struct;
    if ((!mysql) & (mudconf.sql_reconnect != 0))
    {
        /*
         * Try to reconnect.
         */
        retries = 0;
        while ((retries < MYSQL_RETRY_TIMES) && !mysql)
        {
            sleep(1);
            sql_init();
            mysql = mysql_struct;
            retries++;
        }
    }
    if (!mysql)
    {
        notify(player, "No SQL database connection.");
        if (buff)
            safe_str("#-1", buff, bufc);
        return -1;
    }
    if (!q_string || !*q_string)
        return 0;

    /*
     * Send the query.
     */

    got_rows = mysql_real_query(mysql, q_string, strlen(q_string));
    if ((got_rows) && (mysql_errno(mysql) == CR_SERVER_GONE_ERROR))
    {

        /*
         * We got this error because the server died unexpectedly and
         * it shouldn't have. Try repeatedly to reconnect before
         * giving up and failing. This induces a few seconds of lag,
         * depending on number of retries; we put in the sleep() here
         * to see if waiting a little bit helps.
         */

        STARTLOG(LOG_PROBLEMS, "SQL", "GONE")
        log_printf("Connection died to SQL server");
        ENDLOG retries = 0;
        sql_shutdown();

        while ((retries < MYSQL_RETRY_TIMES) && (!mysql))
        {
            sleep(1);
            sql_init();
            mysql = mysql_struct;
            retries++;
        }

        if (mysql)
            got_rows =
                mysql_real_query(mysql, q_string,
                                 strlen(q_string));
    }
    if (got_rows)
    {
        notify(player, mysql_error(mysql));
        if (buff)
            safe_str("#-1", buff, bufc);
        return -1;
    }
    /*
     * A number of affected rows greater than 0 means it wasn't a SELECT
     */

    num_rows = mysql_affected_rows(mysql);
    if (num_rows > 0)
    {
        notify(player, tprintf("SQL query touched %d %s.",
                               num_rows, (num_rows == 1) ? "row" : "rows"));
        return 0;
    }
    else if (num_rows == 0)
    {
        return 0;
    }
    /*
     * Check to make sure we got rows back.
     */

    qres = mysql_store_result(mysql);
    got_rows = mysql_num_rows(qres);
    if (got_rows == 0)
    {
        mysql_free_result(qres);
        return 0;
    }
    /*
     * Construct properly-delimited data.
     */

    if (buff)
    {
        for (i = 0; i < got_rows; i++)
        {
            if (i > 0)
            {
                print_sep(row_delim, buff, bufc);
            }
            row_p = mysql_fetch_row(qres);
            if (row_p)
            {
                got_fields = mysql_num_fields(qres);
                for (j = 0; j < got_fields; j++)
                {
                    if (j > 0)
                    {
                        print_sep(field_delim, buff,
                                  bufc);
                    }
                    if (row_p[j] && *row_p[j])
                        safe_str(row_p[j], buff, bufc);
                }
            }
        }
    }
    else
    {
        for (i = 0; i < got_rows; i++)
        {
            row_p = mysql_fetch_row(qres);
            if (row_p)
            {
                got_fields = mysql_num_fields(qres);
                for (j = 0; j < got_fields; j++)
                {
                    if (row_p[j] && *row_p[j])
                    {
                        notify(player,
                               tprintf
                               ("Row %d, Field %d: %s",
                                i + 1, j + 1,
                                row_p[j]));
                    }
                    else
                    {
                        notify(player,
                               tprintf
                               ("Row %d, Field %d: NULL",
                                i + 1, j + 1));
                    }
                }
            }
            else
            {
                notify(player, tprintf("Row %d: NULL", i + 1));
            }
        }
    }

    mysql_free_result(qres);
    return 0;
}
