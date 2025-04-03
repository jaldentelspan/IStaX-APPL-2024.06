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
#ifndef __VTSS_ACCESS_MGMT_SERIALIZER_HXX__
#define __VTSS_ACCESS_MGMT_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/access_management.h"

static mesa_rc vtss_appl_access_mgmt_control_statistics_dummy_get(BOOL *const act_flag) {
    *act_flag = FALSE;
    return VTSS_RC_OK;
}

/*****************************************************************************
    Data type serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_access_mgmt_ctrl_bool_t, BOOL, a, s) {
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("Clear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("To trigger the control action (only) when TRUE.")
    );
}

/*****************************************************************************
    Enumerator serializer
*****************************************************************************/


/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_access_mgmt_conf_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("AccessIndex"),
        vtss::expose::snmp::RangeSpec<u32>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Index for Access Management IPv4/IPv6 table.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_access_mgmt_stat_index, u8, a, s) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("NullIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Entrance for Access Management statistics "
            "(Only accept 1 as valid key for table access).")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_access_mgmt_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_access_mgmt_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable the Access Management global functionality.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_access_mgmt_ipv4_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_access_mgmt_ipv4_t"));
    int ix = 0;

    m.add_leaf(
        s.vlan_id,
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ID of specific VLAN interface that Access "
            "Management should take effect for IPv4.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.start_address),
        vtss::tag::Name("StartAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The starting IPv4 address of the range "
            "that Access Management performs checking.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.end_address),
        vtss::tag::Name("EndAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ending IPv4 address of the range "
            "that Access Management performs checking.")
    );

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    m.add_leaf(
        vtss::AsBool(s.web_services),
        vtss::tag::Name("WebServices"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable HTTP and HTTPS functionality via Access Management. "
            "At least one of WebServices/SnmpServices/TelnetServices has to be enabled for a "
            "specific AccessIndex in Access Management IPv4 table.")
    );
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */

#ifdef VTSS_SW_OPTION_SNMP
    m.add_leaf(
        vtss::AsBool(s.snmp_services),
        vtss::tag::Name("SnmpServices"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable SNMP functionality via Access Management. "
            "At least one of WebServices/SnmpServices/TelnetServices has to be enabled for a "
            "specific AccessIndex in Access Management IPv4 table.")
    );
#endif /* VTSS_SW_OPTION_SNMP */

    m.add_leaf(
        vtss::AsBool(s.telnet_services),
        vtss::tag::Name("TelnetServices"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable TELNET/SSH functionality via Access Management. "
            "At least one of WebServices/SnmpServices/TelnetServices has to be enabled for a "
            "specific AccessIndex in Access Management IPv4 table.")
    );
}

#ifdef VTSS_SW_OPTION_IPV6
template<typename T>
void serialize(T &a, vtss_appl_access_mgmt_ipv6_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_access_mgmt_ipv6_t"));
    int ix = 0;

    m.add_leaf(
        s.vlan_id,
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ID of specific VLAN interface that Access "
            "Management should take effect for IPv6.")
    );

    m.add_leaf(
        s.start_address,
        vtss::tag::Name("StartAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The starting IPv6 address of the range "
            "that Access Management performs checking.")
    );

    m.add_leaf(
        s.end_address,
        vtss::tag::Name("EndAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ending IPv6 address of the range "
            "that Access Management performs checking.")
    );
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    m.add_leaf(
        vtss::AsBool(s.web_services),
        vtss::tag::Name("WebServices"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable HTTP and HTTPS functionality via Access Management. "
            "At least one of WebServices/SnmpServices/TelnetServices has to be enabled for a "
            "specific AccessIndex in Access Management IPv6 table.")
    );
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */

#ifdef VTSS_SW_OPTION_SNMP
    m.add_leaf(
        vtss::AsBool(s.snmp_services),
        vtss::tag::Name("SnmpServices"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable SNMP functionality via Access Management. "
            "At least one of WebServices/SnmpServices/TelnetServices has to be enabled for a "
            "specific AccessIndex in Access Management IPv6 table.")
    );
#endif /* VTSS_SW_OPTION_SNMP */

    m.add_leaf(
        vtss::AsBool(s.telnet_services),
        vtss::tag::Name("TelnetServices"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable TELNET/SSH functionality via Access Management. "
            "At least one of WebServices/SnmpServices/TelnetServices has to be enabled for a "
            "specific AccessIndex in Access Management IPv6 table.")
    );
}
#endif /* VTSS_SW_OPTION_IPV6 */

template<typename T>
void serialize(T &a, vtss_appl_access_mgmt_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_access_mgmt_statistics_t"));
    int ix = 0;

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    m.add_leaf(
        s.http_receive_cnt,
        vtss::tag::Name("HttpReceivedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Received count of frames via HTTP.")
    );

    m.add_leaf(
        s.http_permit_cnt,
        vtss::tag::Name("HttpAllowedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Permit count of frames via HTTP.")
    );

    m.add_leaf(
        s.http_discard_cnt,
        vtss::tag::Name("HttpDiscardedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Drop count of frames via HTTP.")
    );

#if defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    m.add_leaf(
        s.https_receive_cnt,
        vtss::tag::Name("HttpsReceivedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Received count of frames via HTTPS.")
    );

    m.add_leaf(
        s.https_permit_cnt,
        vtss::tag::Name("HttpsAllowedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Permit count of frames via HTTPS.")
    );

    m.add_leaf(
        s.https_discard_cnt,
        vtss::tag::Name("HttpsDiscardedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Drop count of frames via HTTPS.")
    );
#endif //VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */

#ifdef VTSS_SW_OPTION_SNMP
    m.add_leaf(
        s.snmp_receive_cnt,
        vtss::tag::Name("SnmpReceivedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Received count of frames via SNMP.")
    );

    m.add_leaf(
        s.snmp_permit_cnt,
        vtss::tag::Name("SnmpAllowedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Permit count of frames via SNMP.")
    );

    m.add_leaf(
        s.snmp_discard_cnt,
        vtss::tag::Name("SnmpDiscardedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Drop count of frames via SNMP.")
    );
#endif /* VTSS_SW_OPTION_SNMP */

    m.add_leaf(
        s.telnet_receive_cnt,
        vtss::tag::Name("TelnetReceivedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Received count of frames via TELNET.")
    );

    m.add_leaf(
        s.telnet_permit_cnt,
        vtss::tag::Name("TelnetAllowedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Permit count of frames via TELNET.")
    );

    m.add_leaf(
        s.telnet_discard_cnt,
        vtss::tag::Name("TelnetDiscardedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Drop count of frames via TELNET.")
    );

    m.add_leaf(
        s.ssh_receive_cnt,
        vtss::tag::Name("SshReceivedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Received count of frames via SSH.")
    );

    m.add_leaf(
        s.ssh_permit_cnt,
        vtss::tag::Name("SshAllowedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Permit count of frames via SSH.")
    );

    m.add_leaf(
        s.ssh_discard_cnt,
        vtss::tag::Name("SshDiscardedPkts"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Drop count of frames via SSH.")
    );
}


namespace vtss {
namespace appl {
namespace access_mgmt {
namespace interfaces {

struct AccessMgmtParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_access_mgmt_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_access_mgmt_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_access_mgmt_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_access_mgmt_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct AccessMgmtIpv4ConfigTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_access_mgmt_ipv4_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing Access Management per IPv4 basis";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_access_mgmt_conf_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_access_mgmt_ipv4_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_access_mgmt_ipv4_table_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_access_mgmt_ipv4_table_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_access_mgmt_ipv4_table_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_access_mgmt_ipv4_table_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_access_mgmt_ipv4_table_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

#ifdef VTSS_SW_OPTION_IPV6
struct AccessMgmtIpv6ConfigTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_access_mgmt_ipv6_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing Access Management per IPv6 basis";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_access_mgmt_conf_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_access_mgmt_ipv6_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_access_mgmt_ipv6_table_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_access_mgmt_ipv6_table_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_access_mgmt_ipv6_table_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_access_mgmt_ipv6_table_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_access_mgmt_ipv6_table_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};
#endif /* VTSS_SW_OPTION_IPV6 */

struct AccessMgmtStatisticsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_access_mgmt_statistics_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_access_mgmt_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_access_mgmt_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct AccessMgmtCtrlStatsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<BOOL *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_access_mgmt_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_access_mgmt_control_statistics_dummy_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_access_mgmt_control_statistics_clr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY);
};

}  // namespace interfaces
}  // namespace access_mgmt
}  // namespace appl
}  // namespace vtss
#endif /* __VTSS_ACCESS_MGMT_SERIALIZER_HXX__ */
