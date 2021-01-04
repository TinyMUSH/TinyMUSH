/**
 * @file udb.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * @copyright Copyright (c) 1990 The Regents of the University of California.
 * 
 */

/* udb.h */

#include "copyright.h"

#ifndef __UDB_H
#define __UDB_H

/**
 * 
 * This code is derived from software contributed to Berkeley by Margo Seltzer.
 * 
 * Copyright (c) 1990 The Regents of the University of California. All rights
 * reserved.
 *
  * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/** 
 * Define the number of objects we may be reading/writing to at one time 
 * 
 */
#define NUM_OBJPIPES 64

/** 
 * For MUSH, an int works great as an object ID And attributes are zero
 * terminated strings, so we heave the size out. We hand around attribute
 * identifiers in the last things.
 * 
 */
typedef struct udb_aname
{
    unsigned int object;
    unsigned int attrnum;
} UDB_ANAME;

/** 
 * In general, we want binary attributes, so we do this. 
 * 
 */
typedef struct udb_attrib
{
    int attrnum; /*!< MUSH specific identifier */
    int size;
    char *data;
} UDB_ATTRIB;

/** 
 * An object is a name, an attribute count, and a vector of attributes which
 * Attr's are stowed in a contiguous array pointed at by atrs.
 * 
 */
typedef struct udb_object
{
    unsigned int name;
    time_t counter;
    int dirty;
    int at_count;
    UDB_ATTRIB *atrs;
} UDB_OBJECT;

typedef struct udb_cache
{
    void *keydata;
    int keylen;
    void *data;
    int datalen;
    unsigned int type;
    unsigned int flags;
    struct udb_cache *nxt;
    struct udb_cache *prvfree;
    struct udb_cache *nxtfree;
} UDB_CACHE;

typedef struct udb_chain
{
    UDB_CACHE *head;
    UDB_CACHE *tail;
} UDB_CHAIN;

typedef struct udb_data
{
    void *dptr;
    int dsize;
} UDB_DATA;

/** 
 * Cache flags 
 * 
 */
#define CACHE_DIRTY 0x00000001

#endif /* __UDB_H */
