/* stringutil.c - string utilities */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"	/* required by mudconf */
#include "mushconf.h"	/* required by code */
#include "db.h"			/* required by externs */
#include "interface.h"	/* required by code */
#include "externs.h"	/* required by code */
#include "stringutil.h" /* required by code */

/**
 * @brief Convert ansi character code (%x?) to ansi sequence.
 * 
 * @param ch Character to convert
 * @return char* Ansi sequence
 */
char *ansiChar(int ch)
{
	switch (ch)
	{
	case 'B':
		return ANSI_BBLUE;
	case 'C':
		return ANSI_BCYAN;
	case 'G':
		return ANSI_BGREEN;
	case 'M':
		return ANSI_BMAGENTA;
	case 'R':
		return ANSI_BRED;
	case 'W':
		return ANSI_BWHITE;
	case 'X':
		return ANSI_BBLACK;
	case 'Y':
		return ANSI_BYELLOW;
	case 'b':
		return ANSI_BLUE;
	case 'c':
		return ANSI_CYAN;
	case 'f':
		return ANSI_BLINK;
	case 'g':
		return ANSI_GREEN;
	case 'h':
		return ANSI_HILITE;
	case 'i':
		return ANSI_INVERSE;
	case 'm':
		return ANSI_MAGENTA;
	case 'n':
		return ANSI_NORMAL;
	case 'r':
		return ANSI_RED;
	case 'u':
		return ANSI_UNDER;
	case 'w':
		return ANSI_WHITE;
	case 'x':
		return ANSI_BLACK;
	case 'y':
		return ANSI_YELLOW;
	}
	return STRING_EMPTY;
}

/**
 * @brief Convert ansi character code (%x?) to numeric values.
 * 
 * @param ch ANSI character
 * @return int ANSI numeric values
 */
int ansiNum(int ch)
{
	switch (ch)
	{
	case 'B':
		return I_ANSI_BBLUE;
	case 'C':
		return I_ANSI_BCYAN;
	case 'G':
		return I_ANSI_BGREEN;
	case 'M':
		return I_ANSI_BMAGENTA;
	case 'R':
		return I_ANSI_BRED;
	case 'W':
		return I_ANSI_BWHITE;
	case 'X':
		return I_ANSI_BBLACK;
	case 'Y':
		return I_ANSI_BYELLOW;
	case 'b':
		return I_ANSI_BLUE;
	case 'c':
		return I_ANSI_CYAN;
	case 'f':
		return I_ANSI_BLINK;
	case 'g':
		return I_ANSI_GREEN;
	case 'h':
		return I_ANSI_HILITE;
	case 'i':
		return I_ANSI_INVERSE;
	case 'm':
		return I_ANSI_MAGENTA;
	case 'n':
		return I_ANSI_NORMAL;
	case 'r':
		return I_ANSI_RED;
	case 'u':
		return I_ANSI_UNDER;
	case 'w':
		return I_ANSI_WHITE;
	case 'x':
		return I_ANSI_BLACK;
	case 'y':
		return I_ANSI_YELLOW;
	}
	return 0;
}

/**
 * @brief Convert ansi numeric values to character code (%x?).
 * 
 * @param num ANSI number
 * @return char ANSI character
 */
char ansiLetter(int num)
{
	switch (num)
	{
	case 1:
		return 'h';
	case 4:
		return 'u';
	case 5:
		return 'f';
	case 7:
		return 'i';
	case 30:
		return 'x';
	case 31:
		return 'r';
	case 32:
		return 'g';
	case 33:
		return 'y';
	case 34:
		return 'b';
	case 35:
		return 'm';
	case 36:
		return 'c';
	case 37:
		return 'w';
	case 40:
		return 'X';
	case 41:
		return 'R';
	case 42:
		return 'G';
	case 43:
		return 'Y';
	case 44:
		return 'B';
	case 45:
		return 'M';
	case 46:
		return 'C';
	case 47:
		return 'W';
	}
	return 0;
}

char ansiMushCode(int num, bool bg)
{
	switch (num)
	{
	case 0:
		return bg ? 'X' : 'x';
	case 1:
		return bg ? 'R' : 'r';
	case 2:
		return bg ? 'G' : 'g';
	case 3:
		return bg ? 'Y' : 'y';
	case 4:
		return bg ? 'B' : 'b';
	case 5:
		return bg ? 'M' : 'm';
	case 6:
		return bg ? 'C' : 'c';
	case 7:
		return bg ? 'W' : 'w';
	}
	return 0;
}

/**
 * \brief ANSI packed state definitions -- number-to-bitmask translation table.
 *
 * The mask specifies the state bits that are altered by a particular ansi
 * code. Bits are laid out as follows:
 *
 *  0x1000 -- No ansi. Every valid ansi code clears this bit.
 *  0x0800 -- inverse
 *  0x0400 -- flash
 *  0x0200 -- underline
 *  0x0100 -- highlight
 *  0x0080 -- "use default bg", set by ansi normal, cleared by other bg's
 *  0x0070 -- three bits of bg color
 *  0x0008 -- "use default fg", set by ansi normal, cleared by other fg's
 *  0x0007 -- three bits of fg color
 */

/*
int ansi_mask_bits[I_ANSI_LIM] = {

	0x1fff, 0x1100, 0x1100, 0x0000, 0x1200, 0x1400, 0x0000, 0x1800, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1100, 0x1100, 0x0000, 0x1200, 0x1400, 0x0000, 0x1800, 0x0000, 0x0000,
	0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x0000, 0x0000,
	0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x0000, 0x0000};
*/

/**
 * @brief ANSI packed state definitions -- number-to-bitmask translation
 * 
 * The mask specifies the state bits that are altered by a particular ansi
 * code. Bits are laid out as follows:
 *
 *  0x1000 -- No ansi. Every valid ansi code clears this bit.
 *  0x0800 -- inverse
 *  0x0400 -- flash
 *  0x0200 -- underline
 *  0x0100 -- highlight
 *  0x0080 -- "use default bg", set by ansi normal, cleared by other bg's
 *  0x0070 -- three bits of bg color
 *  0x0008 -- "use default fg", set by ansi normal, cleared by other fg's
 *  0x0007 -- three bits of fg color
 * 
 * @param num ANSI number
 * @return int bitmask
 */
int ansiBitsMask(int num)
{
	switch (num)
	{
	case 0:
		return 0x1fff;
	case 1:
	case 2:
	case 21:
	case 22:
		return 0x1100;
	case 4:
	case 24:
		return 0x1200;
	case 5:
	case 25:
		return 0x1400;
	case 7:
	case 27:
		return 0x1800;
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
		return 0x100f;
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
		return 0x10f0;
	}
	return 0;
}

/**
 * \brief ANSI packed state definitions -- number-to-bitvalue translation table.
 */

/**
 * @brief ANSI packed state definitions -- number-to-bitvalue translation
 * 
 * @param num ANSI number
 * @return int ANSI bitvalue.
 */
int ansiBits(int num)
{
	switch (num)
	{
	case 0:
		return 0x0099;
	case 1:
		return 0x0100;
	case 4:
		return 0x0200;
	case 5:
		return 0x0400;
	case 7:
		return 0x0800;
	case 31:
		return 0x0001;
	case 32:
		return 0x0002;
	case 33:
		return 0x0003;
	case 34:
		return 0x0004;
	case 35:
		return 0x0005;
	case 36:
		return 0x0006;
	case 37:
		return 0x0007;
	case 41:
		return 0x0010;
	case 42:
		return 0x0020;
	case 43:
		return 0x0030;
	case 44:
		return 0x0040;
	case 45:
		return 0x0050;
	case 46:
		return 0x0060;
	case 47:
		return 0x0070;
	}
	return 0;
}

/**
 * \fn char *strip_ansi ( const char *s )
 * \brief Return a new string with ansi escape codes removed
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param s Pointer to the original string
 *
 * \return 0 Pointer to the new string with ansi code removed.
 */

char *strip_ansi(const char *s)
{
	char *buf, *p, *s1;
	p = buf = XMALLOC(LBUF_SIZE, "buf");
	*buf = '\0';
	s1 = (char *)s;

	if (s1 && *s1)
	{
		while (*s1 == ESC_CHAR)
		{
			skip_esccode(&s1);
		}

		while (*s1)
		{
			*p++ = *s1++;

			while (*s1 == ESC_CHAR)
			{
				skip_esccode(&s1);
			}
		}

		*p = '\0';
	}

	return (buf);
}

/**
 * \fn char *strip_xterm ( char *s )
 * \brief Return a new string with xterm color code removed
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param s Pointer to the original string
 *
 * \return 0 Pointer to the new string with ansi code removed.
 */

char *strip_xterm(char *s)
{
	char *buf, *p;
	char *s1 = s;
	int skip = 0;
	p = buf = XMALLOC(LBUF_SIZE, "buf");

	while (*s1)
	{
		if (strncmp(s1, ANSI_XTERM_FG, strlen(ANSI_XTERM_FG)) == 0)
		{
			skip = 1;

			while (skip)
			{
				if ((*s1 == '\0') || (*s1 == ANSI_END))
				{
					skip = 0;

					if (*s1 == ANSI_END)
					{
						s1++;
					}
				}
				else
				{
					s1++;
				}
			}
		}

		if (strncmp(s1, ANSI_XTERM_BG, strlen(ANSI_XTERM_BG)) == 0)
		{
			skip = 1;

			while (skip)
			{
				if ((*s1 == '\0') || (*s1 == ANSI_END))
				{
					skip = 0;

					if (*s1 == ANSI_END)
					{
						s1++;
					}
				}
				else
				{
					s1++;
				}
			}
		}

		if (*s1)
		{
			*p++ = *s1++;
		}
		else
		{
			break;
		}
	}

	*p = '\0';
	return (buf);
}

/**
 * \fn int strip_ansi_len ( const char *s )
 * \brief Count non-escape-code characters.
 *
 * \param s Pointer to the string.
 *
 * \return An integer representing the number of character in the string.
 */

int strip_ansi_len(const char *s)
{
	int n = 0;
	char *s1;
	char *s2;
	s1 = XSTRDUP(s, "s1");
	s2 = s1;

	if (s1)
	{
		while (*s1 == ESC_CHAR)
		{
			skip_esccode(&s1);
		}

		while (*s1)
		{
			++s1, ++n;

			while (*s1 == ESC_CHAR)
			{
				skip_esccode(&s1);
			}
		}

		XFREE(s2);
	}

	return n;
}

/**
 * \fn char *normal_to_white ( const char *raw )
 * \brief This function implements the NOBLEED flag.
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param raw Pointer to the string who need to be ansi terminated.
 *
 * \return A pointer to the string terminated.
 */

char *normal_to_white(const char *raw)
{
	char *buf, *q;
	char *p = (char *)raw;
	char *just_after_csi;
	char *just_after_esccode = p;
	unsigned int param_val;
	int has_zero;
	buf = XMALLOC(LBUF_SIZE, "buf");
	q = buf;

	while (p && *p)
	{
		if (*p == ESC_CHAR)
		{
			SAFE_STRNCAT(buf, &q, just_after_esccode, p - just_after_esccode, LBUF_SIZE);

			if (p[1] == ANSI_CSI)
			{
				SAFE_LB_CHR(*p, buf, &q);
				++p;
				SAFE_LB_CHR(*p, buf, &q);
				++p;
				just_after_csi = p;
				has_zero = 0;

				while ((*p & 0xf0) == 0x30)
				{
					if (*p == '0')
					{
						has_zero = 1;
					}

					++p;
				}

				while ((*p & 0xf0) == 0x20)
				{
					++p;
				}

				if (*p == ANSI_END && has_zero)
				{
					/*
		     * it really was an ansi code,
		     * * go back and fix up the zero
		     */
					p = just_after_csi;
					param_val = 0;

					while ((*p & 0xf0) == 0x30)
					{
						if (*p < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*p & 0x0f);
							SAFE_LB_CHR(*p, buf, &q);
						}
						else
						{
							if (param_val == 0)
							{
								/*
				 * ansi normal
				 */
								SAFE_STRNCAT(buf, &q, "m\033[37m\033[", 8, LBUF_SIZE);
							}
							else
							{
								/*
				 * some other color
				 */
								SAFE_LB_CHR(*p, buf, &q);
							}

							param_val = 0;
						}

						++p;
					}

					while ((*p & 0xf0) == 0x20)
					{
						++p;
					}

					SAFE_LB_CHR(*p, buf, &q);
					++p;

					if (param_val == 0)
					{
						SAFE_STRNCAT(buf, &q, ANSI_WHITE, 5, LBUF_SIZE);
					}
				}
				else
				{
					++p;
					SAFE_STRNCAT(buf, &q, just_after_csi, p - just_after_csi, LBUF_SIZE);
				}
			}
			else
			{
				safe_copy_esccode(&p, buf, &q);
			}

			just_after_esccode = p;
		}
		else
		{
			++p;
		}
	}

	SAFE_STRNCAT(buf, &q, just_after_esccode, p - just_after_esccode, LBUF_SIZE);
	return (buf);
}

/**
 * \fn char *ansi_transition_esccode ( int ansi_before, int ansi_after )
 * \brief Handle the transition between two ansi sequence
 *
 * It is the responsibility of the caller to free the returning buffer
 * with XFREE();
 *
 * \param ansi_before Ansi state before transition.
 * \param ansi_after ansi state after transition.
 *
 * \return A pointer to an ansi sequence that will do the transition.
 */

char *ansi_transition_esccode(int ansi_before, int ansi_after)
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	char *buffer;
	buffer = XMALLOC(SBUF_SIZE, "buffer");
	*buffer = '\0';

	if (ansi_before != ansi_after)
	{
		buffer[0] = ESC_CHAR;
		buffer[1] = ANSI_CSI;
		p = buffer + 2;
		/*
	 * If they turn off any highlight bits, or they change from some color
	 * * to default color, we need to use ansi normal first.
	 */
		ansi_bits_set = (~ansi_before) & ansi_after;
		ansi_bits_clr = ansi_before & (~ansi_after);

		if ((ansi_bits_clr & 0xf00) || /* highlights off */
			(ansi_bits_set & 0x088) || /* normal to color */
			(ansi_bits_clr == 0x1000))
		{ /* explicit normal */
			XSTRCPY(p, "0;");
			p += 2;
			ansi_bits_set = (~ansiBits(0)) & ansi_after;
			ansi_bits_clr = ansiBits(0) & (~ansi_after);
		}

		/*
	 * Next reproduce the highlight state
	 */

		if (ansi_bits_set & 0x100)
		{
			XSTRCPY(p, "1;");
			p += 2;
		}

		if (ansi_bits_set & 0x200)
		{
			XSTRCPY(p, "4;");
			p += 2;
		}

		if (ansi_bits_set & 0x400)
		{
			XSTRCPY(p, "5;");
			p += 2;
		}

		if (ansi_bits_set & 0x800)
		{
			XSTRCPY(p, "7;");
			p += 2;
		}

		/*
	 * Foreground color
	 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x00f)
		{
			XSTRCPY(p, "30;");
			p += 3;
			p[-2] |= (ansi_after & 0x00f);
		}

		/*
	 * Background color
	 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x0f0)
		{
			XSTRCPY(p, "40;");
			p += 3;
			p[-2] |= ((ansi_after & 0x0f0) >> 4);
		}

		/*
	 * Terminate
	 */
		if (p > buffer + 2)
		{
			p[-1] = ANSI_END;
			/*
	     * Buffer is already null-terminated by XSTRCPY
	     */
		}
		else
		{
			buffer[0] = '\0';
		}
	}

	return buffer;
}

/**
 * \fn char *ansi_transition_mushcode ( int ansi_before, int ansi_after )
 * \brief Handle the transition between two ansi sequence of mushcode.
 *
 * It is the responsibility of the caller to free the returning buffer
 * with XFREE();
 *
 * \param ansi_before Ansi state before transition.
 * \param ansi_after ansi state after transition.
 *
 * \return A pointer to a mushcode sequence that will do the transition.
 */

char *ansi_transition_mushcode(int ansi_before, int ansi_after)
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	char *buffer;
	buffer = XMALLOC(SBUF_SIZE, "buffer");
	*buffer = '\0';

	if (ansi_before != ansi_after)
	{
		p = buffer;
		/*
	 * If they turn off any highlight bits, or they change from some color
	 * * to default color, we need to use ansi normal first.
	 */
		ansi_bits_set = (~ansi_before) & ansi_after;
		ansi_bits_clr = ansi_before & (~ansi_after);

		if ((ansi_bits_clr & 0xf00) || /* highlights off */
			(ansi_bits_set & 0x088) || /* normal to color */
			(ansi_bits_clr == 0x1000))
		{ /* explicit normal */
			XSTRCPY(p, "%xn");
			p += 3;
			ansi_bits_set = (~ansiBits(0)) & ansi_after;
			ansi_bits_clr = ansiBits(0) & (~ansi_after);
		}

		/*
	 * Next reproduce the highlight state
	 */

		if (ansi_bits_set & 0x100)
		{
			XSTRCPY(p, "%xh");
			p += 3;
		}

		if (ansi_bits_set & 0x200)
		{
			XSTRCPY(p, "%xu");
			p += 3;
		}

		if (ansi_bits_set & 0x400)
		{
			XSTRCPY(p, "%xf");
			p += 3;
		}

		if (ansi_bits_set & 0x800)
		{
			XSTRCPY(p, "%xi");
			p += 3;
		}

		/*
	 * Foreground color
	 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x00f)
		{
			XSTRCPY(p, "%xx");
			p += 3;
			p[-1] = ansiMushCode(ansi_after & 0x00f, false);
		}

		/*
	 * Background color
	 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x0f0)
		{
			XSTRCPY(p, "%xX");
			p += 3;
			p[-1] = ansiMushCode((ansi_after & 0x0f0) >> 4, true);
		}

		/*
	 * Terminate
	 */
		*p = '\0';
	}

	return buffer;
}

/**
 * \fn char *ansi_transition_letters ( int ansi_before, int ansi_after )
 * \brief Handle the transition between two ansi sequence of mushcode.
 *
 * It is the responsibility of the caller to free the returning buffer
 * with XFREE();
 *
 * \param ansi_before Ansi state before transition.
 * \param ansi_after ansi state after transition.
 *
 * \return A pointer to the letters of mushcode that will do the transition.
 */

char *ansi_transition_letters(int ansi_before, int ansi_after)
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	char *buffer;
	buffer = XMALLOC(SBUF_SIZE, "buffer");
	*buffer = '\0';

	if (ansi_before != ansi_after)
	{
		p = buffer;
		/*
	 * If they turn off any highlight bits, or they change from some color
	 * * to default color, we need to use ansi normal first.
	 */
		ansi_bits_set = (~ansi_before) & ansi_after;
		ansi_bits_clr = ansi_before & (~ansi_after);

		if ((ansi_bits_clr & 0xf00) || /* highlights off */
			(ansi_bits_set & 0x088) || /* normal to color */
			(ansi_bits_clr == 0x1000))
		{ /* explicit normal */
			*p++ = 'n';
			ansi_bits_set = (~ansiBits(0)) & ansi_after;
			ansi_bits_clr = ansiBits(0) & (~ansi_after);
		}

		/*
	 * Next reproduce the highlight state
	 */

		if (ansi_bits_set & 0x100)
		{
			*p++ = 'h';
		}

		if (ansi_bits_set & 0x200)
		{
			*p++ = 'u';
		}

		if (ansi_bits_set & 0x400)
		{
			*p++ = 'f';
		}

		if (ansi_bits_set & 0x800)
		{
			*p++ = 'i';
		}

		/*
	 * Foreground color
	 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x00f)
		{
			*p++ = ansiMushCode(ansi_after & 0x00f, false);
		}

		/*
	 * Background color
	 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x0f0)
		{
			*p++ = ansiMushCode((ansi_after & 0x0f0) >> 4, true);
		}

		/*
	 * Terminate
	 */
		*p = '\0';
	}

	return buffer;
}

/**
 * \fn int ansi_map_states ( const char *s, int **m, char **p )
 * \brief Identify ansi state of every character in a string
 *
 * It is the responsibility of the caller to free m with
 * XFREE(m, "ansi_map_states_ansi_map") and p with
 * XFREE(p, "ansi_map_states_stripped");
 *
 * \param s Pointer to the string to be mapped.
 * \param m Pointer to the ansi map to be build.
 * \param p Pointer to the mapped string.
 *
 * \return The number of items mapped.
 */

int ansi_map_states(const char *s, int **m, char **p)
{
	int *ansi_map;
	char *stripped;
	char *s1, *s2;
	int n = 0, ansi_state = ANST_NORMAL;
	ansi_map = (int *)XCALLOC(HBUF_SIZE, sizeof(int), "ansi_map");
	stripped = XMALLOC(LBUF_SIZE, "stripped");
	s2 = s1 = XSTRDUP(s, "s1");

	while (*s1)
	{
		if (*s1 == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s1);
				if (*(s1) == ANSI_CSI)
				{
					while ((*(++(s1)) & 0xf0) == 0x30)
					{
						if (*(s1) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s1)&0x0f);
						}
						else
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							param_val = 0;
						}
					}
				}
				while ((*(s1)&0xf0) == 0x20)
				{
					++(s1);
				}
				if (*(s1) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s1);
				}
				else if (*(s1))
				{
					++(s1);
				}
			} while (0);
		}
		else
		{
			ansi_map[n] = ansi_state;
			stripped[n++] = *s1++;
		}
	}

	ansi_map[n] = ANST_NORMAL;
	stripped[n] = '\0';
	*m = ansi_map;
	*p = stripped;
	XFREE(s2);
	return n;
}

/**
 * \fn char *remap_colors ( const char *s, int *cmap )
 * \brief Allow a change of the color sequences
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param s Pointer to the string to be remap.
 * \param m Pointer to the ansi color map to use.
 *
 * \return Pointer to the remapped string.
 */

char *remap_colors(const char *s, int *cmap)
{
	char *buf;
	char *bp;
	int n;
	buf = XMALLOC(LBUF_SIZE, "buf");

	if (!s || !*s || !cmap)
	{
		XSTRNCPY(buf, s, LBUF_SIZE);
		buf[LBUF_SIZE] = '\0';
		return (buf);
	}

	bp = buf;

	do
	{
		while (*s && (*s != ESC_CHAR))
		{
			SAFE_LB_CHR(*s, buf, &bp);
			s++;
		}

		if (*s == ESC_CHAR)
		{
			SAFE_LB_CHR(*s, buf, &bp);
			s++;

			if (*s == ANSI_CSI)
			{
				SAFE_LB_CHR(*s, buf, &bp);
				s++;

				do
				{
					n = (int)strtol(s, (char **)NULL, 10);

					if ((n >= I_ANSI_BLACK) && (n < I_ANSI_NUM) && (cmap[n - I_ANSI_BLACK] != 0))
					{
						SAFE_LTOS(buf, &bp, cmap[n - I_ANSI_BLACK], LBUF_SIZE);

						while (isdigit(*s))
						{
							s++;
						}
					}
					else
					{
						while (isdigit(*s))
						{
							SAFE_LB_CHR(*s, buf, &bp);
							s++;
						}
					}

					if (*s == ';')
					{
						SAFE_LB_CHR(*s, buf, &bp);
						s++;
					}
				} while (*s && (*s != ANSI_END));

				if (*s == ANSI_END)
				{
					SAFE_LB_CHR(*s, buf, &bp);
					s++;
				}
			}
			else if (*s)
			{
				SAFE_LB_CHR(*s, buf, &bp);
				s++;
			}
		}
	} while (*s);

	return (buf);
}

/**
 * \fn char *translate_string ( char *str, int type )
 * \brief Convert raw ansi to mushcode or strip it.
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param str Pointer to the string to be translated
 * \param type 1 = Convert to mushcode, 0 strip ansi.
 *
 * \return Pointer to the translatted string.
 */

char *translate_string(char *str, int type)
{
	char *buff, *bp;
	bp = buff = XMALLOC(LBUF_SIZE, "buff");

	if (type)
	{
		int ansi_state = ANST_NORMAL;
		int ansi_state_prev = ANST_NORMAL;

		while (*str)
		{
			switch (*str)
			{
			case ESC_CHAR:
				while (*str == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(str);
						if (*(str) == ANSI_CSI)
						{
							while ((*(++(str)) & 0xf0) == 0x30)
							{
								if (*(str) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(str)&0x0f);
								}
								else
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									param_val = 0;
								}
							}
						}
						while ((*(str)&0xf0) == 0x20)
						{
							++(str);
						}
						if (*(str) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++(str);
						}
						else if (*(str))
						{
							++(str);
						}
					} while (0);
				}

				SAFE_LB_STR(ansi_transition_mushcode(ansi_state_prev, ansi_state), buff, &bp);
				ansi_state_prev = ansi_state;
				continue;

			case ' ':
				if (str[1] == ' ')
				{
					SAFE_STRNCAT(buff, &bp, "%b", 2, LBUF_SIZE);
				}
				else
				{
					SAFE_LB_CHR(' ', buff, &bp);
				}

				break;

			case '\\':
			case '%':
			case '[':
			case ']':
			case '{':
			case '}':
			case '(':
			case ')':
				SAFE_LB_CHR('%', buff, &bp);
				SAFE_LB_CHR(*str, buff, &bp);
				break;

			case '\r':
				break;

			case '\n':
				SAFE_STRNCAT(buff, &bp, "%r", 2, LBUF_SIZE);
				break;

			case '\t':
				SAFE_STRNCAT(buff, &bp, "%t", 2, LBUF_SIZE);
				break;

			default:
				SAFE_LB_CHR(*str, buff, &bp);
			}

			str++;
		}
	}
	else
	{
		while (*str)
		{
			switch (*str)
			{
			case ESC_CHAR:
				skip_esccode(&str);
				continue;

			case '\r':
				break;

			case '\n':
			case '\t':
				SAFE_LB_CHR(' ', buff, &bp);
				break;

			default:
				SAFE_LB_CHR(*str, buff, &bp);
			}

			str++;
		}
	}

	*bp = '\0';
	return buff;
}

/*
 * ---------------------------------------------------------------------------
 * rgb2xterm -- Convert an RGB value to xterm color
 */

/**
 * \fn int rgb2xterm ( long rgb )
 * \brief Convert an RGB value to xterm value
 *
 * \param rgb Color to convert to xterm value
 *
 * \return The xterm value of the color.
 */

int rgb2xterm(long rgb)
{
	int xterm, r, g, b;

	/* First, handle standard colors */
	if (rgb == 0x000000)
	{
		return (0);
	}

	if (rgb == 0x800000)
	{
		return (1);
	}

	if (rgb == 0x008000)
	{
		return (2);
	}

	if (rgb == 0x808000)
	{
		return (3);
	}

	if (rgb == 0x000080)
	{
		return (4);
	}

	if (rgb == 0x800080)
	{
		return (5);
	}

	if (rgb == 0x008080)
	{
		return (6);
	}

	if (rgb == 0xc0c0c0)
	{
		return (7);
	}

	if (rgb == 0x808080)
	{
		return (8);
	}

	if (rgb == 0xff0000)
	{
		return (9);
	}

	if (rgb == 0x00ff00)
	{
		return (10);
	}

	if (rgb == 0xffff00)
	{
		return (11);
	}

	if (rgb == 0x0000ff)
	{
		return (12);
	}

	if (rgb == 0xff00ff)
	{
		return (13);
	}

	if (rgb == 0x00ffff)
	{
		return (14);
	}

	if (rgb == 0xffffff)
	{
		return (15);
	}

	r = (rgb & 0xFF0000) >> 16;
	g = (rgb & 0x00FF00) >> 8;
	b = rgb & 0x0000FF;

	/* Next, handle grayscales */

	if ((r == g) && (r == b))
	{
		if (rgb <= 0x080808)
		{
			return (232);
		}

		if (rgb <= 0x121212)
		{
			return (233);
		}

		if (rgb <= 0x1c1c1c)
		{
			return (234);
		}

		if (rgb <= 0x262626)
		{
			return (235);
		}

		if (rgb <= 0x303030)
		{
			return (236);
		}

		if (rgb <= 0x3a3a3a)
		{
			return (237);
		}

		if (rgb <= 0x444444)
		{
			return (238);
		}

		if (rgb <= 0x4e4e4e)
		{
			return (239);
		}

		if (rgb <= 0x585858)
		{
			return (240);
		}

		if (rgb <= 0x606060)
		{
			return (241);
		}

		if (rgb <= 0x666666)
		{
			return (242);
		}

		if (rgb <= 0x767676)
		{
			return (243);
		}

		if (rgb <= 0x808080)
		{
			return (244);
		}

		if (rgb <= 0x8a8a8a)
		{
			return (245);
		}

		if (rgb <= 0x949494)
		{
			return (246);
		}

		if (rgb <= 0x9e9e9e)
		{
			return (247);
		}

		if (rgb <= 0xa8a8a8)
		{
			return (248);
		}

		if (rgb <= 0xb2b2b2)
		{
			return (249);
		}

		if (rgb <= 0xbcbcbc)
		{
			return (250);
		}

		if (rgb <= 0xc6c6c6)
		{
			return (251);
		}

		if (rgb <= 0xd0d0d0)
		{
			return (252);
		}

		if (rgb <= 0xdadada)
		{
			return (253);
		}

		if (rgb <= 0xe4e4e4)
		{
			return (254);
		}

		if (rgb <= 0xeeeeee)
		{
			return (255);
		}
	}

	/* It's an RGB, convert it */
	xterm = (((r / 51) * 36) + ((g / 51) * 6) + (b / 51)) + 16;

	/* Just in case... */

	if (xterm < 16)
	{
		xterm = 16;
	}

	if (xterm > 231)
	{
		xterm = 231;
	}

	return (xterm);
}

/**
 * \fn int str2xterm ( char *str )
 * \brief Convert a value to xterm color
 *
 * \param str A string representing the color to be convert into xterm value.
 *            The value can be express as hex (#rrggbb), decimal (r g b). a 24
 *            bit integer value, or the actual xterm value.
 *
 * \return The xterm value of the color.
 */

int str2xterm(char *str)
{
	long rgb;
	int r, g, b;
	char *p, *t;
	p = str;

	if (*p == '#')
	{ /* Ok, it's a RGB in hex */
		p++;
		rgb = strtol(p, &t, 16);

		if (p == t)
		{
			return (-1);
		}
		else
		{
			return (rgb2xterm(rgb));
		}
	}
	else
	{ /* Then it must be decimal */
		r = strtol(p, &t, 10);

		if (p == t)
		{
			return (-1);
		}
		else if (*t == 0)
		{
			if (r < 256)
			{
				return (r); /* It's the color index */
			}

			return (rgb2xterm(r)); /* It's a RGB, fetch the index */
		}
		else
		{
			p = t;

			while (!isdigit(*p) && (*p != 0))
			{
				p++;
			}

			g = strtol(p, &t, 10);

			if ((p == t) || (*p == 0))
			{
				return (-1);
			}

			p = t;

			while (!isdigit(*p) && (*p != 0))
			{
				p++;
			}

			b = strtol(p, &t, 10);

			if ((p == t) || (*p == 0))
			{
				return (-1);
			}

			rgb = (r << 16) + (g << 8) + b;
			return (rgb2xterm(rgb));
		}
	}

	return (-1); /* Something is terribly wrong... */
}

/**
 * \fn char *upcasestr ( char *s )
 * \brief Capitalizes an entire string
 *
 * \param s The string that need to be capitalized.
 *
 * \return The string capitalized.
 */

char *upcasestr(char *s)
{
	char *p;

	for (p = s; p && *p; p++)
	{
		*p = toupper(*p);
	}

	return s;
}

/**
 * \fn char *munge_space ( char *string )
 * \brief Compress multiple spaces into one and remove leading/trailing spaces.
 *
 * It is the responsibility of the caller to free the returned
 * buffer with XFREE().
 *
 * \param s The string that need to be munge
 *
 * \return The string munged
 */

char *munge_space(char *string)
{
	char *buffer, *p, *q;
	buffer = XMALLOC(LBUF_SIZE, "buffer");
	p = string;
	q = buffer;

	while (p && *p && isspace(*p))
	{
		p++;
	}

	/* remove initial spaces */

	while (p && *p)
	{
		while (*p && !isspace(*p))
		{
			*q++ = *p++;
		}

		while (*p && isspace(*++p))
			;

		if (*p)
		{
			*q++ = ' ';
		}
	}

	*q = '\0';
	/* remove terminal spaces and terminate string */
	return (buffer);
}

/**
 * \fn char *trim_spaces ( char *string )
 * \brief Remove leading and trailing spaces.
 *
 * \param s The string that need to be trimmed.
 *
 * \return The trimmed string.
 */

char *trim_spaces(char *string)
{
	char *buffer, *p, *q;
	buffer = XMALLOC(LBUF_SIZE, "buffer");
	p = string;
	q = buffer;

	while (p && *p && isspace(*p))
	{
		p++; /* remove inital spaces */
	}

	while (p && *p)
	{
		while (*p && !isspace(*p))
		{
			*q++ = *p++; /* copy nonspace chars */
		}

		while (*p && isspace(*p))
		{
			p++; /* compress spaces */
		}

		if (*p)
		{
			*q++ = ' '; /* leave one space */
		}
	}

	*q = '\0'; /* terminate string */
	return (buffer);
}

/**
 * \fn char *grabto ( char **str, char targ )
 * \brief Return portion of a string up to the indicated character.
 *
 * Note that str will be move to the position following the searched
 * character. If you need the original string, save it before calling
 * this function.
 *
 * \param str The string you want to search
 * \param targ The caracter you want to search for,
 *
 * \return The string up to the character you've search for.
 */

char *grabto(char **str, char targ)
{
	char *savec, *cp;

	if (!str || !*str || !**str)
	{
		return NULL;
	}

	savec = cp = *str;

	while (*cp && *cp != targ)
	{
		cp++;
	}

	if (*cp)
	{
		*cp++ = '\0';
	}

	*str = cp;
	return savec;
}

/**
 * \fn int string_compare ( const char *s1, const char *s2 )
 * \brief Compare two string. Treat multiple spaces as one.
 *
 * \param s1 The first string to compare
 * \param s2 The second string to compare
 *
 * \return 0 if the string are different.
 */

int string_compare(const char *s1, const char *s2)
{
	if (mudstate.standalone || mudconf.space_compress)
	{
		while (isspace(*s1))
		{
			s1++;
		}

		while (isspace(*s2))
		{
			s2++;
		}

		while (*s1 && *s2 && ((tolower(*s1) == tolower(*s2)) || (isspace(*s1) && isspace(*s2))))
		{
			if (isspace(*s1) && isspace(*s2))
			{
				/* skip all other spaces */
				while (isspace(*s1))
				{
					s1++;
				}

				while (isspace(*s2))
				{
					s2++;
				}
			}
			else
			{
				s1++;
				s2++;
			}
		}

		if ((*s1) && (*s2))
		{
			return (1);
		}

		if (isspace(*s1))
		{
			while (isspace(*s1))
			{
				s1++;
			}

			return (*s1);
		}

		if (isspace(*s2))
		{
			while (isspace(*s2))
			{
				s2++;
			}

			return (*s2);
		}

		if ((*s1) || (*s2))
		{
			return (1);
		}

		return (0);
	}
	else
	{
		while (*s1 && *s2 && tolower(*s1) == tolower(*s2))
		{
			s1++, s2++;
		}

		return (tolower(*s1) - tolower(*s2));
	}
}

/**
 * \fn int string_prefix ( const char *string, const char *prefix )
 * \brief Compare a string with a prefix
 *
 * \param string The string you want to search
 * \param prefix The prefix you are searching for.
 *
 * \return 0 if the prefix isn't found.
 */

int string_prefix(const char *string, const char *prefix)
{
	int count = 0;

	while (*string && *prefix && tolower(*string) == tolower(*prefix))
	{
		string++, prefix++, count++;
	}

	if (*prefix == '\0')
	{ /* Matched all of prefix */
		return (count);
	}
	else
	{
		return (0);
	}
}

/**
 * \fn const char *string_match ( const char *src, const char *sub )
 * \brief Compare a string with a substring, accepts only nonempty matches starting at the beginning of a word
 *
 * \param src The string you want to search
 * \param sub The search term.
 *
 * \return The position of the search term. 0 if not found.
 */

const char *string_match(const char *src, const char *sub)
{
	if ((*sub != '\0') && (src))
	{
		while (*src)
		{
			if (string_prefix(src, sub))
			{
				return src;
			}

			/*  else scan to beginning of next word */

			while (*src && isalnum(*src))
			{
				src++;
			}

			while (*src && !isalnum(*src))
			{
				src++;
			}
		}
	}

	return 0;
}

/**
 * \fn char *replace_string ( const char *old, const char *new, const char *string )
 * \brief Replace all occurences of a substring with a new substring.
 *
 * \param old The string you want to replace.
 * \param new The string you want to replace with.
 * \param string The string that is modified.
 *
 * \return A pointer to the modified string.
 */

char *replace_string(const char *old, const char *new, const char *string)
{
	char *result, *r, *s;
	int olen;
	r = result = XMALLOC(LBUF_SIZE, "result");

	if (string != NULL)
	{
		s = (char *)string;
		olen = strlen(old);

		while (*s)
		{
			/* Copy up to the next occurrence of the first char of OLD */
			while (*s && *s != *old)
			{
				SAFE_LB_CHR(*s, result, &r);
				s++;
			}

			/*
	     * If we are really at an OLD, append NEW to the result and
	     * bump the input string past the occurrence of
	     * OLD. Otherwise, copy the char and try again.
	     */

			if (*s)
			{
				if (!strncmp(old, s, olen))
				{
					SAFE_LB_STR((char *)new, result, &r);
					s += olen;
				}
				else
				{
					SAFE_LB_CHR(*s, result, &r);
					s++;
				}
			}
		}
	}

	*r = '\0';
	return result;
}

/**
 * \fn void edit_string ( char *src, char **dst, char *from, char *to )
 * \brief Replace all occurences of a substring with a new substring.
 *        Sensitive about ANSI codes, and handles special ^ and $ cases.
 *
 * \param src Pointer to the original string.
 * \param dst Pointer to the new string.
 * \param from Pointer to the string to be replaced
 * \param to Pointer to the string to be replaced with.
 *
 * \return None
 */

void edit_string(char *src, char **dst, char *from, char *to)
{
	char *cp, *p;
	int ansi_state, to_ansi_set, to_ansi_clr, tlen, flen;
	/*
     * We may have gotten an ANSI_NORMAL termination to OLD and NEW,
     * that the user probably didn't intend to be there. (If the
     * user really did want it there, he simply has to put a double
     * ANSI_NORMAL in; this is non-intuitive but without it we can't
     * let users swap one ANSI code for another using this.)  Thus,
     * we chop off the terminating ANSI_NORMAL on both, if there is
     * one.
     */
	p = from + strlen(from) - 4;

	if (p >= from && !strcmp(p, ANSI_NORMAL))
	{
		*p = '\0';
	}

	p = to + strlen(to) - 4;

	if (p >= to && !strcmp(p, ANSI_NORMAL))
	{
		*p = '\0';
	}

	/*
     * Scan the contents of the TO string. Figure out whether we
     * have any embedded ANSI codes.
     */
	ansi_state = ANST_NONE;

	do
	{
		p = to;
		while (*p)
		{
			if (*p == ESC_CHAR)
			{
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++p;
					if (*p == ANSI_CSI)
					{
						while ((*(++p) & 0xf0) == 0x30)
						{
							if (*p < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*p & 0x0f);
							}
							else
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								param_val = 0;
							}
						}
					}
					while ((*p & 0xf0) == 0x20)
					{
						++p;
					}
					if (*p == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
						++p;
					}
					else if (*p)
					{
						++p;
					}
				} while (0);
			}
			else
			{
				++p;
			}
		}
	} while (0);

	to_ansi_set = (~ANST_NONE) & ansi_state;
	to_ansi_clr = ANST_NONE & (~ansi_state);
	tlen = p - to;
	/* Do the substitution.  Idea for prefix/suffix from R'nice@TinyTIM */
	cp = *dst = XMALLOC(LBUF_SIZE, "edit_string_cp");

	if (!strcmp(from, "^"))
	{
		/* Prepend 'to' to string */
		SAFE_STRNCAT(*dst, &cp, to, tlen, LBUF_SIZE);

		do
		{
			p = src;
			while (*p)
			{
				if (*p == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++p;
						if (*p == ANSI_CSI)
						{
							while ((*(++p) & 0xf0) == 0x30)
							{
								if (*p < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*p & 0x0f);
								}
								else
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									param_val = 0;
								}
							}
						}
						while ((*p & 0xf0) == 0x20)
						{
							++p;
						}
						if (*p == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++p;
						}
						else if (*p)
						{
							++p;
						}
					} while (0);
				}
				else
				{
					++p;
				}
			}
		} while (0);

		SAFE_STRNCAT(*dst, &cp, src, p - src, LBUF_SIZE);
	}
	else if (!strcmp(from, "$"))
	{
		/* Append 'to' to string */
		ansi_state = ANST_NONE;

		do
		{
			p = src;
			while (*p)
			{
				if (*p == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++p;
						if (*p == ANSI_CSI)
						{
							while ((*(++p) & 0xf0) == 0x30)
							{
								if (*p < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*p & 0x0f);
								}
								else
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									param_val = 0;
								}
							}
						}
						while ((*p & 0xf0) == 0x20)
						{
							++p;
						}
						if (*p == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++p;
						}
						else if (*p)
						{
							++p;
						}
					} while (0);
				}
				else
				{
					++p;
				}
			}
		} while (0);

		SAFE_STRNCAT(*dst, &cp, src, p - src, LBUF_SIZE);
		ansi_state |= to_ansi_set;
		ansi_state &= ~to_ansi_clr;
		SAFE_STRNCAT(*dst, &cp, to, tlen, LBUF_SIZE);
	}
	else
	{
		/*
	 * Replace all occurances of 'from' with 'to'.  Handle the
	 * special cases of from = \$ and \^.
	 */
		if (((from[0] == '\\') || (from[0] == '%')) && ((from[1] == '$') || (from[1] == '^')) && (from[2] == '\0'))
		{
			from++;
		}

		flen = strlen(from);
		ansi_state = ANST_NONE;

		while (*src)
		{
			/* Copy up to the next occurrence of the first char of FROM. */
			p = src;

			while (*src && (*src != *from))
			{
				if (*src == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(src);
						if (*(src) == ANSI_CSI)
						{
							while ((*(++(src)) & 0xf0) == 0x30)
							{
								if (*(src) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(src)&0x0f);
								}
								else
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									param_val = 0;
								}
							}
						}
						while ((*(src)&0xf0) == 0x20)
						{
							++(src);
						}
						if (*(src) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++(src);
						}
						else if (*(src))
						{
							++(src);
						}
					} while (0);
				}
				else
				{
					++src;
				}
			}

			SAFE_STRNCAT(*dst, &cp, p, src - p, LBUF_SIZE);

			/*
	     * If we are really at a FROM, append TO to the result
	     * and bump the input string past the occurrence of
	     * FROM. Otherwise, copy the char and try again.
	     */

			if (*src)
			{
				if (!strncmp(from, src, flen))
				{
					/* Apply whatever ANSI transition happens in TO */
					ansi_state |= to_ansi_set;
					ansi_state &= ~to_ansi_clr;
					SAFE_STRNCAT(*dst, &cp, to, tlen, LBUF_SIZE);
					src += flen;
				}
				else
				{
					/*
		     * We have to handle the case where
		     * the first character in FROM is the
		     * ANSI escape character. In that case
		     * we move over and copy the entire
		     * ANSI code. Otherwise we just copy
		     * the character.
		     */
					if (*from == ESC_CHAR)
					{
						p = src;
						do
						{
							int ansi_mask = 0;
							int ansi_diff = 0;
							unsigned int param_val = 0;
							++(src);
							if (*(src) == ANSI_CSI)
							{
								while ((*(++(src)) & 0xf0) == 0x30)
								{
									if (*(src) < 0x3a)
									{
										param_val <<= 1;
										param_val += (param_val << 2) + (*(src)&0x0f);
									}
									else
									{
										if (param_val < I_ANSI_LIM)
										{
											ansi_mask |= ansiBitsMask(param_val);
											ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
										}
										param_val = 0;
									}
								}
							}
							while ((*(src)&0xf0) == 0x20)
							{
								++(src);
							}
							if (*(src) == ANSI_END)
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
								++(src);
							}
							else if (*(src))
							{
								++(src);
							}
						} while (0);
						SAFE_STRNCAT(*dst, &cp, p, src - p, LBUF_SIZE);
					}
					else
					{
						SAFE_LB_CHR(*src, *dst, &cp);
						++src;
					}
				}
			}
		}
	}

	p = ansi_transition_esccode(ansi_state, ANST_NONE);
	SAFE_LB_STR(p, *dst, &cp);
	XFREE(p);
}

/**
 * \fn int minmatch ( char *str, char *target, int min )
 * \brief Find if a substring at the start of a string exist.
 *
 * At least MIN characters must match. This is how flags are
 * match by the command queue.
 *
 * \param str Pointer to the string that will be search.
 * \param target Pointer to the string being search.
 * \param min Minimum length of match.
 *
 * \return 1 if found, 0 if not.
 */

int minmatch(char *str, char *target, int min)
{
	while (*str && *target && (tolower(*str) == tolower(*target)))
	{
		str++;
		target++;
		min--;
	}

	if (*str)
	{
		return 0;
	}

	if (!*target)
	{
		return 1;
	}

	return ((min <= 0) ? 1 : 0);
}

/**
* \fn int matches_exit_from_list ( char *exit_list, char *pattern )
* \brief Find if pattern is found in the exit list.
*
* \param exit_list Pointer to the list of names of an exit
* \param pattern Pointer to the pattern we are searching
*
* \return 1 if the pattern is in the list, 0 if not.
*/

int matches_exit_from_list(char *exit_list, char *pattern)
{
	char *s;

	if (*exit_list == '\0')
	{ /* never match empty */
		return 0;
	}

	while (*pattern)
	{
		/* check out this one */
		for (s = exit_list; (*s && (tolower(*s) == tolower(*pattern)) && *pattern && (*pattern != EXIT_DELIMITER)); s++, pattern++)
			;

		/* Did we match it all? */

		if (*s == '\0')
		{
			/* Make sure nothing afterwards */
			while (*pattern && isspace(*pattern))
			{
				pattern++;
			}

			/* Did we get it? */

			if (!*pattern || (*pattern == EXIT_DELIMITER))
			{
				return 1;
			}
		}

		/* We didn't get it, find next string to test */

		while (*pattern && *pattern++ != EXIT_DELIMITER)
			;

		while (isspace(*pattern))
		{
			pattern++;
		}
	}

	return 0;
}

/**
* \fn char *ltos ( char long num )
* \brief Convert a long signed number into string.
*
* It is the caller's responsibility to free the returned buffer with
* XFREE();
*
* \param num Number to convert.
*
* \return A pointer to the resulting string.
*/

char *ltos(long num)
{
	/* Mark Vasoll's long int to string converter. */
	char *buf, *p, *dest, *destp;
	unsigned long anum;
	p = buf = XMALLOC(SBUF_SIZE, "ltos_buf");
	destp = dest = XMALLOC(SBUF_SIZE, "ltos_dest");
	/* absolute value */
	anum = (num < 0) ? -num : num;

	/* build up the digits backwards by successive division */
	while (anum > 9)
	{
		*p++ = '0' + (anum % 10);
		anum /= 10;
	}

	/* put in the sign if needed */
	if (num < 0)
	{
		*destp++ = '-';
	}

	/* put in the last digit, this makes very fast single digits numbers */
	*destp++ = '0' + (char)anum;

	/* reverse the rest of the digits (if any) into the provided buf */
	while (p-- > buf)
	{
		*destp++ = *p;
	}

	/* terminate the resulting string */
	*destp = '\0';
	XFREE(buf);
	return (dest);
}

/**
* \fn char *repeatchar ( int count, char ch )
* \brief Return a string with 'count' number of 'ch' characters.
*
* It is the responsibility of the caller to free the resulting
* buffer.
*
* \param count Length of the string to build
* \param ch Character used to fill the string.
*
* \return A Pointer to the new string.
*/

char *repeatchar(int count, char ch)
{
	char *str, *ptr;
	ptr = str = XMALLOC(LBUF_SIZE, "repeatchar_ptr");

	if (count > 0)
	{
		for (; str < ptr + count; str++)
		{
			*str = ch;
		}
	}

	*str = '\0';
	return ptr;
}

/**
* \fn void skip_esccode ( char **s )
* \brief Move the pointer after an ansi escape sequence.
*
* \param s Pointer that need to be modified.
*/

void skip_esccode(char **s)
{
	++(*s);

	if (**s == ANSI_CSI)
	{
		do
		{
			++(*s);
		} while ((**s & 0xf0) == 0x30);
	}

	while ((**s & 0xf0) == 0x20)
	{
		++(*s);
	}

	if (**s)
	{
		++(*s);
	}
}

/**
* \fn void copy_esccode ( char **s, char **t )
* \brief Copy the ansi sequence into another pointer
*
* \param s Pointer containing the ansi sequence.
* \param t Pointer who will receive the ansi sequence.
*/

void copy_esccode(char **s, char **t)
{
	**t = **s;
	++(*s);
	++(*t);

	if (**s == ANSI_CSI)
	{
		do
		{
			**t = **s;
			++(*s);
			++(*t);
		} while ((**s & 0xf0) == 0x30);
	}

	while ((**s & 0xf0) == 0x20)
	{
		**t = **s;
		++(*s);
		++(*t);
	}

	if (**s)
	{
		**t = **s;
		++(*s);
		++(*t);
	}
}

/**
* \fn void safe_copy_esccode ( char **s, char *buff, char **bufc )
* \brief Copy the ansi sequence into another pointer, moving bufc to the end of the receiving buffer and watching for overflow.
*
* \param s Pointer containing the ansi sequence.
* \param buff Pointer to the receiving buffer.
* \param bufc Pointer to where the data will be copied into the receiving buffer.
*/

void safe_copy_esccode(char **s, char *buff, char **bufc)
{
	SAFE_LB_CHR(**s, buff, bufc);
	++(*s);

	if (**s == ANSI_CSI)
	{
		do
		{
			SAFE_LB_CHR(**s, buff, bufc);
			++(*s);
		} while ((**s & 0xf0) == 0x30);
	}

	while ((**s & 0xf0) == 0x20)
	{
		SAFE_LB_CHR(**s, buff, bufc);
		++(*s);
	}

	if (**s)
	{
		SAFE_LB_CHR(**s, buff, bufc);
		++(*s);
	}
}

/**
* \fn void track_ansi_letters ( char *t, int *ansi_state )
* \brief Convert mushcode to ansi state.
*
* \param t Pointer containing the ansi sequence.
* \param ansi_state The ansi state that need to be updated.
*/

void track_ansi_letters(char *t, int *ansi_state)
{
	char *s;
	s = t;

	while (*s)
	{
		switch (*s)
		{
		case ESC_CHAR:
			skip_esccode(&s);
			break;
		case '<': // Skip xterm, we handle it elsewhere
		case '/':
			while ((*s != '>') && (*s != 0))
			{
				++s;
			}

			break;
		default:
			*ansi_state = ((*ansi_state & ~ansiBitsMask(ansiNum((unsigned char)*s))) | ansiBits(ansiNum((unsigned char)*s)));
			++s;
		}
	}
}
