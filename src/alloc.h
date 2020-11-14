/**
 * @file alloc.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief memory management subsystem
 * @version 3.3
 * @date 2020-11-14
 * 
 * @copyright Copyright (C) 1989-2020 TinyMUSH development team.
 * 
 */

#include "copyright.h"

#ifndef __ALLOC_H
#define __ALLOC_H

#define XMAGIC 0x00deadbeefbaad00

// Utilities
#define XLOGALLOC(x,y,z,s,...) if(mudconf.malloc_logger){log_write(x,y,z,s,##__VA_ARGS__);}

// Allocation Functions
#define XMALLOC(s, v) __xmalloc(s, __FILE__, __LINE__, __func__, v)
#define XNMALLOC(s) __xmalloc(s, NULL, 0, NULL, NULL)
#define XCALLOC(n, s, v) __xcalloc(n, s, __FILE__, __LINE__, __func__, v)
#define XNCALLOC(n, s) __xcalloc(n, s, NULL, 0, NULL, NULL)
#define XREALLOC(p, s, v) __xrealloc(p, s, __FILE__, __LINE__, __func__, v)
#define XNREALLOC(p, s) __xrealloc(p, s, NULL, 0, NULL, NULL)
#define XFREE(p) __xfree(p)
#define XHEADER(p) (MEMTRACK *)((char *)p - sizeof(MEMTRACK));

//String Functions
#define XASPRINTF(v, f,...) __xasprintf(__FILE__,__LINE__,__func__,v,f,##__VA_ARGS__)
#define XNASPRINTF(f,...) __xasprintf(NULL,0,NULL,NULL,f,##__VA_ARGS__);
#define XSPRINTF(s,f,...) __xsprintf(s,f,##__VA_ARGS__)
#define XVSPRINTF(s,f,a) __xvsprintf(s,f,a)
#define XSNPRINTF(s,n,f,...) __xsnprintf(s,n,f,##__VA_ARGS__)
#define XVSNPRINTF(s,n,f,...) __xvsnprintf(s,n,f,a)
#define XSTRDUP(s, v) __xstrdup(s, __FILE__,__LINE__,__func__,v)
#define XNSTRDUP(s) __xstrdup(s,NULL,0,NULL,NULL)
#define XSTRCAT(d,s) __xstrcat(d,s)
#define XSTRNCAT(d,s,n) __xstrncat(d,s,n)
#define XSTRCPY(d,s) __xstrcpy(d,s)
#define XSTRNCPY(d,s,n) __xstrncpy(d,s,n)
#define XMEMMOVE(d,s,n) __xmemmove(d,s,n)
#define XMEMCPY(d,s,n) __xmemmove(d,s,n)

typedef struct tracemem_header
{
    size_t size;
    void *bptr;
    const char *file;
    int line;
    const char *function;
    const char *var;
    uint64_t *magic;
    struct tracemem_header *next;
} MEMTRACK;

#define HBUF_SIZE 32768 /*!< Huge buffer size */
#define LBUF_SIZE 8192  /*!< Large buffer size */
#define GBUF_SIZE 1024  /*!< Generic buffer size */
#define MBUF_SIZE 512   /*!< Standard buffer size */
#define SBUF_SIZE 64    /*!< Small buffer size */

#endif /* __ALLOC_H */
