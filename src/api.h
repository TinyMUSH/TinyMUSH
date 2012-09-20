/* api.h.  Generated from api.h.in by configure.  */
/* api.h - must be included by all dynamically loaded modules */

#ifndef __API_H
#define __API_H

#include "copyright.h"
#include "config.h"

#include "mushconf.h"		/* required by code */
#include "game.h"		/* required by code */
#include "interface.h"		/* required by code */
#include "command.h"		/* required by code */
#include "match.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "ansi.h"		/* required by code */
#include "functions.h"		/* required by code */


/*
 * Function prototype
 */

#define	XFUNCTION(x)	\
	extern void FDECL(x, (char *, char **, dbref, dbref, dbref, \
			      char *[], int, char *[], int))

/*
 * ------------------------------------------------------------------------
 * Command handler macros.
 */

#define DO_CMD_NO_ARG(name) \
	void name (player, cause, key) \
	dbref player, cause; \
	int key;

#define DO_CMD_ONE_ARG(name) \
	void name (player, cause, key, arg1) \
	dbref player, cause; \
	int key; \
	char *arg1;

#define DO_CMD_TWO_ARG(name) \
	void name (player, cause, key, arg1, arg2) \
	dbref player, cause; \
	int key; \
	char *arg1, *arg2;

#define DO_CMD_TWO_ARG_ARGV(name) \
	void name (player, cause, key, arg1, arg2_vector, vector_size) \
	dbref player, cause; \
	int key; \
	char *arg1; \
	char *arg2_vector[]; \
	int vector_size;

/*
 * ------------------------------------------------------------------------
 * API interface macros.
 */

#define SIZE_HACK 1

#define DB_GROW_MODULE(old_db,new_size,new_top,proto) \
{ \
    char *dg__cp; \
    proto *dg__new_db; \
    int dg__i; \
    dg__new_db = (proto *) XMALLOC((new_size + SIZE_HACK) * sizeof(proto), "db_grow.mod_db"); \
    if (!dg__new_db) { \
	STARTLOG(LOG_ALWAYS, "ALC", "DB") \
	    log_printf("Could not allocate space for %d item module struct database.", new_size); \
	ENDLOG \
	abort(); \
    } \
    if (old_db) { \
	old_db -= SIZE_HACK; \
	memcpy((char *) dg__new_db, (char *) old_db, \
	       (mudstate.db_top + SIZE_HACK) * sizeof(proto)); \
	dg__cp = (char *) old_db; \
	XFREE(dg__cp, "db_grow.mod_db"); \
    } else { \
	old_db = dg__new_db; \
        for (dg__i = 0; dg__i < SIZE_HACK; dg__i++) {   /* 'NOTHING' fill */ \
            OBJ_INIT_MODULE(dg__i);        /* define macro in module file */ \
        } \
    } \
    old_db = dg__new_db + SIZE_HACK; \
    for (dg__i = mudstate.db_top; dg__i < new_size; dg__i++) { \
        OBJ_INIT_MODULE(dg__i); \
    } \
}

/*
 * ------------------------------------------------------------------------
 * API interface functions.
 */

extern void	FDECL(register_api, (char *, char *, API_FUNCTION *));
extern void    *FDECL(request_api_function, (char *, char *));

extern void	FDECL(register_commands, (CMDENT *));
extern void	FDECL(register_prefix_cmds, (const char *));
extern void	FDECL(register_functions, (FUN *));
extern void	FDECL(register_hashtables, (MODHASHES *, MODNHASHES *));
extern unsigned int FDECL(register_dbtype, (char *));

/*
 * ------------------------------------------------------------------------
 * External necessities.
 */

extern		CF_HDCL (cf_alias);
extern		CF_HDCL (cf_bool);
extern		CF_HDCL (cf_const);
extern		CF_HDCL (cf_dbref);
extern		CF_HDCL (cf_int);
extern		CF_HDCL (cf_int_factor);
extern		CF_HDCL (cf_modify_bits);
extern		CF_HDCL (cf_ntab_access);
extern		CF_HDCL (cf_option);
extern		CF_HDCL (cf_set_flags);
extern		CF_HDCL (cf_string);

#endif				/* __API_H */
