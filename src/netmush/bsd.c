/**
 * @file bsd.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Unix socket I/O, descriptor management, and signal handling for the server loop
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @note Why should I care what color the bikeshed is? :)
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

/**
 * @attention Since this is the core of the whole show, better keep theses globals.
 *
 */
int sock = 0;				  /*!< Game socket */
int ndescriptors = 0;		  /*!< New Descriptor */
int maxd = 0;				  /*!< Max Descriptors */
DESC *descriptor_list = NULL; /*!< Descriptor list */

/**
 * @brief Message queue related
 *
 */

key_t msgq_Key = 0; /*!< Message Queue Key */
int msgq_Id = 0;	/*!< Message Queue ID (cached globally) */

/**
 * @brief Static debug command labels to avoid repeated allocations
 *
 */
static const char *DBG_SHOVECHARS = "< shovechars >";
static const char *DBG_NEW_CONNECTION = "< new_connection >";
static const char *DBG_PROCESS_OUTPUT = "< process_output >";
static const char *DBG_PROCESS_INPUT = "< process_input >";

/**
 * @brief Convert a text ip address to binary format for the message queue.
 *        Not used for now.
 *
 * @param addr IP address
 * @return MSGQ_DNSRESOLVER
 */
MSGQ_DNSRESOLVER mk_msgq_dnsresolver(const char *addr)
{
	MSGQ_DNSRESOLVER h;
	memset(&h, 0, sizeof(h));
	h.destination = MSGQ_DEST_DNSRESOLVER;
	h.payload.addrf = AF_UNSPEC;

	if (addr)
	{

		struct addrinfo hint, *res = NULL;

		memset(&hint, 0, sizeof(hint));

		hint.ai_family = PF_UNSPEC;
		hint.ai_flags = AI_NUMERICHOST;

		if (!getaddrinfo(addr, NULL, &hint, &res))
		{
			switch (res->ai_family)
			{
			case AF_INET:
				inet_pton(AF_INET, addr, &(h.payload.ip.v4));
				h.payload.addrf = res->ai_family;
				break;
			case AF_INET6:
				inet_pton(AF_INET6, addr, &(h.payload.ip.v6));
				h.payload.addrf = res->ai_family;
				break;
			}
		}
	}
	return h;
}

/**
 * @brief DNS Resolver thread
 *
 * @param args Message queue key
 * @return void* Always null. Not used.
 */
void *dnsResolver(void *args)
{
	MSGQ_DNSRESOLVER msgData;
	int msgQID = msgget(*((key_t *)args), 0666 | IPC_CREAT);

	do
	{
		memset(&msgData, 0, sizeof(msgData));
		msgrcv(msgQID, &msgData, sizeof(msgData.payload), MSGQ_DEST_DNSRESOLVER, 0);

		if ((msgData.payload.addrf == AF_INET) || (msgData.payload.addrf == AF_INET6))
		{
			MSGQ_DNSRESOLVER clbData = msgData;
			struct hostent *hostEnt = NULL;
			clbData.destination = MSGQ_DEST_REPLY - MSGQ_DEST_DNSRESOLVER;
			clbData.payload.hostname = NULL;

			switch (msgData.payload.addrf)
			{
			case AF_INET:
			{
				struct sockaddr_in sa;
				char hostname[NI_MAXHOST];
				memset(&sa, 0, sizeof(sa));
				sa.sin_family = AF_INET;
				sa.sin_addr = msgData.payload.ip.v4;
				if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD) == 0)
				{
					clbData.payload.hostname = strdup(hostname);
				}
			}
			break;
			case AF_INET6:
			{
				struct sockaddr_in6 sa6;
				char hostname[NI_MAXHOST];
				memset(&sa6, 0, sizeof(sa6));
				sa6.sin6_family = AF_INET6;
				sa6.sin6_addr = msgData.payload.ip.v6;
				if (getnameinfo((struct sockaddr *)&sa6, sizeof(sa6), hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD) == 0)
				{
					clbData.payload.hostname = strdup(hostname);
				}
			}
			break;
			}
			if (clbData.payload.hostname)
			{
				msgsnd(msgQID, &clbData, sizeof(clbData.payload), 0);
			}
		}
	} while ((msgData.payload.addrf == AF_INET) || (msgData.payload.addrf == AF_INET6));

	msgctl(msgQID, IPC_RMID, NULL);

	printf("Thread exiting.\n");

	return NULL;
}

void check_dnsResolver_status(dbref player, __attribute__((unused)) dbref cause, __attribute__((unused)) int key)
{
	/**
	 * @todo Just a placeholder for now. Call by @startslave
	 * that should also be rename to something better suiting
	 * once i sort out what this should do.
	 *
	 */
	notify(player, "This feature is being rework.");
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

	if (!mushstate.restarting)
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
	pthread_t thread_Dns = 0;
	int msgq_Id = 0;
	char *msgq_Path = NULL;
	MSGQ_DNSRESOLVER msgq_Dns;

	mushstate.debug_cmd = (char *)"< shovechars >";

	if (!mushstate.restarting)
	{
		sock = make_socket(port);
	}

	if (!mushstate.restarting)
	{
		maxd = sock + 1;
	}

	safe_gettimeofday(&last_slice);

	maxfds = getdtablesize();

	avail_descriptors = maxfds - 7;

	/**
	 * @brief Start the message queue.
	 *
	 */

	msgq_Path = mkdtemp(XASPRINTF("s", "%s/%sXXXXXX", mushconf.pid_home, mushconf.mush_shortname));
	msgq_Key = ftok(msgq_Path, 0x32);
	msgq_Id = msgget(msgq_Key, 0666 | IPC_CREAT);
	memset(&msgq_Dns, 0, sizeof(msgq_Dns));

	/**
	 * @brief Start the DNS resolver thread
	 *
	 */
	pthread_create(&thread_Dns, NULL, dnsResolver, &msgq_Key);

	/**
	 * @attention This is the main loop of the MUSH, everything derive from here...
	 *
	 */
	while (mushstate.shutdown_flag == 0)
	{
		safe_gettimeofday(&current_time);

		last_slice = update_quotas(last_slice, current_time);
		process_commands();

		if (mushstate.shutdown_flag)
		{
			break;
		}
		/**
		 * We've gotten a signal to dump flatfiles
		 *
		 */
		if (mushstate.flatfile_flag && !mushstate.dumping)
		{
			if (mushconf.dump_msg)
			{
				if (*mushconf.dump_msg)
				{
					raw_broadcast(0, "%s", mushconf.dump_msg);
				}
			}

			mushstate.dumping = 1;
			log_write(LOG_DBSAVES, "DMP", "CHKPT", "Flatfiling: %s.#%d#", mushconf.db_file, mushstate.epoch);
			dump_database_internal(DUMP_DB_FLATFILE);
			mushstate.dumping = 0;

			if (mushconf.postdump_msg)
			{
				if (*mushconf.postdump_msg)
				{
					raw_broadcast(0, "%s", mushconf.postdump_msg);
				}
			}

			mushstate.flatfile_flag = 0;
		}
		/**
		 * We've gotten a signal to backup
		 *
		 */
		if (mushstate.backup_flag && !mushstate.dumping)
		{
			mushstate.backup_flag = 0;
			fork_and_backup();
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
		next_slice = msec_add(last_slice, mushconf.timeslice);
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
		 * Mark sockets that we want to test for change in status
		 *
		 */
		for (d = descriptor_list; (d); d = (d)->next)
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

				for (d = descriptor_list; (d); d = (d)->next)
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
			if (mushconf.queue_chunk)
			{
				do_top(mushconf.queue_chunk);
			}

			continue;
		}
		else
		{
			do_top(mushconf.active_q_chunk);
		}

		/**
		 * @brief Check if we have something from the DNS Resolver.
		 *
		 */

		if (msgrcv(msgq_Id, &msgq_Dns, sizeof(msgq_Dns.payload), MSGQ_DEST_REPLY - MSGQ_DEST_DNSRESOLVER, IPC_NOWAIT) > 0)
		{
			if (mushconf.use_hostname)
			{
				for (DESC *d = descriptor_list; d; d = d->next)
				{
					struct in_addr daddr;
					inet_pton(AF_INET, d->addr, &daddr);
					if (msgq_Dns.payload.ip.v4.s_addr != daddr.s_addr)
					{
						continue;
					}

					if (d->player != 0)
					{
						if (d->username[0])
						{
							char *s = XASPRINTF("s", "%s@%s", d->username, msgq_Dns.payload.hostname);
							atr_add_raw(d->player, A_LASTSITE, s);
							XFREE(s);
						}
						else
						{
							atr_add_raw(d->player, A_LASTSITE, msgq_Dns.payload.hostname);
						}
					}

					XSTRNCPY(d->addr, msgq_Dns.payload.hostname, 50);
					d->addr[50] = '\0';
				}
			}
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
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
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

	/**
	 * @brief Stop the DNS message queue.
	 *
	 */
	memset(&msgq_Dns, 0, sizeof(msgq_Dns));
	msgq_Dns.destination = MSGQ_DEST_DNSRESOLVER;
	msgq_Dns.payload.addrf = AF_UNSPEC;
	msgsnd(msgq_Id, &msgq_Dns, sizeof(msgq_Dns.payload), 0);

	/**
	 * @brief Wait for the thread to end.
	 *
	 */
	pthread_join(thread_Dns, NULL);

	rmdir(msgq_Path);
	XFREE(msgq_Path);
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
	socklen_t addr_len;
	MSGQ_DNSRESOLVER msgData;

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)DBG_NEW_CONNECTION;
	addr_len = sizeof(struct sockaddr);
	newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);

	if (newsock < 0)
	{
		return 0;
	}

	if (site_check(addr.sin_addr, mushstate.access_list) & H_FORBIDDEN)
	{
		char addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
		log_write(LOG_NET | LOG_SECURITY, "NET", "SITE", "[%d/%s] Connection refused.  (Remote port %d)", newsock, addr_str, ntohs(addr.sin_port));
		fcache_rawdump(newsock, FC_CONN_SITE);
		shutdown(newsock, 2);
		close(newsock);
		errno = 0;
		d = NULL;
	}
	else
	{
		/**
		 * @brief Ask DNS Resolver for hostname
		 *
		 */
		memset(&msgData, 0, sizeof(msgData));
		msgData.destination = MSGQ_DEST_DNSRESOLVER;
		msgData.payload.ip.v4 = addr.sin_addr;
		msgData.payload.addrf = AF_INET;
		msgsnd(msgq_Id, &msgData, sizeof(msgData.payload), 0);

		char conn_addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr.sin_addr, conn_addr_str, sizeof(conn_addr_str));
		log_write(LOG_NET, "NET", "CONN", "[%d/%s] Connection opened (remote port %d)", newsock, conn_addr_str, ntohs(addr.sin_port));
		d = initializesock(newsock, &addr);
	}

	/* Do not free debug_cmd - it's pointing to a static constant */
	mushstate.debug_cmd = cmdsave;
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

	if ((reason == R_LOGOUT) && (site_check((d->address).sin_addr, mushstate.access_list) & H_FORBIDDEN))
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
		now = mushstate.now - d->connected_at;
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

		for (dtemp = (DESC *)nhashfind((int)d->player, &mushstate.desc_htab); dtemp; dtemp = dtemp->hashnext)
		{
			ncon++;
		}

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
		d->retries_left = mushconf.retry_limit;
		d->command_count = 0;
		d->timeout = mushconf.idle_timeout;
		d->player = 0;

		if (d->doing)
		{
			XFREE(d->doing);
			d->doing = NULL;
		}

		d->quota = mushconf.cmd_quota_max;
		d->last_time = 0;
		d->host_info = site_check((d->address).sin_addr, mushstate.access_list) | site_check((d->address).sin_addr, mushstate.suspect_list);
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
	struct linger ling;
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
	ling.l_onoff = 0;
	ling.l_linger = 0;

	if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling)) < 0)
	{
		log_perror("NET", "FAIL", "linger", "setsockopt");
	}
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

	ndescriptors++;
	d = XMALLOC(sizeof(DESC), "d");
	d->descriptor = s;
	d->flags = 0;
	d->connected_at = time(NULL);
	d->retries_left = mushconf.retry_limit;
	d->command_count = 0;
	d->timeout = mushconf.idle_timeout;
	d->host_info = site_check((*a).sin_addr, mushstate.access_list) | site_check((*a).sin_addr, mushstate.suspect_list);
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
	d->quota = mushconf.cmd_quota_max;
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
	inet_ntop(AF_INET, &a->sin_addr, d->addr, 50);
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

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< process_output >";
	tb = d->output_head;

	while (tb != NULL)
	{
		while (tb->hdr.nchars > 0)
		{
			cnt = write(d->descriptor, tb->hdr.start, tb->hdr.nchars);

			if (cnt < 0)
			{
				XFREE(mushstate.debug_cmd);
				mushstate.debug_cmd = cmdsave;

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

	XFREE(mushstate.debug_cmd);
	mushstate.debug_cmd = cmdsave;
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

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)DBG_PROCESS_INPUT;
	buf = XMALLOC(LBUF_SIZE, "buf");
	got = in = read(d->descriptor, buf, LBUF_SIZE);

	if (got <= 0)
	{
		mushstate.debug_cmd = cmdsave;
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
		else if (*q == ESC_CHAR && p < pend)
		{
			/* Allow ESC for ANSI sequences */
			*p++ = *q;
		}
		else if ((*q == '\t' || *q == '\r' || *q == BEEP_CHAR) && p < pend)
		{
			/* Allow TAB (%t), CR (%r), and BEEP (%b) for mushcode */
			*p++ = *q;
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
	mushstate.debug_cmd = cmdsave;
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

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
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

	log_write(LOG_BUGS, "BUG", "INFO", "Command: '%s'", mushstate.debug_cmd);

	if (Good_obj(mushstate.curr_player))
	{
		player = log_getname(mushstate.curr_player);

		if ((mushstate.curr_enactor != mushstate.curr_player) && Good_obj(mushstate.curr_enactor))
		{
			enactor = log_getname(mushstate.curr_enactor);
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

/**
 * @brief Handle system signals
 *
 * @param sig Signal to handle
 */
void sighandler(int sig)
{
	const char *signames[] = {"SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT", "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2"};
	int i = 0;
	pid_t child = 0;
	char *s = NULL;
	int stat = 0;

	switch (sig)
	{
	case SIGUSR1: /*!< Normal restart now */
		log_signal(signames[sig]);
		do_restart(GOD, GOD, 0);
		break;

	case SIGUSR2: /*!< Dump a flatfile soon */
		mushstate.flatfile_flag = 1;
		break;

	case SIGALRM: /*!< Timer */
		mushstate.alarm_triggered = 1;
		break;

	case SIGCHLD: /*!< Change in child status */
		while ((child = waitpid(0, &stat, WNOHANG)) > 0)
		{
			if (mushconf.fork_dump && mushstate.dumping && child == mushstate.dumper && (WIFEXITED(stat) || WIFSIGNALED(stat)))
			{
				mushstate.dumping = 0;
				mushstate.dumper = 0;
			}
		}

		break;

	case SIGHUP: /*!< Dump database soon */
		log_signal(signames[sig]);
		mushstate.dump_counter = 0;
		break;

	case SIGINT: /*!< Force a live backup */
		mushstate.backup_flag = 1;
		break;

	case SIGQUIT: /*!< Normal shutdown soon */
		mushstate.shutdown_flag = 1;
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

		if (mushconf.sig_action != SA_EXIT)
		{
			raw_broadcast(0, "GAME: Fatal signal %s caught, restarting with previous database.", signames[sig]);
			/**
			 * Not good, Don't sync, restart using older db.
			 *
			 */
			dump_database_internal(DUMP_DB_CRASH);
			cache_sync();
			dddb_close();

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
			execl(mushconf.game_exec, mushconf.game_exec, mushconf.config_file, (char *)NULL);
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

	// Signal handler is persistent with sigaction(), no need to re-register
	mushstate.panicking = 0;
	return;
}

/**
 * @brief Set the signals handlers
 *
 */
void set_signals(void)
{
	sigset_t sigs;
	struct sigaction sa;
	/**
	 * We have to reset our signal mask, because of the possibility that
	 * we triggered a restart on a SIGUSR1. If we did so, then the signal became
	 * blocked, and stays blocked, since control never returns to the caller;
	 * i.e., further attempts to send a SIGUSR1 would fail.
	 *
	 */
	sigfillset(&sigs);
	sigprocmask(SIG_UNBLOCK, &sigs, NULL);

	// Setup sigaction structure for our handler
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; // Restart interrupted syscalls automatically

	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Setup for SIG_IGN
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	// Restore handler for remaining signals
	sa.sa_handler = sighandler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
#ifdef SIGXCPU
	sigaction(SIGXCPU, &sa, NULL);
#endif

	// Setup for SIG_IGN
	sa.sa_handler = SIG_IGN;
	sigaction(SIGFPE, &sa, NULL);

	// Restore handler for remaining signals
	sa.sa_handler = sighandler;
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
#ifdef SIGFSZ
	sigaction(SIGXFSZ, &sa, NULL);
#endif
#ifdef SIGEMT
	sigaction(SIGEMT, &sa, NULL);
#endif
#ifdef SIGBUS
	sigaction(SIGBUS, &sa, NULL);
#endif
#ifdef SIGSYS
	sigaction(SIGSYS, &sa, NULL);
#endif
}

/**
 * @brief Unset the signal handlers
 *
 */
void unset_signals(void)
{
	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	for (int i = 0; i < NSIG; i++)
	{
		sigaction(i, &sa, NULL);
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
	if (mushstate.panicking)
	{
		for (int i = 0; i < NSIG; i++)
		{
			signal(i, SIG_DFL);
		}

		kill(getpid(), sig);
	}

	mushstate.panicking = 1;
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
