/**
 * @file db_sql_pgsql.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module for pgSQL interface
 * @version 3.3
 * @date 2020-12-28
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 * @bug This code is currently not working. Won't be fixed until we go Beta
 * 
 */

#include <config.h>
#include <system.h>

#include <tinymushapi.h>

#include "db_sql.h"

void sql_shutdown ( dbref player, dbref cause, char *buff, char **bufc )
{
    PGconn *pgsql;

    if ( !pgsql_struct )
        return;

    pgsql = pgsql_struct;
    log_write ( LOG_ALWAYS, "SQL", "DISC", "Disconnected from SQL server %s, SQL database selected: %s", PQhost ( pgsql ), PQdb ( pgsql ) );
    PQfinish ( pgsql );
    pgsql_struct = NULL;
    mod_db_sql_config.socket = -1;
}

int sql_init ( dbref player, dbref cause, char *buff, char **bufc )
{
    PGconn *pgsql;
    PGresult *result;
    char connect_string[CONNECT_STRING_SIZE];

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

    if ( pgsql_struct )
        sql_shutdown ( 0, 0, NULL, NULL );

    /*
     * Try to connect to the database host. If we have specified
     * localhost, use the Unix domain socket instead.
     */
    snprintf ( connect_string, CONNECT_STRING_SIZE,
               "host = '%s' dbname = '%s' user = '%s' password = '%s'",
               mod_db_sql_config.host, mod_db_sql_config.db, mod_db_sql_config.username,
               mod_db_sql_config.password );
    pgsql = PQconnectdb ( connect_string );

    if ( !pgsql ) {
        log_write ( LOG_ALWAYS, "SQL", "CONN", "Failed connection to SQL server %s: %s", mod_db_sql_config.host, PQerrorMessage ( pgsql ) );
        return -1;
    }

    log_write ( LOG_ALWAYS, "SQL", "CONN", "Connected to SQL server %s, SQL database selected: %s", PQhost ( pgsql ), PQdb ( pgsql ) );
    pgsql_struct = pgsql;
    mod_db_sql_config.socket = PQsocket ( pgsql );
    return 1;
}

int sql_query ( dbref player, char *q_string, char *buff, char **bufc, const Delim *row_delim, const Delim *field_delim )
{
    PGresult *pgres;
    ExecStatusType pgstat;
    PGconn *pgsql;
    char *pg_data;
    int num_rows, got_rows, got_fields;
    int i, j;
    int retries;
    /*
     * If we have no connection, and we don't have auto-reconnect on (or
     * we try to auto-reconnect and we fail), this is an error generating
     * a #-1. Notify the player, too, and set the return code.
     */
    pgsql = pgsql_struct;

    if ( ( !pgsql ) && ( mod_db_sql_config.reconnect != 0 ) ) {
        /*
         * Try to reconnect.
         */
        retries = 0;

        while ( ( retries < PGSQL_RETRY_TIMES ) && !pgsql ) {
            sleep ( 1 );
            sql_init ( 0, 0, NULL, NULL );
            pgsql = pgsql_struct;
            retries++;
        }
    }

    if ( !pgsql ) {
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
    pgres = PQexec ( pgsql, q_string );
    pgstat = PQresultStatus ( pgres );

    if ( pgstat != PGRES_COMMAND_OK && pgstat != PGRES_TUPLES_OK ) {
        notify ( player, PQresultErrorMessage ( pgres ) );

        if ( buff )
            safe_str ( "#-1", buff, bufc );

        PQclear ( pgres );
        return -1;
    }

    /*
     * A number of affected rows greater than 0 means it wasn't a SELECT
     */
    num_rows = atoi ( PQcmdTuples ( pgres ) );

    if ( num_rows > 0 ) {
        notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "SQL query touched %d %s.", num_rows, ( num_rows == 1 ) ? "row" : "rows" );
        PQclear ( pgres );
        return 0;
    }

    /*
     * Check to make sure we got rows back.
     */
    got_rows = PQntuples ( pgres );

    if ( got_rows == 0 ) {
        PQclear ( pgres );
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

            got_fields = PQnfields ( pgres );

            if ( got_fields ) {
                for ( j = 0; j < got_fields; j++ ) {
                    pg_data = PQgetvalue ( pgres, i, j );

                    if ( j > 0 ) {
                        print_sep ( field_delim, buff,
                                    bufc );
                    }

                    if ( pg_data && *pg_data )
                        safe_str ( pg_data, buff, bufc );
                }
            }
        }
    } else {
        for ( i = 0; i < got_rows; i++ ) {
            got_fields = PQnfields ( pgres );

            if ( got_fields ) {
                for ( j = 0; j < got_fields; j++ ) {
                    pg_data = PQgetvalue ( pgres, i, j );

                    if ( pg_data && *pg_data ) {
                        notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Row %d, Field %d: %s", i + 1, j + 1, pg_data );
                    } else {
                        notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Row %d, Field %d: NULL", i + 1, j + 1 );
                    }
                }
            } else {
                notify_check ( player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Row %d: NULL", i + 1 );
            }
        }
    }

    PQclear ( pgres );
    return 0;
}
