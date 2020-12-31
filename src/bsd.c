/**
 * @file bsd.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief BSD-style network and signal routines
 * @version 3.3
 * @date 2020-12-25
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 * @note Why should I care what color the bikeshed is? :)
 * 
 */

/* bsd.c - BSD-style network and signal routines */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"
#include "game.h"
#include "alloc.h"
#include "flags.h"
#include "htab.h"
#include "ltdl.h"
#include "udb.h"
#include "udb_defs.h"
#include "mushconf.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "file_c.h"
#include "command.h"
#include "attrs.h"
#include "nametabs.h"

#ifndef NSIG
extern const int _sys_nsig;
#define NSIG _sys_nsig
#endif

/**
 * @attention Since this is the core of the whole show, better keep theses globals.
 * 
 */
int sock = 0;					/*!< Game socket */
int ndescriptors = 0;			/*!< New Descriptor */
int maxd = 0;					/*!< Max Descriptors */
DESC *descriptor_list = NULL;	/*!< Descriptor list */
volatile pid_t slave_pid = 0;	/*!< PID of the slace */
volatile int slave_socket = -1; /*!< Socket of the slave */

/**
 * @note Some systems are lame, and inet_addr() returns -1 on failure, despite the
 * fact that it returns an unsigned long.
 */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

/**
 * @brief Get the slave's result
 * 
 * @return int 
 */
int get_slave_result(void)
{
	char *host1 = NULL, *hostname = NULL, *host2 = NULL, *p = NULL, *userid = NULL, *s = NULL;
	char *buf = XMALLOC(LBUF_SIZE, "buf");
	int remote_port = 0, len = 0;
	unsigned long addr = 0L;
	DESC *d = NULL;

	len = read(slave_socket, buf, LBUF_SIZE - 1);

	if (len < 0)
	{
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
		{
			XFREE(buf);
			return -1;
		}

		close(slave_socket);
		slave_socket = -1;
		XFREE(buf);
		return -1;
	}
	else if (len == 0)
	{
		XFREE(buf);
		return -1;
	}

	buf[len] = '\0';
	host1 = buf;
	hostname = strchr(host1, ' ');

	if (!hostname)
	{
		goto gsr_end;
	}

	++hostname;
	hostname[-1] = '\0';
	host2 = strchr(hostname, '\n');

	if (!host2)
	{
		goto gsr_end;
	}
	++host2;

	host2[-1] = '\0';

	if (mudconf.use_hostname)
	{
		for (d = descriptor_list; d; d = d->next)
		{
			if (strcmp(d->addr, host1))
			{
				continue;
			}

			if (d->player != 0)
			{
				if (d->username[0])
				{
					s = XASPRINTF("s", "%s@%s", d->username, hostname);
					atr_add_raw(d->player, A_LASTSITE, s);
					XFREE(s);
				}
				else
				{
					atr_add_raw(d->player, A_LASTSITE, hostname);
				}
			}

			XSTRNCPY(d->addr, hostname, 50);
			d->addr[50] = '\0';
		}
	}

	p = strchr(host2, ' ');
	if (!p)
	{
		goto gsr_end;
	}

	++p;
	p[-1] = '\0';
	addr = inet_addr(host2);

	if (addr == INADDR_NONE)
	{
		goto gsr_end;
	}
	/**
	 * now we're into the RFC 1413 ident reply
	 * 
	 */
	while (isspace(*p))
	{
		++p;
	}

	remote_port = 0;

	while (isdigit(*p))
	{
		remote_port <<= 1;
		remote_port += (remote_port << 2) + (*p & 0x0f);
		++p;
	}

	while (isspace(*p))
	{
		++p;
	}

	if (*p != ',')
	{
		goto gsr_end;
	}

	++p;

	while (isspace(*p))
	{
		++p;
	}
	/**
	 * Skip the local port, making sure it consists of digits
	 * 
	 */
	while (isdigit(*p))
	{
		++p;
	}

	while (isspace(*p))
	{
		++p;
	}

	if (*p != ':')
	{
		goto gsr_end;
	}

	++p;

	while (isspace(*p))
	{
		++p;
	}
	/**
	 * Identify the reply type
	 * 
	 */
	if (strncmp(p, "USERID", 6))
	{
		goto gsr_end; /** the other standard possibility here is "ERROR" */
	}

	p += 6;

	while (isspace(*p))
	{
		++p;
	}

	if (*p != ':')
	{
		goto gsr_end;
	}

	++p;

	while (isspace(*p))
	{
		++p;
	}

	/*
	 * don't include the trailing linefeed in the userid
	 */

	userid = strchr(p, '\n');
	if (!userid)
	{
		goto gsr_end;
	}

	++userid;
	userid[-1] = '\0';
	/**
	 * go back and skip over the "OS [, charset] : " field
	 * 
	 */
	userid = strchr(p, ':');
	if (!userid)
	{
		goto gsr_end;
	}

	++userid;

	while (isspace(*userid))
	{
		++userid;
	}

	for (d = descriptor_list; d; d = d->next)
	{
		if (ntohs((d->address).sin_port) != remote_port)
		{
			continue;
		}

		if ((d->address).sin_addr.s_addr != addr)
		{
			continue;
		}

		if (d->player != 0)
		{
			if (mudconf.use_hostname)
			{
				s = XASPRINTF("s", "%s@%s", userid, hostname);
				atr_add_raw(d->player, A_LASTSITE, s);
				XFREE(s);
			}
			else
			{
				s = XASPRINTF("s", "%s@%s", userid, host2);
				atr_add_raw(d->player, A_LASTSITE, s);
				XFREE(s);
			}
		}

		XSTRNCPY(d->username, userid, 10);
		d->username[10] = '\0';
		break;
	}

gsr_end:
	XFREE(buf);
	return 0;
}

/**
 * @brief Bootstrap the slave process
 * @todo Slave should be a completly separated exec (or a thread?)... We end up with two netmush in memory like it is now!
 * 
 */
void boot_slave(void)
{
	int sv[2];
	int i = 0;
	int maxfds = 0;
	char *s = NULL;

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
	/**
	 * set to nonblocking
	 * 
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

	case 0:
		/** 
		 * child 
		 * 
		 */
		close(sv[0]);

		if (dup2(sv[1], 0) == -1)
		{
			_exit(EXIT_FAILURE);
		}

		if (dup2(sv[1], 1) == -1)
		{
			_exit(EXIT_FAILURE);
		}

		for (i = 3; i < maxfds; ++i)
		{
			close(i);
		}

		s = XASPRINTF("s", "%s/slave", mudconf.binhome);
		execlp(s, "slave", NULL);
		XFREE(s);
		_exit(EXIT_FAILURE);
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
	log_write(LOG_ALWAYS, "NET", "SLAVE", "DNS lookup slave started on fd %d", slave_socket);
}
/**
 * @brief Create a socket
 * 
 * @param port Port for the socket
 * @return int file descriptor of the socket
 */
int make_socket(int port)
{
	int s = 0, opt = 0;
	struct sockaddr_in server;

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0)
	{
		log_perror("NET", "FAIL", NULL, "creating master socket");
		exit(EXIT_FAILURE);
	}

	opt = 1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
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
			exit(EXIT_FAILURE);
		}

	listen(s, 5);
	return s;
}

/**
 * @brief Process input form a port and feed to the engine.
 * 
 * @param port Port to listen
 */
void shovechars(int port)
{
	fd_set input_set, output_set;
	struct timeval last_slice, current_time, next_slice, timeout;
	int found = 0, check = 0;
	DESC *d = NULL, *dnext = NULL, *newd = NULL;
	int avail_descriptors = 0, maxfds = 0;
	struct stat fstatbuf;

	mudstate.debug_cmd = (char *)"< shovechars >";

	if (!mudstate.restarting)
	{
		sock = make_socket(port);
	}

	if (!mudstate.restarting)
	{
		maxd = sock + 1;
	}

	get_tod(&last_slice);
#ifdef HAVE_GETDTABLESIZE
	maxfds = getdtablesize();
#else
	maxfds = sysconf(_SC_OPEN_MAX);
#endif
	avail_descriptors = maxfds - 7;

	/**
	 * @attention This is the main loop of the MUSH, everything derive from here... 
	 * 
	 */
	while (mudstate.shutdown_flag == 0)
	{
		get_tod(&current_time);
		last_slice = update_quotas(last_slice, current_time);
		process_commands();

		if (mudstate.shutdown_flag)
		{
			break;
		}
		/**
		 * We've gotten a signal to dump flatfiles
		 * 
		 */
		if (mudstate.flatfile_flag && !mudstate.dumping)
		{
			if (mudconf.dump_msg)
			{
				if (*mudconf.dump_msg)
				{
					raw_broadcast(0, "%s", mudconf.dump_msg);
				}
			}

			mudstate.dumping = 1;
			log_write(LOG_DBSAVES, "DMP", "CHKPT", "Flatfiling: %s.#%d#", mudconf.db_file, mudstate.epoch);
			dump_database_internal(DUMP_DB_FLATFILE);
			mudstate.dumping = 0;

			if (mudconf.postdump_msg)
			{
				if (*mudconf.postdump_msg)
				{
					raw_broadcast(0, "%s", mudconf.postdump_msg);
				}
			}

			mudstate.flatfile_flag = 0;
		}
		/**
		 * test for events
		 * 
		 */
		dispatch();
		/**
		 * any queued robot commands waiting?
		 * 
		 */
		timeout.tv_sec = que_next();
		timeout.tv_usec = 0;
		next_slice = msec_add(last_slice, mudconf.timeslice);
		timeval_sub(next_slice, current_time);
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		/**
		 * Listen for new connections if there are free descriptors
		 * 
		 */
		if (ndescriptors < avail_descriptors)
		{
			FD_SET(sock, &input_set);
		}
		/**
		 * Listen for replies from the slave socket
		 * 
		 */
		if (slave_socket != -1)
		{
			FD_SET(slave_socket, &input_set);
		}
		/**
		 * Mark sockets that we want to test for change in status
		 * 
		 */
		DESC_ITER_ALL(d)
		{
			if (!d->input_head)
			{
				FD_SET(d->descriptor, &input_set);
			}

			if (d->output_head)
			{
				FD_SET(d->descriptor, &output_set);
			}
		}
		/**
		 * Wait for something to happen
		 * 
		 */
		found = select(maxd, &input_set, &output_set, (fd_set *)NULL, &timeout);

		if (found < 0)
		{
			if (errno == EBADF)
			{
				/**
				 * This one is bad, as it results in a spiral
				 * of doom, unless we can figure out what the bad file 
				 * descriptor is and get rid of it.
				 * 
				 */
				log_perror("NET", "FAIL", "checking for activity", "select");
				DESC_ITER_ALL(d)
				{
					if (fstat(d->descriptor, &fstatbuf) < 0)
					{
						/**
						 * It's a player. Just toss the connection.
						 * 
						 */
						log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad descriptor %d", d->descriptor);
						shutdownsock(d, R_SOCKDIED);
					}
				}

				if ((slave_socket == -1) || (fstat(slave_socket, &fstatbuf) < 0))
				{
					/**
					 * It's the slave. Try to restart it, since it
					 * presumably died.
					 * 
					 */
					log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad slave descriptor %d", slave_socket);
					boot_slave();
				}

				if ((sock != -1) && (fstat(sock, &fstatbuf) < 0))
				{
					/**
					 * We didn't figured it out, well that's it, game over.
					 * 
					 */
					log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad game port descriptor %d", sock);
					break;
				}
			}
			else if (errno != EINTR)
			{
				log_perror("NET", "FAIL", "checking for activity", "select");
			}

			continue;
		}
		/**
		 * if !found then time for robot commands
		 * 
		 */
		if (!found)
		{
			if (mudconf.queue_chunk)
			{
				do_top(mudconf.queue_chunk);
			}

			continue;
		}
		else
		{
			do_top(mudconf.active_q_chunk);
		}
		/**
		 * Get usernames and hostnames
		 * 
		 */
		if (slave_socket != -1 && FD_ISSET(slave_socket, &input_set))
		{
			while (get_slave_result() == 0)
				;
		}
		/**
		 * Check for new connection requests
		 * 
		 */
		if (FD_ISSET(sock, &input_set))
		{
			newd = new_connection(sock);

			if (!newd)
			{
				check = (errno && (errno != EINTR) && (errno != EMFILE) && (errno != ENFILE));

				if (check)
				{
					log_perror("NET", "FAIL", NULL, "new_connection");
				}
			}
			else
			{
				if (newd->descriptor >= maxd)
				{
					maxd = newd->descriptor + 1;
				}
			}
		}
		/**
		 * Check for activity on user sockets
		 * 
		 */
		DESC_SAFEITER_ALL(d, dnext)
		{
			/**
			 * Process input from sockets with pending input
			 * 
		 	 */
			if (FD_ISSET(d->descriptor, &input_set))
			{
				/**
				 * Undo AutoDark
				 * 
		 		 */
				if (d->flags & DS_AUTODARK)
				{
					d->flags &= ~DS_AUTODARK;
					s_Flags(d->player, Flags(d->player) & ~DARK);
				}
				/**
				 * Process received data
				 * 
		 		 */
				if (!process_input(d))
				{
					shutdownsock(d, R_SOCKDIED);
					continue;
				}
			}
			/**
			 * Process output for sockets with pending output
			 * 
	 		 */
			if (FD_ISSET(d->descriptor, &output_set))
			{
				if (!process_output(d))
				{
					shutdownsock(d, R_SOCKDIED);
				}
			}
		}
	}
}

/**
 * @brief Handle new connections
 * 
 * @param sock		Socket to listen for new connections
 * @return DESC*	Connection descriptor
 */
DESC *new_connection(int sock)
{
	int newsock = 0;
	char *cmdsave = NULL;
	DESC *d = NULL;
	struct sockaddr_in addr;
	socklen_t addr_len, len;
	char *buf = NULL;

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = XSTRDUP("< new_connection >", "mudstate.debug_cmd");
	addr_len = sizeof(struct sockaddr);
	newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);

	if (newsock < 0)
	{
		return 0;
	}

	if (site_check(addr.sin_addr, mudstate.access_list) & H_FORBIDDEN)
	{
		log_write(LOG_NET | LOG_SECURITY, "NET", "SITE", "[%d/%s] Connection refused.  (Remote port %d)", newsock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		fcache_rawdump(newsock, FC_CONN_SITE);
		shutdown(newsock, 2);
		close(newsock);
		errno = 0;
		d = NULL;
	}
	else
	{
		/**
		 * Ask slave process for host and username
		 * 
 		 */
		if ((slave_socket != -1) && mudconf.use_hostname)
		{
			buf = XASPRINTF("buf", "%s\n%s,%d,%d\n", inet_ntoa(addr.sin_addr), inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), mudconf.port);
			len = strlen(buf);

			if (write(slave_socket, buf, len) < 0)
			{
				close(slave_socket);
				slave_socket = -1;
			}
			XFREE(buf);
		}

		log_write(LOG_NET, "NET", "CONN", "[%d/%s] Connection opened (remote port %d)", newsock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		d = initializesock(newsock, &addr);
		mudstate.debug_cmd = cmdsave;
	}

	mudstate.debug_cmd = cmdsave;
	return d;
}

/**
 * @brief (Dis)connect reasons that get written to the logfile
 * 
 * @param reason reason ID
 * @return char* reason message
 */
char *connReasons(int reason)
{
	switch (reason)
	{
	case 0:
		return "Unspecified";
	case 1:
		return "Guest-connected to";
	case 2:
		return "Created";
	case 3:
		return "Connected to";
	case 4:
		return "Dark-connected to";
	case 5:
		return "Quit";
	case 6:
		return "Inactivity Timeout";
	case 7:
		return "Booted";
	case 8:
		return "Remote Close or Net Failure";
	case 9:
		return "Game Shutdown";
	case 10:
		return "Login Retry Limit";
	case 11:
		return "Logins Disabled";
	case 12:
		return "Logout (Connection Not Dropped)";
	case 13:
		return "Too Many Connected Players";
	}
	return NULL;
}

/**
 * @brief (Dis)connect reasons that get fed to A_A(DIS)CONNECT via announce_connattr
 * 
 * @param message reason ID
 * @return char* reason message
 */
char *connMessages(int reason)
{
	switch (reason)
	{
	case 0:
		return "unknown";
	case 1:
		return "guest";
	case 2:
		return "create";
	case 3:
		return "connect";
	case 4:
		return "cd";
	case 5:
		return "quit";
	case 6:
		return "timeout";
	case 7:
		return "boot";
	case 8:
		return "netdeath";
	case 9:
		return "shutdown";
	case 10:
		return "badlogin";
	case 11:
		return "nologins";
	case 12:
		return "logout";
	}
	return NULL;
}

/**
 * @brief Handle disconnections
 * 
 * @param d			Connection descriptor
 * @param reason	Reason for disconnection
 */
void shutdownsock(DESC *d, int reason)
{
	char *buff = NULL, *buff2 = NULL;
	time_t now = 0L;
	int ncon = 0;
	DESC *dtemp = NULL;

	if ((reason == R_LOGOUT) && (site_check((d->address).sin_addr, mudstate.access_list) & H_FORBIDDEN))
	{
		reason = R_QUIT;
	}

	buff = log_getname(d->player);

	if (d->flags & DS_CONNECTED)
	{
		/**
		 * Do the disconnect stuff if we aren't doing a LOGOUT (which
		 * keeps the connection open so the player can connect to a different
		 * character).
 		 */
		if (reason != R_LOGOUT)
		{
			if (reason != R_SOCKDIED)
			{
				/**
				 * If the socket died, there's no reason to
				 * display the quit file.
				 * 
 				 */
				fcache_dump(d, FC_QUIT);
			}

			log_write(LOG_NET | LOG_LOGIN, "NET", "DISC", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, connReasons(reason), d->command_count, d->input_tot, d->output_tot, (int)(time(NULL) - d->connected_at));
		}
		else
		{
			log_write(LOG_NET | LOG_LOGIN, "NET", "LOGO", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, connReasons(reason), d->command_count, d->input_tot, d->output_tot, (int)(time(NULL) - d->connected_at));
		}
		/**
		 * If requested, write an accounting record of the form:
		 * Plyr# Flags Cmds ConnTime Loc Money [Site] <DiscRsn> Name
		 * 
 		 */
		now = mudstate.now - d->connected_at;
		buff2 = unparse_flags(GOD, d->player);
		log_write(LOG_ACCOUNTING, "DIS", "ACCT", "%d %s %d %d %d %d [%s] <%s> %s", d->player, buff2, d->command_count, (int)now, Location(d->player), Pennies(d->player), d->addr, connReasons(reason), buff);
		XFREE(buff2);
		announce_disconnect(d->player, d, connMessages(reason));
	}
	else
	{
		if (reason == R_LOGOUT)
		{
			reason = R_QUIT;
		}

		log_write(LOG_SECURITY | LOG_NET, "NET", "DISC", "[%d/%s] Connection closed, never connected. <Reason: %s>", d->descriptor, d->addr, connReasons(reason));
	}

	XFREE(buff);
	process_output(d);
	clearstrings(d);

	/**
	 * If this was our only connection, get out of interactive mode.
	 * 
	 * 
	 */
	if (d->program_data)
	{
		ncon = 0;
		DESC_ITER_PLAYER(d->player, dtemp)
		ncon++;

		if (ncon == 0)
		{
			/** 
			 * Free_RegData 
			 * 
			 */
			if (d->program_data->wait_data)
			{
				for (int z = 0; z < d->program_data->wait_data->q_alloc; z++)
				{
					if (d->program_data->wait_data->q_regs[z])
						XFREE(d->program_data->wait_data->q_regs[z]);
				}

				for (int z = 0; z < d->program_data->wait_data->xr_alloc; z++)
				{
					if (d->program_data->wait_data->x_names[z])
						XFREE(d->program_data->wait_data->x_names[z]);
						
					if (d->program_data->wait_data->x_regs[z])
						XFREE(d->program_data->wait_data->x_regs[z]);
				}

				/** 
				 * Free_RegDataStruct
				 * 
				 */
				if (d->program_data->wait_data->q_regs)
				{
					XFREE(d->program_data->wait_data->q_regs);
				}

				if (d->program_data->wait_data->q_lens)
				{
					XFREE(d->program_data->wait_data->q_lens);
				}

				if (d->program_data->wait_data->x_names)
				{
					XFREE(d->program_data->wait_data->x_names);
				}

				if (d->program_data->wait_data->x_regs)
				{
					XFREE(d->program_data->wait_data->x_regs);
				}

				if (d->program_data->wait_data->x_lens)
				{
					XFREE(d->program_data->wait_data->x_lens);
				}

				XFREE(d->program_data->wait_data);
			}

			XFREE(d->program_data);
			atr_clr(d->player, A_PROGCMD);
		}

		d->program_data = NULL;
	}

	if (d->colormap)
	{
		XFREE(d->colormap);
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

		if (d->doing)
		{
			XFREE(d->doing);
			d->doing = NULL;
		}

		d->quota = mudconf.cmd_quota_max;
		d->last_time = 0;
		d->host_info = site_check((d->address).sin_addr, mudstate.access_list) | site_check((d->address).sin_addr, mudstate.suspect_list);
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
		{
			d->next->prev = d->prev;
		}

		XFREE(d);
		ndescriptors--;
	}
}

/**
 * @brief Make a socket nonblocking if needed
 * 
 * @param s		Socket to modify.
 */
void make_nonblocking(int s)
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

	if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling)) < 0)
	{
		log_perror("NET", "FAIL", "linger", "setsockopt");
	}
#endif
}

/**
 * @brief Initialize a socket
 * 
 * @param s			Socket file descriptor
 * @param a			Socket internet address descriptor
 * @return DESC*	Connection descriptor
 */
DESC *initializesock(int s, struct sockaddr_in *a)
{
	DESC *d = NULL;

	if (s == slave_socket)
	{
		/**
		 * Whoa. We shouldn't be allocating this. If we got this
		 * descriptor, our connection with the slave must have died somehow.
		 * We make sure to take note appropriately.
		 * 
		 * 
		 */
		log_write(LOG_ALWAYS, "ERR", "SOCK", "Player descriptor clashes with slave fd %d", slave_socket);
		slave_socket = -1;
	}

	ndescriptors++;
	d = XMALLOC(sizeof(DESC), "d");
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

	if (descriptor_list)
	{
		descriptor_list->prev = &d->next;
	}

	d->hashnext = NULL;
	d->next = descriptor_list;
	d->prev = &descriptor_list;
	XSTRNCPY(d->addr, inet_ntoa(a->sin_addr), 50);
	descriptor_list = d;
	welcome_user(d);
	return d;
}

/**
 * @brief Process output to a socket
 * 
 * @param d		Socket description
 * @return int 
 */
int process_output(DESC *d)
{
	TBLOCK *tb = NULL, *save = NULL;
	int cnt = 0;
	char *cmdsave = NULL;

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *)"< process_output >";
	tb = d->output_head;

	while (tb != NULL)
	{
		while (tb->hdr.nchars > 0)
		{
			cnt = write(d->descriptor, tb->hdr.start, tb->hdr.nchars);

			if (cnt < 0)
			{
				mudstate.debug_cmd = cmdsave;

				if (errno == EWOULDBLOCK)
				{
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
		XFREE(save->data);
		XFREE(save);
		d->output_head = tb;

		if (tb == NULL)
		{
			d->output_tail = NULL;
		}
	}

	mudstate.debug_cmd = cmdsave;
	return 1;
}

/**
 * @brief Process input from a socket
 * 
 * @param d		Socket description
 * @return int 
 */
int process_input(DESC *d)
{
	char *buf = NULL;
	int got = 0, in = 0, lost = 0;
	char *p = NULL, *pend = NULL, *q = NULL, *qend = NULL, *cmdsave = NULL;

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = XSTRDUP("< process_input >", mudstate.debug_cmd);
	buf = XMALLOC(LBUF_SIZE, "buf");
	got = in = read(d->descriptor, buf, LBUF_SIZE);

	if (got <= 0)
	{
		mudstate.debug_cmd = cmdsave;
		XFREE(buf);
		return 0;
	}

	if (!d->raw_input)
	{
		d->raw_input = (CBLK *)XMALLOC(LBUF_SIZE, "d->raw_input");
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
				d->raw_input = (CBLK *)XMALLOC(LBUF_SIZE, "d->raw_input");
				p = d->raw_input_at = d->raw_input->cmd;
				pend = d->raw_input->cmd - sizeof(CBLKHDR) - 1 + LBUF_SIZE;
			}
			else
			{
				in -= 1; /* for newline */
			}
		}
		else if ((*q == '\b') || (*q == 127))
		{
			if (*q == 127)
			{
				queue_string(d, NULL, "\b \b");
			}
			else
			{
				queue_string(d, NULL, " \b");
			}

			in -= 2;

			if (p > d->raw_input->cmd)
			{
				p--;
			}

			if (p < d->raw_input_at)
			{
				(d->raw_input_at)--;
			}
		}
		else if (p < pend && isascii(*q) && isprint(*q))
		{
			*p++ = *q;
		}
		else
		{
			in--;

			if (p >= pend)
			{
				lost++;
			}
		}
	}

	if (in < 0)
	{
		in = 0; /* backspace and delete by themselves */
	}

	if (p > d->raw_input->cmd)
	{
		d->raw_input_at = p;
	}
	else
	{
		XFREE(d->raw_input);
		d->raw_input = NULL;
		d->raw_input_at = NULL;
	}

	d->input_tot += got;
	d->input_size += in;
	d->input_lost += lost;
	XFREE(buf);
	mudstate.debug_cmd = cmdsave;
	return 1;
}

/**
 * @brief Close all sockets
 * 
 * @param emergency Closing caused by emergency?
 * @param message	Message to send before closing
 */
void close_sockets(int emergency, char *message)
{
	DESC *d = NULL, *dnext = NULL;

	DESC_SAFEITER_ALL(d, dnext)
	{
		if (emergency)
		{
			if (write(d->descriptor, message, strlen(message)) < 0)
			{
				log_perror("NET", "FAIL", NULL, "shutdown");
			}

			if (shutdown(d->descriptor, 2) < 0)
			{
				log_perror("NET", "FAIL", NULL, "shutdown");
			}

			close(d->descriptor);
		}
		else
		{
			queue_string(d, NULL, message);
			queue_write(d, "\r\n", 2);
			shutdownsock(d, R_GOING_DOWN);
		}
	}
	close(sock);
}

/**
 * @brief Suggar we're going down!!!
 * 
 */
void emergency_shutdown(void)
{
	close_sockets(1, (char *)"Going down - Bye");
}

/**
 * @brief Write a report to log
 * 
 */
void report(void)
{
	char *player = NULL, *enactor = NULL;

	log_write(LOG_BUGS, "BUG", "INFO", "Command: '%s'", mudstate.debug_cmd);

	if (Good_obj(mudstate.curr_player))
	{
		player = log_getname(mudstate.curr_player);

		if ((mudstate.curr_enactor != mudstate.curr_player) && Good_obj(mudstate.curr_enactor))
		{
			enactor = log_getname(mudstate.curr_enactor);
			log_write(LOG_BUGS, "BUG", "INFO", "Player: %s Enactor: %s", player, enactor);
			XFREE(enactor);
		}
		else
		{
			log_write(LOG_BUGS, "BUG", "INFO", "Player: %s", player);
		}
		XFREE(player);
	}
}

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

/**
 * @brief Handle system signals
 * 
 * @param sig Signal to handle
 */
void sighandler(int sig)
{
#ifdef HAVE_SYS_SIGNAME
#define signames sys_signame
#else
#ifdef SYS_SIGLIST_DECLARED
#define signames sys_siglist
#else
	const char *signames[] = {"SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT", "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2"};
#endif /* SYS_SIGLIST_DECLARED */
#endif /* HAVE_SYS_SIGNAME */
	int i = 0;
	pid_t child = 0;
	char *s = NULL;
#if defined(HAVE_UNION_WAIT) && defined(NEED_WAIT3_DCL)
	union wait stat;
#else
	int stat = 0;
#endif

	switch (sig)
	{
	case SIGUSR1: /*!< Normal restart now */
		log_signal(signames[sig]);
		do_restart(GOD, GOD, 0);
		break;

	case SIGUSR2: /*!< Dump a flatfile soon */
		mudstate.flatfile_flag = 1;
		break;

	case SIGALRM: /*!< Timer */
		mudstate.alarm_triggered = 1;
		break;

	case SIGCHLD: /*!< Change in child status */
#ifndef SIGNAL_SIGCHLD_BRAINDAMAGE
		signal(SIGCHLD, sighandler);
#endif

		while ((child = waitpid(0, &stat, WNOHANG)) > 0)
		{
			if (mudconf.fork_dump && mudstate.dumping && child == mudstate.dumper && (WIFEXITED(stat) || WIFSIGNALED(stat)))
			{
				mudstate.dumping = 0;
				mudstate.dumper = 0;
			}
			else if (child == slave_pid && (WIFEXITED(stat) || WIFSIGNALED(stat)))
			{
				slave_pid = 0;
				slave_socket = -1;
			}
		}

		break;

	case SIGHUP: /*!< Dump database soon */
		log_signal(signames[sig]);
		mudstate.dump_counter = 0;
		break;

	case SIGINT: /*!< Force a live backup */
		log_signal(signames[sig]);
		do_backup_mush(GOD, GOD, 0);
		break;

	case SIGQUIT: /*!< Normal shutdown soon */
		mudstate.shutdown_flag = 1;
		break;

	case SIGTERM: /*!< Killed shutdown now */
#ifdef SIGXCPU
	case SIGXCPU:
#endif
		check_panicking(sig);
		log_signal(signames[sig]);
		raw_broadcast(0, "GAME: Caught signal %s, exiting.", signames[sig]);
		dump_database_internal(DUMP_DB_KILLED);
		s = XASPRINTF("s", "Caught signal %s", signames[sig]);
		write_status_file(NOTHING, s);
		XFREE(s);
		exit(EXIT_SUCCESS);
		break;

	case SIGILL: /*!< Panic save + restart now, or coredump now */
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
			raw_broadcast(0, "GAME: Fatal signal %s caught, restarting with previous database.", signames[sig]);
			/**
			 * Not good, Don't sync, restart using older db.
			 * 
			 */
			dump_database_internal(DUMP_DB_CRASH);
			cache_sync();
			dddb_close();

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
			/**
			 * Try our best to dump a usable core by generating a
			 * second signal with the SIG_DFL action.
			 * 
			 */
			if (fork() > 0)
			{
				unset_signals();
				/**
				 * In the parent process (easier to follow
				 * with gdb), we're about to return from this
				 * signal handler and hope that a second
				 * signal is delivered. Meanwhile let's close
				 * all our files to avoid corrupting the
				 * child process.
				 * 
				 */
				for (i = 0; i < maxd; i++)
				{
					close(i);
				}

				return;
			}

			alarm(0);
			dump_restart_db();
			execl(mudconf.game_exec, mudconf.game_exec, mudconf.config_file, (char *)NULL);
			break;
		}
		else
		{
			unset_signals();
			log_write_raw(1, "ABORT! bsd.c, SA_EXIT requested.\n");
			write_status_file(NOTHING, "ABORT! bsd.c, SA_EXIT requested.");
			abort();
		}

	case SIGABRT: /*!< Coredump now */
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

/**
 * @brief Set the signals handlers
 * 
 */
void set_signals(void)
{
	sigset_t sigs;
	/**
	 * We have to reset our signal mask, because of the possibility that
	 * we triggered a restart on a SIGUSR1. If we did so, then the signal became
	 * blocked, and stays blocked, since control never returns to the caller;
	 * i.e., further attempts to send a SIGUSR1 would fail.
	 * 
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

/**
 * @brief Unset the signal handlers
 * 
 */
void unset_signals(void)
{
	for (int i = 0; i < NSIG; i++)
	{
		signal(i, SIG_DFL);
	}
}

/**
 * @brief Check if we are panicking.
 * 
 * @param sig Signal that triggered the check
 */
void check_panicking(int sig)
{
	/**
	 * If we are panicking, turn off signal catching and resignal
	 * 
	 */
	if (mudstate.panicking)
	{
		for (int i = 0; i < NSIG; i++)
		{
			signal(i, SIG_DFL);
		}

		kill(getpid(), sig);
	}

	mudstate.panicking = 1;
}

/**
 * @brief Log the signal we caugh.
 * 
 * @param signame Signal name.
 */
void log_signal(const char *signame)
{
	log_write(LOG_PROBLEMS, "SIG", "CATCH", "Caught signal %s", signame);
}
