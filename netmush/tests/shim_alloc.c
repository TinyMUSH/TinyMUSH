#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Minimal shim for tests: implements __xasprintf used by XASPRINTF macro.
// Ignores tracking and 'var' label, simply allocates and returns a formatted string.
char *__xasprintf(const char *file, int line, const char *function, const char *var, const char *format, ...)
{
    (void)file;
    (void)line;
    (void)function;
    (void)var;
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    if (n < 0)
        return NULL;
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf)
        return NULL;
    va_start(ap, format);
    vsnprintf(buf, (size_t)n + 1, format, ap);
    va_end(ap);
    return buf;
}
