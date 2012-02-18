/* gdbfdesc.c - return the file descriptor associated with the database. */

/* $id$ */

#include "autoconf.h"

/* Return the file number of the DBF file. */

int
gdbm_fdesc(dbf)
	gdbm_file_info *dbf;
{
	return (dbf->desc);
}
