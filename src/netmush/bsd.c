/**
 * @file bsd.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Main server loop and socket management coordination
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @note Why should I care what color the bikeshed is? :)
 *
 * This file has been refactored to split functionality into logical modules:
 * - bsd_dns.c:    DNS resolver thread and hostname lookup functionality
 * - bsd_socket.c: Socket creation, initialization, and configuration
 * - bsd_conn.c:   Connection management and disconnection handling
 * - bsd_io.c:     Input and output processing for network connections
 * - bsd_sig.c:    Signal handling and server control
 * - bsd.c:        Main server loop (this file)
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
 * @brief Forward declarations for functions implemented in separate modules
 *
 */
extern int init_socket(int port);
extern DESC *new_connection(int sock);
extern void shutdownsock(DESC *d, int reason);
extern int process_output(DESC *d);
extern int process_input_wrapper(DESC *d);
extern void check_dnsResolver_status(dbref player, dbref cause, int key);
extern void *dnsResolver(void *args);
extern void close_sockets(int emergency, char *message);
extern void report(void);
extern void set_signals(void);
extern void unset_signals(void);

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
	int local_msgq_Id = 0;
	char *msgq_Path = NULL;
	MSGQ_DNSRESOLVER msgq_Dns;

	mushstate.debug_cmd = (char *)"< shovechars >";

	if (!mushstate.restarting)
	{
		sock = init_socket(port);
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
	local_msgq_Id = msgget(msgq_Key, 0666 | IPC_CREAT);
	msgq_Id = local_msgq_Id;
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
		if (msgrcv(local_msgq_Id, &msgq_Dns, sizeof(msgq_Dns.payload), MSGQ_DEST_REPLY - MSGQ_DEST_DNSRESOLVER, IPC_NOWAIT) > 0)
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
				if (!process_input_wrapper(d))
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
	msgsnd(local_msgq_Id, &msgq_Dns, sizeof(msgq_Dns.payload), 0);

	/* Wait for the thread to end. */
	pthread_join(thread_Dns, NULL);

	rmdir(msgq_Path);
	XFREE(msgq_Path);
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
