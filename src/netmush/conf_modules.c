/**
 * @file conf_modules.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module loading helpers and shared library symbol resolution
 * @version 4.0
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <ctype.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CF_Result cf_module(int *vp, char *modname, long extra, dbref player, char *cmd)
{
	MODULE *mp = NULL;
	MODULE *iter = NULL;
	void *handle = NULL;
	void (*initptr)(void) = NULL;
	char *name = modname;
	char *end = NULL;

	(void)vp;
	(void)extra;

	if (modname == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Module name is required");
		return CF_Failure;
	}

	while (*name != '\0' && isspace((unsigned char)*name))
	{
		name++;
	}

	end = name + strlen(name);
	while ((end > name) && isspace((unsigned char)end[-1]))
	{
		end--;
	}
	*end = '\0';

	if (*name == '\0')
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Module name is required");
		return CF_Failure;
	}

	for (iter = mushstate.modules_list; iter != NULL; iter = iter->next)
	{
		if (strcmp(iter->modname, name) == 0)
		{
			cf_log(player, "CNF", "MOD", cmd, "Module %s already loaded", name);
			return CF_Success;
		}
	}

	handle = dlopen_format("%s/lib%s.so", mushconf.modules_home, name);

	if (!handle)
	{
		cf_log(player, "CNF", "MOD", cmd, "Loading of %s/lib%s.so failed: %s", mushconf.modules_home, name, dlerror());
		return CF_Failure;
	}

	mp = (MODULE *)XMALLOC(sizeof(MODULE), "mp");
	mp->modname = XSTRDUP(name, "mp->modname");
	mp->handle = handle;
	mp->next = mushstate.modules_list;
	mushstate.modules_list = mp;

	mp->process_command = (int (*)(dbref, dbref, int, char *, char *[], int))dlsym_format(handle, "mod_%s_%s", name, "process_command");
	mp->process_no_match = (int (*)(dbref, dbref, int, char *, char *, char *[], int))dlsym_format(handle, "mod_%s_%s", name, "process_no_match");
	mp->did_it = (int (*)(dbref, dbref, dbref, int, const char *, int, const char *, int, int, char *[], int, int))dlsym_format(handle, "mod_%s_%s", name, "did_it");
	mp->create_obj = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", name, "create_obj");
	mp->destroy_obj = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", name, "destroy_obj");
	mp->create_player = (void (*)(dbref, dbref, int, int))dlsym_format(handle, "mod_%s_%s", name, "create_player");
	mp->destroy_player = (void (*)(dbref, dbref))dlsym_format(handle, "mod_%s_%s", name, "destroy_player");
	mp->announce_connect = (void (*)(dbref, const char *, int))dlsym_format(handle, "mod_%s_%s", name, "announce_connect");
	mp->announce_disconnect = (void (*)(dbref, const char *, int))dlsym_format(handle, "mod_%s_%s", name, "announce_disconnect");
	mp->examine = (void (*)(dbref, dbref, dbref, int, int))dlsym_format(handle, "mod_%s_%s", name, "examine");
	mp->dump_database = (void (*)(FILE *))dlsym_format(handle, "mod_%s_%s", name, "dump_database");
	mp->db_grow = (void (*)(int, int))dlsym_format(handle, "mod_%s_%s", name, "db_grow");
	mp->db_write = (void (*)(void))dlsym_format(handle, "mod_%s_%s", name, "db_write");
	mp->db_write_flatfile = (void (*)(FILE *))dlsym_format(handle, "mod_%s_%s", name, "db_write_flatfile");
	mp->do_second = (void (*)(void))dlsym_format(handle, "mod_%s_%s", name, "do_second");
	mp->cache_put_notify = (void (*)(UDB_DATA, unsigned int))dlsym_format(handle, "mod_%s_%s", name, "cache_put_notify");
	mp->cache_del_notify = (void (*)(UDB_DATA, unsigned int))dlsym_format(handle, "mod_%s_%s", name, "cache_del_notify");

	if (!mushstate.standalone)
	{
		if ((initptr = (void (*)(void))dlsym_format(handle, "mod_%s_%s", modname, "init")) != NULL)
		{
			(*initptr)();
		}
	}

	log_write(LOG_STARTUP, "CNF", "MOD", "Loaded module: %s", modname);
	return CF_Success;
}

void *dlopen_format(const char *filename, ...)
{
	char stackbuf[256];
	char *buf = stackbuf;
	size_t size = sizeof(stackbuf);
	int n = 0;
	va_list ap;
	void *dlhandle = NULL;

	if (filename == NULL)
	{
		return NULL;
	}

	va_start(ap, filename);
	n = vsnprintf(buf, size, filename, ap);
	va_end(ap);

	if (n < 0)
	{
		return NULL;
	}

	if ((size_t)n >= size)
	{
		size = (size_t)n + 1;
		buf = malloc(size);
		if (buf == NULL)
		{
			return NULL;
		}

		va_start(ap, filename);
		n = vsnprintf(buf, size, filename, ap);
		va_end(ap);

		if ((n < 0) || ((size_t)n >= size))
		{
			free(buf);
			return NULL;
		}
	}

	dlhandle = dlopen(buf, RTLD_LAZY);

	if (buf != stackbuf)
	{
		free(buf);
	}

	return dlhandle;
}

void *dlsym_format(void *place, const char *symbol, ...)
{
	char stackbuf[256];
	char *buf = stackbuf;
	size_t size = sizeof(stackbuf);
	int n = 0;
	va_list ap;
	void *_dlsym = NULL;

	if ((place == NULL) || (symbol == NULL))
	{
		return NULL;
	}

	va_start(ap, symbol);
	n = vsnprintf(buf, size, symbol, ap);
	va_end(ap);

	if (n < 0)
	{
		return NULL;
	}

	if ((size_t)n >= size)
	{
		size = (size_t)n + 1;
		buf = malloc(size);
		if (buf == NULL)
		{
			return NULL;
		}

		va_start(ap, symbol);
		n = vsnprintf(buf, size, symbol, ap);
		va_end(ap);

		if ((n < 0) || ((size_t)n >= size))
		{
			free(buf);
			return NULL;
		}
	}

	_dlsym = dlsym(place, buf);

	if (buf != stackbuf)
	{
		free(buf);
	}

	return _dlsym;
}
