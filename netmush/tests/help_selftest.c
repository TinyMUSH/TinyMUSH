#define HELP_SELFTEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

/* Minimal allocator stubs used by macros.h */
void *__xmalloc(size_t s, const char *file, int line, const char *func, const char *var)
{
    (void)file;
    (void)line;
    (void)func;
    (void)var;
    void *p = malloc(s);
    if (!p)
    {
        perror("malloc");
        exit(1);
    }
    return p;
}

void *__xcalloc(size_t n, size_t s, const char *file, int line, const char *func, const char *var)
{
    (void)file;
    (void)line;
    (void)func;
    (void)var;
    void *p = calloc(n, s);
    if (!p)
    {
        perror("calloc");
        exit(1);
    }
    return p;
}

void *__xrealloc(void *p, size_t s, const char *file, int line, const char *func, const char *var)
{
    (void)file;
    (void)line;
    (void)func;
    (void)var;
    void *np = realloc(p, s);
    if (!np)
    {
        perror("realloc");
        exit(1);
    }
    return np;
}

int __xfree(void *p)
{
    free(p);
    return 0;
}

char *__xasprintf(const char *file, int line, const char *func, const char *var, const char *fmt, ...)
{
    (void)file;
    (void)line;
    (void)func;
    (void)var;
    char *out = NULL;
    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&out, fmt, ap) < 0)
    {
        perror("vasprintf");
        exit(1);
    }
    va_end(ap);
    return out;
}

int __xsprintf(char *s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int rc = vsprintf(s, fmt, ap);
    va_end(ap);
    return rc;
}

void *__xmemset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

/* Logging stubs */
void cf_log(dbref player, const char *type, const char *action, char *cmd, const char *fmt, ...)
{
    (void)player;
    (void)type;
    (void)action;
    (void)cmd;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[cf_log] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

#undef log_write
void _log_write(const char *file, int line, int key, const char *primary, const char *secondary, const char *fmt, ...)
{
    (void)file;
    (void)line;
    (void)key;
    (void)primary;
    (void)secondary;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[log] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

extern int g_help_test_fail_after;
extern int g_help_test_call;
int helpmkindx(dbref player, char *confcmd, char *helpfile);

static int write_file(const char *path, const char *content)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;
    fputs(content, fp);
    return fclose(fp);
}

static int file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

static int test_success(const char *dir)
{
    char base[512];
    char src[512];
    char dst[512];
    snprintf(base, sizeof(base), "%s/success", dir);
    size_t blen = strlen(base);
    if (blen + 4 >= sizeof(src) || blen + 5 >= sizeof(dst))
        return -1;
    memcpy(src, base, blen);
    memcpy(src + blen, ".txt", 5);
    memcpy(dst, base, blen);
    memcpy(dst + blen, ".indx", 6);
    if (write_file(src, "&foo\nline\n&bar\nnext\n") != 0)
        return -1;
    g_help_test_fail_after = 0;
    g_help_test_call = 0;
    if (helpmkindx(0, "test", base) != 0)
        return -1;
    return file_exists(dst) ? 0 : -1;
}

static int test_dump_failure(const char *dir)
{
    char base[512];
    char src[512];
    snprintf(base, sizeof(base), "%s/fail", dir);
    size_t blen = strlen(base);
    if (blen + 4 >= sizeof(src))
        return -1;
    memcpy(src, base, blen);
    memcpy(src + blen, ".txt", 5);
    if (write_file(src, "&foo\nline\n&bar\nnext\n") != 0)
        return -1;
    g_help_test_fail_after = 1; /* fail on first dump */
    g_help_test_call = 0;
    return helpmkindx(0, "test", base) == -1 ? 0 : -1;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    char tmpl[] = "/tmp/helpstXXXXXX";
    char *dir = mkdtemp(tmpl);
    if (!dir)
    {
        perror("mkdtemp");
        return 1;
    }

    int rc = 0;
    if (test_success(dir) != 0)
    {
        fprintf(stderr, "selftest: success case failed\n");
        rc = 1;
    }
    else if (test_dump_failure(dir) != 0)
    {
        fprintf(stderr, "selftest: dump failure case failed\n");
        rc = 1;
    }

    /* best-effort cleanup */
    rmdir(dir);
    return rc;
}
