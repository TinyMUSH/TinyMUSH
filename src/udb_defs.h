/**
 * @file udb_defs.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief UnterMud DB layer definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __UDB_DEFS_H
#define __UDB_DEFS_H

/** 
 * default (runtime-resettable) cache parameters 
 * 
 */
#define CACHE_SIZE 1000000 /*!< 1 million bytes */
#define CACHE_WIDTH 200	   /*!< Cache width */

/** 
 * Datatypes that we have in cache and on disk 
 * 
 */
#define DBTYPE_EMPTY 0			   /*!< This entry is empty */
#define DBTYPE_ATTRIBUTE 1		   /*!< This is an attribute */
#define DBTYPE_DBINFO 2			   /*!< Various DB paramaters */
#define DBTYPE_OBJECT 3			   /*!< Object structure */
#define DBTYPE_ATRNUM 4			   /*!< Attribute number to name map */
#define DBTYPE_MODULETYPE 5		   /*!< DBTYPE to module name map */
#define DBTYPE_RESERVED 0x0000FFFF /*!< Numbers >= are free for use by user code (modules) */
#define DBTYPE_END 0xFFFFFFFF	   /*!< Highest type */

#endif /* __UDB_DEFS_H */
