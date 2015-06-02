/* udb_defs.h - Header file for the UnterMud DB layer, in TinyMUSH 3.0 */

#include "copyright.h"

#ifndef __UDB_DEFS_H
#define __UDB_DEFS_H

/* If your malloc() returns void or char pointers... */
/* typedef  void    *mall_t; */
typedef char *mall_t;

/* default (runtime-resettable) cache parameters */

#define CACHE_SIZE  1000000	/* 1 million bytes */
#define CACHE_WIDTH 200

/* Datatypes that we have in cache and on disk */

#define DBTYPE_EMPTY        0	/* This entry is empty */
#define DBTYPE_ATTRIBUTE    1	/* This is an attribute */
#define DBTYPE_DBINFO       2	/* Various DB paramaters */
#define DBTYPE_OBJECT       3	/* Object structure */
#define DBTYPE_ATRNUM       4	/* Attribute number to name map */
#define DBTYPE_MODULETYPE   5	/* DBTYPE to module name map */
#define DBTYPE_RESERVED     0x0000FFFF	/* Numbers >= are free for
					 * use by user code (modules) */
#define DBTYPE_END      0xFFFFFFFF	/* Highest type */

#endif				/* __UDB_DEFS_H */
