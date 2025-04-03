/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __IPMC_LIB_PROFILE_SERIALIZER_HXX__
#define __IPMC_LIB_PROFILE_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include <vtss/appl/ipmc_lib.h>
#include "ipmc_lib_profile_expose.hxx"

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(ipmc_profile_name_type, vtss_appl_ipmc_lib_profile_key_t, a, s)
{
    a.add_leaf(
        vtss::AsDisplayString(s.inner.name, sizeof(s.inner.name)),
        vtss::tag::Name("ProfileName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The name of the IPMC profile management entry.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_range_name_type, vtss_appl_ipmc_lib_profile_range_key_t, a, s)
{
    a.add_leaf(
        vtss::AsDisplayString(s.inner.name, sizeof(s.inner.name)),
        vtss::tag::Name("RangeName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The name of the IPMC profile address range entry.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(ipmc_rule_name_type, vtss_appl_ipmc_lib_profile_range_key_t, a, s)
{
    a.add_leaf(
        vtss::AsDisplayString(s.inner.name, sizeof(s.inner.name)),
        vtss::tag::Name("RuleRange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The name of the IPMC profile address range used as a rule.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_profile_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_profile_global_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.enable),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable/Disable IPMC Profile functionality.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ipmc_lib_profile_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_profile_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.dscr, sizeof(s.dscr)),
        vtss::tag::Name("ProfileDescription"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The description of the IPMC Profile management entry.")
    );
}

template<typename T>
void serialize(T &a, ipmc_lib_expose_profile_range_conf_ipv4_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_profile_range_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsIpv4(s.conf.start.ipv4),
        vtss::tag::Name("StartAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The starting IPv4 multicast address of the "
                               "range that IPMC Profile performs checking.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.conf.end.ipv4),
        vtss::tag::Name("EndAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ending IPv4 multicast address of the range "
                               "that IPMC Profile performs checking.")
    );
}

#ifdef VTSS_SW_OPTION_IPV6
template<typename T>
void serialize(T &a, ipmc_lib_expose_profile_range_conf_ipv6_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_profile_range_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.conf.start.ipv6,
        vtss::tag::Name("StartAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The starting IPv6 multicast address of the "
                               "range that IPMC Profile performs checking.")
    );

    m.add_leaf(
        s.conf.end.ipv6,
        vtss::tag::Name("EndAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ending IPv6 multicast address of the range "
                               "that IPMC Profile performs checking.")
    );
}
#endif /* VTSS_SW_OPTION_IPV6 */

template<typename T>
void serialize(T &a, ipmc_lib_expose_profile_rule_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ipmc_lib_profile_rule_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.next_range.name, sizeof(s.next_range.name)),
        vtss::tag::Name("NextRuleRange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The next rule's address range name "
                               "that this IPMC Profile management entry performs checking.")
    );

    m.add_leaf(
        vtss::AsBool(s.conf.deny),
        vtss::tag::Name("Deny"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The filtering action while this IPMC Profile management "
                               "entry performs checking. "
                               "Set to true to prohibit the IPMC control frames destined to protocol stack. "
                               "Set to false to pass the IPMC control frames destined to protocol stack.")
    );

    m.add_leaf(
        vtss::AsBool(s.conf.log),
        vtss::tag::Name("Log"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Enable the IPMC Profile will log matched group address "
                               "that is filtered by this rule with the corresponding action (deny or permit). "
                               "Disable the IPMC Profile will not log any action for any group address "
                               "whether or not to be filtered by this rule.")
    );
}

namespace vtss
{
namespace appl
{
namespace ipmc_lib_profile
{
namespace interfaces
{
struct IpmcProfileGlobalsConfig {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ipmc_lib_profile_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipmc_lib_profile_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ipmc_lib_profile_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ipmc_lib_profile_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcProfileManagementTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ipmc_lib_profile_key_t *>,
         vtss::expose::ParamVal<vtss_appl_ipmc_lib_profile_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing IPMC profile entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipmc_lib_profile_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_profile_name_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_profile_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ipmc_lib_profile_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ipmc_lib_profile_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ipmc_lib_profile_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ipmc_lib_profile_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ipmc_lib_profile_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcProfileIpv4RangeTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ipmc_lib_profile_range_key_t *>,
         vtss::expose::ParamVal<ipmc_lib_expose_profile_range_conf_ipv4_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing the IPv4 multicast address range entries that will be applied for IPMC profile(s).";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipmc_lib_profile_range_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_range_name_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(ipmc_lib_expose_profile_range_conf_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_lib_expose_ipmc_profile_range_ipv4_conf_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_lib_expose_ipmc_profile_range_ipv4_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_lib_expose_ipmc_profile_range_ipv4_conf_set);
    VTSS_EXPOSE_ADD_PTR(ipmc_lib_expose_ipmc_profile_range_ipv4_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ipmc_lib_profile_range_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcProfileIpv6RangeTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ipmc_lib_profile_range_key_t *>,
         vtss::expose::ParamVal<ipmc_lib_expose_profile_range_conf_ipv6_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing the IPv6 multicast address range entries that will be applied for IPMC profile(s).";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipmc_lib_profile_range_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_range_name_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(ipmc_lib_expose_profile_range_conf_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_lib_expose_ipmc_profile_range_ipv6_conf_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_lib_expose_ipmc_profile_range_ipv6_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_lib_expose_ipmc_profile_range_ipv6_conf_set);
    VTSS_EXPOSE_ADD_PTR(ipmc_lib_expose_ipmc_profile_range_ipv6_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ipmc_lib_profile_range_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

struct IpmcProfileRuleTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ipmc_lib_profile_key_t *>,
         vtss::expose::ParamKey<vtss_appl_ipmc_lib_profile_range_key_t *>,
         vtss::expose::ParamVal<ipmc_lib_expose_profile_rule_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing the filtering rules with respect to a set of address range used in a specific IPMC profile management entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ipmc_lib_profile_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ipmc_profile_name_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ipmc_lib_profile_range_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, ipmc_rule_name_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(ipmc_lib_expose_profile_rule_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(ipmc_lib_expose_profile_rule_conf_get);
    VTSS_EXPOSE_ITR_PTR(ipmc_lib_expose_profile_rule_itr);
    VTSS_EXPOSE_SET_PTR(ipmc_lib_expose_profile_rule_conf_set);
    VTSS_EXPOSE_ADD_PTR(ipmc_lib_expose_profile_rule_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ipmc_lib_profile_rule_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC);
};

}  // namespace interfaces
}  // namespace ipmc_lib_profile
}  // namespace appl
}  // namespace vtss

#endif /* __IPMC_LIB_PROFILE_SERIALIZER_HXX__ */

