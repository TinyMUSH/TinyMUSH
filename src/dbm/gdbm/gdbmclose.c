/* gdbmclose.c - Close a previously opened dbm file. */

#include "gdbmsystems.h"

/* Close the dbm file and free all memory associated with the file DBF.
   Before freeing members of DBF, check and make sure that they were
   allocated.  */

void
gdbm_close ( dbf )
gdbm_file_info *dbf;
{
    register int index;     /* For freeing the bucket cache. */

    /* Make sure the database is all on disk. */
    if ( dbf->read_write != GDBM_READER )
        fsync ( dbf->desc );

    /* Close the file and free all malloced memory. */
    if ( dbf->file_locking ) {
        UNLOCK_FILE ( dbf );
    }

    close ( dbf->desc );
    free ( dbf->name );

    if ( dbf->dir != NULL )
        free ( dbf->dir );

    if ( dbf->bucket_cache != NULL ) {
        for ( index = 0; index < dbf->cache_size; index++ ) {
            if ( dbf->bucket_cache[index].ca_bucket != NULL )
                free ( dbf->bucket_cache[index].ca_bucket );

            if ( dbf->bucket_cache[index].ca_data.dptr != NULL )
                free ( dbf->bucket_cache[index].ca_data.dptr );
        }

        free ( dbf->bucket_cache );
    }

    if ( dbf->header != NULL )
        free ( dbf->header );

    free ( dbf );
}
