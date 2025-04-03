/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "mstp_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::mstp::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_mstp("mstp");
extern "C" void vtss_appl_mstp_json_init()
{
    json_node_add(&ns_mstp);
}

NS(ns_conf,       ns_mstp, "config");
NS(ns_status,     ns_mstp, "status");
NS(ns_statistics, ns_mstp, "statistics");

NS(ns_conf_msti,  ns_conf, "msti");
NS(ns_conf_cist,  ns_conf, "cist");

namespace vtss
{
namespace appl
{
namespace mstp
{
namespace interfaces
{
// mstp.conf.bridge
static StructReadWrite<MstpBridgeLeaf> mstp_bridge_leaf(&ns_conf, "bridge");

// mstp.conf.msti.table
static TableReadWrite<MstpMstiParamsTable> mstp_msti_params_table(&ns_conf_msti, "parameters");

// mstp.conf.msti.config
static StructReadWrite<MstpMstiConfigLeaf> mstp_msti_config_leaf(&ns_conf_msti, "config");

// mstp.conf.msti.map
static TableReadWrite<MstpMstiConfigTableEntries> mstp_msti_config_table_entries(&ns_conf_msti, "map");

// mstp.conf.msti.vlan.bitmap.map
static TableReadWrite<MstpMstiConfigVlanBitmapTableEntries> mstp_msti_config_vlan_bitmap_table_entries(&ns_conf_msti, "vlan_bitmap");

// mstp.conf.cist.interface
static TableReadWrite<MstpCistportParamsTable> mstp_cistport_params_table(&ns_conf_cist, "interface");

// mstp.conf.cist.aggr
static StructReadWrite<MstpAggrParamLeaf> mstp_aggr_param_leaf(&ns_conf_cist, "aggr");

// mstp.conf.msti.interface
static TableReadWrite<MstpMstiportParamTable> mstp_mstiport_param_table(&ns_conf_msti, "interface");

// mstp.conf.msti.aggr
static TableReadWrite<MstpMstiportAggrparamTable> mstp_mstiport_aggrparam_table(&ns_conf_msti, "aggr");

// mstp.status.bridge
static TableReadOnly<MstpBridgeStatusTable> mstp_bridge_status_table(&ns_status, "bridge");

// mstp.status.interface
static TableReadOnly<MstpPortStatusTable> mstp_port_status_table(&ns_status, "interface");

// mstp.statistics.interface
static TableReadOnly<MstpPortStatsTable> mstp_port_stats_table(&ns_statistics, "interface");
}  // namespace interfaces
}  // namespace mstp
}  // namespace appl
}  // namespace vtss

