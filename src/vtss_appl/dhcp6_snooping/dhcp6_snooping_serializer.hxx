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
#ifndef __VTSS_DHCP6_SNOOPING_SERIALIZER_HXX__
#define __VTSS_DHCP6_SNOOPING_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dhcp6_snooping.h"
#include "vtss_appl_formatting_tags.hxx"
#include "dhcp6_snooping_expose.h"

/*****************************************************************************
Enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t dhcp6_snooping_mode_txt[];
extern const vtss_enum_descriptor_t dhcp6_snooping_nh_unknown_mode_txt[];
extern const vtss_enum_descriptor_t dhcp6_snooping_port_trust_mode_txt[];

VTSS_XXXX_SERIALIZE_ENUM(
    dhcp6_system_mode_t,
    "Dhcp6SnoopingModeEnum",
    dhcp6_snooping_mode_txt,
    "This enumeration defines the mode of snooping.");

VTSS_XXXX_SERIALIZE_ENUM(
    dhcp6_nh_unknown_mode_t,
    "Dhcp6SnoopingUnknownModeEnum",
    dhcp6_snooping_nh_unknown_mode_txt,
    "This enumeration defines the mode of operation when meeting a unknown IPv6 next header.");

VTSS_XXXX_SERIALIZE_ENUM(
    dhcp6_port_trust_mode_t,
    "Dhcp6SnoopingPortTrustModeEnum",
    dhcp6_snooping_port_trust_mode_txt,
    "This enumeration defines the trust mode of a port.");

struct DuidHasher
{
    DuidHasher(vtss_appl_dhcp6_snooping_duid_t &duidval)
    {
        auto duid = dhcp_duid_t(duidval);
        str_hash = (uint32_t)std::hash<std::string>{}(duid.to_string());
    }

    uint32_t get_hash() { return str_hash; }

private:
    uint32_t str_hash = 0;
};

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(dhcp6_snooping_ifindex_index, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp6_snooping_duid_index, vtss_appl_dhcp6_snooping_duid_t, a, s ) {
    auto hasher = DuidHasher(s.inner);
    a.add_leaf(
        hasher.get_hash(),
        vtss::tag::Name("ClientDuidHash"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Client DUID hash value.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp6_snooping_iaid_index, vtss_appl_dhcp6_snooping_iaid_t, a, s ) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("InterfaceIaid"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Interface Identity Association Identifier.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp6_snooping_vid_index, mesa_vid_t, a, s ) {
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
void serialize(T &a, vtss_appl_dhcp6_snooping_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6_snooping_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.snooping_mode,
        vtss::tag::Name("SnoopingMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP snooping mode operation. "
            "Possible modes are - enable: Enable DHCP snooping mode operation. "
            "When DHCP snooping mode operation is enabled, the DHCP request "
            "messages will be forwarded to trusted ports and only allow reply "
            "packets from trusted ports. disable: Disable DHCP snooping mode "
            "operation.")
    );
    
    m.add_leaf(
        s.nh_unknown_mode,
        vtss::tag::Name("UnknownNextHeaderMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP snooping mode operation. "
            "Possible modes are - allow: Allow packets with unknown IPv6 ext. headers. "
            " drop: Drop packets with unknown IPv6 ext. header.")
    );


}

template<typename T>
void serialize(T &a, vtss_appl_dhcp6_snooping_port_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6_snooping_port_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.trust_mode,
        vtss::tag::Name("TrustMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP snooping port mode. "
            "Possible port modes are - trusted: Configures the port as trusted "
            "source of the DHCP messages. untrusted: Configures the port as "
            "untrusted source of the DHCP messages.")
    );
}

template<typename T>
void serialize(T & a, vtss_appl_dhcp6_snooping_global_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6_snooping_global_status_t"));
    int ix = 1;

    m.add_leaf(
        s.last_change_ts,
        vtss::tag::Name("LastChangeTs"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Timestamp for last change to snooping tables content.")
    );
}

template<typename T>
void serialize(T & a, vtss_appl_dhcp6_snooping_client_info_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6_snooping_client_info_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString((char *)dhcp_duid_t(s.duid).to_string().c_str(), 2*DHCP6_DUID_MAX_SIZE),
        vtss::tag::Name("ClientDuid"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client DUID.")
    );
    m.add_leaf(
        s.mac,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client MAC address.")
    );
    m.add_leaf(
        vtss::AsInterfaceIndex(s.if_index),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

template<typename T>
void serialize(T & a, vtss_appl_dhcp6_snooping_assigned_ip_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6_snooping_assigned_ip_t"));
    int ix = 0;

    m.add_leaf(
        s.ip_address,
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The assigned IPv6 address.")
    );
    m.add_leaf(
        s.iaid,
        vtss::tag::Name("Iaid"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The interface IAID.")
    );
    m.add_leaf(
        s.vid,
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(1, 4095),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN id of the VLAN.")
    );
    m.add_leaf(
        s.lease_time,
        vtss::tag::Name("LeaseTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The lease time assigned to the address.")
    );
    m.add_leaf(
        s.dhcp_server_ip,
        vtss::tag::Name("DhcpServerIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("IP address of the DHCP server that assigns the IP address.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp6_snooping_port_statistics_t &s)
{
    const int TxOidOffset = 100;
    const int CmdOidOffset = 200;
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6_snooping_port_statistics_t"));
    int ix = 0;

    m.add_leaf(
        s.rx.solicit,
        vtss::tag::Name("RxSolicit"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received SOLICIT packets.")
    );

    m.add_leaf(
        s.rx.request,
        vtss::tag::Name("RxRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received REQUEST packets.")
    );

    m.add_leaf(
        s.rx.infoRequest,
        vtss::tag::Name("RxInfoRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received INFOREQUEST packets.")
    );

    m.add_leaf(
        s.rx.confirm,
        vtss::tag::Name("RxConfirm"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received CONFIRM packets.")
    );

    m.add_leaf(
        s.rx.renew,
        vtss::tag::Name("RxRenew"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received RENEW packets.")
    );

    m.add_leaf(
        s.rx.rebind,
        vtss::tag::Name("RxRebind"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received REBIND packets.")
    );

    m.add_leaf(
        s.rx.decline,
        vtss::tag::Name("RxDecline"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received DECLINE packets.")
    );

    m.add_leaf(
        s.rx.advertise,
        vtss::tag::Name("RxAdvertise"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received ADVERTISE packets.")
    );

    m.add_leaf(
        s.rx.reply,
        vtss::tag::Name("RxReply"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received REPLY packetsreceived.")
    );

    m.add_leaf(
        s.rx.reconfigure,
        vtss::tag::Name("RxReconfigure"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received RECONFIGURE packetsreceived.")
    );

    m.add_leaf(
        s.rx.release,
        vtss::tag::Name("RxRelease"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received RELEASE packetsreceived.")
    );

    m.add_leaf(
        s.rxDiscardUntrust,
        vtss::tag::Name("RxDiscardUntrust"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of received packets that was discarded as port was untrusted.")
    );

    // Make an offset for the TX part so that we can add more RX stat counters later on.
    ix = TxOidOffset;
    m.add_leaf(
        s.tx.solicit,
        vtss::tag::Name("TxSolicit"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted SOLICIT packets.")
    );

    m.add_leaf(
        s.tx.request,
        vtss::tag::Name("TxRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted REQUEST packets.")
    );

    m.add_leaf(
        s.tx.infoRequest,
        vtss::tag::Name("TxInfoRequest"),
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted INFOREQUEST packets.")
    );

    m.add_leaf(
        s.tx.confirm,
        vtss::tag::Name("TxConfirm"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted CONFIRM packets.")
    );

    m.add_leaf(
        s.tx.renew,
        vtss::tag::Name("TxRenew"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted RENEW packets.")
    );

    m.add_leaf(
        s.tx.rebind,
        vtss::tag::Name("TxRebind"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted REBIND packets.")
    );

    m.add_leaf(
        s.tx.decline,
        vtss::tag::Name("TxDecline"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted DECLINE packets.")
    );

    m.add_leaf(
        s.tx.advertise,
        vtss::tag::Name("TxAdvertise"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted ADVERTISE packets.")
    );

    m.add_leaf(
        s.tx.reply,
        vtss::tag::Name("TxReply"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted REPLY packets.")
    );

    m.add_leaf(
        s.tx.reconfigure,
        vtss::tag::Name("TxReconfigure"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted RECONFIGURE packets.")
    );

    m.add_leaf(
        s.tx.release,
        vtss::tag::Name("TxRelease"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::MaxAccess::ReadOnly,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of transmitted RELEASE packets.")
    );

    ix = CmdOidOffset;

    m.add_leaf(
        s.clear_stats,
        vtss::tag::Name("ClearStats"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Clear statistics counters for this port.")
    );

}

namespace vtss {
namespace appl {
namespace dhcp6_snooping {
namespace interfaces {
struct Dhcp6SnoopingParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp6_snooping_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6_snooping_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp6_snooping_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6_SNOOPING);
};

struct Dhcp6SnoopingInterfaceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_port_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of DHCPv6 Snooping port configuration parameters";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Interface"));
        serialize(h, dhcp6_snooping_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6_snooping_port_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("Parameter"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6_snooping_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6_snooping_port_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp6_snooping_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6_SNOOPING);
};

struct Dhcp6SnoopingGlobalStatusImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_global_status_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp6_snooping_global_status_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP6_SNOOPING);
    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6_snooping_global_status_get);
};

struct Dhcp6SnoopingClientInfoEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_dhcp6_snooping_duid_t *>,
        vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_client_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of known DHCPv6 clients";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp6_snooping_duid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_snooping_duid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6_snooping_client_info_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(dhcp6_snooping_expose_client_info_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6_snooping_client_info_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP6_SNOOPING);
};

struct Dhcp6SnoopingAddressInfoEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_dhcp6_snooping_duid_t *>,
        vtss::expose::ParamKey<vtss_appl_dhcp6_snooping_iaid_t *>,
        vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_assigned_ip_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of addresses for known DHCPv6 clients";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp6_snooping_duid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_snooping_duid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6_snooping_iaid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, dhcp6_snooping_iaid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_dhcp6_snooping_assigned_ip_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(dhcp6_snooping_expose_assigned_ip_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6_snooping_assigned_ip_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP6_SNOOPING);
};

struct Dhcp6SnoopingInterfaceStatisticsEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_port_statistics_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of port statistics in DHCPv6 Snooping ";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_snooping_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6_snooping_port_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6_snooping_port_statistics_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp6_snooping_port_statistics_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6_snooping_port_statistics_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP6_SNOOPING);
};

}  // namespace interfaces
}  // namespace aggr
}  // namespace dhcp_snooping
}  // namespace vtss


#endif /* __VTSS_DHCP6_SNOOPING_SERIALIZER_HXX__ */
