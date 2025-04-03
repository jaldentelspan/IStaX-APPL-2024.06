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
#include "thermal_protect_serializer.hxx"
#include "vtss/appl/thermal_protect.h"

VTSS_MIB_MODULE("ThermalProtectionMib", "THERMAL-PROTECTION",
                Thermal_protection_mib_init, VTSS_MODULE_ID_THERMAL_PROTECT,
                root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private version of thermal protection. The PHY "
                  "thermal protections consists of four groups. Each PHY is "
                  "associated to a group, and each group has a configured max "
                  "temperature. If the average temperature of all sensors "
                  "exceeds the configured max temperature of a group, then the "
                  "PHYs in that group is shoutdown.");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace thermal_protect {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(thermal_protect_mib_objects, root, 1, "ThermalProtectionMibObjects");
NS(thermal_protect_config, thermal_protect_mib_objects, 2, "ThermalProtectionConfig");
NS(thermal_protect_status, thermal_protect_mib_objects, 3, "ThermalProtectionStatus");
NS(thermal_protect_config_global, thermal_protect_config, 1, "ThermalProtectionConfigGlobals");
NS(thermal_protect_config_if, thermal_protect_config, 2, "ThermalProtectionConfigInterface");

static TableReadWrite2<ThermalProtectPriorityTempEntry> thermal_protect_priority_temp_entry(
        &thermal_protect_config_global, vtss::expose::snmp::OidElement(1, "thermalProtectionConfigGlobalsParamTable"));

static TableReadWrite2<ThermalProtectInterfacePriorityEntry> thermal_protect_interface_priority_entry(
        &thermal_protect_config_if, vtss::expose::snmp::OidElement(1, "thermalProtectionConfigInterfaceParamTable"));

static TableReadOnly2<ThermalProtectionInterfaceStatusEntry> thermal_protection_interface_status_entry(
        &thermal_protect_status, vtss::expose::snmp::OidElement(1, "thermalProtectionStatusInterfaceTable"));

static StructRO2<ThermalProtectionCapabilitiesEntry> thermal_protection_capabilities_entry(
        &thermal_protect_mib_objects, vtss::expose::snmp::OidElement(1, "thermalProtectionCapabilities"));
}  // namespace interfaces
}  // namespace thermal_protect
}  // namespace appl
}  // namespace vtss
