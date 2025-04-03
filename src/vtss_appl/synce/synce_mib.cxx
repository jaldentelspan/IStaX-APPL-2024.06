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

#include "synce_serializer.hxx"

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::synce::interfaces;

VTSS_MIB_MODULE("synceMib", "SYNCE", vtss_synce_mib_init, VTSS_MODULE_ID_SYNCE, root, h) {
    h.add_history_element("201406240000Z", "Initial version");
    h.add_history_element("201602240000Z", "Updated MIB to indicate capability to nominate PTP sources in SyncE");
    h.add_history_element("201603170000Z", "Updated MIB to indicate capability to quality level in SyncE");
    h.add_history_element("201606160000Z", "Added synceLolAlarmState used in place of TruthValue in SynceStatusGlobalClockSelectionModeLol");
    h.add_history_element("202110210000Z", "Added dpllType and clockType to capabilities");
    h.add_history_element("202304270000Z", "Extended VTSSsynceDpllHwType with new values hwZL30731, hwZL30732, hwZL30733, hwZL30734, hwZL30735");
    h.description("Private SyncE MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

// Parent: vtss ----------------------------------------------------------------
NS(ns_synce, root, 1, "synceMibObjects");

// Parent: vtss/synce ----------------------------------------------------------
NS(ns_capabilities, ns_synce, 1, "synceCapabilities");
NS(ns_config, ns_synce, 2, "synceConfig");
NS(ns_status, ns_synce, 3, "synceStatus");
NS(ns_control, ns_synce, 4, "synceControl");

// Parent: vtss/synce/capabilities---------------------------------------------
static StructRO2<CapabilitiesGlobal> ro_capabilities_globals(
        &ns_capabilities, expose::snmp::OidElement(1, "synceCapabilitiesGlobal"));

// Parent: vtss/synce/config ---------------------------------------------------
NS(ns_config_global, ns_config, 1, "synceConfigGlobal");
NS(ns_config_sources, ns_config, 2, "synceConfigSources");
NS(ns_config_ports, ns_config, 3, "synceConfigPorts");

// Parent: vtss/synce/config/global --------------------------------------------
static StructRW2<ConfigGlobalClockSelectionMode>
        rw_config_global_clock_selection_mode(
                &ns_config_global,
                expose::snmp::OidElement(1, "synceConfigGlobalClockSelectionMode"));

// Parent: vtss/synce/config/global --------------------------------------------
static StructRW2<ConfigGlobalStationClocks> rw_config_global_station_clocks(
        &ns_config_global,
        expose::snmp::OidElement(2, "synceConfigGlobalStationClocks"));

// Parent: vtss/synce/config/sources -------------------------------------------
static TableReadWrite2<ConfigSourcesClockSourceNomination> rw_config_sources_clock_source_nomination(
                &ns_config_sources,
                OidElement(1, "synceConfigSourcesClockSourceNomination"));

// Parent: vtss/synce/config/ports ---------------------------------------------
static TableReadWrite2<ConfigPortsPortConfig> rw_config_ports_port_config(
        &ns_config_ports, OidElement(1, "synceConfigPortsPortConfig"));

// Parent: vtss/synce/status ---------------------------------------------------
NS(ns_status_global, ns_status, 1, "synceStatusGlobal");
NS(ns_status_sources, ns_status, 2, "synceStatusSources");
NS(ns_status_ports, ns_status, 3, "synceStatusPorts");
NS(ns_status_ptp, ns_status, 4, "synceStatusPtp");

// Parent: vtss/synce/status/global --------------------------------------------
static StructRO2<StatusGlobalClockSelectionMode> r_status_global_clock_selection_mode(
                &ns_status_global,
                OidElement(1, "synceStatusGlobalClockSelectionMode"));

// Parent: vtss/synce/status/sources -------------------------------------------
static TableReadOnly2<StatusSourcesClockSourceNomination> r_status_sources_clock_source_nomination(
                &ns_status_sources,
                OidElement(1, "synceStatusSourcesClockSourceNomination"));

// Parent: vtss/synce/status/ports ---------------------------------------------
static TableReadOnly2<StatusPortsPortStatus> r_status_ports_port_status(
        &ns_status_ports, OidElement(1, "synceStatusPortsPortStatus"));

// Parent: vtss/synce/status/ptp ---------------------------------------------
static TableReadOnly2<StatusPtpPortStatus> r_status_ptp_port_status(
        &ns_status_ptp, OidElement(1, "synceStatusPtpPortStatus"));

// Parent: vtss/synce/control
// ---------------------------------------------------
NS(ns_control_sources, ns_control, 1, "synceControlSources");

// Parent: vtss/synce/control/sources ------------------------------------------
static TableReadWrite2<ControlSourcesClockSourceNomination>
        rw_control_sources_clock_source_nomination(
                &ns_control_sources,
                OidElement(1, "synceControlSourcesClockSource"));

