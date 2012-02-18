/* gdbmexists.c - Check to see if a key exists */

/* $id$ */

#include "autoconf.h"

/* this is nothing more than a wrapper around _gdbm_findkey(). the
   point? it doesn't alloate any memory. */

int
gdbm_exists(dbf, key)
	gdbm_file_info *dbf;
	datum key;
{
	char *find_data;		/* dummy */
	int hash_val;			/* dummy */

	return (_gdbm_findkey(dbf, key, &find_data, &hash_val) >= 0);
}
