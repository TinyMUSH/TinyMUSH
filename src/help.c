/* help.c - commands for giving help */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "help.h"		/* required by code */

int
helpindex_read(htab, filename)
HASHTAB *htab;

char *filename;
{
    help_indx entry;

    char *p;

    int count;

    FILE *fp;

    struct help_entry *htab_entry;

    /*
     * Let's clean out our hash table, before we throw it away.
     */
    for (p = hash_firstkey(htab); p; p = hash_nextkey(htab))
    {
        if (!(hashfindflags(p, htab) & HASH_ALIAS))
        {
            htab_entry = (struct help_entry *)hashfind(p, htab);
            XFREE(htab_entry, "helpindex_read.hent0");
        }
    }

    hashflush(htab, 0);

    if ((fp = tf_fopen(filename, O_RDONLY)) == NULL)
    {
        STARTLOG(LOG_PROBLEMS, "HLP", "RINDX")
        log_printf("Can't open %s for reading.", filename);
        ENDLOG return -1;
    }
    count = 0;
    while ((fread((char *)&entry, sizeof(help_indx), 1, fp)) == 1)
    {

        /*
         * Lowercase the entry and add all leftmost substrings.
         * * Substrings already added will be rejected by hashadd.
         */
        for (p = entry.topic; *p; p++)
            *p = tolower(*p);

        htab_entry =
            (struct help_entry *)XMALLOC(sizeof(struct help_entry),
                                         "helpindex_read.hent1");

        htab_entry->pos = entry.pos;
        htab_entry->len = entry.len;

        if (hashadd(entry.topic, (int *)htab_entry, htab, 0) == 0)
        {
            count++;
            while (p > (entry.topic + 1))
            {
                p--;
                *p = '\0';
                if (!isspace(*(p - 1)))
                {
                    if (hashadd(entry.topic,
                                (int *)htab_entry, htab,
                                HASH_ALIAS) == 0)
                    {
                        count++;
                    }
                    else
                    {
                        /*
                         * It didn't make it into the hash
                         * * table
                         */
                        break;
                    }
                }
            }
        }
        else
        {
            STARTLOG(LOG_ALWAYS, "HLP", "RINDX")
            log_printf("Topic already exists: %s",
                       entry.topic);
            ENDLOG XFREE(htab_entry, "helpindex_read.hent1");
        }
    }
    tf_fclose(fp);
    hashreset(htab);
    return count;
}

void
helpindex_load(player)
dbref player;
{
    int i;

    char buf[SBUF_SIZE + 8];

    if (!mudstate.hfiletab)
    {
        if ((player != NOTHING) && !Quiet(player))
            notify(player,
                   "No indexed files have been configured.");
        return;
    }

    for (i = 0; i < mudstate.helpfiles; i++)
    {
        sprintf(buf, "%s.indx", mudstate.hfiletab[i]);
        helpindex_read(&mudstate.hfile_hashes[i], buf);
    }

    if ((player != NOTHING) && !Quiet(player))
        notify(player, "Indexed file cache updated.");
}


void
NDECL(helpindex_init)
{
    /*
     * We do not need to do hashinits here, as this will already have
     * * been done by the add_helpfile() calls.
     */

    helpindex_load(NOTHING);
}

void
help_write(player, topic, htab, filename, eval)
dbref player;

char *topic, *filename;

HASHTAB *htab;

int eval;
{
    FILE *fp;

    char *p, *line, *result, *str, *bp;

    int entry_offset, entry_length;

    struct help_entry *htab_entry;

    char matched;

    char *topic_list, *buffp;

    if (*topic == '\0')
        topic = (char *)"help";
    else
        for (p = topic; *p; p++)
            *p = tolower(*p);
    htab_entry = (struct help_entry *)hashfind(topic, htab);
    if (htab_entry)
    {
        entry_offset = htab_entry->pos;
        entry_length = htab_entry->len;
    }
    else if (strpbrk(topic, "*?\\"))
    {
        matched = 0;
        for (result = hash_firstkey(htab); result != NULL;
                result = hash_nextkey(htab))
        {
            if (!(hashfindflags(result, htab) & HASH_ALIAS) &&
                    quick_wild(topic, result))
            {
                if (matched == 0)
                {
                    matched = 1;
                    topic_list = alloc_lbuf("help_write");
                    buffp = topic_list;
                }
                safe_str(result, topic_list, &buffp);
                safe_chr(' ', topic_list, &buffp);
                safe_chr(' ', topic_list, &buffp);
            }
        }
        if (matched == 0)
            notify(player, tprintf("No entry for '%s'.", topic));
        else
        {
            notify(player,
                   tprintf("Here are the entries which match '%s':",
                           topic));
            *buffp = '\0';
            notify(player, topic_list);
            free_lbuf(topic_list);
        }
        return;
    }
    else
    {
        notify(player, tprintf("No entry for '%s'.", topic));
        return;
    }
    if ((fp = tf_fopen(filename, O_RDONLY)) == NULL)
    {
        notify(player,
               "Sorry, that function is temporarily unavailable.");
        STARTLOG(LOG_PROBLEMS, "HLP", "OPEN")
        log_printf("Can't open %s for reading.", filename);
        ENDLOG return;
    }
    if (fseek(fp, entry_offset, 0) < 0L)
    {
        notify(player,
               "Sorry, that function is temporarily unavailable.");
        STARTLOG(LOG_PROBLEMS, "HLP", "SEEK")
        log_printf("Seek error in file %s.", filename);
        ENDLOG tf_fclose(fp);
        return;
    }
    line = alloc_lbuf("help_write");
    if (eval)
    {
        result = alloc_lbuf("help_write.2");
    }
    for (;;)
    {
        if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
            break;
        if (line[0] == '&')
            break;
        for (p = line; *p != '\0'; p++)
            if (*p == '\n')
                *p = '\0';
        if (eval)
        {
            str = line;
            bp = result;
            exec(result, &bp, 0, player, player,
                 EV_NO_COMPRESS | EV_FIGNORE | EV_EVAL, &str,
                 (char **)NULL, 0);
            notify(player, result);
        }
        else
            notify(player, line);
    }
    tf_fclose(fp);
    free_lbuf(line);
    if (eval)
    {
        free_lbuf(result);
    }
}

/* ---------------------------------------------------------------------------
 * help_helper: Write entry into a buffer for a function.
 */

void
help_helper(player, hf_num, eval, topic, buff, bufc)
dbref player;

int hf_num, eval;

char *topic, *buff, **bufc;
{
    char tbuf[SBUF_SIZE + 8];

    char tname[LBUF_SIZE];

    char *p, *q, *line, *result, *str, *bp;

    struct help_entry *htab_entry;

    int entry_offset, entry_length, count;

    FILE *fp;

    if (hf_num >= mudstate.helpfiles)
    {
        STARTLOG(LOG_BUGS, "BUG", "HELP")
        log_printf("Unknown help file number: %d", hf_num);
        ENDLOG safe_str((char *)"#-1 NOT FOUND", buff, bufc);
        return;
    }

    if (!topic || !*topic)
    {
        strcpy(tname, (char *)"help");
    }
    else
    {
        for (p = topic, q = tname; *p; p++, q++)
            *q = tolower(*p);
        *q = '\0';
    }
    htab_entry = (struct help_entry *)hashfind(tname,
                 &mudstate.hfile_hashes[hf_num]);
    if (!htab_entry)
    {
        safe_str((char *)"#-1 NOT FOUND", buff, bufc);
        return;
    }
    entry_offset = htab_entry->pos;
    entry_length = htab_entry->len;

    sprintf(tbuf, "%s.txt", mudstate.hfiletab[hf_num]);
    if ((fp = tf_fopen(tbuf, O_RDONLY)) == NULL)
    {
        STARTLOG(LOG_PROBLEMS, "HLP", "OPEN")
        log_printf("Can't open %s for reading.", tbuf);
        ENDLOG safe_str((char *)"#-1 ERROR", buff, bufc);
        return;
    }
    if (fseek(fp, entry_offset, 0) < 0L)
    {
        STARTLOG(LOG_PROBLEMS, "HLP", "SEEK")
        log_printf("Seek error in file %s.", tbuf);
        ENDLOG tf_fclose(fp);
        safe_str((char *)"#-1 ERROR", buff, bufc);
        return;
    }

    line = alloc_lbuf("help_helper");
    if (eval)
    {
        result = alloc_lbuf("help_helper.2");
    }
    count = 0;
    for (;;)
    {
        if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
            break;
        if (line[0] == '&')
            break;
        for (p = line; *p != '\0'; p++)
            if (*p == '\n')
                *p = '\0';
        if (count > 0)
        {
            safe_crlf(buff, bufc);
        }
        if (eval)
        {
            str = line;
            bp = result;
            exec(result, &bp, 0, player, player,
                 EV_NO_COMPRESS | EV_FIGNORE | EV_EVAL, &str,
                 (char **)NULL, 0);
            safe_str(result, buff, bufc);
        }
        else
        {
            safe_str(line, buff, bufc);
        }
        count++;
    }
    tf_fclose(fp);
    free_lbuf(line);
    if (eval)
    {
        free_lbuf(result);
    }
}

/* ---------------------------------------------------------------------------
 * do_help: display information from new-format news and help files
 */

void
do_help(player, cause, key, message)
dbref player, cause;

int key;

char *message;
{
    char tbuf[SBUF_SIZE + 8];

    int hf_num;

    hf_num = key & ~HELP_RAWHELP;

    if (hf_num >= mudstate.helpfiles)
    {
        STARTLOG(LOG_BUGS, "BUG", "HELP")
        log_printf("Unknown help file number: %d", hf_num);
        ENDLOG notify(player, "No such indexed file found.");
        return;
    }

    sprintf(tbuf, "%s.txt", mudstate.hfiletab[hf_num]);
    help_write(player, message, &mudstate.hfile_hashes[hf_num], tbuf,
               (key & HELP_RAWHELP) ? 0 : 1);
}
