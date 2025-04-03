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
#ifndef __UDLD_SERIALIZER_HXX__
#define __UDLD_SERIALIZER_HXX__

#include "vtss/appl/udld.h"
#include "vtss_appl_serialize.hxx"

mesa_rc udld_if2ife(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife);
mesa_rc vtss_appl_udld_interface_config_set(vtss_ifindex_t ifindex,
                                            const vtss_appl_udld_port_conf_struct_t *conf);
mesa_rc vtss_appl_udld_interface_config_get(vtss_ifindex_t ifindex,
                                            vtss_appl_udld_port_conf_struct_t *conf);
mesa_rc vtss_appl_udld_interface_status_get(vtss_ifindex_t ifindex,
                                            vtss_appl_udld_port_info_t *status);
mesa_rc vtss_appl_udld_interface_neighbor_status_get(vtss_ifindex_t ifindex,
                                                     vtss_appl_udld_neighbor_info_t *status);

extern const vtss_enum_descriptor_t vtss_appl_udld_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_udld_mode_t,
                         "UdldMode",
                         vtss_appl_udld_mode_txt,
                         "This enumeration defines the available udld mode.");

extern const vtss_enum_descriptor_t vtss_udld_detection_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_udld_detection_state_t,
                         "UdldDetectionState",
                         vtss_udld_detection_state_txt,
                         "This enumeration defines the link detection state.");

VTSS_SNMP_TAG_SERIALIZE(UDLD_ifindex_index, vtss_ifindex_t , a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Logical interface number.")
               );
}

template<typename T>
void serialize(T &a, vtss_appl_udld_port_info_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_udld_port_info_t"));
    m.add_leaf(vtss::AsDisplayString(s.device_id, sizeof(s.device_id)),
            vtss::tag::Name("DeviceID"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description("Local device id."));
    m.add_leaf(vtss::AsDisplayString(s.device_name, sizeof(s.device_name)),
            vtss::tag::Name("DeviceName"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(2),
            vtss::tag::Description("Local device name."));
    m.add_leaf(s.detection_state, vtss::tag::Name("LinkState"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(3),
            vtss::tag::Description("Local device link detected state."));
}

template<typename T>
void serialize(T &a, vtss_appl_udld_neighbor_info_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_udld_neighbor_info_t"));
    m.add_leaf(vtss::AsDisplayString(s.device_id, sizeof(s.device_id)),
            vtss::tag::Name("NeighborDeviceID"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description("Neighbor device id."));
    m.add_leaf(vtss::AsDisplayString(s.port_id, sizeof(s.port_id)), 
            vtss::tag::Name("NeighborPortID"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(2),
            vtss::tag::Description("Neighbor port id."));
    m.add_leaf(vtss::AsDisplayString(s.device_name, sizeof(s.device_name)), 
            vtss::tag::Name("NeighborDeviceName"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(3),
            vtss::tag::Description("Neighbor device name."));
    m.add_leaf(s.detection_state, vtss::tag::Name("LinkDetectionState"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(4),
            vtss::tag::Description("Neighbor device link detected state."));
}

template<typename T>
void serialize(T &a, vtss_appl_udld_port_conf_struct_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_udld_port_conf_struct_t"));
    m.add_leaf(s.udld_mode, 
               vtss::tag::Name("UdldMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Port udld mode disable/normal/aggresive."));
    m.add_leaf(s.probe_msg_interval, 
               vtss::tag::Name("ProbeMsgInterval"),
               vtss::expose::snmp::RangeSpec<u32>(7, 90),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Port probe message interval(seconds). Valid range: 7 to 90 seconds."));
}

namespace vtss {
namespace appl {
namespace udld {
namespace interfaces {

struct UdldPortParamsTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_udld_port_conf_struct_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of udld interface conf parameters";

    static constexpr const char *index_description =
        "Each physical interface has a set of configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, UDLD_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_udld_port_conf_struct_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_udld_interface_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_ifindex_getnext_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_udld_interface_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_UDLD);
};

struct UdldPortInterfaceStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_udld_port_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of udld interface local device information";

    static constexpr const char *index_description =
        "Each udld enabled interface has a local device information";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, UDLD_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_udld_port_info_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_udld_interface_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_ifindex_getnext_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_UDLD);
};

struct UdldPortNeighborStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_udld_neighbor_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of udld interface neighbor cache information";

    static constexpr const char *index_description =
        "Each udld enabled interface has a neighbor cache information";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, UDLD_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_udld_neighbor_info_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_udld_interface_neighbor_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_ifindex_getnext_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_UDLD);
};

}  // namespace interfaces
}  // namespace udld
}  // namespace appl
}  // namespace vtss
#endif
