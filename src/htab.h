/* htab.h - Structures and declarations needed for table hashing */

#include "copyright.h"

#ifndef __HTAB_H
#define __HTAB_H

typedef union {
    char *s;
    int i;
} HASHKEY;

typedef struct hashentry HASHENT;
struct hashentry {
    HASHKEY target;
    int *data;
    int flags;
    struct hashentry *next;
};
#define NHSHENT HASHENT

typedef struct hashtable HASHTAB;
struct hashtable {
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
    int last_hval;		/* Used for hashfirst & hashnext. */
    HASHENT *last_entry;	/* like last_hval */
};
#define NHSHTAB HASHTAB

typedef struct mod_hashes MODHASHES;
struct mod_hashes {
    char *tabname;
    HASHTAB *htab;
    int size_factor;
    int min_size;
};
#define MODNHASHES MODHASHES


/**
 * Definititon of a name table
 */
typedef struct name_table {
    char *name;			//!< Name of the entry
    int minlen;			//!< Minimum length of the entry
    int perm;			//!< Permissions
    int flag;			//!< Flags
} NAMETAB;

/* Hash entry flags */

#define HASH_ALIAS  0x00000001	/* This entry is just a copy */

/* Hash table flags */

#define HT_STR      0x00000000	/* String-keyed hashtable */
#define HT_NUM      0x00000001	/* Numeric-keyed hashtable */
#define HT_TYPEMASK 0x0000000f	/* Reserve up to 16 key types */
#define HT_KEYREF   0x00000010	/* Store keys by reference not copy */

extern void hashinit(HASHTAB *, int, int);
#define nhashinit(h,sz)     hashinit((h), (sz), HT_NUM)
extern void hashreset(HASHTAB *);
#define nhashreset(h)       hashreset((h))
extern int hashval(char *, int);
extern int get_hashmask(int *);
extern int *hashfind_generic(HASHKEY, HASHTAB *);
#define hashfind(s,h)       hashfind_generic((HASHKEY) (s), (h))
#define nhashfind(n,h)      hashfind_generic((HASHKEY) (n), (h))
extern int hashfindflags_generic(HASHKEY, HASHTAB *);
#define hashfindflags(s,h)  hashfindflags_generic((HASHKEY) (s), (h))
extern int hashadd_generic(HASHKEY, int *, HASHTAB *, int);
#define hashadd(s,d,h,f)    hashadd_generic((HASHKEY) (s), (d), (h), (f))
#define nhashadd(n,d,h)     hashadd_generic((HASHKEY) (n), (d), (h), 0)
extern void hashdelete_generic(HASHKEY, HASHTAB *);
#define hashdelete(s,h)     hashdelete_generic((HASHKEY) (s), (h))
#define nhashdelete(n,h)    hashdelete_generic((HASHKEY) (n), (h))
extern void hashdelall(int *, HASHTAB *);
extern void hashflush(HASHTAB *, int);
#define nhashflush(h,sz)    hashflush((h), (sz))
extern int hashrepl_generic(HASHKEY, int *, HASHTAB *);
#define hashrepl(s,d,h)     hashrepl_generic((HASHKEY) (s), (d), (h))
#define nhashrepl(n,d,h)    hashrepl_generic((HASHKEY) (n), (d), (h))
extern void hashreplall(int *, int *, HASHTAB *);
extern char *hashinfo(const char *, HASHTAB *);
#define nhashinfo(t,h)      hashinfo((t), (h))
extern int *hash_nextentry(HASHTAB * htab);
extern int *hash_firstentry(HASHTAB * htab);
extern HASHKEY hash_firstkey_generic(HASHTAB * htab);
#define hash_firstkey(h)    (hash_firstkey_generic((h)).s)
extern HASHKEY hash_nextkey_generic(HASHTAB * htab);
#define hash_nextkey(h)     (hash_nextkey_generic((h)).s)
extern void hashresize(HASHTAB *, int);
#define nhashresize(h,sz)   hashresize((h), (sz))

extern int search_nametab(dbref, NAMETAB *, char *);
extern NAMETAB *find_nametab_ent(dbref, NAMETAB *, char *);
extern NAMETAB *find_nametab_ent_flag(dbref, NAMETAB *, int);
extern void display_nametab(dbref, NAMETAB *, char *, int);
extern void interp_nametab(dbref, NAMETAB *, int, char *, char *, char *);
extern void listset_nametab(dbref, NAMETAB *, int, char *, int);

extern NAMETAB powers_nametab[];

#endif				/* __HTAB_H */
