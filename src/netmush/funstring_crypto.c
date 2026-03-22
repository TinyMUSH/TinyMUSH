/**
 * @file funstring_crypto.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Security and encoding functions
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


/**
 * @brief Returns &lt;string&gt; after replacing the characters [](){};,%\$ with spaces.
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
void fun_secure(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s = fargs[0], *escape_start = NULL;

	while (*s)
	{
		switch (*s)
		{
		case C_ANSI_ESC:
			escape_start = s;
			skip_esccode(&s);
			for (char *seq = escape_start; seq < s; ++seq)
			{
				XSAFELBCHR(*seq, buff, bufc);
			}
		case '%':
		case '$':
		case '\\':
		case '[':
		case ']':
		case '(':
		case ')':
		case '{':
		case '}':
		case ',':
		case ';':
			XSAFELBCHR(' ', buff, bufc);
			break;

		default:
			XSAFELBCHR(*s, buff, bufc);
			break;
		}

		++s;
	}
}

/**
 * @brief Returns &lt;string&gt; after adding an escape character (\) at the start
 *        of the string and also before each of the characters %;[]{}\ that
 *        appear in the string.
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
void fun_escape(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s = fargs[0], *d = NULL, *escape_start = NULL;

	if (!*s)
	{
		return;
	}

	XSAFELBCHR('\\', buff, bufc);
	d = *bufc;

	while (*s)
	{
		switch (*s)
		{
		case C_ANSI_ESC:
			escape_start = s;
			skip_esccode(&s);
			for (char *seq = escape_start; seq < s; ++seq)
			{
				XSAFELBCHR(*seq, buff, bufc);
			}
		case '%':
		case '\\':
		case '[':
		case ']':
		case '{':
		case '}':
		case ';':
			if (*bufc != d)
			{
				XSAFELBCHR('\\', buff, bufc);
			}
			//[[fallthrough]];
			__attribute__((fallthrough));
		default:
			XSAFELBCHR(*s, buff, bufc);
			break;
		}

		++s;
	}
}

/**
 * @brief Less aggressive escape; it does not put a \ at the start of the
 *        string, and it only escapes %[]\ -- making it more suitable for
 *        strings that you simply don't want evaluated.
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
void fun_esc(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s = fargs[0], *escape_start = NULL;

	if (!*s)
	{
		return;
	}

	while (*s)
	{
		switch (*s)
		{
		case C_ANSI_ESC:
			escape_start = s;
			skip_esccode(&s);
			for (char *seq = escape_start; seq < s; ++seq)
			{
				XSAFELBCHR(*seq, buff, bufc);
			}
		case '%':
		case '\\':
		case '[':
		case ']':
			XSAFELBCHR('\\', buff, bufc);
			XSAFELBCHR(*s, buff, bufc);
			break;
		default:
			XSAFELBCHR(*s, buff, bufc);
			break;
		}

		++s;
	}
}

/**
 * @brief Remove all of a set of characters from a string.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_stripchars(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s = NULL;
	unsigned char stab[256];
	Delim osep;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	/**
	 * Output delimiter should default to null, not a space
	 *
	 */
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	if (nfargs < 3)
	{
		osep.str[0] = '\0';
	}

	XMEMSET(stab, 0, sizeof(stab));

	for (s = fargs[1]; *s; s++)
	{
		stab[(unsigned char)*s] = 1;
	}

	for (s = fargs[0]; *s; s++)
	{
		if (stab[(unsigned char)*s] == 0)
		{
			XSAFELBCHR(*s, buff, bufc);
		}
		else if (nfargs > 2)
		{
			print_separator(&osep, buff, bufc);
		}
	}
}
/*---------------------------------------------------------------------------
 * encrypt() and decrypt(): From DarkZone.
 */

void crunch_code(char *code)
{
	char *in, *out;
	in = out = code;

	while (*in)
	{
		if ((*in >= CRYPTCODE_LO) && (*in <= CRYPTCODE_HI))
		{
			*out++ = *in++;
		}
		else if (*in == C_ANSI_ESC)
		{
			skip_esccode(&in);
		}
		else
		{
			++in;
		}
	}

	*out = '\0';
}

void crypt_code(char *buff, char **bufc, char *code, char *text, int type)
{
	char *p, *q;

	if (!*text)
	{
		return;
	}

	crunch_code(code);

	if (!*code)
	{
		XSAFELBSTR(text, buff, bufc);
		return;
	}

	q = code;
	p = *bufc;
	XSAFELBSTR(text, buff, bufc);

	/*
	 * Encryption: Simply go through each character of the text, get its
	 * ascii value, subtract LO, add the ascii value (less LO) of the
	 * code, mod the result, add LO. Continue
	 */
	while (*p)
	{
		if ((*p >= CRYPTCODE_LO) && (*p <= CRYPTCODE_HI))
		{
			if (type)
			{
				*p = (((*p - CRYPTCODE_LO) + (*q - CRYPTCODE_LO)) % CRYPTCODE_MOD) + CRYPTCODE_LO;
			}
			else
			{
				*p = (((*p - *q) + 2 * CRYPTCODE_MOD) % CRYPTCODE_MOD) + CRYPTCODE_LO;
			}

			++p, ++q;

			if (!*q)
			{
				q = code;
			}
		}
		else if (*p == C_ANSI_ESC)
		{
			skip_esccode(&p);
		}
		else
		{
			++p;
		}
	}
}

void fun_encrypt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 1);
}

void fun_decrypt(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 0);
}

/*
 * ---------------------------------------------------------------------------
 * fun_scramble:  randomizes the letters in a string.
 */

/* Borrowed from PennMUSH 1.50 */
void fun_scramble(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int n, i, j, num_transitions;
	ColorState *color_states;
	char *stripped, *seq_buf;
	char escape_buffer[256];
	size_t escape_offset;
	ColorType color_type = ColorTypeNone;
	dbref color_target = (cause != NOTHING) ? cause : player;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	// Always process colors at highest fidelity: they get converted to player-appropriate level at send time.
	color_type = ColorTypeTrueColor;

	n = ansi_map_states_colorstate(fargs[0], &color_states, &stripped);
	num_transitions = 0;

	for (i = 0; i < n; i++)
	{
		j = random_range(i, n - 1);

		// Output color transition if state changed
		ColorState before = (i > 0) ? color_states[i - 1] : (ColorState){0};
		if (memcmp(&before, &color_states[j], sizeof(ColorState)) != 0)
		{
			num_transitions++;
			escape_offset = 0;
			to_ansi_escape_sequence(escape_buffer, sizeof(escape_buffer), &escape_offset, &color_states[j], color_type);
			if (escape_offset > 0)
			{
				XSAFELBSTR(escape_buffer, buff, bufc);
			}
		}

		// Output the scrambled character
		XSAFELBCHR(stripped[j], buff, bufc);

		// Swap color states and characters
		ColorState temp_state = color_states[j];
		color_states[j] = color_states[i];
		color_states[i] = temp_state;

		char temp_char = stripped[j];
		stripped[j] = stripped[i];
		stripped[i] = temp_char;
	}

	// Reset to normal at the end, if required
	escape_offset = 0;
	ColorState reset_state = (ColorState){.reset = ColorStatusReset};
	to_ansi_escape_sequence(escape_buffer, sizeof(escape_buffer), &escape_offset, &reset_state, color_type);
	if (escape_offset > 0 && num_transitions > 0)
	{
		XSAFELBSTR(escape_buffer, buff, bufc);
	}

	XFREE(color_states);
	XFREE(stripped);
}
