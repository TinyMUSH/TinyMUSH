/* alloc.h - External definitions for memory allocation subsystem */
/* $Id: alloc.h,v 1.18 2002/09/23 04:59:36 rmg Exp $ */

#include "copyright.h"

#ifndef __ALLOC_H
#define __ALLOC_H

/* We need to 64-bit-align the end of the pool header. */

typedef struct pool_header
{
    int		magicnum;	/* For consistency check */
    int		pool_size;	/* For consistency check */
    struct pool_header *next;	/* Next pool header in chain */
    struct pool_header *nxtfree;	/* Next pool header in freelist */
    char           *buf_tag;/* Debugging/trace tag */
    char		align     [(256 - 2 * sizeof(int) - 3 * sizeof(char *)) & 0x7];
}		POOLHDR;

typedef struct pool_footer
{
    int		magicnum;	/* For consistency check */
}		POOLFTR;

typedef struct pooldata
{
    int		pool_size;	/* Size in bytes of a buffer */
    POOLHDR        *free_head;	/* Buffer freelist head */
    POOLHDR        *chain_head;	/* Buffer chain head */
    int		tot_alloc;	/* Total buffers allocated */
    int		num_alloc;	/* Number of buffers currently
					 * allocated */
    int		max_alloc;	/* Max # buffers allocated at one
					 * time */
    int		num_lost;	/* Buffers lost due to corruption */
}		POOL;

#define	POOL_SBUF	0
#define	POOL_MBUF	1
#define	POOL_LBUF	2
#define	POOL_BOOL	3
#define	POOL_DESC	4
#define	POOL_QENTRY	5
#define POOL_PCACHE	6
#define	NUM_POOLS	7


#define LBUF_SIZE	8000	/* standard lbuf */
#define GBUF_SIZE       1024	/* generic buffer size */
#define MBUF_SIZE	400	/* standard mbuf */
#define SBUF_SIZE	64	/* standard sbuf, short strings */

/*
 * #define LBUF_SIZE	4000 #define MBUF_SIZE	200 #define SBUF_SIZE	32
 */

/*
 * ---------------------------------------------------------------------------
 * Basic allocation.
 */

#define RAW_MALLOC(x,y) (malloc(x))
#define RAW_CALLOC(x,z,y) (calloc((x),(z)))
#define RAW_REALLOC(x,z,y) (realloc((x),(z)))
#define RAW_STRDUP(x,y) (strdup(x))
#define RAW_FREE(x,y) (free((void *)(x)), (x) = NULL)

#ifdef TEST_MALLOC

extern int	malloc_count;
extern int	malloc_bytes;
extern char    *malloc_str;
extern char    *malloc_ptr;

#define XMALLOC(x,y) (fprintf(stderr,"Malloc: %s/%d\n", (y), (x)), malloc_count++, \
                    malloc_ptr = (char *)malloc((x) + sizeof(int)), malloc_bytes += (x), \
                    *(int *)malloc_ptr = (x), malloc_ptr + sizeof(int))
#define XCALLOC(x,z,y) (fprintf(stderr,"Calloc: %s/%d\n", (y), (x)*(z)), malloc_count++, \
                    malloc_ptr = (char *)malloc((x)*(z) + sizeof(int)), malloc_bytes += (x)*(z), \
                    memset(malloc_ptr, 0, (x)*(z) + sizeof(int)), \
                    *(int *)malloc_ptr = (x)*(z), malloc_ptr + sizeof(int))
#define XREALLOC(x,z,y) (fprintf(stderr,"Realloc: %s/%d\n", (y), (z)), \
                    malloc_ptr = (char *)malloc((z) + sizeof(int)), malloc_bytes += (z), \
                    malloc_bytes -= *(int *)((char *)(x)-sizeof(int)), memcpy(malloc_ptr + sizeof(int), (x), *(int *)((char *)(x) - sizeof(int))), \
                    free((char *)(x) - sizeof(int)), *(int *)malloc_ptr = (z), malloc_ptr + sizeof(int))
#define XSTRDUP(x,y) (malloc_str = (char *)(x), \
		    fprintf(stderr,"Strdup: %s/%d\n", (y), strlen(malloc_str)+1), malloc_count++, \
                    malloc_ptr = (char *)malloc(strlen(malloc_str) + 1 + sizeof(int)), \
                    malloc_bytes += strlen(malloc_str) + 1, strcpy(malloc_ptr + sizeof(int), malloc_str), \
                    *(int *)malloc_ptr = strlen(malloc_str) + 1, malloc_ptr + sizeof(int))
#define XSTRNDUP(x,z,y) (malloc_str = (char *)(x), \
		    fprintf(stderr,"Strndup: %s/%d\n", (y), strlen(malloc_str)+1), malloc_count++, \
                    malloc_ptr = (char *)malloc((z) + sizeof(int)), \
                    malloc_bytes += (z), strncpy(malloc_ptr + sizeof(int), malloc_str, (z)), \
                    *(int *)malloc_ptr = (z), malloc_ptr + sizeof(int))
#define XFREE(x,y) (fprintf(stderr, "Free: %s/%d\n", (y), (x) ? *(int *)((char *)(x)-sizeof(int)) : 0), \
                    ((x) ? malloc_count--, malloc_bytes -= *(int *)((char *)(x)-sizeof(int)), \
                    free((char *)(x) - sizeof(int)), (x)=NULL : (x)))

#else				/* ! TEST_MALLOC  */

#ifdef RAW_MEMTRACKING

#define XMALLOC(x,y) (track_malloc((x),(y)))
#define XCALLOC(x,z,y) (track_calloc((x),(z),(y)))
#define XREALLOC(x,z,y) (track_realloc((x),(z),(y)))
#define XSTRDUP(x,y) (track_strdup((x),(y)))
#define XFREE(x,y) (track_free((void *)(x),(y)), (x) = NULL)

extern void    *FDECL(track_malloc, (size_t, const char *));
extern void    *FDECL(track_calloc, (size_t, size_t, const char *));
extern void    *FDECL(track_realloc, (void *, size_t, const char *));
extern char    *FDECL(track_strdup, (const char *, const char *));
extern void	FDECL(track_free, (void *, const char *));

typedef struct tracemem_header
{
    void           *bptr;
    const char     *buf_tag;
    size_t		alloc;
    struct tracemem_header *next;
}		MEMTRACK;

#else

#define XMALLOC(x,y)	RAW_MALLOC((x),(y))
#define XCALLOC(x,z,y)	RAW_CALLOC((x),(z),(y))
#define XREALLOC(x,z,y)	RAW_REALLOC((x),(z),(y))
#define XSTRDUP(x,y)	RAW_STRDUP((x),(y))
#define XFREE(x,y)	RAW_FREE((x),(y))

#endif				/* RAW_MEMTRACKING */
#endif				/* TEST_MALLOC */

/*
 * ---------------------------------------------------------------------------
 * Pool allocation.
 */

extern void	FDECL(pool_init, (int, int));
extern char    *FDECL(pool_alloc, (int, const char *));
extern void	FDECL(pool_free, (int, char **));
extern void	FDECL(list_bufstats, (dbref));
extern void	FDECL(list_buftrace, (dbref));

#define	alloc_lbuf(s)	pool_alloc(POOL_LBUF,s)
#define	free_lbuf(b)	pool_free(POOL_LBUF,((char **)&(b)))
#define	alloc_mbuf(s)	pool_alloc(POOL_MBUF,s)
#define	free_mbuf(b)	pool_free(POOL_MBUF,((char **)&(b)))
#define	alloc_sbuf(s)	pool_alloc(POOL_SBUF,s)
#define	free_sbuf(b)	pool_free(POOL_SBUF,((char **)&(b)))
#define	alloc_bool(s)	(struct boolexp *)pool_alloc(POOL_BOOL,s)
#define	free_bool(b)	pool_free(POOL_BOOL,((char **)&(b)))
#define	alloc_qentry(s)	(BQUE *)pool_alloc(POOL_QENTRY,s)
#define	free_qentry(b)	pool_free(POOL_QENTRY,((char **)&(b)))
#define alloc_pcache(s)	(PCACHE *)pool_alloc(POOL_PCACHE,s)
#define free_pcache(b)	pool_free(POOL_PCACHE,((char **)&(b)))

#define safe_copy_chr(scc__src,scc__buff,scc__bufp,scc__max) {\
    char *scc__tp;\
\
    scc__tp = *scc__bufp;\
    if ((scc__tp - scc__buff) < scc__max) {\
	*scc__tp++ = scc__src;\
	*scc__bufp = scc__tp;\
	*scc__tp = '\0';\
    } else {\
	scc__buff[scc__max] = '\0';\
    }\
}

#define	safe_str(s,b,p)		safe_copy_str((s),(b),(p),(LBUF_SIZE-1))
#define	safe_str_fn(s,b,p)	safe_copy_str_fn((s),(b),(p),(LBUF_SIZE-1))
#define	safe_chr(c,b,p)		safe_copy_chr((c),(b),(p),(LBUF_SIZE-1))
#define safe_long_str(s,b,p)    safe_copy_long_str((s),(b),(p),(LBUF_SIZE-1))
#define	safe_sb_str(s,b,p)	safe_copy_str((s),(b),(p),(SBUF_SIZE-1))
#define	safe_sb_chr(c,b,p)	safe_copy_chr((c),(b),(p),(SBUF_SIZE-1))
#define	safe_mb_str(s,b,p)	safe_copy_str((s),(b),(p),(MBUF_SIZE-1))
#define	safe_mb_chr(c,b,p)	safe_copy_chr((c),(b),(p),(MBUF_SIZE-1))
#define safe_chr_fn(c,b,p)      safe_chr_real_fn((c),(b),(p),(LBUF_SIZE-1))

#endif				/* __ALLOC_H */
