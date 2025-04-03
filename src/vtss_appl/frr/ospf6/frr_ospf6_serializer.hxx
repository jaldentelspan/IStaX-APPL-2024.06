/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _FRR_OSPF6_SERIALIZER_HXX_
#define _FRR_OSPF6_SERIALIZER_HXX_
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_OSPF6
#include "frr_trace.hxx"  // For module trace group definitions

/**
 * \file frr_ospf6_serializer.hxx
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
#include "vtss_appl_serialize.hxx"
#include "frr_ospf6_api.hxx"  // For frr_ospf6_control_globals_dummy_get()
#include "frr_ospf6_expose.hxx"
#include "vtss/appl/ospf6.h"
#include "vtss/appl/types.hxx"

/**
 * \brief Get status for all neighbor information.
 * \param req [IN]  Json request.
 * \param os  [OUT] Json output string.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

/**
 * \brief Get status for all interface information.
 * \param req [IN]  Json request.
 * \param os  [OUT] Json output string.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_interface_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_ospf6_route_ipv6_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_ospf6_db_get_all_json(const vtss::expose::json::Request *req,
                                        vtss::ostreamBuf *os);

mesa_rc vtss_appl_ospf6_db_link_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_intra_area_prefix_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_router_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_network_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_inter_area_prefix_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_inter_area_router_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_external_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf6_db_nssa_external_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

/******************************************************************************/
/** enum serializer (enum value-string mapping)                               */
/******************************************************************************/
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf6_interface_state_t, "Ospf6InterfaceState",
                         vtss_appl_ospf6_interface_state_txt,
                         "The state of the link.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf6_neighbor_state_t, "Ospf6NeighborState",
                         vtss_appl_ospf6_neighbor_state_txt,
                         "The state of the neighbor node.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf6_area_type_t, "Ospf6AreaType",
                         vtss_appl_ospf6_area_type_txt, "The area type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf6_route_type_t, "Ospf6RouteType",
                         vtss_appl_ospf6_route_type_txt, "The OSPF6 route type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf6_route_br_type_t, "Ospf6BorderRouterType",
                         vtss_appl_ospf6_route_border_router_type_txt,
                         "The border router type of the OSPF6 route entry.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf6_lsdb_type_t, "Ospf6LsdbType",
                         vtss_appl_ospf6_lsdb_type_txt,
                         "The type of the link state advertisement.");

/******************************************************************************/
/** Table index/key serializer                                                */
/******************************************************************************/

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_instance_id, vtss_appl_ospf6_id_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("InstanceId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(
                   VTSS_APPL_OSPF6_INSTANCE_ID_START,
                   VTSS_APPL_OSPF6_INSTANCE_ID_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF6 process instance ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_area_id, vtss_appl_ospf6_area_id_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF6 area ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_db_key_area_id, vtss_appl_ospf6_area_id_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The OSPF6 area ID of the link state advertisement. For "
                   "type 5 and type 7, the value is always 255.255.255.255 "
                   "since it is not required for these two types."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_val_area_id, vtss_appl_ospf6_area_id_t, a, s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("ospf6_val_area_id"));

    m.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF6 area ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_router_id, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF6 router ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_ipv6_network, mesa_ipv6_network_t, a, s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("mesa_ipv6_network_t"));
    m.add_leaf((s.inner.address), vtss::tag::Name("Network"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("IPv6 network address."));

    m.add_leaf(vtss::AsInt(s.inner.prefix_size),
               vtss::tag::Name("IpSubnetMaskLength"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IPv6 network mask length."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_intf_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_ipv6_addr, mesa_ipv6_t, a, s)
{
    a.add_leaf((s.inner), vtss::tag::Name("Ipv6Addr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The IPv6 address."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_ipv6_nexthop, mesa_ipv6_t, a, s)
{
    a.add_leaf((s.inner), vtss::tag::Name("NextHop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The nexthop to the route."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_md_key_id, vtss_appl_ospf6_md_key_id_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("MdKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The key ID for message digest authentication."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_md_key_precedence, uint32_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("Precedence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The precedence of message digest key."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_val_md_key_id, vtss_appl_ospf6_md_key_id_t, a, s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_md_key_id_t"));
    m.add_leaf(s.inner, vtss::tag::Name("MdKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The key ID for message digest authentication."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_route_type, vtss_appl_ospf6_route_type_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("RouteType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF6 route type."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_route_area, vtss_appl_ospf6_area_id_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0), vtss::tag::Description("It indicates which area the route or router can be reached "
                                                                              "via/to."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_db_lsdb_type, vtss_appl_ospf6_lsdb_type_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("LsdbType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The type of the link state advertisement."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_db_link_state_id, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("LinkStateId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF6 link state ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf6_key_db_adv_router_id, vtss_appl_ospf6_router_id_t,
                        a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AdvRouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The advertising router ID which originated the LSA."));
}

/******************************************************************************/
/** Table entry serializer                                                    */
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_ospf6_capabilities_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_capabilities_t"));

    int ix = 0;

    // OSPF6 valid range: instance ID
    m.add_leaf(p.instance_id_min, vtss::tag::Name("MinInstanceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OSPF6 minimum instance ID"));
    m.add_leaf(p.instance_id_max, vtss::tag::Name("MaxInstanceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OSPF6 maximum instance ID"));

    // OSPF6 valid range: router ID
    m.add_leaf(p.router_id_min, vtss::tag::Name("MinRouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of OSPF6 router ID"));
    m.add_leaf(p.router_id_max, vtss::tag::Name("MaxRouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of OSPF6 router ID"));

    // OSPF6 valid range: priority
    m.add_leaf(p.priority_min, vtss::tag::Name("MinPriority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of OSPF6 priority"));
    m.add_leaf(p.priority_max, vtss::tag::Name("MaxPriority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of OSPF6 priority"));

    // OSPF6 valid range: general cost
    m.add_leaf(
        p.general_cost_min, vtss::tag::Name("MinGeneralCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF6 interface cost"));
    m.add_leaf(
        p.general_cost_max, vtss::tag::Name("MaxGeneralCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF6 interface cost"));

    // OSPF6 valid range: interface cost
    m.add_leaf(
        p.intf_cost_min, vtss::tag::Name("MinInterfaceCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF6 interface cost"));
    m.add_leaf(
        p.intf_cost_max, vtss::tag::Name("MaxInterfaceCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF6 interface cost"));

    // OSPF6 valid rangeL hello interval
    m.add_leaf(
        p.hello_interval_min, vtss::tag::Name("MinHelloInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF6 hello interval"));
    m.add_leaf(
        p.hello_interval_max, vtss::tag::Name("MaxHelloInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF6 hello interval"));

    // OSPF6 valid range :retransmit interval
    m.add_leaf(p.retransmit_interval_min,
               vtss::tag::Name("MinRetransmitInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum value of OSPF6 retransmit interval"));
    m.add_leaf(p.retransmit_interval_max,
               vtss::tag::Name("MaxRetransmitInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum value of OSPF6 retransmit interval"));

    // OSPF6 valid range: dead interval
    m.add_leaf(
        p.dead_interval_min, vtss::tag::Name("MinDeadInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF6 dead interval"));
    m.add_leaf(
        p.dead_interval_max, vtss::tag::Name("MaxDeadInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF6 dead interval"));

    m.add_leaf(vtss::AsBool(p.ripng_redistributed_supported),
               vtss::tag::Name("IsRipngRedistributedSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate if RIPNG redistributed is supported or not"));

    // OSPF6 valid range: administrative distance
    m.add_leaf(
        p.admin_distance_min, vtss::tag::Name("MinAdminDistance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum value of OSPF6 administrative distance value"));
    m.add_leaf(
        p.admin_distance_max, vtss::tag::Name("MaxAdminDistance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of OSPF6 administrative distance value"));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_router_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_router_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.router_id.is_specific_id),
               vtss::tag::Name("IsSpecificRouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicate the 'RouterId' argument is a "
                                      "specific configured value or not."));

    m.add_leaf(vtss::AsIpv4(p.router_id.id), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 router ID"));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF6_REDIST_PROTOCOL_CONNECTED].is_redist_enable,
               vtss::tag::Name("ConnectedRedistEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF redistribute enabled for"
                                      " connected route or not."));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF6_REDIST_PROTOCOL_STATIC].is_redist_enable,
               vtss::tag::Name("StaticRedistEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF redistributeenabled for "
                                      "the static routes or not."));

    m.add_leaf(p.admin_distance, vtss::tag::Name("AdminDistance"),
               vtss::expose::snmp::RangeSpec<uint8_t>(VTSS_APPL_OSPF6_ADMIN_DISTANCE_MIN, VTSS_APPL_OSPF6_ADMIN_DISTANCE_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 administrative distance."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_router_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_router_status_t"));

    int ix = 0;

    m.add_leaf(vtss::AsIpv4(p.ospf6_router_id), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OSPF6 router ID"));

    m.add_leaf(p.spf_delay, vtss::tag::Name("SpfDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Delay time (in seconds)of SPF calculations."));

    m.add_leaf(p.spf_holdtime, vtss::tag::Name("SpfHoldTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum hold time (in milliseconds) "
                                      "between consecutive SPF calculations."));

    m.add_leaf(p.spf_max_waittime, vtss::tag::Name("SpfMaxWaitTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum wait time (in milliseconds) "
                                      "between consecutive SPF calculations."));

    m.add_leaf(vtss::AsCounter(p.last_executed_spf_ts),
               vtss::tag::Name("LastExcutedSpfTs"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Time (in milliseconds) that has passed "
                                      "between the start of the SPF algorithm "
                                      "execution and the current time."));

    m.add_leaf(
        p.attached_area_count, vtss::tag::Name("AttachedAreaCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of areas attached for the router."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_router_intf_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_router_intf_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.area_id.is_specific_id),
               vtss::tag::Name("IsSpecificAreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the AreaId is valid "
                                      "or not."));

    m.add_leaf(vtss::AsIpv4(p.area_id.id), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 interface Area ID."
                                      "Only valid if 'is_specific_id' is true"));

}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_intf_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_intf_conf_t"));

    int ix = 0;

    m.add_leaf(p.priority, vtss::tag::Name("Priority"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_priority_t>(
                   VTSS_APPL_OSPF6_PRIORITY_MIN, VTSS_APPL_OSPF6_PRIORITY_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified router priority for the interface."));

    m.add_leaf(vtss::AsBool(p.is_specific_cost),
               vtss::tag::Name("IsSpecificCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate the 'cost' argument is a specific configured "
                   "value or not."));

    m.add_leaf(
        p.cost, vtss::tag::Name("Cost"),
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_cost_t>(
            VTSS_APPL_OSPF6_INTF_COST_MIN, VTSS_APPL_OSPF6_INTF_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified cost for this interface. "
            "It's link state metric for the interface. "
            "The field is significant only when 'IsSpecificCost' is "
            "TRUE."));

    m.add_leaf(
        p.dead_interval, vtss::tag::Name("DeadInterval"),
        vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_OSPF6_DEAD_INTERVAL_MIN,
                                           VTSS_APPL_OSPF6_DEAD_INTERVAL_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The time interval (in seconds) between hello packets."));

    m.add_leaf(p.hello_interval, vtss::tag::Name("HelloInterval"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF6_HELLO_INTERVAL_MIN,
                   VTSS_APPL_OSPF6_HELLO_INTERVAL_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "How many Hello packets will be sent per second."));

    m.add_leaf(p.retransmit_interval, vtss::tag::Name("RetransmitInterval"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MIN,
                   VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The time interval (in seconds) between "
                                      "link-state advertisement(LSA) "
                                      "retransmissions for adjacencies."));

    m.add_leaf(vtss::AsBool(p.is_passive),
               vtss::tag::Name("IsPassiveInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether the interface is passive or not"));

    m.add_leaf(p.transmit_delay, vtss::tag::Name("TransmitDelay"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF6_TRANSMIT_DELAY_MIN,
                   VTSS_APPL_OSPF6_TRANSMIT_DELAY_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The time (in seconds) "
                                      "taken to transmit a packet."));

    m.add_leaf(vtss::AsBool(p.mtu_ignore),
               vtss::tag::Name("MtuIgnore"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("When false (default), the interface MTU of received OSPFv3 "
                                      "database description (DBD) packets will be checked against our "
                                      "own MTU, and if the remote MTU differs from our own, the DBD "
                                      "will be ignored. When true, the interface MTU of the received OSPFv3 "
                                      "DBD packets will be ignored."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_area_range_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_area_range_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.is_advertised), vtss::tag::Name("Advertised"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "When the value is true, it summarizes intra area paths "
                   "from the address range in one Inter-Area Prefix LSA(Type-0x2003) and "
                   "advertised to other areas.\n"
                   "Otherwise, the intra area paths from the address range "
                   "are not advertised to other areas."));

    m.add_leaf(vtss::AsBool(p.is_specific_cost),
               vtss::tag::Name("IsSpecificCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate the 'cost' argument is a specific configured "
                   "value or not."));

    m.add_leaf(
        p.cost, vtss::tag::Name("Cost"),
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_cost_t>(
            VTSS_APPL_OSPF6_GENERAL_COST_MIN,
            VTSS_APPL_OSPF6_GENERAL_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified cost (or metric) for this summary route. "
            "The field is significant only when 'IsSpecificCost' is "
            "TRUE."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_stub_area_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_stub_area_conf_t"));

    int ix = 0;

    m.add_leaf(
        vtss::AsBool(p.no_summary), vtss::tag::Name("NoSummary"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The value is true to configure the inter-area routes do "
            "not inject into this stub area."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_area_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_area_status_t"));

    int ix = 0;

    m.add_leaf(
        vtss::AsBool(p.is_backbone), vtss::tag::Name("IsBackbone"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if it's backbone area or not."));

    m.add_leaf(p.area_type, vtss::tag::Name("AreaType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The area type."));

    m.add_leaf(p.attached_intf_total_count,
               vtss::tag::Name("AttachedIntfCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Number of active interfaces attached in the area."));

    m.add_leaf(p.spf_executed_count, vtss::tag::Name("SpfExecutedCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of times SPF algorithm has been "
                                      "executed for the particular area."));

    m.add_leaf(p.lsa_count, vtss::tag::Name("LsaCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of the total LSAs for the "
                                      "particular area."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_interface_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_interface_status_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.status), vtss::tag::Name("Status"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("It's used to indicate if the interface "
                                      "is up or down."));

    m.add_leaf(vtss::AsBool(p.is_passive), vtss::tag::Name("Passive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicate if the interface "
                                      "is passive interface."));

    m.add_leaf((p.network.address), vtss::tag::Name("Network"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv6 network address."));

    m.add_leaf(vtss::AsInt(p.network.prefix_size),
               vtss::tag::Name("IpSubnetMaskLength"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv6 network mask length."));

    m.add_leaf(vtss::AsIpv4(p.area_id), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 area ID."));

    m.add_leaf(vtss::AsIpv4(p.router_id), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 router ID."));

    m.add_leaf(vtss::AsInt(p.cost), vtss::tag::Name("Cost"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_cost_t>(
                   VTSS_APPL_OSPF6_INTF_COST_MIN,
                   VTSS_APPL_OSPF6_INTF_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The cost of the interface."));

    m.add_leaf(p.state, vtss::tag::Name("State"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The state of the link."));

    m.add_leaf(vtss::AsInt(p.priority), vtss::tag::Name("Priority"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_priority_t>(
                   VTSS_APPL_OSPF6_PRIORITY_MIN, VTSS_APPL_OSPF6_PRIORITY_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 priority. It helps determine "
                                      "the DR and BDR on the network to which "
                                      "this interface is connected."));

    m.add_leaf(vtss::AsIpv4(p.dr_id), vtss::tag::Name("DrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of DR."));

    m.add_leaf(vtss::AsIpv4(p.bdr_id), vtss::tag::Name("BdrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of BDR."));

    m.add_leaf(p.hello_time, vtss::tag::Name("HelloTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Hello timer. A time interval that a "
                                      "router sends an OSPF6 hello packet."));

    m.add_leaf(
        p.dead_time, vtss::tag::Name("DeadTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Dead timer. Dead timer is a time interval "
                               "to wait before declaring a neighbor dead. "
                               "The unit of time is the second."));

    m.add_leaf(p.retransmit_time, vtss::tag::Name("RetransmitTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Retransmit timer. A time interval to "
                                      "wait before retransmitting a database "
                                      "description packet when it has not been "
                                      "acknowledged."));


    m.add_leaf(p.transmit_delay, vtss::tag::Name("TransmitDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The estimated time to transmit a "
                                      "link-state update packet on the "
                                      "interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_neighbor_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_neighbor_status_t"));

    int ix = 0;

    m.add_leaf((p.ip_addr), vtss::tag::Name("IpAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The IP address."));

    m.add_leaf(vtss::AsIpv4(p.area_id), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 area ID."));

    m.add_leaf(vtss::AsIpv4(p.neighbor_id), vtss::tag::Name("NeighborId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of Neighbor."));

    m.add_leaf(vtss::AsInt(p.priority), vtss::tag::Name("Priority"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_priority_t>(
                   VTSS_APPL_OSPF6_PRIORITY_MIN, VTSS_APPL_OSPF6_PRIORITY_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The priority of OSPF6 neighbor. It indicates the "
                   "priority of the neighbor router. This item is used "
                   "when selecting the DR for the network. The router with "
                   "the highest priority becomes the DR."));

    // vtss_appl_ospf6_neighbor_state_t
    m.add_leaf(p.state, vtss::tag::Name("State"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The state of OSPF6 neighbor. It "
                                      "indicates the functional state of the "
                                      "neighbor router."));

    m.add_leaf(vtss::AsIpv4(p.dr_id), vtss::tag::Name("DrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of DR."));

    m.add_leaf(vtss::AsIpv4(p.bdr_id), vtss::tag::Name("BdrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of BDR."));

    m.add_leaf(p.dead_time, vtss::tag::Name("DeadTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Dead timer. It indicates the amount of time "
                   "remaining that the router waits to receive "
                   "an OSPF6 hello packet from the neighbor "
                   "before declaring the neighbor down."));

    m.add_leaf(vtss::AsIpv4(p.transit_id), vtss::tag::Name("TransitAreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF6 transit area ID for the "
                                      "neighbor on virtual link interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_route_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_route_status_t"));

    int ix = 0;

    m.add_leaf(p.cost, vtss::tag::Name("Cost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The cost of the route."));

    m.add_leaf(
        p.as_cost, vtss::tag::Name("AsCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The cost of the route within the OSPF6 network. It is "
            "valid "
            "for external Type-2 route and always '0' for other route "
            "type."));

    m.add_leaf(p.border_router_type, vtss::tag::Name("BorderRouterType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The border router type of the OSPF6 "
                                      "route entry."));

    m.add_leaf(vtss::AsInterfaceIndex(p.ifindex), vtss::tag::Name("Ifindex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The interface where the ip packet is outgoing."));

    m.add_leaf(vtss::AsBool(p.connected), vtss::tag::Name("IsConnected"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The destination is connected directly or not."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_general_info_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_db_general_info_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.router_link_count, vtss::tag::Name("RouterLinkCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The link count of the LSA. The field is "
                                      "significant only when the link state "
                                      "type is 'Router Link State' (Type 1)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_control_globals_t &s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf6_control_globals_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.reload_process), vtss::tag::Name("ReloadProcess"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Set true to reload OSPF6 process."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_link_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_link_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF6 option field which is present in "
                   "OSPF6 hello packets, which enables OSPF6 routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF6 "
                   "routers."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf(p.link_local_address, vtss::tag::Name("linkLocalAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Link Local Address of the LSA."));

    m.add_leaf(p.prefix_cnt, vtss::tag::Name("PrefixCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The count of the LSA."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_intra_area_prefix_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_intra_area_prefix_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf(p.prefix_cnt, vtss::tag::Name("PrefixCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The count of the Prefixes."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_router_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_router_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF6 option field which is present in "
                   "OSPF6 hello packets, which enables OSPF6 routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF6 "
                   "routers."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf(p.router_link_count, vtss::tag::Name("RouterLinkCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The link count of the LSA. The field is "
                                      "significant only when the link state "
                                      "type is 'Router Link State' (Type 1)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_network_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_network_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF6 option field which is present in "
                   "OSPF6 hello packets, which enables OSPF6 routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF6 "
                   "routers."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf(p.attached_router_count, vtss::tag::Name("AttachedRouterCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The attached router count of the LSA. The field is "
                   "significant only when the link state "
                   "type is 'Network Link State' (Type 2)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_inter_area_prefix_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_inter_area_prefix_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF6 option field which is present in "
                   "OSPF6 hello packets, which enables OSPF6 routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF6 "
                   "routers."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf(p.prefix.address, vtss::tag::Name("PrefixAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv6 network address."));

    m.add_leaf(vtss::AsInt(p.prefix.prefix_size),
               vtss::tag::Name("IpSubnetMaskLength"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv6 network mask length."));

    m.add_leaf(
        p.metric, vtss::tag::Name("Metric"),
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_cost_t>(
            VTSS_APPL_OSPF6_GENERAL_COST_MIN,
            VTSS_APPL_OSPF6_GENERAL_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified metric for this summary route. "
            "The field is significant only when the link state "
            "type is 'Inter_Area Prefix/Router Link State' (Type 3, 4)."));
}


template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_inter_area_router_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_inter_area_router_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF6 option field which is present in "
                   "OSPF6 hello packets, which enables OSPF6 routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF6 "
                   "routers."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf(
        p.metric, vtss::tag::Name("Metric"),
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_cost_t>(
            VTSS_APPL_OSPF6_GENERAL_COST_MIN,
            VTSS_APPL_OSPF6_GENERAL_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified metric for this summary route. "
            "The field is significant only when the link state "
            "type is 'Summary/ASBR Summary Link State' (Type 3, 4)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf6_db_external_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf6_db_external_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF6 option field which is present in "
                   "OSPF6 hello packets, which enables OSPF6 routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF6 "
                   "routers."));

    m.add_leaf(p.sequence, vtss::tag::Name("Sequence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LS sequence number of the LSA."));

    m.add_leaf(p.checksum, vtss::tag::Name("Checksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The checksum of the LSA contents."));

    m.add_leaf(p.length, vtss::tag::Name("Length"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Length in bytes of the LSA."));

    m.add_leaf((p.prefix.address), vtss::tag::Name("PrefixAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv6 network address."));

    m.add_leaf(vtss::AsInt(p.prefix.prefix_size),
               vtss::tag::Name("IpSubnetMaskLength"),
               vtss::expose::snmp::RangeSpec<u32>(0, 128),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv6 network mask length."));

    m.add_leaf(p.metric, vtss::tag::Name("Metric"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf6_cost_t>(
                   VTSS_APPL_OSPF6_GENERAL_COST_MIN,
                   VTSS_APPL_OSPF6_GENERAL_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified metric for this summary route. "
                   "The field is significant only when the link state "
                   "type is 'External/NSSA External Link State' (Type 5, "
                   "7)."));

    m.add_leaf(p.metric_type, vtss::tag::Name("MetricType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The External type of the LSA. "
                   "The field is significant only when the link state "
                   "type is 'External/NSSA External Link State' (Type 5, "
                   "7)."));

    m.add_leaf((p.forward_address),
               vtss::tag::Name("ForwardAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The IP address of forward address. "
                   "The field is significant only when the link state "
                   "type is 'External/NSSA External Link State' (Type 5, "
                   "7)."));
}

/******************************************************************************/
/** Table-entry/data structure serializer                                     */
/******************************************************************************/
namespace vtss
{
namespace appl
{
namespace ospf6
{
namespace interfaces
{

//------------------------------------------------------------------------------
//** OSPF6 module capabilities
//------------------------------------------------------------------------------
struct Ospf6CapabilitiesTabular {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ospf6_capabilities_t * >>
                                                            P;

    /* Description */
    static constexpr const char *table_description =
        "This is OSPF6 capabilities tabular. It provides the capabilities "
        "of OSPF6 configuration.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Capabilities"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_capabilities_get);
};

//------------------------------------------------------------------------------
//** OSPF6 instance configuration
//------------------------------------------------------------------------------
struct Ospf6ConfigProcessEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_ospf6_id_t>> P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 process configuration table. It is used to enable or "
        "disable the routing process on a specific process ID.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 routing process.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_inst_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf6_add);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_ospf6_def);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf6_del);
};

//------------------------------------------------------------------------------
//** OSPF6 router configuration
//------------------------------------------------------------------------------
struct Ospf6ConfigRouterEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
            vtss::expose::ParamVal<vtss_appl_ospf6_router_conf_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 router configuration table. It is a general group to "
        "configure the OSPF6 common router parameters.";
    static constexpr const char *index_description =
        "Each entry in this table represents the OSPF6 router interface "
        "configuration.";

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_router_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouterConfig"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_inst_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_router_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_router_conf_set);
    VTSS_EXPOSE_DEF_PTR(frr_ospf6_router_conf_def);
};

//------------------------------------------------------------------------------
//** OSPF6 router status
//------------------------------------------------------------------------------
struct Ospf6StatusRouterEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_router_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 router status table. It is used to provide the "
        "OSPF6 router status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 router status "
        "information.";

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_router_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouterStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_inst_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_router_status_get);
};

//------------------------------------------------------------------------------
//** OSPF6 router interface configuration
//------------------------------------------------------------------------------
struct Ospf6ConfigRouterInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_router_intf_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 router interface configuration table.";
    static constexpr const char *index_description =
        "Each router interface has a set of parameters.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("Interface"));
        serialize(h, ospf6_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_router_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("RouterInterfaceConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_router_intf_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_router_intf_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_router_intf_conf_set);
};

//----------------------------------------------------------------------------
//** OSPF6 area range
//----------------------------------------------------------------------------
struct Ospf6ConfigAreaRangeEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<mesa_ipv6_network_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_area_range_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 area range configuration table. It is used to "
        "summarize the intra area paths from a specific address range in "
        "one summary-LSA(Type-0x2003) and advertised to other areas or "
        "configure the address range status as 'DoNotAdvertise' which the "
        "summary-LSA(Type-0x2003) is suppressed.\n"
        "The area range configuration is used for Area Border Routers "
        "(ABRs) and only router-LSAs(Type-0x2001) and network-LSAs (Type-0x2002) can "
        "be summarized. The AS-external-LSAs(Type-0x4005) cannot be summarized "
        "because the scope is OSPF6 autonomous system (AS). The "
        "AS-external-LSAs(Type-0x4007) cannot be summarized because the feature "
        "is not supported yet.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 area range configuration."
        " The overlap configuration of address range is not allowed in "
        "order to avoid the conflict.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv6_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("NetworkAddress"));
        serialize(h, ospf6_key_ipv6_network(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_ospf6_area_range_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AreaRangeConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_area_range_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_area_range_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_area_range_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf6_area_range_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf6_area_range_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf6_area_range_conf_def);
};

//----------------------------------------------------------------------------
//** OSPF6 stub area
//----------------------------------------------------------------------------
struct Ospf6ConfigStubAreaEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_stub_area_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 stub area configuration table. The configuration is "
        "used to reduce the link-state database size and therefore the "
        "memory and CPU requirement by forbidding some LSAs.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 stub area configuration.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_stub_area_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("StubAreaConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_stub_area_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_stub_area_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_stub_area_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf6_stub_area_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf6_stub_area_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf6_stub_area_conf_def);
};

//----------------------------------------------------------------------------
//** OSPF6 interface parameter tuning
//----------------------------------------------------------------------------
struct Ospf6ConfigInterfaceEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_ospf6_intf_conf_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is interface configuration parameter table.";
    static constexpr const char *index_description =
        "Each interface has a set of parameters.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Interface"));
        serialize(h, ospf6_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("Parameter"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_intf_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_intf_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_intf_conf_set);
    VTSS_EXPOSE_DEF_PTR(frr_ospf6_intf_conf_def);
};

//------------------------------------------------------------------------------
//** OSPF6 status
//------------------------------------------------------------------------------
struct Ospf6StatusAreaEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_area_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 network area status table. It is used to provide the "
        "OSPF6 network area status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 network area status "
        "information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_area_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("AreaStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_area_status_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_area_status_get);
};

//------------------------------------------------------------------------------
//** OSPF6 interface status
//------------------------------------------------------------------------------
struct Ospf6StatusInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_interface_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 interface status table. It is used to provide the "
        "OSPF6 interface status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 interface status "
        "information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Ifindex"));
        serialize(h, ospf6_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_interface_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("InterfaceStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_interface_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_interface_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_interface_status_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 neighbor status
//------------------------------------------------------------------------------
struct Ospf6StatusNeighborIpv6Entry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamKey<mesa_ipv6_t>, vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_neighbor_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 IPv6 neighbor status table. It is used to provide "
        "the OSPF6 neighbor status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 IPv6 neighbor "
        "status information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("NeighborId"));
        serialize(h, ospf6_key_router_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("NeighborIP"));
        serialize(h, ospf6_key_ipv6_addr(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("Interface"));
        serialize(h, ospf6_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_neighbor_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("NeighborStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_neighbor_status_itr2);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_neighbor_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_neighbor_status_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 routing information
//------------------------------------------------------------------------------
struct Ospf6StatusRouteIpv6Entry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_route_type_t>,
         vtss::expose::ParamKey<mesa_ipv6_network_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<mesa_ipv6_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_route_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF6 routing status table. It is used to provide the "
        "OSPF6 routing status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF6 routing status "
        "information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_route_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouteType"));
        serialize(h, ospf6_key_route_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv6_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("Destination"));
        serialize(h, ospf6_key_ipv6_network(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_route_area(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(mesa_ipv6_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("NextHop"));
        serialize(h, ospf6_key_ipv6_nexthop(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_route_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(7),
                              tag::Name("RouteIpv6Status"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_route_ipv6_status_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_route_ipv6_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_route_ipv6_status_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database information
//------------------------------------------------------------------------------
struct Ospf6StatusDbEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_general_info_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }



    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_general_info_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail link information
//------------------------------------------------------------------------------
struct Ospf6StatusDbLinkEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_link_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA Link link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_link_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbLinkStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_link_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_link_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_link_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail Intra Area Prefix information
//------------------------------------------------------------------------------
struct Ospf6StatusDbIntraAreaPrefixEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_intra_area_prefix_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA Inter Area Prefix link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_intra_area_prefix_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbIntraAreaPrefixStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_intra_area_prefix_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_intra_area_prefix_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_intra_area_prefix_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail router information
//------------------------------------------------------------------------------
struct Ospf6StatusDbRouterEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_router_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA Router link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_router_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbRouterStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_router_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_router_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_router_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail network information
//------------------------------------------------------------------------------
struct Ospf6StatusDbNetworkEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_network_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA Network link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_network_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbNetworkStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_network_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_network_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_network_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail inter area_prefix information
//------------------------------------------------------------------------------
struct Ospf6StatusDbInterAreaPrefixEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_inter_area_prefix_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA Summary link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_inter_area_prefix_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbInterAreaPrefixStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_inter_area_prefix_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_inter_area_prefix_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_inter_area_prefix_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail asbr summary information
//------------------------------------------------------------------------------
struct Ospf6StatusDbAsbrSummaryEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_inter_area_router_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA ASBR Summary link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_inter_area_router_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbInterAreaRouterStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_inter_area_router_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_inter_area_router_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_inter_area_router_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail external information
//------------------------------------------------------------------------------
struct Ospf6StatusDbExternalEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf6_id_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf6_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf6_db_external_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF6 LSA External link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf6_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf6_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf6_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf6_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf6_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf6_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf6_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf6_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf6_db_external_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbExternalStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf6_db_detail_external_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf6_db_detail_external_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf6_db_external_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF6 database detail nssa external information
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//** OSPF6 global controls
//------------------------------------------------------------------------------
struct Ospf6ControlGlobalsTabular {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ospf6_control_globals_t * >>
                                                               P;

    /* Description */
    static constexpr const char *table_description =
        "This is OSPF6 global control tabular. It is used to set the OSPF6 "
        "global control options.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf6_control_globals_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf6_control_globals);
    VTSS_EXPOSE_GET_PTR(frr_ospf6_control_globals_dummy_get);
};

}  // namespace interfaces
}  // namespace ospf6
}  // namespace appl
}  // namespace vtss

#endif  // _FRR_OSPF6_SERIALIZER_HXX_

