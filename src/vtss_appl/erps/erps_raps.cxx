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

#include "erps_base.hxx"
#include "erps_raps.hxx"
#include "erps_trace.h"
#include "cfm_api.h"    /* For CFM_ETYPE */

// As long as we can't receive a frame behind two tags, the R-APS PDU contents
// always starts at offset 14 (two MACs + EtherType) because the packet module
// strips a possible outer tag.
#define ERPS_RAPS_PDU_START_OFFSET 14

// The DMAC used in R-APS PDUs.
const mesa_mac_t erps_raps_dmac = {{0x01, 0x19, 0xa7, 0x00, 0x00, 0x01}};

/******************************************************************************/
// ERPS_RAPS_tx_frame_generate()
/******************************************************************************/
static mesa_rc ERPS_RAPS_tx_frame_generate(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, int p, packet_tx_props_t *tx_props)
{
    erps_ring_port_state_t *ring_port_state = &erps_state->ring_port_state[ring_port];
    erps_port_state_t      *port_state = ring_port_state->port_states[p];
    packet_tx_info_t       *packet_info;
    mesa_packet_tx_info_t  *tx_info;
    uint8_t                *frm;
    uint32_t               offset, frm_len;
    size_t                 vid_len;
    mesa_vid_t             vid;

    // Gotta initialize it ourselves.
    packet_tx_props_init(tx_props);

    // Always send tagged
    vid     = erps_state->conf.control_vlan;
    vid_len = 4;

    // Figure out how much space we need
    frm_len = 6       /* DMAC                       */ +
              6       /* SMAC                       */ +
              vid_len /* Possible VLAN tag          */ +
              2       /* Ethertype                  */ +
              4       /* Common CFM Header          */ +
              32      /* R-APS-Specific Information */ +
              1       /* End TLV                    */;

    packet_info = &tx_props->packet_info;

    if ((packet_info->frm = packet_tx_alloc(MAX(60, frm_len))) == NULL) {
        T_EG(ERPS_TRACE_GRP_FRAME_TX, "Out of memory");
        return VTSS_APPL_ERPS_RC_OUT_OF_MEMORY;
    }

    packet_info->modid   = VTSS_MODULE_ID_ERPS;
    packet_info->len     = frm_len;
    packet_info->no_free = TRUE;

    tx_info                = &tx_props->tx_info;
    tx_info->pdu_offset    = 14 + vid_len;
    tx_info->dst_port_mask = VTSS_BIT64(port_state->port_no);
    tx_info->pipeline_pt   = MESA_PACKET_PIPELINE_PT_REW_PORT_VOE;
    tx_info->tag.vid       = 0;
    tx_info->tag.pcp       = erps_state->conf.pcp;
    tx_info->tag.dei       = 0;
    tx_info->cos           = erps_state->conf.pcp;
    tx_info->cosid         = erps_state->conf.pcp;
    tx_info->dp            = 0;

    frm = tx_props->packet_info.frm;
    memset(frm, 0, MAX(60, frm_len));
    offset = 0;

    //
    // Layer 2
    //

    // DMAC
    memcpy(&frm[offset], erps_raps_dmac.addr, sizeof(erps_raps_dmac.addr));

    // Change last nibble of DMAC to become the ring ID
    frm[offset + 5] = erps_state->conf.ring_id;
    offset += sizeof(erps_raps_dmac.addr);

    // SMAC
    memcpy(&frm[offset], ring_port_state->smac.addr, sizeof(ring_port_state->smac.addr));
    offset += sizeof(ring_port_state->smac.addr);

    // VLAN tag
    if (vid_len) {
        // Insert TPID
        frm[offset++] = port_state->tpid >> 8;
        frm[offset++] = port_state->tpid >> 0;

        // Compute PCP, DEI (= 0), and VID
        frm[offset++] = (erps_state->conf.pcp << 5) | ((vid >> 8) & 0x0F);
        frm[offset++] = vid & 0xFF;
    }

    // EtherType
    frm[offset++] = (CFM_ETYPE >> 8) & 0xFF;
    frm[offset++] = (CFM_ETYPE >> 0) & 0xFF;

    //
    // CFM header
    //

    // MD level + Version (= 0 for v1, 1 for v2)
    frm[offset++] = (erps_state->conf.level << 5) | erps_state->conf.version;

    // OpCode = R-APS
    frm[offset++] = ERPS_OPCODE_RAPS;

    // Flags Always 0.
    frm[offset++] = 0;

    // First TLV Offset. Number of bytes after this one until first TLV comes.
    // Always 32.
    frm[offset++] = 32;

    // R-APS. There's 32 bytes of R-APS-specific bytes of which only the first
    // 2 bytes vary. The next 6 bytes are the Node ID, which by default is the
    // switch's own MAC address (which may be overridden by configuration), and
    // the 24 last bytes are reserved and must be sent as all-zeros.
    ring_port_state->tx_raps_info_offset[p] = offset; // Save this for later updates
    memcpy(&frm[offset], erps_state->tx_info, sizeof(erps_state->tx_info));
    offset += sizeof(erps_state->tx_info);

    // Node ID (6 bytes).
    memcpy(&frm[offset], erps_state->status.tx_raps_info.node_id.addr, sizeof(erps_state->status.tx_raps_info.node_id.addr));
    offset += sizeof(erps_state->status.tx_raps_info.node_id.addr);

    // 24 bytes all-zeros. frm already memset() to 0, so just skip ahead.
    offset += 24;

    //
    // End TLV
    //
    frm[offset++] = 0;

    // Final check of length
    if (offset != frm_len) {
        T_EG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s: Pre-calculated R-APS PDU length (%u) doesn't match final length (%u)", erps_state->inst, ring_port, frm_len, offset);
        packet_tx_free(frm);
        tx_props->packet_info.frm = nullptr;
        return VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// ERPS_RAPS_tx()
/******************************************************************************/
static bool ERPS_RAPS_tx(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, int p)
{
    erps_itr_t             connected_ring_itr;
    erps_ring_port_state_t *ring_port_state = &erps_state->ring_port_state[ring_port];
    erps_port_state_t      *port_state      = ring_port_state->port_states[p];
    uint32_t               connected_ring_inst;
    mesa_rc                rc;

    if (port_state == nullptr || ring_port_state->tx_props[p].packet_info.frm == nullptr) {
        // No frame generated for this ring-port's sub-index
        return false;
    }

    if (!port_state->link) {
        T_NG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s:%d: No link on %u, so no Tx of R-APS PDU", erps_state->inst, ring_port, p, port_state->port_no);
        return false;
    }

    if (ring_port == VTSS_APPL_ERPS_RING_PORT1 && erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB && erps_state->conf.virtual_channel) {
        // Gotta check the connected ring's block state, because in reality we
        // should use up-injections for these frames, so that they get into the
        // control VLAN and get sent to the non-blocked ring ports of the
        // connected  ring, but this implementation has chosen to use
        // down-injections, which means that we must obey the connected ring's
        // block/unblock ring-port states, since our R-APS PDUs are just normal
        // service traffic for that ring.
        // This should be OK, because in G.8032, clause 9.7.1, right below
        // Figure 9-7, it says:
        // "R-APS messages of this sub-ring that are forwarded over its R-APS
        //  virtual channel are broadcast or multicast over the interconnected
        //  network. For this reason, the broadcast or multicast domain of the
        //  R-APS virtual channel could span only the interconnecting Ethernet
        //  rings or sub-rings that are necessary for forwarding R-APS messages
        //  for this sub-ring."
        //
        // We won't get here unless the connected ring instance is really a ring
        // with two ring ports and is active, because then port_states[p] would
        // be nullptrs.
        connected_ring_inst = erps_state->conf.interconnect_conf.connected_ring_inst;
        if ((connected_ring_itr = ERPS_map.find(connected_ring_inst)) == ERPS_map.end()) {
            T_EG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s:%d: Connected ring instance (%u) not found", erps_state->inst, ring_port, p, connected_ring_inst);
            return false;
        }

        if (connected_ring_itr->second.status.ring_port_status[p].blocked) {
            T_DG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s: Connected ring (inst %u)'s port%d is blocked for service traffic.", erps_state->inst, ring_port, connected_ring_inst, p);
            return false;
        }
    }

    T_NG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s:%d: Tx R-APS", erps_state->inst, ring_port, p);
    if ((rc = packet_tx(&ring_port_state->tx_props[p])) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s:%d: packet_tx() failed: %s", erps_state->inst, ring_port, p, error_txt(rc));
        return false;
    }

    return true;
}

/******************************************************************************/
// ERPS_RAPS_tx_statistics_update()
/******************************************************************************/
static void ERPS_RAPS_tx_statistics_update(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, vtss_appl_erps_raps_info_t &tx_info, uint32_t cnt)
{
    vtss_appl_erps_statistics_t &s = erps_state->status.ring_port_status[ring_port].statistics;

    switch (tx_info.request) {
    case VTSS_APPL_ERPS_REQUEST_NR:
        if (tx_info.rb) {
            s.tx_nr_rb_cnt += cnt;
        } else {
            s.tx_nr_cnt += cnt;
        }

        break;

    case VTSS_APPL_ERPS_REQUEST_MS:
        s.tx_ms_cnt += cnt;
        break;

    case VTSS_APPL_ERPS_REQUEST_SF:
        s.tx_sf_cnt += cnt;
        break;

    case VTSS_APPL_ERPS_REQUEST_FS:
        s.tx_fs_cnt += cnt;
        break;

    case VTSS_APPL_ERPS_REQUEST_EVENT:
        s.tx_event_cnt += cnt;
        break;

    default:
        T_EG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s: Invalid request (%d)", erps_state->inst, ring_port, tx_info.request);
        break;
    }
}

/******************************************************************************/
// ERPS_RAPS_tx_timeout()
// Timer == tx_timer
/******************************************************************************/
static void ERPS_RAPS_tx_timeout(erps_timer_t &timer, void *context)
{
    vtss_appl_erps_ring_port_t ring_port;
    erps_state_t               *erps_state = (erps_state_t *)context;
    bool                       pdu_transmitted;
    int                        p;

    VTSS_ASSERT(erps_state);

    if (!erps_state->status.tx_raps_active) {
        // We are not supposed to transmit any R-APS PDUs now.
        T_NG(ERPS_TRACE_GRP_FRAME_TX, "%u: Not supposed to transmit R-APS PDUs", erps_state->inst);
        return;
    }

    // This timer fires whenever it's time to send one R-APS PDU on both ring
    // ports.
    // We transmit frames on both ring ports whether they are blocked or not.
    // One exception is with interconnected sub-rings w/o virtual channel,
    // where we only send on port0.
    // Another exception is with interconnected sub-rings w/ virtual channel,
    // where we only send on connected ring's non-blocked ports (because this
    // should have been an un-injection).
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        // Count only once per ring port, even though we might send it twice on
        // an interconnected sub-ring's connected ring's ring ports.
        pdu_transmitted = false;

        for (p = 0; p < ARRSZ(erps_state->ring_port_state[ring_port].tx_props); p++) {
            if (ERPS_RAPS_tx(erps_state, ring_port, p)) {
                pdu_transmitted = true;
            }
        }

        if (pdu_transmitted) {
            ERPS_RAPS_tx_statistics_update(erps_state, ring_port, erps_state->status.tx_raps_info, 1 /* sent once */);
        }
    }
}

/******************************************************************************/
// ERPS_RAPS_frame_tx_cancel()
/******************************************************************************/
static void ERPS_RAPS_frame_tx_cancel(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, int p)
{
    erps_ring_port_state_t *ring_port_state = &erps_state->ring_port_state[ring_port];

    // Stop Tx timer (in case of manual injection)
    erps_timer_stop(erps_state->tx_timer);

    // Free the packet pointer
    if (ring_port_state->tx_props[p].packet_info.frm) {
        packet_tx_free(ring_port_state->tx_props[p].packet_info.frm);
        ring_port_state->tx_props[p].packet_info.frm = nullptr;
    }

    ring_port_state->tx_props[p].packet_info.len = 0;
    ring_port_state->raps_pdu_transmitted = false;
}

/******************************************************************************/
// ERPS_RAPS_frame_tx_three_times()
// Returns true if at least one frame was transmitted. on this ring port.
/******************************************************************************/
static bool ERPS_RAPS_frame_tx_three_times(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, int p, bool is_event)
{
    erps_ring_port_state_t *ring_port_state = &erps_state->ring_port_state[ring_port];
    int                    i;
    bool                   pdu_transmitted = false;

    if (!erps_state->status.tx_raps_active && !is_event) {
        // We are not supposed to transmit any R-APS PDUs now and this is not
        // an event.
        T_NG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s:%d Not supposed to transmit R-APS PDUs", erps_state->inst, ring_port, p);
        return false;
    }

    // Either this is an event, which must be sent whether tx_raps_active is set
    // or not or tx_raps_active is set.

    // Tx three frames as fast as possible according to G.8032, clause 10.1.3.
    for (i = 0; i < 3; i++) {
        if (ERPS_RAPS_tx(erps_state, ring_port, p)) {
            pdu_transmitted = true;
        }
    }

    ring_port_state->raps_pdu_transmitted = pdu_transmitted;

    return pdu_transmitted;
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
// erps_raps_tx_frame_update()
// This will re-generate frames for a given ring port because one or more of the
// fields of the PDU (not Tx Info) has changed.
/******************************************************************************/
static mesa_rc ERPS_RAPS_tx_frame_update(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, int p)
{
    erps_ring_port_state_t *ring_port_state = &erps_state->ring_port_state[ring_port];
    packet_tx_props_t      new_tx_props, *tx_props;

    T_IG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s: p = %d", erps_state->inst, ring_port, p);

    if (ring_port_state->port_states[p] == nullptr) {
        // This ring port is not/no longer used.
        ERPS_RAPS_frame_tx_cancel(erps_state, ring_port, p);
        return VTSS_RC_OK;
    }

    // Try to generate a new R-APS PDU
    VTSS_RC(ERPS_RAPS_tx_frame_generate(erps_state, ring_port, p, &new_tx_props));

    // See packet_tx_props_t::operator==
    if (new_tx_props == ring_port_state->tx_props[p]) {
        // No change in R-APS PDU. Keep the old one going and delete the new.
        T_DG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s No change in R-APS PDU for sub-index %d", erps_state->inst, ring_port, p);

        packet_tx_free(new_tx_props.packet_info.frm);
        return VTSS_RC_OK;
    }

    // Cancel the old R-APS PDU (if any)
    ERPS_RAPS_frame_tx_cancel(erps_state, ring_port, p);

    T_IG(ERPS_TRACE_GRP_FRAME_TX, "%u:%s: New R-APS PDU of length %u w/o FCS", erps_state->inst, ring_port, new_tx_props.packet_info.len);

    // Print the IFH.
    packet_debug_tx_props_print(VTSS_MODULE_ID_ERPS, ERPS_TRACE_GRP_FRAME_TX, VTSS_TRACE_LVL_DEBUG, &new_tx_props);

    // And the frame.
    T_DG_HEX(ERPS_TRACE_GRP_FRAME_TX, new_tx_props.packet_info.frm, new_tx_props.packet_info.len);

    tx_props = &ring_port_state->tx_props[p];
    *tx_props = new_tx_props;

    return VTSS_RC_OK;
}

/******************************************************************************/
// erps_raps_tx_frame_update()
// This will re-generate the frame, because one or more of the fields of the PDU
// (not Tx Info) has changed.
/******************************************************************************/
mesa_rc erps_raps_tx_frame_update(erps_state_t *erps_state, bool do_tx)
{
    vtss_appl_erps_ring_port_t ring_port;
    bool                       pdu_transmitted;
    int                        p;

    T_IG(ERPS_TRACE_GRP_FRAME_TX, "%u: Enter", erps_state->inst);

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        pdu_transmitted = false;

        for (p = 0; p < ARRSZ(erps_state->ring_port_state[ring_port].tx_props); p++) {
            VTSS_RC(ERPS_RAPS_tx_frame_update(erps_state, ring_port, p));

            if (do_tx) {
                if (ERPS_RAPS_frame_tx_three_times(erps_state, ring_port, p, false /* this is not an event, so only send R-APS PDUs if tx_raps_active is true */)) {
                    pdu_transmitted = true;
                }
            }
        }

        if (pdu_transmitted) {
            ERPS_RAPS_tx_statistics_update(erps_state, ring_port, erps_state->status.tx_raps_info, 3 /* sent three times */);
        }
    }

    if (do_tx) {
        // (Re-)start the timer (fires every 5 seconds)-
        erps_timer_start(erps_state->tx_timer, ERPS_RAPS_TX_TIMEOUT_MS, true);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// erps_raps_statistics_clear()
/******************************************************************************/
void erps_raps_statistics_clear(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        memset(&erps_state->status.ring_port_status[ring_port].statistics, 0, sizeof(erps_state->status.ring_port_status[ring_port].statistics));
    }
}

/******************************************************************************/
// erps_raps_tx_info_update()
// This will just replace the Tx info in the frame if it has changed.
/******************************************************************************/
void erps_raps_tx_info_update(erps_state_t *erps_state, vtss_appl_erps_raps_info_t &new_tx_raps_info)
{
    vtss_appl_erps_ring_port_t ring_port;
    uint8_t                    new_tx_info[ERPS_RAPS_TX_DATA_LENGTH];
    erps_ring_port_state_t     *ring_port_state;
    uint8_t                    *frm;
    bool                       is_event = new_tx_raps_info.request == VTSS_APPL_ERPS_REQUEST_EVENT, restart_timer = false;
    bool                       pdu_transmitted, raps_pdu_transmitted;
    int                        p;

    memset(new_tx_info, 0, sizeof(new_tx_info));

    // Care is taken to enumerate the request according to the standard, so no
    // need for conversion.
    new_tx_info[0] = new_tx_raps_info.request << 4;

    if (new_tx_raps_info.rb) {
        // RPL Blocked
        new_tx_info[1] |= (1 << 7);
    }

    if (new_tx_raps_info.dnf) {
        // Do Not Flush
        new_tx_info[1] |= (1 << 6);
    }

    if (new_tx_raps_info.bpr == VTSS_APPL_ERPS_RING_PORT1) {
        // BPR - blocked port reference
        new_tx_info[1] |= (1 << 5);
    }

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        ring_port_state = &erps_state->ring_port_state[ring_port];

        // Gotta take a snapshot of this one, because it's not replicated twice
        // and will be set in the very first iteration below by
        // ERPS_RAPS_frame_tx_three_times().
        raps_pdu_transmitted = ring_port_state->raps_pdu_transmitted;

        // Make sure to only count it once per ring port.
        pdu_transmitted = false;

        for (p = 0; p < ARRSZ(erps_state->ring_port_state[ring_port].tx_props); p++) {
            frm = ring_port_state->tx_props[p].packet_info.frm;

            if (!frm) {
                continue;
            }

            // The frame is active. If the Tx info has changed or we haven't yet
            // sent the first R-APS PDU or it's an event, we must transmit the
            // frame three times in a row. We also start a 5 sec. timer if this
            // is not an event for further transmission.
            if (!raps_pdu_transmitted ||
                is_event              ||
                memcmp(new_tx_info, &frm[ring_port_state->tx_raps_info_offset[p]], ERPS_RAPS_TX_DATA_LENGTH)) {

                memcpy(&frm[ring_port_state->tx_raps_info_offset[p]], new_tx_info, ERPS_RAPS_TX_DATA_LENGTH);

                // Transmit it three times. Enforce transmission even if
                // tx_raps_active is false if this is an event.
                if (ERPS_RAPS_frame_tx_three_times(erps_state, ring_port, p, is_event)) {
                    pdu_transmitted = true;
                }

                if (is_event) {
                    // Restore whatever Tx info we used to send.
                    memcpy(&frm[ring_port_state->tx_raps_info_offset[p]], erps_state->tx_info, sizeof(erps_state->tx_info));
                } else {
                    restart_timer = true;
                }
            }
        }

        if (pdu_transmitted) {
            ERPS_RAPS_tx_statistics_update(erps_state, ring_port, new_tx_raps_info, 3);
        }
    }

    if (restart_timer) {
        // (Re-)start the Tx timer (fires every 5 seocnds).
        erps_timer_start(erps_state->tx_timer, ERPS_RAPS_TX_TIMEOUT_MS, true);
    }

    if (!is_event) {
        // Save a copy of the new R-APS Tx info for possible later use by
        // ERPS_RAPS_tx_frame_generate() in case other frame data changes.
        memcpy(erps_state->tx_info, new_tx_info, sizeof(erps_state->tx_info));
    }
}

/******************************************************************************/
// ERPS_RAPS_rx_validation_pass()
/******************************************************************************/
static bool ERPS_RAPS_rx_validation_pass(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, const uint8_t *const frm, uint32_t rx_length, vtss_appl_erps_raps_info_t *rx_raps_info)
{
    uint8_t  level;
    uint32_t offset = ERPS_RAPS_PDU_START_OFFSET, tlv_len, first_tlv_offset;
    uint8_t  tlv_type;
    uint8_t  request;

    if (rx_length < ERPS_RAPS_PDU_START_OFFSET + 37) {
        // Version 0 and 1 of R-APS PDU is exactly 37 bytes long (after DMAC,
        // SMAC, and ethertype), but we accept unknown TLVs.
        return false;
    }

    // Ring ID
    if (frm[5] != erps_state->conf.ring_id) {
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Rx frame with ring ID = %u. Expected %u", erps_state->inst, ring_port, frm[5], erps_state->conf.ring_id);
        return false;
    }

    // SMAC
    memcpy(rx_raps_info->smac.addr, &frm[6], sizeof(rx_raps_info->smac.addr));

    //
    // CFM header
    //

    offset = ERPS_RAPS_PDU_START_OFFSET;

    // MD level + Version. We must not check the version field.
    level = frm[offset] >> 5;
    rx_raps_info->version = frm[offset++] & 0x1F;
    if (level != erps_state->conf.level) {
        // Not for us.
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Rx frame at MEL %u. Expected %u", erps_state->inst, ring_port, level, erps_state->conf.level);
        return false;
    }

    // OpCode. Already checked to be ERPS_OPCODE_RAPS
    offset++;

    // Be forgiving about non-zero flags.
    offset++;

    // First TLV offset. Must be 32.
    if ((first_tlv_offset = frm[offset]) != 32) {
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Invalid First TLV Offset (%u)", erps_state->inst, ring_port, frm[offset]);
        return false;
    }

    // R-APS-specific information comes right after First TLV offset

    // We have to ignore R-APS PDUs where the request/state is invalid, that is,
    // not save it in our local state. If we at one point in time had received
    // a valid R-APS PDU, the rx_timer will be started. If we suddenly start
    // receiving invalid R-APS PDUs, that rx_timer will timeout and a dFOP_TO
    // event will be set in the base, then.
    request = (frm[offset + 1] & 0xF0) >> 4;
    if (request != VTSS_APPL_ERPS_REQUEST_NR &&
        request != VTSS_APPL_ERPS_REQUEST_MS &&
        request != VTSS_APPL_ERPS_REQUEST_SF &&
        request != VTSS_APPL_ERPS_REQUEST_FS &&
        request != VTSS_APPL_ERPS_REQUEST_EVENT) {
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Invalid R-APS request/state (0x%02x) received. Discarding", erps_state->inst, ring_port, request);
        return false;
    }

    rx_raps_info->request = (vtss_appl_erps_request_t)request;
    rx_raps_info->rb      = ((frm[offset + 2] >> 7) & 0x1) != 0;
    rx_raps_info->dnf     = ((frm[offset + 2] >> 6) & 0x1) != 0;
    rx_raps_info->bpr     = ((frm[offset + 2] >> 5) & 0x1) != 0 ? VTSS_APPL_ERPS_RING_PORT1 : VTSS_APPL_ERPS_RING_PORT0;
    memcpy(rx_raps_info->node_id.addr, &frm[offset + 3], sizeof(rx_raps_info->node_id.addr));

    // If the request is a flush event, G.8032, clause 10.1.6 specifies that
    // sub-code must be "0000" and status field must be "00000000" for a real
    // flush indication to be signalled.
    // Sub-code and Reserved Status shall be ignored upon reception for other
    // request types according to G.8032, clause 10.3.
    if (rx_raps_info->request == VTSS_APPL_ERPS_REQUEST_EVENT) {
        if ((frm[offset + 1] & 0x0F) != 0) {
            T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Got Event frame, but sub-code is not all-zeros (%hu). Discarding", erps_state->inst, ring_port, frm[offset + 1]);
            return false;
        }

        if (frm[offset + 2] != 0) {
            T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Got Event frame, but status is not all-zeros (%hu). Discarding", erps_state->inst, ring_port, frm[offset + 2]);
            return false;
        }
    }

    // offset is now pointing to the first byte after First TLV Offset, which is
    // where first_tlv_offset_is counting from. Skip past everything until the
    // first TLV.
    offset += first_tlv_offset;

    // The fixed length header does not overrun the length of the PDU.
    if (rx_length < offset) {
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Length (%u) < offset (%u)", erps_state->inst, ring_port, rx_length, offset);
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
            T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: TLV of type %u with length = %u caused frame to go beyond the frame length (%u)", erps_state->inst, ring_port, tlv_type, tlv_len, rx_length);
            return false;
        }
    }

    // If the End TLV is NOT mandatory, change this from false to true.
    T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: No End TLV found", erps_state->inst, ring_port);
    return false;
}

/******************************************************************************/
// erps_raps_rx_frame()
// Only invoked if either port0 or port1 matches for major rings and
// non-interconnected sub-rings.
// For interconnected sub-rings with virtual channel, it may have been received
// on one of the connected ring's ring ports, in which case ring_port is port1.
/******************************************************************************/
bool erps_raps_rx_frame(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, vtss_appl_erps_raps_info_t *rx_raps_info)
{
    T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u: Frame Rx on %s", erps_state->inst, ring_port);

    if (!ERPS_RAPS_rx_validation_pass(erps_state, ring_port, frm, rx_info->length, rx_raps_info)) {
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u: Frame failed validation", erps_state->inst);
        return false;
    }

    rx_raps_info->update_time_secs = vtss::uptime_seconds();
    return true;
}

/******************************************************************************/
// erps_raps_state_init()
/******************************************************************************/
void erps_raps_state_init(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port;
    erps_ring_port_state_t     *ring_port_state;

    memset(erps_state->tx_info, 0, sizeof(erps_state->tx_info));
    erps_timer_init(erps_state->tx_timer, "Tx timer", erps_state->inst, ERPS_RAPS_tx_timeout, erps_state);

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        ring_port_state  = &erps_state->ring_port_state[ring_port];
        ring_port_state->raps_pdu_transmitted = false;
    }

    erps_raps_statistics_clear(erps_state);
}

/******************************************************************************/
// erps_raps_deactivate()
/******************************************************************************/
void erps_raps_deactivate(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port;
    int                        p;

    T_IG(ERPS_TRACE_GRP_FRAME_TX, "%u: Freeing frames", erps_state->inst);

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        for (p = 0; p < ARRSZ(erps_state->ring_port_state[ring_port].tx_props); p++) {
            // Free any frame we might have allocated previously
            ERPS_RAPS_frame_tx_cancel(erps_state, ring_port, p);
        }
    }
}

