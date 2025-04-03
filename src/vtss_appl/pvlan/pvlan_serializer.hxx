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
#ifndef __VTSS_PVLAN_SERIALIZER_HXX__
#define __VTSS_PVLAN_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/pvlan.h"
#include "vtss/appl/types.hxx"

/*****************************************************************************
    Data type serializer
*****************************************************************************/

/*****************************************************************************
    Enumerator serializer
*****************************************************************************/

/*****************************************************************************
    Index serializer
*****************************************************************************/

VTSS_SNMP_TAG_SERIALIZE(pvlan_isolation_idx, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("PortIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description(
            "Logical interface number of the Private VLAN port isolation.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_pvlan_capabilities_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_pvlan_capabilities_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.support_pvlan_membership_mgmt),
        vtss::tag::Name("HasVlanMembershipMgmt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support PVLAN membership configuration by the device.")
    );

    m.add_leaf(
        s.max_pvlan_membership_vlan_id,
        vtss::tag::Name("VlanIdMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum VLAN ID of PVLAN membership configuration supported by the device.")
    );

    m.add_leaf(
        s.min_pvlan_membership_vlan_id,
        vtss::tag::Name("VlanIdMin"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum VLAN ID of PVLAN membership configuration supported by the device.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_pvlan_membership_conf_t &s)
{
    typename T::Map_t       m = a.as_map(vtss::tag::Typename("vtss_appl_pvlan_membership_conf_t"));
    int                     ix = 0;
    vtss::PortListStackable &list = (vtss::PortListStackable &)s.member_ports;

    m.add_leaf(
        list,
        vtss::tag::Name("PortList"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("It is used to denote the memberships of the specific "
                               "Private VLAN configuration.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_pvlan_isolation_conf_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_pvlan_isolation_conf_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.isolated),
        vtss::tag::Name("Enabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Enable/Disable the Private VLAN isolation functionality.")
    );
}

/*****************************************************************************
    Serializer interface
*****************************************************************************/
namespace vtss
{
namespace appl
{
namespace pvlan
{
namespace interfaces
{

struct PvlanCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_pvlan_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_pvlan_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_pvlan_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PVLAN);
};

struct PvlanVlanMembershipTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32 *>,
         vtss::expose::ParamVal<vtss_appl_pvlan_membership_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing Private VLAN VLAN membership entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        h.add_leaf(i,
                   vtss::tag::Name("PvlanIndex"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(1),
                   vtss::tag::Description(
                       "Configuration index of the Private VLAN membership table.")
                  );
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_pvlan_membership_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_pvlan_membership_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_pvlan_membership_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_pvlan_membership_config_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_pvlan_membership_config_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_pvlan_membership_config_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PVLAN);
};

struct PvlanPortIsolationTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_pvlan_isolation_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing Private VLAN port isolation entries.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, pvlan_isolation_idx(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_pvlan_isolation_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_pvlan_isolation_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_pvlan_isolation_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_pvlan_isolation_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PVLAN);
};

}  // namespace interfaces
}  // namespace pvlan
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_PVLAN_SERIALIZER_HXX__ */
