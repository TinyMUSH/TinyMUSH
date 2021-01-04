/**
 * @file help.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Helpfile structures and definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"

#ifndef __HELP_H
#define __HELP_H

#define LINE_SIZE 90

#define TOPIC_NAME_LEN 30

typedef struct
{
    long pos;                       /*!< index into help file */
    int len;                        /*!< length of help entry */
    char topic[TOPIC_NAME_LEN + 1]; /*!< topic of help entry */
} help_indx;

/** 
 * Pointers to this struct is what gets stored in the help_htab's 
 * 
 */
struct help_entry
{
    int pos; /*!< Position, copied from help_indx */
    int len; /*!< Length of entry, copied from help_indx */
};

typedef struct _help_indx_list
{
    help_indx entry;
    struct _help_indx_list *next;
} help_indx_list;

#endif /* __HELP_H */
