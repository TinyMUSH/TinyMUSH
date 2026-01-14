/**
 * @file conf.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Public interface for configuration subsystem
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#pragma once

#include "typedefs.h"

/*
 * Global configuration and state variables
 * Defined in conf_core.c
 */
extern CONFDATA mushconf;
extern STATEDATA mushstate;

/*
 * Configuration result enumeration
 */
typedef enum {
    CF_Success = 0,   /*!< Configuration change succeeded */
    CF_Partial = 1,   /*!< Configuration change partially succeeded */
    CF_Failure = -1   /*!< Configuration change failed */
} CF_Result;

/*
 * Public interface functions from conf_core.c
 */
void cf_init(void);
void cf_log(dbref player, const char *primary, const char *secondary, char *cmd, const char *format, ...);
CF_Result cf_status_from_succfail(dbref player, char *cmd, int success, int failure);

/*
 * Public interface functions from conf_interface.c
 */
CF_Result cf_set(char *cp, char *ap, dbref player);
CF_Result cf_read(char *fn);
void do_admin(dbref player, dbref cause, int extra, char *kw, char *value);

/*
 * Public interface functions from conf_display.c
 */
void cf_verify(void);
void cf_display(dbref player, char *param_name, char *buff, char **bufc);
void list_cf_access(dbref player);
void list_cf_read_access(dbref player);
void list_options(dbref player);

/*
 * Helper configuration handler types - used internally
 */
typedef CF_Result (*CF_HANDLER)(int *, char *, long, dbref, char *);
typedef CF_Result (*CF_HANDLER_LONG)(long **, char *, long, dbref, char *);

#endif /* __CONF_H__ */
