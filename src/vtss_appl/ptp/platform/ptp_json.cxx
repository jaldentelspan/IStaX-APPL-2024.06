/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "ptp_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::ptp::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_ptp("ptp");
extern "C" void vtss_appl_ptp_json_init() { json_node_add(&ns_ptp); }

// Parent: vtss/ptp ------------------------------------------------------------
NS(ns_config, ns_ptp, "config");
NS(ns_status, ns_ptp, "status");
NS(ns_control, ns_ptp, "control");
NS(ns_statistics, ns_ptp, "statistics");

// Parent: vtss/ptp/config -----------------------------------------------------
NS(ns_config_globals, ns_config, "global");
NS(ns_config_clocks, ns_config, "clocks");

// Parent: vtss/ptp/status -----------------------------------------------------
NS(ns_status_clocks, ns_status, "clocks");

// Parent: vtss/ptp/status -----------------------------------------------------
NS(ns_status_clocks_unicast, ns_status_clocks, "unicast");

// Parent: vtss/ptp/capabilities------------------------------------------------
static StructReadOnly<GlobalCapabilities> ro_capabilities_globals(&ns_ptp, "capabilities");

// Parent: vtss/ptp/config/globals ---------------------------------------------
static StructReadWrite<GlobalsExternalClockMode> rw_config_globals_external_clock_mode(&ns_config_globals, "externalClockMode");

// Parent: vtss/ptp/config/globals ---------------------------------------------
static StructReadWrite<GlobalsSystemTimeSyncMode> rw_config_globals_system_time_sync_mode(&ns_config_globals, "systemTimeSyncMode");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWriteAddDelete<ConfigClocksDefaultDS> rw_config_clocks_default_ds_table(&ns_config_clocks, "defaultDs");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksTimePropertiesDS> rw_config_clocks_time_properties_ds_table(&ns_config_clocks, "timePropertiesDs");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksFilterParameters> rw_config_clocks_filter_parameters_table(&ns_config_clocks, "filterParameters");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksServoParameters> rw_config_clocks_servo_parameters_table(&ns_config_clocks, "servoParameters");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksSlaveConfig> rw_config_clocks_slave_config_table(&ns_config_clocks, "slaveConfig");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksUnicastSlaveConfig> rw_config_clocks_unicast_slave_config_table(&ns_config_clocks, "unicastSlaveConfig");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksPortDS> rw_config_clocks_port_ds_table(&ns_config_clocks, "portDs");

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite<ConfigClocksVirtualPortConfig> rw_config_clocks_virtual_port_config(&ns_config_clocks, "virtualPortConfig");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksDefaultDS> r_status_clocks_default_ds(&ns_status_clocks, "defaultDs");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksCurrentDS> r_status_clocks_current_ds(&ns_status_clocks, "currentDs");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksParentDS> r_status_clocks_parent_ds(&ns_status_clocks, "parentDs");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksTimePropertiesDS> r_status_clocks_time_properties_ds(&ns_status_clocks, "timePropertiesDs");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksSlaveDS> r_status_clocks_slave_ds(&ns_status_clocks, "clocksSlaveDs");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksUnicastMasterTable> r_status_clocks_unicast_master_table(&ns_status_clocks_unicast, "master");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksUnicastSlaveTable> r_status_clocks_unicast_slave_table(&ns_status_clocks_unicast, "slave");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly<StatusClocksPortsDS> r_status_clocks_ports_ds(&ns_status_clocks, "portDs");

// Parent: vtss/ptp/control ----------------------------------------------------
static TableWriteOnly<ControlClocks> rw_control_clocks_table(&ns_control, "clocks");

// Parent: vtss/ptp/statistics -----------------------------------------------------
NS(ns_statistics_clocks, ns_statistics, "clocks");

// Parent: vtss/ptp/statistics/clocks ----------------------------------------------
static TableReadOnly<StatisticsClocksPortsDS> r_statistics_clocks_ports_ds(&ns_statistics_clocks, "portDS");

// Parent: vtss/ptp/statistics -----------------------------------------------------
NS(ns_statistics_cmlds, ns_statistics, "cmldsDS");

// Parent: vtss/ptp/statistics/cmlds ----------------------------------------------
static TableReadOnly<StatisticsCmldsPortDs> r_statistics_cmlds_ports_ds(&ns_statistics_cmlds, "cmldsPortDS");
