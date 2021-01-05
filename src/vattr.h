/**
 * @file vattr.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Definitions for user-defined attributes
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __VATTR_H
#define __VATTR_H

#ifndef VATTR_HASH_SIZE /*!< must be a power of two */
#define VATTR_HASH_SIZE 8192
#endif

#define VNAME_SIZE 32

typedef struct user_attribute
{
    char *name; /*!< Name of user attribute */
    int number; /*!< Assigned attribute number */
    int flags;  /*!< Attribute flags */
} VATTR;

#endif /* __VATTR_H */
