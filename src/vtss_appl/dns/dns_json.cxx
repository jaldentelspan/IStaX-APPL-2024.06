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
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::dns::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_dns("dns");
extern "C" void vtss_appl_dns_json_init()
{
    json_node_add(&ns_dns);
}

NS(ns_conf,             ns_dns,     "config");
NS(ns_status,           ns_dns,     "status");

NS(ns_config_globals,   ns_conf,    "global");
NS(ns_config_servers,   ns_conf,    "server");
NS(ns_status_globals,   ns_status,  "global");
NS(ns_status_servers,   ns_status,  "server");

static StructReadOnly<DnsCapabilitiesLeaf> dns_capabilities(
    &ns_dns, "capabilities");

static StructReadWrite<DnsConfigProxyLeaf> dns_config_proxy_leaf(
    &ns_config_globals, "proxy");

static StructReadWrite<DnsConfigDomainNameLeaf> dns_config_domain_name_leaf(
    &ns_config_globals, "domainName");

static TableReadWriteNoNS<DnsConfigServersTable> dns_config_servers_table(
    &ns_config_servers);

static StructReadOnly<DnsStatusDomainNameLeaf> dns_status_domainname_leaf(
    &ns_status_globals, "defaultDomainName");

static TableReadOnlyNoNS<DnsStatusServersTable> dns_status_servers_table(
    &ns_status_servers);
