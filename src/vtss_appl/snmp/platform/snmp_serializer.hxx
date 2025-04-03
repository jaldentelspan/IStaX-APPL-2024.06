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
#ifndef __VTSS_SNMP_SERIALIZER_HXX__
#define __VTSS_SNMP_SERIALIZER_HXX__

#include "vtss/appl/snmp.h"
#include "vtss_appl_serialize.hxx"

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t snmp_version_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_snmp_version_t,
                         "SnmpVersion",
                         snmp_version_txt,
                         "The version of SNMP");

extern const vtss_enum_descriptor_t snmp_security_level_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_snmp_security_level_t,
                         "SnmpSecurityLevel",
                         snmp_security_level_txt,
                         "The SNMP authentication protocol");

extern const vtss_enum_descriptor_t snmp_auth_protocol_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_snmp_auth_protocol_t,
                         "SnmpAuthProtocl",
                         snmp_auth_protocol_txt,
                         "The SNMP authentication protocol");

extern const vtss_enum_descriptor_t snmp_priv_protocol_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_snmp_priv_protocol_t,
                         "SnmpPrivProtocl",
                         snmp_priv_protocol_txt,
                         "The SNMP privacy protocol");

extern const vtss_enum_descriptor_t vtss_appl_snmp_security_model_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_snmp_security_model_t,
                         "SnmpSecurityModel",
                         vtss_appl_snmp_security_model_txt,
                         "The SNMP security model");

extern const vtss_enum_descriptor_t vtss_appl_snmp_view_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_snmp_view_type_t,
                         "SnmpViewType",
                         vtss_appl_snmp_view_type_txt,
                         "The SNMP view type");

extern const vtss_enum_descriptor_t vtss_appl_trap_notify_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_trap_notify_type_t,
                         "SnmpTrapNotifyType",
                         vtss_appl_trap_notify_type_txt,
                         "The SNMP trap notify type");

/*****************************************************************************
    Index serializer
*****************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_snmp_community_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_community_index_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.name, sizeof(s.name)),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Name of SNMP community"));

    m.add_leaf(vtss::AsIpv4(s.sip.address),
               vtss::tag::Name("SourceIP"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The SNMP access source IPv4 address"));

    m.add_leaf(vtss::AsInt(s.sip.prefix_size),
               vtss::tag::Name("SourceIPPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The SNMP access source IPv4 prefix size"));

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_community6_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_community6_index_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.name, sizeof(s.name)),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Name of SNMP community"));

    m.add_leaf(s.sip_ipv6.address,
               vtss::tag::Name("SourceIPv6"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The SNMP access source IPv6 address"));

    m.add_leaf(vtss::AsInt(s.sip_ipv6.prefix_size),
               vtss::tag::Name("SourceIPv6PrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The SNMP access source IPv6 prefix size"));

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_user_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_user_index_t"));
    int ix = 0;

    m.add_leaf(vtss::BinaryLen(s.engineid, VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN, VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN, s.engineid_len),
               vtss::tag::Name("EngineId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("SNMPv3 engine ID. The length is between 5 ~ 32 bytes. "
                                      "But all-zeros and all-'F's are not allowed."));
    
    m.add_leaf(vtss::AsDisplayString(s.user_name, sizeof(s.user_name)),
               vtss::tag::Name("UserName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The name of this entry")
               );
}

template <typename T>
void serialize(T &a, vtss_appl_snmp_user_to_access_group_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_user_to_access_group_index_t"));
    int ix = 0;

    m.add_leaf(s.security_model,
               vtss::tag::Name("SecurityModel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The security model of this entry. When v1 or v2c UserOrCommunity "
                                      "maps to community, when v3 UserOrCommunity maps to user. "
                                      "The value 'any' is not allowed."));

    m.add_leaf(vtss::AsDisplayString(s.user_or_community, sizeof(s.user_or_community)),
               vtss::tag::Name("UserOrCommunity"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The security name (user or community) of this entry"));
}

template <typename T>
void serialize(T &a, vtss_appl_snmp_view_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_view_index_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.view_name, sizeof(s.view_name)),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The name of this entry"));

    m.add_leaf(vtss::AsDisplayString(s.subtree, sizeof(s.subtree)),
               vtss::tag::Name("Subtree"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OID defining the root of the subtree to add to the named view."));

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_access_group_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_access_group_index_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.access_group_name, sizeof(s.access_group_name)),
               vtss::tag::Name("AccessGroupName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The access group name of this entry"));

    m.add_leaf(s.security_model,
               vtss::tag::Name("SecurityModel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The security model of this entry. Can be v1, v2c, usm or any."));

    m.add_leaf(s.security_level,
               vtss::tag::Name("SecurityLevel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The security level of this entry"));

}

template <typename T>
void serialize(T &a, vtss_appl_trap_receiver_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_trap_receiver_index_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.name, sizeof(s.name)),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Name of trap receiver"));

}

template <typename T>
void serialize(T &a, vtss_appl_trap_source_index_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_trap_source_index_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.name, sizeof(s.name)),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("SNMP trap event/table name")
    );

    m.add_leaf(vtss::AsInt(s.index_filter_id),
               vtss::tag::Name("IndexFilterID"),
               vtss::expose::snmp::RangeSpec<u32>(0, VTSS_APPL_SNMP_TRAP_FILTER_MAX - 1),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("ID of filter for this SNMP trap")
    );

}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_snmp_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_conf_t"));

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Global mode of SNMP.")
    );

    m.add_leaf(
        vtss::BinaryLen(s.engineid, VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN, VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN, s.engineid_len),
        vtss::tag::Name("EngineId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(4),
        vtss::tag::Description("SNMPv3 engine ID. The size of Engine ID is between 5 ~ 32 bytes."
            "But all-zeros and all-'F's are not allowed.")
    );

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_community_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_community_conf_t"));
    int ix = 0;

    m.add_rpc_leaf(
        vtss::AsDisplayString(s.secret, sizeof(s.secret)),
        vtss::tag::Name("Secret"),
        vtss::tag::Description("The community secret of the SNMP community.")
    );

    m.add_snmp_leaf(
        vtss::AsPasswordSetOnly(s.secret, sizeof(s.secret), "", 0),
        vtss::tag::Name("Secret"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The community secret of the SNMP community.")
    );

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_user_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_user_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.security_level,
        vtss::tag::Name("SecurityLevel"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The security level of this entry. The object is read-only if "
            "the entry is existent.")
    );

    m.add_leaf(
        s.auth_protocol,
        vtss::tag::Name("AuthProtocol"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The authentication protocol of this entry. The object is read-only if "
            "the entry is existent.")
    );

    m.add_leaf(
        vtss::AsPasswordSetOnly(s.auth_password, sizeof(s.auth_password), "", 8),
        vtss::tag::Name("AuthPassword"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The authentication password of this entry. Acceptable string length range from 8 to 40."
             "The object is read-only if the entry is existent.")
    );

    m.add_leaf(
        s.priv_protocol,
        vtss::tag::Name("PrivProtocol"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The privacy protocol of this entry. The object is read-only if "
            "the entry is existent.")
    );

    m.add_leaf(
        vtss::AsPasswordSetOnly(s.priv_password, sizeof(s.priv_password), "", 8),
        vtss::tag::Name("PrivPassword"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The privacy password of this entry. Acceptable string length range from 8 to 32."
            "The object is read-only if the entry is existent.")
    );

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_user_to_access_group_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_user_to_access_group_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.access_group_name, sizeof(s.access_group_name)),
        vtss::tag::Name("AccessGroupName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The access group name of this entry.")
    );

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_view_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_view_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.view_type,
        vtss::tag::Name("ViewType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The view type of this entry. The value can be set to 'included' or 'excluded',"
            "'included' indicates the view subtree is visible, otherwise the view subtree is invisible.")
    );

}

template <typename T>
void serialize(T &a, vtss_appl_snmp_access_group_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_snmp_access_group_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.read_view_name, sizeof(s.read_view_name)),
        vtss::tag::Name("ReadViewName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The name of the MIB view defining the MIB objects for which this request may request the current values.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.write_view_name, sizeof(s.write_view_name)),
        vtss::tag::Name("WriteViewName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The name of the MIB view defining the MIB objects for which this request may potentially set new values.")
    );

}

template <typename T>
void serialize(T &a, vtss_appl_trap_receiver_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_trap_receiver_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.enable),
        vtss::tag::Name("Enable"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("SNMP trap receiver enabled.")
    );

    m.add_leaf(
        s.dest_addr,
        vtss::tag::Name("Address"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Internet address of the SNMP trap receiver.")
    );

    m.add_leaf(
        s.port,
        vtss::tag::Name("Port"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Port number of the SNMP trap receiver.")
    );

    m.add_leaf(
        s.version,
        vtss::tag::Name("Version"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("SNMP Version. The supported versions are snmpV1, snmpV2c, snmpV3.")
    );

    m.add_rpc_leaf(
        vtss::AsDisplayString(s.community, sizeof(s.community)),
        vtss::tag::Name("Community"),
        vtss::tag::Description("The community secret to use for SNMP traps.")
    );

    m.add_snmp_leaf(
        vtss::AsPasswordSetOnly(s.community, sizeof(s.community), "", 0),
        vtss::tag::Name("Community"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The community secret to use for SNMP traps.")
    );

    m.add_leaf(
        s.notify_type,
        vtss::tag::Name("NotifyType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Notification type: trap or inform.")
    );

    m.add_leaf(
        vtss::AsInt(s.timeout),
        vtss::tag::Name("Timeout"),
        vtss::expose::snmp::RangeSpec<u32>(1, VTSS_APPL_SNMP_TRAP_TIMEOUT_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Timeout. Only relevant for informs.")
    );

    m.add_leaf(
        vtss::AsInt(s.retries),
        vtss::tag::Name("Retries"),
        vtss::expose::snmp::RangeSpec<u32>(1, VTSS_APPL_SNMP_TRAP_RETRIES_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Retries. Only relevant for informs.")
    );

    m.add_leaf(vtss::BinaryLen(s.engineid, VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN, VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN, s.engineid_len),
               vtss::tag::Name("EngineId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("SNMPv3 engine ID. The length is between 5 ~ 32 bytes. "
                    "But all-zeros and all-'F's are not allowed."));
    
    m.add_leaf(vtss::AsDisplayString(s.user_name, sizeof(s.user_name)),
               vtss::tag::Name("UserName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("SNMPv3 user name.")
               );

}

template <typename T>
void serialize(T &a, vtss_appl_trap_source_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_trap_source_conf_t"));
    int ix = 0;

    m.add_rpc_leaf(vtss::AsSnmpObjectIdentifier(s.index_filter, VTSS_APPL_SNMP_MAX_OID_LEN, s.index_filter_len),
               vtss::tag::Name("IndexFilter"),
               vtss::tag::Description("OID subtree to use as index filter.")
    );

    m.add_snmp_leaf(vtss::BinaryU32Len(s.index_filter, VTSS_APPL_SNMP_MAX_OID_LEN, s.index_filter_len),
               vtss::tag::Name("IndexFilter"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OID subtree to use as index filter. Every OID is hex encoded as 4 bytes.")
    );

    m.add_leaf(vtss::BinaryLen(s.index_mask, VTSS_APPL_SNMP_MAX_SUBTREE_LEN, s.index_mask_len),
               vtss::tag::Name("IndexMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Mask for the OID subtree to use as index filter.")
    );

    m.add_leaf(s.filter_type,
               vtss::tag::Name("FilterType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The filter type of this entry. The value can be set to 'included' or 'excluded'.")
    );

}

namespace vtss {
namespace appl {
namespace snmp {
namespace interfaces {
struct SnmpParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_snmp_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_conf_t &i) {
        h.argument_properties(tag::Name("global_config"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpCommunityEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_snmp_community_index_t>,
        vtss::expose::ParamVal<vtss_appl_snmp_community_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMPv3 communities.";

    static constexpr const char *index_description =
        "The entry index key is community.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_community_index_t &i) {
        h.argument_properties(tag::Name("community_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_snmp_community_conf_t &i) {
        h.argument_properties(tag::Name("community_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_community_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_snmp_community_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_community_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_snmp_community_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_snmp_community_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_snmp_community_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpCommunity6Entry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_snmp_community6_index_t>,
        vtss::expose::ParamVal<vtss_appl_snmp_community_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMPv3 communities for IPv6.";

    static constexpr const char *index_description =
        "The entry index key is community.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_community6_index_t &i) {
        h.argument_properties(tag::Name("community_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_snmp_community_conf_t &i) {
        h.argument_properties(tag::Name("community_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_community6_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_snmp_community6_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_community6_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_snmp_community6_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_snmp_community6_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_snmp_community6_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpUserEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_snmp_user_index_t>,
        vtss::expose::ParamVal<vtss_appl_snmp_user_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMPv3 users.";

    static constexpr const char *index_description =
        "The entry index keys are engine ID and user name.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_user_index_t &i) {
        h.argument_properties(tag::Name("user_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_snmp_user_conf_t &i) {
        h.argument_properties(tag::Name("user_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_user_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_snmp_user_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_user_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_snmp_user_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_snmp_user_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_snmp_user_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpGroupEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_snmp_user_to_access_group_index_t>,
        vtss::expose::ParamVal<vtss_appl_snmp_user_to_access_group_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
    "This is a table for configuring SNMPv3 groups.";

    static constexpr const char *index_description =
    "The entry index keys are security model and user/community name.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_user_to_access_group_index_t &i) {
        h.argument_properties(tag::Name("group_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_snmp_user_to_access_group_conf_t &i) {
        h.argument_properties(tag::Name("group_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_user_to_access_group_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_snmp_user_to_access_group_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_user_to_access_group_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_snmp_user_to_access_group_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_snmp_user_to_access_group_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_snmp_user_to_access_group_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpViewEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_snmp_view_index_t>,
        vtss::expose::ParamVal<vtss_appl_snmp_view_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMPv3 views.";

    static constexpr const char *index_description =
        "The entry index keys are name and subtree.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_view_index_t &i) {
        h.argument_properties(tag::Name("view_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_snmp_view_conf_t &i) {
        h.argument_properties(tag::Name("view_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_view_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_snmp_view_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_view_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_snmp_view_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_snmp_view_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_snmp_view_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpAccessEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_snmp_access_group_index_t>,
        vtss::expose::ParamVal<vtss_appl_snmp_access_group_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMPv3 accesse groups.";

    static constexpr const char *index_description =
        "The entry index keys are access group name, security model and security level.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_snmp_access_group_index_t &i) {
        h.argument_properties(tag::Name("access_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_snmp_access_group_conf_t &i) {
        h.argument_properties(tag::Name("access_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_snmp_access_group_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_snmp_access_group_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_snmp_access_group_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_snmp_access_group_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_snmp_access_group_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_snmp_access_group_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpTrapReceiverEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_trap_receiver_index_t>,
        vtss::expose::ParamVal<vtss_appl_trap_receiver_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMP trap receivers.";

    static constexpr const char *index_description =
        "The entry index keys is trap receiver name.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_trap_receiver_index_t &i) {
        h.argument_properties(tag::Name("trap_receiver_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_trap_receiver_conf_t &i) {
        h.argument_properties(tag::Name("trap_receiver_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_trap_receiver_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_trap_receiver_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_trap_receiver_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_trap_receiver_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_trap_receiver_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_trap_receiver_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct SnmpTrapSourceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_trap_source_index_t>,
        vtss::expose::ParamVal<vtss_appl_trap_source_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for configuring SNMP trap sources.\n"
        "A trap is sent for the given trap source if at least one filter with "
        "filter_type included matches the filter, and no filters with filter_type "
        "excluded matches.";

    static constexpr const char *index_description =
        "The entry index keys is trap source name and a filter ID.\n"
        "The filter matches an index OID if the index_filter matches the OID of the "
        "index taking index_mask into account.\n"
        "Each bit of index_mask corresponds to a sub-identifier of index_filter, "
        "with the most significant bit of the i-th octet of this octet string value "
        "(extended if necessary) corresponding to the (8*i - 7)-th sub-identifier.\n"
        "Each bit of index_mask specifies whether or not the corresponding "
        "sub-identifiers must match when determining if an OID matches; a '1' "
        "indicates that an exact match must occur; a '0' indicates 'wild card'.\n"
        "If the value of index_mask is M bits long and there are more than M  "
        "sub-identifiers in index_filter, then the bit mask is extended with 1's to "
        "be the required length.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_trap_source_index_t &i) {
        h.argument_properties(tag::Name("trap_source_key"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_trap_source_conf_t &i) {
        h.argument_properties(tag::Name("trap_source_name"));
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_trap_source_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_trap_source_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_trap_source_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_trap_source_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_trap_source_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_trap_source_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

}  // namespace interfaces
}  // namespace snmp
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_SNMP_SERIALIZER_HXX__ */
