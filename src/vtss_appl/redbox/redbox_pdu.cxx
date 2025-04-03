/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "redbox_base.hxx"
#include "redbox_pdu.hxx"
#include "redbox_trace.h"

// The DMAC used in PRP/HSR PDUs. The last byte is modified by the code with the
// user-specified sv_dmac_lsb
const mesa_mac_t redbox_multicast_dmac = {{0x01, 0x15, 0x4e, 0x00, 0x01, 0x00}};

/******************************************************************************/
// REDBOX_PDU_tlv_type_to_str()
/******************************************************************************/
static const char *REDBOX_PDU_tlv_type_to_str(redbox_tlv_type_t type)
{
    switch (type) {
    case REDBOX_TLV_TYPE_NONE:
        return "None";

    case REDBOX_TLV_TYPE_PRP_DD:
        return "PRP-DD";

    case REDBOX_TLV_TYPE_PRP_DA:
        return "PRP-DA";

    case REDBOX_TLV_TYPE_HSR:
        return "HSR";

    case REDBOX_TLV_TYPE_REDBOX:
        return "RedBox";

    default:
        T_EG(REDBOX_TRACE_GRP_FRAME_RX, "Unknown TLV type (%d)", type);
        return "Unknown";
    }
}

/******************************************************************************/
// redbox_pdu_info_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const redbox_pdu_info_t &info)
{
    o << "{port_type = "            << redbox_util_port_type_to_str(info.port_type)
      << ", dmac_lsb = "            << info.dmac_lsb
      << ", smac = "                << info.smac
      << ", sup_sequence_number = " << info.sup_sequence_number
      << ", tlv1_type = "           << REDBOX_PDU_tlv_type_to_str(info.tlv1_type)
      << ", tlv1_mac = "            << info.tlv1_mac
      << ", tlv2_type = "           << REDBOX_PDU_tlv_type_to_str(info.tlv2_type)
      << ", tlv2_mac = "            << info.tlv2_mac
      << ", rct_present = "         << info.rct_present
      << ", rct_lan_id = "          << redbox_util_lan_id_to_str(info.rct_lan_id)
      << "}";

    return o;
}

/******************************************************************************/
// redbox_rx_pdu_info_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const redbox_pdu_info_t *info)
{
    o << *info;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// REDBOX_PDU_16bit_read()
/******************************************************************************/
static uint16_t REDBOX_PDU_16bit_read(const uint8_t *&p)
{
    uint16_t val;

    val  = *(p++) << 8;
    val |= *(p++);

    return val;
}

/******************************************************************************/
// REDBOX_PDU_16bit_write()
/******************************************************************************/
static void REDBOX_PDU_16bit_write(uint8_t *&p, uint16_t val, bool advance = false)
{
    uint8_t *p1 = p;

    *(p1++) = (val >> 8) & 0xFF;
    *(p1++) = (val >> 0) & 0xFF;

    if (advance) {
        p = p1;
    }
}

/******************************************************************************/
// REDBOX_PDU_vlan_tag_write()
/******************************************************************************/
static void REDBOX_PDU_vlan_tag_write(uint8_t *&p, const mesa_vlan_tag_t &vlan_tag, bool advance = false)
{
    uint8_t *p1 = p;

    REDBOX_PDU_16bit_write(p1, vlan_tag.tpid, true);

    // Compute PCP, DEI (= 0), and VID
    *(p1++) = (vlan_tag.pcp << 5) | ((vlan_tag.vid >> 8) & 0x0F);
    *(p1++) = vlan_tag.vid & 0xFF;

    if (advance) {
        p = p1;
    }
}

/******************************************************************************/
// REDBOX_PDU_rx_tlv_len_mac_valid()
/******************************************************************************/
static bool REDBOX_PDU_rx_tlv_len_mac_valid(const uint8_t *&p, mesa_mac_t &mac, bool is_tlv1)
{
    uint8_t val8;

    // TLV[1|2].Length
    val8 = *(p++);

    if (val8 != sizeof(mac.addr)) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Invalid TLV%d.Length. Expected %zu, got %u", is_tlv1 ? 1 : 2, sizeof(mac.addr), val8);
        return false;
    }

    // TLV[1|2].MacAddress
    memcpy(mac.addr, p, sizeof(mac.addr));
    p += sizeof(mac.addr);

    if (!mac_is_unicast(mac)) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "TLV%d.Data is not a unicast MAC address (%s)", is_tlv1 ? 1 : 2, mac.addr);
        return false;
    }

    return true;
}

/******************************************************************************/
// REDBOX_PDU_rx_tlv0_parse()
/******************************************************************************/
static bool REDBOX_PDU_rx_tlv0_parse(const uint8_t *&p)
{
    uint8_t val8;

    // TLV0.Type
    val8 = *(p++);
    if (val8 != REDBOX_TLV_TYPE_NONE) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Expected TLV0.Type, got %u", val8);
        return false;
    }

    // TLV0.Length
    val8  = *(p++);
    if (val8 != 0) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Expected TLV0.Length == 0, got %u", val8);
        return false;
    }

    return true;
}

/******************************************************************************/
// REDBOX_PDU_rx_tlv1_parse()
/******************************************************************************/
static bool REDBOX_PDU_rx_tlv1_parse(const uint8_t *&p, const mesa_packet_rx_info_t *const rx_info, redbox_pdu_info_t &rx_pdu_info)
{
    uint8_t val8;

    // TLV1.Type
    val8 = *(p++);

    rx_pdu_info.tlv1_type = (redbox_tlv_type_t)val8;

    if (rx_pdu_info.tlv1_type != REDBOX_TLV_TYPE_PRP_DD && rx_pdu_info.tlv1_type != REDBOX_TLV_TYPE_PRP_DA && rx_pdu_info.tlv1_type != REDBOX_TLV_TYPE_HSR) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Invalid TLV1.Type (%d)", val8);
        return false;
    }

    // TLV1.Length and TLV1.MacAddress
    if (!REDBOX_PDU_rx_tlv_len_mac_valid(p, rx_pdu_info.tlv1_mac, true /* TLV1 */)) {
        return false;
    }

    return true;
}

/******************************************************************************/
// REDBOX_PDU_rx_tlv2_parse()
/******************************************************************************/
static bool REDBOX_PDU_rx_tlv2_parse(const uint8_t *&p, redbox_pdu_info_t &rx_pdu_info)
{
    uint8_t val8;

    // TLV2.Type
    val8 = *(p++);

    if (val8 != REDBOX_TLV_TYPE_REDBOX) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Invalid TLV2.Type (%d)", val8);
        return false;
    }

    rx_pdu_info.tlv2_type = (redbox_tlv_type_t)val8;

    // TLV2.Length and TLV2.RedBoxMacAddress
    if (!REDBOX_PDU_rx_tlv_len_mac_valid(p, rx_pdu_info.tlv2_mac, false /* TLV2 */)) {
        return false;
    }

    return true;
}

/******************************************************************************/
// REDBOX_PDU_rx_validation_pass()
/******************************************************************************/
static bool REDBOX_PDU_rx_validation_pass(const uint8_t *frm, const mesa_packet_rx_info_t *const rx_info, redbox_pdu_info_t &rx_pdu_info, bool hsr_tagged)
{
    uint16_t      val16;
    uint8_t       val8;
    const uint8_t *p;

    vtss_clear(rx_pdu_info);

    // Check first five bytes of DMAC
    if (memcmp(frm, redbox_multicast_dmac.addr, sizeof(redbox_multicast_dmac.addr) - 1) != 0) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Invalid first five bytes of DMAC");
        return false;
    }

    // DMAC.LSByte
    rx_pdu_info.dmac_lsb = frm[5];

    // SMAC
    memcpy(rx_pdu_info.smac.addr, &frm[6], sizeof(rx_pdu_info.smac.addr));

    // As long as we can't receive a frame behind two tags, the HSR tag or SV
    // PDU contents always starts at offset 12 (two MACs), because the packet
    // module strips a possible outer VLAN tag.
    if (hsr_tagged) {
        // SV PDU starts at offset 12 (MACs) + 6 (HSR-tag) + 2 (SV EtherType)
        p = &frm[20];
    } else {
        // SV PDU starts at offset 12 (MACs) + 2 (SV EtherType)
        p = &frm[14];
    }

    // SupPath and SupVersion: Ignore
    p += 2;

    // SupSequenceNumber
    rx_pdu_info.sup_sequence_number = REDBOX_PDU_16bit_read(p);

    // TLV1
    if (!REDBOX_PDU_rx_tlv1_parse(p, rx_info, rx_pdu_info)) {
        return false;
    }

    // TLV0 or TLV2
    val8 = *p; // Don't advance

    if (val8 == REDBOX_TLV_TYPE_NONE) {
        if (!REDBOX_PDU_rx_tlv0_parse(p)) {
            return false;
        }

        // I am not sure if the L2.SMAC must be identical to TLV1.MAC, but for
        // safety, we skip this check.
    } else {
        if (!REDBOX_PDU_rx_tlv2_parse(p, rx_pdu_info)) {
            return false;
        }

        if (!REDBOX_PDU_rx_tlv0_parse(p)) {
            return false;
        }

        // There is no guarantee that TLV2.RedBoxMacAddress and L2.SMAC are
        // identical, because the SV frame may be proxied by another RedBox, so
        // that the proxying RedBox's MAC adddress is the L2.SMAC and the TLV2
        // SMAC is the SMAC of the original RedBox.
    }

    if (p - frm > rx_info->length) {
        T_IG(REDBOX_TRACE_GRP_FRAME_RX, "Frame not long enough. Expected at least %u bytes, got %u", p - frm, rx_info->length);
        return false;
    }

    // Parse a possible RCT. The RCT is located at the end of the frame.
    p = &frm[rx_info->length - 2];
    rx_pdu_info.rct_present = REDBOX_PDU_16bit_read(p) == REDBOX_ETYPE_SUPERVISION;

    if (rx_pdu_info.rct_present) {
        // We accept RCT-tagged frames with any LSDUsize, because in HSR-HSR
        // mode, we need to send with LSDUsize == 0 ourselves (due to a
        // cut-through chip-deficiency), and it might happen that this frame
        // ends up on the PRP network, so just check the LanId.
        p = &frm[rx_info->length - 4];
        val16 = REDBOX_PDU_16bit_read(p);

        // Check LanId (which is actually a PathId).
        val8 = val16 >> 12;
        if (val8 != 0xa && val8 != 0xb) {
            // Only LanId 1010 and 1011 are valid.
            rx_pdu_info.rct_present = false;
        } else {
            rx_pdu_info.rct_lan_id = val8 == 0xa ? VTSS_APPL_REDBOX_LAN_ID_A : VTSS_APPL_REDBOX_LAN_ID_B;
        }
    }

    return true;
}

/******************************************************************************/
// REDBOX_PDU_packet_tx()
/******************************************************************************/
static bool REDBOX_PDU_packet_tx(redbox_state_t &redbox_state, redbox_sv_frame_t &sv_frame, mesa_port_no_t port_no, const char *called_from)
{
    mesa_rc rc;

    T_IG_PORT(REDBOX_TRACE_GRP_FRAME_TX, port_no, "%u: %s: packet_tx(%s)", redbox_state.inst, called_from, sv_frame.tx_props);
    T_IG_HEX(REDBOX_TRACE_GRP_FRAME_TX, sv_frame.tx_props.packet_info.frm, sv_frame.tx_props.packet_info.len);
    if ((rc = packet_tx(&sv_frame.tx_props)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: %s: packet_tx(%s) failed: %s", redbox_state.inst, called_from, sv_frame.tx_props, error_txt(rc));
        T_EG_HEX(REDBOX_TRACE_GRP_FRAME_TX, sv_frame.tx_props.packet_info.frm, sv_frame.tx_props.packet_info.len);
        redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR;
        return false;
    }

    return true;
}

/******************************************************************************/
// REDBOX_PDU_tx_in_vlan()
//
// Transmit a SV frame to all ports that are members of this VLAN.
//
// If a port is the I/L port of another RB, that RB will receive the frame
// directly.
//
// The frame is already composed and ready in
// redbox_state.sv_frames[sv_frame_type][]
/******************************************************************************/
static uint32_t REDBOX_PDU_tx_in_vlan(redbox_state_t &redbox_state, redbox_sv_frame_type_t sv_frame_type, const redbox_pdu_info_t &pdu_info, mesa_vlan_tag_t &class_tag)
{
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
    mesa_packet_port_info_t filter_info;
    mesa_port_no_t          port_no, interlink_port_no = redbox_state.interlink_port_no_get();
    uint32_t                tx_cnt = 0;
    bool                    tx_vlan_tagged;
    int                     idx;
    mesa_rc                 rc;

    // We need to send to ports one by one, because transmitting in the VLAN in
    // one go would also send the frame to our own RB, which (if PRP-to-HSR
    // translation is disabled) would also send the frame to our own RB's I/L.
    // In addition, it might be sent to another RB, which has PRP-to-HSR
    // translation enabled, which in turn should have caused it to go to the CPU
    // (which it doesn't because the frame already comes from the CPU).

    // RBNTBD: We may happen to send to more than just one port in an
    // aggregation.

    // Use MESA to figure out which ports belong to this VLAN, and how frames
    // are supposed to be transmitted (tagged/untagged, and if tagged, which
    // tag-type).
    (void)mesa_packet_port_info_init(&filter_info);

    // Pretend the frame is coming from the I/L port. In this way, we can make
    // sure not to transmit anything if the I/L port is not in forwarding state
    // (which could be the case if both Port A and Port B are down), or if the
    // port is not a member of filter_info.vid.

    // We could here pretend that the frame comes from the I/L port in order to
    // make sure not to transmit anything if the I/L port is not in forwarding
    // state (which could be the case if both Port A and Port B are down), or if
    // the port is not a member of filter_info.vid.
    // This would work fine if this function is called from the function that
    // forwards SV frames from HSR ring to PRP network.
    // But if this function is called from the function that sends our own SV
    // frames to the PRP network, it doesn't matter what link state Port A and
    // Port B are in; our own SV frame must go to the PRP N/W.
    // By not pretending that the frame comes from the I/L port in the following
    // call, it still works fine, because the function that calls us due to
    // HSR-to-PRP forwarding has made an ingress VLAN check and since a real
    // frame is received for this to happen, we also know that at least one LRE
    // port is up.
    // Summasummarum: Do not make ingress filtering in any case, but make sure
    // not to forward back to the I/L port.
    filter_info.port_no = MESA_PORT_NO_NONE; // See long description above.
    filter_info.vid     = class_tag.vid;

    T_DG_PORT(REDBOX_TRACE_GRP_FRAME_TX, filter_info.port_no, "%u: mesa_packet_port_filter_get(VID = %u)", redbox_state.inst, filter_info.port_no, filter_info.vid);
    if ((rc = mesa_packet_port_filter_get(NULL, &filter_info, filter.size(), filter.data())) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: mesa_packet_port_filter_get() failed: %s", redbox_state.inst, error_txt(rc));
        return 0;
    }

    // Make sure not to forward back to the I/L port.
    filter[interlink_port_no].filter = MESA_PACKET_FILTER_DISCARD;

    for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
        redbox_port_role_t &redbox_port_role = redbox_port_roles[port_no];

        if (filter[port_no].filter == MESA_PACKET_FILTER_DISCARD || port_no == interlink_port_no || redbox_port_role.type == REDBOX_PORT_ROLE_TYPE_UNCONNECTED) {
            // Don't send the frame back to the port it comes/came from or out
            // on a port that it shouldn't (hereunder an unconnected RB port).
            continue;
        }

        tx_vlan_tagged = filter[port_no].filter == MESA_PACKET_FILTER_TAGGED;

        // Pick the right SV frame to send.
        redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[sv_frame_type][tx_vlan_tagged ? 1 : 0];

        // If sending to another RB's LRE port, it must be dropped UNLESS that
        // other RB is an HSR-PRP RB, in which case we must send the frame down
        // the throat of that RB.
        if (redbox_port_role.type == REDBOX_PORT_ROLE_TYPE_INTERLINK) {
            // This is another RB's I/L. Only active RBs can be in this state,
            // so no need to check oper_state.
            // Only RBs in HSR-PRP mode should receive this frame. RBs in other
            // modes don't care about SV frames received on their I/L and will
            // discard them.
            if (redbox_port_role.redbox_state->conf.mode != VTSS_APPL_REDBOX_MODE_HSR_PRP) {
                continue;
            }

            redbox_base_rx_sv_on_interlink(*redbox_port_role.redbox_state, port_no, pdu_info, class_tag, filter[port_no].filter == MESA_PACKET_FILTER_TAGGED);
        } else {
            if (tx_vlan_tagged) {
                // We need to change the TPID of a possible VLAN tag to that
                // configured on the port.
                REDBOX_PDU_16bit_write(sv_frame.vlan_tag_ptr, filter[port_no].tpid);
            }

            // Set port to which we Tx this frame
            sv_frame.tx_props.tx_info.dst_port_mask = VTSS_BIT64(port_no);

            if (!REDBOX_PDU_packet_tx(redbox_state, sv_frame, port_no, __FUNCTION__)) {
                return tx_cnt;
            }
        }

        tx_cnt++;
    }

    // Move the port number back to what it was (I/L port). This is needed if
    // sv_frame_type is REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2, because it's
    // the same frame pointer also used to send to LRE ports.
    for (idx = 0; idx <= 1; idx++) {
        redbox_state.sv_frames[sv_frame_type][idx].tx_props.tx_info.dst_port_mask = VTSS_BIT64(interlink_port_no);
    }

    return tx_cnt;
}

/******************************************************************************/
// REDBOX_PDU_link_to_fwd()
/******************************************************************************/
static mesa_packet_rb_fwd_t REDBOX_PDU_link_to_fwd(bool port_a_link, bool port_b_link)
{
    if (port_a_link && port_b_link) {
        return MESA_PACKET_RB_FWD_BOTH;
    }

    if (port_a_link) {
        return MESA_PACKET_RB_FWD_A;
    }

    if (port_b_link) {
        return MESA_PACKET_RB_FWD_B;
    }

    return MESA_PACKET_RB_FWD_DEFAULT;
}

/******************************************************************************/
// REDBOX_PDU_tx_pnt_entry()
// Below is a table of how we send frames in different modes.
// "Tx to SwC" indicates whether to also send SV frames to non-LRE ports.
// This is only applicable in HSR-PRP and HSR-HSR modes.
//
// =--------------------------------------------------------------------------=
// | Mode    | Tx to SwC? || Tx w. HSR | Tx w. RCT | Note                     |
// |---------|------------||-----------|-----------|--------------------------|
// | PRP-SAN | 0          || 0         | 0         | H/W adds RCT             |
// | HSR-SAN | 0          || 0         | 0         | H/W adds HSR-tag         |
// | HSR-PRP | 0          || 0         | 1         | H/W moves RCT to HSR-tag |
// | HSR-PRP | 1          || 0         | 1         | RCT is kept              |
// | HSR-HSR | 0          || ?         | ?         | ?!?                      |
// | HSR-HSR | 1          || ?         | ?         | ?!?                      |
// =--------------------------------------------------------------------------=
/******************************************************************************/
static void REDBOX_PDU_tx_pnt_entry(redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr)
{
    vtss_appl_redbox_sv_type_t sv_type;
    redbox_tlv_type_t          tlv_type;
    redbox_pdu_info_t          pdu_info;
    mesa_vlan_tag_t            vlan_tag;
    mesa_vlan_tag_t            class_tag;
    bool                       port_a_link, port_b_link, own, hsr_prp_mode, tx_to_prp_nw, tx_to_lre_ports, tx_vlan_tagged;
    uint8_t                    *p;
    uint32_t                   tx_cnt;
    int                        idx_min, idx_max, idx;

    if (!mac_itr->second.is_pnt_entry) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: %s is not a PNT entry", redbox_state.inst, mac_itr->first);
        return;
    }

    // We generate SV frames without TLV2 only for RB's own MAC address, which
    // is locked and either of type DANH_RB (HSR-xxx modes) or DANP-RB (PRP-SAN
    // mode).
    own = mac_itr->second.pnt.status.locked && (mac_itr->second.pnt.status.node_type == VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB || mac_itr->second.pnt.status.node_type == VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB);

    if (own && redbox_state.tx_spv_suspend_own) {
        // Tx of own SV frames is currently suspended.
        return;
    }

    if (!own && redbox_state.tx_spv_suspend_proxied) {
        // Tx of proxied SV frames is currently suspended.
        return;
    }

    port_a_link = redbox_state.port_link_get(VTSS_APPL_REDBOX_PORT_TYPE_A);
    port_b_link = redbox_state.port_link_get(VTSS_APPL_REDBOX_PORT_TYPE_B);

    T_IG(REDBOX_TRACE_GRP_FRAME_TX, "%u: Tx on behalf of %s. port-a link = %d, port-b link = %d", redbox_state.inst, mac_itr->first, port_a_link, port_b_link);

    // Are we in the special HSR-PRP mode?
    hsr_prp_mode = redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP;

    // We also need to send to PRP network if it's our own SV frame and we are
    // in HSR-PRP mode.
    tx_to_prp_nw = own && hsr_prp_mode;

    if (tx_to_prp_nw) {
        // Create an artificial pdu_info for use by the following function, in
        // case it needs to send the frame to another HSR-PRP RB, so that this
        // other RB doesn't need to parse the frame.
        // We need to do fill in this structure before we increment
        // redbox_state.sup_sequence_number.
        vtss_clear(pdu_info);

        // When this is going to be used, it is seen as a SV frame received on
        // the I/L port.
        pdu_info.port_type           = VTSS_APPL_REDBOX_PORT_TYPE_C;
        pdu_info.dmac_lsb            = redbox_state.conf.sv_dmac_lsb;
        pdu_info.sup_sequence_number = redbox_state.sup_sequence_number;
        pdu_info.smac                = redbox_state.interlink_port_state_get()->redbox_mac;
        pdu_info.tlv1_type           = REDBOX_TLV_TYPE_HSR;
        pdu_info.tlv1_mac            = pdu_info.smac;
        pdu_info.tlv2_type           = REDBOX_TLV_TYPE_NONE;
        pdu_info.rct_present         = true;
        pdu_info.rct_lan_id          = redbox_state.conf.lan_id;
    }

    // We do transmit to LRE ports if there's link on either LRE port and the
    // interlink port is member of the VLAN ID we are asked to transmit with.
    tx_to_lre_ports = (port_a_link || port_b_link) && redbox_state.interlink_member_of_tx_vlan;

    if (tx_to_lre_ports || tx_to_prp_nw) {
        // If either we are transmitting to the LRE ports or we need to transmit
        // to the PRP network later, fill in the frames.

        // We transmit VLAN tagged if the configured VLAN is configured to be
        // tagged on egress.
        tx_vlan_tagged = redbox_state.tx_vlan_tagged();

        // First fill in VLAN-tagged and/or VLAN-untagged templates.
        // Only in HSR-PRP mode and only for the RB's own SV frame, may we use
        // both templates, so in other cases, we just fill in the right one.

        // If idx == 0, it's a VLAN untagged template, we use. Otherwise, it's a
        // VLAN tagged.
        // If we need to send to the PRP network, we fill in both the tagged and
        // the untagged template, because both might be required later.
        // Otherwise, we pick the one that corresponds to whether we need to
        // VLAN tag or not.
        idx_min = tx_to_prp_nw || !tx_vlan_tagged ? 0 : 1;
        idx_max = tx_to_prp_nw ||  tx_vlan_tagged ? 1 : 0;

        for (idx = idx_min; idx <= idx_max; idx++) {
            // Pick the right template. If it's the RB's own SV frame, we send
            // without a TLV2. Otherwise, we send with a TLV2. The second index
            // is VLAN untagged/VLAN tagged.
            redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[own ? REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2 : REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT][idx];

            // Update dmac_lsb (may be changed by configuration on the fly)
            *sv_frame.dmac_lsb_ptr = redbox_state.conf.sv_dmac_lsb;

            if (idx == 1) {
                // Update the VLAN tag.
                vlan_tag.tpid = redbox_state.interlink_port_state_get()->tpid;
                vlan_tag.vid  = redbox_state.tx_vlan_get();
                vlan_tag.pcp  = redbox_state.conf.sv_pcp;
                vlan_tag.dei  = 0;
                REDBOX_PDU_vlan_tag_write(sv_frame.vlan_tag_ptr, vlan_tag);
            }

            // Update SupSequenceNumber. Don't increment until we are done with
            // all templates, because we need to use the same sequence number
            // both when sending to HSR-ring and PRP N/W (in HSR-PRP mode).
            REDBOX_PDU_16bit_write(sv_frame.sup_sequence_number_ptr, redbox_state.sup_sequence_number);

            // Update TLV1.MAC
            memcpy(sv_frame.tlv1_mac_address_ptr, mac_itr->first.addr, sizeof(mac_itr->first.addr));

            // TLV2.MAC is already present if needed (based on chosen template).

            if (hsr_prp_mode) {
                // We transmit with an RCT if we are in HSR-PRP mode and let H/W
                // convert the RCT to an HSR-tag. In other modes, we let H/W
                // insert the HSR-tag using its own redundancy sequence number.
                //
                // Update RCT.SeqNr
                // In HSR-PRP mode, all frames that are sent with the RB's MAC
                // address must have a S/W-generated redundancy sequence number,
                // because we cannot use H/W to insert the HSR-tag, because we
                // need to send with two different NetIds: We use the configured
                // NetId for most proxied SV frames, and NetId 0 only for SV
                // frames generated by our own RB in order for these SV frames
                // to reach the accompanying HSR-PRP RB's CPU without getting
                // NetId filtered.
                //
                // If H/W used its own redundancy sequence number for some of
                // the frames, we would have both S/W-generated redundancy
                // sequence numbers and H/W- generated redundancy sequence
                // numbers, causing potential duplicate discards of either of
                // them.
                // Don't increment the redundancy sequence number until we are
                // done with all templates, because we need to use the same
                // sequence number both when sending to HSR-ring and PRP N/W.
                REDBOX_PDU_16bit_write(sv_frame.rct_ptr, redbox_state.redundancy_tag_sequence_number);

                // Update RCT.LanId (needs on the fly updates, because user may
                // change lan_id).
                p = sv_frame.rct_ptr + 2;

                // Notice that the PathId (wrongly called LanId in IEC62439-3,
                // page 39) is either A or B in the RCT tag.
                // This is converted by the RB to an HSR tag according to the
                // configured NetId. Even for the RB's own SV frames, we must
                // insert A or B to prevent the frame from being wrongly counted
                // by the H/W on entrance to the RB. See code just below on how
                // to get it out with NetId 0.
                *p = (redbox_state.conf.lan_id == VTSS_APPL_REDBOX_LAN_ID_A ? 0xa : 0xb) << 4;

                if (own) {
                    // We need to signal to the RB that when it transforms the
                    // RCT to an HSR-tag, it must use ring_netid (always 0)
                    // rather than the configured NetId in the HSR tag. This
                    // allows the accompanying RB in the other end of the HSR
                    // network to receive the SV frame rather than having it
                    // NetId filtered.
                    sv_frame.tx_props.tx_info.rb_ring_netid_enable = true;
                }
            }
        }
    }

    // Now it's finally time to transmit the frame(s).
    // First we transmit to the LRE ports if they have link and are members of
    // the Tx VLAN.
    if (tx_to_lre_ports) {
        redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[own ? REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2 : REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT][tx_vlan_tagged];

        // Change the IFH to only transmit to the LRE ports with link. This is
        // needed in order to prevent the H/W counters from counting on LRE
        // ports without link (sigh!).
        sv_frame.tx_props.tx_info.rb_fwd = REDBOX_PDU_link_to_fwd(port_a_link, port_b_link);
        REDBOX_PDU_packet_tx(redbox_state, sv_frame, redbox_state.interlink_port_no_get(), __FUNCTION__);
        sv_frame.tx_props.tx_info.rb_fwd = REDBOX_PDU_link_to_fwd(true, true);

        // Prepare for next SV frame
        redbox_state.sup_sequence_number++;

        if (hsr_prp_mode) {
            redbox_state.redundancy_tag_sequence_number++;
        }

        // Update statistics
        sv_type = redbox_pdu_tlv_type_to_sv_type((redbox_tlv_type_t)(*sv_frame.tlv1_type_ptr));
        if (port_a_link) {
            redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_A].sv_tx_cnt[sv_type]++;
        }

        if (port_b_link) {
            redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_B].sv_tx_cnt[sv_type]++;
        }

        mac_itr->second.pnt.status.sv_tx_cnt++;
    }

    if (!tx_to_prp_nw) {
        // Nothing else to do.
        return;
    }

    // In HSR-PRP mode, we also send the frame (for the RedBox' own MAC) to all
    // other ports in the VLAN, including other RBs.

    // Start by setting the bit that we have set to true back to false, because
    // when we send to other ports in the VLAN, some of these ports may be
    // interlink ports for other RBs, and we don't want the NetId to become 0
    // on those ports (it wouldn't matter if all other ports were normal switch
    // ports, because the SwC itself doesn't use this bit for anything - only
    // for signalling to RBs).
    // While we are at it, also change the SV type to the appropriate one for
    // this RB when transmitting to the PRP N/W (currently, it is set to HSR).
    tlv_type = redbox_state.conf.duplicate_discard ? REDBOX_TLV_TYPE_PRP_DD : REDBOX_TLV_TYPE_PRP_DA;
    sv_type = redbox_pdu_tlv_type_to_sv_type(tlv_type);
    for (idx = 0; idx <= 1; idx++) {
        redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2][idx];

        // IFH bit back to false.
        sv_frame.tx_props.tx_info.rb_ring_netid_enable = false;

        // SV type according to our duplicate discard setting.
        *sv_frame.tlv1_type_ptr = tlv_type;
    }

    // Time to transmit.
    vtss_clear(class_tag);
    class_tag.vid = redbox_state.tx_vlan_get();
    class_tag.pcp = redbox_state.conf.sv_pcp;

    tx_cnt = REDBOX_PDU_tx_in_vlan(redbox_state, REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2, pdu_info, class_tag);

    if (tx_cnt) {
        // At least one frame was sent. Update statistics for I/L port.
        redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_C].sv_tx_cnt[sv_type]++;
    }

    // Restore SV type
    for (idx = 0; idx <= 1; idx++) {
        redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2][idx];

        // SV type used to be HSR
        *sv_frame.tlv1_type_ptr = REDBOX_TLV_TYPE_HSR;
    }
}

/******************************************************************************/
// REDBOX_PDU_tx_timeout()
/******************************************************************************/
static void REDBOX_PDU_tx_timeout(redbox_timer_t &timer, const void *context)
{
    redbox_itr_t     redbox_itr;
    redbox_mac_itr_t mac_itr;
    const mesa_mac_t *mac = (const mesa_mac_t *)context;

    if ((redbox_itr = REDBOX_map.find(timer.instance)) == REDBOX_map.end()) {
        T_EG(REDBOX_TRACE_GRP_BASE, "Unable to find RedBox instance #%u in map", timer.instance);
        redbox_timer_stop(timer);
        return;
    }

    redbox_state_t &redbox_state = redbox_itr->second;

    if ((mac_itr = redbox_state.mac_map.find(*mac)) == redbox_state.mac_map.end()) {
        T_EG(REDBOX_TRACE_GRP_BASE, "%u: Unable to find %s in PNT map", redbox_state.inst, *mac);
        redbox_timer_stop(timer);
        return;
    }

    if (!mac_itr->second.is_pnt_entry) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: %s is not a PNT entry", redbox_state.inst, mac_itr->first);
    }

    if (&timer != &mac_itr->second.pnt.timer) {
        T_EG(REDBOX_TRACE_GRP_BASE, "%u: Timer mismatch for %s", redbox_state.inst, *mac);
        return;
    }

    // Tx Supervsion frame
    REDBOX_PDU_tx_pnt_entry(redbox_state, mac_itr);

    // Adjust timeout. It was set to a random value between 0 and
    // sv_frame_interval_secs when first added, in order not to burst
    // all supervision frames. Now, we can set it to the regular frame interval.
    if (timer.period_ms != redbox_state.conf.sv_frame_interval_secs * 1000) {
        // If we re-start the timer every time this function is invoked, it may
        // skew in time, so only do it after the first time.
        redbox_timer_start(timer, redbox_state.conf.sv_frame_interval_secs * 1000, true);
    }
}

/******************************************************************************/
// REDBOX_PDU_tx_props_dispose()
/******************************************************************************/
static void REDBOX_PDU_tx_props_dispose(packet_tx_props_t &tx_props)
{
    if (tx_props.packet_info.frm) {
        packet_tx_free(tx_props.packet_info.frm);
        tx_props.packet_info.frm = nullptr;
    }

    tx_props.packet_info.len = 0;
}

/******************************************************************************/
// REDBOX_PDU_tx_props_dispose_all()
/******************************************************************************/
static void REDBOX_PDU_tx_props_dispose_all(redbox_state_t &redbox_state)
{
    int i, j;

    for (i = 0; i < REDBOX_SV_FRAME_TYPE_CNT; i++) {
        for (j = 0; j < 2; j++) {
            REDBOX_PDU_tx_props_dispose(redbox_state.sv_frames[i][j].tx_props);
        }
    }
}

/******************************************************************************/
// REDBOX_PDU_tx_props_create()
/******************************************************************************/
static mesa_rc REDBOX_PDU_tx_props_create(redbox_state_t &redbox_state, packet_tx_props_t &tx_props, uint32_t frm_len, bool needs_vlan_tag)
{
    packet_tx_info_t      &packet_info = tx_props.packet_info;
    mesa_packet_tx_info_t &tx_info     = tx_props.tx_info;
    uint32_t              normalized_frm_len;

    // Clean slate
    packet_tx_props_init(&tx_props);

    normalized_frm_len = MAX(needs_vlan_tag ? 64 : 60, frm_len);

    if ((packet_info.frm = packet_tx_alloc(normalized_frm_len)) == NULL) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: Out of memory", redbox_state.inst);
        redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR;
        return VTSS_APPL_REDBOX_RC_OUT_OF_MEMORY;
    }

    packet_info.modid     = VTSS_MODULE_ID_REDBOX;
    packet_info.len       = normalized_frm_len;
    packet_info.no_free   = true;
    tx_info.dst_port_mask = VTSS_BIT64(redbox_state.interlink_port_no_get()); // Default to Tx from CPU towards I/L (may be overridden for HSR-to-PRP-forwarding).
    tx_info.tag.vid       = 0; // We insert a possible VLAN tag into the packet ourselves if needed.
    tx_info.tag.pcp       = 7;
    tx_info.tag.dei       = 0;
    tx_info.cos           = 7;
    tx_info.cosid         = 7;
    tx_info.dp            = 0;
    tx_info.rb_fwd        = REDBOX_PDU_link_to_fwd(true, true); // If not set, the RedBox will kill supervision frames. If we send to normal ports, this doesn't matter.

    memset(packet_info.frm, 0, normalized_frm_len);

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_PDU_len_get()
// Returns the size of the requested PRP/HSR PDU.
/******************************************************************************/
static uint32_t REDBOX_PDU_len_get(bool needs_vlan_tag, bool needs_tlv2, bool needs_rct)
{
    uint32_t l2_len, payload_len;

    // Figure out how much space we need
    l2_len = sizeof(mesa_mac_t)       /* DMAC              */ +
             sizeof(mesa_mac_t)       /* SMAC              */ +
             (needs_vlan_tag ? 4 : 0) /* Possible VLAN tag */ +
             2                        /* SV Ethertype      */;

    // SV PDU
    payload_len = 2                    /* SupPath + SupVersion */ +
                  2                    /* SupSequenceNumber    */ +
                  1                    /* TLV1.Type            */ +
                  1                    /* TLV1.Length          */ +
                  6                    /* DANP/DANH MAC        */ +
                  (needs_tlv2 ? 1 : 0) /* TLV2.Type            */ +
                  (needs_tlv2 ? 1 : 0) /* TLV2.Length          */ +
                  (needs_tlv2 ? 6 : 0) /* RedBox MAC           */ +
                  1                    /* TLV0.Type            */ +
                  1                    /* TLV0.Length          */;

    if (needs_rct) {
        // According to Table 5, we need to pad to 70 (untagged) or 74 (tagged)
        // bytes. The RCT starts at offset 60 or 64 then.
        payload_len += (needs_tlv2 ? 24 : 32) /* zero-padding */ +
                       2                      /* SeqNr        */ +
                       1                      /* LanId        */ +
                       3                      /* LSDUsize     */;
    }

    return l2_len + payload_len;
}

/******************************************************************************/
// REDBOX_PDU_tags_get()
/******************************************************************************/
static void REDBOX_PDU_tags_get(redbox_state_t &redbox_state, redbox_sv_frame_type_t sv_frame_type, bool &needs_tlv2, bool &needs_rct)
{
    switch (sv_frame_type) {
    case REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT:
        // Transmit proxied SV frames with TLV2 towards LRE ports. We need RCT
        // in HSR-PRP mode.
        needs_tlv2 = true;
        needs_rct  = redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP;
        break;

    case REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2:
        // This is only for the RB's own SV frames. We don't need TLV2, but we
        // need an RCT in HSR-PRP mode.
        needs_tlv2 = false;
        needs_rct  = redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP;
        break;

    case REDBOX_SV_FRAME_TYPE_FWD_PRP_TO_HSR:
        // This is for SV frames received from PRP network that need to be
        // converted to HSR-tagged frames and sent to LRE ports.
        // We need an RCT and let H/W convert it.
        needs_tlv2 = true;
        needs_rct  = true;
        break;

    case REDBOX_SV_FRAME_TYPE_FWD_HSR_TO_PRP:
        // This is for SV frames received from HSR-ring that need to be
        // converted to RCT-tagged frames and sent to PRP network.
        // We need of course an RCT, but we also always need a TLV2, because
        // either the original frame already contained one or we insert one,
        // according to the corrigendum.
        needs_tlv2 = true;
        needs_rct  = true;
        break;

    default:
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "Unknown SV frame type (%d)", sv_frame_type);
        needs_tlv2 = false;
        needs_rct  = false;
        break;
    }
}

/******************************************************************************/
// redbox_pdu_tx_template_create()
// Creates a supervision PDU template for the requested type.
/******************************************************************************/
static mesa_rc REDBOX_PDU_tx_template_create(redbox_state_t &redbox_state, redbox_sv_frame_type_t sv_frame_type, bool needs_vlan_tag)
{
    redbox_port_state_t *interlink_port_state;
    redbox_sv_frame_t   &sv_frame = redbox_state.sv_frames[sv_frame_type][needs_vlan_tag ? 1 : 0];
    uint8_t             *p;
    uint32_t            frm_len;
    bool                needs_tlv2, needs_rct;

    // Figure out which fields to create this frame with
    REDBOX_PDU_tags_get(redbox_state, sv_frame_type, needs_tlv2, needs_rct);

    // And get a frame length given that info.
    frm_len = REDBOX_PDU_len_get(needs_vlan_tag, needs_tlv2, needs_rct);

    VTSS_RC(REDBOX_PDU_tx_props_create(redbox_state, sv_frame.tx_props, frm_len, needs_vlan_tag));

    T_DG(REDBOX_TRACE_GRP_FRAME_TX, "Creating SV frame template for type = %d. Length = %u", sv_frame_type, frm_len);

    p = sv_frame.tx_props.packet_info.frm;

    // DMAC
    memcpy(p, redbox_multicast_dmac.addr, sizeof(redbox_multicast_dmac.addr));

    // The sv_dmac_lsb is written on the fly in order not to generate new templates
    // whenever configuration changes it. But keep a pointer to it.
    sv_frame.dmac_lsb_ptr = &p[5];
    p += sizeof(redbox_multicast_dmac.addr);

    // SourceAddress is that of the RedBox, except if the RB is configured to
    // forward SV frames as they were received.
    sv_frame.smac_ptr = p;
    interlink_port_state = redbox_state.interlink_port_state_get();
    memcpy(p, interlink_port_state->redbox_mac.addr, sizeof(interlink_port_state->redbox_mac.addr));
    p += sizeof(interlink_port_state->redbox_mac.addr);

    if (needs_vlan_tag) {
        // Make room for a VLAN tag while saving a pointer for it. Always
        // updated on the fly.
        sv_frame.vlan_tag_ptr = p;
        p += 4;
    }

    // SupEtherType
    REDBOX_PDU_16bit_write(p, REDBOX_ETYPE_SUPERVISION, true /* advance p */);

    // SupPath (0) + SupVersion (1)
    REDBOX_PDU_16bit_write(p, 1, true /* advance p */);

    // SupSequenceNumber.
    // Skip this field for now (leave it at 0), but save a pointer to it, so
    // that we can update it on the fly.
    sv_frame.sup_sequence_number_ptr = p;
    p += 2;

    // TLV1.Type (20 or 21 for PRP; 23 for HSR).
    sv_frame.tlv1_type_ptr = p;
    if (sv_frame_type == REDBOX_SV_FRAME_TYPE_FWD_HSR_TO_PRP) {
        // If we are in HSR-PRP mode, the corrigendum states that we must insert
        // 20 for PRP frames (5.2.2.3.2.1, 1b).
        *(p++) = REDBOX_TLV_TYPE_PRP_DD;
    } else if (redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_PRP_SAN) {
        // In PRP-SAN mode, we set it to the configured value.
        *(p++) = redbox_state.conf.duplicate_discard ? REDBOX_TLV_TYPE_PRP_DD : REDBOX_TLV_TYPE_PRP_DA;
    } else {
        // All other frame types and modes use HSR, which only has one value.
        *(p++) = REDBOX_TLV_TYPE_HSR;
    }

    // TLV1.Length = sizeof(MAC) = 6
    *(p++) = 6;

    // MacAddress.
    // Skip this field for now (leave it at 0), but save a pointer to it, so
    // that we can update it on the fly.
    sv_frame.tlv1_mac_address_ptr = p;
    p += 6;

    if (needs_tlv2) {
        // TLV2.Type = 30
        *(p++) = REDBOX_TLV_TYPE_REDBOX;

        // TLV2.Length = sizeof(MAC) = 6
        *(p++) = 6;

        // RedBoxMacAddress.
        memcpy(p, interlink_port_state->redbox_mac.addr, sizeof(interlink_port_state->redbox_mac.addr));

        // In some cases, we must use the original frame's RedBox MAC address,
        // so save a pointer to it.
        sv_frame.tlv2_mac_address_ptr = p;
        p += sizeof(interlink_port_state->redbox_mac.addr);
    }

    // TLV0.Type = 0
    p++;

    // TLV0.Length = 0
    p++;

    if (needs_rct) {
        // Add RCT

        // Padding bytes
        p += needs_tlv2 ? 24 : 32;

        // RCT.SeqNr
        // Skip this field for now, but save a pointer to it, so that we can
        // update it on the fly.
        sv_frame.rct_ptr = p;
        p += 2;

        // RCT.PathId and PRP_LSDUsize (always 52).
        // Leave RCT.PathId at 0 for now, but set the size
        REDBOX_PDU_16bit_write(p, 52, true /* advance p */);

        // RCT.PRPsuffix
        REDBOX_PDU_16bit_write(p, REDBOX_ETYPE_SUPERVISION, true /* advance p */);
    }

    // Sanity check that the frame is of OK length
    if (p - sv_frame.tx_props.packet_info.frm != frm_len) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: Pre-calculated PDU length (%u) doesn't match final length (%u) for SV frame type = %d", redbox_state.inst, frm_len, p - sv_frame.tx_props.packet_info.frm, sv_frame_type);
        REDBOX_PDU_tx_props_dispose(sv_frame.tx_props);
        redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR;
        return VTSS_APPL_REDBOX_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_pdu_tx_templates_create()
// Creates a Supervision PDU template to be used by all proxied SANs.
//
// This function can also create a PRP SV frame template for use when proxying
// from HSR to PRP network (mode = HSR-PRP), and an HSR SV frame template for
// use when proxying from PRP to HSR ring (mode = HSR-PRP).
/******************************************************************************/
mesa_rc redbox_pdu_tx_templates_create(redbox_state_t &redbox_state)
{
    bool vlan_tagged;
    int  i;

    // Get rid of possible old frame templates
    REDBOX_PDU_tx_props_dispose_all(redbox_state);

    // We always need the four first
    for (i = 0; i < 2; i++) {
        vlan_tagged = i == 1;
        VTSS_RC(REDBOX_PDU_tx_template_create(redbox_state, REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT,         vlan_tagged));
        VTSS_RC(REDBOX_PDU_tx_template_create(redbox_state, REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2, vlan_tagged));
    }

    // And we need the last two in HSR-PRP mode only (to forward between HSR
    // ring and PRP network and vice versa).
    if (redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
        for (i = 0; i < 2; i++) {
            vlan_tagged = i == 1;
            VTSS_RC(REDBOX_PDU_tx_template_create(redbox_state, REDBOX_SV_FRAME_TYPE_FWD_PRP_TO_HSR, vlan_tagged));
            VTSS_RC(REDBOX_PDU_tx_template_create(redbox_state, REDBOX_SV_FRAME_TYPE_FWD_HSR_TO_PRP, vlan_tagged));
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_pdu_tx_free()
/******************************************************************************/
void redbox_pdu_tx_free(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t mac_itr;

    // Stop all ProxyNodeTable timers utilizing the Supervision PDU.
    for (mac_itr = redbox_state.mac_map.begin(); mac_itr != redbox_state.mac_map.end(); ++mac_itr) {
        if (mac_itr->second.is_pnt_entry) {
            redbox_timer_stop(mac_itr->second.pnt.timer);
        }
    }

    REDBOX_PDU_tx_props_dispose_all(redbox_state);
}

/******************************************************************************/
// redbox_pdu_tx_start()
/******************************************************************************/
void redbox_pdu_tx_start(redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr)
{
    if (!mac_itr->second.is_pnt_entry) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: %s is not a PNT entry", redbox_state.inst, mac_itr->first);
        return;
    }

    // Initialize Tx timer for this one.
    // We need both the RedBox instance and the MAC address in order to find the
    // frame and source MAC address to use in the called back function, which
    // transmits a Supervision frame.
    redbox_timer_init(mac_itr->second.pnt.timer, "PNT Entry", redbox_state.inst, REDBOX_PDU_tx_timeout, &mac_itr->first);

    // Make its first timeout random in range [0; sv_frame_interval_secs[ in
    // order to spread out Tx of Supervision frames in time in order not to
    // generate bursts. Avoid starting it with 0 ms.
    redbox_timer_start(mac_itr->second.pnt.timer, (rand() % (redbox_state.conf.sv_frame_interval_secs * 1000)) + 100, true);
}

/******************************************************************************/
// redbox_pdu_tx_stop()
/******************************************************************************/
void redbox_pdu_tx_stop(redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr)
{
    if (!mac_itr->second.is_pnt_entry) {
        T_EG(REDBOX_TRACE_GRP_FRAME_TX, "%u: %s is not a PNT entry", redbox_state.inst, mac_itr->first);
        return;
    }

    redbox_timer_stop(mac_itr->second.pnt.timer);
}

/******************************************************************************/
// redbox_pdu_frame_interval_changed()
/******************************************************************************/
void redbox_pdu_frame_interval_changed(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t mac_itr;
    uint64_t         new_period_ms = redbox_state.conf.sv_frame_interval_secs * 1000;

    // Loop through the ProxyNodeTable and adjust the timeout for the next
    // supervision PDU if needed.
    for (mac_itr = redbox_state.mac_map.begin(); mac_itr != redbox_state.mac_map.end(); ++mac_itr) {
        if (!mac_itr->second.is_pnt_entry) {
            continue;
        }

        if (!redbox_timer_active(mac_itr->second.pnt.timer)) {
            continue;
        }

        if (new_period_ms < mac_itr->second.pnt.timer.period_ms) {
            // Restart it with a random timeout in order to avoid bursting, but
            // avoid starting it with 0 ms.
            redbox_timer_start(mac_itr->second.pnt.timer, (rand() % new_period_ms) + 100, true);
        } else {
            // Let the current timer timeout. The callback function restarts it.
        }
    }
}

/******************************************************************************/
// redbox_pdu_rx_frame()
/******************************************************************************/
bool redbox_pdu_rx_frame(const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, redbox_pdu_info_t &rx_pdu_info, bool hsr_tagged)
{
    if (!REDBOX_PDU_rx_validation_pass(frm, rx_info, rx_pdu_info, hsr_tagged)) {
        T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "Frame Rx: Failed validation");
        T_IG_HEX(REDBOX_TRACE_GRP_FRAME_RX, frm, rx_info->length);
        return false;
    }

    T_IG(REDBOX_TRACE_GRP_FRAME_RX, "sv = %s", rx_pdu_info);

    return true;
}

/******************************************************************************/
// redbox_pdu_tx_hsr_to_prp_sv()
// Given a SV frame received on an LRE port in HSR-PRP mode, this function sends
// a PRP SV frame to all non-LRE ports in the VLAN and to RBs on LRE ports that
// are in HSR-PRP mode.
//
// EasyFrame example, where an HSR-tagged SV frame is sent into an LRE port:
// sudo ef -t 1000 name hsr_frm eth dmac 01:15:4e:00:01:90 smac ::4 htag pathid 8 size 52 seqn 43690 sv seqn 48059 tlv1_type 23 tlv1_mac ::4 data repeat 32 0 tx eth6_red name hsr_frm rx eth6_red rx eth6_green rx eth6_blue rx eth6_yellow
//
// EasyFrame example, where we wait 10 seconds without transmitting anything,
// but just print what comes out:
// sudo ef -t 10000 rx eth6_red rx eth6_green rx eth6_blue rx eth6_yellow
//
// EasyFrame example, where we send an HSR-tagged data frame into LRE port with
// DMAC = CPU:
// sudo ef name f eth dmac 02:00:c1:cc:4c:fc htag size 56 et 0xabcd data repeat 50 0xa tx eth6_red name f rx eth6_red rx eth6_green rx eth6_blue rx eth6_yellow
/******************************************************************************/
uint32_t redbox_pdu_tx_hsr_to_prp_sv(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, mesa_vlan_tag_t &class_tag)
{
    redbox_port_state_t *interlink_port_state;
    redbox_pdu_info_t   tx_pdu_info = rx_pdu_info;
    uint32_t            tx_cnt;
    uint8_t             *p;
    int                 idx;

    T_IG(REDBOX_TRACE_GRP_FRAME_TX, "%u: rx_pdu_info = %s, class_tag = %s", redbox_state.inst, rx_pdu_info, class_tag);

    // Other RedBoxes see this as received on their I/L port.
    tx_pdu_info.port_type = VTSS_APPL_REDBOX_PORT_TYPE_C;

    // The HSR SV frame received on the RB represented by redbox_state must be
    // transmitted to all ports in the VLAN it got classified to, but
    // transformed to a PRP SV frame.

    // We need to fill in two frames before starting transmission: One for VLAN
    // untagged and one for VLAN tagged transmission, because we don't know
    // up front which ports require a VLAN tag on egress and which don't.
    // The packet module can indeed insert a VLAN tag based on what we pass in
    // the tx_info, but unfortunately, requesting it to insert a VLAN tag causes
    // the DMAC and SMAC to be moved to another position, so that it's not the
    // same as when we called packet_tx().

    interlink_port_state = redbox_state.interlink_port_state_get();

    for (idx = 0; idx <= 1; idx++) {
        redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[REDBOX_SV_FRAME_TYPE_FWD_HSR_TO_PRP][idx];

        // The frame we are going to transmit now has the RB's SMAC.
        tx_pdu_info.smac = interlink_port_state->redbox_mac;

        // The frame we are going to transmit always uses PRP-DD TLV1 type.
        tx_pdu_info.tlv1_type = REDBOX_TLV_TYPE_PRP_DD;

        // Update DMAC.LSByte
        *sv_frame.dmac_lsb_ptr = rx_pdu_info.dmac_lsb;

        if (idx == 1) {
            // Gotta insert a VLAN tag. We can use any TPID, because it will be
            // updated to the correct one by the function that actually
            // transmits the frame.
            REDBOX_PDU_vlan_tag_write(sv_frame.vlan_tag_ptr, class_tag);
        }

        // Update TLV1.MAC
        memcpy(sv_frame.tlv1_mac_address_ptr, rx_pdu_info.tlv1_mac.addr, sizeof(rx_pdu_info.tlv1_mac.addr));

        // Corrigendum, 5.2.2.3.2.1, 2b:
        // If TLV2 is not present, insert TLV2 and replace the SupSequenceNumber
        // with the RB's own.
        if (rx_pdu_info.tlv2_type == REDBOX_TLV_TYPE_NONE) {
            // Update SupSequenceNumber. Wait with updating it until after we've
            // created both VLAN tagged and VLAN untagged frames. See below for
            // the update.
            REDBOX_PDU_16bit_write(sv_frame.sup_sequence_number_ptr, redbox_state.sup_sequence_number);

            // The frame we are going to transmit now has a different sequence
            // number
            tx_pdu_info.sup_sequence_number = redbox_state.sup_sequence_number;

            // The frame we are going to transmit now contains a TLV2
            tx_pdu_info.tlv2_type = REDBOX_TLV_TYPE_REDBOX;

            // RedBoxMacAddress.
            memcpy(sv_frame.tlv2_mac_address_ptr, interlink_port_state->redbox_mac.addr, sizeof(interlink_port_state->redbox_mac.addr));

            // The frame we are going to transmit now has a TLV2.MAC
            tx_pdu_info.tlv2_mac = interlink_port_state->redbox_mac;
        } else {
            // Leave it as was received in HSR SV frame
            REDBOX_PDU_16bit_write(sv_frame.sup_sequence_number_ptr, rx_pdu_info.sup_sequence_number);

            // MAC address as was received in HSR SV frame
            memcpy(sv_frame.tlv2_mac_address_ptr, rx_pdu_info.tlv2_mac.addr, sizeof(rx_pdu_info.tlv2_mac.addr));
        }

        // Update RCT.SeqNr. Don't update redundancy_tag_sequence_number until
        // we've updated both tagged and untagged frames. See below for the
        // update.
        REDBOX_PDU_16bit_write(sv_frame.rct_ptr, redbox_state.redundancy_tag_sequence_number);

        // Update RCT.LanId (needs on-the-fly update, because user may change
        // lan_id).
        p = sv_frame.rct_ptr + 2;
        *p = (redbox_state.conf.lan_id == VTSS_APPL_REDBOX_LAN_ID_A ? 0xa : 0xb) << 4;

        // The frame we are going to transmit now has an updated RCT
        tx_pdu_info.rct_present = true;
        tx_pdu_info.rct_lan_id = redbox_state.conf.lan_id;
    }

    tx_cnt = REDBOX_PDU_tx_in_vlan(redbox_state, REDBOX_SV_FRAME_TYPE_FWD_HSR_TO_PRP, tx_pdu_info, class_tag);

    if (tx_cnt) {
        // Gotta update the redundancy tag's sequence number if we have actually
        // transmitted a frame.
        redbox_state.redundancy_tag_sequence_number++;

        if (rx_pdu_info.tlv2_type == REDBOX_TLV_TYPE_NONE) {
            // Also update the SV sequence number if we have used it.
            redbox_state.sup_sequence_number++;
        }
    }

    return tx_cnt;
}

/******************************************************************************/
// redbox_pdu_tx_prp_to_hsr_sv()
// Given a SV frame (of any type) received on a non-LRE port in HSR-PRP mode,
// this function sends an HSR SV frame towards the RB's LRE ports.
// We send an RCT-tagged frame to the I/L port and leave it up to the H/W to
// move the RCT to an HSR tag (the frame egressing the LRE port will have the
// RCT stripped). We could as well have created an HSR-tagged frame in the first
// place.
// Notice that the RB only will change the RCT to an HSR tag if the frame's SMAC
// is present in the PNT as a DAN when sending the frame. This is not a problem,
// because we always send these frames with the RB's own MAC as SMAC, and we
// have already created a locked entry in the PNT for that one.
// If the SMAC was not present in the PNT or it was marked as a SAN, the RB
// would add it to the PNT and use its own SeqNr when HSR-tagging the frame.
//
// EasyFrame example, where an RCT-tagged SV frame is sent into a non-LRE port:
//  sudo ef -t 1000 name frame1 eth dmac 01:15:4e:00:01:fb smac ::4 sv seqn 48059 tlv1_type 21 tlv1_mac ::4 data repeat 32 0 prp seqn 52428 lanid 0xa size 52 tx eth6_green name frame1 rx eth6_red rx eth6_green rx eth6_blue rx eth6_yellow
/******************************************************************************/
void redbox_pdu_tx_prp_to_hsr_sv(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, mesa_vlan_tag_t &class_tag, bool tx_vlan_tagged)
{
    vtss_appl_redbox_sv_type_t sv_type;
    redbox_port_state_t        *interlink_port_state = redbox_state.interlink_port_state_get();
    uint8_t                    *p;
    bool                       port_a_link, port_b_link;

    port_a_link = redbox_state.port_link_get(VTSS_APPL_REDBOX_PORT_TYPE_A);
    port_b_link = redbox_state.port_link_get(VTSS_APPL_REDBOX_PORT_TYPE_B);

    T_IG(REDBOX_TRACE_GRP_FRAME_TX, "%u: rx_pdu_info = %s, class_tag = %s, port-a link = %d, port-b link = %d", redbox_state.inst, rx_pdu_info, class_tag, port_a_link, port_b_link);

    if (!port_a_link && !port_b_link) {
        // Don't send supervision frames if not link on at least one of the LRE
        // ports.
        return;
    }

    // If forwarding the received SV frame as is to the HSR ring, we need a
    // different template than when we create an HSR frame from the received
    // SV frame. If forwarding as is, we also either need or don't need a TLV2
    redbox_sv_frame_t &sv_frame = redbox_state.sv_frames[REDBOX_SV_FRAME_TYPE_FWD_PRP_TO_HSR][tx_vlan_tagged ? 1 : 0];

    // Get the I/L port state
    interlink_port_state = redbox_state.interlink_port_state_get();

    // Set port to which we Tx this frame
    sv_frame.tx_props.tx_info.dst_port_mask = VTSS_BIT64(interlink_port_state->port_no);

    // Update DMAC.LSByte
    *sv_frame.dmac_lsb_ptr = rx_pdu_info.dmac_lsb;

    if (tx_vlan_tagged) {
        // Gotta insert a tag. We use TPID from the LRE port and VID, PCP, and
        // DEI from the classified tag.
        class_tag.tpid = interlink_port_state->tpid;
        REDBOX_PDU_vlan_tag_write(sv_frame.vlan_tag_ptr, class_tag);
    }

    // Update TLV1.MAC
    memcpy(sv_frame.tlv1_mac_address_ptr, rx_pdu_info.tlv1_mac.addr, sizeof(rx_pdu_info.tlv1_mac.addr));

    // Corrigendum, 5.2.2.3.2.1, 2b:
    // If TLV2 is not present, insert TLV2 and replace the SupSequenceNumber
    // with the RB's own.
    if (rx_pdu_info.tlv2_type == REDBOX_TLV_TYPE_NONE) {
        // Update SupSequenceNumber
        REDBOX_PDU_16bit_write(sv_frame.sup_sequence_number_ptr, redbox_state.sup_sequence_number++);

        // RedBoxMacAddress.
        memcpy(sv_frame.tlv2_mac_address_ptr, interlink_port_state->redbox_mac.addr, sizeof(interlink_port_state->redbox_mac.addr));
    } else {
        // Leave it as was received in PRP SV frame
        REDBOX_PDU_16bit_write(sv_frame.sup_sequence_number_ptr, rx_pdu_info.sup_sequence_number);

        // MAC address as was received in PRP SV frame
        memcpy(sv_frame.tlv2_mac_address_ptr, rx_pdu_info.tlv2_mac.addr, sizeof(rx_pdu_info.tlv2_mac.addr));
    }

    // Update RCT.SeqNr
    REDBOX_PDU_16bit_write(sv_frame.rct_ptr, redbox_state.redundancy_tag_sequence_number++);

    // Update RCT.LanId (needs on-the-fly update, because user may change
    // lan_id).
    p = sv_frame.rct_ptr + 2;
    *p = (redbox_state.conf.lan_id == VTSS_APPL_REDBOX_LAN_ID_A ? 0xa : 0xb) << 4;

    // Change the IFH to only transmit to the LRE ports with link. This is
    // needed in order to prevent the H/W counters from counting on LRE ports
    // without link (sigh!).
    sv_frame.tx_props.tx_info.rb_fwd = REDBOX_PDU_link_to_fwd(port_a_link, port_b_link);
    (void)REDBOX_PDU_packet_tx(redbox_state, sv_frame, interlink_port_state->port_no, __FUNCTION__);
    sv_frame.tx_props.tx_info.rb_fwd = REDBOX_PDU_link_to_fwd(true, true);

    sv_type = redbox_pdu_tlv_type_to_sv_type((redbox_tlv_type_t)(*sv_frame.tlv1_type_ptr));
    if (port_a_link) {
        redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_A].sv_tx_cnt[sv_type]++;
    }

    if (port_b_link) {
        redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_B].sv_tx_cnt[sv_type]++;
    }
}

/******************************************************************************/
// redbox_pdu_tlv_type_to_sv_type()
/******************************************************************************/
vtss_appl_redbox_sv_type_t redbox_pdu_tlv_type_to_sv_type(redbox_tlv_type_t tlv_type)
{
    switch (tlv_type) {
    case REDBOX_TLV_TYPE_PRP_DD:
        return VTSS_APPL_REDBOX_SV_TYPE_PRP_DD;

    case REDBOX_TLV_TYPE_PRP_DA:
        return VTSS_APPL_REDBOX_SV_TYPE_PRP_DA;

    case REDBOX_TLV_TYPE_HSR:
        return VTSS_APPL_REDBOX_SV_TYPE_HSR;

    default:
        T_EG(REDBOX_TRACE_GRP_FRAME_RX, "Cannot convert tlv_type = %d to a sv_type", tlv_type);
        return VTSS_APPL_REDBOX_SV_TYPE_PRP_DD;
    }
}

