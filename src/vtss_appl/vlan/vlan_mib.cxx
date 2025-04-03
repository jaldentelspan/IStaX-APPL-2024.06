/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include <vtss/appl/vlan.h>
#include "vlan_trace.h"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("vlanMib", "VLAN", vlan_mib_init, VTSS_MODULE_ID_VLAN, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201501160000Z", "Added Shared VLAN Learning table");
    h.add_history_element("201904050000Z", "Obsoleted a couple of VLAN users");
    h.add_history_element("202008240000Z", "Add support for managing flooding");
    h.add_history_element("202104290000Z", "Added support for MRP VLAN user");
    h.description("Private MIB for VLAN.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(VLAN_MIB_node_vlan, root, 1, "vlanMibObjects");
NS(VLAN_MIB_node_config, VLAN_MIB_node_vlan, 2, "vlanConfig");
NS(VLAN_MIB_node_status, VLAN_MIB_node_vlan, 3, "vlanStatus");
NS(VLAN_MIB_node_config_globals,    VLAN_MIB_node_config, 1, "vlanConfigGlobals");
NS(VLAN_MIB_node_config_interfaces, VLAN_MIB_node_config, 2, "vlanConfigInterfaces");
NS(VLAN_MIB_node_status_interfaces, VLAN_MIB_node_status, 1, "vlanStatusInterfaces");
NS(VLAN_MIB_node_status_memberships, VLAN_MIB_node_status, 2, "vlanStatusMemberships");

namespace vtss {
namespace appl {
namespace vlan {
namespace interfaces {

// Parent: vtssVlanMIB.vlanMIBObjects(1)
static StructRO2<Capabilities> capabilities(&VLAN_MIB_node_vlan, OidElement(1, "vlanCapabilities"));

// vtssVlanMIB.vlanMIBObjects(1).vlanConfig(2).vlanConfigGlobals(1).vlanConfigGlobalsMain(1) (.1.2.1.1)
static StructRW2<ConfigGlobal> config_global(&VLAN_MIB_node_config_globals, OidElement(1, "vlanConfigGlobalsMain"));

// vtssVlanMIB.vlanMIBObjects(1).vlanConfig(2).vlanConfigGlobals(1).vlanNameTable(2) (.1.2.1.2)
static TableReadWrite2<NameTable> name_table(&VLAN_MIB_node_config_globals, OidElement(2, "vlanConfigGlobalsNameTable"));

// vtssVlanMIB.vlanMIBObjects(1).vlanConfig(2).vlanConfigGlobals(1).vlanFloodingTable(2) (.1.2.1.3)
static TableReadWrite2<FloodingTable> flooding_table(&VLAN_MIB_node_config_globals, OidElement(3, "vlanConfigGlobalsFloodingTable"));

// vtssVlanMIB.vlanMIBObjects(1).vlanConfig(2).vlanConfigInterfaces(2).vlanConfigInterfacesTable(1) (.1.2.2.1)
static TableReadWrite2<PortConf> port_conf(&VLAN_MIB_node_config_interfaces, OidElement(1, "vlanConfigInterfacesTable"));

// vtssVlanMIB.vlanMIBOjbects(1).vlanConfig(2).vlanConfigInterfaces(2).vlanConfigInterfacesSvlTable(2) (.1.2.2.2)
static TableReadWriteAddDelete2<SvlTable> svl_table(&VLAN_MIB_node_config_interfaces,
                                                   OidElement(2, "vlanConfigInterfacesSvlTable"),
                                                   OidElement(3, "vlanConfigInterfacesSvlTableRowEditor"));

// vtssVlanMIB.vlanMIBObjects(1).vlanStatus(3).vlanStatusInterfaces(1).vlanStatusInterfaceTable(1) (.1.3.1.1)
static TableReadOnly2<StatisIf> statis_if(&VLAN_MIB_node_status_interfaces, OidElement(1, "vlanStatusInterfacesTable"));

// vtssVlanMIB.vlanMIBObjects(1).vlanStatus(3).vlanStatusMemberships(2).vlanStatusMembershipVlan(1) (.1.3.2.1)
static TableReadOnly2<StatusMembership> status_membership(&VLAN_MIB_node_status_memberships, OidElement(1, "vlanStatusMembershipsVlanTable"));
}  // namespace interfaces
}  // namespace vlan
}  // namespace appl
}  // namespace vtss

