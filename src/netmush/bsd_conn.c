/**
 * @file bsd_conn.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Connection management and disconnection handling
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

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

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
