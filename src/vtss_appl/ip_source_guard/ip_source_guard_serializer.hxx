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
#ifndef __VTSS_IP_SOURCE_GUARD_SERIALIZER_HXX__
#define __VTSS_IP_SOURCE_GUARD_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/ip_source_guard.h"
#include "vtss_appl_formatting_tags.hxx"


/*****************************************************************************
    Capabilities
*****************************************************************************/
struct ipSourceGuardStaticIpMaskSupported {
    static constexpr const char *json_ref = "vtss_appl_ip_source_guard_capabilities_t";
    static constexpr const char *name = "StaticIpMask";
    static constexpr const char *desc = "If FALSE, the IP mask of static binding table is only allowed to be configured as 255.255.255.255.";
    static bool get();
};

struct ipSourceGuardStaticMacAddressSupported  {
    static constexpr const char *json_ref = "vtss_appl_ip_source_guard_capabilities_t";
    static constexpr const char *name = "StaticMacAddress";
    static constexpr const char *desc = "If TRUE, the MAC address of static binding table is configurable.";
    static bool get();
};


/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(ip_source_guard_ifindex_index, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}


/*****************************************************************************
    Index serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_static_index_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_static_index_t"));
    int                 ix = 0;
    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifindex),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Logical interface number of the physical port.")
    );

    m.add_leaf(
        vtss::AsVlan(s.vlan_id),
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN ID.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.ip_addr),
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Assigned IP address.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.ip_mask),
        vtss::tag::Name("IpMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Assigned network mask.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_dynamic_index_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_dynamic_index_t"));
    int                 ix = 0;
    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifindex),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Logical interface number of the physical port.")
    );

    m.add_leaf(
        vtss::AsVlan(s.vlan_id),
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN ID.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.ip_addr),
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Learned IP address.")
    );

}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_global_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_global_config_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global mode of IP source guard. TRUE is to "
            "enable IP source guard and FALSE is to disable it.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_port_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_port_config_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Port mode of IP source guard. TURE is to "
            "enable IP source guard on the port and FALSE is to disable "
            "it on the port.")
    );

    m.add_leaf(
        s.dynamic_entry_count,
        vtss::tag::Name("DynamicEntryCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The dynamic entry count is the max number "
            "of dynamic entries allowed on the port.")
    );
}


template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_static_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_static_config_t"));
    int ix = 0;

    m.add_leaf(
        s.mac_addr,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<ipSourceGuardStaticMacAddressSupported>(),
        vtss::tag::Description("Assigned MAC Address.")
    );
}


template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_dynamic_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_dynamic_status_t"));
    int ix = 0;

    m.add_leaf(
        s.mac_addr,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Learned MAC Address.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ip_source_guard_control_translate_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_control_translate_t"));
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
namespace ip_source_guard {
namespace interfaces {

struct IpSourceGuardCapabilitiesLeaf {
        typedef vtss::expose::ParamList<
            vtss::expose::ParamVal<vtss_appl_ip_source_guard_capabilities_t *>
        > P;

        VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_source_guard_capabilities_t &s) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));

        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ip_source_guard_capabilities_t"));

        int ix = 0;

        m.template capability<ipSourceGuardStaticIpMaskSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<ipSourceGuardStaticMacAddressSupported>(vtss::expose::snmp::OidElementValue(ix++));
        }


        VTSS_EXPOSE_GET_PTR(vtss_appl_ip_source_guard_capabilities_get);
        VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};



struct IpSourceGuardParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_ip_source_guard_global_config_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_source_guard_global_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_source_guard_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_source_guard_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct IpSourceGuardInterfaceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_ip_source_guard_port_config_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of IP source guard port configuration parameters.";

    static constexpr const char *index_description =
        "Each port has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ip_source_guard_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_source_guard_port_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_source_guard_port_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ip_source_guard_port_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_source_guard_port_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct IpSourceGuardStaticConfigEntry {
    typedef vtss::expose::ParamList<
          vtss::expose::ParamKey<vtss_appl_ip_source_guard_static_index_t *>,
          vtss::expose::ParamVal<vtss_appl_ip_source_guard_static_config_t *>

    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing the static binding table of IP source guard.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_source_guard_static_index_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_source_guard_static_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(5));
        serialize(h, i);
    }


    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_source_guard_static_config_get);
    
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ip_source_guard_static_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_source_guard_static_config_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ip_source_guard_static_config_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ip_source_guard_static_config_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_ip_source_guard_static_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct IpSourceGuardDynamicStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_ip_source_guard_dynamic_index_t *>,
        vtss::expose::ParamVal<vtss_appl_ip_source_guard_dynamic_status_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table provided dynamic binding table of IP source guard.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_source_guard_dynamic_index_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ip_source_guard_dynamic_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_source_guard_dynamic_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ip_source_guard_dynamic_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct IpSourceGuardControlLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_ip_source_guard_control_translate_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ip_source_guard_control_translate_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ip_source_guard_control_translate_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ip_source_guard_control_translate_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
}  // namespace interfaces
}  // namespace ip_source_guard
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_IP_SOURCE_GUARD_SERIALIZER_HXX__ */
