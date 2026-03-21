/**
 * @file eval.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Core expression evaluator for TinyMUSH
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * This file contains the central expression-evaluation engine:
 *
 *  - mundane_char_table_init() / mundane_char() – hot-path lookup table that
 *    identifies "ordinary" characters that can be bulk-copied without special
 *    processing.
 *
 *  - eval_expression_string() – the main interpreter loop that walks an input
 *    string, expands %-substitutions, executes [...] function calls, handles
 *    {...} literals, and recursively evaluates nested expressions.
 *
 * Supporting subsystems are implemented in sibling modules:
 *  - eval_parse.c  – parse_to(), parse_arglist()
 *  - eval_tcache.c – trace cache (_eval_tcache_*)
 *  - eval_gender.c – get_gender(), _eval_gender_emit_pronoun()
 *  - eval_regs.c   – save_global_regs(), restore_global_regs()
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

/* ---------------------------------------------------------------------------
 * Forward declarations for eval-internal helpers defined in sibling modules.
 * These functions are not part of the public API (not in prototypes.h) but
 * are shared across the eval_*.c family of translation units.
 * --------------------------------------------------------------------------- */

/* eval_tcache.c */
extern bool _eval_tcache_empty(void);
extern void _eval_tcache_add(char *orig, char *result);
extern void _eval_tcache_finish(dbref player);
extern int tcache_count; /* trace cache entry count (defined in eval_tcache.c) */

/* eval_gender.c */
extern void _eval_gender_emit_pronoun(char code, int *gender, dbref cause, char *buff, char **bufc);

/* ---------------------------------------------------------------------------
 * Mundane-character lookup table
 *
 * Inverted logic: 1 = mundane (not special), 0 = always special,
 * 2 = conditionally mundane (special only inside loop/switch context).
 * --------------------------------------------------------------------------- */

/**
 * @brief Lookup table for mundane character detection (hot path optimization).
 *
 * Values: 1 = mundane (not special), 0 = always special,
 *         2 = conditionally mundane (special in loop/switch context only).
 */
static unsigned char mundane_char_table[256];

/**
 * @brief Initialize the mundane character lookup table.
 *
 * Must be called during server initialization before processing any
 * expressions. All characters default to mundane (1), then special chars
 * are marked as 0 or 2.
 */
void mundane_char_table_init(void)
{
	/* Initialize all characters as mundane (1) */
	memset(mundane_char_table, 1, sizeof(mundane_char_table));

	/* Mark special characters as 0 (always special) */
	mundane_char_table[0x00] = 0; /* NUL */
	mundane_char_table[0x1b] = 0; /* ESC */
	mundane_char_table[' '] = 0;
	mundane_char_table['%'] = 0;
	mundane_char_table['('] = 0;
	mundane_char_table['['] = 0;
	mundane_char_table['\\'] = 0;
	mundane_char_table['{'] = 0;
	mundane_char_table['#'] = 2; /* Conditionally mundane (special in loop/switch) */
}

/**
 * @brief Check if a character is mundane (not special) – optimized with lookup table.
 *
 * Returns true for mundane chars, false for special chars (NUL, ESC, ' ', '%', '(',
 * '[', '\\', '{', and '#' in loop/switch context). Uses pre-computed lookup table
 * for O(1) performance.
 *
 * @param ch	Character to check
 * @return bool	true if mundane (not special), false if special
 */
static bool mundane_char(unsigned char ch)
{
	return mundane_char_table[ch] == 2 ? !(mushstate.in_loop || mushstate.in_switch) : mundane_char_table[ch];
}

/* ---------------------------------------------------------------------------
 * Core expression evaluator
 * --------------------------------------------------------------------------- */

/**
 * @brief Evaluate an expression string in place, expanding substitutions and functions.
 *
 * This is the core interpreter for TinyMUSH command lines. It walks the input string
 * referenced by `dstr`, copying plain text to `buff`, expanding %-codes, executing
 * functions inside brackets, and recursively evaluating nested expressions. The
 * cursor `*bufc` is advanced as characters are written. The input pointer `*dstr`
 * is moved forward as segments are consumed so callers can continue parsing after
 * a nested invocation.
 *
 * @param buff		Start of the destination buffer (LBUF-sized)
 * @param bufc		Pointer to the current write cursor inside `buff` (updated in place)
 * @param player	DBref of the executor (permissions and flags)
 * @param caller	DBref of the immediate caller in the call chain
 * @param cause		DBref of the enactor (used for pronouns, name, dbref output)
 * @param eval		Evaluation flags controlling compression, stripping, function checks, etc.
 * @param dstr		Pointer to the input string pointer; advanced past what was consumed
 * @param cargs		Command argument array for numeric %-subs (%0-%9)
 * @param ncargs	Number of entries in `cargs`
 */
void eval_expression_string(char *buff, char **bufc, dbref player, dbref caller, dbref cause, int eval, char **dstr, char *cargs[], int ncargs)
{
char *real_fargs[MAX_NFARGS + 1];
char **fargs = real_fargs + 1;
char *tstr = NULL, *tbuf = NULL, *savepos = NULL, *atr_gotten = NULL, *start = NULL, *oldp = NULL;
char *savestr = NULL, *str = NULL, *xptr = NULL, *mundane = NULL, *p = NULL, *xtp = NULL;
char savec = 0, ch = 0;
char *realbuff = NULL, *realbp = NULL;
char *xtbuf = XMALLOC(SBUF_SIZE, "xtbuf");
dbref aowner = NOTHING;
int at_space = 1, nfargs = 0, gender = -1, i = 0, j = 0, alldone = 0, aflags = 0, alen = 0, feval = 0;
int is_trace = Trace(player) && !(eval & EV_NOTRACE), is_top = 0, save_count = 0;
int ansi = 0, nchar = 0, navail = 0;
bool hilite_mode = false; /*!< Track hilite/bright mode for color codes */
FUN *fp = NULL;
UFUN *ufp = NULL;
VARENT *xvar = NULL;
ATTR *ap = NULL;
GDATA *preserve = NULL;

if (*dstr == NULL)
{
**bufc = '\0';
XFREE(xtbuf);
return;
}

/* Extend the buffer if we need to. */
if (((*bufc) - buff) > (LBUF_SIZE - SBUF_SIZE))
{
realbuff = buff;
realbp = *bufc;
buff = (char *)XMALLOC(LBUF_SIZE, "buff");
*bufc = buff;
}

oldp = start = *bufc;

/* If we are tracing, save a copy of the starting buffer */
savestr = NULL;

if (is_trace)
{
is_top = _eval_tcache_empty();
savestr = XMALLOC(LBUF_SIZE, "savestr");
XSTRCPY(savestr, *dstr);
}

while (**dstr && !alldone)
{
if (mundane_char((unsigned char)**dstr))
{
/* Mundane characters are the most common. There are usually a
   bunch in a row. We should just copy them. */
mundane = *dstr;
nchar = 0;

do
{
nchar++;
} while (mundane_char((unsigned char)*(++mundane)));

p = *bufc;
navail = LBUF_SIZE - 1 - (p - buff);
nchar = (nchar > navail) ? navail : nchar;
XMEMCPY(p, *dstr, nchar);
*bufc = p + nchar;
*dstr = mundane;
at_space = 0;
}

/* We must have a special character at this point. */
if (**dstr == '\0')
{
break;
}

switch (**dstr)
{
case ' ': /* A space. Add a space if not compressing or if previous char was not a space */
if (!(mushconf.space_compress && at_space) || (eval & EV_NO_COMPRESS))
{
XSAFELBCHR(' ', buff, bufc);
at_space = 1;
}

break;

case '\\': /* General escape. Add the following char without special processing */
at_space = 0;
(*dstr)++;

if (**dstr)
{
XSAFELBCHR(**dstr, buff, bufc);
}
else
{
(*dstr)--;
}

break;

case '[': /* Function start. Evaluate the contents of the square brackets as a function. */
at_space = 0;
tstr = (*dstr)++;

if (eval & EV_NOFCHECK)
{
XSAFELBCHR('[', buff, bufc);
*dstr = tstr;
break;
}

/* Parse to the matching closing ] */
tbuf = parse_to(dstr, ']', 0);

if (*dstr == NULL)
{
/* If no closing bracket, insert the [ and continue. */
XSAFELBCHR('[', buff, bufc);
*dstr = tstr;
}
else
{
/* We have a complete function. Parse out the function name and arguments. */
str = tbuf;
eval_expression_string(buff, bufc, player, caller, cause, (eval | EV_FCHECK | EV_FMAND), &str, cargs, ncargs);
(*dstr)--;
}

break;

case '{': /* Literal start. Insert everything up to the terminating } without parsing.
             If no closing brace, insert the { and continue. */
at_space = 0;
tstr = (*dstr)++;
tbuf = parse_to(dstr, '}', 0);

if (*dstr == NULL)
{
XSAFELBCHR('{', buff, bufc);
*dstr = tstr;
}
else
{
if (!(eval & EV_STRIP))
{
XSAFELBCHR('{', buff, bufc);
}

/* Preserve leading spaces (Felan) */
if (*tbuf == ' ')
{
XSAFELBCHR(' ', buff, bufc);
tbuf++;
}

str = tbuf;
eval_expression_string(buff, bufc, player, caller, cause, (eval & ~(EV_STRIP | EV_FCHECK)), &str, cargs, ncargs);

if (!(eval & EV_STRIP))
{
XSAFELBCHR('}', buff, bufc);
}

(*dstr)--;
}

break;

case '%': /* Percent-replace start. Evaluate the chars following and perform the appropriate substitution. */
at_space = 0;
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
int consumed = ansi_parse_single_x_code(dstr, &color, &hilite_mode);

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
ansi = (result == ColorStatusReset) ? 0 : 1;
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
_eval_gender_emit_pronoun(savec, &gender, cause, buff, bufc);
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

break;

case '(': /* Arglist start. See if what precedes is a function. If so, execute it if we should. */
at_space = 0;

if (!(eval & EV_FCHECK))
{
XSAFELBCHR('(', buff, bufc);
break;
}

/* Load an sbuf with an uppercase version of the func name, and see if the func exists.
   Trim trailing spaces from the name if configured. */
**bufc = '\0';
xtp = xtbuf;
XSAFESBSTR(oldp, xtbuf, &xtp);
*xtp = '\0';

if (mushconf.space_compress && (eval & EV_FMAND))
{
while ((--xtp >= xtbuf) && isspace(*xtp))
;

xtp++;
*xtp = '\0';
}

for (xtp = xtbuf; *xtp; xtp++)
{
*xtp = toupper(*xtp);
}

fp = (FUN *)hashfind(xtbuf, &mushstate.func_htab);
/* If not a builtin func, check for global function */
ufp = NULL;

if (fp == NULL)
{
ufp = (UFUN *)hashfind(xtbuf, &mushstate.ufunc_htab);
}

/* Do the right thing if it doesn't exist */
if (!fp && !ufp)
{
if (eval & EV_FMAND)
{
*bufc = oldp;
XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (%s) NOT FOUND", xtbuf);
}
else
{
/* Preserve the parenthesis verbatim */
XSAFELBCHR('(', buff, bufc);
}

eval &= ~EV_FCHECK;
break;
}

/* Get the arglist and count the number of args. Negative # of args means join subsequent args. */
if (ufp)
{
nfargs = MAX_NFARGS;
}
else if (fp->nargs < 0)
{
nfargs = -fp->nargs;
}
else
{
nfargs = MAX_NFARGS;
}

tstr = *dstr;

if ((fp && (fp->flags & FN_NO_EVAL)) || (ufp && (ufp->flags & FN_NO_EVAL)))
{
feval = (eval & ~EV_EVAL) | EV_STRIP_ESC;
}
else
{
feval = eval;
}

*dstr = parse_arglist(player, caller, cause, *dstr + 1, ')', feval, fargs, nfargs, cargs, ncargs);

/* If no closing delim, just insert the '(' and continue normally */
if (!*dstr)
{
*dstr = tstr;
XSAFELBCHR(**dstr, buff, bufc);

for (i = 0; i < nfargs; i++)
if (fargs[i] != NULL)
{
XFREE(fargs[i]);
}

eval &= ~EV_FCHECK;
break;
}

/* Count number of args returned */
(*dstr)--;
j = 0;

for (i = 0; i < nfargs; i++)
if (fargs[i] != NULL)
{
j = i + 1;
}

nfargs = j;
/* We've got function(args) now, so back up over function name in output buffer. */
*bufc = oldp;

/* If it's a user-defined function, perform it now. */
if (ufp)
{
mushstate.func_nest_lev++;
mushstate.func_invk_ctr++;

if (mushstate.func_nest_lev >= mushconf.func_nest_lim)
{
XSAFELBSTR("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
}
else if (mushstate.func_invk_ctr >= mushconf.func_invk_lim)
{
XSAFELBSTR("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
}
else if (Too_Much_CPU())
{
XSAFELBSTR("#-1 FUNCTION CPU LIMIT EXCEEDED", buff, bufc);
}
else if (Going(player))
{
XSAFELBSTR("#-1 BAD INVOKER", buff, bufc);
}
else if (!check_access(player, ufp->perms))
{
XSAFENOPERM(buff, bufc);
}
else
{
tstr = atr_get(ufp->obj, ufp->atr, &aowner, &aflags, &alen);

if (ufp->flags & FN_PRIV)
{
i = ufp->obj;
}
else
{
i = player;
}

str = tstr;

if (ufp->flags & FN_NOREGS)
{
preserve = mushstate.rdata;
mushstate.rdata = NULL;
}
else if (ufp->flags & FN_PRES)
{
preserve = save_global_regs("eval.save");
}

eval_expression_string(buff, bufc, i, player, cause, ((ufp->flags & FN_NO_EVAL) ? (EV_FCHECK | EV_EVAL) : feval), &str, fargs, nfargs);

if (ufp->flags & FN_NOREGS)
{
if (mushstate.rdata)
{
for (int z = 0; z < mushstate.rdata->q_alloc; z++)
{
if (mushstate.rdata->q_regs[z])
XFREE(mushstate.rdata->q_regs[z]);
}
for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
{
if (mushstate.rdata->x_names[z])
XFREE(mushstate.rdata->x_names[z]);
if (mushstate.rdata->x_regs[z])
XFREE(mushstate.rdata->x_regs[z]);
}

if (mushstate.rdata->q_regs)
{
XFREE(mushstate.rdata->q_regs);
}
if (mushstate.rdata->q_lens)
{
XFREE(mushstate.rdata->q_lens);
}
if (mushstate.rdata->x_names)
{
XFREE(mushstate.rdata->x_names);
}
if (mushstate.rdata->x_regs)
{
XFREE(mushstate.rdata->x_regs);
}
if (mushstate.rdata->x_lens)
{
XFREE(mushstate.rdata->x_lens);
}
XFREE(mushstate.rdata);
}

mushstate.rdata = preserve;
}
else if (ufp->flags & FN_PRES)
{
restore_global_regs("eval.restore", preserve);
}

XFREE(tstr);
}

/* Return the space allocated for the args */
mushstate.func_nest_lev--;

for (i = 0; i < nfargs; i++)
if (fargs[i] != NULL)
{
XFREE(fargs[i]);
}

eval &= ~EV_FCHECK;
break;
}

/* If the number of args is right, perform the func. Otherwise return an error message.
   Note that parse_arglist returns zero args as one null arg, so we have to handle that case specially. */
if ((fp->nargs == 0) && (nfargs == 1))
{
if (!*fargs[0])
{
XFREE(fargs[0]);
fargs[0] = NULL;
nfargs = 0;
}
}

if ((nfargs == fp->nargs) || (nfargs == -fp->nargs) || (fp->flags & FN_VARARGS))
{
/* Check recursion limit */
mushstate.func_nest_lev++;
mushstate.func_invk_ctr++;

if (mushstate.func_nest_lev >= mushconf.func_nest_lim)
{
XSAFELBSTR("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
}
else if (mushstate.func_invk_ctr >= mushconf.func_invk_lim)
{
XSAFELBSTR("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
}
else if (Too_Much_CPU())
{
XSAFELBSTR("#-1 FUNCTION CPU LIMIT EXCEEDED", buff, bufc);
}
else if (Going(player))
{
/* Deal with the peculiar case of the calling object being destroyed
   mid-function sequence, such as with a command()/@destroy combo... */
XSAFELBSTR("#-1 BAD INVOKER", buff, bufc);
}
else if (!Check_Func_Access(player, fp))
{
XSAFENOPERM(buff, bufc);
}
else if (mushstate.f_limitmask & fp->flags)
{
XSAFENOPERM(buff, bufc);
}
else
{
fargs[-1] = (char *)fp;
fp->fun(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs);
}

mushstate.func_nest_lev--;
}
else
{
XSAFESPRINTF(buff, bufc, "#-1 FUNCTION (%s) EXPECTS %d ARGUMENTS BUT GOT %d", fp->name, fp->nargs, nfargs);
}

/* Return the space allocated for the arguments */
for (i = 0; i < nfargs; i++)
{
XFREE(fargs[i]);
}

eval &= ~EV_FCHECK;
break;

case '#':
/* We should never reach this point unless we're in a loop or switch, thanks to the table lookup. */
at_space = 0;
(*dstr)++;

if (!strchr("!#$+@", **dstr))
{
(*dstr)--;
XSAFELBCHR(**dstr, buff, bufc);
}
else
{
if ((**dstr == '#') && mushstate.in_loop)
{
XSAFELBSTR(mushstate.loop_token[mushstate.in_loop - 1], buff, bufc);
}
else if ((**dstr == '@') && mushstate.in_loop)
{
XSAFELTOS(buff, bufc, mushstate.loop_number[mushstate.in_loop - 1], LBUF_SIZE);
}
else if ((**dstr == '+') && mushstate.in_loop)
{
XSAFELBSTR(mushstate.loop_token2[mushstate.in_loop - 1], buff, bufc);
}
else if ((**dstr == '$') && mushstate.in_switch)
{
XSAFELBSTR(mushstate.switch_token, buff, bufc);
}
else if (**dstr == '!')
{
/* Nesting level of loop takes precedence over switch nesting level. */
XSAFELTOS(buff, bufc, ((mushstate.in_loop) ? (mushstate.in_loop - 1) : mushstate.in_switch), LBUF_SIZE);
}
else
{
(*dstr)--;
XSAFELBCHR(**dstr, buff, bufc);
}
}

break;

case C_ANSI_ESC:
{
char *escape_start = *dstr;
skip_esccode(dstr);
for (char *seq = escape_start; seq < *dstr; ++seq)
{
XSAFELBCHR(*seq, buff, bufc);
}
(*dstr)--;
break;
}
}

(*dstr)++;
}

/* If we're eating spaces, and the last thing was a space, eat it up. Complicated by the fact
   that at_space is initially true. So check to see if we actually put something in the buffer, too. */
if (mushconf.space_compress && at_space && !(eval & EV_NO_COMPRESS) && (start != *bufc))
{
(*bufc)--;
}

/* The ansi() function knows how to take care of itself. However, if the player used a %x sub
   in the string, and hasn't yet terminated the color with a %xn yet, we'll have to do it for them. */
if (ansi)
{
xsafe_ansi_normal(buff, bufc);
}

**bufc = '\0';

/* Report trace information */
if (is_trace)
{
_eval_tcache_add(savestr, start);
save_count = tcache_count - mushconf.trace_limit;

if (is_top || !mushconf.trace_topdown)
{
_eval_tcache_finish(player);
}

if (is_top && (save_count > 0))
{
tbuf = XMALLOC(MBUF_SIZE, "tbuff");
XSPRINTF(tbuf, "%d lines of trace output discarded.", save_count);
notify(player, tbuf);
XFREE(tbuf);
}
}

if (realbuff)
{
*bufc = realbp;
XSAFELBSTR(buff, realbuff, bufc);
**bufc = '\0';
XFREE(buff);
buff = realbuff;
}
XFREE(xtbuf);
}
