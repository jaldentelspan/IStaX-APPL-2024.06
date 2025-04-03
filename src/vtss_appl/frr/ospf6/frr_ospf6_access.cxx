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

#include "frr_ospf6_access.hxx"
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
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_OSPF6
#include "frr_trace.hxx"  // For module trace group definitions
#else
#include <vtss/basics/trace.hxx>
#endif

vtss_enum_descriptor_t ip_ospf6_neighbor_nsm_state_txt[] {
    {vtss::NSM_DependUpon, "DependUpon"},
    {vtss::NSM_Deleted, "Deleted"},
    {vtss::NSM_Down, "Down"},
    {vtss::NSM_Attempt, "Attempt"},
    {vtss::NSM_Init, "Init"},
    {vtss::NSM_TwoWay, "Twoway"},
    {vtss::NSM_ExStart, "ExStart"},
    {vtss::NSM_Exchange, "ExChange"},
    {vtss::NSM_Loading, "Loading"},
    {vtss::NSM_Full, "Full"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspf6NeighborNSMState, "nbrState",
                         ip_ospf6_neighbor_nsm_state_txt, "-");

vtss_enum_descriptor_t ip_ospf6_interface_network_type_txt[] {
    {vtss::NetworkType_Null, "Null"},
    {vtss::NetworkType_PointToPoint, "POINTOPOINT"},
    {vtss::NetworkType_Broadcast, "BROADCAST"},
    {vtss::NetworkType_Nbma, "NBMA"},
    {vtss::NetworkType_PointToMultipoint, "POINTTOMULTIPOINT"},
    {vtss::NetworkType_VirtualLink, "VIRTUALLINK"},
    {vtss::NetworkType_LoopBack, "LOOPBACK"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspf6IfNetworkType, "networkType",
                         ip_ospf6_interface_network_type_txt, "-");

vtss_enum_descriptor_t ip_ospf6_interface_ism_state_txt[] {
    {vtss::ISM_None, "None"},
    {vtss::ISM_Down, "Down"},
    {vtss::ISM_Loopback, "LoopBack"},
    {vtss::ISM_Waiting, "Waiting"},
    {vtss::ISM_PointToPoint, "Point-To-Point"},
    {vtss::ISM_DROther, "DROther"},
    {vtss::ISM_Backup, "BDR"},
    {vtss::ISM_DR, "DR"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspf6IfISMState, "state",
                         ip_ospf6_interface_ism_state_txt, "-");

vtss_enum_descriptor_t ip_ospf6_interface_type_txt[] {
    {vtss::IfType_Peer, "Peer"}, {vtss::IfType_Broadcast, "Broadcast"}, {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrIpOspf6IfType, "ospf6IfType",
                         ip_ospf6_interface_type_txt, "-");

vtss_enum_descriptor_t FrrOspf6LsdbType_txt[] {
    {vtss::FrrOspf6LsdbType_Router, "Router"},
    {vtss::FrrOspf6LsdbType_Network, "Network"},
    {vtss::FrrOspf6LsdbType_InterPrefix, "Inter-Prefix"},
    {vtss::FrrOspf6LsdbType_InterRouter, "Inter-Router"},
    {vtss::FrrOspf6LsdbType_AsExternal, "AS-External"},
    {vtss::FrrOspf6LsdbType_Link, "Link"},
    {vtss::FrrOspf6LsdbType_IntraPrefix, "Intra-Prefix"},
    {0, 0}
};
VTSS_JSON_SERIALIZE_ENUM(FrrOspf6LsdbType, "ospf6LsdbType", FrrOspf6LsdbType_txt,
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

template <typename KEY, typename VAL>
void serialize(vtss::expose::json::Loader &l, VectorMapSortedCb<KEY, VAL> &m)
{
    const char *b = l.pos_;
    std::string name;

    CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);
    CHECK_LOAD(l.load(name), Error);
    CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);
    CHECK_LOAD(l.load(vtss::expose::json::array_start), Error);
    while (true) {
        CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);
        KEY k {};
        CHECK_LOAD(l.load(k), Error);
        CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);
        VAL v {};
        CHECK_LOAD(l.load(v), Error);
        m.entry(k, v);
        CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);
        if (!l.load(vtss::expose::json::delimetor)) {
            l.reset_error();
            break;
        }
    }

    CHECK_LOAD(l.load(vtss::expose::json::array_end), Error);
    CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);
    return;
Error:
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

// frr_ip_ospf6_route_get ------------------------------------------------------
void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6RouteHopStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6RouteHopStatus"));

    std::string iface;
    if (m.add_leaf(iface, vtss::tag::Name("via"))) {
        s.is_connected = false;
        if (!m.add_leaf((s.ip), vtss::tag::Name("ip"))) {
            s.ip = {0};
        }
    } else if (m.add_leaf(iface, vtss::tag::Name("directly attached to"))) {
        s.is_connected = true;
        s.ip = {0};
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

static FrrOspf6RouteType parse_RouteType(const std::string &type)
{
    if (type == "N IE") {
        return RT_NetworkIA;
    }

    if (type == "D IE") {
        return RT_DiscardIA;
    }

    if (type == "N IA") {
        return RT_Network;
    }
    // FRR json output value is "R ", not "R"
    if (type == "R IA") {
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

static FrrOspf6RouterType parse_RouterType(const std::string &type)
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

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6RouteStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6RouteStatus"));

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

/* The vtss::Set operator (<) for 'APPL_FrrOspf6RouteKey' */
bool operator<(const APPL_FrrOspf6RouteKey &a, const APPL_FrrOspf6RouteKey &b)
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

/* The vtss::Set operator (!=) for 'APPL_FrrOspf6RouteKey' */
bool operator!=(const APPL_FrrOspf6RouteKey &a, const APPL_FrrOspf6RouteKey &b)
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

/* The vtss::Set operator (==) for 'APPL_FrrOspf6RouteKey' */
bool operator==(const APPL_FrrOspf6RouteKey &a, const APPL_FrrOspf6RouteKey &b)
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

struct Ospf6RouteApplMapCb
    : public MapSortedCb<mesa_ipv6_network_t, Vector<FrrOspf6RouteStatus>> {
    void entry(const mesa_ipv6_network_t &key,
               Vector<FrrOspf6RouteStatus> &val) override
    {
        APPL_FrrOspf6RouteKey k;
        APPL_FrrOspf6RouteStatus v;
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

    explicit Ospf6RouteApplMapCb(FrrIpOspf6RouteStatusMap &map) : res {map} {}

    FrrIpOspf6RouteStatusMap &res;
};

FrrIpOspf6RouteStatusMap frr_ip_ospf6_route_status_parse(const std::string &s)
{
    FrrIpOspf6RouteStatusMap result;
    Ospf6RouteApplMapCb res {result};
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(res);
    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf6_route_status_get()
{
    Vector<std::string> res;
    res.push_back("show ipv6 ospf6 route detail json");
    return res;
}

FrrIpOspf6RouteStatusMapResult frr_ip_ospf6_route_status_get()
{
    auto cmds = to_vty_ip_ospf6_route_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_route_status_parse(vty_res);
}

// frr_ip_ospf6_status_get ------------------------------------------------------
void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspf6Area &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf6_area"));
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

void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspf6Status &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf6_status"));
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

FrrIpOspf6Status frr_ip_ospf6_status_parse(const std::string &s)
{
    FrrIpOspf6Status result;
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

FrrRes<Vector<std::string>> to_vty_ip_ospf6_status_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 json");
    return res;
}

FrrRes<FrrIpOspf6Status> frr_ip_ospf6_status_get()
{
    auto cmds = to_vty_ip_ospf6_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
#ifndef VTSS_BASICS_STANDALONE
    VTSS_TRACE(INFO) << "execute cmd " << res;
#endif
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_status_parse(vty_res);
}

// frr_ip_ospf6_interface_status_get---------------------------------------------
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

static mesa_ipv6_network_t parse_network(const str &id)
{

    parser::Ipv6Network net_lit;
    if (frr_util_group_spaces(id, {&net_lit})) {
        return net_lit.get().as_api_type();
    }
    return {0, 0};
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspf6IfStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf6_interface_status"));
    ADD_LEAF_DEFAULT(s.if_up, vtss::tag::Name("ifUp"), false);
    (void)m.add_leaf(s.ospf6_enabled, vtss::tag::Name("ospf6Enabled"));
    if (!s.if_up) {
        return;
    }

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("inet6"));
    s.inet6 = parse_network(buf);
    (void)m.add_leaf(AsIpv4(s.area), vtss::tag::Name("area"));
    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("router_id"));
    (void)m.add_leaf(s.mtu_bytes, vtss::tag::Name("mtuBytes"));
    (void)m.add_leaf(s.mtu_mismatch_detection, vtss::tag::Name("mtuMismatchDetect"));
    (void)m.add_leaf(s.cost, vtss::tag::Name("cost"));
    (void)m.add_leaf(s.state, vtss::tag::Name("state"));
    (void)m.add_leaf(s.hello_interval,
                     vtss::tag::Name("helloInterval"));
    (void)m.add_leaf(s.timer_dead,
                     vtss::tag::Name("deadInterval"));
    (void)m.add_leaf(s.timer_retransmit,
                     vtss::tag::Name("retransmitInterval"));
    (void)m.add_leaf(s.transmit_delay,
                     vtss::tag::Name("transmitDelay"));
    (void)m.add_leaf(s.priority, vtss::tag::Name("priority"));
    (void)m.add_leaf(AsIpv4(s.dr_id), vtss::tag::Name("drId"));
    (void)m.add_leaf(AsIpv4(s.bdr_id), vtss::tag::Name("bdrId"));
    (void)m.add_leaf(s.thread_lsa_update, vtss::tag::Name("threadlsa"));
    (void)m.add_leaf(s.thread_lsa_ack, vtss::tag::Name("threadlsaack"));
    (void)m.add_leaf(s.num_pending_lsa, vtss::tag::Name("numPendingLSA"));
    (void)m.add_leaf(s.num_pending_lsaack, vtss::tag::Name("numPendingLSAAck"));

}

struct Interface6MapCb : public VectorMapSortedCb<std::string, FrrIpOspf6IfStatus> {
    void entry(const std::string &key, FrrIpOspf6IfStatus &val) override
    {
        vtss_ifindex_t ifindex = vtss_ifindex_from_os_ifname(key);
        if (frr_util_ifindex_valid(ifindex)) {
            res.set(ifindex, std::move(val));
            return;
        }
    }

    explicit Interface6MapCb(FrrIpOspf6IfStatusMap &map) : res {map} {}

    FrrIpOspf6IfStatusMap &res;
};

FrrIpOspf6IfStatusMap frr_ip_ospf6_interface_status_parse(const std::string &s)
{
    FrrIpOspf6IfStatusMap result;
    Interface6MapCb res {result};
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(res);
    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf6_interface_status_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 interface json");
    return res;
}

FrrIpOspf6IfStatusMapResult frr_ip_ospf6_interface_status_get()
{
    auto cmds = to_vty_ip_ospf6_interface_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_interface_status_parse(vty_res);
}

// frr_ip_ospf6_neighbor_status_get----------------------------------------------
void serialize(vtss::expose::json::Loader &l, vtss::FrrIpOspf6NeighborStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("vtss_ip_ospf6_neightbor_status"));
    (void)m.add_leaf((s.if_address), vtss::tag::Name("ifaceAddress"));

    (void)m.add_leaf(AsIpv4(s.area), vtss::tag::Name("areaId"));

    std::string iface;
    (void)m.add_leaf(iface, vtss::tag::Name("ifaceName"));
    VTSS_TRACE(DEBUG) << "ifname " << iface;
#ifndef VTSS_BASICS_STANDALONE
    vtss_ifindex_t ifindex = vtss_ifindex_from_os_ifname(iface);
    if (frr_util_ifindex_valid(ifindex)) {
        s.ifindex = ifindex;
    }
#endif

    (void)m.add_leaf(s.nbr_priority, vtss::tag::Name("nbrPriority"));
    (void)m.add_leaf(s.nbr_state, vtss::tag::Name("nbrState"));
    (void)m.add_leaf(AsIpv4(s.dr_id),
                     vtss::tag::Name("routerDesignatedId"));
    (void)m.add_leaf(AsIpv4(s.bdr_id),
                     vtss::tag::Name("routerDesignatedBackupId"));
    (void)m.add_leaf(s.router_dead_interval_timer_due.raw(),
                     vtss::tag::Name("routerDeadIntervalTimerDueMsec"));
}

FrrIpOspf6NbrStatusMap frr_ip_ospf6_neighbor_status_parse(const std::string &s)
{
    FrrIpOspf6NbrStatusMap result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

FrrRes<Vector<std::string>> to_vty_ip_ospf6_neighbor_status_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 neighbor detail json");
    return res;
}

FrrIpOspf6NbrStatusMapResult frr_ip_ospf6_neighbor_status_get()
{
    auto cmds = to_vty_ip_ospf6_neighbor_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_neighbor_status_parse(vty_res);
}

// frr_ip_ospf6_db_get ------------------------------------------------------
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

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbLinkVal &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkVal"));

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

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbLinkStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkStates"));

    (void)m.add_leaf(s.type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.desc, vtss::tag::Name("desc"));

    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));

    (void)m.add_leaf(s.links, vtss::tag::Name("links"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbStatus"));
    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));

    (void)m.add_leaf(s.area, vtss::tag::Name("areas"));

    (void)m.add_leaf(s.type5, vtss::tag::Name("AS External Link States"));
    (void)m.add_leaf(s.type7, vtss::tag::Name("NSSA-external Link States"));
}

/* The vtss::Set operator (<) for 'APPL_FrrOspf6DbKey' */
bool operator<(const APPL_FrrOspf6DbKey &a, const APPL_FrrOspf6DbKey &b)
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

/* The vtss::Set operator (==) for 'APPL_FrrOspf6DbKey' */
bool operator==(const APPL_FrrOspf6DbKey &a, const APPL_FrrOspf6DbKey &b)
{
    if (a.area_id != b.area_id) {
        return a.area_id < b.area_id;
    }

    if (a.type != b.type) {
        return false;
    }

    if (a.link_id != b.link_id) {
        return false;
    }

    return a.adv_router == b.adv_router;
}

/* The vtss::Set operator (!=) for 'APPL_FrrOspf6DbKey' */
bool operator!=(const APPL_FrrOspf6DbKey &a, const APPL_FrrOspf6DbKey &b)
{
    return !(a == b);
}

struct Ospf6DbLinkStateConvertCb : public StructValCb<FrrOspf6DbStatus> {
    void convert(const FrrOspf6DbStatus &val) override
    {
        // area
        for (auto &area : val.area) {
            for (auto &link : area.links) {
                APPL_FrrOspf6DbKey k;
                APPL_FrrOspf6DbLinkStateVal v;

                k.inst_id = 1;
                k.type = area.type;
                k.area_id = area.area_id;
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
            APPL_FrrOspf6DbKey key_type5;
            APPL_FrrOspf6DbLinkStateVal val_type5;

            key_type5.inst_id = 1;
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
            APPL_FrrOspf6DbKey key_type7;
            APPL_FrrOspf6DbLinkStateVal val_type7;

            key_type7.inst_id = 1;
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

    vtss::Map<APPL_FrrOspf6DbKey, APPL_FrrOspf6DbLinkStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbStatus, and return it
 */
FrrOspf6DbStatus frr_ip_ospf6_db_parse_raw(const std::string &s)
{
    FrrOspf6DbStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbStatus, if you want to
   return the sorted MAP data, you can use frr_ip_ospf6_db_get() */
FrrRes<FrrOspf6DbStatus> frr_ip_ospf6_db_get_pure(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbKey, APPL_FrrOspf6DbLinkStateVal> frr_ip_ospf6_db_parse(
    const std::string &s)
{
    Ospf6DbLinkStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbKey, APPL_FrrOspf6DbLinkStateVal>> frr_ip_ospf6_db_get(
                                                              const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_parse(vty_res);
}

// frr_ip_ospf6_db_router_get
// ------------------------------------------------------

template <typename MAP, typename DBTYPE>
void frr_ospf6_db_common_params(MAP &m, DBTYPE &dbtype)
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
               vtss::FrrOspf6DbRouterAreasRoutesLinksVal &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbRouterAreasRoutesLinksVal"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("linkConnectedTo"));
    s.link_connected_to = parse_linkConnectedTo(buf);

    (void)m.add_leaf(AsIpv4(s.link_id), vtss::tag::Name("linkID"));
    (void)m.add_leaf(AsIpv4(s.link_data), vtss::tag::Name("linkData"));

    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));

}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbRouterAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbRouterAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

    (void)m.add_leaf(s.links, vtss::tag::Name("links"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbRouterAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbRouterAreasStates"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbRouterStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbRouterStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

/* The vtss::Set operator (<) for 'APPL_FrrOspf6DbCommonKey' */
bool operator<(const APPL_FrrOspf6DbCommonKey &a,
               const APPL_FrrOspf6DbCommonKey &b)
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

/* The vtss::Set operator (==) for 'APPL_FrrOspf6DbCommonKey' */
bool operator==(const APPL_FrrOspf6DbCommonKey &a,
                const APPL_FrrOspf6DbCommonKey &b)
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

/* The vtss::Set operator (!=) for 'APPL_FrrOspf6DbCommonKey' */
bool operator!=(const APPL_FrrOspf6DbCommonKey &a,
                const APPL_FrrOspf6DbCommonKey &b)
{
    return !(a == b);
}

struct Ospf6DbRouterStateConvertCb : public StructValCb<FrrOspf6DbRouterStatus> {
    void convert(const FrrOspf6DbRouterStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbRouterStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id;
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

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbRouterStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_router_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database router detail json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbRouterStatus, and
 * return it */
FrrOspf6DbRouterStatus frr_ip_ospf6_db_router_parse_raw(const std::string &s)
{
    FrrOspf6DbRouterStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbRouterStatus, if you
   want to
   return the sorted MAP data, you can use frr_ip_ospf6_db_router_get() */
FrrRes<FrrOspf6DbRouterStatus> frr_ip_ospf6_db_router_get_pure(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_router_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_router_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbRouterStateVal>
frr_ip_ospf6_db_router_parse(const std::string &s)
{
    Ospf6DbRouterStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbRouterStateVal>>
                                                                  frr_ip_ospf6_db_router_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_router_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_router_parse(vty_res);
}

// frr_ip_ospf6_db_link_get ------------------------------------------------------
void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbLinkAreasRoutesLinksVal &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkAreasRoutesLinksVal"));

    (void)m.add_leaf(s.prefix, vtss::tag::Name("prefixAddr"));

    (void)m.add_leaf(s.prefix_options, vtss::tag::Name("prefixOptions"));
    (void)m.add_leaf(s.prefix_length, vtss::tag::Name("prefixLength"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbLinkAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

    (void)m.add_leaf(s.links, vtss::tag::Name("links"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbLinkAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkAreasStates"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbLinkStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct Ospf6DbLinkLinkStateConvertCb : public StructValCb<FrrOspf6DbLinkStatus> {
    void convert(const FrrOspf6DbLinkStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbLinkLinkStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id;
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

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbLinkLinkStateVal> res;
};


Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbLinkLinkStateVal>
frr_ip_ospf6_db_link_parse(const std::string &s)
{
    Ospf6DbLinkLinkStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_link_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database link detail json");
    return res;
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbLinkLinkStateVal>>
                                                                    frr_ip_ospf6_db_link_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_link_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_link_parse(vty_res);
}
// frr_ip_ospf6_db_intra_area_prefix_get ------------------------------------------------------
void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbIntraPrefixAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbLinkAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

    (void)m.add_leaf(s.links, vtss::tag::Name("links"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbIntraPrefixAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbIntraPrefixAreasStates"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbIntraPrefixStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbIntraPrefixStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct Ospf6DbIntraPrefixStateConvertCb : public StructValCb<FrrOspf6DbIntraPrefixStatus> {
    void convert(const FrrOspf6DbIntraPrefixStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbIntraPrefixStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.links = route.links;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbIntraPrefixStateVal> res;
};


Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbIntraPrefixStateVal>
frr_ip_ospf6_db_intra_area_prefix_parse(const std::string &s)
{
    Ospf6DbIntraPrefixStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_intra_area_prefix_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database intra-prefix detail json");
    return res;
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbIntraPrefixStateVal>>
                                                                       frr_ip_ospf6_db_intra_area_prefix_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_intra_area_prefix_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_intra_area_prefix_parse(vty_res);
}

// frr_ip_ospf6_db_net_get ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbNetAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbNetAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

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

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbNetAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbNetAreasStates"));

    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbNetStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbNetStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct Ospf6DbNetStateConvertCb : public StructValCb<FrrOspf6DbNetStatus> {
    void convert(const FrrOspf6DbNetStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbNetStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id;
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

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNetStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_network_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database network detail json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbNetStatus, and return
 * it */
FrrOspf6DbNetStatus frr_ip_ospf6_db_net_parse_raw(const std::string &s)
{
    FrrOspf6DbNetStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbNetStatus, if you want
   to
   return the sorted MAP data, you can use frr_ip_ospf6_db_net_get() */
FrrRes<FrrOspf6DbNetStatus> frr_ip_ospf6_db_net_get_pure(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_network_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_net_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNetStateVal> frr_ip_ospf6_db_net_parse(
    const std::string &s)
{
    Ospf6DbNetStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNetStateVal>>
                                                               frr_ip_ospf6_db_net_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_network_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_net_parse(vty_res);
}

// frr_ip_ospf6_db_inter_area_prefix_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbInterAreaPrefixAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbInterAreaPrefixAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("prefix"));
    s.prefix = parse_network(buf);

    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbInterAreaPrefixAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbInterAreaPrefixAreasStates"));

    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbInterAreaPrefixStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbInterAreaPrefixStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct Ospf6DbInterAreaPrefixStateConvertCb : public StructValCb<FrrOspf6DbInterAreaPrefixStatus> {
    void convert(const FrrOspf6DbInterAreaPrefixStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbInterAreaPrefixStateVal v;

                k.inst_id = 1;
                k.type = route.type;
                k.area_id = area.area_id;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.prefix = route.prefix;
                v.metric = route.metric;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaPrefixStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_inter_area_prefix_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database inter-prefix detail json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbInterAreaPrefixStatus, and
 * return it */
FrrOspf6DbInterAreaPrefixStatus frr_ip_ospf6_db_inter_area_prefix_parse_raw(const std::string &s)
{
    FrrOspf6DbInterAreaPrefixStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbInterAreaPrefixStatus, if you
   want to
   return the sorted MAP data, you can use frr_ip_ospf6_db_inter_area_prefix_get() */
FrrRes<FrrOspf6DbInterAreaPrefixStatus> frr_ip_ospf6_db_inter_area_prefix_get_pure(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_inter_area_prefix_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_inter_area_prefix_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaPrefixStateVal>
frr_ip_ospf6_db_inter_area_prefix_parse(const std::string &s)
{
    Ospf6DbInterAreaPrefixStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaPrefixStateVal>> frr_ip_ospf6_db_inter_area_prefix_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_inter_area_prefix_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_inter_area_prefix_parse(vty_res);
}

// frr_ip_ospf6_db_inter_area_router_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbInterAreaRouterAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m = l.as_map(
                                              vtss::tag::Typename("FrrOspf6DbInterAreaRouterAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

    (void)m.add_leaf(AsIpv4(s.destination), vtss::tag::Name("destination"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbInterAreaRouterAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbInterAreaRouterAreasStates"));

    (void)m.add_leaf(AsIpv4(s.area_id), vtss::tag::Name("area"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbInterAreaRouterStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbInterAreaRouterStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));
}

struct Ospf6DbInterAreaRouterStateConvertCb
    : public StructValCb<FrrOspf6DbInterAreaRouterStatus> {
    void convert(const FrrOspf6DbInterAreaRouterStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbInterAreaRouterStateVal v;

                k.inst_id = 1;
                k.area_id = area.area_id;
                k.type = route.type;
                k.link_state_id = route.link_state_id;
                k.adv_router = route.adv_router;

                v.router_id = val.router_id;
                v.age = route.age;
                v.options = route.options;
                v.sequence = route.sequence;
                v.checksum = route.checksum;
                v.length = route.length;
                v.destination = route.destination;
                v.metric = route.metric;

                res.set(k, v);
            }
        }
    }

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaRouterStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_inter_area_router_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database inter-router detail json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbInterAreaRouterStatus,
 * and return it */
FrrOspf6DbInterAreaRouterStatus frr_ip_ospf6_db_inter_area_router_parse_raw(
    const std::string &s)
{
    FrrOspf6DbInterAreaRouterStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbInterAreaRouterStatus, if
   you want to
   return the sorted MAP data, you can use frr_ip_ospf6_db_inter_area_router_get() */
FrrRes<FrrOspf6DbInterAreaRouterStatus> frr_ip_ospf6_db_inter_area_router_get_pure(
    const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_inter_area_router_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_inter_area_router_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaRouterStateVal>
frr_ip_ospf6_db_inter_area_router_parse(const std::string &s)
{
    Ospf6DbInterAreaRouterStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaRouterStateVal>>
                                                                           frr_ip_ospf6_db_inter_area_router_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_inter_area_router_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_inter_area_router_parse(vty_res);
}

// frr_ip_ospf6_db_external_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbExternalAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbExternalAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);
    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("prefix"));
    s.prefix = parse_network(buf);
    (void)m.add_leaf(s.metric_type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
    (void)m.add_leaf((s.forward_address),
                     vtss::tag::Name("forwardAddress"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbExternalRouteStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbExternalRouteStatus"));

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l, vtss::FrrOspf6DbExternalStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbExternalStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.type5, vtss::tag::Name("areas"));
}

struct Ospf6DbExternalStateConvertCb
    : public StructValCb<FrrOspf6DbExternalStatus> {
    void convert(const FrrOspf6DbExternalStatus &val) override
    {
        // for type 5
        for (auto &type : val.type5) {
            for (auto &route : type.routes) {
                APPL_FrrOspf6DbCommonKey key_type5;
                APPL_FrrOspf6DbExternalStateVal val_type5;

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

                val_type5.prefix = route.prefix;
                val_type5.metric_type = route.metric_type;
                val_type5.metric = route.metric;
                val_type5.forward_address = route.forward_address;

                res.set(key_type5, val_type5);
            }
        }
    }

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbExternalStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_external_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database as-external detail json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbExternalStatus, and
 * return it */
FrrOspf6DbExternalStatus frr_ip_ospf6_db_external_parse_raw(const std::string &s)
{
    FrrOspf6DbExternalStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbExternalStatus, if you
   want to
   return the sorted MAP data, you can use frr_ip_ospf6_db_external_get() */
FrrRes<FrrOspf6DbExternalStatus> frr_ip_ospf6_db_external_get_pure(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_external_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbExternalStateVal>
frr_ip_ospf6_db_external_parse(const std::string &s)
{
    Ospf6DbExternalStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbExternalStateVal>>
                                                                    frr_ip_ospf6_db_external_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_external_parse(vty_res);
}

// frr_ip_ospf6_db_nssa_external_get
// ------------------------------------------------------

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbNSSAExternalAreasRoutesStates &s)
{
    vtss::expose::json::Loader::Map_t m = l.as_map(
                                              vtss::tag::Typename("FrrOspf6DbNSSAExternalAreasRoutesStates"));

    frr_ospf6_db_common_params(m, s);

    (void)m.add_leaf(s.network_mask, vtss::tag::Name("networkMask"));
    (void)m.add_leaf(s.metric_type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("metric"));
    (void)m.add_leaf((s.forward_address),
                     vtss::tag::Name("forwardAddress"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbNSSAExternalAreasStates &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbNSSAExternalAreasStates"));

    std::string buf;
    (void)m.add_leaf(buf, vtss::tag::Name("area"));
    s.area_id = parse_area(buf);

    (void)m.add_leaf(s.routes, vtss::tag::Name("routes"));
}

void serialize(vtss::expose::json::Loader &l,
               vtss::FrrOspf6DbNSSAExternalStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrOspf6DbNSSAExternalStatus"));

    (void)m.add_leaf(AsIpv4(s.router_id), vtss::tag::Name("routerId"));
    (void)m.add_leaf(s.areas, vtss::tag::Name("areas"));

    //(void)m.add_leaf(s.type5, vtss::tag::Name("AS NSSAExternal Link States"));
}

struct Ospf6DbNSSAExternalStateConvertCb
    : public StructValCb<FrrOspf6DbNSSAExternalStatus> {
    void convert(const FrrOspf6DbNSSAExternalStatus &val) override
    {
        // area
        for (auto &area : val.areas) {
            for (auto &route : area.routes) {
                APPL_FrrOspf6DbCommonKey k;
                APPL_FrrOspf6DbNSSAExternalStateVal v;

                k.inst_id = 1;
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

    vtss::Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNSSAExternalStateVal> res;
};

FrrRes<Vector<std::string>> to_vty_ip_ospf6_db_nssa_external_get()
{
    Vector<std::string> res;
    res.emplace_back("show ipv6 ospf6 database nssa-external json");
    return res;
}

/* For unittesting, we parse the basic structure, FrrOspf6DbNSSAExternalStatus,
 * and
 * return it */
FrrOspf6DbNSSAExternalStatus frr_ip_ospf6_db_nssa_external_parse_raw(
    const std::string &s)
{
    FrrOspf6DbNSSAExternalStatus result;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* This is a basic function for basic structure, FrrOspf6DbNSSAExternalStatus, if
   you
   want to
   return the sorted MAP data, you can use frr_ip_ospf6_db_nssa_external_get() */
FrrRes<FrrOspf6DbNSSAExternalStatus> frr_ip_ospf6_db_nssa_external_get_pure(
    const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_nssa_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_nssa_external_parse_raw(vty_res);
}

Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNSSAExternalStateVal>
frr_ip_ospf6_db_nssa_external_parse(const std::string &s)
{
    Ospf6DbNSSAExternalStateConvertCb r;
    vtss::expose::json::Loader loader(&*s.begin(), s.c_str() + s.size());
    loader.patch_mode_ = true;
    loader.load(r);
    return std::move(r.res);
}

/* It returns sorted MAP for upper layer */
FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNSSAExternalStateVal>>
                                                                        frr_ip_ospf6_db_nssa_external_get(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ip_ospf6_db_nssa_external_get();
    if (!cmds) {
        return cmds.rc;
    }

    std::string vty_res;
    mesa_rc res = frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds.val, vty_res);
    if (res != VTSS_RC_OK) {
        return res;
    }

    return frr_ip_ospf6_db_nssa_external_parse(vty_res);
}

// OSPF6 Router configuration ---------------------------------------------------
Vector<std::string> to_vty_ospf6_router_conf_set()
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("no router ospf6");
    res.push_back("router ospf6");
    return res;
}

mesa_rc frr_ospf6_router_conf_set(const Ospf6Inst &id)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_conf_set();
    std::string vty_res;

    if (frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Here we enable SPF throttling to fix APPL-207.
     * Values 200(ms),400(ms),10000(ms) is referenced from the example of FRR
     * user guide.
     * TODO: The UI/APPL layer of SPF throttling is not implemented yet.
     */
    return frr_ospf6_router_spf_throttling_conf_set(id, {200, 400, 10000});
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

Vector<std::string> to_vty_ospf6_router_conf_set(const FrrOspf6RouterConf &conf)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router ospf6");
    StringStream buf;
    if (conf.ospf6_router_id.valid()) {
        buf << "ospf6 router-id " << Ipv4Address(conf.ospf6_router_id.get());
        res.emplace_back(vtss::move(buf.buf));
        buf.clear();
    }

    if (conf.ospf6_router_rfc1583.valid()) {
        buf << (conf.ospf6_router_rfc1583.get() ? "" : "no ")
            << "ospf6 rfc1583compatibility";
        res.emplace_back(vtss::move(buf.buf));
        buf.clear();
    }

    if (conf.ospf6_router_abr_type.valid()) {
        buf << "ospf6 abr-type "
            << abr_type_to_str(conf.ospf6_router_abr_type.get());
        res.emplace_back(vtss::move(buf.buf));
        buf.clear();
    }

    return res;
}

mesa_rc frr_ospf6_router_conf_set(const Ospf6Inst &id,
                                  const FrrOspf6RouterConf &conf)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_conf_set(conf);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_router_conf_del()
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("no router ospf6");
    return res;
}

mesa_rc frr_ospf6_router_conf_del(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    auto cmds = to_vty_ospf6_router_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6Router : public FrrUtilsConfStreamParserCB {
private:
    bool try_parse_id(const str &line)
    {
        parser::Lit lit_ospf6("ospf6");
        parser::Lit lit_router("router-id");
        parser::IPv4 router_id;
        if (frr_util_group_spaces(line, {&lit_ospf6, &lit_router, &router_id})) {
            res.ospf6_router_id = router_id.get().as_api_type();
            return true;
        }

        return false;
    }

    bool try_parse_rfc(const str &line)
    {
        parser::Lit lit_compatible("compatible");
        parser::Lit lit_rfc("rfc1583");
        if (frr_util_group_spaces(line, {&lit_compatible, &lit_rfc})) {
            res.ospf6_router_rfc1583 = true;
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
        parser::Lit lit_ospf6("ospf6");
        parser::Lit lit_abr("abr-type");
        parser::OneOrMore<FrrUtilsEatAll> lit_type;
        if (frr_util_group_spaces(line, {&lit_ospf6, &lit_abr, &lit_type})) {
            res.ospf6_router_abr_type = abr_parser(
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

    FrrOspf6RouterConf res;
};

FrrRes<FrrOspf6RouterConf> frr_ospf6_router_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6Router cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// OSPF6 Router Passive interface configuration ---------------------------------


Vector<std::string> to_vty_ospf6_router_if_area_conf_set(const std::string &ifname,
                                                         mesa_ipv4_t area_id)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router ospf6");
    StringStream buf;
    buf << "interface " << ifname << " area " << Ipv4Address(area_id);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

Vector<std::string> to_vty_ospf6_router_if_area_conf_del(const std::string &ifname,
                                                         mesa_ipv4_t area_id)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router ospf6");
    StringStream buf;
    buf << "no interface " << ifname << " area " << Ipv4Address(area_id);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_router_if_area_conf_set(const Ospf6Inst &id,
                                          const vtss_ifindex_t &i,
                                          vtss_appl_ospf6_area_id_conf_t area_id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_if_area_conf_set(interface_name.val, area_id.id);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_router_if_area_conf_del(const Ospf6Inst &id,
                                          const vtss_ifindex_t &i,
                                          vtss_appl_ospf6_area_id_conf_t area_id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_if_area_conf_del(interface_name.val, area_id.id);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaIdIf : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6AreaIdIf(const std::string &name, vtss_appl_ospf6_area_id_conf_t val)
        : if_name {name}, res {val} {}

    void router(const std::string &name, const str &line) override
    {
        parser::Lit interface("interface");
        parser::Lit lit_name(if_name);
        parser::Lit area("area");
        parser::IPv4 area_id;
        if (frr_util_group_spaces(line, {&interface, &lit_name, &area, &area_id})) {
            if (vtss::equal(lit_name.get().begin(), lit_name.get().end(),
                            if_name.begin()) &&
                (size_t)(lit_name.get().end() - lit_name.get().begin()) ==
                if_name.size()) {
                res.id = area_id.get().as_api_type();
                res.is_specific_id = true;
            }
        }
    }

    const std::string if_name;
    vtss_appl_ospf6_area_id_conf_t res;
};

FrrRes<vtss_appl_ospf6_area_id_conf_t> frr_ospf6_router_if_area_id_conf_get(std::string &conf, const Ospf6Inst &id, const vtss_ifindex_t &i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6AreaIdIf cb {interface_name.val, {0, false}};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_router_default_metric_conf
Vector<std::string> to_vty_ospf6_router_default_metric_conf_set(uint32_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "default-metric " << val;
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_router_default_metric_conf_set(const Ospf6Inst &id, uint32_t val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_default_metric_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_router_default_metric_conf_del()
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    res.emplace_back("no default-metric");
    return res;
}

mesa_rc frr_ospf6_router_default_metric_conf_del(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_default_metric_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6RouterDefaultMetric : public FrrUtilsConfStreamParserCB {
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

FrrRes<Optional<uint32_t>> frr_ospf6_router_default_metric_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6RouterDefaultMetric cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_router_distribute_conf
std::string metric_type_to_string(FrrOspf6RouterMetricType val)
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

Vector<std::string> to_vty_ospf6_router_redistribute_conf_set(
    const FrrOspf6RouterRedistribute &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "redistribute " << ip_util_route_protocol_to_str(val.protocol, false);

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_router_redistribute_conf_set(const Ospf6Inst &id, const FrrOspf6RouterRedistribute &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_redistribute_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no redistribute " << ip_util_route_protocol_to_str(val, false);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_router_redistribute_conf_del(const Ospf6Inst &id, vtss_appl_ip_route_protocol_t val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_redistribute_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6RouterRedistribute : public FrrUtilsConfStreamParserCB {
    void parse_protocol(const std::string &protocol, const str &line)
    {
        parser::Lit redistribute_lit {"redistribute"};
        parser::Lit protocol_lit {protocol};

        if (frr_util_group_spaces(line, {&redistribute_lit, &protocol_lit})) {
            FrrOspf6RouterRedistribute redistribute;
            redistribute.protocol = frr_util_route_protocol_from_str(protocol);

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
    Vector<FrrOspf6RouterRedistribute> res;
};

Vector<FrrOspf6RouterRedistribute> frr_ospf6_router_redistribute_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) return Vector<FrrOspf6RouterRedistribute> {};
    FrrParseOspf6RouterRedistribute cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF6 administrative distance
//----------------------------------------------------------------------------
struct FrrParseOspf6AdminDistanceCb : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit distance_lit("distance");
        parser::IntUnsignedBase10<uint8_t, 1, 3> distance_val;
        if (frr_util_group_spaces(line, {&distance_lit, &distance_val})) {
            res = distance_val.get();
        }
    }

    uint8_t res = 110;  // OSPF6 default distance
};

FrrRes<uint8_t> frr_ospf6_router_admin_distance_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6AdminDistanceCb cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

Vector<std::string> to_vty_ospf6_router_admin_distance_set(uint8_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "distance " << val;
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_router_admin_distance_set(const Ospf6Inst &id, uint8_t val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_admin_distance_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

//----------------------------------------------------------------------------
//** OSPF6 default route configuration
//----------------------------------------------------------------------------
// frr_ospf6_router_default_route
Vector<std::string> to_vty_ospf6_router_default_route_conf_set(
    const FrrOspf6RouterDefaultRoute &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
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

mesa_rc frr_ospf6_router_default_route_conf_set(
    const Ospf6Inst &id, const FrrOspf6RouterDefaultRoute &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_default_route_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_router_default_route_conf_del()
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    res.emplace_back("no default-information originate");
    return res;
}

mesa_rc frr_ospf6_router_default_route_conf_del(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_default_route_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6RouterDefaultRoute : public FrrUtilsConfStreamParserCB {
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

    FrrOspf6RouterDefaultRoute res;
};

FrrRes<FrrOspf6RouterDefaultRoute> frr_ospf6_router_default_route_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6RouterDefaultRoute cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF6 SPF throttling configuration
//----------------------------------------------------------------------------
// frr_ospf6_router_spf_throttling
Vector<std::string> to_vty_ospf6_router_spf_throttling_conf_set(
    const FrrOspf6RouterSPFThrotlling &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;

    buf << "timers throttle spf " << val.delay << " " << val.init_holdtime
        << " " << val.max_holdtime;
    res.emplace_back(vtss::move(buf.buf));

    return res;
}

mesa_rc frr_ospf6_router_spf_throttling_conf_set(
    const Ospf6Inst &id, const FrrOspf6RouterSPFThrotlling &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_spf_throttling_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_router_spf_throttling_conf_del()
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    res.emplace_back("no timers throttle spf");
    return res;
}

mesa_rc frr_ospf6_router_spf_throttling_conf_del(const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_router_spf_throttling_conf_del();
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6RouterSPFThrottling : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6RouterSPFThrottling(uint32_t d, uint32_t ih, uint32_t mh)
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

    FrrOspf6RouterSPFThrotlling res = {};
};

FrrRes<FrrOspf6RouterSPFThrotlling> frr_ospf6_router_spf_throttling_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6RouterSPFThrottling cb {0, 50, 5000};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// OSPF6 Area configuration -----------------------------------------------------
Vector<std::string> to_vty_ospf6_area_network_conf_set(const FrrOspf6AreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "network " << val.net << " area " << Ipv4Address(val.area);
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_area_network_conf_set(const Ospf6Inst &id,
                                        const FrrOspf6AreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_network_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_area_network_conf_del(const FrrOspf6AreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no network " << val.net << " area " << Ipv4Address(val.area);
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_area_network_conf_del(const Ospf6Inst &id,
                                        const FrrOspf6AreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_network_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaNetwork : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit net("network");
        parser::Ipv6Network net_val;
        parser::Lit area("area");
        parser::IPv4 area_val;
        if (frr_util_group_spaces(line, {&net, &net_val, &area, &area_val})) {
            res.emplace_back(net_val.get().as_api_type(),
                             area_val.get().as_api_type());
        }
    }

    Vector<FrrOspf6AreaNetwork> res;
};

Vector<FrrOspf6AreaNetwork> frr_ospf6_area_network_conf_get(std::string &conf, const Ospf6Inst &id)
{
    Vector<FrrOspf6AreaNetwork> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspf6AreaNetwork cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_area_range_conf
Vector<std::string> to_vty_ospf6_area_range_conf_set(const FrrOspf6AreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " range " << val.net;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_area_range_conf_set(const Ospf6Inst &id,
                                      const FrrOspf6AreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_range_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_area_range_conf_del(const FrrOspf6AreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " range " << val.net;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_area_range_conf_del(const Ospf6Inst &id,
                                      const FrrOspf6AreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_range_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaRange : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit range_lit("range");
        parser::Ipv6Network net_val;
        if (frr_util_group_spaces(line, {&area_lit, &area_val, &range_lit, &net_val})) {
            res.emplace_back(net_val.get().as_api_type(),
                             area_val.get().as_api_type());
        }
    }

    Vector<FrrOspf6AreaNetwork> res;
};

Vector<FrrOspf6AreaNetwork> frr_ospf6_area_range_conf_get(std::string &conf, const Ospf6Inst &id)
{
    Vector<FrrOspf6AreaNetwork> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspf6AreaRange cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_area_range_not_advertise_conf
Vector<std::string> to_vty_ospf6_area_range_not_advertise_conf_set(
    const FrrOspf6AreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " range " << val.net
        << " not-advertise";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_range_not_advertise_conf_set(const Ospf6Inst &id,
                                                    const FrrOspf6AreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_range_not_advertise_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_area_range_not_advertise_conf_del(
    const FrrOspf6AreaNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " range " << val.net
        << " not-advertise";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_range_not_advertise_conf_del(const Ospf6Inst &id,
                                                    const FrrOspf6AreaNetwork &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_range_not_advertise_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaRangeNotAdvertise : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit range_lit("range");
        parser::Ipv6Network net_val;
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

    Vector<FrrOspf6AreaNetwork> res;
};

Vector<FrrOspf6AreaNetwork> frr_ospf6_area_range_not_advertise_conf_get(std::string &conf, const Ospf6Inst &id)
{
    Vector<FrrOspf6AreaNetwork> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspf6AreaRangeNotAdvertise cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_area_range_const_conf
Vector<std::string> to_vty_ospf6_area_range_cost_conf_set(
    const FrrOspf6AreaNetworkCost &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " range " << val.net << " cost "
        << val.cost;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_range_cost_conf_set(const Ospf6Inst &id,
                                           const FrrOspf6AreaNetworkCost &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_range_cost_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_area_range_cost_conf_del(
    const FrrOspf6AreaNetworkCost &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " range " << val.net
        << " cost 0";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_range_cost_conf_del(const Ospf6Inst &id,
                                           const FrrOspf6AreaNetworkCost &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_range_cost_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaRangeCost : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit range_lit("range");
        parser::Ipv6Network net_val;
        parser::Lit cost_lit("cost");
        parser::IntUnsignedBase10<uint32_t, 1, 9> cost_val;
        if (frr_util_group_spaces(line, {&area_lit, &area_val, &range_lit, &net_val, &cost_lit, &cost_val})) {
            res.emplace_back(net_val.get().as_api_type(), area_val.get().as_api_type(), cost_val.get());
        }
    }

    Vector<FrrOspf6AreaNetworkCost> res;
};

Vector<FrrOspf6AreaNetworkCost> frr_ospf6_area_range_cost_conf_get(std::string &conf, const Ospf6Inst &id)
{
    Vector<FrrOspf6AreaNetworkCost> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspf6AreaRangeCost cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_area_virtual_link_conf
Vector<std::string> to_vty_ospf6_area_virtual_link_conf_set(
    const FrrOspf6AreaVirtualLink &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
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

mesa_rc frr_ospf6_area_virtual_link_conf_set(const Ospf6Inst &id,
                                             const FrrOspf6AreaVirtualLink &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_virtual_link_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_area_virtual_link_conf_del(
    const FrrOspf6AreaVirtualLink &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_area_virtual_link_conf_del(const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_virtual_link_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaVirtualLink : public FrrUtilsConfStreamParserCB {
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
            FrrOspf6AreaVirtualLink tmp(area_val.get().as_api_type(),
                                        dst.get().as_api_type());
            tmp.hello_interval = hello_val.get();
            tmp.retransmit_interval = retransmit_val.get();
            tmp.transmit_delay = transmit_val.get();
            tmp.dead_interval = dead_val.get();
            res.push_back(vtss::move(tmp));
            return;
        }

        if (frr_util_group_spaces(line, {&area_lit, &area_val, &virtual_link, &dst})) {
            FrrOspf6AreaVirtualLink tmp(area_val.get().as_api_type(),
                                        dst.get().as_api_type());
            // default values
            tmp.hello_interval = 10;
            tmp.retransmit_interval = 5;
            tmp.transmit_delay = 1;
            tmp.dead_interval = 40;
            res.push_back(vtss::move(tmp));
        }
    }

    Vector<FrrOspf6AreaVirtualLink> res;
};

Vector<FrrOspf6AreaVirtualLink> frr_ospf6_area_virtual_link_conf_get(std::string &conf, const Ospf6Inst &id)
{
    Vector<FrrOspf6AreaVirtualLink> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspf6AreaVirtualLink cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF6 NSSA
//----------------------------------------------------------------------------
Vector<std::string> to_vty_ospf6_stub_area_conf_set(const FrrOspf6StubArea &conf)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;

    if (conf.no_summary) {
        buf << "area " << Ipv4Address(conf.area) << " stub no-summary";
        res.emplace_back(vtss::move(buf.buf));
    } else {
        buf << "area " << Ipv4Address(conf.area) << " stub";
        res.emplace_back(vtss::move(buf.buf));
    }

    return res;
}

mesa_rc frr_ospf6_stub_area_conf_set(const Ospf6Inst &id,
                                     const FrrOspf6StubArea &conf)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_stub_area_conf_set(conf);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_stub_area_conf_del(const mesa_ipv4_t &area)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(area) << " stub";
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_stub_area_conf_del(const Ospf6Inst &id, const mesa_ipv4_t &area)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_stub_area_conf_del(area);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6Stub : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit stub_lit("stub");
        parser::Lit no_summary_lit("no-summary");

        if (frr_util_group_spaces(line,
        {&area_lit, &area_val, &stub_lit, &no_summary_lit})) {
            res.emplace_back(area_val.get().as_api_type(), true);
        } else if (frr_util_group_spaces(line, {&area_lit, &area_val, &stub_lit})) {
            res.emplace_back(area_val.get().as_api_type(), false);
        }
    }

    Vector<FrrOspf6StubArea> res;
};

Vector<FrrOspf6StubArea> frr_ospf6_stub_area_conf_get(std::string &conf, const Ospf6Inst &id)
{
    Vector<FrrOspf6StubArea> res;
    if (id != 1) {
        return res;
    }

    FrrParseOspf6Stub cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** OSPF6 area authentication
//----------------------------------------------------------------------------
// frr_ospf6_area_authentication_conf
Vector<std::string> to_vty_ospf6_area_authentication_conf_set(
    const mesa_ipv4_t &area, FrrOspf6AuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << (mode == FRR_OSPF6_AUTH_MODE_NULL ? "no " : "");
    buf << "area " << Ipv4Address(area) << " authentication";
    buf << (mode == FRR_OSPF6_AUTH_MODE_MSG_DIGEST ? " message-digest" : "");
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_ospf6_area_authentication_conf_set(const Ospf6Inst &id,
                                               const FrrOspf6AreaAuth &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_authentication_conf_set(val.area, val.auth_mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaAuthentication : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit area_lit("area");
        parser::IPv4 area_val;
        parser::Lit auth_lit("authentication");
        parser::Lit digest_lit("message-digest");
        if (frr_util_group_spaces(line, {&area_lit, &area_val, &auth_lit, &digest_lit})) {
            res.emplace_back(area_val.get().as_api_type(),
                             FRR_OSPF6_AUTH_MODE_MSG_DIGEST);
            return;
        }

        if (frr_util_group_spaces(line, {&area_lit, &area_val, &auth_lit})) {
            res.emplace_back(area_val.get().as_api_type(),
                             FRR_OSPF6_AUTH_MODE_PWD);
            return;
        }
    }

    Vector<FrrOspf6AreaAuth> res;
};

Vector<FrrOspf6AreaAuth> frr_ospf6_area_authentication_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) return Vector<FrrOspf6AreaAuth> {};
    FrrParseOspf6AreaAuthentication cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_area_virtual_link_authentication
Vector<std::string> to_vty_ospf6_area_virtual_link_authentication_conf_set(
    const FrrOspf6AreaVirtualLink &val, FrrOspf6AuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << (mode == FRR_OSPF6_AUTH_MODE_AREA_CFG ? "no " : "");
    buf << "area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " authentication";
    if (mode != FRR_OSPF6_AUTH_MODE_AREA_CFG) {
        buf << (mode == FRR_OSPF6_AUTH_MODE_NULL
                ? " null"
                : mode == FRR_OSPF6_AUTH_MODE_PWD ? ""
                : " message-digest");
    }

    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_virtual_link_authentication_conf_set(
    const Ospf6Inst &id, const FrrOspf6AreaVirtualLinkAuth &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_virtual_link_authentication_conf_set(
                    val.virtual_link, val.auth_mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaVirtualLinkAuthentication : public FrrUtilsConfStreamParserCB {
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
            res.emplace_back(FrrOspf6AreaVirtualLink(area_val.get().as_api_type(),
                                                     dst_val.get().as_api_type()),
                             FRR_OSPF6_AUTH_MODE_MSG_DIGEST);
            return;
        }

        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &link_lit, &dst_val, &auth_lit,
        &null_lit
    })) {
            res.emplace_back(FrrOspf6AreaVirtualLink(area_val.get().as_api_type(),
                                                     dst_val.get().as_api_type()),
                             FRR_OSPF6_AUTH_MODE_NULL);
            return;
        }

        parser::EndOfInput eol;
        if (frr_util_group_spaces(line, {
        &area_lit, &area_val, &link_lit, &dst_val, &auth_lit,
        &eol
    })) {
            res.emplace_back(FrrOspf6AreaVirtualLink(area_val.get().as_api_type(),
                                                     dst_val.get().as_api_type()),
                             FRR_OSPF6_AUTH_MODE_PWD);
            return;
        }

        if (frr_util_group_spaces(line, {&area_lit, &area_val, &link_lit, &dst_val, &eol})) {
            res.emplace_back(FrrOspf6AreaVirtualLink(area_val.get().as_api_type(),
                                                     dst_val.get().as_api_type()),
                             FRR_OSPF6_AUTH_MODE_AREA_CFG);
            return;
        }
    }

    Vector<FrrOspf6AreaVirtualLinkAuth> res;
};

Vector<FrrOspf6AreaVirtualLinkAuth> frr_ospf6_area_virtual_link_authentication_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) return Vector<FrrOspf6AreaVirtualLinkAuth> {};
    FrrParseOspf6AreaVirtualLinkAuthentication cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_area_virtual_link_message_digest
Vector<std::string> to_vty_ospf6_area_virtual_link_message_digest_conf_set(
    const FrrOspf6AreaVirtualLink &val, const FrrOspf6DigestData &data)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " message-digest-key " << data.keyid
        << " md5 " << data.key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_virtual_link_message_digest_conf_set(
    const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val,
    const FrrOspf6DigestData &data)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_virtual_link_message_digest_conf_set(val, data);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_area_virtual_link_message_digest_conf_del(
    const FrrOspf6AreaVirtualLink &val, const FrrOspf6DigestData &data)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.area) << " virtual-link "
        << Ipv4Address(val.dst) << " message-digest-key " << data.keyid
        << " md5 " << data.key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_virtual_link_message_digest_conf_del(
    const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val,
    const FrrOspf6DigestData &data)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_virtual_link_message_digest_conf_del(val, data);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaVirtualLinkMessageDigest : public FrrUtilsConfStreamParserCB {
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
                FrrOspf6AreaVirtualLink {area_val.get().as_api_type(),
                                         dst_val.get().as_api_type()
                                        },
                FrrOspf6DigestData {key_id.get(),
                                    std::string(key_val.get().begin(),
                                                key_val.get().end())
                                   });
        }
    }

    Vector<FrrOspf6AreaVirtualLinkDigest> res;
};

Vector<FrrOspf6AreaVirtualLinkDigest> frr_ospf6_area_virtual_link_message_digest_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) return Vector<FrrOspf6AreaVirtualLinkDigest> {};
    FrrParseOspf6AreaVirtualLinkMessageDigest cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

Vector<std::string> to_vty_ospf6_area_virtual_link_authentication_key_conf_del(
    const FrrOspf6AreaVirtualLinkKey &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router ospf6");
    StringStream buf;
    buf << "no area " << Ipv4Address(val.virtual_link.area) << " virtual-link "
        << Ipv4Address(val.virtual_link.dst) << " authentication-key "
        << val.key_data;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_area_virtual_link_authentication_key_conf_del(
    const Ospf6Inst &id, const FrrOspf6AreaVirtualLinkKey &val)
{
    if (id != 1) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_area_virtual_link_authentication_key_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6AreaVirtualLinkAuthenticationKey
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
                FrrOspf6AreaVirtualLink {area_val.get().as_api_type(),
                                         dst_val.get().as_api_type()
                                        },
                std::string(key_val.get().begin(), key_val.get().end()));
        }
    }

    Vector<FrrOspf6AreaVirtualLinkKey> res;
};

Vector<FrrOspf6AreaVirtualLinkKey>
frr_ospf6_area_virtual_link_authentication_key_conf_get(std::string &conf, const Ospf6Inst &id)
{
    if (id != 1) return Vector<FrrOspf6AreaVirtualLinkKey> {};
    FrrParseOspf6AreaVirtualLinkAuthenticationKey cb;
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// OSPF6 Interface Configuration ------------------------------------------------

Vector<std::string> to_vty_ospf6_if_mtu_ignore_conf_set(const std::string &ifname, bool mtu_ignore)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(std::move(buf.buf));
    buf.clear();
    buf << (mtu_ignore ? "" : "no ") << "ipv6 ospf6 mtu-ignore";
    res.emplace_back(std::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_if_mtu_ignore_conf_set(vtss_ifindex_t i, bool mtu_ignore)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_mtu_ignore_conf_set(interface_name.val, mtu_ignore);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_if_authentication_key_del(const std::string &ifname)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    res.emplace_back("no ipv6 ospf6 authentication-key");
    return res;
}

mesa_rc frr_ospf6_if_authentication_key_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_authentication_key_del(interface_name.val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6IfAuthenticationKey : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6IfAuthenticationKey(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line) override
    {
        if (name == ifname) {
            parser::Lit ip("ipv6");
            parser::Lit ospf6("ospf6");
            parser::Lit auth("authentication-key");
            parser::OneOrMore<FrrUtilsEatAll> key;
            if (frr_util_group_spaces(line, {&ip, &ospf6, &auth, &key})) {
                res = std::string(key.get().begin(), key.get().end());
            }
        }
    }

    FrrRes<std::string> res = std::string("");
    const std::string &name;
};

FrrRes<std::string> frr_ospf6_if_authentication_key_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfAuthenticationKey cb(interface_name.val);
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_if_authentication_conf
Vector<std::string> to_vty_ospf6_if_authentication_set(const std::string &ifname,
                                                       FrrOspf6AuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << (mode == FRR_OSPF6_AUTH_MODE_AREA_CFG ? "no " : "");
    buf << "ipv6 ospf6 authentication";
    buf << (mode == FRR_OSPF6_AUTH_MODE_MSG_DIGEST
            ? " message-digest"
            : mode == FRR_OSPF6_AUTH_MODE_NULL ? " null" : "");
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_if_authentication_conf_set(vtss_ifindex_t i,
                                             FrrOspf6AuthMode mode)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_authentication_set(interface_name.val, mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6IfAuthentication : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6IfAuthentication(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line)
    {
        if (ifname == name) {
            parser::Lit ip("ipv6");
            parser::Lit ospf6("ospf6");
            parser::Lit auth("authentication");
            parser::Lit digest("message-digest");
            parser::Lit auth_null("null");
            if (frr_util_group_spaces(line, {&ip, &ospf6, &auth, &digest})) {
                res = FRR_OSPF6_AUTH_MODE_MSG_DIGEST;
                return;
            }

            if (frr_util_group_spaces(line, {&ip, &ospf6, &auth, &auth_null})) {
                res = FRR_OSPF6_AUTH_MODE_NULL;
                return;
            }

            parser::OneOrMore<FrrUtilsEatAll> others;
            if (frr_util_group_spaces(line, {&ip, &ospf6, &auth}) &&
                !frr_util_group_spaces(line, {&ip, &ospf6, &auth, &others})) {
                res = FRR_OSPF6_AUTH_MODE_PWD;
                return;
            }
        }
    }

    const std::string &name;
    FrrOspf6AuthMode res = FRR_OSPF6_AUTH_MODE_AREA_CFG;
};

FrrRes<FrrOspf6AuthMode> frr_ospf6_if_authentication_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfAuthentication cb(interface_name.val);
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_if_message_digest_conf
Vector<std::string> to_vty_ospf6_if_message_digest_set(
    const std::string &ifname, const FrrOspf6DigestData &data)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "ipv6 ospf6 message-digest-key " << data.keyid << " md5 " << data.key;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_if_message_digest_conf_set(vtss_ifindex_t i,
                                             const FrrOspf6DigestData &data)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_message_digest_set(interface_name.val, data);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

Vector<std::string> to_vty_ospf6_if_message_digest_del(const std::string &ifname,
                                                       uint8_t keyid)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "no ipv6 ospf6 message-digest-key " << keyid;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

mesa_rc frr_ospf6_if_message_digest_conf_del(vtss_ifindex_t i, uint8_t keyid)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_message_digest_del(interface_name.val, keyid);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

struct FrrParseOspf6IfMessageDigest : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6IfMessageDigest(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line)
    {
        if (name == ifname) {
            parser::Lit ip("ipv6");
            parser::Lit ospf6("ospf6");
            parser::Lit key("message-digest-key");
            parser::IntUnsignedBase10<uint8_t, 1, 4> key_id;
            parser::Lit md5("md5");
            parser::OneOrMore<FrrUtilsEatAll> key_val;
            if (frr_util_group_spaces(line, {&ip, &ospf6, &key, &key_id, &md5, &key_val})) {
                res.emplace_back(key_id.get(), std::string(key_val.get().begin(),
                                                           key_val.get().end()));
            }
        }
    }

    Vector<FrrOspf6DigestData> res;
    const std::string name;
};

Vector<FrrOspf6DigestData> frr_ospf6_if_message_digest_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    Vector<FrrOspf6DigestData> res;
    if (!interface_name) {
        return res;
    }

    FrrParseOspf6IfMessageDigest cb(interface_name.val);
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// helper frr_ospf6_if_field
Vector<std::string> to_vty_ospf6_if_field_conf_set(const std::string &ifname,
                                                   const std::string &field,
                                                   uint16_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "ipv6 ospf6 " << field << " " << val;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

Vector<std::string> to_vty_ospf6_if_field_conf_del(const std::string &ifname,
                                                   const std::string &field)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << "no ipv6 ospf6 " << field;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

Vector<std::string> to_vty_ospf6_if_passive_conf_set(const std::string &ifname,
                                                     const mesa_bool_t val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    val == true ? buf << "ipv6 ospf6 passive" : buf << "no ipv6 ospf passive";
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

struct FrrParseOspf6IfField : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6IfField(const std::string &name, const std::string &word,
                         uint32_t val)
        : if_name {name}, key_word {word}, res {val} {}

    void interface(const std::string &ifname, const str &line) override
    {
        if (if_name == ifname) {
            parser::Lit ip("ipv6 ospf6");
            parser::Lit word(key_word.c_str());
            parser::IntUnsignedBase10<uint32_t, 1, 10> val;
            if (key_word != "passive") {
                if (frr_util_group_spaces(line, {&ip, &word, &val})) {
                    res = val.get();
                }
            } else {
                if (frr_util_group_spaces(line, {&ip, &word})) {
                    res = true;
                }
            }
        }
    }

    const std::string if_name;
    const std::string key_word;
    uint32_t res;
};

struct FrrParseOspf6IfFieldNoVal : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6IfFieldNoVal(const std::string &name, const std::string &word)
        : if_name {name}, key_word {word}
    {
        found = false;
    }

    void interface(const std::string &ifname, const str &line) override
    {
        if (if_name == ifname) {
            parser::Lit ip("ipv6 ospf6");
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

struct FrrParseOspf6IfPassive : public FrrUtilsConfStreamParserCB {
    FrrParseOspf6IfPassive(const std::string &name, const std::string &word,
                           mesa_bool_t val)
        : if_name {name}, key_word {word}, res {val} {}

    void interface(const std::string &ifname, const str &line) override
    {
        if (if_name == ifname) {
            parser::Lit ip("ipv6 ospf6");
            parser::Lit word(key_word.c_str());

            if (frr_util_group_spaces(line, {&ip, &word})) {
                res = true;
            }
        }
    }

    const std::string if_name;
    const std::string key_word;
    mesa_bool_t res;
};

// frr_ospf6_if_dead_interval_conf
mesa_rc frr_ospf6_if_dead_interval_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(interface_name.val, "dead-interval", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_dead_interval_minimal_conf_set(vtss_ifindex_t i,
                                                    uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(
                    interface_name.val, "dead-interval minimal hello-multiplier", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_dead_interval_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_del(interface_name.val, "dead-interval");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

FrrRes<FrrOspf6DeadInterval> frr_ospf6_if_dead_interval_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfField cb_multiplier {interface_name.val, "dead-interval minimal hello-multiplier", 0};
    frr_util_conf_parser(conf, cb_multiplier);
    if (cb_multiplier.res != 0) {
        return FrrOspf6DeadInterval {true, cb_multiplier.res};
    }

    FrrParseOspf6IfField cb {interface_name.val, "dead-interval", 40};
    frr_util_conf_parser(conf, cb);
    return FrrOspf6DeadInterval {false, cb.res};
}

// frr_ospf6_if_hello_interface_conf
mesa_rc frr_ospf6_if_hello_interval_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(interface_name.val,
                                               "hello-interval", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_hello_interval_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_del(interface_name.val, "hello-interval");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

FrrRes<uint32_t> frr_ospf6_if_hello_interval_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfField cb {interface_name.val, "hello-interval", 10};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_if_retransmit_interval_conf
mesa_rc frr_ospf6_if_retransmit_interval_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(interface_name.val,
                                               "retransmit-interval", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_retransmit_interval_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_del(interface_name.val,
                                               "retransmit-interval");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

FrrRes<uint32_t> frr_ospf6_if_retransmit_interval_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfField cb {interface_name.val, "retransmit-interval", 5};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_if_passive_conf
mesa_rc frr_ospf6_if_passive_conf_set(vtss_ifindex_t i, mesa_bool_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_passive_conf_set(interface_name.val, val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

// frr_ospf6_if_transmit_delay_conf
mesa_rc frr_ospf6_if_transmit_delay_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(interface_name.val,
                                               "transmit-delay", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_passive_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_del(interface_name.val,
                                               "passive");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_transmit_delay_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_del(interface_name.val,
                                               "transmit-delay");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

FrrRes<mesa_bool_t> frr_ospf6_if_passive_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfPassive cb {interface_name.val, "passive", false};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

FrrRes<uint32_t> frr_ospf6_if_transmit_delay_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfField cb {interface_name.val, "transmit-delay", 1};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

// frr_ospf6_if_cost_conf
mesa_rc frr_ospf6_if_cost_conf_set(vtss_ifindex_t i, uint32_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(interface_name.val, "cost", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

mesa_rc frr_ospf6_if_cost_conf_del(vtss_ifindex_t i)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_del(interface_name.val, "cost");
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

FrrRes<uint32_t> frr_ospf6_if_cost_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfField cb {interface_name.val, "cost", 0};
    frr_util_conf_parser(conf, cb);
    return cb.res;
}

FrrRes<bool> frr_ospf6_if_mtu_ignore_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfFieldNoVal cb {interface_name.val, "mtu-ignore"};
    frr_util_conf_parser(conf, cb);

    return cb.found;
}

// frr_ospf6_if_priority
mesa_rc frr_ospf6_if_priority_conf_set(vtss_ifindex_t i, uint8_t val)
{
    if (frr_daemon_start(FRR_DAEMON_TYPE_OSPF6) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_ospf6_if_field_conf_set(interface_name.val, "priority", val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_OSPF6, cmds, vty_res);
}

FrrRes<uint8_t> frr_ospf6_if_priority_conf_get(std::string &conf, vtss_ifindex_t i)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseOspf6IfField cb {interface_name.val, "priority", 1};
    frr_util_conf_parser(conf, cb);
    return static_cast<uint8_t>(cb.res);
}

}  // namespace vtss

