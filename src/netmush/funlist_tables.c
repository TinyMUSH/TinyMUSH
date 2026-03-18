/**
 * @file funlist_tables.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief List table and column formatting functions
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

#include <ctype.h>
#include <string.h>


static const ColorState color_normal = {.foreground = {.is_set = ColorStatusReset}, .background = {.is_set = ColorStatusReset}};
static const ColorState color_none = {0};

/**
 * @brief Copy delimiter structure (osep = isep)
 *
 * @param dest Destination Delim structure
 * @param src Source Delim structure
 *
 * @details Safely copies the delimiter information from src to dest,
 * handling the variable-length string field.
 */
static inline void copy_delim(Delim *dest, const Delim *src)
{
	if (!dest || !src)
	{
		return;
	}

	size_t copy_len = sizeof(Delim) - MAX_DELIM_LEN + 1 + src->len;
	XMEMCPY(dest, src, copy_len);
}


/**
 * @brief Consume a single ANSI escape sequence and update a ColorState.
 *
 * Advances the input cursor past a single ANSI escape sequence if one is
 * present at the current position, updating the provided `ColorState` to
 * reflect the effect of that sequence. If no valid escape sequence is
 * recognized, the cursor is advanced by one byte to avoid stalling.
 *
 * @param cursor Pointer to the input cursor (will be advanced)
 * @param state Pointer to the current `ColorState` to update
 */
static inline void consume_ansi_sequence_state(char **cursor, ColorState *state)
{
	const char *ptr = *cursor;

	if (ansi_apply_sequence(&ptr, state))
	{
		*cursor = (char *)ptr;
	}
	else
	{
		++(*cursor);
	}
}


/**
 * @brief Validate multiple delimiters for table functions
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function arguments
 * @param nfargs Number of function arguments
 * @param cargs Command arguments
 * @param ncargs Number of command arguments
 * @param list_sep Delimiter for list separation
 * @param field_sep Delimiter for field separation
 * @param pad_char Padding character
 * @param list_pos Position of list delimiter argument
 * @param field_pos Position of field delimiter argument
 * @param pad_pos Position of padding character argument
 * @return int 1 if valid, 0 if error
 *
 * @details Validates the list separator, field separator, and padding character
 * for table-related functions like fun_table.
 */
static int
validate_table_delims(char *buff, char **bufc, dbref player, dbref caller, dbref cause,
					  char *fargs[], int nfargs, char *cargs[], int ncargs,
					  Delim *list_sep, Delim *field_sep, Delim *pad_char,
					  int list_pos, int field_pos, int pad_pos)
{
	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, list_pos, list_sep, DELIM_STRING))
	{
		return 0;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, field_pos, field_sep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return 0;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, pad_pos, pad_char, 0))
	{
		return 0;
	}

	return 1;
}


/**
 * @brief Format a list into fixed-width columns, preserving ANSI color state.
 *
 * Splits the input list on the supplied delimiter, truncates or pads each
 * element to the requested width, and emits ANSI transitions so colored text
 * stays intact across column boundaries.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments: [0]=list, [1]=column width, [2]=optional delimiter
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 *
 * @details Each element is padded or truncated to fit the specified width,
 * and ANSI color states are preserved between elements.
 */
void fun_columns(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	unsigned int spaces, number, striplen;
	unsigned int count, i, indent = 0;
	ColorState ansi_state = color_none;
	int rturn = 1;
	size_t remaining;
	char *p, *q, *buf, *objstring, *cp, *cr = NULL;
	Delim isep;

	if (!validate_list_args(((FUN *)fargs[-1])->name, buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, 4, 3, DELIM_STRING, &isep))
	{
		return;
	}

	number = (int)strtol(fargs[1], (char **)NULL, 10);
	indent = (nfargs >= 4) ? (int)strtol(fargs[3], (char **)NULL, 10) : 0;

	if (indent > 77)
	{
		/**
		 * unsigned int, always a positive number
		 *
		 */
		indent = 1;
	}

	/**
	 * Must check number separately, since number + indent can result in
	 * an integer overflow.
	 *
	 */
	if ((number < 1) || (number > 77) || ((unsigned int)(number + indent) > 78))
	{
		XSAFELBSTR("#-1 OUT OF RANGE", buff, bufc);
		return;
	}

	cp = trim_space_sep(fargs[0], &isep);

	if (!*cp)
	{
		return;
	}

	for (i = 0; i < indent; i++)
	{
		XSAFELBCHR(' ', buff, bufc);
	}

	buf = XMALLOC(LBUF_SIZE, "buf");

	while (cp)
	{
		objstring = split_token(&cp, &isep);
		striplen = ansi_strip_ansi_len(objstring);

		p = objstring;
		q = buf;
		count = 0;
		ansi_state = color_none;

		while (p && *p)
		{
			if (count == number)
			{
				break;
			}

			if (*p == C_ANSI_ESC)
			{
				char *seq_start = p;
				char *seq_end;
				ColorState prev_state = ansi_state;
				size_t seq_len;

				consume_ansi_sequence_state(&p, &ansi_state);
				seq_end = p;
				remaining = LBUF_SIZE - 1 - (size_t)(q - buf);

				if (!remaining)
				{
					break;
				}

				seq_len = seq_end - seq_start;

				if (seq_len > remaining)
				{
					p = seq_start;
					ansi_state = prev_state;
					break;
				}

				XMEMCPY(q, seq_start, seq_len);
				q += seq_len;
			}
			else
			{
				if ((size_t)(q - buf) >= LBUF_SIZE - 1)
				{
					break;
				}

				*q++ = *p++;
				count++;
			}
		}

		if (memcmp(&ansi_state, &color_none, sizeof(ColorState)) != 0)
		{
			char *reset_seq = ansi_transition_colorstate(ansi_state, color_none, ColorTypeTrueColor, false);
			size_t reset_len = strlen(reset_seq);
			remaining = LBUF_SIZE - 1 - (size_t)(q - buf);

			if (reset_len > remaining)
			{
				reset_len = remaining;
			}

			XMEMCPY(q, reset_seq, reset_len);
			q += reset_len;
			XFREE(reset_seq);
		}

		*q = '\0';
		XSAFELBSTR(buf, buff, bufc);

		if (striplen < number)
		{
			/**
			 * We only need spaces if we need to pad out.
			 * Sanitize the number of spaces, too.
			 *
			 */
			spaces = number - striplen;

			if (spaces > LBUF_SIZE)
			{
				spaces = LBUF_SIZE;
			}

			for (i = 0; i < spaces; i++)
			{
				XSAFELBCHR(' ', buff, bufc);
			}
		}

		if (!(rturn % (int)((78 - indent) / number)))
		{
			XSAFECRLF(buff, bufc);
			cr = *bufc;

			for (i = 0; i < indent; i++)
			{
				XSAFELBCHR(' ', buff, bufc);
			}
		}
		else
		{
			cr = NULL;
		}

		rturn++;
	}

	if (cr)
	{
		*bufc = cr;
		**bufc = '\0';
	}
	else
	{
		XSAFECRLF(buff, bufc);
	}

	XFREE(buf);
}

/**
 * @brief Helper function for perform_tables
 *
 * @param list Input list to format into table
 * @param last_state Current ANSI color state
 * @param n_cols Number of columns in the table
 * @param col_widths Array of column widths
 * @param lead_str Leading string for each row
 * @param trail_str Trailing string for each row
 * @param list_sep Separator between list items
 * @param field_sep Separator between fields in items
 * @param pad_char Padding character for alignment
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param just Justification flag
 *
 * @details Formats the list into a table with specified columns, preserving ANSI colors.
 */
void tables_helper(char *list, ColorState *last_state, int n_cols, int col_widths[], char *lead_str, char *trail_str, const Delim *list_sep, const Delim *field_sep, const Delim *pad_char, char *buff, char **bufc, int just)
{
	int i = 0, nwords = 0, nstates = 0, cpos = 0, wcount = 0, over = 0;
	int max = 0, nleft = 0, lead_chrs = 0, lens[LBUF_SIZE / 2];
	ColorState ansi_state = color_none;
	ColorState states[LBUF_SIZE / 2 + 1];
	char *s = NULL, **words = NULL, *buf = NULL;
	/**
	 * Split apart the list. We need to find the length of each
	 * de-ansified word, as well as keep track of the state of each word.
	 * Overly-long words eventually get truncated, but the correct ANSI
	 * state is preserved nonetheless.
	 *
	 */
	nstates = list2ansi(states, last_state, LBUF_SIZE / 2, list, list_sep);
	nwords = list2arr(&words, LBUF_SIZE / 2, list, list_sep);

	if (nstates != nwords)
	{
		XSAFESPRINTF(buff, bufc, "#-1 STATE/WORD COUNT OFF: %d/%d", nstates, nwords);
		XFREE(words);
		return;
	}

	for (i = 0; i < nwords; i++)
	{
		lens[i] = ansi_strip_ansi_len(words[i]);
	}

	over = wcount = 0;

	while ((wcount < nwords) && !over)
	{
		/**
		 * Beginning of new line. Insert newline if this isn't the
		 * first thing we're writing. Write left margin, if
		 * appropriate.
		 *
		 */
		if (wcount != 0)
		{
			XSAFECRLF(buff, bufc);
		}

		if (lead_str)
		{
			over = XSAFELBSTR(lead_str, buff, bufc);
		}

		/**
		 * Do each column in the line.
		 *
		 */
		for (cpos = 0; (cpos < n_cols) && (wcount < nwords) && !over; cpos++, wcount++)
		{
			/**
			 * Write leading padding if we need it.
			 *
			 */
			if (just == JUST_RIGHT)
			{
				nleft = col_widths[cpos] - lens[wcount];

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, pad_char->str[0], nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}
			else if (just == JUST_CENTER)
			{
				lead_chrs = (int)((col_widths[cpos] / 2) - (lens[wcount] / 2) + .5);

				if (lead_chrs > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					lead_chrs = (lead_chrs > max) ? max : lead_chrs;
					XMEMSET(*bufc, pad_char->str[0], lead_chrs);
					*bufc += lead_chrs;
					**bufc = '\0';
				}
			}

			/**
			 * If we had a previous state, we have to write it.
			 *
			 */
			buf = ansi_transition_colorstate(color_none, states[wcount], ColorTypeTrueColor, false);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);

			/**
			 * Copy in the word.
			 *
			 */
			if (lens[wcount] <= col_widths[cpos])
			{
				over = XSAFELBSTR(words[wcount], buff, bufc);
				buf = ansi_transition_colorstate(states[wcount + 1], color_none, ColorTypeTrueColor, false);
				XSAFELBSTR(buf, buff, bufc);
				XFREE(buf);
			}
			else
			{
				/**
				 * Bleah. We have a string that's too long.
				 * Truncate it. Write an ANSI normal at the
				 * end at the end if we need one (we'll
				 * restore the correct ANSI code with the
				 * next word, if need be).
				 *
				 */
				ansi_state = states[wcount];

				for (s = words[wcount], i = 0; *s && (i < col_widths[cpos]);)
				{
					if (*s == C_ANSI_ESC)
					{
						consume_ansi_sequence_state(&s, &ansi_state);
					}
					else
					{
						s++;
						i++;
					}
				}

				XSAFESTRNCAT(buff, bufc, words[wcount], s - words[wcount], LBUF_SIZE);
				buf = ansi_transition_colorstate(ansi_state, color_none, ColorTypeTrueColor, false);
				XSAFELBSTR(buf, buff, bufc);
				XFREE(buf);
			}

			/**
			 * Writing trailing padding if we need it.
			 *
			 */
			if (just & JUST_LEFT)
			{
				nleft = col_widths[cpos] - lens[wcount];

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, pad_char->str[0], nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}
			else if (just & JUST_CENTER)
			{
				nleft = col_widths[cpos] - lead_chrs - lens[wcount];

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, pad_char->str[0], nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}

			/**
			 * Insert the field separator if this isn't the last
			 * column AND this is not the very last word in the
			 * list.
			 *
			 */
			if ((cpos < n_cols - 1) && (wcount < nwords - 1))
			{
				print_separator(field_sep, buff, bufc);
			}
		}

		if (!over && trail_str)
		{
			/**
			 * If we didn't get enough columns to fill out a
			 * line, and this is the last line, then we have to
			 * pad it out.
			 *
			 */
			if ((wcount == nwords) && ((nleft = nwords % n_cols) > 0))
			{
				for (cpos = nleft; (cpos < n_cols) && !over; cpos++)
				{
					print_separator(field_sep, buff, bufc);

					if (col_widths[cpos] > 0)
					{
						max = LBUF_SIZE - 1 - (*bufc - buff);
						col_widths[cpos] = (col_widths[cpos] > max) ? max : col_widths[cpos];
						XMEMSET(*bufc, pad_char->str[0], col_widths[cpos]);
						*bufc += col_widths[cpos];
						**bufc = '\0';
					}
				}
			}

			/**
			 * Write the right margin.
			 *
			 */
			over = XSAFELBSTR(trail_str, buff, bufc);
		}
	}

	/**
	 * Save the ANSI state of the last word.
	 *
	 */
	*last_state = states[nstates - 1];

	/**
	 * Clean up.
	 *
	 */
	XFREE(words);
}

/**
 * @brief Draw a table
 *
 * @param player DBref of player (unused)
 * @param list List to draw in table
 * @param n_cols Number of columns
 * @param col_widths Array of column widths
 * @param lead_str Leading string for each row
 * @param trail_str Trailing string for each row
 * @param list_sep Separator between list items
 * @param field_sep Separator between fields in items
 * @param pad_char Padding character
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param just Justification flag for the table
 *
 * @details Formats the input list into a multi-column table with specified widths and separators.
 */
void perform_tables(dbref player, char *list, int n_cols, int col_widths[], char *lead_str, char *trail_str, const Delim *list_sep, const Delim *field_sep, const Delim *pad_char, char *buff, char **bufc, int just)
{
	char *p, *savep, *bb_p;
	ColorState ansi_state = color_none;

	if (!list || !*list)
	{
		return;
	}

	bb_p = *bufc;
	savep = list;
	p = strchr(list, '\r');

	while (p)
	{
		*p = '\0';

		if (*bufc != bb_p)
		{
			XSAFECRLF(buff, bufc);
		}

		tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str, list_sep, field_sep, pad_char, buff, bufc, just);
		savep = p + 2; /*!< must skip '\n' too */
		p = strchr(savep, '\r');
	}

	if (*bufc != bb_p)
	{
		XSAFECRLF(buff, bufc);
	}

	tables_helper(savep, &ansi_state, n_cols, col_widths, lead_str, trail_str, list_sep, field_sep, pad_char, buff, bufc, just);
}

/**
 * @brief Validate that we have everything to draw the table
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments: [0]=list, [1]=column specs, [2]=optional delimiters
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 *
 * @details Parses column specifications and calls perform_tables to format the table.
 */
void process_tables(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int just;
	int i, num, n_columns, *col_widths;
	Delim list_sep, field_sep, pad_char;
	char **widths;
	just = Func_Mask(JUST_TYPE);

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 7, buff, bufc))
	{
		return;
	}

	if (!validate_table_delims(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, &list_sep, &field_sep, &pad_char, 5, 6, 7))
	{
		return;
	}

	n_columns = list2arr(&widths, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM);

	if (n_columns < 1)
	{
		XFREE(widths);
		return;
	}

	col_widths = (int *)XCALLOC(n_columns, sizeof(int), "col_widths");

	for (i = 0; i < n_columns; i++)
	{
		num = (int)strtol(widths[i], (char **)NULL, 10);
		col_widths[i] = (num < 1) ? 1 : num;
	}

	perform_tables(player, fargs[0], n_columns, col_widths, ((nfargs > 2) && *fargs[2]) ? fargs[2] : NULL, ((nfargs > 3) && *fargs[3]) ? fargs[3] : NULL, &list_sep, &field_sep, &pad_char, buff, bufc, just);
	XFREE(col_widths);
	XFREE(widths);
}

/*---------------------------------------------------------------------------
 * fun_table:
 *   table(&lt;list&gt;,&lt;field width&gt;,&lt;line length&gt;,&lt;list delim&gt;,&lt;field sep&gt;,&lt;pad&gt;)
 *     Only the &lt;list&gt; parameter is mandatory.
 *   tables(&lt;list&gt;,&lt;field widths&gt;,&lt;lead str&gt;,&lt;trail str&gt;,
 *          &lt;list delim&gt;,&lt;field sep str&gt;,&lt;pad&gt;)
 *     Only the &lt;list&gt; and &lt;field widths&gt; parameters are mandatory.
 *
 * There are a couple of PennMUSH incompatibilities. The handling here is
 * more complex and probably more desirable behavior. The issues are:
 *   - ANSI states are preserved even if a word is truncated. Thus, the
 *     next word will start with the correct color.
 *   - ANSI does not bleed into the padding or field separators.
 *   - Having a '%r' embedded in the list will start a new set of columns.
 *     This allows a series of %r-separated lists to be table-ified
 *     correctly, and doesn't mess up the character count.
 */

/**
 * @brief Turn a list into a table.
 *
 *   table(&lt;list&gt;,&lt;field width&gt;,&lt;line length&gt;,&lt;list delim&gt;,&lt;field sep&gt;,&lt;pad&gt;)
 *     Only the &lt;list&gt; parameter is mandatory.
 *   tables(&lt;list&gt;,&lt;field widths&gt;,&lt;lead str&gt;,&lt;trail str&gt;,&lt;list delim&gt;,&lt;field sep str&gt;,&lt;pad&gt;)
 *     Only the &lt;list&gt; and &lt;field widths&gt; parameters are mandatory.
 *
 * There are a couple of PennMUSH incompatibilities. The handling here is
 * more complex and probably more desirable behavior. The issues are:
 *   - ANSI states are preserved even if a word is truncated. Thus, the
 *     next word will start with the correct color.
 *   - ANSI does not bleed into the padding or field separators.
 *   - Having a '%r' embedded in the list will start a new set of columns.
 *     This allows a series of %r-separated lists to be table-ified
 *     correctly, and doesn't mess up the character count.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Number of command's arguments
 */
void fun_table(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int line_length = 78;
	int field_width = 10;
	int just = JUST_LEFT;
	int i, field_sep_width, n_columns, *col_widths;
	char *p;
	Delim list_sep, field_sep, pad_char;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 6, buff, bufc))
	{
		return;
	}

	if (!validate_table_delims(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, &list_sep, &field_sep, &pad_char, 4, 5, 6))
	{
		return;
	}

	/**
	 * Get line length and column width. All columns are the same width.
	 * Calculate what we need to.
	 *
	 */
	if (nfargs > 2)
	{
		line_length = (int)strtol(fargs[2], (char **)NULL, 10);

		if (line_length < 2)
		{
			line_length = 2;
		}
	}

	if (nfargs > 1)
	{
		p = fargs[1];

		switch (*p)
		{
		case '<':
			just = JUST_LEFT;
			p++;
			break;

		case '>':
			just = JUST_RIGHT;
			p++;
			break;

		case '-':
			just = JUST_CENTER;
			p++;
			break;
		}

		field_width = (int)strtol(p, (char **)NULL, 10);

		if (field_width < 1)
		{
			field_width = 1;
		}
		else if (field_width > LBUF_SIZE - 1)
		{
			field_width = LBUF_SIZE - 1;
		}
	}

	if (field_width >= line_length)
	{
		field_width = line_length - 1;
	}

	if (field_sep.len == 1)
	{
		switch (field_sep.str[0])
		{
		case '\r':
		case '\0':
		case '\n':
		case '\a':
			field_sep_width = 0;
			break;

		default:
			field_sep_width = 1;
			break;
		}
	}
	else
	{
		field_sep_width = ansi_strip_ansi_len(field_sep.str);
	}

	n_columns = (int)(line_length / (field_width + field_sep_width));
	col_widths = (int *)XCALLOC(n_columns, sizeof(int), "col_widths");

	for (i = 0; i < n_columns; i++)
	{
		col_widths[i] = field_width;
	}

	perform_tables(player, fargs[0], n_columns, col_widths, NULL, NULL, &list_sep, &field_sep, &pad_char, buff, bufc, just);
	XFREE(col_widths);
}

