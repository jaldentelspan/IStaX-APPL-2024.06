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

#include "dhcp_server_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::dhcp_server::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_dhcp_server("dhcpServer");
extern "C" void vtss_appl_dhcp_server_json_init() { json_node_add(&ns_dhcp_server); }

NS(ns_conf,               ns_dhcp_server, "config");
NS(ns_status,             ns_dhcp_server, "status");
NS(ns_control,            ns_dhcp_server, "control");
NS(ns_control_statistics, ns_control,     "statistics");

static StructReadWrite<DhcpServerConfigGlobalsLeaf> dhcp_server_config_globals_leaf(&ns_conf, "global");
static TableReadWrite<DhcpServerConfigVlanEntry> dhcp_server_config_vlan_entry(&ns_conf, "vlan");
static TableReadWriteAddDelete<DhcpServerConfigExcludedEntry> dhcp_server_config_excluded_entry(&ns_conf, "excluded");
#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
static TableReadWriteAddDelete<DhcpServerConfigReservedEntry> dhcp_server_config_reserved_entry(&ns_conf, "reserved");
#endif
static TableReadWriteAddDelete<DhcpServerConfigPoolEntry> dhcp_server_config_pool_entry(&ns_conf, "pool");
static TableReadOnly<DhcpServerStatusDeclinedEntry> dhcp_server_status_declined_entry(&ns_status, "declined");
static StructReadOnly<DhcpServerStatusStatisticsLeaf> dhcp_server_status_statistics_leaf(&ns_dhcp_server, "statistics");
static TableReadOnly<DhcpServerStatusBindingEntry> dhcp_server_status_binding_entry(&ns_status, "binding");
static StructWriteOnly<DhcpServerControlStatisticsLeaf> dhcp_server_control_statistics_leaf(&ns_control_statistics, "clear");
static StructWriteOnly<DhcpServerControlBindingLeaf> dhcp_server_control_binding_leaf(&ns_control, "binding");
