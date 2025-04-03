/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "fan_serializer.hxx"
VTSS_MIB_MODULE("FanMib", "FAN", fan_mib_init, VTSS_MODULE_ID_FAN, root, h) {
    h.add_history_element("201405220000Z", "Initial version");
    h.add_history_element("202311220000Z", "Added object vtssFanConfigGlobalsPwmFrequency to vtssFanConfigGlobals");
    h.description("This is a private MIB for controlling fan speed, in order to reduce noise and power consumption");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace fan {
namespace interfaces {

// root
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(fanMibObjects, root, 1, "fanMibObjects");;

// parent: fan
NS(fanConfig, fanMibObjects, 2, "fanConfig");
NS(fanStatus, fanMibObjects, 3, "fanStatus");

static StructRO2<FanCapabilities> fan_capabilities(
    &fanMibObjects, vtss::expose::snmp::OidElement(1, "fanCapabilities"));

static StructRW2<FanConfigEntry> fan_config_entry(
    &fanConfig, vtss::expose::snmp::OidElement(1, "fanConfigGlobals"));

static TableReadOnly2<FanStatusSpeedEntry> fan_status_speed_entry(
    &fanStatus, vtss::expose::snmp::OidElement(1, "fanStatusSpeed"));

static TableReadOnly2<FanStatusSensorEntry> fan_status_sensor_entry(
    &fanStatus, vtss::expose::snmp::OidElement(2, "fanStatusSensor"));

}  // interfaces
}  // namespace fan
}  // namespace appl
}  // namespace vtss
