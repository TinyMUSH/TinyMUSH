/* vattr.h - Definitions for user-defined attributes */

#include "copyright.h"

#ifndef __VATTR_H
#define __VATTR_H

#ifndef VATTR_HASH_SIZE /* must be a power of two */
#define VATTR_HASH_SIZE 8192
#endif

#define VNAME_SIZE 32

typedef struct user_attribute VATTR;
struct user_attribute
{
    char *name; /* Name of user attribute */
    int number; /* Assigned attribute number */
    int flags;  /* Attribute flags */
};

#endif /* __VATTR_H */
