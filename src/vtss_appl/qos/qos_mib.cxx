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

using namespace vtss;
using namespace expose::snmp;
VTSS_MIB_MODULE("qosMib", "QOS", qos_mib_init, VTSS_MODULE_ID_QOS, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201408120000Z", "Updated descriptions when referring to a capability.");
    h.add_history_element("201409170000Z", "Added QosCapabilitiesHasQueueShapersExcess object.");
    h.add_history_element("201410080000Z", "Removed QosCapabilitiesBitrateMin, QosCapabilitiesBitrateMax, "
                          "QosCapabilitiesBurstSizeMin and QosCapabilitiesBurstSizeMax.\n"
                          "Added policer and shaper specific min/max capabilities objects.\n"
                          "Added QosConfigGlobalStormPolicersUnicastFrameRate, "
                          "QosConfigGlobalStormPolicersMulticastFrameRate and "
                          "QosConfigGlobalStormPolicersBroadcastFrameRate.");
    h.add_history_element("201411060000Z", "Added new WredV3 related objects and adapted WredTable to WredV3.");
    h.add_history_element("201504070000Z", "Added QosConfigInterfaceQueueShaperRateType, "
                          "QosConfigInterfaceShaperRateType objects.");
    h.add_history_element("201505270000Z", "Added QosConfigIngressMap, "
                          "QosConfigEgressMap objects and related changes.");
    h.add_history_element("201506230000Z", "Removed QosConfigIngressMapPcpAction, QosConfigIngressMapDscpAction, "
                          "QosConfigEgressMapCosidAction and QosConfigEgressMapDscpAction objects.");
    h.add_history_element("201508130000Z", "Changed to use Dscp types in QCEs.");
    h.add_history_element("201509300000Z", "Added QosConfigInterfaceCosId.");
    h.add_history_element("201511130000Z", "Updated descriptions.");
    h.add_history_element("201601190000Z", "Added (IEEE 802.1Qbv) Time Aware Shaper support.");
    h.add_history_element("201602110000Z", "Added QosCapabilitiesHasQueueShapersCredit object.");
    h.add_history_element("201602250000Z", "Added QosCapabilitiesHasQueueCutThrough object.");
    h.add_history_element("201610170000Z", "Updated table dependencies and descriptions. Added new capability element for Wred");
    h.add_history_element("201703200000Z", "Added Frame Preemption support. Added PSFP support.");
    h.add_history_element("201711270000Z", "Added MPLS TC capability test. Split AdminBaseTimes into Secs and NanoSecs. "
                          "Updated various descriptions slightly.");
    h.add_history_element("201806270000Z", "More QCE users.");
    h.add_history_element("201904050000Z", "Obsoleted various MPLS-related fields.");
    h.add_history_element("201905290000Z", "Removed unused PSFP support.");
    h.description("This is a private MIB for QoS");
}

namespace vtss {
namespace appl {
namespace qos {
namespace interfaces {

// root
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(qosMibObjects,      root, 1, "qosMibObjects");

// parent: qos
NS(qosConfig,          qosMibObjects, 2, "qosConfig");
NS(qosStatus,          qosMibObjects, 3, "qosStatus");
NS(qosControl,         qosMibObjects, 4, "qosControl");

// parent: qos/config
NS(qosConfigGlobals,   qosConfig, 1, "qosConfigGlobals"); // Global parameters
NS(qosConfigInterface, qosConfig, 2, "qosConfigInterface"); // Interface parameters
NS(qosConfigQce,       qosConfig, 3, "qosConfigQce"); // QCE parameters
NS(qosConfigIngressMap,qosConfig, 4, "qosConfigIngressMap"); // Ingress Map
NS(qosConfigEgressMap, qosConfig, 5, "qosConfigEgressMap"); // Egress Map
// parent: qos/config/globals
NS(qosStormPolicers,   qosConfigGlobals, 1, "qosConfigGlobalsStormPolicers"); // Storm policers

// parent: qos/status
NS(qosStatusInterface, qosStatus, 2, "qosStatusInterface"); // Interface status
NS(qosStatusQce,       qosStatus, 3, "qosStatusQce");
// qosMib.qosMibObjects.1
static StructRO2<QosCapabilities> qos_capabilities(
        &qosMibObjects, vtss::expose::snmp::OidElement(1, "qosCapabilities"));

// qosMib.qosMibObjects.qosConfig.qosConfigGlobals.qosConfigGlobalsStormPolicers.1
static StructRW2<QosStormPolicerUnicastLeaf> qos_storm_policer_unicast_leaf(
        &qosStormPolicers, vtss::expose::snmp::OidElement(1, "qosConfigGlobalsStormPolicersUnicast"));

// qosMib.qosMibObjects.qosConfig.qosConfigGlobals.qosConfigGlobalsStormPolicers.2
static StructRW2<QosStormPolicerMulticastLeaf> qos_storm_policer_multicast_leaf(
        &qosStormPolicers, vtss::expose::snmp::OidElement(2, "qosConfigGlobalsStormPolicersMulticast"));

// qosMib.qosMibObjects.qosConfig.qosConfigGlobals.qosConfigGlobalsStormPolicers.3
static StructRW2<QosStormPolicerBroadcastLeaf> qos_storm_policer_broadcast_leaf(
        &qosStormPolicers, vtss::expose::snmp::OidElement(3, "qosConfigGlobalsStormPolicersBroadcast"));

// qosMib.qosMibObjects.qosConfig.qosConfigGlobals.2
static TableReadWrite2<QosWredEntry> qos_wred_entry(
        &qosConfigGlobals, vtss::expose::snmp::OidElement(2, "qosConfigGlobalsWredTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigGlobals.3
static TableReadWrite2<QosDscpE> qos_dscp_e(
        &qosConfigGlobals, vtss::expose::snmp::OidElement(3, "qosConfigGlobalsDscpTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigGlobals.4
static TableReadWrite2<QosCosToDscpEntry> qos_cos_to_dscp_entry(
        &qosConfigGlobals, vtss::expose::snmp::OidElement(4, "qosConfigGlobalsCosToDscpTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigIngressMap.1
static TableReadWriteAddDelete2<QosIngressMapEntry> qos_ingress_map_entry(
        &qosConfigIngressMap, vtss::expose::snmp::OidElement(1, "qosConfigIngressMapTable"), vtss::expose::snmp::OidElement(2, "qosConfigIngressMapTableRowEditor"));

// qosMib.qosMibObjects.qosConfig.qosConfigIngressMap.3
static TableReadWrite2<QosIngressMapPcpEntry> qos_ingress_map_pcp_entry(
        &qosConfigIngressMap, vtss::expose::snmp::OidElement(3, "qosConfigIngressMapPcpTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigIngressMap.4
static TableReadWrite2<QosIngressMapDscpEntry> qos_ingress_map_dscp_entry(
        &qosConfigIngressMap, vtss::expose::snmp::OidElement(4, "qosConfigIngressMapDscpTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigEgressMap.1
static TableReadWriteAddDelete2<QosEgressMapEntry> qos_egress_map_entry(
        &qosConfigEgressMap, vtss::expose::snmp::OidElement(1, "qosConfigEgressMapTable"), vtss::expose::snmp::OidElement(2, "qosConfigEgressMapTableRowEditor"));

// qosMib.qosMibObjects.qosConfig.qosConfigEgressMap.3
static TableReadWrite2<QosEgressMapCosidEntry> qos_egress_map_cosid_entry(
        &qosConfigEgressMap, vtss::expose::snmp::OidElement(3, "qosConfigEgressMapCosidTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigEgressMap.4
static TableReadWrite2<QosEgressMapDscpEntry> qos_egress_map_dscp_entry(
        &qosConfigEgressMap, vtss::expose::snmp::OidElement(4, "qosConfigEgressMapDscpTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.1
static TableReadWrite2<QosIfConfigEntry> qos_if_config_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(1, "qosConfigInterfaceTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.2
static TableReadWrite2<QosIfTagToCosEntry> qos_if_tag_to_cos_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(2, "qosConfigInterfaceTagToCosTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.3
static TableReadWrite2<QosIfCosToTagEntry> qos_if_cos_to_tag_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(3, "qosConfigInterfaceCosToTagTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.4
static TableReadWrite2<QosIfPolicerEntry> qos_if_policer_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(4, "qosConfigInterfacePolicerTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.5
static TableReadWrite2<QosIfQueuePolicerEntry> qos_if_queue_policer_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(5, "qosConfigInterfaceQueuePolicerTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.6
static TableReadWrite2<QosIfShaperEntry> qos_if_shaper_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(6, "qosConfigInterfaceShaperTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.7
static TableReadWrite2<QosIfQueueShaperEntry> qos_if_queue_shaper_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(7, "qosConfigInterfaceQueueShaperTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.8
static TableReadWrite2<QosIfSchedulerEntry> qos_if_scheduler_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(8, "qosConfigInterfaceSchedulerTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.9
static TableReadWrite2<QosIfStormPolicerUnicastEntry> qos_if_storm_policer_unicast_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(9, "qosConfigInterfaceStormPolicerUnicastTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.10
static TableReadWrite2<QosIfStormPolicerBroadcastEntry> qos_if_storm_policer_broadcast_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(10, "qosConfigInterfaceStormPolicerBroadcastTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigInterface.11
static TableReadWrite2<QosIfStormPolicerUnknownEntry> qos_if_storm_policer_unknown_entry(
        &qosConfigInterface, vtss::expose::snmp::OidElement(11, "qosConfigInterfaceStormPolicerUnknownTable"));

// qosMib.qosMibObjects.qosConfig.qosConfigQce.1
static TableReadWriteAddDelete2<QosQceEntry> qos_qce_entry(
        &qosConfigQce, vtss::expose::snmp::OidElement(1, "qosConfigQceTable"), vtss::expose::snmp::OidElement(2, "qosConfigQceTableRowEditor"));

// qosMib.qosMibObjects.qosConfig.qosConfigQce.3
static TableReadOnly2<QosQcePrecedenceEntry> qos_qce_precedence_entry(
        &qosConfigQce, vtss::expose::snmp::OidElement(3, "qosConfigQcePrecedenceTable"));

// qosMib.qosMibObjects.qosStatus.qosStatusInterface.1
static TableReadOnly2<QosIfStatusEntry> qos_if_status_entry(
        &qosStatusInterface, vtss::expose::snmp::OidElement(1, "qosStatusInterfaceTable"));

// qosMib.qosMibObjects.qosStatus.qosStatusInterface.2
static TableReadOnly2<QosIfSchedulerStatusEntry> qos_if_scheduler_status_entry(
        &qosStatusInterface, vtss::expose::snmp::OidElement(2, "qosStatusInterfaceSchedulerTable"));

// qosMib.qosMibObjects.qosControl.3
static StructRW2<QosQceConflictResolve> qos_qce_conflict_resolve(
        &qosControl, vtss::expose::snmp::OidElement(3, "qosControlQce"));


}  // namespace interfaces
}  // namespace qos
}  // namespace appl
}  // namespace vtss
