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

#include "vlan_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::vlan::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_vlan("vlan");
extern "C" void vtss_appl_vlan_json_init() { json_node_add(&ns_vlan); }

NS(VLAN_JSON_node_config,         ns_vlan,               "config");
NS(VLAN_JSON_node_interface,      VLAN_JSON_node_config, "interface");
NS(VLAN_JSON_node_status,         ns_vlan,               "status");
NS(VLAN_JSON_node_config_globals, VLAN_JSON_node_config, "global");

namespace vtss {
namespace appl {
namespace vlan {
namespace interfaces {

// vlan.capabilities
static StructReadOnly<Capabilities> capabilities(&ns_vlan, "capabilities");

// vlan.config.globals.main
static StructReadWrite<ConfigGlobal> config_global(&VLAN_JSON_node_config_globals, "main");

// vlan.config.globals.name
static TableReadWrite<NameTable> name_table(&VLAN_JSON_node_config_globals, "name");

// vlan.config.globals.flooding
static TableReadWrite<FloodingTable> flooding_table(&VLAN_JSON_node_config_globals, "flooding");

// vlan.config.interface
static TableReadWriteNoNS<PortConf> port_conf(&VLAN_JSON_node_interface);

// vlan.config.interface.svl
static TableReadWriteAddDelete<SvlTable> svl_table(&VLAN_JSON_node_interface, "svl");

// vlan.status.interface
static TableReadOnly<StatisIf> statis_if(&VLAN_JSON_node_status, "interface");

// vlan.status.membership
static TableReadOnly<StatusMembership> status_membership(&VLAN_JSON_node_status, "membership");
}  // namespace interfaces
}  // namespace vlan
}  // namespace appl
}  // namespace vtss

