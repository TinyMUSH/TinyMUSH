#include "../../copyright.h"
#include "../../api.h"
#include "db_sql.h"

DO_CMD_NO_ARG(mod_db_sql_do_init) {
	notify(player, "#-1 : @SQLINIT no database driver.");
}

DO_CMD_NO_ARG(mod_db_sql_do_connect) {
	notify(player, "#-1 : @SQLCONNECT no database driver.");
}

DO_CMD_NO_ARG(mod_db_sql_do_disconnect) {
	notify(player, "#-1 : @SQLDISCONNECT no database driver.");
}

DO_CMD_ONE_ARG(mod_db_sql_do_query) {

	if (arg1 && *arg1) {
		notify(player, tprintf("#-1 : @SQL no database driver \"%s\"", arg1));
	} else {
		notify(player, "#-1 : @SQL no database driver.");
	}
}

FUNCTION(mod_db_sql_fun_init) {
	safe_str("#-1 : SQL_INIT (No Database Driver)", buff, bufc);
}

FUNCTION(mod_db_sql_fun_connect) {
	safe_str("#-1 : SQL_CONNECT (No Database Driver)", buff, bufc);
}

FUNCTION(mod_db_sql_fun_shutdown) {
	safe_str((char *)"#-1 : SQL_SHUTDOWN (No Database Driver)", buff, bufc);
}

FUNCTION(mod_db_sql_fun_query) {
	safe_str((char *)"#-1 : SQL_QUERY (No Database Driver)", buff, bufc);
}