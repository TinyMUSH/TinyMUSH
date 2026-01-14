/**
 * @file conf_util.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration utility functions for dynamic library loading
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>

/**
 * @brief Wrapper around dlopen that can format filename.
 *
 * @param filename	filename to load (or construct if a format string is given)
 * @param ...		arguments for the format string
 * @return void*    Handler for the module.
 */
void *dlopen_format(const char *filename, ...)
{
    int n = 0;
    size_t size = 0;
    char *p = NULL;
    va_list ap;
    void *dlhandle = NULL;

    va_start(ap, filename);
    n = vsnprintf(p, size, filename, ap);
    va_end(ap);

    if (n < 0)
    {
        return NULL;
    }

    size = (size_t)n + 1;
    p = malloc(size);
    if (p == NULL)
    {
        return NULL;
    }

    va_start(ap, filename);
    n = vsnprintf(p, size, filename, ap);
    va_end(ap);

    if (n < 0)
    {
        free(p);
        return NULL;
    }

    dlhandle = dlopen(p, RTLD_LAZY);
    free(p);

    return dlhandle;
}

/**
 * @brief Wrapper for dlsym that format symbol string.
 *
 * @param place		Module handler
 * @param symbol	Symbol to load (or construct if a format string is given)
 * @param ...		arguments for the format string
 * @return void*	Return the address in the module handle, where the symbol is loaded
 */
void *dlsym_format(void *place, const char *symbol, ...)
{
    int n = 0;
    size_t size = 0;
    char *p = NULL;
    va_list ap;
    void *_dlsym = NULL;

    va_start(ap, symbol);
    n = vsnprintf(p, size, symbol, ap);
    va_end(ap);

    if (n < 0)
    {
        return NULL;
    }

    size = (size_t)n + 1;
    p = malloc(size);
    if (p == NULL)
    {
        return NULL;
    }

    va_start(ap, symbol);
    n = vsnprintf(p, size, symbol, ap);
    va_end(ap);

    if (n < 0)
    {
        free(p);
        return NULL;
    }

    _dlsym = dlsym(place, p);
    free(p);

    return _dlsym;
}
