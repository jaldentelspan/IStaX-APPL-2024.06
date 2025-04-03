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

#include "gvrp_serializer.hxx"
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::gvrp;


VTSS_MIB_MODULE("GvrpMib", "GVRP", vtss_gvrp_mib_init, VTSS_MODULE_ID_XXRP, root, h)
{
    h.add_history_element("201411110000Z", "Initial version");
    h.add_history_element("201510220000Z", "Fixed a typo and updated the description "
                          "of GVRP Max VLANs value.");
    h.add_history_element("201703130000Z", "Updated the description "
                          "of GVRP Max VLANs parameter.");
    h.description("Private GVRP MIB.");
}

// Parent: vtss ------------------------------------------------------------------
NS(gvrp_mib_objects, root, 1, "GvrpMibObjects");

// Parent: vtss/gvrp -------------------------------------------------------------
NS(gvrp_config,           gvrp_mib_objects, 2, "GvrpConfig");
NS(gvrp_config_interface, gvrp_config,      2, "GvrpConfigInterface");

// Parent: vtss/gvrp/config ------------------------------------------------------
static StructRW2<GvrpConfigGlobalsLeaf> gvrp_config_globals_leaf(
    &gvrp_config, expose::snmp::OidElement(1, "GvrpConfigGlobals"));

static TableReadWrite2<GvrpConfigInterfaceEntry> gvrp_config_interface_entry(
    &gvrp_config_interface, OidElement(1, "GvrpConfigInterfaceTable"));
