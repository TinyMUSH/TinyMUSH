/**
 * @file funvars_regex.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Regular expression and grep functions
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <ctype.h>
#include <string.h>
#include <pcre2.h>

void perform_regedit(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    pcre2_code *re;
    pcre2_match_data *match_data;
    int case_option, all_option;
    int errorcode;
    PCRE2_SIZE erroroffset;
    int subpatterns, len;
    PCRE2_SIZE *ovector;
    char *r, *start, *tbuf;
    char tmp;
    int match_offset = 0;
    case_option = Func_Mask(REG_CASELESS);
    all_option = Func_Mask(REG_MATCH_ALL);

    re = pcre2_compile((PCRE2_SPTR)fargs[1], PCRE2_ZERO_TERMINATED, case_option, &errorcode, &erroroffset, NULL);
    if (re == NULL)
    {
        /*
         * Matching error. Note that this returns a null string
         * rather than '#-1 REGEXP ERROR: <error>', as PennMUSH does,
         * in order to remain consistent with our other regexp
         * functions.
         */
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        notify_quiet(player, (char *)buffer);
        return;
    }

    /*
     * JIT compile for optimization
     */
    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);

    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    len = strlen(fargs[0]);
    start = fargs[0];
    subpatterns = pcre2_match(re, (PCRE2_SPTR)fargs[0], len, 0, 0, match_data, NULL);

    /*
     * If there's no match, just return the original.
     */

    if (subpatterns < 0)
    {
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);

        XSAFELBSTR(fargs[0], buff, bufc);
        return;
    }

    ovector = pcre2_get_ovector_pointer(match_data);

    do
    {
        /*
         * If we had too many subpatterns for the offsets vector, set
         * the number to 1/3rd of the size of the offsets vector.
         */
        if (subpatterns == 0)
        {
            subpatterns = PCRE_MAX_OFFSETS / 3;
        }

        /*
         * Copy up to the start of the matched area.
         */
        tmp = fargs[0][ovector[0]];
        fargs[0][ovector[0]] = '\0';
        XSAFELBSTR(start, buff, bufc);
        fargs[0][ovector[0]] = tmp;

        /*
         * Copy in the replacement, putting in captured
         * sub-expressions.
         */

        for (r = fargs[2]; *r; r++)
        {
            int offset, have_brace = 0;
            char *endsub;

            if (*r != '$')
            {
                XSAFELBCHR(*r, buff, bufc);
                continue;
            }

            r++;

            if (*r == '{')
            {
                have_brace = 1;
                r++;
            }

            offset = strtoul(r, &endsub, 10);

            if (r == endsub || (have_brace && *endsub != '}'))
            {
                /*
                 * Not a valid number.
                 */
                XSAFELBCHR('$', buff, bufc);

                if (have_brace)
                {
                    XSAFELBCHR('{', buff, bufc);
                }

                r--;
                continue;
            }

            r = endsub - 1;

            if (have_brace)
            {
                r++;
            }

            tbuf = XMALLOC(LBUF_SIZE, "tbuf");
            PCRE2_SIZE tbuflen = LBUF_SIZE;
            if (pcre2_substring_copy_bynumber(match_data, offset, (PCRE2_UCHAR *)tbuf, &tbuflen) >= 0)
            {
                XSAFELBSTR(tbuf, buff, bufc);
            }
            XFREE(tbuf);
        }

        start = fargs[0] + ovector[1];
        match_offset = ovector[1];
    } while (all_option && (((ovector[0] == ovector[1]) &&
                             /*
                              * PCRE docs note: Perl special-cases the empty-string match in split
                              * and /g. To emulate, first try the match again at the same position
                              * with PCRE_NOTEMPTY, then advance the starting offset if that
                              * fails.
                              */
                             (((subpatterns = pcre2_match(re, (PCRE2_SPTR)fargs[0], len, match_offset, PCRE2_NOTEMPTY, match_data, NULL)) >= 0) || ((match_offset++ < len) && (subpatterns = pcre2_match(re, (PCRE2_SPTR)fargs[0], len, match_offset, 0, match_data, NULL)) >= 0))) ||
                            ((match_offset <= len) && (subpatterns = pcre2_match(re, (PCRE2_SPTR)fargs[0], len, match_offset, 0, match_data, NULL)) >= 0)));

    /*
     * Copy everything after the matched bit.
     */
    XSAFELBSTR(start, buff, bufc);
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
}

/*
 * --------------------------------------------------------------------------
 * wildparse: Set the results of a wildcard match into named variables.
 * wildparse(<string>,<pattern>,<list of variable names>)
 */

void fun_wildparse(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, nqregs;
    char *t_args[NUM_ENV_VARS], **qregs;

    if (!wild(fargs[1], fargs[0], t_args, NUM_ENV_VARS))
    {
        return;
    }

    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);

    for (i = 0; i < nqregs; i++)
    {
        if (qregs[i] && *qregs[i])
        {
            set_xvar(player, qregs[i], t_args[i]);
        }
    }

    /*
     * Need to free up allocated memory from the match.
     */

    for (i = 0; i < NUM_ENV_VARS; i++)
    {
        if (t_args[i])
        {
            XFREE(t_args[i]);
        }
    }

    XFREE(qregs);
}

/*
 * ---------------------------------------------------------------------------
 * perform_regparse: Slurp a string into up to ten named variables ($0 - $9).
 * REGPARSE, REGPARSEI. Unlike regmatch(), this returns no value.
 * regparse(string, pattern, named vars)
 */

void perform_regparse(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, nqregs;
    int case_option;
    char **qregs;
    char *matchbuf = XMALLOC(LBUF_SIZE, "matchbuf");
    pcre2_code *re;
    pcre2_match_data *match_data;
    int errorcode;
    PCRE2_SIZE erroroffset;
    PCRE2_SIZE *ovector;
    int subpatterns;
    case_option = Func_Mask(REG_CASELESS);

    re = pcre2_compile((PCRE2_SPTR)fargs[1], PCRE2_ZERO_TERMINATED, case_option, &errorcode, &erroroffset, NULL);
    if (re == NULL)
    {
        /*
         * Matching error.
         */
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        notify_quiet(player, (char *)buffer);
        XFREE(matchbuf);
        return;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    subpatterns = pcre2_match(re, (PCRE2_SPTR)fargs[0], strlen(fargs[0]), 0, 0, match_data, NULL);

    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if (subpatterns == 0)
    {
        subpatterns = PCRE_MAX_OFFSETS / 3;
    }

    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);
    ovector = pcre2_get_ovector_pointer(match_data);

    for (i = 0; i < nqregs; i++)
    {
        if (qregs[i] && *qregs[i])
        {
            PCRE2_SIZE matchbuflen = LBUF_SIZE;
            if (pcre2_substring_copy_bynumber(match_data, i, (PCRE2_UCHAR *)matchbuf, &matchbuflen) < 0)
            {
                set_xvar(player, qregs[i], NULL);
            }
            else
            {
                set_xvar(player, qregs[i], matchbuf);
            }
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    XFREE(qregs);
    XFREE(matchbuf);
}

/*
 * ---------------------------------------------------------------------------
 * perform_regrab: Like grab() and graball(), but with a regexp pattern.
 * REGRAB, REGRABI. Derived from PennMUSH.
 */

void perform_regrab(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    int case_option, all_option;
    char *r, *s, *bb_p;
    pcre2_code *re;
    pcre2_match_data *match_data;
    int errorcode;
    PCRE2_SIZE erroroffset;
    case_option = Func_Mask(REG_CASELESS);
    all_option = Func_Mask(REG_MATCH_ALL);

    if (all_option)
    {
        if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 4, buff, bufc))
        {
            return;
        }

        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
        {
            return;
        }

        if (nfargs < 4)
        {
            XMEMCPY((&osep), (&isep), sizeof(Delim) - MAX_DELIM_LEN + 1 + (&isep)->len);
        }
        else
        {
            if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
            {
                return;
            }
        }
    }
    else
    {
        if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
        {
            return;
        }

        if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 3, &isep, DELIM_STRING))
        {
            return;
        }
    }

    s = trim_space_sep(fargs[0], &isep);
    bb_p = *bufc;

    re = pcre2_compile((PCRE2_SPTR)fargs[1], PCRE2_ZERO_TERMINATED, case_option, &errorcode, &erroroffset, NULL);
    if (re == NULL)
    {
        /*
         * Matching error. Note difference from PennMUSH behavior:
         * Regular expression errors return 0, not #-1 with an error
         * message.
         */
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        notify_quiet(player, (char *)buffer);
        return;
    }

    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    do
    {
        r = split_token(&s, &isep);

        if (pcre2_match(re, (PCRE2_SPTR)r, strlen(r), 0, 0, match_data, NULL) >= 0)
        {
            if (*bufc != bb_p)
            {
                /*
                 * if true, all_option also
                 * * true
                 */
                print_separator(&osep, buff, bufc);
            }

            XSAFELBSTR(r, buff, bufc);

            if (!all_option)
            {
                break;
            }
        }
    } while (s);

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
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

void perform_regmatch(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int case_option;
    int i, nqregs;
    char **qregs;
    pcre2_code *re;
    pcre2_match_data *match_data;
    int errorcode;
    PCRE2_SIZE erroroffset;
    int subpatterns;
    char tbuf[LBUF_SIZE];
    case_option = Func_Mask(REG_CASELESS);

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 2, 3, buff, bufc))
    {
        return;
    }

    re = pcre2_compile((PCRE2_SPTR)fargs[1], PCRE2_ZERO_TERMINATED, case_option, &errorcode, &erroroffset, NULL);
    if (re == NULL)
    {
        /*
         * Matching error. Note difference from PennMUSH behavior:
         * Regular expression errors return 0, not #-1 with an error
         * message.
         */
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        notify_quiet(player, (char *)buffer);
        XSAFELBCHR('0', buff, bufc);
        return;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    subpatterns = pcre2_match(re, (PCRE2_SPTR)fargs[0], strlen(fargs[0]), 0, 0, match_data, NULL);
    XSAFEBOOL(buff, bufc, (subpatterns >= 0));

    /*
     * If we had too many subpatterns for the offsets vector, set the
     * number to 1/3rd of the size of the offsets vector.
     */
    if (subpatterns == 0)
    {
        subpatterns = PCRE_MAX_OFFSETS / 3;
    }

    /*
     * If we don't have a third argument, we're done.
     */
    if (nfargs != 3)
    {
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return;
    }

    /*
     * We need to parse the list of registers. Anything that we don't get
     * is assumed to be -1. If we didn't match, or the match went wonky,
     * then set the register to empty. Otherwise, fill the register with
     * the subexpression.
     */
    nqregs = list2arr(&qregs, NUM_ENV_VARS, fargs[2], &SPACE_DELIM);

    for (i = 0; i < nqregs; i++)
    {
        PCRE2_SIZE tbuflen = LBUF_SIZE;
        if (pcre2_substring_copy_bynumber(match_data, i, (PCRE2_UCHAR *)tbuf, &tbuflen) < 0)
        {
            set_register("perform_regmatch", qregs[i], NULL);
        }
        else
        {
            set_register("perform_regmatch", qregs[i], tbuf);
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    XFREE(qregs);
}

/*
 * ---------------------------------------------------------------------------
 * fun_until: Much like while(), but operates on multiple lists ala mix().
 * until(eval_fn,cond_fn,list1,list2,compare_str,delim,output delim) The
 * delimiter terminators are MANDATORY. The termination condition is a REGEXP
 * match (thus allowing this to be also used as 'eval until a termination
 * condition is NOT met').
 */

void fun_until(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    Delim isep, osep;
    dbref aowner1 = NOTHING, thing1 = NOTHING, aowner2 = NOTHING, thing2 = NOTHING;
    int aflags1 = 0, aflags2 = 0, anum1 = 0, anum2 = 0, alen1 = 0, alen2 = 0;
    ATTR *ap = NULL, *ap2 = NULL;
    char *atext1 = NULL, *atext2 = NULL, *atextbuf = NULL, *condbuf = NULL;
    char *cp[NUM_ENV_VARS], *os[NUM_ENV_VARS];
    int count[LBUF_SIZE / 2];
    int i = 0, is_exact_same = 0, is_same = 0, nwords = 0, lastn = 0, wc = 0;
    char *str = NULL, *dp = NULL, *savep = NULL, *bb_p = NULL;
    pcre2_code *re = NULL;
    pcre2_match_data *match_data = NULL;
    int errorcode;
    PCRE2_SIZE erroroffset;
    int subpatterns = 0;
    /*
     * We need at least 6 arguments. The last 2 args must be delimiters.
     */
    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 6, 12, buff, bufc))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, nfargs - 1, &isep, DELIM_STRING))
    {
        return;
    }

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, nfargs, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    lastn = nfargs - 4;

    /*
     * Make sure we have a valid regular expression.
     */

    re = pcre2_compile((PCRE2_SPTR)fargs[lastn + 1], PCRE2_ZERO_TERMINATED, 0, &errorcode, &erroroffset, NULL);
    if (re == NULL)
    {
        /*
         * Return nothing on a bad match.
         */
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        notify_quiet(player, (char *)buffer);
        return;
    }

    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    /*
     * Our first and second args can be <obj>/<attr> or just <attr>. Use
     * them if we can access them, otherwise return an empty string.
     *
     * Note that for user-defined attributes, atr_str() returns a pointer to
     * a static, and that therefore we have to be careful about what
     * we're doing.
     */
    if (parse_attrib(player, fargs[0], &thing1, &anum1, 0))
    {
        if ((anum1 == NOTHING) || !(Good_obj(thing1)))
            ap = NULL;
        else
            ap = atr_num(anum1);
    }
    else
    {
        thing1 = player;
        ap = atr_str(fargs[0]);
    }

    if (!ap)
    {
        return;
    }
    atext1 = atr_pget(thing1, ap->number, &aowner1, &aflags1, &alen1);
    if (!*atext1 || !(See_attr(player, thing1, ap, aowner1, aflags1)))
    {
        XFREE(atext1);
        return;
    }

    if (parse_attrib(player, fargs[1], &thing2, &anum2, 0))
    {
        if ((anum2 == NOTHING) || !(Good_obj(thing2)))
            ap2 = NULL;
        else
            ap2 = atr_num(anum2);
    }
    else
    {
        thing2 = player;
        ap2 = atr_str(fargs[1]);
    }

    if (!ap2)
    {
        XFREE(atext1); /* we allocated this, remember? */
        return;
    }

    /*
     * If our evaluation and condition are the same, we can save
     * ourselves some time later. There are two possibilities: we have
     * the exact same obj/attr pair, or the attributes contain identical
     * text.
     */

    if ((thing1 == thing2) && (ap->number == ap2->number))
    {
        is_same = 1;
        is_exact_same = 1;
    }
    else
    {
        is_exact_same = 0;
        atext2 = atr_pget(thing2, ap2->number, &aowner2, &aflags2, &alen2);

        if (!*atext2 || !See_attr(player, thing2, ap2, aowner2, aflags2))
        {
            XFREE(atext1);
            XFREE(atext2);
            return;
        }

        if (!strcmp(atext1, atext2))
        {
            is_same = 1;
        }
        else
        {
            is_same = 0;
        }
    }

    atextbuf = XMALLOC(LBUF_SIZE, "atextbuf");

    if (!is_same)
    {
        condbuf = XMALLOC(LBUF_SIZE, "condbuf");
    }

    bb_p = *bufc;

    /*
     * Process the list one element at a time. We need to find out what
     * the longest list is; assume null-padding for shorter lists.
     */

    for (i = 0; i < NUM_ENV_VARS; i++)
    {
        cp[i] = NULL;
    }

    cp[2] = trim_space_sep(fargs[2], &isep);
    nwords = count[2] = countwords(cp[2], &isep);

    for (i = 3; i <= lastn; i++)
    {
        cp[i] = trim_space_sep(fargs[i], &isep);
        count[i] = countwords(cp[i], &isep);

        if (count[i] > nwords)
        {
            nwords = count[i];
        }
    }

    for (wc = 0; (wc < nwords) && (mushstate.func_invk_ctr < mushconf.func_invk_lim) && !Too_Much_CPU(); wc++)
    {
        for (i = 2; i <= lastn; i++)
        {
            if (count[i])
            {
                os[i - 2] = split_token(&cp[i], &isep);
            }
            else
            {
                os[i - 2] = 0;
            }
        }

        if (*bufc != bb_p)
        {
            print_separator(&osep, buff, bufc);
        }

        XMEMCPY(atextbuf, atext1, alen1);
        atextbuf[alen1] = 0;
        str = atextbuf;
        savep = *bufc;
        eval_expression_string(buff, bufc, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(os[0]), lastn - 1);

        if (!is_same)
        {
            XMEMCPY(atextbuf, atext2, alen2);
            atextbuf[alen2] = '\0';
            dp = savep = condbuf;
            str = atextbuf;
            eval_expression_string(condbuf, &dp, player, caller, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str, &(os[0]), lastn - 1);
        }

        subpatterns = pcre2_match(re, (PCRE2_SPTR)savep, strlen(savep), 0, 0, match_data, NULL);

        if (subpatterns >= 0)
        {
            break;
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    XFREE(atext1);

    if (!is_exact_same)
    {
        XFREE(atext2);
    }

    XFREE(atextbuf);

    if (!is_same)
    {
        XFREE(condbuf);
    }
}

/*
 * ---------------------------------------------------------------------------
 * perform_grep: grep (exact match), wildgrep (wildcard match), regrep
 * (regexp match), and case-insensitive versions. (There is no
 * case-insensitive wildgrep, since all wildcard matches are caseless.)
 */

void perform_grep(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int grep_type, caseless;
    pcre2_code *re = NULL;
    pcre2_match_data *match_data = NULL;
    int errorcode;
    PCRE2_SIZE erroroffset;
    char *patbuf, *patc, *attrib, *p, *bb_p;
    int ca, aflags, alen;
    dbref thing, aowner, it;
    Delim osep;

    if (!fn_range_check(((FUN *)fargs[-1])->name, nfargs, 3, 4, buff, bufc))
    {
        return;
    };

    if (!delim_check(buff, bufc, player, caller, cause, fargs, nfargs, cargs, ncargs, 4, &osep, DELIM_STRING | DELIM_NULL | DELIM_CRLF))
    {
        return;
    }

    grep_type = Func_Mask(REG_TYPE);
    caseless = Func_Mask(REG_CASELESS);
    it = match_thing(player, fargs[0]);

    if (!Good_obj(it))
    {
        XSAFENOMATCH(buff, bufc);
        return;
    }
    else if (!(Examinable(player, it)))
    {
        XSAFENOPERM(buff, bufc);
        return;
    }

    /*
     * Make sure there's an attribute and a pattern
     */

    if (!fargs[1] || !*fargs[1])
    {
        XSAFELBSTR("#-1 NO SUCH ATTRIBUTE", buff, bufc);
        return;
    }

    if (!fargs[2] || !*fargs[2])
    {
        XSAFELBSTR("#-1 INVALID GREP PATTERN", buff, bufc);
        return;
    }

    switch (grep_type)
    {
    case GREP_EXACT:
        if (caseless)
        {
            for (p = fargs[2]; *p; p++)
            {
                *p = tolower(*p);
            }
        }

        break;

    case GREP_REGEXP:
        re = pcre2_compile((PCRE2_SPTR)fargs[2], PCRE2_ZERO_TERMINATED, caseless, &errorcode, &erroroffset, NULL);
        if (re == NULL)
        {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
            notify_quiet(player, (char *)buffer);
            return;
        }

        pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
        match_data = pcre2_match_data_create_from_pattern(re, NULL);

        break;

    default:
        /*
         * No special set-up steps.
         */
        break;
    }

    bb_p = *bufc;
    patc = patbuf = XMALLOC(LBUF_SIZE, "patbuf");
    XSAFESPRINTF(patbuf, &patc, "#%d/%s", it, fargs[1]);
    olist_push();

    if (parse_attrib_wild(player, patbuf, &thing, 0, 0, 1, 1))
    {
        for (ca = olist_first(); ca != NOTHING; ca = olist_next())
        {
            attrib = atr_get(thing, ca, &aowner, &aflags, &alen);

            if ((grep_type == GREP_EXACT) && caseless)
            {
                for (p = attrib; *p; p++)
                {
                    *p = tolower(*p);
                }
            }

            if (((grep_type == GREP_EXACT) && strstr(attrib, fargs[2])) || ((grep_type == GREP_WILD) && quick_wild(fargs[2], attrib)) || ((grep_type == GREP_REGEXP) && (pcre2_match(re, (PCRE2_SPTR)attrib, alen, 0, 0, match_data, NULL) >= 0)))
            {
                if (*bufc != bb_p)
                {
                    print_separator(&osep, buff, bufc);
                }

                XSAFELBSTR((char *)(atr_num(ca))->name, buff, bufc);
            }

            XFREE(attrib);
        }
    }

    XFREE(patbuf);
    olist_pop();

    if (re)
    {
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
    }
}

/*
 * ---------------------------------------------------------------------------
 * Grids.
 * gridmake(<rows>, <columns>[,<grid text>][,<col odelim>][,<row odelim>])
 * gridload(<grid text>[,<odelim for row elems>][,<odelim between rows>])
 * gridset(<y range>,<x range>,<value>[,<input sep for ranges>])
 * gridsize() - returns rows cols grid( , [,<odelim for row elems>][,<odelim
 * between rows>]) - whole grid grid(<y>,<x>) - show particular coordinate
 * grid(<y range>,<x range>[,<odelim for row elems>][,<odelim between rows>])
 */

