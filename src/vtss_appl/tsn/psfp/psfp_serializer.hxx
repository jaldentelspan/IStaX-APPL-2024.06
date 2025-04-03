/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _PSFP_SERIALIZER_HXX_
#define _PSFP_SERIALIZER_HXX_

#include <vtss/appl/psfp.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/types.hxx>
#include "psfp_api.h" // For psfp_util_time_t and friends
#include "tsn_api.h"  // For tsn_util_timestamp_to_iso8601()
#include "vtss_appl_serialize.hxx"

/******************************************************************************/
// psfp_serializer_flow_meter_conf_default_get()
/******************************************************************************/
static inline mesa_rc psfp_serializer_flow_meter_conf_default_get(vtss_appl_psfp_flow_meter_id_t *flow_meter_id, vtss_appl_psfp_flow_meter_conf_t *conf)
{
    if (!flow_meter_id) {
        return VTSS_RC_ERROR;
    }

    *flow_meter_id = VTSS_APPL_PSFP_FLOW_METER_ID_NONE;
    return vtss_appl_psfp_flow_meter_conf_default_get(conf);
}

/******************************************************************************/
// psfp_serializer_flow_meter_conf_default_get_no_inst()
/******************************************************************************/
static inline mesa_rc psfp_serializer_flow_meter_conf_default_get_no_inst(vtss_appl_psfp_flow_meter_conf_t *conf)
{
    vtss_appl_psfp_flow_meter_id_t flow_meter_id;
    return psfp_serializer_flow_meter_conf_default_get(&flow_meter_id, conf);
}

// This struct is used to wrap the time specifiers inside
// vtss_appl_psfp_gate_conf_t to numerators and denominators, which are
// serializable.
struct psfp_serializer_gate_conf_t {
    /**
     * The original gate configuration
     */
    vtss_appl_psfp_gate_conf_t conf;

    /**
     * Serializable cycle time
     */
    psfp_util_time_t cycle_time;

    /**
     * Serializable cycle time extension
     */
    psfp_util_time_t cycle_time_extension;

    /**
     * Serializable per-GCE time intervals.
     */
    psfp_util_time_t gce_time_intervals[ARRSZ(vtss_appl_psfp_gate_conf_t::gcl)];

    /**
     * Base time in ISO 8601 format. Only readable, not writable
     */
    char base_time_iso8601[TSN_UTIL_ISO8601_STRING_SIZE];
};

/******************************************************************************/
// psfp_serializer_gate_conf_convert()
/******************************************************************************/
static inline void psfp_serializer_gate_conf_convert(psfp_serializer_gate_conf_t &s)
{
    int i;

    psfp_util_time_to_num_denom(s.conf.cycle_time_ns,           s.cycle_time);
    psfp_util_time_to_num_denom(s.conf.cycle_time_extension_ns, s.cycle_time_extension);

    for (i = 0; i < ARRSZ(s.gce_time_intervals); i++) {
        psfp_util_time_to_num_denom(s.conf.gcl[i].time_interval_ns, s.gce_time_intervals[i]);
    }

    (void)tsn_util_timestamp_to_iso8601(s.base_time_iso8601, sizeof(s.base_time_iso8601), s.conf.base_time);
}

/******************************************************************************/
// psfp_serializer_gate_conf_default_get()
/******************************************************************************/
static inline mesa_rc psfp_serializer_gate_conf_default_get(vtss_appl_psfp_gate_id_t *gate_id, psfp_serializer_gate_conf_t *s)
{
    if (!gate_id || !s) {
        return VTSS_RC_ERROR;
    }

    *gate_id = VTSS_APPL_PSFP_GATE_ID_NONE;
    VTSS_RC(vtss_appl_psfp_gate_conf_default_get(&s->conf));

    psfp_serializer_gate_conf_convert(*s);

    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_serializer_gate_conf_default_get_no_inst()
/******************************************************************************/
static inline mesa_rc psfp_serializer_gate_conf_default_get_no_inst(psfp_serializer_gate_conf_t *s)
{
    vtss_appl_psfp_gate_id_t gate_id;
    return psfp_serializer_gate_conf_default_get(&gate_id, s);
}

/******************************************************************************/
// psfp_serializer_gate_conf_get()
/******************************************************************************/
static inline mesa_rc psfp_serializer_gate_conf_get(vtss_appl_psfp_gate_id_t gate_id, psfp_serializer_gate_conf_t *s)
{
    if (!s) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_psfp_gate_conf_get(gate_id, &s->conf));

    psfp_serializer_gate_conf_convert(*s);

    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_serializer_gate_conf_set()
/******************************************************************************/
static inline mesa_rc psfp_serializer_gate_conf_set(vtss_appl_psfp_gate_id_t gate_id, const psfp_serializer_gate_conf_t *s)
{
    vtss_appl_psfp_gate_conf_t conf;
    int                        i;

    if (!s) {
        return VTSS_RC_ERROR;
    }

    conf = s->conf;
    VTSS_RC(psfp_util_time_from_num_denom(conf.cycle_time_ns,           s->cycle_time));
    VTSS_RC(psfp_util_time_from_num_denom(conf.cycle_time_extension_ns, s->cycle_time_extension));

    for (i = 0; i < ARRSZ(s->gce_time_intervals); i++) {
        VTSS_RC(psfp_util_time_from_num_denom(conf.gcl[i].time_interval_ns, s->gce_time_intervals[i]));
    }

    return vtss_appl_psfp_gate_conf_set(gate_id, &conf);
}

/******************************************************************************/
// psfp_serializer_filter_conf_default_get()
/******************************************************************************/
static inline mesa_rc psfp_serializer_filter_conf_default_get(vtss_appl_psfp_filter_id_t *filter_id, vtss_appl_psfp_filter_conf_t *s)
{
    if (!filter_id || !s) {
        return VTSS_RC_ERROR;
    }

    *filter_id = VTSS_APPL_PSFP_FILTER_ID_NONE;
    return vtss_appl_psfp_filter_conf_default_get(s);
}

static const vtss_enum_descriptor_t vtss_appl_psfp_flow_meter_cm_txt[] = {
    {VTSS_APPL_PSFP_FLOW_METER_CM_BLIND, "colorBlind"},
    {VTSS_APPL_PSFP_FLOW_METER_CM_AWARE, "colorAware"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_psfp_flow_meter_cm_t,
                         "psfpFlowMeterColorMode",
                         vtss_appl_psfp_flow_meter_cm_txt,
                         "This enumeration defines the color awareness.");

static const vtss_enum_descriptor_t psfp_serializer_gate_states_txt[] = {
    {VTSS_APPL_PSFP_GATE_STATE_CLOSED, "closed"},
    {VTSS_APPL_PSFP_GATE_STATE_OPEN,   "open"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_psfp_gate_state_t,
                         "psfpGateState",
                         psfp_serializer_gate_states_txt,
                         "This enumeration defines the available PSFP Gate states.");

static const vtss_enum_descriptor_t psfp_serializer_time_unit_txt[] = {
    {PSFP_UTIL_TIME_UNIT_NSEC, "ns"},
    {PSFP_UTIL_TIME_UNIT_USEC, "us"},
    {PSFP_UTIL_TIME_UNIT_MSEC, "ms"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(psfp_util_time_unit_t,
                         "psfpTimeUnit",
                         psfp_serializer_time_unit_txt,
                         "This enumeration defines the available PSFP time units.");

VTSS_SNMP_TAG_SERIALIZE(psfp_serializer_flow_meter_id, vtss_appl_psfp_flow_meter_id_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("FlowMeterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The ID of the Flow Meter instance"));
}

VTSS_SNMP_TAG_SERIALIZE(psfp_serializer_gate_id, vtss_appl_psfp_gate_id_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("GateId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The ID of the Stream Gate instance"));
}

VTSS_SNMP_TAG_SERIALIZE(psfp_serializer_filter_id, vtss_appl_psfp_filter_id_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("FilterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The ID of the Stream Filter instance"));
}

VTSS_SNMP_TAG_SERIALIZE(psfp_serializer_filter_statistics_clear_value, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Set to TRUE to clear the statistics of a stream filter instance."));
}

/******************************************************************************/
// psfp_serializer_flow_meter_control_get()
// Dummy function.
/******************************************************************************/
static inline mesa_rc psfp_serializer_flow_meter_control_get(vtss_appl_psfp_flow_meter_id_t flow_meter_id, vtss_appl_psfp_flow_meter_control_t *ctrl)
{
    if (!ctrl) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(*ctrl);
    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_serializer_gate_control_get()
// Dummy function.
/******************************************************************************/
static inline mesa_rc psfp_serializer_gate_control_get(vtss_appl_psfp_gate_id_t gate_id, vtss_appl_psfp_gate_control_t *ctrl)
{
    if (!ctrl) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(*ctrl);
    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_serializer_filter_control_get()
// Dummy function.
/******************************************************************************/
static inline mesa_rc psfp_serializer_filter_control_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_control_t *ctrl)
{
    if (!ctrl) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(*ctrl);
    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_serializer_filter_statistics_clear_get()
// Dummy function.
/******************************************************************************/
static inline mesa_rc psfp_serializer_filter_statistics_clear_get(vtss_appl_psfp_filter_id_t filter_id, BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_serializer_filter_statistics_clear_set()
// Dummy function.
/******************************************************************************/
static inline mesa_rc psfp_serializer_filter_statistics_clear_set(vtss_appl_psfp_filter_id_t filter_id, const BOOL *clear)
{
    if (clear && *clear) {
        return vtss_appl_psfp_filter_statistics_clear(filter_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_capabilities_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_capabilities_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_capabilities_t"));
    int ix = 0;

    m.add_leaf(p.max_filter_instances,
               vtss::tag::Name("FilterInstanceCntMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The FilterInstanceCntMax parameter defines the "
                                      "maximum number of stream filter instances "
                                      "that are supported by this Bridge component. These are numbered [0; max]."));

    m.add_leaf(p.max_gate_instances,
               vtss::tag::Name("GateInstanceCntMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The GateInstanceCntMax parameter defines the "
                                      "maximum number of stream gate instances that are "
                                      "supported by this Bridge component. These are numbered [0; max[."));

    m.add_leaf(p.max_flow_meter_instances,
               vtss::tag::Name("FlowMeterInstanceCntMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The FlowMeterInstanceCntMax parameter defines the "
                                      "maximum number of flow meter instances that are "
                                      "supported by this Bridge component."));

    m.add_leaf(p.gate_control_list_length_max,
               vtss::tag::Name("GateControlListLengthMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The GclListSizeMax parameter defines the "
                                      "The maximum value supported by this Bridge component of "
                                      "the AdminControlListLength and "
                                      "OperControlListLength parameters."));

    m.add_leaf(p.stream_id_max,
               vtss::tag::Name("StreamIdMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum value of a Stream ID."));

    m.add_leaf(p.stream_collection_id_max,
               vtss::tag::Name("StreamCollectionIdMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum value of a Stream Collection ID."));

    m.add_leaf(vtss::AsBool(p.psfp_supported),
               vtss::tag::Name("PsfpSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether PSFP is supported or not on this platform."));
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_flow_meter_conf_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_flow_meter_conf_t"));
    int ix = 0;

    m.add_leaf(p.cir,
               vtss::tag::Name("CIR"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CIR parameter contains an integer value that "
                                      "represents the CIR value for the flow meter, in kbit/s."));

    m.add_leaf(p.cbs,
               vtss::tag::Name("CBS"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CBS parameter contains an integer value that "
                                      "represents the CBS value for the flow meter, in octets."));

    m.add_leaf(p.eir,
               vtss::tag::Name("EIR"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The EIR parameter contains an integer value that "
                                      "represents the EIR value for the flow meter, in kbit/s."));

    m.add_leaf(p.ebs,
               vtss::tag::Name("EBS"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The EBS parameter contains an integer value that "
                                      "represents the EBS value for the flow meter, in octets."));

    m.add_leaf(p.cf,
               vtss::tag::Name("CF"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CF parameter contains an integer value that "
                                      "represents the CF value for the flow meter, as an integer "
                                      "value 0 or 1."));

    m.add_leaf(p.cm,
               vtss::tag::Name("CM"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CM parameter contains an integer value that "
                                      "represents the CM value for the flow meter, as an "
                                      "enumerated value indicating colorBlind or "
                                      "colorAware."));

    m.add_leaf(vtss::AsBool(p.drop_on_yellow),
               vtss::tag::Name("DropOnYellow"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The DropOnYellow parameter contains a boolean "
                                      "value that indicates whether yellow frames are dropped "
                                      "(TRUE) or have DEI set to TRUE (FALSE)"));

    m.add_leaf(vtss::AsBool(p.mark_all_frames_red_enable),
               vtss::tag::Name("MarkAllFramesRedEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The MarkAllFramesRedEnable parameter contains "
                                      "a boolean value that indicates whether the "
                                      "MarkAllFramesRed function is enabled (TRUE) or "
                                      "disabled (FALSE)."));
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_status_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_flow_meter_status_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_flow_meter_status_t"));
    int               ix = 0;

    m.add_leaf(p.mark_all_frames_red,
               vtss::tag::Name("MarkAllFramesRed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The MarkAllFramesRed parameter contains "
                                      "a boolean value that indicates whether, if the "
                                      "MarkAllFramesRed function is enabled, all frames are to "
                                      "be discarded (TRUE) or not (FALSE)."));
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_control_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_flow_meter_control_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_flow_meter_control_t"));
    int               ix = 0;

    m.add_leaf(p.clear_mark_all_frames_red,
               vtss::tag::Name("ClearMarkAllFramesRed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Set to TRUE to clear the MarkAllFramesRed flag."));
}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_t
/******************************************************************************/
template <typename T>
void serialize_gate_conf(T &m, psfp_serializer_gate_conf_t &s, const char *prefix, int &ix)
{
    vtss_appl_psfp_gate_conf_t &p = s.conf;
    char                       name_buf[100];
    int                        i;

    // Converted to text with psfp_serializer_gate_states_txt[]
    sprintf(name_buf, "%sGateState", prefix);
    m.add_leaf(p.gate_state,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The administrative value of the GateState parameter for "
                                      "the stream gate. The open value indicates that the gate "
                                      "is open, the closed value indicates that the gate is "
                                      "closed."));

    sprintf(name_buf, "%sCycleTimeNumerator", prefix);
    m.add_leaf(s.cycle_time.numerator,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The numerator of the administrative value of the AdminCycleTime "
                                      "parameter measured in units of AdminCycleTimeUnit."));

    // Converted to text with psfp_serializer_time_unit_txt[]
    sprintf(name_buf, "%sCycleTimeUnit", prefix);
    m.add_leaf(s.cycle_time.unit,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The denominator (units) of the administrative value of the AdminCycleTime "
                                      "parameter. Numerator in AdminCycleTimeNumerator."));

    sprintf(name_buf, "%sCycleTimeExtensionNumerator", prefix);
    m.add_leaf(s.cycle_time_extension.numerator,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The numerator of the administrative value of the AdminCycleTimeExtension "
                                      "parameter measured in units of AdminCycleTimeExtensionUnit."));

    // Converted to text with psfp_serializer_time_unit_txt[]
    sprintf(name_buf, "%sCycleTimeExtensionUnit", prefix);
    m.add_leaf(s.cycle_time_extension.unit,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The denominator (units) of the administrative value of the AdminCycleTimeExtension "
                                      "parameter. Numerator in AdminCycleTimeExtensionNumerator."));

    sprintf(name_buf, "%sBaseTimeTxt", prefix);
    m.add_leaf(vtss::AsTextualTimestamp(p.base_time),
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The administrative value of the BaseTime parameter for the gate. "
                                      "The value is represented in the format sssssssssssssss[.sssssssss]"));

    // Only readable, not writable
    sprintf(name_buf, "%sBaseTimeIso8601", prefix);
    m.add_leaf(vtss::AsDisplayString(s.base_time_iso8601, sizeof(s.base_time_iso8601)),
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The administrative value of the BaseTime parameter for "
                                      "the gate, in ISO 8601 format (YYYY-MM-DDTHH:mm:ss.SSSZ). "
                                      "This is only readable, not writable."));

    sprintf(name_buf, "%sIpv", prefix);
    m.add_leaf(p.ipv,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The administrative value of the IPV parameter for the gate. "
                                      "Set to -1 in order not to map to a particular egress queue."));

    sprintf(name_buf, "%sCloseGateInvalidRxEnable", prefix);
    m.add_leaf(vtss::AsBool(p.close_gate_due_to_invalid_rx_enable),
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CloseGateInvalidRxEnable object contains "
                                      "a boolean value that indicates whether the "
                                      "PSFPGateClosedInvalidRx function is enabled (TRUE) "
                                      "or disabled (FALSE)."));

    sprintf(name_buf, "%sCloseGateOctetsExceededEnable", prefix);
    m.add_leaf(vtss::AsBool(p.close_gate_due_to_octets_exceeded_enable),
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The CloseGateOctetsExceededEnable object "
                                      "contains a boolean value that indicates whether the "
                                      "PSFPGateClosedOctetsExceeded function is enabled "
                                      "(TRUE) or disabled (FALSE)."));

    sprintf(name_buf, "%sGclLength", prefix);
    m.add_leaf(p.gcl_length,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The length of the GCL list."));

    // Make room for additional, future "global" variables
    ix += 10;

    for (i = 0; i < ARRSZ(p.gcl); i++) {
        vtss_appl_psfp_gate_gce_conf_t &gce = p.gcl[i];

        // Converted to text with psfp_serializer_gate_states_txt[]
        sprintf(name_buf, "%sGce%dGateState", prefix, i);
        m.add_leaf(gce.gate_state,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("The Gate Control entry's gate state. Default is closed."));

        sprintf(name_buf, "%sGce%dIpv", prefix, i);
        m.add_leaf(gce.ipv,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("The internal priority value of the gate entry used "
                                          "to map frames to a particular egress queue. "
                                          "Set to -1 in order to map frames to their default egress queue."));

        sprintf(name_buf, "%sGce%dTimeIntervalNumerator", prefix, i);
        m.add_leaf(s.gce_time_intervals[i].numerator,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("The numerator of the gate entry's time interval "
                                          "parameter measured in units of this gate entry's TimeIntervalUnit."));

        // Converted to text with psfp_serializer_time_unit_txt[]
        sprintf(name_buf, "%sGce%dTimeIntervalUnit", prefix, i);
        m.add_leaf(s.gce_time_intervals[i].unit,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("The denominator (units) of the gate entry's time interval "
                                          "parameter. Numerator in this gate entry's TimeIntervalNumerator."));

        sprintf(name_buf, "%sGce%dIntervalOctetMax", prefix, i);
        m.add_leaf(gce.interval_octet_max,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Frames larger than this value are discarded by the Gate Control entry. "
                                          "Set to 0 to disable this check and let any frame size pass through."));

        // Make room for additional, future per-GCE variables.
        ix += 5;
    }

    sprintf(name_buf, "%sConfigChange", prefix);
    m.add_leaf(vtss::AsBool(p.config_change),
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The ConfigChange parameter signals the start of a "
                                      "configuration change for the gate when it is set to TRUE. "
                                      "This should only be done when the various administrative "
                                      "parameters are all set to appropriate values."));

    sprintf(name_buf, "%sGateEnabled", prefix);
    m.add_leaf(vtss::AsBool(p.gate_enabled),
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The GateEnabled parameter determines whether the "
                                      "stream gate is active (true) or inactive (false)."));

}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_t
/******************************************************************************/
template <typename T>
void serialize(T &a, psfp_serializer_gate_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_gate_conf_t"));
    int               ix = 0;

    serialize_gate_conf(m, s, "Admin", ix);
}

/******************************************************************************/
// vtss_appl_psfp_gate_status_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_gate_status_t &p)
{
    typename T::Map_t           m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_gate_status_t"));
    psfp_serializer_gate_conf_t gate_conf;
    char                        iso8601[TSN_UTIL_ISO8601_STRING_SIZE];
    int                         ix = 0;

    m.add_leaf(vtss::AsBool(p.oper_conf_valid),
               vtss::tag::Name("OperConfValid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the contentsof the operational configuration is valid."));

    // Make p.oper_conf serializable.
    gate_conf.conf = p.oper_conf;
    psfp_serializer_gate_conf_convert(gate_conf);
    serialize_gate_conf(m, gate_conf, "OperConf", ix);

    m.add_leaf(p.config_pending,
               vtss::tag::Name("ConfigPending"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the ConfigPending state machine variable. "
                                      "The value is TRUE if a configuration change is in "
                                      "progress but has not yet completed."));

    // Make p.pend_conf serializable.
    gate_conf.conf = p.pend_conf;
    psfp_serializer_gate_conf_convert(gate_conf);
    serialize_gate_conf(m, gate_conf, "PendConf", ix);

    m.add_leaf(p.oper_gate_state,
               vtss::tag::Name("OperGateState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The operational value of the GateState parameter for the stream gate."));

    // Gotta invent a new name for this one, because p.oper_conf.ipv gets the
    // same name
    m.add_leaf(p.oper_ipv,
               vtss::tag::Name("OperIpv"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The operational value of the IPV parameter for the gate. -1 if disabled."));

    m.add_leaf(vtss::AsTextualTimestamp(p.config_change_time),
               vtss::tag::Name("ConfigChangeTimeTxt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If ConfigPending, the time at which the next config change is scheduled to occur. "
                                      "If not ConfigPending, the time at which the last config change occurred. "
                                      "The value is represented in the format sssssssssssssss[.sssssssss]."));

    (void)tsn_util_timestamp_to_iso8601(iso8601, sizeof(iso8601), p.config_change_time);
    m.add_leaf(vtss::AsDisplayString(iso8601, sizeof(iso8601)),
               vtss::tag::Name("ConfigChangeTimeIso8601"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If ConfigPending, the time at which the next config change is scheduled to occur. "
                                      "If not ConfigPending, the time at which the last config change occurred. "
                                      "The value is in ISO 8601 format (YYYY-MM-DDTHH:mm:ss.SSSZ)."));

    m.add_leaf(vtss::AsTextualTimestamp(p.current_time),
               vtss::tag::Name("CurrentTimeTxt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The current time as maintained by the local system. "
                                      "The value is represented in the format sssssssssssssss[.sssssssss]"));

    (void)tsn_util_timestamp_to_iso8601(iso8601, sizeof(iso8601), p.current_time);
    m.add_leaf(vtss::AsDisplayString(iso8601, sizeof(iso8601)),
               vtss::tag::Name("CurrentTimeIso8601"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The current time as maintained by the local system. "
                                      "The value is in ISO 8601 format  (YYYY-MM-DDTHH:mm:ss.SSSZ)."));

    m.add_leaf(p.tick_granularity,
               vtss::tag::Name("TickGranularity"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The granularity of the cycle time clock, represented as "
                                      "an unsigned number of tenths of nanoseconds."));

    m.add_leaf(p.config_change_errors,
               vtss::tag::Name("ConfigChangeErrors"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A counter of the number of times that a re-configuration "
                                      "of the traffic schedule has been requested with the old "
                                      "schedule still running and the requested base time was "
                                      "in the past."));

    m.add_leaf(p.gate_closed_due_to_invalid_rx,
               vtss::tag::Name("GateClosedDueToInvalidRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The GateClosedDueToInvalidRx object contains "
                                      "a boolean value that indicates whether, if the "
                                      "PSFPGateClosedDueToInvalidRx function is enabled, "
                                      "all frames are to be discarded (TRUE) or not (FALSE)"));

    m.add_leaf(p.gate_closed_due_to_octets_exceeded,
               vtss::tag::Name("GateClosedDueToOctetsExceeded"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The GateClosedDueToOctetsExceeded parameter "
                                      "contains a boolean value that indicates whether, if "
                                      "the PSFPGateClosedDueToOctetsExceeded function is "
                                      "enabled, all frames are to be discarded (TRUE) or not "
                                      "(FALSE)."));
}

/******************************************************************************/
// vtss_appl_psfp_gate_control_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_gate_control_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_gate_control_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(p.clear_gate_closed_due_to_invalid_rx),
               vtss::tag::Name("ClearGateClosedDueToInvalidRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Set to TRUE to clear the GateClosedDueToInvalidRx flag."));
    m.add_leaf(vtss::AsBool(p.clear_gate_closed_due_to_octets_exceeded),
               vtss::tag::Name("ClearGateClosedDueToOctetsExceeded"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Set to TRUE to clear the GateClosedDueToOctetsExceeded flag."));
}

/******************************************************************************/
// vtss_appl_psfp_filter_conf_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_filter_conf_t &p)
{
    typename T::Map_t m  = a.as_map(vtss::tag::Typename("vtss_appl_psfp_filter_conf_t"));
    int               ix = 0;

    m.add_leaf(p.stream_id,
               vtss::tag::Name("StreamId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The StreamId parameter contains a stream identifier value. "
                                      "The filter is not attached to any stream if it is set to 0. "
                                      "For a filter to be operable, it must either be attached to a stream "
                                      "or to a stream collection."));

    m.add_leaf(p.stream_collection_id,
               vtss::tag::Name("StreamCollectionId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The StreamCollectionId parameter contains a stream collection identifier value. "
                                      "The filter is not attached to any stream collection if it is set to 0. "
                                      "For a filter to be operable, it must either be attached to a stream "
                                      "or to a stream collection."));

    m.add_leaf(p.flow_meter_id,
               vtss::tag::Name("FlowMeterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The FlowMeterID parameter contains an index "
                                      "of an entry in the Flow Meter Table. "
                                      "If set to -1, no flow meters are used."));

    m.add_leaf(p.gate_id,
               vtss::tag::Name("GateId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The GateId parameter contains an index "
                                      "of an entry in the Stream Gate Table. "
                                      "If set to -1, no stream gates are used."));

    m.add_leaf(p.max_sdu_size,
               vtss::tag::Name("MaxSDUSize"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum SDU size."));

    m.add_leaf(vtss::AsBool(p.block_due_to_oversize_frame_enable),
               vtss::tag::Name("BlockDueToOversizeFrameEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If enabled and at least one frame has been discarded "
                                      "because it was longer than MaxSDUSize, all frames get "
                                      "discarded. This can be cleared with ClearBlockedDueToOversizeFrame."));
}

/******************************************************************************/
// vtss_appl_psfp_filter_status_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_filter_status_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_filter_status_t"));
    mesa_bool_t       b;
    int               ix = 0;

    m.add_leaf(vtss::AsBool(p.stream_blocked_due_to_oversize_frame),
               vtss::tag::Name("BlockedDueToOversizeFrame"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If feature is enabled, and this is true and an oversize "
                                      "frame is received, the stream filter will discard all subsequent "
                                      "frames as well, until cleared."));

    b = p.oper_warnings == VTSS_APPL_PSFP_FILTER_OPER_WARNING_NONE;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningNone"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("No warnings found."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_NO_STREAM_OR_STREAM_COLLECTION) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningNoStreamOrStreamCollection"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Neither a stream or a stream collection is specified."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_NOT_FOUND) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamNotFound"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream ID does not exist."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamCollectionNotFound"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream collection ID does not exist."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_ATTACH_FAIL) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamAttachFail"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Unable to attach to the specified stream, possibly because it is part of a stream collection"));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamCollectionAttachFail"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Unable to attach to the specified stream collection."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamHasConfigurationalWarnings"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream has configurational warnings."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamCollectionHasConfWarnings"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream collection has configurational warnings."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_FLOW_METER_NOT_FOUND) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningFlowMeterNotFound"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified flow meter ID does not exist."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_FOUND) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningGateNotFound"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream gate ID does not exist."));

    b = (p.oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_ENABLED) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningGateNotEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream gate is not enabled."));
}

/******************************************************************************/
// vtss_appl_psfp_filter_statistics_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_filter_statistics_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_filter_statistics_t"));
    int               ix = 0;

    m.add_leaf(p.matching,
               vtss::tag::Name("Matching"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Counts received frames that match this stream filter."));

    m.add_leaf(p.passing,
               vtss::tag::Name("Passing"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Counts received frames that pass the gate associated with this stream filter."));

    m.add_leaf(p.not_passing,
               vtss::tag::Name("NotPassing"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Counts received frames that do not pass the gate "
                                      "associated with this stream filter."));

    m.add_leaf(p.passing_sdu,
               vtss::tag::Name("PassingSDU"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Counts received frames that pass the SDU size filter specifications "
                                      "associated with this stream filter."));

    m.add_leaf(p.not_passing_sdu,
               vtss::tag::Name("NotPassingSDU"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Counts received frames that do not pass the SDU size filter specifications "
                                      "associated with this stream filter."));

    m.add_leaf(p.red,
               vtss::tag::Name("Red"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Counts received frames that were discarded as a result of the "
                                      "flow meter associated with this stream filter."));
}

/******************************************************************************/
// vtss_appl_psfp_filter_control_t
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_psfp_filter_control_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_psfp_filter_control_t"));
    int               ix = 0;

    m.add_leaf(vtss::AsBool(p.clear_blocked_due_to_oversize_frame),
               vtss::tag::Name("ClearBlockedDueToOversizeFrame"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If the stream is blocked due to reception of an oversize frame "
                                      "all subsequent frames are discarded despite their size "
                                      "until this field gets set to true."));
}

namespace vtss
{
namespace appl
{
namespace psfp
{
namespace interfaces
{

/******************************************************************************/
// Capabilities
/******************************************************************************/
struct Capabilities {
    typedef vtss::expose::ParamList <vtss::expose::ParamVal<vtss_appl_psfp_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_capabilities_t &s)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, s);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FlowMeterDefaultConf
/******************************************************************************/
struct FlowMeterDefaultConf {
    typedef expose::ParamList <expose::ParamVal<vtss_appl_psfp_flow_meter_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_flow_meter_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(psfp_serializer_flow_meter_conf_default_get_no_inst);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FlowMeterConf
/******************************************************************************/
struct FlowMeterConf {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_flow_meter_id_t>, expose::ParamVal<vtss_appl_psfp_flow_meter_conf_t *>> P;

    static constexpr uint32_t   snmpRowEditorOid   = 100;
    static constexpr const char *table_description = "This table holds the configuration of PSFP flow meters";
    static constexpr const char *index_description = "Each index points to a given flow meter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_flow_meter_id_t &i)
    {
        h.argument_properties(tag::Name("FlowMeterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_flow_meter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_flow_meter_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_flow_meter_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_flow_meter_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psfp_flow_meter_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_psfp_flow_meter_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_psfp_flow_meter_conf_del);
    VTSS_EXPOSE_DEF_PTR(psfp_serializer_flow_meter_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FlowMeterStatus
/******************************************************************************/
struct FlowMeterStatus {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_flow_meter_id_t>, expose::ParamVal<vtss_appl_psfp_flow_meter_status_t *>> P;

    static constexpr const char *table_description = "PSFP Flow Meter status";
    static constexpr const char *index_description = "Each index points to a given flow meter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_flow_meter_id_t &i)
    {
        h.argument_properties(tag::Name("FlowMeterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_flow_meter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_flow_meter_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_flow_meter_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_flow_meter_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FlowMeterControl
/******************************************************************************/
struct FlowMeterControl {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_flow_meter_id_t>, expose::ParamVal<vtss_appl_psfp_flow_meter_control_t *>> P;

    static constexpr const char *table_description = "PSFP Flow Meter state clear";
    static constexpr const char *index_description = "Each index points to a given flow meter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_flow_meter_id_t &i)
    {
        h.argument_properties(tag::Name("id"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_flow_meter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_flow_meter_control_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_flow_meter_itr);
    VTSS_EXPOSE_GET_PTR(psfp_serializer_flow_meter_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psfp_flow_meter_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// GateDefaultConf
/******************************************************************************/
struct GateDefaultConf {
    typedef expose::ParamList <expose::ParamVal<psfp_serializer_gate_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(psfp_serializer_gate_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(psfp_serializer_gate_conf_default_get_no_inst);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// GateConf
/******************************************************************************/
struct GateConf {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_gate_id_t>, expose::ParamVal<psfp_serializer_gate_conf_t *>> P;

    static constexpr uint32_t   snmpRowEditorOid   = 200;
    static constexpr const char *table_description = "Stream Gate configuration table";
    static constexpr const char *index_description = "Each index represents the configuration of one stream gate instance";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_gate_id_t &i)
    {
        h.argument_properties(tag::Name("GateId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_gate_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(psfp_serializer_gate_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_gate_itr);
    VTSS_EXPOSE_GET_PTR(psfp_serializer_gate_conf_get);
    VTSS_EXPOSE_SET_PTR(psfp_serializer_gate_conf_set);
    VTSS_EXPOSE_ADD_PTR(psfp_serializer_gate_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_psfp_gate_conf_del);
    VTSS_EXPOSE_DEF_PTR(psfp_serializer_gate_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// GateStatus
/******************************************************************************/
struct GateStatus {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_gate_id_t>, expose::ParamVal<vtss_appl_psfp_gate_status_t *>> P;

    static constexpr const char *table_description = "PSFP Stream Gate status table";
    static constexpr const char *index_description = "Each index represents a stream gate instance";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_gate_id_t &i)
    {
        h.argument_properties(tag::Name("GateId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_gate_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_gate_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_gate_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_gate_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// GateControl
/******************************************************************************/
struct GateControl {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_gate_id_t>, expose::ParamVal<vtss_appl_psfp_gate_control_t *>> P;

    static constexpr const char *table_description = "Table for clearing various stream gate status";
    static constexpr const char *index_description = "Each index in the table represents a stream gate instance";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_gate_id_t &i)
    {
        h.argument_properties(tag::Name("GateId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_gate_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_gate_control_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_gate_itr);
    VTSS_EXPOSE_GET_PTR(psfp_serializer_gate_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psfp_gate_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FilterDefaultConf
/******************************************************************************/
struct FilterDefaultConf {
    typedef expose::ParamList <expose::ParamVal<vtss_appl_psfp_filter_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_filter_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_filter_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FilterConf
/******************************************************************************/
struct FilterConf {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_filter_id_t>, expose::ParamVal<vtss_appl_psfp_filter_conf_t *>> P;

    static constexpr uint32_t   snmpRowEditorOid   = 100;
    static constexpr const char *table_description = "This table holds the configuration of PSFP stream filters";
    static constexpr const char *index_description = "Each index points to a given filter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_filter_id_t &i)
    {
        h.argument_properties(tag::Name("FilterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_filter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_filter_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_filter_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_filter_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psfp_filter_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_psfp_filter_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_psfp_filter_conf_del);
    VTSS_EXPOSE_DEF_PTR(psfp_serializer_filter_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FilterStatus
/******************************************************************************/
struct FilterStatus {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_filter_id_t>, expose::ParamVal<vtss_appl_psfp_filter_status_t *>> P;

    static constexpr const char *table_description = "This table holds the status of PSFP stream filters";
    static constexpr const char *index_description = "Each index points to a given filter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_filter_id_t &i)
    {
        h.argument_properties(tag::Name("FilterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_filter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_filter_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_filter_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_filter_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FilterControl
/******************************************************************************/
struct FilterControl {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_filter_id_t>, expose::ParamVal<vtss_appl_psfp_filter_control_t *>> P;

    static constexpr const char *table_description = "Table for clearing stream filter status";
    static constexpr const char *index_description = "Each index in the table represents a stream filter instance";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_filter_id_t &i)
    {
        h.argument_properties(tag::Name("FilterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_filter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_filter_control_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_filter_itr);
    VTSS_EXPOSE_GET_PTR(psfp_serializer_filter_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psfp_filter_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FilterStatistics
/******************************************************************************/
struct FilterStatistics {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_filter_id_t>, expose::ParamVal<vtss_appl_psfp_filter_statistics_t *>> P;

    static constexpr const char *table_description = "This table holds statistics of PSFP stream filters";
    static constexpr const char *index_description = "Each index points to a given filter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_filter_id_t &i)
    {
        h.argument_properties(tag::Name("FilterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_filter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psfp_filter_statistics_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(21));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_filter_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psfp_filter_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PSFP);
};

/******************************************************************************/
// FilterStatisticsClear()
/******************************************************************************/
struct FilterStatisticsClear {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_psfp_filter_id_t>, expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description = "This is a table to clear stream filter statistics";
    static constexpr const char *index_description = "Each index points to a given stream filter ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psfp_filter_id_t &i)
    {
        h.argument_properties(tag::Name("FilterId"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, psfp_serializer_filter_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, psfp_serializer_filter_statistics_clear_value(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_psfp_filter_itr);
    VTSS_EXPOSE_GET_PTR(psfp_serializer_filter_statistics_clear_get);
    VTSS_EXPOSE_SET_PTR(psfp_serializer_filter_statistics_clear_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PSFP);
};

}  // namespace interfaces
}  // namespace psfp
}  // namespace appl
}  // namespace vtss

#endif  // _PSFP_SERIALIZER_HXX_
