/* proto.h - The prototypes for the dbm routines. */

/* $id$ */

#include "gdbmcopyright.h"

#ifndef __GDBMPROTO_H   
#define __GDBMPROTO_H

#ifndef __P
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif

/* From bucket.c */
void _gdbm_new_bucket __P((gdbm_file_info *, hash_bucket *, int));
void _gdbm_get_bucket __P((gdbm_file_info *, int));
void _gdbm_split_bucket __P((gdbm_file_info *, int));
void _gdbm_write_bucket __P((gdbm_file_info *, cache_elem *));

/* From falloc.c */
off_t _gdbm_alloc __P((gdbm_file_info *, int));
void _gdbm_free __P((gdbm_file_info *, off_t, int));
int _gdbm_put_av_elem __P((avail_elem, avail_elem[], int *, int));

/* From findkey.c */
char   *_gdbm_read_entry __P((gdbm_file_info *, int));
int _gdbm_findkey __P((gdbm_file_info *, datum, char **, int *));

/* From hash.c */
int _gdbm_hash __P((datum));

/* From update.c */
void _gdbm_end_update __P((gdbm_file_info *));
void _gdbm_fatal __P((gdbm_file_info *, char *));

/* From gdbmopen.c */
int _gdbm_init_cache __P((gdbm_file_info *, int));

/* The user callable routines.... */
void gdbm_close __P((gdbm_file_info *));
int gdbm_delete __P((gdbm_file_info *, datum));
datum gdbm_fetch __P((gdbm_file_info *, datum));
gdbm_file_info *gdbm_open __P((char *, int, int, int, void (*) (void)));
int gdbm_reorganize __P((gdbm_file_info *));
datum gdbm_firstkey __P((gdbm_file_info *));
datum gdbm_nextkey __P((gdbm_file_info *, datum));
int gdbm_store __P((gdbm_file_info *, datum, datum, int));
int gdbm_exists __P((gdbm_file_info *, datum));
void gdbm_sync __P((gdbm_file_info *));
int gdbm_setopt __P((gdbm_file_info *, int, int *, int));
int gdbm_fdesc __P((gdbm_file_info *));

#endif                                  /* __GDBMPROTO_H */
