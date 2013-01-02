#include <copyright.h>
#include <config.h>
#include <system.h>

#include <tinymushapi.h>

#include "db_sql.h"

void sql_shutdown(dbref player, dbref cause, char *buff, char **bufc) {
    sqlite3 *sqlite;

    if (!sqlite3_struct)
        return;
    sqlite = sqlite3_struct;
    log_write(LOG_ALWAYS, "SQL", "DISC", "Closed SQLite3 database: %s", mod_db_sql_config.db);
    sqlite3_close(sqlite);
    sqlite3_struct = NULL;
}

int sql_init(dbref player, dbref cause, char *buff, char **bufc) {
    sqlite3 *sqlite;

    int retval;

    /*
     * Make sure we have valid config options. No need to check sql_host,
     * only the db.
     */

    if (!mod_db_sql_config.db || !*mod_db_sql_config.db)
        return -1;

    /*
     * If we are already connected, drop and retry the connection, in
     * case for some reason the server went away.
     */

    if (sqlite3_struct)
        sql_shutdown(0,0,NULL,NULL);

    retval = sqlite3_open(mod_db_sql_config.db, &sqlite);
    if (retval != SQLITE_OK)
    {
        log_write(LOG_ALWAYS, "SQL", "CONN", "Failed to open %s: %s", mod_db_sql_config.db, sqlite3_errmsg(sqlite));
        return -1;
    }
    log_write(LOG_ALWAYS, "SQL", "CONN", "Opened SQLite3 file %s", mod_db_sql_config.db);
    sqlite3_struct = sqlite;
    mod_db_sql_config.socket = -1;
    return 1;
}


int sql_query(dbref player, char *q_string, char *buff, char **bufc, const Delim *row_delim, const Delim *field_delim) {
    sqlite3 *sqlite;

    const unsigned char *col_data;

    int num_rows, got_rows, got_fields;

    int i, j;

    int retries;

    int retval;

    sqlite3_stmt *stmt;

    const char *rest;

    /*
     * If we have no connection, and we don't have auto-reconnect on (or
     * we try to auto-reconnect and we fail), this is an error generating
     * a #-1. Notify the player, too, and set the return code.
     */

    sqlite = sqlite3_struct;
    if ((!sqlite) && (mod_db_sql_config.reconnect != 0))
    {
        /*
         * Try to reconnect.
         */
        retries = 0;
        while ((retries < SQLITE_RETRY_TIMES) && !sqlite)
        {
            sleep(1);
            sql_init(0,0,NULL,NULL);
            sqlite = sqlite3_struct;
            retries++;
        }
    }
    if (!sqlite)
    {
        notify_quiet(player, "No SQL database connection.");
        if (buff)
            safe_str("#-1", buff, bufc);
        return -1;
    }
    if (!q_string || !*q_string)
        return 0;

    /*
     * Prepare the query.
     */
    retval = sqlite3_prepare_v2(sqlite, q_string, -1, &stmt, &rest);
    if (retval != SQLITE_OK)
    {
        notify_quiet(player, sqlite3_errmsg(sqlite));
        if (buff)
            safe_str("#-1", buff, bufc);
        sqlite3_finalize(stmt);
        return -1;
    }
    /*
     * Construct properly-delimited data.
     */
    if (buff)
    {
        i = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (i++ > 0)
            {
                print_sep(row_delim, buff, bufc);
            }
            got_fields = sqlite3_column_count(stmt);
            if (got_fields)
            {
                for (j = 0; j < got_fields; j++)
                {
                    col_data =
                        sqlite3_column_text(stmt, j);
                    if (j > 0)
                    {
                        print_sep(field_delim, buff,
                                  bufc);
                    }
                    if (col_data && *col_data)
                        safe_str((char *)col_data,
                                 buff, bufc);
                }
            }
        }
    }
    else
    {
        i = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (i++ > 0)
            {
                print_sep(row_delim, buff, bufc);
            }
            got_fields = sqlite3_column_count(stmt);
            if (got_fields)
            {
                for (j = 0; j < got_fields; j++)
                {
                    col_data =
                        sqlite3_column_text(stmt, j);
                    if (j > 0)
                    {
                        notify_check(player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Row %d, Field %d: %s", i, j + 1, col_data);
                    }
                    if (col_data && *col_data)
                    {
                        notify_check(player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "Row %d, Field %d: NULL", i, j + 1);
                    }
                }
            }
        }
    }

    if (i == 0)
    {
        num_rows = sqlite3_changes(sqlite);
        if (num_rows > 0)
        {
            notify_check(player, player, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, "SQL query touched %d %s.", num_rows, (num_rows == 1) ? "row" : "rows");
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}
