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

#include "json_rpc_api.hxx"
#include "port_serializer.hxx"

using namespace vtss::json;
using namespace vtss::expose::json;

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_port("port");
extern "C" void vtss_appl_port_json_init(void)
{
    vtss::json_node_add(&ns_port);
}

NS(ns_config, ns_port, "config");
NS(ns_status, ns_port, "status");
NS(ns_st_vp, ns_status, "veriphy");
NS(ns_ctrl, ns_port, "control");
NS(ns_stat_clear, ns_ctrl, "statistics");
#ifdef VTSS_UI_OPT_VERIPHY
NS(ns_ctrl_vp, ns_ctrl, "veriphy");
#endif
NS(ns_stat, ns_port, "statistics");

namespace vtss
{
namespace appl
{
namespace port
{
namespace interfaces
{

TableReadOnlyNotificationNoNS<PortStatusParams> status_params_update(&ns_status, &port_status_update);
StructReadOnly<PortCapabilities> port_capabilities(&ns_port, "capabilities");
TableReadOnly<PortNameMap> portname_map(&ns_port, "namemap");
TableReadWriteNoNS<PortParams> params(&ns_config);
TableReadOnly<PortStatusRmonStatistics> stat_rmon( &ns_stat, "rmon");
TableReadOnly<PortStatusIfGroupStatistics> stat_if_group(&ns_stat, "ifGroup");
TableReadOnly<PortStatusEthernetLikeStatstics> stat_ethernet_like(&ns_stat, "ethernetLike");
TableReadOnly<PortStatusBridgeStatistics> stat_bridge(&ns_stat, "bridge");
TableReadOnly<PortStatusDot3brStatistics> stat_dot3br(&ns_stat, "dot3br");
TableReadOnly<PortStatusQueuesStatistics> stat_queues(&ns_stat, "queues");
//TableReadOnlyNoNS<PortStatusParams> status_params(&ns_status);
TableWriteOnly<PortStatsClear> stats_clear(&ns_stat_clear, "clear");
#ifdef VTSS_UI_OPT_VERIPHY
TableWriteOnly<PortVerifyStart> verify_start(&ns_ctrl_vp, "start");
TableReadOnly<PortVeriphyResult> veriphy_result(&ns_st_vp, "result");
#endif  // VTSS_UI_OPT_VERIPHY

}  // namespace interfaces
}  // namespace port
}  // namespace appl
}  // namespace vtss

