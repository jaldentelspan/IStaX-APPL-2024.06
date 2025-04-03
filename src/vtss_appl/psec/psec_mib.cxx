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

#include "psec_serializer.hxx"
#include <vtss/appl/psec.h>

using namespace vtss;
using namespace vtss::expose::snmp;

VTSS_MIB_MODULE("PsecMib", "PSEC", psec_mib_init, VTSS_MODULE_ID_PSEC, root, h) {
    h.description("Private MIB for Port Security");
    h.add_history_element("201410130000Z", "Initial version");
    h.add_history_element("201412080000Z", "Add users in status port table");
    h.add_history_element("201412100000Z", "Remove user of DHCP snooping");
    h.add_history_element("201606020000Z", "Support SNMP trap");
    h.add_history_element("201610240000Z", "Restructured entire MIB in a non-backward-compatible way");
    h.add_history_element("201801260000Z", "Added support for static and sticky MACs");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(PSEC_MIB_node_psec,               root,                 1, "psecMibObjects");
NS(PSEC_MIB_node_config,             PSEC_MIB_node_psec,   2, "psecConfig");
NS(PSEC_MIB_node_config_globals,     PSEC_MIB_node_config,  1, "psecConfigGlobals");
NS(PSEC_MIB_node_config_interfaces,  PSEC_MIB_node_config,  2, "psecConfigInterfaces");
NS(PSEC_MIB_node_status,             PSEC_MIB_node_psec,   3, "psecStatus");
NS(PSEC_MIB_node_status_globals,     PSEC_MIB_node_status,  1, "psecStatusGlobals");
NS(PSEC_MIB_node_status_interfaces,  PSEC_MIB_node_status,  2, "psecStatusInterfaces");
NS(PSEC_MIB_node_control,            PSEC_MIB_node_psec,   4, "psecControl");
NS(PSEC_MIB_node_control_globals,    PSEC_MIB_node_control, 1, "psecControlGlobals");
NS(PSEC_MIB_node_control_interfaces, PSEC_MIB_node_control, 2, "psecControlInterfaces");
NS(PSEC_MIB_node_traps,              PSEC_MIB_node_psec,   6, "psecTrap");
NS(PSEC_MIB_node_traps_globals,      PSEC_MIB_node_traps,   1, "psecTrapGlobals");
NS(PSEC_MIB_node_traps_interfaces,   PSEC_MIB_node_traps,   2, "psecTrapInterfaces");

namespace vtss {
namespace appl {
namespace psec {

static StructRO2               <Capabilities>                capabilities                 (&PSEC_MIB_node_psec,               OidElement(1, "psecCapabilities"));
static StructRW2               <ConfigGlobal>                config_global                (&PSEC_MIB_node_config_globals,     OidElement(1, "psecConfigGlobalsMain"));
static TableReadWrite2         <ConfigInterface>             config_interface             (&PSEC_MIB_node_config_interfaces,  OidElement(1, "psecConfigInterfacesTable"));
static TableReadWriteAddDelete2<ConfigInterfaceMac>          config_interface_mac         (&PSEC_MIB_node_config_interfaces,  OidElement(2, "psecConfigInterfacesMacTable"),
                                                                                                                              OidElement(3, "psecConfigInterfacesMacTableRowEditor"));
static StructRO2               <StatusGlobal>                status_global                (&PSEC_MIB_node_status_globals,     OidElement(1, "psecStatusGlobalsMain"));
static StructRoTrap            <StatusGlobalNotification>    status_global_notification   (&PSEC_MIB_node_status_globals,     OidElement(2, "psecStatusGlobalsNotification"),
                                                                                           &PSEC_MIB_node_traps_globals,      OidElement(1, "psecTrapGlobalsMain"), &psec_global_notification_status);
static TableReadOnly2          <StatusInterface>             status_interface             (&PSEC_MIB_node_status_interfaces,  OidElement(1, "psecStatusInterfacesTable"));
static TableReadOnlyTrap       <StatusInterfaceNotification> status_interface_notification(&PSEC_MIB_node_status_interfaces,  OidElement(2, "psecStatusInterfacesNotificationTable"), &psec_interface_notification_status,
                                                                                           &PSEC_MIB_node_traps_interfaces, "psecTrapInterfaces",
                                                                                           OidElement(1, "psecTrapInterfacesAdd"),
                                                                                           OidElement(2, "psecTrapInterfacesMod"),
                                                                                           OidElement(3, "psecTrapInterfacesDel"));
static TableReadOnly2          <StatusInterfaceMac>          status_interface_mac         (&PSEC_MIB_node_status_interfaces,  OidElement(3, "psecStatusInterfacesMacTable"));
static StructRW2               <ControlGlobalMacClear>       control_global_mac_clear     (&PSEC_MIB_node_control_globals,    OidElement(1, "psecControlGlobalsMacClear"));

}  // namespace psec
}  // namespace appl
}  // namespace vtss
