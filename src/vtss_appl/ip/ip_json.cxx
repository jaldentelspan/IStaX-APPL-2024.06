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

#include "ip_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
using namespace vtss;
using namespace vtss::expose::json;
using namespace vtss::appl::ip::interfaces;

static NamespaceNode ns_ip("ip");
void vtss_appl_ip_json_init()
{
    json_node_add(&ns_ip);
}

namespace
{

template <class T>
using TRON = TableReadOnlyNotification<T>;

template <class T>
using SRON = StructReadOnlyNotification<T>;

// Parent: vtss/ip -------------------------------------------------------------
NS(ns_config,     ns_ip, "config");
NS(ns_status,     ns_ip, "status");
NS(ns_control,    ns_ip, "control");
NS(ns_statistics, ns_ip, "statistics");

// Parent: vtss/ip/config ------------------------------------------------------
NS(ns_conf_globals,    ns_config, "global");
NS(ns_conf_interfaces, ns_config, "interface");
NS(ns_conf_routes,     ns_config, "route");

// Parent: vtss/ip/status ------------------------------------------------------
NS(ns_st_glb, ns_status, "global");
NS(ns_st_nb,  ns_status, "neighbor");
NS(ns_st_if,  ns_status, "interface");
NS(ns_st_rt,  ns_status, "route");
NS(ns_st_acd, ns_status, "acd");

// Parent: vtss/ip/statistics --------------------------------------------------
NS(ns_stat_global, ns_statistics, "global");
NS(ns_stat_interfaces, ns_statistics, "interface");

// Parent: vtss/ip/control -----------------------------------------------------
NS(ns_ctrl_interfaces, ns_control, "interface");

// Parent: vtss/ip -------------------------------------------------------------
StructReadOnly<GlobalCapabilities> cap(&ns_ip, "capabilities");

// Parent: vtss/ip/config/globals ----------------------------------------------
StructReadWrite<GlobalsParam> rw_global_param(&ns_conf_globals, "main");

// Parent: vtss/ip/config/interfaces -------------------------------------------
TableReadWriteAddDeleteNoNS<IfTable> if_table(&ns_conf_interfaces);
TableReadWrite<Ipv4Table> ipv4_table(&ns_conf_interfaces, "ipv4");
TableReadWrite<Ipv6Table> ipv6_table(&ns_conf_interfaces, "ipv6");

// Parent: vtss/ip/config/routes -----------------------------------------------
TableReadWriteAddDelete<RouteIpv4> route_ipv4(&ns_conf_routes, "ipv4");
TableReadWriteAddDelete<RouteIpv6> route_ipv6(&ns_conf_routes, "ipv6");

// Parent: vtss/ip/status/global -----------------------------------------------
SRON<StatusGlobalNotification> st_globals_notif(&ns_st_glb, "notification", &ip_global_notification_status);

// Parent: vtss/ip/status/neighbor ---------------------------------------------
TableReadOnly<StatusGlobalsIpv4Nb> st_neighbor_ipv4_nb(&ns_st_nb, "ipv4");
TableReadOnly<StatusGlobalIpv6Nb>  st_neighbor_ipv6_nb(&ns_st_nb, "ipv6");

// Parent: vtss/ip/status/interface --------------------------------------------
TRON<StatusIfLink_>  _if_link( &ns_st_if, "link",       &status_if_link);
TRON<StatusIfIpv4_>  _if_ipv4( &ns_st_if, "ipv4",       &status_if_ipv4);
TRON<StatusIfDhcpv4> _if_dhcp_(&ns_st_if, "dhcpClient", &status_if_dhcp4c);
TRON<StatusIfIpv6_>  _if_ipv6_(&ns_st_if, "ipv6",       &status_if_ipv6);

// Parent: vtss/ip/status/route -----------------------------------------------
TableReadOnly<StatusRtIpv4_> status_rt_ipv4_(&ns_st_rt, "ipv4");
TableReadOnly<StatusRtIpv6_> status_rt_ipv6_(&ns_st_rt, "ipv6");

// Parent: vtss/ip/status/acd --------------------------------------------------
TRON<StatusAcdIpv4_> st_acd_ipv4(&ns_st_acd, "ipv4", &status_acd_ipv4);

// Parent: vtss/ip/statistics/globals ------------------------------------------
StructReadOnly<StatGlobal4> stat_global4(&ns_stat_global, "ipv4");
StructReadOnly<StatGlobal6> stat_global6(&ns_stat_global, "ipv6");

// Parent: vtss/ip/statistics/interface ----------------------------------------
TableReadOnly<StatIfLink> stat_if_link(&ns_stat_interfaces, "link");
TableReadOnly<StatIfIpv4> stat_if_ipv4(&ns_stat_interfaces, "ipv4");
TableReadOnly<StatIfIpv6> stat_if_ipv6(&ns_stat_interfaces, "ipv6");

// Parent: vtss/ip/control -----------------------------------------------------
StructReadWrite<ControlGlobals> control_globals(&ns_control, "global");

// Parent: vtss/ip/control/interface -------------------------------------------
TableReadWrite<CtrlDhcpc> ctrl_dhcpc(&ns_ctrl_interfaces, "dhcpClient");

}  // namespace
