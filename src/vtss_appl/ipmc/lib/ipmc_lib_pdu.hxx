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

#ifndef _IPMC_LIB_PDU_HXX_
#define _IPMC_LIB_PDU_HXX_

#include "ipmc_lib_base.hxx"

// Maximum frame size we can handle.
#define IPMC_LIB_PDU_FRAME_SIZE_MAX 1518

// The IPv4 Protocol ID of IGMP
#define IPMC_LIB_PDU_IGMP_PROTOCOL_ID 2

// Convert PDU type to a string
const char *ipmc_lib_pdu_type_to_str(ipmc_lib_pdu_type_t type);

// Convert PDU version to a string
const char *ipmc_lib_pdu_version_to_str(ipmc_lib_pdu_version_t version);

void ipmc_lib_pdu_our_ip_get(const ipmc_lib_vlan_state_t &vlan_state, vtss_appl_ipmc_lib_ip_t &our_ip);
ipmc_lib_pdu_rx_action_t ipmc_lib_pdu_rx_parse(const uint8_t *frm, const mesa_packet_rx_info_t &rx_info, ipmc_lib_pdu_t &pdu);
void ipmc_lib_pdu_tx_group_specific_query(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, const ipmc_lib_src_list_t &src_list, mesa_port_no_t port_no, bool suppress_router_side_processing);
void ipmc_lib_pdu_tx_general_query(       ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no);
void ipmc_lib_pdu_tx_report(              ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr, vtss_appl_ipmc_lib_compatibility_t compat);
void ipmc_lib_pdu_tx_leave(               ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr);
uint32_t ipmc_lib_pdu_tx(const uint8_t *frame, size_t len, const mesa_port_list_t &dst_port_mask, bool force_untag, mesa_port_no_t src_port_no, mesa_vid_t vid, mesa_pcp_t pcp, mesa_dei_t dei);
void ipmc_lib_pdu_statistics_update(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, bool is_rx, bool ignored);

#endif /* _IPMC_LIB_PDU_HXX_ */

