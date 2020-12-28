/**
 * @file db_sql_mysql.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module for MySQL interface
 * @version 3.3
 * @date 2020-12-28
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 * @bug This code is currently not working. Won't be fixed until we go Beta
 * 
 */

#include <copyright.h>
#include <config.h>
#include <system.h>

#include <tinymushapi.h>

#include "db_sql.h"

/* See db_sql.h for details of what each of these functions do. */

void sql_shutdown ( dbref player, dbref cause, char *buff, char **bufc )
{
    MYSQL *mysql;

    if ( !mysql_struct ) {
        return;
    }

    mysql = mysql_struct;
    log_write ( LOG_ALWAYS, "SQL", "DISC", "Disconnected from SQL server %s, SQL database selected: %s", mysql->host, mysql->db );
    mysql_close ( mysql );
    xfree ( mysql, "mysql" );
    mysql_struct = NULL;
    mod_db_sql_config.socket = -1;
}

int sql_init ( dbref player, dbref cause, char *buff, char **bufc )
{
    MYSQL *mysql, *result;

    /*
     * Make sure we have valid config options.
     */

    if ( !mod_db_sql_config.host || !*mod_db_sql_config.host )
        return -1;

    if ( !mod_db_sql_config.db || !*mod_db_sql_config.db )
        return -1;

    /*
     * If we are already connected, drop and retry the connection, in
     * case for some reason the server went away.
     */

    if ( mysql_struct )
        sql_shutdown ( 0, 0, NULL, NULL );

    /*
     * Try to connect to the database host. If we have specified
     * localhost, use the Unix domain socket instead.
     */
    mysql = ( MYSQL * ) xmalloc ( sizeof ( MYSQL ), "mysql" );
    mysql_init ( mysql );
    result = mysql_real_connect ( mysql,        mod_db_sql_config.host, mod_db_sql_config.username, mod_db_sql_config.password, mod_db_sql_config.db, mod_db_sql_config.port,  NULL,                    0 );

    if ( !result ) {
        log_write ( LOG_ALWAYS, "SQL", "CONN", "Failed connection to SQL server %s: %s", mod_db_sql_config.host, mysql_error ( mysql ) );
        xfree ( mysql, "mysql" );
        return -1;
    }

    log_write ( LOG_ALWAYS, "SQL", "CONN", "Connected to SQL server %s, SQL database selected: %s", mysql->host, mysql->db );
    mysql_struct = mysql;
    mod_db_sql_config.socket = mysql->net.fd;
    return 1;
}

int sql_query ( dbref player, char *q_string, char *buff, char **bufc, const Delim *row_delim, const Delim *field_delim )
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

    if ( ( !mysql ) & ( mod_db_sql_config.reconnect != 0 ) ) {
        /*
         * Try to reconnect.
         */
        retries = 0;

        while ( ( retries < MYSQL_RETRY_TIMES ) && !mysql ) {
            sleep ( 1 );
            sql_init ( 0, 0, NULL, NULL );
            mysql = mysql_struct;
            retries++;
        }
    }

    if ( !mysql ) {
        notify ( player, "No SQL database connection." );

        if ( buff )
            safe_str ( "#-1", buff, bufc );

        return -1;
    }

    if ( !q_string || !*q_string )
        return 0;

    /*
     * Send the query.
     */
    got_rows = mysql_real_query ( mysql, q_string, strlen ( q_string ) );

    if ( ( got_rows ) && ( mysql_errno ( mysql ) == CR_SERVER_GONE_ERROR ) ) {
        /*
         * We got this error because the server died unexpectedly and
         * it shouldn't have. Try repeatedly to reconnect before
         * giving up and failing. This induces a few seconds of lag,
         * depending on number of retries; we put in the sleep() here
         * to see if waiting a little bit helps.
         */
        log_write ( LOG_PROBLEMS, "SQL", "GONE", "Connection died to SQL server" );
        retries = 0;
        sql_shutdown ( 0, 0, NULL, NULL );

        while ( ( retries < MYSQL_RETRY_TIMES ) && ( !mysql ) ) {
            sleep ( 1 );
            sql_init ( 0, 0, NULL, NULL );
            mysql = mysql_struct;
            retries++;
        }

        if ( mysql )
            got_rows = mysql_real_query ( mysql, q_string, strlen ( q_string ) );
    }

    if ( got_rows ) {
        notify ( player, mysql_error ( mysql ) );

        if ( buff )
            safe_str ( "#-1", buff, bufc );

        return -1;
    }

    /*
     * A number of affected rows greater than 0 means it wasn't a SELECT
     */
    num_rows = mysql_affected_rows ( mysql );

    if ( num_rows > 0 ) {
        notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "SQL query touched %d %s.", num_rows, ( num_rows == 1 ) ? "row" : "rows" );
        return 0;
    } else if ( num_rows == 0 ) {
        return 0;
    }

    /*
     * Check to make sure we got rows back.
     */
    qres = mysql_store_result ( mysql );
    got_rows = mysql_num_rows ( qres );

    if ( got_rows == 0 ) {
        mysql_free_result ( qres );
        return 0;
    }

    /*
     * Construct properly-delimited data.
     */

    if ( buff ) {
        for ( i = 0; i < got_rows; i++ ) {
            if ( i > 0 ) {
                print_sep ( row_delim, buff, bufc );
            }

            row_p = mysql_fetch_row ( qres );

            if ( row_p ) {
                got_fields = mysql_num_fields ( qres );

                for ( j = 0; j < got_fields; j++ ) {
                    if ( j > 0 ) {
                        print_sep ( field_delim, buff, bufc );
                    }

                    if ( row_p[j] && *row_p[j] )
                        safe_str ( row_p[j], buff, bufc );
                }
            }
        }
    } else {
        for ( i = 0; i < got_rows; i++ ) {
            row_p = mysql_fetch_row ( qres );

            if ( row_p ) {
                got_fields = mysql_num_fields ( qres );

                for ( j = 0; j < got_fields; j++ ) {
                    if ( row_p[j] && *row_p[j] ) {
                        notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Row %d, Field %d: %s", i + 1, j + 1, row_p[j] );
                    } else {
                        notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Row %d, Field %d: NULL", i + 1, j + 1 );
                    }
                }
            } else {
                notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Row %d: NULL", i + 1 );
            }
        }
    }

    mysql_free_result ( qres );
    return 0;
}
