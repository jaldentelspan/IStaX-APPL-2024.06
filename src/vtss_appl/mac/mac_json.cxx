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

#include "mac_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::mac::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_mac("mac");
void vtss_appl_mac_json_init() { json_node_add(&ns_mac); }

NS(ns_conf,       ns_mac,    "config");
NS(ns_status,     ns_mac,    "status");
NS(ns_statistics, ns_mac,    "statistics");

NS(ns_conf_fdb,   ns_conf,   "fdb");
NS(ns_conf_port,  ns_conf,   "interface");
NS(ns_conf_vlan,  ns_conf,   "vlan");

NS(ns_status_fdb, ns_status, "fdb");


namespace vtss {
namespace appl {
namespace mac {
namespace interfaces {
// mac.capabilities
static StructReadOnly<MacCapLeaf> mac_cap_leaf(&ns_mac, "capabilities");

// mac.conf.global
static StructReadWrite<MacAgeLeaf> mac_age_leaf(&ns_conf, "global");

// mac.conf.fdb.static
static TableReadWriteAddDelete<MacAddressConfigTableImpl> mac_address_config_table_impl(&ns_conf_fdb, "static");

// mac.conf.port.learn
static TableReadWrite<MacAddressLearnTableImpl> mac_address_learn_table_impl(&ns_conf_port, "learn");

// mac.conf.vlan.learn
static TableReadWrite<MacAddressVlanLearnTableImpl> mac_address_vlan_learn_table_impl(&ns_conf_vlan, "learn");

// mac.status.fdb.full
static TableReadOnly<MacFdbTableImpl> mac_fdb_table_impl(&ns_status_fdb, "full");

// mac.status.fdb.static
static TableReadOnly<MacFdbTableStaticImpl> mac_fdb_table_static_impl(&ns_status_fdb, "static");

// mac.statistics.interface
static TableReadOnly<MacAddressStatsTableImpl> mac_address_stats_table_impl(&ns_statistics, "interface");

// mac.statistics.fdb
static StructReadOnly<MacStatisLeaf> mac_statis_leaf(&ns_statistics, "fdb");

// mac.control
static StructReadWrite<MacFlushLeaf> mac_flush_leaf(&ns_mac, "control");
}  // namespace interfaces
}  // namespace mac
}  // namespace appl
}  // namespace vtss


