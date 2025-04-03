/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::ptp::interfaces;

VTSS_MIB_MODULE("ptpMib", "PTP", vtss_ptp_mib_init, VTSS_MODULE_ID_PTP, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201503260000Z", "Added attribute Dscp to PtpConfigClocksDefaultDs");
    h.add_history_element("201506030000Z", "Added MS-PDV parameters");
    h.add_history_element("201510280000Z", "Added parameters for specifying leap event.");
    h.add_history_element("201511100000Z", "Added virtual port parameters.");
    h.add_history_element("201604200000Z", "Removed vlanTagEnable parameter.");
    h.add_history_element("201604290000Z", "Added statistic parameters for PTP port.");
    h.add_history_element("201806270000Z", "Added 802.1AS Intervals.");
    h.add_history_element("201902120000Z", "Updated 802.1as CMLDS names,descriptions and reorganised CMLDS DefaultDS MIB objects.");
    h.add_history_element("201905080000Z", "Updated new protocol ethip4ip6.");
    h.add_history_element("202208010000Z", "Changed ptpFilterType and ptpDelayMechanism from an integer to an enumeration.");
    h.description("Private PTP MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

// Parent: vtss ----------------------------------------------------------------
NS(ns_ptp, root, 1, "ptpMibObjects");

// Parent: vtss/ptp ------------------------------------------------------------
NS(ns_capabilities, ns_ptp, 1, "ptpCapabilities");
NS(ns_config, ns_ptp, 2, "ptpConfig");
NS(ns_status, ns_ptp, 3, "ptpStatus");
NS(ns_control, ns_ptp, 4, "ptpControl");
NS(ns_statistics, ns_ptp, 5, "ptpStatistics");

// Parent: vtss/ptp/capabilities------------------------------------------------
static StructRO2<GlobalCapabilities> ro_capabilities_globals(
        &ns_capabilities, expose::snmp::OidElement(1, "ptpCapabilitiesGlobals"));

// Parent: vtss/ptp/config -----------------------------------------------------
NS(ns_config_globals, ns_config, 1, "ptpConfigGlobals");
NS(ns_config_clocks, ns_config, 2, "ptpConfigClocks");
NS(ns_config_cmlds, ns_config, 3, "ptpConfigCmlds");

// Parent: vtss/ptp/config/globals ---------------------------------------------
static StructRW2<GlobalsExternalClockMode> rw_config_globals_external_clock_mode(
        &ns_config_globals, expose::snmp::OidElement(1, "ptpConfigGlobalsExternalClockMode"));

// Parent: vtss/ptp/config/globals ---------------------------------------------
static StructRW2<GlobalsSystemTimeSyncMode> rw_config_globals_system_time_sync_mode(
        &ns_config_globals, expose::snmp::OidElement(2, "ptpConfigGlobalsSystemTimeSyncMode"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWriteAddDelete2<ConfigClocksDefaultDS> rw_config_clocks_default_ds_table(
        &ns_config_clocks, OidElement(1, "ptpConfigClocksDefaultDsTable"),
                           OidElement(2, "ptpConfigClocksDefaultDsTableRowEditor"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite2<ConfigClocksTimePropertiesDS> rw_config_clocks_time_properties_ds_table(
        &ns_config_clocks, OidElement(3, "ptpConfigClocksTimePropertiesDsTable"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite2<ConfigClocksFilterParameters> rw_config_clocks_filter_parameters_table(
        &ns_config_clocks, OidElement(4, "ptpConfigClocksFilterParametersTable"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite2<ConfigClocksServoParameters> rw_config_clocks_servo_parameters_table(
        &ns_config_clocks, OidElement(5, "ptpConfigClocksServoParametersTable"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite2<ConfigClocksSlaveConfig> rw_config_clocks_slave_config_table(
        &ns_config_clocks, OidElement(6, "ptpConfigClocksSlaveConfigTable"));

// Parent: vtss/ptp/config/clocks -------------------------------------------
static TableReadWrite2<ConfigClocksUnicastSlaveConfig> rw_config_clocks_unicast_slave_config_table(
        &ns_config_clocks, OidElement(7, "ptpConfigClocksUnicastSlaveConfigTable"));

// Parent: vtss/ptp/config/clocks -------------------------------------------
static TableReadWrite2<ConfigClocksPortDS> rw_config_clocks_port_ds_table(
        &ns_config_clocks, OidElement(8, "ptpConfigClocksPortDsTable"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite2<ConfigClocksVirtualPortConfig> rw_config_clocks_virtual_port_config_table(
        &ns_config_clocks, OidElement(9, "ptpConfigClocksVirtualPortCfgTable"));

// Parent: vtss/ptp/config/cmlds ----------------------------------------------
static TableReadWrite2<ConfigCmldsPortDs> rw_config_cmlds_Default_Port_Ds_table(&ns_config_cmlds, OidElement(1, "ptpConfigCmldsPortDsTable"));

// Parent: vtss/ptp/status -----------------------------------------------------
NS(ns_status_clocks, ns_status, 1, "ptpStatusClocks");
NS(ns_status_cmlds, ns_status, 2, "ptpStatusCmlds");

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksDefaultDS> r_status_clocks_default_ds(
        &ns_status_clocks, OidElement(1, "ptpStatusClocksDefaultDsTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksCurrentDS> r_status_clocks_current_ds(
        &ns_status_clocks, OidElement(2, "ptpStatusClocksCurrentDsTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksParentDS> r_status_clocks_parent_ds(
        &ns_status_clocks, OidElement(3, "ptpStatusClocksParentDsTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksTimePropertiesDS> r_status_clocks_time_properties_ds(
        &ns_status_clocks, OidElement(4, "ptpStatusClocksTimePropertiesDsTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksSlaveDS> r_status_clocks_slave_ds(
        &ns_status_clocks, OidElement(5, "ptpStatusClocksSlaveDsTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksUnicastMasterTable> r_status_clocks_unicast_master_table(
        &ns_status_clocks, OidElement(6, "ptpStatusClocksUnicastMasterTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksUnicastSlaveTable> r_status_clocks_unicast_slave_table(
        &ns_status_clocks, OidElement(7, "ptpStatusClocksUnicastSlaveTable"));

// Parent: vtss/ptp/status/clocks ----------------------------------------------
static TableReadOnly2<StatusClocksPortsDS> r_status_clocks_ports_ds(
        &ns_status_clocks, OidElement(8, "ptpStatusClocksPortsDsTable"));

// Parent: vtss/ptp/status/cmlds ----------------------------------------------
static TableReadOnly2<StatusCmldsPortDs> r_status_cmlds_port_ds(&ns_status_cmlds, OidElement(1, "ptpStatusCmldsPortDsTable"));

// Parent: vtss/ptp/status/cmlds ----------------------------------------------
static StructRO2<StatusCmldsDefaultDs> r_status_cmlds_Default_Ds_table(&ns_status_cmlds, OidElement(2, "ptpStatusCmldsDefaultDsTable"));

// Parent: vtss/ptp/config/clocks ----------------------------------------------
static TableReadWrite2<ControlClocks> rw_control_clocks_table(
        &ns_control, OidElement(1, "ptpControlClocksTable"));

// Parent: vtss/ptp/statistics -----------------------------------------------------
NS(ns_statistics_clocks, ns_statistics, 1, "ptpStatisticsClocks");

// Parent: vtss/ptp/statistics/clocks ----------------------------------------------
static TableReadOnly2<StatisticsClocksPortsDS> r_statistics_clocks_ports_ds(&ns_statistics_clocks, OidElement(1, "ptpStatisticsClocksPortsDsTable"));

// Parent: vtss/ptp/statistics -----------------------------------------------------
NS(ns_statistics_cmlds, ns_statistics, 2, "ptpStatisticsCmlds");

// Parent: vtss/ptp/statistics/cmlds ----------------------------------------------
static TableReadOnly2<StatisticsCmldsPortDs> r_statistics_cmlds_ports_ds(&ns_statistics_cmlds, OidElement(1, "ptpStatisticsCmldsPortDsTable"));
