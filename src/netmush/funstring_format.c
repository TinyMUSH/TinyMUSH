/**
 * @file funstring_format.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Text padding and justification functions (SPACE, LJUST, RJUST, CENTER, LEFT, RIGHT, REPEAT)
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

