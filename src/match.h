/**
 * @file match.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Match-related definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __MATCH_H
#define __MATCH_H

typedef struct match_state
{
    int confidence;      /*!< How confident are we?  CON_xx */
    int count;           /*!< # of matches at this confidence */
    int pref_type;       /*!< The preferred object type */
    int check_keys;      /*!< Should we test locks? */
    dbref absolute_form; /*!< If #num, then the number */
    dbref match;         /*!< What I've found so far */
    dbref player;        /*!< Who is performing match */
    char *string;        /*!< The string to search for */
} MSTATE;

#define NOMATCH_MESSAGE "I don't see that here."
#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"
#define NOPERM_MESSAGE "Permission denied."

#define MAT_NO_EXITS 1     /*!< Don't check for exits */
#define MAT_EXIT_PARENTS 2 /*!< Check for exits in parents */
#define MAT_NUMERIC 4      /*!< Check for un-#ified dbrefs */
#define MAT_HOME 8         /*!< Check for 'home' */

#endif /* __MATCH_H */
