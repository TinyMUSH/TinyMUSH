/**
 * @file conf_help.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Configuration help system functions
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
#include <libgen.h>
#include <sys/param.h>

extern CONFDATA mushconf;
extern STATEDATA mushstate;

/**
 * @brief Add a help/news-style file. Only valid during startup.
 *
 * @param player        Dbref of player
 * @param confcmd       Command
 * @param str           Filename
 * @param is_raw        Raw textfile?
 * @return CF_Result
 */
CF_Result add_helpfile(dbref player, char *confcmd, char *str, bool is_raw)
{

    CMDENT *cmdp = NULL;
    HASHTAB *hashes = NULL;
    FILE *fp = NULL;
    char *fcmd = NULL, *fpath = NULL, *newstr = NULL, *tokst = NULL;
    char **ftab = NULL;
    char *s = XMALLOC(MAXPATHLEN, "s");

    /**
     * Make a new string so we won't SEGV if given a constant string
     *
     */
    newstr = XMALLOC(MBUF_SIZE, "newstr");
    XSTRCPY(newstr, str);
    fcmd = strtok_r(newstr, " \t=,", &tokst);
    fpath = strtok_r(NULL, " \t=,", &tokst);
    cf_log(player, "HLP", "LOAD", confcmd, "Loading helpfile %s", basename(fpath));

    if (fpath == NULL)
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Missing path for helpfile %s", fcmd);
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

    if (fcmd[0] == '_' && fcmd[1] == '_')
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s would cause @addcommand conflict", fcmd);
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

    /**
     * Check if file exists in given and standard path
     *
     */
    XSNPRINTF(s, MAXPATHLEN, "%s.txt", fpath);
    fp = fopen(s, "r");

    if (fp == NULL)
    {
        fpath = XASPRINTF("fpath", "%s/%s", mushconf.txthome, fpath);
        XSNPRINTF(s, MAXPATHLEN, "%s.txt", fpath);
        fp = fopen(s, "r");

        if (fp == NULL)
        {
            cf_log(player, "HLP", "LOAD", confcmd, "Helpfile %s not found", fcmd);
            XFREE(newstr);
            XFREE(s);
            return -1;
        }
    }

    fclose(fp);

    /**
     * Rebuild Index
     *
     */
    if (helpmkindx(player, confcmd, fpath))
    {
        cf_log(player, "HLP", "LOAD", confcmd, "Could not create index for helpfile %s, not loaded.", basename(fpath));
        XFREE(newstr);
        XFREE(s);
        return -1;
    }

    if (strlen(fpath) > SBUF_SIZE)
    {
        cf_log(player, "CNF", "SYNTX", confcmd, "Helpfile %s filename too long", fcmd);
        XFREE(newstr);
        XFREE(s);
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

    /**
     * We may need to grow the helpfiles table, or create it.
     *
     */
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

    /**
     * Add or replace the path to the file.
     *
     */
    if (mushstate.hfiletab[mushstate.helpfiles] != NULL)
    {
        XFREE(mushstate.hfiletab[mushstate.helpfiles]);
    }

    mushstate.hfiletab[mushstate.helpfiles] = XSTRDUP(fpath, "mushstate.hfiletab[mushstate.helpfiles]");

    /**
     * Initialize the associated hashtable.
     *
     */
    hashinit(&mushstate.hfile_hashes[mushstate.helpfiles], 30 * mushconf.hash_factor, HT_STR);
    mushstate.helpfiles++;
    cf_log(player, "HLP", "LOAD", confcmd, "Successfully loaded helpfile %s", basename(fpath));
    XFREE(s);
    XFREE(newstr);
    return 0;
}

/**
 * @brief Add a helpfile
 *
 * @param vp            Unused
 * @param str           Filename
 * @param extra         Unused
 * @param player        Dbref of player
 * @param cmd           Command
 * @return CF_Result
 */
CF_Result cf_helpfile(int *vp, char *str, long extra, dbref player, char *cmd)
{
    return add_helpfile(player, cmd, str, 0);
}

/**
 * @brief Add a raw helpfile
 *
 * @param vp            Unused
 * @param str           Filename
 * @param extra         Unused
 * @param player        Dbref of player
 * @param cmd           Command
 * @return CF_Result
 */
CF_Result cf_raw_helpfile(int *vp, char *str, long extra, dbref player, char *cmd)
{
    return add_helpfile(player, cmd, str, 1);
}

/**
 * @brief Read another config file. Only valid during startup.
 *
 * @param vp        Variable buffer
 * @param str       String buffer
 * @param extra     Extra data
 * @param player    DBref of player
 * @param cmd       Command
 * @return CF_Result
 */
CF_Result cf_include(int *vp, char *str, long extra, dbref player, char *cmd)
{
    FILE *fp = NULL;
    char *cp = NULL, *ap = NULL, *zp = NULL, *buf = NULL;
    int line = 0;

    if (!mushstate.initializing)
    {
        return CF_Failure;
    }

    buf = XSTRDUP(str, "buf");
    fp = fopen(buf, "r");

    if (fp == NULL)
    {
        XFREE(buf);
        buf = XASPRINTF("buf", "%s/%s", mushconf.config_home, str);
        fp = fopen(buf, "r");

        if (fp == NULL)
        {
            cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Config file", str);
            XFREE(buf);
            return CF_Failure;
        }
    }

    log_write(LOG_ALWAYS, "CNF", "INFO", "Reading configuration file : %s", buf);
    mushstate.cfiletab = add_array(mushstate.cfiletab, buf, &mushstate.configfiles);
    XFREE(buf);
    buf = XMALLOC(LBUF_SIZE, "buf");

    if (fgets(buf, LBUF_SIZE, fp) == NULL)
    {
        if (!feof(fp))
        {
            cf_log(player, "CNF", "ERROR", "Line:", "%d - %s", line + 1, "Error while reading configuration file.");
        }

        XFREE(buf);
        fclose(fp);
        return CF_Failure;
    }

    line++;

    while (!feof(fp))
    {
        cp = buf;

        if (*cp == '#')
        {
            if (fgets(buf, LBUF_SIZE, fp) == NULL)
            {
                if (!feof(fp))
                {
                    cf_log(player, "CNF", "ERROR", "Line:", "%d - %s", line + 1, "Error while reading configuration file.");
                }

                XFREE(buf);
                fclose(fp);
                return CF_Failure;
            }

            line++;
            continue;
        }

        /**
         * Not a comment line.  Strip off the NL and any characters
         * following it.  Then, split the line into the command and
         * argument portions (separated by a space).  Also, trim off
         * the trailing comment, if any (delimited by #)
         */

        for (cp = buf; *cp && *cp != '\n'; cp++)
            ;

        /**
         * strip \n
         *
         */
        *cp = '\0';

        for (cp = buf; *cp && isspace(*cp); cp++)
            ;

        /**
         * strip spaces
         *
         */
        for (ap = cp; *ap && !isspace(*ap); ap++)
            ;

        /**
         * skip over command
         *
         */
        if (*ap)
        {
            /**
             * trim command
             *
             */
            *ap++ = '\0';
        }

        for (; *ap && isspace(*ap); ap++)
            ;

        /**
         * skip spaces
         *
         */
        for (zp = ap; *zp && (*zp != '#'); zp++)
            ;

        /**
         * find comment
         *
         */
        if (*zp && !(isdigit(*(zp + 1)) && isspace(*(zp - 1))))
        {
            *zp = '\0';
        }

        /** zap comment, but only if it's not sitting between whitespace and a
         * digit, which traps a case like 'master_room #2'
         *
         */
        for (zp = zp - 1; zp >= ap && isspace(*zp); zp--)
        {
            /**
             * zap trailing spaces
             *
             */
            *zp = '\0';
        }

        /**
         * Skip empty lines (lines with only whitespace or comments)
         *
         */
        if (*cp)
        {
            cf_set(cp, ap, player);
        }

        if (fgets(buf, LBUF_SIZE, fp) == NULL)
        {
            if (!feof(fp))
            {
                cf_log(player, "CNF", "ERROR", "Line:", "%d - %s", line + 1, "Error while reading configuration file.");
            }

            XFREE(buf);
            fclose(fp);
            return CF_Failure;
        }

        line++;
    }

    XFREE(buf);
    fclose(fp);
    return CF_Success;
}
