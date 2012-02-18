/* htab.c - table hashing routines */
/* $Id: htab.c,v 1.27 2002/10/20 18:26:43 rmg Exp $ */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

/* ---------------------------------------------------------------------------
 * hashval: Compute hash value of a string for a hash table.
 */

int
hashval(str, hashmask)
char *str;

int hashmask;
{
    int hash = 0;

    char *sp;

    /*
     * If the string pointer is null, return 0.  If not, add up the
     * * numeric value of all the characters and return the sum,
     * * modulo the size of the hash table.
     */

    if (str == NULL)
        return 0;
    for (sp = str; *sp; sp++)
        hash = (hash << 5) + hash + *sp;
    return (hash & hashmask);
}


/* ----------------------------------------------------------------------
 * get_hashmask: Get hash mask for mask-style hashing.
 */

int
get_hashmask(size)
int *size;
{
    int tsize;

    /*
     * Get next power-of-two >= size, return power-1 as the mask for
     * * ANDing
     */

    for (tsize = 1; tsize < *size; tsize = tsize << 1);
    *size = tsize;
    return tsize - 1;
}

/* ---------------------------------------------------------------------------
 * hashinit: Initialize a new hash table.
 */

void
hashinit(htab, size, flags)
HASHTAB *htab;

int size, flags;
{
    int i;

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
    htab->entry = (HASHENT **) XCALLOC(size, sizeof(HASHENT *),
                                       "hashinit");

    for (i = 0; i < size; i++)
        htab->entry[i] = NULL;
}

/* ---------------------------------------------------------------------------
 * hashreset: Reset hash table stats.
 */

void
hashreset(htab)
HASHTAB *htab;
{
    htab->checks = 0;
    htab->scans = 0;
    htab->hits = 0;
}

/* ---------------------------------------------------------------------------
 * hashfind_generic: Look up an entry in a hash table and return a pointer
 * to its hash data. Works for both string and numeric hash tables.
 */

int *
hashfind_generic(key, htab)
HASHKEY key;

HASHTAB *htab;
{
    int htype, hval, numchecks;

    HASHENT *hptr, *prev;

    numchecks = 0;
    htab->scans++;
    htype = htab->flags & HT_TYPEMASK;
    if (htype == HT_STR)
    {
        hval = hashval(key.s, htab->mask);
    }
    else
    {
        hval = (key.i & htab->mask);
    }
    for (prev = hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        numchecks++;
        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) ||
                (htype == HT_NUM && key.i == hptr->target.i))
        {
            htab->hits++;
            if (numchecks > htab->max_scan)
                htab->max_scan = numchecks;
            htab->checks += numchecks;

            return hptr->data;
        }
        prev = hptr;
    }
    if (numchecks > htab->max_scan)
        htab->max_scan = numchecks;
    htab->checks += numchecks;
    return NULL;
}

/* ---------------------------------------------------------------------------
 * hashfindflags_generic: Look up an entry in a hash table and return
 * its flags. Works for both string and numeric hash tables.
 */

int
hashfindflags_generic(key, htab)
HASHKEY key;

HASHTAB *htab;
{
    int htype, hval, numchecks;

    HASHENT *hptr, *prev;

    numchecks = 0;
    htab->scans++;
    htype = htab->flags & HT_TYPEMASK;
    if (htype == HT_STR)
    {
        hval = hashval(key.s, htab->mask);
    }
    else
    {
        hval = (key.i & htab->mask);
    }
    for (prev = hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        numchecks++;
        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) ||
                (htype == HT_NUM && key.i == hptr->target.i))
        {
            htab->hits++;
            if (numchecks > htab->max_scan)
                htab->max_scan = numchecks;
            htab->checks += numchecks;

            return hptr->flags;
        }
        prev = hptr;
    }
    if (numchecks > htab->max_scan)
        htab->max_scan = numchecks;
    htab->checks += numchecks;
    return 0;
}

/* ---------------------------------------------------------------------------
 * hashadd_generic: Add a new entry to a hash table. Works for both string
 * and numeric hashtables.
 */

int
hashadd_generic(key, hashdata, htab, flags)
HASHKEY key;

int *hashdata;

HASHTAB *htab;

int flags;
{
    int htype, hval;

    HASHENT *hptr;

    /*
     * Make sure that the entry isn't already in the hash table.  If it
     * is, exit with an error.  Otherwise, create a new hash block and
     * link it in at the head of its thread.
     */

    if (hashfind_generic(key, htab) != NULL)
        return (-1);
    htype = htab->flags & HT_TYPEMASK;
    if (htype == HT_STR)
    {
        hval = hashval(key.s, htab->mask);
    }
    else
    {
        hval = (key.i & htab->mask);
    }
    htab->entries++;
    if (htab->entry[hval] == NULL)
        htab->nulls--;
    hptr = (HASHENT *) XMALLOC(sizeof(HASHENT), "hashadd");
    if (htab->flags & HT_KEYREF)
    {
        hptr->target = key;
    }
    else
    {
        hptr->target.s = XSTRDUP(key.s, "hashadd.target");
    }
    hptr->data = hashdata;
    hptr->flags = flags;
    hptr->next = htab->entry[hval];
    htab->entry[hval] = hptr;
    return (0);
}

/* ---------------------------------------------------------------------------
 * hashdelete_generic: Remove an entry from a hash table. Works for both
 * string and numeric hashtables.
 */

void
hashdelete_generic(key, htab)
HASHKEY key;

HASHTAB *htab;
{
    int htype, hval;

    HASHENT *hptr, *last;

    htype = htab->flags & HT_TYPEMASK;
    if (htype == HT_STR)
    {
        hval = hashval(key.s, htab->mask);
    }
    else
    {
        hval = (key.i & htab->mask);
    }
    last = NULL;
    for (hptr = htab->entry[hval];
            hptr != NULL; last = hptr, hptr = hptr->next)
    {
        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) ||
                (htype == HT_NUM && key.i == hptr->target.i))
        {
            if (last == NULL)
                htab->entry[hval] = hptr->next;
            else
                last->next = hptr->next;
            if (!(htab->flags & HT_KEYREF))
            {
                XFREE(hptr->target.s, "hashdelete.target");
            }
            XFREE(hptr, "hashdelete.hptr");
            htab->deletes++;
            htab->entries--;
            if (htab->entry[hval] == NULL)
                htab->nulls++;
            return;
        }
    }
}

void
hashdelall(old, htab)
int *old;

HASHTAB *htab;
{
    int hval;

    HASHENT *hptr, *prev, *nextp;

    for (hval = 0; hval < htab->hashsize; hval++)
    {
        prev = NULL;
        for (hptr = htab->entry[hval]; hptr != NULL; hptr = nextp)
        {
            nextp = hptr->next;
            if (hptr->data == old)
            {
                if (prev == NULL)
                    htab->entry[hval] = nextp;
                else
                    prev->next = nextp;
                if (!(htab->flags & HT_KEYREF))
                {
                    XFREE(hptr->target.s,
                          "hashdelall.target");
                }
                XFREE(hptr, "hashdelall.hptr");
                htab->deletes++;
                htab->entries--;
                if (htab->entry[hval] == NULL)
                    htab->nulls++;
                continue;
            }
            prev = hptr;
        }
    }
}

/* ---------------------------------------------------------------------------
 * hashflush: free all the entries in a hashtable.
 */

void
hashflush(htab, size)
HASHTAB *htab;

int size;
{
    HASHENT *hent, *thent;

    int i;

    for (i = 0; i < htab->hashsize; i++)
    {
        hent = htab->entry[i];
        while (hent != NULL)
        {
            thent = hent;
            hent = hent->next;
            if (!(htab->flags & HT_KEYREF))
            {
                XFREE(thent->target.s, "hashflush.target");
            }
            XFREE(thent, "hashflush.hent");
        }
        htab->entry[i] = NULL;
    }

    /*
     * Resize if needed.  Otherwise, just zero all the stats
     */

    if ((size > 0) && (size != htab->hashsize))
    {
        XFREE(htab->entry, "hashflush.table");
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
}

/* ---------------------------------------------------------------------------
 * hashrepl_generic: replace the data part of a hash entry. Works for both
 * string and numeric hashtables.
 */

int
hashrepl_generic(key, hashdata, htab)
HASHKEY key;

int *hashdata;

HASHTAB *htab;
{
    HASHENT *hptr;

    int htype, hval;

    htype = htab->flags & HT_TYPEMASK;
    if (htype == HT_STR)
    {
        hval = hashval(key.s, htab->mask);
    }
    else
    {
        hval = (key.i & htab->mask);
    }
    for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
    {
        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) ||
                (htype == HT_NUM && key.i == hptr->target.i))
        {
            hptr->data = hashdata;
            return 1;
        }
    }
    return 0;
}

void
hashreplall(old, new, htab)
int *old, *new;

HASHTAB *htab;
{
    int hval;

    HASHENT *hptr;

    for (hval = 0; hval < htab->hashsize; hval++)
        for (hptr = htab->entry[hval]; hptr != NULL; hptr = hptr->next)
        {
            if (hptr->data == old)
                hptr->data = new;
        }
}

/* ---------------------------------------------------------------------------
 * hashinfo: return an mbuf with hashing stats
 */

char *
hashinfo(tab_name, htab)
const char *tab_name;

HASHTAB *htab;
{
    char *buff;

    buff = alloc_mbuf("hashinfo");
    sprintf(buff, "%-15s %5d%8d%8d%8d%8d%8d%8d%8d",
            tab_name, htab->hashsize, htab->entries, htab->deletes,
            htab->nulls, htab->scans, htab->hits, htab->checks,
            htab->max_scan);
    return buff;
}

/* Returns the data for the first hash entry in 'htab'. */

int *
hash_firstentry(htab)
HASHTAB *htab;
{
    int hval;

    for (hval = 0; hval < htab->hashsize; hval++)
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->data;
        }
    return NULL;
}

int *
hash_nextentry(htab)
HASHTAB *htab;
{
    int hval;

    HASHENT *hptr;

    hval = htab->last_hval;
    hptr = htab->last_entry;
    if (hptr->next != NULL)  	/* We can stay in the same chain */
    {
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

HASHKEY
hash_firstkey_generic(htab)
HASHTAB *htab;
{
    int hval;

    for (hval = 0; hval < htab->hashsize; hval++)
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->target;
        }

    if ((htab->flags & HT_TYPEMASK) == HT_STR)
    {
        return (HASHKEY) ((char *)NULL);
    }
    else
    {
        return (HASHKEY) ((int)-1);
    }
}

HASHKEY
hash_nextkey_generic(htab)
HASHTAB *htab;
{
    int hval;

    HASHENT *hptr;

    hval = htab->last_hval;
    hptr = htab->last_entry;
    if (hptr->next != NULL)  	/* We can stay in the same chain */
    {
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

    if ((htab->flags & HT_TYPEMASK) == HT_STR)
    {
        return (HASHKEY) ((char *)NULL);
    }
    else
    {
        return (HASHKEY) ((int)-1);
    }
}

/* ---------------------------------------------------------------------------
 * hashresize: Resize a hash table, to adjust the number of slots to be
 * a power of 2 appropriate to the number of entries in it.
 */

void
hashresize(htab, min_size)
HASHTAB *htab;

int min_size;
{
    int size, i, htype, hval;

    HASHTAB new_htab;

    HASHENT *hent, *thent;

    size = (htab->entries) * HASH_FACTOR;
    size = (size < min_size) ? min_size : size;
    get_hashmask(&size);
    if ((size > 512) && (size > htab->entries * 1.33 * HASH_FACTOR))
        size /= 2;
    if (size == htab->hashsize)
    {
        /*
         * We're already at the correct size. Don't do anything.
         */
        return;
    }

    hashinit(&new_htab, size, htab->flags);
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
            if (htype == HT_STR)
            {
                hval = hashval(thent->target.s, new_htab.mask);
            }
            else
            {
                hval = (thent->target.i & new_htab.mask);
            }
            if (new_htab.entry[hval] == NULL)
                new_htab.nulls--;
            thent->next = new_htab.entry[hval];
            new_htab.entry[hval] = thent;
        }
    }
    XFREE(htab->entry, "hashinit");

    htab->hashsize = new_htab.hashsize;
    htab->mask = new_htab.mask;
    htab->checks = new_htab.checks;
    htab->scans = new_htab.scans;
    htab->max_scan = new_htab.max_scan;
    htab->hits = new_htab.hits;
    htab->deletes = new_htab.deletes;
    htab->nulls = new_htab.nulls;
    htab->entry = new_htab.entry;
    htab->last_hval = new_htab.last_hval;
    htab->last_entry = new_htab.last_entry;
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

int
search_nametab(player, ntab, flagname)
dbref player;

NAMETAB *ntab;

char *flagname;
{
    NAMETAB *nt;

    for (nt = ntab; nt->name; nt++)
    {
        if (minmatch(flagname, nt->name, nt->minlen))
        {
            if (check_access(player, nt->perm))
            {
                return nt->flag;
            }
            else
                return -2;
        }
    }
    return -1;
}

/* ---------------------------------------------------------------------------
 * find_nametab_ent: Search a name table for a match and return a pointer to it.
 */

NAMETAB *
find_nametab_ent(player, ntab, flagname)
dbref player;

NAMETAB *ntab;

char *flagname;
{
    NAMETAB *nt;

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

NAMETAB *
find_nametab_ent_flag(player, ntab, flag)
dbref player;

NAMETAB *ntab;

int flag;
{
    NAMETAB *nt;

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

void
display_nametab(player, ntab, prefix, list_if_none)
dbref player;

NAMETAB *ntab;

char *prefix;

int list_if_none;
{
    char *buf, *bp, *cp;

    NAMETAB *nt;

    int got_one;

    buf = alloc_lbuf("display_nametab");
    bp = buf;
    got_one = 0;
    for (cp = prefix; *cp; cp++)
        *bp++ = *cp;
    for (nt = ntab; nt->name; nt++)
    {
        if (God(player) || check_access(player, nt->perm))
        {
            *bp++ = ' ';
            for (cp = nt->name; *cp; cp++)
                *bp++ = *cp;
            got_one = 1;
        }
    }
    *bp = '\0';
    if (got_one || list_if_none)
        notify(player, buf);
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * interp_nametab: Print values for flags defined in name table.
 */

void
interp_nametab(player, ntab, flagword, prefix, true_text, false_text)
dbref player;

NAMETAB *ntab;

int flagword;

char *prefix, *true_text, *false_text;
{
    char *buf, *bp, *cp;

    NAMETAB *nt;

    buf = alloc_lbuf("interp_nametab");
    bp = buf;
    for (cp = prefix; *cp; cp++)
        *bp++ = *cp;
    nt = ntab;
    while (nt->name)
    {
        if (God(player) || check_access(player, nt->perm))
        {
            *bp++ = ' ';
            for (cp = nt->name; *cp; cp++)
                *bp++ = *cp;
            *bp++ = '.';
            *bp++ = '.';
            *bp++ = '.';
            if ((flagword & nt->flag) != 0)
                cp = true_text;
            else
                cp = false_text;
            while (*cp)
                *bp++ = *cp++;
            if ((++nt)->name)
                *bp++ = ';';
        }
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * listset_nametab: Print values for flags defined in name table.
 */

void
listset_nametab(player, ntab, flagword, prefix, list_if_none)
dbref player;

NAMETAB *ntab;

int flagword, list_if_none;

char *prefix;
{
    char *buf, *bp, *cp;

    NAMETAB *nt;

    int got_one;

    buf = bp = alloc_lbuf("listset_nametab");
    for (cp = prefix; *cp; cp++)
        *bp++ = *cp;
    nt = ntab;
    got_one = 0;
    while (nt->name)
    {
        if (((flagword & nt->flag) != 0) &&
                (God(player) || check_access(player, nt->perm)))
        {
            *bp++ = ' ';
            for (cp = nt->name; *cp; cp++)
                *bp++ = *cp;
            got_one = 1;
        }
        nt++;
    }
    *bp = '\0';
    if (got_one || list_if_none)
        notify(player, buf);
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * cf_ntab_access: Change the access on a nametab entry.
 */

CF_HAND(cf_ntab_access)
{
    NAMETAB *np;

    char *ap;

    for (ap = str; *ap && !isspace(*ap); ap++);
    if (*ap)
        *ap++ = '\0';
    while (*ap && isspace(*ap))
        ap++;
    for (np = (NAMETAB *) vp; np->name; np++)
    {
        if (minmatch(str, np->name, np->minlen))
        {
            return cf_modify_bits(&(np->perm), ap, extra,
                                  player, cmd);
        }
    }
    cf_log_notfound(player, cmd, "Entry", str);
    return -1;
}
