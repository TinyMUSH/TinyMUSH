/**
 * @file db_sql.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module for SQL interface
 * @version 4.0
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 * @bug This code is currently not working. Won't be fixed until we go Beta
 * 
 */

#include <system.h>
#include <tinymushapi.h>

#include "db_sql.h"

/* --------------------------------------------------------------------------
 * Conf table.
 */

MODVER mod_db_sql_version;

mod_db_sql_confstorage mod_db_sql_config;

CONF mod_db_sql_conftable[] = {
    {(char *)"sql_database", cf_string, CA_STATIC, CA_GOD, (int *)&mod_db_sql_config.db, MBUF_SIZE},
    {(char *)"sql_host", cf_string, CA_STATIC, CA_GOD, (int *)&mod_db_sql_config.host, MBUF_SIZE},
    {(char *)"sql_username", cf_string, CA_STATIC, CA_GOD, (int *)&mod_db_sql_config.username, MBUF_SIZE},
    {(char *)"sql_password", cf_string, CA_STATIC, CA_GOD, (int *)&mod_db_sql_config.password, MBUF_SIZE},
    {(char *)"sql_reconnect", cf_bool, CA_GOD, CA_WIZARD, &mod_db_sql_config.reconnect, (long)"SQL queries re-initiate dropped connections"},
    {(char *)"sql_port", cf_int, CA_STATIC, CA_GOD, &mod_db_sql_config.port, (long)"SQL database port"},
    {(char *)"sql_socket", cf_int, CA_STATIC, CA_GOD, &mod_db_sql_config.socket, 0},
    {NULL, NULL, 0, 0, NULL, 0}};

/* --------------------------------------------------------------------------
 * Commands.
 */

void mod_db_sql_do_init(dbref player, dbref cause, int key)
{
    sql_init(player, cause, NULL, NULL);
}

void mod_db_sql_do_connect(dbref player, dbref cause, int key)
{
    sql_init(player, cause, NULL, NULL);
}

void mod_db_sql_do_disconnect(dbref player, dbref cause, int key)
{
    sql_shutdown(player, cause, NULL, NULL);
}

void mod_db_sql_do_query(dbref player, dbref cause, int key, char *arg1)
{
    sql_query(player, arg1, NULL, NULL, &SPACE_DELIM, &SPACE_DELIM);
}

CMDENT mod_db_sql_cmdtable[] = {
    {(char *)"@sql", NULL, CA_MODULE_OK, 0, CS_ONE_ARG, NULL, NULL, NULL, {mod_db_sql_do_query}},
    {(char *)"@sqlinit", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {mod_db_sql_do_init}},
    {(char *)"@sqlconnect", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {mod_db_sql_do_connect}},
    {(char *)"@sqldisconnect", NULL, CA_WIZARD, 0, CS_NO_ARGS, NULL, NULL, NULL, {mod_db_sql_do_disconnect}},
    {(char *)NULL, NULL, 0, 0, 0, NULL, NULL, NULL, {NULL}}};

/* --------------------------------------------------------------------------
 * Functions.
 */

void mod_db_sql_fun_init(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    sql_init(player, cause, buff, bufc);
}

void mod_db_sql_fun_connect(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    sql_init(player, cause, buff, bufc);
}

void mod_db_sql_fun_shutdown(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    sql_init(player, cause, buff, bufc);
}

void mod_db_sql_fun_query(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim row_delim, field_delim;
    /*
     * Special -- the last two arguments are output delimiters
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 3, buff, bufc)) {return;}

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &row_delim, 0x008 | 0x002 | 0x004)) {return;}

    if (nfargs < 3)
    {
        XMEMCPY(&field_delim, &row_delim, sizeof(Delim) - MAX_DELIM_LEN + 1 + (&row_delim)->len);
    }
    else
    {
        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &field_delim, 0x008 | 0x002 | 0x004))
        {
            return;
        }
    }

    sql_query(player, fargs[0], buff, bufc, &row_delim, &field_delim);
}

FUN mod_db_sql_functable[] = {
    {"SQL", mod_db_sql_fun_query, 0, FN_VARARGS, CA_MODULE_OK, NULL},
    {"SQLINIT", mod_db_sql_fun_init, 0, 0, CA_WIZARD | CA_GOD, NULL},
    {"SQLCONNECT", mod_db_sql_fun_connect, 0, 0, CA_WIZARD | CA_GOD, NULL},
    {"SQLDISCONECT", mod_db_sql_fun_shutdown, 0, 0, CA_WIZARD | CA_GOD, NULL},
    {NULL, NULL, 0, 0, 0, NULL}};

/* --------------------------------------------------------------------------
 * Initialization.
 */

void mod_db_sql_init()
{
    char *str;
    mod_db_sql_config.host = xstrdup("127.0.0.1", "mod_db_sql_init");
    mod_db_sql_config.db = xstrdup("netmush", "mod_db_sql_init");
    mod_db_sql_config.username = xstrdup("netmush", "mod_db_sql_init");
    mod_db_sql_config.password = xstrdup("netmush", "mod_db_sql_init");
    mod_db_sql_config.reconnect = 1;
    mod_db_sql_config.port = 3306;
    str = xmalloc(MBUF_SIZE, "mod_db_sql_init");
    snprintf(str, MBUF_SIZE, "version %d.%d", mushstate.version.major, mushstate.version.minor);

    switch (mushstate.version.status)
    {
    case 0:
        snprintf(str, MBUF_SIZE, "%s, Alpha %d", str, mushstate.version.revision);
        break;

    case 1:
        snprintf(str, MBUF_SIZE, "%s, Beta %d", str, mushstate.version.revision);
        break;

    case 2:
        snprintf(str, MBUF_SIZE, "%s, Release Candidate %d", str, mushstate.version.revision);
        break;

    default:
        if (mushstate.version.revision > 0)
        {
            snprintf(str, MBUF_SIZE, "%s, Patch Level %d", str, mushstate.version.revision);
        }
        else
        {
            snprintf(str, MBUF_SIZE, "%s, Gold Release.", str);
        }
    }

#ifdef SQL_DRIVER
    snprintf(str, MBUF_SIZE, "%s (%s) using %s driver", str, PACKAGE_RELEASE_DATE, SQL_DRIVER);
    mod_db_sql_version.version = xstrdup(str, "mod_db_sql_init");
#else
    snprintf(str, MBUF_SIZE, "%s (%s) using placeholder driver", str, PACKAGE_RELEASE_DATE);
    mod_db_sql_version.version = xstrdup(str, "mod_db_sql_init");
#endif
    mod_db_sql_version.author = xstrdup("TinyMUSH Development Team", "mod_db_sql_init");
    mod_db_sql_version.email = xstrdup("tinymush@googlegroups.com", "mod_db_sql_init");
    mod_db_sql_version.url = xstrdup("https://github.com/TinyMUSH", "mod_db_sql_init");
    mod_db_sql_version.description = xstrdup("SQL Database interface for TinyMUSH", "mod_db_sql_init");
    mod_db_sql_version.copyright = xstrdup("Copyright (C) 2012 TinyMUSH development team.", "mod_db_sql_init");
    xfree(str, "mod_db_sql_init");
    register_commands(mod_db_sql_cmdtable);
    register_functions(mod_db_sql_functable);
}

void mod_db_sql_notify(dbref player, char *buff, char **bufc, const char *format, ...)
{
    va_list ap;
    char *s;
    va_start(ap, format);
    s = (char *)xmalloc(MBUF_SIZE, "mod_db_sql_notify");
    vnsprintf(s, MBUF_SIZE, format, ap);

    if (buff)
    {
        safe_str(s, buff, bufc);
    }
    else if (player)
    {
        notify(player, s);
    }

    xfree(s, "mod_db_sql_notify");
    va_end(ap);
}
