/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "poe_serializer.hxx"
VTSS_MIB_MODULE("poeMib", "POE", poe_mib_init, VTSS_MODULE_ID_POE, root, h) {
    h.add_history_element("201408200000Z", "Initial version");
    h.add_history_element("201609300000Z", "Enhanced PoE interface status messages. Added reserved power status entry.");
    h.add_history_element("201903140000Z", "Refactor according to IEEE802.3BT requirement.");
    h.add_history_element("201904100000Z", "Add ability to disable PoE lldp functionality.");
    h.add_history_element("202303270000Z", "Change structure of global data for PoE.");
    h.add_history_element("202308300000Z", "Change structure of global data for PoE to better capture all features of IEEE802.3BT.");
    h.add_history_element("202310030000Z", "Added attrributes vtssPoeConfigGlobalsSystemPwrReserved "
                          "(causing some other attributes to change oid) and "
                          "vtssPoeConfigSwitchParamSystemPwrReserved (also causing some other "
                          "attributes to change oid).");
    h.add_history_element("202310240000Z", "Change name of object vtssPoeConfigSwitchParamSystemPwrReserved to vtssPoeConfigSwitchParamSystemPwrUsage");
    h.add_history_element("202405070000Z", "Extended VTSSPoeStatusInterfaceEntry "
                          "with vtssPoeStatusInterfaceUdlCount, vtssPoeStatusInterfaceOvlCount, "
                          "vtssPoeStatusInterfaceShortCircuitCount, "
                          "vtssPoeStatusInterfaceInvalidSignatureCount, "
                          "vtssPoeStatusInterfacePowerDeniedCount and changed type of "
                          "vtssPoeStatusInterfacePDClassAltA and vtssPoeStatusInterfacePDClassAltB "
                          "from Integer32 to VTSSUnsigned8 ");
    h.description("This is a private version of Power over Ethernet (PoE).");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace poe {
namespace interfaces {
NS(poe_mib_objects, root, 1, "poeMibObjects");
NS(poe_capability, poe_mib_objects, 1, "poeCapabilities");
NS(poe_config, poe_mib_objects, 2, "poeConfig");
NS(poe_status, poe_mib_objects, 3, "poeStatus");

NS(poe_config_switch, poe_config, 2, "poeConfigSwitch");
NS(poe_config_if, poe_config, 3, "poeConfigInterface");

static StructRW2<poeSwitchConfigEntry> poe_global_config_entry(
        &poe_config, vtss::expose::snmp::OidElement(1, "poeConfigGlobals"));

static TableReadWrite2<poeSwitchConfigEntry> poe_switch_config_entry(
        &poe_config_switch, vtss::expose::snmp::OidElement(1, "poeConfigSwitchParamTable"));

static TableReadWrite2<poeInterfaceConfigEntry> poe_interface_config_entry(
        &poe_config_if, vtss::expose::snmp::OidElement(1, "poeConfigInterfaceParamTable"));

static TableReadOnly2<poeInterfaceStatusEntry> poe_interface_status_entry(
        &poe_status, vtss::expose::snmp::OidElement(1, "poeStatusInterfaceTable"));

static TableReadOnly2<poeInterfaceCapabilitiesEntry> poe_interface_capabilities_entry(
        &poe_capability, vtss::expose::snmp::OidElement(1, "poeCapabilitiesInterface"));

static StructRO2<poePsuCapabilitiesEntry> poe_psu_capabilities_entry(
        &poe_capability, vtss::expose::snmp::OidElement(2, "poeCapabilitiesPsu"));

}  // namespace interfaces
}  // namespace poe
}  // namespace appl
}  // namespace vtss
