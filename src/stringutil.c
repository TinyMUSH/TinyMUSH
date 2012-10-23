/* stringutil.c - string utilities */

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

#include "ansi.h"		/* required by code */

/* ---------------------------------------------------------------------------
 * ANSI character-to-number translation table.
 */

int ansi_nchartab[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, I_ANSI_BBLUE, I_ANSI_BCYAN,
    0, 0, 0, I_ANSI_BGREEN,
    0, 0, 0, 0,
    0, I_ANSI_BMAGENTA, 0, 0,
    0, 0, I_ANSI_BRED, 0,
    0, 0, 0, I_ANSI_BWHITE,
    I_ANSI_BBLACK, I_ANSI_BYELLOW, 0, 0,
    0, 0, 0, 0,
    0, 0, I_ANSI_BLUE, I_ANSI_CYAN,
    0, 0, I_ANSI_BLINK, I_ANSI_GREEN,
    I_ANSI_HILITE, I_ANSI_INVERSE, 0, 0,
    0, I_ANSI_MAGENTA, I_ANSI_NORMAL, 0,
    0, 0, I_ANSI_RED, 0,
    0, I_ANSI_UNDER, 0, I_ANSI_WHITE,
    I_ANSI_BLACK, I_ANSI_YELLOW, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* ---------------------------------------------------------------------------
 * ANSI number-to-character translation table.
 */

char ansi_lettab[I_ANSI_NUM] = {
    '\0', 'h', '\0', '\0', 'u', 'f', '\0', 'i',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', 'x', 'r',
    'g', 'y', 'b', 'm', 'c', 'w', '\0', '\0',
    'X', 'R', 'G', 'Y', 'B', 'M', 'C', 'W'
};

/* ---------------------------------------------------------------------------
 * ANSI packed state definitions -- number-to-bitmask translation table.
 *
 * The mask specifies the state bits that are altered by a particular ansi
 * code. Bits are laid out as follows:
 *
 *  0x1000 -- No ansi. Every valid ansi code clears this bit.
 *  0x0800 -- inverse
 *  0x0400 -- flash
 *  0x0200 -- underline
 *  0x0100 -- highlight
 *  0x0080 -- "use default bg", set by ansi normal, cleared by other bg's
 *  0x0070 -- three bits of bg color
 *  0x0008 -- "use default fg", set by ansi normal, cleared by other fg's
 *  0x0007 -- three bits of fg color
 */

int ansi_mask_bits[I_ANSI_LIM] = {
    0x1fff, 0x1100, 0x1100, 0, 0x1200, 0x1400, 0, 0x1800, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0x1100, 0x1100, 0, 0x1200, 0x1400, 0, 0x1800, 0, 0,
    0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0, 0,
    0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0, 0
};

/* ---------------------------------------------------------------------------
 * ANSI packed state definitions -- number-to-bitvalue translation table.
 */

int ansi_bits[I_ANSI_LIM] = {
    0x0099, 0x0100, 0x0000, 0, 0x0200, 0x0400, 0, 0x0800, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0x0000, 0x0000, 0, 0x0000, 0x0000, 0, 0x0000, 0, 0,
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0, 0,
    0x0000, 0x0010, 0x0020, 0x0030, 0x0040, 0x0050, 0x0060, 0x0070, 0, 0
};

/* ---------------------------------------------------------------------------
 * strip_ansi -- return a new string with escape codes removed
 */

char *strip_ansi ( const char *s ) {
    static char buf[LBUF_SIZE];

    char *p = buf;

    if ( s ) {
        while ( *s == ESC_CHAR ) {
            skip_esccode ( s );
        }

        while ( *s ) {
            *p++ = *s++;
            while ( *s == ESC_CHAR ) {
                skip_esccode ( s );
            }
        }
    }

    *p = '\0';
    return buf;
}

/* ---------------------------------------------------------------------------
 * strip_ansi_len -- count non-escape-code characters
 */

int strip_ansi_len ( const char *s ) {
    int n = 0;

    if ( s ) {
        while ( *s == ESC_CHAR ) {
            skip_esccode ( s );
        }

        while ( *s ) {
            ++s, ++n;
            while ( *s == ESC_CHAR ) {
                skip_esccode ( s );
            }
        }
    }

    return n;
}

/* normal_to_white -- This function implements the NOBLEED flag */

char *normal_to_white ( const char *raw ) {
    static char buf[LBUF_SIZE];

    char *p = ( char * ) raw;

    char *q = buf;

    char *just_after_csi;

    char *just_after_esccode = p;

    unsigned int param_val;

    int has_zero;

    while ( p && *p ) {
        if ( *p == ESC_CHAR ) {
            safe_known_str ( just_after_esccode,
                             p - just_after_esccode, buf, &q );
            if ( p[1] == ANSI_CSI ) {
                safe_chr ( *p, buf, &q );
                ++p;
                safe_chr ( *p, buf, &q );
                ++p;
                just_after_csi = p;
                has_zero = 0;
                while ( ( *p & 0xf0 ) == 0x30 ) {
                    if ( *p == '0' ) {
                        has_zero = 1;
                    }
                    ++p;
                }
                while ( ( *p & 0xf0 ) == 0x20 ) {
                    ++p;
                }
                if ( *p == ANSI_END && has_zero ) {
                    /*
                     * it really was an ansi code,
                     * * go back and fix up the zero
                     */
                    p = just_after_csi;
                    param_val = 0;
                    while ( ( *p & 0xf0 ) == 0x30 ) {
                        if ( *p < 0x3a ) {
                            param_val <<= 1;
                            param_val +=
                                ( param_val << 2 ) +
                                ( *p & 0x0f );
                            safe_chr ( *p, buf, &q );
                        } else {
                            if ( param_val == 0 ) {
                                /*
                                 * ansi normal
                                 */
                                safe_known_str
                                ( "m\033[37m\033[",
                                  8, buf,
                                  &q );
                            } else {
                                /*
                                 * some other color
                                 */
                                safe_chr ( *p,
                                           buf, &q );
                            }
                            param_val = 0;
                        }
                        ++p;
                    }
                    while ( ( *p & 0xf0 ) == 0x20 ) {
                        ++p;
                    }
                    safe_chr ( *p, buf, &q );
                    ++p;
                    if ( param_val == 0 ) {
                        safe_known_str ( ANSI_WHITE, 5,
                                         buf, &q );
                    }
                } else {
                    ++p;
                    safe_known_str ( just_after_csi,
                                     p - just_after_csi, buf, &q );
                }
            } else {
                safe_copy_esccode ( p, buf, &q );
            }
            just_after_esccode = p;
        } else {
            ++p;
        }
    }
    safe_known_str ( just_after_esccode, p - just_after_esccode, buf, &q );
    return buf;
}

char *ansi_transition_esccode ( int ansi_before, int ansi_after ) {
    int ansi_bits_set, ansi_bits_clr;

    char *p;

    static char buffer[64];

    if ( ansi_before == ansi_after ) {
        return "";
    }

    buffer[0] = ESC_CHAR;
    buffer[1] = ANSI_CSI;
    p = buffer + 2;

    /*
     * If they turn off any highlight bits, or they change from some color
     * * to default color, we need to use ansi normal first.
     */

    ansi_bits_set = ( ~ansi_before ) & ansi_after;
    ansi_bits_clr = ansi_before & ( ~ansi_after );
    if ( ( ansi_bits_clr & 0xf00 ) ||	/* highlights off */
            ( ansi_bits_set & 0x088 ) ||	/* normal to color */
            ( ansi_bits_clr == 0x1000 ) ) {	/* explicit normal */
        strcpy ( p, "0;" );
        p += 2;
        ansi_bits_set = ( ~ansi_bits[0] ) & ansi_after;
        ansi_bits_clr = ansi_bits[0] & ( ~ansi_after );
    }

    /*
     * Next reproduce the highlight state
     */

    if ( ansi_bits_set & 0x100 ) {
        strcpy ( p, "1;" );
        p += 2;
    }
    if ( ansi_bits_set & 0x200 ) {
        strcpy ( p, "4;" );
        p += 2;
    }
    if ( ansi_bits_set & 0x400 ) {
        strcpy ( p, "5;" );
        p += 2;
    }
    if ( ansi_bits_set & 0x800 ) {
        strcpy ( p, "7;" );
        p += 2;
    }

    /*
     * Foreground color
     */
    if ( ( ansi_bits_set | ansi_bits_clr ) & 0x00f ) {
        strcpy ( p, "30;" );
        p += 3;
        p[-2] |= ( ansi_after & 0x00f );
    }

    /*
     * Background color
     */
    if ( ( ansi_bits_set | ansi_bits_clr ) & 0x0f0 ) {
        strcpy ( p, "40;" );
        p += 3;
        p[-2] |= ( ( ansi_after & 0x0f0 ) >> 4 );
    }

    /*
     * Terminate
     */
    if ( p > buffer + 2 ) {
        p[-1] = ANSI_END;
        /*
         * Buffer is already null-terminated by strcpy
         */
    } else {
        buffer[0] = '\0';
    }
    return buffer;
}

char *ansi_transition_mushcode ( int ansi_before, int ansi_after ) {
    int ansi_bits_set, ansi_bits_clr;

    char *p;

    static char ansi_mushcode_fg[9] = "xrgybmcw";

    static char ansi_mushcode_bg[9] = "XRGYBMCW";

    static char buffer[64];

    if ( ansi_before == ansi_after ) {
        return "";
    }

    p = buffer;

    /*
     * If they turn off any highlight bits, or they change from some color
     * * to default color, we need to use ansi normal first.
     */

    ansi_bits_set = ( ~ansi_before ) & ansi_after;
    ansi_bits_clr = ansi_before & ( ~ansi_after );
    if ( ( ansi_bits_clr & 0xf00 ) ||	/* highlights off */
            ( ansi_bits_set & 0x088 ) ||	/* normal to color */
            ( ansi_bits_clr == 0x1000 ) ) {	/* explicit normal */
        strcpy ( p, "%xn" );
        p += 3;
        ansi_bits_set = ( ~ansi_bits[0] ) & ansi_after;
        ansi_bits_clr = ansi_bits[0] & ( ~ansi_after );
    }

    /*
     * Next reproduce the highlight state
     */

    if ( ansi_bits_set & 0x100 ) {
        strcpy ( p, "%xh" );
        p += 3;
    }
    if ( ansi_bits_set & 0x200 ) {
        strcpy ( p, "%xu" );
        p += 3;
    }
    if ( ansi_bits_set & 0x400 ) {
        strcpy ( p, "%xf" );
        p += 3;
    }
    if ( ansi_bits_set & 0x800 ) {
        strcpy ( p, "%xi" );
        p += 3;
    }

    /*
     * Foreground color
     */
    if ( ( ansi_bits_set | ansi_bits_clr ) & 0x00f ) {
        strcpy ( p, "%xx" );
        p += 3;
        p[-1] = ansi_mushcode_fg[ ( ansi_after & 0x00f )];
    }

    /*
     * Background color
     */
    if ( ( ansi_bits_set | ansi_bits_clr ) & 0x0f0 ) {
        strcpy ( p, "%xX" );
        p += 3;
        p[-1] = ansi_mushcode_bg[ ( ansi_after & 0x0f0 ) >> 4];
    }

    /*
     * Terminate
     */
    *p = '\0';
    return buffer;
}

char *ansi_transition_letters ( int ansi_before, int ansi_after ) {
    int ansi_bits_set, ansi_bits_clr;

    char *p;

    static char ansi_mushcode_fg[9] = "xrgybmcw";

    static char ansi_mushcode_bg[9] = "XRGYBMCW";

    static char buffer[64];

    if ( ansi_before == ansi_after ) {
        return "";
    }

    p = buffer;

    /*
     * If they turn off any highlight bits, or they change from some color
     * * to default color, we need to use ansi normal first.
     */

    ansi_bits_set = ( ~ansi_before ) & ansi_after;
    ansi_bits_clr = ansi_before & ( ~ansi_after );
    if ( ( ansi_bits_clr & 0xf00 ) ||	/* highlights off */
            ( ansi_bits_set & 0x088 ) ||	/* normal to color */
            ( ansi_bits_clr == 0x1000 ) ) {	/* explicit normal */
        *p++ = 'n';
        ansi_bits_set = ( ~ansi_bits[0] ) & ansi_after;
        ansi_bits_clr = ansi_bits[0] & ( ~ansi_after );
    }

    /*
     * Next reproduce the highlight state
     */

    if ( ansi_bits_set & 0x100 ) {
        *p++ = 'h';
    }
    if ( ansi_bits_set & 0x200 ) {
        *p++ = 'u';
    }
    if ( ansi_bits_set & 0x400 ) {
        *p++ = 'f';
    }
    if ( ansi_bits_set & 0x800 ) {
        *p++ = 'i';
    }

    /*
     * Foreground color
     */
    if ( ( ansi_bits_set | ansi_bits_clr ) & 0x00f ) {
        *p++ = ansi_mushcode_fg[ ( ansi_after & 0x00f )];
    }

    /*
     * Background color
     */
    if ( ( ansi_bits_set | ansi_bits_clr ) & 0x0f0 ) {
        *p++ = ansi_mushcode_bg[ ( ansi_after & 0x0f0 ) >> 4];
    }

    /*
     * Terminate
     */
    *p = '\0';
    return buffer;
}

/* ansi_map_states -- Identify ansi state of every character in a string */

int ansi_map_states ( const char *s, int **m, char **p ) {
    static int ansi_map[LBUF_SIZE + 1];

    static char stripped[LBUF_SIZE + 1];

    int n, ansi_state;

    n = 0;
    ansi_state = ANST_NORMAL;

    while ( *s ) {
        if ( *s == ESC_CHAR ) {
            track_esccode ( s, ansi_state );
        } else {
            ansi_map[n] = ansi_state;
            stripped[n++] = *s++;
        }
    }

    ansi_map[n] = ANST_NORMAL;
    stripped[n] = '\0';

    *m = ansi_map;
    *p = stripped;
    return n;
}

/* remap_colors -- allow a change of the color sequences */

char *remap_colors ( const char *s, int *cmap ) {
    static char new[LBUF_SIZE];

    char *bp;

    int n;

    if ( !s || !*s || !cmap ) {
        return ( char * ) s;
    }

    bp = new;

    do {
        while ( *s && ( *s != ESC_CHAR ) ) {
            safe_chr ( *s, new, &bp );
            s++;
        }
        if ( *s == ESC_CHAR ) {
            safe_chr ( *s, new, &bp );
            s++;
            if ( *s == ANSI_CSI ) {
                safe_chr ( *s, new, &bp );
                s++;
                do {
                    n = ( int ) strtol ( s, ( char ** ) NULL, 10 );
                    if ( ( n >= I_ANSI_BLACK )
                            && ( n < I_ANSI_NUM )
                            && ( cmap[n - I_ANSI_BLACK] != 0 ) ) {
                        safe_ltos ( new, &bp,
                                    cmap[n - I_ANSI_BLACK] );
                        while ( isdigit ( *s ) ) {
                            s++;
                        }
                    } else {
                        while ( isdigit ( *s ) ) {
                            safe_chr ( *s, new, &bp );
                            s++;
                        }
                    }
                    if ( *s == ';' ) {
                        safe_chr ( *s, new, &bp );
                        s++;
                    }
                } while ( *s && ( *s != ANSI_END ) );
                if ( *s == ANSI_END ) {
                    safe_chr ( *s, new, &bp );
                    s++;
                }
            } else if ( *s ) {
                safe_chr ( *s, new, &bp );
                s++;
            }
        }
    } while ( *s );

    return new;
}


/* translate_string -- Convert (type = 1) raw character sequences into
 * MUSH substitutions or strip them (type = 0).
 */

char *translate_string ( char *str, int type ) {
    static char new[LBUF_SIZE];

    char *bp;

    bp = new;

    if ( type ) {
        int ansi_state = ANST_NORMAL;

        int ansi_state_prev = ANST_NORMAL;

        while ( *str ) {
            switch ( *str ) {
            case ESC_CHAR:
                while ( *str == ESC_CHAR ) {
                    track_esccode ( str, ansi_state );
                }
                safe_str ( ansi_transition_mushcode
                           ( ansi_state_prev, ansi_state ), new, &bp );
                ansi_state_prev = ansi_state;
                continue;
            case ' ':
                if ( str[1] == ' ' ) {
                    safe_known_str ( "%b", 2, new, &bp );
                } else {
                    safe_chr ( ' ', new, &bp );
                }
                break;
            case '\\':
            case '%':
            case '[':
            case ']':
            case '{':
            case '}':
            case '(':
            case ')':
                safe_chr ( '%', new, &bp );
                safe_chr ( *str, new, &bp );
                break;
            case '\r':
                break;
            case '\n':
                safe_known_str ( "%r", 2, new, &bp );
                break;
            case '\t':
                safe_known_str ( "%t", 2, new, &bp );
                break;
            default:
                safe_chr ( *str, new, &bp );
            }
            str++;
        }
    } else {
        while ( *str ) {
            switch ( *str ) {
            case ESC_CHAR:
                skip_esccode ( str );
                continue;
            case '\r':
                break;
            case '\n':
            case '\t':
                safe_chr ( ' ', new, &bp );
                break;
            default:
                safe_chr ( *str, new, &bp );
            }
            str++;
        }
    }
    *bp = '\0';
    return new;
}

/*
 * capitalizes an entire string
 */

char *upcasestr ( char *s ) {
    char *p;

    for ( p = s; p && *p; p++ ) {
        *p = toupper ( *p );
    }
    return s;
}

/*
 * ---------------------------------------------------------------------------
 * munge_space: Compress multiple spaces to one space,
 * also remove leading and trailing spaces.
 */

char *munge_space ( char *string ) {
    char *buffer, *p, *q;

    buffer = alloc_lbuf ( "munge_space" );
    p = string;
    q = buffer;
    while ( p && *p && isspace ( *p ) ) {
        p++;
    }		/*
				 * remove initial spaces
				 */
    while ( p && *p ) {
        while ( *p && !isspace ( *p ) ) {
            *q++ = *p++;
        }
        while ( *p && isspace ( *++p ) );
        if ( *p ) {
            *q++ = ' ';
        }
    }
    *q = '\0';		/*
				 * remove terminal spaces and terminate
				 * string
				 */
    return ( buffer );
}

/*
 * ---------------------------------------------------------------------------
 * * trim_spaces: Remove leading and trailing spaces.
 */

char *trim_spaces ( char *string ) {
    char *buffer, *p, *q;

    buffer = alloc_lbuf ( "trim_spaces" );
    p = string;
    q = buffer;
    while ( p && *p && isspace ( *p ) ) {	/* remove inital spaces */
        p++;
    }
    while ( p && *p ) {
        while ( *p && !isspace ( *p ) ) {	/* copy nonspace chars */
            *q++ = *p++;
        }
        while ( *p && isspace ( *p ) ) {	/* compress spaces */
            p++;
        }
        if ( *p ) {
            *q++ = ' ';    /* leave one space */
        }
    }
    *q = '\0';		/* terminate string */
    return ( buffer );
}

/*
 * ---------------------------------------------------------------------------
 * grabto: Return portion of a string up to the indicated character.  Also
 * returns a modified pointer to the string ready for another call.
 */

char *grabto ( char **str, char targ ) {
    char *savec, *cp;

    if ( !str || !*str || !**str ) {
        return NULL;
    }

    savec = cp = *str;
    while ( *cp && *cp != targ ) {
        cp++;
    }
    if ( *cp ) {
        *cp++ = '\0';
    }
    *str = cp;
    return savec;
}

int string_compare ( const char *s1, const char *s2 ) {
    if ( mudstate.standalone || mudconf.space_compress ) {
        while ( isspace ( *s1 ) ) {
            s1++;
        }
        while ( isspace ( *s2 ) ) {
            s2++;
        }
        while ( *s1 && *s2 && ( ( tolower ( *s1 ) == tolower ( *s2 ) ) ||
                                ( isspace ( *s1 ) && isspace ( *s2 ) ) ) ) {
            if ( isspace ( *s1 ) && isspace ( *s2 ) ) {
                /*
                 * skip all
                 * * other spaces
                 */
                while ( isspace ( *s1 ) ) {
                    s1++;
                }
                while ( isspace ( *s2 ) ) {
                    s2++;
                }
            } else {
                s1++;
                s2++;
            }
        }
        if ( ( *s1 ) && ( *s2 ) ) {
            return ( 1 );
        }
        if ( isspace ( *s1 ) ) {
            while ( isspace ( *s1 ) ) {
                s1++;
            }
            return ( *s1 );
        }
        if ( isspace ( *s2 ) ) {
            while ( isspace ( *s2 ) ) {
                s2++;
            }
            return ( *s2 );
        }
        if ( ( *s1 ) || ( *s2 ) ) {
            return ( 1 );
        }
        return ( 0 );
    } else {
        while ( *s1 && *s2 && tolower ( *s1 ) == tolower ( *s2 ) ) {
            s1++, s2++;
        }

        return ( tolower ( *s1 ) - tolower ( *s2 ) );
    }
}

int string_prefix ( const char *string, const char *prefix ) {
    int count = 0;

    while ( *string && *prefix && tolower ( *string ) == tolower ( *prefix ) ) {
        string++, prefix++, count++;
    }
    if ( *prefix == '\0' ) {	/* Matched all of prefix */
        return ( count );
    } else {
        return ( 0 );
    }
}

/*
 * accepts only nonempty matches starting at the beginning of a word
 */

const char *string_match ( const char *src, const char *sub ) {
    if ( ( *sub != '\0' ) && ( src ) ) {
        while ( *src ) {
            if ( string_prefix ( src, sub ) ) {
                return src;
            }
            /*
             * else scan to beginning of next word
             */
            while ( *src && isalnum ( *src ) ) {
                src++;
            }
            while ( *src && !isalnum ( *src ) ) {
                src++;
            }
        }
    }
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * replace_string: Returns an lbuf containing string STRING with all occurances
 * of OLD replaced by NEW. OLD and NEW may be different lengths.
 * (mitch 1 feb 91)
 *
 * edit_string: Like replace_string, but sensitive about ANSI codes, and
 * handles special ^ and $ cases.
 */

char *replace_string ( const char *old, const char *new, const char *string ) {
    char *result, *r, *s;

    int olen;

    if ( string == NULL ) {
        return NULL;
    }
    s = ( char * ) string;
    olen = strlen ( old );
    r = result = alloc_lbuf ( "replace_string" );
    while ( *s ) {

        /*
         * Copy up to the next occurrence of the first char of OLD
         */

        while ( *s && *s != *old ) {
            safe_chr ( *s, result, &r );
            s++;
        }

        /*
         * If we are really at an OLD, append NEW to the result and
         * bump the input string past the occurrence of
         * OLD. Otherwise, copy the char and try again.
         */

        if ( *s ) {
            if ( !strncmp ( old, s, olen ) ) {
                safe_str ( ( char * ) new, result, &r );
                s += olen;
            } else {
                safe_chr ( *s, result, &r );
                s++;
            }
        }
    }
    *r = '\0';
    return result;
}

void edit_string ( char *src, char **dst, char *from, char *to ) {
    char *cp, *p;

    int ansi_state, to_ansi_set, to_ansi_clr, tlen, flen;

    /*
     * We may have gotten an ANSI_NORMAL termination to OLD and NEW,
     * * that the user probably didn't intend to be there. (If the
     * * user really did want it there, he simply has to put a double
     * * ANSI_NORMAL in; this is non-intuitive but without it we can't
     * * let users swap one ANSI code for another using this.)  Thus,
     * * we chop off the terminating ANSI_NORMAL on both, if there is
     * * one.
     */

    p = from + strlen ( from ) - 4;
    if ( p >= from && !strcmp ( p, ANSI_NORMAL ) ) {
        *p = '\0';
    }

    p = to + strlen ( to ) - 4;
    if ( p >= to && !strcmp ( p, ANSI_NORMAL ) ) {
        *p = '\0';
    }


    /*
     * Scan the contents of the TO string. Figure out whether we
     * * have any embedded ANSI codes.
     */
    ansi_state = ANST_NONE;
    track_all_esccodes ( to, p, ansi_state );
    to_ansi_set = ( ~ANST_NONE ) & ansi_state;
    to_ansi_clr = ANST_NONE & ( ~ansi_state );
    tlen = p - to;

    /*
     * Do the substitution.  Idea for prefix/suffix from R'nice@TinyTIM
     */

    cp = *dst = alloc_lbuf ( "edit_string" );

    if ( !strcmp ( from, "^" ) ) {
        /*
         * Prepend 'to' to string
         */

        safe_known_str ( to, tlen, *dst, &cp );
        safe_copy_tracking ( src, p, ansi_state, *dst, &cp );

    } else if ( !strcmp ( from, "$" ) ) {

        /*
         * Append 'to' to string
         */

        ansi_state = ANST_NONE;
        safe_copy_tracking ( src, p, ansi_state, *dst, &cp );

        ansi_state |= to_ansi_set;
        ansi_state &= ~to_ansi_clr;
        safe_known_str ( to, tlen, *dst, &cp );

    } else {
        /*
         * Replace all occurances of 'from' with 'to'.  Handle the
         * special cases of from = \$ and \^.
         */

        if ( ( ( from[0] == '\\' ) || ( from[0] == '%' ) ) &&
                ( ( from[1] == '$' ) || ( from[1] == '^' ) ) &&
                ( from[2] == '\0' ) ) {
            from++;
        }

        flen = strlen ( from );

        ansi_state = ANST_NONE;
        while ( *src ) {

            /*
             * Copy up to the next occurrence of the first
             * * char of FROM.
             */

            p = src;
            while ( *src && ( *src != *from ) ) {
                if ( *src == ESC_CHAR ) {
                    track_esccode ( src, ansi_state );
                } else {
                    ++src;
                }
            }
            safe_known_str ( p, src - p, *dst, &cp );

            /*
             * If we are really at a FROM, append TO to the result
             * * and bump the input string past the occurrence of
             * * FROM. Otherwise, copy the char and try again.
             */

            if ( *src ) {
                if ( !strncmp ( from, src, flen ) ) {
                    /*
                     * Apply whatever ANSI transition
                     * * happens in TO
                     */
                    ansi_state |= to_ansi_set;
                    ansi_state &= ~to_ansi_clr;

                    safe_known_str ( to, tlen, *dst, &cp );
                    src += flen;
                } else {
                    /*
                     * We have to handle the case where
                     * * the first character in FROM is the
                     * * ANSI escape character. In that case
                     * * we move over and copy the entire
                     * * ANSI code. Otherwise we just copy
                     * * the character.
                     */
                    if ( *from == ESC_CHAR ) {
                        p = src;
                        track_esccode ( src, ansi_state );
                        safe_known_str ( p, src - p,
                                         *dst, &cp );
                    } else {
                        safe_chr ( *src, *dst, &cp );
                        ++src;
                    }
                }
            }
        }
    }

    safe_str ( ansi_transition_esccode ( ansi_state, ANST_NONE ), *dst, &cp );
}

int minmatch ( char *str, char *target, int min ) {
    while ( *str && *target && ( tolower ( *str ) == tolower ( *target ) ) ) {
        str++;
        target++;
        min--;
    }
    if ( *str ) {
        return 0;
    }
    if ( !*target ) {
        return 1;
    }
    return ( ( min <= 0 ) ? 1 : 0 );
}

/* ---------------------------------------------------------------------------
 * safe_copy_str, safe_copy_long_str, safe_chr_real_fn - Copy buffers,
 * watching for overflows.
 */

void safe_copy_str ( const char *src, char *buff,  char **bufp, int max ) {
    char *tp, *maxtp, *longtp;

    int n, len;

    tp = *bufp;
    if ( src == NULL ) {
        *tp = '\0';
        return;
    }
    maxtp = buff + max;
    longtp = tp + 7;
    maxtp = ( maxtp < longtp ) ? maxtp : longtp;

    while ( *src && ( tp < maxtp ) ) {
        *tp++ = *src++;
    }

    if ( *src == '\0' ) {	/* copied whole src, and tp is at most maxtp */
        *tp = '\0';
        *bufp = tp;
        return;
    }

    len = strlen ( src );
    n = max - ( tp - buff );	/* tp is either maxtp or longtp */
    if ( n <= 0 ) {
        *tp = '\0';
        *bufp = tp;
        return;
    }
    n = ( ( len < n ) ? len : n );
    memcpy ( tp, src, n );
    tp += n;
    *tp = '\0';
    *bufp = tp;
}


int safe_copy_str_fn ( const char *src, char *buff, char **bufp, int max ) {
    char *tp, *maxtp, *longtp;

    int n, len;

    tp = *bufp;
    if ( src == NULL ) {
        *tp = '\0';
        return 0;
    }
    maxtp = buff + max;
    longtp = tp + 7;
    maxtp = ( maxtp < longtp ) ? maxtp : longtp;

    while ( *src && ( tp < maxtp ) ) {
        *tp++ = *src++;
    }

    if ( *src == '\0' ) {	/* copied whole src, and tp is at most maxtp */
        *tp = '\0';
        *bufp = tp;
        return 0;
    }

    len = strlen ( src );
    n = max - ( tp - buff );	/* tp is either maxtp or longtp */
    if ( n <= 0 ) {
        len -= ( tp - *bufp );
        *tp = '\0';
        *bufp = tp;
        return ( len );
    }
    n = ( ( len < n ) ? len : n );
    memcpy ( tp, src, n );
    tp += n;
    *tp = '\0';
    *bufp = tp;

    return ( len - n );
}

int safe_copy_long_str ( char *src, char *buff, char **bufp, int max ) {
    int len, n;

    char *tp;

    tp = *bufp;
    if ( src == NULL ) {
        *tp = '\0';
        return 0;
    }

    len = strlen ( src );
    n = max - ( tp - buff );
    if ( n < 0 ) {
        n = 0;
    }

    strncpy ( tp, src, n );
    buff[max] = '\0';

    if ( len <= n ) {
        *bufp = tp + len;
        return ( 0 );
    } else {
        *bufp = tp + n;
        return ( len - n );
    }
}


void safe_known_str ( const char *src, int known, char *buff, char **bufp ) {
    int n;

    char *tp, *maxtp;

    tp = *bufp;

    if ( !src ) {
        *tp = '\0';
        return;
    }

    maxtp = buff + LBUF_SIZE - 1;

    if ( known > 7 ) {
        n = maxtp - tp;
        if ( n <= 0 ) {
            *tp = '\0';
            return;
        }
        n = ( ( known < n ) ? known : n );
        memcpy ( tp, src, n );
        tp += n;
        *tp = '\0';
        *bufp = tp;
        return;
    }

    if ( tp + known < maxtp ) {
        maxtp = tp + known;
    }
    while ( *src && ( tp < maxtp ) ) {
        * ( tp ) ++ = *src++;
    }

    *tp = '\0';
    *bufp = tp;
}


int safe_chr_real_fn ( char src, char *buff, char **bufp, int max ) {
    char *tp;

    int retval = 0;

    tp = *bufp;
    if ( ( tp - buff ) < max ) {
        *tp++ = src;
        *bufp = tp;
        *tp = '\0';
    } else {
        buff[max] = '\0';
        retval = 1;
    }

    return retval;
}

/* ---------------------------------------------------------------------------
 * More utilities.
 */

int matches_exit_from_list ( char *str, char *pattern ) {
    char *s;

    if ( *str == '\0' ) {	/* never match empty */
        return 0;
    }

    while ( *pattern ) {
        for ( s = str;	/* check out this one */
                ( *s && ( tolower ( *s ) == tolower ( *pattern ) ) &&
                  *pattern && ( *pattern != EXIT_DELIMITER ) );
                s++, pattern++ );

        /*
         * Did we match it all?
         */

        if ( *s == '\0' ) {

            /*
             * Make sure nothing afterwards
             */

            while ( *pattern && isspace ( *pattern ) ) {
                pattern++;
            }

            /*
             * Did we get it?
             */

            if ( !*pattern || ( *pattern == EXIT_DELIMITER ) ) {
                return 1;
            }
        }
        /*
         * We didn't get it, find next string to test
         */

        while ( *pattern && *pattern++ != EXIT_DELIMITER );
        while ( isspace ( *pattern ) ) {
            pattern++;
        }
    }
    return 0;
}

int ltos ( char *s, long num ) {
    /*
     * Mark Vasoll's long int to string converter.
     */
    char buf[20], *p;

    unsigned long anum;

    p = buf;

    /*
     * absolute value
     */
    anum = ( num < 0 ) ? -num : num;

    /*
     * build up the digits backwards by successive division
     */
    while ( anum > 9 ) {
        *p++ = '0' + ( anum % 10 );
        anum /= 10;
    }

    /*
     * put in the sign if needed
     */
    if ( num < 0 ) {
        *s++ = '-';
    }

    /*
     * put in the last digit, this makes very fast single digits numbers
     */
    *s++ = '0' + ( char ) anum;

    /*
     * reverse the rest of the digits (if any) into the provided buf
     */
    while ( p-- > buf ) {
        *s++ = *p;
    }

    /*
     * terminate the resulting string
     */
    *s = '\0';
    return 0;
}

void safe_ltos ( char *s, char **bufc, long num ) {
    /*
     * Mark Vasoll's long int to string converter.
     */
    char buf[20], *p, *tp, *endp;

    unsigned long anum;

    p = buf;
    tp = *bufc;

    /*
     * absolute value
     */
    anum = ( num < 0 ) ? -num : num;

    /*
     * build up the digits backwards by successive division
     */
    while ( anum > 9 ) {
        *p++ = '0' + ( anum % 10 );
        anum /= 10;
    }

    if ( tp > s + LBUF_SIZE - 21 ) {
        endp = s + LBUF_SIZE - 1;
        /*
         * put in the sign if needed
         */
        if ( num < 0 && ( tp < endp ) ) {
            *tp++ = '-';
        }

        /*
         * put in the last digit, this makes very fast single
         * * digits numbers
         */
        if ( tp < endp ) {
            *tp++ = '0' + ( char ) anum;
        }

        /*
         * reverse the rest of the digits (if any) into the
         * * provided buf
         */
        while ( ( p-- > buf ) && ( tp < endp ) ) {
            *tp++ = *p;
        }
    } else {
        if ( num < 0 ) {
            *tp++ = '-';
        }
        *tp++ = '0' + ( char ) anum;
        while ( p-- > buf ) {
            *tp++ = *p;
        }
    }

    /*
     * terminate the resulting string
     */
    *tp = '\0';
    *bufc = tp;
}


char *repeatchar ( int count, char ch ) {
    int num;
    char *str, *ptr;

    if ( count < 1 ) {
        /*
         * If negative or zero spaces return a single character, -except-
         * allow 'repeatchar(0)' to return "" for calculated padding
         */

        if ( count == 0 ) {
            return NULL;
        }
    }
    str = XMALLOC ( count + 1, "repeatchar" );
    /*
     * Yes i'm a bit paranoid here...
     */
    memset ( str, 0, count + 1 );
    memset ( str, ch, count -1 );
    return str;
}
