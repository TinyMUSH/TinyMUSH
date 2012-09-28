/* file_c.h -- File cache header file */

#include "copyright.h"

#ifndef __FILE_C_H
#define	__FILE_C_H

/* File caches.  These _must_ track the fcache array in file_c.c */

#define	FC_CONN		0
#define	FC_CONN_SITE	1
#define	FC_CONN_DOWN	2
#define	FC_CONN_FULL	3
#define	FC_CONN_GUEST	4
#define	FC_CONN_REG	5
#define	FC_CREA_NEW	6
#define	FC_CREA_REG	7
#define	FC_MOTD		8
#define	FC_WIZMOTD	9
#define	FC_QUIT		10
#ifdef PUEBLO_SUPPORT
#define FC_CONN_HTML    11
#define FC_LAST         11
#else
#define FC_LAST         10
#endif				/* PUEBLO_SUPPORT */

/* File cache routines */

extern void	fcache_rawdump(int fd, int num);
extern void	fcache_dump(DESC * d, int num);
extern void	fcache_send(dbref, int);
extern void	fcache_load(dbref);
extern void	fcache_init(void);

#endif	/* __FILE_C_H */
