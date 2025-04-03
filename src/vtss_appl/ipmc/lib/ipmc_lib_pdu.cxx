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

#include "main.h"
#include "ipmc_lib.hxx"      /* For ipmc_lib_vlan_if_ipv4_get()     */
#include "ipmc_lib_pdu.hxx"  /* Ourselves                           */
#include "ipmc_lib_base.hxx" /* For all the types we use            */
#include "conf_api.h"        /* For conf_mgmt_mac_addr_get()        */
#include "ip_utils.hxx"      /* For vtss_ipv6_addr_link_local_get() */
#include "mac_utils.hxx"     /* For mesa_mac_t::operator!=()        */
#include "mgmt_api.h"
#include "ipmc_lib_trace.h"
#include "packet_api.h"     /* For ETYPE_IPV4/ETYPE_IPV6           */

// Both IPMC and MVR are Tx'ing from the same thread, or at least with the
// IPMC_LIB's mutex taken (IPMC_LIB_LOCK_SCOPE), so only one global instance of
// a Tx PDU is needed.
static ipmc_lib_pdu_t IPMC_LIB_PDU_to_tx;

/******************************************************************************/
// Other stuff
/******************************************************************************/
static       uint8_t                 IPMC_LIB_PDU_tx_buf_ipmc[IPMC_LIB_PDU_FRAME_SIZE_MAX];
static       uint8_t                 IPMC_LIB_PDU_tx_buf_mvr[IPMC_LIB_PDU_FRAME_SIZE_MAX];
static       mesa_mac_t              IPMC_LIB_PDU_system_mac;
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_link_local_ipv6;
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_all_nodes_ipv4;     // 224.0.0.1
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_all_nodes_ipv6;     // ff02::1
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_all_routers_ipv4;   // 224.0.0.2
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_all_routers_ipv6;   // ff02::2
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_all_routers_igmpv3; // 224.0.0.22
static       vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_all_routers_mldv2;  // ff02::16
static const mesa_mac_t              IPMC_LIB_PDU_all_nodes_mac_ipv4     = {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x01}};
static const mesa_mac_t              IPMC_LIB_PDU_all_nodes_mac_ipv6     = {{0x33, 0x33, 0x00, 0x00, 0x00, 0x01}};
static const mesa_mac_t              IPMC_LIB_PDU_all_routers_mac_ipv4   = {{0x01, 0x00, 0x5E, 0x00, 0x00, 0x02}};
static const mesa_mac_t              IPMC_LIB_PDU_all_routers_mac_ipv6   = {{0x33, 0x33, 0x00, 0x00, 0x00, 0x02}};
static const mesa_mac_t              IPMC_LIB_PDU_all_routers_mac_igmpv3 = {{0x01, 0x00, 0x5E, 0x00, 0x00, 0x16}};
static const mesa_mac_t              IPMC_LIB_PDU_all_routers_mac_mldv2  = {{0x33, 0x33, 0x00, 0x00, 0x00, 0x16}};

#define IPMC_LIB_PDU_IPV4_VERSION_VALUE 4
#define IPMC_LIB_PDU_IPV6_VERSION_VALUE 6
#define IPMC_IPHDR_HOPLIMIT             1

// RFC2113, 2.1
#define IPMC_IPV4_ROUTER_ALERT_TYPE  0x9404
#define IPMC_IPV4_ROUTER_ALERT_VALUE 0x0000

#define IPMC_IPV4_RTR_ALERT_PREFIX1 0x94 /* 1 0 0 1 0 1 0 0 0 0 0 0 0 1 0 0 + Value(2 octets) */
#define IPMC_IPV4_RTR_ALERT_PREFIX2 0x04 /* 1 0 0 1 0 1 0 0 0 0 0 0 0 1 0 0 + Value(2 octets) */
#define IPMC_IPV6_RTR_ALERT_TYPE    0x05    /* 0 0 0 0 0 1 0 1 0 0 0 0 0 0 1 0 + Value(2 octets) */
#define IPMC_IPV6_RTR_ALERT_LEN     0x02    /* 0 0 0 0 0 1 0 1 0 0 0 0 0 0 1 0 + Value(2 octets) */

#define IGMP_MIN_PAYLOAD_LEN        8
#define MLD_MIN_HBH_LEN             8
#define MLD_GEN_MIN_PAYLOAD_LEN     24
#define MLD_IPV6_NEXTHDR_OPT_HBH    0x0     /* Hop-By-Hop Option */
#define MLD_IPV6_NEXTHDR_ICMP       0x3A    /* MLD is a subprotocol of ICMPv6 (58) */

#define IPMC_IGMP_MSG_TYPE_QUERY    0x11 /*  17 */
#define IPMC_IGMP_MSG_TYPE_V1JOIN   0x12 /*  18 */
#define IPMC_IGMP_MSG_TYPE_V2JOIN   0x16 /*  22 */
#define IPMC_IGMP_MSG_TYPE_LEAVE    0x17 /*  23 */
#define IPMC_IGMP_MSG_TYPE_V3JOIN   0x22 /*  34 */
#define IPMC_MLD_MSG_TYPE_QUERY     0x82 /* 130 */
#define IPMC_MLD_MSG_TYPE_V1REPORT  0x83 /* 131 */
#define IPMC_MLD_MSG_TYPE_DONE      0x84 /* 132 */
#define IPMC_MLD_MSG_TYPE_V2REPORT  0x8F /* 143 */

/******************************************************************************/
// IPMC_LIB_PDU_16bit_read()
/******************************************************************************/
static uint16_t IPMC_LIB_PDU_16bit_read(const uint8_t *&p)
{
    uint16_t val;

    val  = *(p++) << 8;
    val |= *(p++);

    return val;
}

/******************************************************************************/
// IPMC_LIB_PDU_32bit_read()
/******************************************************************************/
static uint32_t IPMC_LIB_PDU_32bit_read(const uint8_t *&p)
{
    uint32_t val;

    val  = *(p++) << 24;
    val |= *(p++) << 16;
    val |= *(p++) <<  8;
    val |= *(p++) <<  0;

    return val;
}

/******************************************************************************/
// IPMC_LIB_PDU_ip_read()
/******************************************************************************/
static vtss_appl_ipmc_lib_ip_t IPMC_LIB_PDU_ip_read(const uint8_t *&p, bool is_ipv4)
{
    vtss_appl_ipmc_lib_ip_t ip;

    ip.is_ipv4 = is_ipv4;

    if (ip.is_ipv4) {
        ip.ipv4 = IPMC_LIB_PDU_32bit_read(p);
    } else {
        ip.ipv6 = *(mesa_ipv6_t *)p;
        p += sizeof(ip.ipv6.addr);
    }

    return ip;
}

/******************************************************************************/
// IPMC_LIB_PDU_16bit_write()
/******************************************************************************/
static void IPMC_LIB_PDU_16bit_write(uint8_t *&p, uint16_t val, bool advance = true)
{
    uint8_t *p1 = p;

    *(p1++) = (val >> 8) & 0xFF;
    *(p1++) = (val >> 0) & 0xFF;

    if (advance) {
        p = p1;
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_32bit_write()
/******************************************************************************/
static void IPMC_LIB_PDU_32bit_write(uint8_t *&p, uint32_t val, bool advance = true)
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
// IPMC_LIB_PDU_ip_write()
/******************************************************************************/
static void IPMC_LIB_PDU_ip_write(uint8_t *&p, const vtss_appl_ipmc_lib_ip_t &ip)
{
    if (ip.is_ipv4) {
        IPMC_LIB_PDU_32bit_write(p, ip.ipv4);
    } else {
        memcpy(p, ip.ipv6.addr, sizeof(ip.ipv6.addr));
        p += sizeof(ip.ipv6.addr);
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_8bit_float_to_int()
/******************************************************************************/
static uint32_t IPMC_LIB_PDU_8bit_float_to_int(uint8_t code)
{
    uint32_t exp, mant;

    if (code < 128) {
        // Use it directly as an integer.
        return code;
    }

    // RFC3376 utilizes this conversion from a floating point value to a 16 bit
    // integer.
    //   0 1 2 3 4 5 6 7
    //  +-+-+-+-+-+-+-+-+
    //  |1| exp | mant  |
    //  +-+-+-+-+-+-+-+-+
    //
    // res = (mant | 0x10) << (exp + 3)
    //
    // The result of this algorithm is a value in this range:
    // res_min = 0x10 <<  3 => 0b1000_0000          = 0x0080 =   128
    // res_max = 0x1F << 10 => 0b111_1100_0000_0000 = 0x7c00 = 31744 (around 53 minutes)
    //
    // We do the computations in 32-bit arithmetics to avoid having the caller
    // of this function to cast it in case he needs to multiply it by some other
    // number.
    exp  = (code >> 4) & 0x7;
    mant = (code >> 0) & 0xF;
    return (mant | 0x10) << (exp + 3);
}

/******************************************************************************/
// IPMC_LIB_PDU_int_to_8bit_float()
/******************************************************************************/
static uint8_t IPMC_LIB_PDU_int_to_8bit_float(uint16_t val)
{
    int8_t   exp;
    uint16_t e;

    // This function performs the opposite operation of
    // IPMC_LIB_PDU_8bit_float_to_int(). See that function for details.
    if (val < 128) {
        return val;
    }

    if (val > 31744) {
        return 0xFF;
    }

    for (exp = 7; exp >= 0; exp--) {
        e = 0x10 << (exp + 3);
        if (val >= e) {
            return (uint8_t)(0x80 | (exp << 4) | (((val - e) >> (exp + 3)) & 0xF));
        }
    }

    return 0xFF;
}

/******************************************************************************/
// IPMC_LIB_PDU_16bit_float_to_int()
/******************************************************************************/
static uint32_t IPMC_LIB_PDU_16bit_float_to_int(uint16_t code)
{
    uint32_t exp, mant;

    if (code < 32768) {
        // Use it directly as an integer.
        return code;
    }

    // RFC3810 utilizes this conversion from a floating point value to a 16 bit
    // integer.
    //   0 1 2 3 4 5 6 7 8 9 A B C D E F
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |1| exp |          mant         |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // res = (mant | 0x1000) << (exp + 3)
    //
    // The result of this algorithm is a value in this range:
    // res_min = 0x1000 <<  3 => 0b1000_0000_0000_0000          = 0x8000   =   32768
    // res_max = 0x1FFF << 10 => 0b111_1111_1111_1100_0000_0000 = 0x7FFC00 = 8387584
    //
    // We do the computations in 32-bit arithmetics to avoid having the caller
    // of this function to cast it in case he needs to multiply it by some other
    // number.
    exp  = (code >> 12) & 0x007;
    mant = (code >>  0) & 0xFFF;
    return (mant | 0x1000) << (exp + 3);
}

/******************************************************************************/
// IPMC_LIB_PDU_int_to_16bit_float()
/******************************************************************************/
static uint16_t IPMC_LIB_PDU_int_to_16bit_float(uint32_t val)
{
    int8_t   exp;
    uint32_t e;

    // This function performs the opposite operation of
    // IPMC_LIB_PDU_16bit_float_to_int(). See that function for details.
    if (val < 32768) {
        return val;
    }

    if (val >= 8387584) {
        return 0xFFFF;
    }

    for (exp = 7; exp >= 0; exp--) {
        e = 0x1000 << (exp + 3);
        if (val >= e) {
            return (uint16_t)(0x8000 | (exp << 12) | (((val - e) >> (exp + 3)) & 0xFFF));
        }
    }

    return 0xFFFF;
}

/******************************************************************************/
// IPMC_LIB_PDU_chksum_pseudo_calc()
// MLD's checksum is computed across an IPv6 pseudo-header that looks like:
//   16 bytes SIP
//   16 bytes DIP
//    4 bytes MLD message size
//    3 zero bytes
//    1 byte Next Header, which in our case contains the ICMP next-header value
//
// frm points to the beginning of the IMLD message and len is the MLD message
// length. In order to construct the pseudo-header, we also need the IPv6
// header, which is conveyed in pdu.
/******************************************************************************/
static uint16_t IPMC_LIB_PDU_chksum_pseudo_calc(const uint8_t *frm, uint16_t len, const ipmc_lib_pdu_t &pdu)
{
    mesa_ip_addr_t sip, dip;

    sip.type      = MESA_IP_TYPE_IPV6;
    sip.addr.ipv6 = pdu.sip.ipv6;
    dip.type      = MESA_IP_TYPE_IPV6;
    dip.addr.ipv6 = pdu.dip.ipv6;

    return vtss_ip_pseudo_header_checksum(frm, len, sip, dip, MLD_IPV6_NEXTHDR_ICMP);
}

/******************************************************************************/
// ipmc_lib_pdu_type_to_str()
/******************************************************************************/
const char *ipmc_lib_pdu_type_to_str(ipmc_lib_pdu_type_t type)
{
    switch (type) {
    case IPMC_LIB_PDU_TYPE_QUERY:
        return "Query";

    case IPMC_LIB_PDU_TYPE_REPORT:
        return "Report";

    default:
        T_EG(IPMC_LIB_TRACE_GRP_RX, "Invalid PDU type (%d)", type);
        return "Unknown";
    }
}

/******************************************************************************/
// ipmc_lib_pdu_version_to_str()
/******************************************************************************/
const char *ipmc_lib_pdu_version_to_str(ipmc_lib_pdu_version_t version)
{
    switch (version) {
    case IPMC_LIB_PDU_VERSION_IGMP_V1:
        return "IGMPv1";

    case IPMC_LIB_PDU_VERSION_IGMP_V2:
        return "IGMPv2";

    case IPMC_LIB_PDU_VERSION_IGMP_V3:
        return "IGMPv3";

    case IPMC_LIB_PDU_VERSION_MLD_V1:
        return "MLDv1";

    case IPMC_LIB_PDU_VERSION_MLD_V2:
        return "MLVDv2";

    default:
        T_EG(IPMC_LIB_TRACE_GRP_RX, "Invalid PDU version (%d)", version);
        return "Unknown";
    }
}

/******************************************************************************/
// ipmc_lib_pdu_record_type_to_str()
/******************************************************************************/
const char *ipmc_lib_pdu_record_type_to_str(ipmc_lib_pdu_record_type_t record_type)
{
    switch (record_type) {
    case IPMC_LIB_PDU_RECORD_TYPE_IS_IN:
        return "IS_IN";

    case IPMC_LIB_PDU_RECORD_TYPE_IS_EX:
        return "IS_EX";

    case IPMC_LIB_PDU_RECORD_TYPE_TO_IN:
        return "TO_IN";

    case IPMC_LIB_PDU_RECORD_TYPE_TO_EX:
        return "TO_EX";

    case IPMC_LIB_PDU_RECORD_TYPE_ALLOW:
        return "ALLOW_NEW";

    case IPMC_LIB_PDU_RECORD_TYPE_BLOCK:
        return "BLOCK_OLD";

    default:
        T_EG(IPMC_LIB_TRACE_GRP_RX, "Invalid record type (%d)", record_type);
        return "Unknown";
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_mac_compose()
/******************************************************************************/
static mesa_mac_t IPMC_LIB_PDU_mac_compose(const vtss_appl_ipmc_lib_ip_t &ip)
{
    mesa_mac_t mac;

    if (ip.is_ipv4) {
        // A multicast DMAC based on an IPv4 address is "01-00-5e-aa-bb-cc",
        // where aa-bb-cc are the last 23 bits of the IP address (MSbit of aa
        // is cleared). See RFC1112, chapter 6.4, Extensions to an Ethernet
        // Local Network Module.
        mac.addr[0] = 0x01;
        mac.addr[1] = 0x00;
        mac.addr[2] = 0x5E;
        mac.addr[3] = (ip.ipv4 >> 16) & 0x7F;
        mac.addr[4] = (ip.ipv4 >>  8) & 0xFF;
        mac.addr[5] = (ip.ipv4 >>  0) & 0xFF;
    } else {
        // A multicast DMAC based on an IPv6 address is "33-33-aa-bb-cc-dd",
        // where aa-bb-cc-dd are the last four bytes of the Destination IP
        // address (see RFC2464, chapter 7., Address Mapping -- Multicast).
        mac.addr[0] = 0x33;
        mac.addr[1] = 0x33;
        mac.addr[2] = ip.ipv6.addr[12];
        mac.addr[3] = ip.ipv6.addr[13];
        mac.addr[4] = ip.ipv6.addr[14];
        mac.addr[5] = ip.ipv6.addr[15];
    }

    T_NG(IPMC_LIB_TRACE_GRP_TX, "ip = %s => mac = %s", ip, mac);

    return mac;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_report_group_address_check()
/******************************************************************************/
static bool IPMC_LIB_PDU_rx_report_group_address_check(vtss_appl_ipmc_lib_ip_t &grp_addr)
{
    if (!grp_addr.is_mc()) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Group address (%s) is not a multicast address", grp_addr);
        return false;
    }

    if (grp_addr.is_ipv4) {
        // According to the old implementation, all 224.0.0.x addresses must
        // be filtered out. It's not clear to me why this is the case. I do
        // agree that they should not be routed, but it seems that my Windows
        // PC sends an IGMPv3 Report with IS_EX on e.g. 224.0.0.251, which tells
        // me that this is valid.
        // Anyhow, let's filter these out.
        if (grp_addr.ipv4 >= 0xe0000000 && grp_addr.ipv4 <= 0xe00000ff) {
            // Happens pretty often, so use trace debug rather than trace info.
            T_DG(IPMC_LIB_TRACE_GRP_RX, "Filtering out %s group address, because it's not supposed to be used", grp_addr);
            return false;
        }
    } else {
        if (grp_addr == IPMC_LIB_PDU_all_routers_ipv6 ||
            grp_addr == IPMC_LIB_PDU_all_nodes_ipv6) {
            // Happens pretty often, so use trace debug rather than trace info.
            T_DG(IPMC_LIB_TRACE_GRP_RX, "Filtering out %s group address, because it's not supposed to be used", grp_addr);
            return false;
        }
    }

    return true;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_source_address_check()
/******************************************************************************/
static bool IPMC_LIB_PDU_rx_source_address_check(ipmc_lib_pdu_group_record_t &grp_rec)
{
    ipmc_lib_src_list_itr_t src_list_itr;

    for (src_list_itr = grp_rec.src_list.begin(); src_list_itr != grp_rec.src_list.end(); ++src_list_itr) {
        if (!src_list_itr->is_uc() || src_list_itr->is_zero()) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Source address (%s) within group %s is not a unicast address or is all-zeros (which is reserved for the Any-Source Multicast entry)", *src_list_itr, grp_rec.grp_addr);
            return false;
        }
    }

    return true;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_parse_ipv4_header()
/******************************************************************************/
static ipmc_lib_pdu_rx_action_t IPMC_LIB_PDU_rx_parse_ipv4_header(const uint8_t *&p, uint32_t l3_length, ipmc_lib_pdu_t &pdu, uint16_t &pdu_len)
{
    uint16_t      v16, ip_hdr_len, payload_len, checksum, calc_checksum;
    uint8_t       v8, hdr_ver;
    const uint8_t *ipv4_start = p;
    const bool    is_ipv4 = true;

    // Version & Internet Header Length (IHL)
    v8 = *(p++);
    hdr_ver    =      (v8 >> 4) & 0xF;
    ip_hdr_len = 4 * ((v8 >> 0) & 0xF);

    if (hdr_ver != IPMC_LIB_PDU_IPV4_VERSION_VALUE) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Invalid IPv4 version (%d)", hdr_ver);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // The header length must be at least 20 bytes
    if (ip_hdr_len < 20) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Invalid IPv4 header length (%d)", ip_hdr_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Skip past DSCP and ECN
    p++;

    // Total Length
    payload_len = IPMC_LIB_PDU_16bit_read(p);

    // Check payload length against IPv4 header length, since payload length
    // includes the header length.
    if (payload_len <= ip_hdr_len) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The IPv4 header's total length (%u bytes) is <= the IPv4 header length (%u bytes)", payload_len, ip_hdr_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Size of L4 (IGMP/MLD)
    pdu_len = payload_len - ip_hdr_len;

    if (pdu_len < 8) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The IGMP length (%u bytes) is less than the smallest IGMP PDU type (8 bytes)", pdu_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Check the payload length against the L3 length (from start of IPv4
    // header).
    if (l3_length < payload_len) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The L3 length computed from the entire frame's length (%u bytes) is smaller than ipv4.total_length (%u bytes)", l3_length, payload_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Skip past Sequence ID
    p += 2;

    // Get flags and fragment offset
    v16 = IPMC_LIB_PDU_16bit_read(p);

    if (v16 & 0x2000) {
        // The More Fragments bit is set. We don't support assembly of IPV4
        // packets.
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv4.flags.mf is not zero");
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    if (v16 & 0x1FFF) {
        // The offset must be all-zeros
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv4.offset is not all-zeros (0x%04x)", v16);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // TTL
    v8 = *(p++);
    if (v8 != IPMC_IPHDR_HOPLIMIT) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv4.ttl is not %u, but %u", IPMC_IPHDR_HOPLIMIT, v8);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Protocol
    v8 = *(p++);
    if (v8 != IPMC_LIB_PDU_IGMP_PROTOCOL_ID) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv4.proto is not %u, but %u", IPMC_LIB_PDU_IGMP_PROTOCOL_ID, v8);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // IP checksum. The IP checksum is taken of all fields in the header,
    // including the checksum itself.
    checksum = IPMC_LIB_PDU_16bit_read(p);
    if ((calc_checksum = vtss_ip_checksum(ipv4_start /* IP header start */, ip_hdr_len /* number of bytes */)) != 0xFFFF) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "IPv4 checksum error: Expected 0xFFFF (given ipv4.checksum == 0x%04x), but got 0x%04x", checksum, calc_checksum);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Source IP
    pdu.sip = IPMC_LIB_PDU_ip_read(p, is_ipv4);

    // Destination IP
    pdu.dip = IPMC_LIB_PDU_ip_read(p, is_ipv4);

    if (!pdu.dip.is_mc()) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "DIP (%s) is not an IPv4 multicast address", pdu.dip);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Options

    // IGMPv1 does not require a Router Alert option, but in principle IGMPv2
    // and IGMPv3 do (see e.g. RFC2236, last paragraph of 13. Appendix I).
    // To stay backwards compatible with the old implementation, we allow IPv4
    // frames both with and without the Router Alert option.
    // However, if present, it must be correct.
    if (ip_hdr_len > 20) {
        v16 = IPMC_LIB_PDU_16bit_read(p);
        if (v16 == IPMC_IPV4_ROUTER_ALERT_TYPE) {
            // RFC2113
            // 0       Router shall examine packet. [RFC-2113]
            // 1-65535 Reserved for future use.
            v16 = IPMC_LIB_PDU_16bit_read(p);
            if (v16 != IPMC_IPV4_ROUTER_ALERT_VALUE) {
                T_IG(IPMC_LIB_TRACE_GRP_RX, "Expected router alert value to be 0x%04x, but got 0x%04x", IPMC_IPV4_ROUTER_ALERT_VALUE, v16);
                return IPMC_LIB_PDU_RX_ACTION_FLOOD;
            }
        } else {
            // Not a router alert, but we need to advance p to the next
            // 32-bit boundary
            p += 2;
        }
    }

    if (ip_hdr_len > 24) {
        // Skip remaining options.
        p += ip_hdr_len - 24;
    }

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_parse_ipv6_header()
/******************************************************************************/
static ipmc_lib_pdu_rx_action_t IPMC_LIB_PDU_rx_parse_ipv6_header(const uint8_t *&p, uint32_t l3_length, ipmc_lib_pdu_t &pdu, uint16_t &pdu_len)
{
    uint16_t      payload_len;
    uint8_t       v8, hdr_ver, next_header, hdr_ext_len_bytes, tlv_type, tlv_len;
    const uint8_t *tlv_p;
    bool          router_alert_found;
    const bool    is_ipv4 = false;

    // Version and part of Traffic Class
    v8 = *(p++);
    hdr_ver = (v8 >> 4) & 0xF;

    if (hdr_ver != IPMC_LIB_PDU_IPV6_VERSION_VALUE) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Invalid IPv6 version (%d)", hdr_ver);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Skip past remainder of Traffic Class and Flow Label
    p += 3;

    // Payload Length (size of IPv6 header extensions + remainder).
    payload_len = IPMC_LIB_PDU_16bit_read(p);

    // Check the payload length, which contains the length after the IPv6
    // header, against the L3 length, which contains the length starting at the
    // IPv6 header.
    if (l3_length < 40 /* size of IPv6 header without options */ + payload_len) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The L3 length computed from the entire frame's length (%u bytes) is smaller than ipv6.payload_length (%u bytes) + 40", l3_length, payload_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Next Header
    next_header = *(p++);

    // The Next Header must be a Hop-by-Hop Option, containing an IPv6 Router
    // Alert.
    if (next_header != MLD_IPV6_NEXTHDR_OPT_HBH) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The IPv6 header's Next Header (%u) is not a Hop-by-Hop Option (%u)", next_header, MLD_IPV6_NEXTHDR_OPT_HBH);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Hop Limit
    v8 = *(p++);
    if (v8 != IPMC_IPHDR_HOPLIMIT) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv6.hop_limit is not %u, but %u", IPMC_IPHDR_HOPLIMIT, v8);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Source IP
    pdu.sip = IPMC_LIB_PDU_ip_read(p, is_ipv4);

    // Destination IP
    pdu.dip = IPMC_LIB_PDU_ip_read(p, is_ipv4);

    // In the following, we don't advance p, because that becomes dreadful.
    // Hop-by-Hop option
    next_header = p[0];

    // Now, next_header must be ICMP (used to carry MLD PDUs).
    if (next_header != MLD_IPV6_NEXTHDR_ICMP) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The Hop-by-Hop Option's Next Header (%u) is not ICMPv6 (%u)", next_header, MLD_IPV6_NEXTHDR_ICMP);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Verify that the hop-by-hop option contains an IPv6 Router Alert.
    hdr_ext_len_bytes = (p[1] + 1) * 8;

    if (payload_len < hdr_ext_len_bytes) {
        // The payload length includes header extensions
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The Hop-by-Hop options are longer (%u bytes) than ipv6.payload_length (%u bytes)", hdr_ext_len_bytes, payload_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    // Parse the options one by one using tlv_p
    tlv_p = &p[2];

    // Make p point to the first byte after the Hop-by-Hop Options.
    p += hdr_ext_len_bytes;
    router_alert_found = false;
    while (tlv_p < p) {
        tlv_type = *(tlv_p++);

        if (tlv_type == 0) {
            // Pad1 Option
            continue;
        }

        tlv_len = *(tlv_p++);

        if (tlv_type == IPMC_IPV6_RTR_ALERT_TYPE && tlv_len == IPMC_IPV6_RTR_ALERT_LEN && tlv_p[0] == 0 && tlv_p[1] == 0) {
            router_alert_found = true;
            break;
        }

        // Skip past the value
        tlv_p += tlv_len;
    }

    if (!router_alert_found) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The Hop-by-Hop Options did not contain an MLD Router Alert");
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // The MLD PDU starts after the header extension.
    pdu_len = payload_len - hdr_ext_len_bytes;
    if (pdu_len < 24) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The MLD length (%u bytes) is less than the smallest MLD PDU type (24 bytes)", pdu_len);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_parse_igmp_query()
/******************************************************************************/
static ipmc_lib_pdu_rx_action_t IPMC_LIB_PDU_rx_parse_igmp_query(const uint8_t *&p, uint16_t pdu_len, ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_pdu_query_t &q = pdu.query;
    mesa_mac_t           exp_mac;
    uint16_t             cnt, src_cnt;
    uint8_t              v8;
    const bool           is_ipv4 = true;

    q.src_list.clear();

    // Type already parsed
    p++;

    // Max Resp Code/time
    v8 = *(p++);

    if (pdu_len == 8) {
        // Either IGMPv1 or IGMPv2. The "max response time" field
        // indicates whether it's one or the other. If it's 0, it's
        // IGMPv1.
        q.version = v8 == 0 ? IPMC_LIB_PDU_VERSION_IGMP_V1 : IPMC_LIB_PDU_VERSION_IGMP_V2;

        // Max. response time for IGMPv1 is fixed 100 (10000 ms) and
        // as is for IGMPv2.
        // We multiply by 100 to get it in milliseconds.
        q.max_response_time_ms = q.version == IPMC_LIB_PDU_VERSION_IGMP_V1 ? 100 * 100 : 100 * v8;
    } else {
        // IGMPv3.
        if (pdu_len < 12) {
            // RFC3376, 7.1
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length is %u bytes, but must be at least 12 bytes for IGMPv3 queries", pdu_len);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        q.version = IPMC_LIB_PDU_VERSION_IGMP_V3;

        // Max. response time for IGMPv3 is either as is or encoded as
        // a floating point value. Either way, we use a function to get
        // it. Multiply by 100 to get it in milliseconds.
        q.max_response_time_ms = 100 * IPMC_LIB_PDU_8bit_float_to_int(v8);
        if (q.max_response_time_ms == 0) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Maximum Response Delay may not be 0 in IGMPv3 Queries");
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }
    }

    // Checksum already parsed and checked
    p += 2;

    if (q.version == IPMC_LIB_PDU_VERSION_IGMP_V1) {
        // The group address must not be used for anything. Zero it out and
        // advance the pointer 4 bytes.
        q.grp_addr.is_ipv4 = true;
        q.grp_addr.ipv4    = 0;
        p += 4;
    } else {
        // Group address.
        q.grp_addr = IPMC_LIB_PDU_ip_read(p, is_ipv4);

        if (!q.grp_addr.is_zero() && !q.grp_addr.is_mc()) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Group address (%s) is not 0.0.0.0 or an IPv4 multicast address", q.grp_addr);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        // If this is a general query, the IPv4 header's DIP must be the
        // all-systems M/C address (224.0.0.1, 0xe0000001)
        if (q.grp_addr.ipv4 && pdu.dip.ipv4 != 0xe0000001) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "General queries require the ipv4.dip to be 224.0.0.1 and not %s", pdu.dip);
        }

        // Check the group address against the used DMAC.
        // The DMAC must be either 00-01-5e-00-00-01 or it must be the group
        // address converted to a MAC address (see details in
        // IPMC_LIB_PDU_mac_compose()).
        if (pdu.dmac != IPMC_LIB_PDU_all_nodes_mac_ipv4) {
            exp_mac = IPMC_LIB_PDU_mac_compose(q.grp_addr);
            if (pdu.dmac != exp_mac) {
                T_IG(IPMC_LIB_TRACE_GRP_RX, "The DMAC (%s) does not correspond to the group address (%s). Expected either %s or %s", pdu.dmac, q.grp_addr, IPMC_LIB_PDU_all_nodes_mac_ipv4, exp_mac);
                return IPMC_LIB_PDU_RX_ACTION_DISCARD;
            }
        }
    }

    if (q.version == IPMC_LIB_PDU_VERSION_IGMP_V3) {
        v8       = *(p++);
        q.s_flag = (v8 >> 3) & 0x1;
        q.qrv    = (v8 >> 0) & 0x7;
        q.qqi    = IPMC_LIB_PDU_8bit_float_to_int(*(p++));

        // See if there are any sources (for Group-and-Source-Specific queries).
        src_cnt = IPMC_LIB_PDU_16bit_read(p);

        if (pdu_len < 12 + src_cnt * sizeof(mesa_ipv4_t)) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length (%u bytes) does not accommodate %u sources", pdu_len, src_cnt);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        for (cnt = 0; cnt < src_cnt; cnt++) {
            q.src_list.set(IPMC_LIB_PDU_ip_read(p, is_ipv4));
        }
    } else {
        // These are not used in IGMPv1 and IGMPv2:
        q.s_flag  = 0;
        q.qrv     = 0;
        q.qqi     = 0;
    }

    // Copy the version to the top level PDU for easy access.
    pdu.version = q.version;

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_parse_igmp_report()
/******************************************************************************/
static ipmc_lib_pdu_rx_action_t IPMC_LIB_PDU_rx_parse_igmp_report(const uint8_t *&p, uint16_t pdu_len, ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_pdu_report_t       &r = pdu.report;
    ipmc_lib_pdu_group_record_t *rec;
    const uint8_t               *p_start = p;
    uint8_t                     aux_data_len;
    uint16_t                    rec_cnt, cnt, src_cnt, valid_grp_rec_cnt;
    const bool                  is_ipv4 = true;

    // pdu.version is set by the caller based on PDU type. Copy it to the report
    // structure for self-containedness.
    r.version = pdu.version;

    // Type, Unused, and Checksum already parsed and checked by caller.
    p += 4;

    if (r.version == IPMC_LIB_PDU_VERSION_IGMP_V1 || r.version == IPMC_LIB_PDU_VERSION_IGMP_V2) {
        // IGMPv1 && IGMPv2
        if (pdu_len < 8) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length is %u bytes, but must be at least 8 bytes for IGMPv1/v2 reports", pdu_len);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        // Always exactly one record, indexed by 0
        r.rec_cnt = 1;
        rec = &r.group_recs[0];

        // Since we use the same structure for Join and Leave messages, we use
        // the record type to indicate whether it's one or the other.
        rec->record_type = r.is_leave ? IPMC_LIB_PDU_RECORD_TYPE_TO_IN : IPMC_LIB_PDU_RECORD_TYPE_IS_EX;

        // Group address
        rec->grp_addr = IPMC_LIB_PDU_ip_read(p, is_ipv4);

        if (!IPMC_LIB_PDU_rx_report_group_address_check(rec->grp_addr)) {
            return IPMC_LIB_PDU_RX_ACTION_FLOOD;
        }

        // We don't have any sources in those two versions.
        rec->src_list.clear();

        // The record is valid
        rec->valid = true;
    } else {
        // IGMPv3
        if (pdu_len < 16) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length is %u bytes, but must be at least 16 bytes for IGMPv3 reports", pdu_len);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        // Skip past Reserved
        p += 2;

        r.rec_cnt = IPMC_LIB_PDU_16bit_read(p);

        if (r.rec_cnt == 0 || r.rec_cnt > ARRSZ(r.group_recs)) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Number of group records (%u entries) is either zero or greater than what we can cope with (%u entries)", r.rec_cnt, ARRSZ(r.group_recs));
            return IPMC_LIB_PDU_RX_ACTION_FLOOD;
        }

        // We cannot make pdu_len checks yet, because each record is of variable
        // length.

        valid_grp_rec_cnt = 0;
        for (rec_cnt = 0; rec_cnt < r.rec_cnt; rec_cnt++) {
            rec = &r.group_recs[rec_cnt];

            rec->src_list.clear();

            // Assume it's valid
            rec->valid = true;

            // Record Type
            rec->record_type = (ipmc_lib_pdu_record_type_t) * (p++);

            if (rec->record_type < IPMC_LIB_PDU_RECORD_TYPE_IS_IN || rec->record_type > IPMC_LIB_PDU_RECORD_TYPE_BLOCK) {
                // RFC3376, 4.2.12, last line: Unrecognized Record Type values
                // MUST be silently ignored.
                //
                // RFC3810 (MLDv2) states that only this record must be
                // silently ignored, with the rest of the report being
                // processed, so we do the same here (don't return), and rely on
                // the users of this PDU to filter it out.
                T_IG(IPMC_LIB_TRACE_GRP_RX, "Record type of record %u is invalid (%u)", rec_cnt, rec->record_type);
                rec->valid = false;

                // We must continue to process it anyway, because we need to
                // advance the frame pointer.
            }

            // Read Aux Data Len and save it for later.
            aux_data_len = *(p++);

            // Number of sources
            src_cnt = IPMC_LIB_PDU_16bit_read(p);

            if (src_cnt > 365 /* see computations in ipmc_lib_base.hxx */) {
                T_IG(IPMC_LIB_TRACE_GRP_RX, "Number of source addresses (%u) in record #%u is greater than what we can cope with (365 entries)", src_cnt, rec_cnt);
                return IPMC_LIB_PDU_RX_ACTION_FLOOD;
            }

            // Group address
            rec->grp_addr = IPMC_LIB_PDU_ip_read(p, is_ipv4);

            // Process source addresses
            for (cnt = 0; cnt < src_cnt; cnt++) {
                rec->src_list.set(IPMC_LIB_PDU_ip_read(p, is_ipv4));
            }

            // We don't use this group record if the group address is invalid or
            // if at least one source address is invalid (according to old
            // implementation). However, we keep going because we might find at
            // least one valid group record in this PDU.
            if (rec->valid) {
                rec->valid = IPMC_LIB_PDU_rx_report_group_address_check(rec->grp_addr) && IPMC_LIB_PDU_rx_source_address_check(*rec);
            }

            if (rec->valid) {
                valid_grp_rec_cnt++;
            }

            // Advance past the Auxiliary Data (aux_data_len is in units of
            // 32 bit words).
            p += 4 * aux_data_len;

            // Do a pdu_len check before we get too far into the contents after
            // the frame. I don't think we can get into a situation where an
            // exception is thrown for reading past the location of the end of
            // the frame, because the frame is globally allocated.
            if (pdu_len < p - p_start) {
                T_IG(IPMC_LIB_TRACE_GRP_RX, "The total size of the IGMPv3 message (%u bytes) is smaller than what the parsed PDU indicates (%u bytes)", pdu_len, p - p_start);
                return IPMC_LIB_PDU_RX_ACTION_DISCARD;
            }
        }

        if (valid_grp_rec_cnt == 0) {
            // Not a single group record was valid within this PDU. Flood it
            // without processing.
            return IPMC_LIB_PDU_RX_ACTION_FLOOD;
        }
    }

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_parse_mld_query()
/******************************************************************************/
static ipmc_lib_pdu_rx_action_t IPMC_LIB_PDU_rx_parse_mld_query(const uint8_t *&p, uint16_t pdu_len, ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_pdu_query_t    &q = pdu.query;
    mesa_mac_t          exp_mac;
    uint16_t            cnt, src_cnt, v16;
    uint8_t             v8;
    const bool          is_ipv4 = false;

    q.src_list.clear();

    // RFC3810, 5.1.14: All MLDV2 Queries must be sent with a link-local source
    // address.
    if (!vtss_ipv6_addr_is_link_local(&pdu.sip.ipv6)) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv6.sip (%s) is not a link-local address", pdu.sip);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Type already parsed, Code not needed and checksum already checked.
    p += 4;

    // Read Maximum Response Delay
    v16 = IPMC_LIB_PDU_16bit_read(p);

    if (v16 == 0) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Maximum Response Delay may not be 0 in MLD Queries");
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    if (pdu_len == 24) {
        // MLDv1.
        q.version = IPMC_LIB_PDU_VERSION_MLD_V1;

        // Max. response time is as is for MLDv1
        q.max_response_time_ms = v16;
    } else {
        // MLDv2.
        if (pdu_len < 28) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length is %u bytes, but must be at least 28 bytes for MLDv3 queries", pdu_len);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        q.version = IPMC_LIB_PDU_VERSION_MLD_V2;

        // Max. response time for MLDv2 is either as is or encoded as
        // a floating point value. Either way, we use a function to get
        // it.
        q.max_response_time_ms = IPMC_LIB_PDU_16bit_float_to_int(v16);
    }

    // Skip past Reserved
    p += 2;

    q.grp_addr = IPMC_LIB_PDU_ip_read(p, is_ipv4);

    if (!q.grp_addr.is_zero() && !q.grp_addr.is_mc()) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Group address (%s) is not :: or an IPv6 multicast address", q.grp_addr);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Check the group address against the used DMAC.
    // The DMAC must be either 33-33-00-00-00-01, corresponding to FF02::1 (all
    // local nodes) or it must be the group address converted to a MAC address
    // (see tails in IPMC_LIB_PDU_mac_compose()).
    if (pdu.dmac != IPMC_LIB_PDU_all_nodes_mac_ipv6) {
        exp_mac = IPMC_LIB_PDU_mac_compose(q.grp_addr);
        if (pdu.dmac != exp_mac) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "The DMAC (%s) does not correspond to the group address (%s). Expected either %s or %s", pdu.dmac, q.grp_addr, IPMC_LIB_PDU_all_nodes_mac_ipv6, exp_mac);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }
    }

    if (q.version == IPMC_LIB_PDU_VERSION_MLD_V2) {
        v8       = *(p++);
        q.s_flag = (v8 >> 3) & 0x1;
        q.qrv    = (v8 >> 0) & 0x7;

        if (q.qrv == 0) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Query Robustness Varible is 0");
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        q.qqi = IPMC_LIB_PDU_8bit_float_to_int(*(p++));

        // See if there are any sources (for Group-and-Source-Specific queries).
        src_cnt = IPMC_LIB_PDU_16bit_read(p);

        if (pdu_len < 28 + src_cnt * sizeof(mesa_ipv6_t)) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length (%u bytes) does not accommodate %u sources", pdu_len, src_cnt);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        for (cnt = 0; cnt < src_cnt; cnt++) {
            q.src_list.set(IPMC_LIB_PDU_ip_read(p, is_ipv4));
        }
    } else {
        // These are not used in MLDv1:
        q.s_flag  = 0;
        q.qrv     = 0;
        q.qqi     = 0;
    }

    // Copy the version to the top level PDU for easy access.
    pdu.version = q.version;

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// IPMC_LIB_PDU_rx_parse_mld_report()
/******************************************************************************/
static ipmc_lib_pdu_rx_action_t IPMC_LIB_PDU_rx_parse_mld_report(const uint8_t *&p, uint16_t pdu_len, ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_pdu_report_t       &r = pdu.report;
    ipmc_lib_pdu_group_record_t *rec;
    const uint8_t           *p_start = p;
    uint8_t                 aux_data_len;
    uint16_t                rec_cnt, cnt, src_cnt, valid_grp_rec_cnt;
    const bool              is_ipv4 = false;

    // pdu.version is set by the caller based on PDU type. Copy it to the report
    // structure for self-containedness.
    r.version = pdu.version;

    // RFC3810, 5.1.12: All MLDV2 Queries must be sent with a link-local source
    // address or the unspecified address (::).
    if (!vtss_ipv6_addr_is_link_local(&pdu.sip.ipv6) && !pdu.sip.is_zero()) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv6.sip (%s) is not link-local or the unspecified address", pdu.sip);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // Type, Code/Reserved, and Checksum already parsed and checked by caller.
    p += 4;

    if (r.version == IPMC_LIB_PDU_VERSION_MLD_V1) {
        // MLD_V1
        if (pdu_len < 24) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length is %u bytes, but must be at least 24 bytes for MLDv1", pdu_len);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        // Skip past Maximum Response Delay and Reserved
        p += 4;

        // Always exactly one record, indexed by 0
        r.rec_cnt = 1;
        rec = &r.group_recs[0];

        // Since we use the same structure for Report and Done messages, we use
        // the record type to indicate whether it's one or the other.
        rec->record_type   = r.is_leave ? IPMC_LIB_PDU_RECORD_TYPE_TO_IN : IPMC_LIB_PDU_RECORD_TYPE_IS_EX;
        rec->grp_addr = IPMC_LIB_PDU_ip_read(p, is_ipv4);

        if (!IPMC_LIB_PDU_rx_report_group_address_check(rec->grp_addr)) {
            return IPMC_LIB_PDU_RX_ACTION_FLOOD;
        }

        // We don't have any sources in those two versions.
        rec->src_list.clear();

        // The record is valid
        rec->valid = true;
    } else {
        // MLDv2
        if (pdu_len < 28) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "PDU length is %u bytes, but must be at least 28 bytes for MLDv2 reports", pdu_len);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }

        // Skip past Reserved
        p += 2;

        r.rec_cnt = IPMC_LIB_PDU_16bit_read(p);

        if (r.rec_cnt == 0 || r.rec_cnt > ARRSZ(r.group_recs)) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Number of group records (%u entries) is either zero or greater than what we can cope with (%u entries)", r.rec_cnt, ARRSZ(r.group_recs));
            return IPMC_LIB_PDU_RX_ACTION_FLOOD;
        }

        // We cannot make pdu_len checks yet, because each record is of variable
        // length.

        valid_grp_rec_cnt = 0;
        for (rec_cnt = 0; rec_cnt < r.rec_cnt; rec_cnt++) {
            rec = &r.group_recs[rec_cnt];

            rec->src_list.clear();

            // Assume it's valid
            rec->valid = true;

            // Record Type
            rec->record_type = (ipmc_lib_pdu_record_type_t) * (p++);

            if (rec->record_type < IPMC_LIB_PDU_RECORD_TYPE_IS_IN || rec->record_type > IPMC_LIB_PDU_RECORD_TYPE_BLOCK) {
                // RFC3810, 5.2.12:
                // Multicast Address Records with an unrecognized Record Type
                // value MUST be silently ignored, with the rest of the report
                // being processed.
                // So simply continue and let the users of this PDU filter it
                // out.
                T_IG(IPMC_LIB_TRACE_GRP_RX, "Record type of record %u is invalid (%u)", rec_cnt, rec->record_type);
                rec->valid = false;

                // We must continue to process it anyway, because we need to
                // advance the frame pointer.
            }

            // Read Aux Data Len and save it for later.
            aux_data_len = *(p++);

            // Number of sources
            src_cnt = IPMC_LIB_PDU_16bit_read(p);

            // The maximum number of entries for MLD is smaller than that of
            // IGMP, so this could be corrected, but we live with it.
            if (src_cnt > 365 /* see computations in ipmc_lib_base.hxx */) {
                T_IG(IPMC_LIB_TRACE_GRP_RX, "Number of source addresses (%u) in record #%u is greater than what we can cope with (365 entries)", src_cnt, rec_cnt);
                return IPMC_LIB_PDU_RX_ACTION_FLOOD;
            }

            // Group address
            rec->grp_addr = IPMC_LIB_PDU_ip_read(p, is_ipv4);

            // Process source addresses
            for (cnt = 0; cnt < src_cnt; cnt++) {
                rec->src_list.set(IPMC_LIB_PDU_ip_read(p, is_ipv4));
            }

            // We don't use this group record if the group address is invalid or
            // if at least one source address is invalid (according to old
            // implementation). However, we keep going because we might find at
            // least one valid group record in this PDU.
            if (rec->valid) {
                rec->valid = IPMC_LIB_PDU_rx_report_group_address_check(rec->grp_addr) && IPMC_LIB_PDU_rx_source_address_check(*rec);
            }

            if (rec->valid) {
                valid_grp_rec_cnt++;
            }

            // Advance past the Auxiliary Data (aux_data_len is in units of
            // 32 bit words).
            p += 4 * aux_data_len;

            // Do a pdu_len check before we get too far into the contents after
            // the frame. I don't think we can get into a situation where an
            // exception is thrown for reading past the location of the end of
            // the frame, because the frame is globally allocated.
            if (pdu_len < p - p_start) {
                T_IG(IPMC_LIB_TRACE_GRP_RX, "The total size of the MLDv2 message (%u bytes) is smaller than what the parsed PDU indicates (%u bytes)", pdu_len, p - p_start);
                return IPMC_LIB_PDU_RX_ACTION_DISCARD;
            }
        }

        if (valid_grp_rec_cnt == 0) {
            // Not a single group record was valid within this PDU. Flood it
            // without processing.
            return IPMC_LIB_PDU_RX_ACTION_FLOOD;
        }
    }

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_create_l2()
/******************************************************************************/
static void IPMC_LIB_PDU_tx_create_l2(uint8_t *&p, const ipmc_lib_pdu_t &pdu)
{
    memcpy(p, pdu.dmac.addr, sizeof(pdu.dmac.addr));
    p += sizeof(pdu.dmac.addr);

    memcpy(p, pdu.smac.addr, sizeof(pdu.smac.addr));
    p += sizeof(pdu.smac.addr);

    IPMC_LIB_PDU_16bit_write(p, pdu.is_ipv4 ? ETYPE_IPV4 : ETYPE_IPV6);
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_create_ipv4_header()
/******************************************************************************/
static void IPMC_LIB_PDU_tx_create_ipv4_header(uint8_t *&p, const ipmc_lib_pdu_t &pdu, uint16_t pdu_len)
{
    uint8_t  *p_start = p, *ip_checksum_offset, *total_len_offset;
    uint16_t total_len;

    // Skip past Version + IHL for now
    p++;

    // DSCP/ECN
    *(p++) = 0;

    // Skip past Total Length for now
    total_len_offset = p;
    p += 2;

    // Identification
    IPMC_LIB_PDU_16bit_write(p, 0);

    // Flags/Fragment Offset
    IPMC_LIB_PDU_16bit_write(p, 0);

    // TTL
    *(p++) = IPMC_IPHDR_HOPLIMIT;

    // Protocol
    *(p++) = IPMC_LIB_PDU_IGMP_PROTOCOL_ID;

    // Set Header Checksum to 0 for now
    ip_checksum_offset = p;
    IPMC_LIB_PDU_16bit_write(p, 0);

    // SIP
    IPMC_LIB_PDU_ip_write(p, pdu.sip);

    // DIP
    IPMC_LIB_PDU_ip_write(p, pdu.dip);

    // Options
    // Router Alert
    IPMC_LIB_PDU_16bit_write(p, IPMC_IPV4_ROUTER_ALERT_TYPE);
    IPMC_LIB_PDU_16bit_write(p, IPMC_IPV4_ROUTER_ALERT_VALUE);

    // Time for Version + IHL
    *p_start = (IPMC_LIB_PDU_IPV4_VERSION_VALUE << 4) | ((p - p_start) / 4);

    // Total Length
    total_len = p - p_start + pdu_len;
    IPMC_LIB_PDU_16bit_write(total_len_offset, total_len);

    // IPv4 checksum
    IPMC_LIB_PDU_16bit_write(ip_checksum_offset, vtss_ip_checksum(p_start, total_len));
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_create_ipv6_header()
/******************************************************************************/
static void IPMC_LIB_PDU_tx_create_ipv6_header(uint8_t *&p, const ipmc_lib_pdu_t &pdu, uint16_t pdu_len)
{
    uint8_t  *payload_len_offset, *option_offset;
    uint16_t payload_len;

    // Version + Traffic Class (0) + Flow Label (0)
    IPMC_LIB_PDU_32bit_write(p, IPMC_LIB_PDU_IPV6_VERSION_VALUE << 28);

    // Skip past Payload Length for now
    payload_len_offset = p;
    p += 2;

    // Set Next Header to Hop-by-Hop Option
    *(p++) = MLD_IPV6_NEXTHDR_OPT_HBH;

    // Hop Limit
    *(p++) = IPMC_IPHDR_HOPLIMIT;

    // SIP
    IPMC_LIB_PDU_ip_write(p, pdu.sip);

    // DIP
    IPMC_LIB_PDU_ip_write(p, pdu.dip);

    // IPv6 Hop-by-Hop Option
    option_offset = p;

    // Set Next Header to ICMPv6
    *(p++) = MLD_IPV6_NEXTHDR_ICMP;

    // Length is 0 (meaning 8 bytes)
    *(p++) = 0;

    // Set Type to Router Alert
    *(p++) = IPMC_IPV6_RTR_ALERT_TYPE;

    // Set Length to 2
    *(p++) = IPMC_IPV6_RTR_ALERT_LEN;

    // Set value to 0, indicating MLD
    IPMC_LIB_PDU_16bit_write(p, 0);

    // Two bytes missing to fill out the 8 bytes, so issue a PadN with length 0.
    // The old implementation sent two Pad1 bytes.
    *(p++) = 1;
    *(p++) = 0;

    // Payload Length (size of options + MLD message)
    payload_len = p - option_offset + pdu_len;
    IPMC_LIB_PDU_16bit_write(payload_len_offset, payload_len);
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_create_query()
/******************************************************************************/
static mesa_rc IPMC_LIB_PDU_tx_create_query(ipmc_lib_vlan_state_t &vlan_state, uint8_t *&p, const ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_src_list_const_itr_t src_list_itr;
    const ipmc_lib_pdu_query_t    &q = pdu.query;
    uint16_t                      pdu_len, ip_len, v16;
    uint8_t                       *ip_offset, *pdu_offset, *pdu_checksum_offset, *p_end, v8;

    IPMC_LIB_PDU_tx_create_l2(p, pdu);

    // Skip IP header for now.
    ip_offset = p;
    ip_len    = pdu.is_ipv4 ? 24 : 48;
    p        += ip_len;

    pdu_offset = p;
    if (pdu.is_ipv4) {
        // The start is the same for IGMPv1, IGMPv2, and IGMPv3 queries.

        // Type
        *(p++) = IPMC_IGMP_MSG_TYPE_QUERY;

        // Max Resp Time/Code
        switch (pdu.version) {
        case IPMC_LIB_PDU_VERSION_IGMP_V1:
            // Always set to 0 in IGMPv1
            v8 = 0;
            break;

        case IPMC_LIB_PDU_VERSION_IGMP_V2:
            // Use it as is (when converted to 10ths of a second) in IGMPv2, but
            // limit it to 255 (25.5 seconds).
            v8 = MIN(q.max_response_time_ms / 100, 255);
            break;

        default:
            // Possibly convert to a float in IGMPv3.
            v8 = IPMC_LIB_PDU_int_to_8bit_float(q.max_response_time_ms / 100);
            break;
        }

        *(p++) = v8;

        // Checksum. Initialize to zeros and update later.
        pdu_checksum_offset = p;
        IPMC_LIB_PDU_16bit_write(p, 0);

        // Group Address
        IPMC_LIB_PDU_ip_write(p, q.grp_addr);

        if (pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V3) {
            // Resv/S/QRV

            // RFC3376, 4.1.6: If QRV exceeds 7, set it to 0.
            *(p++) = ((q.s_flag & 0x1) << 3) | (q.qrv > 7 ? 0 : q.qrv);

            // QQIC
            *(p++) = IPMC_LIB_PDU_int_to_8bit_float(q.qqi);

            // Number of Sources
            IPMC_LIB_PDU_16bit_write(p, q.src_list.size());

            // Source Addresses
            for (src_list_itr = q.src_list.cbegin(); src_list_itr != q.src_list.cend(); ++src_list_itr) {
                IPMC_LIB_PDU_ip_write(p, *src_list_itr);
            }
        }

        pdu_len = p - pdu_offset;
        p_end   = p;

        // IGMP checksum
        IPMC_LIB_PDU_16bit_write(pdu_checksum_offset, vtss_ip_checksum(pdu_offset, pdu_len));

        // Fill in IPv4 header
        p = ip_offset;
        IPMC_LIB_PDU_tx_create_ipv4_header(p, pdu, pdu_len);
    } else {
        // The start is the same for MLDv1 and MLV2 queries.

        // Type
        *(p++) = IPMC_MLD_MSG_TYPE_QUERY;

        // Code. Always 0
        *(p++) = 0;

        // Checksum. Initialize to zeros and update later.
        pdu_checksum_offset = p;
        IPMC_LIB_PDU_16bit_write(p, 0);

        // Maximum Response Delay/Code
        if (pdu.version == IPMC_LIB_PDU_VERSION_MLD_V1) {
            // Use it as is in MLDv1, but limit it to 65535 (65.535 seconds).
            v16 = MIN(q.max_response_time_ms, 65535);
        } else {
            // Possibly convert to a float in MLDv2
            v16 = IPMC_LIB_PDU_int_to_16bit_float(q.max_response_time_ms);
        }

        IPMC_LIB_PDU_16bit_write(p, v16);

        // Reserved
        IPMC_LIB_PDU_16bit_write(p, 0);

        // Multicast Address
        IPMC_LIB_PDU_ip_write(p, q.grp_addr);

        if (pdu.version == IPMC_LIB_PDU_VERSION_MLD_V2) {
            // Resv/S/QRV

            // RFC3376, 4.1.6: If QRV exceeds 7, set it to 0.
            *(p++) = ((q.s_flag & 0x1) << 3) | (q.qrv > 7 ? 0 : q.qrv);

            // QQIC
            *(p++) = IPMC_LIB_PDU_int_to_8bit_float(q.qqi);

            // Number of Sources
            IPMC_LIB_PDU_16bit_write(p, q.src_list.size());

            // Source Addresses
            for (src_list_itr = q.src_list.cbegin(); src_list_itr != q.src_list.cend(); ++src_list_itr) {
                IPMC_LIB_PDU_ip_write(p, *src_list_itr);
            }
        }

        pdu_len = p - pdu_offset;
        p_end   = p;

        // MLD checksum
        IPMC_LIB_PDU_16bit_write(pdu_checksum_offset, IPMC_LIB_PDU_chksum_pseudo_calc(pdu_offset, pdu_len, pdu));

        // Fill in IPv6 header
        p = ip_offset;
        IPMC_LIB_PDU_tx_create_ipv6_header(p, pdu, pdu_len);
    }

    // Sanity check that we used the correct IP header length.
    // When we get here, p points to the first byte after the IP header.
    if (p - ip_offset != ip_len) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "%s: Internal error: Computed the IP header length incorrectly (calc = %u, actual = %u) for PDU = %s", vlan_state.vlan_key, ip_len, p - ip_offset, pdu);
        return VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
    }

    // Back to the end of the PDU to let the caller compute the correct length.
    p = p_end;

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_create_report()
/******************************************************************************/
static mesa_rc IPMC_LIB_PDU_tx_create_report(uint8_t *&p, const ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_src_list_const_itr_t src_list_itr;
    const ipmc_lib_pdu_report_t       &r = pdu.report;
    uint16_t                      pdu_len, ip_len;
    uint8_t                       *ip_offset, *pdu_offset, *pdu_checksum_offset, *p_end, type;
    int                           i;

    IPMC_LIB_PDU_tx_create_l2(p, pdu);

    // Skip IP header for now.
    ip_offset = p;
    ip_len    = pdu.is_ipv4 ? 24 : 48;
    p        += ip_len;

    if (r.is_leave) {
        type = pdu.is_ipv4 ? IPMC_IGMP_MSG_TYPE_LEAVE : IPMC_MLD_MSG_TYPE_DONE;
    } else {
        switch (pdu.version) {
        case IPMC_LIB_PDU_VERSION_IGMP_V1:
            type = IPMC_IGMP_MSG_TYPE_V1JOIN;
            break;

        case IPMC_LIB_PDU_VERSION_IGMP_V2:
            type = IPMC_IGMP_MSG_TYPE_V2JOIN;
            break;

        case IPMC_LIB_PDU_VERSION_IGMP_V3:
            type = IPMC_IGMP_MSG_TYPE_V3JOIN;
            break;

        case IPMC_LIB_PDU_VERSION_MLD_V1:
            type = IPMC_MLD_MSG_TYPE_V1REPORT;
            break;

        case IPMC_LIB_PDU_VERSION_MLD_V2:
            type = IPMC_MLD_MSG_TYPE_V2REPORT;
            break;

        default:
            T_EG(IPMC_LIB_TRACE_GRP_TX, "Invalid version (%d)", pdu.version);
            return VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
        }
    }

    pdu_offset = p;
    if (pdu.is_ipv4) {
        // The start is the same for IGMPv1, IGMPv2, and IGMPv3 reports.

        // Type
        *(p++) = type;

        // Unused/Max Resp Time/Reserved
        *(p++) = 0;

        // Checksum. Initialize to zeros and update later.
        pdu_checksum_offset = p;
        IPMC_LIB_PDU_16bit_write(p, 0);

        if (pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V1 || pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V2) {
            // IGMPv1 and IGMPv2
            // Group Address
            IPMC_LIB_PDU_ip_write(p, r.group_recs[0].grp_addr);
        } else {
            // IGMPv3

            // Reserved. Always 0.
            IPMC_LIB_PDU_16bit_write(p, 0);

            // Number of Group Records (M)
            IPMC_LIB_PDU_16bit_write(p, r.rec_cnt);

            // Group Records
            for (i = 0; i < r.rec_cnt; i++) {
                // Record Type
                *(p++) = r.group_recs[i].record_type;

                // Aux Data Len. We always set it to 0.
                *(p++) = 0;

                // Number of Sources (N)
                IPMC_LIB_PDU_16bit_write(p, r.group_recs[i].src_list.size());

                // Multicast Address
                IPMC_LIB_PDU_ip_write(p, r.group_recs[i].grp_addr);

                // Source Addresses
                for (src_list_itr = r.group_recs[i].src_list.cbegin(); src_list_itr != r.group_recs[i].src_list.cend(); ++src_list_itr) {
                    IPMC_LIB_PDU_ip_write(p, *src_list_itr);
                }
            }
        }

        pdu_len = p - pdu_offset;
        p_end   = p;

        // IGMP checksum
        IPMC_LIB_PDU_16bit_write(pdu_checksum_offset, vtss_ip_checksum(pdu_offset, pdu_len));

        // Fill in IPv4 header
        p = ip_offset;
        IPMC_LIB_PDU_tx_create_ipv4_header(p, pdu, pdu_len);
    } else {
        // The start is the same for MLDv1 and MLV2 reports.

        // Type
        *(p++) = type;

        // Code/Reserved. Always 0
        *(p++) = 0;

        // Checksum. Initialize to zeros and update later.
        pdu_checksum_offset = p;
        IPMC_LIB_PDU_16bit_write(p, 0);

        // Maximum Response Delay/Reserved. Always 0
        IPMC_LIB_PDU_16bit_write(p, 0);

        if (pdu.version == IPMC_LIB_PDU_VERSION_MLD_V1) {
            // MLDv1
            // Reserved. Always 0.
            IPMC_LIB_PDU_16bit_write(p, 0);

            // Multicast Address
            IPMC_LIB_PDU_ip_write(p, r.group_recs[0].grp_addr);
        } else {
            // MLDv2
            // Nr of Mcast Address Records (M)
            IPMC_LIB_PDU_16bit_write(p, r.rec_cnt);

            // Group Records
            for (i = 0; i < r.rec_cnt; i++) {
                // Record Type
                *(p++) = r.group_recs[i].record_type;

                // Aux Data Len. We always set it to 0.
                *(p++) = 0;

                // Number of Sources (N)
                IPMC_LIB_PDU_16bit_write(p, r.group_recs[i].src_list.size());

                // Multicast Address
                IPMC_LIB_PDU_ip_write(p, r.group_recs[i].grp_addr);

                // Source Addresses
                for (src_list_itr = r.group_recs[i].src_list.cbegin(); src_list_itr != r.group_recs[i].src_list.cend(); ++src_list_itr) {
                    IPMC_LIB_PDU_ip_write(p, *src_list_itr);
                }
            }
        }

        pdu_len = p - pdu_offset;
        p_end   = p;

        // MLD checksum
        IPMC_LIB_PDU_16bit_write(pdu_checksum_offset, IPMC_LIB_PDU_chksum_pseudo_calc(pdu_offset, pdu_len, pdu));

        // Fill in IPv6 header
        p = ip_offset;
        IPMC_LIB_PDU_tx_create_ipv6_header(p, pdu, pdu_len);
    }

    // Sanity check that we used the correct IP header length.
    // When we get here, p points to the first byte after the IP header.
    if (p - ip_offset != ip_len) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Internal error: Computed the IP header length incorrectly (calc = %u, actual = %u) for PDU = %s", ip_len, p - ip_offset, pdu);
        return VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
    }

    // Back to the end of the PDU to let the caller compute the correct length.
    p = p_end;

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_PDU_version_from_compati_get()
/******************************************************************************/
static ipmc_lib_pdu_version_t IPMC_LIB_PDU_version_from_compati_get(vtss_appl_ipmc_lib_compatibility_t compat, bool is_ipv4)
{
    switch (compat) {
    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD:
        return is_ipv4 ? IPMC_LIB_PDU_VERSION_IGMP_V1 : IPMC_LIB_PDU_VERSION_MLD_V1 /* Don't think this is used for MLD */;

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN:
        return is_ipv4 ? IPMC_LIB_PDU_VERSION_IGMP_V2 : IPMC_LIB_PDU_VERSION_MLD_V1;

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO:
    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM:
        return is_ipv4 ? IPMC_LIB_PDU_VERSION_IGMP_V3 : IPMC_LIB_PDU_VERSION_MLD_V2;

    default:
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Cannot determine %s version from compatibility = %d", is_ipv4 ? "IGMP" : "MLD", compat);
        return is_ipv4 ? IPMC_LIB_PDU_VERSION_IGMP_V1 : IPMC_LIB_PDU_VERSION_MLD_V1;
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_query_statistics_update()
/******************************************************************************/
static void IPMC_LIB_PDU_query_statistics_update(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, bool is_rx, bool ignored)
{
    vtss_appl_ipmc_lib_igmp_vlan_statistics_t &igmp = is_rx ? (ignored ? vlan_state.statistics.rx.igmp.ignored : vlan_state.statistics.rx.igmp.utilized) : vlan_state.statistics.tx.igmp;
    vtss_appl_ipmc_lib_mld_vlan_statistics_t  &mld  = is_rx ? (ignored ? vlan_state.statistics.rx.mld.ignored  : vlan_state.statistics.rx.mld.utilized)  : vlan_state.statistics.tx.mld;

    switch (pdu.version) {
    case IPMC_LIB_PDU_VERSION_IGMP_V1:
        igmp.v1_query++;
        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V2:
        if (pdu.query.grp_addr.is_zero()) {
            igmp.v2_g_query++;
        } else {
            igmp.v2_gs_query++;
        }

        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V3:
        if (pdu.query.grp_addr.is_zero()) {
            igmp.v3_g_query++;
        } else if (pdu.query.src_list.empty()) {
            igmp.v3_gs_query++;
        } else {
            igmp.v3_gss_query++;
        }

        break;

    case IPMC_LIB_PDU_VERSION_MLD_V1:
        if (pdu.query.grp_addr.is_zero()) {
            mld.v1_g_query++;
        } else {
            mld.v1_gs_query++;
        }

        break;

    case IPMC_LIB_PDU_VERSION_MLD_V2:
        if (pdu.query.grp_addr.is_zero()) {
            mld.v2_g_query++;
        } else if (pdu.query.src_list.empty()) {
            mld.v2_gs_query++;
        } else {
            mld.v2_gss_query++;
        }

        break;

    default:
        if (is_rx) {
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Unknown PDU version (%d)", pdu.version);
        } else {
            T_EG(IPMC_LIB_TRACE_GRP_TX, "Unknown PDU version (%d)", pdu.version);
        }

        break;
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_report_statistics_update()
/******************************************************************************/
static void IPMC_LIB_PDU_report_statistics_update(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, bool is_rx, bool ignored)
{
    vtss_appl_ipmc_lib_igmp_vlan_statistics_t &igmp = is_rx ? (ignored ? vlan_state.statistics.rx.igmp.ignored : vlan_state.statistics.rx.igmp.utilized) : vlan_state.statistics.tx.igmp;
    vtss_appl_ipmc_lib_mld_vlan_statistics_t  &mld  = is_rx ? (ignored ? vlan_state.statistics.rx.mld.ignored  : vlan_state.statistics.rx.mld.utilized)  : vlan_state.statistics.tx.mld;

    switch (pdu.version) {
    case IPMC_LIB_PDU_VERSION_IGMP_V1:
        igmp.v1_report++;
        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V2:
        if (pdu.report.is_leave) {
            igmp.v2_leave++;
        } else {
            igmp.v2_report++;
        }

        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V3:
        igmp.v3_report++;
        break;

    case IPMC_LIB_PDU_VERSION_MLD_V1:
        if (pdu.report.is_leave) {
            mld.v1_done++;
        } else {
            mld.v1_report++;
        }

        break;

    case IPMC_LIB_PDU_VERSION_MLD_V2:
        mld.v2_report++;
        break;

    default:
        if (is_rx) {
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Unknown PDU version (%d)", pdu.version);
        } else {
            T_EG(IPMC_LIB_TRACE_GRP_TX, "Unknown PDU version (%d)", pdu.version);
        }

        break;
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_query()
// Tx General Query, Group-Specific Query, or Group-and-Source-Specific Query.
/******************************************************************************/
static void IPMC_LIB_PDU_tx_query(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr, const mesa_port_list_t &dst_port_mask, const ipmc_lib_src_list_t &src_list, ipmc_lib_pdu_version_t version, bool suppress_router_side_processing)
{
    ipmc_lib_src_list_const_itr_t src_itr;
    vtss_appl_ipmc_lib_ip_t       group_address = grp_addr;
    uint8_t                       *frm, *p;
    bool                          general_query;
    uint32_t                      frm_len, src_cnt = src_list.size();
    ipmc_lib_pdu_t                &pdu = IPMC_LIB_PDU_to_tx;

    T_DG(IPMC_LIB_TRACE_GRP_TX, "PDU version = %s, grp_addr = %s, dst_port_mask = %s, src_cnt = %zu", ipmc_lib_pdu_version_to_str(version), grp_addr, dst_port_mask, src_cnt);

    if (dst_port_mask.is_empty()) {
        T_DG(IPMC_LIB_TRACE_GRP_TX, "Empty port list");
        return;
    }

    // Auto-adapt to the desired version
    switch (version) {
    case IPMC_LIB_PDU_VERSION_IGMP_V1:
        vtss_clear(group_address);
        general_query = true;
        src_cnt       = 0;
        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V2:
    case IPMC_LIB_PDU_VERSION_MLD_V1:
        general_query = group_address.is_zero();
        src_cnt       = 0;
        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V3:
    case IPMC_LIB_PDU_VERSION_MLD_V2:
    default:
        general_query = group_address.is_zero();
        break;
    }

    // Fill in the PDU
    pdu.is_ipv4 = vlan_state.vlan_key.is_ipv4;
    pdu.version = version;
    pdu.type    = IPMC_LIB_PDU_TYPE_QUERY;

    // L2 + L3
    pdu.smac = IPMC_LIB_PDU_system_mac;
    ipmc_lib_pdu_our_ip_get(vlan_state, pdu.sip);
    if (general_query) {
        pdu.dmac = vlan_state.vlan_key.is_ipv4 ? IPMC_LIB_PDU_all_nodes_mac_ipv4 : IPMC_LIB_PDU_all_nodes_mac_ipv6;
        pdu.dip  = vlan_state.vlan_key.is_ipv4 ? IPMC_LIB_PDU_all_nodes_ipv4     : IPMC_LIB_PDU_all_nodes_ipv6;
    } else {
        pdu.dmac = IPMC_LIB_PDU_mac_compose(group_address);
        pdu.dip  = group_address;
    }

    // IGMP/MLD
    pdu.query.grp_addr = group_address;
    pdu.query.version  = pdu.version;

    // IPMC_LIB_PDU_tx_create_query() doesn't use max_response_time_ms in IGMPv1.
    if (general_query) {
        pdu.query.max_response_time_ms = vlan_state.internal_state.cur_qri  * 100 /* Convert from 10ths of a second to milliseconds */;
    } else {
        pdu.query.max_response_time_ms = vlan_state.internal_state.cur_lmqi * 100 /* Convert from 10ths of a second to milliseconds */;
    }

    // These are used in IGMPv3 and MLDv2 only, but let's just set them anyway
    // for all versions.
    pdu.query.s_flag  = suppress_router_side_processing;
    pdu.query.qrv     = vlan_state.internal_state.cur_rv;

    if (general_query) {
        pdu.query.qqi = vlan_state.internal_state.cur_qi; /* seconds */
    } else {
        pdu.query.qqi = vlan_state.internal_state.cur_lmqi / 10;
    }

    pdu.query.src_list.clear();

    if (src_cnt) {
        pdu.query.src_list = src_list;
    } else {
        pdu.query.src_list.clear();
    }

    // Create the PDU.
    if (vlan_state.vlan_key.is_mvr) {
        frm = IPMC_LIB_PDU_tx_buf_mvr;
    } else {
        frm = IPMC_LIB_PDU_tx_buf_ipmc;
    }

    p = frm;

    if (IPMC_LIB_PDU_tx_create_query(vlan_state, p, pdu) != VTSS_RC_OK) {
        return;
    }

    // Sanity check that we haven't written too much:
    frm_len = p - frm;
    if (frm_len > sizeof(IPMC_LIB_PDU_tx_buf_ipmc)) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "%s: Internal error: Constructed frame size (%u) is larger than allocated (%u)", vlan_state.vlan_key, frm_len, sizeof(IPMC_LIB_PDU_tx_buf_ipmc));
        return;
    }

    // Print the PDU, but don't trace out the frame, because that can be done by
    // ipmc_lib_pdu_tx();
    T_DG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx PDU = %s", vlan_state.vlan_key, pdu);
    if (ipmc_lib_pdu_tx(frm, frm_len, dst_port_mask, vlan_state.vlan_key.is_mvr && !vlan_state.conf.tx_tagged, VTSS_PORT_NO_NONE, vlan_state.vlan_key.vid, vlan_state.conf.pcp, 0)) {
        // No matter how many queries we actually sent, increment statistics by
        // one.
        ipmc_lib_pdu_statistics_update(vlan_state, pdu, false /* is Tx */, false /* doesn't matter */);
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_report()
/******************************************************************************/
static void IPMC_LIB_PDU_tx_report(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr, const mesa_port_list_t &dst_port_mask, vtss_appl_ipmc_lib_compatibility_t compat)
{
    uint8_t                *frm, *p;
    uint32_t               frm_len;
    ipmc_lib_pdu_version_t version = IPMC_LIB_PDU_version_from_compati_get(compat, vlan_state.vlan_key.is_ipv4);
    ipmc_lib_pdu_t         &pdu    = IPMC_LIB_PDU_to_tx;

    T_DG(IPMC_LIB_TRACE_GRP_TX, "PDU version = %s, grp_addr = %s, dst_port_mask = %s", ipmc_lib_pdu_version_to_str(version), grp_addr, dst_port_mask);

    if (dst_port_mask.is_empty()) {
        T_DG(IPMC_LIB_TRACE_GRP_TX, "Empty port list");
        return;
    }

    if (grp_addr.is_zero()) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Tx report message with a zero group address");
        return;
    }

    // Fill in the PDU
    pdu.is_ipv4 = vlan_state.vlan_key.is_ipv4;
    pdu.version = version;
    pdu.type    = IPMC_LIB_PDU_TYPE_REPORT;

    // L2 + L3
    pdu.smac = IPMC_LIB_PDU_system_mac;
    ipmc_lib_pdu_our_ip_get(vlan_state, pdu.sip);
    if (vlan_state.vlan_key.is_ipv4) {
        pdu.dmac = version == IPMC_LIB_PDU_VERSION_IGMP_V3 ? IPMC_LIB_PDU_all_routers_mac_igmpv3 : IPMC_LIB_PDU_mac_compose(grp_addr);
        pdu.dip  = version == IPMC_LIB_PDU_VERSION_IGMP_V3 ? IPMC_LIB_PDU_all_routers_igmpv3     : grp_addr;
    } else {
        pdu.dmac = version == IPMC_LIB_PDU_VERSION_MLD_V2  ? IPMC_LIB_PDU_all_routers_mac_mldv2  : IPMC_LIB_PDU_mac_compose(grp_addr);
        pdu.dip  = version == IPMC_LIB_PDU_VERSION_MLD_V2  ? IPMC_LIB_PDU_all_routers_mldv2      : grp_addr;
    }

    // IGMP/MLD
    pdu.report.version                   = pdu.version;
    pdu.report.is_leave                  = false;
    pdu.report.rec_cnt                   = 1;
    pdu.report.group_recs[0].record_type = IPMC_LIB_PDU_RECORD_TYPE_IS_EX;
    pdu.report.group_recs[0].grp_addr    = grp_addr;
    pdu.report.group_recs[0].src_list.clear();

    // Create the PDU.
    if (vlan_state.vlan_key.is_mvr) {
        frm = IPMC_LIB_PDU_tx_buf_mvr;
    } else {
        frm = IPMC_LIB_PDU_tx_buf_ipmc;
    }

    p = frm;

    if (IPMC_LIB_PDU_tx_create_report(p, pdu) != VTSS_RC_OK) {
        return;
    }

    // Sanity check that we haven't written too much:
    frm_len = p - frm;
    if (frm_len > sizeof(IPMC_LIB_PDU_tx_buf_ipmc)) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Internal error: Constructed frame size (%u) is larger than allocated (%u)", frm_len, sizeof(IPMC_LIB_PDU_tx_buf_ipmc));
        return;
    }

    // Print the PDU, but don't trace out the frame, because that can be done by
    // ipmc_lib_pdu_tx();
    T_DG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx PDU = %s", vlan_state.vlan_key, pdu);

    if (ipmc_lib_pdu_tx(frm, frm_len, dst_port_mask, vlan_state.vlan_key.is_mvr && !vlan_state.conf.tx_tagged, VTSS_PORT_NO_NONE, vlan_state.vlan_key.vid, vlan_state.conf.pcp, 0)) {
        ipmc_lib_pdu_statistics_update(vlan_state, pdu, false /* is Tx */, false /* doesn't matter */);
    }
}

/******************************************************************************/
// IPMC_LIB_PDU_tx_leave()
/******************************************************************************/
static void IPMC_LIB_PDU_tx_leave(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr, const mesa_port_list_t &dst_port_mask)
{
    uint8_t                *frm, *p;
    uint32_t               frm_len;
    ipmc_lib_pdu_version_t version = IPMC_LIB_PDU_version_from_compati_get(vlan_state.status.querier_compat, vlan_state.vlan_key.is_ipv4);
    ipmc_lib_pdu_t         &pdu    = IPMC_LIB_PDU_to_tx;
    bool                   real_leave;

    T_DG(IPMC_LIB_TRACE_GRP_TX, "PDU version = %s, grp_addr = %s, dst_port_mask = %s", ipmc_lib_pdu_version_to_str(version), grp_addr, dst_port_mask);

    if (dst_port_mask.is_empty()) {
        T_DG(IPMC_LIB_TRACE_GRP_TX, "Empty port list");
        return;
    }

    if (grp_addr.is_zero()) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Tx leave message with a zero group address");
        return;
    }

    if (version == IPMC_LIB_PDU_VERSION_IGMP_V1) {
        T_IG(IPMC_LIB_TRACE_GRP_TX, "Router compatibility is IGMPv1, so cannot construct a leave");
        // No such thing as a leave.
        return;
    }

    if (version == IPMC_LIB_PDU_VERSION_IGMP_V3 || version == IPMC_LIB_PDU_VERSION_MLD_V2) {
        // Construct a "leave" by sending a TO_IN({}).
        real_leave = false;
    } else {
        // IGMPv2 and MLv1 have real leave/done PDUs..
        real_leave = true;
    }

    // Fill in the PDU
    pdu.is_ipv4 = vlan_state.vlan_key.is_ipv4;
    pdu.version = version;
    pdu.type    = IPMC_LIB_PDU_TYPE_REPORT;

    // L2 + L3
    pdu.smac = IPMC_LIB_PDU_system_mac;
    ipmc_lib_pdu_our_ip_get(vlan_state, pdu.sip);

    if (real_leave) {
        pdu.dmac = vlan_state.vlan_key.is_ipv4 ? IPMC_LIB_PDU_all_routers_mac_ipv4 : IPMC_LIB_PDU_all_routers_mac_ipv6;
        pdu.dip  = vlan_state.vlan_key.is_ipv4 ? IPMC_LIB_PDU_all_routers_ipv4 : IPMC_LIB_PDU_all_routers_ipv6;
    } else {
        if (vlan_state.vlan_key.is_ipv4) {
            pdu.dmac = version == IPMC_LIB_PDU_VERSION_IGMP_V3 ? IPMC_LIB_PDU_all_routers_mac_igmpv3 : IPMC_LIB_PDU_mac_compose(grp_addr);
            pdu.dip  = version == IPMC_LIB_PDU_VERSION_IGMP_V3 ? IPMC_LIB_PDU_all_routers_igmpv3     : grp_addr;
        } else {
            pdu.dmac = version == IPMC_LIB_PDU_VERSION_MLD_V2  ? IPMC_LIB_PDU_all_routers_mac_mldv2  : IPMC_LIB_PDU_mac_compose(grp_addr);
            pdu.dip  = version == IPMC_LIB_PDU_VERSION_MLD_V2  ? IPMC_LIB_PDU_all_routers_mldv2      : grp_addr;
        }
    }

    // IGMP/MLD
    pdu.report.version                   = pdu.version;
    pdu.report.is_leave                  = real_leave;
    pdu.report.rec_cnt                   = 1;
    pdu.report.group_recs[0].record_type = IPMC_LIB_PDU_RECORD_TYPE_TO_IN; // Leave!
    pdu.report.group_recs[0].grp_addr    = grp_addr;
    pdu.report.group_recs[0].src_list.clear();

    // Create the PDU.
    if (vlan_state.vlan_key.is_mvr) {
        frm = IPMC_LIB_PDU_tx_buf_mvr;
    } else {
        frm = IPMC_LIB_PDU_tx_buf_ipmc;
    }

    p = frm;

    if (IPMC_LIB_PDU_tx_create_report(p, pdu) != VTSS_RC_OK) {
        return;
    }

    // Sanity check that we haven't written too much:
    frm_len = p - frm;
    if (frm_len > sizeof(IPMC_LIB_PDU_tx_buf_ipmc)) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Internal error: Constructed frame size (%u) is larger than allocated (%u)", frm_len, sizeof(IPMC_LIB_PDU_tx_buf_ipmc));
        return;
    }

    // Print the PDU, but don't trace out the frame, because that can be done by
    // ipmc_lib_pdu_tx();
    T_DG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx PDU = %s", vlan_state.vlan_key, pdu);

    if (ipmc_lib_pdu_tx(frm, frm_len, dst_port_mask, vlan_state.vlan_key.is_mvr && !vlan_state.conf.tx_tagged, VTSS_PORT_NO_NONE, vlan_state.vlan_key.vid, vlan_state.conf.pcp, 0)) {
        ipmc_lib_pdu_statistics_update(vlan_state, pdu, false /* is Tx */, false /* doesn't matter */);
    }
}

/******************************************************************************/
// ipmc_lib_pdu_tx_group_specific_query()
// Tx either a group-and-source-specific or group-specific query.
/******************************************************************************/
void ipmc_lib_pdu_tx_group_specific_query(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, const ipmc_lib_src_list_t &src_list, mesa_port_no_t port_no, bool suppress_router_side_processing)
{
    ipmc_lib_pdu_version_t version = IPMC_LIB_PDU_version_from_compati_get(grp_itr->second.grp_compat, vlan_state.vlan_key.is_ipv4);
    mesa_port_list_t   dst_port_mask;

    dst_port_mask.clear_all();
    dst_port_mask[port_no] = 1;
    IPMC_LIB_PDU_tx_query(vlan_state, grp_itr->first.grp_addr, dst_port_mask, src_list, version, suppress_router_side_processing);
}

/******************************************************************************/
// ipmc_lib_pdu_tx_general_query()
// Sends a general query to a particular port or all ports in the VLAN.
// If port_no == MESA_PORT_NO_NONE, it sends to all ports.
/******************************************************************************/
void ipmc_lib_pdu_tx_general_query(ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no)
{
    vtss_appl_ipmc_lib_ip_t grp_addr;
    ipmc_lib_src_list_t     src_list;
    ipmc_lib_pdu_version_t  version = IPMC_LIB_PDU_version_from_compati_get(vlan_state.status.host_compat, vlan_state.vlan_key.is_ipv4);
    mesa_port_list_t        dst_port_mask;

    grp_addr.is_ipv4 = vlan_state.vlan_key.is_ipv4;
    grp_addr.all_zeros_set();

    src_list.clear();

    if (port_no == MESA_PORT_NO_NONE) {
        // By default, we send this frame to all ports in the VLAN. Since this
        // function ultimately calls ipmc_lib_pdu_tx() and that function
        // filters the ports it transmits on based on a.o. VLAN membership, we
        // can safely set the destination port list to all ports.
        dst_port_mask.set_all();
    } else {
        dst_port_mask.clear_all();
        dst_port_mask[port_no] = true;
    }

    T_IG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx general query to %s", vlan_state.vlan_key, dst_port_mask);
    IPMC_LIB_PDU_tx_query(vlan_state, grp_addr, dst_port_mask, src_list, version, false);
}

/******************************************************************************/
// ipmc_lib_pdu_tx_leave()
/******************************************************************************/
void ipmc_lib_pdu_tx_leave(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr)
{
    mesa_port_list_t dst_port_mask(ipmc_lib_base_router_port_mask_get(*vlan_state.global));

    IPMC_LIB_PDU_tx_leave(vlan_state, grp_addr, dst_port_mask);
}

/******************************************************************************/
// ipmc_lib_pdu_tx_report()
/******************************************************************************/
void ipmc_lib_pdu_tx_report(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr, vtss_appl_ipmc_lib_compatibility_t compat)
{
    mesa_port_list_t dst_port_mask(ipmc_lib_base_router_port_mask_get(*vlan_state.global));
    IPMC_LIB_PDU_tx_report(vlan_state, grp_addr, dst_port_mask, compat);
}

/******************************************************************************/
// ipmc_lib_pdu_rx_parse()
//
// The RFCs are not very clear on what to discard and what to flood. Here's a
// snippet from RFC4541, 2.1.1, 5):
//   An IGMP snooping switch must not make use of information in IGMP packets
//   where the IP or IGMP headers have checksum or integrity errors. The switch
//   should not flood such packets.
//
// So what exactly is an integrity error?!? The code used to parse IPMC PDUs
// may not be fully in line with how the RFCs expect them to be flooded or
// discarded. Update as you see fit by using the appropriate
// ipmc_lib_pdu_rx_action_t enumeration.
/******************************************************************************/
ipmc_lib_pdu_rx_action_t ipmc_lib_pdu_rx_parse(const uint8_t *frm, const mesa_packet_rx_info_t &rx_info, ipmc_lib_pdu_t &pdu)
{
    const uint8_t *p = frm;
    mesa_etype_t  etype;
    uint32_t      pdu_offset;
    uint16_t      pdu_len, pdu_checksum, calc_checksum;
    uint8_t       pdu_type;
    bool          pdu_type_is_ipv4;

// This macro returns if action is not IPMC_LIB_PDU_RX_ACTION_PROCESS, indicating
// a parse error.
#define IPMC_LIB_PDU_RX_ACTION_RC(expr) {ipmc_lib_pdu_rx_action_t __action__ = (expr); if (__action__ != IPMC_LIB_PDU_RX_ACTION_PROCESS) return __action__;}

    // No need to clear the PDU's contents. It's quite large, so it takes time.
    // the code ensures to set all relevant fields.

    // Rx info
    pdu.rx_info = rx_info;

    // Reference to the frame we are parsing - in case it needs to be forwarded.
    pdu.frm = frm;

    // DMAC
    pdu.dmac = *(mesa_mac_t *)p;
    p += sizeof(pdu.dmac);

    // SMAC
    pdu.smac = *(mesa_mac_t *)p;
    p += sizeof(pdu.smac);

    // EtherType
    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the ethertype comes at frm[12] and frm[13], because
    // the packet module strips the outer tag.
    etype = IPMC_LIB_PDU_16bit_read(p);

    if (etype == ETYPE_IPV4) {
        pdu.is_ipv4 = true;
    } else if (etype == ETYPE_IPV6) {
        pdu.is_ipv4 = false;
    } else {
        // Should not be possible here, hence T_EG()
        T_EG(IPMC_LIB_TRACE_GRP_RX, "Invalid EtherType (0x%04x)", etype);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    if (pdu.is_ipv4) {
        // Parse and check the IPv4 header. The function advances p so that it
        // points to the beginning of the IGMP message when it returns
        // (successfully). Upon exit, pdu_len contains the number of bytes
        // available in the IGMP message.
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_ipv4_header(p, rx_info.length - 14 /* L2 */, pdu, pdu_len));
    } else {
        // Parse and check the IPv6 header. The function advances p so that it
        // points to the beginning of the MLD message when it returns
        // (successfully). Upon exit, pdu_len contains the number of bytes
        // available in the MLD message.
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_ipv6_header(p, rx_info.length - 14 /* L2 */, pdu, pdu_len));
    }

    // Record the offset of the start of the IGMP/MLD message.
    pdu_offset = p - frm;

    // Then parse and check the first four bytes of the IGMP/MLD message.

    // PDU type
    pdu_type = *(p++);

    // Skip past Code/Max Resp Time/Reserved or whatever the field is called in
    // the different versions.
    p++;

    // Figure out whether we support this PDU type and whether it corresponds to
    // the L3 header (IPv4/IPv6).
    switch (pdu_type) {
    case IPMC_IGMP_MSG_TYPE_QUERY:
    case IPMC_IGMP_MSG_TYPE_V1JOIN:
    case IPMC_IGMP_MSG_TYPE_V2JOIN:
    case IPMC_IGMP_MSG_TYPE_V3JOIN:
    case IPMC_IGMP_MSG_TYPE_LEAVE:
        pdu_type_is_ipv4 = true;
        break;

    case IPMC_MLD_MSG_TYPE_QUERY:
    case IPMC_MLD_MSG_TYPE_V1REPORT:
    case IPMC_MLD_MSG_TYPE_V2REPORT:
    case IPMC_MLD_MSG_TYPE_DONE:
        pdu_type_is_ipv4 = false;
        break;

    default:
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Unknown IGMP/MLD PDU type (%u)", pdu_type);
        return IPMC_LIB_PDU_RX_ACTION_FLOOD;
    }

    if (pdu.is_ipv4 != pdu_type_is_ipv4) {
        // Received IPv4 packet with MLD contents or received IPv6 packet with
        // IGMP contents.
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Received %s packet with %s contents", pdu.is_ipv4 ? "IPv4" : "IPv6", pdu_type_is_ipv4 ? "IGMP" : "MLD");
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    // IGMP/MLD checksum
    pdu_checksum = IPMC_LIB_PDU_16bit_read(p);

    // Set the pointer back to the beginning of the IGMP/MLD message before
    // continuing
    p = &frm[pdu_offset];

    if (pdu.is_ipv4) {
        if ((calc_checksum = vtss_ip_checksum(p, pdu_len)) != 0xFFFF) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "IGMP checksum error: Expected 0xFFFF (PDU's checksum == 0x%04x), but got 0x%04x", pdu_checksum, calc_checksum);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }
    } else {
        // MLD requires an IPv6 pseudo-header to compute the checksum. We get
        // that from what we have already parsed into pdu.
        if ((calc_checksum = IPMC_LIB_PDU_chksum_pseudo_calc(p, pdu_len, pdu)) != 0xFFFF) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "MLD checksum error: Expected 0xFFFF (PDU's checksum == 0x%04x), but got 0x%04x", pdu_checksum, calc_checksum);
            return IPMC_LIB_PDU_RX_ACTION_DISCARD;
        }
    }

    // Time to do the parsing
    switch (pdu_type) {
    case IPMC_IGMP_MSG_TYPE_QUERY:
        pdu.type = IPMC_LIB_PDU_TYPE_QUERY;

        // pdu.version is set by this parser:
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_igmp_query(p, pdu_len, pdu));
        break;

    case IPMC_IGMP_MSG_TYPE_V1JOIN:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_IGMP_V1;
        pdu.report.is_leave = false;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_igmp_report(p, pdu_len, pdu));
        break;

    case IPMC_IGMP_MSG_TYPE_V2JOIN:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_IGMP_V2;
        pdu.report.is_leave = false;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_igmp_report(p, pdu_len, pdu));
        break;

    case IPMC_IGMP_MSG_TYPE_V3JOIN:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_IGMP_V3;
        pdu.report.is_leave = false;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_igmp_report(p, pdu_len, pdu));
        break;

    case IPMC_IGMP_MSG_TYPE_LEAVE:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_IGMP_V2;
        pdu.report.is_leave = true;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_igmp_report(p, pdu_len, pdu));
        break;

    case IPMC_MLD_MSG_TYPE_QUERY:
        pdu.type = IPMC_LIB_PDU_TYPE_QUERY;

        // pdu.version is set by this parser:
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_mld_query(p, pdu_len, pdu));
        break;

    case IPMC_MLD_MSG_TYPE_V1REPORT:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_MLD_V1;
        pdu.report.is_leave = false;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_mld_report(p, pdu_len, pdu));
        break;

    case IPMC_MLD_MSG_TYPE_V2REPORT:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_MLD_V2;
        pdu.report.is_leave = false;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_mld_report(p, pdu_len, pdu));
        break;

    case IPMC_MLD_MSG_TYPE_DONE:
        pdu.type            = IPMC_LIB_PDU_TYPE_REPORT;
        pdu.version         = IPMC_LIB_PDU_VERSION_MLD_V1;
        pdu.report.is_leave = true;
        IPMC_LIB_PDU_RX_ACTION_RC(IPMC_LIB_PDU_rx_parse_mld_report(p, pdu_len, pdu));
        break;
    }

    // Final check in case the individual parsers haven't found it:
    // If p has exceeded the pdu_len by now, we have read more than we should
    // from the frame.
    if (p > &frm[pdu_offset] + pdu_len) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "The frame pointer is advanced to %u bytes after the MLD/IGMP message, even though only %u bytes are available", p - frm[pdu_offset], pdu_len);
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Parsed so far: %s", pdu);
        return IPMC_LIB_PDU_RX_ACTION_DISCARD;
    }

    T_DG(IPMC_LIB_TRACE_GRP_RX, "p - frm[pdu_offset] = %u, pdu_len = %u", p - &frm[pdu_offset], pdu_len);

    return IPMC_LIB_PDU_RX_ACTION_PROCESS;
}

/******************************************************************************/
// ipmc_lib_pdu_tx()
// Returns the number of packets sent.
/******************************************************************************/
uint32_t ipmc_lib_pdu_tx(const uint8_t *frame, size_t len, const mesa_port_list_t &dst_port_mask, bool force_untag, mesa_port_no_t src_port_no, mesa_vid_t vid, mesa_pcp_t pcp, mesa_dei_t dei)
{
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
    uint8_t                     *pkt_buf;
    uint32_t                    tx_len, aggr_idx, tx_cnt = 0;
    bool                        do_tag;
    mesa_port_no_t              port_no;
    mesa_packet_port_info_t     filter_info;
    packet_tx_props_t           tx_props;
    aggr_mgmt_group_member_t    aggr_members;
    mesa_port_list_t            tx_port_list(dst_port_mask);
    mesa_rc                     rc;

    T_DG(IPMC_LIB_TRACE_GRP_TX, "Tx %zu bytes %s frame (source port_no = %u) on VLAN %u with desired destination ports = %s", len, src_port_no == VTSS_PORT_NO_NONE ? "generated" : "forwarded", src_port_no, vid, dst_port_mask);

    // Don't send on each port of an aggregation.
    // We don't know, however, which port is the active forwarding member in
    // each aggregation, so if at least one of the ports in tx_port_list is
    // included in an aggregation, we must mark all ports in that aggregation in
    // the tx_port_list and let mesa_packet_port_filter_get() do the filtering,
    // since that function will only mark one port as forwarding in an
    // aggregation.
    for (aggr_idx = AGGR_MGMT_GROUP_NO_START; aggr_idx < AGGR_MGMT_GROUP_NO_END_; aggr_idx++) {
        vtss_clear(aggr_members);
        if (aggr_mgmt_members_get(VTSS_ISID_LOCAL, aggr_idx, &aggr_members, false) != VTSS_RC_OK) {
            // Aggregation index not active.
            continue;
        }

        if (src_port_no != VTSS_PORT_NO_NONE && aggr_members.entry.member[src_port_no]) {
            // The source port is a member of the aggregation. Don't Tx to any
            // ports in the aggregation.
            continue;
        }

        // Include Tx ports in the aggregation if at least one of the original
        // Tx ports is already member of the aggreagation.
        if ((dst_port_mask & aggr_members.entry.member).is_empty()) {
            // No original destination ports were members of the aggregation.
            continue;
        }

        tx_port_list |= aggr_members.entry.member;
    }

    if (tx_port_list.is_empty()) {
        T_DG(IPMC_LIB_TRACE_GRP_TX, "No Tx ports");
        return tx_cnt;
    }

    // Time to filter based on source port & VID.
    (void)mesa_packet_port_info_init(&filter_info);
    filter_info.port_no = src_port_no;
    filter_info.vid     = vid;

    if ((rc = mesa_packet_port_filter_get(NULL, &filter_info, filter.size(), filter.data())) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "mesa_packet_port_filter_get() failed: %s", error_txt(rc));
        return tx_cnt;
    }

    // Let the actual Tx size be at least 60 bytes (excluding FCS).
    // There is no need to allocate room for a possible VLAN tag, because
    // that's already handled by the packet module in the call to
    // packet_tx_alloc().
    tx_len = MAX(len, 60);

    if ((pkt_buf = packet_tx_alloc(tx_len)) == nullptr) {
        T_EG(IPMC_LIB_TRACE_GRP_TX, "Unable to allocate a Tx buffer of %u bytes", tx_len);
        return tx_cnt;
    }

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!tx_port_list[port_no]) {
            // Not in list of ports to Tx to.
            continue;
        }

        switch (filter[port_no].filter) {
        case MESA_PACKET_FILTER_DISCARD:
            // Port should not have frames transmitted on it.
            continue;

        case MESA_PACKET_FILTER_TAGGED:
            do_tag = !force_untag;
            break;

        case MESA_PACKET_FILTER_UNTAGGED:
            do_tag = false;
            break;

        default:
            T_EG(IPMC_LIB_TRACE_GRP_TX, "Unknown filter (%d) for port_no = %u", filter[port_no].filter, port_no);
            rc = VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
            goto do_exit;
        }

        // Even though we only allocate the frame once, we need to initialize
        // the tx_props and the frame contents for every frame, because
        // packet_tx() may modify them if e.g. tagging the frame.
        packet_tx_props_init(&tx_props);
        memcpy(pkt_buf, frame, len);

        // Clear remainder of frame - if any.
        if (tx_len > len) {
            memset(&pkt_buf[len], 0, tx_len - len);
        }

        tx_props.packet_info.modid     = VTSS_MODULE_ID_IPMC_LIB;
        tx_props.packet_info.frm       = pkt_buf;
        tx_props.packet_info.len       = tx_len;
        tx_props.packet_info.no_free   = true; // We handle the life-time of the frame.
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(port_no);

        // The old implementation double-tagged the code if filter.tpid was not
        // 0x8100. The outer tag was then filter.tpid and the inner tag was
        // 0x8100. I find that unusual and only tag once (with filter.tpid).

        // Ask the packet module to tag the frame. It does so when
        // tx_props.tx_info.switch_frm is false and tx_props.tx_info.tag.tpid is
        // not zero.
        if (do_tag) {
            tx_props.tx_info.tag.tpid = filter[port_no].tpid;
            tx_props.tx_info.tag.pcp  = pcp;
            tx_props.tx_info.tag.dei  = dei;
            tx_props.tx_info.tag.vid  = vid;
        }

        T_DG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "packet_tx() on VLAN %u. tx_props = %s", vid, tx_props);
        T_DG_HEX(IPMC_LIB_TRACE_GRP_TX, pkt_buf, tx_len);
        if ((rc = packet_tx(&tx_props)) != VTSS_RC_OK) {
            T_EG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "Unable to Tx frame of %u bytes. tx_props = %s", tx_len, tx_props);
            T_EG_HEX(IPMC_LIB_TRACE_GRP_TX, pkt_buf, tx_len);
            goto do_exit;
        } else {
            tx_cnt++;
        }
    }

do_exit:
    packet_tx_free(pkt_buf);
    return tx_cnt;
}

/******************************************************************************/
// ipmc_lib_pdu_our_ip_get()
/******************************************************************************/
void ipmc_lib_pdu_our_ip_get(const ipmc_lib_vlan_state_t &vlan_state, vtss_appl_ipmc_lib_ip_t &our_ip)
{
    if (vlan_state.vlan_key.is_ipv4) {
        our_ip = ipmc_lib_vlan_if_ipv4_get(vlan_state.vlan_key.vid, vlan_state.conf.querier_address);
    } else {
        our_ip = IPMC_LIB_PDU_link_local_ipv6;
    }
}

/******************************************************************************/
// ipmc_lib_pdu_init()
/******************************************************************************/
mesa_rc ipmc_lib_pdu_init(vtss_init_data_t *data)
{
    if (data->cmd != INIT_CMD_INIT) {
        return VTSS_RC_OK;
    }

    // Get the system MAC address
    (void)conf_mgmt_mac_addr_get(IPMC_LIB_PDU_system_mac.addr, 0);

    IPMC_LIB_PDU_all_nodes_ipv4.is_ipv4     = true;
    IPMC_LIB_PDU_all_nodes_ipv4.ipv4        = 0xe0000001; // 224.0.0.1
    IPMC_LIB_PDU_all_nodes_ipv6.is_ipv4     = false;
    IPMC_LIB_PDU_all_nodes_ipv6.ipv6        = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}; // ff02::1
    IPMC_LIB_PDU_all_routers_ipv4.is_ipv4   = true;
    IPMC_LIB_PDU_all_routers_ipv4.ipv4      = 0xe0000002; // 224.0.0.2
    IPMC_LIB_PDU_all_routers_ipv6.is_ipv4   = false;
    IPMC_LIB_PDU_all_routers_ipv6.ipv6      = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}; // ff02::2
    IPMC_LIB_PDU_all_routers_igmpv3.is_ipv4 = true;
    IPMC_LIB_PDU_all_routers_igmpv3.ipv4    = 0xe0000016; // 224.0.0.22
    IPMC_LIB_PDU_all_routers_mldv2.is_ipv4  = false;  // ff02::16
    IPMC_LIB_PDU_all_routers_mldv2.ipv6     = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16};// ff02::16

    // Create a link-local IPv6 address from the system MAC address
    vtss_ipv6_addr_link_local_get(&IPMC_LIB_PDU_link_local_ipv6.ipv6, &IPMC_LIB_PDU_system_mac);
    IPMC_LIB_PDU_link_local_ipv6.is_ipv4 = false;

    return VTSS_RC_OK;
}

/******************************************************************************/
// ipmc_lib_pdu_statistics_update()
/******************************************************************************/
void ipmc_lib_pdu_statistics_update(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, bool is_rx, bool ignored)
{
    if (pdu.type == IPMC_LIB_PDU_TYPE_QUERY) {
        IPMC_LIB_PDU_query_statistics_update(vlan_state, pdu, is_rx, ignored);
    } else {
        IPMC_LIB_PDU_report_statistics_update(vlan_state, pdu, is_rx, ignored);
    }
}

