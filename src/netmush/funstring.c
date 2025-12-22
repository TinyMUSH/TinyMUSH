/**
 * @file funstring.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief String manipulation built-ins: search/replace, case handling, formatting
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

/**
 * @brief Is every character in the argument a letter?
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
void fun_isword(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *p = NULL;

	for (p = fargs[0]; *p; p++)
	{
		if (!isalpha(*p))
		{
			XSAFELBCHR('0', buff, bufc);
			return;
		}
	}

	XSAFELBCHR('1', buff, bufc);
}

/**
 * @brief isalnum: is every character in the argument a letter or number?
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
void fun_isalnum(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *p = NULL;

	for (p = fargs[0]; *p; p++)
	{
		if (!isalnum(*p))
		{
			XSAFELBCHR('0', buff, bufc);
			return;
		}
	}

	XSAFELBCHR('1', buff, bufc);
}

/**
 * @brief Is the argument a number?
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
void fun_isnum(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	XSAFELBCHR((is_number(fargs[0]) ? '1' : '0'), buff, bufc);
}

/**
 * @brief Is the argument a valid dbref?
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
void fun_isdbref(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *p = fargs[0];
	dbref dbitem = NOTHING;

	if (*p++ == NUMBER_TOKEN)
	{
		if (*p)
		{
			/**
			 * just the string '#' won't do!
			 *
			 */
			dbitem = parse_dbref_only(p);

			if (Good_obj(dbitem))
			{
				XSAFELBCHR('1', buff, bufc);
				return;
			}
		}
	}

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Is the argument a valid objid?
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
void fun_isobjid(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *p = fargs[0];
	dbref dbitem = NOTHING;

	if (*p++ == NUMBER_TOKEN)
	{
		if (*p)
		{
			/**
			 * just the string '#' won't do!
			 *
			 */
			dbitem = parse_objid(p, NULL);

			if (Good_obj(dbitem))
			{
				XSAFELBCHR('1', buff, bufc);
				return;
			}
		}
	}

	XSAFELBCHR('0', buff, bufc);
}

/**
 * @brief Just eat the contents of the string. Handy for those times when
 *        you've output a bunch of junk in a function call and just want to
 *        dispose of the output (like if you've done an iter() that just did
 *        a bunch of side-effects, and now you have bunches of spaces you need
 *        to get rid of.
 *
 * @param buff Not used
 * @param bufc Not used
 * @param player Not used
 * @param caller Not used
 * @param cause Not used
 * @param fargs Not used
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_null(char *buff __attribute__((unused)), char **bufc __attribute__((unused)), dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
}

/**
 * @brief Squash occurrences of a given character down to 1. We do this
 *        both on leading and trailing chars, as well as internal ones; if the
 *        player wants to trim off the leading and trailing as well, they can
 *        always call trim().
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
void fun_squish(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *tp = NULL, *bp = NULL;
	Delim isep;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &isep, 0))
	{
		return;
	}

	bp = tp = fargs[0];

	while (*tp)
	{
		/**
		 * Move over and copy the non-sep characters
		 *
		 */
		while (*tp && *tp != isep.str[0])
		{
			if (*tp == ESC_CHAR)
			{
				copy_esccode(&tp, &bp);
			}
			else
			{
				*bp++ = *tp++;
			}
		}

		/**
		 * If we've reached the end of the string, leave the loop.
		 *
		 */
		if (!*tp)
		{
			break;
		}

		/**
		 * Otherwise, we've hit a sep char. Move over it, and then
		 * move on to the next non-separator. Note that we're
		 * overwriting our own string as we do this. However, the
		 * other pointer will always be ahead of our current copy
		 * pointer.
		 *
		 */
		*bp++ = *tp++;

		while (*tp && (*tp == isep.str[0]))
		{
			tp++;
		}
	}

	/**
	 * Must terminate the string
	 *
	 */
	*bp = '\0';
	XSAFELBSTR(fargs[0], buff, bufc);
}

/**
 * @brief Trim off unwanted white space.
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
void fun_trim(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *p = NULL, *q = NULL, *endchar = NULL, *ep = NULL;
	int trim = 0;
	Delim isep;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	if (nfargs >= 2)
	{
		switch (tolower(*fargs[1]))
		{
		case 'l':
			trim = TRIM_L;
			break;

		case 'r':
			trim = TRIM_R;
			break;

		default:
			trim = TRIM_L | TRIM_R;
			break;
		}
	}
	else
	{
		trim = TRIM_L | TRIM_R;
	}

	p = fargs[0];

	/**
	 * Single-character delimiters are easy.
	 *
	 */
	if (isep.len == 1)
	{
		if (trim & TRIM_L)
		{
			while (*p == isep.str[0])
			{
				p++;
			}
		}

		if (trim & TRIM_R)
		{
			q = endchar = p;

			while (*q != '\0')
			{
				if (*q == ESC_CHAR)
				{
					skip_esccode(&q);
					endchar = q;
				}
				else if (*q++ != isep.str[0])
				{
					endchar = q;
				}
			}

			*endchar = '\0';
		}

		XSAFELBSTR(p, buff, bufc);
		return;
	}

	/**
	 * Multi-character delimiters take more work.
	 *
	 */
	ep = p + strlen(fargs[0]) - 1; /*!< last char in string */

	if (trim & TRIM_L)
	{
		while (!strncmp(p, isep.str, isep.len) && (p <= ep))
		{
			p = p + isep.len;
		}

		if (p > ep)
		{
			return;
		}
	}

	if (trim & TRIM_R)
	{
		q = endchar = p;

		while (q <= ep)
		{
			if (*q == ESC_CHAR)
			{
				skip_esccode(&q);
				endchar = q;
			}
			else if (!strncmp(q, isep.str, isep.len))
			{
				q = q + isep.len;
			}
			else
			{
				q++;
				endchar = q;
			}
		}

		*endchar = '\0';
	}

	XSAFELBSTR(p, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * fun_after, fun_before: Return substring after or before a specified
 * string.
 */

/**
 * @brief Return substring after a specified string.
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
void fun_after(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *bp = NULL, *cp = NULL, *mp = NULL, *np = NULL, *buf = NULL;
	int ansi_needle = 0, ansi_needle2 = 0, ansi_haystack = 0, ansi_haystack2 = 0;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	bp = fargs[0]; /*!< haystack */
	mp = fargs[1]; /*!< needle */

	/**
	 * Sanity-check arg1 and arg2
	 *
	 */
	if (bp == NULL)
	{
		bp = "";
	}

	if (mp == NULL)
	{
		mp = " ";
	}

	if (!mp || !*mp)
	{
		mp = (char *)" ";
	}

	if ((mp[0] == ' ') && (mp[1] == '\0'))
	{
		bp = Eat_Spaces(bp);
	}

	/**
	 * Get ansi state of the first needle char
	 *
	 */
	ansi_needle = ANST_NONE;

	while (*mp == ESC_CHAR)
	{
		do
		{
			int ansi_mask = 0;
			int ansi_diff = 0;
			unsigned int param_val = 0;
			++(mp);
			if (*(mp) == ANSI_CSI)
			{
				while ((*(++(mp)) & 0xf0) == 0x30)
				{
					if (*(mp) < 0x3a)
					{
						param_val <<= 1;
						param_val += (param_val << 2) + (*(mp) & 0x0f);
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
			while ((*(mp) & 0xf0) == 0x20)
			{
				++(mp);
			}
			if (*(mp) == ANSI_END)
			{
				if (param_val < I_ANSI_LIM)
				{
					ansi_mask |= ansiBitsMask(param_val);
					ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
				}
				ansi_needle = (ansi_needle & ~ansi_mask) | ansi_diff;
				++(mp);
			}
			else if (*(mp))
			{
				++(mp);
			}
		} while (0);

		if (!*mp)
		{
			mp = (char *)" ";
		}
	}

	ansi_haystack = ANST_NORMAL;

	/**
	 * Look for the needle string
	 *
	 */
	while (*bp)
	{
		while (*bp == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(bp);
				if (*(bp) == ANSI_CSI)
				{
					while ((*(++(bp)) & 0xf0) == 0x30)
					{
						if (*(bp) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(bp) & 0x0f);
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
				while ((*(bp) & 0xf0) == 0x20)
				{
					++(bp);
				}
				if (*(bp) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_haystack = (ansi_haystack & ~ansi_mask) | ansi_diff;
					++(bp);
				}
				else if (*(bp))
				{
					++(bp);
				}
			} while (0);
		}

		if ((*bp == *mp) && (ansi_needle == ANST_NONE || ansi_haystack == ansi_needle))
		{
			/**
			 * See if what follows is what we are looking for
			 *
			 */
			ansi_needle2 = ansi_needle;
			ansi_haystack2 = ansi_haystack;
			cp = bp;
			np = mp;

			while (1)
			{
				while (*cp == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(cp);
						if (*(cp) == ANSI_CSI)
						{
							while ((*(++(cp)) & 0xf0) == 0x30)
							{
								if (*(cp) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(cp) & 0x0f);
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
						while ((*(cp) & 0xf0) == 0x20)
						{
							++(cp);
						}
						if (*(cp) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_haystack2 = (ansi_haystack2 & ~ansi_mask) | ansi_diff;
							++(cp);
						}
						else if (*(cp))
						{
							++(cp);
						}
					} while (0);
				}

				while (*np == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(np);
						if (*(np) == ANSI_CSI)
						{
							while ((*(++(np)) & 0xf0) == 0x30)
							{
								if (*(np) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(np) & 0x0f);
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
						while ((*(np) & 0xf0) == 0x20)
						{
							++(np);
						}
						if (*(np) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_needle2 = (ansi_needle2 & ~ansi_mask) | ansi_diff;
							++(np);
						}
						else if (*(np))
						{
							++(np);
						}
					} while (0);
				}

				if ((*cp != *np) || (ansi_needle2 != ANST_NONE && ansi_haystack2 != ansi_needle2) || !*cp || !*np)
				{
					break;
				}

				++cp, ++np;
			}

			if (!*np)
			{
				/**
				 * Yup, return what follows
				 *
				 */
				buf = ansi_transition_esccode(ANST_NORMAL, ansi_haystack2);
				XSAFELBSTR(buf, buff, bufc);
				XFREE(buf);
				XSAFELBSTR(cp, buff, bufc);
				return;
			}
		}

		/**
		 * Nope, continue searching
		 *
		 */
		if (*bp)
		{
			++bp;
		}
	}

	/**
	 * Ran off the end without finding it
	 *
	 */
	return;
}

/**
 * @brief Return substring before a specified string.
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
void fun_before(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *haystack = NULL, *bp = NULL, *cp = NULL, *mp = NULL, *np = NULL, *buf = NULL;
	int ansi_needle = 0, ansi_needle2 = 0, ansi_haystack = 0, ansi_haystack2 = 0;

	if (nfargs == 0)
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	haystack = fargs[0]; /*!< haystack */
	mp = fargs[1];		 /*!< needle */

	/**
	 * Sanity-check arg1 and arg2
	 *
	 */
	if (haystack == NULL)
	{
		haystack = "";
	}

	if (mp == NULL)
	{
		mp = " ";
	}

	if (!mp || !*mp)
	{
		mp = (char *)" ";
	}

	if ((mp[0] == ' ') && (mp[1] == '\0'))
	{
		haystack = Eat_Spaces(haystack);
	}

	bp = haystack;

	/**
	 * Get ansi state of the first needle char
	 *
	 */
	ansi_needle = ANST_NONE;

	while (*mp == ESC_CHAR)
	{
		do
		{
			int ansi_mask = 0;
			int ansi_diff = 0;
			unsigned int param_val = 0;
			++(mp);
			if (*(mp) == ANSI_CSI)
			{
				while ((*(++(mp)) & 0xf0) == 0x30)
				{
					if (*(mp) < 0x3a)
					{
						param_val <<= 1;
						param_val += (param_val << 2) + (*(mp) & 0x0f);
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
			while ((*(mp) & 0xf0) == 0x20)
			{
				++(mp);
			}
			if (*(mp) == ANSI_END)
			{
				if (param_val < I_ANSI_LIM)
				{
					ansi_mask |= ansiBitsMask(param_val);
					ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
				}
				ansi_needle = (ansi_needle & ~ansi_mask) | ansi_diff;
				++(mp);
			}
			else if (*(mp))
			{
				++(mp);
			}
		} while (0);

		if (!*mp)
		{
			mp = (char *)" ";
		}
	}

	ansi_haystack = ANST_NORMAL;

	/**
	 * Look for the needle string
	 *
	 */
	while (*bp)
	{
		/**
		 * See if what follows is what we are looking for
		 *
		 */
		ansi_needle2 = ansi_needle;
		ansi_haystack2 = ansi_haystack;
		cp = bp;
		np = mp;

		while (1)
		{
			while (*cp == ESC_CHAR)
			{
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++(cp);
					if (*(cp) == ANSI_CSI)
					{
						while ((*(++(cp)) & 0xf0) == 0x30)
						{
							if (*(cp) < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*(cp) & 0x0f);
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
					while ((*(cp) & 0xf0) == 0x20)
					{
						++(cp);
					}
					if (*(cp) == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						ansi_haystack2 = (ansi_haystack2 & ~ansi_mask) | ansi_diff;
						++(cp);
					}
					else if (*(cp))
					{
						++(cp);
					}
				} while (0);
			}

			while (*np == ESC_CHAR)
			{
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++(np);
					if (*(np) == ANSI_CSI)
					{
						while ((*(++(np)) & 0xf0) == 0x30)
						{
							if (*(np) < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*(np) & 0x0f);
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
					while ((*(np) & 0xf0) == 0x20)
					{
						++(np);
					}
					if (*(np) == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						ansi_needle2 = (ansi_needle2 & ~ansi_mask) | ansi_diff;
						++(np);
					}
					else if (*(np))
					{
						++(np);
					}
				} while (0);
			}

			if ((*cp != *np) || (ansi_needle2 != ANST_NONE && ansi_haystack2 != ansi_needle2) || !*cp || !*np)
			{
				break;
			}

			++cp, ++np;
		}

		if (!*np)
		{
			/**
			 * Yup, return what came before this
			 *
			 */
			*bp = '\0';
			XSAFELBSTR(haystack, buff, bufc);
			buf = ansi_transition_esccode(ansi_haystack, ANST_NORMAL);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);
			return;
		}

		/**
		 * Nope, continue searching
		 *
		 */
		while (*bp == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(bp);
				if (*(bp) == ANSI_CSI)
				{
					while ((*(++(bp)) & 0xf0) == 0x30)
					{
						if (*(bp) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(bp) & 0x0f);
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
				while ((*(bp) & 0xf0) == 0x20)
				{
					++(bp);
				}
				if (*(bp) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_haystack = (ansi_haystack & ~ansi_mask) | ansi_diff;
					++(bp);
				}
				else if (*(bp))
				{
					++(bp);
				}
			} while (0);
		}

		if (*bp)
		{
			++bp;
		}
	}

	/**
	 * Ran off the end without finding it
	 *
	 */
	XSAFELBSTR(haystack, buff, bufc);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

/**
 * @brief Lowercase string
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
void fun_lcstr(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *ap = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	while (*ap)
	{
		if (*ap == ESC_CHAR)
		{
			skip_esccode(&ap);
		}
		else
		{
			*ap = tolower(*ap);
			ap++;
		}
	}
}

/**
 * @brief Uppercase string
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
void fun_ucstr(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *ap = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	while (*ap)
	{
		if (*ap == ESC_CHAR)
		{
			skip_esccode(&ap);
		}
		else
		{
			*ap = toupper(*ap);
			ap++;
		}
	}
}

/**
 * @brief Capitalize string
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
void fun_capstr(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *ap = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	while (*ap == ESC_CHAR)
	{
		skip_esccode(&ap);
	}

	*ap = toupper(*ap);
}

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
void fun_space(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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
void fun_ljust(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int spaces = 0, max = 0, i = 0, slen = 0;
	char *tp = NULL, *fillchars = NULL;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	spaces = (int)strtol(fargs[1], (char **)NULL, 10) - strip_ansi_len(fargs[0]);
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
		fillchars = strip_ansi(fargs[2]);
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
void fun_rjust(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int spaces = 0, max = 0, i = 0, slen = 0;
	char *tp = NULL, *fillchars = NULL;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	spaces = (int)strtol(fargs[1], (char **)NULL, 10) - strip_ansi_len(fargs[0]);

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
		fillchars = strip_ansi(fargs[2]);
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
void fun_center(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *tp = NULL, *fillchars = NULL;
	int len = 0, lead_chrs = 0, trail_chrs = 0, width = 0, max = 0, i = 0, slen = 0;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	width = (int)strtol(fargs[1], (char **)NULL, 10);
	len = strip_ansi_len(fargs[0]);
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
		fillchars = (char *)strip_ansi(fargs[2]);
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
void fun_left(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s = fargs[0];
	int nchars = (int)strtol(fargs[1], (char **)NULL, 10);
	int ansi_state = ANST_NORMAL;

	if (nchars <= 0)
	{
		return;
	}

	for (int count = 0; (count < nchars) && *s; count++)
	{
		while (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}

		if (*s)
		{
			++s;
		}
	}

	XSAFESTRNCAT(buff, bufc, fargs[0], s - fargs[0], LBUF_SIZE);
	s = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	XSAFELBSTR(s, buff, bufc);
	XFREE(s);
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
void fun_right(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s, *buf;
	int count, start, nchars;
	int ansi_state = ANST_NORMAL;
	s = fargs[0];
	nchars = (int)strtol(fargs[1], (char **)NULL, 10);
	start = strip_ansi_len(s) - nchars;

	if (nchars <= 0)
	{
		return;
	}

	if (start < 0)
	{
		nchars += start;

		if (nchars <= 0)
		{
			return;
		}

		start = 0;
	}

	while (*s == ESC_CHAR)
	{
		do
		{
			int ansi_mask = 0;
			int ansi_diff = 0;
			unsigned int param_val = 0;
			++(s);
			if (*(s) == ANSI_CSI)
			{
				while ((*(++(s)) & 0xf0) == 0x30)
				{
					if (*(s) < 0x3a)
					{
						param_val <<= 1;
						param_val += (param_val << 2) + (*(s) & 0x0f);
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
			while ((*(s) & 0xf0) == 0x20)
			{
				++(s);
			}
			if (*(s) == ANSI_END)
			{
				if (param_val < I_ANSI_LIM)
				{
					ansi_mask |= ansiBitsMask(param_val);
					ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
				}
				ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
				++(s);
			}
			else if (*(s))
			{
				++(s);
			}
		} while (0);
	}

	for (count = 0; (count < start) && *s; count++)
	{
		++s;

		while (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}
	}

	if (*s)
	{
		buf = ansi_transition_esccode(ANST_NORMAL, ansi_state);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
	}

	XSAFELBSTR(s, buff, bufc);
}

/**
 * @brief If the line ends with CRLF, CR, or LF, chop it off
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
void fun_chomp(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *bb_p = *bufc;

	XSAFELBSTR(fargs[0], buff, bufc);

	if (*bufc != bb_p && (*bufc)[-1] == '\n')
	{
		(*bufc)--;
	}

	if (*bufc != bb_p && (*bufc)[-1] == '\r')
	{
		(*bufc)--;
	}
}

/**
 * @brief Exact-string compare
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
void fun_comp(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int x = strcmp(fargs[0], fargs[1]);

	if (x > 0)
	{
		XSAFELBCHR('1', buff, bufc);
	}
	else if (x < 0)
	{
		XSAFELBSTR("-1", buff, bufc);
	}
	else
	{
		XSAFELBCHR('0', buff, bufc);
	}
}

/**
 * @brief non-case-sensitive string compare
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
void fun_streq(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	XSAFEBOOL(buff, bufc, !string_compare(fargs[0], fargs[1]));
}

/**
 * @brief wildcard string compare
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
void fun_strmatch(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	/**
	 * Check if we match the whole string.  If so, return 1
	 *
	 */
	XSAFEBOOL(buff, bufc, quick_wild(fargs[1], fargs[0]));
}

/**
 * @brief Edit text.
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
void fun_edit(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *tstr = NULL;

	edit_string(fargs[0], &tstr, fargs[1], fargs[2]);
	XSAFELBSTR(tstr, buff, bufc);
	XFREE(tstr);
}

/**
 * @brief Given two strings and a character, merge the two strings by
 *        replacing characters in string1 that are the same as the given
 *        character by the corresponding character in string2 (by position).
 *        The strings must be of the same length.
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
void fun_merge(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *str = NULL, *rep = NULL, c = 0;

	/**
	 * do length checks first
	 *
	 */
	if (strlen(fargs[0]) != strlen(fargs[1]))
	{
		XSAFELBSTR("#-1 STRING LENGTHS MUST BE EQUAL", buff, bufc);
		return;
	}

	if (strlen(fargs[2]) > 1)
	{
		XSAFELBSTR("#-1 TOO MANY CHARACTERS", buff, bufc);
		return;
	}

	/**
	 * find the character to look for. null character is considered a space
	 *
	 */
	if (!*fargs[2])
	{
		c = ' ';
	}
	else
	{
		c = *fargs[2];
	}

	/**
	 * walk strings, copy from the appropriate string
	 *
	 */
	for (str = fargs[0], rep = fargs[1]; *str && *rep && ((*bufc - buff) < (LBUF_SIZE - 1)); str++, rep++, (*bufc)++)
	{
		if (*str == c)
		{
			**bufc = *rep;
		}
		else
		{
			**bufc = *str;
		}
	}

	return;
}

/**
 * @brief Returns <string> after replacing the characters [](){};,%\$ with spaces.
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
void fun_secure(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s = fargs[0];

	while (*s)
	{
		switch (*s)
		{
		case ESC_CHAR:
			safe_copy_esccode(&s, buff, bufc);
			continue;

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
 * @brief Returns <string> after adding an escape character (\) at the start
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
void fun_escape(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s = fargs[0], *d = NULL;

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
		case ESC_CHAR:
			safe_copy_esccode(&s, buff, bufc);
			continue;

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
void fun_esc(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s = fargs[0];

	if (!*s)
	{
		return;
	}

	while (*s)
	{
		switch (*s)
		{
		case ESC_CHAR:
			safe_copy_esccode(&s, buff, bufc);
			continue;

		case '%':
		case '\\':
		case '[':
		case ']':
			XSAFELBCHR('\\', buff, bufc);
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

/**
 * @brief Highlight a string using ANSI terminal effects.
 *
 * +colorname
 * #RRGGBB <#RRGGBB>
 * <RR GG BB>
 * XTERN
 * Old Style
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
void fun_ansi(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int ansi_state = 0;
	int xterm = 0;
	char *s = NULL, *xtp = NULL;
	char xtbuf[SBUF_SIZE];

	if (!mushconf.ansi_colors)
	{
		XSAFELBSTR(fargs[1], buff, bufc);
		return;
	}

	if (!fargs[0] || !*fargs[0])
	{
		XSAFELBSTR(fargs[1], buff, bufc);
		return;
	}

	track_ansi_letters(fargs[0], &ansi_state);
	s = ansi_transition_esccode(ANST_NONE, ansi_state);
	XSAFELBSTR(s, buff, bufc);
	XFREE(s);

	/**
	 * Now that normal ansi has been done, time for xterm
	 *
	 */
	s = fargs[0];

	// Parsing foreground color
	XMEMSET(xtbuf, 0, SBUF_SIZE);
	int is_chevron = 0;
	if (*s == '<') {
		is_chevron = 1;
		s++;
	}
	xtp = xtbuf;
	while (*s && ((is_chevron && *s != '>' && *s != '/' && *s != '\0') || (!is_chevron && *s != '/' && *s != '\0'))) {
		XSAFESBCHR(*s, xtbuf, &xtp);
		s++;
	}
	// Si on s'arrête sur '/', on ne l'avance pas ici, il sera traité pour le background
	if (is_chevron && *s == '>')
		s++;
	*xtp = '\0';
	int r = -1, g = -1, b = -1;
	if (xtbuf[0] == '#' && strlen(xtbuf) == 7) {
		sscanf(xtbuf + 1, "%2x%2x%2x", &r, &g, &b);
	} else if (sscanf(xtbuf, "%d %d %d", &r, &g, &b) == 3) {
		// Format "R G B"
	}
	if (r >= 0 && g >= 0 && b >= 0 && r <= 255 && g <= 255 && b <= 255) {
		XSNPRINTF(xtbuf, SBUF_SIZE, "\033[38;2;%d;%d;%dm", r, g, b);
	} else if (xtbuf[0]) {
		int i = str2xterm(xtbuf);
		XSNPRINTF(xtbuf, SBUF_SIZE, "%s%d%c", ANSI_XTERM_FG, i, ANSI_END);
	}
	XSAFELBSTR(xtbuf, buff, bufc);
	if (xtbuf[0])
		xterm = 1;
	// Parsing background color only if '/' is present
	if (*s == '/') {
		s++;
		// Vérifie qu'il y a bien une couleur après le '/'
		if (*s) {
			XMEMSET(xtbuf, 0, SBUF_SIZE);
			is_chevron = 0;
			if (*s == '<') {
				is_chevron = 1;
				s++;
			}
			xtp = xtbuf;
			while (*s && ((is_chevron && *s != '>' && *s != '\0') || (!is_chevron && *s != '\0'))) {
				XSAFESBCHR(*s, xtbuf, &xtp);
				s++;
			}
			if (is_chevron && *s == '>')
				s++;
			*xtp = '\0';
			// Vérifie que xtbuf n'est pas vide ou uniquement des espaces
			int only_spaces = 1;
			for (char *p = xtbuf; *p; ++p) {
				if (!isspace((unsigned char)*p)) {
					only_spaces = 0;
					break;
				}
			}
			if (xtbuf[0] && !only_spaces) {
				r = g = b = -1;
				if (xtbuf[0] == '#' && strlen(xtbuf) == 7) {
					sscanf(xtbuf + 1, "%2x%2x%2x", &r, &g, &b);
				} else if (sscanf(xtbuf, "%d %d %d", &r, &g, &b) == 3) {
					// Format "R G B"
				}
				if (r >= 0 && g >= 0 && b >= 0 && r <= 255 && g <= 255 && b <= 255) {
					XSNPRINTF(xtbuf, SBUF_SIZE, "\033[48;2;%d;%d;%dm", r, g, b);
				} else {
					int i = str2xterm(xtbuf);
					XSNPRINTF(xtbuf, SBUF_SIZE, "%s%d%c", ANSI_XTERM_BG, i, ANSI_END);
				}
				if (xtbuf[0]) {
					XSAFELBSTR(xtbuf, buff, bufc);
					xterm = 1;
				}
			}
		}
	}

	s = fargs[1];

	while (*s)
	{
		if (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}
		else
		{
			++s;
		}
	}

	XSAFELBSTR(fargs[1], buff, bufc);
	s = ansi_transition_esccode(ansi_state, ANST_NONE);
	XSAFELBSTR(s, buff, bufc);
	XFREE(s);

	if (xterm)
	{
		XSAFEANSINORMAL(buff, bufc);
	}
}

void fun_stripansi(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *buf;
	buf = strip_ansi(fargs[0]);
	XSAFELBSTR(buf, buff, bufc);
	XFREE(buf);
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
		else if (*in == ESC_CHAR)
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
		else if (*p == ESC_CHAR)
		{
			skip_esccode(&p);
		}
		else
		{
			++p;
		}
	}
}

void fun_encrypt(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 1);
}

void fun_decrypt(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	crypt_code(buff, bufc, fargs[1], fargs[0], 0);
}

/*
 * ---------------------------------------------------------------------------
 * fun_scramble:  randomizes the letters in a string.
 */

/* Borrowed from PennMUSH 1.50 */
void fun_scramble(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int n, i, j, ansi_state, *ansi_map;
	char *stripped, *buf;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	n = ansi_map_states(fargs[0], &ansi_map, &stripped);
	ansi_state = ANST_NORMAL;

	for (i = 0; i < n; i++)
	{
		j = random_range(i, n - 1);

		if (ansi_state != ansi_map[j])
		{
			buf = ansi_transition_esccode(ansi_state, ansi_map[j]);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);
			ansi_state = ansi_map[j];
		}

		XSAFELBCHR(stripped[j], buff, bufc);
		ansi_map[j] = ansi_map[i];
		stripped[j] = stripped[i];
	}

	buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	XSAFELBSTR(buf, buff, bufc);
	XFREE(buf);
	XFREE(ansi_map);
	XFREE(stripped);
}

/*
 * ---------------------------------------------------------------------------
 * fun_reverse: reverse a string
 */

void fun_reverse(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int n, *ansi_map, ansi_state;
	char *stripped, *buf;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	n = ansi_map_states(fargs[0], &ansi_map, &stripped);
	ansi_state = ansi_map[n];

	while (n--)
	{
		if (ansi_state != ansi_map[n])
		{
			buf = ansi_transition_esccode(ansi_state, ansi_map[n]);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);
			ansi_state = ansi_map[n];
		}

		XSAFELBCHR(stripped[n], buff, bufc);
	}

	buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	XSAFELBSTR(buf, buff, bufc);
	XFREE(buf);
	XFREE(ansi_map);
	XFREE(stripped);
}

/*
 * ---------------------------------------------------------------------------
 * fun_mid: mid(foobar,2,3) returns oba
 */

void fun_mid(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s, *savep, *buf;
	int count, start, nchars;
	int ansi_state = ANST_NORMAL;
	s = fargs[0];
	start = (int)strtol(fargs[1], (char **)NULL, 10);
	nchars = (int)strtol(fargs[2], (char **)NULL, 10);

	if (nchars <= 0)
	{
		return;
	}

	if (start < 0)
	{
		nchars += start;

		if (nchars <= 0)
		{
			return;
		}

		start = 0;
	}

	while (*s == ESC_CHAR)
	{
		do
		{
			int ansi_mask = 0;
			int ansi_diff = 0;
			unsigned int param_val = 0;
			++(s);
			if (*(s) == ANSI_CSI)
			{
				while ((*(++(s)) & 0xf0) == 0x30)
				{
					if (*(s) < 0x3a)
					{
						param_val <<= 1;
						param_val += (param_val << 2) + (*(s) & 0x0f);
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
			while ((*(s) & 0xf0) == 0x20)
			{
				++(s);
			}
			if (*(s) == ANSI_END)
			{
				if (param_val < I_ANSI_LIM)
				{
					ansi_mask |= ansiBitsMask(param_val);
					ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
				}
				ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
				++(s);
			}
			else if (*(s))
			{
				++(s);
			}
		} while (0);
	}

	for (count = 0; (count < start) && *s; ++count)
	{
		++s;

		while (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}
	}

	if (*s)
	{
		buf = ansi_transition_esccode(ANST_NORMAL, ansi_state);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
	}

	savep = s;

	for (count = 0; (count < nchars) && *s; ++count)
	{
		while (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}

		if (*s)
		{
			++s;
		}
	}

	XSAFESTRNCAT(buff, bufc, savep, s - savep, LBUF_SIZE);
	buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
	XSAFELBSTR(buf, buff, bufc);
	XFREE(buf);
}

/*
 * ---------------------------------------------------------------------------
 * fun_translate: Takes a string and a second argument. If the second
 * argument is 0 or s, control characters are converted to spaces. If it's 1
 * or p, they're converted to percent substitutions.
 */

void fun_translate(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	/*
	 * Strictly speaking, we're just checking the first char
	 */

	if (nfargs > 1 && (fargs[1][0] == 's' || fargs[1][0] == '0'))
	{
		char *s = translate_string(fargs[0], 0);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
	else if (nfargs > 1 && fargs[1][0] == 'p')
	{
		char *s = translate_string(fargs[0], 1);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
	else
	{
		char *s = translate_string(fargs[0], 1);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_pos: Find a word in a string
 */

void fun_pos(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int i = 1;
	char *b, *b1, *s, *s1, *t, *u;
	i = 1;
	b1 = b = strip_ansi(fargs[0]);
	s1 = s = strip_ansi(fargs[1]);

	if (*b && !*(b + 1))
	{ /* single character */
		t = strchr(s, *b);

		if (t)
		{
			XSAFELTOS(buff, bufc, (int)(t - s + 1), LBUF_SIZE);
		}
		else
		{
			XSAFENOTHING(buff, bufc);
		}

		XFREE(s1);
		XFREE(b1);
		return;
	}

	while (*s)
	{
		u = s;
		t = b;

		while (*t && *t == *u)
		{
			++t, ++u;
		}

		if (*t == '\0')
		{
			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
			XFREE(s1);
			XFREE(b1);
			return;
		}

		++i, ++s;
	}

	XSAFENOTHING(buff, bufc);
	XFREE(s1);
	XFREE(b1);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * fun_lpos: Find all occurrences of a character in a string, and return a
 * space-separated list of the positions, starting at 0. i.e.,
 * lpos(a-bc-def-g,-) ==> 1 4 8
 */

void fun_lpos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *s, *bb_p, *scratch_chartab, *buf;
	;
	int i;
	Delim osep;

	if (!fargs[0] || !*fargs[0])
	{
		return;
	}

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	scratch_chartab = (char *)XCALLOC(256, sizeof(char), "scratch_chartab");

	if (!fargs[1] || !*fargs[1])
	{
		scratch_chartab[(unsigned char)' '] = 1;
	}
	else
	{
		for (s = fargs[1]; *s; s++)
		{
			scratch_chartab[(unsigned char)*s] = 1;
		}
	}

	bb_p = *bufc;
	buf = s = strip_ansi(fargs[0]);

	for (i = 0; *s; i++, s++)
	{
		if (scratch_chartab[(unsigned char)*s])
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
		}
	}

	XFREE(buf);
	XFREE(scratch_chartab);
}

/*
 * ---------------------------------------------------------------------------
 * diffpos: Return the position of the first character where s1 and s2 are
 * different.
 */

void fun_diffpos(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int i;
	char *s1, *s2;

	for (i = 0, s1 = fargs[0], s2 = fargs[1]; *s1 && *s2; i++, s1++, s2++)
	{
		while (*s1 == ESC_CHAR)
		{
			skip_esccode(&s1);
		}

		while (*s2 == ESC_CHAR)
		{
			skip_esccode(&s2);
		}

		if (*s1 != *s2)
		{
			XSAFELTOS(buff, bufc, i, LBUF_SIZE);
			return;
		}
	}

	XSAFELTOS(buff, bufc, -1, LBUF_SIZE);
}

/*
 * ---------------------------------------------------------------------------
 * Take a character position and return which word that char is in.
 * wordpos(<string>, <charpos>)
 */

void fun_wordpos(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int charpos, i;
	char *buf, *cp, *tp, *xp;
	Delim isep;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
	{
		return;
	}

	charpos = (int)strtol(fargs[1], (char **)NULL, 10);
	buf = cp = strip_ansi(fargs[0]);

	if ((charpos > 0) && (charpos <= (int)strlen(cp)))
	{
		tp = &(cp[charpos - 1]);
		cp = trim_space_sep(cp, &isep);
		xp = split_token(&cp, &isep);

		for (i = 1; xp; i++)
		{
			if (tp < (xp + strlen(xp)))
			{
				break;
			}

			xp = split_token(&cp, &isep);
		}

		XSAFELTOS(buff, bufc, i, LBUF_SIZE);
		XFREE(buf);
		return;
	}

	XSAFENOTHING(buff, bufc);
	XFREE(buf);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * Take a character position and return what color that character would be.
 * ansipos(<string>, <charpos>[, <type>])
 */

void fun_ansipos(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int charpos, i, ansi_state;
	char *s;

	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
	{
		return;
	}

	s = fargs[0];
	charpos = (int)strtol(fargs[1], (char **)NULL, 10);
	ansi_state = ANST_NORMAL;
	i = 0;

	while (*s && i < charpos)
	{
		if (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}
		else
		{
			++s, ++i;
		}
	}

	if (nfargs > 2 && (fargs[2][0] == 'e' || fargs[2][0] == '0'))
	{
		s = ansi_transition_esccode(ANST_NONE, ansi_state);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
	else if (nfargs > 2 && (fargs[2][0] == 'p' || fargs[2][0] == '1'))
	{
		s = ansi_transition_mushcode(ANST_NONE, ansi_state);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
	else
	{
		s = ansi_transition_letters(ANST_NONE, ansi_state);
		XSAFELBSTR(s, buff, bufc);
		XFREE(s);
	}
}

/*
 * ---------------------------------------------------------------------------
 * fun_repeat: repeats a string
 */

void fun_repeat(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
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

void perform_border(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *l_fill = NULL, *r_fill = NULL, *bb_p = NULL, *sl = NULL, *el = NULL, *sw = NULL, *ew = NULL, *buf = NULL;
	int sl_ansi_state = 0, el_ansi_state = 0, sw_ansi_state = 0, ew_ansi_state = 0, sl_pos = 0;
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
	sl_ansi_state = el_ansi_state = ANST_NORMAL;
	sw_ansi_state = ew_ansi_state = ANST_NORMAL;
	sl_pos = el_pos = sw_pos = ew_pos = 0;

	while (1)
	{
		/*
		 * Locate the next start-of-word (SW)
		 */
		for (sw = ew, sw_ansi_state = ew_ansi_state, sw_pos = ew_pos; *sw; ++sw)
		{
			switch (*sw)
			{
			case ESC_CHAR:
				do
				{
					int ansi_mask = 0;
					int ansi_diff = 0;
					unsigned int param_val = 0;
					++(sw);
					if (*(sw) == ANSI_CSI)
					{
						while ((*(++(sw)) & 0xf0) == 0x30)
						{
							if (*(sw) < 0x3a)
							{
								param_val <<= 1;
								param_val += (param_val << 2) + (*(sw) & 0x0f);
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
					while ((*(sw) & 0xf0) == 0x20)
					{
						++(sw);
					}
					if (*(sw) == ANSI_END)
					{
						if (param_val < I_ANSI_LIM)
						{
							ansi_mask |= ansiBitsMask(param_val);
							ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
						}
						sw_ansi_state = (sw_ansi_state & ~ansi_mask) | ansi_diff;
						++(sw);
					}
					else if (*(sw))
					{
						++(sw);
					}
				} while (0);
				--sw;
				continue;

			case '\t':
			case '\r':
				*sw = ' ';
				//[[fallthrough]];
				__attribute__((fallthrough));
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
				case ESC_CHAR:
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(ew);
						if (*(ew) == ANSI_CSI)
						{
							while ((*(++(ew)) & 0xf0) == 0x30)
							{
								if (*(ew) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(ew) & 0x0f);
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
						while ((*(ew) & 0xf0) == 0x20)
						{
							++(ew);
						}
						if (*(ew) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ew_ansi_state = (ew_ansi_state & ~ansi_mask) | ansi_diff;
							++(ew);
						}
						else if (*(ew))
						{
							++(ew);
						}
					} while (0);
					--ew;
					continue;

				case '\r':
				case '\t':
					*ew = ' ';

					/*
					 * FALLTHRU
					 */
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
		buf = ansi_transition_esccode(ANST_NORMAL, sl_ansi_state);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
		/*
		 * Print the words
		 */
		XSAFESTRNCAT(buff, bufc, sl, el - sl, LBUF_SIZE);
		/*
		 * Back to ansi normal
		 */
		buf = ansi_transition_esccode(el_ansi_state, ANST_NORMAL);
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

void perform_align(int n_cols, char **raw_colstrs, char **data, char fillc, Delim col_sep, Delim row_sep, char *buff, char **bufc)
{
	char *bb_p = NULL, *l_p = NULL, **xsl = NULL, **xel = NULL, **xsw = NULL, **xew = NULL;
	char *p = NULL, *sl = NULL, *el = NULL, *sw = NULL, *ew = NULL, *buf = NULL;
	int *xsl_a = NULL, *xel_a = NULL, *xsw_a = NULL, *xew_a = NULL;
	int *xsl_p = NULL, *xel_p = NULL, *xsw_p = NULL, *xew_p = NULL;
	int *col_widths = NULL, *col_justs = NULL, *col_done = NULL;
	int i = 0, n = 0, sl_ansi_state = 0, el_ansi_state = 0, sw_ansi_state = 0, ew_ansi_state = 0;
	int sl_pos = 0, el_pos = 0, sw_pos = 0, ew_pos = 0;
	int width = 0, just = 0, nleft = 0, max = 0, lead_chrs = 0;
	int n_done = 0, pending_coaright = 0;
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
	xsl_a = (int *)XCALLOC(n_cols, sizeof(int), "xsl_a");
	xel_a = (int *)XCALLOC(n_cols, sizeof(int), "xel_a");
	xsw_a = (int *)XCALLOC(n_cols, sizeof(int), "xsw_a");
	xew_a = (int *)XCALLOC(n_cols, sizeof(int), "xew_a");
	xsl_p = (int *)XCALLOC(n_cols, sizeof(int), "xsl_p");
	xel_p = (int *)XCALLOC(n_cols, sizeof(int), "xel_p");
	xsw_p = (int *)XCALLOC(n_cols, sizeof(int), "xsw_p");
	xew_p = (int *)XCALLOC(n_cols, sizeof(int), "xew_p");

	/* XCALLOC() auto-initializes things to 0, so just do the other things */

	for (i = 0; i < n_cols; i++)
	{
		xew[i] = data[i];
		xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ANST_NORMAL;
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
					case ESC_CHAR:
						do
						{
							int ansi_mask = 0;
							int ansi_diff = 0;
							unsigned int param_val = 0;
							++(sw);
							if (*(sw) == ANSI_CSI)
							{
								while ((*(++(sw)) & 0xf0) == 0x30)
								{
									if (*(sw) < 0x3a)
									{
										param_val <<= 1;
										param_val += (param_val << 2) + (*(sw) & 0x0f);
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
							while ((*(sw) & 0xf0) == 0x20)
							{
								++(sw);
							}
							if (*(sw) == ANSI_END)
							{
								if (param_val < I_ANSI_LIM)
								{
									ansi_mask |= ansiBitsMask(param_val);
									ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
								}
								sw_ansi_state = (sw_ansi_state & ~ansi_mask) | ansi_diff;
								++(sw);
							}
							else if (*(sw))
							{
								++(sw);
							}
						} while (0);
						--sw;
						continue;

					case '\t':
					case '\r':
						*sw = ' ';
						//[[fallthrough]];
						__attribute__((fallthrough));
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
						xsl_a[i] = xel_a[i] = ANST_NORMAL;
						xsw_a[i] = xew_a[i] = ANST_NORMAL;
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
						case ESC_CHAR:
							do
							{
								int ansi_mask = 0;
								int ansi_diff = 0;
								unsigned int param_val = 0;
								++(ew);
								if (*(ew) == ANSI_CSI)
								{
									while ((*(++(ew)) & 0xf0) == 0x30)
									{
										if (*(ew) < 0x3a)
										{
											param_val <<= 1;
											param_val += (param_val << 2) + (*(ew) & 0x0f);
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
								while ((*(ew) & 0xf0) == 0x20)
								{
									++(ew);
								}
								if (*(ew) == ANSI_END)
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									ew_ansi_state = (ew_ansi_state & ~ansi_mask) | ansi_diff;
									++(ew);
								}
								else if (*(ew))
								{
									++(ew);
								}
							} while (0);
							--ew;
							continue;

						case '\r':
						case '\t':
							*ew = ' ';

							/*
							 * FALLTHRU
							 */
						case ' ':
						case '\n':
							break;

						case BEEP_CHAR:
							continue;

						default:

							/*
							 * Break up long
							 * words
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
			buf = ansi_transition_esccode(ANST_NORMAL, sl_ansi_state);
			XSAFELBSTR(buf, buff, bufc);
			XFREE(buf);
			/*
			 * Print the words
			 */
			XSAFESTRNCAT(buff, bufc, sl, el - sl, LBUF_SIZE);
			/*
			 * Back to ansi normal
			 */
			buf = ansi_transition_esccode(el_ansi_state, ANST_NORMAL);
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
					xsl_a[i] = xel_a[i] = xsw_a[i] = xew_a[i] = ANST_NORMAL;
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

	perform_align(n_cols, raw_colstrs, fargs + 1, filler.str[0], col_sep, row_sep, buff, bufc);
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

	perform_align(n_cols, raw_colstrs, data, filler.str[0], col_sep, row_sep, buff, bufc);
	XFREE(raw_colstrs);
	XFREE(data);
}

/*
 * ---------------------------------------------------------------------------
 * String concatenation.
 */

void fun_cat(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int i;
	XSAFELBSTR(fargs[0], buff, bufc);

	for (i = 1; i < nfargs; i++)
	{
		XSAFELBCHR(' ', buff, bufc);
		XSAFELBSTR(fargs[i], buff, bufc);
	}
}

void fun_strcat(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	int i;
	XSAFELBSTR(fargs[0], buff, bufc);

	for (i = 1; i < nfargs; i++)
	{
		XSAFELBSTR(fargs[i], buff, bufc);
	}
}

void fun_join(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	Delim osep;
	char *bb_p;
	int i;

	if (nfargs < 1)
	{
		return;
	}

	if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 1, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
	{
		return;
	}

	bb_p = *bufc;

	for (i = 1; i < nfargs; i++)
	{
		if (*fargs[i])
		{
			if (*bufc != bb_p)
			{
				print_separator(&osep, buff, bufc);
			}

			XSAFELBSTR(fargs[i], buff, bufc);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Misc functions.
 */

void fun_strlen(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	XSAFELTOS(buff, bufc, strip_ansi_len(fargs[0]), LBUF_SIZE);
}

void fun_delete(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *s, *savep, *buf;
	int count, start, nchars;
	int ansi_state_l = ANST_NORMAL;
	int ansi_state_r = ANST_NORMAL;
	s = fargs[0];
	start = (int)strtol(fargs[1], (char **)NULL, 10);
	nchars = (int)strtol(fargs[2], (char **)NULL, 10);

	if ((nchars <= 0) || (start + nchars <= 0))
	{
		XSAFELBSTR(s, buff, bufc);
		return;
	}

	savep = s;

	for (count = 0; (count < start) && *s; ++count)
	{
		while (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state_l = (ansi_state_l & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}

		if (*s)
		{
			++s;
		}
	}

	XSAFESTRNCAT(buff, bufc, savep, s - savep, LBUF_SIZE);
	ansi_state_r = ansi_state_l;

	while (*s == ESC_CHAR)
	{
		do
		{
			int ansi_mask = 0;
			int ansi_diff = 0;
			unsigned int param_val = 0;
			++(s);
			if (*(s) == ANSI_CSI)
			{
				while ((*(++(s)) & 0xf0) == 0x30)
				{
					if (*(s) < 0x3a)
					{
						param_val <<= 1;
						param_val += (param_val << 2) + (*(s) & 0x0f);
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
			while ((*(s) & 0xf0) == 0x20)
			{
				++(s);
			}
			if (*(s) == ANSI_END)
			{
				if (param_val < I_ANSI_LIM)
				{
					ansi_mask |= ansiBitsMask(param_val);
					ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
				}
				ansi_state_r = (ansi_state_r & ~ansi_mask) | ansi_diff;
				++(s);
			}
			else if (*(s))
			{
				++(s);
			}
		} while (0);
	}

	for (; (count < start + nchars) && *s; ++count)
	{
		++s;

		while (*s == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s);
				if (*(s) == ANSI_CSI)
				{
					while ((*(++(s)) & 0xf0) == 0x30)
					{
						if (*(s) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s) & 0x0f);
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
				while ((*(s) & 0xf0) == 0x20)
				{
					++(s);
				}
				if (*(s) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state_r = (ansi_state_r & ~ansi_mask) | ansi_diff;
					++(s);
				}
				else if (*(s))
				{
					++(s);
				}
			} while (0);
		}
	}

	if (*s)
	{
		buf = ansi_transition_esccode(ansi_state_l, ansi_state_r);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
		XSAFELBSTR(s, buff, bufc);
	}
	else
	{
		buf = ansi_transition_esccode(ansi_state_l, ANST_NORMAL);
		XSAFELBSTR(buf, buff, bufc);
		XFREE(buf);
	}
}

/*
 * ---------------------------------------------------------------------------
 * Misc PennMUSH-derived functions.
 */

void fun_lit(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	/*
	 * Just returns the argument, literally
	 */
	XSAFELBSTR(fargs[0], buff, bufc);
}

void fun_art(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	/*
	 * checks a word and returns the appropriate article, "a" or "an"
	 */
	char *s = fargs[0];
	char c;

	while (*s && (isspace(*s) || iscntrl(*s)))
	{
		if (*s == ESC_CHAR)
		{
			skip_esccode(&s);
		}
		else
		{
			++s;
		}
	}

	c = tolower(*s);

	if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
	{
		XSAFESTRNCAT(buff, bufc, "an", 2, LBUF_SIZE);
	}
	else
	{
		XSAFELBCHR('a', buff, bufc);
	}
}

void fun_alphamax(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *amax;
	int i = 1;

	if (!fargs[0])
	{
		XSAFELBSTR("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}
	else
	{
		amax = fargs[0];
	}

	while ((i < nfargs) && fargs[i])
	{
		amax = (strcmp(amax, fargs[i]) > 0) ? amax : fargs[i];
		i++;
	}

	XSAFELBSTR(amax, buff, bufc);
}

void fun_alphamin(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	char *amin;
	int i = 1;

	if (!fargs[0])
	{
		XSAFELBSTR("#-1 TOO FEW ARGUMENTS", buff, bufc);
		return;
	}
	else
	{
		amin = fargs[0];
	}

	while ((i < nfargs) && fargs[i])
	{
		amin = (strcmp(amin, fargs[i]) < 0) ? amin : fargs[i];
		i++;
	}

	XSAFELBSTR(amin, buff, bufc);
}

void fun_valid(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	/*
	 * Checks to see if a given <something> is valid as a parameter of a
	 * given type (such as an object name).
	 */
	if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1])
	{
		XSAFELBCHR('0', buff, bufc);
	}
	else if (!strcasecmp(fargs[0], "name"))
	{
		XSAFEBOOL(buff, bufc, ok_name(fargs[1]));
	}
	else if (!strcasecmp(fargs[0], "attrname"))
	{
		XSAFEBOOL(buff, bufc, ok_attr_name(fargs[1]));
	}
	else if (!strcasecmp(fargs[0], "playername"))
	{
		XSAFEBOOL(buff, bufc, (ok_player_name(fargs[1]) && badname_check(fargs[1])));
	}
	else
	{
		XSAFENOTHING(buff, bufc);
	}
}

void fun_beep(char *buff, char **bufc, dbref player __attribute__((unused)), dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[] __attribute__((unused)), int nfargs __attribute__((unused)), char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
	XSAFELBCHR(BEEP_CHAR, buff, bufc);
}
