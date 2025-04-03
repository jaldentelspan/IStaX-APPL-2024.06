/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "lacp_serializer.hxx"
#include "vtss/basics/expose/json.hxx"


using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::lacp::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_lacp("lacp");
extern "C" void vtss_appl_lacp_json_init() { json_node_add(&ns_lacp); }

NS(ns_conf,       ns_lacp, "config");
NS(ns_status,     ns_lacp, "status");
NS(ns_ctrl,       ns_lacp, "control");
NS(ns_statistics, ns_lacp, "statistics");

namespace vtss {
namespace appl {
namespace lacp {
namespace interfaces {
static StructReadWrite<LacpGlobalParamsLeaf>    lacp_global_params_leaf(&ns_conf, "global");
static TableReadWrite<LacpPortConfTable>        lacp_port_conf_table(&ns_conf, "interface");
static TableReadWrite<LacpGroupConfTable>       lacp_group_conf_table(&ns_conf, "group");
static TableReadOnly<LacpSystemStatusTable>     lacp_system_status_table(&ns_status, "system");
static TableReadOnly<LacpPortStatusTable>       lacp_port_status_table(&ns_status, "interface");
static TableReadOnly<LacpPortStatsTable>        lacp_port_stats_table(&ns_statistics, "interface");
static TableWriteOnly<LacpPortStatsClearTable>  lacp_port_stats_clear_table(&ns_ctrl, "interface");
}  // namespace interfaces
}  // namespace lacp
}  // namespace appl
}  // namespace vtss

