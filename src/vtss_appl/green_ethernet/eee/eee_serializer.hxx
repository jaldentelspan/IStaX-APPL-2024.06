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
#ifndef __EEE_SERIALIZER_HXX__
#define __EEE_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/eee.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"

extern const vtss_enum_descriptor_t vtss_appl_eee_optimization_preference_txt[];
VTSS_XXXX_SERIALIZE_ENUM(
        vtss_appl_eee_optimization_preference_t, "EeePreference",
        vtss_appl_eee_optimization_preference_txt,
        "This enumeration defines the types of optimization preferences, "
        "either maximum power savings or low traffic latency.");

extern const vtss_enum_descriptor_t vtss_appl_eee_queue_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(
        vtss_appl_eee_queue_type_t, "EeeQueueType",
        vtss_appl_eee_queue_type_txt,
        "This enumeration defines the types of egress port queues.");

extern const vtss_enum_descriptor_t vtss_appl_eee_status_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eee_status_t, "EeeStatusType",
                         vtss_appl_eee_status_type_txt,
                         "This enumeration defines the feature status.");

/*****************************************************************************
  - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(EEE_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}
VTSS_SNMP_TAG_SERIALIZE(EEE_queue_index, vtss_appl_eee_port_queue_index_t, a,
                        s) {
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Index"),
               vtss::expose::snmp::RangeSpec<u32>(0, 7), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Egress port queue index."));
}

template <typename T>
void serialize(T &a, vtss_appl_eee_port_capabilities_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eee_port_capabilities_t"));
    int ix = 0;
    m.add_leaf(s.queue_count, vtss::tag::Name("MaxEgressQueues"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Maximum number of supported egress port queues."));

    m.add_leaf(vtss::AsBool(s.eee_capable), vtss::tag::Name("EEE"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicates whether interface supports EEE(IEEE "
                       "802.3az). true means EEE supported. false means "
                       "not supported."));
}
template <typename T>
void serialize(T &a, vtss_appl_eee_global_capabilities_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_eee_global_capabilities_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.optimization_preference_capable),
               vtss::tag::Name("OptimizationPreferences"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicates whether device supports optimization "
                       "preferences, true means supported. false means "
                       "not supported."));
}
template <typename T>
void serialize(T &a, vtss_appl_eee_global_conf_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_eee_global_conf_t"));
    int ix = 0;
    m.add_leaf(s.preference, vtss::tag::Name("OptimizationPreferences"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "EEE optimization preferences, either maximum power "
                       "saving or low traffic latency."));
}
template <typename T>
void serialize(T &a, vtss_appl_eee_port_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eee_port_conf_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.eee_enable), vtss::tag::Name("EnableEEE"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Enable EEE (IEEE 802.3az) feature at a interface. "
                       "true is to advertize EEE(IEEE 802.3az) capabilities "
                       "to partner device.  false is to disable it."));
}
template <typename T>
void serialize(T &a, vtss_appl_eee_port_queue_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eee_port_queue_t"));
    int ix = 0;
    m.add_leaf(s.queue_type, vtss::tag::Name("EgressQueueType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Egress port queue is urgent queue or normal queue."));
}
template <typename T>
void serialize(T &a, vtss_appl_eee_port_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eee_port_status_t"));
    int ix = 0;
    m.add_leaf(s.link_partner_eee, vtss::tag::Name("PartnerEEE"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicates whether link partner advertising EEE(IEEE "
                       "802.3az) capabilities."));

    m.add_leaf(s.rx_in_power_save_state, vtss::tag::Name("RxPowerSave"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicates whether interfcae rx path currently in power "
                       "save state."));
}
namespace vtss {
namespace appl {
namespace eee {
namespace interfaces {
struct eeeGlobalConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamVal<vtss_appl_eee_global_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_eee_global_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_eee_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_eee_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct eeeInterfaceConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eee_port_conf_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure EEE configurations for a specific "
            "interface.";

    static constexpr const char *index_description =
            "Each interface has a set of EEE configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, EEE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eee_port_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eee_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_eee_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct eeeInterfaceEgressQueueConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamKey<vtss_appl_eee_port_queue_index_t>,
            vtss::expose::ParamVal<vtss_appl_eee_port_queue_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure egress port queue type, whether "
            "urgent queue or normal queue.  We can configure more than one "
            "egress queues as urgent queues.  Queues configured as urgent, "
            "en-queued data will be transmitted with minimum latency.  Queue "
            "configured as normal, en-queued data will be transmitted with "
            "latency depending upon traffic utilization.";
    static constexpr const char *index_description =
            "Each interface has set of egress queues";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, EEE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eee_port_queue_index_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, EEE_queue_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_eee_port_queue_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eee_port_queue_type_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_eee_port_queue_iterator);
    VTSS_EXPOSE_SET_PTR(vtss_appl_eee_port_queue_type_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct eeeInterfaceStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eee_port_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Energy Efficient Ethernet interface status";

    static constexpr const char *index_description =
            "Each interface has a set of status parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, EEE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eee_port_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eee_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};
struct eeeGlobalsCapabilitiesEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamVal<vtss_appl_eee_global_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_eee_global_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_eee_global_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};
struct eeeInterfaceCapabilitiesEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eee_port_capabilities_t *>> P;

    static constexpr const char *table_description =
            "This is a table to interface capabilities";

    static constexpr const char *index_description =
            "Each interface has a set of capability parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, EEE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eee_port_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eee_port_capabilities_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};
}  // namespace interfaces
}  // namespace eee
}  // namespace appl
}  // namespace vtss
#endif
