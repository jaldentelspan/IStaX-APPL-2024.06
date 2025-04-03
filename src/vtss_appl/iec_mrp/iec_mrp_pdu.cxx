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

#include "iec_mrp_base.hxx"
#include "iec_mrp_pdu.hxx"
#include "iec_mrp_trace.h"

// Currently only one version is defined by the standard.
#define MRP_PDU_VERSION 1

// Most TLVs and SubTLVs must be 32-bit aligned, which is what this macro
// calculates.
#define MRP_PDU_ALIGN_32(_l_) (((_l_) + 3) & ~0x3u)

// The DMAC used in MRP PDUs. The last byte is modified by the code to match
// the mrp_pdu_dmac_type_t.
const mesa_mac_t mrp_multicast_dmac = {{0x01, 0x15, 0x4e, 0x00, 0x00, 0x00}};

/******************************************************************************/
// MRP_PDU_tlv_type_to_pdu_type()
/******************************************************************************/
static vtss_appl_iec_mrp_pdu_type_t MRP_PDU_tlv_type_to_pdu_type(mrp_pdu_tlv_type_t tlv_type)
{
    switch (tlv_type) {
    case MRP_PDU_TLV_TYPE_TEST:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_TEST;

    case MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_TOPOLOGY_CHANGE;

    case MRP_PDU_TLV_TYPE_LINK_DOWN:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_DOWN;

    case MRP_PDU_TLV_TYPE_LINK_UP:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP;

    case MRP_PDU_TLV_TYPE_IN_TEST:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST;

    case MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TOPOLOGY_CHANGE;

    case MRP_PDU_TLV_TYPE_IN_LINK_DOWN:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_DOWN;

    case MRP_PDU_TLV_TYPE_IN_LINK_UP:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP;

    case MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_STATUS_POLL;

    case MRP_PDU_TLV_TYPE_OPTION:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION;

    default:
        return VTSS_APPL_IEC_MRP_PDU_TYPE_UNKNOWN;
    }
}

/******************************************************************************/
// MRP_PDU_tlv_type_to_id()
//
// Unfortunately, the header type IDs are not consecutive, and because we also
// use the type directly to index into an array of sequence IDs, we have this
// function to convert from type to ID.
/******************************************************************************/
static uint8_t MRP_PDU_tlv_type_to_id(mrp_pdu_tlv_type_t tlv_type)
{
    switch (tlv_type) {
    case MRP_PDU_TLV_TYPE_END:
        return 0x00;

    case MRP_PDU_TLV_TYPE_COMMON:
        return 0x01;

    case MRP_PDU_TLV_TYPE_TEST:
        return 0x02;

    case MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE:
        return 0x03;

    case MRP_PDU_TLV_TYPE_LINK_DOWN:
        return 0x04;

    case MRP_PDU_TLV_TYPE_LINK_UP:
        return 0x05;

    case MRP_PDU_TLV_TYPE_IN_TEST:
        return 0x06;

    case MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE:
        return 0x07;

    case MRP_PDU_TLV_TYPE_IN_LINK_DOWN:
        return 0x08;

    case MRP_PDU_TLV_TYPE_IN_LINK_UP:
        return 0x09;

    case MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL:
        return 0x0a;

    case MRP_PDU_TLV_TYPE_OPTION:
        return 0x7f;

    default:
        T_EG(MRP_TRACE_GRP_FRAME_TX, "Invalid tlv_type (%d)", tlv_type);
        return 0xff;
    }
}

/******************************************************************************/
// MRP_PDU_tlv_type_from_id()
//
// Unfortunately, the header type IDs are not consecutive, and because we also
// use the type directly to index into an array of sequence IDs, we have this
// function to convert from ID to type.
/******************************************************************************/
static mrp_pdu_tlv_type_t MRP_PDU_tlv_type_from_id(uint8_t id)
{
    switch (id) {
    case 0x00:
        return MRP_PDU_TLV_TYPE_END;

    case 0x01:
        return MRP_PDU_TLV_TYPE_COMMON;

    case 0x02:
        return MRP_PDU_TLV_TYPE_TEST;

    case 0x03:
        return MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE;

    case 0x04:
        return MRP_PDU_TLV_TYPE_LINK_DOWN;

    case 0x05:
        return MRP_PDU_TLV_TYPE_LINK_UP;

    case 0x06:
        return MRP_PDU_TLV_TYPE_IN_TEST;

    case 0x07:
        return MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE;

    case 0x08:
        return MRP_PDU_TLV_TYPE_IN_LINK_DOWN;

    case 0x09:
        return MRP_PDU_TLV_TYPE_IN_LINK_UP;

    case 0x0a:
        return MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL;

    case 0x7f:
        return MRP_PDU_TLV_TYPE_OPTION;

    default:
        T_IG(MRP_TRACE_GRP_FRAME_RX, "Invalid TLVHeader.Type (%d)", id);
        return MRP_PDU_TLV_TYPE_NONE;
    }
}

/******************************************************************************/
// MRP_PDU_sub_tlv_type_from_id()
/******************************************************************************/
static mrp_pdu_sub_tlv_type_t MRP_PDU_sub_tlv_type_from_id(uint8_t id)
{
    // There's a one-to-one correspondance between the ID and our enumeration.
    switch ((mrp_pdu_sub_tlv_type_t)id) {
    case MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK:
    case MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE:
    case MRP_PDU_SUB_TLV_TYPE_AUTO_MGR:
        return (mrp_pdu_sub_tlv_type_t)id;

    default:
        T_IG(MRP_TRACE_GRP_FRAME_RX, "Unknown SubTLV Type (%d)", id);
        return MRP_PDU_SUB_TLV_TYPE_NONE;
    }
}

/******************************************************************************/
// MRP_PDU_port_role_to_bin()
/******************************************************************************/
static uint16_t MRP_PDU_port_role_to_bin(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    return port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION ? 2 : port_type == mrp_state->vars.prm_ring_port ? 0 : 1;
}

/******************************************************************************/
// MRP_PDU_16bit_read()
/******************************************************************************/
static uint16_t MRP_PDU_16bit_read(const uint8_t *&p)
{
    uint16_t val;

    val  = *(p++) << 8;
    val |= *(p++);

    return val;
}

/******************************************************************************/
// MRP_PDU_32bit_read()
/******************************************************************************/
static uint32_t MRP_PDU_32bit_read(const uint8_t *&p)
{
    uint32_t val;

    val  = *(p++) << 24;
    val |= *(p++) << 16;
    val |= *(p++) <<  8;
    val |= *(p++) <<  0;

    return val;
}

/******************************************************************************/
// MRP_PDU_16bit_write()
/******************************************************************************/
static void MRP_PDU_16bit_write(uint8_t *&p, uint16_t val, bool advance = true)
{
    uint8_t *p1 = p;

    *(p1++) = (val >> 8) & 0xFF;
    *(p1++) = (val >> 0) & 0xFF;

    if (advance) {
        p = p1;
    }
}

/******************************************************************************/
// MRP_PDU_32bit_write()
/******************************************************************************/
static void MRP_PDU_32bit_write(uint8_t *&p, uint32_t val, bool advance = true)
{
    uint8_t *p1 = p;

    *(p1++) = (val >> 24) & 0xFF;
    *(p1++) = (val >> 16) & 0xFF;
    *(p1++) = (val >>  8) & 0xFF;
    *(p1++) = (val >>  0) & 0xFF;

    if (advance) {
        p = p1;
    }
}

/******************************************************************************/
// MRP_PDU_tx_props_dispose()
/******************************************************************************/
static void MRP_PDU_tx_props_dispose(packet_tx_props_t *tx_props)
{
    if (tx_props->packet_info.frm) {
        packet_tx_free(tx_props->packet_info.frm);
        tx_props->packet_info.frm = nullptr;
    }

    tx_props->packet_info.len = 0;
}

/******************************************************************************/
// MRP_PDU_tx()
/******************************************************************************/
static void MRP_PDU_tx(mrp_state_t *mrp_state, packet_tx_props_t *tx_props, vtss_appl_iec_mrp_port_type_t port_type, vtss_appl_iec_mrp_pdu_type_t pdu_type)
{
    mesa_rc rc;

    if (!mrp_state->port_states[port_type]->link) {
        return;
    }

    // Statistics
    mrp_state->status.port_status[port_type].statistics.tx_cnt[pdu_type]++;

    if (pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_TEST || pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST) {
        // Gotta really want to see these to see them
        T_NG(MRP_TRACE_GRP_FRAME_TX, "%u: packet_tx(%s, %s, length = %u)", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type, false, true), iec_mrp_util_pdu_type_to_str(pdu_type), tx_props->packet_info.len);
    } else {
        T_IG(MRP_TRACE_GRP_FRAME_TX, "%u: packet_tx(%s, %s, length = %u)", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type, false, true), iec_mrp_util_pdu_type_to_str(pdu_type), tx_props->packet_info.len);
    }

    if ((rc = packet_tx(tx_props)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: packet_tx(%s, %s) failed: %s", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type), iec_mrp_util_pdu_type_to_str(pdu_type), error_txt(rc));
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_PDU_len_check()
/******************************************************************************/
static mesa_rc MRP_PDU_len_check(mrp_state_t *mrp_state, packet_tx_props_t *tx_props, uint8_t *p, vtss_appl_iec_mrp_pdu_type_t pdu_type, bool interconnection)
{
    uint32_t   offset = p - tx_props->packet_info.frm;
    mesa_vid_t vid    = interconnection ? mrp_state->conf.in_vlan : mrp_state->conf.vlan;

    // Normalize offset to a minimum-sized frame depending on tagged/untagged
    // operation.
    offset = MAX(vid == 0 ? 60 : 64, offset);

    if (offset != tx_props->packet_info.len) {
        T_EG(MRP_TRACE_GRP_FRAME_TX, "Pre-calculated %s PDU length (%u) doesn't match final length (%u)", iec_mrp_util_pdu_type_to_str(pdu_type), tx_props->packet_info.len, offset);
        MRP_PDU_tx_props_dispose(tx_props);
        return VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_l2_len_get()
// Returns the size of the required PDU from DMAC up to and including
// MRP_Version.
/******************************************************************************/
static uint32_t MRP_PDU_l2_len_get(mrp_state_t *mrp_state, bool interconnect)
{
    uint32_t l2_len;
    bool     tagged = interconnect ? mrp_state->conf.in_vlan != 0 : mrp_state->conf.vlan != 0;

    // Figure out how much space we need
    l2_len  = sizeof(mesa_mac_t) /* DMAC                      */ +
              sizeof(mesa_mac_t) /* SMAC                      */ +
              (tagged ? 4 : 0)   /* Possible VLAN tag         */ +
              2                  /* Ethertype                 */ +
              2                  /* MRP_Version               */;

    return l2_len;
}

/******************************************************************************/
// MRP_PDU_ed1_manufacturer_len_get()
/******************************************************************************/
static uint32_t MRP_PDU_ed1_manufacturer_len_get(uint8_t oui[3], uint8_t ed1_type)
{
    if (oui[0] == 0x00 && oui[1] == 0x15 && oui[2] == 0x4e) {
        // Table 27 (OUI == IECOUI == 00-15-e4)
        return 0;
    }

    // Table 26 (OUI != IECOUI)
    switch (ed1_type) {
    case 0:
        return 2;

    case 4:
        return 26;

    default:
        return 0;
    }
}

/******************************************************************************/
// MRP_PDU_sub_tlv_len_get()
// Returns the overall SubTLV length, including MRP_SubTLVHeader of 2 bytes.
/******************************************************************************/
static uint32_t MRP_PDU_sub_tlv_len_get(mrp_pdu_sub_tlv_type_t sub_tlv_type, uint32_t *sub_padding = nullptr)
{
    uint32_t sub_tlv_len;
    uint32_t the_sub_padding;

    if (sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_NONE) {
        // Used internally to indicate that we don't have a SubTLV.
        if (sub_padding) {
            *sub_padding = 0;
        }

        return 0;
    }

    sub_tlv_len = 2 /* MRP_SubTLVHeader */;

    switch (sub_tlv_type) {
    case MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK:
    case MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE:
        sub_tlv_len += 2                  /* MRP_Prio         */ +
                       sizeof(mesa_mac_t) /* MRP_SA           */ +
                       2                  /* MRP_OtherMRMPrio */ +
                       sizeof(mesa_mac_t) /* MRP_OtherMRMSA   */;
        break;

    case MRP_PDU_SUB_TLV_TYPE_AUTO_MGR:
        // This one doesn't need to be 32-bit aligned according to Table 23.
        if (sub_padding) {
            *sub_padding = 0;
        }

        return sub_tlv_len;

    default:
        T_IG(MRP_TRACE_GRP_FRAME_RX, "Invalid sub_tlv_type (%d)", sub_tlv_type);
        if (sub_padding) {
            *sub_padding = 0;
        }

        return 0;
    }

    // If we get here, the overall SubTLV length must be a multiple of 4.
    the_sub_padding = MRP_PDU_ALIGN_32(sub_tlv_len) - sub_tlv_len;

    if (sub_padding) {
        *sub_padding = the_sub_padding;
    }

    return sub_tlv_len + the_sub_padding;
}

/******************************************************************************/
// MRP_PDU_tlv_len_get()
// Returns the overall TLV length, including MRP_TLVHeader of 2 bytes.
/******************************************************************************/
static uint32_t MRP_PDU_tlv_len_get(mrp_pdu_tlv_type_t tlv_type, uint32_t *padding = nullptr, mrp_pdu_sub_tlv_type_t sub_tlv_type = MRP_PDU_SUB_TLV_TYPE_NONE)
{
    uint32_t the_padding;
    uint32_t tlv_len = 2 /* MRP_TLV_Header */;

    switch (tlv_type) {
    case MRP_PDU_TLV_TYPE_END:
        // No data
        if (padding) {
            *padding = 0;
        }

        return tlv_len;

    case MRP_PDU_TLV_TYPE_COMMON:
        tlv_len += 2                  /* MRP_SequenceID */ +
                   16                 /* MRP_DomainUUID */;
        break;

    case MRP_PDU_TLV_TYPE_TEST:
        tlv_len += 2                  /* MRP_Prio       */ +
                   sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_PortRole   */ +
                   2                  /* MRP_RingState  */ +
                   2                  /* MRP_Transition */ +
                   4                  /* MRP_TimeStamp  */;
        break;

    case MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE:
        tlv_len += 2                  /* MRP_Prio       */ +
                   sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_Interval   */;
        break;

    case MRP_PDU_TLV_TYPE_LINK_DOWN:
    case MRP_PDU_TLV_TYPE_LINK_UP:
        tlv_len += sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_PortRole   */ +
                   2                  /* MRP_Interval   */ +
                   2                  /* MRP_Blocked    */;
        break;

    case MRP_PDU_TLV_TYPE_IN_TEST:
        tlv_len += 2                  /* MRP_InID       */ +
                   sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_PortRole   */ +
                   2                  /* MRP_InState    */ +
                   2                  /* MRP_Transition */ +
                   4                  /* MRP_TimeStamp  */;
        break;

    case MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE:
        tlv_len += sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_InID       */ +
                   2                  /* MRP_Interval   */;
        break;

    case MRP_PDU_TLV_TYPE_IN_LINK_DOWN:
    case MRP_PDU_TLV_TYPE_IN_LINK_UP:
        tlv_len += sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_PortRole   */ +
                   2                  /* MRP_InID       */ +
                   2                  /* MRP_Interval   */;
        break;

    case MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL:
        tlv_len += sizeof(mesa_mac_t) /* MRP_SA         */ +
                   2                  /* MRP_PortRole   */ +
                   2                  /* MRP_InID       */;
        break;

    case MRP_PDU_TLV_TYPE_OPTION: {
        // Dummy OUI
        uint8_t oui[3] = {};

        tlv_len += 3 /* MRP_OUI                                          */ +
                   1 /* MRP_Ed1Type                                      */ +
                   MRP_PDU_ed1_manufacturer_len_get(oui, 0 /* We always insert a 0 for MRP_Ed1Type) */) +
                   MRP_PDU_sub_tlv_len_get(sub_tlv_type);
        break;
    }

    default:
        T_IG(MRP_TRACE_GRP_FRAME_RX, "Invalid tlv_type (%d)", tlv_type);

        if (padding) {
            *padding = 0;
        }

        return 0;
    }

    // The overall TLV length must be a multiple of 4, except if TLV Type is
    // MRP_PDU_TLV_TYPE_END, in which case we can't get here.
    the_padding = MRP_PDU_ALIGN_32(tlv_len) - tlv_len;

    if (padding) {
        *padding = the_padding;
    }

    return tlv_len + the_padding;
}

/******************************************************************************/
// MRP_PDU_tlv_len_chk()
/******************************************************************************/
static bool MRP_PDU_tlv_len_chk(mrp_state_t *mrp_state, mrp_pdu_tlv_type_t tlv_type, uint32_t tlv_len)
{
    bool     pass = true;
    uint32_t exp_len;

    if (tlv_type == MRP_PDU_TLV_TYPE_OPTION) {
        // MRP_Option TLVs don't have exact lengths, and since we don't know
        // the MRP_OUI and MRP_Ed1Type yet, we don't know how much to expect, so
        // form now we can only expect it to have MRP_OUI (3 bytes) and
        // MRP_Ed1Type (1 byte).
        exp_len = 3 + 1;
        if (tlv_len < exp_len) {
            pass = false;
        }

    } else {
        // All other TLVs have exact lengths.
        exp_len = MRP_PDU_tlv_len_get(tlv_type) - 2 /* MRP_TLVHeader */;

        if (tlv_len != exp_len) {
            pass = false;
        }
    }

    if (!pass) {
        T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Unexpected TLV length (type = %s, length = %d, expected at least = %d", mrp_state->inst, mrp_pdu_tlv_type_to_str(tlv_type), tlv_len, exp_len);
    }

    return pass;
}

/******************************************************************************/
// MRP_PDU_sub_tlv_len_chk()
/******************************************************************************/
static bool MRP_PDU_sub_tlv_len_chk(mrp_state_t *mrp_state, mrp_pdu_sub_tlv_type_t sub_tlv_type, uint32_t sub_tlv_len)
{
    uint32_t exp_len = MRP_PDU_sub_tlv_len_get(sub_tlv_type) - 2 /* MRP_SubTLVHeader */;

    if (sub_tlv_len != exp_len) {
        T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Unexpected SubTLV length (type = %s, length = %d, expected %d", mrp_state->inst, mrp_pdu_sub_tlv_type_to_str(sub_tlv_type), sub_tlv_len, exp_len);
        return false;
    }

    return true;
}

/******************************************************************************/
// MRP_PDU_sub_tlv_process()
// p points to the first SubTLV (MRP_SubOption2)
// length contains the overall remaining length of all MRP_SubOption2 TLVs.
// Due to the silly way the protocol is specified with padding for 32-bit
// alignment, it's not possible to parse more than one SubTLV. The padding of
// e.g. a MRP_TestMgrNAck PDU is two bytes, which belong to the parent TLV, but
// there's no way to see that when handling the SubTLVs.
// The Wireshark dissector for MRP_Option PDUs also only handles one
// MRP_SubOption2 TLV.
/******************************************************************************/
static bool MRP_PDU_sub_tlv_process(mrp_state_t *mrp_state, const uint8_t *p, uint32_t length, mrp_rx_pdu_info_t &rx_pdu_info)
{
    mrp_pdu_sub_tlv_type_t sub_tlv_type;
    uint8_t                sub_tlv_id;
    uint32_t               sub_tlv_len;

    if (length == 0) {
        return true;
    }

    // This TLV type spans a number of different sub TLV types, of which we only
    // really care about MRP_TestMgrNAck and MRP_TestPropagate
    sub_tlv_id   = *(p++);
    sub_tlv_len  = *(p++);
    length      -= 2;
    sub_tlv_type = MRP_PDU_sub_tlv_type_from_id(sub_tlv_id);

    T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: Got SubTLV.Type = %d = %s, SubTLV.Length = %u", mrp_state->inst, sub_tlv_id, mrp_pdu_sub_tlv_type_to_str(sub_tlv_type), sub_tlv_len);

    // Length check to avoid extracting fields from uninitialized memory.
    if (sub_tlv_len > length) {
        T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Got short MRP_SubOption2. Expected at least %u bytes, but got only %u", mrp_state->inst, sub_tlv_len, length);
        return false;
    }

    // If we don't recognize the SubTLV type, skip past it
    if (sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_NONE) {
        // The Siemens box forwards MRP_InTopologyChange PDUs received on its
        // interconnection to the ring ports while changing the frame's SMAC to
        // its own AND inserting its own domain-UUID in the common TLV AND
        // appending an option TLV with the original OUI and a subTLV type of
        // 0xf2 (242) containing the original domain-UUID followed by, I think,
        // two padding bytes, or perhaps the original IID.
        // Anyway, since this code doesn't know the 0xF2 subtlv type, we end up
        // in this piece of code.
        T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: Got unknown SubTLV ID (%d). Skipping", mrp_state->inst, sub_tlv_id);

        // Skip past this SubTLV.
        return true;
    }

    // Length check of the particular TLV
    if (!MRP_PDU_sub_tlv_len_chk(mrp_state, sub_tlv_type, sub_tlv_len)) {
        // Trace already printed.
        return false;
    }

    switch (sub_tlv_type) {
    case MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK:
    case MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE:
        // MRP_Prio         (2)
        // MRP_SA           (6)
        // MRP_OtherMRMPrio (2)
        // MRP_OtherMRMSA   (6)

        // TestMgrNAckInd() only utilizes MRP_SA, MRP_Prio, and
        // MRP_OtherMRMSA.
        // TestPropagateInd() utilizes all four fields, so transfer them

        // MRP_Prio (clause 8.1.15)
        rx_pdu_info.prio = MRP_PDU_16bit_read(p);

        // MRP_SA (clause 8.1.13)
        memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
        p += sizeof(rx_pdu_info.sa.addr);

        // MRP_OtherMRMPrio (clause 8.1.16)
        rx_pdu_info.other_prio = MRP_PDU_16bit_read(p);

        // MRP_OtherMRMSA (clause 8.1.14)
        memcpy(rx_pdu_info.other_sa.addr, p, sizeof(rx_pdu_info.other_sa.addr));
        p += sizeof(rx_pdu_info.other_sa.addr);

        // Override already found MRP_Option PDU type
        rx_pdu_info.pdu_type = sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK ? VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_MGR_NACK : VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_PROPAGATE;
        break;

    case MRP_PDU_SUB_TLV_TYPE_AUTO_MGR:
    default:
        // Ignore it.
        break;
    }

    return true;
}

/******************************************************************************/
// MRP_PDU_rx_validation_pass()
/******************************************************************************/
static bool MRP_PDU_rx_validation_pass(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, uint32_t rx_length, mrp_rx_pdu_info_t &rx_pdu_info)
{
    mrp_pdu_tlv_type_t     tlv_type;
    uint8_t                tlv_id, oui[3], val8;
    uint32_t               tlv_len, used_tlv_len, remaining_len;
    bool                   first_tlv = true, done = false;
    const uint8_t          *p, *tlv_start_p;

    vtss_clear(rx_pdu_info);
    rx_pdu_info.pdu_type = VTSS_APPL_IEC_MRP_PDU_TYPE_UNKNOWN;

    // As long as we can't receive a frame behind two tags, the MRP PDU contents
    // always starts at offset 14 (two MACs + EtherType) because the packet
    // module  strips a possible outer tag.
    p = &frm[14];

    // Version
    // Be forgiving when interpreting the Version, so just skip past it
    p += 2;

    while (1) {
        // TLVHeader.Type
        tlv_id      = *(p++);
        tlv_len     = *(p++);
        tlv_type = MRP_PDU_tlv_type_from_id(tlv_id);

        // Save a pointer to the first data byte of this TLV
        tlv_start_p = p;

        T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: Got TLV.Type = %d = %s, TLV.Length = %u", mrp_state->inst, tlv_id, mrp_pdu_tlv_type_to_str(tlv_type), tlv_len);

        // Length check to avoid extracting fields from uninitialized memory.
        if (p - frm + tlv_len > rx_length) {
            T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Got short frame. Expected at least %u bytes, but got only %u", mrp_state->inst, p - frm + tlv_len, rx_length);
            return false;
        }

        // If we don't recognize the TLV type, either skip past it or skip the
        // entire frame.
        if (tlv_type == MRP_PDU_TLV_TYPE_NONE) {
            T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Got unknown TLV ID (%d)", mrp_state->inst, tlv_id);

            if (first_tlv) {
                // We don't recognize the frame.
                return false;
            } else {
                // Skip past this TLV.
                p += tlv_len;
                continue;
            }
        }

        if (first_tlv) {
            rx_pdu_info.pdu_type = MRP_PDU_tlv_type_to_pdu_type(tlv_type);
            first_tlv            = false;
        }

        // Length check of the particular TLV
        if (!MRP_PDU_tlv_len_chk(mrp_state, tlv_type, tlv_len)) {
            // Trace already printed.
            return false;
        }

        switch (tlv_type) {
        case MRP_PDU_TLV_TYPE_END:
            // Don't care about anything after the MRP_End TLV.
            done = true;
            break;

        case MRP_PDU_TLV_TYPE_COMMON:
            // MRP_SequenceID (clause 8.1.12)
            rx_pdu_info.sequence_id = MRP_PDU_16bit_read(p);

            // MRP_DomainUUID (clause 8.1.26)
            memcpy(rx_pdu_info.domain_id, p, sizeof(rx_pdu_info.domain_id));
            p += sizeof(rx_pdu_info.domain_id);
            break;

        case MRP_PDU_TLV_TYPE_TEST:
            // MRP_Prio       (2)
            // MRP_SA         (6)
            // MRP_PortRole   (2)
            // MRP_RingState  (2)
            // MRP_Transition (2)
            // MRP_TimeStamp  (4)

            // TestRingInd() only utilizes MRP_Prio and MRP_SA.

            // MRP_Prio (clause 8.1.15)
            rx_pdu_info.prio = MRP_PDU_16bit_read(p);

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // Skip past MRP_PortRole, MRP_RingState, MRP_Transition
            p += 2 + 2 + 2;

            // Get the MRP_Timestamp (for round-trip time calculations).
            rx_pdu_info.timestamp = MRP_PDU_32bit_read(p);
            break;

        case MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE:
            // MRP_Prio     (2)
            // MRP_SA       (6)
            // MRP_Interval (2)

            // TopologyChangeInd() only utilizes MRP_SA and MRP_Interval.

            // Skip past MRP_Prio
            p += 2;

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // MRP_Interval (clause 8.1.19)
            rx_pdu_info.interval_msec = MRP_PDU_16bit_read(p);
            break;

        case MRP_PDU_TLV_TYPE_LINK_DOWN:
        case MRP_PDU_TLV_TYPE_LINK_UP:
            // MRP_SA        (6)
            // MRP_PortRole  (2)
            // MRP_Interval  (2)
            // MRP_Blocked   (2)

            // LinkChangeInd() only utilizes MRP_Blocked and the fact that it's
            // either a Link Down or Link UP TLV, but we parse MRP_SA anyway,
            // because we use it for counting our own PDUs vs. others'.

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // Skip past MRP_PortRole, and MRP_Interval
            p += 2 + 2;

            rx_pdu_info.blocked = MRP_PDU_16bit_read(p);
            break;

        case MRP_PDU_TLV_TYPE_IN_TEST:
            // MRP_InID       (2)
            // MRP_SA         (6)
            // MRP_PortRole   (2)
            // MRP_InState    (2)
            // MRP_Transition (2)
            // MRP_TimeStamp  (4)

            // InterconnTestInd() only utilizes MRP_SA, the port that
            // received the PDU and MRP_InID.

            // MRP_InID (clause 8.1.28)
            rx_pdu_info.in_id = MRP_PDU_16bit_read(p);

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // Skip past MRP_PortRole, MRP_InState, MRP_Transition
            p += 2 + 2 + 2;

            // Get the MRP_Timestamp (for round-trip time calculations).
            rx_pdu_info.timestamp = MRP_PDU_32bit_read(p);
            break;

        case MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE:
            // MRP_SA       (6)
            // MRP_InID     (2)
            // MRP_Interval (2)

            // InterconnTopologyChangeInd() utilizes all fields from the PDU

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // MRP_InID (clause 8.1.28)
            rx_pdu_info.in_id = MRP_PDU_16bit_read(p);

            // MRP_Interval (clause 8.1.19)
            rx_pdu_info.interval_msec = MRP_PDU_16bit_read(p);
            break;

        case MRP_PDU_TLV_TYPE_IN_LINK_DOWN:
        case MRP_PDU_TLV_TYPE_IN_LINK_UP:
            // MRP_SA       (6)
            // MRP_PortRole (2)
            // MRP_InID     (2)
            // MRP_Interval (2)

            // InterconnLinkChangeInd() only utilizes MRP_InID and the fact that
            // it's either an MRP_InLinkDown or an MRP_InLinkUp PDU type.
            // But to detect whether this is our own PDU, we also need to parse
            // SA.

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // Skip past MRP_PortRole
            p += 2;

            // MRP_InID (clause 8.1.28)
            rx_pdu_info.in_id = MRP_PDU_16bit_read(p);

            // Skip past MRP_Interval
            p += 2;
            break;

        case MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL:
            // MRP_SA       (6)
            // MRP_PortRole (2)
            // MRP_InID     (2)

            // InterConnLinkStatusInd() only utilizes MRP_InID from the PDU.
            // But to detect whether this is our own PDU, we also need to parse
            // SA.

            // MRP_SA (clause 8.1.13)
            memcpy(rx_pdu_info.sa.addr, p, sizeof(rx_pdu_info.sa.addr));
            p += sizeof(rx_pdu_info.sa.addr);

            // Skip past MRP_PortRole
            p += 2;

            // MRP_InID (clause 8.1.28)
            rx_pdu_info.in_id = MRP_PDU_16bit_read(p);
            break;

        case MRP_PDU_TLV_TYPE_OPTION:
            // MRP_OUI                   (3)
            // MRP_SubOption1:
            //   MRP_Ed1Type             (1)
            //   MRP_Ed1ManufacturerData (variable, possibly 0 bytes long)
            // MRP_SubOption2*:          (variable)

            // We need the MRP_OUI in order to compute the correct length of the
            // variable MRP_Ed1ManufacturerData field.
            oui[0] = *(p++);
            oui[1] = *(p++);
            oui[2] = *(p++);

            // MRP_Ed1Type
            val8 = *(p++);

            // Skip past MRP_Ed1ManufacturerData
            p += MRP_PDU_ed1_manufacturer_len_get(oui, val8);

            // Length check to avoid extracting fields from uninitialized
            // memory.
            used_tlv_len = p - tlv_start_p;
            if (used_tlv_len > tlv_len) {
                T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: MRP_Option TLV: Expected at least %u bytes, but got only %u", mrp_state->inst, used_tlv_len, tlv_len);
                return false;
            }

            remaining_len = tlv_len - used_tlv_len;
            T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: MRP_Option.SubTLVs: tlv_len = %u, used_tlv_len = %u => remaining_len = %u", mrp_state->inst, tlv_len, used_tlv_len, remaining_len);

            // Process MRP_SubOption2 TLV(s)
            if (!MRP_PDU_sub_tlv_process(mrp_state, p, remaining_len, rx_pdu_info)) {
                return false;
            }

            p += remaining_len;
            break;

        default:
            T_EG(MRP_TRACE_GRP_FRAME_RX, "%u: %s was not handled", mrp_state->inst, mrp_pdu_tlv_type_to_str(tlv_type));
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
            return false;
        }

        // Length check
        if (p - frm > rx_length) {
            T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Got short frame. Expected at least %u bytes, but got only %u", mrp_state->inst, p - frm, rx_length);
            return false;
        }

        // The above might not consume all of the TLV, so adjust p to the next
        // TLV.
        p = tlv_start_p + tlv_len;

        if (done) {
            break;
        }
    }

    return true;
}

/******************************************************************************/
// MRP_PDU_tlv_header_add()
/******************************************************************************/
static uint32_t MRP_PDU_tlv_header_add(uint8_t *&p, mrp_pdu_tlv_type_t tlv_type, mrp_pdu_sub_tlv_type_t sub_tlv_type = MRP_PDU_SUB_TLV_TYPE_NONE)
{
    uint32_t padding;

    // Clause 8.1.8 says:
    // Bits 0-7 = Length, bits 8-15 = Type.
    // IEC 61158-6-10:2014, clause 4.2.1 says: Look in clause 3.4.2.3.4, which
    // says: Look in clause 3.4.3.2.2 Figure 15 & 16, which say:
    // Store bits 8-15 on octet #1 and bits 0-7 on octet #2.
    // Hence: TLVHeader.Type before TLVHeader.Length!
    *(p++) = MRP_PDU_tlv_type_to_id(tlv_type);
    *(p++) = MRP_PDU_tlv_len_get(tlv_type, &padding, sub_tlv_type) - 2 /* Exclude MRP_TLVHeader */;

    return padding;
}

/******************************************************************************/
// MRP_PDU_sub_tlv_header_add()
/******************************************************************************/
static uint32_t MRP_PDU_sub_tlv_header_add(uint8_t *&p, mrp_pdu_sub_tlv_type_t sub_tlv_type)
{
    uint32_t sub_padding;

    *(p++) = sub_tlv_type;
    *(p++) = MRP_PDU_sub_tlv_len_get(sub_tlv_type, &sub_padding) - 2 /* Exclude MRP_SubTLVHeader */;

    return sub_padding;
}

/******************************************************************************/
// MRP_PDU_tx_props_create()
/******************************************************************************/
static mesa_rc MRP_PDU_tx_props_create(mrp_state_t *mrp_state, packet_tx_props_t *tx_props, vtss_appl_iec_mrp_port_type_t port_type, uint32_t frm_len, bool interconnection)
{
    packet_tx_info_t      *packet_info;
    mesa_packet_tx_info_t *tx_info;
    uint32_t              normalized_frm_len;
    mesa_vid_t            vid = interconnection ? mrp_state->conf.in_vlan : mrp_state->conf.vlan;

    // Clean slate
    packet_tx_props_init(tx_props);

    normalized_frm_len = MAX(vid == 0 ? 60 : 64, frm_len);

    packet_info = &tx_props->packet_info;
    if ((packet_info->frm = packet_tx_alloc(normalized_frm_len)) == NULL) {
        T_EG(MRP_TRACE_GRP_FRAME_TX, "Out of memory");
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return VTSS_APPL_IEC_MRP_RC_OUT_OF_MEMORY;
    }

    packet_info->modid     = VTSS_MODULE_ID_IEC_MRP;
    packet_info->len       = normalized_frm_len;
    packet_info->no_free   = true;
    tx_info                = &tx_props->tx_info;
    tx_info->pdu_offset    = 14 + (vid == 0 ? 0 : 4);
    tx_info->dst_port_mask = VTSS_BIT64(mrp_state->port_states[port_type]->port_no);
    tx_info->pipeline_pt   = MESA_PACKET_PIPELINE_PT_REW_PORT_VOE;
    tx_info->tag.vid       = 0; // We insert the VLAN tag into the packet ourselves if needed.
    tx_info->tag.pcp       = 7;
    tx_info->tag.dei       = 0;
    tx_info->cos           = 7;
    tx_info->cosid         = 7;
    tx_info->dp            = 0;

    memset(packet_info->frm, 0, normalized_frm_len);

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_l2_create()
// Consists of:
//    DMAC, SMAC [, VLAN], EtherType, MRP_Version
/******************************************************************************/
static void MRP_PDU_l2_create(mrp_state_t *mrp_state, uint8_t *&p, vtss_appl_iec_mrp_port_type_t port_type, mrp_pdu_dmac_type_t dmac_type, bool interconnect)
{
    mrp_port_state_t *port_state = mrp_state->port_states[port_type];
    mesa_vid_t       vid = interconnect ? mrp_state->conf.in_vlan : mrp_state->conf.vlan;

    // DMAC (clause 8.1.4)
    memcpy(p, mrp_multicast_dmac.addr, sizeof(mrp_multicast_dmac.addr));

    // Change last byte according to type (MC_TEST = 1, MC_CONTROL = 2,
    // MC_INTEST = 3, MC_INCONTROL = 4).
    p[5] = dmac_type;
    p += sizeof(mrp_multicast_dmac.addr);

    // SMAC (Port MAC address; clause 8.1.3)
    memcpy(p, port_state->smac.addr, sizeof(port_state->smac.addr));
    p += sizeof(port_state->smac.addr);

    // Possible VLAN tag (clause 8.1.5)
    if (vid != 0) {
        // Insert TPID
        MRP_PDU_16bit_write(p, port_state->tpid);

        // Compute PCP, DEI (= 0), and VID
        *(p++) = (7 << 5) | ((vid >> 8) & 0x0F);
        *(p++) = vid & 0xFF;
    }

    // EtherType (clause 8.1.6)
    MRP_PDU_16bit_write(p, MRP_ETYPE);

    // MRP_Version (clause 8.1.11)
    MRP_PDU_16bit_write(p, MRP_PDU_VERSION);
}

/******************************************************************************/
// MRP_PDU_common_tlv_create()
// Constists of:
//   MRP_TLVHeader, MRP_SequenceID, MRP_DomainUUID
/******************************************************************************/
static void MRP_PDU_common_tlv_create(mrp_state_t *mrp_state, uint8_t *&p, uint8_t **sequence_id_ptr)
{
    uint32_t padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_COMMON);

    // MRP_SequenceID (clause 8.1.12)
    // We wait with filling this in until we actually transmit the PDU
    *sequence_id_ptr = p;
    p += 2;

    // MRP_DomainUUID (clause 8.1.26)
    memcpy(p, mrp_state->conf.domain_id, sizeof(mrp_state->conf.domain_id));
    p += sizeof(mrp_state->conf.domain_id);

    // Padding
    p += padding;
}

/******************************************************************************/
// MRP_PDU_option_tlv_create()
// Constists of:
//   MRP_TLVHeader, MRP_OUI, MRP_Ed1Type, MRP_Ed1ManufacturerData and
//   MRP_SubTLV.
/******************************************************************************/
static void MRP_PDU_option_tlv_create(mrp_state_t *mrp_state, uint8_t *&p, mrp_pdu_sub_tlv_type_t sub_tlv_type, uint8_t **other_prio_ptr = nullptr, uint8_t **other_mac_ptr = nullptr)
{
    uint32_t padding, sub_padding;

    // MRP_TLVHeader
    padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_OPTION, sub_tlv_type);

    // MRP_OUI.
    switch (mrp_state->conf.oui_type) {
    case VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT:
        // Use the OUI from the switch's own MAC address.
        *(p++) = MRP_chassis_mac.addr[0];
        *(p++) = MRP_chassis_mac.addr[1];
        *(p++) = MRP_chassis_mac.addr[2];
        break;

    case VTSS_APPL_IEC_MRP_OUI_TYPE_SIEMENS:
        // Use the Siemens OUI (08-00-06) in order to get Wireshark show
        // MRP_Option TLVs correctly.
        *(p++) = 0x08;
        *(p++) = 0x00;
        *(p++) = 0x06;
        break;

    case VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM:
    default:
        // Use a custom OUI.
        *(p++) = mrp_state->conf.custom_oui[0];
        *(p++) = mrp_state->conf.custom_oui[1];
        *(p++) = mrp_state->conf.custom_oui[2];
        break;
    }

    // MRP_Ed1Type
    p++;

    // MRP_Ed1ManufacturerData
    p += 2;

    // MRP_SubTLVHeader
    sub_padding = MRP_PDU_sub_tlv_header_add(p, sub_tlv_type);

    switch (sub_tlv_type) {
    case MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK:
    case MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE:
        // MRP_Prio (clause 8.1.15)
        MRP_PDU_16bit_write(p, mrp_state->conf.mrm.prio);

        // MRP_SA (clause 8.1.13)
        // Switch MAC address
        memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
        p += sizeof(MRP_chassis_mac.addr);

        // MRP_OtherMRMPrio (clause 8.1.16)
        if (other_prio_ptr) {
            // We wait with filling this out until we actually send the frame,
            // because it may vary.
            *other_prio_ptr = p;
        }

        p += 2;

        // MRP_OtherMRMSA (clause 8.1.14)
        if (other_mac_ptr) {
            // We wait with filling this out until we actually send the frame,
            // because it may vary.
            *other_mac_ptr = p;
        }

        p += sizeof(mesa_mac_t);
        break;

    case MRP_PDU_SUB_TLV_TYPE_AUTO_MGR:
        // No data.
        break;

    default:
        T_EG(MRP_TRACE_GRP_FRAME_TX, "Unsupported sub_tlv_type (%d)", sub_tlv_type);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        break;
    }

    p += padding + sub_padding;
}

/******************************************************************************/
// MRP_PDU_end_tlv_create()
// Constists of:
//   MRP_TLVHeader
/******************************************************************************/
static void MRP_PDU_end_tlv_create(mrp_state_t *mrp_state, uint8_t *&p)
{
    (void)MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_END);
}

/******************************************************************************/
// MRP_PDU_test_create()
// Corresponds to the standard's SetupTestRingReq().
// It merely creates a packet_tx_props_t template for transmitting MRP_Test
// PDUs.
/******************************************************************************/
static mesa_rc MRP_PDU_test_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps      = &mrp_state->ring_port_states[port_type];
    afi_multi_conf_t      *afi_conf = &rps->test_afi_conf;
    packet_tx_props_t     *tx_props = &afi_conf->tx_props[0];
    uint8_t               *p;
    bool                  include_option_tlv;
    uint32_t              padding;
    uint32_t              frm_len = MRP_PDU_l2_len_get(mrp_state, false)         +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_TEST)   +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON) +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);
    mesa_rc               rc;

    // Get rid of a possible old frame. It's safe to free the frame even if the
    // AFI is currently injecting, because the AFI module has taken a snapshot
    // of the old frame.
    MRP_PDU_tx_props_dispose(tx_props);

    if (MRP_can_use_afi) {
        if ((rc = afi_multi_conf_init(afi_conf)) != VTSS_RC_OK) {
            T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_multi_conf_init() failed: %s", mrp_state->inst, error_txt(rc));
            return rc;
        }

        // We compute it in bits per second (bps), data rate (L2), so we don't
        // include the 20 bytes inter-frame-gap and preample in our
        // calculations.
        afi_conf->params.line_rate = false;
    } else {
        vtss_clear(*afi_conf);
    }

    // We can no longer use the old pointer to &afi_conf->tx_props[0], because
    // that memory was freed with the call to afi_multi_conf_init() or
    // vtss_clear() above, because it contains a CapArray.
    tx_props = &afi_conf->tx_props[0];

    // If we are an MRA, we should include an MRP_Option TLV.
    include_option_tlv = mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRA;

    if (include_option_tlv) {
        // Extend the frame to make room for the MRP_Option TLV
        frm_len += MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_OPTION, nullptr, MRP_PDU_SUB_TLV_TYPE_AUTO_MGR);
    }

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, false));

    if (MRP_hw_support) {
        // We need to specify a particular OAM type in order to get the frames
        // updated dynamically by H/W.
        tx_props->tx_info.oam_type = MESA_PACKET_OAM_TYPE_MRP_TST;
    }

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_TEST, false);

    // Add the header
    padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_TEST);

    // MRP_Test consists of:
    //   MRP_TLVHeader, MRP_Prio, MRP_SA, MRP_PortRole, MRP_RingState,
    //   MRP_Transition, MRP_TimeStamp

    // MRP_Prio (clause 8.1.15)
    MRP_PDU_16bit_write(p, mrp_state->conf.mrm.prio);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_PortRole (clause 8.1.17)
    // We wait with filling this out until we actually send the frame, because
    // it may vary.
    rps->test_port_role_ptr = p;
    p += 2;

    // MRP_RingState (clause 8.1.18)
    // We wait with filling this out until we actually send the frame, because
    // it may vary.
    rps->test_ring_state_ptr = p;
    p += 2;

    // MRP_Transition (clause 8.1.20)
    rps->test_transition_ptr = p;
    p += 2;

    // MRP_Timestamp (clause 8.1.21)
    // We wait with updating the timestamp until we actually send the PDUs, but
    // we need to save the pointer for the Tx function to update.
    rps->test_timestamp_ptr = p;
    p += 4;

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->test_sequence_id_ptr);

    if (include_option_tlv) {
        // MRP_Option
        MRP_PDU_option_tlv_create(mrp_state, p, MRP_PDU_SUB_TLV_TYPE_AUTO_MGR);
    }

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_TEST, false));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_topology_change_create()
/******************************************************************************/
static mesa_rc MRP_PDU_topology_change_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps = &mrp_state->ring_port_states[port_type];
    packet_tx_props_t     *tx_props = &rps->topology_change_tx_props;
    uint8_t               *p;
    uint32_t              padding;
    uint32_t              frm_len = MRP_PDU_l2_len_get(mrp_state, false)                  +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE) +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON)          +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);

    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, false));

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_CONTROL, false);

    // MRP_TopologyChange consists of:
    // MRP_TLVHeader, MRP_Prio, MRP_SA, MRP_Interval
    padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE);

    // MRP_Prio (clause 8.1.15)
    MRP_PDU_16bit_write(p, mrp_state->conf.mrm.prio);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_Interval (clause 8.1.19)
    // We wait with updating the interval until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->topology_change_interval_ptr = p;
    p += 2;

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->topology_change_sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_TOPOLOGY_CHANGE, false));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_link_change_create()
// Corresponds to the standard's LinkChangeReq() function.
// It merely creates a packet_tx_props_t template for transmitting
// MRP_LinkChange PDUs.
/******************************************************************************/
static mesa_rc MRP_PDU_link_change_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps       = &mrp_state->ring_port_states[port_type];
    packet_tx_props_t      *tx_props = &rps->link_change_tx_props;
    uint8_t                *p;
    uint32_t               padding;
    mrp_pdu_tlv_type_t     tlv_type = MRP_PDU_TLV_TYPE_LINK_UP; // Doesn't matter, since both LINK_UP and LINK_DOWN have the same layout
    uint32_t               frm_len  = MRP_PDU_l2_len_get(mrp_state, false)         +
                                      MRP_PDU_tlv_len_get(tlv_type)                +
                                      MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON) +
                                      MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);

    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, false));

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_CONTROL, false);

    // Save a pointer to the TLV type for the Tx function to update
    rps->link_change_tlv_type_ptr = p;

    // MRP_LinkChange consists of:
    // MRP_TLVHeader, MRP_SA, MRP_PortRole, MRP_Interval, MRP_Blocked
    padding = MRP_PDU_tlv_header_add(p, tlv_type);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_PortRole (clause 8.1.17)
    // We wait with updating the port role until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->link_change_port_role_ptr = p;
    p += 2;

    // MRP_Interval (clause 8.1.19)
    // We wait with updating the interval until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->link_change_interval_ptr = p;
    p += 2;

    // MRP Blocked (clause 8.1.22).
    // We always support blocked state (no configuration parameter to control
    // this).
    MRP_PDU_16bit_write(p, 1);

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->link_change_sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP /* or DOWN */, false));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_option_create()
// It merely creates a packet_tx_props_t template for transmitting MRP_Option
// PDUs.
/******************************************************************************/
static mesa_rc MRP_PDU_option_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, mrp_pdu_sub_tlv_type_t sub_tlv_type)
{
    vtss_appl_iec_mrp_pdu_type_t pdu_type;
    mrp_ring_port_state_t        *rps = &mrp_state->ring_port_states[port_type];
    packet_tx_props_t            *tx_props;
    uint8_t                      *p, **other_prio_ptr, **other_mac_ptr, **sequence_id_ptr;
    uint32_t                     frm_len = MRP_PDU_l2_len_get(mrp_state, false)                                +
                                           MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_OPTION, nullptr, sub_tlv_type) +
                                           MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON)                        +
                                           MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);

    if (sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK) {
        tx_props        = &rps->test_mgr_nack_tx_props;
        other_prio_ptr  = &rps->test_mgr_nack_other_prio_ptr;
        other_mac_ptr   = &rps->test_mgr_nack_other_mac_ptr;
        sequence_id_ptr = &rps->test_mgr_nack_sequence_id_ptr;
        pdu_type        = VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_MGR_NACK;
    } else {
        tx_props        = &rps->test_propagate_tx_props;
        other_prio_ptr  = &rps->test_propagate_other_prio_ptr;
        other_mac_ptr   = &rps->test_propagate_other_mac_ptr;
        sequence_id_ptr = &rps->test_propagate_sequence_id_ptr;
        pdu_type        = VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_PROPAGATE;
    }

    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, false));

    p = tx_props->packet_info.frm;

    // The DMAC must be set to MC_TEST when sub_tlv_type is MRP_TestPropagate or
    // MRP_TestMgrNAck (see clause 5.10.3 and Table 19, NOTE 2).
    // Otherwise, it must be set to MC_CONTROL. In this function, we only handle
    // the first two SubTLVs, so use MC_TEST.
    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_TEST, false);

    // MRP_Option
    MRP_PDU_option_tlv_create(mrp_state, p, sub_tlv_type, other_prio_ptr, other_mac_ptr);

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, pdu_type, false));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_in_test_create()
// Corresponds to the standard's SetupInterconnTestReq().
// It merely creates a packet_tx_props_t template for transmitting MRP_InTest
// PDUs.
/******************************************************************************/
static mesa_rc MRP_PDU_in_test_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps      = &mrp_state->ring_port_states[port_type];
    afi_single_conf_t     *afi_conf = &rps->in_test_afi_conf;
    packet_tx_props_t     *tx_props = &afi_conf->tx_props;
    uint8_t               *p;
    uint32_t              padding;
    uint32_t              frm_len = MRP_PDU_l2_len_get(mrp_state, true)           +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_IN_TEST) +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON)  +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);
    mesa_rc               rc;

    // Get rid of a possible old frame. It's safe to free the frame even if the
    // AFI is currently injecting, because the AFI module has taken a snapshot
    // of the old frame.
    MRP_PDU_tx_props_dispose(tx_props);

    if (MRP_can_use_afi) {
        if ((rc = afi_single_conf_init(afi_conf)) != VTSS_RC_OK) {
            T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_single_conf_init() failed: %s", mrp_state->inst, error_txt(rc));
            return rc;
        }

        // Whenever we start transmission, do it ASAP the first time.
        afi_conf->params.first_frame_urgent = true;
    } else {
        vtss_clear(*afi_conf);
    }

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, true));

    if (MRP_hw_support) {
        // We need to specify a particular OAM type in order to get the frames
        // updated dynamically by H/W.
        tx_props->tx_info.oam_type = MESA_PACKET_OAM_TYPE_MRP_ITST;
    }

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_INTEST, true);

    // MRP_InTest consists of:
    //   MRP_TLVHeader, MRP_InID, MRP_SA, MRP_PortRole, MRP_InState,
    //   MRP_Transition, MRP_TimeStamp
    padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_IN_TEST);

    // MRP_InID (clause 8.1.28)
    MRP_PDU_16bit_write(p, mrp_state->conf.in_id);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_PortRole (clause 8.1.17)
    // We wait with updating the port role until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->in_test_port_role_ptr = p;
    p += 2;

    // MRP_InState (clause 8.1.27)
    // We wait with updating the interconnection ring state until we actually
    // send the PDU, but we need to save the pointer for the Tx function to
    // update.
    rps->in_test_in_state_ptr = p;
    p += 2;

    // MRP_Transition (clause 8.1.20)
    // We wait with updating the interconnection transitions until we actually
    // send the PDU, but we need to save the pointer for the Tx function to
    // update.
    rps->in_test_transition_ptr = p;
    p += 2;

    // MRP_Timestamp (clause 8.1.21)
    // We wait with updating the timestamp until we actually send the PDUs, but
    // we need to save the pointer for the Tx function to update.
    rps->in_test_timestamp_ptr = p;
    p += 4;

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->in_test_sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST, true));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_in_topology_change_create()
/******************************************************************************/
static mesa_rc MRP_PDU_in_topology_change_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps      = &mrp_state->ring_port_states[port_type];
    packet_tx_props_t     *tx_props = &rps->in_topology_change_tx_props;
    uint8_t               *p;
    uint32_t              padding;
    uint32_t              frm_len = MRP_PDU_l2_len_get(mrp_state, true)                      +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE) +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON)             +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);

    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, true));

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_INCONTROL, true);

    // MRP_InTopologyChange consists of:
    // MRP_TLVHeader, MRP_SA, MRP_InID, MRP_Interval
    padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_InID (clause 8.1.28)
    MRP_PDU_16bit_write(p, mrp_state->conf.in_id);

    // MRP_Interval (clause 8.1.19)
    // We wait with updating the interval until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->in_topology_change_interval_ptr = p;
    p += 2;

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->in_topology_change_sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TOPOLOGY_CHANGE, true));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_in_link_change_create()
/******************************************************************************/
static mesa_rc MRP_PDU_in_link_change_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps      = &mrp_state->ring_port_states[port_type];
    packet_tx_props_t     *tx_props = &rps->in_link_change_tx_props;
    uint8_t               *p;
    uint32_t              padding;
    mrp_pdu_tlv_type_t    tlv_type = MRP_PDU_TLV_TYPE_IN_LINK_UP; // Doesn't matter, since both IN_LINK_UP and IN_LINK_DOWN have the same layout
    uint32_t              frm_len  = MRP_PDU_l2_len_get(mrp_state, true)          +
                                     MRP_PDU_tlv_len_get(tlv_type)                +
                                     MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON) +
                                     MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);
    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, true));

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_INCONTROL, true);

    // Save a pointer to the TLV type for the Tx function to update
    rps->in_link_change_tlv_type_ptr = p;

    // MRP_InLinkChange consists of:
    // MRP_TLVHeader, MRP_SA, MRP_PortRole, MRP_InID, MRP_Interval
    padding = MRP_PDU_tlv_header_add(p, tlv_type);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_PortRole (clause 8.1.17)
    // We wait with updating the port role until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->in_link_change_port_role_ptr = p;
    p += 2;

    // MRP_InID (clause 8.1.28)
    MRP_PDU_16bit_write(p, mrp_state->conf.in_id);

    // MRP_Interval (clause 8.1.19)
    // We wait with updating the interval until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->in_link_change_interval_ptr = p;
    p += 2;

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->in_link_change_sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP /* or DOWN */, true));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_in_link_status_poll_create()
/******************************************************************************/
static mesa_rc MRP_PDU_in_link_status_poll_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    mrp_ring_port_state_t *rps      = &mrp_state->ring_port_states[port_type];
    packet_tx_props_t     *tx_props = &rps->in_link_status_poll_tx_props;
    uint8_t               *p;
    uint32_t              padding;
    uint32_t              frm_len = MRP_PDU_l2_len_get(mrp_state, true)                       +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL) +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_COMMON)              +
                                    MRP_PDU_tlv_len_get(MRP_PDU_TLV_TYPE_END);

    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, frm_len, true));

    p = tx_props->packet_info.frm;

    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_INCONTROL, true);

    // MRP_InLinkStatusPoll consists of:
    // MRP_TLVHeader, MRP_SA, MRP_PortRole, MRP_InID
    padding = MRP_PDU_tlv_header_add(p, MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL);

    // MRP_SA (clause 8.1.13)
    // Switch MAC address
    memcpy(p, MRP_chassis_mac.addr, sizeof(MRP_chassis_mac.addr));
    p += sizeof(MRP_chassis_mac.addr);

    // MRP_PortRole (clause 8.1.17)
    // We wait with updating the port role until we actually send the PDU, but
    // we need to save the pointer for the Tx function to update.
    rps->in_link_status_poll_port_role_ptr = p;
    p += 2;

    // MRP_InID (clause 8.1.28)
    MRP_PDU_16bit_write(p, mrp_state->conf.in_id);

    // Padding
    p += padding;

    // MRP_Common
    MRP_PDU_common_tlv_create(mrp_state, p, &rps->in_link_status_poll_sequence_id_ptr);

    // MRP_End
    MRP_PDU_end_tlv_create(mrp_state, p);

    VTSS_RC(MRP_PDU_len_check(mrp_state, tx_props, p, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_STATUS_POLL, true));

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_sw_fwd_create()
/******************************************************************************/
static mesa_rc MRP_PDU_sw_fwd_create(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    packet_tx_props_t *tx_props = &mrp_state->ring_port_states[port_type].fwd_tx_props;
    uint8_t           *p;

    // Get rid of a possible old frame
    MRP_PDU_tx_props_dispose(tx_props);

    VTSS_RC(MRP_PDU_tx_props_create(mrp_state, tx_props, port_type, 300 /* long enough to hold any MRP_InXXX frame */, true /* these frames are always MRP_InXXX PDUs, so tag with interconnection VLAN ID */));

    // The only reason for calling this one is to get the VLAN tag filled in.
    // The remainder will be overwritten once a PDU is forwarded.
    p = tx_props->packet_info.frm;
    MRP_PDU_l2_create(mrp_state, p, port_type, MRP_PDU_DMAC_TYPE_MC_INCONTROL, true);

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_PDU_test_tx()
// Invoked when the test_tx_timer expires. Only used when not using the AFI to
// transmit MRP_Test PDUs.
// On these platforms, we can update timestamp and sequence number on the fly.
/******************************************************************************/
static void MRP_PDU_test_tx(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    uint64_t                      now_msecs = vtss::uptime_milliseconds();
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;

    // When using the AFI, we cannot update MRP_timestamp and MRP_sequenceID on
    // the fly, so those two fields only change when the MRP_Test PDU changes.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        // Update MRP_Timestamp (clause 8.1.21)
        MRP_PDU_32bit_write(rps->test_timestamp_ptr, (uint32_t)now_msecs, false /* Don't advance ptr */);

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(rps->test_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_TEST]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, &rps->test_afi_conf.tx_props[0], port_type, VTSS_APPL_IEC_MRP_PDU_TYPE_TEST);
    }
}

/******************************************************************************/
// MRP_PDU_test_tx_start()
/******************************************************************************/
static void MRP_PDU_test_tx_start(mrp_state_t *mrp_state)
{
    uint64_t                      now_msecs = vtss::uptime_milliseconds(), fps_mul1000;
    uint32_t                      frm_sz_bytes;
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;
    mesa_rc                       rc;

    // When using the AFI, we cannot update MRP_timestamp and MRP_sequenceID on
    // the fly, so those two fields only change when the MRP_Test PDU changes.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        if (!rps->test_afi_conf.tx_props[0].packet_info.frm) {
            T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: Invalid MRP_Test PDU for %s", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type));
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
            continue;
        }

        // Update MRP_PortRole (clause 8.1.17)
        MRP_PDU_16bit_write(rps->test_port_role_ptr, MRP_PDU_port_role_to_bin(mrp_state, port_type), false /* Don't advance ptr */);

        // Update MRP_RingState (clause 8.1.18)
        MRP_PDU_16bit_write(rps->test_ring_state_ptr, mrp_state->status.ring_state == VTSS_APPL_IEC_MRP_RING_STATE_OPEN ? 0 : 1, false /* Don't advance ptr */);

        // Update MRP_Transition (clause 8.1.20).
        MRP_PDU_16bit_write(rps->test_transition_ptr, mrp_state->status.transitions, false /* Don't advance ptr */);

        if (MRP_can_use_afi) {
            // Update MRP_Timestamp (clause 8.1.21)
            MRP_PDU_32bit_write(rps->test_timestamp_ptr, (uint32_t)now_msecs, false /* Don't advance ptr */);

            // Update MRP_SequenceID (clause 8.1.12)
            MRP_PDU_16bit_write(rps->test_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_TEST]++, false /* Don't advance ptr */);
        } else {
            // When injecting the frames manually, MRP_Timestamp and
            // MRP_SequenceID are updated by MRP_PDU_test_tx() when the timer
            // expires.
        }

        if (MRP_can_use_afi) {
            // Create the flow on the port. It doesn't really matter whether the
            // port currently has link or not.

            // We need to use multi AFI flows, because the rate may be higher
            // than what single AFI flows support.

            // First figure out the number of frames per second, multiplied by
            // 1000 in order not to lose too many decimals.
            fps_mul1000 = 1000000000 / mrp_state->vars.test_tx_interval_us;

            // Convert to bps.
            frm_sz_bytes                  = rps->test_afi_conf.tx_props[0].packet_info.len + 4 /* FCS */;
            rps->test_afi_conf.params.bps = (fps_mul1000 * frm_sz_bytes * 8) / 1000;

            if ((rc = afi_multi_alloc(&rps->test_afi_conf, &rps->test_afi_id)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_multi_alloc(sz = %u => bps = " VPRI64u ") failed: %s", mrp_state->inst, frm_sz_bytes, rps->test_afi_conf.params.bps, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
                continue;
            }

            if ((rc = afi_multi_pause_resume(rps->test_afi_id, FALSE)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_multi_pause_resume(%u) failed: %s", mrp_state->inst, rps->test_afi_id, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
                continue;
            }

            T_IG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_multi_alloc & resume %s (sz = %u => bps = " VPRI64u ") => AFI ID = %u", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type), frm_sz_bytes, rps->test_afi_conf.params.bps, rps->test_afi_id);
        }
    }

    if (!MRP_can_use_afi) {
        // Start a repating timer and send the frames right away
        mrp_timer_start(mrp_state->test_tx_timer, mrp_state->vars.test_tx_interval_us, true /* Repeating */);
        MRP_PDU_test_tx(mrp_state->test_tx_timer, mrp_state);
    }
}

/******************************************************************************/
// MRP_PDU_test_tx_stop()
/******************************************************************************/
static void MRP_PDU_test_tx_stop(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;
    mesa_rc                       rc;

    // Stop our Tx timer (in case of manual injection)
    mrp_timer_stop(mrp_state->test_tx_timer);

    // Stop AFI (in case of AFI-supported injection)
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        if (rps->test_afi_id != AFI_ID_NONE) {
            T_IG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_multi_free(%u)", mrp_state->inst, rps->test_afi_id);
            if ((rc = afi_multi_free(rps->test_afi_id, nullptr)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_multi_free(%u) failed: %s", mrp_state->inst, rps->test_afi_id, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
            }
        }

        rps->test_afi_id = AFI_ID_NONE;
    }
}

/******************************************************************************/
// MRP_PDU_in_test_tx_on_port()
/******************************************************************************/
static bool MRP_PDU_in_test_tx_on_port(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    // The following table outlines how we start Tx of MRP_InTest, given we are
    // a MIM in RC mode.
    // +----------------------------------------------------------------+
    // | Oper Role || Tx on                                             |
    // |-----------||---------------------------------------------------|
    // | MRC       || I/C and both ring ports                           |
    // | MRM       || Always on I/C port. Only on forwarding ring ports |
    // +----------------------------------------------------------------+

    if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRC) {
        // MRCs transmit on all three ports
        return true;
    }

    // MRMs transmit on I/C port always
    if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) {
        return true;
    }

    // MRMs transmit only on forwarding ring ports
    return mrp_state->status.port_status[port_type].forwarding;
}

/******************************************************************************/
// MRP_PDU_in_test_tx()
// Invoked when the in_test_tx_timer expires. Only used when not using the AFI
// to transmit MRP_Test PDUs.
// On these platforms, we can update timestamp and sequence number on the fly.
/******************************************************************************/
static void MRP_PDU_in_test_tx(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    uint64_t                      now_msecs = vtss::uptime_milliseconds();
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;

    // When using the AFI, we cannot update MRP_timestamp and MRP_sequenceID on
    // the fly, so those two fields only change when the MRP_InTest PDU changes.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {

        if (!MRP_PDU_in_test_tx_on_port(mrp_state, port_type)) {
            continue;
        }

        rps = &mrp_state->ring_port_states[port_type];

        // Update MRP_Timestamp (clause 8.1.21)
        MRP_PDU_32bit_write(rps->in_test_timestamp_ptr, (uint32_t)now_msecs, false /* Don't advance ptr */);

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(rps->in_test_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_IN_TEST]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, &rps->in_test_afi_conf.tx_props, port_type, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST);
    }
}

/******************************************************************************/
// MRP_PDU_in_test_tx_start()
/******************************************************************************/
static void MRP_PDU_in_test_tx_start(mrp_state_t *mrp_state)
{
    uint64_t                      now_msecs = vtss::uptime_milliseconds();
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;
    mesa_rc                       rc;

    // When using the AFI, we cannot update MRP_timestamp and MRP_sequenceID on
    // the fly, so those two fields only change when the MRP_InTest PDU changes.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        if (!MRP_PDU_in_test_tx_on_port(mrp_state, port_type)) {
            continue;
        }

        rps = &mrp_state->ring_port_states[port_type];

        if (!rps->in_test_afi_conf.tx_props.packet_info.frm) {
            T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: Invalid MRP_InTest PDU for %s", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type));
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
            continue;
        }

        // Update MRP_PortRole (clause 8.1.17)
        MRP_PDU_16bit_write(rps->in_test_port_role_ptr, MRP_PDU_port_role_to_bin(mrp_state, port_type), false /* Don't advance ptr */);

        // Update MRP_InState (clause 8.1.27)
        MRP_PDU_16bit_write(rps->in_test_in_state_ptr, mrp_state->status.in_ring_state == VTSS_APPL_IEC_MRP_RING_STATE_OPEN ? 0 : 1, false /* Don't advance ptr */);

        // MRP_Transition (clause 8.1.20)
        MRP_PDU_16bit_write(rps->in_test_transition_ptr, mrp_state->status.in_transitions, false /* Don't advance ptr */);

        if (MRP_can_use_afi) {
            // MRP_Timestamp (clause 8.1.21)
            MRP_PDU_32bit_write(rps->in_test_timestamp_ptr, (uint32_t)now_msecs, false /* Don't advance ptr */);

            // Update MRP_SequenceID (clause 8.1.12)
            MRP_PDU_16bit_write(rps->in_test_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_IN_TEST]++, false /* Don't advance ptr */);
        } else {
            // When injecting the frames manually, MRP_Timestamp and
            // MRP_SequenceID are updated by MRP_PDU_in_test_tx() when the timer
            // expires.
        }

        if (MRP_can_use_afi) {
            // Create the flow on the port. It doesn't really matter whether the
            // port currently has link or not.

            // Compute frames per hour.
            // RBNTBD: Probably need to use multi AFI flows. The following may
            // become zero.
            rps->in_test_afi_conf.params.fph = 3600000 / (mrp_state->vars.in_test_tx_interval_us / 1000);

            if ((rc = afi_single_alloc(&rps->in_test_afi_conf, &rps->in_test_afi_id)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_single_alloc(" VPRI64u ") failed: %s", mrp_state->inst, rps->in_test_afi_conf.params.fph, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
                continue;
            }

            if ((rc = afi_single_pause_resume(rps->in_test_afi_id, FALSE)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_single_pause_resume(%u) failed: %s", mrp_state->inst, rps->in_test_afi_id, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
                continue;
            }

            T_IG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_single_alloc & resume %s => AFI ID = %u", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type), rps->in_test_afi_id);
        }
    }

    if (!MRP_can_use_afi) {
        // Start a repating timer and send the frames right away
        mrp_timer_start(mrp_state->in_test_tx_timer, mrp_state->vars.in_test_tx_interval_us, true /* Repeating */);
        MRP_PDU_in_test_tx(mrp_state->in_test_tx_timer, mrp_state);
    }
}

/******************************************************************************/
// MRP_PDU_in_test_tx_stop()
/******************************************************************************/
static void MRP_PDU_in_test_tx_stop(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;
    mesa_rc                       rc;

    // Stop our Tx timer (in case of manual injection)
    mrp_timer_stop(mrp_state->in_test_tx_timer);

    // Stop AFI (in case of AFI-supported injection)
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        if (rps->in_test_afi_id != AFI_ID_NONE) {
            T_IG(MRP_TRACE_GRP_FRAME_TX, "%u: afi_single_free(%u)", mrp_state->inst, rps->in_test_afi_id);
            if ((rc = afi_single_free(rps->in_test_afi_id, nullptr)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: Unable to free AFI instance %u: %s", mrp_state->inst, rps->in_test_afi_id, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
            }
        }

        rps->in_test_afi_id = AFI_ID_NONE;
    }
}

/******************************************************************************/
// MRP_PDU_test_tx_update_hw_support()
/******************************************************************************/
void MRP_PDU_test_tx_update_hw_support(mrp_state_t *mrp_state, bool tx, uint32_t usec)
{
    mesa_mrp_ring_state_t mesa_ring_state;
    mesa_rc               rc;

    if (!tx || usec != mrp_state->vars.test_tx_interval_us) {
        // If we are asked to stop transmission or the rate changes, stop.
        MRP_PDU_test_tx_stop(mrp_state);
        mrp_state->vars.test_tx_in_progress = false;

        if (!tx) {
            // Nothing more to do if asked to stop transmission.
            return;
        }

        mrp_state->vars.test_tx_interval_us = usec;
    }

    // We are already transmitting or will soon do so.
    if (mrp_state->vars.prm_ring_port != mrp_state->vars.test_tx_prm_ring_port) {
        // Update MRP_PortRole (clause 8.1.17) in H/W.
        // This has already happened in
        // iec_mrp_base.cxx#MRP_BASE_mesa_ring_ports_swap()
        mrp_state->vars.test_tx_prm_ring_port = mrp_state->vars.prm_ring_port;
    }

    if (mrp_state->status.ring_state != mrp_state->vars.test_tx_ring_state) {
        // Update MRP_RingState(clause 8.1.18) and MRP_Transition (clause
        // 8.1.20). The latter is maintained by the API based on changes to the
        // ring state.
        mesa_ring_state = mrp_state->status.ring_state == VTSS_APPL_IEC_MRP_RING_STATE_CLOSED ? MESA_MRP_RING_STATE_CLOSED : MESA_MRP_RING_STATE_OPEN;
        T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_ring_state_set(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), mesa_ring_state);
        if ((rc = mesa_mrp_ring_state_set(nullptr, mrp_state->mrp_idx(), mesa_ring_state)) != VTSS_RC_OK) {
            T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_ring_state_set(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), mesa_ring_state, error_txt(rc));

            // Convert to something sensible
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        }

        mrp_state->vars.test_tx_ring_state  = mrp_state->status.ring_state;

        // There is no guarantee that MRP_Transitions as calculated by the API
        // is identical to that calculated by us. We live with that.
        mrp_state->vars.test_tx_transitions = mrp_state->status.transitions;
    }

    // MRP_Timestamp (clause 8.1.21) and MRP_SequenceID (clause 8.1.12) are
    // automatically updated by H/W.

    if (!mrp_state->vars.test_tx_in_progress) {
        // Time to start
        MRP_PDU_test_tx_start(mrp_state);
        mrp_state->vars.test_tx_in_progress = true;
    }
}

/******************************************************************************/
// MRP_PDU_test_tx_update_no_hw_support()
/******************************************************************************/
void MRP_PDU_test_tx_update_no_hw_support(mrp_state_t *mrp_state, bool tx, uint32_t usec)
{
    bool update_required = false;

    if (mrp_state->vars.test_tx_in_progress) {
        update_required = mrp_state->vars.prm_ring_port != mrp_state->vars.test_tx_prm_ring_port ||
                          mrp_state->status.ring_state  != mrp_state->vars.test_tx_ring_state    ||
                          mrp_state->status.transitions != mrp_state->vars.test_tx_transitions   ||
                          usec                          != mrp_state->vars.test_tx_interval_us;

        if (!tx || update_required) {
            MRP_PDU_test_tx_stop(mrp_state);
            mrp_state->vars.test_tx_in_progress = false;
        }
    }

    if (tx && !mrp_state->vars.test_tx_in_progress) {
        mrp_state->vars.test_tx_prm_ring_port = mrp_state->vars.prm_ring_port;
        mrp_state->vars.test_tx_ring_state    = mrp_state->status.ring_state;
        mrp_state->vars.test_tx_transitions   = mrp_state->status.transitions;
        mrp_state->vars.test_tx_interval_us   = usec;
        MRP_PDU_test_tx_start(mrp_state);
        mrp_state->vars.test_tx_in_progress = true;
    }
}

/******************************************************************************/
// mrp_pdu_test_tx_update()
// Checks if we should start or stop MRP_Test PDUs from being generated.
// May also update the contents of the PDU.
/******************************************************************************/
void mrp_pdu_test_tx_update(mrp_state_t *mrp_state, bool tx, uint32_t usec)
{
    if (MRP_hw_support) {
        MRP_PDU_test_tx_update_hw_support(mrp_state, tx, usec);
    } else {
        MRP_PDU_test_tx_update_no_hw_support(mrp_state, tx, usec);
    }
}

/******************************************************************************/
// mrp_pdu_topology_change_tx()
// Corresponds to SetupTopologyChangeReq() on p 102.
/******************************************************************************/
void mrp_pdu_topology_change_tx(mrp_state_t *mrp_state, uint16_t mrp_interval_msec)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        // Update MRP_Interval (clause 8.1.19)
        MRP_PDU_16bit_write(rps->topology_change_interval_ptr, mrp_interval_msec, false /* Don't advance ptr */);

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(rps->topology_change_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, &rps->topology_change_tx_props, port_type, VTSS_APPL_IEC_MRP_PDU_TYPE_TOPOLOGY_CHANGE);
    }
}

/******************************************************************************/
// mrp_pdu_link_change_tx()
// Corresponds to the standard's LinkChangeReq() function.
// Transmits a pre-created MRP_LinkChange PDU
/******************************************************************************/
void mrp_pdu_link_change_tx(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool link_up)
{
    uint16_t              mrp_interval_msec;
    mrp_ring_port_state_t *rps     = &mrp_state->ring_port_states[port_type];
    mrp_pdu_tlv_type_t    tlv_type = link_up ? MRP_PDU_TLV_TYPE_LINK_UP : MRP_PDU_TLV_TYPE_LINK_DOWN;

    // Update MRP_TLVHeader.Type
    *rps->link_change_tlv_type_ptr = MRP_PDU_tlv_type_to_id(tlv_type);

    // Update MRP_PortRole (clause 8.1.17)
    MRP_PDU_16bit_write(rps->link_change_port_role_ptr, MRP_PDU_port_role_to_bin(mrp_state, port_type), false /* Don't advance ptr */);

    // Update MRP_Interval (clause 8.1.19)
    // Always MRP_LNKNReturn * MRP_LNKupT or MRP_LNKNReturn * MRP_LNKdownT.
    mrp_interval_msec = mrp_state->vars.MRP_LNKNReturn * ((link_up ? mrp_state->mrc_timing.link_up_interval_usec : mrp_state->mrc_timing.link_down_interval_usec) / 1000);
    MRP_PDU_16bit_write(rps->link_change_interval_ptr, mrp_interval_msec, false /* Don't advance ptr */);

    // Update MRP_SequenceID (clause 8.1.12)
    MRP_PDU_16bit_write(rps->link_change_sequence_id_ptr, rps->sequence_id[tlv_type]++, false /* Don't advance ptr */);

    // Transmit it (it won't get freed).
    MRP_PDU_tx(mrp_state, &rps->link_change_tx_props, port_type, link_up ? VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP : VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_DOWN);
}

/******************************************************************************/
// mrp_pdu_option_tx()
// Corresponds to TestMgrNAckReq() on p 103 and TestPropagateReq() on p. 104.
/******************************************************************************/
void mrp_pdu_option_tx(mrp_state_t *mrp_state, mrp_pdu_sub_tlv_type_t sub_tlv_type, uint16_t other_prio, mesa_mac_t *other_mac)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    vtss_appl_iec_mrp_pdu_type_t  pdu_type;
    mrp_ring_port_state_t         *rps;
    packet_tx_props_t             *tx_props;
    uint8_t                       *other_prio_ptr, *other_mac_ptr, *sequence_id_ptr;

    if (sub_tlv_type != MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK && sub_tlv_type != MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE) {
        T_EG(MRP_TRACE_GRP_FRAME_TX, "%u: No pre-created PDU available for sub_tlv_type = %d", mrp_state->inst, sub_tlv_type);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return;
    }

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        if (sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK) {
            tx_props        = &rps->test_mgr_nack_tx_props;
            other_prio_ptr  =  rps->test_mgr_nack_other_prio_ptr;
            other_mac_ptr   =  rps->test_mgr_nack_other_mac_ptr;
            sequence_id_ptr =  rps->test_mgr_nack_sequence_id_ptr;
            pdu_type        = VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_MGR_NACK;
        } else {
            tx_props        = &rps->test_propagate_tx_props;
            other_prio_ptr  =  rps->test_propagate_other_prio_ptr;
            other_mac_ptr   =  rps->test_propagate_other_mac_ptr;
            sequence_id_ptr =  rps->test_propagate_sequence_id_ptr;
            pdu_type        =  VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_PROPAGATE;
        }

        // Update MRP_OtherMRMPrio (clause 8.1.16)
        MRP_PDU_16bit_write(other_prio_ptr, other_prio, false /* Don't advance ptr */);

        // Update MRP_OtherMRMSA (clause 8.1.14)
        // If sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK:
        //   Interface-MAC address of the lower priority manager whose test
        //   frame was received previously
        // If sub_tlv_type == MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE:
        //   Interface-MAC address of the higher priority manager whose test
        //   frame was received previously
        memcpy(other_mac_ptr, other_mac->addr, sizeof(other_mac->addr));

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_OPTION]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, tx_props, port_type, pdu_type);
    }
}

/******************************************************************************/
// MRP_PDU_in_test_tx_update_hw_support()
/******************************************************************************/
void MRP_PDU_in_test_tx_update_hw_support(mrp_state_t *mrp_state, bool tx, uint32_t usec)
{
    mesa_mrp_ring_state_t mesa_in_ring_state;
    mesa_rc               rc;

    if (!tx || usec != mrp_state->vars.in_test_tx_interval_us) {
        // If we are asked to stop transmission or the rate changes, stop.
        MRP_PDU_in_test_tx_stop(mrp_state);
        mrp_state->vars.in_test_tx_in_progress = false;

        if (!tx) {
            // Nothing more to do if asked to stop transmission.
            return;
        }

        mrp_state->vars.in_test_tx_interval_us = usec;
    }

    // We are already transmitting or will soon do so.
    if (mrp_state->vars.prm_ring_port != mrp_state->vars.in_test_tx_prm_ring_port) {
        // Update MRP_PortRole (clause 8.1.17) in H/W.
        // This has already happened in
        // iec_mrp_base.cxx#MRP_BASE_mesa_ring_ports_swap()
        // The port role of the interconnection port never changes.
        mrp_state->vars.in_test_tx_prm_ring_port = mrp_state->vars.prm_ring_port;
    }

    if (mrp_state->status.in_ring_state != mrp_state->vars.in_test_tx_ring_state) {
        // Update MRP_RingState(clause 8.1.18) and MRP_Transition (clause
        // 8.1.20). The latter is maintained by the API based on changes to the
        // ring state.
        mesa_in_ring_state = mrp_state->status.in_ring_state == VTSS_APPL_IEC_MRP_RING_STATE_CLOSED ? MESA_MRP_RING_STATE_CLOSED : MESA_MRP_RING_STATE_OPEN;
        T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_in_ring_state_set(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), mesa_in_ring_state);
        if ((rc = mesa_mrp_in_ring_state_set(nullptr, mrp_state->mrp_idx(), mesa_in_ring_state)) != VTSS_RC_OK) {
            T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_in_ring_state_set(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), mesa_in_ring_state, error_txt(rc));

            // Convert to something sensible
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        }

        mrp_state->vars.in_test_tx_ring_state  = mrp_state->status.in_ring_state;

        // There is no guarantee that MRP_Transitions as calculated by the API
        // is identical to that calculated by us. We live with that.
        mrp_state->vars.in_test_tx_transitions = mrp_state->status.in_transitions;
    }

    // MRP_Timestamp (clause 8.1.21) and MRP_SequenceID (clause 8.1.12) are
    // automatically updated by H/W.

    if (!mrp_state->vars.in_test_tx_in_progress) {
        // Time to start
        MRP_PDU_in_test_tx_start(mrp_state);
        mrp_state->vars.in_test_tx_in_progress = true;
    }
}

/******************************************************************************/
// MRP_PDU_in_test_tx_update_no_hw_support()
/******************************************************************************/
void MRP_PDU_in_test_tx_update_no_hw_support(mrp_state_t *mrp_state, bool tx, uint32_t usec)
{
    bool update_required = false;

    if (mrp_state->vars.in_test_tx_in_progress) {
        update_required = mrp_state->vars.prm_ring_port    != mrp_state->vars.in_test_tx_prm_ring_port ||
                          mrp_state->status.in_ring_state  != mrp_state->vars.in_test_tx_ring_state    ||
                          mrp_state->status.in_transitions != mrp_state->vars.in_test_tx_transitions   ||
                          usec                             != mrp_state->vars.in_test_tx_interval_us;

        if (!tx || update_required) {
            MRP_PDU_in_test_tx_stop(mrp_state);
            mrp_state->vars.in_test_tx_in_progress = false;
        }
    }

    if (tx && !mrp_state->vars.in_test_tx_in_progress) {
        mrp_state->vars.in_test_tx_prm_ring_port = mrp_state->vars.prm_ring_port;
        mrp_state->vars.in_test_tx_ring_state    = mrp_state->status.in_ring_state;
        mrp_state->vars.in_test_tx_transitions   = mrp_state->status.in_transitions;
        mrp_state->vars.in_test_tx_interval_us   = usec;
        MRP_PDU_in_test_tx_start(mrp_state);
        mrp_state->vars.in_test_tx_in_progress = true;
    }
}

/******************************************************************************/
// mrp_pdu_in_test_tx_update()
// Checks if we should start or stop MRP_InTest PDUs from being generated.
// May also update the contents of the PDU.
/******************************************************************************/
void mrp_pdu_in_test_tx_update(mrp_state_t *mrp_state, bool tx, uint32_t usec)
{
    if (MRP_hw_support) {
        MRP_PDU_in_test_tx_update_hw_support(mrp_state, tx, usec);
    } else {
        MRP_PDU_in_test_tx_update_no_hw_support(mrp_state, tx, usec);
    }
}

/******************************************************************************/
// mrp_pdu_in_topology_change_tx()
// Corresponds to SetupInterconnTopologyChangeReq() on p. 125.
/******************************************************************************/
void mrp_pdu_in_topology_change_tx(mrp_state_t *mrp_state, uint16_t mrp_interval_msec)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        // Update MRP_Interval (clause 8.1.19)
        MRP_PDU_16bit_write(rps->in_topology_change_interval_ptr, mrp_interval_msec, false /* Don't advance ptr */);

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(rps->in_topology_change_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, &rps->in_topology_change_tx_props, port_type, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TOPOLOGY_CHANGE);
    }
}

/******************************************************************************/
// mrp_pdu_in_link_change_tx()
// Corresponds to InterconnLinkChangeReq() on p. 125.
/******************************************************************************/
void mrp_pdu_in_link_change_tx(mrp_state_t *mrp_state, bool link_up, uint16_t mrp_interval_msec)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;
    mrp_pdu_tlv_type_t            tlv_type = link_up ? MRP_PDU_TLV_TYPE_IN_LINK_UP : MRP_PDU_TLV_TYPE_IN_LINK_DOWN;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        // Update MRP_TLVHeader.Type
        *rps->in_link_change_tlv_type_ptr = MRP_PDU_tlv_type_to_id(tlv_type);

        // Update MRP_PortRole (clause 8.1.17)
        MRP_PDU_16bit_write(rps->in_link_change_port_role_ptr, MRP_PDU_port_role_to_bin(mrp_state, port_type), false /* Don't advance ptr */);

        // Update MRP_Interval (clause 8.1.19)
        MRP_PDU_16bit_write(rps->in_link_change_interval_ptr, mrp_interval_msec, false /* Don't advance ptr */);

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(rps->in_link_change_sequence_id_ptr, rps->sequence_id[tlv_type]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, &rps->in_link_change_tx_props, port_type, link_up ? VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP : VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_DOWN);
    }
}

/******************************************************************************/
// mrp_pdu_in_link_status_poll_tx()
// Corresponds to SetupInterconnLinkStatusPollReq() on p. 126.
/******************************************************************************/
void mrp_pdu_in_link_status_poll_tx(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;

    // MRP_InLinkStatusPoll PDUs are only transmitted on ring ports.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];

        // Update MRP_PortRole (clause 8.1.17)
        MRP_PDU_16bit_write(rps->in_link_status_poll_port_role_ptr, MRP_PDU_port_role_to_bin(mrp_state, port_type), false /* Don't advance ptr */);

        // Update MRP_SequenceID (clause 8.1.12)
        MRP_PDU_16bit_write(rps->in_link_status_poll_sequence_id_ptr, rps->sequence_id[MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL]++, false /* Don't advance ptr */);

        // Transmit it (it won't get freed).
        MRP_PDU_tx(mrp_state, &rps->in_link_status_poll_tx_props, port_type, VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_STATUS_POLL);
    }
}

/******************************************************************************/
// mrp_pdu_create_all()
/******************************************************************************/
mesa_rc mrp_pdu_create_all(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;

    // These two timers are used for manually injected MRP_Test and MRP_InTest
    // PDUs. Not used when the AFI is used for this injection.
    mrp_timer_init(mrp_state->test_tx_timer,    "TestTx",   mrp_state->inst, MRP_PDU_test_tx,    mrp_state);
    mrp_timer_init(mrp_state->in_test_tx_timer, "InTestTx", mrp_state->inst, MRP_PDU_in_test_tx, mrp_state);

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        if (port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) {
            // The following PDUs are not transmitted on the interconnection
            // port.
            VTSS_RC(MRP_PDU_test_create(           mrp_state, port_type));
            VTSS_RC(MRP_PDU_topology_change_create(mrp_state, port_type));
            VTSS_RC(MRP_PDU_link_change_create(    mrp_state, port_type));
            VTSS_RC(MRP_PDU_option_create(         mrp_state, port_type, MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK));
            VTSS_RC(MRP_PDU_option_create(         mrp_state, port_type, MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE));

            // Create Tx props for S/W-forwarded MRP_InXXX PDUs.
            VTSS_RC(MRP_PDU_sw_fwd_create(mrp_state, port_type));
        }

        if (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
            // The following PDUs are potentially transmitted on all three ports
            VTSS_RC(MRP_PDU_in_test_create(            mrp_state, port_type));
            VTSS_RC(MRP_PDU_in_topology_change_create( mrp_state, port_type));
            VTSS_RC(MRP_PDU_in_link_change_create(     mrp_state, port_type));

            // MRP_InLinkStatusPoll PDUs are only transmitted on ring ports
            if (port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) {
                VTSS_RC(MRP_PDU_in_link_status_poll_create(mrp_state, port_type));
            } else {
                // Create Tx props for S/W-forwarded MRP_InXXX PDUs.
                VTSS_RC(MRP_PDU_sw_fwd_create(mrp_state, port_type));
            }
        }
    }

    // This function may be called while MRP_Test PDUs are being transmitted by
    // the AFI. This requires an update of the AFI. For manually injected
    // frames, this will happen upon the next Tx.
    if (MRP_can_use_afi && mrp_state->vars.test_tx_in_progress) {
        MRP_PDU_test_tx_stop(mrp_state);
        MRP_PDU_test_tx_start(mrp_state);
    }

    // This function may be called while MRP_InTest PDUs are being transmitted
    // by the AFI. This requires an update of the AFI. For manually injected
    // frames, this will happen upon the next Tx.
    if (MRP_can_use_afi && mrp_state->vars.in_test_tx_in_progress) {
        MRP_PDU_in_test_tx_stop(mrp_state);
        MRP_PDU_in_test_tx_start(mrp_state);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// mrp_pdu_free_all()
/******************************************************************************/
void mrp_pdu_free_all(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_ring_port_state_t         *rps;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        rps = &mrp_state->ring_port_states[port_type];
        MRP_PDU_test_tx_stop(mrp_state);
        MRP_PDU_in_test_tx_stop(mrp_state);
        mrp_state->vars.test_tx_in_progress    = false;
        mrp_state->vars.in_test_tx_in_progress = false;
        MRP_PDU_tx_props_dispose(&rps->test_afi_conf.tx_props[0]);
        MRP_PDU_tx_props_dispose(&rps->topology_change_tx_props);
        MRP_PDU_tx_props_dispose(&rps->link_change_tx_props);
        MRP_PDU_tx_props_dispose(&rps->test_mgr_nack_tx_props);
        MRP_PDU_tx_props_dispose(&rps->test_propagate_tx_props);
        MRP_PDU_tx_props_dispose(&rps->in_test_afi_conf.tx_props);
        MRP_PDU_tx_props_dispose(&rps->in_topology_change_tx_props);
        MRP_PDU_tx_props_dispose(&rps->in_link_change_tx_props);
        MRP_PDU_tx_props_dispose(&rps->in_link_status_poll_tx_props);
        MRP_PDU_tx_props_dispose(&rps->fwd_tx_props);
    }
}

/******************************************************************************/
// mrp_pdu_forward()
/******************************************************************************/
void mrp_pdu_forward(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, uint32_t length, vtss_appl_iec_mrp_pdu_type_t pdu_type)
{
    mesa_vid_t        vid       = mrp_state->conf.in_vlan;
    packet_tx_props_t *tx_props = &mrp_state->ring_port_states[port_type].fwd_tx_props;
    uint8_t           *p        = tx_props->packet_info.frm;

    // Copy frame in two tempi, in case the frame needs to be Tx tagged.
    // First 2 * MAC
    memcpy(p, frm, 2 * sizeof(mesa_mac_t));
    p += 2 * sizeof(mesa_mac_t);

    // Then skip past a possible VLAN tag (which already may have been added).
    // This function only forwards MRP_InXXX PDUs, so we always use the
    // interconnection VLAN - no matter which port it egresses.
    p += vid ? 4 : 0;

    // Then the remainder of the frame
    memcpy(p, &frm[2 * sizeof(mesa_mac_t)], length - 2 * sizeof(mesa_mac_t));

    // Adjust the length
    tx_props->packet_info.len = length + (vid ? 4 : 0);

    MRP_PDU_tx(mrp_state, tx_props, port_type, pdu_type);
}

/******************************************************************************/
// mrp_pdu_rx_frame()
/******************************************************************************/
bool mrp_pdu_rx_frame(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, mrp_rx_pdu_info_t &rx_pdu_info)
{
    if (!MRP_PDU_rx_validation_pass(mrp_state, port_type, frm, rx_info->length, rx_pdu_info)) {
        T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: Frame Rx on %s: Failed validation", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type, false, true));
        return false;
    }

    if (rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_TEST || rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST) {
        T_NG(MRP_TRACE_GRP_FRAME_RX, "%u: packet_rx(%s, %s, %s)", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type, false, true), iec_mrp_util_pdu_type_to_str(rx_pdu_info.pdu_type), rx_pdu_info.sa);
    } else {
        T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: packet_rx(%s, %s, %s)", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type, false, true), iec_mrp_util_pdu_type_to_str(rx_pdu_info.pdu_type), rx_pdu_info.sa);
    }

    return true;
}

/******************************************************************************/
// mrp_pdu_tlv_type_to_str()
/******************************************************************************/
const char *mrp_pdu_tlv_type_to_str(mrp_pdu_tlv_type_t tlv_type)
{
    switch (tlv_type) {
    case MRP_PDU_TLV_TYPE_END:
        return "MRP_End";

    case MRP_PDU_TLV_TYPE_COMMON:
        return "MRP_Common";

    case MRP_PDU_TLV_TYPE_TEST:
        return "MRP_Test";

    case MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE:
        return "MRP_TopologyChange";

    case MRP_PDU_TLV_TYPE_LINK_DOWN:
        return "MRP_LinkDown";

    case MRP_PDU_TLV_TYPE_LINK_UP:
        return "MRP_LinkUp";

    case MRP_PDU_TLV_TYPE_IN_TEST:
        return "MRP_InTest";

    case MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE:
        return "MRP_InTopologyChange";

    case MRP_PDU_TLV_TYPE_IN_LINK_DOWN:
        return "MRP_InLinkDown";

    case MRP_PDU_TLV_TYPE_IN_LINK_UP:
        return "MRP_InLinkUp";

    case MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL:
        return "MRP_InLinkStatusPoll";

    case MRP_PDU_TLV_TYPE_OPTION:
        return "MRP_Option";

    default:
        T_IG(MRP_TRACE_GRP_FRAME_RX, "Invalid tlv_type (%d)", tlv_type);
        return "MRP_Unknown";
    }
}

/******************************************************************************/
// mrp_pdu_sub_tlv_type_to_str()
/******************************************************************************/
const char *mrp_pdu_sub_tlv_type_to_str(mrp_pdu_sub_tlv_type_t sub_tlv_type)
{
    switch (sub_tlv_type) {
    case MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK:
        return "MRP_TestMgrNAck";

    case MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE:
        return "MRP_TestPropagate";

    case MRP_PDU_SUB_TLV_TYPE_AUTO_MGR:
        return "MRP_AutoMgr";

    default:
        return "MRP_Unknown";
    }
}

