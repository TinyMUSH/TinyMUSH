/**
 * @file udb_ocache.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief LRU caching
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

#include <string.h>

/* initial settings for cache sizes */

int cwidth = CACHE_WIDTH;

/* sys_c points to all cache lists */

UDB_CHAIN *sys_c;

/* freelist points to an alternate linked list kept in LRU order */

UDB_CHAIN *freelist;
int cache_initted = 0;
int cache_frozen = 0;

/* cache stats */

time_t cs_ltime;
int cs_writes = 0;   /* total writes */
int cs_reads = 0;    /* total reads */
int cs_dbreads = 0;  /* total read-throughs */
int cs_dbwrites = 0; /* total write-throughs */
int cs_dels = 0;     /* total deletes */
int cs_checks = 0;   /* total checks */
int cs_rhits = 0;    /* total reads filled from cache */
int cs_ahits = 0;    /* total reads filled active cache */
int cs_whits = 0;    /* total writes to dirty cache */
int cs_fails = 0;    /* attempts to grab nonexistent */
int cs_syncs = 0;    /* total cache syncs */
int cs_size = 0;     /* total cache size */

int cachehash(void *keydata, int keylen, unsigned int type)
{
    unsigned int hash = 0;
    char *sp;

    if (keydata == NULL)
    {
        return 0;
    }

    for (sp = (char *)keydata; (sp - (char *)keydata) < keylen; sp++)
    {
        hash = (hash << 5) + hash + *sp;
    }

    return ((hash + type) % cwidth);
}

void cache_repl(UDB_CACHE *cp, void *new, int len, unsigned int type, unsigned int flags)
{
    cs_size -= cp->datalen;

    if (cp->data != NULL)
    {
        XFREE(cp->data);
    }

    cp->data = new;
    cp->datalen = len;
    cp->type = type;
    cp->flags = flags;
    cs_size += cp->datalen;
}

int cache_init(int width)
{
    int x;
    UDB_CHAIN *sp;

    if (cache_initted || sys_c != (UDB_CHAIN *)0)
    {
        return (0);
    }

    /*
     * If width is specified as non-zero, change it to that,
     * * otherwise use default.
     */

    if (width)
    {
        cwidth = width;
    }

    sp = sys_c = (UDB_CHAIN *)XMALLOC((unsigned)cwidth * sizeof(UDB_CHAIN), "sys_c");

    if (sys_c == (UDB_CHAIN *)0)
    {
        warning("cache_init: cannot allocate cache: ", (char *)-1, "\n", (char *)0);
        return (-1);
    }

    freelist = (UDB_CHAIN *)XMALLOC(sizeof(UDB_CHAIN), "freelist");

    /*
     * Allocate the initial cache entries
     */

    for (x = 0; x < cwidth; x++, sp++)
    {
        sp->head = (UDB_CACHE *)0;
        sp->tail = (UDB_CACHE *)0;
    }

    /*
     * Init the LRU freelist
     */
    freelist->head = (UDB_CACHE *)0;
    freelist->tail = (UDB_CACHE *)0;

    /*
     * Initialize the object pipelines
     */

    for (x = 0; x < NUM_OBJPIPES; x++)
    {
        mushstate.objpipes[x] = NULL;
    }

    /*
     * Initialize the object access counter
     */
    mushstate.objc = 0;
    /*
     * mark caching system live
     */
    cache_initted++;
    cs_ltime = time(NULL);
    return (0);
}

void cache_reset(void)
{
    int x;
    UDB_CACHE *cp, *nxt;
    UDB_CHAIN *sp;
    UDB_DATA key, data;
    /*
     * Clear the cache after startup and reset stats
     */
    db_lock();

    for (x = 0; x < cwidth; x++, sp++)
    {
        sp = &sys_c[x];

        /*
	 * traverse the chain
	 */
        for (cp = sp->head; cp != NULL; cp = nxt)
        {
            nxt = cp->nxt;

            if (cp->flags & CACHE_DIRTY)
            {
                if (cp->data == NULL)
                {
                    switch (cp->type)
                    {
                    case DBTYPE_ATTRIBUTE:
                        pipe_del_attrib(((UDB_ANAME *)cp->keydata)->attrnum, ((UDB_ANAME *)cp->keydata)->object);
                        break;

                    default:
                        key.dptr = cp->keydata;
                        key.dsize = cp->keylen;
                        db_del(key, cp->type);
                    }

                    cs_dels++;
                }
                else
                {
                    switch (cp->type)
                    {
                    case DBTYPE_ATTRIBUTE:
                        pipe_set_attrib(((UDB_ANAME *)cp->keydata)->attrnum, ((UDB_ANAME *)cp->keydata)->object, (char *)cp->data);
                        break;

                    default:
                        key.dptr = cp->keydata;
                        key.dsize = cp->keylen;
                        data.dptr = cp->data;
                        data.dsize = cp->datalen;
                        db_put(key, data, cp->type);
                    }

                    cs_dbwrites++;
                }
            }

            cache_repl(cp, NULL, 0, DBTYPE_EMPTY, 0);
            XFREE(cp->keydata);
            XFREE(cp);
        }

        sp->head = (UDB_CACHE *)0;
        sp->tail = (UDB_CACHE *)0;
    }

    freelist->head = (UDB_CACHE *)0;
    freelist->tail = (UDB_CACHE *)0;
    db_unlock();
    /*
     * Clear the counters after startup, or they'll be skewed
     */
    cs_writes = 0;   /* total writes */
    cs_reads = 0;    /* total reads */
    cs_dbreads = 0;  /* total read-throughs */
    cs_dbwrites = 0; /* total write-throughs */
    cs_dels = 0;     /* total deletes */
    cs_checks = 0;   /* total checks */
    cs_rhits = 0;    /* total reads filled from cache */
    cs_ahits = 0;    /* total reads filled active cache */
    cs_whits = 0;    /* total writes to dirty cache */
    cs_fails = 0;    /* attempts to grab nonexistent */
    cs_syncs = 0;    /* total cache syncs */
    cs_size = 0;     /* size of cache in bytes */
}

/* list dbrefs of objects in the cache. */

void list_cached_objs(dbref player)
{
    UDB_CHAIN *sp;
    UDB_CACHE *cp;
    int x;
    int aco, maco, asize, msize, oco, moco;
    int *count_array, *size_array;
    char *s;
    aco = maco = asize = msize = oco = moco = 0;
    count_array = (int *)XCALLOC(mushstate.db_top, sizeof(int), "count_array");
    size_array = (int *)XCALLOC(mushstate.db_top, sizeof(int), "size_array");

    for (x = 0, sp = sys_c; x < cwidth; x++, sp++)
    {
        for (cp = sp->head; cp != NULL; cp = cp->nxt)
        {
            if (cp->data && (cp->type == DBTYPE_ATTRIBUTE) && !(cp->flags & CACHE_DIRTY))
            {
                aco++;
                asize += cp->datalen;
                count_array[((UDB_ANAME *)cp->keydata)->object] += 1;
                size_array[((UDB_ANAME *)cp->keydata)->object] += cp->datalen;
            }
        }
    }

    notify(player, "Active Cache                       Dbref                   Attrs           Size");
    notify(player, "---------------------------------- -------------- -------------- --------------");

    for (x = 0; x < mushstate.db_top; x++)
    {
        if (count_array[x] > 0)
        {
            s = strip_ansi(Name(x));
            raw_notify(player, "%-34.34s #%-13d %14d %14d", s, x, count_array[x], size_array[x]);
            XFREE(s);
            oco++;
            count_array[x] = 0;
            size_array[x] = 0;
        }
    }
    notify(player, "-------------------------------------------------------------------------------");

    notify(player, "Modified Active Cache              Dbref                   Attrs           Size");
    notify(player, "---------------------------------- -------------- -------------- --------------");

    for (x = 0, sp = sys_c; x < cwidth; x++, sp++)
    {
        for (cp = sp->head; cp != NULL; cp = cp->nxt)
        {
            if (cp->data && (cp->type == DBTYPE_ATTRIBUTE) && (cp->flags & CACHE_DIRTY))
            {
                maco++;
                msize += cp->datalen;
                count_array[((UDB_ANAME *)cp->keydata)->object] += 1;
                size_array[((UDB_ANAME *)cp->keydata)->object] += cp->datalen;
            }
        }
    }

    for (x = 0; x < mushstate.db_top; x++)
    {
        if (count_array[x] > 0)
        {
            s = strip_ansi(Name(x));
            raw_notify(player, "%-34.34s #%-13d %14d %14d", s, x, count_array[x], size_array[x]);
            XFREE(s);
            moco++;
        }
    }
    notify(player, "-------------------------------------------------------------------------------");

    raw_notify(player, "Active Cache:   %22d  Active Attribute Cache:   %13d", oco, aco);
    raw_notify(player, "Modified Cache: %22d  Modified Attribute Cache: %13d", moco, maco);
    raw_notify(player, "                                        Total Attribute Cache:    %13d", aco + maco);
    raw_notify(player, "Active Cache Size: %13d bytes  Modified Cache Size: %12d bytes", asize, msize);
    notify(player, "-------------------------------------------------------------------------------");
    XFREE(count_array);
    XFREE(size_array);
}

void list_cached_attrs(dbref player)
{
    UDB_CHAIN *sp;
    UDB_CACHE *cp;
    int x;
    int aco, maco, asize, msize;
    ATTR *atr;
    aco = maco = asize = msize = 0;
    notify(player, "Active Cache                  Attribute                         Dbref      Size");
    notify(player, "----------------------------- ---------------------------- ---------- ---------");

    for (x = 0, sp = sys_c; x < cwidth; x++, sp++)
    {
        for (cp = sp->head; cp != NULL; cp = cp->nxt)
        {
            if (cp->data && (cp->type == DBTYPE_ATTRIBUTE) && !(cp->flags & CACHE_DIRTY))
            {
                aco++;
                asize += cp->datalen;
                atr = atr_num(((UDB_ANAME *)cp->keydata)->attrnum);
                raw_notify(player, "%-29.29s %-28.28s #%9d %9d", PureName(((UDB_ANAME *)cp->keydata)->object), (atr ? atr->name : "(Unknown)"), ((UDB_ANAME *)cp->keydata)->object, cp->datalen);
            }
        }
    }

    notify(player, "-------------------------------------------------------------------------------");
    notify(player, "Modified Active Cache         Attribute                         Dbref      Size");
    notify(player, "----------------------------- ---------------------------- ---------- ---------");

    for (x = 0, sp = sys_c; x < cwidth; x++, sp++)
    {
        for (cp = sp->head; cp != NULL; cp = cp->nxt)
        {
            if (cp->data && (cp->type == DBTYPE_ATTRIBUTE) && (cp->flags & CACHE_DIRTY))
            {
                maco++;
                msize += cp->datalen;
                atr = atr_num(((UDB_ANAME *)cp->keydata)->attrnum);
                raw_notify(player, "%-29.29s %-28.28s #%9d %9d", PureName(((UDB_ANAME *)cp->keydata)->object), (atr ? atr->name : "(Unknown)"), ((UDB_ANAME *)cp->keydata)->object, cp->datalen);
            }
        }
    }

    notify(player, "-------------------------------------------------------------------------------");
    raw_notify(player, "Active Attribute Cache:  %13d  Modified Attribute Cache: %13d", aco, maco);
    raw_notify(player, "                                        Total Attribute Cache:    %13d", aco + maco);
    raw_notify(player, "Active Cache Size: %13d bytes  Modified Cache Size: %12d bytes", asize, msize);
    notify(player, "-------------------------------------------------------------------------------");
}

/* Search the cache for an entry of a specific type, if found, copy the data
 * and length into pointers provided by the caller, if not, fetch from DB.
 * You do not need to free data returned by this call. */

UDB_DATA cache_get(UDB_DATA key, unsigned int type)
{
    UDB_CACHE *cp;
    UDB_CHAIN *sp;
    int hv = 0;
    UDB_DATA data;

    if (!key.dptr || !cache_initted)
    {
        data.dptr = NULL;
        data.dsize = 0;
        return data;
    }

    /*
     * If we're dumping, ignore stats - activity during a dump skews the
     * * working set. We make sure in get_free_entry that any activity
     * * resulting from a dump does not push out entries that are already
     * * in the cache
     */

    if (!mushstate.standalone && !mushstate.dumping)
    {
        cs_reads++;
    }

    hv = cachehash(key.dptr, key.dsize, type);
    sp = &sys_c[hv];

    for (cp = sp->head; cp != NULL; cp = cp->nxt)
    {
        if ((type == cp->type) && !memcmp(key.dptr, cp->keydata, key.dsize))
        {
            if (!mushstate.standalone && !mushstate.dumping)
            {
                cs_rhits++;
                cs_ahits++;
            }

            if (cp->nxtfree == (UDB_CACHE *)0)
            {
                if (cp->prvfree != (UDB_CACHE *)0)
                {
                    cp->prvfree->nxtfree = (UDB_CACHE *)0;
                }
                freelist->tail = cp->prvfree;
            }
            if (cp->prvfree == (UDB_CACHE *)0)
            {
                freelist->head = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = (UDB_CACHE *)0;
            }
            else
            {
                cp->prvfree->nxtfree = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = cp->prvfree;
            }
            if (freelist->head == (UDB_CACHE *)0)
            {
                freelist->head = cp;
            }
            else
            {
                freelist->tail->nxtfree = cp;
            }
            cp->prvfree = freelist->tail;
            freelist->tail = cp;
            cp->nxtfree = (UDB_CACHE *)0;
            data.dptr = cp->data;
            data.dsize = cp->datalen;
            return data;
        }
    }


    /*
     * DARN IT - at this point we have a certified, type-A cache miss
     */

    /*
     * Grab the data from wherever
     */

    switch (type)
    {
    case DBTYPE_ATTRIBUTE:

        data.dptr = (void *)pipe_get_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object);

        if (data.dptr == NULL)
        {
            data.dsize = 0;
        }
        else
        {
            data.dsize = strlen(data.dptr) + 1;
        }

        break;

    default:
        data = db_get(key, type);
    }

    if (!mushstate.standalone && !mushstate.dumping)
    {
        cs_dbreads++;
    }

    if (data.dptr == NULL)
    {
        return data;
    }

    if ((cp = get_free_entry(data.dsize)) == NULL)
    {
        return data;
    }

    cp->keydata = (void *)XMALLOC(key.dsize, "cp->keydata");
    XMEMCPY(cp->keydata, key.dptr, key.dsize);
    cp->keylen = key.dsize;
    cp->data = data.dptr;
    cp->datalen = data.dsize;
    cp->type = type;
    cp->flags = 0;
    /*
     * If we're dumping, we'll put everything we fetch that is not
     * already in cache at the end of the chain and set its last
     * referenced time to zero. This will ensure that we won't blow away
     * what's already in cache, since get_free_entry will just reuse
     * these entries.
     */
    cs_size += cp->datalen;

    if (mushstate.dumping)
    {
        /*
	 * Link at head of chain
	 */
        if (sp->head == (UDB_CACHE *)0)
        {
            sp->tail = cp;
            cp->nxt = (UDB_CACHE *)0;
        }
        else
        {
            cp->nxt = sp->head;
        }
        sp->head = cp;
        /*
	 * Link at head of LRU freelist
	 */
        if (freelist->head == (UDB_CACHE *)0)
        {
            freelist->tail = cp;
            cp->nxtfree = (UDB_CACHE *)0;
        }
        else
        {
            cp->nxtfree = freelist->head;
            cp->nxtfree->prvfree = cp;
        }
        cp->prvfree = (UDB_CACHE *)0;
        freelist->head = cp;
    }
    else
    {
        /*
	 * Link at tail of chain
	 */
        if (sp->head == (UDB_CACHE *)0)
        {
            sp->head = cp;
        }
        else
        {
            sp->tail->nxt = cp;
        }
        sp->tail = cp;
        cp->nxt = (UDB_CACHE *)0;
        /*
	 * Link at tail of LRU freelist
	 */
        if (freelist->head == (UDB_CACHE *)0)
        {
            freelist->head = cp;
        }
        else
        {
            freelist->tail->nxtfree = cp;
        }
        cp->prvfree = freelist->tail;
        freelist->tail = cp;
        cp->nxtfree = (UDB_CACHE *)0;
    }

    return data;
}

/*
 * put an attribute back into the cache. this is complicated by the
 * fact that when a function calls this with an object, the object
 * is *already* in the cache, since calling functions operate on
 * pointers to the cached objects, and may or may not be actively
 * modifying them. in other words, by the time the object is handed
 * to cache_put, it has probably already been modified, and the cached
 * version probably already reflects those modifications!
 *
 * so - we do a couple of things: we make sure that the cached object is
 * actually there, and move it to the modified chain. if we can't find it -
 * either we have a (major) programming error, or the
 * *name* of the object has been changed, or the object is a totally
 * new creation someone made and is inserting into the world.
 *
 * in the case of totally new creations, we simply accept the pointer
 * to the object and add it into our environment. freeing it becomes
 * the responsibility of the cache code. DO NOT HAND A POINTER TO
 * CACHE_PUT AND THEN FREE IT YOURSELF!!!!
 *
 */

int cache_put(UDB_DATA key, UDB_DATA data, unsigned int type)
{
    UDB_CACHE *cp;
    UDB_CHAIN *sp;
    int hv = 0;

    if (!key.dptr || !data.dptr || !cache_initted)
    {
        return (1);
    }

    /*
     * Call module API hook
     */

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        if (cam__mp->cache_put_notify)
        {
            (*(cam__mp->cache_put_notify))(key, type);
        }
    }

    if (mushstate.standalone)
    {


        /*
	 * Bypass the cache when standalone or memory based for writes
	 */
        if (data.dptr == NULL)
        {
            switch (type)
            {
            case DBTYPE_ATTRIBUTE:
                pipe_del_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object);
                break;

            default:
                db_lock();
                db_del(key, type);
                db_unlock();
            }
        }
        else
        {
            switch (type)
            {
            case DBTYPE_ATTRIBUTE:
                pipe_set_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object, (char *)data.dptr);

                /*
		 * Don't forget to free data.dptr when standalone
		 */
                XFREE(data.dptr);
                break;

            default:
                db_lock();
                db_put(key, data, type);
                db_unlock();
            }
        }

        return (0);
    }
    cs_writes++;
    /*
     * generate hash
     */
    hv = cachehash(key.dptr, key.dsize, type);
    sp = &sys_c[hv];

    /*
     * step one, search chain, and if we find the obj, dirty it
     */

    for (cp = sp->head; cp != NULL; cp = cp->nxt)
    {
        if ((type == cp->type) && !memcmp(key.dptr, cp->keydata, key.dsize))
        {
            if (!mushstate.dumping)
            {
                cs_whits++;
            }

            if (cp->data != data.dptr)
            {
                cache_repl(cp, data.dptr, data.dsize, type, CACHE_DIRTY);
            }

            if (cp->nxtfree == (UDB_CACHE *)0)
            {
                if (cp->prvfree != (UDB_CACHE *)0)
                {
                    cp->prvfree->nxtfree = (UDB_CACHE *)0;
                }
                freelist->tail = cp->prvfree;
            }
            if (cp->prvfree == (UDB_CACHE *)0)
            {
                freelist->head = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = (UDB_CACHE *)0;
            }
            else
            {
                cp->prvfree->nxtfree = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = cp->prvfree;
            }
            if (freelist->head == (UDB_CACHE *)0)
            {
                freelist->head = cp;
            }
            else
            {
                freelist->tail->nxtfree = cp;
            }
            cp->prvfree = freelist->tail;
            freelist->tail = cp;
            cp->nxtfree = (UDB_CACHE *)0;
            return (0);
        }
    }

    /*
     * Add a new entry to the cache
     */

    if ((cp = get_free_entry(data.dsize)) == NULL)
    {
        return (1);
    }

    cp->keydata = (void *)XMALLOC(key.dsize, "cp->keydata");
    XMEMCPY(cp->keydata, key.dptr, key.dsize);
    cp->keylen = key.dsize;
    cp->data = data.dptr;
    cp->datalen = data.dsize;
    cp->type = type;
    cp->flags = CACHE_DIRTY;
    cs_size += cp->datalen;
    /*
     * link at tail of chain
     */
    if (sp->head == (UDB_CACHE *)0)
    {
        sp->head = cp;
    }
    else
    {
        sp->tail->nxt = cp;
    }
    sp->tail = cp;
    cp->nxt = (UDB_CACHE *)0;
    /*
     * link at tail of LRU freelist
     */
    if (freelist->head == (UDB_CACHE *)0)
    {
        freelist->head = cp;
    }
    else
    {
        freelist->tail->nxtfree = cp;
    }
    cp->prvfree = freelist->tail;
    freelist->tail = cp;
    cp->nxtfree = (UDB_CACHE *)0;
    return (0);
}

UDB_CACHE *get_free_entry(int atrsize)
{
    UDB_DATA key, data;
    UDB_CHAIN *sp;
    UDB_CACHE *cp, *p, *prv;
    int hv;

    /*
     * Flush entries from the cache until there's enough room for
     * * this one. The max size can be dynamically changed-- if it is too
     * * small, the MUSH will flush entries until the cache fits within
     * * this size and if it is too large, we'll fill it up before we
     * * start flushing
     */

    while ((cs_size + atrsize) > (mushconf.cache_size ? mushconf.cache_size : CACHE_SIZE))
    {
        cp = freelist->head;

        if (cp)
        {
            if (cp->nxtfree == (UDB_CACHE *)0)
            {
                if (cp->prvfree != (UDB_CACHE *)0)
                {
                    cp->prvfree->nxtfree = (UDB_CACHE *)0;
                }
                freelist->tail = cp->prvfree;
            }
            if (cp->prvfree == (UDB_CACHE *)0)
            {
                freelist->head = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = (UDB_CACHE *)0;
            }
            else
            {
                cp->prvfree->nxtfree = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = cp->prvfree;
            }
        }

        if (cp && (cp->flags & CACHE_DIRTY))
        {
            /*
	     * Flush the modified attributes to disk
	     */
            if (cp->data == NULL)
            {
                switch (cp->type)
                {
                case DBTYPE_ATTRIBUTE:
                    pipe_del_attrib(((UDB_ANAME *)cp->keydata)->attrnum, ((UDB_ANAME *)cp->keydata)->object);
                    break;

                default:
                    key.dptr = cp->keydata;
                    key.dsize = cp->keylen;
                    db_lock();
                    db_del(key, cp->type);
                    db_unlock();
                }

                cs_dels++;
            }
            else
            {
                switch (cp->type)
                {
                case DBTYPE_ATTRIBUTE:
                    pipe_set_attrib(((UDB_ANAME *)cp->keydata)->attrnum, ((UDB_ANAME *)cp->keydata)->object, (char *)cp->data);
                    break;

                default:
                    key.dptr = cp->keydata;
                    key.dsize = cp->keylen;
                    data.dptr = cp->data;
                    data.dsize = cp->datalen;
                    db_lock();
                    db_put(key, data, cp->type);
                    db_unlock();
                }

                cs_dbwrites++;
            }
        }

        /*
	 * Take the attribute off of its chain and nuke the
	 * attribute's memory
	 */

        if (cp)
        {
            /*
	     * Find the cache entry inside the real cache
	     */
            hv = cachehash(cp->keydata, cp->keylen, cp->type);
            sp = &sys_c[hv];
            prv = NULL;

            for (p = sp->head; p != NULL; p = p->nxt)
            {
                if (cp == p)
                {
                    break;
                }

                prv = p;
            }

            /*
	     * Remove the cache entry
	     */
            if (cp->nxt == (UDB_CACHE *)0)
            {
                if (prv != (UDB_CACHE *)0)
                {
                    prv->nxt = (UDB_CACHE *)0;
                }
                sp->tail = prv;
            }
            if (prv == (UDB_CACHE *)0)
            {
                sp->head = cp->nxt;
            }
            else
            {
                prv->nxt = cp->nxt;
            }

            cache_repl(cp, NULL, 0, DBTYPE_EMPTY, 0);
            XFREE(cp->keydata);
            XFREE(cp);
        }

        cp = NULL;
    }

    /*
     * No valid cache entries to flush, allocate a new one
     */

    if ((cp = (UDB_CACHE *)XMALLOC(sizeof(UDB_CACHE), "cp")) == NULL)
    {
        fatal("cache get_free_entry: XMALLOC failed", (char *)-1, (char *)0);
    }

    cp->keydata = NULL;
    cp->keylen = 0;
    cp->data = NULL;
    cp->datalen = 0;
    cp->type = DBTYPE_EMPTY;
    cp->flags = 0;
    return (cp);
}

int cache_write(UDB_CACHE *cp)
{
    UDB_DATA key, data;

    /*
     * Write a single cache chain to disk
     */

    while (cp != NULL)
    {
        if (!(cp->flags & CACHE_DIRTY))
        {
            cp = cp->nxt;
            continue;
        }

        if (cp->data == NULL)
        {
            switch (cp->type)
            {
            case DBTYPE_ATTRIBUTE:
                pipe_del_attrib(((UDB_ANAME *)cp->keydata)->attrnum, ((UDB_ANAME *)cp->keydata)->object);
                break;

            default:
                key.dptr = cp->keydata;
                key.dsize = cp->keylen;
                db_del(key, cp->type);
            }

            cs_dels++;
        }
        else
        {
            switch (cp->type)
            {
            case DBTYPE_ATTRIBUTE:
                pipe_set_attrib(((UDB_ANAME *)cp->keydata)->attrnum, ((UDB_ANAME *)cp->keydata)->object, (char *)cp->data);
                break;

            default:
                key.dptr = cp->keydata;
                key.dsize = cp->keylen;
                data.dptr = cp->data;
                data.dsize = cp->datalen;
                db_put(key, data, cp->type);
            }

            cs_dbwrites++;
        }

        cp->flags = 0;
        cp = cp->nxt;
    }

    return (0);
}

int cache_sync(void)
{
    int x;
    UDB_CHAIN *sp;
    cs_syncs++;

    if (!cache_initted)
    {
        return (1);
    }

    if (cache_frozen)
    {
        return (0);
    }

    if (mushstate.standalone || mushstate.restarting)
    {
        /*
	 * If we're restarting or standalone, having DBM wait for
	 * * each write is a performance no-no; run asynchronously
	 */
        dddb_setsync(0);
    }

    /*
     * Lock the database
     */
    db_lock();

    for (x = 0, sp = sys_c; x < cwidth; x++, sp++)
    {
        if (cache_write(sp->head))
        {
            return (1);
        }
    }

    /*
     * Also sync the read and write object structures if they're dirty
     */
    attrib_sync();
    /*
     * Unlock the database
     */
    db_unlock();

    if (mushstate.standalone || mushstate.restarting)
    {
        dddb_setsync(1);
    }

    return (0);
}

void cache_del(UDB_DATA key, unsigned int type)
{
    UDB_CACHE *cp;
    UDB_CHAIN *sp;
    int hv = 0;

    if (!key.dptr || !cache_initted)
    {
        return;
    }

    /*
     * Call module API hook
     */

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        if (cam__mp->cache_del_notify)
        {
            (*(cam__mp->cache_del_notify))(key, type);
        }
    }


    cs_dels++;
    hv = cachehash(key.dptr, key.dsize, type);
    sp = &sys_c[hv];

    /*
     * mark dead in cache
     */

    for (cp = sp->head; cp != NULL; cp = cp->nxt)
    {
        if ((type == cp->type) && !memcmp(key.dptr, cp->keydata, key.dsize))
        {
            if (cp->nxtfree == (UDB_CACHE *)0)
            {
                if (cp->prvfree != (UDB_CACHE *)0)
                {
                    cp->prvfree->nxtfree = (UDB_CACHE *)0;
                }
                freelist->tail = cp->prvfree;
            }
            if (cp->prvfree == (UDB_CACHE *)0)
            {
                freelist->head = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = (UDB_CACHE *)0;
            }
            else
            {
                cp->prvfree->nxtfree = cp->nxtfree;
                if (cp->nxtfree)
                    cp->nxtfree->prvfree = cp->prvfree;
            }

            if (freelist->head == (UDB_CACHE *)0)
            {
                freelist->tail = cp;
                cp->nxtfree = (UDB_CACHE *)0;
            }
            else
            {
                cp->nxtfree = freelist->head;
                cp->nxtfree->prvfree = cp;
            }
            cp->prvfree = (UDB_CACHE *)0;
            freelist->head = cp;
            cache_repl(cp, NULL, 0, type, CACHE_DIRTY);
            return;
        }
    }

    if ((cp = get_free_entry(0)) == NULL)
    {
        return;
    }

    cp->keydata = (void *)XMALLOC(key.dsize, "cp->keydata");
    XMEMCPY(cp->keydata, key.dptr, key.dsize);
    cp->keylen = key.dsize;
    cp->type = type;
    cp->flags = CACHE_DIRTY;
    if (sp->head == (UDB_CACHE *)0)
    {
        sp->tail = cp;
        cp->nxt = (UDB_CACHE *)0;
    }
    else
    {
        cp->nxt = sp->head;
    }
    sp->head = cp;
    if (freelist->head == (UDB_CACHE *)0)
    {
        freelist->tail = cp;
        cp->nxtfree = (UDB_CACHE *)0;
    }
    else
    {
        cp->nxtfree = freelist->head;
        cp->nxtfree->prvfree = cp;
    }
    cp->prvfree = (UDB_CACHE *)0;
    freelist->head = cp;
    return;
}
