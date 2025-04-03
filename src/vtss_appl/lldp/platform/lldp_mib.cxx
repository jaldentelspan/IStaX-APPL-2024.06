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

#include "lldp_serializer.hxx"   // For serialized types
#include "vtss/appl/lldp.h"

using namespace vtss;
using namespace vtss::expose::snmp;

VTSS_MIB_MODULE("lldpMib", "LLDP", lldp_mib_init, VTSS_MODULE_ID_LLDP, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201604070000Z", "Add SnmpNotificationEna to lldpConfig table");
    h.description("This is a private version of the LLDP MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(lldp,            root,         1, "lldpMibObjects");
NS(lldp_config,     lldp,         2, "lldpConfig"); 
NS(lldp_status,     lldp,         3, "lldpStatus");
NS(lldp_control,    lldp,         4, "lldpControl");
NS(lldpmed_config,  lldp_config,  3, "lldpConfigMed");
NS(lldp_statistics, lldp_status,  1, "lldpStatusStatistics");
NS(lldpmed_status,  lldp_status,  5, "lldpStatusMed");
NS(lldp_stat_clear, lldp_control, 1, "lldpControlStatisticsClear");

namespace vtss {
namespace appl {
namespace lldp {
namespace interfaces {
static TableReadWrite2<LldpStatClearLeaf> lldp_stat_clear_leaf(&lldp_stat_clear, 
                                                                vtss::expose::snmp::OidElement(1, "lldpControlStatisticsClearTable"));

static TableReadWrite2<LldpPortConfLeaf> lldp_port_conf_leaf(&lldp_config, 
                                                             vtss::expose::snmp::OidElement(2, "lldpConfig"));

static TableReadWrite2<LldpMedPortConfLeaf> lldp_med_port_conf_leaf(&lldpmed_config, 
                                                                    vtss::expose::snmp::OidElement(1, "lldpConfigMed"));


static TableReadWriteAddDelete2<LldpMedPoliciesConfLeaf> lldp_med_policies_conf_leaf(
    &lldpmed_config, 
    vtss::expose::snmp::OidElement(2, "lldpConfigMedPolicy"),
    vtss::expose::snmp::OidElement(6, "lldpConfigMedPolicyRowEditor"));

static TableReadOnly2<LldpStatusNetworkPolicyLeaf> lldp_status_network_policy_leaf(&lldpmed_status, 
                                                                                   vtss::expose::snmp::OidElement(3, "lldpStatusMedRemoteDeviceNetworkPolicyInfo"));


static TableReadOnly2<LldpStatistics> lldp_statistics_leaf(&lldp_statistics, 
                                                           vtss::expose::snmp::OidElement(2, "lldpStatusStatisticsTable"));

static TableReadOnly2<LldpNeighbors> lldp_neighbors_leaf(&lldp_status, 
                                                         vtss::expose::snmp::OidElement(2, "lldpStatusNeighborsInformation"));


static TableReadOnly2<LldpNeighborsMgmt> lldp_neighbors_mgmt_leaf(&lldp_status, 
                                                                  vtss::expose::snmp::OidElement(3, "lldpStatusNeighborsMgmtInformation"));

static TableReadOnly2<LldpPreempt> lldp_preempt_leaf(&lldp_status,
                                                                  vtss::expose::snmp::OidElement(4, "lldpStatusPreemptInformation"));

static TableReadOnly2<LldpMedRemote> lldp_med_remote_leaf(&lldpmed_status, 
                                                           vtss::expose::snmp::OidElement(1, "lldpStatusMedRemoteDeviceInfo"));


static TableReadOnly2<LldpMedCivic> lldp_med_civic_leaf(&lldpmed_status, 
                                                        vtss::expose::snmp::OidElement(2, "lldpStatusMedRemoteDeviceLocInfo"));



static StructRW2<LllpStatGlobalClr> lldp_stat_global_clr_leaf(&lldp_stat_clear, 
                                                              vtss::expose::snmp::OidElement(2, "lldpControlStatisticsClearGlobal"));



static TableReadWrite2<LldpPoliciesListLeaf> lldp_policies_list_leaf(&lldpmed_config, 
                                                                    vtss::expose::snmp::OidElement(3, "lldpConfigMedPolicyList"));


static StructRW2<LllpConfigMedGlobal> lldp_med_config_global_leaf(&lldpmed_config, 
                                                                  vtss::expose::snmp::OidElement(4, "lldpConfigMedGlobal"));


static TableReadWrite2<LllpConfigMedLoc> lldp_med_config_loc_leaf(&lldpmed_config, 
                                                                  vtss::expose::snmp::OidElement(5, "lldpConfigMedLocationInformation"));

static StructRW2<LllpConfigGlobal> lldp_global_leaf(&lldp_config, 
                                                    vtss::expose::snmp::OidElement(1, "lldpConfigGlobal"));

static StructRO2<LllpGlobalStat> lldp_global_stat_leaf(&lldp_statistics, 
                                                        vtss::expose::snmp::OidElement(1, "lldpStatusStatisticsGlobalCounters"));
}  // namespace interfaces
}  // namespace topo
}  // namespace appl
}  // namespace vtss
