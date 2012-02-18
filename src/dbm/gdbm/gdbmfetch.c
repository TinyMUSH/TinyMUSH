/* gdbmfetch.c - Find a key and return the associated data.  */

/* $id$ */

#include "autoconf.h"

/* Look up a given KEY and return the information associated with that KEY.
   The pointer in the structure that is  returned is a pointer to dynamically
   allocated memory block.  */

datum
gdbm_fetch(dbf, key)
	gdbm_file_info *dbf;
	datum key;
{
	datum return_val;		/* The return value. */
	int elem_loc;			/* The location in the bucket. */
	char *find_data;		/* Returned from find_key. */
	int hash_val;			/* Returned from find_key. */

	/* Set the default return value. */
	return_val.dptr = NULL;
	return_val.dsize = 0;

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Find the key and return a pointer to the data. */
	elem_loc = _gdbm_findkey(dbf, key, &find_data, &hash_val);

	/* Copy the data if the key was found.  */
	if (elem_loc >= 0) {
		/* This is the item.  Return the associated data. */
		return_val.dsize = dbf->bucket->h_table[elem_loc].data_size;
		if (return_val.dsize == 0)
			return_val.dptr = (char *)malloc(1);
		else
			return_val.dptr = (char *)malloc(return_val.dsize);
		if (return_val.dptr == NULL)
			_gdbm_fatal(dbf, "malloc error");
		bcopy(find_data, return_val.dptr, return_val.dsize);
	}
	/* Check for an error and return. */
	if (return_val.dptr == NULL)
		gdbm_errno = GDBM_ITEM_NOT_FOUND;
	return return_val;
}
