/**
 * @file mail.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module for penn-based mailer system
 * @version 3.1
 * @date 2020-12-28
 * 
 * @copyright Copyright (c) 2020
 * 
 * @note This code was taken from Kalkin's DarkZone code, which was originally
 * taken from PennMUSH 1.50 p10, and has been heavily modified since being
 * included in MUX (and then being imported wholesale into 3.0).
 * 
 */

#include <copyright.h>
#include <config.h>
#include <system.h>

#include <tinymushapi.h>

#include "mail.h"

/* --------------------------------------------------------------------------
 * Configuration.
 */

#define MAIL_STATS 1     /* Mail stats */
#define MAIL_DSTATS 2    /* More mail stats */
#define MAIL_FSTATS 3    /* Even more mail stats */
#define MAIL_DEBUG 4     /* Various debugging options */
#define MAIL_NUKE 5      /* Nuke the post office */
#define MAIL_FOLDER 6    /* Do folder stuff */
#define MAIL_LIST 7      /* List @mail by time */
#define MAIL_READ 8      /* Read @mail message */
#define MAIL_CLEAR 9     /* Clear @mail message */
#define MAIL_UNCLEAR 10  /* Unclear @mail message */
#define MAIL_PURGE 11    /* Purge cleared @mail messages */
#define MAIL_FILE 12     /* File @mail in folders */
#define MAIL_TAG 13      /* Tag @mail messages */
#define MAIL_UNTAG 14    /* Untag @mail messages */
#define MAIL_FORWARD 15  /* Forward @mail messages */
#define MAIL_SEND 16     /* Send @mail messages in progress */
#define MAIL_EDIT 17     /* Edit @mail messages in progress */
#define MAIL_URGENT 18   /* Sends a @mail message as URGENT */
#define MAIL_ALIAS 19    /* Creates an @mail alias */
#define MAIL_ALIST 20    /* Lists @mail aliases */
#define MAIL_PROOF 21    /* Proofs @mail messages in progress */
#define MAIL_ABORT 22    /* Aborts @mail messages in progress */
#define MAIL_QUICK 23    /* Sends a quick @mail message */
#define MAIL_REVIEW 24   /* Reviews @mail sent to a player */
#define MAIL_RETRACT 25  /* Retracts @mail sent to a player */
#define MAIL_CC 26       /* Carbon copy */
#define MAIL_SAFE 27     /* Defines a piece of mail as safe. */
#define MAIL_REPLY 28    /* Replies to a message. */
#define MAIL_REPLYALL 29 /* Replies to all recipients of msg */
#define MAIL_TO 30       /* Set recipient list */
#define MAIL_BCC 31      /* Blind Carbon copy */
#define MAIL_QUOTE 0x100 /* Quote back original in the reply? */

/* Note that we don't use all hex for mail flags, because the upper 3 bits
 * of the switch word gets occupied by SW_<foo>.
 */

#define MALIAS_DESC 1   /* Describes a mail alias */
#define MALIAS_CHOWN 2  /* Changes a mail alias's owner */
#define MALIAS_ADD 3    /* Adds a player to an alias */
#define MALIAS_REMOVE 4 /* Removes a player from an alias */
#define MALIAS_DELETE 5 /* Deletes a mail alias */
#define MALIAS_RENAME 6 /* Renames a mail alias */
#define MALIAS_LIST 8   /* Lists mail aliases */
#define MALIAS_STATUS 9 /* Status of mail aliases */

MODVER mod_mail_version;

NHSHTAB mod_mail_msg_htab;

MODNHASHES mod_mail_nhashtable[] = {
    {"Mail messages", &mod_mail_msg_htab, 50, 8},
    {NULL, NULL, 0, 0}};

struct mod_mail_confstorage
{
    int mail_expiration; /* Number of days to wait to delete mail */
    int mail_freelist;   /* The next free mail number */
    MENT *mail_list;     /* The mail database */
    int mail_db_top;     /* Like db_top */
    int mail_db_size;    /* Like db_size */
} mod_mail_config;

CONF mod_mail_conftable[] = {
    {(char *)"mail_expiration", cf_int, CA_GOD, CA_PUBLIC, &mod_mail_config.mail_expiration, 0},
    {NULL, NULL, 0, 0, NULL, 0}};

static int sign(int);
static void do_mail_flags(dbref, char *, mail_flag, int);
static void send_mail(dbref, dbref, char *, char *, char *, char *, int, mail_flag, int);
static int player_folder(dbref);
static int parse_msglist(char *, struct mail_selector *, dbref);
static int mail_match(struct mail *, struct mail_selector, int);
static int parse_folder(dbref, char *);
static char *status_chars(struct mail *);
static char *status_string(struct mail *);
void add_folder_name(dbref, int, char *);
static int get_folder_number(dbref, char *);
void check_mail(dbref, int, int);
static char *get_folder_name(dbref, int);
static char *mail_list_time(char *);
static char *make_numlist(dbref, char *);
static char *make_namelist(dbref, char *);
static void mail_to_list(dbref, char *, char *, char *, char *, char *, int, int);
static void do_edit_msg(dbref, char *, char *);
static void do_mail_proof(dbref);
static void do_mail_to(dbref, char *, int);
static int do_expmail_start(dbref, char *, char *, char *);
static void do_expmail_stop(dbref, int);
static void do_expmail_abort(dbref);
struct mail *mail_fetch(dbref, int);

#define MALIAS_LEN 100

struct malias
{
    int owner;
    char *name;
    char *desc;
    int numrecep;
    dbref list[MALIAS_LEN];
};

int ma_size = 0;
int ma_top = 0;

struct malias **malias;

/*
 * Handling functions for the database of mail messages.
 */

/*
 * mail_db_grow - We keep a database of mail text, so if we send a message to
 * more than one player, we won't have to duplicate the text.
 */

static void mail_db_grow(int newtop)
{
    int newsize, i;
    MENT *newdb;

    if (newtop <= mod_mail_config.mail_db_top)
    {
        return;
    }

    if (newtop <= mod_mail_config.mail_db_size)
    {
        for (i = mod_mail_config.mail_db_top; i < newtop; i++)
        {
            mod_mail_config.mail_list[i].count = 0;
            mod_mail_config.mail_list[i].message = NULL;
        }

        mod_mail_config.mail_db_top = newtop;
        return;
    }

    if (newtop <= mod_mail_config.mail_db_size + 100)
    {
        newsize = mod_mail_config.mail_db_size + 100;
    }
    else
    {
        newsize = newtop;
    }

    newdb = (MENT *)XMALLOC((newsize + 1) * sizeof(MENT), "newdb");

    if (!newdb)
    {
        log_write_raw(1, "ABORT! mail.c, unable to malloc new db in mail_db_grow().\n");
        abort();
    }

    if (mod_mail_config.mail_list)
    {
        mod_mail_config.mail_list -= 1;
        memcpy((char *)newdb, (char *)mod_mail_config.mail_list,
               (mod_mail_config.mail_db_top + 1) * sizeof(MENT));
        XFREE(mod_mail_config.mail_list);
    }

    mod_mail_config.mail_list = newdb + 1;
    newdb = NULL;

    for (i = mod_mail_config.mail_db_top; i < newtop; i++)
    {
        mod_mail_config.mail_list[i].count = 0;
        mod_mail_config.mail_list[i].message = NULL;
    }

    mod_mail_config.mail_db_top = newtop;
    mod_mail_config.mail_db_size = newsize;
}

/*
 * make_mail_freelist - move the freelist to the first empty slot in the mail
 * * database.
 */

static void make_mail_freelist(void)
{
    int i;

    for (i = 0; i < mod_mail_config.mail_db_top; i++)
    {
        if (mod_mail_config.mail_list[i].message == NULL)
        {
            mod_mail_config.mail_freelist = i;
            return;
        }
    }

    mail_db_grow(i + 1);
    mod_mail_config.mail_freelist = i;
}

/*
 * add_mail_message - adds a new text message to the mail database, and returns
 * * a unique number for that message.
 */

static int add_mail_message(dbref player, char *message)
{
    int number;
    char *atrstr, *execstr, *bp, *str, *s;
    int aflags, alen;
    dbref aowner;

    if (!mod_mail_config.mail_list)
    {
        mail_db_grow(1);
    }

    /*
     * Add an extra bit of protection here
     */
    while (mod_mail_config.mail_list[mod_mail_config.mail_freelist].message != NULL)
    {
        make_mail_freelist();
    }

    number = mod_mail_config.mail_freelist;
    atrstr = atr_get(player, A_SIGNATURE, &aowner, &aflags, &alen);
    execstr = bp = XMALLOC(LBUF_SIZE, "execstr");
    str = atrstr;
    exec(execstr, &bp, player, player, player, EV_STRIP | EV_FCHECK | EV_EVAL, &str, (char **)NULL, 0);
    s = XMALLOC(LBUF_SIZE, "s");
    snprintf(s, LBUF_SIZE, "%s %s", message, execstr);
    mod_mail_config.mail_list[number].message = XSTRDUP(s, "mod_mail_config.mail_list[number].message");
    XFREE(s);
    XFREE(atrstr);
    XFREE(execstr);
    make_mail_freelist();
    return number;
}

/*
 * add_mail_message_nosig - used for reading in old style messages from disk
 */

static int add_mail_message_nosig(char *message)
{
    int number;
    number = mod_mail_config.mail_freelist;

    if (!mod_mail_config.mail_list)
    {
        mail_db_grow(1);
    }

    mod_mail_config.mail_list[number].message = XSTRDUP(message, "mod_mail_config.mail_list[number].message");
    make_mail_freelist();
    return number;
}

/*
 * new_mail_message - used for reading messages in from disk which already have
 * * a number assigned to them.
 */

static void new_mail_message(char *message, int number)
{
    mod_mail_config.mail_list[number].message = XSTRDUP(message, "mod_mail_config.mail_list[number].message");
}

/*
 * add_count - increments the reference count for any particular message
 */

static void add_count(int number)
{
    mod_mail_config.mail_list[number].count++;
}

/*
 * delete_mail_message - decrements the reference count for a message, and
 * * deletes the message if the counter reaches 0.
 */

static void delete_mail_message(int number)
{
    mod_mail_config.mail_list[number].count--;

    if (mod_mail_config.mail_list[number].count < 1)
    {
        XFREE(mod_mail_config.mail_list[number].message);
        mod_mail_config.mail_list[number].message = NULL;
        mod_mail_config.mail_list[number].count = 0;
    }
}

/*
 * get_mail_message - returns the text for a particular number. This text
 * should NOT be modified.
 */

char *get_mail_message(int number)
{
    static char err[] = "MAIL: This mail message does not exist in the database. Please alert your admin.";

    if (mod_mail_config.mail_list[number].message != NULL)
    {
        return mod_mail_config.mail_list[number].message;
    }
    else
    {
        delete_mail_message(number);
        return err;
    }
}

/*-------------------------------------------------------------------------*
 *   User mail functions (these are called from game.c)
 *
 * do_mail - cases
 * do_mail_read - read messages
 * do_mail_list - list messages
 * do_mail_flags - tagging, untagging, clearing, unclearing of messages
 * do_mail_file - files messages into a new folder
 * do_mail_fwd - forward messages to another player(s)
 * do_mail_reply - reply to a message
 * do_mail_purge - purge cleared messages
 * do_mail_change_folder - change current folder
 *-------------------------------------------------------------------------*/

/*
 * Change or rename a folder
 */
void do_mail_change_folder(dbref player, char *fld, char *newname)
{
    int pfld;
    char *p;

    if (!fld || !*fld)
    {
        /*
         * Check mail in all folders
         */
        for (pfld = MAX_FOLDERS; pfld >= 0; pfld--)
            check_mail(player, pfld, 1);

        pfld = player_folder(player);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Current folder is %d [%s].", pfld, get_folder_name(player, pfld));
        return;
    }

    pfld = parse_folder(player, fld);

    if (pfld < 0)
    {
        notify(player, "MAIL: What folder is that?");
        return;
    }

    if (newname && *newname)
    {
        /*
         * We're changing a folder name here
         */
        if (strlen(newname) > FOLDER_NAME_LEN)
        {
            notify(player, "MAIL: Folder name too long");
            return;
        }

        for (p = newname; p && *p; p++)
        {
            if (!isalnum(*p))
            {
                notify(player, "MAIL: Illegal folder name");
                return;
            }
        }

        add_folder_name(player, pfld, newname);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Folder %d now named '%s'", pfld, newname);
    }
    else
    {
        /*
         * Set a new folder
         */
        set_player_folder(player, pfld);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Current folder set to %d [%s].", pfld, get_folder_name(player, pfld));
    }
}

void do_mail_tag(dbref player, char *msglist)
{
    do_mail_flags(player, msglist, M_TAG, 0);
}

void do_mail_safe(dbref player, char *msglist)
{
    do_mail_flags(player, msglist, M_SAFE, 0);
}

void do_mail_clear(dbref player, char *msglist)
{
    do_mail_flags(player, msglist, M_CLEARED, 0);
}

void do_mail_untag(dbref player, char *msglist)
{
    do_mail_flags(player, msglist, M_TAG, 1);
}

void do_mail_unclear(dbref player, char *msglist)
{
    do_mail_flags(player, msglist, M_CLEARED, 1);
}

/*
 * adjust the flags of a set of messages
 */

static void do_mail_flags(dbref player, char *msglist, mail_flag flag, int negate)
{
    struct mail *mp;
    struct mail_selector ms;
    int i = 0, j = 0, folder;

    if (!parse_msglist(msglist, &ms, player))
    {
        return;
    }

    folder = player_folder(player);

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (All(ms) || (Folder(mp) == folder))
        {
            i++;

            if (mail_match(mp, ms, i))
            {
                j++;

                if (negate)
                {
                    mp->read &= ~flag;
                }
                else
                {
                    mp->read |= flag;
                }

                switch (flag)
                {
                case M_TAG:
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Msg #%d %s.", i, negate ? "untagged" : "tagged");
                    break;

                case M_CLEARED:
                    if (Unread(mp) && !negate)
                    {
                        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Unread Msg #%d cleared! Use @mail/unclear %d to recover.", i, i);
                    }
                    else
                    {
                        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Msg #%d %s.", i, negate ? "uncleared" : "cleared");
                    }

                    break;

                case M_SAFE:
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Msg #%d marked safe.", i);
                    break;
                }
            }
        }
    }

    if (!j)
    {
        /*
         * ran off the end of the list without finding anything
         */
        notify(player, "MAIL: You don't have any matching messages!");
    }
}

/*
 * Change a message's folder
 */

void do_mail_file(dbref player, char *msglist, char *folder)
{
    struct mail *mp;
    struct mail_selector ms;
    int foldernum, origfold;
    int i = 0, j = 0;

    if (!parse_msglist(msglist, &ms, player))
    {
        return;
    }

    if ((foldernum = parse_folder(player, folder)) == -1)
    {
        notify(player, "MAIL: Invalid folder specification");
        return;
    }

    origfold = player_folder(player);

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (All(ms) || (Folder(mp) == origfold))
        {
            i++;

            if (mail_match(mp, ms, i))
            {
                j++;
                mp->read &= M_FMASK; /*
                             * Clear the folder
                             */
                mp->read |= FolderBit(foldernum);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Msg %d filed in folder %d", i, foldernum);
            }
        }
    }

    if (!j)
    {
        /*
         * ran off the end of the list without finding anything
         */
        notify(player, "MAIL: You don't have any matching messages!");
    }
}

/*
 * print a mail message(s)
 */

void do_mail_read(dbref player, char *msglist)
{
    struct mail *mp;
    char *tbuf1, *buff, *status, *names, *ccnames;
    struct mail_selector ms;
    int i = 0, j = 0, folder;
    tbuf1 = XMALLOC(LBUF_SIZE, "tbuf1");

    if (!parse_msglist(msglist, &ms, player))
    {
        XFREE(tbuf1);
        return;
    }

    folder = player_folder(player);

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (Folder(mp) == folder)
        {
            i++;

            if (mail_match(mp, ms, i))
            {
                /*
                 * Read it
                 */
                j++;
                buff = XMALLOC(LBUF_SIZE, "buff");
                XSTRCPY(buff, get_mail_message(mp->number));
                notify(player, DASH_LINE);
                status = status_string(mp);
                names = make_namelist(player, (char *)mp->tolist);
                ccnames = make_namelist(player, (char *)mp->cclist);
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-3d         From:  %-*s  At: %-25s  %s\r\nFldr   : %-2d Status: %s\r\nTo     : %-65s", i, mudconf.player_name_length - 6, Name(mp->from), mp->time, (Connected(mp->from) && (!Hidden(mp->from) || See_Hidden(player))) ? " (Conn)" : "      ", folder, status, names);

                if (*ccnames)
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cc     : %-65s", ccnames);

                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Subject: %-65s", mp->subject);
                XFREE(names);
                XFREE(ccnames);
                XFREE(status);
                notify(player, DASH_LINE);
                XSTRCPY(tbuf1, buff);
                notify(player, tbuf1);
                notify(player, DASH_LINE);
                XFREE(buff);

                if (Unread(mp))
                {
                    /* mark message as read */
                    mp->read |= M_ISREAD;
                }
            }
        }
    }

    if (!j)
    {
        /*
         * ran off the end of the list without finding anything
         */
        notify(player, "MAIL: You don't have that many matching messages!");
    }

    XFREE(tbuf1);
}

void do_mail_retract(dbref player, char *name, char *msglist)
{
    dbref target;
    struct mail *mp, *nextp;
    struct mail_selector ms;
    int i = 0, j = 0;
    target = lookup_player(player, name, 1);

    if (target == NOTHING)
    {
        notify(player, "MAIL: No such player.");
        return;
    }

    if (!parse_msglist(msglist, &ms, player))
    {
        return;
    }

    for (mp = (struct mail *)nhashfind((int)target, &mod_mail_msg_htab);
         mp; mp = nextp)
    {
        if (mp->from == player)
        {
            i++;

            if (mail_match(mp, ms, i))
            {
                j++;

                if (Unread(mp))
                {
                    if (mp->prev == NULL)
                        nhashrepl((int)target,
                                  (int *)mp->next,
                                  &mod_mail_msg_htab);
                    else if (mp->next == NULL)
                        mp->prev->next = NULL;

                    if (mp->prev != NULL)
                        mp->prev->next = mp->next;

                    if (mp->next != NULL)
                        mp->next->prev = mp->prev;

                    nextp = mp->next;
                    XFREE(mp->subject);
                    delete_mail_message(mp->number);
                    XFREE(mp->time);
                    XFREE(mp->tolist);
                    XFREE(mp->cclist);
                    XFREE(mp->bcclist);
                    XFREE(mp);
                    notify(player, "MAIL: Mail retracted.");
                }
                else
                {
                    notify(player, "MAIL: That message has been read.");
                    nextp = mp->next;
                }
            }
            else
            {
                nextp = mp->next;
            }
        }
        else
        {
            nextp = mp->next;
        }
    }

    if (!j)
    {
        /*
         * ran off the end of the list without finding anything
         */
        notify(player, "MAIL: No matching messages.");
    }
}

void do_mail_review(dbref player, char *name, char *msglist)
{
    struct mail *mp;
    struct mail_selector ms;
    int i = 0, j = 0;
    dbref target, thing;
    char *status, *names, *ccnames, *bccnames;
    int all = 0;

    if (!*name || !strcmp(name, "all"))
    {
        target = player;
        all = 1;
    }
    else
    {
        target = lookup_player(player, name, 1);

        if (target == NOTHING)
        {
            notify(player, "MAIL: No such player.");
            return;
        }
    }

    if (!msglist || !*msglist)
    {
        /* no specific list of messages to review -- either see all to
         * one player, or all sent
         */
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%.*s   MAIL: %s   %.*s", (63 - strlen(Name(target))) >> 1, "--------------------------------", Name(target), (64 - strlen(Name(target))) >> 1, "--------------------------------");

        if (all)
        {
            /* ok, review all sent messages */
            MAIL_ITER_ALL(mp, thing)
            {
                if (mp->from == player)
                {
                    i++;
                    /*
                     * list it
                     */
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%s] %-3d (%4d) To: %-*s Sub: %.25s", status_chars(mp), i, strlen(get_mail_message(mp->number)), mudconf.player_name_length - 6, Name(mp->to), mp->subject);
                }
            }
        }
        else
        {
            /* review all messages we sent to the target */
            for (mp = (struct mail *)nhashfind((int)target, &mod_mail_msg_htab); mp; mp = mp->next)
            {
                if (mp->from == player)
                {
                    i++;
                    /*
                     * list it
                     */
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%s] %-3d (%4d) To: %-*s Sub: %.25s", status_chars(mp), i, strlen(get_mail_message(mp->number)), mudconf.player_name_length - 6, Name(mp->to), mp->subject);
                }
            }
        }

        notify(player, DASH_LINE);
    }
    else
    {
        /* specific list of messages, either chosen from all our sent
         * messages, or from messages sent to a particular player
         */
        if (!parse_msglist(msglist, &ms, player))
        {
            return;
        }

        if (all)
        {
            /* ok, choose messages from among all sent */
            MAIL_ITER_ALL(mp, thing)
            {
                if (mp->from == player)
                {
                    i++;

                    if (mail_match(mp, ms, i))
                    {
                        /*
                         * Read it
                         */
                        j++;
                        status = status_string(mp);
                        names = make_namelist(player, (char *)mp->tolist);
                        ccnames = make_namelist(player, (char *)mp->cclist);
                        bccnames = make_namelist(player, (char *)mp->bcclist);
                        notify(player, DASH_LINE);
                        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-3d         From:  %-*s  At: %-25s\r\nFldr   : %-2d Status: %s\r\nTo     : %-65s", i, mudconf.player_name_length - 6, Name(mp->from), mp->time, 0, status, names);

                        if (*ccnames)
                            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cc     : %-65s", ccnames);

                        if (*bccnames)
                            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Bcc    : %-65s", bccnames);

                        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Subject: %-65s", mp->subject);
                        XFREE(status);
                        XFREE(names);
                        XFREE(ccnames);
                        XFREE(bccnames);
                        notify(player, DASH_LINE);
                        notify(player, get_mail_message(mp->number));
                        notify(player, DASH_LINE);
                    }
                }
            }
        }
        else
        {
            /* choose messages from among those sent to a
             * particular player
             */
            for (mp = (struct mail *)nhashfind((int)target, &mod_mail_msg_htab); mp; mp = mp->next)
            {
                if (mp->from == player)
                {
                    i++;

                    if (mail_match(mp, ms, i))
                    {
                        /*
                         * Read it
                         */
                        j++;
                        status = status_string(mp);
                        names = make_namelist(player, (char *)mp->tolist);
                        ccnames = make_namelist(player, (char *)mp->cclist);
                        bccnames = make_namelist(player, (char *)mp->bcclist);
                        notify(player, DASH_LINE);
                        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-3d         From:  %-*s  At: %-25s\r\nFldr   : %-2d Status: %s\r\nTo     : %-65s", i, mudconf.player_name_length - 6, Name(mp->from), mp->time, 0, status, names);

                        if (*ccnames)
                            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cc     : %-65s", ccnames);

                        if (*bccnames)
                            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Bcc    : %-65s", bccnames);

                        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Subject: %-65s", mp->subject);
                        XFREE(status);
                        XFREE(names);
                        XFREE(ccnames);
                        XFREE(bccnames);
                        notify(player, DASH_LINE);
                        notify(player, get_mail_message(mp->number));
                        notify(player, DASH_LINE);
                    }
                }
            }
        }

        if (!j)
        {
            /*
             * ran off the end of the list without finding
             * anything
             */
            notify(player, "MAIL: You don't have that many matching messages!");
        }
    }
}

void do_mail_list(dbref player, char *msglist, int sub)
{
    struct mail *mp;
    struct mail_selector ms;
    int i = 0, folder;
    char *time;

    if (!parse_msglist(msglist, &ms, player))
    {
        return;
    }

    folder = player_folder(player);
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "--------------------------%s   MAIL: Folder %d   ----------------------------", ((folder > 9) ? "" : "-"), folder);

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (Folder(mp) == folder)
        {
            i++;

            if (mail_match(mp, ms, i))
            {
                /*
                 * list it
                 */
                time = mail_list_time(mp->time);

                if (sub)
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%s] %-3d (%4d) From: %-*s Sub: %.25s", status_chars(mp), i, strlen(get_mail_message(mp->number)), mudconf.player_name_length - 6, Name(mp->from), mp->subject);
                else
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "[%s] %-3d (%4d) From: %-*s At: %s %s", status_chars(mp), i, strlen(get_mail_message(mp->number)), mudconf.player_name_length - 6, Name(mp->from), time, ((Connected(mp->from) && (!Hidden(mp->from) || See_Hidden(player))) ? "Conn" : " "));

                XFREE(time);
            }
        }
    }

    notify(player, DASH_LINE);
}

static char *mail_list_time(char *the_time)
{
    char *new;
    char *p, *q;
    int i;
    p = (char *)the_time;
    q = new = XMALLOC(LBUF_SIZE, "new");

    if (!p || !*p)
    {
        *new = '\0';
        return new;
    }

    /*
     * Format of the_time is: day mon dd hh:mm:ss yyyy
     */
    /*
     * Chop out :ss
     */
    for (i = 0; i < 16; i++)
    {
        if (*p)
            *q++ = *p++;
    }

    for (i = 0; i < 3; i++)
    {
        if (*p)
            p++;
    }

    for (i = 0; i < 5; i++)
    {
        if (*p)
            *q++ = *p++;
    }

    *q = '\0';
    return new;
}

void do_mail_purge(dbref player)
{
    struct mail *mp, *nextp;

    /*
     * Go through player's mail, and remove anything marked cleared
     */
    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = nextp)
    {
        if (Cleared(mp))
        {
            /*
             * Delete this one
             */
            /*
             * head and tail of the list are special
             */
            if (mp->prev == NULL)
            {
                if (mp->next == NULL)
                    nhashdelete((int)player, &mod_mail_msg_htab);
                else
                    nhashrepl((int)player, (int *)mp->next, &mod_mail_msg_htab);
            }
            else if (mp->next == NULL)
                mp->prev->next = NULL;

            /*
             * relink the list
             */
            if (mp->prev != NULL)
                mp->prev->next = mp->next;

            if (mp->next != NULL)
                mp->next->prev = mp->prev;

            /*
             * save the pointer
             */
            nextp = mp->next;
            /*
             * then wipe
             */
            XFREE(mp->subject);
            delete_mail_message(mp->number);
            XFREE(mp->time);
            XFREE(mp->tolist);
            XFREE(mp->cclist);
            XFREE(mp->bcclist);
            XFREE(mp);
        }
        else
        {
            nextp = mp->next;
        }
    }

    notify(player, "MAIL: Mailbox purged.");
}

void do_mail_fwd(dbref player, char *msg, char *tolist)
{
    struct mail *mp;
    int num, mail_ok;
    char *s;

    if (Sending_Mail(player))
    {
        notify(player, "MAIL: Mail message already in progress.");
        return;
    }

    if (!msg || !*msg)
    {
        notify(player, "MAIL: No message list.");
        return;
    }

    if (!tolist || !*tolist)
    {
        notify(player, "MAIL: To whom should I forward?");
        return;
    }

    num = atoi(msg);

    if (!num)
    {
        notify(player, "MAIL: I don't understand that message number.");
        return;
    }

    mp = mail_fetch(player, num);

    if (!mp)
    {
        notify(player, "MAIL: You can't forward non-existent messages.");
        return;
    }

    s = XMALLOC(LBUF_SIZE, "s");
    snprintf(s, LBUF_SIZE, "%s (fwd from %s)", mp->subject, Name(mp->from));
    mail_ok = do_expmail_start(player, tolist, NULL, s);
    XFREE(s);

    if (mail_ok)
    {
        atr_add_raw(player, A_MAILMSG, get_mail_message(mp->number));
        s = XMALLOC(LBUF_SIZE, "s");
        snprintf(s, LBUF_SIZE, "%d", atoi(atr_get_raw(player, A_MAILFLAGS)) | M_FORWARD);
        atr_add_raw(player, A_MAILFLAGS, s);
        XFREE(s);
    }
}

void do_mail_reply(dbref player, char *msg, int all, int key)
{
    struct mail *mp;
    int num, mail_ok;
    char *tolist, *ccnames, *bp, *p, *oldlist, *tokst, *s;

    if (Sending_Mail(player))
    {
        notify(player, "MAIL: Mail message already in progress.");
        return;
    }

    if (!msg || !*msg)
    {
        notify(player, "MAIL: No message list.");
        return;
    }

    num = atoi(msg);

    if (!num)
    {
        notify(player, "MAIL: I don't understand that message number.");
        return;
    }

    mp = mail_fetch(player, num);

    if (!mp)
    {
        notify(player, "MAIL: You can't reply to non-existent messages.");
        return;
    }

    ccnames = NULL;

    if (all)
    {
        bp = oldlist = XMALLOC(LBUF_SIZE, "oldlist");
        SAFE_LB_STR((char *)mp->tolist, oldlist, &bp);

        if (*mp->cclist)
            SAFE_SPRINTF(oldlist, &bp, " %s", mp->cclist);

        bp = ccnames = XMALLOC(LBUF_SIZE, "ccnames");

        for (p = strtok_r(oldlist, " ", &tokst);
             p != NULL;
             p = strtok_r(NULL, " ", &tokst))
        {
            if (bp != ccnames)
            {
                SAFE_LB_CHR(' ', ccnames, &bp);
            }

            if (*p == '*')
            {
                SAFE_LB_STR(p, ccnames, &bp);
            }
            else if (atoi(p) != mp->from)
            {
                SAFE_LB_CHR('"', ccnames, &bp);
                safe_name(atoi(p), ccnames, &bp);
                SAFE_LB_CHR('"', ccnames, &bp);
            }
        }

        XFREE(oldlist);
    }

    tolist = msg;
    s = XMALLOC(LBUF_SIZE, "s");

    if (strncmp(mp->subject, "Re:", 3))
    {
        snprintf(s, LBUF_SIZE, "Re: %s", mp->subject);
        mail_ok = do_expmail_start(player, tolist, ccnames, s);
    }
    else
    {
        snprintf(s, LBUF_SIZE, "%s", mp->subject);
        mail_ok = do_expmail_start(player, tolist, ccnames, s);
    }

    if (mail_ok)
    {
        if (key & MAIL_QUOTE)
        {
            snprintf(s, LBUF_SIZE, "On %s, %s wrote:\r\n\r\n%s\r\n\r\n********** End of included message from %s\r\n", mp->time, Name(mp->from), get_mail_message(mp->number), Name(mp->from));
            atr_add_raw(player, A_MAILMSG, s);
        }

        snprintf(s, LBUF_SIZE, "%d", (atoi(atr_get_raw(player, A_MAILFLAGS)) | M_REPLY));
        atr_add_raw(player, A_MAILFLAGS, s);
    }

    XFREE(s);

    if (ccnames)
    {
        XFREE(ccnames);
    }
}

/*-------------------------------------------------------------------------*
 *   Admin mail functions
 *
 * do_mail_nuke - clear & purge mail for a player, or all mail in db.
 * do_mail_debug - fix mail with a sledgehammer
 * do_mail_stats - stats on mail for a player, or for all db.
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 *   Basic mail functions
 *-------------------------------------------------------------------------*/
struct mail *mail_fetch(dbref player, int num)
{
    struct mail *mp;
    int i = 0;

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (Folder(mp) == player_folder(player))
        {
            i++;

            if (i == num)
                return mp;
        }
    }

    return NULL;
}

/*
 * returns count of read, unread, & cleared messages as rcount, ucount,
 * * ccount
 */

void count_mail(dbref player, int folder, int *rcount, int *ucount, int *ccount)
{
    struct mail *mp;
    int rc, uc, cc;
    cc = rc = uc = 0;

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (Folder(mp) == folder)
        {
            if (Read(mp))
                rc++;
            else
                uc++;

            if (Cleared(mp))
                cc++;
        }
    }

    *rcount = rc;
    *ucount = uc;
    *ccount = cc;
}

void urgent_mail(dbref player, int folder, int *ucount)
{
    struct mail *mp;
    int uc;
    uc = 0;

    for (mp = (struct mail *)nhashfind((int)player, &mod_mail_msg_htab);
         mp; mp = mp->next)
    {
        if (Folder(mp) == folder)
        {
            if (!(Read(mp)) && (Urgent(mp)))
                uc++;
        }
    }

    *ucount = uc;
}

static void send_mail(dbref player, dbref target, char *tolist, char *cclist, char *bcclist, char *subject, int number, mail_flag flags, int silent)
{
    struct mail *newp;
    struct mail *mp;
    time_t tt;
    char tbuf1[30];

    if (Typeof(target) != TYPE_PLAYER)
    {
        notify(player, "MAIL: You cannot send mail to non-existent people.");
        delete_mail_message(number);
        return;
    }

    tt = time(NULL);
    XSTRCPY(tbuf1, ctime(&tt));
    tbuf1[strlen(tbuf1) - 1] = '\0'; /*
                         * whack the newline
                         */
    /*
     * initialize the appropriate fields
     */
    newp = (struct mail *)XMALLOC(sizeof(struct mail), "newp");
    newp->to = target;
    newp->from = player;
    newp->tolist = XSTRDUP(tolist, "newp->tolist");

    if (!cclist)
    {
        newp->cclist = XSTRDUP("", "newp->cclist");
    }
    else
    {
        newp->cclist = XSTRDUP(cclist, "newp->cclist");
    }

    if (!bcclist)
    {
        newp->bcclist = XSTRDUP("", "newp->bcclist");
    }
    else
    {
        newp->bcclist = XSTRDUP(bcclist, "newp->bcclist");
    }

    newp->number = number;
    add_count(number);
    newp->time = XSTRDUP(tbuf1, "newp->time");
    newp->subject = XSTRDUP(subject, "newp->subject");
    newp->read = flags & M_FMASK; /*
                     * Send to folder 0
                     */

    /*
     * if this is the first message, it is the head and the tail
     */
    if (!nhashfind((int)target, &mod_mail_msg_htab))
    {
        nhashadd((int)target, (int *)newp, &mod_mail_msg_htab);
        newp->next = NULL;
        newp->prev = NULL;
    }
    else
    {
        for (mp = (struct mail *)nhashfind((int)target, &mod_mail_msg_htab);
             mp->next; mp = mp->next)
            ;

        mp->next = newp;
        newp->next = NULL;
        newp->prev = mp;
    }

    /*
     * notify people
     */
    if (!silent)
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You sent your message to %s.", Name(target));

    notify_check(target, target, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You have a new message from %s.", Name(player));
    did_it(player, target, A_MAIL, NULL, A_NULL, NULL, A_AMAIL, 0,
           (char **)NULL, 0, 0);
    return;
}

void do_mail_nuke(dbref player)
{
    struct mail *mp, *nextp;
    dbref thing;

    if (!God(player))
    {
        notify(player, "The postal service issues a warrant for your arrest.");
        return;
    }

    /*
     * walk the list
     */
    DO_WHOLE_DB(thing)
    {
        for (mp = (struct mail *)nhashfind((int)thing,
                                           &mod_mail_msg_htab);
             mp != NULL; mp = nextp)
        {
            nextp = mp->next;
            delete_mail_message(mp->number);
            XFREE(mp->subject);
            XFREE(mp->tolist);
            XFREE(mp->cclist);
            XFREE(mp->bcclist);
            XFREE(mp->time);
            XFREE(mp);
        }

        nhashdelete((int)thing, &mod_mail_msg_htab);
    }
    log_write(LOG_ALWAYS, "WIZ", "MNUKE", "** MAIL PURGE ** done by ");
    notify(player, "You annihilate the post office. All messages cleared.");
}

void do_mail_debug(dbref player, char *action, char *victim)
{
    dbref target, thing;
    struct mail *mp, *nextp;

    if (!Wizard(player))
    {
        notify(player, "Go get some bugspray.");
        return;
    }

    if (string_prefix("clear", action))
    {
        target = lookup_player(player, victim, 1);

        if (target == NOTHING)
        {
            init_match(player, victim, NOTYPE);
            match_absolute();
            target = match_result();
        }

        if (target == NOTHING)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: no such player.", victim);
            return;
        }

        do_mail_clear(target, NULL);
        do_mail_purge(target);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Mail cleared for %s(#%d).", Name(target), target);
        return;
    }
    else if (string_prefix("sanity", action))
    {
        MAIL_ITER_ALL(mp, thing)
        {
            if (!Good_obj(mp->to))
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Bad object #%d has mail.", mp->to);
            else if (Typeof(mp->to) != TYPE_PLAYER)
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s(#%d) has mail but is not a player.", Name(mp->to), mp->to);
        }
        notify(player, "Mail sanity check completed.");
    }
    else if (string_prefix("fix", action))
    {
        MAIL_ITER_SAFE(mp, thing, nextp)
        {
            if (!Good_obj(mp->to) || (Typeof(mp->to) != TYPE_PLAYER))
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Fixing mail for #%d.", mp->to);

                /*
                 * Delete this one
                 */
                /*
                 * head and tail of the list are special
                 */
                if (mp->prev == NULL)
                {
                    if (mp->next == NULL)
                        nhashdelete((int)thing, &mod_mail_msg_htab);
                    else
                        nhashrepl((int)thing, (int *)mp->next, &mod_mail_msg_htab);
                }
                else if (mp->next == NULL)
                    mp->prev->next = NULL;

                /*
                 * relink the list
                 */
                if (mp->prev != NULL)
                    mp->prev->next = mp->next;

                if (mp->next != NULL)
                    mp->next->prev = mp->prev;

                /*
                 * save the pointer
                 */
                nextp = mp->next;
                /*
                 * then wipe
                 */
                XFREE(mp->subject);
                delete_mail_message(mp->number);
                XFREE(mp->time);
                XFREE(mp->tolist);
                XFREE(mp->cclist);
                XFREE(mp->bcclist);
                XFREE(mp);
            }
            else
                nextp = mp->next;
        }
        notify(player, "Mail sanity fix completed.");
    }
    else
    {
        notify(player, "That is not a debugging option.");
        return;
    }
}

void do_mail_stats(dbref player, char *name, int full)
{
    dbref target, thing;
    int fc, fr, fu, tc, tr, tu, fchars, tchars, cchars, count;
    char last[50];
    struct mail *mp;
    fc = fr = fu = tc = tr = tu = cchars = fchars = tchars = count = 0;

    /*
     * find player
     */

    if ((*name == '\0') || !name)
    {
        if Wizard (player)
            target = AMBIGUOUS;
        else
            target = player;
    }
    else if (*name == NUMBER_TOKEN)
    {
        target = atoi(&name[1]);

        if (!Good_obj(target) || (Typeof(target) != TYPE_PLAYER))
            target = NOTHING;
    }
    else if (!strcasecmp(name, "me"))
    {
        target = player;
    }
    else
    {
        target = lookup_player(player, name, 1);
    }

    if (target == NOTHING)
    {
        init_match(player, name, NOTYPE);
        match_absolute();
        target = match_result();
    }

    if (target == NOTHING)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: No such player.", name);
        return;
    }

    if (!Wizard(player) && (target != player))
    {
        notify(player, "The post office protects privacy!");
        return;
    }

    /*
     * this command is computationally expensive
     */

    if (!payfor(player, mudconf.searchcost))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Finding mail stats costs %d %s.", mudconf.searchcost, (mudconf.searchcost == 1) ? mudconf.one_coin : mudconf.many_coins);
        return;
    }

    if (target == AMBIGUOUS)
    {
        /*
                         * stats for all
                         */
        if (full == 0)
        {
            MAIL_ITER_ALL(mp, thing)
            {
                count++;
            }
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "There are %d messages in the mail spool.", count);
            return;
        }
        else if (full == 1)
        {
            MAIL_ITER_ALL(mp, thing)
            {
                if (Cleared(mp))
                    fc++;
                else if (Read(mp))
                    fr++;
                else
                    fu++;
            }
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: There are %d msgs in the mail spool, %d unread, %d cleared.", fc + fr + fu, fu, fc);
            return;
        }
        else
        {
            MAIL_ITER_ALL(mp, thing)
            {
                if (Cleared(mp))
                {
                    fc++;
                    cchars += strlen(get_mail_message(mp->number));
                }
                else if (Read(mp))
                {
                    fr++;
                    fchars += strlen(get_mail_message(mp->number));
                }
                else
                {
                    fu++;
                    tchars += strlen(get_mail_message(mp->number));
                }
            }
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: There are %d old msgs in the mail spool, totalling %d characters.", fr, fchars);
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: There are %d new msgs in the mail spool, totalling %d characters.", fu, tchars);
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: There are %d cleared msgs in the mail spool, totalling %d characters.", fc, cchars);
            return;
        }
    }

    /*
     * individual stats
     */

    if (full == 0)
    {
        /*
         * just count number of messages
         */
        MAIL_ITER_ALL(mp, thing)
        {
            if (mp->from == target)
                fr++;

            if (mp->to == target)
                tr++;
        }
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s sent %d messages.", Name(target), fr);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s has %d messages.", Name(target), tr);
        return;
    }

    /*
     * more detailed message count
     */
    MAIL_ITER_ALL(mp, thing)
    {
        if (mp->from == target)
        {
            if (Cleared(mp))
                fc++;
            else if (Read(mp))
                fr++;
            else
                fu++;

            if (full == 2)
                fchars += strlen(get_mail_message(mp->number));
        }

        if (mp->to == target)
        {
            if (!tr && !tu)
            {
                XSTRCPY(last, mp->time);
            }

            if (Cleared(mp))
                tc++;
            else if (Read(mp))
                tr++;
            else
                tu++;

            if (full == 2)
            {
                tchars += strlen(get_mail_message(mp->number));
            }
        }
    }
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Mail statistics for %s:", Name(target));

    if (full == 1)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d messages sent, %d unread, %d cleared.", fc + fr + fu, fu, fc);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d messages received, %d unread, %d cleared.", tc + tr + tu, tu, tc);
    }
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d messages sent, %d unread, %d cleared, totalling %d characters.", fc + fr + fu, fu, fc, fchars);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d messages received, %d unread, %d cleared, totalling %d characters.", tc + tr + tu, tu, tc, tchars);
    }

    if (tc + tr + tu > 0)
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Last is dated %s", last);

    return;
}

/*-------------------------------------------------------------------------*
 *   Main mail routine for @mail w/o a switch
 *-------------------------------------------------------------------------*/

void do_mail_stub(dbref player, char *arg1, char *arg2)
{
    if (Typeof(player) != TYPE_PLAYER)
    {
        notify(player, "MAIL: Only players may send and receive mail.");
        return;
    }

    if (!arg1 || !*arg1)
    {
        if (arg2 && *arg2)
        {
            notify(player, "MAIL: Invalid mail command.");
            return;
        }

        /*
         * just the "@mail" command
         */
        do_mail_list(player, arg1, 1);
        return;
    }

    /*
     * purge a player's mailbox
     */
    if (!strcasecmp(arg1, "purge"))
    {
        do_mail_purge(player);
        return;
    }

    /*
     * clear message
     */
    if (!strcasecmp(arg1, "clear"))
    {
        do_mail_clear(player, arg2);
        return;
    }

    if (!strcasecmp(arg1, "unclear"))
    {
        do_mail_unclear(player, arg2);
        return;
    }

    if (arg2 && *arg2)
    {
        /*
         * Sending mail
         */
        (void)do_expmail_start(player, arg1, NULL, arg2);
        return;
    }
    else
    {
        /*
         * Must be reading or listing mail - no arg2
         */
        if (isdigit(*arg1) && !strchr(arg1, '-'))
            do_mail_read(player, arg1);
        else
            do_mail_list(player, arg1, 1);

        return;
    }
}

void do_mail(dbref player, dbref cause __attribute__((unused)), int key, char *arg1, char *arg2)
{
    switch (key & ~MAIL_QUOTE)
    {
    case 0:
        do_mail_stub(player, arg1, arg2);
        break;

    case MAIL_STATS:
        do_mail_stats(player, arg1, 0);
        break;

    case MAIL_DSTATS:
        do_mail_stats(player, arg1, 1);
        break;

    case MAIL_FSTATS:
        do_mail_stats(player, arg1, 2);
        break;

    case MAIL_DEBUG:
        do_mail_debug(player, arg1, arg2);
        break;

    case MAIL_NUKE:
        do_mail_nuke(player);
        break;

    case MAIL_FOLDER:
        do_mail_change_folder(player, arg1, arg2);
        break;

    case MAIL_LIST:
        do_mail_list(player, arg1, 0);
        break;

    case MAIL_READ:
        do_mail_read(player, arg1);
        break;

    case MAIL_CLEAR:
        do_mail_clear(player, arg1);
        break;

    case MAIL_UNCLEAR:
        do_mail_unclear(player, arg1);
        break;

    case MAIL_PURGE:
        do_mail_purge(player);
        break;

    case MAIL_FILE:
        do_mail_file(player, arg1, arg2);
        break;

    case MAIL_TAG:
        do_mail_tag(player, arg1);
        break;

    case MAIL_UNTAG:
        do_mail_untag(player, arg1);
        break;

    case MAIL_FORWARD:
        do_mail_fwd(player, arg1, arg2);
        break;

    case MAIL_REPLY:
        do_mail_reply(player, arg1, 0, key);
        break;

    case MAIL_REPLYALL:
        do_mail_reply(player, arg1, 1, key);
        break;

    case MAIL_SEND:
        do_expmail_stop(player, 0);
        break;

    case MAIL_EDIT:
        do_edit_msg(player, arg1, arg2);
        break;

    case MAIL_URGENT:
        do_expmail_stop(player, M_URGENT);
        break;

    case MAIL_ALIAS:
        do_malias_create(player, arg1, arg2);
        break;

    case MAIL_ALIST:
        do_malias_list_all(player);
        break;

    case MAIL_PROOF:
        do_mail_proof(player);
        break;

    case MAIL_ABORT:
        do_expmail_abort(player);
        break;

    case MAIL_QUICK:
        do_mail_quick(player, arg1, arg2);
        break;

    case MAIL_REVIEW:
        do_mail_review(player, arg1, arg2);
        break;

    case MAIL_RETRACT:
        do_mail_retract(player, arg1, arg2);
        break;

    case MAIL_TO:
        do_mail_to(player, arg1, A_MAILTO);
        break;

    case MAIL_CC:
        do_mail_to(player, arg1, A_MAILCC);
        break;

    case MAIL_BCC:
        do_mail_to(player, arg1, A_MAILBCC);
        break;

    case MAIL_SAFE:
        do_mail_safe(player, arg1);
        break;
    }
}

void mod_mail_dump_database(FILE *fp)
{
    struct mail *mp, *mptr;
    dbref thing;
    int count = 0, i;
    /* Write out version number */
    fprintf(fp, "+V6\n");
    putref(fp, mod_mail_config.mail_db_top);
    DO_WHOLE_DB(thing)
    {
        if (isPlayer(thing))
        {
            mptr = (struct mail *)nhashfind((int)thing, &mod_mail_msg_htab);

            if (mptr != NULL)
                for (mp = mptr; mp; mp = mp->next)
                {
                    putref(fp, mp->to);
                    putref(fp, mp->from);
                    putref(fp, mp->number);
                    putstring(fp, mp->tolist);
                    putstring(fp, mp->cclist);
                    putstring(fp, mp->bcclist);
                    putstring(fp, mp->time);
                    putstring(fp, mp->subject);
                    putref(fp, mp->read);
                    count++;
                }
        }
    }
    fprintf(fp, "*** END OF DUMP ***\n");

    /*
     * Add the db of mail messages
     */
    for (i = 0; i < mod_mail_config.mail_db_top; i++)
    {
        if (mod_mail_config.mail_list[i].count > 0)
        {
            putref(fp, i);
            putstring(fp, get_mail_message(i));
        }
    }

    fprintf(fp, "+++ END OF DUMP +++\n");
    save_malias(fp);
    fflush(fp);
}

void mod_mail_load_database(FILE *fp)
{
    char nbuf1[SBUF_SIZE];
    char *s;
    int mail_top = 0;
    int new = 0;
    int pennsub = 0;
    int read_tolist = 0;
    int read_cclist = 0;
    int read_newdb = 0;
    int read_new_strings = 0;
    int number = 0;
    struct mail *mp, *mptr;

    /*
     * Read the version number
     */
    if (fgets(nbuf1, sizeof(nbuf1), fp) == NULL)
    {
        return;
    }

    if (!strncmp(nbuf1, "+V1", 3))
    {
        new = 0;
    }
    else if (!strncmp(nbuf1, "+V2", 3))
    {
        new = 1;
    }
    else if (!strncmp(nbuf1, "+V3", 3))
    {
        new = 1;
        read_tolist = 1;
    }
    else if (!strncmp(nbuf1, "+V4", 3))
    {
        new = 1;
        read_tolist = 1;
        read_newdb = 1;
    }
    else if (!strncmp(nbuf1, "+V5", 3))
    {
        new = 1;
        read_tolist = 1;
        read_newdb = 1;
        read_new_strings = 1;
    }
    else if (!strncmp(nbuf1, "+V6", 3))
    {
        new = 1;
        read_tolist = 1;
        read_cclist = 1;
        read_newdb = 1;
        read_new_strings = 1;
    }
    else if (!strncmp(nbuf1, "+1", 2))
    {
        pennsub = 1;
    }
    else
    {
        /* Version number mangled */
        fclose(fp);
        return;
    }

    if (pennsub)
    {
        /* Toss away the number of messages */
        if (fgets(nbuf1, sizeof(nbuf1), fp) == NULL)
        {
            return;
        }
    }

    if (read_newdb)
    {
        mail_top = getref(fp);
        mail_db_grow(mail_top + 1);
    }
    else
    {
        mail_db_grow(1);
    }

    if (fgets(nbuf1, sizeof(nbuf1), fp) == NULL)
    {
        return;
    }

    while (strncmp(nbuf1, "*** END OF DUMP ***", 19))
    {
        mp = (struct mail *)XMALLOC(sizeof(struct mail), "mp");
        mp->to = atoi(nbuf1);

        if (!nhashfind((int)mp->to, &mod_mail_msg_htab))
        {
            nhashadd((int)mp->to, (int *)mp, &mod_mail_msg_htab);
            mp->prev = NULL;
            mp->next = NULL;
        }
        else
        {
            for (mptr = (struct mail *)nhashfind((int)mp->to, &mod_mail_msg_htab);
                 mptr->next != NULL; mptr = mptr->next)
                ;

            mptr->next = mp;
            mp->prev = mptr;
            mp->next = NULL;
        }

        mp->from = getref(fp);

        if (read_newdb)
        {
            mp->number = getref(fp);
            add_count(mp->number);
        }

        if (read_tolist)
            mp->tolist = getstring(fp, read_new_strings);
        else
        {
            s = XMALLOC(LBUF_SIZE, "s");
            snprintf(s, LBUF_SIZE, "%d", mp->to);
            mp->tolist = XSTRDUP(s, "mp->tolist");
            XFREE(s);
        }

        if (read_cclist)
        {
            mp->cclist = getstring(fp, read_new_strings);
            mp->bcclist = getstring(fp, read_new_strings);
        }
        else
        {
            mp->cclist = XSTRDUP("", "mp->cclist");
            mp->bcclist = XSTRDUP("", "mp->bcclist");
        }

        mp->time = getstring(fp, read_new_strings);

        if (pennsub)
            mp->subject = getstring(fp, read_new_strings);
        else if (!new)
            mp->subject = XSTRDUP("No subject", "mp->subject");

        if (!read_newdb)
        {
            char *gs = getstring(fp, read_new_strings);
            number = add_mail_message_nosig(gs);
            XFREE(gs);
            add_count(number);
            mp->number = number;
        }

        if (new)
            mp->subject = getstring(fp, read_new_strings);
        else if (!pennsub)
            mp->subject = XSTRDUP("No subject", "mp->subject");

        mp->read = getref(fp);

        if (fgets(nbuf1, sizeof(nbuf1), fp) == NULL)
        {
            return;
        }
    }

    if (read_newdb)
    {
        if (fgets(nbuf1, sizeof(nbuf1), fp) == NULL)
        {
            return;
        }

        while (strncmp(nbuf1, "+++ END OF DUMP +++", 19))
        {
            char *gs = getstring(fp, read_new_strings);
            number = atoi(nbuf1);
            new_mail_message(gs, number);
            XFREE(gs);

            if (fgets(nbuf1, sizeof(nbuf1), fp) == NULL)
            {
                return;
            }
        }
    }

    load_malias(fp);
}

static int get_folder_number(dbref player, char *name)
{
    int aflags, alen;
    dbref aowner;
    char *atrstr;
    char *str, *pat, *res, *p, *bp;
    /* Look up a folder name and return the appropriate number */
    atrstr = atr_get(player, A_MAILFOLDERS, &aowner, &aflags, &alen);

    if (!*atrstr)
    {
        XFREE(atrstr);
        return -1;
    }

    str = XMALLOC(LBUF_SIZE, "str");
    bp = pat = XMALLOC(LBUF_SIZE, "pat");
    strcpy(str, atrstr);
    SAFE_SPRINTF(pat, &bp, ":%s:", upcasestr(name));
    res = (char *)strstr(str, pat);

    if (!res)
    {
        XFREE(str);
        XFREE(pat);
        XFREE(atrstr);
        return -1;
    }

    res += 2 + strlen(name);
    p = res;

    while (!isspace(*p))
        p++;

    p = 0;
    XFREE(atrstr);
    XFREE(str);
    XFREE(pat);
    return atoi(res);
}

static char *get_folder_name(dbref player, int fld)
{
    static char str[LBUF_SIZE];
    char *pat;
    static char *old;
    char *r, *atrstr;
    int flags, len, alen;
    /*
     * Get the name of the folder, or "nameless"
     */
    pat = XMALLOC(LBUF_SIZE, "pat");
    snprintf(pat, LBUF_SIZE, "%d:", fld);
    old = NULL;
    atrstr = atr_get(player, A_MAILFOLDERS, &player, &flags, &alen);

    if (!*atrstr)
    {
        XSTRCPY(str, "unnamed");
        XFREE(pat);
        XFREE(atrstr);
        return str;
    }

    XSTRCPY(str, atrstr);
    old = (char *)string_match(str, pat);
    XFREE(atrstr);

    if (old)
    {
        r = old + strlen(pat);

        while (*r != ':')
            r++;

        *r = '\0';
        len = strlen(pat);
        XFREE(pat);
        return old + len;
    }
    else
    {
        XSTRCPY(str, "unnamed");
        XFREE(pat);
        return str;
    }
}

void add_folder_name(dbref player, int fld, char *name)
{
    char *old, *res, *r, *atrstr;
    char *new, *pat, *str, *tbuf;
    int aflags, alen;
    /*
     * Muck with the player's MAILFOLDERS attrib to add a string of the
     * form: number:name:number to it, replacing any such string with a
     * matching number.
     */
    new = XMALLOC(LBUF_SIZE, "new");
    pat = XMALLOC(LBUF_SIZE, "pat");
    str = XMALLOC(LBUF_SIZE, "str");
    tbuf = XMALLOC(LBUF_SIZE, "tbuf");
    snprintf(new, LBUF_SIZE, "%d:%s:%d ", fld, upcasestr(name), fld);
    snprintf(pat, LBUF_SIZE, "%d:", fld);
    /* get the attrib and the old string, if any */
    old = NULL;
    atrstr = atr_get(player, A_MAILFOLDERS, &player, &aflags, &alen);

    if (*atrstr)
    {
        XSTRCPY(str, atrstr);
        old = (char *)string_match(str, pat);
    }

    if (old && *old)
    {
        XSTRCPY(tbuf, str);
        r = old;

        while (!isspace(*r))
            r++;

        *r = '\0';
        res = (char *)replace_string(old, new, tbuf);
    }
    else
    {
        r = res = XMALLOC(LBUF_SIZE, "res");

        if (*atrstr)
            SAFE_LB_STR(str, res, &r);

        SAFE_LB_STR(new, res, &r);
        *r = '\0';
    }

    /* put the attrib back */
    atr_add(player, A_MAILFOLDERS, res, player,
            AF_MDARK | AF_WIZARD | AF_NOPROG | AF_LOCK);
    XFREE(str);
    XFREE(pat);
    XFREE(new);
    XFREE(tbuf);
    XFREE(atrstr);
    XFREE(res);
}

static int player_folder(dbref player)
{
    /*
     * Return the player's current folder number. If they don't have one,
     * set it to 0
     */
    int flags, number, alen;
    char *atrstr;
    atrstr = atr_pget(player, A_MAILCURF, &player, &flags, &alen);

    if (!*atrstr)
    {
        XFREE(atrstr);
        set_player_folder(player, 0);
        return 0;
    }

    number = atoi(atrstr);
    XFREE(atrstr);
    return number;
}

void set_player_folder(dbref player, int fnum)
{
    /*
     * Set a player's folder to fnum
     */
    ATTR *a;
    char *tbuf1;
    tbuf1 = XMALLOC(LBUF_SIZE, "tbuf1");
    snprintf(tbuf1, LBUF_SIZE, "%d", fnum);
    a = (ATTR *)atr_num(A_MAILCURF);

    if (a)
        atr_add(player, A_MAILCURF, tbuf1, GOD, a->flags);
    else
    {
        /*
                     * Shouldn't happen, but...
                     */
        atr_add(player, A_MAILCURF, tbuf1, GOD, AF_WIZARD | AF_NOPROG | AF_LOCK);
    }

    XFREE(tbuf1);
}

static int parse_folder(dbref player, char *folder_string)
{
    int fnum;

    /*
     * Given a string, return a folder #, or -1 The string is just a * *
     * * number, * for now. Later, this will be where named folders are *
     * *  * handled
     */
    if (!folder_string || !*folder_string)
        return -1;

    if (isdigit(*folder_string))
    {
        fnum = atoi(folder_string);

        if ((fnum < 0) || (fnum > MAX_FOLDERS))
            return -1;
        else
            return fnum;
    }

    /*
     * Handle named folders here
     */
    return get_folder_number(player, folder_string);
}

static int mail_match(struct mail *mp, struct mail_selector ms, int num)
{
    mail_flag mpflag;

    /*
     * Does a piece of mail match the mail_selector?
     */
    if (ms.low && num < ms.low)
        return 0;

    if (ms.high && num > ms.high)
        return 0;

    if (ms.player && mp->from != ms.player)
        return 0;

    mpflag = Read(mp) ? mp->read : (mp->read | M_MSUNREAD);

    if (!(ms.flags & M_ALL) && !(ms.flags & mpflag))
        return 0;

    if (ms.days != -1)
    {
        time_t now, msgtime, diffdays;
        char *msgtimestr;
        struct tm msgtm;
        /*
         * Get the time now, subtract mp->time, and compare the * * *
         * results with * ms.days (in manner of ms.day_comp)
         */
        time(&now);
        msgtimestr = XMALLOC(LBUF_SIZE, "msgtimestr");
        XSTRCPY(msgtimestr, mp->time);

        if (do_convtime(msgtimestr, &msgtm))
        {
            msgtime = timelocal(&msgtm);
            diffdays = (now - msgtime) / 86400;
            XFREE(msgtimestr);

            if (sign(diffdays - ms.days) != ms.day_comp)
            {
                return 0;
            }
        }
        else
        {
            XFREE(msgtimestr);
            return 0;
        }
    }

    return 1;
}

static int parse_msglist(char *msglist, struct mail_selector *ms, dbref player)
{
    char *p, *q;
    char tbuf1[LBUF_SIZE];
    /*
     * Take a message list, and return the appropriate mail_selector
     * setup. For now, msglists are quite restricted. That'll change
     * once all this is working. Returns 0 if couldn't parse, and
     * also notifies player of why.
     */
    /*
     * Initialize the mail selector - this matches all messages
     */
    ms->low = 0;
    ms->high = 0;
    ms->flags = 0x0FFF | M_MSUNREAD;
    ms->player = 0;
    ms->days = -1;
    ms->day_comp = 0;

    /*
     * Now, parse the message list
     */
    if (!msglist || !*msglist)
    {
        /*
         * All messages
         */
        return 1;
    }

    /*
     * Don't mess with msglist itself
     */
    XSTRNCPY(tbuf1, msglist, LBUF_SIZE - 1);
    p = tbuf1;

    while (p && *p && isspace(*p))
        p++;

    if (!p || !*p)
    {
        return 1; /*
                 * all messages
                 */
    }

    if (isdigit(*p))
    {
        /*
         * Message or range
         */
        q = strchr(p, '-');

        if (q)
        {
            /*
             * We have a subrange, split it up and test to see if
             * it is valid
             */
            *q++ = '\0';
            ms->low = atoi(p);

            if (ms->low <= 0)
            {
                notify(player, "MAIL: Invalid message range");
                return 0;
            }

            if (!*q)
            {
                /*
                 * Unbounded range
                 */
                ms->high = 0;
            }
            else
            {
                ms->high = atoi(q);

                if ((ms->low > ms->high) || (ms->high <= 0))
                {
                    notify(player, "MAIL: Invalid message range");
                    return 0;
                }
            }
        }
        else
        {
            /*
             * A single message
             */
            ms->low = ms->high = atoi(p);
        }
    }
    else
    {
        switch (*p)
        {
        case '-': /*
                 * Range with no start
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: Invalid message range");
                return 0;
            }

            ms->high = atoi(p);

            if (ms->high <= 0)
            {
                notify(player, "MAIL: Invalid message range");
                return 0;
            }

            break;

        case '~': /*
                 * exact # of days old
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: Invalid age");
                return 0;
            }

            ms->day_comp = 0;
            ms->days = atoi(p);

            if (ms->days < 0)
            {
                notify(player, "MAIL: Invalid age");
                return 0;
            }

            break;

        case '<': /*
                 * less than # of days old
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: Invalid age");
                return 0;
            }

            ms->day_comp = -1;
            ms->days = atoi(p);

            if (ms->days < 0)
            {
                notify(player, "MAIL: Invalid age");
                return 0;
            }

            break;

        case '>': /*
                 * greater than # of days old
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: Invalid age");
                return 0;
            }

            ms->day_comp = 1;
            ms->days = atoi(p);

            if (ms->days < 0)
            {
                notify(player, "MAIL: Invalid age");
                return 0;
            }

            break;

        case '#': /*
                 * From db#
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: Invalid dbref #");
                return 0;
            }

            ms->player = atoi(p);

            if (!Good_obj(ms->player) || !(ms->player))
            {
                notify(player, "MAIL: Invalid dbref #");
                return 0;
            }

            break;

        case '*': /*
                 * From player name
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: Invalid player");
                return 0;
            }

            ms->player = lookup_player(player, p, 1);

            if (ms->player == NOTHING)
            {
                notify(player, "MAIL: Invalid player");
                return 0;
            }

            break;

        case 'u': /* Urgent, Unread */
        case 'U':
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: U is ambiguous (urgent or unread?)");
                return 0;
            }

            switch (*p)
            {
            case 'r': /*
                     * Urgent
                     */
            case 'R':
                ms->flags = M_URGENT;
                break;

            case 'n': /*
                     * Unread
                     */
            case 'N':
                ms->flags = M_MSUNREAD;
                break;

            default: /* bad */
                notify(player, "MAIL: Invalid message specification");
                return 0;
            }

            break;

        case 'r': /* read */
        case 'R':
            ms->flags = M_ISREAD;
            break;

        case 'c': /* cleared */
        case 'C':
            ms->flags = M_CLEARED;
            break;

        case 't': /* tagged */
        case 'T':
            ms->flags = M_TAG;
            break;

        case 'm':
        case 'M': /*
                 * Mass, me
                 */
            p++;

            if (!p || !*p)
            {
                notify(player, "MAIL: M is ambiguous (mass or me?)");
                return 0;
            }

            switch (*p)
            {
            case 'a':
            case 'A':
                ms->flags = M_MASS;
                break;

            case 'e':
            case 'E':
                ms->player = player;
                break;

            default:
                notify(player, "MAIL: Invalid message specification");
                return 0;
            }

            break;

        default: /* Bad news */
            notify(player, "MAIL: Invalid message specification");
            return 0;
        }
    }

    return 1;
}

void check_mail_expiration(void)
{
    struct mail *mp, *nextp;
    struct tm then_tm;
    time_t then, now = time(0);
    time_t expire_secs = mod_mail_config.mail_expiration * 86400;
    dbref thing;

    /*
     * negative values for expirations never expire
     */
    if (0 > mod_mail_config.mail_expiration)
        return;

    MAIL_ITER_SAFE(mp, thing, nextp)
    {
        if (do_convtime((char *)mp->time, &then_tm))
        {
            then = timelocal(&then_tm);

            if (((now - then) > expire_secs) && !(M_Safe(mp)))
            {
                /*
                 * Delete this one
                 */
                /*
                 * head and tail of the list are special
                 */
                if (mp->prev == NULL)
                {
                    if (mp->next == NULL)
                        nhashdelete((int)mp->to, &mod_mail_msg_htab);
                    else
                        nhashrepl((int)mp->to, (int *)mp->next, &mod_mail_msg_htab);
                }
                else if (mp->next == NULL)
                    mp->prev->next = NULL;

                /*
                 * relink the list
                 */
                if (mp->prev != NULL)
                    mp->prev->next = mp->next;

                if (mp->next != NULL)
                    mp->next->prev = mp->prev;

                /*
                 * save the pointer
                 */
                nextp = mp->next;
                /*
                 * then wipe
                 */
                XFREE(mp->subject);
                delete_mail_message(mp->number);
                XFREE(mp->tolist);
                XFREE(mp->cclist);
                XFREE(mp->bcclist);
                XFREE(mp->time);
                XFREE(mp);
            }
            else
                nextp = mp->next;
        }
        else
            nextp = mp->next;
    }
}

static char *status_chars(struct mail *mp)
{
    /*
     * Return a short description of message flags
     */
    static char res[10];
    char *p;
    XSTRCPY(res, "");
    p = res;
    *p++ = Read(mp) ? '-' : 'N';
    *p++ = M_Safe(mp) ? 'S' : '-';
    *p++ = Cleared(mp) ? 'C' : '-';
    *p++ = Urgent(mp) ? 'U' : '-';
    *p++ = Mass(mp) ? 'M' : '-';
    *p++ = Forward(mp) ? 'F' : '-';
    *p++ = Tagged(mp) ? '+' : '-';
    *p = '\0';
    return res;
}

static char *status_string(struct mail *mp)
{
    /*
     * Return a longer description of message flags
     */
    char *tbuf1;
    tbuf1 = XMALLOC(LBUF_SIZE, "tbuf1");
    XSTRCPY(tbuf1, "");

    if (Read(mp))
        strcat(tbuf1, "Read ");
    else
        strcat(tbuf1, "Unread ");

    if (Cleared(mp))
        strcat(tbuf1, "Cleared ");

    if (Urgent(mp))
        strcat(tbuf1, "Urgent ");

    if (Mass(mp))
        strcat(tbuf1, "Mass ");

    if (Forward(mp))
        strcat(tbuf1, "Fwd ");

    if (Tagged(mp))
        strcat(tbuf1, "Tagged ");

    if (M_Safe(mp))
        strcat(tbuf1, "Safe");

    return tbuf1;
}

void check_mail(dbref player, int folder, int silent)
{
    /*
     * Check for new @mail
     */
    int rc; /*

                 * read messages
                 */
    int uc; /*

                 * unread messages
                 */
    int cc; /*

                 * cleared messages
                 */
    int gc; /*

                 * urgent messages
                 */
    /*
     * just count messages
     */
    count_mail(player, folder, &rc, &uc, &cc);
    urgent_mail(player, folder, &gc);
#ifdef MAIL_ALL_FOLDERS
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: %d messages in folder %d [%s] (%d unread, %d cleared).", rc + uc, folder, get_folder_name(player, folder), uc, cc);
#else

    if (rc + uc > 0)
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: %d messages in folder %d [%s] (%d unread, %d cleared).", rc + uc, folder, get_folder_name(player, folder), uc, cc);
    else if (!silent)
        notify(player, "MAIL: You have no mail.");

    if (gc > 0)
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "URGENT MAIL: You have %d urgent messages in folder %d [%s].", gc, folder, get_folder_name(player, folder));

#endif
    return;
}

static int sign(int x)
{
    if (x == 0)
    {
        return 0;
    }
    else if (x < 0)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

void do_malias_switch(dbref player, char *a1, char *a2)
{
    if ((!a2 || !*a2) && !(!a1 || !*a1))
        do_malias_list(player, a1);
    else if ((!*a1 || !a1) && (!*a2 || !a2))
        do_malias_list_all(player);
    else
        do_malias_create(player, a1, a2);
}

void do_malias(dbref player, dbref cause __attribute__((unused)), int key, char *arg1, char *arg2)
{
    switch (key)
    {
    case 0:
        do_malias_switch(player, arg1, arg2);
        break;

    case 1:
        do_malias_desc(player, arg1, arg2);
        break;

    case 2:
        do_malias_chown(player, arg1, arg2);
        break;

    case 3:
        do_malias_add(player, arg1, arg2);
        break;

    case 4:
        do_malias_remove(player, arg1, arg2);
        break;

    case 5:
        do_malias_delete(player, arg1);
        break;

    case 6:
        do_malias_rename(player, arg1, arg2);
        break;

    case 7:
        /*
         * empty
         */
        break;

    case 8:
        do_malias_adminlist(player);
        break;

    case 9:
        do_malias_status(player);
    }
}

struct malias *get_malias(dbref player, char *alias)
{
    char *mal;
    struct malias *m;
    int i = 0;
    int x = 0;

    if ((*alias == '#') && ExpMail(player))
    {
        x = atoi(alias + 1);

        if (x < 0 || x >= ma_top)
            return NULL;
        else
            return malias[x];
    }
    else
    {
        if (*alias != '*')
            return NULL;

        mal = alias + 1;

        for (i = 0; i < ma_top; i++)
        {
            m = malias[i];

            if ((m->owner == player) || God(m->owner))
            {
                if (!strcmp(mal, m->name)) /*
                                 * Found it!
                                 */
                    return m;
            }
        }
    }

    return NULL;
}

void do_malias_create(dbref player, char *alias, char *tolist)
{
    char *head, *tail, spot;
    struct malias *m;
    struct malias **nm;
    char *na, *buff;
    int i = 0;
    dbref target;

    if (Typeof(player) != TYPE_PLAYER)
    {
        notify(player, "MAIL: Only players may create mail aliases.");
        return;
    }

    if (!alias || !*alias || !tolist || !*tolist)
    {
        notify(player, "MAIL: What alias do you want to create?.");
        return;
    }

    if (*alias != '*')
    {
        notify(player, "MAIL: All Mail aliases must begin with '*'.");
        return;
    }

    if (strlen(alias) > 31)
    {
        notify(player, "MAIL: Alias name too long, truncated.");
        alias[31] = '\0';
    }

    m = get_malias(player, alias);

    if (m)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Mail Alias '%s' already exists.", alias);
        return;
    }

    if (!ma_size)
    {
        ma_size = MA_INC;
        malias = (struct malias **)XMALLOC(sizeof(struct malias *) * ma_size, "malias");
    }
    else if (ma_top >= ma_size)
    {
        ma_size += MA_INC;
        nm = (struct malias **)XMALLOC(sizeof(struct malias *) * ma_size, "nm");

        for (i = 0; i < ma_top; i++)
            nm[i] = malias[i];

        XFREE(malias);
        malias = nm;
    }

    malias[ma_top] = (struct malias *)XMALLOC(sizeof(struct malias), "malias[ma_top]");
    i = 0;
    /* Parse the player list */
    head = (char *)tolist;

    while (head && *head && (i < (MALIAS_LEN - 1)))
    {
        while (*head == ' ')
            head++;

        tail = head;

        while (*tail && (*tail != ' '))
        {
            if (*tail == '"')
            {
                head++;
                tail++;

                while (*tail && (*tail != '"'))
                    tail++;
            }

            if (*tail)
                tail++;
        }

        tail--;

        if (*tail != '"')
            tail++;

        spot = *tail;
        *tail = '\0';

        /*
         * Now locate a target
         */
        if (!strcasecmp(head, "me"))
            target = player;
        else if (*head == '#')
        {
            target = atoi(head + 1);

            if (!Good_obj(target))
                target = NOTHING;
        }
        else
            target = lookup_player(player, head, 1);

        if ((target == NOTHING) || (Typeof(target) != TYPE_PLAYER))
        {
            notify(player, "MAIL: No such player.");
        }
        else
        {
            buff = unparse_object(player, target, 0);
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: %s added to alias %s", buff, alias);
            malias[ma_top]->list[i] = target;
            i++;
            XFREE(buff);
        }

        /*
         * Get the next recip
         */
        *tail = spot;
        head = tail;

        if (*head == '"')
            head++;
    }

    malias[ma_top]->list[i] = NOTHING;
    na = alias + 1;
    malias[ma_top]->name = (char *)XMALLOC(sizeof(char) * (strlen(na) + 1), "malias[ma_top]->name");
    malias[ma_top]->numrecep = i;
    malias[ma_top]->owner = player;
    XSTRCPY(malias[ma_top]->name, na);
    malias[ma_top]->desc = (char *)XMALLOC(sizeof(char) * (strlen(na) + 1), "malias[ma_top]->desc");
    XSTRCPY(malias[ma_top]->desc, na); /*
                         * For now do this.
                         */
    ma_top++;
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Alias set '%s' defined.", alias);
}

void do_malias_list(dbref player, char *alias)
{
    struct malias *m;
    int i = 0;
    char *buff, *bp;
    m = get_malias(player, alias);

    if (!m)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Alias '%s' not found.", alias);
        return;
    }

    if (!ExpMail(player) && (player != m->owner) && !(God(m->owner)))
    {
        notify(player, "MAIL: Permission denied.");
        return;
    }

    bp = buff = XMALLOC(LBUF_SIZE, "bp");
    SAFE_SPRINTF(buff, &bp, "MAIL: Alias *%s: ", m->name);

    for (i = m->numrecep - 1; i > -1; i--)
    {
        safe_name(m->list[i], buff, &bp);
        SAFE_LB_CHR(' ', buff, &bp);
    }

    *bp = '\0';
    notify(player, buff);
    XFREE(buff);
}

void do_malias_list_all(dbref player)
{
    struct malias *m;
    int i = 0;
    int notified = 0;

    for (i = 0; i < ma_top; i++)
    {
        m = malias[i];

        if (God(m->owner) || (m->owner == player) || God(player))
        {
            if (!notified)
            {
                notify(player, "Name         Description                         Owner");
                notified++;
            }

            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-12.12s %-35.35s %-15.15s", m->name, m->desc, Name(m->owner));
        }
    }

    notify(player, "*****  End of Mail Aliases *****");
}

void load_malias(FILE *fp)
{
    char buffer[200];

    //(void)getref(fp);
    if (fscanf(fp, "*** Begin %s ***\n", buffer) == 1 &&
        !strcmp(buffer, "MALIAS"))
    {
        malias_read(fp);
    }
    else
    {
        log_write_raw(1, "ERROR: Couldn't find Begin MALIAS.\n");
        return;
    }
}

void save_malias(FILE *fp)
{
    fprintf(fp, "*** Begin MALIAS ***\n");
    malias_write(fp);
}

void malias_read(FILE *fp)
{
    int i, j;
    char buffer[1000];
    struct malias *m;

    if (fscanf(fp, "%d\n", &ma_top) == EOF)
    {
        return;
    }

    ma_size = ma_top;

    if (ma_top > 0)
        malias = (struct malias **)XMALLOC(sizeof(struct malias *) * ma_size, "malias");
    else
        malias = NULL;

    for (i = 0; i < ma_top; i++)
    {
        malias[i] = (struct malias *)XMALLOC(sizeof(struct malias), "malias[i]");
        m = (struct malias *)malias[i];

        if (fscanf(fp, "%d %d\n", &(m->owner), &(m->numrecep)) == EOF)
        {
            return;
        }

        if (fscanf(fp, "%[^\n]\n", buffer) == EOF)
        {
            return;
        }

        m->name = (char *)XMALLOC(sizeof(char) * (strlen(buffer) - 1), "m->name");
        XSTRCPY(m->name, buffer + 2);

        if (fscanf(fp, "%[^\n]\n", buffer) == EOF)
        {
            return;
        }

        m->desc = (char *)XMALLOC(sizeof(char) * (strlen(buffer) - 1), "m->desc");
        XSTRCPY(m->desc, buffer + 2);

        if (m->numrecep > 0)
        {
            for (j = 0; j < m->numrecep; j++)
            {
                m->list[j] = getref(fp);
            }
        }
        else
        {
            m->list[0] = 0;
        }
    }
}

void malias_write(FILE *fp)
{
    int i, j;
    struct malias *m;
    fprintf(fp, "%d\n", ma_top);

    for (i = 0; i < ma_top; i++)
    {
        m = malias[i];
        fprintf(fp, "%d %d\n", m->owner, m->numrecep);
        fprintf(fp, "N:%s\n", m->name);
        fprintf(fp, "D:%s\n", m->desc);

        for (j = 0; j < m->numrecep; j++)
            fprintf(fp, "%d\n", m->list[j]);
    }
}

static int do_expmail_start(dbref player, char *arg, char *arg2, char *subject)
{
    char *tolist, *cclist, *names, *ccnames;

    if (!arg || !*arg)
    {
        notify(player, "MAIL: I do not know whom you want to mail.");
        return 0;
    }

    if (!subject || !*subject)
    {
        notify(player, "MAIL: No subject.");
        return 0;
    }

    if (Sending_Mail(player))
    {
        notify(player, "MAIL: Mail message already in progress.");
        return 0;
    }

    if (!(tolist = make_numlist(player, arg)))
    {
        return 0;
    }

    atr_add_raw(player, A_MAILTO, tolist);
    names = make_namelist(player, tolist);
    XFREE(tolist);

    if (arg2)
    {
        if (!(cclist = make_numlist(player, arg2)))
        {
            return 0;
        }

        atr_add_raw(player, A_MAILCC, cclist);
        ccnames = make_namelist(player, cclist);
        XFREE(cclist);
    }
    else
    {
        atr_clr(player, A_MAILCC);
        ccnames = NULL;
    }

    atr_clr(player, A_MAILBCC);
    atr_add_raw(player, A_MAILSUB, subject);
    atr_add_raw(player, A_MAILFLAGS, "0");
    atr_clr(player, A_MAILMSG);
    Flags2(player) |= PLAYER_MAILS;

    if (ccnames && *ccnames)
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You are sending mail to '%s', carbon-copied to '%s'.", names, ccnames);
    }
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You are sending mail to '%s'.", names);
    }

    XFREE(names);

    if (ccnames)
    {
        XFREE(ccnames);
    }

    return 1;
}

static void do_mail_to(dbref player, char *arg, int attr)
{
    char *tolist, *names, *cclist, *ccnames, *bcclist, *bccnames;
    dbref aowner;
    int aflags, alen;

    if (!Sending_Mail(player))
    {
        notify(player, "MAIL: No mail message in progress.");
        return;
    }

    if (!arg || !*arg)
    {
        if (attr == A_MAILTO)
        {
            notify(player, "MAIL: I do not know whom you want to mail.");
        }
        else
        {
            atr_clr(player, attr);
            notify_quiet(player, "Cleared.");
        }
    }
    else if ((tolist = make_numlist(player, arg)))
    {
        atr_add_raw(player, attr, tolist);
        notify_quiet(player, "Set.");
        XFREE(tolist);
    }

    tolist = atr_get(player, A_MAILTO, &aowner, &aflags, &alen);
    cclist = atr_get(player, A_MAILCC, &aowner, &aflags, &alen);
    bcclist = atr_get(player, A_MAILBCC, &aowner, &aflags, &alen);
    names = make_namelist(player, tolist);
    ccnames = make_namelist(player, cclist);
    bccnames = make_namelist(player, bcclist);

    if (*ccnames)
    {
        if (*bccnames)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You are sending mail to '%s', carbon-copied to '%s', blind carbon-copied to '%s'.", names, ccnames, bccnames);
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You are sending mail to '%s', carbon-copied to '%s'.", names, ccnames);
        }
    }
    else
    {
        if (*bcclist)
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You are sending mail to '%s', blind carbon-copied to '%s'.", names, bccnames);
        }
        else
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: You are sending mail to '%s'.", names);
        }
    }

    XFREE(names);
    XFREE(ccnames);
    XFREE(bccnames);
    XFREE(tolist);
    XFREE(cclist);
    XFREE(bcclist);
}

static char *make_namelist(dbref player __attribute__((unused)), char *arg)
{
    char *names, *oldarg;
    char *bp, *p, *tokst;
    oldarg = XMALLOC(LBUF_SIZE, "oldarg");
    names = XMALLOC(LBUF_SIZE, "names");
    bp = names;
    XSTRCPY(oldarg, arg);

    for (p = strtok_r(oldarg, " ", &tokst);
         p != NULL;
         p = strtok_r(NULL, " ", &tokst))
    {
        if (bp != names)
        {
            SAFE_LB_STR(", ", names, &bp);
        }

        if (*p == '*')
        {
            SAFE_LB_STR(p, names, &bp);
        }
        else
        {
            safe_name(atoi(p), names, &bp);
        }
    }

    XFREE(oldarg);
    return names;
}

static char *make_numlist(dbref player, char *arg)
{
    char *head, *tail, spot;
    static char *numbuf;
    char *buf;
    char *numbp;
    struct malias *m;
    struct mail *temp;
    dbref target;
    int num;
    numbuf = XMALLOC(LBUF_SIZE, "numbuf");
    numbp = numbuf;
    *numbp = '\0';
    head = (char *)arg;

    while (head && *head)
    {
        while (*head == ' ')
            head++;

        tail = head;

        while (*tail && (*tail != ' '))
        {
            if (*tail == '"')
            {
                head++;
                tail++;

                while (*tail && (*tail != '"'))
                    tail++;
            }

            if (*tail)
                tail++;
        }

        tail--;

        if (*tail != '"')
            tail++;

        spot = *tail;
        *tail = 0;

        if (strlen(head) == 0)
            continue;

        num = atoi(head);

        if (num)
        {
            temp = mail_fetch(player, num);

            if (!temp)
            {
                notify(player, "MAIL: You can't reply to nonexistent mail.");
                XFREE(numbuf);
                return NULL;
            }

            buf = XASPRINTF("buf", "%d ", temp->from);
            SAFE_LB_STR(buf, numbuf, &numbp);
            XFREE(buf);
        }
        else if (*head == '*')
        {
            m = get_malias(player, head);

            if (!m)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Alias '%s' does not exist.", head);
                XFREE(numbuf);
                return NULL;
            }

            SAFE_LB_STR(head, numbuf, &numbp);
            SAFE_LB_CHR(' ', numbuf, &numbp);
        }
        else
        {
            target = lookup_player(player, head, 1);

            if (target != NOTHING)
            {
                buf = XASPRINTF("buf", "%d ", target);
                SAFE_LB_STR(buf, numbuf, &numbp);
                XFREE(buf);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: '%s' does not exist.", head);
                XFREE(numbuf);
                return NULL;
            }
        }

        /*
         * Get the next recip
         */
        *tail = spot;
        head = tail;

        if (*head == '"')
            head++;
    }

    if (!*numbuf)
    {
        notify(player, "MAIL: No players specified.");
        XFREE(numbuf);
        return NULL;
    }
    else
    {
        *(numbp - 1) = '\0';
        return numbuf;
    }
}

void do_mail_quick(dbref player, char *arg1, char *arg2)
{
    char *buf, *bp, *tolist;

    if (!arg1 || !*arg1)
    {
        notify(player, "MAIL: I don't know who you want to mail.");
        return;
    }

    if (!arg2 || !*arg2)
    {
        notify(player, "MAIL: No message.");
        return;
    }

    if (Sending_Mail(player))
    {
        notify(player, "MAIL: Mail message already in progress.");
        return;
    }

    buf = XMALLOC(LBUF_SIZE, "buf");
    bp = buf;
    XSTRCPY(bp, arg1);
    parse_to(&bp, '/', 1);

    if (!bp)
    {
        notify(player, "MAIL: No subject.");
        XFREE(buf);
        return;
    }

    if (!(tolist = make_numlist(player, buf)))
    {
        XFREE(buf);
        return;
    }

    mail_to_list(player, tolist, NULL, NULL, bp, arg2, 0, 0);
    XFREE(buf);
    XFREE(tolist);
}

void mail_to_list(dbref player, char *tolist, char *cclist, char *bcclist, char *subject, char *message, int flags, int silent)
{
    char *head, *tail, spot, *list, *bp, *s;
    struct malias *m;
    dbref target;
    int number, i, j, n_recips, max_recips, *recip_array, *tmp;

    if (!tolist || !*tolist)
    {
        return;
    }

    bp = list = XMALLOC(LBUF_SIZE, "bp");
    SAFE_LB_STR(tolist, list, &bp);

    if (cclist && *cclist)
    {
        SAFE_LB_CHR(' ', list, &bp);
        SAFE_LB_STR(cclist, list, &bp);
    }

    if (bcclist && *bcclist)
    {
        SAFE_LB_CHR(' ', list, &bp);
        SAFE_LB_STR(bcclist, list, &bp);
    }

    number = add_mail_message(player, message);
    n_recips = 0;
    max_recips = SBUF_SIZE;
    recip_array = (int *)XCALLOC(max_recips, sizeof(int), "recip_array");
    head = (char *)list;

    while (head && *head)
    {
        while (*head == ' ')
            head++;

        tail = head;

        while (*tail && (*tail != ' '))
        {
            if (*tail == '"')
            {
                head++;
                tail++;

                while (*tail && (*tail != '"'))
                    tail++;
            }

            if (*tail)
                tail++;
        }

        tail--;

        if (*tail != '"')
            tail++;

        spot = *tail;
        *tail = '\0';

        if (*head == '*')
        {
            m = get_malias(player, head);

            if (!m)
            {
                XFREE(list);
                XFREE(recip_array);
                return;
            }

            for (i = 0; i < m->numrecep; i++)
            {
                if (isPlayer(m->list[i]) && !Going(m->list[i]))
                {
                    if (n_recips >= max_recips)
                    {
                        max_recips += SBUF_SIZE;
                        tmp = (int *)XREALLOC(recip_array, max_recips * sizeof(int), "tmp");

                        if (!tmp)
                        {
                            XFREE(list);
                            XFREE(recip_array);
                            return;
                        }

                        recip_array = tmp;
                    }

                    recip_array[n_recips] = m->list[i];
                    n_recips++;
                }
                else
                {
                    s = XMALLOC(LBUF_SIZE, "s");
                    snprintf(s, LBUF_SIZE, "Alias Error: Bad Player %d for %s", m->list[i], head);
                    send_mail(GOD, GOD, tolist, NULL, NULL, subject, add_mail_message(player, s), flags, silent);
                    XFREE(s);
                }
            }
        }
        else
        {
            target = atoi(head);

            if (Good_obj(target) && !Going(target))
            {
                if (n_recips >= max_recips)
                {
                    max_recips += SBUF_SIZE;
                    tmp = (int *)XREALLOC(recip_array, max_recips * sizeof(int), "tmp");

                    if (!tmp)
                    {
                        XFREE(list);
                        XFREE(recip_array);
                        return;
                    }

                    recip_array = tmp;
                }

                recip_array[n_recips] = target;
                n_recips++;
            }
        }

        /*
         * Get the next recip
         */
        *tail = spot;
        head = tail;

        if (*head == '"')
            head++;
    }

    /* Eliminate duplicates. */

    for (i = 0; i < n_recips; i++)
    {
        for (j = i + 1;
             (recip_array[i] != NOTHING) && (j < n_recips);
             j++)
        {
            if (recip_array[i] == recip_array[j])
                recip_array[i] = NOTHING;
        }

        if (Typeof(recip_array[i]) != TYPE_PLAYER)
            recip_array[i] = NOTHING;
    }

    /* Send it. */

    for (i = 0; i < n_recips; i++)
    {
        if (recip_array[i] != NOTHING)
        {
            send_mail(player, recip_array[i], tolist, cclist, bcclist, subject, number,
                      flags, silent);
        }
    }

    /* Clean up. */
    XFREE(list);
    XFREE(recip_array);
}

void do_expmail_stop(dbref player, int flags)
{
    char *tolist, *cclist, *bcclist, *mailsub, *mailmsg, *mailflags;
    dbref aowner;
    int aflags, alen;
    tolist = atr_get(player, A_MAILTO, &aowner, &aflags, &alen);
    cclist = atr_get(player, A_MAILCC, &aowner, &aflags, &alen);
    bcclist = atr_get(player, A_MAILBCC, &aowner, &aflags, &alen);
    mailmsg = atr_get(player, A_MAILMSG, &aowner, &aflags, &alen);
    mailsub = atr_get(player, A_MAILSUB, &aowner, &aflags, &alen);
    mailflags = atr_get(player, A_MAILFLAGS, &aowner, &aflags, &alen);

    if (!*tolist || !*mailmsg || !Sending_Mail(player))
    {
        notify(player, "MAIL: No such message to send.");
    }
    else
    {
        mail_to_list(player, tolist, cclist, bcclist, mailsub, mailmsg, flags | atoi(mailflags), 0);
    }

    XFREE(tolist);
    XFREE(cclist);
    XFREE(bcclist);
    XFREE(mailmsg);
    XFREE(mailsub);
    XFREE(mailflags);
    Flags2(player) &= ~PLAYER_MAILS;
}

static void do_expmail_abort(dbref player)
{
    Flags2(player) &= ~PLAYER_MAILS;
    notify(player, "MAIL: Message aborted.");
}

void do_prepend(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *text)
{
    char *oldmsg, *newmsg, *bp, *attr;
    dbref aowner;
    int aflags, alen;

    if (Sending_Mail(player))
    {
        oldmsg = atr_get(player, A_MAILMSG, &aowner, &aflags, &alen);

        if (*oldmsg)
        {
            bp = newmsg = XMALLOC(LBUF_SIZE, "newmsg");
            SAFE_LB_STR(text + 1, newmsg, &bp);
            SAFE_LB_CHR(' ', newmsg, &bp);
            SAFE_LB_STR(oldmsg, newmsg, &bp);
            *bp = '\0';
            atr_add_raw(player, A_MAILMSG, newmsg);
            XFREE(newmsg);
        }
        else
            atr_add_raw(player, A_MAILMSG, text + 1);

        XFREE(oldmsg);
        attr = atr_get_raw(player, A_MAILMSG);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d/%d characters prepended.", attr ? strlen(attr) : 0, LBUF_SIZE);
    }
    else
    {
        notify(player, "MAIL: No message in progress.");
    }
}

void do_postpend(dbref player, dbref cause __attribute__((unused)), int key __attribute__((unused)), char *text)
{
    char *oldmsg, *newmsg, *bp, *attr;
    dbref aowner;
    int aflags, alen;

    if ((*(text + 1) == '-') && !(*(text + 2)))
    {
        do_expmail_stop(player, 0);
        return;
    }

    if (Sending_Mail(player))
    {
        oldmsg = atr_get(player, A_MAILMSG, &aowner, &aflags, &alen);

        if (*oldmsg)
        {
            bp = newmsg = XMALLOC(LBUF_SIZE, "newmsg");
            SAFE_LB_STR(oldmsg, newmsg, &bp);
            SAFE_LB_CHR(' ', newmsg, &bp);
            SAFE_LB_STR(text + 1, newmsg, &bp);
            *bp = '\0';
            atr_add_raw(player, A_MAILMSG, newmsg);
            XFREE(newmsg);
        }
        else
            atr_add_raw(player, A_MAILMSG, text + 1);

        XFREE(oldmsg);
        attr = atr_get_raw(player, A_MAILMSG);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%d/%d characters added.", attr ? strlen(attr) : 0, LBUF_SIZE);
    }
    else
    {
        notify(player, "MAIL: No message in progress.");
    }
}

static void do_edit_msg(dbref player, char *from, char *to)
{
    char *result, *msg;
    dbref aowner;
    int aflags, alen;

    if (Sending_Mail(player))
    {
        msg = atr_get(player, A_MAILMSG, &aowner, &aflags, &alen);
        result = replace_string(from, to, msg);
        atr_add(player, A_MAILMSG, result, aowner, aflags);
        notify(player, "Text edited.");
        XFREE(result);
        XFREE(msg);
    }
    else
    {
        notify(player, "MAIL: No message in progress.");
    }
}

static void do_mail_proof(dbref player)
{
    char *mailto, *names, *ccnames, *bccnames, *message;
    dbref aowner;
    int aflags, alen;

    if (Sending_Mail(player))
    {
        mailto = atr_get(player, A_MAILTO, &aowner, &aflags, &alen);
        names = make_namelist(player, mailto);
        XFREE(mailto);
        mailto = atr_get(player, A_MAILCC, &aowner, &aflags, &alen);
        ccnames = make_namelist(player, mailto);
        XFREE(mailto);
        mailto = atr_get(player, A_MAILBCC, &aowner, &aflags, &alen);
        bccnames = make_namelist(player, mailto);
        XFREE(mailto);
        notify(player, DASH_LINE);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "From:  %-*s  Subject: %-35s\nTo: %s", mudconf.player_name_length - 6, Name(player), atr_get_raw(player, A_MAILSUB), names);

        if (*ccnames)
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cc: %s", ccnames);

        if (*bccnames)
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Bcc: %s", bccnames);

        notify(player, DASH_LINE);

        if (!(message = atr_get_raw(player, A_MAILMSG)))
        {
            notify(player, "MAIL: No text.");
        }
        else
        {
            notify(player, message);
            notify(player, DASH_LINE);
        }

        XFREE(names);
        XFREE(ccnames);
        XFREE(bccnames);
    }
    else
    {
        notify(player, "MAIL: No message in progress.");
    }
}

void do_malias_desc(dbref player, char *alias, char *desc)
{
    struct malias *m;

    if (!(m = get_malias(player, alias)))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Alias %s not found.", alias);
        return;
    }
    else if ((m->owner != GOD) || ExpMail(player))
    {
        XFREE(m->desc); /* free it up */

        if (strlen(desc) > 63)
        {
            notify(player, "MAIL: Description too long, truncated.");
            desc[63] = '\0';
        }

        m->desc = (char *)XMALLOC(sizeof(char) * (strlen(desc) + 1), "m->desc");
        XSTRCPY(m->desc, desc);
        notify(player, "MAIL: Description changed.");
    }
    else
        notify(player, "MAIL: Permission denied.");

    return;
}

void do_malias_chown(dbref player, char *alias, char *owner)
{
    struct malias *m;
    dbref no = NOTHING;

    if (!(m = get_malias(player, alias)))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Alias %s not found.", alias);
        return;
    }
    else
    {
        if (!ExpMail(player))
        {
            notify(player, "MAIL: You cannot do that!");
            return;
        }
        else
        {
            if ((no = lookup_player(player, owner, 1)) == NOTHING)
            {
                notify(player, "MAIL: I do not see that here.");
                return;
            }

            m->owner = no;
            notify(player, "MAIL: Owner changed for alias.");
        }
    }
}

void do_malias_add(dbref player, char *alias, char *person)
{
    int i = 0;
    dbref thing;
    struct malias *m;
    thing = NOTHING;

    if (!(m = get_malias(player, alias)))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Alias %s not found.", alias);
        return;
    }

    if (*person == '#')
    {
        thing = parse_dbref(person + 1);

        if (!isPlayer(thing))
        {
            notify(player, "MAIL: Only players may be added.");
            return;
        }
    }

    if (thing == NOTHING)
        thing = lookup_player(player, person, 1);

    if (thing == NOTHING)
    {
        notify(player, "MAIL: I do not see that person here.");
        return;
    }

    if (God(m->owner) && !ExpMail(player))
    {
        notify(player, "MAIL: Permission denied.");
        return;
    }

    for (i = 0; i < m->numrecep; i++)
    {
        if (m->list[i] == thing)
        {
            notify(player, "MAIL: That person is already on the list.");
            return;
        }
    }

    if (i >= (MALIAS_LEN - 1))
    {
        notify(player, "MAIL: The list is full.");
        return;
    }

    m->list[m->numrecep] = thing;
    m->numrecep = m->numrecep + 1;
    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: %s added to %s", Name(thing), m->name);
}

void do_malias_remove(dbref player, char *alias, char *person)
{
    int i, ok = 0;
    dbref thing;
    struct malias *m;
    thing = NOTHING;

    if (!(m = get_malias(player, alias)))
    {
        notify(player, "MAIL: Alias not found.");
        return;
    }

    if (God(m->owner) && !ExpMail(player))
    {
        notify(player, "MAIL: Permission denied.");
        return;
    }

    if (*person == '#')
        thing = parse_dbref(person + 1);

    if (thing == NOTHING)
        thing = lookup_player(player, person, 1);

    if (thing == NOTHING)
    {
        notify(player, "MAIL: I do not see that person here.");
        return;
    }

    for (i = 0; i < m->numrecep; i++)
    {
        if (ok)
            m->list[i] = m->list[i + 1];
        else if ((m->list[i] == thing) && !ok)
        {
            m->list[i] = m->list[i + 1];
            ok = 1;
        }
    }

    if (ok)
        m->numrecep--;

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: %s removed from alias %s.", Name(thing), alias);
}

void do_malias_rename(dbref player, char *alias, char *newname)
{
    struct malias *m;

    if ((m = get_malias(player, alias)) == NULL)
    {
        notify(player, "MAIL: I cannot find that alias!");
        return;
    }

    if (!ExpMail(player) && !(m->owner == player))
    {
        notify(player, "MAIL: Permission denied.");
        return;
    }

    if (*newname != '*')
    {
        notify(player, "MAIL: Bad alias.");
        return;
    }

    if (strlen(newname) > 31)
    {
        notify(player, "MAIL: Alias name too long, truncated.");
        newname[31] = '\0';
    }

    if (get_malias(player, newname) != NULL)
    {
        notify(player, "MAIL: That name already exists!");
        return;
    }

    XFREE(m->name);
    m->name = (char *)XMALLOC(sizeof(char) * strlen(newname), "m->name");
    XSTRCPY(m->name, newname + 1);
    notify(player, "MAIL: Mailing Alias renamed.");
}

void do_malias_delete(dbref player, char *alias)
{
    int i = 0;
    int done = 0;
    struct malias *m;
    m = get_malias(player, alias);

    if (!m)
    {
        notify(player, "MAIL: Not a valid alias. Remember to prefix the alias name with *.");
        return;
    }

    for (i = 0; i < ma_top; i++)
    {
        if (done)
            malias[i] = malias[i + 1];
        else
        {
            if ((m->owner == player) || ExpMail(player))
                if (m == malias[i])
                {
                    done = 1;
                    notify(player, "MAIL: Alias Deleted.");
                    malias[i] = malias[i + 1];
                }
        }
    }

    if (!done)
        notify(player, "MAIL: Alias not found.");
    else
        ma_top--;
}

void do_malias_adminlist(dbref player)
{
    struct malias *m;
    int i;

    if (!ExpMail(player))
    {
        do_malias_list_all(player);
        return;
    }

    notify(player, "Num  Name       Description                              Owner");

    for (i = 0; i < ma_top; i++)
    {
        m = malias[i];
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%-4d %-10.10s %-40.40s %-11.11s", i, m->name, m->desc, Name(m->owner));
    }

    notify(player, "***** End of Mail Aliases *****");
}

void do_malias_status(dbref player)
{
    if (!ExpMail(player))
        notify(player, "MAIL: Permission denied.");
    else
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Number of mail aliases defined: %d", ma_top);
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "MAIL: Allocated slots %d", ma_size);
    }
}

/* --------------------------------------------------------------------------
 * Functions.
 */

void fun_mail(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    /* This function can take one of three formats: 1.  mail(num)  -->
     * returns message <num> for privs. 2.  mail(player)  -->
     * returns number of messages for <player>. 3.
     * mail(player, num)  -->  returns message <num> for
     * <player>.
     *
     * It can now take one more format: 4.  mail() --> returns number of
     * messages for executor
     */
    struct mail *mp;
    dbref playerask;
    int num, rc, uc, cc;
    VaChk_Range(0, 2);

    if ((nfargs == 0) || !fargs[0] || !fargs[0][0])
    {
        count_mail(player, 0, &rc, &uc, &cc);
        SAFE_LTOS(buff, bufc, rc + uc, LBUF_SIZE);
        return;
    }

    if (nfargs == 1)
    {
        if (!is_number(fargs[0]))
        {
            /* handle the case of wanting to count the number of
             * messages
             */
            if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING)
            {
                SAFE_LB_STR("#-1 NO SUCH PLAYER", buff, bufc);
                return;
            }
            else if ((player != playerask) && !Wizard(player))
            {
                SAFE_NOPERM(buff, bufc);
                return;
            }
            else
            {
                count_mail(playerask, 0, &rc, &uc, &cc);
                SAFE_SPRINTF(buff, bufc, "%d %d %d", rc, uc, cc);
                return;
            }
        }
        else
        {
            playerask = player;
            num = atoi(fargs[0]);
        }
    }
    else
    {
        if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING)
        {
            SAFE_LB_STR("#-1 NO SUCH PLAYER", buff, bufc);
            return;
        }
        else if ((player != playerask) && !God(player))
        {
            SAFE_NOPERM(buff, bufc);
            return;
        }

        num = atoi(fargs[1]);
    }

    if ((num < 1) || (Typeof(playerask) != TYPE_PLAYER))
    {
        SAFE_LB_STR("#-1 NO SUCH MESSAGE", buff, bufc);
        return;
    }

    mp = mail_fetch(playerask, num);

    if (mp != NULL)
    {
        SAFE_LB_STR(get_mail_message(mp->number), buff, bufc);
        return;
    }

    /* ran off the end of the list without finding anything */
    SAFE_LB_STR("#-1 NO SUCH MESSAGE", buff, bufc);
}

void fun_mailfrom(char *buff, char **bufc, dbref player, dbref caller __attribute__((unused)), dbref cause __attribute__((unused)), char *fargs[], int nfargs, char *cargs[] __attribute__((unused)), int ncargs __attribute__((unused)))
{
    /* This function can take these formats: 1) mailfrom(<num>) 2)
     * mailfrom(<player>,<num>) It returns the dbref of the player the
     * mail is from
     */
    struct mail *mp;
    dbref playerask;
    int num;
    VaChk_Range(1, 2);

    if (nfargs == 1)
    {
        playerask = player;
        num = atoi(fargs[0]);
    }
    else
    {
        if ((playerask = lookup_player(player, fargs[0], 1)) == NOTHING)
        {
            SAFE_LB_STR("#-1 NO SUCH PLAYER", buff, bufc);
            return;
        }
        else if ((player != playerask) && !Wizard(player))
        {
            SAFE_NOPERM(buff, bufc);
            return;
        }

        num = atoi(fargs[1]);
    }

    if ((num < 1) || (Typeof(playerask) != TYPE_PLAYER))
    {
        SAFE_LB_STR("#-1 NO SUCH MESSAGE", buff, bufc);
        return;
    }

    mp = mail_fetch(playerask, num);

    if (mp != NULL)
    {
        safe_dbref(buff, bufc, mp->from);
        return;
    }

    /* ran off the end of the list without finding anything */
    SAFE_LB_STR("#-1 NO SUCH MESSAGE", buff, bufc);
}

FUN mod_mail_functable[] = {
    {"MAIL", fun_mail, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {"MAILFROM", fun_mailfrom, 0, FN_VARARGS, CA_PUBLIC, NULL},
    {NULL, NULL, 0, 0, 0, NULL}};

/* --------------------------------------------------------------------------
 * Command tables.
 */

NAMETAB mail_sw[] = {
    {(char *)"abort", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_ABORT},
    {(char *)"alias", 4, CA_NO_SLAVE | CA_NO_GUEST, MAIL_ALIAS},
    {(char *)"alist", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_ALIST},
    {(char *)"bcc", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_BCC},
    {(char *)"cc", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_CC},
    {(char *)"clear", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_CLEAR},
    {(char *)"debug", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_DEBUG},
    {(char *)"dstats", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_DSTATS},
    {(char *)"edit", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_EDIT},
    {(char *)"file", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_FILE},
    {(char *)"folder", 3, CA_NO_SLAVE | CA_NO_GUEST, MAIL_FOLDER},
    {(char *)"forward", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_FORWARD},
    {(char *)"fstats", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_FSTATS},
    {(char *)"fwd", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_FORWARD},
    {(char *)"list", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_LIST},
    {(char *)"nuke", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_NUKE},
    {(char *)"proof", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_PROOF},
    {(char *)"purge", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_PURGE},
    {(char *)"quick", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_QUICK},
    {(char *)"quote", 3, CA_NO_SLAVE | CA_NO_GUEST, MAIL_QUOTE | SW_MULTIPLE},
    {(char *)"read", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_READ},
    {(char *)"reply", 3, CA_NO_SLAVE | CA_NO_GUEST, MAIL_REPLY},
    {(char *)"replyall", 6, CA_NO_SLAVE | CA_NO_GUEST, MAIL_REPLYALL},
    {(char *)"retract", 3, CA_NO_SLAVE | CA_NO_GUEST, MAIL_RETRACT},
    {(char *)"review", 3, CA_NO_SLAVE | CA_NO_GUEST, MAIL_REVIEW},
    {(char *)"safe", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_SAFE},
    {(char *)"send", 0, CA_NO_SLAVE | CA_NO_GUEST, MAIL_SEND},
    {(char *)"stats", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_STATS},
    {(char *)"tag", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_TAG},
    {(char *)"to", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_TO},
    {(char *)"unclear", 1, CA_NO_SLAVE | CA_NO_GUEST, MAIL_UNCLEAR},
    {(char *)"untag", 3, CA_NO_SLAVE | CA_NO_GUEST, MAIL_UNTAG},
    {(char *)"urgent", 2, CA_NO_SLAVE | CA_NO_GUEST, MAIL_URGENT},
    {NULL, 0, 0, 0}};

NAMETAB malias_sw[] = {
    {(char *)"desc", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_DESC},
    {(char *)"chown", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_CHOWN},
    {(char *)"add", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_ADD},
    {(char *)"remove", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_REMOVE},
    {(char *)"delete", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_DELETE},
    {(char *)"rename", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_RENAME},
    {(char *)"list", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_LIST},
    {(char *)"status", 1, CA_NO_SLAVE | CA_NO_GUEST, MALIAS_STATUS},
    {NULL, 0, 0, 0}};

CMDENT mod_mail_cmdtable[] = {
    {(char *)"@mail",   mail_sw,    CA_NO_SLAVE | CA_NO_GUEST,  0,  CS_TWO_ARG | CS_INTERP, NULL,   NULL,   NULL,   {do_mail}},
    {(char *)"@malias", malias_sw,  CA_NO_SLAVE | CA_NO_GUEST,  0,  CS_TWO_ARG | CS_INTERP, NULL,   NULL,   NULL,   {do_malias}},
    {(char *)"-",NULL, CA_NO_GUEST | CA_NO_SLAVE | CF_DARK, 0, CS_ONE_ARG | CS_INTERP | CS_LEADIN, NULL, NULL, NULL, {do_postpend}},
    {(char *)"~", NULL, CA_NO_GUEST | CA_NO_SLAVE | CF_DARK, 0, CS_ONE_ARG | CS_INTERP | CS_LEADIN, NULL, NULL, NULL, {do_prepend}},
    {(char *)NULL, NULL, 0, 0, 0, NULL, NULL, NULL, {NULL}}};

/* --------------------------------------------------------------------------
 * Handlers.
 */

void mod_mail_announce_connect(dbref player, char *reason __attribute__((unused)), int num __attribute__((unused)))
{
    check_mail(player, 0, 0);

    if (Sending_Mail(player))
    {
        notify(player, "MAIL: You have a mail message in progress.");
    }
}

void mod_mail_announce_disconnect(dbref player, char *reason __attribute__((unused)), int num __attribute__((unused)))
{
    do_mail_purge(player);
}

void mod_mail_destroy_player(dbref player __attribute__((unused)), dbref victim)
{
    do_mail_clear(victim, NULL);
    do_mail_purge(victim);
}

void mod_mail_cleanup_startup(void)
{
    check_mail_expiration();
}

void mod_mail_init(void)
{
    mod_mail_config.mail_expiration = 14;
    mod_mail_config.mail_db_top = 0;
    mod_mail_config.mail_db_size = 0;
    mod_mail_config.mail_freelist = 0;

    switch (mudstate.version.status)
    {
    case 0:
        mod_mail_version.version = XASPRINTF("mod_comsys_version.version", "%d.%d, Alpha %d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.revision, PACKAGE_RELEASE_DATE);
        break;

    case 1:
        mod_mail_version.version = XASPRINTF("mod_comsys_version.version", "%d.%d, Beta %d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.revision, PACKAGE_RELEASE_DATE);
        break;

    case 2:
        mod_mail_version.version = XASPRINTF("mod_comsys_version.version", "%d.%d, Release Candidate %d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.revision, PACKAGE_RELEASE_DATE);
        break;

    default:
        if (mudstate.version.revision > 0)
        {
            mod_mail_version.version = XASPRINTF("mod_comsys_version.version", "%d.%d, Patch Level %d (%s)", mudstate.version.major, mudstate.version.minor, mudstate.version.revision, PACKAGE_RELEASE_DATE);
        }
        else
        {
            mod_mail_version.version = XASPRINTF("mod_comsys_version.version", "%d.%d, Gold Release (%s)", mudstate.version.major, mudstate.version.minor, PACKAGE_RELEASE_DATE);
        }
    }

    mod_mail_version.author = XSTRDUP("TinyMUSH Development Team", "mod_mail_version.author");
    mod_mail_version.email = XSTRDUP("tinymush@googlegroups.com", "mod_mail_version.email");
    mod_mail_version.url = XSTRDUP("https://github.com/TinyMUSH", "mod_mail_version.url");
    mod_mail_version.description = XSTRDUP("Mail system for TinyMUSH", "mod_mail_version.description");
    mod_mail_version.copyright = XSTRDUP("Copyright (C) TinyMUSH development team.", "mod_mail_version.copyright");
    register_commands(mod_mail_cmdtable);
    register_prefix_cmds("-~");
    register_functions(mod_mail_functable);
    register_hashtables(NULL, mod_mail_nhashtable);
}
