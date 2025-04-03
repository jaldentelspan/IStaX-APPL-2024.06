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

#ifndef _IP_SERIALIZER_HXX_
#define _IP_SERIALIZER_HXX_

#include "vtss/appl/ip.h"
#include "ip_utils.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/module_id.h"
#include "vtss/basics/expose.hxx"
#include "ip_expose.hxx"
#include "ip_trace.h"

/**
 * \brief Get all IPv4 routing entries (for JSON only).
 * \param req [IN]  JSON request.
 * \param os  [OUT] JSON output string.
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_ipv4_status_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ip_dhcp4c_state_t,   "IpDhcpClientState", ip_expose_dhcp4c_state_txt,   "The state of the DHCP client");
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ip_dhcp4c_id_type_t, "IpDhcpClientType",  ip_expose_dhcp4c_id_type_txt, "The type of the DHCP client identifier.");
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ip_route_protocol_t, "IpRouteProtocol",   ip_expose_route_protocol_txt, "The protocol of the route.");

VTSS_SNMP_TAG_SERIALIZE(vtss_ip_findex_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Interface index number."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_ip_ipv6_key, mesa_ipv6_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("ipv6"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IPv6 address."));
}

VTSS_SNMP_TAG_SERIALIZE(IP_SER_if_dhcp4c_restart_action_t, mesa_bool_t, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("restart"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Restart the DHCP client."));
}

struct IpCapabilityHasPerInterfaceIpv4Statistics {
    static constexpr const char *json_ref = "vtss_appl_ip_capabilities_t";
    static constexpr const char *name = "HasPerInterfaceIpv4Statistics";
    static constexpr const char *desc = "If true, the platform supports detailed IPv4 statistics per interface.";
    static constexpr bool get()
    {
        return false;
    }
};

struct IpCapabilityHasPerInterfaceIpv6Statistics {
    static constexpr const char *json_ref = "vtss_appl_ip_capabilities_t";
    static constexpr const char *name = "HasPerInterfaceIpv6Statistics";
    static constexpr const char *desc = "If true, the platform supports detailed IPv6 statistics per interface.";
    static constexpr bool get()
    {
        return false;
    }
};

template <typename T>
void serialize(T &a, vtss_appl_ip_capabilities_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_capabilities_t"));

    m.add_leaf(vtss::AsBool(p.has_ipv4_host_capabilities),
               vtss::tag::Name("hasIpv4HostCapabilities"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description(
                   "The device has IPv4 host capabilities "
                   "for management."));

    m.add_leaf(vtss::AsBool(p.has_ipv6_host_capabilities),
               vtss::tag::Name("hasIpv6HostCapabilities"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description(
                   "The device has IPv6 host capabilities "
                   "for management."));

    m.add_leaf(vtss::AsBool(p.has_ipv4_unicast_routing_capabilities),
               vtss::tag::Name("hasIpv4UnicastRoutingCapabilities"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description(
                   "The device has IPv4 unicast routing "
                   "capabilities."));

    m.add_leaf(vtss::AsBool(p.has_ipv4_unicast_hw_routing_capabilities),
               vtss::tag::Name("hasIpv4UnicastHwRoutingCapabilities"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description(
                   "The device has IPv4 unicast hardware "
                   "accelerated routing capabilities."));

    m.add_leaf(vtss::AsBool(p.has_ipv6_unicast_routing_capabilities),
               vtss::tag::Name("hasIpv6UnicastRoutingCapabilities"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description(
                   "The device has IPv6 unicast routing "
                   "capabilities."));

    m.add_leaf(vtss::AsBool(p.has_ipv6_unicast_hw_routing_capabilities),
               vtss::tag::Name("hasIpv6UnicastHwRoutingCapabilities"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description(
                   "The device has IPv6 unicast hardware "
                   "accelerated routing capabilities."));

    m.add_leaf(p.interface_cnt_max,
               vtss::tag::Name("maxNumberOfIpInterfaces"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description(
                   "Maximum number of IP interfaces "
                   "supported by the device."));

    m.add_leaf(p.static_route_cnt_max,
               vtss::tag::Name("maxNumberOfStaticRoutes"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description(
                   "Maximum number of static configured "
                   "IP routes (shared by IPv4 and IPv6)."));

    m.add_leaf(p.lpm_hw_entry_cnt_max,
               vtss::tag::Name("numberOfLpmHardwareEntries"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description(
                   "Number of hardware LPM (longest "
                   "prefix match) entries."));

    m.template capability<IpCapabilityHasPerInterfaceIpv4Statistics>(vtss::expose::snmp::OidElementValue(10));
    m.template capability<IpCapabilityHasPerInterfaceIpv6Statistics>(vtss::expose::snmp::OidElementValue(11));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_global_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_global_conf_t"));

    m.add_leaf(vtss::AsBool(p.routing_enable), vtss::tag::Name("enableRouting"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Enable routing."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_conf_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_if_conf_t"));

    m.add_leaf(
        p.mtu, vtss::tag::Name("mtu"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("MTU for this IP interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_conf_ipv4_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_conf_ipv4_t"));

    m.add_leaf(
        vtss::AsBool(p.enable), vtss::tag::Name("active"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description(
            "Enable IPv4.\nIPv4 can only be enabled "
            "if either the DHCP client is enabled, or a valid address "
            "has been configured."));

    m.add_leaf(
        vtss::AsBool(p.dhcpc_enable), vtss::tag::Name("enableDhcpClient"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
        vtss::tag::Description(
            "Enable IPv4 DHCP client.\n"
            "Note: the DHCP client can only be enabled if there is no "
            "conflict in the values of: ipv4Address, prefixSize, and "
            "dhcpClientFallbackTimeout."));

    // this must be hidden from the user
    p.dhcpc_params.dhcpc_flags = VTSS_APPL_IP_DHCP4C_FLAG_NONE;

    m.add_leaf(vtss::AsIpv4(p.network.address), vtss::tag::Name("ipv4Address"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description(
                   "IPv4 address.\n"
                   "Note: Multiple interfaces may not have overlapping "
                   "networks."));

    m.add_leaf(p.network.prefix_size, vtss::tag::Name("prefixSize"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description(
                   "Prefix size of the network.\n"
                   "Note: Multiple interfaces may not have overlapping "
                   "networks."));

    m.add_leaf(
        p.fallback_timeout_secs, vtss::tag::Name("dhcpClientFallbackTimeout"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
        vtss::tag::Description(
            "DHCP client fallback timer.\n"
            "If DHCP is disabled then this object has no effect. "
            "If DHCP is enabled and the fallback timeout value is "
            "different from zero, then this timer will stop the DHCP "
            "process and assign the ipv4Address to the interface "
            "instead."));

    m.add_leaf(
        vtss::AsDisplayString(p.dhcpc_params.hostname, sizeof(p.dhcpc_params.hostname)),
        vtss::tag::Name("dhcpClientHostname"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
        vtss::tag::Description(
            "The hostname of DHCP client. If DHCPv4 client is enabled, "
            "the configured hostname will be used in the DHCP option 12 "
            "field. When this value is empty string, the field use the "
            "configured system name plus the latest three bytes of system "
            "MAC addresses as the hostname."));

    m.add_leaf(
        p.dhcpc_params.client_id.type,
        vtss::tag::Name("dhcpClientIdType"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
        vtss::tag::Description("The type of the DHCP client identifier."));

    m.add_leaf(
        vtss::AsInterfaceIndex(p.dhcpc_params.client_id.if_mac),
        vtss::tag::Name("dhcpClientIdIfMac"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
        vtss::tag::Description(
            "The interface name of DHCP client identifier. When DHCPv4 client is enabled "
            "and the client identifier type is 'ifmac', the configured interface's "
            "hardware MAC address will be used in the DHCP option 61 field."));

    m.add_leaf(
        vtss::AsDisplayString(p.dhcpc_params.client_id.ascii, sizeof(p.dhcpc_params.client_id.ascii)),
        vtss::tag::Name("dhcpClientIdAscii"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
        vtss::tag::Description(
            "The ASCII string of DHCP client identifier. When DHCPv4 client is enabled "
            "and the client identifier type is 'ascii', the ASCII string will be used "
            "in the DHCP option 61 field."));

    m.add_leaf(
        vtss::AsDisplayString(p.dhcpc_params.client_id.hex, sizeof(p.dhcpc_params.client_id.hex)),
        vtss::tag::Name("dhcpClientIdHex"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
        vtss::tag::Description(
            "The hexadecimal string of DHCP client identifier. When DHCPv4 client is "
            "enabled and the client identifier type 'hex', the hexadecimal value will "
            "be used in the DHCP option 61 field."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_conf_ipv6_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_conf_ipv6_t"));

    m.add_leaf(
        vtss::AsBool(p.enable), vtss::tag::Name("active"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description(
            "Enable the static configured IPv6 "
            "address.\n"
            "The static configured IPv6 address can only be configured "
            "if a valid address has been written into 'ipv6Address' "
            "and 'prefixSize'."));

    m.add_leaf(p.network.address, vtss::tag::Name("ipv6Address"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Static configured IPv6 address."));

    m.add_leaf(p.network.prefix_size, vtss::tag::Name("prefixSize"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Prefix size of the network."));
}

/******************************************************************************/
// ip_expose_route_key_ipv4_t
/******************************************************************************/
template<typename T> void serialize(T &a, ip_expose_route_key_ipv4_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_route_key_t"));

    m.add_leaf(vtss::AsIpv4(s.key.route.ipv4_uc.network.address),
               vtss::tag::Name("networkAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Network address."));

    m.add_leaf(vtss::AsInt(s.key.route.ipv4_uc.network.prefix_size),
               vtss::tag::Name("networkPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Network prefix size."));

    m.add_leaf(vtss::AsIpv4(s.key.route.ipv4_uc.destination),
               vtss::tag::Name("nextHop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Next-hop address."));
}

/******************************************************************************/
// ip_expose_route_key_ipv6_t
/******************************************************************************/
template<typename T> void serialize(T &a, ip_expose_route_key_ipv6_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_route_key_t"));

    m.add_leaf(s.key.route.ipv6_uc.network.address,
               vtss::tag::Name("networkAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Network address."));

    m.add_leaf(vtss::AsInt(s.key.route.ipv6_uc.network.prefix_size),
               vtss::tag::Name("networkPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Network prefix size."));

    m.add_leaf(s.key.route.ipv6_uc.destination,
               vtss::tag::Name("nextHop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Next-hop address."));

    m.add_leaf(vtss::AsInterfaceIndex(s.key.vlan_ifindex),
               vtss::tag::Name("nextHopInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description(
                   "If the next-hop address is a link-local address, "
                   " then the VLAN interface of the link-local address "
                   " must be specified here. Otherwise this value is "
                   " not used."));
}

/******************************************************************************/
// ip_expose_route_status_key_ipv4_t
/******************************************************************************/
template<typename T> void serialize(T &a, ip_expose_route_status_key_ipv4_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_route_status_key_t"));

    s.key.route.type = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;

    m.add_leaf(vtss::AsIpv4(s.key.route.route.ipv4_uc.network.address),
               vtss::tag::Name("networkAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Network address."));

    m.add_leaf(vtss::AsInt(s.key.route.route.ipv4_uc.network.prefix_size),
               vtss::tag::Name("networkPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Network prefix size."));

    m.add_leaf(s.key.protocol, vtss::tag::Name("Protocol"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The protocol that installed this route."));

    m.add_leaf(vtss::AsIpv4(s.key.route.route.ipv4_uc.destination),
               vtss::tag::Name("nextHop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Next-hop IP address. All-zeroes indicates the link is directly connected and all-ones indicates it is a blackhole route."));

    // Since IPv6 link-local addresses require the nextHopInterface to be
    // encoded in the key, we must also do that for IPv4, because it is not
    // serialized in the value, which is common between IPv4 and IPv6.
    m.add_leaf(vtss::AsInterfaceIndex(s.key.route.vlan_ifindex),
               vtss::tag::Name("nextHopInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Next-hop interface."));
}

/******************************************************************************/
// ip_expose_route_status_key_ipv6_t
/******************************************************************************/
template<typename T> void serialize(T &a, ip_expose_route_status_key_ipv6_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_route_status_key_t"));

    s.key.route.type = VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC;

    m.add_leaf(s.key.route.route.ipv6_uc.network.address,
               vtss::tag::Name("networkAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Network address."));

    m.add_leaf(vtss::AsInt(s.key.route.route.ipv6_uc.network.prefix_size),
               vtss::tag::Name("networkPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Network prefix size."));

    m.add_leaf(s.key.protocol, vtss::tag::Name("Protocol"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The protocol that installed this route."));

    m.add_leaf(s.key.route.route.ipv6_uc.destination,
               vtss::tag::Name("nextHop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Next-hop address. All-zeroes indicates the link is directly connected and all-ones indicates it is a blackhole route"));

    // We have to encode the nextHopInterface in the key, because otherwise we
    // might see non-increasing OIDs if multiple VLAN interfaces are up, because
    // they all have the same link-local address by default (fe80::)
    m.add_leaf(vtss::AsInterfaceIndex(s.key.route.vlan_ifindex),
               vtss::tag::Name("nextHopInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description(
                   "If the next-hop address is a link-local address, "
                   " then this is the VLAN interface of the link-local "
                   " address. Otherwise this value is not used"));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_route_status_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_route_status_t"));

#define FLAG2BOOL(X) (p.flags & VTSS_APPL_IP_ROUTE_STATUS_FLAG_##X) != 0

    m.add_leaf(FLAG2BOOL(SELECTED),
               vtss::tag::Name("Selected"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Route is the best."));

    m.add_leaf(FLAG2BOOL(ACTIVE),
               vtss::tag::Name("Active"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Destination is active."));

    m.add_leaf(FLAG2BOOL(FIB),
               vtss::tag::Name("Fib"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Route is installed in system's forward information base."));

    m.add_leaf(FLAG2BOOL(DIRECTLY_CONNECTED),
               vtss::tag::Name("DirectlyConnected"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Destination is directly connected."));

    m.add_leaf(FLAG2BOOL(ONLINK),
               vtss::tag::Name("Onlink"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Gateway is forced on interface."));

    m.add_leaf(FLAG2BOOL(DUPLICATE),
               vtss::tag::Name("Duplicate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Route is a duplicate."));

    m.add_leaf(FLAG2BOOL(RECURSIVE),
               vtss::tag::Name("Recursive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Route is recursive."));

    m.add_leaf(FLAG2BOOL(UNREACHABLE),
               vtss::tag::Name("Unreachable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("Nexthop is unreachable."));

    m.add_leaf(FLAG2BOOL(REJECT),
               vtss::tag::Name("Reject"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Route is rejected."));

    m.add_leaf(FLAG2BOOL(ADMIN_PROHIBITED),
               vtss::tag::Name("AdminProhib"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Route is prohibited by admin."));

    m.add_leaf(FLAG2BOOL(BLACKHOLE),
               vtss::tag::Name("Blackhole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Destination is a black hole."));

    // The next field in the status is p.nexthop_ifindex. We won't serialize
    // that, because it's already included in the key.

    m.add_leaf(p.metric, vtss::tag::Name("Metric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Metric of the route."));

    m.add_leaf(p.distance, vtss::tag::Name("Distance"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Distance of the route."));

    m.add_leaf(p.uptime, vtss::tag::Name("Uptime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("Time (in seconds) since this route was created"));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_status_link_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_status_link_t"));

    m.add_leaf(p.os_ifindex, vtss::tag::Name("osInterfaceIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description(
                   "Interface index used by the "
                   "operating system."));

    m.add_leaf(p.mtu, vtss::tag::Name("mtu"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("MTU for the interface."));

    m.add_leaf(p.mac, vtss::tag::Name("macAddress"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("MAC-address of the interface."));

    BOOL flag_up = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP;
    m.add_leaf(vtss::AsBool(flag_up), vtss::tag::Name("up"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Indicates if the interface is up."));

    BOOL flag_broadcast = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_BROADCAST;
    m.add_leaf(vtss::AsBool(flag_broadcast), vtss::tag::Name("broadcast"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description(
                   "Indicates if the interface is capable "
                   "of transmitting broadcast traffic."));

    BOOL flag_loopback = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_LOOPBACK;
    m.add_leaf(vtss::AsBool(flag_loopback), vtss::tag::Name("loopback"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description(
                   "Indicates if the interface is a "
                   "loop-back interface."));

    BOOL flag_running = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_RUNNING;
    m.add_leaf(vtss::AsBool(flag_running), vtss::tag::Name("running"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description(
                   "Interface is running (according to the "
                   "operating system)."));

    BOOL flag_noarp = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_NOARP;
    m.add_leaf(vtss::AsBool(flag_noarp), vtss::tag::Name("noarp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description(
                   "Indicates if the interface will answer "
                   "to ARP requests."));

    BOOL flag_promisc = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_PROMISC;
    m.add_leaf(vtss::AsBool(flag_promisc), vtss::tag::Name("promisc"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description(
                   "Indicates if the interface is in "
                   "promisc mode."));

    BOOL flag_multicast = p.flags & VTSS_APPL_IP_IF_LINK_FLAG_MULTICAST;
    m.add_leaf(vtss::AsBool(flag_multicast), vtss::tag::Name("multicast"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description(
                   "Indicates if the interface supports "
                   "multicast."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_info_ipv4_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_info_ipv4_t"));

    m.add_leaf(vtss::AsIpv4(p.broadcast), vtss::tag::Name("broadcast"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Broadcast address."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_info_ipv6_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_info_ipv6_t"));

#define FLAG(X) BOOL flag_##X = p.flags & VTSS_APPL_IP_IF_IPV6_FLAG_##X
    FLAG(TENTATIVE);
    m.add_leaf(vtss::AsBool(flag_TENTATIVE), vtss::tag::Name("tentative"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description(
                   "An address whose uniqueness on a "
                   "link is being verified, prior to its assignment to an "
                   "interface. A tentative address is not considered "
                   "assigned to an interface in the usual sense."));

    FLAG(DUPLICATED);
    m.add_leaf(
        vtss::AsBool(flag_DUPLICATED), vtss::tag::Name("duplicated"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
        vtss::tag::Description(
            "Indicates the address duplication is "
            "detected by Duplicate Address Detection (DAD).\n"
            "If the address is a link-local address formed from an "
            "interface identifier based on the hardware address, which "
            "is supposed to be uniquely assigned (e.g., EUI-64 for an "
            "Ethernet interface), IP operation on the interface should "
            "be disabled."));

    FLAG(DETACHED);
    m.add_leaf(vtss::AsBool(flag_DETACHED), vtss::tag::Name("detached"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description(
                   "Indicates this address is ready to be "
                   "detached from the link (IPv6 network)."));

    FLAG(NODAD);
    m.add_leaf(vtss::AsBool(flag_NODAD), vtss::tag::Name("nodad"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description(
                   "Indicates this address does not perform "
                   "Duplicate Address Detection (DAD)."));

    FLAG(AUTOCONF);
    m.add_leaf(
        vtss::AsBool(flag_AUTOCONF), vtss::tag::Name("autoconf"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
        vtss::tag::Description(
            "Indicates this address is capable of "
            "being retrieved by stateless address autoconfiguration."));

#if 0
    FLAG(ANYCAST);
    a.add_leaf(vtss::AsBool(flag_ANYCAST),
               vtss::tag::Name("anycast"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Indicates if the address is an any-cast "
                                      "address."));
#endif

#if 0
    FLAG(DEPRECATED);
    a.add_leaf(vtss::AsBool(flag_DEPRECATED),
               vtss::tag::Name("deprecated"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("An address assigned to an interface "
                                      "whose use is discouraged, but not forbidden. A "
                                      "deprecated address should no longer be used as a source "
                                      "address in new communications, but packets sent from or "
                                      "to deprecated addresses are delivered as expected.\n"
                                      "A deprecated address may continue to be used as a source "
                                      "address in communications where switching to a preferred "
                                      "address causes hardship to a specific upper-layer "
                                      "activity (e.g., an existing TCP connection)."));
#endif

#if 0
    FLAG(TEMPORARY);
    a.add_leaf(vtss::AsBool(flag_TEMPORARY),
               vtss::tag::Name("temporary"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Indicates this address is a temporary "
                                      "address. A temporary address is used to reduce the "
                                      "prospect of a user identity being permanently tied to an "
                                      "IPv6 address portion. An IPv6 node may create temporary "
                                      "addresses with interface identifiers based on "
                                      "time-varying random bit strings and relatively short "
                                      "lifetimes (hours to days). After that, they are replaced "
                                      "with new addresses."));
#endif

#if 0
    FLAG(HOME);
    a.add_leaf(vtss::AsBool(flag_HOME),
               vtss::tag::Name("home"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Indicates this address is a MIP6 "
                                      "(Mobility for IPv6) home address."));
#endif
#undef FLAG
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_status_dhcp4c_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_status_dhcp4c_t"));

    m.add_leaf(p.state, vtss::tag::Name("state"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("State of the DHCP client."));

    m.add_leaf(vtss::AsIpv4(p.server_ip), vtss::tag::Name("serverIp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description(
                   "IP address of the DHCP server that has "
                   "provided the DHCP offer."));
}

template<typename T>
void serialize(T &a, vtss_appl_ip_global_notification_status_t &global_notif_status)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_global_notification_status_t"));

    m.add_leaf(vtss::AsBool(global_notif_status.hw_routing_table_depleted),
               vtss::tag::Name("hwRoutingTableDepleted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("TRUE if the H/W routing table is full "
                                      "and yet another route is attempted "
                                      "added, causing the kernel's FIB "
                                      " database to become out of sync with "
                                      "H/W. This in turn means that proper "
                                      "routing can no longer be guaranteed.\n"
                                      "FALSE as long as everything is fine "
                                      "or the platform does not support H/W "
                                      "routing."));
}

// TODO, Please Review
template <typename T>
void serialize(T &a, vtss_appl_ip_if_statistics_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_if_statistics_t"));

    m.add_leaf(vtss::AsCounter(p.in_packets), vtss::tag::Name("inPackets"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description(
                   "Number of packets "
                   "delivered by MAC layer to IP layer."));

    m.add_leaf(vtss::AsCounter(p.out_packets), vtss::tag::Name("outPackets"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description(
                   "Number of packets "
                   "IP protocols requested be transmitted, including "
                   "those that were discarded or not sent."));

    m.add_leaf(vtss::AsCounter(p.in_bytes), vtss::tag::Name("inBytes"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description(
                   "Number of octets received "
                   "on the interface, including framing characters."));

    m.add_leaf(vtss::AsCounter(p.out_bytes), vtss::tag::Name("outBytes"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description(
                   "Number of octets transmitted "
                   "out of the interface, including framing characters."));

    m.add_leaf(vtss::AsCounter(p.in_multicasts),
               vtss::tag::Name("inMulticasts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description(
                   "Number of packets delivered by "
                   "MAC layer to IP layer that were addressed to a MAC "
                   "multicast address.\n"
                   "For a MAC layer protocol, this includes both Group and "
                   "Functional addresses."));

    m.add_leaf(vtss::AsCounter(p.out_multicasts),
               vtss::tag::Name("outMulticasts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description(
                   "Number of packets "
                   "addressed to a multicast MAC address that "
                   "IP protocols requested be transmitted, "
                   "including those that were discarded or not sent.\n"
                   "For a MAC layer protocol, this includes both Group and "
                   "Functional addresses."));

    m.add_leaf(vtss::AsCounter(p.in_broadcasts),
               vtss::tag::Name("inBroadcasts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description(
                   "Number of packets delivered by "
                   "MAC layer to IP layer that were addressed to a MAC "
                   "broadcast address."));

    m.add_leaf(vtss::AsCounter(p.out_broadcasts),
               vtss::tag::Name("outBroadcasts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description(
                   "Number of packets "
                   "addressed to a broadcast MAC address that "
                   "IP protocols requested be transmitted, including those "
                   "that were discarded or not sent."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_statistics_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ip_statistics_t"));

    m.add_leaf(
        p.InReceives, vtss::tag::Name("InReceives"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InReceivesValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
    vtss::tag::Description(
        "Number of input IP datagrams "
        "received, including those received in error.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInReceives), vtss::tag::Name("HCInReceives"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInReceivesValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
    vtss::tag::Description(
        "Number of input IP datagrams "
        "received, including those received in error. This object "
        "counts the same datagrams as InReceives, but allows for "
        "larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InOctets, vtss::tag::Name("InOctets"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InOctetsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
    vtss::tag::Description(
        "Number of octets received in "
        "input IP datagrams, including those received in error. "
        "Octets from datagrams counted in InReceives must be "
        "counted here.\n"
        "Discontinuities in the value of this counter may "
        "occur at re-initialization of the system, and at other "
        "times as indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInOctets), vtss::tag::Name("HCInOctets"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInOctetsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
    vtss::tag::Description(
        "Number of octets received in "
        "input IP datagrams, including those received in error. "
        "This object counts the same octets as InOctets, but "
        "allows "
        "for a larger value.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InHdrErrors, vtss::tag::Name("InHdrErrors"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InHdrErrorsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
    vtss::tag::Description(
        "Number of input IP datagrams "
        "discarded due to errors in their IP headers, including "
        "version number mismatch, other format errors, hop count "
        "exceeded, and errors discovered in processing their IP "
        "options.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InNoRoutes, vtss::tag::Name("InNoRoutes"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InNoRoutesValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
    vtss::tag::Description(
        "Number of input IP datagrams "
        "discarded because no route could be found to transmit "
        "them "
        "to their destination.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InAddrErrors, vtss::tag::Name("InAddrErrors"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InAddrErrorsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
    vtss::tag::Description(
        "Number of input IP datagrams "
        "discarded because the IP address in their IP header's "
        "destination field was not a valid address to be received "
        "at this entity. This count includes invalid addresses "
        "(e.g., ::0). For entities that are not IP routers and "
        "therefore do not forward datagrams, this counter includes "
        "datagrams discarded because the destination address was "
        "not a local address.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InUnknownProtos, vtss::tag::Name("InUnknownProtos"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InUnknownProtosValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
    vtss::tag::Description(
        "Number of locally-addressed IP "
        "datagrams received successfully but discarded because "
        "of an unknown or unsupported protocol.\n"
        "When tracking interface statistics, the counter of the "
        "interface to which these datagrams were addressed is "
        "incremented. This interface might not be the same as the "
        "input interface for some of the datagrams.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InTruncatedPkts, vtss::tag::Name("InTruncatedPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InTruncatedPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
    vtss::tag::Description(
        "Number of input IP datagrams "
        "discarded because the datagram frame didn't carry enough "
        "data.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InForwDatagrams, vtss::tag::Name("InForwDatagrams"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InForwDatagramsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
    vtss::tag::Description(
        "Number of input datagrams for which "
        "this entity was not their final IP destination and for "
        "which this entity attempted to find a route to forward "
        "them to their final destination.\n"
        "When tracking interface statistics, the counter of the "
        "incoming interface is incremented for each datagram.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInForwDatagrams),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInForwDatagramsValid;
    }),
    vtss::tag::Name("HCInForwDatagrams"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(11),
    vtss::tag::Description(
        "Number of input datagrams for which "
        "this entity was not their final IP destination and for "
        "which this entity attempted to find a route to forward "
        "them to their final destination. This object counts the "
        "same packets as InForwDatagrams, but allows for larger "
        "values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.ReasmReqds, vtss::tag::Name("ReasmReqds"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.ReasmReqdsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
    vtss::tag::Description(
        "Number of IP fragments received "
        "that needed to be reassembled at this interface.\n"
        "When tracking interface statistics, the counter of the "
        "interface to which these fragments were addressed is "
        "incremented. This interface might not be the same as the "
        "input interface for some of the fragments.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.ReasmOKs, vtss::tag::Name("ReasmOKs"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.ReasmOKsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
    vtss::tag::Description(
        "Number of IP datagrams successfully "
        "reassembled.\n"
        "When tracking interface statistics, the counter of the "
        "interface to which these datagrams were addressed is "
        "incremented. This interface might not be the same as the "
        "input interface for some of the datagrams.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.ReasmFails, vtss::tag::Name("ReasmFails"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.ReasmFailsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
    vtss::tag::Description(
        "Number of failures detected by the "
        "IP re-assembly algorithm.\n"
        "Note: this is not necessarily a count "
        "of discarded IP fragments because some algorithms "
        "(notably "
        "the algorithm in RFC 815) can lose track of the number of "
        "fragments by combining them as they are received.\n"
        "When tracking interface statistics, the counter of the "
        "interface to which these fragments were addressed is "
        "incremented. This interface might not be the same as the "
        "input interface for some of the fragments.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InDiscards, vtss::tag::Name("InDiscards"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InDiscardsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(15),
    vtss::tag::Description(
        "Number of input IP datagrams for "
        "which no problems were encountered to prevent their "
        "continued processing, but were discarded.\n"
        "Note: this counter does not include "
        "any datagrams discarded while awaiting re-assembly.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InDelivers, vtss::tag::Name("InDelivers"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InDeliversValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(16),
    vtss::tag::Description(
        "Number of datagrams "
        "successfully delivered to IP user-protocols (including "
        "ICMP).\n"
        "When tracking interface statistics, the counter of the "
        "interface to which these datagrams were addressed is "
        "incremented. This interface might not be the same as the "
        "input interface for some of the datagrams.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInDelivers), vtss::tag::Name("HCInDelivers"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInDeliversValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(17),
    vtss::tag::Description(
        "Number of datagrams "
        "successfully delivered to IP user-protocols (including "
        "ICMP). This object counts the same packets as "
        "ipSystemStatsInDelivers, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutRequests, vtss::tag::Name("OutRequests"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutRequestsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(18),
    vtss::tag::Description(
        "Number of IP datagrams that "
        "local IP user-protocols (including ICMP) supplied to IP "
        "in requests for transmission.\n"
        "Note: this counter does "
        "not include any datagrams counted in OutForwDatagrams.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutRequests), vtss::tag::Name("HCOutRequests"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutRequestsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(19),
    vtss::tag::Description(
        "Number of IP datagrams that "
        "local IP user-protocols (including ICMP) supplied to IP "
        "in requests for transmission. This object counts the same "
        "packets as OutRequests, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutNoRoutes, vtss::tag::Name("OutNoRoutes"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutNoRoutesValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(20),
    vtss::tag::Description(
        "Number of locally generated IP "
        "datagrams discarded because no route could be found to "
        "transmit them to their destination.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutForwDatagrams, vtss::tag::Name("OutForwDatagrams"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutForwDatagramsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(21),
    vtss::tag::Description(
        "Number of datagrams for which this "
        "entity was not their final IP destination and for which "
        "it was successful in finding a path to their final "
        "destination.\n"
        "When tracking interface statistics, the counter of the "
        "outgoing interface is incremented for a successfully "
        "forwarded datagram.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutForwDatagrams),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutForwDatagramsValid;
    }),
    vtss::tag::Name("HCOutForwDatagrams"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(22),
    vtss::tag::Description(
        "Number of datagrams for which this "
        "entity was not their final IP destination and for which "
        "it was successful in finding a path to their final "
        "destination. This object counts the same packets as "
        "OutForwDatagrams, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutDiscards, vtss::tag::Name("OutDiscards"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutDiscardsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(23),
    vtss::tag::Description(
        "Number of output IP datagrams for "
        "which no problem was encountered to prevent their "
        "transmission to their destination, but were discarded.\n"
        "Note: this counter "
        "includes datagrams counted in OutForwDatagrams if "
        "any "
        "such datagrams met this (discretionary) discard "
        "criterion.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutFragReqds, vtss::tag::Name("OutFragReqds"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutFragReqdsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(24),
    vtss::tag::Description(
        "Number of IP datagrams that "
        "require fragmentation in order to be transmitted.\n"
        "When tracking interface statistics, the counter of the "
        "outgoing interface is incremented for a successfully "
        "fragmented datagram.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutFragOKs, vtss::tag::Name("OutFragOKs"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutFragOKsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(25),
    vtss::tag::Description(
        "Number of IP datagrams that have "
        "been successfully fragmented.\n"
        "When tracking interface statistics, the counter of the "
        "outgoing interface is incremented for a successfully "
        "fragmented datagram.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutFragFails, vtss::tag::Name("OutFragFails"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutFragFailsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(26),
    vtss::tag::Description(
        "Number of IP datagrams that have "
        "been discarded because they needed to be fragmented but "
        "could not be. This includes IPv4 packets that have the "
        "DF bit set and IPv6 packets that are being forwarded and "
        "exceed the outgoing link MTU.\n"
        "When tracking interface statistics, the counter of the "
        "outgoing interface is incremented for an unsuccessfully "
        "fragmented datagram.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutFragCreates, vtss::tag::Name("OutFragCreates"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutFragCreatesValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(27),
    vtss::tag::Description(
        "Number of output datagram fragments "
        "that have been generated as a result of IP "
        "fragmentation.\n"
        "When tracking interface statistics, the counter of the "
        "outgoing interface is incremented for a successfully "
        "fragmented datagram.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutTransmits, vtss::tag::Name("OutTransmits"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutTransmitsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(28),
    vtss::tag::Description(
        "Number of IP datagrams that "
        "this entity supplied to the lower layers for "
        "transmission. "
        "This includes datagrams generated locally and those "
        "forwarded by this entity.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutTransmits),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutTransmitsValid;
    }),
    vtss::tag::Name("HCOutTransmits"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(29),
    vtss::tag::Description(
        "Number of IP datagrams that "
        "this entity supplied to the lower layers for "
        "transmission. "
        "This object counts the same datagrams as OutTransmits, "
        "but "
        "allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutOctets, vtss::tag::Name("OutOctets"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutOctetsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(30),
    vtss::tag::Description(
        "Number of octets in IP "
        "datagrams delivered to the lower layers for transmission. "
        "Octets from datagrams counted in OutTransmits must be "
        "counted here.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutOctets), vtss::tag::Name("HCOutOctets"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutOctetsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(31),
    vtss::tag::Description(
        "Number of octets in IP "
        "datagrams delivered to the lower layers for transmission. "
        "This objects counts the same octets as OutOctets, but "
        "allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InMcastPkts, vtss::tag::Name("InMcastPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InMcastPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(32),
    vtss::tag::Description(
        "Number of IP multicast datagrams "
        "received.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInMcastPkts), vtss::tag::Name("HCInMcastPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInMcastPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(33),
    vtss::tag::Description(
        "Number of IP multicast datagrams "
        "received. This object counts the same datagrams as "
        "InMcastPkts but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InMcastOctets, vtss::tag::Name("InMcastOctets"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InMcastOctetsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(34),
    vtss::tag::Description(
        "Number of octets received in "
        "IP multicast datagrams. Octets from datagrams counted in "
        "InMcastPkts MUST be counted here.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInMcastOctets),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInMcastOctetsValid;
    }),
    vtss::tag::Name("HCInMcastOctets"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(35),
    vtss::tag::Description(
        "Number of octets received in "
        "IP multicast datagrams. This object counts the same "
        "octets "
        "as InMcastOctets, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutMcastPkts, vtss::tag::Name("OutMcastPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutMcastPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(36),
    vtss::tag::Description(
        "Number of IP multicast datagrams "
        "transmitted.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutMcastPkts),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutMcastPktsValid;
    }),
    vtss::tag::Name("HCOutMcastPkts"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(37),
    vtss::tag::Description(
        "Number of IP multicast datagrams "
        "transmitted. This object counts the same datagrams as "
        "OutMcastPkts, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutMcastOctets, vtss::tag::Name("OutMcastOctets"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutMcastOctetsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(38),
    vtss::tag::Description(
        "Number of octets transmitted "
        "in IP multicast datagrams. Octets from datagrams counted "
        "in OutMcastPkts must be counted here.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutMcastOctets),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutMcastOctetsValid;
    }),
    vtss::tag::Name("HCOutMcastOctets"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(39),
    vtss::tag::Description(
        "Number of octets transmitted "
        "in IP multicast datagrams. This object counts the same "
        "octets as OutMcastOctets, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.InBcastPkts, vtss::tag::Name("InBcastPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.InBcastPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(40),
    vtss::tag::Description(
        "Number of IP broadcast datagrams "
        "received.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCInBcastPkts), vtss::tag::Name("HCInBcastPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCInBcastPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(41),
    vtss::tag::Description(
        "Number of IP broadcast datagrams "
        "received. This object counts the same datagrams as "
        "InBcastPkts but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        p.OutBcastPkts, vtss::tag::Name("OutBcastPkts"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.OutBcastPktsValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(42),
    vtss::tag::Description(
        "Number of IP broadcast datagrams "
        "transmitted.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    m.add_leaf(
        vtss::AsCounter(p.HCOutBcastPkts),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.HCOutBcastPktsValid;
    }),
    vtss::tag::Name("HCOutBcastPkts"), vtss::expose::snmp::Status::Current,
    vtss::expose::snmp::OidElementValue(43),
    vtss::tag::Description(
        "Number of IP broadcast datagrams "
        "transmitted. This object counts the same datagrams as "
        "OutBcastPkts, but allows for larger values.\n"
        "Discontinuities in the value of this counter may occur at "
        "re-initialization of the system, and at other times as "
        "indicated by the value of DiscontinuityTime."));

    u64 discontinuity_time = p.DiscontinuityTime.sec_msb;
    discontinuity_time <<= 32;
    discontinuity_time |= p.DiscontinuityTime.seconds;
    m.add_leaf(
        vtss::AsCounter(discontinuity_time),
        vtss::tag::Name("DiscontinuityTime"), vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(44),
        vtss::tag::Description(
            "Value of sysUpTime on the most "
            "recent occasion when any one or more of this entry's "
            "counters suffered a discontinuity.\n"
            "If no such discontinuities have occurred since the last "
            "re-initialization of the IP stack, then this object "
            "contains a zero value."));

    m.add_leaf(
        p.RefreshRate, vtss::tag::Name("RefreshRate"),
    vtss::expose::snmp::PreGetCondition([&]() {
        return p.RefreshRateValid;
    }),
    vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(45),
    vtss::tag::Description(
        "The minimum reasonable polling interval "
        "for this entry. This object provides an indication of the "
        "minimum amount of time required to update the counters in "
        "this entry."));
}

/**
 * A collection of global actions triggers.
 */
typedef struct {
    /**
     * Clear the IPv4 neighbor table.
     */
    mesa_bool_t ipv4_neighbor_table_clear;

    /**
     * Clear the IPv6 neighbor table.
     */
    mesa_bool_t ipv6_neighbor_table_clear;

    /**
     * Clear the IPv4 system statistics.
     */
    mesa_bool_t ipv4_system_statistics_clear;

    /**
     * Clear the IPv6 system statistics.
     */
    mesa_bool_t ipv6_system_statistics_clear;

    /**
     * Clear the IPv4 ACD status table.
     */
    mesa_bool_t ipv4_acd_status_clear;
} IP_SER_global_actions_t;

template <typename T>
void serialize(T &a, IP_SER_global_actions_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("IP_SER_global_actions_t"));

    m.add_leaf(vtss::AsBool(p.ipv4_neighbor_table_clear),
               vtss::tag::Name("ipv4NeighborTableClear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description(
                   "Flush the entries in IPv4 ARP cache "
                   "except for the permanent ones."));

    m.add_leaf(
        vtss::AsBool(p.ipv6_neighbor_table_clear),
        vtss::tag::Name("ipv6NeighborTableClear"),
        vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
        vtss::tag::Description(
            "Erase all the NDP (Neighbor Discovery "
            "Protocol) entries registered in IPv6 neighbor cache."));

    m.add_leaf(p.ipv4_system_statistics_clear,
               vtss::tag::Name("ipv4SystemStatisticsClear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description(
                   "Clear the system-wide IPv4 traffic "
                   "statistics."));

    m.add_leaf(p.ipv6_system_statistics_clear,
               vtss::tag::Name("ipv6SystemStatisticsClear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description(
                   "Clear the system-wide IPv6 traffic "
                   "statistics."));

    m.add_leaf(p.ipv4_acd_status_clear,
               vtss::tag::Name("ipv4AcdStatusClear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description(
                   "Clear the Address Conflict Detection table."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_key_ipv4_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_if_key_ipv4_t"));

    m.add_leaf(p.ifindex,
               vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Interface index number."));

    m.add_rpc_leaf(p.addr,
                   vtss::tag::Name("networkAddress"),
                   vtss::tag::Description("IPv4 network address."));

    m.add_snmp_leaf(vtss::AsIpv4(p.addr.address),
                    vtss::tag::Name("networkAddress"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(2),
                    vtss::tag::Description("IPv4 network address."));

    m.add_snmp_leaf(vtss::AsInt(p.addr.prefix_size),
                    vtss::tag::Name("networkMaskLength"),
                    vtss::expose::snmp::RangeSpec<u32>(0, 32),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(3),
                    vtss::tag::Description("IPv4 network mask length."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_if_key_ipv6_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_if_key_ipv6_t"));

    m.add_leaf(p.ifindex,
               vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Interface index number."));

    m.add_rpc_leaf(p.addr,
                   vtss::tag::Name("networkAddress"),
                   vtss::tag::Description("IPv6 network address."));

    m.add_snmp_leaf(p.addr.address,
                    vtss::tag::Name("networkAddress"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(2),
                    vtss::tag::Description("IPv6 network address."));

    m.add_snmp_leaf(vtss::AsInt(p.addr.prefix_size),
                    vtss::tag::Name("networkMaskLength"),
                    vtss::expose::snmp::RangeSpec<u32>(0, 128),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(3),
                    vtss::tag::Description("IPv6 network mask length."));
}

template <typename T>
void serialize(T &a, ip_expose_neighbor_key_ipv4_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_neighbor_key_t"));

    m.add_leaf(vtss::AsIpv4(p.key.dip.addr.ipv4),
               vtss::tag::Name("ipv4"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IPv4 address."));
}

template <typename T>
void serialize(T &a, ip_expose_neighbor_key_ipv6_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_neighbor_key_t"));

    m.add_leaf(p.key.dip.addr.ipv6, vtss::tag::Name("ipv6"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IPv6 address."));

    m.add_leaf(p.key.ifindex,
               vtss::tag::Name("ifindex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description(
                   "If 'ipAddress' is a link-local address, "
                   "then the interface index where the host can be reached "
                   "must be specified here, otherwise set this to "
                   "zero."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_neighbor_status_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_neighbor_status_t"));

    m.add_leaf(p.dmac,
               vtss::tag::Name("macAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("MAC address associated with the IP address"));

    m.add_leaf(p.ifindex,
               vtss::tag::Name("interface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Interface the neighbor can be reached on."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_route_conf_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ip_route_conf_t"));

    m.add_leaf(p.distance, vtss::tag::Name("distance"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("distance value for this route "));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_acd_status_ipv4_key_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ip_acd_status_ipv4_key_t"));

    m.add_leaf(vtss::AsIpv4(p.sip), vtss::tag::Name("ipAddress"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Sender IP address."));
    m.add_leaf(p.smac, vtss::tag::Name("macAddress"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Sender MAC address."));
}

template <typename T>
void serialize(T &a, vtss_appl_ip_acd_status_ipv4_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ip_acd_status_ipv4_t"));

    m.add_leaf(vtss::AsInt(vtss_ifindex_cast_to_u32(p.ifindex)),
               vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IP interface index."));
    m.add_leaf(vtss::AsInt(vtss_ifindex_cast_to_u32(p.ifindex_port)),
               vtss::tag::Name("ifIndexPort"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Port interface index."));
}

#if defined(VTSS_SW_OPTION_JSON_RPC)
void vtss_appl_ip_json_init();
#endif

namespace vtss
{
namespace appl
{
namespace ip
{
namespace interfaces
{

struct GlobalCapabilities {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ip_capabilities_t *>>
                                                                            P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_capabilities_t &i)
    {
        h.argument_properties(tag::Name("capabilities"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_capabilities_get);
};

struct GlobalsParam {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ip_global_conf_t *>>
                                                                           P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_global_conf_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_global_conf_set);
};

struct IfTable {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_ip_if_conf_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is the IP interface table. When an IP interface is created "
        "it can be configured in the other tables found in this MIB.";

    static constexpr const char *index_description =
        "Entries in this table represent IP interfaces created on the "
        "system";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_conf_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_if_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_if_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ip_if_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ip_if_conf_del);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

struct Ipv4Table {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_ip_if_conf_ipv4_t *>> P;

    static constexpr const char *table_description =
        "IPv4 interface configuration table. This table enables IPv4 "
        "related configuration of the corresponding IP interface.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_conf_ipv4_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_if_conf_ipv4_get);
    VTSS_EXPOSE_SET_PTR(ip_expose_if_conf_ipv4_set);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

struct Ipv6Table {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_ip_if_conf_ipv6_t *>> P;

    static constexpr const char *table_description =
        "IPv6 interface configuration table. This table enables IPv6 "
        "related configuration of the corresponding IP interface.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_conf_ipv6_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_if_conf_ipv6_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_if_conf_ipv6_set);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

struct RouteIpv4 {
    typedef expose::ParamList<expose::ParamKey<ip_expose_route_key_ipv4_t *>, expose::ParamVal<vtss_appl_ip_route_conf_t *>> P;
    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description = "This is the IPv4 route configuration table.";
    static constexpr const char *index_description = "Each entry in this table represents a configured route.\nNote: A route may be configured without being active.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(ip_expose_route_key_ipv4_t &i)
    {
        h.argument_properties(tag::Name("route_key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_route_conf_t &i)
    {
        h.argument_properties(tag::Name("route_conf"));
        h.argument_properties(expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_route_ipv4_conf_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_route_ipv4_itr);
    VTSS_EXPOSE_SET_PTR(ip_expose_route_ipv4_conf_set);
    VTSS_EXPOSE_ADD_PTR(ip_expose_route_ipv4_conf_set);
    VTSS_EXPOSE_DEL_PTR(ip_expose_route_ipv4_conf_del);
};

struct RouteIpv6 {
    typedef expose::ParamList<expose::ParamKey<ip_expose_route_key_ipv6_t *>, expose::ParamVal<vtss_appl_ip_route_conf_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description = "This is the IPv6 route configuration table.";
    static constexpr const char *index_description = "Each entry in this table represents a configured route.\nNote: a route may be configured without being active.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(ip_expose_route_key_ipv6_t &i)
    {
        h.argument_properties(tag::Name("route_key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_route_conf_t &i)
    {
        h.argument_properties(tag::Name("route_conf"));
        h.argument_properties(expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_route_ipv6_conf_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_route_ipv6_itr);
    VTSS_EXPOSE_SET_PTR(ip_expose_route_ipv6_conf_set);
    VTSS_EXPOSE_ADD_PTR(ip_expose_route_ipv6_conf_set);
    VTSS_EXPOSE_DEL_PTR(ip_expose_route_ipv6_conf_del);
};

struct StatusGlobalsIpv4Nb {
    typedef expose::ParamList<expose::ParamKey<ip_expose_neighbor_key_ipv4_t *>, expose::ParamVal<vtss_appl_ip_neighbor_status_t *>> P;

    static constexpr const char *table_description = "This is the IPv4 neighbor (ARP) table.";
    static constexpr const char *index_description = "Each entry in this table represents an entry in the underlying operating system's ARP table.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(ip_expose_neighbor_key_ipv4_t &i)
    {
        h.argument_properties(tag::Name("nb_key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_neighbor_status_t &i)
    {
        h.argument_properties(tag::Name("nb_val"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_neighbor_status_ipv4_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_neighbor_status_ipv4_itr);
};

struct StatusGlobalIpv6Nb {
    typedef expose::ParamList<expose::ParamKey<ip_expose_neighbor_key_ipv6_t *>, expose::ParamVal<vtss_appl_ip_neighbor_status_t *>> P;

    static constexpr const char *table_description = "This is the IPv6 neighbor table.";
    static constexpr const char *index_description = "Each entry in this table represents an entry in the underlying operating system's neighbor table.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(ip_expose_neighbor_key_ipv6_t &i)
    {
        h.argument_properties(tag::Name("nb_key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_neighbor_status_t &i)
    {
        h.argument_properties(tag::Name("nb_val"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_neighbor_status_ipv6_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_neighbor_status_ipv6_itr);
};

struct StatusIfLink_ {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_ip_if_status_link_t *>>
            P;

    static constexpr const char *table_description =
        "This table provides link-layer status information for IP "
        "interfaces.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_status_link_t &i)
    {
        h.argument_properties(tag::Name("status"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
};

struct StatusIfIpv4_ {
    typedef expose::ParamList<expose::ParamKey<vtss_appl_ip_if_key_ipv4_t *>, expose::ParamVal<vtss_appl_ip_if_info_ipv4_t *>> P;

    static constexpr const char *table_description =
        "This table provides IPv4 status information for IP interfaces. If "
        "an interface is configured to use a DHCP client, then the address "
        "can be found here.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_if_key_ipv4_t &i)
    {
        h.argument_properties(tag::Name("ipv4_key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_info_ipv4_t &i)
    {
        h.argument_properties(tag::Name("status"));
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_if_status_ipv4_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ip_if_status_ipv4_itr);
};

struct StatusIfDhcpv4 {
    typedef expose::ParamList <
    expose::ParamKey<vtss_ifindex_t>,
           expose::ParamVal<vtss_appl_ip_if_status_dhcp4c_t * >> P;

    static constexpr const char *table_description =
        "This table provides status on the DHCP client "
        "running on a given interface.";

    static constexpr const char *index_description =
        "Each entry in this table represents an instance of a DHCP client.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_status_dhcp4c_t &i)
    {
        h.argument_properties(tag::Name("status"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_if_status_dhcp4c_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

struct StatusIfIpv6_ {
    typedef expose::ParamList<expose::ParamKey<vtss_appl_ip_if_key_ipv6_t *>, expose::ParamVal<vtss_appl_ip_if_info_ipv6_t *>> P;

    static constexpr const char *table_description =
        "This table provides IPv6 status information for IP interfaces. If "
        "an interface is configured to use a DHCP "
        "client, then the address can be found here.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_if_key_ipv6_t &i)
    {
        h.argument_properties(tag::Name("ipv6_key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_info_ipv6_t &i)
    {
        h.argument_properties(tag::Name("status"));
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_if_status_ipv6_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ip_if_status_ipv6_itr);
};

struct StatusGlobalNotification {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ip_global_notification_status_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_global_notification_status_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_global_notification_status_get);
};

struct StatGlobal4 {
    typedef expose::ParamList <
    expose::ParamVal<vtss_appl_ip_statistics_t * >> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_statistics_t &i)
    {
        h.argument_properties(tag::Name("stats"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_system_statistics_ipv4_get);
};

struct StatGlobal6 {
    typedef expose::ParamList <
    expose::ParamVal<vtss_appl_ip_statistics_t * >> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_statistics_t &i)
    {
        h.argument_properties(tag::Name("stats"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_system_statistics_ipv6_get);
};

struct StatIfLink {
    typedef expose::ParamList <
    expose::ParamKey<vtss_ifindex_t>,
           expose::ParamVal<vtss_appl_ip_if_statistics_t * >> P;

    static constexpr const char *table_description =
        "This table provides interface link statistics for a given IP "
        "interface.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_if_statistics_t &i)
    {
        h.argument_properties(tag::Name("stats"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_if_statistics_link_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

struct StatIfIpv4 {
    typedef expose::ParamList <
    expose::ParamKey<vtss_ifindex_t>,
           expose::ParamVal<vtss_appl_ip_statistics_t * >> P;

    typedef IpCapabilityHasPerInterfaceIpv4Statistics depends_on_t;

    static constexpr const char *table_description =
        "This table provides IPv4 related statitics for a given IP "
        "interface.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_statistics_t &i)
    {
        h.argument_properties(tag::Name("stats"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_if_statistics_ipv4_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

struct StatIfIpv6 {
    typedef expose::ParamList <
    expose::ParamKey<vtss_ifindex_t>,
           expose::ParamVal<vtss_appl_ip_statistics_t * >> P;

    typedef IpCapabilityHasPerInterfaceIpv6Statistics depends_on_t;

    static constexpr const char *table_description =
        "This table provides IPv6 related statitics for a given IP "
        "interface.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IP interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_statistics_t &i)
    {
        h.argument_properties(tag::Name("stat"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_if_statistics_ipv6_get);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

//------------------------------------------------------------------------------
//** IPv4 route status
//------------------------------------------------------------------------------
struct StatusRtIpv4_ {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<ip_expose_route_status_key_ipv4_t *>, vtss::expose::ParamVal<vtss_appl_ip_route_status_t *>> P;

    static constexpr const char *table_description = "This table provides IPv4 routing status.";
    static constexpr const char *index_description = "Each entry in this table represents an IPv4 route.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(ip_expose_route_status_key_ipv4_t &i)
    {
        h.argument_properties(tag::Name("RouteKey"));
        serialize(h, i);
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_route_status_t &i)
    {
        h.argument_properties(tag::Name("RouteStatus"));
        h.argument_properties(vtss::expose::snmp::OidOffset(6));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_ITR_PTR(ip_expose_route_status_ipv4_itr);
    VTSS_EXPOSE_GET_PTR(ip_expose_route_status_ipv4_get);
};

//------------------------------------------------------------------------------
//** IPv6 route status
//------------------------------------------------------------------------------
struct StatusRtIpv6_ {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<ip_expose_route_status_key_ipv6_t *>, vtss::expose::ParamVal<vtss_appl_ip_route_status_t *>> P;

    static constexpr const char *table_description = "This table provides IPv6 routing status.";
    static constexpr const char *index_description = "Each entry in this table represents an IPv6 route.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(ip_expose_route_status_key_ipv6_t &i)
    {
        h.argument_properties(tag::Name("RouteKey"));
        serialize(h, i);
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_route_status_t &i)
    {
        h.argument_properties(tag::Name("RouteStatus"));
        h.argument_properties(vtss::expose::snmp::OidOffset(6));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_ITR_PTR(ip_expose_route_status_ipv6_itr);
    VTSS_EXPOSE_GET_PTR(ip_expose_route_status_ipv6_get);
};

struct StatusAcdIpv4_ {
    typedef expose::ParamList <
    expose::ParamKey<vtss_appl_ip_acd_status_ipv4_key_t *>,
           expose::ParamVal<vtss_appl_ip_acd_status_ipv4_t * >> P;

    static constexpr const char *table_description =
        "This is the IPv4 Address Conflict Detection table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a conflicting node.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_acd_status_ipv4_key_t &i)
    {
        h.argument_properties(tag::Name("key"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_acd_status_ipv4_t &i)
    {
        h.argument_properties(tag::Name("val"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_acd_status_ipv4_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ip_acd_status_ipv4_itr);
};

inline mesa_rc IP_SER_global_control_get(IP_SER_global_actions_t *const a)
{
    memset(a, 0, sizeof(*a));
    return VTSS_RC_OK;
}

inline mesa_rc IP_SER_global_control_set(const IP_SER_global_actions_t *a)
{
    if (a->ipv4_neighbor_table_clear) {
        VTSS_RC(vtss_appl_ip_neighbor_clear(MESA_IP_TYPE_IPV4));
    }

    if (a->ipv6_neighbor_table_clear) {
        VTSS_RC(vtss_appl_ip_neighbor_clear(MESA_IP_TYPE_IPV6));
    }

    if (a->ipv4_system_statistics_clear) {
        VTSS_RC(vtss_appl_ip_system_statistics_ipv4_clear());
    }

    if (a->ipv6_system_statistics_clear) {
        VTSS_RC(vtss_appl_ip_system_statistics_ipv6_clear());
    }

    if (a->ipv4_acd_status_clear) {
        VTSS_RC(vtss_appl_ip_acd_status_ipv4_clear());
    }

    return VTSS_RC_OK;
}

struct ControlGlobals {
    typedef expose::ParamList<expose::ParamVal<IP_SER_global_actions_t *>>
                                                                        P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(IP_SER_global_actions_t &i)
    {
        h.argument_properties(tag::Name("a"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(IP_SER_global_control_get);
    VTSS_EXPOSE_SET_PTR(IP_SER_global_control_set);
};

struct CtrlDhcpc {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<mesa_bool_t>> P;

    static constexpr const char *table_description =
        "This table provides control facilities to control an instance of "
        "the DHCP client.";

    static constexpr const char *index_description =
        "Each entry in this table represents an instance of the DHCP "
        "client.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifidx"));
        serialize(h, vtss_ip_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_bool_t &i)
    {
        h.argument_properties(tag::Name("action"));
        serialize(h, IP_SER_if_dhcp4c_restart_action_t(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
    VTSS_EXPOSE_GET_PTR(ip_expose_dhcp4c_control_restart_get);
    VTSS_EXPOSE_SET_PTR(ip_expose_dhcp4c_control_restart);
    VTSS_EXPOSE_ITR_PTR(ip_expose_if_itr);
};

}  // namespace interfaces
}  // namespace ip
}  // namespace appl
}  // namespace vtss

#endif  // _IP_SERIALIZER_HXX_

