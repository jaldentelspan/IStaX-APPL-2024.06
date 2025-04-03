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
#ifndef __VTSS_AUTH_SERIALIZER_HXX__
#define __VTSS_AUTH_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/auth.h"

extern vtss_enum_descriptor_t vtss_auth_authen_method_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_auth_authen_method_t,
                         "AuthAuthenMethod",
                         vtss_auth_authen_method_txt,
                         "This enumeration defines the available authentication methods.");

extern vtss_enum_descriptor_t vtss_auth_author_method_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_auth_author_method_t,
                         "AuthAuthorMethod",
                         vtss_auth_author_method_txt,
                         "This enumeration defines the available authorization methods.");

extern vtss_enum_descriptor_t vtss_auth_acct_method_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_auth_acct_method_t,
                         "AuthAcctMethod",
                         vtss_auth_acct_method_txt,
                         "This enumeration defines the available accounting methods.");


mesa_rc authen_console_get(uint32_t ix, vtss_appl_auth_authen_method_t *method);
mesa_rc authen_console_set(uint32_t ix, const vtss_appl_auth_authen_method_t *method);
mesa_rc authen_telnet_get(uint32_t ix, vtss_appl_auth_authen_method_t *method);
mesa_rc authen_telnet_set(uint32_t ix, const vtss_appl_auth_authen_method_t *method);
mesa_rc authen_ssh_get(uint32_t ix, vtss_appl_auth_authen_method_t *method);
mesa_rc authen_ssh_set(uint32_t ix, const vtss_appl_auth_authen_method_t *method);
mesa_rc authen_http_get(uint32_t ix, vtss_appl_auth_authen_method_t *method);
mesa_rc authen_http_set(uint32_t ix, const vtss_appl_auth_authen_method_t *method);
mesa_rc authen_method_itr(const uint32_t *prev, uint32_t *next);
mesa_rc acct_console_get(vtss_appl_auth_acct_agent_conf_t *conf);
mesa_rc acct_console_set(const vtss_appl_auth_acct_agent_conf_t *conf);
mesa_rc acct_telnet_get(vtss_appl_auth_acct_agent_conf_t *conf);
mesa_rc acct_telnet_set(const vtss_appl_auth_acct_agent_conf_t *conf);
mesa_rc acct_ssh_get(vtss_appl_auth_acct_agent_conf_t *conf);
mesa_rc acct_ssh_set(const vtss_appl_auth_acct_agent_conf_t *conf);
mesa_rc author_console_get(vtss_appl_auth_author_agent_conf_t *conf);
mesa_rc author_console_set(const vtss_appl_auth_author_agent_conf_t *conf);
mesa_rc author_telnet_get(vtss_appl_auth_author_agent_conf_t *conf);
mesa_rc author_telnet_set(const vtss_appl_auth_author_agent_conf_t *conf);
mesa_rc author_ssh_get(vtss_appl_auth_author_agent_conf_t *conf);
mesa_rc author_ssh_set(const vtss_appl_auth_author_agent_conf_t *conf);
mesa_rc auth_host_itr(const vtss_auth_host_index_t *prev_host, vtss_auth_host_index_t *next_host);

VTSS_SNMP_TAG_SERIALIZE(AUTH_method_index_1, uint32_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Index"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Method priority index, from 0 to " TO_STR(VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX) " where 0 is the highest priority index"));
};

VTSS_SNMP_TAG_SERIALIZE(AUTH_authen_method, vtss_appl_auth_authen_method_t, a, s) {
    a.add_leaf(s.inner,
               vtss::tag::Name("Method"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Authentication method"));
}

template<typename T>
void serialize(T &a, vtss_appl_auth_author_agent_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_auth_author_agent_conf_t"));
    m.add_leaf(s.method, vtss::tag::Name("Method"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Authorization method"));

    m.add_leaf(vtss::AsBool(s.cmd_enable),
               vtss::tag::Name("CmdEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Enable authorization of commands"));

    m.add_leaf(s.cmd_priv_lvl,
               vtss::tag::Name("CmdPrivLvl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Command privilege level. "
                                      "Authorize all commands with a privilege level higher than or equal to this level. "
                                      "Valid values are in the range 0 to 15"));

    m.add_leaf(vtss::AsBool(s.cfg_cmd_enable),
               vtss::tag::Name("CfgCmdEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Also authorize configuration commands"));
}

template<typename T>
void serialize(T &a, vtss_appl_auth_acct_agent_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_auth_acct_agent_conf_t"));
    m.add_leaf(s.method,
               vtss::tag::Name("Method"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Accounting method"));

    m.add_leaf(vtss::AsBool(s.cmd_enable),
               vtss::tag::Name("CmdEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Enable accounting of commands"));

    m.add_leaf(s.cmd_priv_lvl,
               vtss::tag::Name("CmdPrivLvl"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Command privilege level. "
                                      "Log all commands with a privilege level higher than or equal to this level. "
                                      "Valid values are in the range 0 to 15"));

    m.add_leaf(vtss::AsBool(s.exec_enable),
               vtss::tag::Name("ExecEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Enable exec (login) accounting"));
}

VTSS_SNMP_TAG_SERIALIZE(AUTH_host_index_1, vtss_auth_host_index_t, a, s ) {
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Index"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Host entry index"));
}

template<typename T>
void serialize(T &a, vtss_appl_auth_radius_global_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_auth_radius_global_conf_t"));
    m.add_leaf(s.timeout, vtss::tag::Name("Timeout"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Global timeout for for RADIUS servers. Can be overridden by individual host entries. (1 to 1000 seconds)"));
    m.add_leaf(s.retransmit, vtss::tag::Name("Retransmit"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Global retransmit count for RADIUS servers. Can be overridden by individual host entries. (1 to 1000 times)"));
    m.add_leaf(s.deadtime, vtss::tag::Name("Deadtime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Global deadtime for RADIUS servers. (0 to 1440 minutes)"));
    m.add_leaf(vtss::AsDisplayString(s.key, sizeof(s.key)), vtss::tag::Name("Key"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Global secret key for RADIUS servers. Can be overridden by individual host entries."
                                      " If the secret key is unencrypted, then the maximum length is " TO_STR(VTSS_APPL_AUTH_UNENCRYPTED_KEY_INPUT_LEN) "."));
    m.add_leaf(vtss::AsBool(s.encrypted), vtss::tag::Name("Encrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The flag indicates the secret key is encrypted or not"));

    m.add_leaf(vtss::AsBool(s.nas_ip_address_enable), vtss::tag::Name("NasIpv4Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(11), // <- Reserve a few OIDs for other global parameters
               vtss::tag::Description("Enable Global NAS IPv4 address"));
    m.add_leaf(vtss::AsIpv4(s.nas_ip_address), vtss::tag::Name("NasIpv4Address"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Global NAS IPv4 address"));
    m.add_leaf(vtss::AsBool(s.nas_ipv6_address_enable), vtss::tag::Name("NasIpv6Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("Enable Global NAS IPv6 address"));
    m.add_leaf(s.nas_ipv6_address, vtss::tag::Name("NasIpv6Address"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("Global NAS IPv6 address"));
    m.add_leaf(vtss::AsDisplayString(s.nas_identifier,VTSS_APPL_AUTH_HOST_LEN), vtss::tag::Name("NasIdentifier"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("Global NAS Identifier"));
}

template<typename T>
void serialize(T &a, vtss_appl_auth_radius_server_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_auth_radius_server_conf_t"));
    m.add_leaf(vtss::AsDisplayString(s.host, VTSS_APPL_AUTH_HOST_LEN), vtss::tag::Name("Address"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("IPv4/IPv6 address or hostname of this server"));
    m.add_leaf(s.auth_port, vtss::tag::Name("AuthPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Authentication port number (UDP) for use for this server"));
    m.add_leaf(s.acct_port, vtss::tag::Name("AcctPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Accounting port number (UDP) to use for this server"));
    m.add_leaf(s.timeout, vtss::tag::Name("Timeout"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Seconds to wait for a response from this server. Use global timeout if zero"));
    m.add_leaf(s.retransmit, vtss::tag::Name("Retransmit"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Number of times a request is resent to an unresponding server. Use global retransmit if zero"));
    m.add_leaf(vtss::AsDisplayString(s.key, sizeof(s.key)), vtss::tag::Name("Key"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("The secret key to use for this server. Use global key if empty"
                                      " If the secret key is unencrypted, then the maximum length is " TO_STR(VTSS_APPL_AUTH_UNENCRYPTED_KEY_INPUT_LEN) "."));
    m.add_leaf(vtss::AsBool(s.encrypted), vtss::tag::Name("Encrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("The flag indicates the secret key is encrypted or not"));
}

template<typename T>
void serialize(T &a, vtss_appl_auth_tacacs_global_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_auth_tacacs_global_conf_t"));
    m.add_leaf(s.timeout, vtss::tag::Name("Timeout"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Global timeout for for TACACS servers. Can be overridden by individual host entries. (1 to 1000 seconds)"));
    m.add_leaf(s.deadtime, vtss::tag::Name("Deadtime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Global deadtime for TACACS servers. (0 to 1440 minutes)"));
    m.add_leaf(vtss::AsDisplayString(s.key, sizeof(s.key)), vtss::tag::Name("Key"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Global secret key for TACACS servers. Can be overridden by individual host entries."
                                      " If the secret key is unencrypted, then the maximum length is " TO_STR(VTSS_APPL_AUTH_UNENCRYPTED_KEY_INPUT_LEN) "."));
    m.add_leaf(vtss::AsBool(s.encrypted), vtss::tag::Name("Encrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("The flag indicates the secret key is encrypted or not"));
}

template<typename T>
void serialize(T &a, vtss_appl_auth_tacacs_server_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_auth_tacacs_server_conf_t"));
    m.add_leaf(vtss::AsDisplayString(s.host, VTSS_APPL_AUTH_HOST_LEN), vtss::tag::Name("Address"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("IPv4/IPv6 address or hostname of this server"));
    m.add_leaf(s.port, vtss::tag::Name("AuthPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Authentication port number (TCP) to use for this server"));
    m.add_leaf(s.timeout, vtss::tag::Name("Timeout"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Seconds to wait for a response from this server. Use global timeout if zero"));
    m.add_leaf(vtss::AsDisplayString(s.key, sizeof(s.key)), vtss::tag::Name("Key"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The secret key to use for this server. Use global key if empty."
                                      " If the secret key is unencrypted, then the maximum length is " TO_STR(VTSS_APPL_AUTH_UNENCRYPTED_KEY_INPUT_LEN) "."));
    m.add_leaf(vtss::AsBool(s.encrypted), vtss::tag::Name("Encrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("The flag indicates the secret key is encrypted or not"));
}

namespace vtss {
namespace appl {
namespace auth {
namespace interfaces {
struct AuthAgentAuthenConsole {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<uint32_t>,
        vtss::expose::ParamVal<vtss_appl_auth_authen_method_t *>
    > P;

    static constexpr const char *table_description =
        "This is an ordered table of methods used to authenticate console access";

    static constexpr const char *index_description =
        "Each entry defines a method to be consulted with a priorty equal to the index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i) {
        serialize(h, AUTH_method_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_auth_authen_method_t &i) {
        serialize(h, AUTH_authen_method(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(authen_console_get);
    VTSS_EXPOSE_ITR_PTR(authen_method_itr);
    VTSS_EXPOSE_SET_PTR(authen_console_set);
};

struct AuthAgentAuthenTelnet {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<uint32_t>,
        vtss::expose::ParamVal<vtss_appl_auth_authen_method_t *>
    > P;

    static constexpr const char *table_description =
        "This is an ordered table of methods used to authenticate telnet access";

    static constexpr const char *index_description =
        "Each entry defines a method to be consulted with a priorty equal to the index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i) {
        serialize(h, AUTH_method_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_auth_authen_method_t &i) {
        serialize(h, AUTH_authen_method(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(authen_telnet_get);
    VTSS_EXPOSE_ITR_PTR(authen_method_itr);
    VTSS_EXPOSE_SET_PTR(authen_telnet_set);
};

struct AuthAgentAuthenSsh {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<uint32_t>,
        vtss::expose::ParamVal<vtss_appl_auth_authen_method_t *>
    > P;

    static constexpr const char *table_description =
        "This is an ordered table of methods used to authenticate ssh access";

    static constexpr const char *index_description =
        "Each entry defines a method to be consulted with a priorty equal to the index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i) {
        serialize(h, AUTH_method_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_auth_authen_method_t &i) {
        serialize(h, AUTH_authen_method(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(authen_ssh_get);
    VTSS_EXPOSE_ITR_PTR(authen_method_itr);
    VTSS_EXPOSE_SET_PTR(authen_ssh_set);
};

struct AuthAgentAuthenHttp {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<uint32_t>,
        vtss::expose::ParamVal<vtss_appl_auth_authen_method_t *>
    > P;

    static constexpr const char *table_description =
        "This is an ordered table of methods used to authenticate HTTP access";

    static constexpr const char *index_description =
        "Each entry defines a method to be consulted with a priorty equal to the index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i) {
        serialize(h, AUTH_method_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_auth_authen_method_t &i) {
        serialize(h, AUTH_authen_method(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(authen_http_get);
    VTSS_EXPOSE_ITR_PTR(authen_method_itr);
    VTSS_EXPOSE_SET_PTR(authen_http_set);
};


struct AuthAgentAuthorConsole {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_author_agent_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_author_agent_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(author_console_get);
    VTSS_EXPOSE_SET_PTR(author_console_set);
};

struct AuthAgentAuthorTelnet {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_author_agent_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_author_agent_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(author_telnet_get);
    VTSS_EXPOSE_SET_PTR(author_telnet_set);
};

struct AuthAgentAuthorSsh {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_author_agent_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_author_agent_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(author_ssh_get);
    VTSS_EXPOSE_SET_PTR(author_ssh_set);
};

struct AuthAgentAcctConsole {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_acct_agent_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_acct_agent_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(acct_console_get);
    VTSS_EXPOSE_SET_PTR(acct_console_set);
};

struct AuthAgentAcctTelnet {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_acct_agent_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_acct_agent_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(acct_telnet_get);
    VTSS_EXPOSE_SET_PTR(acct_telnet_set);
};

struct AuthAgentAcctSsh {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_acct_agent_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_acct_agent_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(acct_ssh_get);
    VTSS_EXPOSE_SET_PTR(acct_ssh_set);
};

struct AuthRadiusGlobal {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_radius_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_radius_global_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_auth_radius_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_auth_radius_global_conf_set);
};

struct AuthAuthRadiusHosts {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_auth_host_index_t>,
        vtss::expose::ParamVal<vtss_appl_auth_radius_server_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of Radius servers useed to query for RADIUS authentication";

    static constexpr const char *index_description =
        "Each entry defines a RADIUS server, with attributes used for contacting it. Host entries are consulted in numerical order of the entry index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_auth_host_index_t &i) {
        serialize(h, AUTH_host_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_auth_radius_server_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_auth_radius_server_get);
    VTSS_EXPOSE_ITR_PTR(auth_host_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_auth_radius_server_set);
};

struct AuthTacacsGlobal {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_auth_tacacs_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_auth_tacacs_global_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_auth_tacacs_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_auth_tacacs_global_conf_set);
};

struct AuthAuthTacacsHosts {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_auth_host_index_t>,
        vtss::expose::ParamVal<vtss_appl_auth_tacacs_server_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of Tacacs servers useed to query for TACACS authentication";

    static constexpr const char *index_description =
        "Each entry defines a TACACS server, with attributes used for contacting it. Host entries are consulted in numerical order of the entry index";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_auth_host_index_t &i) {
        serialize(h, AUTH_host_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_auth_tacacs_server_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
    VTSS_EXPOSE_GET_PTR(vtss_appl_auth_tacacs_server_get);
    VTSS_EXPOSE_ITR_PTR(auth_host_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_auth_tacacs_server_set);
};
}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

#endif
