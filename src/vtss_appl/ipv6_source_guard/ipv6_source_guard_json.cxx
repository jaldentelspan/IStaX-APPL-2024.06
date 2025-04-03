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

#include "ipv6_source_guard_serializer.hxx"
#include "vtss/basics/expose/json.hxx"
#include "ipv6_source_guard.h"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;

namespace vtss {
    void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_ipv6_source_guard("ipv6_source_guard");
extern "C" void vtss_appl_ipv6_source_guard_json_init() { json_node_add(&ns_ipv6_source_guard); }

// NS(ns_capability,  ns_ipv6_source_guard,  "capabilities");
NS(ns_conf,        ns_ipv6_source_guard,  "config");
NS(ns_status,      ns_ipv6_source_guard,  "status");
NS(ns_control,     ns_ipv6_source_guard,  "control");

namespace vtss {
namespace appl {
namespace ipv6_source_guard {
namespace interfaces {

expose::json::StructReadWrite<IPv6SourceGuardGlobalConfigLeaf> ipv6_source_guard_global_conf_leaf(
    &ns_conf, "global");

expose::json::TableReadWrite<IPv6SourceGuardPortConfigEntry> ipv6_source_guard_port_conf_entry(
    &ns_conf, "port");

expose::json::TableReadWriteAddDelete<Ipv6SourceGuardStaticEntry> ipv6_source_guard_static_entry(
    &ns_conf, "static");

expose::json::TableReadOnly<Ipv6SourceGuardDynamicEntry> ipv6_source_guard_port_dynamic_entry(
    &ns_status, "dynamic");

expose::json::StructWriteOnly<Ipv6SourceGuardControlTranslateLeaf> ipv6_source_guard_translate_control_leaf(
    &ns_control, "translate");

}  // namespace interfaces
}  // namespace ipv6_source_guard
}  // namespace appl
}  // namespace vtss

