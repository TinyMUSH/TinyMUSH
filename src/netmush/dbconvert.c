/**
 * @file dbconvert.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Database converter between GDBM and LMDB formats
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <gdbm.h>
#include <lmdb.h>
#include <sys/stat.h>

#define VERSION "1.0"

static void usage(const char *progname)
{
    fprintf(stderr, "TinyMUSH Database Converter v%s\n", VERSION);
    fprintf(stderr, "Usage: %s [options] <source> <destination>\n\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s gdbm|lmdb    Source database format (required)\n");
    fprintf(stderr, "  -d gdbm|lmdb    Destination database format (required)\n");
    fprintf(stderr, "  -h              Show this help message\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s -s gdbm -d lmdb game.gdbm game.lmdb\n", progname);
    fprintf(stderr, "  %s -s lmdb -d gdbm game.lmdb game.gdbm\n", progname);
    exit(1);
}

typedef struct {
    char *dptr;
    size_t dsize;
} db_data_t;

/* GDBM functions */
static GDBM_FILE gdbm_open_db(const char *filename, bool readonly)
{
    int flags = readonly ? GDBM_READER : GDBM_WRCREAT;
    GDBM_FILE dbf = gdbm_open((char *)filename, 0, flags, 0600, NULL);
    if (!dbf) {
        fprintf(stderr, "Error: Cannot open GDBM database '%s': %s\n", 
                filename, gdbm_strerror(gdbm_errno));
    }
    return dbf;
}

static int gdbm_iterate(GDBM_FILE dbf, int (*callback)(db_data_t key, db_data_t val, void *ctx), void *ctx)
{
    datum key, nextkey, val;
    db_data_t k, v;
    int count = 0;
    
    key = gdbm_firstkey(dbf);
    while (key.dptr != NULL) {
        val = gdbm_fetch(dbf, key);
        if (val.dptr) {
            k.dptr = key.dptr;
            k.dsize = key.dsize;
            v.dptr = val.dptr;
            v.dsize = val.dsize;
            
            if (callback(k, v, ctx) != 0) {
                free(val.dptr);
                free(key.dptr);
                return -1;
            }
            free(val.dptr);
            count++;
        }
        
        nextkey = gdbm_nextkey(dbf, key);
        free(key.dptr);
        key = nextkey;
    }
    
    return count;
}

/* LMDB functions */
static MDB_env *lmdb_open_env(const char *filename, bool readonly)
{
    MDB_env *env = NULL;
    int rc;
    struct stat st;
    
    /* Create directory if it doesn't exist and we're writing */
    if (!readonly && stat(filename, &st) != 0) {
        if (mkdir(filename, 0700) != 0) {
            fprintf(stderr, "Error: Cannot create LMDB directory '%s'\n", filename);
            return NULL;
        }
    }
    
    rc = mdb_env_create(&env);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot create LMDB environment: %s\n", mdb_strerror(rc));
        return NULL;
    }
    
    /* Set reasonable map size: 10 GB */
    rc = mdb_env_set_mapsize(env, 10UL * 1024UL * 1024UL * 1024UL);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot set LMDB mapsize: %s\n", mdb_strerror(rc));
        mdb_env_close(env);
        return NULL;
    }
    
    rc = mdb_env_set_maxdbs(env, 1);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot set LMDB maxdbs: %s\n", mdb_strerror(rc));
        mdb_env_close(env);
        return NULL;
    }
    
    unsigned int flags = MDB_NOSUBDIR;
    if (readonly) {
        flags |= MDB_RDONLY;
    }
    
    rc = mdb_env_open(env, filename, flags, 0600);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot open LMDB environment '%s': %s\n", 
                filename, mdb_strerror(rc));
        mdb_env_close(env);
        return NULL;
    }
    
    return env;
}

typedef struct {
    MDB_env *env;
    MDB_dbi dbi;
    MDB_txn *txn;
    int count;
    int batch_size;
} lmdb_ctx_t;

static int lmdb_write_callback(db_data_t key, db_data_t val, void *ctx)
{
    lmdb_ctx_t *lctx = (lmdb_ctx_t *)ctx;
    MDB_val k, v;
    int rc;
    
    k.mv_data = key.dptr;
    k.mv_size = key.dsize;
    v.mv_data = val.dptr;
    v.mv_size = val.dsize;
    
    rc = mdb_put(lctx->txn, lctx->dbi, &k, &v, 0);
    if (rc != 0) {
        fprintf(stderr, "Error: LMDB write failed: %s\n", mdb_strerror(rc));
        return -1;
    }
    
    lctx->count++;
    
    /* Commit every batch_size records */
    if (lctx->count % lctx->batch_size == 0) {
        rc = mdb_txn_commit(lctx->txn);
        if (rc != 0) {
            fprintf(stderr, "Error: LMDB commit failed: %s\n", mdb_strerror(rc));
            return -1;
        }
        
        /* Start new transaction */
        rc = mdb_txn_begin(lctx->env, NULL, 0, &lctx->txn);
        if (rc != 0) {
            fprintf(stderr, "Error: LMDB transaction begin failed: %s\n", mdb_strerror(rc));
            return -1;
        }
        
        printf("\rConverted %d records...", lctx->count);
        fflush(stdout);
    }
    
    return 0;
}

static int lmdb_iterate(MDB_env *env, MDB_dbi dbi, int (*callback)(db_data_t key, db_data_t val, void *ctx), void *ctx)
{
    MDB_txn *txn = NULL;
    MDB_cursor *cursor = NULL;
    MDB_val key, val;
    db_data_t k, v;
    int count = 0;
    int rc;
    
    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot begin LMDB transaction: %s\n", mdb_strerror(rc));
        return -1;
    }
    
    rc = mdb_cursor_open(txn, dbi, &cursor);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot open LMDB cursor: %s\n", mdb_strerror(rc));
        mdb_txn_abort(txn);
        return -1;
    }
    
    while ((rc = mdb_cursor_get(cursor, &key, &val, count == 0 ? MDB_FIRST : MDB_NEXT)) == 0) {
        k.dptr = key.mv_data;
        k.dsize = key.mv_size;
        v.dptr = val.mv_data;
        v.dsize = val.mv_size;
        
        if (callback(k, v, ctx) != 0) {
            mdb_cursor_close(cursor);
            mdb_txn_abort(txn);
            return -1;
        }
        count++;
    }
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    
    if (rc != MDB_NOTFOUND) {
        fprintf(stderr, "Error: LMDB iteration failed: %s\n", mdb_strerror(rc));
        return -1;
    }
    
    return count;
}

typedef struct {
    GDBM_FILE dbf;
    int count;
} gdbm_ctx_t;

static int gdbm_write_callback(db_data_t key, db_data_t val, void *ctx)
{
    gdbm_ctx_t *gctx = (gdbm_ctx_t *)ctx;
    datum k, v;
    int rc;
    
    k.dptr = key.dptr;
    k.dsize = (int)key.dsize;
    v.dptr = val.dptr;
    v.dsize = (int)val.dsize;
    
    rc = gdbm_store(gctx->dbf, k, v, GDBM_REPLACE);
    if (rc != 0) {
        fprintf(stderr, "Error: GDBM write failed: %s\n", gdbm_strerror(gdbm_errno));
        return -1;
    }
    
    gctx->count++;
    if (gctx->count % 1000 == 0) {
        printf("\rConverted %d records...", gctx->count);
        fflush(stdout);
    }
    
    return 0;
}

/* Conversion functions */
static int convert_gdbm_to_lmdb(const char *src, const char *dst)
{
    GDBM_FILE gdbf;
    MDB_env *lenv = NULL;
    MDB_dbi dbi = 0;
    lmdb_ctx_t lctx = {0};
    int rc;
    int count;
    
    printf("Converting GDBM to LMDB: %s -> %s\n", src, dst);
    
    /* Open GDBM source */
    gdbf = gdbm_open_db(src, true);
    if (!gdbf) {
        return 1;
    }
    
    /* Open LMDB destination */
    lenv = lmdb_open_env(dst, false);
    if (!lenv) {
        gdbm_close(gdbf);
        return 1;
    }
    
    /* Open LMDB database */
    MDB_txn *txn = NULL;
    rc = mdb_txn_begin(lenv, NULL, 0, &txn);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot begin LMDB transaction: %s\n", mdb_strerror(rc));
        gdbm_close(gdbf);
        mdb_env_close(lenv);
        return 1;
    }
    
    rc = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot open LMDB database: %s\n", mdb_strerror(rc));
        mdb_txn_abort(txn);
        gdbm_close(gdbf);
        mdb_env_close(lenv);
        return 1;
    }
    
    mdb_txn_commit(txn);
    
    /* Prepare context */
    lctx.env = lenv;
    lctx.dbi = dbi;
    lctx.count = 0;
    lctx.batch_size = 10000;  /* Commit every 10K records */
    
    /* Start first transaction */
    rc = mdb_txn_begin(lenv, NULL, 0, &lctx.txn);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot begin LMDB transaction: %s\n", mdb_strerror(rc));
        gdbm_close(gdbf);
        mdb_env_close(lenv);
        return 1;
    }
    
    /* Iterate and convert */
    count = gdbm_iterate(gdbf, lmdb_write_callback, &lctx);
    
    /* Commit remaining records */
    if (lctx.txn) {
        mdb_txn_commit(lctx.txn);
    }
    
    printf("\rConverted %d records successfully.\n", lctx.count);
    
    /* Cleanup */
    gdbm_close(gdbf);
    mdb_env_close(lenv);
    
    return (count < 0) ? 1 : 0;
}

static int convert_lmdb_to_gdbm(const char *src, const char *dst)
{
    MDB_env *lenv = NULL;
    MDB_dbi dbi = 0;
    GDBM_FILE gdbf;
    gdbm_ctx_t gctx = {0};
    int rc;
    int count;
    
    printf("Converting LMDB to GDBM: %s -> %s\n", src, dst);
    
    /* Open LMDB source */
    lenv = lmdb_open_env(src, true);
    if (!lenv) {
        return 1;
    }
    
    /* Open LMDB database */
    MDB_txn *txn = NULL;
    rc = mdb_txn_begin(lenv, NULL, MDB_RDONLY, &txn);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot begin LMDB transaction: %s\n", mdb_strerror(rc));
        mdb_env_close(lenv);
        return 1;
    }
    
    rc = mdb_dbi_open(txn, NULL, 0, &dbi);
    if (rc != 0) {
        fprintf(stderr, "Error: Cannot open LMDB database: %s\n", mdb_strerror(rc));
        mdb_txn_abort(txn);
        mdb_env_close(lenv);
        return 1;
    }
    mdb_txn_abort(txn);
    
    /* Open GDBM destination */
    gdbf = gdbm_open_db(dst, false);
    if (!gdbf) {
        mdb_env_close(lenv);
        return 1;
    }
    
    /* Prepare context */
    gctx.dbf = gdbf;
    gctx.count = 0;
    
    /* Iterate and convert */
    count = lmdb_iterate(lenv, dbi, gdbm_write_callback, &gctx);
    
    printf("\rConverted %d records successfully.\n", gctx.count);
    
    /* Cleanup */
    gdbm_close(gdbf);
    mdb_env_close(lenv);
    
    return (count < 0) ? 1 : 0;
}

int main(int argc, char **argv)
{
    char *src_format = NULL;
    char *dst_format = NULL;
    char *src_file = NULL;
    char *dst_file = NULL;
    int opt;
    
    /* Parse command line options */
    while ((opt = getopt(argc, argv, "s:d:h")) != -1) {
        switch (opt) {
            case 's':
                src_format = optarg;
                break;
            case 'd':
                dst_format = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }
    
    /* Check required options */
    if (!src_format || !dst_format) {
        fprintf(stderr, "Error: Source and destination formats are required\n\n");
        usage(argv[0]);
    }
    
    /* Get source and destination files */
    if (optind + 2 != argc) {
        fprintf(stderr, "Error: Source and destination files are required\n\n");
        usage(argv[0]);
    }
    
    src_file = argv[optind];
    dst_file = argv[optind + 1];
    
    /* Validate formats */
    if (strcmp(src_format, "gdbm") != 0 && strcmp(src_format, "lmdb") != 0) {
        fprintf(stderr, "Error: Invalid source format '%s' (must be gdbm or lmdb)\n", src_format);
        return 1;
    }
    
    if (strcmp(dst_format, "gdbm") != 0 && strcmp(dst_format, "lmdb") != 0) {
        fprintf(stderr, "Error: Invalid destination format '%s' (must be gdbm or lmdb)\n", dst_format);
        return 1;
    }
    
    if (strcmp(src_format, dst_format) == 0) {
        fprintf(stderr, "Error: Source and destination formats must be different\n");
        return 1;
    }
    
    /* Perform conversion */
    if (strcmp(src_format, "gdbm") == 0) {
        return convert_gdbm_to_lmdb(src_file, dst_file);
    } else {
        return convert_lmdb_to_gdbm(src_file, dst_file);
    }
}
