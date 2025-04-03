/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _REDBOX_PDU_HXX_
#define _REDBOX_PDU_HXX_

extern const mesa_mac_t redbox_multicast_dmac;

mesa_rc  redbox_pdu_tx_templates_create(   redbox_state_t &redbox_state);
void     redbox_pdu_tx_free(               redbox_state_t &redbox_state);
void     redbox_pdu_tx_start(              redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr);
void     redbox_pdu_tx_stop(               redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr);
void     redbox_pdu_frame_interval_changed(redbox_state_t &redbox_state);
bool     redbox_pdu_rx_frame(const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, redbox_pdu_info_t &rx_pdu_info, bool hsr_tagged);
uint32_t redbox_pdu_tx_hsr_to_prp_sv(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, mesa_vlan_tag_t &class_tag);
void     redbox_pdu_tx_prp_to_hsr_sv(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, mesa_vlan_tag_t &class_tag, bool vlan_tagged);
vtss_appl_redbox_sv_type_t redbox_pdu_tlv_type_to_sv_type(redbox_tlv_type_t tlv_type);

#endif /* _REDBOX_PDU_HXX_ */

