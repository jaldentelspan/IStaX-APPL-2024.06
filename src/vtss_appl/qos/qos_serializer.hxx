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
#ifndef __VTSS_QOS_SERIALIZER_HXX__
#define __VTSS_QOS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include <qos_api.h>
#include <vtss/appl/qos.h>
#include <vtss/appl/vlan.h>
#include <vtss/appl/types.hxx>
#include "vtss_vcap_serializer.hxx"

extern const vtss_appl_qos_capabilities_t * const vtss_appl_qos_capabilities;
#define CAPA vtss_appl_qos_capabilities
#define QOS_CAP_STR(name) " This object is only valid if qosCapabilities" name " is true."

extern vtss::expose::StructStatus<
    vtss::expose::ParamVal<vtss_appl_qos_status_t * >> qos_status_update;


/****************************************************************************
 * Enum definitions
 ****************************************************************************/
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_wred_max_t,
                         "QosWredMaxSelector",
                         vtss_appl_qos_wred_max_txt,
                         "An integer that selects between 'Maximum Drop Probability' or 'Maximum Fill Level'.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_tag_remark_mode_t,
                         "QosTagRemarkingMode",
                         vtss_appl_qos_tag_remark_mode_txt,
                         "An integer that indicates the tag remarking mode.");


VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_dscp_mode_t,
                         "QosDscpClassify",
                         vtss_appl_qos_dscp_mode_txt,
                         "An integer that indicates the DSCP classify mode.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_dscp_emode_t,
                         "QosDscpRemark",
                         vtss_appl_qos_dscp_emode_txt,
                         "An integer that indicates the DSCP remark mode.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_qcl_user_t,
                         "QosQceUserType",
                         vtss_appl_qos_qcl_user_txt,
                         "An integer that indicates the QCE user type.\n"
                         "If the value is zero it indicates a static configuration.\n"
                         "If the value is non-zero it indicates a dynamic configuration made by another subsystem.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_qce_type_t,
                         "QosQceFrameType",
                         vtss_appl_qos_qce_type_txt,
                         "An integer that indicates the QCE frame type key.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_shaper_mode_t,
                         "QosShaperRateType",
                         vtss_appl_qos_shaper_mode_txt,
                         "An integer that indicates the shaper rate type.\n"
                         "If the value is zero, it indicates the shaper uses line-rate.\n"
                         "If the value is one, it indicates the shaper uses data-rate.\n"
                         "NOTE: For QoS Queue shapers, this configuration is valid "
                         "only for Normal port mode. If the port is in Basic or "
                         "Hierarchical mode, the rate-type will be stored and "
                         "become active when the port is switched back to "
                         "Normal Scheduling mode.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_ingress_map_key_t,
                         "QosIngressMapKey",
                         vtss_appl_qos_ingress_map_key_txt,
                         "An integer that indicates the key to use when matching frames in the ingress map.\n"
                         "If the value is zero, use PCP for tagged frames and none for the rest.\n"
                         "If the value is one, use PCP/DEI for tagged frames and none for the rest.\n"
                         "If the value is two, use DSCP as key for IP frames and none for the rest.\n"
                         "If the value is three, use DSCP as key for IP frames, PCP/DEI for tagged frames and none for the rest.\n"
                         "The value four is obsolete.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_qos_egress_map_key_t,
                         "QosEgressMapKey",
                         vtss_appl_qos_egress_map_key_txt,
                         "An integer that indicates the key to use when matching frames in the egress map.\n"
                         "If the value is zero, use classified COS ID.\n"
                         "If the value is one, use classified COS ID and DPL.\n"
                         "If the value is two, use classified DSCP.\n"
                         "If the value is three, use classified DSCP and DPL.\n"
                         "The values four and five are obsolete.");


/****************************************************************************
 * Generic index serializers
 ****************************************************************************/
// Generic serializer for vtss_ifindex_t
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_ifindex_t, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface index."));
}

// Generic serializer for mesa_prio_t used as queue index
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_queue_index_t, mesa_prio_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Queue"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Queue index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_dscp_index_t, mesa_dscp_t, a, s) {
    a.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.inner),
               vtss::tag::Name("Dscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("DSCP index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_cos_index_t, mesa_prio_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Cos"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("CoS index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_wred_group_index_t, mesa_wred_group_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Group"),
               vtss::expose::snmp::RangeSpec<uint32_t>(1, 3),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("WRED group index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_dpl_index_t, mesa_dp_level_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Dpl"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 3),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("DPL index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_color_index_t, mesa_dp_level_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Dpl"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 1),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Color index. It is 0 for DPL 0 and 1 for all other values of DPL"));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_pcp_index_t, mesa_tagprio_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Pcp"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("PCP index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_dei_index_t, mesa_dei_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Dei"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 1),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("DEI index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_cosid_index_t, mesa_cosid_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Cosid"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("COSID index."));
}


/****************************************************************************
 * Capabilities
 ****************************************************************************/

//s.class_min
struct QosCapClassMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "ClassMin";
    static constexpr const char *desc = "Minimum allowed QoS class.";
    static constexpr uint32_t get() {
        return VTSS_APPL_QOS_CLASS_MIN;
    }
};

//s.class_max
struct QosCapClassMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "ClassMax";
    static constexpr const char *desc = "Maximum allowed QoS class.";
    static constexpr uint32_t get() {
        return VTSS_APPL_QOS_CLASS_MAX;
    }
};

//s.dpl_min
struct QosCapDplMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "DplMin";
    static constexpr const char *desc = "Minimum allowed drop precedence level.";
    static mesa_dp_level_t get() { return CAPA->dpl_min; }
};

//s.dpl_max
struct QosCapDplMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "DplMax";
    static constexpr const char *desc = "Maximum allowed drop precedence level.";
    static mesa_dp_level_t get() { return CAPA->dpl_max; }
};

//s.wred_group_min
struct QosCapWredGroupMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "WredGroupMin";
    static constexpr const char *desc = "Minimum allowed WRED group number.";
    static uint32_t get() { return CAPA->wred_group_min; }
};

//s.wred_group_max
struct QosCapWredGroupMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "WredGroupMax";
    static constexpr const char *desc = "Maximum allowed WRED group number.";
    static uint32_t get() { return CAPA->wred_group_max; }
};

//s.wred_dpl_min
struct QosCapWredDplMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "WredDplMin";
    static constexpr const char *desc = "Minimum allowed WRED drop precedence level.";
    static mesa_dp_level_t get() { return CAPA->wred_dpl_min; }
};

//s.wred_dpl_max
struct QosCapWredDplMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "WredDplMax";
    static constexpr const char *desc = "Maximum allowed WRED drop precedence level.";
    static mesa_dp_level_t get() { return CAPA->wred_dpl_max; }
};

//s.qce_id_min
struct QosCapQceIdMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QceIdMin";
    static constexpr const char *desc = "Minimum allowed QCE Id.";
    static constexpr uint32_t get() {
        return VTSS_APPL_QOS_QCE_ID_START;
    }
};

//s.qce_id_max
struct QosCapQceIdMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QceIdMax";
    static constexpr const char *desc = "Maximum allowed QCE Id.";
    static uint32_t get() {
        return VTSS_APPL_QOS_QCE_ID_END;
    }
};

//s.cosid_min
struct QosCapCosIdMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "CosIdMin";
    static constexpr const char *desc = "Minimum allowed COSID value.";
    static uint32_t get() { return CAPA->cosid_min; }
};

//s.cosid_max
struct QosCapCosIdMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "CosIdMax";
    static constexpr const char *desc = "Maximum allowed COSID value.";
    static uint32_t get() { return CAPA->cosid_max; }
};


//s.port_policer_bit_rate_min
struct QosCapPortPolicerBitRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerBitRateMin";
    static constexpr const char *desc = "Minimum supported bit rate in kbps for port policers using bit rate mode.";
    static uint32_t get() { return CAPA->port_policer_bit_rate_min; }
};

//s.port_policer_bit_rate_max
struct QosCapPortPolicerBitRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerBitRateMax";
    static constexpr const char *desc = "Maximum supported bit rate in kbps for port policers using bit rate mode.\n"
                            "If zero, the port policers does not support bit rate mode.";
    static uint32_t get() { return CAPA->port_policer_bit_rate_max; }
};

//s.port_policer_bit_burst_min
struct QosCapPortPolicerBitBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerBitBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in bytes for port policers using bit rate mode.";
    static uint32_t get() { return CAPA->port_policer_bit_burst_min; }
};

//s.port_policer_bit_burst_max
struct QosCapPortPolicerBitBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerBitBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in bytes for port policers using bit rate mode.";
    static uint32_t get() { return CAPA->port_policer_bit_burst_max; }
};

//s.port_policer_frame_rate_min
struct QosCapPortPolicerFrameRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerFrameRateMin";
    static constexpr const char *desc = "Minimum supported frame rate in fps for port policers using frame rate mode.";
    static uint32_t get() { return CAPA->port_policer_frame_rate_min; }
};

//s.port_policer_frame_rate_max
struct QosCapPortPolicerFrameRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerFrameRateMax";
    static constexpr const char *desc = "Maximum supported frame rate in fps for port policers using frame rate mode.\n"
                            "If zero, the port policers does not support frame rate mode.";
    static uint32_t get() { return CAPA->port_policer_frame_rate_max; }
};

//s.port_policer_frame_burst_min
struct QosCapPortPolicerFrameBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerFrameBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in frames for port policers using frame rate mode.";
    static uint32_t get() { return CAPA->port_policer_frame_burst_min; }
};

//s.port_policer_frame_burst_max
struct QosCapPortPolicerFrameBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortPolicerFrameBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in frames for port policers using frame rate mode.";
    static uint32_t get() { return CAPA->port_policer_frame_burst_max; }
};

//s.queue_policer_bit_rate_min
struct QosCapQueuePolicerBitRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerBitRateMin";
    static constexpr const char *desc = "Minimum supported bit rate in kbps for queue policers using bit rate mode.";
    static uint32_t get() { return CAPA->queue_policer_bit_rate_min; }
};

//s.queue_policer_bit_rate_max
struct QosCapQueuePolicerBitRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerBitRateMax";
    static constexpr const char *desc = "Maximum supported bit rate in kbps for queue policers using bit rate mode.\n"
                            "If zero, the queue policers does not support bit rate mode.";
    static uint32_t get() { return CAPA->queue_policer_bit_rate_max; }
};

//s.queue_policer_bit_burst_min
struct QosCapQueuePolicerBitBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerBitBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in bytes for queue policers using bit rate mode.";
    static uint32_t get() { return CAPA->queue_policer_bit_burst_min; }
};

//s.queue_policer_bit_burst_max
struct QosCapQueuePolicerBitBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerBitBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in bytes for queue policers using bit rate mode.";
    static uint32_t get() { return CAPA->queue_policer_bit_burst_max; }
};

//s.queue_policer_frame_rate_min
struct QosCapQueuePolicerFrameRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerFrameRateMin";
    static constexpr const char *desc = "Minimum supported frame rate in fps for queue policers using frame rate mode.";
    static uint32_t get() { return CAPA->queue_policer_frame_rate_min; }
};

//s.queue_policer_frame_rate_max
struct QosCapQueuePolicerFrameRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerFrameRateMax";
    static constexpr const char *desc = "Maximum supported frame rate in fps for queue policers using frame rate mode.\n"
                            "If zero, the port policers does not support frame rate mode.";
    static uint32_t get() { return CAPA->queue_policer_frame_rate_max; }
};

//s.queue_policer_frame_burst_min
struct QosCapQueuePolicerFrameBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerFrameBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in frames for queue policers using frame rate mode.";
    static uint32_t get() { return CAPA->queue_policer_frame_burst_min; }
};

//s.queue_policer_frame_burst_max
struct QosCapQueuePolicerFrameBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueuePolicerFrameBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in frames for queue policers using frame rate mode.";
    static uint32_t get() { return CAPA->queue_policer_frame_burst_max; }
};

//s.port_shaper_bit_rate_min
struct QosCapPortShaperBitRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperBitRateMin";
    static constexpr const char *desc = "Minimum supported bit rate in kbps for port shapers using bit rate mode.";
    static uint32_t get() { return CAPA->port_shaper_bit_rate_min; }
};

//s.port_shaper_bit_rate_max
struct QosCapPortShaperBitRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperBitRateMax";
    static constexpr const char *desc = "Maximum supported bit rate in kbps for port shapers using bit rate mode.\n"
                            "If zero, the port shapers does not support bit rate mode.";
    static uint32_t get() { return CAPA->port_shaper_bit_rate_max; }
};

//s.port_shaper_bit_burst_min
struct QosCapPortShaperBitBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperBitBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in bytes for port shapers using bit rate mode.";
    static uint32_t get() { return CAPA->port_shaper_bit_burst_min; }
};

//s.port_shaper_bit_burst_max
struct QosCapPortShaperBitBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperBitBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in bytes for port shapers using bit rate mode.";
    static uint32_t get() { return CAPA->port_shaper_bit_burst_max; }
};

//s.port_shaper_frame_rate_min
struct QosCapPortShaperFrameRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperFrameRateMin";
    static constexpr const char *desc = "Minimum supported frame rate in fps for port shapers using frame rate mode.";
    static uint32_t get() { return CAPA->port_shaper_frame_rate_min; }
};

//s.port_shaper_frame_rate_max
struct QosCapPortShaperFrameRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperFrameRateMax";
    static constexpr const char *desc = "Maximum supported frame rate in fps for port shapers using frame rate mode.\n"
                            "If zero, the port shapers does not support frame rate mode.";
    static uint32_t get() { return CAPA->port_shaper_frame_rate_max; }
};

//s.port_shaper_frame_burst_min
struct QosCapPortShaperFrameBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperFrameBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in frames for port shapers using frame rate mode.";
    static uint32_t get() { return CAPA->port_shaper_frame_burst_min; }
};

//s.port_shaper_frame_burst_max
struct QosCapPortShaperFrameBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortShaperFrameBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in frames for port shapers using frame rate mode.";
    static uint32_t get() { return CAPA->port_shaper_frame_burst_max; }
};

//s.queue_shaper_bit_rate_min
struct QosCapQueueShaperBitRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperBitRateMin";
    static constexpr const char *desc = "Minimum supported bit rate in kbps for queue shapers using bit rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_bit_rate_min; }
};

//s.queue_shaper_bit_rate_max
struct QosCapQueueShaperBitRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperBitRateMax";
    static constexpr const char *desc = "Maximum supported bit rate in kbps for queue shapers using bit rate mode.\n"
                            "If zero, the queue shapers does not support bit rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_bit_rate_max; }
};

//s.queue_shaper_bit_burst_min
struct QosCapQueueShaperBitBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperBitBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in bytes for queue shapers using bit rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_bit_burst_min; }
};

//s.queue_shaper_bit_burst_max
struct QosCapQueueShaperBitBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperBitBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in bytes for queue shapers using bit rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_bit_burst_max; }
};

//s.queue_shaper_frame_rate_min
struct QosCapQueueShaperFrameRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperFrameRateMin";
    static constexpr const char *desc = "Minimum supported frame rate in fps for queue shapers using frame rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_frame_rate_min; }
};

//s.queue_shaper_frame_rate_max
struct QosCapQueueShaperFrameRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperFrameRateMax";
    static constexpr const char *desc = "Maximum supported frame rate in fps for queue shapers using frame rate mode.\n"
                            "If zero, the port shapers does not support frame rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_frame_rate_max; }
};

//s.queue_shaper_frame_burst_min
struct QosCapQueueShaperFrameBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperFrameBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in frames for queue shapers using frame rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_frame_burst_min; }
};

//s.queue_shaper_frame_burst_max
struct QosCapQueueShaperFrameBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QueueShaperFrameBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in frames for queue shapers using frame rate mode.";
    static uint32_t get() { return CAPA->queue_shaper_frame_burst_max; }
};

//s.global_storm_bit_rate_min
struct QosCapGlobalStormBitRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormBitRateMin";
    static constexpr const char *desc = "Minimum supported bit rate in kbps for global storm policers using bit rate mode.";
    static uint32_t get() { return CAPA->global_storm_bit_rate_min; }
};

//s.global_storm_bit_rate_max
struct QosCapGlobalStormBitRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormBitRateMax";
    static constexpr const char *desc = "Maximum supported bit rate in kbps for global storm policers using bit rate mode.\n"
                            "If zero, the global storm policers does not support bit rate mode.";
    static uint32_t get() { return CAPA->global_storm_bit_rate_max; }
};

//s.global_storm_bit_burst_min
struct QosCapGlobalStormBitBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormBitBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in bytes for global storm policers using bit rate mode.";
    static uint32_t get() { return CAPA->global_storm_bit_burst_min; }
};

//s.global_storm_bit_burst_max
struct QosCapGlobalStormBitBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormBitBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in bytes for global storm policers using bit rate mode.";
    static uint32_t get() { return CAPA->global_storm_bit_burst_max; }
};

//s.global_storm_frame_rate_min
struct QosCapGlobalStormFrameRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormFrameRateMin";
    static constexpr const char *desc = "Minimum supported frame rate in fps for global storm policers using frame rate mode.";
    static uint32_t get() { return CAPA->global_storm_frame_rate_min; }
};

//s.global_storm_frame_rate_max
struct QosCapGlobalStormFrameRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormFrameRateMax";
    static constexpr const char *desc = "Maximum supported frame rate in fps for global storm policers using frame rate mode.\n"
                            "If zero, the global storm policers does not support frame rate mode.";
    static uint32_t get() { return CAPA->global_storm_frame_rate_max; }
};

//s.global_storm_frame_burst_min
struct QosCapGlobalStormFrameBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormFrameBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in frames for global storm policers using frame rate mode.";
    static uint32_t get() { return CAPA->global_storm_frame_burst_min; }
};

//s.global_storm_frame_burst_max
struct QosCapGlobalStormFrameBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "GlobalStormFrameBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in frames for global storm policers using frame rate mode.";
    static uint32_t get() { return CAPA->global_storm_frame_burst_max; }
};

//s.port_storm_bit_rate_min
struct QosCapPortStormBitRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormBitRateMin";
    static constexpr const char *desc = "Minimum supported bit rate in kbps for port storm policers using bit rate mode.";
    static uint32_t get() { return CAPA->port_storm_bit_rate_min; }
};

//s.port_storm_bit_rate_max
struct QosCapPortStormBitRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormBitRateMax";
    static constexpr const char *desc = "Maximum supported bit rate in kbps for port storm policers using bit rate mode.\n"
                            "If zero, the port storm policers does not support bit rate mode.";
    static uint32_t get() { return CAPA->port_storm_bit_rate_max; }
};

//s.port_storm_bit_burst_min
struct QosCapPortStormBitBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormBitBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in bytes for port storm policers using bit rate mode.";
    static uint32_t get() { return CAPA->port_storm_bit_burst_min; }
};

//s.port_storm_bit_burst_max
struct QosCapPortStormBitBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormBitBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in bytes for port storm policers using bit rate mode.";
    static uint32_t get() { return CAPA->port_storm_bit_burst_max; }
};

//s.port_storm_frame_rate_min
struct QosCapPortStormFrameRateMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormFrameRateMin";
    static constexpr const char *desc = "Minimum supported frame rate in fps for port storm policers using frame rate mode.";
    static uint32_t get() { return CAPA->port_storm_frame_rate_min; }
};

//s.port_storm_frame_rate_max
struct QosCapPortStormFrameRateMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormFrameRateMax";
    static constexpr const char *desc = "Maximum supported frame rate in fps for port storm policers using frame rate mode.\n"
                            "If zero, the port storm policers does not support frame rate mode.";
    static uint32_t get() { return CAPA->port_storm_frame_rate_max; }
};

//s.port_storm_frame_burst_min
struct QosCapPortStormFrameBurstMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormFrameBurstMin";
    static constexpr const char *desc = "Minimum supported burst size in frames for port storm policers using frame rate mode.";
    static uint32_t get() { return CAPA->port_storm_frame_burst_min; }
};

//s.port_storm_frame_burst_max,
struct QosCapPortStormFrameBurstMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "PortStormFrameBurstMax";
    static constexpr const char *desc = "Maximum supported burst size in frames for port storm policers using frame rate mode.";
    static uint32_t get() { return CAPA->port_storm_frame_burst_max; }
};

//s.ingress_map_id_min
struct QosCapIngressMapIdMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "IngressMapIdMin";
    static constexpr const char *desc = "Minimum supported index of Ingress map.";
    static uint32_t get() { return CAPA->ingress_map_id_min; }
};

//s.ingress_map_id_max
struct QosCapIngressMapIdMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "IngressMapIdMax";
    static constexpr const char *desc = "Maximum supported index of Ingress map.";
    static uint32_t get() { return CAPA->ingress_map_id_max; }
};

//s.egress_map_id_min
struct QosCapEgressMapIdMin {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "EgressMapIdMin";
    static constexpr const char *desc = "Minimum supported index of Egress map.";
    static uint32_t get() { return CAPA->egress_map_id_min; }
};

//s.egress_map_id_max
struct QosCapEgressMapIdMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "EgressMapIdMax";
    static constexpr const char *desc = "Maximum supported index of Egress map.";
    static uint32_t get() { return CAPA->egress_map_id_max; }
};

//s.qbv_gcl_len_max
struct QosCapQbvGclLenMax {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "QbvGclLenMax";
    static constexpr const char *desc = "Maximum supported length of gate control list.";
    static uint32_t get() {
        return fast_cap(MESA_CAP_QOS_TAS_GCE_CNT);
    }
};

//s.dwrr_cnt_mask
struct QosCapDwrrCountMask {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "DwrrCountMask";
    static constexpr const char *desc = "A bitmask used to show allowed values of DwrrCount (zero is always allowed).\n"
                                        "Example:\n"
                                        "  00100000 (0x20): Allowed values are 0 and 6\n"
                                        "  11111110 (0xFE): Allowed values are 0 and 2..8";
    static uint32_t get() { return CAPA->dwrr_cnt_mask; }
};


//s.has_global_storm_policers
struct QosCapHasGlobalStormPolicers {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasGlobalStormPolicers";
    static constexpr const char *desc = "If true, the platform supports global storm policers.";
    static constexpr bool get() {
        return true;
    }
};

//s.has_port_storm_policers
struct QosCapHasPortStormPolicers {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasPortStormPolicers";
    static constexpr const char *desc = "If true, the platform supports per port storm policers.";
    static bool get() { return CAPA->has_port_storm_policers; }
};

//s.has_port_queue_policers
struct QosCapHasPortQueuePolicers {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasPortQueuePolicers";
    static constexpr const char *desc = "If true, the platform supports per port queue policers.";
    static bool get() { return CAPA->has_port_queue_policers; }
};

//s.has_wred_v1
struct QosCapHasWredV1 {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasWredV1";
    static constexpr const char *desc = "If true, the platform supports WRED version 1.";
    static constexpr bool get() {
        return false;
    }
};

//s.has_wred_v2
struct QosCapHasWredV2 {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasWredV2";
    static constexpr const char *desc = "If true, the platform supports WRED version 2.";
    static bool get() { return CAPA->has_wred_v2; }
};

//s.has_fixed_tag_cos_map
struct QosCapHasFixedTagCosMap {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasFixedTagCosMap";
    static constexpr const char *desc = "If true, the platform supports fixed tag to cos mapping only.";
    static constexpr bool get() {
        return false;
    }
};

//s.has_tag_classification
struct QosCapHasTagClassification {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasTagClassification";
    static constexpr const char *desc = "If true, the platform supports using VLAN tag for classification.";
    static bool get() { return CAPA->has_tag_classification; }
};

//s.has_tag_remarking
struct QosCapHasTagRemarking {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasTagRemarking";
    static constexpr const char *desc = "If true, the platform supports VLAN tag remarking.";
    static bool get() { return CAPA->has_tag_remarking; }
};

//s.has_dscp
struct QosCapHasDscp {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasDscp";
    static constexpr const char *desc = "If true, the platform supports DSCP translation and remarking.";
    static bool get() { return CAPA->has_dscp; }
};

//s.has_dscp_dpl_class
struct QosCapHasDscpDplClassification {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasDscpDplClassification";
    static constexpr const char *desc = "If true, the platform supports DPL based DSCP classification.";
    static constexpr bool get() {
        return true;
    }
};

//s.has_dscp_dpl_remark
struct QosCapHasDscpDplRemarking {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasDscpDplRemarking";
    static constexpr const char *desc = "If true, the platform supports DPL based DSCP remarking.";
    static bool get() { return CAPA->has_dscp_dpl_remark; }
};

//s.has_port_policers_fc
struct QosCapHasPortPolicersFc {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasPortPolicersFc";
    static constexpr const char *desc = "If true, the platform supports flow control in port policers.";
    static constexpr bool get() {
        // A call to vtss_appl_port_capabilities_get().aggr_caps in qos.cxx will determine the runtime capability
        return true;
    }
};

//s.has_queue_policers_fc
struct QosCapHasQueuePolicersFc {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQueuePolicersFc";
    static constexpr const char *desc = "If true, the platform supports flow control in queue policers.";
    static constexpr bool get() {
        // currently always false
        return false;
    }
};

//s.has_port_shapers_dlb
struct QosCapHasPortShapersDlb {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasPortShapersDlb";
    static constexpr const char *desc = "If true, the platform supports dual leaky bucket egress port shapers.";
    static bool get() { return CAPA->has_port_shapers_dlb; }
};

//s.has_queue_shapers_dlb
struct QosCapHasQueueShapersDlb {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQueueShapersDlb";
    static constexpr const char *desc = "If true, the platform supports dual leaky bucket egress queue shapers.";
    static bool get() { return CAPA->has_queue_shapers_dlb; }
};

//s.has_queue_shapers_eb
struct QosCapHasQueueShapersExcess {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQueueShapersExcess";
    static constexpr const char *desc = "If true, the platform supports excess bandwidth in egress queue shapers.";
    static bool get() { return CAPA->has_queue_shapers_eb; }
};

//s.has_queue_shapers_crb
struct QosCapHasQueueShapersCredit {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQueueShapersCredit";
    static constexpr const char *desc = "If true, the platform supports bursty traffic in egress queue shapers.";
    static bool get() { return CAPA->has_queue_shapers_crb; }
};

//s.has_queue_cut_through
struct QosCapHasQueueCutThrough {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQueueCutThrough";
    static constexpr const char *desc = "If true, the platform supports cut through in egress queue.";
    static bool get() { return CAPA->has_queue_cut_through; }
};

//s.has_queue_frame_preemption
struct QosCapHasQueueFramePreemption {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQueueFramePreemption";
    static constexpr const char *desc = "If true, the platform supports frame preemption in egress queue.";
    static bool get() { return CAPA->has_queue_frame_preemption; }
};

//s.has_wred_v3
struct QosCapHasWredV3 {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasWredV3";
    static constexpr const char *desc = "If true, the platform supports WRED version 3.";
    static bool get() { return CAPA->has_wred_v3; }
};

//s.has_qce
struct QosCapHasQce {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQce";
    static constexpr const char *desc = "If true, the platform supports QCEs.";
    static bool get() { return CAPA->has_qce; }
};

//s.has_qce_address_mode
struct QosCapHasQceAddressMode {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceAddressMode";
    static constexpr const char *desc = "If true, the platform supports QCE address mode.";
    static bool get() { return CAPA->has_qce_address_mode; }
};

//s.has_qce_key_type
struct QosCapHasQceKeyType {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceKeyType";
    static constexpr const char *desc = "If true, the platform supports QCE key type.";
    static bool get() { return CAPA->has_qce_key_type; }
};

//s.has_qce_mac_oui
struct QosCapHasQceMacOui {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceMacOui";
    static constexpr const char *desc = "If true, the platform supports QCE MAC OUI part only (24 most significant bits).";
    static constexpr bool get() {
        return false;
    }
};

//s.has_qce_dmac
struct QosCapHasQceDmac {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceDmac";
    static constexpr const char *desc = "If true, the platform supports QCE destination MAC keys.";
    static bool get() { return CAPA->has_qce_dmac; }
};

//s.has_qce_dip
struct QosCapHasQceDip {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceDip";
    static constexpr const char *desc = "If true, the platform supports QCE destination IP keys.";
    static bool get() { return CAPA->has_qce_dip; }
};

//s.has_qce_ctag
struct QosCapHasQceCTag {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceCTag";
    static constexpr const char *desc = "If true, the platform supports QCE VLAN C-TAG keys.";
    static bool get() { return CAPA->has_qce; }
};

//s.has_qce_stag
struct QosCapHasQceSTag {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceSTag";
    static constexpr const char *desc = "If true, the platform supports QCE VLAN S-TAG keys.";
    static bool get() { return CAPA->has_qce; }
};

//s.has_qce_inner_tag
struct QosCapHasQceInnerTag {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceInnerTag";
    static constexpr const char *desc = "If true, the platform supports QCE inner VLAN tag keys.";
    static bool get() { return CAPA->has_qce_inner_tag; }
};

//s.has_qce_action_pcp_dei
struct QosCapHasQceActionPcpDei {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceActionPcpDei";
    static constexpr const char *desc = "If true, the platform supports QCE PCP and DEI actions.";
    static bool get() { return CAPA->has_qce; }
};

//s.has_qce_action_policy
struct QosCapHasQceActionPolicy {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceActionPolicy";
    static constexpr const char *desc = "If true, the platform supports QCE policy actions.";
    static bool get() { return CAPA->has_qce; }
};

//s.has_qce_action_map
struct QosCapHasQceActionMap {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQceActionMap";
    static constexpr const char *desc = "If true, the platform supports QCE ingress map actions.";
    static bool get() { return CAPA->has_ingress_map; }
};

//s.has_shapers_rt
struct QosCapHasShapersRt {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasShapersRt";
    static constexpr const char *desc = "If true, the platform supports rate-type configurable egress shapers.";
    static bool get() { return CAPA->has_shapers_rt; }
};

//s.has_ingress_map
struct QosCapHasIngressMap {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasIngressMap";
    static constexpr const char *desc = "If true, the platform supports Ingress mapping tables";
    static bool get() { return CAPA->has_ingress_map; }
};

//s.has_egress_map
struct QosCapHasEgressMap {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasEgressMap";
    static constexpr const char *desc = "If true, the platform supports Egress mapping tables";
    static bool get() { return CAPA->has_egress_map; }
};

// A derived capability, used to get appropriate dependency text
//s.has_wred2_or_wred3
struct QosCapHasWred2orWred3 {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasWred2orWredw3";
    static constexpr const char *desc = "This object is only valid if qosCapabilitiesHasWredV2 or qosCapabilitiesHasWredV3 is true.";
    static bool get() { return CAPA->has_wred2_or_wred3; }
};

// A derived capability, used to get appropriate dependency text
//s.has_dscp_dp2
struct QosCapHasDscpDp2 {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasDscpDp2";
    static constexpr const char *desc = "This object is only valid if qosCapabilitiesHasDscpDplClassification is true and qosCapabilitiesDplMax is greater than 1.";
    static bool get() { return CAPA->has_dscp_dp2; }
};

// A derived capability, used to get appropriate dependency text
//s.has_dscp_dp3
struct QosCapHasDscpDp3 {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasDscpDp3";
    static constexpr const char *desc = "This object is only valid if qosCapabilitiesHasDscpDplClassification is true and qosCapabilitiesDplMax is greater than 2.";
    static bool get() { return CAPA->has_dscp_dp3; }
};

// A derived capability, used to get appropriate dependency text
//s.has_default_pcp_and_dei
struct QosCapHasDefaultPcpAndDei {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasDefaultPcpAndDei";
    static constexpr const char *desc = "This object is only valid if qosCapabilitiesHasDefaultPcpAndDei is true.";
    static constexpr bool get() {
        return true;
    }
};

// A derived capability, used to get appropriate dependency text
//s.has_trust_tag
struct QosCapHasTrustTag {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasTrustTag";
    static constexpr const char *desc = "This object is only valid if qosCapabilitiesHasFixedTagCosMap is false and qosCapabilitiesHasTagClassification is true.";
    static bool get() { return CAPA->has_trust_tag; }
};

//s.has_qbv
struct QosCapHasQbv {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasQbv";
    static constexpr const char *desc = "If true, the platform supports Time Aware Shaper.";
    static bool get() { return CAPA->has_qbv; }
};

//s.has_cosid_classification
struct QosCapHasCosIdClassification {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasCosIdClassification";
    static constexpr const char *desc = "If true, the platform supports COSID classification.";
    static bool get() { return CAPA->has_cosid_classification; }
};

// A derived capability, used to ensure that wred tables only are defined if wred is supported.
// Update this if more versions of WRED are added
struct QosCapHasWred {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasWred";
    static constexpr const char *desc = "This object is only valid if the platform supports any form of Wred.";
    static bool get() { return CAPA->has_wred2_or_wred3; }
};

//s.has_psfp
struct QosCapHasPsfp {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasPsfp";
    static constexpr const char *desc = "If true, the platform supports Per stream Filtering and Policing.";
    static bool get() { return CAPA->has_psfp; }
};

//s.has_mpls_tc
struct QosCapHasMplsTc {
    static constexpr const char *json_ref = "vtss_appl_qos_capabilities_t";
    static constexpr const char *name = "HasTc";
    static constexpr const char *desc = "If true, the platform supports MPLS TC in ingress and egress maps (obsolete. Always false).";
    static bool get() { return CAPA->has_mpls_tc_obsolete; }
};

/****************************************************************************
* vtss_appl_qos_status_t
****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_status_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.storm_active),
               vtss::tag::Name("StormActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasGlobalStormPolicers>(),
               vtss::tag::Description("If true, the storm policer has been active in present time interval.\n"));

}

/****************************************************************************
 * vtss_appl_qos_global_storm_policer_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_global_storm_policer_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_global_storm_policer_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasGlobalStormPolicers>(),
               vtss::tag::Description("If true, the storm policer is enabled.\n"));

    m.add_leaf(s.rate,
               vtss::tag::Name("Rate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasGlobalStormPolicers>(),
               vtss::tag::Description("Configure the storm policer rate.\n"
                                      "Valid range is from the smallest value of qosCapabilitiesGlobalStormBitRateMin and qosCapabilitiesGlobalStormFrameRateMin "
                                      "to the largest value of qosCapabilitiesGlobalStormBitRateMax and qosCapabilitiesGlobalStormFrameRateMax.\n"));

    m.add_leaf(vtss::AsBool(s.frame_rate),
               vtss::tag::Name("FrameRate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasGlobalStormPolicers>(),
               vtss::tag::Description("If true, the rate is measured in fps instead of kbps.\n"
                                      "fps  (FrameRate == TRUE)  is only valid if qosCapabilitiesGlobalStormFrameRateMax is non-zero.\n"
                                      "kbps (FrameRate == FALSE) is only valid if qosCapabilitiesGlobalStormBitRateMax is non-zero.\n"));

}

/****************************************************************************
 * vtss_appl_qos_wred_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_wred_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_wred_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, RED is enabled for this queue."));

    m.add_leaf(vtss::AsPercent(s.min),
               vtss::tag::Name("MinimumFillLevel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls the lower RED fill level threshold in percent of the whole queue.\n"
                                      "If the queue filling level is below this threshold, the drop probability is zero.\n"
                                      "Valid range is 0-100."));

    m.add_leaf(vtss::AsPercent(s.max),
               vtss::tag::Name("Maximum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasWred2orWred3>(),
               vtss::tag::Description("Controls the upper RED drop probability or fill level threshold for frames marked with Drop Precedence Level 1 (yellow frames).\n"
                                      "Valid range is 1-100.\n"));

    m.add_leaf(s.max_unit,
               vtss::tag::Name("MaxSelector"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasWred2orWred3>(),
               vtss::tag::Description("Selects whether 'Maximum' controls 'Maximum Drop Probability' or 'Maximum Fill Level'.\n"));

    m.add_leaf(vtss::AsPercent(s.max_prob_1),
               vtss::tag::Name("MaximumDp1"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasWredV1>(),
               vtss::tag::Description("Controls the upper RED drop probability for frames marked with Drop Precedence Level 1 (yellow frames).\n"
                                      "Valid range is 0-100.\n"));

    m.add_leaf(vtss::AsPercent(s.max_prob_2),
               vtss::tag::Name("MaximumDp2"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasWredV1>(),
               vtss::tag::Description("Controls the upper RED drop probability for frames marked with Drop Precedence Level 2 (yellow frames).\n"
                                      "Valid range is 0-100.\n"));

    m.add_leaf(vtss::AsPercent(s.max_prob_3),
               vtss::tag::Name("MaximumDp3"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasWredV1>(),
               vtss::tag::Description("Controls the upper RED drop probability for frames marked with Drop Precedence Level 3 (yellow frames).\n"
                                      "Valid range is 0-100.\n"));
}

/****************************************************************************
 * vtss_appl_qos_dscp_entry_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_dscp_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_dscp_entry_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.trust),
               vtss::tag::Name("Trust"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, this DSCP value is trusted. Packets arriving on interfaces where 'TrustDscp' "
                                      "is enabled will be classified to the corresponding CoS and DPL in this table."));

    m.add_leaf(s.cos,
               vtss::tag::Name("Cos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CoS value which the DSCP value maps to.\n"
                                      "Valid range is qosCapabilitiesClassMin-qosCapabilitiesClassMax."));

    m.add_leaf(s.dpl,
               vtss::tag::Name("Dpl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DPL (Drop Precedence Level) which the DSCP value maps to.\n"
                                      "Valid range is qosCapabilitiesDplMin-qosCapabilitiesDplMax."));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp),
               vtss::tag::Name("IngressTranslation"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The new classified DSCP value which the original DSCP value is replaced with if 'DscpTranslate' is enabled on the interface"));

    m.add_leaf(vtss::AsBool(s.remark),
               vtss::tag::Name("Classify"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, then packets received on interfaces where 'DscpClassify' is set to 'selected' "
                                      "will be classified to a new DSCP value based on the values configured in the qosConfigGlobalsCosToDscpTable."));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp_remap),
               vtss::tag::Name("EgressTranslation"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The new DSCP value that will be written into the packet at egress if 'DscpRemark' on the interface is set to 'remap'"
                                      " or 'remapDp'.\n"
                                      "If 'remapDp' is set then only packets classified to Dpl 0 will use this table."));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp_remap_dp1),
               vtss::tag::Name("EgressTranslationDp1"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscpDplRemarking>(),
               vtss::tag::Description("The new DSCP value that will be written into the packet at egress if 'DscpRemark' on the interface is set to 'remapDp'.\n"
                                      "Only packets classified to Dpl 1 will use this table.\n"));
}

/****************************************************************************
 * vtss_appl_qos_cos_dscp_entry_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_cos_dscp_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_cos_dscp_entry_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp),
               vtss::tag::Name("Dscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If qosCapabilitiesHasDscpDplClassification is true: This is the DSCP value which the classified CoS value maps to when the classified DPL is 0.\n"
                                      "If qosCapabilitiesHasDscpDplClassification is false: This is the DSCP value which the classified CoS value maps to."));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp_dp1),
               vtss::tag::Name("DscpDp1"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscpDplClassification>(),
               vtss::tag::Description("The DSCP value which the classified CoS value maps to when the classified DPL is 1.\n"));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp_dp2),
               vtss::tag::Name("DscpDp2"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscpDp2>(),
               vtss::tag::Description("The DSCP value which the classified CoS value maps to when the classified DPL is 2.\n"));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp_dp3),
               vtss::tag::Name("DscpDp3"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscpDp3>(),
               vtss::tag::Description("The DSCP value which the classified CoS value maps to when the classified DPL is 3.\n"));
}

/****************************************************************************
 * vtss_appl_qos_port_conf_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_if_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_if_conf_t"));
    int ix = 0;

    m.add_leaf(s.default_cos,
               vtss::tag::Name("Cos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The default CoS value a packet is classified to.\n"
                                      "Valid range is qosCapabilitiesClassMin-qosCapabilitiesClassMax."));

    m.add_leaf(s.default_dpl,
               vtss::tag::Name("Dpl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The default DPL (Drop Precedence Level) a packet is classified to.\n"
                                      "Valid range is qosCapabilitiesDplMin-qosCapabilitiesDplMax."));

    m.add_leaf(s.default_pcp,
               vtss::tag::Name("Pcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDefaultPcpAndDei>(),
               vtss::tag::Description("The default PCP value a packet is classified to.\n"
                                      "Valid range is 0-7.\n"));

    m.add_leaf(s.default_dei,
               vtss::tag::Name("Dei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDefaultPcpAndDei>(),
               vtss::tag::Description("The default DEI value a packet is classified to.\n"
                                      "Valid range is 0-1.\n"));

    m.add_leaf(vtss::AsBool(s.trust_tag),
               vtss::tag::Name("TrustTag"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasTrustTag>(),
               vtss::tag::Description("If true and the packet is VLAN tagged, then the CoS and DPL assigned to the packet is the PCP and DEI "
                                      "value in the packet mapped to a CoS and DPL value defined in the qosConfigInterfaceTagToCosTable.\n"));

    m.add_leaf(vtss::AsBool(s.trust_dscp),
               vtss::tag::Name("TrustDscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscp>(),
               vtss::tag::Description("If true and the packet type is IPv4 or IPv6 and the packet DSCP value is trusted in qosConfigGlobalsDscpTable, "
                                      "then the CoS and DPL assigned to the packet is the DSCP "
                                      "value in the packet mapped to a CoS and DPL value defined in the qosConfigGlobalsDscpTable.\n"));

    m.add_leaf(s.dwrr_cnt,
               vtss::tag::Name("DwrrCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of queues that are running Deficit Weighted Round Robin scheduling.\n"
                                      "This value is restricted to 0 or one of the values defined by qosCapabilitiesDwrrCountMask."));

    m.add_leaf(s.tag_remark_mode,
               vtss::tag::Name("TagRemarkingMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasTagRemarking>(),
               vtss::tag::Description("Selects the tag remarking mode for egress. Valid values are:\n"
                                      "'classified' : Remark the tag with the classified values of PCP and DEI.\n"
                                      "'default' : Remark the tag with the configured TagPcp and TagDei.\n"
                                      "'mapped' : Remark the tag with the values from the classified CoS and DPL mapped through the qosConfigInterfaceCosToTagTable.\n"));

    m.add_leaf(s.tag_default_pcp,
               vtss::tag::Name("TagPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasTagRemarking>(),
               vtss::tag::Description("The PCP value to put in the packet if TagRemarkingMode is set to 'default'.\n"
                                      "Valid range is 0-7.\n"));

    m.add_leaf(s.tag_default_dei,
               vtss::tag::Name("TagDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasTagRemarking>(),
               vtss::tag::Description("The DEI value to put in the packet if TagRemarkingMode is set to 'default'.\n"
                                      "Valid range is 0-1.\n"));

    m.add_leaf(vtss::AsBool(s.dscp_translate),
               vtss::tag::Name("DscpTranslate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscp>(),
               vtss::tag::Description("If true then the classified DSCP value is the DSCP value in the packed mapped to a new DSCP value defined "
                                      "in qosConfigGlobalsDscpTable.IngressTranslation.\n"));

    m.add_leaf(s.dscp_imode,
               vtss::tag::Name("DscpClassify"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscp>(),
               vtss::tag::Description("If qosCapabilitiesHasDscpDplClassification is true: Selects if and how the classified DSCP is based on the classified CoS and DPL "
                                      "for IPv4 and IPv6 packets on ingress.\n"

                                      "If a packet is selected for being classified to a new DSCP, the classification in based on the classified CoS "
                                      "mapped to a new DSCP value through the qosConfigGlobalsCosToDscpTable. "
                                      "If the classified DPL is 1, the new DSCP value is taken from the DscpDp1 entry, otherwise it is taken from the Dscp entry. "

                                      "Valid values are:\n"

                                      "'none' : Always classify to the DSCP value in the received packet and ignore the classified CoS and DPL.\n"

                                      "'zero' : If the DSCP value in the received packet is 0, "
                                      "then classify to a new DSCP value based on the classified CoS and DPL.\n"

                                      "'selected' : If the DSCP value in the received packet is enabled in qosConfigGlobalsDscpTable.Classify, "
                                      "then classify to a new DSCP value based on the classified CoS and DPL.\n"

                                      "'all' : Always classify to a new DSCP value based on the classified CoS and DPL.\n\n"

                                      "If qosCapabilitiesHasDscpDplClassification is false: Selects if and how the classified DSCP is based on the classified CoS "
                                      "for IPv4 and IPv6 packets on ingress.\n"

                                      "If a packet is selected for being classified to a new DSCP, the classification in based on the classified CoS "
                                      "mapped to a new DSCP value through the qosConfigGlobalsCosToDscpTable.Dscp "

                                      "Valid values are:\n"

                                      "'none' : Always classify to the DSCP value in the received packet and ignore the classified CoS.\n"

                                      "'zero' : If the DSCP value in the received packet is 0, "
                                      "then classify to a new DSCP value based on the classified CoS.\n"

                                      "'selected' : If the DSCP value in the received packet is enabled in qosConfigGlobalsDscpTable.Classify, "
                                      "then classify to a new DSCP value based on the classified CoS.\n"

                                      "'all' : Always classify to a new DSCP value based on the classified CoS.\n"));

    m.add_leaf(s.dscp_emode,
               vtss::tag::Name("DscpRemark"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasDscp>(),
               vtss::tag::Description("If qosCapabilitiesHasDscpDplRemarking is true: Selects if and how the classified DSCP is written into the packet at egress. Valid values are:\n"
                                      "'disabled' : Do not write the classified DSCP into the packet.\n"
                                      "'rewrite' : Write the classified DSCP into the packet.\n"
                                      "'remap' : Remap the classified DSCP through qosConfigGlobalsDscpTable, and write the value from EgressTranslation into the packet.\n"
                                      "'remapDp' : Remap the classified DSCP through qosConfigGlobalsDscpTable. If the classified DPL is 0 then write the value from "
                                      "EgressTranslation into the packet, otherwise write the value from EgressTranslationDp1 into the packet.\n"

                                      "If qosCapabilitiesHasDscpDplRemarking is false: Selects if and how the classified DSCP is written into the packet at egress. Valid values are:\n"
                                      "'disabled' : Do not write the classified DSCP into the packet.\n"
                                      "'rewrite' : Write the classified DSCP into the packet.\n"
                                      "'remap' : Remap the classified DSCP through qosConfigGlobalsDscpTable, and write the value from EgressTranslation into the packet.\n"));

    m.add_leaf(vtss::AsBool(s.dmac_dip),
               vtss::tag::Name("QceAddressMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceAddressMode>(),
               vtss::tag::Description("If false, the QCE classification for this interface is based on source (SMAC/SIP) addresses.\n"
                                      "If true, the QCE classification for this interface is based on destination (DMAC/DIP) addresses.\n"

                                      "This parameter is only used when qosCapabilitiesHasQceKeyType is false or QceKeyType is normal.\n"));

    m.add_leaf(s.key_type,
               vtss::tag::Name("QceKeyType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceKeyType>(),
               vtss::tag::Description("The key type specifying the key generated for packets received on the interface. Valid values are:\n"
                                      "'normal' : Half key, match outer tag, SIP and SMAC.\n"
                                      "'doubleTag' : Quarter key, match inner and outer tag.\n"
                                      "'ipAddr' : Half key, match inner and outer tag, SIP and DIP. For non-IP frames, match outer tag only.\n"
                                      "'macIpAddr' : Full key, match inner and outer tag, SMAC, DMAC, SIP and DIP.\n"

                                      "Filtering on DMAC type (unicast/multicast/broadcast) is supported for any key type.\n"));

    m.add_leaf(s.wred_group,
               vtss::tag::Name("WredGroup"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasWredV3>(),
               vtss::tag::Description("The WRED group an interface is a member of.\n"));

    m.add_leaf(s.ingress_map,
               vtss::tag::Name("IngressMap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("The Ingress Map an interface is associated with for packet classification.\n"
                                      "Valid range is qosCapabilitiesIngressMapIdMin-qosCapabilitiesIngressMapIdMax and 4095 for NONE"));

    m.add_leaf(s.egress_map,
               vtss::tag::Name("EgressMap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("The Egress Map an interface is associated with for packet mapping.\n"
                                      "Valid range is qosCapabilitiesEgressMapIdMin-qosCapabilitiesEgressMapIdMax and 4095 for NONE"));

    m.add_leaf(s.default_cosid,
               vtss::tag::Name("CosId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasCosIdClassification>(),
               vtss::tag::Description("The default COSID value a packet is classified to.\n"
                                      "Valid range is qosCapabilitiesCosIdMin-qosCapabilitiesCosIdMax."));

}

/****************************************************************************
 * vtss_appl_qos_tag_cos_entry_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_tag_cos_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_tag_cos_entry_t"));
    int ix = 0;

    m.add_leaf(s.cos,
               vtss::tag::Name("Cos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CoS value which this entry maps to.\n"
                                      "Valid range is qosCapabilitiesClassMin-qosCapabilitiesClassMax."));

    m.add_leaf(s.dpl,
               vtss::tag::Name("Dpl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DPL (Drop Precedence Level) value which this entry maps to.\n"
                                      "Valid range is qosCapabilitiesDplMin-qosCapabilitiesDplMax."));
}

/****************************************************************************
 * vtss_appl_qos_cos_tag_entry_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_cos_tag_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_cos_tag_entry_t"));
    int ix = 0;

    m.add_leaf(s.pcp,
               vtss::tag::Name("Pcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The PCP value which this entry maps to.\n"
                                      "Valid range is 0-7."));

    m.add_leaf(s.dei,
               vtss::tag::Name("Dei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DEI value which this entry maps to.\n"
                                      "Valid range is 0-1."));
}

/****************************************************************************
 * vtss_appl_qos_port_policer_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_port_policer_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_port_policer_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the port policer is enabled."));

    m.add_leaf(vtss::AsBool(s.frame_rate),
               vtss::tag::Name("FrameRate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the rate is measured in fps instead of kbps."));

    m.add_leaf(vtss::AsBool(s.flow_control),
               vtss::tag::Name("FlowControl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasPortPolicersFc>(),
               vtss::tag::Description("If true, and flow control is enabled on the interface, "
                                      "then issue flow control pause frames instead of discarding frames.\n"));

    m.add_leaf(s.cir,
               vtss::tag::Name("Cir"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Committed Information Rate. Measured in kbps if FrameRate is false and fps if FrameRate is true.\n"
                                      "Valid range is from the smallest value of qosCapabilitiesPortPolicerBitRateMin and qosCapabilitiesPortPolicerFrameRateMin "
                                      "to the largest value of qosCapabilitiesPortPolicerBitRateMax and qosCapabilitiesPortPolicerFrameRateMax.\n"));

}
/****************************************************************************
 * vtss_appl_qos_queue_policer_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_queue_policer_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_queue_policer_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the queue policer is enabled."));

    ix++; // Reserved for FrameRate
    ix++; // Reserved for FlowControl

    m.add_leaf(s.cir,
               vtss::tag::Name("Cir"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Committed Information Rate. Measured in kbps.\n"
                                      "Valid range is from the smallest value of qosCapabilitiesQueuePolicerBitRateMin and qosCapabilitiesQueuePolicerFrameRateMin "
                                      "to the largest value of qosCapabilitiesQueuePolicerBitRateMax and qosCapabilitiesQueuePolicerFrameRateMax.\n"));
}

/****************************************************************************
 * vtss_appl_qos_port_shaper_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_port_shaper_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_port_shaper_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the port shaper is enabled."));

    m.add_leaf(s.cir,
               vtss::tag::Name("Cir"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Committed Information Rate. Measured in kbps.\n"
                                      "Valid range is from the smallest value of qosCapabilitiesPortShaperBitRateMin and qosCapabilitiesPortShaperFrameRateMin "
                                      "to the largest value of qosCapabilitiesPortShaperBitRateMax and qosCapabilitiesPortShaperFrameRateMax.\n"));

    m.add_leaf(s.mode,
               vtss::tag::Name("RateType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasShapersRt>(),
               vtss::tag::Description("Indicates the shaper's rate type.\n"
                                      "Valid selections are line (0) and data (1).\n"));
}

/****************************************************************************
 * vtss_appl_qos_queue_shaper_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_queue_shaper_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_queue_shaper_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the queue shaper is enabled."));

    m.add_leaf(vtss::AsBool(s.excess),
               vtss::tag::Name("Excess"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQueueShapersExcess>(),
               vtss::tag::Description("If true, then allow this queue to get a share of the excess bandwidth.\n"
                                      "Excess bandwidth is allowed if this shaper is closed and no "
                                      "other queues with open shapers have frames for transmission.\n"));

    m.add_leaf(s.cir,
               vtss::tag::Name("Cir"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Committed Information Rate. Measured in kbps.\n"
                                      "Valid range is from the smallest value of qosCapabilitiesQueueShaperBitRateMin and qosCapabilitiesQueueShaperFrameRateMin "
                                      "to the largest value of qosCapabilitiesQueueShaperBitRateMax and qosCapabilitiesQueueShaperFrameRateMax.\n"));

    m.add_leaf(s.mode,
               vtss::tag::Name("RateType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasShapersRt>(),
               vtss::tag::Description("Indicates the shaper's rate type.\n"
                                      "Valid selections are line (0), data (1) and frame (2).\n"
                                      "NOTE: This configuration is valid "
                                      "only for Normal port mode. If the port is in Basic or "
                                      "Hierarchical mode, the rate-type will be stored and "
                                      "become active when the port is switched back to "
                                      "Normal Scheduling mode.\n"));

    m.add_leaf(vtss::AsBool(s.credit),
               vtss::tag::Name("Credit"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQueueShapersCredit>(),
               vtss::tag::Description("If true, then this queue supports the credit based shaper.\n"
                                      "Creating burst capacity only when data is available."
                                      "\n"));

}

/****************************************************************************
 * vtss_appl_qos_scheduler_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_scheduler_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_scheduler_t"));
    int ix = 0;

    m.add_leaf(vtss::AsPercent(s.weight),
               vtss::tag::Name("Weight"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The weight for this queue.\n"
                                      "Valid range is 1-100."));


    m.add_leaf(vtss::AsBool(s.cut_through),
               vtss::tag::Name("CutThrough"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQueueCutThrough>(),
               vtss::tag::Description("If true, then this queue supports the cut through capability.\n"
                                      "Supported only when egress speed is lower or equal to ingress speed."));

    m.add_leaf(vtss::AsBool(s.frame_preemption),
               vtss::tag::Name("FramePreemption"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQueueFramePreemption>(),
               vtss::tag::Description("If true, then this queue supports frame preemption capability."));
}

/****************************************************************************
 * vtss_appl_qos_scheduler_status_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_scheduler_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_scheduler_status_t"));
    int ix = 0;

    m.add_leaf(vtss::AsPercent(s.weight),
               vtss::tag::Name("Weight"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The actual weight for this queue."));
}

/****************************************************************************
 * vtss_appl_qos_port_storm_policer_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_port_storm_policer_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_port_storm_policer_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasPortStormPolicers>(),
               vtss::tag::Description("If true, the storm policer is enabled.\n"));

    m.add_leaf(vtss::AsBool(s.frame_rate),
               vtss::tag::Name("FrameRate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasPortStormPolicers>(),
               vtss::tag::Description("If true, the rate is measured in fps instead of kbps.\n"));

    m.add_leaf(s.cir,
               vtss::tag::Name("Cir"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasPortStormPolicers>(),
               vtss::tag::Description("Committed Information Rate. Measured in kbps if FrameRate is false and fps if FrameRate is true.\n"
                                      "Valid range is from the smallest value of qosCapabilitiesPortStormBitRateMin and qosCapabilitiesPortStormFrameRateMin "
                                      "to the largest value of qosCapabilitiesPortStormBitRateMax and qosCapabilitiesPortStormFrameRateMax.\n"));
}

/****************************************************************************
 * vtss_appl_qos_if_status_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_if_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_if_status_t"));
    int ix = 0;
    m.add_leaf(s.default_cos,
               vtss::tag::Name("Cos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Some subsystems are allowed to modify the default CoS value for an interface. "
                                      "This object shows the actual default CoS value a packet is classified to."));
}

/****************************************************************************
 * vtss_appl_qos_qce_index_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_qce_index_t, mesa_qce_id_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("QceId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("QCE Id.\n"
                                      "Valid range is qosCapabilitiesQceIdMin-qosCapabilitiesQceIdMax or 0.\n"
                                      "Using a value of 0 when adding a new QCE, will assign the QCE Id in the new QCE to the first unused QCE Id."));
}

/****************************************************************************
 * vtss_appl_qos_qce_conf_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_qce_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_qce_conf_t"));
    int       ix     = 0;
    const int offset = 2; /* start OID */

    m.add_leaf(s.next_qce_id,
               vtss::tag::Name("NextQceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("QCE Id of the next QCE. Valid range is qosCapabilitiesQceIdMin-qosCapabilitiesQceIdMax or 0.\n"
                                      "The value 0, denotes the end of the list."));

    m.add_leaf(s.usid,
               vtss::tag::Name("SwitchId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the switch on which this QCE is active."
                                      "Valid range is documented in the Stacking MIB.\n"
                                      "This object is only valid on stacking platforms."),
               vtss::expose::snmp::PreGetCondition([](){
                       return false;
                   }));

    m.add_leaf((vtss::PortListStackable &)s.key.port_list,
               vtss::tag::Name("PortList"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("List of ports that are member of this QCE."));

    m.add_leaf(s.key.mac.dmac_type,
               vtss::tag::Name("DestMacType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the destination MAC type to match."));

    m.add_leaf(s.key.mac.dmac.value,
               vtss::tag::Name("DestMac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceDmac>(),
               vtss::tag::Description("Indicates the 48 bits destination MAC address.\n"
                                      "The packet's destination address is AND-ed with the value of DestMacMask and then compared against the value of this object.\n"
                                      "If this object value and the value of DestMacMask is 00-00-00-00-00-00, this entry matches any destination MAC address.\n"
                                      "This object can only be configured if DestMacType is any(0) and DestMacMask is ff-ff-ff-ff-ff-ff.\n"));

    m.add_leaf(s.key.mac.dmac.mask,
               vtss::tag::Name("DestMacMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceDmac>(),
               vtss::tag::Description("Indicates the 48 bits destination MAC address mask.\n"
                                      "Valid values are 00-00-00-00-00-00 or ff-ff-ff-ff-ff-ff.\n"));

    m.add_leaf(s.key.mac.smac.value,
               vtss::tag::Name("SrcMac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the 48 bits source MAC address.\n"
                                      "The packet's source address is AND-ed with the value of SrcMacMask and then compared against the value of this object.\n"
                                      "If this object value and the value of SrcMacMask is 00-00-00-00-00-00, this entry matches any source MAC address.\n"
                                      "This object can only be configured if SrcMacMask is ff-ff-ff-ff-ff-ff.\n"
                                      "If the value of qosCapabilitiesHasQceMacOui is true then only the OUI part of the MAC address (24 most significant bits) is used."));

    m.add_leaf(s.key.mac.smac.mask,
               vtss::tag::Name("SrcMacMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the 48 bits source MAC address mask.\n"
                                      "Valid values are 00-00-00-00-00-00 or ff-ff-ff-ff-ff-ff."));

    m.add_leaf(s.key.tag.tag_type,
               vtss::tag::Name("VlanTagType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the VLAN tag type to match.\n"
                                      "The value cTagged(3) is only supported if qosCapabilitiesHasQceCTag is true.\n"
                                      "The value sTagged(3) is only supported if qosCapabilitiesHasQceSTag is true."));

    m.add_leaf(s.key.tag.vid.match,
               vtss::tag::Name("VlanIdOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how a packets's VLAN ID is to be compared."));

    m.add_leaf(s.key.tag.vid.low,
               vtss::tag::Name("VlanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The VLAN ID to be compared.\n"
                                      "If the VlanIdOp object in the same row is range(2), this object will be the starting VLAN ID of the range.\n"
                                      "Valid range is 0-4095.\n"
                                      "This object can only be configured if VlanIdOp is not any(0)."));

    m.add_leaf(s.key.tag.vid.high,
               vtss::tag::Name("VlanIdRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The VLAN ID to be compared.\n"
                                      "If the VlanIdOp object in the same row is range(2), this object will be the ending VLAN ID of the range.\n"
                                      "Valid range is 0-4095.\n"
                                      "This object can only be configured if VlanIdOp is range(2)."));

    m.add_leaf(s.key.tag.pcp,
               vtss::tag::Name("Pcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The PCP value(s) to be compared."));

    m.add_leaf(s.key.tag.dei,
               vtss::tag::Name("Dei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DEI value to be compared.\n"
                                      "Valid range is 0-1."));

    m.add_leaf(s.key.inner_tag.tag_type,
               vtss::tag::Name("InnerVlanTagType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceInnerTag>(),
               vtss::tag::Description("Indicates the inner VLAN tag type to match.\n"));

    m.add_leaf(s.key.inner_tag.vid.match,
               vtss::tag::Name("InnerVlanIdOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceInnerTag>(),
               vtss::tag::Description("Indicates how a packets's inner VLAN ID is to be compared.\n"));

    m.add_leaf(s.key.inner_tag.vid.low,
               vtss::tag::Name("InnerVlanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceInnerTag>(),
               vtss::tag::Description("The inner VLAN ID to be compared.\n"
                                      "If the InnerVlanIdOp object in the same row is range(2), this object will be the starting VLAN ID of the range.\n"
                                      "Valid range is 0-4095.\n"
                                      "This object can only be configured if InnerVlanIdOp is not any(0).\n"));

    m.add_leaf(s.key.inner_tag.vid.high,
               vtss::tag::Name("InnerVlanIdRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceInnerTag>(),
               vtss::tag::Description("The inner VLAN ID to be compared.\n"
                                      "If the InnerVlanIdOp object in the same row is range(2), this object will be the ending VLAN ID of the range.\n"
                                      "Valid range is 0-4095.\n"
                                      "This object can only be configured if InnerVlanIdOp is range(2).\n"));

    m.add_leaf(s.key.inner_tag.pcp,
               vtss::tag::Name("InnerPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceInnerTag>(),
               vtss::tag::Description("The inner PCP value(s) to be compared.\n"));

    m.add_leaf(s.key.inner_tag.dei,
               vtss::tag::Name("InnerDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceInnerTag>(),
               vtss::tag::Description("The inner DEI value to be compared.\n"
                                      "Valid range is 0-1.\n"));

    ix = 100 - offset; /* Reserve some OIDs for additional common keys */

    m.add_leaf(s.key.type,
               vtss::tag::Name("FrameType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the packet's frame type.\n"
                                      "Modifying the frame type on an existing QCE will restore the content of all frame type dependent configuration to default."));

    m.add_leaf(vtss::AsEtherType(s.key.frame.etype.etype),
               vtss::tag::Name("Etype"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the packet's 16 bit Ethernet type field.\n"
                                      "Valid values are: 0(match any), 0x600-0xFFFF but excluding 0x800(IPv4) and 0x86DD(IPv6).\n"
                                      "This object can only be configured if FrameType is etype(1)."));

    ix = 200 - offset; /* Reserve some OIDs for additional EtherType keys such as payload and start LLC at 200 */

    m.add_leaf(s.key.frame.llc.dsap.value,
               vtss::tag::Name("LlcDsap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the packets's 8 bit LLC DSAP field.\n"
                                      "The packet's LLC DSAP field is AND-ed with the value of LlcDsapMask and then compared against the value of this object.\n"
                                      "If this object value and the value of LlcDsapMask is 0x00, this entry matches any LLC DSAP.\n"
                                      "Valid range is 0x00-0xFF.\n"
                                      "This object can only be configured if FrameType is llc(2)."));

    m.add_leaf(s.key.frame.llc.dsap.mask,
               vtss::tag::Name("LlcDsapMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the 8 bit LLC DSAP mask.\n"
                                      "Valid values are 0x00 or 0xFF.\n"
                                      "This object can only be configured if FrameType is llc(2)."));

    m.add_leaf(s.key.frame.llc.ssap.value,
               vtss::tag::Name("LlcSsap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the packets's 8 bit LLC SSAP field.\n"
                                      "The packet's LLC SSAP field is AND-ed with the value of LlcSsapMask and then compared against the value of this object.\n"
                                      "If this object value and the value of LlcSsapMask is 0x00, this entry matches any LLC SSAP.\n"
                                      "Valid range is 0x00-0xFF.\n"
                                      "This object can only be configured if FrameType is llc(2)."));

    m.add_leaf(s.key.frame.llc.ssap.mask,
               vtss::tag::Name("LlcSsapMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the 8 bit LLC SSAP mask.\n"
                                      "Valid values are 0x00 or 0xFF.\n"
                                      "This object can only be configured if FrameType is llc(2)."));

    m.add_leaf(s.key.frame.llc.control.value,
               vtss::tag::Name("LlcControl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the packets's 8 bit LLC Control field.\n"
                                      "The packet's LLC Control field is AND-ed with the value of LlcControlMask and then compared against the value of this object.\n"
                                      "If this object value and the value of LlcControlMask is 0x00, this entry matches any LLC Control.\n"
                                      "Valid range is 0x00-0xFF.\n"
                                      "This object can only be configured if FrameType is llc(2)."));

    m.add_leaf(s.key.frame.llc.control.mask,
               vtss::tag::Name("LlcControlMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the 8 bit LLC Control mask.\n"
                                      "Valid values are 0x00 or 0xFF.\n"
                                      "This object can only be configured if FrameType is llc(2)."));

    ix = 300 - offset; /* Reserve some OIDs for additional LLC keys such as payload and start SNAP at 300 */

    ix += 2; /* Reserve OIDs for future SnapOui and SnapOuiMask */

    m.add_leaf(s.key.frame.snap.pid.value,
               vtss::tag::Name("SnapPid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the packet's 16 bit SNAP PID field.\n"
                                      "The packet's SNAP PID field is AND-ed with the value of SnapPidMask and then compared against the value of this object.\n"
                                      "If this object value and the value of SnapPidMask is 0x0000, this entry matches any ethertype.\n"
                                      "This object can only be configured if FrameType is snap(3)."));

    m.add_leaf(s.key.frame.snap.pid.mask,
               vtss::tag::Name("SnapPidMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the 16 bit SNAP PID mask.\n"
                                      "Valid values are 0x0000 or 0xFFFF.\n"
                                      "This object can only be configured if FrameType is snap(3)."));

    ix = 400 - offset; /* Reserve some OIDs for additional SNAP keys such as payload  and start IPv4 at 400*/

    m.add_leaf(s.key.frame.ipv4.fragment,
               vtss::tag::Name("Ipv4Fragment"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how an IPv4 fragment bit is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(s.key.frame.ipv4.dscp.match,
               vtss::tag::Name("Ipv4DscpOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how an IPv4 DSCP field is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(vtss::AsDscp<mesa_vcap_vr_value_t>(s.key.frame.ipv4.dscp.low),
               vtss::tag::Name("Ipv4Dscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DSCP value to be compared.\n"
                                      "If the Ipv4DscpOp object in the same row is range(2), this object will be the starting DSCP value of the range.\n"
                                      "Valid range is 0-63.\n"
                                      "This object can only be configured if FrameType is ipv4(4) and Ipv4DscpOp is not any(0)."));

    m.add_leaf(vtss::AsDscp<mesa_vcap_vr_value_t>(s.key.frame.ipv4.dscp.high),
               vtss::tag::Name("Ipv4DscpRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DSCP value to be compared.\n"
                                      "If the Ipv4DscpOp object in the same row is range(2), this object will be the ending DSCP value of the range.\n"
                                      "Valid range is 0-63.\n"
                                      "This object can only be configured if FrameType is ipv4(4) and Ipv4DscpOp is range(2)."));

    m.add_leaf(s.key.frame.ipv4.proto.value,
               vtss::tag::Name("Ipv4Protocol"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The protocol number field in the IPv4 header used to indicate a higher layer protocol.\n"
                                      "The packet's protocol number field is AND-ed with the value of Ipv4ProtocolMask and then compared with the value of this object.\n"
                                      "If Ipv4Protocol and Ipv4protocolMask are 0, this entry matches any IPv4 protocol.\n"
                                      "Valid range is 0-255.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(s.key.frame.ipv4.proto.mask,
               vtss::tag::Name("Ipv4ProtocolMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified IPv4 protocol number mask.\n"
                                      "Valid values are 0 or 255.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(vtss::AsIpv4(s.key.frame.ipv4.sip.value),
               vtss::tag::Name("Ipv4SrcIp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified IPv4 source address.\n"
                                      "The packet's source address is AND-ed with the value of Ipv4SrcIpMask and then compared with the value of this object.\n"
                                      "If Ipv4SrcIP and Ipv4SrcIpMask are 0.0.0.0, this entry matches any IPv4 source address.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(vtss::AsIpv4(s.key.frame.ipv4.sip.mask),
               vtss::tag::Name("Ipv4SrcIpMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified IPv4 source address mask.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(vtss::AsIpv4(s.key.frame.ipv4.dip.value),
               vtss::tag::Name("Ipv4DestIp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceDip>(),
               vtss::tag::Description("The specified IPv4 destination address.\n"
                                      "The packet's destination address is AND-ed with the value of Ipv4DestIpMask and then compared with the value of this object.\n"
                                      "If Ipv4DestIP and Ipv4DestIpMask are 0.0.0.0, this entry matches any IPv4 destination address.\n"
                                      "This object can only be configured if FrameType is ipv4(4).\n"));

    m.add_leaf(vtss::AsIpv4(s.key.frame.ipv4.dip.mask),
               vtss::tag::Name("Ipv4DestIpMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceDip>(),
               vtss::tag::Description("The specified IPv4 destination address mask.\n"
                                      "This object can only be configured if FrameType is ipv4(4).\n"));

    m.add_leaf(s.key.frame.ipv4.sport.match,
               vtss::tag::Name("Ipv4SrcPortOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how a packet's source TCP/UDP port number is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(s.key.frame.ipv4.sport.low,
               vtss::tag::Name("Ipv4SrcPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The source port number of the TCP or UDP protocol.\n"
                                      "If the Ipv4SrcPortOp object in the same row is range(2), this object will be the starting port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv4(4) and Ipv4SrcPortOp is not any(0)."));

    m.add_leaf(s.key.frame.ipv4.sport.high,
               vtss::tag::Name("Ipv4SrcPortRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The source port number of the TCP or UDP protocol.\n"
                                      "If the Ipv4SrcPortOp object in the same row is range(2), this object will be the ending port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv4(4) and Ipv4SrcPortOp is range(2)."));

    m.add_leaf(s.key.frame.ipv4.dport.match,
               vtss::tag::Name("Ipv4DestPortOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how a packet's destination TCP/UDP port number is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv4(4)."));

    m.add_leaf(s.key.frame.ipv4.dport.low,
               vtss::tag::Name("Ipv4DestPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The destination port number of the TCP or UDP protocol.\n"
                                      "If the Ipv4DestPortOp object in the same row is range(2), this object will be the starting port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv4(4) and Ipv4DestPortOp is not any(0)."));

    m.add_leaf(s.key.frame.ipv4.dport.high,
               vtss::tag::Name("Ipv4DestPortRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The source port number of the TCP or UDP protocol.\n"
                                      "If the Ipv4DestPortOp object in the same row is range(2), this object will be the ending port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv4(4) and Ipv4DestPortOp is range(2)."));

    ix = 500 - offset; /* Reserve some OIDs for additional IPv4 keys and start IPv6 at 500 */

    m.add_leaf(s.key.frame.ipv6.dscp.match,
               vtss::tag::Name("Ipv6DscpOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how an IPv6 DSCP field is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    m.add_leaf(vtss::AsDscp<mesa_vcap_vr_value_t>(s.key.frame.ipv6.dscp.low),
               vtss::tag::Name("Ipv6Dscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DSCP value to be compared.\n"
                                      "If the Ipv6DscpOp object in the same row is range(2), this object will be the starting DSCP value of the range.\n"
                                      "Valid range is 0-63.\n"
                                      "This object can only be configured if FrameType is ipv6(5) and Ipv6DscpOp is not any(0)."));

    m.add_leaf(vtss::AsDscp<mesa_vcap_vr_value_t>(s.key.frame.ipv6.dscp.high),
               vtss::tag::Name("Ipv6DscpRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DSCP value to be compared.\n"
                                      "If the Ipv6DscpOp object in the same row is range(2), this object will be the ending DSCP value of the range.\n"
                                      "Valid range is 0-63.\n"
                                      "This object can only be configured if FrameType is ipv6(5) and Ipv6DscpOp is range(2)."));

    m.add_leaf(s.key.frame.ipv6.proto.value,
               vtss::tag::Name("Ipv6Protocol"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The protocol number field in the IPv6 header used to indicate a higher layer protocol.\n"
                                      "The packet's protocol number field is AND-ed with the value of Ipv6ProtocolMask and then compared with the value of this object.\n"
                                      "If Ipv46rotocol and Ipv6protocolMask are 0, this entry matches any IPv6 protocol.\n"
                                      "Valid range is 0-255.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    m.add_leaf(s.key.frame.ipv6.proto.mask,
               vtss::tag::Name("Ipv6ProtocolMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified IPv6 protocol number mask.\n"
                                      "Valid values are 0 or 255.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    ix++; /* Reserve OID for InetAddressType */
    m.add_leaf(s.key.frame.ipv6.sip.value,
               vtss::tag::Name("Ipv6SrcIp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified IPv6 source address.\n"
                                      "The packet's source address is AND-ed with the value of Ipv6SrcIpMask and then compared with the value of this object.\n"
                                      "If Ipv6SrcIP and Ipv6SrcIpMask are all zeros, this entry matches any IPv6 source address.\n"
                                      "Only the least significant 32 bits of this object are used.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    ix++; /* Reserve OID for InetAddressType */
    m.add_leaf(s.key.frame.ipv6.sip.mask,
               vtss::tag::Name("Ipv6SrcIpMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified IPv6 source address mask.\n"
                                      "Only the least significant 32 bits of this object are used.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    ix++; /* Reserve OID for InetAddressType */
    m.add_leaf(s.key.frame.ipv6.dip.value,
               vtss::tag::Name("Ipv6DestIp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceDip>(),
               vtss::tag::Description("The specified IPv6 destination address.\n"
                                      "The packet's destination address is AND-ed with the value of Ipv6DestIpMask and then compared with the value of this object.\n"
                                      "If Ipv6DestIP and Ipv6DestIpMask are all zeros, this entry matches any IPv6 destination address.\n"
                                      "Only the least significant 32 bits of this object are used.\n"
                                      "This object can only be configured if FrameType is ipv6(5).\n"));

    ix++; /* Reserve OID for InetAddressType */
    m.add_leaf(s.key.frame.ipv6.dip.mask,
               vtss::tag::Name("Ipv6DestIpMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceDip>(),
               vtss::tag::Description("The specified IPv6 destination address mask.\n"
                                      "Only the least significant 32 bits of this object are used.\n"
                                      "This object can only be configured if FrameType is ipv6(5).\n"));

    m.add_leaf(s.key.frame.ipv6.sport.match,
               vtss::tag::Name("Ipv6SrcPortOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how a packet's source TCP/UDP port number is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    m.add_leaf(s.key.frame.ipv6.sport.low,
               vtss::tag::Name("Ipv6SrcPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The source port number of the TCP or UDP protocol.\n"
                                      "If the Ipv6SrcPortOp object in the same row is range(2), this object will be the starting port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv6(5) and Ipv6SrcPortOp is not any(0)."));

    m.add_leaf(s.key.frame.ipv6.sport.high,
               vtss::tag::Name("Ipv6SrcPortRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The source port number of the TCP or UDP protocol.\n"
                                      "If the Ipv6SrcPortOp object in the same row is range(2), this object will be the ending port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv6(5) and Ipv6SrcPortOp is range(2)."));

    m.add_leaf(s.key.frame.ipv6.dport.match,
               vtss::tag::Name("Ipv6DestPortOp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates how a packet's destination TCP/UDP port number is to be compared.\n"
                                      "This object can only be configured if FrameType is ipv6(5)."));

    m.add_leaf(s.key.frame.ipv6.dport.low,
               vtss::tag::Name("Ipv6DestPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The destination port number of the TCP or UDP protocol.\n"
                                      "If the Ipv6DestPortOp object in the same row is range(2), this object will be the starting port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv6(5) and Ipv6DestPortOp is not any(0)."));

    m.add_leaf(s.key.frame.ipv6.dport.high,
               vtss::tag::Name("Ipv6DestPortRange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The source port number of the TCP or UDP protocol.\n"
                                      "If the Ipv6DestPortOp object in the same row is range(2), this object will be the ending port number of the port range.\n"
                                      "Valid range is 0-65535.\n"
                                      "This object can only be configured if FrameType is ipv6(5) and Ipv6DestPortOp is range(2)."));

    ix = 1000 - offset; /* Reserve some OIDs for additional IPv6 keys and future frame types and start actions at 1000 */

    m.add_leaf(vtss::AsBool(s.action.prio_enable),
               vtss::tag::Name("ActionCosEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the packet is classified to the CoS value in ActionCos."));

    m.add_leaf(s.action.prio,
               vtss::tag::Name("ActionCos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CoS value used for classification.\n"
                                      "Valid range is qosCapabilitiesClassMin-qosCapabilitiesClassMax."));

    m.add_leaf(vtss::AsBool(s.action.dp_enable),
               vtss::tag::Name("ActionDplEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the packet is classified to the DPL value in ActionDpl."));

    m.add_leaf(s.action.dp,
               vtss::tag::Name("ActionDpl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DPL value used for classification.\n"
                                      "Valid range is qosCapabilitiesDplMin-qosCapabilitiesDplMax."));

    m.add_leaf(vtss::AsBool(s.action.dscp_enable),
               vtss::tag::Name("ActionDscpEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, the packet is classified to the DSCP value in ActionDscp."));

    m.add_leaf(s.action.dscp,
               vtss::tag::Name("ActionDscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DSCP value used for classification.\n"
                                      "Valid range is 0-63."));

    m.add_leaf(vtss::AsBool(s.action.pcp_dei_enable),
               vtss::tag::Name("ActionPcpDeiEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionPcpDei>(),
               vtss::tag::Description("If true, the packet is classified to the PCP value in ActionPcp and the DEI value in ActionDei.\n"));

    m.add_leaf(s.action.pcp,
               vtss::tag::Name("ActionPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionPcpDei>(),
               vtss::tag::Description("The PCP value used for classification.\n"
                                      "Valid range is 0-7.\n"));

    m.add_leaf(s.action.dei,
               vtss::tag::Name("ActionDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionPcpDei>(),
               vtss::tag::Description("The DEI value used for classification.\n"
                                      "Valid range is 0-1.\n"));

    m.add_leaf(vtss::AsBool(s.action.policy_no_enable),
               vtss::tag::Name("ActionPolicyEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionPolicy>(),
               vtss::tag::Description("If true, the packet is classified to the policy number in ActionPolicy.\n"));

    m.add_leaf(s.action.policy_no,
               vtss::tag::Name("ActionPolicy"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionPolicy>(),
               vtss::tag::Description("The policy number used for classification.\n"
                                      "Valid range is documented in the ACL MIB.\n"));

    m.add_leaf(vtss::AsBool(s.action.map_id_enable),
               vtss::tag::Name("ActionMapEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionMap>(),
               vtss::tag::Description("If true, the packet is classified by the Ingress Map Id in ActionMap.\n"));

    m.add_leaf(s.action.map_id,
               vtss::tag::Name("ActionMap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasQceActionMap>(),
               vtss::tag::Description("The Id of an Ingress Map used for classification.\n"
                                      "Valid range is qosCapabilitiesIngressMapIdMin-qosCapabilitiesIngressMapIdMax.\n"));

}

/****************************************************************************
 * vtss_appl_qos_usid_index_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_usid_index_t, vtss_usid_t, a, s) {
    a.add_leaf(s.inner,
               vtss::tag::Name("SwitchId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Switch Id index."));
}

/****************************************************************************
 * vtss_appl_qos_qce_precedence_index_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_qce_precedence_index_t, u32, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Index"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("QCE precedence index. Indicates the position in QCL."));
}

/****************************************************************************
 * vtss_appl_qos_qce_precedence_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_qce_precedence_t, vtss_appl_qos_qce_conf_t, a, s) {
    int ix = 0;

    a.add_leaf(s.inner.qce_id,
               vtss::tag::Name("QceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("QCE Id."));
}

/****************************************************************************
 * vtss_appl_qos_qce_status_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_qce_status_t, vtss_appl_qos_qce_conf_t, a, s) {
    int ix = 0;
    const int offset = 3; /* start OID */

    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_qos_qce_status_t"));

    m.add_leaf(s.inner.user_id,
               vtss::tag::Name("UserId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("User Id."));

    m.add_leaf(s.inner.qce_id,
               vtss::tag::Name("QceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("QCE Id."));


    ix = 100 - offset; /* Reserve some OIDs for more information */

    m.add_leaf(vtss::AsBool(s.inner.conflict),
               vtss::tag::Name("Conflict"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If true, a resource conflict is detected."));

}

/****************************************************************************
 * vtss_appl_qos_qce_conflict_resolve_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_qce_conflict_resolve_t, BOOL, a, s) {
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("ConflictResolve"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::DependOnCapability<QosCapHasQce>(),
               vtss::tag::Description("Set to true to resolve resource conflicts.\n"
                                      "If different components competes for the same resources in the switch, "
                                      "it is possible that some of resources needed by one or more QCE are not available.\n"
                                      "These conflicts, if any, are shown in the qosStatusQceTable.\n"
                                      "To solve these conflicts, you must delete one or more of the competing settings and set ConflictResolve to true. "
                                      "The switch will then reapply the current QCE configuration.\n"));
}

/****************************************************************************
 * vtss_appl_qos_ingress_map_index_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_ingress_map_index_t, mesa_qos_ingress_map_id_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("IngressMapId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Ingress Map Id.\n"
                                      "Valid range is qosCapabilitiesIngressMapIdMin-qosCapabilitiesIngressMapIdMax.\n"));
}

/****************************************************************************
 * vtss_appl_qos_ingress_map_conf_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_ingress_map_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_ingress_map_conf_t"));
    int       ix     = 0;

    m.add_leaf(s.key,
               vtss::tag::Name("Key"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("An integer that indicates the key to use when matching frames in the ingress map."
                                      " If the value is pcp(0), use classified PCP value as key (default).\n"
                                      " If the value is pcpDei(1), use classified PCP and DEI values as key.\n"
                                      " If the value is dscp(2), use the frame's DSCP value as key. For non-IP frames, no mapping is done.\n"
                                      " If the value is dscpPcpDei(3), use the frame's DSCP value as key. For non-IP frames, use classified PCP and DEI values as key."));

    m.add_leaf(vtss::AsBool(s.action.cos),
               vtss::tag::Name("ActionCos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of CoS."));

    m.add_leaf(vtss::AsBool(s.action.dpl),
               vtss::tag::Name("ActionDpl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of DPL."));

    m.add_leaf(vtss::AsBool(s.action.pcp),
               vtss::tag::Name("ActionPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of PCP."));

    m.add_leaf(vtss::AsBool(s.action.dei),
               vtss::tag::Name("ActionDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of DEI."));

    m.add_leaf(vtss::AsBool(s.action.dscp),
               vtss::tag::Name("ActionDscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of DSCP."));

    m.add_leaf(vtss::AsBool(s.action.cosid),
               vtss::tag::Name("ActionCosid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of COSID."));

    m.add_leaf(false,
               vtss::tag::Name("ActionPath"),
               vtss::expose::snmp::Status::Obsoleted,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("If true, enable classification of PATH-COSID (obsolete)."));
}

template<typename T>
void serialize(T &a, vtss_appl_qos_ingress_map_values_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_ingress_map_values_t"));
    int       ix     = 0;

    m.add_leaf(s.cos,
               vtss::tag::Name("ToCos"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped CoS value.\n"
                                      "Valid range is qosCapabilitiesClassMin-qosCapabilitiesClassMax."));

    m.add_leaf(s.dpl,
               vtss::tag::Name("ToDpl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped DPL (Drop Precedence Level) value.\n"
                                      "Valid range is qosCapabilitiesDplMin-qosCapabilitiesDplMax."));

    m.add_leaf(s.pcp,
               vtss::tag::Name("ToPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped PCP value.\n"
                                      "Valid range is 0-7.\n"));

    m.add_leaf(s.dei,
               vtss::tag::Name("ToDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped DEI value.\n"
                                      "Valid range is 0-1.\n"));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp),
               vtss::tag::Name("ToDscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped DSCP value.\n"
                                      "Valid range is 0-63.\n"));

    m.add_leaf(s.cosid,
               vtss::tag::Name("ToCosid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped COSID value.\n"
                                      "Valid range is qosCapabilitiesCosIdMin-qosCapabilitiesCosIdMax.\n"));

    m.add_leaf((mesa_cosid_t)0,
               vtss::tag::Name("ToPathCosid"),
               vtss::expose::snmp::Status::Obsoleted,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasIngressMap>(),
               vtss::tag::Description("Setup the mapped PATH-COSID value.\n"
                                       "Valid range is qosCapabilitiesCosIdMin-qosCapabilitiesCosIdMax (obsolete).\n"));
}

/****************************************************************************
 * vtss_appl_qos_egress_map_index_t
 ****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_egress_map_index_t, mesa_qos_egress_map_id_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("EgressMapId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Egress Map Id.\n"
                                      "Valid range is qosCapabilitiesEgressMapIdMin-qosCapabilitiesEgressMapIdMax.\n"));
}

/****************************************************************************
 * vtss_appl_qos_egress_map_conf_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_qos_egress_map_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_egress_map_conf_t"));
    int       ix     = 0;

    m.add_leaf(s.key,
                vtss::tag::Name("Key"),
                vtss::expose::snmp::Status::Current,
                vtss::expose::snmp::OidElementValue(ix++),
                vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
                vtss::tag::Description("An integer that indicates the key to use when matching frames in the egress map.\n"
                        "If the value is cosid(0), use classified COSID.\n"
                        "If the value is cosidDpl(1), use classified COSID and DPL.\n"
                        "If the value is dscp(2), use classified DSCP.\n"
                        "If the value is dscpDpl(3), use classified DSCP and DPL."));

    m.add_leaf(vtss::AsBool(s.action.pcp),
               vtss::tag::Name("ActionPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("If true, enable rewriting of PCP."));

    m.add_leaf(vtss::AsBool(s.action.dei),
               vtss::tag::Name("ActionDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("If true, enable rewriting of DEI."));

    m.add_leaf(vtss::AsBool(s.action.dscp),
               vtss::tag::Name("ActionDscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("If true, enable rewriting of DSCP."));

    m.add_leaf(false,
               vtss::tag::Name("ActionPath"),
               vtss::expose::snmp::Status::Obsoleted,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("If true, enable rewriting of PATH-COSID (obsolete)."));
}

template<typename T>
void serialize(T &a, vtss_appl_qos_egress_map_values_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_qos_egress_map_values_t"));
    int       ix     = 0;

    m.add_leaf(s.pcp,
               vtss::tag::Name("ToPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("Setup the mapped PCP value.\n"
                                      "Valid range is 0-7.\n"));

    m.add_leaf(s.dei,
               vtss::tag::Name("ToDei"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("Setup the mapped DEI value.\n"
                                      "Valid range is 0-1.\n"));

    m.add_leaf(vtss::AsDscp<mesa_dscp_t>(s.dscp),
               vtss::tag::Name("ToDscp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("Setup the mapped DSCP value.\n"
                                      "Valid range is 0-63.\n"));

    m.add_leaf((mesa_cosid_t)0,
               vtss::tag::Name("ToPathCosid"),
               vtss::expose::snmp::Status::Obsoleted,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::DependOnCapability<QosCapHasEgressMap>(),
               vtss::tag::Description("Setup the mapped PATH-COSID value.\n"
                                      "Valid range is qosCapabilitiesCosIdMin-qosCapabilitiesCosIdMax (obsolete).\n"));
}

namespace vtss {
namespace appl {
namespace qos {
namespace interfaces {
struct QosCapabilities {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_qos_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_qos_capabilities_t &s) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));

        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_qos_capabilities_t"));

        /* Numeric capabilities */
        m.template capability<QosCapClassMin>(vtss::expose::snmp::OidElementValue(1));
        m.template capability<QosCapClassMax>(vtss::expose::snmp::OidElementValue(2));
        m.template capability<QosCapDplMin>(vtss::expose::snmp::OidElementValue(3));
        m.template capability<QosCapDplMax>(vtss::expose::snmp::OidElementValue(4));
        m.template capability<QosCapWredGroupMin>(vtss::expose::snmp::OidElementValue(5));
        m.template capability<QosCapWredGroupMax>(vtss::expose::snmp::OidElementValue(6));
        m.template capability<QosCapWredDplMin>(vtss::expose::snmp::OidElementValue(7));
        m.template capability<QosCapWredDplMax>(vtss::expose::snmp::OidElementValue(8));
        m.template capability<QosCapQceIdMin>(vtss::expose::snmp::OidElementValue(9));
        m.template capability<QosCapQceIdMax>(vtss::expose::snmp::OidElementValue(10));
        m.template capability<QosCapPortPolicerBitRateMin>(vtss::expose::snmp::OidElementValue(11));
        m.template capability<QosCapPortPolicerBitRateMax>(vtss::expose::snmp::OidElementValue(12));
        m.template capability<QosCapPortPolicerBitBurstMin>(vtss::expose::snmp::OidElementValue(13));
        m.template capability<QosCapPortPolicerBitBurstMax>(vtss::expose::snmp::OidElementValue(14));
        m.template capability<QosCapPortPolicerFrameRateMin>(vtss::expose::snmp::OidElementValue(15));
        m.template capability<QosCapPortPolicerFrameRateMax>(vtss::expose::snmp::OidElementValue(16));
        m.template capability<QosCapPortPolicerFrameBurstMin>(vtss::expose::snmp::OidElementValue(17));
        m.template capability<QosCapPortPolicerFrameBurstMax>(vtss::expose::snmp::OidElementValue(18));
        m.template capability<QosCapQueuePolicerBitRateMin>(vtss::expose::snmp::OidElementValue(19));
        m.template capability<QosCapQueuePolicerBitRateMax>(vtss::expose::snmp::OidElementValue(20));
        m.template capability<QosCapQueuePolicerBitBurstMin>(vtss::expose::snmp::OidElementValue(21));
        m.template capability<QosCapQueuePolicerBitBurstMax>(vtss::expose::snmp::OidElementValue(22));
        m.template capability<QosCapQueuePolicerFrameRateMin>(vtss::expose::snmp::OidElementValue(23));
        m.template capability<QosCapQueuePolicerFrameRateMax>(vtss::expose::snmp::OidElementValue(24));
        m.template capability<QosCapQueuePolicerFrameBurstMin>(vtss::expose::snmp::OidElementValue(25));
        m.template capability<QosCapQueuePolicerFrameBurstMax>(vtss::expose::snmp::OidElementValue(26));
        m.template capability<QosCapPortShaperBitRateMin>(vtss::expose::snmp::OidElementValue(27));
        m.template capability<QosCapPortShaperBitRateMax>(vtss::expose::snmp::OidElementValue(28));
        m.template capability<QosCapPortShaperBitBurstMin>(vtss::expose::snmp::OidElementValue(29));
        m.template capability<QosCapPortShaperBitBurstMax>(vtss::expose::snmp::OidElementValue(30));
        m.template capability<QosCapPortShaperFrameRateMin>(vtss::expose::snmp::OidElementValue(31));
        m.template capability<QosCapPortShaperFrameRateMax>(vtss::expose::snmp::OidElementValue(32));
        m.template capability<QosCapPortShaperFrameBurstMin>(vtss::expose::snmp::OidElementValue(33));
        m.template capability<QosCapPortShaperFrameBurstMax>(vtss::expose::snmp::OidElementValue(34));
        m.template capability<QosCapQueueShaperBitRateMin>(vtss::expose::snmp::OidElementValue(35));
        m.template capability<QosCapQueueShaperBitRateMax>(vtss::expose::snmp::OidElementValue(36));
        m.template capability<QosCapQueueShaperBitBurstMin>(vtss::expose::snmp::OidElementValue(37));
        m.template capability<QosCapQueueShaperBitBurstMax>(vtss::expose::snmp::OidElementValue(38));
        m.template capability<QosCapQueueShaperFrameRateMin>(vtss::expose::snmp::OidElementValue(39));
        m.template capability<QosCapQueueShaperFrameRateMax>(vtss::expose::snmp::OidElementValue(40));
        m.template capability<QosCapQueueShaperFrameBurstMin>(vtss::expose::snmp::OidElementValue(41));
        m.template capability<QosCapQueueShaperFrameBurstMax>(vtss::expose::snmp::OidElementValue(42));
        m.template capability<QosCapGlobalStormBitRateMin>(vtss::expose::snmp::OidElementValue(43));
        m.template capability<QosCapGlobalStormBitRateMax>(vtss::expose::snmp::OidElementValue(44));
        m.template capability<QosCapGlobalStormBitBurstMin>(vtss::expose::snmp::OidElementValue(45));
        m.template capability<QosCapGlobalStormBitBurstMax>(vtss::expose::snmp::OidElementValue(46));
        m.template capability<QosCapGlobalStormFrameRateMin>(vtss::expose::snmp::OidElementValue(47));
        m.template capability<QosCapGlobalStormFrameRateMax>(vtss::expose::snmp::OidElementValue(48));
        m.template capability<QosCapGlobalStormFrameBurstMin>(vtss::expose::snmp::OidElementValue(49));
        m.template capability<QosCapGlobalStormFrameBurstMax>(vtss::expose::snmp::OidElementValue(50));
        m.template capability<QosCapPortStormBitRateMin>(vtss::expose::snmp::OidElementValue(51));
        m.template capability<QosCapPortStormBitRateMax>(vtss::expose::snmp::OidElementValue(52));
        m.template capability<QosCapPortStormBitBurstMin>(vtss::expose::snmp::OidElementValue(53));
        m.template capability<QosCapPortStormBitBurstMax>(vtss::expose::snmp::OidElementValue(54));
        m.template capability<QosCapPortStormFrameRateMin>(vtss::expose::snmp::OidElementValue(55));
        m.template capability<QosCapPortStormFrameRateMax>(vtss::expose::snmp::OidElementValue(56));
        m.template capability<QosCapPortStormFrameBurstMin>(vtss::expose::snmp::OidElementValue(57));
        m.template capability<QosCapPortStormFrameBurstMax>(vtss::expose::snmp::OidElementValue(58));
        m.template capability<QosCapIngressMapIdMin>(vtss::expose::snmp::OidElementValue(59));
        m.template capability<QosCapIngressMapIdMax>(vtss::expose::snmp::OidElementValue(60));
        m.template capability<QosCapEgressMapIdMin>(vtss::expose::snmp::OidElementValue(61));
        m.template capability<QosCapEgressMapIdMax>(vtss::expose::snmp::OidElementValue(62));
        m.template capability<QosCapCosIdMin>(vtss::expose::snmp::OidElementValue(63));
        m.template capability<QosCapCosIdMax>(vtss::expose::snmp::OidElementValue(64));
        m.template capability<QosCapQbvGclLenMax>(vtss::expose::snmp::OidElementValue(65));

        m.template capability<QosCapDwrrCountMask>(vtss::expose::snmp::OidElementValue(101));

        /* Has capabilities */
        m.template capability<QosCapHasGlobalStormPolicers>(vtss::expose::snmp::OidElementValue(201));
        m.template capability<QosCapHasPortStormPolicers>(vtss::expose::snmp::OidElementValue(202));
        m.template capability<QosCapHasPortQueuePolicers>(vtss::expose::snmp::OidElementValue(203));
        m.template capability<QosCapHasWredV1>(vtss::expose::snmp::OidElementValue(204));
        m.template capability<QosCapHasWredV2>(vtss::expose::snmp::OidElementValue(205));
        m.template capability<QosCapHasFixedTagCosMap>(vtss::expose::snmp::OidElementValue(206));
        m.template capability<QosCapHasTagClassification>(vtss::expose::snmp::OidElementValue(207));
        m.template capability<QosCapHasTagRemarking>(vtss::expose::snmp::OidElementValue(208));
        m.template capability<QosCapHasDscp>(vtss::expose::snmp::OidElementValue(209));
        m.template capability<QosCapHasDscpDplClassification>(vtss::expose::snmp::OidElementValue(210));
        m.template capability<QosCapHasDscpDplRemarking>(vtss::expose::snmp::OidElementValue(211));
        m.template capability<QosCapHasPortPolicersFc>(vtss::expose::snmp::OidElementValue(212));
        m.template capability<QosCapHasQueuePolicersFc>(vtss::expose::snmp::OidElementValue(213));
        m.template capability<QosCapHasPortShapersDlb>(vtss::expose::snmp::OidElementValue(214));
        m.template capability<QosCapHasQueueShapersDlb>(vtss::expose::snmp::OidElementValue(215));
        m.template capability<QosCapHasQueueShapersExcess>(vtss::expose::snmp::OidElementValue(216));
        m.template capability<QosCapHasWredV3>(vtss::expose::snmp::OidElementValue(217));
        m.template capability<QosCapHasQueueShapersCredit>(vtss::expose::snmp::OidElementValue(218));
        m.template capability<QosCapHasQueueCutThrough>(vtss::expose::snmp::OidElementValue(219));
        m.template capability<QosCapHasQueueFramePreemption>(vtss::expose::snmp::OidElementValue(220));
        m.template capability<QosCapHasQce>(vtss::expose::snmp::OidElementValue(301));
        m.template capability<QosCapHasQceAddressMode>(vtss::expose::snmp::OidElementValue(302));
        m.template capability<QosCapHasQceKeyType>(vtss::expose::snmp::OidElementValue(303));
        m.template capability<QosCapHasQceMacOui>(vtss::expose::snmp::OidElementValue(304));
        m.template capability<QosCapHasQceDmac>(vtss::expose::snmp::OidElementValue(305));
        m.template capability<QosCapHasQceDip>(vtss::expose::snmp::OidElementValue(306));
        m.template capability<QosCapHasQceCTag>(vtss::expose::snmp::OidElementValue(307));
        m.template capability<QosCapHasQceSTag>(vtss::expose::snmp::OidElementValue(308));
        m.template capability<QosCapHasQceInnerTag>(vtss::expose::snmp::OidElementValue(309));
        m.template capability<QosCapHasQceActionPcpDei>(vtss::expose::snmp::OidElementValue(310));
        m.template capability<QosCapHasQceActionPolicy>(vtss::expose::snmp::OidElementValue(311));
        m.template capability<QosCapHasShapersRt>(vtss::expose::snmp::OidElementValue(312));
        m.template capability<QosCapHasQceActionMap>(vtss::expose::snmp::OidElementValue(313));
        m.template capability<QosCapHasIngressMap>(vtss::expose::snmp::OidElementValue(314));
        m.template capability<QosCapHasEgressMap>(vtss::expose::snmp::OidElementValue(315));
        m.template capability<QosCapHasWred2orWred3>(vtss::expose::snmp::OidElementValue(316));
        m.template capability<QosCapHasDscpDp2>(vtss::expose::snmp::OidElementValue(317));
        m.template capability<QosCapHasDscpDp3>(vtss::expose::snmp::OidElementValue(318));
        m.template capability<QosCapHasDefaultPcpAndDei>(vtss::expose::snmp::OidElementValue(319));
        m.template capability<QosCapHasTrustTag>(vtss::expose::snmp::OidElementValue(320));
        m.template capability<QosCapHasCosIdClassification>(vtss::expose::snmp::OidElementValue(321));
        m.template capability<QosCapHasQbv>(vtss::expose::snmp::OidElementValue(322));
        m.template capability<QosCapHasWred>(vtss::expose::snmp::OidElementValue(323));
        m.template capability<QosCapHasPsfp>(vtss::expose::snmp::OidElementValue(324));
        m.template capability<QosCapHasMplsTc>(vtss::expose::snmp::OidElementValue(325));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosStatusParams {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_qos_status_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_qos_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosStormPolicerUnicastLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_qos_global_storm_policer_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_qos_global_storm_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_global_uc_policer_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_global_uc_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosStormPolicerMulticastLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_qos_global_storm_policer_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_qos_global_storm_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_global_mc_policer_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_global_mc_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosStormPolicerBroadcastLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_qos_global_storm_policer_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_qos_global_storm_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_global_bc_policer_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_global_bc_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosWredEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_wred_group_t>,
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamKey<mesa_dp_level_t>,
        vtss::expose::ParamVal<vtss_appl_qos_wred_t *>
    > P;

    typedef QosCapHasWred depends_on_t;

    static constexpr const char *table_description =
        "This table contains the configuration of WRED (Weighted Random Early Discard).\n"
        "This is an optional table and is only present if qosCapabilitiesHasWred is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific WRED profile";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_wred_group_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_wred_group_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_dp_level_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_dpl_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_qos_wred_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_wred_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_wred_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_wred_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosDscpE {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_dscp_t>,
        vtss::expose::ParamVal<vtss_appl_qos_dscp_entry_t *>
    > P;

    typedef QosCapHasDscp depends_on_t;

    static constexpr const char *table_description =
        "This table has 64 entries, one for each DSCP value.\nThis is an optional table and is only present if qosCapabilitiesHasDscp is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific DSCP";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_dscp_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_dscp_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_dscp_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_dscp_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_dscp_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_dscp_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosCosToDscpEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamVal<vtss_appl_qos_cos_dscp_entry_t *>
    > P;

    typedef QosCapHasDscp depends_on_t;

    static constexpr const char *table_description =
        "This table has 8 entries, one for each Cos value.\nThis is an optional table and is only present if qosCapabilitiesHasDscp is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific Cos";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_cos_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_cos_dscp_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_cos_dscp_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_cos_dscp_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_cos_dscp_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIngressMapEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qos_ingress_map_id_t>,
        vtss::expose::ParamVal<vtss_appl_qos_ingress_map_conf_t *>
    > P;

    typedef QosCapHasIngressMap depends_on_t;

    static constexpr uint32_t snmpRowEditorOid = 10000;
    static constexpr const char *table_description =
        "This table contains the configuration of QoS Ingress Maps. The index is IngressMapId.\n"
        "This is an optional table and is only present if qosCapabilitiesHasIngressMap is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of Key and Action values for a single QoS Ingress Map.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qos_ingress_map_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ingress_map_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_ingress_map_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_ingress_map_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_ingress_map_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_ingress_map_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_qos_ingress_map_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_qos_ingress_map_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIngressMapPcpEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qos_ingress_map_id_t>,
        vtss::expose::ParamKey<mesa_tagprio_t>,
        vtss::expose::ParamKey<mesa_dei_t>,
        vtss::expose::ParamVal<vtss_appl_qos_ingress_map_values_t *>
    > P;

    typedef QosCapHasIngressMap depends_on_t;

    static constexpr const char *table_description =
        "This table contains the configuration of QoS Ingress Maps entries.\n"
        "This is an optional table and is only present if qosCapabilitiesHasIngressMap is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of a single entry in a QoS Ingress Map with PCP and DEI as index.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qos_ingress_map_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ingress_map_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_tagprio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_pcp_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_dei_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_dei_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_qos_ingress_map_values_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_ingress_map_pcp_dei_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_ingress_map_pcp_dei_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_ingress_map_pcp_dei_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIngressMapDscpEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qos_ingress_map_id_t>,
        vtss::expose::ParamKey<mesa_dscp_t>,
        vtss::expose::ParamVal<vtss_appl_qos_ingress_map_values_t *>
    > P;

    typedef QosCapHasIngressMap depends_on_t;

    static constexpr const char *table_description =
        "This table contains the configuration of QoS Ingress Maps entries.\n"
        "This is an optional table and is only present if qosCapabilitiesHasIngressMap is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of a single entry in a QoS Ingress Map with DSCP as index.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qos_ingress_map_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ingress_map_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_dscp_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_dscp_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_qos_ingress_map_values_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_ingress_map_dscp_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_ingress_map_dscp_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_ingress_map_dscp_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosEgressMapEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qos_egress_map_id_t>,
        vtss::expose::ParamVal<vtss_appl_qos_egress_map_conf_t *>
    > P;

    typedef QosCapHasEgressMap depends_on_t;

    static constexpr uint32_t snmpRowEditorOid = 10000;
    static constexpr const char *table_description =
        "This table contains the configuration of Egress Maps. The index is EgressMapId.\n"
        "This is an optional table and is only present if qosCapabilitiesHasEgressMap is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of key and Action values for a single Egress Map.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qos_egress_map_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_egress_map_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_egress_map_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_egress_map_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_egress_map_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_egress_map_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_qos_egress_map_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_qos_egress_map_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosEgressMapCosidEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qos_egress_map_id_t>,
        vtss::expose::ParamKey<mesa_cosid_t>,
        vtss::expose::ParamKey<mesa_dpl_t>,
        vtss::expose::ParamVal<vtss_appl_qos_egress_map_values_t *>
    > P;

    typedef QosCapHasEgressMap depends_on_t;

    static constexpr const char *table_description =
        "This table contains the configuration of QoS Egress Maps entries.\n"
        "This is an optional table and is only present if qosCapabilitiesHasEgressMap is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of a single entry in a QoS Egress Map with COSID and DPL as index.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qos_egress_map_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_egress_map_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_cosid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_cosid_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_dpl_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_dpl_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_qos_egress_map_values_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_egress_map_cosid_dpl_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_egress_map_cosid_dpl_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_egress_map_cosid_dpl_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosEgressMapDscpEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qos_egress_map_id_t>,
        vtss::expose::ParamKey<mesa_dscp_t>,
        vtss::expose::ParamKey<mesa_dpl_t>,
        vtss::expose::ParamVal<vtss_appl_qos_egress_map_values_t *>
    > P;

    typedef QosCapHasEgressMap depends_on_t;

    static constexpr const char *table_description =
        "This table contains the configuration of QoS Egress Maps entries.\n"
        "This is an optional table and is only present if qosCapabilitiesHasEgressMap is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of a single entry in a QoS Egress Map with DSCP and DPL as index.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qos_egress_map_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_egress_map_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_dscp_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_dscp_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_dpl_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_dpl_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_qos_egress_map_values_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_egress_map_dscp_dpl_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_egress_map_dscp_dpl_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_egress_map_dscp_dpl_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfConfigEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_if_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This table provides QoS configuration for QoS manageable interfaces";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific interface";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_if_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_interface_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_interface_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_interface_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfTagToCosEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_tagprio_t>,
        vtss::expose::ParamKey<mesa_dei_t>,
        vtss::expose::ParamVal<vtss_appl_qos_tag_cos_entry_t *>
    > P;

    typedef QosCapHasTrustTag depends_on_t;

    static constexpr const char *table_description =
        "This table contains the mapping of (interface, PCP, DEI) to (CoS, DPL) values.\n"
        "The mappings given by this table is used for all tagged packets received on an interface if and only if that interface has the value of TrustTag set to 'true'\n"
        "This is an optional table and is only present if qosCapabilitiesHasFixedTagCosMap is false and qosCapabilitiesHasTagClassification is true.";

    static constexpr const char *index_description =
        "Each row contains the mapping from (interface, PCP, DEI) to (CoS, DPL) values.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_tagprio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_pcp_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_dei_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_dei_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_qos_tag_cos_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_tag_cos_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_tag_cos_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_tag_cos_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfCosToTagEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamKey<mesa_dp_level_t>,
        vtss::expose::ParamVal<vtss_appl_qos_cos_tag_entry_t *>
    > P;

    typedef QosCapHasTagRemarking depends_on_t;

    static constexpr const char *table_description =
        "This table contains the mapping of (interface, CoS, DPL) to (PCP, DEI) values.\n"
        "The mappings given by this table is used for packets transmitted on an interface if and only if that interface has the value of TagRemarkingMode set to 'mapped'\n"
        "This is an optional table and is only present if qosCapabilitiesHasTagRemarking is true.";

    static constexpr const char *index_description =
        "Each row contains the mapping from (interface, CoS, DPL) to (PCP, DEI) values.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_cos_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_dp_level_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_color_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_qos_cos_tag_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_cos_tag_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_cos_tag_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_cos_tag_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfPolicerEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_port_policer_t *>
    > P;

    static constexpr const char *table_description =
        "This table provides policer configuration for an interface";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific policer";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_port_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_port_policer_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_port_policer_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_port_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfQueuePolicerEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamVal<vtss_appl_qos_queue_policer_t *>
    > P;

    typedef QosCapHasPortQueuePolicers depends_on_t;

    static constexpr const char *table_description =
        "This table provides queue policer configuration for interfaces\n"
        "This is an optional table and is only present if qosCapabilitiesHasPortQueuePolicers is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific queue policer";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_qos_queue_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_queue_policer_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_queue_policer_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_queue_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfShaperEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_port_shaper_t *>
    > P;

    static constexpr const char *table_description =
        "This table provides shaper configuration for an interface";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific shaper";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_port_shaper_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_port_shaper_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_port_shaper_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_port_shaper_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfQueueShaperEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamVal<vtss_appl_qos_queue_shaper_t *>
    > P;

    static constexpr const char *table_description =
        "This table provides queue shaper configuration for interfaces";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific queue shaper";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_qos_queue_shaper_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_queue_shaper_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_queue_shaper_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_queue_shaper_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfSchedulerEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamVal<vtss_appl_qos_scheduler_t *>
    > P;

    static constexpr const char *table_description =
        "This table contains the mapping of (interface, queue) to weight values.\n"
        "The mappings given by this table is used when an interface has the value of DwrrCount greater than 0.\n"
        "Read the qosSchedulerStatusTable in order to get the 'real' weights in percent as used by the hardware.";

    static constexpr const char *index_description =
        "Each row contains the scheduler configuration for a specific queue.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_qos_scheduler_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_scheduler_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_scheduler_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_scheduler_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfStormPolicerUnicastEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_port_storm_policer_t *>
    > P;

    typedef QosCapHasPortStormPolicers depends_on_t;

    static constexpr const char *table_description =
        "This table provides storm policer configuration for unicast packets received on an interface\n"
        "This is an optional table and is only present if qosCapabilitiesHasPortStormPolicers is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific storm policer";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_port_storm_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_port_uc_policer_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_port_storm_policer_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_port_uc_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfStormPolicerBroadcastEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_port_storm_policer_t *>
    > P;

    typedef QosCapHasPortStormPolicers depends_on_t;

    static constexpr const char *table_description =
        "This table provides storm policer configuration for broadcast packets received on an interface\n"
        "This is an optional table and is only present if qosCapabilitiesHasPortStormPolicers is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific storm policer";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_port_storm_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_port_bc_policer_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_port_storm_policer_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_port_bc_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfStormPolicerUnknownEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_port_storm_policer_t *>
    > P;

    typedef QosCapHasPortStormPolicers depends_on_t;

    static constexpr const char *table_description =
        "This table provides storm policer configuration for unknown (flooded) packets received on an interface\n"
        "This is an optional table and is only present if qosCapabilitiesHasPortStormPolicers is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration for a specific storm policer";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_port_storm_policer_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_port_un_policer_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_port_storm_policer_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_port_un_policer_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosQceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_qce_id_t>,
        vtss::expose::ParamVal<vtss_appl_qos_qce_conf_t *>
    > P;

    typedef QosCapHasQce depends_on_t;

    static constexpr uint32_t snmpRowEditorOid = 10000;
    static constexpr const char *table_description =
        "This table contains the configuration of QCEs. The index is QceId.\n"
        "This is an optional table and is only present if qosCapabilitiesHasQce is true.";

    static constexpr const char *index_description =
        "Each row contains the configuration of a single QCE.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_qce_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_qce_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_qce_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_qce_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_qce_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_qce_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_qos_qce_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_qos_qce_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosQcePrecedenceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_qos_qce_conf_t *>
    > P;

    typedef QosCapHasQce depends_on_t;

    static constexpr const char *table_description =
        "This table contains the precedence of QCEs ordered by their position in the QCL.\n"
        "This is an optional table and is only present if qosCapabilitiesHasQce is true.";

    static constexpr const char *index_description =
        "Each row contains the precedence of a single QCE.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_qce_precedence_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_qce_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_qce_precedence_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_qce_precedence_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_qce_precedence_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_qos_if_status_t *>
    > P;

    static constexpr const char *table_description =
        "This table provides QoS status for QoS manageable interfaces";

    static constexpr const char *index_description =
        "Each row contains the status for a specific interface";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_qos_if_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_interface_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_interface_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosIfSchedulerStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_prio_t>,
        vtss::expose::ParamVal<vtss_appl_qos_scheduler_status_t *>
    > P;

    static constexpr const char *table_description =
        "This table contains the mapping of (interface, queue) to weight values.\n"
        "The mappings given by this table are the 'real' weights in percent as used by the hardware.";

    static constexpr const char *index_description =
        "Each row contains the scheduler status for a specific queue.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_qos_scheduler_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_scheduler_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_scheduler_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosQceStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_qos_qce_conf_t *>
    > P;

    typedef QosCapHasQce depends_on_t;

    static constexpr const char *table_description =
        "This table contains the QCE status in the hardware table.\n"
        "This is an optional table and is only present if qosCapabilitiesHasQce is true.";

    static constexpr const char *index_description =
        "Each row contains the status of a single QCE.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_usid_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_qce_precedence_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_qos_qce_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, vtss_appl_qos_qce_status_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_qce_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_qos_qce_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_QOS);
};

struct QosQceConflictResolve {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<BOOL *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_qos_qce_conflict_resolve_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_qos_qce_conflict_resolve_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_qos_qce_conflict_resolve_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS);
};

}  // namespace interfaces
}  // namespace qos
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_QOS_SERIALIZER_HXX__ */
