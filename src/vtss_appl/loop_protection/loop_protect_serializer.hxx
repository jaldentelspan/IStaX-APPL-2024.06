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
#ifndef __LOOP_PROTECT_SERIALIZER_HXX__
#define __LOOP_PROTECT_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/interface.h"
#include "vtss/appl/loop_protect.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"

extern vtss_enum_descriptor_t vtss_loop_protection_action_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_loop_protect_action_t,
                         "LoopProtectionAction",
                         vtss_loop_protection_action_txt,
                         "This enumeration defines the available actions for when a loop on an interface is detected.");

VTSS_SNMP_TAG_SERIALIZE(LPROT_ifindex_index, vtss_ifindex_t , a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Logical interface number."));
}

mesa_rc lprot_if2ife(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife);
mesa_rc vtss_appl_lprot_interface_config_set(vtss_ifindex_t ifindex, const vtss_appl_loop_protect_port_conf_t *conf);
mesa_rc vtss_appl_lprot_interface_config_get(vtss_ifindex_t ifindex, vtss_appl_loop_protect_port_conf_t *conf);
mesa_rc vtss_appl_lprot_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_loop_protect_port_info_t *status);

template<typename T>
void serialize(T &a, vtss_appl_loop_protect_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_loop_protect_conf_t"));
    m.add_leaf(vtss::AsBool(s.enabled), vtss::tag::Name("Enabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Global enabled for loop protection on any port."));
    m.add_leaf(s.transmission_time, vtss::tag::Name("TransmitInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Port transmission interval (seconds). Valid range: 1-10 seconds."));
    m.add_leaf(s.shutdown_time, vtss::tag::Name("ShutdownPeriod"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Port shutdown period (seconds). Valid range: 0 to 604800 seconds."));
}

template<typename T>
void serialize(T &a, vtss_appl_loop_protect_port_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_loop_protect_port_conf_t"));
    m.add_leaf(vtss::AsBool(s.enabled), vtss::tag::Name("Enabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Enabled loop protection on port"));
    m.add_leaf(s.action, vtss::tag::Name("Action"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Action if loop detected"));
    m.add_leaf(vtss::AsBool(s.transmit), vtss::tag::Name("Transmit"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Actively generate PDUs"));
}

template<typename T>
void serialize(T &a, vtss_appl_loop_protect_port_info_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_loop_protect_port_info_t"));
    m.add_leaf(vtss::AsBool(s.disabled), vtss::tag::Name("Disabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Whether a port is currently disabled"));
    m.add_leaf(vtss::AsBool(s.loop_detect), vtss::tag::Name("LoopDetected"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Whether a port has a loop detected"));
    m.add_leaf(s.loops, vtss::tag::Name("LoopCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Number of times a loop has been detected on a port"));
    m.add_leaf(s.last_loop, vtss::tag::Name("LastLoop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Time of last loop condition"));
}

namespace vtss {
namespace appl {
namespace loop_protect {
namespace interfaces {
struct LprotGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_loop_protect_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_loop_protect_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_loop_protect_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_loop_protect_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LOOP_PROTECT);
};

struct LprotPortParamsTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_loop_protect_port_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of loop protection interface parameters";

    static constexpr const char *index_description =
        "Each physical interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LPROT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_loop_protect_port_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lprot_interface_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_lprot_interface_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LOOP_PROTECT);
};

struct LprotPortStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_loop_protect_port_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of loop protection interface status";

    static constexpr const char *index_description =
        "Each physical interface has a set of status objects";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LPROT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_loop_protect_port_info_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lprot_interface_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LOOP_PROTECT);
};
}  // namespace interfaces
}  // namespace loop_protect
}  // namespace appl
}  // namespace vtss

#endif
