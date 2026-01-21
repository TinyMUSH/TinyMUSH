/**
 * @file conf_internal.h
 * @brief Shared internal declarations for configuration modules
 */

#pragma once

#include "typedefs.h"

/* Current interpreter callback used during cf_set dispatch */
extern int (*cf_interpreter)(int *, char *, long, dbref, char *);
