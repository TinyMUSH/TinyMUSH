/* flags.c - flag manipulation routines */

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

#include "command.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "match.h"		/* required by code */
#include "ansi.h"		/* required by code */

/*
 * ---------------------------------------------------------------------------
 * fh_any: set or clear indicated bit, no security checking
 */

int fh_any ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    /*
     * Never let God drop his own wizbit.
     */

    if ( God ( target ) && reset && ( flag == WIZARD ) &&
            ! ( fflags & FLAG_WORD2 ) && ! ( fflags & FLAG_WORD3 ) ) {
        notify ( player, "You cannot make God mortal." );
        return 0;
    }
    /*
     * Otherwise we can go do it.
     */

    if ( fflags & FLAG_WORD3 ) {
        if ( reset ) {
            s_Flags3 ( target, Flags3 ( target ) & ~flag );
        } else {
            s_Flags3 ( target, Flags3 ( target ) | flag );
        }
    } else if ( fflags & FLAG_WORD2 ) {
        if ( reset ) {
            s_Flags2 ( target, Flags2 ( target ) & ~flag );
        } else {
            s_Flags2 ( target, Flags2 ( target ) | flag );
        }
    } else {
        if ( reset ) {
            s_Flags ( target, Flags ( target ) & ~flag );
        } else {
            s_Flags ( target, Flags ( target ) | flag );
        }
    }
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * fh_god: only GOD may set or clear the bit
 */

int fh_god ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( !God ( player ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_wiz: only WIZARDS (or GOD) may set or clear the bit
 */

int fh_wiz ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( !Wizard ( player ) && !God ( player ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_wizroy: only WIZARDS, ROYALTY, (or GOD) may set or clear the bit
 */

int fh_wizroy ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( !WizRoy ( player ) && !God ( player ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_restrict_player: Only Wizards can set this on players, but ordinary
 * players can set it on other types of objects.
 */

int fh_restrict_player ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( isPlayer ( target ) && !Wizard ( player ) && !God ( player ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_privileged: You can set this flag on a non-player object, if you
 * yourself have this flag and are a player who owns themselves (i.e., no
 * robots). Only God can set this on a player.
 */

int fh_privileged ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    int has_it;

    if ( !God ( player ) ) {

        if ( !isPlayer ( player ) || ( player != Owner ( player ) ) ) {
            return 0;
        }
        if ( isPlayer ( target ) ) {
            return 0;
        }

        if ( fflags & FLAG_WORD3 ) {
            has_it = ( Flags3 ( player ) & flag ) ? 1 : 0;
        } else if ( fflags & FLAG_WORD2 ) {
            has_it = ( Flags2 ( player ) & flag ) ? 1 : 0;
        } else {
            has_it = ( Flags ( player ) & flag ) ? 1 : 0;
        }

        if ( !has_it ) {
            return 0;
        }
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_inherit: only players may set or clear this bit.
 */

int fh_inherit ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( !Inherits ( player ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_dark_bit: manipulate the dark bit. Nonwizards may not set on players.
 */

int fh_dark_bit ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( !reset && isPlayer ( target ) && ! ( ( target == player ) &&
            Can_Hide ( player ) ) && ( !Wizard ( player ) && !God ( player ) ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_going_bit: Manipulate the going bit. Can only be cleared on objects
 * slated for destruction, by non-god. May only be set by god. Even god can't
 * destroy nondestroyable objects.
 */

int fh_going_bit ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( Going ( target ) && reset && ( Typeof ( target ) != TYPE_GARBAGE ) ) {
        notify ( player,
                 "Your object has been spared from destruction." );
        return ( fh_any ( target, player, flag, fflags, reset ) );
    }
    if ( !God ( player ) || !destroyable ( target ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_hear_bit: set or clear bits that affect hearing.
 */

int fh_hear_bit ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    int could_hear;

    int result;

    could_hear = Hearer ( target );
    result = fh_any ( target, player, flag, fflags, reset );
    handle_ears ( target, could_hear, Hearer ( target ) );
    return result;
}

/*
 * ---------------------------------------------------------------------------
 * fh_player_bit: Can set and reset this on everything but players.
 */

int fh_player_bit ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( isPlayer ( target ) ) {
        return 0;
    }
    return ( fh_any ( target, player, flag, fflags, reset ) );
}

/*
 * ---------------------------------------------------------------------------
 * fh_power_bit: Check power bit to set/reset.
 */

int fh_power_bit ( dbref target, dbref player, FLAG flag, int fflags, int reset ) {
    if ( flag & WATCHER ) {
        /*
         * Wizards can set this on anything. Players with the Watch
         * power can set this on themselves.
         */
        if ( Wizard ( player ) ||
                ( ( Owner ( player ) == Owner ( target ) ) && Can_Watch ( player ) ) ) {
            return ( fh_any ( target, player, flag, fflags, reset ) );
        } else {
            return 0;
        }
    }
    return 0;
}


/* *INDENT-OFF* */

/* All flags names MUST be in uppercase! */

FLAGENT		gen_flags[] = {
    {
        "ABODE", ABODE, 'A',
        FLAG_WORD2, 0, fh_any
    },
    {
        "BLIND", BLIND, 'B',
        FLAG_WORD2, 0, fh_wiz
    },
    {
        "CHOWN_OK", CHOWN_OK, 'C',
        0, 0, fh_any
    },
    {
        "DARK", DARK, 'D',
        0, 0, fh_dark_bit
    },
    {
        "FREE", NODEFAULT, 'F',
        FLAG_WORD3, 0, fh_wiz
    },
    {
        "GOING", GOING, 'G',
        0, CA_NO_DECOMP, fh_going_bit
    },
    {
        "HAVEN", HAVEN, 'H',
        0, 0, fh_any
    },
    {
        "INHERIT", INHERIT, 'I',
        0, 0, fh_inherit
    },
    {
        "JUMP_OK", JUMP_OK, 'J',
        0, 0, fh_any
    },
    {
        "KEY", KEY, 'K',
        FLAG_WORD2, 0, fh_any
    },
    {
        "LINK_OK", LINK_OK, 'L',
        0, 0, fh_any
    },
    {
        "MONITOR", MONITOR, 'M',
        0, 0, fh_hear_bit
    },
    {
        "NOSPOOF", NOSPOOF, 'N',
        0, CA_WIZARD, fh_any
    },
    {
        "OPAQUE", OPAQUE, 'O',
        0, 0, fh_any
    },
    {
        "QUIET", QUIET, 'Q',
        0, 0, fh_any
    },
    {
        "STICKY", STICKY, 'S',
        0, 0, fh_any
    },
    {
        "TRACE", TRACE, 'T',
        0, 0, fh_any
    },
    {
        "UNFINDABLE", UNFINDABLE, 'U',
        FLAG_WORD2, 0, fh_any
    },
    {
        "VISUAL", VISUAL, 'V',
        0, 0, fh_any
    },
    {
        "WIZARD", WIZARD, 'W',
        0, 0, fh_god
    },
    {
        "ANSI", ANSI, 'X',
        FLAG_WORD2, 0, fh_any
    },
    {
        "PARENT_OK", PARENT_OK, 'Y',
        FLAG_WORD2, 0, fh_any
    },
    {
        "ROYALTY", ROYALTY, 'Z',
        0, 0, fh_wiz
    },
    {
        "AUDIBLE", HEARTHRU, 'a',
        0, 0, fh_hear_bit
    },
    {
        "BOUNCE", BOUNCE, 'b',
        FLAG_WORD2, 0, fh_any
    },
    {
        "CONNECTED", CONNECTED, 'c',
        FLAG_WORD2, CA_NO_DECOMP, fh_god
    },
    {
        "DESTROY_OK", DESTROY_OK, 'd',
        0, 0, fh_any
    },
    {
        "ENTER_OK", ENTER_OK, 'e',
        0, 0, fh_any
    },
    {
        "FIXED", FIXED, 'f',
        FLAG_WORD2, 0, fh_restrict_player
    },
    {
        "UNINSPECTED", UNINSPECTED, 'g',
        FLAG_WORD2, 0, fh_wizroy
    },
    {
        "HALTED", HALT, 'h',
        0, 0, fh_any
    },
    {
        "IMMORTAL", IMMORTAL, 'i',
        0, 0, fh_wiz
    },
    {
        "GAGGED", GAGGED, 'j',
        FLAG_WORD2, 0, fh_wiz
    },
    {
        "CONSTANT", CONSTANT_ATTRS, 'k',
        FLAG_WORD2, 0, fh_wiz
    },
    {
        "LIGHT", LIGHT, 'l',
        FLAG_WORD2, 0, fh_any
    },
    {
        "MYOPIC", MYOPIC, 'm',
        0, 0, fh_any
    },
    {
        "AUDITORIUM", AUDITORIUM, 'n',
        FLAG_WORD2, 0, fh_any
    },
    {
        "ZONE", ZONE_PARENT, 'o',
        FLAG_WORD2, 0, fh_any
    },
    {
        "PUPPET", PUPPET, 'p',
        0, 0, fh_hear_bit
    },
    {
        "TERSE", TERSE, 'q',
        0, 0, fh_any
    },
    {
        "ROBOT", ROBOT, 'r',
        0, 0, fh_player_bit
    },
    {
        "SAFE", SAFE, 's',
        0, 0, fh_any
    },
    {
        "TRANSPARENT", SEETHRU, 't',
        0, 0, fh_any
    },
    {
        "SUSPECT", SUSPECT, 'u',
        FLAG_WORD2, CA_WIZARD, fh_wiz
    },
    {
        "VERBOSE", VERBOSE, 'v',
        0, 0, fh_any
    },
    {
        "STAFF", STAFF, 'w',
        FLAG_WORD2, 0, fh_wiz
    },
    {
        "SLAVE", SLAVE, 'x',
        FLAG_WORD2, CA_WIZARD, fh_wiz
    },
    {
        "ORPHAN", ORPHAN, 'y',
        FLAG_WORD3, 0, fh_any
    },
    {
        "CONTROL_OK", CONTROL_OK, 'z',
        FLAG_WORD2, 0, fh_any
    },
    {
        "STOP", STOP_MATCH, '!',
        FLAG_WORD2, 0, fh_wiz
    },
    {
        "COMMANDS", HAS_COMMANDS, '$',
        FLAG_WORD2, 0, fh_any
    },
    {
        "PRESENCE", PRESENCE, '^',
        FLAG_WORD3, 0, fh_wiz
    },
    {
        "NOBLEED", NOBLEED, '-',
        FLAG_WORD2, 0, fh_any
    },
    {
        "VACATION", VACATION, '|',
        FLAG_WORD2, 0, fh_restrict_player
    },
    {
        "HEAD", HEAD_FLAG, '?',
        FLAG_WORD2, 0, fh_wiz
    },
    {
        "WATCHER", WATCHER, '+',
        FLAG_WORD2, 0, fh_power_bit
    },
    {
        "HAS_DAILY", HAS_DAILY, '*',
        FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "HAS_STARTUP", HAS_STARTUP, '=',
        0, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "HAS_FORWARDLIST", HAS_FWDLIST, '&',
        FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "HAS_LISTEN", HAS_LISTEN, '@',
        FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "HAS_PROPDIR", HAS_PROPDIR, ',',
        FLAG_WORD3, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "PLAYER_MAILS", PLAYER_MAILS, '`',
        FLAG_WORD2, CA_GOD | CA_NO_DECOMP, fh_god
    },
#ifdef PUEBLO_SUPPORT
    {
        "HTML", HTML, '~',
        FLAG_WORD2, 0, fh_any
    },
#endif
    {
        "REDIR_OK", REDIR_OK, '>',
        FLAG_WORD3, 0, fh_any
    },
    {
        "HAS_REDIRECT", HAS_REDIRECT, '<',
        FLAG_WORD3, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "HAS_DARKLOCK", HAS_DARKLOCK, '.',
        FLAG_WORD3, CA_GOD | CA_NO_DECOMP, fh_god
    },
    {
        "SPEECHMOD", HAS_SPEECHMOD, '"',
        FLAG_WORD3, 0, fh_any
    },
    {
        "MARKER0", MARK_0, '0',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER1", MARK_1, '1',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER2", MARK_2, '2',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER3", MARK_3, '3',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER4", MARK_4, '4',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER5", MARK_5, '5',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER6", MARK_6, '6',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER7", MARK_7, '7',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER8", MARK_8, '8',
        FLAG_WORD3, 0, fh_god
    },
    {
        "MARKER9", MARK_9, '9',
        FLAG_WORD3, 0, fh_god
    },
    {
        NULL, 0, ' ',
        0, 0, NULL
    }
};

OBJENT		object_types[8] = {
    {"ROOM", 'R', CA_PUBLIC, OF_CONTENTS | OF_EXITS | OF_DROPTO | OF_HOME},
    {
        "THING", ' ', CA_PUBLIC,
        OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_SIBLINGS
    },
    {"EXIT", 'E', CA_PUBLIC, OF_SIBLINGS},
    {
        "PLAYER", 'P', CA_PUBLIC,
        OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_OWNER | OF_SIBLINGS
    },
    {"TYPE5", '+', CA_GOD, 0},
    {
        "GARBAGE", '_', CA_PUBLIC,
        OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_SIBLINGS
    },
    {"GARBAGE", '#', CA_GOD, 0}
};

/* *INDENT-ON* */





/*
 * ---------------------------------------------------------------------------
 * init_flagtab: initialize flag hash tables.
 */

void init_flagtab ( void ) {
    FLAGENT *fp;

    hashinit ( &mudstate.flags_htab, 100 * HASH_FACTOR, HT_STR | HT_KEYREF );

    for ( fp = gen_flags; fp->flagname; fp++ ) {
        hashadd ( ( char * ) fp->flagname, ( int * ) fp, &mudstate.flags_htab,
                  0 );
    }
}

/*
 * ---------------------------------------------------------------------------
 * display_flags: display available flags.
 */

void display_flagtab ( dbref player ) {
    char *buf, *bp;

    FLAGENT *fp;

    bp = buf = alloc_lbuf ( "display_flagtab" );
    safe_str ( ( char * ) "Flags:", buf, &bp );
    for ( fp = gen_flags; fp->flagname; fp++ ) {
        if ( ( fp->listperm & CA_WIZARD ) && !Wizard ( player ) ) {
            continue;
        }
        if ( ( fp->listperm & CA_GOD ) && !God ( player ) ) {
            continue;
        }
        safe_chr ( ' ', buf, &bp );
        safe_str ( ( char * ) fp->flagname, buf, &bp );
        safe_chr ( '(', buf, &bp );
        safe_chr ( fp->flaglett, buf, &bp );
        safe_chr ( ')', buf, &bp );
    }
    *bp = '\0';
    notify ( player, buf );
    free_lbuf ( buf );
}

FLAGENT *find_flag ( dbref thing, char *flagname ) {
    char *cp;

    /*
     * Make sure the flag name is valid
     */

    for ( cp = flagname; *cp; cp++ ) {
        *cp = toupper ( *cp );
    }
    return ( FLAGENT * ) hashfind ( flagname, &mudstate.flags_htab );
}

/*
 * ---------------------------------------------------------------------------
 * flag_set: Set or clear a specified flag on an object.
 */

void flag_set ( dbref target, dbref player, char *flag, int key ) {
    FLAGENT *fp;

    int negate, result;

    /*
     * Trim spaces, and handle the negation character
     */

    negate = 0;
    while ( *flag && isspace ( *flag ) ) {
        flag++;
    }
    if ( *flag == '!' ) {
        negate = 1;
        flag++;
    }
    while ( *flag && isspace ( *flag ) ) {
        flag++;
    }

    /*
     * Make sure a flag name was specified
     */

    if ( *flag == '\0' ) {
        if ( negate ) {
            notify ( player, "You must specify a flag to clear." );
        } else {
            notify ( player, "You must specify a flag to set." );
        }
        return;
    }
    fp = find_flag ( target, flag );
    if ( fp == NULL ) {
        notify ( player, "I don't understand that flag." );
        return;
    }
    /*
     * Invoke the flag handler, and print feedback
     */

    result = fp->handler ( target, player, fp->flagvalue,
                           fp->flagflag, negate );
    if ( !result ) {
        notify ( player, NOPERM_MESSAGE );
    } else if ( ! ( key & SET_QUIET ) && !Quiet ( player ) ) {
        notify ( player, ( negate ? "Cleared." : "Set." ) );
        s_Modified ( target );
    }
    return;
}

/*
 * ---------------------------------------------------------------------------
 * decode_flags: converts a flag set into corresponding letters.
 */

char *decode_flags ( dbref player, FLAGSET flagset ) {
    char *buf, *bp, *s;

    FLAGENT *fp;

    int flagtype;

    FLAG fv;

    buf = bp = s = alloc_sbuf ( "decode_flags" );
    *bp = '\0';

    flagtype = ( flagset.word1 & TYPE_MASK );
    if ( object_types[flagtype].lett != ' ' ) {
        safe_sb_chr ( object_types[flagtype].lett, buf, &bp );
    }

    for ( fp = gen_flags; fp->flagname; fp++ ) {
        if ( fp->flagflag & FLAG_WORD3 ) {
            fv = flagset.word3;
        } else if ( fp->flagflag & FLAG_WORD2 ) {
            fv = flagset.word2;
        } else {
            fv = flagset.word1;
        }
        if ( fv & fp->flagvalue ) {
            if ( ( fp->listperm & CA_WIZARD ) && !Wizard ( player ) ) {
                continue;
            }
            if ( ( fp->listperm & CA_GOD ) && !God ( player ) ) {
                continue;
            }

            safe_sb_chr ( fp->flaglett, buf, &bp );
        }
    }

    *bp = '\0';
    return buf;
}

/*
 * ---------------------------------------------------------------------------
 * unparse_flags: converts a thing's flags into corresponding letters.
 */

char *unparse_flags ( dbref player, dbref thing ) {
    char *buf, *bp, *s;

    FLAGENT *fp;

    int flagtype;

    FLAG fv, flagword, flag2word, flag3word;

    buf = bp = s = alloc_sbuf ( "unparse_flags" );
    *bp = '\0';

    if ( !Good_obj ( player ) || !Good_obj ( thing ) ) {
        strcpy ( buf, "#-2 ERROR" );
        return buf;
    }
    flagword = Flags ( thing );
    flag2word = Flags2 ( thing );
    flag3word = Flags3 ( thing );
    flagtype = ( flagword & TYPE_MASK );
    if ( object_types[flagtype].lett != ' ' ) {
        safe_sb_chr ( object_types[flagtype].lett, buf, &bp );
    }

    for ( fp = gen_flags; fp->flagname; fp++ ) {
        if ( fp->flagflag & FLAG_WORD3 ) {
            fv = flag3word;
        } else if ( fp->flagflag & FLAG_WORD2 ) {
            fv = flag2word;
        } else {
            fv = flagword;
        }
        if ( fv & fp->flagvalue ) {

            if ( ( fp->listperm & CA_WIZARD ) && !Wizard ( player ) ) {
                continue;
            }
            if ( ( fp->listperm & CA_GOD ) && !God ( player ) ) {
                continue;
            }

            /*
             * don't show CONNECT on dark wizards to mortals
             */

            if ( ( flagtype == TYPE_PLAYER ) && isConnFlag ( fp ) &&
                    Can_Hide ( thing ) && Hidden ( thing ) &&
                    !See_Hidden ( player ) ) {
                continue;
            }

            /*
             * Check if this is a marker flag and we're at the
             * beginning of a buffer. If so, we need to insert a
             * separator character first so we don't end up
             * running the dbref number into the flags.
             */
            if ( ( s == bp ) && isMarkerFlag ( fp ) ) {
                safe_sb_chr ( MARK_FLAG_SEP, buf, &bp );
            }

            safe_sb_chr ( fp->flaglett, buf, &bp );
        }
    }

    *bp = '\0';
    return buf;
}

/*
 * ---------------------------------------------------------------------------
 * has_flag: does object have flag visible to player?
 */

int has_flag ( dbref player, dbref it, char *flagname ) {
    FLAGENT *fp;

    FLAG fv;

    fp = find_flag ( it, flagname );
    if ( fp == NULL ) {	/* find_flag() uppercases the string */
        if ( !strcmp ( flagname, "PLAYER" ) ) {
            return isPlayer ( it );
        }
        if ( !strcmp ( flagname, "THING" ) ) {
            return isThing ( it );
        }
        if ( !strcmp ( flagname, "ROOM" ) ) {
            return isRoom ( it );
        }
        if ( !strcmp ( flagname, "EXIT" ) ) {
            return isExit ( it );
        }
        return 0;
    }
    if ( fp->flagflag & FLAG_WORD3 ) {
        fv = Flags3 ( it );
    } else if ( fp->flagflag & FLAG_WORD2 ) {
        fv = Flags2 ( it );
    } else {
        fv = Flags ( it );
    }

    if ( fv & fp->flagvalue ) {
        if ( ( fp->listperm & CA_WIZARD ) && !Wizard ( player ) ) {
            return 0;
        }
        if ( ( fp->listperm & CA_GOD ) && !God ( player ) ) {
            return 0;
        }
        /*
         * don't show CONNECT on dark wizards to mortals
         */
        if ( isPlayer ( it ) && isConnFlag ( fp ) &&
                Can_Hide ( it ) && Hidden ( it ) && !See_Hidden ( player ) ) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * flag_description: Return an mbuf containing the type and flags on thing.
 */

char *flag_description ( dbref player, dbref target ) {
    char *buff, *bp;

    FLAGENT *fp;

    int otype;

    FLAG fv;

    /*
     * Allocate the return buffer
     */

    otype = Typeof ( target );
    bp = buff = alloc_mbuf ( "flag_description" );

    /*
     * Store the header strings and object type
     */

    safe_mb_str ( ( char * ) "Type: ", buff, &bp );
    safe_mb_str ( ( char * ) object_types[otype].name, buff, &bp );
    safe_mb_str ( ( char * ) " Flags:", buff, &bp );
    if ( object_types[otype].perm != CA_PUBLIC ) {
        return buff;
    }
    /*
     * Store the type-invariant flags
     */

    for ( fp = gen_flags; fp->flagname; fp++ ) {
        if ( fp->flagflag & FLAG_WORD3 ) {
            fv = Flags3 ( target );
        } else if ( fp->flagflag & FLAG_WORD2 ) {
            fv = Flags2 ( target );
        } else {
            fv = Flags ( target );
        }
        if ( fv & fp->flagvalue ) {
            if ( ( fp->listperm & CA_WIZARD ) && !Wizard ( player ) ) {
                continue;
            }
            if ( ( fp->listperm & CA_GOD ) && !God ( player ) ) {
                continue;
            }
            /*
             * don't show CONNECT on dark wizards to mortals
             */
            if ( isPlayer ( target ) && isConnFlag ( fp ) &&
                    Can_Hide ( target ) && Hidden ( target ) &&
                    !See_Hidden ( player ) ) {
                continue;
            }
            safe_mb_chr ( ' ', buff, &bp );
            safe_mb_str ( ( char * ) fp->flagname, buff, &bp );
        }
    }

    return buff;
}

/*
 *
 * --------------------------------------------------------------------------- *
 * Return an lbuf containing the name and number of an object
 */

char *unparse_object_numonly ( dbref target ) {
    char *buf, *bp;

    buf = alloc_lbuf ( "unparse_object_numonly" );
    if ( target == NOTHING ) {
        strcpy ( buf, "*NOTHING*" );
    } else if ( target == HOME ) {
        strcpy ( buf, "*HOME*" );
    } else if ( target == AMBIGUOUS ) {
        strcpy ( buf, "*VARIABLE*" );
    } else if ( !Good_obj ( target ) ) {
        sprintf ( buf, "*ILLEGAL*(#%d)", target );
    } else {
        bp = buf;
        safe_tmprintf_str ( buf, &bp, "%s(#%d)", Name ( target ), target );
    }
    return buf;
}

/*
 * ---------------------------------------------------------------------------
 * Return an lbuf pointing to the object name and possibly the db# and flags
 */

char *unparse_object ( dbref player, dbref target, int obey_myopic ) {
    char *buf, *fp, *bp;

    int exam;

    buf = alloc_lbuf ( "unparse_object" );
    if ( target == NOTHING ) {
        strcpy ( buf, "*NOTHING*" );
    } else if ( target == HOME ) {
        strcpy ( buf, "*HOME*" );
    } else if ( target == AMBIGUOUS ) {
        strcpy ( buf, "*VARIABLE*" );
    } else if ( isGarbage ( target ) ) {
        fp = unparse_flags ( player, target );
        bp = buf;
        safe_tmprintf_str ( buf, &bp, "*GARBAGE*(#%d%s)", target, fp );
        free_sbuf ( fp );
    } else if ( !Good_obj ( target ) ) {
        sprintf ( buf, "*ILLEGAL*(#%d)", target );
    } else {
        if ( obey_myopic ) {
            exam = MyopicExam ( player, target );
        } else {
            exam = Examinable ( player, target );
        }
        if ( exam ||
                ( Flags ( target ) & ( CHOWN_OK | JUMP_OK | LINK_OK |
                                       DESTROY_OK ) ) || ( Flags2 ( target ) & ABODE ) ) {

            /*
             * show everything
             */
            fp = unparse_flags ( player, target );
            bp = buf;
            safe_tmprintf_str ( buf, &bp, "%s(#%d%s)",
                                Name ( target ), target, fp );
            free_sbuf ( fp );
        } else {
            /*
             * show only the name.
             */
            strcpy ( buf, Name ( target ) );
        }
    }
    return buf;
}

/*
 * ---------------------------------------------------------------------------
 * letter_to_flag: Given a one-character flag abbrev, return the flag
 * pointer.
 */

FLAGENT *letter_to_flag ( char this_letter ) {
    FLAGENT *fp;

    for ( fp = gen_flags; fp->flagname; fp++ ) {
        if ( fp->flaglett == this_letter ) {
            return fp;
        }
    }
    return NULL;
}

/*
 * ---------------------------------------------------------------------------
 * cf_flag_access: Modify who can set a flag.
 */

int cf_flag_access ( int *vp, char *str, long extra, dbref player, char *cmd ) {
    char *fstr, *permstr, *tokst;

    FLAGENT *fp;

    fstr = strtok_r ( str, " \t=,", &tokst );
    permstr = strtok_r ( NULL, " \t=,", &tokst );

    if ( !fstr || !*fstr || !permstr || !*permstr ) {
        return -1;
    }
    if ( ( fp = find_flag ( GOD, fstr ) ) == NULL ) {
        cf_log_notfound ( player, cmd, "No such flag", fstr );
        return -1;
    }
    /*
     * Don't change the handlers on special things.
     */

    if ( ( fp->handler != fh_any ) &&
            ( fp->handler != fh_wizroy ) &&
            ( fp->handler != fh_wiz ) &&
            ( fp->handler != fh_god ) &&
            ( fp->handler != fh_restrict_player ) &&
            ( fp->handler != fh_privileged ) ) {

        STARTLOG ( LOG_CONFIGMODS, "CFG", "PERM" )
        log_printf ( "Cannot change access for flag: %s",
                     fp->flagname );
        ENDLOG
        return -1;
    }
    if ( !strcmp ( permstr, ( char * ) "any" ) ) {
        fp->handler = fh_any;
    } else if ( !strcmp ( permstr, ( char * ) "royalty" ) ) {
        fp->handler = fh_wizroy;
    } else if ( !strcmp ( permstr, ( char * ) "wizard" ) ) {
        fp->handler = fh_wiz;
    } else if ( !strcmp ( permstr, ( char * ) "god" ) ) {
        fp->handler = fh_god;
    } else if ( !strcmp ( permstr, ( char * ) "restrict_player" ) ) {
        fp->handler = fh_restrict_player;
    } else if ( !strcmp ( permstr, ( char * ) "privileged" ) ) {
        fp->handler = fh_privileged;
    } else {
        cf_log_notfound ( player, cmd, "Flag access", permstr );
        return -1;
    }
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * cf_flag_name: Modify the name of a user-defined flag.
 */

int cf_flag_name ( int *vp, char *str, long extra, dbref player, char *cmd ) {
    char *numstr, *namestr, *tokst;

    FLAGENT *fp;

    int flagnum = -1;

    char *flagstr, *cp;

    numstr = strtok_r ( str, " \t=,", &tokst );
    namestr = strtok_r ( NULL, " \t=,", &tokst );

    if ( numstr && ( strlen ( numstr ) == 1 ) ) {
        flagnum = ( int ) strtol ( numstr, ( char ** ) NULL, 10 );
    }
    if ( ( flagnum < 0 ) || ( flagnum > 9 ) ) {
        cf_log_notfound ( player, cmd, "Not a marker flag", numstr );
        return -1;
    }
    if ( ( fp = letter_to_flag ( *numstr ) ) == NULL ) {
        cf_log_notfound ( player, cmd, "Marker flag", numstr );
        return -1;
    }
    /*
     * Our conditions: The flag name MUST start with an underscore. It
     * must not conflict with the name of any existing flag. There is a
     * KNOWN MEMORY LEAK here -- if you name the flag and rename it
     * later, the old bit of memory for the name won't get freed. This
     * should pretty much never happen, since you're not going to run
     * around arbitrarily giving your flags new names all the time.
     */

    flagstr = XSTRDUP ( tmprintf ( "_%s", namestr ), "cf_flag_name" );
    if ( strlen ( flagstr ) > 31 ) {
        cf_log_syntax ( player, cmd, "Marker flag name too long: %s",
                        namestr );
        XFREE ( flagstr, "cf_flag_name" );
    }
    for ( cp = flagstr; cp && *cp; cp++ ) {
        if ( !isalnum ( *cp ) && ( *cp != '_' ) ) {
            cf_log_syntax ( player, cmd,
                            "Illegal marker flag name: %s", namestr );
            XFREE ( flagstr, "cf_flag_name" );
            return -1;
        }
        *cp = tolower ( *cp );
    }

    if ( hashfind ( flagstr, &mudstate.flags_htab ) ) {
        XFREE ( flagstr, "cf_flag_name" );
        cf_log_syntax ( player, cmd, "Marker flag name in use: %s",
                        namestr );
        return -1;
    }
    for ( cp = flagstr; cp && *cp; cp++ ) {
        *cp = toupper ( *cp );
    }

    fp->flagname = ( const char * ) flagstr;
    hashadd ( ( char * ) fp->flagname, ( int * ) fp, &mudstate.flags_htab, 0 );

    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * convert_flags: convert a list of flag letters into its bit pattern. Also
 * set the type qualifier if specified and not already set.
 */

int convert_flags ( dbref player, char *flaglist, FLAGSET *fset, FLAG *p_type ) {
    int i, handled;

    char *s;

    FLAG flag1mask, flag2mask, flag3mask, type;

    FLAGENT *fp;

    flag1mask = flag2mask = flag3mask = 0;
    type = NOTYPE;

    for ( s = flaglist; *s; s++ ) {
        handled = 0;

        /*
         * Check for object type
         */

        for ( i = 0; ( i <= 7 ) && !handled; i++ ) {
            if ( ( object_types[i].lett == *s ) &&
                    ! ( ( ( object_types[i].perm & CA_WIZARD ) &&
                          !Wizard ( player ) ) ||
                        ( ( object_types[i].perm & CA_GOD ) &&
                          !God ( player ) ) ) ) {
                if ( ( type != NOTYPE ) && ( type != i ) ) {
                    notify ( player,
                             tmprintf
                             ( "%c: Conflicting type specifications.",
                               *s ) );
                    return 0;
                }
                type = i;
                handled = 1;
            }
        }

        /*
         * Check generic flags
         */

        if ( handled ) {
            continue;
        }
        for ( fp = gen_flags; ( fp->flagname ) && !handled; fp++ ) {
            if ( ( fp->flaglett == *s ) &&
                    ! ( ( ( fp->listperm & CA_WIZARD ) &&
                          !Wizard ( player ) ) ||
                        ( ( fp->listperm & CA_GOD ) && !God ( player ) ) ) ) {
                if ( fp->flagflag & FLAG_WORD3 ) {
                    flag3mask |= fp->flagvalue;
                } else if ( fp->flagflag & FLAG_WORD2 ) {
                    flag2mask |= fp->flagvalue;
                } else {
                    flag1mask |= fp->flagvalue;
                }
                handled = 1;
            }
        }

        if ( !handled ) {
            notify ( player,
                     tmprintf
                     ( "%c: Flag unknown or not valid for specified object type",
                       *s ) );
            return 0;
        }
    }

    /*
     * return flags to search for and type
     */

    ( *fset ).word1 = flag1mask;
    ( *fset ).word2 = flag2mask;
    ( *fset ).word3 = flag3mask;
    *p_type = type;
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * decompile_flags: Produce commands to set flags on target.
 */

void decompile_flags ( dbref player, dbref thing, char *thingname ) {
    FLAG f1, f2, f3;

    FLAGENT *fp;

    /*
     * Report generic flags
     */

    f1 = Flags ( thing );
    f2 = Flags2 ( thing );
    f3 = Flags3 ( thing );

    for ( fp = gen_flags; fp->flagname; fp++ ) {

        /*
         * Skip if we shouldn't decompile this flag
         */

        if ( fp->listperm & CA_NO_DECOMP ) {
            continue;
        }

        /*
         * Skip if this flag is not set
         */

        if ( fp->flagflag & FLAG_WORD3 ) {
            if ( ! ( f3 & fp->flagvalue ) ) {
                continue;
            }
        } else if ( fp->flagflag & FLAG_WORD2 ) {
            if ( ! ( f2 & fp->flagvalue ) ) {
                continue;
            }
        } else {
            if ( ! ( f1 & fp->flagvalue ) ) {
                continue;
            }
        }

        /*
         * Skip if we can't see this flag
         */

        if ( !check_access ( player, fp->listperm ) ) {
            continue;
        }

        /*
         * We made it this far, report this flag
         */

        notify ( player, tmprintf ( "@set %s=%s", strip_ansi ( thingname ),
                                    fp->flagname ) );
    }
}
