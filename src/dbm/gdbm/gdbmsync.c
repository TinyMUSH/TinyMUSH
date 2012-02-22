/* gdbmsync.c - Sync the disk with the in memory state. */

/* $id$ */

#include "gdbmsystems.h"

/* Make sure the database is all on disk. */

void
gdbm_sync(dbf)
	gdbm_file_info *dbf;
{

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Do the sync on the file. */
	fsync(dbf->desc);

}
