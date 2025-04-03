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
#ifndef __VTSS_LACP_SERIALIZER_HXX__
#define __VTSS_LACP_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/lacp.h"
#include "vtss/appl/types.hxx"
#include "vtss_appl_formatting_tags.hxx"

mesa_rc aggregation_iface_itr_lag_idx(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex);
mesa_rc lacp_ifaceport_idx_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex);
mesa_rc lacp_ifacegroup_idx_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex);
mesa_rc lacp_port_stats_clr_set( vtss_ifindex_t ifindex, const BOOL *const clear);
mesa_rc lacp_port_stats_clr_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear);

VTSS_SNMP_TAG_SERIALIZE(lacp_ifindex_index, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("InterfaceNo"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(vtss_lacp_ctrl_bool_t, BOOL, a, s) {
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("PortStatisticsClear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Set to true to clear the statistics of a port.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_lacp_globals_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lacp_globals_t"));
    int ix = 0;

    m.add_leaf(
        s.system_prio,
        vtss::tag::Name("dot3adAggrSystemPriority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LACP_PRIORITY_MINIMUM, VTSS_APPL_LACP_PRIORITY_MAXIMUM),
        vtss::tag::Description("LACP system priority is a value.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_lacp_group_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lacp_group_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.revertive),
        vtss::tag::Name("Revertive"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Determines whether the LACP group failover is revertive or not. "
            "A revertive (TRUE) group will change back to the active port if it "
            "comes back up. A non-revertive (FALSE) group will remain on the "
            "standby port even of the active port comes back up."
            )
    );

    m.add_leaf(
        s.max_bundle,
        vtss::tag::Name("MaxBundle"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Max number of ports that can bundle up in an aggregation. Remaining "
            "ports will go into standby mode. The maximum number of ports in a "
            "bundle is 16 (or the number of physical ports on the device if that is lower).")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_lacp_port_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lacp_port_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.fast_xmit_mode),
        vtss::tag::Name("dot3adAggrTimeout"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The Timeout controls the period between BPDU "
            "transmissions. Fast(true) will transmit LACP packets each second, "
            "while Slow(0) will wait for 30 seconds before sending a LACP packet.")
    );

    m.add_leaf(
        s.port_prio,
        vtss::tag::Name("dot3adAggrPortPriority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LACP_PRIORITY_MINIMUM, VTSS_APPL_LACP_PRIORITY_MAXIMUM),
        vtss::tag::Description(
            "The Port Priority controls the priority of the "
            "port. If the LACP partner wants to form a larger group than is "
            "supported by this device then this parameter will control which "
            "ports will be active and which ports will be in a backup role. "
            "Lower number means greater priority.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_lacp_aggregator_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lacp_aggregator_status_t"));
    int ix = 0;

    m.add_leaf(
        s.aggr_id,
        vtss::tag::Name("dot3adAggrID"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The aggregation ID for a particular link aggregation group.")
    );

    m.add_leaf(
        s.partner_oper_system,
        vtss::tag::Name("dot3adAggrPartnerSystemID"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The system ID (MAC address) of the aggregation partner.")
    );

    m.add_leaf(
            s.partner_oper_key, vtss::tag::Name("dot3adAggrPartnerOperKey"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(
                "The Key that the partner has assigned to this aggregation ID.")
    );

    m.add_leaf(
        s.partner_oper_system_priority,
        vtss::tag::Name("dot3adAggrPartnerOperSystemPriority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "A 2-octet read-only value indicating the operational value\
            of priority associated with the Partner's System ID. The\
            value of this attribute may contain the manually configured value\
            carried in aAggPortPartnerAdminSystemPriority\
            if there is no protocol Partner.")
    );

    m.add_leaf(
        s.secs_since_last_change,
        vtss::tag::Name("dot3adAggrPartnerStateLastChanged"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The time in second since this aggregation changed")
    );

    vtss::PortListStackable &list = (vtss::PortListStackable &)s.port_list;
    m.add_leaf(
        list,
        vtss::tag::Name("dot3adAggrLocalPorts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Local port list")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_lacp_port_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lacp_port_status_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.lacp_mode),
        vtss::tag::Name("dot3adAggrActorAdminMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Shows the current Admin mode of port, if LACP enabled then returns\
            true else returns false.")
    );

    m.add_leaf(
        s.actor_oper_port_key,
        vtss::tag::Name("dot3adAggrActorAdminKey"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Shows the current administrative value of the\
            Key for the Aggregator. The administrative Key value may differ\
            from the operational Key value for the reasons discussed in 43.6.2.\
            This is a 16-bit, read-write value. The meaning of particular Key\
            values is of local significance")
    );

    m.add_leaf(
        s.partner_oper_port_number,
        vtss::tag::Name("dot3adAggrPartnerOperPortIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Shows the port index of the partner port connected to this port.")
    );

    m.add_leaf(
        s.partner_oper_port_priority,
        vtss::tag::Name("dot3adAggrPartnerOperPortPriority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Shows the port priority of the port partner port connected to this\
            port.")
    );

    m.add_leaf(
        s.actor_port_prio,
        vtss::tag::Name("dot3adActorAggrPortPriority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current administrative priority assigned to the port.")
    );

    m.add_leaf(
        vtss::AsOctetString(&(s.actor_lacp_state), 1),
        vtss::tag::Name("dot3adAggrPortOperState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current operational state of the port as a set of bits according "
            "to the defintion of the Actor_State octet (IEEE 802.1AX-2014, section 6.4.2.3).")
    );

    m.add_leaf(
        s.partner_oper_port_key,
        vtss::tag::Name("dot3adAggrPartnerKey"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The current operational value of the\
            key for the partner. The administrative key value may differ\
            from the operational key value for the reasons discussed in 43.6.2.\
            This is a 16-bit value. The meaning of particular key\
            values is of local significance.")
    );

    m.add_leaf(
        vtss::AsOctetString(&(s.partner_lacp_state), 1),
        vtss::tag::Name("dot3adAggrPartnerOperState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current operational state of the partner port as a set of bits according "
            "to the definition of the Partner_State octet (IEEE 802.1AX-2014, section 6.4.2.3).")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_lacp_port_stats_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lacp_port_stats_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsCounter(s.lacp_frame_rx),
        vtss::tag::Name("dot3adAggrRxFrames"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Shows received LACP frame count.")
    );

    m.add_leaf(
        vtss::AsCounter(s.lacp_frame_tx),
        vtss::tag::Name("dot3adAggrTxFrames"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Shows transmitted LACP frame count.")
    );

    m.add_leaf(
        vtss::AsCounter(s.illegal_frame_rx),
        vtss::tag::Name("dot3adAggrRxIllegalFrames"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Shows received illegal LACP frame count.")
    );

    m.add_leaf(
        vtss::AsCounter(s.unknown_frame_rx),
        vtss::tag::Name("dot3adAggrRxUnknownFrames"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Shows received unknown LACP frame count.")
    );
}


namespace vtss {
namespace appl {
namespace lacp {
namespace interfaces {
struct LacpGlobalParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_lacp_globals_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_lacp_globals_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lacp_globals_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_lacp_globals_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LACP);
};

struct LacpPortConfTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_lacp_port_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of the LACP port configurations.";

    static constexpr const char *index_description =
        "Each port has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, lacp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lacp_port_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lacp_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(lacp_ifaceport_idx_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_lacp_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LACP);
};

struct LacpGroupConfTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_lacp_group_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of the LACP group configurations. Entries in this table "
        "are also present in the AggrConfigGroupTable in the LACP-MIB but the "
        "LacpGroupConfTable will only contain group entries configured for LACP "
        "operation.";

    static constexpr const char *index_description =
        "Each group has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, lacp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lacp_group_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lacp_group_if_conf_get);
    VTSS_EXPOSE_ITR_PTR(lacp_ifacegroup_idx_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_lacp_group_if_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LACP);
};

struct LacpSystemStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_lacp_aggregator_status_t *>
    > P;

    static constexpr const char *table_description =
        "This table contains the LACP aggregation group system status. Each entry represents a "
        "single aggregation group and is indexed with the ifIndex of the aggregation group. "
        "The table is auto-populated by the system when valid parther information exist for the group.";

    static constexpr const char *index_description =
        "Each entry represents a set of status parameters for an aggregation group.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, lacp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lacp_aggregator_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_lacp_system_status_get);
    VTSS_EXPOSE_ITR_PTR(aggregation_iface_itr_lag_idx);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP);
};

struct LacpPortStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_lacp_port_status_t *>
    > P;

    static constexpr const char *table_description =
        "This table contains LACP port operational status parameters. Each table entry represents a "
        "single aggregation port and is indexed with the ifIndex of the port. "
        "The table is auto-populated by the system when valid LACP status exist for the port.";

    static constexpr const char *index_description =
        "Each entry represents a set of status parameters for an LACP port.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, lacp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lacp_port_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lacp_port_status_get);
    VTSS_EXPOSE_ITR_PTR(lacp_ifaceport_idx_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP);
};

struct LacpPortStatsTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_lacp_port_stats_t *>
    > P;

    static constexpr const char *table_description =
        "This table contains LACP port statistics counters. Each table entry represents a single "
        "aggregation port and is indexed with the ifIndex of the port. "
        "The table is auto-populated by the system when valid LACP statistics exist for the port.";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, lacp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lacp_port_stats_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lacp_port_stats_get);
    VTSS_EXPOSE_ITR_PTR(lacp_ifaceport_idx_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP);
};

struct LacpPortStatsClearTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear LACP port statistics";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, lacp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_lacp_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(lacp_port_stats_clr_dummy_get);
    VTSS_EXPOSE_ITR_PTR(lacp_ifaceport_idx_itr);
    VTSS_EXPOSE_SET_PTR(lacp_port_stats_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP);
};
}  // namespace interfaces
}  // namespace lacp
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_LACP_SERIALIZER_HXX__ */
