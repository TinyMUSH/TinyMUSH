/**
 * @file bsd_socket.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Socket creation and configuration for the server loop
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
#include <fcntl.h>
#include <unistd.h>

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
static int _bsd_socket_create(int port)
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
 * @brief Set socket to non-blocking mode and configure linger options
 *
 * Configures a socket for non-blocking I/O operations, which is essential for
 * the event-driven main server loop. Also disables socket lingering to ensure
 * immediate closure when the socket is closed, preventing resource exhaustion
 * from accumulating TIME_WAIT connections.
 *
 * @param s The socket file descriptor to configure.
 *
 * @return void
 *
 * @note This function modifies socket options but does not return errors
 * @note Error handling: Logs failures but continues execution (graceful degradation)
 * @note Socket configuration: Uses fcntl() for non-blocking mode (FNDELAY or O_NDELAY)
 * @note Linger option: SO_LINGER set to disabled (immediate closure)
 * @note Portability: Supports both BSD (FNDELAY) and POSIX (O_NDELAY) variants
 * @note Performance: Essential for efficient event loop operation
 *
 * Configuration performed:
 * 1. Set non-blocking flag via fcntl() - supports both FNDELAY and O_NDELAY
 * 2. Disable SO_LINGER to prevent TIME_WAIT state accumulation
 */
static inline void _bsd_socket_nonblocking_set(int s)
{
	struct linger ling;

	/* Initialize linger structure to disable lingering */
	ling.l_onoff = 0;
	ling.l_linger = 0;

#ifdef FNDELAY
	/* BSD-style non-blocking flag */
	if (fcntl(s, F_SETFL, FNDELAY) == -1)
	{
		log_perror("NET", "FAIL", "_make_nonblocking", "fcntl");
	}
#else
	/* POSIX-style non-blocking flag */
	if (fcntl(s, F_SETFL, O_NDELAY) == -1)
	{
		log_perror("NET", "FAIL", "_make_nonblocking", "fcntl");
	}
#endif

	/* Disable lingering to ensure immediate socket closure */
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling)) < 0)
	{
		log_perror("NET", "FAIL", "linger", "setsockopt");
	}
}

/* Public exports for bsd_main.c and bsd_connection.c */
int bsd_socket_create(int port)
{
	return _bsd_socket_create(port);
}

void bsd_socket_nonblocking_set(int s)
{
	_bsd_socket_nonblocking_set(s);
}
