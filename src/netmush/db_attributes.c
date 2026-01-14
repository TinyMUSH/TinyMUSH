
/**
 * @file db_attributes.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Attribute management system
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms of the Artistic License,
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
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @brief Initialize the attribute hash tables.
 *
 */
void init_attrtab(void)
{
    ATTR *a = NULL;
    char *p = NULL, *q = NULL, *buff = XMALLOC(SBUF_SIZE, "buff");

    hashinit(&mushstate.attr_name_htab, 100 * mushconf.hash_factor, HT_STR);

    for (a = attr; a->number; a++)
    {
        anum_extend(a->number);
        anum_set(a->number, a);

        for (p = buff, q = (char *)a->name; *q; p++, q++)
        {
            *p = toupper(*q);
        }

        *p = '\0';
        hashadd(buff, (int *)a, &mushstate.attr_name_htab, 0);
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
    if (!mushstate.standalone)
    {
        a = (ATTR *)hashfind_generic((HASHKEY)(buff), (&mushstate.attr_name_htab));

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

    if (mushstate.standalone)
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

    if (!mushstate.standalone)
    {
        delta = mushconf.init_size;
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
        if (!mushstate.standalone)
        {
            vflags = mushconf.vattr_flags;

            for (kp = mushconf.vattr_flag_list; kp != NULL; kp = kp->next)
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
            va = vattr_alloc(buff, mushconf.vattr_flags);
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

    if ((!Has_Commands(thing) && mushconf.req_cmds_flag) || Halted(thing))
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
    if (mushstate.mod_al_id != NOTHING)
    {
        if (mushstate.mod_alist && *mushstate.mod_alist)
        {
            atr_add_raw(mushstate.mod_al_id, A_LIST, mushstate.mod_alist);
        }
        else
        {
            atr_clr(mushstate.mod_al_id, A_LIST);
        }
    }

    mushstate.mod_al_id = NOTHING;
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
    if (mushstate.mod_al_id == thing)
    {
        return mushstate.mod_alist;
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
        al_extend(&mushstate.mod_alist, &mushstate.mod_size, len, 0);
        XMEMCPY(mushstate.mod_alist, astr, len);
        XFREE(astr);
    }
    else
    {
        al_extend(&mushstate.mod_alist, &mushstate.mod_size, 1, 0);
        *mushstate.mod_alist = '\0';
    }

    mushstate.mod_al_id = thing;
    return mushstate.mod_alist;
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
    al_extend(&mushstate.mod_alist, &mushstate.mod_size, (cp - abuf + ATR_BUF_INCR), 1);

    if (mushstate.mod_alist != abuf)
    {
        /**
         * extend returned different buffer, re-find the end
         *
         */
        abuf = mushstate.mod_alist;

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
void makekey(dbref thing, int atr, UDB_ANAME *abuff)
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
    if (mushstate.mod_al_id == thing)
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
                *alen = strlen(iattr);
                XMEMCPY(oattr, iattr, (size_t)(*alen) + 1);
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
                *alen = strlen(iattr);
                XMEMCPY(oattr, iattr, (size_t)(*alen) + 1);
            }
        }

        /**
         * Get the attribute text
         *
         */
        if (oattr)
        {
            *alen = strlen(cp);
            XMEMCPY(oattr, cp, (size_t)(*alen) + 1);
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
            *alen = strlen(iattr);
            XMEMCPY(oattr, iattr, (size_t)(*alen) + 1);
        }
    }
}

static void notify_module_attr_put(UDB_DATA key)
{
    for (MODULE *mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if (mp->cache_put_notify)
        {
            mp->cache_put_notify(key, DBTYPE_ATTRIBUTE);
        }
    }
}

static void notify_module_attr_del(UDB_DATA key)
{
    for (MODULE *mp = mushstate.modules_list; mp != NULL; mp = mp->next)
    {
        if (mp->cache_del_notify)
        {
            mp->cache_del_notify(key, DBTYPE_ATTRIBUTE);
        }
    }
}

static void db_attribute_delete(UDB_ANAME *okey)
{
    UDB_DATA key = {okey, sizeof(UDB_ANAME)};

    notify_module_attr_del(key);

    if (mushstate.standalone)
    {
        pipe_del_attrib(okey->attrnum, okey->object);
        return;
    }

    db_lock();
    pipe_del_attrib(okey->attrnum, okey->object);
    attrib_sync();
    db_unlock();
}

static void db_attribute_store(UDB_ANAME *okey, char *value)
{
    UDB_DATA key = {okey, sizeof(UDB_ANAME)};

    notify_module_attr_put(key);

    if (mushstate.standalone)
    {
        pipe_set_attrib(okey->attrnum, okey->object, value);
        XFREE(value);
        return;
    }

    db_lock();
    pipe_set_attrib(okey->attrnum, okey->object, value);
    attrib_sync();
    db_unlock();

    XFREE(value);
}

static char *db_attribute_fetch(UDB_ANAME *okey)
{
    return pipe_get_attrib(okey->attrnum, okey->object);
}

int db_sync_attributes(void)
{
    if (mushstate.standalone || mushstate.restarting)
    {
        dddb_setsync(0);
    }

    db_lock();
    attrib_sync();
    db_unlock();

    if (mushstate.standalone || mushstate.restarting)
    {
        dddb_setsync(1);
    }

    return 0;
}

/**
 * @brief clear an attribute in the list.
 *
 * @param thing DBref of thing
 * @param atr Attribute number
 */
void atr_clr(dbref thing, int atr)
{
    UDB_ANAME okey;

    /**
     * Delete the entry from cache
     *
     */
    makekey(thing, atr, &okey);
    db_attribute_delete(&okey);
    al_delete(thing, atr);

    if (!mushstate.standalone && !mushstate.loading_db)
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

        if (!mushstate.standalone)
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
        if (!mushstate.standalone)
        {
            desc_reload(thing);
        }

        break;

    case A_QUEUEMAX:
        if (!mushstate.standalone)
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
    char *a = NULL;
    UDB_ANAME okey;

    makekey(thing, atr, &okey);

    if (!buff || !*buff)
    {
        /**
         * Delete the entry from cache
         *
         */
        db_attribute_delete(&okey);
        al_delete(thing, atr);
        return;
    }

    if ((a = (char *)XMALLOC(strlen(buff) + 1, "a")) == (char *)0)
    {
        return;
    }

    XSTRCPY(a, buff);
    /**
     * Store the value in cache
     *
     */
    db_attribute_store(&okey, a);
    al_add(thing, atr);

    if (!mushstate.standalone && !mushstate.loading_db)
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

        if (!mushstate.standalone && !mushstate.loading_db)
        {
            char *tbuf = XMALLOC(SBUF_SIZE, "tbuf");
            (void)cron_clr(thing, A_DAILY);
            XSPRINTF(tbuf, "0 %d * * *", mushconf.events_daily_hour);
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
        if (!mushstate.standalone)
        {
            desc_reload(thing);
        }

        break;

    case A_QUEUEMAX:
        if (!mushstate.standalone)
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
 * Caller owns the returned buffer and must free it with XFREE().
 *
 * @param thing DBref of thing
 * @param atr Attribute type
 * @return char*
 */
char *atr_get_raw(dbref thing, int atr)
{
    UDB_ANAME okey;

    if (Typeof(thing) == TYPE_GARBAGE)
    {
        return NULL;
    }

    if (!mushstate.standalone && !mushstate.loading_db)
    {
        s_Accessed(thing);
    }

    makekey(thing, atr, &okey);
    /**
     * Fetch the entry from cache and return it
     *
     */
    return db_attribute_fetch(&okey);
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

    if (buff)
    {
        XFREE(buff);
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
    XFREE(buff);
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

    for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
    {
        buff = atr_get_raw(parent, atr);

        if (buff && *buff)
        {
            atr_decode(buff, s, thing, owner, flags, atr, alen);

            if ((lev == 0) || !(*flags & AF_PRIVATE))
            {
                XFREE(buff);
                return s;
            }
        }

        if (buff)
        {
            XFREE(buff);
            buff = NULL;
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
        for (lev = 0; (lev < pp->count) && (lev < mushconf.propdir_lim); lev++)
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
                        XFREE(buff);
                        return s;
                    }
                }

                if (buff)
                {
                    XFREE(buff);
                    buff = NULL;
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

    for (lev = 0, parent = thing; (Good_obj(parent) && (lev < mushconf.parent_nest_lim)); parent = Parent(parent), lev++)
    {
        buff = atr_get_raw(parent, atr);

        if (buff && *buff)
        {
            atr_decode(buff, NULL, thing, owner, flags, atr, &alen);

            if ((lev == 0) || !(*flags & AF_PRIVATE))
            {
                XFREE(buff);
                return 1;
            }
        }

        if (buff)
        {
            XFREE(buff);
            buff = NULL;
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
        for (lev = 0; (lev < pp->count) && (lev < mushconf.propdir_lim); lev++)
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
                        XFREE(buff);
                        return 1;
                    }
                }

                if (buff)
                {
                    XFREE(buff);
                    buff = NULL;
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

    new_alist->data = mushstate.iter_alist.data;
    new_alist->len = mushstate.iter_alist.len;
    new_alist->next = mushstate.iter_alist.next;
    mushstate.iter_alist.data = NULL;
    mushstate.iter_alist.len = 0;
    mushstate.iter_alist.next = new_alist;

    return;
}

/**
 * @brief Pop attr lists.
 *
 */
void atr_pop(void)
{
    ALIST *old_alist = mushstate.iter_alist.next;

    if (mushstate.iter_alist.data)
    {
        XFREE(mushstate.iter_alist.data);
    }

    if (old_alist)
    {
        mushstate.iter_alist.data = old_alist->data;
        mushstate.iter_alist.len = old_alist->len;
        mushstate.iter_alist.next = old_alist->next;
        XFREE((char *)old_alist);
    }
    else
    {
        mushstate.iter_alist.data = NULL;
        mushstate.iter_alist.len = 0;
        mushstate.iter_alist.next = NULL;
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
    bool needs_free = false;
    int alen = 0;

    /**
     * Get attribute list. Save a read if it is in the modify atr list
     *
     */
    if (thing == mushstate.mod_al_id)
    {
        astr = mushstate.mod_alist;
    }
    else
    {
        astr = atr_get_raw(thing, A_LIST);
        needs_free = true;
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
    al_extend(&mushstate.iter_alist.data, &mushstate.iter_alist.len, alen, 0);
    XMEMCPY(mushstate.iter_alist.data, astr, alen);

    if (needs_free && astr)
    {
        XFREE(astr);
    }

    *attrp = mushstate.iter_alist.data;
    return atr_next(attrp);
}

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
    }
}
