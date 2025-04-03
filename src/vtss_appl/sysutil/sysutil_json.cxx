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
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::sysutil::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_sysutil("systemUtility");
extern "C" void vtss_appl_sysutil_json_init() { json_node_add(&ns_sysutil); }

NS(ns_conf,       ns_sysutil, "config");
NS(ns_status,     ns_sysutil, "status");
NS(ns_statistics, ns_sysutil, "statistics");
NS(ns_control,    ns_sysutil, "control");

static StructReadOnly<SysutilCapabilitiesLeaf> sysutil_capabilities_leaf(
        &ns_sysutil, "capabilities");

static StructReadOnly<SysutilStatusCpuLoadLeaf> sysutil_status_cpu_load_leaf(
        &ns_status, "cpuLoad");

static TableReadOnly<SysutilPowerSupplyLeaf> sysutil_power_supply_leaf(
        &ns_status, "powerSupply");

static TableReadOnly<SysutilSystemLedLeaf> sysutil_system_led_leaf(
        &ns_status, "systemLed");

static StructReadOnly<SysutilBoardInfoEntry> sysutil_board_info_leaf(
        &ns_status, "boardinfo");

static StructReadOnly<SysutilSystemUptimeLeaf> sysutil_system_uptime_leaf(
        &ns_status, "systemUptime");

static TableWriteOnly<SysutilControlRebootEntry> sysutil_control_reboot_entry(
        &ns_control, "reboot");

static TableWriteOnly<SysutilControlSystemLedEntry> sysutil_control_system_led_entry(
        &ns_control, "systemLed");

static StructReadWrite<SysutilConfigSystemInfo> sysutil_config_system_info(
        &ns_conf, "systemInfo");

static StructReadWrite<SysutilSystemTimeInfo> sysutil_config_system_time(
        &ns_conf, "systemTime");

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
static TableReadWrite<SysutilConfigTemperatureMonitor> sysutil_config_temperature_monitor(
        &ns_conf, "temperatureMonitor");

static TableReadOnly<SysutilStatusTemperatureMonitor> sysutil_status_temperature_monitor(
        &ns_status, "temperatureMonitor");

/* JSON notification */
static TableReadOnlyNotification<SysutilEventTemperatureMonitor> sysutil_event_temperature_monitor(
        &ns_status, "temperatureMonitorEvent", &tm_status_event_update);
#endif /* VTSS_APPL_SYSUTIL_TM_SUPPORTED */

