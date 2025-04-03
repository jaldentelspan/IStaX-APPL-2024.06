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

#ifndef _IPMC_LIB_BASE_HXX_
#define _IPMC_LIB_BASE_HXX_

#include <vtss/basics/set.hxx>
#include <vtss/basics/map.hxx>
#include <vtss/appl/ipmc_lib.h>

// Number of ports on DUT.
extern uint32_t IPMC_LIB_port_cnt;

// vtss_appl_ipmc_lib_key_t::operator==()
static inline bool operator==(const vtss_appl_ipmc_lib_key_t &lhs, const vtss_appl_ipmc_lib_key_t &rhs)
{
    return lhs.is_mvr == rhs.is_mvr && lhs.is_ipv4 == rhs.is_ipv4;
}

// vtss_appl_ipmc_lib_key_t::operator!=()
static inline bool operator!=(const vtss_appl_ipmc_lib_key_t &lhs, const vtss_appl_ipmc_lib_key_t &rhs)
{
    return !(lhs == rhs);
}

// vtss_appl_ipmc_lib_vlan_key_t::operator==()
static inline bool operator==(const vtss_appl_ipmc_lib_vlan_key_t &lhs, const vtss_appl_ipmc_lib_vlan_key_t &rhs)
{
    return lhs.is_mvr == rhs.is_mvr && lhs.is_ipv4 == rhs.is_ipv4 && lhs.vid == rhs.vid;
}

// vtss_appl_ipmc_lib_vlan_key_t::operator!=()
static inline bool operator!=(const vtss_appl_ipmc_lib_vlan_key_t &lhs, const vtss_appl_ipmc_lib_vlan_key_t &rhs)
{
    return !(lhs == rhs);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_key_t::operator<()
// Used to sort entries in IPMC_LIB_vlan_map.
/******************************************************************************/
static inline bool operator<(const vtss_appl_ipmc_lib_vlan_key_t &lhs, const vtss_appl_ipmc_lib_vlan_key_t &rhs)
{
    // First sort by is_mvr (IPMC before MVR). This is in order to better
    // support IPMC vs MVR iterations.
    if (lhs.is_mvr != rhs.is_mvr) {
        return !lhs.is_mvr;
    }

    // Then sort by is_ipv4 (IPv4 before IPv6). This is in order to better
    // support IGMP vs MLD iterations.
    if (lhs.is_ipv4 != rhs.is_ipv4) {
        return lhs.is_ipv4;
    }

    // Finally sort by VID.
    return lhs.vid < rhs.vid;
}

// Forward declaration
struct ipmc_lib_vlan_state_s;

// This holds an ordered list of source addresses. We use vtss::Set rather than
// vtss::List, because we in this way remove duplicate members and make it
// easier to search for items.
struct ipmc_lib_src_list_t : public vtss::Set<vtss_appl_ipmc_lib_ip_t> {
    // A = B + C
    ipmc_lib_src_list_t operator+(const ipmc_lib_src_list_t &rhs) const;

    // A = B - C
    ipmc_lib_src_list_t operator-(const ipmc_lib_src_list_t &rhs) const;

    // A = B * C (intersection between this and C).
    ipmc_lib_src_list_t operator*(const ipmc_lib_src_list_t &rhs) const;
};

typedef ipmc_lib_src_list_t::iterator       ipmc_lib_src_list_itr_t;
typedef ipmc_lib_src_list_t::const_iterator ipmc_lib_src_list_const_itr_t;

typedef enum {
    IPMC_LIB_PDU_VERSION_IGMP_V1,
    IPMC_LIB_PDU_VERSION_IGMP_V2,
    IPMC_LIB_PDU_VERSION_IGMP_V3,
    IPMC_LIB_PDU_VERSION_MLD_V1,
    IPMC_LIB_PDU_VERSION_MLD_V2,
    IPMC_LIB_PDU_VERSION_CNT // Must come last
} ipmc_lib_pdu_version_t;

typedef struct {
    // This is a copy of ipmc_lib_pdu_t::version to make this structure
    // self-contained.
    ipmc_lib_pdu_version_t version;

    // Maximum response time measured in milliseconds.
    //
    // IGMP specifies the max response time in 10ths of seconds. When the query
    // PDU is parsed, the 10ths of seconds are converted to milliseconds, which
    // is what the max_response_time_ms member below is measured in.
    //
    // Analysis of the max_resonse_time ranges for each of the three IGMP
    // versions:
    // IGMPv1: The field does not exist in this version, and the max response
    //         time is fixed to 100 10ths of seconds = 10000 milliseconds.
    // IGMPv2: The 8-bit field is called "Max Response Time" and is used
    //         directly as max response time and must be in the range [1; 255]
    //         10ths of seconds, which translates to [100; 25500] milliseconds.
    // IGMPv3: The 8-bit field is called "Max Response Code" and is used either
    //         directly or interpreted as a floating point number (see
    //         IPMC_LIB_PDU_8bit_float_to_int()).
    //         If used directly, the range is [1; 127] 10ths of seconds, which
    //         is [100; 12700] milliseconds.
    //         If interpreted as floating point, the resulting range is
    //         [128; 31744] 10ths of seconds, which translates to
    //         [12800; 3174400] milliseconds.
    //         All in all, the range is [100; 3174400] milliseconds.
    //
    // MLD specifies the max response time directly in milliseconds.
    //
    // Analysis of the max_response_time_ms ranges for each of the two MLD
    // versions:
    // MLDv1: The 16-bit field is called "Maximum Response Delay" and is used
    //        directly as max response time. The range is therefore [1; 65535]
    //        milliseconds.
    // MLDv2: The 16-bit field is called "Max Response Code" and is used either
    //        directly or interpreted as a floating point number (see
    //         IPMC_LIB_PDU_16bit_float_to_int()).
    //         If used directly, the range is [1; 32767] milliseconds.
    //         If interpreted as floating point, the resulting range is
    //         [32768; 8387584] milliseconds (around 2.3 hours).
    //         All in all, the range is [1; 8387584] milliseconds.
    uint32_t max_response_time_ms;

    // Either 0.0.0.0/:: (General Query) or a valid IPv4/IPv6 multicast address
    // (Group-Specific Query).
    vtss_appl_ipmc_lib_ip_t grp_addr;

    // S Flag (Suppress Router-Side Processing)
    // IGMPv3 (RFC3376, 4.1.5) and MLDv2 (RFC3810, 5.1.7), only.
    bool s_flag;

    // QRV (Querier's Robustness Variable)
    // IGMPv3 (RFC3376, 4.1.6) and MLDv2 (RFC3810, 5.1.8), only.
    uint8_t qrv;

    // QQI (Querier's Query Interval Code, RFC3376, 4.1.7) measured in seconds
    // IGMPv3 (RFC3376, 4.1.7) and MLDv2 (RFC3810, 5.1.9), only.
    uint16_t qqi;

    // Source Addresses. List of source addresses in a Group-and-Source-
    // Specific query.
    // Size of array when used with IGMPv3:
    //   1518 - 18 (L2) - 24 (L3 + Router Alert) - 12 (IGMP query header) = 1464
    //   bytes left for IPv4 source addresses.
    //   This gives 1464 / 4 = 366 entries.
    // Size of array when used with MLDv2:
    //   1518 - 18 (L2) - 48 (L3 + Hop-by-Hop) - 24 (MLD query header) = 1424
    //   bytes left for IPv6 source addresses.
    //   This gives 1424 / 16 = 89 entries.
    ipmc_lib_src_list_t src_list;
} ipmc_lib_pdu_query_t;

// The Record Type for IGMPv3 Join messages (see RFC3376, 4.2.12).
typedef enum {
    IPMC_LIB_PDU_RECORD_TYPE_IS_IN = 1,
    IPMC_LIB_PDU_RECORD_TYPE_IS_EX = 2,
    IPMC_LIB_PDU_RECORD_TYPE_TO_IN = 3,
    IPMC_LIB_PDU_RECORD_TYPE_TO_EX = 4,
    IPMC_LIB_PDU_RECORD_TYPE_ALLOW = 5,
    IPMC_LIB_PDU_RECORD_TYPE_BLOCK = 6
} ipmc_lib_pdu_record_type_t;

const char *ipmc_lib_pdu_record_type_to_str(ipmc_lib_pdu_record_type_t record_type);

typedef struct {
    // The parser sets this flag if this particular group is valid. The thing is
    // that IGMPv3 and MLDv2 may contain multiple group records, and we process
    // those that we find valid and do not process those we don't.
    // If no valid group records are found, the packet is discarded or flooded
    // before it gets to the protocol handler. The protocol handler must not
    // process the group record if valid is false.
    bool valid;

    // If this is an IGMPv1, IMGPv2, or MLDv1 Join/Report PDU, the record type
    // is set to IPMC_LIB_PDU_RECORD_TYPE_IS_EX, which means "exclude the
    // sources listed in src_list". src_list is empty in this case, which
    // implies "exclude none", which is the same as "include all".
    // If this is an IGMPv2 Leave or MLDv1 Done PDU, the record type is always
    // set to IPMC_LIB_PDU_RECORD_TYPE_TO_IN, which means "exclude all
    // sources except for those listed in src_list". src_list is empty in
    // this case, which implies "include none", which is the same as "exclude
    // all".
    ipmc_lib_pdu_record_type_t record_type;

    // For IGMP:
    //   A valid IPv4 multicast address, except for 224.0.0.x addresses.
    //   The standard specifies that 224.0.0.0 is guaranteed not to be
    //   assigned to any group (RFC1112, 4.) and that 224.0.0.1 should never
    //   be reported (RFC1112, Appendix 1), but the old implementation of
    //   this module filters out all 224.0.0.x addresses. So do this version
    //   of the module - then.
    vtss_appl_ipmc_lib_ip_t grp_addr;

    // Source addresses.
    // Only used with IGMPv3 and MLDv2.
    // If IGMPv1, IGMPv2, or MLDv1, src_list.size() is 0.
    // See computations on maximum number of entries below.
    ipmc_lib_src_list_t src_list;
} ipmc_lib_pdu_group_record_t;

typedef struct {
    // This is a copy of ipmc_lib_pdu_t::version to make this structure
    // self-contained.
    ipmc_lib_pdu_version_t version;

    // If this is true, it's an IMGPv2 Leave or MLDv1 Done PDU in disguise.
    bool is_leave;

    // The number of valid entries in the group_recs[] array below.
    // This is exactly 1 for IGMPv1, IGMPv2, and MLDv1 PDUs, and may be greater
    // than 1 for IGMPv3 and MLDv2 PDUs.
    uint16_t rec_cnt;

    // The following structure supports all three IGMP and both MLD versions.
    // For IGMPv1, IGMPv2, and MLDv1 only a single entry is used.
    //
    // For IGMPv3 and MLDv2, we need to find a maximum of entries, we support.
    // Worstcase is for IGMPv3, which has 4 byte addresses rather than 16 byte
    // addresses, so we do the computations based on that.
    //
    // The maximum number of group entries for IGMPv3 is calculated as follows:
    //   An MTU is 1518 bytes. A minimum-sized group record is 8 bytes (in the
    //   case where no source addresses are specified. This gives the following
    //   maximum number of group records in a IGMPv3 join PDU:
    //   1518 (MTU) - 18 (L2) - 24 (L3 + Router Alert) - 8 (IGMP header) = 1468
    //   bytes left for group records.
    //   With the minimum group record size (8 bytes), this gives:
    //   1468 / 8 = 183 group records.
    //
    // The maximum number of source addresses that can be held in an IGMPv3 Join
    // PDU is when there is only one single group header:
    //   1518 (MTU) - 18 (L2) - 24 (L3 + Router Alert) - 8 (IGMP header) - 8
    //   (Group header) = 1460 bytes left for source addresses.
    //   With one source address being 4 bytes, this gives:
    //   1460 / 4 = 365 source addresses.
    //
    // I know that this structure becomes quite large (>128 KBytes), even though
    // we get the information from a PDU of at most 1518 bytes. I think this is
    // OK, because we allocate one single instance of the ipmc_lib_pdu_t
    // structure and always use that one. This is possible, because all IPMC
    // PDUs are received and handled by one single thread.
    ipmc_lib_pdu_group_record_t group_recs[183];
} ipmc_lib_pdu_report_t;

typedef enum {
    IPMC_LIB_PDU_RX_ACTION_PROCESS, // No errors.
    IPMC_LIB_PDU_RX_ACTION_DISCARD, // Ignore (discard) the PDU.
    IPMC_LIB_PDU_RX_ACTION_FLOOD,   // Don't use it, but flood it
} ipmc_lib_pdu_rx_action_t;

// Packet Rx parsing.
typedef enum {
    IPMC_LIB_PDU_TYPE_QUERY,
    IPMC_LIB_PDU_TYPE_REPORT, // Covers both IGMP Join, IGMP Leave, MLD Report, and MLD Done messages.
} ipmc_lib_pdu_type_t;

// The contents of this structure are taken from the following RFCs:
// IGMPv1: RFC1112
// IGMPv2: RFC2236
// IGMPv3: RFC3376
// MLDv1:  RFC2710
// MLDv2:  RFC3810
typedef struct {
    // Quick access to whether this structure holds an IGMP or MLD PDU.
    bool is_ipv4;

    // Rx meta info
    mesa_packet_rx_info_t rx_info;

    // Reference to the frame we have received. May be used in case we are
    // forwarding the frame.
    const uint8_t *frm;

    // What kind of IGMP/MLD PDU do we have here?
    ipmc_lib_pdu_type_t type;

    // What version of the IGMP/MLD standard is this PDU?
    ipmc_lib_pdu_version_t version;

    // Destination MAC address
    mesa_mac_t dmac;

    // Source MAC address
    mesa_mac_t smac;

    // Source IP address from IPv4/IPv6 header.
    vtss_appl_ipmc_lib_ip_t sip;

    // Destination IP address from IPv4/IPv6 header.
    vtss_appl_ipmc_lib_ip_t dip;

    // PDU contents. Unfortunately, this has to be two separate fields instead
    // of a union, because of its use of C++ constructors/destructors.

    // Structure used when type == IPMC_LIB_PDU_TYPE_QUERY.
    ipmc_lib_pdu_query_t query;

    // Structure used when type == IPMC_LIB_PDU_TYPE_REPORT.
    ipmc_lib_pdu_report_t report;
} ipmc_lib_pdu_t;

/******************************************************************************/
// This structure contains <G, S, port> specific state.
/******************************************************************************/
typedef struct {
    // Absolute time (in seconds since boot) that this source entry will time
    // out. Source timers are only active when they are members of the "Include
    // List".
    // It is an absolute time, because the owner of this structure's
    // next_src_timeout field must be an absolute time in order to compare with
    // "now", rather than making it a down-counter.
    uint32_t src_timeout;

    // Absolute time (in seconds since boot) that a group-and-source specific
    // query must be (re-)transmitted for this source.
    uint32_t query_timeout;

    // The number of times this source still needs to be retransmitted in a
    // group-and-source-specific query.
    uint32_t tx_cnt_left;
} ipmc_lib_src_port_state_t;

/******************************************************************************/
// This structure is used in two different places:
// 1) As value in a multicast group's ipmc_lib_grp_state_t::src_map,
//    representing the state of one source address.
// 2) As a value in a multicast group itself (ipmc_lib_grp_state_t::asm_state).
//    This is the Any-Source Multicast entry, which is a catch-all entry, that
//    is, an entry that catches traffic from sources not matched earlier in the
//    chip's matching process.
/******************************************************************************/
typedef struct {
    // When used in src_map:
    // ---------------------
    // Holds the time that the next source amongst all ports in ports[] times
    // out.
    // It is an absolute time (measured in seconds since boot), because we
    // cannot count it down on each tick, because in that case we don't know
    // which entry in ports[] timed out, and we don't want to count down all
    // entries in ports[] per tick.

    // When used in asm_state:
    // -----------------------
    // Not used, because the ASM entry doesn't have an associated source.
    uint32_t next_src_timeout;

    // When used in src_map and asm_state:
    // -----------------------------------
    // Holds the time that the next query should be sent. If this times out, the
    // code looks through all ports[] to see which one that timed out.
    // If this structure is used in src_map, a group-and-source specific query
    // will be transmitted, if the grp_compat allows for it.
    // If used in asm_state, a group-specific query will be transmitted.
    uint32_t next_query_timeout;

    // When used in src_map:
    // --------------------
    // If filter_mode is INCLUDE, this list is called the "Include List".
    // If filter_mode is EXCLUDE, this list is called the "Requested List".
    //
    // In INCLUDE mode, the "Include List"'s source timers are > 0 and hold an
    // absolute timeout. When the source timer expires, the source is deleted
    // from the "Include List".
    //
    // In EXCLUDE mode, the "Requested List"'s source timers are > 0 and hold an
    // absolute timeout. When the source timer expires, the source is moved from
    // the "Requested List" to the "Exclude List" (see exclude_port_list[]).
    //
    // When used as asm_state:
    // -----------------------
    // This is updated with the non-router ports that we actually transmit to.
    // It is not used by the protocol code itself, but may be used in management
    // interfaces that would like to show a brief status of a given M/C group.
    mesa_port_list_t include_port_list;

    // When used in src_map:
    // ---------------------
    // If a port's filter_mode is INCLUDE, exclude_port_list[port] is always
    // false (the list is not used).
    //
    // If a port's filter mode is EXCLUDE, this is called the "Exclude List". If
    // exclude_port_list[port] is true, then this entry is not being forwarded.
    //
    // When used in asm_state:
    // -----------------------
    // Not used.
    mesa_port_list_t exclude_port_list;

    // When used in src_map and asm_state:
    // -----------------------------------
    // Indicates whether the entry is stored in H/W - and if so, whether
    // it uses the TCAM or the MAC table.
    vtss_appl_ipmc_lib_hw_location_t hw_location;

    // When used in src_map:
    // ---------------------
    // When updating this source state, we use this boolean to mark it as
    // "something has happened". Its true-state is very short-lived.
    // When synchronizing with the chip, the function knows that it should do
    // something about this source address. The include_port_list and
    // exclude_port_list lists are at that point in time updated to reflect the
    // new state.
    //
    // When used in asm_state:
    // -----------------------
    // Also used when synchronizing the chip to indicate that "something has
    // happened", but unlike when used in src_map, the ASM entry is not based on
    // the include_port_list and exclude_port_list.
    bool changed;

    // Used in both src_map and asm_state.
    // ----------------------------------
    // See structure for details.
    CapArray<ipmc_lib_src_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;
} ipmc_lib_src_state_t;

/******************************************************************************/
// The following structure is a map with key = Source IP address and value =
// some state. It inherits from vtss::Map and adds a couple of fields to the
// source map itself, mainly to facilitate fast ticks, but also to figure out
// whether any entry has changed when receiving a report.
/******************************************************************************/
struct ipmc_lib_src_map_t : public vtss::Map<vtss_appl_ipmc_lib_ip_t, ipmc_lib_src_state_t> {
    // For fast lookups of next source timer timeout in this source map, we have
    // this one. It is 0 if no timers are active. It contains the absolute time
    // in seconds since boot of the next source timer timeout amongst all ports
    // that use this source.
    uint32_t next_src_timeout;

    // For fast lookups of next query timer timeout in this source map, we have
    // this one. It is 0 if no timers are active. It contains the absolute time
    // in seconds since boot of the next query timer timeout amongst all ports
    // that use this source.
    uint32_t next_query_timeout;

    // Temporary boolean to tell the function that synchronizes with the chip
    // that at least one entry in this map has changed as a result of receiving
    // a report.
    bool changed;
};

// Map with key = source address
typedef ipmc_lib_src_map_t::iterator       ipmc_lib_src_itr_t;
typedef ipmc_lib_src_map_t::const_iterator ipmc_lib_src_map_const_itr_t;

/******************************************************************************/
// This structure contains per-port group specific state.
/******************************************************************************/
typedef struct {
    // In IGMP, this is called the group timer (see RFC3376, section 6.5).
    // In MLD, this is called the filter timer (see RFC3810, section 7.2.2).
    //
    // Holds the absolute time (in seconds since boot) that this group times
    // out. Although we need the relative time from now to judge whether to set
    // or clear the "suppress source-side processing" flag (see RFC3376, section
    // 6.6.3.1), it is chosen to make it absolute, because it is also used to
    // set source timers, which must be absolute (see src_timeout and
    // next_src_timeout above).
    uint32_t grp_timeout;
} ipmc_lib_grp_port_state_t;

/******************************************************************************/
// This structure contains the entire state of a multicast group.
/******************************************************************************/
typedef struct {
    // List of ports on which we have received IGMP/MLD reports that haven't
    // yet timed out.
    mesa_port_list_t active_ports;

    // List of ports in EXCLUDE filter-mode
    // (VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE).
    //
    // The filter mode is described in RFC3376 section 6.2.1 and RFC3810 section
    // 7.2.1.
    //
    // Before we continue, we need to understand the term Any-source Multicast,
    // a.k.a. ASM.
    // An ASM entry is installed in the chip with a zero source address (0.0.0.0
    // in IPv4 and :: in IPv6).
    // MESA will take care of installing this last in the TCAM table, so that
    // source-specific entries are matched first.
    //
    // Let's see how this plays out with filter mode set to INCLUDE/EXCLUDE,
    // respectively (the filter mode is better described in RFC3810 than in
    // RFC3376).
    //
    // If a port's filter mode is INCLUDE, all listeners on the port are
    // interested *ONLY* in the source addresses marked as forwarding in
    // include_port_list[port]. Multicast frames to the group address with other
    // source addresses must be blocked.
    //
    // If a port's filter mode is EXCLUDE, the source addresses marked in
    // exclude_port_list[port] must not be forwarded to the port.
    // Multicast frames to the group address with other source addresses must be
    // forwarded to the port. The include_port_list[] is called the "Requested
    // List" in this case and has no effect on the forwarding.
    //
    // For each <grp, src> we add a TCAM entry, where the destination list is
    // constructed as follows:
    //    1) Add all router ports.
    //    2) Add all *active* ports in INCLUDE mode, where include_port_list[port] == true.
    //    3) Add all *active* ports in EXCLUDE mode, where exclude_port_list[port] == false.
    //
    // We also need an ASM entry (<grp, 0>), where the destination list is
    // constructed as follows:
    //    1) Add all router ports.
    //    2) Add all *active* ports in EXCLUDE mode.
    mesa_port_list_t exclude_mode_ports;

    // Group compatibility as described in RFC3376 section 7.3.2 and RFC3810
    // section 8.3.2.
    vtss_appl_ipmc_lib_compatibility_t grp_compat;

    // This corresponds to RFC3376 7.3.2 and 8.13's Older Version Host Present
    // Timer for IGMPv1 and is not used for MLD.
    // It contains the absolute time (in seconds since boot) before possibly
    // moving the grp_compat = GEN or SFM.
    uint32_t grp_older_version_host_present_timeout_old;

    // This corresponds to RFC3376 7.3.2 and 8.13's Older Version Host Present
    // Timer for IGMPv2 and RFC3810, 8.3.2 for MLDv1.
    // It contains the absolute time (in seconds since boot) before possibly
    // moving the grp_compat to SFM, unless the
    // grp_older_version_host_present_timeout_old is also running, in which case
    // it rules.
    uint32_t grp_older_version_host_present_timeout_gen;

    // This entry is used for the ASM entry. See definition of
    // ipmc_lib_src_state_t for a description.
    ipmc_lib_src_state_t asm_state;

    // Source addresses in this group. If empty, we're only using the
    // asm_state.
    ipmc_lib_src_map_t src_map;

    // Per-port state. See definition of ipmc_lib_grp_port_state_t for a
    // description.
    CapArray<ipmc_lib_grp_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;

    // Pointer that points back to the VLAN state owning this structure
    struct ipmc_lib_vlan_state_s *vlan_state;
} ipmc_lib_grp_state_t;

/******************************************************************************/
// IPMC and MVR share one single map that contains ALL groups. The map is
// indexed by <is_mvr, is_ipv4, vid> (to figure out to which instance this group
// belongs and by group address - making up the key.
/******************************************************************************/
typedef struct {
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    vtss_appl_ipmc_lib_ip_t       grp_addr;
} ipmc_lib_grp_key_t;

/******************************************************************************/
// For comparison (used by map.find() and friends).
// The map is sorted first by IP family, then by VLAN ID, and finally by IP
// address.
// By sorting by IP family first, management interfaces can ask for either IGMP
// or MLD and stop iterating whenever an IP family is met that doesn't match the
// one requested.
/******************************************************************************/
bool operator<(const ipmc_lib_grp_key_t &lhs, const ipmc_lib_grp_key_t &rhs);

typedef vtss::Map<ipmc_lib_grp_key_t, ipmc_lib_grp_state_t> ipmc_lib_grp_map_t;
typedef ipmc_lib_grp_map_t::iterator                        ipmc_lib_grp_itr_t;
typedef ipmc_lib_grp_map_t::const_iterator                  ipmc_lib_grp_const_itr_t;

// This is a map of proxied groups. The value is a porinter to the corresponding
// VLAN map.
typedef vtss::Map<ipmc_lib_grp_key_t, struct ipmc_lib_vlan_state_s *> ipmc_lib_proxy_grp_map_t;
typedef ipmc_lib_proxy_grp_map_t::iterator                            ipmc_lib_proxy_grp_map_itr_t;

// There is one such instance shared by both IPMC and MVR - as well as IGMP and
// MLD.
typedef struct {
    // Map that contains all groups.
    ipmc_lib_grp_map_t grp_map;

    // Map that contains all proxied groups to transmit reports for.
    // Only used by IPMC.
    ipmc_lib_proxy_grp_map_t proxy_grp_map;

    // Aggregation port masks. If there are no aggregations, these port masks
    // are empty. If e.g. port_no is != 7 and aggregated with port 7,
    // then aggr_port_masks[port_no][7] is set and other bits are cleared.
    CapArray<mesa_port_list_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> aggr_port_masks;
} ipmc_lib_global_lists_t;

typedef struct {
    // Indicates whether this global state is for IPMC/MVR and IGMP/MLD.
    vtss_appl_ipmc_lib_key_t key;

    // Capabilities for IPMC *and* MVR.
    vtss_appl_ipmc_lib_capabilities_t lib_cap;

    // Capabilities for IPMC *or* MVR.
    vtss_appl_ipmc_capabilities_t protocol_cap;

    // Global configuration
    vtss_appl_ipmc_lib_global_conf_t conf;

    // We suppress query flooding if we receive too much.
    uint32_t query_flooding_cnt;
    uint32_t query_suppression_timeout;

    // Static router ports (syncronized from port_conf.router).
    mesa_port_list_t static_router_ports;

    // Dynamic router ports
    mesa_port_list_t dynamic_router_ports;

    // Global per-port configuration
    CapArray<vtss_appl_ipmc_lib_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;

    // Global per-port status
    CapArray<vtss_appl_ipmc_lib_port_status_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_status;

    // Lists common to both IGMP and MLD.
    ipmc_lib_global_lists_t *lists;
} ipmc_lib_global_state_t;

typedef struct {
    // Querier stuff
    uint16_t proxy_query_timeout;
    uint32_t cur_rv;   // Currently used Robustness Variable
    uint32_t cur_qi;   // Currently used Query Interval measured in seconds.
    uint32_t cur_qri;  // Currently used Query Response Interval (Maximum Response Time). Measured in 10ths of a second
    uint32_t cur_lmqi; // Currently used Last Member Query Interval. Measured in 10ths of a second
    uint32_t startup_query_cnt_left;
    uint16_t proxy_report_timeout;
} ipmc_lib_vlan_internal_state_t;

typedef struct ipmc_lib_vlan_state_s {
    // Key indicating which <IPMC/MVR, IGMP/MLD, VLAN ID> this instance
    // represents.
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;

    // Configuration set by the end-user
    vtss_appl_ipmc_lib_vlan_conf_t conf;

    // Per-port-per-VLAN configuration (MVR only).
    CapArray<vtss_appl_ipmc_lib_vlan_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;

    // Status
    vtss_appl_ipmc_lib_vlan_status_t status;

    // Statistics
    vtss_appl_ipmc_lib_vlan_statistics_t statistics;

    // Internal state (makes it easier to clear when it's in its own struct).
    ipmc_lib_vlan_internal_state_t internal_state;

    // Global configuration and status identical for all instances.
    ipmc_lib_global_state_t *global;
} ipmc_lib_vlan_state_t;

void ipmc_lib_base_rx_pdu(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu);
mesa_port_list_t ipmc_lib_base_router_port_mask_get(ipmc_lib_global_state_t &glb);

// Tick functions:
//   grp_map_tick:      Once per second
//   proxy_report_tick: Once per second
//   vlan_tick:         Once per second per VLAN
//   global_tick:       Once per second per protocol (IPMC/MVR) per IP family (IGMP/MLD)
void ipmc_lib_base_grp_map_tick(ipmc_lib_grp_map_t &grp_map, uint32_t now);
void ipmc_lib_base_proxy_report_tick(ipmc_lib_proxy_grp_map_t &proxy_grp_map);
void ipmc_lib_base_vlan_tick(ipmc_lib_vlan_state_t &vlan_state, uint32_t now);
void ipmc_lib_base_global_tick(ipmc_lib_global_state_t &glb);

void ipmc_lib_base_vlan_state_init(ipmc_lib_vlan_state_t &vlan_state, bool clear_statistics);
void ipmc_lib_base_unregistered_flooding_update(ipmc_lib_global_state_t &glb);
void ipmc_lib_base_grp_cnt_max_update(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no);
void ipmc_lib_base_querier_state_update(ipmc_lib_vlan_state_t &vlan_state);
void ipmc_lib_base_compatibility_status_update(ipmc_lib_vlan_state_t &vlan_state, vtss_appl_ipmc_lib_compatibility_t old_compat);
void ipmc_lib_base_proxy_grp_map_clear(ipmc_lib_vlan_state_t &vlan_state);
void ipmc_lib_base_port_down(ipmc_lib_grp_map_t &grp_map, mesa_port_no_t port_no);
void ipmc_lib_base_router_status_update(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no, bool dynamic, bool add);
void ipmc_lib_base_deactivate(ipmc_lib_vlan_state_t &vlan_state);
void ipmc_lib_base_stp_forwarding_set(ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no);
void ipmc_lib_base_port_profile_changed(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no);
void ipmc_lib_base_vlan_profile_changed(ipmc_lib_vlan_state_t &vlan_state);
void ipmc_lib_base_vlan_compatible_mode_changed(ipmc_lib_vlan_state_t &vlan_state);
void ipmc_lib_base_vlan_port_role_changed(ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no);
void ipmc_lib_base_aggr_port_update(ipmc_lib_grp_map_t &grp_map, mesa_port_no_t port_no);

#endif /* _IPMC_LIB_BASE_HXX_ */

