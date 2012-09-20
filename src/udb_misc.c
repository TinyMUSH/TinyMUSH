/* udb_misc.c - Misc support routines for unter style error management. */

/*
 * Stolen from mjr.
 *
 * Andrew Molitor, amolitor@eagle.wesleyan.edu
 */

#include "copyright.h"
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

/*
 * Log database errors
 */

void
log_db_err(obj, attr, txt)
int obj, attr;

const char *txt;
{
    if (!mudstate.standalone)
    {
        STARTLOG(LOG_ALWAYS, "DBM", "ERROR")
        log_printf("Could not %s object #%d", txt, obj);
        if (attr != NOTHING)
        {
            log_printf(" attr #%d", attr);
        }
        ENDLOG
    }
    else
    {
        mainlog_printf("Could not %s object #%d", txt, obj);
        if (attr != NOTHING)
        {
            mainlog_printf(" attr #%d", attr);
        }
        mainlog_printf("\n");
    }
}

/*
 * print a series of warnings - do not exit
 */
/*
 * VARARGS
 */
#if defined(__STDC__) && defined(STDC_HEADERS)
void
warning(char *p, ...)
#else
void
warning(va_alist)
va_dcl
#endif
{
    va_list ap;

#if defined(__STDC__) && defined(STDC_HEADERS)
    va_start(ap, p);
#else
    char *p;

    va_start(ap);
    p = va_arg(ap, char *);

#endif
    while (1)
    {
        if (p == (char *)0)
            break;

        if (p == (char *)-1)
            p = (char *)strerror(errno);

        (void)mainlog_printf("%s", p);
        p = va_arg(ap, char *);
    }
    va_end(ap);
}

/*
 * print a series of warnings - exit
 */
/*
 * VARARGS
 */
#if defined(__STDC__) && defined(STDC_HEADERS)
void
fatal(char *p, ...)
#else
void
fatal(va_alist)
va_dcl
#endif
{
    va_list ap;

#if defined(__STDC__) && defined(STDC_HEADERS)
    va_start(ap, p);
#else
    char *p;

    va_start(ap);
    p = va_arg(ap, char *);

#endif
    while (1)
    {
        if (p == (char *)0)
            break;

        if (p == (char *)-1)
            p = (char *)strerror(errno);

        (void)mainlog_printf("%s", p);
        p = va_arg(ap, char *);
    }
    va_end(ap);
    exit(1);
}
