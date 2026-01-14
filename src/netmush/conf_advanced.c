/**
 * @file conf_advanced.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Advanced configuration handlers
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
#include <fcntl.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <netinet/in.h>

extern CONFDATA mushconf;
extern STATEDATA mushstate;

/**
 * @brief Add an arbitrary field to INFO output.
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Not used
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_infotext(int *vp __attribute__((unused)), char *str, long extra __attribute__((unused)), dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    LINKEDLIST *itp = NULL, *prev = NULL;
    char *fvalue = NULL, *tokst = NULL;
    char *fname = strtok_r(str, " \t=,", &tokst);

    if (tokst)
    {
        for (fvalue = tokst; *fvalue && ((*fvalue == ' ') || (*fvalue == '\t')); fvalue++)
        {
            /**
             * Empty loop
             *
             */
        }
    }
    else
    {
        fvalue = NULL;
    }

    if (!fvalue || !*fvalue)
    {
        for (itp = mushconf.infotext_list, prev = NULL; itp != NULL; itp = itp->next)
        {
            if (!strcasecmp(fname, itp->name))
            {
                XFREE(itp->name);
                XFREE(itp->value);

                if (prev)
                {
                    prev->next = itp->next;
                }
                else
                {
                    mushconf.infotext_list = itp->next;
                }

                XFREE(itp);
                return CF_Partial;
            }
            else
            {
                prev = itp;
            }
        }

        return CF_Partial;
    }

    /**
     * Otherwise we're setting. Replace if we had a previous value.
     *
     */
    for (itp = mushconf.infotext_list; itp != NULL; itp = itp->next)
    {
        if (!strcasecmp(fname, itp->name))
        {
            XFREE(itp->value);
            itp->value = XSTRDUP(fvalue, "itp->value");
            return CF_Partial;
        }
    }

    /**
     * No previous value. Add a node.
     *
     */
    itp = (LINKEDLIST *)XMALLOC(sizeof(LINKEDLIST), "itp");
    itp->name = XSTRDUP(fname, "itp->name");
    itp->value = XSTRDUP(fvalue, "itp->value");
    itp->next = mushconf.infotext_list;
    mushconf.infotext_list = itp;
    return CF_Partial;
}

/**
 * @brief Redirect a log type.
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_divert_log(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *type_str = NULL, *file_str = NULL, *tokst = NULL;
    int f = 0, fd = 0;
    FILE *fptr = NULL;
    LOGFILETAB *tp = NULL, *lp = NULL;

    /**
     * Two args, two args only
     *
     */
    type_str = strtok_r(str, " \t", &tokst);
    file_str = strtok_r(NULL, " \t", &tokst);

    if (!type_str || !file_str)
    {
        cf_log(player, "CNF", "SYNTX", cmd, "Missing pathname to log to.");
        return CF_Failure;
    }

    /**
     * Find the log.
     *
     */
    f = search_nametab(GOD, (NAMETAB *)extra, type_str);

    if (f <= 0)
    {
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Log diversion", str);
        return CF_Failure;
    }

    for (tp = logfds_table; tp->log_flag; tp++)
    {
        if (tp->log_flag == f)
        {
            break;
        }
    }

    if (tp == NULL)
    {
        /**
         * This should never happen!
         *
         */
        cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Logfile table corruption", str);
        return CF_Failure;
    }

    /**
     * We shouldn't have a file open already.
     *
     */
    if (tp->filename != NULL)
    {
        log_write(LOG_STARTUP, "CNF", "DIVT", "Log type %s already diverted: %s", type_str, tp->filename);
        return CF_Failure;
    }

    /**
     * Check to make sure that we don't have this filename open already.
     *
     */
    fptr = NULL;

    for (lp = logfds_table; lp->log_flag; lp++)
    {
        if (lp->filename && !strcmp(file_str, lp->filename))
        {
            fptr = lp->fileptr;
            break;
        }
    }

    /**
     * We don't have this filename yet. Open the logfile.
     *
     */
    if (!fptr)
    {
        fptr = fopen(file_str, "w");

        if (!fptr)
        {
            log_write(LOG_STARTUP, "CNF", "DIVT", "Cannot open logfile: %s", file_str);
            return CF_Failure;
        }

        if ((fd = fileno(fptr)) == -1)
        {
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
            return -1;
        }
#endif
    }

    /**
     * Indicate that this is being diverted.
     *
     */
    tp->fileptr = fptr;
    tp->filename = XSTRDUP(file_str, "tp->filename");
    *vp |= f;
    return CF_Success;
}

/**
 * @brief set or clear bits in a flag word from a namelist.
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_modify_bits(int *vp, char *str, long extra, dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL;
    int f = 0, negate = 0, success = 0, failure = 0;
    /**
     * Walk through the tokens
     *
     */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);

    while (sp != NULL)
    {
        /**
         * Check for negation
         *
         */
        negate = 0;

        if (*sp == '!')
        {
            negate = 1;
            sp++;
        }

        /**
         * Set or clear the appropriate bit
         *
         */
        f = search_nametab(GOD, (NAMETAB *)extra, sp);

        if (f > 0)
        {
            if (negate)
            {
                *vp &= ~f;
            }
            else
            {
                *vp |= f;
            }

            success++;
        }
        else
        {
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
            failure++;
        }

        /**
         * Get the next token
         *
         */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    return cf_status_from_succfail(player, cmd, success, failure);
}

/**
 * @brief Helper function to change xfuncs.
 *
 * @param fn_name   Function name
 * @param fn_ptr    Function pointer
 * @param xfuncs    External functions
 * @param negate    true: remove, false add:
 * @return bool
 */
bool modify_xfuncs(char *fn_name, int (*fn_ptr)(dbref), EXTFUNCS **xfuncs, bool negate)
{
    NAMEDFUNC *np = NULL, **tp = NULL;
    int i = 0;
    EXTFUNCS *xfp = *xfuncs;

    /**
     * If we're negating, just remove it from the list of functions.
     *
     */
    if (negate)
    {
        if (!xfp)
        {
            return false;
        }

        for (i = 0; i < xfp->num_funcs; i++)
        {
            if (!strcmp(xfp->ext_funcs[i]->fn_name, fn_name))
            {
                xfp->ext_funcs[i] = NULL;
                return true;
            }
        }

        return false;
    }

    /**
     * Have we encountered this function before?
     *
     */
    np = NULL;

    for (i = 0; i < xfunctions.count; i++)
    {
        if (!strcmp(xfunctions.func[i]->fn_name, fn_name))
        {
            np = xfunctions.func[i];
            break;
        }
    }

    /**
     * If not, we need to allocate it.
     *
     */
    if (!np)
    {
        np = (NAMEDFUNC *)XMALLOC(sizeof(NAMEDFUNC), "np");
        np->fn_name = (char *)XSTRDUP(fn_name, "np->fn_name");
        np->handler = fn_ptr;
    }

    /**
     * Add it to the ones we know about.
     *
     */
    if (xfunctions.count == 0)
    {
        xfunctions.func = (NAMEDFUNC **)XMALLOC(sizeof(NAMEDFUNC *), "xfunctions.func");
        xfunctions.count = 1;
        xfunctions.func[0] = np;
    }
    else
    {
        tp = (NAMEDFUNC **)XREALLOC(xfunctions.func, (xfunctions.count + 1) * sizeof(NAMEDFUNC *), "tp");
        tp[xfunctions.count] = np;
        xfunctions.func = tp;
        xfunctions.count++;
    }

    /**
     * Do we have an existing list of functions? If not, this is easy.
     *
     */
    if (!xfp)
    {
        xfp = (EXTFUNCS *)XMALLOC(sizeof(EXTFUNCS), "xfp");
        xfp->num_funcs = 1;
        xfp->ext_funcs = (NAMEDFUNC **)XMALLOC(sizeof(NAMEDFUNC *), "xfp->ext_funcs");
        xfp->ext_funcs[0] = np;
        *xfuncs = xfp;
        return true;
    }

    /**
     * See if we have an empty slot to insert into.
     *
     */
    for (i = 0; i < xfp->num_funcs; i++)
    {
        if (!xfp->ext_funcs[i])
        {
            xfp->ext_funcs[i] = np;
            return true;
        }
    }

    /**
     * Guess not. Tack it onto the end.
     *
     */
    tp = (NAMEDFUNC **)XREALLOC(xfp->ext_funcs, (xfp->num_funcs + 1) * sizeof(NAMEDFUNC *), "tp");
    tp[xfp->num_funcs] = np;
    xfp->ext_funcs = tp;
    xfp->num_funcs++;
    return true;
}

/**
 * @brief Parse an extended access list with module callouts.
 *
 * @param perms     Permissions
 * @param xperms    Extendes permissions
 * @param str       String buffer
 * @param ntab      Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result parse_ext_access(int *perms, EXTFUNCS **xperms, char *str, NAMETAB *ntab, dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL, *cp = NULL, *ostr = NULL, *s = NULL;
    int f = 0, negate = 0, success = 0, failure = 0, got_one = 0;
    MODULE *mp = NULL;
    int (*hp)(dbref) = NULL;

    /**
     * Walk through the tokens
     *
     */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);

    while (sp != NULL)
    {
        /**
         * Check for negation
         *
         */
        negate = 0;

        if (*sp == '!')
        {
            negate = 1;
            sp++;
        }

        /**
         * Set or clear the appropriate bit
         *
         */
        f = search_nametab(GOD, ntab, sp);

        if (f > 0)
        {
            if (negate)
            {
                *perms &= ~f;
            }
            else
            {
                *perms |= f;
            }

            success++;
        }
        else
        {
            /**
             * Is this a module callout?
             *
             */
            got_one = 0;

            if (!strncmp(sp, "mod_", 4))
            {
                /**
                 * Split it apart, see if we have anything.
                 *
                 */
                s = XMALLOC(MBUF_SIZE, "s");
                ostr = (char *)XSTRDUP(sp, "ostr");

                if (*(sp + 4) != '\0')
                {
                    cp = strchr(sp + 4, '_');

                    if (cp)
                    {
                        *cp++ = '\0';

                        for (mp = mushstate.modules_list; mp != NULL; mp = mp->next)
                        {
                            got_one = 1;

                            if (!strcmp(sp + 4, mp->modname))
                            {
                                XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, cp);
                                hp = (int (*)(dbref))dlsym(mp->handle, s);

                                if (!hp)
                                {
                                    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Module function", str);
                                    failure++;
                                }
                                else
                                {
                                    if (modify_xfuncs(ostr, hp, xperms, negate))
                                    {
                                        success++;
                                    }
                                    else
                                    {
                                        failure++;
                                    }
                                }

                                break;
                            }
                        }

                        if (!got_one)
                        {
                            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Loaded module", str);
                            got_one = 1;
                        }
                    }
                }

                XFREE(ostr);
                XFREE(s);
            }

            if (!got_one)
            {
                cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
                failure++;
            }
        }

        /**
         * Get the next token
         *
         */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    return cf_status_from_succfail(player, cmd, success, failure);
}

/**
 * @brief Clear flag word and then set from a flags htab.
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_set_flags(int *vp, char *str, long extra __attribute__((unused)), dbref player, char *cmd)
{
    char *sp = NULL, *tokst = NULL;
    FLAGENT *fp = NULL;
    FLAGSET *fset = NULL;
    int success = 0, failure = 0;

    for (sp = str; *sp; sp++)
    {
        *sp = toupper(*sp);
    }

    /**
     * Walk through the tokens
     *
     */
    success = failure = 0;
    sp = strtok_r(str, " \t", &tokst);
    fset = (FLAGSET *)vp;

    while (sp != NULL)
    {
        /**
         * Set the appropriate bit
         *
         */
        fp = (FLAGENT *)hashfind(sp, &mushstate.flags_htab);

        if (fp != NULL)
        {
            if (success == 0)
            {
                (*fset).word1 = 0;
                (*fset).word2 = 0;
                (*fset).word3 = 0;
            }

            if (fp->flagflag & FLAG_WORD3)
            {
                (*fset).word3 |= fp->flagvalue;
            }
            else if (fp->flagflag & FLAG_WORD2)
            {
                (*fset).word2 |= fp->flagvalue;
            }
            else
            {
                (*fset).word1 |= fp->flagvalue;
            }

            success++;
        }
        else
        {
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
            failure++;
        }

        /**
         * Get the next token
         *
         */
        sp = strtok_r(NULL, " \t", &tokst);
    }

    if ((success == 0) && (failure == 0))
    {
        (*fset).word1 = 0;
        (*fset).word2 = 0;
        (*fset).word3 = 0;
        return CF_Success;
    }

    if (success > 0)
    {
        return ((failure == 0) ? CF_Success : CF_Partial);
    }

    return CF_Failure;
}

/**
 * @brief Disallow use of player name/alias.
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_badname(int *vp __attribute__((unused)), char *str, long extra, dbref player __attribute__((unused)), char *cmd __attribute__((unused)))
{
    if (extra)
    {
        badname_remove(str);
    }
    else
    {
        badname_add(str);
    }

    return CF_Success;
}

/**
 * @brief Replacement for inet_addr()
 *
 * inet_addr() does not necessarily do reasonable checking for sane syntax.
 * On certain operating systems, if passed less than four octets, it will
 * cause a segmentation violation. This is unfriendly. We take steps here
 * to deal with it.
 *
 * @param str       IP Address
 * @return in_addr_t
 */
in_addr_t sane_inet_addr(char *str)
{
    int i = 0;
    char *p = str;

    for (i = 1; (p = strchr(p, '.')) != NULL; i++, p++)
        ;

    if (i < 4)
    {
        return INADDR_NONE;
    }
    else
    {
        struct in_addr addr;
        if (inet_pton(AF_INET, str, &addr) == 1)
        {
            return addr.s_addr;
        }
        return INADDR_NONE;
    }
}

/**
 * @brief Update site information
 *
 * @param vp        Variable buffer
 * @param str       String Buffer
 * @param extra     Name Table
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_site(long **vp, char *str, long extra, dbref player, char *cmd)
{
    SITE *site = NULL, *last = NULL, *head = NULL;
    char *addr_txt = NULL, *mask_txt = NULL, *tokst = NULL;
    struct in_addr addr_num, mask_num;
    int mask_bits = 0;

    if ((mask_txt = strchr(str, '/')) == NULL)
    {
        /**
         * Standard IP range and netmask notation.
         *
         */
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

        if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
            return CF_Failure;
        }

        if (((mask_num.s_addr = sane_inet_addr(mask_txt)) == INADDR_NONE) && strcmp(mask_txt, "255.255.255.255"))
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed mask address: %s", mask_txt);
            return CF_Failure;
        }
    }
    else
    {
        /**
         * RFC 1517, 1518, 1519, 1520: CIDR IP prefix notation
         *
         */
        addr_txt = str;
        *mask_txt++ = '\0';
        mask_bits = (int)strtol(mask_txt, (char **)NULL, 10);

        if ((mask_bits > 32) || (mask_bits < 0))
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Mask bits (%d) in CIDR IP prefix out of range.", mask_bits);
            return CF_Failure;
        }
        else if (mask_bits == 0)
        {
            /**
             * can't shift by 32
             *
             */
            mask_num.s_addr = htonl(0);
        }
        else
        {
            mask_num.s_addr = htonl(0xFFFFFFFFU << (32 - mask_bits));
        }

        if ((addr_num.s_addr = sane_inet_addr(addr_txt)) == INADDR_NONE)
        {
            cf_log(player, "CNF", "SYNTX", cmd, "Malformed host address: %s", addr_txt);
            return CF_Failure;
        }
    }

    head = (SITE *)*vp;

    /**
     * Parse the access entry and allocate space for it
     *
     */
    if (!(site = (SITE *)XMALLOC(sizeof(SITE), "site")))
    {
        return CF_Failure;
    }

    /**
     * Initialize the site entry
     *
     */
    site->address.s_addr = addr_num.s_addr;
    site->mask.s_addr = mask_num.s_addr;
    site->flag = (long)extra;
    site->next = NULL;

    /**
     * Link in the entry. Link it at the start if not initializing, at
     * the end if initializing. This is so that entries in the config
     * file are processed as you would think they would be, while entries
     * made while running are processed first.
     *
     */
    if (mushstate.initializing)
    {
        if (head == NULL)
        {
            *vp = (long *)site;
        }
        else
        {
            for (last = head; last->next; last = last->next)
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
