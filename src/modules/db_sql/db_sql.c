
#include "../../api.h"

/* --------------------------------------------------------------------------
 * Conf table.
 */

struct mod_sql_confstorage {
	char	*host;		/* IP address of SQL database */
	char	*db;		/* Database to use */
	char	*username;	/* Username for database */
	char	*password;	/* Password for database */
	int	sql_reconnect;	/* Auto-reconnect if connection dropped? */
	int	sql_socket;	/* Socket fd for SQL database connection */
}	mod_sql_config;

CONF	mod_sql_conftable[] = {
   	{(char *)"sql_database",	cf_string,	CA_STATIC,	CA_GOD,		&mod_sql_config.db,		MBUF_SIZE},
   	{(char *)"sql_host",		cf_string,	CA_STATIC,	CA_GOD,		&mod_sql_config.host,		MBUF_SIZE},
   	{(char *)"sql_username",	cf_string,	CA_STATIC,	CA_GOD,		&mod_sql_config.username,	MBUF_SIZE},
   	{(char *)"sql_password",	cf_string,	CA_STATIC,	CA_GOD,		&mod_sql_config.password,	MBUF_SIZE},
   	{(char *)"sql_reconnect",	cf_bool,	CA_GOD,		CA_WIZARD,	&mod_sql_config.reconnect,	(long)"SQL queries re-initiate dropped connections"},
   	{(char *)"sql_socket",		cf_int,		CA_STATIC,	CA_GOD,		&mod_sql_config.socket,		0}
   	{NULL,				NULL,		0,              0,		NULL,				0}
};

/* --------------------------------------------------------------------------
 * Commands.
 */

DO_CMD_NO_ARG(mod_sql_do_init)
{
	notify(player, "#-1 : @SQLINIT no database driver.");
}

DO_CMD_NO_ARG(mod_sql_do_connect)
{
	notify(player, "#-1 : @SQLCONNECT no database driver.");
}

DO_CMD_NO_ARG(mod_sql_do_connect)
{
	notify(player, "#-1 : @SQLDISCONNECT no database driver.");
}

DO_CMD_ONE_ARG(mod_sql_do_query)
{

	if (arg1 && *arg1) {
		notify(player, tprintf("#-1 : @SQL no database driver \"%s\"", arg1));
	} else {
		notify(player, "#-1 : @SQL no database driver.");
	}
}

CMDENT	mod_sql_cmdtable[] = {
	{(char *)"@sql",		NULL,	CA_SQL_OK,	0,	CS_ONE_ARGS,	NULL,	NULL,	NULL, {mod_sql_do_query}},
	{(char *)"@sqlinit",		NULL,	CA_WIZARD,	0,	CS_NO_ARG,	NULL,	NULL,	NULL, {mod_sql_do_init}},
	{(char *)"@sqlconnect",		NULL,	CA_WIZARD,	0,	CS_NO_ARG,	NULL,	NULL,	NULL, {mod_sql_do_connect}},
	{(char *)"@sqldisconnect",	NULL,	CA_WIZARD,	0,	CS_NO_ARG,	NULL,	NULL,	NULL, {mod_sql_do_disconnect}},
	{(char *)NULL,			NULL, 	0,		0, 		0,	NULL,	NULL,	NULL, {NULL}}
};

/* --------------------------------------------------------------------------
 * Functions.
 */

FUNCTION(mod_sql_fun_init)
{
	safe_str("#-1 : SQL_INIT (No Database Driver");
}

FUNCTION(mod_sql_fun_connect)
{
	safe_str("#-1 : SQL_CONNECT (No Database Driver");
}

FUNCTION(mod_sql_fun_shutdown)
{
	safe_str("#-1 : SQL_SHUTDOWN (No Database Driver");
}

FUNCTION(mod_sql_fun_query)
{
	safe_str("#-1 : SQL_QUERY (No Database Driver");
}

FUN	mod_sql_functable[] = {
	{"sql_init",	mod_sql_fun_init,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"sql_connect",	mod_sql_fun_connect,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"sql_shutdown",mod_sql_fun_shutdown,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"sql_query",	mod_sql_fun_query,	0,	FN_VARARGS,	CA_SQL_OK,		NULL},
	{NULL,		NULL,			0,	0,		0,			NULL}
};

/* --------------------------------------------------------------------------
 * Initialization.
 */

void 
mod_sql_init()
{
	/* Give our configuration some default values. */

	mod_sql_config.host = XSTRDUP("127.0.0.1", "mod_sql_init");
	mod_sql_config.db = XSTRDUP("netmush", "mod_sql_init");
	mod_sql_config.username = XSTRDUP("netmush", "mod_sql_init");
	mod_sql_config.password = XSTRDUP("netmush", "mod_sql_init");
	mod_sql_config.reconnect = 1;
	mod_sql_config.socket = 3306;

	/* Register everything we have to register. */

	register_commands(mod_sql_cmdtable);
	register_functions(mod_sql_functable);
}

