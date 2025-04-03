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

#ifndef _FRR_OSPF6_ACCESS_HXX_
#define _FRR_OSPF6_ACCESS_HXX_

#include <unistd.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/ip.h>
#include <vtss/appl/ospf6.h>
#include <vtss/appl/types.h>
#include <string>
#include <vtss/basics/map.hxx>
#include <vtss/basics/optional.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/time.hxx>
#include <vtss/basics/vector.hxx>
#include "frr_daemon.hxx"

namespace vtss
{
typedef uint32_t Ospf6Inst;  // TODO, apply int-tag

// - Sx "show ipv6 ospf6 route [json]"

//"N IA", "D IA", "N", "R", "N E1", "N E2"
enum FrrOspf6RouteType {
    RT_Network,
    RT_NetworkIA,
    RT_DiscardIA,
    RT_Router,
    RT_ExtNetworkTypeOne,
    RT_ExtNetworkTypeTwo
};

enum FrrOspf6RouterType {
    RouterType_None,
    RouterType_ABR,
    RouterType_ASBR,
    RouterType_ABR_ASBR
};

struct FrrOspf6RouteHopStatus {
    FrrOspf6RouteHopStatus(const FrrOspf6RouteHopStatus &) = delete;
    FrrOspf6RouteHopStatus &operator=(const vtss::FrrOspf6RouteHopStatus &) = delete;
    FrrOspf6RouteHopStatus(FrrOspf6RouteHopStatus &&) = default;
    FrrOspf6RouteHopStatus &operator=(FrrOspf6RouteHopStatus && ) = default;
    mesa_ipv6_t ip;
    vtss_ifindex_t ifindex;
    bool is_connected;
#ifdef VTSS_BASICS_STANDALONE
    std::string ifname;
#endif
};

struct FrrOspf6RouteStatus {
    FrrOspf6RouteType route_type;
    uint32_t cost;
    uint32_t ext_cost;
    mesa_ipv4_t area;
    bool is_ia;
    FrrOspf6RouterType router_type;
    Vector<FrrOspf6RouteHopStatus> next_hops;
};

struct APPL_FrrOspf6RouteKey {
    Ospf6Inst inst_id;
    FrrOspf6RouteType route_type;
    mesa_ipv6_network_t network;
    mesa_ipv4_t area;
    mesa_ipv6_t nexthop_ip;
};

struct APPL_FrrOspf6RouteStatus {
    uint32_t cost;
    uint32_t ext_cost;
    bool is_ia;
    FrrOspf6RouterType router_type;
    bool is_connected;
    vtss_ifindex_t ifindex;
#ifdef VTSS_BASICS_STANDALONE
    std::string ifname;
#endif
};

using FrrIpOspf6RouteStatusMap =
    Map<APPL_FrrOspf6RouteKey, APPL_FrrOspf6RouteStatus>;
using FrrIpOspf6RouteStatusMapResult = FrrRes<FrrIpOspf6RouteStatusMap>;
FrrIpOspf6RouteStatusMapResult frr_ip_ospf6_route_status_get();

struct FrrIpOspf6Area {
    bool backbone;
    bool stub_no_summary;
    bool stub_shortcut;
    bool nssa_translator_elected;
    bool nssa_translator_always;
    int32_t area_if_total_counter;
    int32_t area_if_activ_counter;
    int32_t full_adjancet_counter;
    int32_t spf_executed_counter;
    int32_t lsa_nr;
    int32_t lsa_router_nr;
    int32_t lsa_router_checksum;
    int32_t lsa_network_nr;
    int32_t lsa_network_checksum;
    int32_t lsa_summary_nr;
    int32_t lsa_summary_checksum;
    int32_t lsa_asbr_nr;
    int32_t lsa_asbr_checksum;
    int32_t lsa_nssa_nr;
    int32_t lsa_nssa_checksum;
    int32_t lsa_opaque_link_nr;
    int32_t lsa_opaque_link_checksum;
    int32_t lsa_opaque_area_nr;
    int32_t lsa_opaque_area_checksum;
};

struct FrrIpOspf6Status {
    mesa_ipv4_t router_id;
    milliseconds deferred_shutdown_time;
    bool tos_routes_only;
    bool rfc2328;
    milliseconds spf_schedule_delay;
    milliseconds hold_time_min;
    milliseconds hold_time_max;
    int32_t hold_time_multiplier;
    milliseconds spf_last_executed;
    milliseconds spf_last_duration;
    milliseconds lsa_min_interval;
    milliseconds lsa_min_arrival;
    int32_t write_multiplier;
    milliseconds refresh_timer;
    int32_t lsa_external_counter;
    int32_t lsa_external_checksum;
    int32_t lsa_asopaque_counter;
    int32_t lsa_asopaque_checksums;
    int32_t attached_area_counter;
    Map<mesa_ipv4_t, FrrIpOspf6Area> areas;
};
FrrRes<FrrIpOspf6Status> frr_ip_ospf6_status_get();

// - Sx "show ipv6 ospf6 interface [INTERFACE] [json]" - 3d PGS-28
enum AreaDescriptionType {
    AreaDescriptionType_Default,
    AreaDescriptionType_Stub,
    AreaDescriptionType_Nssa
};

struct AreaDescription {
    AreaDescription() = default;
    AreaDescription(mesa_ipv4_t a, AreaDescriptionType t) : area {a}, type {t} {}

    mesa_ipv4_t area;
    AreaDescriptionType type;
};

enum FrrIpOspf6IfNetworkType {
    NetworkType_Null = 0,
    NetworkType_PointToPoint = 1,
    NetworkType_Broadcast = 2,
    NetworkType_Nbma = 3,
    NetworkType_PointToMultipoint = 4,
    NetworkType_VirtualLink = 5,
    NetworkType_LoopBack = 6
};

enum FrrIpOspf6IfISMState {
    ISM_None = 0,
    ISM_Down = 1,
    ISM_Loopback = 2,
    ISM_Waiting = 3,
    ISM_PointToPoint = 4,
    ISM_DROther = 5,
    ISM_Backup = 6,
    ISM_DR = 7
};

enum FrrIpOspf6IfType { IfType_Peer = 0, IfType_Broadcast = 1 };

struct FrrIpOspf6IfStatus {
    FrrIpOspf6IfStatus(const FrrIpOspf6IfStatus &) = delete;
    FrrIpOspf6IfStatus &operator=(const vtss::FrrIpOspf6IfStatus &) = delete;
    FrrIpOspf6IfStatus(FrrIpOspf6IfStatus &&) = default;
    FrrIpOspf6IfStatus &operator=(FrrIpOspf6IfStatus && ) = default;
    FrrIpOspf6IfStatus() = default;
    bool if_up;
    vtss_ifindex_t ifindex;
    int32_t mtu_bytes;
    bool mtu_mismatch_detection;
    std::string if_flags;
    bool ospf6_enabled;
    mesa_ipv6_network_t inet6;
    mesa_ipv4_t area;
    mesa_ipv4_t router_id;
    uint32_t cost;
    FrrIpOspf6IfISMState state;
    int32_t priority;
    mesa_ipv4_t bdr_id;
    mesa_ipv4_t dr_id;

    uint32_t timer_dead;
    uint32_t timer_retransmit;
    uint32_t transmit_delay; // in seconds
    uint32_t hello_interval; // in seconds
    bool timer_passive_iface;

    bool thread_lsa_update;
    uint32_t num_pending_lsa;
    bool thread_lsa_ack;
    uint32_t num_pending_lsaack;
};

using FrrIpOspf6IfStatusMap = Map<vtss_ifindex_t, FrrIpOspf6IfStatus>;
using FrrIpOspf6IfStatusMapResult = FrrRes<FrrIpOspf6IfStatusMap>;
FrrIpOspf6IfStatusMapResult frr_ip_ospf6_interface_status_get();

// - Sx "show ipv6 ospf6 neighbor [json]" - 3d PGS-29 - conf
enum FrrIpOspf6NeighborNSMState {
    NSM_DependUpon = 0,
    NSM_Deleted = 1,
    NSM_Down = 2,
    NSM_Attempt = 3,
    NSM_Init = 4,
    NSM_TwoWay = 5,
    NSM_ExStart = 6,
    NSM_Exchange = 7,
    NSM_Loading = 8,
    NSM_Full = 9
};

struct FrrIpOspf6NeighborStatus {
    // Remove the copy constructor
    FrrIpOspf6NeighborStatus(const FrrIpOspf6NeighborStatus &) = delete;

    // Remove the copy assignment operator
    FrrIpOspf6NeighborStatus &operator=(const vtss::FrrIpOspf6NeighborStatus &) = delete;

    // Add the move constructor
    FrrIpOspf6NeighborStatus(FrrIpOspf6NeighborStatus &&) = default;

    // Add the move assignment operator
    FrrIpOspf6NeighborStatus &operator=(FrrIpOspf6NeighborStatus && ) = default;

    mesa_ipv6_t if_address;
    AreaDescription transit_id;
    mesa_ipv4_t area;
    vtss_ifindex_t ifindex;
    int32_t nbr_priority;
    FrrIpOspf6NeighborNSMState nbr_state;

    /* Notice that the value of "routerDesignatedId" in FRR JSON output
     * represents the IP address of DR on the network. */
    mesa_ipv4_t dr_id;

    /* Notice that the value of "routerDesignatedBackupId" in FRR JSON output
     * represents the IP address of BDR on the network. */
    mesa_ipv4_t bdr_id;

    milliseconds router_dead_interval_timer_due;
};

using FrrIpOspf6NbrStatusMap = Map<mesa_ipv4_t, Vector<FrrIpOspf6NeighborStatus>>;
using FrrIpOspf6NbrStatusMapResult = FrrRes<FrrIpOspf6NbrStatusMap>;

// the key is the neighbor router id
FrrIpOspf6NbrStatusMapResult frr_ip_ospf6_neighbor_status_get();

// - Sx "show ipv6 ospf6 database [json]"
enum FrrOspf6LsdbType {
    FrrOspf6LsdbType_None = 0,
    FrrOspf6LsdbType_Router = 0x2001,
    FrrOspf6LsdbType_Network = 0x2002,
    FrrOspf6LsdbType_InterPrefix = 0x2003,
    FrrOspf6LsdbType_InterRouter = 0x2004,
    FrrOspf6LsdbType_AsExternal = 0x4005,
    FrrOspf6LsdbType_Nssa = 0x2007,
    FrrOspf6LsdbType_Link = 0x008,
    FrrOspf6LsdbType_IntraPrefix = 0x2009,
};

struct APPL_FrrOspf6DbKey {
    Ospf6Inst inst_id;
    int32_t type;
    mesa_ipv4_t area_id;
    mesa_ipv4_t link_id;
    mesa_ipv4_t adv_router;
};

struct APPL_FrrOspf6DbLinkStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    uint32_t sequence;
    int32_t checksum;
    int32_t router_link_count;          // Optional: used by type 1 only
    mesa_ipv4_network_t summary_route;  // Optional: used by type 3 only
    int32_t external;  // Optional: used by type 5 and type 7 only
    int32_t tag;       // Optional: used by type 5 and type 7 only
};

struct FrrOspf6DbLinkVal {
    mesa_ipv4_t link_id;
    mesa_ipv4_t adv_router;
    int32_t age;
    uint32_t sequence;
    int32_t checksum;
    int32_t router_link_count;          // optional for type 1 only
    mesa_ipv4_network_t summary_route;  // optional for type 3 only
    int32_t external;                   // optional for type 5 and type 7
    int32_t tag;                        // optional for type 5 and type 7
};

struct FrrOspf6DbLinkStates {
    int32_t type;
    std::string desc;
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbLinkVal> links;
};

struct FrrOspf6DbStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbLinkStates> area;
    FrrOspf6DbLinkStates type5;
    FrrOspf6DbLinkStates type7;
};

FrrRes<FrrOspf6DbStatus> frr_ip_ospf6_db_get_pure(const Ospf6Inst &id);
FrrRes<Map<APPL_FrrOspf6DbKey, APPL_FrrOspf6DbLinkStateVal>> frr_ip_ospf6_db_get(
                                                              const Ospf6Inst &id);

// OSPF6 db common part

struct APPL_FrrOspf6DbCommonKey {
    Ospf6Inst inst_id;
    mesa_ipv4_t area_id;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
};
// - Sx "show ipv6 ospf6 database link [json]"

struct FrrOspf6DbLinkAreasRoutesLinksVal {
    mesa_ipv6_t prefix;
    uint32_t prefix_length;
    uint8_t  prefix_options;
};

struct APPL_FrrOspf6DbLinkLinkStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspf6DbLinkAreasRoutesLinksVal> links;
};

struct FrrOspf6DbLinkAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspf6DbLinkAreasRoutesLinksVal> links;
};

struct FrrOspf6DbLinkAreasStates {
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbLinkAreasRoutesStates> routes;
};

struct FrrOspf6DbLinkStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbLinkAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbLinkLinkStateVal>>
                                                                    frr_ip_ospf6_db_link_get(const Ospf6Inst &id);

// - Sx "show ipv6 ospf6 database intra-prefix [json]"

struct APPL_FrrOspf6DbIntraPrefixStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspf6DbLinkAreasRoutesLinksVal> links;
};

struct FrrOspf6DbIntraPrefixAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspf6DbLinkAreasRoutesLinksVal> links;
};

struct FrrOspf6DbIntraPrefixAreasStates {
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbIntraPrefixAreasRoutesStates> routes;
};

struct FrrOspf6DbIntraPrefixStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbIntraPrefixAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbIntraPrefixStateVal>>
                                                                       frr_ip_ospf6_db_intra_area_prefix_get(const Ospf6Inst &id);


// - Sx "show ipv6 ospf6 database router [json]"

struct FrrOspf6DbRouterAreasRoutesLinksVal {
    int32_t link_connected_to;
    mesa_ipv4_t link_id;
    mesa_ipv4_t link_data;
    int32_t metric;
};

struct APPL_FrrOspf6DbRouterStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspf6DbRouterAreasRoutesLinksVal> links;
};

struct FrrOspf6DbRouterAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspf6DbRouterAreasRoutesLinksVal> links;
};

struct FrrOspf6DbRouterAreasStates {
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbRouterAreasRoutesStates> routes;
};

struct FrrOspf6DbRouterStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbRouterAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbRouterStateVal>>
                                                                  frr_ip_ospf6_db_router_get(const Ospf6Inst &id);

// - Sx "show ipv6 ospf6 database network [json]"

struct APPL_FrrOspf6DbNetStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    Vector<mesa_ipv4_t> attached_router;
};

struct FrrOspf6DbNetAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    Vector<mesa_ipv4_t> attached_router;
};

struct FrrOspf6DbNetAreasStates {
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbNetAreasRoutesStates> routes;
};

struct FrrOspf6DbNetStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbNetAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNetStateVal>>
                                                               frr_ip_ospf6_db_net_get(const Ospf6Inst &id);

// - Sx "show ipv6 ospf6 database summary [json]"

struct APPL_FrrOspf6DbInterAreaPrefixStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    mesa_ipv6_network_t prefix;
    int32_t metric;
};

struct FrrOspf6DbInterAreaPrefixAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    mesa_ipv6_network_t prefix;
    int32_t metric;
};

struct FrrOspf6DbInterAreaPrefixAreasStates {
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbInterAreaPrefixAreasRoutesStates> routes;
};

struct FrrOspf6DbInterAreaPrefixStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbInterAreaPrefixAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaPrefixStateVal>>
                                                                           frr_ip_ospf6_db_inter_area_prefix_get(const Ospf6Inst &id);

// - Sx "show ipv6 ospf6 database asbr-summary [json]"

struct APPL_FrrOspf6DbInterAreaRouterStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    mesa_ipv4_t destination;
    int32_t metric;
};

struct FrrOspf6DbInterAreaRouterAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    mesa_ipv4_t destination;
    int32_t metric;
};

struct FrrOspf6DbInterAreaRouterAreasStates {
    mesa_ipv4_t area_id;
    Vector<FrrOspf6DbInterAreaRouterAreasRoutesStates> routes;
};

struct FrrOspf6DbInterAreaRouterStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbInterAreaRouterAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaRouterStateVal>>
                                                                           frr_ip_ospf6_db_inter_area_router_get(const Ospf6Inst &id);

// - Sx "show ipv6 ospf6 database external [json]"

struct APPL_FrrOspf6DbExternalStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    mesa_ipv6_network_t prefix;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv6_t forward_address;
};

struct FrrOspf6DbExternalAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    mesa_ipv6_network_t prefix;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv6_t forward_address;
};

struct FrrOspf6DbExternalRouteStatus {
    Vector<FrrOspf6DbExternalAreasRoutesStates> routes;
};

struct FrrOspf6DbExternalStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbExternalRouteStatus> type5;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbExternalStateVal>>
                                                                    frr_ip_ospf6_db_external_get(const Ospf6Inst &id);

// - Sx "show ipv6 ospf6 database nssa-external [json]"

struct APPL_FrrOspf6DbNSSAExternalStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv6_t forward_address;
};

struct FrrOspf6DbNSSAExternalAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspf6LsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv6_t forward_address;
};

struct FrrOspf6DbNSSAExternalAreasStates {
    AreaDescription area_id;
    Vector<FrrOspf6DbNSSAExternalAreasRoutesStates> routes;
};

struct FrrOspf6DbNSSAExternalStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspf6DbNSSAExternalAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNSSAExternalStateVal>> frr_ip_ospf6_db_nssa_external_get(const Ospf6Inst &id);

// OSPF6 Router configuration ---------------------------------------------------

enum AbrType { AbrType_Cisco, AbrType_IBM, AbrType_Shortcut, AbrType_Standard };
struct FrrOspf6RouterConf {
    vtss::Optional<mesa_ipv4_t> ospf6_router_id;
    vtss::Optional<AbrType> ospf6_router_abr_type;
    vtss::Optional<bool> ospf6_router_rfc1583;
};

FrrRes<FrrOspf6RouterConf> frr_ospf6_router_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_router_conf_set(const Ospf6Inst &id);
mesa_rc frr_ospf6_router_conf_set(const Ospf6Inst &id, const FrrOspf6RouterConf &conf);
mesa_rc frr_ospf6_router_conf_del(const Ospf6Inst &id);

mesa_rc      frr_ospf6_router_if_area_conf_set(const Ospf6Inst &id, const vtss_ifindex_t &i, vtss_appl_ospf6_area_id_conf_t area_id);
mesa_rc      frr_ospf6_router_if_area_conf_del(const Ospf6Inst &id, const vtss_ifindex_t &i, vtss_appl_ospf6_area_id_conf_t area_id);
FrrRes<vtss_appl_ospf6_area_id_conf_t>  frr_ospf6_router_if_area_id_conf_get(std::string &running_conf, const Ospf6Inst &id, const vtss_ifindex_t &i);
mesa_rc      frr_ospf6_router_default_metric_conf_set(const Ospf6Inst &id, uint32_t val);
mesa_rc      frr_ospf6_router_default_metric_conf_del(const Ospf6Inst &id);

FrrRes<Optional<uint32_t>> frr_ospf6_router_default_metric_conf_get(std::string &running_conf, const Ospf6Inst &id);

enum FrrOspf6RouterMetricType {MetricType_One = 1, MetricType_Two};

struct FrrOspf6RouterRedistribute {
    vtss_appl_ip_route_protocol_t protocol;
};

Vector<FrrOspf6RouterRedistribute> frr_ospf6_router_redistribute_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_router_redistribute_conf_set(const Ospf6Inst &id, const FrrOspf6RouterRedistribute &val);
mesa_rc frr_ospf6_router_redistribute_conf_del(const Ospf6Inst &id, vtss_appl_ip_route_protocol_t val);

//----------------------------------------------------------------------------
//** OSPF6 administrative distance
//----------------------------------------------------------------------------
FrrRes<uint8_t> frr_ospf6_router_admin_distance_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_router_admin_distance_set(const Ospf6Inst &id, uint8_t val);
Vector<std::string> to_vty_ospf6_router_admin_distance_set(uint8_t val);

//----------------------------------------------------------------------------
//** OSPF6 default route configuration
//----------------------------------------------------------------------------
struct FrrOspf6RouterDefaultRoute {
    FrrOspf6RouterDefaultRoute() = default;
    vtss::Optional<bool> always;
    vtss::Optional<uint32_t> metric;
    vtss::Optional<FrrOspf6RouterMetricType> metric_type;
    vtss::Optional<std::string> route_map;
};

FrrRes<FrrOspf6RouterDefaultRoute> frr_ospf6_router_default_route_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_router_default_route_conf_set(const Ospf6Inst &id, const FrrOspf6RouterDefaultRoute &val);
mesa_rc frr_ospf6_router_default_route_conf_del(const Ospf6Inst &id);

//----------------------------------------------------------------------------
//** OSPF6 SPF throttling
//----------------------------------------------------------------------------
struct FrrOspf6RouterSPFThrotlling {
    FrrOspf6RouterSPFThrotlling() = default;
    uint32_t delay;
    uint32_t init_holdtime;
    uint32_t max_holdtime;
};

FrrRes<FrrOspf6RouterSPFThrotlling> frr_ospf6_router_spf_throttling_conf_get(std::string &running_conf, const Ospf6Inst &id);

mesa_rc frr_ospf6_router_spf_throttling_conf_set(const Ospf6Inst &id, const FrrOspf6RouterSPFThrotlling &val);
mesa_rc frr_ospf6_router_spf_throttling_conf_del(const Ospf6Inst &id);

// OSPF6 Area configuration -----------------------------------------------------
struct FrrOspf6AreaNetwork {
    FrrOspf6AreaNetwork() = default;
    FrrOspf6AreaNetwork(const mesa_ipv6_network_t &n, const mesa_ipv4_t &a)
        : net {n}, area {a} {}

    mesa_ipv6_network_t net;
    mesa_ipv4_t area;
};

Vector<FrrOspf6AreaNetwork> frr_ospf6_area_network_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_network_conf_set(const Ospf6Inst &id, const FrrOspf6AreaNetwork &val);
mesa_rc frr_ospf6_area_network_conf_del(const Ospf6Inst &id, const FrrOspf6AreaNetwork &net);

Vector<FrrOspf6AreaNetwork> frr_ospf6_area_range_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_range_conf_set(const Ospf6Inst &id, const FrrOspf6AreaNetwork &val);
mesa_rc frr_ospf6_area_range_conf_del(const Ospf6Inst &id, const FrrOspf6AreaNetwork &val);

Vector<FrrOspf6AreaNetwork> frr_ospf6_area_range_not_advertise_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_range_not_advertise_conf_set(const Ospf6Inst &id, const FrrOspf6AreaNetwork &val);
mesa_rc frr_ospf6_area_range_not_advertise_conf_del(const Ospf6Inst &id, const FrrOspf6AreaNetwork &val);

struct FrrOspf6AreaNetworkCost {
    FrrOspf6AreaNetworkCost() = default;
    FrrOspf6AreaNetworkCost(const mesa_ipv6_network_t &n, const mesa_ipv4_t &a, uint32_t c)
        : net {n}, area {a}, cost {c} {}

    mesa_ipv6_network_t net;
    mesa_ipv4_t area;
    uint32_t cost;
};

Vector<FrrOspf6AreaNetworkCost> frr_ospf6_area_range_cost_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_range_cost_conf_set(const Ospf6Inst &id, const FrrOspf6AreaNetworkCost &val);
mesa_rc frr_ospf6_area_range_cost_conf_del(const Ospf6Inst &id, const FrrOspf6AreaNetworkCost &val);

struct FrrOspf6AreaVirtualLink {
    FrrOspf6AreaVirtualLink() = default;
    FrrOspf6AreaVirtualLink(const mesa_ipv4_t &a, const mesa_ipv4_t &dst_ip)
        : area {a}, dst {dst_ip} {}

    mesa_ipv4_t area;
    mesa_ipv4_t dst;
    vtss::Optional<uint16_t> hello_interval;
    vtss::Optional<uint16_t> retransmit_interval;
    vtss::Optional<uint16_t> transmit_delay;
    vtss::Optional<uint16_t> dead_interval;
};

Vector<FrrOspf6AreaVirtualLink> frr_ospf6_area_virtual_link_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_virtual_link_conf_set(const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val);
mesa_rc frr_ospf6_area_virtual_link_conf_del(const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val);

//----------------------------------------------------------------------------
//** OSPF6 NSSA
//----------------------------------------------------------------------------
enum FrrOspf6NssaTranslatorRole {
    NssaTranslatorRoleCandidate,
    NssaTranslatorRoleAlways,
    NssaTranslatorRoleNever
};

struct FrrOspf6StubArea {
    FrrOspf6StubArea() = default;
    FrrOspf6StubArea(const mesa_ipv4_t &a, bool s)
        : area {a}, no_summary {s} {}

    mesa_ipv4_t area;
    bool no_summary;
};

Vector<FrrOspf6StubArea> frr_ospf6_stub_area_conf_get(std::string &running_conf, const Ospf6Inst &id);

enum FrrOspf6AuthMode {
    FRR_OSPF6_AUTH_MODE_AREA_CFG,
    FRR_OSPF6_AUTH_MODE_NULL,
    FRR_OSPF6_AUTH_MODE_PWD,
    FRR_OSPF6_AUTH_MODE_MSG_DIGEST,
};

struct FrrOspf6AreaAuth {
    FrrOspf6AreaAuth() = default;
    FrrOspf6AreaAuth(const mesa_ipv4_t &a, FrrOspf6AuthMode mode)
        : area {a}, auth_mode {mode} {}

    mesa_ipv4_t area;
    FrrOspf6AuthMode auth_mode;
};

Vector<FrrOspf6AreaAuth> frr_ospf6_area_authentication_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_stub_area_conf_set(const Ospf6Inst &id, const FrrOspf6StubArea &conf);
mesa_rc frr_ospf6_stub_area_conf_del(const Ospf6Inst &id, const mesa_ipv4_t &area);

//----------------------------------------------------------------------------
//** OSPF6 area authentication
//----------------------------------------------------------------------------
mesa_rc frr_ospf6_area_authentication_conf_set(const Ospf6Inst &id,
                                               const FrrOspf6AreaAuth &val);

struct FrrOspf6DigestData {
    FrrOspf6DigestData(const uint8_t kid, const std::string &kval)
        : keyid {kid}, key {kval} {}

    uint8_t keyid;
    std::string key;
};

struct FrrOspf6AreaVirtualLinkAuth {
    FrrOspf6AreaVirtualLinkAuth() = default;
    FrrOspf6AreaVirtualLinkAuth(const FrrOspf6AreaVirtualLink &link,
                                FrrOspf6AuthMode mode)
        : virtual_link {link}, auth_mode {mode} {}

    FrrOspf6AreaVirtualLink virtual_link;
    FrrOspf6AuthMode auth_mode;
};

Vector<FrrOspf6AreaVirtualLinkAuth> frr_ospf6_area_virtual_link_authentication_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_virtual_link_authentication_conf_set( const Ospf6Inst &id, const FrrOspf6AreaVirtualLinkAuth &val);

struct FrrOspf6AreaVirtualLinkDigest {
    FrrOspf6AreaVirtualLinkDigest() = default;
    FrrOspf6AreaVirtualLinkDigest(const FrrOspf6AreaVirtualLink &link,
                                  const FrrOspf6DigestData &data)
        : virtual_link {link}, digest_data {data} {}

    FrrOspf6AreaVirtualLink virtual_link;
    FrrOspf6DigestData digest_data;
};

Vector<FrrOspf6AreaVirtualLinkDigest> frr_ospf6_area_virtual_link_message_digest_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_virtual_link_message_digest_conf_set(const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val, const FrrOspf6DigestData &data);
mesa_rc frr_ospf6_area_virtual_link_message_digest_conf_del(const Ospf6Inst &id, const FrrOspf6AreaVirtualLink &val, const FrrOspf6DigestData &data);

struct FrrOspf6AreaVirtualLinkKey {
    FrrOspf6AreaVirtualLinkKey() = default;
    FrrOspf6AreaVirtualLinkKey(const FrrOspf6AreaVirtualLink &link, const std::string key)
        : virtual_link {link}, key_data {key} {}

    FrrOspf6AreaVirtualLink virtual_link;
    std::string key_data;
};

Vector<FrrOspf6AreaVirtualLinkKey> frr_ospf6_area_virtual_link_authentication_key_conf_get(std::string &running_conf, const Ospf6Inst &id);
mesa_rc frr_ospf6_area_virtual_link_authentication_key_conf_del(const Ospf6Inst &id, const FrrOspf6AreaVirtualLinkKey &val);

// OSPF6 Interface Configuration
mesa_rc frr_ospf6_if_mtu_ignore_conf_set(vtss_ifindex_t i, bool);
FrrRes<bool> frr_ospf6_if_mtu_ignore_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);

struct FrrOspf6DeadInterval {
    FrrOspf6DeadInterval() = default;
    FrrOspf6DeadInterval(bool mul, uint32_t v) : multiplier {mul}, val {v} {}

    bool multiplier;
    uint32_t val;
};

FrrRes<FrrOspf6DeadInterval> frr_ospf6_if_dead_interval_conf_get(std::string &running_config, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_dead_interval_conf_set(vtss_ifindex_t i, uint32_t val);
mesa_rc frr_ospf6_if_dead_interval_minimal_conf_set(vtss_ifindex_t ifindex, uint32_t val);
mesa_rc frr_ospf6_if_dead_interval_conf_del(vtss_ifindex_t ifindex);

FrrRes<uint32_t> frr_ospf6_if_hello_interval_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_hello_interval_conf_set(vtss_ifindex_t ifindex, uint32_t val);
mesa_rc frr_ospf6_if_hello_interval_conf_del(vtss_ifindex_t ifindex);

FrrRes<uint32_t> frr_ospf6_if_retransmit_interval_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_retransmit_interval_conf_set(vtss_ifindex_t i, uint32_t val);
mesa_rc frr_ospf6_if_retransmit_interval_conf_del(vtss_ifindex_t i);

FrrRes<mesa_bool_t> frr_ospf6_if_passive_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_passive_conf_set(vtss_ifindex_t i, mesa_bool_t val);
mesa_rc frr_ospf6_if_passive_conf_del(vtss_ifindex_t i);

FrrRes<uint32_t> frr_ospf6_if_transmit_delay_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_transmit_delay_conf_set(vtss_ifindex_t i, uint32_t val);
mesa_rc frr_ospf6_if_transmit_delay_conf_del(vtss_ifindex_t i);

FrrRes<uint32_t> frr_ospf6_if_cost_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_cost_conf_set(vtss_ifindex_t ifindex, uint32_t val);
mesa_rc frr_ospf6_if_cost_conf_del(vtss_ifindex_t ifindex);

FrrRes<std::string> frr_ospf6_if_authentication_key_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_authentication_key_conf_del(vtss_ifindex_t ifindex);

FrrRes<FrrOspf6AuthMode> frr_ospf6_if_authentication_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_authentication_conf_set(vtss_ifindex_t ifindex, FrrOspf6AuthMode mode);

Vector<FrrOspf6DigestData> frr_ospf6_if_message_digest_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_message_digest_conf_set(vtss_ifindex_t ifindex, const FrrOspf6DigestData &data);
mesa_rc frr_ospf6_if_message_digest_conf_del(vtss_ifindex_t ifindex, uint8_t keyid);

FrrRes<uint8_t> frr_ospf6_if_priority_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf6_if_priority_conf_set(vtss_ifindex_t i, uint8_t val);

mesa_rc frr_ospf6_if_del(vtss_ifindex_t ifindex);

/******************************************************************************/
/** For unit-test purpose                                                     */
/******************************************************************************/
bool operator<(const APPL_FrrOspf6RouteKey    &a, const APPL_FrrOspf6RouteKey    &b);
bool operator<(const APPL_FrrOspf6DbKey       &a, const APPL_FrrOspf6DbKey       &b);
bool operator<(const APPL_FrrOspf6DbCommonKey &a, const APPL_FrrOspf6DbCommonKey &b);

Vector<std::string> to_vty_ospf6_router_conf_del(void);
Vector<std::string> to_vty_ospf6_router_conf_set(void);
Vector<std::string> to_vty_ospf6_router_conf_set(const FrrOspf6RouterConf &conf);
Vector<std::string> to_vty_ospf6_area_network_conf_set(const FrrOspf6AreaNetwork &val);
Vector<std::string> to_vty_ospf6_area_network_conf_del(const FrrOspf6AreaNetwork &val);
Vector<std::string> to_vty_ospf6_area_authentication_conf_set(const mesa_ipv4_t &area, FrrOspf6AuthMode mode);
Vector<std::string> to_vty_ospf6_if_field_conf_set(const std::string &ifname, const std::string &field, uint16_t val);
Vector<std::string> to_vty_ospf6_if_field_conf_del(const std::string &ifname, const std::string &field);
Vector<std::string> to_vty_ospf6_area_range_conf_set(const FrrOspf6AreaNetwork &val);
Vector<std::string> to_vty_ospf6_area_range_conf_del(const FrrOspf6AreaNetwork &val);
Vector<std::string> to_vty_ospf6_area_range_not_advertise_conf_set(const FrrOspf6AreaNetwork &val);
Vector<std::string> to_vty_ospf6_area_range_not_advertise_conf_del(const FrrOspf6AreaNetwork &val);
Vector<std::string> to_vty_ospf6_area_range_cost_conf_set(const FrrOspf6AreaNetworkCost &val);
Vector<std::string> to_vty_ospf6_area_range_cost_conf_del(const FrrOspf6AreaNetworkCost &val);
Vector<std::string> to_vty_ospf6_area_virtual_link_conf_set(const FrrOspf6AreaVirtualLink &val);
Vector<std::string> to_vty_ospf6_area_virtual_link_conf_del(const FrrOspf6AreaVirtualLink &val);
Vector<std::string> to_vty_ospf6_stub_area_conf_set(const FrrOspf6StubArea &conf);
Vector<std::string> to_vty_ospf6_stub_area_conf_del(const mesa_ipv4_t &area);
Vector<std::string> to_vty_ospf6_if_authentication_key_del(const std::string &ifname);
Vector<std::string> to_vty_ospf6_if_authentication_set(const std::string &ifname, FrrOspf6AuthMode mode);
Vector<std::string> to_vty_ospf6_if_message_digest_set(const std::string &ifname, const FrrOspf6DigestData &data);
Vector<std::string> to_vty_ospf6_if_message_digest_del(const std::string &ifname, uint8_t keyid);
Vector<std::string> to_vty_ospf6_area_stub_no_summary_conf_set(const mesa_ipv4_t &area);
Vector<std::string> to_vty_ospf6_area_stub_no_summary_conf_del(const mesa_ipv4_t &area);
Vector<std::string> to_vty_ospf6_area_virtual_link_authentication_conf_set(const FrrOspf6AreaVirtualLink &val, FrrOspf6AuthMode mode);
Vector<std::string> to_vty_ospf6_area_virtual_link_message_digest_conf_set(const FrrOspf6AreaVirtualLink &val, const FrrOspf6DigestData &data);
Vector<std::string> to_vty_ospf6_area_virtual_link_message_digest_conf_del(const FrrOspf6AreaVirtualLink &val, const FrrOspf6DigestData &data);
Vector<std::string> to_vty_ospf6_area_virtual_link_authentication_key_conf_del(const FrrOspf6AreaVirtualLinkKey &val);
Vector<std::string> to_vty_ospf6_router_default_metric_conf_set(uint32_t val);
Vector<std::string> to_vty_ospf6_router_default_metric_conf_del(void);
Vector<std::string> to_vty_ospf6_router_default_route_conf_set(const FrrOspf6RouterDefaultRoute &val);
Vector<std::string> to_vty_ospf6_router_default_route_conf_del(void);
Vector<std::string> to_vty_ospf6_router_redistribute_conf_set(const FrrOspf6RouterRedistribute &val);
Vector<std::string> to_vty_ospf6_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t val);

Vector<std::string> to_vty_ospf6_router_spf_throttling_conf_set(const FrrOspf6RouterSPFThrotlling &val);
Vector<std::string> to_vty_ospf6_router_spf_throttling_conf_del(void);

Map<APPL_FrrOspf6RouteKey, APPL_FrrOspf6RouteStatus> frr_ip_ospf6_route_status_parse(const std::string &s);
FrrIpOspf6Status frr_ip_ospf6_status_parse(const std::string &s);
Map<vtss_ifindex_t, FrrIpOspf6IfStatus> frr_ip_ospf6_interface_status_parse(const std::string &s);
Map<mesa_ipv4_t, Vector<FrrIpOspf6NeighborStatus>> frr_ip_ospf6_neighbor_status_parse(const std::string &s);

FrrOspf6DbStatus frr_ip_ospf6_db_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbKey, APPL_FrrOspf6DbLinkStateVal> frr_ip_ospf6_db_parse(const std::string &s);

FrrOspf6DbRouterStatus frr_ip_ospf6_db_router_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbRouterStateVal> frr_ip_ospf6_db_router_parse(const std::string &s);

FrrOspf6DbNetStatus frr_ip_ospf6_db_net_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNetStateVal> frr_ip_ospf6_db_net_parse(const std::string &s);

FrrOspf6DbInterAreaPrefixStatus frr_ip_ospf6_db_inter_area_prefix_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaPrefixStateVal> frr_ip_ospf6_db_inter_area_prefix_parse(const std::string &s);

FrrOspf6DbInterAreaRouterStatus frr_ip_ospf6_db_inter_area_router_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaRouterStateVal> frr_ip_ospf6_db_inter_area_router_parse(const std::string &s);

FrrOspf6DbExternalStatus frr_ip_ospf6_db_external_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbExternalStateVal> frr_ip_ospf6_db_external_parse(const std::string &s);

FrrOspf6DbNSSAExternalStatus frr_ip_ospf6_db_nssa_external_parse_raw(const std::string &s);
Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNSSAExternalStateVal> frr_ip_ospf6_db_nssa_external_parse(const std::string &s);

}  // namespace vtss

#endif  // _FRR_OSPF6_ACCESS_HXX_

