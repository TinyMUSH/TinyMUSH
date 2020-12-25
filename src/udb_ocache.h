#include "copyright.h"

#ifndef __UDB_OCACHE_H
#define __UDB_OCACHE_H

extern time_t cs_ltime;
extern int cs_writes;	/*!<  total writes */
extern int cs_reads;	/*!<  total reads */
extern int cs_dbreads;	/*!<  total read-throughs */
extern int cs_dbwrites; /*!<  total write-throughs */
extern int cs_dels;		/*!<  total deletes */
extern int cs_checks;	/*!<  total checks */
extern int cs_rhits;	/*!<  total reads filled from cache */
extern int cs_ahits;	/*!<  total reads filled active cache */
extern int cs_whits;	/*!<  total writes to dirty cache */
extern int cs_fails;	/*!<  attempts to grab nonexistent */
extern int cs_resets;	/*!<  total cache resets */
extern int cs_syncs;	/*!<  total cache syncs */
extern int cs_size;		/*!<  total cache size */

extern Chain *sys_c;    /*!<  sys_c points to all cache lists */

#endif /* __UDB_OCACHE_H */
