/*!
 * \file alloc.h
 *
 * \date
 *
 * External definitions for memory allocation subsystem
 */

#include "copyright.h"

#ifndef __ALLOC_H
#define __ALLOC_H

/* We need to 64-bit-align the end of the pool header. */

typedef struct pool_header {
    int magicnum;		/*!< Magic Number, For consistency check */
    int pool_size;		/*!< Size of the pool,  for consistency check */
    struct pool_header *next;	/*!< Next pool header in chain */
    struct pool_header *nxtfree;	/*!< Next pool header in freelist */
    char *buf_tag;		/*!< Debugging/trace tag */
    char align[(256 - (2 * sizeof(int)) - (3 * sizeof(char *))) & 0x7];	/*!< Padding */
} POOLHDR;

typedef struct pool_footer {
    int magicnum;		/*!< Magic Number, For consistency check */
} POOLFTR;

typedef struct pooldata {
    int pool_size;		/*!< Size in bytes of a buffer */
    POOLHDR *free_head;		/*!< Buffer freelist head */
    POOLHDR *chain_head;	/*!< Buffer chain head */
    int tot_alloc;		/*!< Total buffers allocated */
    int num_alloc;		/*!< Number of buffers currently allocated */
    int max_alloc;		/*!< Max # buffers allocated at one time */
    int num_lost;		/*!< Buffers lost due to corruption */
} POOL;

#define POOL_SBUF	0	/*!< Small buffer pool */
#define POOL_MBUF	1	/*!< Standard buffer pool */
#define POOL_GBUF	2	/*!< Generic buffer pool */
#define POOL_LBUF	3	/*!< Large buffer pool */
#define POOL_HBUF	4	/*!< Huge buffer pool */
#define POOL_BOOL	5	/*!< Boolean buffer poll */
#define POOL_DESC	6	/*!< Description buffer pool */
#define POOL_QENTRY	7	/*!< Queue Entry buffer pool */
#define POOL_PCACHE	8	/*!< PCache buffer pool */
#define NUM_POOLS	9	/*!< Number of pool defined */

#define HBUF_SIZE	32768	/*!< Huge buffer size */
#define LBUF_SIZE	8192	/*!< Large buffer size */
#define GBUF_SIZE	1024	/*!< Generic buffer size */
#define MBUF_SIZE	512	/*!< Standard buffer size */
#define SBUF_SIZE	64	/*!< Small buffer size */

/*
 * ---------------------------------------------------------------------------
 * Basic allocation.
 */

extern int malloc_count;
extern int malloc_bytes;
extern void *malloc_str;
extern void *malloc_ptr;

extern void *xmalloc(size_t, const char *);
extern void *xcalloc(size_t, size_t, const char *);
extern void *xrealloc(void *, size_t, const char *);
extern void *xstrdup(const char *, const char *);
extern void *xstrndup(const void *, size_t, const char *);
extern void xfree(void *, const char *);

static void trace_alloc(size_t amount, const char *name, void *ptr);
static void trace_free(const char *name, void *ptr);

typedef struct tracemem_header {
    void *bptr;
    const char *buf_tag;
    size_t alloc;
    struct tracemem_header *next;
} MEMTRACK;

/*
 * ---------------------------------------------------------------------------
 * Pool allocation.
 */

extern void pool_init(int, int);
extern char *pool_alloc(int, const char *);
extern void pool_free(int, char **);
extern void list_bufstats(dbref);
extern void list_buftrace(dbref);

#define alloc_hbuf(s)	pool_alloc(POOL_HBUF,s)
#define free_hbuf(b)	pool_free(POOL_HBUF,((char **)&(b)))
#define alloc_lbuf(s)	pool_alloc(POOL_LBUF,s)
#define free_lbuf(b)	pool_free(POOL_LBUF,((char **)&(b)))
#define alloc_gbuf(s)	pool_alloc(POOL_GBUF,s)
#define free_gbuf(b)	pool_free(POOL_GBUF,((char **)&(b)))
#define alloc_mbuf(s)	pool_alloc(POOL_MBUF,s)
#define free_mbuf(b)	pool_free(POOL_MBUF,((char **)&(b)))
#define alloc_sbuf(s)	pool_alloc(POOL_SBUF,s)
#define free_sbuf(b)	pool_free(POOL_SBUF,((char **)&(b)))
#define alloc_bool(s)	(struct boolexp *)pool_alloc(POOL_BOOL,s)
#define free_bool(b)	pool_free(POOL_BOOL,((char **)&(b)))
#define alloc_qentry(s)	(BQUE *)pool_alloc(POOL_QENTRY,s)
#define free_qentry(b)	pool_free(POOL_QENTRY,((char **)&(b)))
#define alloc_pcache(s)	(PCACHE *)pool_alloc(POOL_PCACHE,s)
#define free_pcache(b)	pool_free(POOL_PCACHE,((char **)&(b)))

extern void safe_copy_chr(char, char[], char **, int);

#define safe_str(s,b,p)		safe_strcat((b),(p),(s),(LBUF_SIZE-1))
#define safe_chr(c,b,p)		safe_copy_chr((c),(b),(p),(LBUF_SIZE-1))
#define safe_sb_str(s,b,p)	safe_strcat((b),(p),(s),(SBUF_SIZE-1))
#define safe_sb_chr(c,b,p)	safe_copy_chr((c),(b),(p),(SBUF_SIZE-1))
#define safe_mb_str(s,b,p)	safe_strcat((b),(p),(s),(MBUF_SIZE-1))
#define safe_mb_chr(c,b,p)	safe_copy_chr((c),(b),(p),(MBUF_SIZE-1))
#define safe_chr_fn(c,b,p)	safe_strcatchr((b),(p),(c),(LBUF_SIZE-1))

#endif				/* __ALLOC_H */
