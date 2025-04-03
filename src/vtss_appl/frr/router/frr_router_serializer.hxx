/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.
 store and modify, the software and its source code is granted but only in

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

#ifndef _FRR_ROUTER_SERIALIZER_HXX_
#define _FRR_ROUTER_SERIALIZER_HXX_

/**
 * \file frr_router_serializer.hxx
 * \brief This file contains the definitions of module serializer.
 *
 * In this file all interface-descriptors and all templated serialize functions
 * must be defind and implemented.
 *
 * The central part is the interface-descriptors, it is a C++ struct/class which
 * provides various bits of information. And a key part on a given interface is
 * serialization and de-serialization.
 *
 * Serialization:
 * It is the act of converting an internal data structure into a public data
 * structure using the encoding dened by the interface.
 *
 * De-serialization:
 * It is the reverse part, where the public data structures represented in the
 * encoding used by the interface is converted into the internal data
 * structures.
 *
 * For more deatil information, please refer to 'TN1255-vtss-expose' document.
 */

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_router_api.hxx"
#include "frr_router_expose.hxx"
#include "vtss/appl/router.h"
#include "vtss/appl/types.hxx"
#include "vtss_appl_serialize.hxx"

/******************************************************************************/
/** The JSON Get-All functions                                                */
/******************************************************************************/
mesa_rc vtss_appl_router_key_chain_name_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_router_key_chain_key_conf_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_router_access_list_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

/******************************************************************************/
/** enum serializer (enum value-string mapping)                               */
/******************************************************************************/

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_router_access_list_mode_t, "RouterAccessListMode",
    vtss_appl_router_access_list_mode_txt,
    "The access right mode of the router access-list entry.");

/******************************************************************************/
/** Table index/key serializer                                                */
/******************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(router_key_ipv4_network, mesa_ipv4_network_t, a, s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("mesa_ipv4_network_t"));
    m.add_leaf(vtss::AsIpv4(s.inner.address), vtss::tag::Name("Network"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("IPv4 network address."));

    m.add_leaf(vtss::AsInt(s.inner.prefix_size),
               vtss::tag::Name("IpSubnetMaskLength"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IPv4 network mask length."));
}

//----------------------------------------------------------------------------
//**  Key-chain
//----------------------------------------------------------------------------
VTSS_SNMP_TAG_SERIALIZE(router_key_chain_name,
                        vtss_appl_router_key_chain_name_t, a, s)
{
    a.add_leaf(vtss::AsDisplayString(s.inner.name, sizeof(s.inner.name)),
               vtss::tag::Name("KeyChainName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The key chain name."));
}

VTSS_SNMP_TAG_SERIALIZE(router_key_chain_key_id,
                        vtss_appl_router_key_chain_key_id_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("KeyId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(
                   VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MIN,
                   VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The key ID of key chain."));
}

VTSS_SNMP_TAG_SERIALIZE(router_key_access_list_name,
                        vtss_appl_router_access_list_name_t, a, s)
{
    a.add_leaf(vtss::AsDisplayString(s.inner.name, sizeof(s.inner.name)),
               vtss::tag::Name("Name"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Access-list name."));
}

VTSS_SNMP_TAG_SERIALIZE(router_access_mode_key,
                        vtss_appl_router_access_list_mode_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The access right mode of the access-list entry."));
}

VTSS_SNMP_TAG_SERIALIZE(router_key_access_list_precedence, uint32_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("RouterAccessListPrecedence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The precedence of router access-list entry."));
}

/******************************************************************************/
/** Table entry serializer                                                    */
/******************************************************************************/
//------------------------------------------------------------------------------
//** Router module capabilities
//------------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_router_capabilities_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_router_capabilities_t"));

    int ix = 0;

    // key chain name max. count
    m.add_leaf(p.key_chain_name_list_max_count,
               vtss::tag::Name("MaxKeyChainNameMaxCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum count of the router key-chain name list"));

    // Router valid range: key chain name
    m.add_leaf(p.key_chain_name_len_min, vtss::tag::Name("MinKeyChainNameLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum name length of router key chain"));
    m.add_leaf(p.key_chain_name_len_max, vtss::tag::Name("MaxKeyChainNameLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum name length of router key chain"));

    // Router valid range: key chain key id
    m.add_leaf(p.key_chain_key_id_min, vtss::tag::Name("MinKeyChainKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum value of router key chain key ID"));
    m.add_leaf(p.key_chain_key_id_max, vtss::tag::Name("MaxKeyChainKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum value of router key chain key ID"));

    // Router valid range: key chain plain text key string
    m.add_leaf(p.key_chain_plain_text_key_str_len_min,
               vtss::tag::Name("MinKeyChainPlainTextKeyStringLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum length of router key chain "
                                      "plain text key string"));
    m.add_leaf(p.key_chain_plain_text_key_str_len_max,
               vtss::tag::Name("MaxKeyChainPlainTextKeyStringLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum length of router key chain "
                                      "plain text key string"));

    // Router valid range: key chain encrypted key string
    m.add_leaf(p.key_chain_encrypted_key_str_len_min,
               vtss::tag::Name("MinKeyChainKeyEncryptedStringLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum length of router key chain "
                                      "encrypted key string"));
    m.add_leaf(p.key_chain_encrypted_key_str_len_max,
               vtss::tag::Name("MaxKeyChainKeyEncryptedStringLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum length of router key chain "
                                      "encrypted key string"));

    // access-list max. count
    m.add_leaf(p.access_list_max_count, vtss::tag::Name("AccessListMaxCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum count of the router access-list"));

    // RIP valid range: access-list name
    m.add_leaf(p.access_list_name_len_min,
               vtss::tag::Name("MinAccessListNameLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum name length of router access-list"));
    m.add_leaf(p.access_list_name_len_max,
               vtss::tag::Name("MaxAccessListNameLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum name length of router access-list"));

    // RIP valid range: access-list precedence
    m.add_leaf(
        p.ace_precedence_min, vtss::tag::Name("MinAcePrecedence"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum value of RIP access-list entry precedence"));
    m.add_leaf(
        p.ace_precedence_max, vtss::tag::Name("MaxAcePrecedence"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of RIP access-list entry precedence"));
}

//----------------------------------------------------------------------------
//**  Key-chain
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_router_key_chain_key_conf_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_router_key_chain_key_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.is_encrypted), vtss::tag::Name("IsEncrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The flag indicates the key string is encrypted or "
                   "not. TRUE means the key string is encrypted."
                   " FALSE means the key string is plain text."));

    m.add_leaf(vtss::AsDisplayString(p.key, sizeof(p.key)),
               vtss::tag::Name("KeyString"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The key string."));
}

//----------------------------------------------------------------------------
//**  Access-list
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_router_ace_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_router_ace_conf_t"));
    int ix = 0;

    m.add_leaf(p.mode, vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The access right mode of the access-list entry."));

    m.add_leaf(vtss::AsIpv4(p.network.address),
               vtss::tag::Name("NetworkAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The IPv4 address of the access-list entry."));

    m.add_leaf(vtss::AsInt(p.network.prefix_size),
               vtss::tag::Name("NetworkPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The network prefix size of "
                                      "the access-list entry."));
}

/******************************************************************************/
/** Table-entry/data structure serializer                                     */
/******************************************************************************/
namespace vtss
{
namespace appl
{
namespace router
{
namespace interfaces
{

//------------------------------------------------------------------------------
//** Router module capabilities
//------------------------------------------------------------------------------
struct RouterCapabilitiesTabular {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_router_capabilities_t * >>
                                                             P;

    /* Description */
    static constexpr const char *table_description =
        "This is router capabilities tabular. It provides the capabilities "
        "of router configuration.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_router_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Capabilities"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_GET_PTR(vtss_appl_router_capabilities_get);
};

//------------------------------------------------------------------------------
//** Router key chain
//------------------------------------------------------------------------------
struct RouterConfigKeyChainNameEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_router_key_chain_name_t * >>
                                                               P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is router key chain name table.";
    static constexpr const char *index_description =
        "Each row contains the name for the key chain.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_router_key_chain_name_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("KeyChainName"));
        serialize(h, router_key_chain_name(i));
    }

    /* No Entry data */

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR_ROUTER);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_router_key_chain_name_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_router_key_chain_name_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_router_key_chain_name_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_router_key_chain_name_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_router_key_chain_name_del);
    // VTSS_JSON_GET_ALL_PTR(vtss_appl_router_key_chain_name_get_all_json);
};

struct RouterConfigKeyChainKeyConfEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_router_key_chain_name_t *>,
         vtss::expose::ParamKey<vtss_appl_router_key_chain_key_id_t>,
         vtss::expose::ParamVal<vtss_appl_router_key_chain_key_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is router key chain key ID configuration table.";
    static constexpr const char *index_description =
        "Each row contains the key configuration of the corresponding key "
        "chain and key ID.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_router_key_chain_name_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("KeyChainName"));
        serialize(h, router_key_chain_name(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_router_key_chain_key_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("KeyId"));
        serialize(h, router_key_chain_key_id(i));
    }

    /* No Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_router_key_chain_key_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("KeyConfig"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR_ROUTER);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_router_key_chain_key_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_router_key_chain_key_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_router_key_chain_key_conf_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_router_key_chain_key_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_router_key_chain_key_conf_del);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_router_key_chain_key_conf_get_all_json);
};

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
struct RouterConfigAccessListEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_router_access_list_name_t *>,
         vtss::expose::ParamKey<vtss_appl_router_access_list_mode_t>,
         vtss::expose::ParamKey<mesa_ipv4_network_t >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is router access-list configuration table.";
    static constexpr const char *index_description =
        "Each access-list entry has a set of parameters.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_router_access_list_name_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, router_key_access_list_name(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_router_access_list_mode_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              vtss::tag::Name("Mode"));
        serialize(h, router_access_mode_key(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv4_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              vtss::tag::Name("NetworkAddress"));
        serialize(h, router_key_ipv4_network(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_router_access_list_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_router_access_list_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_router_access_list_conf_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_router_access_list_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_router_access_list_conf_del);
};

struct RouterStatusAccessListEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_router_access_list_name_t *>,
         vtss::expose::ParamKey<uint32_t>,
         vtss::expose::ParamVal<vtss_appl_router_ace_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is router access-list configuration table.";
    static constexpr const char *index_description =
        "Each access-list entry has a set of parameters.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_router_access_list_name_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, router_key_access_list_name(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(uint32_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              vtss::tag::Name("Precedence"));
        serialize(h, router_key_access_list_precedence(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_router_ace_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              vtss::tag::Name("RouterAceConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_router_access_list_precedence_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_router_access_list_precedence_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_router_access_list_status_get_all_json);
};

}  // namespace interfaces
}  // namespace router
}  // namespace appl
}  // namespace vtss

#endif  // _FRR_ROUTER_SERIALIZER_HXX_

