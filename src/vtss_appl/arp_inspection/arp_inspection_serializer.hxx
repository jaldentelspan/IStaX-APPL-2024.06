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
#ifndef __VTSS_ARP_INSPECTION_SERIALIZER_HXX__
#define __VTSS_ARP_INSPECTION_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/arp_inspection.h"

/*****************************************************************************
 - JSON notification serializer
*****************************************************************************/
extern vtss::expose::StructStatus <
    vtss::expose::ParamVal<vtss_appl_arp_inspection_status_event_t *>
    > arp_inspection_status_event_update;

/*****************************************************************************
    Data type serializer
*****************************************************************************/

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_arp_inspection_ctrl_bool_t, BOOL, a, s) {
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("TranslateDynamicToStatic"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("To trigger the control action (only) when TRUE.")
    );
}

mesa_rc vtss_appl_arp_inspection_control_dummy_get(BOOL *const act_flag);
/*****************************************************************************
    Enumerator serializer
*****************************************************************************/
extern vtss_enum_descriptor_t arp_inspection_logType_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_arp_inspection_log_t,
                         "ArpInspectionLogType",
                         arp_inspection_logType_txt,
                         "This enumeration indicates the ARP entry log type.");

extern vtss_enum_descriptor_t arp_inspection_regStatus_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_arp_inspection_status_t,
                         "ArpInspectionRegisterStatus",
                         arp_inspection_regStatus_txt,
                         "This enumeration indicates the ARP entry registration type.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(arp_inspection_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(arp_inspection_vid_index, mesa_vid_t, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(1, 4095),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The VID of the VLAN.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(arp_inspection_mac_index, mesa_mac_t, a, s) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned MAC address.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(arp_inspection_ip_index, mesa_ipv4_t, a, s) {
    a.add_leaf(
        vtss::AsIpv4(s.inner),
        vtss::tag::Name("IpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Assigned IPv4 address.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_arp_inspection_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_arp_inspection_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the ARP Inspection global functionality.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_arp_inspection_port_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_arp_inspection_port_config_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the ARP Inspection per-port functionality. "
            "Only when both Global Mode and Port Mode on a given port are enabled, "
            "ARP Inspection is enabled on this given port.")
    );

    m.add_leaf(
        vtss::AsBool(s.check_vlan),
        vtss::tag::Name("CheckVlan"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable the ARP Inspection VLAN checking will log the inspected "
            "entries by referring to arpInspectionVlanConfigTable setting. Disable the ARP "
            "Inspection VLAN checking will log the inspected entries by referring to "
            "arpInspectionPortConfigTable setting.")
    );

    m.add_leaf(
        s.log_type,
        vtss::tag::Name("LogType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The capability to log the inspected entries per port basis. "
            "none(0) will log nothing. deny(1) will log the denied entries. permit(2) will "
            "log the permitted entries. all(3) will log all kinds of inspected entries.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_arp_inspection_vlan_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_arp_inspection_vlan_config_t"));
    int ix = 0;

    m.add_leaf(
        s.log_type,
        vtss::tag::Name("LogType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The capability to log the inspected entries per VLAN basis. "
            "none(0) will log nothing. deny(1) will log the denied entries. permit(2) will "
            "log the permitted entries. all(3) will log all kinds of inspected entries.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_arp_inspection_entry_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_arp_inspection_entry_t"));
    int ix = 0;

    m.add_leaf(
        s.reg_status,
        vtss::tag::Name("Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Either static(0) or dynamic(1) for the specific ARP entry.")
    );
}

/* JSON notification */
template<typename T>
void serialize(T &a, vtss_appl_arp_inspection_status_event_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_arp_inspection_status_event_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.crossed_maximum_entries),
        vtss::tag::Name("CrossedMaximumEntries"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This is an ARP inspection event status. When the status value is true, "
            "it means the ARP inspection status is reached the maximum entries.")
    );
}

namespace vtss {
namespace appl {
namespace arp_inspection {
namespace interfaces {
struct ArpInspectionParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_arp_inspection_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_arp_inspection_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_arp_inspection_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ArpInspectionPortConfigTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_arp_inspection_port_config_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table for managing ARP Inspection per port basis";

    static constexpr const char *index_description =
        "Each port has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, arp_inspection_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_arp_inspection_port_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_port_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_arp_inspection_port_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_arp_inspection_port_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ArpInspectionVlanConfigTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamVal<vtss_appl_arp_inspection_vlan_config_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing ARP Inspection per VLAN basis";

    static constexpr const char *index_description =
        "Each VLAN has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, arp_inspection_vid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_arp_inspection_vlan_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_vlan_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_arp_inspection_vlan_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_arp_inspection_vlan_config_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_arp_inspection_vlan_config_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_arp_inspection_vlan_config_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_arp_inspection_vlan_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ArpInspectionStaticConfigTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamKey<mesa_mac_t>,
        vtss::expose::ParamKey<mesa_ipv4_t>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing static ARP Inspection configuration";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, arp_inspection_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, arp_inspection_vid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_mac_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, arp_inspection_mac_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, arp_inspection_ip_index(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_static_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_arp_inspection_static_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_arp_inspection_static_entry_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_arp_inspection_static_entry_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_arp_inspection_static_entry_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_arp_inspection_static_entry_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ArpInspectionDynamicStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<mesa_vid_t>,
        vtss::expose::ParamKey<mesa_mac_t>,
        vtss::expose::ParamKey<mesa_ipv4_t>,
        vtss::expose::ParamVal<vtss_appl_arp_inspection_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table for displaying all ARP Inspection entries";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, arp_inspection_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, arp_inspection_vid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_mac_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, arp_inspection_mac_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, arp_inspection_ip_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_arp_inspection_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(5));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_dynamic_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_arp_inspection_dynamic_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ArpInspectionActionLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<BOOL *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_arp_inspection_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_control_dummy_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_arp_inspection_control_translate_dynamic_to_static_act);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

/* JSON notification */
struct ArpInspectionStatusEventEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_arp_inspection_status_event_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_arp_inspection_status_event_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_arp_inspection_status_event_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

}  // namespace interfaces
}  // namespace arp_inspection
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_ARP_INSPECTION_SERIALIZER_HXX__ */
