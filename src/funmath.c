/* funmath.c - math and logic functions */

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

/*
 * ---------------------------------------------------------------------------
 * fval: copy the floating point value into a buffer and make it presentable
 */

#define FP_SIZE ((sizeof(double) + sizeof(unsigned int) - 1) / sizeof(unsigned int))
#define FP_EXP_WEIRD    0x1
#define FP_EXP_ZERO 0x2

typedef union {
    double d;
    unsigned int u[FP_SIZE];
} fp_union_uint;

static unsigned int fp_check_weird( char *buff, char **bufc, double result )
{
    static fp_union_uint fp_sign_mask, fp_exp_mask, fp_mant_mask, fp_val;

    static const double d_zero = 0.0;

    static int fp_initted = 0;

    unsigned int fp_sign, fp_exp, fp_mant;

    int i;

    if( !fp_initted ) {
        memset( fp_sign_mask.u, 0, sizeof( fp_sign_mask ) );
        memset( fp_exp_mask.u, 0, sizeof( fp_exp_mask ) );
        memset( fp_val.u, 0, sizeof( fp_val ) );
        fp_exp_mask.d = 1.0 / d_zero;
        fp_sign_mask.d = -1.0 / fp_exp_mask.d;
        for( i = 0; i < FP_SIZE; i++ ) {
            fp_mant_mask.u[i] =
                ~( fp_exp_mask.u[i] | fp_sign_mask.u[i] );
        }
        fp_initted = 1;
    }
    fp_val.d = result;

    fp_sign = fp_mant = 0;
    fp_exp = FP_EXP_ZERO | FP_EXP_WEIRD;

    for( i = 0; ( i < FP_SIZE ) && fp_exp; i++ ) {
        if( fp_exp_mask.u[i] ) {
            unsigned int x = ( fp_exp_mask.u[i] & fp_val.u[i] );

            if( x == fp_exp_mask.u[i] ) {
                /*
                 * these bits are all set. can't be zero
                 * exponent, but could still be max (weird)
                 * exponent.
                 */
                fp_exp &= ~FP_EXP_ZERO;
            } else if( x == 0 ) {
                /*
                 * none of these bits are set. can't be max
                 * exponent, but could still be zero
                 * exponent.
                 */
                fp_exp &= ~FP_EXP_WEIRD;
            } else {
                /*
                 * some bits were set but not others. can't
                 * be either zero exponent or max exponent.
                 */
                fp_exp = 0;
            }
        }
        fp_sign |= ( fp_sign_mask.u[i] & fp_val.u[i] );
        fp_mant |= ( fp_mant_mask.u[i] & fp_val.u[i] );
    }
    if( fp_exp == FP_EXP_WEIRD ) {
        if( fp_sign ) {
            safe_chr( '-', buff, bufc );
        }
        if( fp_mant ) {
            safe_known_str( "NaN", 3, buff, bufc );
        } else {
            safe_known_str( "Inf", 3, buff, bufc );
        }
    }
    return fp_exp;
}

static void fval( char *buff, char **bufc, double result )
{
    char *p, *buf1;

    switch( fp_check_weird( buff, bufc, result ) ) {
    case FP_EXP_WEIRD:
        return;
    case FP_EXP_ZERO:
        result = 0.0;   /* FALLTHRU */
    default:
        break;
    }

    buf1 = *bufc;
    safe_sprintf( buff, bufc, "%.6f", result ); /* get double val into
                             * buffer */
    **bufc = '\0';
    p = strrchr( buf1, '0' );
    if( p == NULL ) {   /* remove useless trailing 0's */
        return;
    } else if( * ( p + 1 ) == '\0' ) {
        while( *p == '0' ) {
            *p-- = '\0';
        }
        *bufc = p + 1;
    }
    p = strrchr( buf1, '.' );   /* take care of dangling '.' */

    if( ( p != NULL ) && ( * ( p + 1 ) == '\0' ) ) {
        *p = '\0';
        *bufc = p;
    }
    /*
     * Handle bogus result of "-0" from sprintf.  Yay, cclib.
     */

    if( !strcmp( buf1, "-0" ) ) {
        *buf1 = '0';
        *bufc = buf1 + 1;
    }
}

/*
 * ---------------------------------------------------------------------------
 * Constant math funcs: PI, E
 */

void fun_pi( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_known_str( "3.141592654", 11, buff, bufc );
}

void fun_e( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_known_str( "2.718281828", 11, buff, bufc );
}

/*
 * ---------------------------------------------------------------------------
 * Math operations on one number: SIGN, ABS, FLOOR, CEIL, ROUND, TRUNC, INC,
 * DEC, SQRT, EXP, LN, [A][SIN,COS,TAN][D]
 */

void fun_sign( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL num;

    num = strtod( fargs[0], ( char **) NULL );
    if( num < 0 ) {
        safe_known_str( "-1", 2, buff, bufc );
    } else {
        safe_bool( buff, bufc, ( num > 0 ) );
    }
}

void fun_abs( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL num;

    num = strtod( fargs[0], ( char **) NULL );
    if( num == 0 ) {
        safe_chr( '0', buff, bufc );
    } else if( num < 0 ) {
        fval( buff, bufc, -num );
    } else {
        fval( buff, bufc, num );
    }
}

void fun_floor( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char *oldp;

    NVAL x;

    oldp = *bufc;
    x = floor( strtod( fargs[0], ( char **) NULL ) );
    switch( fp_check_weird( buff, bufc, x ) ) {
    case FP_EXP_WEIRD:
        return;
    case FP_EXP_ZERO:
        x = 0.0;    /* FALLTHRU */
    default:
        break;
    }
    safe_sprintf( buff, bufc, "%.0f", x );
    /*
     * Handle bogus result of "-0" from sprintf.  Yay, cclib.
     */

    if( !strcmp( oldp, "-0" ) ) {
        *oldp = '0';
        *bufc = oldp + 1;
    }
}

void fun_ceil( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char *oldp;

    NVAL x;

    oldp = *bufc;
    x = ceil( strtod( fargs[0], ( char **) NULL ) );
    switch( fp_check_weird( buff, bufc, x ) ) {
    case FP_EXP_WEIRD:
        return;
    case FP_EXP_ZERO:
        x = 0.0;    /* FALLTHRU */
    default:
        break;
    }
    safe_sprintf( buff, bufc, "%.0f", x );
    /*
     * Handle bogus result of "-0" from sprintf.  Yay, cclib.
     */

    if( !strcmp( oldp, "-0" ) ) {
        *oldp = '0';
        *bufc = oldp + 1;
    }
}

void fun_round( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    const char *fstr;

    char *oldp;

    NVAL x;

    oldp = *bufc;

    switch( ( int ) strtol( fargs[1], ( char **) NULL, 10 ) ) {
    case 1:
        fstr = "%.1f";
        break;
    case 2:
        fstr = "%.2f";
        break;
    case 3:
        fstr = "%.3f";
        break;
    case 4:
        fstr = "%.4f";
        break;
    case 5:
        fstr = "%.5f";
        break;
    case 6:
        fstr = "%.6f";
        break;
    default:
        fstr = "%.0f";
        break;
    }
    x = strtod( fargs[0], ( char **) NULL );
    switch( fp_check_weird( buff, bufc, x ) ) {
    case FP_EXP_WEIRD:
        return;
    case FP_EXP_ZERO:
        x = 0.0;    /* FALLTHRU */
    default:
        break;
    }
    safe_sprintf( buff, bufc, ( char *) fstr, x );

    /*
     * Handle bogus result of "-0" from sprintf.  Yay, cclib.
     */

    if( !strcmp( oldp, "-0" ) ) {
        *oldp = '0';
        *bufc = oldp + 1;
    }
}

void fun_trunc( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL x;

    x = strtod( fargs[0], ( char **) NULL );
    x = ( x >= 0 ) ? floor( x ) : ceil( x );
    switch( fp_check_weird( buff, bufc, x ) ) {
    case FP_EXP_WEIRD:
        return;
    case FP_EXP_ZERO:
        x = 0.0;    /* FALLTHRU */
    default:
        break;
    }
    fval( buff, bufc, x );
}

void fun_inc( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) + 1 );
}

void fun_dec( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) - 1 );
}

void fun_sqrt( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL val;

    val = strtod( fargs[0], ( char **) NULL );
    if( val < 0 ) {
        safe_str( "#-1 SQUARE ROOT OF NEGATIVE", buff, bufc );
    } else if( val == 0 ) {
        safe_chr( '0', buff, bufc );
    } else {
        fval( buff, bufc, sqrt( ( double ) val ) );
    }
}

void fun_exp( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    fval( buff, bufc, exp( ( double ) strtod( fargs[0], ( char **) NULL ) ) );
}

void fun_ln( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL val;

    val = strtod( fargs[0], ( char **) NULL );
    if( val > 0 ) {
        fval( buff, bufc, log( ( double ) val ) );
    } else {
        safe_str( "#-1 LN OF NEGATIVE OR ZERO", buff, bufc );
    }
}

void handle_trig( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL val;

    int oper, flag;

    static double( *const trig_funcs[8] )( double ) = {
        sin, cos, tan, NULL,    /* XXX no cotangent function */
        asin, acos, atan, NULL
    };

    flag = Func_Flags( fargs );
    oper = flag & TRIG_OPER;

    val = strtod( fargs[0], ( char **) NULL );
    if( ( flag & TRIG_ARC ) && !( flag & TRIG_TAN ) &&
            ( ( val < -1 ) || ( val > 1 ) ) ) {
        safe_sprintf( buff, bufc, "#-1 %s ARGUMENT OUT OF RANGE", ( ( FUN *) fargs[-1] )->name );
        return;
    }
    if( ( flag & TRIG_DEG ) && !( flag & TRIG_ARC ) ) {
        val = val * ( M_PI / 180 );
    }

    val = ( trig_funcs[oper] )( ( double ) val );

    if( ( flag & TRIG_DEG ) && ( flag & TRIG_ARC ) ) {
        val = ( val * 180 ) / M_PI;
    }

    fval( buff, bufc, val );
}

/*
 * ---------------------------------------------------------------------------
 * Base conversion: BASECONV
 */

static char from_base_64[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static char to_base_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static char from_base_36[256] = {
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

static char to_base_36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void fun_baseconv( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    long n, m;

    int from, to, isneg;

    char *frombase, *tobase, *p, *nbp;

    char nbuf[LBUF_SIZE];

    /*
     * Use the base 36 conversion table by default
     */

    frombase = from_base_36;
    tobase = to_base_36;

    /*
     * Figure out our bases
     */

    if( !is_integer( fargs[1] ) || !is_integer( fargs[2] ) ) {
        safe_known_str( "#-1 INVALID BASE", 16, buff, bufc );
        return;
    }
    from = ( int ) strtol( fargs[1], ( char **) NULL, 10 );
    to = ( int ) strtol( fargs[2], ( char **) NULL, 10 );

    if( ( from < 2 ) || ( from > 64 ) || ( to < 2 ) || ( to > 64 ) ) {
        safe_known_str( "#-1 BASE OUT OF RANGE", 21, buff, bufc );
        return;
    }
    if( from > 36 ) {
        frombase = from_base_64;
    }
    if( to > 36 ) {
        tobase = to_base_64;
    }

    /*
     * Parse the number to convert
     */

    p = Eat_Spaces( fargs[0] );
    n = 0;
    if( p ) {
        /*
         * If we have a leading hyphen, and we're in base 63/64,
         * always treat it as a minus sign. PennMUSH consistency.
         */
        if( ( from < 63 ) && ( to < 63 ) && ( *p == '-' ) ) {
            isneg = 1;
            p++;
        } else {
            isneg = 0;
        }
        while( *p ) {
            n *= from;
            if( frombase[( unsigned char ) *p] >= 0 ) {
                n += frombase[( unsigned char ) *p];
                p++;
            } else {
                safe_known_str( "#-1 MALFORMED NUMBER", 20,
                                buff, bufc );
                return;
            }
        }
        if( isneg ) {
            safe_chr( '-', buff, bufc );
        }
    }
    /*
     * Handle the case of 0 and less than base case.
     */

    if( n < to ) {
        safe_chr( tobase[( unsigned char ) n], buff, bufc );
        return;
    }
    /*
     * Build up the number backwards, then reverse it.
     */

    nbp = nbuf;
    while( n > 0 ) {
        m = n % to;
        n = n / to;
        safe_chr( tobase[( unsigned char ) m], nbuf, &nbp );
    }
    nbp--;
    while( nbp >= nbuf ) {
        safe_chr( *nbp, buff, bufc );
        nbp--;
    }
}

/*
 * ---------------------------------------------------------------------------
 * Math comparison funcs: GT, GTE, LT, LTE, EQ, NEQ, NCOMP
 */

void fun_gt( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) > strtod( fargs[1], ( char **) NULL ) ) );
}

void fun_gte( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) >= strtod( fargs[1], ( char **) NULL ) ) );
}

void fun_lt( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) < strtod( fargs[1], ( char **) NULL ) ) );
}

void fun_lte( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) <= strtod( fargs[1], ( char **) NULL ) ) );
}

void fun_eq( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) == strtod( fargs[1], ( char **) NULL ) ) );
}

void fun_neq( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) != strtod( fargs[1], ( char **) NULL ) ) );
}

void fun_ncomp( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL x, y;

    x = strtod( fargs[0], ( char **) NULL );
    y = strtod( fargs[1], ( char **) NULL );

    if( x == y ) {
        safe_chr( '0', buff, bufc );
    } else if( x < y ) {
        safe_str( "-1", buff, bufc );
    } else {
        safe_chr( '1', buff, bufc );
    }
}

/*
 * ---------------------------------------------------------------------------
 * Two-argument math functions: SUB, DIV, FLOORDIV, FDIV, MODULO, REMAINDER,
 * POWER, LOG
 */

void fun_sub( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    fval( buff, bufc, strtod( fargs[0], ( char **) NULL ) - strtod( fargs[1], ( char **) NULL ) );
}

void fun_div( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    long top, bot;

    /*
     * The C / operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     */

    top = strtol( fargs[0], ( char **) NULL, 10 );
    bot = strtol( fargs[1], ( char **) NULL, 10 );
    if( bot == 0 ) {
        safe_str( "#-1 DIVIDE BY ZERO", buff, bufc );
        return;
    }
    if( top < 0 ) {
        if( bot < 0 ) {
            top = ( -top ) / ( -bot );
        } else {
            top = - ( ( -top ) / bot );
        }
    } else {
        if( bot < 0 ) {
            top = - ( top / ( -bot ) );
        } else {
            top = top / bot;
        }
    }
    safe_ltos( buff, bufc, top );
}

void fun_floordiv( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int top, bot, res;

    /*
     * The C / operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     */

    top = ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    bot = ( int ) strtol( fargs[1], ( char **) NULL, 10 );
    if( bot == 0 ) {
        safe_str( "#-1 DIVIDE BY ZERO", buff, bufc );
        return;
    }
    if( top < 0 ) {
        if( bot < 0 ) {
            res = ( -top ) / ( -bot );
        } else {
            res = - ( ( -top ) / bot );
            if( top % bot ) {
                res--;
            }
        }
    } else {
        if( bot < 0 ) {
            res = - ( top / ( -bot ) );
            if( top % bot ) {
                res--;
            }
        } else {
            res = top / bot;
        }
    }
    safe_ltos( buff, bufc, res );
}

void fun_fdiv( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL bot;

    bot = strtod( fargs[1], ( char **) NULL );
    if( bot == 0 ) {
        safe_str( "#-1 DIVIDE BY ZERO", buff, bufc );
    } else {
        fval( buff, bufc, ( strtod( fargs[0], ( char **) NULL ) / bot ) );
    }
}

void fun_modulo( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int top, bot;

    /*
     * The C % operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     */

    top = ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    bot = ( int ) strtol( fargs[1], ( char **) NULL, 10 );
    if( bot == 0 ) {
        bot = 1;
    }
    if( top < 0 ) {
        if( bot < 0 ) {
            top = - ( ( -top ) % ( -bot ) );
        } else {
            top = ( bot - ( ( -top ) % bot ) ) % bot;
        }
    } else {
        if( bot < 0 ) {
            top = - ( ( ( -bot ) - ( top % ( -bot ) ) ) % ( -bot ) );
        } else {
            top = top % bot;
        }
    }
    safe_ltos( buff, bufc, top );
}

void fun_remainder( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int top, bot;

    /*
     * The C % operator is only fully specified for non-negative
     * operands, so we try not to give it negative operands here
     */

    top = ( int ) strtol( fargs[0], ( char **) NULL, 10 );
    bot = ( int ) strtol( fargs[1], ( char **) NULL, 10 );
    if( bot == 0 ) {
        bot = 1;
    }
    if( top < 0 ) {
        if( bot < 0 ) {
            top = - ( ( -top ) % ( -bot ) );
        } else {
            top = - ( ( -top ) % bot );
        }
    } else {
        if( bot < 0 ) {
            top = top % ( -bot );
        } else {
            top = top % bot;
        }
    }
    safe_ltos( buff, bufc, top );
}

void fun_power( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL val1, val2;

    val1 = strtod( fargs[0], ( char **) NULL );
    val2 = strtod( fargs[1], ( char **) NULL );
    if( val1 < 0 ) {
        safe_str( "#-1 POWER OF NEGATIVE", buff, bufc );
    } else {
        fval( buff, bufc, pow( ( double ) val1, ( double ) val2 ) );
    }
}

void fun_log( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL val, base;

    VaChk_Range( 1, 2 );

    val = strtod( fargs[0], ( char **) NULL );

    if( nfargs == 2 ) {
        base = strtod( fargs[1], ( char **) NULL );
    } else {
        base = 10;
    }

    if( ( val <= 0 ) || ( base <= 0 ) ) {
        safe_str( "#-1 LOG OF NEGATIVE OR ZERO", buff, bufc );
    } else if( base == 1 ) {
        safe_str( "#-1 DIVISION BY ZERO", buff, bufc );
    } else {
        fval( buff, bufc, log( ( double ) val ) / log( ( double ) base ) );
    }
}

/*
 * ------------------------------------------------------------------------
 * Bitwise two-argument integer math functions: SHL, SHR, BAND, BOR, BNAND
 */

void fun_shl( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) << ( int ) strtol( fargs[1], ( char **) NULL, 10 ) );
}

void fun_shr( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) >> ( int ) strtol( fargs[1], ( char **) NULL, 10 ) );
}

void fun_band( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) & ( int ) strtol( fargs[1], ( char **) NULL, 10 ) );
}

void fun_bor( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) | ( int ) strtol( fargs[1], ( char **) NULL, 10 ) );
}

void fun_bnand( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_ltos( buff, bufc, ( int ) strtol( fargs[0], ( char **) NULL, 10 ) & ~( ( int ) strtol( fargs[1], ( char **) NULL, 10 ) ) );
}

/*
 * ---------------------------------------------------------------------------
 * Multi-argument math functions: ADD, MUL, MAX, MIN
 */

void fun_add( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL sum;

    int i;

    if( nfargs < 2 ) {
        safe_known_str( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
    } else {
        sum = strtod( fargs[0], ( char **) NULL );
        for( i = 1; i < nfargs; i++ ) {
            sum += strtod( fargs[i], ( char **) NULL );
        }
        fval( buff, bufc, sum );
    }
    return;
}

void fun_mul( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL prod;

    int i;

    if( nfargs < 2 ) {
        safe_known_str( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
    } else {
        prod = strtod( fargs[0], ( char **) NULL );
        for( i = 1; i < nfargs; i++ ) {
            prod *= strtod( fargs[i], ( char **) NULL );
        }
        fval( buff, bufc, prod );
    }
    return;
}

void fun_max( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i;

    NVAL max, val;

    if( nfargs < 1 ) {
        safe_known_str( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
    } else {
        max = strtod( fargs[0], ( char **) NULL );
        for( i = 1; i < nfargs; i++ ) {
            val = strtod( fargs[i], ( char **) NULL );
            max = ( max < val ) ? val : max;
        }
        fval( buff, bufc, max );
    }
}

void fun_min( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int i;

    NVAL min, val;

    if( nfargs < 1 ) {
        safe_known_str( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
    } else {
        min = strtod( fargs[0], ( char **) NULL );
        for( i = 1; i < nfargs; i++ ) {
            val = strtod( fargs[i], ( char **) NULL );
            min = ( min > val ) ? val : min;
        }
        fval( buff, bufc, min );
    }
}

/*
 * ---------------------------------------------------------------------------
 * bound(): Force a number to conform to specified bounds.
 */

void fun_bound( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL min, max, val;

    char *cp;

    VaChk_Range( 1, 3 );

    val = strtod( fargs[0], ( char **) NULL );

    if( nfargs < 2 ) {  /* just the number; no bounds enforced */
        fval( buff, bufc, val );
        return;
    }
    if( nfargs > 1 ) {  /* if empty, don't check the minimum */
        for( cp = fargs[1]; *cp && isspace( *cp ); cp++ );
        if( *cp ) {
            min = strtod( fargs[1], ( char **) NULL );
            val = ( val < min ) ? min : val;
        }
    }
    if( nfargs > 2 ) {  /* if empty, don't check the maximum */
        for( cp = fargs[2]; *cp && isspace( *cp ); cp++ );
        if( *cp ) {
            max = strtod( fargs[2], ( char **) NULL );
            val = ( val > max ) ? max : val;
        }
    }
    fval( buff, bufc, val );
}

/*
 * ---------------------------------------------------------------------------
 * Integer point distance functions: DIST2D, DIST3D
 */

void fun_dist2d( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int d;

    double r;

    d = ( int ) strtol( fargs[0], ( char **) NULL, 10 ) - ( int ) strtol( fargs[2], ( char **) NULL, 10 );
    r = ( double )( d * d );
    d = ( int ) strtol( fargs[1], ( char **) NULL, 10 ) - ( int ) strtol( fargs[3], ( char **) NULL, 10 );
    r += ( double )( d * d );
    d = ( int )( sqrt( r ) + 0.5 );
    safe_ltos( buff, bufc, d );
}

void fun_dist3d( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    int d;

    double r;

    d = ( int ) strtol( fargs[0], ( char **) NULL, 10 ) - ( int ) strtol( fargs[3], ( char **) NULL, 10 );
    r = ( double )( d * d );
    d = ( int ) strtol( fargs[1], ( char **) NULL, 10 ) - ( int ) strtol( fargs[4], ( char **) NULL, 10 );
    r += ( double )( d * d );
    d = ( int ) strtol( fargs[2], ( char **) NULL, 10 ) - ( int ) strtol( fargs[5], ( char **) NULL, 10 );
    r += ( double )( d * d );
    d = ( int )( sqrt( r ) + 0.5 );
    safe_ltos( buff, bufc, d );
}

/*
 * ---------------------------------------------------------------------------
 * Math "accumulator" operations on a list: LADD, LMAX, LMIN
 */

void fun_ladd( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL sum;

    char *cp, *curr;

    Delim isep;

    if( nfargs == 0 ) {
        safe_chr( '0', buff, bufc );
        return;
    }
    VaChk_Only_In( 2 );

    sum = 0;
    cp = trim_space_sep( fargs[0], &isep );
    while( cp ) {
        curr = split_token( &cp, &isep );
        sum += strtod( curr, ( char **) NULL );
    }
    fval( buff, bufc, sum );
}

void fun_lmax( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL max, val;

    char *cp, *curr;

    Delim isep;

    VaChk_Only_In( 2 );

    cp = trim_space_sep( fargs[0], &isep );
    if( cp ) {
        curr = split_token( &cp, &isep );
        max = strtod( curr, ( char **) NULL );
        while( cp ) {
            curr = split_token( &cp, &isep );
            val = strtod( curr, ( char **) NULL );
            if( max < val ) {
                max = val;
            }
        }
        fval( buff, bufc, max );
    }
}

void fun_lmin( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    NVAL min, val;

    char *cp, *curr;

    Delim isep;

    VaChk_Only_In( 2 );

    cp = trim_space_sep( fargs[0], &isep );
    if( cp ) {
        curr = split_token( &cp, &isep );
        min = strtod( curr, ( char **) NULL );
        while( cp ) {
            curr = split_token( &cp, &isep );
            val = strtod( curr, ( char **) NULL );
            if( min > val ) {
                min = val;
            }
        }
        fval( buff, bufc, min );
    }
}

/*
 * ---------------------------------------------------------------------------
 * Operations on a single vector: VMAG, VUNIT (VDIM is implemented by
 * fun_words)
 */

void handle_vector( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    char **v1;

    int n, i, oper;

    NVAL tmp, res = 0;

    Delim isep, osep;

    oper = Func_Mask( VEC_OPER );

    if( oper == VEC_UNIT ) {
        VaChk_Only_In_Out( 3 );
    } else {
        VaChk_Only_In( 2 );
    }

    /*
     * split the list up, or return if the list is empty
     */
    if( !fargs[0] || !*fargs[0] ) {
        return;
    }
    n = list2arr( &v1, LBUF_SIZE, fargs[0], &isep );

    /*
     * calculate the magnitude
     */
    for( i = 0; i < n; i++ ) {
        tmp = strtod( v1[i], ( char **) NULL );
        res += tmp * tmp;
    }

    /*
     * if we're just calculating the magnitude, return it
     */
    if( oper == VEC_MAG ) {
        if( res > 0 ) {
            fval( buff, bufc, sqrt( res ) );
        } else {
            safe_chr( '0', buff, bufc );
        }
        XFREE( v1, "handle_vector.v1" );
        return;
    }
    if( res <= 0 ) {
        safe_str( "#-1 CAN'T MAKE UNIT VECTOR FROM ZERO-LENGTH VECTOR",
                  buff, bufc );
        XFREE( v1, "handle_vector.v1" );
        return;
    }
    res = sqrt( res );
    fval( buff, bufc,  strtod( v1[0], ( char **) NULL ) / res );
    for( i = 1; i < n; i++ ) {
        print_sep( &osep, buff, bufc );
        fval( buff, bufc, strtod( v1[i], ( char **) NULL ) / res );
    }
    XFREE( v1, "handle_vector.v1" );
}

/*
 * ---------------------------------------------------------------------------
 * Operations on a pair of vectors: VADD, VSUB, VMUL, VDOT, VOR, VAND, VXOR
 */

void handle_vectors( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;

    int oper;

    char **v1, **v2;

    NVAL scalar;

    int n, m, i, x, y;

    oper = Func_Mask( VEC_OPER );

    if( oper != VEC_DOT ) {
        VaChk_Only_In_Out( 4 );
    } else {
        /*
         * dot product returns a scalar, so no output delim
         */
        VaChk_Only_In( 3 );
    }

    /*
     * split the list up, or return if the list is empty
     */
    if( !fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1] ) {
        return;
    }
    n = list2arr( &v1, LBUF_SIZE, fargs[0], &isep );
    m = list2arr( &v2, LBUF_SIZE, fargs[1], &isep );

    /*
     * It's okay to have vmul() be passed a scalar first or second arg,
     * but everything else has to be same-dimensional.
     */
    if( ( n != m ) && !( ( oper == VEC_MUL ) && ( ( n == 1 ) || ( m == 1 ) ) ) ) {
        safe_str( "#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufc );
        XFREE( v1, "handle_vectors.v1" );
        XFREE( v2, "handle_vectors.v2" );
        return;
    }
    switch( oper ) {
    case VEC_ADD:
        fval( buff, bufc, strtod( v1[0], ( char **) NULL ) + strtod( v2[0], ( char **) NULL ) );
        for( i = 1; i < n; i++ ) {
            print_sep( &osep, buff, bufc );
            fval( buff, bufc, strtod( v1[0], ( char **) NULL ) + strtod( v2[0], ( char **) NULL ) );
        }
        break;
    case VEC_SUB:
        fval( buff, bufc, strtod( v1[0], ( char **) NULL ) - strtod( v2[0], ( char **) NULL ) );
        for( i = 1; i < n; i++ ) {
            print_sep( &osep, buff, bufc );
            fval( buff, bufc, strtod( v1[0], ( char **) NULL ) - strtod( v2[0], ( char **) NULL ) );
        }
        break;
    case VEC_OR:
        safe_bool( buff, bufc, xlate( v1[0] ) || xlate( v2[0] ) );
        for( i = 1; i < n; i++ ) {
            print_sep( &osep, buff, bufc );
            safe_bool( buff, bufc, xlate( v1[i] ) || xlate( v2[i] ) );
        }
        break;
    case VEC_AND:
        safe_bool( buff, bufc, xlate( v1[0] ) && xlate( v2[0] ) );
        for( i = 1; i < n; i++ ) {
            print_sep( &osep, buff, bufc );
            safe_bool( buff, bufc, xlate( v1[i] ) && xlate( v2[i] ) );
        }
        break;
    case VEC_XOR:
        x = xlate( v1[0] );
        y = xlate( v2[0] );
        safe_bool( buff, bufc, ( x && !y ) || ( !x && y ) );
        for( i = 1; i < n; i++ ) {
            print_sep( &osep, buff, bufc );
            x = xlate( v1[i] );
            y = xlate( v2[i] );
            safe_bool( buff, bufc, ( x && !y ) || ( !x && y ) );
        }
        break;
    case VEC_MUL:
        /*
         * if n or m is 1, this is scalar multiplication. otherwise,
         * multiply elementwise.
         */
        if( n == 1 ) {
            scalar = strtod( v1[0], ( char **) NULL );
            fval( buff, bufc, strtod( v2[0], ( char **) NULL ) * scalar );
            for( i = 1; i < m; i++ ) {
                print_sep( &osep, buff, bufc );
                fval( buff, bufc, strtod( v2[0], ( char **) NULL ) * scalar );
            }
        } else if( m == 1 ) {
            scalar = strtod( v2[0], ( char **) NULL );
            fval( buff, bufc, strtod( v1[0], ( char **) NULL ) * scalar );
            for( i = 1; i < n; i++ ) {
                print_sep( &osep, buff, bufc );
                fval( buff, bufc, strtod( v1[0], ( char **) NULL ) * scalar );
            }
        } else {
            /*
             * vector elementwise product.
             *
             * Note this is a departure from TinyMUX, but an
             * imitation of the PennMUSH behavior: the
             * documentation in Penn claims it's a dot product,
             * but the actual behavior isn't. We implement dot
             * product separately!
             */
            fval( buff, bufc, strtod( v1[0], ( char **) NULL ) * strtod( v2[0], ( char **) NULL ) );
            for( i = 1; i < n; i++ ) {
                print_sep( &osep, buff, bufc );
                fval( buff, bufc, strtod( v1[0], ( char **) NULL ) * strtod( v2[0], ( char **) NULL ) );
            }
        }
        break;
    case VEC_DOT:
        /*
         * dot product: (a,b,c) . (d,e,f) = ad + be + cf
         *
         * no cross product implementation yet: it would be (a,b,c) x
         * (d,e,f) = (bf - ce, cd - af, ae - bd)
         */
        scalar = 0;
        for( i = 0; i < n; i++ ) {
            scalar += strtod( v1[0], ( char **) NULL ) * strtod( v2[0], ( char **) NULL );
        }
        fval( buff, bufc, scalar );
        break;
    default:
        /*
         * If we reached this, we're in trouble.
         */
        safe_str( "#-1 UNIMPLEMENTED", buff, bufc );
        break;
    }
    XFREE( v1, "handle_vectors.v1" );
    XFREE( v2, "handle_vectors.v2" );
}

/*
 * ---------------------------------------------------------------------------
 * Simple boolean funcs: NOT, NOTBOOL, T
 */

void fun_not( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, !( int ) strtol( fargs[0], ( char **) NULL, 10 ) );
}

void fun_notbool( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, !xlate( fargs[0] ) );
}

void fun_t( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    safe_bool( buff, bufc, xlate( fargs[0] ) );
}

int cvtfun( int flag, char *str )
{
    if( flag & LOGIC_BOOL ) {
        return ( xlate( str ) );
    } else {
        return ( ( int ) strtol( str, ( char **) NULL, 10 ) );
    }
}

/*
 * ---------------------------------------------------------------------------
 * Multi-argument boolean funcs: various combinations of
 * [L,C][AND,OR,XOR][BOOL]
 */
void handle_logic( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep;

    int flag, oper, i, val;

    char *str, *tbuf, *bp;


    flag = Func_Flags( fargs );
    oper = ( flag & LOGIC_OPER );

    /*
     * most logic operations on an empty string should be false
     */
    val = 0;

    if( flag & LOGIC_LIST ) {

        if( nfargs == 0 ) {
            safe_chr( '0', buff, bufc );
            return;
        }
        /*
         * the arguments come in a pre-evaluated list
         */
        VaChk_Only_In( 2 );

        bp = trim_space_sep( fargs[0], &isep );
        while( bp ) {
            tbuf = split_token( &bp, &isep );
            val = ( ( oper == LOGIC_XOR )
                    && val ) ? !cvtfun( flag, tbuf ) : cvtfun( flag, tbuf );
            if( ( ( oper == LOGIC_AND ) && !val )
                    || ( ( oper == LOGIC_OR ) && val ) ) {
                break;
            }
        }

    } else if( nfargs < 2 ) {
        /*
         * separate arguments, but not enough of them
         */
        safe_known_str( "#-1 TOO FEW ARGUMENTS", 21, buff, bufc );
        return;

    } else if( flag & FN_NO_EVAL ) {
        /*
         * separate, unevaluated arguments
         */
        tbuf = alloc_lbuf( "handle_logic" );
        for( i = 0; i < nfargs; i++ ) {
            str = fargs[i];
            bp = tbuf;
            exec( tbuf, &bp, player, caller, cause,
                  EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs,
                  ncargs );
            val = ( ( oper == LOGIC_XOR )
                    && val ) ? !cvtfun( flag, tbuf ) : cvtfun( flag, tbuf );
            if( ( ( oper == LOGIC_AND ) && !val )
                    || ( ( oper == LOGIC_OR ) && val ) ) {
                break;
            }
        }
        free_lbuf( tbuf );

    } else {
        /*
         * separate, pre-evaluated arguments
         */
        for( i = 0; i < nfargs; i++ ) {
            val = ( ( oper == LOGIC_XOR )
                    && val ) ? !cvtfun( flag, fargs[i] ) : cvtfun( flag, fargs[i] );
            if( ( ( oper == LOGIC_AND ) && !val )
                    || ( ( oper == LOGIC_OR ) && val ) ) {
                break;
            }
        }
    }

    safe_bool( buff, bufc, val );
}

/*
 * ---------------------------------------------------------------------------
 * ltrue() and lfalse(): Get boolean values for an entire list.
 */

void handle_listbool( char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs )
{
    Delim isep, osep;

    int flag, n;

    char *tbuf, *bp, *bb_p;

    flag = Func_Flags( fargs );
    VaChk_Only_In_Out( 3 );

    if( !fargs[0] || !*fargs[0] ) {
        return;
    }

    bb_p = *bufc;
    bp = trim_space_sep( fargs[0], &isep );
    while( bp ) {
        tbuf = split_token( &bp, &isep );
        if( *bufc != bb_p ) {
            print_sep( &osep, buff, bufc );
        }
        if( flag & IFELSE_BOOL ) {
            n = xlate( tbuf );
        } else {
            n = !( ( int ) strtol( tbuf, ( char **) NULL, 10 ) == 0 ) && is_number( tbuf );
        }
        if( flag & IFELSE_FALSE ) {
            n = !n;
        }
        safe_bool( buff, bufc, n );
    }
}
