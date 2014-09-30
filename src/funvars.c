/* funvars.c - structure, variable, stack, and regexp functions */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"           /* required by mudconf */
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */

#include "mushconf.h"       /* required by code */

#include "db.h"         /* required by externs */
#include "interface.h"
#include "externs.h"        /* required by code */

#include "functions.h"      /* required by code */
#include "match.h"      /* required by code */
#include "attrs.h"      /* required by code */
#include "powers.h"     /* required by code */
#include "pcre.h"       /* required by code */

/*
 * ---------------------------------------------------------------------------
 * setq, setr, r: set and read global registers.
 */

/*
 * ASCII character table for %qa - %qz
 *
 * 0   - 47  : NULL to / (3 rows) 48  - 63  : 0 to ? 64  - 79  : @, A to O 80  -
 * 95  : P to _ 96  - 111 : `, a to o 112 - 127 : p to DEL 128 - 255 :
 * specials (8 rows)
 */

char qidx_chartab[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static const char *qidx_str = "0123456789abcdefghijklmnopqrstuvwxyz";

int set_register ( const char *funcname, char *name, char *data )
{
    /*
     * Return number of characters set. -1 indicates a name error. -2
     * indicates that a limit was exceeded.
     */
    int i, regnum, len, a_size, *tmp_lens;
    char *p, **tmp_regs;

    if ( !name || !*name ) {
        return -1;
    }

    if ( name[1] == '\0' ) {
        /*
         * Single-letter q-register. We allocate these either as a
         * block of 10 or a block of 36. (Most code won't go beyond
         * %q0-%q9, especially legacy code which predates the larger
         * number of global registers.)
         */
        regnum = qidx_chartab[ ( unsigned char ) * name];

        if ( ( regnum < 0 ) || ( regnum >= mudconf.max_global_regs ) ) {
            return -1;
        }

        /*
         * Check to see if we're just clearing. If we're clearing a
         * register that doesn't exist, then we do nothing. Otherwise
         * we wipe out the data.
         */

        if ( !data || !*data ) {
            if ( !mudstate.rdata || !mudstate.rdata->q_alloc ||
                    ( regnum >= mudstate.rdata->q_alloc ) ) {
                return 0;
            }

            if ( mudstate.rdata->q_regs[regnum] ) {
                free_lbuf ( mudstate.rdata->q_regs[regnum] );
                mudstate.rdata->q_regs[regnum] = NULL;
                mudstate.rdata->q_lens[regnum] = 0;
                mudstate.rdata->dirty++;
            }

            return 0;
        }

        /*
         * We're actually setting a register. Take care of allocating
         * space first.
         */

        if ( !mudstate.rdata ) {
            Init_RegData ( funcname, mudstate.rdata );
        }

        if ( !mudstate.rdata->q_alloc ) {
            a_size = ( regnum < 10 ) ? 10 : mudconf.max_global_regs;
            mudstate.rdata->q_alloc = a_size;
            mudstate.rdata->q_regs =
                xcalloc ( a_size, sizeof ( char * ), "q_regs" );
            mudstate.rdata->q_lens =
                xcalloc ( a_size, sizeof ( int ), "q_lens" );
            mudstate.rdata->q_alloc = a_size;
        } else if ( regnum >= mudstate.rdata->q_alloc ) {
            a_size = mudconf.max_global_regs;
            tmp_regs = xrealloc ( mudstate.rdata->q_regs,
                                  a_size * sizeof ( char * ), "q_regs" );
            tmp_lens = xrealloc ( mudstate.rdata->q_lens,
                                  a_size * sizeof ( int ), "q_lens" );
            memset ( &tmp_regs[mudstate.rdata->q_alloc], ( int ) 0, ( a_size - mudstate.rdata->q_alloc ) * sizeof ( char * ) );
            memset ( &tmp_lens[mudstate.rdata->q_alloc], ( int ) 0, ( a_size - mudstate.rdata->q_alloc ) * sizeof ( int ) );
            mudstate.rdata->q_regs = tmp_regs;
            mudstate.rdata->q_lens = tmp_lens;
            mudstate.rdata->q_alloc = a_size;
        }

        /*
         * Set it.
         */

        if ( !mudstate.rdata->q_regs[regnum] ) {
            mudstate.rdata->q_regs[regnum] = alloc_lbuf ( funcname );
        }

        len = strlen ( data );
        memcpy ( mudstate.rdata->q_regs[regnum], data, len + 1 );
        mudstate.rdata->q_lens[regnum] = len;
        mudstate.rdata->dirty++;
        return len;
    }

    /*
     * We have an arbitrarily-named register. Check for data-clearing
     * first, since that's easier.
     */

    if ( !data || !*data ) {
        if ( !mudstate.rdata || !mudstate.rdata->xr_alloc ) {
            return 0;
        }

        for ( p = name; *p; p++ ) {
            *p = tolower ( *p );
        }

        for ( i = 0; i < mudstate.rdata->xr_alloc; i++ ) {
            if ( mudstate.rdata->x_names[i] &&
                    !strcmp ( name, mudstate.rdata->x_names[i] ) ) {
                if ( mudstate.rdata->x_regs[i] ) {
                    free_sbuf ( mudstate.rdata->x_names[i] );
                    mudstate.rdata->x_names[i] = NULL;
                    free_lbuf ( mudstate.rdata->x_regs[i] );
                    mudstate.rdata->x_regs[i] = NULL;
                    mudstate.rdata->x_lens[i] = 0;
                    mudstate.rdata->dirty++;
                    return 0;
                } else {
                    return 0;
                }
            }
        }

        return 0;   /* register unset, so just return */
    }

    /*
     * Check for a valid name. We enforce names beginning with a letter,
     * in case we want to do something special with naming conventions at
     * some later date. We also limit the characters than can go into a
     * name.
     */

    if ( strlen ( name ) >= SBUF_SIZE ) {
        return -1;
    }

    if ( !isalpha ( *name ) ) {
        return -1;
    }

    for ( p = name; *p; p++ ) {
        if ( isalnum ( *p ) ||
                ( *p == '_' ) || ( *p == '-' ) || ( *p == '.' ) || ( *p == '#' ) ) {
            *p = tolower ( *p );
        } else {
            return -1;
        }
    }

    len = strlen ( data );

    /*
     * If we have no existing data, life is easy; just set it.
     */

    if ( !mudstate.rdata ) {
        Init_RegData ( funcname, mudstate.rdata );
    }

    if ( !mudstate.rdata->xr_alloc ) {
        a_size = NUM_ENV_VARS;
        mudstate.rdata->x_names =
            xcalloc ( a_size, sizeof ( char * ), "x_names" );
        mudstate.rdata->x_regs =
            xcalloc ( a_size, sizeof ( char * ), "x_regs" );
        mudstate.rdata->x_lens =
            xcalloc ( a_size, sizeof ( int ), "x_lens" );
        mudstate.rdata->xr_alloc = a_size;
        mudstate.rdata->x_names[0] = alloc_sbuf ( funcname );
        strcpy ( mudstate.rdata->x_names[0], name );
        mudstate.rdata->x_regs[0] = alloc_lbuf ( funcname );
        memcpy ( mudstate.rdata->x_regs[0], data, len + 1 );
        mudstate.rdata->x_lens[0] = len;
        mudstate.rdata->dirty++;
        return len;
    }

    /*
     * Search for an existing entry to replace.
     */

    for ( i = 0; i < mudstate.rdata->xr_alloc; i++ ) {
        if ( mudstate.rdata->x_names[i] &&
                !strcmp ( name, mudstate.rdata->x_names[i] ) ) {
            memcpy ( mudstate.rdata->x_regs[i], data, len + 1 );
            mudstate.rdata->x_lens[i] = len;
            mudstate.rdata->dirty++;
            return len;
        }
    }

    /*
     * Check for an empty cell to insert into.
     */

    for ( i = 0; i < mudstate.rdata->xr_alloc; i++ ) {
        if ( mudstate.rdata->x_names[i] == NULL ) {
            mudstate.rdata->x_names[i] = alloc_sbuf ( funcname );
            strcpy ( mudstate.rdata->x_names[i], name );

            if ( !mudstate.rdata->x_regs[i] )   /* should never happen */
                mudstate.rdata->x_regs[i] =
                    alloc_lbuf ( funcname );

            memcpy ( mudstate.rdata->x_regs[i], data, len + 1 );
            mudstate.rdata->x_lens[i] = len;
            mudstate.rdata->dirty++;
            return len;
        }
    }

    /*
     * Oops. We're out of room in our existing array. Go allocate more
     * space, unless we're at our limit.
     */
    regnum = mudstate.rdata->xr_alloc;
    a_size = regnum + NUM_ENV_VARS;

    if ( a_size > mudconf.register_limit ) {
        a_size = mudconf.register_limit;

        if ( a_size <= regnum ) {
            return -2;
        }
    }

    tmp_regs = ( char ** ) xrealloc ( mudstate.rdata->x_names,
                                      a_size * sizeof ( char * ), funcname );
    mudstate.rdata->x_names = tmp_regs;
    tmp_regs = ( char ** ) xrealloc ( mudstate.rdata->x_regs,
                                      a_size * sizeof ( char * ), funcname );
    mudstate.rdata->x_regs = tmp_regs;
    tmp_lens = ( int * ) xrealloc ( mudstate.rdata->x_lens,
                                    a_size * sizeof ( int ), funcname );
    mudstate.rdata->x_lens = tmp_lens;
    memset ( &mudstate.rdata->x_names[mudstate.rdata->xr_alloc], ( int ) 0, ( a_size - mudstate.rdata->xr_alloc ) * sizeof ( char * ) );
    memset ( &mudstate.rdata->x_regs[mudstate.rdata->xr_alloc], ( int ) 0, ( a_size - mudstate.rdata->xr_alloc ) * sizeof ( char * ) );
    memset ( &mudstate.rdata->x_lens[mudstate.rdata->xr_alloc], ( int ) 0, ( a_size - mudstate.rdata->xr_alloc ) * sizeof ( int ) );
    mudstate.rdata->xr_alloc = a_size;
    /*
     * Now we know we can insert into the first empty.
     */
    mudstate.rdata->x_names[regnum] = alloc_sbuf ( funcname );
    strcpy ( mudstate.rdata->x_names[regnum], name );
    mudstate.rdata->x_regs[regnum] = alloc_lbuf ( funcname );
    memcpy ( mudstate.rdata->x_regs[regnum], data, len + 1 );
    mudstate.rdata->x_lens[regnum] = len;
    mudstate.rdata->dirty++;
    return len;
}

static char *get_register ( GDATA *g, char *r )
{
    /*
     * Given a pointer to a register data structure, and the name of a
     * register, return a pointer to the string value of that register.
     * This may modify r, turning it lowercase.
     */
    int regnum;
    char *p;

    if ( !g || !r || !*r ) {
        return NULL;
    }

    if ( r[1] == '\0' ) {
        regnum = qidx_chartab[ ( unsigned char ) r[0]];

        if ( ( regnum < 0 ) || ( regnum >= mudconf.max_global_regs ) ) {
            return NULL;
        } else if ( ( g->q_alloc > regnum ) && g->q_regs[regnum] ) {
            return g->q_regs[regnum];
        }

        return NULL;
    }

    if ( !g->xr_alloc ) {
        return NULL;
    }

    for ( p = r; *p; p++ ) {
        *p = tolower ( *p );
    }

    for ( regnum = 0; regnum < g->xr_alloc; regnum++ ) {
        if ( g->x_names[regnum] && !strcmp ( r, g->x_names[regnum] ) ) {
            if ( g->x_regs[regnum] ) {
                return g->x_regs[regnum];
            }

            return NULL;
        }
    }

    return NULL;
}

void fun_setq ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int result, count, i;

    if ( nfargs < 2 ) {
        safe_sprintf ( buff, bufc, "#-1 FUNCTION (SETQ) EXPECTS AT LEAST 2 ARGUMENTS BUT GOT %d", nfargs );
        return;
    }

    if ( nfargs % 2 != 0 ) {
        safe_sprintf ( buff, bufc, "#-1 FUNCTION (SETQ) EXPECTS AN EVEN NUMBER OF ARGUMENTS BUT GOT %d", nfargs );
        return;
    }

    if ( nfargs > MAX_NFARGS - 2 ) {
        /*
         * Prevent people from doing something dumb by providing this
         * too many arguments and thus having the fifteenth register
         * contain the remaining args. Cut them off at the
         * fourteenth.
         */
        safe_sprintf ( buff, bufc, "#-1 FUNCTION (SETQ) EXPECTS NO MORE THAN %d ARGUMENTS BUT GOT %d", MAX_NFARGS - 2, nfargs );
        return;
    }

    if ( nfargs == 2 ) {
        result = set_register ( "fun_setq", fargs[0], fargs[1] );

        if ( result == -1 ) {
            safe_str ( "#-1 INVALID GLOBAL REGISTER", buff, bufc );
        } else if ( result == -2 ) {
            safe_str ( "#-1 REGISTER LIMIT EXCEEDED", buff, bufc );
        }

        return;
    }

    count = 0;

    for ( i = 0; i < nfargs; i += 2 ) {
        result = set_register ( "fun_setq", fargs[i], fargs[i + 1] );

        if ( result < 0 ) {
            count++;
        }
    }

    if ( count > 0 ) {
        safe_sprintf ( buff, bufc, "#-1 ENCOUNTERED %d ERRORS", count );
    }
}

void fun_setr ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int result;
    result = set_register ( "fun_setr", fargs[0], fargs[1] );

    if ( result == -1 ) {
        safe_str ( "#-1 INVALID GLOBAL REGISTER", buff, bufc );
    } else if ( result == -2 ) {
        safe_str ( "#-1 REGISTER LIMIT EXCEEDED", buff, bufc );
    } else if ( result > 0 ) {
        safe_known_str ( fargs[1], result, buff, bufc );
    }
}

static void read_register ( char *regname, char *buff, char **bufc )
{
    int regnum;
    char *p;

    if ( regname[1] == '\0' ) {
        regnum = qidx_chartab[ ( unsigned char ) * regname];

        if ( ( regnum < 0 ) || ( regnum >= mudconf.max_global_regs ) ) {
            safe_str ( "#-1 INVALID GLOBAL REGISTER", buff, bufc );
        } else {
            if ( mudstate.rdata
                    && ( mudstate.rdata->q_alloc > regnum )
                    && mudstate.rdata->q_regs[regnum] ) {
                safe_known_str ( mudstate.rdata->q_regs[regnum],
                                 mudstate.rdata->q_lens[regnum], buff,
                                 bufc );
            }
        }

        return;
    }

    if ( !mudstate.rdata || !mudstate.rdata->xr_alloc ) {
        return;
    }

    for ( p = regname; *p; p++ ) {
        *p = tolower ( *p );
    }

    for ( regnum = 0; regnum < mudstate.rdata->xr_alloc; regnum++ ) {
        if ( mudstate.rdata->x_names[regnum] &&
                !strcmp ( regname, mudstate.rdata->x_names[regnum] ) ) {
            if ( mudstate.rdata->x_regs[regnum] ) {
                safe_known_str ( mudstate.rdata->x_regs[regnum],
                                 mudstate.rdata->x_lens[regnum],
                                 buff, bufc );
                return;
            }
        }
    }
}

void fun_r ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    read_register ( fargs[0], buff, bufc );
}

/*
 * --------------------------------------------------------------------------
 * lregs: List all the non-empty q-registers.
 */

void fun_lregs ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i;
    GDATA *g;
    char *bb_p;

    if ( !mudstate.rdata ) {
        return;
    }

    bb_p = *bufc;
    g = mudstate.rdata;

    for ( i = 0; i < g->q_alloc; i++ ) {
        if ( g->q_regs[i] && * ( g->q_regs[i] ) ) {
            if ( *bufc != bb_p ) {
                print_sep ( &SPACE_DELIM, buff, bufc );
            }

            safe_chr ( qidx_str[i], buff, bufc );
        }
    }

    for ( i = 0; i < g->xr_alloc; i++ ) {
        if ( g->x_names[i] && * ( g->x_names[i] ) &&
                g->x_regs[i] && * ( g->x_regs[i] ) ) {
            if ( *bufc != bb_p ) {
                print_sep ( &SPACE_DELIM, buff, bufc );
            }

            safe_str ( g->x_names[i], buff, bufc );
        }
    }
}

/*
 * --------------------------------------------------------------------------
 * wildmatch: Set the results of a wildcard match into the global registers.
 * wildmatch(<string>,<wildcard pattern>,<register list>)
 */

void fun_wildmatch ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i, nqregs;
    char *t_args[NUM_ENV_VARS], **qregs;    /* %0-%9 is limiting */

    if ( !wild ( fargs[1], fargs[0], t_args, NUM_ENV_VARS ) ) {
        safe_chr ( '0', buff, bufc );
        return;
    }

    safe_chr ( '1', buff, bufc );
    /*
     * Parse the list of registers. Anything that we don't get is assumed
     * to be -1. Fill them in.
     */
    nqregs = list2arr ( &qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM );

    for ( i = 0; i < nqregs; i++ ) {
        set_register ( "fun_wildmatch", qregs[i], t_args[i] );
    }

    /*
     * Need to free up allocated memory from the match.
     */

    for ( i = 0; i < NUM_ENV_VARS; i++ ) {
        if ( t_args[i] ) {
            free_lbuf ( t_args[i] );
        }
    }

    xfree ( qregs, "fun_wildmatch.qregs" );
}

/*
 * --------------------------------------------------------------------------
 * qvars: Set the contents of a list into a named list of global registers
 * qvars(<register list>,<list of elements>[,<input delim>])
 */

void fun_qvars ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i, nqregs, n_elems;
    char **qreg_names, **elems;
    char *varlist, *elemlist;
    Delim isep;
    VaChk_Only_In ( 3 );

    if ( !fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1] ) {
        return;
    }

    varlist = alloc_lbuf ( "fun_qvars.vars" );
    strcpy ( varlist, fargs[0] );
    nqregs = list2arr ( &qreg_names, LBUF_SIZE / 2, varlist, &SPACE_DELIM );

    if ( nqregs == 0 ) {
        free_lbuf ( varlist );
        xfree ( qreg_names, "fun_qvars.qreg_names" );
        return;
    }

    elemlist = alloc_lbuf ( "fun_qvars.elems" );
    strcpy ( elemlist, fargs[1] );
    n_elems = list2arr ( &elems, LBUF_SIZE / 2, elemlist, &isep );

    if ( n_elems != nqregs ) {
        safe_str ( "#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc );
        free_lbuf ( varlist );
        free_lbuf ( elemlist );
        xfree ( qreg_names, "fun_qvars.qreg_names" );
        xfree ( elems, "fun_qvars.elems" );
        return;
    }

    for ( i = 0; i < n_elems; i++ ) {
        set_register ( "fun_qvars", qreg_names[i], elems[i] );
    }

    free_lbuf ( varlist );
    free_lbuf ( elemlist );
    xfree ( qreg_names, "fun_qvars.qreg_names" );
    xfree ( elems, "fun_qvars.elems" );
}

/*---------------------------------------------------------------------------
 * fun_qsub: "Safe" substitution using $name$ dollar-variables.
 *           Can specify beginning and ending variable markers.
 */

void fun_qsub ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char *nextp, *strp;
    Delim bdelim, edelim;
    VaChk_Range ( 0, 3 );

    if ( !fargs[0] || !*fargs[0] ) {
        return;
    }

    if ( !delim_check ( buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 2, &bdelim, DELIM_STRING ) ) {
        return;
    }

    if ( !delim_check ( buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &edelim, DELIM_STRING ) ) {
        return;
    }

    /*
     * Defaulted space delims are actually $
     */
    if ( ( bdelim.len == 1 ) && ( bdelim.str[0] == ' ' ) ) {
        bdelim.str[0] = '$';
    }

    if ( ( edelim.len == 1 ) && ( edelim.str[0] == ' ' ) ) {
        edelim.str[0] = '$';
    }

    nextp = fargs[0];

    while ( nextp && ( ( strp = split_token ( &nextp, &bdelim ) ) != NULL ) ) {
        safe_str ( strp, buff, bufc );

        if ( nextp ) {
            strp = split_token ( &nextp, &edelim );
            read_register ( strp, buff, bufc );
        }
    }
}

/*---------------------------------------------------------------------------
 * fun_nofx: Prevent certain types of side-effects.
 */

static int calc_limitmask ( char *lstr )
{
    char *p;
    int lmask = 0;

    for ( p = lstr; *p; p++ ) {
        switch ( *p ) {
        case 'd':
        case 'D':
            lmask |= FN_DBFX;
            break;

        case 'q':
        case 'Q':
            lmask |= FN_QFX;
            break;

        case 'o':
        case 'O':
            lmask |= FN_OUTFX;
            break;

        case 'v':
        case 'V':
            lmask |= FN_VARFX;
            break;

        case 's':
        case 'S':
            lmask |= FN_STACKFX;
            break;

        case ' ':
            /*
             * ignore spaces
             */
            /*
             * EMPTY
             */
            break;

        default:
            return -1;
        }
    }

    return lmask;
}

void fun_nofx ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int save_state, lmask;
    char *str;
    lmask = calc_limitmask ( fargs[0] );

    if ( lmask == -1 ) {
        safe_known_str ( "#-1 INVALID LIMIT", 17, buff, bufc );
        return;
    }

    save_state = mudstate.f_limitmask;
    mudstate.f_limitmask |= lmask;
    str = fargs[1];
    exec ( buff, bufc, player, caller, cause,
           EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs );
    mudstate.f_limitmask = save_state;
}

/*
 * ---------------------------------------------------------------------------
 * ucall: Call a u-function, passing through only certain local registers,
 * and restoring certain local registers afterwards.
 *
 * ucall(<register names to pass thru>,<registers to keep local>,<o/a>,<args>)
 * sandbox(<object>,<limiter>,<reg pass>,<reg keep>,<o/a>,<args>)
 *
 * Registers to pass through to function @_ to pass through all, @_ <list> to
 * pass through all except <list> blank to pass through none, <list> to pass
 * through just those Registers whose value should be local (restored to
 * value pre-function call) @_ to restore all, @_ <list> to restore all
 * except <list> blank to restore none, <list> to keep just those on the list
 * @_! to return to original values (like a uprivate() would) @_! <list> to
 * return to original values, keeping new values on <list>
 */

static char is_in_array ( char *word, char **list, int list_length )
{
    int n;

    for ( n = 0; n < list_length; n++ )
        if ( !strcasecmp ( word, list[n] ) ) {
            return 1;
        }

    return 0;
}

void handle_ucall ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref aowner, thing, obj;
    int aflags, alen, anum, trace_flag, i, ncregs;
    int is_sandbox, lmask, save_state;
    ATTR *ap;
    char *atext, *str, *callp, *call_list;
    char **cregs;
    char cbuf[2];
    GDATA *preserve, *tmp;
    is_sandbox = Is_Func ( UCALL_SANDBOX );

    /*
     * Three arguments to ucall(), five to sandbox()
     */

    if ( nfargs < 3 ) {
        safe_known_str ( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
        return;
    }

    if ( is_sandbox && ( nfargs < 5 ) ) {
        safe_known_str ( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
        return;
    }

    /*
     * Figure our our limits
     */

    if ( is_sandbox ) {
        lmask = calc_limitmask ( fargs[1] );

        if ( lmask == -1 ) {
            safe_known_str ( "#-1 INVALID LIMIT", 17, buff, bufc );
            return;
        }

        save_state = mudstate.f_limitmask;
        mudstate.f_limitmask |= lmask;
    }

    /*
     * Save everything to start with, then construct our pass-in
     */
    preserve = save_global_regs ( "fun_ucall.save" );

    if ( is_sandbox ) {
        callp = Eat_Spaces ( fargs[2] );
    } else {
        callp = Eat_Spaces ( fargs[0] );
    }

    if ( !*callp ) {
        Free_RegData ( mudstate.rdata );
        mudstate.rdata = NULL;
    } else if ( !strcmp ( callp, "@_" ) ) {
        /*
         * Pass everything in
         */
        /*
         * EMPTY
         */
    } else if ( !strncmp ( callp, "@_ ", 3 ) && callp[3] ) {
        /*
         * Pass in everything EXCEPT the named registers
         */
        call_list = alloc_lbuf ( "fun_ucall.call_list" );
        strcpy ( call_list, callp + 3 );
        ncregs =
            list2arr ( &cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM );

        for ( i = 0; i < ncregs; i++ ) {
            set_register ( "fun_ucall", cregs[i], NULL );
        }

        free_lbuf ( call_list );
    } else {
        /*
         * Pass in ONLY the named registers
         */
        Free_RegData ( mudstate.rdata );
        mudstate.rdata = NULL;
        call_list = alloc_lbuf ( "fun_ucall.call_list" );
        strcpy ( call_list, callp );
        ncregs =
            list2arr ( &cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM );

        for ( i = 0; i < ncregs; i++ ) {
            set_register ( "fun_ucall", cregs[i],
                           get_register ( preserve, cregs[i] ) );
        }

        free_lbuf ( call_list );
    }

    /*
     * What to call: <obj>/<attr> or <attr> or #lambda/<code>
     */
    Get_Ulambda ( player, thing, ( is_sandbox ) ? fargs[4] : fargs[2],
                  anum, ap, atext, aowner, aflags, alen );

    /*
     * Find our perspective
     */

    if ( is_sandbox ) {
        obj = match_thing ( player, fargs[0] );

        if ( Cannot_Objeval ( player, obj ) ) {
            obj = player;
        }
    } else {
        obj = thing;
    }

    /*
     * If the trace flag is on this attr, set the object Trace
     */

    if ( !Trace ( obj ) && ( aflags & AF_TRACE ) ) {
        trace_flag = 1;
        s_Trace ( obj );
    } else {
        trace_flag = 0;
    }

    /*
     * Evaluate it using the rest of the passed function args
     */
    str = atext;
    exec ( buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str,
           ( is_sandbox ) ? & ( fargs[5] ) : & ( fargs[3] ),
           nfargs - ( ( is_sandbox ) ? 5 : 3 ) );
    free_lbuf ( atext );

    /*
     * Reset the trace flag if we need to
     */

    if ( trace_flag ) {
        c_Trace ( obj );
    }

    /*
     * Restore / clean registers
     */

    if ( is_sandbox ) {
        callp = Eat_Spaces ( fargs[3] );
    } else {
        callp = Eat_Spaces ( fargs[1] );
    }

    if ( !*callp ) {
        /*
         * Restore nothing, so we keep our data as-is.
         */
        Free_RegData ( preserve );
    } else if ( !strncmp ( callp, "@_!", 3 ) &&
                ( ( callp[3] == '\0' ) || ( callp[3] == ' ' ) ) ) {
        if ( callp[3] == '\0' ) {
            /*
             * Clear out all data
             */
            restore_global_regs ( "fun_ucall.restore", preserve );
        } else {
            /*
             * Go back to the original registers, but ADD BACK IN
             * the new values of the registers on the list.
             */
            tmp = preserve;
            preserve = mudstate.rdata;  /* preserve is now the
                             * new vals */
            mudstate.rdata = tmp;   /* this is now the original
                         * vals */
            call_list = alloc_lbuf ( "fun_ucall.call_list" );
            strcpy ( call_list, callp + 4 );
            ncregs = list2arr ( &cregs, LBUF_SIZE / 2, call_list,
                                &SPACE_DELIM );

            for ( i = 0; i < ncregs; i++ ) {
                set_register ( "fun_ucall", cregs[i],
                               get_register ( preserve, cregs[i] ) );
            }

            free_lbuf ( call_list );
            Free_RegData ( preserve );
        }
    } else if ( !strncmp ( callp, "@_", 2 ) &&
                ( ( callp[2] == '\0' ) || ( callp[2] == ' ' ) ) ) {
        if ( callp[2] == '\0' ) {
            /*
             * Restore all registers we had before
             */
            call_list = NULL;
        } else {
            /*
             * Restore all registers EXCEPT the ones listed. We
             * assume that this list is going to be pretty short,
             * so we can do a crude, unsorted search.
             */
            call_list = alloc_lbuf ( "fun_ucall.call_list" );
            strcpy ( call_list, callp + 3 );
            ncregs = list2arr ( &cregs, LBUF_SIZE / 2, call_list,
                                &SPACE_DELIM );
        }

        for ( i = 0; i < preserve->q_alloc; i++ ) {
            if ( preserve->q_regs[i] && * ( preserve->q_regs[i] ) ) {
                cbuf[0] = qidx_str[i];
                cbuf[1] = '\0';

                if ( !call_list
                        || !is_in_array ( cbuf, cregs, ncregs ) )
                    set_register ( "fun_ucall", cbuf,
                                   preserve->q_regs[i] );
            }
        }

        for ( i = 0; i < preserve->xr_alloc; i++ ) {
            if ( preserve->x_names[i] && * ( preserve->x_names[i] ) &&
                    preserve->x_regs[i] && * ( preserve->x_regs[i] ) ) {
                if ( !call_list ||
                        !is_in_array ( preserve->x_names[i], cregs,
                                       ncregs ) ) {
                    set_register ( "fun_ucall",
                                   preserve->x_names[i],
                                   preserve->x_regs[i] );
                }
            }
        }

        if ( call_list != NULL ) {
            free_lbuf ( call_list );
        }

        Free_RegData ( preserve );
    } else {
        /*
         * Restore ONLY these named registers
         */
        call_list = alloc_lbuf ( "fun_ucall.call_list" );
        strcpy ( call_list, callp );
        ncregs =
            list2arr ( &cregs, LBUF_SIZE / 2, call_list, &SPACE_DELIM );

        for ( i = 0; i < ncregs; i++ ) {
            set_register ( "fun_ucall", cregs[i],
                           get_register ( preserve, cregs[i] ) );
        }

        free_lbuf ( call_list );
        Free_RegData ( preserve );
    }

    if ( is_sandbox ) {
        mudstate.f_limitmask = save_state;
    }
}

/*
 * --------------------------------------------------------------------------
 * Auxiliary stuff for structures and variables.
 */

#define Set_Max(x,y)     (x) = ((y) > (x)) ? (y) : (x);

static void print_htab_matches ( dbref obj, HASHTAB *htab, char *buff, char **bufc )
{
    /*
     * Lists out hashtable matches. Things which use this are
     * computationally expensive, and should be discouraged.
     */
    char tbuf[SBUF_SIZE], *tp, *bb_p;
    HASHENT *hptr;
    int i, len;
    tp = tbuf;
    safe_ltos ( tbuf, &tp, obj );
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';
    len = strlen ( tbuf );
    bb_p = *bufc;

    for ( i = 0; i < htab->hashsize; i++ ) {
        for ( hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next ) {
            if ( !strncmp ( tbuf, hptr->target.s, len ) ) {
                if ( *bufc != bb_p ) {
                    safe_chr ( ' ', buff, bufc );
                }

                safe_str ( strchr ( hptr->target.s, '.' ) + 1, buff,
                           bufc );
            }
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * fun_x: Returns a variable. x(<variable name>) fun_setx: Sets a variable.
 * setx(<variable name>,<value>) fun_store: Sets and returns a variable.
 * store(<variable name>,<value>) fun_xvars: Takes a list, parses it, sets it
 * into variables. xvars(<space-separated variable list>,<list>,<delimiter>)
 * fun_let: Takes a list of variables and their values, sets them, executes a
 * function, and clears out the variables. (Scheme/ML-like.) If <list> is
 * empty, the values are reset to null. let(<space-separated var
 * list>,<list>,<body>,<delimiter>) fun_lvars: Shows a list of variables
 * associated with that object. fun_clearvars: Clears all variables
 * associated with that object.
 */


void set_xvar ( dbref obj, char *name, char *data )
{
    VARENT *xvar;
    char tbuf[SBUF_SIZE], *tp, *p;

    /*
     * If we don't have at least one character in the name, toss it.
     */

    if ( !name || !*name ) {
        return;
    }

    /*
     * Variable string is '<dbref number minus #>.<variable name>'. We
     * lowercase all names. Note that we're going to end up automatically
     * truncating long names.
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, obj );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = name; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( name, tbuf, &tp );
    *tp = '\0';

    /*
     * Search for it. If it exists, replace it. If we get a blank string,
     * delete the variable.
     */

    if ( ( xvar = ( VARENT * ) hashfind ( tbuf, &mudstate.vars_htab ) ) ) {
        if ( xvar->text ) {
            xfree ( xvar->text, "xvar_data" );
        }

        if ( data && *data ) {
            xvar->text =
                ( char * ) xmalloc ( sizeof ( char ) * ( strlen ( data ) + 1 ),
                                     "xvar_data" );

            if ( !xvar->text ) {
                return;    /* out of memory */
            }

            strcpy ( xvar->text, data );
        } else {
            xvar->text = NULL;
            xfree ( xvar, "xvar_struct" );
            hashdelete ( tbuf, &mudstate.vars_htab );
            s_VarsCount ( obj, VarsCount ( obj ) - 1 );
        }
    } else {
        /*
         * We haven't found it. If it's non-empty, set it, provided
         * we're not running into a limit on the number of vars per
         * object.
         */
        if ( VarsCount ( obj ) + 1 > mudconf.numvars_lim ) {
            return;
        }

        if ( data && *data ) {
            xvar =
                ( VARENT * ) xmalloc ( sizeof ( VARENT ), "xvar_struct" );

            if ( !xvar ) {
                return;    /* out of memory */
            }

            xvar->text =
                ( char * ) xmalloc ( sizeof ( char ) * ( strlen ( data ) + 1 ),
                                     "xvar_data" );

            if ( !xvar->text ) {
                return;    /* out of memory */
            }

            strcpy ( xvar->text, data );
            hashadd ( tbuf, ( int * ) xvar, &mudstate.vars_htab, 0 );
            s_VarsCount ( obj, VarsCount ( obj ) + 1 );
            Set_Max ( mudstate.max_vars, mudstate.vars_htab.entries );
        }
    }
}


static void clear_xvars ( dbref obj, char **xvar_names, int n_xvars )
{
    /*
     * Clear out an array of variable names.
     */
    char pre[SBUF_SIZE], tbuf[SBUF_SIZE], *tp, *p;
    VARENT *xvar;
    int i;
    /*
     * Build our dbref bit first.
     */
    tp = pre;
    safe_ltos ( pre, &tp, obj );
    safe_sb_chr ( '.', pre, &tp );
    *tp = '\0';

    /*
     * Go clear stuff.
     */

    for ( i = 0; i < n_xvars; i++ ) {
        for ( p = xvar_names[i]; *p; p++ ) {
            *p = tolower ( *p );
        }

        tp = tbuf;
        safe_sb_str ( pre, tbuf, &tp );
        safe_sb_str ( xvar_names[i], tbuf, &tp );
        *tp = '\0';

        if ( ( xvar = ( VARENT * ) hashfind ( tbuf, &mudstate.vars_htab ) ) ) {
            if ( xvar->text ) {
                xfree ( xvar->text, "xvar_data" );
                xvar->text = NULL;
            }

            xfree ( xvar, "xvar_struct" );
            hashdelete ( tbuf, &mudstate.vars_htab );
        }
    }

    s_VarsCount ( obj, VarsCount ( obj ) - n_xvars );
}


void xvars_clr ( dbref player )
{
    char tbuf[SBUF_SIZE], *tp;
    HASHTAB *htab;
    HASHENT *hptr, *last, *next;
    int i, len;
    VARENT *xvar;
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';
    len = strlen ( tbuf );
    htab = &mudstate.vars_htab;

    for ( i = 0; i < htab->hashsize; i++ ) {
        last = NULL;

        for ( hptr = htab->entry[i]; hptr != NULL; hptr = next ) {
            next = hptr->next;

            if ( !strncmp ( tbuf, hptr->target.s, len ) ) {
                if ( last == NULL ) {
                    htab->entry[i] = next;
                } else {
                    last->next = next;
                }

                xvar = ( VARENT * ) hptr->data;
                xfree ( xvar->text, "xvar_data" );
                xfree ( xvar, "xvar_struct" );
                xfree ( hptr->target.s, "xvar_hptr_target" );
                xfree ( hptr, "xvar_hptr" );
                htab->deletes++;
                htab->entries--;

                if ( htab->entry[i] == NULL ) {
                    htab->nulls++;
                }
            } else {
                last = hptr;
            }
        }
    }

    s_VarsCount ( player, 0 );
}


void fun_x ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    VARENT *xvar;
    char tbuf[SBUF_SIZE], *tp, *p;
    /*
     * Variable string is '<dbref number minus #>.<variable name>'
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], tbuf, &tp );
    *tp = '\0';

    if ( ( xvar = ( VARENT * ) hashfind ( tbuf, &mudstate.vars_htab ) ) ) {
        safe_str ( xvar->text, buff, bufc );
    }
}


void fun_setx ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    set_xvar ( player, fargs[0], fargs[1] );
}

void fun_store ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    set_xvar ( player, fargs[0], fargs[1] );
    safe_str ( fargs[1], buff, bufc );
}

void fun_xvars ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char **xvar_names, **elems;
    int n_xvars, n_elems;
    char *varlist, *elemlist;
    int i;
    Delim isep;
    VaChk_Only_In ( 3 );
    varlist = alloc_lbuf ( "fun_xvars.vars" );
    strcpy ( varlist, fargs[0] );
    n_xvars = list2arr ( &xvar_names, LBUF_SIZE / 2, varlist, &SPACE_DELIM );

    if ( n_xvars == 0 ) {
        free_lbuf ( varlist );
        xfree ( xvar_names, "fun_xvars.xvar_names" );
        return;
    }

    if ( !fargs[1] || !*fargs[1] ) {
        /*
         * Empty list, clear out the data.
         */
        clear_xvars ( player, xvar_names, n_xvars );
        free_lbuf ( varlist );
        xfree ( xvar_names, "fun_xvars.xvar_names" );
        return;
    }

    elemlist = alloc_lbuf ( "fun_xvars.elems" );
    strcpy ( elemlist, fargs[1] );
    n_elems = list2arr ( &elems, LBUF_SIZE / 2, elemlist, &isep );

    if ( n_elems != n_xvars ) {
        safe_str ( "#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc );
        free_lbuf ( varlist );
        free_lbuf ( elemlist );
        xfree ( xvar_names, "fun_xvars.xvar_names" );
        xfree ( elems, "fun_xvars.elems" );
        return;
    }

    for ( i = 0; i < n_elems; i++ ) {
        set_xvar ( player, xvar_names[i], elems[i] );
    }

    free_lbuf ( varlist );
    free_lbuf ( elemlist );
    xfree ( xvar_names, "fun_xvars.xvar_names" );
    xfree ( elems, "fun_xvars.elems" );
}


void fun_let ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char **xvar_names, **elems;
    char *old_xvars[LBUF_SIZE / 2];
    int n_xvars, n_elems;
    char *varlist, *elemlist;
    char *str, *bp, *p;
    char pre[SBUF_SIZE], tbuf[SBUF_SIZE], *tp;
    VARENT *xvar;
    int i;
    Delim isep;
    VaChk_Only_In ( 4 );

    if ( !fargs[0] || !*fargs[0] ) {
        return;
    }

    varlist = bp = alloc_lbuf ( "fun_let.vars" );
    str = fargs[0];
    exec ( varlist, &bp, player, caller, cause,
           EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs );
    n_xvars = list2arr ( &xvar_names, LBUF_SIZE / 2, varlist, &SPACE_DELIM );

    if ( n_xvars == 0 ) {
        free_lbuf ( varlist );
        xfree ( xvar_names, "fun_let.xvar_names" );
        return;
    }

    /*
     * Lowercase our variable names.
     */

    for ( i = 0, p = xvar_names[i]; *p; p++ ) {
        *p = tolower ( *p );
    }

    /*
     * Save our original values. Copying this stuff into an array is
     * unnecessarily expensive because we allocate and free memory that
     * we could theoretically just trade pointers around for -- but this
     * way is cleaner.
     */
    tp = pre;
    safe_ltos ( pre, &tp, player );
    safe_sb_chr ( '.', pre, &tp );
    *tp = '\0';

    for ( i = 0; i < n_xvars; i++ ) {
        tp = tbuf;
        safe_sb_str ( pre, tbuf, &tp );
        safe_sb_str ( xvar_names[i], tbuf, &tp );
        *tp = '\0';

        if ( ( xvar = ( VARENT * ) hashfind ( tbuf, &mudstate.vars_htab ) ) ) {
            if ( xvar->text )
                old_xvars[i] =
                    xstrdup ( xvar->text, "fun_let.preserve" );
            else {
                old_xvars[i] = NULL;
            }
        } else {
            old_xvars[i] = NULL;
        }
    }

    if ( fargs[1] && *fargs[1] ) {
        /*
         * We have data, so we should initialize variables to their
         * values, ala xvars(). However, unlike xvars(), if we don't
         * get a list, we just leave the values alone (we don't clear
         * them out).
         */
        elemlist = bp = alloc_lbuf ( "fun_let.elemlist" );
        str = fargs[1];
        exec ( elemlist, &bp, player, caller, cause,
               EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs );
        n_elems = list2arr ( &elems, LBUF_SIZE / 2, elemlist, &isep );

        if ( n_elems != n_xvars ) {
            safe_str ( "#-1 LIST MUST BE OF EQUAL SIZE", buff, bufc );
            free_lbuf ( varlist );
            free_lbuf ( elemlist );

            for ( i = 0; i < n_xvars; i++ ) {
                if ( old_xvars[i] ) {
                    xfree ( old_xvars[i], "fun_let" );
                }
            }

            xfree ( xvar_names, "fun_let.xvar_names" );
            xfree ( elems, "fun_let.elems" );
            return;
        }

        for ( i = 0; i < n_elems; i++ ) {
            set_xvar ( player, xvar_names[i], elems[i] );
        }

        free_lbuf ( elemlist );
    }

    /*
     * Now we go to execute our function body.
     */
    str = fargs[2];
    exec ( buff, bufc, player, caller, cause,
           EV_FCHECK | EV_STRIP | EV_EVAL, &str, cargs, ncargs );

    /*
     * Restore the old values.
     */

    for ( i = 0; i < n_xvars; i++ ) {
        set_xvar ( player, xvar_names[i], old_xvars[i] );

        if ( old_xvars[i] ) {
            xfree ( old_xvars[i], "fun_let" );
        }
    }

    free_lbuf ( varlist );
    xfree ( xvar_names, "fun_let.xvar_names" );
    xfree ( elems, "fun_let.elems" );
}


void fun_lvars ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    print_htab_matches ( player, &mudstate.vars_htab, buff, bufc );
}


void fun_clearvars ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    /*
     * This is computationally expensive. Necessary, but its use should
     * be avoided if possible.
     */
    xvars_clr ( player );
}

/*
 * ---------------------------------------------------------------------------
 * Structures.
 */

static int istype_char ( char *str )
{
    if ( strlen ( str ) == 1 ) {
        return 1;
    } else {
        return 0;
    }
}

static int istype_dbref ( char *str )
{
    dbref it;

    if ( *str++ != NUMBER_TOKEN ) {
        return 0;
    }

    if ( *str ) {
        it = parse_dbref_only ( str );
        return ( Good_obj ( it ) );
    }

    return 0;
}

static int istype_int ( char *str )
{
    return ( is_integer ( str ) );
}

static int istype_float ( char *str )
{
    return ( is_number ( str ) );
}

static int istype_string ( char *str )
{
    char *p;

    for ( p = str; *p; p++ ) {
        if ( isspace ( *p ) ) {
            return 0;
        }
    }

    return 1;
}


void fun_structure ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep;     /* delim for default values */
    Delim osep;     /* output delim for structure values */
    char tbuf[SBUF_SIZE], *tp;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    char *comp_names, *type_names, *default_vals;
    char **comp_array, **type_array, **def_array;
    int n_comps, n_types, n_defs;
    int i;
    STRUCTDEF *this_struct;
    COMPONENT *this_comp;
    int check_type = 0;
    VaChk_Only_In_Out ( 6 );

    /*
     * Prevent null delimiters and line delimiters.
     */

    if ( ( osep.len > 1 ) || ( osep.str[0] == '\0' ) || ( osep.str[0] == '\r' ) ) {
        notify_quiet ( player, "You cannot use that output delimiter." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Enforce limits.
     */

    if ( StructCount ( player ) > mudconf.struct_lim ) {
        notify_quiet ( player, "Too many structures." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * If our structure name is too long, reject it.
     */

    if ( strlen ( fargs[0] ) > ( SBUF_SIZE / 2 ) - 9 ) {
        notify_quiet ( player, "Structure name is too long." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * No periods in structure names
     */

    if ( strchr ( fargs[0], '.' ) ) {
        notify_quiet ( player,
                       "Structure names cannot contain periods." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * The hashtable is indexed by <dbref number>.<structure name>
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], tbuf, &tp );
    *tp = '\0';

    /*
     * If we have this structure already, reject.
     */

    if ( hashfind ( tbuf, &mudstate.structs_htab ) ) {
        notify_quiet ( player, "Structure is already defined." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Split things up. Make sure lists are the same size. If everything
     * eventually goes well, comp_names and default_vals will REMAIN
     * allocated.
     */
    comp_names = xstrdup ( fargs[1], "struct.comps" );
    n_comps =
        list2arr ( &comp_array, LBUF_SIZE / 2, comp_names, &SPACE_DELIM );

    if ( n_comps < 1 ) {
        notify_quiet ( player, "There must be at least one component." );
        safe_chr ( '0', buff, bufc );
        xfree ( comp_names, "struct.comps" );
        xfree ( comp_array, "fun_structure.comp_array" );
        return;
    }

    /*
     * Make sure that we have a sane name for the components. They must
     * be smaller than half an SBUF.
     */

    for ( i = 0; i < n_comps; i++ ) {
        if ( strlen ( comp_array[i] ) > ( SBUF_SIZE / 2 ) - 9 ) {
            notify_quiet ( player, "Component name is too long." );
            safe_chr ( '0', buff, bufc );
            xfree ( comp_names, "struct.comps" );
            xfree ( comp_array, "fun_structure.comp_array" );
            return;
        }
    }

    type_names = alloc_lbuf ( "struct.types" );
    strcpy ( type_names, fargs[2] );
    n_types =
        list2arr ( &type_array, LBUF_SIZE / 2, type_names, &SPACE_DELIM );

    /*
     * Make sure all types are valid. We look only at the first char, so
     * typos will not be caught.
     */

    for ( i = 0; i < n_types; i++ ) {
        switch ( * ( type_array[i] ) ) {
        case 'a':
        case 'A':
        case 'c':
        case 'C':
        case 'd':
        case 'D':
        case 'i':
        case 'I':
        case 'f':
        case 'F':
        case 's':
        case 'S':
            /*
             * Valid types
             */
            break;

        default:
            notify_quiet ( player, "Invalid data type specified." );
            safe_chr ( '0', buff, bufc );
            xfree ( comp_names, "struct.comps" );
            xfree ( comp_array, "fun_structure.comp_array" );
            free_lbuf ( type_names );
            xfree ( type_array, "fun_structure.type_array" );
            return;
        }
    }

    if ( fargs[3] && *fargs[3] ) {
        default_vals = xstrdup ( fargs[3], "struct.defaults" );
        n_defs =
            list2arr ( &def_array, LBUF_SIZE / 2, default_vals, &isep );
    } else {
        default_vals = NULL;
        n_defs = 0;
    }

    if ( ( n_comps != n_types ) || ( n_defs && ( n_comps != n_defs ) ) ) {
        notify_quiet ( player, "List sizes must be identical." );
        safe_chr ( '0', buff, bufc );
        xfree ( comp_names, "struct.comps" );
        xfree ( comp_array, "fun_structure.comp_array" );
        free_lbuf ( type_names );
        xfree ( type_array, "fun_structure.type_array" );

        if ( default_vals ) {
            xfree ( default_vals, "struct.defaults" );
            xfree ( def_array, "fun_structure.def_array" );
        }

        return;
    }

    /*
     * Allocate the structure and stuff it in the hashtable. Note that we
     * retain one of the string arrays allocated by list2arr!
     */
    this_struct = ( STRUCTDEF * ) xmalloc ( sizeof ( STRUCTDEF ), "struct_alloc" );
    this_struct->s_name = xstrdup ( fargs[0], "struct.s_name" );
    this_struct->c_names = comp_array;
    this_struct->c_array =
        ( COMPONENT ** ) xcalloc ( n_comps, sizeof ( COMPONENT * ),
                                   "struct.n_comps" );
    this_struct->c_count = n_comps;
    this_struct->delim = osep.str[0];
    this_struct->n_instances = 0;
    this_struct->names_base = comp_names;
    this_struct->defs_base = default_vals;
    hashadd ( tbuf, ( int * ) this_struct, &mudstate.structs_htab, 0 );
    Set_Max ( mudstate.max_structs, mudstate.structs_htab.entries );
    /*
     * Now that we're done with the base name, we can stick the joining
     * period on the end.
     */
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';

    /*
     * Allocate each individual component.
     */

    for ( i = 0; i < n_comps; i++ ) {
        cp = cbuf;
        safe_sb_str ( tbuf, cbuf, &cp );

        for ( p = comp_array[i]; *p; p++ ) {
            *p = tolower ( *p );
        }

        safe_sb_str ( comp_array[i], cbuf, &cp );
        *cp = '\0';
        this_comp =
            ( COMPONENT * ) xmalloc ( sizeof ( COMPONENT ), "comp_alloc" );
        this_comp->def_val = ( default_vals ? def_array[i] : NULL );

        switch ( * ( type_array[i] ) ) {
        case 'a':
        case 'A':
            this_comp->typer_func = NULL;
            break;

        case 'c':
        case 'C':
            this_comp->typer_func = istype_char;
            check_type = 1;
            break;

        case 'd':
        case 'D':
            this_comp->typer_func = istype_dbref;
            check_type = 1;
            break;

        case 'i':
        case 'I':
            this_comp->typer_func = istype_int;
            check_type = 1;
            break;

        case 'f':
        case 'F':
            this_comp->typer_func = istype_float;
            check_type = 1;
            break;

        case 's':
        case 'S':
            this_comp->typer_func = istype_string;
            check_type = 1;
            break;

        default:
            /*
             * Should never happen
             */
            this_comp->typer_func = NULL;
        }

        this_struct->need_typecheck = check_type;
        this_struct->c_array[i] = this_comp;
        hashadd ( cbuf, ( int * ) this_comp, &mudstate.cdefs_htab, 0 );
        Set_Max ( mudstate.max_cdefs, mudstate.cdefs_htab.entries );
    }

    free_lbuf ( type_names );
    xfree ( type_array, "fun_structure.type_array" );

    if ( default_vals ) {
        xfree ( def_array, "fun_structure.def_array" );
    }

    s_StructCount ( player, StructCount ( player ) + 1 );
    safe_chr ( '1', buff, bufc );
}


void fun_construct ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep;
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    STRUCTDEF *this_struct;
    char *comp_names, *init_vals;
    char **comp_array, **vals_array;
    int n_comps, n_vals;
    int i;
    COMPONENT *c_ptr;
    INSTANCE *inst_ptr;
    STRUCTDATA *d_ptr;
    int retval;
    /*
     * This one is complicated: We need two, four, or five args.
     */
    VaChk_In ( 2, 5 );

    if ( nfargs == 3 ) {
        safe_sprintf ( buff, bufc, "#-1 FUNCTION (CONSTRUCT) EXPECTS 2 OR 4 OR 5 ARGUMENTS BUT GOT %d", nfargs );
        return;
    }

    /*
     * Enforce limits.
     */

    if ( InstanceCount ( player ) > mudconf.instance_lim ) {
        notify_quiet ( player, "Too many instances." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * If our instance name is too long, reject it.
     */

    if ( strlen ( fargs[0] ) > ( SBUF_SIZE / 2 ) - 9 ) {
        notify_quiet ( player, "Instance name is too long." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Make sure this instance doesn't exist.
     */
    ip = ibuf;
    safe_ltos ( ibuf, &ip, player );
    safe_sb_chr ( '.', ibuf, &ip );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], ibuf, &ip );
    *ip = '\0';

    if ( hashfind ( ibuf, &mudstate.instance_htab ) ) {
        notify_quiet ( player,
                       "That instance has already been defined." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Look up the structure.
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[1]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[1], tbuf, &tp );
    *tp = '\0';
    this_struct = ( STRUCTDEF * ) hashfind ( tbuf, &mudstate.structs_htab );

    if ( !this_struct ) {
        notify_quiet ( player, "No such structure." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Check to make sure that all the component names are valid, if we
     * have been given defaults. Also, make sure that the defaults are of
     * the appropriate type.
     */
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';

    if ( fargs[2] && *fargs[2] && fargs[3] && *fargs[3] ) {
        comp_names = alloc_lbuf ( "construct.comps" );
        strcpy ( comp_names, fargs[2] );
        n_comps =
            list2arr ( &comp_array, LBUF_SIZE / 2, comp_names,
                       &SPACE_DELIM );
        init_vals = alloc_lbuf ( "construct.vals" );
        strcpy ( init_vals, fargs[3] );
        n_vals =
            list2arr ( &vals_array, LBUF_SIZE / 2, init_vals, &isep );

        if ( n_comps != n_vals ) {
            notify_quiet ( player, "List sizes must be identical." );
            safe_chr ( '0', buff, bufc );
            free_lbuf ( comp_names );
            free_lbuf ( init_vals );
            xfree ( comp_array, "fun_construct.comp_array" );
            xfree ( vals_array, "fun_construct.vals_array" );
            return;
        }

        for ( i = 0; i < n_comps; i++ ) {
            cp = cbuf;
            safe_sb_str ( tbuf, cbuf, &cp );

            for ( p = comp_array[i]; *p; p++ ) {
                *p = tolower ( *p );
            }

            safe_sb_str ( comp_array[i], cbuf, &cp );
            c_ptr =
                ( COMPONENT * ) hashfind ( cbuf, &mudstate.cdefs_htab );

            if ( !c_ptr ) {
                notify_quiet ( player,
                               "Invalid component name." );
                safe_chr ( '0', buff, bufc );
                free_lbuf ( comp_names );
                free_lbuf ( init_vals );
                xfree ( comp_array, "fun_construct.comp_array" );
                xfree ( vals_array, "fun_construct.vals_array" );
                return;
            }

            if ( c_ptr->typer_func ) {
                retval =
                    ( * ( c_ptr->typer_func ) ) ( vals_array[i] );

                if ( !retval ) {
                    notify_quiet ( player,
                                   "Default value is of invalid type." );
                    safe_chr ( '0', buff, bufc );
                    free_lbuf ( comp_names );
                    free_lbuf ( init_vals );
                    xfree ( comp_array,
                            "fun_construct.comp_array" );
                    xfree ( vals_array,
                            "fun_construct.vals_array" );
                    return;
                }
            }
        }
    } else if ( ( !fargs[2] || !*fargs[2] ) && ( !fargs[3] || !*fargs[3] ) ) {
        /*
         * Blank initializers. This is just fine.
         */
        comp_names = init_vals = NULL;
        comp_array = vals_array = NULL;
        n_comps = n_vals = 0;
    } else {
        notify_quiet ( player, "List sizes must be identical." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Go go gadget constructor. Allocate the instance. We should have
     * already made sure that the instance doesn't exist.
     */
    inst_ptr = ( INSTANCE * ) xmalloc ( sizeof ( INSTANCE ), "constructor.inst" );
    inst_ptr->datatype = this_struct;
    hashadd ( ibuf, ( int * ) inst_ptr, &mudstate.instance_htab, 0 );
    Set_Max ( mudstate.max_instance, mudstate.instance_htab.entries );

    /*
     * Populate with default values.
     */

    for ( i = 0; i < this_struct->c_count; i++ ) {
        d_ptr =
            ( STRUCTDATA * ) xmalloc ( sizeof ( STRUCTDATA ),
                                       "constructor.data" );

        if ( this_struct->c_array[i]->def_val ) {
            d_ptr->text = ( char * )
                          xstrdup ( this_struct->c_array[i]->def_val,
                                    "constructor.dtext" );
        } else {
            d_ptr->text = NULL;
        }

        tp = tbuf;
        safe_sb_str ( ibuf, tbuf, &tp );
        safe_sb_chr ( '.', tbuf, &tp );
        safe_sb_str ( this_struct->c_names[i], tbuf, &tp );
        *tp = '\0';
        hashadd ( tbuf, ( int * ) d_ptr, &mudstate.instdata_htab, 0 );
        Set_Max ( mudstate.max_instdata, mudstate.instdata_htab.entries );
    }

    /*
     * Overwrite with component values.
     */

    for ( i = 0; i < n_comps; i++ ) {
        tp = tbuf;
        safe_sb_str ( ibuf, tbuf, &tp );
        safe_sb_chr ( '.', tbuf, &tp );
        safe_sb_str ( comp_array[i], tbuf, &tp );
        *tp = '\0';
        d_ptr = ( STRUCTDATA * ) hashfind ( tbuf, &mudstate.instdata_htab );

        if ( d_ptr ) {
            if ( d_ptr->text ) {
                xfree ( d_ptr->text, "constructor.dtext" );
            }

            if ( vals_array[i] && * ( vals_array[i] ) )
                d_ptr->text =
                    xstrdup ( vals_array[i],
                              "constructor.dtext" );
            else {
                d_ptr->text = NULL;
            }
        }
    }

    if ( comp_names ) {
        free_lbuf ( comp_names );
        xfree ( comp_array, "fun_construct.comp_array" );
    }

    if ( init_vals ) {
        free_lbuf ( init_vals );
        xfree ( vals_array, "fun_construct.vals_array" );
    }

    this_struct->n_instances += 1;
    s_InstanceCount ( player, InstanceCount ( player ) + 1 );
    safe_chr ( '1', buff, bufc );
}


static void load_structure ( dbref player, char *buff, char **bufc, char *inst_name, char *str_name, char *raw_text, char sep, int use_def_delim )
{
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    char *p;
    STRUCTDEF *this_struct;
    char *val_list;
    char **val_array;
    int n_vals;
    INSTANCE *inst_ptr;
    STRUCTDATA *d_ptr;
    int i;
    Delim isep;

    /*
     * Enforce limits.
     */

    if ( InstanceCount ( player ) > mudconf.instance_lim ) {
        notify_quiet ( player, "Too many instances." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * If our instance name is too long, reject it.
     */

    if ( strlen ( inst_name ) > ( SBUF_SIZE / 2 ) - 9 ) {
        notify_quiet ( player, "Instance name is too long." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Make sure this instance doesn't exist.
     */
    ip = ibuf;
    safe_ltos ( ibuf, &ip, player );
    safe_sb_chr ( '.', ibuf, &ip );

    for ( p = inst_name; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( inst_name, ibuf, &ip );
    *ip = '\0';

    if ( hashfind ( ibuf, &mudstate.instance_htab ) ) {
        notify_quiet ( player, "That instance has already been defined." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Look up the structure.
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = str_name; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( str_name, tbuf, &tp );
    *tp = '\0';
    this_struct = ( STRUCTDEF * ) hashfind ( tbuf, &mudstate.structs_htab );

    if ( !this_struct ) {
        notify_quiet ( player, "No such structure." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Chop up the raw stuff according to the delimiter.
     */
    isep.len = 1;

    if ( use_def_delim ) {
        isep.str[0] = this_struct->delim;
    } else {
        isep.str[0] = sep;
    }

    val_list = alloc_lbuf ( "load.val_list" );
    strcpy ( val_list, raw_text );
    n_vals = list2arr ( &val_array, LBUF_SIZE / 2, val_list, &isep );

    if ( n_vals != this_struct->c_count ) {
        notify_quiet ( player, "Incorrect number of components." );
        safe_chr ( '0', buff, bufc );
        free_lbuf ( val_list );
        xfree ( val_array, "load_structure.val_array" );
        return;
    }

    /*
     * Check the types of the data we've been passed.
     */

    for ( i = 0; i < n_vals; i++ ) {
        if ( this_struct->c_array[i]->typer_func &&
                ! ( ( * ( this_struct->c_array[i]->typer_func ) ) ( val_array[i] ) ) ) {
            notify_quiet ( player, "Value is of invalid type." );
            safe_chr ( '0', buff, bufc );
            free_lbuf ( val_list );
            xfree ( val_array, "load_structure.val_array" );
            return;
        }
    }

    /*
     * Allocate the instance. We should have already made sure that the
     * instance doesn't exist.
     */
    inst_ptr = ( INSTANCE * ) xmalloc ( sizeof ( INSTANCE ), "constructor.inst" );
    inst_ptr->datatype = this_struct;
    hashadd ( ibuf, ( int * ) inst_ptr, &mudstate.instance_htab, 0 );
    Set_Max ( mudstate.max_instance, mudstate.instance_htab.entries );

    /*
     * Stuff data into memory.
     */

    for ( i = 0; i < this_struct->c_count; i++ ) {
        d_ptr =
            ( STRUCTDATA * ) xmalloc ( sizeof ( STRUCTDATA ),
                                       "constructor.data" );

        if ( val_array[i] && * ( val_array[i] ) )
            d_ptr->text =
                xstrdup ( val_array[i], "constructor.dtext" );
        else {
            d_ptr->text = NULL;
        }

        tp = tbuf;
        safe_sb_str ( ibuf, tbuf, &tp );
        safe_sb_chr ( '.', tbuf, &tp );
        safe_sb_str ( this_struct->c_names[i], tbuf, &tp );
        *tp = '\0';
        hashadd ( tbuf, ( int * ) d_ptr, &mudstate.instdata_htab, 0 );
        Set_Max ( mudstate.max_instdata, mudstate.instdata_htab.entries );
    }

    free_lbuf ( val_list );
    xfree ( val_array, "load_structure.val_array" );
    this_struct->n_instances += 1;
    s_InstanceCount ( player, InstanceCount ( player ) + 1 );
    safe_chr ( '1', buff, bufc );
}

void fun_load ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep;
    VaChk_Only_InPure ( 4 );
    load_structure ( player, buff, bufc,
                     fargs[0], fargs[1], fargs[2], isep.str[0], ( nfargs != 4 ) ? 1 : 0 );
}

void fun_read ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it, aowner;
    int atr, aflags, alen;
    char *atext;

    if ( !parse_attrib ( player, fargs[0], &it, &atr, 1 ) || ( atr == NOTHING ) ) {
        safe_chr ( '0', buff, bufc );
        return;
    }

    atext = atr_pget ( it, atr, &aowner, &aflags, &alen );
    load_structure ( player, buff, bufc,
                     fargs[1], fargs[2], atext, GENERIC_STRUCT_DELIM, 0 );
    free_lbuf ( atext );
}

void fun_delimit ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it, aowner;
    int atr, aflags, alen, nitems, i, over = 0;
    char *atext, **ptrs;
    Delim isep;
    /*
     * This function is unusual in that the second argument is a
     * delimiter string of arbitrary length, rather than a character. The
     * input delimiter is the final, optional argument; if it's not
     * specified it defaults to the "null" structure delimiter. (This
     * function's primary purpose is to extract out data that's been
     * stored as a "null"-delimited structure, but it's also useful for
     * transforming any delim-separated list to a list whose elements are
     * separated by arbitrary strings.)
     */
    VaChk_Only_InPure ( 3 );

    if ( nfargs != 3 ) {
        isep.str[0] = GENERIC_STRUCT_DELIM;
    }

    if ( !parse_attrib ( player, fargs[0], &it, &atr, 1 ) || ( atr == NOTHING ) ) {
        safe_noperm ( buff, bufc );
        return;
    }

    atext = atr_pget ( it, atr, &aowner, &aflags, &alen );
    nitems = list2arr ( &ptrs, LBUF_SIZE / 2, atext, &isep );

    if ( nitems ) {
        over = safe_str ( ptrs[0], buff, bufc );
    }

    for ( i = 1; !over && ( i < nitems ); i++ ) {
        over = safe_str ( fargs[1], buff, bufc );

        if ( !over ) {
            over = safe_str ( ptrs[i], buff, bufc );
        }
    }

    free_lbuf ( atext );
    xfree ( ptrs, "fun_delimit.ptrs" );
}


void fun_z ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char tbuf[SBUF_SIZE], *tp;
    char *p;
    STRUCTDATA *s_ptr;
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], tbuf, &tp );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[1]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[1], tbuf, &tp );
    *tp = '\0';
    s_ptr = ( STRUCTDATA * ) hashfind ( tbuf, &mudstate.instdata_htab );

    if ( !s_ptr || !s_ptr->text ) {
        return;
    }

    safe_str ( s_ptr->text, buff, bufc );
}


void fun_modify ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char tbuf[SBUF_SIZE], *tp;
    char cbuf[SBUF_SIZE], *cp;
    char *endp, *p;
    INSTANCE *inst_ptr;
    COMPONENT *c_ptr;
    STRUCTDATA *s_ptr;
    char **words, **vals;
    int retval, nwords, nvals, i, n_mod;
    Delim isep;
    VaChk_Only_In ( 4 );
    /*
     * Find the instance first, since this is how we get our typechecker.
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], tbuf, &tp );
    *tp = '\0';
    endp = tp;      /* save where we are */
    inst_ptr = ( INSTANCE * ) hashfind ( tbuf, &mudstate.instance_htab );

    if ( !inst_ptr ) {
        notify_quiet ( player, "No such instance." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Process for each component in the list.
     */
    nwords = list2arr ( &words, LBUF_SIZE / 2, fargs[1], &SPACE_DELIM );
    nvals = list2arr ( &vals, LBUF_SIZE / 2, fargs[2], &isep );
    n_mod = 0;

    for ( i = 0; i < nwords; i++ ) {
        /*
         * Find the component and check the type.
         */
        if ( inst_ptr->datatype->need_typecheck ) {
            cp = cbuf;
            safe_ltos ( cbuf, &cp, player );
            safe_sb_chr ( '.', cbuf, &cp );
            safe_sb_str ( inst_ptr->datatype->s_name, cbuf, &cp );
            safe_sb_chr ( '.', cbuf, &cp );

            for ( p = words[i]; *p; p++ ) {
                *p = tolower ( *p );
            }

            safe_sb_str ( words[i], cbuf, &cp );
            *cp = '\0';
            c_ptr =
                ( COMPONENT * ) hashfind ( cbuf, &mudstate.cdefs_htab );

            if ( !c_ptr ) {
                notify_quiet ( player, "No such component." );
                continue;
            }

            if ( c_ptr->typer_func ) {
                retval = ( * ( c_ptr->typer_func ) ) ( fargs[2] );

                if ( !retval ) {
                    notify_quiet ( player,
                                   "Value is of invalid type." );
                    continue;
                }
            }
        }

        /*
         * Now go set it.
         */
        tp = endp;
        safe_sb_chr ( '.', tbuf, &tp );
        safe_sb_str ( words[i], tbuf, &tp );
        *tp = '\0';
        s_ptr = ( STRUCTDATA * ) hashfind ( tbuf, &mudstate.instdata_htab );

        if ( !s_ptr ) {
            notify_quiet ( player, "No such data." );
            continue;
        }

        if ( s_ptr->text ) {
            xfree ( s_ptr->text, "modify.dtext" );
        }

        if ( ( i < nvals ) && vals[i] && *vals[i] ) {
            s_ptr->text = xstrdup ( vals[i], "modify.dtext" );
        } else {
            s_ptr->text = NULL;
        }

        n_mod++;
    }

    xfree ( words, "fun_modify.words" );
    xfree ( vals, "fun_modify.vals" );
    safe_ltos ( buff, bufc, n_mod );
}


static void unload_structure ( dbref player, char *buff, char **bufc, char *inst_name, char sep, int use_def_delim )
{
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    INSTANCE *inst_ptr;
    char *p;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    int i;
    /*
     * Get the instance.
     */
    ip = ibuf;
    safe_ltos ( ibuf, &ip, player );
    safe_sb_chr ( '.', ibuf, &ip );

    for ( p = inst_name; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( inst_name, ibuf, &ip );
    *ip = '\0';
    inst_ptr = ( INSTANCE * ) hashfind ( ibuf, &mudstate.instance_htab );

    if ( !inst_ptr ) {
        return;
    }

    /*
     * From the instance, we can get a pointer to the structure. We then
     * have the information we need to figure out what components are
     * associated with this, and print them appropriately.
     */
    safe_sb_chr ( '.', ibuf, &ip );
    *ip = '\0';
    this_struct = inst_ptr->datatype;

    /*
     * Our delimiter is a special case.
     */
    if ( use_def_delim ) {
        sep = this_struct->delim;
    }

    for ( i = 0; i < this_struct->c_count; i++ ) {
        if ( i != 0 ) {
            safe_chr ( sep, buff, bufc );
        }

        tp = tbuf;
        safe_sb_str ( ibuf, tbuf, &tp );
        safe_sb_str ( this_struct->c_names[i], tbuf, &tp );
        *tp = '\0';
        d_ptr = ( STRUCTDATA * ) hashfind ( tbuf, &mudstate.instdata_htab );

        if ( d_ptr && d_ptr->text ) {
            safe_str ( d_ptr->text, buff, bufc );
        }
    }
}

void fun_unload ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep;
    VaChk_Only_InPure ( 2 );
    unload_structure ( player, buff, bufc, fargs[0], isep.str[0],
                       ( nfargs != 2 ) ? 1 : 0 );
}

void fun_write ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it, aowner;
    int atrnum, aflags;
    char tbuf[LBUF_SIZE], *tp, *str;
    ATTR *attr;

    if ( !parse_thing_slash ( player, fargs[0], &str, &it ) ) {
        safe_nomatch ( buff, bufc );
        return;
    }

    tp = tbuf;
    *tp = '\0';
    unload_structure ( player, tbuf, &tp, fargs[1], GENERIC_STRUCT_DELIM, 0 );

    if ( *tbuf ) {
        atrnum = mkattr ( str );

        if ( atrnum <= 0 ) {
            safe_str ( "#-1 UNABLE TO CREATE ATTRIBUTE", buff, bufc );
            return;
        }

        attr = atr_num ( atrnum );
        atr_pget_info ( it, atrnum, &aowner, &aflags );

        if ( !attr || !Set_attr ( player, it, attr, aflags ) ||
                ( attr->check != NULL ) ) {
            safe_noperm ( buff, bufc );
        } else {
            atr_add ( it, atrnum, tbuf, Owner ( player ),
                      aflags | AF_STRUCTURE );
        }
    }
}


void fun_destruct ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char tbuf[SBUF_SIZE], *tp;
    char ibuf[SBUF_SIZE], *ip;
    INSTANCE *inst_ptr;
    char *p;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    int i;
    /*
     * Get the instance.
     */
    ip = ibuf;
    safe_ltos ( ibuf, &ip, player );
    safe_sb_chr ( '.', ibuf, &ip );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], ibuf, &ip );
    *ip = '\0';
    inst_ptr = ( INSTANCE * ) hashfind ( ibuf, &mudstate.instance_htab );

    if ( !inst_ptr ) {
        notify_quiet ( player, "No such instance." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Now we can get a pointer to the structure and find the rest of the
     * components.
     */
    this_struct = inst_ptr->datatype;
    xfree ( inst_ptr, "constructor.inst" );
    hashdelete ( ibuf, &mudstate.instance_htab );
    safe_sb_chr ( '.', ibuf, &ip );
    *ip = '\0';

    for ( i = 0; i < this_struct->c_count; i++ ) {
        tp = tbuf;
        safe_sb_str ( ibuf, tbuf, &tp );
        safe_sb_str ( this_struct->c_names[i], tbuf, &tp );
        *tp = '\0';
        d_ptr = ( STRUCTDATA * ) hashfind ( tbuf, &mudstate.instdata_htab );

        if ( d_ptr ) {
            if ( d_ptr->text ) {
                xfree ( d_ptr->text, "constructor.data" );
            }

            xfree ( d_ptr, "constructor.data" );
            hashdelete ( tbuf, &mudstate.instdata_htab );
        }
    }

    this_struct->n_instances -= 1;
    s_InstanceCount ( player, InstanceCount ( player ) - 1 );
    safe_chr ( '1', buff, bufc );
}


void fun_unstructure ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char tbuf[SBUF_SIZE], *tp;
    char cbuf[SBUF_SIZE], *cp;
    char *p;
    STRUCTDEF *this_struct;
    int i;
    /*
     * Find the structure
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, player );
    safe_sb_chr ( '.', tbuf, &tp );

    for ( p = fargs[0]; *p; p++ ) {
        *p = tolower ( *p );
    }

    safe_sb_str ( fargs[0], tbuf, &tp );
    *tp = '\0';
    this_struct = ( STRUCTDEF * ) hashfind ( tbuf, &mudstate.structs_htab );

    if ( !this_struct ) {
        notify_quiet ( player, "No such structure." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Can't delete what's in use.
     */

    if ( this_struct->n_instances > 0 ) {
        notify_quiet ( player, "This structure is in use." );
        safe_chr ( '0', buff, bufc );
        return;
    }

    /*
     * Wipe the structure from the hashtable.
     */
    hashdelete ( tbuf, &mudstate.structs_htab );
    /*
     * Wipe out every component definition.
     */
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';

    for ( i = 0; i < this_struct->c_count; i++ ) {
        cp = cbuf;
        safe_sb_str ( tbuf, cbuf, &cp );
        safe_sb_str ( this_struct->c_names[i], cbuf, &cp );
        *cp = '\0';

        if ( this_struct->c_array[i] ) {
            xfree ( this_struct->c_array[i], "comp_alloc" );
        }

        hashdelete ( cbuf, &mudstate.cdefs_htab );
    }

    /*
     * Free up our bit of memory.
     */
    xfree ( this_struct->s_name, "struct.s_name" );

    if ( this_struct->names_base ) {
        xfree ( this_struct->names_base, "struct.names_base" );
    }

    if ( this_struct->defs_base ) {
        xfree ( this_struct->defs_base, "struct.defs_base" );
    }

    xfree ( this_struct->c_names, "struct.c_names" );
    xfree ( this_struct, "struct_alloc" );
    s_StructCount ( player, StructCount ( player ) - 1 );
    safe_chr ( '1', buff, bufc );
}

void fun_lstructures ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    print_htab_matches ( player, &mudstate.structs_htab, buff, bufc );
}

void fun_linstances ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    print_htab_matches ( player, &mudstate.instance_htab, buff, bufc );
}

void structure_clr ( dbref thing )
{
    /*
     * Wipe out all structure information associated with an object. Find
     * all the object's instances. Destroy them. Then, find all the
     * object's defined structures, and destroy those.
     */
    HASHTAB *htab;
    HASHENT *hptr;
    char tbuf[SBUF_SIZE], ibuf[SBUF_SIZE], cbuf[SBUF_SIZE], *tp, *ip, *cp, *tname;
    int i, j, len, count;
    INSTANCE **inst_array;
    char **name_array;
    STRUCTDEF *this_struct;
    STRUCTDATA *d_ptr;
    STRUCTDEF **struct_array;
    /*
     * The instance table is indexed as <dbref number>.<instance name>
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, thing );
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';
    len = strlen ( tbuf );
    /*
     * Because of the hashtable rechaining that's done, we cannot simply
     * walk the hashtable and delete entries as we go. Instead, we've got
     * to keep track of all of our pointers, and go back and do them one
     * by one.
     */
    inst_array = ( INSTANCE ** ) xcalloc ( mudconf.instance_lim + 1,
                                           sizeof ( INSTANCE * ), "structure_clr.inst_array" );
    name_array = ( char ** ) xcalloc ( mudconf.instance_lim + 1, sizeof ( char * ),
                                       "structure_clr.name_array" );
    htab = &mudstate.instance_htab;
    count = 0;

    for ( i = 0; i < htab->hashsize; i++ ) {
        for ( hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next ) {
            if ( !strncmp ( tbuf, hptr->target.s, len ) ) {
                name_array[count] = ( char * ) hptr->target.s;
                inst_array[count] = ( INSTANCE * ) hptr->data;
                count++;
            }
        }
    }

    /*
     * Now that we have the pointers to the instances, we can get the
     * structure definitions, and use that to hunt down and wipe the
     * components.
     */

    if ( count > 0 ) {
        for ( i = 0; i < count; i++ ) {
            this_struct = inst_array[i]->datatype;
            xfree ( inst_array[i], "constructor.inst" );
            hashdelete ( name_array[i], &mudstate.instance_htab );
            ip = ibuf;
            safe_sb_str ( name_array[i], ibuf, &ip );
            safe_sb_chr ( '.', ibuf, &ip );
            *ip = '\0';

            for ( j = 0; j < this_struct->c_count; j++ ) {
                cp = cbuf;
                safe_sb_str ( ibuf, cbuf, &cp );
                safe_sb_str ( this_struct->c_names[j], cbuf,
                              &cp );
                *cp = '\0';
                d_ptr =
                    ( STRUCTDATA * ) hashfind ( cbuf,
                                                &mudstate.instdata_htab );

                if ( d_ptr ) {
                    if ( d_ptr->text )
                        xfree ( d_ptr->text,
                                "constructor.data" );

                    xfree ( d_ptr, "constructor.data" );
                    hashdelete ( cbuf,
                                 &mudstate.instdata_htab );
                }
            }

            this_struct->n_instances -= 1;
        }
    }

    xfree ( inst_array, "structure_clr.inst_array" );
    xfree ( name_array, "structure_clr.name_array" );
    /*
     * The structure table is indexed as <dbref number>.<struct name>
     */
    tp = tbuf;
    safe_ltos ( tbuf, &tp, thing );
    safe_sb_chr ( '.', tbuf, &tp );
    *tp = '\0';
    len = strlen ( tbuf );
    /*
     * Again, we have the hashtable rechaining problem.
     */
    struct_array = ( STRUCTDEF ** ) xcalloc ( mudconf.struct_lim + 1,
                   sizeof ( STRUCTDEF * ), "structure_clr.struct_array" );
    name_array = ( char ** ) xcalloc ( mudconf.struct_lim + 1, sizeof ( char * ),
                                       "structure_clr.name_array2" );
    htab = &mudstate.structs_htab;
    count = 0;

    for ( i = 0; i < htab->hashsize; i++ ) {
        for ( hptr = htab->entry[i]; hptr != NULL; hptr = hptr->next ) {
            if ( !strncmp ( tbuf, hptr->target.s, len ) ) {
                name_array[count] = ( char * ) hptr->target.s;
                struct_array[count] = ( STRUCTDEF * ) hptr->data;
                count++;
            }
        }
    }

    /*
     * We have the pointers to the structures. Flag a big error if
     * they're still in use, wipe them from the hashtable, then wipe out
     * every component definition. Free up the memory.
     */

    if ( count > 0 ) {
        for ( i = 0; i < count; i++ ) {
            if ( struct_array[i]->n_instances > 0 ) {
                tname = log_getname ( thing, "structure_clr" );
                log_write ( LOG_ALWAYS, "BUG", "STRUCT", "%s's structure %s has %d allocated instances uncleared.", tname, name_array[i], struct_array[i]->n_instances );
                xfree ( tname, "structure_clr" );
            }

            hashdelete ( name_array[i], &mudstate.structs_htab );
            ip = ibuf;
            safe_sb_str ( name_array[i], ibuf, &ip );
            safe_sb_chr ( '.', ibuf, &ip );
            *ip = '\0';

            for ( j = 0; j < struct_array[i]->c_count; j++ ) {
                cp = cbuf;
                safe_sb_str ( ibuf, cbuf, &cp );
                safe_sb_str ( struct_array[i]->c_names[j], cbuf,
                              &cp );
                *cp = '\0';

                if ( struct_array[i]->c_array[j] ) {
                    xfree ( struct_array[i]->c_array[j],
                            "comp_alloc" );
                }

                hashdelete ( cbuf, &mudstate.cdefs_htab );
            }

            xfree ( struct_array[i]->s_name, "struct.s_name" );

            if ( struct_array[i]->names_base )
                xfree ( struct_array[i]->names_base,
                        "struct.names_base" );

            if ( struct_array[i]->defs_base )
                xfree ( struct_array[i]->defs_base,
                        "struct.defs_base" );

            xfree ( struct_array[i]->c_names, "struct.c_names" );
            xfree ( struct_array[i], "struct_alloc" );
        }
    }

    xfree ( struct_array, "structure_clr.struct_array" );
    xfree ( name_array, "structure_clr.name_array2" );
}

/*
 * --------------------------------------------------------------------------
 * Auxiliary functions for stacks.
 */

#define stack_get(x)   ((OBJSTACK *) nhashfind(x, &mudstate.objstack_htab))

#define stack_object(p,x)               \
        x = match_thing(p, fargs[0]);           \
    if (!Good_obj(x)) {             \
        return;                 \
    }                       \
    if (!Controls(p, x)) {              \
            notify_quiet(p, NOPERM_MESSAGE);        \
        return;                 \
    }

/*
 * ---------------------------------------------------------------------------
 * Object stack functions.
 */

void stack_clr ( dbref thing )
{
    OBJSTACK *sp, *tp, *xp;
    sp = stack_get ( thing );

    if ( sp ) {
        for ( tp = sp; tp != NULL; ) {
            xfree ( tp->data, "stack_clr_data" );
            xp = tp;
            tp = tp->next;
            xfree ( xp, "stack_clr" );
        }

        nhashdelete ( thing, &mudstate.objstack_htab );
        s_StackCount ( thing, 0 );
    }
}

static int stack_set ( dbref thing, OBJSTACK *sp )
{
    OBJSTACK *xsp;
    char *tname;
    int stat;

    if ( !sp ) {
        nhashdelete ( thing, &mudstate.objstack_htab );
        return 1;
    }

    xsp = stack_get ( thing );

    if ( xsp ) {
        stat = nhashrepl ( thing, ( int * ) sp, &mudstate.objstack_htab );
    } else {
        stat = nhashadd ( thing, ( int * ) sp, &mudstate.objstack_htab );
        Set_Max ( mudstate.max_stacks, mudstate.objstack_htab.entries );
    }

    if ( stat < 0 ) {       /* failure for some reason */
        tname = log_getname ( thing, "stack_set" );
        log_write ( LOG_BUGS, "STK", "SET", "%s, Failure", tname );
        xfree ( tname, "stack_set" );
        stack_clr ( thing );
        return 0;
    }

    return 1;
}

void fun_empty ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;
    VaChk_Range ( 0, 1 );

    if ( !fargs[0] ) {
        it = player;
    } else {
        stack_object ( player, it );
    }

    stack_clr ( it );
}

void fun_items ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;

    if ( !fargs[0] ) {
        it = player;
    } else {
        stack_object ( player, it );
    }

    safe_ltos ( buff, bufc, StackCount ( it ) );
}


void fun_push ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;
    char *data;
    OBJSTACK *sp;
    VaChk_Range ( 0, 2 );

    if ( !fargs[1] ) {
        it = player;

        if ( !fargs[0] || !*fargs[0] ) {
            data = ( char * ) "";
        } else {
            data = fargs[0];
        }
    } else {
        stack_object ( player, it );
        data = fargs[1];
    }

    if ( StackCount ( it ) + 1 > mudconf.stack_lim ) {
        return;
    }

    sp = ( OBJSTACK * ) xmalloc ( sizeof ( OBJSTACK ), "stack_push" );

    if ( !sp ) { /* out of memory, ouch */
        return;
    }

    sp->next = stack_get ( it );
    sp->data = ( char * ) xmalloc ( sizeof ( char ) * ( strlen ( data ) + 1 ),
                                    "stack_push_data" );

    if ( !sp->data ) {
        return;
    }

    strcpy ( sp->data, data );

    if ( stack_set ( it, sp ) ) {
        s_StackCount ( it, StackCount ( it ) + 1 );
    }
}

void fun_dup ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;
    OBJSTACK *hp;       /* head of stack */
    OBJSTACK *tp;       /* temporary stack pointer */
    OBJSTACK *sp;       /* new stack element */
    int pos, count = 0;
    VaChk_Range ( 0, 2 );

    if ( !fargs[0] ) {
        it = player;
    } else {
        stack_object ( player, it );
    }

    if ( StackCount ( it ) + 1 > mudconf.stack_lim ) {
        return;
    }

    if ( !fargs[1] || !*fargs[1] ) {
        pos = 0;
    } else {
        pos = ( int ) strtol ( fargs[1], ( char ** ) NULL, 10 );
    }

    hp = stack_get ( it );

    for ( tp = hp; ( count != pos ) && ( tp != NULL ); count++, tp = tp->next );

    if ( !tp ) {
        notify_quiet ( player, "No such item on stack." );
        return;
    }

    sp = ( OBJSTACK * ) xmalloc ( sizeof ( OBJSTACK ), "stack_dup" );

    if ( !sp ) {
        return;
    }

    sp->next = hp;
    sp->data = ( char * ) xmalloc ( sizeof ( char ) * ( strlen ( tp->data ) + 1 ),
                                    "stack_dup_data" );

    if ( !sp->data ) {
        return;
    }

    strcpy ( sp->data, tp->data );

    if ( stack_set ( it, sp ) ) {
        s_StackCount ( it, StackCount ( it ) + 1 );
    }
}

void fun_swap ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;
    OBJSTACK *sp, *tp;
    VaChk_Range ( 0, 1 );

    if ( !fargs[0] ) {
        it = player;
    } else {
        stack_object ( player, it );
    }

    sp = stack_get ( it );

    if ( !sp || ( sp->next == NULL ) ) {
        notify_quiet ( player, "Not enough items on stack." );
        return;
    }

    tp = sp->next;
    sp->next = tp->next;
    tp->next = sp;
    stack_set ( it, tp );
}

void handle_pop ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;
    int pos, count = 0, peek_flag, toss_flag;
    OBJSTACK *sp;
    OBJSTACK *prev = NULL;
    peek_flag = Is_Func ( POP_PEEK );
    toss_flag = Is_Func ( POP_TOSS );
    VaChk_Range ( 0, 2 );

    if ( !fargs[0] ) {
        it = player;
    } else {
        stack_object ( player, it );
    }

    if ( !fargs[1] || !*fargs[1] ) {
        pos = 0;
    } else {
        pos = ( int ) strtol ( fargs[1], ( char ** ) NULL, 10 );
    }

    sp = stack_get ( it );

    if ( !sp ) {
        return;
    }

    while ( count != pos ) {
        if ( !sp ) {
            return;
        }

        prev = sp;
        sp = sp->next;
        count++;
    }

    if ( !sp ) {
        return;
    }

    if ( !toss_flag ) {
        safe_str ( sp->data, buff, bufc );
    }

    if ( !peek_flag ) {
        if ( count == 0 ) {
            stack_set ( it, sp->next );
        } else {
            prev->next = sp->next;
        }

        xfree ( sp->data, "stack_pop_data" );
        xfree ( sp, "stack_pop" );
        s_StackCount ( it, StackCount ( it ) - 1 );
    }
}

void fun_popn ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref it;
    int pos, nitems, i, count = 0, over = 0;
    OBJSTACK *sp, *tp, *xp;
    OBJSTACK *prev = NULL;
    Delim osep;
    char *bb_p;
    VaChk_Only_Out ( 4 );
    stack_object ( player, it );
    pos = ( int ) strtol ( fargs[1], ( char ** ) NULL, 10 );
    nitems = ( int ) strtol ( fargs[2], ( char ** ) NULL, 10 );
    sp = stack_get ( it );

    if ( !sp ) {
        return;
    }

    while ( count != pos ) {
        if ( !sp ) {
            return;
        }

        prev = sp;
        sp = sp->next;
        count++;
    }

    if ( !sp ) {
        return;
    }

    /*
     * We've now hit the start item, the first item. Copy 'em off.
     */

    for ( i = 0, tp = sp, bb_p = *bufc; ( i < nitems ) && ( tp != NULL ); i++ ) {
        if ( !over ) {
            /*
             * We have to pop off the items regardless of whether
             * or not there's an overflow, but we can save
             * ourselves some copying if so.
             */
            if ( *bufc != bb_p ) {
                print_sep ( &osep, buff, bufc );
            }

            over = safe_str ( tp->data, buff, bufc );
        }

        xp = tp;
        tp = tp->next;
        xfree ( xp->data, "stack_popn_data" );
        xfree ( xp, "stack_popn" );
        s_StackCount ( it, StackCount ( it ) - 1 );
    }

    /*
     * Relink the chain.
     */

    if ( count == 0 ) {
        stack_set ( it, tp );
    } else {
        prev->next = tp;
    }
}

void fun_lstack ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim osep;
    dbref it;
    OBJSTACK *sp;
    char *bp, *bb_p;
    int over = 0;
    VaChk_Out ( 0, 2 );

    if ( !fargs[0] ) {
        it = player;
    } else {
        stack_object ( player, it );
    }

    bp = buff;
    bb_p = *bufc;

    for ( sp = stack_get ( it ); ( sp != NULL ) && !over; sp = sp->next ) {
        if ( *bufc != bb_p ) {
            print_sep ( &osep, buff, bufc );
        }

        over = safe_str ( sp->data, buff, bufc );
    }
}

/*
 * --------------------------------------------------------------------------
 * regedit: Edit a string for sed/perl-like s//
 * regedit(<string>,<regexp>,<replacement>) Derived from the PennMUSH code.
 */

void perform_regedit ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    pcre *re;
    pcre_extra *study = NULL;
    const char *errptr;
    int case_option, all_option;
    int erroffset, subpatterns, len;
    int offsets[PCRE_MAX_OFFSETS];
    char *r, *start;
    char tbuf[LBUF_SIZE];
    char tmp;
    int match_offset = 0;
    case_option = Func_Mask ( REG_CASELESS );
    all_option = Func_Mask ( REG_MATCH_ALL );

    if ( ( re = pcre_compile ( fargs[1], case_option,
                               &errptr, &erroffset, mudstate.retabs ) ) == NULL ) {
        /*
         * Matching error. Note that this returns a null string
         * rather than '#-1 REGEXP ERROR: <error>', as PennMUSH does,
         * in order to remain consistent with our other regexp
         * functions.
         */
        notify_quiet ( player, errptr );
        return;
    }

    /*
     * Study the pattern for optimization, if we're going to try multiple
     * matches.
     */
    if ( all_option ) {
        study = pcre_study ( re, 0, &errptr );

        if ( errptr != NULL ) {
            xfree ( re, "perform_regedit.re" );
            notify_quiet ( player, errptr );
            return;
        }
    }

    len = strlen ( fargs[0] );
    start = fargs[0];
    subpatterns = pcre_exec ( re, study, fargs[0], len, 0, 0, offsets,
                              PCRE_MAX_OFFSETS );

    /*
     * If there's no match, just return the original.
     */

    if ( subpatterns < 0 ) {
        xfree ( re, "perform_regedit.re" );

        if ( study ) {
            xfree ( study, "perform_regedit.study" );
        }

        safe_str ( fargs[0], buff, bufc );
        return;
    }

    do {
        /*
         * If we had too many subpatterns for the offsets vector, set
         * the number to 1/3rd of the size of the offsets vector.
         */
        if ( subpatterns == 0 ) {
            subpatterns = PCRE_MAX_OFFSETS / 3;
        }

        /*
         * Copy up to the start of the matched area.
         */
        tmp = fargs[0][offsets[0]];
        fargs[0][offsets[0]] = '\0';
        safe_str ( start, buff, bufc );
        fargs[0][offsets[0]] = tmp;

        /*
         * Copy in the replacement, putting in captured
         * sub-expressions.
         */

        for ( r = fargs[2]; *r; r++ ) {
            int offset, have_brace = 0;
            char *endsub;

            if ( *r != '$' ) {
                safe_chr ( *r, buff, bufc );
                continue;
            }

            r++;

            if ( *r == '{' ) {
                have_brace = 1;
                r++;
            }

            offset = strtoul ( r, &endsub, 10 );

            if ( r == endsub || ( have_brace && *endsub != '}' ) ) {
                /*
                 * Not a valid number.
                 */
                safe_chr ( '$', buff, bufc );

                if ( have_brace ) {
                    safe_chr ( '{', buff, bufc );
                }

                r--;
                continue;
            }

            r = endsub - 1;

            if ( have_brace ) {
                r++;
            }

            if ( pcre_copy_substring ( fargs[0], offsets, subpatterns,
                                       offset, tbuf, LBUF_SIZE ) >= 0 ) {
                safe_str ( tbuf, buff, bufc );
            }
        }

        start = fargs[0] + offsets[1];
        match_offset = offsets[1];
    } while ( all_option && ( ( ( offsets[0] == offsets[1] ) &&
                                /*
                                 * PCRE docs note: Perl special-cases the empty-string match in split
                                 * and /g. To emulate, first try the match again at the same position
                                 * with PCRE_NOTEMPTY, then advance the starting offset if that
                                 * fails.
                                 */
                                ( ( ( subpatterns = pcre_exec ( re, study, fargs[0], len,
                                        match_offset, PCRE_NOTEMPTY, offsets,
                                        PCRE_MAX_OFFSETS ) ) >= 0 ) ||
                                  ( ( match_offset++ < len ) &&
                                    ( subpatterns = pcre_exec ( re, study, fargs[0], len,
                                            match_offset, 0, offsets,
                                            PCRE_MAX_OFFSETS ) ) >= 0 ) ) ) ||
                              ( ( match_offset <= len ) &&
                                ( subpatterns = pcre_exec ( re, study, fargs[0], len,
                                        match_offset, 0, offsets,
                                        PCRE_MAX_OFFSETS ) ) >= 0 ) ) );

    /*
     * Copy everything after the matched bit.
     */
    safe_str ( start, buff, bufc );
    xfree ( re, "perform_regedit.re" );

    if ( study ) {
        xfree ( study, "perform_regedit.study" );
    }
}

/*
 * --------------------------------------------------------------------------
 * wildparse: Set the results of a wildcard match into named variables.
 * wildparse(<string>,<pattern>,<list of variable names>)
 */

void fun_wildparse ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i, nqregs;
    char *t_args[NUM_ENV_VARS], **qregs;

    if ( !wild ( fargs[1], fargs[0], t_args, NUM_ENV_VARS ) ) {
        return;
    }

    nqregs = list2arr ( &qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM );

    for ( i = 0; i < nqregs; i++ ) {
        if ( qregs[i] && *qregs[i] ) {
            set_xvar ( player, qregs[i], t_args[i] );
        }
    }

    /*
     * Need to free up allocated memory from the match.
     */

    for ( i = 0; i < NUM_ENV_VARS; i++ ) {
        if ( t_args[i] ) {
            free_lbuf ( t_args[i] );
        }
    }

    xfree ( qregs, "wildparse.qregs" );
}

/*
 * ---------------------------------------------------------------------------
 * perform_regparse: Slurp a string into up to ten named variables ($0 - $9).
 * REGPARSE, REGPARSEI. Unlike regmatch(), this returns no value.
 * regparse(string, pattern, named vars)
 */

void perform_regparse ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i, nqregs;
    int case_option;
    char **qregs;
    char matchbuf[LBUF_SIZE];
    pcre *re;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    int subpatterns;
    case_option = Func_Mask ( REG_CASELESS );

    if ( ( re = pcre_compile ( fargs[1], case_option,
                               &errptr, &erroffset, mudstate.retabs ) ) == NULL ) {
        /*
         * Matching error.
         */
        notify_quiet ( player, errptr );
        return;
    }

    subpatterns = pcre_exec ( re, NULL, fargs[0], strlen ( fargs[0] ),
                              0, 0, offsets, PCRE_MAX_OFFSETS );

    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if ( subpatterns == 0 ) {
        subpatterns = PCRE_MAX_OFFSETS / 3;
    }

    nqregs = list2arr ( &qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM );

    for ( i = 0; i < nqregs; i++ ) {
        if ( qregs[i] && *qregs[i] ) {
            if ( pcre_copy_substring ( fargs[0], offsets, subpatterns,
                                       i, matchbuf, LBUF_SIZE ) < 0 ) {
                set_xvar ( player, qregs[i], NULL );
            } else {
                set_xvar ( player, qregs[i], matchbuf );
            }
        }
    }

    xfree ( re, "perform_regparse.re" );
    xfree ( qregs, "perform_regparse.qregs" );
}

/*
 * ---------------------------------------------------------------------------
 * perform_regrab: Like grab() and graball(), but with a regexp pattern.
 * REGRAB, REGRABI. Derived from PennMUSH.
 */

void perform_regrab ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;
    int case_option, all_option;
    char *r, *s, *bb_p;
    pcre *re;
    pcre_extra *study;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    case_option = Func_Mask ( REG_CASELESS );
    all_option = Func_Mask ( REG_MATCH_ALL );

    if ( all_option ) {
        VaChk_Only_In_Out ( 4 );
    } else {
        VaChk_Only_In ( 3 );
    }

    s = trim_space_sep ( fargs[0], &isep );
    bb_p = *bufc;

    if ( ( re = pcre_compile ( fargs[1], case_option, &errptr, &erroffset,
                               mudstate.retabs ) ) == NULL ) {
        /*
         * Matching error. Note difference from PennMUSH behavior:
         * Regular expression errors return 0, not #-1 with an error
         * message.
         */
        notify_quiet ( player, errptr );
        return;
    }

    study = pcre_study ( re, 0, &errptr );

    if ( errptr != NULL ) {
        notify_quiet ( player, errptr );
        xfree ( re, "perform_regrab.re" );
        return;
    }

    do {
        r = split_token ( &s, &isep );

        if ( pcre_exec ( re, study, r, strlen ( r ), 0, 0,
                         offsets, PCRE_MAX_OFFSETS ) >= 0 ) {
            if ( *bufc != bb_p ) {
                /*
                 * if true, all_option also
                 * * true
                 */
                print_sep ( &osep, buff, bufc );
            }

            safe_str ( r, buff, bufc );

            if ( !all_option ) {
                break;
            }
        }
    } while ( s );

    xfree ( re, "perform_regrab.re" );

    if ( study ) {
        xfree ( study, "perform_regrab.study" );
    }
}

/*
 * ---------------------------------------------------------------------------
 * perform_regmatch: Return 0 or 1 depending on whether or not a regular
 * expression matches a string. If a third argument is specified, dump the
 * results of a regexp pattern match into a set of arbitrary r()-registers.
 * REGMATCH, REGMATCHI
 *
 * regmatch(string, pattern, list of registers) If the number of matches exceeds
 * the registers, those bits are tossed out. If -1 is specified as a register
 * number, the matching bit is tossed. Therefore, if the list is "-1 0 3 5",
 * the regexp $0 is tossed, and the regexp $1, $2, and $3 become r(0), r(3),
 * and r(5), respectively.
 *
 * PCRE modifications adapted from PennMUSH.
 *
 */

void perform_regmatch ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int case_option;
    int i, nqregs;
    char **qregs;
    pcre *re;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    int subpatterns;
    char tbuf[LBUF_SIZE], *p;
    case_option = Func_Mask ( REG_CASELESS );
    VaChk_Range ( 2, 3 );

    if ( ( re = pcre_compile ( fargs[1], case_option,
                               &errptr, &erroffset, mudstate.retabs ) ) == NULL ) {
        /*
         * Matching error. Note difference from PennMUSH behavior:
         * Regular expression errors return 0, not #-1 with an error
         * message.
         */
        notify_quiet ( player, errptr );
        safe_chr ( '0', buff, bufc );
        return;
    }

    subpatterns = pcre_exec ( re, NULL, fargs[0], strlen ( fargs[0] ),
                              0, 0, offsets, PCRE_MAX_OFFSETS );
    safe_bool ( buff, bufc, ( subpatterns >= 0 ) );

    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if ( subpatterns == 0 ) {
        subpatterns = PCRE_MAX_OFFSETS / 3;
    }

    /*
     * If we don't have a third argument, we're done.
     */
    if ( nfargs != 3 ) {
        xfree ( re, "perform_regmatch.re" );
        return;
    }

    /*
     * We need to parse the list of registers. Anything that we don't get
     * is assumed to be -1. If we didn't match, or the match went wonky,
     * then set the register to empty. Otherwise, fill the register with
     * the subexpression.
     */
    nqregs = list2arr ( &qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM );

    for ( i = 0; i < nqregs; i++ ) {
        if ( pcre_copy_substring ( fargs[0], offsets, subpatterns, i,
                                   tbuf, LBUF_SIZE ) < 0 ) {
            set_register ( "perform_regmatch", qregs[i], NULL );
        } else {
            set_register ( "perform_regmatch", qregs[i], tbuf );
        }
    }

    xfree ( re, "perform_regmatch.re" );
    xfree ( qregs, "perform_regmatch.qregs" );
}

/*
 * ---------------------------------------------------------------------------
 * fun_until: Much like while(), but operates on multiple lists ala mix().
 * until(eval_fn,cond_fn,list1,list2,compare_str,delim,output delim) The
 * delimiter terminators are MANDATORY. The termination condition is a REGEXP
 * match (thus allowing this to be also used as 'eval until a termination
 * condition is NOT met').
 */

void fun_until ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;
    dbref aowner1, thing1, aowner2, thing2;
    int aflags1, aflags2, anum1, anum2, alen1, alen2;
    ATTR *ap, *ap2;
    char *atext1, *atext2, *atextbuf, *condbuf;
    char *cp[NUM_ENV_VARS], *os[NUM_ENV_VARS];
    int count[LBUF_SIZE / 2];
    int i, is_exact_same, is_same, nwords, lastn, wc;
    char *str, *dp, *savep, *bb_p;
    char tmpbuf[2];
    pcre *re;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    int subpatterns;
    /*
     * We need at least 6 arguments. The last 2 args must be delimiters.
     */
    VaChk_Range ( 6, 12 );
    VaChk_InSep ( nfargs - 1, 0 );
    VaChk_OutSep ( nfargs, 0 );
    lastn = nfargs - 4;

    /*
     * Make sure we have a valid regular expression.
     */

    if ( ( re = pcre_compile ( fargs[lastn + 1], 0,
                               &errptr, &erroffset, mudstate.retabs ) ) == NULL ) {
        /*
         * Return nothing on a bad match.
         */
        notify_quiet ( player, errptr );
        return;
    }

    /*
     * Our first and second args can be <obj>/<attr> or just <attr>. Use
     * them if we can access them, otherwise return an empty string.
     *
     * Note that for user-defined attributes, atr_str() returns a pointer to
     * a static, and that therefore we have to be careful about what
     * we're doing.
     */
    Parse_Uattr ( player, fargs[0], thing1, anum1, ap );
    Get_Uattr ( player, thing1, ap, atext1, aowner1, aflags1, alen1 );
    Parse_Uattr ( player, fargs[1], thing2, anum2, ap2 );

    if ( !ap2 ) {
        free_lbuf ( atext1 );   /* we allocated this, remember? */
        return;
    }

    /*
     * If our evaluation and condition are the same, we can save
     * ourselves some time later. There are two possibilities: we have
     * the exact same obj/attr pair, or the attributes contain identical
     * text.
     */

    if ( ( thing1 == thing2 ) && ( ap->number == ap2->number ) ) {
        is_same = 1;
        is_exact_same = 1;
    } else {
        is_exact_same = 0;
        atext2 =
            atr_pget ( thing2, ap2->number, &aowner2, &aflags2, &alen2 );

        if ( !*atext2
                || !See_attr ( player, thing2, ap2, aowner2, aflags2 ) ) {
            free_lbuf ( atext1 );
            free_lbuf ( atext2 );
            return;
        }

        if ( !strcmp ( atext1, atext2 ) ) {
            is_same = 1;
        } else {
            is_same = 0;
        }
    }

    atextbuf = alloc_lbuf ( "fun_while.eval" );

    if ( !is_same ) {
        condbuf = alloc_lbuf ( "fun_while.cond" );
    }

    bb_p = *bufc;

    /*
     * Process the list one element at a time. We need to find out what
     * the longest list is; assume null-padding for shorter lists.
     */

    for ( i = 0; i < NUM_ENV_VARS; i++ ) {
        cp[i] = NULL;
    }

    cp[2] = trim_space_sep ( fargs[2], &isep );
    nwords = count[2] = countwords ( cp[2], &isep );

    for ( i = 3; i <= lastn; i++ ) {
        cp[i] = trim_space_sep ( fargs[i], &isep );
        count[i] = countwords ( cp[i], &isep );

        if ( count[i] > nwords ) {
            nwords = count[i];
        }
    }

    for ( wc = 0;
            ( wc < nwords ) && ( mudstate.func_invk_ctr < mudconf.func_invk_lim )
            && !Too_Much_CPU(); wc++ ) {
        for ( i = 2; i <= lastn; i++ ) {
            if ( count[i] ) {
                os[i - 2] = split_token ( &cp[i], &isep );
            } else {
                tmpbuf[0] = '\0';
                os[i - 2] = tmpbuf;
            }
        }

        if ( *bufc != bb_p ) {
            print_sep ( &osep, buff, bufc );
        }

        StrCopyKnown ( atextbuf, atext1, alen1 );
        str = atextbuf;
        savep = *bufc;
        exec ( buff, bufc, player, caller, cause,
               EV_STRIP | EV_FCHECK | EV_EVAL, &str, & ( os[0] ), lastn - 1 );

        if ( !is_same ) {
            StrCopyKnown ( atextbuf, atext2, alen2 );
            dp = savep = condbuf;
            str = atextbuf;
            exec ( condbuf, &dp, player, caller, cause,
                   EV_STRIP | EV_FCHECK | EV_EVAL, &str, & ( os[0] ),
                   lastn - 1 );
        }

        subpatterns = pcre_exec ( re, NULL, savep, strlen ( savep ),
                                  0, 0, offsets, PCRE_MAX_OFFSETS );

        if ( subpatterns >= 0 ) {
            break;
        }
    }

    xfree ( re, "until.re" );
    free_lbuf ( atext1 );

    if ( !is_exact_same ) {
        free_lbuf ( atext2 );
    }

    free_lbuf ( atextbuf );

    if ( !is_same ) {
        free_lbuf ( condbuf );
    }
}


/*
 * ---------------------------------------------------------------------------
 * perform_grep: grep (exact match), wildgrep (wildcard match), regrep
 * (regexp match), and case-insensitive versions. (There is no
 * case-insensitive wildgrep, since all wildcard matches are caseless.)
 */

void perform_grep ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int grep_type, caseless;
    pcre *re = NULL;
    pcre_extra *study = NULL;
    const char *errptr;
    int erroffset;
    int offsets[PCRE_MAX_OFFSETS];
    char *patbuf, *patc, *attrib, *p, *bb_p;
    int ca, aflags, alen;
    dbref thing, aowner, it;
    Delim osep;
    VaChk_Only_Out ( 4 );
    grep_type = Func_Mask ( REG_TYPE );
    caseless = Func_Mask ( REG_CASELESS );
    it = match_thing ( player, fargs[0] );

    if ( !Good_obj ( it ) ) {
        safe_nomatch ( buff, bufc );
        return;
    } else if ( ! ( Examinable ( player, it ) ) ) {
        safe_noperm ( buff, bufc );
        return;
    }

    /*
     * Make sure there's an attribute and a pattern
     */

    if ( !fargs[1] || !*fargs[1] ) {
        safe_str ( "#-1 NO SUCH ATTRIBUTE", buff, bufc );
        return;
    }

    if ( !fargs[2] || !*fargs[2] ) {
        safe_str ( "#-1 INVALID GREP PATTERN", buff, bufc );
        return;
    }

    switch ( grep_type ) {
    case GREP_EXACT:
        if ( caseless ) {
            for ( p = fargs[2]; *p; p++ ) {
                *p = tolower ( *p );
            }
        }

        break;

    case GREP_REGEXP:
        if ( ( re = pcre_compile ( fargs[2], caseless, &errptr, &erroffset,
                                   mudstate.retabs ) ) == NULL ) {
            notify_quiet ( player, errptr );
            return;
        }

        study = pcre_study ( re, 0, &errptr );

        if ( errptr != NULL ) {
            xfree ( re, "perform_grep.re" );
            notify_quiet ( player, errptr );
            return;
        }

        break;

    default:
        /*
         * No special set-up steps.
         */
        break;
    }

    bb_p = *bufc;
    patc = patbuf = alloc_lbuf ( "perform_grep.parse_attrib" );
    safe_sprintf ( patbuf, &patc, "#%d/%s", it, fargs[1] );
    olist_push();

    if ( parse_attrib_wild ( player, patbuf, &thing, 0, 0, 1, 1 ) ) {
        for ( ca = olist_first(); ca != NOTHING; ca = olist_next() ) {
            attrib = atr_get ( thing, ca, &aowner, &aflags, &alen );

            if ( ( grep_type == GREP_EXACT ) && caseless ) {
                for ( p = attrib; *p; p++ ) {
                    *p = tolower ( *p );
                }
            }

            if ( ( ( grep_type == GREP_EXACT )
                    && strstr ( attrib, fargs[2] ) )
                    || ( ( grep_type == GREP_WILD )
                         && quick_wild ( fargs[2], attrib ) )
                    || ( ( grep_type == GREP_REGEXP )
                         && ( pcre_exec ( re, study, attrib, alen, 0, 0,
                                          offsets, PCRE_MAX_OFFSETS ) >= 0 ) ) ) {
                if ( *bufc != bb_p ) {
                    print_sep ( &osep, buff, bufc );
                }

                safe_str ( ( char * ) ( atr_num ( ca ) )->name, buff,
                           bufc );
            }

            free_lbuf ( attrib );
        }
    }

    free_lbuf ( patbuf );
    olist_pop();

    if ( re ) {
        xfree ( re, "perform_grep.re" );
    }

    if ( study ) {
        xfree ( study, "perform_grep.study" );
    }
}

/*
 * ---------------------------------------------------------------------------
 * Grids. gridmake(<rows>, <columns>[,<grid text>][,<col odelim>][,<row
 * odelim>]) gridload(<grid text>[,<odelim for row elems>][,<odelim between
 * rows>]) gridset(<y range>,<x range>,<value>[,<input sep for ranges>])
 * gridsize() - returns rows cols grid( , [,<odelim for row elems>][,<odelim
 * between rows>]) - whole grid grid(<y>,<x>) - show particular coordinate
 * grid(<y range>,<x range>[,<odelim for row elems>][,<odelim between rows>])
 */

#define grid_get(x)  ((OBJGRID *) nhashfind(x, &mudstate.objgrid_htab))

#define grid_raw_set(gp,gr,gc,gv) \
     if ((gp)->data[(gr)][(gc)] != NULL) { \
         xfree((gp)->data[(gr)][(gc)], "grid_set.replace");   \
     } \
     if (*(gv)) {                        \
             (gp)->data[(gr)][(gc)] = xstrdup((gv), "grid_set"); \
     }

#define grid_set(gp,gr,gc,gv,ge) \
     if (((gr) < 0) || ((gc) < 0) || \
     ((gr) >= (gp)->rows) || ((gc) >= (gp)->cols)) { \
     (ge)++; \
     } else { \
         grid_raw_set((gp),(gr),(gc),(gv)); \
     }

#define grid_print(gp,gr,gc,gn,gsep) \
     if (gn) { \
     print_sep(&(gsep), buff, bufc); \
     } \
     if (!(((gr) < 0) || ((gc) < 0) || \
       ((gr) >= (gp)->rows) || ((gc) >= (gp)->cols) || \
       ((gp)->data[(gr)][(gc)] == NULL))) { \
     safe_str((gp)->data[(gr)][(gc)], buff, bufc); \
     }

static void grid_free ( dbref thing, OBJGRID *ogp )
{
    int r, c;

    if ( ogp ) {
        for ( r = 0; r < ogp->rows; r++ ) {
            for ( c = 0; c < ogp->cols; c++ ) {
                if ( ogp->data[r][c] != NULL ) {
                    xfree ( ogp->data[r][c],
                            "grid_free_elem" );
                }
            }

            xfree ( ogp->data[r], "grid_free_row" );
        }

        nhashdelete ( thing, &mudstate.objgrid_htab );
        xfree ( ogp, "grid_free" );
    }
}

void fun_gridmake ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    OBJGRID *ogp;
    int rows, cols, dimension, r, c, status, data_rows, data_elems, errs;
    char *rbuf, *pname;
    char **row_text, **elem_text;
    Delim csep, rsep;
    VaChk_Range ( 2, 5 );
    VaChk_SepIn ( csep, 4, 0 );
    VaChk_SepIn ( rsep, 5, 0 );
    rows = ( int ) strtol ( fargs[0], ( char ** ) NULL, 10 );
    cols = ( int ) strtol ( fargs[1], ( char ** ) NULL, 10 );
    dimension = rows * cols;

    if ( ( dimension > mudconf.max_grid_size ) || ( dimension < 0 ) ) {
        safe_str ( "#-1 INVALID GRID SIZE", buff, bufc );
        return;
    }

    ogp = grid_get ( player );

    if ( ogp ) {
        grid_free ( player, ogp );
    }

    if ( dimension == 0 ) {
        return;
    }

    /*
     * We store the grid on a row-by-row basis, i.e., the first index is
     * the y-coord and the second is the x-coord.
     */
    ogp = ( OBJGRID * ) xmalloc ( sizeof ( OBJGRID ), "grid_make" );
    ogp->rows = rows;
    ogp->cols = cols;
    ogp->data = ( char *** ) xcalloc ( rows, sizeof ( char *** ), "grid_data" );

    for ( r = 0; r < rows; r++ ) {
        ogp->data[r] =
            ( char ** ) xcalloc ( cols, sizeof ( char * ), "grid_row" );
    }

    status = nhashadd ( player, ( int * ) ogp, &mudstate.objgrid_htab );

    if ( status < 0 ) {
        pname = log_getname ( player, "fun_gridmake" );
        log_write ( LOG_BUGS, "GRD", "MAKE", "%s Failure" );
        xfree ( pname, "fun_gridmake" );
        grid_free ( player, ogp );
        safe_str ( "#-1 FAILURE", buff, bufc );
        return;
    }

    /*
     * Populate data if we have any
     */

    if ( !fargs[2] || !*fargs[2] ) {
        return;
    }

    rbuf = alloc_lbuf ( "fun_gridmake.rows" );
    strcpy ( rbuf, fargs[2] );
    data_rows = list2arr ( &row_text, LBUF_SIZE / 2, rbuf, &rsep );

    if ( data_rows > rows ) {
        safe_str ( "#-1 TOO MANY DATA ROWS", buff, bufc );
        free_lbuf ( rbuf );
        grid_free ( player, ogp );
        return;
    }

    errs = 0;

    for ( r = 0; r < data_rows; r++ ) {
        data_elems =
            list2arr ( &elem_text, LBUF_SIZE / 2, row_text[r], &csep );

        if ( data_elems > cols ) {
            safe_sprintf ( buff, bufc, "#-1 ROW %d HAS TOO MANY ELEMS", r );
            free_lbuf ( rbuf );
            grid_free ( player, ogp );
            return;
        }

        for ( c = 0; c < data_elems; c++ ) {
            grid_raw_set ( ogp, r, c, elem_text[c] );
        }
    }

    free_lbuf ( rbuf );
}

void fun_gridsize ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    OBJGRID *ogp;
    ogp = grid_get ( player );

    if ( !ogp ) {
        safe_str ( "0 0", buff, bufc );
    } else {
        safe_sprintf ( buff, bufc, "%d %d", ogp->rows, ogp->cols );
    }
}

void fun_gridset ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    OBJGRID *ogp;
    char *xlist, *ylist;
    int n_x, n_y, r, c, i, j, errs;
    char **x_elems, **y_elems;
    Delim isep;
    VaChk_Only_In ( 4 );
    ogp = grid_get ( player );

    if ( !ogp ) {
        safe_str ( "#-1 NO GRID", buff, bufc );
        return;
    }

    /*
     * Handle the common case of just one position and a simple
     * separator, first.
     */

    if ( ( isep.len == 1 ) &&
            *fargs[0] && !strchr ( fargs[0], isep.str[0] ) &&
            *fargs[1] && !strchr ( fargs[1], isep.str[0] ) ) {
        r = ( int ) strtol ( fargs[0], ( char ** ) NULL, 10 ) - 1;
        c = ( int ) strtol ( fargs[1], ( char ** ) NULL, 10 ) - 1;
        grid_set ( ogp, r, c, fargs[2], errs );

        if ( errs ) {
            safe_sprintf ( buff, bufc, "#-1 GOT %d OUT OF RANGE ERRORS", errs );
        }

        return;
    }

    /*
     * Complex ranges
     */

    if ( fargs[0] && *fargs[0] ) {
        ylist = alloc_lbuf ( "fun_gridset.ylist" );
        strcpy ( ylist, fargs[0] );
        n_y = list2arr ( &y_elems, LBUF_SIZE / 2, ylist, &isep );

        if ( ( n_y == 1 ) && !*y_elems[0] ) {
            free_lbuf ( ylist );
            n_y = -1;
        }
    } else {
        n_y = -1;
    }

    if ( fargs[1] && *fargs[1] ) {
        xlist = alloc_lbuf ( "fun_gridset.xlist" );
        strcpy ( xlist, fargs[1] );
        n_x = list2arr ( &x_elems, LBUF_SIZE / 2, xlist, &isep );

        if ( ( n_x == 1 ) && !*x_elems[0] ) {
            free_lbuf ( xlist );
            n_x = -1;
        }
    } else {
        n_x = -1;
    }

    errs = 0;

    if ( n_y == -1 ) {
        for ( r = 0; r < ogp->rows; r++ ) {
            if ( n_x == -1 ) {
                for ( c = 0; c < ogp->cols; c++ ) {
                    grid_raw_set ( ogp, r, c, fargs[2] );
                }
            } else {
                for ( i = 0; i < n_x; i++ ) {
                    c = ( int ) strtol ( x_elems[i], ( char ** ) NULL, 10 ) - 1;
                    grid_set ( ogp, r, c, fargs[2], errs );
                }
            }
        }
    } else {
        for ( j = 0; j < n_y; j++ ) {
            r = ( int ) strtol ( y_elems[j], ( char ** ) NULL, 10 ) - 1;

            if ( ( r < 0 ) || ( r >= ogp->rows ) ) {
                errs++;
            } else {
                if ( n_x == -1 ) {
                    for ( c = 0; c < ogp->cols; c++ ) {
                        grid_set ( ogp, r, c, fargs[2],
                                   errs );
                    }
                } else {
                    for ( i = 0; i < n_x; i++ ) {
                        c = ( int ) strtol ( x_elems[i], ( char ** ) NULL, 10 ) - 1;
                        grid_set ( ogp, r, c, fargs[2],
                                   errs );
                    }
                }
            }
        }
    }

    if ( n_x > 0 ) {
        free_lbuf ( xlist );
    }

    if ( n_y > 0 ) {
        free_lbuf ( ylist );
    }

    if ( errs ) {
        safe_sprintf ( buff, bufc, "#-1 GOT %d OUT OF RANGE ERRORS", errs );
    }
}

void fun_grid ( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim csep, rsep;
    OBJGRID *ogp;
    char *xlist, *ylist, *bb_p;
    int n_x, n_y, r, c, i, j, errs;
    char **x_elems, **y_elems;
    VaChk_Range ( 0, 4 );
    VaChk_SepOut ( csep, 3, 0 );
    VaChk_SepOut ( rsep, 4, 0 );
    ogp = grid_get ( player );

    if ( !ogp ) {
        safe_str ( "#-1 NO GRID", buff, bufc );
        return;
    }

    /*
     * Handle the common case of just one position, first
     */

    if ( fargs[0] && *fargs[0] && !strchr ( fargs[0], ' ' ) &&
            fargs[1] && *fargs[1] && !strchr ( fargs[1], ' ' ) ) {
        r = ( int ) strtol ( fargs[0], ( char ** ) NULL, 10 ) - 1;
        c = ( int ) strtol ( fargs[1], ( char ** ) NULL, 10 ) - 1;
        grid_print ( ogp, r, c, 0, csep );
        return;
    }

    /*
     * Complex ranges
     */

    if ( !fargs[0] || !*fargs[0] ) {
        n_y = -1;
    } else {
        ylist = alloc_lbuf ( "fun_grid.ylist" );
        strcpy ( ylist, fargs[0] );
        n_y = list2arr ( &y_elems, LBUF_SIZE / 2, ylist, &SPACE_DELIM );

        if ( ( n_y == 1 ) && !*y_elems[0] ) {
            free_lbuf ( ylist );
            n_y = -1;
        }
    }

    if ( !fargs[1] || !*fargs[1] ) {
        n_x = -1;
    } else {
        xlist = alloc_lbuf ( "fun_grid.xlist" );
        strcpy ( xlist, fargs[1] );
        n_x = list2arr ( &x_elems, LBUF_SIZE / 2, xlist, &SPACE_DELIM );

        if ( ( n_x == 1 ) && !*x_elems[0] ) {
            free_lbuf ( xlist );
            n_x = -1;
        }
    }

    if ( n_y == -1 ) {
        for ( r = 0; r < ogp->rows; r++ ) {
            if ( r != 0 ) {
                print_sep ( &rsep, buff, bufc );
            }

            if ( n_x == -1 ) {
                for ( c = 0; c < ogp->cols; c++ ) {
                    grid_print ( ogp, r, c, ( c != 0 ), csep );
                }
            } else {
                for ( i = 0; i < n_x; i++ ) {
                    c = ( int ) strtol ( x_elems[i], ( char ** ) NULL, 10 ) - 1;
                    grid_print ( ogp, r, c, ( i != 0 ), csep );
                }
            }
        }
    } else {
        for ( j = 0; j < n_y; j++ ) {
            if ( j != 0 ) {
                print_sep ( &rsep, buff, bufc );
            }

            r = ( int ) strtol ( y_elems[j], ( char ** ) NULL, 10 ) - 1;

            if ( ! ( ( r < 0 ) || ( r >= ogp->rows ) ) ) {
                if ( n_x == -1 ) {
                    for ( c = 0; c < ogp->cols; c++ ) {
                        grid_print ( ogp, r, c, ( c != 0 ),
                                     csep );
                    }
                } else {
                    for ( i = 0; i < n_x; i++ ) {
                        c = ( int ) strtol ( x_elems[i], ( char ** ) NULL, 10 ) - 1;
                        grid_print ( ogp, r, c, ( i != 0 ),
                                     csep );
                    }
                }
            }
        }
    }

    if ( n_x > 0 ) {
        free_lbuf ( xlist );
    }

    if ( n_y > 0 ) {
        free_lbuf ( ylist );
    }
}
