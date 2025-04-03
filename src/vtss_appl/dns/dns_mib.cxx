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

#include "dns_serializer.hxx"

VTSS_MIB_MODULE("dnsMib", "DNS", vtss_appl_dns_mib_init, VTSS_MODULE_ID_IP_DNS, root, h)
{
    h.add_history_element("201407010000Z", "Initial version.");
    h.add_history_element(
            "201411250000Z",
            "Support multiple DNS server settings and default domain name configuration.");
    h.description("This is a private version of the DNS MIB.");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss
{
namespace appl
{
namespace dns
{
namespace interfaces
{

/* Construct the MIB Objects hierarchy */
/*
    xxxDnsMIBObjects        +-  xxxDnsCapabilities
                            +-  xxxDnsConfig
                                +-  xxxDnsConfigGlobals -> ...
                                +-  xxxDnsConfigServers -> ...
                            +-  xxxDnsStatus
                                +-  xxxDnsStatusGlobals -> ...

    xxxDnsMIBConformance will be generated automatically
*/
/* root: parent node (See VTSS_MIB_MODULE) */
NS(dns_objects,             root,               1,  "dnsMibObjects");

/* xxxDnsCapabilities */
static StructRO2<DnsCapabilitiesLeaf> dns_capabilities_leaf(
    &dns_objects,
    vtss::expose::snmp::OidElement(1, "dnsCapabilities"));

/* xxxDnsConfig */
NS(dns_config,              dns_objects,        2,  "dnsConfig");
/* xxxDnsStatus */
NS(dns_status,              dns_objects,        3,  "dnsStatus");

/* xxxDnsConfig:xxxDnsConfigGlobals */
NS(dns_config_globals,      dns_config,         1,  "dnsConfigGlobals");

/* xxxDnsConfig:xxxDnsConfigServer */
NS(dns_config_server,       dns_config,         2,  "dnsConfigServers");

static StructRW2<DnsConfigProxyLeaf> dns_config_proxy_leaf(
    &dns_config_globals,
    vtss::expose::snmp::OidElement(1, "dnsConfigGlobalsProxy"));

static StructRW2<DnsConfigDomainNameLeaf> dns_config_domain_name_leaf(
    &dns_config_globals,
    vtss::expose::snmp::OidElement(2, "dnsConfigGlobalsDefaultDomainName"));

/* xxxDnsConfigServerTable */
static TableReadWrite2<DnsConfigServersTable> dns_config_servers_table(
    &dns_config_server,
    vtss::expose::snmp::OidElement(1, "dnsConfigServersTable"));

/* xxxDnsStatus:xxxDnsStatusGlobals */
NS(dns_status_globals,      dns_status,         1,  "dnsStatusGlobals");

/* xxxDnsStatus:xxxDnsStatusServers */
NS(dns_status_servers,      dns_status,         2,  "dnsStatusServers");

static StructRO2<DnsStatusDomainNameLeaf> dns_status_domainname_leaf(
    &dns_status_globals,
    vtss::expose::snmp::OidElement(1, "dnsStatusGlobalsDefaultDomainName"));

/* xxxDnsStatusServerTable */
static TableReadOnly2<DnsStatusServersTable> dns_status_servers_table(
    &dns_status_servers,
    vtss::expose::snmp::OidElement(1, "dnsStatusServersTable"));

}  // namespace interfaces
}  // namespace dns
}  // namespace appl
}  // namespace vtss
