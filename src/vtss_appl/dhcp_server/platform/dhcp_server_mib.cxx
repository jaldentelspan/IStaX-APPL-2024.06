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

#include "dhcp_server_serializer.hxx"

VTSS_MIB_MODULE("dhcpServerMib", "DHCP-SERVER", dhcp_server_mib_init,
                VTSS_MODULE_ID_DHCP_SERVER, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201411270000Z", "revise descriptions by Palle");
    h.add_history_element("201508240000Z", "revise fqdn as name");
    h.add_history_element("201903110000Z", "add support for dhcp server per port");
    h.description("This is a private version of DhcpServer");
}


using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace dhcp_server {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(dhcp_server_mib_objects, root, 1, "dhcpServerMibObjects");;
NS(dhcp_server_config, dhcp_server_mib_objects, 2, "dhcpServerConfig");;
NS(dhcp_server_status, dhcp_server_mib_objects, 3, "dhcpServerStatus");;
NS(dhcp_server_control, dhcp_server_mib_objects, 4, "dhcpServerControl");;

static StructRW2<DhcpServerConfigGlobalsLeaf> dhcp_server_config_globals_leaf(
        &dhcp_server_config, vtss::expose::snmp::OidElement(1, "dhcpServerConfigGlobals"));

static TableReadWrite2<DhcpServerConfigVlanEntry> dhcp_server_config_vlan_entry(
        &dhcp_server_config, vtss::expose::snmp::OidElement(2, "dhcpServerConfigVlanTable"));

static TableReadWriteAddDelete2<DhcpServerConfigExcludedEntry> dhcp_server_config_excluded_entry(
        &dhcp_server_config, vtss::expose::snmp::OidElement(3, "dhcpServerConfigExcludedTable"), vtss::expose::snmp::OidElement(4, "dhcpServerConfigExcludedIpTableRowEditor"));

static TableReadWriteAddDelete2<DhcpServerConfigPoolEntry> dhcp_server_config_pool_entry(
        &dhcp_server_config, vtss::expose::snmp::OidElement(5, "dhcpServerConfigPoolTable"), vtss::expose::snmp::OidElement(6, "dhcpServerConfigPoolTableRowEditor"));

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
static TableReadWriteAddDelete2<DhcpServerConfigReservedEntry> dhcp_server_config_reserved_entry(
        &dhcp_server_config, vtss::expose::snmp::OidElement(7, "dhcpServerConfigReservedTable"), vtss::expose::snmp::OidElement(8, "dhcpServerConfigReservedIpTableRowEditor"));
#endif

static TableReadOnly2<DhcpServerStatusDeclinedEntry> dhcp_server_status_declined_entry(
        &dhcp_server_status, vtss::expose::snmp::OidElement(1, "dhcpServerStatusDeclinedTable"));

static StructRO2<DhcpServerStatusStatisticsLeaf> dhcp_server_status_statistics_leaf(
        &dhcp_server_status, vtss::expose::snmp::OidElement(2, "dhcpServerStatusStatistics"));

static TableReadOnly2<DhcpServerStatusBindingEntry> dhcp_server_status_binding_entry(
        &dhcp_server_status, vtss::expose::snmp::OidElement(3, "dhcpServerStatusBindingTable"));

static StructRW2<DhcpServerControlStatisticsLeaf> dhcp_server_control_statistics_leaf(
        &dhcp_server_control, vtss::expose::snmp::OidElement(1, "dhcpServerControlStatistics"));

static StructRW2<DhcpServerControlBindingLeaf> dhcp_server_control_binding_leaf(
        &dhcp_server_control, vtss::expose::snmp::OidElement(2, "dhcpServerControlBinding"));
}  // namespace interfaces
}  // namespace dhcp_server
}  // namespace appl
}  // namespace vtss

