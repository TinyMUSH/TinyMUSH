/**
 * @file udb.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief UDB Object definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * @copyright Copyright (c) 1990 The Regents of the University of California.
 *            You may distribute under the terms the BSD two clauses license,
 *            as specified in the COPYING file.
 */

#ifndef __UDB_H
#define __UDB_H

/** 
 * Define the number of objects we may be reading/writing to at one time 
 * 
 */
#define NUM_OBJPIPES 64

/** 
 * For MUSH, an int works great as an object ID And attributes are zero
 * terminated strings, so we heave the size out. We hand around attribute
 * identifiers in the last things.
 * 
 */
typedef struct udb_aname
{
    unsigned int object;
    unsigned int attrnum;
} UDB_ANAME;

/** 
 * In general, we want binary attributes, so we do this. 
 * 
 */
typedef struct udb_attrib
{
    int attrnum; /*!< MUSH specific identifier */
    int size;
    char *data;
} UDB_ATTRIB;

/** 
 * An object is a name, an attribute count, and a vector of attributes which
 * Attr's are stowed in a contiguous array pointed at by atrs.
 * 
 */
typedef struct udb_object
{
    unsigned int name;
    time_t counter;
    int dirty;
    int at_count;
    UDB_ATTRIB *atrs;
} UDB_OBJECT;

typedef struct udb_cache
{
    void *keydata;
    int keylen;
    void *data;
    int datalen;
    unsigned int type;
    unsigned int flags;
    struct udb_cache *nxt;
    struct udb_cache *prvfree;
    struct udb_cache *nxtfree;
} UDB_CACHE;

typedef struct udb_chain
{
    UDB_CACHE *head;
    UDB_CACHE *tail;
} UDB_CHAIN;

typedef struct udb_data
{
    void *dptr;
    int dsize;
} UDB_DATA;

/** 
 * Cache flags 
 * 
 */
#define CACHE_DIRTY 0x00000001

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

extern time_t cs_ltime; /*!<  cache start time */
extern int cs_writes;   /*!<  total writes */
extern int cs_reads;    /*!<  total reads */
extern int cs_dbreads;  /*!<  total read-throughs */
extern int cs_dbwrites; /*!<  total write-throughs */
extern int cs_dels;     /*!<  total deletes */
extern int cs_checks;   /*!<  total checks */
extern int cs_rhits;    /*!<  total reads filled from cache */
extern int cs_ahits;    /*!<  total reads filled active cache */
extern int cs_whits;    /*!<  total writes to dirty cache */
extern int cs_fails;    /*!<  attempts to grab nonexistent */
extern int cs_resets;   /*!<  total cache resets */
extern int cs_syncs;    /*!<  total cache syncs */
extern int cs_size;     /*!<  total cache size */
extern UDB_CHAIN *sys_c;    /*!<  sys_c points to all cache lists */

#endif /* __UDB_H */
