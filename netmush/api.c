/**
 * @file api.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief functions called only by modules
 * @version 3.3
 * @date 2020-12-24
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

/* api.c -  */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

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
	MODULE *mp = NULL;
	API_FUNCTION *afp = NULL;

	void (*fn_ptr)(void *, void *) = NULL;
	int succ = 0;
	char *s = NULL;

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
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
		fn_ptr = (void (*)(void *, void *))dlsym(mp->handle, s);
		XFREE(s);

		if (fn_ptr != NULL)
		{
			afp->handler = fn_ptr;
			s = XASPRINTF("s", "%s_%s", api_name, afp->name);
			hashadd(s, (int *)afp, &mushstate.api_func_htab, 0);
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
	API_FUNCTION *afp = NULL;

	char *s = XASPRINTF("s", "%s_%s", api_name, fn_name);
	afp = (API_FUNCTION *)hashfind(s, &mushstate.api_func_htab);
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
	CMDENT *cp = NULL;
	char *s = NULL;

	if (cmdtab)
	{
		for (cp = cmdtab; cp->cmdname; cp++)
		{
			hashadd(cp->cmdname, (int *)cp, &mushstate.command_htab, 0);
			s = XASPRINTF("s", "__%s", cp->cmdname);
			hashadd(s, (int *)cp, &mushstate.command_htab, HASH_ALIAS);
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
	const char *cp = NULL;
	char *cn = XSTRDUP("x", "cn");

	if (cmdchars)
	{
		for (cp = cmdchars; *cp; cp++)
		{
			cn[0] = *cp;
			prefix_cmds[(unsigned char)*cp] = (CMDENT *)hashfind(cn, &mushstate.command_htab);
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
	FUN *fp = NULL;

	if (functab)
	{
		for (fp = functab; fp->name; fp++)
		{
			hashadd((char *)fp->name, (int *)fp, &mushstate.func_htab, 0);
		}
	}
}

/**
 * @brief Register module's hashtables with the main hashtables handler
 *
 * @param htab
 * @param ntab
 */
void register_hashtables(MODHASHES *htab, MODHASHES *ntab)
{
	MODHASHES *hp = NULL;
	MODHASHES *np = NULL;

	if (htab)
	{
		for (hp = htab; hp->tabname != NULL; hp++)
		{
			hashinit(hp->htab, hp->size_factor * mushconf.hash_factor, HT_STR);
		}
	}

	if (ntab)
	{
		for (np = ntab; np->tabname != NULL; np++)
		{
			nhashinit(np->htab, np->size_factor * mushconf.hash_factor);
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
	unsigned int type = 0;
	UDB_DATA key, data;
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

	if ((mushstate.moduletype_top >= DBTYPE_RESERVED) && (mushstate.moduletype_top < DBTYPE_END))
	{
		/*
		 * Write the entry to GDBM
		 */
		data.dptr = &mushstate.moduletype_top;
		data.dsize = sizeof(unsigned int);
		db_put(key, data, DBTYPE_MODULETYPE);
		type = mushstate.moduletype_top;
		mushstate.moduletype_top++;
		return type;
	}
	else
	{
		return 0;
	}
}
