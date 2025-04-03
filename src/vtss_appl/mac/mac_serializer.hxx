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
#ifndef __VTSS_MAC_SERIALIZER_HXX__
#define __VTSS_MAC_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/mac.h"
#include "vtss/appl/interface.h"
#include "vtss/appl/types.hxx"

mesa_rc vtss_appl_mac_appl_table_flush_dummy(vtss_appl_mac_flush_t *const flush);
mesa_rc vtss_appl_mac_table_itr_vid(const mesa_vid_t *prev_vlan, mesa_vid_t *next_vlan);

extern vtss_enum_descriptor_t mac_learning_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_mac_port_learn_mode_t, 
                         "MACPortLearnMode", 
                         mac_learning_txt,
                         "The learning mode of the port.");

struct MAC_ifindex_index {
    MAC_ifindex_index(vtss_ifindex_t &x) : inner(x) { }
    vtss_ifindex_t &inner;
};


template<typename T>
void serialize(T &a, vtss_appl_mac_age_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_age_conf_t"));
    m.add_leaf(s.mac_age_time, vtss::tag::Name("AgeTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Mac address aging time in the FDB."));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_stack_addr_entry_t &s) {
  typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_stack_addr_entry_t"));
  vtss::PortListStackable &list = (vtss::PortListStackable &)s.destination;
  m.add_leaf(list, 
             vtss::tag::Name("PortList"),
             vtss::expose::snmp::Status::Current,
             vtss::expose::snmp::OidElementValue(1),
             vtss::tag::Description("List of destination ports for which frames " 
                                      "with this DMAC is forwarded to."));
  m.add_leaf(s.dynamic, 
             vtss::tag::Name("Dynamic"),
             vtss::expose::snmp::Status::Current,
             vtss::expose::snmp::OidElementValue(2),
             vtss::tag::Description("The entry is dynamically learned (True) or statically added (False)"));

  m.add_leaf(s.copy_to_cpu, 
             vtss::tag::Name("CopyToCpu"),
             vtss::expose::snmp::Status::Current,
             vtss::expose::snmp::OidElementValue(3),
             vtss::tag::Description("Copy this frame to the CPU (True) or not (False)"));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_stack_addr_entry_conf_t &s) {
  typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_stack_addr_entry_conf_t"));
  vtss::PortListStackable &list = (vtss::PortListStackable &)s.destination;
  m.add_leaf(list, 
             vtss::tag::Name("PortList"),
             vtss::expose::snmp::Status::Current,
             vtss::expose::snmp::OidElementValue(1),
             vtss::tag::Description("List of destination ports for which frames " 
                                      "with this DMAC is forwarded to."));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_flush_t &s) {
  typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_flush_t"));
  m.add_leaf(vtss::AsBool(s.flush_all), vtss::tag::Name("FlushAll"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Flush all dynamic learned Mac addresses. "
                                      "Set the value to 'true' to perform the action. "
                                      "Read will always return 'false'."));
}

VTSS_SNMP_TAG_SERIALIZE(mac_address_static_table_vid, mesa_vid_t, a, s ) {
    a.add_leaf(
        vtss::AsVlan(s.inner),  
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("Vlan id used for indexing.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mac_address_static_table_mac, mesa_mac_t, a, s ) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("The destination MAC address which this entry applies.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(mac_address_table_ifindex, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("Interface index.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_mac_learn_mode_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_learn_mode_t"));
    m.add_leaf(s.learn_mode, vtss::tag::Name("LearnMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The learn mode of the port. "
                                      "Auto(0) means auto learning. "
                                      "Disable(1) means that learning is disabled. "
                                      "Secure(2) means that learning frames are discarded."));

    m.add_leaf(vtss::AsBool(s.chg_allowed), vtss::tag::Name("ChangeAllowed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("If internal modules have changed the learned mode "
                                      "then the user is not allowed to change it from this interface. "
                                      "This entry tells you if the LearnMode "
                                      "can be changed (true) or not (false). "
                                      "This is a read only entry - write is ignored."));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_port_stats_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_port_stats_t"));
    m.add_leaf(s.dynamic, vtss::tag::Name("Dynamic"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Total number of dynamic learned addresses on the port"));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_vid_learn_mode_t &s) {
  typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_vid_learn_mode_t"));
  m.add_leaf(vtss::AsBool(s.learning), vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Learn mode of the VLAN, True = Enabled, False = Disabled"));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_capabilities_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_capabilities_t"));
    m.add_leaf(s.mac_addr_non_volatile_max, vtss::tag::Name("NonVolatileMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Max number of static non-volatile MAC addresses that can be stored in the system."));
}

template<typename T>
void serialize(T &a, vtss_appl_mac_table_stats_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mac_table_stats_t"));
    m.add_leaf(s.dynamic_total, vtss::tag::Name("TotalDynamic"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Total dynamic learned addresses in the FDB"));

    m.add_leaf(s.static_total, vtss::tag::Name("TotalStatic"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Total static addresses in the FDB"));
}

namespace vtss {
namespace appl {
namespace mac {
namespace interfaces {

struct MacCapLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_mac_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mac_capabilities_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacAgeLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_mac_age_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mac_age_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_age_time_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mac_age_time_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacAddressConfigTableImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamKey<mesa_mac_t>,
        vtss::expose::ParamVal<vtss_appl_mac_stack_addr_entry_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This table represents static mac addresses added through the mgmt interface. ";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, mac_address_static_table_vid(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_mac_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mac_address_static_table_mac(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_mac_stack_addr_entry_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_table_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mac_table_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mac_table_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_mac_table_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_mac_table_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_mac_table_conf_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacAddressLearnTableImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_mac_learn_mode_t *>
    > P;

    static constexpr const char *table_description =
        "This table represents the learning mode of each port";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, mac_address_table_ifindex(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mac_learn_mode_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_learn_mode_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mac_learn_mode_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacAddressVlanLearnTableImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamVal<vtss_appl_mac_vid_learn_mode_t *>
    > P;

    static constexpr const char *table_description =
        "This table represents the learning mode of each vlan 1-4095";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, mac_address_static_table_vid(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mac_vid_learn_mode_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_vlan_learn_mode_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mac_table_itr_vid);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mac_vlan_learn_mode_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacFdbTableImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamKey<mesa_mac_t>,
        vtss::expose::ParamVal<vtss_appl_mac_stack_addr_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This table represents all mac addresses in the FDB";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, mac_address_static_table_vid(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_mac_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mac_address_static_table_mac(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_mac_stack_addr_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_table_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mac_table_all_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacFdbTableStaticImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamKey<mesa_mac_t>,
        vtss::expose::ParamVal<vtss_appl_mac_stack_addr_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This table represents all static mac addresses in the FDB";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, mac_address_static_table_vid(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_mac_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, mac_address_static_table_mac(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_mac_stack_addr_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_table_static_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mac_table_static_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacAddressStatsTableImpl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_mac_port_stats_t *>
    > P;

    static constexpr const char *table_description =
        "This table represent the statistics of the Port interfaces";

    static constexpr const char *index_description =
        "Each port has a number of learned addresses";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, mac_address_table_ifindex(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mac_port_stats_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_port_stats_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacStatisLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_mac_table_stats_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mac_table_stats_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_table_stats_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC);
};

struct MacFlushLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_mac_flush_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mac_flush_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mac_appl_table_flush_dummy);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mac_table_flush);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC);
};

}  // namespace interfaces
}  // namespace mac
}  // namespace appl
}  // namespace vtss

#endif
