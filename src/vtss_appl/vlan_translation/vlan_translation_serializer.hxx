/*
 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VLAN_TRANSLATION_SERIALIZER_HXX_
#define _VLAN_TRANSLATION_SERIALIZER_HXX_

#include "vtss/appl/vlan_translation.h"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/module_id.h"
#include "vtss/basics/expose.hxx"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss/appl/types.hxx"

VTSS_XXXX_SERIALIZE_ENUM(mesa_vlan_trans_dir_t,
                         "VlanTranslationDir",
                         mesa_vlan_trans_dir_txt,
                         "The VLAN Translation Direction.");

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_vlan_trans_tvid_key, mesa_vid_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("TVlanId"),
               vtss::expose::snmp::RangeSpec<u32>(1, 4095),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Translated VLAN ID of the VLAN translation mapping."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_vlan_trans_findex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Interface index number."));
}

template <typename T>
void serialize(T &a, vtss_appl_vlan_translation_capabilities_t &p) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_vlan_translation_capabilities_t"));

    m.add_leaf(p.max_number_of_translations,
               vtss::tag::Name("maxNumberOfTranslations"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Maximum number of VLAN translation mappings "
                                      "the user can store in the VLAN Translation mapping table."));
}

template <typename T>
void serialize(T &a, vtss_appl_vlan_translation_if_conf_value_t &p) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_vlan_translation_if_conf_value_t"));

    m.add_leaf(vtss::AsInt(p.gid),
               vtss::tag::Name("GroupId"),
               vtss::expose::snmp::RangeSpec<u16>(1, 65535),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Group ID of the interface configuration."));
}

template <typename T>
void serialize(T &a, vtss_appl_vlan_translation_group_mapping_key_t &p) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_vlan_translation_group_mapping_key_t"));

    m.add_leaf(vtss::AsInt(p.gid),
               vtss::tag::Name("GroupId"),
               vtss::expose::snmp::RangeSpec<u16>(1, 65535),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Group ID of the VLAN translation mapping key."));

    m.add_leaf(p.dir,
               vtss::tag::Name("Direction"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The VLAN Translation Direction."));
    
    m.add_leaf(vtss::AsInt(p.vid),
               vtss::tag::Name("VlanId"),
               vtss::expose::snmp::RangeSpec<u32>(1, 4095),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Vlan ID of the VLAN translation mapping key."));
}

namespace vtss {
namespace appl {
namespace vlan_translation {
namespace interfaces {

struct GlobalCapabilities {
    typedef expose::ParamList<
            expose::ParamVal<vtss_appl_vlan_translation_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_vlan_translation_capabilities_t &i) {
        h.argument_properties(tag::Name("capabilities"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vlan_translation_global_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VLAN_TRANSLATION);
};

struct VTEntry {
    typedef expose::ParamList<
            expose::ParamKey<vtss_appl_vlan_translation_group_mapping_key_t>,
            expose::ParamVal<mesa_vid_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
            "This is the VLAN translation mapping table.\n Here the user stores VLAN translation mappings (VID->TVID) inside groups that can later be activated on specific switch interfaces";

    static constexpr const char *index_description =
            "Each entry in this table represents a VLAN translation mapping stored inside a specific VLAN translation Group.\nThe entry key is the Group ID and the source VLAN ID, while the value is the translated VID.\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_vlan_translation_group_mapping_key_t &i) {
        h.argument_properties(tag::Name("mappingkey"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(tag::Name("tvid"));
        h.argument_properties(expose::snmp::OidOffset(4));
        serialize(h, vtss_appl_vlan_trans_tvid_key(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vlan_translation_group_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_vlan_translation_group_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vlan_translation_group_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_vlan_translation_group_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_vlan_translation_group_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_vlan_translation_group_conf_def);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN_TRANSLATION);
};

struct IfTable {
    typedef expose::ParamList<
            expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_vlan_translation_if_conf_value_t *>> P;

    static constexpr const char *table_description =
            "This is the VLAN translation interface table. The number of interfaces is the total number of ports "
            "available on the switch. Each one of these interfaces can be set to use a specific Group of VLAN translation "
            "mappings, identified by the respective Group ID.";

    static constexpr const char *index_description =
            "Entries in this table represent switch interfaces and their matching VLAN translation Groups (identified through "
            "their Group IDs)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifidx"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_vlan_trans_findex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vlan_translation_if_conf_value_t &i) {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vlan_translation_if_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vlan_translation_if_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN_TRANSLATION);
};

}  // namespace interfaces
}  // namespace vlan_translation
}  // namespace appl
}  // namespace vtss

#endif  // _VLAN_TRANSLATION_SERIALIZER_HXX_
