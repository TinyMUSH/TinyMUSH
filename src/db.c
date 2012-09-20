/* db.c - attribute interface, some flatfile and object routines */

#include "copyright.h"
#include "mushconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by interface */
#include "interface.h"	/* required by code */

#include "attrs.h"	/* required by code */
#include "vattr.h"	/* required by code */
#include "match.h"	/* required by code */
#include "powers.h"	/* required by code */
#include "udb.h"	/* required by code */
#include "ansi.h"	/* required by code */

#ifndef O_ACCMODE
#define O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif

/*
 * Restart definitions
 */
#define RS_CONCENTRATE		0x00000002
#define RS_RECORD_PLAYERS	0x00000004
#define RS_NEW_STRINGS		0x00000008
#define RS_COUNT_REBOOTS	0x00000010

OBJ *db = NULL;
NAME *names = NULL;
NAME *purenames = NULL;

extern int sock;
extern int ndescriptors;
extern int maxd;
extern int slave_socket;

extern pid_t slave_pid;
extern void FDECL(desc_addhash, (DESC *));

#ifdef TEST_MALLOC
int malloc_count = 0;
int malloc_bytes = 0;
char *malloc_ptr;
char *malloc_str;
#endif /* TEST_MALLOC */

extern VATTR *FDECL(vattr_rename, (char *, char *));

/* ---------------------------------------------------------------------------
 * Temp file management, used to get around static limits in some versions
 * of libc.
 */

FILE *t_fd;
int t_is_pipe;

#ifdef TLI
int t_is_tli;

#endif

static void tf_xclose(fd)
FILE *fd;
{
    if (fd) {
        if (t_is_pipe)
            pclose(fd);
#ifdef TLI
        else if (t_is_tli)
            t_close(fd);
#endif
        else
            fclose(fd);
    } else {
        close(0);
    }
    t_fd = NULL;
    t_is_pipe = 0;
}

static int tf_fiddle(tfd)
int tfd;
{
    if (tfd < 0) {
        tfd = open(DEV_NULL, O_RDONLY, 0);
        return -1;
    }
    if (tfd != 0) {
        dup2(tfd, 0);
        close(tfd);
    }
    return 0;
}

static int tf_xopen(fname, mode)
char *fname;
int mode;
{
    int fd;

    fd = open(fname, mode, 0600);
    fd = tf_fiddle(fd);
    return fd;
}

/* #define t_xopen(f,m) t_fiddle(open(f, m, 0600)) */

static const char *mode_txt(mode)
int mode;
{
    switch (mode & O_ACCMODE) {
    case O_RDONLY:
        return "r";
    case O_WRONLY:
        return "w";
    }
    return "r+";
}

void NDECL(tf_init)
{
    fclose(stdin);
    tf_xopen(DEV_NULL, O_RDONLY);
    t_fd = NULL;
    t_is_pipe = 0;
}

int tf_open(fname, mode)
char *fname;
int mode;
{
    tf_xclose(t_fd);
    return tf_xopen(fname, mode);
}

#ifdef TLI
int tf_topen(fam, mode)
int fam, mode;
{
    tf_xclose(t_fd);
    return tf_fiddle(t_open(fam, mode, NULL));
}

#endif

void tf_close(fdes)
int fdes;
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

FILE *tf_fopen(fname, mode)
char *fname;
int mode;
{
    tf_xclose(t_fd);
    if (tf_xopen(fname, mode) >= 0) {
        t_fd = fdopen(0, mode_txt(mode));
        return t_fd;
    }
    return NULL;
}

void tf_fclose(fd)
FILE *fd;
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

FILE *tf_popen(fname, mode)
char *fname;
int mode;
{
    tf_xclose(t_fd);
    t_fd = popen(fname, mode_txt(mode));
    if (t_fd != NULL) {
        t_is_pipe = 1;
    }
    return t_fd;
}

/* #define GNU_MALLOC_TEST 1 */

#ifdef GNU_MALLOC_TEST
extern unsigned int malloc_sbrk_used;	/* amount of data space used now */

#endif

/*
 * Check routine forward declaration.
 */
static int FDECL(fwdlist_ck, (int, dbref, dbref, int, char *));
static int FDECL(propdir_ck, (int, dbref, dbref, int, char *));

extern void FDECL(pcache_reload, (dbref));
extern void FDECL(desc_reload, (dbref));

/* *INDENT-OFF* */

/* List of built-in attributes */
ATTR attr[] =
{
    {"Aahear",	A_AAHEAR,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Aclone",	A_ACLONE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Aconnect",	A_ACONNECT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Adesc",	A_ADESC,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Adfail",	A_ADFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Adisconnect",	A_ADISCONNECT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Adrop",	A_ADROP,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Aefail",	A_AEFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Aenter",	A_AENTER,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Afail",	A_AFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Agfail",	A_AGFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Ahear",	A_AHEAR,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Akill",	A_AKILL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Aleave",	A_ALEAVE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Alfail",	A_ALFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {   "Alias",	A_ALIAS,	AF_NOPROG|AF_NOCMD|AF_NOCLONE|AF_PRIVATE|AF_CONST,
        NULL
    },
    {"Allowance",	A_ALLOWANCE,	AF_MDARK|AF_NOPROG|AF_WIZARD,	NULL},
    {"Amail",	A_AMAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Amhear",	A_AMHEAR,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Amove",	A_AMOVE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Apay",	A_APAY,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"Arfail",	A_ARFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Asucc",	A_ASUCC,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Atfail",	A_ATFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Atport",	A_ATPORT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Atofail",	A_ATOFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Aufail",	A_AUFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Ause",	A_AUSE,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"Away",	A_AWAY,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"Charges",	A_CHARGES,	AF_NOPROG,			NULL},
    {"ChownLock",	A_LCHOWN,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Comment",	A_COMMENT,	AF_NOPROG|AF_MDARK|AF_WIZARD,	NULL},
    {"Conformat",	A_LCON_FMT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"ControlLock",	A_LCONTROL,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Cost",	A_COST,		AF_NOPROG,			NULL},
    {"Daily",	A_DAILY,	AF_NOPROG,			NULL},
    {"DarkLock",	A_LDARK,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Desc",	A_DESC,		AF_DEFAULT|AF_VISUAL|AF_NOPROG,		NULL},
    {"DefaultLock",	A_LOCK,		AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Destroyer",	A_DESTROYER,	AF_MDARK|AF_WIZARD|AF_NOPROG,	NULL},
    {"Dfail",	A_DFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Drop",	A_DROP,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"DropLock",	A_LDROP,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Ealias",	A_EALIAS,	AF_NOPROG,			NULL},
    {"Efail",	A_EFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Enter",	A_ENTER,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"EnterLock",	A_LENTER,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Exitformat",	A_LEXITS_FMT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Exitto",	A_EXITVARDEST,	AF_NOPROG,			NULL},
    {"Fail",	A_FAIL,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"Filter",	A_FILTER,	AF_NOPROG,			NULL},
    {"Forwardlist",	A_FORWARDLIST,	AF_NOPROG,			fwdlist_ck},
    {"Gfail",	A_GFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"GiveLock",	A_LGIVE,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"HeardLock",	A_LHEARD,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"HearsLock",	A_LHEARS,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Idesc",	A_IDESC,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Idle",	A_IDLE,		AF_NOPROG,			NULL},
    {"Infilter",	A_INFILTER,	AF_NOPROG,			NULL},
    {"Inprefix",	A_INPREFIX,	AF_NOPROG,			NULL},
    {"Kill",	A_KILL,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"KnownLock",	A_LKNOWN,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"KnowsLock",	A_LKNOWS,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Lalias",	A_LALIAS,	AF_NOPROG,			NULL},
    {   "Last",	A_LAST,	AF_VISUAL|AF_WIZARD|AF_NOCMD|AF_NOPROG|AF_NOCLONE,
        NULL
    },
    {   "Lastip",	A_LASTIP,	AF_NOPROG|AF_NOCMD|AF_NOCLONE|AF_GOD,
        NULL
    },
    {   "Lastpage",	A_LASTPAGE,	AF_INTERNAL|AF_NOCMD|AF_NOPROG|AF_GOD|AF_PRIVATE,
        NULL
    },
    {   "Lastsite",	A_LASTSITE,	AF_NOPROG|AF_NOCMD|AF_NOCLONE|AF_GOD,
        NULL
    },
    {"Leave",	A_LEAVE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"LeaveLock",	A_LLEAVE,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Lfail",	A_LFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"LinkLock",	A_LLINK,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Listen",	A_LISTEN,	AF_NOPROG,			NULL},
    {   "Logindata",	A_LOGINDATA,	AF_MDARK|AF_NOPROG|AF_NOCMD|AF_CONST,
        NULL
    },
    {   "Mailcurf",	A_MAILCURF,	AF_MDARK|AF_WIZARD|AF_NOPROG|AF_NOCLONE,
        NULL
    },
    {   "Mailflags",	A_MAILFLAGS,	AF_MDARK|AF_WIZARD|AF_NOPROG|AF_NOCLONE,
        NULL
    },
    {   "Mailfolders",	A_MAILFOLDERS,	AF_MDARK|AF_WIZARD|AF_NOPROG|AF_NOCLONE,
        NULL
    },
    {   "Mailmsg",	A_MAILMSG,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {   "Mailsub",	A_MAILSUB,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {"Mailsucc",	A_MAIL,		AF_DEFAULT|AF_NOPROG,			NULL},
    {   "Mailto",	A_MAILTO,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {"MovedLock",	A_LMOVED,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"MovesLock",	A_LMOVES,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Move",	A_MOVE,		AF_DEFAULT|AF_NOPROG,			NULL},
    {   "Name",	A_NAME,		AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {"Nameformat",	A_NAME_FMT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {   "Newobjs",	A_NEWOBJS,	AF_MDARK|AF_NOPROG|AF_GOD|AF_NOCMD|AF_NOCLONE,
        NULL
    },
    {"Odesc",	A_ODESC,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Odfail",	A_ODFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Odrop",	A_ODROP,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oefail",	A_OEFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oenter",	A_OENTER,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Ofail",	A_OFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Ogfail",	A_OGFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Okill",	A_OKILL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oleave",	A_OLEAVE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Olfail",	A_OLFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Omove",	A_OMOVE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Opay",	A_OPAY,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"Orfail",	A_ORFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Osucc",	A_OSUCC,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Otfail",	A_OTFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Otport",	A_OTPORT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Otofail",	A_OTOFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oufail",	A_OUFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Ouse",	A_OUSE,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oxenter",	A_OXENTER,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oxleave",	A_OXLEAVE,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Oxtport",	A_OXTPORT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {   "Pagegroup",	A_PAGEGROUP,	AF_INTERNAL|AF_NOCMD|AF_NOPROG|AF_GOD|AF_PRIVATE,
        NULL
    },
    {"PageLock",	A_LPAGE,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"ParentLock",	A_LPARENT,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Pay",		A_PAY,		AF_NOPROG,			NULL},
    {"Prefix",	A_PREFIX,	AF_NOPROG,			NULL},
    {   "Progcmd",	A_PROGCMD,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {"Propdir",	A_PROPDIR,	AF_NOPROG,			propdir_ck},
    {"Queuemax",	A_QUEUEMAX,	AF_MDARK|AF_WIZARD|AF_NOPROG,	NULL},
    {   "Quota",	A_QUOTA,	AF_MDARK|AF_NOPROG|AF_GOD|AF_NOCMD|AF_NOCLONE,
        NULL
    },
    {"ReceiveLock",	A_LRECEIVE,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Reject",	A_REJECT,	AF_NOPROG,			NULL},
    {"Rfail",	A_RFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {   "Rquota",	A_RQUOTA,	AF_MDARK|AF_NOPROG|AF_GOD|AF_NOCMD|AF_NOCLONE,
        NULL
    },
    {"Runout",	A_RUNOUT,	AF_NOPROG,			NULL},
    {   "Semaphore",	A_SEMAPHORE,	AF_NOPROG|AF_WIZARD|AF_NOCMD|AF_NOCLONE,
        NULL
    },
    {"Sex",		A_SEX,	AF_VISUAL|AF_NOPROG,			NULL},
    {"Signature",	A_SIGNATURE,	AF_NOPROG,			NULL},
    {"Speechformat",A_SPEECHFMT,	AF_DEFAULT|AF_NOPROG,		NULL},
    {"SpeechLock",	A_LSPEECH,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Startup",	A_STARTUP,	AF_NOPROG,			NULL},
    {"Succ",	A_SUCC,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"TeloutLock",	A_LTELOUT,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Tfail",	A_TFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Timeout",	A_TIMEOUT,	AF_MDARK|AF_NOPROG|AF_WIZARD,	NULL},
    {"Tport",	A_TPORT,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"TportLock",	A_LTPORT,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Tofail",	A_TOFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Ufail",	A_UFAIL,	AF_DEFAULT|AF_NOPROG,			NULL},
    {"Use",		A_USE,		AF_DEFAULT|AF_NOPROG,			NULL},
    {"UseLock",	A_LUSE,		AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"UserLock",	A_LUSER,	AF_NOPROG|AF_NOCMD|AF_IS_LOCK,	NULL},
    {"Va",		A_VA,		0,				NULL},
    {"Vb",		A_VA+1,		0,				NULL},
    {"Vc",		A_VA+2,		0,				NULL},
    {"Vd",		A_VA+3,		0,				NULL},
    {"Ve",		A_VA+4,		0,				NULL},
    {"Vf",		A_VA+5,		0,				NULL},
    {"Vg",		A_VA+6,		0,				NULL},
    {"Vh",		A_VA+7,		0,				NULL},
    {"Vi",		A_VA+8,		0,				NULL},
    {"Vj",		A_VA+9,		0,				NULL},
    {"Vk",		A_VA+10,	0,				NULL},
    {"Vl",		A_VA+11,	0,				NULL},
    {"Vm",		A_VA+12,	0,				NULL},
    {"Vn",		A_VA+13,	0,				NULL},
    {"Vo",		A_VA+14,	0,				NULL},
    {"Vp",		A_VA+15,	0,				NULL},
    {"Vq",		A_VA+16,	0,				NULL},
    {"Vr",		A_VA+17,	0,				NULL},
    {"Vs",		A_VA+18,	0,				NULL},
    {"Vt",		A_VA+19,	0,				NULL},
    {"Vu",		A_VA+20,	0,				NULL},
    {"Vv",		A_VA+21,	0,				NULL},
    {"Vw",		A_VA+22,	0,				NULL},
    {"Vx",		A_VA+23,	0,				NULL},
    {"Vy",		A_VA+24,	0,				NULL},
    {"Vz",		A_VA+25,	0,				NULL},
    {"Vrml_url",	A_VRML_URL,	AF_NOPROG,			NULL},
    {"Htdesc",	A_HTDESC,	AF_DEFAULT|AF_VISUAL|AF_NOPROG,		NULL},
    {"*Atrlist",	A_LIST,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,	NULL},
    {"*Password",	A_PASS,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,	NULL},
    {   "*Money",	A_MONEY,	AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {   "*Invalid",	A_TEMP,		AF_DARK|AF_NOPROG|AF_NOCMD|AF_INTERNAL,
        NULL
    },
    {NULL,		0,		0,				NULL}
};

/* *INDENT-ON* */

/* ---------------------------------------------------------------------------
 * fwdlist_set, fwdlist_clr: Manage cached forwarding lists
 */

void fwdlist_set(thing, ifp)
dbref thing;
FWDLIST *ifp;
{
    FWDLIST *fp, *xfp;
    int i, stat = 0;

    /* If fwdlist is null, clear */

    if (!ifp || (ifp->count <= 0)) {
        fwdlist_clr(thing);
        return;
    }
    /* Copy input forwardlist to a correctly-sized buffer */

    fp = (FWDLIST *) XMALLOC(sizeof(FWDLIST), "fwdlist_set");
    fp->data = (int *) XCALLOC(ifp->count, sizeof(int),
                               "fwdlist_set.data");
    for (i = 0; i < ifp->count; i++) {
        fp->data[i] = ifp->data[i];
    }
    fp->count = ifp->count;

    /*
     * Replace an existing forwardlist, or add a new one
     */

    xfp = fwdlist_get(thing);
    if (xfp) {
        if (xfp->data)
            XFREE(xfp->data, "fwdlist_set.xfp_data");
        XFREE(xfp, "fwdlist_set.xfp");
        stat = nhashrepl(thing, (int *)fp, &mudstate.fwdlist_htab);
    } else {
        stat = nhashadd(thing, (int *)fp, &mudstate.fwdlist_htab);
    }
    if (stat < 0) {              /* the add or replace failed */
        if (fp->data)
            XFREE(fp->data, "fwdlist_set.fp_data");
        XFREE(fp, "fwdlist_set.fp");
    }
}

void fwdlist_clr(thing)
dbref thing;
{
    FWDLIST *xfp;

    /* If a forwardlist exists, delete it */

    xfp = fwdlist_get(thing);
    if (xfp) {
        if (xfp->data)
            XFREE(xfp->data, "fwdlist_clr.data");
        XFREE(xfp, "fwdlist_clr");
        nhashdelete(thing, &mudstate.fwdlist_htab);
    }
}

/* ---------------------------------------------------------------------------
 * fwdlist_load: Load text into a forwardlist.
 */

int fwdlist_load(fp, player, atext)
FWDLIST *fp;
dbref player;
char *atext;
{
    dbref target;
    char *tp, *bp, *dp;
    int i, count, errors, fail;
    int *tmp_array;

    tmp_array = (int *)XCALLOC((LBUF_SIZE / 2), sizeof(int),
                               "fwdlist_load.tmp");

    count = 0;
    errors = 0;
    bp = tp = alloc_lbuf("fwdlist_load.str");
    strcpy(tp, atext);
    do {
        for (; *bp && isspace(*bp); bp++) ;	/* skip spaces */
        for (dp = bp; *bp && !isspace(*bp); bp++) ;	/* remember string */
        if (*bp)
            *bp++ = '\0';	/* terminate string */
        if ((*dp++ == '#') && isdigit(*dp)) {
            target = atoi(dp);

            if (!mudstate.standalone) {
                fail = (!Good_obj(target) ||
                        (!God(player) &&
                         !controls(player, target) &&
                         (!Link_ok(target) ||
                          !could_doit(player, target, A_LLINK))));
            } else {
                fail = !Good_obj(target);
            }

            if (fail) {
                if (!mudstate.standalone)
                    notify(player,
                           tprintf("Cannot forward to #%d: Permission denied.",
                                   target));
                errors++;
            } else if (count < mudconf.fwdlist_lim) {
                if (fp->data)
                    fp->data[count++] = target;
                else
                    tmp_array[count++] = target;
            } else {
                if (!mudstate.standalone)
                    notify(player,
                           tprintf("Cannot forward to #%d: Forwardlist limit exceeded.",
                                   target));
                errors++;
            }
        }
    } while (*bp);

    free_lbuf(tp);

    if ((fp->data == NULL) && (count > 0)) {
        fp->data = (int *) XCALLOC(count, sizeof(int), "fwdlist_load.data");
        for (i = 0; i < count; i++)
            fp->data[i] = tmp_array[i];
    }

    fp->count = count;
    XFREE(tmp_array, "fwdlist_load.tmp");
    return errors;
}

/*
 * ---------------------------------------------------------------------------
 * fwdlist_rewrite: Generate a text string from a FWDLIST buffer.
 */

int fwdlist_rewrite(fp, atext)
FWDLIST *fp;
char *atext;
{
    char *tp, *bp;
    int i, count;

    if (fp && fp->count) {
        count = fp->count;
        tp = alloc_sbuf("fwdlist_rewrite.errors");
        bp = atext;
        for (i = 0; i < fp->count; i++) {
            if (Good_obj(fp->data[i])) {
                sprintf(tp, "#%d ", fp->data[i]);
                safe_str(tp, atext, &bp);
            } else {
                count--;
            }
        }
        *bp = '\0';
        free_sbuf(tp);
    } else {
        count = 0;
        *atext = '\0';
    }
    return count;
}

/* ---------------------------------------------------------------------------
 * fwdlist_ck:  Check a list of dbref numbers to forward to for AUDIBLE
 */

static int fwdlist_ck(key, player, thing, anum, atext)
int key, anum;
dbref player, thing;
char *atext;
{
    FWDLIST *fp;
    int count;

    if (mudstate.standalone)
        return 1;

    count = 0;

    if (atext && *atext) {
        fp = (FWDLIST *) XMALLOC(sizeof(FWDLIST), "fwdlist_ck.fp");
        fp->data = NULL;
        fwdlist_load(fp, player, atext);
    } else {
        fp = NULL;
    }

    /* Set the cached forwardlist */

    fwdlist_set(thing, fp);
    count = fwdlist_rewrite(fp, atext);
    if (fp) {
        if (fp->data)
            XFREE(fp->data, "fwdlist_ck.fp_data");
        XFREE(fp, "fwdlist_ck.fp");
    }
    return ((count > 0) || !atext || !*atext);
}

FWDLIST *fwdlist_get(thing)
dbref thing;
{
    dbref aowner;
    int aflags, alen, errors;
    char *tp;
    static FWDLIST *fp = NULL;

    if (!mudstate.standalone)
        return (FWDLIST *)nhashfind((thing), &mudstate.fwdlist_htab);

    if (!fp) {
        fp = (FWDLIST *) XMALLOC(sizeof(FWDLIST), "fwdlist_get");
        fp->data = NULL;
    }
    tp = atr_get(thing, A_FORWARDLIST, &aowner, &aflags, &alen);
    errors = fwdlist_load(fp, GOD, tp);
    free_lbuf(tp);
    return fp;
}

/* ---------------------------------------------------------------------------
 * propdir functions
 */

void propdir_set(thing, ifp)
dbref thing;
PROPDIR *ifp;
{
    PROPDIR *fp, *xfp;
    int i, stat = 0;

    /* If propdir list is null, clear */

    if (!ifp || (ifp->count <= 0)) {
        propdir_clr(thing);
        return;
    }
    /* Copy input propdir to a correctly-sized buffer */

    fp = (PROPDIR *) XMALLOC(sizeof(PROPDIR), "propdir_set");
    fp->data = (int *) XCALLOC(ifp->count, sizeof(int),
                               "propdir_set.data");
    for (i = 0; i < ifp->count; i++) {
        fp->data[i] = ifp->data[i];
    }
    fp->count = ifp->count;

    /*
     * Replace an existing propdir, or add a new one
     */

    xfp = propdir_get(thing);
    if (xfp) {
        if (xfp->data)
            XFREE(xfp->data, "propdir_set.xfp_data");
        XFREE(xfp, "propdir_set.xfp");
        stat = nhashrepl(thing, (int *)fp, &mudstate.propdir_htab);
    } else {
        stat = nhashadd(thing, (int *)fp, &mudstate.propdir_htab);
    }
    if (stat < 0) {              /* the add or replace failed */
        if (fp->data)
            XFREE(fp->data, "propdir_set.fp_data");
        XFREE(fp, "propdir_set.fp");
    }
}

void propdir_clr(thing)
dbref thing;
{
    PROPDIR *xfp;

    /* If a propdir exists, delete it */

    xfp = propdir_get(thing);
    if (xfp) {
        if (xfp->data)
            XFREE(xfp->data, "propdir_clr.data");
        XFREE(xfp, "propdir_clr");
        nhashdelete(thing, &mudstate.propdir_htab);
    }
}

int propdir_load(fp, player, atext)
PROPDIR *fp;
dbref player;
char *atext;
{
    dbref target;
    char *tp, *bp, *dp;
    int i, count, errors, fail;
    int *tmp_array;

    tmp_array = (int *)XCALLOC((LBUF_SIZE / 2), sizeof(int),
                               "propdir_load.tmp");

    count = 0;
    errors = 0;
    bp = tp = alloc_lbuf("propdir_load.str");
    strcpy(tp, atext);
    do {
        for (; *bp && isspace(*bp); bp++) ;	/* skip spaces */
        for (dp = bp; *bp && !isspace(*bp); bp++) ;	/* remember string */
        if (*bp)
            *bp++ = '\0';	/* terminate string */
        if ((*dp++ == '#') && isdigit(*dp)) {
            target = atoi(dp);

            if (!mudstate.standalone) {
                fail = (!Good_obj(target) || !Parentable(player, target));
            } else {
                fail = !Good_obj(target);
            }

            if (fail) {
                if (!mudstate.standalone) {
                    notify(player,
                           tprintf("Cannot parent to #%d: Permission denied.",
                                   target));
                }
                errors++;
            } else if (count < mudconf.propdir_lim) {
                if (fp->data)
                    fp->data[count++] = target;
                else
                    tmp_array[count++] = target;
            } else {
                if (!mudstate.standalone) {
                    notify(player,
                           tprintf("Cannot parent to #%d: Propdir limit exceeded.",
                                   target));
                }
                errors++;
            }
        }
    } while (*bp);

    free_lbuf(tp);

    if ((fp->data == NULL) && (count > 0)) {
        fp->data = (int *) XCALLOC(count, sizeof(int), "propdir_load.data");
        for (i = 0; i < count; i++)
            fp->data[i] = tmp_array[i];
    }

    fp->count = count;
    XFREE(tmp_array, "propdir_load.tmp");
    return errors;
}

int propdir_rewrite(fp, atext)
PROPDIR *fp;
char *atext;
{
    char *tp, *bp;
    int i, count;

    if (fp && fp->count) {
        count = fp->count;
        tp = alloc_sbuf("propdir_rewrite.errors");
        bp = atext;
        for (i = 0; i < fp->count; i++) {
            if (Good_obj(fp->data[i])) {
                sprintf(tp, "#%d ", fp->data[i]);
                safe_str(tp, atext, &bp);
            } else {
                count--;
            }
        }
        *bp = '\0';
        free_sbuf(tp);
    } else {
        count = 0;
        *atext = '\0';
    }
    return count;
}

static int propdir_ck(key, player, thing, anum, atext)
int key, anum;
dbref player, thing;
char *atext;
{
    PROPDIR *fp;
    int count;

    if (mudstate.standalone)
        return 1;

    count = 0;

    if (atext && *atext) {
        fp = (PROPDIR *) XMALLOC(sizeof(PROPDIR), "propdir_ck.fp");
        fp->data = NULL;
        propdir_load(fp, player, atext);
    } else {
        fp = NULL;
    }

    /* Set the cached propdir */

    propdir_set(thing, fp);
    count = propdir_rewrite(fp, atext);
    if (fp) {
        if (fp->data)
            XFREE(fp->data, "propdir_ck.fp_data");
        XFREE(fp, "propdir_ck.fp");
    }
    return ((count > 0) || !atext || !*atext);
}

PROPDIR *propdir_get(thing)
dbref thing;
{
    dbref aowner;
    int aflags, alen, errors;
    char *tp;
    static PROPDIR *fp = NULL;

    if (!mudstate.standalone)
        return (PROPDIR *)nhashfind((thing), &mudstate.propdir_htab);

    if (!fp) {
        fp = (PROPDIR *) XMALLOC(sizeof(PROPDIR), "propdir_get");
        fp->data = NULL;
    }
    tp = atr_get(thing, A_PROPDIR, &aowner, &aflags, &alen);
    errors = propdir_load(fp, GOD, tp);
    free_lbuf(tp);
    return fp;
}

/* ---------------------------------------------------------------------------
 */

static char *set_string(ptr, new)
char **ptr, *new;
{
    /* if pointer not null, free it */

    if (*ptr)
        XFREE(*ptr, "set_string");

    /* if new string is not null allocate space for it and copy it */

    if (!new)		/* || !*new */
        return (*ptr = NULL);	/* Check with GAC about this */
    *ptr = XSTRDUP(new, "set_string");
    return (*ptr);
}

/* ---------------------------------------------------------------------------
 * Name, s_Name: Get or set an object's name.
 */

INLINE void safe_name(thing, outbuf, bufc)
dbref thing;
char *outbuf, **bufc;
{
    dbref aowner;
    int aflags, alen;
    time_t save_access_time;
    char *buff;

    /* Retrieving a name never counts against an object's access time. */
    save_access_time = AccessTime(thing);

    if (!purenames[thing]) {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        set_string(&purenames[thing], strip_ansi(buff));
        free_lbuf(buff);
    }

    if (!names[thing]) {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        set_string(&names[thing], buff);
        s_NameLen(thing, alen);
        free_lbuf(buff);
    }
    safe_known_str(names[thing], NameLen(thing), outbuf, bufc);

    s_AccessTime(thing, save_access_time);
}

INLINE char *Name(thing)
dbref thing;
{
    dbref aowner;
    int aflags, alen;
    time_t save_access_time;
    char *buff;

    save_access_time = AccessTime(thing);

    if (!purenames[thing]) {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        set_string(&purenames[thing], strip_ansi(buff));
        free_lbuf(buff);
    }

    if (!names[thing]) {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        set_string(&names[thing], buff);
        s_NameLen(thing, alen);
        free_lbuf(buff);
    }
    s_AccessTime(thing, save_access_time);
    return names[thing];
}

INLINE char *PureName(thing)
dbref thing;
{
    dbref aowner;
    int aflags, alen;
    time_t save_access_time;
    char *buff;

    save_access_time = AccessTime(thing);

    if (!names[thing]) {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        set_string(&names[thing], buff);
        s_NameLen(thing, alen);
        free_lbuf(buff);
    }

    if (!purenames[thing]) {
        buff = atr_get(thing, A_NAME, &aowner, &aflags, &alen);
        set_string(&purenames[thing], strip_ansi(buff));
        free_lbuf(buff);
    }
    s_AccessTime(thing, save_access_time);
    return purenames[thing];
}

INLINE void s_Name(thing, s)
dbref thing;
char *s;
{
    int len;

    /* Truncate the name if we have to */

    if (s) {
        len = strlen(s);
        if (len > MBUF_SIZE) {
            len = MBUF_SIZE - 1;
            s[len] = '\0';
        }
    } else {
        len = 0;
    }

    atr_add_raw(thing, A_NAME, (char *)s);
    s_NameLen(thing, len);
    set_string(&names[thing], (char *)s);
    set_string(&purenames[thing], strip_ansi((char *)s));
}

void safe_exit_name(it, buff, bufc)
dbref it;
char *buff, **bufc;
{
    char *s = *bufc;
    int ansi_state = ANST_NORMAL;

    safe_name(it, buff, bufc);

    while (*s && (*s != EXIT_DELIMITER)) {
        if (*s == ESC_CHAR) {
            track_esccode(s, ansi_state);
        } else {
            ++s;
        }
    }

    *bufc = s;
    safe_str(ansi_transition_esccode(ansi_state, ANST_NORMAL), buff, bufc);
}

void s_Pass(thing, s)
dbref thing;
const char *s;
{
    atr_add_raw(thing, A_PASS, (char *)s);
}

/* ---------------------------------------------------------------------------
 * do_attribute: Manage user-named attributes.
 */

extern NAMETAB attraccess_nametab[];

void do_attribute(player, cause, key, aname, value)
dbref player, cause;
int key;
char *aname, *value;
{
    int success, negate, f;
    char *buff, *sp, *p, *q, *tbuf, *tokst;
    VATTR *va;
    ATTR *va2;

    /* Look up the user-named attribute we want to play with.
     * Note vattr names have a limited size.
     */

    buff = alloc_sbuf("do_attribute");
    for (p = buff, q = aname;
            *q && ((p - buff) < (VNAME_SIZE - 1));
            p++, q++)
        *p = toupper(*q);
    *p = '\0';

    if (!(ok_attr_name(buff) && (va = vattr_find(buff)))) {
        notify(player, "No such user-named attribute.");
        free_sbuf(buff);
        return;
    }
    switch (key) {
    case ATTRIB_ACCESS:

        /* Modify access to user-named attribute */

        for (sp = value; *sp; sp++)
            *sp = toupper(*sp);
        sp = strtok_r(value, " ", &tokst);
        success = 0;
        while (sp != NULL) {

            /* Check for negation */

            negate = 0;
            if (*sp == '!') {
                negate = 1;
                sp++;
            }
            /* Set or clear the appropriate bit */

            f = search_nametab(player, attraccess_nametab, sp);
            if (f > 0) {
                success = 1;
                if (negate)
                    va->flags &= ~f;
                else
                    va->flags |= f;

                /* Set the dirty bit */
                va->flags |= AF_DIRTY;
            } else {
                notify(player,
                       tprintf("Unknown permission: %s.", sp));
            }

            /* Get the next token */

            sp = strtok_r(NULL, " ", &tokst);
        }
        if (success && !Quiet(player))
            notify(player, "Attribute access changed.");
        break;

    case ATTRIB_RENAME:

        /* Make sure the new name doesn't already exist */

        va2 = atr_str(value);
        if (va2) {
            notify(player,
                   "An attribute with that name already exists.");
            free_sbuf(buff);
            return;
        }
        if (vattr_rename(va->name, value) == NULL)
            notify(player, "Attribute rename failed.");
        else
            notify(player, "Attribute renamed.");
        break;

    case ATTRIB_DELETE:

        /* Remove the attribute */

        vattr_delete(buff);
        notify(player, "Attribute deleted.");
        break;

    case ATTRIB_INFO:

        /* Print info, like @list user_attr does */

        if (!(va->flags & AF_DELETED)) {
            tbuf = alloc_lbuf("attribute_info");
            sprintf(tbuf, "%s(%d):", va->name, va->number);
            listset_nametab(player, attraccess_nametab, va->flags,
                            tbuf, 1);
            free_lbuf(tbuf);
        } else {
            notify(player, "That attribute has been deleted.");
        }
        break;
    }
    free_sbuf(buff);
    return;
}

/* ---------------------------------------------------------------------------
 * do_fixdb: Directly edit database fields
 */

void do_fixdb(player, cause, key, arg1, arg2)
dbref player, cause;
int key;
char *arg1, *arg2;
{
    dbref thing, res;

    init_match(player, arg1, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
        return;

    res = NOTHING;
    switch (key) {
    case FIXDB_OWNER:
    case FIXDB_LOC:
    case FIXDB_CON:
    case FIXDB_EXITS:
    case FIXDB_NEXT:
        init_match(player, arg2, NOTYPE);
        match_everything(0);
        res = noisy_match_result();
        break;
    case FIXDB_PENNIES:
        res = atoi(arg2);
        break;
    }

    switch (key) {
    case FIXDB_OWNER:
        s_Owner(thing, res);
        if (!Quiet(player))
            notify(player, tprintf("Owner set to #%d", res));
        break;
    case FIXDB_LOC:
        s_Location(thing, res);
        if (!Quiet(player))
            notify(player, tprintf("Location set to #%d", res));
        break;
    case FIXDB_CON:
        s_Contents(thing, res);
        if (!Quiet(player))
            notify(player, tprintf("Contents set to #%d", res));
        break;
    case FIXDB_EXITS:
        s_Exits(thing, res);
        if (!Quiet(player))
            notify(player, tprintf("Exits set to #%d", res));
        break;
    case FIXDB_NEXT:
        s_Next(thing, res);
        if (!Quiet(player))
            notify(player, tprintf("Next set to #%d", res));
        break;
    case FIXDB_PENNIES:
        s_Pennies(thing, res);
        if (!Quiet(player))
            notify(player, tprintf("Pennies set to %d", res));
        break;
    case FIXDB_NAME:
        if (Typeof(thing) == TYPE_PLAYER) {
            if (!ok_player_name(arg2)) {
                notify(player,
                       "That's not a good name for a player.");
                return;
            }
            if (lookup_player(NOTHING, arg2, 0) != NOTHING) {
                notify(player,
                       "That name is already in use.");
                return;
            }
            STARTLOG(LOG_SECURITY, "SEC", "CNAME")
            log_name(thing),
                     log_printf(" renamed to %s", strip_ansi(arg2));
            ENDLOG
            if (Suspect(player)) {
                raw_broadcast(WIZARD,
                              "[Suspect] %s renamed to %s",
                              Name(thing), arg2);
            }
            delete_player_name(thing, Name(thing));
            s_Name(thing, arg2);
            add_player_name(thing, arg2);
        } else {
            if (!ok_name(arg2)) {
                notify(player,
                       "Warning: That is not a reasonable name.");
            }
            s_Name(thing, arg2);
        }
        if (!Quiet(player))
            notify(player, tprintf("Name set to %s", arg2));
        break;
    }
}

/* ---------------------------------------------------------------------------
 * init_attrtab: Initialize the attribute hash tables.
 */

void NDECL(init_attrtab)
{
    ATTR *a;
    char *buff, *p, *q;

    hashinit(&mudstate.attr_name_htab, 100 * HASH_FACTOR, HT_STR);
    buff = alloc_sbuf("init_attrtab");
    for (a = attr; a->number; a++) {
        anum_extend(a->number);
        anum_set(a->number, a);
        for (p = buff, q = (char *)a->name; *q; p++, q++)
            *p = toupper(*q);
        *p = '\0';
        hashadd(buff, (int *)a, &mudstate.attr_name_htab, 0);
    }
    free_sbuf(buff);
}

/* ---------------------------------------------------------------------------
 * atr_str: Look up an attribute by name.
 */

ATTR *atr_str(s)
char *s;
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to uppercase. Limit length of name.  */

    buff = alloc_sbuf("atr_str");
    for (p = buff, q = s; *q && ((p - buff) < (VNAME_SIZE - 1)); p++, q++)
        *p = toupper(*q);
    *p = '\0';

    if (!ok_attr_name(buff)) {
        free_sbuf(buff);
        return NULL;
    }

    /* Look for a predefined attribute */

    if (!mudstate.standalone) {
        a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
        if (a != NULL) {
            free_sbuf(buff);
            return a;
        }
    } else {
        for (a = attr; a->name; a++) {
            if (!string_compare(a->name, s)) {
                free_sbuf(buff);
                return a;
            }
        }
    }

    /* Nope, look for a user attribute */

    va = vattr_find(buff);
    free_sbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
        tattr.name = va->name;
        tattr.number = va->number;
        tattr.flags = va->flags;
        tattr.check = NULL;
        return &tattr;
    }

    if (mudstate.standalone) {
        /*
         * No exact match, try for a prefix match on predefined attribs.
         * Check for both longer versions and shorter versions.
         */

        for (a = attr; a->name; a++) {
            if (string_prefix(s, a->name))
                return a;
            if (string_prefix(a->name, s))
                return a;
        }
    }

    /* All failed, return NULL */

    return NULL;
}

/* ---------------------------------------------------------------------------
 * anum_extend: Grow the attr num lookup table.
 */

ATTR **anum_table = NULL;
int anum_alc_top = 0;

void anum_extend(newtop)
int newtop;
{
    ATTR **anum_table2;
    int delta, i;

    if (!mudstate.standalone)
        delta = mudconf.init_size;
    else
        delta = 1000;

    if (newtop <= anum_alc_top)
        return;
    if (newtop < anum_alc_top + delta)
        newtop = anum_alc_top + delta;
    if (anum_table == NULL) {
        anum_table = (ATTR **) XCALLOC(newtop + 1, sizeof(ATTR *),
                                       "anum_extend");
    } else {
        anum_table2 = (ATTR **) XCALLOC(newtop + 1, sizeof(ATTR *),
                                        "anum_extend");
        for (i = 0; i <= anum_alc_top; i++)
            anum_table2[i] = anum_table[i];
        XFREE(anum_table, "anum_extend");
        anum_table = anum_table2;
    }
    anum_alc_top = newtop;
}

/* ---------------------------------------------------------------------------
 * atr_num: Look up an attribute by number.
 */

ATTR *atr_num(anum)
int anum;
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */

    if (anum < A_USER_START)
        return anum_get(anum);

    if (anum > anum_alc_top)
        return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
        tattr.name = va->name;
        tattr.number = va->number;
        tattr.flags = va->flags;
        tattr.check = NULL;
        return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

/* ---------------------------------------------------------------------------
 * mkattr: Lookup attribute by name, creating if needed.
 */

int mkattr(buff)
char *buff;
{
    ATTR *ap;
    VATTR *va;
    int vflags;
    KEYLIST *kp;

    if (!(ap = atr_str(buff))) {

        /* Unknown attr, create a new one.
         * Check if it matches any attribute type pattern that
         * we have defined; if it does, give it those flags.
         * Otherwise, use the default vattr flags.
         */

        if (!mudstate.standalone) {
            vflags = mudconf.vattr_flags;
            for (kp = mudconf.vattr_flag_list; kp != NULL; kp = kp->next) {
                if (quick_wild(kp->name, buff)) {
                    vflags = kp->data;
                    break;
                }
            }
            va = vattr_alloc(buff, vflags);
        } else {
            va = vattr_alloc(buff, mudconf.vattr_flags);
        }

        if (!va || !(va->number))
            return -1;
        return va->number;
    }
    if (!(ap->number))
        return -1;
    return ap->number;
}

/* ---------------------------------------------------------------------------
 * al_decode: Fetch an attribute number from an alist.
 */

static int al_decode(app)
char **app;
{
    int atrnum = 0, anum, atrshft = 0;
    char *ap;

    ap = *app;

    for (;;) {
        anum = ((*ap) & 0x7f);
        if (atrshft > 0)
            atrnum += (anum << atrshft);
        else
            atrnum = anum;
        if (!(*ap++ & 0x80)) {
            *app = ap;
            return atrnum;
        }
        atrshft += 7;
    }
    /* NOTREACHED */
}

/* ---------------------------------------------------------------------------
 * al_code: Store an attribute number in an alist.
 */

static void al_code(app, atrnum)
char **app;
int atrnum;
{
    char *ap;

    ap = *app;

    for (;;) {
        *ap = atrnum & 0x7f;
        atrnum = atrnum >> 7;
        if (!atrnum) {
            *app = ++ap;
            return;
        }
        *ap++ |= 0x80;
    }
}

/* ---------------------------------------------------------------------------
 * Commer: check if an object has any $-commands in its attributes.
 */

int Commer(thing)
dbref thing;
{
    char *s, *as;
    int attr, aflags, alen;
    dbref aowner;
    ATTR *ap;

    if ((!Has_Commands(thing) && mudconf.req_cmds_flag) || Halted(thing))
        return 0;

    s = alloc_lbuf("Commer");
    atr_push();
    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
        ap = atr_num(attr);
        if (!ap || (ap->flags & AF_NOPROG))
            continue;

        s = atr_get_str(s, thing, attr, &aowner, &aflags, &alen);

        if ((*s == '$') && !(aflags & AF_NOPROG)) {
            atr_pop();
            free_lbuf(s);
            return 1;
        }
    }
    atr_pop();
    free_lbuf(s);
    return 0;
}

/* routines to handle object attribute lists */

/* ---------------------------------------------------------------------------
 * al_size, al_fetch, al_store, al_add, al_delete: Manipulate attribute lists
 */

/* al_extend: Get more space for attributes, if needed */

void al_extend(buffer, bufsiz, len, copy)
char **buffer;
int *bufsiz, len, copy;
{
    char *tbuff;
    int newsize;

    if (len > *bufsiz) {
        newsize = len + ATR_BUF_CHUNK;
        tbuff = (char *) XMALLOC(newsize, "al_extend");
        if (*buffer) {
            if (copy)
                memcpy(tbuff, *buffer, *bufsiz);
            XFREE(*buffer, "al_extend");
        }
        *buffer = tbuff;
        *bufsiz = newsize;
    }
}

/* al_size: Return length of attribute list in chars */

int al_size(astr)
char *astr;
{
    if (!astr)
        return 0;
    return (strlen(astr) + 1);
}

/* al_store: Write modified attribute list */

void NDECL(al_store)
{
    if (mudstate.mod_al_id != NOTHING) {
        if (mudstate.mod_alist && *mudstate.mod_alist) {
            atr_add_raw(mudstate.mod_al_id, A_LIST,
                        mudstate.mod_alist);
        } else {
            atr_clr(mudstate.mod_al_id, A_LIST);
        }
    }
    mudstate.mod_al_id = NOTHING;
}

/* al_fetch: Load attribute list */

char *al_fetch(thing)
dbref thing;
{
    char *astr;
    int len;

    /* We only need fetch if we change things */

    if (mudstate.mod_al_id == thing)
        return mudstate.mod_alist;

    /* Fetch and set up the attribute list */

    al_store();
    astr = atr_get_raw(thing, A_LIST);
    if (astr) {
        len = al_size(astr);
        al_extend(&mudstate.mod_alist, &mudstate.mod_size, len, 0);
        memcpy(mudstate.mod_alist, astr, len);
    } else {
        al_extend(&mudstate.mod_alist, &mudstate.mod_size, 1, 0);
        *mudstate.mod_alist = '\0';
    }
    mudstate.mod_al_id = thing;
    return mudstate.mod_alist;
}

/* al_add: Add an attribute to an attribute list */

void al_add(thing, attrnum)
dbref thing;
int attrnum;
{
    char *abuf, *cp;
    int anum;

    /* If trying to modify List attrib, return.  Otherwise, get the
     * attribute list.
     */

    if (attrnum == A_LIST)
        return;
    abuf = al_fetch(thing);

    /* See if attr is in the list.  If so, exit (need not do anything) */

    cp = abuf;
    while (*cp) {
        anum = al_decode(&cp);
        if (anum == attrnum)
            return;
    }

    /* Nope, extend it */

    al_extend(&mudstate.mod_alist, &mudstate.mod_size,
              (cp - abuf + ATR_BUF_INCR), 1);
    if (mudstate.mod_alist != abuf) {

        /* extend returned different buffer, re-find the end */

        abuf = mudstate.mod_alist;
        for (cp = abuf; *cp; anum = al_decode(&cp)) ;
    }
    /* Add the new attribute on to the end */

    al_code(&cp, attrnum);
    *cp = '\0';

    return;
}

/* al_delete: Remove an attribute from an attribute list */

void al_delete(thing, attrnum)
dbref thing;
int attrnum;
{
    int anum;
    char *abuf, *cp, *dp;

    /* If trying to modify List attrib, return.  Otherwise, get the
     * attribute list.
     */

    if (attrnum == A_LIST)
        return;
    abuf = al_fetch(thing);
    if (!abuf)
        return;

    cp = abuf;
    while (*cp) {
        dp = cp;
        anum = al_decode(&cp);
        if (anum == attrnum) {
            while (*cp) {
                anum = al_decode(&cp);
                al_code(&dp, anum);
            }
            *dp = '\0';
            return;
        }
    }
    return;
}

INLINE static void makekey(thing, atr, abuff)
dbref thing;
int atr;
Aname *abuff;
{
    abuff->object = thing;
    abuff->attrnum = atr;
    return;
}

/* ---------------------------------------------------------------------------
 * al_destroy: wipe out an object's attribute list.
 */

void al_destroy(thing)
dbref thing;
{
    if (mudstate.mod_al_id == thing)
        al_store();	/* remove from cache */
    atr_clr(thing, A_LIST);
}

/* ---------------------------------------------------------------------------
 * atr_encode: Encode an attribute string.
 */

static char *atr_encode(iattr, thing, owner, flags, atr)
char *iattr;
dbref thing, owner;
int flags, atr;
{

    /* If using the default owner and flags (almost all attributes will),
     * just store the string.
     */

    if (((owner == Owner(thing)) || (owner == NOTHING)) && !flags)
        return iattr;

    /* Encode owner and flags into the attribute text */

    if (owner == NOTHING)
        owner = Owner(thing);
    return tprintf("%c%d:%d:%s", ATR_INFO_CHAR, owner, flags, iattr);
}

/* ---------------------------------------------------------------------------
 * atr_decode: Decode an attribute string.
 */

static void atr_decode(iattr, oattr, thing, owner, flags, atr, alen)
char *iattr, *oattr;
dbref thing, *owner;
int *flags, atr, *alen;
{
    char *cp;
    int neg;

    /* See if the first char of the attribute is the special character */

    if (*iattr == ATR_INFO_CHAR) {

        /* It is, crack the attr apart */

        cp = &iattr[1];

        /* Get the attribute owner */

        *owner = 0;
        neg = 0;
        if (*cp == '-') {
            neg = 1;
            cp++;
        }
        while (isdigit(*cp)) {
            *owner = (*owner * 10) + (*cp++ - '0');
        }
        if (neg)
            *owner = 0 - *owner;

        /* If delimiter is not ':', just return attribute */

        if (*cp++ != ':') {
            *owner = Owner(thing);
            *flags = 0;
            if (oattr) {
                StrCopyLen(oattr, iattr, alen);
            }
            return;
        }
        /* Get the attribute flags */

        *flags = 0;
        while (isdigit(*cp)) {
            *flags = (*flags * 10) + (*cp++ - '0');
        }

        /* If delimiter is not ':', just return attribute */

        if (*cp++ != ':') {
            *owner = Owner(thing);
            *flags = 0;
            if (oattr) {
                StrCopyLen(oattr, iattr, alen);
            }
        }
        /* Get the attribute text */

        if (oattr) {
            StrCopyLen(oattr, cp, alen);
        }
        if (*owner == NOTHING)
            *owner = Owner(thing);
    } else {

        /* Not the special character, return normal info */

        *owner = Owner(thing);
        *flags = 0;
        if (oattr) {
            StrCopyLen(oattr, iattr, alen);
        }
    }
}

/* ---------------------------------------------------------------------------
 * atr_clr: clear an attribute in the list.
 */

void atr_clr(thing, atr)
dbref thing;
int atr;
{
    Aname okey;
    DBData key;

    makekey(thing, atr, &okey);

    /* Delete the entry from cache */

    key.dptr = &okey;
    key.dsize = sizeof(Aname);
    cache_del(key, DBTYPE_ATTRIBUTE);

    al_delete(thing, atr);

    if (!mudstate.standalone && !mudstate.loading_db)
        s_Modified(thing);

    switch (atr) {
    case A_STARTUP:
        s_Flags(thing, Flags(thing) & ~HAS_STARTUP);
        break;
    case A_DAILY:
        s_Flags2(thing, Flags2(thing) & ~HAS_DAILY);
        if (!mudstate.standalone)
            (void) cron_clr(thing, A_DAILY);
        break;
    case A_FORWARDLIST:
        s_Flags2(thing, Flags2(thing) & ~HAS_FWDLIST);
        break;
    case A_LISTEN:
        s_Flags2(thing, Flags2(thing) & ~HAS_LISTEN);
        break;
    case A_SPEECHFMT:
        s_Flags3(thing, Flags3(thing) & ~HAS_SPEECHMOD);
        break;
    case A_PROPDIR:
        s_Flags3(thing, Flags3(thing) & ~HAS_PROPDIR);
        break;
    case A_TIMEOUT:
        if (!mudstate.standalone)
            desc_reload(thing);
        break;
    case A_QUEUEMAX:
        if (!mudstate.standalone)
            pcache_reload(thing);
        break;
    }
}

/* ---------------------------------------------------------------------------
 * atr_add_raw, atr_add: add attribute of type atr to list
 */

void atr_add_raw(thing, atr, buff)
dbref thing;
int atr;
char *buff;
{
    Attr *a;
    Aname okey;
    DBData key, data;

    makekey(thing, atr, &okey);

    if (!buff || !*buff) {
        /* Delete the entry from cache */

        key.dptr = &okey;
        key.dsize = sizeof(Aname);
        cache_del(key, DBTYPE_ATTRIBUTE);

        al_delete(thing, atr);
        return;
    }

    if ((a = (Attr *) XMALLOC(strlen(buff) + 1, "atr_add_raw.2")) == (char *)0) {
        return;
    }
    strcpy(a, buff);

    /* Store the value in cache */

    key.dptr = &okey;
    key.dsize = sizeof(Aname);
    data.dptr = a;
    data.dsize = strlen(a) + 1;
    cache_put(key, data, DBTYPE_ATTRIBUTE);

    al_add(thing, atr);

    if (!mudstate.standalone && !mudstate.loading_db)
        s_Modified(thing);

    switch (atr) {
    case A_STARTUP:
        s_Flags(thing, Flags(thing) | HAS_STARTUP);
        break;
    case A_DAILY:
        s_Flags2(thing, Flags2(thing) | HAS_DAILY);
        if (!mudstate.standalone && !mudstate.loading_db) {
            char tbuf[SBUF_SIZE];
            (void) cron_clr(thing, A_DAILY);
            sprintf(tbuf, "0 %d * * *", mudconf.events_daily_hour);
            call_cron(thing, thing, A_DAILY, tbuf);
        }
        break;
    case A_FORWARDLIST:
        s_Flags2(thing, Flags2(thing) | HAS_FWDLIST);
        break;
    case A_LISTEN:
        s_Flags2(thing, Flags2(thing) | HAS_LISTEN);
        break;
    case A_SPEECHFMT:
        s_Flags3(thing, Flags3(thing) | HAS_SPEECHMOD);
        break;
    case A_PROPDIR:
        s_Flags3(thing, Flags3(thing) | HAS_PROPDIR);
        break;
    case A_TIMEOUT:
        if (!mudstate.standalone)
            desc_reload(thing);
        break;
    case A_QUEUEMAX:
        if (!mudstate.standalone)
            pcache_reload(thing);
        break;
    }
}

void atr_add(thing, atr, buff, owner, flags)
dbref thing, owner;
int atr, flags;
char *buff;
{
    char *tbuff;

    if (!buff || !*buff) {
        atr_clr(thing, atr);
    } else {
        tbuff = atr_encode(buff, thing, owner, flags, atr);
        atr_add_raw(thing, atr, tbuff);
    }
}

void atr_set_owner(thing, atr, owner)
dbref thing, owner;
int atr;
{
    dbref aowner;
    int aflags, alen;
    char *buff;

    buff = atr_get(thing, atr, &aowner, &aflags, &alen);
    atr_add(thing, atr, buff, owner, aflags);
    free_lbuf(buff);
}

void atr_set_flags(thing, atr, flags)
dbref thing, flags;
int atr;
{
    dbref aowner;
    int aflags, alen;
    char *buff;

    buff = atr_get(thing, atr, &aowner, &aflags, &alen);
    atr_add(thing, atr, buff, aowner, flags);
    free_lbuf(buff);
}

/* ---------------------------------------------------------------------------
 * atr_get_raw, atr_get_str, atr_get: Get an attribute from the database.
 */

char *atr_get_raw(thing, atr)
dbref thing;
int atr;
{
    DBData key, data;
    Aname okey;

    if (Typeof(thing) == TYPE_GARBAGE)
        return NULL;

    if (!mudstate.standalone && !mudstate.loading_db)
        s_Accessed(thing);

    makekey(thing, atr, &okey);

    /* Fetch the entry from cache and return it */

    key.dptr = &okey;
    key.dsize = sizeof(Aname);
    data = cache_get(key, DBTYPE_ATTRIBUTE);
    return data.dptr;
}

char *atr_get_str(s, thing, atr, owner, flags, alen)
char *s;
dbref thing, *owner;
int atr, *flags, *alen;
{
    char *buff;

    buff = atr_get_raw(thing, atr);
    if (!buff) {
        *owner = Owner(thing);
        *flags = 0;
        *alen = 0;
        *s = '\0';
    } else {
        atr_decode(buff, s, thing, owner, flags, atr, alen);
    }
    return s;
}

char *atr_get(thing, atr, owner, flags, alen)
dbref thing, *owner;
int atr, *flags, *alen;
{
    char *buff;

    buff = alloc_lbuf("atr_get");
    return atr_get_str(buff, thing, atr, owner, flags, alen);
}

int atr_get_info(thing, atr, owner, flags)
dbref thing, *owner;
int atr, *flags;
{
    int alen;
    char *buff;

    buff = atr_get_raw(thing, atr);
    if (!buff) {
        *owner = Owner(thing);
        *flags = 0;
        return 0;
    }
    atr_decode(buff, NULL, thing, owner, flags, atr, &alen);
    return 1;
}

char *atr_pget_str(s, thing, atr, owner, flags, alen)
char *s;
dbref thing, *owner;
int atr, *flags, *alen;
{
    char *buff;
    dbref parent;
    int lev;
    ATTR *ap;
    PROPDIR *pp;

    ITER_PARENTS(thing, parent, lev) {
        buff = atr_get_raw(parent, atr);
        if (buff && *buff) {
            atr_decode(buff, s, thing, owner, flags, atr, alen);
            if ((lev == 0) || !(*flags & AF_PRIVATE))
                return s;
        }
        if ((lev == 0) && Good_obj(Parent(parent))) {
            ap = atr_num(atr);
            if (!ap || ap->flags & AF_PRIVATE)
                break;
        }
    }

    if (H_Propdir(thing) && ((pp = propdir_get(thing)) != NULL)) {
        for (lev = 0;
                (lev < pp->count) && (lev < mudconf.propdir_lim);
                lev++) {
            parent = pp->data[lev];
            if (Good_obj(parent) && (parent != thing)) {
                buff = atr_get_raw(parent, atr);
                if (buff && *buff) {
                    atr_decode(buff, s, thing, owner, flags, atr, alen);
                    if (!(*flags & AF_PRIVATE))
                        return s;
                }
            }
        }
    }

    *owner = Owner(thing);
    *flags = 0;
    *alen = 0;
    *s = '\0';
    return s;
}

char *atr_pget(thing, atr, owner, flags, alen)
dbref thing, *owner;
int atr, *flags, *alen;
{
    char *buff;

    buff = alloc_lbuf("atr_pget");
    return atr_pget_str(buff, thing, atr, owner, flags, alen);
}

int atr_pget_info(thing, atr, owner, flags)
dbref thing, *owner;
int atr, *flags;
{
    char *buff;
    dbref parent;
    int lev, alen;
    ATTR *ap;
    PROPDIR *pp;

    ITER_PARENTS(thing, parent, lev) {
        buff = atr_get_raw(parent, atr);
        if (buff && *buff) {
            atr_decode(buff, NULL, thing, owner, flags, atr, &alen);
            if ((lev == 0) || !(*flags & AF_PRIVATE))
                return 1;
        }
        if ((lev == 0) && Good_obj(Parent(parent))) {
            ap = atr_num(atr);
            if (!ap || ap->flags & AF_PRIVATE)
                break;
        }
    }

    if (H_Propdir(thing) && ((pp = propdir_get(thing)) != NULL)) {
        for (lev = 0;
                (lev < pp->count) && (lev < mudconf.propdir_lim);
                lev++) {
            parent = pp->data[lev];
            if (Good_obj(parent) && (parent != thing)) {
                buff = atr_get_raw(parent, atr);
                if (buff && *buff) {
                    atr_decode(buff, NULL, thing, owner, flags, atr &alen);
                    if (!(*flags & AF_PRIVATE))
                        return 1;
                }
            }
        }
    }

    *owner = Owner(thing);
    *flags = 0;
    return 0;
}


/* ---------------------------------------------------------------------------
 * atr_free: Return all attributes of an object.
 */

void atr_free(thing)
dbref thing;
{
    int attr;
    char *as;
    atr_push();
    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
        atr_clr(thing, attr);
    }
    atr_pop();
    al_destroy(thing);	/* Just to be on the safe side */
}

/* ---------------------------------------------------------------------------
 * atr_cpy: Copy all attributes from one object to another.  Takes the
 * player argument to ensure that only attributes that COULD be set by
 * the player are copied.
 */

void atr_cpy(player, dest, source)
dbref player, dest, source;
{
    int attr, aflags, alen;
    dbref owner, aowner;
    char *as, *buf;
    ATTR *at;

    owner = Owner(dest);
    buf = alloc_lbuf("atr_cpy");
    atr_push();
    for (attr = atr_head(source, &as); attr; attr = atr_next(&as)) {
        buf = atr_get_str(buf, source, attr, &aowner, &aflags, &alen);
        if (!(aflags & AF_LOCK))
            aowner = owner;		/* change owner */
        at = atr_num(attr);
        if (attr && at) {
            if (Write_attr(owner, dest, at, aflags))
                /* Only set attrs that owner has perm to set */
                atr_add(dest, attr, buf, aowner, aflags);
        }
    }
    atr_pop();
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * atr_chown: Change the ownership of the attributes of an object to the
 * current owner if they are not locked.
 */

void atr_chown(obj)
dbref obj;
{
    int attr, aflags, alen;
    dbref owner, aowner;
    char *as, *buf;

    owner = Owner(obj);
    buf = alloc_lbuf("atr_chown");
    atr_push();
    for (attr = atr_head(obj, &as); attr; attr = atr_next(&as)) {
        buf = atr_get_str(buf, obj, attr, &aowner, &aflags, &alen);
        if ((aowner != owner) && !(aflags & AF_LOCK))
            atr_add(obj, attr, buf, owner, aflags);
    }
    atr_pop();
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * atr_next: Return next attribute in attribute list.
 */

int atr_next(attrp)
char **attrp;
{
    if (!*attrp || !**attrp) {
        return 0;
    } else {
        return al_decode(attrp);
    }
}

/* ---------------------------------------------------------------------------
 * atr_push, atr_pop: Push and pop attr lists.
 */

void NDECL(atr_push)
{
    ALIST *new_alist;

    new_alist = (ALIST *) alloc_sbuf("atr_push");
    new_alist->data = mudstate.iter_alist.data;
    new_alist->len = mudstate.iter_alist.len;
    new_alist->next = mudstate.iter_alist.next;

    mudstate.iter_alist.data = NULL;
    mudstate.iter_alist.len = 0;
    mudstate.iter_alist.next = new_alist;
    return;
}

void NDECL(atr_pop)
{
    ALIST *old_alist;
    char *cp;

    old_alist = mudstate.iter_alist.next;

    if (mudstate.iter_alist.data) {
        XFREE(mudstate.iter_alist.data, "al_extend");
    }
    if (old_alist) {
        mudstate.iter_alist.data = old_alist->data;
        mudstate.iter_alist.len = old_alist->len;
        mudstate.iter_alist.next = old_alist->next;
        cp = (char *)old_alist;
        free_sbuf(cp);
    } else {
        mudstate.iter_alist.data = NULL;
        mudstate.iter_alist.len = 0;
        mudstate.iter_alist.next = NULL;
    }
    return;
}

/* ---------------------------------------------------------------------------
 * atr_head: Returns the head of the attr list for object 'thing'
 */

int atr_head(thing, attrp)
dbref thing;
char **attrp;
{
    char *astr;
    int alen;

    /* Get attribute list.  Save a read if it is in the modify atr list */

    if (thing == mudstate.mod_al_id) {
        astr = mudstate.mod_alist;
    } else {
        astr = atr_get_raw(thing, A_LIST);
    }
    alen = al_size(astr);

    /* If no list, return nothing */

    if (!alen)
        return 0;

    /* Set up the list and return the first entry */

    al_extend(&mudstate.iter_alist.data, &mudstate.iter_alist.len,
              alen, 0);
    memcpy(mudstate.iter_alist.data, astr, alen);
    *attrp = mudstate.iter_alist.data;
    return atr_next(attrp);
}


/* ---------------------------------------------------------------------------
 * db_grow: Extend the struct database.
 */

#define	SIZE_HACK	1	/* So mistaken refs to #-1 won't die. */

void initialize_objects(first, last)
dbref first, last;
{
    dbref thing;

    for (thing = first; thing < last; thing++) {
        s_Owner(thing, GOD);
        s_Flags(thing, (TYPE_GARBAGE | GOING));
        s_Powers(thing, 0);
        s_Powers2(thing, 0);
        s_Location(thing, NOTHING);
        s_Contents(thing, NOTHING);
        s_Exits(thing, NOTHING);
        s_Link(thing, NOTHING);
        s_Next(thing, NOTHING);
        s_Zone(thing, NOTHING);
        s_Parent(thing, NOTHING);
#ifdef MEMORY_BASED
        db[thing].attrtext.atrs = NULL;
        db[thing].attrtext.at_count = 0;
#endif
    }
}

void db_grow(newtop)
dbref newtop;
{
    int newsize, marksize, delta, i;
    MARKBUF *newmarkbuf;
    OBJ *newdb;
    NAME *newnames, *newpurenames;
    char *cp;

    if (!mudstate.standalone)
        delta = mudconf.init_size;
    else
        delta = 1000;

    /* Determine what to do based on requested size, current top and
     * size.  Make sure we grow in reasonable-size chunks to prevent
     * frequent reallocations of the db array.
     */

    /* If requested size is smaller than the current db size, ignore it */

    if (newtop <= mudstate.db_top) {
        return;
    }
    /* If requested size is greater than the current db size but smaller
     * than the amount of space we have allocated, raise the db size
     * and initialize the new area.
     */

    if (newtop <= mudstate.db_size) {
        for (i = mudstate.db_top; i < newtop; i++) {
            names[i] = NULL;
            purenames[i] = NULL;
        }
        initialize_objects(mudstate.db_top, newtop);
        mudstate.db_top = newtop;
        return;
    }
    /* Grow by a minimum of delta objects */

    if (newtop <= mudstate.db_size + delta) {
        newsize = mudstate.db_size + delta;
    } else {
        newsize = newtop;
    }

    /* Enforce minimum database size */

    if (newsize < mudstate.min_size)
        newsize = mudstate.min_size + delta;

    /* Grow the name tables */

    newnames = (NAME *) XCALLOC(newsize + SIZE_HACK, sizeof(NAME),
                                "db_grow.names");
    if (!newnames) {
        mainlog_printf("ABORT! db.c, could not allocate space for %d item name cache in db_grow().\n", newsize);
        abort();
    }

    if (names) {
        /* An old name cache exists.  Copy it. */

        names -= SIZE_HACK;
        memcpy((char *)newnames, (char *)names,
               (newtop + SIZE_HACK) * sizeof(NAME));
        cp = (char *)names;
        XFREE(cp, "db_grow.name");
    } else {

        /* Creating a brand new struct database.  Fill in the
         * 'reserved' area in case it gets referenced.
         */

        names = newnames;
        for (i = 0; i < SIZE_HACK; i++) {
            names[i] = NULL;
        }
    }
    names = newnames + SIZE_HACK;
    newnames = NULL;

    newpurenames = (NAME *) XCALLOC(newsize + SIZE_HACK,
                                    sizeof(NAME),
                                    "db_grow.purenames");

    if (!newpurenames) {
        mainlog_printf("ABORT! db.c, could not allocate space for %d item name cache in db_grow().\n", newsize);
        abort();
    }
    memset((char *)newpurenames, 0, (newsize + SIZE_HACK) * sizeof(NAME));

    if (purenames) {

        /* An old name cache exists.  Copy it. */

        purenames -= SIZE_HACK;
        memcpy((char *)newpurenames, (char *)purenames,
               (newtop + SIZE_HACK) * sizeof(NAME));
        cp = (char *)purenames;
        XFREE(cp, "db_grow.purename");
    } else {

        /* Creating a brand new struct database.  Fill in the
         * 'reserved' area in case it gets referenced.
         */

        purenames = newpurenames;
        for (i = 0; i < SIZE_HACK; i++) {
            purenames[i] = NULL;
        }
    }
    purenames = newpurenames + SIZE_HACK;
    newpurenames = NULL;

    /* Grow the db array */

    newdb = (OBJ *) XCALLOC(newsize + SIZE_HACK, sizeof(OBJ),
                            "db_grow.db");
    if (!newdb) {
        STARTLOG(LOG_ALWAYS, "ALC", "DB")
        log_printf("Could not allocate space for %d item struct database.", newsize);
        ENDLOG
        abort();
    }
    if (db) {

        /* An old struct database exists.  Copy it to the new buffer */

        db -= SIZE_HACK;
        memcpy((char *)newdb, (char *)db,
               (mudstate.db_top + SIZE_HACK) * sizeof(OBJ));
        cp = (char *)db;
        XFREE(cp, "db_grow.db");
    } else {

        /* Creating a brand new struct database.  Fill in the
         * 'reserved' area in case it gets referenced.
         */

        db = newdb;
        for (i = 0; i < SIZE_HACK; i++) {
            s_Owner(i, GOD);
            s_Flags(i, (TYPE_GARBAGE | GOING));
            s_Flags2(i, 0);
            s_Flags3(i, 0);
            s_Powers(i, 0);
            s_Powers2(i, 0);
            s_Location(i, NOTHING);
            s_Contents(i, NOTHING);
            s_Exits(i, NOTHING);
            s_Link(i, NOTHING);
            s_Next(i, NOTHING);
            s_Zone(i, NOTHING);
            s_Parent(i, NOTHING);
        }
    }
    db = newdb + SIZE_HACK;
    newdb = NULL;

    /* Go do the rest of the things */

    CALL_ALL_MODULES(db_grow, (newsize, newtop));

    for (i = mudstate.db_top; i < newtop; i++) {
        names[i] = NULL;
        purenames[i] = NULL;
    }
    initialize_objects(mudstate.db_top, newtop);
    mudstate.db_top = newtop;
    mudstate.db_size = newsize;

    /* Grow the db mark buffer */

    marksize = (newsize + 7) >> 3;
    newmarkbuf = (MARKBUF *) XMALLOC(marksize, "db_grow");
    memset((char *)newmarkbuf, 0, marksize);
    if (mudstate.markbits) {
        marksize = (newtop + 7) >> 3;
        memcpy((char *)newmarkbuf, (char *)mudstate.markbits, marksize);
        cp = (char *)mudstate.markbits;
        XFREE(cp, "db_grow");
    }
    mudstate.markbits = newmarkbuf;
}

void NDECL(db_free)
{
    char *cp;

    if (db != NULL) {
        db -= SIZE_HACK;
        cp = (char *)db;
        XFREE(cp, "db_grow");
        db = NULL;
    }
    mudstate.db_top = 0;
    mudstate.db_size = 0;
    mudstate.freelist = NOTHING;
}

void NDECL(db_make_minimal)
{
    dbref obj;

    db_free();
    db_grow(1);

    s_Name(0, "Limbo");
    s_Flags(0, TYPE_ROOM);
    s_Flags2(0, 0);
    s_Flags3(0, 0);
    s_Powers(0, 0);
    s_Powers2(0, 0);
    s_Location(0, NOTHING);
    s_Exits(0, NOTHING);
    s_Link(0, NOTHING);
    s_Parent(0, NOTHING);
    s_Zone(0, NOTHING);
    s_Pennies(0, 1);
    s_Owner(0, 1);

    /* should be #1 */
    load_player_names();
    obj = create_player((char *)"Wizard", (char *)"potrzebie", NOTHING, 0, 1);
    s_Flags(obj, Flags(obj) | WIZARD);
    s_Flags2(obj, 0);
    s_Flags3(obj, 0);
    s_Powers(obj, 0);
    s_Powers2(obj, 0);
    s_Pennies(obj, 1000);

    /* Manually link to Limbo, just in case */

    s_Location(obj, 0);
    s_Next(obj, NOTHING);
    s_Contents(0, obj);
    s_Link(obj, 0);
}

dbref parse_dbref_only(s)
const char *s;
{
    const char *p;
    int x;

    /* Enforce completely numeric dbrefs */

    for (p = s; *p; p++) {
        if (!isdigit(*p))
            return NOTHING;
    }

    x = atoi(s);
    return ((x >= 0) ? x : NOTHING);
}

dbref parse_objid(s, p)
const char *s, *p;
{
    dbref it;
    time_t tt;
    const char *r;
    static char tbuf[LBUF_SIZE];

    /* We're passed two parameters: the start of the string, and the
     * pointer to where the ':' in the string is. If the latter is NULL,
     * go find it.
     */

    if (p == NULL) {
        if ((p = strchr(s, ':')) == NULL)
            return parse_dbref_only(s);
    }

    /* ObjID takes the format <dbref>:<timestamp as long int>
     * If we match the dbref but its creation time doesn't match the
     * timestamp, we don't have a match.
     */

    strncpy(tbuf, s, p - s);
    it = parse_dbref_only(tbuf);
    if (Good_obj(it)) {
        p++;
        for (r = p; *r; r++) {
            if (!isdigit(*r))
                return NOTHING;
        }
        tt = (time_t) atol(p);
        return ((CreateTime(it) == tt) ? it : NOTHING);
    }
    return NOTHING;
}

dbref parse_dbref(s)
const char *s;
{
    const char *p;
    int x;

    /* Either pure dbrefs or objids are okay */

    for (p = s; *p; p++) {
        if (!isdigit(*p)) {
            if (*p == ':')
                return parse_objid(s, p);
            else
                return NOTHING;
        }
    }

    x = atoi(s);
    return ((x >= 0) ? x : NOTHING);
}

void putstring(f, s)
FILE *f;
const char *s;
{
    putc('"', f);

    while (s && *s) {
        switch (*s) {
        case '\n':
            putc('\\', f);
            putc('n', f);
            break;
        case '\r':
            putc('\\', f);
            putc('r', f);
            break;
        case '\t':
            putc('\\', f);
            putc('t', f);
            break;
        case ESC_CHAR:
            putc('\\', f);
            putc('e', f);
            break;
        case '\\':
        case '"':
            putc('\\', f);
        default:
            putc(*s, f);
        }
        s++;
    }
    putc('"', f);
    putc('\n', f);
}

const char *getstring_noalloc(f, new_strings)
FILE *f;
int new_strings;
{
    static char buf[LBUF_SIZE];
    char *p;
    int c, lastc;

    p = buf;
    c = fgetc(f);
    if (!new_strings || (c != '"')) {
        ungetc(c, f);
        c = '\0';
        for (;;) {
            lastc = c;
            c = fgetc(f);

            /* If EOF or null, return */

            if (!c || (c == EOF)) {
                *p = '\0';
                return buf;
            }
            /* If a newline, return if prior char is not a cr.
             * Otherwise keep on truckin'
             */

            if ((c == '\n') && (lastc != '\r')) {
                *p = '\0';
                return buf;
            }
            safe_chr(c, buf, &p);
        }
    } else {
        for (;;) {
            c = fgetc(f);
            if (c == '"') {
                if ((c = fgetc(f)) != '\n')
                    ungetc(c, f);
                *p = '\0';
                return buf;
            } else if (c == '\\') {
                c = fgetc(f);
                switch (c) {
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'e':
                    c = ESC_CHAR;
                    break;
                }
            }
            if ((c == '\0') || (c == EOF)) {
                *p = '\0';
                return buf;
            }
            safe_chr(c, buf, &p);
        }
    }
}

INLINE dbref getref(f)
FILE *f;
{
    static char buf[SBUF_SIZE];
    fgets(buf, sizeof(buf), f);
    return (atoi(buf));
}

INLINE long getlong(f)
FILE *f;
{
    static char buf[SBUF_SIZE];
    fgets(buf, sizeof(buf), f);
    return (atol(buf));
}

void free_boolexp(b)
BOOLEXP *b;
{
    if (b == TRUE_BOOLEXP)
        return;

    switch (b->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
        free_boolexp(b->sub1);
        free_boolexp(b->sub2);
        free_bool(b);
        break;
    case BOOLEXP_NOT:
    case BOOLEXP_CARRY:
    case BOOLEXP_IS:
    case BOOLEXP_OWNER:
    case BOOLEXP_INDIR:
        free_boolexp(b->sub1);
        free_bool(b);
        break;
    case BOOLEXP_CONST:
        free_bool(b);
        break;
    case BOOLEXP_ATR:
    case BOOLEXP_EVAL:
        XFREE(b->sub1, "test_atr.sub1");
        free_bool(b);
        break;
    }
}

BOOLEXP *dup_bool(b)
BOOLEXP *b;
{
    BOOLEXP *r;

    if (b == TRUE_BOOLEXP)
        return (TRUE_BOOLEXP);

    r = alloc_bool("dup_bool");
    switch (r->type = b->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
        r->sub2 = dup_bool(b->sub2);
    case BOOLEXP_NOT:
    case BOOLEXP_CARRY:
    case BOOLEXP_IS:
    case BOOLEXP_OWNER:
    case BOOLEXP_INDIR:
        r->sub1 = dup_bool(b->sub1);
    case BOOLEXP_CONST:
        r->thing = b->thing;
        break;
    case BOOLEXP_EVAL:
    case BOOLEXP_ATR:
        r->thing = b->thing;
        r->sub1 = (BOOLEXP *) XSTRDUP((char *)b->sub1, "dup_bool.sub1");
        break;
    default:
        mainlog_printf("bad bool type!!\n");
        return (TRUE_BOOLEXP);
    }
    return (r);
}

int init_gdbm_db(gdbmfile)
char *gdbmfile;
{
    /* Calculate proper database block size */

    for (mudstate.db_block_size = 1; mudstate.db_block_size < (LBUF_SIZE * 4);
            mudstate.db_block_size = mudstate.db_block_size << 1) ;

    cache_init(mudconf.cache_width);
    dddb_setfile(gdbmfile);
    dddb_init();
    STARTLOG(LOG_ALWAYS, "INI", "LOAD")
    log_printf("Using gdbm file: %s", gdbmfile);
    ENDLOG
    db_free();
    return (0);
}

/* check_zone - checks back through a zone tree for control */

int check_zone(player, thing)
dbref player, thing;
{
    if (mudstate.standalone)
        return 0;

    if (!mudconf.have_zones || (Zone(thing) == NOTHING) ||
            isPlayer(thing) ||
            (mudstate.zone_nest_num + 1 == mudconf.zone_nest_lim)) {
        mudstate.zone_nest_num = 0;
        return 0;
    }

    /* We check Control_OK on the thing itself, not on its ZMO --
     * that allows us to have things default into a zone without
     * needing to be controlled by that ZMO.
     */
    if (!Control_ok(thing)) {
        return 0;
    }

    mudstate.zone_nest_num++;

    /* If the zone doesn't have a ControlLock, DON'T allow control. */

    if (atr_get_raw(Zone(thing), A_LCONTROL) &&
            could_doit(player, Zone(thing), A_LCONTROL)) {
        mudstate.zone_nest_num = 0;
        return 1;
    } else {
        return check_zone(player, Zone(thing));
    }

}

int check_zone_for_player(player, thing)
dbref player, thing;
{
    if (!Control_ok(Zone(thing))) {
        return 0;
    }

    mudstate.zone_nest_num++;

    if (!mudconf.have_zones || (Zone(thing) == NOTHING) ||
            (mudstate.zone_nest_num == mudconf.zone_nest_lim) || !(isPlayer(thing))) {
        mudstate.zone_nest_num = 0;
        return 0;
    }

    if (atr_get_raw(Zone(thing), A_LCONTROL) && could_doit(player, Zone(thing), A_LCONTROL)) {
        mudstate.zone_nest_num = 0;
        return 1;
    } else {
        return check_zone(player, Zone(thing));
    }

}

/* ---------------------------------------------------------------------------
 * dump_restart_db: Writes out socket information.
 */

void dump_restart_db()
{
    FILE *f;
    DESC *d;
    int version = 0;

    /* We maintain a version number for the restart database,
     * so we can restart even if the format of the restart db
     * has been changed in the new executable.
     */

    version |= RS_RECORD_PLAYERS;
    version |= RS_NEW_STRINGS;
    version |= RS_COUNT_REBOOTS;

    f = fopen("restart.db", "w");
    fprintf(f, "+V%d\n", version);
    putref(f, sock);
    putlong(f, mudstate.start_time);
    putref(f, mudstate.reboot_nums);
    putstring(f, mudstate.doing_hdr);
    putref(f, mudstate.record_players);
    DESC_ITER_ALL(d) {
        putref(f, d->descriptor);
        putref(f, d->flags);
        putlong(f, d->connected_at);
        putref(f, d->command_count);
        putref(f, d->timeout);
        putref(f, d->host_info);
        putref(f, d->player);
        putlong(f, d->last_time);
        putstring(f, d->output_prefix);
        putstring(f, d->output_suffix);
        putstring(f, d->addr);
        putstring(f, d->doing);
        putstring(f, d->username);
    }
    putref(f, 0);

    fclose(f);
}

void load_restart_db()
{
    FILE *f;
    DESC *d;
    DESC *p;

    int val, version, new_strings = 0;
    char *temp, buf[8];
    struct stat fstatbuf;

    f = fopen("restart.db", "r");
    if (!f) {
        mudstate.restarting = 0;
        return;
    }
    mudstate.restarting = 1;

    fgets(buf, 3, f);
    if (strncmp(buf, "+V", 2)) {
        abort();
    }
    version = getref(f);
    sock = getref(f);

    if (version & RS_NEW_STRINGS)
        new_strings = 1;

    maxd = sock + 1;
    mudstate.start_time = (time_t) getlong(f);

    if (version & RS_COUNT_REBOOTS)
        mudstate.reboot_nums = getref(f) + 1;

    strcpy(mudstate.doing_hdr, getstring_noalloc(f, new_strings));

    if (version & RS_CONCENTRATE) {
        (void)getref(f);
    }

    if (version & RS_RECORD_PLAYERS) {
        mudstate.record_players = getref(f);
    }

    while ((val = getref(f)) != 0) {
        ndescriptors++;
        d = alloc_desc("restart");
        d->descriptor = val;
        d->flags = getref(f);
        d->connected_at = (time_t) getlong(f);
        d->retries_left = mudconf.retry_limit;
        d->command_count = getref(f);
        d->timeout = getref(f);
        d->host_info = getref(f);
        d->player = getref(f);
        d->last_time = (time_t) getlong(f);
        temp = (char *)getstring_noalloc(f, new_strings);
        if (*temp) {
            d->output_prefix = alloc_lbuf("set_userstring");
            strcpy(d->output_prefix, temp);
        } else {
            d->output_prefix = NULL;
        }
        temp = (char *)getstring_noalloc(f, new_strings);
        if (*temp) {
            d->output_suffix = alloc_lbuf("set_userstring");
            strcpy(d->output_suffix, temp);
        } else {
            d->output_suffix = NULL;
        }

        strcpy(d->addr, getstring_noalloc(f, new_strings));
        strcpy(d->doing, getstring_noalloc(f, new_strings));
        strcpy(d->username, getstring_noalloc(f, new_strings));
        d->colormap = NULL;

        if (version & RS_CONCENTRATE) {
            (void)getref(f);
            (void)getref(f);
        }
        d->output_size = 0;
        d->output_tot = 0;
        d->output_lost = 0;
        d->output_head = NULL;
        d->output_tail = NULL;
        d->input_head = NULL;
        d->input_tail = NULL;
        d->input_size = 0;
        d->input_tot = 0;
        d->input_lost = 0;
        d->raw_input = NULL;
        d->raw_input_at = NULL;
        d->quota = mudconf.cmd_quota_max;
        d->program_data = NULL;
        d->hashnext = NULL;
        /* Note that d->address is NOT INITIALIZED, and it DOES
         * get used later, particularly when checking logout.
         */

        if (descriptor_list) {
            for (p = descriptor_list; p->next; p = p->next) ;
            d->prev = &p->next;
            p->next = d;
            d->next = NULL;
        } else {
            d->next = descriptor_list;
            d->prev = &descriptor_list;
            descriptor_list = d;
        }

        if (d->descriptor >= maxd)
            maxd = d->descriptor + 1;
        desc_addhash(d);
        if (isPlayer(d->player))
            s_Flags2(d->player, Flags2(d->player) | CONNECTED);
    }

    /* In case we've had anything bizarre happen... */

    DESC_ITER_ALL(d) {
        if (fstat(d->descriptor, &fstatbuf) < 0) {
            STARTLOG(LOG_PROBLEMS, "ERR", "RESTART")
            log_printf("Bad descriptor %d", d->descriptor);
            ENDLOG
            shutdownsock(d, R_SOCKDIED);
        }
    }

    DESC_ITER_CONN(d) {
        if (!isPlayer(d->player)) {
            shutdownsock(d, R_QUIT);
        }

    }

    fclose(f);
    remove("restart.db");
}
