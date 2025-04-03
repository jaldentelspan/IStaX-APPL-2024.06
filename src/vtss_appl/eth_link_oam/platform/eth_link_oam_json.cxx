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

#include "eth_link_oam_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::eth_link_oam::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_eth_link_oam("ethernetLinkOam");
extern "C" void vtss_appl_eth_link_oam_json_init() { json_node_add(&ns_eth_link_oam); }

NS(ns_capability, ns_eth_link_oam, "capabilities");
NS(ns_conf,       ns_eth_link_oam, "config");
NS(ns_status,     ns_eth_link_oam, "status");
NS(ns_statistics, ns_eth_link_oam, "statistics");
NS(ns_ctrl,       ns_eth_link_oam, "control");

NS(ns_conf_if,       ns_conf,       "interface");
NS(ns_status_if,     ns_status,     "interface");
NS(ns_statistics_if, ns_statistics, "interface");

NS(ns_stat_clear,             ns_ctrl,       "statistics");
NS(ns_stat_control_interface, ns_stat_clear, "interface");

namespace vtss {
namespace appl {
namespace eth_link_oam {
namespace interfaces   {
//ethernet-link-oam.capabilities.interface
static TableReadOnly<ethLinkOamInterfaceCapabilitiesEntry> eth_link_oam_capabilities(
        &ns_capability, "interface");

// ethernet_link_oam.config.interface
static TableReadWriteNoNS<ethLinkOamInterfaceConfigEntry> eth_link_oam_interface_config_entry(
        &ns_conf_if);

//ethernet_link_oam.config.interface.event
static TableReadWrite<ethLinkOamInterfaceEventConfigEntry> eth_link_oam_interface_event_config_entry(
        &ns_conf_if, "event");

//ethernet_link_oam.config.interface.loopback-test
static TableReadWrite<ethLinkOamInterfaceLoopbackTestConfigEntry> eth_link_oam_interface_loopback_config_entry(
        &ns_conf_if, "loopbackTest");

//ethernet_link_oam.status.interface
static TableReadOnlyNoNS<ethLinkOamInterfaceStatusEntry> eth_link_oam_interface_status_entry(
        &ns_status_if);

//ethernet_link_oam.status.interface.peer
static TableReadOnly<ethLinkOamInterfacePeerStatusEntry> eth_link_oam_interface_status_peer_entry(
        &ns_status_if, "peer");

//ethernet_link_oam.status.interface.link-event
static TableReadOnly<ethLinkOamInterfaceLinkEventStatusEntry> eth_link_oam_interface_status_link_event_entry(
        &ns_status_if, "linkEvent");

//ethernet_link_oam.status.interface.peer-link-event
static TableReadOnly<ethLinkOamInterfacePeerLinkEventStatusEntry> 
    eth_link_oam_interface_status_peer_link_event_entry(&ns_status_if, "peerLinkEvent");

//ethernet_link_oam.statistics.interface
static TableReadOnlyNoNS<ethLinkOamInterfaceStatisticsEntry> eth_link_oam_interface_stats_entry(
        &ns_statistics_if);

//ethernet_link_oam.statistics.interface.critical-link-event.update
TableReadOnlyNotification<ethLinkOamInterfaceCriticalLinkEventStatisticsEntry>
    eth_link_oam_interface_stats_critical_link_event_update(&ns_statistics_if, 
            "criticalLinkEvent", &eth_link_oam_crit_event_update);

//ethernet_link_oam.control.statistics.interface.clear
TableWriteOnly<ethLinkOamInterfaceStatsClearEntry> eth_link_oam_interface_stats_clear(
        &ns_stat_control_interface, "clear");

}  // namespace interfaces
}  // namespace eth_link_oam
}  // namespace appl
}  // namespace vtss
