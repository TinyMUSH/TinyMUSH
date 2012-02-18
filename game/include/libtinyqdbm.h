/*************************************************************************************************
 * This file is part of QDBM, Quick Database Manager.  Copyright (C) 2000-2007 Mikio Hirabayashi *
 *  QDBM is free software;  you can redistribute it and/or modify it under the terms of the GNU  *
 *  Lesser General Public License as published by the Free Software Foundation;  either version  *
 *   2.1 of the License or any later version.  QDBM is distributed in the hope that it will be   *
 *   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or   *
 *     FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more     *
 *                                            details.                                           *
 * You should have received a copy of the GNU Lesser General Public License along with QDBM;  if *
 *   not, write to the Free Software Foundation,  Inc., 59 Temple Place, Suite 330, Boston, MA   *
 *                                        02111-1307 USA.                                        *
 *************************************************************************************************/

#ifndef __DEPOT_H                         /* duplication check */
#define __DEPOT_H

/*
 * The basic API of QDBM
 */

#if defined(__cplusplus)                 /* export for C++ */
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(QDBM_INTERNAL) && !defined(QDBM_STATIC)
#define MYEXTERN extern __declspec(dllimport)
#else
#define MYEXTERN extern
#endif



/*************************************************************************************************
 * API
 *************************************************************************************************/


typedef struct {                         /* type of structure for a database handle */
  char *name;                            /* name of the database file */
  int wmode;                             /* whether to be writable */
  int inode;                             /* inode of the database file */
  time_t mtime;                          /* last modified time of the database */
  int fd;                                /* file descriptor of the database file */
  int fsiz;                              /* size of the database file */
  char *map;                             /* pointer to the mapped memory */
  int msiz;                              /* size of the mapped memory */
  int *buckets;                          /* pointer to the bucket array */
  int bnum;                              /* number of the bucket array */
  int rnum;                              /* number of records */
  int fatal;                             /* whether a fatal error occured */
  int ioff;                              /* offset of the iterator */
  int *fbpool;                           /* free block pool */
  int fbpsiz;                            /* size of the free block pool */
  int fbpinc;                            /* incrementor of update of the free block pool */
  int align;                             /* basic size of alignment */
} DEPOT;

enum {                                   /* enumeration for error codes */
  DP_ENOERR,                             /* no error */
  DP_EFATAL,                             /* with fatal error */
  DP_EMODE,                              /* invalid mode */
  DP_EBROKEN,                            /* broken database file */
  DP_EKEEP,                              /* existing record */
  DP_ENOITEM,                            /* no item found */
  DP_EALLOC,                             /* memory allocation error */
  DP_EMAP,                               /* memory mapping error */
  DP_EOPEN,                              /* open error */
  DP_ECLOSE,                             /* close error */
  DP_ETRUNC,                             /* trunc error */
  DP_ESYNC,                              /* sync error */
  DP_ESTAT,                              /* stat error */
  DP_ESEEK,                              /* seek error */
  DP_EREAD,                              /* read error */
  DP_EWRITE,                             /* write error */
  DP_ELOCK,                              /* lock error */
  DP_EUNLINK,                            /* unlink error */
  DP_EMKDIR,                             /* mkdir error */
  DP_ERMDIR,                             /* rmdir error */
  DP_EMISC                               /* miscellaneous error */
};

enum {                                   /* enumeration for open modes */
  DP_OREADER = 1 << 0,                   /* open as a reader */
  DP_OWRITER = 1 << 1,                   /* open as a writer */
  DP_OCREAT = 1 << 2,                    /* a writer creating */
  DP_OTRUNC = 1 << 3,                    /* a writer truncating */
  DP_ONOLCK = 1 << 4,                    /* open without locking */
  DP_OLCKNB = 1 << 5,                    /* lock without blocking */
  DP_OSPARSE = 1 << 6                    /* create as a sparse file */
};

enum {                                   /* enumeration for write modes */
  DP_DOVER,                              /* overwrite an existing value */
  DP_DKEEP,                              /* keep an existing value */
  DP_DCAT                                /* concatenate values */
};


/* String containing the version information. */
MYEXTERN const char *dpversion;


/* Last happened error code. */
#define dpecode        (*dpecodeptr())


/* Get a message string corresponding to an error code.
   `ecode' specifies an error code.
   The return value is the message string of the error code. The region of the return value
   is not writable. */
const char *dperrmsg(int ecode);


/* Get a database handle.
   `name' specifies the name of a database file.
   `omode' specifies the connection mode: `DP_OWRITER' as a writer, `DP_OREADER' as a reader.
   If the mode is `DP_OWRITER', the following may be added by bitwise or: `DP_OCREAT', which
   means it creates a new database if not exist, `DP_OTRUNC', which means it creates a new
   database regardless if one exists.  Both of `DP_OREADER' and `DP_OWRITER' can be added to by
   bitwise or: `DP_ONOLCK', which means it opens a database file without file locking, or
   `DP_OLCKNB', which means locking is performed without blocking.  `DP_OCREAT' can be added to
   by bitwise or: `DP_OSPARSE', which means it creates a database file as a sparse file.
   `bnum' specifies the number of elements of the bucket array.  If it is not more than 0,
   the default value is specified.  The size of a bucket array is determined on creating,
   and can not be changed except for by optimization of the database.  Suggested size of a
   bucket array is about from 0.5 to 4 times of the number of all records to store.
   The return value is the database handle or `NULL' if it is not successful.
   While connecting as a writer, an exclusive lock is invoked to the database file.
   While connecting as a reader, a shared lock is invoked to the database file.  The thread
   blocks until the lock is achieved.  If `DP_ONOLCK' is used, the application is responsible
   for exclusion control. */
DEPOT *dpopen(const char *name, int omode, int bnum);


/* Close a database handle.
   `depot' specifies a database handle.
   If successful, the return value is true, else, it is false.
   Because the region of a closed handle is released, it becomes impossible to use the handle.
   Updating a database is assured to be written when the handle is closed.  If a writer opens
   a database but does not close it appropriately, the database will be broken. */
int dpclose(DEPOT *depot);


/* Store a record.
   `depot' specifies a database handle connected as a writer.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `vbuf' specifies the pointer to the region of a value.
   `vsiz' specifies the size of the region of the value.  If it is negative, the size is
   assigned with `strlen(vbuf)'.
   `dmode' specifies behavior when the key overlaps, by the following values: `DP_DOVER',
   which means the specified value overwrites the existing one, `DP_DKEEP', which means the
   existing value is kept, `DP_DCAT', which means the specified value is concatenated at the
   end of the existing value.
   If successful, the return value is true, else, it is false. */
int dpput(DEPOT *depot, const char *kbuf, int ksiz, const char *vbuf, int vsiz, int dmode);


/* Delete a record.
   `depot' specifies a database handle connected as a writer.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is true, else, it is false.  False is returned when no
   record corresponds to the specified key. */
int dpout(DEPOT *depot, const char *kbuf, int ksiz);


/* Retrieve a record.
   `depot' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `start' specifies the offset address of the beginning of the region of the value to be read.
   `max' specifies the max size to be read.  If it is negative, the size to read is unlimited.
   `sp' specifies the pointer to a variable to which the size of the region of the return
   value is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record, else, it is `NULL'.  `NULL' is returned when no record corresponds to
   the specified key or the size of the value of the corresponding record is less than `start'.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if it
   is no longer in use. */
char *dpget(DEPOT *depot, const char *kbuf, int ksiz, int start, int max, int *sp);


/* Retrieve a record and write the value into a buffer.
   `depot' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `start' specifies the offset address of the beginning of the region of the value to be read.
   `max' specifies the max size to be read.  It shuld be equal to or less than the size of the
   writing buffer.
   `vbuf' specifies the pointer to a buffer into which the value of the corresponding record is
   written.
   If successful, the return value is the size of the written data, else, it is -1.  -1 is
   returned when no record corresponds to the specified key or the size of the value of the
   corresponding record is less than `start'.
   Note that no additional zero code is appended at the end of the region of the writing buffer. */
int dpgetwb(DEPOT *depot, const char *kbuf, int ksiz, int start, int max, char *vbuf);


/* Get the size of the value of a record.
   `depot' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1.
   Because this function does not read the entity of a record, it is faster than `dpget'. */
int dpvsiz(DEPOT *depot, const char *kbuf, int ksiz);


/* Initialize the iterator of a database handle.
   `depot' specifies a database handle.
   If successful, the return value is true, else, it is false.
   The iterator is used in order to access the key of every record stored in a database. */
int dpiterinit(DEPOT *depot);


/* Get the next key of the iterator.
   `depot' specifies a database handle.
   `sp' specifies the pointer to a variable to which the size of the region of the return
   value is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record is to be get out of the iterator.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if
   it is no longer in use.  It is possible to access every record by iteration of calling
   this function.  However, it is not assured if updating the database is occurred while the
   iteration.  Besides, the order of this traversal access method is arbitrary, so it is not
   assured that the order of storing matches the one of the traversal access. */
char *dpiternext(DEPOT *depot, int *sp);


/* Set alignment of a database handle.
   `depot' specifies a database handle connected as a writer.
   `align' specifies the size of alignment.
   If successful, the return value is true, else, it is false.
   If alignment is set to a database, the efficiency of overwriting values is improved.
   The size of alignment is suggested to be average size of the values of the records to be
   stored.  If alignment is positive, padding whose size is multiple number of the alignment
   is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
   is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
   saved in a database, you should specify alignment every opening a database. */
int dpsetalign(DEPOT *depot, int align);


/* Set the size of the free block pool of a database handle.
   `depot' specifies a database handle connected as a writer.
   `size' specifies the size of the free block pool of a database.
   If successful, the return value is true, else, it is false.
   The default size of the free block pool is 16.  If the size is greater, the space efficiency
   of overwriting values is improved with the time efficiency sacrificed. */
int dpsetfbpsiz(DEPOT *depot, int size);


/* Synchronize updating contents with the file and the device.
   `depot' specifies a database handle connected as a writer.
   If successful, the return value is true, else, it is false.
   This function is useful when another process uses the connected database file. */
int dpsync(DEPOT *depot);


/* Optimize a database.
   `depot' specifies a database handle connected as a writer.
   `bnum' specifies the number of the elements of the bucket array.  If it is not more than 0,
   the default value is specified.
   If successful, the return value is true, else, it is false.
   In an alternating succession of deleting and storing with overwrite or concatenate,
   dispensable regions accumulate.  This function is useful to do away with them. */
int dpoptimize(DEPOT *depot, int bnum);


/* Get the name of a database.
   `depot' specifies a database handle.
   If successful, the return value is the pointer to the region of the name of the database,
   else, it is `NULL'.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if it is no longer in use. */
char *dpname(DEPOT *depot);


/* Get the size of a database file.
   `depot' specifies a database handle.
   If successful, the return value is the size of the database file, else, it is -1. */
int dpfsiz(DEPOT *depot);


/* Get the number of the elements of the bucket array.
   `depot' specifies a database handle.
   If successful, the return value is the number of the elements of the bucket array, else, it
   is -1. */
int dpbnum(DEPOT *depot);


/* Get the number of the used elements of the bucket array.
   `depot' specifies a database handle.
   If successful, the return value is the number of the used elements of the bucket array,
   else, it is -1.
   This function is inefficient because it accesses all elements of the bucket array. */
int dpbusenum(DEPOT *depot);


/* Get the number of the records stored in a database.
   `depot' specifies a database handle.
   If successful, the return value is the number of the records stored in the database, else,
   it is -1. */
int dprnum(DEPOT *depot);


/* Check whether a database handle is a writer or not.
   `depot' specifies a database handle.
   The return value is true if the handle is a writer, false if not. */
int dpwritable(DEPOT *depot);


/* Check whether a database has a fatal error or not.
   `depot' specifies a database handle.
   The return value is true if the database has a fatal error, false if not. */
int dpfatalerror(DEPOT *depot);


/* Get the inode number of a database file.
   `depot' specifies a database handle.
   The return value is the inode number of the database file. */
int dpinode(DEPOT *depot);


/* Get the last modified time of a database.
   `depot' specifies a database handle.
   The return value is the last modified time of the database. */
time_t dpmtime(DEPOT *depot);


/* Get the file descriptor of a database file.
   `depot' specifies a database handle.
   The return value is the file descriptor of the database file.
   Handling the file descriptor of a database file directly is not suggested. */
int dpfdesc(DEPOT *depot);


/* Remove a database file.
   `name' specifies the name of a database file.
   If successful, the return value is true, else, it is false. */
int dpremove(const char *name);


/* Repair a broken database file.
   `name' specifies the name of a database file.
   If successful, the return value is true, else, it is false.
   There is no guarantee that all records in a repaired database file correspond to the original
   or expected state. */
int dprepair(const char *name);


/* Dump all records as endian independent data.
   `depot' specifies a database handle.
   `name' specifies the name of an output file.
   If successful, the return value is true, else, it is false. */
int dpexportdb(DEPOT *depot, const char *name);


/* Load all records from endian independent data.
   `depot' specifies a database handle connected as a writer.  The database of the handle must
   be empty.
   `name' specifies the name of an input file.
   If successful, the return value is true, else, it is false. */
int dpimportdb(DEPOT *depot, const char *name);


/* Retrieve a record directly from a database file.
   `name' specifies the name of a database file.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `sp' specifies the pointer to a variable to which the size of the region of the return
   value is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record, else, it is `NULL'.  `NULL' is returned when no record corresponds to
   the specified key.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if it
   is no longer in use.  Although this function can be used even while the database file is
   locked by another process, it is not assured that recent updated is reflected. */
char *dpsnaffle(const char *name, const char *kbuf, int ksiz, int *sp);


/* Hash function used inside Depot.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   The return value is the hash value of 31 bits length computed from the key.
   This function is useful when an application calculates the state of the inside bucket array. */
int dpinnerhash(const char *kbuf, int ksiz);


/* Hash function which is independent from the hash functions used inside Depot.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   The return value is the hash value of 31 bits length computed from the key.
   This function is useful when an application uses its own hash algorithm outside Depot. */
int dpouterhash(const char *kbuf, int ksiz);


/* Get a natural prime number not less than a number.
   `num' specified a natural number.
   The return value is a natural prime number not less than the specified number.
   This function is useful when an application determines the size of a bucket array of its
   own hash algorithm. */
int dpprimenum(int num);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


#define _QDBM_VERSION  "1.8.78"
#define _QDBM_LIBVER   1414


/* Name of the operating system. */
MYEXTERN const char *dpsysname;


/* File descriptor for debugging output. */
MYEXTERN int dpdbgfd;


/* Whether this build is reentrant. */
MYEXTERN const int dpisreentrant;


/* Set the last happened error code.
   `ecode' specifies the error code.
   `line' specifies the number of the line where the error happened. */
void dpecodeset(int ecode, const char *file, int line);


/* Get the pointer of the variable of the last happened error code.
   The return value is the pointer of the variable. */
int *dpecodeptr(void);


/* Synchronize updating contents on memory.
   `depot' specifies a database handle connected as a writer.
   If successful, the return value is true, else, it is false. */
int dpmemsync(DEPOT *depot);


/* Synchronize updating contents on memory, not physically.
   `depot' specifies a database handle connected as a writer.
   If successful, the return value is true, else, it is false. */
int dpmemflush(DEPOT *depot);


/* Get flags of a database.
   `depot' specifies a database handle.
   The return value is the flags of a database. */
int dpgetflags(DEPOT *depot);


/* Set flags of a database.
   `depot' specifies a database handle connected as a writer.
   `flags' specifies flags to set.  Least ten bits are reserved for internal use.
   If successful, the return value is true, else, it is false. */
int dpsetflags(DEPOT *depot, int flags);



#undef MYEXTERN

#if defined(__cplusplus)                 /* export for C++ */
}
#endif

#endif                                   /* __DEPOT_H */
#ifndef __CURIA_H                         /* duplication check */
#define __CURIA_H

/*
 * The extended API of QDBM
 */
 
#if defined(__cplusplus)                 /* export for C++ */
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(QDBM_INTERNAL) && !defined(QDBM_STATIC)
#define MYEXTERN extern __declspec(dllimport)
#else
#define MYEXTERN extern
#endif



/*************************************************************************************************
 * API
 *************************************************************************************************/


typedef struct {                         /* type of structure for the database handle */
  char *name;                            /* name of the database directory */
  int wmode;                             /* whether to be writable */
  int inode;                             /* inode of the database directory */
  DEPOT *attr;                           /* database handle for attributes */
  DEPOT **depots;                        /* handles of the record database */
  int dnum;                              /* number of record database handles */
  int inum;                              /* number of the database of the using iterator */
  int lrnum;                             /* number of large objects */
} CURIA;

enum {                                   /* enumeration for open modes */
  CR_OREADER = 1 << 0,                   /* open as a reader */
  CR_OWRITER = 1 << 1,                   /* open as a writer */
  CR_OCREAT = 1 << 2,                    /* a writer creating */
  CR_OTRUNC = 1 << 3,                    /* a writer truncating */
  CR_ONOLCK = 1 << 4,                    /* open without locking */
  CR_OLCKNB = 1 << 5,                    /* lock without blocking */
  CR_OSPARSE = 1 << 6                    /* create as sparse files */
};

enum {                                   /* enumeration for write modes */
  CR_DOVER,                              /* overwrite an existing value */
  CR_DKEEP,                              /* keep an existing value */
  CR_DCAT                                /* concatenate values */
};


/* Get a database handle.
   `name' specifies the name of a database directory.
   `omode' specifies the connection mode: `CR_OWRITER' as a writer, `CR_OREADER' as a reader.
   If the mode is `CR_OWRITER', the following may be added by bitwise or: `CR_OCREAT', which
   means it creates a new database if not exist, `CR_OTRUNC', which means it creates a new
   database regardless if one exists.  Both of `CR_OREADER' and `CR_OWRITER' can be added to by
   bitwise or: `CR_ONOLCK', which means it opens a database directory without file locking, or
   `CR_OLCKNB', which means locking is performed without blocking.  `CR_OCREAT' can be added to
   by bitwise or: `CR_OSPARSE', which means it creates database files as sparse files.
   `bnum' specifies the number of elements of each bucket array.  If it is not more than 0,
   the default value is specified.  The size of each bucket array is determined on creating,
   and can not be changed except for by optimization of the database.  Suggested size of each
   bucket array is about from 0.5 to 4 times of the number of all records to store.
   `dnum' specifies the number of division of the database.  If it is not more than 0, the
   default value is specified.  The number of division can not be changed from the initial value.
   The max number of division is 512.
   The return value is the database handle or `NULL' if it is not successful.
   While connecting as a writer, an exclusive lock is invoked to the database directory.
   While connecting as a reader, a shared lock is invoked to the database directory.
   The thread blocks until the lock is achieved.  If `CR_ONOLCK' is used, the application is
   responsible for exclusion control. */
CURIA *cropen(const char *name, int omode, int bnum, int dnum);


/* Close a database handle.
   `curia' specifies a database handle.
   If successful, the return value is true, else, it is false.
   Because the region of a closed handle is released, it becomes impossible to use the handle.
   Updating a database is assured to be written when the handle is closed.  If a writer opens
   a database but does not close it appropriately, the database will be broken. */
int crclose(CURIA *curia);


/* Store a record.
   `curia' specifies a database handle connected as a writer.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `vbuf' specifies the pointer to the region of a value.
   `vsiz' specifies the size of the region of the value.  If it is negative, the size is
   assigned with `strlen(vbuf)'.
   `dmode' specifies behavior when the key overlaps, by the following values: `CR_DOVER',
   which means the specified value overwrites the existing one, `CR_DKEEP', which means the
   existing value is kept, `CR_DCAT', which means the specified value is concatenated at the
   end of the existing value.
   If successful, the return value is true, else, it is false. */
int crput(CURIA *curia, const char *kbuf, int ksiz, const char *vbuf, int vsiz, int dmode);


/* Delete a record.
   `curia' specifies a database handle connected as a writer.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is true, else, it is false.  False is returned when no
   record corresponds to the specified key. */
int crout(CURIA *curia, const char *kbuf, int ksiz);


/* Retrieve a record.
   `curia' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `start' specifies the offset address of the beginning of the region of the value to be read.
   `max' specifies the max size to be read.  If it is negative, the size to read is unlimited.
   `sp' specifies the pointer to a variable to which the size of the region of the return value
   is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record, else, it is `NULL'.  `NULL' is returned when no record corresponds to
   the specified key or the size of the value of the corresponding record is less than `start'.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if it
   is no longer in use. */
char *crget(CURIA *curia, const char *kbuf, int ksiz, int start, int max, int *sp);


/* Retrieve a record and write the value into a buffer.
   `curia' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `start' specifies the offset address of the beginning of the region of the value to be read.
   `max' specifies the max size to be read.  It shuld be equal to or less than the size of the
   writing buffer.
   `vbuf' specifies the pointer to a buffer into which the value of the corresponding record is
   written.
   If successful, the return value is the size of the written data, else, it is -1.  -1 is
   returned when no record corresponds to the specified key or the size of the value of the
   corresponding record is less than `start'.
   Note that no additional zero code is appended at the end of the region of the writing buffer. */
int crgetwb(CURIA *curia, const char *kbuf, int ksiz, int start, int max, char *vbuf);


/* Get the size of the value of a record.
   `curia' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1.
   Because this function does not read the entity of a record, it is faster than `crget'. */
int crvsiz(CURIA *curia, const char *kbuf, int ksiz);


/* Initialize the iterator of a database handle.
   `curia' specifies a database handle.
   If successful, the return value is true, else, it is false.
   The iterator is used in order to access the key of every record stored in a database. */
int criterinit(CURIA *curia);


/* Get the next key of the iterator.
   `curia' specifies a database handle.
   `sp' specifies the pointer to a variable to which the size of the region of the return
   value is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record is to be get out of the iterator.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if it
   is no longer in use.  It is possible to access every record by iteration of calling this
   function.  However, it is not assured if updating the database is occurred while the
   iteration.  Besides, the order of this traversal access method is arbitrary, so it is not
   assured that the order of storing matches the one of the traversal access. */
char *criternext(CURIA *curia, int *sp);


/* Set alignment of a database handle.
   `curia' specifies a database handle connected as a writer.
   `align' specifies the size of alignment.
   If successful, the return value is true, else, it is false.
   If alignment is set to a database, the efficiency of overwriting values is improved.
   The size of alignment is suggested to be average size of the values of the records to be
   stored.  If alignment is positive, padding whose size is multiple number of the alignment
   is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
   is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
   saved in a database, you should specify alignment every opening a database. */
int crsetalign(CURIA *curia, int align);


/* Set the size of the free block pool of a database handle.
   `curia' specifies a database handle connected as a writer.
   `size' specifies the size of the free block pool of a database.
   If successful, the return value is true, else, it is false.
   The default size of the free block pool is 16.  If the size is greater, the space efficiency
   of overwriting values is improved with the time efficiency sacrificed. */
int crsetfbpsiz(CURIA *curia, int size);


/* Synchronize updating contents with the files and the devices.
   `curia' specifies a database handle connected as a writer.
   If successful, the return value is true, else, it is false.
   This function is useful when another process uses the connected database directory. */
int crsync(CURIA *curia);


/* Optimize a database.
   `curia' specifies a database handle connected as a writer.
   `bnum' specifies the number of the elements of each bucket array.  If it is not more than 0,
   the default value is specified.
   If successful, the return value is true, else, it is false.
   In an alternating succession of deleting and storing with overwrite or concatenate,
   dispensable regions accumulate.  This function is useful to do away with them. */
int croptimize(CURIA *curia, int bnum);

/* Get the name of a database.
   `curia' specifies a database handle.
   If successful, the return value is the pointer to the region of the name of the database,
   else, it is `NULL'.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if it is no longer in use. */
char *crname(CURIA *curia);


/* Get the total size of database files.
   `curia' specifies a database handle.
   If successful, the return value is the total size of the database files, else, it is -1.
   If the total size is more than 2GB, the return value overflows. */
int crfsiz(CURIA *curia);


/* Get the total size of database files as double-precision floating-point number.
   `curia' specifies a database handle.
   If successful, the return value is the total size of the database files, else, it is -1.0. */
double crfsizd(CURIA *curia);


/* Get the total number of the elements of each bucket array.
   `curia' specifies a database handle.
   If successful, the return value is the total number of the elements of each bucket array,
   else, it is -1. */
int crbnum(CURIA *curia);


/* Get the total number of the used elements of each bucket array.
   `curia' specifies a database handle.
   If successful, the return value is the total number of the used elements of each bucket
   array, else, it is -1.
   This function is inefficient because it accesses all elements of each bucket array. */
int crbusenum(CURIA *curia);


/* Get the number of the records stored in a database.
   `curia' specifies a database handle.
   If successful, the return value is the number of the records stored in the database, else,
   it is -1. */
int crrnum(CURIA *curia);


/* Check whether a database handle is a writer or not.
   `curia' specifies a database handle.
   The return value is true if the handle is a writer, false if not. */
int crwritable(CURIA *curia);


/* Check whether a database has a fatal error or not.
   `curia' specifies a database handle.
   The return value is true if the database has a fatal error, false if not. */
int crfatalerror(CURIA *curia);


/* Get the inode number of a database directory.
   `curia' specifies a database handle.
   The return value is the inode number of the database directory. */
int crinode(CURIA *curia);


/* Get the last modified time of a database.
   `curia' specifies a database handle.
   The return value is the last modified time of the database. */
time_t crmtime(CURIA *curia);


/* Remove a database directory.
   `name' specifies the name of a database directory.
   If successful, the return value is true, else, it is false. */
int crremove(const char *name);


/* Repair a broken database directory.
   `name' specifies the name of a database directory.
   If successful, the return value is true, else, it is false.
   There is no guarantee that all records in a repaired database directory correspond to the
   original or expected state. */
int crrepair(const char *name);


/* Dump all records as endian independent data.
   `curia' specifies a database handle.
   `name' specifies the name of an output directory.
   If successful, the return value is true, else, it is false.
   Note that large objects are ignored. */
int crexportdb(CURIA *curia, const char *name);


/* Load all records from endian independent data.
   `curia' specifies a database handle connected as a writer.  The database of the handle must
   be empty.
   `name' specifies the name of an input directory.
   If successful, the return value is true, else, it is false.
   Note that large objects are ignored. */
int crimportdb(CURIA *curia, const char *name);


/* Retrieve a record directly from a database directory.
   `name' specifies the name of a database directory.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `sp' specifies the pointer to a variable to which the size of the region of the return
   value is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record, else, it is `NULL'.  `NULL' is returned when no record corresponds to
   the specified key.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if it
   is no longer in use.  Although this function can be used even while the database directory is
   locked by another process, it is not assured that recent updated is reflected. */
char *crsnaffle(const char *name, const char *kbuf, int ksiz, int *sp);


/* Store a large object.
   `curia' specifies a database handle connected as a writer.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `vbuf' specifies the pointer to the region of a value.
   `vsiz' specifies the size of the region of the value.  If it is negative, the size is
   assigned with `strlen(vbuf)'.
   `dmode' specifies behavior when the key overlaps, by the following values: `CR_DOVER',
   which means the specified value overwrites the existing one, `CR_DKEEP', which means the
   existing value is kept, `CR_DCAT', which means the specified value is concatenated at the
   end of the existing value.
   If successful, the return value is true, else, it is false. */
int crputlob(CURIA *curia, const char *kbuf, int ksiz, const char *vbuf, int vsiz, int dmode);


/* Delete a large object.
   `curia' specifies a database handle connected as a writer.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is true, else, it is false.  false is returned when no large
   object corresponds to the specified key. */
int croutlob(CURIA *curia, const char *kbuf, int ksiz);


/* Retrieve a large object.
   `curia' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   `start' specifies the offset address of the beginning of the region of the value to be read.
   `max' specifies the max size to be read.  If it is negative, the size to read is unlimited.
   `sp' specifies the pointer to a variable to which the size of the region of the return value
   is assigned.  If it is `NULL', it is not used.
   If successful, the return value is the pointer to the region of the value of the
   corresponding large object, else, it is `NULL'.  `NULL' is returned when no large object
   corresponds to the specified key or the size of the value of the corresponding large object
   is less than `start'.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call if it
   is no longer in use. */
char *crgetlob(CURIA *curia, const char *kbuf, int ksiz, int start, int max, int *sp);


/* Get the file descriptor of a large object.
   `curia' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is the file descriptor of the corresponding large object,
   else, it is -1.  -1 is returned when no large object corresponds to the specified key.  The
   returned file descriptor is opened with the `open' call.  If the database handle was opened
   as a writer, the descriptor is writable (O_RDWR), else, it is not writable (O_RDONLY).  The
   descriptor should be closed with the `close' call if it is no longer in use. */
int crgetlobfd(CURIA *curia, const char *kbuf, int ksiz);


/* Get the size of the value of a large object.
   `curia' specifies a database handle.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.  If it is negative, the size is assigned
   with `strlen(kbuf)'.
   If successful, the return value is the size of the value of the corresponding large object,
   else, it is -1.
   Because this function does not read the entity of a large object, it is faster than
   `crgetlob'. */
int crvsizlob(CURIA *curia, const char *kbuf, int ksiz);


/* Get the number of the large objects stored in a database.
   `curia' specifies a database handle.
   If successful, the return value is the number of the large objects stored in the database,
   else, it is -1. */
int crrnumlob(CURIA *curia);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Synchronize updating contents on memory.
   `curia' specifies a database handle connected as a writer.
   If successful, the return value is true, else, it is false. */
int crmemsync(CURIA *curia);


/* Synchronize updating contents on memory, not physically.
   `curia' specifies a database handle connected as a writer.
   If successful, the return value is true, else, it is false. */
int crmemflush(CURIA *curia);


/* Get flags of a database.
   `curia' specifies a database handle.
   The return value is the flags of a database. */
int crgetflags(CURIA *curia);


/* Set flags of a database.
   `curia' specifies a database handle connected as a writer.
   `flags' specifies flags to set.  Least ten bits are reserved for internal use.
   If successful, the return value is true, else, it is false. */
int crsetflags(CURIA *curia, int flags);



#undef MYEXTERN

#if defined(__cplusplus)                 /* export for C++ */
}
#endif

#endif                                   /* __CURIA_H */
#ifndef __HOVEL_H                         /* duplication check */
#define __HOVEL_H

/*
 * The GDBM-compatible API of QDBM
 */

#if defined(__cplusplus)                 /* export for C++ */
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(QDBM_INTERNAL) && !defined(QDBM_STATIC)
#define MYEXTERN extern __declspec(dllimport)
#else
#define MYEXTERN extern
#endif



/*************************************************************************************************
 * API
 *************************************************************************************************/


enum {                                   /* enumeration for error codes */
  GDBM_NO_ERROR,                         /* no error */
  GDBM_MALLOC_ERROR,                     /* malloc error */
  GDBM_BLOCK_SIZE_ERROR,                 /* block size error */
  GDBM_FILE_OPEN_ERROR,                  /* file open error */
  GDBM_FILE_WRITE_ERROR,                 /* file write error */
  GDBM_FILE_SEEK_ERROR,                  /* file seek error */
  GDBM_FILE_READ_ERROR,                  /* file read error */
  GDBM_BAD_MAGIC_NUMBER,                 /* bad magic number */
  GDBM_EMPTY_DATABASE,                   /* empty database */
  GDBM_CANT_BE_READER,                   /* can't be reader */
  GDBM_CANT_BE_WRITER,                   /* can't be writer */
  GDBM_READER_CANT_DELETE,               /* reader can't delete */
  GDBM_READER_CANT_STORE,                /* reader can't store */
  GDBM_READER_CANT_REORGANIZE,           /* reader can't reorganize */
  GDBM_UNKNOWN_UPDATE,                   /* unknown update */
  GDBM_ITEM_NOT_FOUND,                   /* item not found */
  GDBM_REORGANIZE_FAILED,                /* reorganize failed */
  GDBM_CANNOT_REPLACE,                   /* cannot replace */
  GDBM_ILLEGAL_DATA,                     /* illegal data */
  GDBM_OPT_ALREADY_SET,                  /* option already set */
  GDBM_OPT_ILLEGAL                       /* option illegal */
};

typedef int gdbm_error;                  /* type of error codes */

typedef struct {                         /* type of structure for a database handle */
  DEPOT *depot;                          /* internal database handle of Depot */
  CURIA *curia;                          /* internal database handle of Curia */
  int syncmode;                          /* whether to be besyncronous mode */
} GDBM;

typedef GDBM *GDBM_FILE;                 /* type of pointer to a database handle */

typedef struct {                         /* type of structure for a key or a value */
  char *dptr;                            /* pointer to the region */
  size_t dsize;                          /* size of the region */
} datum;

enum {                                   /* enumeration for open modes */
  GDBM_READER = 1 << 0,                  /* open as a reader */
  GDBM_WRITER = 1 << 1,                  /* open as a writer */
  GDBM_WRCREAT = 1 << 2,                 /* a writer creating */
  GDBM_NEWDB = 1 << 3,                   /* a writer creating and truncating */
  GDBM_SYNC = 1 << 4,                    /* syncronous mode */
  GDBM_NOLOCK = 1 << 5,                  /* no lock mode */
  GDBM_LOCKNB = 1 << 6,                  /* non-blocking lock mode */
  GDBM_FAST = 1 << 7,                    /* fast mode */
  GDBM_SPARSE = 1 << 8                   /* create as sparse file */
};

enum {                                   /* enumeration for write modes */
  GDBM_INSERT,                           /* keep an existing value */
  GDBM_REPLACE                           /* overwrite an existing value */
};

enum {                                   /* enumeration for options */
  GDBM_CACHESIZE,                        /* set cache size */
  GDBM_FASTMODE,                         /* set fast mode */
  GDBM_SYNCMODE,                         /* set syncronous mode */
  GDBM_CENTFREE,                         /* set free block pool */
  GDBM_COALESCEBLKS                      /* set free block marging */
};


/* String containing the version information. */
MYEXTERN char *gdbm_version;


/* Last happened error code. */
#define gdbm_errno     (*gdbm_errnoptr())


/* Get a message string corresponding to an error code.
   `gdbmerrno' specifies an error code.
   The return value is the message string of the error code.  The region of the return value
   is not writable. */
char *gdbm_strerror(gdbm_error gdbmerrno);


/* Get a database handle after the fashion of GDBM.
   `name' specifies a name of a database.
   `read_write' specifies the connection mode: `GDBM_READER' as a reader, `GDBM_WRITER',
   `GDBM_WRCREAT' and `GDBM_NEWDB' as a writer.  `GDBM_WRCREAT' makes a database file or
   directory if it does not exist.  `GDBM_NEWDB' makes a new database even if it exists.
   You can add the following to writer modes by bitwise or: `GDBM_SYNC', `GDBM_NOLOCK',
   `GDBM_LCKNB', `GDBM_FAST', and `GDBM_SPARSE'.  `GDBM_SYNC' means a database is synchronized
   after every updating method.  `GDBM_NOLOCK' means a database is opened without file locking.
   `GDBM_LOCKNB' means file locking is performed without blocking.  `GDBM_FAST' is ignored.
   `GDBM_SPARSE' is an original mode of QDBM and makes database a sparse file.
   `mode' specifies a mode of a database file or a database directory as the one of `open'
   or `mkdir' call does.
   `bnum' specifies the number of elements of each bucket array.  If it is not more than 0,
   the default value is specified.
   `dnum' specifies the number of division of the database.  If it is not more than 0, the
   returning handle is created as a wrapper of Depot, else, it is as a wrapper of Curia.
   The return value is the database handle or `NULL' if it is not successful.
   If the database already exists, whether it is one of Depot or Curia is measured
   automatically. */
GDBM_FILE gdbm_open(char *name, int block_size, int read_write, int mode,
                    void (*fatal_func)(void));


/* Get a database handle after the fashion of QDBM.
   `name' specifies a name of a database.
   `read_write' specifies the connection mode: `GDBM_READER' as a reader, `GDBM_WRITER',
   `GDBM_WRCREAT' and `GDBM_NEWDB' as a writer.  `GDBM_WRCREAT' makes a database file or
   directory if it does not exist.  `GDBM_NEWDB' makes a new database even if it exists.
   You can add the following to writer modes by bitwise or: `GDBM_SYNC', `GDBM_NOLOCK',
   `GDBM_LOCKNB', `GDBM_FAST', and `GDBM_SPARSE'.  `GDBM_SYNC' means a database is synchronized
   after every updating method.  `GDBM_NOLOCK' means a database is opened without file locking.
   `GDBM_LOCKNB' means file locking is performed without blocking.  `GDBM_FAST' is ignored.
   `GDBM_SPARSE' is an original mode of QDBM and makes database sparse files.
   `mode' specifies a mode of a database file as the one of `open' or `mkdir' call does.
   `bnum' specifies the number of elements of each bucket array.  If it is not more than 0,
   the default value is specified.
   `dnum' specifies the number of division of the database.  If it is not more than 0, the
   returning handle is created as a wrapper of Depot, else, it is as a wrapper of Curia.
   `align' specifies the basic size of alignment.
   The return value is the database handle or `NULL' if it is not successful. */
GDBM_FILE gdbm_open2(char *name, int read_write, int mode, int bnum, int dnum, int align);


/* Close a database handle.
   `dbf' specifies a database handle.
   Because the region of the closed handle is released, it becomes impossible to use the
   handle. */
void gdbm_close(GDBM_FILE dbf);


/* Store a record.
   `dbf' specifies a database handle connected as a writer.
   `key' specifies a structure of a key.  `content' specifies a structure of a value.
   `flag' specifies behavior when the key overlaps, by the following values: `GDBM_REPLACE',
   which means the specified value overwrites the existing one, `GDBM_INSERT', which means
   the existing value is kept.
   The return value is 0 if it is successful, 1 if it gives up because of overlaps of the key,
   -1 if other error occurs. */
int gdbm_store(GDBM_FILE dbf, datum key, datum content, int flag);


/* Delete a record.
   `dbf' specifies a database handle connected as a writer.
   `key' specifies a structure of a key.
   The return value is 0 if it is successful, -1 if some errors occur. */
int gdbm_delete(GDBM_FILE dbf, datum key);


/* Retrieve a record.
   `dbf' specifies a database handle.
   `key' specifies a structure of a key.
   The return value is a structure of the result.
   If a record corresponds, the member `dptr' of the structure is the pointer to the region
   of the value.  If no record corresponds or some errors occur, `dptr' is `NULL'.  Because
   the region pointed to by `dptr' is allocated with the `malloc' call, it should be
   released with the `free' call if it is no longer in use. */
datum gdbm_fetch(GDBM_FILE dbf, datum key);


/* Check whether a record exists or not.
   `dbf' specifies a database handle.
   `key' specifies a structure of a key.
   The return value is true if a record corresponds and no error occurs, or false, else,
   it is false. */
int gdbm_exists(GDBM_FILE dbf, datum key);


/* Get the first key of a database.
   `dbf' specifies a database handle.
   The return value is a structure of the result.
   If a record corresponds, the member `dptr' of the structure is the pointer to the region
   of the first key.  If no record corresponds or some errors occur, `dptr' is `NULL'.
   Because the region pointed to by `dptr' is allocated with the `malloc' call, it should
   be released with the `free' call if it is no longer in use. */
datum gdbm_firstkey(GDBM_FILE dbf);


/* Get the next key of a database.
   `dbf' specifies a database handle.
   The return value is a structure of the result.
   If a record corresponds, the member `dptr' of the structure is the pointer to the region
   of the next key.  If no record corresponds or some errors occur, `dptr' is `NULL'.
   Because the region pointed to by `dptr' is allocated with the `malloc' call, it should
   be released with the `free' call if it is no longer in use. */
datum gdbm_nextkey(GDBM_FILE dbf, datum key);


/* Synchronize updating contents with the file and the device.
   `dbf' specifies a database handle connected as a writer. */
void gdbm_sync(GDBM_FILE dbf);


/* Reorganize a database.
   `dbf' specifies a database handle connected as a writer.
   If successful, the return value is 0, else -1. */
int gdbm_reorganize(GDBM_FILE dbf);


/* Get the file descriptor of a database file.
   `dbf' specifies a database handle connected as a writer.
   The return value is the file descriptor of the database file.
   If the database is a directory the return value is -1. */
int gdbm_fdesc(GDBM_FILE dbf);


/* No effect.
   `dbf' specifies a database handle.
   `option' is ignored.  `size' is ignored.
   The return value is 0.
   The function is only for compatibility. */
int gdbm_setopt(GDBM_FILE dbf, int option, int *value, int size);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Get the pointer of the last happened error code. */
int *gdbm_errnoptr(void);

/* Start with the constant definitions.  */
#define  TRUE    1
#define  FALSE   0

/* Parameters to gdbm_open. */
#define  GDBM_READER  0                 /* READERS only. */
#define  GDBM_WRITER  1                 /* READERS and WRITERS.  Can not
                                         * create. */
#define  GDBM_WRCREAT 2                 /* If not found, create the db. */
#define  GDBM_NEWDB   3                 /* ALWAYS create a new db.  (WRITER) */
#define  GDBM_OPENMASK 7                /* Mask for the above. */
#define  GDBM_FAST    0x10              /* Write fast! => No fsyncs.
                                         * OBSOLETE. */
#define  GDBM_SYNC    0x20              /* Sync operations to the disk. */
#define  GDBM_NOLOCK  0x40              /* Don't do file locking operations. */

/* Parameters to gdbm_store for simple insertion or replacement in the
   case a key to store is already in the database. */
#define  GDBM_INSERT  0                 /* Do not overwrite data in the
                                         * database. */
#define  GDBM_REPLACE 1                 /* Replace the old value with the new
                                         * value. */

/* Parameters to gdbm_setopt, specifing the type of operation to perform. */
#define  GDBM_CACHESIZE 1               /* Set the cache size. */
#define  GDBM_FASTMODE  2               /* Turn on or off fast mode.
                                         * OBSOLETE. */
#define  GDBM_SYNCMODE  3               /* Turn on or off sync operations. */
#define  GDBM_CENTFREE  4               /* Keep all free blocks in the header. */
#define  GDBM_COALESCEBLKS 5            /* Attempt to coalesce free blocks. */

/* In freeing blocks, we will ignore any blocks smaller (and equal) to
   IGNORE_SIZE number of bytes. */
#define IGNORE_SIZE		4

/* The number of key bytes kept in a hash bucket. */
#define SMALL			4

/* The number of bucket_avail entries in a hash bucket. */
#define BUCKET_AVAIL		6

/* The size of the bucket cache. */
#define DEFAULT_CACHESIZE	10

/* The dbm hash bucket element contains the full 31 bit hash value, the
   "pointer" to the key and data (stored together) with their sizes.  It also
   has a small part of the actual key value.  It is used to verify the first
   part of the key has the correct value without having to read the actual
   key. */

typedef struct {
        char    start_tag[4];
        int     hash_value;             /* The complete 31 bit value. */
        char    key_start[SMALL];       /* Up to the first SMALL bytes of the
                                         * key.  */
        off_t   data_pointer;           /* The file address of the key record.
                                         * The data record directly follows
                                         * the key.  */
        int     key_size;               /* Size of key data in the file. */
        int     data_size;              /* Size of associated data in the
                                         * file. */
}       bucket_element;

#undef MYEXTERN

#if defined(__cplusplus)                 /* export for C++ */
}
#endif

#endif                                   /* __HOVEL_H */
