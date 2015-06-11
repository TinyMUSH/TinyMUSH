/* quota.c - quota management commands */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"		/* required by mudconf */
#include "game.h"		/* required by mudconf */
#include "alloc.h"		/* required by mudconf */
#include "flags.h"		/* required by mudconf */
#include "htab.h"		/* required by mudconf */
#include "ltdl.h"		/* required by mudconf */
#include "udb.h"		/* required by mudconf */
#include "udb_defs.h"		/* required by mudconf */

#include "mushconf.h"		/* required by code */

#include "db.h"			/* required by externs */
#include "interface.h"
#include "externs.h"		/* required by code */

#include "match.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */


/* ---------------------------------------------------------------------------
 * load_quota: Load a quota into an array. Expects A_QUOTA or A_QUOTA.
 */

void load_quota(int q_list[], dbref player, int qtype)
{
    int i, aowner, aflags, alen;
    char *quota_str, *p, *tokst;
    quota_str = atr_get(player, qtype, &aowner, &aflags, &alen);

    if (!*quota_str) {
	for (i = 0; i < 5; i++) {
	    q_list[i] = 0;
	}

	free_lbuf(quota_str);
	return;
    }

    for (p = strtok_r(quota_str, " ", &tokst), i = 0; p && (i < 5); p = strtok_r(NULL, " ", &tokst), i++) {
	q_list[i] = (int) strtol(p, (char **) NULL, 10);
    }

    free_lbuf(quota_str);
}

/* ---------------------------------------------------------------------------
 * save_quota: turns a quota array into an attribute.
 */
void save_quota(int q_list[], dbref player, int qtype)
{
    char buf[MBUF_SIZE];
    sprintf(buf, "%d %d %d %d %d", q_list[0], q_list[1], q_list[2], q_list[3], q_list[4]);
    atr_add_raw(player, qtype, buf);
}

/* ---------------------------------------------------------------------------
 * mung_quota, show_quota, do_quota: Manage quotas.
 */

void count_objquota(dbref player, int *aq, int *rq, int *eq, int *tq, int *pq)
{
    register int a, r, e, t, p, i;
    a = r = e = t = p = 0;
    DO_WHOLE_DB(i) {
	if ((Owner(i) != player) || (Going(i) && !isRoom(i))) {
	    continue;
	}

	switch (Typeof(i)) {
	case TYPE_ROOM:
	    a += mudconf.room_quota;
	    r++;
	    break;

	case TYPE_EXIT:
	    a += mudconf.exit_quota;
	    e++;
	    break;

	case TYPE_THING:
	    a += mudconf.thing_quota;
	    t++;
	    break;

	case TYPE_PLAYER:
	    a += mudconf.player_quota;
	    p++;
	    break;
	}
    }
    *aq = a;
    *rq = r;
    *eq = e;
    *tq = t;
    *pq = p;
}


void adjust_quota(dbref player, int qtype, int value, int key)
{
    register int aq, rq;
    int q_list[5], rq_list[5];
    load_quota(q_list, player, A_QUOTA);
    load_quota(rq_list, player, A_RQUOTA);
    aq = q_list[qtype];
    rq = rq_list[qtype];

    /*
     * Adjust values
     */

    if (key & QUOTA_REM) {
	aq += (value - rq);
	rq = value;
    } else {
	rq += (value - aq);
	aq = value;
    }

    /*
     * Set both abs and relative quota
     */
    q_list[qtype] = aq;
    rq_list[qtype] = rq;
    save_quota(q_list, player, A_QUOTA);
    save_quota(rq_list, player, A_RQUOTA);
}


void mung_quotas(dbref player, int key, int value)
{
    int xq, rooms, exits, things, players;
    int q_list[5], rq_list[5];

    if (key & QUOTA_FIX) {
	/*
	 * Get value of stuff owned and good value, set other values
	 * * from that.
	 */
	count_objquota(player, &xq, &rooms, &exits, &things, &players);

	if (key & QUOTA_TOT) {
	    load_quota(rq_list, player, A_RQUOTA);
	    rq_list[QTYPE_ALL] += xq;
	    save_quota(rq_list, player, A_QUOTA);
	} else {
	    load_quota(q_list, player, A_QUOTA);
	    q_list[QTYPE_ALL] -= xq;
	    q_list[QTYPE_ROOM] -= rooms;
	    q_list[QTYPE_EXIT] -= exits;
	    q_list[QTYPE_THING] -= things;
	    q_list[QTYPE_PLAYER] -= players;
	    save_quota(q_list, player, A_RQUOTA);
	}
    } else if (key & QUOTA_ROOM) {
	adjust_quota(player, QTYPE_ROOM, value, key);
    } else if (key & QUOTA_EXIT) {
	adjust_quota(player, QTYPE_EXIT, value, key);
    } else if (key & QUOTA_THING) {
	adjust_quota(player, QTYPE_THING, value, key);
    } else if (key & QUOTA_PLAYER) {
	adjust_quota(player, QTYPE_PLAYER, value, key);
    } else {
	adjust_quota(player, QTYPE_ALL, value, key);
    }
}

void show_quota(dbref player, dbref victim)
{
    int q_list[5], rq_list[5], dq_list[5], i;
    load_quota(q_list, victim, A_QUOTA);
    load_quota(rq_list, victim, A_RQUOTA);

    for (i = 0; i < 5; i++) {
	dq_list[i] = q_list[i] - rq_list[i];
    }

    if (Free_Quota(victim)) {
	if (mudconf.typed_quotas) {
	    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - N/A  %4d - N/A  %4d - N/A  %4d - N/A  %4d - N/A", Name(victim), dq_list[QTYPE_ALL], dq_list[QTYPE_ROOM], dq_list[QTYPE_EXIT], dq_list[QTYPE_THING], dq_list[QTYPE_PLAYER]);
	} else {
	    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - N/A", Name(victim), dq_list[QTYPE_ALL]);
	}
    } else {
	if (mudconf.typed_quotas) {
	    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - %3d  %4d - %3d  %4d - %3d  %4d - %3d  %4d - %3d", Name(victim), dq_list[QTYPE_ALL], q_list[QTYPE_ALL], dq_list[QTYPE_ROOM], q_list[QTYPE_ROOM], dq_list[QTYPE_EXIT], q_list[QTYPE_EXIT], dq_list[QTYPE_THING], q_list[QTYPE_THING], dq_list[QTYPE_PLAYER], q_list[QTYPE_PLAYER]);
	} else {
	    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "%-16s: %4d - %3d", Name(victim), dq_list[QTYPE_ALL], q_list[QTYPE_ALL]);
	}
    }
}

void show_quota_header(dbref player)
{
    if (mudconf.typed_quotas)
	notify_quiet(player, "Name            : Quot - Lim  Room - Lim  Exit - Lim  Thin - Lim  Play - Lim");
    else {
	notify_quiet(player, "Name            : Quot - Lim");
    }
}

void do_quota(dbref player, __attribute__((unused)) dbref cause, int key, char *arg1, char *arg2)
{
    dbref who;
    char *name, *target;
    register int set = 0, value = 0, i;

    if (!(mudconf.quotas || Can_Set_Quota(player))) {
	notify_quiet(player, "Quotas are not enabled.");
	return;
    }

    if ((key & QUOTA_TOT) && (key & QUOTA_REM)) {
	notify_quiet(player, "Illegal combination of switches.");
	return;
    }

    /*
     * Show or set all quotas if requested
     */

    if (key & QUOTA_ALL) {
	if (arg1 && *arg1) {
	    value = (int) strtol(arg1, (char **) NULL, 10);
	    set = 1;

	    if (value < 0) {
		notify(player, "Illegal quota value.");
		return;
	    }
	} else if (key & (QUOTA_SET | QUOTA_FIX)) {
	    value = 0;
	    set = 1;
	}

	if (set) {
	    name = log_getname(player);
	    log_write(LOG_WIZARD, "WIZ", "QUOTA", "%s changed everyone's quota.", name);
	    free_lbuf(name);
	}

	show_quota_header(player);
	DO_WHOLE_DB(i) {
	    if (isPlayer(i)) {
		if (set) {
		    mung_quotas(i, key, value);
		}

		show_quota(player, i);
	    }
	}
	return;
    }

    /*
     * Find out whose quota to show or set
     */

    if (!arg1 || *arg1 == '\0') {
	who = Owner(player);
    } else {
	who = lookup_player(player, arg1, 1);

	if (!Good_obj(who)) {
	    notify_quiet(player, "Not found.");
	    return;
	}
    }

    /*
     * Make sure we have permission to do it
     */

    if (!Can_Set_Quota(player)) {
	if (arg2 && *arg2) {
	    notify_quiet(player, NOPERM_MESSAGE);
	    return;
	}

	if (Owner(player) != who) {
	    notify_quiet(player, NOPERM_MESSAGE);
	    return;
	}
    }

    if (arg2 && *arg2) {
	set = 1;
	value = (int) strtol(arg2, (char **) NULL, 10);

	if (value < 0) {
	    notify(player, "Illegal quota value.");
	    return;
	}
    } else if (key & QUOTA_FIX) {
	set = 1;
	value = 0;
    }

    if (set) {
	name = log_getname(player);
	target = log_getname(who);
	log_write(LOG_WIZARD, "WIZ", "QUOTA", "%s changed the quota of %s", name, target);
	free_lbuf(name);
	free_lbuf(target);
	mung_quotas(who, key, value);
    }

    show_quota_header(player);
    show_quota(player, who);
}
