/* htab.h - Structures and declarations needed for table hashing */

#include "copyright.h"

#ifndef __HTAB_H
#define __HTAB_H

typedef union
{
    char *s;
    int i;
} HASHKEY;

typedef struct hashentry HASHENT;
struct hashentry
{
    HASHKEY target;
    int *data;
    int flags;
    struct hashentry *next;
};
#define NHSHENT HASHENT

typedef struct hashtable HASHTAB;
struct hashtable
{
    int hashsize;
    int mask;
    int checks;
    int scans;
    int max_scan;
    int hits;
    int entries;
    int deletes;
    int nulls;
    int flags;
    HASHENT **entry;
    int last_hval;       /* Used for hashfirst & hashnext. */
    HASHENT *last_entry; /* like last_hval */
};
#define NHSHTAB HASHTAB

typedef struct mod_hashes MODHASHES;
struct mod_hashes
{
    char *tabname;
    HASHTAB *htab;
    int size_factor;
    int min_size;
};
#define MODNHASHES MODHASHES

/**
 * Definititon of a name table
 */
typedef struct name_table
{
    char *name; //!< Name of the entry
    int minlen; //!< Minimum length of the entry
    int perm;   //!< Permissions
    int flag;   //!< Flags
} NAMETAB;

/* Hash entry flags */

#define HASH_ALIAS 0x00000001 /* This entry is just a copy */

/* Hash table flags */

#define HT_STR 0x00000000      /* String-keyed hashtable */
#define HT_NUM 0x00000001      /* Numeric-keyed hashtable */
#define HT_TYPEMASK 0x0000000f /* Reserve up to 16 key types */
#define HT_KEYREF 0x00000010   /* Store keys by reference not copy */

#define nhashinit(h, sz) hashinit((h), (sz), HT_NUM)
#define nhashreset(h) hashreset((h))
#define hashfind(s, h) hashfind_generic((HASHKEY)(s), (h))
#define nhashfind(n, h) hashfind_generic((HASHKEY)(n), (h))
#define hashfindflags(s, h) hashfindflags_generic((HASHKEY)(s), (h))
#define hashadd(s, d, h, f) hashadd_generic((HASHKEY)(s), (d), (h), (f))
#define nhashadd(n, d, h) hashadd_generic((HASHKEY)(n), (d), (h), 0)
#define hashdelete(s, h) hashdelete_generic((HASHKEY)(s), (h))
#define nhashdelete(n, h) hashdelete_generic((HASHKEY)(n), (h))
#define nhashflush(h, sz) hashflush((h), (sz))
#define hashrepl(s, d, h) hashrepl_generic((HASHKEY)(s), (d), (h))
#define nhashrepl(n, d, h) hashrepl_generic((HASHKEY)(n), (d), (h))
#define nhashinfo(t, h) hashinfo((t), (h))
#define hash_firstkey(h) (hash_firstkey_generic((h)).s)
#define hash_nextkey(h) (hash_nextkey_generic((h)).s)
#define nhashresize(h, sz) hashresize((h), (sz))

extern NAMETAB powers_nametab[];

#endif /* __HTAB_H */
