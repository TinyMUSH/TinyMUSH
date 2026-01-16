/**
 * @file bsd_internal.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Internal declarations and globals for BSD socket modules
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @note Shared global state for socket and I/O modules
 *
 */

#ifndef __BSD_INTERNAL_H__
#define __BSD_INTERNAL_H__

/**
 * @attention Since this is the core of the whole show, better keep these globals.
 *
 */
extern int sock;				  /*!< Game socket */
extern int ndescriptors;		  /*!< New Descriptor */
extern int maxd;				  /*!< Max Descriptors */
extern DESC *descriptor_list;	  /*!< Descriptor list */

/**
 * @brief Message queue related
 *
 */
extern key_t msgq_Key;	/*!< Message Queue Key */
extern int msgq_Id;		/*!< Message Queue ID (cached globally) */

/**
 * @brief Public API for main server loop (from bsd_main.c)
 *
 */
extern void shovechars(int port);

#endif /* __BSD_INTERNAL_H__ */
