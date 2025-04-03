/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VCL_SERIALIZER_HXX_
#define _VCL_SERIALIZER_HXX_

#include "vtss/appl/vcl.h"
#include "vtss/appl/module_id.h"
#include "vtss/appl/types.hxx"
#include "vtss_appl_serialize.hxx"

extern vtss_enum_descriptor_t vtss_appl_vcl_proto_encap_type_txt[];

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_vcl_proto_encap_type_t, "VclProtoEncapType",
                         vtss_appl_vcl_proto_encap_type_txt, "The frame encapsulation type");

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_vcl_mac_key, mesa_mac_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("MacAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The MAC address for which this entry is applicable."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_vcl_vid_filter_key, mesa_vid_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("Vid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The VID used for filtering."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_vcl_mac_filter_key, vtss_appl_vcap_mac_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("mac_vcap"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The MAC filter."));
}

template <typename T>
void serialize(T &a, vtss_appl_vcl_generic_conf_global_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_vcl_generic_conf_global_t"));
    vtss::PortListStackable &list = (vtss::PortListStackable &)p.ports;

    m.add_leaf(vtss::AsInt(p.vid),
               vtss::tag::Name("VlanId"),
               vtss::expose::snmp::RangeSpec<u32>(1, 4095),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Vlan id of the mapping."));

    m.add_leaf(list,
               vtss::tag::Name("PortList"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("List of stack/switch ports on which "
                                      "this entry is active."));
}

struct AsVclProto;

VTSS_SNMP_TAG_SERIALIZE(AsVclProto, vtss_appl_vcl_proto_t, a, s)
{
    a.add_leaf(s, // This is not really a tag serializer, just an encapsulation to allow the usage of add_leaf
               vtss::tag::Name("ProtocolEncapsulation"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The protocol encapsulation of the Protocol to Group mapping."));
}

template <typename T>
void serialize(T &a, vtss_appl_vcl_proto_group_conf_proto_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_vcl_proto_group_conf_proto_t"));
    m.add_leaf(vtss::AsDisplayString((char *)p.name, sizeof(p.name)),
               vtss::tag::Name("ProtocolGroupName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(" This is a name identifying the protocol group."));
}

VTSS_SNMP_TAG_SERIALIZE(VCL_SERIALIZER_ifindex, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

namespace vtss
{
namespace appl
{
namespace vcl
{
namespace interfaces
{

struct MacEntry {
    typedef expose::ParamList <
    expose::ParamKey<mesa_mac_t>,
           expose::ParamVal<vtss_appl_vcl_generic_conf_global_t * >> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "This is the MAC address to VLAN ID configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a configured MAC-based classification.\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_mac_t &i)
    {
        h.argument_properties(tag::Name("mac"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_vcl_mac_key(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vcl_generic_conf_global_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vcl_mac_table_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_vcl_mac_table_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vcl_mac_table_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_vcl_mac_table_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_vcl_mac_table_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_vcl_mac_table_conf_def);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL);
};

struct IpSubnetEntry {
    typedef expose::ParamList <
    expose::ParamKey<mesa_ipv4_network_t>,
           expose::ParamVal<vtss_appl_vcl_generic_conf_global_t * >> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "This is the IP Subnet to VLAN ID configuration table. "
        "The key of this table is the IP subnet expressed as x.x.x.x/x, "
        "where the first 4 octets represent the IPv4 address "
        "and the last one is the mask length.\n"
        "NOTE#1: Inside the VCL module these entries are actually sorted based "
        "on a priority defined by the mask length, so that subnets with larger mask lengths "
        "are first in the list, followed by entries with smaller mask lengths. "
        "SNMP cannot follow this sorting, therefore the order the entries are "
        "retrieved by the iterator may not be the same as the actually stored order. "
        "(This is not an issue, but should be taken into consideration when using the SNMP "
        "interface to create a user interface.\n"
        "NOTE#2: Even though only the subnet address is stored in the table "
        "(i.e. both 192.168.1.0/4 and 192.168.2.0/4 will end up as 192.0.0.0/4), "
        "the SNMP iterator will NOT take this into consideration. "
        "So, when searching the next subnet of 192.168.1.0/4, the result could be "
        "193.0.0.0/4 but not 192.168.1.0/24 (granted that these entries are present)";

    static constexpr const char *index_description =
        "Each entry in this table represents a configured IP Subnet-based classification.\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_ipv4_network_t &i)
    {
        h.add_snmp_leaf(vtss::AsIpv4(i.address),
                        vtss::tag::Name("IpSubnetAddress"),
                        vtss::expose::snmp::Status::Current,
                        vtss::expose::snmp::OidElementValue(1),
                        vtss::tag::Description("The IP subnet address for which this entry is applicable."));

        h.add_snmp_leaf(vtss::AsInt(i.prefix_size),
                        vtss::tag::Name("IpSubnetMaskLength"),
                        vtss::expose::snmp::RangeSpec<u32>(0, 32),
                        vtss::expose::snmp::Status::Current,
                        vtss::expose::snmp::OidElementValue(2),
                        vtss::tag::Description("The IP subnet mask length for which this entry is applicable."));

        h.add_rpc_leaf(i, vtss::tag::Name("Address"), vtss::tag::Description("The IP subnet address for which this entry is applicable."));

    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vcl_generic_conf_global_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vcl_ip_table_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_vcl_ip_table_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vcl_ip_table_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_vcl_ip_table_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_vcl_ip_table_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_vcl_ip_table_conf_def);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL);
};

struct ProtocolProtoEntry {
    typedef expose::ParamList <
    expose::ParamKey<vtss_appl_vcl_proto_t>,
           expose::ParamVal<vtss_appl_vcl_proto_group_conf_proto_t * >> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "This is the Protocol to Protocol Group mapping table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a Protocol to Group mapping.\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_vcl_proto_t &i)
    {
        h.argument_properties(tag::Name("protocol"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, AsVclProto(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vcl_proto_group_conf_proto_t &i)
    {
        h.argument_properties(tag::Name("name"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vcl_proto_table_proto_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_vcl_proto_table_proto_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vcl_proto_table_proto_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_vcl_proto_table_proto_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_vcl_proto_table_proto_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_vcl_proto_table_proto_def);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL);
};

struct ProtocolGroupEntry {
    typedef expose::ParamList <
    expose::ParamKey<vtss_appl_vcl_proto_group_conf_proto_t>,
           expose::ParamVal<vtss_appl_vcl_generic_conf_global_t * >> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "This is the Protocol Group to VLAN ID configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a Protocol Group to VLAN ID mapping.\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_vcl_proto_group_conf_proto_t &i)
    {
        h.argument_properties(tag::Name("groupname"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vcl_generic_conf_global_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vcl_proto_table_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_vcl_proto_table_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vcl_proto_table_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_vcl_proto_table_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_vcl_proto_table_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_vcl_proto_table_conf_def);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL);
};

}  // namespace interfaces
}  // namespace vcl
}  // namespace appl
}  // namespace vtss

#endif  // _VCL_SERIALIZER_HXX_
