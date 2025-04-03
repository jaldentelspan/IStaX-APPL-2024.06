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

//#include "xxrp_api.h"
#include "xxrp_serializer.hxx"

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::xxrp::interfaces;

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

#ifdef VTSS_SW_OPTION_MRP
namespace vtss {
	namespace mrp {
VTSS_MIB_MODULE("MrpMib", "MRP", vtss_appl_mrp_mib_init, VTSS_MODULE_ID_MRP, root, h)
{
    h.add_history_element("201510190000Z", "Initial version");
    h.description("Private MRP MIB.");
}

NS(ns_mrp, root, 1, "MrpMibObjects");
static StructRO2<MrpCapabilities>
    mrp_capabilities(&ns_mrp, vtss::expose::snmp::OidElement(1, "MrpCapabilities"));

NS(ns_mrp_conf, ns_mrp, 2, "MrpConfig");
NS(ns_mrp_if_conf, ns_mrp_conf, 1, "MrpConfigInterface");
static TableReadWrite2<MrpInterfaceConfigurationTable>
    mrp_if_conf_table(&ns_mrp_if_conf, OidElement(1, "MrpConfigInterfaceTable"));
}
}
#endif /* VTSS_SW_OPTION_MRP */

#ifdef VTSS_SW_OPTION_MVRP
namespace vtss {
	namespace mvrp {
VTSS_MIB_MODULE("MvrpMib", "MVRP", vtss_appl_mvrp_mib_init, VTSS_MODULE_ID_MVRP, root, h)
{
    h.add_history_element("201510190000Z", "Initial version");
    h.description("Private MVRP MIB.");
}

NS(ns_mvrp, root, 1, "MvrpMibObjects");

NS(ns_mvrp_conf, ns_mvrp, 2, "MvrpConfig");
static StructRW2<MvrpGlobalsLeaf>
    mvrp_globals_conf_leaf(&ns_mvrp_conf, vtss::expose::snmp::OidElement(1, "MvrpConfigGlobals"));

NS(ns_mvrp_if_conf, ns_mvrp_conf, 2, "MvrpConfigInterface");
static TableReadWrite2<MvrpInterfaceConfigurationTable>
    mvrp_if_conf_table(&ns_mvrp_if_conf, OidElement(1, "MvrpConfigInterfaceTable"));

NS(ns_mvrp_statistics, ns_mvrp, 3, "MvrpStatus");
static TableReadOnly2<MvrpInterfaceStatisticsTable>
    mvrp_if_stat_table(&ns_mvrp_statistics, OidElement(1, "MvrpStatusInterfaceTable"));
}
}
#endif /* VTSS_SW_OPTION_MVRP */
