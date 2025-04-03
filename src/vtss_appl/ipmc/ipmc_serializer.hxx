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

#ifndef __IPMC_SERIALIZER_HXX__
#define __IPMC_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "ipmc_expose.hxx"
#include <vtss/appl/ipmc_lib.h>
#include <vtss/appl/types.hxx>

/*****************************************************************************
    Enumerator serializer
*****************************************************************************/

extern vtss_enum_descriptor_t ipmc_expose_router_status_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ipmc_lib_router_status_t,
                         "IpmcSnoopingRouterPortStatusEnum",
                         ipmc_expose_router_status_txt,
                         "This enumeration indicates the router port status from IGMP/MLD snooping.");

extern vtss_enum_descriptor_t ipmc_lib_expose_querier_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_ipmc_lib_querier_state_t,
                                "IpmcQuerierStatus",
                                ipmc_lib_expose_querier_state_txt,
                                "This enumeration indicates the Querier status for IGMP/MLD snooping VLAN interface.");

extern vtss_enum_descriptor_t ipmc_lib_expose_filter_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_ipmc_lib_filter_mode_t,
                                "IpmcFilterMode",
                                ipmc_lib_expose_filter_mode_txt,
                                "This enumeration indicates the group filter mode for an IPMC group address.");

extern vtss_enum_descriptor_t ipmc_snp4_compatiType_txt[];
VTSS_XXXX_SERIALIZE_ENUM(ipmc_expose_compati4_t,
                         "IpmcSnoopingIgmpInterfaceCompatibilityEnum",
                         ipmc_snp4_compatiType_txt,
                         "This enumeration indicates the version compatibility for IGMP snooping VLAN interface.");

extern vtss_enum_descriptor_t ipmc_snp6_compatiType_txt[];
VTSS_XXXX_SERIALIZE_ENUM(ipmc_expose_compati6_t,
                         "IpmcSnoopingMldInterfaceCompatibilityEnum",
                         ipmc_snp6_compatiType_txt,
                         "This enumeration indicates the version compatibility for MLD snooping VLAN interface.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_port_ifindex_idx_t, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("PortIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_vlan_ifindex_t, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the VLAN interface.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_igmp_grp_addr_idx_t, mesa_ipv4_t, a, s)
{
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("GroupAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv4 multicast address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_igmp_src_addr_idx_t, mesa_ipv4_t, a, s)
{
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("HostAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv4 source address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_mld_grp_addr_idx_t, mesa_ipv6_t, a, s )
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("GroupAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv6 multicast address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_mld_src_addr_idx_t, mesa_ipv6_t, a, s )
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("HostAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv6 source address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_serializer_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Set to TRUE to clear the counters of this VLAN."));
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_global_conf_t &s, bool is_ipv4)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_global_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.admin_active),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the IGMP snooping global functionality.")
    );

    m.add_leaf(
        vtss::AsBool(s.unregistered_flooding_enable),
        vtss::tag::Name("UnregisteredFlooding"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the control for flooding unregistered IPv4 multicast traffic.")
    );

    if (is_ipv4) {
        m.add_leaf(
            vtss::AsIpv4(s.ssm_prefix.ipv4),
            vtss::tag::Name("SsmRangeAddress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The address prefix value defined for IGMP SSM service model.")
        );
    } else {
        m.add_leaf(
            s.ssm_prefix.ipv6,
            vtss::tag::Name("SsmRangeAddress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The address prefix value defined for MLD SSM service model.")
        );
    }

    m.add_leaf(
        s.ssm_prefix_len,
        vtss::tag::Name("SsmRangeMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The address prefix length defined for MLD/IGMP SSM service model.")
    );

    m.add_leaf(
        vtss::AsBool(s.proxy_enable),
        vtss::tag::Name("Proxy"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the MLD/IGMP proxy functionality.")
    );

    m.add_leaf(
        vtss::AsBool(s.leave_proxy_enable),
        vtss::tag::Name("LeaveProxy"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the MLD/IGMP leave-proxy functionality.")
    );
}

template<typename T>
void serialize(T &a, ipmc_expose_igmp_global_conf_t &s)
{
    serialize(a, s.global_conf, true); // vtss_appl_ipmc_lib_global_conf_t
}

template<typename T>
void serialize(T &a, ipmc_expose_mld_global_conf_t &s)
{
    serialize(a, s.global_conf, false); // vtss_appl_ipmc_lib_global_conf_t
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_port_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_port_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.router,
        vtss::tag::Name("AsRouterPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the IGMP/MLD static router port functionality.")
    );

    m.add_leaf(
        vtss::AsBool(s.fast_leave),
        vtss::tag::Name("DoFastLeave"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the IGMP/MLD fast leave functionality.")
    );

    m.add_leaf(
        s.grp_cnt_max,
        vtss::tag::Name("ThrottlingNumber"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum number of groups to be registered on the specific port.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.profile_key.name, sizeof(s.profile_key.name)),
        vtss::tag::Name("FilteringProfile"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The profile used for IGMP filtering per-port basis.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_vlan_conf_t &s, bool is_ipv4, ipmc_expose_compati4_t &compati4, ipmc_expose_compati6_t &compati6)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_vlan_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.admin_active),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable snooping on this VLAN.")
    );

    m.add_leaf(
        vtss::AsBool(s.querier_enable),
        vtss::tag::Name("QuerierElection"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the capability to run IGMP/MLD Querier election per-VLAN basis.")
    );

    if (is_ipv4) {
        m.add_leaf(
            vtss::AsIpv4(s.querier_address.ipv4),
            vtss::tag::Name("QuerierAddress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The static IPv4 source address of the specific IGMP interface "
                                   "for seding IGMP Query message with respect to IGMP Querier election.")
        );

        m.add_leaf(
            compati4,
            vtss::tag::Name("Compatibility"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The compatibility control for IGMP snooping to run the "
                                   "corresponding protocol version.")
        );
    } else {
        ix++; // Skip one
        m.add_leaf(
            compati6,
            vtss::tag::Name("Compatibility"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The compatibility control for MLD snooping to run the "
                                   "corresponding protocol version.")
        );
    }

    m.add_leaf(
        s.pcp,
        vtss::tag::Name("Priority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting is used as PCP value when "
                               "transmitting tagged IGMP/MLD control frames.")
    );

    m.add_leaf(
        s.rv,
        vtss::tag::Name("Rv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting Robustness Variable is used to control IGMP/MLD "
                               "protocol stack as stated in RFC-3376 8.1.")
    );

    m.add_leaf(
        s.qi,
        vtss::tag::Name("Qi"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting Query Interval is used to control IGMP/MLD "
                               "protocol stack as stated in RFC-3376 8.2.")
    );

    m.add_leaf(
        s.qri,
        vtss::tag::Name("Qri"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting Query Response Interval is used to control "
                               "IGMP/MLD protocol stack as stated in RFC-3376 8.3.")
    );

    m.add_leaf(
        s.lmqi,
        vtss::tag::Name("Lmqi"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting Last Member Query Interval is used to "
                               "control IGMP/MLD protocol stack as stated in RFC-3376 8.8.")
    );

    m.add_leaf(
        s.uri,
        vtss::tag::Name("Uri"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting Unsolicited Report Interval is used to "
                               "control IGMP/MLD protocol stack as stated in RFC-3376 8.11.")
    );
}

template<typename T>
void serialize(T &a, ipmc_expose_igmp_vlan_conf_t &s)
{
    ipmc_expose_compati6_t dummy;
    serialize(a, s.conf, true, s.compati4, dummy); // vtss_appl_ipmc_lib_vlan_conf_t
}

template<typename T>
void serialize(T &a, ipmc_expose_mld_vlan_conf_t &s)
{
    ipmc_expose_compati4_t dummy;
    serialize(a, s.conf, false, dummy, s.compati6); // vtss_appl_ipmc_lib_vlan_conf_t
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_port_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_port_status_t"));
    int ix = 0;

    m.add_leaf(
        s.router_status,
        vtss::tag::Name("Status"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The IGMP/MLD snooping router port status.")
    );
}

template<typename T>
int serialize(T &a, vtss_appl_ipmc_lib_vlan_status_t &s, bool is_ipv4)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_vlan_status_t"));
    int               ix = 0;
    char              buf[200];
    const char        *protocol_str = is_ipv4 ? "IGMP"    : "MLD";
    const char        *rfc_str      = is_ipv4 ? "RFC3376" : "RFC3810";
    uint32_t          timeout;

    snprintf(buf, sizeof(buf), "The %s Querier status of this VLAN interface.", protocol_str);
    m.add_leaf(
        s.querier_state,
        vtss::tag::Name("QuerierStatus"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    if (is_ipv4) {
        m.add_leaf(
            vtss::AsIpv4(s.active_querier_address.ipv4),
            vtss::tag::Name("ActiveQuerierAddress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The active IGMP Querier address on this VLAN interface.")
        );
    } else {
        m.add_leaf(
            s.active_querier_address.ipv6,
            vtss::tag::Name("ActiveQuerierAddress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The active MLD Querier address on this VLAN interface.")
        );
    }

    snprintf(buf, sizeof(buf), "Number of seconds this node has acted as %s Querier on this VLAN interface.", protocol_str);
    m.add_leaf(
        s.querier_uptime,
        vtss::tag::Name("QuerierUptime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Time left (in seconds) until next query is sent on this VLAN interface if we are the active querier.");
    m.add_leaf(
        s.query_interval_left,
        vtss::tag::Name("QueryInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Querier Expire Time as stated in %s section %s on this VLAN interface.", rfc_str, is_ipv4 ? "8.5" : "9.5");
    m.add_leaf(
        s.other_querier_expiry_time,
        vtss::tag::Name("QuerierExpiryTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "The current %s version that this VLAN interface behaves like when running %s protocol as a router.", protocol_str, protocol_str);
    m.add_leaf(
        ipmc_expose_compatibility_to_old_version(s.querier_compat, is_ipv4),
        vtss::tag::Name("QuerierVersion"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    if (s.older_version_querier_present_timeout_old) {
        timeout = s.older_version_querier_present_timeout_old;
    } else {
        timeout = s.older_version_querier_present_timeout_gen;
    }

    snprintf(buf, sizeof(buf), "Older Version Querier Present Timeout as stated in %s section %s on this VLAN interface.", rfc_str, is_ipv4 ? "8.12" : "9.12");
    m.add_leaf(
        timeout,
        vtss::tag::Name("QuerierPresentTimeout"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Current %s compatibility protocol setting for listener ports in this VLAN.", protocol_str);
    m.add_leaf(
        ipmc_expose_compatibility_to_old_version(s.host_compat, is_ipv4),
        vtss::tag::Name("HostVersion"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    if (s.older_version_host_present_timeout_old) {
        timeout = s.older_version_host_present_timeout_old;
    } else {
        timeout = s.older_version_host_present_timeout_gen;
    }

    snprintf(buf, sizeof(buf), "Old Version Host Present Interval as stated in %s section %s on this VLAN interface.", rfc_str, is_ipv4 ? "8.13" : "9.13");
    m.add_leaf(
        timeout,
        vtss::tag::Name("HostPresentTimeout"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    return ix;
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_vlan_statistics_t &s, int ix, bool is_ipv4)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_vlan_statistics_t")); // Keep the tag name like this
    char              buf[200];
    const char        *protocol_str = is_ipv4 ? "IGMP" : "MLD";

    snprintf(buf, sizeof(buf), "Number of %s Query control frames transmitted on this VLAN interface.", protocol_str);
    m.add_leaf(
        s.tx_query,
        vtss::tag::Name("CounterTxQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Number of %s Group Query control frames transmitted on this VLAN interface.", protocol_str);
    m.add_leaf(
        s.tx_specific_query,
        vtss::tag::Name("CounterTxSpecificQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Number of %s Query control frames received on this VLAN interface.", protocol_str);
    m.add_leaf(
        s.rx_query,
        vtss::tag::Name("CounterRxQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    if (is_ipv4) {
        m.add_leaf(
            s.rx.igmp.utilized.v1_report + s.rx.igmp.ignored.v1_report,
            vtss::tag::Name("CounterRxV1Join"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv1 Join frames received and processed on this VLAN interface.")
        );

        m.add_leaf(
            s.rx.igmp.utilized.v2_report + s.rx.igmp.ignored.v2_report,
            vtss::tag::Name("CounterRxV2Join"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv2 Join frames receives on this VLAN interface.")
        );

        m.add_leaf(
            s.rx.igmp.utilized.v2_leave + s.rx.igmp.ignored.v2_leave,
            vtss::tag::Name("CounterRxV2Leave"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv2 Leave frames received on this VLAN interface.")
        );

        m.add_leaf(
            s.rx.igmp.utilized.v3_report + s.rx.igmp.ignored.v3_report,
            vtss::tag::Name("CounterRxV3Join"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv3 Join frames received on this VLAN interface.")
        );
    } else {
        m.add_leaf(
            s.rx.mld.utilized.v1_report + s.rx.mld.ignored.v1_report,
            vtss::tag::Name("CounterRxV1Report"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of MLDv1 Report frames received on this VLAN interface.")
        );

        m.add_leaf(
            s.rx.mld.utilized.v1_done + s.rx.mld.ignored.v1_done,
            vtss::tag::Name("CounterRxV1Done"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of MLDv1 Done frames received on this VLAN interface.")
        );

        m.add_leaf(
            s.rx.mld.utilized.v2_report + s.rx.mld.ignored.v2_report,
            vtss::tag::Name("CounterRxV2Report"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of MLDv2 Report frames received on this VLAN interface.")
        );
    }

    snprintf(buf, sizeof(buf), "Number of invalid %s control frames received on this VLAN interface.", protocol_str);
    m.add_leaf(
        s.rx_errors,
        vtss::tag::Name("CounterRxErrors"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );
}

template<typename T>
void serialize(T &a, ipmc_expose_igmp_vlan_status_t &s)
{
    int ix = serialize(a, s.status, true); // vtss_appl_ipmc_lib_vlan_status_t
    serialize(a, s.statistics, ix, true);  // vtss_appl_ipmc_lib_vlan_statistics_t

}

template<typename T>
void serialize(T &a, ipmc_expose_mld_vlan_status_t &s)
{
    int ix = serialize(a, s.status, false); // vtss_appl_ipmc_lib_vlan_status_t
    serialize(a, s.statistics, ix, false);  // vtss_appl_ipmc_lib_vlan_statistics_t
}

template<typename T>
void serialize(T &a, ipmc_expose_grp_status_t &s)
{
    typename T::Map_t       m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_grp_status_t"));
    int                     ix = 0;
    mesa_bool_t             b;
    vtss::PortListStackable &list = (vtss::PortListStackable &)s.port_list_stackable;

    m.add_leaf(
        list,
        vtss::tag::Name("MemberPorts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contains the listener ports of this multicast address")
    );

    b = s.status.hw_location != VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE;
    m.add_leaf(
        vtss::AsBool(b),
        vtss::tag::Name("HardwareSwitch"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, the chip contains an entry with this group address")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_src_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_src_status_t"));
    int         ix = 0;
    mesa_bool_t b;

    m.add_leaf(
        s.filter_mode,
        vtss::tag::Name("GroupFilterMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The filter mode of this multicast group on this port.")
    );

    m.add_leaf(
        s.grp_timeout,
        vtss::tag::Name("FilterTimer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of seconds until this group on this port times out (only used if filter mode is EXCLUDE).")
    );

    m.add_leaf(
        vtss::AsBool(s.forwarding),
        vtss::tag::Name("SourceType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates whether this source address is forwarding or not.")
    );

    m.add_leaf(
        s.src_timeout,
        vtss::tag::Name("SourceTimer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of seconds until this source times out.")
    );

    b = s.hw_location != VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE;
    m.add_leaf(
        vtss::AsBool(b),
        vtss::tag::Name("HardwareFilter"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, the chip contains an entry with this source address.")
    );
}

namespace vtss
{
namespace appl
{
namespace ipmc
{
namespace interfaces
{

struct IpmcIgmpGlobalsConfig {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<ipmc_expose_igmp_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(ipmc_expose_igmp_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_global_conf_get);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_igmp_global_conf_set);
    VTSS_EXPOSE_DEF_PTR(ipmc_expose_igmp_global_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldGlobalsConfig {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<ipmc_expose_mld_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(ipmc_expose_mld_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_global_conf_get);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_mld_global_conf_set);
    VTSS_EXPOSE_DEF_PTR(ipmc_expose_mld_global_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcIgmpPortConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_port_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing extra IGMP snooping helper features per port basis";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_port_ifindex_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_port_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipmc_lib_port_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_igmp_port_conf_set);
    VTSS_EXPOSE_DEF_PTR(ipmc_expose_igmp_port_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldPortConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_port_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing extra MLD snooping helper features per port basis";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_port_ifindex_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_port_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipmc_lib_port_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_mld_port_conf_set);
    VTSS_EXPOSE_DEF_PTR(ipmc_expose_mld_port_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcIgmpVlanConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<ipmc_expose_igmp_vlan_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing IGMP Snooping VLAN interface entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(ipmc_expose_igmp_vlan_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_vlan_conf_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_igmp_vlan_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_igmp_vlan_conf_set);
    VTSS_EXPOSE_DEF_PTR(ipmc_expose_igmp_vlan_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldVlanConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<ipmc_expose_mld_vlan_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing MLD Snooping VLAN interface entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(ipmc_expose_mld_vlan_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_vlan_conf_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_mld_vlan_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_mld_vlan_conf_set);
    VTSS_EXPOSE_DEF_PTR(ipmc_expose_mld_vlan_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcIgmpPortStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_port_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the IGMP port status.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_port_ifindex_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_port_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipmc_lib_port_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldPortStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_port_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the MLD router port status.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_port_ifindex_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_port_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipmc_lib_port_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcIgmpVlanStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<ipmc_expose_igmp_vlan_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the per VLAN interface status in IGMP snooping configuration.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(ipmc_expose_igmp_vlan_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_vlan_status_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_igmp_vlan_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldVlanStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<ipmc_expose_mld_vlan_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the per VLAN interface status in MLD snooping configuration.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(ipmc_expose_mld_vlan_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_vlan_status_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_mld_vlan_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcIgmpGrpTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv4_t *>,
         vtss::expose::ParamVal<ipmc_expose_grp_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the registered IPv4 multicast group address status from IGMP snooping.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_serializer_igmp_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(ipmc_expose_grp_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_grp_status_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_igmp_grp_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldGrpTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv6_t *>,
         vtss::expose::ParamVal<ipmc_expose_grp_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the registered IPv6 multicast group address status from MLD snooping.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_serializer_mld_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(ipmc_expose_grp_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_grp_status_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_mld_grp_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};


struct IpmcIgmpSrcTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv4_t *>,
         vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv4_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_src_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the address SFM (a.k.a Source List Multicast) status in source list of the registered IPv4 multicast group from IGMP snooping.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_serializer_igmp_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, ipmc_serializer_port_ifindex_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, ipmc_serializer_igmp_src_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ipmc_lib_src_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_igmp_src_status_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_igmp_src_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldSrcTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv6_t *>,
         vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv6_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_src_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the address SFM (a.k.a Source List Multicast) status in source list of the registered IPv6 multicast group from MLD snooping.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_serializer_mld_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, ipmc_serializer_port_ifindex_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, ipmc_serializer_mld_src_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ipmc_lib_src_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_expose_mld_src_status_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_expose_mld_src_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcIgmpStatisticsClear {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<BOOL *>
         > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_serializer_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(ipmc_expose_igmp_vlan_itr);
    VTSS_EXPOSE_GET_PTR(ipmc_expose_vlan_statistics_dummy_get);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_igmp_vlan_statistics_clear);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcMldStatisticsClear {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<BOOL *>
         > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_serializer_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(ipmc_expose_mld_vlan_itr);
    VTSS_EXPOSE_GET_PTR(ipmc_expose_vlan_statistics_dummy_get);
    VTSS_EXPOSE_SET_PTR(ipmc_expose_mld_vlan_statistics_clear);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC);
};

}  // namespace interfaces
}  // namespace ipmc
}  // namespace appl
}  // namespace vtss
#endif /* __IPMC_SERIALIZER_HXX__ */

