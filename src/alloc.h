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

#ifndef __ALLOC_H
#define __ALLOC_H

#define XMAGIC 0x00deadbeefbaad00

// Allocation Functions
#define XMALLOC(s, v) __xmalloc(s, __FILE__, __LINE__, __func__, v)
#define XNMALLOC(s) __xmalloc(s, NULL, 0, NULL, NULL)
#define XCALLOC(n, s, v) __xcalloc(n, s, __FILE__, __LINE__, __func__, v)
#define XNCALLOC(n, s) __xcalloc(n, s, NULL, 0, NULL, NULL)
#define XREALLOC(p, s, v) __xrealloc(p, s, __FILE__, __LINE__, __func__, v)
#define XNREALLOC(p, s) __xrealloc(p, s, NULL, 0, NULL, NULL)
#define XFREE(p) __xfree(p)
#define XHEADER(p) (MEMTRACK *)((char *)p - sizeof(MEMTRACK));

// String Functions
#define XASPRINTF(v, f, ...) __xasprintf(__FILE__, __LINE__, __func__, v, f, ##__VA_ARGS__)
#define XNASPRINTF(f, ...) __xasprintf(NULL, 0, NULL, NULL, f, ##__VA_ARGS__);
#define XSPRINTF(s, f, ...) __xsprintf(s, f, ##__VA_ARGS__)
#define XSPRINTFCAT(s, f, ...) __xsprintfcat(s, f, ##__VA_ARGS__)
#define XVSPRINTF(s, f, a) __xvsprintf(s, f, a)
#define XSNPRINTF(s, n, f, ...) __xsnprintf(s, n, f, ##__VA_ARGS__)
#define XVSNPRINTF(s, n, f, a) __xvsnprintf(s, n, f, a)
#define XSTRDUP(s, v) __xstrdup(s, __FILE__, __LINE__, __func__, v)
#define XNSTRDUP(s) __xstrdup(s, NULL, 0, NULL, NULL)
#define XSTRCAT(d, s) __xstrcat(d, s)
#define XSTRNCAT(d, s, n) __xstrncat(d, s, n)
#define XSTRCCAT(d, s) __xstrccat(d, s)
#define XSTRNCCAT(d, s, n) __xstrnccat(d, s, n)
#define XSTRCPY(d, s) __xstrcpy(d, s)
#define XSTRNCPY(d, s, n) __xstrncpy(d, s, n)
#define XMEMMOVE(d, s, n) __xmemmove(d, s, n)
#define XMEMCPY(d, s, n) __xmemmove(d, s, n)
#define XMEMSET(s, c, n) __xmemset(s, c, n)

// Replacement for the safe_* functions.
#define SAFE_SPRINTF(s, p, f, ...) __xsafesprintf(s, p, f, ##__VA_ARGS__);

#define SAFE_COPY_CHR(c, b, p, s) __xsafestrcatchr(b, p, c, s)
#define SAFE_STRCATCHR(b, p, c, s) __xsafestrcatchr(b, p, c, s)

#define SAFE_LB_CHR(c, b, p) __xsafestrcatchr(b, p, c, LBUF_SIZE - 1)
#define SAFE_SB_CHR(c, b, p) __xsafestrcatchr(b, p, c, SBUF_SIZE - 1)
#define SAFE_MB_CHR(c, b, p) __xsafestrcatchr(b, p, c, MBUF_SIZE - 1)

#define SAFE_STRNCAT(b, p, s, n, z) __xsafestrncat(b, p, s, n, z);
#define SAFE_STRCAT(b, p, s, n) __xsafestrncat(b, p, s, strlen(s), n)

#define SAFE_LB_STR(s, b, p) __xsafestrncpy(b, p, s, strlen(s), LBUF_SIZE - 1)
#define SAFE_SB_STR(s, b, p) __xsafestrncpy(b, p, s, strlen(s), SBUF_SIZE - 1)
#define SAFE_MB_STR(s, b, p) __xsafestrncpy(b, p, s, strlen(s), MBUF_SIZE - 1)

#define SAFE_CRLF(b, p) SAFE_STRCAT(b, p, "\r\n", LBUF_SIZE - 1)
#define SAFE_ANSI_NORMAL(b, p) SAFE_STRCAT(b, p, ANSI_NORMAL, LBUF_SIZE - 1)
#define SAFE_NOTHING(b, p) SAFE_STRCAT(b, p, "#-1", LBUF_SIZE - 1)
#define SAFE_NOPERM(b, p) SAFE_STRCAT(b, p, "#-1 PERMISSION DENIED", LBUF_SIZE - 1)
#define SAFE_NOMATCH(b, p) SAFE_STRCAT(b, p, "#-1 NO MATCH", LBUF_SIZE - 1)
#define SAFE_BOOL(b, p, n) SAFE_LB_CHR(((n) ? '1' : '0'), (b), (p))

#define SAFE_LTOS(d, p, n, s) __xsafeltos(d, p, n, s)

// Misc Macros
#define XGETSIZE(p) (((char *)__xfind(p)->magic - p) - sizeof(char))
#define XCALSIZE(s, d, p) ((s - ((*p) - d)))
#define XLTOS(n) __xasprintf(__FILE__, __LINE__, __func__, "XLTOS", "%ld", n)

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
