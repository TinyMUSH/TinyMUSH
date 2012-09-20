/* extern.h - The collection of external definitions needed. */

#include "gdbmcopyright.h"

#ifndef __GDBMEXTERN_H
#define __GDBMEXTERN_H

/* The global variables used for the "original" interface. */
extern gdbm_file_info *_gdbm_file;

/* Memory for return data for the "original" interface. */
extern datum _gdbm_memory;
extern char *_gdbm_fetch_val;

#endif                                  /* __GDBMEXTERN_H */
