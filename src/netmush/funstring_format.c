/**
 * @file funstring_format.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Text formatting and alignment functions
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
#include "ansi.h"

#include <ctype.h>
#include <string.h>

static const ColorState color_normal = {
	.foreground = {.is_set = ColorStatusReset},
	.background = {.is_set = ColorStatusReset}};


/**
 * @brief Make spaces.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_space(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int num = 0, max = 0;

	if (!fargs[0] || !(*fargs[0]))
	{
		num = 1;
	}
	else
	{
		num = (int)strtol(fargs[0], (char **)NULL, 10);
	}

	if (num < 1)
	{
		/**
		 * If negative or zero spaces return a single space, -except-
		 * allow 'space(0)' to return "" for calculated padding
		 *
		 */
		if (!is_integer(fargs[0]) || (num != 0))
		{
			num = 1;
		}
	}

	max = LBUF_SIZE - 1 - (*bufc - buff);
	num = (num > max) ? max : num;
	XMEMSET(*bufc, ' ', num);
	*bufc += num;
	**bufc = '\0';
}

/*
 * ---------------------------------------------------------------------------
 * rjust, ljust, center: Justify or center text, specifying fill character
 */

/**
 * @brief Left justify string, specifying fill character
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ljust(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int spaces = 0, max = 0, i = 0, slen = 0;
	char *tp = NULL, *fillchars = NULL;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	spaces = (int)strtol(fargs[1], (char **)NULL, 10) - ansi_strip_ansi_len(fargs[0]);
	XSAFELBSTR(fargs[0], buff, bufc);

	/**
	 * Sanitize number of spaces
	 *
	 */
	if (spaces <= 0)
	{
		/**
		 * no padding needed, just return string
		 *
		 */
		return;
	}

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /*!< chars left in buffer */
	spaces = (spaces > max) ? max : spaces;

	if (fargs[2])
	{
		fillchars = ansi_strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > spaces) ? spaces : slen;

		if (slen == 0)
		{
			/**
			 * NULL character fill
			 *
			 */
			XMEMSET(tp, ' ', spaces);
			tp += spaces;
		}
		else if (slen == 1)
		{
			/**
			 * single character fill
			 *
			 */
			XMEMSET(tp, *fillchars, spaces);
			tp += spaces;
		}
		else
		{
			/**
			 * multi character fill
			 *
			 */
			for (i = spaces; i >= slen; i -= slen)
			{
				XMEMCPY(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/**
				 * we have a remainder here
				 *
				 */
				XMEMCPY(tp, fillchars, i);
				tp += i;
			}
		}

		XFREE(fillchars);
	}
	else
	{
		/**
		 * no fill character specified
		 *
		 */
		XMEMSET(tp, ' ', spaces);
		tp += spaces;
	}

	*tp = '\0';
	*bufc = tp;
}

/**
 * @brief Right justify string, specifying fill character
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_rjust(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int spaces = 0, max = 0, i = 0, slen = 0;
	char *tp = NULL, *fillchars = NULL;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	spaces = (int)strtol(fargs[1], (char **)NULL, 10) - ansi_strip_ansi_len(fargs[0]);

	/**
	 * Sanitize number of spaces
	 *
	 */
	if (spaces <= 0)
	{
		/**
		 * no padding needed, just return string
		 *
		 */
		XSAFELBSTR(fargs[0], buff, bufc);
		return;
	}

	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /*!< chars left in buffer */
	spaces = (spaces > max) ? max : spaces;

	if (fargs[2])
	{
		fillchars = ansi_strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > spaces) ? spaces : slen;

		if (slen == 0)
		{
			/**
			 * NULL character fill
			 *
			 */
			XMEMSET(tp, ' ', spaces);
			tp += spaces;
		}
		else if (slen == 1)
		{
			/**
			 * single character fill
			 *
			 */
			XMEMSET(tp, *fillchars, spaces);
			tp += spaces;
		}
		else
		{
			/**
			 * multi character fill
			 *
			 */
			for (i = spaces; i >= slen; i -= slen)
			{
				XMEMCPY(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/**
				 * we have a remainder here
				 *
				 */
				XMEMCPY(tp, fillchars, i);
				tp += i;
			}
		}

		XFREE(fillchars);
	}
	else
	{
		/**
		 * no fill character specified
		 *
		 */
		XMEMSET(tp, ' ', spaces);
		tp += spaces;
	}

	*bufc = tp;
	XSAFELBSTR(fargs[0], buff, bufc);
}

/**
 * @brief Center string, specifying fill character
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_center(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tp = NULL, *fillchars = NULL;
	int len = 0, lead_chrs = 0, trail_chrs = 0, width = 0, max = 0, i = 0, slen = 0;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	width = (int)strtol(fargs[1], (char **)NULL, 10);
	len = ansi_strip_ansi_len(fargs[0]);
	width = (width > LBUF_SIZE - 1) ? LBUF_SIZE - 1 : width;

	if (len >= width)
	{
		XSAFELBSTR(fargs[0], buff, bufc);
		return;
	}

	lead_chrs = (int)((width / 2) - (len / 2) + .5);
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff); /*!< chars left in buffer */
	lead_chrs = (lead_chrs > max) ? max : lead_chrs;

	if (fargs[2])
	{
		fillchars = (char *)ansi_strip_ansi(fargs[2]);
		slen = strlen(fillchars);
		slen = (slen > lead_chrs) ? lead_chrs : slen;

		if (slen == 0)
		{
			/**
			 * NULL character fill
			 *
			 */
			XMEMSET(tp, ' ', lead_chrs);
			tp += lead_chrs;
		}
		else if (slen == 1)
		{
			/**
			 * single character fill
			 *
			 */
			XMEMSET(tp, *fillchars, lead_chrs);
			tp += lead_chrs;
		}
		else
		{
			/**
			 * multi character fill
			 *
			 */
			for (i = lead_chrs; i >= slen; i -= slen)
			{
				XMEMCPY(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/**
				 * we have a remainder here
				 *
				 */
				XMEMCPY(tp, fillchars, i);
				tp += i;
			}
		}

		XFREE(fillchars);
	}
	else
	{
		/**
		 * no fill character specified
		 *
		 */
		XMEMSET(tp, ' ', lead_chrs);
		tp += lead_chrs;
	}

	*bufc = tp;
	XSAFELBSTR(fargs[0], buff, bufc);
	trail_chrs = width - lead_chrs - len;
	tp = *bufc;
	max = LBUF_SIZE - 1 - (tp - buff);
	trail_chrs = (trail_chrs > max) ? max : trail_chrs;

	if (fargs[2])
	{
		if (slen == 0)
		{
			/**
			 * NULL character fill
			 *
			 */
			XMEMSET(tp, ' ', trail_chrs);
			tp += trail_chrs;
		}
		else if (slen == 1)
		{
			/**
			 * single character fill
			 *
			 */
			XMEMSET(tp, *fillchars, trail_chrs);
			tp += trail_chrs;
		}
		else
		{
			/**
			 * multi character fill
			 *
			 */
			for (i = trail_chrs; i >= slen; i -= slen)
			{
				XMEMCPY(tp, fillchars, slen);
				tp += slen;
			}

			if (i)
			{
				/**
				 * we have a remainder here
				 *
				 */
				XMEMCPY(tp, fillchars, i);
				tp += i;
			}
		}
	}
	else
	{
		/**
		 * no fill character specified
		 *
		 */
		XMEMSET(tp, ' ', trail_chrs);
		tp += trail_chrs;
	}

	*tp = '\0';
	*bufc = tp;
}

/**
 * @brief Returns first n characters in a string
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_left(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	ColorState *states = NULL;
	char *stripped = NULL;
	ColorState normal = color_normal;
	ColorType color_type = ColorTypeTrueColor;
	int nchars = (int)strtol(fargs[1], (char **)NULL, 10);

	if (nchars <= 0)
	{
		return;
	}

	int len = ansi_map_states_colorstate(fargs[0], &states, &stripped);

	if (nchars > len)
	{
		nchars = len;
	}

	emit_colored_range(stripped, states, 0, nchars, normal, normal, color_type, buff, bufc);

	if (states)
	{
		XFREE(states);
	}

	if (stripped)
	{
		XFREE(stripped);
	}
}

/**
 * @brief fun_right: Returns last n characters in a string
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_right(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	ColorState *states = NULL;
	char *stripped = NULL;
	ColorState normal = color_normal;
	ColorType color_type = ColorTypeTrueColor;
	int nchars = (int)strtol(fargs[1], (char **)NULL, 10);

	if (nchars <= 0)
	{
		return;
	}

	int len = ansi_map_states_colorstate(fargs[0], &states, &stripped);
	int start = len - nchars;

	if (start < 0)
	{
		nchars += start;
		start = 0;
	}

	if (nchars <= 0 || start > len)
	{
		goto cleanup_right;
	}

	int end = start + nchars;
	if (end > len)
	{
		end = len;
	}

	emit_colored_range(stripped, states, start, end, normal, states ? states[len] : normal, color_type, buff, bufc);

cleanup_right:
	if (states)
	{
		XFREE(states);
	}

	if (stripped)
	{
		XFREE(stripped);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_repeat: repeats a string
 */

void fun_repeat(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int times, len, i, maxtimes;
	times = (int)strtol(fargs[1], (char **)NULL, 10);

	if ((times < 1) || (fargs[0] == NULL) || (!*fargs[0]))
	{
		return;
	}
	else if (times == 1)
	{
		XSAFELBSTR(fargs[0], buff, bufc);
	}
	else
	{
		len = strlen(fargs[0]);
		maxtimes = (LBUF_SIZE - 1 - (*bufc - buff)) / len;
		maxtimes = (times > maxtimes) ? maxtimes : times;

		for (i = 0; i < maxtimes; i++)
		{
			XMEMCPY(*bufc, fargs[0], len);
			*bufc += len;
		}

		if (times > maxtimes)
		{
			XSAFESTRNCAT(buff, bufc, fargs[0], len, LBUF_SIZE);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * perform_border: Turn a string of words into a bordered paragraph: BORDER,
 * CBORDER, RBORDER border(<words>,<width without margins>[,<L margin
 * fill>[,<R margin fill>]])
 */

void perform_border(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *l_fill = NULL, *r_fill = NULL, *bb_p = NULL, *sl = NULL, *el = NULL, *sw = NULL, *ew = NULL, *buf = NULL;
	ColorState sl_ansi_state = color_normal, el_ansi_state = color_normal, sw_ansi_state = color_normal, ew_ansi_state = color_normal;
	int sl_pos = 0;
	int el_pos = 0, sw_pos = 0, ew_pos = 0, width = 0, just = 0, nleft = 0, max = 0, lead_chrs = 0;

	just = Func_Mask(JUST_TYPE);

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
	{
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	width = (int)strtol(fargs[1], (char **)NULL, 10);

	if (width < 1)
	{
		width = 1;
	}

	if (nfargs > 2)
	{
		l_fill = fargs[2];
	}
	else
	{
		l_fill = NULL;
	}

	if (nfargs > 3)
	{
		r_fill = fargs[3];
	}
	else
	{
		r_fill = NULL;
	}

	bb_p = *bufc;
	sl = el = sw = NULL;
	ew = fargs[0];
	sl_ansi_state = el_ansi_state = color_normal;
	sw_ansi_state = ew_ansi_state = color_normal;
	sl_pos = el_pos = sw_pos = ew_pos = 0;
	ColorType color_type = ColorTypeTrueColor;
	const ColorState ansi_normal = color_normal;

	while (1)
	{
		/*
		 * Locate the next start-of-word (SW)
		 */
		for (sw = ew, sw_ansi_state = ew_ansi_state, sw_pos = ew_pos; *sw; ++sw)
		{
			switch (*sw)
			{
			case C_ANSI_ESC:
			{
				consume_ansi_sequence_state(&sw, &sw_ansi_state);
				--sw;
				continue;
			}

			case '\t':
			case '\r':
				*sw = ' ';
				++sw_pos;
				continue;
			case ' ':
				++sw_pos;
				continue;
			case BEEP_CHAR:
				continue;
			default:
				break;
			}

			break;
		}

		/*
		 * Three ways out of that loop: end-of-string (ES),
		 * end-of-line (EL), and start-of-word (SW)
		 */
		if (!*sw && sl == NULL)
		{ /* ES, and nothing left to output */
			break;
		}

		/*
		 * Decide where start-of-line (SL) was
		 */
		if (sl == NULL)
		{
			if (ew == fargs[0] || ew[-1] == '\n')
			{
				/*
				 * Preserve indentation at SS or after
				 * explicit EL
				 */
				sl = ew;
				sl_ansi_state = ew_ansi_state;
				sl_pos = ew_pos;
			}
			else
			{
				/*
				 * Discard whitespace if previous line
				 * wrapped
				 */
				sl = sw;
				sl_ansi_state = sw_ansi_state;
				sl_pos = sw_pos;
			}
		}

		if (*sw == '\n')
		{ /* EL, so we have to output */
			ew = sw;
			ew_ansi_state = sw_ansi_state;
			ew_pos = sw_pos;
		}
		else
		{
			/*
			 * Locate the next end-of-word (EW)
			 */
			for (ew = sw, ew_ansi_state = sw_ansi_state, ew_pos = sw_pos; *ew; ++ew)
			{
				switch (*ew)
				{
				case C_ANSI_ESC:
				{
					consume_ansi_sequence_state(&ew, &ew_ansi_state);
					--ew;
					continue;
				}

				case '\r':
				case '\t':
					*ew = ' ';
					break;
				case ' ':
				case '\n':
					break;

				case BEEP_CHAR:
					continue;

				default:

					/*
					 * Break up long words
					 */
					if (ew_pos - sw_pos == width)
					{
						break;
					}

					++ew_pos;
					continue;
				}

				break;
			}

			/*
			 * Three ways out of that loop: ES, EL, EW
			 */

			/*
			 * If it fits on the line, add it
			 */
			if (ew_pos - sl_pos <= width)
			{
				el = ew;
				el_ansi_state = ew_ansi_state;
				el_pos = ew_pos;
			}

			/*
			 * If it's just EW, not ES or EL, and the line isn't
			 * too long, keep adding words to the line
			 */
			if (*ew && *ew != '\n' && (ew_pos - sl_pos <= width))
			{
				continue;
			}

			/*
			 * So now we definitely need to output a line
			 */
		}

		/*
		 * Could be a blank line, no words fit
		 */
		if (el == NULL)
		{
			el = sw;
			el_ansi_state = sw_ansi_state;
			el_pos = sw_pos;
		}

		/*
		 * Newline if this isn't the first line
		 */
		if (*bufc != bb_p)
		{
			XSAFECRLF(buff, bufc);
		}

		/*
		 * Left border text
		 */
		XSAFELBSTR(l_fill, buff, bufc);

		/*
		 * Left space padding if needed
		 */
		if (just == JUST_RIGHT)
		{
			nleft = width - el_pos + sl_pos;

			if (nleft > 0)
			{
				max = LBUF_SIZE - 1 - (*bufc - buff);
				nleft = (nleft > max) ? max : nleft;
				XMEMSET(*bufc, ' ', nleft);
				*bufc += nleft;
				**bufc = '\0';
			}
		}
		else if (just == JUST_CENTER)
		{
			lead_chrs = (int)((width / 2) - ((el_pos - sl_pos) / 2) + .5);

			if (lead_chrs > 0)
			{
				max = LBUF_SIZE - 1 - (*bufc - buff);
				lead_chrs = (lead_chrs > max) ? max : lead_chrs;
				XMEMSET(*bufc, ' ', lead_chrs);
				*bufc += lead_chrs;
				**bufc = '\0';
			}
		}

		/*
		 * Restore previous ansi state
		 */
		buf = ansi_transition_colorstate(color_normal, sl_ansi_state, color_type, false);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
		/*
		 * Print the words
		 */
		XSAFESTRNCAT(buff, bufc, sl, el - sl, LBUF_SIZE);
		/*
		 * Back to ansi normal
		 */
		buf = ansi_transition_colorstate(el_ansi_state, color_normal, color_type, false);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);

		/*
		 * Right space padding if needed
		 */
		if (just == JUST_LEFT)
		{
			nleft = width - el_pos + sl_pos;

			if (nleft > 0)
			{
				max = LBUF_SIZE - 1 - (*bufc - buff);
				nleft = (nleft > max) ? max : nleft;
				XMEMSET(*bufc, ' ', nleft);
				*bufc += nleft;
				**bufc = '\0';
			}
		}
		else if (just == JUST_CENTER)
		{
			nleft = width - lead_chrs - el_pos + sl_pos;

			if (nleft > 0)
			{
				max = LBUF_SIZE - 1 - (*bufc - buff);
				nleft = (nleft > max) ? max : nleft;
				XMEMSET(*bufc, ' ', nleft);
				*bufc += nleft;
				**bufc = '\0';
			}
		}

		/*
		 * Right border text
		 */
		XSAFELBSTR(r_fill, buff, bufc);

		/*
		 * Update pointers for the next line
		 */
		if (!*el)
		{
			/*
			 * ES, and nothing left to output
			 */
			break;
		}
		else if (*ew == '\n' && sw == ew)
		{
			/*
			 * EL already handled on this line, and no new word
			 * yet
			 */
			++ew;
			sl = el = NULL;
		}
		else if (sl == sw)
		{
			/*
			 * No new word yet
			 */
			sl = el = NULL;
		}
		else
		{
			/*
			 * ES with more to output, EL for next line, or just
			 * a full line
			 */
			sl = sw;
			sl_ansi_state = sw_ansi_state;
			sl_pos = sw_pos;
			el = ew;
			el_ansi_state = ew_ansi_state;
			el_pos = ew_pos;
		}
	}
}

/*---------------------------------------------------------------------------
 * fun_align: Turn a set of lists into newspaper-like columns.
 *   align(<widths>,<col1>,...,<colN>,<fill char>,<col sep>,<row sep>)
 *   lalign(<widths>,<col data>,<delim>,<fill char>,<col sep>,<row sep>)
 *   Only <widths> and the column data parameters are mandatory.
 *
 * This is mostly PennMUSH-compatible, but not 100%.
 *   - ANSI is not stripped out of the column text. States will be correctly
 *     preserved, and will not bleed.
 *   - ANSI states are not supported in the widths, as they are unnecessary.
 */

void perform_align(int n_cols, char **raw_colstrs, char **data, char fillc, Delim col_sep, Delim row_sep, char *buff, char **bufc, dbref player, dbref cause)
{
	char *bb_p = NULL, *l_p = NULL, **xsl = NULL, **xel = NULL, **xsw = NULL, **xew = NULL;
	char *p = NULL, *sl = NULL, *el = NULL, *sw = NULL, *ew = NULL, *buf = NULL;
	ColorState *xsl_a = NULL, *xel_a = NULL, *xsw_a = NULL, *xew_a = NULL;
	int *xsl_p = NULL, *xel_p = NULL, *xsw_p = NULL, *xew_p = NULL;
	int *col_widths = NULL, *col_justs = NULL, *col_done = NULL;
	int i = 0, n = 0;
	ColorState sl_ansi_state = {0}, el_ansi_state = {0}, sw_ansi_state = {0}, ew_ansi_state = {0};
	int sl_pos = 0, el_pos = 0, sw_pos = 0, ew_pos = 0;
	int width = 0, just = 0, nleft = 0, max = 0, lead_chrs = 0;
	int n_done = 0, pending_coaright = 0;
	const ColorState ansi_normal = color_normal;
	const ColorType color_type = ColorTypeTrueColor;
	/*
	 * Parse the column widths and justifications
	 */
	col_widths = (int *)XCALLOC(n_cols, sizeof(int), "col_widths");
	col_justs = (int *)XCALLOC(n_cols, sizeof(int), "col_justs");

	for (i = 0; i < n_cols; i++)
	{
		p = raw_colstrs[i];

		switch (*p)
		{
		case '<':
			col_justs[i] = JUST_LEFT;
			p++;
			break;

		case '>':
			col_justs[i] = JUST_RIGHT;
			p++;
			break;

		case '-':
			col_justs[i] = JUST_CENTER;
			p++;
			break;

		default:
			col_justs[i] = JUST_LEFT;
		}

		for (n = 0; *p && isdigit((unsigned char)*p); p++)
		{
			n *= 10;
			n += *p - '0';
		}

		if (n < 1)
		{
			XSAFELBSTR("#-1 INVALID COLUMN WIDTH", buff, bufc);
			XFREE(col_widths);
			XFREE(col_justs);
			return;
		}

		col_widths[i] = n;

		switch (*p)
		{
		case '.':
			col_justs[i] |= JUST_REPEAT;
			p++;
			break;

		case '`':
			col_justs[i] |= JUST_COALEFT;
			p++;
			break;

		case '\'':
			col_justs[i] |= JUST_COARIGHT;
			p++;
			break;
		}

		if (*p)
		{
			XSAFELBSTR("#1 INVALID ALIGN STRING", buff, bufc);
			XFREE(col_widths);
			XFREE(col_justs);
			return;
		}
	}

	col_done = (int *)XCALLOC(n_cols, sizeof(int), "col_done");
	xsl = (char **)XCALLOC(n_cols, sizeof(char *), "xsl");
	xel = (char **)XCALLOC(n_cols, sizeof(char *), "xel");
	xsw = (char **)XCALLOC(n_cols, sizeof(char *), "xsw");
	xew = (char **)XCALLOC(n_cols, sizeof(char *), "xew");
	xsl_a = (ColorState *)XCALLOC(n_cols, sizeof(ColorState), "xsl_a");
	xel_a = (ColorState *)XCALLOC(n_cols, sizeof(ColorState), "xel_a");
	xsw_a = (ColorState *)XCALLOC(n_cols, sizeof(ColorState), "xsw_a");
	xew_a = (ColorState *)XCALLOC(n_cols, sizeof(ColorState), "xew_a");
	xsl_p = (int *)XCALLOC(n_cols, sizeof(int), "xsl_p");
	xel_p = (int *)XCALLOC(n_cols, sizeof(int), "xel_p");
	xsw_p = (int *)XCALLOC(n_cols, sizeof(int), "xsw_p");
	xew_p = (int *)XCALLOC(n_cols, sizeof(int), "xew_p");

	/* XCALLOC() auto-initializes things to 0, so just do the other things */

	for (i = 0; i < n_cols; i++)
	{
		xew[i] = data[i];
		xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ansi_normal;
	}

	bb_p = *bufc;
	l_p = *bufc;

	while (n_done < n_cols)
	{
		for (i = 0; i < n_cols; i++)
		{
			/*
			 * If this is the first column, and it's not our
			 * first line, output a row separator.
			 */
			if ((i == 0) && (*bufc != bb_p))
			{
				print_separator(&row_sep, buff, bufc);
				l_p = *bufc;
			}

			/*
			 * If our column width is 0, we've coalesced and we
			 * can safely continue.
			 */
			if (col_widths[i] == 0)
			{
				continue;
			}

			/*
			 * If this is not the first column of this line,
			 * output a column separator.
			 */
			if (*bufc != l_p)
			{
				print_separator(&col_sep, buff, bufc);
			}

			/*
			 * If we have a pending right-coalesce, we take care
			 * of it now, though we save our previous width at
			 * this stage, since we're going to output at that
			 * width during this pass. We know we're not at width
			 * 0 ourselves at this point, so we don't have to
			 * worry about a cascading coalesce; it'll get taken
			 * care of by the loop, automatically. If we have a
			 * pending coalesce-right and we're at the first
			 * column, we know that we overflowed and should just
			 * clear it.
			 */
			width = col_widths[i];

			if (pending_coaright)
			{
				if (i > 0)
					col_widths[i] += pending_coaright + col_sep.len;

				pending_coaright = 0;
			}

			/*
			 * If we're done and our column width is not zero,
			 * and we are not repeating, we must fill in spaces
			 * before we continue.
			 */

			if (col_done[i] && !(col_justs[i] & JUST_REPEAT))
			{
				if (width > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					width = (width > max) ? max : width;
					XMEMSET(*bufc, fillc, width);
					*bufc += width;
					**bufc = '\0';
				}

				continue;
			}

			/*
			 * Restore our state variables
			 */
			sl = xsl[i];
			el = xel[i];
			sw = xsw[i];
			ew = xew[i];
			sl_ansi_state = xsl_a[i];
			el_ansi_state = xel_a[i];
			sw_ansi_state = xsw_a[i];
			ew_ansi_state = xew_a[i];
			sl_pos = xsl_p[i];
			el_pos = xel_p[i];
			sw_pos = xsw_p[i];
			ew_pos = xew_p[i];
			just = col_justs[i];

			while (1)
			{
				/*
				 * Locate the next start-of-word (SW)
				 */
				for (sw = ew, sw_ansi_state = ew_ansi_state, sw_pos = ew_pos; *sw; ++sw)
				{
					switch (*sw)
					{
					case C_ANSI_ESC:
					{
						consume_ansi_sequence_state(&sw, &sw_ansi_state);
						--sw;
						continue;
					}

					case '\t':
					case '\r':
						*sw = ' ';
						++sw_pos;
						continue;
					case ' ':
						++sw_pos;
						continue;

					case BEEP_CHAR:
						continue;

					default:
						break;
					}

					break;
				}

				/*
				 * Three ways out of that locator loop:
				 * end-of-string (ES), end-of-line (EL), and
				 * start-of-word (SW)
				 */

				if (!*sw && sl == NULL)
				{
					/*
					 * ES, and nothing left
					 * * to output
					 */
					/*
					 * If we're coalescing left, we set
					 * this column to 0 width, and
					 * increase the width of the left
					 * column. If we're coalescing right,
					 * we can't widen that column yet,
					 * because otherwise it'll throw off
					 * its width for this pass, so we've
					 * got to do that later. If we're
					 * repeating, we reset our pointer
					 * state, but we keep track of our
					 * done-ness. Don't increment done
					 * more than once, since we might
					 * repeat several times.
					 */
					if (!col_done[i])
					{
						n_done++;
						col_done[i] = 1;
					}

					if (i && (just & JUST_COALEFT))
					{
						/*
						 * Find the next-left column
						 * with a nonzero width,
						 * since we can have
						 * casdading coalescing.
						 */
						for (n = i - 1; (n > 0) && (col_widths[n] == 0); n--)
							;

						/*
						 * We have to add not only
						 * the width of the column,
						 * but the column separator
						 * length.
						 */
						col_widths[n] += col_widths[i] + col_sep.len;
						col_widths[i] = 0;
					}
					else if ((just & JUST_COARIGHT) && (i + 1 < n_cols))
					{
						pending_coaright = col_widths[i];
						col_widths[i] = 0;
					}
					else if (just & JUST_REPEAT)
					{
						xsl[i] = xel[i] = xsw[i] = NULL;
						xew[i] = data[i];
						xsl_a[i] = xel_a[i] = ansi_normal;
						xsw_a[i] = xew_a[i] = ansi_normal;
						xsl_p[i] = xel_p[i] = xsw_p[i] = xew_p[i] = 0;
					}

					break; /* get out of our infinite
							* while */
				}

				/*
				 * Decide where start-of-line (SL) was
				 */
				if (sl == NULL)
				{
					if (ew == data[i] || ew[-1] == '\n')
					{
						/*
						 * Preserve indentation at SS
						 * or after explicit EL
						 */
						sl = ew;
						sl_ansi_state = ew_ansi_state;
						sl_pos = ew_pos;
					}
					else
					{
						/*
						 * Discard whitespace if
						 * previous line wrapped
						 */
						sl = sw;
						sl_ansi_state = sw_ansi_state;
						sl_pos = sw_pos;
					}
				}

				if (*sw == '\n')
				{
					/*
					 * EL, so we have to
					 * * output
					 */
					ew = sw;
					ew_ansi_state = sw_ansi_state;
					ew_pos = sw_pos;
					break;
				}
				else
				{
					/*
					 * Locate the next end-of-word (EW)
					 */
					for (ew = sw, ew_ansi_state = sw_ansi_state, ew_pos = sw_pos; *ew; ++ew)
					{
						switch (*ew)
						{
						case C_ANSI_ESC:
						{
							consume_ansi_sequence_state(&ew, &ew_ansi_state);
							--ew;
							continue;
						}

						case '\r':
						case '\t':
							*ew = ' ';
							break;
						case ' ':
						case '\n':
							break;

						case BEEP_CHAR:
							continue;

						default:

							/*
							 * Break up long words
							 */
							if (ew_pos - sw_pos == width)
							{
								break;
							}

							++ew_pos;
							continue;
						}

						break;
					}

					/*
					 * Three ways out of that previous
					 * for loop: ES, EL, EW
					 */

					/*
					 * If it fits on the line, add it
					 */
					if (ew_pos - sl_pos <= width)
					{
						el = ew;
						el_ansi_state = ew_ansi_state;
						el_pos = ew_pos;
					}

					/*
					 * If it's just EW, not ES or EL, and
					 * the line isn't too long, keep
					 * adding words to the line
					 */
					if (*ew && *ew != '\n' && (ew_pos - sl_pos <= width))
					{
						continue;
					}

					/*
					 * So now we definitely need to
					 * output a line
					 */
					break;
				}
			}

			/*
			 * Could be a blank line, no words fit
			 */
			if (el == NULL)
			{
				el = sw;
				el_ansi_state = sw_ansi_state;
				el_pos = sw_pos;
			}

			/*
			 * Left space padding if needed
			 */
			if (just & JUST_RIGHT)
			{
				nleft = width - el_pos + sl_pos;

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, fillc, nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}
			else if (just & JUST_CENTER)
			{
				lead_chrs = (int)((width / 2) - ((el_pos - sl_pos) / 2) + .5);

				if (lead_chrs > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					lead_chrs = (lead_chrs > max) ? max : lead_chrs;
					XMEMSET(*bufc, fillc, lead_chrs);
					*bufc += lead_chrs;
					**bufc = '\0';
				}
			}

			/*
			 * Restore previous ansi state
			 */
			buf = ansi_transition_colorstate(ansi_normal, sl_ansi_state, color_type, false);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);
			/*
			 * Print the words
			 */
			XSAFESTRNCAT(buff, bufc, sl, el - sl, LBUF_SIZE);
			/*
			 * Back to ansi normal
			 */
			buf = ansi_transition_colorstate(el_ansi_state, ansi_normal, color_type, false);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);

			/*
			 * Right space padding if needed
			 */
			if (just & JUST_LEFT)
			{
				nleft = width - el_pos + sl_pos;

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, fillc, nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}
			else if (just & JUST_CENTER)
			{
				nleft = width - lead_chrs - el_pos + sl_pos;

				if (nleft > 0)
				{
					max = LBUF_SIZE - 1 - (*bufc - buff);
					nleft = (nleft > max) ? max : nleft;
					XMEMSET(*bufc, fillc, nleft);
					*bufc += nleft;
					**bufc = '\0';
				}
			}

			/*
			 * Update pointers for the next line
			 */

			if (!*el)
			{
				/*
				 * ES, and nothing left to output
				 */
				if (!col_done[i])
				{
					n_done++;
					col_done[i] = 1;
				}

				if ((just & JUST_COALEFT) && (i - 1 >= 0))
				{
					for (n = i - 1; (n > 0) && (col_widths[n] == 0); n--)
						;

					col_widths[n] += col_widths[i] + col_sep.len;
					col_widths[i] = 0;
				}
				else if ((just & JUST_COARIGHT) && (i + 1 < n_cols))
				{
					pending_coaright = col_widths[i];
					col_widths[i] = 0;
				}
				else if (just & JUST_REPEAT)
				{
					xsl[i] = xel[i] = xsw[i] = NULL;
					xew[i] = data[i];
					xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ansi_normal;
					xsl_p[i] = xel_p[i] = xsw_p[i] = xew_p[i] = 0;
				}
			}
			else
			{
				if (*ew == '\n' && sw == ew)
				{
					/*
					 * EL already handled on this line,
					 * and no new word yet
					 */
					++ew;
					sl = el = NULL;
				}
				else if (sl == sw)
				{
					/*
					 * No new word yet
					 */
					sl = el = NULL;
				}
				else
				{
					/*
					 * ES with more to output, EL for
					 * next line, or just a full line
					 */
					sl = sw;
					sl_ansi_state = sw_ansi_state;
					sl_pos = sw_pos;
					el = ew;
					el_ansi_state = ew_ansi_state;
					el_pos = ew_pos;
				}

				/*
				 * Save state
				 */
				xsl[i] = sl;
				xel[i] = el;
				xsw[i] = sw;
				xew[i] = ew;
				xsl_a[i] = sl_ansi_state;
				xel_a[i] = el_ansi_state;
				xsw_a[i] = sw_ansi_state;
				xew_a[i] = ew_ansi_state;
				xsl_p[i] = sl_pos;
				xel_p[i] = el_pos;
				xsw_p[i] = sw_pos;
				xew_p[i] = ew_pos;
			}
		}
	}

	XFREE(col_widths);
	XFREE(col_justs);
	XFREE(col_done);
	XFREE(xsl);
	XFREE(xel);
	XFREE(xsw);
	XFREE(xew);
	XFREE(xsl_a);
	XFREE(xel_a);
	XFREE(xsw_a);
	XFREE(xew_a);
	XFREE(xsl_p);
	XFREE(xel_p);
	XFREE(xsw_p);
	XFREE(xew_p);
}

void fun_align(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **raw_colstrs;
	int n_cols;
	Delim filler, col_sep, row_sep;

	if (nfargs < 2)
	{
		XSAFELBSTR("#-1 FUNCTION (ALIGN) EXPECTS AT LEAST 2 ARGUMENTS", buff, bufc);
		return;
	}

	/*
	 * We need to know how many columns we have, so we know where the
	 * column arguments stop and where the optional arguments start.
	 */
	n_cols = list2arr(&raw_colstrs, LBUF_SIZE / 2, fargs[0], &SPACE_DELIM);

	if (nfargs < n_cols + 1)
	{
		XSAFELBSTR("#-1 NOT ENOUGH COLUMNS FOR ALIGN", buff, bufc);
		XFREE(raw_colstrs);
		return;
	}

	if (nfargs > n_cols + 4)
	{
		XSAFELBSTR("#-1 TOO MANY COLUMNS FOR ALIGN", buff, bufc);
		XFREE(raw_colstrs);
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, n_cols + 2, &filler, 0))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, n_cols + 3, &(col_sep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, n_cols + 4, &(row_sep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (nfargs < n_cols + 4)
	{
		row_sep.str[0] = '\r';
	}

	perform_align(n_cols, raw_colstrs, fargs + 1, filler.str[0], col_sep, row_sep, buff, bufc, player, cause);
	XFREE(raw_colstrs);
}

void fun_lalign(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char **raw_colstrs, **data;
	int n_cols, n_data;
	Delim isep, filler, col_sep, row_sep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 6, buff, bufc))
	{
		return;
	}

	n_cols = list2arr(&raw_colstrs, LBUF_SIZE / 2, fargs[0], &SPACE_DELIM);

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	n_data = list2arr(&data, LBUF_SIZE / 2, fargs[1], &isep);

	if (n_cols > n_data)
	{
		XSAFELBSTR("#-1 NOT ENOUGH COLUMNS FOR LALIGN", buff, bufc);
		XFREE(raw_colstrs);
		XFREE(data);
		return;
	}

	if (n_cols < n_data)
	{
		XSAFELBSTR("#-1 TOO MANY COLUMNS FOR LALIGN", buff, bufc);
		XFREE(raw_colstrs);
		XFREE(data);
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &filler, 0))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 5, &(col_sep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 6, &(row_sep), DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (nfargs < 6)
	{
		row_sep.str[0] = '\r';
	}

	perform_align(n_cols, raw_colstrs, data, filler.str[0], col_sep, row_sep, buff, bufc, player, cause);
	XFREE(raw_colstrs);
	XFREE(data);
}
