/* bsd.c - BSD-style network and signal routines */
/* $Id: bsd.c,v 1.75 2007/10/04 22:58:20 lwl Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs.h */
#include "externs.h"		/* required by interface.h */
#include "interface.h"		/* required by code */

#include "file_c.h"		/* required by code */
#include "command.h"		/* required by code */
#include "attrs.h"		/* required by code */

#ifndef NSIG
extern const int _sys_nsig;

#define NSIG _sys_nsig
#endif

extern void NDECL(dispatch);

int sock;

int ndescriptors = 0;

int maxd = 0;

DESC *descriptor_list = NULL;

volatile pid_t slave_pid = 0;

volatile int slave_socket = -1;

DESC *FDECL(initializesock, (int, struct sockaddr_in *));

DESC *FDECL(new_connection, (int));

int FDECL(process_output, (DESC *));

int FDECL(process_input, (DESC *));

/*
 * Some systems are lame, and inet_addr() returns -1 on failure, despite the
 * fact that it returns an unsigned long.
 */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

/*
 * get a result from the slave
 */
#define GSR_SKIP_WHITESPACE(x) \
	while (isspace(*(x))) \
		++(x)

#define GSR_REQUIRE_CHAR(x,c) \
	if (*(x) != (c)) { \
		goto gsr_end; \
	} \
	++(x)

#define GSR_STRCHR_INC(x,y,c) \
	(x) = strchr((y), (c)); \
	if (!(x)) { \
		goto gsr_end; \
	} \
	++(x)

static int
get_slave_result()
{
    char *buf, *host1, *hostname, *host2, *p, *userid;

    int remote_port, len;

    unsigned long addr;

    DESC *d;

    buf = alloc_lbuf("slave_buf");

    len = read(slave_socket, buf, LBUF_SIZE - 1);
    if (len < 0)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            free_lbuf(buf);
            return (-1);
        }
        close(slave_socket);
        slave_socket = -1;
        free_lbuf(buf);
        return (-1);
    }
    else if (len == 0)
    {
        free_lbuf(buf);
        return (-1);
    }
    buf[len] = '\0';

    host1 = buf;

    GSR_STRCHR_INC(hostname, host1, ' ');
    hostname[-1] = '\0';

    GSR_STRCHR_INC(host2, hostname, '\n');
    host2[-1] = '\0';

    if (mudconf.use_hostname)
    {
        for (d = descriptor_list; d; d = d->next)
        {
            if (strcmp(d->addr, host1))
                continue;
            if (d->player != 0)
            {
                if (d->username[0])
                {
                    atr_add_raw(d->player, A_LASTSITE,
                                tprintf("%s@%s",
                                        d->username, hostname));
                }
                else
                {
                    atr_add_raw(d->player, A_LASTSITE,
                                hostname);
                }
            }
            strncpy(d->addr, hostname, 50);
            d->addr[50] = '\0';
        }
    }
    GSR_STRCHR_INC(p, host2, ' ');
    p[-1] = '\0';

    addr = inet_addr(host2);
    if (addr == INADDR_NONE)
    {
        goto gsr_end;
    }
    /*
     * now we're into the RFC 1413 ident reply
     */
    GSR_SKIP_WHITESPACE(p);
    remote_port = 0;
    while (isdigit(*p))
    {
        remote_port <<= 1;
        remote_port += (remote_port << 2) + (*p & 0x0f);
        ++p;
    }

    GSR_SKIP_WHITESPACE(p);
    GSR_REQUIRE_CHAR(p, ',');
    GSR_SKIP_WHITESPACE(p);

    /*
     * skip the local port, making sure it consists of digits
     */
    while (isdigit(*p))
    {
        ++p;
    }

    GSR_SKIP_WHITESPACE(p);
    GSR_REQUIRE_CHAR(p, ':');
    GSR_SKIP_WHITESPACE(p);

    /*
     * identify the reply type
     */
    if (strncmp(p, "USERID", 6))
    {
        /*
         * the other standard possibility here is "ERROR"
         */
        goto gsr_end;
    }
    p += 6;

    GSR_SKIP_WHITESPACE(p);
    GSR_REQUIRE_CHAR(p, ':');
    GSR_SKIP_WHITESPACE(p);

    /*
     * don't include the trailing linefeed in the userid
     */
    GSR_STRCHR_INC(userid, p, '\n');
    userid[-1] = '\0';

    /*
     * go back and skip over the "OS [, charset] : " field
     */
    GSR_STRCHR_INC(userid, p, ':');
    GSR_SKIP_WHITESPACE(userid);

    for (d = descriptor_list; d; d = d->next)
    {
        if (ntohs((d->address).sin_port) != remote_port)
            continue;
        if ((d->address).sin_addr.s_addr != addr)
            continue;
        if (d->player != 0)
        {
            if (mudconf.use_hostname)
            {
                atr_add_raw(d->player, A_LASTSITE,
                            tprintf("%s@%s", userid, hostname));
            }
            else
            {
                atr_add_raw(d->player, A_LASTSITE,
                            tprintf("%s@%s", userid, host2));
            }
        }
        strncpy(d->username, userid, 10);
        d->username[10] = '\0';
        break;
    }
gsr_end:
    free_lbuf(buf);
    return 0;
}

void
boot_slave()
{
    int sv[2];

    int i;

    int maxfds;

    char *s;

#ifdef HAVE_GETDTABLESIZE
    maxfds = getdtablesize();
#else
    maxfds = sysconf(_SC_OPEN_MAX);
#endif

    if (slave_socket != -1)
    {
        close(slave_socket);
        slave_socket = -1;
    }
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) < 0)
    {
        return;
    }
    /*
     * set to nonblocking
     */
#ifdef FNDELAY
    if (fcntl(sv[0], F_SETFL, FNDELAY) == -1)
    {
#else
    if (fcntl(sv[0], F_SETFL, O_NDELAY) == -1)
    {
#endif
        close(sv[0]);
        close(sv[1]);
        return;
    }
    slave_pid = vfork();
    switch (slave_pid)
    {
    case -1:
        close(sv[0]);
        close(sv[1]);
        return;

    case 0:		/* child */
        close(sv[0]);
        if (dup2(sv[1], 0) == -1)
        {
            _exit(1);
        }
        if (dup2(sv[1], 1) == -1)
        {
            _exit(1);
        }
        for (i = 3; i < maxfds; ++i)
        {
            close(i);
        }
        s = (char *)XMALLOC(MBUF_SIZE, "boot_slave");
        sprintf(s, "%s/slave", mudconf.binhome);
        execlp(s, "slave", NULL);
        XFREE(s, "boot_slave");
        _exit(1);
    }
    close(sv[1]);

#ifdef FNDELAY
    if (fcntl(sv[0], F_SETFL, FNDELAY) == -1)
    {
#else
    if (fcntl(sv[0], F_SETFL, O_NDELAY) == -1)
    {
#endif
        close(sv[0]);
        return;
    }
    slave_socket = sv[0];
    STARTLOG(LOG_ALWAYS, "NET", "SLAVE")
    log_printf("DNS lookup slave started on fd %d", slave_socket);
    ENDLOG
}

int
make_socket(port)
int port;
{
    int s, opt;

    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        log_perror("NET", "FAIL", NULL, "creating master socket");
        exit(3);
    }
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&opt, sizeof(opt)) < 0)
    {
        log_perror("NET", "FAIL", NULL, "setsockopt");
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = (unsigned short)htons((unsigned short)port);
    if (!mudstate.restarting)
        if (bind(s, (struct sockaddr *)&server, sizeof(server)))
        {
            log_perror("NET", "FAIL", NULL, "bind");
            close(s);
            exit(4);
        }
    listen(s, 5);
    return s;
}

void
shovechars(port)
int port;
{
    fd_set input_set, output_set;

    struct timeval last_slice, current_time, next_slice, timeout,
            slice_timeout;
    int found, check;

    DESC *d, *dnext, *newd;

    int avail_descriptors, maxfds;

    struct stat fstatbuf;

#define CheckInput(x)	FD_ISSET(x, &input_set)
#define CheckOutput(x)	FD_ISSET(x, &output_set)

    mudstate.debug_cmd = (char *)"< shovechars >";
    if (!mudstate.restarting)
{
        sock = make_socket(port);
    }
    if (!mudstate.restarting)
        maxd = sock + 1;

    get_tod(&last_slice);

#ifdef HAVE_GETDTABLESIZE
    maxfds = getdtablesize();
#else
    maxfds = sysconf(_SC_OPEN_MAX);
#endif

    avail_descriptors = maxfds - 7;

    while (mudstate.shutdown_flag == 0)
    {
        get_tod(&current_time);
        last_slice = update_quotas(last_slice, current_time);

        process_commands();

        if (mudstate.shutdown_flag)
            break;

        /*
         * We've gotten a signal to dump flatfiles
         */

        if (mudstate.flatfile_flag && !mudstate.dumping)
        {
            if (*mudconf.dump_msg)
                raw_broadcast(0, "%s", mudconf.dump_msg);

            mudstate.dumping = 1;
            STARTLOG(LOG_DBSAVES, "DMP", "CHKPT")
            log_printf("Flatfiling: %s.#%d#",
                       mudconf.gdbm, mudstate.epoch);
            ENDLOG dump_database_internal(DUMP_DB_FLATFILE);
            mudstate.dumping = 0;

            if (*mudconf.postdump_msg)
                raw_broadcast(0, "%s", mudconf.postdump_msg);
            mudstate.flatfile_flag = 0;
        }
        /*
         * test for events
         */

        dispatch();

        /*
         * any queued robot commands waiting?
         */

        timeout.tv_sec = que_next();
        timeout.tv_usec = 0;
        next_slice = msec_add(last_slice, mudconf.timeslice);
        slice_timeout = timeval_sub(next_slice, current_time);

        FD_ZERO(&input_set);
        FD_ZERO(&output_set);

        /*
         * Listen for new connections if there are free descriptors
         */

        if (ndescriptors < avail_descriptors)
        {
            FD_SET(sock, &input_set);
        }
        /*
         * Listen for replies from the slave socket
         */

        if (slave_socket != -1)
        {
            FD_SET(slave_socket, &input_set);
        }
        /*
         * Mark sockets that we want to test for change in status
         */

        DESC_ITER_ALL(d)
        {
            if (!d->input_head)
                FD_SET(d->descriptor, &input_set);
            if (d->output_head)
                FD_SET(d->descriptor, &output_set);
        }

        /*
         * Wait for something to happen
         */
        found = select(maxd, &input_set, &output_set, (fd_set *) NULL,
                       &timeout);

        if (found < 0)
        {
            if (errno == EBADF)
            {
                /*
                 * This one is bad, as it results in a spiral
                 * of doom, unless we can figure out what the
                 * bad file descriptor is and get rid of it.
                 */
                log_perror("NET", "FAIL",
                           "checking for activity", "select");
                DESC_ITER_ALL(d)
                {
                    if (fstat(d->descriptor,
                              &fstatbuf) < 0)
                    {
                        /*
                         * It's a player. Just toss
                         * the connection.
                         */
                        STARTLOG(LOG_PROBLEMS, "ERR",
                                 "EBADF")
                        log_printf
                        ("Bad descriptor %d",
                         d->descriptor);
                        ENDLOG shutdownsock(d,
                                            R_SOCKDIED);
                    }
                }
                if ((slave_socket == -1) ||
                        (fstat(slave_socket, &fstatbuf) < 0))
                {
                    /*
                     * Try to restart the slave, since it
                     * presumably died.
                     */
                    STARTLOG(LOG_PROBLEMS, "ERR", "EBADF")
                    log_printf
                    ("Bad slave descriptor %d",
                     slave_socket);
                    ENDLOG boot_slave();
                }
                if ((sock != -1) &&
                        (fstat(sock, &fstatbuf) < 0))
                {
                    /*
                     * That's it, game over.
                     */
                    STARTLOG(LOG_PROBLEMS, "ERR", "EBADF")
                    log_printf
                    ("Bad game port descriptor %d",
                     sock);
                    ENDLOG break;
                }
            }
            else if (errno != EINTR)
            {
                log_perror("NET", "FAIL",
                           "checking for activity", "select");
            }
            continue;
        }
        /*
         * if !found then time for robot commands
         */

        if (!found)
        {
            if (mudconf.queue_chunk)
                do_top(mudconf.queue_chunk);
            continue;
        }
        else
        {
            do_top(mudconf.active_q_chunk);
        }

        /*
         * Get usernames and hostnames
         */

        if (slave_socket != -1 && FD_ISSET(slave_socket, &input_set))
        {
            while (get_slave_result() == 0);
        }
        /*
         * Check for new connection requests
         */

        if (CheckInput(sock))
        {
            newd = new_connection(sock);
            if (!newd)
            {
                check = (errno && (errno != EINTR) &&
                         (errno != EMFILE) && (errno != ENFILE));
                if (check)
                {
                    log_perror("NET", "FAIL", NULL,
                               "new_connection");
                }
            }
            else
            {
                if (newd->descriptor >= maxd)
                    maxd = newd->descriptor + 1;
            }
        }
        /*
         * Check for activity on user sockets
         */

        DESC_SAFEITER_ALL(d, dnext)
        {

            /*
             * Process input from sockets with pending input
             */

            if (CheckInput(d->descriptor))
            {

                /*
                 * Undo AutoDark
                 */

                if (d->flags & DS_AUTODARK)
                {
                    d->flags &= ~DS_AUTODARK;
                    s_Flags(d->player,
                            Flags(d->player) & ~DARK);
                }
                /*
                 * Process received data
                 */

                if (!process_input(d))
                {
                    shutdownsock(d, R_SOCKDIED);
                    continue;
                }
            }
            /*
             * Process output for sockets with pending output
             */

            if (CheckOutput(d->descriptor))
            {
                if (!process_output(d))
                {
                    shutdownsock(d, R_SOCKDIED);
                }
            }
        }
    }
}

#ifdef BROKEN_GCC_PADDING
char *
inet_ntoa(in)
struct in_addr in;
{
    /*
     * gcc 2.8.1 does not correctly pass/return structures which are
     * smaller than 16 bytes, but are not 8 bytes. Structures get padded
     * at the wrong end. gcc is consistent with itself, but if you try to
     * link with library files, there's a problem. This particularly
     * affects Irix 6, but also affects other 64-bit targets.
     *
     * There may be little/big-endian problems with this, too.
     */

    static char buf[MBUF_SIZE];

    long a = in.s_addr;

    sprintf(buf, "%d.%d.%d.%d",
            (int)((a >> 24) & 0xff),
            (int)((a >> 16) & 0xff), (int)((a >> 8) & 0xff), (int)(a & 0xff));
    return buf;
}

#endif				/* BROKEN_GCC_PADDING */

DESC *
new_connection(sock)
int sock;
{
    int newsock;

    char *buff, *cmdsave;

    DESC *d;

    struct sockaddr_in addr;

    int addr_len, len;

    char *buf;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *)"< new_connection >";
    addr_len = sizeof(struct sockaddr);

    newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);
    if (newsock < 0)
        return 0;

    if (site_check(addr.sin_addr, mudstate.access_list) & H_FORBIDDEN)
    {
        STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
        log_printf("[%d/%s] Connection refused.  (Remote port %d)",
                   newsock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ENDLOG fcache_rawdump(newsock, FC_CONN_SITE);
        shutdown(newsock, 2);
        close(newsock);
        errno = 0;
        d = NULL;
    }
    else
    {
        buff = alloc_mbuf("new_connection.address");
        buf = alloc_lbuf("new_connection.write");
        strcpy(buff, inet_ntoa(addr.sin_addr));

        /*
         * Ask slave process for host and username
         */
        if ((slave_socket != -1) && mudconf.use_hostname)
        {
            sprintf(buf, "%s\n%s,%d,%d\n",
                    inet_ntoa(addr.sin_addr), inet_ntoa(addr.sin_addr),
                    ntohs(addr.sin_port), mudconf.port);
            len = strlen(buf);
            if (WRITE(slave_socket, buf, len) < 0)
            {
                close(slave_socket);
                slave_socket = -1;
            }
        }
        free_lbuf(buf);
        STARTLOG(LOG_NET, "NET", "CONN")
        log_printf("[%d/%s] Connection opened (remote port %d)",
                   newsock, buff, ntohs(addr.sin_port));
        ENDLOG d = initializesock(newsock, &addr);
        mudstate.debug_cmd = cmdsave;
        free_mbuf(buff);
    }
    mudstate.debug_cmd = cmdsave;
    return (d);
}

/*
 * (Dis)connect reasons that get written to the logfile
 */

const char *conn_reasons[] =
{
    "Unspecified",

    "Guest-connected to",
    "Created",
    "Connected to",
    "Dark-connected to",

    "Quit",
    "Inactivity Timeout",
    "Booted",
    "Remote Close or Net Failure",
    "Game Shutdown",
    "Login Retry Limit",
    "Logins Disabled",
    "Logout (Connection Not Dropped)",
    "Too Many Connected Players"
};

/*
 * (Dis)connect reasons that get fed to A_A(DIS)CONNECT via announce_connattr
 */

const char *conn_messages[] =
{
    "unknown",

    "guest",
    "create",
    "connect",
    "cd",

    "quit",
    "timeout",
    "boot",
    "netdeath",
    "shutdown",
    "badlogin",
    "nologins",
    "logout"
};

void
shutdownsock(d, reason)
DESC *d;

int reason;
{
    char *buff2;

    time_t now;

    int ncon;

    DESC *dtemp;

    if ((reason == R_LOGOUT) &&
            (site_check((d->address).sin_addr,
                        mudstate.access_list) & H_FORBIDDEN))
        reason = R_QUIT;

    if (d->flags & DS_CONNECTED)
    {

        /*
         * Do the disconnect stuff if we aren't doing a LOGOUT (which
         * keeps the connection open so the player can connect to a
         * different character).
         */

        if (reason != R_LOGOUT)
        {
            if (reason != R_SOCKDIED)
            {
                /*
                 * If the socket died, there's no reason to
                 * display the quit file.
                 */
                fcache_dump(d, FC_QUIT);
            }
            STARTLOG(LOG_NET | LOG_LOGIN, "NET", "DISC")
            log_printf("[%d/%s] Logout by ",
                       d->descriptor, d->addr);
            log_name(d->player);
            log_printf
            (" <%s: %d cmds, %d bytes in, %d bytes out, %d secs>",
             conn_reasons[reason], d->command_count,
             d->input_tot, d->output_tot,
             (int)(time(NULL) - d->connected_at));
            ENDLOG
        }
        else
        {
            STARTLOG(LOG_NET | LOG_LOGIN, "NET", "LOGO")
            log_printf("[%d/%s] Logout by ",
                       d->descriptor, d->addr);
            log_name(d->player);
            log_printf
            (" <%s: %d cmds, %d bytes in, %d bytes out, %d secs>",
             conn_reasons[reason], d->command_count,
             d->input_tot, d->output_tot,
             (int)(time(NULL) - d->connected_at));
            ENDLOG
        }

        /*
         * If requested, write an accounting record of the form:
         * Plyr# Flags Cmds ConnTime Loc Money [Site] <DiscRsn> Name
         */

        STARTLOG(LOG_ACCOUNTING, "DIS", "ACCT")
        now = mudstate.now - d->connected_at;
        buff2 = unparse_flags(GOD, d->player);
        log_printf("%d %s %d %d %d %d [%s] <%s> ",
                   d->player, buff2, d->command_count, (int)now,
                   Location(d->player), Pennies(d->player),
                   d->addr, conn_reasons[reason]);
        log_name(d->player);
        free_sbuf(buff2);
        ENDLOG
        announce_disconnect(d->player, d, conn_messages[reason]);
    }
    else
    {
        if (reason == R_LOGOUT)
            reason = R_QUIT;
        STARTLOG(LOG_SECURITY | LOG_NET, "NET", "DISC")
        log_printf
        ("[%d/%s] Connection closed, never connected. <Reason: %s>",
         d->descriptor, d->addr, conn_reasons[reason]);
        ENDLOG
    }
    process_output(d);
    clearstrings(d);
    /*
     * If this was our only connection, get out of interactive mode.
     */
    if (d->program_data)
    {
        ncon = 0;
        DESC_ITER_PLAYER(d->player, dtemp) ncon++;
        if (ncon == 0)
        {
            Free_RegData(d->program_data->wait_data);
            XFREE(d->program_data, "do_prog");
            atr_clr(d->player, A_PROGCMD);
        }
        d->program_data = NULL;
    }
    if (d->colormap)
    {
        XFREE(d->colormap, "colormap");
        d->colormap = NULL;
    }
    if (reason == R_LOGOUT)
    {
        d->flags &= ~DS_CONNECTED;
        d->connected_at = time(NULL);
        d->retries_left = mudconf.retry_limit;
        d->command_count = 0;
        d->timeout = mudconf.idle_timeout;
        d->player = 0;
        d->doing[0] = '\0';
        d->quota = mudconf.cmd_quota_max;
        d->last_time = 0;
        d->host_info = site_check((d->address).sin_addr,
                                  mudstate.access_list) |
                       site_check((d->address).sin_addr, mudstate.suspect_list);
        d->input_tot = d->input_size;
        d->output_tot = 0;
        welcome_user(d);
    }
    else
    {
        shutdown(d->descriptor, 2);
        close(d->descriptor);
        freeqs(d);
        *d->prev = d->next;
        if (d->next)
            d->next->prev = d->prev;
        free_desc(d);
        ndescriptors--;
    }
}

void
make_nonblocking(s)
int s;
{
#ifdef HAVE_LINGER
    struct linger ling;

#endif

#ifdef FNDELAY
    if (fcntl(s, F_SETFL, FNDELAY) == -1)
    {
        log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
    }
#else
    if (fcntl(s, F_SETFL, O_NDELAY) == -1)
    {
        log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
    }
#endif
#ifdef HAVE_LINGER
    ling.l_onoff = 0;
    ling.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER,
                   (char *)&ling, sizeof(ling)) < 0)
    {
        log_perror("NET", "FAIL", "linger", "setsockopt");
    }
#endif
}

DESC *
initializesock(s, a)
int s;

struct sockaddr_in *a;
{
    DESC *d;

    if (s == slave_socket)
    {
        /*
         * Whoa. We shouldn't be allocating this. If we got this
         * descriptor, our connection with the slave must have died
         * somehow. We make sure to take note appropriately.
         */
        STARTLOG(LOG_ALWAYS, "ERR", "SOCK")
        log_printf("Player descriptor clashes with slave fd %d",
                   slave_socket);
        ENDLOG slave_socket = -1;
    }
    ndescriptors++;
    d = alloc_desc("init_sock");
    d->descriptor = s;
    d->flags = 0;
    d->connected_at = time(NULL);
    d->retries_left = mudconf.retry_limit;
    d->command_count = 0;
    d->timeout = mudconf.idle_timeout;
    d->host_info = site_check((*a).sin_addr, mudstate.access_list) |
                   site_check((*a).sin_addr, mudstate.suspect_list);
    d->player = 0;		/* be sure #0 isn't wizard.  Shouldn't be. */

    d->addr[0] = '\0';
    d->doing[0] = '\0';
    d->username[0] = '\0';
    d->colormap = NULL;
    make_nonblocking(s);
    d->output_prefix = NULL;
    d->output_suffix = NULL;
    d->output_size = 0;
    d->output_tot = 0;
    d->output_lost = 0;
    d->output_head = NULL;
    d->output_tail = NULL;
    d->input_head = NULL;
    d->input_tail = NULL;
    d->input_size = 0;
    d->input_tot = 0;
    d->input_lost = 0;
    d->raw_input = NULL;
    d->raw_input_at = NULL;
    d->quota = mudconf.cmd_quota_max;
    d->program_data = NULL;
    d->last_time = 0;
    d->address = *a;	/* added 5/3/90 SCG */
    if (descriptor_list)
        descriptor_list->prev = &d->next;
    d->hashnext = NULL;
    d->next = descriptor_list;
    d->prev = &descriptor_list;
    strncpy(d->addr, inet_ntoa(a->sin_addr), 50);
    descriptor_list = d;
    welcome_user(d);
    return d;
}

int
process_output(d)
DESC *d;
{
    TBLOCK *tb, *save;

    int cnt;

    char *cmdsave;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *)"< process_output >";

    tb = d->output_head;

    while (tb != NULL)
    {
        while (tb->hdr.nchars > 0)
        {
            cnt = WRITE(d->descriptor, tb->hdr.start,
                        tb->hdr.nchars);
            if (cnt < 0)
            {
                mudstate.debug_cmd = cmdsave;
                if (errno == EWOULDBLOCK)
                    return 1;
                return 0;
            }
            d->output_size -= cnt;
            tb->hdr.nchars -= cnt;
            tb->hdr.start += cnt;
        }
        save = tb;
        tb = tb->hdr.nxt;
        XFREE(save, "queue_write");
        d->output_head = tb;
        if (tb == NULL)
            d->output_tail = NULL;
    }

    mudstate.debug_cmd = cmdsave;
    return 1;
}

int
process_input(d)
DESC *d;
{
    char *buf;

    int got, in, lost;

    char *p, *pend, *q, *qend;

    char *cmdsave;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *)"< process_input >";
    buf = alloc_lbuf("process_input.buf");

    got = in = READ(d->descriptor, buf, LBUF_SIZE);
    if (got <= 0)
    {
        mudstate.debug_cmd = cmdsave;
        free_lbuf(buf);
        return 0;
    }
    if (!d->raw_input)
    {
        d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
        d->raw_input_at = d->raw_input->cmd;
    }
    p = d->raw_input_at;
    pend = d->raw_input->cmd - sizeof(CBLKHDR) - 1 + LBUF_SIZE;
    lost = 0;
    for (q = buf, qend = buf + got; q < qend; q++)
    {
        if (*q == '\n')
        {
            *p = '\0';
            if (p > d->raw_input->cmd)
            {
                save_command(d, d->raw_input);
                d->raw_input =
                    (CBLK *) alloc_lbuf("process_input.raw");

                p = d->raw_input_at = d->raw_input->cmd;
                pend =
                    d->raw_input->cmd - sizeof(CBLKHDR) - 1 +
                    LBUF_SIZE;
            }
            else
            {
                in -= 1;	/* for newline */
            }
        }
        else if ((*q == '\b') || (*q == 127))
        {
            if (*q == 127)
                queue_string(d, "\b \b");
            else
                queue_string(d, " \b");
            in -= 2;
            if (p > d->raw_input->cmd)
                p--;
            if (p < d->raw_input_at)
                (d->raw_input_at)--;
        }
        else if (p < pend && isascii(*q) && isprint(*q))
        {
            *p++ = *q;
        }
        else
        {
            in--;
            if (p >= pend)
                lost++;
        }
    }
    if (in < 0)		/* backspace and delete by themselves */
        in = 0;
    if (p > d->raw_input->cmd)
    {
        d->raw_input_at = p;
    }
    else
    {
        free_lbuf(d->raw_input);
        d->raw_input = NULL;
        d->raw_input_at = NULL;
    }
    d->input_tot += got;
    d->input_size += in;
    d->input_lost += lost;
    free_lbuf(buf);
    mudstate.debug_cmd = cmdsave;
    return 1;
}

void
close_sockets(emergency, message)
int emergency;

char *message;
{
    DESC *d, *dnext;

    DESC_SAFEITER_ALL(d, dnext)
    {
        if (emergency)
        {

            WRITE(d->descriptor, message, strlen(message));
            if (shutdown(d->descriptor, 2) < 0)
                log_perror("NET", "FAIL", NULL, "shutdown");
            close(d->descriptor);
        }
        else
        {
            queue_string(d, message);
            queue_write(d, "\r\n", 2);
            shutdownsock(d, R_GOING_DOWN);
        }
    }
    close(sock);
}

void
NDECL(emergency_shutdown)
{
    close_sockets(1, (char *)"Going down - Bye");
}

/*
 * ---------------------------------------------------------------------------
 * Print out stuff into error file.
 */

void
NDECL(report)
{
    STARTLOG(LOG_BUGS, "BUG", "INFO")
    log_printf("Command: '%s'", mudstate.debug_cmd);
    ENDLOG if (Good_obj(mudstate.curr_player))
    {
        STARTLOG(LOG_BUGS, "BUG", "INFO")
        log_printf("Player: ");
        log_name_and_loc(mudstate.curr_player);
        if ((mudstate.curr_enactor != mudstate.curr_player) &&
                Good_obj(mudstate.curr_enactor))
        {
            log_printf(" Enactor: ");
            log_name_and_loc(mudstate.curr_enactor);
        }
        ENDLOG
    }
}

/*
 * ---------------------------------------------------------------------------
 * * Signal handling routines.
 */

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

static RETSIGTYPE FDECL(sighandler, (int));

/* *INDENT-OFF* */

NAMETAB		sigactions_nametab[] =
{
    {(char *)"exit", 3, 0, SA_EXIT},
    {(char *)"default", 1, 0, SA_DFLT},
    {NULL, 0, 0, 0}
};

/* *INDENT-ON* */

void
NDECL(set_signals)
{
    sigset_t sigs;

    /*
     * We have to reset our signal mask, because of the possibility that
     * we triggered a restart on a SIGUSR1. If we did so, then the signal
     * became blocked, and stays blocked, since control never returns to
     * the caller; i.e., further attempts to send a SIGUSR1 would fail.
     */
    sigfillset(&sigs);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);

    signal(SIGALRM, sighandler);
    signal(SIGCHLD, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR1, sighandler);
    signal(SIGUSR2, sighandler);
    signal(SIGTRAP, sighandler);
#ifdef SIGXCPU
    signal(SIGXCPU, sighandler);
#endif

    signal(SIGFPE, SIG_IGN);

    signal(SIGILL, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGABRT, sighandler);
#ifdef SIGFSZ
    signal(SIGXFSZ, sighandler);
#endif
#ifdef SIGEMT
    signal(SIGEMT, sighandler);
#endif
#ifdef SIGBUS
    signal(SIGBUS, sighandler);
#endif
#ifdef SIGSYS
    signal(SIGSYS, sighandler);
#endif

}

static void
unset_signals()
{
    int i;

    for (i = 0; i < NSIG; i++)
        signal(i, SIG_DFL);
}

static void
check_panicking(sig)
int sig;
{
    int i;

    /*
     * If we are panicking, turn off signal catching and resignal
     */

    if (mudstate.panicking)
    {
        for (i = 0; i < NSIG; i++)
            signal(i, SIG_DFL);
        kill(getpid(), sig);
    }
    mudstate.panicking = 1;
}

void
log_signal(signame)
const char *signame;
{
    STARTLOG(LOG_PROBLEMS, "SIG", "CATCH")
    log_printf("Caught signal %s", signame);
    ENDLOG
}

static RETSIGTYPE
sighandler(sig)
int sig;
{
#ifdef HAVE_SYS_SIGNAME
#define signames sys_signame
#else
#ifdef SYS_SIGLIST_DECLARED
#define signames sys_siglist
#else
    static const char *signames[] =
    {
        "SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT",
        "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT",
        "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV",
        "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM",
        "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT",
        "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO",
        "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF",
        "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2"
    };

#endif				/* SYS_SIGLIST_DECLARED */
#endif				/* HAVE_SYS_SIGNAME */

    int i;

    pid_t child;

#if defined(HAVE_UNION_WAIT) && defined(NEED_WAIT3_DCL)
    union wait stat;

#else
    int stat;

#endif

    switch (sig)
    {
    case SIGUSR1:		/* Normal restart now */
        log_signal(signames[sig]);
        do_restart(GOD, GOD, 0);
        break;
    case SIGUSR2:		/* Dump a flatfile soon */
        mudstate.flatfile_flag = 1;
        break;
    case SIGALRM:		/* Timer */
        mudstate.alarm_triggered = 1;
        break;
    case SIGCHLD:		/* Change in child status */
#ifndef SIGNAL_SIGCHLD_BRAINDAMAGE
        signal(SIGCHLD, sighandler);
#endif
        while ((child = WAITOPT(&stat, WNOHANG)) > 0)
        {
            if (mudconf.fork_dump && mudstate.dumping &&
                    child == mudstate.dumper &&
                    (WIFEXITED(stat) || WIFSIGNALED(stat)))
            {
                mudstate.dumping = 0;
                mudstate.dumper = 0;
            }
            else if (child == slave_pid &&
                     (WIFEXITED(stat) || WIFSIGNALED(stat)))
            {
                slave_pid = 0;
                slave_socket = -1;
            }
        }
        break;
    case SIGHUP:		/* Dump database soon */
        log_signal(signames[sig]);
        mudstate.dump_counter = 0;
        break;
    case SIGINT:		/* Log + ignore */
        log_signal(signames[sig]);
        break;
    case SIGQUIT:		/* Normal shutdown soon */
        mudstate.shutdown_flag = 1;
        break;
    case SIGTERM:		/* Killed shutdown now */
#ifdef SIGXCPU
    case SIGXCPU:
#endif
        check_panicking(sig);
        log_signal(signames[sig]);
        raw_broadcast(0, "GAME: Caught signal %s, exiting.",
                      signames[sig]);
        dump_database_internal(DUMP_DB_KILLED);
        exit(0);
        break;
    case SIGILL:		/* Panic save + restart now, or coredump now */
    case SIGFPE:
    case SIGSEGV:
    case SIGTRAP:
#ifdef SIGXFSZ
    case SIGXFSZ:
#endif
#ifdef SIGEMT
    case SIGEMT:
#endif
#ifdef SIGBUS
    case SIGBUS:
#endif
#ifdef SIGSYS
    case SIGSYS:
#endif
        check_panicking(sig);
        log_signal(signames[sig]);
        report();
        if (mudconf.sig_action != SA_EXIT)
        {

            raw_broadcast(0,
                          "GAME: Fatal signal %s caught, restarting with previous database.",
                          signames[sig]);

            /*
             * Don't sync first. Using older db.
             */

            dump_database_internal(DUMP_DB_CRASH);
            CLOSE;
            if (slave_socket != -1)
            {
                shutdown(slave_socket, 2);
                close(slave_socket);
                slave_socket = -1;
            }
            if (slave_pid != 0)
            {
                kill(slave_pid, SIGKILL);
            }
            /*
             * Try our best to dump a usable core by generating a
             * second signal with the SIG_DFL action.
             */

            if (fork() > 0)
            {
                unset_signals();

                /*
                 * In the parent process (easier to follow
                 * with gdb), we're about to return from this
                 * signal handler and hope that a second
                 * signal is delivered. Meanwhile let's close
                 * all our files to avoid corrupting the
                 * child process.
                 */
                for (i = 0; i < maxd; i++)
                    close(i);

                return;
            }
            alarm(0);
            dump_restart_db();
            execl(mudconf.exec_path, mudconf.exec_path,
                  (char *)"-c", mudconf.config_file,
                  (char *)"-l", mudconf.mudlogname,
                  (char *)"-p", mudconf.pid_file,
                  (char *)"-t", mudconf.txthome,
                  (char *)"-b", mudconf.binhome,
                  (char *)"-d", mudconf.dbhome,
                  (char *)"-g", mudconf.gdbm,
                  (char *)"-k", mudconf.crashdb, NULL);
            break;
        }
        else
        {
            unset_signals();
            fprintf(mainlog_fp,
                    "ABORT! bsd.c, SA_EXIT requested.\n");
            abort();
        }
    case SIGABRT:		/* Coredump now */
        check_panicking(sig);
        log_signal(signames[sig]);
        report();

        unset_signals();
        fprintf(mainlog_fp, "ABORT! bsd.c, SIGABRT received.\n");
        abort();
    }
    signal(sig, sighandler);
    mudstate.panicking = 0;
    return;
}
