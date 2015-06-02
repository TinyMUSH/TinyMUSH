/* api.h - must be included by all dynamically loaded modules */

#include "copyright.h"

#include <ltdl.h>

#ifndef __API_H
#define __API_H

#define SIZE_HACK	1

/* XXX Need to be convert into a function in api.c */
#define DB_GROW_MODULE(old_db,new_size,new_top,proto) \
{ \
    char *dg__cp; \
    proto *dg__new_db; \
    int dg__i; \
    dg__new_db = (proto *) xmalloc((new_size + SIZE_HACK) * sizeof(proto), "db_grow.mod_db"); \
    if (!dg__new_db) { \
        log_write(LOG_ALWAYS, "ALC", "DB", "Could not allocate space for %d item module struct database.", new_size); \
    abort(); \
    } \
    if (old_db) { \
    old_db -= SIZE_HACK; \
    memcpy((char *) dg__new_db, (char *) old_db, \
           (mudstate.db_top + SIZE_HACK) * sizeof(proto)); \
    dg__cp = (char *) old_db; \
    xfree(dg__cp, "db_grow.mod_db"); \
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

extern void register_api(char *, char *, API_FUNCTION *);
extern void *request_api_function(char *, char *);

extern void register_commands(CMDENT *);
extern void register_prefix_cmds(const char *);
extern void register_functions(FUN *);
extern void register_hashtables(MODHASHES *, MODNHASHES *);
extern unsigned int register_dbtype(char *);

/*
 * ------------------------------------------------------------------------
 * External necessities.
 */

extern int cf_alias(int *, char *, long, dbref, char *);
extern int cf_bool(int *, char *, long, dbref, char *);
extern int cf_const(int *, char *, long, dbref, char *);
extern int cf_dbref(int *, char *, long, dbref, char *);
extern int cf_int(int *, char *, long, dbref, char *);
extern int cf_int_factor(int *, char *, long, dbref, char *);
extern int cf_int_modify_bits(int *, char *, long, dbref, char *);
extern int cf_int_ntab_access(int *, char *, long, dbref, char *);
extern int cf_option(int *, char *, long, dbref, char *);
extern int cf_set_flags(int *, char *, long, dbref, char *);
extern int cf_string(int *, char *, long, dbref, char *);

#endif				/* __API_H */
