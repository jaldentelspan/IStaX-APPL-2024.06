/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "dhcp6_snooping_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::dhcp6_snooping::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_dhcp6_snooping("dhcp6Snooping");
extern "C" void vtss_appl_dhcp6_snooping_json_init() { json_node_add(&ns_dhcp6_snooping); }

NS(ns_conf,                         ns_dhcp6_snooping,      "config");
NS(ns_status,                       ns_dhcp6_snooping,      "status");
NS(ns_control,                      ns_dhcp6_snooping,      "control");
NS(ns_statistics,                   ns_dhcp6_snooping,      "statistics");
//NS(ns_control_statistics,           ns_control,             "statistics");
//NS(ns_control_statistics_interface, ns_control_statistics,  "interface");

namespace vtss {
namespace appl {
namespace dhcp6_snooping {
namespace interfaces {
static StructReadWrite<Dhcp6SnoopingParamsLeaf> dhcp6_snooping_params_leaf(
        &ns_conf, "global");
static TableReadWrite<Dhcp6SnoopingInterfaceEntry> dhcp6_snooping_interface_entry(
        &ns_conf, "interface");
static StructReadOnly<Dhcp6SnoopingGlobalStatusImpl> dhcp6_snooping_global_status_impl(
        &ns_status, "global");
static TableReadOnly<Dhcp6SnoopingClientInfoEntry> dhcp6_snooping_client_table_entry(
        &ns_status, "client");
static TableReadOnly<Dhcp6SnoopingAddressInfoEntry> dhcp6_snooping_address_table_entry(
        &ns_status, "address");
static TableReadWrite<Dhcp6SnoopingInterfaceStatisticsEntry> dhcp6_snooping_interface_statistics_entry(
        &ns_statistics, "interface");
}  // namespace interfaces
}  // namespace dhcp6_snooping
}  // namespace appl
}  // namespace vtss
