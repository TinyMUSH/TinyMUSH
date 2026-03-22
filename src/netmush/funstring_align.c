/**
 * @file funstring_align.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Column-alignment functions (ALIGN, LALIGN)
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
						 * cascading coalescing.
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
