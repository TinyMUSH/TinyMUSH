/**
 * @file udb_ocache.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Thin pass-through wrappers for database access (cache removed)
 * @version 4.0
 *
 * Copyright (C) 1989-2025 TinyMUSH development team.
 * You may distribute under the terms of the Artistic License, as specified in the COPYING file.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "udb_backend.h"

#include <string.h>

/* Legacy globals kept for compatibility with the old cache API */
int cwidth = CACHE_WIDTH;
UDB_CHAIN *sys_c = NULL;
UDB_CHAIN *freelist = NULL;
int cache_initted = 0;
int cache_frozen = 0;

/* Cache stats (still exposed for diagnostics, but no longer populated meaningfully) */
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

static void cache_reset_counters(void)
{
    cs_writes = 0;
    cs_reads = 0;
    cs_dbreads = 0;
    cs_dbwrites = 0;
    cs_dels = 0;
    cs_checks = 0;
    cs_rhits = 0;
    cs_ahits = 0;
    cs_whits = 0;
    cs_fails = 0;
    cs_syncs = 0;
    cs_size = 0;
}

int cachehash(void *keydata, int keylen, unsigned int type)
{
    /* Retain the old hash implementation for ABI compatibility; now unused. */
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

    return ((hash + type) % (cwidth ? cwidth : 1));
}

void cache_repl(UDB_CACHE *cp, void *newdata, int len, unsigned int type, unsigned int flags)
{
    if (!cp)
    {
        return;
    }

    if (cp->data && cp->data != newdata)
    {
        XFREE(cp->data);
    }

    cp->data = newdata;
    cp->datalen = len;
    cp->type = type;
    cp->flags = flags;
}

int cache_init(int width)
{
    if (cache_initted)
    {
        return 0;
    }

    if (width)
    {
        cwidth = width;
    }

    cache_initted = 1;
    cache_reset_counters();
    cs_ltime = time(NULL);

    /* Initialise object pipelines used by attrib_sync(). */
    for (int i = 0; i < NUM_OBJPIPES; i++)
    {
        mushstate.objpipes[i] = NULL;
    }

    mushstate.objc = 0;
    return 0;
}

void cache_reset(void)
{
    cache_reset_counters();
    cache_initted = 1;
    attrib_sync();
}

void list_cached_objs(dbref player)
{
    notify(player, "Object cache disabled: backend is accessed directly.");
}

void list_cached_attrs(dbref player)
{
    notify(player, "Attribute cache disabled: backend is accessed directly.");
}

/* Fetch data directly from the backend. Caller owns the returned buffer. */
UDB_DATA cache_get(UDB_DATA key, unsigned int type)
{
    UDB_DATA data;
    data.dptr = NULL;
    data.dsize = 0;

    if (!key.dptr || !cache_initted)
    {
        return data;
    }

    if (!mushstate.standalone && !mushstate.dumping)
    {
        cs_reads++;
        cs_dbreads++;
    }

    switch (type)
    {
    case DBTYPE_ATTRIBUTE:
        data.dptr = pipe_get_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object);

        if (data.dptr)
        {
            data.dsize = (int)strlen(data.dptr) + 1;
        }
        break;

    default:
        data = db_get(key, type);
    }

    return data;
}

int cache_put(UDB_DATA key, UDB_DATA data, unsigned int type)
{
    if (!key.dptr || !cache_initted)
    {
        return 1;
    }

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        if (cam__mp->cache_put_notify)
        {
            cam__mp->cache_put_notify(key, type);
        }
    }

    cs_writes++;

    if (data.dptr == NULL)
    {
        cache_del(key, type);
        return 0;
    }

    if (mushstate.standalone)
    {
        if (type == DBTYPE_ATTRIBUTE)
        {
            pipe_set_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object, (char *)data.dptr);
            XFREE(data.dptr);
        }
        else
        {
            db_lock();
            db_put(key, data, type);
            db_unlock();
        }
        return 0;
    }

    switch (type)
    {
    case DBTYPE_ATTRIBUTE:
        db_lock();
        pipe_set_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object, (char *)data.dptr);
        attrib_sync();
        db_unlock();
        break;

    default:
        db_lock();
        db_put(key, data, type);
        db_unlock();
        cs_dbwrites++;
    }

    if ((type == DBTYPE_ATTRIBUTE) && data.dptr)
    {
        XFREE(data.dptr);
    }

    return 0;
}

UDB_CACHE *get_free_entry(int atrsize)
{
    (void)atrsize;
    UDB_CACHE *cp = (UDB_CACHE *)XCALLOC(1, sizeof(UDB_CACHE), "cp");
    return cp;
}

int cache_write(UDB_CACHE *cp)
{
    (void)cp;
    attrib_sync();
    return 0;
}

int cache_sync(void)
{
    cs_syncs++;

    if (!cache_initted)
    {
        return 1;
    }

    if (cache_frozen)
    {
        return 0;
    }

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

void cache_del(UDB_DATA key, unsigned int type)
{
    if (!key.dptr || !cache_initted)
    {
        return;
    }

    for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
    {
        if (cam__mp->cache_del_notify)
        {
            cam__mp->cache_del_notify(key, type);
        }
    }

    cs_dels++;

    if (mushstate.standalone)
    {
        if (type == DBTYPE_ATTRIBUTE)
        {
            pipe_del_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object);
        }
        else
        {
            db_del(key, type);
        }
        return;
    }

    switch (type)
    {
    case DBTYPE_ATTRIBUTE:
        db_lock();
        pipe_del_attrib(((UDB_ANAME *)key.dptr)->attrnum, ((UDB_ANAME *)key.dptr)->object);
        attrib_sync();
        db_unlock();
        break;

    default:
        db_lock();
        db_del(key, type);
        db_unlock();
    }
}

