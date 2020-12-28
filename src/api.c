/**
 * @file api.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief functions called only by modules
 * @version 3.3
 * @date 2020-12-24
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

/* api.c -  */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mushconf */
#include "game.h"	   /* required by mushconf */
#include "alloc.h"	   /* required by mushconf */
#include "flags.h"	   /* required by mushconf */
#include "htab.h"	   /* required by mushconf */
#include "ltdl.h"	   /* required by mushconf */
#include "udb.h"	   /* required by mushconf */
#include "udb_defs.h"  /* required by mushconf */
#include "mushconf.h"  /* required by code */
#include "db.h"		   /* required by externs */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by interface */
#include "command.h"   /* required by code */
#include "match.h"	   /* required by code */
#include "vattr.h"	   /* required by code */
#include "attrs.h"	   /* required by code */
#include "powers.h"	   /* required by code */
#include "functions.h" /* required by code */
#include "udb.h"	   /* required by code */
#include "udb_defs.h"  /* required by code */

extern CMDENT *prefix_cmds[256];

/**
 * @brief Register a module
 * 
 * @param module_name	Module name
 * @param api_name		API name
 * @param ftable		Function table
 */
void register_api(char *module_name, char *api_name, API_FUNCTION *ftable)
{
	MODULE *mp;
	API_FUNCTION *afp;
	void (*fn_ptr)(void *, void *);
	int succ = 0;
	char *s;

	for (mp = mudstate.modules_list; mp != NULL; mp = mp->next)
	{
		if (!strcmp(module_name, mp->modname))
		{
			succ = 1;
			break;
		}
	}

	if (!succ)
	{
		/** 
		 * no such module
		 * 
		 */
		return;
	}

	for (afp = ftable; afp->name; afp++)
	{
		s = XASPRINTF("s", "mod_%s_%s", module_name, afp->name);
		fn_ptr = (void (*)(void *, void *))lt_dlsym(mp->handle, s);
		XFREE(s);

		if (fn_ptr != NULL)
		{
			afp->handler = fn_ptr;
			s = XASPRINTF("s", "%s_%s", api_name, afp->name);
			hashadd(s, (int *)afp, &mudstate.api_func_htab, 0);
			XFREE(s);
		}
	}
}

/**
 * @brief Return the handler of an API function
 * 
 * @param api_name	API name
 * @param fn_name 	Function name
 * @return void*	Handler of that function
 */
void *request_api_function(char *api_name, char *fn_name)
{
	API_FUNCTION *afp;
	char *s = XASPRINTF("s", "%s_%s", api_name, fn_name);
	afp = (API_FUNCTION *)hashfind(s, &mudstate.api_func_htab);
	XFREE(s);

	if (!afp)
	{
		return NULL;
	}

	return afp->handler;
}

/**
 * @brief Register module's commands with the main command handler
 * 
 * @param cmdtab Module's command table.
 */
void register_commands(CMDENT *cmdtab)
{
	CMDENT *cp;
	char *s;

	if (cmdtab)
	{
		for (cp = cmdtab; cp->cmdname; cp++)
		{
			hashadd(cp->cmdname, (int *)cp, &mudstate.command_htab, 0);
			s = XASPRINTF("s", "__%s", cp->cmdname);
			hashadd(s, (int *)cp, &mudstate.command_htab, HASH_ALIAS);
			XFREE(s);
		}
	}
}

/**
 * @brief Register prefix commands.
 * 
 * @param cmdchars char array of prefixes
 */
void register_prefix_cmds(const char *cmdchars)
{
	const char *cp;
	char *cn = XSTRDUP("x", "cn");

	if (cmdchars)
	{
		for (cp = cmdchars; *cp; cp++)
		{
			cn[0] = *cp;
			prefix_cmds[(unsigned char)*cp] = (CMDENT *)hashfind(cn, &mudstate.command_htab);
		}
	}
	XFREE(cn);
}

/**
 * @brief Register module's functions with the main functions handler
 * 
 * @param functab 
 */
void register_functions(FUN *functab)
{
	FUN *fp;

	if (functab)
	{
		for (fp = functab; fp->name; fp++)
		{
			hashadd((char *)fp->name, (int *)fp, &mudstate.func_htab, 0);
		}
	}
}

/**
 * @brief Register module's hashtables with the main hashtables handler
 * 
 * @param htab 
 * @param ntab 
 */
void register_hashtables(MODHASHES *htab, MODNHASHES *ntab)
{
	MODHASHES *hp;
	MODNHASHES *np;

	if (htab)
	{
		for (hp = htab; hp->tabname != NULL; hp++)
		{
			hashinit(hp->htab, hp->size_factor * mudconf.hash_factor, HT_STR);
		}
	}

	if (ntab)
	{
		for (np = ntab; np->tabname != NULL; np++)
		{
			nhashinit(np->htab, np->size_factor * mudconf.hash_factor);
		}
	}
}

/**
 * @brief Register a module's DB type.
 * 
 * @param modname Module name
 * @return unsigned int Module's DBType
 */
unsigned int register_dbtype(char *modname)
{
	unsigned int type;
	DBData key, data;
	/**
	 * Find out if the module already has a registered DB type
	 * 
	 */
	key.dptr = modname;
	key.dsize = strlen(modname) + 1;
	data = db_get(key, DBTYPE_MODULETYPE);

	if (data.dptr)
	{
		XMEMCPY((void *)&type, (void *)data.dptr, sizeof(unsigned int));
		XFREE(data.dptr);
		return type;
	}

	/**
	 * If the type is in range, return it, else return zero as an error code
	 * 
	 */

	if ((mudstate.moduletype_top >= DBTYPE_RESERVED) && (mudstate.moduletype_top < DBTYPE_END))
	{
		/*
		 * Write the entry to GDBM
		 */
		data.dptr = &mudstate.moduletype_top;
		data.dsize = sizeof(unsigned int);
		db_put(key, data, DBTYPE_MODULETYPE);
		type = mudstate.moduletype_top;
		mudstate.moduletype_top++;
		return type;
	}
	else
	{
		return 0;
	}
}
