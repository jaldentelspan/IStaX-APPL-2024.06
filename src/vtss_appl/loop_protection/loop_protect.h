/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

#ifndef _VTSS_LOOP_PROTECT_H_
#define _VTSS_LOOP_PROTECT_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_LOOP_PROTECT

#include <vtss_trace_api.h>

#include "critd_api.h"
#include "loop_protect_api.h"

#define SHA1_HASH_SIZE 4  /* Dummy */

typedef struct {
    vtss_appl_loop_protect_conf_t global;
    CapArray<vtss_appl_loop_protect_port_conf_t, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;
} lprot_conf_t;

#define LPROT_ACE_ID        1    /* ID of sole ACE entry */
#define LPROT_TTL_NOCHECK   2    /* Seconds */
#define LPROT_MAX_TTL_MSECS 5000 /* Msecs */

typedef struct {
    u8  dst[6];
    u8  src[6];
    u16 oui;                    /* 9003 */
#define LPROT_PROTVERSION 1
    u16 version;
    u32 tstamp;                 /* Aligned! */
    u8  switchmac[6];
    u16 usid;
    u16 lport;
    u8  authcode[20];
    u8  pad[8];
} __attribute__((packed)) loop_prot_pdu_t;

/* LOOP_PROTECT messages IDs */
typedef enum {
    LOOP_PROTECT_MSG_ID_CONF,
    LOOP_PROTECT_MSG_ID_CONF_PORT,
    LOOP_PROTECT_MSG_ID_PORT_CTL,
    LOOP_PROTECT_MSG_ID_PORT_STATUS_REQ,
    LOOP_PROTECT_MSG_ID_PORT_STATUS_RSP,
} loop_protect_msg_id_t;

typedef struct loop_protect_msg {
    loop_protect_msg_id_t msg_id;
    /* Message data, depending on message ID */
    union {
        struct {
            u8 key[SHA1_HASH_SIZE];
            u8 mac[6];
            vtss_usid_t usid;
            vtss_appl_loop_protect_conf_t global_conf;
        } unit_conf;
        struct {
            mesa_port_no_t port_no;
            vtss_appl_loop_protect_port_conf_t port_conf;
        } port_conf;
        struct {
            mesa_port_no_t port_no;
            BOOL           disable;
        } port_ctl;
        struct {
        } port_info;
    } data;
} loop_protect_msg_t;

#endif /* _VTSS_LOOP_PROTECT_H_ */

