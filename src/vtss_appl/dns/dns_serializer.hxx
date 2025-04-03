/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_DNS_SERIALIZER_HXX__
#define __VTSS_DNS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dns.h"
#include "vtss/appl/types.hxx"

/*****************************************************************************
    Data type serializer
*****************************************************************************/

/*****************************************************************************
    Enumerator serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t dns_configType_txt[];

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_dns_config_type_t,
                         "DnsConfigType", 
                         dns_configType_txt,
                         "This enumeration indicates the configured DNS server type or default domain name type.");

/*****************************************************************************
    Index serializer
*****************************************************************************/

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_dns_capabilities_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_dns_capabilities_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.support_dhcp4_config_server),
        vtss::tag::Name("SupportDhcp4ConfigServer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support setting DNS server from DHCPv4.")
    );

    m.add_leaf(
        vtss::AsBool(s.support_dhcp6_config_server),
        vtss::tag::Name("SupportDhcp6ConfigServer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support setting DNS server from DHCPv6.")
    );

    m.add_leaf(
        vtss::AsBool(s.support_default_domain_name),
        vtss::tag::Name("SupportDefaultDomainName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support setting default domain name.")
    );

    m.add_leaf(
        vtss::AsBool(s.support_dhcp4_domain_name),
        vtss::tag::Name("SupportDhcp4DomainName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support setting default domain name from DHCPv4.")
    );

    m.add_leaf(
        vtss::AsBool(s.support_dhcp6_domain_name),
        vtss::tag::Name("SupportDhcp6DomainName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support setting default domain name from DHCPv6.")
    );

    m.add_leaf(
        s.ns_cnt_max,
        vtss::tag::Name("NsCntMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum number of supported name servers.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dns_proxy_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dns_proxy_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.proxy_admin_state),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the DNS-Proxy feature.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dns_name_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dns_name_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.domainname_type,
        vtss::tag::Name("Setting"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Default domain name administrative type. "
            "A default domain name is used as the suffix of the given name in DNS lookup. "
            "none(0) means no default domain name is used and thus no domain name suffix "
            "is appended in DNS lookup. static(1) means the default domain name will be "
            "manually set. dhcpv4(2) means default domain name will be determined by DHCPv4 "
            "discovery. dhcpv4Vlan(3) means default domain name will be determined by DHCPv4 "
            "discovery on a specific IP VLAN interface. dhcpv6(4) means default domain name "
            "will be determined by DHCPv6 discovery. dhcpv6Vlan(5) means default domain name "
            "will be determined by DHCPv6 discovery on a specific IP VLAN interface.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.static_domain_name, sizeof(s.static_domain_name)),
        vtss::tag::Name("StaticName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The static default domain name. It will be a reference only "
            "when DomainNameSetting is static(1).")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.dhcp_ifindex),
        vtss::tag::Name("DhcpIfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ifIndex of specific VLAN interface that default domain name "
            "will be retrieved from DHCP. It will be a reference only when DomainNameSetting is "
            "either dhcpv4Vlan(3) or dhcpv6Vlan(5).")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dns_server_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dns_server_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.server_type,
        vtss::tag::Name("Setting"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Per precedence DNS server administrative type. The DNS server "
            "setting will be used in DNS lookup. "
            "none(0) denotes no DNS server is used and thus domain name lookup always fails. "
            "static(1) denotes the DNS server address will be manually set, in either IPv4 or "
            "IPv6 address form. dhcpv4(2) denotes DNS server address will be determined by DHCPv4 "
            "discovery. dhcpv4Vlan(3) denotes DNS server address will be determined by DHCPv4 "
            "discovery on a specifc IP VLAN interface. dhcpv6(4) denotes DNS server address will "
            "be determined by DHCPv6 discovery. dhcpv6Vlan(5) denotes DNS server address will be "
            "determined by DHCPv6 discovery on a specifc IP VLAN interface.")
    );

    m.add_leaf(
        s.static_ip_address,
        vtss::tag::Name("StaticIpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The static DNS server address. It will be a reference only when "
            "Setting is static(1)")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.static_ifindex),
        vtss::tag::Name("StaticIfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ifIndex of specific VLAN interface that DNS server address will "
            "be retrieved from DHCP and where the server resides. It will be a reference only when "
            "Setting is either dhcpv4Vlan(3) or dhcpv6Vlan(5).")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dns_domainname_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dns_domainname_status_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.default_domain_name, sizeof(s.default_domain_name)),
        vtss::tag::Name("Suffix"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The suffix of the given domain name used in DNS lookup.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dns_server_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dns_server_status_t"));
    int ix = 0;

    m.add_leaf(
        s.configured_type,
        vtss::tag::Name("ConfiguredType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Per precedence DNS server configured type."
            "none(0) denotes no DNS server is used and thus domain name lookup always fails. "
            "static(1) denotes the DNS server address will be manually set, in either IPv4 or "
            "IPv6 address form. dhcpv4(2) denotes DNS server address will be determined by DHCPv4 "
            "discovery. dhcpv4Vlan(3) denotes DNS server address will be determined by DHCPv4 "
            "discovery on a specifc IP VLAN interface. dhcpv6(4) denotes DNS server address will "
            "be determined by DHCPv6 discovery. dhcpv6Vlan(5) denotes DNS server address will be "
            "determined by DHCPv6 discovery on a specifc IP VLAN interface.")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.reference_ifindex),
        vtss::tag::Name("ReferenceIfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ifIndex of specific VLAN interface that DNS server address will "
            "be retrieved from DHCP and where the server resides. It will be a reference only when "
            "Setting is either dhcpv4Vlan(3) or dhcpv6Vlan(5).")
    );

    m.add_leaf(
        s.ip_address,
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The DNS server address that will be used for domain name lookup.")
    );
}

namespace vtss
{
namespace appl
{
namespace dns
{
namespace interfaces
{

struct DnsCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_dns_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dns_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("capability"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dns_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
};

struct DnsConfigProxyLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dns_proxy_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dns_proxy_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("proxy_config"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dns_proxy_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dns_proxy_config_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_dns_proxy_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
};

struct DnsConfigDomainNameLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dns_name_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dns_name_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("domainname_config"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dns_domain_name_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dns_domain_name_config_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_dns_domain_name_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
};

struct DnsConfigServersTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32 *>,
         vtss::expose::ParamVal<vtss_appl_dns_server_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing DNS server configuration.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        h.add_leaf(
            i,
            vtss::tag::Name("Precedence"),
            vtss::expose::snmp::RangeSpec<u32>(0, 3),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description(
                "Table index also represents the precedence in selecting target DNS server: "
                "less index value means higher priority in round-robin selection. Only one "
                "server is working at a time, that is when the chosen server is active, system "
                "marks the designated server as target and stops selection. When the active "
                "server becomes inactive, system starts another round of selection starting from "
                "the next available server setting.")
        );
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dns_server_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2), vtss::tag::Name("server_config"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dns_server_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dns_server_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dns_server_config_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_dns_server_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP);
};

struct DnsStatusDomainNameLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dns_domainname_status_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dns_domainname_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("default_domain_name"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dns_domainname_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
};

struct DnsStatusServersTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32 *>,
         vtss::expose::ParamVal<vtss_appl_dns_server_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for displaying DNS server information.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        h.add_leaf(
            i,
            vtss::tag::Name("Precedence"),
            vtss::expose::snmp::RangeSpec<u32>(0, 3),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description(
                "Table index also represents the precedence in selecting target DNS server: "
                "less index value means higher priority in round-robin selection. Only one "
                "server is working at a time, that is when the chosen server is active, system "
                "marks the designated server as target and stops selection. When the active "
                "server becomes inactive, system starts another round of selection starting from "
                "the next available server setting.")
        );
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_dns_server_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2), vtss::tag::Name("server_status"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dns_server_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_dns_server_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP);
};

}  // namespace interfaces
}  // namespace dns
}  // namespace appl
}  // namespace vtss
#endif /* __VTSS_DNS_SERIALIZER_HXX__ */
