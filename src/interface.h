/* interface.h -- network-related definitions */

#include "copyright.h"

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "externs.h"

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* (Dis)connection reason codes */

#define	R_GUEST		1	/* Guest connection */
#define	R_CREATE	2	/* User typed 'create' */
#define	R_CONNECT	3	/* User typed 'connect' */
#define	R_DARK		4	/* User typed 'cd' */

#define	R_QUIT		5	/* User quit */
#define	R_TIMEOUT	6	/* Inactivity timeout */
#define	R_BOOT		7	/* Victim of @boot, @toad, or @destroy */
#define	R_SOCKDIED	8	/* Other end of socket closed it */
#define	R_GOING_DOWN	9	/* Game is going down */
#define	R_BADLOGIN	10	/* Too many failed login attempts */
#define	R_GAMEDOWN	11	/* Not admitting users now */
#define	R_LOGOUT	12	/* Logged out w/o disconnecting */
#define R_GAMEFULL	13	/* Too many players logged in */

/* Logged out command table definitions */

#define CMD_QUIT	1
#define CMD_WHO		2
#define CMD_DOING	3
#define CMD_PREFIX	5
#define CMD_SUFFIX	6
#define CMD_LOGOUT	7
#define CMD_SESSION	8
#define CMD_PUEBLOCLIENT 9
#define CMD_INFO	10

#define CMD_MASK	0xff
#define CMD_NOxFIX	0x100

extern NAMETAB logout_cmdtable[];

typedef struct cmd_block CBLK;
typedef struct cmd_block_hdr CBLKHDR;
struct cmd_block_hdr
{
    struct cmd_block *nxt;
};
struct cmd_block
{
    CBLKHDR hdr;
    char	cmd[LBUF_SIZE - sizeof(CBLKHDR)];
};

typedef struct text_block TBLOCK;
typedef struct text_block_hdr TBLKHDR;
struct text_block_hdr
{
    struct text_block *nxt;
    char	*start;
    char	*end;
    int	nchars;
};
struct text_block
{
    TBLKHDR hdr;
    char	data[OUTPUT_BLOCK_SIZE - sizeof(TBLKHDR)];
};

typedef struct prog_data PROG;
struct prog_data
{
    dbref wait_cause;
    GDATA *wait_data;
};

typedef struct descriptor_data DESC;
struct descriptor_data
{
    int descriptor;
    int flags;
    int retries_left;
    int command_count;
    int timeout;
    int host_info;
    char addr[51];
    char username[11];
    char doing[DOING_LEN];
    dbref player;
    int *colormap;
    char *output_prefix;
    char *output_suffix;
    int output_size;
    int output_tot;
    int output_lost;
    TBLOCK *output_head;
    TBLOCK *output_tail;
    int input_size;
    int input_tot;
    int input_lost;
    CBLK *input_head;
    CBLK *input_tail;
    CBLK *raw_input;
    char *raw_input_at;
    time_t connected_at;
    time_t last_time;
    int quota;
    PROG *program_data;
    struct sockaddr_in address;	/* added 3/6/90 SCG */
    struct descriptor_data *hashnext;
    struct descriptor_data *next;
    struct descriptor_data **prev;
};

/* flags in the flag field */
#define	DS_CONNECTED	0x0001		/* player is connected */
#define	DS_AUTODARK	0x0002		/* Wizard was auto set dark. */
#define DS_PUEBLOCLIENT 0x0004          /* Client is Pueblo-enhanced. */

extern DESC *descriptor_list;

#ifndef HAVE_GETTIMEOFDAY
#define get_tod(x)	{ (x)->tv_sec = time(NULL); (x)->tv_usec = 0; }
#else
#define get_tod(x)	gettimeofday(x, NULL)
#endif

/* from the net interface */

extern void	NDECL(emergency_shutdown);
extern void	FDECL(shutdownsock, (DESC *, int));
extern void	FDECL(shovechars, (int));
extern void	NDECL(set_signals);

/* from netcommon.c */

extern struct timeval	FDECL(timeval_sub, (struct timeval, struct timeval));
extern int	FDECL(msec_diff, (struct timeval now, struct timeval then));
extern struct timeval	FDECL(msec_add, (struct timeval, int));
extern struct timeval	FDECL(update_quotas, (struct timeval, struct timeval));
extern void	FDECL(raw_notify, (dbref, char *));
extern void	FDECL(raw_notify_newline, (dbref));
extern void	FDECL(clearstrings, (DESC *));
extern void	FDECL(queue_write, (DESC *, const char *, int));
extern INLINE void	FDECL(queue_string, (DESC *, const char *));
extern void	FDECL(freeqs, (DESC *));
extern void	FDECL(welcome_user, (DESC *));
extern void	FDECL(save_command, (DESC *, CBLK *));
extern void	FDECL(announce_disconnect, (dbref, DESC *, const char *));
extern int	FDECL(boot_off, (dbref, char *));
extern int	FDECL(boot_by_port, (int, int, char *));
extern void	NDECL(check_idle);
extern void	NDECL(process_commands);
extern int	FDECL(site_check, (struct in_addr, SITE *));
extern dbref	FDECL(find_connected_name, (dbref, char *));

/* From predicates.c */

#define alloc_desc(s) (DESC *)pool_alloc(POOL_DESC,s)
#define free_desc(b) pool_free(POOL_DESC,((char **)&(b)))

#define DESC_ITER_PLAYER(p,d) \
	for (d=(DESC *)nhashfind((int)p,&mudstate.desc_htab);d;d=d->hashnext)
#define DESC_ITER_CONN(d) \
	for (d=descriptor_list;(d);d=(d)->next) \
		if ((d)->flags & DS_CONNECTED)
#define DESC_ITER_ALL(d) \
	for (d=descriptor_list;(d);d=(d)->next)

#define DESC_SAFEITER_PLAYER(p,d,n) \
	for (d=(DESC *)nhashfind((int)p,&mudstate.desc_htab), \
        	n=((d!=NULL) ? d->hashnext : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->hashnext : NULL))
#define DESC_SAFEITER_CONN(d,n) \
	for (d=descriptor_list,n=((d!=NULL) ? d->next : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->next : NULL)) \
		if ((d)->flags & DS_CONNECTED)
#define DESC_SAFEITER_ALL(d,n) \
	for (d=descriptor_list,n=((d!=NULL) ? d->next : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->next : NULL))

#endif /* __INTERFACE_H */
