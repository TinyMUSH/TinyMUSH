/**
 * @file conf_files.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration file handling, helpfile loading, and runtime dispatch
 * @version 4.0
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "conf_internal.h"

#include <ctype.h>
#include <dlfcn.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

CF_Result cf_add_helpfile(dbref player, char *confcmd, char *str, bool is_raw)
{
	CMDENT *cmdp = NULL;
	HASHTAB *hashes = NULL;
	FILE *fp = NULL;
	char *fcmd = NULL, *fpath = NULL, *newstr = NULL, *tokst = NULL;
	char **ftab = NULL;
	char *s = NULL;
	char *full_fpath = NULL;

	if ((str == NULL) || (confcmd == NULL))
	{
		cf_log(player, "CNF", "SYNTX", confcmd ? confcmd : "cf_add_helpfile", "Missing input parameters");
		return -1;
	}

	newstr = XMALLOC(MBUF_SIZE, "newstr");
	XSTRCPY(newstr, str);
	fcmd = strtok_r(newstr, " \t=,", &tokst);
	fpath = strtok_r(NULL, " \t=,", &tokst);

	if ((fcmd == NULL) || (*fcmd == '\0') || (fpath == NULL) || (*fpath == '\0'))
	{
		cf_log(player, "CNF", "SYNTX", confcmd, "Missing command name or file path");
		XFREE(newstr);
		return -1;
	}

	if ((fcmd[0] == '_') && (fcmd[1] == '_'))
	{
		cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s would cause @addcommand conflict", fcmd);
		XFREE(newstr);
		return -1;
	}

	s = XMALLOC(MAXPATHLEN, "s");

	XSNPRINTF(s, MAXPATHLEN, "%s.txt", fpath);
	fp = fopen(s, "r");

	if (fp == NULL)
	{
		full_fpath = XASPRINTF("full_fpath", "%s/%s", mushconf.txthome, fpath);
		XSNPRINTF(s, MAXPATHLEN, "%s.txt", full_fpath);
	fp = fopen(s, "r");

		if (fp == NULL)
		{
			cf_log(player, "HLP", "LOAD", confcmd, "Helpfile %s not found", fcmd);
			XFREE(newstr);
			XFREE(s);
			XFREE(full_fpath);
			return -1;
		}

		fpath = full_fpath;
	}

	fclose(fp);

	if (strlen(fpath) > SBUF_SIZE)
	{
		cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s filename too long", fcmd);
		XFREE(newstr);
		XFREE(s);
		XFREE(full_fpath);
		return -1;
	}

	cf_log(player, "HLP", "LOAD", confcmd, "Loading helpfile %s", basename(fpath));

	if (helpmkindx(player, confcmd, fpath))
	{
		cf_log(player, "HLP", "LOAD", confcmd, "Could not create index for helpfile %s, not loaded.", basename(fpath));
		XFREE(newstr);
		XFREE(s);
		XFREE(full_fpath);
		return -1;
	}

	cmdp = (CMDENT *)XMALLOC(sizeof(CMDENT), "cmdp");
	cmdp->cmdname = XSTRDUP(fcmd, "cmdp->cmdname");
	cmdp->switches = NULL;
	cmdp->perms = 0;
	cmdp->pre_hook = NULL;
	cmdp->post_hook = NULL;
	cmdp->userperms = NULL;
	cmdp->callseq = CS_ONE_ARG;
	cmdp->info.handler = do_help;
	cmdp->extra = mushstate.helpfiles;

	if (is_raw)
	{
		cmdp->extra |= HELP_RAWHELP;
	}

	hashdelete(cmdp->cmdname, &mushstate.command_htab);
	hashadd(cmdp->cmdname, (int *)cmdp, &mushstate.command_htab, 0);
	XSNPRINTF(s, MAXPATHLEN, "__%s", cmdp->cmdname);
	hashdelete(s, &mushstate.command_htab);
	hashadd(s, (int *)cmdp, &mushstate.command_htab, HASH_ALIAS);

	if (!mushstate.hfiletab)
	{
		mushstate.hfiletab = (char **)XCALLOC(4, sizeof(char *), "mushstate.hfiletab");
		mushstate.hfile_hashes = (HASHTAB *)XCALLOC(4, sizeof(HASHTAB), "mushstate.hfile_hashes");
		mushstate.hfiletab_size = 4;
	}
	else if (mushstate.helpfiles >= mushstate.hfiletab_size)
	{
		ftab = (char **)XREALLOC(mushstate.hfiletab, (mushstate.hfiletab_size + 4) * sizeof(char *), "ftab");
		hashes = (HASHTAB *)XREALLOC(mushstate.hfile_hashes, (mushstate.hfiletab_size + 4) * sizeof(HASHTAB), "hashes");
		ftab[mushstate.hfiletab_size] = NULL;
		ftab[mushstate.hfiletab_size + 1] = NULL;
		ftab[mushstate.hfiletab_size + 2] = NULL;
		ftab[mushstate.hfiletab_size + 3] = NULL;
		mushstate.hfiletab_size += 4;
		mushstate.hfiletab = ftab;
		mushstate.hfile_hashes = hashes;
	}

	if (mushstate.hfiletab[mushstate.helpfiles] != NULL)
	{
		XFREE(mushstate.hfiletab[mushstate.helpfiles]);
	}

	mushstate.hfiletab[mushstate.helpfiles] = XSTRDUP(fpath, "mushstate.hfiletab[mushstate.helpfiles]");

	hashinit(&mushstate.hfile_hashes[mushstate.helpfiles], 30 * mushconf.hash_factor, HT_STR);
	mushstate.helpfiles++;

	cf_log(player, "HLP", "LOAD", confcmd, "Successfully loaded helpfile %s", basename(fpath));

	XFREE(s);
	XFREE(newstr);
	XFREE(full_fpath);

	return 0;
}

CF_Result cf_helpfile(int *vp, char *str, long extra, dbref player, char *cmd)
{
	(void)vp;
	(void)extra;
	return cf_add_helpfile(player, cmd, str, 0);
}

CF_Result cf_raw_helpfile(int *vp, char *str, long extra, dbref player, char *cmd)
{
	(void)vp;
	(void)extra;
	return cf_add_helpfile(player, cmd, str, 1);
}

CF_Result cf_include(int *vp, char *filename, long extra, dbref player, char *cmd)
{
	FILE *fp = NULL;
	char *filepath = NULL, *buf = NULL, *line_ptr = NULL, *cmd_ptr = NULL;
	char *arg_ptr = NULL, *comment_ptr = NULL, *trim_ptr = NULL;
	int line_num = 0;

	(void)vp;
	(void)extra;

	if (!mushstate.initializing)
	{
		return CF_Failure;
	}

	if ((filename == NULL) || (cmd == NULL))
	{
		cf_log(player, "CNF", "SYNTX", cmd ? cmd : "include", "Missing filename parameter");
		return CF_Failure;
	}

	filepath = XSTRDUP(filename, "filepath");
	fp = fopen(filepath, "r");

	if (fp == NULL)
	{
		char *full_path = XASPRINTF("full_path", "%s/%s", mushconf.config_home, filename);
		fp = fopen(full_path, "r");

		if (fp == NULL)
		{
			cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config file", filename);
			XFREE(filepath);
			XFREE(full_path);
			return CF_Failure;
		}

		XFREE(filepath);
		filepath = full_path;
	}

	log_write(LOG_ALWAYS, "CNF", "INFO", "Reading configuration file : %s", filepath);
	mushstate.cfiletab = add_array(mushstate.cfiletab, filepath, &mushstate.configfiles);
	XFREE(filepath);

	buf = XMALLOC(LBUF_SIZE, "buf");

	while (fgets(buf, LBUF_SIZE, fp) != NULL)
	{
		line_num++;
		line_ptr = buf;

		comment_ptr = strchr(line_ptr, '\n');
		if (comment_ptr)
		{
			*comment_ptr = '\0';
		}

		while (*line_ptr && isspace((unsigned char)*line_ptr))
		{
			line_ptr++;
		}

		if ((!*line_ptr) || (*line_ptr == '#'))
		{
			continue;
		}

		cmd_ptr = line_ptr;
		while (*cmd_ptr && !isspace((unsigned char)*cmd_ptr))
		{
			cmd_ptr++;
		}

		if (*cmd_ptr)
		{
			*cmd_ptr++ = '\0';

			while (*cmd_ptr && isspace((unsigned char)*cmd_ptr))
			{
				cmd_ptr++;
			}

			arg_ptr = cmd_ptr;
		}
		else
		{
			arg_ptr = cmd_ptr;
		}

		comment_ptr = strchr(arg_ptr, '#');
		if (comment_ptr)
		{
			if ((comment_ptr == arg_ptr) || (isspace((unsigned char)*(comment_ptr - 1)) && isdigit((unsigned char)*(comment_ptr + 1))))
			{
				/* keep */
			}
			else
			{
				*comment_ptr = '\0';
			}
		}

		trim_ptr = arg_ptr + strlen(arg_ptr);
		while ((trim_ptr > arg_ptr) && isspace((unsigned char)*(trim_ptr - 1)))
		{
			trim_ptr--;
		}
		*trim_ptr = '\0';

		if (*line_ptr)
		{
			cf_set(line_ptr, arg_ptr, player);
		}
	}

	if (!feof(fp))
	{
		cf_log(player, "CNF", "ERROR", cmd, "Line %d: Error reading configuration file", line_num);
		XFREE(buf);
		fclose(fp);
		return CF_Failure;
	}

	XFREE(buf);
	fclose(fp);
	return CF_Success;
}

static CF_Result _cf_set(char *cp, char *ap, dbref player, CONF *tp)
{
	CF_Result result = CF_Failure;
	const char *status_msg = "Strange.";
	char *buff = NULL, *buf = NULL, *name = NULL;
	int interp_result = 0;

	if (!mushstate.standalone && !mushstate.initializing && !check_access(player, tp->flags))
	{
		notify(player, NOPERM_MESSAGE);
		return CF_Failure;
	}

	if (!mushstate.initializing)
	{
		buff = XSTRDUP(ap, "buff");
	}

	cf_interpreter = tp->interpreter;
	interp_result = cf_interpreter(tp->loc, ap, tp->extra, player, cp);

	if (mushstate.initializing)
	{
		return interp_result;
	}

	switch (interp_result)
	{
	case 0:
		result = CF_Success;
		status_msg = "Success.";
		break;

	case 1:
		result = CF_Partial;
		status_msg = "Partial success.";
		break;

	case -1:
		result = CF_Failure;
		status_msg = "Failure.";
		break;

	default:
		result = CF_Failure;
		status_msg = "Strange.";
	}

	name = log_getname(player);
	buf = ansi_strip_ansi(buff);
	log_write(LOG_CONFIGMODS, "CFG", "UPDAT", "%s entered config directive: %s with args '%s'. Status: %s", name, cp, buf, status_msg);

	XFREE(buf);
	XFREE(name);
	XFREE(buff);

	return result;
}

CF_Result cf_set(char *cp, char *ap, dbref player)
{
	CONF *tp = NULL, *ctab = NULL;
	MODULE *mp = NULL;
	bool is_essential = false;

	if (cp == NULL)
	{
		cf_log(player, "CNF", "SYNTX", (char *)"Set", "Missing configuration directive name");
		return CF_Failure;
	}

	is_essential = (!strcmp(cp, "module") || !strcmp(cp, "database_home"));

	if (mushstate.standalone && !is_essential)
	{
		return CF_Success;
	}

	for (tp = conftable; tp->pname; tp++)
	{
		if (!strcmp(tp->pname, cp))
		{
			return _cf_set(cp, ap, player, tp);
		}
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		if ((ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable")) != NULL)
		{
			for (tp = ctab; tp->pname; tp++)
			{
				if (!strcmp(tp->pname, cp))
				{
					return _cf_set(cp, ap, player, tp);
				}
			}
		}
	}

	if (!mushstate.standalone)
	{
		cf_log(player, "CNF", "NFND", (char *)"Set", "%s %s not found", "Config directive", cp);
	}

	return CF_Failure;
}

void cf_do_admin(dbref player, dbref cause, int extra, char *kw, char *value)
{
	(void)cause;
	(void)extra;

	if (kw == NULL)
	{
		notify(player, "Syntax: @admin <directive> <value>");
		return;
	}

	int result = cf_set(kw, value, player);

	if ((result >= 0) && !Quiet(player))
	{
		notify(player, "Set.");
	}
}

CF_Result cf_read(char *fn)
{
	if (fn == NULL)
	{
		log_write(LOG_ALWAYS, "CNF", "ERROR", "cf_read: NULL filename provided");
		return CF_Failure;
	}

	return cf_include(NULL, fn, 0, 0, "init");
}

static void _cf_verify_table(CONF *ctab)
{
	CONF *tp = NULL;
	dbref current = NOTHING;
	dbref default_ref = NOTHING;
	bool valid_ref = false;

	if (ctab == NULL)
	{
		return;
	}

	for (tp = ctab; tp->pname; tp++)
	{
		if (tp->interpreter != cf_dbref)
		{
			continue;
		}

		if (tp->loc == NULL)
		{
			continue;
		}

		current = *(tp->loc);
		default_ref = (dbref)tp->extra;
		valid_ref = (((default_ref == NOTHING) && (current == NOTHING)) || (Good_obj(current) && !Going(current)));

		if (!valid_ref)
		{
			log_write(LOG_ALWAYS, "CNF", "VRFY", "%s #%d is invalid. Reset to #%d.", tp->pname, current, default_ref);
			*(tp->loc) = default_ref;
		}
	}
}

void cf_verify(void)
{
	CONF *ctab = NULL;
	MODULE *mp = NULL;

	_cf_verify_table(conftable);

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
		_cf_verify_table(ctab);
	}
}

static void _cf_display(dbref player, char *buff, char **bufc, CONF *tp)
{
	NAMETAB *opt = NULL;

	if ((tp == NULL) || (buff == NULL) || (bufc == NULL) || (tp->loc == NULL))
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	if (!check_access(player, tp->rperms))
	{
		XSAFENOPERM(buff, bufc);
		return;
	}

	if ((tp->interpreter == cf_bool) || (tp->interpreter == cf_int) || (tp->interpreter == cf_int_factor) || (tp->interpreter == cf_const))
	{
		XSAFELTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
		return;
	}

	if (tp->interpreter == cf_string)
	{
		XSAFELBSTR(*((char **)tp->loc), buff, bufc);
		return;
	}

	if (tp->interpreter == cf_dbref)
	{
		XSAFELBCHR('#', buff, bufc);
		XSAFELTOS(buff, bufc, *(tp->loc), LBUF_SIZE);
		return;
	}

	if (tp->interpreter == cf_option)
	{
		opt = find_nametab_ent_flag(GOD, (NAMETAB *)tp->extra, *(tp->loc));
		XSAFELBSTR((opt ? opt->name : "*UNKNOWN*"), buff, bufc);
		return;
	}

	XSAFENOPERM(buff, bufc);
	return;
}

void cf_display(dbref player, char *param_name, char *buff, char **bufc)
{
	CONF *tp = NULL, *ctab = NULL;
	MODULE *mp = NULL;

	if ((param_name == NULL) || (buff == NULL) || (bufc == NULL))
	{
		if ((buff != NULL) && (bufc != NULL))
		{
			XSAFENOMATCH(buff, bufc);
		}
		return;
	}

	if (*param_name == '\0')
	{
		XSAFENOMATCH(buff, bufc);
		return;
	}

	for (tp = conftable; tp->pname; tp++)
	{
		if (!strcasecmp(tp->pname, param_name))
		{
			_cf_display(player, buff, bufc, tp);
			return;
		}
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
		if (ctab == NULL)
		{
			continue;
		}

		for (tp = ctab; tp->pname; tp++)
		{
			if (!strcasecmp(tp->pname, param_name))
			{
				_cf_display(player, buff, bufc, tp);
				return;
			}
		}
	}

	XSAFENOMATCH(buff, bufc);
}

static void _list_option_entry(dbref player, CONF *tp)
{
	char status = (*(tp->loc) ? 'Y' : 'N');
	char *desc = (tp->extra ? (char *)tp->extra : "");

	raw_notify(player, "%-25s %c %s?", tp->pname, status, desc);
}

void list_options(dbref player)
{
	CONF *tp = NULL, *ctab = NULL;
	MODULE *mp = NULL;
	bool draw_header = false;
	bool is_option = false;

	if (!Good_obj(player))
	{
		return;
	}

	notify(player, "Global Options            S Description");
	notify(player, "------------------------- - ---------------------------------------------------");
	for (tp = conftable; tp->pname; tp++)
	{
		is_option = ((tp->interpreter == cf_const) || (tp->interpreter == cf_bool));
		if (is_option && check_access(player, tp->rperms))
		{
			_list_option_entry(player, tp);
		}
	}

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		ctab = (CONF *)dlsym_format(mp->handle, "mod_%s_%s", mp->modname, "conftable");
		if (ctab == NULL)
		{
			continue;
		}

		draw_header = false;
		for (tp = ctab; tp->pname; tp++)
		{
			is_option = ((tp->interpreter == cf_const) || (tp->interpreter == cf_bool));
			if (is_option && check_access(player, tp->rperms))
			{
				if (!draw_header)
				{
					raw_notify(player, "\nModule %-18.18s S Description", mp->modname);
					notify(player, "------------------------- - ---------------------------------------------------");
					draw_header = true;
				}
				_list_option_entry(player, tp);
			}
		}
	}
	notify(player, "-------------------------------------------------------------------------------");
}
