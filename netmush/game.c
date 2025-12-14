/**
 * @file game.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Main program and misc functions
 * @version 3.3
 * @date 2021-01-04
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
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
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pcre.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <signal.h>

extern LOGFILETAB logfds_table[];

#ifdef NEED_EMPTY_LTPLSYM
const lt_dlsymlist lt_preloaded_symbols[] = {{0, (lt_ptr_t)0}};
#endif

/*
 * Used to figure out if netmush is already running. Since there's
 * so many differences between sysctl implementation, i prefer to
 * call pgrep. If every system would do like FreeBSD and implement
 * PIDFILE(3), the world would be a better place!
 */

pid_t isrunning(char *pidfile)
{
	FILE *fp;
	pid_t pid = 0, rpid = 0;
	int i = 0;
	char *buff = XMALLOC(MBUF_SIZE, "buff");

	if (mushstate.restarting)
	{
		XFREE(buff);
		return 0;
	}

	fp = fopen(pidfile, "r");

	if (fp == NULL)
	{
		XFREE(buff);
		return 0;
	}

	if (fgets(buff, MBUF_SIZE, fp) == NULL)
	{
		fclose(fp);
		XFREE(buff);
		return 0;
	}

	fclose(fp);
	pid = (pid_t)strtol(buff, (char **)NULL, 10);
	fp = popen("pgrep netmush", "r");

	if (fp == NULL)
	{
		XFREE(buff);
		return 0;
	}

	while (fgets(buff, MBUF_SIZE, fp) != NULL)
	{
		rpid = (pid_t)strtol(buff, (char **)NULL, 10);

		if (pid == rpid)
		{
			i = 1;
			break;
		}
	}

	pclose(fp);

	if (i)
	{
		XFREE(buff);
		return pid;
	}

	XFREE(buff);
	return 0;
}

/*
 * CHeck if a file exist
 */

int fileexist(char *file)
{
	int fp;
	fp = open(file, O_RDONLY);

	if (fp < 0)
	{
		return (0);
	}

	close(fp);
	return (1);
}

void handlestartupflatfiles(int flag)
{
	char *db, *flat, *db_bak, *flat_bak;
	char *ts;
	int i;
	struct stat sb1, sb2;
	ts = mktimestamp();

	db = XASPRINTF("db", "%s/%s", mushconf.dbhome, mushconf.db_file);
	flat = XASPRINTF("flat", "%s/%s.%s", mushconf.bakhome, mushconf.db_file, (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILLED"));
	db_bak = XASPRINTF("db_bak", "%s/%s.%s", mushconf.bakhome, mushconf.db_file, ts);
	flat_bak = XASPRINTF("flat_bak", "%s/%s.%s.%s", mushconf.bakhome, mushconf.db_file, (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILLED"), ts);
	XFREE(ts);
	i = open(flat, O_RDONLY);

	if (i > 0)
	{
		fstat(i, &sb1);
		close(i);
		stat(db, &sb2);

		if (tailfind(flat, "***END OF DUMP***\n"))
		{
			log_write(LOG_ALWAYS, "INI", "LOAD", "A non-corrupt %s file is present.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));

			if (difftime(sb1.st_mtime, sb2.st_mtime) > (double)0)
			{
				log_write(LOG_ALWAYS, "INI", "LOAD", "The %s file is newer than your current database.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));

				if (copy_file(db, db_bak, 1) != 0)
				{
					log_write(LOG_ALWAYS, "INI", "LOAD", "Unable to archive previous db to : %s", db_bak);
				}

				recover_flatfile(flat);

				if (unlink(flat) != 0)
				{
					log_write(LOG_ALWAYS, "INI", "LOAD", "Unable to delete : %s", flat);
				}

				log_write(LOG_ALWAYS, "INI", "LOAD", "Recovery successfull");
			}
			else
			{
				log_write(LOG_ALWAYS, "INI", "LOAD", "The %s file is older than your current database.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));

				if (copy_file(flat, flat_bak, 1) == 0)
				{
					log_write(LOG_ALWAYS, "INI", "LOAD", "Older %s file archived as : %s", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"), flat_bak);
				}
				else
				{
					log_write(LOG_ALWAYS, "INI", "LOAD", "Unable to archive %s file as : %s", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"), flat_bak);
				}
			}
		}
		else
		{
			log_write(LOG_ALWAYS, "INI", "LOAD", "A corrupt %s file is present.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));

			if (!copy_file(flat, flat_bak, 1))
			{
				log_write(LOG_ALWAYS, "INI", "LOAD", "Archived as : %s, using previous db to load", flat_bak);
			}
			else
			{
				log_write(LOG_ALWAYS, "INI", "LOAD", "Unable to archive %s file, using previous db to load", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));
			}
		}
	}

	XFREE(db);
	XFREE(flat);
	XFREE(db_bak);
	XFREE(flat_bak);
}

/*
 * Read the last line of a file and compare
 * with the given key. Return 1 of they
 * match.
 */

int tailfind(char *file, char *key)
{
	int fp;
	char *s = XMALLOC(MBUF_SIZE, "s");
	fp = open(file, O_RDONLY);

	if (fp < 0)
	{
		XFREE(s);
		return 0;
	}

	lseek(fp, 0 - strlen(key), SEEK_END);

	if (read(fp, s, strlen(key)) != -1)
	{
		if (strncmp(s, key, strlen(key)) == 0)
		{
			close(fp);
			XFREE(s);
			return (1);
		}
	}

	close(fp);
	XFREE(s);
	return (0);
}

/*
 * used to allocate storage for temporary stuff, cleared before command
 * execution
 */

void do_dump(dbref player, dbref cause, int key)
{
	if (mushstate.dumping)
	{
		notify(player, "Dumping. Please try again later.");
		return;
	}

	notify(player, "Dumping");
	fork_and_dump(player, cause, key);
}

/*
 * ----------------------------------------------------------------------
 * Hashtable resize.
 */

void do_hashresize(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
	MODULE *mp;
	MODHASHES *m_htab, *hp;
	MODHASHES *m_ntab, *np;
	char *s;
	hashresize(&mushstate.command_htab, 512);
	hashresize(&mushstate.player_htab, 16);
	hashresize(&mushstate.nref_htab, 8);
	hashresize(&mushstate.vattr_name_htab, 256);
	nhashresize(&mushstate.qpid_htab, 256);
	nhashresize(&mushstate.fwdlist_htab, 8);
	nhashresize(&mushstate.propdir_htab, 8);
	nhashresize(&mushstate.redir_htab, 8);
	hashresize(&mushstate.ufunc_htab, 8);
	hashresize(&mushstate.structs_htab, (mushstate.max_structs < 16) ? 16 : mushstate.max_structs);
	hashresize(&mushstate.cdefs_htab, (mushstate.max_cdefs < 16) ? 16 : mushstate.max_cdefs);
	hashresize(&mushstate.instance_htab, (mushstate.max_instance < 16) ? 16 : mushstate.max_instance);
	hashresize(&mushstate.instdata_htab, (mushstate.max_instdata < 16) ? 16 : mushstate.max_instdata);
	nhashresize(&mushstate.objstack_htab, (mushstate.max_stacks < 16) ? 16 : mushstate.max_stacks);
	nhashresize(&mushstate.objgrid_htab, 16);
	hashresize(&mushstate.vars_htab, (mushstate.max_vars < 16) ? 16 : mushstate.max_vars);
	hashresize(&mushstate.api_func_htab, 8);
	s = XMALLOC(MBUF_SIZE, "s");

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, "hashtable");
		m_htab = (MODHASHES *)dlsym(mp->handle, s);

		if (m_htab)
		{
			for (hp = m_htab; hp->tabname != NULL; hp++)
			{
				hashresize(hp->htab, hp->min_size);
			}
		}

		XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, "nhashtable");
		m_ntab = (MODHASHES *)dlsym(mp->handle, s);

		if (m_ntab)
		{
			for (np = m_ntab; np->tabname != NULL; np++)
			{
				nhashresize(np->htab, np->min_size);
			}
		}
	}

	XFREE(s);

	if (!mushstate.restarting)
	{
		notify(player, "Resized.");
	}
}

/*
 * ----------------------------------------------------------------------
 * regexp_match: Load a regular expression match and insert it into
 * registers. PCRE modifications adapted from PennMUSH.
 */

int regexp_match(char *pattern, char *str, int case_opt, char *args[], int nargs)
{
	int i;
	pcre *re;
	const char *errptr;
	int erroffset;
	int offsets[PCRE_MAX_OFFSETS];
	int subpatterns;

	if ((re = pcre_compile(pattern, case_opt, &errptr, &erroffset, mushstate.retabs)) == NULL)
	{
		/*
		 * This is a matching error. We have an error message in
		 * errptr that we can ignore, since we're doing
		 * command-matching.
		 */
		return 0;
	}

	/*
	 * Now we try to match the pattern. The relevant fields will
	 * automatically be filled in by this.
	 */
	if ((subpatterns = pcre_exec(re, NULL, str, strlen(str), 0, 0, offsets, PCRE_MAX_OFFSETS)) < 0)
	{
		pcre_free(re);
		return 0;
	}

	/*
	 * If we had too many subpatterns for the offsets vector, set the
	 * number to 1/3rd of the size of the offsets vector.
	 */
	if (subpatterns == 0)
	{
		subpatterns = PCRE_MAX_OFFSETS / 3;
	}

	/*
	 * Now we fill in our args vector. Note that in regexp matching, 0 is
	 * the entire string matched, and the parenthesized strings go from 1
	 * to 9. We DO PRESERVE THIS PARADIGM, for consistency with other
	 * languages.
	 */

	for (i = 0; i < nargs; i++)
	{
		args[i] = NULL;
	}

	for (i = 0; i < nargs; i++)
	{
		args[i] = XMALLOC(LBUF_SIZE, "args[i]");

		if (pcre_copy_substring(str, offsets, subpatterns, i, args[i], LBUF_SIZE) < 0)
		{
			/*
			 * Match behavior of wild(): clear out null values.
			 */
			XFREE(args[i]);
			args[i] = NULL;
		}
	}

	pcre_free(re);
	return 1;
}

/*
 * ----------------------------------------------------------------------
 * atr_match: Check attribute list for wild card matches and queue them.
 */

int atr_match1(dbref thing, dbref parent, dbref player, char type, char *str, char *raw_str, int check_exclude, int hash_insert)
{
	dbref aowner;
	int match, attr, aflags, alen, i;
	char *buff, *s, *as;
	char *args[NUM_ENV_VARS];
	ATTR *ap;

	/*
	 * See if we can do it.  Silently fail if we can't.
	 */

	if (!could_doit(player, parent, A_LUSE))
	{
		return -1;
	}

	match = 0;
	buff = XMALLOC(LBUF_SIZE, "buff");
	atr_push();

	for (attr = atr_head(parent, &as); attr; attr = atr_next(&as))
	{
		ap = atr_num(attr);

		/*
		 * Never check NOPROG attributes.
		 */

		if (!ap || (ap->flags & AF_NOPROG))
		{
			continue;
		}

		/*
		 * If we aren't the bottom level check if we saw this attr
		 * before.  Also exclude it if the attribute type is PRIVATE.
		 */

		if (check_exclude && ((ap->flags & AF_PRIVATE) || nhashfind(ap->number, &mushstate.parent_htab)))
		{
			continue;
		}

		atr_get_str(buff, parent, attr, &aowner, &aflags, &alen);

		/*
		 * Skip if private and on a parent
		 */

		if (check_exclude && (aflags & AF_PRIVATE))
		{
			continue;
		}

		/*
		 * If we aren't the top level remember this attr so we
		 * exclude it from now on.
		 */

		if (hash_insert)
			nhashadd(ap->number, (int *)&attr, &mushstate.parent_htab);

		/*
		 * Check for the leadin character after excluding the attrib
		 * This lets non-command attribs on the child block commands
		 * on the parent.
		 */

		if ((buff[0] != type) || (aflags & AF_NOPROG))
		{
			continue;
		}

		/*
		 * decode it: search for first un escaped :
		 */

		for (s = buff + 1; *s && ((*s != ':') || (*(s - 1) == '\\')); s++)
			;

		if (!*s)
		{
			continue;
		}

		*s++ = 0;

		if ((!(aflags & (AF_REGEXP | AF_RMATCH)) && wild(buff + 1, ((aflags & AF_NOPARSE) ? raw_str : str), args, NUM_ENV_VARS)) || ((aflags & AF_REGEXP) && regexp_match(buff + 1, ((aflags & AF_NOPARSE) ? raw_str : str), ((aflags & AF_CASE) ? 0 : PCRE_CASELESS), args, NUM_ENV_VARS)) || ((aflags & AF_RMATCH) && register_match(buff + 1, ((aflags & AF_NOPARSE) ? raw_str : str), args, NUM_ENV_VARS)))
		{
			match = 1;

			if (aflags & AF_NOW)
			{
				process_cmdline(thing, player, s, args, NUM_ENV_VARS, NULL);
			}
			else
			{
				wait_que(thing, player, 0, NOTHING, 0, s, args, NUM_ENV_VARS, mushstate.rdata);
			}

			for (i = 0; i < NUM_ENV_VARS; i++)
			{
				if (args[i])
				{
					XFREE(args[i]);
				}
			}
		}
	}

	XFREE(buff);
	atr_pop();
	return (match);
}

int atr_match(dbref thing, dbref player, char type, char *str, char *raw_str, int check_parents)
{
	int match, lev, result, exclude, insert;
	dbref parent;

	/*
	 * If thing is halted, or it doesn't have a COMMANDS flag and we're
	 * we're doing a $-match, don't check it.
	 */

	if (((type == AMATCH_CMD) && !Has_Commands(thing) && mushconf.req_cmds_flag) || Halted(thing))
	{
		return 0;
	}

	/*
	 * If not checking parents, just check the thing
	 */
	match = 0;

	if (!check_parents || Orphan(thing))
		return atr_match1(thing, thing, player, type, str, raw_str, 0, 0);

	/*
	 * Check parents, ignoring halted objects
	 */
	exclude = 0;
	insert = 1;
	nhashflush(&mushstate.parent_htab, 0);
	for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
	{
		if (!Good_obj(Parent(parent)))
		{
			insert = 0;
		}

		result = atr_match1(thing, parent, player, type, str, raw_str, exclude, insert);

		if (result > 0)
		{
			match = 1;
		}
		else if (result < 0)
		{
			return match;
		}

		exclude = 1;
	}
	return match;
}

/*
 * ---------------------------------------------------------------------------
 * notify_check: notifies the object #target of the message msg, and
 * optionally notify the contents, neighbors, and location also.
 */

int check_filter(dbref object, dbref player, int filter, const char *msg)
{
	int aflags, alen;
	dbref aowner;
	char *buf, *nbuf, *cp, *dp, *str;
	pcre *re;
	const char *errptr;
	int len, case_opt, erroffset, subpatterns;
	int offsets[PCRE_MAX_OFFSETS];
	GDATA *preserve;
	buf = atr_pget(object, filter, &aowner, &aflags, &alen);

	if (!*buf)
	{
		XFREE(buf);
		return (1);
	}

	if (!(aflags & AF_NOPARSE))
	{
		preserve = save_global_regs("check_filter.save");
		nbuf = dp = XMALLOC(LBUF_SIZE, "dp");
		str = buf;
		eval_expression_string(nbuf, &dp, object, player, player, EV_FIGNORE | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
		dp = nbuf;
		XFREE(buf);
		restore_global_regs("check_filter.restore", preserve);
	}
	else
	{
		dp = buf;
		nbuf = buf; /* this way, buf will get freed correctly */
	}

	if (!(aflags & AF_REGEXP))
	{
		do
		{
			cp = parse_to(&dp, ',', EV_STRIP);

			if (quick_wild(cp, (char *)msg))
			{
				XFREE(nbuf);
				return (0);
			}
		} while (dp != NULL);
	}
	else
	{
		len = strlen(msg);
		case_opt = (aflags & AF_CASE) ? 0 : PCRE_CASELESS;

		do
		{
			cp = parse_to(&dp, ',', EV_STRIP);
			re = pcre_compile(cp, case_opt, &errptr, &erroffset, mushstate.retabs);

			if (re != NULL)
			{
				subpatterns = pcre_exec(re, NULL, (char *)msg, len, 0, 0, offsets, PCRE_MAX_OFFSETS);

				if (subpatterns >= 0)
				{
					pcre_free(re);
					XFREE(nbuf);
					return (0);
				}

				pcre_free(re);
			}
		} while (dp != NULL);
	}

	XFREE(nbuf);
	return (1);
}

char *add_prefix(dbref object, dbref player, int prefix, const char *msg, const char *dflt)
{
	int aflags, alen;
	dbref aowner;
	char *buf, *nbuf, *cp, *str;
	GDATA *preserve;
	buf = atr_pget(object, prefix, &aowner, &aflags, &alen);

	if (!*buf)
	{
		cp = buf;
		SAFE_LB_STR((char *)dflt, buf, &cp);
	}
	else
	{
		preserve = save_global_regs("add_prefix_save");
		nbuf = cp = XMALLOC(LBUF_SIZE, "cp");
		str = buf;
		eval_expression_string(nbuf, &cp, object, player, player, EV_FIGNORE | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
		XFREE(buf);
		restore_global_regs("add_prefix_restore", preserve);
		buf = nbuf;
	}

	if (cp != buf)
	{
		SAFE_LB_CHR(' ', buf, &cp);
	}

	SAFE_LB_STR((char *)msg, buf, &cp);
	return (buf);
}

char *dflt_from_msg(dbref sender, dbref sendloc)
{
	char *tp, *tbuff;
	tp = tbuff = XMALLOC(LBUF_SIZE, "tbuff");
	SAFE_STRNCAT(tbuff, &tp, (char *)"From ", 5, LBUF_SIZE);

	if (Good_obj(sendloc))
	{
		safe_name(sendloc, tbuff, &tp);
	}
	else
	{
		safe_name(sender, tbuff, &tp);
	}

	SAFE_LB_CHR(',', tbuff, &tp);
	return tbuff;
}

/*
 * Do HTML escaping, converting < to &lt;, etc.  'dest' needs to be allocated
 * & freed by the caller.
 *
 * If you're using this to append to a string, you can pass in the
 * safe_{str|chr} (char **) so we can just do the append directly, saving you
 * an XMALLOC(LBUF_SIZE, )...XFREE().  If you want us to append from the start of
 * 'dest', just pass in a 0 for 'destp'.
 */

void html_escape(const char *src, char *dest, char **destp)
{
	const char *msg_orig;
	char *temp;

	if (destp == 0)
	{
		temp = dest;
		destp = &temp;
	}

	for (msg_orig = src; msg_orig && *msg_orig; msg_orig++)
	{
		switch (*msg_orig)
		{
		case '<':
			SAFE_STRNCAT(dest, destp, "&lt;", 4, LBUF_SIZE);
			break;

		case '>':
			SAFE_STRNCAT(dest, destp, "&gt;", 4, LBUF_SIZE);
			break;

		case '&':
			SAFE_STRNCAT(dest, destp, "&amp;", 5, LBUF_SIZE);
			break;

		case '\"':
			SAFE_STRNCAT(dest, destp, "&quot;", 6, LBUF_SIZE);
			break;

		default:
			SAFE_LB_CHR(*msg_orig, dest, destp);
			break;
		}
	}

	**destp = '\0';
}

void notify_check(dbref target, dbref sender, int key, const char *format, ...)
{
	char *msg = XMALLOC(LBUF_SIZE, "msg");
	char *msg_ns, *mp, *tbuff, *tp, *buff;
	char *args[NUM_ENV_VARS];
	dbref aowner, targetloc, recip, obj;
	int i, nargs, aflags, alen, has_neighbors, pass_listen, herekey;
	int check_listens, pass_uselock, is_audible, will_send;
	FWDLIST *fp;
	NUMBERTAB *np;
	GDATA *preserve;
	va_list ap;
	va_start(ap, format);

	if ((!format || !(*format)))
	{
		/*
		 * @todo: Check if ap is NULL
		 */
		if ((tbuff = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, tbuff, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);

	/*
	 * If speaker is invalid or message is empty, just exit
	 */

	if (!Good_obj(target) || !msg)
	{
		XFREE(msg);
		return;
	}

	/*
	 * Enforce a recursion limit
	 */
	mushstate.ntfy_nest_lev++;

	if (mushstate.ntfy_nest_lev >= mushconf.ntfy_nest_lim)
	{
		mushstate.ntfy_nest_lev--;
		XFREE(msg);
		return;
	}

	/*
	 * If we want NOSPOOF output, generate it.  It is only needed if we
	 * are sending the message to the target object
	 */

	if (key & MSG_ME)
	{
		mp = msg_ns = XMALLOC(LBUF_SIZE, "msg_ns");

		if (Nospoof(target) && (target != sender) && (target != mushstate.curr_enactor) && (target != mushstate.curr_player))
		{
			if (sender != Owner(sender))
			{
				if (sender != mushstate.curr_enactor)
				{
					SAFE_SPRINTF(msg_ns, &mp, "[%s(#%d){%s}<-(#%d)] ", Name(sender), sender, Name(Owner(sender)), mushstate.curr_enactor);
				}
				else
				{
					SAFE_SPRINTF(msg_ns, &mp, "[%s(#%d){%s}] ", Name(sender), sender, Name(Owner(sender)));
				}
			}
			else if (sender != mushstate.curr_enactor)
			{
				SAFE_SPRINTF(msg_ns, &mp, "[%s(#%d)<-(#%d)] ", Name(sender), sender, mushstate.curr_enactor);
			}
			else
			{
				SAFE_SPRINTF(msg_ns, &mp, "[%s(#%d)] ", Name(sender), sender);
			}
		}

		SAFE_LB_STR(msg, msg_ns, &mp);
	}
	else
	{
		msg_ns = NULL;
	}

	/*
	 * msg contains the raw message, msg_ns contains the NOSPOOFed msg
	 */
	s_Accessed(target);
	check_listens = Halted(target) ? 0 : 1;
	herekey = key & (MSG_SPEECH | MSG_MOVE | MSG_PRESENCE);
	will_send = OK_To_Send(sender, target);

	switch (Typeof(target))
	{
	case TYPE_PLAYER:
		if (will_send)
		{
			if (mushconf.have_pueblo == 1)
			{
				if (key & MSG_ME)
				{
					raw_notify(target, NULL, msg_ns);
				}
			}
			else
			{
				if (key & MSG_ME)
				{
					if (key & MSG_HTML)
					{
						raw_notify_html(target, NULL, msg_ns);
					}
					else
					{
						if (Html(target))
						{
							char *msg_ns_escaped;
							msg_ns_escaped = XMALLOC(LBUF_SIZE, "msg_ns_escaped");
							html_escape(msg_ns, msg_ns_escaped, 0);
							raw_notify(target, NULL, msg_ns_escaped);
							XFREE(msg_ns_escaped);
						}
						else
						{
							raw_notify(target, NULL, msg_ns);
						}
					}
				}
			}

			if (!mushconf.player_listen)
			{
				check_listens = 0;
			}
		}
		//[[fallthrough]];
		__attribute__((fallthrough));
	case TYPE_THING:
	case TYPE_ROOM:

		/*
		 * If we're in a pipe, objects can receive raw_notify if
		 * they're not a player (players were already notified
		 * above).
		 */
		if (mushstate.inpipe && !isPlayer(target) && will_send)
		{
			raw_notify(target, NULL, msg_ns);
		}

		/*
		 * Forward puppet message if it is for me
		 */
		has_neighbors = Has_location(target);
		targetloc = where_is(target);
		is_audible = Audible(target);

		if (will_send && (key & MSG_ME) && Puppet(target) && (target != Owner(target)) && ((key & MSG_PUP_ALWAYS) || ((targetloc != Location(Owner(target))) && (targetloc != Owner(target)))))
		{
			tp = tbuff = XMALLOC(LBUF_SIZE, "tbuff");
			safe_name(target, tbuff, &tp);
			SAFE_STRNCAT(tbuff, &tp, (char *)"> ", 2, LBUF_SIZE);
			SAFE_LB_STR(msg_ns, tbuff, &tp);

			/*
			 * Criteria for redirection of a puppet is based on
			 * the "normal" conditions for hearing and not
			 * conditions based on who the target of the
			 * redirection. Use of raw_notify() means that
			 * recursion is avoided.
			 */
			if (H_Redirect(target))
			{
				np = (NUMBERTAB *)nhashfind(target, &mushstate.redir_htab);

				if (np && Good_obj(np->num))
				{
					raw_notify(Owner(np->num), NULL, tbuff);
				}
			}
			else
			{
				raw_notify(Owner(target), NULL, tbuff);
			}

			XFREE(tbuff);
		}

		/*
		 * Make sure that we're passing an empty set of global
		 * registers to the evaluations we are going to run. We are
		 * specifically not calling a save, since that doesn't empty
		 * the registers.
		 */
		preserve = mushstate.rdata;
		mushstate.rdata = NULL;
		/*
		 * Check for @Listen match if it will be useful
		 */
		pass_listen = 0;
		nargs = 0;

		if (will_send && check_listens && (key & (MSG_ME | MSG_INV_L)) && H_Listen(target))
		{
			tp = atr_get(target, A_LISTEN, &aowner, &aflags, &alen);

			if (*tp && ((!(aflags & AF_REGEXP) && wild(tp, msg, args, NUM_ENV_VARS)) || ((aflags & AF_REGEXP) && regexp_match(tp, msg, ((aflags & AF_CASE) ? 0 : PCRE_CASELESS), args, NUM_ENV_VARS))))
			{
				for (nargs = NUM_ENV_VARS; nargs && (!args[nargs - 1] || !(*args[nargs - 1])); nargs--)
					;

				pass_listen = 1;
			}

			XFREE(tp);
		}

		/*
		 * If we matched the @listen or are monitoring, check the * *
		 * USE lock
		 */
		pass_uselock = 0;

		if (will_send && (key & MSG_ME) && check_listens && (pass_listen || Monitor(target)))
		{
			pass_uselock = could_doit(sender, target, A_LUSE);
		}

		/*
		 * Process AxHEAR if we pass LISTEN, USElock and it's for me
		 */

		if (will_send && (key & MSG_ME) && pass_listen && pass_uselock)
		{
			if (sender != target)
				did_it(sender, target, A_NULL, NULL, A_NULL, NULL, A_AHEAR, 0, args, nargs, 0);
			else
				did_it(sender, target, A_NULL, NULL, A_NULL, NULL, A_AMHEAR, 0, args, nargs, 0);

			did_it(sender, target, A_NULL, NULL, A_NULL, NULL, A_AAHEAR, 0, args, nargs, 0);
		}

		/*
		 * Get rid of match arguments. We don't need them any more
		 */

		if (pass_listen)
		{
			for (i = 0; i < nargs; i++)
				if (args[i] != NULL)
				{
					XFREE(args[i]);
				}
		}

		/*
		 * Process ^-listens if for me, MONITOR, and we pass UseLock
		 */

		if (will_send && (key & MSG_ME) && pass_uselock && (sender != target) && Monitor(target))
		{
			(void)atr_match(target, sender, AMATCH_LISTEN, msg, msg, 0);
		}

		/*
		 * Deliver message to forwardlist members. No presence
		 * control is done on forwarders; if the target can get it,
		 * so can they.
		 */

		if (will_send && (key & MSG_FWDLIST) && Audible(target) && H_Fwdlist(target) && check_filter(target, sender, A_FILTER, msg))
		{
			tbuff = dflt_from_msg(sender, target);
			buff = add_prefix(target, sender, A_PREFIX, msg, tbuff);
			XFREE(tbuff);
			fp = fwdlist_get(target);

			if (fp)
			{
				for (i = 0; i < fp->count; i++)
				{
					recip = fp->data[i];

					if (!Good_obj(recip) || (recip == target))
					{
						continue;
					}

					notify_check(recip, sender, (MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE), NULL, buff);
				}
			}

			XFREE(buff);
		}

		/*
		 * Deliver message through audible exits. If the exit can get
		 * it, we don't do further checking for whatever is beyond
		 * it. Otherwise we have to continue checking.
		 */

		if (will_send && (key & MSG_INV_EXITS))
		{
			for (obj = Exits(target); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
			{
				recip = Location(obj);

				if (Audible(obj) && ((recip != target) && check_filter(obj, sender, A_FILTER, msg)))
				{
					buff = add_prefix(obj, target, A_PREFIX, msg, "From a distance,");
					notify_check(recip, sender, MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | (OK_To_Send(sender, obj) ? 0 : herekey), NULL, buff);
					XFREE(buff);
				}
			}
		}

		/*
		 * Deliver message through neighboring audible exits. Note
		 * that the target doesn't have to hear it in order for us to
		 * do this check. If the exit can get it, we don't do further
		 * checking for whatever is beyond it. Otherwise we have to
		 * continue checking.
		 */

		if (has_neighbors && ((key & MSG_NBR_EXITS) || ((key & MSG_NBR_EXITS_A) && is_audible)))
		{
			/*
			 * If from inside, we have to add the prefix string
			 * of the container.
			 */
			if (key & MSG_S_INSIDE)
			{
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX, msg, tbuff);
				XFREE(tbuff);
			}
			else
			{
				buff = msg;
			}

			for (obj = Exits(Location(target)); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
			{
				recip = Location(obj);

				if (Good_obj(recip) && Audible(obj) && (recip != targetloc) && (recip != target) && check_filter(obj, sender, A_FILTER, msg))
				{
					tbuff = add_prefix(obj, target, A_PREFIX, buff, "From a distance,");
					notify_check(recip, sender, MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | (OK_To_Send(sender, obj) ? 0 : herekey), NULL, tbuff);
					XFREE(tbuff);
				}
			}

			if (key & MSG_S_INSIDE)
			{
				XFREE(buff);
			}
		}

		if (Bouncer(target))
		{
			pass_listen = 1;
		}

		/*
		 * Deliver message to contents only if target passes check.
		 * But things within it must still pass the check.
		 */

		if (will_send && ((key & MSG_INV) || ((key & MSG_INV_L) && pass_listen && check_filter(target, sender, A_INFILTER, msg))))
		{
			/*
			 * Don't prefix the message if we were given the
			 * MSG_NOPREFIX key.
			 */
			if (key & MSG_S_OUTSIDE)
			{
				buff = add_prefix(target, sender, A_INPREFIX, msg, "");
			}
			else
			{
				buff = msg;
			}

			for (obj = Contents(target); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
			{
				if (obj != target)
				{
					if (mushconf.have_pueblo == 1)
					{
						notify_check(obj, sender, MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | (key & MSG_HTML) | herekey, NULL, buff);
					}
					else
					{
						notify_check(obj, sender, MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | herekey, NULL, buff);
					}
				}
			}

			if (key & MSG_S_OUTSIDE)
			{
				XFREE(buff);
			}
		}

		/*
		 * Deliver message to neighbors
		 */

		if (has_neighbors && ((key & MSG_NBR) || ((key & MSG_NBR_A) && is_audible && check_filter(target, sender, A_FILTER, msg))))
		{
			if (key & MSG_S_INSIDE)
			{
				buff = add_prefix(target, sender, A_PREFIX, msg, "");
			}
			else
			{
				buff = msg;
			}

			for (obj = Contents(targetloc); (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
			{
				if ((obj != target) && (obj != targetloc))
				{
					notify_check(obj, sender, MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | herekey, NULL, buff);
				}
			}

			if (key & MSG_S_INSIDE)
			{
				XFREE(buff);
			}
		}

		/*
		 * Deliver message to container
		 */

		if (has_neighbors && ((key & MSG_LOC) || ((key & MSG_LOC_A) && is_audible && check_filter(target, sender, A_FILTER, msg))))
		{
			if (key & MSG_S_INSIDE)
			{
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX, msg, tbuff);
				XFREE(tbuff);
			}
			else
			{
				buff = msg;
			}

			notify_check(targetloc, sender, MSG_ME | MSG_F_UP | MSG_S_INSIDE | herekey, NULL, buff);

			if (key & MSG_S_INSIDE)
			{
				XFREE(buff);
			}
		}

		/*
		 * mushstate.rdata should be empty, but empty it just in case
		 */

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

	if (msg_ns)
	{
		XFREE(msg_ns);
	}

	XFREE(msg);

	mushstate.ntfy_nest_lev--;
}

void notify_except(dbref loc, dbref player, dbref exception, int flags, const char *format, ...)
{
	dbref first;
	char *msg = XMALLOC(LBUF_SIZE, "msg");
	char *s;
	va_list ap;
	va_start(ap, format);

	if ((!format || !*format))
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, s, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);

	if (loc != exception)
	{
		notify_check(loc, player, (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A | flags), NULL, msg);
	}

	for (first = Contents(loc); (first != NOTHING) && (Next(first) != first); first = Next(first))
	{
		if (first != exception)
		{
			notify_check(first, player, (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | flags), NULL, msg);
		}
	}
	XFREE(msg);
}

void notify_except2(dbref loc, dbref player, dbref exc1, dbref exc2, int flags, const char *format, ...)
{
	dbref first;
	char *msg = XMALLOC(LBUF_SIZE, "msg");
	char *s;
	va_list ap;
	va_start(ap, format);

	if ((!format || !*format))
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, s, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);

	if ((loc != exc1) && (loc != exc2))
	{
		notify_check(loc, player, (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A | flags), NULL, msg);
	}

	for (first = Contents(loc); (first != NOTHING) && (Next(first) != first); first = Next(first))
	{
		if (first != exc1 && first != exc2)
		{
			notify_check(first, player, (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | flags), NULL, msg);
		}
	}
	XFREE(msg);
}

/*
 * ----------------------------------------------------------------------
 * Reporting of CPU information.
 */

void report_timecheck(dbref player, int yes_screen, int yes_log, int yes_clear)
{
	int thing, obj_counted;
	long used_msecs, total_msecs;
	struct timeval obj_time;
	char *pname;

	if (mushconf.lag_check_clk)
	{
		pname = log_getname(player);

		if (!(yes_log && (LOG_TIMEUSE & mushconf.log_options) != 0))
		{
			yes_log = 0;
			log_write(LOG_ALWAYS, "WIZ", "TIMECHECK", "%s checks object time use over %d seconds\n", pname, (int)(time(NULL) - mushstate.cpu_count_from));
		}
		else
		{
			log_write(LOG_ALWAYS, "OBJ", "CPU", "%s checks object time use over %d seconds\n", pname, (int)(time(NULL) - mushstate.cpu_count_from));
		}

		XFREE(pname);
		obj_counted = 0;
		total_msecs = 0;
		/*
		 * Step through the db. Care only about the ones that are nonzero.
		 * And yes, we violate several rules of good programming practice by
		 * failing to abstract our log calls. Oh well.
		 */
		for (thing = 0; thing < mushstate.db_top; thing++)
		{
			obj_time = Time_Used(thing);

			if (obj_time.tv_sec || obj_time.tv_usec)
			{
				obj_counted++;
				used_msecs = (obj_time.tv_sec * 1000) + (obj_time.tv_usec / 1000);
				total_msecs += used_msecs;

				if (yes_log)
				{
					log_write(LOG_ALWAYS, "OBJ", "CPU", "#%d\t%ld\n", thing, used_msecs);
				}

				if (yes_screen)
				{
					raw_notify(player, "#%d\t%ld", thing, used_msecs);
				}

				if (yes_clear)
				{
					obj_time.tv_usec = obj_time.tv_sec = 0;
				}
			}

			db[thing].cpu_time_used.tv_sec = obj_time.tv_sec;
			db[thing].cpu_time_used.tv_usec = obj_time.tv_usec;
		}

		if (yes_screen)
		{
			raw_notify(player, "Counted %d objects using %ld msecs over %d seconds.", obj_counted, total_msecs, (int)(time(NULL) - mushstate.cpu_count_from));
		}

		if (yes_log)
		{
			log_write(LOG_ALWAYS, "OBJ", "CPU", "Counted %d objects using %ld msecs over %d seconds.", obj_counted, total_msecs, (int)(time(NULL) - mushstate.cpu_count_from));
		}

		if (yes_clear)
		{
			mushstate.cpu_count_from = time(NULL);
		}
	}
	else
	{
		raw_notify(player, NULL, "Sorry, this command has been disabled.");
	}
}

void do_timecheck(dbref player, dbref cause __attribute__((unused)), int key)
{
	int yes_screen, yes_log, yes_clear;
	yes_screen = yes_log = yes_clear = 0;

	if (key == 0)
	{
		/*
		 * No switches, default to printing to screen and clearing
		 * counters
		 */
		yes_screen = 1;
		yes_clear = 1;
	}
	else
	{
		if (key & TIMECHK_RESET)
		{
			yes_clear = 1;
		}

		if (key & TIMECHK_SCREEN)
		{
			yes_screen = 1;
		}

		if (key & TIMECHK_LOG)
		{
			yes_log = 1;
		}
	}

	report_timecheck(player, yes_screen, yes_log, yes_clear);
}

/*
 * ----------------------------------------------------------------------
 * Miscellaneous startup/stop routines.
 */

char **add_array(char **b, char *s, int *i)
{
	b = (char **)XREALLOC(b, (*i + 1) * sizeof(char *), "b");

	if (s != NULL)
	{
		b[*i] = XSTRDUP(s, "b[*i]");
	}
	else
	{
		b[*i] = NULL;
	}

	*i = *i + 1;
	return (b);
}

int backup_copy(char *src, char *dst, int flag)
{
	char *fn;
	int i;
	/*
	 * Copy or move a file to dst directory
	 */
	fn = XASPRINTF("fn", "%s/%s", realpath(dst, NULL), basename(src));
	i = copy_file(src, fn, flag);
	XFREE(fn);
	return (i);
}

char *mktimestamp(void)
{
	struct tm *t;
	time_t ts;
	ts = time(NULL);
	t = localtime(&ts);
	return XASPRINTF("buff", "%04d%02d%02d-%02d%02d%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
}

void do_backup_mush(dbref player, dbref cause, int key)
{
	backup_mush(player, cause, key);
}

int backup_mush(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
	int i, txt_n = 0, cnf_n = 0, dbf_n = 0;
	char **txt = NULL, **cnf = NULL, **dbf = NULL;
	char *tmpdir, *s, *buff, *tb, *cwd, *ts;
	char *s1 = XMALLOC(MBUF_SIZE, "s1");
	FILE *fp = NULL;
	MODULE *mp;

	if (player != NOTHING)
	{
		raw_broadcast(0, "GAME: Backup in progress. Game may freeze for a few minutes.");
	}

	log_write(LOG_ALWAYS, "BCK", "INFO", "Getting list of files to backup");

	if (player != NOTHING)
	{
		notify(player, "Getting list of files to backup");
	}

	/*
	 * First, get a list of all our text files
	 */

	for (i = 0; i < mushstate.helpfiles; i++)
	{
		XSNPRINTF(s1, MBUF_SIZE, "%s.txt", mushstate.hfiletab[i]);
		txt = add_array(txt, s1, &txt_n);
	}

	txt = add_array(txt, mushconf.guest_file, &txt_n);
	txt = add_array(txt, mushconf.conn_file, &txt_n);
	txt = add_array(txt, mushconf.creg_file, &txt_n);
	txt = add_array(txt, mushconf.regf_file, &txt_n);
	txt = add_array(txt, mushconf.motd_file, &txt_n);
	txt = add_array(txt, mushconf.wizmotd_file, &txt_n);
	txt = add_array(txt, mushconf.quit_file, &txt_n);
	txt = add_array(txt, mushconf.down_file, &txt_n);
	txt = add_array(txt, mushconf.full_file, &txt_n);
	txt = add_array(txt, mushconf.site_file, &txt_n);
	txt = add_array(txt, mushconf.crea_file, &txt_n);

	if (mushconf.have_pueblo == 1)
	{
		txt = add_array(txt, mushconf.htmlconn_file, &txt_n);
	}

	/*
	 * Next, get a list of all our config files
	 */
	for (i = 0; i < mushstate.configfiles; i++)
	{
		cnf = add_array(cnf, mushstate.cfiletab[i], &cnf_n);
	}

	log_write(LOG_ALWAYS, "BCK", "INFO", "Making sure flatfiles are up to date");

	if (player != NOTHING)
	{
		notify(player, "Making sure flatfiles are up to date");
	}

	/*
	 * Next. get a list of all our module files
	 */

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.bakhome, mushconf.mush_shortname, mp->modname);

		if (mp->db_write_flatfile)
		{
			fp = db_module_flatfile(s, 1);

			if (fp)
			{
				dbf = add_array(dbf, s, &dbf_n);
				(*(mp->db_write_flatfile))(fp);
				tf_fclose(fp);
			}
		}

		if (mp->dump_database)
		{
			fp = db_module_flatfile(s, 1);

			if (fp)
			{
				dbf = add_array(dbf, s, &dbf_n);
				(*(mp->dump_database))(fp);
				tf_fclose(fp);
			}
		}

		XFREE(s);
	}

	/* Finally Dump our flatfile */
	s = XASPRINTF("s", "%s/%s.FLAT", mushconf.bakhome, mushconf.db_file);
	log_write(LOG_ALWAYS, "DMP", "DUMP", "Writing db: %s", s);
	pcache_sync();
	cache_sync();
	fp = tf_fopen(s, O_WRONLY | O_CREAT | O_TRUNC);

	if (fp != NULL)
	{
		dbf = add_array(dbf, s, &dbf_n);
		db_write_flatfile(fp, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
		tf_fclose(fp);
	}
	else
	{
		log_write(LOG_ALWAYS, "BCK", "INFO", "Unable to create MUSH's flatfile");

		if (player != NOTHING)
		{
			notify(player, "Unable to create MUSH's flatfile");
			raw_broadcast(0, "GAME: Backup finished.");
		}
	}

	XFREE(s);
	/*
	 * End our argument list
	 */
	txt = add_array(txt, NULL, &txt_n);
	cnf = add_array(cnf, NULL, &cnf_n);
	dbf = add_array(dbf, NULL, &dbf_n);
	log_write(LOG_ALWAYS, "BCK", "INFO", "Found %d text files to backup", txt_n - 1);
	log_write(LOG_ALWAYS, "BCK", "INFO", "Found %d config files to backup", cnf_n - 1);
	log_write(LOG_ALWAYS, "BCK", "INFO", "Found %d db files to backup", dbf_n - 1);

	if (player != NOTHING)
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Found, %d text files, %d config files and %d db files to backup", txt_n - 1, cnf_n - 1, dbf_n - 1);
	}

	/* We have everything we need to backup, create a temp directory */
	tmpdir = XASPRINTF("tmpdir", "%s/backup.XXXXXX", mushconf.bakhome);

	if ((mkdtemp(tmpdir)) == NULL)
	{
		log_write(LOG_ALWAYS, "BCK", "MKDIR", "Unable to create temp directory");

		if (player != NOTHING)
		{
			notify(player, "Backup abort, Unable to create temp directory");
			raw_broadcast(0, "GAME: Backup finished.");
		}

		XFREE(tmpdir);
		XFREE(s1);
		return (-1);
	}

	log_write(LOG_ALWAYS, "BCK", "INFO", "Creating backup set");

	if (player != NOTHING)
	{
		notify(player, "Creating backup set");
	}

	/* Copy files to our backup directory */

	for (i = 0; txt[i] != NULL; i++)
	{
		backup_copy(txt[i], tmpdir, 0);
	}

	for (i = 0; cnf[i] != NULL; i++)
	{
		backup_copy(cnf[i], tmpdir, 0);
	}

	/* We can move the flatfiles there */
	for (i = 0; dbf[i] != NULL; i++)
	{
		backup_copy(dbf[i], tmpdir, 1);
	}

	/* Create our backup config file */
	s = XASPRINTF("s", "%s/netmush.backup", tmpdir);

	if ((fp = fopen(s, "w")) != NULL)
	{
		fprintf(fp, "version\t%d\n", BACKUP_VERSION);
		fprintf(fp, "texts\t");

		for (i = 0; txt[i] != NULL; i++)
		{
			if (txt[i + 1] != NULL)
			{
				fprintf(fp, "%s ", basename(txt[i]));
			}
			else
			{
				fprintf(fp, "%s", basename(txt[i]));
			}
		}

		fprintf(fp, "\nconfigs\t");

		for (i = 0; cnf[i] != NULL; i++)
		{
			if (cnf[i + 1] != NULL)
			{
				fprintf(fp, "%s ", basename(cnf[i]));
			}
			else
			{
				fprintf(fp, "%s", basename(cnf[i]));
			}
		}

		fprintf(fp, "\ndatabases\t");

		for (i = 0; dbf[i] != NULL; i++)
		{
			if (dbf[i + 1] != NULL)
			{
				fprintf(fp, "%s ", basename(dbf[i]));
			}
			else
			{
				fprintf(fp, "%s", basename(dbf[i]));
			}
		}

		fprintf(fp, "\n");
		fclose(fp);
	}

	XFREE(s);
	/* Call our external utility to pack everything together */
	ts = mktimestamp();
	s = XASPRINTF("s", "%s %s %s/%s_%s.%s * 2>&1", mushconf.backup_exec, mushconf.backup_compress, mushconf.bakhome, mushconf.mush_shortname, ts, mushconf.backup_ext);
	XFREE(ts);
	cwd = getcwd(NULL, 0);

	if (cwd == NULL)
	{
		log_write(LOG_ALWAYS, "BCK", "GETCD", "Unable to get the current working directory");

		if (player != NOTHING)
		{
			notify(player, "Unable to get the current working directory");
			raw_broadcast(0, "GAME: Backup finished.");
		}

		XFREE(s);
		XFREE(s1);
		return -1;
	}

	if (chdir(tmpdir) == -1)
	{
		log_write(LOG_ALWAYS, "BCK", "SETCD", "Unable to set the working directory (%s)", tmpdir);

		if (player != NOTHING)
		{
			notify(player, "Unable to set the working directory");
			raw_broadcast(0, "GAME: Backup finished.");
		}

		XFREE(s);
		XFREE(s1);
		return -1;
	}

	buff = XMALLOC(MBUF_SIZE, "buff");
	log_write(LOG_ALWAYS, "BCK", "RUN", "Executing external command %s", s);

	if (player != NOTHING)
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Executing external command %s", s);
	}

	if ((fp = popen(s, "r")) != NULL)
	{
		while (fgets(buff, MBUF_SIZE, fp) != NULL)
		{
			if ((tb = strchr(buff, '\n')))
			{
				tb[0] = 0x00;
			}

			if ((tb = strchr(buff, '\r')))
			{
				tb[0] = 0x00;
			}

			log_write(LOG_ALWAYS, "BCK", "RUN", "%s", buff);
		}

		pclose(fp);
		log_write(LOG_ALWAYS, "BCK", "RUN", "External command done");

		if (player != NOTHING)
		{
			notify(player, "External command done");
		}
	}
	else
	{
		log_write(LOG_ALWAYS, "BCK", "RUN", "Unable to run external command");

		if (player != NOTHING)
		{
			notify(player, "Unable to run external command");
		}
	}

	if (chdir(cwd) == -1)
	{
		log_write(LOG_ALWAYS, "BCK", "SETCD", "Unable to restore the working directory");

		if (player != NOTHING)
		{
			notify(player, "Unable to restore the working directory");
		}
	}

	XFREE(cwd);
	XFREE(buff);
	XFREE(s);
	/* Cleanup */
	log_write(LOG_ALWAYS, "BCK", "INFO", "Cleaning up");

	if (player != NOTHING)
	{
		notify(player, "Cleaning up");
	}

	for (i = 0; txt[i] != NULL; i++)
	{
		s = XASPRINTF("s", "%s/%s", tmpdir, basename(txt[i]));

		if (unlink(s) == -1)
		{
			log_write(LOG_ALWAYS, "BCK", "UNLK", "Unable to remove file %s", s);

			if (player != NOTHING)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unable to remove file %s", s);
			}
		}

		XFREE(txt[i]);
		XFREE(s);
	}

	XFREE(txt);

	for (i = 0; cnf[i] != NULL; i++)
	{
		s = XASPRINTF("s", "%s/%s", tmpdir, basename(cnf[i]));

		if (unlink(s) == -1)
		{
			log_write(LOG_ALWAYS, "BCK", "UNLK", "Unable to remove file %s", s);

			if (player != NOTHING)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unable to remove file %s", s);
			}
		}

		XFREE(cnf[i]);
		XFREE(s);
	}

	XFREE(cnf);

	for (i = 0; dbf[i] != NULL; i++)
	{
		s = XASPRINTF("s", "%s/%s", tmpdir, basename(dbf[i]));

		if (unlink(s) == -1)
		{
			log_write(LOG_ALWAYS, "BCK", "UNLK", "Unable to remove file %s", s);

			if (player != NOTHING)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unable to remove file %s", s);
			}
		}

		XFREE(dbf[i]);
		XFREE(s);
	}

	XFREE(dbf);
	s = XASPRINTF("s", "%s/netmush.backup", tmpdir);

	if (unlink(s) == -1)
	{
		log_write(LOG_ALWAYS, "BCK", "UNLK", "Unable to remove file %s", s);

		if (player != NOTHING)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unable to remove file %s", s);
		}
	}

	XFREE(s);

	if (rmdir(tmpdir) == -1)
	{
		log_write(LOG_ALWAYS, "BCK", "RMDIR", "Unable to remove directory %s", tmpdir);

		if (player != NOTHING)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unable to remove directory %s", tmpdir);
		}
	}

	XFREE(tmpdir);
	log_write(LOG_ALWAYS, "BCK", "INFO", "Backup done");

	if (player != NOTHING)
	{
		raw_broadcast(0, "GAME: Backup finished.");
	}

	XFREE(s1);
	return 0;
}

int copy_file(char *src, char *dst, int flag)
{
	FILE *fsrc, *fdst;
	ssize_t size, wsize;
	char *buff;
	/*
	 * Rename could work, but only within the same filesystem.
	 * It's slower but safer. If flag is set, delete src (move file)
	 */
	buff = XMALLOC(LBUF_SIZE, "buff");

	if ((fsrc = fopen(src, "r")) == NULL)
	{
		return (-1);
	}

	unlink(dst);

	if ((fdst = fopen(dst, "w")) == NULL)
	{
		return (-1);
	}

	while ((size = fread(buff, sizeof(char), LBUF_SIZE, fsrc)) > 0)
	{
		wsize = fwrite(buff, sizeof(char), size, fdst);

		if (wsize != size)
		{
			break;
		}
	}

	fclose(fsrc);
	fclose(fdst);
	XFREE(buff);

	if (flag)
	{
		unlink(src);
	}

	return (0);
}

pid_t write_pidfile(char *fn)
{
	FILE *f;
	pid_t pid;

	pid = getpid();

	if ((f = fopen(fn, "w")) != NULL)
	{
		fprintf(f, "%d\n", pid);
		fclose(f);
	}
	else
	{
		log_write(LOG_ALWAYS, "PID", "FAIL", "Failed to write pidfile %s\n", fn);
	}

	return (pid);
}

FILE *fmkstemp(char *template)
{
	FILE *fp;
	int fd = -1;

	if ((fd = mkstemp(template)) == -1 || (fp = fdopen(fd, "w+")) == NULL)
	{
		if (fd != -1)
		{
			unlink(template);
			close(fd);
		}

		return (NULL);
	}

	return (fp);
}

void write_status_file(dbref player, char *message)
{
	int fd, size;
	char *msg;
	fd = tf_open(mushconf.status_file, O_RDWR | O_CREAT | O_TRUNC);

	if (player != NOTHING)
	{
		msg = XASPRINTF("msg", "Shutdown by : %s\n", Name(Owner(player)));
	}
	else
	{
		msg = XSTRDUP("Shutdown by : System\n", "write_status_file");
	}

	size = write(fd, msg, strlen(msg));
	XFREE(msg);

	if (size < 0)
	{
		log_write(LOG_ALWAYS, "WIZ", "WRSTF", "Error while writing to status file");
	}

	if (message && *message)
	{
		msg = XASPRINTF("msg", "Status : %s\n", message);
		size = write(fd, msg, strlen(msg));
		log_write(LOG_ALWAYS, "WIZ", "WRSTF", "Shutdown status: %s", message);

		if (size < 0)
		{
			log_write(LOG_ALWAYS, "WIZ", "WRSTF", "Error while writing to status file");
		}

		XFREE(msg);
	}

	tf_close(fd);
}

void do_shutdown(dbref player, dbref cause __attribute__((unused)), int key, char *message)
{
	char *name;
	name = log_getname(player);

	if (key & SHUTDN_COREDUMP)
	{
		if (player != NOTHING)
		{
			raw_broadcast(0, "GAME: Aborted by %s", Name(Owner(player)));
			log_write(LOG_ALWAYS, "WIZ", "SHTDN", "Abort and coredump by %s", name);
		}

		write_status_file(player, "Abort and coredump");
		/*
		 * Don't bother to even shut down the network or dump.
		 */
		/*
		 * Die. Die now.
		 */
		abort();
	}

	if (mushstate.dumping)
	{
		notify(player, "Dumping. Please try again later.");
		XFREE(name);
		return;
	}

	do_dbck(NOTHING, NOTHING, 0); /* dump consistent state */

	if (player != NOTHING)
	{
		if (message && *message)
		{
			raw_broadcast(0, "GAME: Shutdown by %s: %s", Name(Owner(player)), message);
		}
		else
		{
			raw_broadcast(0, "GAME: Shutdown by %s", Name(Owner(player)));
		}

		log_write(LOG_ALWAYS, "WIZ", "SHTDN", "Shutdown by %s", name);
	}
	else
	{
		raw_broadcast(0, "GAME: Fatal Error: %s", message);
		log_write(LOG_ALWAYS, "WIZ", "SHTDN", "Fatal error: %s", message);
	}

	write_status_file(player, message);
	/*
	 * Set up for normal shutdown
	 */
	mushstate.shutdown_flag = 1;
	XFREE(name);
	return;
}

void dump_database_internal(int dump_type)
{
	char *tmpfile = XMALLOC(MAXPATHLEN, "tmpfile");
	char *s;
	/* *c; */
	FILE *f = NULL;
	MODULE *mp;
	/*
	 * Call modules to write to DBM
	 */
	db_lock();

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->db_write)
		{
			(*(cam__mp->db_write))();
		}
	}

	db_unlock();

	switch (dump_type)
	{
	case DUMP_DB_CRASH:
		XSPRINTF(tmpfile, "%s/%s.CRASH", mushconf.bakhome, mushconf.db_file);
		unlink(tmpfile);
		f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);

		if (f != NULL)
		{
			db_write_flatfile(f, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			tf_fclose(f);
		}
		else
		{
			log_perror("DMP", "FAIL", "Opening crash file", tmpfile);
		}

		break;

	case DUMP_DB_RESTART:
		db_write();
		break;

	case DUMP_DB_FLATFILE:

		/*
		 * Trigger modules to write their flat-text dbs
		 */
		for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
		{
			s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);

			if (mp->db_write_flatfile)
			{
				f = db_module_flatfile(s, 1);

				if (f)
				{
					(*(mp->db_write_flatfile))(f);
					tf_fclose(f);
				}
			}

			if (mp->dump_database)
			{
				f = db_module_flatfile(s, 1);

				if (f)
				{
					(*(mp->dump_database))(f);
					tf_fclose(f);
				}
			}

			XFREE(s);
		}

		/*
		 * Write the game's flatfile
		 */
		XSPRINTF(tmpfile, "%s/%s.FLAT", mushconf.bakhome, mushconf.db_file);
		f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);

		if (f != NULL)
		{
			db_write_flatfile(f, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			tf_fclose(f);
		}
		else
		{
			log_perror("DMP", "FAIL", "Opening flatfile", tmpfile);
		}

		break;

	case DUMP_DB_KILLED:
		XSPRINTF(tmpfile, "%s/%s.KILLED", mushconf.bakhome, mushconf.db_file);
		f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);

		if (f != NULL)
		{
			/*
			 * Write a flatfile
			 */
			db_write_flatfile(f, F_TINYMUSH, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			tf_fclose(f);
		}
		else
		{
			log_perror("DMP", "FAIL", "Opening killed file", tmpfile);
		}

		break;

	default:
		db_write();
	}

	if (dump_type != DUMP_DB_FLATFILE)
	{
		/*
		 * Call modules to write to their flat-text database
		 */
		for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
		{
			s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);

			if (mp->db_write_flatfile)
			{
				f = db_module_flatfile(s, 1);

				if (f)
				{
					(*(mp->db_write_flatfile))(f);
					tf_fclose(f);
				}
			}

			if (mp->dump_database)
			{
				f = db_module_flatfile(s, 1);

				if (f)
				{
					(*(mp->dump_database))(f);
					tf_fclose(f);
				}
			}

			XFREE(s);
		}
	}
	XFREE(tmpfile);
}

void dump_database(void)
{
	mushstate.epoch++;
	mushstate.dumping = 1;
	log_write(LOG_DBSAVES, "DMP", "DUMP", "Dumping: %s.#%d#", mushconf.db_file, mushstate.epoch);
	pcache_sync();
	cache_sync();
	dump_database_internal(DUMP_DB_NORMAL);
	log_write(LOG_DBSAVES, "DMP", "DONE", "Dump complete: %s.#%d#", mushconf.db_file, mushstate.epoch);
	mushstate.dumping = 0;
}

void fork_and_dump(dbref player, dbref cause __attribute__((unused)), int key)
{
	if (mushconf.dump_msg)
	{
		raw_broadcast(0, "%s", mushconf.dump_msg);
	}

	mushstate.epoch++;
	mushstate.dumping = 1;

	if (!key || (key & DUMP_TEXT))
	{
		log_write(LOG_DBSAVES, "DMP", "CHKPT", "SYNCing");
	}

	if (!key || (key & DUMP_STRUCT) || (key & DUMP_FLATFILE))
	{
		log_write(LOG_DBSAVES, "DMP", "CHKPT", "Checkpointing: %s.#%d#", mushconf.db_file, mushstate.epoch);
	}

	al_store(); /* Save cached modified attribute list */

	if (!key || (key & DUMP_TEXT))
	{
		pcache_sync();
	}

	if (!(key & DUMP_FLATFILE))
	{
		cache_sync();

		if ((key & DUMP_OPTIMIZE) || (mushconf.dbopt_interval && (mushstate.epoch % mushconf.dbopt_interval == 0)))
		{
			dddb_optimize();
		}
	}

	if (!key || (key & DUMP_STRUCT) || (key & DUMP_FLATFILE))
	{
		if (mushconf.fork_dump)
		{
			mushstate.dumper = fork();
		}
		else
		{
			mushstate.dumper = 0;
		}

		if (mushstate.dumper == 0)
		{
			if (key & DUMP_FLATFILE)
			{
				dump_database_internal(DUMP_DB_FLATFILE);
			}
			else
			{
				dump_database_internal(DUMP_DB_NORMAL);
			}

			if (mushconf.fork_dump)
			{
				_exit(EXIT_SUCCESS);
			}
		}
		else if (mushstate.dumper < 0)
		{
			log_perror("DMP", "FORK", NULL, "fork()");
		}
	}

	if (mushstate.dumper <= 0 || kill(mushstate.dumper, 0) == -1)
	{
		mushstate.dumping = 0;
		mushstate.dumper = 0;
	}

	if (mushconf.postdump_msg)
	{
		if (*mushconf.postdump_msg)
		{
			raw_broadcast(0, "%s", mushconf.postdump_msg);
		}
	}

	if (!Quiet(player) && (player != NOTHING))
	{
		notify(player, "Done");
	}
}

void call_all_modules_nocache(char *xfn)
{
	MODULE *mp;
	void (*ip)(void);
	char *s;
	s = XMALLOC(MBUF_SIZE, "s");

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, xfn);

		if ((ip = (void (*)(void))dlsym(mp->handle, s)) != NULL)
		{
			(*ip)();
		}
	}

	XFREE(s);
}

int load_game(void)
{
	FILE *f;
	MODULE *mp;
	void (*modfunc)(FILE *);
	char *s, *s1;
	log_write(LOG_STARTUP, "INI", "LOAD", "Loading object structures.");

	if (db_read() < 0)
	{
		log_write(LOG_ALWAYS, "INI", "FATAL", "Error loading object structures.");
		return -1;
	}

	/*
	 * Call modules to load data from DBM
	 */
	call_all_modules_nocache("db_read");
	/*
	 * Call modules to load data from their flat-text database
	 */
	s1 = XMALLOC(MBUF_SIZE, "s1");

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		XSNPRINTF(s1, MBUF_SIZE, "mod_%s_%s", mp->modname, "load_database");

		if ((modfunc = (void (*)(FILE *))dlsym(mp->handle, s1)) != NULL)
		{
			s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);
			f = db_module_flatfile(s, 0);

			if (f)
			{
				(*modfunc)(f);
				tf_fclose(f);
			}

			XFREE(s);
		}
	}

	XFREE(s1);
	log_write(LOG_STARTUP, "INI", "LOAD", "Load complete.");
	return (0);
}

/* match a list of things, using the no_command flag */

int list_check(dbref thing, dbref player, char type, char *str, char *raw_str, int check_parent, int *stop_status)
{
	int match;
	match = 0;

	while (thing != NOTHING)
	{
		if ((thing != player) && (atr_match(thing, player, type, str, raw_str, check_parent) > 0))
		{
			match = 1;

			if (Stop_Match(thing))
			{
				*stop_status = 1;
				return match;
			}
		}

		if (thing != Next(thing))
		{
			thing = Next(thing);
		}
		else
		{
			thing = NOTHING; /* make sure we don't
							  * infinite loop */
		}
	}

	return match;
}

int Hearer(dbref thing)
{
	char *as, *buff, *s;
	dbref aowner;
	int attr, aflags, alen;
	ATTR *ap;

	if (mushstate.inpipe && (thing == mushstate.poutobj))
	{
		return 1;
	}

	if (Connected(thing) || Puppet(thing) || H_Listen(thing))
	{
		return 1;
	}

	if (!Monitor(thing))
	{
		return 0;
	}

	buff = XMALLOC(LBUF_SIZE, "buff");
	atr_push();

	for (attr = atr_head(thing, &as); attr; attr = atr_next(&as))
	{
		ap = atr_num(attr);

		if (!ap || (ap->flags & AF_NOPROG))
		{
			continue;
		}

		atr_get_str(buff, thing, attr, &aowner, &aflags, &alen);

		/*
		 * Make sure we can execute it
		 */

		if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
		{
			continue;
		}

		/*
		 * Make sure there's a : in it
		 */

		for (s = buff + 1; *s && (*s != ':'); s++)
			;

		if (s)
		{
			XFREE(buff);
			atr_pop();
			return 1;
		}
	}

	XFREE(buff);
	atr_pop();
	return 0;
}

/*
 * ----------------------------------------------------------------------
 * Write message to logfile.
 */

void do_logwrite(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *msgtype, char *message)
{
	const char *mt;
	char *msg, *p, *pname;

	/*
	 * If we don't have both a msgtype and a message, make msgtype LOCAL.
	 * Otherwise, truncate msgtype to five characters and capitalize.
	 */

	if (!message || !*message)
	{
		mt = (const char *)"LOCAL";
		msg = msgtype;
	}
	else
	{
		if (strlen(msgtype) > 5)
		{
			msgtype[5] = '\0';
		}

		for (p = msgtype; *p; p++)
		{
			*p = toupper(*p);
		}

		mt = (const char *)msgtype;
		msg = message;
	}

	/*
	 * Just dump it to the log.
	 */
	pname = log_getname(player);
	log_write(LOG_LOCAL, "MSG", mt, "%s: %s", pname, msg);
	XFREE(pname);
	notify_quiet(player, "Logged.");
}

/*
 * ----------------------------------------------------------------------
 * Database and startup stuff.
 */

void do_readcache(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)))
{
	helpindex_load(player);
	fcache_load(player);
}

void process_preload(void)
{
	dbref thing, parent, aowner;
	int aflags, alen, lev;
	char *tstr;
	char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
	FWDLIST *fp;
	PROPDIR *pp;
	fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
	pp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "pp");
	tstr = XMALLOC(LBUF_SIZE, "tstr");

	for (thing = 0; thing < mushstate.db_top; thing++)
	{
		/*
		 * Ignore GOING objects
		 */
		if (Going(thing))
		{
			continue;
		}

		/*
		 * Look for a FORWARDLIST attribute. Load these before doing
		 * anything else, so startup notifications work correctly.
		 */

		if (H_Fwdlist(thing))
		{
			(void)atr_get_str(tstr, thing, A_FORWARDLIST, &aowner, &aflags, &alen);

			if (*tstr)
			{
				fp->data = NULL;
				fwdlist_load(fp, GOD, tstr);

				if (fp->count > 0)
				{
					fwdlist_set(thing, fp);
				}

				if (fp->data)
				{
					XFREE(fp->data);
				}
			}
		}

		/*
		 * Ditto for PROPDIRs
		 */

		if (H_Propdir(thing))
		{
			(void)atr_get_str(tstr, thing, A_PROPDIR, &aowner, &aflags, &alen);

			if (*tstr)
			{
				pp->data = NULL;
				propdir_load(pp, GOD, tstr);

				if (pp->count > 0)
				{
					propdir_set(thing, pp);
				}

				if (pp->data)
				{
					XFREE(pp->data);
				}
			}
		}

		do_top(10);
		/*
		 * Look for STARTUP and DAILY attributes on parents.
		 */
		for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (H_Startup(thing))
			{
				did_it(Owner(thing), thing, A_NULL, NULL, A_NULL, NULL, A_STARTUP, 0, (char **)NULL, 0, 0);
				/*
				 * Process queue entries as we add them
				 */
				do_second();
				do_top(10);
				break;
			}
		}

		for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
		{
			if (Flags2(thing) & HAS_DAILY)
			{
				XSPRINTF(tbuf, "0 %d * * *", mushconf.events_daily_hour);
				call_cron(thing, thing, A_DAILY, tbuf);
				break;
			}
		}
	}
	XFREE(fp);
	XFREE(pp);
	XFREE(tstr);
	XFREE(tbuf);
}

/*
 * ---------------------------------------------------------------------------
 * info: display info about the file being read or written.
 */

void info(int fmt, int flags, int ver)
{
	const char *cp;

	switch (fmt)
	{
	case F_TINYMUSH:
		cp = "TinyMUSH-3";
		break;

	case F_MUX:
		cp = "TinyMUX";
		break;

	case F_MUSH:
		cp = "TinyMUSH";
		break;

	case F_MUSE:
		cp = "TinyMUSE";
		break;

	case F_MUD:
		cp = "TinyMUD";
		break;

	case F_MUCK:
		cp = "TinyMUCK";
		break;

	default:
		cp = "*unknown*";
		break;
	}

	log_write_raw(1, "%s version %d:", cp, ver);

	if (flags & V_ZONE)
	{
		log_write_raw(1, " Zone");
	}

	if (flags & V_LINK)
	{
		log_write_raw(1, " Link");
	}

	if (flags & V_GDBM)
	{
		log_write_raw(1, " GDBM");
	}

	if (flags & V_ATRNAME)
	{
		log_write_raw(1, " AtrName");
	}

	if (flags & V_ATRKEY)
	{
		if ((fmt == F_MUSH) && (ver == 2))
		{
			log_write_raw(1, " ExtLocks");
		}
		else
		{
			log_write_raw(1, " AtrKey");
		}
	}

	if (flags & V_PARENT)
	{
		log_write_raw(1, " Parent");
	}

	if (flags & V_COMM)
	{
		log_write_raw(1, " Comm");
	}

	if (flags & V_ATRMONEY)
	{
		log_write_raw(1, " AtrMoney");
	}

	if (flags & V_XFLAGS)
	{
		log_write_raw(1, " ExtFlags");
	}

	if (flags & V_3FLAGS)
	{
		log_write_raw(1, " MoreFlags");
	}

	if (flags & V_POWERS)
	{
		log_write_raw(1, " Powers");
	}

	if (flags & V_QUOTED)
	{
		log_write_raw(1, " QuotedStr");
	}

	if (flags & V_TQUOTAS)
	{
		log_write_raw(1, " TypedQuotas");
	}

	if (flags & V_TIMESTAMPS)
	{
		log_write_raw(1, " Timestamps");
	}

	if (flags & V_VISUALATTRS)
	{
		log_write_raw(1, " VisualAttrs");
	}

	if (flags & V_CREATETIME)
	{
		log_write_raw(1, " CreateTime");
	}

	log_write_raw(1, "\n");
}

void usage_dbconvert(void)
{
	fprintf(stderr, "  -f, --config=<filename>   config file\n");
	fprintf(stderr, "  -C, --check               perform consistency check\n");
	fprintf(stderr, "  -d, --data=<path>         data directory\n");
	fprintf(stderr, "  -D, --gdbmdb=<filename>   gdbm database\n");
	fprintf(stderr, "  -r, --crashdb=<filename>  gdbm crash db\n");
	fprintf(stderr, "  -q, --cleanattr           clean attribute table\n");
	fprintf(stderr, "  -G, --gdbm                write in gdbm format\n");
	fprintf(stderr, "  -g, --flat                write in flat file format\n");
	fprintf(stderr, "  -K, --keyattr             store key as an attribute\n");
	fprintf(stderr, "  -k, --keyhdr              store key in the header\n");
	fprintf(stderr, "  -L, --links               include link information\n");
	fprintf(stderr, "  -l, --nolinks             don't include link information\n");
	fprintf(stderr, "  -M, --maps                store attr map if GDBM\n");
	fprintf(stderr, "  -m, --nomaps              don't store attr map if GDBM\n");
	fprintf(stderr, "  -N, --nameattr            store name as an attribute\n");
	fprintf(stderr, "  -n, --namehdr             store name in the header\n");
	fprintf(stderr, "  -P, --parents             include parent information\n");
	fprintf(stderr, "  -p, --noparents           don't include parent information\n");
	fprintf(stderr, "  -W, --write               write the output file\n");
	fprintf(stderr, "  -w, --nowrite             don't write the output file.\n");
	fprintf(stderr, "  -X, --mindb               create a default GDBM db\n");
	fprintf(stderr, "  -x, --minflat             create a default flat file db\n");
	fprintf(stderr, "  -Z, --zones               include zone information\n");
	fprintf(stderr, "  -z, --nozones             don't include zone information\n");
	fprintf(stderr, "  -o, --output=<number>     set output version number\n\n");
}

void usage_dbrecover(void)
{
	fprintf(stderr, "  -i, --input               dbm file to recover\n");
	fprintf(stderr, "  -o, --output              recovered db file\n\n");
}

void usage(char *prog, int which)
{
	fprintf(stderr, "\n%s\n\n", mushstate.version.versioninfo);

	switch (which)
	{
	case 0:
		fprintf(stderr, "Usage: %s [options] [CONFIG-FILE]\n", prog);
		fprintf(stderr, "       %s -c DBM-FILE [< INPUT-FILE] [> OUTPUT-FILE]\n", prog);
		fprintf(stderr, "       %s -r -i INPUT-DBM -o OUTPUT-DBM\n\n", prog);
		fprintf(stderr, "When call without -c or -e option, %s accept the following options:\n\n", prog);
		fprintf(stderr, "  CONFIG-FILE               configuration file\n");
		fprintf(stderr, "  -d, --debug               debug mode, do not fork to background\n");
		fprintf(stderr, "  -m, --mindb               delete the current databases and create a new one\n\n");
		fprintf(stderr, "When call with the -c option, %s accept the following options:\n\n", prog);
		usage_dbconvert();
		fprintf(stderr, "When call with the -r option, %s accept the following options:\n\n", prog);
		usage_dbrecover();
		break;

	case 1:
		fprintf(stderr, "Usage: dbconvert [options] DBM-FILE [< INPUT-FILE] [> OUTPUT-FILE]\n");
		fprintf(stderr, "Options:\n");
		usage_dbconvert();
		break;

	case 2:
		fprintf(stderr, "Usage: recover -i INPUT-DBM -o OUTPUT-DBM\n");
		usage_dbrecover();
		break;
	}

	fprintf(stderr, "\nDefault configuration file : %s\n\n", DEFAULT_CONFIG_FILE);
}

void recover_flatfile(char *flat)
{
	int db_ver, db_format, db_flags, setflags, clrflags;
	MODULE *mp;
	void (*modfunc)(FILE *);
	FILE *f;
	char *s, *s1;
	vattr_init();

	if (init_gdbm_db(mushconf.db_file) < 0)
	{
		log_write_raw(1, "Can't open GDBM file\n");
		exit(EXIT_FAILURE);
	}

	db_lock();
	f = fopen(flat, "r");
	db_read_flatfile(f, &db_format, &db_ver, &db_flags);
	fclose(f);
	/*
	 * Call modules to load their flatfiles
	 */
	s1 = XMALLOC(MBUF_SIZE, "s1");

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		XSNPRINTF(s1, MBUF_SIZE, "mod_%s_%s", mp->modname, "db_read_flatfile");

		if ((modfunc = (void (*)(FILE *))dlsym(mp->handle, s1)) != NULL)
		{
			s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);
			f = db_module_flatfile(s, 0);

			if (f)
			{
				(*modfunc)(f);
				tf_fclose(f);
			}

			XFREE(s);
		}
	}

	XFREE(s1);
	db_ver = OUTPUT_VERSION;
	setflags = OUTPUT_FLAGS;
	clrflags = 0xffffffff;
	db_flags = (db_flags & ~clrflags) | setflags;
	db_write();
	/*
	 * Call all modules to write to GDBM
	 */
	call_all_modules_nocache("db_write");
	db_unlock();
	cache_sync();
	dddb_close();
}

int dbconvert(int argc, char *argv[])
{
	int setflags, clrflags, ver;
	int db_ver, db_format, db_flags, do_check, do_write;
	int c, dbclean, errflg = 0;
	char *opt_conf = (char *)DEFAULT_CONFIG_FILE;
	char *opt_datadir = (char *)DEFAULT_DATABASE_HOME;
	char *opt_gdbmfile = (char *)DEFAULT_CONFIG_FILE;
	char *s, *s1;
	FILE *f;
	MODULE *mp;
	void (*modfunc)(FILE *);
	int option_index = 0;
	struct option long_options[] = {
		{"config", required_argument, 0, 'f'},
		{"check", no_argument, 0, 'C'},
		{"data", required_argument, 0, 'd'},
		{"gdbmdb", required_argument, 0, 'D'},
		{"crashdb", required_argument, 0, 'r'},
		{"cleanattr", no_argument, 0, 'q'},
		{"gdbm", no_argument, 0, 'G'},
		{"flat", no_argument, 0, 'g'},
		{"keyattr", no_argument, 0, 'K'},
		{"keyhdr", no_argument, 0, 'k'},
		{"links", no_argument, 0, 'L'},
		{"nolinks", no_argument, 0, 'l'},
		{"maps", no_argument, 0, 'M'},
		{"nomaps", no_argument, 0, 'm'},
		{"nameattr", no_argument, 0, 'N'},
		{"namehdr", no_argument, 0, 'n'},
		{"parents", no_argument, 0, 'P'},
		{"noparents", no_argument, 0, 'p'},
		{"write", no_argument, 0, 'W'},
		{"nowrite", no_argument, 0, 'w'},
		{"mindb", no_argument, 0, 'X'},
		{"minflat", no_argument, 0, 'x'},
		{"zones", no_argument, 0, 'Z'},
		{"nozones", no_argument, 0, 'z'},
		{"output", required_argument, 0, 'o'},
		{"help", no_argument, 0, '?'},
		{0, 0, 0, 0}};
	logfile_init(NULL);
	/*
	 * Decide what conversions to do and how to format the output file
	 */
	setflags = clrflags = ver = do_check = 0;
	do_write = 1;
	dbclean = V_DBCLEAN;

	while ((c = getopt_long(argc, argv, "f:Cd:D:r:qGgKkLlMmNnPpWwXxZzo:?", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'c':
			opt_conf = optarg;
			break;

		case 'd':
			opt_datadir = optarg;
			break;

		case 'D':
			opt_gdbmfile = optarg;
			break;

		case 'C':
			do_check = 1;
			break;

		case 'q':
			dbclean = 0;
			break;

		case 'G':
			clrflags = 0xffffffff;
			setflags = OUTPUT_FLAGS;
			ver = OUTPUT_VERSION;
			break;

		case 'g':
			clrflags = 0xffffffff;
			setflags = UNLOAD_OUTFLAGS;
			ver = UNLOAD_VERSION;
			break;

		case 'Z':
			setflags |= V_ZONE;
			break;

		case 'z':
			clrflags |= V_ZONE;
			break;

		case 'L':
			setflags |= V_LINK;
			break;

		case 'l':
			clrflags |= V_LINK;
			break;

		case 'N':
			setflags |= V_ATRNAME;
			break;

		case 'n':
			clrflags |= V_ATRNAME;
			break;

		case 'K':
			setflags |= V_ATRKEY;
			break;

		case 'k':
			clrflags |= V_ATRKEY;
			break;

		case 'P':
			setflags |= V_PARENT;
			break;

		case 'p':
			clrflags |= V_PARENT;
			break;

		case 'W':
			do_write = 1;
			break;

		case 'w':
			do_write = 0;
			break;

		case 'X':
			clrflags = 0xffffffff;
			setflags = OUTPUT_FLAGS;
			ver = OUTPUT_VERSION;
			break;

		case 'x':
			clrflags = 0xffffffff;
			setflags = UNLOAD_OUTFLAGS;
			ver = UNLOAD_VERSION;
			break;

		case 'o':
			ver = ver * 10 + (int)strtol(optarg, NULL, 10);
			break;

		default:
			errflg++;
		}
	}

	if (errflg || optind >= argc)
	{
		usage(basename(argv[0]), 1);
		exit(EXIT_FAILURE);
	}

	mushconf.dbhome = XSTRDUP(opt_datadir, "argv");
	mushconf.db_file = XSTRDUP(opt_gdbmfile, "argv");
	cf_init();
	mushstate.standalone = 1;
	cf_read(opt_conf);
	mushstate.initializing = 0;

	/*
	 * Open the gdbm file
	 */
	vattr_init();

	if (init_gdbm_db(argv[optind]) < 0)
	{
		log_write_raw(1, "Can't open GDBM file\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Lock the database
	 */
	db_lock();

	/*
	 * Go do it
	 */

	if (!(setflags & V_GDBM))
	{
		db_read();
		/*
		 * Call all modules to read from GDBM
		 */
		call_all_modules_nocache("db_read");
		db_format = F_TINYMUSH;
		db_ver = OUTPUT_VERSION;
		db_flags = OUTPUT_FLAGS;
	}
	else
	{
		db_read_flatfile(stdin, &db_format, &db_ver, &db_flags);
		/*
		 * Call modules to load their flatfiles
		 */
		s1 = XMALLOC(MBUF_SIZE, "s1");

		for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
		{
			XSNPRINTF(s1, MBUF_SIZE, "mod_%s_%s", mp->modname, "db_read_flatfile");

			if ((modfunc = (void (*)(FILE *))dlsym(mp->handle, s1)) != NULL)
			{
				s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);
				f = db_module_flatfile(s, 0);

				if (f)
				{
					(*modfunc)(f);
					tf_fclose(f);
				}

				XFREE(s);
			}
		}

		XFREE(s1);
	}

	log_write_raw(1, "Input: ");
	info(db_format, db_flags, db_ver);

	if (do_check)
	{
		do_dbck(NOTHING, NOTHING, DBCK_FULL);
	}

	if (do_write)
	{
		db_flags = (db_flags & ~clrflags) | setflags;

		if (ver != 0)
		{
			db_ver = ver;
		}
		else
		{
			db_ver = 3;
		}

		log_write_raw(1, "Output: ");
		info(F_TINYMUSH, db_flags, db_ver);

		if (db_flags & V_GDBM)
		{
			db_write();
			/*
			 * Call all modules to write to GDBM
			 */
			db_lock();
			call_all_modules_nocache("db_write");
			db_unlock();
		}
		else
		{
			db_write_flatfile(stdout, F_TINYMUSH, db_ver | db_flags | dbclean);
			/*
			 * Call all modules to write to flatfile
			 */
			s1 = XMALLOC(MBUF_SIZE, "s1");

			for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
			{
				XSNPRINTF(s1, MBUF_SIZE, "mod_%s_%s", mp->modname, "db_write_flatfile");

				if ((modfunc = (void (*)(FILE *))dlsym(mp->handle, s1)) != NULL)
				{
					s = XASPRINTF("s", "%s/%s_mod_%s.db", mushconf.dbhome, mushconf.mush_shortname, mp->modname);
					f = db_module_flatfile(s, 1);

					if (f)
					{
						(*modfunc)(f);
						tf_fclose(f);
					}

					XFREE(s);
				}
			}

			XFREE(s1);
		}
	}

	/*
	 * Unlock the database
	 */
	db_unlock();
	cache_sync();
	dddb_close();
	exit(EXIT_SUCCESS);
}

/**
 * \fn int main ( int argc, char *argv[] )
 * \brief main entry point for TinyMUSH
 *
 * \param argc Number of arguments from command line
 * \param argv Array of arguments from command line
 * \return 0 if everything went well, anything else indicate that an error occured.
 */

int main(int argc, char *argv[])
{
	int mindb = 0;
	CMDENT *cmdp;
	int i, c;
	int errflg = 0;
	pid_t pid;
	char *s, *ts;
	char *templog;
	MODULE *mp;
	char *bp;
	struct stat;
	MODHASHES *m_htab, *hp;
	MODHASHES *m_ntab, *np;
	int option_index = 0;
	struct option long_options[] = {
		{"debug", no_argument, 0, 'd'},
		{"mindb", no_argument, 0, 'm'},
		{"convert", no_argument, 0, 'c'},
		{"recover", no_argument, 0, 'r'},
		{"help", no_argument, 0, '?'},
		{0, 0, 0, 0}};
	mushstate.initializing = 1;
	mushstate.debug = 0;
	mushstate.restarting = 0;
	/*
	 * Do this first, before anything gets a chance to allocate memory.
	 */
	mushstate.raw_allocs = NULL;
	umask(077); /* Keep things to us by default */
	init_version();
	/*
	 * If we are called with the name 'dbconvert', do a DB conversion and
	 * exit
	 */
	s = basename(argv[0]);

	if (s && *s && !strcmp(s, "dbconvert"))
	{
		dbconvert(argc, argv);
	}

	if (s && *s && !strcmp(s, "recover"))
	{
		dbrecover(argc, argv);
	}

	/*
	 * Configure the minimum default values we need to start.
	 */
	mushconf.mush_shortname = XSTRDUP(DEFAULT_SHORTNAME, "cf_string");
	s = getcwd(NULL, 0);
	mushconf.game_home = realpath(s, NULL);
	XFREE(s);
	mushconf.game_exec = realpath(argv[0], NULL);

	/*
	 * Parse options
	 */
	// while ( ( c = getopt ( argc, argv, "drmc?" ) ) != -1 ) {
	while ((c = getopt_long(argc, argv, "dmcr?", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'd': /* Debug mode, do not fork */
			mushstate.debug = 1;
			break;

		case 'm': /* Force minimum db generation */
			mindb = 1;
			break;

		case 'c': /* dbconvert */
			dbconvert(argc, argv);
			exit(EXIT_SUCCESS);
			break;

		case 'r': /* recover */
			dbrecover(argc, argv);
			exit(EXIT_SUCCESS);
			break;

		default:
			errflg++;
			break;
		}
	}

	if (optind < argc)
	{
		/*
		 * The first non-option element is our config file.
		 */
		s = XSTRDUP(argv[optind++], "s");
		mushconf.config_file = realpath(s, NULL);
		XFREE(s);

		s = XSTRDUP(mushconf.config_file, "s");
		mushconf.config_home = XSTRDUP(dirname(s), "mushconf.config_home");
		XFREE(s);
	}
	else
	{
		/*
		 * If there was none, use the default value.
		 */
		s = XSTRDUP(DEFAULT_CONFIG_FILE, "s");
		mushconf.config_file = realpath(s, NULL);
		XFREE(s);

		s = XSTRDUP(mushconf.config_file, "s");
		mushconf.config_home = XSTRDUP(dirname(s), "mushconf.config_home");
		XFREE(s);
	}

	/* Make sure we can read the config file */

	if (!fileexist(mushconf.config_file))
	{
		fprintf(stderr, "Unable to read configuration file %s.\n", mushconf.config_file);
		errflg++;
	}

	if (errflg)
	{
		usage(basename(argv[0]), 0);
		exit(EXIT_FAILURE);
	}

	tf_init();
	time(&mushstate.start_time);
	mushstate.restart_time = mushstate.start_time;
	time(&mushstate.cpu_count_from);
	tcache_init();
	pcache_init();
	templog = XSTRDUP("netmush.XXXXXX", "templog");
	logfile_init(templog);
	cf_init();
	init_rlimit();
	init_cmdtab();
	init_logout_cmdtab();
	init_flagtab();
	init_powertab();
	init_functab();
	init_attrtab();
	log_version();
	init_mstate();
	log_write(LOG_ALWAYS, "INI", "LOAD", "Full path and name of netmush : %s", mushconf.game_exec);
	log_write(LOG_ALWAYS, "INI", "LOAD", "Full path of work directory : %s", mushconf.game_home);
	log_write(LOG_ALWAYS, "INI", "LOAD", "Configuration file : %s", mushconf.config_file);
	log_write(LOG_ALWAYS, "INI", "LOAD", "Configuration home : %s", mushconf.config_home);
	cf_read(mushconf.config_file);

	/*
	 * Abort if someone tried to set the number of global registers to
	 * something stupid. Also adjust the character table if we need to.
	 */
	if ((mushconf.max_global_regs < 10) || (mushconf.max_global_regs > 36))
	{
		fprintf(stderr, "max_global_registers is configured to be less than 10 or more than 36. Please fix this error.\n");
		exit(EXIT_FAILURE);
	}

	if (mushconf.hash_factor < 2)
	{
		mushconf.hash_factor = 2;
		fprintf(stderr, "hash_factor increased to 2, fix your configuration to remove this warning.\n");
	}

	if (mushconf.max_command_args < 10)
	{
		mushconf.max_command_args = 10;
		fprintf(stderr, "max_command_arguments increased to 10, fix your configuration to remove this warning.\n");
	}

	if (mushconf.player_name_length < 22)
	{
		mushconf.player_name_length = 22;
		fprintf(stderr, "max_player_name_length increased to 22, fix your configuration to remove this warning.\n");
	}

	hashinit(&mushstate.player_htab, 250 * mushconf.hash_factor, HT_STR);
	hashinit(&mushstate.nref_htab, 5 * mushconf.hash_factor, HT_STR);
	nhashinit(&mushstate.qpid_htab, 50 * mushconf.hash_factor);
	nhashinit(&mushstate.fwdlist_htab, 25 * mushconf.hash_factor);
	nhashinit(&mushstate.propdir_htab, 25 * mushconf.hash_factor);
	nhashinit(&mushstate.redir_htab, 5 * mushconf.hash_factor);
	nhashinit(&mushstate.objstack_htab, 50 * mushconf.hash_factor);
	nhashinit(&mushstate.objgrid_htab, 50 * mushconf.hash_factor);
	nhashinit(&mushstate.parent_htab, 5 * mushconf.hash_factor);
	nhashinit(&mushstate.desc_htab, 25 * mushconf.hash_factor);
	hashinit(&mushstate.vars_htab, 250 * mushconf.hash_factor, HT_STR);
	hashinit(&mushstate.structs_htab, 15 * mushconf.hash_factor, HT_STR);
	hashinit(&mushstate.cdefs_htab, 15 * mushconf.hash_factor, HT_STR);
	hashinit(&mushstate.instance_htab, 15 * mushconf.hash_factor, HT_STR);
	hashinit(&mushstate.instdata_htab, 25 * mushconf.hash_factor, HT_STR);
	hashinit(&mushstate.api_func_htab, 5 * mushconf.hash_factor, HT_STR);

	mushconf.log_file = XASPRINTF("mushconf.log_file", "%s/%s.log", mushconf.log_home, mushconf.mush_shortname);

	if (tailfind(mushconf.log_file, "GDBM panic: write error\n"))
	{
		log_write(LOG_ALWAYS, "INI", "FATAL", "Log indicate the last run ended with GDBM panic: write error");
		fprintf(stderr, "\nYour log file indicates that the MUSH went down on a GDBM panic\n");
		fprintf(stderr, "while trying to write to the database. This error normally\n");
		fprintf(stderr, "occurs with an out-of-disk-space problem, though it might also\n");
		fprintf(stderr, "be the result of disk-quota-exceeded, or an NFS server issue.\n");
		fprintf(stderr, "Please check to make sure that this condition has been fixed,\n");
		fprintf(stderr, "before restarting the MUSH.\n\n");
		fprintf(stderr, "This error may also indicates that the issue prevented the MUSH\n");
		fprintf(stderr, "from writing out the data it was trying to save to disk, which\n");
		fprintf(stderr, "means that you may have suffered from some database corruption.\n");
		fprintf(stderr, "Please type the following now, to ensure database integrity:\n\n");
		fprintf(stderr, "    ./Reconstruct\n");
		fprintf(stderr, "    ./Backup\n");
		fprintf(stderr, "    mv -f %s %s.old\n\n", mushconf.log_file, mushconf.log_file);
		fprintf(stderr, "If this is all successful, you may type ./Startmush again to\n");
		fprintf(stderr, "restart the MUSH. If the recovery attempt fails, you will\n");
		fprintf(stderr, "need to restore from a previous backup.\n\n");
		exit(EXIT_FAILURE);
	}

	if (fileexist(mushconf.log_file))
	{
		ts = mktimestamp();
		s = XASPRINTF("s", "%s.%s", mushconf.log_file, ts);
		log_write(LOG_STARTUP, "LOG", "CLN", "Renaming old logfile to %s", basename(s));
		copy_file(mushconf.log_file, s, 1);
		XFREE(s);
		XFREE(ts);
	}

	logfile_move(templog, mushconf.log_file);
	XFREE(templog);
	mushconf.pid_file = XASPRINTF("mushconf.pid_file", "%s/%s.pid", mushconf.pid_home, mushconf.mush_shortname);
	mushconf.db_file = XASPRINTF("mushconf.db_file", "%s.db", mushconf.mush_shortname);
	mushconf.status_file = XASPRINTF("mushconf.status_file", "%s/%s.SHUTDOWN", mushconf.log_home, mushconf.mush_shortname);

	s = XASPRINTF("s", "%s/%s.db.RESTART", mushconf.dbhome, mushconf.mush_shortname);

	if (fileexist(s))
	{
		log_write(LOG_ALWAYS, "INI", "LOAD", "There is a restart database, %s, present. Restarting", s);
		mushstate.restarting = 1;
	}

	XFREE(s);
	pid = isrunning(mushconf.pid_file);

	if (pid)
	{
		log_write(LOG_ALWAYS, "INI", "FATAL", "The MUSH already seems to be running at pid %ld.", (long)pid);
		exit(EXIT_FAILURE);
	}

	handlestartupflatfiles(HANDLE_FLAT_KILL);
	handlestartupflatfiles(HANDLE_FLAT_CRASH);

	if (mushconf.help_users == NULL)
	{
		mushconf.help_users = XASPRINTF("mushconf.help_users", "help %s/help", mushconf.txthome);
	}

	if (mushconf.help_wizards == NULL)
	{
		mushconf.help_wizards = XASPRINTF("mushconf.help_wizards", "wizhelp %s/wizhelp", mushconf.txthome);
	}

	if (mushconf.help_quick == NULL)
	{
		mushconf.help_quick = XASPRINTF("mushconf.help_quick", "qhelp %s/qhelp", mushconf.txthome);
	}

	add_helpfile(GOD, (char *)"main:add_helpfile", mushconf.help_users, 1);
	add_helpfile(GOD, (char *)"main:add_helpfile", mushconf.help_wizards, 1);
	add_helpfile(GOD, (char *)"main:add_helpfile", mushconf.help_quick, 1);

	if (mushconf.guest_file == NULL)
	{
		mushconf.guest_file = XASPRINTF("mushconf.guest_file", "%s/guest.txt", mushconf.txthome);
	}

	if (mushconf.conn_file == NULL)
	{
		mushconf.conn_file = XASPRINTF("mushconf.conn_file", "%s/connect.txt", mushconf.txthome);
	}

	if (mushconf.creg_file == NULL)
	{
		mushconf.creg_file = XASPRINTF("mushconf.creg_file", "%s/register.txt", mushconf.txthome);
	}

	if (mushconf.regf_file == NULL)
	{
		mushconf.regf_file = XASPRINTF("mushconf.regf_file", "%s/create_reg.txt", mushconf.txthome);
	}

	if (mushconf.motd_file == NULL)
	{
		mushconf.motd_file = XASPRINTF("mushconf.motd_file", "%s/motd.txt", mushconf.txthome);
	}

	if (mushconf.wizmotd_file == NULL)
	{
		mushconf.wizmotd_file = XASPRINTF("mushconf.wizmotd_file", "%s/wizmotd.txt", mushconf.txthome);
	}

	if (mushconf.quit_file == NULL)
	{
		mushconf.quit_file = XASPRINTF("mushconf.quit_file", "%s/quit.txt", mushconf.txthome);
	}

	if (mushconf.down_file == NULL)
	{
		mushconf.down_file = XASPRINTF("mushconf.down_file", "%s/down.txt", mushconf.txthome);
	}

	if (mushconf.full_file == NULL)
	{
		mushconf.full_file = XASPRINTF("mushconf.full_file", "%s/full.txt", mushconf.txthome);
	}

	if (mushconf.site_file == NULL)
	{
		mushconf.site_file = XASPRINTF("mushconf.site_file", "%s/badsite.txt", mushconf.txthome);
	}

	if (mushconf.crea_file == NULL)
	{
		mushconf.crea_file = XASPRINTF("mushconf.crea_file", "%s/newuser.txt", mushconf.txthome);
	}

	if (mushconf.have_pueblo == 1)
	{
		if (mushconf.htmlconn_file == NULL)
		{
			mushconf.htmlconn_file = XASPRINTF("mushconf.htmlconn_file", "%s/htmlconn.txt", mushconf.txthome);
		}
	}

	vattr_init();
	cmdp = (CMDENT *)hashfind((char *)"wizhelp", &mushstate.command_htab);

	if (cmdp)
	{
		cmdp->perms |= CA_WIZARD;
	}

	bp = mushstate.modloaded;

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		if (bp != mushstate.modloaded)
		{
			SAFE_MB_CHR(' ', mushstate.modloaded, &bp);
		}

		SAFE_MB_STR(mp->modname, mushstate.modloaded, &bp);
	}

	mushconf.exec_path = XSTRDUP(argv[0], "argv");
	fcache_init();
	helpindex_init();
	/*
	 * If after doing all that stuff, there is still no db, create a minimal one.
	 */
	s = XASPRINTF("s", "%s/%s", mushconf.dbhome, mushconf.db_file);

	if (!fileexist(s))
	{
		log_write(LOG_ALWAYS, "INI", "LOAD", "No database exist, creating a new database.");
		mindb = 1;
	}

	XFREE(s);

	if (mindb)
	{
		unlink(mushconf.db_file);
	}

	if (init_gdbm_db(mushconf.db_file) < 0)
	{
		log_write(LOG_ALWAYS, "INI", "FATAL", "Couldn't load text database: %s", mushconf.db_file);
		exit(EXIT_FAILURE);
	}

	mushstate.record_players = 0;
	mushstate.loading_db = 1;

	if (mindb)
	{
		db_make_minimal();
		call_all_modules_nocache("make_minimal");
	}
	else if (load_game() < 0)
	{
		log_write(LOG_ALWAYS, "INI", "FATAL", "Couldn't load objects.");
		exit(EXIT_FAILURE);
	}

	mushstate.loading_db = 0;
	set_signals();

	/*
	 * Do a consistency check and set up the freelist
	 */

	if (!Good_obj(GOD) || !isPlayer(GOD))
	{
		log_write(LOG_ALWAYS, "CNF", "VRFY", "Fatal error: GOD object #%d is not a valid player.", GOD);
		exit(EXIT_FAILURE);
	}

	do_dbck(NOTHING, NOTHING, 0);
	/*
	 * Reset all the hash stats
	 */
	hashreset(&mushstate.command_htab);
	hashreset(&mushstate.logout_cmd_htab);
	hashreset(&mushstate.func_htab);
	hashreset(&mushstate.ufunc_htab);
	hashreset(&mushstate.powers_htab);
	hashreset(&mushstate.flags_htab);
	hashreset(&mushstate.attr_name_htab);
	hashreset(&mushstate.vattr_name_htab);
	hashreset(&mushstate.player_htab);
	hashreset(&mushstate.nref_htab);
	nhashreset(&mushstate.desc_htab);
	nhashreset(&mushstate.qpid_htab);
	nhashreset(&mushstate.fwdlist_htab);
	nhashreset(&mushstate.propdir_htab);
	nhashreset(&mushstate.objstack_htab);
	nhashreset(&mushstate.objgrid_htab);
	nhashreset(&mushstate.parent_htab);
	hashreset(&mushstate.vars_htab);
	hashreset(&mushstate.structs_htab);
	hashreset(&mushstate.cdefs_htab);
	hashreset(&mushstate.instance_htab);
	hashreset(&mushstate.instdata_htab);
	hashreset(&mushstate.api_func_htab);

	for (i = 0; i < mushstate.helpfiles; i++)
	{
		hashreset(&mushstate.hfile_hashes[i]);
	}

	s = XMALLOC(MBUF_SIZE, "s");

	for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
	{
		XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, "hashtable");
		m_htab = (MODHASHES *)dlsym(mp->handle, s);

		if (m_htab)
		{
			for (hp = m_htab; hp->tabname != NULL; hp++)
			{
				hashreset(hp->htab);
			}
		}

		XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, "nhashtable");
		m_ntab = (MODHASHES *)dlsym(mp->handle, s);

		if (m_ntab)
		{
			for (np = m_ntab; np->tabname != NULL; np++)
			{
				nhashreset(np->htab);
			}
		}
	}

	XFREE(s);
	mushstate.now = time(NULL);
	/*
	 * Initialize PCRE tables for locale.
	 */
	mushstate.retabs = pcre_maketables();
	/*
	 * Go do restart things.
	 */
	if (mushstate.restarting)
	{
		load_restart_db();
	}

	/*
	 * We have to do an update, even though we're starting up, because
	 * there may be players connected from a restart, as well as objects.
	 */
	call_all_modules_nocache("cleanup_startup");
	/*
	 * You must do your startups AFTER you load your restart database, or
	 * softcode that depends on knowing who is connected and so forth
	 * will be hosed.
	 */
	process_preload();

	/*
	 * Startup is done.
	 */
	mushstate.initializing = 0;

	/*
	 * Clear all reference flags in the cache-- what happens when the
	 * game loads is NOT representative of normal cache behavior :)
	 * Neither is creating a new db, but in that case the objects exist
	 * only in the cache...
	 */
	if (!mindb)
	{
		cache_reset();
	}

	/*
	 * This must happen after startups are run, in order to get a really
	 * good idea of what's actually out there.
	 */
	do_hashresize(GOD, GOD, 0);
	log_write(LOG_STARTUP, "INI", "LOAD", "Cleanup completed.");

	if (mushstate.restarting)
	{
		raw_broadcast(0, "GAME: Restart finished.");
	}

	if (!mushstate.restarting)
	{
		if (backup_mush(NOTHING, NOTHING, 0) != 0)
		{
			log_write(LOG_STARTUP, "INI", "FATAL", "Unable to backup");
		}
	}

	if (!(getppid() == 1) && !mushstate.debug)
	{
		int forkstatus;
		forkstatus = fork();

		if (forkstatus < 0)
		{
			log_write(LOG_STARTUP, "INI", "FORK", "Unable to fork, %s", strerror(errno));
		}
		else if (forkstatus > 0)
		{
			exit(EXIT_SUCCESS);
		}
		else
		{
			setsid();

			if (chdir(mushconf.game_home) < 0)
			{
				log_write(LOG_STARTUP, "INI", "FORK", "Unable to chdir to game directory, %s", strerror(errno));
			}
		}
	}

	log_write(LOG_STARTUP, "INI", "RUN", "Startup processing complete. (Process ID : %d)\n", write_pidfile(mushconf.pid_file));

	if (!mushstate.restarting)
	{
		/*
		 * Cosmetic, force a newline to stderr to clear console logs
		 */

		fflush(stderr);
		fflush(stdout);
		fprintf(stderr, "\n");

		if (!mushstate.debug)
		{
			if (freopen(DEV_NULL, "w", stdout) == NULL)
			{
				log_write(LOG_STARTUP, "INI", "LOAD", "Cannot redirect stdout to /dev/null");
			}

			if (freopen(DEV_NULL, "w", stderr) == NULL)
			{
				log_write(LOG_STARTUP, "INI", "LOAD", "Cannot redirect stdout to /dev/null");
			}
		}
	}

	/*
	 * go do it
	 */
	if (!mushstate.debug)
	{
		mushstate.logstderr = 0;
	}

	init_timer();
	shovechars(mushconf.port);
	log_write(LOG_STARTUP, "INI", "SHDN", "Going down.");
	close_sockets(0, (char *)"Going down - Bye");
	dump_database();
	cache_sync();
	dddb_close();

	if (fileexist(mushconf.log_file))
	{
		ts = mktimestamp();
		s = XASPRINTF("s", "%s.%s", mushconf.log_file, ts);
		copy_file(mushconf.log_file, s, 1);
		XFREE(s);
		XFREE(ts);
	}

	exit(EXIT_SUCCESS);
}

void init_rlimit(void)
{
	struct rlimit *rlp;
	rlp = (struct rlimit *)XMALLOC(sizeof(struct rlimit), "rlp");

	if (getrlimit(RLIMIT_NOFILE, rlp))
	{
		log_perror("RLM", "FAIL", NULL, "getrlimit()");
		XFREE(rlp);
		return;
	}

	rlp->rlim_cur = rlp->rlim_max;

	if (setrlimit(RLIMIT_NOFILE, rlp))
	{
		log_perror("RLM", "FAIL", NULL, "setrlimit()");
	}

	XFREE(rlp);
}
