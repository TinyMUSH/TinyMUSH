/**
 * @file eval_subst.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Percent-substitution handler for the expression evaluator
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * This module implements _eval_expand_percent(), the handler for the
 * case '%': branch inside eval_expression_string(). It processes all
 * %-substitution codes (%0-%9, %r, %t, %b, %x, %v, %q, pronouns,
 * %#, %!, %n, %l, %@, etc.) and writes the expanded text to buff.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "ansi.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

/* eval_gender.c */
extern void _eval_gender_emit_pronoun(char code, int *gender, dbref cause, char *buff, char **bufc);

/**
 * @brief Expand a single %-substitution code into buff.
 *
 * Handles the entire body of the `case '%':` branch from
 * eval_expression_string(). On entry, @p *dstr points at the character
 * immediately after the '%' that triggered the dispatch. On return,
 * @p *dstr points at the last character of the substitution token (the
 * caller's `(*dstr)++` will advance past it).
 *
 * @param dstr      Pointer to the current input cursor (advanced in place)
 * @param buff      Start of the destination output buffer (LBUF-sized)
 * @param bufc      Pointer to the write cursor inside @p buff (updated in place)
 * @param player    DBref of the executor
 * @param caller    DBref of the immediate caller
 * @param cause     DBref of the enactor (used for pronouns, name, dbref)
 * @param eval      Evaluation flags (EV_NO_LOCATION, etc.)
 * @param cargs     Command argument array for %0-%9 substitutions
 * @param ncargs    Number of entries in @p cargs
 * @param hilite_mode Pointer to hilite/bright mode tracking state for %x codes
 * @param at_space  Pointer to the space-compression state flag (set to 0)
 * @param ansi      Pointer to the ANSI-active flag (set when a color code is emitted)
 * @param gender    Pointer to the cached gender value for pronoun substitutions
 * @param xtbuf     Scratch buffer (SBUF_SIZE) for intermediate string building
 */
void _eval_expand_percent(char **dstr, char *buff, char **bufc,
						  dbref player, dbref caller, dbref cause, int eval,
						  char *cargs[], int ncargs, bool *hilite_mode,
						  int *at_space, int *ansi, int *gender, char *xtbuf)
{
	char savec, ch;
	char *savepos, *xptr, *xtp;
	dbref aowner = NOTHING;
	int i, aflags, alen;
	ATTR *ap = NULL;
	char *atr_gotten = NULL;
	VARENT *xvar = NULL;

	*at_space = 0;
	(*dstr)++;
	savec = **dstr;
	savepos = *bufc;

	switch (savec)
	{
	case '\0': /* Null - all done */
		(*dstr)--;
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': /* Command argument number N */
		i = (**dstr - '0');

		if ((i < ncargs) && (cargs[i] != NULL))
		{
			XSAFELBSTR(cargs[i], buff, bufc);
		}

		break;

	case 'r':
	case 'R': /* Carriage return */
		XSAFECRLF(buff, bufc);
		break;

	case 't':
	case 'T': /* Tab */
		XSAFELBCHR('\t', buff, bufc);
		break;

	case 'B':
	case 'b': /* Blank */
		XSAFELBCHR(' ', buff, bufc);
		break;

	case 'C':
	case 'c': /* %c can be last command executed or legacy color (legacy color is deprecated) */
		if (mushconf.c_cmd_subst)
		{
			XSAFELBSTR(mushstate.curr_cmd, buff, bufc);
			break;
		}
		__attribute__((fallthrough)); /* Legacy color code */

	case 'x':
	case 'X': /* ANSI color - Use centralized parsing from ansi.c */

		(*dstr)++;

		if (!**dstr)
		{
			/* End of string after %x - back up and break */
			(*dstr)--;
			break;
		}

		if (!mushconf.ansi_colors)
		{
			/* ANSI colors disabled - skip the code */
			break;
		}

		/* Parse the color code using ansi_parse_single_x_code().
		   This handles all formats: <color>, <fg/bg>, +<color>, simple letters, etc. */
		{
			ColorState color = {0};
			int consumed = ansi_parse_single_x_code(dstr, &color, hilite_mode);

			if (consumed > 0)
			{
				/* Always process colors at highest fidelity: they get converted to
				   player-appropriate level at send time. */
				ColorType color_type = ColorTypeTrueColor;
				char ansi_buf[256];
				size_t ansi_offset = 0;
				ColorStatus result = to_ansi_escape_sequence(ansi_buf, sizeof(ansi_buf), &ansi_offset, &color, color_type);

				if (result != ColorStatusNone)
				{
					XSAFELBSTR(ansi_buf, buff, bufc);
					*ansi = (result == ColorStatusReset) ? 0 : 1;
				}

				/* dstr has already been advanced by ansi_parse_single_x_code(),
				   compensate for the (*dstr)++ that will happen at the end of the switch */
				(*dstr)--;
			}
			else
			{
				/* Failed to parse - copy the character literally.
				   This maintains backward compatibility for invalid codes. */
				XSAFELBCHR(**dstr, buff, bufc);
			}
		}

		break;

	case '=': /* Equivalent of generic v() attr get */
		(*dstr)++;

		if (**dstr != '<')
		{
			(*dstr)--;
			break;
		}

		xptr = *dstr;
		(*dstr)++;

		if (!**dstr)
		{
			*dstr = xptr;
			break;
		}

		xtp = xtbuf;

		while (**dstr && (**dstr != '>'))
		{
			XSAFESBCHR(**dstr, xtbuf, &xtp);
			(*dstr)++;
		}

		if (**dstr != '>')
		{
			/* Ran off the end. Back up. */
			*dstr = xptr;
			break;
		}

		*xtp = '\0';
		ap = atr_str(xtbuf);

		if (!ap)
		{
			break;
		}

		atr_pget_info(player, ap->number, &aowner, &aflags);

		if (See_attr(player, player, ap, aowner, aflags))
		{
			atr_gotten = atr_pget(player, ap->number, &aowner, &aflags, &alen);
			XSAFESTRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
			XFREE(atr_gotten);
		}

		break;

	case '_': /* x-variable */
		(*dstr)++;

		/* Check for %_<varname> */
		if (**dstr != '<')
		{
			ch = tolower(**dstr);

			if (!**dstr)
			{
				(*dstr)--;
			}

			if (!isalnum(ch))
			{
				break;
			}

			xtp = xtbuf;
			XSAFELTOS(xtbuf, &xtp, player, LBUF_SIZE);
			XSAFECOPYCHR('.', xtbuf, &xtp, SBUF_SIZE - 1);
			XSAFECOPYCHR(ch, xtbuf, &xtp, SBUF_SIZE - 1);
		}
		else
		{
			xptr = *dstr;
			(*dstr)++;

			if (!**dstr)
			{
				*dstr = xptr;
				break;
			}

			xtp = xtbuf;
			XSAFELTOS(xtbuf, &xtp, player, LBUF_SIZE);
			XSAFECOPYCHR('.', xtbuf, &xtp, SBUF_SIZE - 1);

			while (**dstr && (**dstr != '>'))
			{
				/* Copy. No interpretation. */
				ch = tolower(**dstr);
				XSAFESBCHR(ch, xtbuf, &xtp);
				(*dstr)++;
			}

			if (**dstr != '>')
			{
				/* We ran off the end of the string without finding a termination condition. Go back. */
				*dstr = xptr;
				break;
			}
		}

		*xtp = '\0';

		if (!(mushstate.f_limitmask & FN_VARFX) && (xvar = (VARENT *)hashfind(xtbuf, &mushstate.vars_htab)))
		{
			XSAFELBSTR(xvar->text, buff, bufc);
		}

		break;

	case 'V':
	case 'v': /* Variable attribute */
		(*dstr)++;
		ch = toupper(**dstr);

		if (!**dstr)
		{
			(*dstr)--;
		}

		if ((ch < 'A') || (ch > 'Z'))
		{
			break;
		}

		i = A_VA + ch - 'A';
		atr_gotten = atr_pget(player, i, &aowner, &aflags, &alen);
		XSAFESTRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
		XFREE(atr_gotten);
		break;

	case 'Q':
	case 'q': /* Local registers */
		(*dstr)++;

		if (!**dstr)
		{
			(*dstr)--;
			break;
		}

		if (**dstr != '<')
		{
			i = qidx_chartab((unsigned char)**dstr);

			if ((i < 0) || (i >= mushconf.max_global_regs))
			{
				break;
			}

			if (mushstate.rdata && mushstate.rdata->q_alloc > i)
			{
				char *qreg = mushstate.rdata->q_regs[i];
				size_t qlen = mushstate.rdata->q_lens[i];
				XSAFESTRNCAT(buff, bufc, qreg, qlen, LBUF_SIZE);
			}

			if (!**dstr)
			{
				(*dstr)--;
			}

			break;
		}

		xptr = *dstr;
		(*dstr)++;

		if (!**dstr)
		{
			*dstr = xptr;
			break;
		}

		if (!mushstate.rdata || !mushstate.rdata->xr_alloc)
		{
			/* We know there's no result, so we just advance past. */
			while (**dstr && (**dstr != '>'))
			{
				(*dstr)++;
			}

			if (**dstr != '>')
			{
				/* Whoops, no end. Go back. */
				*dstr = xptr;
				break;
			}

			break;
		}

		xtp = xtbuf;

		while (**dstr && (**dstr != '>'))
		{
			XSAFESBCHR(tolower(**dstr), xtbuf, &xtp);
			(*dstr)++;
		}

		if (**dstr != '>')
		{
			/* Ran off the end. Back up. */
			*dstr = xptr;
			break;
		}

		*xtp = '\0';

		for (i = 0; i < mushstate.rdata->xr_alloc; i++)
		{
			if (mushstate.rdata->x_names[i] && !strcmp(xtbuf, mushstate.rdata->x_names[i]))
			{
				XSAFESTRNCAT(buff, bufc, mushstate.rdata->x_regs[i], mushstate.rdata->x_lens[i], LBUF_SIZE);
				break;
			}
		}

		break;

	case 'O':
	case 'o':
	case 'P':
	case 'p':
	case 'S':
	case 's':
	case 'A':
	case 'a': /* Objective pronoun / Possessive adjective / Subjective / Absolute possessive */
		_eval_gender_emit_pronoun(savec, gender, cause, buff, bufc);
		break;

	case '#': /* Invoker DB number */
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, cause, LBUF_SIZE);
		break;

	case '!': /* Executor DB number */
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, player, LBUF_SIZE);
		break;

	case 'N':
	case 'n': /* Invoker name */
		safe_name(cause, buff, bufc);
		break;

	case 'L':
	case 'l': /* Invoker location db# */
		if (!(eval & EV_NO_LOCATION))
		{
			XSAFELBCHR('#', buff, bufc);
			XSAFELTOS(buff, bufc, where_is(cause), LBUF_SIZE);
		}

		break;

	case '@': /* Caller dbref */
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, caller, LBUF_SIZE);
		break;

	case ':': /* Enactor's objID */
		XSAFELBCHR(':', buff, bufc);
		XSAFELTOS(buff, bufc, CreateTime(cause), LBUF_SIZE);
		break;

	case 'M':
	case 'm':
		XSAFELBSTR(mushstate.curr_cmd, buff, bufc);
		break;

	case 'I':
	case 'i': /* itext() equivalent */
	case 'J':
	case 'j': /* itext2() equivalent */
		xtp = *dstr;
		(*dstr)++;

		if (!**dstr)
		{
			(*dstr)--;
		}

		if (**dstr == '-')
		{
			/* use absolute level number */
			(*dstr)++;

			if (!**dstr)
			{
				(*dstr)--;
			}

			if (!isdigit(**dstr))
			{
				break;
			}

			i = (**dstr - '0');
		}
		else
		{
			/* use number as delta back from current */
			if (!mushstate.in_loop || !isdigit(**dstr))
			{
				break;
			}

			i = mushstate.in_loop - 1 - (**dstr - '0');

			if (i < 0)
			{
				break;
			}
		}

		if (i > mushstate.in_loop - 1)
		{
			break;
		}

		if ((*xtp == 'i') || (*xtp == 'I'))
		{
			/* itext() */
			XSAFELBSTR(mushstate.loop_token[i], buff, bufc);
		}
		else
		{
			/* itext2() */
			XSAFELBSTR(mushstate.loop_token2[i], buff, bufc);
		}

		break;

	case '+': /* Arguments to function */
		XSAFELTOS(buff, bufc, ncargs, LBUF_SIZE);
		break;

	case '|': /* Piped command output */
		XSAFELBSTR(mushstate.pout, buff, bufc);
		break;

	case '%': /* Percent - a literal % */
		XSAFELBCHR('%', buff, bufc);
		break;

	default: /* Just copy */
		XSAFELBCHR(**dstr, buff, bufc);
	}

	if (isupper(savec))
	{
		*savepos = toupper(*savepos);
	}
}
