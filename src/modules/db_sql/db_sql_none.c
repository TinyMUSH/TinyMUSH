/**
 * @file db_sql_none.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module placeholder when no SQL interface is implemented
 * @version 4.0
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

#define SQL_NONE_ERROR "NO EXTERNAL SQL DATABASE CONNECTIVITY IS CONFIGURED"

void sql_shutdown ( dbref player, dbref cause, char *buff, char **bufc )
{
    log_write ( LOG_ALWAYS, "SQL", "DISC", "Disconnected from SQL server (simulated)." );

    if ( buff ) {
        mod_db_sql_notify ( player, buff, bufc, "#-1 %s", SQL_NONE_ERROR );
    } else if ( player ) {
        mod_db_sql_notify ( player, buff, bufc, "%s", SQL_NONE_ERROR );
    }
}

int sql_init ( dbref player, dbref cause, char *buff, char **bufc )
{
    log_write ( LOG_ALWAYS, "SQL", "CONN", "Connected to SQL server (simulated)" );

    if ( buff ) {
        mod_db_sql_notify ( player, buff, bufc, "#-1 %s", SQL_NONE_ERROR );
    } else if ( player ) {
        mod_db_sql_notify ( player, buff, bufc, "%s", SQL_NONE_ERROR );
    }

    return 1;
}

int sql_query ( dbref player, char *q_string, char *buff, char **bufc, const Delim *row_delim, const Delim *field_delim )
{
    if ( buff ) {
        mod_db_sql_notify ( player, buff, bufc, "#-1 %s", SQL_NONE_ERROR );
    } else if ( player ) {
        mod_db_sql_notify ( player, buff, bufc, "%s", SQL_NONE_ERROR );
    }

    return 0;
}
