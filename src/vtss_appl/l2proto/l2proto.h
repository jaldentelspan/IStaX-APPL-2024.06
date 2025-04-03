/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _L2PROTO_H_
#define _L2PROTO_H_

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_L2PROTO

#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_PACKET  1

#include <vtss_trace_api.h>

/* L2 messages IDs */
typedef enum {
    L2_MSG_ID_FRAME_RX_IND,     /* Frame receive indication */
} l2_msg_id_t;

typedef enum {
    L2_PORT_TYPE_VLAN,          /* VLAN */
    L2_PORT_TYPE_POAG,          /* Physical switch port / LLAG */
    L2_PORT_TYPE_GLAG,          /* GLAG */
} l2_port_type;

typedef struct {
    l2_msg_id_t msg_id;

    /* Message data, depending on message ID */
    union {
        /* L2_MSG_ID_FRAME_RX_IND */
        struct {
            vtss_module_id_t modid;
            mesa_port_no_t   switchport;
            size_t           len;
            mesa_vid_t       vid;
            mesa_glag_no_t   glag_no;
        } rx_ind;

    } data;
} l2_msg_t;

#define STP_STATE_CHANGE_REG_MAX 10

static void l2local_flush_vport(l2_port_type type, int port, vtss_common_vlanid_t vlan_id);

#endif /* _L2PROTO_H_ */

