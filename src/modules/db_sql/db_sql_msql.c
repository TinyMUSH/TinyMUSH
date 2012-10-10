#include <copyright.h>
#include <config.h>
#include <system.h>

#include <tinymushapi.h>

#include "db_sql.h"

/*
 * ------------------------------------------------------------------------
 * Do the work here.
 */


void sql_shutdown(dbref player, dbref cause, char *buff, char **bufc) {
    if (mod_db_sql_config.socket == -1)
        return;
    msqlClose(mod_db_sql_config.socket);
    mod_db_sql_config.socket = -1;
}


int sql_init(dbref player, dbref cause, char *buff, char **bufc) {
    int result;

    /*
     * Make sure we have valid config options.
     */

    if (!mod_db_sql_config.host || !*mod_db_sql_config.host)
        return -1;
    if (!mod_db_sql_config.db || !*mod_db_sql_config.db)
        return -1;

    /*
     * If we are already connected, drop and retry the connection, in
     * case for some reason the server went away.
     */

    sql_shutdown(0,0,NULL,NULL);

    /*
     * Try to connect to the database host. If we have specified
     * localhost, use the Unix domain socket instead.
     */

    if (!strcmp(mod_db_sql_config.host, (char *)"localhost"))
        result = msqlConnect(NULL);
    else
        result = msqlConnect(mod_db_sql_config.host);
    if (result == -1)
    {
        STARTLOG(LOG_ALWAYS, "SQL", "CONN")
        log_printf("Failed connection to SQL server %s: %s",
                   mod_db_sql_config.host, msqlErrMsg);
        ENDLOG return -1;
    }
    STARTLOG(LOG_ALWAYS, "SQL", "CONN")
    log_printf("Connected to SQL server %s, socket fd %d",
               mod_db_sql_config.host, result);
    ENDLOG mod_db_sql_config.socket = result;

    /*
     * Select the database we want. If we can't get it, disconnect.
     */

    if (msqlSelectDB(mod_db_sql_config.socket, mod_db_sql_config.db) == -1)
    {
        STARTLOG(LOG_ALWAYS, "SQL", "CONN")
        log_printf("Failed db select: %s", msqlErrMsg);
        ENDLOG msqlClose(mod_db_sql_config.socket);
        mod_db_sql_config.socket = -1;
        return -1;
    }
    STARTLOG(LOG_ALWAYS, "SQL", "CONN")
    log_printf("SQL database selected: %s", mod_db_sql_config.db);
    ENDLOG return (mod_db_sql_config.socket);
}

int sql_query(dbref player, char *q_string, char *buff, char **bufc, const Delim *row_delim, const Delim *field_delim) {
    m_result *qres;

    m_row row_p;

    int got_rows, got_fields;

    int i, j;

    int retries;

    /*
     * If we have no connection, and we don't have auto-reconnect on (or
     * we try to auto-reconnect and we fail), this is an error generating
     * a #-1. Notify the player, too, and set the return code.
     */

    if ((mod_db_sql_config.socket == -1) && (mod_db_sql_config.reconnect != 0))
    {
        /*
         * Try to reconnect.
         */
        retries = 0;
        while ((retries < MSQL_RETRY_TIMES) &&
                (mod_db_sql_config.socket == -1))
        {
            sleep(1);
            sql_init(0,0,NULL,NULL);
            retries++;
        }
    }
    if (mod_db_sql_config.socket == -1)
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

    got_rows = msqlQuery(mod_db_sql_config.socket, q_string);
    if ((got_rows == -1) && !strcmp(msqlErrMsg, MSQL_SERVER_GONE_ERROR))
    {

        /*
         * We got this error because the server died unexpectedly and
         * it shouldn't have. Try repeatedly to reconnect before
         * giving up and failing. This induces a few seconds of lag,
         * depending on number of retries; we put in the sleep() here
         * to see if waiting a little bit helps.
         */

        STARTLOG(LOG_PROBLEMS, "SQL", "GONE")
        log_printf("Connection died to SQL server on fd %d",
                   mod_db_sql_config.socket);
        ENDLOG retries = 0;
        mod_db_sql_config.socket = -1;

        while ((retries < MSQL_RETRY_TIMES) &&
                (mod_db_sql_config.socket == -1))
        {
            sleep(1);
            sql_init(0,0,NULL,NULL);
            retries++;
        }

        if (mod_db_sql_config.socket != -1)
            got_rows = msqlQuery(mod_db_sql_config.socket, q_string);
    }
    if (got_rows == -1)
    {
        notify(player, msqlErrMsg);
        if (buff)
            safe_str("#-1", buff, bufc);
        return -1;
    }
    /*
     * A null store means that this wasn't a SELECT
     */

    qres = msqlStoreResult();
    if (!qres)
    {
        notify(player, tmprintf("SQL query touched %d %s.",
                               got_rows, (got_rows == 1) ? "row" : "rows"));
        return 0;
    }
    /*
     * Check to make sure we got rows back.
     */

    got_rows = msqlNumRows(qres);
    if (got_rows == 0)
    {
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
            row_p = msqlFetchRow(qres);
            if (row_p)
            {
                got_fields = msqlNumFields(qres);
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
            row_p = msqlFetchRow(qres);
            if (row_p)
            {
                got_fields = msqlNumFields(qres);
                for (j = 0; j < got_fields; j++)
                {
                    if (row_p[j] && *row_p[j])
                    {
                        notify(player,
                               tmprintf
                               ("Row %d, Field %d: %s",
                                i + 1, j + 1,
                                row_p[j]));
                    }
                    else
                    {
                        notify(player,
                               tmprintf
                               ("Row %d, Field %d: NULL",
                                i + 1, j + 1));
                    }
                }
            }
            else
            {
                notify(player, tmprintf("Row %d: NULL", i + 1));
            }
        }
    }

    msqlFreeResult(qres);
    return 0;
}
