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

#ifndef __IPV6_SOURCE_GUARD_SERIALIZER_HXX__
#define __IPV6_SOURCE_GUARD_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/ipv6_source_guard.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"
#include "ipv6_source_guard.h"

/*****************************************************************************
  - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(IPV6_SOURCE_GUARD_port_interface, vtss_ifindex_t, a, s) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

// IPv6 Source Guard Global config serializer.
template<typename T>
void serialize(T &a, vtss_appl_ipv6_source_guard_global_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipv6_source_guard_global_config_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.enabled),
        vtss::tag::Name("Enabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global config of IPv6 source guard. TRUE is to "
            "enable IP source guard and FALSE is to disable it.")
    );
}

// IPv6 Source Guard Port config serializer.
template <typename T>
void serialize(T &a, vtss_appl_ipv6_source_guard_port_config_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_ipv6_source_guard_port_config_t"));
    int ix = 0;
    m.add_leaf(
        vtss::AsBool(s.enabled),
        vtss::tag::Name("enabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("True means that ipv6 source guard is enabled on port.")
        );

    m.add_leaf(
        s.max_dynamic_entries, 
        vtss::tag::Name("MaxDynamicEntries"),
        vtss::expose::snmp::Status::Current, 
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Max number of allowed dynamic entries per port.")
        );
}

// IPv6 Source Guard Binding Table Entry serializer.
template<typename T>
void serialize(T &a, vtss_appl_ipv6_source_guard_entry_index_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipv6_source_guard_entry_index_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifindex),
        vtss::tag::Name("Ifindex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Logical interface number of the physical port entry is bound to.")
    );

    m.add_leaf(
        vtss::AsVlan(s.vlan_id),
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN ID. 0 means no vlan id is needed.")
    );

    m.add_leaf(s.ipv6_addr,
        vtss::tag::Name("ipv6Address"),
        vtss::expose::snmp::Status::Current, 
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Entry's IPv6 address.")
        );
}

// IPv6 Source Guard Binding Table Entry Data serializer.
template<typename T>
void serialize(T &a, vtss_appl_ipv6_source_guard_entry_data_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_static_config_t"));
    int ix = 0;

    m.add_leaf(
        s.mac_addr,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Assigned MAC Address.")
    );
}

// IPv6 Source Guard Translate Action serializer.
template<typename T>
void serialize(T &a, vtss_appl_ipv6_source_guard_control_translate_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipv6_source_guard_control_translate_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.translate),
        vtss::tag::Name("TranslateDynamicToStatic"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Translate all the current "
            "dynamic entries to static ones. Set it as TRUE to do the "
            "action.")
    );
}

namespace vtss {
namespace appl {
namespace ipv6_source_guard {
namespace interfaces {

struct IPv6SourceGuardGlobalConfigLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_global_config_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipv6_source_guard_global_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPV6_SOURCE_GUARD);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ipv6_source_guard_global_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ipv6_source_guard_global_config_set);
};

struct IPv6SourceGuardPortConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_port_config_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure IPv6 Source Guard for a specific port.";

    static constexpr const char *index_description =
            "Each port interface can be configured for ipv6 source guard";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, IPV6_SOURCE_GUARD_port_interface(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipv6_source_guard_port_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPV6_SOURCE_GUARD);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ipv6_source_guard_port_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ipv6_source_guard_port_config_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipv6_source_guard_port_conf_itr);
};

struct Ipv6SourceGuardStaticEntry {
    typedef vtss::expose::ParamList<
          vtss::expose::ParamKey<vtss_appl_ipv6_source_guard_entry_index_t *>,
          vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_entry_data_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "This is a table for managing the static binding entries of IPv6 source guard.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipv6_source_guard_entry_index_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipv6_source_guard_entry_data_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPV6_SOURCE_GUARD);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ipv6_source_guard_static_entry_data_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipv6_source_guard_static_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ipv6_source_guard_static_entry_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ipv6_source_guard_static_entry_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ipv6_source_guard_static_entry_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_ipv6_source_guard_static_entry_default);
};

struct Ipv6SourceGuardDynamicEntry {
    typedef vtss::expose::ParamList<
          vtss::expose::ParamKey<vtss_appl_ipv6_source_guard_entry_index_t *>,
          vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_entry_data_t *>> P;

    static constexpr const char *table_description =
        "This is a table for managing the static binding entries of IPv6 source guard.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipv6_source_guard_entry_index_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipv6_source_guard_entry_data_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPV6_SOURCE_GUARD);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ipv6_source_guard_dynamic_entry_data_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipv6_source_guard_dynamic_entry_itr);

};

struct Ipv6SourceGuardControlTranslateLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_control_translate_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipv6_source_guard_control_translate_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ipv6_source_guard_control_translate_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ipv6_source_guard_control_translate_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPV6_SOURCE_GUARD);
};

}  // namespace interfaces
}  // namespace ipv6_source_guard
}  // namespace appl
}  // namespace vtss

#endif //__IPV6_SOURCE_GUARD_SERIALIZER_HXX__

