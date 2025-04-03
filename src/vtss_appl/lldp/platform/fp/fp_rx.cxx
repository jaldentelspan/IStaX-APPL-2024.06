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

/*****************************************************************************
* This file contains code for handling received LLDP Frame Preemption frames.
******************************************************************************/

#include "fp_shared.h" /* For FP_TLV_SUBTYPE */
#include "lldp_remote.h"
#include "lldp_trace.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

// Validates an Frame Preemption TLV and updates the LLDP entry table with the information.
//
// In: tlv - Pointer to the TLV that shall be validated
//     len - The length of the TLV
//
// Out: rx_entry - Pointer to the entry that shall have information updated
//
// Return: TRUE if the TLV was accepted.
lldp_bool_t fp_validate_lldpdu(lldp_u8_t *tlv, lldp_rx_remote_entry_t *rx_entry, lldp_u16_t len)
{
    lldp_bool_t org_tlv_supported = LLDP_FALSE;
    // Switch on tlv subtype
    switch (tlv[5]) {
    case FP_TLV_SUBTYPE :
        if (len >= 6) {
            // Length is normally 6 for this TLV, but should be able to handle other lengths
            // according to IEEE Std 802.3br-2016: 79.3.7.1
            rx_entry->fp.RemFramePreemptSupported  = (CHECK_BIT(tlv[7], 0)) ? LLDP_TRUE : LLDP_FALSE;
            rx_entry->fp.RemFramePreemptEnabled    = (CHECK_BIT(tlv[7], 1)) ? LLDP_TRUE : LLDP_FALSE;
            rx_entry->fp.RemFramePreemptActive     = (CHECK_BIT(tlv[7], 2)) ? LLDP_TRUE : LLDP_FALSE;
            rx_entry->fp.RemFrameAddFragSize       = ((tlv[7] >> 3) & 0x03);
            org_tlv_supported = TRUE;
            T_DG_PORT(TRACE_GRP_FP_RX, rx_entry->receive_port,
                      "framePreemptSupported = %u, framePreemptEnabled = %d, framePreemptActive = %d frameAddFragSize = %d",
                      rx_entry->fp.RemFramePreemptSupported, rx_entry->fp.RemFramePreemptEnabled,
                      rx_entry->fp.RemFramePreemptActive, rx_entry->fp.RemFrameAddFragSize);
        } else {
            // IEEE Std 802.3br-2016 79.3.7.1: If fewer octet(s) are received than defined,
            // the implementation shall act as if the additional octet(s) were received as zero.
            T_DG_PORT(TRACE_GRP_FP_RX, rx_entry->receive_port,
                      "framePreempt pdu with short length = %u received. Should minimum be 6. Acting as all zeroes received", len );
            rx_entry->fp.RemFramePreemptSupported  = LLDP_FALSE;
            rx_entry->fp.RemFramePreemptEnabled    = LLDP_FALSE;
            rx_entry->fp.RemFramePreemptActive     = LLDP_FALSE;
            rx_entry->fp.RemFrameAddFragSize       = 0;
            org_tlv_supported = TRUE;
        }
        break;
    default:
        org_tlv_supported = LLDP_FALSE;
    }

    T_DG(TRACE_GRP_FP, "Validating LLDP for Frame Preemption %d, type = %d", org_tlv_supported, tlv[5]);
    return org_tlv_supported;
}

// Returns true if the LLDP entry table needs to be updated with the information in the received LLDP frame.
//
// In : rx_entry - Pointer to the last received lldp information
//      entry    - Pointer to the current lldp information stored in the entry table.
//
// Return: TRUE if current entry information isn't equal to the last received lldp information
lldp_bool_t fp_update_necessary(lldp_rx_remote_entry_t *rx_entry, vtss_appl_lldp_remote_entry_t *entry)
{
    return memcmp(&rx_entry->fp, &entry->fp, sizeof(vtss_appl_lldp_fp_t)) == 0 ? LLDP_FALSE : LLDP_TRUE;
}

/// Copies the information from the received packet (rx_enty) to the entry table.
//
//  In : rx_entry - Pointer to the last received lldp information.
//       entry    - Pointer to the entry that shall be updated
void fp_update_entry(lldp_rx_remote_entry_t *rx_entry, vtss_appl_lldp_remote_entry_t *entry)
{
    entry->fp = rx_entry->fp;
}

