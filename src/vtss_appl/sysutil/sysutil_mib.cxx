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

#include "sysutil_serializer.hxx"
#include "vtss/basics/enum-descriptor.h"    // vtss_enum_descriptor_t

VTSS_MIB_MODULE("sysutilMib", "SYSUTIL", sysutil_mib_init,
                VTSS_MODULE_ID_SYSTEM, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201410100000Z", "Editorial changes");
    h.add_history_element("201411110000Z", "Add system LED status");
    h.add_history_element("201510150000Z", "Add system uptime status");
    h.add_history_element("201510200000Z", "Add system config info");
    h.add_history_element("201510300000Z", "Add board info");
    h.add_history_element("201511020000Z", "Add system time config");
    h.add_history_element("201602150000Z", "Add board serial and type to board info");
    h.add_history_element("201602170000Z", "Add system temperature monitor");
    h.description("This is a private version of SysUtil");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace sysutil {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(sysutil_mib_objects, root, 1, "sysutilMibObjects");
NS(sysutil_config, sysutil_mib_objects, 2, "sysutilConfig");
NS(sysutil_status, sysutil_mib_objects, 3, "sysutilStatus");
NS(sysutil_control, sysutil_mib_objects, 4, "sysutilControl");

static StructRO2<SysutilCapabilitiesLeaf> sysutil_capabilities_leaf(
        &sysutil_mib_objects, vtss::expose::snmp::OidElement(1, "sysutilCapabilities"));

static StructRO2<SysutilStatusCpuLoadLeaf> sysutil_status_cpu_load_leaf(
        &sysutil_status, vtss::expose::snmp::OidElement(1, "sysutilStatusCpuLoad"));

static TableReadOnly2<SysutilPowerSupplyLeaf> sysutil_power_supply_leaf(
        &sysutil_status, vtss::expose::snmp::OidElement(2, "sysutilStatusPowerSupply"));

static TableReadOnly2<SysutilSystemLedLeaf> sysutil_system_led_leaf(
        &sysutil_status, vtss::expose::snmp::OidElement(3, "sysutilStatusSystemLed"));

static StructRO2<SysutilSystemUptimeLeaf> sysutil_system_uptime_leaf(
        &sysutil_status, vtss::expose::snmp::OidElement(4, "sysutilStatusSystemUptime"));

static StructRO2<SysutilBoardInfoEntry> sysutil_board_info_leaf(
        &sysutil_status, vtss::expose::snmp::OidElement(5, "sysutilStatusBoardInfo"));

static TableReadWrite2<SysutilControlRebootEntry> sysutil_control_reboot_entry(
        &sysutil_control, vtss::expose::snmp::OidElement(1, "sysutilControlReboot"));

static TableReadWrite2<SysutilControlSystemLedEntry> sysutil_control_system_led_entry(
        &sysutil_control, vtss::expose::snmp::OidElement(2, "sysutilControlSystemLed"));



static StructRW2<SysutilConfigSystemInfo> sysutil_config_system_info(
        &sysutil_config, vtss::expose::snmp::OidElement(1, "sysutilConfigSystemInfo"));

static StructRW2<SysutilSystemTimeInfo> sysutil_config_system_time(
        &sysutil_config, vtss::expose::snmp::OidElement(2, "sysutilConfigSystemTime"));

static TableReadWrite2<SysutilConfigTemperatureMonitor> sysutil_config_temperature_monitor(
        &sysutil_config, vtss::expose::snmp::OidElement(3, "sysutilConfigTemperatureMonitor"));

static TableReadOnly2<SysutilStatusTemperatureMonitor> sysutil_status_temperature_monitor(
        &sysutil_status, vtss::expose::snmp::OidElement(6, "sysutilStatusTemperatureMonitor"));

}  // namespace interfaces
}  // namespace sysutil
}  // namespace appl
}  // namespace vtss

