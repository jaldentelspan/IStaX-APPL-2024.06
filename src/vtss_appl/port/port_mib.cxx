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

#include "port_serializer.hxx"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("portMib", "PORT", port_mib_init, VTSS_MODULE_ID_PORT, root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201507070000Z", "Port speed is moved into the TC MIB");
    h.add_history_element("201611280000Z", "Add 2.5G auto-negotiate capability");
    h.add_history_element("201701270000Z", "Added 802.3br statistics");
    h.add_history_element("201711270000Z", "Changed PortConfigAdvertiseDisabled to Unsigned32");
    h.add_history_element("201808240000Z", "Add capabilities.");
    h.add_history_element("201911290000Z", "Added more media types in VTSSPortMedia enumeration.");
    h.add_history_element("202011160000Z", "Added SFPDateCode to SFP info.");
    h.add_history_element("202101060000Z", "Added more capabilities, ForceClause73, and FECMode.");
    h.add_history_element("202103120000Z", "Added link up/down counters to port status.");
    h.description("This is a private version of the PORT MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "portMibObjects");
NS(portConfig, objects, 2, "portConfig");
NS(port_status, objects, 3, "portStatus");
NS(port_control, objects, 4, "portControl");
NS(port_statistics, objects, 5, "portStatistics");
NS(port_veriphy_result, port_status, 3, "portStatusVeriPhyResult");
NS(port_stat_clear, port_control, 1, "portControlStatisticsClear");
#ifdef VTSS_UI_OPT_VERIPHY
NS(port_veriphy_start_node, port_control, 2, "portControlVeriPhyStart");
#endif

namespace vtss
{
namespace appl
{
namespace port
{
namespace interfaces
{
static StructRO2<PortCapabilities> port_capabilities(
    &objects, vtss::expose::snmp::OidElement(1, "portCapabilities"));

static TableReadWrite2<PortParams> port_params_table(
    &portConfig, OidElement(1, "portConfig"));

static TableReadOnly2<PortStatusRmonStatistics>
port_status_rmon_statistics_table(
    &port_statistics,
    OidElement(1, "portStatisticsRmonStatisticsTable"));

static TableReadOnly2<PortStatusIfGroupStatistics>
port_status_if_group_statistics_table(
    &port_statistics,
    OidElement(2, "portStatisticsIfGroupStatisticsTable"));

static TableReadOnly2<PortStatusEthernetLikeStatstics>
port_status_ethernet_like_statstics_table(
    &port_statistics,
    OidElement(3, "portStatisticsEthernetLikeStatisticsTable"));

static TableReadOnly2<PortStatusQueuesStatistics>
port_status_queues_statistics_table(
    &port_statistics,
    OidElement(4, "portStatisticsQueuesStatisticsTable"));

static TableReadOnly2<PortStatusBridgeStatistics>
port_status_bridge_statistics_table(
    &port_statistics,
    OidElement(5, "portStatisticsBridgeStatisticsTable"));

static TableReadOnly2<PortStatusDot3brStatistics>
port_status_dot3br_statistics_table(
    &port_statistics,
    OidElement(6, "portStatisticsDot3brStatisticsTable"));

static TableReadOnly2<PortStatusParams> port_status_params_table(
    &port_status, OidElement(1, "portStatusInformationTable"));

static TableReadWrite2<PortStatsClear> port_stats_clear_table(
    &port_stat_clear, OidElement(1, "portControlStatisticsClearTable"));

#ifdef VTSS_UI_OPT_VERIPHY
static TableReadWrite2<PortVerifyStart> port_verify_start_table(
    &port_veriphy_start_node, OidElement(1, "portControlVeriPhyStartTable"));

static TableReadOnly2<PortVeriphyResult> port_veriphy_result_table(
    &port_veriphy_result, OidElement(1, "portStatusVeriPhyResultTable"));
#endif  // VTSS_UI_OPT_VERIPHY

}  // namespace interfaces
}  // namespace port
}  // namespace appl
}  // namespace vtss

