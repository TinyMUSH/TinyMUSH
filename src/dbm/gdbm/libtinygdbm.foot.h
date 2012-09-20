
/* The file information header. This is good enough for most applications. */
typedef struct {int dummy[10];} *GDBM_FILE;

/* Determine if the C(++) compiler requires complete function prototype  */
#ifndef __P
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define __P(x) x
#else
#define __P(x) ()
#endif
#endif

/* External variable, the gdbm build release string. */
extern char *gdbm_version;	


/* GDBM C++ support */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* These are the routines! */

extern GDBM_FILE gdbm_open __P((char *, int, int, int, void (*)()));
extern void gdbm_close __P((GDBM_FILE));
extern int gdbm_store __P((GDBM_FILE, datum, datum, int));
extern datum gdbm_fetch __P((GDBM_FILE, datum));
extern int gdbm_delete __P((GDBM_FILE, datum));
extern datum gdbm_firstkey __P((GDBM_FILE));
extern datum gdbm_nextkey __P((GDBM_FILE, datum));
extern int gdbm_reorganize __P((GDBM_FILE));
extern void gdbm_sync __P((GDBM_FILE));
extern int gdbm_exists __P((GDBM_FILE, datum));
extern int gdbm_setopt __P((GDBM_FILE, int, int *, int));
extern int gdbm_fdesc __P((GDBM_FILE));

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/* extra prototypes */

/* GDBM C++ support */
#if defined(__cplusplus) || defined(c_plusplus)
extern	"C" {
#endif

	extern char *gdbm_strerror __P((gdbm_error));

#if defined(__cplusplus) || defined(c_plusplus)
}

#endif

#endif					/* __LIBTINYGDBM_H */
