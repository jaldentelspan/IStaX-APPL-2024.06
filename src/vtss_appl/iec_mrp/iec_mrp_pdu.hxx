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

#ifndef _IEC_MRP_PDU_HXX_
#define _IEC_MRP_PDU_HXX_

#include "iec_mrp_base.hxx"

void mrp_pdu_test_tx_update(        mrp_state_t *mrp_state, bool tx, uint32_t usec);
void mrp_pdu_topology_change_tx(    mrp_state_t *mrp_state, uint16_t mrp_interval_msec);
void mrp_pdu_link_change_tx(        mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool link_up);
void mrp_pdu_option_tx(             mrp_state_t *mrp_state, mrp_pdu_sub_tlv_type_t sub_tlv_type, uint16_t other_prio, mesa_mac_t *other_mac);
void mrp_pdu_in_test_tx_update(     mrp_state_t *mrp_state, bool tx, uint32_t usec);
void mrp_pdu_in_topology_change_tx( mrp_state_t *mrp_state, uint16_t mrp_interval_msec);
void mrp_pdu_in_link_change_tx(     mrp_state_t *mrp_state, bool link_up, uint16_t mrp_interval_msec);
void mrp_pdu_in_link_status_poll_tx(mrp_state_t *mrp_state);
mesa_rc mrp_pdu_create_all(            mrp_state_t *mrp_state);
void    mrp_pdu_free_all(              mrp_state_t *mrp_state);
void    mrp_pdu_forward(               mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, uint32_t length, vtss_appl_iec_mrp_pdu_type_t pdu_type);
bool    mrp_pdu_rx_frame(              mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, mrp_rx_pdu_info_t &rx_pdu_info);

const char *mrp_pdu_tlv_type_to_str(mrp_pdu_tlv_type_t tlv_type);
const char *mrp_pdu_sub_tlv_type_to_str(mrp_pdu_sub_tlv_type_t sub_tlv_type);

#endif /* _IEC_MRP_PDU_HXX_ */

