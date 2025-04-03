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

#include "aps_base.hxx"
#include "aps_laps.hxx"
#include "aps_trace.h"
#include "cfm_api.h"    /* For CFM_ETYPE */

// As long as we can't receive a frame behind two tags, the LAPS PDU contents
// always starts at offset 14 (two MACs + EtherType) because the packet module
// strips a possible outer tag.
#define APS_LAPS_PDU_START_OFFSET 14
#define APS_LAPS_TX_TIMEOUT_MS    5000
#define APS_LAPS_RX_TIMEOUT_MS    ((35 * APS_LAPS_TX_TIMEOUT_MS) / 10)

// The DMAC used in APS PDUs. The least significant nibble of the last byte is
// modified by the code to match the MD level. This is a.k.a. Class1 Multicast.
const mesa_mac_t aps_multicast_dmac = {{0x01, 0x80, 0xc2, 0x00, 0x00, 0x30}};

/******************************************************************************/
// APS_LAPS_tx_frame_generate()
/******************************************************************************/
static mesa_rc APS_LAPS_tx_frame_generate(aps_state_t *aps_state, packet_tx_props_t *tx_props)
{
    packet_tx_info_t      *packet_info;
    mesa_packet_tx_info_t *tx_info;
    uint8_t               *frm;
    uint32_t              offset, frm_len;
    size_t                vid_len;
    bool                  tagged;
    mesa_vid_t            vid;

    // Gotta initialize it ourselves.
    packet_tx_props_init(tx_props);

    tagged  = aps_state->is_tagged();
    vid     = tagged ? aps_state->conf.vlan : 0;
    vid_len = tagged ? 4 : 0;

    // Figure out how much space we need
    frm_len = 6       /* DMAC                      */ +
              6       /* SMAC                      */ +
              vid_len /* Possible VLAN tag         */ +
              2       /* Ethertype                 */ +
              4       /* Common CFM Header         */ +
              4       /* APS-Specific Information  */ +
              1       /* End TLV                   */;

    packet_info = &tx_props->packet_info;

    if ((packet_info->frm = packet_tx_alloc(MAX(60, frm_len))) == NULL) {
        T_EG(APS_TRACE_GRP_FRAME_TX, "Out of memory");
        return VTSS_APPL_APS_RC_OUT_OF_MEMORY;
    }

    packet_info->modid     = VTSS_MODULE_ID_APS;
    packet_info->len       = frm_len;
    packet_info->no_free   = TRUE;
    tx_info                = &tx_props->tx_info;
    tx_info->pdu_offset    = 14 + vid_len;
    tx_info->dst_port_mask = VTSS_BIT64(aps_state->p_port_state->port_no);
    tx_info->pipeline_pt   = MESA_PACKET_PIPELINE_PT_REW_PORT_VOE;
    tx_info->tag.vid       = 0;
    tx_info->tag.pcp       = aps_state->conf.pcp;
    tx_info->tag.dei       = 0;
    tx_info->cos           = aps_state->conf.pcp;
    tx_info->cosid         = aps_state->conf.pcp;
    tx_info->dp            = 0;

    frm = tx_props->packet_info.frm;
    memset(frm, 0, MAX(60, frm_len));
    offset = 0;

    //
    // Layer 2
    //

    // DMAC
    memcpy(&frm[offset], aps_multicast_dmac.addr, sizeof(aps_multicast_dmac.addr));

    // Change last nibble of DMAC to match the MD level
    frm[offset + 5] |= aps_state->conf.level;
    offset += sizeof(aps_multicast_dmac.addr);

    // SMAC
    memcpy(&frm[offset], aps_state->smac.addr, sizeof(aps_state->smac.addr));

    offset += sizeof(aps_state->smac.addr);

    // Possible VLAN tag
    if (vid_len) {
        // Insert TPID
        frm[offset++] = aps_state->p_port_state->tpid >> 8;
        frm[offset++] = aps_state->p_port_state->tpid >> 0;

        // Compute PCP, DEI (= 0), and VID
        frm[offset++] = (aps_state->conf.pcp << 5) | ((vid >> 8) & 0x0F);
        frm[offset++] = vid & 0xFF;
    }

    // EtherType
    frm[offset++] = (CFM_ETYPE >> 8) & 0xFF;
    frm[offset++] = (CFM_ETYPE >> 0) & 0xFF;

    //
    // CFM header
    //

    // MD level + Version (= 0)
    frm[offset++] = aps_state->conf.level << 5;

    // OpCode = L-APS
    frm[offset++] = APS_OPCODE_LAPS;

    // Flags Always 0.
    frm[offset++] = 0;

    // First TLV Offset. Number of bytes after this one until first TLV comes.
    // Always 4.
    frm[offset++] = 4;

    //
    // L-APS
    //
    aps_state->tx_aps_info_offset = offset; // Save this for later updates
    memcpy(&frm[offset], aps_state->tx_aps_info, sizeof(aps_state->tx_aps_info));
    offset += sizeof(aps_state->tx_aps_info);

    //
    // End TLV
    //
    frm[offset++] = 0;

    // Final check of length
    if (offset != frm_len) {
        T_EG(APS_TRACE_GRP_FRAME_TX, "Pre-calculated L-APS PDU length (%u) doesn't match final length (%u)", frm_len, offset);
        packet_tx_free(frm);
        tx_props->packet_info.frm = nullptr;
        return VTSS_APPL_APS_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// APS_LAPS_rx_w_timeout()
// Timer == rx_w_timer
/******************************************************************************/
static void APS_LAPS_rx_w_timeout(aps_timer_t &timer, void *context)
{
    aps_base_rx_timeout((aps_state_t *)context, true);
}

/******************************************************************************/
// APS_LAPS_rx_p_timeout()
// Timer == rx_p_timer
/******************************************************************************/
static void APS_LAPS_rx_p_timeout(aps_timer_t &timer, void *context)
{
    aps_base_rx_timeout((aps_state_t *)context, false);
}

/******************************************************************************/
// APS_LAPS_tx_timeout()
// Timer == tx_timer
/******************************************************************************/
static void APS_LAPS_tx_timeout(aps_timer_t &timer, void *context)
{
    aps_state_t *aps_state = (aps_state_t *)context;
    mesa_rc     rc;

    VTSS_ASSERT(aps_state);

    if (aps_state->p_port_state->link) {
        // This timer fires whenever it's time to send one LAPS PDU
        T_NG(APS_TRACE_GRP_FRAME_TX, "%u: Tx LAPS", aps_state->inst);

        // Print the IFH.
        packet_debug_tx_props_print(VTSS_MODULE_ID_APS, APS_TRACE_GRP_FRAME_TX, VTSS_TRACE_LVL_DEBUG, &aps_state->tx_props);

        // And the frame.
        T_DG_HEX(APS_TRACE_GRP_FRAME_TX, aps_state->tx_props.packet_info.frm, aps_state->tx_props.packet_info.len);

        if ((rc = packet_tx(&aps_state->tx_props)) != VTSS_RC_OK) {
            T_EG(APS_TRACE_GRP_FRAME_TX, "%u: packet_tx() failed: %s", aps_state->inst, error_txt(rc));
        }

        aps_state->status.tx_cnt++;
    } else {
        T_NG(APS_TRACE_GRP_FRAME_TX, "%u: No link, so no Tx of LAPS PDU", aps_state->inst);
    }
}

/******************************************************************************/
// APS_LAPS_frame_tx_cancel()
/******************************************************************************/
static void APS_LAPS_frame_tx_cancel(aps_state_t *aps_state)
{
    // Stop Tx timer (in case of manual injection)
    aps_timer_stop(aps_state->tx_timer);

    // Free the packet pointer
    if (aps_state->tx_props.packet_info.frm) {
        packet_tx_free(aps_state->tx_props.packet_info.frm);
        aps_state->tx_props.packet_info.frm = nullptr;
    }

    aps_state->tx_props.packet_info.len = 0;
    aps_state->laps_pdu_transmitted = false;
}

/******************************************************************************/
// APS_LAPS_frame_tx_start()
/******************************************************************************/
static void APS_LAPS_frame_tx_start(aps_state_t *aps_state)
{
    int i;

    // Tx three frames ASAP according to G.8031, clause 11.2.4
    for (i = 0; i < 3; i++) {
        APS_LAPS_tx_timeout(aps_state->tx_timer, aps_state);
    }

    aps_state->laps_pdu_transmitted = true;

    // And (re-)start the timer (fires every 5 seconds)-
    aps_timer_start(aps_state->tx_timer, APS_LAPS_TX_TIMEOUT_MS, true);
}

/******************************************************************************/
// packet_tx_props_t::operator==
// Cannot memcmp() the whole structure, since the packet_info.frm pointer may
// differ between the two structs.
/******************************************************************************/
static bool operator==(const packet_tx_props_t &lhs, const packet_tx_props_t &rhs)
{
    if (memcmp(&lhs.tx_info, &rhs.tx_info, sizeof(lhs.tx_info)) != 0) {
        return false;
    }

    if (lhs.packet_info.len != rhs.packet_info.len) {
        return false;
    }

    // Special handling if one or both of the frames are NULL pointers, because
    // memcmp() would otherwise cause a core dump.
    if (lhs.packet_info.frm == nullptr && rhs.packet_info.frm == nullptr) {
        return true;
    } else if (lhs.packet_info.frm == nullptr || rhs.packet_info.frm == nullptr) {
        return false;
    }

    return memcmp(lhs.packet_info.frm, rhs.packet_info.frm, lhs.packet_info.len) == 0;
}

/******************************************************************************/
// aps_laps_statistics_clear()
/******************************************************************************/
void aps_laps_statistics_clear(aps_state_t *aps_state)
{
    aps_state->status.rx_valid_cnt   = 0;
    aps_state->status.rx_invalid_cnt = 0;
    aps_state->status.tx_cnt         = 0;
}

/******************************************************************************/
// aps_laps_tx_frame_update()
// This will re-generate the frame, because one or more of the fields of the PDU
// (not Tx APS Info) has changed.
/******************************************************************************/
mesa_rc aps_laps_tx_frame_update(aps_state_t *aps_state, bool do_tx)
{
    packet_tx_props_t new_tx_props;

    if (!aps_state->tx_laps_pdus()) {
        APS_LAPS_frame_tx_cancel(aps_state);
        return VTSS_RC_OK;
    }

    // Try to generate a new LAPS PDU
    VTSS_RC(APS_LAPS_tx_frame_generate(aps_state, &new_tx_props));

    // See packet_tx_props_t::operator==
    if (new_tx_props == aps_state->tx_props) {
        // No change in LAPS PDU. Keep the old one going and delete the new.
        T_DG(APS_TRACE_GRP_FRAME_TX, "%u: No change in LAPS PDU", aps_state->inst);

        packet_tx_free(new_tx_props.packet_info.frm);
        return VTSS_RC_OK;
    }

    // Cancel the old LAPS PDU (if any)
    APS_LAPS_frame_tx_cancel(aps_state);

    T_IG(APS_TRACE_GRP_FRAME_TX, "%u: New LAPS PDU of length %u w/o FCS", aps_state->inst, new_tx_props.packet_info.len);

    // Print the IFH.
    packet_debug_tx_props_print(VTSS_MODULE_ID_APS, APS_TRACE_GRP_FRAME_TX, VTSS_TRACE_LVL_DEBUG, &new_tx_props);

    // And the frame.
    T_DG_HEX(APS_TRACE_GRP_FRAME_TX, new_tx_props.packet_info.frm, new_tx_props.packet_info.len);

    // Start the new
    aps_state->tx_props = new_tx_props;

    if (do_tx) {
        APS_LAPS_frame_tx_start(aps_state);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// aps_laps_tx_info_update()
// This will just replace the Tx APS in the frame if it has changed.
/******************************************************************************/
void aps_laps_tx_info_update(aps_state_t *aps_state, bool shutdown)
{
    uint8_t *frm = aps_state->tx_props.packet_info.frm;

    if (frm) {
        // The frame is active. If the Tx APS info has changed or if we are
        // shutting down or we haven't yet sent the first LAPS PDU, we must
        // transmit the frame three times in a row and start a 5 sec. timer for
        // further transmissions.
        if (!aps_state->laps_pdu_transmitted ||
            shutdown                         ||
            memcmp(aps_state->tx_aps_info, &frm[aps_state->tx_aps_info_offset], sizeof(aps_state->tx_aps_info))) {
            memcpy(&frm[aps_state->tx_aps_info_offset], aps_state->tx_aps_info, sizeof(aps_state->tx_aps_info));

            // Transmit it three times and restart the Tx timer.
            APS_LAPS_frame_tx_start(aps_state);
        }

        if (shutdown) {
            // Free the frame and stop the timer.
            APS_LAPS_frame_tx_cancel(aps_state);
        }
    }
}

/******************************************************************************/
// APS_LAPS_rx_validation_pass()
/******************************************************************************/
static bool APS_LAPS_rx_validation_pass(aps_state_t *aps_state, const uint8_t *const frm, uint32_t rx_length, uint8_t rx_aps_info[APS_LAPS_DATA_LENGTH])
{
    uint8_t  level;
    uint32_t offset = APS_LAPS_PDU_START_OFFSET, tlv_len, first_tlv_offset;
    uint8_t  tlv_type;

    if (rx_length < APS_LAPS_PDU_START_OFFSET + 9) {
        // Version 0 of LAPS PDU is exactly 9 bytes long (after DMAC, SMAC, and
        // ethertype), but we accept unknown TLVs.
        return false;
    }

    // SMAC
    memcpy(aps_state->status.smac.addr, &frm[6], sizeof(aps_state->status.smac.addr));

    //
    // CFM header
    //

    offset = APS_LAPS_PDU_START_OFFSET;

    // MD level + Version. We must not check the version field, since we only
    // support version 0. Level has already been determined.
    level = frm[offset++] >> 5;
    if (level != aps_state->conf.level) {
        // Not for us.
        T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Rx frame at MEL %u. Expected %u", aps_state->inst, level, aps_state->conf.level);
        return false;
    }

    // OpCode. Already checked to be APS_OPCODE_LAPS
    offset++;

    // Be forgiving against non-zero flags.
    offset++;

    // First TLV offset. Must be 4.
    if ((first_tlv_offset = frm[offset]) != 4) {
        T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Invalid First TLV Offset (%u)", aps_state->inst, frm[offset]);
        return false;
    }

    // APS-specific information comes right after First TLV offset
    memcpy(rx_aps_info, &frm[offset + 1], APS_LAPS_DATA_LENGTH);

    // offset is now pointing to the first byte after First TLV Offset, which is
    // where first_tlv_offset_is counting from. Skip past everything until the
    // first TLV.
    offset += first_tlv_offset;

    // The fixed length header does not overrun the length of the PDU.
    if (rx_length < offset) {
        T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Length (%u) < offset (%u)", aps_state->inst, rx_length, offset);
        return false;
    }

    // Loop through TLVs, searching for an end TLV.
    while (offset < rx_length) {
        tlv_type = frm[offset++];
        if (tlv_type == 0) {
            // End TLV. Do not process anything beyond an End TLV.
            return true;
        }

        // Get length of this TLV. This only includes what comes after this
        // field.
        tlv_len  = (frm[offset + 0] << 8) | (frm[offset + 1] << 0);
        offset += 2;

        // Skip past this TLV. offset now points to the Type field of the next
        // TLV - if any
        offset += tlv_len;

        if (offset > rx_length) {
            // The frame was not long enough to accommodate this TLV's length
            // field.
            T_IG(APS_TRACE_GRP_FRAME_RX, "%u: TLV of type %u with length = %u caused frame to go beyond the frame length (%u)", aps_state->inst, tlv_type, tlv_len, rx_length);
            return false;
        }
    }

    // If the End TLV is NOT mandatory, change this from false to true.
    T_IG(APS_TRACE_GRP_FRAME_RX, "%u: No End TLV found", aps_state->inst);
    return false;
}

/******************************************************************************/
// aps_laps_rx_frame()
/******************************************************************************/
bool aps_laps_rx_frame(aps_state_t *aps_state, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, uint8_t aps[APS_LAPS_DATA_LENGTH])
{
    bool     working = rx_info->port_no == aps_state->w_port_state->port_no;
    uint8_t request_state;

    T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Frame Rx on %c port", aps_state->inst, working ? 'W' : 'P');

    if (working) {
        // Don't attempt to validate nor count this frame. Just register that we
        // have received it and (re-)start a W-port timer. The purpose of this
        // timer is to let the base know when we haven't received a LAPS PDU on
        // the working port for 3.5 * 5 = 17.5 seconds.
        // According to G.8021, Table 6-2, we need to clear the dFOP-CM defect
        // when we haven't received a LAPS PDU on working for that many seconds.
        aps_timer_start(aps_state->rx_w_timer, APS_LAPS_RX_TIMEOUT_MS, false);
        return false;
    }

    if (!APS_LAPS_rx_validation_pass(aps_state, frm, rx_info->length, aps)) {
        T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Frame failed validation", aps_state->inst);
        aps_state->status.rx_invalid_cnt++;
        return false;
    }

    // We have to ignore LAPS PDUs where the request/state is invalid, that is,
    // not save it in our local state. If we at one point in time had received
    // a valid LAPS PDU, the rx_p_timer will be started. If we suddenly start
    // receiving invalid LAPS PDUs, that rx_p_timer will timeout and a dFOP_TO
    // event will be set in the base, then.
    request_state = (aps[0] & 0xF0) >> 4;
    if (request_state == 0x3 || request_state == 0x6 || request_state == 0x8 || request_state == 0xA || request_state == 0xC) {
        T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Invalid APS request/state (0x%02x) received. Ignoring", aps_state->inst, aps[0]);
        aps_state->status.rx_invalid_cnt++;
        return false;
    }

    // Re-start APS Rx P Timer if received a PDU on protect port.
    // According to G.8031, clause 11.15.d and G.8021, Table 6-2, dFOP-TO
    // detection, we must set the dFOP-TO defect if we haven't received a valid
    // LAPS PDU on the protect entity for 3.5 * 5 = 17.5 seconds.
    if (!aps_state->sf_p) {
        // Only start it if we don't have SF on protect port.
        aps_timer_start(aps_state->rx_p_timer, APS_LAPS_RX_TIMEOUT_MS, false);
    }

    aps_state->status.rx_valid_cnt++;
    return true;
}

/******************************************************************************/
// aps_laps_sf_sd_set()
/******************************************************************************/
void aps_laps_sf_sd_set(aps_state_t *aps_state, bool working, bool new_sf, bool new_sd)
{
    if (working) {
        // We only care about SF on the protect port.
        return;
    }

    if (new_sf || new_sd) {
        // Cannot time out if there's SF on the protect port.
        aps_timer_stop(aps_state->rx_p_timer);
    } else {
        // No longer SF on protect port. Start the Rx-P timer unless it's
        // already active.
        if (!aps_timer_active(aps_state->rx_p_timer)) {
            aps_timer_start(aps_state->rx_p_timer, APS_LAPS_RX_TIMEOUT_MS, false);
        }
    }
}

/******************************************************************************/
// aps_laps_state_init()
/******************************************************************************/
void aps_laps_state_init(aps_state_t *aps_state)
{
    aps_timer_init(aps_state->rx_p_timer, "Rx-P timer", aps_state->inst, APS_LAPS_rx_p_timeout, aps_state);
    aps_timer_init(aps_state->rx_w_timer, "Rx-W timer", aps_state->inst, APS_LAPS_rx_w_timeout, aps_state);
    aps_timer_init(aps_state->tx_timer,   "Tx timer",   aps_state->inst, APS_LAPS_tx_timeout,   aps_state);

    aps_state->laps_pdu_transmitted = false;
    memset(&aps_state->status.smac, 0, sizeof(aps_state->status.smac));
    aps_laps_statistics_clear(aps_state);
}

