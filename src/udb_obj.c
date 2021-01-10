
/**
 * @file udb_obj.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Binary object handling gear.
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 * 
 * @note Why not just write the attributes individually to disk? Well, when 
 *       you're running on a platform that does synchronous writes with a large
 *       database, thousands of I/O operations tend to be expensive. When you
 *       'coalesce' many attribute writes onto a single object and do only one
 *       I/O operation, you can get an order of magnitude speed difference,
 *       especially on loading/unloading to flatfile. It also has the side
 *       effect of pre-fetching on reads, since you often have sequential
 *       attribute reads off of the same object.
 * 
 *       Wile all of this is extremely true, keep in mind that text was written
 *       over 25 years ago. Even if we didn't optimized our read/write sequence
 *       you woudn't see the difference.
 * 
 *       If you really want to see the difference, it's time to get youself into
 *       the retro-computing scene :)
 */

#include "system.h"

#include "defaults.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

/* Take a chunk of data which contains an object, and parse it into an
 * object structure. */

UDB_OBJECT *unroll_obj(char *data)
{
    int i, j;
    UDB_OBJECT *o;
    UDB_ATTRIB *a;
    char *dptr;
    dptr = data;

    /*
     * Get a new Obj struct
     */

    if ((o = (UDB_OBJECT *)XMALLOC(sizeof(UDB_OBJECT), "o")) == NULL)
    {
        return (NULL);
    }

    /*
     * Read in the header
     */

    if (XMEMCPY((void *)&(o->name), (void *)dptr, sizeof(unsigned int)) == NULL)
    {
        XFREE(o);
        return (NULL);
    }

    dptr += sizeof(unsigned int);

    if (XMEMCPY((void *)&i, (void *)dptr, sizeof(int)) == NULL)
    {
        XFREE(o);
        return (NULL);
    }

    dptr += sizeof(int);
    o->at_count = i;
    /*
     * Now get an array of Attrs
     */
    a = o->atrs = (UDB_ATTRIB *)XMALLOC(i * sizeof(UDB_ATTRIB), "o->atrs");

    if (!o->atrs)
    {
        XFREE(o);
        return (NULL);
    }

    /*
     * Now go get the attrs, one at a time.
     */

    for (j = 0; j < i;)
    {
        /*
	 * Attribute size
	 */
        if (XMEMCPY((void *)&(a[j].size), (void *)dptr, sizeof(int)) == NULL)
        {
            goto bail;
        }

        dptr += sizeof(int);

        /*
	 * Attribute number
	 */

        if (XMEMCPY((void *)&(a[j].attrnum), (void *)dptr, sizeof(int)) == NULL)
        {
            goto bail;
        }

        dptr += sizeof(int);

        /*
	 * get some memory for the data
	 */

        if ((a[j].data = (char *)XMALLOC(a[j].size, "a[j].data")) == NULL)
        {
            goto bail;
        }

        /*
	 * Preincrement j, so we know how many to free if this next
	 * * bit fails.
	 */
        j++;

        /*
	 * Now get the data
	 */

        if (XMEMCPY((void *)a[j - 1].data, (void *)dptr, a[j - 1].size) == NULL)
        {
            goto bail;
        }

        dptr += a[j - 1].size;
    }

    /*
     * Should be all done...
     */
    o->dirty = 0;
    return (o);
    /*
     * Oh shit. We gotta free up all these little bits of memory.
     */
bail:
    /*
     * j points one attribute *beyond* what we need to free up
     */

    for (i = 0; i < j; i++)
    {
        XFREE(a[i].data);
    }

    XFREE(a);
    XFREE(o);
    return (NULL);
}

/* Rollup an object structure into a single buffer for a write to disk. */

char *rollup_obj(UDB_OBJECT *o)
{
    int i;
    UDB_ATTRIB *a;
    char *dptr, *data;
    dptr = data = (char *)XMALLOC(obj_siz(o), "data");
    /*
     * Mark the object as clean
     */
    o->dirty = 0;

    /*
     * Write out the object header
     */

    if (XMEMCPY((void *)dptr, (void *)&(o->name), sizeof(unsigned int)) == NULL)
    {
        return NULL;
    }

    dptr += sizeof(unsigned int);

    if (XMEMCPY((void *)dptr, (void *)&(o->at_count), sizeof(int)) == NULL)
    {
        return NULL;
    }

    dptr += sizeof(int);
    /*
     * Now do the attributes, one at a time.
     */
    a = o->atrs;

    for (i = 0; i < o->at_count; i++)
    {
        /*
	 * Attribute size.
	 */
        if (XMEMCPY((void *)dptr, (void *)&(a[i].size), sizeof(int)) == NULL)
        {
            return NULL;
        }

        dptr += sizeof(int);

        /*
	 * Attribute number
	 */

        if (XMEMCPY((void *)dptr, (void *)&(a[i].attrnum), sizeof(int)) == NULL)
        {
            return NULL;
        }

        dptr += sizeof(int);

        /*
	 * Attribute data
	 */

        if (XMEMCPY((void *)dptr, (void *)a[i].data, a[i].size) == NULL)
        {
            return NULL;
        }

        dptr += a[i].size;
    }

    /*
     * That's all she wrote.
     */
    return data;
}

/* Return the size, on disk, the thing is going to take up.*/

int obj_siz(UDB_OBJECT *o)
{
    int i;
    int siz;
    siz = OBJ_HEADER_SIZE;

    for (i = 0; i < o->at_count; i++)
    {
        siz += (((o->atrs)[i]).size + ATTR_HEADER_SIZE);
    }

    return (siz);
}

/* And something to free all the goo on an Obj, as well as the Obj.*/

void objfree(UDB_OBJECT *o)
{
    int i;
    UDB_ATTRIB *a;

    if (!o->atrs)
    {
        XFREE(o);
        return;
    }

    a = o->atrs;

    for (i = 0; i < o->at_count; i++)
    {
        XFREE(a[i].data);
    }

    XFREE(a);
    XFREE(o);
}

/* Routines to manipulate attributes within the object structure */

char *obj_get_attrib(int anam, UDB_OBJECT *obj)
{
    int lo, mid, hi;
    UDB_ATTRIB *a;
    /*
     * Binary search for the attribute
     */
    lo = 0;
    hi = obj->at_count - 1;
    a = obj->atrs;

    while (lo <= hi)
    {
        mid = ((hi - lo) >> 1) + lo;

        if (a[mid].attrnum == anam)
        {
            return (char *)a[mid].data;
            break;
        }
        else if (a[mid].attrnum > anam)
        {
            hi = mid - 1;
        }
        else
        {
            lo = mid + 1;
        }
    }

    return (NULL);
}

void obj_set_attrib(int anam, UDB_OBJECT *obj, char *value)
{
    int hi, lo, mid;
    UDB_ATTRIB *a;

    /*
     * Demands made elsewhere insist that we cope with the case of an
     * * empty object.
     */

    if (obj->atrs == NULL)
    {
        a = (UDB_ATTRIB *)XMALLOC(sizeof(UDB_ATTRIB), "a");
        obj->atrs = a;
        obj->at_count = 1;
        a[0].attrnum = anam;
        a[0].data = (char *)value;
        a[0].size = strlen(value) + 1;
        return;
    }

    /*
     * Binary search for the attribute.
     */
    lo = 0;
    hi = obj->at_count - 1;
    a = obj->atrs;

    while (lo <= hi)
    {
        mid = ((hi - lo) >> 1) + lo;

        if (a[mid].attrnum == anam)
        {
            XFREE(a[mid].data);
            a[mid].data = (char *)value;
            a[mid].size = strlen(value) + 1;
            return;
        }
        else if (a[mid].attrnum > anam)
        {
            hi = mid - 1;
        }
        else
        {
            lo = mid + 1;
        }
    }

    /*
     * If we got here, we didn't find it, so lo = hi + 1, and the
     * * attribute should be inserted between them.
     */
    a = (UDB_ATTRIB *)XREALLOC(obj->atrs, (obj->at_count + 1) * sizeof(UDB_ATTRIB), "a");

    /*
     * Move the stuff upwards one slot.
     */

    if (lo < obj->at_count)
        XMEMMOVE((void *)(a + lo + 1), (void *)(a + lo), (obj->at_count - lo) * sizeof(UDB_ATTRIB));

    a[lo].data = value;
    a[lo].attrnum = anam;
    a[lo].size = strlen(value) + 1;
    obj->at_count++;
    obj->atrs = a;
}

void obj_del_attrib(int anam, UDB_OBJECT *obj)
{
    int hi, lo, mid;
    UDB_ATTRIB *a;

    if (!obj->at_count || !obj->atrs)
    {
        return;
    }

    if (obj->at_count < 0)
    {
        abort();
    }

    /*
     * Binary search for the attribute.
     */
    lo = 0;
    hi = obj->at_count - 1;
    a = obj->atrs;

    while (lo <= hi)
    {
        mid = ((hi - lo) >> 1) + lo;

        if (a[mid].attrnum == anam)
        {
            XFREE(a[mid].data);
            obj->at_count--;

            if (mid != obj->at_count)
                XMEMCPY((void *)(a + mid), (void *)(a + mid + 1), (obj->at_count - mid) * sizeof(UDB_ATTRIB));

            if (obj->at_count == 0)
            {
                XFREE(obj->atrs);
                obj->atrs = NULL;
            }

            return;
        }
        else if (a[mid].attrnum > anam)
        {
            hi = mid - 1;
        }
        else
        {
            lo = mid + 1;
        }
    }
}

/* Now let's define the functions that the cache actually uses. These
 * 'pipeline' attribute writes using a pair of object structures. When
 * the object which is being read or written to changes, we have to
 * dump the current object, and bring in the new one. */

/* get_free_objpipe: return an object pipeline */

UDB_OBJECT *get_free_objpipe(unsigned int obj)
{
    UDB_DATA key, data;
    int i, j = 0;

    /*
     * Try to see if it's already in a pipeline first
     */

    for (i = 0; i < NUM_OBJPIPES; i++)
    {
        if (mudstate.objpipes[i] && mudstate.objpipes[i]->name == obj)
        {
            mudstate.objpipes[i]->counter = mudstate.objc;
            mudstate.objc++;
            return mudstate.objpipes[i];
        }
    }

    /*
     * Look for an empty pipeline
     */

    for (i = 0; i < NUM_OBJPIPES; i++)
    {
        if (!mudstate.objpipes[i])
        {
            /*
	     * If there's no object there, read one in
	     */
            key.dptr = &obj;
            key.dsize = sizeof(int);
            data = db_get(key, DBTYPE_ATTRIBUTE);

            if (data.dptr)
            {
                mudstate.objpipes[i] = unroll_obj(data.dptr);
                XFREE(data.dptr);
            }
            else
            {
                /*
		 * New object
		 */
                if ((mudstate.objpipes[i] = (UDB_OBJECT *)XMALLOC(sizeof(UDB_OBJECT), "mudstate.objpipes[i]")) == NULL)
                {
                    return (NULL);
                }

                mudstate.objpipes[i]->name = obj;
                mudstate.objpipes[i]->at_count = 0;
                mudstate.objpipes[i]->dirty = 0;
                mudstate.objpipes[i]->atrs = NULL;
            }

            mudstate.objpipes[i]->counter = mudstate.objc;
            mudstate.objc++;
            return mudstate.objpipes[i];
        }

        if (mudstate.objpipes[i]->counter < mudstate.objpipes[j]->counter)
        {
            j = i;
        }
    }

    /*
     * We got here, so we have to push the oldest object out. If it's
     * * dirty, write it first
     */

    if (mudstate.objpipes[j]->dirty)
    {
        data.dptr = rollup_obj(mudstate.objpipes[j]);
        data.dsize = obj_siz(mudstate.objpipes[j]);
        key.dptr = &mudstate.objpipes[j]->name;
        key.dsize = sizeof(int);
        db_lock();
        db_put(key, data, DBTYPE_ATTRIBUTE);
        db_unlock();
        XFREE(data.dptr);
    }

    objfree(mudstate.objpipes[j]);
    /*
     * Bring in the object from disk if it exists there
     */
    key.dptr = &obj;
    key.dsize = sizeof(int);
    data = db_get(key, DBTYPE_ATTRIBUTE);

    if (data.dptr)
    {
        mudstate.objpipes[j] = unroll_obj(data.dptr);
        XFREE(data.dptr);

        if (mudstate.objpipes[j] == NULL)
        {
            log_write(LOG_PROBLEMS, "ERR", "CACHE", "Null returned on unroll of object #%d", j);
            return (NULL);
        }
    }
    else
    {
        /*
	 * New object
	 */
        if ((mudstate.objpipes[j] = (UDB_OBJECT *)XMALLOC(sizeof(UDB_OBJECT), "mudstate.objpipes[j]")) == NULL)
        {
            return (NULL);
        }

        mudstate.objpipes[j]->name = obj;
        mudstate.objpipes[j]->at_count = 0;
        mudstate.objpipes[j]->dirty = 0;
        mudstate.objpipes[j]->atrs = NULL;
    }

    mudstate.objpipes[j]->counter = mudstate.objc;
    mudstate.objc++;
    return mudstate.objpipes[j];
}

char *pipe_get_attrib(int anum, unsigned int obj)
{
    UDB_OBJECT *object;
    char *value, *tmp;
    object = get_free_objpipe(obj);
    value = obj_get_attrib(anum, object);

    if (value)
    {
        tmp = XSTRDUP(value, "tmp");
        return tmp;
    }
    else
    {
        return NULL;
    }
}

void pipe_set_attrib(int anum, unsigned int obj, char *value)
{
    UDB_OBJECT *object;
    char *newvalue;
    /*
     * Write the damn thing
     */
    object = get_free_objpipe(obj);
    object->dirty = 1;
    newvalue = XSTRDUP(value, "newvalue");
    obj_set_attrib(anum, object, newvalue);
    return;
}

void pipe_del_attrib(int anum, unsigned int obj)
{
    UDB_OBJECT *object;
    /*
     * Write the damn thing
     */
    object = get_free_objpipe(obj);
    object->dirty = 1;
    obj_del_attrib(anum, object);
    return;
}

void attrib_sync(void)
{
    UDB_DATA key, data;
    int i;

    /*
     * Make sure dirty objects are written to disk
     */

    for (i = 0; i < NUM_OBJPIPES; i++)
    {
        if (mudstate.objpipes[i] && mudstate.objpipes[i]->dirty)
        {
            data.dptr = rollup_obj(mudstate.objpipes[i]);
            data.dsize = obj_siz(mudstate.objpipes[i]);
            key.dptr = &mudstate.objpipes[i]->name;
            key.dsize = sizeof(int);
            db_put(key, data, DBTYPE_ATTRIBUTE);
            XFREE(data.dptr);
            mudstate.objpipes[i]->dirty = 0;
        }
    }
}
