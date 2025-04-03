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

#include "dhcp6_relay_serializer.hxx"
#include "vtss/basics/expose/json.hxx"
#include "dhcp6_relay.h"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_dhcp6_relay("dhcp6_relay");
extern "C" void vtss_appl_dhcp6_relay_json_init() { json_node_add(&ns_dhcp6_relay); }

NS(ns_capability,  ns_dhcp6_relay,  "capabilities");
NS(ns_conf,        ns_dhcp6_relay,  "config");
NS(ns_status,      ns_dhcp6_relay,  "status");
NS(ns_statistics,  ns_dhcp6_relay,  "statistics");
NS(ns_control,     ns_dhcp6_relay,  "control");

namespace vtss {
namespace appl {
namespace dhcp6_relay {
namespace interfaces {

expose::json::TableReadWriteAddDelete<Dhcp6RelayVlanConfigEntry> dhcp6_relay_interface_conf_table(
    &ns_conf, "vlan");

expose::json::TableReadOnly<Dhcp6RelayVlanStatusEntry> dhcp6_relay_interface_status_table(
&ns_status, "vlan");

expose::json::TableReadWrite<Dhcp6RelayVlanStatisticsEntry> dhcp6_relay_interface_statistics_table(
&ns_statistics, "vlan");

expose::json::StructReadOnly<Dhcp6RelayInterfaceMissinglLeaf> dhcp6_relay_statistics_leaf(
&ns_statistics, "global");

expose::json::StructReadWrite<Dhcp6RelayControlLeaf> dhcp6_relay_conf_leaf(
&ns_control, "global");

}  // namespace interfaces
}  // namespace dhcp6_relay
}  // namespace appl
}  // namespace vtss
