/* attrs.h - Attribute definitions */

#include "copyright.h"

#ifndef __ATTRS_H
#define __ATTRS_H

/* Attribute flags */
#define AF_ODARK		0x00000001	/* players other than owner can't see it */
#define AF_DARK			0x00000002	/* No one can see it */
#define AF_WIZARD		0x00000004	/* only wizards can change it */
#define AF_MDARK		0x00000008	/* Only wizards can see it. Dark to mortals */
#define AF_INTERNAL		0x00000010	/* Don't show even to #1 */
#define AF_NOCMD		0x00000020	/* Don't create a @ command for it */
#define AF_LOCK			0x00000040	/* Attribute is locked */
#define AF_DELETED		0x00000080	/* Attribute should be ignored */
#define AF_NOPROG		0x00000100	/* Don't process $-commands from this attr */
#define AF_GOD			0x00000200	/* Only #1 can change it */
#define AF_IS_LOCK		0x00000400	/* Attribute is a lock */
#define AF_VISUAL		0x00000800	/* Anyone can see */
#define AF_PRIVATE		0x00001000	/* Not inherited by children */
#define AF_HTML			0x00002000	/* Don't HTML escape this in did_it() */
#define AF_NOPARSE		0x00004000	/* Don't evaluate when checking for $-cmds */
#define AF_REGEXP		0x00008000	/* Do a regexp rather than wildcard match */
#define AF_NOCLONE		0x00010000	/* Don't copy this attr when cloning. */
#define AF_CONST		0x00020000	/* No one can change it (set by server) */
#define AF_CASE			0x00040000	/* Regexp matches are case-sensitive */
#define AF_STRUCTURE	0x00080000	/* Attribute contains a structure */
#define AF_DIRTY		0x00100000	/* Attribute number has been modified */
#define AF_DEFAULT		0x00200000	/* did_it() checks attr_defaults obj */
#define AF_NONAME		0x00400000	/* If used as oattr, no name prepend */
#define AF_RMATCH		0x00800000	/* Set the result of match into regs */
#define AF_NOW			0x01000000	/* execute match immediately */
#define AF_TRACE		0x02000000	/* trace ufunction */
#define AF_FREE_1		0x04000000	/* Reserved for futur use */
#define AF_FREE_2		0x08000000	/* Reserved for futur use */
#define AF_FREE_3		0x10000000	/* Reserved for futur use */
#define AF_FREE_4		0x20000000	/* Reserved for futur use */
#define AF_FREE_5		0x40000000	/* Reserved for futur use */
#define AF_FREE_6		0x80000000	/* Reserved for futur use */

#define A_NULL			0	/* Nothing */
#define A_OSUCC			1	/* Others success message */
#define A_OFAIL			2	/* Others fail message */
#define A_FAIL			3	/* Invoker fail message */
#define A_SUCC			4	/* Invoker success message */
#define A_PASS			5	/* Password (only meaningful for players) */
#define A_DESC			6	/* Description */
#define A_SEX			7	/* Sex */
#define A_ODROP			8	/* Others drop message */
#define A_DROP			9	/* Invoker drop message */
#define A_OKILL			10	/* Others kill message */
#define A_KILL			11	/* Invoker kill message */
#define A_ASUCC			12	/* Success action list */
#define A_AFAIL			13	/* Failure action list */
#define A_ADROP			14	/* Drop action list */
#define A_AKILL			15	/* Kill action list */
#define A_AUSE			16	/* Use action list */
#define A_CHARGES		17	/* Number of charges remaining */
#define A_RUNOUT		18	/* Actions done when no more charges */
#define A_STARTUP		19	/* Actions run when game started up */
#define A_ACLONE		20	/* Actions run when obj is cloned */
#define A_APAY			21	/* Actions run when given COST pennies */
#define A_OPAY			22	/* Others pay message */
#define A_PAY			23	/* Invoker pay message */
#define A_COST			24	/* Number of pennies needed to invoke xPAY */
#define A_MONEY			25	/* Value or Wealth (internal) */
#define A_LISTEN		26	/* (Wildcarded) string to listen for */
#define A_AAHEAR		27	/* Actions to do when anyone says LISTEN str */
#define A_AMHEAR		28	/* Actions to do when I say LISTEN str */
#define A_AHEAR			29	/* Actions to do when others say LISTEN str */
#define A_LAST			30	/* Date/time of last login (players only) */
#define A_QUEUEMAX		31	/* Max. # of entries obj has in the queue */
#define A_IDESC			32	/* Inside description (ENTER to get inside) */
#define A_ENTER			33	/* Invoker enter message */
#define A_OXENTER		34	/* Others enter message in dest */
#define A_AENTER		35	/* Enter action list */
#define A_ADESC			36	/* Describe action list */
#define A_ODESC			37	/* Others describe message */
#define A_RQUOTA		38	/* Relative object quota */
#define A_ACONNECT		39	/* Actions run when player connects */
#define A_ADISCONNECT   40	/* Actions run when player disconnects */
#define A_ALLOWANCE		41	/* Daily allowance, if diff from default */
#define A_LOCK			42	/* Object lock */
#define A_NAME			43	/* Object name */
#define A_COMMENT		44	/* Wizard-accessible comments */
#define A_USE			45	/* Invoker use message */
#define A_OUSE			46	/* Others use message */
#define A_SEMAPHORE		47	/* Semaphore control info */
#define A_TIMEOUT		48	/* Per-user disconnect timeout */
#define A_QUOTA			49	/* Absolute quota (to speed up @quota) */
#define A_LEAVE			50	/* Invoker leave message */
#define A_OLEAVE		51	/* Others leave message in src */
#define A_ALEAVE		52	/* Leave action list */
#define A_OENTER		53	/* Others enter message in src */
#define A_OXLEAVE		54	/* Others leave message in dest */
#define A_MOVE			55	/* Invoker move message */
#define A_OMOVE			56	/* Others move message */
#define A_AMOVE			57	/* Move action list */
#define A_ALIAS			58	/* Alias for player names */
#define A_LENTER		59	/* ENTER lock */
#define A_LLEAVE		60	/* LEAVE lock */
#define A_LPAGE			61	/* PAGE lock */
#define A_LUSE			62	/* USE lock */
#define A_LGIVE			63	/* Give lock (who may give me away?) */
#define A_EALIAS		64	/* Alternate names for ENTER */
#define A_LALIAS		65	/* Alternate names for LEAVE */
#define A_EFAIL			66	/* Invoker entry fail message */
#define A_OEFAIL		67	/* Others entry fail message */
#define A_AEFAIL		68	/* Entry fail action list */
#define A_LFAIL			69	/* Invoker leave fail message */
#define A_OLFAIL		70	/* Others leave fail message */
#define A_ALFAIL		71	/* Leave fail action list */
#define A_REJECT		72	/* Rejected page return message */
#define A_AWAY			73	/* Not_connected page return message */
#define A_IDLE			74	/* Success page return message */
#define A_UFAIL			75	/* Invoker use fail message */
#define A_OUFAIL		76	/* Others use fail message */
#define A_AUFAIL		77	/* Use fail action list */
#define A_FREE78		78	/* Unused, Formerly A_PFAIL: Invoker page fail message */
#define A_TPORT			79	/* Invoker teleport message */
#define A_OTPORT		80	/* Others teleport message in src */
#define A_OXTPORT		81	/* Others teleport message in dst */
#define A_ATPORT		82	/* Teleport action list */
#define A_FREE83		83	/* Unused, Formerly A_PRIVS: Individual permissions */
#define A_LOGINDATA		84	/* Recent login information */
#define A_LTPORT		85	/* Teleport lock (can others @tel to me?) */
#define A_LDROP			86	/* Drop lock (can I be dropped or @tel'ed) */
#define A_LRECEIVE		87	/* Receive lock (who may give me things?) */
#define A_LASTSITE		88	/* Last site logged in from, in cleartext */
#define A_INPREFIX		89	/* Prefix on incoming messages into objects */
#define A_PREFIX		90	/* Prefix used by exits/objects when audible */
#define A_INFILTER		91	/* Filter to zap incoming text into objects */
#define A_FILTER		92	/* Filter to zap text forwarded by audible. */
#define A_LLINK			93	/* Who may link to here */
#define A_LTELOUT		94	/* Who may teleport out from here */
#define A_FORWARDLIST	95	/* Recipients of AUDIBLE output */
#define A_MAILFOLDERS	96	/* @mail folders */
#define A_LUSER			97	/* Spare lock not referenced by server */
#define A_LPARENT		98	/* Who may @parent to me if PARENT_OK set */
#define A_LCONTROL		99	/* Who controls me if CONTROL_OK set */
#define A_VA			100	/* VA-Z attribute */
#define A_VB			101	/* VA-Z attribute */
#define A_VC			102	/* VA-Z attribute */
#define A_VD			103	/* VA-Z attribute */
#define A_VE			104	/* VA-Z attribute */
#define A_VF			105	/* VA-Z attribute */
#define A_VG			106	/* VA-Z attribute */
#define A_VH			107	/* VA-Z attribute */
#define A_VI			108	/* VA-Z attribute */
#define A_VJ			109	/* VA-Z attribute */
#define A_VK			110	/* VA-Z attribute */
#define A_VL			111	/* VA-Z attribute */
#define A_VM			112	/* VA-Z attribute */
#define A_VN			113	/* VA-Z attribute */
#define A_VO			114	/* VA-Z attribute */
#define A_VP			115	/* VA-Z attribute */
#define A_VQ			116	/* VA-Z attribute */
#define A_VR			117	/* VA-Z attribute */
#define A_VS			118	/* VA-Z attribute */
#define A_VT			119	/* VA-Z attribute */
#define A_VU			120	/* VA-Z attribute */
#define A_VV			121	/* VA-Z attribute */
#define A_VW			122	/* VA-Z attribute */
#define A_VX			123	/* VA-Z attribute */
#define A_VY			124	/* VA-Z attribute */
#define A_VZ			125	/* VA-Z attribute */
#define A_FREE126		126	/* Unused */
#define A_FREE127		127	/* Unused */
#define A_FREE128		128	/* Unused */
#define A_GFAIL			129	/* Give fail message */
#define A_OGFAIL		130	/* Others give fail message */
#define A_AGFAIL		131	/* Give fail action */
#define A_RFAIL			132	/* Receive fail message */
#define A_ORFAIL		133	/* Others receive fail message */
#define A_ARFAIL		134	/* Receive fail action */
#define A_DFAIL			135	/* Drop fail message */
#define A_ODFAIL		136	/* Others drop fail message */
#define A_ADFAIL		137	/* Drop fail action */
#define A_TFAIL			138	/* Teleport (to) fail message */
#define A_OTFAIL		139	/* Others teleport (to) fail message */
#define A_ATFAIL		140	/* Teleport fail action */
#define A_TOFAIL		141	/* Teleport (from) fail message */
#define A_OTOFAIL		142	/* Others teleport (from) fail message */
#define A_ATOFAIL		143	/* Teleport (from) fail action */
#define A_FREE144		144	/* Unused */
#define A_FREE145		145	/* Unused */
#define A_FREE146		146	/* Unused */
#define A_FREE147		147	/* Unused */
#define A_FREE148		148	/* Unused */
#define A_FREE149		149	/* Unused */
#define A_FREE150		150	/* Unused */
#define A_FREE151		151	/* Unused */
#define A_FREE152		152	/* Unused */
#define A_FREE153		153	/* Unused */
#define A_FREE154		154	/* Unused */
#define A_FREE155		155	/* Unused */
#define A_FREE156		156	/* Unused */
#define A_FREE157		157	/* Unused */
#define A_FREE158		158	/* Unused */
#define A_FREE159		159	/* Unused */
#define A_FREE160		160	/* Unused */
#define A_FREE161		161	/* Unused */
#define A_FREE162		162	/* Unused */
#define A_FREE163		163	/* Unused */
#define A_FREE164		164	/* Unused */
#define A_FREE165		165	/* Unused */
#define A_FREE166		166	/* Unused */
#define A_FREE167		167	/* Unused */
#define A_FREE168		168	/* Unused */
#define A_FREE169		169	/* Unused */
#define A_FREE170		170	/* Unused */
#define A_FREE171		171	/* Unused */
#define A_FREE172		172	/* Unused */
#define A_FREE173		173	/* Unused */
#define A_FREE174		174	/* Unused */
#define A_FREE175		175	/* Unused */
#define A_FREE176		176	/* Unused */
#define A_FREE177		177	/* Unused */
#define A_FREE178		178	/* Unused */
#define A_FREE179		179	/* Unused */
#define A_FREE180		180	/* Unused */
#define A_FREE181		181	/* Unused */
#define A_FREE182		182	/* Unused */
#define A_FREE183		183	/* Unused */
#define A_FREE184		184	/* Unused */
#define A_FREE185		185	/* Unused */
#define A_FREE186		186	/* Unused */
#define A_FREE187		187	/* Unused */
#define A_FREE188		188	/* Unused */
#define A_FREE189		189	/* Unused */
#define A_FREE190		190	/* Unused */
#define A_FREE191		191	/* Unused */
#define A_FREE192		192	/* Unused */
#define A_FREE193		193	/* Unused */
#define A_FREE194		194	/* Unused */
#define A_FREE195		195	/* Unused */
#define A_FREE196		196	/* Unused */
#define A_FREE197		197	/* Unused */
#define A_MAILCC		198	/* Who is the mail Cc'ed to? */
#define A_MAILBCC		199	/* Who is the mail Bcc'ed to? */
#define A_LASTPAGE		200	/* Player last paged */
#define A_MAIL 			201	/* Message echoed to sender */
#define A_AMAIL			202	/* Action taken when mail received */
#define A_SIGNATURE		203	/* Mail signature */
#define A_DAILY			204	/* Daily attribute to be executed */
#define A_MAILTO		205	/* Who is the mail to? */
#define A_MAILMSG		206	/* The mail message itself */
#define A_MAILSUB		207	/* The mail subject */
#define A_MAILCURF		208	/* The current @mail folder */
#define A_LSPEECH		209	/* Speechlocks */
#define A_PROGCMD		210	/* Command for execution by @prog */
#define A_MAILFLAGS		211	/* Flags for extended mail */
#define A_DESTROYER		212	/* Who is destroying this object? */
#define A_NEWOBJS		213	/* New object array */
#define A_LCON_FMT		214	/* Player-specified contents format */
#define A_LEXITS_FMT	215	/* Player-specified exits format */
#define A_EXITVARDEST	216	/* Variable exit destination */
#define A_LCHOWN		217	/* ChownLock */
#define A_LASTIP		218	/* Last IP address logged in from */
#define A_LDARK			219	/* DarkLock */
#define A_VRML_URL		220	/* URL of the VRML scene for this object */
#define A_HTDESC		221	/* HTML @desc */
#define A_NAME_FMT		222	/* Player-specified name format */
#define A_LKNOWN		223	/* Who is this player seen by? (presence) */
#define A_LHEARD		224	/* Who is this player heard by? (speech) */
#define A_LMOVED		225	/* Who notices this player moving? */
#define A_LKNOWS		226	/* Who does this player see? (presence) */
#define A_LHEARS		227	/* Who does this player hear? (speech) */
#define A_LMOVES		228	/* Who does this player notice moving? */
#define A_SPEECHFMT		229	/* Format speech */
#define A_PAGEGROUP		230	/* Last paged as part of this group */
#define A_PROPDIR		231	/* Property directory dbref list */
#define A_FREE232		232	/* Unused */
#define A_FREE233		233	/* Unused */
#define A_FREE234		234	/* Unused */
#define A_FREE235		235	/* Unused */
#define A_FREE236		236	/* Unused */
#define A_FREE237		237	/* Unused */
#define A_FREE238		238	/* Unused */
#define A_FREE239		239	/* Unused */
#define A_FREE240		240	/* Unused */
#define A_FREE241		241	/* Unused */
#define A_FREE242		242	/* Unused */
#define A_FREE243		243	/* Unused */
#define A_FREE244		244	/* Unused */
#define A_FREE245		245	/* Unused */
#define A_FREE246		246	/* Unused */
#define A_FREE247		247	/* Unused */
#define A_FREE248		248	/* Unused */
#define A_FREE249		249	/* Unused */
#define A_FREE250		250	/* Unused */
#define A_FREE251		251	/* Unused */
#define A_FREE252		252	/* Unused Formerly A_VLIST */
#define A_LIST			253	/* A_VLIST */
#define A_FREE254		254	/* Unused Formerly A_STRUCT */
#define A_TEMP			255	/* Temporary */
#define A_USER_START	256	/* Start of user-named attributes */
#define ATR_BUF_CHUNK	100	/* Min size to allocate for attribute buffer */
#define ATR_BUF_INCR	6	/* Max size of one attribute */

/* XXX Need to convert to a function eventually */
#define Print_Attr_Flags(a,b,p) \
    p = b; \
    if (a & AF_LOCK) *p++ = '+'; \
    if (a & AF_NOPROG) *p++ = '$'; \
    if (a & AF_CASE) *p++ = 'C'; \
    if (a & AF_DEFAULT) *p++ = 'D'; \
    if (a & AF_HTML) *p++ = 'H'; \
    if (a & AF_PRIVATE) *p++ = 'I'; \
    if (a & AF_RMATCH) *p++ = 'M'; \
    if (a & AF_NONAME) *p++ = 'N'; \
    if (a & AF_NOPARSE) *p++ = 'P'; \
    if (a & AF_NOW) *p++ = 'Q'; \
    if (a & AF_REGEXP) *p++ = 'R'; \
    if (a & AF_STRUCTURE) *p++ = 'S'; \
    if (a & AF_TRACE) *p++ = 'T'; \
    if (a & AF_VISUAL) *p++ = 'V'; \
    if (a & AF_NOCLONE) *p++ = 'c'; \
    if (a & AF_DARK) *p++ = 'd'; \
    if (a & AF_GOD) *p++ = 'g'; \
    if (a & AF_CONST) *p++ = 'k'; \
    if (a & AF_MDARK) *p++ = 'm'; \
    if (a & AF_WIZARD) *p++ = 'w'; \
    *p = '\0';

#endif				/* __ATTRS_H */
