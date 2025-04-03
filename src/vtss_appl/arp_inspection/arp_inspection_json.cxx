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

#include "arp_inspection_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::arp_inspection::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_arp_inspection("arpInspection");
extern "C" void vtss_appl_arp_inspection_json_init() { json_node_add(&ns_arp_inspection); }

NS(ns_conf,       ns_arp_inspection, "config");
NS(ns_status,     ns_arp_inspection, "status");
NS(ns_statistics, ns_arp_inspection, "statistics");
NS(ns_control,    ns_arp_inspection, "control");

namespace vtss {
namespace appl {
namespace arp_inspection {
namespace interfaces {
static StructReadWrite<ArpInspectionParamsLeaf> arp_inspection_params_leaf(
        &ns_conf, "global");
static TableReadWrite<ArpInspectionPortConfigTable> arp_inspection_port_config_table(
        &ns_conf, "interface");
static TableReadWriteAddDelete<ArpInspectionVlanConfigTable> arp_inspection_vlan_config_table(
        &ns_conf, "vlan");
static TableReadWriteAddDelete<ArpInspectionStaticConfigTable> arp_inspection_static_config_table(
        &ns_conf, "static");
static TableReadOnly<ArpInspectionDynamicStatusTable> arp_inspection_dynamic_status_table(
        &ns_status, "dynamic");
static StructWriteOnly<ArpInspectionActionLeaf> arp_inspection_action_leaf(
        &ns_control, "translate");
/* JSON notification */
static StructReadOnlyNotification<ArpInspectionStatusEventEntry> arp_inspection_status_event_entry(
        &ns_status, "crossedThreshold", &arp_inspection_status_event_update);
}  // namespace interfaces
}  // namespace arp_inspection
}  // namespace appl
}  // namespace vtss

