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
#ifndef __VTSS_TSN_SERIALIZER_HXX__
#define __VTSS_TSN_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/tsn.h"
#include "vtss/basics/expose/json.hxx"
#include "vtss_appl_formatting_tags.hxx" // for AsInterfaceIndex
#include <vtss/appl/types.hxx>


// Generic serializer for vtss_ifindex_t
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_tsn_ifindex_t, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface index."));
}

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t vtss_appl_tsn_procedure_txt[];
VTSS_XXXX_SERIALIZE_ENUM(appl_tsn_time_start_procedure_t,
                         "ProcedureForTimeStatus",
                         vtss_appl_tsn_procedure_txt,
                         "Procedure to use when checking for time status");

extern const vtss_enum_descriptor_t vtss_appl_tsn_start_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(appl_tsn_start_state_t,
                         "StateForTimeStatus",
                         vtss_appl_tsn_start_state_txt,
                         "State of starting process");


template<typename T>
void serialize(T &a, vtss_appl_tsn_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_capabilities_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.has_queue_frame_preemption),
        vtss::tag::Name("HasQueueFramePreemption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, the platform supports frame preemption in egress queue.")
    );

    m.add_leaf(
        vtss::AsBool(s.has_tas),
        vtss::tag::Name("HasTas"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, the platform supports Time Aware Shaper.")
    );

    m.add_leaf(
        vtss::AsBool(s.has_frer),
        vtss::tag::Name("HasFrer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, the platform supports FRER.")
    );

    m.add_leaf(
        vtss::AsBool(s.has_psfp),
        vtss::tag::Name("HasPsfp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, the platform supports PSFP.")
    );

    m.add_leaf(
        s.min_add_frag_size,
        vtss::tag::Name("MinAddFragSize"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimal value of IEEE802.3br: aMACMergeAddFragSize.")
    );

    m.add_leaf(
        vtss::AsBool(s.tas_mac_restrict),
        vtss::tag::Name("TasHasMacRestrict"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If true, hold and release MAC operations are restricted.")
    );

    m.add_leaf(
        s.tas_max_gce_cnt,
        vtss::tag::Name("TasMaxGceCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum number of Gate Control Elements in a TAS ControlList.")
    );

    m.add_leaf(
        s.tas_max_sdu_min,
        vtss::tag::Name("TasMinSdu"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The Minimum value of TAS maxSDU.")
    );

    m.add_leaf(
        s.tas_max_sdu_max,
        vtss::tag::Name("TasMaxSdu"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum value of TAS maxSDU.")
    );

    m.add_leaf(
        s.tas_ct_min,
        vtss::tag::Name("TasMinCycleTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The Minimum TAS cycle time in nano seconds supported.")
    );

    m.add_leaf(
        s.tas_ct_max,
        vtss::tag::Name("TasMaxCycleTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The Minimum TAS cycle time in nano seconds supported.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_tsn_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_global_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.procedure,
        vtss::tag::Name("Procedure"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Procedure")
    );

    m.add_leaf(
        s.timeout,
        vtss::tag::Name("Timeout"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("timeout")
    );

    m.add_leaf(
        s.ptp_port,
        vtss::tag::Name("PtpPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("ptp_port")
    );

    m.add_leaf(
        s.clock_domain,
        vtss::tag::Name("ClockDomain"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("clock_domain")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_tsn_global_state_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_global_state_t"));
    int ix = 0;

    m.add_leaf(
        s.start_state,
        vtss::tag::Name("StartState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Procedure")
    );

    m.add_leaf(
        s.time_passed,
        vtss::tag::Name("TimePassed"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Time passed")
    );

    m.add_leaf(
        s.start,
        vtss::tag::Name("Start"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Start")
    );
}



namespace vtss
{
namespace appl
{
namespace tsn
{
namespace interfaces
{

struct TsnConfigGlobalsLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_tsn_global_conf_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_tsn_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_tsn_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_tsn_capabilities_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_tsn_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnStatusGlobalsLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_tsn_global_state_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_tsn_global_state_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_global_state_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TSN);
};


}  // namespace interfaces
}  // namespace tsn
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_TSN_SERIALIZER_HXX__ */
