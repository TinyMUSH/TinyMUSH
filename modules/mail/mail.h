/**
 * @file mail.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Module for penn-based mailer system
 * @version 3.1
 * @date 2020-12-28
 *
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @note This code was taken from Kalkin's DarkZone code, which was originally
 * taken from PennMUSH 1.50 p10, and has been heavily modified since being
 * included in MUX (and then being imported wholesale into 3.0).
 *
 */

#ifndef __MAIL_H
#define __MAIL_H

/* Some of this isn't implemented yet, but heralds the future! */
#define M_ISREAD 0x0001
#define M_CLEARED 0x0002
#define M_URGENT 0x0004
#define M_MASS 0x0008
#define M_SAFE 0x0010
#define M_TAG 0x0040
#define M_FORWARD 0x0080
/* 0x0100 - 0x0F00 reserved for folder numbers */
#define M_FMASK 0xF0FF
#define M_ALL 0x1000      /* Used in mail_selectors */
#define M_MSUNREAD 0x2000 /* Mail selectors */
#define M_REPLY 0x4000
/* 0x8000 available */

#define MAX_FOLDERS 15
#define FOLDER_NAME_LEN MBUF_SIZE
#define FolderBit(f) (256 * (f))

#define Urgent(m) (m->read & M_URGENT)
#define Mass(m) (m->read & M_MASS)
#define M_Safe(m) (m->read & M_SAFE)
#define Forward(m) (m->read & M_FORWARD)
#define Tagged(m) (m->read & M_TAG)
#define Folder(m) ((m->read & ~M_FMASK) >> 8)
#define Read(m) (m->read & M_ISREAD)
#define Cleared(m) (m->read & M_CLEARED)
#define Unread(m) (!Read(m))
#define All(ms) (ms.flags & M_ALL)
#define ExpMail(x) (Wizard(x))
#define Reply(m) (m->read & M_REPLY)

#define MA_INC 2 /* what interval to increase the malias list */

#define DASH_LINE \
    "---------------------------------------------------------------------------"

#define MAIL_ITER_ALL(mp, thing)                             \
    for ((thing) = 0; (thing) < mushstate.db_top; (thing)++) \
        for (mp = (struct mail *)nhashfind((int)thing, &mod_mail_msg_htab); mp != NULL; mp = mp->next)

/* This macro requires you to put nextp = mp->next at
 * the beginning of the loop.
 */

#define MAIL_ITER_SAFE(mp, thing, nextp)                     \
    for ((thing) = 0; (thing) < mushstate.db_top; (thing)++) \
        for (mp = (struct mail *)nhashfind((int)thing, &mod_mail_msg_htab); mp != NULL; mp = nextp)

typedef unsigned int mail_flag;

struct mail
{
    struct mail *next;
    struct mail *prev;
    dbref to;
    dbref from;
    int number;
    char *time;
    char *subject;
    char *tolist;
    char *cclist;
    char *bcclist;
    int read;
};

struct mail_selector
{
    int low, high;
    mail_flag flags;
    dbref player;
    int days, day_comp;
};

typedef struct mail_entry MENT;
struct mail_entry
{
    char *message;
    int count;
};

extern void set_player_folder(dbref, int);
extern struct malias *get_malias(dbref, char *);
extern void load_malias(FILE *);
extern void save_malias(FILE *);
extern void malias_read(FILE *);
extern void malias_write(FILE *);
extern void do_malias_chown(dbref, char *, char *);
extern void do_malias_desc(dbref, char *, char *);
extern void do_mail_quick(dbref, char *, char *);
extern void do_malias_rename(dbref, char *, char *);
extern void do_malias_adminlist(dbref);
extern void do_malias_delete(dbref, char *);
extern void do_malias_status(dbref);
extern void do_malias_create(dbref, char *, char *);
extern void do_malias_list(dbref, char *);
extern void do_malias_list_all(dbref);
extern void do_malias_add(dbref, char *, char *);
extern void do_malias_remove(dbref, char *, char *);

#endif /* __MAIL_H */
