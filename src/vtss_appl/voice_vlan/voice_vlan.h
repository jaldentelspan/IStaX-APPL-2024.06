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

#ifndef _VTSS_VOICE_VLAN_H_
#define _VTSS_VOICE_VLAN_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include "voice_vlan_api.h"
#include "vtss_module_id.h"
#ifdef VTSS_SW_OPTION_LLDP
#include "lldp_api.h"
#endif

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_VOICE_VLAN

#include <vtss_trace_api.h>

/* ================================================================= *
 *  VOICE_VLAN configuration header
 * ================================================================= */

/* VOICE_VLAN OUI configuration */
typedef struct {
    u32                     entry_num;
    voice_vlan_oui_entry_t  entry[VOICE_VLAN_OUI_ENTRIES_CNT];
} voice_vlan_oui_conf_t;

/* VOICE_VLAN LLDP telephony MAC */
typedef struct {
    u32                                     entry_num;
    CapArray<voice_vlan_lldp_telephony_mac_entry_t, VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT> entry;
} voice_vlan_lldp_telephony_mac_t;


/* ================================================================= *
 *  VOICE_VLAN stack messages
 * ================================================================= */

/* VOICE_VLAN messages IDs */
typedef enum {
    VOICE_VLAN_MSG_ID_LLDP_CB_IND,      /* LLDP callback indication */
    VOICE_VLAN_MSG_ID_QCE_SET_REQ      /* Voice VLAN QCE configuration set request (no reply) */
} voice_vlan_msg_id_t;

/* Request message */
typedef struct {
    /* Message ID */
    voice_vlan_msg_id_t    msg_id;

    /* Request data, depending on message ID */
    union {
        /* VOICE_VLAN_MSG_ID_PORT_CHANGE_CB_IND */
        struct {
            mesa_port_no_t  port_no;
            BOOL            link;
        } port_change_cb_ind;

        /* VOICE_VLAN_MSG_ID_LLDP_CB_IND */
        struct {
            mesa_port_no_t      port_no;
#ifdef VTSS_SW_OPTION_LLDP
            vtss_appl_lldp_remote_entry_t entry;
#endif
        } lldp_cb_ind;

        /* VOICE_VLAN_MSG_ID_QCE_SET_REQ */
        struct {
            BOOL            del_qce;
            BOOL            is_port_list;
            BOOL            port_list[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];
            BOOL            is_port_add;
            BOOL            is_port_del;
            mesa_port_no_t  iport;
            BOOL            change_traffic_class;
            mesa_prio_t     traffic_class;
            BOOL            change_vid;
            mesa_vid_t      vid;
        } qce_conf_set;
    } req;
} voice_vlan_msg_req_t;


/* ================================================================= *
 *  VOICE_VLAN global structure
 * ================================================================= */

/* VOICE_VLAN global structure */
typedef struct {
    critd_t                         crit;
    voice_vlan_conf_t               conf;
    vtss_flag_t                     conf_flags;
    vtss_mtimer_t                   conf_timer[VTSS_ISID_END];
    voice_vlan_port_conf_t          port_conf[VTSS_ISID_END];
    voice_vlan_oui_conf_t           oui_conf;
    voice_vlan_lldp_telephony_mac_t lldp_telephony_mac;
    CapArray<u32, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> oui_cnt;
    CapArray<BOOL, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> sw_learn;
    CapArray<mesa_qce_id_t, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> qcl_no;

    /* Request buffer and semaphore */
    struct {
        vtss_sem_t           sem;
        voice_vlan_msg_req_t msg;
    } request;

} voice_vlan_global_t;

#endif /* _VTSS_VOICE_VLAN_H_ */

