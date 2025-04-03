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

#ifndef _FRR_OSPF_SERIALIZER_HXX_
#define _FRR_OSPF_SERIALIZER_HXX_

/**
 * \file frr_ospf_serializer.hxx
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
#include "frr_ospf_api.hxx"  // For frr_ospf_control_globals_dummy_get()
#include "frr_ospf_expose.hxx"
#include "vtss/appl/ospf.h"
#include "vtss/appl/types.hxx"
#include "vtss_appl_serialize.hxx"

/**
 * \brief Get status for all neighbor information.
 * \param req [IN]  Json request.
 * \param os  [OUT] Json output string.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

/**
 * \brief Get status for all interface information.
 * \param req [IN]  Json request.
 * \param os  [OUT] Json output string.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_interface_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_ospf_route_ipv4_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_ospf_db_get_all_json(const vtss::expose::json::Request *req,
                                       vtss::ostreamBuf *os);

mesa_rc vtss_appl_ospf_db_router_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf_db_network_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf_db_summary_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf_db_asbr_summary_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf_db_external_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);
mesa_rc vtss_appl_ospf_db_nssa_external_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

/******************************************************************************/
/** enum serializer (enum value-string mapping)                               */
/******************************************************************************/
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_ospf_auth_type_t, "OspfAuthType", vtss_appl_ospf_auth_type_txt,
    "This enumeration indicates OSPF authentication type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_interface_state_t, "OspfInterfaceState",
                         vtss_appl_ospf_interface_state_txt,
                         "The state of the link.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_neighbor_state_t, "OspfNeighborState",
                         vtss_appl_ospf_neighbor_state_txt,
                         "The state of the neighbor node.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_area_type_t, "OspfAreaType",
                         vtss_appl_ospf_area_type_txt, "The area type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_redist_metric_type_t,
                         "OspfRedistributedMetricType",
                         vtss_appl_ospf_redist_metric_type_txt,
                         "The OSPF redistributed metric type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_nssa_translator_role_t,
                         "OspfNssaTranslatorRole",
                         vtss_appl_ospf_nssa_translator_role_text,
                         "The OSPF NSSA translator role.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_nssa_translator_state_t,
                         "OspfNssaTranslatorState",
                         vtss_appl_ospf_nssa_translator_state_text,
                         "The OSPF NSSA translator state.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_route_type_t, "OspfRouteType",
                         vtss_appl_ospf_route_type_txt, "The OSPF route type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_route_br_type_t, "OspfBorderRouterType",
                         vtss_appl_ospf_route_border_router_type_txt,
                         "The border router type of the OSPF route entry.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ospf_lsdb_type_t, "OspfLsdbType",
                         vtss_appl_ospf_lsdb_type_txt,
                         "The type of the link state advertisement.");

/******************************************************************************/
/** Table index/key serializer                                                */
/******************************************************************************/

VTSS_SNMP_TAG_SERIALIZE(ospf_key_instance_id, vtss_appl_ospf_id_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("InstanceId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(
                   VTSS_APPL_OSPF_INSTANCE_ID_START,
                   VTSS_APPL_OSPF_INSTANCE_ID_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF process instance ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_area_id, vtss_appl_ospf_area_id_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF area ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_db_key_area_id, vtss_appl_ospf_area_id_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The OSPF area ID of the link state advertisement. For "
                   "type 5 and type 7, the value is always 255.255.255.255 "
                   "since it is not required for these two types."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_val_area_id, vtss_appl_ospf_area_id_t, a, s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("ospf_val_area_id"));

    m.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF area ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_router_id, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF router ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_ipv4_network, mesa_ipv4_network_t, a, s)
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

VTSS_SNMP_TAG_SERIALIZE(ospf_key_intf_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_ipv4_addr, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("Ipv4Addr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The IPv4 address."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_ipv4_nexthop, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("NextHop"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The nexthop to the route."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_md_key_id, vtss_appl_ospf_md_key_id_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("MdKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The key ID for message digest authentication."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_md_key_precedence, uint32_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("Precedence"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The precedence of message digest key."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_val_md_key_id, vtss_appl_ospf_md_key_id_t, a, s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_md_key_id_t"));
    m.add_leaf(s.inner, vtss::tag::Name("MdKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The key ID for message digest authentication."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_val_area_type, vtss_appl_ospf_auth_type_t, a, s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("ospf_val_area_type"));

    m.add_leaf(s.inner, vtss::tag::Name("AreaAuthType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF authentication type."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_route_type, vtss_appl_ospf_route_type_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("RouteType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF route type."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_route_area, vtss_appl_ospf_area_id_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0), vtss::tag::Description("It indicates which area the route or router can be reached "
                                                                              "via/to."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_db_lsdb_type, vtss_appl_ospf_lsdb_type_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("LsdbType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(
                   "The type of the link state advertisement."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_db_link_state_id, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("LinkStateId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The OSPF link state ID."));
}

VTSS_SNMP_TAG_SERIALIZE(ospf_key_db_adv_router_id, vtss_appl_ospf_router_id_t,
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
void serialize(T &a, vtss_appl_ospf_capabilities_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_capabilities_t"));

    int ix = 0;

    // OSPF valid range: instance ID
    m.add_leaf(p.instance_id_min, vtss::tag::Name("MinInstanceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OSPF minimum instance ID"));
    m.add_leaf(p.instance_id_max, vtss::tag::Name("MaxInstanceId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OSPF maximum instance ID"));

    // OSPF valid range: router ID
    m.add_leaf(p.router_id_min, vtss::tag::Name("MinRouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of OSPF router ID"));
    m.add_leaf(p.router_id_max, vtss::tag::Name("MaxRouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of OSPF router ID"));

    // OSPF valid range: priority
    m.add_leaf(p.priority_min, vtss::tag::Name("MinPriority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of OSPF priority"));
    m.add_leaf(p.priority_max, vtss::tag::Name("MaxPriority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of OSPF priority"));

    // OSPF valid range: general cost
    m.add_leaf(
        p.general_cost_min, vtss::tag::Name("MinGeneralCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF interface cost"));
    m.add_leaf(
        p.general_cost_max, vtss::tag::Name("MaxGeneralCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF interface cost"));

    // OSPF valid range: interface cost
    m.add_leaf(
        p.intf_cost_min, vtss::tag::Name("MinInterfaceCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF interface cost"));
    m.add_leaf(
        p.intf_cost_max, vtss::tag::Name("MaxInterfaceCost"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF interface cost"));

    // OSPF valid range: redistribute cost
    m.add_leaf(p.redist_cost_min, vtss::tag::Name("MinRedistributeCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum value of OSPF redistribute cost"));
    m.add_leaf(p.redist_cost_max, vtss::tag::Name("MaxRedistributeCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum value of OSPF redistribute cost"));

    // OSPF valid rangeL hello interval
    m.add_leaf(
        p.hello_interval_min, vtss::tag::Name("MinHelloInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF hello interval"));
    m.add_leaf(
        p.hello_interval_max, vtss::tag::Name("MaxHelloInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF hello interval"));

    // OSPF valid range: fast hello packets
    m.add_leaf(p.fast_hello_packets_min, vtss::tag::Name("MinFastHelloPackets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum value of OSPF fast hello packets"));
    m.add_leaf(p.fast_hello_packets_max, vtss::tag::Name("MaxFastHelloPackets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum value of OSPF fast hello packets"));

    // OSPF valid range :retransmit interval
    m.add_leaf(p.retransmit_interval_min,
               vtss::tag::Name("MinRetransmitInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum value of OSPF retransmit interval"));
    m.add_leaf(p.retransmit_interval_max,
               vtss::tag::Name("MaxRetransmitInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The maximum value of OSPF retransmit interval"));

    // OSPF valid range: dead interval
    m.add_leaf(
        p.dead_interval_min, vtss::tag::Name("MinDeadInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum value of OSPF dead interval"));
    m.add_leaf(
        p.dead_interval_max, vtss::tag::Name("MaxDeadInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum value of OSPF dead interval"));

    // OSPF valid range: max. metric on startup stage
    m.add_leaf(
        p.router_lsa_startup_min, vtss::tag::Name("MinRouterLsaOnStartup"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum value of OSPF router LSA on startup stage"));
    m.add_leaf(
        p.router_lsa_startup_max, vtss::tag::Name("MaxRouterLsaOnStartup"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of OSPF router LSA on startup stage"));

    // OSPF valid range: router LSA on shutdown stage
    m.add_leaf(p.router_lsa_shutdown_min,
               vtss::tag::Name("MinRouterLsaOnShutdown"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The minimum value of router LSA on shutdown stage"));
    m.add_leaf(
        p.router_lsa_shutdown_max,
        vtss::tag::Name("MaxRouterLsaOnShutdown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of OSPF router LSA on shutdown stage"));

    // OSPF valid range: authentication  message digest key ID
    m.add_leaf(p.md_key_id_min, vtss::tag::Name("MinMdKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of OSPF "
                                      "authentication message digest key ID"));
    m.add_leaf(p.md_key_id_max, vtss::tag::Name("MaxMdKeyId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of OSPF "
                                      "authentication message digest key ID"));

    // OSPF valid range: authentication simple password length
    m.add_leaf(p.simple_pwd_len_min, vtss::tag::Name("MinSimplePwdLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum vlength of OSPF "
                                      "authentication simple password length"));
    m.add_leaf(p.simple_pwd_len_max, vtss::tag::Name("MaxSimplePwdLen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum length of OSPF "
                                      "authentication simple password length"));

    // OSPF valid range: authentication "message digest key length
    m.add_leaf(
        p.md_key_len_min, vtss::tag::Name("MinMdKeyLen"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The minimum length of OSPF authentication "
                               "message digest key length"));
    m.add_leaf(
        p.md_key_len_max, vtss::tag::Name("MaxMdKeyLen"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum length of OSPF authentication "
                               "message digest key length"));

    m.add_leaf(vtss::AsBool(p.rip_redistributed_supported),
               vtss::tag::Name("IsRipRedistributedSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate if RIP redistributed is supported or not"));

    // OSPF valid range: administrative distance
    m.add_leaf(
        p.admin_distance_min, vtss::tag::Name("MinAdminDistance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum value of OSPF administrative distance value"));
    m.add_leaf(
        p.admin_distance_max, vtss::tag::Name("MaxAdminDistance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of OSPF administrative distance value"));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_router_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_router_conf_t"));

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
               vtss::tag::Description("The OSPF router ID"));

    m.add_leaf(vtss::AsBool(p.default_passive_interface),
               vtss::tag::Name("DefPassiveInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Configure all interfaces as "
                                      "passive-interface by default."));

    m.add_leaf(vtss::AsBool(p.is_specific_def_metric),
               vtss::tag::Name("IsSpecificDefMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicate the default metric is a "
                                      "specific configured value or not."));

    m.add_leaf(
        p.def_metric, vtss::tag::Name("DefMetricVal"),
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
            VTSS_APPL_OSPF_REDIST_COST_MIN,
            VTSS_APPL_OSPF_REDIST_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified default metric value for the OSPF routing "
            "protocol. The field is significant only when the arugment "
            "'IsSpecificDefMetric' is TRUE"));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_CONNECTED].type,
               vtss::tag::Name("ConnectedRedistMetricType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF redistributed metric type for "
                                      "the connected interfaces."));

    m.add_leaf(
        vtss::AsBool(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_CONNECTED]
                     .is_specific_metric),
        vtss::tag::Name("ConnectedRedistIsSpecificMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate the metric value of connected interfaces "
            "redistribution is a specific configured value or not."));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_CONNECTED].metric,
               vtss::tag::Name("ConnectedRedistMetricVal"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
                   VTSS_APPL_OSPF_REDIST_COST_MIN,
                   VTSS_APPL_OSPF_REDIST_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified metric value for the connected "
                   "interfaces. The field is significant only when the "
                   "argument 'ConnectedRedistIsSpecificMetric' is TRUE"));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_STATIC].type,
               vtss::tag::Name("StaticRedistMetricType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF redistributed metric type for "
                                      "the static routes."));

    m.add_leaf(
        vtss::AsBool(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_STATIC]
                     .is_specific_metric),
        vtss::tag::Name("StaticRedistIsSpecificMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate the metric value if static route redistribution"
            " is a specific configured value or not."));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_STATIC].metric,
               vtss::tag::Name("StaticRedistMetricVal"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
                   VTSS_APPL_OSPF_REDIST_COST_MIN,
                   VTSS_APPL_OSPF_REDIST_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified metric value for the static routes. "
                   "The field is significant only when the argument "
                   "'StaticRedistIsSpecificMetric' is TRUE"));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_RIP].type,
               vtss::tag::Name("RipRedistMetricType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF redistributed metric type for "
                                      "the RIP routes. The field is "
                                      "significant only when the RIP "
                                      "protocol is supported on the device."));

    m.add_leaf(vtss::AsBool(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_RIP]
                            .is_specific_metric),
               vtss::tag::Name("RipRedistIsSpecificMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate the metric value if RIP route redistribution"
                   " is a specific configured value or not."
                   "The field is significant only when the RIP protocol is "
                   "supported on the device."));

    m.add_leaf(p.redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_RIP].metric,
               vtss::tag::Name("RipRedistMetricVal"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
                   VTSS_APPL_OSPF_REDIST_COST_MIN,
                   VTSS_APPL_OSPF_REDIST_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified metric value for the RIP routes. "
                   "The field is significant only when the RIP protocol is "
                   "supported on the device and argument "
                   "'RipRedistIsSpecificMetric' is TRUE."));

    m.add_leaf(vtss::AsBool(p.stub_router.is_on_startup),
               vtss::tag::Name("IsOnStartup"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Configures OSPF to advertise a maximum metric during "
                   "startup for a configured period of time."));

    m.add_leaf(p.stub_router.on_startup_interval,
               vtss::tag::Name("OnStartupInterval"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MIN,
                   VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("User specified time interval (seconds) "
                                      "to advertise itself as stub area. "
                                      "The field is significant only when the "
                                      "on-startup mode is enabled."));

    m.add_leaf(
        vtss::AsBool(p.stub_router.is_on_shutdown),
        vtss::tag::Name("IsOnShutdown"), vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Configures OSPF to advertise a maximum metric during "
            "shutdown for a configured period of time. "
            "The device advertises a maximum metric "
            "when the OSPF router mode is disabled and notice that the "
            "mechanism also works when the device reboots but not for "
            "the'reload default' case."));

    m.add_leaf(p.stub_router.on_shutdown_interval,
               vtss::tag::Name("OnShutdownInterval"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MIN,
                   VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("User specified time interval (seconds) "
                                      "to wait till shutdown completed. "
                                      "The field is significant only when the "
                                      "on-shutdown mode is enabled."));

    m.add_leaf(vtss::AsBool(p.stub_router.is_administrative),
               vtss::tag::Name("IsAdministrative"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Configures OSPF stub router mode administratively "
                   "applied, for an indefinite period."));

    m.add_leaf(p.def_route_conf.type,
               vtss::tag::Name("DefaultRouteRedistMetricType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF redistributed metric type for "
                                      "a default route."));

    m.add_leaf(
        vtss::AsBool(p.def_route_conf.is_specific_metric),
        vtss::tag::Name("DefaultRouteRedistIsSpecificMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate the metric value of a default route "
            "redistribution is a specific configured value or not."));

    m.add_leaf(p.def_route_conf.metric,
               vtss::tag::Name("DefaultRouteRedistMetricVal"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
                   VTSS_APPL_OSPF_REDIST_COST_MIN,
                   VTSS_APPL_OSPF_REDIST_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified metric value for a default route. The "
                   "field is significant only when the argument "
                   "'DefaultRouteRedistIsSpecificMetric' is TRUE"));

    m.add_leaf(
        vtss::AsBool(p.def_route_conf.is_always),
        vtss::tag::Name("DefaultRouteRedistAlways"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Specifies to always advertise a default route into all "
            "external-routing capable areas. Otherwise, the router "
            "only to advertise the default route when the advertising"
            " router already has a default route."));

    m.add_leaf(p.admin_distance, vtss::tag::Name("AdminDistance"),
               vtss::expose::snmp::RangeSpec<uint8_t>(VTSS_APPL_OSPF_ADMIN_DISTANCE_MIN, VTSS_APPL_OSPF_ADMIN_DISTANCE_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF administrative distance."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_router_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_router_status_t"));

    int ix = 0;

    m.add_leaf(vtss::AsIpv4(p.ospf_router_id), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OSPF router ID"));

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

    m.add_leaf(p.min_lsa_interval, vtss::tag::Name("MinLsaInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum interval (in seconds) between "
                                      "link-state advertisements."));

    m.add_leaf(p.min_lsa_arrival, vtss::tag::Name("MinLsaArrival"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum arrival time (in milliseconds) "
                                      "of link-state advertisements."));

    m.add_leaf(p.external_lsa_count, vtss::tag::Name("ExternalLsaCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Number of external link-state advertisements."));

    m.add_leaf(
        p.external_lsa_checksum, vtss::tag::Name("ExternalLsaChecksum"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of external link-state checksum."));

    m.add_leaf(
        p.attached_area_count, vtss::tag::Name("AttachedAreaCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of areas attached for the router."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_router_intf_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_router_intf_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.passive_enabled),
               vtss::tag::Name("PassiveInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Enable the interface as OSPF passive-interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_intf_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_intf_conf_t"));

    int ix = 0;

    m.add_leaf(p.priority, vtss::tag::Name("Priority"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_priority_t>(
                   VTSS_APPL_OSPF_PRIORITY_MIN, VTSS_APPL_OSPF_PRIORITY_MAX),
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
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
            VTSS_APPL_OSPF_INTF_COST_MIN, VTSS_APPL_OSPF_INTF_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified cost for this interface. "
            "It's link state metric for the interface. "
            "The field is significant only when 'IsSpecificCost' is "
            "TRUE."));

    m.add_leaf(vtss::AsBool(p.is_fast_hello_enabled),
               vtss::tag::Name("IsFastHelloEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Enable the feature of fast hello packets or not."));

    m.add_leaf(p.fast_hello_packets, vtss::tag::Name("FastHelloPackets"),
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_OSPF_FAST_HELLO_MIN,
                                                  VTSS_APPL_OSPF_FAST_HELLO_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "How many Hello packets will be sent per second."));

    m.add_leaf(
        p.dead_interval, vtss::tag::Name("DeadInterval"),
        vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_OSPF_DEAD_INTERVAL_MIN,
                                           VTSS_APPL_OSPF_DEAD_INTERVAL_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The time interval (in seconds) between hello packets."));

    m.add_leaf(p.hello_interval, vtss::tag::Name("HelloInterval"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF_HELLO_INTERVAL_MIN,
                   VTSS_APPL_OSPF_HELLO_INTERVAL_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "How many Hello packets will be sent per second."));

    m.add_leaf(p.retransmit_interval, vtss::tag::Name("RetransmitInterval"),
               vtss::expose::snmp::RangeSpec<u32>(
                   VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MIN,
                   VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The time interval (in seconds) between "
                                      "link-state advertisement(LSA) "
                                      "retransmissions for adjacencies."));

    m.add_leaf(p.auth_type, vtss::tag::Name("AuthType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The authentication type."));

    m.add_leaf(vtss::AsBool(p.is_encrypted), vtss::tag::Name("IsEncrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The flag indicates the simple password is encrypted or "
                   "not."
                   " TRUE means the simple password is encrypted."
                   " FALSE means the simple password is plain text."));

    m.add_leaf(vtss::AsDisplayString(p.auth_key, sizeof(p.auth_key)),
               vtss::tag::Name("SimplePwd"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The simple password."));

    m.add_leaf(vtss::AsBool(p.mtu_ignore),
               vtss::tag::Name("MtuIgnore"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("When false (default), the interface MTU of received OSPF "
                                      "database description (DBD) packets will be checked against our "
                                      "own MTU, and if the remote MTU is greater than our own, the DBD "
                                      "will be ignored. When true, the interface MTU of the received OSPF "
                                      "DBD packets will be ignored."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_auth_digest_key_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_auth_digest_key_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.is_encrypted), vtss::tag::Name("IsEncrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The flag indicates themessage digest key is encrypted "
                   "or not."
                   " TRUE means the message digest key is encrypted."
                   " FALSE means the message digest key is plain text."));

    m.add_leaf(vtss::AsDisplayString(p.digest_key, sizeof(p.digest_key)),
               vtss::tag::Name("MdKey"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The message digest key."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_area_range_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_area_range_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.is_advertised), vtss::tag::Name("Advertised"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "When the value is true, it summarizes intra area paths "
                   "from the address range in one summary-LSA(Type-3) and "
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
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
            VTSS_APPL_OSPF_GENERAL_COST_MIN,
            VTSS_APPL_OSPF_GENERAL_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified cost (or metric) for this summary route. "
            "The field is significant only when 'IsSpecificCost' is "
            "TRUE."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_vlink_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_vlink_conf_t"));

    int ix = 0;

    m.add_leaf(
        p.hello_interval, vtss::tag::Name("HelloInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The time interval (in seconds) between hello packets."));

    m.add_leaf(p.dead_interval, vtss::tag::Name("DeadInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of seconds to wait until the "
                                      "neighbor is declared to be dead."));

    m.add_leaf(p.retransmit_interval, vtss::tag::Name("RetransmitInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The time interval (in seconds) between "
                                      "link-state advertisement(LSA) "
                                      "retransmissions for adjacencies."));

    m.add_leaf(p.auth_type, vtss::tag::Name("AuthType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The authentication type on an area."));

    m.add_leaf(vtss::AsBool(p.is_encrypted), vtss::tag::Name("IsEncrypted"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The flag indicates the simple password is encrypted or "
                   "not."
                   " TRUE means the simple password is encrypted."
                   " FALSE means the simple password is plain text."));

    m.add_leaf(vtss::AsDisplayString(p.simple_pwd, sizeof(p.simple_pwd)),
               vtss::tag::Name("SimplePwd"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The simple password."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_stub_area_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_stub_area_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.is_nssa), vtss::tag::Name("IsNssa"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF stub configured type."));

    m.add_leaf(
        vtss::AsBool(p.no_summary), vtss::tag::Name("NoSummary"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The value is true to configure the inter-area routes do "
            "not inject into this stub area."));

    m.add_leaf(p.nssa_translator_role, vtss::tag::Name("NssaTranslatorRole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF NSSA translator role."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_area_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_area_status_t"));

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

    m.add_leaf(p.attached_intf_active_count,
               vtss::tag::Name("AttachedIntfActiveCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Number of active interfaces attached in the area."));

    m.add_leaf(p.auth_type, vtss::tag::Name("AuthType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The authentication type in the area."));

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

    m.add_leaf(p.router_lsa_count, vtss::tag::Name("RouterLsaCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of the router-LSAs(Type-1) of a "
                                      "given type for the particular area."));

    m.add_leaf(p.router_lsa_checksum, vtss::tag::Name("RouterLsaChecksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The the router-LSAs(Type-1) checksum."));

    m.add_leaf(p.network_lsa_count, vtss::tag::Name("NetworkLsaCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of the network-LSAs(Type-2) of a "
                                      "given type for the particular area."));

    m.add_leaf(
        p.network_lsa_checksum, vtss::tag::Name("NetworkLsaChecksum"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The the network-LSAs(Type-2) checksum."));

    m.add_leaf(p.summary_lsa_count, vtss::tag::Name("SummaryLsaCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of the summary-LSAs(Type-3) of a "
                                      "given type for the particular area."));

    m.add_leaf(
        p.summary_lsa_checksum, vtss::tag::Name("SummaryLsaChecksum"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The the summary-LSAs(Type-3) checksum."));

    m.add_leaf(
        p.asbr_summary_lsa_count, vtss::tag::Name("AsbrSummaryLsaCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of the ASBR-summary-LSAs(Type-4) "
                               "of a given type for the particular area."));

    m.add_leaf(p.asbr_summary_lsa_checksum,
               vtss::tag::Name("AsbrSummaryLsaChecksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The the ASBR-summary-LSAs(Type-4) checksum."));

    m.add_leaf(
        p.nssa_lsa_count, vtss::tag::Name("NssaLsaCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of the NSSA LSAs "
                               "of a given type for the particular area."));

    m.add_leaf(p.nssa_lsa_checksum, vtss::tag::Name("NssaLsaChecksum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The the NSSA LSAs checksum."));

    m.add_leaf(p.nssa_trans_state, vtss::tag::Name("NssaTranslatorState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicate the current state of the "
                                      "NSSA-ABR translator which the router "
                                      "uses to translate Type-7 LSAs in the "
                                      "NSSA to Type-5 LSAs in backbone area."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_interface_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_interface_status_t"));

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

    m.add_leaf(vtss::AsIpv4(p.network.address), vtss::tag::Name("Network"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv4 network address."));

    m.add_leaf(vtss::AsInt(p.network.prefix_size),
               vtss::tag::Name("IpSubnetMaskLength"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IPv4 network mask length."));

    m.add_leaf(vtss::AsIpv4(p.area_id), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF area ID."));

    m.add_leaf(vtss::AsIpv4(p.router_id), vtss::tag::Name("RouterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF router ID."));

    m.add_leaf(vtss::AsInt(p.cost), vtss::tag::Name("Cost"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
                   VTSS_APPL_OSPF_REDIST_COST_MIN,
                   VTSS_APPL_OSPF_REDIST_COST_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The cost of the interface."));

    m.add_leaf(p.state, vtss::tag::Name("State"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The state of the link."));

    m.add_leaf(vtss::AsInt(p.priority), vtss::tag::Name("Priority"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_priority_t>(
                   VTSS_APPL_OSPF_PRIORITY_MIN, VTSS_APPL_OSPF_PRIORITY_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF priority. It helps determine "
                                      "the DR and BDR on the network to which "
                                      "this interface is connected."));

    m.add_leaf(vtss::AsIpv4(p.dr_id), vtss::tag::Name("DrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of DR."));

    m.add_leaf(vtss::AsIpv4(p.dr_addr), vtss::tag::Name("DrAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The IP address of DR."));

    m.add_leaf(vtss::AsIpv4(p.bdr_id), vtss::tag::Name("BdrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of BDR."));

    m.add_leaf(vtss::AsIpv4(p.bdr_addr), vtss::tag::Name("BdrAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The IP address of BDR."));

    m.add_leaf(p.hello_time, vtss::tag::Name("HelloTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Hello timer. A time interval that a "
                                      "router sends an OSPF hello packet."));

    m.add_leaf(
        p.dead_time, vtss::tag::Name("DeadTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Dead timer. Dead timer is a time interval "
                               "to wait before declaring a neighbor dead. "
                               "The unit of time is the second."));

    m.add_leaf(p.dead_time, vtss::tag::Name("WaitTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "This interval is used in Wait Timer. "
                   "Wait timer is a single shot timer that "
                   "causes the interface to exit waiting "
                   "and select a DR on the network. Wait "
                   "Time interval is the same as Dead time interval."));

    m.add_leaf(p.retransmit_time, vtss::tag::Name("RetransmitTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Retransmit timer. A time interval to "
                                      "wait before retransmitting a database "
                                      "description packet when it has not been "
                                      "acknowledged."));

    m.add_leaf(p.hello_due_time, vtss::tag::Name("HelloDueTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Hello due timer. An OSPF hello packet "
                                      "will be sent on this interface after "
                                      "this due time."));

    m.add_leaf(
        p.neighbor_count, vtss::tag::Name("NeighborCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Neighbor count. This is the number of OSPF "
                               "neighbors discovered on this interface."));

    m.add_leaf(p.adj_neighbor_count, vtss::tag::Name("AdjNeighborCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Adjacent neighbor count. This is the "
                                      "number of routers running OSPF that are "
                                      "fully adjacent with this router."));

    m.add_leaf(p.transmit_delay, vtss::tag::Name("TransmitDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The estimated time to transmit a "
                                      "link-state update packet on the "
                                      "interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_neighbor_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_neighbor_status_t"));

    int ix = 0;

    m.add_leaf(vtss::AsIpv4(p.ip_addr), vtss::tag::Name("IpAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The IP address."));

    m.add_leaf(vtss::AsIpv4(p.area_id), vtss::tag::Name("AreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF area ID."));

    m.add_leaf(vtss::AsInt(p.priority), vtss::tag::Name("Priority"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_priority_t>(
                   VTSS_APPL_OSPF_PRIORITY_MIN, VTSS_APPL_OSPF_PRIORITY_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The priority of OSPF neighbor. It indicates the "
                   "priority of the neighbor router. This item is used "
                   "when selecting the DR for the network. The router with "
                   "the highest priority becomes the DR."));

    // vtss_appl_ospf_neighbor_state_t
    m.add_leaf(p.state, vtss::tag::Name("State"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The state of OSPF neighbor. It "
                                      "indicates the functional state of the "
                                      "neighbor router."));

    m.add_leaf(vtss::AsIpv4(p.dr_id), vtss::tag::Name("DrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of DR."));

    m.add_leaf(vtss::AsIpv4(p.dr_addr), vtss::tag::Name("DrAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The IP address of DR."));

    m.add_leaf(vtss::AsIpv4(p.bdr_id), vtss::tag::Name("BdrId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The router ID of BDR."));

    m.add_leaf(vtss::AsIpv4(p.bdr_addr), vtss::tag::Name("BdrAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The IP address of BDR."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF option field which is present in "
                   "OSPF hello packets, which enables OSPF routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF "
                   "routers."));

    m.add_leaf(p.dead_time, vtss::tag::Name("DeadTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Dead timer. It indicates the amount of time "
                   "remaining that the router waits to receive "
                   "an OSPF hello packet from the neighbor "
                   "before declaring the neighbor down."));

    m.add_leaf(vtss::AsIpv4(p.transit_id), vtss::tag::Name("TransitAreaId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The OSPF transit area ID for the "
                                      "neighbor on virtual link interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_route_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_route_status_t"));

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
            "The cost of the route within the OSPF network. It is "
            "valid "
            "for external Type-2 route and always '0' for other route "
            "type."));

    m.add_leaf(p.border_router_type, vtss::tag::Name("BorderRouterType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The border router type of the OSPF "
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
void serialize(T &a, vtss_appl_ospf_db_general_info_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_db_general_info_t"));

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

    m.add_leaf(p.router_link_count, vtss::tag::Name("RouterLinkCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The link count of the LSA. The field is "
                                      "significant only when the link state "
                                      "type is 'Router Link State' (Type 1)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_control_globals_t &s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ospf_control_globals_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.reload_process), vtss::tag::Name("ReloadProcess"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Set true to reload OSPF process."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_db_router_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf_db_router_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF option field which is present in "
                   "OSPF hello packets, which enables OSPF routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF "
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
void serialize(T &a, vtss_appl_ospf_db_network_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf_db_network_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF option field which is present in "
                   "OSPF hello packets, which enables OSPF routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF "
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
        vtss::AsInt(p.network_mask), vtss::tag::Name("NetworkMask"),
        vtss::expose::snmp::RangeSpec<u32>(0, 32),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Network mask length. The field is "
                               "significant only when the link state "
                               "type is 'Network Link State' (Type 2)."));

    m.add_leaf(p.attached_router_count, vtss::tag::Name("AttachedRouterCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The attached router count of the LSA. The field is "
                   "significant only when the link state "
                   "type is 'Network Link State' (Type 2)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_db_summary_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf_db_summary_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF option field which is present in "
                   "OSPF hello packets, which enables OSPF routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF "
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
        vtss::AsInt(p.network_mask), vtss::tag::Name("NetworkMask"),
        vtss::expose::snmp::RangeSpec<u32>(0, 32),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Network mask length. The field is "
            "significant only when the link state "
            "type is 'Summary/ASBR Summary Link State' (Type 3, 4)."));

    m.add_leaf(
        p.metric, vtss::tag::Name("Metric"),
        vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
            VTSS_APPL_OSPF_GENERAL_COST_MIN,
            VTSS_APPL_OSPF_GENERAL_COST_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified metric for this summary route. "
            "The field is significant only when the link state "
            "type is 'Summary/ASBR Summary Link State' (Type 3, 4)."));
}

template <typename T>
void serialize(T &a, vtss_appl_ospf_db_external_data_entry_t &p)
{
    typename T::Map_t m = a.as_map(
                              vtss::tag::Typename("vtss_appl_ospf_db_external_data_entry_t"));

    int ix = 0;

    m.add_leaf(p.age, vtss::tag::Name("Age"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time in seconds since the LSA was originated."));

    m.add_leaf(p.options, vtss::tag::Name("Options"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The OSPF option field which is present in "
                   "OSPF hello packets, which enables OSPF routers to "
                   "support (or not support) optional capabilities, and to "
                   "communicate their capability level to other OSPF "
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

    m.add_leaf(vtss::AsInt(p.network_mask), vtss::tag::Name("NetworkMask"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Network mask length. The field is "
                                      "significant only when the link state "
                                      "type is 'External/NSSA External Link "
                                      "State' (Type 5, 7)."));

    m.add_leaf(p.metric, vtss::tag::Name("Metric"),
               vtss::expose::snmp::RangeSpec<vtss_appl_ospf_cost_t>(
                   VTSS_APPL_OSPF_GENERAL_COST_MIN,
                   VTSS_APPL_OSPF_GENERAL_COST_MAX),
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

    m.add_leaf(vtss::AsIpv4(p.forward_address),
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
namespace ospf
{
namespace interfaces
{

//------------------------------------------------------------------------------
//** OSPF module capabilities
//------------------------------------------------------------------------------
struct OspfCapabilitiesTabular {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ospf_capabilities_t * >>
                                                           P;

    /* Description */
    static constexpr const char *table_description =
        "This is OSPF capabilities tabular. It provides the capabilities "
        "of OSPF configuration.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Capabilities"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_capabilities_get);
};

//------------------------------------------------------------------------------
//** OSPF instance configuration
//------------------------------------------------------------------------------
struct OspfConfigProcessEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_ospf_id_t>> P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF process configuration table. It is used to enable or "
        "disable the routing process on a specific process ID.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF routing process.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_inst_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_add);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_ospf_def);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_del);
};

//------------------------------------------------------------------------------
//** OSPF router configuration
//------------------------------------------------------------------------------
struct OspfConfigRouterEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
            vtss::expose::ParamVal<vtss_appl_ospf_router_conf_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF router configuration table. It is a general group to "
        "configure the OSPF common router parameters.";
    static constexpr const char *index_description =
        "Each entry in this table represents the OSPF router interface "
        "configuration.";

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_router_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouterConfig"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_inst_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_router_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_router_conf_set);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_router_conf_def);
};

//------------------------------------------------------------------------------
//** OSPF router status
//------------------------------------------------------------------------------
struct OspfStatusRouterEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_router_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF router status table. It is used to provide the "
        "OSPF router status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF router status "
        "information.";

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_router_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouterStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_inst_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_router_status_get);
};

//------------------------------------------------------------------------------
//** OSPF network area configuration
//------------------------------------------------------------------------------
struct OspfConfigAreaEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
            vtss::expose::ParamKey<mesa_ipv4_network_t *>,
            vtss::expose::ParamVal<vtss_appl_ospf_area_id_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF area configuration table. It is used to specify the "
        "OSPF enabled interface(s). When OSPF is enabled on the specific "
        "interface(s), the router can provide the network information to "
        "the other OSPF routers via those interfaces.";
    static constexpr const char *index_description =
        "Each entry in this table represents the OSPF enabled interface(s) "
        "and its area ID.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_ipv4_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("NetworkAddress"));
        serialize(h, ospf_key_ipv4_network(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("AreaId"));
        serialize(h, ospf_val_area_id(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_area_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_area_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_area_conf_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_area_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_area_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_area_conf_def);
};

//------------------------------------------------------------------------------
//** OSPF router interface configuration
//------------------------------------------------------------------------------
struct OspfConfigRouterInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_router_intf_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF router interface configuration table.";
    static constexpr const char *index_description =
        "Each router interface has a set of parameters.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("Interface"));
        serialize(h, ospf_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_router_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("RouterInterfaceConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_router_intf_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_router_intf_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_router_intf_conf_set);
};

//----------------------------------------------------------------------------
//** OSPF authentication
//----------------------------------------------------------------------------
struct OspfConfigAreaAuthEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
            vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
            vtss::expose::ParamVal<vtss_appl_ospf_auth_type_t>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF area authentication configuration table. It is used "
        "to applied the authentication to all the interfaces belong to the "
        " area.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF area authentication "
        "configuration. Notice that the authentication type setting on the "
        "sepecific interface overrides the area's setting.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_auth_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("AreaAuthType"));
        serialize(h, ospf_val_area_type(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_area_auth_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_area_auth_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_area_auth_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_area_auth_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_area_auth_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_area_auth_conf_def);
};

struct OspfConfigInterfaceAuthMdKeyEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_md_key_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_auth_digest_key_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is interface authentication message digest key configuration "
        "able.";
    static constexpr const char *index_description =
        "Each interface has a set of parameters.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Interface"));
        serialize(h, ospf_key_intf_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_md_key_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("MdKeyId"));
        serialize(h, ospf_key_md_key_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_auth_digest_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("MdConfig"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_intf_auth_digest_key_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_intf_auth_digest_key_get);
    VTSS_EXPOSE_SET_PTR(frr_ospf_intf_auth_digest_key_dummy_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_intf_auth_digest_key_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_intf_auth_digest_key_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_intf_auth_digest_key_def);
};

//----------------------------------------------------------------------------
//** OSPF area range
//----------------------------------------------------------------------------
struct OspfConfigAreaRangeEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<mesa_ipv4_network_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_area_range_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF area range configuration table. It is used to "
        "summarize the intra area paths from a specific address range in "
        "one summary-LSA(Type-3) and advertised to other areas or "
        "configure the address range status as 'DoNotAdvertise' which the "
        "summary-LSA(Type-3) is suppressed.\n"
        "The area range configuration is used for Area Border Routers "
        "(ABRs) and only router-LSAs(Type-1) and network-LSAs (Type-2) can "
        "be summarized. The AS-external-LSAs(Type-5) cannot be summarized "
        "because the scope is OSPF autonomous system (AS). The "
        "AS-external-LSAs(Type-7) cannot be summarized because the feature "
        "is not supported yet.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF area range configuration."
        " The overlap configuration of address range is not allowed in "
        "order to avoid the conflict.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv4_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("NetworkAddress"));
        serialize(h, ospf_key_ipv4_network(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_ospf_area_range_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AreaRangeConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_area_range_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_area_range_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_area_range_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_area_range_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_area_range_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_area_range_conf_def);
};

//----------------------------------------------------------------------------
//** OSPF virtual link
//----------------------------------------------------------------------------
struct OspfConfigVlinkEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_vlink_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF virtual link configuration table. The virtual link "
        "is established between 2 ABRs to overcome that all the areas have "
        "to be connected directly to the backbone area.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF virtual link "
        "configuration.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("RouterId"));
        serialize(h, ospf_key_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_ospf_vlink_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("VirtualLinkConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_vlink_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_vlink_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_vlink_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_vlink_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_vlink_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_vlink_conf_def);
};

//----------------------------------------------------------------------------
//** OSPF virtual link messge digest key
//----------------------------------------------------------------------------
struct OspfConfigVlinkMdKeyEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_md_key_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_auth_digest_key_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is virtual link authentication message digest key "
        "configuration "
        "able.";
    static constexpr const char *index_description =
        "Each virtual link entry has a set of parameters.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("RouterId"));
        serialize(h, ospf_key_router_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_ospf_md_key_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("MdKeyId"));
        serialize(h, ospf_key_md_key_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_auth_digest_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("MessageDigestConfig"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_vlink_md_key_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_vlink_md_key_conf_get);
    VTSS_EXPOSE_SET_PTR(frr_ospf_vlink_md_key_conf_dummy_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_vlink_md_key_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_vlink_md_key_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_vlink_md_key_conf_def);
};

//----------------------------------------------------------------------------
//** OSPF stub area
//----------------------------------------------------------------------------
struct OspfConfigStubAreaEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_stub_area_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF stub area configuration table. The configuration is "
        "used to reduce the link-state database size and therefore the "
        "memory and CPU requirement by forbidding some LSAs.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF stub area configuration.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_stub_area_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("StubAreaConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_stub_area_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_stub_area_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_stub_area_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ospf_stub_area_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ospf_stub_area_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_stub_area_conf_def);
};

//----------------------------------------------------------------------------
//** OSPF interface parameter tuning
//----------------------------------------------------------------------------
struct OspfConfigInterfaceEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_ospf_intf_conf_t *>>
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
        serialize(h, ospf_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("Parameter"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_intf_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_intf_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_intf_conf_set);
    VTSS_EXPOSE_DEF_PTR(frr_ospf_intf_conf_def);
};

//------------------------------------------------------------------------------
//** OSPF status
//------------------------------------------------------------------------------
struct OspfStatusAreaEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_area_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF network area status table. It is used to provide the "
        "OSPF network area status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF network area status "
        "information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_area_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("AreaStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_area_status_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_area_status_get);
};

//------------------------------------------------------------------------------
//** OSPF interface status
//------------------------------------------------------------------------------
struct OspfStatusInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_interface_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF interface status table. It is used to provide the "
        "OSPF interface status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF interface status "
        "information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Ifindex"));
        serialize(h, ospf_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_interface_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("InterfaceStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_interface_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_interface_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_interface_status_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF neighbor status
//------------------------------------------------------------------------------
struct OspfStatusNeighborIpv4Entry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>, vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_neighbor_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF IPv4 neighbor status table. It is used to provide "
        "the OSPF neighbor status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF IPv4 neighbor "
        "status information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("NeighborId"));
        serialize(h, ospf_key_router_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("NeighborIP"));
        serialize(h, ospf_key_ipv4_addr(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("Interface"));
        serialize(h, ospf_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_neighbor_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("NeighborStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_neighbor_status_itr2);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_neighbor_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_neighbor_status_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF interface message digest key precedence
//------------------------------------------------------------------------------
struct OspfStatusInterfaceMdKeyPrecedenceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamKey<uint32_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_md_key_id_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is virtual link authentication message digest key precedence"
        "configuration "
        "able.";
    static constexpr const char *index_description =
        "Each row contains the corresponding message digest key ID.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Interface"));
        serialize(h, ospf_key_intf_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(uint32_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("Precedence"));
        serialize(h, ospf_key_md_key_precedence(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_md_key_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("MdKeyId"));
        serialize(h, ospf_val_md_key_id(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_intf_md_key_precedence_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_intf_md_key_precedence_get);
};

//------------------------------------------------------------------------------
//** OSPF virtual link message digest key precedence
//------------------------------------------------------------------------------
struct OspfStatusVlinkMdKeyPrecedenceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamKey<uint32_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_md_key_id_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is virtual link authentication message digest key precedence"
        "configuration "
        "able.";
    static constexpr const char *index_description =
        "Each row contains the corresponding message digest key ID.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("RouterId"));
        serialize(h, ospf_key_router_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(uint32_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("Precedence"));
        serialize(h, ospf_key_md_key_precedence(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_md_key_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("MdKeyId"));
        serialize(h, ospf_val_md_key_id(i));
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_vlink_md_key_precedence_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_vlink_md_key_precedence_get);
};

//------------------------------------------------------------------------------
//** OSPF routing information
//------------------------------------------------------------------------------
struct OspfStatusRouteIpv4Entry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_route_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_network_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_route_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is OSPF routing status table. It is used to provide the "
        "OSPF routing status information.";
    static constexpr const char *index_description =
        "Each entry in this table represents OSPF routing status "
        "information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_route_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouteType"));
        serialize(h, ospf_key_route_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_ipv4_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("Destination"));
        serialize(h, ospf_key_ipv4_network(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_route_area(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("NextHop"));
        serialize(h, ospf_key_ipv4_nexthop(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_route_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(7),
                              tag::Name("RouteIpv4Status"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_route_ipv4_status_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_route_ipv4_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_route_ipv4_status_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database information
//------------------------------------------------------------------------------
struct OspfStatusDbEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_general_info_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_general_info_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database detail router information
//------------------------------------------------------------------------------
struct OspfStatusDbRouterEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_router_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA Router link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_router_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbRouterStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_detail_router_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_detail_router_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_router_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database detail network information
//------------------------------------------------------------------------------
struct OspfStatusDbNetworkEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_network_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA Network link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_network_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbNetworkStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_detail_network_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_detail_network_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_network_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database detail summary information
//------------------------------------------------------------------------------
struct OspfStatusDbSummaryEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_summary_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA Summary link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_summary_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbSummaryStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_detail_summary_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_detail_summary_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_summary_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database detail asbr summary information
//------------------------------------------------------------------------------
struct OspfStatusDbAsbrSummaryEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_summary_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA ASBR Summary link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_summary_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbASBRSummaryStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_detail_asbr_summary_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_detail_asbr_summary_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_asbr_summary_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database detail external information
//------------------------------------------------------------------------------
struct OspfStatusDbExternalEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_external_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA External link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_db_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_external_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbExternalStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_detail_external_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_detail_external_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_external_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF database detail nssa external information
//------------------------------------------------------------------------------
struct OspfStatusDbNssaExternalEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_ospf_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_area_id_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_lsdb_type_t>,
         vtss::expose::ParamKey<mesa_ipv4_t>,
         vtss::expose::ParamKey<vtss_appl_ospf_router_id_t>,
         vtss::expose::ParamVal<vtss_appl_ospf_db_external_data_entry_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The OSPF LSA NSSA External link state database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a single link "
        "state advertisement.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("InstanceId"));
        serialize(h, ospf_key_instance_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ospf_area_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("AreaId"));
        serialize(h, ospf_db_key_area_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ospf_lsdb_type_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("LsdbType"));
        serialize(h, ospf_key_db_lsdb_type(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("LinkStateId"));
        serialize(h, ospf_key_db_link_state_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_5(vtss_appl_ospf_router_id_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(5),
                              tag::Name("AdvRouterId"));
        serialize(h, ospf_key_db_adv_router_id(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_6(vtss_appl_ospf_db_external_data_entry_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(6),
                              tag::Name("DbNSSAExternalStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ospf_db_detail_nssa_external_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_ospf_db_detail_nssa_external_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_ospf_db_nssa_external_get_all_json);
};

//------------------------------------------------------------------------------
//** OSPF global controls
//------------------------------------------------------------------------------
struct OspfControlGlobalsTabular {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ospf_control_globals_t * >>
                                                              P;

    /* Description */
    static constexpr const char *table_description =
        "This is OSPF global control tabular. It is used to set the OSPF "
        "global control options.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ospf_control_globals_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ospf_control_globals);
    VTSS_EXPOSE_GET_PTR(frr_ospf_control_globals_dummy_get);
};

}  // namespace interfaces
}  // namespace ospf
}  // namespace appl
}  // namespace vtss

#endif  // _FRR_OSPF_SERIALIZER_HXX_

