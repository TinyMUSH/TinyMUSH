
#include "../../copyright.h"
#include "../../api.h"
#include "db_sql.h"

/* --------------------------------------------------------------------------
 * Conf table.
 */

struct mod_db_sql_confstorage {
	char	*host;		/* IP address of SQL database */
	char	*db;		/* Database to use */
	char	*username;	/* Username for database */
	char	*password;	/* Password for database */
	int	reconnect;	/* Auto-reconnect if connection dropped? */
	int	socket;		/* Socket fd for SQL database connection */
}	mod_db_sql_config;

CONF	mod_db_sql_conftable[] = {
   	{(char *)"sql_database",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.db,		MBUF_SIZE},
   	{(char *)"sql_host",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.host,		MBUF_SIZE},
   	{(char *)"sql_username",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.username,	MBUF_SIZE},
   	{(char *)"sql_password",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.password,	MBUF_SIZE},
   	{(char *)"sql_reconnect",	cf_bool,	CA_GOD,		CA_WIZARD,	&mod_db_sql_config.reconnect,	(long)"SQL queries re-initiate dropped connections"},
   	{(char *)"sql_socket",		cf_int,		CA_STATIC,	CA_GOD,		&mod_db_sql_config.socket,		0},
   	{NULL,				NULL,		0,              0,		NULL,				0}
};

/* --------------------------------------------------------------------------
 * Commands.
 */

/*
 * The command code is define in the driver 
 */

CMDENT	mod_db_sql_cmdtable[] = {
	{(char *)"@sql",		NULL,	CA_MODULE_OK,	0,	CS_ONE_ARG,	NULL,	NULL,	NULL, {mod_db_sql_do_query}},
	{(char *)"@sqlinit",		NULL,	CA_WIZARD,	0,	CS_NO_ARGS,	NULL,	NULL,	NULL, {mod_db_sql_do_init}},
	{(char *)"@sqlconnect",		NULL,	CA_WIZARD,	0,	CS_NO_ARGS,	NULL,	NULL,	NULL, {mod_db_sql_do_connect}},
	{(char *)"@sqldisconnect",	NULL,	CA_WIZARD,	0,	CS_NO_ARGS,	NULL,	NULL,	NULL, {mod_db_sql_do_disconnect}},
	{(char *)NULL,			NULL, 	0,		0, 		0,	NULL,	NULL,	NULL, {NULL}}
};

/* --------------------------------------------------------------------------
 * Functions.
 */
 
/* 
 * The function code is define in the driver
 */

FUN	mod_db_sql_functable[] = {
	{"SQL_INIT",	mod_db_sql_fun_init,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"SQL_CONNECT",	mod_db_sql_fun_connect,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"SQL_SHUTDOWN",mod_db_sql_fun_shutdown,0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"SQL_QUERY",	mod_db_sql_fun_query,	0,	FN_VARARGS,	CA_MODULE_OK,		NULL},
	{NULL,		NULL,			0,	0,		0,			NULL}
};

/* --------------------------------------------------------------------------
 * Initialization.
 */

void 
mod_db_sql_init()
{
	/* Give our configuration some default values. */

	mod_db_sql_config.host = XSTRDUP("127.0.0.1", "mod_db_sql_init");
	mod_db_sql_config.db = XSTRDUP("netmush", "mod_db_sql_init");
	mod_db_sql_config.username = XSTRDUP("netmush", "mod_db_sql_init");
	mod_db_sql_config.password = XSTRDUP("netmush", "mod_db_sql_init");
	mod_db_sql_config.reconnect = 1;
	mod_db_sql_config.socket = 3306;
	
	/* Register everything we have to register. */

	register_commands(mod_db_sql_cmdtable);
	register_functions(mod_db_sql_functable);
}

void mod_db_sql_version(player, cause, extra)
	dbref player, cause;
	int extra;
{
	char string[MBUF_SIZE];
	
	sprintf(string, "Module DB_SQL : version %d.%d", mudstate.version.major, mudstate.version.minor);
	switch(mudstate.version.status){
		case 0: 
			sprintf(string, "%s, Alpha %d", string, mudstate.version.revision);
                	break;
		case 1: 
			sprintf(string, "%s, Beta %d", string, mudstate.version.revision);
			break;
		case 2: 
			sprintf(string,"%s, Release Candidate %d", string, mudstate.version.revision);
			break;
		default:
			if(mudstate.version.revision > 0) {
				sprintf(string, "%s, Patch Level %d", string, mudstate.version.revision);
			} else {
				sprintf(string, "%s, Gold Release.", string);
			}
	}
	notify(player, tprintf("%s (%s)", string, PACKAGE_RELEASE_DATE));
}
