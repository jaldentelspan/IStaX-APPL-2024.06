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

#include "ipmc_serializer.hxx"
#include <vtss/basics/expose/json.hxx>

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::ipmc::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_ipmc_snp("ipmcSnooping");
extern "C" void ipmc_json_init()
{
    json_node_add(&ns_ipmc_snp);
}

/*
    ipmc-snooping
        => config.
              => global.
                    => igmp.
                    => mld.
              => interface.
                    => igmp.
                           => port.
                           => vlan.
                    => mld.
                           => port.
                           => vlan.
        => status.
              => interface.
                    => igmp.
                           => router-port.
                           => group-address.
                           => group-src-list
                    => mld.
                           => router-port.
                           => group-address.
                           => group-src-list
        => statistics.
              => global.
                    => group-address-count.
              => interface.
                    => igmp.
                           => vlan.
                    => mld.
                           => vlan.
        => control.
              => interfaces.
                    => igmp.
                           => statistics.clear.
                    => mld.
                           => statistics.clear.
*/

//IGMP - Config
NS(ns_conf,                ns_ipmc_snp,       "config");
NS(ns_conf_global,         ns_conf,           "global");
NS(ns_conf_interface,      ns_conf,           "interface");
NS(ns_conf_interface_igmp, ns_conf_interface, "igmp");

static StructReadWrite<IpmcIgmpGlobalsConfig> ipmc_snooping_config_global_igmp(
    &ns_conf_global, "igmp");
static TableReadWrite<IpmcIgmpPortConfigTable> ipmc_snooping_config_interface_igmp_port(
    &ns_conf_interface_igmp, "port");
static TableReadWrite<IpmcIgmpVlanConfigTable> ipmc_snooping_config_interface_igmp_vlan(
    &ns_conf_interface_igmp, "vlan");

//IGMP - Status
NS(ns_status,                ns_ipmc_snp, "status");
NS(ns_status_interface,      ns_status,   "interface");
NS(ns_status_interface_igmp, ns_status,   "igmp");

static TableReadOnly<IpmcIgmpPortStatusTable> ipmc_snooping_status_interface_igmp_port(
    &ns_status_interface_igmp, "routerPort");
static TableReadOnly<IpmcIgmpGrpTable> ipmc_snooping_status_interface_igmp_grpadrs(
    &ns_status_interface_igmp, "groupAddress");
#ifdef VTSS_SW_OPTION_SMB_IPMC
static TableReadOnly<IpmcIgmpSrcTable> ipmc_snooping_status_interface_igmp_srclist(
    &ns_status_interface_igmp, "groupSrcList");
#endif /* VTSS_SW_OPTION_SMB_IPMC */

//IPMC - Statistics
NS(ns_statistics,        ns_ipmc_snp,   "statistics");
NS(ns_statistics_global, ns_statistics, "global");

//IGMP - Statistics
NS(ns_statistics_interface,      ns_statistics,           "interface");
NS(ns_statistics_interface_igmp, ns_statistics_interface, "igmp");
static TableReadOnly<IpmcIgmpVlanStatusTable> ipmc_snooping_statistics_interface_igmp_vlan(
    &ns_statistics_interface_igmp, "vlan");

//IGMP - Control
NS(ns_control,                           ns_ipmc_snp,               "control");
NS(ns_control_interface,                 ns_control,                "interface");
NS(ns_control_interface_igmp,            ns_control_interface,      "igmp");
NS(ns_control_interface_igmp_statistics, ns_control_interface_igmp, "statistics");
static StructWriteOnly<IpmcIgmpStatisticsClear> ipmc_snooping_control_interface_igmp_statistics(
    &ns_control_interface_igmp_statistics, "clear");

#ifdef VTSS_SW_OPTION_SMB_IPMC
//MLD - Config
NS(ns_conf_interface_mld, ns_conf_interface, "mld");
static StructReadWrite<IpmcMldGlobalsConfig> ipmc_snooping_config_global_mld(
    &ns_conf_global, "mld");
static TableReadWrite<IpmcMldPortConfigTable> ipmc_snooping_config_interface_mld_port(
    &ns_conf_interface_mld, "port");
static TableReadWrite<IpmcMldVlanConfigTable> ipmc_snooping_config_interface_mld_vlan(
    &ns_conf_interface_mld, "vlan");

//MLD - Status
NS(ns_status_interface_mld, ns_status, "mld");
static TableReadOnly<IpmcMldPortStatusTable> ipmc_snooping_status_interface_mld_port(
    &ns_status_interface_mld, "routerPort");
static TableReadOnly<IpmcMldGrpTable> ipmc_snooping_status_interface_mld_grpadrs(
    &ns_status_interface_mld, "groupAddress");
static TableReadOnly<IpmcMldSrcTable> ipmc_snooping_status_interface_mld_srclist(
    &ns_status_interface_mld, "groupSrcList");

//MLD - Statistics
NS(ns_statistics_interface_mld, ns_statistics_interface, "mld");
static TableReadOnly<IpmcMldVlanStatusTable> ipmc_snooping_statistics_interface_mld_vlan(
    &ns_statistics_interface_mld, "vlan");

//MLD - Control
NS(ns_control_interface_mld,            ns_control_interface,     "mld");
NS(ns_control_interface_mld_statistics, ns_control_interface_mld, "statistics");
static StructWriteOnly<IpmcMldStatisticsClear> ipmc_snooping_control_interface_mld_statistics(
    &ns_control_interface_mld_statistics, "clear");
#endif /* VTSS_SW_OPTION_SMB_IPMC */

