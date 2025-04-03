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
#include "port_power_savings_serializer.hxx"

VTSS_MIB_MODULE("PortPowerSavingsMib", "PORT-POWER-SAVINGS",
                vtss_appl_port_power_savings_mib_init,
                VTSS_MODULE_ID_GREEN_ETHERNET, root, h) {
    h.add_history_element("201408070000Z", "Initial version");
    h.description(
            "This is a private version of Port Power Saving. Port power saving "
            "reduces the switch power consumptionby lowering the port power "
            "supply when there is no link partner connected to a port as well "
            "as when link partner is connected through a short cable.");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace port_power_savings {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(pps_mib_objects, root, 1, "PortPowerSavingsMibObjects");
NS(pps_capability, pps_mib_objects, 1, "PortPowerSavingsCapabilities");
NS(pps_config, pps_mib_objects, 2, "PortPowerSavingsConfig");
NS(pps_status, pps_mib_objects, 3, "PortPowerSavingsStatus");

static TableReadOnly2<ppsInterfaceCapabilitiesEntry>
        pps_interface_capabilities_entry(
                &pps_capability,
                vtss::expose::snmp::OidElement(
                        1, "PortPowerSavingsCapabilitiesInterface"));

static TableReadWrite2<ppsInterfaceConfigEntry> pps_interface_config_entry(
        &pps_config,
        vtss::expose::snmp::OidElement(1, "PortPowerSavingsConfigInterfaceParamTable"));

static TableReadOnly2<ppsInterfaceStatusEntry> pps_interface_status_entry(
        &pps_status,
        vtss::expose::snmp::OidElement(1, "PortPowerSavingsStatusInterfaceTable"));

}  // namespace interfaces
}  // namespace port power saving
}  // namespace appl
}  // namespace vtss
