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

#ifdef VTSS_BASICS_STANDALONE
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "frr_ospf_access.hxx"
#include "frr_utils.hxx"
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <initializer_list>
#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/expose/json.hxx>
#include <vtss/basics/expose/json/enum-macros.hxx>
#include <vtss/basics/fd.hxx>
#include <vtss/basics/notifications/process-daemon.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/string-utils.hxx>
#include <vtss/basics/time.hxx>
#include <vtss/basics/vector.hxx>
#ifdef VTSS_BASICS_STANDALONE
#include <signal.h>
#else
#include <vtss_timer_api.h>
#include "icli_api.h"
#include "ip_utils.hxx"  // For the operator of mesa_ipv4_network_t
#include "subject.hxx"
#endif

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#ifndef VTSS_BASICS_STANDALONE
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_OSPF
#include "frr_trace.hxx"  // For module trace group definitions
#else
#include <vtss/basics/trace.hxx>
#endif

vtss_enum_descriptor_t ip_ospf_area_authentication_txt[] {
    {VTSS_APPL_OSPF_AUTH_TYPE_NULL,            "authenticationNone"},
    {VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD, "authenticationSimplePassword"},
    {VTSS_APPL_OSPF_AUTH_TYPE_MD5,             "authenticationMessageDigest"},
    {VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG,        "authenticationAreaCfg"},
    {}
};

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ospf_auth_type_t, "authentication", ip_ospf_area_authentication_txt, "-");

vtss_enum_descriptor_t ip_ospf_neighbor_nsm_state_txt[] {
    {vtss::NSM_DependUpon, "DependUpon"},
    {vtss::NSM_Deleted, "Deleted"},
    {vtss::NSM_Down, "Down"},
    {vtss::NSM_Attempt, "Attempt"},
    {vtss::NSM_Init, "Init"},
    {vtss::NSM_TwoWay, "2-Way"},
    {vtss::NSM_ExStart, "ExStart"},
    {vtss::NSM_Exchange, "Exchange"},
    {vtss::NSM_Loading, "Loading"},
    {vtss::NSM_Full, "Full"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspfNeighborNSMState, "nbrState",
                         ip_ospf_neighbor_nsm_state_txt, "-");

vtss_enum_descriptor_t ip_ospf_interface_network_type_txt[] {
    {vtss::NetworkType_Null, "Null"},
    {vtss::NetworkType_PointToPoint, "POINTOPOINT"},
    {vtss::NetworkType_Broadcast, "BROADCAST"},
    {vtss::NetworkType_Nbma, "NBMA"},
    {vtss::NetworkType_PointToMultipoint, "POINTTOMULTIPOINT"},
    {vtss::NetworkType_VirtualLink, "VIRTUALLINK"},
    {vtss::NetworkType_LoopBack, "LOOPBACK"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspfIfNetworkType, "networkType",
                         ip_ospf_interface_network_type_txt, "-");

vtss_enum_descriptor_t ip_ospf_interface_ism_state_txt[] {
    {vtss::ISM_DependUpon, "DependUpon"},
    {vtss::ISM_Down, "Down"},
    {vtss::ISM_Loopback, "LoopBack"},
    {vtss::ISM_Waiting, "Waiting"},
    {vtss::ISM_PointToPoint, "Point-To-Point"},
    {vtss::ISM_DROther, "DROther"},
    {vtss::ISM_Backup, "Backup"},
    {vtss::ISM_DR, "DR"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspfIfISMState, "state",
                         ip_ospf_interface_ism_state_txt, "-");

vtss_enum_descriptor_t ip_ospf_interface_type_txt[] {
    {vtss::IfType_Peer, "Peer"}, {vtss::IfType_Broadcast, "Broadcast"}, {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspfIfType, "ospfIfType",
                         ip_ospf_interface_type_txt, "-");

vtss_enum_descriptor_t FrrOspfLsdbType_txt[] {
    {vtss::FrrOspfLsdbType_Router, "router-LSA"},
    {vtss::FrrOspfLsdbType_Network, "network-LSA"},
    {vtss::FrrOspfLsdbType_Summary, "summary-LSA"},
    {vtss::FrrOspfLsdbType_AsbrSummary, "summary-LSA"},
    {vtss::FrrOspfLsdbType_AsExternal, "AS-external-LSA"},
    {vtss::FrrOspfLsdbType_Nssa, "NSSA-LSA"},
    {vtss::FrrOspfLsdbType_Opaque, "Link-Local Opaque-LSA"},
    {vtss::FrrOspfLsdbType_OpaqueArea, "Area-Local Opaque-LSA"},
    {vtss::FrrOspfLsdbType_OpaqueAs, "AS-external Opaque-LSA"},
    {0, 0}
};
VTSS_JSON_SERIALIZE_ENUM(FrrOspfLsdbType, "ospfLsdbType", FrrOspfLsdbType_txt,
                         "-");

namespace vtss
{

/* Assign a default value to argument 'X' since it is an optional JSON object
 * in FRR output.
 */
#define ADD_LEAF_DEFAULT(X, TAG, default_value) \
    if (!m.add_leaf(X, TAG)) {                  \
        X = default_value;                      \
    }

template <typename KEY, typename VAL>
struct VectorMapSortedCb {
    virtual void entry(const KEY &key, VAL &val) {}

    virtual ~VectorMapSortedCb() = default;
};

template <typename VAL>
struct StructValCb {
    virtual void convert(const VAL &val) {}

    virtual ~StructValCb() = default;
};

/******************************************************************************/
// This one is called when loader.load() is invoked in
// frr_ip_ospf_interface_status_parse()
//
// The output of "show ip ospf interface json" could look like this:
//
// {
//  "interfaces":[
//    {
//      "vtss.vlan.10":{
//        "ifUp":true,
//        "ifIndex":4,
//        "mtuBytes":1500,
//        "bandwidthMbit":0,
//        "ifFlags":"<UP,BROADCAST,RUNNING,MULTICAST>",
//        "ospfEnabled":true,
//        "ipAddress":"10.10.137.24",
//        "ipAddressPrefixlen":24,
//        "ospfIfType":"Broadcast",
//        "localIfUsed":"10.10.137.255",
//        "area":"0.0.0.0",
//        "routerId":"1.1.1.7",
//        "networkType":"BROADCAST",
//        "cost":10,
//        "transmitDelaySecs":1,
//        "state":"DR",
//        "priority":1,
//        "mcastMemberOspfAllRouters":true,
//        "mcastMemberOspfDesignatedRouters":true,
//        "timerMsecs":10,
//        "timerDeadMSecs":40,
//        "timerWaitMSecs":40,
//        "timerRetransmit":5,
//        "timerHelloInMsecs":7208,
//        "nbrCount":0,
//        "nbrAdjacentCount":0
//      }
//    },
//   {
//      "vtss.vlan.100":{
//        "ifUp":true,
//        "ifIndex":5,
//        "mtuBytes":1500,
//        "bandwidthMbit":0,
//        "ifFlags":"<UP,BROADCAST,RUNNING,MULTICAST>",
//        "ospfEnabled":true,
//        "ipAddress":"1.2.3.24",
//        "ipAddressPrefixlen":24,
//        "ospfIfType":"Broadcast",
//        "localIfUsed":"1.2.3.255",
//        "area":"0.0.0.0",
//        "routerId":"1.1.1.7",
//        "networkType":"BROADCAST",
//        "cost":10,
//        "transmitDelaySecs":1,
//        "state":"Backup",
//        "priority":1,
//        "bdrId":"1.1.1.7",
//        "bdrAddress":"1.2.3.24",
//        "mcastMemberOspfAllRouters":true,
//        "mcastMemberOspfDesignatedRouters":true,
//        "timerMsecs":10,
//        "timerDeadMSecs":40,
//        "timerWaitMSecs":40,
//        "timerRetransmit":5,
//        "timerHelloInMsecs":4291,
//        "nbrCount":1,
//        "nbrAdjacentCount":1
//      }
//    }
//  ]
// }
template <typename KEY, typename VAL>
void serialize(vtss::expose::json::Loader &l, VectorMapSortedCb<KEY, VAL> &m)
{
    const char *b = l.pos_;
    std::string name;

    // If l.load() fails, l.pos_ will remain unchanged. Otherwise it will be
    // moved to the next token.
    CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);
    CHECK_LOAD(l.load(name), Error);
    CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);
    CHECK_LOAD(l.load(vtss::expose::json::array_start), Error);
    while (true) {
        if (l.load(vtss::expose::json::array_end)) {
            // Done
            break;
        }

        // l.load(array_end) failed, so there's an error we gotta clear
        l.reset_error();

        CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);
        KEY k {};
        CHECK_LOAD(l.load(k), Error);
        CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);
        VAL v {};
        CHECK_LOAD(l.load(v), Error);
        m.entry(k, v);
        CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);

        // Load a possible delimiter that separates array elements (,)
        (void)l.load(vtss::expose::json::delimetor);

        // And clear the error that could occur if it's not there.
        // The only tiny flaw here is that this code will still work if two
        // array elements (maps) were not separated by a comma.
        l.reset_error();
    }

    CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);
    return;

Error:
    T_EG(FRR_TRACE_GRP_OSPF, "Entire JSON:\n%s\nParser stopped at:\n%s", b, l.pos_);
    l.pos_ = b;
}

template <typename VAL>
void serialize(vtss::expose::json::Loader &l, StructValCb<VAL> &s)
{
    const char *b = l.pos_;

    VAL v {};
    CHECK_LOAD(l.load(v), Error);
    s.convert(v);
    return;
Error:
    l.pos_ = b;
}

// frr_ip_ospf_route_get ------------------------------------------------------
void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfRouteHopStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfRouteHopStatus"));

    std::string iface;
    if (m.add_leaf(iface, vtss::tag::Name("via"))) {
        s.is_connected = false;
        if (!m.add_leaf(AsIpv4(s.ip), vtss::tag::Name("ip"))) {
            s.ip = 0;
        }
    } else if (m.add_leaf(iface, vtss::tag::Name("directly attached to"))) {
        s.is_connected = true;
        s.ip = 0;
    } else {
        return;
    }

#ifndef VTSS_BASICS_STANDALONE
    vtss_ifindex_t ifindex = vtss_ifindex_from_os_ifname(iface);
    if (frr_util_ifindex_valid(ifindex)) {
        s.ifindex = ifindex;
    }
#else
    s.ifname = iface;
#endif
}

static FrrOspfRouteType parse_RouteType(const std::string &type)
{
    if (type == "N IA") {
        return RT_NetworkIA;
    }

    if (type == "D IA") {
        return RT_DiscardIA;
    }

    if (type == "N") {
        return RT_Network;
    }
    // FRR json output value is "R ", not "R"
    if (type == "R ") {
        return RT_Router;
    }

    if (type == "N E1") {
        return RT_ExtNetworkTypeOne;
    }

    if (type == "N E2") {
        return RT_ExtNetworkTypeTwo;
    }

    return RT_Network;
}

static FrrOspfRouterType parse_RouterType(const std::string &type)
{
    if (type == "abr") {
        return RouterType_ABR;
    }

    if (type == "asbr") {
        return RouterType_ASBR;
    }

    if (type == "abr asbr") {
        return RouterType_ABR_ASBR;
    }

    return RouterType_None;
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfRouteStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfRouteStatus"));

    std::string route_type;
    (void)m.add_leaf(route_type, vtss::tag::Name("routeType"));
    s.route_type = parse_RouteType(route_type);

    ADD_LEAF_DEFAULT(s.cost, vtss::tag::Name("cost"), 0);
    ADD_LEAF_DEFAULT(s.ext_cost, vtss::tag::Name("ext_cost"), 0);
    if (!m.add_leaf(AsIpv4(s.area), vtss::tag::Name("area"))) {
        s.area = 0;
    }

    ADD_LEAF_DEFAULT(s.is_ia, vtss::tag::Name("IA"), false);
    std::string router_type;
    if (m.add_leaf(router_type, vtss::tag::Name("routerType"))) {
        s.router_type = parse_RouterType(router_type);
    } else {
        s.router_type = RouterType_None;
    }
    (void)m.add_leaf(s.next_hops, vtss::tag::Name("nexthops"));
}

/* The vtss::Set operator (<) for 'APPL_FrrOspfRouteKey' */
bool operator<(const APPL_FrrOspfRouteKey &a, const APPL_FrrOspfRouteKey &b)
{
    if (a.route_type != b.route_type) {
        return a.route_type < b.route_type;
    }

    if (a.network != b.network) {
        return a.network < b.network;
    }

    if (a.area != b.area) {
        return a.area < b.area;
    }

    return a.nexthop_ip < b.nexthop_ip;
}

/* The vtss::Set operator (!=) for 'APPL_FrrOspfRouteKey' */
bool operator!=(const APPL_FrrOspfRouteKey &a, const APPL_FrrOspfRouteKey &b)
{
    if (a.route_type != b.route_type) {
        return true;
    }

    if (a.network != b.network) {
        return true;
    }

    if (a.area != b.area) {
        return true;
    }

    return a.nexthop_ip != b.nexthop_ip;
}

/* The vtss::Set operator (==) for 'APPL_FrrOspfRouteKey' */
bool operator==(const APPL_FrrOspfRouteKey &a, const APPL_FrrOspfRouteKey &b)
{
    if (a.route_type != b.route_type) {
        return false;
    }

    if (a.network != b.network) {
        return false;
    }

    if (a.area != b.area) {
        return false;
    }

    return a.nexthop_ip == b.nexthop_ip;
}

struct OspfRouteApplMapCb
    : public MapSortedCb<mesa_ipv4_network_t, Vector<FrrOspfRouteStatus>> {
    void entry(const mesa_ipv4_network_t &key,
               Vector<FrrOspfRouteStatus> &val) override
    {
        APPL_FrrOspfRouteKey k;
        APPL_FrrOspfRouteStatus v;
        for (auto &itr : val) {
            k.inst_id = 1;
            k.network = key;
            k.route_type = itr.route_type;
            k.area = itr.area;
            v.cost = itr.cost;
            v.ext_cost = itr.ext_cost;
            v.is_ia = itr.is_ia;
            v.router_type = itr.router_type;
            for (auto &next_hop : itr.next_hops) {
                k.nexthop_ip = next_hop.ip;
                v.ifindex = next_hop.ifindex;
                v.is_connected = next_hop.is_connected;
#ifdef VTSS_BASICS_STANDALONE
                v.ifname = next_hop.ifname;
#endif
                res.set(k, v);
            }
        }
    }

    explicit OspfRouteApplMapCb(FrrIpOspfRouteStatusMap &map) : res {map} {}

    FrrIpOspfRouteStatusMap &res;
};

FrrIpOspfRouteStatusMap frr_ip_ospf_route_status_parse(const std::string &s)
{
    FrrIpOspfRouteStatusMap result;
    OspfRouteApplMapCb res {result};
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(res);
    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf_route_status_get()
{
    Vector<std::string> res;
    res.push_back("show ip ospf route json");
    return res;
}

FrrIpOspfRouteStatusMapResult frr_ip_ospf_route_status_get()
{
    auto cmds = to_vty_ip_ospf_route_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_route_status_parse(vty_res);
}

// frr_ip_ospf_status_get ------------------------------------------------------
void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspfArea &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf_area"));
    ADD_LEAF_DEFAULT(s.backbone, vtss::tag::Name("backbone"), false);
    ADD_LEAF_DEFAULT(s.stub_no_summary, vtss::tag::Name("stubNoSummary"), false);
    ADD_LEAF_DEFAULT(s.stub_shortcut, vtss::tag::Name("stubShortcut"), false);
    ADD_LEAF_DEFAULT(s.nssa_translator_elected,
                     vtss::tag::Name("nssaTranslatorElected"), false);
    ADD_LEAF_DEFAULT(s.nssa_translator_always,
                     vtss::tag::Name("nssaTranslatorAlways"), false);
    (void)m.add_leaf(s.area_if_total_counter,
                     vtss::tag::Name("areaIfTotalCounter"));
    (void)m.add_leaf(s.area_if_activ_counter,
                     vtss::tag::Name("areaIfActiveCounter"));
    (void)m.add_leaf(s.full_adjancet_counter,
                     vtss::tag::Name("nbrFullAdjacentCounter"));
    (void)m.add_leaf(s.authentication, vtss::tag::Name("authentication"));
    (void)m.add_leaf(s.spf_executed_counter,
                     vtss::tag::Name("spfExecutedCounter"));
    (void)m.add_leaf(s.lsa_nr, vtss::tag::Name("lsaNumber"));
    (void)m.add_leaf(s.lsa_router_nr, vtss::tag::Name("lsaRouterNumber"));
    (void)m.add_leaf(s.lsa_router_checksum,
                     vtss::tag::Name("lsaRouterChecksum"));
    (void)m.add_leaf(s.lsa_network_nr, vtss::tag::Name("lsaNetworkNumber"));
    (void)m.add_leaf(s.lsa_network_checksum,
                     vtss::tag::Name("lsaNetworkChecksum"));
    (void)m.add_leaf(s.lsa_summary_nr, vtss::tag::Name("lsaSummaryNumber"));
    (void)m.add_leaf(s.lsa_summary_checksum,
                     vtss::tag::Name("lsaSummaryChecksum"));
    (void)m.add_leaf(s.lsa_asbr_nr, vtss::tag::Name("lsaAsbrNumber"));
    (void)m.add_leaf(s.lsa_asbr_checksum, vtss::tag::Name("lsaAsbrChecksum"));
    (void)m.add_leaf(s.lsa_nssa_nr, vtss::tag::Name("lsaNssaNumber"));
    (void)m.add_leaf(s.lsa_nssa_checksum, vtss::tag::Name("lsaNssaChecksum"));
    (void)m.add_leaf(s.lsa_opaque_link_nr,
                     vtss::tag::Name("lsaOpaqueLinkNumber"));
    (void)m.add_leaf(s.lsa_opaque_link_checksum,
                     vtss::tag::Name("lsaOpaqueLinkChecksum"));
    (void)m.add_leaf(s.lsa_opaque_area_nr,
                     vtss::tag::Name("lsaOpaqueAreaNumber"));
    (void)m.add_leaf(s.lsa_opaque_area_checksum,
                     vtss::tag::Name("lsaOpaqueAreaChecksum"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspfStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf_status"));
    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.deferred_shutdown_time.raw(),
                     vtss::tag::Name("deferredShutdownMsecs"));
    (void)m.add_leaf(s.tos_routes_only, vtss::tag::Name("tosRoutesOnly"));

    // Assign a default value to argument 'rfc2328Conform' since it is
    // an optional JSON object in FRR output.
    ADD_LEAF_DEFAULT(s.rfc2328, vtss::tag::Name("rfc2328Conform"), false);

    (void)m.add_leaf(s.spf_schedule_delay.raw(),
                     vtss::tag::Name("spfScheduleDelayMsecs"));
    (void)m.add_leaf(s.hold_time_min.raw(),
                     vtss::tag::Name("holdtimeMinMsecs"));
    (void)m.add_leaf(s.hold_time_max.raw(),
                     vtss::tag::Name("holdtimeMaxMsecs"));
    (void)m.add_leaf(s.hold_time_multiplier,
                     vtss::tag::Name("holdtimeMultplier"));  // No typo
    (void)m.add_leaf(s.spf_last_executed.raw(),
                     vtss::tag::Name("spfLastExecutedMsecs"));
    (void)m.add_leaf(s.spf_last_duration.raw(),
                     vtss::tag::Name("spfLastDurationMsecs"));
    (void)m.add_leaf(s.lsa_min_interval.raw(),
                     vtss::tag::Name("lsaMinIntervalMsecs"));
    (void)m.add_leaf(s.lsa_min_arrival.raw(),
                     vtss::tag::Name("lsaMinArrivalMsecs"));
    (void)m.add_leaf(s.write_multiplier, vtss::tag::Name("writeMultiplier"));
    (void)m.add_leaf(s.refresh_timer.raw(),
                     vtss::tag::Name("refreshTimerMsecs"));
    (void)m.add_leaf(s.lsa_external_counter,
                     vtss::tag::Name("lsaExternalCounter"));
    (void)m.add_leaf(s.lsa_external_checksum,
                     vtss::tag::Name("lsaExternalChecksum"));
    (void)m.add_leaf(s.lsa_asopaque_counter,
                     vtss::tag::Name("lsaAsopaqueCounter"));  // No typo
    (void)m.add_leaf(s.lsa_asopaque_checksums,
                     vtss::tag::Name("lsaAsOpaqueChecksum"));
    (void)m.add_leaf(s.attached_area_counter,
                     vtss::tag::Name("attachedAreaCounter"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

FrrIpOspfStatus frr_ip_ospf_status_parse(const std::string &s)
{
    FrrIpOspfStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);

#ifndef VTSS_BASICS_STANDALONE
    VTSS_TRACE(INFO) << "s " << s;
    VTSS_TRACE(INFO) << "got " << result.areas.size() << " areas";
    VTSS_TRACE(INFO) << "lsa_min_interval = " << result.lsa_min_interval.raw();
#endif

    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf_status_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf json");
    return res;
}

FrrRes<FrrIpOspfStatus> frr_ip_ospf_status_get()
{
    auto cmds = to_vty_ip_ospf_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
#ifndef VTSS_BASICS_STANDALONE
    VTSS_TRACE(INFO) << "execute cmd " << res;
#endif
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_status_parse(vty_res);
}

// frr_ip_ospf_interface_status_get---------------------------------------------
static AreaDescription parse_area(const str &id)
{
    parser::IPv4 ip_lit;
    parser::Lit stub("[Stub]");
    parser::Lit nssa("[NSSA]");

    if (frr_util_group_spaces(id, {&ip_lit, &stub})) {
        return {ip_lit.get().as_api_type(), AreaDescriptionType_Stub};
    }

    if (frr_util_group_spaces(id, {&ip_lit, &nssa})) {
        return {ip_lit.get().as_api_type(), AreaDescriptionType_Nssa};
    }

    if (frr_util_group_spaces(id, {&ip_lit})) {
        return {ip_lit.get().as_api_type(), AreaDescriptionType_Default};
    }

    return {0, AreaDescriptionType_Default};
}

// This one is called when serialize(Loader, VectorMapSortedCb) invokes
// l.load((v), Error)
void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspfIfStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf_interface_status"));
    ADD_LEAF_DEFAULT(s.if_up, vtss::tag::Name("ifUp"), false);
    (void)m.add_leaf(s.mtu_bytes, vtss::tag::Name("mtuBytes"));
    (void)m.add_leaf(s.bandwidth_mbit, vtss::tag::Name("bandwidthMbit"));
    (void)m.add_leaf(s.if_flags, vtss::tag::Name("ifFlags"));
    ADD_LEAF_DEFAULT(s.ospf_enabled, vtss::tag::Name("ospfEnabled"), false);
    if (!s.if_up) {
        return;
    }

    mesa_ipv4_network_t net;
    (void)m.add_leaf(AsIpv4(net.address), vtss::tag::Name("ipAddress"));
    (void)m.add_leaf(net.prefix_size, vtss::tag::Name("ipAddressPrefixlen"));
    s.net = net;
    (void)m.add_leaf(s.if_type, vtss::tag::Name("ospfIfType"));
    (void)m.add_leaf(AsIpv4(s.local_if_used), vtss::tag::Name("localIfUsed"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area = parse_area(buf);

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.network_type, vtss::tag::Name("networkType"));
    (void)m.add_leaf(s.cost, vtss::tag::Name("cost"));
    (void)m.add_leaf(s.transmit_delay.raw(), vtss::tag::Name("transmitDelaySecs"));
    (void)m.add_leaf(s.state, vtss::tag::Name("state"));
    (void)m.add_leaf(s.priority, vtss::tag::Name("priority"));
    (void)m.add_leaf(AsIpv4(s.bdr_id), vtss::tag::Name("bdrId"));
    (void)m.add_leaf(AsIpv4(s.bdr_address), vtss::tag::Name("bdrAddress"));
    if (!m.add_leaf(s.network_lsa_sequence,
                    vtss::tag::Name("networkLsaSequence"))) {
        s.network_lsa_sequence = 0;
    }

    if (!m.add_leaf(s.mcast_member_ospf_all_routers,
                    vtss::tag::Name("mcastMemberOspfAllRouters"))) {
        s.mcast_member_ospf_all_routers = false;
    }

    if (!m.add_leaf(s.mcast_member_ospf_designated_routers,
                    vtss::tag::Name("mcastMemberOspfDesignatedRouters"))) {
        s.mcast_member_ospf_designated_routers = false;
    }
    (void)m.add_leaf(s.timer.raw(), vtss::tag::Name("timerMsecs"));
    (void)m.add_leaf(s.timer_dead.raw(), vtss::tag::Name("timerDeadMSecs"));
    (void)m.add_leaf(s.timer_wait.raw(), vtss::tag::Name("timerWaitMSecs"));
    (void)m.add_leaf(s.timer_retransmit.raw(),
                     vtss::tag::Name("timerRetransmit"));
    (void)m.add_leaf(s.timer_hello.raw(), vtss::tag::Name("timerHelloInMsecs"));
    ADD_LEAF_DEFAULT(s.timer_passive_iface,
                     vtss::tag::Name("timerPassiveIface"), false);
    (void)m.add_leaf(s.nbr_count, vtss::tag::Name("nbrCount"));
    (void)m.add_leaf(s.nbr_adjacent_count, vtss::tag::Name("nbrAdjacentCount"));
    if (!m.add_leaf(AsIpv4(s.vlink_peer_addr), vtss::tag::Name("vlinkPeer"))) {
        s.vlink_peer_addr = 0;
    }
}

struct InterfaceMapCb : public VectorMapSortedCb<std::string, FrrIpOspfIfStatus> {
    void entry(const std::string &key, FrrIpOspfIfStatus &val) override
    {
        vtss_ifindex_t ifindex = vtss_ifindex_from_os_ifname(key);
        if (frr_util_ifindex_valid(ifindex)) {
            res.set(ifindex, std::move(val));
            return;
        }
    }

    explicit InterfaceMapCb(FrrIpOspfIfStatusMap &map) : res {map} {}

    FrrIpOspfIfStatusMap &res;
};

FrrIpOspfIfStatusMap frr_ip_ospf_interface_status_parse(const std::string &s)
{
    FrrIpOspfIfStatusMap result;
    InterfaceMapCb res {result};
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    // If not all fields are matched, no error is thrown, when _patch_mode is
    // true.
    loader.patch_mode_ = true;
    (void)loader.load(res);
    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf_interface_status_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf interface json");
    return res;
}

FrrIpOspfIfStatusMapResult frr_ip_ospf_interface_status_get()
{
    auto cmds = to_vty_ip_ospf_interface_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_interface_status_parse(vty_res);
}

// frr_ip_ospf_neighbor_status_get----------------------------------------------
void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspfNeighborStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf_neightbor_status"));
    (void)m.add_leaf(AsIpv4(s.if_address), vtss::tag::Name("ifaceAddress"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("areaId"));
    s.area = parse_area(buf);
    buf.clear();

    if (m.add_leaf(buf, vtss::tag::Name("transitAreaId"))) {
        s.transit_id = parse_area(buf);
    }

    std::string iface;
    (void)m.add_leaf(iface, vtss::tag::Name("ifaceName"));
#ifndef VTSS_BASICS_STANDALONE
    vtss_ifindex_t ifindex = vtss_ifindex_from_os_ifname(iface);
    if (frr_util_ifindex_valid(ifindex)) {
        s.ifindex = ifindex;
    }
#endif

    (void)m.add_leaf(s.nbr_priority, vtss::tag::Name("nbrPriority"));
    (void)m.add_leaf(s.nbr_state, vtss::tag::Name("nbrState"));
    (void)m.add_leaf(AsIpv4(s.dr_ip_addr),
                     vtss::tag::Name("routerDesignatedId"));
    (void)m.add_leaf(AsIpv4(s.bdr_ip_addr),
                     vtss::tag::Name("routerDesignatedBackupId"));
    (void)m.add_leaf(s.options_list, vtss::tag::Name("optionsList"));
    (void)m.add_leaf(s.options_counter, vtss::tag::Name("optionsCounter"));
    (void)m.add_leaf(s.router_dead_interval_timer_due.raw(),
                     vtss::tag::Name("routerDeadIntervalTimerDueMsec"));
}

FrrIpOspfNbrStatusMap frr_ip_ospf_neighbor_status_parse(const std::string &s)
{
    FrrIpOspfNbrStatusMap result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf_neighbor_status_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf neighbor detail all json");
    return res;
}

FrrIpOspfNbrStatusMapResult frr_ip_ospf_neighbor_status_get()
{
    auto cmds = to_vty_ip_ospf_neighbor_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_neighbor_status_parse(vty_res);
}

// frr_ip_ospf_db_get ------------------------------------------------------
static int32_t parse_external(const str &id)
{
    str first("E1");
    str second("E2");

    if (id == first) {
        return 1;
    } else if (id == second) {
        return 2;
    }

    return 1;
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbLinkVal &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbLinkVal"));

    (void)m.add_leaf(AsIpv4(s.link_id), vtss::tag::Name("id"));
    (void)m.add_leaf(AsIpv4(s.adv_router), vtss::tag::Name("router"));

    (void)m.add_leaf(s.age, vtss::tag::Name("age"));

    uint64_t tmp;
    (void)m.add_leaf(AsCounter(tmp), vtss::tag::Name("seq"));
    s.sequence = (uint32_t)tmp;

    (void)m.add_leaf(s.checksum, vtss::tag::Name("checksum"));

    // only for type 1
    (void)m.add_leaf(s.router_link_count, vtss::tag::Name("link"));

    // only for type 3
    mesa_ipv4_network_t net;
    (void)m.add_leaf(AsIpv4(net.address), vtss::tag::Name("prefix"));
    (void)m.add_leaf(net.prefix_size, vtss::tag::Name("prefix_len"));
    s.summary_route = net;

    // only for type 5 and type 7
    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("external"));
    s.external = parse_external(buf);
    (void)m.add_leaf(s.tag, vtss::tag::Name("tag"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbLinkStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbLinkStates"));

    (void)m.add_leaf(s.type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.desc, vtss::tag::Name("desc"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.links, vtss::tag::Name("links"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbStatus"));
    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));

    (void)m.add_leaf(s.area, vtss::tag::Name("areas"));

    (void)m.add_leaf(s.type5, vtss::tag::Name("AS External Link States"));
    (void)m.add_leaf(s.type7, vtss::tag::Name("NSSA-external Link States"));
}

/* The vtss::Set operator (<) for 'APPL_FrrOspfDbKey' */
bool operator<(const APPL_FrrOspfDbKey &a, const APPL_FrrOspfDbKey &b)
{
    if (a.area_id != b.area_id) {
        return a.area_id < b.area_id;
    }

    if (a.type != b.type) {
        return a.type < b.type;
    }

    if (a.link_id != b.link_id) {
        return a.link_id < b.link_id;
    }

    return a.adv_router < b.adv_router;
}

/* The vtss::Set operator (==) for 'APPL_FrrOspfDbKey' */
bool operator==(const APPL_FrrOspfDbKey &a, const APPL_FrrOspfDbKey &b)
{
    if (a.area_id != b.area_id) {
        return false;
    }

    if (a.type != b.type) {
        return false;
    }

    if (a.link_id != b.link_id) {
        return false;
    }

    return a.adv_router == b.adv_router;
}

/* The vtss::Set operator (!=) for 'APPL_FrrOspfDbKey' */
bool operator!=(const APPL_FrrOspfDbKey &a, const APPL_FrrOspfDbKey &b)
{
    return !(a == b);
}

struct OspfDbLinkStateConvertCb : public StructValCb<FrrOspfDbStatus> {
    void convert(const FrrOspfDbStatus &val) override
    {
        // area
        for (auto &area : val.area) {
            for (auto &link : area.links) {
                APPL_FrrOspfDbKey k;
                APPL_FrrOspfDbLinkStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id.area;
                k.type = area.type;
                k.link_id = link.link_id;
                k.adv_router = link.adv_router;

                v.router_id = val.router_id;
                v.age = link.age;
                v.sequence = link.sequence;
                v.checksum = link.checksum;
                v.router_link_count = link.router_link_count;
                v.summary_route = link.summary_route;
                v.external = link.external;
                v.tag = link.tag;

                res.set(k, v);
            }
        }

        // for type 5
        for (auto &link : val.type5.links) {
            APPL_FrrOspfDbKey key_type5;
            APPL_FrrOspfDbLinkStateVal val_type5;

            key_type5.inst_id = 1;
            key_type5.area_id = 0xffffffff;  // type 5 don't use area id as key.
            key_type5.type = val.type5.type;
            key_type5.link_id = link.link_id;
            key_type5.adv_router = link.adv_router;

            val_type5.router_id = val.router_id;
            val_type5.age = link.age;
            val_type5.sequence = link.sequence;
            val_type5.checksum = link.checksum;
            val_type5.router_link_count = link.router_link_count;
            val_type5.summary_route = link.summary_route;
            val_type5.external = link.external;
            val_type5.tag = link.tag;

            res.set(key_type5, val_type5);
        }

        // for type 7
        for (auto &link : val.type7.links) {
            APPL_FrrOspfDbKey key_type7;
            APPL_FrrOspfDbLinkStateVal val_type7;

            key_type7.inst_id = 1;
            key_type7.area_id = 0xffffffff;  // type 7 don't use area id as key.
            key_type7.type = val.type7.type;
            key_type7.link_id = link.link_id;
            key_type7.adv_router = link.adv_router;

            val_type7.router_id = val.router_id;
            val_type7.age = link.age;
            val_type7.sequence = link.sequence;
            val_type7.checksum = link.checksum;
            val_type7.router_link_count = link.router_link_count;
            val_type7.summary_route = link.summary_route;
            val_type7.external = link.external;
            val_type7.tag = link.tag;

            res.set(key_type7, val_type7);
        }
    }

    vtss::Map<APPL_FrrOspfDbKey, APPL_FrrOspfDbLinkStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbStatus, and return it
 */
FrrOspfDbStatus frr_ip_ospf_db_parse_raw(const std::string &s)
{
    FrrOspfDbStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbStatus, if you want to
   return the sorted MAP data, you can use frr_ip_ospf_db_get() */
FrrRes<FrrOspfDbStatus> frr_ip_ospf_db_get_pure(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbKey, APPL_FrrOspfDbLinkStateVal> frr_ip_ospf_db_parse(
    const std::string &s)
{
    OspfDbLinkStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbKey, APPL_FrrOspfDbLinkStateVal>> frr_ip_ospf_db_get(
                                                            const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_parse(vty_res);
}

// frr_ip_ospf_db_router_get
// ------------------------------------------------------

template <typename MAP, typename DBTYPE>
void frr_ospf_db_common_params(MAP &m, DBTYPE &dbtype)
{
    (void)m.add_leaf(dbtype.age, vtss::tag::Name("lsAge"));
    (void)m.add_leaf(dbtype.options, vtss::tag::Name("optionsList"));
    (void)m.add_leaf(dbtype.type, vtss::tag::Name("lsType"));
    (void)m.add_leaf(AsIpv4(dbtype.link_state_id),
                     vtss::tag::Name("linkStateId"));
    (void)m.add_leaf(AsIpv4(dbtype.adv_router), vtss::tag::Name("advRouter"));

    uint64_t tmp;
    (void)m.add_leaf(AsCounter(tmp), vtss::tag::Name("sequence"));
    dbtype.sequence = (uint32_t)tmp;

    (void)m.add_leaf(dbtype.checksum, vtss::tag::Name("checksum"));
    (void)m.add_leaf(dbtype.length, vtss::tag::Name("length"));
}

static int32_t parse_linkConnectedTo(const std::string &type)
{
    if (type == "another Router (point-to-point)") {
        return 1;
    }

    if (type == "a Transit Network") {
        return 2;
    }

    if (type == "Stub Network") {
        return 3;
    }

    if (type == "a Virtual Link") {
        return 4;
    }

    return 0;
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbRouterAreasRoutesLinksVal &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbRouterAreasRoutesLinksVal"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("linkConnectedTo"));
    s.link_connected_to = parse_linkConnectedTo(buf);

    (void)m.add_leaf(AsIpv4(s.link_id), vtss::tag::Name("linkID"));
    (void)m.add_leaf(AsIpv4(s.link_data), vtss::tag::Name("linkData"));

    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbRouterAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbRouterAreasRoutesStates"));

    frr_ospf_db_common_params(m, s);

    (void)m.add_leaf(s.links, vtss::tag::Name("links"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbRouterAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbRouterAreasStates"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbRouterStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbRouterStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

/* The vtss::Set operator (<) for 'APPL_FrrOspfDbCommonKey' */
bool operator<(const APPL_FrrOspfDbCommonKey &a,
               const APPL_FrrOspfDbCommonKey &b)
{
    if (a.area_id != b.area_id) {
        return a.area_id < b.area_id;
    }

    if (a.type != b.type) {
        return a.type < b.type;
    }

    if (a.link_state_id != b.link_state_id) {
        return a.link_state_id < b.link_state_id;
    }

    return a.adv_router < b.adv_router;
}

/* The vtss::Set operator (==) for 'APPL_FrrOspfDbCommonKey' */
bool operator==(const APPL_FrrOspfDbCommonKey &a,
                const APPL_FrrOspfDbCommonKey &b)
{
    if (a.area_id != b.area_id) {
        return false;
    }

    if (a.type != b.type) {
        return false;
    }

    if (a.link_state_id != b.link_state_id) {
        return false;
    }

    return a.adv_router == b.adv_router;
}

/* The vtss::Set operator (!=) for 'APPL_FrrOspfDbCommonKey' */
bool operator!=(const APPL_FrrOspfDbCommonKey &a,
                const APPL_FrrOspfDbCommonKey &b)
{
    return !(a == b);
}

struct OspfDbRouterStateConvertCb : public StructValCb<FrrOspfDbRouterStatus> {
    void convert(const FrrOspfDbRouterStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspfDbCommonKey k;
                APPL_FrrOspfDbRouterStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id.area;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.links = route.links;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbRouterStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_router_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database router json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbRouterStatus, and
 * return it */
FrrOspfDbRouterStatus frr_ip_ospf_db_router_parse_raw(const std::string &s)
{
    FrrOspfDbRouterStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbRouterStatus, if you
   want to
   return the sorted MAP data, you can use frr_ip_ospf_db_router_get() */
FrrRes<FrrOspfDbRouterStatus> frr_ip_ospf_db_router_get_pure(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_router_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_router_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbRouterStateVal>
frr_ip_ospf_db_router_parse(const std::string &s)
{
    OspfDbRouterStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbRouterStateVal>>
                                                                frr_ip_ospf_db_router_get(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_router_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_router_parse(vty_res);
}

// frr_ip_ospf_db_net_get ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbNetAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbNetAreasRoutesStates"));

    frr_ospf_db_common_params(m, s);

    (void)m.add_leaf(s.network_mask, vtss::tag::Name("networkMask"));

    mesa_ipv4_t tmp_router;
    char str_buf[128];
    int count = 1;

    while (count < 9) {
        sprintf(str_buf, "attachedRouter%d", count);
        if (m.add_leaf(AsIpv4(tmp_router), vtss::tag::Name(str_buf))) {
            s.attached_router.push_back(tmp_router);
        }

        count++;
    }
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbNetAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbNetAreasStates"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbNetStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbNetStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct OspfDbNetStateConvertCb : public StructValCb<FrrOspfDbNetStatus> {
    void convert(const FrrOspfDbNetStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspfDbCommonKey k;
                APPL_FrrOspfDbNetStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id.area;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.network_mask = route.network_mask;
                v.attached_router = route.attached_router;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNetStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_network_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database network json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbNetStatus, and return
 * it */
FrrOspfDbNetStatus frr_ip_ospf_db_net_parse_raw(const std::string &s)
{
    FrrOspfDbNetStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbNetStatus, if you want
   to
   return the sorted MAP data, you can use frr_ip_ospf_db_net_get() */
FrrRes<FrrOspfDbNetStatus> frr_ip_ospf_db_net_get_pure(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_network_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_net_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNetStateVal> frr_ip_ospf_db_net_parse(
    const std::string &s)
{
    OspfDbNetStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNetStateVal>>
                                                             frr_ip_ospf_db_net_get(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_network_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_net_parse(vty_res);
}

// frr_ip_ospf_db_summary_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbSummaryAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbSummaryAreasRoutesStates"));

    frr_ospf_db_common_params(m, s);

    (void)m.add_leaf(s.network_mask, vtss::tag::Name("networkMask"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbSummaryAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbSummaryAreasStates"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbSummaryStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbSummaryStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct OspfDbSummaryStateConvertCb : public StructValCb<FrrOspfDbSummaryStatus> {
    void convert(const FrrOspfDbSummaryStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspfDbCommonKey k;
                APPL_FrrOspfDbSummaryStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id.area;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.network_mask = route.network_mask;
                v.metric = route.metric;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbSummaryStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_summary_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database summary json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbSummaryStatus, and
 * return it */
FrrOspfDbSummaryStatus frr_ip_ospf_db_summary_parse_raw(const std::string &s)
{
    FrrOspfDbSummaryStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbSummaryStatus, if you
   want to
   return the sorted MAP data, you can use frr_ip_ospf_db_summary_get() */
FrrRes<FrrOspfDbSummaryStatus> frr_ip_ospf_db_summary_get_pure(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_summary_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_summary_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbSummaryStateVal>
frr_ip_ospf_db_summary_parse(const std::string &s)
{
    OspfDbSummaryStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbSummaryStateVal>> frr_ip_ospf_db_summary_get(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_summary_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_summary_parse(vty_res);
}

// frr_ip_ospf_db_asbr_summary_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbASBRSummaryAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m = l.as_map(
                                              vtss::tag::Typename("FrrOspfDbASBRSummaryAreasRoutesStates"));

    frr_ospf_db_common_params(m, s);

    (void)m.add_leaf(s.network_mask, vtss::tag::Name("networkMask"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbASBRSummaryAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbASBRSummaryAreasStates"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbASBRSummaryStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbASBRSummaryStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct OspfDbASBRSummaryStateConvertCb
    : public StructValCb<FrrOspfDbASBRSummaryStatus> {
    void convert(const FrrOspfDbASBRSummaryStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspfDbCommonKey k;
                APPL_FrrOspfDbASBRSummaryStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id.area;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.network_mask = route.network_mask;
                v.metric = route.metric;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbASBRSummaryStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_asbr_summary_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database asbr-summary json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbASBRSummaryStatus,
 * and return it */
FrrOspfDbASBRSummaryStatus frr_ip_ospf_db_asbr_summary_parse_raw(
    const std::string &s)
{
    FrrOspfDbASBRSummaryStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbASBRSummaryStatus, if
   you want to
   return the sorted MAP data, you can use frr_ip_ospf_db_asbr_summary_get() */
FrrRes<FrrOspfDbASBRSummaryStatus> frr_ip_ospf_db_asbr_summary_get_pure(
    const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_asbr_summary_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_asbr_summary_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbASBRSummaryStateVal>
frr_ip_ospf_db_asbr_summary_parse(const std::string &s)
{
    OspfDbASBRSummaryStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbASBRSummaryStateVal>>
                                                                     frr_ip_ospf_db_asbr_summary_get(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_asbr_summary_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_asbr_summary_parse(vty_res);
}

// frr_ip_ospf_db_external_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbExternalAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbExternalAreasRoutesStates"));

    frr_ospf_db_common_params(m, s);

    (void)m.add_leaf(s.network_mask, vtss::tag::Name("networkMask"));
    (void)m.add_leaf(s.metric_type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
    (void)m.add_leaf(AsIpv4(s.forward_address),
                     vtss::tag::Name("forwardAddress"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbExternalRouteStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbExternalRouteStatus"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspfDbExternalStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbExternalStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.type5, vtss::tag::Name("AS External Link States"));
}

struct OspfDbExternalStateConvertCb
    : public StructValCb<FrrOspfDbExternalStatus> {
    void convert(const FrrOspfDbExternalStatus &val) override
    {
        // for type 5
        for (auto &type : val.type5) {
            for (auto &route : type.routes) {
                APPL_FrrOspfDbCommonKey key_type5;
                APPL_FrrOspfDbExternalStateVal val_type5;

                key_type5.inst_id = 1;
                key_type5.area_id =
                    0xffffffff;  // type 5 don't use area id as key.
                key_type5.type = route.type;
                key_type5.link_state_id = route.link_state_id;
                key_type5.adv_router = route.adv_router;

                val_type5.router_id = val.router_id;
                val_type5.age = route.age;
                val_type5.options = route.options;
                val_type5.sequence = route.sequence;
                val_type5.checksum = route.checksum;
                val_type5.length = route.length;

                val_type5.network_mask = route.network_mask;
                val_type5.metric_type = route.metric_type;
                val_type5.metric = route.metric;
                val_type5.forward_address = route.forward_address;

                res.set(key_type5, val_type5);
            }
        }
    }

    vtss::Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbExternalStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_external_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database external json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbExternalStatus, and
 * return it */
FrrOspfDbExternalStatus frr_ip_ospf_db_external_parse_raw(const std::string &s)
{
    FrrOspfDbExternalStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbExternalStatus, if you
   want to
   return the sorted MAP data, you can use frr_ip_ospf_db_external_get() */
FrrRes<FrrOspfDbExternalStatus> frr_ip_ospf_db_external_get_pure(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_external_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbExternalStateVal>
frr_ip_ospf_db_external_parse(const std::string &s)
{
    OspfDbExternalStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbExternalStateVal>>
                                                                  frr_ip_ospf_db_external_get(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_external_parse(vty_res);
}

// frr_ip_ospf_db_nssa_external_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbNSSAExternalAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m = l.as_map(
                                              vtss::tag::Typename("FrrOspfDbNSSAExternalAreasRoutesStates"));

    frr_ospf_db_common_params(m, s);

    (void)m.add_leaf(s.network_mask, vtss::tag::Name("networkMask"));
    (void)m.add_leaf(s.metric_type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
    (void)m.add_leaf(AsIpv4(s.forward_address),
                     vtss::tag::Name("forwardAddress"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbNSSAExternalAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbNSSAExternalAreasStates"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspfDbNSSAExternalStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspfDbNSSAExternalStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));

    //(void)m.add_leaf(s.type5, vtss::tag::Name("AS NSSAExternal Link States"));
}

struct OspfDbNSSAExternalStateConvertCb
    : public StructValCb<FrrOspfDbNSSAExternalStatus> {
    void convert(const FrrOspfDbNSSAExternalStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspfDbCommonKey k;
                APPL_FrrOspfDbNSSAExternalStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id.area;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;

                v.network_mask = route.network_mask;
                v.metric_type = route.metric_type;
                v.metric = route.metric;
                v.forward_address = route.forward_address;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNSSAExternalStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf_db_nssa_external_get()
{
    Vector<std::string> res;
    res.emplace_back("show ip ospf database nssa-external json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspfDbNSSAExternalStatus,
 * and
 * return it */
FrrOspfDbNSSAExternalStatus frr_ip_ospf_db_nssa_external_parse_raw(
    const std::string &s)
{
    FrrOspfDbNSSAExternalStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspfDbNSSAExternalStatus, if
   you
   want to
   return the sorted MAP data, you can use frr_ip_ospf_db_nssa_external_get() */
FrrRes<FrrOspfDbNSSAExternalStatus> frr_ip_ospf_db_nssa_external_get_pure(
    const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_nssa_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_nssa_external_parse_raw(vty_res);
}

Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNSSAExternalStateVal>
frr_ip_ospf_db_nssa_external_parse(const std::string &s)
{
    OspfDbNSSAExternalStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNSSAExternalStateVal>>
                                                                      frr_ip_ospf_db_nssa_external_get(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf_db_nssa_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf_db_nssa_external_parse(vty_res);
}

// OSPF Router configuration ---------------------------------------------------
Vector<std::string> to_vty_ospf_router_conf_set()
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("no router ospf");
    res.push_back("router ospf");
    return res;
}

mesa_rc frr_ospf_router_conf_set(const OspfInst &id)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_conf_set();
    std::string vty_res;

    if (frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Here we enable SPF throttling to fix APPL-207.
     * Values 200(ms),400(ms),10000(ms) is referenced from the example of FRR
     * user guide.
     * TODO: The UI/APPL layer of SPF throttling is not implemented yet.
     */
    return frr_ospf_router_spf_throttling_conf_set(id, {200, 400, 10000});
}

static std::string abr_type_to_str(AbrType type)
{
    switch (type) {
    case AbrType_Cisco:
        return "cisco";
    case AbrType_IBM:
        return "ibm";
    case AbrType_Shortcut:
        return "shortcut";
    case AbrType_Standard:
        return "standard";
    default:
        return "cisco";
    }
}

Vector<std::string> to_vty_ospf_router_conf_set(const FrrOspfRouterConf &conf)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router ospf");
    StringStream buf;
    if (conf.ospf_router_id.valid()) {
        buf << "ospf router-id " << Ipv4Address(conf.ospf_router_id.get());
        res.emplace_back(vtss::move(buf.buf));
        buf.clear();
    }

    if (conf.ospf_router_rfc1583.valid()) {
        buf << (conf.ospf_router_rfc1583.get() ? "" : "no ")
            << "ospf rfc1583compatibility";
        res.emplace_back(vtss::move(buf.buf));
        buf.clear();
    }

    if (conf.ospf_router_abr_type.valid()) {
        buf << "ospf abr-type "
            << abr_type_to_str(conf.ospf_router_abr_type.get());
        res.emplace_back(vtss::move(buf.buf));
        buf.clear();
    }

    return res;
}

mesa_rc frr_ospf_router_conf_set(const OspfInst &id,
                                 const FrrOspfRouterConf &conf)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_conf_set(conf);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_router_conf_del()
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("no router ospf");
    return res;
}

mesa_rc frr_ospf_router_conf_del(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    /* APPL-1474: The passive interface configuration isn't removed
     *            after OSPF router mode is disabled.
     *
     * Background:
     * When the OSPF router mode is disabled, all router mode related
     * commands are expected to be removed but the passive interface
     * configuration doesn't. It is because the configuration is stored
     * in the interface data structure.
     * See the details in source file '\frr\ospf\ospf_interface.h' (refer to
     * FRR v4.0)
     * #45  struct ospf_if_params {
     *          ...
     * #61      DECLARE_IF_PARAM(u_char, passive_interface);
     *          ...
     *      }
     *
     * Solution:
     * In order to perform the same behavior as others router mode commands.
     * We restore the passive interface configuration before the OSPF router
     * mode is disabled.
     */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));
    frr_ospf_router_passive_if_disable_all(frr_running_conf, id);

    auto cmds = to_vty_ospf_router_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfRouter : public FrrUtilsConfStreamParserCB {
private:
    bool try_parse_id(const str &line)
    {
        parser::Lit lit_ospf("ospf");
        parser::Lit lit_router("router-id");
        parser::IPv4 router_id;
        if (frr_util_group_spaces(line, {&lit_ospf, &lit_router, &router_id})) {
            res.ospf_router_id = router_id.get().as_api_type();
            return true;
        }

        return false;
    }

    bool try_parse_rfc(const str &line)
    {
        parser::Lit lit_compatible("compatible");
        parser::Lit lit_rfc("rfc1583");
        if (frr_util_group_spaces(line, {&lit_compatible, &lit_rfc})) {
            res.ospf_router_rfc1583 = true;
            return true;
        }

        return false;
    }

    AbrType abr_parser(const std::string &type)
    {
        if (type == "cisco") {
            return AbrType_Cisco;
        }

        if (type == "ibm") {
            return AbrType_IBM;
        }

        if (type == "shortcut") {
            return AbrType_Shortcut;
        }

        if (type == "standard") {
            return AbrType_Standard;
        }

        return AbrType_Cisco;
    }

    bool try_parse_abr(const str &line)
    {
        parser::Lit lit_ospf("ospf");
        parser::Lit lit_abr("abr-type");
        parser::OneOrMore<FrrUtilsEatAll> lit_type;
        if (frr_util_group_spaces(line, {&lit_ospf, &lit_abr, &lit_type})) {
            res.ospf_router_abr_type = abr_parser(
                                           std::string(lit_type.get().begin(), lit_type.get().end()));
            return true;
        }

        return false;
    }

public:
    void router(const std::string &name, const str &line) override
    {
        if (try_parse_id(line)) {
            return;
        }

        if (try_parse_abr(line)) {
            return;
        }

        if (try_parse_rfc(line)) {
            return;
        }
    }

    FrrOspfRouterConf res;
};

FrrRes<FrrOspfRouterConf> frr_ospf_router_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfRouter cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// OSPF Router Passive interface configuration ---------------------------------
Vector<std::string> to_vty_ospf_router_passive_if_default(bool enable)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router ospf");
    StringStream buf;
    buf << (enable ? "" : "no ") << "passive-interface default";
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_passive_if_default_set(const OspfInst &id, bool enable)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_passive_if_default(enable);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfPassiveIfDefault : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit lit_passive_default("passive-interface default");
        if (frr_util_group_spaces(line, {&lit_passive_default})) {
            res = true;
        }
    }

    bool res = {false};
};

FrrRes<bool> frr_ospf_router_passive_if_default_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfPassiveIfDefault cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

Vector<std::string> to_vty_ospf_router_passive_if_conf_set(const std::string &ifname,
                                                           bool passive)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router ospf");
    StringStream buf;
    buf << (passive ? "" : "no ") << "passive-interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_passive_if_conf_set(const OspfInst &id,
                                            const vtss_ifindex_t &i,
                                            bool passive)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds =
        to_vty_ospf_router_passive_if_conf_set(interface_name.val, passive);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfPassiveIf : public FrrUtilsConfStreamParserCB {
    FrrParseOspfPassiveIf(const std::string &name, bool enabled)
        : if_name {name}, default_enabled {enabled}, res {enabled} {}

    void router(const std::string &name, const str &line) override
    {
        parser::Lit lit_passive(default_enabled ? "no passive-interface"
                                : "passive-interface");
        parser::OneOrMore<FrrUtilsEatAll> lit_name;
        if (frr_util_group_spaces(line, {&lit_passive, &lit_name})) {
            if (vtss::equal(lit_name.get().begin(), lit_name.get().end(),
                            if_name.begin()) &&
                (size_t)(lit_name.get().end() - lit_name.get().begin()) ==
                if_name.size()) {
                res = !default_enabled;
            }
        }
    }

    const std::string &if_name;
    const bool default_enabled;
    bool res;
};

FrrRes<bool> frr_ospf_router_passive_if_conf_get(std::string &running_conf, const OspfInst &id, const vtss_ifindex_t &ifindex)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> if_name = frr_util_os_ifname_get(ifindex);
    if (!if_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfPassiveIfDefault cb_default;
    frr_util_conf_parser(running_conf, cb_default);
    FrrParseOspfPassiveIf cb(if_name.val, cb_default.res);
    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

struct FrrParseOspfDisableAllPassiveIf : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit lit_no("no");
        parser::Lit lit_passive("passive-interface");
        parser::Lit lit_default("default");
        parser::OneOrMore<FrrUtilsEatAll> lit_if_name;
        StringStream str_buf;
        bool is_found = false;

        /* Convert to the opposite command by adding/removing no form */
        if (frr_util_group_spaces(line, {&lit_passive, &lit_if_name})) {
            str_buf << "no " << lit_passive.get() << " " << lit_if_name.get();
            is_found = true;
        } else if (frr_util_group_spaces(line, {&lit_no, &lit_passive, &lit_if_name})) {
            is_found = true;
            str_buf << lit_passive.get() << " " << lit_if_name.get();
        } else if (frr_util_group_spaces(line, {&lit_passive, &lit_default})) {
            is_found = true;
            str_buf << "no " << lit_passive.get() << " " << lit_default.get();
        }

        /* Add the opposite command */
        if (is_found) {
            // Add the initial commands to enter the OSPF router mode
            if (!disable_cmds.size()) {
                disable_cmds.emplace_back("configure terminal");
                disable_cmds.emplace_back("router ospf");
            }

            disable_cmds.emplace_back(vtss::move(str_buf.buf));
        }
    }

    explicit FrrParseOspfDisableAllPassiveIf(Vector<std::string> &v)
        : disable_cmds {v} {}

    Vector<std::string> &disable_cmds;
};

Vector<std::string> to_vty_ospf_router_passive_if_disable_all(const std::string &conf)
{
    Vector<std::string> res;
    FrrParseOspfDisableAllPassiveIf parse_cb(res);
    frr_util_conf_parser(conf, parse_cb);
    return res;
}

mesa_rc frr_ospf_router_passive_if_disable_all(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    Vector<std::string> disable_cmds;
    disable_cmds = vtss::move(to_vty_ospf_router_passive_if_disable_all(conf));

    // Apply the disable commands if needed
    if (disable_cmds.size()) {
        // Debug message
        for (const auto &cmd : disable_cmds) {
            VTSS_TRACE(DEBUG) << cmd;
        }

        std::string vty_res;
        return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, disable_cmds, vty_res);
    }

    return VTSS_RC_OK;
}

// frr_ospf_router_default_metric_conf
Vector<std::string> to_vty_ospf_router_default_metric_conf_set(uint32_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "default-metric " << val;
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_default_metric_conf_set(const OspfInst &id, uint32_t val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_default_metric_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_router_default_metric_conf_del()
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    res.emplace_back("no default-metric");
    return res;
}

mesa_rc frr_ospf_router_default_metric_conf_del(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_default_metric_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfRouterDefaultMetric : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit default_lit("default-metric");
        parser::IntUnsignedBase10<uint32_t, 1, 9> val;
        if (frr_util_group_spaces(line, {&default_lit, &val})) {
            res = val.get();
        }
    }

    Optional<uint32_t> res;
};

FrrRes<Optional<uint32_t>> frr_ospf_router_default_metric_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfRouterDefaultMetric cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_router_distribute_conf
std::string metric_type_to_string(FrrOspfRouterMetricType val)
{
    switch (val) {
    case MetricType_One:
        return "1";
    case MetricType_Two:
        return "2";
    }

    VTSS_ASSERT(false);
    return "";
}

Vector<std::string> to_vty_ospf_router_redistribute_conf_set(
    const FrrOspfRouterRedistribute &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "redistribute " << ip_util_route_protocol_to_str(val.protocol, false);

    if (val.metric.valid()) {
        buf << " metric " << val.metric.get();
    } else if (val.route_map.valid()) {
        buf << " route-map " << val.route_map.get();
    }

    buf << " metric-type " << metric_type_to_string(val.metric_type);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_redistribute_conf_set(const OspfInst &id, const FrrOspfRouterRedistribute &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_redistribute_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no redistribute " << ip_util_route_protocol_to_str(val, false);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_redistribute_conf_del(const OspfInst &id, vtss_appl_ip_route_protocol_t val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_redistribute_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfRouterRedistribute : public FrrUtilsConfStreamParserCB {
    void parse_protocol(const std::string &protocol, const str &line)
    {
        parser::Lit redistribute_lit {"redistribute"};
        parser::Lit protocol_lit {protocol};

        // Command output 'metric <0-16777214>' are optional arguments
        // Command output 'metric-type { 1 | 2 }' are optional arguments
        // Command output 'route-map <WORD>' are optional arguments
        // We assign an invalid value when the optional argument are not
        // existing in the command output that we don't need to parse various
        // combined conditions.

        // Notice that the usage of second/third arguments in
        // IntUnsignedBase10<TYPE, MIN, MAX>
        // The <MIN, MAX> means input character length, not valid range value.
        // In this case, the valid metric range is 0-16777214, so the valid
        // character input length is 1-8.
        parser::TagValue<parser::IntUnsignedBase10<uint32_t, 1, 8>, int> metric(
            "metric", 16777214 + 1);  // plus 1 to become an invalid value
        parser::TagValue<parser::IntUnsignedBase10<uint8_t, 1, 1>, int> metric_type(
            "metric-type", 2 + 1);  // plus 1 to become an invalid value
        parser::TagValue<parser::OneOrMore<FrrUtilsEatAll>> route_map("route-map");

        if (frr_util_group_spaces(line, {&redistribute_lit, &protocol_lit},
    {&metric, &metric_type, &route_map})) {
            FrrOspfRouterRedistribute redistribute;
            redistribute.protocol = frr_util_route_protocol_from_str(protocol);

            if (metric.get().get() != 16777215) {
                redistribute.metric = metric.get().get();
            }

            redistribute.metric_type = metric_type.get().get() == 1
                                       ? MetricType_One
                                       : MetricType_Two;

            if (route_map.get().get().begin() != route_map.get().get().end()) {
                redistribute.route_map =
                    std::string(route_map.get().get().begin(),
                                route_map.get().get().end());
            }

            res.push_back(vtss::move(redistribute));
        }
    }

    void router(const std::string &name, const str &line) override
    {
        for (auto protocol : protocols) {
            parse_protocol(protocol, line);
        }
    }

    const Vector<std::string> protocols {"kernel", "connected", "static", "rip",
        "bgp"
    };
    Vector<FrrOspfRouterRedistribute> res;
};

Vector<FrrOspfRouterRedistribute> frr_ospf_router_redistribute_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) return Vector<FrrOspfRouterRedistribute> {};
    FrrParseOspfRouterRedistribute cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF administrative distance
//----------------------------------------------------------------------------
struct FrrParseOspfAdminDistanceCb : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit distance_lit("distance");
        parser::IntUnsignedBase10<uint8_t, 1, 3> distance_val;
        if (frr_util_group_spaces(line, {&distance_lit, &distance_val})) {
            res = distance_val.get();
        }
    }

    uint8_t res = 110;  // OSPF default distance
};

FrrRes<uint8_t> frr_ospf_router_admin_distance_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfAdminDistanceCb cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

Vector<std::string> to_vty_ospf_router_admin_distance_set(uint8_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "distance " << val;
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_admin_distance_set(const OspfInst &id, uint8_t val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_admin_distance_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

//----------------------------------------------------------------------------
//** OSPF default route configuration
//----------------------------------------------------------------------------
// frr_ospf_router_default_route
Vector<std::string> to_vty_ospf_router_default_route_conf_set(
    const FrrOspfRouterDefaultRoute &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "default-information originate";
    if (val.always.valid() && val.always.get()) {
        buf << " always";
    }

    if (val.metric.valid()) {
        buf << " metric " << val.metric.get();
    }

    if (val.metric_type.valid()) {
        buf << " metric-type " << metric_type_to_string(val.metric_type.get());
    }

    if (val.route_map.valid()) {
        buf << " route-map " << val.route_map.get();
    }

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_router_default_route_conf_set(
    const OspfInst &id, const FrrOspfRouterDefaultRoute &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_default_route_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_router_default_route_conf_del()
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    res.emplace_back("no default-information originate");
    return res;
}

mesa_rc frr_ospf_router_default_route_conf_del(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_default_route_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfRouterDefaultRoute : public FrrUtilsConfStreamParserCB {
    /* Command output 'metric <0-16777214>' are optional arguments
     * Command output 'metric-type { 1 | 2 }' are optional arguments
     * Command output 'route-map <WORD>' are optional arguments
     * We assign an invalid value when the optional argument are not
     * existing in the command output that we don't need to parse various
     * combined conditions.
     */
    void parser_leftovers(
        /* Notice that the usage of second/third arguments in
         * IntUnsignedBase10<TYPE, MIN, MAX>
         * The <MIN, MAX> means input character length, not valid range
         * value.
         * In this case, the valid metric range is 0-16777214, so the valid
         * character input length is 1-8.
         */
        const parser::TagValue<parser::IntUnsignedBase10<uint32_t, 1, 8>, int>
        &metric,
        const parser::TagValue<parser::IntUnsignedBase10<uint8_t, 1, 1>, int>
        &metric_type,
        const parser::TagValue<parser::OneOrMore<FrrUtilsEatAll>> &route_map)
    {
        if (metric.get().get() != 16777215) {
            res.metric = metric.get().get();
        }

        res.metric_type =
            metric_type.get().get() == 1 ? MetricType_One : MetricType_Two;

        if (route_map.get().get().begin() != route_map.get().get().end()) {
            res.route_map = std::string(route_map.get().get().begin(),
                                        route_map.get().get().end());
        }
    }

    void router(const std::string &name, const str &line) override
    {
        parser::Lit default_info_lit {"default-information originate"};
        parser::Lit always_lit {"always"};
        parser::TagValue<parser::IntUnsignedBase10<uint32_t, 1, 8>, int> metric(
            "metric", 16777214 + 1);  // plus 1 to become an invalid value
        parser::TagValue<parser::IntUnsignedBase10<uint8_t, 1, 1>, int> metric_type(
            "metric-type", 2 + 1);  // plus 1 to become an invalid value
        parser::TagValue<parser::OneOrMore<FrrUtilsEatAll>> route_map("route-map");

        // Parse optional argument "always"
        if (frr_util_group_spaces(line, {&default_info_lit, &always_lit},
    {&metric, &metric_type, &route_map})) {
            res.always = true;
            parser_leftovers(metric, metric_type, route_map);
            return;
        } else if (frr_util_group_spaces(line, {&default_info_lit},
    {&metric, &metric_type, &route_map})) {
            res.always = false;
            parser_leftovers(metric, metric_type, route_map);
            return;
        }
    }

    FrrOspfRouterDefaultRoute res;
};

FrrRes<FrrOspfRouterDefaultRoute> frr_ospf_router_default_route_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfRouterDefaultRoute cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF stub router configuration
//----------------------------------------------------------------------------
// frr_ospf_router_stub_router
Vector<std::string> to_vty_ospf_router_stub_router_conf_set(
    const FrrOspfRouterStubRouter &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;

    if (val.is_administrative) {
        res.emplace_back("max-metric router-lsa administrative");
    }

    if (val.on_startup_interval.valid()) {
        buf << "max-metric router-lsa on-startup "
            << val.on_startup_interval.get();
        res.emplace_back(vtss::move(buf.buf));
    }

    if (val.on_shutdown_interval.valid()) {
        buf << "max-metric router-lsa on-shutdown "
            << val.on_shutdown_interval.get();
        res.emplace_back(vtss::move(buf.buf));
    }

    return res;
}

mesa_rc frr_ospf_router_stub_router_conf_set(const OspfInst &id,
                                             const FrrOspfRouterStubRouter &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_stub_router_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_router_stub_router_conf_del(
    const FrrOspfRouterStubRouter &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");

    if (val.is_administrative) {
        res.emplace_back("no max-metric router-lsa administrative");
    }

    if (val.on_startup_interval.valid()) {
        res.emplace_back("no max-metric router-lsa on-startup");
    }

    if (val.on_shutdown_interval.valid()) {
        res.emplace_back("no max-metric router-lsa on-shutdown");
    }

    return res;
}

mesa_rc frr_ospf_router_stub_router_conf_del(const OspfInst &id,
                                             const FrrOspfRouterStubRouter &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_stub_router_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfRouterStubRouter : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit max_metric_lit("max-metric");
        parser::Lit router_lsa_lit("router-lsa");
        parser::Lit startup_lit("on-startup");
        parser::Lit shutdown_lit("on-shutdown");
        parser::Lit admin_lit("administrative");
        parser::IntUnsignedBase10<uint32_t, 1, 5> startup_interval;
        parser::IntUnsignedBase10<uint32_t, 1, 3> shutdown_interval;

        if (frr_util_group_spaces(line, {
        &max_metric_lit, &router_lsa_lit, &startup_lit,
        &startup_interval
    })) {
            res.on_startup_interval = startup_interval.get();
            return;
        }

        if (frr_util_group_spaces(line, {
        &max_metric_lit, &router_lsa_lit, &shutdown_lit,
        &shutdown_interval
    })) {
            res.on_shutdown_interval = shutdown_interval.get();
            return;
        }

        if (frr_util_group_spaces(line, {&max_metric_lit, &router_lsa_lit, &admin_lit})) {
            res.is_administrative = true;
            return;
        }
    }

    FrrOspfRouterStubRouter res = {};
};

FrrRes<FrrOspfRouterStubRouter> frr_ospf_router_stub_router_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfRouterStubRouter cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF SPF throttling configuration
//----------------------------------------------------------------------------
// frr_ospf_router_spf_throttling
Vector<std::string> to_vty_ospf_router_spf_throttling_conf_set(
    const FrrOspfRouterSPFThrotlling &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;

    buf << "timers throttle spf " << val.delay << " " << val.init_holdtime
        << " " << val.max_holdtime;
    res.emplace_back(vtss::move(buf.buf));

    return res;
}

mesa_rc frr_ospf_router_spf_throttling_conf_set(
    const OspfInst &id, const FrrOspfRouterSPFThrotlling &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_spf_throttling_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_router_spf_throttling_conf_del()
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    res.emplace_back("no timers throttle spf");
    return res;
}

mesa_rc frr_ospf_router_spf_throttling_conf_del(const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_router_spf_throttling_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfRouterSPFThrottling : public FrrUtilsConfStreamParserCB {
    FrrParseOspfRouterSPFThrottling(uint32_t d, uint32_t ih, uint32_t mh)
        : res {d, ih, mh} {}

    void router(const std::string &name, const str &line) override
    {
        parser::Lit timers_lit("timers");
        parser::Lit throttling_lit("throttle");
        parser::Lit spf_lit("spf");
        /* Notice: the usage of IntUnsignedBase10<TYPE, MIN, MAX>.
         * The <MIN, MAX> means input character length, not valid range
         * value. In this case, the valid ranges are all 0-600000, so the valid
         * character input length is 1-6.
         */
        parser::IntUnsignedBase10<uint32_t, 1, 6> delay;
        parser::IntUnsignedBase10<uint32_t, 1, 6> init_holdtime;
        parser::IntUnsignedBase10<uint32_t, 1, 6> max_holdtime;

        if (frr_util_group_spaces(line, {
        &timers_lit, &throttling_lit, &spf_lit, &delay,
        &init_holdtime, &max_holdtime
    })) {
            res.delay = delay.get();
            res.init_holdtime = init_holdtime.get();
            res.max_holdtime = max_holdtime.get();
            return;
        }
    }

    FrrOspfRouterSPFThrotlling res = {};
};

FrrRes<FrrOspfRouterSPFThrotlling> frr_ospf_router_spf_throttling_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfRouterSPFThrottling cb {0, 50, 5000};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// OSPF Area configuration -----------------------------------------------------
Vector<std::string> to_vty_ospf_area_network_conf_set(const FrrOspfAreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "network " << val.net << " area " << Ipv4Address(val.area);
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_network_conf_set(const OspfInst &id,
                                       const FrrOspfAreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_network_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_network_conf_del(const FrrOspfAreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no network " << val.net << " area " << Ipv4Address(val.area);
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_network_conf_del(const OspfInst &id,
                                       const FrrOspfAreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_network_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaNetwork : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit net("network");
        parser::Ipv4Network net_val;
        parser::Lit area("area");
        parser::IPv4 area_val;
        if (frr_util_group_spaces(line, {&net, &net_val, &area, &area_val})) {
            res.emplace_back(net_val.get().as_api_type(),
                             area_val.get().as_api_type());
        }
    }

    Vector<FrrOspfAreaNetwork> res;
};

Vector<FrrOspfAreaNetwork> frr_ospf_area_network_conf_get(std::string &conf, const OspfInst &id)
{
    Vector<FrrOspfAreaNetwork> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspfAreaNetwork cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_range_conf
Vector<std::string> to_vty_ospf_area_range_conf_set(const FrrOspfAreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " range " << val.net;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_range_conf_set(const OspfInst &id,
                                     const FrrOspfAreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_range_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_range_conf_del(const FrrOspfAreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " range " << val.net;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_range_conf_del(const OspfInst &id,
                                     const FrrOspfAreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_range_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaRange : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit range_lit("range");
        parser::Ipv4Network net_val;
        if (frr_util_group_spaces(line, {&area_lit, &area_val, &range_lit, &net_val})) {
            res.emplace_back(net_val.get().as_api_type(),
                             area_val.get().as_api_type());
        }
    }

    Vector<FrrOspfAreaNetwork> res;
};

Vector<FrrOspfAreaNetwork> frr_ospf_area_range_conf_get(std::string &conf, const OspfInst &id)
{
    Vector<FrrOspfAreaNetwork> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspfAreaRange cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_range_not_advertise_conf
Vector<std::string> to_vty_ospf_area_range_not_advertise_conf_set(
    const FrrOspfAreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " range " << val.net
        << " not-advertise";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_range_not_advertise_conf_set(const OspfInst &id,
                                                   const FrrOspfAreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_range_not_advertise_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_range_not_advertise_conf_del(
    const FrrOspfAreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " range " << val.net
        << " not-advertise";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_range_not_advertise_conf_del(const OspfInst &id,
                                                   const FrrOspfAreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_range_not_advertise_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaRangeNotAdvertise : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit range_lit("range");
        parser::Ipv4Network net_val;
        parser::Lit cost_lit("cost");
        parser::IntUnsignedBase10<uint32_t, 0, 24> cost_val;
        parser::Lit not_advertise_lit("not-advertise");
        /* for configuration 'area range not-advertise' */
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &range_lit, &net_val,
        &not_advertise_lit
    })) {
            res.emplace_back(net_val.get().as_api_type(),
                             area_val.get().as_api_type());
            return;
        }
        /* for configuration 'area range cost <cost> not-advertise' */
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &range_lit, &net_val, &cost_lit,
        &cost_val, &not_advertise_lit
    })) {
            res.emplace_back(net_val.get().as_api_type(),
                             area_val.get().as_api_type());
            return;
        }
    }

    Vector<FrrOspfAreaNetwork> res;
};

Vector<FrrOspfAreaNetwork> frr_ospf_area_range_not_advertise_conf_get(std::string &conf, const OspfInst &id)
{
    Vector<FrrOspfAreaNetwork> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspfAreaRangeNotAdvertise cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_range_const_conf
Vector<std::string> to_vty_ospf_area_range_cost_conf_set(
    const FrrOspfAreaNetworkCost &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " range " << val.net << " cost "
        << val.cost;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_range_cost_conf_set(const OspfInst &id,
                                          const FrrOspfAreaNetworkCost &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_range_cost_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_range_cost_conf_del(
    const FrrOspfAreaNetworkCost &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " range " << val.net
        << " cost 0";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_range_cost_conf_del(const OspfInst &id,
                                          const FrrOspfAreaNetworkCost &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_range_cost_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaRangeCost : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit range_lit("range");
        parser::Ipv4Network net_val;
        parser::Lit cost_lit("cost");
        parser::IntUnsignedBase10<uint32_t, 1, 9> cost_val;
        if (frr_util_group_spaces(line, {&area_lit, &area_val, &range_lit, &net_val, &cost_lit, &cost_val})) {
            res.emplace_back(net_val.get().as_api_type(), area_val.get().as_api_type(), cost_val.get());
        }
    }

    Vector<FrrOspfAreaNetworkCost> res;
};

Vector<FrrOspfAreaNetworkCost> frr_ospf_area_range_cost_conf_get(std::string &conf, const OspfInst &id)
{
    Vector<FrrOspfAreaNetworkCost> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspfAreaRangeCost cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_virtual_link_conf
Vector<std::string> to_vty_ospf_area_virtual_link_conf_set(
    const FrrOspfAreaVirtualLink &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst);
    if (val.hello_interval.valid()) {
        buf << " hello-interval " << val.hello_interval.get();
    }

    if (val.retransmit_interval.valid()) {
        buf << " retransmit-interval " << val.retransmit_interval.get();
    }

    if (val.transmit_delay.valid()) {
        buf << " transmit-delay " << val.transmit_delay.get();
    }

    if (val.dead_interval.valid()) {
        buf << " dead-interval " << val.dead_interval.get();
    }

    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_virtual_link_conf_set(const OspfInst &id,
                                            const FrrOspfAreaVirtualLink &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_virtual_link_conf_del(
    const FrrOspfAreaVirtualLink &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_virtual_link_conf_del(const OspfInst &id, const FrrOspfAreaVirtualLink &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaVirtualLink : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit virtual_link("virtual-link");
        parser::IPv4 dst;
        parser::Lit hello_lit("hello-interval");
        parser::IntUnsignedBase10<uint16_t, 1, 6> hello_val;
        parser::Lit retransmit_lit("retransmit-interval");
        parser::IntUnsignedBase10<uint16_t, 1, 6> retransmit_val;
        parser::Lit transmit_lit("transmit-delay");
        parser::IntUnsignedBase10<uint16_t, 1, 6> transmit_val;
        parser::Lit dead_lit("dead-interval");
        parser::IntUnsignedBase10<uint16_t, 1, 6> dead_val;
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &virtual_link, &dst, &hello_lit,
        &hello_val, &retransmit_lit, &retransmit_val,
        &transmit_lit, &transmit_val, &dead_lit, &dead_val
    })) {
            FrrOspfAreaVirtualLink tmp(area_val.get().as_api_type(),
                                       dst.get().as_api_type());
            tmp.hello_interval = hello_val.get();
            tmp.retransmit_interval = retransmit_val.get();
            tmp.transmit_delay = transmit_val.get();
            tmp.dead_interval = dead_val.get();
            res.push_back(vtss::move(tmp));
            return;
        }

        if (frr_util_group_spaces(line, {&area_lit, &area_val, &virtual_link, &dst})) {
            FrrOspfAreaVirtualLink tmp(area_val.get().as_api_type(),
                                       dst.get().as_api_type());
            // default values
            tmp.hello_interval = 10;
            tmp.retransmit_interval = 5;
            tmp.transmit_delay = 1;
            tmp.dead_interval = 40;
            res.push_back(vtss::move(tmp));
        }
    }

    Vector<FrrOspfAreaVirtualLink> res;
};

Vector<FrrOspfAreaVirtualLink> frr_ospf_area_virtual_link_conf_get(std::string &conf, const OspfInst &id)
{
    Vector<FrrOspfAreaVirtualLink> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspfAreaVirtualLink cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF NSSA
//----------------------------------------------------------------------------
Vector<std::string> to_vty_ospf_stub_area_conf_set(const FrrOspfStubArea &conf)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;

    buf << "area " << Ipv4Address(conf.area)
        << (conf.is_nssa ? " nssa" : " stub");
    if (conf.is_nssa) {
        if (conf.nssa_translator_role == NssaTranslatorRoleNever) {
            buf << " translate-never";
        } else if (conf.nssa_translator_role == NssaTranslatorRoleAlways) {
            buf << " translate-always";
        } else {
            // do nothing here since it equals the default setting (candidate)
        }
    }

    res.emplace_back(vtss::move(buf.buf));

    buf.clear();
    if (conf.no_summary) {
        buf << "area " << Ipv4Address(conf.area)
            << (conf.is_nssa ? " nssa" : " stub") << " no-summary";
        res.emplace_back(vtss::move(buf.buf));
    } else {
        buf << "no area " << Ipv4Address(conf.area)
            << (conf.is_nssa ? " nssa" : " stub") << " no-summary";
        res.emplace_back(vtss::move(buf.buf));
    }

    return res;
}

mesa_rc frr_ospf_stub_area_conf_set(const OspfInst &id,
                                    const FrrOspfStubArea &conf)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_stub_area_conf_set(conf);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_stub_area_conf_del(const mesa_ipv4_t &area)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(area) << " nssa";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "no area " << Ipv4Address(area) << " stub";
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_stub_area_conf_del(const OspfInst &id, const mesa_ipv4_t &area)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_stub_area_conf_del(area);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfNssa : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit stub_lit("stub");
        parser::Lit nssa_lit("nssa");
        parser::Lit never_lit("translate-never");
        parser::Lit always_lit("translate-always");
        parser::Lit no_summary_lit("no-summary");

        if (frr_util_group_spaces(line,
        {&area_lit, &area_val, &stub_lit, &no_summary_lit})) {
            res.emplace_back(area_val.get().as_api_type(), false, true, NssaTranslatorRoleCandidate);
        } else if (frr_util_group_spaces(line, {&area_lit, &area_val, &stub_lit})) {
            res.emplace_back(area_val.get().as_api_type(), false, false, NssaTranslatorRoleCandidate);
        } else if (frr_util_group_spaces(line, {&area_lit, &area_val, &nssa_lit, &no_summary_lit})) {
            /* Notice that FRR divides the NSSA command output into two groups:
             * 1) Area type: "area nssa
             * {translate-candidate|translate-never|translate-always}" and
             * 2) NSSA translate roles: "area nssa [no-summary]"
             * Since group 2) MUST come after 1), we have to check if the area
             * has been existing before adding a new Vector element (with
             * no_summary == true).
             */
            if (res.size() &&
                res[res.size() - 1].area == area_val.get().as_api_type()) {
                res[res.size() - 1].no_summary = true;
            } else {
                res.emplace_back(area_val.get().as_api_type(), true, true, NssaTranslatorRoleCandidate);
            }
        } else if (frr_util_group_spaces(line, {&area_lit, &area_val, &nssa_lit, &never_lit})) {
            res.emplace_back(area_val.get().as_api_type(), true, false, NssaTranslatorRoleNever);
        } else if (frr_util_group_spaces(line, {&area_lit, &area_val, &nssa_lit, &always_lit})) {
            res.emplace_back(area_val.get().as_api_type(), true, false, NssaTranslatorRoleAlways);
        } else if (frr_util_group_spaces(line, {&area_lit, &area_val, &nssa_lit})) {
            res.emplace_back(area_val.get().as_api_type(), true, false, NssaTranslatorRoleCandidate);
        }
    }

    Vector<FrrOspfStubArea> res;
};

Vector<FrrOspfStubArea> frr_ospf_stub_area_conf_get(std::string &conf, const OspfInst &id)
{
    Vector<FrrOspfStubArea> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspfNssa cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF area authentication
//----------------------------------------------------------------------------
// frr_ospf_area_authentication_conf
Vector<std::string> to_vty_ospf_area_authentication_conf_set(
    const mesa_ipv4_t &area, FrrOspfAuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << (mode == FRR_OSPF_AUTH_MODE_NULL ? "no " : "");
    buf << "area " << Ipv4Address(area) << " authentication";
    buf << (mode == FRR_OSPF_AUTH_MODE_MSG_DIGEST ? " message-digest" : "");
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf_area_authentication_conf_set(const OspfInst &id,
                                              const FrrOspfAreaAuth &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_authentication_conf_set(val.area, val.auth_mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaAuthentication : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit auth_lit("authentication");
        parser::Lit digest_lit("message-digest");
        if (frr_util_group_spaces(line, {&area_lit, &area_val, &auth_lit, &digest_lit})) {
            res.emplace_back(area_val.get().as_api_type(),
                             FRR_OSPF_AUTH_MODE_MSG_DIGEST);
            return;
        }

        if (frr_util_group_spaces(line, {&area_lit, &area_val, &auth_lit})) {
            res.emplace_back(area_val.get().as_api_type(),
                             FRR_OSPF_AUTH_MODE_PWD);
            return;
        }
    }

    Vector<FrrOspfAreaAuth> res;
};

Vector<FrrOspfAreaAuth> frr_ospf_area_authentication_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) return Vector<FrrOspfAreaAuth> {};
    FrrParseOspfAreaAuthentication cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_virtual_link_authentication
Vector<std::string> to_vty_ospf_area_virtual_link_authentication_conf_set(
    const FrrOspfAreaVirtualLink &val, FrrOspfAuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << (mode == FRR_OSPF_AUTH_MODE_AREA_CFG ? "no " : "");
    buf << "area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " authentication";
    if (mode != FRR_OSPF_AUTH_MODE_AREA_CFG) {
        buf << (mode == FRR_OSPF_AUTH_MODE_NULL
                ? " null"
                : mode == FRR_OSPF_AUTH_MODE_PWD ? ""
                : " message-digest");
    }

    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_virtual_link_authentication_conf_set(
    const OspfInst &id, const FrrOspfAreaVirtualLinkAuth &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_authentication_conf_set(
                    val.virtual_link, val.auth_mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaVirtualLinkAuthentication : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit link_lit("virtual-link");
        parser::IPv4 dst_val;
        parser::Lit auth_lit("authentication");
        parser::Lit digest_lit("message-digest");
        parser::Lit null_lit("null");
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &link_lit, &dst_val, &auth_lit,
        &digest_lit
    })) {
            res.emplace_back(FrrOspfAreaVirtualLink(area_val.get().as_api_type(),
                                                    dst_val.get().as_api_type()),
                             FRR_OSPF_AUTH_MODE_MSG_DIGEST);
            return;
        }

        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &link_lit, &dst_val, &auth_lit,
        &null_lit
    })) {
            res.emplace_back(FrrOspfAreaVirtualLink(area_val.get().as_api_type(),
                                                    dst_val.get().as_api_type()),
                             FRR_OSPF_AUTH_MODE_NULL);
            return;
        }

        parser::EndOfInput eol;
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &link_lit, &dst_val, &auth_lit,
        &eol
    })) {
            res.emplace_back(FrrOspfAreaVirtualLink(area_val.get().as_api_type(),
                                                    dst_val.get().as_api_type()),
                             FRR_OSPF_AUTH_MODE_PWD);
            return;
        }

        if (frr_util_group_spaces(line, {&area_lit, &area_val, &link_lit, &dst_val, &eol})) {
            res.emplace_back(FrrOspfAreaVirtualLink(area_val.get().as_api_type(),
                                                    dst_val.get().as_api_type()),
                             FRR_OSPF_AUTH_MODE_AREA_CFG);
            return;
        }
    }

    Vector<FrrOspfAreaVirtualLinkAuth> res;
};

Vector<FrrOspfAreaVirtualLinkAuth> frr_ospf_area_virtual_link_authentication_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) return Vector<FrrOspfAreaVirtualLinkAuth> {};
    FrrParseOspfAreaVirtualLinkAuthentication cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_virtual_link_message_digest
Vector<std::string> to_vty_ospf_area_virtual_link_message_digest_conf_set(
    const FrrOspfAreaVirtualLink &val, const FrrOspfDigestData &data)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " message-digest-key " << data.keyid
        << " md5 " << data.key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_virtual_link_message_digest_conf_set(
    const OspfInst &id, const FrrOspfAreaVirtualLink &val,
    const FrrOspfDigestData &data)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_message_digest_conf_set(val, data);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_virtual_link_message_digest_conf_del(
    const FrrOspfAreaVirtualLink &val, const FrrOspfDigestData &data)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " message-digest-key " << data.keyid
        << " md5 " << data.key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_virtual_link_message_digest_conf_del(
    const OspfInst &id, const FrrOspfAreaVirtualLink &val,
    const FrrOspfDigestData &data)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_message_digest_conf_del(val, data);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaVirtualLinkMessageDigest : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit virtual_link_lit("virtual-link");
        parser::IPv4 dst_val;
        parser::Lit message_lit("message-digest-key");
        parser::IntUnsignedBase10<uint8_t, 1, 4> key_id;
        parser::Lit md5_lit("md5");
        parser::OneOrMore<FrrUtilsEatAll> key_val;
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &virtual_link_lit, &dst_val,
        &message_lit, &key_id, &md5_lit, &key_val
    })) {
            res.emplace_back(
                FrrOspfAreaVirtualLink {area_val.get().as_api_type(),
                                        dst_val.get().as_api_type()
                                       },
                FrrOspfDigestData {key_id.get(),
                                   std::string(key_val.get().begin(),
                                               key_val.get().end())
                                  });
        }
    }

    Vector<FrrOspfAreaVirtualLinkDigest> res;
};

Vector<FrrOspfAreaVirtualLinkDigest> frr_ospf_area_virtual_link_message_digest_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) return Vector<FrrOspfAreaVirtualLinkDigest> {};
    FrrParseOspfAreaVirtualLinkMessageDigest cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_area_virtual_link_authentication_key
Vector<std::string> to_vty_ospf_area_virtual_link_authentication_key_conf_set(
    const FrrOspfAreaVirtualLink &val, const std::string &auth_key)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " authentication-key " << auth_key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_virtual_link_authentication_key_conf_set(
    const OspfInst &id, const FrrOspfAreaVirtualLink &val,
    const std::string &auth_key)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_authentication_key_conf_set(
                    val, auth_key);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_area_virtual_link_authentication_key_conf_del(
    const FrrOspfAreaVirtualLinkKey &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.virtual_link.area) << " virtual-link "
        << Ipv4Address(val.virtual_link.dst) << " authentication-key "
        << val.key_data;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_area_virtual_link_authentication_key_conf_del(
    const OspfInst &id, const FrrOspfAreaVirtualLinkKey &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_area_virtual_link_authentication_key_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfAreaVirtualLinkAuthenticationKey
    : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit virtual_link_lit("virtual-link");
        parser::IPv4 dst_val;
        parser::Lit key_lit("authentication-key");
        parser::OneOrMore<FrrUtilsEatAll> key_val;
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &virtual_link_lit, &dst_val,
        &key_lit, &key_val
    })) {
            res.emplace_back(
                FrrOspfAreaVirtualLink {area_val.get().as_api_type(),
                                        dst_val.get().as_api_type()
                                       },
                std::string(key_val.get().begin(), key_val.get().end()));
        }
    }

    Vector<FrrOspfAreaVirtualLinkKey> res;
};

Vector<FrrOspfAreaVirtualLinkKey>
frr_ospf_area_virtual_link_authentication_key_conf_get(std::string &conf, const OspfInst &id)
{
    if (id != 1) return Vector<FrrOspfAreaVirtualLinkKey> {};
    FrrParseOspfAreaVirtualLinkAuthenticationKey cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// OSPF Interface Configuration ------------------------------------------------

Vector<std::string> to_vty_ospf_if_mtu_ignore_conf_set(const std::string &ifname, bool mtu_ignore)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(std::move(buf.buf));
    buf.clear();
    buf << (mtu_ignore ? "" : "no ") << "ip ospf mtu-ignore";
    res.emplace_back(std::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_if_mtu_ignore_conf_set(vtss_ifindex_t i, bool mtu_ignore)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_mtu_ignore_conf_set(interface_name.val, mtu_ignore);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

// frr_ospf_if_authentication_key_conf
Vector<std::string> to_vty_ospf_if_authentication_key_set(
    const std::string &ifname, const std::string &auth_key)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "ip ospf authentication-key " << auth_key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_if_authentication_key_conf_set(vtss_ifindex_t i,
                                                const std::string &auth_key)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds =
        to_vty_ospf_if_authentication_key_set(interface_name.val, auth_key);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_if_authentication_key_del(const std::string &ifname)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    res.emplace_back("no ip ospf authentication-key");
    return res;
}

mesa_rc frr_ospf_if_authentication_key_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_authentication_key_del(interface_name.val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfIfAuthenticationKey : public FrrUtilsConfStreamParserCB {
    FrrParseOspfIfAuthenticationKey(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line) override
    {
        if (name == ifname) {
            parser::Lit ip("ip");
            parser::Lit ospf("ospf");
            parser::Lit auth("authentication-key");
            parser::OneOrMore<FrrUtilsEatAll> key;
            if (frr_util_group_spaces(line, {&ip, &ospf, &auth, &key})) {
                res = std::string(key.get().begin(), key.get().end());
            }
        }
    }

    FrrRes<std::string> res = std::string("");
    const std::string &name;
};

FrrRes<std::string> frr_ospf_if_authentication_key_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfAuthenticationKey cb(interface_name.val);
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_if_authentication_conf
Vector<std::string> to_vty_ospf_if_authentication_set(const std::string &ifname,
                                                      FrrOspfAuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << (mode == FRR_OSPF_AUTH_MODE_AREA_CFG ? "no " : "");
    buf << "ip ospf authentication";
    buf << (mode == FRR_OSPF_AUTH_MODE_MSG_DIGEST
            ? " message-digest"
            : mode == FRR_OSPF_AUTH_MODE_NULL ? " null" : "");
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_if_authentication_conf_set(vtss_ifindex_t i,
                                            FrrOspfAuthMode mode)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_authentication_set(interface_name.val, mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfIfAuthentication : public FrrUtilsConfStreamParserCB {
    FrrParseOspfIfAuthentication(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line)
    {
        if (ifname == name) {
            parser::Lit ip("ip");
            parser::Lit ospf("ospf");
            parser::Lit auth("authentication");
            parser::Lit digest("message-digest");
            parser::Lit auth_null("null");
            if (frr_util_group_spaces(line, {&ip, &ospf, &auth, &digest})) {
                res = FRR_OSPF_AUTH_MODE_MSG_DIGEST;
                return;
            }

            if (frr_util_group_spaces(line, {&ip, &ospf, &auth, &auth_null})) {
                res = FRR_OSPF_AUTH_MODE_NULL;
                return;
            }

            parser::OneOrMore<FrrUtilsEatAll> others;
            if (frr_util_group_spaces(line, {&ip, &ospf, &auth}) &&
                !frr_util_group_spaces(line, {&ip, &ospf, &auth, &others})) {
                res = FRR_OSPF_AUTH_MODE_PWD;
                return;
            }
        }
    }

    const std::string &name;
    FrrOspfAuthMode res = FRR_OSPF_AUTH_MODE_AREA_CFG;
};

FrrRes<FrrOspfAuthMode> frr_ospf_if_authentication_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfAuthentication cb(interface_name.val);
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_if_message_digest_conf
Vector<std::string> to_vty_ospf_if_message_digest_set(
    const std::string &ifname, const FrrOspfDigestData &data)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "ip ospf message-digest-key " << data.keyid << " md5 " << data.key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_if_message_digest_conf_set(vtss_ifindex_t i,
                                            const FrrOspfDigestData &data)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_message_digest_set(interface_name.val, data);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

Vector<std::string> to_vty_ospf_if_message_digest_del(const std::string &ifname,
                                                      uint8_t keyid)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "no ip ospf message-digest-key " << keyid;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf_if_message_digest_conf_del(vtss_ifindex_t i, uint8_t keyid)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_message_digest_del(interface_name.val, keyid);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

struct FrrParseOspfIfMessageDigest : public FrrUtilsConfStreamParserCB {
    FrrParseOspfIfMessageDigest(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line)
    {
        if (name == ifname) {
            parser::Lit ip("ip");
            parser::Lit ospf("ospf");
            parser::Lit key("message-digest-key");
            parser::IntUnsignedBase10<uint8_t, 1, 4> key_id;
            parser::Lit md5("md5");
            parser::OneOrMore<FrrUtilsEatAll> key_val;
            if (frr_util_group_spaces(line, {&ip, &ospf, &key, &key_id, &md5, &key_val})) {
                res.emplace_back(key_id.get(), std::string(key_val.get().begin(),
                                                           key_val.get().end()));
            }
        }
    }

    Vector<FrrOspfDigestData> res;
    const std::string name;
};

Vector<FrrOspfDigestData> frr_ospf_if_message_digest_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    Vector<FrrOspfDigestData> res;
    if (!interface_name) {
        return res;
    }

    FrrParseOspfIfMessageDigest cb(interface_name.val);
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// helper frr_ospf_if_field
Vector<std::string> to_vty_ospf_if_field_conf_set(const std::string &ifname,
                                                  const std::string &field,
                                                  uint16_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "ip ospf " << field << " " << val;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

Vector<std::string> to_vty_ospf_if_field_conf_del(const std::string &ifname,
                                                  const std::string &field)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "no ip ospf " << field;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

struct FrrParseOspfIfField : public FrrUtilsConfStreamParserCB {
    FrrParseOspfIfField(const std::string &name, const std::string &word,
                        uint32_t val)
        : if_name {name}, key_word {word}, res {val} {}

    void interface(const std::string &ifname, const str &line) override
    {
        if (if_name == ifname) {
            parser::Lit ip("ip ospf");
            parser::Lit word(key_word.c_str());
            parser::IntUnsignedBase10<uint32_t, 1, 10> val;
            if (frr_util_group_spaces(line, {&ip, &word, &val})) {
                res = val.get();
            }
        }
    }

    const std::string if_name;
    const std::string key_word;
    uint32_t res;
};

struct FrrParseOspfIfFieldNoVal : public FrrUtilsConfStreamParserCB {
    FrrParseOspfIfFieldNoVal(const std::string &name, const std::string &word)
        : if_name {name}, key_word {word}
    {
        found = false;
    }

    void interface(const std::string &ifname, const str &line) override
    {
        if (if_name == ifname) {
            parser::Lit ip("ip ospf");
            parser::Lit word(key_word.c_str());
            if (frr_util_group_spaces(line, {&ip, &word})) {
                found = true;
            }
        }
    }

    const std::string if_name;
    const std::string key_word;
    bool              found;
};

// frr_ospf_if_dead_interval_conf
mesa_rc frr_ospf_if_dead_interval_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_set(interface_name.val, "dead-interval", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

mesa_rc frr_ospf_if_dead_interval_minimal_conf_set(vtss_ifindex_t i,
                                                   uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_set(
                    interface_name.val, "dead-interval minimal hello-multiplier", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

mesa_rc frr_ospf_if_dead_interval_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_del(interface_name.val, "dead-interval");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

FrrRes<FrrOspfDeadInterval> frr_ospf_if_dead_interval_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfField cb_multiplier {interface_name.val, "dead-interval minimal hello-multiplier", 0};
    frr_util_conf_parser(conf, cb_multiplier);
    if (cb_multiplier.res != 0) {
        return FrrOspfDeadInterval {true, cb_multiplier.res};
    }

    FrrParseOspfIfField cb {interface_name.val, "dead-interval", 40};
    frr_util_conf_parser(conf, cb);
    return FrrOspfDeadInterval {false, cb.res};
}

// frr_ospf_if_hello_interface_conf
mesa_rc frr_ospf_if_hello_interval_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_set(interface_name.val,
                                              "hello-interval", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

mesa_rc frr_ospf_if_hello_interval_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_del(interface_name.val, "hello-interval");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

FrrRes<uint32_t> frr_ospf_if_hello_interval_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfField cb {interface_name.val, "hello-interval", 10};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_if_retransmit_interval_conf
mesa_rc frr_ospf_if_retransmit_interval_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_set(interface_name.val,
                                              "retransmit-interval", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

mesa_rc frr_ospf_if_retransmit_interval_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_del(interface_name.val,
                                              "retransmit-interval");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

FrrRes<uint32_t> frr_ospf_if_retransmit_interval_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfField cb {interface_name.val, "retransmit-interval", 5};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf_if_cost_conf
mesa_rc frr_ospf_if_cost_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_set(interface_name.val, "cost", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

mesa_rc frr_ospf_if_cost_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_del(interface_name.val, "cost");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

FrrRes<uint32_t> frr_ospf_if_cost_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfField cb {interface_name.val, "cost", 0};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

FrrRes<bool> frr_ospf_if_mtu_ignore_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfFieldNoVal cb {interface_name.val, "mtu-ignore"};
    frr_util_conf_parser(conf, cb);

    return cb.found;
}

// frr_ospf_if_priority
mesa_rc frr_ospf_if_priority_conf_set(vtss_ifindex_t i, uint8_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf_if_field_conf_set(interface_name.val, "priority", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF, cmds, vty_res);
}

FrrRes<uint8_t> frr_ospf_if_priority_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspfIfField cb {interface_name.val, "priority", 1};
    frr_util_conf_parser(conf, cb);
    return static_cast<uint8_t>(cb.res);
}

}  // namespace vtss

