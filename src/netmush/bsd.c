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
 * @brief Function prototypes for internal functions
 *
 */
static MSGQ_DNSRESOLVER mk_msgq_dnsresolver(const char *addr);	   /*!< Create DNS resolver message queue entry */
static void *dnsResolver(void *args);							   /*!< DNS resolver thread function */
void check_dnsResolver_status(dbref player, dbref cause, int key); /*!< Check DNS resolver thread status */
static int make_socket(int port);								   /*!< Create and bind server socket */
void shovechars(int port);										   /*!< Accept new connections on server socket */
static DESC *new_connection(int sock);							   /*!< Initialize new connection descriptor */
char *connReasons(int reason);									   /*!< Connection reason strings */
char *connMessages(int reason);									   /*!< Connection message strings */
void shutdownsock(DESC *d, int reason);							   /*!< Shutdown a socket connection */
static inline void make_nonblocking(int s);						   /*!< Set socket to non-blocking mode */
static DESC *initializesock(int s, struct sockaddr_in *a);		   /*!< Initialize socket descriptor */
int process_output(DESC *d);									   /*!< Process output for a descriptor */
static int process_input(DESC *d);								   /*!< Process input for a descriptor */
void close_sockets(int emergency, char *message);				   /*!< Close all sockets and descriptors */
void report(void);												   /*!< Report current descriptor status */
static void sighandler(int sig);								   /*!< Signal handler function */
void set_signals(void);											   /*!< Set up signal handlers */
static void unset_signals(void);								   /*!< Unset signal handlers */
static inline void check_panicking(int sig);					   /*!< Check if already panicking on signal */
static inline void log_signal(const char *signame);				   /*!< Log received signal */

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
static const char *DBG_SHOVECHARS = "< shovechars >";		  /*!< shovechars debug label */
static const char *DBG_NEW_CONNECTION = "< new_connection >"; /*!< new_connection debug label */
static const char *DBG_PROCESS_OUTPUT = "< process_output >"; /*!< process_output debug label */
static const char *DBG_PROCESS_INPUT = "< process_input >";	  /*!< process_input debug label */

/**
 * @brief Create a DNS resolver message queue entry from an IP address string
 *
 * Converts a textual IP address (IPv4 or IPv6) into a binary format suitable
 * for the DNS resolver message queue. This function validates the IP address
 * format and stores it in the appropriate union member based on the address family.
 *
 * @param addr Null-terminated string containing the IP address to convert.
 *             Can be NULL, in which case an empty/unset entry is returned.
 *
 * @return MSGQ_DNSRESOLVER structure with the converted IP address.
 *         - If addr is NULL or invalid: payload.addrf = AF_UNSPEC, payload.ip unset
 *         - If addr is valid IPv4: payload.addrf = AF_INET, payload.ip.v4 contains binary address
 *         - If addr is valid IPv6: payload.addrf = AF_INET6, payload.ip.v6 contains binary address
 *
 * @note This function performs format validation but does not perform DNS resolution.
 *       The returned structure is intended to be sent to the DNS resolver thread
 *       via the message queue for hostname lookup.
 *
 * @note Thread-safe: No global state is modified.
 */
static MSGQ_DNSRESOLVER mk_msgq_dnsresolver(const char *addr)
{
	MSGQ_DNSRESOLVER h;

	/* Initialize structure to zero */
	memset(&h, 0, sizeof(h));
	h.destination = MSGQ_DEST_DNSRESOLVER;
	h.payload.addrf = AF_UNSPEC;

	if (addr && *addr)
	{
		/* Try IPv4 first (most common case) */
		if (inet_pton(AF_INET, addr, &h.payload.ip.v4) == 1)
		{
			h.payload.addrf = AF_INET;
		}
		/* If not IPv4, try IPv6 */
		else if (inet_pton(AF_INET6, addr, &h.payload.ip.v6) == 1)
		{
			h.payload.addrf = AF_INET6;
		}
		/* Invalid address format - leave as AF_UNSPEC */
	}

	return h;
}

/**
 * @brief DNS resolver thread function for asynchronous hostname lookups
 *
 * Background thread that processes DNS resolution requests from the main server thread.
 * Receives IP addresses via System V message queue, performs reverse DNS lookups using
 * getnameinfo(), and sends back the resolved hostnames. This allows non-blocking DNS
 * resolution without stalling the main game loop.
 *
 * @param args Pointer to key_t containing the message queue key for communication
 *
 * @return void* Always returns NULL (thread termination)
 *
 * @note This function runs in a separate POSIX thread created during server initialization
 * @note Message queue: Uses System V IPC for thread-safe communication with main thread
 * @note DNS resolution: Uses getnameinfo() for IPv4/IPv6 hostname resolution
 * @note Thread lifecycle: Runs indefinitely until receiving AF_UNSPEC message to terminate
 * @note Error handling: Logs failures but continues processing other requests
 * @note Performance: Processes requests sequentially, suitable for low-frequency lookups
 *
 * Thread communication protocol:
 * 1. Main thread sends MSGQ_DNSRESOLVER with IP address (addrf=AF_INET/AF_INET6)
 * 2. DNS thread receives, performs getnameinfo() lookup
 * 3. If successful, sends MSGQ_DNSRESOLVER response with hostname
 * 4. Main thread receives and processes the hostname
 * 5. Shutdown: Main thread sends message with addrf=AF_UNSPEC to terminate thread
 */
static void *dnsResolver(void *args)
{
	key_t msgq_key = *((key_t *)args);
	int msgq_id;
	MSGQ_DNSRESOLVER request_msg;

	/* Create/open the message queue */
	msgq_id = msgget(msgq_key, 0666 | IPC_CREAT);
	if (msgq_id == -1)
	{
		log_perror("DNS", "FAIL", "dnsResolver", "msgget");
		return NULL;
	}

	/* Main processing loop */
	for (;;)
	{
		MSGQ_DNSRESOLVER response_msg;
		int lookup_success = 0;
		char hostname[NI_MAXHOST];

		/* Receive next DNS resolution request */
		memset(&request_msg, 0, sizeof(request_msg));
		if (msgrcv(msgq_id, &request_msg, sizeof(request_msg.payload),
				   MSGQ_DEST_DNSRESOLVER, 0) <= 0)
		{
			/* Error receiving message or shutdown signal */
			break;
		}

		/* Check for shutdown signal (unspecified address family) */
		if (request_msg.payload.addrf == AF_UNSPEC)
		{
			break;
		}

		/* Prepare response message */
		response_msg = request_msg;
		response_msg.destination = MSGQ_DEST_REPLY - MSGQ_DEST_DNSRESOLVER;
		response_msg.payload.hostname = NULL;

		/* Perform reverse DNS lookup based on address family */
		if (request_msg.payload.addrf == AF_INET)
		{
			/* IPv4 lookup */
			struct sockaddr_in sa = {
				.sin_family = AF_INET,
				.sin_addr = request_msg.payload.ip.v4};

			if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), hostname, sizeof(hostname),
							NULL, 0, NI_NAMEREQD) == 0)
			{
				response_msg.payload.hostname = strdup(hostname);
				lookup_success = (response_msg.payload.hostname != NULL);
			}
		}
		else if (request_msg.payload.addrf == AF_INET6)
		{
			/* IPv6 lookup */
			struct sockaddr_in6 sa6 = {
				.sin6_family = AF_INET6,
				.sin6_addr = request_msg.payload.ip.v6};

			if (getnameinfo((struct sockaddr *)&sa6, sizeof(sa6), hostname, sizeof(hostname),
							NULL, 0, NI_NAMEREQD) == 0)
			{
				response_msg.payload.hostname = strdup(hostname);
				lookup_success = (response_msg.payload.hostname != NULL);
			}
		}

		/* Send response only if lookup was successful */
		if (lookup_success)
		{
			msgsnd(msgq_id, &response_msg, sizeof(response_msg.payload), 0);
		}
	}

	/* Cleanup: Remove the message queue */
	msgctl(msgq_id, IPC_RMID, NULL);
	log_write(LOG_STARTUP, "DNS", "STOP", "DNS resolver thread exiting");

	return NULL;
}

/**
 * @brief Check and report DNS resolver thread status
 *
 * Administrative command to display the current status of the DNS resolver subsystem.
 * Provides information about the DNS resolution thread, message queue status, and
 * basic statistics. This function is intended to be called by administrative commands
 * to monitor the health and performance of the DNS resolution system.
 *
 * @param player The database reference of the player executing the command
 * @param cause The database reference of the object causing the command execution
 * @param key Additional command flags (currently unused)
 *
 * @note This function is a placeholder and does not yet implement actual status reporting.
 * @note Future implementation will include thread health, queue status, and performance metrics
 * @note Requires administrative privileges to access system diagnostics
 *
 * @todo Implement comprehensive DNS resolver status reporting:
 *       - Message queue ID and key validation
 *       - Thread responsiveness and uptime
 *       - Pending request queue length
 *       - Resolution success/failure statistics
 *       - Cache performance metrics
 *       - Error conditions and recovery status
 */
void check_dnsResolver_status(dbref player, dbref cause, int key)
{
	/** @todo Implement DNS resolver status reporting */
	notify(player, "This feature is not currently available.");
}

/**
 * @brief Create and configure a TCP listening socket
 *
 * Creates an IPv4 TCP socket, configures it for address reuse, binds it to the
 * specified port on all available interfaces, and sets it to listen for incoming
 * connections. This socket serves as the main game server port for accepting
 * player connections.
 *
 * @param port The port number to bind to (1-65535). Should be a valid, unused port.
 * @return int The file descriptor of the created socket on success, -1 on failure.
 *
 * @note The socket is configured with SO_REUSEADDR to allow quick restarts
 * @note During server restart (mushstate.restarting), binding is skipped to
 *       allow the existing socket to remain active
 * @note Listen backlog is set to 5 connections (reasonable for a game server)
 * @note On failure, the function returns -1 instead of exiting to allow
 *       graceful error handling by the caller
 *
 * Error conditions:
 * - Socket creation fails: Returns -1, logs error
 * - setsockopt fails: Returns -1, logs error, closes socket
 * - bind fails: Returns -1, logs error, closes socket
 * - listen fails: Returns -1, logs error, closes socket
 *
 * @warning This function may return -1, caller must check return value
 */
static int make_socket(int port)
{
	int sock_fd;
	int reuse_addr = 1;
	struct sockaddr_in server_addr;

	/* Create IPv4 TCP socket */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
	{
		log_perror("NET", "FAIL", NULL, "creating master socket");
		return -1;
	}

	/* Enable address reuse to allow quick server restarts */
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0)
	{
		log_perror("NET", "FAIL", NULL, "setsockopt SO_REUSEADDR");
		close(sock_fd);
		return -1;
	}

	/* Configure server address structure */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; /* Listen on all interfaces */
	server_addr.sin_port = htons(port);

	/* Bind socket to address (skip during restart to preserve existing binding) */
	if (!mushstate.restarting)
	{
		if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		{
			log_perror("NET", "FAIL", NULL, "bind socket");
			close(sock_fd);
			return -1;
		}
	}

	/* Set socket to listen for incoming connections */
	if (listen(sock_fd, 5) < 0)
	{
		log_perror("NET", "FAIL", NULL, "listen on socket");
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

/**
 * @brief Main server event loop processing network I/O and game commands
 *
 * This function implements the core server loop for TinyMUSH, handling network connections,
 * DNS resolution, command processing, and various server maintenance tasks. It creates
 * the listening socket, starts background threads, and continuously monitors for events
 * using select().
 *
 * The loop processes:
 * - New incoming connections on the game port
 * - Input/output on existing player connections
 * - DNS resolution responses from background thread
 * - Periodic command queue processing
 * - Database dumping and backup signals
 * - Server shutdown signals
 *
 * @param port The TCP port number to listen for incoming connections (1-65535).
 *             Must be a valid, unused port in the system's port range.
 *
 * @note This is the main server loop; the function does not return until shutdown
 * @note Threading: Spawns a DNS resolver background thread for hostname lookups
 * @note Blocking: Uses blocking select() calls with timeouts for event-driven operation
 * @note Memory: Allocates temporary message queue path and structures, cleaned up on exit
 * @note Error handling: Graceful shutdown on critical errors like socket creation failure
 * @note Performance: Optimized for low-latency response to network events and commands
 *
 * Server startup sequence:
 * 1. Create listening socket (if not restarting)
 * 2. Check for socket creation errors and shutdown if failed
 * 3. Initialize System V message queue for DNS resolver communication
 * 4. Start DNS resolver background thread
 * 5. Enter main event loop processing network and game events
 * 6. On shutdown: Signal DNS thread to stop, wait for thread completion, cleanup resources
 *
 * Event loop behavior:
 * - Monitors all active sockets (listening + player connections) for readability/writability
 * - Processes DNS responses asynchronously without blocking main loop
 * - Handles connection limits and descriptor exhaustion gracefully
 * - Supports hot restart by preserving existing socket during mushstate.restarting
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

	/* Check if socket creation failed */
	if (!mushstate.restarting && sock == -1)
	{
		log_write(LOG_STARTUP, "NET", "FATAL", "Cannot create listening socket, server cannot start");
		mushstate.shutdown_flag = 1;
	}

	safe_gettimeofday(&last_slice);

	maxfds = getdtablesize();

	avail_descriptors = maxfds - 7;

	/* Start the message queue. */

	msgq_Path = XASPRINTF("s", "/tmp/%sXXXXXX", mushconf.mush_shortname);
	msgq_Path = mkdtemp(msgq_Path);
	msgq_Key = ftok(msgq_Path, 0x32);
	msgq_Id = msgget(msgq_Key, 0666 | IPC_CREAT);
	memset(&msgq_Dns, 0, sizeof(msgq_Dns));

	/* Start the DNS resolver thread */
	pthread_create(&thread_Dns, NULL, dnsResolver, &msgq_Key);

	/* This is the main loop of the MUSH, everything derive from here... */
	while (mushstate.shutdown_flag == 0)
	{
		safe_gettimeofday(&current_time);

		last_slice = update_quotas(last_slice, current_time);
		process_commands();

		if (mushstate.shutdown_flag)
		{
			break;
		}
		/* We've gotten a signal to dump flatfiles */
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
		/* We've gotten a signal to backup */
		if (mushstate.backup_flag && !mushstate.dumping)
		{
			mushstate.backup_flag = 0;
			fork_and_backup();
		}
		/* test for events */
		dispatch();

		/* any queued robot commands waiting? */
		timeout.tv_sec = que_next();
		timeout.tv_usec = 0;
		next_slice = msec_add(last_slice, mushconf.timeslice);
		timeval_sub(next_slice, current_time);
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);

		/* Listen for new connections if there are free descriptors */
		if (ndescriptors < avail_descriptors)
		{
			FD_SET(sock, &input_set);
		}

		/* Mark sockets that we want to test for change in status */
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
		/* Wait for something to happen */
		found = select(maxd, &input_set, &output_set, (fd_set *)NULL, &timeout);

		if (found < 0)
		{
			if (errno == EBADF)
			{
				/* This one is bad, as it results in a spiral of doom,
				 * unless we can figure out what the bad file descriptor
				 * is and get rid of it. */
				log_perror("NET", "FAIL", "checking for activity", "select");

				for (d = descriptor_list; (d); d = (d)->next)
				{
					if (fstat(d->descriptor, &fstatbuf) < 0)
					{
						/* It's a player. Just toss the connection. */
						log_write(LOG_PROBLEMS, "ERR", "EBADF", "Bad descriptor %d", d->descriptor);
						shutdownsock(d, R_SOCKDIED);
					}
				}

				if ((sock != -1) && (fstat(sock, &fstatbuf) < 0))
				{
					/* We didn't figured it out, well that's it, game over. */
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
		/* if !found then time for robot commands */
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

		/* Check if we have something from the DNS Resolver. */
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

		/* Check for new connection requests */
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
		/* Check for activity on user sockets */
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		{
			/* Process input from sockets with pending input */
			if (FD_ISSET(d->descriptor, &input_set))
			{
				/* Undo AutoDark */
				if (d->flags & DS_AUTODARK)
				{
					d->flags &= ~DS_AUTODARK;
					s_Flags(d->player, Flags(d->player) & ~DARK);
				}
				/* Process received data */
				if (!process_input(d))
				{
					shutdownsock(d, R_SOCKDIED);
					continue;
				}
			}
			/* Process output for sockets with pending output */
			if (FD_ISSET(d->descriptor, &output_set))
			{
				if (!process_output(d))
				{
					shutdownsock(d, R_SOCKDIED);
				}
			}
		}
	}

	/* Stop the DNS message queue. */
	memset(&msgq_Dns, 0, sizeof(msgq_Dns));
	msgq_Dns.destination = MSGQ_DEST_DNSRESOLVER;
	msgq_Dns.payload.addrf = AF_UNSPEC;
	msgsnd(msgq_Id, &msgq_Dns, sizeof(msgq_Dns.payload), 0);

	/* Wait for the thread to end. */
	pthread_join(thread_Dns, NULL);

	rmdir(msgq_Path);
	XFREE(msgq_Path);
}

/**
 * @brief Accept and initialize a new client connection
 *
 * Accepts an incoming TCP connection on the listening socket, performs access control checks,
 * logs the connection attempt, and initializes a new descriptor for the client if allowed.
 * This function handles the initial connection handshake and sets up the client for
 * further processing in the main server loop.
 *
 * @param sock The file descriptor of the listening socket to accept connections from.
 *             Must be a valid, bound, and listening socket.
 *
 * @return DESC* Pointer to the initialized descriptor structure on success, NULL on failure.
 *         - Returns NULL if accept() fails (e.g., no pending connections)
 *         - Returns NULL if the client's IP address is forbidden by site access lists
 *         - Returns a valid DESC* pointer for allowed connections after initialization
 *
 * @note This function is called from the main event loop when select() indicates
 *       the listening socket has pending connections
 * @note Access control: Uses site_check() against mushstate.access_list for IP filtering
 * @note DNS resolution: Sends a request to the DNS resolver thread for hostname lookup
 * @note Logging: Logs all connection attempts with IP address and port information
 * @note Resource management: Allocates descriptor memory via initializesock() on success
 * @note Error handling: Closes socket and resets errno for rejected connections
 * @note Thread safety: Safe to call from main thread, communicates with DNS thread via message queue
 *
 * Connection processing flow:
 * 1. Accept the incoming connection and get client address
 * 2. Check client IP against access control lists
 * 3. If forbidden: log rejection, send refusal message, close socket, return NULL
 * 4. If allowed: send DNS lookup request, log acceptance, initialize descriptor, return DESC*
 */
static DESC *new_connection(int sock)
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

	/* Convert client IP address to string for logging */
	char conn_addr_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr.sin_addr, conn_addr_str, sizeof(conn_addr_str));

	if (site_check(addr.sin_addr, mushstate.access_list) & H_FORBIDDEN)
	{
		log_write(LOG_NET | LOG_SECURITY, "NET", "SITE", "[%d/%s] Connection refused.  (Remote port %d)", newsock, conn_addr_str, ntohs(addr.sin_port));
		fcache_rawdump(newsock, FC_CONN_SITE);
		shutdown(newsock, 2);
		close(newsock);
		errno = 0;
		d = NULL;
	}
	else
	{
		/* Ask DNS Resolver for hostname */
		memset(&msgData, 0, sizeof(msgData));
		msgData.destination = MSGQ_DEST_DNSRESOLVER;
		msgData.payload.ip.v4 = addr.sin_addr;
		msgData.payload.addrf = AF_INET;
		msgsnd(msgq_Id, &msgData, sizeof(msgData.payload), 0);

		log_write(LOG_NET, "NET", "CONN", "[%d/%s] Connection opened (remote port %d)", newsock, conn_addr_str, ntohs(addr.sin_port));
		d = initializesock(newsock, &addr);
	}

	/* Do not free debug_cmd - it's pointing to a static constant */
	mushstate.debug_cmd = cmdsave;
	return d;
}

/**
 * @brief Get human-readable disconnect reason string for logging
 *
 * Converts a numeric disconnect reason code into a descriptive string
 * suitable for logging and administrative records. These reasons are
 * used in server logs to explain why player connections were terminated.
 *
 * @param reason The disconnect reason code (0-13).
 *             Valid codes correspond to predefined disconnection types.
 *
 * @return const char* Pointer to a static string describing the disconnect reason.
 *         Returns NULL if the reason code is invalid or out of range.
 *
 * @note The returned strings are static constants and should not be modified
 * @note Used in logging functions like log_write() for connection tracking
 * @note Thread-safe: Returns pointers to static data
 * @note Memory: No dynamic allocation, all strings are compile-time constants
 *
 */
inline char *connReasons(int reason)
{
	static const char *reason_strings[] = {
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
		"Too Many Connected Players"};

	if (reason >= 0 && reason < 14)
	{
		return (char *)reason_strings[reason];
	}
	return NULL;
}

/**
 * @brief Get short disconnect reason string for announcements
 *
 * Converts a numeric disconnect reason code into a short string
 * used in game announcements and attribute notifications (A_A(DIS)CONNECT).
 * These messages are displayed to players and objects when connections
 * are terminated, providing concise reason descriptions.
 *
 * @param reason The disconnect reason code (0-12).
 *             Valid codes correspond to predefined disconnection types.
 *
 * @return const char* Pointer to a static string with the short disconnect reason.
 *         Returns NULL if the reason code is invalid or out of range.
 *
 * @note The returned strings are static constants and should not be modified
 * @note Used in announce_disconnect() and attribute notifications for in-game messaging
 * @note Thread-safe: Returns pointers to static data
 * @note Memory: No dynamic allocation, all strings are compile-time constants
 *
 */
inline char *connMessages(int reason)
{
	static const char *message_strings[] = {
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
		"logout"};

	if (reason >= 0 && reason < 13)
	{
		return (char *)message_strings[reason];
	}
	return NULL;
}

/**
 * @brief Cleanly terminate a client connection and perform all associated cleanup
 *
 * Handles the complete disconnection process for a client socket, including logging,
 * attribute updates, resource cleanup, and connection state management. This function
 * is responsible for properly shutting down connections whether they are active player
 * sessions or failed connection attempts.
 *
 * @param d Pointer to the DESC structure representing the connection to terminate.
 *         Must not be NULL and should be a valid descriptor from the descriptor list.
 * @param reason The disconnect reason code (defined in constants.h) indicating why
 *               the connection is being terminated.
 *
 * @note This function modifies global state: updates ndescriptors, removes from descriptor_list
 * @note Memory management: Frees all dynamically allocated resources associated with the descriptor
 * @note Logging: Writes to multiple log categories (NET, LOGIN, ACCOUNTING, SECURITY) based on connection state
 * @note Attribute handling: Updates player attributes like A_LASTSITE and clears A_PROGCMD if needed
 * @note Interactive mode: Checks and exits interactive mode if this was the player's only connection
 * @note Thread safety: Safe to call from main thread, but descriptor should not be accessed concurrently
 *
 * Disconnection processing flow:
 * 1. Adjust reason code for forbidden IPs during logout
 * 2. If connected player: log disconnect, write accounting record, announce to objects
 * 3. If unconnected: log security disconnect
 * 4. Flush pending output and clear input buffers
 * 5. Clean up program data and interactive mode if applicable
 * 6. Free colormap and other resources
 * 7. For logout: reset descriptor for reconnection, keep socket open
 * 8. For other reasons: close socket, free descriptor, update descriptor count
 *
 * Special cases:
 * - R_LOGOUT: Keeps connection open for character switching, resets state
 * - R_SOCKDIED: Skips quit file display due to network failure
 * - Forbidden IPs: Converts logout to quit to prevent reconnection exploits
 */
void shutdownsock(DESC *d, int reason)
{
	char *buff = NULL, *buff2 = NULL;
	time_t now = 0L;
	int ncon = 0;
	DESC *dtemp = NULL;
	int conn_time = 0;

	if ((reason == R_LOGOUT) && (site_check((d->address).sin_addr, mushstate.access_list) & H_FORBIDDEN))
	{
		reason = R_QUIT;
	}

	buff = log_getname(d->player);
	conn_time = (int)(time(NULL) - d->connected_at);

	if (d->flags & DS_CONNECTED)
	{
		/* Do the disconnect stuff if we aren't doing a LOGOUT (which
		 * keeps the connection open so the player can connect to a different
		 * character). */
		if (reason != R_LOGOUT)
		{
			if (reason != R_SOCKDIED)
			{
				/* If the socket died, there's no reason to display the quit file. */
				fcache_dump(d, FC_QUIT);
			}

			log_write(LOG_NET | LOG_LOGIN, "NET", "DISC", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, connReasons(reason), d->command_count, d->input_tot, d->output_tot, conn_time);
		}
		else
		{
			log_write(LOG_NET | LOG_LOGIN, "NET", "LOGO", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, connReasons(reason), d->command_count, d->input_tot, d->output_tot, conn_time);
		}
		/* If requested, write an accounting record of the form: Plyr# Flags Cmds ConnTime Loc Money [Site] \<DiscRsn\> Name */
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

	/* If this was our only connection, get out of interactive mode. */
	if (d->program_data)
	{
		ncon = 0;

		for (dtemp = (DESC *)nhashfind((int)d->player, &mushstate.desc_htab); dtemp; dtemp = dtemp->hashnext)
		{
			ncon++;
		}

		if (ncon == 0)
		{
			/* Free_RegData */
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

				/* Free_RegDataStruct */
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
 * @brief Configure a socket for non-blocking I/O and disable lingering
 *
 * Sets the specified socket to non-blocking mode to prevent I/O operations from blocking
 * the calling thread, and disables the SO_LINGER socket option to ensure immediate closure
 * without waiting for pending data to be sent. This configuration is essential for
 * event-driven network servers that must handle multiple connections concurrently.
 *
 * @param s The file descriptor of the socket to configure.
 *          Must be a valid, open socket descriptor.
 *
 * @note This function does not return a value; errors are logged but do not prevent execution
 * @note Non-blocking mode: Uses FNDELAY on systems that define it (BSD variants),
 *       otherwise uses O_NDELAY (POSIX systems) for maximum portability
 * @note Linger disabled: SO_LINGER is set to off (l_onoff=0) to prevent connection closure delays
 * @note Error handling: fcntl() and setsockopt() failures are logged using log_perror()
 *       but the function continues execution to avoid disrupting server operation
 * @note Thread safety: Safe to call from any thread, but socket should not be used concurrently
 * @note Performance: Minimal overhead, called once per socket during initialization
 *
 * Socket configuration sequence:
 * 1. Set non-blocking flag using fcntl(F_SETFL) with appropriate flag for platform
 * 2. Configure SO_LINGER option to disable lingering on close
 * 3. Log any errors encountered during configuration
 */
static inline void make_nonblocking(int s)
{
	struct linger ling;

	/* Initialize linger structure to disable lingering */
	ling.l_onoff = 0;
	ling.l_linger = 0;

#ifdef FNDELAY
	/* BSD-style non-blocking flag */
	if (fcntl(s, F_SETFL, FNDELAY) == -1)
	{
		log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
	}
#else
	/* POSIX-style non-blocking flag */
	if (fcntl(s, F_SETFL, O_NDELAY) == -1)
	{
		log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
	}
#endif

	/* Disable lingering to ensure immediate socket closure */
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling)) < 0)
	{
		log_perror("NET", "FAIL", "linger", "setsockopt");
	}
}

/**
 * @brief Initialize a new client socket descriptor and add it to the active connection list
 *
 * Allocates and initializes a DESC structure for a newly accepted client connection,
 * configuring all necessary state for network I/O, player association, and server integration.
 * This function transforms a raw socket file descriptor into a fully functional connection
 * descriptor ready for use in the main server event loop.
 *
 * @param s The socket file descriptor from accept() representing the client connection.
 *          Must be a valid, connected socket descriptor.
 * @param a Pointer to the sockaddr_in structure containing the client's address information
 *          from the accept() call. Must not be NULL.
 *
 * @return DESC* Pointer to the initialized descriptor structure, never NULL on success.
 *         The function does not return on allocation failure (XMALLOC exits on failure).
 *
 * @note This function modifies global state: increments ndescriptors, updates descriptor_list
 * @note Memory management: Allocates DESC structure via XMALLOC, caller must not free directly
 * @note Socket configuration: Sets socket to non-blocking mode via make_nonblocking()
 * @note Access control: Computes host_info for IP-based access checking and suspect list matching
 * @note Linked list: Inserts descriptor at head of descriptor_list for O(1) insertion
 * @note Address conversion: Converts binary IPv4 address to string format for logging/display
 * @note Welcome message: Calls welcome_user() to send initial connection greeting
 * @note Thread safety: Safe to call from main thread only (modifies global descriptor list)
 *
 * Initialization sequence:
 * 1. Allocate DESC structure and increment global descriptor count
 * 2. Initialize all fields to default values (mostly zero/NULL)
 * 3. Set socket-specific values (descriptor, address, timestamps)
 * 4. Configure socket for non-blocking I/O
 * 5. Compute access control information from client IP
 * 6. Insert descriptor into global linked list
 * 7. Convert IP address to string format
 * 8. Send welcome message to client
 *
 * Default field values:
 * - Connection state: Unconnected, no player associated
 * - I/O buffers: Empty, no pending input/output
 * - Limits: Standard command quota, idle timeout, retry limits
 * - Resources: No colormap, program data, or special buffers allocated
 */
static DESC *initializesock(int s, struct sockaddr_in *a)
{
	DESC *d = NULL;

	ndescriptors++;
	d = XMALLOC(sizeof(DESC), "d");

	/* Zero all fields for clean initialization */
	memset(d, 0, sizeof(DESC));

	/* Set socket and connection-specific values */
	d->descriptor = s;
	d->connected_at = time(NULL);
	d->address = *a;

	/* Configure socket for non-blocking I/O */
	make_nonblocking(s);

	/* Set default limits and timeouts */
	d->retries_left = mushconf.retry_limit;
	d->timeout = mushconf.idle_timeout;
	d->quota = mushconf.cmd_quota_max;

	/* Compute access control information */
	d->host_info = site_check(a->sin_addr, mushstate.access_list) | site_check(a->sin_addr, mushstate.suspect_list);

	/* Insert at head of descriptor list for efficient management */
	if (descriptor_list)
	{
		descriptor_list->prev = &d->next;
	}
	d->next = descriptor_list;
	d->prev = &descriptor_list;
	descriptor_list = d;

	/* Convert IP address to string for logging and display */
	inet_ntop(AF_INET, &a->sin_addr, d->addr, sizeof(d->addr));

	/* Send welcome message to new connection */
	welcome_user(d);

	return d;
}

/**
 * @brief Process and send pending output data to a client socket
 *
 * Attempts to write all buffered output data to the client's socket in blocking mode,
 * handling partial writes, memory cleanup, and error conditions. This function is
 * responsible for flushing the output queue of a connection descriptor, ensuring
 * that text messages, prompts, and other data are delivered to the client.
 *
 * @param d Pointer to the DESC structure representing the client connection.
 *          Must not be NULL and should have a valid socket descriptor.
 *
 * @return int Status code indicating the result of the output processing.
 *         - Returns 1 if all data was successfully written or if the socket would block
 *         - Returns 0 if a fatal error occurred (connection should be closed)
 *
 * @note This function modifies the descriptor's output queue: removes completed blocks,
 *       updates counters, and frees memory for sent data
 * @note Blocking behavior: Uses blocking write() calls; returns 1 on EWOULDBLOCK to
 *       allow the event loop to retry later
 * @note Memory management: Frees TBLOCK structures and their data buffers after successful writes
 * @note Queue management: Maintains output_head/output_tail pointers, updates output_size counter
 * @note Error handling: Distinguishes between temporary (EWOULDBLOCK) and fatal errors
 * @note Thread safety: Safe to call from main thread only (accesses socket and descriptor)
 * @note Performance: Processes output in chunks to minimize system calls and memory usage
 *
 * Output processing flow:
 * 1. Save current debug context and set function-specific debug string
 * 2. Iterate through the output block queue (d->output_head)
 * 3. For each block, write remaining data to socket in a loop
 * 4. Handle partial writes by updating block pointers and counters
 * 5. When block is fully written, free it and move to next block
 * 6. Update queue pointers when head block is exhausted
 * 7. Restore debug context and return success status
 *
 * Error conditions:
 * - write() returns -1 with errno != EWOULDBLOCK: Fatal error, return 0
 * - write() returns -1 with errno == EWOULDBLOCK: Temporary block, return 1 (retry later)
 * - All data written successfully: Return 1
 */
int process_output(DESC *d)
{
	TBLOCK *tb = NULL, *save = NULL;
	int cnt = 0;
	char *cmdsave = NULL;

	/* Save debug context for error reporting */
	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< process_output >";

	/* Process each output block in the queue */
	tb = d->output_head;
	while (tb != NULL)
	{
		/* Write remaining data in current block */
		while (tb->hdr.nchars > 0)
		{
			cnt = write(d->descriptor, tb->hdr.start, tb->hdr.nchars);

			if (cnt < 0)
			{
				/* Restore debug context on error */
				mushstate.debug_cmd = cmdsave;

				/* Check if this is a temporary blocking condition */
				if (errno == EWOULDBLOCK)
				{
					/* Socket would block, retry later */
					return 1;
				}

				/* Fatal error, connection should be closed */
				return 0;
			}

			/* Update counters and pointers for partial write */
			d->output_size -= cnt;
			tb->hdr.nchars -= cnt;
			tb->hdr.start += cnt;
		}

		/* Block fully written, free it and move to next */
		save = tb;
		tb = tb->hdr.nxt;
		XFREE(save->data);
		XFREE(save);
		d->output_head = tb;

		/* Clear tail pointer if queue becomes empty */
		if (tb == NULL)
		{
			d->output_tail = NULL;
		}
	}

	/* Restore debug context and return success */
	mushstate.debug_cmd = cmdsave;
	return 1;
}

/**
 * @brief Character validation lookup table for fast filtering
 *
 * Lookup table used for O(1) character validation during input processing.
 * Each byte represents whether the corresponding ASCII character (0-255) is
 * considered "safe" for processing in user input. A value of 1 indicates the
 * character is acceptable, 0 indicates it should be filtered out.
 *
 * This table replaces multiple conditional checks with a single array lookup,
 * significantly improving performance during telnet protocol processing and
 * input sanitization.
 *
 * @note Initialized to all zeros, then populated by init_char_table() on first use
 * @note Thread-safe: Read-only after initialization, safe for concurrent access
 * @note Memory: 256 bytes, minimal overhead for substantial performance gain
 */
static unsigned char char_valid_table[256] = {0};

/**
 * @brief Initialize the character validation lookup table for input processing
 *
 * Populates the global char_valid_table with acceptable character codes for
 * user input filtering. This function ensures that only safe characters are
 * allowed through during telnet protocol handling and command processing,
 * preventing potential security issues from malformed or malicious input.
 *
 * The table marks printable ASCII characters (space through tilde) as safe,
 * along with specific control characters needed for terminal functionality.
 * Carriage return (CR) is intentionally excluded as it's handled separately
 * for proper line ending processing.
 *
 * @note This function uses a guard variable to ensure one-time initialization
 * @note Called automatically from process_input() when needed
 * @note Performance: Uses memset for efficient range initialization
 * @note Thread safety: Safe to call from multiple threads (idempotent)
 *
 * Character classification:
 * - Printable ASCII (0x20-0x7E): Allowed for user input and commands
 * - Control characters: Only BEEP, TAB, LF, ESC, DEL are permitted
 * - CR (0x0D): Excluded (handled separately for CRLF processing)
 * - All others: Filtered out to prevent protocol attacks or display issues
 *
 * Initialization is performed once per server run, after which the table
 * remains static and is used for fast character validation throughout the
 * input processing pipeline.
 */
static void init_char_table(void)
{
	static int initialized = 0;
	if (initialized)
		return;

	/* Set printable ASCII range (space through tilde) using memset for efficiency */
	memset(&char_valid_table[0x20], 1, 0x7E - 0x20 + 1);

	/* Add specific safe control characters */
	char_valid_table[0x07] = 1;		  /* BEEP (\a) */
	char_valid_table[0x09] = 1;		  /* TAB (\t) */
	char_valid_table[0x0A] = 1;		  /* LF (\n) */
	/* char_valid_table[0x0D] = 1; */ /* CR - handled separately, not buffered */
	char_valid_table[0x1B] = 1;		  /* ESC */
	char_valid_table[0x7F] = 1;		  /* DEL */

	initialized = 1;
}

/**
 * @brief Process raw input data from a client socket with telnet protocol handling
 *
 * Reads incoming data from a client connection, processes telnet IAC (Interpret As Command)
 * protocol sequences, filters invalid characters, handles line termination (CRLF), and
 * assembles command lines for the game engine. This function implements a highly optimized
 * single-pass processing pipeline that combines protocol parsing, input validation, and
 * command assembly into one efficient loop.
 *
 * @param d Pointer to the DESC structure representing the client connection.
 *          Must not be NULL and should have a valid socket descriptor.
 *
 * @return int Status code indicating the result of input processing.
 *         - Returns 1 if input was successfully processed (even if no complete commands)
 *         - Returns 0 if a read error occurred or connection was closed (EOF)
 *
 * @note This function modifies the descriptor's input buffers and statistics
 * @note Telnet protocol: Handles IAC sequences for option negotiation and data transparency
 * @note Character filtering: Uses lookup table for O(1) validation of acceptable characters
 * @note Line handling: Properly processes CRLF sequences, treating CR+LF as single newline
 * @note Buffer management: Manages raw_input buffer allocation and command assembly
 * @note Statistics: Updates input_tot, input_size, and input_lost counters
 * @note Memory: Allocates temporary read buffer, manages descriptor's raw_input buffer
 * @note Thread safety: Safe to call from main thread only (accesses socket and descriptor)
 * @note Performance: Single-pass processing eliminates multiple buffer scans
 *
 * Input processing pipeline:
 * 1. Initialize character validation table (one-time setup)
 * 2. Read raw data from socket into temporary buffer
 * 3. Ensure descriptor has raw_input buffer allocated
 * 4. Process each byte in single pass:
 *    - Handle CRLF line termination
 *    - Process telnet IAC protocol sequences
 *    - Filter invalid control characters
 *    - Handle backspace/delete editing
 *    - Assemble command lines
 * 5. Update buffer pointers and statistics
 * 6. Clean up temporary buffers
 *
 * Telnet IAC handling:
 * - IAC IAC (0xFF 0xFF): Escaped literal 0xFF character
 * - IAC DO/DONT/WILL/WONT: Option negotiation (logged and responded to)
 * - Other IAC sequences: Stripped from input stream
 *
 * Character filtering:
 * - Printable ASCII (0x20-0x7E): Allowed
 * - Safe controls (BELL, TAB, LF, ESC, DEL): Allowed
 * - CR: Handled separately for line termination
 * - Invalid controls: Filtered out
 * - Backspace/Delete: Handled for line editing
 */
static int process_input(DESC *d)
{
	char *buf, *p, *pend, *q, *qend, *cmdsave;
	int got, in, lost;
	unsigned char ch, cmd, option, response[3];

	init_char_table();

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

	/* Ensure raw_input buffer exists */
	if (!d->raw_input)
	{
		d->raw_input = (CBLK *)XMALLOC(LBUF_SIZE, "d->raw_input");
		d->raw_input_at = d->raw_input->cmd;
	}

	p = d->raw_input_at;
	pend = d->raw_input->cmd - sizeof(CBLKHDR) - 1 + LBUF_SIZE;
	lost = 0;

	/* Single-pass processing: IAC handling, filtering, and line assembly This eliminates two complete buffer passes from the original code */
	int skip_next_lf = 0; /* Flag to skip LF after CR */

	for (q = buf, qend = buf + got; q < qend; q++)
	{
		ch = (unsigned char)*q;

		/* Skip LF if it immediately follows a CR (handle CRLF properly) */
		if (skip_next_lf && ch == '\n')
		{
			skip_next_lf = 0;
			in--;
			continue;
		}
		skip_next_lf = 0;

		/* Fast path: handle telnet IAC protocol sequences */
		if (ch == 0xFF && q + 1 < qend)
		{
			cmd = (unsigned char)q[1];

			/* IAC IAC -> escaped 0xFF literal (check this first) */
			if (cmd == 0xFF)
			{
				q++;  /* Skip second 0xFF, fall through to process first */
				in--; /* One less byte counted */
				ch = 0xFF;
				/* Continue to normal processing below */
			}
			/* Three-byte IAC negotiation (DO, DONT, WILL, WONT) */
			else if ((cmd >= 0xFB && cmd <= 0xFE) && q + 2 < qend)
			{
				option = (unsigned char)q[2];

				/* Log client telnet negotiation */
				{
					const char *cmd_name = NULL;
					switch (cmd)
					{
					case 0xFD:
						cmd_name = "DO";
						break;
					case 0xFE:
						cmd_name = "DONT";
						break;
					case 0xFB:
						cmd_name = "WILL";
						break;
					case 0xFC:
						cmd_name = "WONT";
						break;
					default:
						cmd_name = "UNKNOWN";
						break;
					}
					log_write(LOG_NET, "NET", "TELNEG", "[%d] Client sent %s %d (0x%02X)", d->descriptor, cmd_name, option, option);
				}

				/*
				 * Respond to prevent client hang
				 * Accept options we advertised (SUPPRESS_GO_AHEAD, ECHO)
				 * Refuse everything else
				 */
				if (cmd == 0xFD) /* DO -> check if we can/want to do it */
				{
					if (option == 0x03) /* SUPPRESS_GO_AHEAD - we advertised this */
					{
						/* Accept: send WILL (already sent in welcome, but confirm) */
						/* No response needed - we already sent WILL */
					}
					else if (option == 0x00) /* BINARY - we advertised this */
					{
						/* Accept: no response needed - we already sent WILL */
					}
					else if (option == 0x22) /* LINEMODE - accept this */
					{
						/* Accept: send WILL */
						response[0] = 0xFF; /* IAC */
						response[1] = 0xFB; /* WILL */
						response[2] = option;
						log_write(LOG_NET, "NET", "TELNEG", "[%d] Server sent WILL %d (0x%02X)", d->descriptor, option, option);
						queue_write(d, (char *)response, 3);
					}
					else
					{
						/* Refuse: send WONT */
						response[0] = 0xFF; /* IAC */
						response[1] = 0xFC; /* WONT */
						response[2] = option;
						log_write(LOG_NET, "NET", "TELNEG", "[%d] Server sent WONT %d (0x%02X)", d->descriptor, option, option);
						queue_write(d, (char *)response, 3);
					}
				}
				else if (cmd == 0xFB) /* WILL -> respond based on option */
				{
					if (option == 0x01) /* ECHO - we said WONT, accept their WILL */
					{
						/* Accept: send DO */
						response[0] = 0xFF; /* IAC */
						response[1] = 0xFD; /* DO */
						response[2] = option;
						log_write(LOG_NET, "NET", "TELNEG", "[%d] Server sent DO %d (0x%02X)", d->descriptor, option, option);
						queue_write(d, (char *)response, 3);
					}
					else
					{
						/* Refuse: send DONT */
						response[0] = 0xFF; /* IAC */
						response[1] = 0xFE; /* DONT */
						response[2] = option;
						log_write(LOG_NET, "NET", "TELNEG", "[%d] Server sent DONT %d (0x%02X)", d->descriptor, option, option);
						queue_write(d, (char *)response, 3);
					}
				}
				/* For WONT and DONT, no response needed */

				q += 2;	 /* Skip IAC, command, and option */
				in -= 2; /* Discount from input count */
				continue;
			}
			/* All other two-byte IAC commands - strip them */
			else
			{
				/* This catches: SE, NOP, DM, BRK, IP, AO, AYT, EC, EL, GA, SB, etc. */
				q++;  /* Skip both IAC and command byte */
				in--; /* Discount from input count */
				continue;
			}
		}

		/* Handle line terminators first (before validation check) */
		if (ch == '\n' || ch == '\r')
		{
			/* Set flag to skip LF after CR */
			if (ch == '\r')
				skip_next_lf = 1;

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
				in--; /* Empty line */
			}
		}
		/* Filter: reject invalid control characters using lookup table */
		else if (!char_valid_table[ch])
		{
			in--; /* Discount rejected character */
			continue;
		}
		else if (ch == '\b' || ch == 0x7F)
		{
			/* Backspace/delete - echo and remove char */
			queue_string(d, NULL, (ch == 0x7F) ? "\b \b" : " \b");
			in -= 2;

			if (p > d->raw_input->cmd)
				p--;
			if (p < d->raw_input_at)
				d->raw_input_at--;
		}
		else if (p < pend)
		{
			/* Add valid character to buffer */
			*p++ = ch;
		}
		else
		{
			/* Buffer overflow - character lost */
			in--;
			lost++;
		}
	}

	/* Update buffer pointer and statistics */
	if (in < 0)
		in = 0;

	if (p > d->raw_input->cmd)
		d->raw_input_at = p;
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
 * @brief Close all active client connections and the listening socket
 *
 * Terminates all client connections by either performing an immediate emergency shutdown
 * or a graceful shutdown with proper disconnection messaging. This function is called
 * during server shutdown or restart to cleanly close all network connections.
 *
 * @param emergency Flag indicating the type of shutdown to perform.
 *                  - If true: Immediate emergency shutdown, sends message and closes sockets forcefully
 *                  - If false: Graceful shutdown, queues messages and uses proper disconnection procedures
 * @param message The message to send to clients before closing their connections.
 *                For emergency shutdown, sent directly via write(); for graceful shutdown,
 *                queued through the normal output system.
 *
 * @note This function modifies global state: closes all descriptors in descriptor_list
 * @note Emergency mode: Bypasses normal output queuing for immediate shutdown
 * @note Graceful mode: Uses shutdownsock() for proper cleanup and accounting
 * @note Listening socket: Always closes the main listening socket (global 'sock')
 * @note Thread safety: Safe to call from main thread only (modifies global descriptor list)
 * @note Memory: Does not free descriptor structures (handled by shutdownsock in graceful mode)
 *
 * Shutdown process:
 * 1. Iterate through all descriptors in descriptor_list
 * 2. For each connection:
 *    - Emergency: Send message directly, shutdown socket, close descriptor
 *    - Graceful: Queue message and CRLF, call shutdownsock() for full cleanup
 * 3. Close the main listening socket
 *
 * Emergency vs Graceful shutdown:
 * - Emergency: Fast, direct socket closure, minimal cleanup, for critical failures
 * - Graceful: Proper disconnection logging, attribute updates, resource cleanup
 *
 * Error handling:
 * - write() failures in emergency mode are logged but don't prevent shutdown
 * - shutdown() failures are logged but don't prevent socket closure
 * - All connections are closed regardless of individual operation failures
 */
void close_sockets(int emergency, char *message)
{
	DESC *d = NULL, *dnext = NULL;
	size_t msg_len = 0;

	/* Pre-compute message length for emergency shutdown efficiency */
	if (emergency && message)
	{
		msg_len = strlen(message);
	}

	/* Safely iterate through descriptor list (handles removal during iteration) */
	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
	{
		if (emergency)
		{
			/* Emergency shutdown: direct write, shutdown, and close */
			if (message && write(d->descriptor, message, msg_len) < 0)
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
			/* Graceful shutdown: queue message and use proper disconnection */
			queue_string(d, NULL, message);
			queue_write(d, "\r\n", 2);
			shutdownsock(d, R_GOING_DOWN);
		}
	}

	/* Close the main listening socket */
	close(sock);
}

/**
 * @brief Log debugging information about the current command and player context
 *
 * Writes a detailed bug report entry to the server log, capturing the current debug command
 * and associated player/enactor information when available. This function is typically called
 * during error conditions or signal handling to provide diagnostic context for debugging.
 *
 * The report includes:
 * - The current debug command string being processed
 * - Player name if a valid current player exists
 * - Enactor name if different from player and valid
 *
 * @note This function modifies no global state except through logging calls
 * @note Memory management: Allocates temporary strings for player/enactor names via log_getname(),
 *       frees them immediately after use to prevent memory leaks
 * @note Object validation: Uses Good_obj() to ensure player/enactor objects are valid before processing
 * @note Logging: Writes to LOG_BUGS category with "BUG" facility and "INFO" level
 * @note Thread safety: Safe to call from signal handlers (uses async-signal-safe logging)
 * @note Performance: Minimal overhead, only allocates memory when player information is available
 *
 * Processing flow:
 * 1. Always log the current debug command string
 * 2. If current player is valid:
 *    - Get player name string
 *    - If enactor differs from player and is valid:
 *      - Get enactor name string
 *      - Log both player and enactor names
 *      - Free enactor name string
 *    - Otherwise log only player name
 *    - Free player name string
 *
 * Use cases:
 * - Signal handlers for fatal errors (SIGSEGV, SIGILL, etc.)
 * - Critical error reporting during command processing
 * - Debugging context capture for crash analysis
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
 * @brief Handle system signals and perform appropriate server actions
 *
 * Processes incoming POSIX signals and executes the corresponding server response.
 * This function serves as the central signal dispatcher for TinyMUSH, handling
 * everything from graceful shutdowns to emergency restarts and crash recovery.
 * Signal handling is critical for server stability and proper resource cleanup.
 *
 * @param sig The signal number received from the operating system.
 *            Valid signals include SIGUSR1, SIGUSR2, SIGALRM, SIGCHLD, SIGHUP,
 *            SIGINT, SIGQUIT, SIGTERM, SIGILL, SIGFPE, SIGSEGV, SIGTRAP, SIGABRT,
 *            and platform-specific signals like SIGXCPU, SIGXFSZ, SIGEMT, SIGBUS, SIGSYS.
 *
 * @return void This function may not return for signals that cause shutdown, restart, or abort.
 *         For other signals, it returns after performing the required action.
 *
 * @note This function is installed as a signal handler and must use only async-signal-safe operations
 * @note Global state: Modifies mushstate flags (flatfile_flag, alarm_triggered, dumping, etc.)
 * @note Signal persistence: Handlers remain installed with sigaction(), no re-registration needed
 * @note Panic handling: Uses check_panicking() to prevent recursive signal handling
 * @note Thread safety: Safe for signal context (no locks, minimal heap allocation)
 * @note Performance: Designed for low latency signal response
 *
 * Signal handling actions:
 * - SIGUSR1: Log signal and perform normal server restart
 * - SIGUSR2: Set flag to dump flatfile database soon
 * - SIGALRM: Set alarm triggered flag for timer processing
 * - SIGCHLD: Reap child processes, handle dump completion
 * - SIGHUP: Log signal and reset database dump counter
 * - SIGINT: Set flag for live backup
 * - SIGQUIT: Set flag for normal shutdown
 * - SIGTERM/SIGXCPU: Graceful shutdown with database dump and exit
 * - SIGILL/SIGFPE/SIGSEGV/SIGTRAP/SIGXFSZ/SIGEMT/SIGBUS/SIGSYS: Fatal error handling
 *   - If SA_EXIT: Abort with core dump
 *   - Otherwise: Crash dump database, restart server
 * - SIGABRT: Immediate abort with core dump
 *
 * Fatal signal processing (SIGILL, SIGFPE, etc.):
 * 1. Check for panic recursion prevention
 * 2. Log the signal
 * 3. Call report() for debugging context
 * 4. If configured for restart:
 *    - Broadcast restart message
 *    - Save in-memory attributes
 *    - Dump crash database
 *    - Fork to generate core dump in child
 *    - Parent closes files and returns
 *    - Child restarts server with exec()
 * 5. If configured for exit: Abort immediately
 *
 * Graceful shutdown processing (SIGTERM):
 * 1. Check panic state
 * 2. Log signal
 * 3. Broadcast shutdown message
 * 4. Save attributes
 * 5. Dump normal database
 * 6. Write status file
 * 7. Exit successfully
 *
 * This function ensures the server responds appropriately to system events,
 * maintaining data integrity and providing diagnostic information for debugging.
 */
static void sighandler(int sig)
{
	const char *signames[] = {"SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT", "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2"};
	int i = 0;
	pid_t child = 0;
	char *s = NULL;
	int stat = 0;

	switch (sig)
	{
	case SIGUSR1: /* Normal restart now */
		log_signal(signames[sig]);
		do_restart(GOD, GOD, 0);
		break;

	case SIGUSR2: /* Dump a flatfile soon */
		mushstate.flatfile_flag = 1;
		break;

	case SIGALRM: /* Timer */
		mushstate.alarm_triggered = 1;
		break;

	case SIGCHLD: /* Change in child status */
		while ((child = waitpid(0, &stat, WNOHANG)) > 0)
		{
			if (mushconf.fork_dump && mushstate.dumping && child == mushstate.dumper && (WIFEXITED(stat) || WIFSIGNALED(stat)))
			{
				mushstate.dumping = 0;
				mushstate.dumper = 0;
			}
		}

		break;

	case SIGHUP: /* Dump database soon */
		log_signal(signames[sig]);
		mushstate.dump_counter = 0;
		break;

	case SIGINT: /* Force a live backup */
		mushstate.backup_flag = 1;
		break;

	case SIGQUIT: /* Normal shutdown soon */
		mushstate.shutdown_flag = 1;
		break;

	case SIGTERM: /* Graceful shutdown with full database dump */
#ifdef SIGXCPU
	case SIGXCPU:
#endif
		check_panicking(sig);
		log_signal(signames[sig]);
		raw_broadcast(0, "GAME: Caught signal %s, shutting down gracefully.", signames[sig]);
		al_store();								/* Persist any in-memory attribute list before exit */
		dump_database_internal(DUMP_DB_NORMAL); /* Use normal dump for graceful shutdown */
		s = XASPRINTF("s", "Caught signal %s", signames[sig]);
		write_status_file(NOTHING, s);
		XFREE(s);
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

		if (mushconf.sig_action != SA_EXIT)
		{
			raw_broadcast(0, "GAME: Fatal signal %s caught, restarting with previous database.", signames[sig]);
			/* Not good, Don't sync, restart using older db. */
			al_store(); /* Persist any in-memory attribute list before crash dump */
			dump_database_internal(DUMP_DB_CRASH);
			db_sync_attributes();
			dddb_close();

			/* Try our best to dump a usable core by generating a second signal with the SIG_DFL action. */
			if (fork() > 0)
			{
				unset_signals();
				/* In the parent process (easier to follow with gdb), we're about to return from this signal handler
				 * and hope that a second signal is delivered. Meanwhile let's close all our files to avoid corrupting
				 * the child process. */
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

	case SIGABRT: /* Coredump now */
		check_panicking(sig);
		log_signal(signames[sig]);
		report();
		unset_signals();
		log_write_raw(1, "ABORT! bsd.c, SIGABRT received.\n");
		write_status_file(NOTHING, "ABORT! bsd.c, SIGABRT received.");
		abort();
	}

	/* Signal handler is persistent with sigaction(), no need to re-register */
	mushstate.panicking = 0;
	return;
}

/**
 * @brief Install and configure signal handlers for the server
 *
 * Sets up comprehensive POSIX signal handling for the TinyMUSH server using sigaction().
 * This function configures the server to respond appropriately to system signals, ensuring
 * proper shutdown, restart, and error handling capabilities. Signal handlers are installed
 * for critical signals while others are ignored to prevent interference.
 *
 * The function first unblocks all signals to ensure they can be received, then installs
 * handlers for signals that require server action, and ignores signals that should be
 * discarded. This setup is essential for server stability and proper resource management.
 *
 * @note This function modifies the process signal mask and signal dispositions
 * @note Called during server initialization and after signal-triggered restarts
 * @note Signal mask: Unblocks all signals to allow reception after potential blocking
 * @note Handler persistence: Uses sigaction() with SA_RESTART for automatic syscall restart
 * @note Thread safety: Safe to call from main thread during initialization
 * @note Platform compatibility: Conditionally handles platform-specific signals
 *
 * Signal configuration:
 * - Handled signals (sighandler): SIGALRM, SIGCHLD, SIGHUP, SIGINT, SIGQUIT, SIGTERM,
 *   SIGUSR1, SIGUSR2, SIGTRAP, SIGILL, SIGSEGV, SIGABRT, and platform variants
 * - Ignored signals (SIG_IGN): SIGPIPE (broken pipe), SIGFPE (floating point exception)
 * - Platform-specific: SIGXCPU, SIGXFSZ, SIGEMT, SIGBUS, SIGSYS when available
 *
 * Signal handler setup sequence:
 * 1. Unblock all signals in the process mask (prevent blocking from previous restarts)
 * 2. Initialize sigaction structure with sighandler, empty mask, and SA_RESTART flags
 * 3. Install sighandler for core operational signals (ALRM, CHLD, HUP, INT, QUIT, TERM)
 * 4. Ignore SIGPIPE to prevent termination on broken client connections
 * 5. Install sighandler for user-defined signals (USR1, USR2, TRAP) and CPU limit signals
 * 6. Ignore SIGFPE to allow graceful handling of floating point errors
 * 7. Install sighandler for fatal error signals (ILL, SEGV, ABRT) and file size signals
 * 8. Install sighandler for remaining platform-specific signals when available
 *
 * This configuration ensures the server can handle system events gracefully while
 * maintaining operational continuity and providing diagnostic capabilities.
 */
void set_signals(void)
{
	sigset_t sigs;
	struct sigaction sa;
	/* We have to reset our signal mask, because of the possibility that we triggered a restart on a SIGUSR1.
	 * If we did so, then the signal became blocked, and stays blocked, since control never returns to the
	 * caller; i.e., further attempts to send a SIGUSR1 would fail. */
	sigfillset(&sigs);
	sigprocmask(SIG_UNBLOCK, &sigs, NULL);

	/* Setup sigaction structure for our handler */
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);

	/* Restart interrupted syscalls automatically */
	sa.sa_flags = SA_RESTART;

	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/* Setup for SIG_IGN */
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	/* Restore handler for remaining signals */
	sa.sa_handler = sighandler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
#ifdef SIGXCPU
	sigaction(SIGXCPU, &sa, NULL);
#endif

	/* Setup for SIG_IGN */
	sa.sa_handler = SIG_IGN;
	sigaction(SIGFPE, &sa, NULL);

	/* Restore handler for remaining signals */
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
 * @brief Reset all signal handlers to system default behavior
 *
 * Removes all custom signal handlers installed by the server and restores the default
 * system signal dispositions. This function ensures that signals behave according to
 * system defaults, typically used during shutdown, restart, or when resetting signal
 * state to prevent interference from custom handlers.
 *
 * The function iterates through all possible signal numbers (0 to NSIG-1) and sets
 * each signal's disposition to SIG_DFL (default action), with an empty signal mask
 * and no special flags. This comprehensive reset ensures complete removal of any
 * server-specific signal handling.
 *
 * @note This function modifies the process signal dispositions for all signals
 * @note Called during server shutdown, crash handling, and signal state resets
 * @note Signal mask: Sets empty mask to avoid blocking other signals
 * @note Flags: Clears all flags for standard default behavior
 * @note Thread safety: Safe to call from main thread during shutdown sequences
 * @note Performance: Simple loop over NSIG signals, minimal overhead
 *
 * Use cases:
 * - Server shutdown to restore clean signal state
 * - Crash dump preparation to avoid handler interference
 * - Signal handler reset before exec() in child processes
 * - Emergency cleanup when custom handlers may cause issues
 *
 * Signal reset process:
 * 1. Initialize sigaction structure with SIG_DFL handler
 * 2. Set empty signal mask and zero flags
 * 3. Iterate through all signals (0 to NSIG-1)
 * 4. Apply default disposition to each signal
 *
 * This function provides a clean slate for signal handling, ensuring that
 * subsequent signal behavior follows system defaults rather than server-specific logic.
 */
static void unset_signals(void)
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
 * @brief Check and handle panic state to prevent recursive signal processing
 *
 * Implements panic detection and recovery mechanism to prevent infinite loops
 * in signal handling. When a fatal signal is received, this function checks if
 * the server is already in a panic state. If so, it resets all signal handlers
 * to default behavior and re-sends the signal to ensure proper core dump generation.
 * Otherwise, it marks the server as panicking to prevent further signal recursion.
 *
 * This function is critical for crash handling, ensuring that fatal signals
 * don't get caught in loops between the signal handler and panic recovery code.
 * It provides a clean exit path for unrecoverable errors while preserving
 * diagnostic information through core dumps.
 *
 * @param sig The signal number that triggered this panic check.
 *            Used to re-signal the process if already panicking.
 *
 * @return void This function does not return when re-signaling (process terminates).
 *
 * @note This function modifies global state: sets mushstate.panicking flag
 * @note Signal handling: Resets all signal dispositions to SIG_DFL if panicking
 * @note Process termination: Calls kill() to re-send signal, causing core dump
 * @note Thread safety: Safe to call from signal handlers (async-signal-safe operations)
 * @note Performance: Minimal overhead, designed for critical error paths
 *
 * Panic handling logic:
 * 1. Check if mushstate.panicking is already set (recursive signal prevention)
 * 2. If panicking:
 *    - Reset all signal handlers to SIG_DFL using signal() for simplicity
 *    - Re-send the original signal to the process using kill()
 *    - This ensures default signal behavior and core dump generation
 * 3. If not panicking:
 *    - Set mushstate.panicking = 1 to prevent future recursion
 *    - Allow normal signal processing to continue
 *
 * Use cases:
 * - Called from sighandler() before processing fatal signals (SIGILL, SIGSEGV, etc.)
 * - Prevents signal handler loops during crash recovery
 * - Ensures clean core dump generation for debugging
 * - Maintains server stability during critical failures
 *
 * This mechanism ensures that fatal errors are handled exactly once,
 * preventing resource exhaustion and ensuring reliable crash diagnostics.
 */
static inline void check_panicking(int sig)
{
	/* If we are panicking, turn off signal catching and resignal */
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
 * @brief Log a signal reception event to the server log
 *
 * Records the reception of a POSIX signal in the server's problem log for
 * diagnostic and monitoring purposes. This function provides a standardized
 * way to track signal events, helping administrators understand system events
 * and server behavior during operation.
 *
 * The log entry includes the signal name and uses a consistent format that
 * integrates with the server's logging system. This aids in troubleshooting
 * signal-related issues and monitoring server responsiveness to system events.
 *
 * @param signame The name of the signal that was received (e.g., "SIGTERM", "SIGSEGV").
 *                This should be a human-readable string identifier for the signal.
 *
 * @return void
 *
 * @note This function modifies no global state except through logging calls
 * @note Logging: Writes to LOG_PROBLEMS category with "SIG" facility and "CATCH" level
 * @note Thread safety: Safe to call from signal handlers (uses async-signal-safe logging)
 * @note Performance: Minimal overhead, single logging call with formatted message
 *
 * Log entry format:
 * - Category: LOG_PROBLEMS (problem and error tracking)
 * - Facility: "SIG" (signal-related events)
 * - Level: "CATCH" (signal reception events)
 * - Message: "Caught signal <signame>"
 *
 * Use cases:
 * - Signal handlers logging signal reception before processing
 * - Diagnostic logging during signal-based server operations
 * - Monitoring signal frequency and types for system health analysis
 * - Debugging signal handling behavior and timing
 *
 * This function ensures consistent signal logging across the server,
 * providing valuable information for system administration and troubleshooting.
 */
static inline void log_signal(const char *signame)
{
	log_write(LOG_PROBLEMS, "SIG", "CATCH", "Caught signal %s", signame);
}
