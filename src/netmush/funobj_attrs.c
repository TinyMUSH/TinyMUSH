/**
 * @file funobj_attrs.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Object-focused built-ins: attribute access and user-defined functions
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

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

/**
 * @brief Does object X have attribute Y.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_hasattr(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	ATTR *attr = NULL;
	char *tbuf = NULL;
	int aflags = 0, alen = 0, check_parents = Is_Func(CHECK_PARENTS);
	dbref thing = match_thing(player, fargs[0]), aowner = NOTHING;
	int exam = 0;

	if (!Good_obj(thing))
	{
		XSAFENOMATCH(buff, bufc);
		return;
	}
	else if (!(exam = Examinable(player, thing)))
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	attr = atr_str(fargs[1]);

	if (!attr)
	{
		XSAFELBCHR('0', buff, bufc);
		return;
	}

	if (check_parents)
	{
		atr_pget_info(thing, attr->number, &aowner, &aflags);
	}
	else
	{
		atr_get_info(thing, attr->number, &aowner, &aflags);
	}

	if (!See_attr(player, thing, attr, aowner, aflags))
	{
		XSAFELBCHR('0', buff, bufc);
	}
	else
	{
		if (check_parents)
		{
			tbuf = atr_pget(thing, attr->number, &aowner, &aflags, &alen);
		}
		else
		{
			tbuf = atr_get(thing, attr->number, &aowner, &aflags, &alen);
		}

		if (*tbuf)
		{
			XSAFELBCHR('1', buff, bufc);
		}
		else
		{
			XSAFELBCHR('0', buff, bufc);
		}

		XFREE(tbuf);
	}
}

/**
 * @brief Function form of %-substitution
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_v(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref aowner;
	int aflags, alen;
	char *sbufc, *tbuf, *str;
	ATTR *ap;
	tbuf = fargs[0];

	if (isalpha(tbuf[0]) && tbuf[1])
	{
		/**
		 * Fetch an attribute from me.  First see if it exists,
		 * returning a null string if it does not.
		 *
		 */
		ap = atr_str(fargs[0]);

		if (!ap)
		{
			return;
		}

		/**
		 * If we can access it, return it, otherwise return a null string
		 *
		 */
		tbuf = atr_pget(player, ap->number, &aowner, &aflags, &alen);

		if (See_attr(player, player, ap, aowner, aflags))
		{
			XSAFESTRNCAT(buff, bufc, tbuf, alen, LBUF_SIZE);
		}

		XFREE(tbuf);
		return;
	}

	/**
	 * Not an attribute, process as %&lt;arg&gt;
	 *
	 */
	char sbuf[SBUF_SIZE];
	sbufc = sbuf;
	XSAFESBCHR('%', sbuf, &sbufc);
	XSAFESBSTR(fargs[0], sbuf, &sbufc);
	*sbufc = '\0';
	str = sbuf;
	eval_expression_string(buff, bufc, player, caller, cause, EV_FIGNORE, &str, cargs, ncargs);
}

/**
 * @brief Get attribute from object: GET, XGET, GET_EVAL, EVAL(obj,atr)
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause Not used
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void perform_get(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	char *atr_gotten = NULL, *str = NULL;
	int attrib = 0, aflags = 0, alen, eval_it = Is_Func(GET_EVAL);

	if (Is_Func(GET_XARGS))
	{
		if (!*fargs[0] || !*fargs[1])
		{
			return;
		}

		str = XASPRINTF("str", "%s/%s", fargs[0], fargs[1]);
	}
	else
	{
		str = XASPRINTF("str", "%s", fargs[0]);
	}

	if (!parse_attrib(player, str, &thing, &attrib, 0))
	{
		XSAFENOMATCH(buff, bufc);
		XFREE(str);
		return;
	}

	XFREE(str);

	if (attrib == NOTHING)
	{
		return;
	}

	/**
	 * There used to be code here to handle AF_IS_LOCK attributes, but
	 * parse_attrib can never return one of those. Use fun_lock instead.
	 *
	 */
	atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);

	if (eval_it)
	{
		str = atr_gotten;
		eval_expression_string(buff, bufc, thing, player, player, EV_FIGNORE | EV_EVAL, &str, (char **)NULL, 0);
	}
	else
	{
		XSAFESTRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
	}

	XFREE(atr_gotten);
}

/**
 * @brief The first form of this function is identical to get_eval(), but
 *        splits the &lt;object&gt; and &lt;attribute&gt; into two arguments, rather
 *        than having an &lt;object&gt;/&lt;attribute&gt; pair. It is provided for
 *        PennMUSH compatibility.
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
void fun_eval(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 1, 2, buff, bufc))
	{
		return;
	}

	if (nfargs == 1)
	{
		char *str = fargs[0];
		eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_FCHECK, &str, (char **)NULL, 0);
		return;
	}

	perform_get(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs);
}

/**
 * @brief Call a user-defined function: U, ULOCAL, UPRIVATE
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void do_ufun(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref aowner = NOTHING, thing = NOTHING;
	int aflags = 0, alen = 0, anum = 0, trace_flag = 0;
	ATTR *ap = NULL;
	char *atext = NULL, *str = NULL;
	GDATA *preserve = NULL;
	int is_local = Is_Func(U_LOCAL), is_private = Is_Func(U_PRIVATE);

	/**
	 * We need at least one argument
	 *
	 */
	if (nfargs < 1)
	{
		XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
		return;
	}

	/**
	 * First arg: &lt;obj&gt;/&lt;attr&gt; or &lt;attr&gt; or \#lambda/&lt;code&gt;
	 *
	 */
	if (string_prefix(fargs[0], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen((fargs[0]) + 8);
		__xstrcpy(atext, fargs[0] + 8);
		atext[alen] = '\0';
		aowner = player;
		aflags = 0;
	}
	else
	{
		if (parse_attrib(player, fargs[0], &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(fargs[0]);
		}
		if (!ap)
		{
			return;
		}
		atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
		if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
		{
			XFREE(atext);
			return;
		}
	}

	/**
	 * If we're evaluating locally, preserve the global registers. If
	 * we're evaluating privately, preserve and wipe out.
	 *
	 */
	if (is_local)
	{
		preserve = save_global_regs("fun_ulocal.save");
	}
	else if (is_private)
	{
		preserve = mushstate.rdata;
		mushstate.rdata = NULL;
	}

	/**
	 * If the trace flag is on this attr, set the object Trace
	 *
	 */
	if (!Trace(thing) && (aflags & AF_TRACE))
	{
		trace_flag = 1;
		s_Trace(thing);
	}
	else
	{
		trace_flag = 0;
	}

	/**
	 * Evaluate it using the rest of the passed function args
	 *
	 */
	str = atext;
	eval_expression_string(buff, bufc, thing, player, cause, EV_FCHECK | EV_EVAL, &str, &(fargs[1]), nfargs - 1);
	XFREE(atext);

	/**
	 * Reset the trace flag if we need to
	 *
	 */
	if (trace_flag)
	{
		c_Trace(thing);
	}

	/**
	 * If we're evaluating locally, restore the preserved registers. If
	 * we're evaluating privately, free whatever data we had and restore.
	 *
	 */
	if (is_local)
	{
		restore_global_regs("fun_ulocal.restore", preserve);
	}
	else if (is_private)
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
}

/**
 * @brief  Call the text of a u-function from a specific object's perspective.
 *         (i.e., get the text as the player, but execute it as the specified
 *         object.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller Not used
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Number of function's arguments
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_objcall(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref obj = NOTHING, aowner = NOTHING, thing = NOTHING;
	int aflags = 0, alen = 0, anum = 0;
	ATTR *ap = NULL;
	char *atext = NULL, *str = NULL;

	if (nfargs < 2)
	{
		XSAFESTRNCAT(buff, bufc, "#-1 TOO FEW ARGUMENTS", 21, LBUF_SIZE);
		return;
	}

	/**
	 * First arg: &lt;obj&gt;/&lt;attr&gt; or &lt;attr&gt; or \#lambda/&lt;code&gt;
	 *
	 */
	if (string_prefix(fargs[1], "#lambda/"))
	{
		thing = player;
		anum = NOTHING;
		ap = NULL;
		atext = XMALLOC(LBUF_SIZE, "lambda.atext");
		alen = strlen((fargs[1]) + 8);
		__xstrcpy(atext, fargs[1] + 8);
		atext[alen] = '\0';
		aowner = player;
		aflags = 0;
	}
	else
	{
		if (parse_attrib(player, fargs[1], &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(fargs[1]);
		}
		if (!ap)
		{
			return;
		}
		atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);
		if (!*atext || !(See_attr(player, thing, ap, aowner, aflags)))
		{
			XFREE(atext);
			return;
		}
	}
	/**
	 * Find our perspective.
	 *
	 */
	obj = match_thing(player, fargs[0]);

	if (Cannot_Objeval(player, obj))
	{
		obj = player;
	}

	/**
	 * Evaluate using the rest of the passed function args.
	 *
	 */
	str = atext;
	eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str, &(fargs[2]), nfargs - 2);
	XFREE(atext);
}

/**
 * @brief Evaluate a function with local scope (i.e., preserve and restore the
 *        r-registers). Essentially like calling ulocal() but with the function
 *        string directly.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_localize(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str = fargs[0];
	;
	GDATA *preserve = save_global_regs("fun_localize_save");

	eval_expression_string(buff, bufc, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	restore_global_regs("fun_localize_restore", preserve);
}

/**
 * @brief Evaluate a function with a strictly local scope -- do not pass global
 *        registers and discard any changes made to them. Like calling
 *        uprivate() but with the function string directly.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_private(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *str = fargs[0];
	GDATA *preserve = mushstate.rdata;
	mushstate.rdata = NULL;

	eval_expression_string(buff, bufc, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);

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

/**
 * @brief This function returns the value of &lt;obj&gt;/&lt;attr&gt;, as if retrieved via
 *        the get() function, if the attribute exists and is readable by Player
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_default(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int attrib = 0, aflags = 0, alen = 0;
	ATTR *attr = NULL;
	char *objname = NULL, *atr_gotten = NULL, *bp = NULL, *str = fargs[0];
	objname = bp = XMALLOC(LBUF_SIZE, "objname");

	eval_expression_string(objname, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);

	/**
	 * First we check to see that the attribute exists on the object. If
	 * so, we grab it and use it.
	 *
	 */
	if (objname != NULL)
	{
		if (parse_attrib(player, objname, &thing, &attrib, 0) && (attrib != NOTHING))
		{
			attr = atr_num(attrib);

			if (attr && !(attr->flags & AF_IS_LOCK))
			{
				atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);

				if (*atr_gotten)
				{
					XSAFESTRNCAT(buff, bufc, atr_gotten, alen, LBUF_SIZE);
					XFREE(atr_gotten);
					XFREE(objname);
					return;
				}

				XFREE(atr_gotten);
			}
		}

		XFREE(objname);
	}

	/**
	 * If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default.
	 *
	 */
	str = fargs[1];
	eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/**
 * @brief This function returns the evaluated value of &lt;obj&gt;/&lt;attr&gt;, as if
 *        retrieved via the get_eval() function, if the attribute exists and
 *        is readable by you. Otherwise, it evaluates the default case, and
 *        returns that. The default case is only evaluated if the attribute
 *        does not exist or cannot be read.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_edefault(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int attrib = 0, aflags = 0, alen = 0;
	ATTR *attr = NULL;
	char *objname = NULL, *atr_gotten = NULL, *bp = NULL, *str = fargs[0];

	objname = bp = XMALLOC(LBUF_SIZE, "objname");

	eval_expression_string(objname, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);

	/**
	 * First we check to see that the attribute exists on the object. If
	 * so, we grab it and use it.
	 *
	 */
	if (objname != NULL)
	{
		if (parse_attrib(player, objname, &thing, &attrib, 0) && (attrib != NOTHING))
		{
			attr = atr_num(attrib);

			if (attr && !(attr->flags & AF_IS_LOCK))
			{
				atr_gotten = atr_pget(thing, attrib, &aowner, &aflags, &alen);

				if (*atr_gotten)
				{
					str = atr_gotten;
					eval_expression_string(buff, bufc, thing, player, player, EV_FIGNORE | EV_EVAL, &str, (char **)NULL, 0);
					XFREE(atr_gotten);
					XFREE(objname);
					return;
				}

				XFREE(atr_gotten);
			}
		}

		XFREE(objname);
	}

	/**
	 * If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default.
	 *
	 */
	str = fargs[1];
	eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/**
 * @brief This function returns the value of the user-defined function as
 *        defined by &lt;attr&gt; (or &lt;obj&gt;/&lt;attr&gt;), as if retrieved via the u()
 *        function, with &lt;args&gt;, if the attribute exists and is readable
 *        by you.
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
void fun_udefault(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref thing = NOTHING, aowner = NOTHING;
	int aflags = 0, alen = 0, anum = 0, i = 0, j = 0, trace_flag = 0;
	ATTR *ap = NULL;
	char *objname = NULL, *atext = NULL, *str = NULL, *bp = NULL, *xargs[NUM_ENV_VARS];

	if (nfargs < 2)
	{
		/**
		 * must have at least two arguments
		 *
		 */
		return;
	}

	str = fargs[0];
	objname = bp = XMALLOC(LBUF_SIZE, "objname");
	eval_expression_string(objname, &bp, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);

	/**
	 * First we check to see that the attribute exists on the object. If
	 * so, we grab it and use it.
	 *
	 */
	if (objname != NULL)
	{
		if (parse_attrib(player, objname, &thing, &anum, 0))
		{
			if ((anum == NOTHING) || !(Good_obj(thing)))
				ap = NULL;
			else
				ap = atr_num(anum);
		}
		else
		{
			thing = player;
			ap = atr_str(objname);
		}

		if (ap)
		{
			atext = atr_pget(thing, ap->number, &aowner, &aflags, &alen);

			if (*atext)
			{
				/**
				 * Now we have a problem -- we've got to go
				 * eval all of those arguments to the
				 * function.
				 *
				 */
				for (i = 2, j = 0; j < NUM_ENV_VARS; i++, j++)
				{
					if ((i < nfargs) && fargs[i])
					{
						bp = xargs[j] = XMALLOC(LBUF_SIZE, "xargs[j]");
						str = fargs[i];
						eval_expression_string(xargs[j], &bp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
					}
					else
					{
						xargs[j] = NULL;
					}
				}

				/**
				 * We have the args, now call the ufunction.
				 * Obey the trace flag on the attribute if
				 * there is one.
				 */
				if (!Trace(thing) && (aflags & AF_TRACE))
				{
					trace_flag = 1;
					s_Trace(thing);
				}
				else
				{
					trace_flag = 0;
				}

				str = atext;
				eval_expression_string(buff, bufc, thing, player, cause, EV_FCHECK | EV_EVAL, &str, xargs, nfargs - 2);

				if (trace_flag)
				{
					c_Trace(thing);
				}

				/**
				 * Then clean up after ourselves.
				 *
				 */
				for (j = 0; j < NUM_ENV_VARS; j++)
					if (xargs[j])
					{
						XFREE(xargs[j]);
					}

				XFREE(atext);
				XFREE(objname);
				return;
			}

			XFREE(atext);
		}

		XFREE(objname);
	}

	/**
	 * If we've hit this point, we've not gotten anything useful, so we
	 * go and evaluate the default.
	 *
	 */
	str = fargs[1];
	eval_expression_string(buff, bufc, player, caller, cause, EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
}

/**
 * @brief Evaluate from a specific object's perspective.
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player DBref of player
 * @param caller DBref of caller
 * @param cause DBref of cause
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Command's arguments
 * @param ncargs Nomber of command's arguments
 */
void fun_objeval(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref obj = NOTHING;
	char *name = NULL, *bp = NULL, *str = NULL;

	if (!*fargs[0])
	{
		return;
	}

	name = bp = XMALLOC(LBUF_SIZE, "bp");
	str = fargs[0];
	eval_expression_string(name, &bp, player, caller, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	obj = match_thing(player, name);

	/**
	 * In order to evaluate from something else's viewpoint, you must
	 * have the same owner as it, or be a wizard (unless
	 * objeval_requires_control is turned on, in which case you must
	 * control it, period). Otherwise, we default to evaluating from our
	 * own viewpoint. Also, you cannot evaluate things from the point of
	 * view of God.
	 *
	 */
	if (Cannot_Objeval(player, obj))
	{
		obj = player;
	}

	str = fargs[1];
	eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs);
	XFREE(name);
}
