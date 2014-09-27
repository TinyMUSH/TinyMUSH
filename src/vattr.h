/* vattr.h - Definitions for user-defined attributes */

#include "copyright.h"

#ifndef __VATTR_H
#define __VATTR_H

#ifndef VATTR_HASH_SIZE     /* must be a power of two */
#define VATTR_HASH_SIZE 8192
#endif

#define VNAME_SIZE  32

typedef struct user_attribute VATTR;
struct user_attribute {
    char    *name; /* Name of user attribute */
    int number;     /* Assigned attribute number */
    int flags;      /* Attribute flags */
};

extern void vattr_init ( void );
extern VATTR     *vattr_rename ( char *, char * );
extern VATTR     *vattr_find ( char * );
extern VATTR     *vattr_alloc ( char *, int );
extern VATTR     *vattr_define ( char *, int, int );
extern void vattr_delete ( char * );
extern VATTR     *vattr_first ( void );
extern VATTR     *vattr_next ( VATTR * );

#endif  /* __VATTR_H */
