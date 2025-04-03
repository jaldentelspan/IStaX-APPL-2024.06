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

#include "dhcp6_client_serializer.hxx"

VTSS_MIB_MODULE("dhcp6ClientMib", "DHCP6-CLIENT", vtss_appl_dhcp6_client_mib_init, VTSS_MODULE_ID_DHCP6C, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private version of the DHCPv6 Client MIB");
}


using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace dhcp6_client {
namespace interfaces {

/* Construct the MIB Objects hierarchy */
/*
    xxxDhcp6ClientMIBObjects    +-  xxxDhcp6ClientCapabilities
                                +-  xxxDhcp6ClientConfig    -> ...
                                +-  xxxDhcp6ClientStatus    -> ...
                                +-  xxxDhcp6ClientControl   -> ...

    xxxDhcp6ClientMIBConformance will be generated automatically
*/
/* root: parent node (See VTSS_MIB_MODULE) */
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(dhcp6_client_mib_objects,    root,                       1,  "dhcp6ClientMibObjects");

/* xxxDhcp6ClientCapabilities:xxxDhcp6ClientCapabilitiesMaxNumberOfInterfaces */
static StructRO2<Dhcp6ClientCapabilitiesLeaf> dhcp6_client_capabilities_leaf(
    &dhcp6_client_mib_objects,
    vtss::expose::snmp::OidElement(1, "dhcp6ClientCapabilities"));

NS(dhcp6_client_config,         dhcp6_client_mib_objects,   2,  "dhcp6ClientConfig");
NS(dhcp6_client_status,         dhcp6_client_mib_objects,   3,  "dhcp6ClientStatus");
NS(dhcp6_client_control,        dhcp6_client_mib_objects,   4,  "dhcp6ClientControl");
NS(dhcp6_client_config_intf,    dhcp6_client_config,        1,  "dhcp6ClientConfigInterface");
NS(dhcp6_client_status_intf,    dhcp6_client_status,        1,  "dhcp6ClientStatusInterface");
NS(dhcp6_client_control_intf,   dhcp6_client_control,       1,  "dhcp6ClientControlInterface");

/* xxxDhcp6ClientConfig:xxxDhcp6ClientConfigInterface:xxxDhcp6ClientConfigInterfaceTable */
static TableReadWriteAddDelete2<Dhcp6ClientInterfaceConfigurationTable> dhcp6_client_intf_conf_table(
    &dhcp6_client_config_intf,
    vtss::expose::snmp::OidElement(1, "dhcp6ClientConfigInterfaceTable"),
    vtss::expose::snmp::OidElement(2, "dhcp6ClientConfigInterfaceTableRowEditor"));

/* xxxDhcp6ClientStatus:xxxDhcp6ClientStatusInterface:xxxDhcp6ClientStatusInterfaceInformationTable */
static TableReadOnly2<Dhcp6ClientInterfaceInformationTable> dhcp6_client_intf_info_table(
        &dhcp6_client_status_intf,
        vtss::expose::snmp::OidElement(1, "dhcp6ClientStatusInterfaceInformationTable"));

/* xxxDhcp6ClientStatus:xxxDhcp6ClientStatusInterface:xxxDhcp6ClientStatusInterfaceStatisticsTable */
static TableReadOnly2<Dhcp6ClientInterfaceStatisticsTable> dhcp6_client_intf_cntr_table(
        &dhcp6_client_status_intf,
        vtss::expose::snmp::OidElement(2, "dhcp6ClientStatusInterfaceStatisticsTable"));

/* xxxDhcp6ClientControl:xxxDhcp6ClientControlInterface:xxxDhcp6ClientControlInterfaceRestartIfIndex */
static StructRW2<Dhcp6ClientActionLeaf> dhcp6_client_intf_action_leaf(
        &dhcp6_client_control_intf,
        vtss::expose::snmp::OidElement(1, "dhcp6ClientControlInterfaceRestart"));

}  // namespace interfaces
}  // namespace dhcp6_client
}  // namespace appl
}  // namespace vtss
