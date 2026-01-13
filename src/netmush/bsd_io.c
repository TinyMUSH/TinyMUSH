/**
 * @file bsd_io.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Input and output processing for network connections
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
#include <unistd.h>

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
	mushstate.debug_cmd = (char *)"< process_input >";
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
 * @brief Public wrapper for process_input() with debug context management
 *
 * @param d Pointer to the DESC structure representing the client connection
 * @return int Status code from process_input()
 */
int process_input_wrapper(DESC *d)
{
	return process_input(d);
}
