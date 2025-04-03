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

#include "sflow_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::sflow::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_sflow("sflow");
extern "C" void vtss_appl_sflow_json_init() { json_node_add(&ns_sflow); }

NS(ns_conf,     ns_sflow, "config");;
NS(ns_status,   ns_sflow, "status");;
NS(ns_stats,    ns_sflow, "statistics");;
NS(ns_control,  ns_sflow, "control");;

namespace vtss {
namespace appl {
namespace sflow {
static StructReadWrite<SflowAgentConfigGlobals>     sflow_agent_params_leaf(&ns_conf, "agent");
static TableReadWriteAddDelete<SflowRcvrConfTable>  sflow_rcvr_conf_table(&ns_conf, "rcvr");
static TableReadWriteAddDelete<SflowFlowSamConfTable> sflow_sampler_conf_table(&ns_conf, "fs");
static TableReadWriteAddDelete<SflowCpConfTable>    sflow_cpol_conf___ad__table(&ns_conf, "cp");
static TableReadOnly<SflowRcvrStatusTable>          sflow_rcvr_status_get_table(&ns_status, "rcvr");
static TableReadOnly<SflowRcvrStatisticsTable>      sflow_rcvr_statis_get_table(&ns_stats, "rcvr");
static TableReadWrite<SflowRcvrStatsClearTable>     sflow_rcvr_statis_clr_table(&ns_control, "rcvr");
static TableReadOnly<SflowInstanceStatisticsTable>  sflow_inst_statis_get_table(&ns_stats, "instance");
static TableReadWrite<SflowInstanceStatsClearTable> sflow_inst_statis_clr_table(&ns_control, "instance");
}  // namespace sflow
}  // namespace appl
}  // namespace vtss
