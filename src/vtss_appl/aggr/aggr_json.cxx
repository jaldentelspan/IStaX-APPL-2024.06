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

#include "aggr_serializer.hxx"
#include "vtss/basics/expose/json.hxx"
#include "vtss/appl/aggr.h"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::aggr::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_aggr("aggregation");
extern "C" void vtss_appl_aggr_json_init() { json_node_add(&ns_aggr); }

NS(ns_conf, ns_aggr, "config");
NS(ns_status, ns_aggr, "status");

namespace vtss {
namespace appl {
namespace aggr {
namespace interfaces {

StructReadWrite<AggrModeParamsLeaf> aggr_mode_params_leaf(&ns_conf, "mode");
TableReadWrite<AggrGroupConfTable> aggr_group_conf_table(&ns_conf, "group");
TableReadOnly<AggrGroupStatusTable> aggr_group_status_table(&ns_status, "group");
TableReadOnlyNotification<AggrGroupStatusTable> status_params_update(&ns_status, "notification", &aggr_status_update);

}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss
