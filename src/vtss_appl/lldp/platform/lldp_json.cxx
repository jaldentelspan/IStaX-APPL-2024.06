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

#include "json_rpc_api.hxx"
#include "lldp_serializer.hxx"

using namespace vtss::json;
using namespace vtss::expose::json;

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_lldp("lldp");
void vtss_appl_lldp_json_init() { vtss::json_node_add(&ns_lldp); }

NS(lldp_config,     ns_lldp,      "config"); 
NS(lldp_status,     ns_lldp,      "status");
NS(lldp_control,    ns_lldp,      "control");
NS(lldp_statistics, ns_lldp,      "statistics");
NS(lldpmed_config,  lldp_config,  "med");
NS(lldpmed_status,  lldp_status,  "med");
NS(lldp_control_stat, lldp_control, "statistics");
NS(lldp_control_stat_interface, lldp_control_stat, "interface");
NS(lldp_control_stat_global, lldp_control_stat, "global");

namespace vtss {
namespace appl {
namespace lldp {
namespace interfaces {

TableWriteOnly<LldpStatClearLeaf> lldp_stat_clear_leaf(&lldp_control_stat_interface, "clear");

TableReadWrite<LldpPortConfLeaf> lldp_port_conf_leaf(&lldp_config, "interface");

TableReadWrite<LldpMedPortConfLeaf> lldp_med_port_conf_leaf(&lldpmed_config, "interface");
 
TableReadWriteAddDelete<LldpMedPoliciesConfLeaf> lldp_med_policies_conf_leaf(&lldpmed_config, "policy");

TableReadOnly<LldpStatusNetworkPolicyLeaf> lldp_status_network_policy_leaf(&lldpmed_status, "remoteDeviceNetworkPolicy");

TableReadWrite<LldpPoliciesListLeaf> lldp_policies_list_leaf(&lldpmed_config, "policyList");

TableReadOnly<LldpStatistics> lldp_statistics_leaf(&lldp_statistics, "interfaces");

TableReadOnly<LldpNeighbors> lldp_neighbors_leaf(&lldp_status, "neighbors");

TableReadOnly<LldpNeighborsMgmt> lldp_neighbors_mgmt_leaf(&lldp_status, "neighborsMgmt");

TableReadOnly<LldpMedRemote> lldp_med_remote_leaf(&lldpmed_status, "remoteDevice");

TableReadOnly<LldpMedCivic> lldp_med_civic_leaf(&lldpmed_status, "remoteDeviceLoc");

StructWriteOnly<LllpStatGlobalClr> lldp_stat_global_clr_leaf(&lldp_control_stat_global, "clear");

StructReadWrite<LllpConfigMedGlobal> lldp_med_config_global_leaf(&lldpmed_config, "global");

TableReadWrite<LllpConfigMedLoc> lldp_med_config_loc_leaf(&lldpmed_config, "location");

StructReadWrite<LllpConfigGlobal> lldp_global_leaf(&lldp_config, "global");

StructReadOnly<LllpGlobalStat> lldp_global_stat_leaf(&lldp_statistics, "global");
}  // namespace interfaces
}  // namespace port
}  // namespace appl
}  // namespace vtss

