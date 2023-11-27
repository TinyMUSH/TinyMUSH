/**
 * @file system.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Global headers and system compatibility
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 */

#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>


#include <string.h>

#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include <sys/resource.h>

#include <sys/file.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <sys/wait.h>


#include <sys/param.h>

#include <errno.h>

#include <malloc.h>

//#include <vfork.h>

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>

#include <fcntl.h>

#include <sys/socket.h>

#include <sys/utsname.h>

#include <signal.h>

#include <math.h>

#include <netinet/in.h>

//#include <zlib.h>

#include <sys/select.h>

#include <pcre.h>

//#include <ltdl.h>
#include <dlfcn.h>
#include <libgen.h>

#include <getopt.h>

#include <inttypes.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <float.h>

#include <gdbm.h>


#include <stdint.h>

#include <pthread.h>


#include <sys/ipc.h>

#include <sys/msg.h>


#include <stdbool.h>

/**
 * @note Take care of all the assorted problems associated with getrusage().
 * 
 */

/*
#ifdef _SEQUENT_
#define HAVE_GET_PROCESS_STATS 1
#include <sys/procstats.h>
#endif
*/
/*
#ifndef NSIG
extern const int _sys_nsig;
#define NSIG _sys_nsig
#endif
*/

/**
 * Some systems are lame, and inet_addr() claims to return -1 on failure,
 * despite the fact that it returns an unsigned long. (It's not really a -1,
 * obviously.) Better-behaved systems use INADDR_NONE.
 * 
 */
/*
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
*/

/*
#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif
*/

#endif /* __SYSTEM_H */
