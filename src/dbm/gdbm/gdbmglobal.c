/* global.c - The external variables needed for "original" interface and
   error messages. */

#include "gdbmsystems.h"

/* The global variables used for the "original" interface. */
gdbm_file_info *_gdbm_file = NULL;

/* Memory for return data for the "original" interface. */
datum	_gdbm_memory = {NULL, 0};	/* Used by firstkey and nextkey. */
char   *_gdbm_fetch_val = NULL;		/* Used by fetch. */

/* The dbm error number is placed in the variable GDBM_ERRNO. */
gdbm_error gdbm_errno = GDBM_NO_ERROR;
