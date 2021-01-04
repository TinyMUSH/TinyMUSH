/**
 * @file system.h
 * @author TinyMUSH (https://github.com/TinyMUSH)
 * @brief Global headers and system compatibility
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

/* system.h */

#ifndef __SYSTEM_H
#define __SYSTEM_H

#ifdef STDC_HEADERS
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif /* __STDC__ */
#include <stdlib.h>
#include <limits.h>
#else /* STDC_HEADERS */
#include <varargs.h>
#endif /* STDC_HEADERS */

#ifdef STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif /* HAVE_MEMORY_H */
#endif /* STDC_HEADERS */

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

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_PCRE_H
#include <pcre.h>
#endif

#include <ltdl.h>
#include <libgen.h>

#include <getopt.h>

#include <inttypes.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_FLOAT_H
#include <float.h>
#else
#define LDBL_DIG 6
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
/* XXX Add definitions here */
#endif

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
#ifndef __cplusplus
#define bool    _Bool
#define true    1
#define false   0
#else
#define _Bool   bool
#if __cplusplus < 201103L
#define bool    bool
#define false   false
#define true    true
#endif
#endif
#define __bool_true_false_are_defined   1
#endif

/**
 * @note Take care of all the assorted problems associated with getrusage().
 * 
 */

#ifdef hpux
#define HAVE_GETRUSAGE 1
#include <sys/syscall.h>
#define getrusage(x, p) syscall(SYS_GETRUSAGE, x, p)
#endif

#ifdef _SEQUENT_
#define HAVE_GET_PROCESS_STATS 1
#include <sys/procstats.h>
#endif

#endif /* __SYSTEM_H */
