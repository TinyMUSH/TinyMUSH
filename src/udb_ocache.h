/**
 * @file udb_ocache.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Globals for the UnterMud DB layer
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"

#ifndef __UDB_OCACHE_H
#define __UDB_OCACHE_H

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

#endif /* __UDB_OCACHE_H */
