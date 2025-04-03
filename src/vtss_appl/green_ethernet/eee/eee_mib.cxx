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
#include "eee_serializer.hxx"
VTSS_MIB_MODULE("EeeMib", "EEE", eee_mib_init, VTSS_MODULE_ID_EEE, root, h) {
    h.add_history_element("201407240000Z", "Initial version");
    h.description("This is a private version of Energy Efficient Ethernet(EEE). ");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace eee {
namespace interfaces {
NS(eee_mib_objects, root, 1, "EeeMibObjects");
NS(eee_capability, eee_mib_objects, 1, "EeeCapabilities");
NS(eee_config, eee_mib_objects, 2, "EeeConfig");
NS(eee_status, eee_mib_objects, 3, "EeeStatus");
NS(eee_config_if, eee_config, 2, "EeeConfigInterface");

static StructRW2<eeeGlobalConfigEntry> eee_global_config_entry(
        &eee_config, vtss::expose::snmp::OidElement(1, "EeeConfigGlobals"));

static TableReadWrite2<eeeInterfaceConfigEntry> eee_interface_config_entry(
        &eee_config_if, vtss::expose::snmp::OidElement(1, "EeeConfigInterfaceParamTable"));

static TableReadWrite2<eeeInterfaceEgressQueueConfigEntry> eee_interface_egress_queue_config_entry(
        &eee_config_if, vtss::expose::snmp::OidElement(2, "EeeConfigInterfaceQueueTable"));

static TableReadOnly2<eeeInterfaceStatusEntry> eee_interface_status_entry(
        &eee_status, vtss::expose::snmp::OidElement(1, "EeeStatusInterfaceTable"));

static StructRO2<eeeGlobalsCapabilitiesEntry> eee_globals_capabilities_entry(
        &eee_capability, vtss::expose::snmp::OidElement(1, "EeeCapabilitiesGlobals"));

static TableReadOnly2<eeeInterfaceCapabilitiesEntry> eee_interface_capabilities_entry(
        &eee_capability, vtss::expose::snmp::OidElement(2, "EeeCapabilitiesInterface"));
}  // namespace interfaces
}  // namespace eee
}  // namespace appl
}  // namespace vtss
