#include "../../copyright.h"
#include "../../api.h"

#ifndef __DB_SQL_H
#define __DB_SQL_H

CMD_NO_ARG(mod_db_sql_do_init);
CMD_NO_ARG(mod_db_sql_do_connect);
CMD_NO_ARG(mod_db_sql_do_disconnect);
CMD_ONE_ARG(mod_db_sql_do_query);

XFUNCTION(mod_db_sql_fun_init);
XFUNCTION(mod_db_sql_fun_connect);
XFUNCTION(mod_db_sql_fun_shutdown);
XFUNCTION(mod_db_sql_fun_query);

#endif /* __DB_SQL_H */
