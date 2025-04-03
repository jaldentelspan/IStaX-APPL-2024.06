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

#ifndef _FRER_SERIALIZER_HXX_
#define _FRER_SERIALIZER_HXX_

#include "vtss/appl/frer.hxx"
#include "vtss/appl/module_id.h"
#include "vtss/appl/types.hxx"
#include "vtss_appl_serialize.hxx"
#include "mgmt_api.h"              /* For mgmt_port_list_XXX_stackable() */

/******************************************************************************/
// frer_serializer_control_get()
/******************************************************************************/
static inline mesa_rc frer_serializer_control_get(uint32_t instance, vtss_appl_frer_control_t *ctrl)
{
    if (ctrl) {
        ctrl->reset              = false;
        ctrl->latent_error_clear = false;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_serializer_conf_t()
/******************************************************************************/
struct frer_serializer_conf_t {
    /**
     * The original frer configuration
     */
    vtss_appl_frer_conf_t conf;

    /**
     * conf.egress_ports converted to PortListStackable
     */
    vtss_port_list_stackable_t egress_port_list_stackable;
};

/******************************************************************************/
// frer_serializer_conf_default_get()
/******************************************************************************/
static inline mesa_rc frer_serializer_conf_default_get(uint32_t *instance, frer_serializer_conf_t *s)
{
    if (!instance || !s) {
        return VTSS_RC_ERROR;
    }

    *instance = 0;

    VTSS_RC(vtss_appl_frer_conf_default_get(&s->conf));
    mgmt_port_list_to_stackable(s->egress_port_list_stackable, s->conf.egress_ports);

    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_serializer_conf_default_get_no_inst()
/******************************************************************************/
static inline mesa_rc frer_serializer_conf_default_get_no_inst(frer_serializer_conf_t *s)
{
    uint32_t inst;
    return frer_serializer_conf_default_get(&inst, s);
}

/******************************************************************************/
// frer_serializer_conf_get()
/******************************************************************************/
static inline mesa_rc frer_serializer_conf_get(uint32_t instance, frer_serializer_conf_t *s)
{
    if (!s) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_frer_conf_get(instance, &s->conf));
    mgmt_port_list_to_stackable(s->egress_port_list_stackable, s->conf.egress_ports);
    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_serializer_conf_set()
/******************************************************************************/
static inline mesa_rc frer_serializer_conf_set(uint32_t instance, const frer_serializer_conf_t *s)
{
    vtss_appl_frer_conf_t conf;

    if (!s) {
        return VTSS_RC_ERROR;
    }

    conf = s->conf;
    mgmt_port_list_from_stackable(s->egress_port_list_stackable, conf.egress_ports);
    return vtss_appl_frer_conf_set(instance, &conf);
}

/******************************************************************************/
// frer_serializer_statistics_clear()
/******************************************************************************/
static inline mesa_rc frer_serializer_statistics_clear(uint32_t instance, const BOOL *clear)
{
    if (clear && *clear) {
        VTSS_RC(vtss_appl_frer_statistics_clear(instance));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_serializer_statistics_clear_dummy()
/******************************************************************************/
static inline mesa_rc frer_serializer_statistics_clear_dummy(uint32_t instance,  BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}

static inline const vtss_enum_descriptor_t frer_serializer_mode_txt[] = {
    {VTSS_APPL_FRER_MODE_GENERATION, "generation"},
    {VTSS_APPL_FRER_MODE_RECOVERY,   "recovery"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_frer_mode_t,
                         "FrerFrerMode",
                         frer_serializer_mode_txt,
                         "This enumeration defines whether FRER runs in generation or recovery mode.");

static inline const vtss_enum_descriptor_t frer_serializer_rcvy_algorithm_txt[] = {
    {MESA_FRER_RECOVERY_ALG_VECTOR, "vector"},
    {MESA_FRER_RECOVERY_ALG_MATCH,  "match"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(mesa_frer_recovery_alg_t,
                         "FrerRecoveryAlg",
                         frer_serializer_rcvy_algorithm_txt,
                         "This enumeration defines the chosen recovery algorithm.");

static inline const vtss_enum_descriptor_t frer_serializer_oper_state_txt[] = {
    {VTSS_APPL_FRER_OPER_STATE_ADMIN_DISABLED,  "disabled"},
    {VTSS_APPL_FRER_OPER_STATE_ACTIVE,          "active"},
    {VTSS_APPL_FRER_OPER_STATE_INTERNAL_ERROR,  "internalError"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_frer_oper_state_t,
                         "FrerOperState",
                         frer_serializer_oper_state_txt,
                         "Operational state of a FRER instance");

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_frer_ifindex_t, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface index."));
}

VTSS_SNMP_TAG_SERIALIZE(FrerInstance, uint32_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Id"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The FRER instance ID"));
}

VTSS_SNMP_TAG_SERIALIZE(StreamInstance, vtss_appl_stream_id_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("StreamId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The Stream ID"));
}

VTSS_SNMP_TAG_SERIALIZE(frer_serializer_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Set to TRUE to clear the counters of an FRER instance."));
}

template <typename T>
void serialize(T &a, vtss_appl_frer_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_frer_capabilities_t"));
    int               ix = 0;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstanceMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of creatable FRER instances."
                                      "This value is 0 if FRER is not supported on this platform."));

    m.add_leaf(s.rcvy_mstream_cnt_max,
               vtss::tag::Name("MstreamMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of creatable member streams."));

    m.add_leaf(s.rcvy_cstream_cnt_max,
               vtss::tag::Name("CstreamMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of creatable compound streams."));

    m.add_leaf(s.rcvy_reset_timeout_ms_min,
               vtss::tag::Name("ResetTimeoutMsecMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum value to configure for reset timer (in milliseconds)."));

    m.add_leaf(s.rcvy_reset_timeout_ms_max,
               vtss::tag::Name("ResetTimeoutMsecMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum value to configure for reset timer (in milliseconds)."));

    m.add_leaf(s.rcvy_history_len_min,
               vtss::tag::Name("HistorylenMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum history length (when using vector recovery algorithm)."));

    m.add_leaf(s.rcvy_history_len_max,
               vtss::tag::Name("HistorylenMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum history length (when using vector recovery algorithm)."));

    m.add_leaf(s.rcvy_latent_error_difference_min,
               vtss::tag::Name("LaErrDifferenceMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum allowed sequence number distance used in latent error detection."));

    m.add_leaf(s.rcvy_latent_error_difference_max,
               vtss::tag::Name("LaErrDifferenceMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum allowed sequence number distance used in latent error detection."));

    m.add_leaf(s.rcvy_latent_error_period_ms_min,
               vtss::tag::Name("LaErrPeriodMsecMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum configurable latent error period in milliseconds."));

    m.add_leaf(s.rcvy_latent_error_period_ms_max,
               vtss::tag::Name("LaErrPeriodMsecMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum configurable latent error period in milliseconds."));

    m.add_leaf(s.rcvy_latent_error_paths_min,
               vtss::tag::Name("LaErrPathsMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum allowed number of paths used in latent error detection."));

    m.add_leaf(s.rcvy_latent_error_paths_max,
               vtss::tag::Name("LaErrPathsMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum allowed number of paths used in latent error detection.."));

    m.add_leaf(s.rcvy_latent_reset_period_ms_min,
               vtss::tag::Name("LaErrResetPeriodMsecMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum configurable latent error reset period in milliseconds."));

    m.add_leaf(s.rcvy_latent_reset_period_ms_max,
               vtss::tag::Name("LaErrResetPeriodMsecMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum configurable latent error reset period in milliseconds."));

    m.add_leaf(s.egress_port_cnt_max,
               vtss::tag::Name("EgressPortCntMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of egress ports."));

    m.add_leaf(s.stream_id_max,
               vtss::tag::Name("StreamIdMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum value of a Stream ID."));

    m.add_leaf(s.stream_collection_id_max,
               vtss::tag::Name("StreamCollectionIdMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum value of a Stream Collection ID."));
}

template <typename T>
void serialize(T &a, frer_serializer_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_frer_conf_t"));
    int               ix = 2;
    uint32_t          i;
    char              namebuf[128], dscrbuf[128];

    // Using frer_serializer_mode_txt[]
    m.add_leaf(s.conf.mode,
               vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Mode that this instance is running in."));

    m.add_leaf(s.conf.frer_vlan,
               vtss::tag::Name("FrerVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The VLAN ID that the streams identified by stream_ids get classified to."));

    vtss::PortListStackable &egress_ports = (vtss::PortListStackable &)s.egress_port_list_stackable;

    m.add_leaf(egress_ports,
               vtss::tag::Name("EgressPorts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("List of Egress ports."));

    // Utilizing frer_serializer_rcvy_algorithm_txt[]
    m.add_leaf(s.conf.rcvy_algorithm,
               vtss::tag::Name("Algorithm"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The chosen recovery algorithm."));

    m.add_leaf(s.conf.rcvy_history_len,
               vtss::tag::Name("HistoryLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The vector recovery algorithm's history length."));

    m.add_leaf(s.conf.rcvy_reset_timeout_ms,
               vtss::tag::Name("ResetTimeoutMsec"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The reset time in milliseconds of the recovery algorithm."));

    m.add_leaf(vtss::AsBool(s.conf.rcvy_take_no_sequence),
               vtss::tag::Name("TakeNoSequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls latent error detection enabledness. "
                                      "Indicates whether individual recovery is enabled on this FRER instance. "
                                      "When individual recovery is enabled, each member flow will run the "
                                      "selected recovery algorithm before passing it to the compound recovery, "
                                      "allowing for detecting a sender that sends the same R-Tag sequence number "
                                      "over and over again (see e.g. 802.1CB, Annex C.10)."));

    m.add_leaf(vtss::AsBool(s.conf.rcvy_individual),
               vtss::tag::Name("IndividualRecovery"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether we are an end system that terminates recovery and "
                                      "removes the R-tag before the frames are egressing."));

    m.add_leaf(vtss::AsBool(s.conf.rcvy_terminate),
               vtss::tag::Name("Terminate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether we are an end system that terminates recovery "
                                      "and removes the R-tag before the frames are egressing."));

    m.add_leaf(vtss::AsBool(s.conf.rcvy_latent_error_detection.enable),
               vtss::tag::Name("LaErrDetection"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls latent error detection enabledness."));

    m.add_leaf(s.conf.rcvy_latent_error_detection.difference,
               vtss::tag::Name("LaErrDifference"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Specifies the maximum distance between the number of discarded packets "
                                      "and the number of member streams multiplied by the number of passed "
                                      "packets. Any larger difference will trigger the detection of a latent "
                                      "error by the latent error test function."));

    m.add_leaf(s.conf.rcvy_latent_error_detection.period_ms,
               vtss::tag::Name("LaErrPeriodMsec"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of milliseconds between running the latent error test function."));

    m.add_leaf(s.conf.rcvy_latent_error_detection.paths,
               vtss::tag::Name("LaErrPaths"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of paths that the latent error detection function operates on."));

    m.add_leaf(s.conf.rcvy_latent_error_detection.reset_period_ms,
               vtss::tag::Name("LaErrResetPeriodMsec"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of milliseconds between running the latent error reset function."));

    m.add_leaf(vtss::AsBool(s.conf.admin_active),
               vtss::tag::Name("AdminActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls whether this instance is active or not."));

    ix = 50;
    for (i = 0; i < VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX; i++) {
        sprintf(namebuf, "StreamId%u", i);
        sprintf(dscrbuf, "Stream identity list element %u", i);
        m.add_leaf(s.conf.stream_ids[i],
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscrbuf));
    }

    m.add_leaf(s.conf.stream_collection_id,
               vtss::tag::Name("StreamCollectionId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If set to 0, StreamIds are used, otherwise stream collections are used. Stream collections cannot be used with individual recovery."));

    m.add_leaf(vtss::AsBool(s.conf.outer_tag_pop),
               vtss::tag::Name("IngressOuterTagPop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If set to true, a possible VLAN tag of the frames matching the "
                                      "stream or stream collection will be stripped. "
                                      "Only used in generation mode."));
}

template <typename T>
void serialize(T &a, vtss_appl_frer_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_frer_status_t"));
    mesa_bool_t       b;
    int               ix = 1;

    // Using frer_serializer_oper_state_txt[]
    m.add_leaf(s.oper_state,
               vtss::tag::Name("OperState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Operational state of this FRER instance."));

    m.add_leaf(vtss::AsBool(s.notif_status.latent_error),
               vtss::tag::Name("LatentError"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If latent error detection is enabled and a SIGNAL_LATENT_ERROR "
                                      "is raised, this member will become true."));

    // The warnings come in a perculiar order, because some are new and some are
    // obsoletem, and we must preserve OID order.
    b = s.oper_warnings == VTSS_APPL_FRER_OPER_WARNING_NONE;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningNone"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("No warnings found."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STREAM_NOT_FOUND) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamNotFound"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the matching streams doesn't exist."));

    b = false;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningNoIngressPorts"),
               vtss::expose::snmp::Status::Obsoleted,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Obsolete. Will always be false. Use WarningStreamHasConfigurationalWarnings."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STREAM_ATTACH_FAIL) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamAttachFail"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Failed to attach to at least one of the streams, possibly because it is part of a stream collection."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_INGRESS_EGRESS_OVERLAP) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningIngressEgressOverlap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("There is an overlap between ingress and egress ports."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_EGRESS_PORT_CNT) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningEgressPortCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("In generation mode, only one egress port is specified."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_INGRESS_NO_LINK) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningIngressNoLink"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the ingress ports doesn't have link."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_EGRESS_NO_LINK) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningEgressNoLink"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the egress ports doesn't have link."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_VLAN_MEMBERSHIP) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningVlanMembership"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the egress ports is not member of the FRER VLAN."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STP_BLOCKED) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStpBlocked"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the egress ports is blocked by STP."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_MSTP_BLOCKED) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningMstpBlocked"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the egress ports is blocked by MSTP."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamHasConfigurationalWarnings"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("At least one of the ingress streams has configurational warnings."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamCollectionNotFound"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Stream collection doesn't exist."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamCollectionAttachFail"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Failed to attach to stream collection."));

    b = (s.oper_warnings & VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS) != 0;
    m.add_leaf(vtss::AsBool(b), vtss::tag::Name("WarningStreamCollectionHasConfigurationalWarnings"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The specified stream collection has configurational warnings."));
}

template <typename T>
void serialize(T &a, vtss_appl_frer_notification_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_frer_notification_status_t"));
    int               ix = 1;

    m.add_leaf(vtss::AsBool(s.latent_error),
               vtss::tag::Name("LatentError"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If latent error detection is enabled and a SIGNAL_LATENT_ERROR "
                                      "is raised, this member will become true."));
}

template <typename T>
void serialize(T &a, vtss_appl_frer_statistics_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_frer_statistics_t"));
    int               ix = 1;

    m.add_leaf(vtss::AsCounter(s.rcvy_out_of_order_packets),
               vtss::tag::Name("OutOfOrder"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyOutOfOrderPackets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_rogue_packets),
               vtss::tag::Name("Rogue"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyRoguePackets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_passed_packets),
               vtss::tag::Name("Passed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyPassedPackets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_discarded_packets),
               vtss::tag::Name("Discarded"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyDiscardedPackets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_lost_packets),
               vtss::tag::Name("Lost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyLostPackets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_tagless_packets),
               vtss::tag::Name("Tagless"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyTaglessPackets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_resets),
               vtss::tag::Name("RecoveryResets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyResets"));

    m.add_leaf(vtss::AsCounter(s.rcvy_latent_error_resets),
               vtss::tag::Name("LaErrResets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqRcvyLatentErrorResets (S/W counted)"));

    m.add_leaf(vtss::AsCounter(s.gen_resets),
               vtss::tag::Name("GenerationResets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqGenResets (S/W counted)"));

    m.add_leaf(vtss::AsCounter(s.gen_matches),
               vtss::tag::Name("GenerationMatches"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("frerCpsSeqGenMatches"));
}

//----------------------------------------------------------------------------
// Control
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_frer_control_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_frer_control_t"));
    int               ix = 3;

    m.add_leaf(vtss::AsBool(s.reset),
               vtss::tag::Name("Reset"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If this FRER instance is in generation mode, this member is used to reset the"
                                      "sequence number of the sequence generator."
                                      "If this FRER instance is in recovery mode, this member is used to reset the"
                                      "recovery function. It resets both possible individual recovery functions"
                                      "and the compound recovery functions. A value of false has no effect."));

    m.add_leaf(vtss::AsBool(s.latent_error_clear),
               vtss::tag::Name("LaErrClear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Clear a sticky latent error."
                                      "If this FRER instance is in generation mode, this member has no effect."
                                      "If this FRER instance is in recovery mode, but latent error detection is"
                                      "disabled, this member has no effect."
                                      "A value of false has no effect."));
}

namespace vtss
{
namespace appl
{
namespace frer
{
namespace interfaces
{

/******************************************************************************/
// FrerCapabilities
/******************************************************************************/
struct FrerCapabilities {
    typedef vtss::expose::ParamList <vtss::expose::ParamVal<vtss_appl_frer_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_frer_capabilities_t &s)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, s);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_frer_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerDefaultConf
/******************************************************************************/
struct FrerDefaultConf {
    typedef expose::ParamList <expose::ParamVal<frer_serializer_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(frer_serializer_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(frer_serializer_conf_default_get_no_inst);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerConf
/******************************************************************************/
struct FrerConf {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<frer_serializer_conf_t * >> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description = "This is the FRER instance configuration table.";
    static constexpr const char *index_description = "Each entry in this table represents a FRER instance";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(tag::Name("id"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, FrerInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(frer_serializer_conf_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(frer_serializer_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_frer_itr);
    VTSS_EXPOSE_SET_PTR(frer_serializer_conf_set);
    VTSS_EXPOSE_ADD_PTR(frer_serializer_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_frer_conf_del);
    VTSS_EXPOSE_DEF_PTR(frer_serializer_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerStatus
/******************************************************************************/
struct FrerStatus {
    typedef vtss::expose::ParamList <vtss::expose::ParamKey<uint32_t>, vtss::expose::ParamVal<vtss_appl_frer_status_t *>> P;

    static constexpr const char *table_description = "This is a table of created FRER instance status.";
    static constexpr const char *index_description = "This is a created FRER instance status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        serialize(h, FrerInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_frer_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_frer_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_frer_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerNotifStatus
/******************************************************************************/
struct FrerNotifStatus {
    typedef vtss::expose::ParamList <vtss::expose::ParamKey<uint32_t>, vtss::expose::ParamVal<vtss_appl_frer_notification_status_t *>> P;

    static constexpr const char *table_description = "This is a table of created FRER instance notification status.";
    static constexpr const char *index_description = "This is a created FRER instance notification status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        serialize(h, FrerInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_frer_notification_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_frer_notification_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_frer_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerStatistics
/******************************************************************************/
struct FrerStatistics {
    typedef vtss::expose::ParamList <vtss::expose::ParamKey<uint32_t>, vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamKey<vtss_appl_stream_id_t>, vtss::expose::ParamVal<vtss_appl_frer_statistics_t *>> P;

    static constexpr const char *table_description = "This is a table of FRER instance statistics.";
    static constexpr const char *index_description = "FRER statistics.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        serialize(h, FrerInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_frer_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_stream_id_t &i)
    {
        serialize(h, StreamInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_frer_statistics_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_frer_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_frer_statistics_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerStatisticsClear
/******************************************************************************/
struct FrerStatisticsClear {
    typedef vtss::expose::ParamList <vtss::expose::ParamKey<uint32_t>, vtss::expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description = "This is a table of FRER clear commands.";
    static constexpr const char *index_description = "This is a created FRER clear command.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, FrerInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, frer_serializer_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_frer_itr);
    VTSS_EXPOSE_SET_PTR(frer_serializer_statistics_clear);
    VTSS_EXPOSE_GET_PTR(frer_serializer_statistics_clear_dummy);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRER);
};

/******************************************************************************/
// FrerControl
/******************************************************************************/
struct FrerControl {
    typedef vtss::expose::ParamList <vtss::expose::ParamKey<uint32_t>, vtss::expose::ParamVal<vtss_appl_frer_control_t *>> P;

    static constexpr const char *table_description = "This is the FRER instance control table.";
    static constexpr const char *index_description = "Each entry in this table represents dynamic control elements an FRER instance.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, FrerInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_frer_control_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(frer_serializer_control_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_frer_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_frer_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRER);
};

}
}
}
}

#endif // _FRER_SERIALIZER_HXX_

