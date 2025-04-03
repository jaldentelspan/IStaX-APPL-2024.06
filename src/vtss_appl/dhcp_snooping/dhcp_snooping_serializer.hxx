/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_DHCP_SNOOPING_SERIALIZER_HXX__
#define __VTSS_DHCP_SNOOPING_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dhcp_snooping.h"
#include "vtss_appl_formatting_tags.hxx"

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(dhcp_snooping_ifindex_index, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_snooping_mac_index, mesa_mac_t, a, s ) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("MAC address.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_snooping_vid_index, mesa_vid_t, a, s ) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(1, 4095),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The VLAN id of the VLAN.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_dhcp_snooping_param_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_snooping_param_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP snooping mode operation. "
            "Possible modes are - true: Enable DHCP snooping mode operation. "
            "When DHCP snooping mode operation is enabled, the DHCP request "
            "messages will be forwarded to trusted ports and only allow reply "
            "packets from trusted ports. false: Disable DHCP snooping mode "
            "operation.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_snooping_port_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_snooping_port_config_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.trustMode),
        vtss::tag::Name("TrustMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP snooping port mode. "
            "Possible port modes are - true: Configures the port as trusted "
            "source of the DHCP messages. false: Configures the port as "
            "untrusted source of the DHCP messages.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_snooping_assigned_ip_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_snooping_assigned_ip_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifIndex),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Logical interface number of the physical port "
            "of the DHCP client.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.ipAddr),
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("IP address assigned to DHCP client by DHCP server.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.netmask),
        vtss::tag::Name("Netmask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Netmask assigned to DHCP client by DHCP server.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.dhcpServerIp),
        vtss::tag::Name("DhcpServerIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("IP address of the DHCP server that assigns "
            "the IP address and netmask.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_snooping_port_statistics_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_snooping_port_statistics_t"));
    int ix = 0;

    m.add_leaf(
        s.rxDiscover,
        vtss::tag::Name("RxDiscover"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of discover (option 53 with value 1) packets received.")
    );

    m.add_leaf(
        s.rxOffer,
        vtss::tag::Name("RxOffer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of offer (option 53 with value 2) packets received.")
    );

    m.add_leaf(
        s.rxRequest,
        vtss::tag::Name("RxRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of request (option 53 with value 3) packets received.")
    );

    m.add_leaf(
        s.rxDecline,
        vtss::tag::Name("RxDecline"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of decline (option 53 with value 4) packets received.")
    );

    m.add_leaf(
        s.rxAck,
        vtss::tag::Name("RxAck"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of ACK (option 53 with value 5) packets received.")
    );

    m.add_leaf(
        s.rxNak,
        vtss::tag::Name("RxNak"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of NAK (option 53 with value 6) packets received.")
    );

    m.add_leaf(
        s.rxRelease,
        vtss::tag::Name("RxRelease"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of release (option 53 with value 7) packets received.")
    );

    m.add_leaf(
        s.rxInform,
        vtss::tag::Name("RxInform"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of inform (option 53 with value 8) packets received.")
    );

    m.add_leaf(
        s.rxLeaseQuery,
        vtss::tag::Name("RxLeaseQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease query (option 53 with value 10) packets received.")
    );

    m.add_leaf(
        s.rxLeaseUnassigned,
        vtss::tag::Name("RxLeaseUnassigned"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease unassigned (option 53 with value 11) packets received.")
    );

    m.add_leaf(
        s.rxLeaseUnknown,
        vtss::tag::Name("RxLeaseUnknown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease unknown (option 53 with value 12) packets received.")
    );

    m.add_leaf(
        s.rxLeaseActive,
        vtss::tag::Name("RxLeaseActive"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease active (option 53 with value 13) packets received.")
    );

    m.add_leaf(
        s.rxDiscardChksumErr,
        vtss::tag::Name("RxDiscardChksumErr"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of discard packet that IP/UDP checksum is error.")
    );

    m.add_leaf(
        s.rxDiscardUntrust,
        vtss::tag::Name("RxDiscardUntrust"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of discard packet that are coming from untrusted port.")
    );

    m.add_leaf(
        s.txDiscover,
        vtss::tag::Name("TxDiscover"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of discover (option 53 with value 1) packets transmited.")
    );

    m.add_leaf(
        s.txOffer,
        vtss::tag::Name("TxOffer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of offer (option 53 with value 2) packets transmited.")
    );

    m.add_leaf(
        s.txRequest,
        vtss::tag::Name("TxRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of request (option 53 with value 3) packets transmited.")
    );

    m.add_leaf(
        s.txDecline,
        vtss::tag::Name("TxDecline"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of decline (option 53 with value 4) packets transmited.")
    );

    m.add_leaf(
        s.txAck,
        vtss::tag::Name("TxAck"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of ACK (option 53 with value 5) packets transmited.")
    );

    m.add_leaf(
        s.txNak,
        vtss::tag::Name("TxNak"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of NAK (option 53 with value 6) packets transmited.")
    );

    m.add_leaf(
        s.txRelease,
        vtss::tag::Name("TxRelease"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of release (option 53 with value 7) packets transmited.")
    );

    m.add_leaf(
        s.txInform,
        vtss::tag::Name("TxInform"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of inform (option 53 with value 8) packets transmited.")
    );

    m.add_leaf(
        s.txLeaseQuery,
        vtss::tag::Name("TxLeaseQuery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease query (option 53 with value 10) packets transmited.")
    );

    m.add_leaf(
        s.txLeaseUnassigned,
        vtss::tag::Name("TxLeaseUnassigned"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease unassigned (option 53 with value 11) packets transmited.")
    );

    m.add_leaf(
        s.txLeaseUnknown,
        vtss::tag::Name("TxLeaseUnknown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease unknown (option 53 with value 12) packets transmited.")
    );

    m.add_leaf(
        s.txLeaseActive,
        vtss::tag::Name("TxLeaseActive"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of lease active (option 53 with value 13) packets transmited.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_snooping_clear_port_statistics_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_snooping_clear_port_statistics_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.clearPortStatistics),
        vtss::tag::Name("Clear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("clear statistics per physical port.")
    );
}

namespace vtss {
namespace appl {
namespace dhcp_snooping {
namespace interfaces {
struct DhcpSnoopingParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_snooping_param_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_snooping_param_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_snooping_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_snooping_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpSnoopingInterfaceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_snooping_port_config_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of DHCP Snooping port configuration parameters";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_snooping_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_snooping_port_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_snooping_port_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_snooping_port_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_snooping_port_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpSnoopingAssignedIpEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_mac_t>,
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_snooping_assigned_ip_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of assigned IP information in DHCP Snooping ";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_mac_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_snooping_mac_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, dhcp_snooping_vid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_dhcp_snooping_assigned_ip_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_snooping_assigned_ip_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_snooping_assigned_ip_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpSnoopingInterfaceStatisticsEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_snooping_port_statistics_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of port statistics in DHCP Snooping ";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_snooping_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_snooping_port_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_snooping_port_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_snooping_port_statistics_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpSnoopingInterfaceClearStatisticsEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_snooping_clear_port_statistics_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear port statistics in DHCP Snooping";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_snooping_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_snooping_clear_port_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_snooping_clear_port_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_snooping_port_statistics_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_snooping_clear_port_statistics_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};
}  // namespace interfaces
}  // namespace aggr
}  // namespace dhcp_snooping
}  // namespace vtss


#endif /* __VTSS_DHCP_SNOOPING_SERIALIZER_HXX__ */
