/* autoinclude.h -- System-dependent configuration information part 2 */

/*
 * ---------------------------------------------------------------------------
 * Setup section:
 *
 * Load system-dependent header files.
 */

/* Prototype templates for ANSI C and traditional C */

#ifdef STDC_HEADERS
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif				/* __STDC__ */
#include <stdlib.h>
#include <limits.h>
#else				/* STDC_HEADERS */
#include <varargs.h>
#endif				/* STDC_HEADERS */

#ifdef STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif				/* HAVE_MEMORY_H */
#endif				/* STDC_HEADERS */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
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
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <sys/param.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>

#include <fcntl.h>

#include <sys/socket.h>

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

//#include <ltdl.h>
