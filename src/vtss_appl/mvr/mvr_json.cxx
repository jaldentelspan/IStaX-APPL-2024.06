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

#include "mvr_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::mvr::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_mvr("mvr");
extern "C" void mvr_json_init(void)
{
    json_node_add(&ns_mvr);
}

NS(ns_config,             ns_mvr,           "config");
NS(ns_status,             ns_mvr,           "status");
NS(ns_control,            ns_mvr,           "control");

NS(ns_conf_interface,     ns_config,        "interface");

NS(ns_status_igmp,        ns_status,        "igmp");
NS(ns_status_igmp_group,  ns_status_igmp,   "group");

NS(ns_status_mld,         ns_status,        "mld");
NS(ns_status_mld_group,   ns_status_mld,    "group");

NS(ns_control_statistics, ns_control,       "statistics");

namespace vtss
{
namespace appl
{
namespace mvr
{
namespace interfaces
{

// mvr.config.globals
static StructReadWrite<MvrGlobalsConfig> mvr_globals_config(
    &ns_config, "global");

// mvr.config.interface.port
static TableReadWrite<MvrPortConfigTable> mvr_port_config_table(
    &ns_conf_interface, "port");

// mvr.config.interface.vlan
static TableReadWriteAddDelete<MvrVlanConfigTable> mvr_vlan_config_table(
    &ns_conf_interface, "vlan");

// mvr.config.vlan-port
static TableReadWrite<MvrVlanPortConfigTable> mvr_vlan_port_config_table(
    &ns_config, "vlanPort");

// mvr.status.igmp.vlan
static TableReadOnly<MvrIgmpVlanStatusTable> mvr_igmp_vlan_status_table(
    &ns_status_igmp, "vlan");

// mvr.status.igmp.group.address
static TableReadOnly<MvrIgmpGrpTable> mvr_igmp_grpadrs_table(
    &ns_status_igmp_group, "address");

// mvr.status.igmp.group.src-list
static TableReadOnly<MvrIgmpSrcTable> mvr_igmp_srclist_table(
    &ns_status_igmp_group, "srcList");

// mvr.status.mdl.vlan
static TableReadOnly<MvrMldVlanStatusTable> mvr_mld_vlan_status_table(
    &ns_status_mld, "vlan");

// mvr.status.mdl.group.address
static TableReadOnly<MvrMldGrpTable> mvr_mld_grpadrs_table(
    &ns_status_mld_group, "address");

// mvr.status.mdl.group.src-list
static TableReadOnly<MvrMldSrcTable> mvr_mld_srclist_table(
    &ns_status_mld_group, "srcList");

// mvr.control.statistics
static StructWriteOnly<MvrStatisticsClear> mvr_control_clear_statistics(
    &ns_control_statistics, "clear");

}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

