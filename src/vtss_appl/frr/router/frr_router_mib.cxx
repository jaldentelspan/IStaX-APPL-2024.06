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
#include "frr_router_serializer.hxx"

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace expose::snmp;

/******************************************************************************/
/** Register module MIB resources                                             */
/******************************************************************************/
VTSS_MIB_MODULE("routerMib", "ROUTER", frr_router_mib_init,
                VTSS_MODULE_ID_FRR_ROUTER, root, h)
{
    /* MIB history */
    h.add_history_element("201807270000Z", "Initial version.");

    /* MIB description */
    h.description("This is a private version of the ROUTER MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

/******************************************************************************/
/** Module MIB table                                                          */
/******************************************************************************/
/* Hierarchical overview
 *
 *  RouterMibObjects
 *      .RouterCapabilities
 *      .RouterConfig
 *          .RouterConfigKeyChainTable
 *          .RouterConfigKeyChainTableRowEditor
 *          .RouterConfigAccessListTable
 *          .RouterConfigAccessListTableRowEditor
 *      .RouterStatus
 *          .RouterStatusKeyChainTable
 *          .RouterStatusAccessListTable
 */
namespace vtss
{
namespace appl
{
namespace router
{
namespace interfaces
{

// Root
NS(router_mib_objects, root, 1, "routerMibObjects");

// RouterCapabilities
static StructRO2<RouterCapabilitiesTabular> router_capabilities_tabular(
    &router_mib_objects,
    vtss::expose::snmp::OidElement(1, "routerCapabilities"));

// RipConfig
NS(router_config, router_mib_objects, 2, "RouterConfig");

// RipStatus
NS(router_status, router_mib_objects, 3, "RouterStatus");

//----------------------------------------------------------------------------
//** Key-chain: Configuration
//----------------------------------------------------------------------------
// .RouterConfigKeyChainTable
// .RouterConfigKeyChainTableRowEditor
static TableReadWriteAddDelete2<RouterConfigKeyChainNameEntry> router_config_key_chain_table(
    &router_config,
    vtss::expose::snmp::OidElement(1, "RouterConfigKeyChainTable"),
    vtss::expose::snmp::OidElement(2,
                                   "RouterConfigKeyChainTableRowEditor"));

static TableReadWriteAddDelete2<RouterConfigKeyChainKeyConfEntry>
router_config_key_chain_key_id_table(
    &router_config, vtss::expose::snmp::OidElement(
        3, "RouterConfigKeyChainKeyIdTable"),
    vtss::expose::snmp::OidElement(
        4, "RouterConfigKeyChainKeyIdTableRowEditor"));

//----------------------------------------------------------------------------
//** Access-list: Configuration
//----------------------------------------------------------------------------
// .RouterConfigAccessListTable
// .RouterConfigAccessListTableRowEditor
static TableReadWriteAddDelete2<RouterConfigAccessListEntry> router_config_access_list_table(
    &router_config,
    vtss::expose::snmp::OidElement(5, "RouterConfigAccessListTable"),
    vtss::expose::snmp::OidElement(6,
                                   "RouterConfigAccessListTableRowEditor"));

//----------------------------------------------------------------------------
//** Key-chain: Status
//----------------------------------------------------------------------------
// .RouterStatusKeyChainTable
// Reserved

//----------------------------------------------------------------------------
//** Access-list: Status
//----------------------------------------------------------------------------
// .RouterStatusAccessListTable
static TableReadOnly2<RouterStatusAccessListEntry> router_status_access_list_table(
    &router_status,
    vtss::expose::snmp::OidElement(2, "RouterStatusAccessListTable"));

}  // namespace interfaces
}  // namespace router
}  // namespace appl
}  // namespace vtss

