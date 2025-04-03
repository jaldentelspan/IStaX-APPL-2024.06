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
#ifndef _GVRP_SERIALIZER_HXX_
#define _GVRP_SERIALIZER_HXX_

#include "vtss/appl/module_id.h"
#include "vtss/appl/gvrp.h"
#include "vtss_appl_serialize.hxx"

VTSS_SNMP_TAG_SERIALIZE(gvrp_ifindex_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The index of logical interface.")
    );
}

template <typename T>
void serialize(T &a, vtss_appl_gvrp_config_globals_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_gvrp_config_globals_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(p.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global enabling of GVRP protocol. "
                               "TRUE - enable GVRP, FALSE - disable GVRP.")
    );


    m.add_leaf(
        vtss::AsInt(p.join_time),
        vtss::tag::Name("JoinTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::expose::snmp::RangeSpec<u32>(1, 20),
        vtss::tag::Description("Join-time protocol parameter. Range [1,20] centi seconds.")
    );

    m.add_leaf(
        vtss::AsInt(p.leave_time),
        vtss::tag::Name("LeaveTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::expose::snmp::RangeSpec<u32>(60, 300),
        vtss::tag::Description("Leave-time protocol parameter. Range [60,300] centi seconds.")
    );

    m.add_leaf(
        vtss::AsInt(p.leave_all_time),
        vtss::tag::Name("LeaveAllTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::expose::snmp::RangeSpec<u32>(1000, 5000),
        vtss::tag::Description("Leave-all-time protocol parameter. Range [1000,5000] centi seconds.")
    );

    m.add_leaf(
        vtss::AsInt(p.max_vlans),
        vtss::tag::Name("MaxVlans"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::expose::snmp::RangeSpec<u32>(1, 4094),
        vtss::tag::Description("Maximum number of VLANs simultaneously supported "
                               "by GVRP. Range is [1,4094]. GVRP must be disabled in "
                               "order to change this parameter."));
}

template<typename T>
void serialize(T &a, vtss_appl_gvrp_config_interface_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_gvrp_config_interface_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(p.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Per-port mode of GVRP. "
                               "TRUE - enable GVRP on the port, "
                               "FALSE - disable GVRP on the port.")
    );
}

namespace vtss
{
namespace gvrp
{

struct GvrpConfigGlobalsLeaf {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_gvrp_config_globals_t *>>  P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_gvrp_config_globals_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_gvrp_config_globals_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_gvrp_config_globals_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_XXRP);
};

struct GvrpConfigInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_gvrp_config_interface_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of interface configuration";

    static constexpr const char *index_description =
        "Each interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, gvrp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_gvrp_config_interface_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_gvrp_config_interface_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_gvrp_config_interface_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_gvrp_config_interface_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_XXRP);
};

} // gvrp
} // vtss


#endif // _GVRP_SERIALIZER_HXX_
