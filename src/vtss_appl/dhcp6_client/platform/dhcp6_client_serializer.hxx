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
#ifndef __VTSS_DHCP6_CLIENT_SERIALIZER_HXX__
#define __VTSS_DHCP6_CLIENT_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dhcp6_client.h"
#include "vtss/appl/types.hxx"

/*****************************************************************************
    Data type serializer
*****************************************************************************/

mesa_rc vtss_appl_dhcp6c_interface_restart_dummy_get(vtss_ifindex_t *const act_value);
/*****************************************************************************
    Enumerator serializer
*****************************************************************************/

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(dhcp6_client_ifindex_idx, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description(
            "Logical interface number of the VLAN interface.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_dhcp6c_capabilities_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6c_capabilities_t"));
    int                 ix = 0;

    m.add_leaf(
        s.max_number_of_interfaces,
        vtss::tag::Name("MaxNumberOfInterfaces"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum number of DHCPv6 client interfaces supported by the device.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp6c_intf_conf_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6c_intf_conf_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.rapid_commit),
        vtss::tag::Name("RapidCommit"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Enable/Disable the rapid-commit capability in DHCPv6 message exchanges.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp6c_interface_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6c_interface_t"));
    int                 ix = 0;
    u64                 timestamp;

    m.add_leaf(
        s.address,
        vtss::tag::Name("Address"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The IPv6 address determined from DHCPv6 for this interface.")
    );

    m.add_leaf(
        s.srv_addr,
        vtss::tag::Name("ServerAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The IPv6 address of the bounded DHCPv6 server for this interface.")
    );

    m.add_leaf(
        s.dns_srv_addr,
        vtss::tag::Name("DnsServerAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The DNS server address retrieved from DHCPv6.")
    );

    timestamp = s.timers.preferred_lifetime.sec_msb;
    timestamp <<= 32;
    timestamp |= s.timers.preferred_lifetime.seconds;
    m.add_leaf(
        vtss::AsCounter(timestamp),
        vtss::tag::Name("PreferredLifetime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The recorded Preferred-Lifetime for the DHCPv6 client interface.  "
            "From RFC-4862 and RFC-3315: It is the preferred lifetime for the IPv6 address, expressed "
            "in units of seconds.  When the preferred lifetime expires, the address becomes deprecated.")
    );

    timestamp = s.timers.valid_lifetime.sec_msb;
    timestamp <<= 32;
    timestamp |= s.timers.valid_lifetime.seconds;
    m.add_leaf(
        vtss::AsCounter(timestamp),
        vtss::tag::Name("ValidLifetime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The recorded Valid-Lifetime for the DHCPv6 client interface.  "
            "From RFC-4862 and RFC-3315: It is the valid lifetime for the IPv6 address, expressed "
            "in units of seconds.  The valid lifetime must be greater than or equal to the preferred "
            "lifetime.  When the valid lifetime expires, the address becomes invalid.")
    );

    timestamp = s.timers.t1.sec_msb;
    timestamp <<= 32;
    timestamp |= s.timers.t1.seconds;
    m.add_leaf(
        vtss::AsCounter(timestamp),
        vtss::tag::Name("T1"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The recorded T1 for the DHCPv6 client interface.  "
            "From RFC-3315: It is the time at which the client contacts the server from which the address "
            "is obtained to extend the lifetimes of the non-temporary address assigned; T1 is a time "
            "duration relative to the current time expressed in units of seconds.")
    );

    timestamp = s.timers.t2.sec_msb;
    timestamp <<= 32;
    timestamp |= s.timers.t2.seconds;
    m.add_leaf(
        vtss::AsCounter(timestamp),
        vtss::tag::Name("T2"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The recorded T2 for the DHCPv6 client interface.  "
            "From RFC-3315: It is the time at which the client contacts any available server to "
            "extend the lifetimes of the non-temporary address assigned; T2 is a time duration "
            "relative to the current time expressed in units of seconds.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp6c_intf_cntr_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp6c_intf_cntr_t"));
    int                 ix = 0;

    m.add_leaf(
        s.tx_solicit,
        vtss::tag::Name("TxSolicit"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 SOLICIT message count.")
    );

    m.add_leaf(
        s.tx_request,
        vtss::tag::Name("TxRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 REQUEST message count.")
    );

    m.add_leaf(
        s.tx_confirm,
        vtss::tag::Name("TxConfirm"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 CONFIRM message count.")
    );

    m.add_leaf(
        s.tx_renew,
        vtss::tag::Name("TxRenew"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 RENEW message count.")
    );

    m.add_leaf(
        s.tx_rebind,
        vtss::tag::Name("TxRebind"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 REBIND message count.")
    );

    m.add_leaf(
        s.tx_release,
        vtss::tag::Name("TxRelease"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 RELEASE message count.")
    );

    m.add_leaf(
        s.tx_decline,
        vtss::tag::Name("TxDecline"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 DECLINE message count.")
    );

    m.add_leaf(
        s.tx_information_request,
        vtss::tag::Name("TxInfoRequest"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 INFORMATION-REQUEST message count.")
    );

    m.add_leaf(
        s.tx_error,
        vtss::tag::Name("TxError"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 message error count.")
    );

    m.add_leaf(
        s.tx_drop,
        vtss::tag::Name("TxDrop"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 message drop count.")
    );

    m.add_leaf(
        s.tx_unknown,
        vtss::tag::Name("TxUnknown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Transmitted DHCPv6 unknown message type count.")
    );


    m.add_leaf(
        s.rx_advertise,
        vtss::tag::Name("RxAdvertise"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Received DHCPv6 ADVERTISE message count.")
    );

    m.add_leaf(
        s.rx_reply,
        vtss::tag::Name("RxReply"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Received DHCPv6 REPLY message count.")
    );

    m.add_leaf(
        s.rx_reconfigure,
        vtss::tag::Name("RxReconfigure"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Received DHCPv6 RECONFIGURE message count.")
    );

    m.add_leaf(
        s.rx_error,
        vtss::tag::Name("RxError"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Received DHCPv6 message error count.")
    );

    m.add_leaf(
        s.rx_drop,
        vtss::tag::Name("RxDrop"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Received DHCPv6 message drop count.")
    );

    m.add_leaf(
        s.rx_unknown,
        vtss::tag::Name("RxUnknown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Received DHCPv6 unknown message type count.")
    );
}

/*****************************************************************************
    Serializer interface
*****************************************************************************/
namespace vtss
{
namespace appl
{
namespace dhcp6_client
{
namespace interfaces
{

struct Dhcp6ClientCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_dhcp6c_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp6c_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6c_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP6C);
};

struct Dhcp6ClientInterfaceConfigurationTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_dhcp6c_intf_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing DHCPv6 client interface entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_client_ifindex_idx(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6c_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6c_interface_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6c_interface_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp6c_interface_config_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_dhcp6c_interface_config_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_dhcp6c_interface_config_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6C);
};

struct Dhcp6ClientInterfaceInformationTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_dhcp6c_interface_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying per DHCPv6 client interface information derived from DHCPv6 server.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_client_ifindex_idx(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6c_interface_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6c_interface_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6c_interface_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6C);
};

struct Dhcp6ClientInterfaceStatisticsTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_dhcp6c_intf_cntr_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying per DHCPv6 client interface control message statistics in DHCPv6 message exchanges.";

    static constexpr const char *index_description =
        "Each entry has a set of counters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_client_ifindex_idx(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp6c_intf_cntr_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6c_interface_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp6c_interface_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP6C);
};

struct Dhcp6ClientActionLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_ifindex_t *>
        > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp6_client_ifindex_idx(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp6c_interface_restart_dummy_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp6c_interface_restart_act);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6C);
};

}  // namespace interfaces
}  // namespace dhcp6_client
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_DHCP6_CLIENT_SERIALIZER_HXX__ */
