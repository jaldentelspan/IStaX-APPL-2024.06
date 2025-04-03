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

#ifndef __VTSS_AGGR_SERIALIZER_HXX__
#define __VTSS_AGGR_SERIALIZER_HXX__

#include "vtss_appl_formatting_tags.hxx"
#include "vtss_appl_serialize.hxx"
#include <vtss/appl/aggr.h>
#include <vtss/appl/types.hxx>
#include "port_serializer.hxx" /* For serializing mesa_port_speed_t */

extern vtss::expose::TableStatus<
  vtss::expose::ParamKey<vtss_ifindex_t>,
  vtss::expose::ParamVal<vtss_appl_aggr_group_status_t *>> aggr_status_update;

mesa_rc aggregation_iface_itr_lag_idx( const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex);

extern vtss_enum_descriptor_t vtss_appl_aggr_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aggr_mode_t,
                         "AggregationMode",
                         vtss_appl_aggr_mode_txt,
                         "The aggregation mode.");

VTSS_SNMP_TAG_SERIALIZE(aggr_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("AggrIndexNo"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Link Aggregation Group Identifier.")
    );
}

template<typename T>
void serialize(T &a, mesa_aggr_mode_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("mesa_aggr_mode_t"));

    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.smac_enable),
        vtss::tag::Name("SmacAddr"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set to true to enable the use of the Source MAC\
            address, or false to disable. By default, Source MAC Address is\
            enabled.")
    );

    m.add_leaf(
        vtss::AsBool(s.dmac_enable),
        vtss::tag::Name("DmacAddr"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set to true to enable the use of the Destination\
            MAC address, or false to disable. By default, Destination MAC Address\
            is disabled.")
    );

    m.add_leaf(
        vtss::AsBool(s.sip_dip_enable),
        vtss::tag::Name("SourceAndDestinationIpAddr"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set to true to enable the use of the IP address,\
            or false to disable. By default, Destination MAC Address is enabled.")
    );

    m.add_leaf(
        vtss::AsBool(s.sport_dport_enable),
        vtss::tag::Name("TcpOrUdpSportAndDportNo"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set to true to enable the use of the TCP/UDP\
            Port Number, or false to disable. "
            "By default, TCP/UDP Port Number is enabled.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_aggr_group_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_aggr_group_conf_t"));
    vtss::PortListStackable &list = (vtss::PortListStackable &)s.cfg_ports.member;
    int ix = 0;

    m.add_leaf(
        list,
        vtss::tag::Name("PortMembers"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The bitmap type containing the port members'\
            list for this aggregation group.")
    );

    m.add_leaf(
        s.mode,
        vtss::tag::Name("AggrMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Aggregation group mode.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_aggr_group_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_aggr_group_status_t"));
    int ix = 0;

    m.add_leaf(
        vtss::PortListStackable(s.cfg_ports.member),
        vtss::tag::Name("ConfiguredPorts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Configured member ports of the aggregation Group.")
    );

    m.add_leaf(
        vtss::PortListStackable(s.aggr_ports.member),
        vtss::tag::Name("AggregatedPorts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Aggregated member ports of the aggregation Group.")
    );

    m.add_leaf(
        s.speed,
        vtss::tag::Name("Speed"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Speed of the Aggregation Group.")
    );

    m.add_leaf(
        s.mode,
        vtss::tag::Name("AggrMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Aggregation group mode.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.type, sizeof(s.type)),
        vtss::tag::Name("Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of the Aggregation Group.")
    );
}

namespace vtss {
namespace appl {
namespace aggr {
namespace interfaces {
struct AggrModeParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<mesa_aggr_mode_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_aggr_mode_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aggregation_mode_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_aggregation_mode_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_AGGR);
};

struct AggrGroupConfTable {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_ifindex_t>,
                                    vtss::expose::ParamVal<vtss_appl_aggr_group_conf_t *>> P;

     static constexpr const char *table_description =
        "The table is static Link Aggregation Group configuration table. The index is Aggregration Group Identifier.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, aggr_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_aggr_group_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aggregation_group_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_aggregation_port_members_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_aggregation_group_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_AGGR);
};

struct AggrGroupStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_aggr_group_status_t *>
    > P;

    static constexpr const char *table_description =
        "The table is Aggregation Group status table. The index is Aggregration Group Identifier.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, aggr_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_aggr_group_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aggregation_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_aggregation_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_AGGR);
};
}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_AGGR_SERIALIZER_HXX__ */
