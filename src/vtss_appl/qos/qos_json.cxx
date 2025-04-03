/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "qos_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::qos::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_qos("qos");
extern "C" void vtss_appl_qos_json_init() { json_node_add(&ns_qos); }

NS(ns_conf,              ns_qos,         "config");
NS(ns_status,            ns_qos,         "status");
NS(ns_control,           ns_qos,         "control");

NS(ns_conf_global,       ns_conf,        "global");
NS(ns_conf_if,           ns_conf,        "interface");
NS(ns_conf_qce,          ns_conf,        "qce");
NS(ns_conf_ingress_map,  ns_conf,        "ingressMap");
NS(ns_conf_egress_map,   ns_conf,        "egressMap");

NS(ns_conf_global_storm, ns_conf_global, "stormPolicer");
NS(ns_conf_if_storm,     ns_conf_if,     "stormPolicer");

NS(ns_status_if,         ns_status,      "interface");
NS(ns_status_global,     ns_status,      "global");


static StructReadOnlyNotificationNoNS<QosStatusParams> qos_status(&ns_status_global, &qos_status_update);

static StructReadOnly<QosCapabilities> qos_capabilities(&ns_qos, "capabilities");

// qos.config.global.storm-policer.Unicast
static StructReadWrite<QosStormPolicerUnicastLeaf> qos_storm_policer_unicast_leaf(&ns_conf_global_storm, "unicast");

// qos.config.global.storm-policer.multicast
static StructReadWrite<QosStormPolicerMulticastLeaf> qos_storm_policer_multicast_leaf(&ns_conf_global_storm, "multicast");

// qos.config.global.storm-policer.broadcast
static StructReadWrite<QosStormPolicerBroadcastLeaf> qos_storm_policer_broadcast_leaf(&ns_conf_global_storm, "broadcast");

// qos.config.global.wred
static TableReadWrite<QosWredEntry> qos_wred_entry(&ns_conf_global, "wred");

// qos.config.global.dscp
static TableReadWrite<QosDscpE> qos_dscp_e(&ns_conf_global, "dscp");

// qos.config.global.cos-to-dscp
static TableReadWrite<QosCosToDscpEntry> qos_cos_to_dscp_entry(&ns_conf_global, "cosToDscp");

// qos.config.ingress-map
static TableReadWriteAddDeleteNoNS<QosIngressMapEntry> qos_ingress_map_entry(&ns_conf_ingress_map);

// qos.config.ingress-map.pcp
static TableReadWrite<QosIngressMapPcpEntry> qos_ingress_map_pcp_entry(&ns_conf_ingress_map, "pcp");

// qos.config.ingress-map.dscp
static TableReadWrite<QosIngressMapDscpEntry> qos_ingress_map_dscp_entry(&ns_conf_ingress_map, "dscp");

// qos.config.egress-map
static TableReadWriteAddDeleteNoNS<QosEgressMapEntry> qos_egress_map_entry(&ns_conf_egress_map);

// qos.config.egress-map.cosid
static TableReadWrite<QosEgressMapCosidEntry> qos_egress_map_cosid_entry(&ns_conf_egress_map, "cosid");

// qos.config.egress-map.dscp
static TableReadWrite<QosEgressMapDscpEntry> qos_egress_map_dscp_entry(&ns_conf_egress_map, "dscp");

// qos.config.interface
static TableReadWriteNoNS<QosIfConfigEntry> qos_if_config_entry(&ns_conf_if);

// qos.config.interface.tag-to-cos
static TableReadWrite<QosIfTagToCosEntry> qos_if_tag_to_cos_entry(&ns_conf_if, "tagToCos");

// qos.config.interface.cos-to-tag
static TableReadWrite<QosIfCosToTagEntry> qos_if_cos_to_tag_entry(&ns_conf_if, "cosToTag");

// qos.config.interface.policer
static TableReadWrite<QosIfPolicerEntry> qos_if_policer_entry(&ns_conf_if, "policer");

// qos.config.interface.queue-policer
static TableReadWrite<QosIfQueuePolicerEntry> qos_if_queue_policer_entry(&ns_conf_if, "queuePolicer");

// qos.config.interface.shaper
static TableReadWrite<QosIfShaperEntry> qos_if_shaper_entry(&ns_conf_if, "shaper");

// qos.config.interface.queue-shaper
static TableReadWrite<QosIfQueueShaperEntry> qos_if_queue_shaper_entry(&ns_conf_if, "queueShaper");

// qos.config.interface.scheduler
static TableReadWrite<QosIfSchedulerEntry> qos_if_scheduler_entry(&ns_conf_if, "scheduler");

// qos.config.interface.strom-policer.unicast
static TableReadWrite<QosIfStormPolicerUnicastEntry> qos_if_storm_policer_unicast_entry(&ns_conf_if_storm, "unicast");

// qos.config.interface.strom-policer.broadcast
static TableReadWrite<QosIfStormPolicerBroadcastEntry> qos_if_storm_policer_broadcast_entry(&ns_conf_if_storm, "broadcast");

// qos.config.interface.strom-policer.unknown
static TableReadWrite<QosIfStormPolicerUnknownEntry> qos_if_storm_policer_unknown_entry(&ns_conf_if_storm, "unknown");

// qos.config.qce
static TableReadWriteAddDeleteNoNS<QosQceEntry> qos_qce_entry(&ns_conf_qce);

// qos.config.qce.precedence
static TableReadOnly<QosQcePrecedenceEntry> qos_qce_precedence_entry(&ns_conf_qce, "precedence");

// qos.status.interface
static TableReadOnlyNoNS<QosIfStatusEntry> qos_if_status_entry(&ns_status_if);

// qos.status.interface.scheduler
static TableReadOnly<QosIfSchedulerStatusEntry> qos_if_scheduler_status_entry(&ns_status_if, "scheduler");

// qos.status.qce
static TableReadOnly<QosQceStatusEntry> qos_qce_status_entry(&ns_status, "qce");

// qos.control.qce
static StructWriteOnly<QosQceConflictResolve> qos_qce_conflict_resolve(&ns_control, "qce");

