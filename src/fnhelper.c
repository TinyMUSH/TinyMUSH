/* fnhelper.c - helper functions for MUSH functions */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include <typedefs.h>
#include "game.h" /* required by mudconf */
#include "alloc.h" /* required by mudconf */
#include "flags.h" /* required by mudconf */
#include "htab.h" /* required by mudconf */
#include "ltdl.h" /* required by mudconf */
#include "udb.h" /* required by mudconf */
#include "udb_defs.h" /* required by mudconf */
#include "typedefs.h"           /* required by mudconf */
#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "externs.h"		/* required by code */

#include "functions.h"		/* required by code */
#include "match.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "ansi.h"		/* required by code */

static long genrand_int31(void);

/*
 * ---------------------------------------------------------------------------
 * Trim off leading and trailing spaces if the separator char is a space
 */

char *trim_space_sep(char *str, const Delim *sep) {
    char *p;

    if ((sep->len > 1) || (sep->str[0] != ' ') || !*str)
        return str;
    while (*str == ' ')
        str++;
    for (p = str; *p; p++);
    for (p--; *p == ' ' && p > str; p--);
    p++;
    *p = '\0';
    return str;
}

/*
 * ---------------------------------------------------------------------------
 * Tokenizer functions. next_token: Point at start of next token in string.
 * split_token: Get next token from string as null-term string.  String is
 * destructively modified. next_token_ansi: Point at start of next token, and
 * tell what color it is.
 */

char *next_token(char *str, const Delim *sep) {
    char *p;

    if (sep->len == 1) {
        while (*str == ESC_CHAR) {
            skip_esccode(str);
        }
        while (*str && (*str != sep->str[0])) {
            ++str;
            while (*str == ESC_CHAR) {
                skip_esccode(str);
            }
        }
        if (!*str)
            return NULL;
        ++str;
        if (sep->str[0] == ' ') {
            while (*str == ' ')
                ++str;
        }
    } else {
        if ((p = strstr(str, sep->str)) == NULL)
            return NULL;
        str = p + sep->len;
    }
    return str;
}

char *split_token(char **sp, const Delim *sep) {
    char *str, *save, *p;

    save = str = *sp;
    if (!str) {
        *sp = NULL;
        return NULL;
    }
    if (sep->len == 1) {
        while (*str == ESC_CHAR) {
            skip_esccode(str);
        }
        while (*str && (*str != sep->str[0])) {
            ++str;
            while (*str == ESC_CHAR) {
                skip_esccode(str);
            }
        }
        if (*str) {
            *str++ = '\0';
            if (sep->str[0] == ' ') {
                while (*str == ' ')
                    ++str;
            }
        } else {
            str = NULL;
        }
    } else {
        if ((p = strstr(str, sep->str)) != NULL) {
            *p = '\0';
            str = p + sep->len;
        } else {
            str = NULL;
        }
    }
    *sp = str;
    return save;
}

char *next_token_ansi(char *str, const Delim *sep, int *ansi_state_ptr) {
    int ansi_state = *ansi_state_ptr;

    char *p;

    if (sep->len == 1) {
        while (*str == ESC_CHAR) {
            track_esccode(str, ansi_state);
        }
        while (*str && (*str != sep->str[0])) {
            ++str;
            while (*str == ESC_CHAR) {
                track_esccode(str, ansi_state);
            }
        }
        if (!*str) {
            *ansi_state_ptr = ansi_state;
            return NULL;
        }
        ++str;
        if (sep->str[0] == ' ') {
            while (*str == ' ')
                ++str;
        }
    } else {
        /*
         * ansi tracking not supported yet in multichar delims
         */
        if ((p = strstr(str, sep->str)) == NULL)
            return NULL;
        str = p + sep->len;
    }
    *ansi_state_ptr = ansi_state;
    return str;
}

/*
 * ---------------------------------------------------------------------------
 * Count the words in a delimiter-separated list.
 */

int countwords(char *str, const Delim *sep) {
    int n;

    str = trim_space_sep(str, sep);
    if (!*str)
        return 0;
    for (n = 0; str; str = next_token(str, sep), n++);
    return n;
}

/*
 * ---------------------------------------------------------------------------
 * list2arr, arr2list: Convert lists to arrays and vice versa.
 */

int list2arr(char ***arr, int maxtok, char *list, const Delim *sep) {
    static unsigned char tok_starts[(LBUF_SIZE >> 3) + 1];

    static int initted = 0;

    char *tok, *liststart;

    int ntok, tokpos, i, bits;

    /*
     * Mark token starting points in a static 1k bitstring, then go back
     * and collect them into an array of just the right number of
     * pointers.
     */

    if (!initted) {
        memset(tok_starts, 0, sizeof(tok_starts));
        initted = 1;
    }
    liststart = list = trim_space_sep(list, sep);
    tok = split_token(&list, sep);
    for (ntok = 0, tokpos = 0; tok && ntok < maxtok;
            ++ntok, tok = split_token(&list, sep)) {
        tokpos = tok - liststart;
        tok_starts[tokpos >> 3] |= (1 << (tokpos & 0x7));
    }
    if (ntok == 0) {
        /*
         * So we don't try to malloc(0).
         */
        ++ntok;
    }
    /*
     * Caller must free this array of pointers later. Validity of the
     * pointers is dependent upon the original list string having not
     * been freed yet.
     */
    *arr = (char **)XCALLOC(ntok, sizeof(char *), "list2arr");

    tokpos >>= 3;
    ntok = 0;
    for (i = 0; i <= tokpos; ++i) {
        if (tok_starts[i]) {
            /*
             * There's at least one token starting in this byte
             * of the bitstring, so we scan the bits.
             */
            bits = tok_starts[i];
            tok_starts[i] = 0;
            tok = liststart + (i << 3);
            do {
                if (bits & 0x1) {
                    (*arr)[ntok] = tok;
                    ++ntok;
                }
                bits >>= 1;
                ++tok;
            } while (bits);
        }
    }
    return ntok;
}

void arr2list(char **arr, int alen, char *list, char **bufc, const Delim *sep) {
    int i;

    if (alen) {
        safe_str(arr[0], list, bufc);
    }
    for (i = 1; i < alen; i++) {
        print_sep(sep, list, bufc);
        safe_str(arr[i], list, bufc);
    }
}

/*
 * ---------------------------------------------------------------------------
 * Find the ANSI states at the beginning and end of each word of a list.
 * NOTE! Needs one more array slot than list2arr (think fenceposts) but still
 * takes the same maxlen and returns the same number of words.
 */

int list2ansi(int *arr, int *prior_state, int maxlen, char *list, const Delim *sep) {
    int i, ansi_state;

    if (maxlen <= 0)
        return 0;

    ansi_state = arr[0] = *prior_state;
    list = trim_space_sep(list, sep);

    for (i = 1; list && i < maxlen;
            list = next_token_ansi(list, sep, &ansi_state), ++i) {
        arr[i - 1] = ansi_state;
    }
    arr[i] = ANST_NONE;
    return i - 1;
}

/*
 * ---------------------------------------------------------------------------
 * Quick-matching for function purposes.
 */

dbref match_thing(dbref player, char *name) {
    init_match(player, name, NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    return (noisy_match_result());
}

/*
 * ---------------------------------------------------------------------------
 * fn_range_check: Check # of args to a function with an optional argument
 * for validity.
 */

int fn_range_check(const char *fname, int nfargs, int minargs, int maxargs, char *result, char **bufc) {
    if ((nfargs >= minargs) && (nfargs <= maxargs))
        return 1;

    if (maxargs == (minargs + 1))
        safe_tmprintf_str(result, bufc,
                         "#-1 FUNCTION (%s) EXPECTS %d OR %d ARGUMENTS BUT GOT %d",
                         fname, minargs, maxargs, nfargs);
    else
        safe_tmprintf_str(result, bufc,
                         "#-1 FUNCTION (%s) EXPECTS BETWEEN %d AND %d ARGUMENTS BUT GOT %d",
                         fname, minargs, maxargs, nfargs);
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * delim_check: obtain delimiter
 */

int delim_check(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs, int sep_arg, Delim *sep, int dflags) {
    char *tstr, *bp, *str;

    int tlen;

    if (nfargs < sep_arg) {
        sep->str[0] = ' ';
        sep->len = 1;
        return 1;
    }
    tlen = strlen(fargs[sep_arg - 1]);
    if (tlen <= 1)
        dflags &= ~DELIM_EVAL;

    if (dflags & DELIM_EVAL) {
        tstr = bp = alloc_lbuf("delim_check");
        str = fargs[sep_arg - 1];
        exec(tstr, &bp, player, caller, cause,
             EV_EVAL | EV_FCHECK, &str, cargs, ncargs);
        tlen = strlen(tstr);
    } else {
        tstr = fargs[sep_arg - 1];
    }

    sep->len = 1;
    if (tlen == 0) {
        sep->str[0] = ' ';
    } else if (tlen == 1) {
        sep->str[0] = *tstr;
    } else {
        if ((dflags & DELIM_NULL) &&
                !strcmp(tstr, (char *)NULL_DELIM_VAR)) {
            sep->str[0] = '\0';
        } else if ((dflags & DELIM_CRLF) &&
                   !strcmp(tstr, (char *)"\r\n")) {
            sep->str[0] = '\r';
        } else if (dflags & DELIM_STRING) {
            if (tlen > MAX_DELIM_LEN) {
                safe_str("#-1 SEPARATOR TOO LONG", buff, bufc);
                sep->len = 0;
            } else {
                strcpy(sep->str, tstr);
                sep->len = tlen;
            }
        } else {
            safe_str("#-1 SEPARATOR MUST BE ONE CHARACTER",
                     buff, bufc);
            sep->len = 0;
        }
    }

    if (dflags & DELIM_EVAL)
        free_lbuf(tstr);

    return (sep->len);
}

/*
 * ---------------------------------------------------------------------------
 * Boolean true/false check.
 */

int xlate(char *arg) {
    char *temp2;

    if (arg[0] == '#') {
        arg++;
        if (is_integer(arg)) {
            if (mudconf.bools_oldstyle) {
                switch ((int)strtol(arg, (char **)NULL, 10)) {
                case -1:
                    return 0;
                case 0:
                    return 0;
                default:
                    return 1;
                }
            } else {
                return ((int)strtol(arg, (char **)NULL, 10) >= 0);
            }
        }
        if (mudconf.bools_oldstyle) {
            return 0;
        } else {
            /*
             * Case of '#-1 <string>'
             */
            return !((arg[0] == '-') && (arg[1] == '1') &&
                     (arg[2] == ' '));
        }
    }
    temp2 = Eat_Spaces(arg);
    if (!*temp2)
        return 0;
    if (is_integer(temp2))
        return (int)strtol(temp2, (char **)NULL, 10);
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * used by fun_reverse and fun_revwords to reverse things
 */

void do_reverse(char *from, char *to) {
    char *tp;

    tp = to + strlen(from);
    *tp-- = '\0';
    while (*from) {
        *tp-- = *from++;
    }
}

/*
 * ---------------------------------------------------------------------------
 * Random number generator, from PennMUSH's get_random_long(), which is turn
 * based on MUX2's RandomINT32().
 */

long random_range(long low, long high) {
    unsigned long x, n, n_limit;

    /*
     * Validate parameters.
     */

    if (high < low)
        return 0;
    else if (high == low)
        return low;

    x = high - low;
    if (INT_MAX < x)
        return -1;
    x++;

    /*
     * Look for a random number on the interval [0,x-1]. MUX2's
     * implementation states: In order to be perfectly conservative about
     * not introducing any further sources of statistical bias, we're
     * going to call getrand() until we get a number less than the
     * greatest representable multiple of x. We'll then return n mod x.
     * N.B. This loop happens in randomized constant time, and pretty
     * damn fast randomized constant time too, since P(UINT32_MAX_VALUE -
     * n < UINT32_MAX_VALUE % x) < 0.5, for any x. So even for the least
     * desirable x, the average number of times we will call getrand() is
     * less than 2.
     */

    n_limit = INT_MAX - (INT_MAX % x);
    do {
        n = genrand_int31();
    } while (n >= n_limit);

    return low + (n % x);
}

/*
 * ---------------------------------------------------------------------------
 * ALL OTHER CODE FOR TINYMUSH SHOULD GO ABOVE THIS LINE.
 * ---------------------------------------------------------------------------
 * */

/*
 * A C-program for MT19937, with initialization improved 2002/2/10. Coded by
 * Takuji Nishimura and Makoto Matsumoto. This is a faster version by taking
 * Shawn Cokus's optimization, Matthe Bellew's simplification, Isaku Wada's
 * real version.
 *
 * Before using, initialize the state by using init_genrand(seed) or
 * init_by_array(init_key, key_length).
 *
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura, All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The names of its contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Any feedback is very welcome. http://www.math.keio.ac.jp/matumoto/emt.html
 * email: matumoto@math.keio.ac.jp
 */

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL	/* constant vector a */
#define UMASK 0x80000000UL	/* most significant w-r bits */
#define LMASK 0x7fffffffUL	/* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

static unsigned long state[N];	/* the array for the state vector  */

static int left = 1;

static int initf = 0;

static unsigned long *next;

/* initializes state[N] with a seed */
void init_genrand(unsigned long s) {
    int j;

    state[0] = s & 0xffffffffUL;
    for (j = 1; j < N; j++) {
        state[j] =
            (1812433253UL * (state[j - 1] ^ (state[j - 1] >> 30)) + j);
        /*
         * See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
         */
        /*
         * In the previous versions, MSBs of the seed affect
         */
        /*
         * only MSBs of the array state[].
         */
        /*
         * 2002/01/09 modified by Makoto Matsumoto
         */
        state[j] &= 0xffffffffUL;	/* for >32 bit machines */
    }
    left = 1;
    initf = 1;
}

static void next_state(void) {
    unsigned long *p = state;

    int j;

    /*
     * if init_genrand() has not been called,
     */
    /*
     * a default initial seed is used
     */
    if (initf == 0)
        init_genrand(5489UL);

    left = N;
    next = state;

    for (j = N - M + 1; --j; p++)
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j = M; --j; p++)
        *p = p[M - N] ^ TWIST(p[0], p[1]);

    *p = p[M - N] ^ TWIST(p[0], state[0]);
}

/* generates a random number on [0,0x7fffffff]-interval */
static long genrand_int31(void) {
    unsigned long y;

    if (--left == 0)
        next_state();
    y = *next++;

    /*
     * Tempering
     */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (long)(y >> 1);
}
