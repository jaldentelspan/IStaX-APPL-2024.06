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

#include "ip_source_guard_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::ip_source_guard::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_ip_source_guard("ipSourceGuard");
extern "C" void vtss_appl_ip_source_guard_json_init() { json_node_add(&ns_ip_source_guard); }

NS(ns_conf,       ns_ip_source_guard, "config");
NS(ns_status,     ns_ip_source_guard, "status");
NS(ns_control,    ns_ip_source_guard, "control");

namespace vtss {
namespace appl {
namespace ip_source_guard {
namespace interfaces {
static StructReadOnly<IpSourceGuardCapabilitiesLeaf> ip_source_guard_capabilities_leaf(
        &ns_ip_source_guard, "capabilities");
static StructReadWrite<IpSourceGuardParamsLeaf> ip_source_guard_params_leaf(
        &ns_conf, "global");
static TableReadWrite<IpSourceGuardInterfaceEntry> ip_source_guard_interface_entry(
        &ns_conf, "interface");
static TableReadWriteAddDelete<IpSourceGuardStaticConfigEntry> ip_source_guard_static_config_entry(
        &ns_conf, "static");
static TableReadOnly<IpSourceGuardDynamicStatusEntry> ip_source_guard_dynamic_status_entry(
        &ns_status, "dynamic");
static StructWriteOnly<IpSourceGuardControlLeaf> ip_source_guard_control_leaf(
        &ns_control, "translate");
}  // namespace interfaces
}  // namespace ip_source_guard
}  // namespace appl
}  // namespace vtss
