/**
 * @file constants.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Global numeric and string constants for flags, limits, and parser tokens
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include <float.h>

#define HBUF_SIZE 32768 /*!< Huge buffer size */
#define LBUF_SIZE 8192  /*!< Large buffer size */
#define GBUF_SIZE 1024  /*!< Generic buffer size */
#define MBUF_SIZE 512   /*!< Standard buffer size */
#define SBUF_SIZE 64    /*!< Small buffer size */

#define XMAGIC 0x00deadbeefbaad00 /*!< __XMALLOC magic ID */

/**
 * @brief Maximum iterations for list/exit traversal in match.c
 *
 * Prevents infinite loops when traversing potentially circular
 * linked lists in the database (contents, exits, parent chains).
 */
#define MAX_MATCH_ITERATIONS 10000

/**
 * @brief Attributes constants
 *
 */

/**
 * @brief Allocate space for strings in lumps this big.
 */
#define STRINGBLOCK 1000

/**
 * @brief Flags's attribute
 *
 */
#define AF_ODARK 0x00000001     /*!< players other than owner can't see it */
#define AF_DARK 0x00000002      /*!< No one can see it */
#define AF_WIZARD 0x00000004    /*!< only wizards can change it */
#define AF_MDARK 0x00000008     /*!< Only wizards can see it. Dark to mortals */
#define AF_INTERNAL 0x00000010  /*!< Don't show even to #1 */
#define AF_NOCMD 0x00000020     /*!< Don't create a @ command for it */
#define AF_LOCK 0x00000040      /*!< Attribute is locked */
#define AF_DELETED 0x00000080   /*!< Attribute should be ignored */
#define AF_NOPROG 0x00000100    /*!< Don't process $-commands from this attr */
#define AF_GOD 0x00000200       /*!< Only #1 can change it */
#define AF_IS_LOCK 0x00000400   /*!< Attribute is a lock */
#define AF_VISUAL 0x00000800    /*!< Anyone can see */
#define AF_PRIVATE 0x00001000   /*!< Not inherited by children */
#define AF_HTML 0x00002000      /*!< Don't HTML escape this in did_it() */
#define AF_NOPARSE 0x00004000   /*!< Don't evaluate when checking for $-cmds */
#define AF_REGEXP 0x00008000    /*!< Do a regexp rather than wildcard match */
#define AF_NOCLONE 0x00010000   /*!< Don't copy this attr when cloning. */
#define AF_CONST 0x00020000     /*!< No one can change it (set by server) */
#define AF_CASE 0x00040000      /*!< Regexp matches are case-sensitive */
#define AF_STRUCTURE 0x00080000 /*!< Attribute contains a structure */
#define AF_DIRTY 0x00100000     /*!< Attribute number has been modified */
#define AF_DEFAULT 0x00200000   /*!< did_it() checks attr_defaults obj */
#define AF_NONAME 0x00400000    /*!< If used as oattr, no name prepend */
#define AF_RMATCH 0x00800000    /*!< Set the result of match into regs */
#define AF_NOW 0x01000000       /*!< execute match immediately */
#define AF_TRACE 0x02000000     /*!< trace ufunction */
#define AF_FREE_1 0x04000000    /*!< Reserved for futur use */
#define AF_FREE_2 0x08000000    /*!< Reserved for futur use */
#define AF_FREE_3 0x10000000    /*!< Reserved for futur use */
#define AF_FREE_4 0x20000000    /*!< Reserved for futur use */
#define AF_FREE_5 0x40000000    /*!< Reserved for futur use */
#define AF_FREE_6 0x80000000    /*!< Reserved for futur use */

/**
 * @brief General's attributes.
 *
 */
#define A_NULL 0          /*!< Nothing */
#define A_OSUCC 1         /*!< Others success message */
#define A_OFAIL 2         /*!< Others fail message */
#define A_FAIL 3          /*!< Invoker fail message */
#define A_SUCC 4          /*!< Invoker success message */
#define A_PASS 5          /*!< Password (only meaningful for players) */
#define A_DESC 6          /*!< Description */
#define A_SEX 7           /*!< Sex */
#define A_ODROP 8         /*!< Others drop message */
#define A_DROP 9          /*!< Invoker drop message */
#define A_OKILL 10        /*!< Others kill message */
#define A_KILL 11         /*!< Invoker kill message */
#define A_ASUCC 12        /*!< Success action list */
#define A_AFAIL 13        /*!< Failure action list */
#define A_ADROP 14        /*!< Drop action list */
#define A_AKILL 15        /*!< Kill action list */
#define A_AUSE 16         /*!< Use action list */
#define A_CHARGES 17      /*!< Number of charges remaining */
#define A_RUNOUT 18       /*!< Actions done when no more charges */
#define A_STARTUP 19      /*!< Actions run when game started up */
#define A_ACLONE 20       /*!< Actions run when obj is cloned */
#define A_APAY 21         /*!< Actions run when given COST pennies */
#define A_OPAY 22         /*!< Others pay message */
#define A_PAY 23          /*!< Invoker pay message */
#define A_COST 24         /*!< Number of pennies needed to invoke xPAY */
#define A_MONEY 25        /*!< Value or Wealth (internal) */
#define A_LISTEN 26       /*!< (Wildcarded) string to listen for */
#define A_AAHEAR 27       /*!< Actions to do when anyone says LISTEN str */
#define A_AMHEAR 28       /*!< Actions to do when I say LISTEN str */
#define A_AHEAR 29        /*!< Actions to do when others say LISTEN str */
#define A_LAST 30         /*!< Date/time of last login (players only) */
#define A_QUEUEMAX 31     /*!< Max. # of entries obj has in the queue */
#define A_IDESC 32        /*!< Inside description (ENTER to get inside) */
#define A_ENTER 33        /*!< Invoker enter message */
#define A_OXENTER 34      /*!< Others enter message in dest */
#define A_AENTER 35       /*!< Enter action list */
#define A_ADESC 36        /*!< Describe action list */
#define A_ODESC 37        /*!< Others describe message */
#define A_RQUOTA 38       /*!< Relative object quota */
#define A_ACONNECT 39     /*!< Actions run when player connects */
#define A_ADISCONNECT 40  /*!< Actions run when player disconnects */
#define A_ALLOWANCE 41    /*!< Daily allowance, if diff from default */
#define A_LOCK 42         /*!< Object lock */
#define A_NAME 43         /*!< Object name */
#define A_COMMENT 44      /*!< Wizard-accessible comments */
#define A_USE 45          /*!< Invoker use message */
#define A_OUSE 46         /*!< Others use message */
#define A_SEMAPHORE 47    /*!< Semaphore control info */
#define A_TIMEOUT 48      /*!< Per-user disconnect timeout */
#define A_QUOTA 49        /*!< Absolute quota (to speed up @quota) */
#define A_LEAVE 50        /*!< Invoker leave message */
#define A_OLEAVE 51       /*!< Others leave message in src */
#define A_ALEAVE 52       /*!< Leave action list */
#define A_OENTER 53       /*!< Others enter message in src */
#define A_OXLEAVE 54      /*!< Others leave message in dest */
#define A_MOVE 55         /*!< Invoker move message */
#define A_OMOVE 56        /*!< Others move message */
#define A_AMOVE 57        /*!< Move action list */
#define A_ALIAS 58        /*!< Alias for player names */
#define A_LENTER 59       /*!< ENTER lock */
#define A_LLEAVE 60       /*!< LEAVE lock */
#define A_LPAGE 61        /*!< PAGE lock */
#define A_LUSE 62         /*!< USE lock */
#define A_LGIVE 63        /*!< Give lock (who may give me away?) */
#define A_EALIAS 64       /*!< Alternate names for ENTER */
#define A_LALIAS 65       /*!< Alternate names for LEAVE */
#define A_EFAIL 66        /*!< Invoker entry fail message */
#define A_OEFAIL 67       /*!< Others entry fail message */
#define A_AEFAIL 68       /*!< Entry fail action list */
#define A_LFAIL 69        /*!< Invoker leave fail message */
#define A_OLFAIL 70       /*!< Others leave fail message */
#define A_ALFAIL 71       /*!< Leave fail action list */
#define A_REJECT 72       /*!< Rejected page return message */
#define A_AWAY 73         /*!< Not_connected page return message */
#define A_IDLE 74         /*!< Success page return message */
#define A_UFAIL 75        /*!< Invoker use fail message */
#define A_OUFAIL 76       /*!< Others use fail message */
#define A_AUFAIL 77       /*!< Use fail action list */
#define A_FREE78 78       /*!< Unused, Formerly A_PFAIL: Invoker page fail message */
#define A_TPORT 79        /*!< Invoker teleport message */
#define A_OTPORT 80       /*!< Others teleport message in src */
#define A_OXTPORT 81      /*!< Others teleport message in dst */
#define A_ATPORT 82       /*!< Teleport action list */
#define A_FREE83 83       /*!< Unused, Formerly A_PRIVS: Individual permissions */
#define A_LOGINDATA 84    /*!< Recent login information */
#define A_LTPORT 85       /*!< Teleport lock (can others @tel to me?) */
#define A_LDROP 86        /*!< Drop lock (can I be dropped or @tel'ed) */
#define A_LRECEIVE 87     /*!< Receive lock (who may give me things?) */
#define A_LASTSITE 88     /*!< Last site logged in from, in cleartext */
#define A_INPREFIX 89     /*!< Prefix on incoming messages into objects */
#define A_PREFIX 90       /*!< Prefix used by exits/objects when audible */
#define A_INFILTER 91     /*!< Filter to zap incoming text into objects */
#define A_FILTER 92       /*!< Filter to zap text forwarded by audible. */
#define A_LLINK 93        /*!< Who may link to here */
#define A_LTELOUT 94      /*!< Who may teleport out from here */
#define A_FORWARDLIST 95  /*!< Recipients of AUDIBLE output */
#define A_MAILFOLDERS 96  /*!< @mail folders */
#define A_LUSER 97        /*!< Spare lock not referenced by server */
#define A_LPARENT 98      /*!< Who may @parent to me if PARENT_OK set */
#define A_LCONTROL 99     /*!< Who controls me if CONTROL_OK set */
#define A_VA 100          /*!< VA-Z attribute */
#define A_VB 101          /*!< VA-Z attribute */
#define A_VC 102          /*!< VA-Z attribute */
#define A_VD 103          /*!< VA-Z attribute */
#define A_VE 104          /*!< VA-Z attribute */
#define A_VF 105          /*!< VA-Z attribute */
#define A_VG 106          /*!< VA-Z attribute */
#define A_VH 107          /*!< VA-Z attribute */
#define A_VI 108          /*!< VA-Z attribute */
#define A_VJ 109          /*!< VA-Z attribute */
#define A_VK 110          /*!< VA-Z attribute */
#define A_VL 111          /*!< VA-Z attribute */
#define A_VM 112          /*!< VA-Z attribute */
#define A_VN 113          /*!< VA-Z attribute */
#define A_VO 114          /*!< VA-Z attribute */
#define A_VP 115          /*!< VA-Z attribute */
#define A_VQ 116          /*!< VA-Z attribute */
#define A_VR 117          /*!< VA-Z attribute */
#define A_VS 118          /*!< VA-Z attribute */
#define A_VT 119          /*!< VA-Z attribute */
#define A_VU 120          /*!< VA-Z attribute */
#define A_VV 121          /*!< VA-Z attribute */
#define A_VW 122          /*!< VA-Z attribute */
#define A_VX 123          /*!< VA-Z attribute */
#define A_VY 124          /*!< VA-Z attribute */
#define A_VZ 125          /*!< VA-Z attribute */
#define A_FREE126 126     /*!< Unused */
#define A_FREE127 127     /*!< Unused */
#define A_FREE128 128     /*!< Unused */
#define A_GFAIL 129       /*!< Give fail message */
#define A_OGFAIL 130      /*!< Others give fail message */
#define A_AGFAIL 131      /*!< Give fail action */
#define A_RFAIL 132       /*!< Receive fail message */
#define A_ORFAIL 133      /*!< Others receive fail message */
#define A_ARFAIL 134      /*!< Receive fail action */
#define A_DFAIL 135       /*!< Drop fail message */
#define A_ODFAIL 136      /*!< Others drop fail message */
#define A_ADFAIL 137      /*!< Drop fail action */
#define A_TFAIL 138       /*!< Teleport (to) fail message */
#define A_OTFAIL 139      /*!< Others teleport (to) fail message */
#define A_ATFAIL 140      /*!< Teleport fail action */
#define A_TOFAIL 141      /*!< Teleport (from) fail message */
#define A_OTOFAIL 142     /*!< Others teleport (from) fail message */
#define A_ATOFAIL 143     /*!< Teleport (from) fail action */
#define A_FREE144 144     /*!< Unused */
#define A_FREE145 145     /*!< Unused */
#define A_FREE146 146     /*!< Unused */
#define A_FREE147 147     /*!< Unused */
#define A_FREE148 148     /*!< Unused */
#define A_FREE149 149     /*!< Unused */
#define A_FREE150 150     /*!< Unused */
#define A_FREE151 151     /*!< Unused */
#define A_FREE152 152     /*!< Unused */
#define A_FREE153 153     /*!< Unused */
#define A_FREE154 154     /*!< Unused */
#define A_FREE155 155     /*!< Unused */
#define A_FREE156 156     /*!< Unused */
#define A_FREE157 157     /*!< Unused */
#define A_FREE158 158     /*!< Unused */
#define A_FREE159 159     /*!< Unused */
#define A_FREE160 160     /*!< Unused */
#define A_FREE161 161     /*!< Unused */
#define A_FREE162 162     /*!< Unused */
#define A_FREE163 163     /*!< Unused */
#define A_FREE164 164     /*!< Unused */
#define A_FREE165 165     /*!< Unused */
#define A_FREE166 166     /*!< Unused */
#define A_FREE167 167     /*!< Unused */
#define A_FREE168 168     /*!< Unused */
#define A_FREE169 169     /*!< Unused */
#define A_FREE170 170     /*!< Unused */
#define A_FREE171 171     /*!< Unused */
#define A_FREE172 172     /*!< Unused */
#define A_FREE173 173     /*!< Unused */
#define A_FREE174 174     /*!< Unused */
#define A_FREE175 175     /*!< Unused */
#define A_FREE176 176     /*!< Unused */
#define A_FREE177 177     /*!< Unused */
#define A_FREE178 178     /*!< Unused */
#define A_FREE179 179     /*!< Unused */
#define A_FREE180 180     /*!< Unused */
#define A_FREE181 181     /*!< Unused */
#define A_FREE182 182     /*!< Unused */
#define A_FREE183 183     /*!< Unused */
#define A_FREE184 184     /*!< Unused */
#define A_FREE185 185     /*!< Unused */
#define A_FREE186 186     /*!< Unused */
#define A_FREE187 187     /*!< Unused */
#define A_FREE188 188     /*!< Unused */
#define A_FREE189 189     /*!< Unused */
#define A_FREE190 190     /*!< Unused */
#define A_FREE191 191     /*!< Unused */
#define A_FREE192 192     /*!< Unused */
#define A_FREE193 193     /*!< Unused */
#define A_FREE194 194     /*!< Unused */
#define A_FREE195 195     /*!< Unused */
#define A_FREE196 196     /*!< Unused */
#define A_FREE197 197     /*!< Unused */
#define A_MAILCC 198      /*!< Who is the mail Cc'ed to? */
#define A_MAILBCC 199     /*!< Who is the mail Bcc'ed to? */
#define A_LASTPAGE 200    /*!< Player last paged */
#define A_MAIL 201        /*!< Message echoed to sender */
#define A_AMAIL 202       /*!< Action taken when mail received */
#define A_SIGNATURE 203   /*!< Mail signature */
#define A_DAILY 204       /*!< Daily attribute to be executed */
#define A_MAILTO 205      /*!< Who is the mail to? */
#define A_MAILMSG 206     /*!< The mail message itself */
#define A_MAILSUB 207     /*!< The mail subject */
#define A_MAILCURF 208    /*!< The current @mail folder */
#define A_LSPEECH 209     /*!< Speechlocks */
#define A_PROGCMD 210     /*!< Command for execution by @prog */
#define A_MAILFLAGS 211   /*!< Flags for extended mail */
#define A_DESTROYER 212   /*!< Who is destroying this object? */
#define A_NEWOBJS 213     /*!< New object array */
#define A_LCON_FMT 214    /*!< Player-specified contents format */
#define A_LEXITS_FMT 215  /*!< Player-specified exits format */
#define A_EXITVARDEST 216 /*!< Variable exit destination */
#define A_LCHOWN 217      /*!< ChownLock */
#define A_LASTIP 218      /*!< Last IP address logged in from */
#define A_LDARK 219       /*!< DarkLock */
#define A_VRML_URL 220    /*!< URL of the VRML scene for this object */
#define A_HTDESC 221      /*!< HTML @desc */
#define A_NAME_FMT 222    /*!< Player-specified name format */
#define A_LKNOWN 223      /*!< Who is this player seen by? (presence) */
#define A_LHEARD 224      /*!< Who is this player heard by? (speech) */
#define A_LMOVED 225      /*!< Who notices this player moving? */
#define A_LKNOWS 226      /*!< Who does this player see? (presence) */
#define A_LHEARS 227      /*!< Who does this player hear? (speech) */
#define A_LMOVES 228      /*!< Who does this player notice moving? */
#define A_SPEECHFMT 229   /*!< Format speech */
#define A_PAGEGROUP 230   /*!< Last paged as part of this group */
#define A_PROPDIR 231     /*!< Property directory dbref list */
#define A_FREE232 232     /*!< Unused */
#define A_FREE233 233     /*!< Unused */
#define A_FREE234 234     /*!< Unused */
#define A_FREE235 235     /*!< Unused */
#define A_FREE236 236     /*!< Unused */
#define A_FREE237 237     /*!< Unused */
#define A_FREE238 238     /*!< Unused */
#define A_FREE239 239     /*!< Unused */
#define A_FREE240 240     /*!< Unused */
#define A_FREE241 241     /*!< Unused */
#define A_FREE242 242     /*!< Unused */
#define A_FREE243 243     /*!< Unused */
#define A_FREE244 244     /*!< Unused */
#define A_FREE245 245     /*!< Unused */
#define A_FREE246 246     /*!< Unused */
#define A_FREE247 247     /*!< Unused */
#define A_FREE248 248     /*!< Unused */
#define A_FREE249 249     /*!< Unused */
#define A_FREE250 250     /*!< Unused */
#define A_FREE251 251     /*!< Unused */
#define A_FREE252 252     /*!< Unused Formerly A_VLIST */
#define A_LIST 253        /*!< A_VLIST */
#define A_FREE254 254     /*!< Unused Formerly A_STRUCT */
#define A_TEMP 255        /*!< Temporary */
#define A_USER_START 256  /*!< Start of user-named attributes */
#define ATR_BUF_CHUNK 100 /*!< Min size to allocate for attribute buffer */
#define ATR_BUF_INCR 6    /*!< Max size of one attribute */

/**
 * @brief Commands constants
 *
 */

#define NOGO_MESSAGE "You can't go that way."

/**
 * @brief Command handler call conventions
 *
 */
#define CS_NO_ARGS 0x00000      /*!< No arguments */
#define CS_ONE_ARG 0x00001      /*!< One argument */
#define CS_TWO_ARG 0x00002      /*!< Two arguments */
#define CS_NARG_MASK 0x00003    /*!< Argument count mask */
#define CS_ARGV 0x00004         /*!< ARG2 is in ARGV form */
#define CS_INTERP 0x00010       /*!< Interpret ARG2 if 2 args, ARG1 if 1 */
#define CS_NOINTERP 0x00020     /*!< Never interp ARG2 if 2 or ARG1 if 1 */
#define CS_CAUSE 0x00040        /*!< Pass cause to old command handler */
#define CS_UNPARSE 0x00080      /*!< Pass unparsed cmd to old-style handler */
#define CS_CMDARG 0x00100       /*!< Pass in given command args */
#define CS_STRIP 0x00200        /*!< Strip braces even when not interpreting */
#define CS_STRIP_AROUND 0x00400 /*!< Strip braces around entire string only */
#define CS_ADDED 0x00800        /*!< Command has been added by @addcommand */
#define CS_LEADIN 0x01000       /*!< Command is a single-letter lead-in */
#define CS_PRESERVE 0x02000     /*!< For hooks, preserve global registers */
#define CS_NOSQUISH 0x04000     /*!< Do not space-compress */
#define CS_FUNCTION 0x08000     /*!< Can call with command() */
#define CS_ACTOR 0x10000        /*!< @addcommand executed by player, not obj */
#define CS_PRIVATE 0x20000      /*!< For hooks, use private global registers */

/**
 * @brief Command permission flags
 *
 */
#define CA_PUBLIC 0x00000000    /*!< No access restrictions */
#define CA_GOD 0x00000001       /*!< GOD only... */
#define CA_WIZARD 0x00000002    /*!< Wizards only */
#define CA_BUILDER 0x00000004   /*!< Builders only */
#define CA_IMMORTAL 0x00000008  /*!< Immortals only */
#define CA_STAFF 0x00000010     /*!< Must have STAFF flag */
#define CA_HEAD 0x00000020      /*!< Must have HEAD flag */
#define CA_MODULE_OK 0x00000040 /*!< Must have MODULE_OK power */
#define CA_ADMIN 0x00000080     /*!< Wizard or royal */

#define CA_ISPRIV_MASK (CA_GOD | CA_WIZARD | CA_BUILDER | CA_IMMORTAL | CA_STAFF | CA_HEAD | CA_ADMIN | CA_MODULE_OK)

#define CA_NO_HAVEN 0x00000100   /*!< Not by HAVEN players */
#define CA_NO_ROBOT 0x00000200   /*!< Not by ROBOT players */
#define CA_NO_SLAVE 0x00000400   /*!< Not by SLAVE players */
#define CA_NO_SUSPECT 0x00000800 /*!< Not by SUSPECT players */
#define CA_NO_GUEST 0x00001000   /*!< Not by GUEST players */

#define CA_ISNOT_MASK (CA_NO_HAVEN | CA_NO_ROBOT | CA_NO_SLAVE | CA_NO_SUSPECT | CA_NO_GUEST)

#define CA_MARKER0 0x00002000
#define CA_MARKER1 0x00004000
#define CA_MARKER2 0x00008000
#define CA_MARKER3 0x00010000
#define CA_MARKER4 0x00020000
#define CA_MARKER5 0x00040000
#define CA_MARKER6 0x00080000
#define CA_MARKER7 0x00100000
#define CA_MARKER8 0x00200000
#define CA_MARKER9 0x00400000

#define CA_MARKER_MASK (CA_MARKER0 | CA_MARKER1 | CA_MARKER2 | CA_MARKER3 | CA_MARKER4 | CA_MARKER5 | CA_MARKER6 | CA_MARKER7 | CA_MARKER8 | CA_MARKER9)

#define CA_GBL_BUILD 0x00800000  /*!< Requires the global BUILDING flag */
#define CA_GBL_INTERP 0x01000000 /*!< Requires the global INTERP flag */
#define CA_DISABLED 0x02000000   /*!< Command completely disabled */
#define CA_STATIC 0x04000000     /*!< Cannot be changed at runtime */
#define CA_NO_DECOMP 0x08000000  /*!< Don't include in @decompile */

#define CA_LOCATION 0x10000000 /*!< Invoker must have location */
#define CA_CONTENTS 0x20000000 /*!< Invoker must have contents */
#define CA_PLAYER 0x40000000   /*!< Invoker must be a player */
#define CF_DARK 0x80000000     /*!< Command doesn't show up in list */

#define SW_MULTIPLE 0x80000000   /*!< This sw may be spec'd w/others */
#define SW_GOT_UNIQUE 0x40000000 /*!< Already have a unique option */
#define SW_NOEVAL 0x20000000     /*!< Don't parse args before calling handler (typically via a switch alias) */

#define LIST_ATTRIBUTES 1
#define LIST_COMMANDS 2
#define LIST_COSTS 3
#define LIST_FLAGS 4
#define LIST_FUNCTIONS 5
#define LIST_GLOBALS 6
#define LIST_ALLOCATOR 7
#define LIST_LOGGING 8
#define LIST_DF_FLAGS 9
#define LIST_PERMS 10
#define LIST_ATTRPERMS 11
#define LIST_OPTIONS 12
#define LIST_HASHSTATS 13
#define LIST_BUFTRACE 14
#define LIST_CONF_PERMS 15
#define LIST_SITEINFO 16
#define LIST_POWERS 17
#define LIST_SWITCHES 18
#define LIST_VATTRS 19
#define LIST_DB_STATS 20
#define LIST_PROCESS 21
#define LIST_BADNAMES 22
#define LIST_CACHEOBJS 23
#define LIST_TEXTFILES 24
#define LIST_PARAMS 25
#define LIST_CF_RPERMS 26
#define LIST_ATTRTYPES 27
#define LIST_FUNCPERMS 28
#define LIST_MEMORY 29
#define LIST_CACHEATTRS 30
#define LIST_RAWMEM 31

/**
 * @brief Database constants
 *
 */

#define RS_CONCENTRATE 0x00000002    /*!< Restart DB has Port-Concentrator informations */
#define RS_RECORD_PLAYERS 0x00000004 /*!< Restart DB has Record of connected players */
#define RS_NEW_STRINGS 0x00000008    /*!< Restart DB has new strings informations */
#define RS_COUNT_REBOOTS 0x00000010  /*!< Restart DB has reboot count */

#define HANDLE_FLAT_CRASH 1
#define HANDLE_FLAT_KILL 2

/**
 * Database R/W flags.
 *
 */
#define OUTPUT_VERSION 1 /*!< Version 1 */
#define UNLOAD_VERSION 1 /*!< version for export */

/**
 * @brief Leadin char for attr control data
 *
 */
#define ATR_INFO_CHAR '\1'

/**
 * @brief Boolean expressions, for locks
 *
 */
#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_ATR 4
#define BOOLEXP_INDIR 5
#define BOOLEXP_CARRY 6
#define BOOLEXP_IS 7
#define BOOLEXP_OWNER 8
#define BOOLEXP_EVAL 9

/**
 * @briefBoolexp decompile formats
 *
 */
#define F_EXAMINE 1   /** Normal */
#define F_QUIET 2     /** Binary for db dumps */
#define F_DECOMPILE 3 /** @decompile output */
#define F_FUNCTION 4  /** [lock()] output */

/**
 * @brief Database format information
 *
 */
#define F_UNKNOWN 0  /*!< Unknown database format */
#define F_MUSH 1     /*!< MUSH format (many variants) */
#define F_MUSE 2     /*!< MUSE format */
#define F_MUD 3      /*!< Old TinyMUD format */
#define F_MUCK 4     /*!< TinyMUCK format */
#define F_MUX 5      /*!< TinyMUX format */
#define F_TINYMUSH 6 /*!< TinyMUSH 3.0 format */

#define V_MASK 0x000000ff        /*!< Database version */
#define V_ZONE 0x00000100        /*!< ZONE/DOMAIN field */
#define V_LINK 0x00000200        /*!< LINK field (exits from objs) */
#define V_GDBM 0x00000400        /*!< attrs are in a gdbm db, not here */
#define V_ATRNAME 0x00000800     /*!< NAME is an attr, not in the hdr */
#define V_ATRKEY 0x00001000      /*!< KEY is an attr, not in the hdr */
#define V_PERNKEY 0x00001000     /*!< PERN: Extra locks in object hdr */
#define V_PARENT 0x00002000      /*!< db has the PARENT field */
#define V_COMM 0x00004000        /*!< PERN: Comm status in header */
#define V_ATRMONEY 0x00008000    /*!< Money is kept in an attribute */
#define V_XFLAGS 0x00010000      /*!< An extra word of flags */
#define V_POWERS 0x00020000      /*!< Powers? */
#define V_3FLAGS 0x00040000      /*!< Adding a 3rd flag word */
#define V_QUOTED 0x00080000      /*!< Quoted strings, ala PennMUSH */
#define V_TQUOTAS 0x00100000     /*!< Typed quotas */
#define V_TIMESTAMPS 0x00200000  /*!< Timestamps */
#define V_VISUALATTRS 0x00400000 /*!< ODark-to-Visual attr flags */
#define V_CREATETIME 0x00800000  /*!< Create time */
#define V_DBCLEAN 0x80000000     /*!< Option to clean attr table */

/* special dbref's */
#define NOTHING -1   /*!< null dbref */
#define AMBIGUOUS -2 /*!< multiple possibilities, for matchers */
#define HOME -3      /*!< virtual room, represents mover's home */
#define NOPERM -4    /*!< Error status, no permission */
#define ANY_OWNER -2 /*!< multiple possibilities, for owner */

/**
 * Command handler keys
 *
 */
#define ADDCMD_PRESERVE 1       /*!< Use player rather than addcommand thing */
#define ATTRIB_ACCESS 1         /*!< Change access to attribute */
#define ATTRIB_RENAME 2         /*!< Rename attribute */
#define ATTRIB_DELETE 4         /*!< Delete attribute */
#define ATTRIB_INFO 8           /*!< Info (number, flags) about attribute */
#define BOOT_QUIET 1            /*!< Inhibit boot message to victim */
#define BOOT_PORT 2             /*!< Boot by port number */
#define CHOWN_ONE 1             /*!< item = new_owner */
#define CHOWN_ALL 2             /*!< old_owner = new_owner */
#define CHOWN_NOSTRIP 4         /*!< Don't strip (most) flags from object */
#define CHZONE_NOSTRIP 1        /*!< Don't strip (most) flags from object */
#define CLONE_LOCATION 0        /*!< Create cloned object in my location */
#define CLONE_INHERIT 1         /*!< Keep INHERIT bit if set */
#define CLONE_PRESERVE 2        /*!< Preserve the owner of the object */
#define CLONE_INVENTORY 4       /*!< Create cloned object in my inventory */
#define CLONE_SET_COST 8        /*!< ARG2 is cost of cloned object */
#define CLONE_FROM_PARENT 16    /*!< Set parent instead of cloning attrs */
#define CLONE_NOSTRIP 32        /*!< Don't strip (most) flags from clone */
#define DBCK_FULL 1             /*!< Do all tests */
#define DECOMP_PRETTY 1         /*!< pretty-format output */
#define DEST_ONE 1              /*!< object */
#define DEST_ALL 2              /*!< owner */
#define DEST_OVERRIDE 4         /*!< override Safe() */
#define DEST_INSTANT 8          /*!< instantly destroy */
#define DIG_TELEPORT 1          /*!< teleport to room after @digging */
#define DOLIST_SPACE 0          /*!< expect spaces as delimiter */
#define DOLIST_DELIMIT 1        /*!< expect custom delimiter */
#define DOLIST_NOTIFY 2         /*!< queue a '@notify me' at the end */
#define DOLIST_NOW 4            /*!< Run commands immediately, no queueing */
#define DOING_MESSAGE 0         /*!< Set my DOING message */
#define DOING_HEADER 1          /*!< Set the DOING header */
#define DOING_POLL 2            /*!< List DOING header */
#define DOING_QUIET 4           /*!< Inhibit 'Set.' message */
#define DROP_QUIET 1            /*!< Don't do Odrop/Adrop if control */
#define DUMP_STRUCT 1           /*!< Dump flat structure file */
#define DUMP_TEXT 2             /*!< Dump text (gdbm) file */
#define DUMP_FLATFILE 8         /*!< Dump to flatfile */
#define DUMP_OPTIMIZE 16        /*!< Optimized dump */
#define ENDCMD_BREAK 0          /*!< @break - end action list on true */
#define ENDCMD_ASSERT 1         /*!< @assert - end action list on false */
#define EXAM_DEFAULT 0          /*!< Default */
#define EXAM_BRIEF 1            /*!< Don't show attributes */
#define EXAM_LONG 2             /*!< Nonowner sees public attrs too */
#define EXAM_DEBUG 4            /*!< Display more info for finding db problems */
#define EXAM_PARENT 8           /*!< Get attr from parent when exam obj/attr */
#define EXAM_PRETTY 16          /*!< Pretty-format output */
#define EXAM_PAIRS 32           /*!< Print paren matches in color */
#define EXAM_OWNER 64           /*!< Nonowner sees just owner */
#define FIXDB_OWNER 1           /*!< Fix OWNER field */
#define FIXDB_LOC 2             /*!< Fix LOCATION field */
#define FIXDB_CON 4             /*!< Fix CONTENTS field */
#define FIXDB_EXITS 8           /*!< Fix EXITS field */
#define FIXDB_NEXT 16           /*!< Fix NEXT field */
#define FIXDB_PENNIES 32        /*!< Fix PENNIES field */
#define FIXDB_NAME 64           /*!< Set NAME attribute */
#define FLOATERS_ALL 1          /*!< Display all floating rooms in db */
#define FUNCT_LIST 1            /*!< List the user-defined functions */
#define FUNCT_NO_EVAL 2         /*!< Don't evaluate args to function */
#define FUNCT_PRIV 4            /*!< Perform ufun as holding obj */
#define FUNCT_PRES 8            /*!< Preserve r-regs before ufun */
#define FUNCT_NOREGS 16         /*!< Private r-regs for ufun */
#define FRC_COMMAND 1           /*!< what=command */
#define FRC_NOW 2               /*!< run command immediately, no queueing */
#define GET_QUIET 1             /*!< Don't do osucc/asucc if control */
#define GIVE_QUIET 1            /*!< Inhibit give messages */
#define GLOB_ENABLE 1           /*!< key to enable */
#define GLOB_DISABLE 2          /*!< key to disable */
#define HALT_ALL 1              /*!< halt everything */
#define HALT_PID 2              /*!< halt a particular PID */
#define HELP_FIND 1             /*!< do a wildcard search through help subjects */
#define HELP_RAWHELP 0x08000000 /*!< A high bit. Don't eval text. */
#define HOOK_BEFORE 1           /*!< pre-command hook */
#define HOOK_AFTER 2            /*!< post-command hook */
#define HOOK_PRESERVE 4         /*!< preserve global regs */
#define HOOK_NOPRESERVE 8       /*!< don't preserve global regs */
#define HOOK_PERMIT 16          /*!< user-defined permissions */
#define HOOK_PRIVATE 32         /*!< private global regs */
#define KILL_KILL 1             /*!< gives victim insurance */
#define KILL_SLAY 2             /*!< no insurance */
#define LOOK_LOOK 1             /*!< list desc (and succ/fail if room) */
#define LOOK_INVENTORY 2        /*!< list inventory of object */
#define LOOK_SCORE 4            /*!< list score (# coins) */
#define LOOK_OUTSIDE 8          /*!< look for object in container of player */
#define MARK_SET 0              /*!< Set mark bits */
#define MARK_CLEAR 1            /*!< Clear mark bits */
#define MOTD_ALL 0              /*!< login message for all */
#define MOTD_WIZ 1              /*!< login message for wizards */
#define MOTD_DOWN 2             /*!< login message when logins disabled */
#define MOTD_FULL 4             /*!< login message when too many players on */
#define MOTD_LIST 8             /*!< Display current login messages */
#define MOTD_BRIEF 16           /*!< Suppress motd file display for wizards */
#define MOVE_QUIET 1            /*!< Dont do Osucc/Ofail/Asucc/Afail if ctrl */
#define NFY_NFY 0               /*!< Notify first waiting command */
#define NFY_NFYALL 1            /*!< Notify all waiting commands */
#define NFY_DRAIN 2             /*!< Delete waiting commands */
#define NREF_LIST 1             /*!< List rather than set nrefs */
#define OPEN_LOCATION 0         /*!< Open exit in my location */
#define OPEN_INVENTORY 1        /*!< Open exit in me */
#define PASS_ANY 1              /*!< name=newpass */
#define PASS_MINE 2             /*!< oldpass=newpass */
#define PCRE_PLAYER 1           /*!< create new player */
#define PCRE_ROBOT 2            /*!< create robot player */
#define PEMIT_PEMIT 1           /*!< emit to named player */
#define PEMIT_OEMIT 2           /*!< emit to all in current room except named */
#define PEMIT_WHISPER 3         /*!< whisper to player in current room */
#define PEMIT_FSAY 4            /*!< force controlled obj to say */
#define PEMIT_FEMIT 5           /*!< force controlled obj to emit */
#define PEMIT_FPOSE 6           /*!< force controlled obj to pose */
#define PEMIT_FPOSE_NS 7        /*!< force controlled obj to pose w/o space */
#define PEMIT_CONTENTS 8        /*!< Send to contents (additive) */
#define PEMIT_HERE 16           /*!< Send to location (@femit, additive) */
#define PEMIT_ROOM 32           /*!< Send to containing rm (@femit, additive) */
#define PEMIT_LIST 64           /*!< Send to a list */
#define PEMIT_SPEECH 128        /*!< Explicitly tag this as speech */
#define PEMIT_HTML 256          /*!< HTML escape, and no newline */
#define PEMIT_MOVE 512          /*!< Explicitly tag this as a movement message */
#define PEMIT_SPOOF 1024        /*!< change enactor to target object */
#define PS_BRIEF 0              /*!< Short PS report */
#define PS_LONG 1               /*!< Long PS report */
#define PS_SUMM 2               /*!< Queue counts only */
#define PS_ALL 4                /*!< List entire queue */
#define QUEUE_KICK 1            /*!< Process commands from queue */
#define QUEUE_WARP 2            /*!< Advance or set back wait queue clock */
#define QUOTA_SET 1             /*!< Set a quota */
#define QUOTA_FIX 2             /*!< Repair a quota */
#define QUOTA_TOT 4             /*!< Operate on total quota */
#define QUOTA_REM 8             /*!< Operate on remaining quota */
#define QUOTA_ALL 16            /*!< Operate on all players */
#define QUOTA_ROOM 32           /*!< Room quota set */
#define QUOTA_EXIT 64           /*!< Exit quota set */
#define QUOTA_THING 128         /*!< Thing quota set */
#define QUOTA_PLAYER 256        /*!< Player quota set */
#define SAY_SAY 1               /*!< say in current room */
#define SAY_NOSPACE 1           /*!< OR with xx_EMIT to get nospace form */
#define SAY_POSE 2              /*!< pose in current room */
#define SAY_POSE_NOSPC 3        /*!< pose w/o space in current room */
#define SAY_EMIT 5              /*!< emit in current room */
#define SAY_SHOUT 8             /*!< shout to all logged-in players */
#define SAY_WALLPOSE 9          /*!< Pose to all logged-in players */
#define SAY_WALLEMIT 10         /*!< Emit to all logged-in players */
#define SAY_WIZSHOUT 12         /*!< shout to all logged-in wizards */
#define SAY_WIZPOSE 13          /*!< Pose to all logged-in wizards */
#define SAY_WIZEMIT 14          /*!< Emit to all logged-in wizards */
#define SAY_ADMINSHOUT 15       /*!< Emit to all wizards or royalty */
#define SAY_NOTAG 32            /*!< Don't put Broadcast: in front (additive) */
#define SAY_HERE 64             /*!< Output to current location */
#define SAY_ROOM 128            /*!< Output to containing room */
#define SAY_HTML 256            /*!< Don't output a newline */
#define SAY_PREFIX 512          /*!< first char indicates formatting */
#define SET_QUIET 1             /*!< Don't display 'Set.' message. */
#define SHUTDN_COREDUMP 1       /*!< Produce a coredump */
#define SRCH_SEARCH 1           /*!< Do a normal search */
#define SRCH_MARK 2             /*!< Set mark bit for matches */
#define SRCH_UNMARK 3           /*!< Clear mark bit for matches */
#define STAT_PLAYER 0           /*!< Display stats for one player or tot objs */
#define STAT_ALL 1              /*!< Display global stats */
#define STAT_ME 2               /*!< Display stats for me */
#define SWITCH_DEFAULT 0        /*!< Use the configured default for switch */
#define SWITCH_ANY 1            /*!< Execute all cases that match */
#define SWITCH_ONE 2            /*!< Execute only first case that matches */
#define SWITCH_NOW 4            /*!< Execute case(s) immediately, no queueing */
#define SWEEP_ME 1              /*!< Check my inventory */
#define SWEEP_HERE 2            /*!< Check my location */
#define SWEEP_COMMANDS 4        /*!< Check for $-commands */
#define SWEEP_LISTEN 8          /*!< Check for @listen-ers */
#define SWEEP_PLAYER 16         /*!< Check for players and puppets */
#define SWEEP_CONNECT 32        /*!< Search for connected players/puppets */
#define SWEEP_EXITS 64          /*!< Search the exits for audible flags */
#define SWEEP_VERBOSE 256       /*!< Display what pattern matches */
#define TELEPORT_DEFAULT 1      /*!< Emit all messages */
#define TELEPORT_QUIET 2        /*!< Teleport in quietly */
#define TIMECHK_RESET 1         /*!< Reset all counters to zero */
#define TIMECHK_SCREEN 2        /*!< Write info to screen */
#define TIMECHK_LOG 4           /*!< Write info to log */
#define TOAD_NO_CHOWN 1         /*!< Don't change ownership */
#define TRIG_QUIET 1            /*!< Don't display 'Triggered.' message. */
#define TRIG_NOW 2              /*!< Run immediately, no queueing */
#define TWARP_QUEUE 1           /*!< Warp the wait and sem queues */
#define TWARP_DUMP 2            /*!< Warp the dump interval */
#define TWARP_CLEAN 4           /*!< Warp the cleaning interval */
#define TWARP_IDLE 8            /*!< Warp the idle check interval */
#define TWARP_EMPTY 16          /*!< Not used - Available */
#define TWARP_EVENTS 32         /*!< Warp the events checking interval */
#define VERB_NOW 1              /*!< Run @afoo immediately, no queueing */
#define VERB_MOVE 2             /*!< Treat like movement message */
#define VERB_SPEECH 4           /*!< Treat like speech message */
#define VERB_PRESENT 8          /*!< Treat like presence message */
#define VERB_NONAME 16          /*!< Do not prepend name to odefault */
#define WAIT_UNTIL 1            /*!< Absolute time wait */
#define WAIT_PID 2              /*!< Change the wait on a PID */
/**
 * Hush codes for movement messages
 *
 */
#define HUSH_ENTER 1 /*!< xENTER/xEFAIL */
#define HUSH_LEAVE 2 /*!< xLEAVE/xLFAIL */
#define HUSH_EXIT 4  /*!< xSUCC/xDROP/xFAIL from exits */
/**
 * Evaluation directives
 *
 */
#define EV_FIGNORE 0x00000000      /*!< Don't look for func if () found */
#define EV_FMAND 0x00000100        /*!< Text before () must be func name */
#define EV_FCHECK 0x00000200       /*!< Check text before () for function */
#define EV_STRIP 0x00000400        /*!< Strip one level of brackets */
#define EV_EVAL 0x00000800         /*!< Evaluate results before returning */
#define EV_STRIP_TS 0x00001000     /*!< Strip trailing spaces */
#define EV_STRIP_LS 0x00002000     /*!< Strip leading spaces */
#define EV_STRIP_ESC 0x00004000    /*!< Strip one level of \ characters */
#define EV_STRIP_AROUND 0x00008000 /*!< Strip {} only at ends of string */
#define EV_TOP 0x00010000          /*!< This is a toplevel call to eval() */
#define EV_NOTRACE 0x00020000      /*!< Don't trace this call to eval */
#define EV_NO_COMPRESS 0x00040000  /*!< Don't compress spaces. */
#define EV_NO_LOCATION 0x00080000  /*!< Suppresses %l */
#define EV_NOFCHECK 0x00100000     /*!< Do not evaluate functions! */
/**
 * Function flags
 *
 */
#define FN_VARARGS 0x80000000 /*!< allows variable # of args */
#define FN_NO_EVAL 0x40000000 /*!< Don't evaluate args to function */
#define FN_PRIV 0x20000000    /*!< Perform ufun as holding obj */
#define FN_PRES 0x10000000    /*!< Preserve r-regs before ufun */
#define FN_NOREGS 0x08000000  /*!< Private r-regs for ufun */
#define FN_DBFX 0x04000000    /*!< DB-affecting side effects */
#define FN_QFX 0x02000000     /*!< Queue-affecting side effects */
#define FN_OUTFX 0x01000000   /*!< Output-affecting side effects */
#define FN_STACKFX 0x00800000 /*!< All stack functions */
#define FN_VARFX 0x00400000   /*!< All xvars functions */
/**
 * Lower flag values are used for function-specific switches
 *
 * Message forwarding directives
 *
 */
#define MSG_PUP_ALWAYS 0x00001  /*!< Always forward msg to puppet own */
#define MSG_INV 0x00002         /*!< Forward msg to contents */
#define MSG_INV_L 0x00004       /*!< ... only if msg passes my @listen */
#define MSG_INV_EXITS 0x00008   /*!< Forward through my audible exits */
#define MSG_NBR 0x00010         /*!< Forward msg to neighbors */
#define MSG_NBR_A 0x00020       /*!< ... only if I am audible */
#define MSG_NBR_EXITS 0x00040   /*!< Also forward to neighbor exits */
#define MSG_NBR_EXITS_A 0x00080 /*!< ... only if I am audible */
#define MSG_LOC 0x00100         /*!< Send to my location */
#define MSG_LOC_A 0x00200       /*!< ... only if I am audible */
#define MSG_FWDLIST 0x00400     /*!< Forward to my fwdlist members if audible */
#define MSG_ME 0x00800          /*!< Send to me */
#define MSG_S_INSIDE 0x01000    /*!< Originator is inside target */
#define MSG_S_OUTSIDE 0x02000   /*!< Originator is outside target */
#define MSG_HTML 0x04000        /*!< Don't send \r\n */
#define MSG_SPEECH 0x08000      /*!< This message is speech. */
#define MSG_MOVE 0x10000        /*!< This message is movement. */
#define MSG_PRESENCE 0x20000    /*!< This message is related to presence. */

/**
 * Look primitive directives
 *
 */
#define LK_IDESC 0x0001
#define LK_OBEYTERSE 0x0002
#define LK_SHOWATTR 0x0004
#define LK_SHOWEXIT 0x0008
#define LK_SHOWVRML 0x0010

#define CONTENTS_LOCAL 0
#define CONTENTS_NESTED 1
#define CONTENTS_REMOTE 2

/**
 * Match related
 *
 */

#define CON_LOCAL 0x01    /* Match is near me */
#define CON_TYPE 0x02     /* Match is of requested type */
#define CON_LOCK 0x04     /* I pass the lock on match */
#define CON_COMPLETE 0x08 /* Name given is the full name */
#define CON_TOKEN 0x10    /* Name is a special token */
#define CON_DBREF 0x20    /* Name is a dbref */

/**
 * Quota types
 *
 */
#define QTYPE_ALL 0
#define QTYPE_ROOM 1
#define QTYPE_EXIT 2
#define QTYPE_THING 3
#define QTYPE_PLAYER 4
/**
 * Signal handling directives
 *
 */
#define SA_EXIT 1 /*!< Exit, and dump core */
#define SA_DFLT 2 /*!< Try to restart on a fatal error */
/**
 * Database dumping directives for dump_database_internal()
 *
 */
#define DUMP_DB_NORMAL 0
#define DUMP_DB_CRASH 1
#define DUMP_DB_RESTART 2
#define DUMP_DB_FLATFILE 3
#define DUMP_DB_KILLED 4
/**
 * Constant messages
 *
 */
#define CANNOT_HEAR_MSG "That target cannot hear you."
#define NOT_PRESENT_MSG "That target is not present."

/**
 * @brief Flags constants
 *
 */

#define FLAG_WORD1 0x0 /*!< 1st word of flags. */
#define FLAG_WORD2 0x1 /*!< 2nd word of flags. */
#define FLAG_WORD3 0x2 /*!< 3rd word of flags. */

/**
 * Object types
 *
 */
#define TYPE_ROOM 0x0
#define TYPE_THING 0x1
#define TYPE_EXIT 0x2
#define TYPE_PLAYER 0x3
#define TYPE_ZONE 0x4
#define TYPE_GARBAGE 0x5
#define GOODTYPE 0x5 /*!< Values less than this pass Good_obj check */
#define NOTYPE 0x7
#define TYPE_MASK 0x7
/**
 * First word of flags
 *
 */
#define SEETHRU 0x00000008     /*!< Can see through to the other side */
#define WIZARD 0x00000010      /*!< gets automatic control */
#define LINK_OK 0x00000020     /*!< anybody can link to this room */
#define DARK 0x00000040        /*!< Don't show contents or presence */
#define JUMP_OK 0x00000080     /*!< Others may @tel here */
#define STICKY 0x00000100      /*!< Object goes home when dropped */
#define DESTROY_OK 0x00000200  /*!< Others may @destroy */
#define HAVEN 0x00000400       /*!< No killing here, or no pages */
#define QUIET 0x00000800       /*!< Prevent 'feelgood' messages */
#define HALT 0x00001000        /*!< object cannot perform actions */
#define TRACE 0x00002000       /*!< Generate evaluation trace output */
#define GOING 0x00004000       /*!< object is available for recycling */
#define MONITOR 0x00008000     /*!< Process ^x:action listens on obj? */
#define MYOPIC 0x00010000      /*!< See things as nonowner/nonwizard */
#define PUPPET 0x00020000      /*!< Relays ALL messages to owner */
#define CHOWN_OK 0x00040000    /*!< Object may be @chowned freely */
#define ENTER_OK 0x00080000    /*!< Object may be ENTERed */
#define VISUAL 0x00100000      /*!< Everyone can see properties */
#define IMMORTAL 0x00200000    /*!< Object can't be killed */
#define HAS_STARTUP 0x00400000 /*!< Load some attrs at startup */
#define OPAQUE 0x00800000      /*!< Can't see inside */
#define VERBOSE 0x01000000     /*!< Tells owner everything it does. */
#define INHERIT 0x02000000     /*!< Gets owner's privs. (i.e. Wiz) */
#define NOSPOOF 0x04000000     /*!< Report originator of all actions. */
#define ROBOT 0x08000000       /*!< Player is a ROBOT */
#define SAFE 0x10000000        /*!< Need /override to @destroy */
#define ROYALTY 0x20000000     /*!< Sees like a wiz, but ca't modify */
#define HEARTHRU 0x40000000    /*!< Can hear out of this obj or exit */
#define TERSE 0x80000000       /*!< Only show room name on look */
/**
 * Second word of flags
 *
 */
#define KEY 0x00000001         /*!< No puppets */
#define ABODE 0x00000002       /*!< May @set home here */
#define FLOATING 0x00000004    /*!< -- Legacy -- */
#define UNFINDABLE 0x00000008  /*!< Can't loc() from afar */
#define PARENT_OK 0x00000010   /*!< Others may @parent to me */
#define LIGHT 0x00000020       /*!< Visible in dark places */
#define HAS_LISTEN 0x00000040  /*!< Internal: LISTEN attr set */
#define HAS_FWDLIST 0x00000080 /*!< Internal: FORWARDLIST attr set */
#define AUDITORIUM 0x00000100  /*!< Should we check the SpeechLock? */
#define ANSI 0x00000200
#define HEAD_FLAG 0x00000400
#define FIXED 0x00000800
#define UNINSPECTED 0x00001000
#define ZONE_PARENT 0x00002000 /*!< Check as local master room */
#define DYNAMIC 0x00004000
#define NOBLEED 0x00008000
#define STAFF 0x00010000
#define HAS_DAILY 0x00020000
#define GAGGED 0x00040000
#define HAS_COMMANDS 0x00080000   /*!< Check it for $commands */
#define STOP_MATCH 0x00100000     /*!< Stop matching commands if found */
#define BOUNCE 0x00200000         /*!< Forward messages to contents */
#define CONTROL_OK 0x00400000     /*!< ControlLk specifies who ctrls me */
#define CONSTANT_ATTRS 0x00800000 /*!< Can't set attrs on this object */
#define VACATION 0x01000000
#define PLAYER_MAILS 0x02000000 /*!< Mail message in progress */
#define HTML 0x04000000         /*!< Player supports HTML */
#define BLIND 0x08000000        /*!< Suppress has arrived / left msgs */
#define SUSPECT 0x10000000      /*!< Report some activities to wizards */
#define WATCHER 0x20000000      /*!< Watch logins */
#define CONNECTED 0x40000000    /*!< Player is connected */
#define SLAVE 0x80000000        /*!< Disallow most commands */
/**
 * Third word of flags
 *
 */
#define REDIR_OK 0x00000001      /*!< Can be victim of @redirect */
#define HAS_REDIRECT 0x00000002  /*!< Victim of @redirect */
#define ORPHAN 0x00000004        /*!< Don't check parent chain for $cmd */
#define HAS_DARKLOCK 0x00000008  /*!< Has a DarkLock */
#define DIRTY 0x00000010         /*!< Temporary flag: object is dirty */
#define NODEFAULT 0x00000020     /*!< Not subject to attr defaults */
#define PRESENCE 0x00000040      /*!< Check presence-related locks */
#define HAS_SPEECHMOD 0x00000080 /*!< Check @speechmod attr */
#define HAS_PROPDIR 0X00000100   /*!< Internal: has Propdir attr */
#define COLOR256 0x00000200      /*!< Player support XTERM 256 colors */
#define COLOR24BIT 0x00000400    /*!< Player support XTERM 24 bit colors */
#define FLAG_RES03 0x00000800
#define FLAG_RES04 0x00001000
#define FLAG_RES05 0x00002000
#define FLAG_RES06 0x00004000
#define FLAG_RES07 0x00008000
#define FLAG_RES08 0x00010000
#define FLAG_RES09 0x00020000
#define FLAG_RES10 0x00040000
#define FLAG_RES11 0x00080000
#define FLAG_RES12 0x00100000
#define FLAG_RES13 0x00200000
#define MARK_0 0x00400000 /*!< User-defined flags */
#define MARK_1 0x00800000
#define MARK_2 0x01000000
#define MARK_3 0x02000000
#define MARK_4 0x04000000
#define MARK_5 0x08000000
#define MARK_6 0x10000000
#define MARK_7 0x20000000
#define MARK_8 0x40000000
#define MARK_9 0x80000000
#define MARK_FLAGS 0xffc00000 /*!< Bitwise-or of all marker flags */

#define OF_CONTENTS 0x0001 /*!< Object has contents: Contents() */
#define OF_LOCATION 0x0002 /*!< Object has a location: Location() */
#define OF_EXITS 0x0004    /*!< Object has exits: Exits() */
#define OF_HOME 0x0008     /*!< Object has a home: Home() */
#define OF_DROPTO 0x0010   /*!< Object has a dropto: Dropto() */
#define OF_OWNER 0x0020    /*!< Object can own other objects */
#define OF_SIBLINGS 0x0040 /*!< Object has siblings: Next() */

#define VE_LOC_XAM 0x01   /*!< Location is examinable */
#define VE_LOC_DARK 0x02  /*!< Location is dark */
#define VE_BASE_DARK 0x04 /*!< Base location (pre-parent) is dark */

/**
 * @brief Functions constants
 *
 */

#define MAX_NFARGS 30

/**
 * @brief List management
 *
 */
#define ALPHANUM_LIST 1
#define NUMERIC_LIST 2
#define DBREF_LIST 3
#define FLOAT_LIST 4
#define NOCASE_LIST 5

#define IF_DELETE 0
#define IF_REPLACE 1
#define IF_INSERT 2

/**
 * @brief String trimming
 *
 */
#define TRIM_L 0x1
#define TRIM_R 0x2

/**
 * @brief encode() and decode() copy over only alphanumeric chars
 *
 */
#define CRYPTCODE_LO 32  /* space */
#define CRYPTCODE_HI 126 /* tilde */
#define CRYPTCODE_MOD 95 /* count of printable ascii chars */

/**
 * @brief Constants used in delimiter macros.
 *
 */
#define DELIM_EVAL 0x001   /*!< Must eval delimiter. */
#define DELIM_NULL 0x002   /*!< Null delimiter okay. */
#define DELIM_CRLF 0x004   /*!< '%r' delimiter okay. */
#define DELIM_STRING 0x008 /*!< Multi-character delimiter okay. */
/**
 * @brief Function-specific flags used in the function table.
 *
 * from handle_sort (sort, isort):
 *
 */
#define SORT_OPER 0x0f /*!< mask to select sort operation bits */
#define SORT_ITEMS 0
#define SORT_POS 1
/**
 * from handle_sets (setunion, setdiff, setinter, lunion, ldiff, linter):
 *
 */
#define SET_OPER 0x0f /*!< mask to select set operation bits */
#define SET_UNION 0
#define SET_INTERSECT 1
#define SET_DIFF 2
#define SET_TYPE 0x10 /*!< set type is given, don't autodetect */
/**
 * from process_tables (tables, rtables, ctables):
 * from perform_border (border, rborder, cborder):
 * from perform_align (align, lalign):
 *
 */
#define JUST_TYPE 0x0f /*!< mask to select justification bits */
#define JUST_LEFT 0x01
#define JUST_RIGHT 0x02
#define JUST_CENTER 0x04
#define JUST_REPEAT 0x10
#define JUST_COALEFT 0x20
#define JUST_COARIGHT 0x40
/**
 * from handle_logic (and, or, andbool, orbool, land, lor, landbool, lorbool,
 * cand, cor, candbool, corbool, xor, xorbool):
 * from handle_flaglists (andflags, orflags):
 * from handle_filter (filter, filterbool):
 *
 */
#define LOGIC_OPER 0x0f /*!< mask to select boolean operation bits */
#define LOGIC_AND 0
#define LOGIC_OR 1
#define LOGIC_XOR 2
#define LOGIC_BOOL 0x10 /*!< interpret operands as boolean, not int */
#define LOGIC_LIST 0x40 /*!< operands come in a list, not separately */
/**
 * from handle_vectors (vadd, vsub, vmul, vdot):
 *
 */
#define VEC_OPER 0x0f /*!< mask to select vector operation bits */
#define VEC_ADD 0
#define VEC_SUB 1
#define VEC_MUL 2
#define VEC_DOT 3
#define VEC_CROSS 4 /*!< not implemented */
#define VEC_OR 7
#define VEC_AND 8
#define VEC_XOR 9
/**
 * from handle_vector (vmag, vunit):
 *
 */
#define VEC_MAG 5
#define VEC_UNIT 6
/**
 * from perform_loop (loop, parse):
 * from perform_iter (list, iter, whentrue, whenfalse, istrue, isfalse):
 *
 */
#define BOOL_COND_TYPE 0x0f   /*!< mask to select exit-condition bits */
#define BOOL_COND_NONE 1      /*!< loop until end of list */
#define BOOL_COND_FALSE 2     /*!< loop until true */
#define BOOL_COND_TRUE 3      /*!< loop until false */
#define FILT_COND_TYPE 0x0f0  /*!< mask to select filter bits */
#define FILT_COND_NONE 0x010  /*!< show all results */
#define FILT_COND_FALSE 0x020 /*!< show only false results */
#define FILT_COND_TRUE 0x030  /*!< show only true results */
#define LOOP_NOTIFY 0x100     /*!< send loop results directly to enactor */
#define LOOP_TWOLISTS 0x200   /*!< process two lists */
/**
 * from handle_okpres (hears, moves, knows):
 *
 */
#define PRESFN_OPER 0x0f  /*!< Mask to select bits */
#define PRESFN_HEARS 0x01 /*!< Detect hearing */
#define PRESFN_MOVES 0x02 /*!< Detect movement */
#define PRESFN_KNOWS 0x04 /*!< Detect knows */
/**
 * from perform_get (get, get_eval, xget, eval(a,b)):
 *
 */
#define GET_EVAL 0x01  /*!< evaluate the attribute */
#define GET_XARGS 0x02 /*!< obj and attr are two separate args */
/**
 * from handle_pop (pop, peek, toss):
 *
 */
#define POP_PEEK 0x01 /*!< don't remove item from stack */
#define POP_TOSS 0x02 /*!< don't display item from stack */
/**
 * from perform_regedit (regedit, regediti, regeditall, regeditalli):
 * from perform_regparse (regparse, regparsei):
 * from perform_regrab (regrab, regrabi, regraball, regraballi):
 * from perform_regmatch (regmatch, regmatchi):
 * from perform_grep (grep, grepi, wildgrep, regrep, regrepi):
 *
 */
#define REG_CASELESS 0x01  /*!< XXX must equal PCRE_CASELESS */
#define REG_MATCH_ALL 0x02 /*!< operate on all matches in a list */
#define REG_TYPE 0x0c      /*!< mask to select grep type bits */
#define GREP_EXACT 0
#define GREP_WILD 4
#define GREP_REGEXP 8
/**
 * from handle_trig (sin, cos, tan, asin, acos, atan, sind, cosd, tand, asind, acosd, atand):
 *
 */
#define TRIG_OPER 0x0f /*!< mask to select trig function bits */
#define TRIG_CO 0x01   /*!< co-function, like cos as opposed to sin */
#define TRIG_TAN 0x02  /*!< tan-function, like cot as opposed to cos */
#define TRIG_ARC 0x04  /*!< arc-function, like asin as opposed to sin */
#define TRIG_REC 0x08  /*!<  -- reciprocal, like sec as opposed to sin */
#define TRIG_DEG 0x10  /*!< angles are in degrees, not radians */
/**
 * from handle_pronoun (obj, poss, subj, aposs):
 *
 */
#define PRONOUN_OBJ 0
#define PRONOUN_POSS 1
#define PRONOUN_SUBJ 2
#define PRONOUN_APOSS 3
/**
 * from do_ufun():
 *
 */
#define U_LOCAL 0x01   /*!< ulocal: preserve global registers */
#define U_PRIVATE 0x02 /*!< ulocal: preserve global registers */
/**
 * from handle_ifelse() and handle_if()
 *
 */
#define IFELSE_OPER 0x0f    /*!< mask */
#define IFELSE_BOOL 0x01    /*!< check for boolean (defaults to nonzero) */
#define IFELSE_FALSE 0x02   /*!< order false,true instead of true,false */
#define IFELSE_DEFAULT 0x04 /*!< only two args, use condition as output */
#define IFELSE_TOKEN 0x08   /*!< allow switch-token substitution */
/**
 * from handle_timestamps()
 *
 */
#define TIMESTAMP_MOD 0x01 /*!< lastmod() */
#define TIMESTAMP_ACC 0X02 /*!< lastaccess() */
#define TIMESTAMP_CRE 0x04 /*!< creation() */
/**
 * Miscellaneous
 *
 */
#define LATTR_COUNT 0x01     /*!< nattr: just return attribute count */
#define LOCFN_WHERE 0x01     /*!< loc: where() vs. loc() */
#define NAMEFN_FULLNAME 0x01 /*!< name: fullname() vs. name() */
#define CHECK_PARENTS 0x01   /*!< hasattrp: recurse up the parent chain */
#define CONNINFO_IDLE 0x01   /*!< conninfo: idle() vs. conn() */
#define UCALL_SANDBOX 0x01   /*!< ucall: sandbox() vs. ucall() */

#define FP_SIZE ((sizeof(long double) + sizeof(unsigned int) - 1) / sizeof(unsigned int))
#define FP_EXP_WEIRD 0x1
#define FP_EXP_ZERO 0x2

/**
 * @brief File cache constants
 *
 */

/**
 * File caches.  These _must_ track the fcache array in file_c.c
 *
 */
#define FC_CONN 0
#define FC_CONN_SITE 1
#define FC_CONN_DOWN 2
#define FC_CONN_FULL 3
#define FC_CONN_GUEST 4
#define FC_CONN_REG 5
#define FC_CREA_NEW 6
#define FC_CREA_REG 7
#define FC_MOTD 8
#define FC_WIZMOTD 9
#define FC_QUIT 10
#define FC_CONN_HTML 11
#define FC_LAST 11

/**
 * @brief Game constants
 *
 */

/**
 * magic lock cookies
 *
 */
#define NOT_TOKEN '!'
#define AND_TOKEN '&'
#define OR_TOKEN '|'
#define LOOKUP_TOKEN '*'
#define NUMBER_TOKEN '#'
#define INDIR_TOKEN '@' /*!< One of these two should go. */
#define CARRY_TOKEN '+' /*!< One of these two should go. */
#define IS_TOKEN '='
#define OWNER_TOKEN '$'
/**
 * matching attribute tokens
 *
 */
#define AMATCH_CMD '$'
#define AMATCH_LISTEN '^'
/**
 * delimiters for various things
 *
 */
#define EXIT_DELIMITER ';'
#define ARG_DELIMITER '='
/**
 * These chars get replaced by the current item from a list in commands and
 * functions that do iterative replacement, such as @apply_marked, dolist,
 * the eval= operator for @search, and iter().
 *
 */
#define BOUND_VAR "##"
#define LISTPLACE_VAR "#@"
/**
 * This token is similar, marking the first argument in a switch.
 *
 */
#define SWITCH_VAR "#$"
/**
 * This token is used to denote a null output delimiter.
 *
 */
#define NULL_DELIM_VAR "@@"
/**
 * This is used to indent output from pretty-printing.
 *
 */
#define INDENT_STR "  "
/**
 * This is used as the 'null' delimiter for structures stored via write().
 *
 */
#define GENERIC_STRUCT_DELIM '\f' /*!< form feed char */
#define GENERIC_STRUCT_STRDELIM "\f"
/**
 * amount of object endowment, based on cost
 *
 */
#define OBJECT_ENDOWMENT(cost) (((cost) / mushconf.sacfactor) + mushconf.sacadjust)
/**
 * !!! added for recycling, return value of object
 *
 */
#define OBJECT_DEPOSIT(pennies) (((pennies) - mushconf.sacadjust) * mushconf.sacfactor)
/**
 * Always nice to have a trash can.
 *
 */
#define DEV_NULL "/dev/null"
/**
 * This is used to define the version of our backup file
 *
 */
#define BACKUP_VERSION 1
/**
 * Max offset for PCRE
 *
 */
#define PCRE_MAX_OFFSETS 99

/**
 * @brief Help constants
 *
 */

#define LINE_SIZE 90

#define TOPIC_NAME_LEN 30

/**
 * @brief HTab constants
 *
 */

/**
 * Hash entry flags
 *
 */
#define HASH_ALIAS 0x00000001 /*!< This entry is just a copy */

/**
 * Hash table flags
 *
 */
#define HT_STR 0x00000000      /*!< String-keyed hashtable */
#define HT_NUM 0x00000001      /*!< Numeric-keyed hashtable */
#define HT_TYPEMASK 0x0000000f /*!< Reserve up to 16 key types */
#define HT_KEYREF 0x00000010   /*!< Store keys by reference not copy */

/**
 * @brief Interface constants
 *
 */

/* (Dis)connection reason codes */

#define R_GUEST 1   /*!< Guest connection */
#define R_CREATE 2  /*!< User typed 'create' */
#define R_CONNECT 3 /*!< User typed 'connect' */
#define R_DARK 4    /*!< User typed 'cd' */

#define R_QUIT 5       /*!< User quit */
#define R_TIMEOUT 6    /*!< Inactivity timeout */
#define R_BOOT 7       /*!< Victim of @boot, @toad, or @destroy */
#define R_SOCKDIED 8   /*!< Other end of socket closed it */
#define R_GOING_DOWN 9 /*!< Game is going down */
#define R_BADLOGIN 10  /*!< Too many failed login attempts */
#define R_GAMEDOWN 11  /*!< Not admitting users now */
#define R_LOGOUT 12    /*!< Logged out w/o disconnecting */
#define R_GAMEFULL 13  /*!< Too many players logged in */

/* Logged out command table definitions */

#define CMD_QUIT 1
#define CMD_WHO 2
#define CMD_DOING 3
#define CMD_PREFIX 5
#define CMD_SUFFIX 6
#define CMD_LOGOUT 7
#define CMD_SESSION 8
#define CMD_PUEBLOCLIENT 9
#define CMD_INFO 10

#define CMD_MASK 0xff
#define CMD_NOxFIX 0x100

/* flags in the flag field */
#define DS_CONNECTED 0x0001    /*!< player is connected */
#define DS_AUTODARK 0x0002     /*!< Wizard was auto set dark. */
#define DS_PUEBLOCLIENT 0x0004 /*!< Client is Pueblo-enhanced. */

/**
 * Site flags
 *
 */
#define S_SUSPECT 1
#define S_ACCESS 2

/**
 * @brief Match constants
 *
 */

#define NOMATCH_MESSAGE "I don't see that here."
#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"
#define NOPERM_MESSAGE "Permission denied."

#define MAT_NO_EXITS 1     /*!< Don't check for exits */
#define MAT_EXIT_PARENTS 2 /*!< Check for exits in parents */
#define MAT_NUMERIC 4      /*!< Check for un-#ified dbrefs */
#define MAT_HOME 8         /*!< Check for 'home' */

/**
 * @brief MUSH constants
 *
 */

/**
 * Game control flags in mushconf.control_flags
 *
 */
#define CF_LOGIN 0x0001      /*!< Allow nonwiz logins to the MUSH */
#define CF_BUILD 0x0002      /*!< Allow building commands */
#define CF_INTERP 0x0004     /*!< Allow object triggering */
#define CF_CHECKPOINT 0x0008 /*!< Perform auto-checkpointing */
#define CF_DBCHECK 0x0010    /*!< Periodically check/clean the DB */
#define CF_IDLECHECK 0x0020  /*!< Periodically check for idle users */
#define CF_NOTUSED1 0x0040   /*!< empty        0x0040 */
#define CF_NOTUSED2 0x0080   /*!< empty        0x0080 */
#define CF_DEQUEUE 0x0100    /*!< Remove entries from the queue */
#define CF_GODMONITOR 0x0200 /*!< Display commands to the God. */
#define CF_EVENTCHECK 0x0400 /*!< Allow events checking */
/**
 * Host information codes
 *
 */
#define H_REGISTRATION 0x0001 /*!< Registration ALWAYS on */
#define H_FORBIDDEN 0x0002    /*!< Reject all connects */
#define H_SUSPECT 0x0004      /*!< Notify wizards of connects/disconnects */
#define H_GUEST 0x0008        /*!< Don't permit guests from here */
/**
 * Logging options
 *
 */
#define LOG_ALLCOMMANDS 0x00000001 /*!< Log all commands */
#define LOG_ACCOUNTING 0x00000002  /*!< Write accounting info on logout */
#define LOG_BADCOMMANDS 0x00000004 /*!< Log bad commands */
#define LOG_BUGS 0x00000008        /*!< Log program bugs found */
#define LOG_DBSAVES 0x00000010     /*!< Log database dumps */
#define LOG_CONFIGMODS 0x00000020  /*!< Log changes to configuration */
#define LOG_PCREATES 0x00000040    /*!< Log character creations */
#define LOG_KILLS 0x00000080       /*!< Log KILLs */
#define LOG_LOGIN 0x00000100       /*!< Log logins and logouts */
#define LOG_NET 0x00000200         /*!< Log net connects and disconnects */
#define LOG_SECURITY 0x00000400    /*!< Log security-related events */
#define LOG_SHOUTS 0x00000800      /*!< Log shouts */
#define LOG_STARTUP 0x00001000     /*!< Log nonfatal errors in startup */
#define LOG_WIZARD 0x00002000      /*!< Log dangerous things */
#define LOG_ALLOCATE 0x00004000    /*!< Log alloc/free from buffer pools */
#define LOG_PROBLEMS 0x00008000    /*!< Log runtime problems */
#define LOG_KBCOMMANDS 0x00010000  /*!< Log keyboard commands */
#define LOG_SUSPECTCMDS 0x00020000 /*!< Log SUSPECT player keyboard cmds */
#define LOG_TIMEUSE 0x00040000     /*!< Log CPU time usage */
#define LOG_LOCAL 0x00080000       /*!< Log user stuff via @log */
#define LOG_MALLOC 0x00100000      /*!< Log malloc requests */
#define LOG_FORCE 0x04000000       /*!< Ignore mushstate.logging */
#define LOG_ALWAYS 0x80000000      /*!< Always log it */

#define LOGOPT_FLAGS 0x01     /*!< Report flags on object */
#define LOGOPT_LOC 0x02       /*!< Report loc of obj when requested */
#define LOGOPT_OWNER 0x04     /*!< Report owner of obj if not obj */
#define LOGOPT_TIMESTAMP 0x08 /*!< Timestamp log entries */

/**
 * @brief Players constants
 *
 */

#define NUM_GOOD 4 /*!< # of successful logins to save data for */
#define NUM_BAD 3  /*!< # of failed logins to save data for */

/**
 * @brief Powers constants
 *
 */

#define POWER_EXT 0x1 /*!< Lives in extended powers word */
/**
 * First word of powers
 *
 */
#define POW_CHG_QUOTAS 0x00000001  /*!< May change and see quotas */
#define POW_CHOWN_ANY 0x00000002   /*!< Can @chown anything or to anyone */
#define POW_ANNOUNCE 0x00000004    /*!< May use @wall */
#define POW_BOOT 0x00000008        /*!< May use @boot */
#define POW_HALT 0x00000010        /*!< May @halt on other's objects */
#define POW_CONTROL_ALL 0x00000020 /*!< I control everything */
#define POW_WIZARD_WHO 0x00000040  /*!< See extra WHO information */
#define POW_EXAM_ALL 0x00000080    /*!< I can examine everything */
#define POW_FIND_UNFIND 0x00000100 /*!< Can find unfindable players */
#define POW_FREE_MONEY 0x00000200  /*!< I have infinite money */
#define POW_FREE_QUOTA 0x00000400  /*!< I have infinite quota */
#define POW_HIDE 0x00000800        /*!< Can set themselves DARK */
#define POW_IDLE 0x00001000        /*!< No idle limit */
#define POW_SEARCH 0x00002000      /*!< Can @search anyone */
#define POW_LONGFINGERS 0x00004000 /*!< Can get/whisper/etc from a distance */
#define POW_PROG 0x00008000        /*!< Can use the @prog command */
#define POW_MDARK_ATTR 0x00010000  /*!< Can read AF_MDARK attrs */
#define POW_WIZ_ATTR 0x00020000    /*!< Can write AF_WIZARD attrs */
#define POW_FREE_ATTR 0x00040000   /*!< Not used - Available */
#define POW_COMM_ALL 0x00080000    /*!< Channel wiz */
#define POW_SEE_QUEUE 0x00100000   /*!< Player can see the entire queue */
#define POW_SEE_HIDDEN 0x00200000  /*!< Player can see hidden players on WHO list */
#define POW_WATCH 0x00400000       /*!< Player can set or clear WATCHER */
#define POW_POLL 0x00800000        /*!< Player can set the doing poll */
#define POW_NO_DESTROY 0x01000000  /*!< Cannot be destroyed */
#define POW_GUEST 0x02000000       /*!< Player is a guest */
#define POW_PASS_LOCKS 0x04000000  /*!< Player can pass any lock */
#define POW_STAT_ANY 0x08000000    /*!< Can @stat anyone */
#define POW_STEAL 0x10000000       /*!< Can give negative money */
#define POW_TEL_ANYWHR 0x20000000  /*!< Teleport anywhere */
#define POW_TEL_UNRST 0x40000000   /*!< Teleport anything */
#define POW_UNKILLABLE 0x80000000  /*!< Can't be killed */
/**
 * Second word of powers
 *
 */
#define POW_BUILDER 0x00000001    /*!< Can build */
#define POW_LINKVAR 0x00000002    /*!< Can link an exit to "variable" */
#define POW_LINKTOANY 0x00000004  /*!< Can link to any object */
#define POW_OPENANYLOC 0x00000008 /*!< Can open from anywhere */
#define POW_USE_MODULE 0x00000010 /*!< Can use MODULE queries directly */
#define POW_LINKHOME 0x00000020   /*!< Can link object to any home */
#define POW_CLOAK 0x00000040      /*!< Can vanish from sight via DARK */

/**
 * @brief String constants
 *
 */

#define STRING_EMPTY ""

/**
 * ANSI control codes for various neat-o terminal effects.
 *
 */
#define BEEP_CHAR '\07'
#define ESC_CHAR '\033'

#define ANSI_CSI '['
#define ANSI_END 'm'

#define ANSI_NORMAL "\033[0m"

#define ANSI_REVERSE_NORMAL "m0[\033"
#define ANSI_REVERSE_HIRED "m13[\033m1[\033"

#define ANSI_HILITE "\033[1m"
#define ANSI_INVERSE "\033[7m"
#define ANSI_BLINK "\033[5m"
#define ANSI_UNDER "\033[4m"

#define ANSI_INV_BLINK "\033[7;5m"
#define ANSI_INV_HILITE "\033[1;7m"
#define ANSI_BLINK_HILITE "\033[1;5m"
#define ANSI_INV_BLINK_HILITE "\033[1;5;7m"

/**
 * Foreground colors
 *
 */
#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

/**
 * Background colors
 *
 */
#define ANSI_BBLACK "\033[40m"
#define ANSI_BRED "\033[41m"
#define ANSI_BGREEN "\033[42m"
#define ANSI_BYELLOW "\033[43m"
#define ANSI_BBLUE "\033[44m"
#define ANSI_BMAGENTA "\033[45m"
#define ANSI_BCYAN "\033[46m"
#define ANSI_BWHITE "\033[47m"

/**
 * XTERM ansi codes
 *
 */
#define ANSI_XTERM_FG "\033[38;5;"
#define ANSI_XTERM_BG "\033[48;5;"

#define ANSI_24BIT_FG "\033[38;2;"
#define ANSI_24BIT_BG "\033[48;2;"

/**
 * Numeric-only definitions
 *
 */
#define N_ANSI_NORMAL "0"
#define N_ANSI_HILITE "1"
#define N_ANSI_INVERSE "7"
#define N_ANSI_BLINK "5"
#define N_ANSI_UNDER "4"

#define N_ANSI_BLACK "30"
#define N_ANSI_RED "31"
#define N_ANSI_GREEN "32"
#define N_ANSI_YELLOW "33"
#define N_ANSI_BLUE "34"
#define N_ANSI_MAGENTA "35"
#define N_ANSI_CYAN "36"
#define N_ANSI_WHITE "37"

#define N_ANSI_BBLACK "40"
#define N_ANSI_BRED "41"
#define N_ANSI_BGREEN "42"
#define N_ANSI_BYELLOW "43"
#define N_ANSI_BBLUE "44"
#define N_ANSI_BMAGENTA "45"
#define N_ANSI_BCYAN "46"
#define N_ANSI_BWHITE "47"

#define N_ANSI_NORMAL "0"

/**
 * Integers
 *
 */
#define I_ANSI_NORMAL 0

#define I_ANSI_HILITE 1
#define I_ANSI_INVERSE 7
#define I_ANSI_BLINK 5
#define I_ANSI_UNDER 4

#define I_ANSI_BLACK 30
#define I_ANSI_RED 31
#define I_ANSI_GREEN 32
#define I_ANSI_YELLOW 33
#define I_ANSI_BLUE 34
#define I_ANSI_MAGENTA 35
#define I_ANSI_CYAN 36
#define I_ANSI_WHITE 37

#define I_ANSI_BBLACK 40
#define I_ANSI_BRED 41
#define I_ANSI_BGREEN 42
#define I_ANSI_BYELLOW 43
#define I_ANSI_BBLUE 44
#define I_ANSI_BMAGENTA 45
#define I_ANSI_BCYAN 46
#define I_ANSI_BWHITE 47

#define I_ANSI_NUM 48
#define I_ANSI_LIM 50

#define ANST_NORMAL 0x0099
#define ANST_NONE 0x1099

/**
 * @brief UDB Constants
 *
 */

/**
 * Define the number of objects we may be reading/writing to at one time
 *
 */
#define NUM_OBJPIPES 64

/**
 * Cache flags
 *
 */
#define CACHE_DIRTY 0x00000001

/**
 * default (runtime-resettable) cache parameters
 *
 */
#define CACHE_SIZE 1000000 /*!< 1 million bytes */
#define CACHE_WIDTH 200    /*!< Cache width */

/**
 * Datatypes that we have in cache and on disk
 *
 */
#define DBTYPE_EMPTY 0             /*!< This entry is empty */
#define DBTYPE_ATTRIBUTE 1         /*!< This is an attribute */
#define DBTYPE_DBINFO 2            /*!< Various DB paramaters */
#define DBTYPE_OBJECT 3            /*!< Object structure */
#define DBTYPE_ATRNUM 4            /*!< Attribute number to name map */
#define DBTYPE_MODULETYPE 5        /*!< DBTYPE to module name map */
#define DBTYPE_RESERVED 0x0000FFFF /*!< Numbers >= are free for use by user code (modules) */
#define DBTYPE_END 0xFFFFFFFF      /*!< Highest type */

#define DEFAULT_DBMCHUNKFILE "netmush"

/**
 * @brief Player cache related
 *
 */
#define PF_DEAD 0x0001
#define PF_REF 0x0002
#define PF_MONEY_CH 0x0004
#define PF_QMAX_CH 0x0008

/**
 * @brief User attributes constants
 *
 */

#ifndef VATTR_HASH_SIZE /*!< must be a power of two */
#define VATTR_HASH_SIZE 8192
#endif

#define VNAME_SIZE 32

/**
 * @brief Cron constants
 *
 */
#define FIRST_MINUTE 0
#define LAST_MINUTE 59

#define FIRST_HOUR 0
#define LAST_HOUR 23

#define FIRST_DOM 1
#define LAST_DOM 31

#define FIRST_MONTH 1
#define LAST_MONTH 12

/**
 * @note on DOW, 0 and 7 are both Sunday, for compatibility reasons.
 *
 */
#define FIRST_DOW 0
#define LAST_DOW 7

#define DOM_STAR 0x01
#define DOW_STAR 0x02

/**
 * @brief Floating point precision
 *
 */

#define FPTS_DIG LDBL_DIG - 1

#endif /* __CONSTANTS_H */