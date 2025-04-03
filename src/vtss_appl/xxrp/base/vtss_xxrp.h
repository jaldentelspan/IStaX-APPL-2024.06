/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef _VTSS_XXRP_XXRP_H_
#define _VTSS_XXRP_XXRP_H_

#include "vtss_xxrp_api.h"
#include "vtss_xxrp_mvrp.h"
#include "vtss_xxrp_mad.h"
#include "vtss_xxrp_madtt.h"

#define VTSS_XXRP_MAX_PDU_SIZE                1500                          /* Maximum MVRP PDU size          */
#define VTSS_XXRP_MIN_PDU_SIZE                25                            /* Minimum MVRP PDU size          */
#define VTSS_XXRP_MIN_MSG_SIZE                6                             /* Minimum MVRP msg size          */
#define VTSS_XXRP_VEC_ATTR_HDR_SIZE           4                             /* Vector header size             */
#define VTSS_MRP_ENDMARK                      0                             /* MRP End Mark                   */

#define VTSS_XXRP_APPL_EVENT_NEW              0                             /* New event                      */
#define VTSS_XXRP_APPL_EVENT_JOININ           1                             /* JoinIn event                   */
#define VTSS_XXRP_APPL_EVENT_IN               2                             /* In event                       */
#define VTSS_XXRP_APPL_EVENT_JOINMT           3                             /* JoinMt event                   */
#define VTSS_XXRP_APPL_EVENT_MT               4                             /* Mt event                       */
#define VTSS_XXRP_APPL_EVENT_LV               5                             /* Leave event                    */
#define VTSS_XXRP_APPL_EVENT_INVALID          6                             /* Invalid event                  */
#define VTSS_XXRP_APPL_EVENT_LA               0xFF                          /* LeaveAll event                 */
#define VTSS_XXRP_APPL_EVENT_TX_PKTS          7                             /* Transmitted frames             */
#define VTSS_XXRP_APPL_EVENT_INVALID_BYTE     ((VTSS_XXRP_APPL_EVENT_INVALID << 4) | VTSS_XXRP_APPL_EVENT_INVALID)

#define VTSS_XXRP_SET_EVENT(arr, indx, val)   ((indx % 2) ? (arr[indx/2] = ((arr[indx/2] & 0x0F) | (val << 4))) \
                                                          : (arr[indx/2] = ((arr[indx/2] & 0xF0) | val)))
#define VTSS_XXRP_GET_EVENT(arr, indx)        ((indx % 2) ? (((arr[indx/2]) >> 4) & 0xF) : ((arr[indx/2] & 0xF)))

typedef struct {
    u8    dst_mac[VTSS_XXRP_MAC_ADDR_LEN];
    u8    src_mac[VTSS_XXRP_MAC_ADDR_LEN];
    u8    eth_type[VTSS_XXRP_ETH_TYPE_LEN];
    u8    dsap;
    u8    lsap;
    u8    control;
} XXRP_ATTRIBUTE_PACKED xxrp_eth_hdr_t;

const char *vtss_mrp_event2txt(u8 event);

void mvrp_vid_to_mad_fsm_index(mesa_vid_t vid, u16 *mad_fsm_index);

mesa_rc vtss_mrp_process_leaveall(vtss_mrp_appl_t appl, u32 l2port);
mesa_rc vtss_mrp_process_new_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index);
mesa_rc vtss_mrp_process_joinin_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index);
mesa_rc vtss_mrp_process_in_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index);
mesa_rc vtss_mrp_process_joinmt_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index);
mesa_rc vtss_mrp_process_mt_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index);
mesa_rc vtss_mrp_process_lv_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index);
mesa_rc vtss_mrp_join_indication(vtss_mrp_appl_t appl, u32 l2port, u32 mad_indx, BOOL new_);
mesa_rc vtss_mrp_leave_indication(vtss_mrp_appl_t appl, u32 l2port, u32 mad_indx);
mesa_rc vtss_mrp_mad_process_events(vtss_mrp_appl_t appl, u32 l2port,
                                    u32 mad_fsm_indx, vtss_mad_fsm_events *fsm_events);
mesa_rc vtss_mrp_handle_periodic_timer(vtss_mrp_appl_t appl, u32 l2port);
mesa_rc vtss_xxrp_vlan_change_handler(vtss_mrp_appl_t application, u32 fsm_index, u32 port_no, BOOL is_add);
mesa_rc vtss_mrp_map_port_change_handler(vtss_mrp_appl_t appl, u32 l2port, BOOL add);
mesa_rc vtss_mrp_port_update_peer_mac_addr(vtss_mrp_appl_t appl, u32 l2port, u8 *mac_addr);
void vtss_xxrp_update_tx_stats(vtss_mrp_appl_t appl, u32 l2port, u8 event);
void vtss_xxrp_update_rx_stats(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_stat_type_t type);

#ifdef VTSS_SW_OPTION_MVRP
const char *mad_reg_state_to_txt(u8 state);
const char *mad_appl_state_to_txt(u8 state);
const char *mad_la_or_periodic_state_to_txt(u8 state);
#endif

mesa_rc vtss_mrp_port_control_conf_get(vtss_mrp_appl_t appl, u32 l2port, BOOL *const status);
#endif /* _VTSS_XXRP_XXRP_H_ */
