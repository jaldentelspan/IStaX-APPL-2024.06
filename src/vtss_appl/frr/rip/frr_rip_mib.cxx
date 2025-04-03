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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_rip_serializer.hxx"

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace expose::snmp;

/******************************************************************************/
/** Register module MIB resources                                             */
/******************************************************************************/
VTSS_MIB_MODULE("ripMib", "RIP", frr_rip_mib_init, VTSS_MODULE_ID_FRR_RIP, root,
                h)
{
    /* MIB history */
    h.add_history_element("201806110000Z", "Initial version.");

    /* MIB description */
    h.description("This is a private version of the RIP MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

/******************************************************************************/
/** Module MIB table                                                          */
/******************************************************************************/
/* Hierarchical overview
 *
 *  ripMibObjects
 *      .RipCapabilities
 *      .RipConfig
 *          .RipConfigRouterTable
 *          .RipConfigNetworkTable
 *          .RipConfigNetworkTableRowEditor
 *          .RipConfigRouterInterfaceTable
 *          .RipConfigNeighborTable
*           .RipConfigNeighborTableRowEditor
 *          .RipConfigInterfaceTable
 *          .RipConfigOffsetListTable
 *          .RipConfigOffsetListTableRowEditor
 *      .RipStatus
 *          .RipStatusInterfaceTable
 *          .RipStatusDbTable
 *          .RipStatusPeerTable
 *          .RipStatusAccessListTable
 *      .RipControl
 *          .RipControlGlobals
 */
namespace vtss
{
namespace appl
{
namespace rip
{
namespace interfaces
{

// Root
NS(rip_mib_objects, root, 1, "ripMibObjects");

// RipCapabilities
static StructRO2<RipCapabilitiesTabular> rip_capabilities_tabular(
    &rip_mib_objects, vtss::expose::snmp::OidElement(1, "ripCapabilities"));

// RipConfig
NS(rip_config, rip_mib_objects, 2, "RipConfig");

// RipStatus
NS(rip_status, rip_mib_objects, 3, "RipStatus");

// RipControl
NS(rip_control, rip_mib_objects, 4, "RipControl");

// RipConfigRouterTable
static StructRW2<RipConfigRouterTabular> rip_config_router_tabular(
    &rip_config, vtss::expose::snmp::OidElement(1, "RipConfigRouterTable"));

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
// RipConfigNetworkTable
static TableReadWriteAddDelete2<RipConfigNetworkEntry> rip_config_network_table(
    &rip_config, vtss::expose::snmp::OidElement(2, "RipConfigNetworkTable"),
    vtss::expose::snmp::OidElement(3, "RipConfigNetworkTableRowEditor"));

//----------------------------------------------------------------------------
//** RIP router interface configuration
//----------------------------------------------------------------------------
// RipConfigRouterInterfaceTable
static TableReadWrite2<RipConfigRouterInterfaceEntry> rip_config_router_interface_table(
    &rip_config,
    vtss::expose::snmp::OidElement(4, "RipConfigRouterInterfaceTable"));

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
static TableReadWriteAddDelete2<RipConfigNeighborEntry> rip_config_neighbor_table(
    &rip_config,
    vtss::expose::snmp::OidElement(5, "RipConfigNeighborTable"),
    vtss::expose::snmp::OidElement(6, "RipConfigNeighborTableRowEditor"));

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
static TableReadWrite2<RipConfigInterfaceEntry> rip_config_interface_table(
    &rip_config,
    vtss::expose::snmp::OidElement(7, "RipConfigInterfaceTable"));

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
static TableReadWriteAddDelete2<RipConfigOffsetListEntry> router_config_offset_list_table(
    &rip_config,
    vtss::expose::snmp::OidElement(8, "RipConfigOffsetListTable"),
    vtss::expose::snmp::OidElement(9, "RipConfigOffsetListTableRowEditor"));

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
static StructRO2<RipStatusGeneralEntry> rip_general_status_table(
    &rip_status,
    vtss::expose::snmp::OidElement(1, "RipStatusGeneralTable"));

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
static TableReadOnly2<RipStatusInterfaceEntry> rip_interface_status_table(
    &rip_status,
    vtss::expose::snmp::OidElement(2, "RipStatusInterfaceTable"));

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
static TableReadOnly2<RipStatusDbEntry> rip_status_db_table(
    &rip_status, vtss::expose::snmp::OidElement(3, "RipStatusDbTable"));

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
static TableReadOnly2<RipStatusPeerEntry> rip_status_peer_table(
    &rip_status, vtss::expose::snmp::OidElement(4, "RipStatusPeerTable"));

// RipControlGlobals
static StructRW2<RIpControlGlobalsTabular> rip_control_globals_tabular(
    &rip_control, vtss::expose::snmp::OidElement(1, "RipControlGlobals"));

}  // namespace interfaces
}  // namespace rip
}  // namespace appl
}  // namespace vtss

