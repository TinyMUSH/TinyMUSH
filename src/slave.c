/* slave.c - does iptoname conversions, and identquery lookups */

/*
 * The philosophy is to keep this program as simple/small as possible.
 * It does normal fork()s, so the smaller it is, the faster it goes.
 */
#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "slave.h"
#include <arpa/inet.h>

/* Some systems are lame, and inet_addr() returns -1 on failure, despite
 * the fact that it returns an unsigned long.
 */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

#define MAX_STRING 1000
#define MAX_CHILDREN 20

pid_t parent_pid;

volatile pid_t child_pids[MAX_CHILDREN];

char *arg_for_errors;

char *format_inet_addr(char *dest, unsigned long addr) {
    sprintf(dest, "%lu.%lu.%lu.%lu",
            (addr & 0xFF000000) >> 24,
            (addr & 0x00FF0000) >> 16,
            (addr & 0x0000FF00) >> 8, (addr & 0x000000FF));
    return (dest + strlen(dest));
}

/*
 * copy a string, returning pointer to the null terminator of dest
 */
char *stpcopy(char *dest, const char *src) {
    while ((*dest = *src)) {
        ++dest;
        ++src;
    }
    return (dest);
}

void child_timeout_signal(int sig) {
    exit(1);
}

int query(char *ip, char *orig_arg) {
    char *comma;

    char *port_pair;

    struct hostent *hp;

    struct sockaddr_in sin;

    int s;

    FILE *f;

    char result[MAX_STRING];

    char buf[MAX_STRING * 2];

    char buf2[MAX_STRING * 2];

    char buf3[MAX_STRING * 4];

    char arg[MAX_STRING];

    size_t len;

    char *p;

    unsigned int addr;


    addr = inet_addr(ip);
    if (addr == INADDR_NONE) {
        return (-1);
    }
    hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
    sprintf(buf, "%s %s\n",
            ip, ((hp && strlen(hp->h_name) < MAX_STRING) ? hp->h_name : ip));
    arg_for_errors = orig_arg;
    strcpy(arg, orig_arg);
    comma = (char *)strrchr(arg, ',');
    if (comma == NULL) {
        return (-1);
    }
    *comma = 0;
    port_pair = (char *)strrchr(arg, ',');
    if (port_pair == NULL) {
        return (-1);
    }
    *port_pair++ = 0;
    *comma = ',';

    hp = gethostbyname(arg);
    if (hp == NULL) {
        static struct hostent def;

        static struct in_addr defaddr;

        static char *alist[1];

        static char namebuf[MAX_STRING];

        defaddr.s_addr = (unsigned int)inet_addr(arg);
        if (defaddr.s_addr == INADDR_NONE) {
            return (-1);
        }
        strcpy(namebuf, arg);
        def.h_name = namebuf;
        def.h_addr_list = alist;
        def.h_addr = (char *)&defaddr;
        def.h_length = sizeof(struct in_addr);

        def.h_addrtype = AF_INET;
        def.h_aliases = 0;
        hp = &def;
    }
    sin.sin_family = hp->h_addrtype;
    memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_port = htons(113);	/*
					 * ident port
					 */
    s = socket(hp->h_addrtype, SOCK_STREAM, 0);
    if (s < 0) {
        return (-1);
    }
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        if (errno != ECONNREFUSED
                && errno != ETIMEDOUT
                && errno != ENETUNREACH && errno != EHOSTUNREACH) {
            close(s);
            return (-1);
        }
        buf2[0] = '\0';
    } else {
        len = strlen(port_pair);
        if (write(s, port_pair, len) != (int)len) {
            close(s);
            return (-1);
        }
        if (write(s, "\r\n", 2) != 2) {
            close(s);
            return (-1);
        }
        f = fdopen(s, "r");
        {
            int c;

            p = result;
            while ((c = fgetc(f)) != EOF) {
                if (c == '\n')
                    break;
                if (isprint(c)) {
                    *p++ = c;
                    if (p - result == MAX_STRING - 1)
                        break;
                }
            }
            *p = '\0';
        }
        (void)fclose(f);
        p = format_inet_addr(buf2, ntohl(sin.sin_addr.s_addr));
        *p++ = ' ';
        p = stpcopy(p, result);
        *p++ = '\n';
        *p++ = '\0';
    }
    sprintf(buf3, "%s%s", buf, buf2);
    write(1, buf3, strlen(buf3));
    return (0);
}

void child_signal(int sig) {
    pid_t child_pid;

    int i;

    /*
     * see if any children have exited
     */
    while ((child_pid = waitpid(0, NULL, WNOHANG)) > 0) {
        for (i = 0; i < MAX_CHILDREN; i++) {
            if (child_pids[i] == child_pid) {
                child_pids[i] = -1;
                break;
            }
        }
    }
    signal(SIGCHLD, (void (*)(int))child_signal);
}

void alarm_signal(int sig) {
    struct itimerval itime;

    struct timeval interval;

    if (getppid() != parent_pid) {
        exit(1);
    }
    signal(SIGALRM, (void (*)(int))alarm_signal);
    interval.tv_sec = 120;	/*
				 * 2 minutes
				 */
    interval.tv_usec = 0;
    itime.it_interval = interval;
    itime.it_value = interval;
    setitimer(ITIMER_REAL, &itime, 0);
}


int main(int argc, char **argv) {
    char arg[MAX_STRING];

    char *p;

    int i, len;

    pid_t child_pid;

    parent_pid = getppid();
    if (parent_pid == 1) {
        exit(1);
    }
    for (i = 0; i < MAX_CHILDREN; i++) {
        child_pids[i] = -1;
    }
    alarm_signal(SIGALRM);
    signal(SIGCHLD, (void (*)(int))child_signal);
    signal(SIGPIPE, SIG_DFL);

    for (;;) {
        /*
         * Find an empty child process slot, or wait until one is
         * * available. Otherwise there's no point in reading in a
         * * request yet.
         */
        do {
            /*
             * see if any children have exited
             */
            while ((child_pid = waitpid(0, NULL, WNOHANG)) > 0) {
                for (i = 0; i < MAX_CHILDREN; i++) {
                    if (child_pids[i] == child_pid) {
                        child_pids[i] = -1;
                        break;
                    }
                }
            }
            /*
             * look for an available child process slot
             */
            for (i = 0; i < MAX_CHILDREN; i++) {
                child_pid = child_pids[i];
                if (child_pid == -1 ||
                        kill(child_pid, 0) == -1)
                    break;
            }
            if (i < MAX_CHILDREN)
                break;
            /*
             * no slot available, wait for some child to exit
             */
            child_pid = waitpid(0, NULL, 0);
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (child_pids[i] == child_pid) {
                    child_pids[i] = -1;
                    break;
                }
            }
        } while (i == MAX_CHILDREN);

        /*
         * ok, now read a request (blocking if no request is waiting,
         * * and stopping when interrupted by a signal)
         */
        len = read(0, arg, MAX_STRING - 1);
        if (len == 0)
            break;
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        arg[len] = '\0';
        p = strchr(arg, '\n');
        if (p)
            *p = '\0';

        child_pid = fork();
        switch (child_pid) {
        case -1:
            exit(1);

        case 0:	/*
				 * child
				 */
        {
            /*
             * we don't want to try this for more than 5
             * minutes
             */
            struct itimerval itime;

            struct timeval interval;

            interval.tv_sec = 300;	/*
* 5 minutes
*/
            interval.tv_usec = 0;
            itime.it_interval = interval;
            itime.it_value = interval;
            signal(SIGALRM,
                   (void (*)(int))child_timeout_signal);
            setitimer(ITIMER_REAL, &itime, 0);
        }
        exit(query(arg, p + 1) != 0);

        default:
            /*
             * parent
             */
            child_pids[i] = child_pid;
            break;
        }
    }

    /*
     * wait for any remaining children
     */
    for (i = 0; i < MAX_CHILDREN; i++) {
        child_pid = child_pids[i];
        if (child_pid != -1 && kill(child_pid, 0) != -1) {
            kill(child_pid, SIGKILL);
            waitpid(child_pid, NULL, 0);
        }
    }
    exit(0);
}
