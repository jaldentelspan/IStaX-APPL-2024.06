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
#ifndef __VTSS_TSN_TAS_SERIALIZER_HXX__
#define __VTSS_TSN_TAS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/tsn.h"
#include "vtss/basics/expose/json.hxx"
#include "vtss_appl_formatting_tags.hxx" // for AsInterfaceIndex
#include <vtss/appl/types.hxx>

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t vtss_appl_tsn_tas_gco_txt[];
VTSS_XXXX_SERIALIZE_ENUM(mesa_qos_tas_gco_t,
                         "Gate Control Operation",
                         vtss_appl_tsn_tas_gco_txt,
                         "Gate Control Operation.");


/****************************************************************************
 * Generic index serializers
 ****************************************************************************/

// Generic serializer for mesa_prio_t used as queue index
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_qos_queue_index_t, mesa_prio_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Queue"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Queue index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_tsn_tas_gcl_index_t, vtss_gcl_index_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("GclIndex"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 63),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Gate control list index."));
}


namespace vtss
{
namespace appl
{
namespace tsn
{
namespace interfaces
{

template<typename T>
void serialize(T &a, vtss_appl_tsn_tas_cfg_global_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_tas_cfg_global_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.always_guard_band),
        vtss::tag::Name("AlwaysGuardBand"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("When set, a guard band is implemented even for scheduled queues \n"
                               "to scheduled queue transition.\n"
                               "false: Guard band is implemented for non-scheduled queues to scheduled queues transition.\n"
                               "true: Guard band is implemented for any queue to scheduled queues transition."));

}

template<typename T>
void serialize(T &a, vtss_appl_tsn_tas_cfg_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_tas_cfg_t"));

    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.gate_enabled),
        vtss::tag::Name("GateEnabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The GateEnabled parameter determines whether traffic scheduling\n"
                               "is active (true) or inactive (false).\n"));

    m.add_leaf(
        vtss::AsOctetString(&s.admin_gate_states, 1),
        vtss::tag::Name("AdminGateStates"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the GateStates parameter for the Port.\n"
                               "The bits of the octet represent the gate states for the\n"
                               "corresponding traffic classes;the MS bit corresponds to traffic class 7,\n"
                               "the LS bit to traffic class 0. A bit value of 0 indicates closed; a\n"
                               "bit value of 1 indicates open.\n"));

    m.add_leaf(
        s.admin_control_list_length,
        vtss::tag::Name("AdminControlListLength"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the ListMax parameter for the Port.\n"
                               "The integer value indicates the number of entries (TLVs) in the\n"
                               "AdminControlList.\n"));

    m.add_leaf(
        s.admin_cycle_time_numerator,
        vtss::tag::Name("AdminCycleTimeNumerator"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the numerator of the CycleTime\n"
                               "parameter for the Port.\n"
                               "The numerator and denominator together represent the cycle time as\n"
                               "a rational number of seconds.\n"));

    m.add_leaf(
        s.admin_cycle_time_denominator,
        vtss::tag::Name("AdminCycleTimeDenominator"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the denominator of the\n"
                               "CycleTime parameter for the Port.\n"
                               "The numerator and denominator together represent the cycle time as\n"
                               "a rational number of seconds.\n"));

    m.add_leaf(
        s.admin_cycle_time_extension,
        vtss::tag::Name("AdminCycleTimeExtension"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the CycleTimeExtension\n"
                               "parameter for the Port.\n"
                               "The value is an unsigned integer number of nanoseconds.\n"));

    m.add_leaf(
        s.admin_base_time,
        vtss::tag::Name("AdminBaseTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the BaseTime parameter for the Port.\n"
                               "The value is a representation of a PTPtime value, \n"
                               "consisting of a 48-bit integer\n"
                               "number of seconds and a 32-bit integer number of nanoseconds.\n"));

    m.add_leaf(
        vtss::AsTextualTimestamp(s.admin_base_time),
        vtss::tag::Name("AdminBaseTimeTxt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative value of the BaseTime parameter for the gate. "
                               "The value is represented in the format YYYY/MM/DD hh:mm:ss[.sssssssss]"));

    m.add_leaf(
        vtss::AsBool(s.config_change),
        vtss::tag::Name("ConfigChange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ConfigChange parameter signals the start of a\n"
                               "configuration change\n"
                               "when it is set to TRUE. This should only be done\n"
                               "when the various administrative parameters\n"
                               "are all set to appropriate values.\n"));
}

template<typename T>
void serialize(T &a, vtss_appl_tsn_tas_max_sdu_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_tas_max_sdu_t"));

    m.add_leaf(
        s.max_sdu,
        vtss::tag::Name("MaxSDU"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The value of the MaxSDU parameter for the traffic class.\n"
                               "This value is represented as an unsigned integer. A value\n"
                               "of 0 is interpreted as the max SDU size supported by\n"
                               "the underlying MAC.\n"
                               "The default value of the MaxSDU parameter is 0.\n"));
}

template<typename T>
void serialize(T &a, vtss_appl_tsn_tas_gcl_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_tas_gcl_t"));

    int ix = 0;

    m.add_leaf(
        s.gate_operation,
        vtss::tag::Name("GCO"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Gate Control Operation"));

    m.add_leaf(
        vtss::AsOctetString(&s.gate_state, sizeof(s.gate_state)),
        vtss::tag::Name("GateState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("A GateState parameter is encoded in a single octet.\n"
                               "The bits of the octet represent the gate states for the\n"
                               "corresponding traffic classes;the MS bit corresponds to\n"
                               "traffic class 7, the LS bit to traffic class 0.\n"
                               "A bit value of 0 indicates closed; a\n"
                               "bit value of 1 indicates open.\n"));

    m.add_leaf(
        s.time_interval,
        vtss::tag::Name("TimeInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("A TimeInterval is encoded in 4 octets as a 32-bit \n"
                               "unsigned integer, representing\n"
                               "a number of nanoseconds. The first octet encodes the\n"
                               "most significant 8 bits of the integer, and the fourth\n"
                               "octet encodes the least significant 8 bits.\n"));

}


template<typename T>
void serialize(T &a, vtss_appl_tsn_tas_oper_state_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_tas_oper_state_t"));

    int ix = 0;

    m.add_leaf(
        vtss::AsOctetString(&s.oper_gate_states, 1),
        vtss::tag::Name("OperGateStates"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operational value of the GateStates parameter for the Port.\n"
                               "The bits of the octet represent the gate states for the \n"
                               "corresponding traffic classes;the MS bit corresponds to traffic class 7,\n"
                               "the LS bit to traffic class 0. A bit value of 0 indicates closed;a \n"
                               "bit value of 1 indicates open.\n"));

    m.add_leaf(
        s.oper_control_list_length,
        vtss::tag::Name("OperControlListLength"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operational value of the ListMax parameter for the Port.\n"
                               "The integer value indicates the number of entries (TLVs) in the\n"
                               "OperControlList.\n"));

    m.add_leaf(
        s.oper_cycle_time_numerator,
        vtss::tag::Name("OperCycleTimeNumerator"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operational value of the numerator of the \n"
                               "CycleTime parameter for the Port.\n"
                               "The numerator and denominator together represent the cycle\n"
                               "time as a rational number of seconds.\n"));

    m.add_leaf(
        s.oper_cycle_time_denominator,
        vtss::tag::Name("OperCycleTimeDenominator"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operational value of the denominator of the \n"
                               "CycleTime parameter for the Port.\n"
                               "The numerator and denominator together represent the \n"
                               "cycle time as a rational number of seconds.\n"));

    m.add_leaf(
        s.oper_cycle_time_extension,
        vtss::tag::Name("OperCycleTimeExtension"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operational value of the CycleTimeExtension parameter for the Port.\n"
                               "The value is an unsigned integer number of nanoseconds.\n"));

    m.add_leaf(
        s.oper_base_time,
        vtss::tag::Name("OperBaseTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operationsl value of the BaseTime parameter for the Port.\n"
                               "The value is a representation of a PTPtime value, \n"
                               "consisting of a 48-bit integer\n"
                               "number of seconds and a 32-bit integer number of nanoseconds.\n"));

    m.add_leaf(
        vtss::AsTextualTimestamp(s.oper_base_time),
        vtss::tag::Name("OperBaseTimeTxt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The operational value of the BaseTime parameter for the gate. "
                               "The value is represented in the format "
                               "YYYY/MM/DD hh:mm:ss[.sssssssss]"));

    m.add_leaf(
        s.config_change_time,
        vtss::tag::Name("ConfigChangeTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The PTPtime at which the next config change is scheduled to occur.\n"
                               "The value is a representation of a PTPtime value, \n"
                               "consisting of a 48-bit integer\n"
                               "number of seconds and a 32-bit integer number of nanoseconds.\n"));

    m.add_leaf(
        vtss::AsTextualTimestamp(s.config_change_time),
        vtss::tag::Name("ConfigChangeTimeTxt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The PTPtime at which the next config change is scheduled to occur. "
                               "The value is represented in the format YYYY/MM/DD hh:mm:ss[.sssssssss]"));

    m.add_leaf(
        s.tick_granularity,
        vtss::tag::Name("TickGranularity"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The granularity of the cycle time clock, represented as an\n"
                               "unsigned number of tenths of nanoseconds.\n"));

    m.add_leaf(
        s.current_time,
        vtss::tag::Name("CurrentTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The current time, in PTPtime, as maintained by the local system.\n"
                               "The value is a representation of a PTPtime value, \n"
                               "consisting of a 48-bit integer\n"
                               "number of seconds and a 32-bit integer number of nanoseconds.\n"));

    m.add_leaf(
        vtss::AsTextualTimestamp(s.current_time),
        vtss::tag::Name("CurrentTimeTxt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The current time, in PTPtime, as maintained by the local system. "
                               "The value is represented in the format YYYY/MM/DD hh:mm:ss[.sssssssss]"));

    m.add_leaf(
        vtss::AsBool(s.config_pending),
        vtss::tag::Name("ConfigPending"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The value of the ConfigPending state machine variable.\n"
                               "The value is TRUE if a configuration change is in progress\n"
                               "but has not yet completed.\n"));

    m.add_leaf(
        vtss::AsCounter(s.config_change_error),
        vtss::tag::Name("ConfigChangeError"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("A counter of the number of times that a re-configuration\n"
                               "of the traffic schedule has been requested with the old\n"
                               "schedule still running and the requested base time was\n"
                               "in the past.\n"));

    m.add_leaf(
        s.supported_list_max,
        vtss::tag::Name("SupportedListMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value supported by this Port of the \n"
                               "AdminControlListLength and OperControlListLength\n"
                               "parameters.\n"));
}

struct TsnTasConfigGlobalsLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_tsn_tas_cfg_global_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_tsn_tas_cfg_global_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_tas_cfg_get_global);
    VTSS_EXPOSE_SET_PTR(vtss_appl_tsn_tas_cfg_set_global);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnTasPerQMaxSduEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<mesa_prio_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_tas_max_sdu_t *>
         > P;

    static constexpr const char *table_description =
        "A table containing a set of max SDU parameters, one for each traffic class.\n"
        "All writeable objects in this table must be persistent over power up restart/reboot.\n";

    static constexpr const char *index_description =
        "Each row contains the maximum SDU value for a specific queue.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_qos_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_tsn_tas_max_sdu_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_tas_per_q_max_sdu_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_tas_per_q_max_sdu_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_tsn_tas_per_q_max_sdu_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnTasGclAdminEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<vtss_gcl_index_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_tas_gcl_t *>
         > P;

    static constexpr const char *table_description =
        "A table containing the admin entries/TLVs of the GCL, as defined in IEEE 802.1Qbv";

    static constexpr const char *index_description =
        "Each row contains single GCL entry with a unique gcl_index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_gcl_index_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_tsn_tas_gcl_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_tsn_tas_gcl_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_tas_gcl_admin_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_tas_gcl_admin_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_tsn_tas_gcl_admin_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnTasGclOperEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<vtss_gcl_index_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_tas_gcl_t *>
         > P;

    static constexpr const char *table_description =
        "A table containing the operational entries/TLVs of the GCL, as defined in IEEE 802.1Qbv";

    static constexpr const char *index_description =
        "Each row contains single GCL entry with a unique gcl_index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_gcl_index_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_tsn_tas_gcl_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_tsn_tas_gcl_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_tas_gcl_oper_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_tas_gcl_oper_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnTasParamsEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_tas_cfg_t *>
         > P;

    static constexpr const char *table_description =
        "A table that contains the per-port manageable parameters for traffic scheduling.";

    static constexpr const char *index_description =
        "A list of objects that contains the manageable parameters for traffic scheduling for a port.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_tsn_tas_cfg_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_tas_cfg_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_tas_cfg_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_tsn_tas_cfg_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnTasOperStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_tas_oper_state_t *>
         > P;

    static constexpr const char *table_description =
        "A table that contains the per-port operational value of parameters for traffic scheduling.";

    static constexpr const char *index_description =
        "A list of objects that contains the operational value of parameters for a port.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_tsn_tas_oper_state_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_tas_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_tas_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TSN);
};


}  // namespace interfaces
}  // namespace tsn
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_TSN_TAS_SERIALIZER_HXX__ */
