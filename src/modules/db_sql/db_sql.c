#include <copyright.h>
#include <config.h>
#include <system.h>

#include <tinymushapi.h>

#include "db_sql.h"

/* --------------------------------------------------------------------------
 * Conf table.
 */

mod_db_sql_confstorage mod_db_sql_config;

CONF	mod_db_sql_conftable[] = {
   	{(char *)"sql_database",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.db,		MBUF_SIZE},
   	{(char *)"sql_host",		cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.host,		MBUF_SIZE},
   	{(char *)"sql_username",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.username,	MBUF_SIZE},
   	{(char *)"sql_password",	cf_string,	CA_STATIC,	CA_GOD,		(int *)&mod_db_sql_config.password,	MBUF_SIZE},
   	{(char *)"sql_reconnect",	cf_bool,	CA_GOD,		CA_WIZARD,	&mod_db_sql_config.reconnect,		(long)"SQL queries re-initiate dropped connections"},
   	{(char *)"sql_port",		cf_int,		CA_STATIC,	CA_GOD,		&mod_db_sql_config.port,		(long)"SQL database port"},
   	{(char *)"sql_socket",		cf_int,		CA_STATIC,	CA_GOD,		&mod_db_sql_config.socket,		0},
   	{NULL,				NULL,		0,              0,		NULL,				0}
};

/* --------------------------------------------------------------------------
 * Commands.
 */

void mod_db_sql_do_init(dbref player, dbref cause, int key) {
        sql_init(player, cause, NULL, NULL);
}

void mod_db_sql_do_connect(dbref player, dbref cause, int key) {
        sql_init(player, cause, NULL, NULL);
}

void mod_db_sql_do_disconnect(dbref player, dbref cause, int key) {
        sql_shutdown(player, cause, NULL, NULL);
}

void mod_db_sql_do_query(dbref player, dbref cause, int key, char *arg1) {
        sql_query(player, arg1, NULL, NULL, &SPACE_DELIM, &SPACE_DELIM);
}

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

void mod_db_sql_fun_init(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs) {
        sql_init(player, cause, buff, bufc);
}

void mod_db_sql_fun_connect(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs) {
        sql_init(player, cause, buff, bufc);
}

void mod_db_sql_fun_shutdown(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs) {
        sql_init(player, cause, buff, bufc);
}

void mod_db_sql_fun_query(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs) {
	Delim row_delim, field_delim;

	/* 
	 * Special -- the last two arguments are output delimiters
	 */

	VaChk_Range(1, 3);

	VaChk_Sep(&row_delim, 2, DELIM_STRING | DELIM_NULL | DELIM_CRLF);
	if (nfargs < 3) {
		Delim_Copy(&field_delim, &row_delim);
	} else {
		VaChk_Sep(&field_delim, 3,
		DELIM_STRING | DELIM_NULL | DELIM_CRLF);
	}

	sql_query(player, fargs[0], buff, bufc, &row_delim, &field_delim);
}

FUN	mod_db_sql_functable[] = {
	{"SQL",			mod_db_sql_fun_query,	0,	FN_VARARGS,	CA_MODULE_OK,		NULL},
	{"SQLINIT",		mod_db_sql_fun_init,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"SQLCONNECT",		mod_db_sql_fun_connect,	0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{"SQLDISCONECT",	mod_db_sql_fun_shutdown,0,	0,		CA_WIZARD|CA_GOD,	NULL},
	{NULL,			NULL,			0,	0,		0,			NULL}
};

/* --------------------------------------------------------------------------
 * Initialization.
 */

void mod_db_sql_init() {

	mod_db_sql_config.host = XSTRDUP("127.0.0.1", "mod_db_sql_init");
	mod_db_sql_config.db = XSTRDUP("netmush", "mod_db_sql_init");
	mod_db_sql_config.username = XSTRDUP("netmush", "mod_db_sql_init");
	mod_db_sql_config.password = XSTRDUP("netmush", "mod_db_sql_init");
	mod_db_sql_config.reconnect = 1;
	mod_db_sql_config.port = 3306;
	
	register_commands(mod_db_sql_cmdtable);
	register_functions(mod_db_sql_functable);
}

void mod_db_sql_version(dbref player, dbref cause, int extra) {
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
#ifdef SQL_DRIVER
	notify(player, tprintf("%s (%s) using %s driver", string, PACKAGE_RELEASE_DATE, SQL_DRIVER));
#else
        notify(player, tprintf("%s (%s) using placeholder driver", string, PACKAGE_RELEASE_DATE));
#endif
}

void mod_db_sql_notify(dbref player, char *buff, char **bufc, const char *format, ...) {
    va_list ap;
    char *s;

    va_start(ap, format);
    
    s = (char *)XMALLOC(MBUF_SIZE, "mod_db_sql_notify");
    vsprintf(s, format, ap);
    
    if(buff) {
        safe_str(s, buff, bufc);
    } else if(player){
        notify(player, s);
    }
    
    XFREE(s, "mod_db_sql_notify");

    va_end(ap);
}
