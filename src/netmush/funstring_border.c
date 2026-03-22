/**
 * @file funstring_border.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Text border-formatting functions (BORDER, CBORDER, RBORDER)
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

