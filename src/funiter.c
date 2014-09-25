/* funiter.c - functions for user-defined iterations over lists */

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

#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "interface.h"
#include "externs.h"		/* required by code */

#include "functions.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */

/*
 * ---------------------------------------------------------------------------
 * perform_loop: backwards-compatible looping constructs: LOOP, PARSE See
 * notes on perform_iter for the explanation.
 */

void perform_loop( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;

    int flag;		/* 0 is parse(), 1 is loop() */

    char *curr, *objstring, *buff2, *buff3, *cp, *dp, *str, *result, *bb_p;

    char tbuf[8];

    int number = 0;

    flag = Func_Mask( LOOP_NOTIFY );

    if( flag ) {
        VaChk_Only_InEval( 3 );
    } else {
        VaChk_InEval_OutEval( 2, 4 );
    }

    dp = cp = curr = alloc_lbuf( "perform_loop.1" );
    str = fargs[0];
    exec( curr, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
          &str, cargs, ncargs );
    cp = trim_space_sep( cp, &isep );
    if( !*cp ) {
        free_lbuf( curr );
        return;
    }
    bb_p = *bufc;

    while( cp && ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {
        if( !flag && ( *bufc != bb_p ) ) {
            print_sep( &osep, buff, bufc );
        }
        number++;
        objstring = split_token( &cp, &isep );
        buff2 = replace_string( BOUND_VAR, objstring, fargs[1] );
        ltos( tbuf, number );
        buff3 = replace_string( LISTPLACE_VAR, tbuf, buff2 );
        str = buff3;
        if( !flag ) {
            exec( buff, bufc, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
        } else {
            dp = result = alloc_lbuf( "perform_loop.2" );
            exec( result, &dp, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
            notify( cause, result );
            free_lbuf( result );
        }
        free_lbuf( buff2 );
        free_lbuf( buff3 );
    }
    free_lbuf( curr );
}

/*
 * ---------------------------------------------------------------------------
 * perform_iter: looping constructs
 *
 * iter() and list() parse an expression, substitute elements of a list, one at
 * a time, using the '##' replacement token. Uses of these functions can be
 * nested. In older versions of MUSH, these functions could not be nested.
 * parse() and loop() exist for reasons of backwards compatibility, since the
 * peculiarities of the way substitutions were done in the string
 * replacements make it necessary to provide some way of doing backwards
 * compatibility, in order to avoid breaking a lot of code that relies upon
 * particular patterns of necessary escaping.
 *
 * whentrue() and whenfalse() work similarly to iter(). whentrue() loops as long
 * as the expression evaluates to true. whenfalse() loops as long as the
 * expression evaluates to false.
 *
 * istrue() and isfalse() are inline filterbool() equivalents returning the
 * elements of the list which are true or false, respectively.
 *
 * iter2(), list2(), etc. are two-list versions of all of the above.
 */

void perform_iter( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;

    int flag;		/* 0 is iter(), 1 is list() */

    int bool_flag, filt_flag, two_flag;

    int need_result, need_bool;

    char *list_str, *lp, *input_p;

    char *list_str2, *lp2, *input_p2;

    char *str, *bb_p, *work_buf;

    char *ep, *savep, *dp, *result;

    int is_true, cur_lev, elen;

    char tmpbuf[1] = "";

    char tmpbuf2[1] = "";

    /*
     * Enforce maximum nesting level.
     */

    if( mudstate.in_loop >= MAX_ITER_NESTING - 1 ) {
        notify_quiet( player, "Exceeded maximum iteration nesting." );
        return;
    }
    /*
     * Figure out what functionality we're getting.
     */

    flag = Func_Mask( LOOP_NOTIFY );
    bool_flag = Func_Mask( BOOL_COND_TYPE );
    filt_flag = Func_Mask( FILT_COND_TYPE );
    two_flag = Func_Mask( LOOP_TWOLISTS );

    need_result = ( flag || ( filt_flag != FILT_COND_NONE ) ) ? 1 : 0;
    need_bool = ( ( bool_flag != BOOL_COND_NONE ) ||
                  ( filt_flag != FILT_COND_NONE ) ) ? 1 : 0;

    if( !two_flag ) {
        if( flag ) {
            VaChk_Only_InEval( 3 );
        } else {
            VaChk_InEval_OutEval( 2, 4 );
        }
        ep = fargs[1];
    } else {
        if( flag ) {
            VaChk_Only_InEval( 4 );
        } else {
            VaChk_InEval_OutEval( 3, 5 );
        }
        ep = fargs[2];
    }

    /*
     * The list argument is unevaluated. Go evaluate it.
     */

    input_p = lp = list_str = alloc_lbuf( "perform_iter.list" );
    str = fargs[0];
    exec( list_str, &lp, player, caller, cause,
          EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
    input_p = trim_space_sep( input_p, &isep );

    /*
     * Same thing for the second list arg, if we have it
     */

    if( two_flag ) {
        input_p2 = lp2 = list_str2 = alloc_lbuf( "perform_iter.list2" );
        str = fargs[1];
        exec( list_str2, &lp2, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs );
        input_p2 = trim_space_sep( input_p2, &isep );
    } else {
        input_p2 = lp2 = list_str2 = NULL;
    }

    /*
     * If both lists are empty, we're done
     */

    if( !( input_p && *input_p ) && !( input_p2 && *input_p2 ) ) {
        free_lbuf( list_str );
        if( list_str2 ) {
            free_lbuf( list_str2 );
        }
        return;
    }
    cur_lev = mudstate.in_loop++;
    mudstate.loop_token[cur_lev] = NULL;
    mudstate.loop_token2[cur_lev] = NULL;
    mudstate.loop_number[cur_lev] = 0;
    mudstate.loop_break[cur_lev] = 0;

    bb_p = *bufc;
    elen = strlen( ep );

    while( ( input_p || input_p2 ) &&
            !mudstate.loop_break[cur_lev] &&
            ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {
        if( !need_result && ( *bufc != bb_p ) ) {
            print_sep( &osep, buff, bufc );
        }
        if( input_p )
            mudstate.loop_token[cur_lev] =
                split_token( &input_p, &isep );
        else {
            mudstate.loop_token[cur_lev] = tmpbuf;
        }
        if( input_p2 )
            mudstate.loop_token2[cur_lev] =
                split_token( &input_p2, &isep );
        else {
            mudstate.loop_token2[cur_lev] = tmpbuf;
        }
        mudstate.loop_number[cur_lev] += 1;
        work_buf = alloc_lbuf( "perform_iter.eval" );
        StrCopyKnown( work_buf, ep, elen );	/* we might nibble this */
        str = work_buf;
        savep = *bufc;
        if( !need_result ) {
            exec( buff, bufc, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
            if( need_bool ) {
                is_true = xlate( savep );
            }
        } else {
            dp = result = alloc_lbuf( "perform_iter.out" );
            exec( result, &dp, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs,
                  ncargs );
            if( need_bool ) {
                is_true = xlate( result );
            }
            if( flag ) {
                notify( cause, result );
            } else if( ( ( filt_flag == FILT_COND_TRUE ) && is_true )
                       || ( ( filt_flag == FILT_COND_FALSE ) && !is_true ) ) {
                if( *bufc != bb_p ) {
                    print_sep( &osep, buff, bufc );
                }
                safe_str( mudstate.loop_token[cur_lev], buff,
                          bufc );
            }
            free_lbuf( result );
        }
        free_lbuf( work_buf );
        if( bool_flag != BOOL_COND_NONE ) {
            if( !is_true && ( bool_flag == BOOL_COND_TRUE ) ) {
                break;
            }
            if( is_true && ( bool_flag == BOOL_COND_FALSE ) ) {
                break;
            }
        }
    }

    free_lbuf( list_str );
    if( list_str2 ) {
        free_lbuf( list_str2 );
    }
    mudstate.in_loop--;
}

/*
 * ---------------------------------------------------------------------------
 * itext(), inum(), ilev(): Obtain nested iter tokens (##, #@, #!).
 */

void fun_ilev( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, mudstate.in_loop - 1 );
}

void fun_inum( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int lev;

    lev = ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    if( ( lev > mudstate.in_loop - 1 ) || ( lev < 0 ) ) {
        safe_chr( '0', buff, bufc );
        return;
    }
    safe_ltos( buff, bufc, mudstate.loop_number[lev] );
}

void fun_itext( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int lev;

    lev = ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    if( ( lev > mudstate.in_loop - 1 ) || ( lev < 0 ) ) {
        return;
    }

    safe_str( mudstate.loop_token[lev], buff, bufc );
}

void fun_itext2( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int lev;

    lev = ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    if( ( lev > mudstate.in_loop - 1 ) || ( lev < 0 ) ) {
        return;
    }

    safe_str( mudstate.loop_token2[lev], buff, bufc );
}

void fun_ibreak( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int lev;

    lev = mudstate.in_loop - 1 - ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    if( ( lev > mudstate.in_loop - 1 ) || ( lev < 0 ) ) {
        return;
    }
    mudstate.loop_break[lev] = 1;
}

/*
 * ---------------------------------------------------------------------------
 * fun_fold: iteratively eval an attrib with a list of arguments and an
 * optional base case.  With no base case, the first list element is passed
 * as %0 and the second is %1.  The attrib is then evaluated with these args,
 * the result is then used as %0 and the next arg is %1 and so it goes as
 * there are elements left in the list.  The optinal base case gives the user
 * a nice starting point.
 *
 * > &REP_NUM object=[%0][repeat(%1,%1)] > say fold(OBJECT/REP_NUM,1 2 3 4 5,->)
 * You say "->122333444455555"
 *
 * NOTE: To use added list separator, you must use base case!
 */

void fun_fold( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref aowner, thing;

    int aflags, alen, anum, i;

    ATTR *ap;

    char *atext, *result, *curr, *bp, *str, *cp, *atextbuf;

    char *op, *clist[3], *rstore;

    Delim isep;

    /*
     * We need two to four arguments only
     */

    VaChk_In( 2, 4 );

    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>.
     */

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    /*
     * Evaluate it using the rest of the passed function args
     */

    cp = curr = fargs[1];
    atextbuf = alloc_lbuf( "fun_fold" );
    StrCopyKnown( atextbuf, atext, alen );

    /*
     * may as well handle first case now
     */

    i = 1;
    clist[2] = alloc_sbuf( "fun_fold.objplace" );
    op = clist[2];
    safe_ltos( clist[2], &op, i );

    if( ( nfargs >= 3 ) && ( fargs[2] ) ) {
        clist[0] = fargs[2];
        clist[1] = split_token( &cp, &isep );
        result = bp = alloc_lbuf( "fun_fold" );
        str = atextbuf;
        exec( result, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3 );
        i++;
    } else {
        clist[0] = split_token( &cp, &isep );
        clist[1] = split_token( &cp, &isep );
        result = bp = alloc_lbuf( "fun_fold" );
        str = atextbuf;
        exec( result, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3 );
        i += 2;
    }

    rstore = result;
    result = NULL;

    while( cp && ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {
        clist[0] = rstore;
        clist[1] = split_token( &cp, &isep );
        op = clist[2];
        safe_ltos( clist[2], &op, i );
        StrCopyKnown( atextbuf, atext, alen );
        result = bp = alloc_lbuf( "fun_fold" );
        str = atextbuf;
        exec( result, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, clist, 3 );
        strcpy( rstore, result );
        free_lbuf( result );
        i++;
    }
    safe_str( rstore, buff, bufc );
    free_lbuf( rstore );
    free_lbuf( atext );
    free_lbuf( atextbuf );
    free_sbuf( clist[2] );
}

/*
 * ---------------------------------------------------------------------------
 * fun_filter: iteratively perform a function with a list of arguments and
 * return the arg, if the function evaluates to TRUE using the arg.
 *
 * > &IS_ODD object=mod(%0,2) > say filter(object/is_odd,1 2 3 4 5) You say "1 3
 * 5" > say filter(object/is_odd,1-2-3-4-5,-) You say "1-3-5"
 *
 * NOTE:  If you specify a separator it is used to delimit returned list
 */

void handle_filter( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;

    int flag;		/* 0 is filter(), 1 is filterbool() */

    dbref aowner, thing;

    int aflags, alen, anum, i;

    ATTR *ap;

    char *atext, *result, *curr, *objs[2], *bp, *str, *cp, *op, *atextbuf;

    char *bb_p;

    flag = Func_Mask( LOGIC_BOOL );

    VaChk_Only_In_Out( 4 );

    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>.
     */

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    /*
     * Now iteratively eval the attrib with the argument list
     */

    cp = curr = trim_space_sep( fargs[1], &isep );
    atextbuf = alloc_lbuf( "fun_filter.atextbuf" );
    objs[1] = alloc_sbuf( "fun_filter.objplace" );
    bb_p = *bufc;
    i = 1;
    while( cp ) {
        objs[0] = split_token( &cp, &isep );
        op = objs[1];
        safe_ltos( objs[1], &op, i );
        StrCopyKnown( atextbuf, atext, alen );
        result = bp = alloc_lbuf( "fun_filter" );
        str = atextbuf;
        exec( result, &bp, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2 );
        if( ( !flag && ( *result == '1' ) ) || ( flag && xlate( result ) ) ) {
            if( *bufc != bb_p ) {
                print_sep( &osep, buff, bufc );
            }
            safe_str( objs[0], buff, bufc );
        }
        free_lbuf( result );
        i++;
    }
    free_lbuf( atext );
    free_lbuf( atextbuf );
    free_sbuf( objs[1] );
}

/*
 * ---------------------------------------------------------------------------
 * fun_map: iteratively evaluate an attribute with a list of arguments. >
 * &DIV_TWO object=fdiv(%0,2) > say map(1 2 3 4 5,object/div_two) You say
 * "0.5 1 1.5 2 2.5" > say map(object/div_two,1-2-3-4-5,-) You say
 * "0.5-1-1.5-2-2.5"
 *
 */

void fun_map( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref aowner, thing;

    int aflags, alen, anum;

    ATTR *ap;

    char *atext, *objs[2], *str, *cp, *atextbuf, *bb_p, *op;

    Delim isep, osep;

    int i;

    VaChk_Only_In_Out( 4 );

    /*
     * If we don't have anything for a second arg, don't bother.
     */
    if( !fargs[1] || !*fargs[1] ) {
        return;
    }

    /*
     * Two possibilities for the second arg: <obj>/<attr> and <attr>.
     */

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    /*
     * now process the list one element at a time
     */

    cp = trim_space_sep( fargs[1], &isep );
    atextbuf = alloc_lbuf( "fun_map.atextbuf" );
    objs[1] = alloc_sbuf( "fun_map.objplace" );
    bb_p = *bufc;
    i = 1;
    while( cp && ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        objs[0] = split_token( &cp, &isep );
        op = objs[1];
        safe_ltos( objs[1], &op, i );
        StrCopyKnown( atextbuf, atext, alen );
        str = atextbuf;
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2 );
        i++;
    }
    free_lbuf( atext );
    free_lbuf( atextbuf );
    free_sbuf( objs[1] );
}

/*
 * ---------------------------------------------------------------------------
 * fun_mix: Like map, but operates on two lists or more lists simultaneously,
 * passing the elements as %0, %1, %2, etc.
 */

void fun_mix( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref aowner, thing;

    int aflags, alen, anum, i, lastn, nwords, wc;

    ATTR *ap;

    char *str, *atext, *os[NUM_ENV_VARS], *atextbuf, *bb_p;

    Delim isep;

    char *cp[NUM_ENV_VARS];

    int count[NUM_ENV_VARS];

    char tmpbuf[1] = "";

    /*
     * Check to see if we have an appropriate number of arguments. If
     * there are more than three arguments, the last argument is ALWAYS
     * assumed to be a delimiter.
     */

    VaChk_Range( 3, 12 );
    if( nfargs < 4 ) {
        isep.str[0] = ' ';
        isep.len = 1;
        lastn = nfargs - 1;
    } else if( !delim_check( buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, nfargs, &isep, DELIM_STRING ) ) {
        return;
    } else {
        lastn = nfargs - 2;
    }

    /*
     * Get the attribute, check the permissions.
     */

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    for( i = 0; i < NUM_ENV_VARS; i++ ) {
        cp[i] = NULL;
    }

    bb_p = *bufc;

    /*
     * process the lists, one element at a time.
     */

    nwords = 0;
    for( i = 0; i < lastn; i++ ) {
        cp[i] = trim_space_sep( fargs[i + 1], &isep );
        count[i] = countwords( cp[i], &isep );
        if( count[i] > nwords ) {
            nwords = count[i];
        }
    }
    atextbuf = alloc_lbuf( "fun_mix" );

    for( wc = 0;
            ( wc < nwords ) && ( mudstate.func_invk_ctr < mudconf.func_invk_lim )
            && !Too_Much_CPU(); wc++ ) {
        for( i = 0; i < lastn; i++ ) {
            if( wc < count[i] ) {
                os[i] = split_token( &cp[i], &isep );
            } else {
                os[i] = tmpbuf;
            }
        }
        StrCopyKnown( atextbuf, atext, alen );

        if( *bufc != bb_p ) {
            print_sep( &isep, buff, bufc );
        }
        str = atextbuf;

        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, os, lastn );
    }
    free_lbuf( atext );
    free_lbuf( atextbuf );
}

/*
 * ---------------------------------------------------------------------------
 * fun_step: A little like a fusion of iter() and mix(), it takes elements of
 * a list X at a time and passes them into a single function as %0, %1, etc.
 * step(<attribute>,<list>,<step size>,<delim>,<outdelim>)
 */

void fun_step( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    ATTR *ap;

    dbref aowner, thing;

    int aflags, alen, anum;

    char *atext, *str, *cp, *atextbuf, *bb_p, *os[NUM_ENV_VARS];

    Delim isep, osep;

    int step_size, i;

    VaChk_Only_In_Out( 5 );

    step_size = ( int ) strtol( fargs[2], ( char **) NULL, 10 );
    if( ( step_size < 1 ) || ( step_size > NUM_ENV_VARS ) ) {
        notify( player, "Illegal step size." );
        return;
    }
    /*
     * Get attribute. Check permissions.
     */

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    cp = trim_space_sep( fargs[1], &isep );
    atextbuf = alloc_lbuf( "fun_step" );
    bb_p = *bufc;
    while( cp && ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        for( i = 0; cp && ( i < step_size ); i++ ) {
            os[i] = split_token( &cp, &isep );
        }
        StrCopyKnown( atextbuf, atext, alen );
        str = atextbuf;
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, & ( os[0] ), i );
    }
    free_lbuf( atext );
    free_lbuf( atextbuf );
}

/*
 * ---------------------------------------------------------------------------
 * fun_foreach: like map(), but it operates on a string, rather than on a
 * list, calling a user-defined function for each character in the string. No
 * delimiter is inserted between the results.
 */

void fun_foreach( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref aowner, thing;

    int aflags, alen, anum, i;

    ATTR *ap;

    char *str, *atext, *atextbuf, *cp, *cbuf[2], *op;

    char start_token, end_token;

    int in_string = 1;

    VaChk_Range( 2, 4 );

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    atextbuf = alloc_lbuf( "fun_foreach" );
    cbuf[0] = alloc_lbuf( "fun_foreach.cbuf" );
    cp = Eat_Spaces( fargs[1] );

    start_token = '\0';
    end_token = '\0';

    if( nfargs > 2 ) {
        in_string = 0;
        start_token = *fargs[2];
    }
    if( nfargs > 3 ) {
        end_token = *fargs[3];
    }
    i = -1;			/* first letter in string is 0, not 1 */
    cbuf[1] = alloc_sbuf( "fun_foreach.objplace" );

    while( cp && *cp && ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {

        if( !in_string ) {
            /*
             * Look for a start token.
             */
            while( *cp && ( *cp != start_token ) ) {
                safe_chr( *cp, buff, bufc );
                cp++;
                i++;
            }
            if( !*cp ) {
                break;
            }
            /*
             * Skip to the next character. Don't copy the start
             * token.
             */
            cp++;
            i++;
            if( !*cp ) {
                break;
            }
            in_string = 1;
        }
        if( *cp == end_token ) {
            /*
             * We've found an end token. Skip over it. Note that
             * it's possible to have a start and end token next
             * to one another.
             */
            cp++;
            i++;
            in_string = 0;
            continue;
        }
        i++;
        cbuf[0][0] = *cp++;
        cbuf[0][1] = '\0';
        op = cbuf[1];
        safe_ltos( cbuf[1], &op, i );
        StrCopyKnown( atextbuf, atext, alen );
        str = atextbuf;
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, cbuf, 2 );
    }

    free_lbuf( atextbuf );
    free_lbuf( atext );
    free_lbuf( cbuf[0] );
    free_sbuf( cbuf[1] );
}

/*
 * ---------------------------------------------------------------------------
 * fun_munge: combines two lists in an arbitrary manner.
 */

void fun_munge( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    dbref aowner, thing;

    int aflags, alen, anum, nptrs1, nptrs2, nresults, i, j;

    ATTR *ap;

    char *list1, *list2, *rlist, *st[2];

    char **ptrs1, **ptrs2, **results;

    char *atext, *bp, *str, *oldp;

    Delim isep, osep;

    oldp = *bufc;
    if( ( nfargs == 0 ) || !fargs[0] || !*fargs[0] ) {
        return;
    }
    VaChk_Only_In_Out( 5 );

    /*
     * Find our object and attribute
     */

    Get_Ulambda( player, thing, fargs[0],
                 anum, ap, atext, aowner, aflags, alen );

    /*
     * Copy our lists and chop them up.
     */

    list1 = alloc_lbuf( "fun_munge.list1" );
    list2 = alloc_lbuf( "fun_munge.list2" );
    strcpy( list1, fargs[1] );
    strcpy( list2, fargs[2] );
    nptrs1 = list2arr( &ptrs1, LBUF_SIZE / 2, list1, &isep );
    nptrs2 = list2arr( &ptrs2, LBUF_SIZE / 2, list2, &isep );

    if( nptrs1 != nptrs2 ) {
        safe_str( "#-1 LISTS MUST BE OF EQUAL SIZE", buff, bufc );
        free_lbuf( atext );
        free_lbuf( list1 );
        free_lbuf( list2 );
        XFREE( ptrs1, "fun_munge.ptrs1" );
        XFREE( ptrs2, "fun_munge.ptrs2" );
        return;
    }
    /*
     * Call the u-function with the first list as %0. Pass the input
     * separator as %1, which makes sorting, etc. easier.
     */

    st[0] = fargs[1];
    st[1] = alloc_lbuf( "fun_munge.sep" );
    bp = st[1];
    print_sep( &isep, st[1], &bp );

    bp = rlist = alloc_lbuf( "fun_munge.rlist" );
    str = atext;
    exec( rlist, &bp, player, caller, cause,
          EV_STRIP | EV_FCHECK | EV_EVAL, &str, st, 2 );

    /*
     * Now that we have our result, put it back into array form. Search
     * through list1 until we find the element position, then copy the
     * corresponding element from list2.
     */

    nresults = list2arr( &results, LBUF_SIZE / 2, rlist, &isep );

    for( i = 0; i < nresults; i++ ) {
        for( j = 0; j < nptrs1; j++ ) {
            if( !strcmp( results[i], ptrs1[j] ) ) {
                if( *bufc != oldp ) {
                    print_sep( &osep, buff, bufc );
                }
                safe_str( ptrs2[j], buff, bufc );
                ptrs1[j][0] = '\0';
                break;
            }
        }
    }
    free_lbuf( atext );
    free_lbuf( list1 );
    free_lbuf( list2 );
    free_lbuf( rlist );
    free_lbuf( st[1] );
    XFREE( ptrs1, "fun_munge.ptrs1" );
    XFREE( ptrs2, "fun_munge.ptrs2" );
    XFREE( results, "fun_munge.results" );
}

/*
 * ---------------------------------------------------------------------------
 * fun_while: Evaluate a list until a termination condition is met:
 * while(EVAL_FN,CONDITION_FN,foo|flibble|baz|meep,1,|,-) where EVAL_FN is
 * "[strlen(%0)]" and CONDITION_FN is "[strmatch(%0,baz)]" would result in
 * '3-7-3' being returned. The termination condition is an EXACT not wild
 * match.
 */

void fun_while( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;

    dbref aowner1, thing1, aowner2, thing2;

    int aflags1, aflags2, anum1, anum2, alen1, alen2, i, tmp_num;

    int is_same, is_exact_same;

    ATTR *ap, *ap2;

    char *atext1, *atext2, *atextbuf, *condbuf;

    char *objs[2], *cp, *str, *dp, *savep, *bb_p, *op;

    VaChk_Only_In_Out( 6 );

    /*
     * If our third arg is null (empty list), don't bother.
     */

    if( !fargs[2] || !*fargs[2] ) {
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

    Parse_Uattr( player, fargs[0], thing1, anum1, ap );
    Get_Uattr( player, thing1, ap, atext1, aowner1, aflags1, alen1 );
    tmp_num = ap->number;
    Parse_Uattr( player, fargs[1], thing2, anum2, ap2 );
    if( !ap2 ) {
        free_lbuf( atext1 );	/* we allocated this, remember? */
        return;
    }
    /*
     * If our evaluation and condition are the same, we can save
     * ourselves some time later. There are two possibilities: we have
     * the exact same obj/attr pair, or the attributes contain identical
     * text.
     */

    if( ( thing1 == thing2 ) && ( tmp_num == ap2->number ) ) {
        is_same = 1;
        is_exact_same = 1;
    } else {
        is_exact_same = 0;
        atext2 =
            atr_pget( thing2, ap2->number, &aowner2, &aflags2, &alen2 );
        if( !*atext2
                || !See_attr( player, thing2, ap2, aowner2, aflags2 ) ) {
            free_lbuf( atext1 );
            free_lbuf( atext2 );
            return;
        }
        if( !strcmp( atext1, atext2 ) ) {
            is_same = 1;
        } else {
            is_same = 0;
        }
    }

    /*
     * Process the list one element at a time.
     */

    cp = trim_space_sep( fargs[2], &isep );
    atextbuf = alloc_lbuf( "fun_while.eval" );
    if( !is_same ) {
        condbuf = alloc_lbuf( "fun_while.cond" );
    }
    objs[1] = alloc_sbuf( "fun_while.objplace" );
    bb_p = *bufc;
    i = 1;
    while( cp && ( mudstate.func_invk_ctr < mudconf.func_invk_lim ) &&
            !Too_Much_CPU() ) {
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        objs[0] = split_token( &cp, &isep );
        op = objs[1];
        safe_ltos( objs[1], &op, i );
        StrCopyKnown( atextbuf, atext1, alen1 );
        str = atextbuf;
        savep = *bufc;
        exec( buff, bufc, player, caller, cause,
              EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2 );
        if( !is_same ) {
            StrCopyKnown( atextbuf, atext2, alen2 );
            dp = savep = condbuf;
            str = atextbuf;
            exec( condbuf, &dp, player, caller, cause,
                  EV_STRIP | EV_FCHECK | EV_EVAL, &str, objs, 2 );
        }
        if( !strcmp( savep, fargs[3] ) ) {
            break;
        }
        i++;
    }
    free_lbuf( atext1 );
    if( !is_exact_same ) {
        free_lbuf( atext2 );
    }
    free_lbuf( atextbuf );
    if( !is_same ) {
        free_lbuf( condbuf );
    }
    free_sbuf( objs[1] );
}
