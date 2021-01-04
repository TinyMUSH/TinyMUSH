/**
 * @file interface.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Network-related definitions
 * @version 3.3
 * @date 2021-01-04
 * 
 * @copyright Copyright (C) 1989-2021 TinyMUSH development team.
 * 
 */

/* interface.h -- Network-related definitions */

#include "copyright.h"

#ifndef __INTERFACE_H
#define __INTERFACE_H

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

typedef struct cmd_block_hdr
{
    struct cmd_block *nxt;
} CBLKHDR;
typedef struct cmd_block
{
    CBLKHDR hdr;
    char cmd[LBUF_SIZE - sizeof(CBLKHDR)];
} CBLK;

typedef struct text_block_hdr
{
    struct text_block *nxt;
    char *start;
    char *end;
    int nchars;
} TBLKHDR;

typedef struct text_block
{
    TBLKHDR hdr;
    char *data;
} TBLOCK;

typedef struct prog_data
{
    dbref wait_cause;
    GDATA *wait_data;
} PROG;

typedef struct descriptor_data
{
    int descriptor;
    int flags;
    int retries_left;
    int command_count;
    int timeout;
    int host_info;
    char addr[51];
    char username[11];
    char *doing;
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
    struct sockaddr_in address;
    struct descriptor_data *hashnext;
    struct descriptor_data *next;
    struct descriptor_data **prev;
} DESC;

/* flags in the flag field */
#define DS_CONNECTED 0x0001    /*!< player is connected */
#define DS_AUTODARK 0x0002     /*!< Wizard was auto set dark. */
#define DS_PUEBLOCLIENT 0x0004 /*!< Client is Pueblo-enhanced. */

extern DESC *descriptor_list;

#endif /* __INTERFACE_H */
