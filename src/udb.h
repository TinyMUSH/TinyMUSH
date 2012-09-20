/* udb.h */

#include "copyright.h"

#ifndef __UDB_H
#define __UDB_H

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* Define the number of objects we may be reading/writing to at one time */

#define NUM_OBJPIPES	64

/* For MUSH, an int works great as an object ID
 * And attributes are zero terminated strings, so we heave the size out.
 * We hand around attribute identifiers in the last things.
 */

typedef	char		Attr;

typedef	unsigned int	Objname;

typedef struct Aname
{
    unsigned int	object;
    unsigned int	attrnum;
} Aname;

/* In general, we want binary attributes, so we do this. */

typedef struct Attrib
{
    int	attrnum;	/* MUSH specific identifier */
    int	size;
    char	*data;
} Attrib;

/* An object is a name, an attribute count, and a vector of attributes */
/* which Attr's are stowed in a contiguous array pointed at by atrs.   */

typedef struct Obj
{
    Objname	name;
    time_t	counter;
    int	dirty;
    int	at_count;
    Attrib	*atrs;
} Obj;

typedef struct cache
{
    void *keydata;
    int keylen;
    void *data;
    int datalen;
    unsigned int type;
    unsigned int flags;
    struct cache *nxt;
    struct cache *prvfree;
    struct cache *nxtfree;
} Cache;

typedef struct
{
    Cache *head;
    Cache *tail;
} Chain;

typedef struct
{
    void *dptr;
    int dsize;
} DBData;

/* Cache flags */

#define CACHE_DIRTY	0x00000001

extern int 	FDECL(cache_init, (int));
extern void 	NDECL(cache_reset);
extern int 	NDECL(cache_sync);
extern DBData	FDECL(db_get, (DBData, unsigned int));
extern int	FDECL(db_put, (DBData, DBData, unsigned int));
extern int	FDECL(db_del, (DBData, unsigned int));
extern void	NDECL(db_lock);
extern void	NDECL(db_unlock);
extern DBData	FDECL(cache_get, (DBData, unsigned int));
extern int	FDECL(cache_put, (DBData, DBData, unsigned int));
extern void	FDECL(cache_del, (DBData, unsigned int));
extern void	NDECL(attrib_sync);
extern char	*FDECL(pipe_get_attrib, (int, int));
extern void	FDECL(pipe_set_attrib, (int, int, char *));
extern void	FDECL(pipe_del_attrib, (int, int));
extern int	FDECL(obj_siz, (Obj *));

#endif /* __UDB_H */
