/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_ospf6_serializer.hxx"

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace expose::snmp;

/******************************************************************************/
/** Register module MIB resources                                             */
/******************************************************************************/
// For backward compatible, we still use VTSS_MODULE_ID_FRR(143) for the
// VTSS-OSPF6-MIB.mib
VTSS_MIB_MODULE("ospf6Mib", "OSPF6", frr_ospf6_mib_init, VTSS_MODULE_ID_FRR_OSPF6, root,
                h)
{
    /* MIB history */
    h.add_history_element("202008030000Z", "Initial version.");
    h.add_history_element("202402090000Z", "Add MtuIgnore");

    /* MIB description */
    h.description("This is a private version of the OSPF6 MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

/******************************************************************************/
/** Module MIB table                                                          */
/******************************************************************************/
/* Hierarchical overview
 *
 *  ospf6MibObjects
 *      .Ospf6Capabilities
 *      .Ospf6Config
 *          .Ospf6ConfigProcessTable
 *          .Ospf6ConfigRouterTable
 *          .Ospf6ConfigAreaTable
 *          .Ospf6ConfigAuthenticationTable
 *          .Ospf6ConfigAreaRangeTable
 *          .Ospf6ConfigVlinkTable
 *          .Ospf6ConfigVlinkMdKeyTable
 *          .Ospf6ConfigStubAreaTable
 *          .Ospf6ConfigInterfaceTable
 *          .Ospf6ConfigInterfaceMdKeyTable
 *      .Ospf6Status
 *          .Ospf6StatusRouterTable
 *          .Ospf6StatusAreaTable
 *          .Ospf6StatusInterfaceTable
 *          .Ospf6StatusNeighborIpv6Table
 *          .Ospf6StatusInterfaceMdKeyPrecedenceTable
 *          .Ospf6StatusVlinkMdKeyPrecedenceTable
 *          .Ospf6StatusRouteIpv6Table
 *          .Ospf6StatusDbTable
 *          .Ospf6StatusDbRouterTable
 *          .Ospf6StatusDbNetworkTable
 *          .Ospf6StatusDbInterAreaPrefixTable
 *          .Ospf6StatusDbInterAreaRouterTable
 *          .Ospf6StatusDbExternalTable
 *          .Ospf6StatusDbNSSAExternalTable
 *      .Ospf6Control
 *          .Ospf6ControlGlobals
 */
namespace vtss
{
namespace appl
{
namespace ospf6
{
namespace interfaces
{

// Root
NS(ospf6_mib_objects, root, 1, "ospf6MibObjects");

// Ospf6Capabilities
static StructRO2<Ospf6CapabilitiesTabular> ospf6_capabilities_tabular(
    &ospf6_mib_objects,
    vtss::expose::snmp::OidElement(1, "ospf6Capabilities"));

// Ospf6Config
NS(ospf6_config, ospf6_mib_objects, 2, "Ospf6Config");

// Ospf6ConfigInterface
NS(ospf6_config_intf, ospf6_config, 100, "Ospf6ConfigInterface");

// Ospf6Status
NS(ospf6_status, ospf6_mib_objects, 3, "Ospf6Status");

// Ospf6Control
NS(ospf6_control, ospf6_mib_objects, 4, "Ospf6Control");

// Ospf6ConfigProcessTable
static TableReadWriteAddDelete2<Ospf6ConfigProcessEntry> ospf6_config_process_table(
    &ospf6_config,
    vtss::expose::snmp::OidElement(1, "Ospf6ConfigProcessTable"),
    vtss::expose::snmp::OidElement(2, "Ospf6ConfigProcessTableRowEditor"));

// Ospf6ConfigRouterTable
static TableReadWrite2<Ospf6ConfigRouterEntry> ospf6_config_router_table(
    &ospf6_config,
    vtss::expose::snmp::OidElement(3, "Ospf6ConfigRouterTable"));

// Ospf6ConfigRouterInterfaceTable
static TableReadWrite2<Ospf6ConfigRouterInterfaceEntry> ospf6_config_router_interface_table(
    &ospf6_config,
    vtss::expose::snmp::OidElement(4, "Ospf6ConfigRouterInterfaceTable"));

// OspfConfigAreaRangeTable
static TableReadWriteAddDelete2<Ospf6ConfigAreaRangeEntry> ospf6_config_area_range_table(
    &ospf6_config,
    vtss::expose::snmp::OidElement(5, "Ospf6ConfigAreaRangeTable"),
    vtss::expose::snmp::OidElement(6,
                                   "Ospf6ConfigAreaRangeTableRowEditor"));

static TableReadWriteAddDelete2<Ospf6ConfigStubAreaEntry> ospf6_config_stub_area_table(
    &ospf6_config,
    vtss::expose::snmp::OidElement(7, "Ospf6ConfigStubAreaTable"),
    vtss::expose::snmp::OidElement(8, "Ospf6ConfigStubAreaTableRowEditor"));

static TableReadWrite2<Ospf6ConfigInterfaceEntry> ospf6_config_interface_table(
    &ospf6_config,
    vtss::expose::snmp::OidElement(9, "Ospf6ConfigInterfaceTable"));

// Ospf6StatusRouterTable
static TableReadOnly2<Ospf6StatusRouterEntry> ospf6_status_router_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(1, "Ospf6StatusRouterTable"));

// Ospf6StatusRouteAreaTable
static TableReadOnly2<Ospf6StatusAreaEntry> ospf6_status_area_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(2, "Ospf6StatusRouteAreaTable"));

// Ospf6StatusInterfaceTable
static TableReadOnly2<Ospf6StatusInterfaceEntry> ospf6_status_interface_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(5, "Ospf6StatusInterfaceTable"));

// Ospf6StatusNeighborIpv6Table
static TableReadOnly2<Ospf6StatusNeighborIpv6Entry> ospf6_status_neighbor_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(6, "Ospf6StatusNeighborIpv6Table"));

// Ospf6StatusRouteIpv6Table
static TableReadOnly2<Ospf6StatusRouteIpv6Entry> ospf6_status_route_ipv6_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(10, "Ospf6StatusRouteIpv6Table"));

// Ospf6StatusDbTable
static TableReadOnly2<Ospf6StatusDbEntry> ospf6_status_db_table(
    &ospf6_status, vtss::expose::snmp::OidElement(11, "Ospf6StatusDbTable"));

// Ospf6StatusDbRouterTable
static TableReadOnly2<Ospf6StatusDbRouterEntry> ospf6_status_db_router_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(12, "Ospf6StatusDbRouterTable"));

// Ospf6StatusDbNetworkTable
static TableReadOnly2<Ospf6StatusDbNetworkEntry> ospf6_status_db_network_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(13, "Ospf6StatusDbNetworkTable"));

// Ospf6StatusDbInterAreaPrefixTable
static TableReadOnly2<Ospf6StatusDbInterAreaPrefixEntry> ospf6_status_db_inter_area_prefix_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(14, "Ospf6StatusDbInterAreaPrefixTable"));

// Ospf6StatusDbInterAreaRouterTable
static TableReadOnly2<Ospf6StatusDbAsbrSummaryEntry> ospf6_status_db_inter_area_router_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(15, "Ospf6StatusDbInterAreaRouterTable"));

// Ospf6StatusDbExternalTable
static TableReadOnly2<Ospf6StatusDbExternalEntry> ospf6_status_db_external_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(16, "Ospf6StatusDbExternalTable"));

// Ospf6StatusDbLinkTable
static TableReadOnly2<Ospf6StatusDbLinkEntry> ospf6_status_db_link_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(17, "Ospf6StatusDbLinkTable"));

// Ospf6StatusDbintraAreaPrefixTable
static TableReadOnly2<Ospf6StatusDbLinkEntry> ospf6_status_db_intra_area_prefix_table(
    &ospf6_status,
    vtss::expose::snmp::OidElement(18, "Ospf6StatusDbintraAreaPrefixTable"));

// Ospf6ControlGlobals
static StructRW2<Ospf6ControlGlobalsTabular> ospf6_control_globals_tabular(
    &ospf6_control, vtss::expose::snmp::OidElement(4, "Ospf6ControlGlobals"));

}  // namespace interfaces
}  // namespace ospf6
}  // namespace appl
}  // namespace vtss

