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
#include "frr_ospf_serializer.hxx"

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace expose::snmp;

/******************************************************************************/
/** Register module MIB resources                                             */
/******************************************************************************/
// For backward compatible, we still use VTSS_MODULE_ID_FRR(143) for the
// VTSS-OSPF-MIB.mib
VTSS_MIB_MODULE("ospfMib", "OSPF", frr_ospf_mib_init, VTSS_MODULE_ID_FRR_OSPF, root,
                h)
{
    /* MIB history */
    h.add_history_element("201711110000Z", "Initial version.");
    h.add_history_element("201805140000Z",
                          "Add stub router, NSSA, default route redistribution "
                          "and passive interface support.");
    h.add_history_element("201805160000Z", "Update OSPF capabilities.");
    h.add_history_element("201807090000Z",
                          "Add RIP redistributed, database support, and "
                          "correct the key length from 160 to 128 "
                          "according to the cryptography.");
    h.add_history_element("202402090000Z", "Add MtuIgnore");

    /* MIB description */
    h.description("This is a private version of the OSPF MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

/******************************************************************************/
/** Module MIB table                                                          */
/******************************************************************************/
/* Hierarchical overview
 *
 *  ospfMibObjects
 *      .OspfCapabilities
 *      .OspfConfig
 *          .OspfConfigProcessTable
 *          .OspfConfigRouterTable
 *          .OspfConfigAreaTable
 *          .OspfConfigAuthenticationTable
 *          .OspfConfigAreaRangeTable
 *          .OspfConfigVlinkTable
 *          .OspfConfigVlinkMdKeyTable
 *          .OspfConfigStubAreaTable
 *          .OspfConfigInterfaceTable
 *          .OspfConfigInterfaceMdKeyTable
 *      .OspfStatus
 *          .OspfStatusRouterTable
 *          .OspfStatusAreaTable
 *          .OspfStatusInterfaceTable
 *          .OspfStatusNeighborIpv4Table
 *          .OspfStatusInterfaceMdKeyPrecedenceTable
 *          .OspfStatusVlinkMdKeyPrecedenceTable
 *          .OspfStatusRouteIpv4Table
 *          .OspfStatusDbTable
 *          .OspfStatusDbRouterTable
 *          .OspfStatusDbNetworkTable
 *          .OspfStatusDbSummaryTable
 *          .OspfStatusDbASBRSummaryTable
 *          .OspfStatusDbExternalTable
 *          .OspfStatusDbNSSAExternalTable
 *      .OspfControl
 *          .OspfControlGlobals
 */
namespace vtss
{
namespace appl
{
namespace ospf
{
namespace interfaces
{

// Root
NS(ospf_mib_objects, root, 1, "ospfMibObjects");

// OspfCapabilities
static StructRO2<OspfCapabilitiesTabular> ospf_capabilities_tabular(
    &ospf_mib_objects,
    vtss::expose::snmp::OidElement(1, "ospfCapabilities"));

// OspfConfig
NS(ospf_config, ospf_mib_objects, 2, "OspfConfig");

// OspfConfigInterface
NS(ospf_config_intf, ospf_config, 100, "OspfConfigInterface");

// OspfStatus
NS(ospf_status, ospf_mib_objects, 3, "OspfStatus");

// OspfControl
NS(ospf_control, ospf_mib_objects, 4, "OspfControl");

// OspfConfigProcessTable
static TableReadWriteAddDelete2<OspfConfigProcessEntry> ospf_config_process_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(1, "OspfConfigProcessTable"),
    vtss::expose::snmp::OidElement(2, "OspfConfigProcessTableRowEditor"));

// OspfConfigRouterTable
static TableReadWrite2<OspfConfigRouterEntry> ospf_config_router_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(3, "OspfConfigRouterTable"));

// OspfConfigRouterInterfaceTable
static TableReadWrite2<OspfConfigRouterInterfaceEntry> ospf_config_router_interface_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(4, "OspfConfigRouterInterfaceTable"));

// OspfConfigAreaTable
static TableReadWriteAddDelete2<OspfConfigAreaEntry> ospf_config_network_area_table(
    &ospf_config, vtss::expose::snmp::OidElement(5, "OspfConfigAreaTable"),
    vtss::expose::snmp::OidElement(6, "OspfConfigAreaTableRowEditor"));

// OspfConfigAreaAuthTable
static TableReadWriteAddDelete2<OspfConfigAreaAuthEntry> ospf_config_area_auth_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(7, "OspfConfigAreaAuthTable"),
    vtss::expose::snmp::OidElement(8, "OspfConfigAreaAuthTableRowEditor"));

// OspfConfigAreaRangeTable
static TableReadWriteAddDelete2<OspfConfigAreaRangeEntry> ospf_config_area_range_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(9, "OspfConfigAreaRangeTable"),
    vtss::expose::snmp::OidElement(10,
                                   "OspfConfigAreaRangeTableRowEditor"));

// OspfConfigInterfaceMessageDigestKeyTable
static TableReadWriteAddDelete2<OspfConfigInterfaceAuthMdKeyEntry>
ospf_config_intf_auth_md_key_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(11,
                                   "OspfConfigInterfaceMdKeyTable"),
    vtss::expose::snmp::OidElement(
        12, "OspfConfigInterfaceMdKeyTableRowEditor"));

static TableReadWriteAddDelete2<OspfConfigVlinkEntry> ospf_config_vlink_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(13, "OspfConfigVlinkTable"),
    vtss::expose::snmp::OidElement(14, "OspfConfigVlinkTableRowEditor"));

static TableReadWriteAddDelete2<OspfConfigVlinkMdKeyEntry> ospf_config_vlink_md_key_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(15, "OspfConfigVlinkMdKeyTable"),
    vtss::expose::snmp::OidElement(16,
                                   "OspfConfigVlinkTableMdKeyRowEditor"));

static TableReadWriteAddDelete2<OspfConfigStubAreaEntry> ospf_config_stub_area_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(17, "OspfConfigStubAreaTable"),
    vtss::expose::snmp::OidElement(18, "OspfConfigStubAreaTableRowEditor"));

static TableReadWrite2<OspfConfigInterfaceEntry> ospf_config_interface_table(
    &ospf_config,
    vtss::expose::snmp::OidElement(19, "OspfConfigInterfaceTable"));

// OspfStatusRouterTable
static TableReadOnly2<OspfStatusRouterEntry> ospf_status_router_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(1, "OspfStatusRouterTable"));

// OspfStatusRouteAreaTable
static TableReadOnly2<OspfStatusAreaEntry> ospf_status_area_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(2, "OspfStatusRouteAreaTable"));

// OspfStatusInterfaceTable
static TableReadOnly2<OspfStatusInterfaceEntry> ospf_status_interface_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(5, "OspfStatusInterfaceTable"));

// OspfStatusNeighborIpv4Table
static TableReadOnly2<OspfStatusNeighborIpv4Entry> ospf_status_neighbor_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(6, "OspfStatusNeighborIpv4Table"));

// OspfStatusInterfaceMdKeyPrecedenceTable
static TableReadOnly2<OspfStatusInterfaceMdKeyPrecedenceEntry>
ospf_status_interface_md_key_precedence_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(
        8, "OspfStatusInterfaceMdKeyPrecedenceTable"));

// OspfStatusVlinkMdkeyPrecedenceTable
static TableReadOnly2<OspfStatusVlinkMdKeyPrecedenceEntry>
ospf_status_vlink_md_key_precedence_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(
        9, "OspfStatusVlinkMdkeyPrecedenceTable"));

// OspfStatusRouteIpv4Table
static TableReadOnly2<OspfStatusRouteIpv4Entry> ospf_status_route_ipv4_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(10, "OspfStatusRouteIpv4Table"));

// OspfStatusDbTable
static TableReadOnly2<OspfStatusDbEntry> ospf_status_db_table(
    &ospf_status, vtss::expose::snmp::OidElement(11, "OspfStatusDbTable"));

// OspfStatusDbRouterTable
static TableReadOnly2<OspfStatusDbRouterEntry> ospf_status_db_router_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(12, "OspfStatusDbRouterTable"));

// OspfStatusDbNetworkTable
static TableReadOnly2<OspfStatusDbNetworkEntry> ospf_status_db_network_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(13, "OspfStatusDbNetworkTable"));

// OspfStatusDbSummaryTable
static TableReadOnly2<OspfStatusDbSummaryEntry> ospf_status_db_summary_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(14, "OspfStatusDbSummaryTable"));

// OspfStatusDbASBRSummaryTable
static TableReadOnly2<OspfStatusDbAsbrSummaryEntry> ospf_status_db_asbr_summary_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(15, "OspfStatusDbASBRSummaryTable"));

// OspfStatusDbExternalTable
static TableReadOnly2<OspfStatusDbExternalEntry> ospf_status_db_external_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(16, "OspfStatusDbExternalTable"));

// OspfStatusDbNSSAExternalTable
static TableReadOnly2<OspfStatusDbNssaExternalEntry> ospf_status_db_nssa_external_table(
    &ospf_status,
    vtss::expose::snmp::OidElement(17, "OspfStatusDbNSSAExternalTable"));

// OspfControlGlobals
static StructRW2<OspfControlGlobalsTabular> ospf_control_globals_tabular(
    &ospf_control, vtss::expose::snmp::OidElement(4, "OspfControlGlobals"));

}  // namespace interfaces
}  // namespace ospf
}  // namespace appl
}  // namespace vtss

