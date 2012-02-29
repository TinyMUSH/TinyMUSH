/* db_empty.c - Placeholder code if not using an external SQL database. */
/* $Id: db_empty.c,v 1.7 2003/02/24 18:05:22 rmg Exp $ */

#include "../../api.h"

/* See db_sql.h for details of what each of these functions do. */

struct mod_sql_none_confstorage {
    char        *sql_host;      /* IP address of SQL database */
    char        *sql_db;        /* Database to use */
    char        *sql_username;  /* Username for database */
    char        *sql_password;  /* Password for database */
int sql_reconnect;  /* Auto-reconnect if connection dropped? */
int sql_socket;     /* Socket fd for SQL database connection */

} mod_sql_none_config;


CONF mod_sql_none_conftable[] = {
    {(char *)"sql_database",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_db,		MBUF_SIZE},
    {(char *)"sql_host",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_host,	MBUF_SIZE},
    {(char *)"sql_username",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_username,	MBUF_SIZE},
    {(char *)"sql_password",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mudconf.sql_password,	MBUF_SIZE},
    {(char *)"sql_reconnect",	cf_bool,	CA_GOD,		CA_WIZARD,	&mudconf.sql_reconnect,		(long)"SQL queries re-initiate dropped connections"},
    {NULL,			NULL,		0,              0,		NULL,				0}
};


void mod_sql_none_init()
{
  /* Initialize local configuration to default values. */

  //mod_<name>_config.<bool_param> = <0 or 1>;
  //mod_<name>_config.<int_param> = <integer>;
  //mod_<name>_config.<string_param> = XSTRDUP(<string>, "mod_<name>_init");

    mudconf.sql_host = XSTRDUP("127.0.0.1", "cf_string");
    mudconf.sql_db = XSTRDUP("", "cf_string");
    mudconf.sql_username = XSTRDUP("", "cf_string");
    mudconf.sql_password = XSTRDUP("", "cf_string");
    mudconf.sql_reconnect = 0;
    
    mudstate.sql_socket = -1; 

  /* Load tables. */

  register_hashtables(mod_<name>_hashtable, mod_<name>_nhashtable);
  register_commands(mod_<name>_cmdtable);
  register_prefix_cmds("<prefix chars>");
  register_functions(mod_<name>_functable);

  /* Any other initialization you need to do. */
}

/*
		//used to be in shovechars(port)
                if ((mudstate.sql_socket != -1) &&
                        (fstat(mudstate.sql_socket,
                               &fstatbuf) < 0))
                {
                     // Just mark it dead.
                    STARTLOG(LOG_PROBLEMS, "ERR", "EBADF")
                    log_printf("Bad SQL descriptor %d",
                               mudstate.sql_socket);
                    ENDLOG mudstate.sql_socket = -1;
                }

	// used to be in struct sockaddr_in *a;
	    if (s == mudstate.sql_socket)
    {
        
         // Whoa. We shouldn't be allocating this. If we got this
         // descriptor, our connection with the slave must have died
         // somehow. We make sure to take note appropriately.
         
        STARTLOG(LOG_ALWAYS, "ERR", "SOCK")
        log_printf
        ("Player descriptor clashes with SQL server fd %d",
         mudstate.sql_socket);
        ENDLOG mudstate.sql_socket = -1;
    }

	//used to be in the access_nametab
	{(char *)"sql", 2, CA_GOD, CA_SQL_OK},

	CMD_NO_ARG(do_sql_connect);     // Create a SQL db connection 
	CMD_ONE_ARG(do_sql);            // Execute a SQL command

	XFUNCTION(fun_sql);

*/

CMDENT mod_sql_none_cmdtable[] = {
{
{(char *)"@sql",		NULL,	CA_SQL_OK,	0,	CS_ONE_ARG,	NULL,	NULL,	NULL,	{do_sql}},
{(char *)"@sqlconnect",		NULL,	CA_WIZARD,	0,	CS_NO_ARGS,	NULL,	NULL,	NULL,	{do_sql_connect}},
{(char *)"@sqldisconnect",	NULL,	CA_WIZARD,	0,	CS_NO_ARGS,	NULL,	NULL,	NULL,	{sql_shutdown}},
{(char *)NULL,			NULL,	0,		0,	0,		NULL,	NULL,	NULL,	{NULL}}
};


FUN mod_<name>_functable[] = {
{"sql_init",	mod_sql_none_sql_init,		0,	0,		CA_WIZARD|CA_GOD},
{"sql_query",	mod_sql_none_fun_sql,		0,	FN_VARARGS,	CA_PUBLIC|CA_WIZARD|CA_GOD},
{"sql_shutdown",mod_sql_none_sql_shutdown,	0, 	0,		CA_WIZARD|CA_GOD},
{"SQL", 	fun_sql,			0,	FN_VARARGS,	CA_SQL_OK,	NULL},
{NULL,          NULL,				0,	0,		0}
};

int
mod_sql_none_sql_init()
{
    return -1;
}

/*---------------------------------------------------------------------------
 * SQL stuff.
 */

/*
FUNCTION(fun_sql)
{
    Delim row_delim, field_delim;

     //  Special -- the last two arguments are output delimiters

    VaChk_Range(1, 3);

    VaChk_Sep(&row_delim, 2, DELIM_STRING | DELIM_NULL | DELIM_CRLF);
    if (nfargs < 3)
    {
        Delim_Copy(&field_delim, &row_delim);
    }
    else
    {
        VaChk_Sep(&field_delim, 3,
                  DELIM_STRING | DELIM_NULL | DELIM_CRLF);
    }
    sql_query(player, fargs[0], buff, bufc, &row_delim, &field_delim);
}  
*/ 

FUNCTION(mod_sql_none_fun_sql)
{
    Delim row_delim, field_delim;

    /*
     * Special -- the last two arguments are output delimiters
     */

    VaChk_Range(1, 3);

    VaChk_Sep(&row_delim, 2, DELIM_STRING | DELIM_NULL | DELIM_CRLF);
    if (nfargs < 3)
    {
        Delim_Copy(&field_delim, &row_delim);
    }
    else 
    {
        VaChk_Sep(&field_delim, 3,
                  DELIM_STRING | DELIM_NULL | DELIM_CRLF);
    }
    notify(player, "No external SQL database connectivity is configured.");
    if (buff)
        safe_str("#-1", buff, bufc);
    return -1;
}


/* ---------------------------------------------------------------------------
 * Do SQL Command.
 */

void
do_sql(player, cause, key, name)
dbref player, cause;

int key;

char *name;
{
    sql_query(player, name, NULL, NULL, &SPACE_DELIM, &SPACE_DELIM);
}

/* ---------------------------------------------------------------------------
 * Connect SQL database.
 */

void
do_sql_connect(player, cause, key)
dbref player, cause;

int key;
{
    if (sql_init() < 0)
    {
        notify(player, "Database connection attempt failed.");
    }
    else
    {
        notify(player, "Database connection succeeded.");
    }
}
void
mod_sql_none_sql_shutdown()
{
    mudstate.sql_socket = -1;
}
