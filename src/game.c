/* game.c - main program and misc functions */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */
#include "typedefs.h"           /* required by mudconf */
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "udb.h"		/* required by code */
#include "udb_defs.h"
#include "externs.h"		/* required by interface */
#include "interface.h"		/* required by code */

#include "file_c.h"		/* required by code */
#include "command.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "pcre.h"		/* required by code */
#include "defaults.h"		/* required by code */

extern void init_attrtab(void);

extern void init_cmdtab(void);

extern void cf_init(void);

extern void pcache_init(void);

extern int cf_read(char *fn);

extern void init_functab(void);

extern void close_sockets(int emergency, char *message);

extern void init_version(void);

extern void init_logout_cmdtab(void);

extern void init_timer(void);

extern void raw_notify_html(dbref, char *);

extern void do_dbck(dbref, dbref, int);

extern void logfile_init(char *);

extern void init_genrand(unsigned long);

void fork_and_dump(dbref, dbref, int);

void dump_database(void);

dbref db_read_flatfile(FILE *, int *, int *, int *);

void pcache_sync(void);

static void init_rlimit(void);

extern int add_helpfile(dbref, char *, char *, int);

extern void vattr_init(void);

extern void load_restart_db(void);

extern void tcache_init(void);

extern void helpindex_load(dbref);

extern void helpindex_init(void);

extern int dddb_optimize(void);

extern LOGFILETAB logfds_table[];

extern volatile pid_t slave_pid;

extern volatile int slave_socket;

extern char qidx_chartab[256];

#ifdef NEED_EMPTY_LTPLSYM
const lt_dlsymlist lt_preloaded_symbols[] = { {0, (lt_ptr_t) 0} };
#endif

extern char *optarg;

extern int optind;

void recover(char *);


/*
 * Used to figure out if netmush is already running. Since there's
 * so many differences between sysctl implementation, i prefer to
 * call pgrep. If every system would do like FreeBSD and implement
 * PIDFILE(3), the world would be a better place!
 */

pid_t isrunning(char *pidfile) {
    FILE *fp;
    pid_t pid=0, rpid=0;
    int i=0;
    char buff[MBUF_SIZE];
    
    fp=fopen(pidfile, "r");
    
    if(fp == NULL)
        return(0);
    
    fgets(buff, MBUF_SIZE, fp);
    
    fclose(fp);
    
    pid = (pid_t)strtol(buff, (char **)NULL, 10);
    
    fp = popen("pgrep netmush", "r");
    
    if( fp == NULL)
        return(0);
        
    while(fgets(buff, MBUF_SIZE, fp)!=NULL) {
        rpid=(pid_t)strtol(buff, (char **)NULL, 10);
        if(pid == rpid) {
            i = 1;
            break;
        }
    }
    
    pclose(fp);
    
    if(i)
        return(pid);
    
    return(0);    
}


/*
 * CHeck if a file exist
 */
 
int fileexist(char *file) {
    int fp;
    
    fp=open(file, O_RDONLY);
    
    if(fp < 0)
        return(0);
    close(fp);
    
    return(1);
}
#define HANDLE_FLAT_CRASH	1
#define HANDLE_FLAT_KILL	2

int handlestartupflatfiles(int flag) {
    char *db, *flat, *db_bak, *flat_bak;
    int i;
    struct stat sb1, sb2;

    db = XSTRDUP(tmprintf("%s/%s", mudconf.dbhome, mudconf.db_file), "handlestartupflatfiles");
    flat = XSTRDUP(tmprintf("%s/%s.%s", mudconf.dbhome, mudconf.db_file, (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILLED") ), "handlestartupflatfiles");
    db_bak = XSTRDUP(tmprintf("%s/%s.%ld", mudconf.dbhome, mudconf.db_file, time(NULL)), "handlestartupflatfiles");
    flat_bak = XSTRDUP(tmprintf("%s/%s.%s.%ld", mudconf.dbhome, mudconf.db_file, (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILLED") ,time(NULL)), "handlestartupflatfiles");
    
    i = open(flat, O_RDONLY);
    if(i>0) {
        fstat(i,&sb1);
        close(i);
        stat(db,&sb2);
        if(tailfind(flat, "***END OF DUMP***\n")) {
            STARTLOG(LOG_ALWAYS, "INI", "LOAD")
            log_printf("A non-corrupt %s file is present.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));
            ENDLOG
            if(difftime(sb1.st_mtime, sb2.st_mtime) > (double)0) { 
                STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                log_printf("The %s file is newer than your current database.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));
                ENDLOG
                if(rename(db, db_bak) != 0) {
                    STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                    log_printf("Unable to archive previous db to : %s", db_bak);
                    ENDLOG
                }
                recover(flat);
                if(unlink(flat) != 0) {
                    STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                    log_printf("Unable to delete : %s", flat);
                    ENDLOG
                } 
                STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                log_printf("Recovery successfull");
                ENDLOG
            } else {
                STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                log_printf("The %s file is older than your current database.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));
                ENDLOG
                if(rename(flat, flat_bak) == 0) {
                    STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                    log_printf("Older %s file archived as : %s", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"), flat_bak);
                    ENDLOG
                } else {
                    STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                    log_printf("Unable to archive %s file as : %s", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"), flat_bak);
                    ENDLOG
                }
            }
        } else {
            STARTLOG(LOG_ALWAYS, "INI", "LOAD")
            log_printf("A corrupt %s file is present.", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));
            ENDLOG
            STARTLOG(LOG_ALWAYS, "INI", "LOAD")
            if(!rename(flat, flat_bak)) {
                log_printf("Archived as : %s, using previous db to load", flat_bak);
            } else {
                log_printf("Unable to archive %s file, using previous db to load", (flag == HANDLE_FLAT_CRASH ? "CRASH" : "KILL"));
            }
            ENDLOG
        }
    }

    XFREE(db, "handlestartupflatfiles");
    XFREE(flat, "handlestartupflatfiles");
    XFREE(db_bak, "handlestartupflatfiles");
    XFREE(flat_bak, "handlestartupflatfiles");
}

/*
 * Read the last line of a file and compare
 * with the given key. Return 1 of they
 * match.
 */

int tailfind(char *file, char *key) {
    int fp;
    char s[MBUF_SIZE];
    off_t pos;
    
    fp=open(file, O_RDONLY);
    
    if(fp < 0) {
        return(0);
    }
    
    pos = lseek(fp, 0 - strlen(key), SEEK_END);
    read(fp, s, strlen(key));
    close(fp);
    
    if(strncmp(s, key, strlen(key))==0) 
        return(1);
    return(0);
}


/*
 * used to allocate storage for temporary stuff, cleared before command
 * execution
 */

void do_dump(dbref player, dbref cause, int key) {
    if (mudstate.dumping) {
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

void do_hashresize(dbref player, dbref cause, int key) {
    MODULE *mp;

    MODHASHES *m_htab, *hp;

    MODNHASHES *m_ntab, *np;

    hashresize(&mudstate.command_htab, 512);
    hashresize(&mudstate.player_htab, 16);
    hashresize(&mudstate.nref_htab, 8);
    hashresize(&mudstate.vattr_name_htab, 256);
    nhashresize(&mudstate.qpid_htab, 256);
    nhashresize(&mudstate.fwdlist_htab, 8);
    nhashresize(&mudstate.propdir_htab, 8);
    nhashresize(&mudstate.redir_htab, 8);
    hashresize(&mudstate.ufunc_htab, 8);
    hashresize(&mudstate.structs_htab,
               (mudstate.max_structs < 16) ? 16 : mudstate.max_structs);
    hashresize(&mudstate.cdefs_htab,
               (mudstate.max_cdefs < 16) ? 16 : mudstate.max_cdefs);
    hashresize(&mudstate.instance_htab,
               (mudstate.max_instance < 16) ? 16 : mudstate.max_instance);
    hashresize(&mudstate.instdata_htab,
               (mudstate.max_instdata < 16) ? 16 : mudstate.max_instdata);
    nhashresize(&mudstate.objstack_htab,
                (mudstate.max_stacks < 16) ? 16 : mudstate.max_stacks);
    nhashresize(&mudstate.objgrid_htab, 16);
    hashresize(&mudstate.vars_htab,
               (mudstate.max_vars < 16) ? 16 : mudstate.max_vars);
    hashresize(&mudstate.api_func_htab, 8);

    WALK_ALL_MODULES(mp) {
        m_htab = DLSYM_VAR(mp->handle, mp->modname,
                           "hashtable", MODHASHES *);
        if (m_htab) {
            for (hp = m_htab; hp->tabname != NULL; hp++) {
                hashresize(hp->htab, hp->min_size);
            }
        }
        m_ntab = DLSYM_VAR(mp->handle, mp->modname,
                           "nhashtable", MODNHASHES *);
        if (m_ntab) {
            for (np = m_ntab; np->tabname != NULL; np++) {
                nhashresize(np->htab, np->min_size);
            }
        }
    }

    if (!mudstate.restarting)
        notify(player, "Resized.");
}

/*
 * ----------------------------------------------------------------------
 * regexp_match: Load a regular expression match and insert it into
 * registers. PCRE modifications adapted from PennMUSH.
 */

#define PCRE_MAX_OFFSETS 99

int regexp_match(char *pattern, char *str, int case_opt, char *args[], int nargs) {
    int i;

    pcre *re;

    const char *errptr;

    int erroffset;

    int offsets[PCRE_MAX_OFFSETS];

    int subpatterns;

    if ((re = pcre_compile(pattern, case_opt,
                           &errptr, &erroffset, mudstate.retabs)) == NULL) {
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
    if ((subpatterns = pcre_exec(re, NULL, str, strlen(str), 0, 0,
                                 offsets, PCRE_MAX_OFFSETS)) < 0) {
        XFREE(re, "regexp_match.re");
        return 0;
    }
    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if (subpatterns == 0)
        subpatterns = PCRE_MAX_OFFSETS / 3;

    /*
     * Now we fill in our args vector. Note that in regexp matching, 0 is
     * the entire string matched, and the parenthesized strings go from 1
     * to 9. We DO PRESERVE THIS PARADIGM, for consistency with other
     * languages.
     */

    for (i = 0; i < nargs; i++) {
        args[i] = NULL;
    }

    for (i = 0; i < nargs; i++) {
        args[i] = alloc_lbuf("regexp_match");
        if (pcre_copy_substring(str, offsets, subpatterns, i,
                                args[i], LBUF_SIZE) < 0) {
            /*
             * Match behavior of wild(): clear out null values.
             */
            free_lbuf(args[i]);
            args[i] = NULL;
        }
    }

    XFREE(re, "regexp_match.re");
    return 1;
}

/*
 * ----------------------------------------------------------------------
 * atr_match: Check attribute list for wild card matches and queue them.
 */

static int atr_match1(dbref thing, dbref parent, dbref player, char type, char *str, char *raw_str, int check_exclude, int hash_insert) {
    dbref aowner;

    int match, attr, aflags, alen, i;

    char *buff, *s, *as;

    char *args[NUM_ENV_VARS];

    ATTR *ap;

    /*
     * See if we can do it.  Silently fail if we can't.
     */

    if (!could_doit(player, parent, A_LUSE))
        return -1;

    match = 0;
    buff = alloc_lbuf("atr_match1");
    atr_push();
    for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
        ap = atr_num(attr);

        /*
         * Never check NOPROG attributes.
         */

        if (!ap || (ap->flags & AF_NOPROG))
            continue;

        /*
         * If we aren't the bottom level check if we saw this attr
         * before.  Also exclude it if the attribute type is PRIVATE.
         */

        if (check_exclude &&
                ((ap->flags & AF_PRIVATE) ||
                 nhashfind(ap->number, &mudstate.parent_htab))) {
            continue;
        }
        atr_get_str(buff, parent, attr, &aowner, &aflags, &alen);

        /*
         * Skip if private and on a parent
         */

        if (check_exclude && (aflags & AF_PRIVATE)) {
            continue;
        }
        /*
         * If we aren't the top level remember this attr so we
         * exclude it from now on.
         */

        if (hash_insert)
            nhashadd(ap->number, (int *)&attr,
                     &mudstate.parent_htab);

        /*
         * Check for the leadin character after excluding the attrib
         * This lets non-command attribs on the child block commands
         * on the parent.
         */

        if ((buff[0] != type) || (aflags & AF_NOPROG))
            continue;

        /*
         * decode it: search for first un escaped :
         */

        for (s = buff + 1;
                *s && ((*s != ':') || (*(s - 1) == '\\')); s++);
        if (!*s)
            continue;
        *s++ = 0;
        if ((!(aflags & (AF_REGEXP | AF_RMATCH)) &&
                wild(buff + 1,
                     ((aflags & AF_NOPARSE) ? raw_str : str),
                     args, NUM_ENV_VARS)) ||
                ((aflags & AF_REGEXP) &&
                 regexp_match(buff + 1,
                              ((aflags & AF_NOPARSE) ? raw_str : str),
                              ((aflags & AF_CASE) ? 0 : PCRE_CASELESS),
                              args, NUM_ENV_VARS)) ||
                ((aflags & AF_RMATCH) &&
                 register_match(buff + 1,
                                ((aflags & AF_NOPARSE) ? raw_str : str),
                                args, NUM_ENV_VARS))) {
            match = 1;
            if (aflags & AF_NOW) {
                process_cmdline(thing, player, s, args,
                                NUM_ENV_VARS, NULL);
            } else {
                wait_que(thing, player, 0, NOTHING, 0, s, args,
                         NUM_ENV_VARS, mudstate.rdata);
            }
            for (i = 0; i < NUM_ENV_VARS; i++) {
                if (args[i])
                    free_lbuf(args[i]);
            }
        }
    }
    free_lbuf(buff);
    atr_pop();
    return (match);
}

int atr_match(dbref thing, dbref player, char type, char *str, char *raw_str, int check_parents) {
    int match, lev, result, exclude, insert;

    dbref parent;

    /*
     * If thing is halted, or it doesn't have a COMMANDS flag and we're
     * we're doing a $-match, don't check it.
     */

    if (((type == AMATCH_CMD) &&
            !Has_Commands(thing) && mudconf.req_cmds_flag)
            || Halted(thing))
        return 0;

    /*
     * If not checking parents, just check the thing
     */

    match = 0;
    if (!check_parents || Orphan(thing))
        return atr_match1(thing, thing, player, type, str, raw_str, 0,
                          0);

    /*
     * Check parents, ignoring halted objects
     */

    exclude = 0;
    insert = 1;
    nhashflush(&mudstate.parent_htab, 0);
    ITER_PARENTS(thing, parent, lev) {
        if (!Good_obj(Parent(parent)))
            insert = 0;
        result = atr_match1(thing, parent, player, type, str, raw_str,
                            exclude, insert);
        if (result > 0) {
            match = 1;
        } else if (result < 0) {
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

int check_filter(dbref object, dbref player, int filter, const char *msg) {
    int aflags, alen;

    dbref aowner;

    char *buf, *nbuf, *cp, *dp, *str;

    pcre *re;

    const char *errptr;

    int len, case_opt, erroffset, subpatterns;

    int offsets[PCRE_MAX_OFFSETS];

    GDATA *preserve;

    buf = atr_pget(object, filter, &aowner, &aflags, &alen);
    if (!*buf) {
        free_lbuf(buf);
        return (1);
    }
    if (!(aflags & AF_NOPARSE)) {
        preserve = save_global_regs("check_filter.save");
        nbuf = dp = alloc_lbuf("check_filter");
        str = buf;
        exec(nbuf, &dp, object, player, player,
             EV_FIGNORE | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
        dp = nbuf;
        free_lbuf(buf);
        restore_global_regs("check_filter.restore", preserve);
    } else {
        dp = buf;
        nbuf = buf;	/* this way, buf will get freed correctly */
    }

    if (!(aflags & AF_REGEXP)) {
        do {
            cp = parse_to(&dp, ',', EV_STRIP);
            if (quick_wild(cp, (char *)msg)) {
                free_lbuf(nbuf);
                return (0);
            }
        } while (dp != NULL);
    } else {
        len = strlen(msg);
        case_opt = (aflags & AF_CASE) ? 0 : PCRE_CASELESS;
        do {
            cp = parse_to(&dp, ',', EV_STRIP);
            re = pcre_compile(cp, case_opt, &errptr, &erroffset,
                              mudstate.retabs);
            if (re != NULL) {
                subpatterns =
                    pcre_exec(re, NULL, (char *)msg, len, 0, 0,
                              offsets, PCRE_MAX_OFFSETS);
                if (subpatterns >= 0) {
                    XFREE(re, "check_filter.re");
                    free_lbuf(nbuf);
                    return (0);
                }
                XFREE(re, "check_filter.re");
            }
        } while (dp != NULL);
    }
    free_lbuf(nbuf);
    return (1);
}

static char *add_prefix(dbref object, dbref player, int prefix, const char *msg, const char *dflt) {
    int aflags, alen;

    dbref aowner;

    char *buf, *nbuf, *cp, *str;

    GDATA *preserve;

    buf = atr_pget(object, prefix, &aowner, &aflags, &alen);
    if (!*buf) {
        cp = buf;
        safe_str((char *)dflt, buf, &cp);
    } else {
        preserve = save_global_regs("add_prefix_save");
        nbuf = cp = alloc_lbuf("add_prefix");
        str = buf;
        exec(nbuf, &cp, object, player, player,
             EV_FIGNORE | EV_EVAL | EV_TOP, &str, (char **)NULL, 0);
        free_lbuf(buf);
        restore_global_regs("add_prefix_restore", preserve);
        buf = nbuf;
    }
    if (cp != buf) {
        safe_chr(' ', buf, &cp);
    }
    safe_str((char *)msg, buf, &cp);
    return (buf);
}

static char *dflt_from_msg(dbref sender, dbref sendloc) {
    char *tp, *tbuff;

    tp = tbuff = alloc_lbuf("notify_check.fwdlist");
    safe_known_str((char *)"From ", 5, tbuff, &tp);
    if (Good_obj(sendloc))
        safe_name(sendloc, tbuff, &tp);
    else
        safe_name(sender, tbuff, &tp);
    safe_chr(',', tbuff, &tp);
    return tbuff;
}

#ifdef PUEBLO_SUPPORT

/*
 * Do HTML escaping, converting < to &lt;, etc.  'dest' needs to be allocated
 * & freed by the caller.
 *
 * If you're using this to append to a string, you can pass in the
 * safe_{str|chr} (char **) so we can just do the append directly, saving you
 * an alloc_lbuf()...free_lbuf().  If you want us to append from the start of
 * 'dest', just pass in a 0 for 'destp'.
 */

void html_escape(const char *src, char *dest, char **destp) {
    const char *msg_orig;

    char *temp;

    if (destp == 0) {
        temp = dest;
        destp = &temp;
    }
    for (msg_orig = src; msg_orig && *msg_orig; msg_orig++) {
        switch (*msg_orig) {
        case '<':
            safe_known_str("&lt;", 4, dest, destp);
            break;
        case '>':
            safe_known_str("&gt;", 4, dest, destp);
            break;
        case '&':
            safe_known_str("&amp;", 5, dest, destp);
            break;
        case '\"':
            safe_known_str("&quot;", 6, dest, destp);
            break;
        default:
            safe_chr(*msg_orig, dest, destp);
            break;
        }
    }
    **destp = '\0';
}

#endif				/* PUEBLO_SUPPORT */

#define OK_To_Send(p,t)  (!herekey || \
			  ((!Unreal(p) || \
			    ((key & MSG_SPEECH) && Check_Heard((t),(p))) || \
			    ((key & MSG_MOVE) && Check_Noticed((t),(p))) || \
			    ((key & MSG_PRESENCE) && Check_Known((t),(p)))) && \
			   (!Unreal(t) || \
			    ((key & MSG_SPEECH) && Check_Hears((p),(t))) || \
			    ((key & MSG_MOVE) && Check_Notices((p),(t))) || \
			    ((key & MSG_PRESENCE) && Check_Knows((p),(t))))))

void notify_check(dbref target, dbref sender, const char *msg, int key) {
    char *msg_ns, *mp, *tbuff, *tp, *buff;

    char *args[NUM_ENV_VARS];

    dbref aowner, targetloc, recip, obj;

    int i, nargs, aflags, alen, has_neighbors, pass_listen, herekey;

    int check_listens, pass_uselock, is_audible, will_send;

    FWDLIST *fp;

    NUMBERTAB *np;

    GDATA *preserve;

    /*
     * If speaker is invalid or message is empty, just exit
     */

    if (!Good_obj(target) || !msg || !*msg)
        return;

    /*
     * Enforce a recursion limit
     */

    mudstate.ntfy_nest_lev++;
    if (mudstate.ntfy_nest_lev >= mudconf.ntfy_nest_lim) {
        mudstate.ntfy_nest_lev--;
        return;
    }
    /*
     * If we want NOSPOOF output, generate it.  It is only needed if we
     * are sending the message to the target object
     */

    if (key & MSG_ME) {
        mp = msg_ns = alloc_lbuf("notify_check");
        if (Nospoof(target) &&
                (target != sender) &&
                (target != mudstate.curr_enactor) &&
                (target != mudstate.curr_player)) {
            if (sender != Owner(sender)) {
                if (sender != mudstate.curr_enactor) {
                    safe_tmprintf_str(msg_ns, &mp,
                                     "[%s(#%d){%s}<-(#%d)] ",
                                     Name(sender), sender,
                                     Name(Owner(sender)),
                                     mudstate.curr_enactor);
                } else {
                    safe_tmprintf_str(msg_ns, &mp,
                                     "[%s(#%d){%s}] ",
                                     Name(sender), sender,
                                     Name(Owner(sender)));
                }
            } else if (sender != mudstate.curr_enactor) {
                safe_tmprintf_str(msg_ns, &mp,
                                 "[%s(#%d)<-(#%d)] ",
                                 Name(sender), sender,
                                 mudstate.curr_enactor);
            } else {
                safe_tmprintf_str(msg_ns, &mp,
                                 "[%s(#%d)] ", Name(sender), sender);
            }
        }
        safe_str((char *)msg, msg_ns, &mp);
    } else {
        msg_ns = NULL;
    }

    /*
     * msg contains the raw message, msg_ns contains the NOSPOOFed msg
     */

    s_Accessed(target);
    check_listens = Halted(target) ? 0 : 1;
    herekey = key & (MSG_SPEECH | MSG_MOVE | MSG_PRESENCE);
    will_send = OK_To_Send(sender, target);

    switch (Typeof(target)) {
    case TYPE_PLAYER:
        if (will_send) {
#ifndef PUEBLO_SUPPORT
            if (key & MSG_ME)
                raw_notify(target, msg_ns);
#else
            if (key & MSG_ME) {
                if (key & MSG_HTML) {
                    raw_notify_html(target, msg_ns);
                } else {
                    if (Html(target)) {
                        char *msg_ns_escaped;

                        msg_ns_escaped =
                            alloc_lbuf
                            ("notify_check_escape");
                        html_escape(msg_ns,
                                    msg_ns_escaped, 0);
                        raw_notify(target,
                                   msg_ns_escaped);
                        free_lbuf(msg_ns_escaped);
                    } else {
                        raw_notify(target, msg_ns);
                    }
                }
            }
#endif				/* ! PUEBLO_SUPPORT */
            if (!mudconf.player_listen)
                check_listens = 0;
        }
    case TYPE_THING:
    case TYPE_ROOM:

        /*
         * If we're in a pipe, objects can receive raw_notify if
         * they're not a player (players were already notified
         * above).
         */

        if (mudstate.inpipe && !isPlayer(target) && will_send) {
            raw_notify(target, msg_ns);
        }
        /*
         * Forward puppet message if it is for me
         */

        has_neighbors = Has_location(target);
        targetloc = where_is(target);
        is_audible = Audible(target);

        if (will_send && (key & MSG_ME) &&
                Puppet(target) &&
                (target != Owner(target)) &&
                ((key & MSG_PUP_ALWAYS) ||
                 ((targetloc != Location(Owner(target))) &&
                  (targetloc != Owner(target))))) {

            tp = tbuff = alloc_lbuf("notify_check.puppet");
            safe_name(target, tbuff, &tp);
            safe_known_str((char *)"> ", 2, tbuff, &tp);
            safe_str(msg_ns, tbuff, &tp);

            /*
             * Criteria for redirection of a puppet is based on
             * the "normal" conditions for hearing and not
             * conditions based on who the target of the
             * redirection. Use of raw_notify() means that
             * recursion is avoided.
             */
            if (H_Redirect(target)) {
                np = (NUMBERTAB *) nhashfind(target,
                                             &mudstate.redir_htab);
                if (np && Good_obj(np->num)) {
                    raw_notify(Owner(np->num), tbuff);
                }
            } else {
                raw_notify(Owner(target), tbuff);
            }
            free_lbuf(tbuff);
        }
        /*
         * Make sure that we're passing an empty set of global
         * registers to the evaluations we are going to run. We are
         * specifically not calling a save, since that doesn't empty
         * the registers.
         */
        preserve = mudstate.rdata;
        mudstate.rdata = NULL;

        /*
         * Check for @Listen match if it will be useful
         */

        pass_listen = 0;
        nargs = 0;
        if (will_send && check_listens &&
                (key & (MSG_ME | MSG_INV_L)) && H_Listen(target)) {
            tp = atr_get(target, A_LISTEN, &aowner, &aflags,
                         &alen);
            if (*tp && ((!(aflags & AF_REGEXP)
                         && wild(tp, (char *)msg, args,
                                 NUM_ENV_VARS)) || ((aflags & AF_REGEXP)
                                                    && regexp_match(tp, (char *)msg,
                                                            ((aflags & AF_CASE) ? 0 :
                                                                    PCRE_CASELESS), args,
                                                            NUM_ENV_VARS)))) {
                for (nargs = NUM_ENV_VARS;
                        nargs && (!args[nargs - 1]
                                  || !(*args[nargs - 1])); nargs--);
                pass_listen = 1;
            }
            free_lbuf(tp);
        }
        /*
         * If we matched the @listen or are monitoring, check the * *
         * USE lock
         */

        pass_uselock = 0;
        if (will_send && (key & MSG_ME) && check_listens &&
                (pass_listen || Monitor(target)))
            pass_uselock = could_doit(sender, target, A_LUSE);

        /*
         * Process AxHEAR if we pass LISTEN, USElock and it's for me
         */

        if (will_send && (key & MSG_ME) && pass_listen && pass_uselock) {
            if (sender != target)
                did_it(sender, target,
                       A_NULL, NULL, A_NULL, NULL,
                       A_AHEAR, 0, args, nargs, 0);
            else
                did_it(sender, target,
                       A_NULL, NULL, A_NULL, NULL,
                       A_AMHEAR, 0, args, nargs, 0);
            did_it(sender, target, A_NULL, NULL, A_NULL, NULL,
                   A_AAHEAR, 0, args, nargs, 0);
        }
        /*
         * Get rid of match arguments. We don't need them any more
         */

        if (pass_listen) {
            for (i = 0; i < nargs; i++)
                if (args[i] != NULL)
                    free_lbuf(args[i]);
        }
        /*
         * Process ^-listens if for me, MONITOR, and we pass UseLock
         */

        if (will_send && (key & MSG_ME) && pass_uselock &&
                (sender != target) && Monitor(target)) {
            (void)atr_match(target, sender,
                            AMATCH_LISTEN, (char *)msg, (char *)msg, 0);
        }
        /*
         * Deliver message to forwardlist members. No presence
         * control is done on forwarders; if the target can get it,
         * so can they.
         */

        if (will_send && (key & MSG_FWDLIST) &&
                Audible(target) && H_Fwdlist(target) &&
                check_filter(target, sender, A_FILTER, msg)) {
            tbuff = dflt_from_msg(sender, target);
            buff = add_prefix(target, sender, A_PREFIX,
                              msg, tbuff);
            free_lbuf(tbuff);

            fp = fwdlist_get(target);
            if (fp) {
                for (i = 0; i < fp->count; i++) {
                    recip = fp->data[i];
                    if (!Good_obj(recip) ||
                            (recip == target))
                        continue;
                    notify_check(recip, sender, buff,
                                 (MSG_ME | MSG_F_UP | MSG_F_CONTENTS
                                  | MSG_S_INSIDE));
                }
            }
            free_lbuf(buff);
        }
        /*
         * Deliver message through audible exits. If the exit can get
         * it, we don't do further checking for whatever is beyond
         * it. Otherwise we have to continue checking.
         */

        if (will_send && (key & MSG_INV_EXITS)) {
            DOLIST(obj, Exits(target)) {
                recip = Location(obj);
                if (Audible(obj) && ((recip != target) &&
                                     check_filter(obj, sender, A_FILTER,
                                                  msg))) {
                    buff =
                        add_prefix(obj, target, A_PREFIX,
                                   msg, "From a distance,");
                    notify_check(recip, sender, buff,
                                 MSG_ME | MSG_F_UP | MSG_F_CONTENTS
                                 | MSG_S_INSIDE |
                                 (OK_To_Send(sender,
                                             obj) ? 0 : herekey));
                    free_lbuf(buff);
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

        if (has_neighbors &&
                ((key & MSG_NBR_EXITS) ||
                 ((key & MSG_NBR_EXITS_A) && is_audible))) {

            /*
             * If from inside, we have to add the prefix string
             * of the container.
             */

            if (key & MSG_S_INSIDE) {
                tbuff = dflt_from_msg(sender, target);
                buff = add_prefix(target, sender, A_PREFIX,
                                  msg, tbuff);
                free_lbuf(tbuff);
            } else {
                buff = (char *)msg;
            }

            DOLIST(obj, Exits(Location(target))) {
                recip = Location(obj);
                if (Good_obj(recip) && Audible(obj) &&
                        (recip != targetloc) &&
                        (recip != target) &&
                        check_filter(obj, sender, A_FILTER, msg)) {
                    tbuff = add_prefix(obj, target,
                                       A_PREFIX, buff,
                                       "From a distance,");
                    notify_check(recip, sender, tbuff,
                                 MSG_ME | MSG_F_UP | MSG_F_CONTENTS
                                 | MSG_S_INSIDE |
                                 (OK_To_Send(sender,
                                             obj) ? 0 : herekey));
                    free_lbuf(tbuff);
                }
            }
            if (key & MSG_S_INSIDE) {
                free_lbuf(buff);
            }
        }
        if (Bouncer(target))
            pass_listen = 1;

        /*
         * Deliver message to contents only if target passes check.
         * But things within it must still pass the check.
         */

        if (will_send &&
                ((key & MSG_INV) ||
                 ((key & MSG_INV_L) && pass_listen &&
                  check_filter(target, sender, A_INFILTER, msg)))) {

            /*
             * Don't prefix the message if we were given the
             * MSG_NOPREFIX key.
             */

            if (key & MSG_S_OUTSIDE) {
                buff = add_prefix(target, sender, A_INPREFIX,
                                  msg, "");
            } else {
                buff = (char *)msg;
            }
            DOLIST(obj, Contents(target)) {
                if (obj != target) {
#ifdef PUEBLO_SUPPORT
                    notify_check(obj, sender, buff,
                                 MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE
                                 | (key & MSG_HTML) | herekey);
#else
                    notify_check(obj, sender, buff,
                                 MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE
                                 | herekey);
#endif				/* PUEBLO_SUPPORT */
                }
            }
            if (key & MSG_S_OUTSIDE)
                free_lbuf(buff);
        }
        /*
         * Deliver message to neighbors
         */

        if (has_neighbors &&
                ((key & MSG_NBR) ||
                 ((key & MSG_NBR_A) && is_audible &&
                  check_filter(target, sender, A_FILTER, msg)))) {
            if (key & MSG_S_INSIDE) {
                buff = add_prefix(target, sender, A_PREFIX,
                                  msg, "");
            } else {
                buff = (char *)msg;
            }
            DOLIST(obj, Contents(targetloc)) {
                if ((obj != target) && (obj != targetloc)) {
                    notify_check(obj, sender, buff,
                                 MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE
                                 | herekey);
                }
            }
            if (key & MSG_S_INSIDE) {
                free_lbuf(buff);
            }
        }
        /*
         * Deliver message to container
         */

        if (has_neighbors &&
                ((key & MSG_LOC) ||
                 ((key & MSG_LOC_A) && is_audible &&
                  check_filter(target, sender, A_FILTER, msg)))) {
            if (key & MSG_S_INSIDE) {
                tbuff = dflt_from_msg(sender, target);
                buff = add_prefix(target, sender, A_PREFIX,
                                  msg, tbuff);
                free_lbuf(tbuff);
            } else {
                buff = (char *)msg;
            }
            notify_check(targetloc, sender, buff,
                         MSG_ME | MSG_F_UP | MSG_S_INSIDE | herekey);
            if (key & MSG_S_INSIDE) {
                free_lbuf(buff);
            }
        }
        /*
         * mudstate.rdata should be empty, but empty it just in case
         */
        Free_RegData(mudstate.rdata);
        mudstate.rdata = preserve;
    }
    if (msg_ns)
        free_lbuf(msg_ns);
    mudstate.ntfy_nest_lev--;
}

void notify_except(dbref loc, dbref player, dbref exception, const char *msg, int flags) {
    dbref first;

    if (loc != exception)
        notify_check(loc, player, msg,
                     (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A |
                      flags));
    DOLIST(first, Contents(loc)) {
        if (first != exception) {
            notify_check(first, player, msg,
                         (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | flags));
        }
    }
}

void notify_except2(dbref loc, dbref player, dbref exc1, dbref exc2, const char *msg, int flags) {
    dbref first;

    if ((loc != exc1) && (loc != exc2))
        notify_check(loc, player, msg,
                     (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A |
                      flags));
    DOLIST(first, Contents(loc)) {
        if (first != exc1 && first != exc2) {
            notify_check(first, player, msg,
                         (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | flags));
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * Reporting of CPU information.
 */

#ifndef NO_TIMECHECKING
static void report_timecheck(dbref player, int yes_screen, int yes_log, int yes_clear) {
    int thing, obj_counted;

    long used_msecs, total_msecs;

    struct timeval obj_time;

    if (!(yes_log && (LOG_TIMEUSE & mudconf.log_options) != 0)) {
        yes_log = 0;
        STARTLOG(LOG_ALWAYS, "WIZ", "TIMECHECK")
        log_name(player);
        log_printf(" checks object time use over %d seconds\n",
                   (int)(time(NULL) - mudstate.cpu_count_from));
        ENDLOG
    } else {
        STARTLOG(LOG_ALWAYS, "OBJ", "CPU")
        log_name(player);
        log_printf(" checks object time use over %d seconds\n",
                   (int)(time(NULL) - mudstate.cpu_count_from));
        ENDLOG
    }

    obj_counted = 0;
    total_msecs = 0;

    /*
     * Step through the db. Care only about the ones that are nonzero.
     * And yes, we violate several rules of good programming practice by
     * failing to abstract our log calls. Oh well.
     */

    DO_WHOLE_DB(thing) {
        obj_time = Time_Used(thing);
        if (obj_time.tv_sec || obj_time.tv_usec) {
            obj_counted++;
            used_msecs =
                (obj_time.tv_sec * 1000) +
                (obj_time.tv_usec / 1000);
            total_msecs += used_msecs;
            if (yes_log)
                log_printf("#%d\t%ld\n", thing, used_msecs);
            if (yes_screen)
                raw_notify(player, tmprintf("#%d\t%ld", thing,
                                           used_msecs));
            if (yes_clear)
                obj_time.tv_usec = obj_time.tv_sec = 0;
        }
        s_Time_Used(thing, obj_time);
    }

    if (yes_screen) {
        raw_notify(player,
                   tmprintf
                   ("Counted %d objects using %ld msecs over %d seconds.",
                    obj_counted, total_msecs,
                    (int)(time(NULL) - mudstate.cpu_count_from)));
    }
    if (yes_log) {
        log_printf
        ("Counted %d objects using %ld msecs over %d seconds.",
         obj_counted, total_msecs,
         (int)(time(NULL) - mudstate.cpu_count_from));
        end_log();
    }
    if (yes_clear)
        mudstate.cpu_count_from = time(NULL);
}
#else
static void report_timecheck(dbref player, int yes_screen, int yes_log, int yes_clear) {
    raw_notify(player, "Sorry, this command has been disabled.");
}
#endif				/* ! NO_TIMECHECKING */

void do_timecheck(dbref player, dbref cause, int key) {
    int yes_screen, yes_log, yes_clear;

    yes_screen = yes_log = yes_clear = 0;

    if (key == 0) {
        /*
         * No switches, default to printing to screen and clearing
         * counters
         */
        yes_screen = 1;
        yes_clear = 1;
    } else {
        if (key & TIMECHK_RESET)
            yes_clear = 1;
        if (key & TIMECHK_SCREEN)
            yes_screen = 1;
        if (key & TIMECHK_LOG)
            yes_log = 1;
    }

    report_timecheck(player, yes_screen, yes_log, yes_clear);
}

/*
 * ----------------------------------------------------------------------
 * Miscellaneous startup/stop routines.
 */

void write_pidfile(char *fn) {
    FILE *f;

    if ((f = fopen(fn, "w")) != NULL) {
        fprintf(f, "%d\n", getpid());
        fclose(f);
    } else {
        STARTLOG(LOG_ALWAYS, "PID", "FAIL")
        log_printf("Failed to write pidfile %s\n", fn);
        ENDLOG
    }
}

void do_shutdown(dbref player, dbref cause, int key, char *message) {
    int fd;
    char *msg;

    if (key & SHUTDN_COREDUMP) {
        if (player != NOTHING) {
            raw_broadcast(0, "GAME: Aborted by %s", Name(Owner(player)));
            STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
            log_printf("Abort and coredump by ");
            log_name(player);
            ENDLOG
        }
        /*
         * Don't bother to even shut down the network or dump.
         */
        /*
         * Die. Die now.
         */
        abort();
    }
    if (mudstate.dumping) {
        notify(player, "Dumping. Please try again later.");
        return;
    }
    do_dbck(NOTHING, NOTHING, 0);	/* dump consistent state */
    
    fd = tf_open(mudconf.status_file, O_RDWR | O_CREAT | O_TRUNC);

    if (player != NOTHING) {
        raw_broadcast(0, "GAME: Shutdown by %s", Name(Owner(player)));
        STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
        log_printf("Shutdown by ");
        log_name(player);
        ENDLOG
        msg=XSTRDUP(tmprintf("Shutdown by %s. ",Name(Owner(player))), "do_shutdown");
        (void)write(fd, msg, strlen(msg));
        XFREE(msg, "do_shutdown");
    } else {
        
        raw_broadcast(0, "GAME: Fatal Error: %s", message);
        STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
        log_printf("Fatal error: %s", message);
        ENDLOG
        msg=XSTRDUP("Fatal Error: ", "do_shutdown");
        (void)write(fd, msg, strlen(msg));
        XFREE(msg, "do_shutdown");
    }
    STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
    log_printf("Shutdown status: %s", message);
    ENDLOG
    (void)write(fd, message, strlen(message));
    (void)write(fd, (char *)"\n", 1);
    tf_close(fd);

    /*
     * Set up for normal shutdown
     */

    mudstate.shutdown_flag = 1;
    return;
}

void dump_database_internal(int dump_type) {
    char tmpfile[256], prevfile[256], *c;

    FILE *f = NULL;

    MODULE *mp;

    switch (dump_type) {
    case DUMP_DB_CRASH:
        sprintf(tmpfile, "%s/%s.CRASH", mudconf.dbhome, mudconf.db_file);
        unlink(tmpfile);
        f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
        if (f != NULL) {
            db_write_flatfile(f, F_TINYMUSH,
                              UNLOAD_VERSION | UNLOAD_OUTFLAGS);
            tf_fclose(f);
        } else {
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

        WALK_ALL_MODULES(mp) {
            if (mp->db_write_flatfile) {
                f = db_module_flatfile(mp->modname, 1);
                if (f) {
                    (*(mp->db_write_flatfile)) (f);
                    tf_fclose(f);
                }
            }
            if (mp->dump_database) {
                f = db_module_flatfile(mp->modname, 1);
                if (f) {
                    (*(mp->dump_database)) (f);
                    tf_fclose(f);
                }
            }
        }

        /*
         * Write the game's flatfile
         */

        sprintf(tmpfile, "%s/%s.FLAT", mudconf.dbhome, mudconf.db_file);
        f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
        if (f != NULL) {
            db_write_flatfile(f, F_TINYMUSH,
                              UNLOAD_VERSION | UNLOAD_OUTFLAGS);
            tf_fclose(f);
        } else {
            log_perror("DMP", "FAIL", "Opening flatfile", tmpfile);
        }

        break;
    case DUMP_DB_KILLED:
        sprintf(tmpfile, "%s/%s.KILLED", mudconf.dbhome, mudconf.db_file);
        f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
        if (f != NULL) {
            /*
             * Write a flatfile
             */
            db_write_flatfile(f, F_TINYMUSH,
                              UNLOAD_VERSION | UNLOAD_OUTFLAGS);
            tf_fclose(f);
        } else {
            log_perror("DMP", "FAIL", "Opening killed file",
                       tmpfile);
        }
        break;
    default:
        db_write();
    }

    /*
     * Call modules to write to DBM
     */

    db_lock();
    CALL_ALL_MODULES(db_write, ());
    db_unlock();

    /*
     * Call modules to write to their flat-text database
     */

     WALK_ALL_MODULES(mp) {
        if (mp->dump_database) {
            f = db_module_flatfile(mp->modname, 1);
            if (f) {
                (*(mp->dump_database)) (f);
                tf_fclose(f);
            }
        }
    }
}

void dump_database(void) {
    mudstate.epoch++;
    mudstate.dumping = 1;
    STARTLOG(LOG_DBSAVES, "DMP", "DUMP")
    log_printf("Dumping: %s.#%d#", mudconf.db_file, mudstate.epoch);
    ENDLOG pcache_sync();
    SYNC;
    dump_database_internal(DUMP_DB_NORMAL);
    STARTLOG(LOG_DBSAVES, "DMP", "DONE")
    log_printf("Dump complete: %s.#%d#", mudconf.db_file, mudstate.epoch);
    ENDLOG mudstate.dumping = 0;
}

void fork_and_dump(dbref player, dbref cause, int key) {
    if (*mudconf.dump_msg)
        raw_broadcast(0, "%s", mudconf.dump_msg);

    mudstate.epoch++;
    mudstate.dumping = 1;
    STARTLOG(LOG_DBSAVES, "DMP", "CHKPT")
    if (!key || (key & DUMP_TEXT)) {
        log_printf("SYNCing");
        if (!key || (key & DUMP_STRUCT))
            log_printf(" and ");
    }
    if (!key || (key & DUMP_STRUCT) || (key & DUMP_FLATFILE)) {
        log_printf("Checkpointing: %s.#%d#",
                   mudconf.db_file, mudstate.epoch);
    }
    ENDLOG al_store();	/* Save cached modified attribute list */
    if (!key || (key & DUMP_TEXT))
        pcache_sync();

    if (!(key & DUMP_FLATFILE)) {
        SYNC;
        if ((key & DUMP_OPTIMIZE) ||
                (mudconf.dbopt_interval &&
                 (mudstate.epoch % mudconf.dbopt_interval == 0))) {
            OPTIMIZE;
        }
    }
    if (!key || (key & DUMP_STRUCT) || (key & DUMP_FLATFILE)) {
        if (mudconf.fork_dump) {
            if (mudconf.fork_vfork) {
                mudstate.dumper = vfork();
            } else {
                mudstate.dumper = fork();
            }
        } else {
            mudstate.dumper = 0;
        }
        if (mudstate.dumper == 0) {
            if (key & DUMP_FLATFILE) {
                dump_database_internal(DUMP_DB_FLATFILE);
            } else {
                dump_database_internal(DUMP_DB_NORMAL);
            }
            if (mudconf.fork_dump)
                _exit(0);
        } else if (mudstate.dumper < 0) {
            log_perror("DMP", "FORK", NULL, "fork()");
        }
    }
    if (mudstate.dumper <= 0 || kill(mudstate.dumper, 0) == -1) {
        mudstate.dumping = 0;
        mudstate.dumper = 0;
    }
    if (*mudconf.postdump_msg)
        raw_broadcast(0, "%s", mudconf.postdump_msg);
        
    if (!Quiet(player) && (player != NOTHING))
        notify(player, "Done");
}

static int load_game(void) {
    FILE *f;

    MODULE *mp;

    void (*modfunc) (FILE *);

    STARTLOG(LOG_STARTUP, "INI", "LOAD")
    log_printf("Loading object structures.");
    ENDLOG if (db_read() < 0) {
        STARTLOG(LOG_ALWAYS, "INI", "FATAL")
        log_printf("Error loading object structures.");
        ENDLOG return -1;
    }
    /*
     * Call modules to load data from DBM
     */

    CALL_ALL_MODULES_NOCACHE("db_read", (void), ());

    /*
     * Call modules to load data from their flat-text database
     */

    WALK_ALL_MODULES(mp) {
        if ((modfunc = DLSYM(mp->handle, mp->modname, "load_database", (FILE *))) != NULL) {
            f = db_module_flatfile(mp->modname, 0);
            if (f) {
                (*modfunc) (f);
                tf_fclose(f);
            }
        }
    }

    STARTLOG(LOG_STARTUP, "INI", "LOAD")
    log_printf("Load complete.");
    ENDLOG return (0);
}

/* match a list of things, using the no_command flag */

int list_check(dbref thing, dbref player, char type, char *str, char *raw_str, int check_parent, int *stop_status) {
    int match;

    match = 0;
    while (thing != NOTHING) {
        if ((thing != player) &&
                (atr_match(thing, player, type, str, raw_str,
                           check_parent) > 0)) {
            match = 1;
            if (Stop_Match(thing)) {
                *stop_status = 1;
                return match;
            }
        }
        if (thing != Next(thing)) {
            thing = Next(thing);
        } else {
            thing = NOTHING;	/* make sure we don't
						 * infinite loop */
        }
    }
    return match;
}

int Hearer(dbref thing) {
    char *as, *buff, *s;

    dbref aowner;

    int attr, aflags, alen;

    ATTR *ap;

    if (mudstate.inpipe && (thing == mudstate.poutobj))
        return 1;

    if (Connected(thing) || Puppet(thing) || H_Listen(thing))
        return 1;

    if (!Monitor(thing))
        return 0;

    buff = alloc_lbuf("Hearer");
    atr_push();
    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
        ap = atr_num(attr);
        if (!ap || (ap->flags & AF_NOPROG))
            continue;

        atr_get_str(buff, thing, attr, &aowner, &aflags, &alen);

        /*
         * Make sure we can execute it
         */

        if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
            continue;

        /*
         * Make sure there's a : in it
         */

        for (s = buff + 1; *s && (*s != ':'); s++);
        if (s) {
            free_lbuf(buff);
            atr_pop();
            return 1;
        }
    }
    free_lbuf(buff);
    atr_pop();
    return 0;
}

/*
 * ----------------------------------------------------------------------
 * Write message to logfile.
 */

void do_logwrite(dbref player, dbref cause, int key, char *msgtype, char *message) {
    const char *mt;

    char *msg, *p;

    /*
     * If we don't have both a msgtype and a message, make msgtype LOCAL.
     * Otherwise, truncate msgtype to five characters and capitalize.
     */

    if (!message || !*message) {
        mt = (const char *)"LOCAL";
        msg = msgtype;
    } else {
        if (strlen(msgtype) > 5)
            msgtype[5] = '\0';
        for (p = msgtype; *p; p++)
            *p = toupper(*p);
        mt = (const char *)msgtype;
        msg = message;
    }

    /*
     * Just dump it to the log.
     */

    STARTLOG(LOG_LOCAL, "MSG", mt)
    log_name(player);
    log_printf(": %s", msg);
    ENDLOG notify_quiet(player, "Logged.");
}

/*
 * ----------------------------------------------------------------------
 * Log rotation.
 */

void do_logrotate(dbref player, dbref cause, int key) {
    LOGFILETAB *lp;

    mudstate.mudlognum++;

    if (mainlog_fp == stderr) {
        notify(player,
               "Warning: can't rotate main log when logging to stderr.");
    } else {
        fclose(mainlog_fp);
        rename(mudconf.log_file,
               tmprintf("%s.%ld", mudconf.log_file, (long)mudstate.now));
        logfile_init(mudconf.log_file);
    }

    notify(player, "Logs rotated.");
    STARTLOG(LOG_ALWAYS, "WIZ", "LOGROTATE")
    log_name(player);
    log_printf(": logfile rotation %d", mudstate.mudlognum);
    ENDLOG
    /*
     * Any additional special ones
     */
    for (lp = logfds_table; lp->log_flag; lp++) {
        if (lp->filename && lp->fileptr) {
            fclose(lp->fileptr);
            rename(lp->filename, tmprintf("%s.%ld", lp->filename, (long)mudstate.now));
            lp->fileptr = fopen(lp->filename, "w");
            if (lp->fileptr)
                setbuf(lp->fileptr, NULL);
        }
    }

}


/*
 * ----------------------------------------------------------------------
 * Database and startup stuff.
 */

void do_readcache(dbref player, dbref cause, int key) {
    helpindex_load(player);
    fcache_load(player);
}

static void process_preload(void) {
    dbref thing, parent, aowner;

    int aflags, alen, lev, i;

    char *tstr;

    char tbuf[SBUF_SIZE];

    FWDLIST *fp;

    PROPDIR *pp;

    fp = (FWDLIST *) XMALLOC(sizeof(FWDLIST), "process_preload.fwdlist");
    pp = (PROPDIR *) XMALLOC(sizeof(PROPDIR), "process_preload.propdir");
    tstr = alloc_lbuf("process_preload.string");
    i = 0;
    DO_WHOLE_DB(thing) {

        /*
         * Ignore GOING objects
         */

        if (Going(thing))
            continue;

        /*
         * Look for a FORWARDLIST attribute. Load these before doing
         * anything else, so startup notifications work correctly.
         */

        if (H_Fwdlist(thing)) {
            (void)atr_get_str(tstr, thing, A_FORWARDLIST,
                              &aowner, &aflags, &alen);
            if (*tstr) {
                fp->data = NULL;
                fwdlist_load(fp, GOD, tstr);
                if (fp->count > 0)
                    fwdlist_set(thing, fp);
                if (fp->data) {
                    XFREE(fp->data,
                          "process_preload.fwdlist_data");
                }
            }
        }
        /*
         * Ditto for PROPDIRs
         */

        if (H_Propdir(thing)) {
            (void)atr_get_str(tstr, thing, A_PROPDIR,
                              &aowner, &aflags, &alen);
            if (*tstr) {
                pp->data = NULL;
                propdir_load(pp, GOD, tstr);
                if (pp->count > 0)
                    propdir_set(thing, pp);
                if (pp->data) {
                    XFREE(pp->data,
                          "process_preload.propdir_data");
                }
            }
        }
        do_top(10);

        /*
         * Look for STARTUP and DAILY attributes on parents.
         */

        ITER_PARENTS(thing, parent, lev) {
            if (H_Startup(thing)) {
                did_it(Owner(thing), thing,
                       A_NULL, NULL, A_NULL, NULL,
                       A_STARTUP, 0, (char **)NULL, 0, 0);
                /*
                 * Process queue entries as we add them
                 */

                do_second();
                do_top(10);
                break;
            }
        }

        ITER_PARENTS(thing, parent, lev) {
            if (Flags2(thing) & HAS_DAILY) {
                sprintf(tbuf, "0 %d * * *",
                        mudconf.events_daily_hour);
                call_cron(thing, thing, A_DAILY, tbuf);
                break;
            }
        }

    }
    XFREE(fp, "process_preload.fwdlist");
    XFREE(pp, "process_preload.propdir");
    free_lbuf(tstr);
}

/*
 * ---------------------------------------------------------------------------
 * info: display info about the file being read or written.
 */

void info(int fmt, int flags, int ver) {
    const char *cp;

    switch (fmt) {
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
    mainlog_printf("%s version %d:", cp, ver);
    if (flags & V_ZONE)
        mainlog_printf(" Zone");
    if (flags & V_LINK)
        mainlog_printf(" Link");
    if (flags & V_GDBM)
        mainlog_printf(" GDBM");
    if (flags & V_ATRNAME)
        mainlog_printf(" AtrName");
    if (flags & V_ATRKEY) {
        if ((fmt == F_MUSH) && (ver == 2))
            mainlog_printf(" ExtLocks");
        else
            mainlog_printf(" AtrKey");
    }
    if (flags & V_PARENT)
        mainlog_printf(" Parent");
    if (flags & V_COMM)
        mainlog_printf(" Comm");
    if (flags & V_ATRMONEY)
        mainlog_printf(" AtrMoney");
    if (flags & V_XFLAGS)
        mainlog_printf(" ExtFlags");
    if (flags & V_3FLAGS)
        mainlog_printf(" MoreFlags");
    if (flags & V_POWERS)
        mainlog_printf(" Powers");
    if (flags & V_QUOTED)
        mainlog_printf(" QuotedStr");
    if (flags & V_TQUOTAS)
        mainlog_printf(" TypedQuotas");
    if (flags & V_TIMESTAMPS)
        mainlog_printf(" Timestamps");
    if (flags & V_VISUALATTRS)
        mainlog_printf(" VisualAttrs");
    if (flags & V_CREATETIME)
        mainlog_printf(" CreateTime");
    mainlog_printf("\n");
}

void usage(char *prog) {
    mainlog_printf("Usage: %s [options] gdbm-file [< in-file] [> out-file]\n", prog);
    mainlog_printf("   Available flags are:\n");
    mainlog_printf("      -c <filename> - Config file     -C - Perform consistency check\n");
    mainlog_printf("      -d <path> - Data directory      -D <filename> - gdbm database\n");
    mainlog_printf("      -r <filename> - gdbm crash db\n");
    mainlog_printf("      -G - Write in gdbm format       -g - Write in flat file format\n");
    mainlog_printf("      -K - Store key as an attribute  -k - Store key in the header\n");
    mainlog_printf("      -L - Include link information   -l - Don't include link information\n");
    mainlog_printf("      -M - Store attr map if GDBM     -m - Don't store attr map if GDBM\n");
    mainlog_printf("      -N - Store name as an attribute -n - Store name in the header\n");
    mainlog_printf("      -P - Include parent information -p - Don't include parent information\n");
    mainlog_printf("      -W - Write the output file  b   -w - Don't write the output file.\n");
    mainlog_printf("      -X - Create a default GDBM db   -x - Create a default flat file db\n");
    mainlog_printf("      -Z - Include zone information   -z - Don't include zone information\n");
    mainlog_printf("      -<number> - Set output version number\n");
}


void recover(char *flat) {
    int db_ver, db_format, db_flags, setflags, clrflags, ver;
    MODULE *mp;
    void (*modfunc) (FILE *);
    FILE *f;

    vattr_init();
    if (init_gdbm_db(mudconf.db_file) < 0) {
        mainlog_printf("Can't open GDBM file\n");
        exit(1);
    }
        
    db_lock();

    f = fopen(flat, "r");
    db_read_flatfile(f, &db_format, &db_ver, &db_flags);
    fclose(f);

    /*
     * Call modules to load their flatfiles
     */

    WALK_ALL_MODULES(mp) {
        if ((modfunc = DLSYM(mp->handle, mp->modname, "db_read_flatfile", (FILE *))) != NULL) {
            f = db_module_flatfile(mp->modname, 0);
            if (f) {
                (*modfunc) (f);
                tf_fclose(f);
            }
        }
    }
        
    db_ver = OUTPUT_VERSION;
    setflags = OUTPUT_FLAGS;
    clrflags = 0xffffffff;
    db_flags = (db_flags & ~clrflags) | setflags;
    db_write();

    /*
     * Call all modules to write to GDBM
     */

    CALL_ALL_MODULES_NOCACHE("db_write", (void), ());
        
    db_unlock();
    CLOSE;
}

int dbconvert(int argc, char *argv[]) {
    int setflags, clrflags, ver;

    int db_ver, db_format, db_flags, do_check, do_write;

    int c, dbclean, errflg = 0;

    char *opt_conf = (char *)DEFAULT_CONFIG_FILE;

    char *opt_datadir = (char *)DEFAULT_DATABASE_HOME;

    char *opt_gdbmfile = (char *)DEFAULT_CONFIG_FILE;

    FILE *f;

    MODULE *mp;

    void (*modfunc) (FILE *);

    logfile_init(NULL);

    /*
     * Decide what conversions to do and how to format the output file
     */

    setflags = clrflags = ver = do_check = 0;
    do_write = 1;
    dbclean = V_DBCLEAN;

    while ((c = getopt(argc, argv, "c:d:D:CqGgZzLlNnKkPpWwXx0123456789")) != -1) {
        switch (c) {
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
            setflags |= V_GDBM;
            break;
        case 'g':
            clrflags |= V_GDBM;
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
            ver = ver * 10 + (c - '0');
            break;
        default:
            errflg++;
        }
    }

    if (errflg || optind >= argc) {
        usage(argv[0]);
        exit(1);
    }
    LTDL_SET_PRELOADED_SYMBOLS();
    lt_dlinit();
    pool_init(POOL_LBUF, LBUF_SIZE);
    pool_init(POOL_MBUF, MBUF_SIZE);
    pool_init(POOL_SBUF, SBUF_SIZE);
    pool_init(POOL_BOOL, sizeof(struct boolexp));

    mudconf.dbhome = XSTRDUP(opt_datadir, "argv");
    mudconf.db_file = XSTRDUP(opt_gdbmfile, "argv");

    cf_init();
    mudstate.standalone = 1;
    cf_read(opt_conf);
    mudstate.initializing = 0;

    /*
     * Open the gdbm file
     */

    vattr_init();
    
    printf("OPTIND : %s\n", argv[optind]);
    if (init_gdbm_db(argv[optind]) < 0) {
        mainlog_printf("Can't open GDBM file\n");
        exit(1);
    }
    /*
     * Lock the database
     */

    db_lock();

    /*
     * Go do it
     */

    if (!(setflags & V_GDBM)) {
        db_read();

        /*
         * Call all modules to read from GDBM
         */

        CALL_ALL_MODULES_NOCACHE("db_read", (void), ());

        db_format = F_TINYMUSH;
        db_ver = OUTPUT_VERSION;
        db_flags = OUTPUT_FLAGS;
    } else {
        db_read_flatfile(stdin, &db_format, &db_ver, &db_flags);

        /*
         * Call modules to load their flatfiles
         */

        WALK_ALL_MODULES(mp) {
            if ((modfunc = DLSYM(mp->handle, mp->modname, "db_read_flatfile", (FILE *))) != NULL) {
                f = db_module_flatfile(mp->modname, 0);
                if (f) {
                    (*modfunc) (f);
                    tf_fclose(f);
                }
            }
        }
    }

    mainlog_printf("Input: ");
    info(db_format, db_flags, db_ver);

    if (do_check)
        do_dbck(NOTHING, NOTHING, DBCK_FULL);

    if (do_write) {
        db_flags = (db_flags & ~clrflags) | setflags;
        if (ver != 0)
            db_ver = ver;
        else
            db_ver = 3;
        mainlog_printf("Output: ");
        info(F_TINYMUSH, db_flags, db_ver);
        if (db_flags & V_GDBM) {
            db_write();

            /*
             * Call all modules to write to GDBM
             */

            db_lock();
            CALL_ALL_MODULES_NOCACHE("db_write", (void), ());
            db_unlock();
        } else {
            db_write_flatfile(stdout, F_TINYMUSH,
                              db_ver | db_flags | dbclean);

            /*
             * Call all modules to write to flatfile
             */

            WALK_ALL_MODULES(mp) {
                if ((modfunc =
                            DLSYM(mp->handle, mp->modname,
                                  "db_write_flatfile",
                                  (FILE *))) != NULL) {
                    f = db_module_flatfile(mp->modname, 1);
                    if (f) {
                        (*modfunc) (f);
                        tf_fclose(f);
                    }
                }
            }
        }
    }
    /*
     * Unlock the database
     */

    db_unlock();

    CLOSE;
    exit(0);
}

int main(int argc, char *argv[]) {
    int mindb = 0;

    CMDENT *cmdp;

    int i, c;

    int errflg = 0, gotcfg = 0;
    
    pid_t pid;

    char *s, *s1, *s2, *s3;

    MODULE *mp;

    char *bp;
    
    FILE *fp;
    
    struct stat sb1, sb2;
    
    MODHASHES *m_htab, *hp;

    MODNHASHES *m_ntab, *np;

    mudstate.initializing = 1;

    /*
     * Try to get the binary name
     */

    s = strrchr(argv[0], (int)'/');
    if (s) {
        s++;
    } else {
        s = argv[0];
    }

    /*
     * If we are called with the name 'dbconvert', do a DB conversion and
     * exit
     */

    if (s && *s && !strcmp(s, "dbconvert")) {
        dbconvert(argc, argv);
    }
#if !defined(TEST_MALLOC) && defined(RAW_MEMTRACKING)
    /*
     * Do this first, before anything gets a chance to allocate memory.
     */
    mudstate.raw_allocs = NULL;
#endif

    /*
     * Parse options
     */

    mudconf.mud_shortname = XSTRDUP(DEFAULT_SHORTNAME, "main_mudconf_mud_shortname");
    mudconf.config_file = XSTRDUP(DEFAULT_CONFIG_FILE, "main_mudconf_mud_config_file");
    mudconf.config_home = XSTRDUP(DEFAULT_CONFIG_HOME, "main_mudconf_mud_config_home");
    mudconf.game_home=getcwd(NULL, 0);

    while ((c = getopt(argc, argv, "c:sr")) != -1) {
        switch (c) {
        case 'c':
            mudconf.config_file = XSTRDUP(optarg, "main_mudconf_mud_config_file");
            mudconf.config_home = XSTRDUP(realpath(dirname(mudconf.config_file), NULL), "main_mudconf_mud_config_home");
            break;
        case 's':
            mindb = 1;
            break;
        case 'r':
            mudstate.restarting = 1;
            break;
        default:
            errflg++;
            break;
        }
    }

    /* Make sure we can read the config file */
    
    fp = fopen(mudconf.config_file, "r");
    
    if(fp != NULL) {
        fclose(fp);
        gotcfg = 1;
    } else {
        fprintf(stderr, "Unable to read configuration file %s.\n", mudconf.config_file);
        gotcfg = 0;
    }    

    if (errflg || gotcfg == 0) {
        fprintf(stderr, "Usage: %s [-s] [-c config_file]\n", argv[0]);
        exit(1);
    }

    /*
     * Abort if someone tried to set the number of global registers to
     * something stupid. Also adjust the character table if we need to.
     */
    if ((MAX_GLOBAL_REGS < 10) || (MAX_GLOBAL_REGS > 36)) {
        fprintf(stderr,
                "You have compiled TinyMUSH with MAX_GLOBAL_REGS defined to be less than 10 or more than 36. Please fix this error and recompile.\n");
        exit(1);
    }
    if (MAX_GLOBAL_REGS < 36) {
        for (i = 0; i < 36 - MAX_GLOBAL_REGS; i++) {
            qidx_chartab[90 - i] = -1;
            qidx_chartab[122 - i] = -1;
        }
    }


    tf_init();
    LTDL_SET_PRELOADED_SYMBOLS();
    lt_dlinit();
    time(&mudstate.start_time);
    mudstate.restart_time = mudstate.start_time;
    time(&mudstate.cpu_count_from);
    pool_init(POOL_LBUF, LBUF_SIZE);
    pool_init(POOL_MBUF, MBUF_SIZE);
    pool_init(POOL_SBUF, SBUF_SIZE);
    pool_init(POOL_BOOL, sizeof(struct boolexp));

    pool_init(POOL_DESC, sizeof(DESC));
    pool_init(POOL_QENTRY, sizeof(BQUE));
    tcache_init();
    pcache_init();
    
    cf_init();

    init_rlimit();
    init_cmdtab();
    init_logout_cmdtab();
    init_flagtab();
    init_powertab();
    init_functab();
    init_attrtab();
    init_version();
    hashinit(&mudstate.player_htab, 250 * HASH_FACTOR, HT_STR);
    hashinit(&mudstate.nref_htab, 5 * HASH_FACTOR, HT_STR);
    nhashinit(&mudstate.qpid_htab, 50 * HASH_FACTOR);
    nhashinit(&mudstate.fwdlist_htab, 25 * HASH_FACTOR);
    nhashinit(&mudstate.propdir_htab, 25 * HASH_FACTOR);
    nhashinit(&mudstate.redir_htab, 5 * HASH_FACTOR);
    nhashinit(&mudstate.objstack_htab, 50 * HASH_FACTOR);
    nhashinit(&mudstate.objgrid_htab, 50 * HASH_FACTOR);
    nhashinit(&mudstate.parent_htab, 5 * HASH_FACTOR);
    nhashinit(&mudstate.desc_htab, 25 * HASH_FACTOR);
    hashinit(&mudstate.vars_htab, 250 * HASH_FACTOR, HT_STR);
    hashinit(&mudstate.structs_htab, 15 * HASH_FACTOR, HT_STR);
    hashinit(&mudstate.cdefs_htab, 15 * HASH_FACTOR, HT_STR);
    hashinit(&mudstate.instance_htab, 15 * HASH_FACTOR, HT_STR);
    hashinit(&mudstate.instdata_htab, 25 * HASH_FACTOR, HT_STR);
    hashinit(&mudstate.api_func_htab, 5 * HASH_FACTOR, HT_STR);
    
    cf_read(mudconf.config_file);
    
    mudconf.log_file = XSTRDUP(tmprintf("%s/%s.log", mudconf.log_home, mudconf.mud_shortname), "main_mudconf_log_file");
    mudconf.pid_file = XSTRDUP(tmprintf("%s/%s.pid", mudconf.pid_home, mudconf.mud_shortname), "main_mudconf_pid_file");    
    mudconf.db_file =  XSTRDUP(tmprintf("%s.db", mudconf.mud_shortname), "main_mudconf_db_file");    
    mudconf.status_file = XSTRDUP(tmprintf("%s/%s.SHUTDOWN", mudconf.log_home, mudconf.mud_shortname), "main_mudconf_status_file");
    
    pid = isrunning(mudconf.pid_file);
    
    if(pid) {
        STARTLOG(LOG_ALWAYS, "INI", "FATAL")
        log_printf("The MUSH already seems to be running at pid %ld.", (long)pid);  
        ENDLOG exit(2);
    }
    
    if(tailfind(mudconf.log_file, "GDBM panic: write error\n")) {
	STARTLOG(LOG_ALWAYS, "INI", "FATAL")
	log_printf("Log indicate the last run ended with GDBM panic: write error");
	ENDLOG
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
        fprintf(stderr, "    mv -f %s %s.old\n\n", mudconf.log_file, mudconf.log_file);
        fprintf(stderr, "If this is all successful, you may type ./Startmush again to\n");
        fprintf(stderr, "restart the MUSH. If the recovery attempt fails, you will\n");
        fprintf(stderr, "need to restore from a previous backup.\n\n");
	exit(2);
    }
    
    if(!mudstate.restarting) {
        s = XSTRDUP(tmprintf("%s/%s.db.RESTART", mudconf.dbhome, mudconf.mud_shortname), "test_restart_db");
        i = open(s, O_RDONLY);
        if(i>=0) {
            close(i);
            STARTLOG(LOG_ALWAYS, "INI", "LOAD")
            log_printf("There is a restart database, %s, present.", s);
            ENDLOG
            if(unlink(s1) != 0) {
                STARTLOG(LOG_ALWAYS, "INI", "FATAL")
                log_printf("Unable to delete : %s, remove it before restarting the MUSH.", s);
                XFREE(s, "test_restart_db");
                ENDLOG
                exit(2);
            } else {
                STARTLOG(LOG_ALWAYS, "INI", "LOAD")
                log_printf("%s deleted.", s);
                ENDLOG
            }
        }
        XFREE(s, "test_restart_db");
    }

    handlestartupflatfiles(HANDLE_FLAT_KILL);
    handlestartupflatfiles(HANDLE_FLAT_CRASH);

    if( mudconf.help_users == NULL)
        mudconf.help_users = XSTRDUP(tmprintf("help %s/help", mudconf.txthome), "main_add_helpfile_help");
    if( mudconf.help_wizards == NULL)
        mudconf.help_wizards = XSTRDUP(tmprintf("wizhelp %s/wizhelp", mudconf.txthome), "main_add_helpfile_wizhelp");
    if( mudconf.help_quick == NULL)
        mudconf.help_quick = XSTRDUP(tmprintf("qhelp %s/qhelp", mudconf.txthome), "main_add_helpfile_qhelp");
    add_helpfile(GOD, (char *)"main:add_helpfile", mudconf.help_users, 1);
    add_helpfile(GOD, (char *)"main:add_helpfile", mudconf.help_wizards, 1);
    add_helpfile(GOD, (char *)"main:add_helpfile", mudconf.help_quick, 1);
            
    if(mudconf.guest_file == NULL) 
        mudconf.guest_file =   XSTRDUP(tmprintf("%s/guest.txt", mudconf.txthome), "main_mudconf_guest_file");
    if( mudconf.conn_file == NULL)
        mudconf.conn_file =    XSTRDUP(tmprintf("%s/connect.txt", mudconf.txthome), "main_mudconf_conn_file");
    if( mudconf.creg_file == NULL)
        mudconf.creg_file =    XSTRDUP(tmprintf("%s/register.txt", mudconf.txthome), "main_mudconf_creg_file");
    if( mudconf.regf_file == NULL)
        mudconf.regf_file =    XSTRDUP(tmprintf("%s/create_reg.txt", mudconf.txthome), "main_mudconf_regf_file");
    if( mudconf.motd_file == NULL)
        mudconf.motd_file =    XSTRDUP(tmprintf("%s/motd.txt", mudconf.txthome), "main_mudconf_motd_file");
    if( mudconf.wizmotd_file == NULL)
        mudconf.wizmotd_file = XSTRDUP(tmprintf("%s/wizmotd.txt", mudconf.txthome), "main_mudconf_wizmotd_file");
    if( mudconf.quit_file == NULL)
        mudconf.quit_file =    XSTRDUP(tmprintf("%s/quit.txt", mudconf.txthome), "main_mudconf_quit_file");
    if( mudconf.down_file == NULL)
        mudconf.down_file =    XSTRDUP(tmprintf("%s/down.txt", mudconf.txthome), "main_mudconf_down_file");
    if( mudconf.full_file == NULL)
        mudconf.full_file =    XSTRDUP(tmprintf("%s/full.txt", mudconf.txthome), "main_mudconf_full_file");
    if( mudconf.site_file == NULL)
        mudconf.site_file =    XSTRDUP(tmprintf("%s/badsite.txt", mudconf.txthome), "main_mudconf_site_file");
    if( mudconf.crea_file == NULL)
        mudconf.crea_file =    XSTRDUP(tmprintf("%s/newuser.txt", mudconf.txthome), "main_mudconf_crea_file");
#ifdef PUEBLO_SUPPORT
    if( mudconf.htmlconn_file == NULL)
        mudconf.htmlconn_file = XSTRDUP(tmprintf("%s/htmlconn.txt", mudconf.txthome), "main_mudconf_htmlconn_file");
#endif

    vattr_init();

    cmdp = (CMDENT *) hashfind((char *)"wizhelp", &mudstate.command_htab);
    if (cmdp)
        cmdp->perms |= CA_WIZARD;

    bp = mudstate.modloaded;
    WALK_ALL_MODULES(mp) {
        if (bp != mudstate.modloaded) {
            safe_mb_chr(' ', mudstate.modloaded, &bp);
        }
        safe_mb_str(mp->modname, mudstate.modloaded, &bp);
    }

    mudconf.exec_path = XSTRDUP(argv[0], "argv");

    fcache_init();
    helpindex_init();

    if (mindb)
        unlink(mudconf.db_file);
    if (init_gdbm_db(mudconf.db_file) < 0) {
        STARTLOG(LOG_ALWAYS, "INI", "FATAL")
        log_printf("Couldn't load text database: %s", mudconf.db_file);
        ENDLOG exit(2);
    }
    
    mudstate.record_players = 0;

    mudstate.loading_db = 1;
    if (mindb) {
        db_make_minimal();
        CALL_ALL_MODULES_NOCACHE("make_minimal", (void), ());
    } else if (load_game() < 0) {
        STARTLOG(LOG_ALWAYS, "INI", "FATAL")
        log_printf("Couldn't load objects.");
        ENDLOG exit(2);
    }
    mudstate.loading_db = 0;

    init_genrand(getpid() | (time(NULL) << 16));
    set_signals();

    /*
     * Do a consistency check and set up the freelist
     */

    if (!Good_obj(GOD) || !isPlayer(GOD)) {
        STARTLOG(LOG_ALWAYS, "CNF", "VRFY")
        log_printf
        ("Fatal error: GOD object #%d is not a valid player.", GOD);
        ENDLOG exit(3);
    }
    do_dbck(NOTHING, NOTHING, 0);

    /*
     * Reset all the hash stats
     */

    hashreset(&mudstate.command_htab);
    hashreset(&mudstate.logout_cmd_htab);
    hashreset(&mudstate.func_htab);
    hashreset(&mudstate.ufunc_htab);
    hashreset(&mudstate.powers_htab);
    hashreset(&mudstate.flags_htab);
    hashreset(&mudstate.attr_name_htab);
    hashreset(&mudstate.vattr_name_htab);
    hashreset(&mudstate.player_htab);
    hashreset(&mudstate.nref_htab);
    nhashreset(&mudstate.desc_htab);
    nhashreset(&mudstate.qpid_htab);
    nhashreset(&mudstate.fwdlist_htab);
    nhashreset(&mudstate.propdir_htab);
    nhashreset(&mudstate.objstack_htab);
    nhashreset(&mudstate.objgrid_htab);
    nhashreset(&mudstate.parent_htab);
    hashreset(&mudstate.vars_htab);
    hashreset(&mudstate.structs_htab);
    hashreset(&mudstate.cdefs_htab);
    hashreset(&mudstate.instance_htab);
    hashreset(&mudstate.instdata_htab);
    hashreset(&mudstate.api_func_htab);

    for (i = 0; i < mudstate.helpfiles; i++)
        hashreset(&mudstate.hfile_hashes[i]);

    WALK_ALL_MODULES(mp) {
        m_htab = DLSYM_VAR(mp->handle, mp->modname,
                           "hashtable", MODHASHES *);
        if (m_htab) {
            for (hp = m_htab; hp->tabname != NULL; hp++) {
                hashreset(hp->htab);
            }
        }
        m_ntab = DLSYM_VAR(mp->handle, mp->modname,
                           "nhashtable", MODNHASHES *);
        if (m_ntab) {
            for (np = m_ntab; np->tabname != NULL; np++) {
                nhashreset(np->htab);
            }
        }
    }

    mudstate.now = time(NULL);

    /*
     * Initialize PCRE tables for locale.
     */
    mudstate.retabs = pcre_maketables();

    /*
     * Go do restart things.
     */

    load_restart_db();

    if (!mudstate.restarting) {
        /*
         * CAUTION: We do this here rather than up at the top of this
         * function because we need to know if we're restarting. If
         * we are, our previous process closed stdout at inception,
         * and therefore we don't need to do so. More importantly, on
         * a restart, the file descriptor normally allocated to
         * stdout could have been used for a player socket
         * descriptor. Thus, closing it like a stream is really,
         * really bad. Moreover, stdin gets closed and its descriptor
         * reused in tf_init. A double fclose of stdin would be a
         * really bad idea.
         */ 
        //fclose(stdout);
    }
    /*
     * We have to do an update, even though we're starting up, because
     * there may be players connected from a restart, as well as objects.
     */
    CALL_ALL_MODULES_NOCACHE("cleanup_startup", (void), ());

    /*
     * You must do your startups AFTER you load your restart database, or
     * softcode that depends on knowing who is connected and so forth
     * will be hosed.
     */
    process_preload();

    if (!(getppid()==1)) {
        int forkstatus;
        
        forkstatus=fork();
        
        if(forkstatus<0) {
            STARTLOG(LOG_STARTUP, "INI", "FORK")
            log_printf("Unable to fork, %s", strerror(errno));
            ENDLOG
        } else if(forkstatus>0) {
            exit(0);
        } else {
            setsid();
            umask(027);
            chdir(mudconf.game_home);
        }
        
    }

    write_pidfile(mudconf.pid_file);
    logfile_init(mudconf.log_file);

    STARTLOG(LOG_STARTUP, "INI", "LOAD")
    log_printf("Startup processing complete. (Process ID : %d)",  getpid());
    ENDLOG
    
    if (!mudstate.restarting) {
        /* Cosmetic, force a newline to stderr to clear console logs */
        fprintf(stderr, "\n");
        fflush(stderr);
        fflush(stdout);
        freopen(DEV_NULL, "w", stdout);
        freopen(DEV_NULL, "w", stderr);
    }

    /*
     * Startup is done.
     */

    mudstate.initializing = 0;
    mudstate.running = 1;

    /*
     * Clear all reference flags in the cache-- what happens when the
     * game loads is NOT representative of normal cache behavior :)
     * Neither is creating a new db, but in that case the objects exist
     * only in the cache...
     */
    if (!mindb)
        cache_reset();

    /*
     * Start the DNS and identd lookup slave process
     */

    boot_slave();

    /*
     * This must happen after startups are run, in order to get a really
     * good idea of what's actually out there.
     */
    do_hashresize(GOD, GOD, 0);
    STARTLOG(LOG_STARTUP, "INI", "LOAD")
    log_printf("Cleanup completed.");
    ENDLOG if (mudstate.restarting) {
        raw_broadcast(0, "GAME: Restart finished.");
    }
#ifdef MCHECK
    mtrace();
#endif

    /*
     * go do it
     */

    init_timer();
    shovechars(mudconf.port);

#ifdef MCHECK
    muntrace();
#endif

    close_sockets(0, (char *)"Going down - Bye");
    dump_database();
    CLOSE;

    if (slave_socket != -1) {
        shutdown(slave_socket, 2);
        close(slave_socket);
        slave_socket = -1;
    }
    if (slave_pid != 0) {
        kill(slave_pid, SIGKILL);
    }
    exit(0);
}

static void init_rlimit(void) {
#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
    struct rlimit *rlp;

    rlp = (struct rlimit *)XMALLOC(sizeof(struct rlimit), "rlimit");

    if (getrlimit(RLIMIT_NOFILE, rlp)) {
        log_perror("RLM", "FAIL", NULL, "getrlimit()");
        free_lbuf(rlp);
        return;
    }
    rlp->rlim_cur = rlp->rlim_max;
    if (setrlimit(RLIMIT_NOFILE, rlp))
        log_perror("RLM", "FAIL", NULL, "setrlimit()");
    XFREE(rlp, "rlimit");
#else
#if defined(_SEQUENT_) && defined(NUMFDS_LIMIT)
    setdtablesize(NUMFDS_LIMIT);
#endif				/* Sequent and unlimiting #define'd */
#endif				/* HAVE_SETRLIMIT */
}
