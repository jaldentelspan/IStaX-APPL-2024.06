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

/*****************************************************************************
* This file contains code for handling received LLDP FRAME PREEMPTION frames.
******************************************************************************/


#include "fp_shared.h" // For FP_TLV_SUBTYPE
#include "vtss/appl/tsn.h"
#include "lldp_trace.h"
#include "lldp_tlv.h"

//
// Append Frame Preemption to the LLDP frame being build.
//
// In: port_idx - The port in question
//
// In/Out: Buf - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
static lldp_u8_t fp_update_tlv(lldp_u8_t *buf, lldp_port_t port_idx)
{
    mesa_rc                      rc = VTSS_RC_OK;
    int                          buf_len = 0;
    vtss_appl_tsn_fp_status_t    tsn_fp_status;
    uint16_t                     fp_tlv_word = 0;
    vtss_ifindex_t               index;

    if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_idx, &index) != VTSS_RC_OK) {
        T_E("vtss_ifindex_from_port failed  port %u", port_idx);
        return 0;
    }

    rc = vtss_appl_tsn_fp_status_get(index, &tsn_fp_status);
    if (rc != VTSS_RC_OK) {
        T_E("vtss_appl_tsn_fp_status_get failed  port %u ", port_idx);
        return 0;
    }

    T_DG_PORT(TRACE_GRP_FP, port_idx, "%s", "fp_update_tlv");

    // OUI - Figure 79-6, IEEE802.3br_D2p0
    buf[buf_len++] = 0x00;
    buf[buf_len++] = 0x12;
    buf[buf_len++] = 0x0F;

    buf[buf_len++] = FP_TLV_SUBTYPE; // Subtype - TBD Figure 79-6, IEEE802.3br_D2p0

    // Additional Ethernet capabilities Figure 79-6, IEEE802.3br_D2p0
    // bit 0 - preemption capability support
    // bit 1 - preemption capability status
    // bit 2 - preemption capability active
    // bit 3-4 - additional fragment size

    if (tsn_fp_status.loc_preempt_supported) {
        fp_tlv_word |= 0x01;
    }
    if (tsn_fp_status.loc_preempt_enabled) {
        fp_tlv_word |= (0x01 << 1);
    }
    if (tsn_fp_status.loc_preempt_active) {
        fp_tlv_word |= (0x01 << 2);
    }

    fp_tlv_word |= (tsn_fp_status.loc_add_frag_size & 0x03) << 3;

    buf[buf_len++] = (fp_tlv_word >> 8) & 0xFF; // bit 15:8 reserved
    buf[buf_len++] = fp_tlv_word & 0xFF;


    T_DG_PORT(TRACE_GRP_FP_TX, port_idx, "capability support %d status %d active %d frag size %d word %d",
              tsn_fp_status.loc_preempt_supported, tsn_fp_status.loc_preempt_enabled,
              tsn_fp_status.loc_preempt_active, tsn_fp_status.loc_add_frag_size, fp_tlv_word);
    return buf_len;
}

// Function for adding Frame Preemption TLV information
//
// In: port_idx - The port in question
//
// In/Out: Buf - Pointer to buffer containing the LLDP frame
//
// Return the length of the TLV.
lldp_u16_t fp_tlv_add(lldp_u8_t *buf, lldp_port_t port_idx)
{
    lldp_u16_t tlv_info_len = 0;

    // Organizationally TLV
    tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len,
                                                          LLDP_TLV_ORG_TLV,
                                                          fp_update_tlv(buf + 2 + tlv_info_len, port_idx));

    T_DG_PORT(TRACE_GRP_FP, port_idx, "tlv_info_len = %d", tlv_info_len);
    return tlv_info_len;
}


