/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_UPNP_H_
#define _VTSS_UPNP_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */
#include "critd_api.h"
#include "vtss_upnp_api.h"
#include "vtss_module_id.h"
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>

/* We have two devices on hand, D_Link DIR-635 and Accton ES3526XA, supporting UPnP.
 * Those two devices use source port 1900 to repsonse M-search messages from control
 * points. It does not make sense but we keep this flag to have the backward
 * compatibility to old control points.
 */
#define VTSS_BACKWARD_COMPATIBLE    0
#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_UPNP

/* UPnP ACE IDs */
#define UPNP_SSDP_ACE_ID                1
#define IGMP_QUERY_ACE_ID               2

#ifdef VTSS_SW_OPTION_IP
#define UPNP_IP_INTF_MAX_OPST           4
#define UPNP_IP_INTF_OPST_UP(x)         ((x) ? (((x)->type == VTSS_APPL_IP_IF_STATUS_TYPE_LINK) ? (((x)->u.link.flags&VTSS_APPL_IP_IF_LINK_FLAG_UP) && ((x)->u.link.flags&VTSS_APPL_IP_IF_LINK_FLAG_RUNNING)) : FALSE) : FALSE)
#define UPNP_IP_INTF_OPST_ADR4(x)       ((x) ? (((x)->type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) ? ((x)->u.ipv4.net.address) : 0) : 0)
#define UPNP_IP_INTF_OPST_GET(x, y, z)  (vtss_appl_ip_if_status_get((x), VTSS_APPL_IP_IF_STATUS_TYPE_ANY, UPNP_IP_INTF_MAX_OPST, &(z), (y)) == VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_IP */

/* ================================================================= *
 *  UPNP configuration blocks
 * ================================================================= */

/* Block versions */
#define UPNP_CONF_BLK_VERSION      1

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================= *
 *  UPNP event definitions
 * ================================================================= */
#define UPNP_EVENT_DB_CHANGE 0x00000001
#define UPNP_EVENT_FIND_IP   0x00000002
#define UPNP_EVENT_SET_ACL   0x00000004
#define UPNP_EVENT_RESTART   0x00000008
#define UPNP_EVENT_ANY       0xffffffff /* Any possible bit */
#define UPNP_EVENT_WAIT_TO_DEF    10000
#define UPNP_EVENT_WAIT_TO_LONG   86400000

/* ================================================================= *
 *  UPNP stack messages
 * ================================================================= */

/* UPNP messages IDs */
typedef enum {
#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
    UPNP_MSG_ID_UPNP_CONF_SET_REQ,          /* UPNP configuration set request (no reply) */
#endif
    UPNP_MSG_ID_FRAME_RX_IND,               /* Frame receive indication */
} upnp_msg_id_t;

/* Request message */
typedef struct {
    /* Message ID */
    upnp_msg_id_t    msg_id;

    /* Request data, depending on message ID */
    union {
        /* UPNP_MSG_ID_UPNP_CONF_SET_REQ */
        struct {
            vtss_appl_upnp_param_t  conf;
        } conf_set;

        /* UPNP_MSG_ID_FRAME_RX_IND */
        struct {
            size_t              len;
            unsigned long       vid;
            unsigned long       port_no;
        } rx_ind;

    } req;
} upnp_msg_req_t;

/* UPNP message buffer */
typedef struct {
    vtss_sem_t   *sem; /* Semaphore */
    void         *msg; /* Message */
} upnp_msg_buf_t;

/* ================================================================= *
 *  UPNP global structure
 * ================================================================= */

/* UPNP global structure */
typedef struct {
    critd_t     crit;
    /* UPnP device IP address and Interface VLAN ID */
    char        upnp_dev_ip_running[UPNP_MGMT_IPSTR_SIZE];
    mesa_vid_t  upnp_dev_vid_running;
    vtss_appl_upnp_param_t upnp_conf;
    /* Request buffer and semaphore */
    struct {
        vtss_sem_t     sem;
        upnp_msg_req_t msg;
    } request;

} upnp_global_t;

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_UPNP_H_ */

