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

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::ip::interfaces;
VTSS_MIB_MODULE("ipMib", "IP", vtss_ip_mib_init, VTSS_MODULE_ID_IP, root, h)
{
    h.add_history_element("201407010000Z", "Initial version.");
    h.add_history_element(
        "201409110000Z",
        "Revise definition of IPv4/IPv6 interface status table index.");
    h.add_history_element("201410210000Z",
                          "Added arpCheck in the DhcpClientState enum");
    h.add_history_element(
        "201410290000Z",
        "Removed the fields arpRetransmitTime and reasmMaxSize.");
    h.add_history_element(
        "201508240000Z",
        "Add capability for IPv4/IPv6 statistics availability.");
    h.add_history_element(
        "201607280000Z",
        "Add ability to clear address conflict detection table.");
    h.add_history_element(
        "201705240000Z",
        "Add DHCP client hostname and client identifier configuration.");
    h.add_history_element(
        "201801300000Z",
        "Add IPv4 route distance configuration and routing information base table.");
    h.add_history_element(
        "201805290000Z",
        "Replace 'protoConneted(1)' with 'protoConnected(1)'.");
    h.add_history_element(
        "202003050000Z",
        "Non-backwards compatible changes due to (re-)introduction of dynamic routing.");
    h.add_history_element(
        "202006250000Z",
        "Non-backwards compatible change in ipStatusRoutesIpv4 and ipStatusRoutesIpv6 to make snmp-walk work for multiple link-local IPv6 addresses.");
    h.add_history_element(
        "202011060000Z",
        "Added global status trap to indicate H/W routing table depletion.");
    h.description("Private IP MIB.");
}

// Parent: vtss ----------------------------------------------------------------
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(ns_ip, root, 1, "ipMibObjects");

// Parent: vtss/ip -------------------------------------------------------------
NS(ns_config,     ns_ip, 2, "ipConfig");
NS(ns_status,     ns_ip, 3, "ipStatus");
NS(ns_control,    ns_ip, 4, "ipControl");
NS(ns_statistics, ns_ip, 5, "ipStatistics");
NS(ns_traps,      ns_ip, 6, "ipTrap");

// Parent: vtss/ip/config ------------------------------------------------------
NS(ns_conf_globals,    ns_config, 1, "ipConfigGlobals");
NS(ns_conf_interfaces, ns_config, 2, "ipConfigInterfaces");
NS(ns_conf_routes,     ns_config, 3, "ipConfigRoutes");

// Parent: vtss/ip/status ------------------------------------------------------
NS(ns_st_globals,    ns_status, 1, "ipStatusGlobals");
NS(ns_st_interfaces, ns_status, 2, "ipStatusInterfaces");
NS(ns_st_routes,     ns_status, 3, "ipStatusRoutes");

// Parent: vtss/ip/statistics --------------------------------------------------
NS(ns_stat_global,     ns_statistics, 1, "ipStatisticsGlobals");
NS(ns_stat_interfaces, ns_statistics, 2, "ipStatisticsInterfaces");

// Parent: vtss/ip/control -----------------------------------------------------
NS(ns_ctrl_interfaces, ns_control, 2, "ipControlInterface");

// Parent: vtss/ip -------------------------------------------------------------
static StructRO2<GlobalCapabilities> ro_global_capabilities(
    &ns_ip, OidElement(1, "ipCapabilities"));

// Parent: vtss/ip/config/globals ----------------------------------------------
static StructRW2<GlobalsParam> rw_global_param(&ns_conf_globals, OidElement(1, "ipConfigGlobalsMain"));

// Parent: vtss/ip/config/interfaces -------------------------------------------
static TableReadWriteAddDelete2<IfTable> if_table(
    &ns_conf_interfaces, OidElement(1, "ipConfigInterfacesTable"),
    OidElement(2, "ipConfigInterfacesTableRowEditor"));

static TableReadWrite2<Ipv4Table> ipv4_table(
    &ns_conf_interfaces, OidElement(3, "ipConfigInterfacesIpv4Table"));

static TableReadWrite2<Ipv6Table> ipv6_table(
    &ns_conf_interfaces, OidElement(4, "ipConfigInterfacesIpv6Table"));

// Parent: vtss/ip/config/routes -----------------------------------------------
static TableReadWriteAddDelete2<RouteIpv4> route_ipv4(
    &ns_conf_routes, OidElement(1, "ipConfigRoutesIpv4Table"),
    OidElement(2, "ipConfigRoutesIpv4RowEditor"));

static TableReadWriteAddDelete2<RouteIpv6> route_ipv6(
    &ns_conf_routes, OidElement(3, "ipConfigRoutesIpv6Table"),
    OidElement(4, "ipConfigRoutesIpv6RowEditor"));

// Parent: vtss/ip/status/globals ----------------------------------------------
static TableReadOnly2<StatusGlobalsIpv4Nb> status_globals_ipv4_nb(
    &ns_st_globals, OidElement(1, "ipStatusGlobalsIpv4Neighbor"));

static TableReadOnly2<StatusGlobalIpv6Nb> status_global_ipv6_nb(
    &ns_st_globals, OidElement(2, "ipStatusGlobalsIpv6Neighbor"));

// Parent: vtss/ip/status/interface --------------------------------------------
static TableReadOnlyTrap<StatusIfLink_> _if_link(
    &ns_st_interfaces, OidElement(1, "ipStatusInterfacesLink"),
    &status_if_link, &ns_traps, "ipTrapInterfacesLink",
    OidElement(1, "ipTrapInterfacesLinkAdd"),
    OidElement(2, "ipTrapInterfacesLinkMod"),
    OidElement(3, "ipTrapInterfacesLinkDel"));

static StructRoTrap<StatusGlobalNotification> status_global_notification(
    &ns_st_globals, OidElement(3, "ipStatusGlobalsNotification"),                        // Where it ends up in the ip::status::globals tree
    &ns_traps,      OidElement(4, "ipTrapGlobalsMain"), &ip_global_notification_status); // Where it ends up in the ip::trap tree

static TableReadOnly2<StatusIfIpv4_> _if_ipv4(
    &ns_st_interfaces, OidElement(2, "ipStatusInterfacesIpv4"));

static TableReadOnly2<StatusIfDhcpv4> _if_dhcpv4(
    &ns_st_interfaces, OidElement(3, "ipStatusInterfacesDhcpClientV4"));

static TableReadOnly2<StatusIfIpv6_> _if_ipv6(
    &ns_st_interfaces, OidElement(4, "ipStatusInterfacesIpv6"));

// Parent: vtss/ip/status/statistics/globals -----------------------------------
static StructRO2<StatGlobal4> stat_global4(
    &ns_stat_global, OidElement(1, "ipStatisticsGlobalsIpv4"));

static StructRO2<StatGlobal6> stat_global6(
    &ns_stat_global, OidElement(2, "ipStatisticsGlobalsIpv6"));

// Parent: vtss/ip/status/statistics/interface ---------------------------------
static TableReadOnly2<StatIfLink> stat_if_link(
    &ns_stat_interfaces, OidElement(1, "ipStatisticsInterfacesLink"));

static TableReadOnly2<StatIfIpv4> stat_if_ipv4(
    &ns_stat_interfaces, OidElement(2, "ipStatisticsInterfacesIpv4"));

static TableReadOnly2<StatIfIpv6> stat_if_ipv6(
    &ns_stat_interfaces, OidElement(3, "ipStatisticsInterfacesIpv6"));

// Parent: vtss/ip/status/routes -----------------------------------------------
static TableReadOnly2<StatusRtIpv4_> _rt_ipv4(&ns_st_routes, OidElement(1, "ipStatusRoutesIpv4"));
static TableReadOnly2<StatusRtIpv6_> _rt_ipv6(&ns_st_routes, OidElement(2, "ipStatusRoutesIpv6"));

// Parent: vtss/ip/control -----------------------------------------------------
static StructRW2<ControlGlobals> control_globals(
    &ns_control, OidElement(1, "ipControlGlobals"));

// Parent: vtss/ip/control/interface -------------------------------------------
static TableReadWrite2<CtrlDhcpc> ctrl_dhcpc(&ns_ctrl_interfaces, OidElement(1, "ipControlInterfaceDhcpClient"));

