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
#ifndef __MVR_SERIALIZER_HXX__
#define __MVR_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "mvr_expose.hxx"
#include <vtss/appl/mvr.h>
#include <vtss/appl/types.hxx>
#include "ipmc_lib_utils.hxx"

/*****************************************************************************
    Enumerator serializer
*****************************************************************************/
extern vtss_enum_descriptor_t ipmc_lib_expose_querier_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_ipmc_lib_querier_state_t,
                                "IpmcQuerierStatus",
                                ipmc_lib_expose_querier_state_txt,
                                "This enumeration indicates the Querier status for MVR VLAN interface.");

extern vtss_enum_descriptor_t ipmc_lib_expose_filter_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_ipmc_lib_filter_mode_t,
                                "IpmcFilterMode",
                                ipmc_lib_expose_filter_mode_txt,
                                "This enumeration indicates the group filter mode for an IPMC group address.");

extern vtss_enum_descriptor_t mvr_expose_port_role_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ipmc_lib_port_role_t,
                         "IpmcMvrVlanInterfacePortRole",
                         mvr_expose_port_role_txt,
                         "This enumeration indicates the MVR port's operational role.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_port_ifindex_t, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("PortIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_vlan_ifindex_t, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the VLAN interface.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_igmp_grp_addr_idx_t, mesa_ipv4_t, a, s)
{
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("GroupAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv4 multicast address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_igmp_src_addr_idx_t, mesa_ipv4_t, a, s)
{
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("HostAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv4 source address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_mld_grp_addr_idx_t, mesa_ipv6_t, a, s )
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("GroupAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv6 multicast address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_mld_src_addr_idx_t, mesa_ipv6_t, a, s )
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("HostAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv6 source address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mvr_serializer_ctrl_bool_t, BOOL, a, s)
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
void serialize(T &a, vtss_appl_ipmc_lib_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_global_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.admin_active),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable or disable MVR globally.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_port_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_port_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.fast_leave),
        vtss::tag::Name("DoImmediateLeave"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/disable the MVR immediate leave functionality.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_vlan_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_vlan_conf_t"));
    char              buf[50];
    int               ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.name, sizeof(s.name)),
        vtss::tag::Name("Name"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("MVR Name is an optional attribute to indicate the name "
                               "of this MVR VLAN allowing the user to easily associate "
                               "the MVR VLAN purpose with its name.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.querier_address.ipv4),
        vtss::tag::Name("IgmpQuerierAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The static IPv4 source address of the specific MVR interface "
                               "for seding IGMP Query message with respect to IGMP Querier election.")
    );

    sprintf(buf, "%s", ipmc_lib_util_compatible_mode_to_str(s.compatible_mode));
    m.add_leaf(
        vtss::AsDisplayString(buf, 11),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("In Dynamic mode, MVR allows dynamic MVR membership reports on "
                               "source ports. In Compatible mode, MVR membership reports are forbidden on source ports.")
    );

    m.add_leaf(
        vtss::AsBool(s.tx_tagged),
        vtss::tag::Name("Tagging"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify whether the IGMP/MLD control frames will be "
                               "transmitted tagged or untagged with MVR VLAN ID.")
    );

    m.add_leaf(
        s.pcp,
        vtss::tag::Name("Priority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify how the traversed IGMP/MLD control frames will be sent "
                               "in prioritized manner in VLAN tag.")
    );

    m.add_leaf(
        s.lmqi,
        vtss::tag::Name("LastListenerQueryInt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This setting Last Listener/Member Query Interval is used to "
                               "control IGMP protocol stack for fast aging mechanism. It defines the maximum time "
                               "to wait for IGMP/MLD report memberships on a port before removing the port from "
                               "multicast group membership. The value is in units of tenths of a seconds. The range "
                               "is from 0 to 31744.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.channel_profile.name, sizeof(s.channel_profile.name)),
        vtss::tag::Name("ChannelProfile"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The profile used for the channel filtering condition in the specific "
                               "MVR VLAN. Profile selected for designated interface channel is not allowed to have "
                               "overlapped permit group address by comparing with other MVR VLAN interface's channel.")
    );

    m.add_leaf(
        vtss::AsBool(s.querier_enable),
        vtss::tag::Name("QuerierElection"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the capability to run IGMP/MLD Querier election per-VLAN basis.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_vlan_port_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_vlan_port_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.role,
        vtss::tag::Name("Role"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Configure a MVR port of the designated MVR VLAN as one of the "
                               "following roles: Inactive, Source or Receiver. An inactive port does not participate "
                               "MVR operations. Configure uplink ports that receive and send multicast data as "
                               "a source port, and multicast subscribers cannot be directly connected to source ports. "
                               "Configure a port as a receiver port if it is a subscriber port and should only receive "
                               "multicast data, and a receiver port does not receive data unless it becomes a member of "
                               "the multicast group by issuing IGMP/MLD control messages. Be Caution: MVR source ports "
                               "are not recommended to be overlapped with management VLAN ports.")
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

    snprintf(buf, sizeof(buf), "The %s Querier status of this VLAN.", protocol_str);
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
            vtss::tag::Description("The active IGMP Querier address on this VLAN.")
        );
    } else {
        m.add_leaf(
            s.active_querier_address.ipv6,
            vtss::tag::Name("ActiveQuerierAddress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The active MLD Querier address on tis VLAN.")
        );
    }

    snprintf(buf, sizeof(buf), "Number of seconds this node has acted as %s Querier on this VLAN", protocol_str);
    m.add_leaf(
        s.querier_uptime,
        vtss::tag::Name("QuerierUptime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Time left (in seconds) until next query is sent on this VLAN if we are the active querier.");
    m.add_leaf(
        s.query_interval_left,
        vtss::tag::Name("QueryInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Other Querier Expire Time as stated in %s section %s on this VLAN.", rfc_str, is_ipv4 ? "8.5" : "9.5");
    m.add_leaf(
        s.other_querier_expiry_time,
        vtss::tag::Name("QuerierExpiryTime"),
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

    snprintf(buf, sizeof(buf), "Number of %s Query control frames transmitted on this VLAN.", protocol_str);
    m.add_leaf(
        s.tx_query,
        vtss::tag::Name("CounterTxQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Number of %s Group Query control frames transmitted on this VLAN.", protocol_str);
    m.add_leaf(
        s.tx_specific_query,
        vtss::tag::Name("CounterTxSpecificQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );

    snprintf(buf, sizeof(buf), "Number of %s Query control frames received on this VLAN.", protocol_str);
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
            vtss::tag::Description("Number of IGMPv1 Join frames received and processed on this VLAN.")
        );

        m.add_leaf(
            s.rx.igmp.utilized.v2_report + s.rx.igmp.ignored.v2_report,
            vtss::tag::Name("CounterRxV2Join"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv2 Join frames receives on this VLAN.")
        );

        m.add_leaf(
            s.rx.igmp.utilized.v2_leave + s.rx.igmp.ignored.v2_leave,
            vtss::tag::Name("CounterRxV2Leave"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv2 Leave frames received on this VLAN.")
        );

        m.add_leaf(
            s.rx.igmp.utilized.v3_report + s.rx.igmp.ignored.v3_report,
            vtss::tag::Name("CounterRxV3Join"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of IGMPv3 Join frames received on this VLAN.")
        );
    } else {
        m.add_leaf(
            s.rx.mld.utilized.v1_report + s.rx.mld.ignored.v1_report,
            vtss::tag::Name("CounterRxV1Report"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of MLDv1 Report frames received on this VLAN.")
        );

        m.add_leaf(
            s.rx.mld.utilized.v1_done + s.rx.mld.ignored.v1_done,
            vtss::tag::Name("CounterRxV1Done"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of MLDv1 Done frames received on this VLAN.")
        );

        m.add_leaf(
            s.rx.mld.utilized.v2_report + s.rx.mld.ignored.v2_report,
            vtss::tag::Name("CounterRxV2Report"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Number of MLDv2 Report frames received on this VLAN.")
        );
    }

    snprintf(buf, sizeof(buf), "Number of invalid %s control frames received on this VLAN.", protocol_str);
    m.add_leaf(
        s.rx_errors,
        vtss::tag::Name("CounterRxErrors"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(buf)
    );
}

template<typename T>
void serialize(T &a, mvr_expose_igmp_vlan_status_t &s)
{
    int ix = serialize(a, s.status, true); // vtss_appl_ipmc_lib_vlan_status_t
    serialize(a, s.statistics, ix, true);  // vtss_appl_ipmc_lib_vlan_statistics_t

}

template<typename T>
void serialize(T &a, mvr_expose_mld_vlan_status_t &s)
{
    int ix = serialize(a, s.status, false); // vtss_appl_ipmc_lib_vlan_status_t
    serialize(a, s.statistics, ix, false);  // vtss_appl_ipmc_lib_vlan_statistics_t
}

template<typename T>
void serialize(T &a, mvr_expose_grp_status_t &s)
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
    int               ix = 0;
    mesa_bool_t       b;

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
        vtss::tag::Description("Indicates whether this source address is forwarding or not")
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
        vtss::tag::Description("It true, the chip contain an entry with this source address.")
    );
}

namespace vtss
{
namespace appl
{
namespace mvr
{
namespace interfaces
{

struct MvrGlobalsConfig {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ipmc_lib_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipmc_lib_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mvr_global_conf_set);
    VTSS_EXPOSE_DEF_PTR(mvr_expose_global_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrPortConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_port_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing extra MVR helper features per port basis.";

    static constexpr const char *index_description =
        "Each port has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_port_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_port_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipmc_lib_port_itr);
    VTSS_EXPOSE_SET_PTR(mvr_expose_port_conf_set);
    VTSS_EXPOSE_DEF_PTR(mvr_expose_port_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrVlanConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_vlan_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing MVR VLAN interface entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_vlan_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_vlan_conf_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_vlan_itr);
    VTSS_EXPOSE_SET_PTR(mvr_expose_vlan_conf_set);
    VTSS_EXPOSE_ADD_PTR(mvr_expose_vlan_conf_set);
    VTSS_EXPOSE_DEL_PTR(mvr_expose_vlan_conf_del);
    VTSS_EXPOSE_DEF_PTR(mvr_expose_vlan_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrVlanPortConfigTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_vlan_port_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing MVR port roles of a specific MVR VLAN interface.";

    static constexpr const char *index_description =
        "Each port has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, mvr_serializer_port_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ipmc_lib_vlan_port_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_vlan_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_vlan_port_itr);
    VTSS_EXPOSE_SET_PTR(mvr_expose_vlan_port_conf_set);
    VTSS_EXPOSE_DEF_PTR(mvr_expose_vlan_port_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrIgmpVlanStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<mvr_expose_igmp_vlan_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the per VLAN interface status in MVR from IGMP protocol.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mvr_expose_igmp_vlan_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_igmp_vlan_status_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_vlan_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrMldVlanStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<mvr_expose_mld_vlan_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the per VLAN interface status in MVR from MLD protocol.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mvr_expose_mld_vlan_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_mld_vlan_status_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_vlan_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrIgmpGrpTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv4_t *>,
         vtss::expose::ParamVal<mvr_expose_grp_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the registered IPv4 multicast group address status in IGMP from MVR.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, mvr_serializer_igmp_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mvr_expose_grp_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_igmp_grp_status_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_igmp_grp_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrMldGrpTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv6_t *>,
         vtss::expose::ParamVal<mvr_expose_grp_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the registered IPv6 multicast group address status in MLD from MVR.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, mvr_serializer_mld_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mvr_expose_grp_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_mld_grp_status_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_mld_grp_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrIgmpSrcTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv4_t *>,
         vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv4_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_src_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the address SFM (a.k.a Source List Multicast) status in source list of the registered IPv4 multicast group in IGMP from MVR.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, mvr_serializer_igmp_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, mvr_serializer_port_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, mvr_serializer_igmp_src_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ipmc_lib_src_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_igmp_src_status_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_igmp_src_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrMldSrcTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv6_t *>,
         vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamKey<mesa_ipv6_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_src_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying the address SFM (a.k.a Source List Multicast) status in source list of the registered IPv6 multicast group in MLD from MVR.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, mvr_serializer_mld_grp_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, mvr_serializer_port_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, mvr_serializer_mld_src_addr_idx_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ipmc_lib_src_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(mvr_expose_mld_src_status_get);
    VTSS_EXPOSE_ITR_PTR(mvr_expose_mld_src_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};

struct MvrStatisticsClear {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<BOOL *>
         > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mvr_serializer_vlan_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, mvr_serializer_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(mvr_expose_vlan_itr);
    VTSS_EXPOSE_GET_PTR(mvr_expose_vlan_statistics_dummy_get);
    VTSS_EXPOSE_SET_PTR(mvr_expose_vlan_statistics_clear);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR);
};
}  // namespace interfaces
}  // namespace mvr
}  // namespace appl
}  // namespace vtss

#endif /* __MVR_SERIALIZER_HXX__ */

