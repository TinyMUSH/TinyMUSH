/**
 * @file db.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Attribute interface, some flatfile and object routines
 * @version 3.3
 * @date 2020-12-28
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"
#include "game.h"
#include "alloc.h"
#include "flags.h"
#include "htab.h"
#include "ltdl.h"
#include "udb.h"
#include "udb_defs.h"
#include "mushconf.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "attrs.h"
#include "vattr.h"
#include "match.h"
#include "powers.h"
#include "udb.h"
#include "stringutil.h"
#include "nametabs.h"

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

/**
 * Restart definitions
 * 
 */
#define RS_CONCENTRATE 0x00000002
#define RS_RECORD_PLAYERS 0x00000004
#define RS_NEW_STRINGS 0x00000008
#define RS_COUNT_REBOOTS 0x00000010

OBJ *db = NULL;         /*!< struct database */
NAME *names = NULL;     /*!< Name buffer */
NAME *purenames = NULL; /*!< Pure Name Buffer */

extern int sock;
extern int ndescriptors;
extern int maxd;
extern int slave_socket;

extern pid_t slave_pid;

FILE *t_fd;
bool t_is_pipe = false;

/**
 * @brief Close file/stream
 * 
 * @param fd 
 */
void tf_xclose(FILE *fd)
{
    if (fd)
    {
        if (t_is_pipe)
        {
            pclose(fd);
        }
        else
        {
            fclose(fd);
        }
    }
    else
    {
        close(0);
    }

    t_fd = NULL;
    t_is_pipe = false;
}

/**
 * @brief Fiddle file/stream
 * 
 * @param tfd   Descriptor
 * @return int 
 */
int tf_fiddle(int tfd)
{
    if (tfd < 0)
    {
        tfd = open(DEV_NULL, O_RDONLY, 0);
        return -1;
    }

    if (tfd != 0)
    {
        dup2(tfd, 0);
        close(tfd);
    }

    return 0;
}

/**
 * @brief Open a file
 * 
 * @param fname Filename
 * @param mode  Mode
 * @return int 
 */
int tf_xopen(char *fname, int mode)
{
    int fd = open(fname, mode, 0600);

    fd = tf_fiddle(fd);

    return fd;
}

/**
 * @brief Convert mode to char mode
 * 
 * @param mode 
 * @return const char* 
 */
const char *mode_txt(int mode)
{
    switch (mode & O_ACCMODE)
    {
    case O_RDONLY:
        return "r";

    case O_WRONLY:
        return "w";
    }

    return "r+";
}

/**
 * @brief Initialize tf file handler
 * 
 */
void tf_init(void)
{
    fclose(stdin);
    tf_xopen(DEV_NULL, O_RDONLY);
    t_fd = NULL;
    t_is_pipe = false;
}

/**
 * @brief Open file
 * 
 * @param fname Filename
 * @param mode  Mode
 * @return int 
 */
int tf_open(char *fname, int mode)
{
    tf_xclose(t_fd);

    return tf_xopen(fname, mode);
}

/**
 * @brief Close file
 * 
 * @param fdes  File descriptor
 */
void tf_close(int fdes)
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

/**
 * @brief Open file
 * 
 * @param fname File
 * @param mode  Mode
 * @return FILE* 
 */
FILE *tf_fopen(char *fname, int mode)
{
    tf_xclose(t_fd);

    if (tf_xopen(fname, mode) >= 0)
    {
        t_fd = fdopen(0, mode_txt(mode));
        return t_fd;
    }

    return NULL;
}

/**
 * @brief Close file
 * 
 * @param fd File descriptor
 */
void tf_fclose(FILE *fd)
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

/**
 * @brief Open file
 * 
 * @param fname File
 * @param mode  File mode
 * @return FILE* 
 */
FILE *tf_popen(char *fname, int mode)
{
    tf_xclose(t_fd);
    t_fd = popen(fname, mode_txt(mode));

    if (t_fd != NULL)
    {
        t_is_pipe = true;
    }

    return t_fd;
}

/**
 * Check routine forward declaration.
 * 
 */
int fwdlist_ck(int, dbref, dbref, int, char *);
int propdir_ck(int, dbref, dbref, int, char *);

/**
 * @brief Set cached forwarding lists
 * 
 * @param thing DBref of thing
 * @param ifp   Forward list
 */
void fwdlist_set(dbref thing, FWDLIST *ifp)
{
    FWDLIST *fp = NULL, *xfp = NULL;
    int stat = 0;

    /** 
     * If fwdlist is null, clear 
     * 
     */
    if (!ifp || (ifp->count <= 0))
    {
        fwdlist_clr(thing);
        return;
    }

    /** 
     * Copy input forwardlist to a correctly-sized buffer 
     * 
     */
    fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
    fp->data = (int *)XCALLOC(ifp->count, sizeof(int), "fp->data");

    for (int i = 0; i < ifp->count; i++)
    {
        fp->data[i] = ifp->data[i];
    }

    fp->count = ifp->count;

    /**
     * Replace an existing forwardlist, or add a new one
     * 
     */
    xfp = fwdlist_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        stat = nhashrepl(thing, (int *)fp, &mudstate.fwdlist_htab);
    }
    else
    {
        stat = nhashadd(thing, (int *)fp, &mudstate.fwdlist_htab);
    }

    if (stat < 0)
    {
        /** 
         * the add or replace failed
         * 
         */
        if (fp->data)
        {
            XFREE(fp->data);
        }

        XFREE(fp);
    }
}

/**
 * @brief Clear cached forwarding lists
 * 
 * @param thing DBref of thing
 */
void fwdlist_clr(dbref thing)
{
    FWDLIST *xfp = NULL;
    /** 
     * If a forwardlist exists, delete it
     * 
     */
    xfp = fwdlist_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        nhashdelete(thing, &mudstate.fwdlist_htab);
    }
}

/**
 * @brief Load text into a forwardlist.
 * 
 * @param fp        Forward list
 * @param player    DBref of player
 * @param atext     Text
 * @return int 
 */
int fwdlist_load(FWDLIST *fp, dbref player, char *atext)
{
    dbref target = NOTHING;
    int i = 0, count = 0, errors = 0, fail = 0;
    int *tmp_array = (int *)XCALLOC((LBUF_SIZE / 2), sizeof(int), "tmp_array");
    char *tp = XMALLOC(LBUF_SIZE, "tp");
    char *dp = NULL, *bp = tp;

    XSTRCPY(tp, atext);

    do
    {
        for (; *bp && isspace(*bp); bp++)
            ; /* skip spaces */

        for (dp = bp; *bp && !isspace(*bp); bp++)
            ; /* remember string */

        if (*bp)
        {
            *bp++ = '\0'; /* terminate string */
        }

        if ((*dp++ == '#') && isdigit(*dp))
        {
            target = (int)strtol(dp, (char **)NULL, 10);

            if (!mudstate.standalone)
            {
                fail = (!Good_obj(target) || (!God(player) && !controls(player, target) && (!Link_ok(target) || !could_doit(player, target, A_LLINK))));
            }
            else
            {
                fail = !Good_obj(target);
            }

            if (fail)
            {
                if (!mudstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot forward to #%d: Permission denied.", target);
                }

                errors++;
            }
            else if (count < mudconf.fwdlist_lim)
            {
                if (fp->data)
                {
                    fp->data[count++] = target;
                }
                else
                {
                    tmp_array[count++] = target;
                }
            }
            else
            {
                if (!mudstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot forward to #%d: Forwardlist limit exceeded.", target);
                }

                errors++;
            }
        }
    } while (*bp);

    XFREE(tp);

    if ((fp->data == NULL) && (count > 0))
    {
        fp->data = (int *)XCALLOC(count, sizeof(int), "fp->data");

        for (i = 0; i < count; i++)
        {
            fp->data[i] = tmp_array[i];
        }
    }

    fp->count = count;
    XFREE(tmp_array);
    return errors;
}

/**
 * @brief Generate a text string from a FWDLIST buffer.
 * 
 * @param fp    Forward list
 * @param atext Text buffer
 * @return int 
 */
int fwdlist_rewrite(FWDLIST *fp, char *atext)
{
    char *tp = NULL, *bp = NULL;
    int count = 0;

    if (fp && fp->count)
    {
        count = fp->count;
        tp = XMALLOC(SBUF_SIZE, "tp");
        bp = atext;

        for (int i = 0; i < fp->count; i++)
        {
            if (Good_obj(fp->data[i]))
            {
                XSPRINTF(tp, "#%d ", fp->data[i]);
                SAFE_LB_STR(tp, atext, &bp);
            }
            else
            {
                count--;
            }
        }

        *bp = '\0';
        XFREE(tp);
    }
    else
    {
        count = 0;
        *atext = '\0';
    }

    return count;
}

/**
 * @brief Check a list of dbref numbers to forward to for AUDIBLE
 * 
 * @param key       Key
 * @param player    DBref of player
 * @param thing     DBref of thing
 * @param anum      Attribute
 * @param atext     Attribute text
 * @return int 
 */
int fwdlist_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
    FWDLIST *fp = NULL;
    int count = 0;

    if (mudstate.standalone)
    {
        return 1;
    }

    count = 0;

    if (atext && *atext)
    {
        fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
        fp->data = NULL;
        fwdlist_load(fp, player, atext);
    }
    else
    {
        fp = NULL;
    }

    /**
     * Set the cached forwardlist
     * 
     */
    fwdlist_set(thing, fp);
    count = fwdlist_rewrite(fp, atext);

    if (fp)
    {
        if (fp->data)
        {
            XFREE(fp->data);
        }

        XFREE(fp);
    }

    return ((count > 0) || !atext || !*atext);
}

/**
 * @brief Fetch a forward list
 * 
 * @param thing     DBref of thing
 * @return FWDLIST* 
 */
FWDLIST *fwdlist_get(dbref thing)
{
    dbref aowner = NOTHING;
    FWDLIST *fp = NULL;
    int aflags = 0, alen = 0;
    char *tp = NULL;

    if (!mudstate.standalone)
    {
        return (FWDLIST *)nhashfind((thing), &mudstate.fwdlist_htab);
    }

    if (!fp)
    {
        fp = (FWDLIST *)XMALLOC(sizeof(FWDLIST), "fp");
        fp->data = NULL;
    }

    tp = atr_get(thing, A_FORWARDLIST, &aowner, &aflags, &alen);
    fwdlist_load(fp, GOD, tp);
    XFREE(tp);
    return fp;
}

/**
 * @brief Set propdir
 * 
 * @param thing DBref of thing
 * @param ifp   Propdir.
 */
void propdir_set(dbref thing, PROPDIR *ifp)
{
    PROPDIR *fp = NULL, *xfp = NULL;
    int i = 0, stat = 0;

    /** 
     * If propdir list is null, clear
     * 
     */
    if (!ifp || (ifp->count <= 0))
    {
        propdir_clr(thing);
        return;
    }

    /** 
     * Copy input propdir to a correctly-sized buffer
     * 
     */
    fp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "fp");
    fp->data = (int *)XCALLOC(ifp->count, sizeof(int), "fp->data");

    for (i = 0; i < ifp->count; i++)
    {
        fp->data[i] = ifp->data[i];
    }

    fp->count = ifp->count;

    /**
     * Replace an existing propdir, or add a new one
     * 
     */
    xfp = propdir_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        stat = nhashrepl(thing, (int *)fp, &mudstate.propdir_htab);
    }
    else
    {
        stat = nhashadd(thing, (int *)fp, &mudstate.propdir_htab);
    }

    if (stat < 0)
    {
        /**
         * the add or replace failed
         * 
         */
        if (fp->data)
        {
            XFREE(fp->data);
        }

        XFREE(fp);
    }
}

/**
 * @brief Clear a propdir
 * 
 * @param thing DBref of thing
 */
void propdir_clr(dbref thing)
{
    PROPDIR *xfp = NULL;

    /** 
     * If a propdir exists, delete it 
     * 
     */
    xfp = propdir_get(thing);

    if (xfp)
    {
        if (xfp->data)
        {
            XFREE(xfp->data);
        }

        XFREE(xfp);
        nhashdelete(thing, &mudstate.propdir_htab);
    }
}

/**
 * @brief Load a propdir
 * 
 * @param fp        Propdir
 * @param player    DBref of player
 * @param atext     Text
 * @return int 
 */
int propdir_load(PROPDIR *fp, dbref player, char *atext)
{
    dbref target = NOTHING;
    int i = 0, count = 0, errors = 0, fail = 0;
    int *tmp_array = (int *)XCALLOC((LBUF_SIZE / 2), sizeof(int), "tmp_array");
    char *tp = XMALLOC(LBUF_SIZE, "tp");
    char *bp = tp, *dp = NULL;

    XSTRCPY(tp, atext);

    do
    {
        for (; *bp && isspace(*bp); bp++)
        {
            /**
             * skip spaces
             * 
             */
        }

        for (dp = bp; *bp && !isspace(*bp); bp++)
        {
            /**
             * remember string
             * 
             */
        }

        if (*bp)
        {
            /**
             * terminate string
             * 
             */
            *bp++ = '\0';
        }

        if ((*dp++ == '#') && isdigit(*dp))
        {
            target = (int)strtol(dp, (char **)NULL, 10);

            if (!mudstate.standalone)
            {
                fail = (!Good_obj(target) || !Parentable(player, target));
            }
            else
            {
                fail = !Good_obj(target);
            }

            if (fail)
            {
                if (!mudstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot parent to #%d: Permission denied.", target);
                }

                errors++;
            }
            else if (count < mudconf.propdir_lim)
            {
                if (fp->data)
                {
                    fp->data[count++] = target;
                }
                else
                {
                    tmp_array[count++] = target;
                }
            }
            else
            {
                if (!mudstate.standalone)
                {
                    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Cannot parent to #%d: Propdir limit exceeded.", target);
                }

                errors++;
            }
        }
    } while (*bp);

    XFREE(tp);

    if ((fp->data == NULL) && (count > 0))
    {
        fp->data = (int *)XCALLOC(count, sizeof(int), "fp->data");

        for (i = 0; i < count; i++)
        {
            fp->data[i] = tmp_array[i];
        }
    }

    fp->count = count;
    XFREE(tmp_array);
    return errors;
}

/**
 * @brief Rewrite a propdir
 * 
 * @param fp    Propdir
 * @param atext Text
 * @return int 
 */
int propdir_rewrite(PROPDIR *fp, char *atext)
{
    char *tp = NULL, *bp = NULL;
    int count = 0;

    if (fp && fp->count)
    {
        count = fp->count;
        tp = XMALLOC(SBUF_SIZE, "tp");
        bp = atext;

        for (int i = 0; i < fp->count; i++)
        {
            if (Good_obj(fp->data[i]))
            {
                XSPRINTF(tp, "#%d ", fp->data[i]);
                SAFE_LB_STR(tp, atext, &bp);
            }
            else
            {
                count--;
            }
        }

        *bp = '\0';
        XFREE(tp);
    }
    else
    {
        count = 0;
        *atext = '\0';
    }

    return count;
}

/**
 * @brief Check a popdir
 * 
 * @param key       Key
 * @param player    DBref of player
 * @param thing     DBref of thing
 * @param anum      Attribute
 * @param atext     Attribute text
 * @return int 
 */
int propdir_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
    PROPDIR *fp = NULL;
    int count = 0;

    if (mudstate.standalone)
    {
        return 1;
    }

    count = 0;

    if (atext && *atext)
    {
        fp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "fp");
        fp->data = NULL;
        propdir_load(fp, player, atext);
    }
    else
    {
        fp = NULL;
    }

    /** 
     * Set the cached propdir
     * 
     */
    propdir_set(thing, fp);
    count = propdir_rewrite(fp, atext);

    if (fp)
    {
        if (fp->data)
        {
            XFREE(fp->data);
        }

        XFREE(fp);
    }

    return ((count > 0) || !atext || !*atext);
}

/**
 * @brief Fetch a Propdir
 * 
 * @param thing     DBref of thing
 * @return PROPDIR* 
 */
PROPDIR *propdir_get(dbref thing)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *tp = NULL;
    PROPDIR *fp = NULL;

    if (!mudstate.standalone)
    {
        return (PROPDIR *)nhashfind((thing), &mudstate.propdir_htab);
    }

    if (!fp)
    {
        fp = (PROPDIR *)XMALLOC(sizeof(PROPDIR), "fp");
        fp->data = NULL;
    }

    tp = atr_get(thing, A_PROPDIR, &aowner, &aflags, &alen);
    propdir_load(fp, GOD, tp);
    XFREE(tp);
    return fp;
}

/**
 * @brief Sanitize a name
 * 
 * @param thing Thing to check name
 * @param outbuf Output buffer
 * @param bufc Tracking buffer
 */
void safe_name(dbref thing, char *outbuf, char **bufc)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    time_t save_access_time = 0L;
    char *buff = NULL, *buf = NULL;

    /** 
     * Retrieving a name never counts against an object's access time.
     * 
     */
    save_access_time = AccessTime(thing);

    if (!purenames[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        buf = strip_ansi(buff);
        purenames[thing] = XSTRDUP(buf, "purenames[thing]");
        XFREE(buf);
        XFREE(buff);
    }

    if (!names[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        names[thing] = XSTRDUP(buff, "names[thing]");
        s_NameLen(thing, alen);
        XFREE(buff);
    }

    SAFE_STRNCAT(outbuf, bufc, names[thing], NameLen(thing), LBUF_SIZE);
    s_AccessTime(thing, save_access_time);
}

/**
 * @brief Get the name of a thing
 * 
 * @param thing     DBref of the thing
 * @return char* 
 */
char *Name(dbref thing)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *buff = NULL, *buf = NULL;
    time_t save_access_time = AccessTime(thing);

    if (!purenames[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        buf = strip_ansi(buff);
        purenames[thing] = XSTRDUP(buf, "purenames[thing]");
        XFREE(buf);
        XFREE(buff);
    }

    if (!names[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        names[thing] = XSTRDUP(buff, "names[thing] ");
        s_NameLen(thing, alen);
        XFREE(buff);
    }

    s_AccessTime(thing, save_access_time);
    return names[thing];
}

/**
 * @brief Get the pure name (no ansi) of the thing
 * 
 * @param thing     DBref of the thing
 * @return char* 
 */
char *PureName(dbref thing)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *buff = NULL, *buf = NULL;
    time_t save_access_time = AccessTime(thing);

    if (!names[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        names[thing] = XSTRDUP(buff, "names[thing]");
        s_NameLen(thing, alen);
        XFREE(buff);
    }

    if (!purenames[thing])
    {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        buf = strip_ansi(buff);
        purenames[thing] = XSTRDUP(buf, "purenames[thing]");
        XFREE(buf);
        XFREE(buff);
    }

    s_AccessTime(thing, save_access_time);
    return purenames[thing];
}

/**
 * @brief Set the name of the thing
 * 
 * @param thing     DBref of thing
 * @param s         Name
 */
void s_Name(dbref thing, char *s)
{
    int len = 0;
    char *buf = NULL;

    /* Truncate the name if we have to */

    if (s)
    {
        len = strlen(s);

        if (len > MBUF_SIZE)
        {
            len = MBUF_SIZE - 1;
            s[len] = '\0';
        }
    }
    else
    {
        len = 0;
    }

    atr_add_raw(thing, A_NAME, (char *)s);
    s_NameLen(thing, len);
    names[thing] = XSTRDUP(s, "names[thing]");
    buf = strip_ansi(s);
    purenames[thing] = XSTRDUP(buf, "purenames[thing]");
    XFREE(buf);
}

/**
 * @brief Sanitize an exit name
 * 
 * @param it    DBref of exit
 * @param buff  Buffer
 * @param bufc  Tracking buffer
 */
void safe_exit_name(dbref it, char *buff, char **bufc)
{
    char *s = *bufc;
    char *buf = NULL;
    int ansi_state = ANST_NORMAL;

    safe_name(it, buff, bufc);

    while (*s && (*s != EXIT_DELIMITER))
    {
        if (*s == ESC_CHAR)
        {
            TRACK_ESCCODES(s, ansi_state);
        }
        else
        {
            ++s;
        }
    }

    *bufc = s;
    buf = ansi_transition_esccode(ansi_state, ANST_NORMAL);
    SAFE_LB_STR(buf, buff, bufc);
    XFREE(buf);
}

/**
 * @brief Add a raw attribute to a thing
 * 
 * @param thing     DBref of the thing
 * @param s         Attribute
 */
void s_Pass(dbref thing, const char *s)
{
    if (mudstate.standalone)
    {
        log_write_raw(1, "P");
    }

    atr_add_raw(thing, A_PASS, (char *)s);
}

/**
 * @brief Manage user-named attributes.
 * 
 * @param player    DBref of player
 * @param cause     DBref of cause
 * @param key       Key
 * @param aname     Attribute name
 * @param value     Value
 */
void do_attribute(dbref player, dbref cause, int key, char *aname, char *value)
{
    int success = 0, negate = 0, f = 0;
    char *buff = NULL, *sp = NULL, *p = NULL, *q = NULL, *tbuf = NULL, *tokst = NULL;
    VATTR *va = NULL;
    ATTR *va2 = NULL;

    /** 
     * Look up the user-named attribute we want to play with.
     * Note vattr names have a limited size.
     * 
     */
    buff = XMALLOC(SBUF_SIZE, "buff");

    for (p = buff, q = aname; *q && ((p - buff) < (VNAME_SIZE - 1)); p++, q++)
    {
        *p = toupper(*q);
    }

    *p = '\0';

    if (!(ok_attr_name(buff) && (va = vattr_find(buff))))
    {
        notify(player, "No such user-named attribute.");
        XFREE(buff);
        return;
    }

    switch (key)
    {
    case ATTRIB_ACCESS:

        /**
         * Modify access to user-named attribute
         * 
         */
        for (sp = value; *sp; sp++)
        {
            *sp = toupper(*sp);
        }

        sp = strtok_r(value, " ", &tokst);
        success = 0;

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
            f = search_nametab(player, attraccess_nametab, sp);

            if (f > 0)
            {
                success = 1;

                if (negate)
                {
                    va->flags &= ~f;
                }
                else
                {
                    va->flags |= f;
                }

                /** 
                 * Set the dirty bit
                 * 
                 */
                va->flags |= AF_DIRTY;
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Unknown permission: %s.", sp);
            }

            /** 
             * Get the next token
             * 
             */
            sp = strtok_r(NULL, " ", &tokst);
        }

        if (success && !Quiet(player))
        {
            notify(player, "Attribute access changed.");
        }

        break;

    case ATTRIB_RENAME:
        /** 
         * Make sure the new name doesn't already exist 
         * 
         */
        va2 = atr_str(value);

        if (va2)
        {
            notify(player, "An attribute with that name already exists.");
            XFREE(buff);
            return;
        }

        if (vattr_rename(va->name, value) == NULL)
        {
            notify(player, "Attribute rename failed.");
        }
        else
        {
            notify(player, "Attribute renamed.");
        }

        break;

    case ATTRIB_DELETE:
        /** 
         * Remove the attribute
         * 
         */
        vattr_delete(buff);
        notify(player, "Attribute deleted.");
        break;

    case ATTRIB_INFO:
        /** 
         * Print info, like @list user_attr does
         * 
         */
        if (!(va->flags & AF_DELETED))
        {
            tbuf = XMALLOC(LBUF_SIZE, "tbuf");
            XSPRINTF(tbuf, "%s(%d):", va->name, va->number);
            listset_nametab(player, attraccess_nametab, va->flags, tbuf, 1);
            XFREE(tbuf);
        }
        else
        {
            notify(player, "That attribute has been deleted.");
        }

        break;
    }

    XFREE(buff);
    return;
}

/**
 * @brief Directly edit database fields
 * 
 * @param player    DBref of plauyer
 * @param cause     DBref of cause
 * @param key       Key
 * @param arg1      Argument 1
 * @param arg2      Argument 2
 */
void do_fixdb(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
    dbref thing = NOTHING, res = NOTHING;
    char *tname = NULL, *buf = NULL;

    init_match(player, arg1, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();

    if (thing == NOTHING)
    {
        return;
    }

    res = NOTHING;

    switch (key)
    {
    case FIXDB_OWNER:
    case FIXDB_LOC:
    case FIXDB_CON:
    case FIXDB_EXITS:
    case FIXDB_NEXT:
        init_match(player, arg2, NOTYPE);
        match_everything(0);
        res = noisy_match_result();
        break;

    case FIXDB_PENNIES:
        res = (int)strtol(arg2, (char **)NULL, 10);
        break;
    }

    switch (key)
    {
    case FIXDB_OWNER:
        s_Owner(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Owner set to #%d", res);
        }

        break;

    case FIXDB_LOC:
        s_Location(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Location set to #%d", res);
        }

        break;

    case FIXDB_CON:
        s_Contents(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Contents set to #%d", res);
        }

        break;

    case FIXDB_EXITS:
        s_Exits(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Exits set to #%d", res);
        }

        break;

    case FIXDB_NEXT:
        s_Next(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Exits set to #%d", res);
        }

        break;

    case FIXDB_PENNIES:
        s_Pennies(thing, res);

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Pennies set to %d", res);
        }

        break;

    case FIXDB_NAME:
        if (Typeof(thing) == TYPE_PLAYER)
        {
            if (!ok_player_name(arg2))
            {
                notify(player, "That's not a good name for a player.");
                return;
            }

            if (lookup_player(NOTHING, arg2, 0) != NOTHING)
            {
                notify(player, "That name is already in use.");
                return;
            }

            tname = log_getname(thing);
            buf = strip_ansi(arg2);
            log_write(LOG_SECURITY, "SEC", "CNAME", "%s renamed to %s", buf);
            XFREE(buf);
            XFREE(tname);

            if (Suspect(player))
            {
                raw_broadcast(WIZARD, "[Suspect] %s renamed to %s", Name(thing), arg2);
            }

            delete_player_name(thing, Name(thing));
            s_Name(thing, arg2);
            add_player_name(thing, arg2);
        }
        else
        {
            if (!ok_name(arg2))
            {
                notify(player, "Warning: That is not a reasonable name.");
            }

            s_Name(thing, arg2);
        }

        if (!Quiet(player))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Name set to %s", arg2);
        }

        break;
    }
}

/**
 * @brief Initialize the attribute hash tables.
 * 
 */
void init_attrtab(void)
{
    ATTR *a = NULL;
    char *p = NULL, *q = NULL, *buff = XMALLOC(SBUF_SIZE, "buff");

    hashinit(&mudstate.attr_name_htab, 100 * mudconf.hash_factor, HT_STR);

    for (a = attr; a->number; a++)
    {
        anum_extend(a->number);
        anum_set(a->number, a);

        for (p = buff, q = (char *)a->name; *q; p++, q++)
        {
            *p = toupper(*q);
        }

        *p = '\0';
        hashadd(buff, (int *)a, &mudstate.attr_name_htab, 0);
    }

    XFREE(buff);
}

/**
 * @brief Look up an attribute by name.
 * 
 * @param s         Attribute name
 * @return ATTR* 
 */
ATTR *atr_str(char *s)
{
    char *buff = NULL, *p = NULL, *q = NULL;
    static ATTR *a = NULL; /** @todo Should return a buffer instead of a static pointer */
    static ATTR tattr;     /** @todo Should return a buffer instead of a static pointer */
    VATTR *va = NULL;

    /** 
     * Convert the buffer name to uppercase. Limit length of name.
     * 
     */
    buff = XMALLOC(SBUF_SIZE, "buff");

    for (p = buff, q = s; *q && ((p - buff) < (VNAME_SIZE - 1)); p++, q++)
    {
        *p = toupper(*q);
    }

    *p = '\0';

    if (!ok_attr_name(buff))
    {
        XFREE(buff);
        return NULL;
    }

    /** 
     * Look for a predefined attribute 
     * 
     */
    if (!mudstate.standalone)
    {
        a = (ATTR *)hashfind_generic((HASHKEY)(buff), (&mudstate.attr_name_htab));

        if (a != NULL)
        {
            XFREE(buff);
            return a;
        }
    }
    else
    {
        for (a = attr; a->name; a++)
        {
            if (!string_compare(a->name, s))
            {
                XFREE(buff);
                return a;
            }
        }
    }

    /** 
     * Nope, look for a user attribute
     * 
     */
    va = vattr_find(buff);
    XFREE(buff);

    /** 
     * If we got one, load tattr and return a pointer to it.
     * 
     */
    if (va != NULL)
    {
        tattr.name = va->name;
        tattr.number = va->number;
        tattr.flags = va->flags;
        tattr.check = NULL;
        return &tattr;
    }

    if (mudstate.standalone)
    {
        /**
    	 * No exact match, try for a prefix match on predefined attribs.
         * Check for both longer versions and shorter versions.
    	 */
        for (a = attr; a->name; a++)
        {
            if (string_prefix(s, a->name))
            {
                return a;
            }

            if (string_prefix(a->name, s))
            {
                return a;
            }
        }
    }

    /** 
     * All failed, return NULL
     * 
     */
    return NULL;
}

ATTR **anum_table = NULL;
int anum_alc_top = 0;

/**
 * @brief Grow the attr num lookup table.
 * 
 * @param newtop New size
 */
void anum_extend(int newtop)
{
    ATTR **anum_table2 = NULL;
    int delta = 0;

    if (!mudstate.standalone)
    {
        delta = mudconf.init_size;
    }
    else
    {
        delta = 1000;
    }

    if (newtop <= anum_alc_top)
    {
        return;
    }

    if (newtop < anum_alc_top + delta)
    {
        newtop = anum_alc_top + delta;
    }

    if (anum_table == NULL)
    {
        anum_table = (ATTR **)XCALLOC(newtop + 1, sizeof(ATTR *), "anum_table");
    }
    else
    {
        anum_table2 = (ATTR **)XCALLOC(newtop + 1, sizeof(ATTR *), "anum_table2");

        for (int i = 0; i <= anum_alc_top; i++)
        {
            anum_table2[i] = anum_table[i];
        }

        XFREE(anum_table);
        anum_table = anum_table2;
    }

    anum_alc_top = newtop;
}

/**
 * @brief Look up an attribute by number.
 * 
 * @param anum      Attribute number
 * @return ATTR* 
 */
ATTR *atr_num(int anum)
{
    VATTR *va = NULL;
    static ATTR tattr; /** @todo Should return a buffer instead of a static pointer */

    /** 
     * Look for a predefined attribute 
     * 
     */
    if (anum < A_USER_START)
    {
        return anum_get(anum);
    }

    if (anum > anum_alc_top)
    {
        return NULL;
    }

    /** 
     * It's a user-defined attribute, we need to copy data
     * 
     */
    va = (VATTR *)anum_get(anum);

    if (va != NULL)
    {
        tattr.name = va->name;
        tattr.number = va->number;
        tattr.flags = va->flags;
        tattr.check = NULL;
        return &tattr;
    }

    /** 
     * All failed, return NULL 
     * 
     */
    return NULL;
}

/**
 * @brief Lookup attribute by name, creating if needed.
 * 
 * @param buff  Attribute name
 * @return int 
 */
int mkattr(char *buff)
{
    ATTR *ap = NULL;
    VATTR *va = NULL;
    int vflags = 0;
    KEYLIST *kp = NULL;

    if (!(ap = atr_str(buff)))
    {
        /** 
         * Unknown attr, create a new one. Check if it matches any
         * attribute type pattern that we have defined; if it does, give it
         * those flags. Otherwise, use the default vattr flags.
         * 
    	 */
        if (!mudstate.standalone)
        {
            vflags = mudconf.vattr_flags;

            for (kp = mudconf.vattr_flag_list; kp != NULL; kp = kp->next)
            {
                if (quick_wild(kp->name, buff))
                {
                    vflags = kp->data;
                    break;
                }
            }

            va = vattr_alloc(buff, vflags);
        }
        else
        {
            va = vattr_alloc(buff, mudconf.vattr_flags);
        }

        if (!va || !(va->number))
        {
            return -1;
        }

        return va->number;
    }

    if (!(ap->number))
    {
        return -1;
    }

    return ap->number;
}

/**
 * @brief Fetch an attribute number from an alist.
 * 
 * @param app Attribute list
 * @return int 
 */
int al_decode(char **app)
{
    int atrnum = 0, anum, atrshft = 0;
    char *ap;
    ap = *app;

    while (true)
    {
        anum = ((*ap) & 0x7f);

        if (atrshft > 0)
        {
            atrnum += (anum << atrshft);
        }
        else
        {
            atrnum = anum;
        }

        if (!(*ap++ & 0x80))
        {
            *app = ap;
            return atrnum;
        }

        atrshft += 7;
    }
}

/**
 * @brief Store an attribute number in an alist.
 * 
 * @param app       Attribute list
 * @param atrnum    Attribute number
 */
void al_code(char **app, int atrnum)
{
    char *ap;
    ap = *app;

    while (true)
    {
        *ap = atrnum & 0x7f;
        atrnum = atrnum >> 7;

        if (!atrnum)
        {
            *app = ++ap;
            return;
        }

        *ap++ |= 0x80;
    }
}

/**
 * @brief check if an object has any $-commands in its attributes.
 * 
 * @param thing DBref of thing
 * @return int 
 */
bool Commer(dbref thing)
{
    char *s = NULL, *as = NULL;
    int attr = 0, aflags = 0, alen = 0;
    dbref aowner = NOTHING;
    ATTR *ap = NULL;

    if ((!Has_Commands(thing) && mudconf.req_cmds_flag) || Halted(thing))
    {
        return false;
    }

    s = XMALLOC(LBUF_SIZE, "s");
    atr_push();

    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as))
    {
        ap = atr_num(attr);

        if (!ap || (ap->flags & AF_NOPROG))
        {
            continue;
        }

        s = atr_get_str(s, thing, attr, &aowner, &aflags, &alen);

        if ((*s == '$') && !(aflags & AF_NOPROG))
        {
            atr_pop();
            XFREE(s);
            return true;
        }
    }

    atr_pop();
    XFREE(s);
    return false;
}

/**
 * @brief Get more space for attributes, if needed
 * 
 * @param buffer    Attribute buffer
 * @param bufsiz    Buffer size
 * @param len       Length
 * @param copy      Copy buffer content to the extended buffer?
 */
void al_extend(char **buffer, int *bufsiz, int len, bool copy)
{
    char *tbuff = NULL;
    int newsize = 0;

    if (len > *bufsiz)
    {
        newsize = len + ATR_BUF_CHUNK;
        tbuff = (char *)XMALLOC(newsize, "tbuff");

        if (*buffer)
        {
            if (copy)
            {
                XMEMCPY(tbuff, *buffer, *bufsiz);
            }

            XFREE(*buffer);
        }

        *buffer = tbuff;
        *bufsiz = newsize;
    }
}

/**
 * @brief Return length of attribute list in chars
 * 
 * @param astr Attribute list
 * @return int 
 */
int al_size(char *astr)
{
    if (!astr)
    {
        return 0;
    }

    return (strlen(astr) + 1);
}

/**
 * @brief Write modified attribute list
 * 
 */
void al_store(void)
{
    if (mudstate.mod_al_id != NOTHING)
    {
        if (mudstate.mod_alist && *mudstate.mod_alist)
        {
            atr_add_raw(mudstate.mod_al_id, A_LIST, mudstate.mod_alist);
        }
        else
        {
            atr_clr(mudstate.mod_al_id, A_LIST);
        }
    }

    mudstate.mod_al_id = NOTHING;
}

/**
 * @brief Load attribute list
 * 
 * @param thing     DBref of thing
 * @return char* 
 */
char *al_fetch(dbref thing)
{
    char *astr = NULL;
    int len = 0;

    /** 
     * We only need fetch if we change things
     * 
     */
    if (mudstate.mod_al_id == thing)
    {
        return mudstate.mod_alist;
    }

    /** 
     * Fetch and set up the attribute list
     * 
     */
    al_store();
    astr = atr_get_raw(thing, A_LIST);

    if (astr)
    {
        len = al_size(astr);
        al_extend(&mudstate.mod_alist, &mudstate.mod_size, len, 0);
        XMEMCPY(mudstate.mod_alist, astr, len);
    }
    else
    {
        al_extend(&mudstate.mod_alist, &mudstate.mod_size, 1, 0);
        *mudstate.mod_alist = '\0';
    }

    mudstate.mod_al_id = thing;
    return mudstate.mod_alist;
}

/**
 * @brief Add an attribute to an attribute list
 * 
 * @param thing     DBref of thing
 * @param attrnum   Attribute number
 */
void al_add(dbref thing, int attrnum)
{
    char *abuf = NULL, *cp = NULL;
    int anum = 0;

    /** 
     * If trying to modify List attrib, return.  Otherwise, get the
     * attribute list.
     * 
     */
    if (attrnum == A_LIST)
    {
        return;
    }

    abuf = al_fetch(thing);

    /** 
     * See if attr is in the list.  If so, exit (need not do anything) 
     * 
     */
    cp = abuf;

    while (*cp)
    {
        anum = al_decode(&cp);

        if (anum == attrnum)
        {
            return;
        }
    }

    /** 
     * Nope, extend it 
     * 
     */
    al_extend(&mudstate.mod_alist, &mudstate.mod_size, (cp - abuf + ATR_BUF_INCR), 1);

    if (mudstate.mod_alist != abuf)
    {
        /** 
         * extend returned different buffer, re-find the end
         * 
         */
        abuf = mudstate.mod_alist;

        for (cp = abuf; *cp; anum = al_decode(&cp))
            ;
    }

    /** 
     * Add the new attribute on to the end 
     * 
     */
    al_code(&cp, attrnum);
    *cp = '\0';
    return;
}

/**
 * @brief Remove an attribute from an attribute list
 * 
 * @param thing     DBref of thing
 * @param attrnum   Attribute number
 */
void al_delete(dbref thing, int attrnum)
{
    int anum = 0;
    char *abuf = NULL, *cp = NULL, *dp = NULL;

    /** 
     * If trying to modify List attrib, return.  Otherwise, get the attribute list.
     * 
     */

    if (attrnum == A_LIST)
    {
        return;
    }

    abuf = al_fetch(thing);

    if (!abuf)
    {
        return;
    }

    cp = abuf;

    while (*cp)
    {
        dp = cp;
        anum = al_decode(&cp);

        if (anum == attrnum)
        {
            while (*cp)
            {
                anum = al_decode(&cp);
                al_code(&dp, anum);
            }

            *dp = '\0';
            return;
        }
    }

    return;
}

/**
 * @brief Make a key
 * 
 * @param thing DBref of thing
 * @param atr   Attribute number
 * @param abuff Attribute buffer
 */
void makekey(dbref thing, int atr, Aname *abuff)
{
    abuff->object = thing;
    abuff->attrnum = atr;
    return;
}

/**
 * @brief wipe out an object's attribute list.
 * 
 * @param thing DBref of thing
 */
void al_destroy(dbref thing)
{
    if (mudstate.mod_al_id == thing)
    {
        /**
         * remove from cache
         * 
         */
        al_store();
    }

    atr_clr(thing, A_LIST);
}

/**
 * @brief Encode an attribute string.
 * 
 * @param iattr Attribute string
 * @param thing DBref of thing
 * @param owner DBref of owner
 * @param flags Flags
 * @param atr   Attribute numbrer
 * @return char* 
 */
char *atr_encode(char *iattr, dbref thing, dbref owner, int flags, int atr)
{
    char *attr = XMALLOC(MBUF_SIZE, "attr");

    /** 
     * If using the default owner and flags (almost all attributes will),
     * just store the string.
     * 
     */
    if (((owner == Owner(thing)) || (owner == NOTHING)) && !flags)
    {
        return (iattr);
    }

    /** 
     * Encode owner and flags into the attribute text 
     * 
     */
    if (owner == NOTHING)
    {
        owner = Owner(thing);
    }

    XSNPRINTF(attr, MBUF_SIZE, "%c%d:%d:%s", ATR_INFO_CHAR, owner, flags, iattr);
    return (attr);
}

/**
 * @brief Decode an attribute string.
 * 
 * @param iattr Input attribute string
 * @param oattr Output attribute string
 * @param thing DBref of thing
 * @param owner DBref of owner
 * @param flags Flags
 * @param atr   Attribute number
 * @param alen  Attribute len
 */
void atr_decode(char *iattr, char *oattr, dbref thing, dbref *owner, int *flags, int atr, int *alen)
{
    char *cp = NULL;
    int neg = 0;

    /** 
     * See if the first char of the attribute is the special character
     * 
     */
    if (*iattr == ATR_INFO_CHAR)
    {
        /** 
         * It is, crack the attr apart
         * 
         */
        cp = &iattr[1];
        /** 
         * Get the attribute owner
         * 
         */
        *owner = 0;
        neg = 0;

        if (*cp == '-')
        {
            neg = 1;
            cp++;
        }

        while (isdigit(*cp))
        {
            *owner = (*owner * 10) + (*cp++ - '0');
        }

        if (neg)
        {
            *owner = 0 - *owner;
        }

        /** 
         * If delimiter is not ':', just return attribute
         * 
         */
        if (*cp++ != ':')
        {
            *owner = Owner(thing);
            *flags = 0;

            if (oattr)
            {
                StrCopyLen(oattr, iattr, alen);
            }

            return;
        }

        /** 
         * Get the attribute flags 
         * 
         */
        *flags = 0;

        while (isdigit(*cp))
        {
            *flags = (*flags * 10) + (*cp++ - '0');
        }

        /** 
         * If delimiter is not ':', just return attribute
         * 
         */
        if (*cp++ != ':')
        {
            *owner = Owner(thing);
            *flags = 0;

            if (oattr)
            {
                StrCopyLen(oattr, iattr, alen);
            }
        }

        /** 
         * Get the attribute text 
         * 
         */
        if (oattr)
        {
            StrCopyLen(oattr, cp, alen);
        }

        if (*owner == NOTHING)
        {
            *owner = Owner(thing);
        }
    }
    else
    {
        /** 
         * Not the special character, return normal info 
         * 
         */
        *owner = Owner(thing);
        *flags = 0;

        if (oattr)
        {
            StrCopyLen(oattr, iattr, alen);
        }
    }
}

/**
 * @brief clear an attribute in the list.
 * 
 * @param thing DBref of thing
 * @param atr Attribute number
 */
void atr_clr(dbref thing, int atr)
{
    Aname okey;
    DBData key;

    /** 
     * Delete the entry from cache 
     * 
     */
    makekey(thing, atr, &okey);
    key.dptr = &okey;
    key.dsize = sizeof(Aname);
    cache_del(key, DBTYPE_ATTRIBUTE);
    al_delete(thing, atr);

    if (!mudstate.standalone && !mudstate.loading_db)
    {
        s_Modified(thing);
    }

    switch (atr)
    {
    case A_STARTUP:
        s_Flags(thing, Flags(thing) & ~HAS_STARTUP);
        break;

    case A_DAILY:
        s_Flags2(thing, Flags2(thing) & ~HAS_DAILY);

        if (!mudstate.standalone)
        {
            (void)cron_clr(thing, A_DAILY);
        }

        break;

    case A_FORWARDLIST:
        s_Flags2(thing, Flags2(thing) & ~HAS_FWDLIST);
        break;

    case A_LISTEN:
        s_Flags2(thing, Flags2(thing) & ~HAS_LISTEN);
        break;

    case A_SPEECHFMT:
        s_Flags3(thing, Flags3(thing) & ~HAS_SPEECHMOD);
        break;

    case A_PROPDIR:
        s_Flags3(thing, Flags3(thing) & ~HAS_PROPDIR);
        break;

    case A_TIMEOUT:
        if (!mudstate.standalone)
        {
            desc_reload(thing);
        }

        break;

    case A_QUEUEMAX:
        if (!mudstate.standalone)
        {
            pcache_reload(thing);
        }

        break;
    }
}

/**
 * @brief add attribute of type atr to list
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param buff  Attribute buffer
 */
void atr_add_raw(dbref thing, int atr, char *buff)
{
    Attr *a = NULL;
    Aname okey;
    DBData key, data;

    makekey(thing, atr, &okey);

    if (!buff || !*buff)
    {
        /** 
         * Delete the entry from cache 
         * 
         */
        key.dptr = &okey;
        key.dsize = sizeof(Aname);
        cache_del(key, DBTYPE_ATTRIBUTE);
        al_delete(thing, atr);
        return;
    }

    if ((a = (Attr *)XMALLOC(strlen(buff) + 1, "a")) == (char *)0)
    {
        return;
    }

    XSTRCPY(a, buff);
    /** 
     * Store the value in cache 
     * 
     */
    key.dptr = &okey;
    key.dsize = sizeof(Aname);
    data.dptr = a;
    data.dsize = strlen(a) + 1;
    cache_put(key, data, DBTYPE_ATTRIBUTE);
    al_add(thing, atr);

    if (!mudstate.standalone && !mudstate.loading_db)
    {
        s_Modified(thing);
    }

    switch (atr)
    {
    case A_STARTUP:
        s_Flags(thing, Flags(thing) | HAS_STARTUP);
        break;

    case A_DAILY:
        s_Flags2(thing, Flags2(thing) | HAS_DAILY);

        if (!mudstate.standalone && !mudstate.loading_db)
        {
            char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
            (void)cron_clr(thing, A_DAILY);
            XSPRINTF(tbuf, "0 %d * * *", mudconf.events_daily_hour);
            call_cron(thing, thing, A_DAILY, tbuf);
            XFREE(tbuf);
        }

        break;

    case A_FORWARDLIST:
        s_Flags2(thing, Flags2(thing) | HAS_FWDLIST);
        break;

    case A_LISTEN:
        s_Flags2(thing, Flags2(thing) | HAS_LISTEN);
        break;

    case A_SPEECHFMT:
        s_Flags3(thing, Flags3(thing) | HAS_SPEECHMOD);
        break;

    case A_PROPDIR:
        s_Flags3(thing, Flags3(thing) | HAS_PROPDIR);
        break;

    case A_TIMEOUT:
        if (!mudstate.standalone)
        {
            desc_reload(thing);
        }

        break;

    case A_QUEUEMAX:
        if (!mudstate.standalone)
        {
            pcache_reload(thing);
        }

        break;
    }
}

/**
 * @brief add attribute of type atr to list
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param buff  Attribute buffer
 * @param owner DBref of owner
 * @param flags Attribute flags
 */
void atr_add(dbref thing, int atr, char *buff, dbref owner, int flags)
{
    char *tbuff = NULL;

    if (!buff || !*buff)
    {
        atr_clr(thing, atr);
    }
    else
    {
        tbuff = atr_encode(buff, thing, owner, flags, atr);
        atr_add_raw(thing, atr, tbuff);
        XFREE(tbuff);
    }
}

/**
 * @brief Set owner of attribute
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner DBref of owner
 */
void atr_set_owner(dbref thing, int atr, dbref owner)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *buff = atr_get(thing, atr, &aowner, &aflags, &alen);

    atr_add(thing, atr, buff, owner, aflags);

    XFREE(buff);
}

/**
 * @brief Set flag of attribute
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param flags Flags
 */
void atr_set_flags(dbref thing, int atr, dbref flags)
{
    dbref aowner = NOTHING;
    int aflags = 0, alen = 0;
    char *buff = atr_get(thing, atr, &aowner, &aflags, &alen);

    atr_add(thing, atr, buff, aowner, flags);

    XFREE(buff);
}

/**
 * @brief Get an attribute from the database.
 * 
 * @param thing DBref of thing
 * @param atr Attribute type
 * @return char* 
 */
char *atr_get_raw(dbref thing, int atr)
{
    DBData key, data;
    Aname okey;

    if (Typeof(thing) == TYPE_GARBAGE)
    {
        return NULL;
    }

    if (!mudstate.standalone && !mudstate.loading_db)
    {
        s_Accessed(thing);
    }

    makekey(thing, atr, &okey);
    /** 
     * Fetch the entry from cache and return it 
     * 
     */
    key.dptr = &okey;
    key.dsize = sizeof(Aname);
    data = cache_get(key, DBTYPE_ATTRIBUTE);
    return data.dptr;
}

/**
 * @brief Get an attribute from the database.
 * 
 * @param s     String buffer
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner DBref of owner
 * @param flags Attribute flag
 * @param alen  Attribute length
 * @return char* 
 */
char *atr_get_str(char *s, dbref thing, int atr, dbref *owner, int *flags, int *alen)
{
    char *buff = atr_get_raw(thing, atr);

    if (!buff)
    {
        *owner = Owner(thing);
        *flags = 0;
        *alen = 0;
        *s = '\0';
    }
    else
    {
        atr_decode(buff, s, thing, owner, flags, atr, alen);
    }

    return s;
}

/**
 * @brief Get an attribute from the database.
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner DBref of owner
 * @param flags Flags
 * @param alen  Attribute length
 * @return char* 
 */
char *atr_get(dbref thing, int atr, dbref *owner, int *flags, int *alen)
{
    char *buff = XMALLOC(LBUF_SIZE, "buff");

    return atr_get_str(buff, thing, atr, owner, flags, alen);
}

/**
 * @brief Get information about an attribute
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner DBref of owner
 * @param flags Flags
 * @return bool 
 */
bool atr_get_info(dbref thing, int atr, dbref *owner, int *flags)
{
    int alen = 0;
    char *buff = atr_get_raw(thing, atr);

    if (!buff)
    {
        *owner = Owner(thing);
        *flags = 0;
        return false;
    }

    atr_decode(buff, NULL, thing, owner, flags, atr, &alen);
    return true;
}

/**
 * @brief Get a propdir attribute
 * 
 * @param s     String buffer
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner Owner of thing
 * @param flags Flags
 * @param alen  Attribute length
 * @return char* 
 */
char *atr_pget_str(char *s, dbref thing, int atr, dbref *owner, int *flags, int *alen)
{
    char *buff = NULL;
    dbref parent = NOTHING;
    int lev = 0;
    ATTR *ap = NULL;
    PROPDIR *pp = NULL;

    ITER_PARENTS(thing, parent, lev)
    {
        buff = atr_get_raw(parent, atr);

        if (buff && *buff)
        {
            atr_decode(buff, s, thing, owner, flags, atr, alen);

            if ((lev == 0) || !(*flags & AF_PRIVATE))
            {
                return s;
            }
        }

        if ((lev == 0) && Good_obj(Parent(parent)))
        {
            ap = atr_num(atr);

            if (!ap || ap->flags & AF_PRIVATE)
            {
                break;
            }
        }
    }

    if (H_Propdir(thing) && ((pp = propdir_get(thing)) != NULL))
    {
        for (lev = 0; (lev < pp->count) && (lev < mudconf.propdir_lim); lev++)
        {
            parent = pp->data[lev];

            if (Good_obj(parent) && (parent != thing))
            {
                buff = atr_get_raw(parent, atr);

                if (buff && *buff)
                {
                    atr_decode(buff, s, thing, owner, flags, atr, alen);

                    if (!(*flags & AF_PRIVATE))
                    {
                        return s;
                    }
                }
            }
        }
    }

    *owner = Owner(thing);
    *flags = 0;
    *alen = 0;
    *s = '\0';
    return s;
}

/**
 * @brief  * @brief Get a propdir attribute
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner Owner of thing
 * @param flags Flags
 * @param alen  Attribute length
 * @return char* 
 */
char *atr_pget(dbref thing, int atr, dbref *owner, int *flags, int *alen)
{
    char *buff = XMALLOC(LBUF_SIZE, "buff");

    return atr_pget_str(buff, thing, atr, owner, flags, alen);
}

/**
 * @brief Get information about a propdir attribute
 * 
 * @param thing DBref of thing
 * @param atr   Attribute type
 * @param owner DBref of owner
 * @param flags Flags
 * @return int 
 */
int atr_pget_info(dbref thing, int atr, dbref *owner, int *flags)
{
    char *buff = NULL;
    dbref parent = NOTHING;
    int lev = 0, alen = 0;
    ATTR *ap = NULL;
    PROPDIR *pp = NULL;

    ITER_PARENTS(thing, parent, lev)
    {
        buff = atr_get_raw(parent, atr);

        if (buff && *buff)
        {
            atr_decode(buff, NULL, thing, owner, flags, atr, &alen);

            if ((lev == 0) || !(*flags & AF_PRIVATE))
            {
                return 1;
            }
        }

        if ((lev == 0) && Good_obj(Parent(parent)))
        {
            ap = atr_num(atr);

            if (!ap || ap->flags & AF_PRIVATE)
            {
                break;
            }
        }
    }

    if (H_Propdir(thing) && ((pp = propdir_get(thing)) != NULL))
    {
        for (lev = 0; (lev < pp->count) && (lev < mudconf.propdir_lim); lev++)
        {
            parent = pp->data[lev];

            if (Good_obj(parent) && (parent != thing))
            {
                buff = atr_get_raw(parent, atr);

                if (buff && *buff)
                {
                    atr_decode(buff, NULL, thing, owner, flags, atr, &alen);

                    if (!(*flags & AF_PRIVATE))
                    {
                        return 1;
                    }
                }
            }
        }
    }

    *owner = Owner(thing);
    *flags = 0;
    return 0;
}

/**
 * @brief Remove all attributes of an object.
 * 
 * @param thing DBref of thing
 */
void atr_free(dbref thing)
{
    int attr = 0;
    char *as = NULL;

    atr_push();

    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as))
    {
        atr_clr(thing, attr);
    }

    atr_pop();
    /**
     * Just to be on the safe side 
     * 
     */
    al_destroy(thing);
}

/**
 * @brief Copy all attributes from one object to another.  Takes the
 * player argument to ensure that only attributes that COULD be set by
 * the player are copied.
 * 
 * @param player    DBref of player
 * @param dest      DBref of source
 * @param source    DBref of destination
 */
void atr_cpy(dbref player, dbref dest, dbref source)
{
    int attr = 0, aflags = 0, alen = 0;
    ATTR *at = NULL;
    dbref aowner = NOTHING, owner = Owner(dest);
    char *as = NULL, *buf = XMALLOC(LBUF_SIZE, "buf");

    atr_push();

    for (attr = atr_head(source, &as); attr; attr = atr_next(&as))
    {
        buf = atr_get_str(buf, source, attr, &aowner, &aflags, &alen);

        if (!(aflags & AF_LOCK))
        {
            /** 
             * change owner 
             * 
             */
            aowner = owner;
        }

        at = atr_num(attr);

        if (attr && at)
        {
            if (Write_attr(owner, dest, at, aflags))
            {
                /** 
                 * Only set attrs that owner has perm to set
                 * 
                 */
                atr_add(dest, attr, buf, aowner, aflags);
            }
        }
    }

    atr_pop();
    XFREE(buf);
}

/**
 * @brief Change the ownership of the attributes of an object to the
 * current owner if they are not locked.
 * 
 * @param obj DBref of object
 */
void atr_chown(dbref obj)
{
    int attr = 0, aflags = 0, alen = 0;
    dbref aowner = NOTHING, owner = Owner(obj);
    char *as, *buf = XMALLOC(LBUF_SIZE, "buf");

    atr_push();

    for (attr = atr_head(obj, &as); attr; attr = atr_next(&as))
    {
        buf = atr_get_str(buf, obj, attr, &aowner, &aflags, &alen);

        if ((aowner != owner) && !(aflags & AF_LOCK))
        {
            atr_add(obj, attr, buf, owner, aflags);
        }
    }

    atr_pop();
    XFREE(buf);
}

/**
 * @brief Return next attribute in attribute list.
 * 
 * @param attrp 
 * @return int 
 */
int atr_next(char **attrp)
{
    if (!*attrp || !**attrp)
    {
        return 0;
    }
    else
    {
        return al_decode(attrp);
    }
}

/**
 * @brief Push attr lists.
 * 
 */
void atr_push(void)
{
    ALIST *new_alist = (ALIST *)XMALLOC(SBUF_SIZE, "new_alist");

    new_alist->data = mudstate.iter_alist.data;
    new_alist->len = mudstate.iter_alist.len;
    new_alist->next = mudstate.iter_alist.next;
    mudstate.iter_alist.data = NULL;
    mudstate.iter_alist.len = 0;
    mudstate.iter_alist.next = new_alist;

    return;
}

/**
 * @brief Pop attr lists.
 * 
 */
void atr_pop(void)
{
    ALIST *old_alist = mudstate.iter_alist.next;

    if (mudstate.iter_alist.data)
    {
        XFREE(mudstate.iter_alist.data);
    }

    if (old_alist)
    {
        mudstate.iter_alist.data = old_alist->data;
        mudstate.iter_alist.len = old_alist->len;
        mudstate.iter_alist.next = old_alist->next;
        XFREE((char *)old_alist);
    }
    else
    {
        mudstate.iter_alist.data = NULL;
        mudstate.iter_alist.len = 0;
        mudstate.iter_alist.next = NULL;
    }

    return;
}

/**
 * @brief Returns the head of the attr list for object 'thing'
 * 
 * @param thing DBref of thing
 * @param attrp Attribute
 * @return int 
 */
int atr_head(dbref thing, char **attrp)
{
    char *astr = NULL;
    int alen = 0;

    /** 
     * Get attribute list. Save a read if it is in the modify atr list
     * 
     */
    if (thing == mudstate.mod_al_id)
    {
        astr = mudstate.mod_alist;
    }
    else
    {
        astr = atr_get_raw(thing, A_LIST);
    }

    alen = al_size(astr);

    /** 
     * If no list, return nothing
     * 
     */
    if (!alen)
    {
        return 0;
    }

    /** 
     * Set up the list and return the first entry
     * 
     */
    al_extend(&mudstate.iter_alist.data, &mudstate.iter_alist.len, alen, 0);
    XMEMCPY(mudstate.iter_alist.data, astr, alen);
    *attrp = mudstate.iter_alist.data;
    return atr_next(attrp);
}

#define SIZE_HACK 1 /* So mistaken refs to #-1 won't die. */

/**
 * @brief Initialize an object
 * 
 * @param first DBref of first object to initialize
 * @param last DBref of last object to initialize
 */
void initialize_objects(dbref first, dbref last)
{
    dbref thing = NOTHING;

    for (thing = first; thing < last; thing++)
    {
        s_Owner(thing, GOD);
        s_Flags(thing, (TYPE_GARBAGE | GOING));
        s_Powers(thing, 0);
        s_Powers2(thing, 0);
        s_Location(thing, NOTHING);
        s_Contents(thing, NOTHING);
        s_Exits(thing, NOTHING);
        s_Link(thing, NOTHING);
        s_Next(thing, NOTHING);
        s_Zone(thing, NOTHING);
        s_Parent(thing, NOTHING);
#ifdef MEMORY_BASED
        db[thing].attrtext.atrs = NULL;
        db[thing].attrtext.at_count = 0;
#endif
    }
}

/**
 * @brief Extend the struct database.
 * 
 * @param newtop New top of database
 */
void db_grow(dbref newtop)
{
    int newsize = 0, marksize = 0, delta = 0;
    MARKBUF *newmarkbuf = NULL;
    OBJ *newdb = NULL;
    NAME *newnames = NULL, *newpurenames = NULL;
    char *cp = NULL;

    if (!mudstate.standalone)
    {
        delta = mudconf.init_size;
    }
    else
    {
        delta = 1000;
    }

    /** 
     * Determine what to do based on requested size, current top and
     * size. Make sure we grow in reasonable-size chunks to prevent
     * frequent reallocations of the db array.
     * 
     */
    if (newtop <= mudstate.db_top)
    {
        /**
         * If requested size is smaller than the current db size, ignore it
         * 
         */
        return;
    }

    /** 
     * If requested size is greater than the current db size but smaller
     * than the amount of space we have allocated, raise the db size
     * and initialize the new area.
     * 
     */
    if (newtop <= mudstate.db_size)
    {
        for (int i = mudstate.db_top; i < newtop; i++)
        {
            names[i] = NULL;
            purenames[i] = NULL;
        }

        initialize_objects(mudstate.db_top, newtop);
        mudstate.db_top = newtop;
        return;
    }

    /**
     * Grow by a minimum of delta objects
     * 
     */

    if (newtop <= mudstate.db_size + delta)
    {
        newsize = mudstate.db_size + delta;
    }
    else
    {
        newsize = newtop;
    }

    /** 
     * Enforce minimum database size
     * 
     */

    if (newsize < mudstate.min_size)
    {
        newsize = mudstate.min_size + delta;
    }

    /**
     * Grow the name tables
     * 
     */
    newnames = (NAME *)XCALLOC(newsize + SIZE_HACK, sizeof(NAME), "newnames");

    if (!newnames)
    {
        log_write_raw(1, "ABORT! db.c, could not allocate space for %d item name cache in db_grow().\n", newsize);
        abort();
    }

    if (names)
    {
        /** 
         * An old name cache exists.  Copy it. 
         * 
         */
        names -= SIZE_HACK;
        XMEMCPY((char *)newnames, (char *)names, (newtop + SIZE_HACK) * sizeof(NAME));
        cp = (char *)names;
        XFREE(cp);
    }
    else
    {
        /** 
         * Creating a brand new struct database. Fill in the
    	 * 'reserved' area in case it gets referenced.
         * 
    	 */
        names = newnames;

        for (int i = 0; i < SIZE_HACK; i++)
        {
            names[i] = NULL;
        }
    }

    names = newnames + SIZE_HACK;
    newnames = NULL;
    newpurenames = (NAME *)XCALLOC(newsize + SIZE_HACK, sizeof(NAME), "newpurenames");

    if (!newpurenames)
    {
        log_write_raw(1, "ABORT! db.c, could not allocate space for %d item name cache in db_grow().\n", newsize);
        abort();
    }

    XMEMSET((char *)newpurenames, 0, (newsize + SIZE_HACK) * sizeof(NAME));

    if (purenames)
    {
        /** 
         * An old name cache exists.  Copy it.
         * 
         */
        purenames -= SIZE_HACK;
        XMEMCPY((char *)newpurenames, (char *)purenames, (newtop + SIZE_HACK) * sizeof(NAME));
        cp = (char *)purenames;
        XFREE(cp);
    }
    else
    {
        /** 
         * Creating a brand new struct database.  Fill in the
    	 * 'reserved' area in case it gets referenced.
         * 
    	 */
        purenames = newpurenames;

        for (int i = 0; i < SIZE_HACK; i++)
        {
            purenames[i] = NULL;
        }
    }

    purenames = newpurenames + SIZE_HACK;
    newpurenames = NULL;
    /** 
     * Grow the db array
     * 
     */
    newdb = (OBJ *)XCALLOC(newsize + SIZE_HACK, sizeof(OBJ), "newdb");

    if (!newdb)
    {
        log_write(LOG_ALWAYS, "ALC", "DB", "Could not allocate space for %d item struct database.", newsize);
        abort();
    }

    if (db)
    {
        /** 
         * An old struct database exists.  Copy it to the new buffer
         * 
         */
        db -= SIZE_HACK;
        XMEMCPY((char *)newdb, (char *)db, (mudstate.db_top + SIZE_HACK) * sizeof(OBJ));
        cp = (char *)db;
        XFREE(cp);
    }
    else
    {
        /** 
         * Creating a brand new struct database.  Fill in the 
         * 'reserved' area in case it gets referenced.
         * 
    	 */
        db = newdb;

        for (int i = 0; i < SIZE_HACK; i++)
        {
            s_Owner(i, GOD);
            s_Flags(i, (TYPE_GARBAGE | GOING));
            s_Flags2(i, 0);
            s_Flags3(i, 0);
            s_Powers(i, 0);
            s_Powers2(i, 0);
            s_Location(i, NOTHING);
            s_Contents(i, NOTHING);
            s_Exits(i, NOTHING);
            s_Link(i, NOTHING);
            s_Next(i, NOTHING);
            s_Zone(i, NOTHING);
            s_Parent(i, NOTHING);
        }
    }

    db = newdb + SIZE_HACK;
    newdb = NULL;
    /** 
     * Go do the rest of the things
     * 
     */
    CALL_ALL_MODULES(db_grow, (newsize, newtop));

    for (int i = mudstate.db_top; i < newtop; i++)
    {
        names[i] = NULL;
        purenames[i] = NULL;
    }

    initialize_objects(mudstate.db_top, newtop);
    mudstate.db_top = newtop;
    mudstate.db_size = newsize;
    /** 
     * Grow the db mark buffer
     * 
     */
    marksize = (newsize + 7) >> 3;
    newmarkbuf = (MARKBUF *)XMALLOC(marksize, "newmarkbuf");
    XMEMSET((char *)newmarkbuf, 0, marksize);

    if (mudstate.markbits)
    {
        marksize = (newtop + 7) >> 3;
        XMEMCPY((char *)newmarkbuf, (char *)mudstate.markbits, marksize);
        cp = (char *)mudstate.markbits;
        XFREE(cp);
    }

    mudstate.markbits = newmarkbuf;
}

/**
 * @brief Free a DB
 * 
 */
void db_free(void)
{
    if (db != NULL)
    {
        db -= SIZE_HACK;
        XFREE((char *)db);
        db = NULL;
    }

    mudstate.db_top = 0;
    mudstate.db_size = 0;
    mudstate.freelist = NOTHING;
}

/**
 * @brief Create a minimal DB
 * 
 */
void db_make_minimal(void)
{
    dbref obj = NOTHING;

    db_free();
    db_grow(1);
    s_Name(0, "Limbo");
    s_Flags(0, TYPE_ROOM);
    s_Flags2(0, 0);
    s_Flags3(0, 0);
    s_Powers(0, 0);
    s_Powers2(0, 0);
    s_Location(0, NOTHING);
    s_Exits(0, NOTHING);
    s_Link(0, NOTHING);
    s_Parent(0, NOTHING);
    s_Zone(0, NOTHING);
    s_Pennies(0, 1);
    s_Owner(0, 1);

    /** 
     * should be #1 
     * 
     */
    load_player_names();
    obj = create_player((char *)"Wizard", (char *)"potrzebie", NOTHING, 0, 1);
    s_Flags(obj, Flags(obj) | WIZARD);
    s_Flags2(obj, 0);
    s_Flags3(obj, 0);
    s_Powers(obj, 0);
    s_Powers2(obj, 0);
    s_Pennies(obj, 1000);
    /** 
     * Manually link to Limbo, just in case
     */
    s_Location(obj, 0);
    s_Next(obj, NOTHING);
    s_Contents(0, obj);
    s_Link(obj, 0);
}

/**
 * @brief Enforce completely numeric dbrefs
 * 
 * @param s         String to check
 * @return dbref 
 */
dbref parse_dbref_only(const char *s)
{
    int x;

    for (const char *p = s; *p; p++)
    {
        if (!isdigit(*p))
        {
            return NOTHING;
        }
    }

    x = (int)strtol(s, (char **)NULL, 10);
    return ((x >= 0) ? x : NOTHING);
}

/**
 * @brief Parse an object id
 * 
 * @param s String
 * @param p Pointer to ':' in string
 * @return dbref 
 */
dbref parse_objid(const char *s, const char *p)
{
    dbref it = NOTHING;
    time_t tt = 0L;
    const char *r = NULL;
    char *tbuf = XMALLOC(LBUF_SIZE, "tbuf");
    ;

    /** 
     * We're passed two parameters: the start of the string, and the
     * pointer to where the ':' in the string is. If the latter is NULL,
     * go find it.
     * 
     */

    if (p == NULL)
    {
        if ((p = strchr(s, ':')) == NULL)
        {
            XFREE(tbuf);
            return parse_dbref_only(s);
        }
    }

    /** 
     * ObjID takes the format <dbref>:<timestamp as long int>
     * If we match the dbref but its creation time doesn't match the
     * timestamp, we don't have a match.
     */
    XSTRNCPY(tbuf, s, p - s);
    it = parse_dbref_only(tbuf);

    if (Good_obj(it))
    {
        p++;

        for (r = p; *r; r++)
        {
            if (!isdigit(*r))
            {
                XFREE(tbuf);
                return NOTHING;
            }
        }

        tt = (time_t)strtol(p, (char **)NULL, 10);
        XFREE(tbuf);
        return ((CreateTime(it) == tt) ? it : NOTHING);
    }

    XFREE(tbuf);
    return NOTHING;
}

/**
 * @brief Parse string for dbref
 * 
 * @param s String
 * @return dbref 
 */
dbref parse_dbref(const char *s)
{
    int x;

    /* Either pure dbrefs or objids are okay */

    for (const char *p = s; *p; p++)
    {
        if (!isdigit(*p))
        {
            if (*p == ':')
            {
                return parse_objid(s, p);
            }
            else
            {
                return NOTHING;
            }
        }
    }

    x = (int)strtol(s, (char **)NULL, 10);
    return ((x >= 0) ? x : NOTHING);
}

/**
 * @brief Write string to file, escaping char as needed
 * 
 * @param f File
 * @param s String
 */
void putstring(FILE *f, const char *s)
{
    putc('"', f);

    while (s && *s)
    {
        switch (*s)
        {
        case '\n':
            putc('\\', f);
            putc('n', f);
            break;

        case '\r':
            putc('\\', f);
            putc('r', f);
            break;

        case '\t':
            putc('\\', f);
            putc('t', f);
            break;

        case ESC_CHAR:
            putc('\\', f);
            putc('e', f);
            break;

        case '\\':
        case '"':
            putc('\\', f);
            putc(*s, f);
            break;
        default:
            putc(*s, f);
            break;
        }

        s++;
    }

    putc('"', f);
    putc('\n', f);
}

/**
 * @brief Read a trring from file, unescaping char as needed
 * 
 * @param f File
 * @param new_strings 
 * @return char* 
 */
char *getstring(FILE *f, bool new_strings)
{
    char *buf = XMALLOC(LBUF_SIZE, "buf");
    char *p = buf;
    int lastc = 0, c = fgetc(f);

    if (!new_strings || (c != '"'))
    {
        ungetc(c, f);
        c = '\0';

        for (;;)
        {
            lastc = c;
            c = fgetc(f);

            /** 
             * If EOF or null, return
             * 
             */
            if (!c || (c == EOF))
            {
                *p = '\0';
                return buf;
            }

            /** 
             * If a newline, return if prior char is not a cr. Otherwise keep on truckin'
             * 
             */
            if ((c == '\n') && (lastc != '\r'))
            {
                *p = '\0';
                return buf;
            }

            SAFE_LB_CHR(c, buf, &p);
        }
    }
    else
    {
        for (;;)
        {
            c = fgetc(f);

            if (c == '"')
            {
                if ((c = fgetc(f)) != '\n')
                {
                    ungetc(c, f);
                }

                *p = '\0';
                return buf;
            }
            else if (c == '\\')
            {
                c = fgetc(f);

                switch (c)
                {
                case 'n':
                    c = '\n';
                    break;

                case 'r':
                    c = '\r';
                    break;

                case 't':
                    c = '\t';
                    break;

                case 'e':
                    c = ESC_CHAR;
                    break;
                }
            }

            if ((c == '\0') || (c == EOF))
            {
                *p = '\0';
                return buf;
            }

            SAFE_LB_CHR(c, buf, &p);
        }
    }
}

/**
 * @brief Get dbref from file
 * 
 * @param f         File
 * @return dbref 
 */
dbref getref(FILE *f)
{
    dbref d = NOTHING;
    char *buf = XMALLOC(SBUF_SIZE, "buf");

    if (fgets(buf, sizeof(buf), f) != NULL)
    {
        d = ((int)strtol(buf, (char **)NULL, 10));
    }

    XFREE(buf);
    return (d);
}

/**
 * @brief Get long int from file
 * 
 * @param f     File
 * @return long 
 */
long getlong(FILE *f)
{
    long d = 0;
    char *buf = XMALLOC(SBUF_SIZE, "buf");

    if (fgets(buf, sizeof(buf), f) != NULL)
    {
        d = (strtol(buf, (char **)NULL, 10));
    }

    XFREE(buf);
    return (d);
}

/**
 * @brief Initializing a GDBM file
 * 
 * @param gdbmfile Filename
 * @return int 
 */
int init_gdbm_db(char *gdbmfile)
{
    for (mudstate.db_block_size = 1; mudstate.db_block_size < (LBUF_SIZE * 4); mudstate.db_block_size = mudstate.db_block_size << 1)
    {
        /**
         * Calculate proper database block size
         * 
         */
    }

    cache_init(mudconf.cache_width);
    dddb_setfile(gdbmfile);
    dddb_init();
    log_write(LOG_ALWAYS, "INI", "LOAD", "Using db file: %s", gdbmfile);
    db_free();
    return (0);
}

/**
 * @brief checks back through a zone tree for control
 * 
 * @param player 
 * @param thing 
 * @return bool
 */
bool check_zone(dbref player, dbref thing)
{
    if (mudstate.standalone)
    {
        return false;
    }

    if (!mudconf.have_zones || (Zone(thing) == NOTHING) || isPlayer(thing) || (mudstate.zone_nest_num + 1 == mudconf.zone_nest_lim))
    {
        mudstate.zone_nest_num = 0;
        return false;
    }

    /** 
     * We check Control_OK on the thing itself, not on its ZMO
     * that allows us to have things default into a zone without
     * needing to be controlled by that ZMO.
     * 
     */
    if (!Control_ok(thing))
    {
        return false;
    }

    mudstate.zone_nest_num++;

    /** 
     * If the zone doesn't have a ControlLock, DON'T allow control.
     * 
     */
    if (atr_get_raw(Zone(thing), A_LCONTROL) && could_doit(player, Zone(thing), A_LCONTROL))
    {
        mudstate.zone_nest_num = 0;
        return true;
    }
    else
    {
        return check_zone(player, Zone(thing));
    }
}

bool check_zone_for_player(dbref player, dbref thing)
{
    if (!Control_ok(Zone(thing)))
    {
        return false;
    }

    mudstate.zone_nest_num++;

    if (!mudconf.have_zones || (Zone(thing) == NOTHING) || (mudstate.zone_nest_num == mudconf.zone_nest_lim) || !(isPlayer(thing)))
    {
        mudstate.zone_nest_num = 0;
        return false;
    }

    if (atr_get_raw(Zone(thing), A_LCONTROL) && could_doit(player, Zone(thing), A_LCONTROL))
    {
        mudstate.zone_nest_num = 0;
        return true;
    }
    else
    {
        return check_zone(player, Zone(thing));
    }
}

/**
 * @brief Write a restart db.
 * 
 */
void dump_restart_db(void)
{
    FILE *f = NULL;
    DESC *d = NULL;
    int version = 0;
    char *dbf = NULL;

    /** 
     * We maintain a version number for the restart database,
     * so we can restart even if the format of the restart db
     * has been changed in the new executable.
     * 
     */
    version |= RS_RECORD_PLAYERS;
    version |= RS_NEW_STRINGS;
    version |= RS_COUNT_REBOOTS;
    dbf = XASPRINTF("dbf", "%s/%s.db.RESTART", mudconf.dbhome, mudconf.mud_shortname);
    f = fopen(dbf, "w");
    XFREE(dbf);
    fprintf(f, "+V%d\n", version);
    putref(f, sock);
    putlong(f, mudstate.start_time);
    putref(f, mudstate.reboot_nums);
    putstring(f, mudstate.doing_hdr);
    putref(f, mudstate.record_players);
    DESC_ITER_ALL(d)
    {
        putref(f, d->descriptor);
        putref(f, d->flags);
        putlong(f, d->connected_at);
        putref(f, d->command_count);
        putref(f, d->timeout);
        putref(f, d->host_info);
        putref(f, d->player);
        putlong(f, d->last_time);
        putstring(f, d->output_prefix);
        putstring(f, d->output_suffix);
        putstring(f, d->addr);
        putstring(f, d->doing);
        putstring(f, d->username);
    }
    putref(f, 0);
    fclose(f);
}

/**
 * @brief Load a restart DB
 * 
 */
void load_restart_db(void)
{
    DESC *d = NULL;
    DESC *p = NULL;
    struct stat fstatbuf;
    int val = 0, version = 0, new_strings = 0;
    char *temp = NULL, *buf = XMALLOC(SBUF_SIZE, "buf");
    char *dbf = XASPRINTF("dbf", "%s/%s.db.RESTART", mudconf.dbhome, mudconf.mud_shortname);
    FILE *f = fopen(dbf, "r");

    if (!f)
    {
        mudstate.restarting = 0;
        return;
    }

    mudstate.restarting = 1;

    if (fgets(buf, 3, f) != NULL)
    {
        if (strncmp(buf, "+V", 2))
        {
            abort();
        }
    }
    else
    {
        abort();
    }

    version = getref(f);
    sock = getref(f);

    if (version & RS_NEW_STRINGS)
    {
        new_strings = 1;
    }

    maxd = sock + 1;
    mudstate.start_time = (time_t)getlong(f);

    if (version & RS_COUNT_REBOOTS)
    {
        mudstate.reboot_nums = getref(f) + 1;
    }

    mudstate.doing_hdr = getstring(f, new_strings);

    if (version & RS_CONCENTRATE)
    {
        (void)getref(f);
    }

    if (version & RS_RECORD_PLAYERS)
    {
        mudstate.record_players = getref(f);
    }

    while ((val = getref(f)) != 0)
    {
        ndescriptors++;
        d = XMALLOC(sizeof(DESC), "d");
        d->descriptor = val;
        d->flags = getref(f);
        d->connected_at = (time_t)getlong(f);
        d->retries_left = mudconf.retry_limit;
        d->command_count = getref(f);
        d->timeout = getref(f);
        d->host_info = getref(f);
        d->player = getref(f);
        d->last_time = (time_t)getlong(f);
        temp = getstring(f, new_strings);

        if (*temp)
        {
            d->output_prefix = temp;
        }
        else
        {
            d->output_prefix = NULL;
        }

        temp = getstring(f, new_strings);

        if (*temp)
        {
            d->output_suffix = temp;
        }
        else
        {
            d->output_suffix = NULL;
        }

        temp = getstring(f, new_strings);
        XSTRNCPY(d->addr, temp, 50);
        XFREE(temp);

        temp = getstring(f, new_strings);
        d->doing = sane_doing(temp, "doing");
        XFREE(temp);

        temp = getstring(f, new_strings);
        XSTRNCPY(d->username, temp, 10);
        XFREE(temp);

        d->colormap = NULL;

        if (version & RS_CONCENTRATE)
        {
            (void)getref(f);
            (void)getref(f);
        }

        d->output_size = 0;
        d->output_tot = 0;
        d->output_lost = 0;
        d->output_head = NULL;
        d->output_tail = NULL;
        d->input_head = NULL;
        d->input_tail = NULL;
        d->input_size = 0;
        d->input_tot = 0;
        d->input_lost = 0;
        d->raw_input = NULL;
        d->raw_input_at = NULL;
        d->quota = mudconf.cmd_quota_max;
        d->program_data = NULL;
        d->hashnext = NULL;

        /**
         * Note that d->address is NOT INITIALIZED, and it DOES get used later, particularly when checking logout.
         * 
         */

        if (descriptor_list)
        {
            for (p = descriptor_list; p->next; p = p->next)
                ;

            d->prev = &p->next;
            p->next = d;
            d->next = NULL;
        }
        else
        {
            d->next = descriptor_list;
            d->prev = &descriptor_list;
            descriptor_list = d;
        }

        if (d->descriptor >= maxd)
        {
            maxd = d->descriptor + 1;
        }

        desc_addhash(d);

        if (isPlayer(d->player))
        {
            s_Flags2(d->player, Flags2(d->player) | CONNECTED);
        }
    }

    /** 
     * In case we've had anything bizarre happen...
     * 
     */
    DESC_ITER_ALL(d)
    {
        if (fstat(d->descriptor, &fstatbuf) < 0)
        {
            log_write(LOG_PROBLEMS, "ERR", "RESTART", "Bad descriptor %d", d->descriptor);
            shutdownsock(d, R_SOCKDIED);
        }
    }
    DESC_ITER_CONN(d)
    {
        if (!isPlayer(d->player))
        {
            shutdownsock(d, R_QUIT);
        }
    }
    fclose(f);
    remove(dbf);
    XFREE(dbf);
    XFREE(buf);
}
