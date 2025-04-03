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

#include "xxrp_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::xxrp::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);

#ifdef VTSS_SW_OPTION_MRP
static NamespaceNode ns_mrp("mrp");
extern "C" void vtss_appl_mrp_json_init()
{
	json_node_add(&ns_mrp);
}

static StructReadOnly<MrpCapabilities>
    mrp_capabilities(&ns_mrp, "capabilities");
NS(ns_mrp_conf, ns_mrp, "config");
static TableReadWrite<MrpInterfaceConfigurationTable>
    mrp_if_conf_table(&ns_mrp_conf, "interface");
#endif // VTSS_SW_OPTION_MRP

#ifdef VTSS_SW_OPTION_MVRP
static NamespaceNode ns_mvrp("mvrp");
extern "C" void vtss_appl_mvrp_json_init()
{
    json_node_add(&ns_mvrp);
}

NS(ns_mvrp_conf, ns_mvrp, "config");
static StructReadWrite<MvrpGlobalsLeaf> mvrp_globals_conf_leaf(&ns_mvrp_conf, "global");
static TableReadWrite<MvrpInterfaceConfigurationTable>
    mvrp_if_conf_table(&ns_mvrp_conf, "interface");

NS(ns_mvrp_statistics, ns_mvrp, "statistics");
static TableReadOnly<MvrpInterfaceStatisticsTable>
    mvrp_if_stat_table(&ns_mvrp_statistics, "interface");
#endif // VTSS_SW_OPTION_MVRP
