/* autoinclude.h -- System-dependent configuration information part 2 */

/*
 * ---------------------------------------------------------------------------
 * Setup section:
 *
 * Load system-dependent header files.
 */

/* Prototype templates for ANSI C and traditional C */

#ifdef __STDC__
#define	NDECL(f)	f(void)
#define	FDECL(f,p)	f p
#ifdef STDC_HEADERS
#define	VDECL(f,p)	f p
#else
#define VDECL(f,p)	f()
#endif
#else
#define NDECL(f)	f()
#define FDECL(f,p)	f()
#define VDECL(f,p)	f()
#endif

#ifdef STDC_HEADERS
#ifdef __STDC__
#include <stdarg.h>
#else				/* __STDC__ */
#include <varargs.h>
#endif				/* __STDC__ */
#include <stdlib.h>
#include <limits.h>
#else
#include <varargs.h>
extern int	FDECL(atoi, (const char *));
extern double	FDECL(atof, (const char *));
extern long	FDECL(atol, (const char *));
extern int	FDECL(qsort, (char *, int, int, int (*) ()));

#endif

#ifdef STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#endif

#ifdef NEED_STRTOK_R_DCL
extern char    *FDECL(strtok_r, (char *, const char *, char **));

#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#ifndef HAVE_GETPAGESIZE
#ifndef _SC_PAGE_SIZE
#define NM_BLOODY_PAGE_SYMBOL _SC_PAGESIZE
#else
#define NM_BLOODY_PAGE_SYMBOL _SC_PAGE_SIZE
#endif
#endif
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if defined(HAVE_SETRLIMIT) || defined(HAVE_GETRUSAGE)
#include <sys/resource.h>
#ifdef NEED_GETRUSAGE_DCL
extern int	FDECL(getrusage, (int, struct rusage *));

#endif
#ifdef NEED_GETRLIMIT_DCL
extern int	FDECL(getrlimit, (int, struct rlimit *));
extern int	FDECL(setrlimit, (int, struct rlimit *));

#endif
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_GETTIMEOFDAY
#ifdef NEED_GETTIMEOFDAY_DCL
extern int	FDECL(gettimeofday, (struct timeval *, struct timezone *));

#endif
#endif

#ifdef HAVE_GETDTABLESIZE
extern int	NDECL(getdtablesize);

#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_WAIT3
#ifdef NEED_WAIT3_DCL
#ifdef HAVE_UNION_WAIT
extern int	FDECL(wait3, (union wait *, int, struct rusage *));

#else
extern int	FDECL(wait3, (int *, int, struct rusage *));

#endif
#endif
#define WAITOPT(s,o) wait3((s), (o), NULL)
#else
#define WAITOPT(s,o) waitpid(0, (s), (o))
#endif

#ifdef HAVE_WAIT4
#ifdef NEED_WAIT4_DCL
#ifdef HAVE_UNION_WAIT
extern int	FDECL(wait4, (pid_t, union wait *, int, struct rusage *));

#else
extern int	FDECL(wait4, (pid_t, int *, int, struct rusage *));

#endif
#endif
#define WAITPID(p,s,o) wait4((p), (s), (o), NULL)
#else
#define WAITPID waitpid
#endif

#include <sys/param.h>
#ifndef HAVE_GETPAGESIZE
#ifdef EXEC_PAGESIZE
#define getpagesize()	EXEC_PAGESIZE
#else
#ifdef NBPG
#ifndef CLSIZE
#define CLSIZE 1
#endif				/* no CLSIZE */
#define getpagesize() NBPG * CLSIZE
#else				/* no NBPG */
#ifdef NBPC
#define getpagesize() NBPC
#else
#define getpagesize() PAGESIZE
#endif				/* no NBPC */
#endif				/* no NBPG */
#endif				/* no EXEC_PAGESIZE */
#else				/* we've got a getpagesize() function, whee */
#ifdef NEED_GETPAGESIZE_DCL
extern int	NDECL(getpagesize);

#endif				/* NEED_GETPAGESIZE_DCL */
#endif				/* HAVE_GETPAGESIZE */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#ifdef NEED_PERROR_DCL
extern void	FDECL(perror, (const char *));

#endif
#else
extern int	errno;
extern void	FDECL(perror, (const char *));

#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#ifdef NEED_MALLOC_DCL
extern char    *FDECL(malloc, (int));
extern char    *FDECL(realloc, (char *, int));
extern int	FDECL(free, (char *));

#endif
#endif

#ifndef HAVE_TIMELOCAL

#ifndef HAVE_MKTIME
#define NEED_TIMELOCAL
extern time_t	FDECL(timelocal, (struct tm *));

#else
#define timelocal mktime
#endif				/* HAVE_MKTIME */

#endif				/* HAVE_TIMELOCAL */

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#ifndef HAVE_SRANDOM
#define random rand
#define srandom srand
#else
#ifdef NEED_SRANDOM_DCL
#ifndef random			/* only if not a macro */
#ifdef NEED_RANDOM_DCL
extern long	NDECL(random);

#endif				/* NEED_RANDOM_DCL */
#endif
extern int	FDECL(srandom, (int));

#endif				/* NEED_SRANDOM_DCL */
#endif				/* HAVE_SRANDOM */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>

#ifndef VMS
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#ifdef NEED_SPRINTF_DCL
extern char    *VDECL(sprintf, (char *, const char *,...));

#endif

#ifndef EXTENDED_STDIO_DCLS
extern int	VDECL(fprintf, (FILE *, const char *,...));
extern int	VDECL(printf, (const char *,...));
extern int	VDECL(sscanf, (const char *, const char *,...));
extern int	FDECL(close, (int));
extern int	FDECL(fclose, (FILE *));
extern int	FDECL(fflush, (FILE *));
extern int	FDECL(fgetc, (FILE *));
extern int	FDECL(fputc, (int, FILE *));
extern int	FDECL(fputs, (const char *, FILE *));
extern int	FDECL(fread, (void *, size_t, size_t, FILE *));
extern int	FDECL(fseek, (FILE *, long, int));
extern int	FDECL(fwrite, (void *, size_t, size_t, FILE *));
extern pid_t	FDECL(getpid, (void));
extern int	FDECL(pclose, (FILE *));
extern int	FDECL(rename, (char *, char *));
extern time_t	FDECL(time, (time_t *));
extern int	FDECL(ungetc, (int, FILE *));
extern int	FDECL(unlink, (const char *));

#endif

#include <sys/socket.h>
#ifndef EXTENDED_SOCKET_DCLS
extern int	FDECL(accept, (int, struct sockaddr *, int *));
extern int	FDECL(bind, (int, struct sockaddr *, int));
extern int	FDECL(listen, (int, int));
extern int	FDECL(setsockopt, (int, int, int, void *, int));
extern int	FDECL(shutdown, (int, int));
extern int	FDECL(socket, (int, int, int));
extern int	FDECL(select, (int, fd_set *, fd_set *, fd_set *, struct timeval *));

#endif

/*
 * #ifdef HAVE_ST_BLKSIZE #define STATBLKSIZE file_stat.st_blksize #else
 * #define STATBLKSIZE 8192 #endif
 */

#ifdef __linux__
#ifndef __GLIBC__

/*
 * In theory, under Linux, we want to use the optimized string functions.
 * However, they make Redhat Linux 6, at least, spew, because GNU libc
 * already uses its own set of optimized string functions with gcc -O.
 */

#include <asm/string.h>

#else

/*
 * GNU libc is also broken in the fact that it declares BSD datatypes along
 * with SYSV, POSIX, etc which breaks autoconf. So we have to manually fix
 * those inconsistencies.
 */

#define GLIBC_BRAINDAMAGE

#endif
#endif

typedef int	dbref;
typedef int	FLAG;
typedef int	POWER;
typedef char	boolexp_type;
typedef char	IBUF[16];

#ifndef HAVE_STRCASECMP
/*
 * Since strcasecmp isn't POSIX, some systems don't have it by default
 */
#define strcasecmp(a,b)	memcmp((void *)a,(void *)b,strlen(a))
#endif

#ifdef _UWIN
#define INLINE
#else
#define INLINE inline
#endif

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif
