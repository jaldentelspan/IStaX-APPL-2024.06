/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __DHCP6_RELAY_SERIALIZER_HXX__
#define __DHCP6_RELAY_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dhcp6_relay.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"
#include "dhcp6_relay.h"


/*****************************************************************************
  - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(DHCP6_RELAY_vlan_interface, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("VlanInterface"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Vlan Interface."));
}

VTSS_SNMP_TAG_SERIALIZE(DHCP6_RELAY_relay_interface, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("RelayVlanInterface"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Relay Vlan Interface."));
}


template <typename T>
void serialize(T &a, vtss_appl_dhcpv6_relay_vlan_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_dhcpv6_relay_vlan_t"));
    int ix = 0;
    m.add_leaf(s.relay_destination, vtss::tag::Name("RelayDestination"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Ipv6 address of the DHCP server that requests are being relayed to. "
                   "The default address is the multicast address "
                   "ALL_DHCP_SERVERS (FF05::1:3)"
                                      ));
}

template <typename T>
void serialize(T &a, vtss_appl_dhcpv6_relay_intf_missing_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcpv6_relay_intf_missing_t"));
    int ix = 0;
    m.add_leaf(
        s.num, 
        vtss::tag::Name("NumIntfMissing"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Number of server packets dropped because interface option is missing.")
                                );
}

template <typename T>
void serialize(T &a, vtss_appl_dhcpv6_relay_vlan_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcpv6_relay_vlan_statistics_t"));
    int ix = 0;
    m.add_leaf(
        s.tx_to_server,
        vtss::tag::Name("TxToServer"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets sent to dhcpv6 server from vlan.")
        );

    m.add_leaf(
        s.rx_from_server,
        vtss::tag::Name("RxFromServer"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets received from dhcpv6 server on vlan.")
        );

    m.add_leaf(
        s.server_pkt_dropped,
        vtss::tag::Name("ServerPktDropped"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets from dhcpv6 server to vlan dropped.")
        );
    
    m.add_leaf(
        s.tx_to_client,
        vtss::tag::Name("TxToClient"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets sent to dhcpv6 client from vlan.")
        );

    m.add_leaf(
        s.rx_from_client,
        vtss::tag::Name("RxFromClient"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets received from dhcpv6 client on vlan.")
        );
    
     m.add_leaf(
        s.client_pkt_dropped,
        vtss::tag::Name("ClientPktDropped"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets from dhcpv6 client to vlan dropped.")
        );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcpv6_relay_control_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcpv6_relay_control_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.clear_all_stats),
        vtss::tag::Name("ClearAllStatistics"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The action to clear all statistics. True "
            "clears the statistics data. False does nothing.")
    );
}

namespace vtss {
namespace appl {
namespace dhcp6_relay {

struct Dhcp6RelayVlanConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_vlan_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure Dhcp6_Relay for a specific vlan.";

    static constexpr const char *index_description =
            "Each vlan interface can be configured for dhcp relay";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, DHCP6_RELAY_vlan_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, DHCP6_RELAY_relay_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_dhcpv6_relay_vlan_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6_RELAY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcpv6_relay_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcpv6_relay_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_dhcpv6_relay_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_dhcpv6_relay_conf_del);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcpv6_relay_vlan_conf_itr);
};

struct Dhcp6RelayVlanStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_vlan_t *>> P;

    static constexpr const char *table_description =
            "This is a table containing active Dhcp6_Relay agents.";

    static constexpr const char *index_description =
            "vlan interface being serviced by relay agent";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, DHCP6_RELAY_vlan_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, DHCP6_RELAY_relay_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_dhcpv6_relay_vlan_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6_RELAY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcpv6_relay_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcpv6_relay_vlan_status_itr);
};

struct Dhcp6RelayVlanStatisticsEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_vlan_statistics_t *>> P;

    static constexpr const char *table_description =
            "This is a table to containing statistics for Dhcp6_Relay for a specific vlan. Statistics can be cleared";

    static constexpr const char *index_description =
            "Vlan interface being serviced by relay agent";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, DHCP6_RELAY_vlan_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, DHCP6_RELAY_relay_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_dhcpv6_relay_vlan_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP6_RELAY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcpv6_relay_vlan_statistics_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcpv6_relay_vlan_statistics_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcpv6_relay_vlan_statistics_itr);
};

struct Dhcp6RelayControlLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_control_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcpv6_relay_control_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcpv6_relay_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcpv6_relay_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct Dhcp6RelayInterfaceMissinglLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_intf_missing_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcpv6_relay_intf_missing_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcpv6_relay_interface_missing_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};


}  // namespace dhcp6_relay
}  // namespace appl
}  // namespace vtss
#endif
