/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::synce::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_synce("synce");
extern "C" void vtss_appl_synce_json_init() { json_node_add(&ns_synce); }

// Parent: vtss/synce ----------------------------------------------------------
NS(ns_config, ns_synce, "config");
NS(ns_status, ns_synce, "status");
NS(ns_control, ns_synce, "control");
NS(ns_statistics, ns_synce, "statistics");

// Parent: vtss/synce/config -----------------------------------------------------
NS(ns_config_global, ns_config, "global");
NS(ns_config_sources, ns_config, "sources");
NS(ns_config_ports, ns_config, "ports");

// Parent: vtss/synce/status ---------------------------------------------------
NS(ns_status_global, ns_status, "global");
NS(ns_status_sources, ns_status, "sources");
NS(ns_status_ports, ns_status, "ports");
NS(ns_status_ptp, ns_status, "synceStatusPtp");

// Parent: vtss/synce/control
// ---------------------------------------------------
NS(ns_control_sources, ns_control, "sources");

// Parent: vtss/synce/capabilities---------------------------------------------
static StructReadOnly<CapabilitiesGlobal> ro_capabilities_globals(&ns_synce, "capabilities");

// Parent: vtss/synce/config/global --------------------------------------------
static StructReadWrite<ConfigGlobalClockSelectionMode> rw_config_global_clock_selection_mode(&ns_config_global, "clockSelectionMode");

// Parent: vtss/synce/config/global --------------------------------------------
static StructReadWrite<ConfigGlobalStationClocks> rw_config_global_station_clocks(&ns_config_global, "stationClocks");

// Parent: vtss/synce/config/sources -------------------------------------------
static TableReadWrite<ConfigSourcesClockSourceNomination> rw_config_sources_clock_source_nomination(&ns_config_sources, "clockSourceNomination");

// Parent: vtss/synce/config/ports ---------------------------------------------
static TableReadWrite<ConfigPortsPortConfig> rw_config_ports_port_config(&ns_config_ports, "portConfig");

// Parent: vtss/synce/status/global --------------------------------------------
static StructReadOnly<StatusGlobalClockSelectionMode> r_status_global_clock_selection_mode(&ns_status_global, "clockSelectionMode");

// Parent: vtss/synce/status---------------------------------------------
static TableReadOnly<PossibleSources> ro_capabilities_sources(&ns_status, "possibleSources");

// Parent: vtss/synce/status/sources -------------------------------------------
static TableReadOnly<StatusSourcesClockSourceNomination> r_status_sources_clock_source_nomination(&ns_status_sources, "clockSourceNomination");

// Parent: vtss/synce/status/ptp ---------------------------------------------
static TableReadOnly<StatusPtpPortStatus> r_status_ptp_port_status(
        &ns_status_ptp, "synceStatusPtpPortStatus");

// Parent: vtss/synce/status/ports ---------------------------------------------
static TableReadOnly<StatusPortsPortStatus> r_(&ns_status_ports, "portStatus");

// Parent: vtss/synce/control/sources ------------------------------------------
static TableReadWrite<ControlSourcesClockSourceNomination> rw_control_sources_clock_source_nomination(&ns_control_sources, "clockSource");
