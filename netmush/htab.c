/**
 * @file htab.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Table hashing routines
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

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

/* ---------------------------------------------------------------------------
 * hashval: Compute hash value of a string for a hash table.
 */

int hashval(char *str, int hashmask)
{
    unsigned int hash = 2166136261u; /* FNV-1a offset basis */
    char *sp;

    /*
     * If the string pointer is null, return 0.  If not, use FNV-1a hash
     * to avoid overflow issues with signed arithmetic.
     */

    if (str == NULL)
    {
        return 0;
    }

    for (sp = str; *sp; sp++)
    {
        hash ^= (unsigned char)(*sp);
        hash *= 16777619u; /* FNV prime */
    }

    return (hash & hashmask);
}

/* ----------------------------------------------------------------------
 * get_hashmask: Get hash mask for mask-style hashing.
 */

int get_hashmask(int *size)
{
    int tsize;

    /*
     * Get next power-of-two >= size, return power-1 as the mask for
     * ANDing. Ensure size doesn't overflow.
     */

    /* Ensure size is at least 1 to avoid undefined behavior */
    if (*size < 1)
    {
        *size = 1;
    }

    /* Check for overflow: cap at largest safe power of 2 */
    if (*size > (INT_MAX / 2))
    {
        *size = (INT_MAX / 2) + 1; /* Largest power of 2 that fits in int */
        return *size - 1;
    }

    /* Find next power-of-two >= size */
    for (tsize = 1; tsize < *size; tsize = tsize << 1)
        ;

    *size = tsize;
    return tsize - 1;
}

/* ---------------------------------------------------------------------------
 * hashinit: Initialize a new hash table.
 */

void hashinit(HASHTAB *htab, int size, int flags)
{
    htab->mask = get_hashmask(&size);
    htab->hashsize = size;
    htab->checks = 0;
    htab->scans = 0;
    htab->max_scan = 0;
    htab->hits = 0;
    htab->entries = 0;
    htab->deletes = 0;
    htab->nulls = size;

    if ((flags & HT_TYPEMASK) == HT_NUM)
    {
        /*
         * Numeric hashtabs implicitly store keys by reference
         */
        flags |= HT_KEYREF;
    }

    htab->flags = flags;
    htab->entry = (HASHENT **)XCALLOC(size, sizeof(HASHENT *), "htab->entry");

    if (htab->entry == NULL)
    {
        log_write(LOG_BUGS, "BUG", "HASH", "Failed to allocate hash table of size %d", size);
        /* Initialize with minimal state to prevent crashes */
        htab->hashsize = 0;
        htab->mask = 0;
        htab->entries = 0;
        htab->nulls = 0;
        htab->last_entry = NULL;
        htab->last_hval = 0;
        return;
    }

    /* XCALLOC already initializes all entries to NULL, no loop needed */
    /* Initialize iterator state */
    htab->last_entry = NULL;
    htab->last_hval = 0;
}

/* ---------------------------------------------------------------------------
 * hashreset: Reset hash table stats.
 */

void hashreset(HASHTAB *htab)
{
    if (htab == NULL)
    {
        return;
    }

    htab->checks = 0;
    htab->scans = 0;
    htab->max_scan = 0;
    htab->hits = 0;
    /* Note: entries and deletes are structural, not reset here */
}

/* ---------------------------------------------------------------------------
 * get_hash_value: Compute bucket index for a key in a hash table.
 * Handles both string and numeric key types.
 */

static inline int get_hash_value(HASHKEY key, HASHTAB *htab)
{
    /* This should never happen if callers are correct, but be defensive */
    if (htab == NULL)
    {
        return 0;
    }

    int htype = htab->flags & HT_TYPEMASK;

    if (htype == HT_STR)
    {
        return hashval(key.s, htab->mask);
    }
    else
    {
        return (key.i & htab->mask);
    }
}

static inline int key_matches(HASHKEY a, HASHKEY b, int htype)
{
    if (htype == HT_STR)
    {
        /* Guard against NULL pointers */
        if (a.s == NULL || b.s == NULL)
        {
            return (a.s == b.s);
        }
        return strcmp(a.s, b.s) == 0;
    }
    else
    {
        return a.i == b.i;
    }
}

/* ---------------------------------------------------------------------------
 * update_hash_stats: Safely update hash statistics with overflow protection.
 */

static inline void update_hash_stats(HASHTAB *htab, int numchecks, int is_hit)
{
    if (is_hit)
    {
        if (htab->hits < INT_MAX)
        {
            htab->hits++;
        }
    }

    if (numchecks > htab->max_scan)
    {
        htab->max_scan = numchecks;
    }

    /* Add with overflow check to prevent stat corruption */
    if (htab->checks < INT_MAX - numchecks)
    {
        htab->checks += numchecks;
    }
    else
    {
        /* Overflow detected; reset stats to prevent corruption */
        log_write(LOG_BUGS, "BUG", "HASH", "Hash statistics overflow detected, resetting stats");
        hashreset(htab);
    }
}

/* ---------------------------------------------------------------------------
 * increment_scans: Safely increment scan counter with overflow protection.
 */

static inline void increment_scans(HASHTAB *htab)
{
    if (htab->scans < INT_MAX)
    {
        htab->scans++;
    }
    else
    {
        /* Overflow detected; reset stats to prevent corruption */
        log_write(LOG_BUGS, "BUG", "HASH", "Hash scans counter overflow detected, resetting stats");
        hashreset(htab);
    }
}

/* ---------------------------------------------------------------------------
 * hashfind_generic: Look up an entry in a hash table and return a pointer
 * to its hash data. Works for both string and numeric hash tables.
 */

int *hashfind_generic(HASHKEY key, HASHTAB *htab)
{
    int hval, numchecks;
    HASHENT *hptr;
    int htype;

    /* Guard against uninitialized or failed hash table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return NULL;
    }

    htype = htab->flags & HT_TYPEMASK;

    numchecks = 0;
    increment_scans(htab);

    hval = get_hash_value(key, htab);

    for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        numchecks++;

        if (key_matches(key, hptr->target, htype))
        {
            update_hash_stats(htab, numchecks, 1);
            return hptr->data;
        }
    }

    update_hash_stats(htab, numchecks, 0);
    return NULL;
}

/* ---------------------------------------------------------------------------
 * hashfindflags_generic: Look up an entry in a hash table and return
 * its flags. Works for both string and numeric hash tables.
 */

int hashfindflags_generic(HASHKEY key, HASHTAB *htab)
{
    int hval, numchecks;
    HASHENT *hptr;
    int htype;

    /* Guard against uninitialized or failed hash table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return 0;
    }

    htype = htab->flags & HT_TYPEMASK;

    numchecks = 0;
    increment_scans(htab);

    hval = get_hash_value(key, htab);

    for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        numchecks++;

        if (key_matches(key, hptr->target, htype))
        {
            update_hash_stats(htab, numchecks, 1);
            return hptr->flags;
        }
    }

    update_hash_stats(htab, numchecks, 0);
    return 0;
}

/* ---------------------------------------------------------------------------
 * hashadd_generic: Add a new entry to a hash table. Works for both string
 * and numeric hashtables.
 */

CF_Result hashadd_generic(HASHKEY key, int *hashdata, HASHTAB *htab, int flags)
{
    int hval;
    HASHENT *hptr;
    int htype;

    /* Guard against uninitialized or failed hash table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        log_write(LOG_BUGS, "BUG", "HASH", "Attempted to add to uninitialized hash table");
        return CF_Failure;
    }

    htype = htab->flags & HT_TYPEMASK;

    /*
     * Make sure that the entry isn't already in the hash table.  If it
     * is, exit with an error.  Otherwise, create a new hash block and
     * link it in at the head of its thread.
     */

    hval = get_hash_value(key, htab);

    /* Check for duplicate in single pass */
    for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        if (key_matches(key, hptr->target, htype))
        {
            return CF_Failure;
        }
    }

    hptr = (HASHENT *)XMALLOC(sizeof(HASHENT), "hptr");

    if (hptr == NULL)
    {
        return CF_Failure;
    }

    if (htab->flags & HT_KEYREF)
    {
        hptr->target = key;
    }
    else
    {
        hptr->target.s = XSTRDUP(key.s, "hptr->target.s");

        if (hptr->target.s == NULL)
        {
            XFREE(hptr);
            return CF_Failure;
        }
    }

    hptr->data = hashdata;
    hptr->flags = flags;
    hptr->next = htab->entry[hval];
    htab->entry[hval] = hptr;

    /* Update statistics only after successful insertion */
    htab->entries++;

    if (hptr->next == NULL)
    {
        htab->nulls--;
    }
    return CF_Success;
}

/* ---------------------------------------------------------------------------
 * hashdelete_generic: Remove an entry from a hash table. Works for both
 * string and numeric hashtables.
 */

void hashdelete_generic(HASHKEY key, HASHTAB *htab)
{
    int hval;
    HASHENT *hptr, *last;
    int htype;

    /* Guard against uninitialized or failed hash table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return;
    }

    htype = htab->flags & HT_TYPEMASK;

    hval = get_hash_value(key, htab);
    last = NULL;

    for (hptr = htab->entry[hval]; hptr != NULL; last = hptr, hptr = hptr->next)
    {
        if (key_matches(key, hptr->target, htype))
        {
            if (last == NULL)
            {
                htab->entry[hval] = hptr->next;
            }
            else
            {
                last->next = hptr->next;
            }

            if (!(htab->flags & HT_KEYREF))
            {
                XFREE(hptr->target.s);
            }

            XFREE(hptr);

            if (htab->deletes < INT_MAX)
            {
                htab->deletes++;
            }

            htab->entries--;

            if (htab->entry[hval] == NULL)
            {
                htab->nulls++;
            }

            return;
        }
    }
}

void hashdelall(int *old, HASHTAB *htab)
{
    int hval;
    HASHENT *hptr, *prev, *nextp;

    /* Guard against NULL pointer deletion and uninitialized table */
    if (old == NULL || htab->entry == NULL || htab->hashsize == 0)
    {
        return;
    }

    for (hval = 0; hval < htab->hashsize; hval++)
    {
        prev = NULL;
        bool bucket_was_nonempty = (htab->entry[hval] != NULL);

        for (hptr = htab->entry[hval]; hptr != NULL; hptr = nextp)
        {
            nextp = hptr->next;

            if (hptr->data == old)
            {
                if (prev == NULL)
                {
                    htab->entry[hval] = nextp;
                }
                else
                {
                    prev->next = nextp;
                }

                if (!(htab->flags & HT_KEYREF))
                {
                    XFREE(hptr->target.s);
                }

                XFREE(hptr);

                if (htab->deletes < INT_MAX)
                {
                    htab->deletes++;
                }

                htab->entries--;

                continue;
            }

            prev = hptr;
        }

        /* Update nulls count only once per bucket if it became empty */
        if (bucket_was_nonempty && htab->entry[hval] == NULL)
        {
            htab->nulls++;
        }
    }
}

/* ---------------------------------------------------------------------------
 * hashflush: free all the entries in a hashtable.
 */

void hashflush(HASHTAB *htab, int size)
{
    HASHENT *hent, *thent;
    int i;

    /* Guard against NULL hash table */
    if (htab == NULL)
    {
        return;
    }

    /* Validate size parameter */
    if (size < 0)
    {
        log_write(LOG_BUGS, "BUG", "HASH", "hashflush called with negative size %d", size);
        size = 0;
    }

    /* Guard against uninitialized table */
    if (htab->entry != NULL && htab->hashsize > 0)
    {
        for (i = 0; i < htab->hashsize; i++)
        {
            hent = htab->entry[i];

            while (hent != NULL)
            {
                thent = hent;
                hent = hent->next;

                if (!(htab->flags & HT_KEYREF))
                {
                    XFREE(thent->target.s);
                }

                XFREE(thent);
            }

            htab->entry[i] = NULL;
        }
    }

    /*
     * Resize if needed.  Otherwise, just zero all the stats
     */

    if ((size > 0) && (size != htab->hashsize))
    {
        XFREE(htab->entry);
        hashinit(htab, size, htab->flags);
    }
    else
    {
        htab->checks = 0;
        htab->scans = 0;
        htab->max_scan = 0;
        htab->hits = 0;
        htab->entries = 0;
        htab->deletes = 0;
        htab->nulls = htab->hashsize;
    }

    /* Reset iterator state to prevent use-after-free on next_* calls */
    htab->last_entry = NULL;
    htab->last_hval = 0;
}

/* ---------------------------------------------------------------------------
 * hashrepl_generic: replace the data part of a hash entry. Works for both
 * string and numeric hashtables.
 */

int hashrepl_generic(HASHKEY key, int *hashdata, HASHTAB *htab)
{
    HASHENT *hptr;
    int hval;
    int htype;

    /* Guard against uninitialized or failed hash table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return 0;
    }

    htype = htab->flags & HT_TYPEMASK;

    hval = get_hash_value(key, htab);

    for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        if (key_matches(key, hptr->target, htype))
        {
            hptr->data = hashdata;
            return 1;
        }
    }

    return 0;
}

void hashreplall(int *old, int *new, HASHTAB *htab)
{
    int hval;
    HASHENT *hptr;

    /* Guard against NULL pointers and uninitialized table */
    if (htab == NULL || old == NULL || new == NULL || htab->entry == NULL || htab->hashsize == 0)
    {
        return;
    }

    for (hval = 0; hval < htab->hashsize; hval++)
        for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
        {
            if (hptr->data == old)
            {
                hptr->data = new;
            }
        }
}

/* ---------------------------------------------------------------------------
 * hashinfo: return an mbuf with hashing stats
 */

char *hashinfo(const char *tab_name, HASHTAB *htab)
{
    char *buff;

    if (htab == NULL)
    {
        return NULL;
    }

    buff = XMALLOC(MBUF_SIZE, "buff");

    if (buff == NULL)
    {
        return NULL;
    }

    XSPRINTF(buff, "%-15.15s%8d%8d%8d%8d%8d%8d%8d%8d", tab_name, htab->hashsize, htab->entries, htab->deletes, htab->nulls, htab->scans, htab->hits, htab->checks, htab->max_scan);
    return buff;
}

/* Returns the data for the first hash entry in 'htab'. */

int *hash_firstentry(HASHTAB *htab)
{
    int hval;

    /* Guard against uninitialized table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return NULL;
    }

    /* Reset iterator state only, not stats */
    htab->last_entry = NULL;
    htab->last_hval = 0;

    for (hval = 0; hval < htab->hashsize; hval++)
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->data;
        }

    return NULL;
}

int *hash_nextentry(HASHTAB *htab)
{
    int hval;
    HASHENT *hptr;

    /* Guard against uninitialized table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return NULL;
    }

    hval = htab->last_hval;
    hptr = htab->last_entry;

    if (hptr == NULL)
    {
        return NULL;
    }

    if (hptr->next != NULL)
    { /* We can stay in the same chain */
        htab->last_entry = hptr->next;
        return hptr->next->data;
    }

    /*
     * We were at the end of the previous chain, go to the next one
     */
    hval++;

    while (hval < htab->hashsize)
    {
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->data;
        }

        hval++;
    }

    return NULL;
}

/* Returns the key for the first hash entry in 'htab'. */

HASHKEY hash_firstkey_generic(HASHTAB *htab)
{
    int hval;
    int htype;
    HASHKEY null_key;

    /* Guard against uninitialized table - check before accessing htab->flags */
    if (htab == NULL || htab->entry == NULL || htab->hashsize == 0)
    {
        /* Return safe NULL key - assume string type for safety */
        null_key.s = NULL;
        return null_key;
    }

    htype = htab->flags & HT_TYPEMASK;

    /* Prepare type-appropriate NULL key */
    if (htype == HT_STR)
    {
        null_key.s = NULL;
    }
    else
    {
        null_key.i = -1;
    }

    for (hval = 0; hval < htab->hashsize; hval++)
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->target;
        }

    return null_key;
}

HASHKEY hash_nextkey_generic(HASHTAB *htab)
{
    int hval;
    HASHENT *hptr;
    int htype;
    HASHKEY null_key;

    /* Guard against uninitialized table - check before accessing htab->flags */
    if (htab == NULL || htab->entry == NULL || htab->hashsize == 0)
    {
        /* Return safe NULL key - assume string type for safety */
        null_key.s = NULL;
        return null_key;
    }

    htype = htab->flags & HT_TYPEMASK;

    /* Prepare type-appropriate NULL key */
    if (htype == HT_STR)
    {
        null_key.s = NULL;
    }
    else
    {
        null_key.i = -1;
    }

    hval = htab->last_hval;
    hptr = htab->last_entry;

    if (hptr == NULL)
    {
        return null_key;
    }

    if (hptr->next != NULL)
    { /* We can stay in the same chain */
        htab->last_entry = hptr->next;
        return hptr->next->target;
    }

    /*
     * We were at the end of the previous chain, go to the next one
     */
    hval++;

    while (hval < htab->hashsize)
    {
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->target;
        }

        hval++;
    }

    return null_key;
}

/* ---------------------------------------------------------------------------
 * hashresize: Resize a hash table, to adjust the number of slots to be
 * a power of 2 appropriate to the number of entries in it.
 */

void hashresize(HASHTAB *htab, int min_size)
{
    int size, i, htype, hval;
    HASHTAB new_htab;
    HASHENT *hent, *thent;

    /* Guard against uninitialized table */
    if (htab->entry == NULL || htab->hashsize == 0)
    {
        return;
    }

    /* Guard against invalid hash_factor */
    if (mushconf.hash_factor < 1)
    {
        log_write(LOG_BUGS, "BUG", "HASH", "Invalid hash_factor %d, aborting resize", mushconf.hash_factor);
        return;
    }

    /* Calculate new size with overflow check */
    if (htab->entries > INT_MAX / mushconf.hash_factor)
    {
        /* Avoid overflow; cap at reasonable size */
        size = INT_MAX / 2;
    }
    else
    {
        size = (htab->entries) * mushconf.hash_factor;
    }

    size = (size < min_size) ? min_size : size;
    get_hashmask(&size);

    /* Downsize if table is oversized (avoid overflow in threshold calculation) */
    if (size > 512)
    {
        /* Use integer arithmetic to avoid overflow: size > entries * 1.33 * factor
         * means size > entries * factor * 1.33, approximately size > entries * factor * 4/3 */
        long long threshold = ((long long)htab->entries * mushconf.hash_factor * 4) / 3;
        if (threshold > INT_MAX)
        {
            threshold = INT_MAX;
        }
        if (size > threshold)
        {
            size /= 2;
            /* Ensure size remains power-of-2 after division */
            get_hashmask(&size);
        }
    }

    if (size == htab->hashsize)
    {
        /*
         * We're already at the correct size. Don't do anything.
         */
        return;
    }

    hashinit(&new_htab, size, htab->flags);

    if (new_htab.entry == NULL)
    {
        /* Allocation failed, abort resize */
        return;
    }

    htype = htab->flags & HT_TYPEMASK;

    for (i = 0; i < htab->hashsize; i++)
    {
        hent = htab->entry[i];

        while (hent != NULL)
        {
            thent = hent;
            hent = hent->next;

            /*
             * don't free and reallocate entries, just copy the pointers
             */
            hval = get_hash_value(thent->target, &new_htab);

            if (new_htab.entry[hval] == NULL)
            {
                new_htab.nulls--;
            }

            thent->next = new_htab.entry[hval];
            new_htab.entry[hval] = thent;
        }
    }

    XFREE(htab->entry);
    htab->hashsize = new_htab.hashsize;
    htab->mask = new_htab.mask;
    /* Keep original statistics across resize; only update structure references */
    htab->nulls = new_htab.nulls;
    htab->entry = new_htab.entry;
    /* Note: Resize invalidates any active iterators. Caller must restart iteration. */
    htab->last_hval = 0;
    htab->last_entry = NULL;
    /*
     * number of entries doesn't change
     */
    /*
     * flags don't change
     */
}

/* ---------------------------------------------------------------------------
 * search_nametab: Search a name table for a match and return the flag value.
 */

int search_nametab(dbref player, NAMETAB *ntab, char *flagname)
{
    NAMETAB *nt;

    if (ntab == NULL)
    {
        return -1;
    }

    for (nt = ntab; nt->name; nt++)
    {
        if (minmatch(flagname, nt->name, nt->minlen))
        {
            if (check_access(player, nt->perm))
            {
                return nt->flag;
            }
            else
            {
                return -2;
            }
        }
    }

    return -1;
}

/* ---------------------------------------------------------------------------
 * find_nametab_ent: Search a name table for a match and return a pointer to it.
 */

NAMETAB *find_nametab_ent(dbref player, NAMETAB *ntab, char *flagname)
{
    NAMETAB *nt;

    if (ntab == NULL)
    {
        return NULL;
    }

    for (nt = ntab; nt->name; nt++)
    {
        if (minmatch(flagname, nt->name, nt->minlen))
        {
            if (check_access(player, nt->perm))
            {
                return nt;
            }
        }
    }

    return NULL;
}

/* ---------------------------------------------------------------------------
 * find_nametab_ent_flag: Search a name table for a match by flag value
 * and return a pointer to it.
 */

NAMETAB *find_nametab_ent_flag(dbref player, NAMETAB *ntab, int flag)
{
    NAMETAB *nt;

    if (ntab == NULL)
    {
        return NULL;
    }

    for (nt = ntab; nt->name; nt++)
    {
        if (flag == nt->flag)
        {
            if (check_access(player, nt->perm))
            {
                return nt;
            }
        }
    }

    return NULL;
}

/* ---------------------------------------------------------------------------
 * display_nametab: Print out the names of the entries in a name table.
 */

void display_nametab(dbref player, NAMETAB *ntab, bool list_if_none, const char *format, ...)
{
    char *prefix, *buf, *bp, *cp;
    NAMETAB *nt;
    bool got_one = false;

    if (ntab == NULL)
    {
        return;
    }

    bp = buf = XMALLOC(LBUF_SIZE, "buf");

    if (buf == NULL)
    {
        return;
    }

    va_list ap;

    va_start(ap, format);
    prefix = XAVSPRINTF("buf", format, ap);
    va_end(ap);

    for (nt = ntab; nt->name; nt++)
    {
        if (God(player) || check_access(player, nt->perm))
        {
            if (got_one)
            {
                /* Check buffer overflow before adding separator */
                if (bp - buf >= LBUF_SIZE - 1)
                {
                    break;
                }
                *bp++ = ' ';
            }

            for (cp = nt->name; *cp; cp++)
            {
                /* Check buffer overflow before adding each character */
                if (bp - buf >= LBUF_SIZE - 1)
                {
                    break;
                }
                *bp++ = *cp;
            }

            /* Stop if buffer is full */
            if (bp - buf >= LBUF_SIZE - 1)
            {
                break;
            }

            got_one = true;
        }
    }

    *bp = '\0';

    if ((got_one || list_if_none) && prefix != NULL)
    {
        raw_notify(player, "%s %s", prefix, buf);
    }

    XFREE(buf);
    XFREE(prefix);
}

/* ---------------------------------------------------------------------------
 * interp_nametab: Print values for flags defined in name table.
 */

void interp_nametab(dbref player, NAMETAB *ntab, int flagword, char *prefix, char *state, char *true_text, char *false_text, bool show_sep)
{
    NAMETAB *nt = ntab;

    if (ntab == NULL)
    {
        return;
    }

    raw_notify(player, "%-30.30s %s", prefix, state);
    if (show_sep)
    {
        notify(player, "------------------------------ ------------------------------------------------");
    }

    while (nt->name)
    {
        if (God(player) || check_access(player, nt->perm))
        {
            raw_notify(player, "%-30.30s %s", nt->name, (flagword & nt->flag) ? true_text : false_text);
        }
        ++nt;
    }
    if (show_sep)
    {
        notify(player, "-------------------------------------------------------------------------------");
    }
}

/* ---------------------------------------------------------------------------
 * listset_nametab: Print values for flags defined in name table.
 */

void listset_nametab(dbref player, NAMETAB *ntab, int flagword, bool list_if_none, char *format, ...)
{
    char *prefix, *buf, *bp, *cp;
    NAMETAB *nt = ntab;
    bool got_one = false;

    if (ntab == NULL)
    {
        return;
    }

    buf = bp = XMALLOC(LBUF_SIZE, "buf");

    if (buf == NULL)
    {
        return;
    }

    va_list ap;

    va_start(ap, format);
    prefix = XAVSPRINTF("buf", format, ap);
    va_end(ap);

    while (nt->name)
    {
        if (((flagword & nt->flag) != 0) && (God(player) || check_access(player, nt->perm)))
        {
            if (got_one)
            {
                /* Check buffer overflow before adding separator */
                if (bp - buf >= LBUF_SIZE - 1)
                {
                    break;
                }
                *bp++ = ' ';
            }

            for (cp = nt->name; *cp; cp++)
            {
                /* Check buffer overflow before adding each character */
                if (bp - buf >= LBUF_SIZE - 1)
                {
                    break;
                }
                *bp++ = *cp;
            }

            /* Stop if buffer is full */
            if (bp - buf >= LBUF_SIZE - 1)
            {
                break;
            }

            got_one = true;
        }

        nt++;
    }

    *bp = '\0';

    if ((got_one || list_if_none) && prefix != NULL)
    {
        raw_notify(player, "%s%s", prefix, buf);
    }

    XFREE(buf);
    XFREE(prefix);
}

/* ---------------------------------------------------------------------------
 * cf_ntab_access: Change the access on a nametab entry.
 */

int cf_ntab_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
    NAMETAB *np;
    char *ap;

    for (ap = str; *ap && !isspace(*ap); ap++)
        ;

    if (*ap)
    {
        *ap++ = '\0';
    }

    while (*ap && isspace(*ap))
    {
        ap++;
    }

    for (np = (NAMETAB *)vp; np->name; np++)
    {
        if (minmatch(str, np->name, np->minlen))
        {
            return cf_modify_bits(&(np->perm), ap, extra, player, cmd);
        }
    }

    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Entry", str);
    return -1;
}
