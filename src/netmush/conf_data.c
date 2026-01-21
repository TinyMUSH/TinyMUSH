/**
 * @file conf_data.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration helpers for aliases, info fields, logging diversion, sites, and access control
 * @version 4.0
 *
 * Contains configuration interpreters and helpers related to hash-table aliases,
 * INFO text management, log diversion, site/netmask parsing, and configuration
 * directive access (read/write) listing and modification.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool _cf_alias_ensure_hashtab(HASHTAB *htab)
{
	bool initialized = false;

	if (htab == NULL)
	{
		return false;
	}

	initialized = ((htab->entry != NULL) && (htab->hashsize > 0));

	if (initialized)
	{
		return true;
	}

	if (htab == &mushstate.command_htab)
	{
		init_cmdtab();
	}
	else if (htab == &mushstate.logout_cmd_htab)
	{
		init_logout_cmdtab();
	}
	else if (htab == &mushstate.flags_htab)
	{
		init_flagtab();
	}
	else if (htab == &mushstate.powers_htab)
	{
		init_powertab();
	}
	else if (htab == &mushstate.func_htab)
	{
		init_functab();
	}
	else if (htab == &mushstate.attr_name_htab)
	{
		init_attrtab();
	}

	return ((htab->entry != NULL) && (htab->hashsize > 0));
}

CF_Result cf_alias(int *vp, char *str, long extra, dbref player, char *cmd)
{
	HASHTAB *htab = (HASHTAB *)vp;
	int *cp = NULL;
	char *p = NULL;
	char *tokst = NULL;
	char *alias_start = NULL;
	char *alias_end = NULL;
	char *orig_start = NULL;
	char *orig_end = NULL;
	int upcase = 0;

	if ((str == NULL) || (htab == NULL))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias configuration requires valid input");
		return CF_Failure;
	}

	if (!_cf_alias_ensure_hashtab(htab))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Invalid hash table for alias");
		return CF_Failure;
	}

	alias_start = strtok_r(str, " \t=,", &tokst);

	if (alias_start == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias requires name");
		return CF_Failure;
	}

	orig_start = strtok_r(NULL, " \t=,", &tokst);

	if (orig_start == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias %s requires original entry", alias_start);
		return CF_Failure;
	}

	alias_end = alias_start + strlen(alias_start);
	while ((alias_end > alias_start) && isspace((unsigned char)alias_end[-1]))
	{
		alias_end--;
	}
	*alias_end = '\0';

	orig_end = orig_start + strlen(orig_start);
	while ((orig_end > orig_start) && isspace((unsigned char)orig_end[-1]))
	{
		orig_end--;
	}
	*orig_end = '\0';

	if ((*alias_start == '\0') || (*orig_start == '\0'))
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Alias and original names cannot be empty");
		return CF_Failure;
	}

	upcase = 0;

	for (p = orig_start; *p; p++)
	{
		*p = tolower(*p);
	}

	cp = hashfind(orig_start, htab);

	if (cp == NULL)
	{
		upcase++;

		for (p = orig_start; *p; p++)
		{
			*p = toupper(*p);
		}

		cp = hashfind(orig_start, htab);

		if (cp == NULL)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", (char *)extra, orig_start);
			return CF_Failure;
		}
	}

	if (upcase)
	{
		for (p = alias_start; *p; p++)
		{
			*p = toupper(*p);
		}
	}
	else
	{
		for (p = alias_start; *p; p++)
		{
			*p = tolower(*p);
		}
	}

	if (htab->flags & HT_KEYREF)
	{
		p = alias_start;
		alias_start = XSTRDUP(p, "alias");
	}

	return hashadd(alias_start, cp, htab, HASH_ALIAS);
}

static LINKEDLIST *_conf_infotext_find(const char *name, LINKEDLIST **prev_out)
{
	LINKEDLIST *prev = NULL;
	LINKEDLIST *cur = mushconf.infotext_list;

	while (cur != NULL)
	{
		if (!strcasecmp(name, cur->name))
		{
			if (prev_out)
			{
				*prev_out = prev;
			}
			return cur;
		}

		prev = cur;
		cur = cur->next;
	}

	if (prev_out)
	{
		*prev_out = NULL;
	}

	return NULL;
}

CF_Result cf_infotext(int *vp, char *str, long extra, dbref player, char *cmd)
{
	LINKEDLIST *prev = NULL;
	LINKEDLIST *itp = NULL;
	char *fvalue = NULL, *tokst = NULL;
	char *fname = NULL;

	(void)vp;
	(void)extra;
	(void)player;
	(void)cmd;

	fname = strtok_r(str, " \t=,", &tokst);

	if (!fname || (*fname == '\0'))
	{
		return CF_Partial;
	}

	if (tokst)
	{
		for (fvalue = tokst; *fvalue && ((*fvalue == ' ') || (*fvalue == '\t')); fvalue++)
		{
			/* noop */
		}
	}
	else
	{
		fvalue = NULL;
	}

	itp = _conf_infotext_find(fname, &prev);

	if (!fvalue || !*fvalue)
	{
		if (itp)
		{
			if (prev)
			{
				prev->next = itp->next;
			}
			else
			{
				mushconf.infotext_list = itp->next;
			}

			XFREE(itp->name);
			XFREE(itp->value);
			XFREE(itp);
		}

		return CF_Partial;
	}

	if (itp)
	{
		XFREE(itp->value);
		itp->value = XSTRDUP(fvalue, "itp->value");
		return CF_Partial;
	}

	itp = (LINKEDLIST *)XMALLOC(sizeof(LINKEDLIST), "itp");
	itp->name = XSTRDUP(fname, "itp->name");
	itp->value = XSTRDUP(fvalue, "itp->value");
	itp->next = mushconf.infotext_list;
	mushconf.infotext_list = itp;
	return CF_Partial;
}

CF_Result cf_divert_log(int *vp, char *str, long extra, dbref player, char *cmd)
{
	char *type_str = NULL, *file_str = NULL, *tokst = NULL;
	int f = 0, fd = 0;
	FILE *fptr = NULL;
	LOGFILETAB *tp = NULL, *lp = NULL, *target = NULL;

	(void)extra;
	(void)player;

	type_str = strtok_r(str, " \t", &tokst);
	file_str = strtok_r(NULL, " \t", &tokst);

	if (!type_str || !file_str)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Missing pathname to log to.");
		return CF_Failure;
	}

	f = search_nametab(GOD, (NAMETAB *)extra, type_str);

	if (f <= 0)
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Log diversion", str);
		return CF_Failure;
	}

	for (lp = logfds_table; lp->log_flag; lp++)
	{
		if (lp->log_flag == f)
		{
			target = lp;
		}

		if (lp->filename && !strcmp(file_str, lp->filename))
		{
			fptr = lp->fileptr;
		}
	}

	if (!target)
	{
		cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Logfile table corruption", type_str);
		return CF_Failure;
	}

	if (target->filename != NULL)
	{
		log_write(LOG_STARTUP, "CNF", "DIVT", "Log type %s already diverted: %s", type_str, target->filename);
		return CF_Failure;
	}

	if (!fptr)
	{
		fptr = fopen(file_str, "w");

		if (!fptr)
		{
			log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot open logfile: %s", file_str);
			return CF_Failure;
		}

		fd = fileno(fptr);
		if (fd == -1)
		{
			log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot get fd for logfile: %s", file_str);
			return CF_Failure;
		}
#ifdef FNDELAY
		if (fcntl(fd, F_SETFL, FNDELAY) == -1)
		{
			log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot make nonblocking: %s", file_str);
			return CF_Failure;
		}
#else
		if (fcntl(fd, F_SETFL, O_NDELAY) == -1)
		{
			log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot make nonblocking: %s", file_str);
			return CF_Failure;
		}
#endif
	}

	target->fileptr = fptr;
	target->filename = XSTRDUP(file_str, "tp->filename");
	*vp |= f;
	return CF_Success;
}

static in_addr_t _cf_sane_inet_addr(char *str)
{
	struct in_addr addr;

	if ((str == NULL) || (inet_pton(AF_INET, str, &addr) != 1))
	{
		return INADDR_NONE;
	}

	return addr.s_addr;
}

CF_Result cf_site(long **vp, char *str, long extra, dbref player, char *cmd)
{
	SITE *site = NULL, *last = NULL, *head = NULL;
	char *addr_txt = NULL, *mask_txt = NULL, *tokst = NULL, *endp = NULL;
	struct in_addr addr_num, mask_num;
	int mask_bits = 0;

	(void)player;
	(void)cmd;

	if (str == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Missing site address and mask.");
		return CF_Failure;
	}

	memset(&mask_num, 0, sizeof(mask_num));

	if ((mask_txt = strchr(str, '/')) == NULL)
	{
		addr_txt = strtok_r(str, " \t=,", &tokst);
		if (addr_txt)
		{
			mask_txt = strtok_r(NULL, " \t=,", &tokst);
		}

		if (!addr_txt || !*addr_txt || !mask_txt || !*mask_txt)
		{
			cf_log(player, "CNF", "SYNTX", cmd, "Missing host address or mask.");
			return CF_Failure;
		}

		if ((addr_num.s_addr = _cf_sane_inet_addr(addr_txt)) == INADDR_NONE)
		{
			cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
			return CF_Failure;
		}

		if (strcmp(mask_txt, "255.255.255.255") != 0)
		{
			if ((mask_num.s_addr = _cf_sane_inet_addr(mask_txt)) == INADDR_NONE)
			{
				cf_log(player, "CNF", "SYNTX", cmd, "Malformed mask address: %s", mask_txt);
				return CF_Failure;
			}
		}
		else
		{
			mask_num.s_addr = htonl(0xFFFFFFFFU);
		}
	}
	else
	{
		addr_txt = str;
		*mask_txt++ = '\0';

		mask_bits = (int)strtol(mask_txt, &endp, 10);

		if ((mask_bits < 0) || (mask_bits > 32) || (*endp != '\0'))
		{
			cf_log(player, "CNF", "SYNTX", cmd, "Invalid CIDR mask: %s (expected 0-32)", mask_txt);
			return CF_Failure;
		}

		if (mask_bits == 0)
		{
			mask_num.s_addr = htonl(0);
		}
		else if (mask_bits == 32)
		{
			mask_num.s_addr = htonl(0xFFFFFFFFU);
		}
		else
		{
			mask_num.s_addr = htonl(0xFFFFFFFFU << (32 - mask_bits));
		}

		if ((addr_num.s_addr = _cf_sane_inet_addr(addr_txt)) == INADDR_NONE)
		{
			cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
			return CF_Failure;
		}
	}

	head = (SITE *)*vp;

	site = (SITE *)XMALLOC(sizeof(SITE), "site");
	if (!site)
	{
		return CF_Failure;
	}

	site->address.s_addr = addr_num.s_addr;
	site->mask.s_addr = mask_num.s_addr;
	site->flag = (long)extra;
	site->next = NULL;

	if (mushstate.initializing)
	{
		if (head == NULL)
		{
			*vp = (long *)site;
		}
		else
		{
			for (last = head; last->next != NULL; last = last->next)
				;
			last->next = site;
		}
	}
	else
	{
		site->next = head;
		*vp = (long *)site;
	}

	return CF_Success;
}

static CF_Result _cf_cf_access(CONF *tp, dbref player, int *vp, char *ap, char *cmd, long extra)
{
	const char *access_type = ((long)vp) ? "read" : "write";

	(void)extra;

	if (tp->flags & CA_STATIC)
	{
		notify(player, NOPERM_MESSAGE);

		if (db)
		{
			char *name = log_getname(player);
			log_write(LOG_CONFIGMODS, "CFG", "PERM", "%s tried to change %s access to static param: %s", name, access_type, tp->pname);
			XFREE(name);
		}
		else
		{
			log_write(LOG_CONFIGMODS, "CFG", "PERM", "System tried to change %s access to static param: %s", access_type, tp->pname);
		}

		return CF_Failure;
	}

	if ((long)vp)
	{
		return cf_modify_bits(&tp->rperms, ap, extra, player, cmd);
	}
	else
	{
		return cf_modify_bits(&tp->flags, ap, extra, player, cmd);
	}
}

CF_Result cf_cf_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
	CONF *tp = NULL, *ctab = NULL;
	MODULE *mp = NULL;
	char *directive_name = NULL, *perms_str = NULL;
	char *str_copy = NULL;

	if (str == NULL)
	{
		cf_log(player, "CNF", "SYNTX", cmd, "Missing directive name and permissions.");
		return CF_Failure;
	}

	str_copy = XSTRDUP(str, "str_copy");
	directive_name = str_copy;

	for (perms_str = directive_name; *perms_str && !isspace((unsigned char)*perms_str); perms_str++)
		;

	if (*perms_str)
	{
		*perms_str++ = '\0';
		while (*perms_str && isspace((unsigned char)*perms_str))
			perms_str++;
	}
	else
	{
		perms_str = "";
	}

	for (tp = conftable; tp->pname; tp++)
	{
		if (!strcmp(tp->pname, directive_name))
		{
			CF_Result result = _cf_cf_access(tp, player, vp, perms_str, cmd, extra);
			XFREE(str_copy);
			return result;
		}
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
		{
			for (tp = ctab; tp->pname; tp++)
			{
				if (!strcmp(tp->pname, directive_name))
				{
					CF_Result result = _cf_cf_access(tp, player, vp, perms_str, cmd, extra);
					XFREE(str_copy);
					return result;
				}
			}
		}
	}

	cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config directive", directive_name);
	XFREE(str_copy);
	return CF_Failure;
}

void list_cf_access(dbref player)
{
	CONF *tp = NULL, *ctab = NULL;
	MODULE *mp = NULL;

	if (!Good_obj(player))
	{
		return;
	}

	notify(player, "Attribute                      Permission");
	notify(player, "------------------------------ ------------------------------------------------");

	for (tp = conftable; tp->pname; tp++)
	{
		if (God(player) || check_access(player, tp->flags))
		{
			listset_nametab(player, access_nametab, tp->flags, true, "%-30.30s ", tp->pname);
		}
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
		if (ctab != NULL)
		{
			for (tp = ctab; tp->pname; tp++)
			{
				if (God(player) || check_access(player, tp->flags))
				{
					listset_nametab(player, access_nametab, tp->flags, true, "%-30.30s ", tp->pname);
				}
			}
		}
	}
	notify(player, "-------------------------------------------------------------------------------");
}

void list_cf_read_access(dbref player)
{
	CONF *tp = NULL, *ctab = NULL;
	MODULE *mp = NULL;
	char *buff = NULL;

	if (!Good_obj(player))
	{
		return;
	}

	notify(player, "Attribute                      Permission");
	notify(player, "------------------------------ ------------------------------------------------");

	buff = XMALLOC(SBUF_SIZE, "buff");

	for (tp = conftable; tp->pname; tp++)
	{
		if (God(player) || check_access(player, tp->rperms))
		{
			XSNPRINTF(buff, SBUF_SIZE, "%-30.30s ", tp->pname);
			listset_nametab(player, access_nametab, tp->rperms, true, buff);
		}
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
		if (ctab != NULL)
		{
			for (tp = ctab; tp->pname; tp++)
			{
				if (God(player) || check_access(player, tp->rperms))
				{
					XSNPRINTF(buff, SBUF_SIZE, "%-30.30s ", tp->pname);
					listset_nametab(player, access_nametab, tp->rperms, true, buff);
				}
			}
		}
	}

	XFREE(buff);
	notify(player, "-------------------------------------------------------------------------------");
}
