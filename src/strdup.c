/* strdup.c - For systems like Ultrix that don't have it. */
/* $Id: strdup.c,v 1.2 2000/05/29 22:20:56 rmg Exp $ */

#include "copyright.h"
#include "autoconf.h"

char *
strdup(const char *s)
{
    char *result;

    result = (char *)malloc(strlen(s) + 1);
    strcpy(result, s);
    return result;
}
