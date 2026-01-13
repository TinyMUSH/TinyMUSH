/**
 * @file bsd_socket.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Socket creation, initialization, and configuration
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
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
#include <sys/socket.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief External global variables from bsd.c
 *
 */
extern int msgq_Id;			   /*!< Message Queue ID (defined in bsd.c) */
extern int msgq_Key;		   /*!< Message Queue Key (defined in bsd.c) */
extern DESC *descriptor_list;  /*!< Descriptor list (defined in bsd.c) */
extern int ndescriptors;	   /*!< New Descriptor (defined in bsd.c) */

/**
 * @brief Forward declarations for static functions
 *
 */
static DESC *initializesock(int s, struct sockaddr_in *a);

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
DESC *new_connection(int sock)
{
	int newsock = 0;
	char *cmdsave = NULL;
	DESC *d = NULL;
	struct sockaddr_in addr;
	socklen_t addr_len;
	MSGQ_DNSRESOLVER msgData;

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< new_connection >";
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
 * @brief Create listening socket and prepare to accept connections
 *
 * Wrapper function that creates the main game server listening socket.
 * This is typically called once during server initialization.
 *
 * @param port The port number to listen on (1-65535)
 * @return int The socket file descriptor, or -1 on failure
 */
int init_socket(int port)
{
	return make_socket(port);
}
