/**
 * @file bsd_connection.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Connection management and lifecycle for client descriptors
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
#include <sys/msg.h>
#include <time.h>
#include <unistd.h>



/* Forward declarations for functions in other modules */
extern void bsd_socket_nonblocking_set(int s);
extern int bsd_io_output_process(DESC *d);

/**
 * @brief Static debug label for bsd_conn_new
 *
 */
static const char *DBG_NEW_CONNECTION = "< bsd_conn_new >";

/* Forward declarations */
static DESC *_bsd_sock_initialize(int s, struct sockaddr_in *a);

/**
 * @brief Accept a new client connection and perform initial setup
 *
 * Accepts an incoming client connection on the listening socket, performs
 * access control checks, initiates DNS resolution, and creates a descriptor
 * for the new connection. This function handles the initial stage of connection
 * establishment, setting up the framework for subsequent authentication and play.
 *
 * @param sock The listening socket file descriptor returned from socket()
 *             and configured with listen().
 *
 * @return DESC* Pointer to the initialized descriptor for the accepted connection,
 *         or NULL if the connection was refused (due to access control) or accept() failed.
 *
 * @note This function modifies global state through descriptor initialization
 * @note Blocking: Uses accept() which may block if no connections are pending
 * @note Access control: Checks H_FORBIDDEN flag from site access list
 * @note DNS: Asynchronously sends IP address to DNS resolver via message queue
 * @note Error handling: Returns NULL on failure, logs all connection events
 * @note Thread safety: Safe to call from main thread only (modifies global descriptor list)
 * @note Socket handling: Closes socket if connection is refused
 *
 * Connection acceptance flow:
 * 1. Accept incoming connection from listening socket
 * 2. Convert client IP address to string format
 * 3. Check access control list for forbidden hosts
 * 4. If forbidden: log denial, send rejection file, close socket, return NULL
 * 5. If allowed:
 *    - Send IP address to DNS resolver thread via message queue
 *    - Log connection opening
 *    - Initialize descriptor via initializesock()
 *    - Return initialized descriptor
 *
 * @warning Returns NULL on failure; caller must check return value
 * @see _bsd_sock_initialize() for descriptor initialization details
 */
static DESC *_bsd_conn_new(int sock)
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
		d = _bsd_sock_initialize(newsock, &addr);
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
static DESC *_bsd_sock_initialize(int s, struct sockaddr_in *a)
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
	bsd_socket_nonblocking_set(s);

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
inline char *bsd_conn_reason_string(int reason)
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
inline char *bsd_conn_message_string(int reason)
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
void _bsd_conn_shutdown(DESC *d, int reason)
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

			log_write(LOG_NET | LOG_LOGIN, "NET", "DISC", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, bsd_conn_reason_string(reason), d->command_count, d->input_tot, d->output_tot, conn_time);
		}
		else
		{
			log_write(LOG_NET | LOG_LOGIN, "NET", "LOGO", "[%d/%s] Logout by %s <%s: %d cmds, %d bytes in, %d bytes out, %d secs>", d->descriptor, d->addr, buff, bsd_conn_reason_string(reason), d->command_count, d->input_tot, d->output_tot, conn_time);
		}
		/* If requested, write an accounting record of the form: Plyr# Flags Cmds ConnTime Loc Money [Site] \<DiscRsn\> Name */
		now = mushstate.now - d->connected_at;
		buff2 = unparse_flags(GOD, d->player);
		log_write(LOG_ACCOUNTING, "DIS", "ACCT", "%d %s %d %d %d %d [%s] <%s> %s", d->player, buff2, d->command_count, (int)now, Location(d->player), Pennies(d->player), d->addr, bsd_conn_reason_string(reason), buff);
		XFREE(buff2);
		announce_disconnect(d->player, d, bsd_conn_message_string(reason));
	}
	else
	{
		if (reason == R_LOGOUT)
		{
			reason = R_QUIT;
		}

		log_write(LOG_SECURITY | LOG_NET, "NET", "DISC", "[%d/%s] Connection closed, never connected. <Reason: %s>", d->descriptor, d->addr, bsd_conn_reason_string(reason));
	}

	XFREE(buff);
	bsd_io_output_process(d);
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

/* Public exports for bsd_main.c */
DESC *bsd_conn_new(int sock)
{
	return _bsd_conn_new(sock);
}

DESC *bsd_sock_initialize(int s, struct sockaddr_in *a)
{
	return _bsd_sock_initialize(s, a);
}

void bsd_conn_shutdown(DESC *d, int reason)
{
	_bsd_conn_shutdown(d, reason);
}
