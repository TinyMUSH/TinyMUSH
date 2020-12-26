/* htab.c - table hashing routines */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mudconf */
#include "game.h"      /* required by mudconf */
#include "alloc.h"     /* required by mudconf */
#include "flags.h"     /* required by mudconf */
#include "htab.h"      /* required by mudconf */
#include "ltdl.h"      /* required by mudconf */
#include "udb.h"       /* required by mudconf */
#include "udb_defs.h"  /* required by mudconf */
#include "mushconf.h"  /* required by code */
#include "db.h"        /* required by externs */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by code */

/* ---------------------------------------------------------------------------
 * hashval: Compute hash value of a string for a hash table.
 */

int hashval(char *str, int hashmask)
{
    int hash = 0;
    char *sp;

    /*
     * If the string pointer is null, return 0.  If not, add up the
     * * numeric value of all the characters and return the sum,
     * * modulo the size of the hash table.
     */

    if (str == NULL)
    {
        return 0;
    }

    for (sp = str; *sp; sp++)
    {
        hash = (hash << 5) + hash + *sp;
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
     * * ANDing
     */

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
    htab->entry = (HASHENT **)XCALLOC(size, sizeof(HASHENT *), "htab->entry");

    for (i = 0; i < size; i++)
    {
        htab->entry[i] = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * hashreset: Reset hash table stats.
 */

void hashreset(HASHTAB *htab)
{
    htab->checks = 0;
    htab->scans = 0;
    htab->hits = 0;
}

/* ---------------------------------------------------------------------------
 * hashfind_generic: Look up an entry in a hash table and return a pointer
 * to its hash data. Works for both string and numeric hash tables.
 */

int *hashfind_generic(HASHKEY key, HASHTAB *htab)
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

        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) || (htype == HT_NUM && key.i == hptr->target.i))
        {
            htab->hits++;

            if (numchecks > htab->max_scan)
            {
                htab->max_scan = numchecks;
            }

            htab->checks += numchecks;
            return hptr->data;
        }

        prev = hptr;
    }

    if (numchecks > htab->max_scan)
    {
        htab->max_scan = numchecks;
    }

    htab->checks += numchecks;
    return NULL;
}

/* ---------------------------------------------------------------------------
 * hashfindflags_generic: Look up an entry in a hash table and return
 * its flags. Works for both string and numeric hash tables.
 */

int hashfindflags_generic(HASHKEY key, HASHTAB *htab)
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

        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) || (htype == HT_NUM && key.i == hptr->target.i))
        {
            htab->hits++;

            if (numchecks > htab->max_scan)
            {
                htab->max_scan = numchecks;
            }

            htab->checks += numchecks;
            return hptr->flags;
        }

        prev = hptr;
    }

    if (numchecks > htab->max_scan)
    {
        htab->max_scan = numchecks;
    }

    htab->checks += numchecks;
    return 0;
}

/* ---------------------------------------------------------------------------
 * hashadd_generic: Add a new entry to a hash table. Works for both string
 * and numeric hashtables.
 */

CF_Result hashadd_generic(HASHKEY key, int *hashdata, HASHTAB *htab, int flags)
{
    int htype, hval;
    HASHENT *hptr;

    /*
     * Make sure that the entry isn't already in the hash table.  If it
     * is, exit with an error.  Otherwise, create a new hash block and
     * link it in at the head of its thread.
     */

    if (hashfind_generic(key, htab) != NULL)
    {
        return CF_Failure;
    }

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
    {
        htab->nulls--;
    }

    hptr = (HASHENT *)XMALLOC(sizeof(HASHENT), "hptr");

    if (htab->flags & HT_KEYREF)
    {
        hptr->target = key;
    }
    else
    {
        hptr->target.s = XSTRDUP(key.s, "hptr->target.s");
    }

    hptr->data = hashdata;
    hptr->flags = flags;
    hptr->next = htab->entry[hval];
    htab->entry[hval] = hptr;
    return CF_Success;
}

/* ---------------------------------------------------------------------------
 * hashdelete_generic: Remove an entry from a hash table. Works for both
 * string and numeric hashtables.
 */

void hashdelete_generic(HASHKEY key, HASHTAB *htab)
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

    for (hptr = htab->entry[hval]; hptr != NULL; last = hptr, hptr = hptr->next)
    {
        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) || (htype == HT_NUM && key.i == hptr->target.i))
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
            htab->deletes++;
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

    for (hval = 0; hval < htab->hashsize; hval++)
    {
        prev = NULL;

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
                htab->deletes++;
                htab->entries--;

                if (htab->entry[hval] == NULL)
                {
                    htab->nulls++;
                }

                continue;
            }

            prev = hptr;
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
}

/* ---------------------------------------------------------------------------
 * hashrepl_generic: replace the data part of a hash entry. Works for both
 * string and numeric hashtables.
 */

int hashrepl_generic(HASHKEY key, int *hashdata, HASHTAB *htab)
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
        if ((htype == HT_STR && strcmp(key.s, hptr->target.s) == 0) || (htype == HT_NUM && key.i == hptr->target.i))
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
    buff = XMALLOC(MBUF_SIZE, "buff");
    XSPRINTF(buff, "%-15s %5d%8d%8d%8d%8d%8d%8d%8d", tab_name, htab->hashsize, htab->entries, htab->deletes, htab->nulls, htab->scans, htab->hits, htab->checks, htab->max_scan);
    return buff;
}

/* Returns the data for the first hash entry in 'htab'. */

int *hash_firstentry(HASHTAB *htab)
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

int *hash_nextentry(HASHTAB *htab)
{
    int hval;
    HASHENT *hptr;
    hval = htab->last_hval;
    hptr = htab->last_entry;

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

    for (hval = 0; hval < htab->hashsize; hval++)
        if (htab->entry[hval] != NULL)
        {
            htab->last_hval = hval;
            htab->last_entry = htab->entry[hval];
            return htab->entry[hval]->target;
        }

    if ((htab->flags & HT_TYPEMASK) == HT_STR)
    {
        return (HASHKEY)((char *)NULL);
    }
    else
    {
        return (HASHKEY)((int)-1);
    }
}

HASHKEY hash_nextkey_generic(HASHTAB *htab)
{
    int hval;
    HASHENT *hptr;
    hval = htab->last_hval;
    hptr = htab->last_entry;

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

    if ((htab->flags & HT_TYPEMASK) == HT_STR)
    {
        return (HASHKEY)((char *)NULL);
    }
    else
    {
        return (HASHKEY)((int)-1);
    }
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
    size = (htab->entries) * mudconf.hash_factor;
    size = (size < min_size) ? min_size : size;
    get_hashmask(&size);

    if ((size > 512) && (size > htab->entries * 1.33 * mudconf.hash_factor))
    {
        size /= 2;
    }

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

int search_nametab(dbref player, NAMETAB *ntab, char *flagname)
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

void display_nametab(dbref player, NAMETAB *ntab, char *prefix, int list_if_none)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;
    int got_one;
    bp = buf = XMALLOC(LBUF_SIZE, "buf");
    got_one = 0;

    for (cp = prefix; *cp; cp++)
    {
        *bp++ = *cp;
    }

    for (nt = ntab; nt->name; nt++)
    {
        if (God(player) || check_access(player, nt->perm))
        {
            *bp++ = ' ';

            for (cp = nt->name; *cp; cp++)
            {
                *bp++ = *cp;
            }

            got_one = 1;
        }
    }

    *bp = '\0';

    if (got_one || list_if_none)
    {
        notify(player, buf);
    }

    XFREE(buf);
}

/* ---------------------------------------------------------------------------
 * interp_nametab: Print values for flags defined in name table.
 */

void interp_nametab(dbref player, NAMETAB *ntab, int flagword, char *prefix, char *true_text, char *false_text)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;
    buf = XMALLOC(LBUF_SIZE, "buf");
    bp = buf;

    for (cp = prefix; *cp; cp++)
    {
        *bp++ = *cp;
    }

    nt = ntab;

    while (nt->name)
    {
        if (God(player) || check_access(player, nt->perm))
        {
            *bp++ = ' ';

            for (cp = nt->name; *cp; cp++)
            {
                *bp++ = *cp;
            }

            *bp++ = '.';
            *bp++ = '.';
            *bp++ = '.';

            if ((flagword & nt->flag) != 0)
            {
                cp = true_text;
            }
            else
            {
                cp = false_text;
            }

            while (*cp)
            {
                *bp++ = *cp++;
            }

            if ((++nt)->name)
            {
                *bp++ = ';';
            }
        }
    }

    *bp = '\0';
    notify(player, buf);
    XFREE(buf);
}

/* ---------------------------------------------------------------------------
 * listset_nametab: Print values for flags defined in name table.
 */

void listset_nametab(dbref player, NAMETAB *ntab, int flagword, char *prefix, int list_if_none)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;
    int got_one;
    buf = bp = XMALLOC(LBUF_SIZE, "buf");

    for (cp = prefix; *cp; cp++)
    {
        *bp++ = *cp;
    }

    nt = ntab;
    got_one = 0;

    while (nt->name)
    {
        if (((flagword & nt->flag) != 0) && (God(player) || check_access(player, nt->perm)))
        {
            *bp++ = ' ';

            for (cp = nt->name; *cp; cp++)
            {
                *bp++ = *cp;
            }

            got_one = 1;
        }

        nt++;
    }

    *bp = '\0';

    if (got_one || list_if_none)
    {
        notify(player, buf);
    }

    XFREE(buf);
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
