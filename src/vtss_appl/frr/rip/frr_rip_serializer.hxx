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

#ifndef _FRR_RIP_SERIALIZER_HXX_
#define _FRR_RIP_SERIALIZER_HXX_

/**
 * \file frr_rip_serializer.hxx
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
#include "frr_rip_api.hxx"
#include "frr_rip_expose.hxx"
#include "vtss/appl/rip.h"
#include "vtss/appl/types.hxx"
#include "vtss_appl_serialize.hxx"

/******************************************************************************/
/** The JSON Get-All functions                                                */
/******************************************************************************/
mesa_rc vtss_appl_rip_peer_get_all_json(const vtss::expose::json::Request *req,
                                        vtss::ostreamBuf *os);

mesa_rc vtss_appl_rip_db_get_all_json(const vtss::expose::json::Request *req,
                                      vtss::ostreamBuf *os);

mesa_rc vtss_appl_rip_intf_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

mesa_rc vtss_appl_rip_offset_list_conf_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os);

/******************************************************************************/
/** enum serializer (enum value-string mapping)                               */
/******************************************************************************/
//----------------------------------------------------------------------------
//** RIP split horizon
//----------------------------------------------------------------------------
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_rip_split_horizon_mode_t,
                         "RipSplitHorizonMode",
                         vtss_appl_rip_split_horizon_mode_txt,
                         "The split horizon mode.");

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_rip_db_proto_type_t, "RipDbProtoType",
                         vtss_appl_rip_db_proto_type_txt,
                         "This enumeration indicates the protocol type of the "
                         "RIP database entry.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_rip_db_proto_subtype_t, "RipDbProtoSubType",
                         vtss_appl_rip_db_proto_subtype_txt,
                         "This enumeration indicates the protocol sub-type of "
                         "the RIP database entry.");

//----------------------------------------------------------------------------
//** RIP version
//----------------------------------------------------------------------------
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_rip_global_ver_t, "RipGlobalVerType",
                         vtss_appl_rip_global_ver_txt,
                         "The global RIP version.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_rip_intf_recv_ver_t, "RipIntfReceiveVersion",
    vtss_appl_rip_intf_recv_ver_txt,
    "The RIP version for the advertisement reception on the interface.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_rip_intf_send_ver_t, "RipIntfSendVersion",
    vtss_appl_rip_intf_send_ver_txt,
    "The RIP version for the advertisement transmission on the interface.");

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_rip_auth_type_t, "RipAuthType",
                         vtss_appl_rip_auth_type_txt,
                         "This enumeration indicates RIP authentication type.");

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_rip_offset_direction_t, "RipOffsetListDirection",
    vtss_appl_rip_offset_direction_txt,
    "The direction to add the offset to routing metric update.");

/******************************************************************************/
/** Table index/key serializer                                                */
/******************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(rip_key_intf_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(rip_key_ipv4_addr, mesa_ipv4_t, a, s)
{
    a.add_leaf(vtss::AsIpv4(s.inner), vtss::tag::Name("Ipv4Addr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The IPv4 address."));
}

VTSS_SNMP_TAG_SERIALIZE(rip_key_ipv4_network, mesa_ipv4_network_t, a, s)
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

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_rip_offset_direction_key,
                        vtss_appl_rip_offset_direction_t, a, s)
{
    a.add_leaf(s.inner, vtss::tag::Name("RipOffsetListDirection"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The direction to add the offset to "
                                      "routing metric update."));
}

/******************************************************************************/
/** Table entry serializer                                                    */
/******************************************************************************/
//------------------------------------------------------------------------------
//** RIP module capabilities
//------------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_capabilities_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_capabilities_t"));

    int ix = 0;

    // RIP valid range: redistributed default metric
    m.add_leaf(
        p.redist_def_metric_min, vtss::tag::Name("MinRedistDefMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum value of RIP redistributed default metric"));
    m.add_leaf(
        p.redist_def_metric_max, vtss::tag::Name("MaxRedistDefMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of RIP redistributed default metric"));

    // RIP valid range: redistributed specific metric
    m.add_leaf(p.redist_specific_metric_min,
               vtss::tag::Name("MinRedistSpecificMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of RIP redistributed "
                                      "specific metric value."));
    m.add_leaf(p.redist_specific_metric_max,
               vtss::tag::Name("MaxRedistSpecificMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of RIP redistributed "
                                      "specific metric value."));

    // Indicate if OSPF redistributed is supported or not
    m.add_leaf(vtss::AsBool(p.ospf_redistributed_supported),
               vtss::tag::Name("IsOspfRedistributedSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate if OSPF redistributed is supported or not"));

    // RIP valid range: timer
    m.add_leaf(p.timer_min, vtss::tag::Name("MinTimer"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of RIP timers value"));
    m.add_leaf(p.timer_max, vtss::tag::Name("MaxTimer"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of RIP timers value"));

    // RIP valid range: administrative distance
    m.add_leaf(
        p.admin_distance_min, vtss::tag::Name("MinAdminDistance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum value of RIP administrative distance value"));
    m.add_leaf(
        p.admin_distance_max, vtss::tag::Name("MaxAdminDistance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum value of RIP administrative distance value"));
    // RIP valid range: authentication simple password
    m.add_leaf(p.simple_pwd_string_len_max, vtss::tag::Name("SimplePwdLenMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Maximum value of simple password string length"));
    // RIP valid range: authentication key chain name
    m.add_leaf(
        p.key_chain_name_len_max, vtss::tag::Name("KeyChainNameLenMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum value of key chain name length"));

    // offset-list max. count
    m.add_leaf(
        p.offset_list_max_count, vtss::tag::Name("OffsetListMaxCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum count of the RIP offset-list"));

    // RIP valid range: offset-list metric
    m.add_leaf(p.offset_list_metric_min, vtss::tag::Name("MinOffsetListMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The minimum value of RIP offset-list "
                                      "metric value."));
    m.add_leaf(p.offset_list_metric_max, vtss::tag::Name("MaxOffsetListMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum value of RIP offset-list "
                                      "specific metric value."));

    // network segments max. count
    m.add_leaf(p.network_segment_max_count,
               vtss::tag::Name("NetworkSegmentMaxCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The maximum count of the RIP count of "
                                      "the RIP network segments"));

    // neighbors max. count
    m.add_leaf(
        p.neighbor_max_count, vtss::tag::Name("NeighborMaxCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum count of the RIP neighbors"));
}

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_router_conf_t &s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_router_conf_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.router_mode), vtss::tag::Name("RouterMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("RIP router mode."));

    m.add_leaf(s.version, vtss::tag::Name("Version"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("RIP version support."));

    m.add_leaf(s.timers.update_timer, vtss::tag::Name("UpdateTimer"),
               vtss::expose::snmp::RangeSpec<vtss_appl_rip_timer_t>(
                   VTSS_APPL_RIP_TIMER_MIN, VTSS_APPL_RIP_TIMER_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The timer interval (in seconds) between "
                                      "the router sends the complete routing "
                                      "table to all neighboring RIP routers."));

    m.add_leaf(s.timers.invalid_timer, vtss::tag::Name("InvalidTimer"),
               vtss::expose::snmp::RangeSpec<vtss_appl_rip_timer_t>(
                   VTSS_APPL_RIP_TIMER_MIN, VTSS_APPL_RIP_TIMER_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The invalid timer is the number of "
                   "seconds after which a route will be marked invalid."));

    m.add_leaf(
        s.timers.garbage_collection_timer,
        vtss::tag::Name("GarbageCollectionTimer"),
        vtss::expose::snmp::RangeSpec<vtss_appl_rip_timer_t>(
            VTSS_APPL_RIP_TIMER_MIN, VTSS_APPL_RIP_TIMER_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The garbage collection timer is the "
            "number of seconds after which a route will be deleted."));

    m.add_leaf(
        s.redist_def_metric, vtss::tag::Name("RedistDefaultMetric"),
        vtss::expose::snmp::RangeSpec<vtss_appl_rip_metric_t>(
            VTSS_APPL_RIP_REDIST_DEF_METRIC_MIN,
            VTSS_APPL_RIP_REDIST_DEF_METRIC_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The RIP default redistributed metric."
            "It is used when the metric value isn't specificed for the "
            "redistributed protocol type."));

    m.add_leaf(
        vtss::AsBool(s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED]
                     .is_enabled),
        vtss::tag::Name("ConnectedRedistEnabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate if the router redistribute the directly "
            "connected routes with RIP not enabled into the "
            "RIP domain or not."));

    m.add_leaf(
        vtss::AsBool(s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED]
                     .is_specific_metric),
        vtss::tag::Name("ConnectedRedistIsSpecificMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate the metric value of connected interfaces "
            "redistribution is a specific configured value or not."));

    m.add_leaf(s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED].metric,
               vtss::tag::Name("ConnectedRedistMetricVal"),
               vtss::expose::snmp::RangeSpec<vtss_appl_rip_metric_t>(
                   VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN,
                   VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "User specified metric value for the connected "
                   "interfaces. The field is significant only when the "
                   "argument 'ConnectedRedistIsSpecificMetric' is TRUE. "
                   "If the specific metric setting is removed while the "
                   "connected redistributed mode is enabled, the router "
                   "will updates the original connected redistributed "
                   "routes with metric value 16 before updates to the "
                   "new metric value."));

    m.add_leaf(
        vtss::AsBool(s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC]
                     .is_enabled),
        vtss::tag::Name("StaticRedistEnabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate if the router redistribute the static routes into"
            "the RIP domain or not."));

    m.add_leaf(
        vtss::AsBool(s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC]
                     .is_specific_metric),
        vtss::tag::Name("StaticRedistIsSpecificMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate the metric value if static route redistribution"
            " is a specific configured value or not."));

    m.add_leaf(
        s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC].metric,
        vtss::tag::Name("StaticRedistMetricVal"),
        vtss::expose::snmp::RangeSpec<vtss_appl_rip_metric_t>(
            VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN,
            VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified metric value for the static routes. "
            "The field is significant only when the argument "
            "'StaticRedistIsSpecificMetric' is TRUE. "
            "If the specific metric setting is removed while the "
            "static redistributed mode is enabled, the router will "
            "updates the original static redistributed routes with "
            "metric value 16 before updates to the new metric value"));

    m.add_leaf(
        vtss::AsBool(
            s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF].is_enabled),
        vtss::tag::Name("OspfRedistEnabled"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate if the router redistribute the OSPF routes into "
            "the RIP domain or not. The field is significant only "
            "when the OSPF protocol is supported on the device."));

    m.add_leaf(
        vtss::AsBool(s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF]
                     .is_specific_metric),
        vtss::tag::Name("OspfRedistIsSpecificMetric"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicate the metric value if RIP route redistribution"
            " is a specific configured value or not."
            "The field is significant only when the OSPF protocol is "
            "supported on the device."));

    m.add_leaf(
        s.redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF].metric,
        vtss::tag::Name("OspfRedistMetricVal"),
        vtss::expose::snmp::RangeSpec<vtss_appl_rip_metric_t>(
            VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN,
            VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "User specified metric value for the RIP routes. "
            "The field is significant only when the OSPF protocol is "
            "supported on the device and argument "
            "'OspfRedistIsSpecificMetric' is TRUE. "
            "If the specific metric setting is removed while the "
            "OSPF redistributed mode is enabled, the router will "
            "updates the original OSPF redistributed routes with "
            "metric value 16 before updates to the new metric value"));

    m.add_leaf(vtss::AsBool(s.def_route_redist),
               vtss::tag::Name("DefaultRouteRedist"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The RIP default route redistribution."));

    m.add_leaf(vtss::AsBool(s.def_passive_intf),
               vtss::tag::Name("DefPassiveIntf"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Configure all interfaces as "
                                      "passive-interface by default."));

    m.add_leaf(s.admin_distance, vtss::tag::Name("AdminDistance"),
               vtss::expose::snmp::RangeSpec<vtss_appl_rip_distance_t>(
                   VTSS_APPL_RIP_ADMIN_DISTANCE_MIN,
                   VTSS_APPL_RIP_ADMIN_DISTANCE_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The RIP administrative distance."));
}

//----------------------------------------------------------------------------
//** RIP router interface configuration
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_router_intf_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_router_intf_conf_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(p.passive_enabled),
               vtss::tag::Name("PassiveInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Enable the interface as RIP passive-interface."));
}

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_intf_conf_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_intf_conf_t"));

    int ix = 0;

    m.add_leaf(p.send_ver, vtss::tag::Name("SendVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The RIP version for the advertisement "
                                      "transmission on the interface."));

    m.add_leaf(p.recv_ver, vtss::tag::Name("RecvVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The RIP version for the advertisement "
                                      "reception on the interface."));

    m.add_leaf(p.split_horizon_mode, vtss::tag::Name("SplitHorizonMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The split horizon mode."));

    m.add_leaf(p.auth_type, vtss::tag::Name("AuthType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The authentication type."));

    m.add_leaf(vtss::AsDisplayString(p.md5_key_chain_name,
                                     sizeof(p.md5_key_chain_name)),
               vtss::tag::Name("KeyChainName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The key chain used by MD5 authentication."));

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

struct RipConfigInterfaceEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_rip_intf_conf_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is RIP interface configuration table.";
    static constexpr const char *index_description =
        "Each interface has a set of parameters.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              vtss::tag::Name("Interface"));
        serialize(h, rip_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_rip_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              vtss::tag::Name("Parameter"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_intf_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_intf_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rip_intf_conf_set);
    VTSS_EXPOSE_DEF_PTR(frr_rip_intf_conf_def);
};

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
// Network entry only has key, no data field

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
// No data field in neighbor entries

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_offset_entry_data_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_offset_entry_data_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(p.name.name, sizeof(p.name.name)),
               vtss::tag::Name("AccessListName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Access-list name."));

    m.add_leaf(p.offset_metric, vtss::tag::Name("OffsetMetric"),
               vtss::expose::snmp::RangeSpec<vtss_appl_rip_metric_t>(
                   VTSS_APPL_RIP_OFFSET_LIST_METRIC_MIN,
                   VTSS_APPL_RIP_OFFSET_LIST_METRIC_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The offset to incoming or outgoing routing metric."));
}

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_general_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_general_status_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(p.is_enabled),
               vtss::tag::Name("RouterModeIsEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("RIP router mode."));

    m.add_leaf(p.version, vtss::tag::Name("version"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "This indicates the global rip version. By default, the "
                   "router sends RIPv2 and accepts both RIPv1 and RIPv2. "
                   "When the router receive either version of REQUESTS or "
                   "triggered updates packets, it replies with the "
                   "appropriate version. "
                   "Be aware that the RIP network class configuration "
                   "when RIPv1 is involved in the topology. RIPv1 uses "
                   "classful routing, the subnet information is not "
                   "included in the routing updates. The limitation makes "
                   "it impossible to  have different-sized subnets inside "
                   "of the same network class. In other words, all "
                   "subnets in a network class must have the same size."));

    m.add_leaf(p.timers.update_timer, vtss::tag::Name("UpdateTimer"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The timer interval (in seconds) between "
                                      "the router sends the complete routing "
                                      "table to all neighboring RIP routers"));

    m.add_leaf(p.timers.invalid_timer, vtss::tag::Name("InvalidTimer"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The invalid timer is the number of "
                   "seconds after which a route will be marked invalid."));

    m.add_leaf(
        p.timers.garbage_collection_timer,
        vtss::tag::Name("GarbageCollectionTimer"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The garbage collection timer is the "
            "number of seconds after which a route will be deleted."));

    m.add_leaf(p.next_update_time, vtss::tag::Name("NextUpdateTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Specifies when the next round of updates will be sent "
                   "out from this router in seconds."));

    m.add_leaf(p.default_metric, vtss::tag::Name("RedistDefMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This indicates the default metric value "
                                      "of redistributed routes."));

    m.add_leaf(
        vtss::AsBool(
            p.redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED]),
        vtss::tag::Name("IsRedistributeConnected"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This indicates the connected route is "
                               "redistributed or not."));

    m.add_leaf(
        vtss::AsBool(
            p.redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC]),
        vtss::tag::Name("IsRedistributeStatic"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This indicates the static route is "
                               "redistributed or not."));

    m.add_leaf(
        vtss::AsBool(
            p.redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF]),
        vtss::tag::Name("IsRedistributeOSPF"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "This indicates the OSPF route is redistributed or not."));

    m.add_leaf(p.admin_distance, vtss::tag::Name("AdminDistance"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "This indicates administrative distance value"));
}

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_interface_status_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_interface_status_t"));
    int ix = 0;

    m.add_leaf(p.send_version, vtss::tag::Name("SendVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The RIP version for the advertisement "
                                      "transmission on the interface."));

    m.add_leaf(p.recv_version, vtss::tag::Name("RecvVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The RIP version for the advertisement "
                                      "reception on the interface."));

    m.add_leaf(vtss::AsBool(p.triggered_update),
               vtss::tag::Name("TriggeredUpdate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This indicates the interface enable "
                                      "triggered update or not."));

    m.add_leaf(vtss::AsBool(p.is_passive_intf),
               vtss::tag::Name("IsPassiveInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This indicates if the passive-interface "
                                      "is active on the interface or not."));

    m.add_leaf(p.auth_type, vtss::tag::Name("AuthType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The authentication type."));

    m.add_leaf(
        vtss::AsDisplayString(p.key_chain, sizeof(p.key_chain)),
        vtss::tag::Name("KeyChainName"), vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This indicates the interface is associate "
                               "with a specific key-chain name."));
}

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_peer_data_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_peer_data_t"));
    int ix = 0;

    m.add_leaf(p.rip_version, vtss::tag::Name("version"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The RIP version number in the header of the last"
                   " RIP packet received from the neighbor."));

    m.add_leaf(p.last_update_time, vtss::tag::Name("LastUpdateTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time duration in seconds from the time the last RIP"
                   " packet received from the neighbor to now."));

    m.add_leaf(
        p.recv_bad_packets, vtss::tag::Name("RecvBadPackets"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of RIP response packets"
                               " from the neighbor discarded as invalid."));

    m.add_leaf(p.recv_bad_routes, vtss::tag::Name("RecvBadRoutes"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The number of routes from the neighbor"
                   " that were ignored because they were invalid."));
}

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_db_key_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_db_key_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsIpv4(p.network.address), vtss::tag::Name("NetworkAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The destination IP address of the route."));

    m.add_leaf(vtss::AsInt(p.network.prefix_size),
               vtss::tag::Name("NetworkPrefixSize"),
               vtss::expose::snmp::RangeSpec<u32>(0, 32),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The destination network prefix size of the route."));

    m.add_leaf(
        vtss::AsIpv4(p.nexthop), vtss::tag::Name("NextHop"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The first gateway along the route to the destination."));
}

template <typename T>
void serialize(T &a, vtss_appl_rip_db_data_t &p)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_db_data_t"));
    int ix = 0;

    m.add_leaf(p.type, vtss::tag::Name("Type"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The protocol type of the route."));

    m.add_leaf(p.subtype, vtss::tag::Name("SubType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The protocol sub-type of the route."));

    m.add_leaf(p.metric, vtss::tag::Name("Metric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The metric of the route."));

    m.add_leaf(p.external_metric, vtss::tag::Name("ExternalMetric"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The field is significant only when the "
                   "route is redistributed from other protocol type, for "
                   "example, OSPF. This indicates the metric value from "
                   "the original redistributed source."));

    m.add_leaf(
        vtss::AsBool(p.self_intf), vtss::tag::Name("IsSelfInterface"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This indicates the route is generated "
                               "from one of the local interfaces or not."));

    m.add_leaf(vtss::AsIpv4(p.src_addr), vtss::tag::Name("SourceAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The learning source IP address of the route. This "
                   "indicates the route is learned an IP address. The "
                   "field is significant only when the route isn't "
                   "generated from the local interfaces."));

    m.add_leaf(p.tag, vtss::tag::Name("Tag"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The tag of the route. It is used to "
                   "provide a method of separating 'internal' RIP routes, "
                   "which may have been imported from an EGP (Exterior "
                   "gateway protocol) or another IGP (Interior gateway "
                   "protocol). For example, routes imported from OSPF "
                   "can have a route tag value which the other routing "
                   "protocols can use to prevent advertising the same "
                   "route back to the original protocol routing domain."));

    m.add_leaf(p.uptime, vtss::tag::Name("Uptime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The time field is significant only when the route is "
                   "learned from the neighbors. When the route destination "
                   "is reachable (its metric value less than 16), the time "
                   "field means the invalid time of the route. When the "
                   "route destination is unreachable (its metric value "
                   "great than 16), the time field means the "
                   "garbage-collection time of the route."));
}

//------------------------------------------------------------------------------
//** RIP global controls
//------------------------------------------------------------------------------
template <typename T>
void serialize(T &a, vtss_appl_rip_control_globals_t &s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_rip_control_globals_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.reload_process), vtss::tag::Name("ReloadProcess"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Set true to reload RIP process."));
}

/******************************************************************************/
/** Table-entry/data structure serializer                                     */
/******************************************************************************/
namespace vtss
{
namespace appl
{
namespace rip
{
namespace interfaces
{

//------------------------------------------------------------------------------
//** RIP module capabilities
//------------------------------------------------------------------------------
struct RipCapabilitiesTabular {
    typedef vtss::expose::ParamList<vtss::expose::ParamVal<vtss_appl_rip_capabilities_t *>>
            P;

    /* Description */
    static constexpr const char *table_description =
        "This is RIP capabilities tabular. It provides the capabilities "
        "of RIP configuration.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_rip_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Capabilities"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_capabilities_get);
};

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
struct RipConfigRouterTabular {
    typedef vtss::expose::ParamList<vtss::expose::ParamVal<vtss_appl_rip_router_conf_t *>>
            P;

    /* Description */
    static constexpr const char *table_description =
        "This is RIP router configuration table. It is a general group to "
        "configure the RIP common router parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_rip_router_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_router_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rip_router_conf_set);
    VTSS_EXPOSE_DEF_PTR(frr_rip_router_conf_def);
};

//----------------------------------------------------------------------------
//** RIP router interface configuration
//----------------------------------------------------------------------------
struct RipConfigRouterInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_rip_router_intf_conf_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is RIP router interface configuration table.";
    static constexpr const char *index_description =
        "Each router interface has a set of parameters.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Interface"));
        serialize(h, rip_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_rip_router_intf_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("RouterInterfaceConf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_router_intf_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_router_intf_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rip_router_intf_conf_set);
};

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
struct RipConfigNetworkEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<mesa_ipv4_network_t *>> P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is RIP network configuration table. It is used to specify "
        "the RIP enabled interface(s). When RIP is enabled "
        "on the specific interface(s), the router can provide the "
        "network information to the other RIP routers via those "
        "interfaces.";
    static constexpr const char *index_description =
        "Each entry in this table represents the RIP enabled interface(s).";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_ipv4_network_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("NetworkAddress"));
        serialize(h, rip_key_ipv4_network(i));
    }

    /* No entry data */

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_network_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_network_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rip_network_conf_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rip_network_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rip_network_conf_del);
    VTSS_EXPOSE_DEF_PTR(frr_rip_network_conf_def);
};

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
struct RipConfigNeighborEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<mesa_ipv4_t>> P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is RIP neighbor connection table. It is used to configure "
        "the RIP router to send RIP updates to specific neighbors using "
        "the unicast, broadcast, or network IP address after update timer "
        "expiration.";
    static constexpr const char *index_description =
        "Each entry in this table represents the neighbor address.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry key */
    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("NeighborAddress"));
        serialize(h, rip_key_ipv4_addr(i));
    }

    /* No entry data */

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_neighbor_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_neighbor_conf_get);
    VTSS_EXPOSE_SET_PTR(frr_rip_neighbor_dummy_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rip_neighbor_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rip_neighbor_conf_del);
    /* The default function is unnecessary because there's no data in the table
     */
};

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
struct RipConfigOffsetListEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<vtss_appl_rip_offset_direction_t>,
         vtss::expose::ParamVal<vtss_appl_rip_offset_entry_data_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is RIP offset-list configuration table.";
    static constexpr const char *index_description =
        "Each offset-list entry has a set of parameters.";

    /* SNMP row editor OID (Provides for SNMP Add/Delete operations) */
    static constexpr uint32_t snmpRowEditorOid = 100;

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              vtss::tag::Name("Interface"));
        serialize(h, rip_key_intf_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_rip_offset_direction_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              vtss::tag::Name("Direction"));
        serialize(h, vtss_appl_rip_offset_direction_key(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_rip_offset_entry_data_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(3),
                              tag::Name("OffsetEntryData"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_offset_list_conf_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_offset_list_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rip_offset_list_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rip_offset_list_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rip_offset_list_conf_del);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_rip_offset_list_conf_get_all_json);
};

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
struct RipStatusGeneralEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_rip_general_status_t * >>
                                                            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The RIP general status information table.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_rip_general_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("RouterStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_general_status_get);
};

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
struct RipStatusInterfaceEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_rip_interface_status_t * >>
         P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The RIP interface status information table.";
    static constexpr const char *index_description =
        "Each entry in this table represents RIP the active interface "
        "status information.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("Interface"));
        serialize(h, rip_key_intf_index(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_rip_interface_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("InterfaceStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_intf_status_ifindex_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_intf_status_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_rip_intf_status_get_all_json);
};

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
struct RipStatusPeerEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<mesa_ipv4_t>,
            vtss::expose::ParamVal<vtss_appl_rip_peer_data_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "This is RIP peer table. It is used to provide the "
        "RIP peer information.";
    static constexpr const char *index_description =
        "Each entry in this table represents the information "
        "for a RIP peer.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_ipv4_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("PeerIP"));
        serialize(h, rip_key_ipv4_addr(i));
    }

    /* Entry data */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_rip_peer_data_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2),
                              tag::Name("PeerStatus"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_peer_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_peer_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_rip_peer_get_all_json);
};

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
struct RipStatusDbEntry {
    typedef vtss::expose::ParamList<vtss::expose::ParamKey<vtss_appl_rip_db_key_t *>,
            vtss::expose::ParamVal<vtss_appl_rip_db_data_t *>>
            P;

    /* Descriptions */
    static constexpr const char *table_description =
        "The RIP database information table.";
    static constexpr const char *index_description =
        "Each row contains the corresponding information of a routing "
        "entry in the RIP database.";

    /* Entry keys */
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_rip_db_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1),
                              tag::Name("DbEntryKey"));
        serialize(h, i);
    }

    /* Entry data, the SNMP OID offset number is started from 4 since
     * there are 3 elements in the RIP database key structure */
    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_rip_db_data_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(4),
                              tag::Name("DbEntryData"));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rip_db_itr);
    VTSS_EXPOSE_GET_PTR(vtss_appl_rip_db_get);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_rip_db_get_all_json);
};

//------------------------------------------------------------------------------
//** RIP global controls
//------------------------------------------------------------------------------
struct RIpControlGlobalsTabular {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_rip_control_globals_t * >>
                                                             P;

    /* Description */
    static constexpr const char *table_description =
        "This is RIP global control tabular. It is used to set the RIP "
        "global control options.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_rip_control_globals_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FRR);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rip_control_globals);
    VTSS_EXPOSE_GET_PTR(frr_rip_control_globals_dummy_get);
};

}  // namespace interfaces
}  // namespace rip
}  // namespace appl
}  // namespace vtss

#endif  // _FRR_RIP_SERIALIZER_HXX_

