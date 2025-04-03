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

#include "cfm_base.hxx"
#include "cfm_ccm.hxx"
#include "cfm_trace.h"
#include "cfm_api.h"     /* For cfm_util_fng_state_to_str() */
#include "misc_api.h"    /* For misc_mac_txt()              */

// As long as we can't receive a frame behind two tags, the CCM PDU contents
// always starts at offset 14 (two MACs + EtherType) because the packet module
// strips a possible outer tag.
#define CFM_CCM_PDU_START_OFFSET 14

#define CFM_CCM_TIMEOUT_CALC(_fph_) ((35 * 3600000) / (10 * (_fph_)))

// The DMAC used in CCM PDUs. The least significant nibble of the last byte is
// modified by the code to match the MD level. This is a.k.a. Class1 Multicast.
const mesa_mac_t cfm_multicast_dmac = {{0x01, 0x80, 0xc2, 0x00, 0x00, 0x30}};

// Forward declaration
static mesa_rc CFM_CCM_tx_frame_update(cfm_mep_state_t *mep_state);

/******************************************************************************/
// mesa_voe_cc_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_voe_cc_status_t &s)
{
    o << "{zero_period = "      << s.zero_period
      << ", rdi = "             << s.rdi
      << ", loc = "             << s.loc
      << ", period_unexp = "    << s.period_unexp
      << ", priority_unexp = "  << s.priority_unexp
      << ", mep_id_unexp = "    << s.mep_id_unexp
      << ", meg_id_unexp = "    << s.meg_id_unexp
      << ", mel_unexp = "       << s.mel_unexp
      << ", rx_port = "         << s.rx_port
      << ", port_status_tlv = " << s.port_status_tlv
      << ", if_status_tlv = "   << s.if_status_tlv
      << ", seen = "            << s.seen
      << ", tlv_seen = "        << s.tlv_seen
      << ", seq_unexp_seen = "  << s.seq_unexp_seen
      << "}";

    return o;
}

/******************************************************************************/
// mesa_voe_cc_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_voe_cc_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// CFM_CCM_use_afi()
/******************************************************************************/
static bool CFM_CCM_use_afi(const cfm_mep_state_t *mep_state)
{
    return mep_state->ma_conf->ccm_interval <= mep_state->global_state->ccm_afi_interval_max;
}

/******************************************************************************/
// CFM_CCM_interval_to_fph()
/******************************************************************************/
static mesa_rc CFM_CCM_interval_to_fph(cfm_mep_state_t *mep_state, vtss_appl_cfm_ccm_interval_t ccm_interval, uint64_t &fph, bool is_tx)
{
    switch (ccm_interval) {
    case VTSS_APPL_CFM_CCM_INTERVAL_300HZ:
        fph = 300 * 3600;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_10MS:
        fph = 100 * 3600;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_100MS:
        fph = 10 * 3600;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_1S:
        fph = 3600;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_10S:
        fph = 3600 / 10;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_1MIN:
        fph = 3600 / 60;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_10MIN:
        fph = 3600 / 600;
        break;

    case VTSS_APPL_CFM_CCM_INTERVAL_INVALID:
    default:
        if (is_tx) {
            // Only give a trace error if this function is invoked when we need
            // to construct the CCM frame we transmit outselves.
            T_EG(CFM_TRACE_GRP_CCM, "Invalid CCM interval (%d)", ccm_interval);
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        } else {
            return VTSS_RC_ERROR;
        }
    }

    if (!is_tx) {
        // If this function is invoked when we Rx a CCM frame, don't make more
        // checks.
        return VTSS_RC_OK;
    }

    if (!CFM_CCM_use_afi(mep_state)) {
        return VTSS_RC_OK;
    }

    if (fph < AFI_SINGLE_RATE_FPH_MIN) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Wanted = " VPRI64u " fph. Minimum allowed = %u", fph, AFI_SINGLE_RATE_FPH_MIN);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    if (fph > AFI_SINGLE_RATE_FPH_MAX) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Wanted = " VPRI64u " fph. Maximum allowed = %u", fph, AFI_SINGLE_RATE_FPH_MAX);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_sender_id_tlv_option_get()
/******************************************************************************/
static vtss_appl_cfm_sender_id_tlv_option_t CFM_CCM_tx_sender_id_tlv_option_get(const cfm_mep_state_t *mep_state)
{
    if (mep_state->ma_conf->sender_id_tlv_option != VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER) {
        // Sender ID TLV option is determined by the MA
        return mep_state->ma_conf->sender_id_tlv_option;
    }

    if (mep_state->md_conf->sender_id_tlv_option != VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER) {
        // Sender ID TLV option is determined by the MD
        return mep_state->md_conf->sender_id_tlv_option;
    }

    // Sender ID TLV option is determined by the global configuration
    return mep_state->global_conf->sender_id_tlv_option;
}

/******************************************************************************/
// CFM_CCM_tx_sender_id_tlv_option_len_get()
// See 802.1Q-2018, Table 21-7.
// Returns 0 or the total TLV length required.
/******************************************************************************/
static mesa_rc CFM_CCM_tx_sender_id_tlv_option_len_get(const cfm_mep_state_t *mep_state, size_t &len)
{
    vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option = CFM_CCM_tx_sender_id_tlv_option_get(mep_state);
    size_t                               mgmt_addr_len =  mep_state->global_state->ip_status.type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6 ? 18 : 6;

    switch (sender_id_tlv_option) {
    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE:
        len = 0;
        break;

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS:
        len = 1  /* Type = 1                 */ +
              2  /* Length                   */ +
              1  /* Chassis ID Length        */ +
              1  /* Chassis ID subtype (4)   */ +
              17 /* Chassis ID (MAC Address) */;
        break;

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_MANAGE:
        len = 1             /* Type = 1                                                                         */ +
              2             /* Length                                                                           */ +
              1             /* Chassis ID Length                                                                */ +
              1             /* Mgmt Addr Domain Length                                                          */ +
              10            /* Mgmt Addr Domain: transportDomainUdpIpv4 or transportDomainUdpIpv6 - BER-encoded */ +
              1             /* Mgmt Addr Length                                                                 */ +
              mgmt_addr_len /* Mgmt Addr (IPv4/v6 address + UDP port)                                           */;
        break;

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS_MANAGE:
        len = 1             /* Type = 1                                                                         */ +
              2             /* Length                                                                           */ +
              1             /* Chassis ID Length                                                                */ +
              1             /* Chassis ID subtype (4)                                                           */ +
              17            /* Chassis ID (MAC Address)                                                         */ +
              1             /* Mgmt Addr Domain Length                                                          */ +
              10            /* Mgmt Addr Domain: transportDomainUdpIpv4 or transportDomainUdpIpv6 - BER-encoded */ +
              1             /* Mgmt Addr Length                                                                 */ +
              mgmt_addr_len /* Mgmt Addr (IPv4/v6 address + UDP port)                                           */;
        break;

    default:
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Invalid sender_id_tlv_option (%d)", sender_id_tlv_option);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_sender_id_tlv_get()
// Clause 21.5.3.
// Used in CCM Frame Rx
// Given a frame, fills in a vtss_appl_cfm_sender_id_t structure.
// Returns true if it went OK, false otherwise.
// #offset is the position within #frm of the Chassis ID length field.
// #tlv_len is the length of the TLV.
/******************************************************************************/
static bool CFM_CCM_sender_id_tlv_get(const cfm_mep_state_t *mep_state, const uint8_t *frm, uint32_t offset, uint32_t tlv_len, vtss_appl_cfm_sender_id_t &sender_id)
{
    uint32_t tlv_pos = offset;

    // Validation test: The Chassis ID length field is less than tlv_len
    // - 1.
    if (frm[tlv_pos] >= tlv_len - 1) {
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Sender ID TLV Chassis ID len = %u >= tlv_len (%u) - 1)", mep_state->key, frm[tlv_pos], tlv_len);
        return false;
    }

    // Chassis ID Length (21.5.3.1)
    sender_id.chassis_id_len = frm[tlv_pos++];

    if (sender_id.chassis_id_len) {
        // Chassis ID Subtype (21.5.3.2)
        sender_id.chassis_id_subtype = (vtss_appl_cfm_chassis_id_subtype_t)frm[tlv_pos++];

        // Chassis ID (21.5.3.3)
        memcpy(sender_id.chassis_id, &frm[tlv_pos], MIN(sender_id.chassis_id_len, sizeof(sender_id.chassis_id)));
        tlv_pos += sender_id.chassis_id_len;
    }

    if (tlv_pos >= offset + tlv_len) {
        // Management Address Domain and Management Address parts of this
        // TLV are not present. Done.
        return true;
    }

    sender_id.mgmt_addr_domain_len = frm[tlv_pos++];
    if (!sender_id.mgmt_addr_domain_len) {
        // Management Addr Domain and Management Address parts of this
        // TLV are not present. Done.
        return true;
    }

    memcpy(sender_id.mgmt_addr_domain, &frm[tlv_pos], MIN(sender_id.mgmt_addr_domain_len, sizeof(sender_id.mgmt_addr_domain)));
    tlv_pos += sender_id.mgmt_addr_domain_len;

    if (tlv_pos >= offset + tlv_len) {
        // Management Address part of this TLV not present. Done.
        return true;
    }

    sender_id.mgmt_addr_len = frm[tlv_pos++];
    if (!sender_id.mgmt_addr_len) {
        // Management Address part of this TLV not present. Done.
        return true;
    }

    memcpy(sender_id.mgmt_addr, &frm[tlv_pos], MIN(sender_id.mgmt_addr_len, sizeof(sender_id.mgmt_addr)));

    return true;
}

/******************************************************************************/
// CFM_CCM_sender_id_tlv_set()
// Clause 21.5.3.
// Used in CCM Frame Tx
//
// The OID-to-BER encoder I used is here:
// https://misc.daniel-marschall.de/asn.1/oid-converter/online.php
/******************************************************************************/
static mesa_rc CFM_CCM_sender_id_tlv_set(const cfm_mep_state_t *mep_state, uint8_t *frm, uint32_t &offset)
{
    vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option = CFM_CCM_tx_sender_id_tlv_option_get(mep_state);
    size_t                               len;
    uint32_t                             start_offset;

    if (sender_id_tlv_option == VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE) {
        // Done.
        return VTSS_RC_OK;
    }

    start_offset = offset;

    // Type = Sender ID TLV = 1
    frm[offset++] = 1;

    // Length = 2 bytes.
    // #len is the total length of the TLV including 1 byte TLV Type and 2 bytes
    // TLV Length. The TLV's Length field does not include the Type and Length
    // field itself, hence "-3".
    VTSS_RC(CFM_CCM_tx_sender_id_tlv_option_len_get(mep_state, len));
    frm[offset++] = (len - 3) >> 8;
    frm[offset++] = (len - 3) >> 0;

    // Chassis ID
    if (sender_id_tlv_option == VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS ||
        sender_id_tlv_option == VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS_MANAGE) {
        // Chassis ID Length = Length of Chassis ID field = 17 (MAC address as
        // string.
        frm[offset++] = 17;

        // Chassis ID Subtype. We only support Type 4 = MAC Address.
        frm[offset++] = 4;

        // Chassis ID (System MAC Address).
        (void)misc_mac_txt(mep_state->global_state->smac.addr, (char *)&frm[offset]);
        offset += 17;
    } else {
        // Chassis ID not present => Chassis ID Length = 0.
        frm[offset++] = 0;
    }

    // Management Address Domain
    if (sender_id_tlv_option == VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_MANAGE ||
        sender_id_tlv_option == VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS_MANAGE) {
        // Mgmt Addr Domain Length
        frm[offset++] = 10;

        // Mgmt Addr Domain:
        //   transportDomainUdpIpv4 (OID = 1.3.6.1.2.1.100.1.1 => Hex = 06 08 2B 06 01 02 01 64 01 01 (BER-encoded))
        //   transportDomainUdpIpv6 (OID = 1.3.6.1.2.1.100.1.2 => Hex = 06 08 2B 06 01 02 01 64 01 02 (BER-encoded))
        frm[offset++] = 0x06; // Identifier (6 = ABSOLUTE OID)
        frm[offset++] = 0x08; // Length
        frm[offset++] = 0x2B; // Remaining bytes are the OID.
        frm[offset++] = 0x06;
        frm[offset++] = 0x01;
        frm[offset++] = 0x02;
        frm[offset++] = 0x01;
        frm[offset++] = 0x64;
        frm[offset++] = 0x01;
        frm[offset++] = mep_state->global_state->ip_status.type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6 ? 0x02 : 0x01;

        // Management Address Length
        frm[offset++] = mep_state->global_state->ip_status.type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6 ? 18 : 6;

        // Management Address
        if (mep_state->global_state->ip_status.type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6) {
            memcpy(&frm[offset], mep_state->global_state->ip_status.u.ipv6.net.address.addr, sizeof(mep_state->global_state->ip_status.u.ipv6.net.address.addr));
            offset += sizeof(mep_state->global_state->ip_status.u.ipv6.net.address.addr);
        } else {
            frm[offset++] = mep_state->global_state->ip_status.u.ipv4.net.address >> 24;
            frm[offset++] = mep_state->global_state->ip_status.u.ipv4.net.address >> 16;
            frm[offset++] = mep_state->global_state->ip_status.u.ipv4.net.address >>  8;
            frm[offset++] = mep_state->global_state->ip_status.u.ipv4.net.address >>  0;
        }

        // SNMP UDP port = 161 = 0xA1
        frm[offset++] = 0x00;
        frm[offset++] = 0xA1;
    }

    if (offset != start_offset + len) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Internal error: %u + %u != %u", start_offset, len, offset);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_port_status_tlv_option_get()
/******************************************************************************/
static vtss_appl_cfm_tlv_option_t CFM_CCM_tx_port_status_tlv_option_get(const cfm_mep_state_t *mep_state)
{
    if (mep_state->ma_conf->port_status_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Port Status TLV option is determined by the MA
        return mep_state->ma_conf->port_status_tlv_option;
    }

    if (mep_state->md_conf->port_status_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Port Status TLV option is determined by the MD
        return mep_state->md_conf->port_status_tlv_option;
    }

    // Port Status TLV option is determined by the global configuration
    return mep_state->global_conf->port_status_tlv_option;
}

/******************************************************************************/
// CFM_CCM_tx_port_status_tlv_option_len_get()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_port_status_tlv_option_len_get(const cfm_mep_state_t *mep_state, size_t &len)
{
    vtss_appl_cfm_tlv_option_t port_status_tlv_option = CFM_CCM_tx_port_status_tlv_option_get(mep_state);

    len = port_status_tlv_option == VTSS_APPL_CFM_TLV_OPTION_ENABLE ? 4 : 0;
    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_port_status_tlv_option_fill()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_port_status_tlv_option_fill(const cfm_mep_state_t *mep_state, uint8_t *frm, uint32_t &offset)
{
    size_t len;

    VTSS_RC(CFM_CCM_tx_port_status_tlv_option_len_get(mep_state, len));

    if (!len) {
        // Don't include it.
        return VTSS_RC_OK;
    }

    // Type = Port Status TLV = 2
    frm[offset++] = 2;

    // Length = 1
    frm[offset++] = (len - 3) >> 8;
    frm[offset++] = (len - 3) >> 0;

    // Value
    frm[offset++] = mep_state->status.errors.enableRmepDefect ? VTSS_APPL_CFM_PORT_STATUS_UP : VTSS_APPL_CFM_PORT_STATUS_BLOCKED;

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_interface_status_tlv_option_get()
/******************************************************************************/
static vtss_appl_cfm_tlv_option_t CFM_CCM_tx_interface_status_tlv_option_get(const cfm_mep_state_t *mep_state)
{
    if (mep_state->ma_conf->interface_status_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Interface Status TLV option is determined by the MA
        return mep_state->ma_conf->interface_status_tlv_option;
    }

    if (mep_state->md_conf->interface_status_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Interface Status TLV option is determined by the MD
        return mep_state->md_conf->interface_status_tlv_option;
    }

    // Interface Status TLV option is determined by the global configuration
    return mep_state->global_conf->interface_status_tlv_option;
}

/******************************************************************************/
// CFM_CCM_tx_interface_status_tlv_option_len_get()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_interface_status_tlv_option_len_get(const cfm_mep_state_t *mep_state, size_t &len)
{
    vtss_appl_cfm_tlv_option_t interface_status_tlv_option = CFM_CCM_tx_interface_status_tlv_option_get(mep_state);

    len = interface_status_tlv_option == VTSS_APPL_CFM_TLV_OPTION_ENABLE ? 4 : 0;
    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_interface_status_tlv_option_fill()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_interface_status_tlv_option_fill(const cfm_mep_state_t *mep_state, uint8_t *frm, uint32_t &offset)
{
    size_t len;

    VTSS_RC(CFM_CCM_tx_interface_status_tlv_option_len_get(mep_state, len));

    if (!len) {
        // Don't include it.
        return VTSS_RC_OK;
    }

    // Type = Interface Status TLV = 2
    frm[offset++] = 4;

    // Length = 1
    frm[offset++] = (len - 3) >> 8;
    frm[offset++] = (len - 3) >> 0;

    // Value (RBNTBD. Cannot depend on link state, since if link is down, we
    // cannot send CCM PDUs. Guess it's only used in Up-MEPs).
    frm[offset++] = mep_state->port_state->link ? VTSS_APPL_CFM_INTERFACE_STATUS_UP : VTSS_APPL_CFM_INTERFACE_STATUS_DOWN;

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_organization_specific_tlv_option_get()
/******************************************************************************/
static vtss_appl_cfm_tlv_option_t CFM_CCM_tx_organization_specific_tlv_option_get(const cfm_mep_state_t *mep_state)
{
    if (mep_state->ma_conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Organization-Specific TLV option is determined by the MA
        return mep_state->ma_conf->organization_specific_tlv_option;
    }

    if (mep_state->md_conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Organization-Specific TLV option is determined by the MD
        return mep_state->md_conf->organization_specific_tlv_option;
    }

    // Organization-specific TLV option is determined by the global configuration
    return mep_state->global_conf->organization_specific_tlv_option;
}

/******************************************************************************/
// CFM_CCM_tx_organization_specific_tlv_option_len_get()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_organization_specific_tlv_option_len_get(const cfm_mep_state_t *mep_state, size_t &len)
{
    vtss_appl_cfm_tlv_option_t organization_specific_tlv_option = CFM_CCM_tx_organization_specific_tlv_option_get(mep_state);

    if (organization_specific_tlv_option == VTSS_APPL_CFM_TLV_OPTION_DISABLE) {
        len = 0;
        return VTSS_RC_OK;
    }

    len = 1                                                           /* Type = 31 */ +
          2                                                           /* Length    */ +
          3                                                           /* OUI/CID   */ +
          1                                                           /* Subtype   */ +
          mep_state->global_conf->organization_specific_tlv.value_len /* Value     */;

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_organization_specific_tlv_get()
// Clause 21.5.2.
// Used in CCM Frame Rx
// Given a frame, fills in a vtss_appl_cfm_organization_specific_tlv_t structure
// Returns true if it went OK, false otherwise.
// #offset is the position within #frm of the OUI field (first field in TLV).
// #tlv_len is the length of the TLV.
/******************************************************************************/
static bool CFM_CCM_organization_specific_tlv_get(const cfm_mep_state_t *mep_state, const uint8_t *frm, uint32_t offset, uint32_t tlv_len, vtss_appl_cfm_organization_specific_tlv_t &tlv)
{
    if (tlv_len < 4) {
        // Must be at least 4 bytes long (OUI + Subtype)
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Organization-Specific TLV length (%u) < 4", mep_state->key, tlv_len);
        return false;
    }

    memcpy(tlv.oui, &frm[offset], sizeof(tlv.oui));
    tlv.subtype = frm[offset + 3];

    if (tlv_len > 4) {
        // Value present.
        tlv.value_len = tlv_len - 4;
        memcpy(tlv.value, &frm[offset + 4], MIN(tlv.value_len, sizeof(tlv.value)));
    }

    return true;
}

/******************************************************************************/
// CFM_CCM_organization_specific_tlv_set()
// Clause 21.5.2.
// Used in CCM Frame Tx
/******************************************************************************/
static mesa_rc CFM_CCM_organization_specific_tlv_set(const cfm_mep_state_t *mep_state, uint8_t *frm, uint32_t &offset)
{
    vtss_appl_cfm_organization_specific_tlv_t *tlv;
    uint32_t                                  start_offset;
    size_t                                    len;

    VTSS_RC(CFM_CCM_tx_organization_specific_tlv_option_len_get(mep_state, len));

    if (!len) {
        // Don't include it.
        return VTSS_RC_OK;
    }

    start_offset = offset;

    // Type = Organization-Specific TLV = 31
    frm[offset++] = 31;

    // Length = len - sizeof(Type) - sizeof(LengthFieldItself)
    frm[offset++] = (len - 3) >> 8;
    frm[offset++] = (len - 3) >> 0;

    tlv = &mep_state->global_conf->organization_specific_tlv;

    // OUI
    memcpy(&frm[offset], tlv->oui, sizeof(tlv->oui));
    offset += sizeof(tlv->oui);

    // Subtype
    frm[offset++] = tlv->subtype;

    // Value
    if (tlv->value_len) {
        memcpy(&frm[offset], tlv->value, tlv->value_len);
        offset += tlv->value_len;
    }

    if (offset != start_offset + len) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Internal error: %u + %u != %u", start_offset, len, offset);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_maid_create()
/******************************************************************************/
static mesa_rc CFM_CCM_maid_create(cfm_mep_state_t *mep_state)
{
    size_t   len;
    uint32_t offset = 0;
    uint8_t  *maid = mep_state->maid;

    memset(mep_state->maid, 0, sizeof(mep_state->maid));

    // MD Name Format
    maid[offset++] = mep_state->md_conf->format;

    if (mep_state->md_conf->format == VTSS_APPL_CFM_MD_FORMAT_STRING) {
        // MD Name length
        len = strlen(mep_state->md_conf->name);
        maid[offset++] = len;

        // MD Name
        memcpy(&maid[offset], mep_state->md_conf->name, len);
        offset += len;
    }

    // Short MA Name Format.
    maid[offset++] = mep_state->ma_conf->format;

    switch (mep_state->ma_conf->format) {
    case VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER:
    case VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID:
        // Short MA Name Length
        maid[offset++] = 2;

        if (mep_state->ma_conf->format == VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER) {
            // First two bytes of ma_conf->name
            maid[offset++] = mep_state->ma_conf->name[0];
            maid[offset++] = mep_state->ma_conf->name[1];
        } else {
            // MA's primary VID
            maid[offset++] = (mep_state->ma_conf->vlan >> 8) & 0xFF;
            maid[offset++] = (mep_state->ma_conf->vlan >> 0) & 0xFF;
        }

        break;

    case VTSS_APPL_CFM_MA_FORMAT_STRING:
    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC:
    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC:
        // Short MA Name length
        len = strlen(mep_state->ma_conf->name);
        maid[offset++] = len;

        // Short MA Name
        memcpy(&maid[offset], mep_state->ma_conf->name, len);
        offset += len;
        break;

    default:
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Unsupported MA format (%d)", mep_state->ma_conf->format);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_frame_generate()
// Whether or not we use the AFI, we use #afi_conf to hold the frame and Tx
// properties.
/******************************************************************************/
static mesa_rc CFM_CCM_tx_frame_generate(cfm_mep_state_t *mep_state, afi_single_conf_t *afi_conf)
{
    packet_tx_info_t      *packet_info;
    mesa_packet_tx_info_t *tx_info;
    uint8_t               *frm;
    uint32_t              offset, frm_len;
    size_t                sender_id_tlv_option_len, port_status_tlv_option_len, vid_len;
    size_t                interface_status_tlv_option_len, organization_specific_tlv_option_len;
    bool                  rdi, isdx_based = false;
    mesa_vid_t            vid;
    mesa_rc               rc;

    if (CFM_CCM_use_afi(mep_state)) {
        if ((rc = afi_single_conf_init(afi_conf)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_FRAME_TX, "%s", error_txt(rc));
            return rc;
        }
    } else {
        // Gotta initialize it ourselves.
        packet_tx_props_init(&afi_conf->tx_props);
    }

    afi_conf->params.fph = mep_state->fph;

    // Transmit first frame ASAP (according to 802.1Q clause 20.12).
    afi_conf->params.first_frame_urgent = TRUE;

    vid = mep_state->classified_vid_get();

    if (mep_state->is_port_mep()) {
        // Independent of platform, we always control egress tagging for Port
        // MEPs ourselves, that is, we inject directly into the port (Lu26 and
        // Serval-1) or inject directly into an ISDX-based ES0 rule, which
        // takes care of NOT pushing any tags (JR2).

        // The idea here is that since Port MEPs sits at the ISS entity in
        // 802.1Q terminology, there is no automatic tagging function, so all
        // tagging must be performed by the Down MEP itself if needed.
        // If the user has configured the MEP as untagged, we send untagged
        // traffic.
        // If the user has not configured the MEP as untagged, we always send
        // tagged whether or not the Port MEP's VLAN is the PVID and the PVID
        // is configured to be untagged.
        if (mep_state->is_tagged()) {
            vid_len = 4;
        } else {
            // Truly untagged Port MEP
            vid_len = 0;
        }

        // On JR2, our Port MEPs always use ISDX-based injection, because
        // otherwise we wouldn't be able to have a VLAN MEP in the same VLAN and
        // hit the port VOE and at the same time get the frame marked as OAM.
        // On Serval-1, we don't have TCEs for Port MEPs, so there, we always
        // hit the Port VOEs.
        isdx_based = mep_state->global_state->has_vop_v2;
    } else {
        // For VLAN MEPs it's a different story, since they sit at the EISS
        // entity, so that a tagging function sits between the MEPs and the
        // physical port.

        // Here, we have to branch on the chip we are using, because they all
        // behave slightly differently.
        if (!mep_state->global_state->has_vop) {
            // On Lu26, we still need to control tagging entirely ourselves.
            // This means that we have to take into account whether the MEP's
            // VLAN gets tagged or untagged on egress.
            if (mep_state->classified_vid_gets_tagged()) {
                vid_len = 4;
            } else {
                vid_len = 0;
            }
        } else if (mep_state->global_state->has_vop_v1) {
            // On Serval-1, we must hit an ISDX-based TCE on egress. That TCE is
            // only hit by the frames we inject. When it was configured (in
            // mep_base.cxx), it was chosen whether or not the frames we inject
            // should be tagged, so we should not put a tag into the frame here.
            vid_len = 0;
            isdx_based = true;
        } else {
            // This branch is for VOP_V2, because VOP_V0 cannot create VLAN
            // MEPs.
            // On JR2, we hit a Classified-VID TCE, which is configured to
            // follow the port's tagging, so here we do not insert a tag, and
            // it is not ISDX-based.
            vid_len = 0;
        }
    }

    VTSS_RC(CFM_CCM_tx_sender_id_tlv_option_len_get(mep_state, sender_id_tlv_option_len));
    VTSS_RC(CFM_CCM_tx_port_status_tlv_option_len_get(mep_state, port_status_tlv_option_len));
    VTSS_RC(CFM_CCM_tx_interface_status_tlv_option_len_get(mep_state, interface_status_tlv_option_len));
    VTSS_RC(CFM_CCM_tx_organization_specific_tlv_option_len_get(mep_state, organization_specific_tlv_option_len));

    // Figure out how much space we need
    frm_len = 6                                    /* DMAC                      */ +
              6                                    /* SMAC                      */ +
              vid_len                              /* Possible VLAN tag         */ +
              2                                    /* Ethertype                 */ +
              4                                    /* Common CFM Header         */ +
              4                                    /* Sequence number           */ +
              2                                    /* MEP-ID                    */ +
              MESA_OAM_MEG_ID_LENGTH               /* MAID                      */ +
              16                                   /* TxFCf, RxFCb, TxFCb       */ +
              sender_id_tlv_option_len             /* Sender ID TLV             */ +
              port_status_tlv_option_len           /* Port Status TLV           */ +
              interface_status_tlv_option_len      /* Interface Status TLV      */ +
              organization_specific_tlv_option_len /* Organization-Specific TLV */ +
              1                                    /* End TLV                   */;

    packet_info = &afi_conf->tx_props.packet_info;

    if ((packet_info->frm = packet_tx_alloc(frm_len)) == NULL) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Out of memory");
        return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
    }

    packet_info->modid   = VTSS_MODULE_ID_CFM;
    packet_info->len     = frm_len;
    packet_info->no_free = TRUE;

    tx_info             = &afi_conf->tx_props.tx_info;
    tx_info->oam_type   = MESA_PACKET_OAM_TYPE_CCM;
    tx_info->pdu_offset = 14 + vid_len;

    if (mep_state->mep_conf->mep_conf.direction == VTSS_APPL_CFM_DIRECTION_DOWN) {
        tx_info->dst_port_mask = VTSS_BIT64(mep_state->port_state->port_no);
        tx_info->pipeline_pt   = MESA_PACKET_PIPELINE_PT_REW_IN_VOE;
    } else {
        tx_info->switch_frm      = TRUE;
        tx_info->masquerade_port = mep_state->port_state->port_no;
        tx_info->pipeline_pt     = MESA_PACKET_PIPELINE_PT_ANA_OU_VOE;
    }

    if (mep_state->is_port_mep() && (mep_state->global_state->has_vop_v0 || mep_state->global_state->has_vop_v1)) {
        // If not sending with 'classified' VID == 0, a port MEP will become
        // double tagged, because we cannot avoid to hit some sort of rewriter.
        // On JR2, we have ES0 rules that we hit in this case, that will take
        // care of not having H/W adding another tag.
        tx_info->tag.vid = 0;
    } else {
        tx_info->tag.vid = vid;
    }

    tx_info->tag.pcp = mep_state->mep_conf->mep_conf.pcp;
    tx_info->tag.dei = 0;
    tx_info->cos     = mep_state->mep_conf->mep_conf.pcp;
    tx_info->cosid   = mep_state->mep_conf->mep_conf.pcp;
    tx_info->dp      = 0;

    if (isdx_based) {
        // As discussed above, we use an ISDX-based injection TCE (ES0 rule) in
        // some cases.
        tx_info->iflow_id = mep_state->iflow_id;
    }

    frm = afi_conf->tx_props.packet_info.frm;
    memset(frm, 0, frm_len);
    offset = 0;

    //
    // Layer 2
    //

    // DMAC
    memcpy(&frm[offset], cfm_multicast_dmac.addr, sizeof(cfm_multicast_dmac.addr));

    // Change last nibble of DMAC to match the MD level
    frm[offset + 5] |= mep_state->md_conf->level;
    offset += sizeof(cfm_multicast_dmac.addr);

    // SMAC
    memcpy(&frm[offset], cfm_base_smac_get(mep_state).addr, sizeof(mesa_mac_t));

    offset += sizeof(mep_state->mep_conf->mep_conf.smac.addr);

    // Possible VLAN tag
    if (vid_len) {
        // Insert TPID
        frm[offset++] = mep_state->port_state->tpid >> 8;
        frm[offset++] = mep_state->port_state->tpid >> 0;

        // Compute PCP, DEI (= 0), and VID
        frm[offset++] = (mep_state->mep_conf->mep_conf.pcp << 5) | ((vid >> 8) & 0x0F);
        frm[offset++] = vid & 0xFF;
    }

    // EtherType
    frm[offset++] = (CFM_ETYPE >> 8) & 0xFF;
    frm[offset++] = (CFM_ETYPE >> 0) & 0xFF;

    //
    // CFM header
    //

    // MD level + Version (= 0)
    frm[offset++] = mep_state->md_conf->level << 5;

    // OpCode = CCM = 1
    frm[offset++] = CFM_OPCODE_CCM;

    // CCM flags (RDI (1 bit), Reserved (4 bits), CCM Interval (3 bits)
    // If we have a VOP, the RDI bit is set or cleared through calls to the API.
    rdi = mep_state->global_state->has_vop ? 0 : mep_state->status.present_rdi;
    frm[offset++] = (rdi << 7) | (mep_state->ma_conf->ccm_interval);

    // First TLV Offset. Number of bytes after this one until first TLV comes:
    //    Sequence number:     4 bytes
    //    MEP-ID:              2 bytes
    //    MAID:               48 bytes
    //    Defined by Y.1731   16 bytes
    frm[offset++] = 4 + 2 + 48 + 16; // == 70

    //
    // CCM
    //

    // Sequence number (4 bytes). Let H/W update whenever possible, and let
    // S/W update whenever needed.
    mep_state->ccm_state.seq_number_offset = offset;
    offset += 4;

    // MEP-ID
    frm[offset++] = mep_state->key.mepid >> 8;
    frm[offset++] = mep_state->key.mepid >> 0;

    //
    // MAID (a.k.a. MEGID)
    //

    memcpy(&frm[offset], mep_state->maid, sizeof(mep_state->maid));
    offset += MESA_OAM_MEG_ID_LENGTH;

    // 21.6.6.
    // Add 16 0-bytes defined by Y.1731 (TxFCf, RcFCb, TxFCb, Reserved).
    // According to 802.1Q Table 21-14, this is also added for non-Y.1731-
    // formatted PDUs.
    offset += 16;

    //
    // Optional TLVs
    //

    // Sender ID TLV
    if ((rc = CFM_CCM_sender_id_tlv_set(mep_state, frm, offset)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // Port Status TLV
    if ((rc  = CFM_CCM_tx_port_status_tlv_option_fill(mep_state, frm, offset)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // Interface Status TLV
    if ((rc = CFM_CCM_tx_interface_status_tlv_option_fill(mep_state, frm, offset)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // Organization-Specific TLV.
    if ((rc = CFM_CCM_organization_specific_tlv_set(mep_state, frm, offset)) != VTSS_RC_OK) {
        goto do_exit;
    }

    //
    // End TLV
    //
    frm[offset++] = 0;

    // Final check of length
    if (offset != frm_len) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "Pre-calculated CCM PDU length (%u) doesn't match final length (%u)", frm_len, offset);
        rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        goto do_exit;
    }

do_exit:
    if (rc != VTSS_RC_OK) {
        packet_tx_free(frm);
        afi_conf->tx_props.packet_info.frm = nullptr;
    }

    return rc;
}

/******************************************************************************/
// CFM_CCM_notif_entry_create()
/******************************************************************************/
static void CFM_CCM_notif_entry_create(cfm_mep_state_t *mep_state)
{
    vtss_appl_cfm_mep_notification_status_t notif_status;
    mesa_rc                                 rc;

    memset(&notif_status, 0, sizeof(notif_status));

    T_DG(CFM_TRACE_GRP_NOTIF, "MEP %s. Create entry", mep_state->key);
    if ((rc = cfm_mep_notification_status.set(mep_state->key, &notif_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to create notif status: %s", mep_state->key, error_txt(rc));
    }
}

/******************************************************************************/
// CFM_CCM_clearFaultAlarm()
/******************************************************************************/
static void CFM_CCM_clearFaultAlarm(cfm_mep_state_t *mep_state)
{
    vtss_appl_cfm_mep_notification_status_t notif_status;
    mesa_rc                                 rc;

    if ((rc = cfm_mep_notification_status.get(mep_state->key, &notif_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to get fault alarm status: %s", mep_state->key, error_txt(rc));
    }

    notif_status.highest_defect = VTSS_APPL_CFM_MEP_DEFECT_NONE;

    T_DG(CFM_TRACE_GRP_NOTIF, "MEP %s", mep_state->key);
    if ((rc = cfm_mep_notification_status.set(mep_state->key, &notif_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to create/clear fault alarm: %s", mep_state->key, error_txt(rc));
    }
}

/******************************************************************************/
// CFM_CCM_xmitFaultAlarm()
/******************************************************************************/
static void CFM_CCM_xmitFaultAlarm(cfm_mep_state_t *mep_state)
{
    vtss_appl_cfm_mep_notification_status_t notif_status;
    mesa_rc                                 rc;

    if ((rc = cfm_mep_notification_status.get(mep_state->key, &notif_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to get fault alarm status: %s", mep_state->key, error_txt(rc));
    }

    if (notif_status.highest_defect >= mep_state->status.highest_defect) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Setting notification of highest defect to %d, but it is already at or higher (%d)",
             mep_state->key, mep_state->status.highest_defect, notif_status.highest_defect);
    }

    notif_status.highest_defect = mep_state->status.highest_defect;
    T_DG(CFM_TRACE_GRP_NOTIF, "MEP %s: Setting highest defect to %s", mep_state->key, cfm_util_mep_defect_to_str(notif_status.highest_defect));

    if ((rc = cfm_mep_notification_status.set(mep_state->key, &notif_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to set fault alarm status to %d: %s", mep_state->key, mep_state->status.highest_defect, error_txt(rc));
    }

    // Proceed to the FNG_DEFECT_REPORTED state
    mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_DEFECT_REPORTED;
}

/******************************************************************************/
// CFM_CCM_rx_defects_update()
/******************************************************************************/
static void CFM_CCM_rx_defects_update(cfm_mep_state_t *mep_state)
{
    cfm_ccm_state_t            *ccm_state = &mep_state->ccm_state;
    cfm_rmep_state_itr_t       rmep_state_itr;
    bool                       portStatusDefect = true, interfaceStatusDefect = false, old_presentRDI, old_MAdefectIndication;
    vtss_appl_cfm_mep_defect_t new_highest_defect;
    int                        trace_lvl;
    uint8_t                    old_defects;
    char                       buf[20];

    if (!mep_state->status.mep_active) {
        return;
    }

    // Compute RMEP defects
    ccm_state->someRMEPCCMdefect   = false;
    ccm_state->someMACstatusDefect = false;
    ccm_state->someRDIdefect       = false;
    ccm_state->allRMEPsDead        = true;

    for (rmep_state_itr = mep_state->rmep_states.begin(); rmep_state_itr != mep_state->rmep_states.end(); ++rmep_state_itr) {
        // 20.35.5
        // When RMEP is in RMEP_START state, we haven't received any CCM yet, so
        // there's a CCM defect.
        ccm_state->someRMEPCCMdefect |= rmep_state_itr->second.rMEPCCMdefect || rmep_state_itr->second.status.state == VTSS_APPL_CFM_RMEP_STATE_START;

        // Preparation for 20.35.6
        portStatusDefect             &= rmep_state_itr->second.rMEPportStatusDefect;
        interfaceStatusDefect        |= rmep_state_itr->second.rMEPinterfaceStatusDefect;

        // 20.35.7
        ccm_state->someRDIdefect     |= rmep_state_itr->second.status.rdi; // rMEPlastRDI

        // 20.9.4
        ccm_state->allRMEPsDead      &= rmep_state_itr->second.rMEPCCMdefect;
    }

    // Finalize 20.35.6
    ccm_state->someMACstatusDefect = portStatusDefect || interfaceStatusDefect;

    if (ccm_state->someRDIdefect || ccm_state->someMACstatusDefect || ccm_state->someRMEPCCMdefect || ccm_state->errorCCMdefect || ccm_state->xconCCMdefect) {
        trace_lvl = VTSS_TRACE_LVL_DEBUG;
    } else {
        trace_lvl = VTSS_TRACE_LVL_NOISE;
    }

    T(CFM_TRACE_GRP_CCM, trace_lvl, "someRDIdefect = %d, someMACstatusDefect = %d, someRMEPCCMdefect = %d, errorCCMdefect = %d, xconCCMdefect = %d",
      ccm_state->someRDIdefect,
      ccm_state->someMACstatusDefect,
      ccm_state->someRMEPCCMdefect,
      ccm_state->errorCCMdefect,
      ccm_state->xconCCMdefect);

    // Compute MAdefectIndication (20.9.3)
    old_MAdefectIndication = ccm_state->MAdefectIndication;
    ccm_state->MAdefectIndication = false;

    // Compute presentRDI (20.9.6), which also depends on the alarm-level.
    old_presentRDI = mep_state->status.present_rdi;
    mep_state->status.present_rdi = false;

    // mep_conf->alarm_level === LowestAlarmPri
    if (mep_state->mep_conf->mep_conf.alarm_level <= 1) {
        ccm_state->MAdefectIndication |= ccm_state->someRDIdefect;
    }

    if (mep_state->mep_conf->mep_conf.alarm_level <= 2) {
        ccm_state->MAdefectIndication |= ccm_state->someMACstatusDefect;
        mep_state->status.present_rdi  |= ccm_state->someMACstatusDefect;
    }

    if (mep_state->mep_conf->mep_conf.alarm_level <= 3) {
        ccm_state->MAdefectIndication |= ccm_state->someRMEPCCMdefect;
        mep_state->status.present_rdi  |= ccm_state->someRMEPCCMdefect;
    }

    if (mep_state->mep_conf->mep_conf.alarm_level <= 4) {
        ccm_state->MAdefectIndication |= ccm_state->errorCCMdefect;
        mep_state->status.present_rdi  |= ccm_state->errorCCMdefect;
    }

    if (mep_state->mep_conf->mep_conf.alarm_level <= 5) {
        ccm_state->MAdefectIndication |= ccm_state->xconCCMdefect;
        mep_state->status.present_rdi  |= ccm_state->xconCCMdefect;
    }

    if (old_presentRDI != mep_state->status.present_rdi && mep_state->mep_conf->mep_conf.ccm_enable && mep_state->status.mep_active) {
        // Gotta update the CCM PDUs we send, because the RDI bit changes.
        T_IG(CFM_TRACE_GRP_CCM, "MEP %s: Frame update due to change in presentRDI (%d->%d)", mep_state->key, old_presentRDI, mep_state->status.present_rdi);
        (void)CFM_CCM_tx_frame_update(mep_state);
    }

    // Compute the new highest_defect. highest_defect obeys the selected
    // alarm_level (lowestAlarmPri), but needs not be guarded by that level when
    // we compute it, because we also use the value of MAdefectIndication, which
    // is already guarded by the alarm level.
    if (ccm_state->xconCCMdefect) {
        new_highest_defect = VTSS_APPL_CFM_MEP_DEFECT_XCON_CCM;
    } else if (ccm_state->errorCCMdefect) {
        new_highest_defect = VTSS_APPL_CFM_MEP_DEFECT_ERROR_CCM;
    } else if (ccm_state->someRMEPCCMdefect) {
        new_highest_defect = VTSS_APPL_CFM_MEP_DEFECT_REMOTE_CCM;
    } else if (ccm_state->someMACstatusDefect) {
        new_highest_defect = VTSS_APPL_CFM_MEP_DEFECT_MAC_STATUS;
    } else if (ccm_state->someRDIdefect) {
        new_highest_defect = VTSS_APPL_CFM_MEP_DEFECT_RDI_CCM;
    } else {
        new_highest_defect = VTSS_APPL_CFM_MEP_DEFECT_NONE;
    }

    // Also update the #defects member of the status. This is always updated
    // regardless of the alarm_level.
    old_defects = mep_state->status.defects;
    mep_state->status.defects = 0;
    if (ccm_state->xconCCMdefect) {
        mep_state->status.defects |= VTSS_APPL_CFM_MEP_DEFECT_MASK_XCON_CCM;
    }

    if (ccm_state->errorCCMdefect) {
        mep_state->status.defects |= VTSS_APPL_CFM_MEP_DEFECT_MASK_ERROR_CCM;
    }

    if (ccm_state->someRMEPCCMdefect) {
        mep_state->status.defects |= VTSS_APPL_CFM_MEP_DEFECT_MASK_REMOTE_CCM;
    }

    if (ccm_state->someMACstatusDefect) {
        mep_state->status.defects |= VTSS_APPL_CFM_MEP_DEFECT_MASK_MAC_STATUS;
    }

    if (ccm_state->someRDIdefect) {
        mep_state->status.defects |= VTSS_APPL_CFM_MEP_DEFECT_MASK_RDI_CCM;
    }

    if (mep_state->status.defects || old_defects != mep_state->status.defects) {
        T_IG(CFM_TRACE_GRP_CCM, "MEP %s: defects = %s", mep_state->key, cfm_util_mep_defects_to_str(mep_state->status.defects, buf));
    }

    if (old_MAdefectIndication != ccm_state->MAdefectIndication || mep_state->status.fng_state != VTSS_APPL_CFM_FNG_STATE_RESET) {
        T_DG(CFM_TRACE_GRP_CCM, "MEP %s: MAdefectIndication: %d->%d, FNG State = %s", mep_state->key, old_MAdefectIndication, ccm_state->MAdefectIndication, cfm_util_fng_state_to_str(mep_state->status.fng_state));
    }

    if (new_highest_defect != mep_state->status.highest_defect) {
        T_IG(CFM_TRACE_GRP_CCM, "MEP %s: FNG state = %s. highest_defect: %s->%s", mep_state->key, cfm_util_fng_state_to_str(mep_state->status.fng_state), cfm_util_mep_defect_to_str(mep_state->status.highest_defect), cfm_util_mep_defect_to_str(new_highest_defect));
    }

    // Create a notification if needed
    cfm_base_mep_ok_notification_update(mep_state);

    // MEP Fault Notification Generator state machine (20.37)
    switch (mep_state->status.fng_state) {
    case VTSS_APPL_CFM_FNG_STATE_RESET:
        // FNG_RESET
        if (!ccm_state->MAdefectIndication) {
            break;
        }

        // No last reported defect.
        if (mep_state->status.highest_defect != VTSS_APPL_CFM_MEP_DEFECT_NONE) {
            T_EG(CFM_TRACE_GRP_CCM, "MEP %s: In FNG_RESET state, but current highest_defect is not NONE, but %d", mep_state->key, mep_state->status.highest_defect);
        }

        mep_state->status.highest_defect = new_highest_defect;

        // Proceed to the FNG_DEFECT state.
        mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_DEFECT;

        // Start FNGwhile timer with fngAlarmTime, so that it becomes an alarm
        // when it expires.
        T_IG(CFM_TRACE_GRP_CCM, "MEP %s: Starting FNGwhile_timer(present)", mep_state->key);
        cfm_timer_start(ccm_state->FNGwhile_timer, mep_state->mep_conf->mep_conf.alarm_time_present_ms, false);
        break;

    case VTSS_APPL_CFM_FNG_STATE_DEFECT:
        // FNG_DEFECT
        if (!ccm_state->MAdefectIndication) {
            // Just stop the timer, because the defect was too short to give
            // rise to a fault alarm.
            T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Stopping FNGwhile_timer", mep_state->key);
            cfm_timer_stop(ccm_state->FNGwhile_timer);

            // Go back to the reset state.
            mep_state->status.fng_state      = VTSS_APPL_CFM_FNG_STATE_RESET;
            mep_state->status.highest_defect = VTSS_APPL_CFM_MEP_DEFECT_NONE;
            CFM_CCM_clearFaultAlarm(mep_state);
        } else {
            // As long as we haven't yet reported any defects, keep updating the
            // current highest defect, so don't compare to the currently stored.
            mep_state->status.highest_defect = new_highest_defect;

            // Wait for FNGwhile_timer to expire
            // Handled by CFM_CCM_FNGwhile_timeout()
        }

        break;

    case VTSS_APPL_CFM_FNG_STATE_DEFECT_REPORTED:
        // FNG_DEFECT_REPORTED.
        // At least one defect has already been reported.
        if (!ccm_state->MAdefectIndication) {
            // Since there's no longer a (reportable) defect, we need to start
            // the FNGwhile_timer with the user-selected fngResetTime and
            // proceed to the FNG_DEFECT_CLEARING state.
            mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_DEFECT_CLEARING;
            T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Starting FNGwhile_timer(absent)", mep_state->key);
            cfm_timer_start(ccm_state->FNGwhile_timer, mep_state->mep_conf->mep_conf.alarm_time_absent_ms, false);
        } else {
            // If the new highest defect has higher priority than the last
            // reported, we need to generate a new Fault Alarm.
            if (new_highest_defect > mep_state->status.highest_defect) {
                mep_state->status.highest_defect = new_highest_defect;
                mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT;
                CFM_CCM_xmitFaultAlarm(mep_state);
            }
        }

        break;

    case VTSS_APPL_CFM_FNG_STATE_DEFECT_CLEARING:
        // FNG_DEFECT_CLEARING
        // We are currently waiting for fngResetTime to reach zero.
        if (!ccm_state->MAdefectIndication) {
            // Still no alarm.
            break;
        }

        // Alarm again!
        // Stop alarm clearing timer.
        T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Stopping FNGwhile_timer", mep_state->key);
        cfm_timer_stop(ccm_state->FNGwhile_timer);

        // And go to one or the other mode, depending on whether this new defect
        // has higher priority than the last reported.
        if (new_highest_defect > mep_state->status.highest_defect) {
            // This defect has higher priority than the last reported
            // defect, so gotta report a new.
            mep_state->status.highest_defect = new_highest_defect;
            mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT;
            CFM_CCM_xmitFaultAlarm(mep_state);
        } else {
            mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_DEFECT_REPORTED;
        }

        break;

    case VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT: // Transitional
    default:
        T_EG(CFM_TRACE_GRP_CCM, "Unsupported Fault Notification Generator State (%d)", mep_state->status.fng_state);
        break;
    }
}

/******************************************************************************/
// CFM_CCM_rx_counter_poll_timeout()
// Timer == rx_counter_poll_timer
// Fires when it's time to read counters. This is only happening on chips with a
// VOP.
/******************************************************************************/
static void CFM_CCM_rx_counter_poll_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_t        *mep_state = (cfm_mep_state_t *)context;
    mesa_voe_cc_counters_t cc_counters;
    mesa_rc                rc;

    VTSS_ASSERT(mep_state);

    T_NG(CFM_TRACE_GRP_CCM, "MEP %s: Time to poll counters", mep_state->key);

    if (mep_state->voe_idx == MESA_VOE_IDX_NONE) {
        return;
    }

    if ((rc = mesa_voe_cc_counters_get(nullptr, mep_state->voe_idx, &cc_counters)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CCM, "mesa_voe_cc_counters_get(%u) failed: %s", mep_state->voe_idx, error_txt(rc));
        return;
    }

    mep_state->status.ccm_rx_valid_cnt          = cc_counters.rx_valid_counter;
    mep_state->status.ccm_rx_invalid_cnt        = cc_counters.rx_invalid_counter;
    mep_state->status.ccm_rx_sequence_error_cnt = cc_counters.rx_oo_counter;
    mep_state->status.ccm_tx_cnt                = cc_counters.tx_counter;
}

/******************************************************************************/
// CFM_CCM_CCIwhile_timeout()
// Timer == CCIwhile_timer
/******************************************************************************/
static void CFM_CCM_CCIwhile_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_t *mep_state = (cfm_mep_state_t *)context;
    uint32_t        seq_number, offset;
    uint8_t         *frm;

    VTSS_ASSERT(mep_state);

    // This timer fires if not using AFI to transmit frames.
    if (CFM_CCM_use_afi(mep_state)) {
        T_EG(CFM_TRACE_GRP_FRAME_TX, "MEP %s. Why did the CCM Tx timer fire when using AFI?", mep_state->key);
        cfm_timer_stop(timer);
        return;
    }

    // Gotta write the sequece number before the frame gets transmitted.
    frm        = mep_state->ccm_state.afi_conf.tx_props.packet_info.frm;
    offset     = mep_state->ccm_state.seq_number_offset;
    seq_number = mep_state->ccm_state.next_seq_number;

    frm[offset + 0] = seq_number >> 24;
    frm[offset + 1] = seq_number >> 16;
    frm[offset + 2] = seq_number >>  8;
    frm[offset + 3] = seq_number >>  0;

    mep_state->ccm_state.next_seq_number++;

    // Transmit the frame manually
    T_NG(CFM_TRACE_GRP_FRAME_TX, "MEP %s: Tx CCM", mep_state->key);
    (void)packet_tx(&mep_state->ccm_state.afi_conf.tx_props);
    mep_state->status.ccm_tx_cnt++;
}

/******************************************************************************/
// CFM_CCM_FNGwhile_timeout()
// Timer == FNGwhile_timer
/******************************************************************************/
static void CFM_CCM_FNGwhile_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_t           *mep_state = (cfm_mep_state_t *)context;
    vtss_appl_cfm_fng_state_t old_fng_state;

    VTSS_ASSERT(mep_state);

    old_fng_state = mep_state->status.fng_state;

    // We should only get here if current Fault Notification Generator state is
    // FNG_DEFECT or FNG_DEFECT_CLEARING.
    switch (mep_state->status.fng_state) {
    case VTSS_APPL_CFM_FNG_STATE_DEFECT:
        // Time to transmit a Fault Alarm.
        mep_state->status.fng_state = VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT;
        CFM_CCM_xmitFaultAlarm(mep_state);
        break;

    case VTSS_APPL_CFM_FNG_STATE_DEFECT_CLEARING:
        // The defect is not cleared and we can go back to FNG_RESET.
        mep_state->status.highest_defect = VTSS_APPL_CFM_MEP_DEFECT_NONE;
        mep_state->status.fng_state      = VTSS_APPL_CFM_FNG_STATE_RESET;
        CFM_CCM_clearFaultAlarm(mep_state);
        break;

    default:
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: Should not be able to time out FNGwhile timer in this state (%d)", mep_state->key, mep_state->status.fng_state);
        break;
    }

    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: FNGwhile timed out. FNG state: %s->%s", mep_state->key, cfm_util_fng_state_to_str(old_fng_state), cfm_util_fng_state_to_str(mep_state->status.fng_state));
}

/******************************************************************************/
// CFM_CCM_xcon_timeout()
// Timer == xconCCMwhile_timer
/******************************************************************************/
static void CFM_CCM_xcon_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_t *mep_state = (cfm_mep_state_t *)context;

    VTSS_ASSERT(mep_state);

    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: xconCCMwhile timed out", mep_state->key);

    mep_state->ccm_state.xconCCMdefect = false;
    CFM_CCM_rx_defects_update(mep_state);
}

/******************************************************************************/
// CFM_CCM_error_timeout()
// Timer == errorCCMwhile_timer
/******************************************************************************/
static void CFM_CCM_error_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_t *mep_state = (cfm_mep_state_t *)context;

    VTSS_ASSERT(mep_state);

    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: errorCCMwhile timed out", mep_state->key);

    mep_state->ccm_state.errorCCMdefect = false;
    CFM_CCM_rx_defects_update(mep_state);
}

/******************************************************************************/
// CFM_CCM_rmep_loc()
// 20.20: Invoked when rMEPwhile timer times out (S/W-based) or LOC is detected
// (H/W-based).
/******************************************************************************/
static void CFM_CCM_rmep_loc(cfm_rmep_state_itr_t rmep_state_itr)
{
    rmep_state_itr->second.rMEPCCMdefect = true;
    T_DG(CFM_TRACE_GRP_CCM, "RMEPID %u: Loss-of-continuity", rmep_state_itr->first);

    if (rmep_state_itr->second.status.state != VTSS_APPL_CFM_RMEP_STATE_FAILED) {
        // Only update the time we entered this state if we are currently in
        // another.
        rmep_state_itr->second.status.state          = VTSS_APPL_CFM_RMEP_STATE_FAILED;
        rmep_state_itr->second.status.failed_ok_time = vtss::uptime_seconds();
    }
}

/******************************************************************************/
// CFM_CCM_rmep_no_loc()
// 20.20: Invoked when valid CCM is received (S/W-based) or no LOC is detected
// (H/W-based).
/******************************************************************************/
static void CFM_CCM_rmep_no_loc(cfm_rmep_state_itr_t rmep_state_itr)
{
    rmep_state_itr->second.rMEPCCMdefect = false;
    T_DG(CFM_TRACE_GRP_CCM, "RMEPID %u: No loss-of-continuity", rmep_state_itr->first);

    if (rmep_state_itr->second.status.state != VTSS_APPL_CFM_RMEP_STATE_OK) {
        // Only update the time we entered this state if we are currently in
        // another.
        rmep_state_itr->second.status.state          = VTSS_APPL_CFM_RMEP_STATE_OK;
        rmep_state_itr->second.status.failed_ok_time = vtss::uptime_seconds();
    }
}

/******************************************************************************/
// CFM_CCM_rMEPwhile_timeout()
// 20.20: Fires when Remote MEP state machine's rMEPwhile reaches 0.
// Corresponds to entering RMEP_FAILED state.
// Timer == rMEPwhile_timer.
/******************************************************************************/
static void CFM_CCM_rMEPwhile_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_t *mep_state = (cfm_mep_state_t *)context;
    cfm_rmep_state_itr_t rmep_state_itr;

    VTSS_ASSERT(mep_state);

    // Find correct RMEP. We can only locate it by its timer.
    for (rmep_state_itr = mep_state->rmep_states.begin(); rmep_state_itr != mep_state->rmep_states.end(); ++rmep_state_itr) {
        if (&rmep_state_itr->second.rMEPwhile_timer == &timer) {
            break;
        }
    }

    if (rmep_state_itr == mep_state->rmep_states.end()) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: Got rMEPwhile timeout on unknown timer", mep_state->key);
        return;
    }

    CFM_CCM_rmep_loc(rmep_state_itr);
    CFM_CCM_rx_defects_update(mep_state);
}

/******************************************************************************/
// CFM_CCM_frame_tx_cancel()
/******************************************************************************/
static mesa_rc CFM_CCM_frame_tx_cancel(cfm_mep_state_t *mep_state)
{
    mesa_rc rc = VTSS_RC_OK;

    // Stop Tx timer (in case of manual injection)
    cfm_timer_stop(mep_state->ccm_state.CCIwhile_timer);

    // Stop AFI (in case of AFI-supported injection)
    if (mep_state->ccm_state.afi_id != AFI_ID_NONE) {
        if ((rc = afi_single_free(mep_state->ccm_state.afi_id, nullptr)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CCM, "Unable to free AFI instance %u: %s", mep_state->ccm_state.afi_id, error_txt(rc));
        }
    }

    // Free the packet pointer
    if (mep_state->ccm_state.afi_conf.tx_props.packet_info.frm) {
        packet_tx_free(mep_state->ccm_state.afi_conf.tx_props.packet_info.frm);
        mep_state->ccm_state.afi_conf.tx_props.packet_info.frm = nullptr;
    }

    mep_state->ccm_state.afi_conf.tx_props.packet_info.len = 0;
    mep_state->ccm_state.afi_id = AFI_ID_NONE;
    return rc;
}

/******************************************************************************/
// CFM_CCM_frame_tx_start()
/******************************************************************************/
static mesa_rc CFM_CCM_frame_tx_start(cfm_mep_state_t *mep_state)
{
    cfm_ccm_state_t *ccm_state = &mep_state->ccm_state;
    mesa_rc         rc = VTSS_RC_OK;

    if (CFM_CCM_use_afi(mep_state)) {
        if ((rc = afi_single_alloc(&ccm_state->afi_conf, &ccm_state->afi_id)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CCM, "MEP %s: afi_single_alloc(" VPRI64u ") failed: %s", mep_state->key, ccm_state->afi_conf.params.fph, error_txt(rc));
            return VTSS_APPL_CFM_RC_HW_RESOURCES;
        }

        if ((rc = afi_single_pause_resume(ccm_state->afi_id, FALSE)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CCM, "Unable to start AFI flow with afi_id = %u: %s", ccm_state->afi_id, error_txt(rc));

            // Free the frame and afi_id.
            (void)CFM_CCM_frame_tx_cancel(mep_state);
            return rc;
        }
    } else {
        // Tx first frame ASAP according to 802.1Q clause 20.12.
        CFM_CCM_CCIwhile_timeout(ccm_state->CCIwhile_timer, mep_state);

        // And start the timer.
        // afi_conf->params.fph holds the number of frames per hour to send.
        // To get to an interval measured in milliseconds, we need to divide
        // the number of ms/hour by the fph.
        cfm_timer_start(ccm_state->CCIwhile_timer, 3600000 / ccm_state->afi_conf.params.fph, true);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// packet_tx_props_t::operator!=
// Cannot memcmp() the whole structure, since the packet_info.frm pointer may
// differ between the two structs.
/******************************************************************************/
static bool operator!=(const packet_tx_props_t &lhs, const packet_tx_props_t &rhs)
{
    if (memcmp(&lhs.tx_info, &rhs.tx_info, sizeof(lhs.tx_info)) != 0) {
        return true;
    }

    if (lhs.packet_info.len != rhs.packet_info.len) {
        return true;
    }

    return memcmp(lhs.packet_info.frm, rhs.packet_info.frm, lhs.packet_info.len) != 0;
}

/******************************************************************************/
// afi_single_conf_t::operator==
/******************************************************************************/
static bool operator==(const afi_single_conf_t &lhs, const afi_single_conf_t &rhs)
{
    // Use packet_tx_props_t::operator!=
    if (lhs.tx_props != rhs.tx_props) {
        return false;
    }

    return memcmp(&lhs.params, &rhs.params, sizeof(lhs.params)) == 0;
}

/******************************************************************************/
// CFM_CCM_tx_frame_update()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_frame_update(cfm_mep_state_t *mep_state)
{
    afi_single_conf_t new_afi_conf;
    uint32_t          new_afi_id = AFI_ID_NONE;
    mesa_bool_t       old_rdi;
    mesa_rc           rc;

    // Check to see if we need the VOE to generate RDI
    if (mep_state->global_state->has_vop) {
        if ((rc = mesa_voe_cc_rdi_get(nullptr, mep_state->voe_idx, &old_rdi)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_rdi_get(%u) failed: %s", mep_state->key, mep_state->voe_idx, error_txt(rc));
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        }

        if (old_rdi != mep_state->status.present_rdi) {
            T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Changing RDI from %d to %d", mep_state->key, old_rdi, mep_state->status.present_rdi);
            if ((rc = mesa_voe_cc_rdi_set(nullptr, mep_state->voe_idx, mep_state->status.present_rdi)) != VTSS_RC_OK) {
                T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_rdi_get(%u) failed: %s", mep_state->key, mep_state->voe_idx, error_txt(rc));
                return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
            }
        }
    }

    // Try to generate a new CCM frame
    VTSS_RC(CFM_CCM_tx_frame_generate(mep_state, &new_afi_conf));

    // See afi_single_conf_t::operator==
    if (new_afi_conf == mep_state->ccm_state.afi_conf) {
        // No change in CCM PDU. Keep the old one going and delete the new.
        T_DG(CFM_TRACE_GRP_CCM, "MEP %s: No change in CCM PDU", mep_state->key);

        packet_tx_free(new_afi_conf.tx_props.packet_info.frm);
        return VTSS_RC_OK;
    }

    // Cancel the old CCM frame (if any)
    (void)CFM_CCM_frame_tx_cancel(mep_state);

    T_DG(CFM_TRACE_GRP_FRAME_TX, "MEP %s: New CCM frame of length %u w/o FCS", mep_state->key, new_afi_conf.tx_props.packet_info.len);

    // Print the IFH.
    packet_debug_tx_props_print(VTSS_MODULE_ID_CFM, CFM_TRACE_GRP_FRAME_TX, VTSS_TRACE_LVL_DEBUG, &new_afi_conf.tx_props);

    // And the frame.
    T_DG_HEX(CFM_TRACE_GRP_FRAME_TX, new_afi_conf.tx_props.packet_info.frm, new_afi_conf.tx_props.packet_info.len);

    // Start the new
    mep_state->ccm_state.afi_id   = new_afi_id;
    mep_state->ccm_state.afi_conf = new_afi_conf;

    VTSS_RC(CFM_CCM_frame_tx_start(mep_state));

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_interval_to_mesa_period()
/******************************************************************************/
static mesa_voe_ccm_period_t CFM_CCM_interval_to_mesa_period(vtss_appl_cfm_ccm_interval_t ccm_interval)
{
    switch (ccm_interval) {
    case VTSS_APPL_CFM_CCM_INTERVAL_300HZ:
        return MESA_VOE_CCM_PERIOD_3_3_MS;

    case VTSS_APPL_CFM_CCM_INTERVAL_10MS:
        return MESA_VOE_CCM_PERIOD_10_MS;

    case VTSS_APPL_CFM_CCM_INTERVAL_100MS:
        return MESA_VOE_CCM_PERIOD_100_MS;

    case VTSS_APPL_CFM_CCM_INTERVAL_1S:
        return MESA_VOE_CCM_PERIOD_1_SEC;

    case VTSS_APPL_CFM_CCM_INTERVAL_10S:
        return MESA_VOE_CCM_PERIOD_10_SEC; // VOP_V2

    case VTSS_APPL_CFM_CCM_INTERVAL_1MIN:
    case VTSS_APPL_CFM_CCM_INTERVAL_10MIN:
    case VTSS_APPL_CFM_CCM_INTERVAL_INVALID:
    default:
        // MESA doesn't support the slowest rates.
        T_EG(CFM_TRACE_GRP_CCM, "Invalid or unsupported CCM interval (%d)", ccm_interval);
        return MESA_VOE_CCM_PERIOD_1_SEC;
    }
}

/******************************************************************************/
// CFM_CCM_rx_hw_voe_activate()
// Only invoked on platforms with a H/W VOE.
/******************************************************************************/
static mesa_rc CFM_CCM_rx_hw_voe_activate(cfm_mep_state_t *mep_state)
{
    cfm_rmep_state_itr_t rmep_state_itr;
    mesa_voe_cc_conf_t   old_conf, new_conf;
    mesa_rc              rc;

    if (mep_state->voe_event_mask) {
        // This MEP is supposed to use at least one H/W-based event. Enable it.
        T_DG(CFM_TRACE_GRP_CCM, "MEP = %s: mesa_voe_event_mask_set(%u, 0x%08x)", mep_state->key, mep_state->voe_idx, mep_state->voe_event_mask);
        if ((rc = mesa_voe_event_mask_set(nullptr, mep_state->voe_idx, mep_state->voe_event_mask, true)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_event_mask_set(%u, 0x%08x) failed: %s", mep_state->key, mep_state->voe_idx, mep_state->voe_event_mask, error_txt(rc));
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        }
    }

    // Configure VOE's CC part.
    if ((rc = mesa_voe_cc_conf_get(nullptr, mep_state->voe_idx, &old_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_conf_get() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    memset(&new_conf, 0, sizeof(new_conf));
    new_conf.enable            = true;
    new_conf.seq_no_update     = true;
    new_conf.expected_period   = CFM_CCM_interval_to_mesa_period(mep_state->ma_conf->ccm_interval);

    if (mep_state->rmep_states.size() > 1) {
        // We need all CCMs to the CPU, because the H/W only supports one Remote
        // MEP, and we have multiple.
        new_conf.cpu_copy = MESA_OAM_CPU_COPY_ALL;
    } else  if (mep_state->global_state->has_vop_v0 || mep_state->global_state->has_vop_v1) {
        // VOP_V0 and VOP_V1 only support MESA_OAM_CPU_COPY_NONE or
        // MESA_OAM_CPU_COPY_ALL.
        // We would have liked to get a snapshot from time to time (to be able
        // to update TLVs), using MESA_OAM_CPU_COPY_AUTO, but that's not
        // implemented in the API.
        // Instead, we rely on our own timer (started in
        // cfm_base.cxx#cfm_base_init()) to get CCMs to the CPU periodically.
        // When that timer expires, it invokes mesa_voe_cc_cpu_copy_next_set()
        // for each active VOE with only one RMEP. This causes the frames to
        // come to the CPU at a constant rate, so we don't need the VOE to do
        // anthing.
        new_conf.cpu_copy = MESA_OAM_CPU_COPY_NONE;
    } else {
        // VOP_V2 supports MESA_OAM_CPU_COPY_NONE, MESA_OAM_CPU_COPY_ALL,
        // and MESA_OAM_CPU_COPY_AUTO.
        // When configuring the VOP (cfm_base.cxx#cfm_base_init(), we already
        // took a stand on the rate with which various CCMs (invalid/valid/TLV)
        // come to the CPU when AUTO is chosen. Here we just need to pick AUTO.
        new_conf.cpu_copy = MESA_OAM_CPU_COPY_AUTO;
    }

    // RBNTBD: This is actually a CoSID and must be calculated based on ingress
    // configuration of what a tagged frame with PCP = mep_conf.pcp will hit of
    // CoSID or what default CosID an untagged frame will get. If not, we will
    // get a cPriority defect, which will not give rise to an alarm in 802.1Q,
    // but only in ITU-T.
    new_conf.expected_priority = mep_state->mep_conf->mep_conf.pcp;

    // MAID/MEGID
    memcpy(new_conf.expected_megid, mep_state->maid, sizeof(new_conf.expected_megid));

    // We must have at least one Remote MEP. If not, this function should never
    // have been invoked.
    if ((rmep_state_itr = mep_state->rmep_states.begin()) == mep_state->rmep_states.end()) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: Internal error: Invalid RMEP count (0)", mep_state->key);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    // Pick the first configured Remote MEP and use that as peer MEP-ID.
    // This is also fine if we have multiple Remote MEPs, because we don't
    // utilize VOE events in this case.
    new_conf.expected_peer_mepid = rmep_state_itr->first;

    if (memcmp(&new_conf, &old_conf, sizeof(new_conf)) == 0) {
        return VTSS_RC_OK;
    }

    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Invoking mesa_voe_cc_conf_set(%u)", mep_state->key, mep_state->voe_idx);
    if ((rc = mesa_voe_cc_conf_set(nullptr, mep_state->voe_idx, &new_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_conf_set() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_rx_hw_voe_deactivate()
// Only invoked on platforms with a H/W VOE.
/******************************************************************************/
static mesa_rc CFM_CCM_rx_hw_voe_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_voe_cc_conf_t conf;
    mesa_rc            rc;

    // Configure VOE's CC part.
    if (mep_state->voe_idx == MESA_VOE_IDX_NONE) {
        return VTSS_RC_OK;
    }

    if ((rc = mesa_voe_cc_conf_get(nullptr, mep_state->voe_idx, &conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_conf_get() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    if (!conf.enable) {
        return VTSS_RC_OK;
    }

    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Invoking mesa_voe_cc_conf_set(%u) to disable", mep_state->key, mep_state->voe_idx);
    if ((rc = mesa_voe_cc_conf_set(nullptr, mep_state->voe_idx, &conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_conf_set(%u) failed: %s", mep_state->key, mep_state->voe_idx, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_rx_voe_deactivate()
/******************************************************************************/
static mesa_rc CFM_CCM_rx_voe_deactivate(cfm_mep_state_t *mep_state)
{
    if (mep_state->global_state->has_vop) {
        VTSS_RC(CFM_CCM_rx_hw_voe_deactivate(mep_state));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_rx_voe_activate()
/******************************************************************************/
static mesa_rc CFM_CCM_rx_voe_activate(cfm_mep_state_t *mep_state)
{
    if (mep_state->global_state->has_vop) {
        VTSS_RC(CFM_CCM_rx_hw_voe_activate(mep_state));

        // Start a repeating timer that polls counters from the VOE once a
        // second.
        T_DG(CFM_TRACE_GRP_CCM, "Starting counter poll timer");
        cfm_timer_start(mep_state->ccm_state.rx_counter_poll_timer, 1000, true);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_rx_rmep_init()
/******************************************************************************/
static void CFM_CCM_rx_rmep_init(cfm_mep_state_t *mep_state)
{
    cfm_rmep_conf_itr_t  rmep_conf_itr;
    cfm_rmep_state_itr_t rmep_state_itr;
    bool                 hw_based_loc;

    // Stop existing RMEPs' timers.
    for (rmep_state_itr = mep_state->rmep_states.begin(); rmep_state_itr != mep_state->rmep_states.end(); ++rmep_state_itr) {
        cfm_timer_stop(rmep_state_itr->second.rMEPwhile_timer);
    }

    // Remove all entries.
    mep_state->rmep_states.clear();

    if (mep_state->mep_conf == nullptr) {
        // Tearing down.
        return;
    }

    hw_based_loc = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_LOC) != 0;

    // And create new corresponding to the RMEP configuration.
    for (rmep_conf_itr = mep_state->mep_conf->rmep_confs.begin(); rmep_conf_itr != mep_state->mep_conf->rmep_confs.end(); ++rmep_conf_itr) {
        if ((rmep_state_itr = mep_state->rmep_states.get(rmep_conf_itr->first.rmepid)) == mep_state->rmep_states.end()) {
            // Cannot return VTSS_APPL_CFM_RC_OUT_OF_MEMORY.
            continue;
        }

        memset(&rmep_state_itr->second, 0, sizeof(rmep_state_itr->second));

        // The RMEP_IDLE state cannot be reached, since there is an
        // unconditional transition to the RMEP_START state.
        rmep_state_itr->second.status.state = VTSS_APPL_CFM_RMEP_STATE_START;

        cfm_timer_init(rmep_state_itr->second.rMEPwhile_timer, "rMEPwhile", rmep_state_itr->first, CFM_CCM_rMEPwhile_timeout, mep_state);

        if (!hw_based_loc && mep_state->status.mep_active) {
            // We have no VOP or it doesn't support Loss-of-Continuity or we
            // have disabled H/W events because of multiple RMEPs, so there's
            // nothing else to drive the state machine than ourselves.

            // Start rMEPwhile_timer.
            cfm_timer_start(rmep_state_itr->second.rMEPwhile_timer, CFM_CCM_TIMEOUT_CALC(mep_state->fph), false);
        }
    }

    CFM_CCM_rx_defects_update(mep_state);
}

/******************************************************************************/
// CFM_CCM_state_reinit()
/******************************************************************************/
static void CFM_CCM_state_reinit(cfm_mep_state_t *mep_state)
{
    cfm_ccm_state_t *ccm_state = &mep_state->ccm_state;

    mep_state->status.fng_state      = VTSS_APPL_CFM_FNG_STATE_RESET;
    mep_state->status.highest_defect = VTSS_APPL_CFM_MEP_DEFECT_NONE;
    mep_state->status.defects        = 0;
    mep_state->status.present_rdi    = false;

    (void)cfm_ccm_statistics_clear(mep_state);
    CFM_CCM_clearFaultAlarm(mep_state);

    ccm_state->errorCCMdefect      = false;
    ccm_state->xconCCMdefect       = false;
    ccm_state->someRMEPCCMdefect   = false;
    ccm_state->someMACstatusDefect = false;
    ccm_state->someRDIdefect       = false;
    ccm_state->MAdefectIndication  = false;
    ccm_state->allRMEPsDead        = false;

    // Stop Rx timers.
    cfm_timer_stop(ccm_state->rx_counter_poll_timer);
    cfm_timer_stop(ccm_state->errorCCMwhile_timer);
    cfm_timer_stop(ccm_state->xconCCMwhile_timer);
    cfm_timer_stop(ccm_state->FNGwhile_timer);

    CFM_CCM_rx_rmep_init(mep_state);
}

/******************************************************************************/
// CFM_CCM_rx_xconCCMreceived_update()
// Returns true if a defect-update is needed.
/******************************************************************************/
static bool CFM_CCM_rx_xconCCMreceived_update(cfm_mep_state_t *mep_state, bool xconCCMreceived, uint64_t fph)
{
    uint32_t timeout_ms;

    // MEP Cross Connect state machine - one per MEP (20.24).
    // If using H/W, we only get changes to xconCCMreceived when the received
    // CCM's MEG ID or MEG Level actually changes, so we cannot start (extend)
    // the xconCCMwhile_timer when xconCCMreceived is true, because it will time
    // out while the CCM's MEG ID or MEG level are still invalid.
    // If using S/W, we get all CCM PDUs to the CPU and we should extend the
    // xconCCMwhile_timer as long as xconCCMreceived is true.
    // However, this corresponds to starting the timer when the xconCCMreceived
    // transitions from a 1 to 0.
    // We therefore check this transition here.
    if (mep_state->ccm_state.xconCCMdefect && !xconCCMreceived) {
        timeout_ms = CFM_CCM_TIMEOUT_CALC(fph);
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s, xconCCMdefect transitions from 1 to 0. Starting xconCCMwhile_timer with timeout_ms = %u", mep_state->key, timeout_ms);
        cfm_timer_start(mep_state->ccm_state.xconCCMwhile_timer, timeout_ms, false);
        mep_state->ccm_state.xconCCMdefect = false;
        return true;
    } else if (!mep_state->ccm_state.xconCCMdefect && xconCCMreceived) {
        // Transition from 0 to 1. Stop any ongoing xconCCMwhile_timer.
        cfm_timer_stop(mep_state->ccm_state.xconCCMwhile_timer);
        mep_state->ccm_state.xconCCMdefect = true;
        return true;
    }

    // No change in xconCCMdefect state.
    return false;
}

/******************************************************************************/
// CFM_CCM_rx_errorCCMreceived_update()
// Returns true if a defect update is neeeded.
/******************************************************************************/
static bool CFM_CCM_rx_errorCCMreceived_update(cfm_mep_state_t *mep_state, bool errorCCMreceived, uint64_t fph)
{
    uint32_t timeout_ms;

    // Remote MEP Error state machine - one per MEP (20.22).
    // If using H/W, we only get changes to errorCCMreceived when the received
    // CCM's MEP ID or CCM Interval actually changes, so we cannot start
    // (extend) the errorCCMwhile_timer when errorCCMreceived is true, because
    // it will time out while the CCM's MEP ID or CCM interval are still
    // invalid.
    // If using S/W, we get all CCM PDUs to the CPU and we should extend the
    // errorCCMwhile_timer as long as errorCCMreceived is true.
    // However, this corresponds to starting the timer when the errorCCMreceived
    // transitions from a 1 to 0.
    // We therefore check this transition here.
    if (mep_state->ccm_state.errorCCMdefect && !errorCCMreceived) {
        timeout_ms = CFM_CCM_TIMEOUT_CALC(fph);
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s, errorCCMdefect transitions from 1 to 0. Starting errorCCMwhile_timer with timeout_ms = %u", mep_state->key, timeout_ms);
        cfm_timer_start(mep_state->ccm_state.errorCCMwhile_timer, timeout_ms, false);
        mep_state->ccm_state.errorCCMdefect = false;
        return true;
    } else if (!mep_state->ccm_state.errorCCMdefect && errorCCMreceived) {
        // Transition from 0 to 1. Stop any ongoing errorCCMwhile_timer.
        cfm_timer_stop(mep_state->ccm_state.errorCCMwhile_timer);
        mep_state->ccm_state.errorCCMdefect = true;
        return true;
    }

    return false;
}

/******************************************************************************/
// CFM_CCM_rx_rmep_sm_run()
// Invoked whenever H/W tells us there is an event on this MEP.
// Will only be invoked if the number of RMEPs is one, since multiple RMEPs are
// handled entirely by S/W.
/******************************************************************************/
static void CFM_CCM_rx_rmep_sm_run(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change)
{
    vtss_appl_cfm_rmep_status_t *rmep_status;
    cfm_rmep_state_itr_t        rmep_state_itr;
    mesa_voe_cc_status_t        ccm_status = {};
    bool                        xconCCMreceived = false, errorCCMreceived = false;
    bool                        hw_based_meg, hw_based_mel, hw_based_mepid, hw_based_interval, hw_based_loc;
    mesa_rc                     rc;

    if ((rc = mesa_voe_cc_status_get(nullptr, mep_state->voe_idx, &ccm_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CCM, "mesa_voe_cc_status_get(%u) failed: %s", mep_state->voe_idx, error_txt(rc));
        return;
    }

    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: change = %s:\n %s", mep_state->key, cfm_util_mep_state_change_to_str(change), ccm_status);

    if (mep_state->rmep_states.size() != 1) {
        T_EG(CFM_TRACE_GRP_CCM, "MEP %s. Number of RMEPs is %zu, but still got an event", mep_state->key, mep_state->rmep_states.size());
        return;
    }

    if (ccm_status.zero_period) {
        // 20.17.1.a: Invalid frame. Counted by H/W
        return;
    }

    hw_based_mel      = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEG_LEVEL) != 0;
    hw_based_meg      = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEG_ID)    != 0;
    hw_based_mepid    = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEP_ID)    != 0;
    hw_based_interval = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_PERIOD)    != 0;
    hw_based_loc      = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_LOC)       != 0;

    // 20.17.2.b. Check level (if we're using H/W for doing that)
    if (hw_based_mel && ccm_status.mel_unexp) {
        xconCCMreceived = true;
        goto do_exit;
    }

    // 20.17.1.b. Check MAID (if we're using H/W for doing that)
    if (hw_based_meg && ccm_status.meg_id_unexp) {
        xconCCMreceived = true;
        goto do_exit;
    }

    // 20.17.1.c.1+2. Check MEP-ID
    if (hw_based_mepid && ccm_status.mep_id_unexp) {
        // Remote MEP-ID doesn't match the configured RMEP's MEP-ID.
        errorCCMreceived = true;
        goto do_exit;
    }

    // 20.17.1.c.3. Check CCM interval
    if (hw_based_interval && ccm_status.period_unexp) {
        errorCCMreceived = true;
        goto do_exit;
    }

    if (!mep_state->status.errors.enableRmepDefect) {
        // The Remote MEP state machine (20.20) must not run if enableRmepDefect
        // is false.
        T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Skipping RMEP SM, because enableRmepDefect is false", mep_state->key);
        return;
    }

    // Remote MEP state machine - one per RMEP (20.20).
    // We know that there is exactly one RMEP (checked above).
    rmep_state_itr = mep_state->rmep_states.begin();
    rmep_status = &rmep_state_itr->second.status;

    if (hw_based_loc) {
        if (ccm_status.loc) {
            // rMEPwhile H/W timer timed out.
            CFM_CCM_rmep_loc(rmep_state_itr);
        } else {
            // Still going strong
            CFM_CCM_rmep_no_loc(rmep_state_itr);
        }
    }

    // The following fields may be updated by H/W events.
    if (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_RX_RDI) {
        rmep_status->rdi = ccm_status.rdi; // rMEPlastRDI
    }

    // We don't use the CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS
    // and CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS events, even if
    // they are supported by the H/W, because we don't get events when these
    // TLVs disappear from the CCM PDU.

do_exit:
    if (hw_based_mel || hw_based_meg) {
        // MEP Cross Connect state machine - one per MEP (20.24).
        // According to 20.22 (Error SM) and 20.24 (Cross Connect SM), the
        // timeout must be 3.5 times the CCM interval received in the erroneous
        // CCM PDU, but since we don't have that, we set the timeout to 3.5
        // times the CCM interval we are configured with (which is stored in
        // mep_state->fph).
        (void)CFM_CCM_rx_xconCCMreceived_update(mep_state, xconCCMreceived, mep_state->fph);
    }

    if (hw_based_mepid || hw_based_interval) {
        // Remote MEP Error state machine - one per MEP (20.22).
        // According to 20.22 (Error SM), the timeout must be 3.5 times the
        // CCM interval received in the erroneous CCM PDU, but since we don't
        // have that, we set the timeout to 3.5 times the CCM interval we are
        // configured with (which is stored in mep_state->fph).
        (void)CFM_CCM_rx_errorCCMreceived_update(mep_state, errorCCMreceived, mep_state->fph);
    }

    CFM_CCM_rx_defects_update(mep_state);
}

/******************************************************************************/
// CFM_CCM_rx_deactivate()
/******************************************************************************/
static mesa_rc CFM_CCM_rx_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    if ((rc = CFM_CCM_rx_voe_deactivate(mep_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    // Reset state.
    CFM_CCM_state_reinit(mep_state);

    if (!mep_state->mep_conf) {
        // Remove this MEP's entry from the notification table.
        T_DG(CFM_TRACE_GRP_NOTIF, "MEP %s: Deleting entry", mep_state->key);
        if ((rc = cfm_mep_notification_status.del(mep_state->key)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to delete fault alarm notification table entry: %s", mep_state->key, error_txt(rc));
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_CCM_rx_activate()
// mep_state->status.mep_active is guaranteed to be true when this function is
// invoked.
/******************************************************************************/
static mesa_rc CFM_CCM_rx_activate(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change)
{
    bool run_rmep_update  = false;
    bool run_conf_update  = false;
    bool run_state_update = false;
    bool reset_state      = false;

    switch (change) {
    case CFM_MEP_STATE_CHANGE_CONF_NO_RESET:
    case CFM_MEP_STATE_CHANGE_PVID_OR_ACCEPTABLE_FRAME_TYPE:
        run_conf_update = true;
        break;

    case CFM_MEP_STATE_CHANGE_ENABLE_RMEP_DEFECT:
        // If a change in enableRmepDefect has occurred, and the new state is
        // false, we must disable the Remote MEP State Machines.
        run_rmep_update = true;
        break;

    case CFM_MEP_STATE_CHANGE_CONF:
    case CFM_MEP_STATE_CHANGE_CONF_RMEP:
        run_conf_update = true;
        reset_state     = true;
        break;

    case CFM_MEP_STATE_CHANGE_TPID:
    case CFM_MEP_STATE_CHANGE_IP_ADDR:
    case CFM_MEP_STATE_CHANGE_PORT_TYPE:
        // Changes to these do not affect Rx
        break;

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PERIOD:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LOC:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MEPID:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MAID:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LEVEL:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_RDI:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS:
        run_state_update = true;
        break;

    default:
        T_EG(CFM_TRACE_GRP_CCM, "Unsupported state change enum (%d = %s)", change, cfm_util_mep_state_change_to_str(change));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    T_DG(CFM_TRACE_GRP_CCM, "MEP = %s: change = %s: run_conf_update = %d, run_rmep_update = %d, reset_state = %d", mep_state->key, cfm_util_mep_state_change_to_str(change), run_conf_update, run_rmep_update, reset_state);

    if (run_conf_update || run_rmep_update) {
        // Compute mep_state->fph from ma_conf->ccm_interval
        VTSS_RC(CFM_CCM_interval_to_fph(mep_state, mep_state->ma_conf->ccm_interval, mep_state->fph, true));

        // Also update our state
        run_state_update = true;

        if (run_conf_update) {
            if (reset_state) {
                // Re-start whole MEP.
                CFM_CCM_state_reinit(mep_state);
            }

            // Create our MAID (a.k.a. MEGID)
            VTSS_RC(CFM_CCM_maid_create(mep_state));

            // (Re-)configure VOE
            VTSS_RC(CFM_CCM_rx_voe_activate(mep_state));
        } else {
            // Re-start all RMEPs.
            CFM_CCM_rx_rmep_init(mep_state);
        }
    }

    if (mep_state->global_state->has_vop && mep_state->rmep_states.size() == 1 && run_state_update) {
        // Read the latest status from H/W when the chip supports it and we have
        // exactly one Remote MEP (because if we had more than one, we would not
        // use the VOE to get status, because it only supports exactly one).
        CFM_CCM_rx_rmep_sm_run(mep_state, change);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_tx_deactivate()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    // Stop CCM
    if ((rc = CFM_CCM_frame_tx_cancel(mep_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_CCM_tx_activate()
/******************************************************************************/
static mesa_rc CFM_CCM_tx_activate(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change)
{
    bool run_update = false;

    switch (change) {
    case CFM_MEP_STATE_CHANGE_CONF:
    case CFM_MEP_STATE_CHANGE_CONF_NO_RESET:
    case CFM_MEP_STATE_CHANGE_CONF_RMEP:
    case CFM_MEP_STATE_CHANGE_ENABLE_RMEP_DEFECT:
    case CFM_MEP_STATE_CHANGE_TPID:
    case CFM_MEP_STATE_CHANGE_IP_ADDR:
    case CFM_MEP_STATE_CHANGE_PVID_OR_ACCEPTABLE_FRAME_TYPE: // PVID may cause a change in Tx'd frame on Caracal
        run_update = true;
        break;

    case CFM_MEP_STATE_CHANGE_PORT_TYPE:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PERIOD:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LOC:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MEPID:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MAID:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LEVEL:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_RDI:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS:
        // Changes to these do not affect the frame we are sending.
        break;

    default:
        T_EG(CFM_TRACE_GRP_CCM, "Unsupported state change enum (%d = %s)", change, cfm_util_mep_state_change_to_str(change));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    if (!run_update) {
        return VTSS_RC_OK;
    }

    // Update/create a frame
    T_DG(CFM_TRACE_GRP_CCM, "MEP %s: Frame update due to %s", mep_state->key, cfm_util_mep_state_change_to_str(change));
    VTSS_RC(CFM_CCM_tx_frame_update(mep_state));

    return VTSS_RC_OK;
}

/******************************************************************************/
// cfm_ccm_update()
/******************************************************************************/
mesa_rc cfm_ccm_update(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    if (mep_state->status.mep_active) {
        // Operational up.

        // Activate CCM Rx
        VTSS_RC(CFM_CCM_rx_activate(mep_state, change));

        if (mep_state->mep_conf->mep_conf.ccm_enable) {
            // CCM Tx is enabled. Activate Tx.
            VTSS_RC(CFM_CCM_tx_activate(mep_state, change));
        } else {
            // CCM Tx is disabled. Deactivate.
            VTSS_RC(CFM_CCM_tx_deactivate(mep_state));
        }
    } else {
        // Must go through all deactivation steps.
        if ((rc = CFM_CCM_tx_deactivate(mep_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        if ((rc = CFM_CCM_rx_deactivate(mep_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// cfm_ccm_statistics_clear()
/******************************************************************************/
mesa_rc cfm_ccm_statistics_clear(cfm_mep_state_t *mep_state)
{
    mesa_rc rc;

    mep_state->status.ccm_rx_valid_cnt          = 0;
    mep_state->status.ccm_rx_invalid_cnt        = 0;
    mep_state->status.ccm_rx_sequence_error_cnt = 0;
    mep_state->status.ccm_tx_cnt                = 0;

    if (mep_state->global_state->has_vop && mep_state->voe_idx != MESA_VOE_IDX_NONE) {
        // Also clear H/W counters
        if ((rc = mesa_voe_cc_counters_clear(nullptr, mep_state->voe_idx)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CCM, "MEP %s: mesa_voe_cc_counters_clear(%u) failed: %s", mep_state->key, mep_state->voe_idx, error_txt(rc));
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_CCM_rx_validation_pass()
// 20.51.4.2: What not to validate
// 20.51.4.3: What to validate
/******************************************************************************/
static bool CFM_CCM_rx_validation_pass(cfm_mep_state_t *mep_state, const uint8_t *const frm, uint32_t rx_length,
                                       vtss_appl_cfm_rmep_status_t  &rmep_status,
                                       vtss_appl_cfm_ccm_interval_t &ccm_interval,
                                       uint32_t                     &sequence_number,
                                       vtss_appl_cfm_mepid_t        &mepid,
                                       uint32_t                     &maid_offset)
{
    uint32_t offset = CFM_CCM_PDU_START_OFFSET, tlv_len, first_tlv_offset;
    uint8_t  tlv_type;

    memset(&rmep_status, 0, sizeof(rmep_status));

    if (rx_length < CFM_CCM_PDU_START_OFFSET + 4) {
        // Can't even hold the first_tlv_offset.
        return false;
    }

    // SMAC
    memcpy(rmep_status.smac.addr, &frm[6], sizeof(rmep_status.smac.addr));

    //
    // CFM header
    //

    // MD level + Version. We must not check the version field, since we only
    // support version 0. Level has already been determined.
    offset++;

    // OpCode. Already checked to be CCM
    offset++;

    // CCM flags (RDI (1 bit), Reserved (4 bits), CCM Interval (3 bits)
    rmep_status.rdi = frm[offset] >> 7;
    ccm_interval    = (vtss_appl_cfm_ccm_interval_t)(frm[offset++] & 0x7);
    if (ccm_interval == VTSS_APPL_CFM_CCM_INTERVAL_INVALID) {
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Invalid CCM interval received", mep_state->key);
        return false;
    }

    // 20.51.4.3.a: First TLV Offset. Must be >= 4 + 2 + 48 + 16 = 70
    if ((first_tlv_offset = frm[offset++]) < 70) {
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Invalid First TLV Offset (%u)", mep_state->key, first_tlv_offset);
        return false;
    }

    sequence_number = frm[offset + 0] << 24 |
                      frm[offset + 1] << 16 |
                      frm[offset + 2] <<  8 |
                      frm[offset + 3] <<  0;

    // Caller wants to know the MEP-ID
    mepid = frm[offset + 4] << 8 | frm[offset + 5];

    // Caller wants to know the offset of the MAID
    maid_offset = offset + 6;

    // offset is now pointing to first byte after First TLV Offset, which is
    // where first_tlv_offset is counting from. Skip past everything until the
    // first TLV.
    offset += first_tlv_offset;

    // 20.51.4.3.b: The fixed length header does not overrun the length of the
    // PDU. Notice that the End TLV may be missing (according to 20.51.4.2.e),
    // so do not assume it's there.
    if (rx_length < offset) {
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Length (%u) < offset (%u)", mep_state->key, rx_length, offset);
        return false;
    }

    // Loop through TLVs
    while (offset < rx_length) {
        tlv_type = frm[offset++];
        if (tlv_type == 0) {
            // End TLV.
            // 20.51.5.d says: Do not process anything beyond an End TLV.
            break;
        }

        // Get length of this TLV. This only includes what comes after this
        // field.
        tlv_len  = (frm[offset + 0] << 8) | (frm[offset + 1] << 0);
        offset += 2;

        switch (tlv_type) {
        case 1:
            // Sender ID TLV (21.5.3)
            if (!CFM_CCM_sender_id_tlv_get(mep_state, frm, offset, tlv_len, rmep_status.sender_id)) {
                return false;
            }

            // Done
            break;

        case 2:
            // Port Status TLV (21.5.4)
            if (tlv_len != 1) {
                // Must be exactly 1 long
                T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Port Status TLV's length (%u) != 1", mep_state->key, tlv_len);
                return false;
            }

            // Validation test: Must contain one of the values from Table 21-9.
            rmep_status.port_status = (vtss_appl_cfm_port_status_t)frm[offset];
            if (frm[offset] != VTSS_APPL_CFM_PORT_STATUS_BLOCKED && frm[offset] != VTSS_APPL_CFM_PORT_STATUS_UP) {
                T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Port Status TLV's contents not valid (%u)", mep_state->key, frm[offset]);
                return false;
            }

            break;

        case 4:
            // Interface Status TLV (21.5.5)
            if (tlv_len != 1)  {
                // Must be exactly 1 byte long
                T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Interface Status TLV's length (%u) != 1", mep_state->key, tlv_len);
                return false;
            }

            // Validation test: Must contain one of the values from Table 21-11.
            rmep_status.interface_status = (vtss_appl_cfm_interface_status_t)frm[offset];
            if (frm[offset] < VTSS_APPL_CFM_INTERFACE_STATUS_UP || frm[offset] > VTSS_APPL_CFM_INTERFACE_STATUS_LOWER_LAYER_DOWN) {
                T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Interface Status TLV's contents not valid (%u)", mep_state->key, frm[offset]);
                return false;
            }

            break;

        case 31:
            // Organization-Specific TLV (21.5.2)
            if (!CFM_CCM_organization_specific_tlv_get(mep_state, frm, offset, tlv_len, rmep_status.organization_specific_tlv))  {
                return false;
            }

            rmep_status.organization_specific_tlv_present = true;
            break;

        default:
            // Don't care about contents, but skip over lengths.
            break;
        }

        // Skip past this TLV. offset now points to the Type field of the next
        // TLV - if any
        offset += tlv_len;

        if (offset > rx_length) {
            // The frame was not long enough to accommodate this TLV's length
            // field.
            T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: TLV of type %u with length = %u caused frame to go beyond the frame length (%u)", mep_state->key, tlv_type, tlv_len, rx_length);
            return false;
        }
    }

    return true;
}

/******************************************************************************/
// cfm_ccm_rx_frame()
// Only invoked if frame's Level <= our level and mep_active is true.
// If we are running w/o a VOP or if we have multiple Remote MEPs, we get
// invoked everytime the chip receives a CCM.
// If we run with a VOP and the chip only has one Remote MEP, we only getinvoked
// every three seconds per VOE.
/******************************************************************************/
void cfm_ccm_rx_frame(cfm_mep_state_t *mep_state, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    uint8_t                      level = frm[CFM_CCM_PDU_START_OFFSET] >> 5;
    uint64_t                     fph;
    bool                         xconCCMreceived = false, errorCCMreceived = false, defect_update = false;
    bool                         hw_based_mel, hw_based_meg, hw_based_mepid, hw_based_interval, hw_based_loc;
    vtss_appl_cfm_mepid_t        rmepid;
    vtss_appl_cfm_rmep_status_t  new_rmep_status, *rmep_status;
    vtss_appl_cfm_ccm_interval_t ccm_interval;
    cfm_rmep_state_itr_t         rmep_state_itr;
    uint32_t                     sequence_number, maid_offset;

    // The Active Level Demultiplexer has already discarded CCM PDUs with level
    // greater than our level, so there's only equal or lower levels left.
    // The first parts of 20.17.1 (Equal) and 20.17.2 (Low) are identical, so
    // let's handle them identically.

    T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Frame Rx", mep_state->key);

    // 20.17.1.a and 20.17.2.a
    if (!CFM_CCM_rx_validation_pass(mep_state, frm, rx_info->length, new_rmep_status, ccm_interval, sequence_number, rmepid, maid_offset)) {
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: Frame failed validation", mep_state->key);
        if (!mep_state->global_state->has_vop) {
            // Only update the invalid counter if we don't rely on H/W counters.
            mep_state->status.ccm_rx_invalid_cnt++;
        }

        return;
    }

    // Convert recvdInterval to frames/hour for later usage.
    if (CFM_CCM_interval_to_fph(mep_state, ccm_interval, fph, false) != VTSS_RC_OK) {
        if (!mep_state->global_state->has_vop) {
            // Only update the invalid counter if we don't rely on H/W counters.
            mep_state->status.ccm_rx_invalid_cnt++;
        }

        return;
    }

    hw_based_mel      = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEG_LEVEL) != 0;
    hw_based_meg      = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEG_ID)    != 0;
    hw_based_mepid    = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEP_ID)    != 0;
    hw_based_interval = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_PERIOD)    != 0;
    hw_based_loc      = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_LOC)       != 0;

    if (level < mep_state->md_conf->level) {
        if (!mep_state->global_state->has_vop) {
            // Only update the invalid counter if we don't rely on H/W counters.
            mep_state->status.ccm_rx_invalid_cnt++;
        }

        if (hw_based_mel) {
            // The H/W gives interrupts when a change in MEG/MD Level occurs, so
            // nothing else to do.
            return;
        } else {
            // 20.17.2.b
            xconCCMreceived = true;

            // Low level processing done.
            goto do_exit;
        }
    }

    // For equal level, we need to go a bit further.
    // 20.17.1.b: Check MAID.
    if (memcmp(&frm[maid_offset], mep_state->maid, sizeof(mep_state->maid)) != 0) {
        if (!mep_state->global_state->has_vop) {
            // Only update the invalid counter if we don't rely on H/W counters.
            mep_state->status.ccm_rx_invalid_cnt++;
        }

        if (hw_based_meg) {
            // The H/W gives interrupts when a change in MEGID/MAID occurs, so
            // nothing else to do.
            return;
        } else {
            T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: MAIDs don't match", mep_state->key);
            xconCCMreceived = true;
            goto do_exit;
        }
    }

    // 20.17.1.c.1+2
    // Lookup remote MEP-ID amongst the configured RMEPs.
    if ((rmep_state_itr = mep_state->rmep_states.find(rmepid)) == mep_state->rmep_states.end()) {
        if (!mep_state->global_state->has_vop) {
            // Only update the invalid counter if we don't rely on H/W counters.
            mep_state->status.ccm_rx_invalid_cnt++;
        }

        if (hw_based_mepid) {
            // The H/W gives interrupts when a change in MEP-ID occurs, so
            // nothing else to do.
            return;
        } else {
            // 20.17.1.c.1+2
            // Remote MEP-ID was not amongst configured RMEPs (our own MEP-ID
            // cannot be one of the Remote MEPIDs (checked by cfm.cxx)).
            T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: RMEP (%u) not found", mep_state->key, rmepid);
            errorCCMreceived = true;
            goto do_exit;
        }
    }

    // 20.17.1.c.3
    // If CCM interval in frame doesn't match the configured CCM interval, we
    // kick the error CCM SM.
    if (ccm_interval != mep_state->ma_conf->ccm_interval) {
        if (!mep_state->global_state->has_vop) {
            // Only update the invalid counter if we don't rely on H/W counters.
            mep_state->status.ccm_rx_invalid_cnt++;
        }

        if (hw_based_interval) {
            // The H/W gives interrupts when a change in CCM interval occurs, so
            // nothing else to do.
            return;
        } else {
            T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s: CCM interval in frame (%u) doesn't match configured CCM interval (%u)", mep_state->key, ccm_interval, mep_state->ma_conf->ccm_interval);
            errorCCMreceived  = true;
            goto do_exit;
        }
    }

    if (!mep_state->status.errors.enableRmepDefect) {
        // The Remote MEP state machine (20.20) must not run if enableRmepDefect
        // is false.
        return;
    }

    // The CCM is valid. Count it unless it's already counted by H/W.
    if (!mep_state->global_state->has_vop) {
        // Only update the invalid counter if we don't rely on H/W counters.
        mep_state->status.ccm_rx_valid_cnt++;
    }

    T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s, Rx valid frame from RMEPID = %u", mep_state->key, rmepid);

    // Remote MEP state machine - one per RMEP (20.20).
    // 20.17.1.e-g
    // I think it's a bug in the 802.1ag spec that the CCMsequenceErrors
    // may count even if the Remote MEP is currently disabled. According to
    // the standard, CCMsequenceErrors may increase if a valid CCM is
    // received even if the Remote MEP SM is currently disabled.
    // I have chosen to move that check into the SM itself rather than
    // having it outside.
    if (!mep_state->global_state->has_vop) {
        // Only update the sequence error counter if we don't rely on H/W
        // counters.
        if (sequence_number && rmep_state_itr->second.sequence_number) {
            // Both the new and the previous sequence numbers are non-zero.
            // Compare them.
            if (sequence_number != rmep_state_itr->second.sequence_number + 1) {
                mep_state->status.ccm_rx_sequence_error_cnt++;
            }
        }
    }

    rmep_state_itr->second.sequence_number = sequence_number;
    rmep_status = &rmep_state_itr->second.status;

    if (!hw_based_loc) {
        // The VOE doesn't support Loss-of-Continuity, so there's nothing else
        // to drive the state machine than ourselves.
        CFM_CCM_rmep_no_loc(rmep_state_itr);

        // (Re-)start rMEPwhile_timer.
        cfm_timer_start(rmep_state_itr->second.rMEPwhile_timer, CFM_CCM_TIMEOUT_CALC(fph), false);
    }

    // Copy selected fields into the RMEP's public status. These are always
    // updated, because H/W cannot do that for us.
    rmep_status->smac                              = new_rmep_status.smac;                              // rMEPmacAddress
    rmep_status->sender_id                         = new_rmep_status.sender_id;                         // rMEPlastSenderId
    rmep_status->organization_specific_tlv_present = new_rmep_status.organization_specific_tlv_present; // Not part of official rMEP status.
    rmep_status->organization_specific_tlv         = new_rmep_status.organization_specific_tlv;         // Not part of official rMEP status.

    if (!(mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_TLV_PORT_STATUS)) {
        rmep_status->port_status = new_rmep_status.port_status; // rMEPlastPortState
        rmep_state_itr->second.rMEPportStatusDefect = rmep_status->port_status != VTSS_APPL_CFM_PORT_STATUS_NOT_RECEIVED && rmep_status->port_status != VTSS_APPL_CFM_PORT_STATUS_UP;
        defect_update = true;
    }

    if (!(mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_TLV_IF_STATUS)) {
        rmep_status->interface_status = new_rmep_status.interface_status; // rMEPlastInterfaceStatus
        rmep_state_itr->second.rMEPinterfaceStatusDefect = rmep_status->interface_status != VTSS_APPL_CFM_INTERFACE_STATUS_NOT_RECEIVED && rmep_status->interface_status != VTSS_APPL_CFM_INTERFACE_STATUS_UP;
        defect_update = true;
    }

    // The following fields may be updated by H/W events.
    if (!(mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_RX_RDI)) {
        rmep_status->rdi = new_rmep_status.rdi; // rMEPlastRDI
        defect_update = true;
    }

do_exit:
    if (!hw_based_meg && !hw_based_mel) {
        // MEP Cross Connect state machine - one per MEP (20.24).
        // We can only get here, if the H/W doesn't give us interrupts on this.
        if (CFM_CCM_rx_xconCCMreceived_update(mep_state, xconCCMreceived, fph)) {
            defect_update = true;
        }
    }

    if (!hw_based_mepid && !hw_based_interval) {
        // Remote MEP Error state machine - one per MEP (20.22).
        // We can only get here, if the H/W doesn't give us interrupts on this.
        if (CFM_CCM_rx_errorCCMreceived_update(mep_state, errorCCMreceived, fph)) {
            defect_update = true;
        }
    }

    if (defect_update) {
        CFM_CCM_rx_defects_update(mep_state);
    }
}

/******************************************************************************/
// cfm_ccm_state_init()
/******************************************************************************/
mesa_rc cfm_ccm_state_init(cfm_mep_state_t *mep_state)
{
    mep_state->ccm_state.afi_id = AFI_ID_NONE;
    cfm_timer_init(mep_state->ccm_state.rx_counter_poll_timer, "CCM Rx Counter Poll", mep_state->key.mepid, CFM_CCM_rx_counter_poll_timeout, mep_state);
    cfm_timer_init(mep_state->ccm_state.CCIwhile_timer,        "CCIwhile",            mep_state->key.mepid, CFM_CCM_CCIwhile_timeout,        mep_state);
    cfm_timer_init(mep_state->ccm_state.FNGwhile_timer,        "FNGwhile",            mep_state->key.mepid, CFM_CCM_FNGwhile_timeout,        mep_state);
    cfm_timer_init(mep_state->ccm_state.xconCCMwhile_timer,    "xconCCMwhile",        mep_state->key.mepid, CFM_CCM_xcon_timeout,            mep_state);
    cfm_timer_init(mep_state->ccm_state.errorCCMwhile_timer,   "errorCCMwhile",       mep_state->key.mepid, CFM_CCM_error_timeout,           mep_state);

    mep_state->status.fng_state      = VTSS_APPL_CFM_FNG_STATE_RESET;
    mep_state->status.highest_defect = VTSS_APPL_CFM_MEP_DEFECT_NONE;

    // Create an entry in the global notification status table.
    CFM_CCM_notif_entry_create(mep_state);

    return VTSS_RC_OK;
}

