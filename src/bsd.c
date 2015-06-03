/* bsd.c - BSD-style network and signal routines */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mushconf */
#include "game.h"		/* required by mushconf */
#include "alloc.h"		/* required by mushconf */
#include "flags.h"		/* required by mushconf */
#include "htab.h"		/* required by mushconf */
#include "ltdl.h"		/* required by mushconf */
#include "udb.h"		/* required by mushconf */
#include "udb_defs.h"	/* required by mushconf */

#include "mushconf.h"	/* required by code */

#include "db.h"			/* required by externs.h */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by interface.h */

#include "file_c.h"		/* required by code */
#include "command.h"	/* required by code */
#include "attrs.h"		/* required by code */

#ifndef NSIG
extern const int _sys_nsig;

#define NSIG _sys_nsig
#endif

extern void dispatch(void);
int sock;
int ndescriptors = 0;
int maxd = 0;
DESC *descriptor_list = NULL;
volatile pid_t slave_pid = 0;
volatile int slave_socket = -1;
DESC *initializesock(int, struct sockaddr_in *);
DESC *new_connection(int);
int process_output(DESC *);
int process_input(DESC *);

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

static int get_slave_result(void) {
	char *buf, *host1, *hostname, *host2, *p, *userid;
	int remote_port, len;
	unsigned long addr;
	DESC *d;
	char s[MBUF_SIZE];
	buf = alloc_lbuf("slave_buf");
	len = read(slave_socket, buf, LBUF_SIZE - 1);

	if (len < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			free_lbuf(buf);
			return (-1);
		}

		close(slave_socket);
		slave_socket = -1;
		free_lbuf(buf);
		return (-1);
	} else if (len == 0) {
		free_lbuf(buf);
		return (-1);
	}

	buf[len] = '\0';
	host1 = buf;
	hostname = strchr(host1, ' ');
	if (!hostname) {
		goto gsr_end;
	}
	++hostname;
	hostname[-1] = '\0';
	host2 = strchr(hostname, '\n');
	if (!host2) {
		goto gsr_end;
	}
	++host2;

	host2[-1] = '\0';

	if (mudconf.use_hostname) {
		for (d = descriptor_list; d; d = d->next) {
			if (strcmp(d->addr, host1)) {
				continue;
			}

			if (d->player != 0) {
				if (d->username[0]) {
					snprintf(s, MBUF_SIZE, "%s@%s", d->username, hostname);
					atr_add_raw(d->player, A_LASTSITE, s);
				} else {
					atr_add_raw(d->player, A_LASTSITE, hostname);
				}
			}

			strncpy(d->addr, hostname, 50);
			d->addr[50] = '\0';
		}
	}

	p = strchr(host2, ' ');
	if (!p) {
		goto gsr_end;
	}
	++p;
	p[-1] = '\0';
	addr = inet_addr(host2);

	if (addr == INADDR_NONE) {
		goto gsr_end;
	}

	/*
	 * now we're into the RFC 1413 ident reply
	 */

	while (isspace(*p))
		++p;
	remote_port = 0;

	while (isdigit(*p)) {
		remote_port <<= 1;
		remote_port += (remote_port << 2) + (*p & 0x0f);
		++p;
	}

	while (isspace(*p))
		++p;
	if (*p != ',') {
		goto gsr_end;
	}
	++p;

	while (isspace(*p))
		++p;

	/*
	 * skip the local port, making sure it consists of digits
	 */

	while (isdigit(*p)) {
		++p;
	}

	while (isspace(*p))
		++p;
	if (*p != ':') {
		goto gsr_end;
	}
	++p;
	while (isspace(*p))
		++p;

	/*
	 * identify the reply type
	 */

	if (strncmp(p, "USERID", 6)) {

		/*
		 * the other standard possibility here is "ERROR"
		 */

		goto gsr_end;
	}

	p += 6;

	while (isspace(*p))
		++p;
	if (*p != ':') {
		goto gsr_end;
	}
	++p;
	while (isspace(*p))
		++p;

	/*
	 * don't include the trailing linefeed in the userid
	 */

	userid = strchr(p, '\n');
	if (!userid) {
		goto gsr_end;
	}
	++userid;
	userid[-1] = '\0';

	/*
	 * go back and skip over the "OS [, charset] : " field
	 */

	userid = strchr(p, ':');
	if (!userid) {
		goto gsr_end;
	}
	++userid;

	while (isspace(*userid))
		++userid;

	for (d = descriptor_list; d; d = d->next) {
		if (ntohs((d->address).sin_port) != remote_port) {
			continue;
		}

		if ((d->address).sin_addr.s_addr != addr) {
			continue;
		}

		if (d->player != 0) {
			if (mudconf.use_hostname) {
				snprintf(s, MBUF_SIZE, "%s@%s", userid, hostname);
				atr_add_raw(d->player, A_LASTSITE, s);
			} else {
				snprintf(s, MBUF_SIZE, "%s@%s", userid, host2);
				atr_add_raw(d->player, A_LASTSITE, s);
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

/* XXX Slave should be a completly separated exec... We end up with two netmush in memory like it is now! */

void boot_slave(void) {
	int sv[2];
	int i;
	int maxfds;
	char *s;
#ifdef HAVE_GETDTABLESIZE
	maxfds = getdtablesize();
#else
	maxfds = sysconf(_SC_OPEN_MAX);
#endif

	if (slave_socket != -1) {
		close(slave_socket);
		slave_socket = -1;
	}

	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) < 0) {
		return;
	}

	/*
	 * set to nonblocking
	 */
#ifdef FNDELAY
	if (fcntl(sv[0], F_SETFL, FNDELAY) == -1) {
#else
		if (fcntl(sv[0], F_SETFL, O_NDELAY) == -1) {
#endif
		close(sv[0]);
		close(sv[1]);
		return;
	}

	slave_pid = vfork();

	switch (slave_pid) {
	case -1:
		close(sv[0]);
		close(sv[1]);
		return;

	case 0: /* child */
		close(sv[0]);

		if (dup2(sv[1], 0) == -1) {
			_exit(EXIT_FAILURE);
		}

		if (dup2(sv[1], 1) == -1) {
			_exit(EXIT_FAILURE);
		}

		for (i = 3; i < maxfds; ++i) {
			close(i);
		}

		s = (char *) xmalloc(MBUF_SIZE, "boot_slave");
		sprintf(s, "%s/slave", mudconf.binhome);
		execlp(s, "slave", NULL);
		xfree(s, "boot_slave");
		_exit(EXIT_FAILURE);
	}

	close(sv[1]);
#ifdef FNDELAY

	if (fcntl(sv[0], F_SETFL, FNDELAY) == -1) {
#else

		if (fcntl(sv[0], F_SETFL, O_NDELAY) == -1) {
#endif
		close(sv[0]);
		return;
	}

	slave_socket = sv[0];
	log_write(LOG_ALWAYS, "NET", "SLAVE", "DNS lookup slave started on fd %d", slave_socket);
}

int make_socket(int port) {
	int s, opt;
	struct sockaddr_in server;
	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		log_perror("NET", "FAIL", NULL, "creating master socket");
		exit(EXIT_FAILURE);
	}

	opt = 1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		log_perror("NET", "FAIL", NULL, "setsockopt");
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = (unsigned short) htons((unsigned short) port);

	if (!mudstate.restarting)
		if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
			log_perror("NET", "FAIL", NULL, "bind");
			close(s);
			exit(EXIT_FAILURE);
		}

	listen(s, 5);
	return s;
}

void shovechars(int port) {
	fd_set input_set, output_set;
	struct timeval last_slice, current_time, next_slice, timeout, slice_timeout;
	int found, check;
	DESC *d, *dnext, *newd;
	int avail_descriptors, maxfds;
	struct stat fstatbuf;

	mudstate.debug_cmd = (char *) "< shovechars >";

	if (!mudstate.restarting) {
		sock = make_socket(port);
	}

	if (!mudstate.restarting) {
		maxd = sock + 1;
	}

	get_tod(&last_slice);
#ifdef HAVE_GETDTABLESIZE
	maxfds = getdtablesize();
#else
	maxfds = sysconf(_SC_OPEN_MAX);
#endif
	avail_descriptors = maxfds - 7;

	while (mudstate.shutdown_flag == 0) {
		get_tod(&current_time);
		last_slice = update_quotas(last_slice, current_time);
		process_commands();

		if (mudstate.shutdown_flag) {
			break;
		}

		/*
		 * We've gotten a signal to dump flatfiles
		 */

		if (mudstate.flatfile_flag && !mudstate.dumping) {
			if (*mudconf.dump_msg) {
				raw_broadcast(0, "%s", mudconf.dump_msg);
			}

			mudstate.dumping = 1;
			log_write(LOG_DBSAVES, "DMP", "CHKPT", "Flatfiling: %s.#%d#", mudconf.db_file, mudstate.epoch);
			dump_database_internal(DUMP_DB_FLATFILE);
			mudstate.dumping = 0;

			if (*mudconf.postdump_msg) {
				raw_broadcast(0, "%s", mudconf.postdump_msg);
			}

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

		if (ndescriptors < avail_descriptors) {
			FD_SET(sock, &input_set);
		}

		/*
		 * Listen for replies from the slave socket
		 */

		if (slave_socket != -1) {
			FD_SET(slave_socket, &input_set);
		}

		/*
		 * Mark sockets that we want to test for change in status
		 */

		DESC_ITER_ALL(d)
		{
			if (!d->input_head) {
				FD_SET(d->descriptor, &input_set);
			}

			if (d->output_head) {
				FD_SET(d->descriptor, &output_set);
			}
		}

		/*
		 * Wait for something to happen
		 */

		found = select(maxd, &input_set, &output_set, (fd_set *) NULL, &timeout);

		if (found < 0) {
			if (errno == EBADF) {

				/*
				 * This one is bad, as it results in a spiral
				 * of doom, unless we can figure out what the
				 * bad file descriptor is and get rid of it.
				 */

				log_perror("NET", "FAIL", "checking for activity", "select");
				DESC_ITER_ALL(d)
				{
					if (fstat(d->descriptor, &fstatbuf) < 0) {

						/*
						 * It's a player. Just toss
						 * the connection.
						 */

						log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad descriptor %d", d->descriptor);
						shutdownsock(d, R_SOCKDIED);
					}
				}

				if ((slave_socket == -1) || (fstat(slave_socket, &fstatbuf) < 0)) {

					/*
					 * Try to restart the slave, since it
					 * presumably died.
					 */

					log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad slave descriptor %d", slave_socket);
					boot_slave();
				}

				if ((sock != -1) && (fstat(sock, &fstatbuf) < 0)) {

					/*
					 * That's it, game over.
					 */

					log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad game port descriptor %d", sock);
					break;
				}
			} else if (errno != EINTR) {
				log_perror("NET", "FAIL", "checking for activity", "select");
			}

			continue;
		}

		/*
		 * if !found then time for robot commands
		 */

		if (!found) {
			if (mudconf.queue_chunk) {
				do_top(mudconf.queue_chunk);
			}

			continue;
		} else {
			do_top(mudconf.active_q_chunk);
		}

		/*
		 * Get usernames and hostnames
		 */

		if (slave_socket != -1 && FD_ISSET(slave_socket, &input_set)) {
			while (get_slave_result() == 0)
				;
		}

		/*
		 * Check for new connection requests
		 */

		if (FD_ISSET(sock, &input_set)) {
			newd = new_connection(sock);

			if (!newd) {
				check = (errno && (errno != EINTR) && (errno != EMFILE) && (errno != ENFILE));

				if (check) {
					log_perror("NET", "FAIL", NULL, "new_connection");
				}
			} else {
				if (newd->descriptor >= maxd) {
					maxd = newd->descriptor + 1;
				}
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

			if (FD_ISSET(d->descriptor, &input_set)) {

				/*
				 * Undo AutoDark
				 */

				if (d->flags & DS_AUTODARK) {
					d->flags &= ~DS_AUTODARK;
					s_Flags(d->player, Flags(d->player) & ~DARK);
				}

				/*
				 * Process received data
				 */

				if (!process_input(d)) {
					shutdownsock(d, R_SOCKDIED);
					continue;
				}
			}

			/*
			 * Process output for sockets with pending output
			 */

			if (FD_ISSET(d->descriptor, &output_set)) {
				if (!process_output(d)) {
					shutdownsock(d, R_SOCKDIED);
				}
			}
		}
	}
}

DESC *new_connection(int sock) {
	int newsock;
	char *buff, *cmdsave;
	DESC *d;
	struct sockaddr_in addr;
	socklen_t addr_len, len;
	char *buf;
	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *) "< new_connection >";
	addr_len = sizeof(struct sockaddr);
	newsock = accept(sock, (struct sockaddr *) &addr, &addr_len);

	if (newsock < 0) {
		return 0;
	}

	if (site_check(addr.sin_addr, mudstate.access_list) & H_FORBIDDEN) {
		log_write(LOG_NET | LOG_SECURITY, "NET", "SITE", "[%d/%s] Connection refused.  (Remote port %d)", newsock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		fcache_rawdump(newsock, FC_CONN_SITE);
		shutdown(newsock, 2);
		close(newsock);
		errno = 0;
		d = NULL;
	} else {
		buff = alloc_mbuf("new_connection.address");
		buf = alloc_lbuf("new_connection.write");
		strcpy(buff, inet_ntoa(addr.sin_addr));

		/*
		 * Ask slave process for host and username
		 */

		if ((slave_socket != -1) && mudconf.use_hostname) {
			sprintf(buf, "%s\n%s,%d,%d\n", inet_ntoa(addr.sin_addr), inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), mudconf.port);
			len = strlen(buf);

			if (write(slave_socket, buf, len) < 0) {
				close(slave_socket);
				slave_socket = -1;
			}
		}

		free_lbuf(buf);
		log_write(LOG_NET, "NET", "CONN", "[%d/%s] Connection opened (remote port %d)", newsock, buff, ntohs(addr.sin_port));
		d = initializesock(newsock, &addr);
		mudstate.debug_cmd = cmdsave;
		free_mbuf(buff);
	}

	mudstate.debug_cmd = cmdsave;
	return (d);
}

/*
 * (Dis)connect reasons that get written to the logfile
 */

const char *conn_reasons[] = { "Unspecified", "Guest-connected to", "Created", "Connected to", "Dark-connected to", "Quit", "Inactivity Timeout", "Booted", "Remote Close or Net Failure", "Game Shutdown", "Login Retry Limit", "Logins Disabled", "Logout (Connection Not Dropped)", "Too Many Connected Players" };

/*
 * (Dis)connect reasons that get fed to A_A(DIS)CONNECT via announce_connattr
 */

const char *conn_messages[] = { "unknown", "guest", "create", "connect", "cd", "quit", "timeout", "boot", "netdeath", "shutdown", "badlogin", "nologins", "logout" };

void shutdownsock(DESC * d, int reason) {
	char *buff, *buff2;
	time_t now;
	int ncon;
	DESC *dtemp;

	if ((reason == R_LOGOUT) && (site_check((d->address).sin_addr, mudstate.access_list) & H_FORBIDDEN)) {
		reason = R_QUIT;
	}

	buff = log_getname(d->player);

	if (d->flags & DS_CONNECTED) {

		/*
		 * Do the disconnect stuff if we aren't doing a LOGOUT (which
		 * keeps the connection open so the player can connect to a
		 * different character).
		 */

		if (reason != R_LOGOUT) {
			if (reason != R_SOCKDIED) {

				/*
				 * If the socket died, there's no reason to
				 * display the quit file.
				 */

				fcache_dump(d, FC_QUIT);
			}

			log_write(LOG_NET | LOG_LOGIN, "NET", "DISC", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, conn_reasons[reason], d->command_count, d->input_tot, d->output_tot, (int) (time(NULL) - d->connected_at));
		} else {
			log_write(LOG_NET | LOG_LOGIN, "NET", "LOGO", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, conn_reasons[reason], d->command_count, d->input_tot, d->output_tot, (int) (time(NULL) - d->connected_at));
		}

		/*
		 * If requested, write an accounting record of the form:
		 * Plyr# Flags Cmds ConnTime Loc Money [Site] <DiscRsn> Name
		 */

		now = mudstate.now - d->connected_at;
		buff2 = unparse_flags(GOD, d->player);
		log_write(LOG_ACCOUNTING, "DIS", "ACCT", "%d %s %d %d %d %d [%s] <%s> %s", d->player, buff2, d->command_count, (int ) now, Location(d->player), Pennies(d->player), d->addr, conn_reasons[reason], buff);
		free_sbuf(buff2);
		announce_disconnect(d->player, d, conn_messages[reason]);
	} else {
		if (reason == R_LOGOUT) {
			reason = R_QUIT;
		}

		log_write(LOG_SECURITY | LOG_NET, "NET", "DISC", "[%d/%s] Connection closed, never connected. <Reason: %s>", d->descriptor, d->addr, conn_reasons[reason]);
	}

	free_lbuf(buff);
	process_output(d);
	clearstrings(d);

	/*
	 * If this was our only connection, get out of interactive mode.
	 */

	if (d->program_data) {
		ncon = 0;
		DESC_ITER_PLAYER(d->player, dtemp)
			ncon++;

		if (ncon == 0) {
			Free_RegData(d->program_data->wait_data);
			xfree(d->program_data, "do_prog");
			atr_clr(d->player, A_PROGCMD);
		}

		d->program_data = NULL;
	}

	if (d->colormap) {
		xfree(d->colormap, "colormap");
		d->colormap = NULL;
	}

	if (reason == R_LOGOUT) {
		d->flags &= ~DS_CONNECTED;
		d->connected_at = time(NULL);
		d->retries_left = mudconf.retry_limit;
		d->command_count = 0;
		d->timeout = mudconf.idle_timeout;
		d->player = 0;

		if (d->doing) {
			xfree(d->doing, "doing");
			d->doing = NULL;
		}

		d->quota = mudconf.cmd_quota_max;
		d->last_time = 0;
		d->host_info = site_check((d->address).sin_addr, mudstate.access_list) | site_check((d->address).sin_addr, mudstate.suspect_list);
		d->input_tot = d->input_size;
		d->output_tot = 0;
		welcome_user(d);
	} else {
		shutdown(d->descriptor, 2);
		close(d->descriptor);
		freeqs(d);
		*d->prev = d->next;

		if (d->next) {
			d->next->prev = d->prev;
		}

		free_desc(d);
		ndescriptors--;
	}
}

void make_nonblocking(int s) {
#ifdef HAVE_LINGER
	struct linger ling;
#endif
#ifdef FNDELAY

	if (fcntl(s, F_SETFL, FNDELAY) == -1) {
		log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
	}
#else

	if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
		log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
	}
#endif
#ifdef HAVE_LINGER
	ling.l_onoff = 0;
	ling.l_linger = 0;

	if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling)) < 0) {
		log_perror("NET", "FAIL", "linger", "setsockopt");
	}
#endif
}

DESC *initializesock(int s, struct sockaddr_in *a) {
	DESC *d;

	if (s == slave_socket) {

		/*
		 * Whoa. We shouldn't be allocating this. If we got this
		 * descriptor, our connection with the slave must have died
		 * somehow. We make sure to take note appropriately.
		 */

		log_write(LOG_ALWAYS, "ERR", "SOCK", "Player descriptor clashes with slave fd %d", slave_socket);
		slave_socket = -1;
	}

	ndescriptors++;
	d = alloc_desc("init_sock");
	d->descriptor = s;
	d->flags = 0;
	d->connected_at = time(NULL);
	d->retries_left = mudconf.retry_limit;
	d->command_count = 0;
	d->timeout = mudconf.idle_timeout;
	d->host_info = site_check((*a).sin_addr, mudstate.access_list) | site_check((*a).sin_addr, mudstate.suspect_list);
	d->player = 0; /* be sure #0 isn't wizard.  Shouldn't be. */
	d->addr[0] = '\0';
	d->doing = NULL;
	//d->doing = sane_doing("", "doing");
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
	d->address = *a;

	if (descriptor_list) {
		descriptor_list->prev = &d->next;
	}

	d->hashnext = NULL;
	d->next = descriptor_list;
	d->prev = &descriptor_list;
	strncpy(d->addr, inet_ntoa(a->sin_addr), 50);
	descriptor_list = d;
	welcome_user(d);
	return d;
}

int process_output(DESC * d) {
	TBLOCK *tb, *save;
	int cnt;
	char *cmdsave;
	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *) "< process_output >";
	tb = d->output_head;

	while (tb != NULL) {
		while (tb->hdr.nchars > 0) {
			cnt = write(d->descriptor, tb->hdr.start, tb->hdr.nchars);

			if (cnt < 0) {
				mudstate.debug_cmd = cmdsave;

				if (errno == EWOULDBLOCK) {
					return 1;
				}

				return 0;
			}

			d->output_size -= cnt;
			tb->hdr.nchars -= cnt;
			tb->hdr.start += cnt;
		}

		save = tb;
		tb = tb->hdr.nxt;
		xfree(save, "queue_write");
		d->output_head = tb;

		if (tb == NULL) {
			d->output_tail = NULL;
		}
	}

	mudstate.debug_cmd = cmdsave;
	return 1;
}

int process_input(DESC * d) {
	char *buf;
	int got, in, lost;
	char *p, *pend, *q, *qend;
	char *cmdsave;
	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *) "< process_input >";
	buf = alloc_lbuf("process_input.buf");
	got = in = read(d->descriptor, buf, LBUF_SIZE);

	if (got <= 0) {
		mudstate.debug_cmd = cmdsave;
		free_lbuf(buf);
		return 0;
	}

	if (!d->raw_input) {
		d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
		d->raw_input_at = d->raw_input->cmd;
	}

	p = d->raw_input_at;
	pend = d->raw_input->cmd - sizeof(CBLKHDR) - 1 + LBUF_SIZE;
	lost = 0;

	for (q = buf, qend = buf + got; q < qend; q++) {
		if (*q == '\n') {
			*p = '\0';

			if (p > d->raw_input->cmd) {
				save_command(d, d->raw_input);
				d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
				p = d->raw_input_at = d->raw_input->cmd;
				pend = d->raw_input->cmd - sizeof(CBLKHDR) - 1 + LBUF_SIZE;
			} else {
				in -= 1; /* for newline */
			}
		} else if ((*q == '\b') || (*q == 127)) {
			if (*q == 127) {
				queue_string(d, NULL, "\b \b");
			} else {
				queue_string(d, NULL, " \b");
			}

			in -= 2;

			if (p > d->raw_input->cmd) {
				p--;
			}

			if (p < d->raw_input_at) {
				(d->raw_input_at)--;
			}
		} else if (p < pend && isascii(*q) && isprint(*q)) {
			*p++ = *q;
		} else {
			in--;

			if (p >= pend) {
				lost++;
			}
		}
	}

	if (in < 0) { /* backspace and delete by themselves */
		in = 0;
	}

	if (p > d->raw_input->cmd) {
		d->raw_input_at = p;
	} else {
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

void close_sockets(int emergency, char *message) {
	DESC *d, *dnext;
	DESC_SAFEITER_ALL(d, dnext)
	{
		if (emergency) {
			if (write(d->descriptor, message, strlen(message)) < 0) {
				log_perror("NET", "FAIL", NULL, "shutdown");
			}

			if (shutdown(d->descriptor, 2) < 0) {
				log_perror("NET", "FAIL", NULL, "shutdown");
			}

			close(d->descriptor);
		} else {
			queue_string(d, NULL, message);
			queue_write(d, "\r\n", 2);
			shutdownsock(d, R_GOING_DOWN);
		}
	}
	close(sock);
}

void emergency_shutdown(void) {
	close_sockets(1, (char *) "Going down - Bye");
}

/*
 * ---------------------------------------------------------------------------
 * Print out stuff into error file.
 */

void report(void) {
	char *player, *enactor;
	log_write(LOG_BUGS, "BUG", "INFO", "Command: '%s'", mudstate.debug_cmd);

	if (Good_obj(mudstate.curr_player)) {
		player = log_getname(mudstate.curr_player);

		if ((mudstate.curr_enactor != mudstate.curr_player) && Good_obj(mudstate.curr_enactor)) {
			enactor = log_getname(mudstate.curr_enactor);
			log_write(LOG_BUGS, "BUG", "INFO", "Player: %s Enactor: %s", player, enactor);
			free_lbuf(enactor);
		} else {
			log_write(LOG_BUGS, "BUG", "INFO", "Player: %s", player);
		}
		free_lbuf(player);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * Signal handling routines.
 */

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

static void sighandler(int sig) {
#ifdef HAVE_SYS_SIGNAME
#define signames sys_signame
#else
#ifdef SYS_SIGLIST_DECLARED
#define signames sys_siglist
#else
	static const char *signames[] = { "SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT", "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2" };
#endif				/* SYS_SIGLIST_DECLARED */
#endif				/* HAVE_SYS_SIGNAME */
	int i;
	pid_t child;
	char *s;
#if defined(HAVE_UNION_WAIT) && defined(NEED_WAIT3_DCL)
	union wait stat;
#else
	int stat;
#endif

	switch (sig) {
	case SIGUSR1: /* Normal restart now */
		log_signal(signames[sig]);
		do_restart(GOD, GOD, 0);
		break;

	case SIGUSR2: /* Dump a flatfile soon */
		mudstate.flatfile_flag = 1;
		break;

	case SIGALRM: /* Timer */
		mudstate.alarm_triggered = 1;
		break;

	case SIGCHLD: /* Change in child status */
#ifndef SIGNAL_SIGCHLD_BRAINDAMAGE
		signal(SIGCHLD, sighandler);
#endif

		while ((child = waitpid(0, &stat, WNOHANG)) > 0) {
			if (mudconf.fork_dump && mudstate.dumping && child == mudstate.dumper && (WIFEXITED(stat) || WIFSIGNALED(stat))) {
				mudstate.dumping = 0;
				mudstate.dumper = 0;
			} else if (child == slave_pid && (WIFEXITED(stat) || WIFSIGNALED(stat))) {
				slave_pid = 0;
				slave_socket = -1;
			}
		}

		break;

	case SIGHUP: /* Dump database soon */
		log_signal(signames[sig]);
		mudstate.dump_counter = 0;
		break;

	case SIGINT: /* Force a live backup */
		log_signal(signames[sig]);
		do_backup_mush(GOD, GOD, 0);
		break;

	case SIGQUIT: /* Normal shutdown soon */
		mudstate.shutdown_flag = 1;
		break;

	case SIGTERM: /* Killed shutdown now */
#ifdef SIGXCPU
	case SIGXCPU:
#endif
		check_panicking(sig);
		log_signal(signames[sig]);
		raw_broadcast(0, "GAME: Caught signal %s, exiting.", signames[sig]);
		dump_database_internal(DUMP_DB_KILLED);
		s = xstrprintf("sighandler", "Caught signal %s", signames[sig]);
		write_status_file(NOTHING, s);
		xfree(s, "sighandler");
		exit(EXIT_SUCCESS);
		break;

	case SIGILL: /* Panic save + restart now, or coredump now */
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

		if (mudconf.sig_action != SA_EXIT) {
			raw_broadcast(0, "GAME: Fatal signal %s caught, restarting with previous database.", signames[sig]);
			/*
			 * Don't sync first. Using older db.
			 */
			dump_database_internal(DUMP_DB_CRASH);
			CLOSE
			;

			if (slave_socket != -1) {
				shutdown(slave_socket, 2);
				close(slave_socket);
				slave_socket = -1;
			}

			if (slave_pid != 0) {
				kill(slave_pid, SIGKILL);
			}

			/*
			 * Try our best to dump a usable core by generating a
			 * second signal with the SIG_DFL action.
			 */

			if (fork() > 0) {
				unset_signals();

				/*
				 * In the parent process (easier to follow
				 * with gdb), we're about to return from this
				 * signal handler and hope that a second
				 * signal is delivered. Meanwhile let's close
				 * all our files to avoid corrupting the
				 * child process.
				 */
				for (i = 0; i < maxd; i++) {
					close(i);
				}

				return;
			}

			alarm(0);
			dump_restart_db();
			execl(mudconf.game_exec, mudconf.game_exec, mudconf.config_file, (char *) NULL);
			break;
		} else {
			unset_signals();
			log_write_raw(1, "ABORT! bsd.c, SA_EXIT requested.\n");
			write_status_file(NOTHING, "ABORT! bsd.c, SA_EXIT requested.");
			abort();
		}

	case SIGABRT: /* Coredump now */
		check_panicking(sig);
		log_signal(signames[sig]);
		report();
		unset_signals();
		log_write_raw(1, "ABORT! bsd.c, SIGABRT received.\n");
		write_status_file(NOTHING, "ABORT! bsd.c, SIGABRT received.");
		abort();
	}

	signal(sig, sighandler);
	mudstate.panicking = 0;
	return;
}

NAMETAB sigactions_nametab[] = { { (char *) "exit", 3, 0, SA_EXIT }, { (char *) "default", 1, 0, SA_DFLT }, { NULL, 0, 0, 0 } };

void set_signals(void) {
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

static void unset_signals(void) {
	int i;

	for (i = 0; i < NSIG; i++) {
		signal(i, SIG_DFL);
	}
}

static void check_panicking(int sig) {
	int i;

	/*
	 * If we are panicking, turn off signal catching and resignal
	 */

	if (mudstate.panicking) {
		for (i = 0; i < NSIG; i++) {
			signal(i, SIG_DFL);
		}

		kill(getpid(), sig);
	}

	mudstate.panicking = 1;
}

void log_signal(const char *signame) {
	log_write(LOG_PROBLEMS, "SIG", "CATCH", "Caught signal %s", signame);
}
