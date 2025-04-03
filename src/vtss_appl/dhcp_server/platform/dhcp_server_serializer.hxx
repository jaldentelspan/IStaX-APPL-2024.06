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
#ifndef __VTSS_DHCP_SERVER_SERIALIZER_HXX__
#define __VTSS_DHCP_SERVER_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dhcp_server.h"

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t dhcp_server_pool_type_txt[];
extern const vtss_enum_descriptor_t dhcp_server_netbios_node_type_txt[];
extern const vtss_enum_descriptor_t dhcp_server_client_identifier_type_txt[];
extern const vtss_enum_descriptor_t dhcp_server_binding_type_txt[];
extern const vtss_enum_descriptor_t dhcp_server_binding_state_txt[];

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_dhcp_server_pool_type_t,
    "DhcpServerPoolEnum",
    dhcp_server_pool_type_txt,
    "This enumeration defines the type of DHCP pool.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_dhcp_server_netbios_node_type_t,
    "DhcpServerNetbiosNodeEnum",
    dhcp_server_netbios_node_type_txt,
    "This enumeration defines the type of NetBIOS node.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_dhcp_server_client_identifier_type_t,
    "DhcpServerClientIdentifierEnum",
    dhcp_server_client_identifier_type_txt,
    "This enumeration defines the type of client identifier.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_dhcp_server_binding_type_t,
    "DhcpServerBindingEnum",
    dhcp_server_binding_type_txt,
    "This enumeration defines the type of binding.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_dhcp_server_binding_state_t,
    "DhcpServerBindingStateEnum",
    dhcp_server_binding_state_txt,
    "This enumeration defines the state of binding.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_vlan_ifindex_index, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of VLAN.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_low_ip_index, mesa_ipv4_t, a, s ) {
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("LowIpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Low IP address.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_high_ip_index, mesa_ipv4_t, a, s ) {
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("HighIpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("High IP address.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_ip_index, mesa_ipv4_t, a, s ) {
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("IP address.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_reserved_ip_index, mesa_ipv4_t, a, s ) {
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Reserved IP address.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_pool_name_index, vtss_appl_dhcp_server_pool_name_t, a, s) {
    a.add_leaf(
        vtss::AsDisplayString(s.inner.pool_name, sizeof(s.inner.pool_name)),
        vtss::tag::Name("PoolName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Name of DHCP pool.")
    );
}

//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(dhcp_server_entry_no_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("EntryNo"),
        vtss::expose::snmp::RangeSpec<u32>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The number of entry. The number starts from 1.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(dhcp_ifindex, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number for the interface for which the ip address is reserved."));
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_config_globals_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_config_globals_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global mode of DHCP server. "
            "true is to enable the functions of DHCP server and "
            "false is to disable it.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_config_vlan_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_config_vlan_entry_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("VLAN mode of DHCP server. "
            "true is to enable DHCP server per VLAN and "
            "false is to disable it per VLAN.")
    );
}

#define _SERVER_LEAF(_f_, _name_, _desc_)    \
    m.add_leaf(                              \
        vtss::AsIpv4(s._f_[0]),              \
        vtss::tag::Name(_name_ "1"),        \
        vtss::expose::snmp::Status::Current,         \
        vtss::expose::snmp::OidElementValue(ix++),   \
        vtss::tag::Description(_desc_ " 1.") \
    );                                       \
    m.add_leaf(                              \
        vtss::AsIpv4(s._f_[1]),              \
        vtss::tag::Name(_name_ "2"),        \
        vtss::expose::snmp::Status::Current,         \
        vtss::expose::snmp::OidElementValue(ix++),   \
        vtss::tag::Description(_desc_ " 2.") \
    );                                       \
    m.add_leaf(                              \
        vtss::AsIpv4(s._f_[2]),              \
        vtss::tag::Name(_name_ "3"),        \
        vtss::expose::snmp::Status::Current,         \
        vtss::expose::snmp::OidElementValue(ix++),   \
        vtss::tag::Description(_desc_ " 3.") \
    );                                       \
    m.add_leaf(                              \
        vtss::AsIpv4(s._f_[3]),              \
        vtss::tag::Name(_name_ "4"),        \
        vtss::expose::snmp::Status::Current,         \
        vtss::expose::snmp::OidElementValue(ix++),   \
        vtss::tag::Description(_desc_ " 4.") \
    )

#define _VENDOR_CLASS_INFO_LEAF(_i_, _no_)                                                                              \
    m.add_leaf(                                                                                                         \
        vtss::AsDisplayString(s.vendor_class_info[_i_].class_id, sizeof(s.vendor_class_info[_i_].class_id)),                  \
        vtss::tag::Name("VendorClassId" _no_),                                                                         \
        vtss::expose::snmp::Status::Current,                                                                                    \
        vtss::expose::snmp::OidElementValue(ix++),                                                                              \
        vtss::tag::Description("Vendor Class Identifier. DHCP option 60. "                                              \
            "Specify to be used by DHCP client to optionally identify the "                                             \
            "vendor type and configuration of a DHCP client. DHCP server "                                              \
            "will deliver the corresponding option 43 specific information "                                            \
            "to the client that sends option 60 vendor class identifier.")                                              \
    );                                                                                                                  \
    m.add_leaf(                                                                                                         \
        vtss::AsDisplayString(s.vendor_class_info[_i_].specific_info, sizeof(s.vendor_class_info[_i_].specific_info)),  \
        vtss::tag::Name("VendorSpecificInfo" _no_),                                                                    \
        vtss::expose::snmp::Status::Current,                                                                                    \
        vtss::expose::snmp::OidElementValue(ix++),                                                                              \
        vtss::tag::Description("Vendor Specific Information. DHCP option 43. "                                          \
            "Specify vendor specific information corresponding to option 60 "                                           \
            "vendor class identifier. Therefore, the corresponding vendor "                                             \
            "class identifier must be defined before this specific information.")                                       \
    )

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_config_pool_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_config_pool_entry_t"));
    int ix = 0;

    m.add_leaf(
        s.type,
        vtss::tag::Name("PoolType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of pool. "
            "none(0) means the pool type is not defined yet. "
            "network(1) means the pool defines a pool of IP addresses to "
            "service more than one DHCP client. "
            "host(2) means the pool services for a specific DHCP client "
            "identified by client identifier or hardware address.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.ip),
        vtss::tag::Name("Ipv4Address"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Network number of the subnet. "
            "If the pool type is of network, the IP address can be any general IP address. "
            "If the pool type is of host, the IP address must be a unicast IP address.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.subnet_mask),
        vtss::tag::Name("SubnetMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Subnet Mask. DHCP option 1. "
            "Specify subnet mask of the DHCP address pool, "
            "excluding 0.0.0.0 and 255.255.255.255.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.subnet_broadcast),
        vtss::tag::Name("SubnetBroadcast"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Broadcast IP address in the subnet. DHCP option 28. "
            "Specify the broadcast address in use on the client's subnet.")
    );

    m.add_leaf(
        s.lease_day,
        vtss::tag::Name("LeaseDay"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of days of lease time. DHCP option 51, 58 and 59. "
            "The value range is 0-365. "
            "Specify lease time that allows the client to request a lease "
            "time for the IP address. If all of LeaseDay, LeaseHour and "
            "LeaseMinute are 0's, then it means the lease time is infinite.")
    );

    m.add_leaf(
        s.lease_hour,
        vtss::tag::Name("LeaseHour"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of hours of lease time. DHCP option 51, 58 and 59. "
            "The value range is 0-23. "
            "Specify lease time that allows the client to request a lease "
            "time for the IP address. If all of LeaseDay, LeaseHour and "
            "LeaseMinute are 0's, then it means the lease time is infinite.")
    );

    m.add_leaf(
        s.lease_minute,
        vtss::tag::Name("LeaseMinute"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of minutes of lease time. DHCP option 51, 58 and 59. "
            "The value range is 0-59. "
            "Specify lease time that allows the client to request a lease "
            "time for the IP address. If all of LeaseDay, LeaseHour and "
            "LeaseMinute are 0's, then it means the lease time is infinite.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.domain_name, sizeof(s.domain_name)),
        vtss::tag::Name("DomainName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Domain name. DHCP option 15. "
            "Specify domain name that client should use when resolving hostname via DNS.")
    );

    _SERVER_LEAF(default_router, "DefaultRouter", "Default router");

    _SERVER_LEAF(dns_server, "DnsServer", "DNS server");

    _SERVER_LEAF(ntp_server, "NtpServer", "NTP server");

    m.add_leaf(
        s.netbios_node_type,
        vtss::tag::Name("NetbiosNodeType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of NetBIOS node. DHCP option 46. "
            "Specify NetBIOS node type option to allow Netbios over TCP/IP "
            "clients which are configurable to be configured as described "
            "in RFC 1001/1002. "
            "nodeNone(0) means the node type is not defined yet. "
            "nodeB(1) means the node type is type of B. "
            "nodeP(2) means the node type is type of P. "
            "nodeM(3) means the node type is type of M. "
            "nodeH(4) means the node type is type of H.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.netbios_scope, sizeof(s.netbios_scope)),
        vtss::tag::Name("NetbiosScope"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("NetBIOS scope. DHCP option 47. "
            "Specify the NetBIOS over TCP/IP scope parameter for the client "
            "as specified in RFC 1001/1002.")
    );

    _SERVER_LEAF(netbios_name_server, "NetbiosNameServer", "NetBIOS name server");

    m.add_leaf(
        vtss::AsDisplayString(s.nis_domain_name, sizeof(s.nis_domain_name)),
        vtss::tag::Name("NisDomainName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("NIS Domain Name. DHCP option 40. "
            "Specify the name of the client's NIS domain.")
    );

    _SERVER_LEAF(nis_server, "NisServer", "NIS server");

    m.add_leaf(
        s.client_identifier_type,
        vtss::tag::Name("ClientIdentifierType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of client identifier. DHCP option 61. "
            "Specify client's unique identifier to be used when the pool is the type of host. "
            "none(0) means the client identifier type is not defined yet. "
            "name(1) means the client identifier type is other than hardware. "
            "mac(2) means the client identifier type is type of MAC address.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.client_identifier_name, sizeof(s.client_identifier_name)),
        vtss::tag::Name("ClientIdentifierName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client identifier which type is other than hardware. DHCP option 61. "
            "Specify client's unique identifier to be used when the pool is the type of host. "
            "This takes effect only if ClientIdentifierType is defined name(1).")
    );

    m.add_leaf(
        s.client_identifier_mac,
        vtss::tag::Name("ClientIdentifierMac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client's MAC address. DHCP option 61. "
            "Specify client's unique identifier to be used when the pool is the type of host. "
            "This takes effect only if ClientIdentifierType is defined as mac(2).")
    );

    m.add_leaf(
        s.client_haddr,
        vtss::tag::Name("ClientHardwareAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client's hardware address. "
            "Specify client's hardware(MAC) address to be used when the pool is the type of host.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.client_name, sizeof(s.client_name)),
        vtss::tag::Name("ClientName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client name. DHCP option 12. "
            "Specify the name of client to be used when the pool is the type of host.")
    );

    _VENDOR_CLASS_INFO_LEAF( 0, "1" );
    _VENDOR_CLASS_INFO_LEAF( 1, "2" );
    _VENDOR_CLASS_INFO_LEAF( 2, "3" );
    _VENDOR_CLASS_INFO_LEAF( 3, "4" );

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
    m.add_leaf(
        vtss::AsBool(s.reserved_only),
        vtss::tag::Name("ReservedOnly"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Whether to only hand out reserved addresses (TRUE) or not (FALSE).")
    );
#endif
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_status_declined_ip_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_status_declined_ip_entry_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsIpv4(s.ip),
        vtss::tag::Name("Ipv4Address"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("IPv4 address declined by DHCP client.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_status_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_status_statistics_t"));
    int ix = 0;

    m.add_leaf(
        s.discover_cnt,
        vtss::tag::Name("DiscoverCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP DISCOVER messages received.")
    );

    m.add_leaf(
        s.offer_cnt,
        vtss::tag::Name("OfferCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP OFFER messages sent.")
    );

    m.add_leaf(
        s.request_cnt,
        vtss::tag::Name("RequestCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP REQUEST messages received.")
    );

    m.add_leaf(
        s.ack_cnt,
        vtss::tag::Name("AckCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP ACK messages sent.")
    );

    m.add_leaf(
        s.nak_cnt,
        vtss::tag::Name("NakCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP NAK messages sent.")
    );

    m.add_leaf(
        s.decline_cnt,
        vtss::tag::Name("DeclineCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP DECLINE messages received.")
    );

    m.add_leaf(
        s.release_cnt,
        vtss::tag::Name("ReleaseCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP RELEASE messages received.")
    );

    m.add_leaf(
        s.inform_cnt,
        vtss::tag::Name("InformCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of DHCP INFORM messages received.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_status_binding_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_status_binding_entry_t"));
    int ix = 0;

    m.add_leaf(
        s.state,
        vtss::tag::Name("State"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("State of binding. "
            "none(0) means the binding is not in use. "
            "allocated(1) means the binding is allocated to the new DHCP client who send DHCPDISCOVER. "
            "committed(2) means the binding is committed as the DHCP process is completed successfully. "
            "expired(3) means the lease of the binding expired.")
    );

    m.add_leaf(
        s.type,
        vtss::tag::Name("Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of binding. "
            "none(0) means the binding is not in use. "
            "automatic(1) means the binding is mapped to network-type pool. "
            "manual(2) means the binding is mapped to host-type pool. "
            "expired(3) means the lease of the binding expired.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.pool_name, sizeof(s.pool_name)),
        vtss::tag::Name("PoolName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Name of the pool that creates the binding.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.server_id),
        vtss::tag::Name("ServerId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("IP address of the DHCP server to service the binding.")
    );

    m.add_leaf(
        s.vid,
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN where the binding works on.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.subnet_mask),
        vtss::tag::Name("SubnetMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Subnet mask of the DHCP client.")
    );

    m.add_leaf(
        s.client_identifier_type,
        vtss::tag::Name("ClientIdentifierType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of client identifier. DHCP option 61. "
            "Specify client's unique identifier to be used when the pool is the type of host. "
            "none(0) means the client identifier type is not defined yet. "
            "name(1) means the client identifier type is other than hardware. "
            "mac(2) means the client identifier type is type of MAC address.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.client_identifier_name, sizeof(s.client_identifier_name)),
        vtss::tag::Name("ClientIdentifierName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client identifier which type is other than hardware. DHCP option 61.")
    );

    m.add_leaf(
        s.client_identifier_mac,
        vtss::tag::Name("ClientIdentifierMac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Client's MAC address. DHCP option 61.")
    );

    m.add_leaf(
        s.chaddr,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("MAC address of the DHCP client.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.lease_str, sizeof(s.lease_str)),
        vtss::tag::Name("Lease"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Lease time of the binding.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.time_to_expire_str, sizeof(s.time_to_expire_str)),
        vtss::tag::Name("TimeToExpire"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("remaining time to expire.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_control_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_control_statistics_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.clear),
        vtss::tag::Name("Clear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Clear all statistics.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_server_control_binding_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_control_binding_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsIpv4(s.clear_by_ip),
        vtss::tag::Name("ClearByIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Clear binding with the IP address. "
            "If 0.0.0.0 then do nothing.")
    );

    m.add_leaf(
        s.clear_by_type,
        vtss::tag::Name("ClearByType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Clear binding by binding type. "
            "If none(0) then do nothing.")
    );
}

template<typename T>
void serialize(T &a, vtss_ifindex_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_server_control_binding_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsInterfaceIndex(s),
        vtss::tag::Name("interface"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Interface that the IP address is reserved for.")
    );
}

namespace vtss {
namespace appl {
namespace dhcp_server {
namespace interfaces {

struct DhcpServerConfigGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_server_config_globals_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_server_config_globals_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_config_globals_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_config_globals_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerConfigVlanEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_server_config_vlan_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is the table of DHCP server VLAN configuration. The index is VLAN ID.";

    static constexpr const char *index_description =
        "Each VLAN has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_server_vlan_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_server_config_vlan_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_config_vlan_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_server_config_vlan_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_config_vlan_entry_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerConfigExcludedEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_ipv4_t>,
        vtss::expose::ParamKey<mesa_ipv4_t>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is DHCP server excluded IP onfiguration table. The indexes are low IP and high IP address.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_server_low_ip_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, dhcp_server_high_ip_index(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_config_excluded_ip_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_server_config_excluded_ip_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_config_excluded_ip_entry_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_dhcp_server_config_excluded_ip_entry_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_dhcp_server_config_excluded_ip_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
struct DhcpServerConfigReservedEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_dhcp_server_pool_name_t *>,
        vtss::expose::ParamKey<mesa_ipv4_t *>,
        vtss::expose::ParamVal<vtss_ifindex_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is DHCP server reserved entries. The index is the name of the pool and the ip address";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_server_pool_name_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_server_pool_name_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, dhcp_server_reserved_ip_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, dhcp_ifindex(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_config_reserved_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_server_config_reserved_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_config_reserved_entry_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_dhcp_server_config_reserved_entry_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_dhcp_server_config_reserved_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};
#endif // VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES


struct DhcpServerConfigPoolEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_dhcp_server_pool_name_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_server_config_pool_entry_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is DHCP server pool onfiguration table. The indexe is pool name.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_server_pool_name_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_server_pool_name_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_server_config_pool_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_config_pool_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_server_config_pool_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_config_pool_entry_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_dhcp_server_config_pool_entry_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_dhcp_server_config_pool_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerStatusDeclinedEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_dhcp_server_status_declined_ip_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of IP addresses declined by DHCP client.";

    static constexpr const char *index_description =
        "Each entry has a declined IP address.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_server_entry_no_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_server_status_declined_ip_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_status_declined_ip_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_server_status_declined_ip_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerStatusStatisticsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_server_status_statistics_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_server_status_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_status_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerStatusBindingEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_ipv4_t>,
        vtss::expose::ParamVal<vtss_appl_dhcp_server_status_binding_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of binding data.";

    static constexpr const char *index_description =
        "Each entry has the binding data.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, dhcp_server_ip_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dhcp_server_status_binding_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_status_binding_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dhcp_server_status_binding_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerControlStatisticsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_server_control_statistics_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_server_control_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_control_statistics_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_control_statistics_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpServerControlBindingLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_server_control_binding_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_server_control_binding_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_server_control_binding_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_server_control_binding_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

}  // namespace interfaces
}  // namespace dhcp_server
}  // namespace appl
}  // namespace vtss
#endif /* __VTSS_DHCP_SERVER_SERIALIZER_HXX__ */
