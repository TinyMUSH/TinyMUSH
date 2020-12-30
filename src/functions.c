/* functions.c - MUSH function handlers */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"   /* required by mudconf */
#include "game.h"       /* required by mudconf */
#include "alloc.h"      /* required by mudconf */
#include "flags.h"      /* required by mudconf */
#include "htab.h"       /* required by mudconf */
#include "ltdl.h"       /* required by mudconf */
#include "udb.h"        /* required by mudconf */
#include "udb_defs.h"   /* required by mudconf */
#include "mushconf.h"   /* required by code */
#include "db.h"         /* required by externs */
#include "interface.h"  /* required by code */
#include "externs.h"    /* required by interface */
#include "command.h"    /* required by code */
#include "match.h"      /* required by code */
#include "attrs.h"      /* required by code */
#include "powers.h"     /* required by code */
#include "functions.h"  /* required by code */
#include "fnproto.h"    /* required by code */
#include "stringutil.h" /* required by code */
#include "nametabs.h"

extern int parse_ext_access(int *, EXTFUNCS **, char *, NAMETAB *, dbref, char *);

UFUN *ufun_head;

OBJXFUNCS xfunctions;

const Delim SPACE_DELIM = {1, " "};

void init_functab(void)
{
    FUN *fp;
    hashinit(&mudstate.func_htab, 250 * mudconf.hash_factor, HT_STR | HT_KEYREF);

    for (fp = flist; fp->name; fp++)
    {
        hashadd((char *)fp->name, (int *)fp, &mudstate.func_htab, 0);
    }

    ufun_head = NULL;
    hashinit(&mudstate.ufunc_htab, 15 * mudconf.hash_factor, HT_STR);

    xfunctions.func = NULL;
    xfunctions.count = 0;
}

void do_function(dbref player, dbref cause __attribute__((unused)), int key, char *fname, char *target)
{
    UFUN *ufp, *ufp2;
    ATTR *ap;
    char *np, *bp;
    int atr, aflags;
    dbref obj, aowner;

    /*
     * Check for list first
     */

    if (key & FUNCT_LIST)
    {
        if (fname && *fname)
        {
            /*
	     * Make it case-insensitive, and look it up.
	     */
            for (bp = fname; *bp; bp++)
            {
                *bp = tolower(*bp);
            }

            ufp = (UFUN *)hashfind(fname, &mudstate.ufunc_htab);

            if (ufp)
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/%s", ufp->name, ufp->obj, ((ATTR *)atr_num(ufp->atr))->name);
            }
            else
            {
                notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s not found in user function table.", fname);
            }

            return;
        }

        /*
	 * No name given, list them all.
	 */

        for (ufp = (UFUN *)hash_firstentry(&mudstate.ufunc_htab); ufp != NULL; ufp = (UFUN *)hash_nextentry(&mudstate.ufunc_htab))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "%s: #%d/%s", ufp->name, ufp->obj, ((ATTR *)atr_num(ufp->atr))->name);
        }

        return;
    }

    /*
     * Make a local uppercase copy of the function name
     */
    bp = np = XMALLOC(SBUF_SIZE, "np");
    SAFE_SB_STR(fname, np, &bp);
    *bp = '\0';

    for (bp = np; *bp; bp++)
    {
        *bp = toupper(*bp);
    }

    /*
     * Verify that the function doesn't exist in the builtin table
     */

    if (hashfind(np, &mudstate.func_htab) != NULL)
    {
        notify_quiet(player, "Function already defined in builtin function table.");
        XFREE(np);
        return;
    }

    /*
     * Make sure the target object exists
     */

    if (!parse_attrib(player, target, &obj, &atr, 0))
    {
        notify_quiet(player, NOMATCH_MESSAGE);
        XFREE(np);
        return;
    }

    /*
     * Make sure the attribute exists
     */

    if (atr == NOTHING)
    {
        notify_quiet(player, "No such attribute.");
        XFREE(np);
        return;
    }

    /*
     * Make sure attribute is readably by me
     */
    ap = atr_num(atr);

    if (!ap)
    {
        notify_quiet(player, "No such attribute.");
        XFREE(np);
        return;
    }

    atr_get_info(obj, atr, &aowner, &aflags);

    if (!See_attr(player, obj, ap, aowner, aflags))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        XFREE(np);
        return;
    }

    /*
     * Privileged functions require you control the obj.
     */

    if ((key & FUNCT_PRIV) && !Controls(player, obj))
    {
        notify_quiet(player, NOPERM_MESSAGE);
        XFREE(np);
        return;
    }

    /*
     * See if function already exists.  If so, redefine it
     */
    ufp = (UFUN *)hashfind(np, &mudstate.ufunc_htab);

    if (!ufp)
    {
        ufp = (UFUN *)XMALLOC(sizeof(UFUN), "ufp");
        ufp->name = XSTRDUP(np, "ufp->name");

        for (bp = (char *)ufp->name; *bp; bp++)
        {
            *bp = toupper(*bp);
        }

        ufp->obj = obj;
        ufp->atr = atr;
        ufp->perms = CA_PUBLIC;
        ufp->next = NULL;

        if (!ufun_head)
        {
            ufun_head = ufp;
        }
        else
        {
            for (ufp2 = ufun_head; ufp2->next; ufp2 = ufp2->next)
                ;

            ufp2->next = ufp;
        }

        if (hashadd(np, (int *)ufp, &mudstate.ufunc_htab, 0))
        {
            notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Function %s not defined.", fname);
            XFREE(ufp->name);
            XFREE(ufp);
            XFREE(np);
            return;
        }
    }

    ufp->obj = obj;
    ufp->atr = atr;
    ufp->flags = 0;

    if (key & FUNCT_NO_EVAL)
    {
        ufp->flags |= FN_NO_EVAL;
    }

    if (key & FUNCT_PRIV)
    {
        ufp->flags |= FN_PRIV;
    }

    if (key & FUNCT_NOREGS)
    {
        ufp->flags |= FN_NOREGS;
    }
    else if (key & FUNCT_PRES)
    {
        ufp->flags |= FN_PRES;
    }

    XFREE(np);

    if (!Quiet(player))
    {
        notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Function %s defined.", fname);
    }
}

/**
 * @brief List available functions.
 * 
 * @param player 
 */
void list_functable(dbref player)
{
    char *buf = XMALLOC(LBUF_SIZE, "buf");
    //buf = XMALLOC(LBUF_SIZE, "buf");
    XSTRCPY(buf, "Built-in functions:");
    //SAFE_LB_STR((char *)"Built-in functions:", buf, &bp);

    for (FUN *fp = flist; fp->name; fp++)
    {
        if (Check_Func_Access(player, fp))
        {
            XSPRINTFCAT(buf, " %s", fp->name);
            /*
            SAFE_LB_CHR(' ', buf, &bp);
            SAFE_LB_STR((char *)fp->name, buf, &bp);
            */
        }
    }

    notify(player, buf);

    for (MODULE *mp = mudstate.modules_list; mp != NULL; mp = mp->next)
    {
        FUN *modfns;
        char *s = XASPRINTF("s", "mod_%s_%s", mp->modname, "functable");

        if ((modfns = (FUN *)lt_dlsym(mp->handle, s)) != NULL)
        {
            XSPRINTF(buf, "Module %s functions:", mp->modname);
            for (FUN *fp = modfns; fp->name; fp++)
            {
                if (Check_Func_Access(player, fp))
                {
                    XSPRINTFCAT(buf, " %s", fp->name);
                    /*
                    SAFE_LB_CHR(' ', buf, &bp);
                    SAFE_LB_STR((char *)fp->name, buf, &bp);
                    */
                }
            }

            notify(player, buf);
        }
        XFREE(s);
    }
    

    XSTRCPY(buf, "User-defined functions:");
    for (UFUN *ufp = ufun_head; ufp; ufp = ufp->next)
    {
        if (check_access(player, ufp->perms))
        {
            XSPRINTFCAT(buf, " %s", ufp->name);
            /*
            SAFE_LB_CHR(' ', buf, &bp);
            SAFE_LB_STR(ufp->name, buf, &bp);
            */
        }
    }

    notify(player, buf);
    XFREE(buf);
}

/*
 * ---------------------------------------------------------------------------
 * list_funcaccess: List access on functions.
 */

void helper_list_funcaccess(dbref player, FUN *fp, char *buff)
{
    char *bp;
    int i;

    while (fp->name)
    {
        if (Check_Func_Access(player, fp))
        {
            if (!fp->xperms)
            {
                XSPRINTF(buff, "%s:", fp->name);
            }
            else
            {
                bp = buff;
                SAFE_LB_STR((char *)fp->name, buff, &bp);
                SAFE_LB_CHR(':', buff, &bp);

                for (i = 0; i < fp->xperms->num_funcs; i++)
                {
                    if (fp->xperms->ext_funcs[i])
                    {
                        SAFE_LB_CHR(' ', buff, &bp);
                        SAFE_LB_STR(fp->xperms->ext_funcs[i]->fn_name, buff, &bp);
                    }
                }
            }

            listset_nametab(player, access_nametab, fp->perms, buff, 1);
        }

        fp++;
    }
}

void list_funcaccess(dbref player)
{
    char *buff, *s;
    FUN *ftab;
    UFUN *ufp;
    MODULE *mp;
    buff = XMALLOC(SBUF_SIZE, "list_funcaccess");
    helper_list_funcaccess(player, flist, buff);
    s = XMALLOC(MBUF_SIZE, "list_funcaccess");

    for (mp = mudstate.modules_list; mp != NULL; mp = mp->next)
    {
        XSNPRINTF(s, MBUF_SIZE, "mod_%s_%s", mp->modname, "functable");

        if ((ftab = (FUN *)lt_dlsym(mp->handle, s)) != NULL)
        {
            helper_list_funcaccess(player, ftab, buff);
        }
    }

    XFREE(s);

    for (ufp = ufun_head; ufp; ufp = ufp->next)
    {
        if (check_access(player, ufp->perms))
        {
            XSPRINTF(buff, "%s:", ufp->name);
            listset_nametab(player, access_nametab, ufp->perms, buff, 1);
        }
    }

    XFREE(buff);
}

/*
 * ---------------------------------------------------------------------------
 * cf_func_access: set access on functions
 */

int cf_func_access(int *vp __attribute__((unused)), char *str, long extra, dbref player, char *cmd)
{
    FUN *fp;
    UFUN *ufp;
    char *ap;

    for (ap = str; *ap && !isspace(*ap); ap++)
        ;

    if (*ap)
    {
        *ap++ = '\0';
    }

    for (fp = flist; fp->name; fp++)
    {
        if (!string_compare(fp->name, str))
        {
            return (parse_ext_access(&fp->perms, &fp->xperms, ap, (NAMETAB *)extra, player, cmd));
        }
    }

    for (ufp = ufun_head; ufp; ufp = ufp->next)
    {
        if (!string_compare(ufp->name, str))
        {
            return (cf_modify_bits(&ufp->perms, ap, extra, player, cmd));
        }
    }

    cf_log(player, "CNF", "NFND", cmd, "%s %s not found", "Function", str);
    return -1;
}
