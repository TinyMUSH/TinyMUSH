/**
 * @file eval.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Command evaluation and cracking
 * @version 3.3
 * @date 2020-12-28
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"
#include "game.h"
#include "alloc.h"
#include "flags.h"
#include "htab.h"
#include "ltdl.h"
#include "udb.h"
#include "udb_defs.h"
#include "mushconf.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "powers.h"
#include "attrs.h"
#include "functions.h"
#include "stringutil.h"

/**
 * @brief Helper for parse_to
 * 
 * @param eval		Evaluation flags
 * @param first		Is this the first pass?
 * @param cstr		String buffer
 * @param rstr		String buffer
 * @param zstr		String buffer
 * @return char* 
 */
char *parse_to_cleanup(int eval, bool first, char *cstr, char *rstr, char *zstr)
{
	if ((mudconf.space_compress || (eval & EV_STRIP_TS)) && !(eval & EV_NO_COMPRESS) && !first && (cstr[-1] == ' '))
	{
		zstr--;
	}

	if ((eval & EV_STRIP_AROUND) && (*rstr == '{') && (zstr[-1] == '}'))
	{
		rstr++;

		if ((mudconf.space_compress && !(eval & EV_NO_COMPRESS)) || (eval & EV_STRIP_LS))
			while (*rstr && isspace(*rstr))
			{
				rstr++;
			}

		rstr[-1] = '\0';
		zstr--;

		if ((mudconf.space_compress && !(eval & EV_NO_COMPRESS)) || (eval & EV_STRIP_TS))
			while (zstr[-1] && isspace(zstr[-1]))
			{
				zstr--;
			}

		*zstr = '\0';
	}

	*zstr = '\0';
	return rstr;
}

/**
 * We can't change this to just '*zstr++ = *cstr++', because of the inherent
 * problems with copying a memory location to itself.
 * 
 */
#define NEXTCHAR      \
	if (cstr == zstr) \
	{                 \
		cstr++;       \
		zstr++;       \
	}                 \
	else              \
		*zstr++ = *cstr++

/**
 * @brief Split a line at a character, obeying nesting.
 * 
 * The line is destructively modified (a null is inserted where the delimiter
 * was found) dstr is modified to point to the char after the delimiter, and
 * the function return value points to the found string (space compressed if
 * specified). If we ran off the end of the string without finding the 
 * delimiter, dstr is returned as NULL.
 * 
 * @param dstr	String to parse
 * @param delim	Delimiter
 * @param eval	Evaluation flags
 * @return char* 
 */
char *parse_to(char **dstr, char delim, int eval)
{
	char *stack = XMALLOC(32, "stack");
	char *rstr, *cstr, *zstr;
	int sp, tp, bracketlev;
	bool first = true;

	if ((dstr == NULL) || (*dstr == NULL))
	{
		XFREE(stack);
		return NULL;
	}

	if (**dstr == '\0')
	{
		rstr = *dstr;
		*dstr = NULL;
		XFREE(stack);
		return rstr;
	}

	sp = 0;
	rstr = *dstr;

	if ((mudconf.space_compress || (eval & EV_STRIP_LS)) && !(eval & EV_NO_COMPRESS))
	{
		while (*rstr && isspace(*rstr))
		{
			rstr++;
		}

		*dstr = rstr;
	}

	zstr = cstr = rstr;

	while (*cstr)
	{
		switch (*cstr)
		{
		case '\\': /** general escape */
		case '%':  /** also escapes chars */
			if ((*cstr == '\\') && (eval & EV_STRIP_ESC))
			{
				cstr++;
			}
			else
			{
				NEXTCHAR;
			}

			if (*cstr)
			{
				NEXTCHAR;
			}

			first = false;
			break;

		case ']':
		case ')':
			for (tp = sp - 1; (tp >= 0) && (stack[tp] != *cstr); tp--)
				;

			/**
		     * If we hit something on the stack, unwind to it Otherwise (it's
			 * not on stack), if it's our delim we are done, and we convert the
			 * delim to a null and return a ptr to the char after the null. If
		     * it's not our delimiter, skip over it normally
			 * 
	    	 */
			if (tp >= 0)
			{
				sp = tp;
			}
			else if (*cstr == delim)
			{
				rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
				*dstr = ++cstr;
				XFREE(stack);
				return rstr;
			}

			first = false;
			NEXTCHAR;
			break;

		case '{':
			bracketlev = 1;

			if (eval & EV_STRIP)
			{
				cstr++;
			}
			else
			{
				NEXTCHAR;
			}

			while (*cstr && (bracketlev > 0))
			{
				switch (*cstr)
				{
				case '\\':
				case '%':
					if (cstr[1])
					{
						if ((*cstr == '\\') && (eval & EV_STRIP_ESC))
						{
							cstr++;
						}
						else
						{
							NEXTCHAR;
						}
					}

					break;

				case '{':
					bracketlev++;
					break;

				case '}':
					bracketlev--;
					break;
				}

				if (bracketlev > 0)
				{
					NEXTCHAR;
				}
			}

			if ((eval & EV_STRIP) && (bracketlev == 0))
			{
				cstr++;
			}
			else if (bracketlev == 0)
			{
				NEXTCHAR;
			}

			first = false;
			break;

		default:
			if ((*cstr == delim) && (sp == 0))
			{
				rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
				*dstr = ++cstr;
				XFREE(stack);
				return rstr;
			}

			switch (*cstr)
			{
			case ' ': /** space */
				if (mudconf.space_compress && !(eval & EV_NO_COMPRESS))
				{
					if (first)
					{
						rstr++;
					}
					else if (cstr[-1] == ' ')
					{
						zstr--;
					}
				}

				NEXTCHAR;
				break;

			case '[':
				if (sp < 32)
				{
					stack[sp++] = ']';
				}

				NEXTCHAR;
				first = false;
				break;

			case '(':
				if (sp < 32)
				{
					stack[sp++] = ')';
				}

				NEXTCHAR;
				first = false;
				break;

			case ESC_CHAR:
				NEXTCHAR;

				if (*cstr == ANSI_CSI)
				{
					do
					{
						NEXTCHAR;
					} while ((*cstr & 0xf0) == 0x30);
				}

				while ((*cstr & 0xf0) == 0x20)
				{
					NEXTCHAR;
				}

				if (*cstr)
				{
					NEXTCHAR;
				}

				first = false;
				break;

			default:
				first = false;
				NEXTCHAR;
				break;
			}
		}
	}

	rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
	*dstr = NULL;
	XFREE(stack);
	return rstr;
}

/**
 * @brief Parse a line into an argument list contained in lbufs.
 * 
 * A pointer is returned to whatever follows the final delimiter. If the
 * arglist is unterminated, a NULL is returned.  The original arglist is
 * destructively modified.
 * 
 * @param player	DBref of player
 * @param caller	DBref of caller
 * @param cause		DBref of cause
 * @param dstr		Destination buffer
 * @param delim		Delimiter
 * @param eval		DBref of victim
 * @param fargs		Function arguments
 * @param nfargs	Number of function arguments
 * @param cargs		Command arguments
 * @param ncargs	Niimber of command arguments
 * @return char* 
 */
char *parse_arglist(dbref player, dbref caller, dbref cause, char *dstr, char delim, dbref eval, char *fargs[], dbref nfargs, char *cargs[], dbref ncargs)
{
	char *rstr, *tstr, *bp, *str;
	int arg, peval;

	for (arg = 0; arg < nfargs; arg++)
	{
		fargs[arg] = NULL;
	}

	if (dstr == NULL)
	{
		return NULL;
	}

	rstr = parse_to(&dstr, delim, 0);
	arg = 0;
	peval = (eval & ~EV_EVAL);

	while ((arg < nfargs) && rstr)
	{
		if (arg < (nfargs - 1))
		{
			tstr = parse_to(&rstr, ',', peval);
		}
		else
		{
			tstr = parse_to(&rstr, '\0', peval);
		}

		if (eval & EV_EVAL)
		{
			bp = fargs[arg] = XMALLOC(LBUF_SIZE, "fargs[arg]");
			str = tstr;
			exec(fargs[arg], &bp, player, caller, cause, eval | EV_FCHECK, &str, cargs, ncargs);
		}
		else
		{
			fargs[arg] = XMALLOC(LBUF_SIZE, "fargs[arg]");
			XSTRCPY(fargs[arg], tstr);
		}

		arg++;
	}

	return dstr;
}

/**
 * @brief Process a command line, evaluating function calls and %-substitutions.
 * 
 * @param player	DBref of player
 * @return int 
 */
int get_gender(dbref player)
{
	char first, *atr_gotten;
	dbref aowner;
	int aflags, alen;
	atr_gotten = atr_pget(player, A_SEX, &aowner, &aflags, &alen);
	first = *atr_gotten;
	XFREE(atr_gotten);

	switch (first)
	{
	case 'P':
	case 'p':
		return 4;

	case 'M':
	case 'm':
		return 3;

	case 'F':
	case 'f':
	case 'W':
	case 'w':
		return 2;

	default:
		return 1;
	}
}

typedef struct tcache_ent TCENT;

struct tcache_ent
{
	char *orig;
	char *result;
	struct tcache_ent *next;
} * tcache_head;

int tcache_top, tcache_count;

/**
 * @brief Initialize trace cache
 * 
 */
void tcache_init(void)
{
	tcache_head = NULL;
	tcache_top = 1;
	tcache_count = 0;
}

/**
 * @brief Empty trace cache
 * 
 * @return int 
 */
int tcache_empty(void)
{
	if (tcache_top)
	{
		tcache_top = 0;
		tcache_count = 0;
		return 1;
	}

	return 0;
}

/**
 * @brief Add to the trace cache
 * 
 * @param orig		Origin buffer
 * @param result	Result buffer
 */
void tcache_add(char *orig, char *result)
{
	char *tp;
	TCENT *xp;

	if (strcmp(orig, result))
	{
		tcache_count++;

		if (tcache_count <= mudconf.trace_limit)
		{
			xp = (TCENT *)XMALLOC(SBUF_SIZE, "xp");
			tp = XMALLOC(LBUF_SIZE, "tp");
			XSTRCPY(tp, result);
			xp->orig = orig;
			xp->result = tp;
			xp->next = tcache_head;
			tcache_head = xp;
		}
		else
		{
			XFREE(orig);
		}
	}
	else
	{
		XFREE(orig);
	}
}

/**
 * @brief Terminate trace cache
 * 
 * @param player	DBref of player
 */
void tcache_finish(dbref player)
{
	TCENT *xp;
	NUMBERTAB *np;
	dbref target;

	if (H_Redirect(player))
	{
		np = (NUMBERTAB *)nhashfind(player, &mudstate.redir_htab);

		if (np)
		{
			target = np->num;
		}
		else
		{
			/**
		     * Ick. If we have no pointer, we should have no
		     * flag.
	    	 */
			s_Flags3(player, Flags3(player) & ~HAS_REDIRECT);
			target = Owner(player);
		}
	}
	else
	{
		target = Owner(player);
	}

	while (tcache_head != NULL)
	{
		xp = tcache_head;
		tcache_head = xp->next;
		notify_check(target, target, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s(#%d)} '%s' -> '%s'", Name(player), player, xp->orig, xp->result);
		XFREE(xp->orig);
		XFREE(xp->result);
		XFREE(xp);
	}

	tcache_top = 1;
	tcache_count = 0;
}

/**
 * ASCII table for reference
 * 
 *		0	1	2	3	4	5	6	7	8	9	A	B	C	D	E	F   
 * 0	NUL	SOH	STX	ETX	EOT	ENQ	ACK	BEL	BS	TAB	LF	VT	FF	CR	SO	SI
 * 1	DLE	DC1	DC2	DC3	DC4	NAK	SYN	ETB	CAN	EM	SUB	ESC	FS	GS	RS	US
 * 2	SPC	!	"	#	$	%	&	'	(	)	*	+	,	-	.	/
 * 3	0	1	2	3	4	5	6	7	8	9	:	;	<	=	>	?
 * 4	@	A	B	C	D	E	F	G	H	I	J	K	L	M	N	O
 * 5	P	Q	R	S	T	U	V	W	X	Y	Z	[	\	]	^	_
 * 6	`	a	b	c	d	e	f	g	h	i	j	k	l	m	n	o
 * 7	p	q	r	s	t	u	v	w	x	y	z	{	|	}	~	DEL
 * 
 */

/**
 * @brief Check if character is a special char.
 * 
 * We want '', ESC, ' ', '\', '[', '{', '(', '%', and '#'.
 * 
 * @param ch	Character to check
 * @return bool	True if special, false if not.
 */
bool special_char(unsigned char ch)
{
	switch (ch)
	{
	case 0x00: // 'NUL'
	case 0x1b: // 'ESC'
	case ' ':
	case '%':
	case '(':
	case '[':
	case '\\':
	case '{':
		return true;
	case 0x23: // ' # '
		/**
		 * We adjust the return value in order to avoid always treating '#' 
		 * like a special character, as it gets used a whole heck of a lot.
		 * 
		 */
		return (mudstate.in_loop || mudstate.in_switch) ? 1 : 0;
	}
	return false;
}

/**
 * @brief Check if a character is a token.
 * 
 * * We want '!', '#', '$', '+', 'A'.
 * 
 * @param ch	Character to check
 * @return char 1 if true, 0 if false
 */
bool token_char(int ch)
{
	switch (ch)
	{
	case 0x21: // ' ! '
	case 0x23: // ' # '
	case 0x24: // ' $ '
	case 0x2B: // ' + '
	case 0x40: // ' A '
		return true;
		break;
	}
	return false;
}

/**
 * @brief Get subject pronoun
 * 
 * @param subj	id of the pronoun
 * @return char* pronoun
 */
char *get_subj(int subj)
{
	switch (subj)
	{
	case 1:
		return "it";
	case 2:
		return "she";
	case 3:
		return "he";
	case 4:
		return "they";
	}
	return STRING_EMPTY;
}

/**
 * @brief Get Possessive Adjective pronoun
 * 
 * @param subj	id of the pronoun
 * @return char*
 */
char *get_poss(int poss)
{
	switch (poss)
	{
	case 1:
		return "its";
	case 2:
		return "her";
	case 3:
		return "his";
	case 4:
		return "their";
	}
	return STRING_EMPTY;
}

/**
 * @brief Get Object pronoun
 * 
 * @param subj	id of the pronoun
 * @return char*
 */
char *get_obj(int obj)
{
	switch (obj)
	{
	case 1:
		return "it";
	case 2:
		return "her";
	case 3:
		return "him";
	case 4:
		return "them";
	}
	return STRING_EMPTY;
}

/**
 * @brief Get Absolute Possessive pronoun
 * 
 * @param subj	id of the pronoun
 * @return char*
 */
char *get_absp(int absp)
{
	switch (absp)
	{
	case 1:
		return "its";
	case 2:
		return "hers";
	case 3:
		return "his";
	case 4:
		return "theirs";
	}
	return STRING_EMPTY;
}

/**
 * @brief Execute commands
 * 
 * @param buff		Output buffer
 * @param bufc		Output buffer tracker
 * @param player	DBref of player
 * @param caller	DBref of caller
 * @param cause		DBref of cause
 * @param eval		Evaluation flags
 * @param dstr		Destination Buffer
 * @param cargs		Command arguments
 * @param ncargs	Number of command arguments
 */
void exec(char *buff, char **bufc, dbref player, dbref caller, dbref cause, int eval, char **dstr, char *cargs[], int ncargs)
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


	/**
     * Extend the buffer if we need to.
	 * 
     */
	if (((*bufc) - buff) > (LBUF_SIZE - SBUF_SIZE))
	{
		realbuff = buff;
		realbp = *bufc;
		buff = (char *)XMALLOC(LBUF_SIZE, "buff");
		*bufc = buff;
	}

	oldp = start = *bufc;

	/**
     * If we are tracing, save a copy of the starting buffer
	 * 
     */
	savestr = NULL;

	if (is_trace)
	{
		is_top = tcache_empty();
		savestr = XMALLOC(LBUF_SIZE, "savestr");
		XSTRCPY(savestr, *dstr);
	}

	while (**dstr && !alldone)
	{
		if (!special_char((unsigned char)**dstr))
		{
			/**
		     * Mundane characters are the most common. There are
		     * usually a bunch in a row. We should just copy
		     * them.
			 * 
	    	 */
			mundane = *dstr;
			nchar = 0;

			do
			{
				nchar++;
			} while (!special_char((unsigned char)*(++mundane)));

			p = *bufc;
			navail = LBUF_SIZE - 1 - (p - buff);
			nchar = (nchar > navail) ? navail : nchar;
			XMEMCPY(p, *dstr, nchar);
			*bufc = p + nchar;
			*dstr = mundane;
			at_space = 0;
		}

		/**
		 * We must have a special character at this point.
		 * 
		 */
		if (**dstr == '\0')
		{
			break;
		}

		switch (**dstr)
		{
		case ' ':
			/**
		     * A space.  Add a space if not compressing or if
		     * previous char was not a space
			 * 
		     */
			if (!(mudconf.space_compress && at_space) || (eval & EV_NO_COMPRESS))
			{
				SAFE_LB_CHR(' ', buff, bufc);
				at_space = 1;
			}

			break;

		case '\\':
			/**
		     * General escape.  Add the following char without
		     * special processing
			 * 
		     */
			at_space = 0;
			(*dstr)++;

			if (**dstr)
			{
				SAFE_LB_CHR(**dstr, buff, bufc);
			}
			else
			{
				(*dstr)--;
			}

			break;

		case '[':
			/**
		     * Function start.  Evaluate the contents of the
		     * square brackets as a function.  If no closing
	    	 * bracket, insert the [ and continue.
			 * 
		     */
			at_space = 0;
			tstr = (*dstr)++;

			if (eval & EV_NOFCHECK)
			{
				SAFE_LB_CHR('[', buff, bufc);
				*dstr = tstr;
				break;
			}

			tbuf = parse_to(dstr, ']', 0);

			if (*dstr == NULL)
			{
				SAFE_LB_CHR('[', buff, bufc);
				*dstr = tstr;
			}
			else
			{
				str = tbuf;
				exec(buff, bufc, player, caller, cause, (eval | EV_FCHECK | EV_FMAND), &str, cargs, ncargs);
				(*dstr)--;
			}

			break;

		case '{':
			/**
		     * Literal start.  Insert everything up to the
		     * terminating } without parsing.  If no closing
	    	 * brace, insert the { and continue.
			 * 
		     */
			at_space = 0;
			tstr = (*dstr)++;
			tbuf = parse_to(dstr, '}', 0);

			if (*dstr == NULL)
			{
				SAFE_LB_CHR('{', buff, bufc);
				*dstr = tstr;
			}
			else
			{
				if (!(eval & EV_STRIP))
				{
					SAFE_LB_CHR('{', buff, bufc);
				}

				/**
				 * Preserve leading spaces (Felan)
				 */
				if (*tbuf == ' ')
				{
					SAFE_LB_CHR(' ', buff, bufc);
					tbuf++;
				}

				str = tbuf;
				exec(buff, bufc, player, caller, cause, (eval & ~(EV_STRIP | EV_FCHECK)), &str, cargs, ncargs);

				if (!(eval & EV_STRIP))
				{
					SAFE_LB_CHR('}', buff, bufc);
				}

				(*dstr)--;
			}

			break;

		case '%':
			/**
		     * Percent-replace start.  Evaluate the chars
		     * following and perform the appropriate
	    	 * substitution.
			 * 
		     */
			at_space = 0;
			(*dstr)++;
			savec = **dstr;
			savepos = *bufc;

			switch (savec)
			{
			case '\0':
				/** 
				 * Null - all done 
				 * 
				 */
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
			case '9':
				/**
				 * Command argument number N
				 * 
				 */
				i = (**dstr - '0');

				if ((i < ncargs) && (cargs[i] != NULL))
				{
					SAFE_LB_STR(cargs[i], buff, bufc);
				}

				break;

			case 'r':
			case 'R':
				/** 
				 * Carriage return
				 * 
				 */
				SAFE_CRLF(buff, bufc);
				break;

			case 't':
			case 'T':
				/** 
				 * Tab 
				 * 
				 */
				SAFE_LB_CHR('\t', buff, bufc);
				break;

			case 'B':
			case 'b':
				/** 
				 * Blank
				 * 
				 */
				SAFE_LB_CHR(' ', buff, bufc);
				break;

			case 'C':
			case 'c':
				if (mudconf.c_cmd_subst)
				{
					/**
					 * %c is last command executed
					 * 
					 */
					SAFE_LB_STR(mudstate.curr_cmd, buff, bufc);
					break;
				}
				//[[fallthrough]];
				__attribute__((fallthrough));
			/**
			 * %c is color
			 * 
			 */
			case 'x':
			case 'X':
				/** 
				 * ANSI color
				 * 
				 */
				(*dstr)++;

				if (!**dstr)
				{
					/** 
					 * @note There is an interesting bug/misfeature in the
					 * implementation of %v? and %q? -- if the second character
					 * is garbage or non-existent, it and the leading v or q
					 * gets eaten. In the interests of not changing the old
					 * behavior, this is not getting "fixed", but in this case,
					 * where moving the pointer back without exiting on an 
					 * error condition ends up turning things black, the
					 * behavior must by necessity be different. So we do break
					 * out of the switch.
					 * 
				     */
					(*dstr)--;
					break;
				}

				if (!mudconf.ansi_colors)
				{
					/**
				     * just skip over the characters
				     */
					break;
				}

				if (**dstr == '<' || **dstr == '/')
				{ 
					/**
					 * Xterm colors
					 * 
					 * @todo Fix bugs and streamline implementation
					 * 
					 */

					int xterm_isbg = 0;

					while (1)
					{
						if (**dstr == '/')
						{ 
							/** 
							 * We are dealing with background 
							 * 
							 */
							xptr = *dstr;
							(*dstr)++;

							if (!**dstr)
							{
								*dstr = xptr;
								break;
							}
							else
							{
								xterm_isbg = 1;
							}
						}
						else
						{
							/** 
							 * We are dealing with foreground
							 * 
							 */
							xterm_isbg = 0; 
						}

						if (**dstr == '<')
						{ 
							/** 
							 * Ok we got a color to process
							 * 
							 */
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
								SAFE_SB_CHR(**dstr, xtbuf, &xtp);
								(*dstr)++;
							}

							if (**dstr != '>')
							{
								/**
								 * Ran off the end. Back up.
								 * 
								 */
								*dstr = xptr;
								break;
							}

							*xtp = '\0';
							/** 
							 * Now we have the color string... Time to handle it
							 * 
							 */
							i = str2xterm(xtbuf);

							if (xterm_isbg)
							{
								XSNPRINTF(xtbuf, SBUF_SIZE, "%s%d%c", ANSI_XTERM_BG, i, ANSI_END);
							}
							else
							{
								XSNPRINTF(xtbuf, SBUF_SIZE, "%s%d%c", ANSI_XTERM_FG, i, ANSI_END);
							}

							SAFE_LB_STR(xtbuf, buff, bufc);
							ansi = 1;
						}
						else
						{
							break;
						}

						/** 
						 * Shall we continue?
						 * 
						 */
						xptr = *dstr;
						(*dstr)++;

						if (**dstr != '<' && **dstr != '/')
						{
							*dstr = xptr;
							break;
						}
					}

					break;
				}

				if (!ansiChar((unsigned char)**dstr))
				{
					SAFE_LB_CHR(**dstr, buff, bufc);
				}
				else
				{
					SAFE_LB_STR(ansiChar((unsigned char)**dstr), buff, bufc);
					ansi = (**dstr == 'n') ? 0 : 1;
				}

				break;

			case '=': 
				/** 
				 * Equivalent of generic v() attr get
				 * 
				 */
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
					SAFE_SB_CHR(**dstr, xtbuf, &xtp);
					(*dstr)++;
				}

				if (**dstr != '>')
				{
					/** 
					 * Ran off the end. Back up.
					 * 
					 */
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
					SAFE_STRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
					XFREE(atr_gotten);
				}

				break;

			case '_':
				/** 
				 * x-variable
				 * 
				 */
				(*dstr)++;

				/**
				 * Check for %_<varname>
				 * 
				 */
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
					SAFE_LTOS(xtbuf, &xtp, player, LBUF_SIZE);
					SAFE_COPY_CHR('.', xtbuf, &xtp, SBUF_SIZE - 1);
					SAFE_COPY_CHR(ch, xtbuf, &xtp, SBUF_SIZE - 1);
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
					SAFE_LTOS(xtbuf, &xtp, player, LBUF_SIZE);
					SAFE_COPY_CHR('.', xtbuf, &xtp, SBUF_SIZE - 1);

					while (**dstr && (**dstr != '>'))
					{
						/**
						 * Copy. No interpretation.
						 * 
						 */
						ch = tolower(**dstr);
						SAFE_SB_CHR(ch, xtbuf, &xtp);
						(*dstr)++;
					}

					if (**dstr != '>')
					{
						/**
						 * We ran off the end of the string without finding a
						 * termination condition. Go back.
						 */
						*dstr = xptr;
						break;
					}
				}

				*xtp = '\0';

				if (!(mudstate.f_limitmask & FN_VARFX) && (xvar = (VARENT *)hashfind(xtbuf, &mudstate.vars_htab)))
				{
					SAFE_LB_STR(xvar->text, buff, bufc);
				}

				break;

			case 'V':
				/** 
				 * Variable attribute
				 * 
				 */
			case 'v':
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
				SAFE_STRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
				XFREE(atr_gotten);
				break;

			case 'Q':
				/** 
				 * Local registers
				 * 
				 */
			case 'q':
				(*dstr)++;

				if (!**dstr)
				{
					(*dstr)--;
					break;
				}

				if (**dstr != '<')
				{
					i = qidx_chartab((unsigned char)**dstr);

					if ((i < 0) || (i >= mudconf.max_global_regs))
					{
						break;
					}

					if (mudstate.rdata && mudstate.rdata->q_alloc > i)
					{
						char *qreg = mudstate.rdata->q_regs[i];
						size_t qlen = mudstate.rdata->q_lens[i];
						SAFE_STRNCAT(buff, bufc, qreg, qlen, LBUF_SIZE);
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

				if (!mudstate.rdata || !mudstate.rdata->xr_alloc)
				{
					/**
					 * We know there's no result, so we just advance past.
					 * 
					 */
					while (**dstr && (**dstr != '>'))
					{
						(*dstr)++;
					}

					if (**dstr != '>')
					{
						/**
						 * Whoops, no end. Go back.
						 * 
						 */
						*dstr = xptr;
						break;
					}

					break;
				}

				xtp = xtbuf;

				while (**dstr && (**dstr != '>'))
				{
					SAFE_SB_CHR(tolower(**dstr), xtbuf, &xtp);
					(*dstr)++;
				}

				if (**dstr != '>')
				{
					/**
				     * Ran off the end. Back up.
					 * 
				     */
					*dstr = xptr;
					break;
				}

				*xtp = '\0';

				for (i = 0; i < mudstate.rdata->xr_alloc; i++)
				{
					if (mudstate.rdata->x_names[i] && !strcmp(xtbuf, mudstate.rdata->x_names[i]))
					{
						SAFE_STRNCAT(buff, bufc, mudstate.rdata->x_regs[i], mudstate.rdata->x_lens[i], LBUF_SIZE);
						break;
					}
				}

				break;

			case 'O':
				/** 
				 * Objective pronoun
				 * 
				 */
			case 'o':
				if (gender < 0)
				{
					gender = get_gender(cause);
				}

				if (!gender)
				{
					safe_name(cause, buff, bufc);
				}
				else
					SAFE_LB_STR((char *)get_obj(gender), buff, bufc);

				break;

			case 'P':
				/** 
				 * Personal pronoun
				 * 
				 */
			case 'p':
				if (gender < 0)
				{
					gender = get_gender(cause);
				}

				if (!gender)
				{
					safe_name(cause, buff, bufc);
					SAFE_LB_CHR('s', buff, bufc);
				}
				else
				{
					SAFE_LB_STR((char *)get_poss(gender), buff, bufc);
				}

				break;

			case 'S':
				/** 
				 * Subjective pronoun
				 * 
				 */
			case 's':
				if (gender < 0)
				{
					gender = get_gender(cause);
				}

				if (!gender)
				{
					safe_name(cause, buff, bufc);
				}
				else
					SAFE_LB_STR((char *)get_subj(gender), buff, bufc);

				break;

			case 'A': 
			case 'a':
				/** 
				 * Absolute possessive - idea from Empedocles
				 * 
				 */
				if (gender < 0)
				{
					gender = get_gender(cause);
				}

				if (!gender)
				{
					safe_name(cause, buff, bufc);
					SAFE_LB_CHR('s', buff, bufc);
				}
				else
				{
					SAFE_LB_STR((char *)get_absp(gender), buff, bufc);
				}

				break;

			case '#':
				/** 
				 * Invoker DB number
				 * 
				 */
				safe_dbref(buff, bufc, cause);
				break;

			case '!':
				/** 
				 * Executor DB number
				 * 
				 */
				safe_dbref(buff, bufc, player);
				break;

			case 'N':
			case 'n':
				/** 
				 * Invoker name
				 * 
				 */
				safe_name(cause, buff, bufc);
				break;

			case 'L':
			case 'l':
				/** 
				 * Invoker location db#
				 * 
				 */
				if (!(eval & EV_NO_LOCATION))
				{
					safe_dbref(buff, bufc, where_is(cause));
				}

				break;

			case '@':
				/** 
				 * Caller dbref 
				 * 
				 */
				safe_dbref(buff, bufc, caller);
				break;

			case ':': 
				/** 
				 * Enactor's objID
				 * 
				 */
				safe_dbref(buff, bufc, cause);
				SAFE_LB_CHR(':', buff, bufc);
				SAFE_LTOS(buff, bufc, CreateTime(cause), LBUF_SIZE);
				break;

			case 'M':
			case 'm':
				SAFE_LB_STR(mudstate.curr_cmd, buff, bufc);
				break;

			case 'I':
			case 'i':
				/**
				 * itext() equivalent
				 * 
				 */
			case 'J':
			case 'j':
				/** 
				 * itext2() equivalent
				 * 
				 */
				xtp = *dstr;
				(*dstr)++;

				if (!**dstr)
				{
					(*dstr)--;
				}

				if (**dstr == '-')
				{
					/**
				     * use absolute level number
					 * 
				     */
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
					/**
				     * use number as delta back from current
					 * 
				     */
					if (!mudstate.in_loop || !isdigit(**dstr))
					{
						break;
					}

					i = mudstate.in_loop - 1 - (**dstr - '0');

					if (i < 0)
					{
						break;
					}
				}

				if (i > mudstate.in_loop - 1)
				{
					break;
				}

				if ((*xtp == 'i') || (*xtp == 'I'))
				{
					SAFE_LB_STR(mudstate.loop_token[i], buff, bufc);
				}
				else
				{
					SAFE_LB_STR(mudstate.loop_token2[i], buff, bufc);
				}

				break;

			case '+': 
				/** 
				 * Arguments to function 
				 * 
				 */
				SAFE_LTOS(buff, bufc, ncargs, LBUF_SIZE);
				break;

			case '|':
				/** 
				 * Piped command output
				 * 
				 */
				SAFE_LB_STR(mudstate.pout, buff, bufc);
				break;

			case '%': 
				/** 
				 * Percent - a literal %
				 * 
				 */
				SAFE_LB_CHR('%', buff, bufc);
				break;

			default:
				/** 
				 * Just copy
				 * 
				 */
				SAFE_LB_CHR(**dstr, buff, bufc);
			}

			if (isupper(savec))
			{
				*savepos = toupper(*savepos);
			}

			break;

		case '(':
			/**
		     * Arglist start.  See if what precedes is a function. If so,
			 * execute it if we should.
			 * 
		     */
			at_space = 0;

			if (!(eval & EV_FCHECK))
			{
				SAFE_LB_CHR('(', buff, bufc);
				break;
			}

			/**
		     * Load an sbuf with an uppercase version of the func name, and see
			 * if the func exists.  Trim trailing spaces from the name if 
			 * configured.
			 * 
		     */
			**bufc = '\0';
			xtp = xtbuf;
			SAFE_SB_STR(oldp, xtbuf, &xtp);
			*xtp = '\0';

			if (mudconf.space_compress && (eval & EV_FMAND))
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

			fp = (FUN *)hashfind(xtbuf, &mudstate.func_htab);
			/**
		     * If not a builtin func, check for global func
			 * 
		     */
			ufp = NULL;

			if (fp == NULL)
			{
				ufp = (UFUN *)hashfind(xtbuf, &mudstate.ufunc_htab);
			}

			/**
		     * Do the right thing if it doesn't exist
			 * 
		     */
			if (!fp && !ufp)
			{
				if (eval & EV_FMAND)
				{
					*bufc = oldp;
					SAFE_SPRINTF(buff, bufc, "#-1 FUNCTION (%s) NOT FOUND", xtbuf);
					alldone = 1;
				}
				else
				{
					SAFE_LB_CHR('(', buff, bufc);
				}

				eval &= ~EV_FCHECK;
				break;
			}

			/*
		     * Get the arglist and count the number of args Negative # of args
			 * means join subsequent args.
			 * 
		     */
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

			/**
		     * If no closing delim, just insert the '(' and continue normally
			 * 
		     */
			if (!*dstr)
			{
				*dstr = tstr;
				SAFE_LB_CHR(**dstr, buff, bufc);

				for (i = 0; i < nfargs; i++)
					if (fargs[i] != NULL)
					{
						XFREE(fargs[i]);
					}

				eval &= ~EV_FCHECK;
				break;
			}

			/**
		     * Count number of args returned
			 * 
		     */
			(*dstr)--;
			j = 0;

			for (i = 0; i < nfargs; i++)
				if (fargs[i] != NULL)
				{
					j = i + 1;
				}

			nfargs = j;
			/**
		     * We've got function(args) now, so back up over function name in
			 * output buffer.
			 * 
		     */
			*bufc = oldp;

			/**
		     * If it's a user-defined function, perform it now.
			 * 
		     */

			if (ufp)
			{
				mudstate.func_nest_lev++;
				mudstate.func_invk_ctr++;

				if (mudstate.func_nest_lev >= mudconf.func_nest_lim)
				{
					SAFE_LB_STR("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
				}
				else if (mudstate.func_invk_ctr >= mudconf.func_invk_lim)
				{
					SAFE_LB_STR("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
				}
				else if (Too_Much_CPU())
				{
					SAFE_LB_STR("#-1 FUNCTION CPU LIMIT EXCEEDED", buff, bufc);
				}
				else if (Going(player))
				{
					SAFE_LB_STR("#-1 BAD INVOKER", buff, bufc);
				}
				else if (!check_access(player, ufp->perms))
				{
					SAFE_NOPERM(buff, bufc);
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
						preserve = mudstate.rdata;
						mudstate.rdata = NULL;
					}
					else if (ufp->flags & FN_PRES)
					{
						preserve = save_global_regs("eval.save");
					}

					exec(buff, bufc, i, player, cause, ((ufp->flags & FN_NO_EVAL) ? (EV_FCHECK | EV_EVAL) : feval), &str, fargs, nfargs);

					if (ufp->flags & FN_NOREGS)
					{
						Free_RegData(mudstate.rdata);
						mudstate.rdata = preserve;
					}
					else if (ufp->flags & FN_PRES)
					{
						restore_global_regs("eval.restore", preserve);
					}

					XFREE(tstr);
				}

				/**
				 * Return the space allocated for the args
				 * 
				 */
				mudstate.func_nest_lev--;

				for (i = 0; i < nfargs; i++)
					if (fargs[i] != NULL)
					{
						XFREE(fargs[i]);
					}

				eval &= ~EV_FCHECK;
				break;
			}

			/**
		     * If the number of args is right, perform the func. Otherwise
			 * return an error message.  Note that parse_arglist returns zero
			 * args as one null arg, so we have to handle that case specially.
			 * 
	    	 */
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
				/**
				 * Check recursion limit
				 * 
				 */
				mudstate.func_nest_lev++;
				mudstate.func_invk_ctr++;

				if (mudstate.func_nest_lev >= mudconf.func_nest_lim)
				{
					SAFE_LB_STR("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufc);
				}
				else if (mudstate.func_invk_ctr >= mudconf.func_invk_lim)
				{
					SAFE_LB_STR("#-1 FUNCTION INVOCATION LIMIT EXCEEDED", buff, bufc);
				}
				else if (Too_Much_CPU())
				{
					SAFE_LB_STR("#-1 FUNCTION CPU LIMIT EXCEEDED", buff, bufc);
				}
				else if (Going(player))
				{
					/**
				     * Deal with the peculiar case of the calling object being destroyed
				     * mid-function sequence, such as with a command()/@destroy combo...
					 * 
				     */
					SAFE_LB_STR("#-1 BAD INVOKER", buff, bufc);
				}
				else if (!Check_Func_Access(player, fp))
				{
					SAFE_NOPERM(buff, bufc);
				}
				else if (mudstate.f_limitmask & fp->flags)
				{
					SAFE_NOPERM(buff, bufc);
				}
				else
				{
					fargs[-1] = (char *)fp;
					fp->fun(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs);
				}

				mudstate.func_nest_lev--;
			}
			else
			{
				SAFE_SPRINTF(buff, bufc, "#-1 FUNCTION (%s) EXPECTS %d ARGUMENTS BUT GOT %d", fp->name, fp->nargs, nfargs);
			}

			/**
		     * Return the space allocated for the arguments
			 * 
		     */
			for (i = 0; i < nfargs; i++)
				if (fargs[i] != NULL)
				{
					XFREE(fargs[i]);
				}

			eval &= ~EV_FCHECK;
			break;

		case '#':
			/**
		     * We should never reach this point unless we're in a loop or
			 * switch, thanks to the table lookup.
			 * 
		     */
			at_space = 0;
			(*dstr)++;

			if (!token_char((unsigned char)**dstr))
			{
				(*dstr)--;
				SAFE_LB_CHR(**dstr, buff, bufc);
			}
			else
			{
				if ((**dstr == '#') && mudstate.in_loop)
				{
					SAFE_LB_STR(mudstate.loop_token[mudstate.in_loop - 1], buff, bufc);
				}
				else if ((**dstr == '@') && mudstate.in_loop)
				{
					SAFE_LTOS(buff, bufc, mudstate.loop_number[mudstate.in_loop - 1], LBUF_SIZE);
				}
				else if ((**dstr == '+') && mudstate.in_loop)
				{
					SAFE_LB_STR(mudstate.loop_token2[mudstate.in_loop - 1], buff, bufc);
				}
				else if ((**dstr == '$') && mudstate.in_switch)
				{
					SAFE_LB_STR(mudstate.switch_token, buff, bufc);
				}
				else if (**dstr == '!')
				{
					/**
				     * Nesting level of loop takes precedence over switch
					 * nesting level.
					 * 
				     */
					SAFE_LTOS(buff, bufc, ((mudstate.in_loop) ? (mudstate.in_loop - 1) : mudstate.in_switch), LBUF_SIZE);
				}
				else
				{
					(*dstr)--;
					SAFE_LB_CHR(**dstr, buff, bufc);
				}
			}

			break;

		case ESC_CHAR:
			safe_copy_esccode(&(*dstr), buff, bufc);
			(*dstr)--;
			break;
		}

		(*dstr)++;
	}

	/**
     * If we're eating spaces, and the last thing was a space, eat it up.
     * Complicated by the fact that at_space is initially true. So check
     * to see if we actually put something in the buffer, too.
	 * 
     */
	if (mudconf.space_compress && at_space && !(eval & EV_NO_COMPRESS) && (start != *bufc))
	{
		(*bufc)--;
	}

	/**
     * The ansi() function knows how to take care of itself. However, if
     * the player used a %x sub in the string, and hasn't yet terminated
     * the color with a %xn yet, we'll have to do it for them.
	 * 
     */
	if (ansi)
	{
		SAFE_ANSI_NORMAL(buff, bufc);
	}

	**bufc = '\0';

	/**
     * Report trace information
	 * 
     */
	if (is_trace)
	{
		tcache_add(savestr, start);
		save_count = tcache_count - mudconf.trace_limit;
		;

		if (is_top || !mudconf.trace_topdown)
		{
			tcache_finish(player);
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
		SAFE_LB_STR(buff, realbuff, bufc);
		**bufc = '\0';
		XFREE(buff);
		buff = realbuff;
	}
	XFREE(xtbuf);
}

/**
 * @brief Save the global registers to protect them from various sorts of 
 * munging.
 * 
 * @param funcname	Function name
 * @return GDATA* 
 */
GDATA *save_global_regs(const char *funcname)
{
	GDATA *preserve;

	if (mudstate.rdata)
	{
		Alloc_RegData(funcname, mudstate.rdata, preserve);
		Copy_RegData(funcname, mudstate.rdata, preserve);
	}
	else
	{
		preserve = NULL;
	}

	return preserve;
}

/**
 * @brief Restore the global registers to protect them from various sorts of
 * munging.
 * 
 * @param funcname Function name
 * @param preserve Buffer where globals have been saved to.
 */
void restore_global_regs(const char *funcname, GDATA *preserve)
{
	if (!mudstate.rdata && !preserve)
	{
		return;
	}

	if (mudstate.rdata && preserve && (mudstate.rdata->dirty == preserve->dirty))
	{
		/**
		 * No change in the values. Move along.
		 * 
		 */
		Free_RegData(preserve);
		return;
	}

	/**
     * Rather than doing a big free-and-copy thing, we could just handle
     * changes in the data structure size. Place for future optimization.
	 * 
     */
	if (!preserve)
	{
		Free_RegData(mudstate.rdata);
		mudstate.rdata = NULL;
	}
	else
	{
		if (mudstate.rdata)
		{
			Free_RegData(mudstate.rdata);
		}

		Alloc_RegData(funcname, preserve, mudstate.rdata);
		Copy_RegData(funcname, preserve, mudstate.rdata);
		Free_RegData(preserve);
	}
}
