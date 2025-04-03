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

#ifndef _FRR_OSPF_ACCESS_HXX_
#define _FRR_OSPF_ACCESS_HXX_

#include <unistd.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/ip.h>
#include <vtss/appl/ospf.h>
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
typedef uint32_t OspfInst;  // TODO, apply int-tag

// - Sx "show ip ospf route [json]"

//"N IA", "D IA", "N", "R", "N E1", "N E2"
enum FrrOspfRouteType {
    RT_Network,
    RT_NetworkIA,
    RT_DiscardIA,
    RT_Router,
    RT_ExtNetworkTypeOne,
    RT_ExtNetworkTypeTwo
};

enum FrrOspfRouterType {
    RouterType_None,
    RouterType_ABR,
    RouterType_ASBR,
    RouterType_ABR_ASBR
};

struct FrrOspfRouteHopStatus {
    FrrOspfRouteHopStatus(const FrrOspfRouteHopStatus &) = delete;
    FrrOspfRouteHopStatus &operator=(const vtss::FrrOspfRouteHopStatus &) = delete;
    FrrOspfRouteHopStatus(FrrOspfRouteHopStatus &&) = default;
    FrrOspfRouteHopStatus &operator=(FrrOspfRouteHopStatus && ) = default;
    mesa_ipv4_t ip;
    vtss_ifindex_t ifindex;
    bool is_connected;
#ifdef VTSS_BASICS_STANDALONE
    std::string ifname;
#endif
};

struct FrrOspfRouteStatus {
    FrrOspfRouteType route_type;
    uint32_t cost;
    uint32_t ext_cost;
    mesa_ipv4_t area;
    bool is_ia;
    FrrOspfRouterType router_type;
    Vector<FrrOspfRouteHopStatus> next_hops;
};

struct APPL_FrrOspfRouteKey {
    OspfInst inst_id;
    FrrOspfRouteType route_type;
    mesa_ipv4_network_t network;
    mesa_ipv4_t area;
    mesa_ipv4_t nexthop_ip;
};

struct APPL_FrrOspfRouteStatus {
    uint32_t cost;
    uint32_t ext_cost;
    bool is_ia;
    FrrOspfRouterType router_type;
    bool is_connected;
    vtss_ifindex_t ifindex;
#ifdef VTSS_BASICS_STANDALONE
    std::string ifname;
#endif
};

using FrrIpOspfRouteStatusMap =
    Map<APPL_FrrOspfRouteKey, APPL_FrrOspfRouteStatus>;
using FrrIpOspfRouteStatusMapResult = FrrRes<FrrIpOspfRouteStatusMap>;
FrrIpOspfRouteStatusMapResult frr_ip_ospf_route_status_get();

struct FrrIpOspfArea {
    bool backbone;
    bool stub_no_summary;
    bool stub_shortcut;
    bool nssa_translator_elected;
    bool nssa_translator_always;
    int32_t area_if_total_counter;
    int32_t area_if_activ_counter;
    int32_t full_adjancet_counter;
    vtss_appl_ospf_auth_type_t authentication;
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

struct FrrIpOspfStatus {
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
    Map<mesa_ipv4_t, FrrIpOspfArea> areas;
};
FrrRes<FrrIpOspfStatus> frr_ip_ospf_status_get();

// - Sx "show ip ospf interface [INTERFACE] [json]" - 3d PGS-28
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

enum FrrIpOspfIfNetworkType {
    NetworkType_Null = 0,
    NetworkType_PointToPoint = 1,
    NetworkType_Broadcast = 2,
    NetworkType_Nbma = 3,
    NetworkType_PointToMultipoint = 4,
    NetworkType_VirtualLink = 5,
    NetworkType_LoopBack = 6
};

enum FrrIpOspfIfISMState {
    ISM_DependUpon = 0,
    ISM_Down = 1,
    ISM_Loopback = 2,
    ISM_Waiting = 3,
    ISM_PointToPoint = 4,
    ISM_DROther = 5,
    ISM_Backup = 6,
    ISM_DR = 7
};

enum FrrIpOspfIfType { IfType_Peer = 0, IfType_Broadcast = 1 };

struct FrrIpOspfIfStatus {
    FrrIpOspfIfStatus(const FrrIpOspfIfStatus &) = delete;
    FrrIpOspfIfStatus &operator=(const vtss::FrrIpOspfIfStatus &) = delete;
    FrrIpOspfIfStatus(FrrIpOspfIfStatus &&) = default;
    FrrIpOspfIfStatus &operator=(FrrIpOspfIfStatus && ) = default;
    FrrIpOspfIfStatus() = default;
    bool if_up;
    vtss_ifindex_t ifindex;
    int32_t mtu_bytes;
    int32_t bandwidth_mbit;
    std::string if_flags;
    bool ospf_enabled;
    mesa_ipv4_network_t net;
    FrrIpOspfIfType if_type;
    mesa_ipv4_t local_if_used;
    AreaDescription area;
    mesa_ipv4_t router_id;
    FrrIpOspfIfNetworkType network_type;
    uint32_t cost;
    seconds transmit_delay;
    FrrIpOspfIfISMState state;
    int32_t priority;
    mesa_ipv4_t bdr_id;
    mesa_ipv4_t bdr_address;

    /* Next network-LSA sequence number, it is used when we are elected DR */
    uint32_t network_lsa_sequence;

    bool mcast_member_ospf_all_routers;
    bool mcast_member_ospf_designated_routers;
    milliseconds timer;
    milliseconds timer_dead;
    milliseconds timer_wait;
    milliseconds timer_retransmit;
    milliseconds timer_hello;
    bool timer_passive_iface;
    int32_t nbr_count;
    int32_t nbr_adjacent_count;
    mesa_ipv4_t vlink_peer_addr;
};

using FrrIpOspfIfStatusMap = Map<vtss_ifindex_t, FrrIpOspfIfStatus>;
using FrrIpOspfIfStatusMapResult = FrrRes<FrrIpOspfIfStatusMap>;
FrrIpOspfIfStatusMapResult frr_ip_ospf_interface_status_get();

// - Sx "show ip ospf neighbor [json]" - 3d PGS-29 - conf
enum FrrIpOspfNeighborNSMState {
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

struct FrrIpOspfNeighborStatus {
    // Remove the copy constructor
    FrrIpOspfNeighborStatus(const FrrIpOspfNeighborStatus &) = delete;

    // Remove the copy assignment operator
    FrrIpOspfNeighborStatus &operator=(const vtss::FrrIpOspfNeighborStatus &) = delete;

    // Add the move constructor
    FrrIpOspfNeighborStatus(FrrIpOspfNeighborStatus &&) = default;

    // Add the move assignment operator
    FrrIpOspfNeighborStatus &operator=(FrrIpOspfNeighborStatus && ) = default;

    mesa_ipv4_t if_address;
    AreaDescription area;
    AreaDescription transit_id;
    vtss_ifindex_t ifindex;
    int32_t nbr_priority;
    FrrIpOspfNeighborNSMState nbr_state;

    /* Notice that the value of "routerDesignatedId" in FRR JSON output
     * represents the IP address of DR on the network. */
    mesa_ipv4_t dr_ip_addr;

    /* Notice that the value of "routerDesignatedBackupId" in FRR JSON output
     * represents the IP address of BDR on the network. */
    mesa_ipv4_t bdr_ip_addr;

    std::string options_list;
    int32_t options_counter;
    milliseconds router_dead_interval_timer_due;
};

using FrrIpOspfNbrStatusMap = Map<mesa_ipv4_t, Vector<FrrIpOspfNeighborStatus>>;
using FrrIpOspfNbrStatusMapResult = FrrRes<FrrIpOspfNbrStatusMap>;

// the key is the neighbor router id
FrrIpOspfNbrStatusMapResult frr_ip_ospf_neighbor_status_get();

// - Sx "show ip ospf database [json]"
enum FrrOspfLsdbType {
    FrrOspfLsdbType_None = 0,
    FrrOspfLsdbType_Router = 1,
    FrrOspfLsdbType_Network = 2,
    FrrOspfLsdbType_Summary = 3,
    FrrOspfLsdbType_AsbrSummary = 4,
    FrrOspfLsdbType_AsExternal = 5,
    FrrOspfLsdbType_Nssa = 7,
    FrrOspfLsdbType_Opaque = 9,
    FrrOspfLsdbType_OpaqueArea = 10,
    FrrOspfLsdbType_OpaqueAs = 11
};

struct APPL_FrrOspfDbKey {
    OspfInst inst_id;
    mesa_ipv4_t area_id;
    int32_t type;
    mesa_ipv4_t link_id;
    mesa_ipv4_t adv_router;
};

struct APPL_FrrOspfDbLinkStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    uint32_t sequence;
    int32_t checksum;
    int32_t router_link_count;          // Optional: used by type 1 only
    mesa_ipv4_network_t summary_route;  // Optional: used by type 3 only
    int32_t external;  // Optional: used by type 5 and type 7 only
    int32_t tag;       // Optional: used by type 5 and type 7 only
};

struct FrrOspfDbLinkVal {
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

struct FrrOspfDbLinkStates {
    int32_t type;
    std::string desc;
    AreaDescription area_id;
    Vector<FrrOspfDbLinkVal> links;
};

struct FrrOspfDbStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbLinkStates> area;
    FrrOspfDbLinkStates type5;
    FrrOspfDbLinkStates type7;
};

FrrRes<FrrOspfDbStatus> frr_ip_ospf_db_get_pure(const OspfInst &id);
FrrRes<Map<APPL_FrrOspfDbKey, APPL_FrrOspfDbLinkStateVal>> frr_ip_ospf_db_get(
                                                            const OspfInst &id);

// OSPF db common part

struct APPL_FrrOspfDbCommonKey {
    OspfInst inst_id;
    mesa_ipv4_t area_id;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
};

// - Sx "show ip ospf database router [json]"

struct FrrOspfDbRouterAreasRoutesLinksVal {
    int32_t link_connected_to;
    mesa_ipv4_t link_id;
    mesa_ipv4_t link_data;
    int32_t metric;
};

struct APPL_FrrOspfDbRouterStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspfDbRouterAreasRoutesLinksVal> links;
};

struct FrrOspfDbRouterAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    Vector<FrrOspfDbRouterAreasRoutesLinksVal> links;
};

struct FrrOspfDbRouterAreasStates {
    AreaDescription area_id;
    Vector<FrrOspfDbRouterAreasRoutesStates> routes;
};

struct FrrOspfDbRouterStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbRouterAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbRouterStateVal>>
                                                                frr_ip_ospf_db_router_get(const OspfInst &id);

// - Sx "show ip ospf database network [json]"

struct APPL_FrrOspfDbNetStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    Vector<mesa_ipv4_t> attached_router;
};

struct FrrOspfDbNetAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    Vector<mesa_ipv4_t> attached_router;
};

struct FrrOspfDbNetAreasStates {
    AreaDescription area_id;
    Vector<FrrOspfDbNetAreasRoutesStates> routes;
};

struct FrrOspfDbNetStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbNetAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNetStateVal>>
                                                             frr_ip_ospf_db_net_get(const OspfInst &id);

// - Sx "show ip ospf database summary [json]"

struct APPL_FrrOspfDbSummaryStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric;
};

struct FrrOspfDbSummaryAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric;
};

struct FrrOspfDbSummaryAreasStates {
    AreaDescription area_id;
    Vector<FrrOspfDbSummaryAreasRoutesStates> routes;
};

struct FrrOspfDbSummaryStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbSummaryAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbSummaryStateVal>>
                                                                 frr_ip_ospf_db_summary_get(const OspfInst &id);

// - Sx "show ip ospf database asbr-summary [json]"

struct APPL_FrrOspfDbASBRSummaryStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric;
};

struct FrrOspfDbASBRSummaryAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric;
};

struct FrrOspfDbASBRSummaryAreasStates {
    AreaDescription area_id;
    Vector<FrrOspfDbASBRSummaryAreasRoutesStates> routes;
};

struct FrrOspfDbASBRSummaryStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbASBRSummaryAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbASBRSummaryStateVal>>
                                                                     frr_ip_ospf_db_asbr_summary_get(const OspfInst &id);

// - Sx "show ip ospf database external [json]"

struct APPL_FrrOspfDbExternalStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv4_t forward_address;
};

struct FrrOspfDbExternalAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv4_t forward_address;
};

struct FrrOspfDbExternalRouteStatus {
    Vector<FrrOspfDbExternalAreasRoutesStates> routes;
};

struct FrrOspfDbExternalStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbExternalRouteStatus> type5;
};

FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbExternalStateVal>>
                                                                  frr_ip_ospf_db_external_get(const OspfInst &id);

// - Sx "show ip ospf database nssa-external [json]"

struct APPL_FrrOspfDbNSSAExternalStateVal {
    mesa_ipv4_t router_id;
    int32_t age;
    std::string options;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv4_t forward_address;
};

struct FrrOspfDbNSSAExternalAreasRoutesStates {
    int32_t age;
    std::string options;
    FrrOspfLsdbType type;
    mesa_ipv4_t link_state_id;
    mesa_ipv4_t adv_router;
    uint32_t sequence;
    int32_t checksum;
    int32_t length;
    int32_t network_mask;
    int32_t metric_type;
    int32_t metric;
    mesa_ipv4_t forward_address;
};

struct FrrOspfDbNSSAExternalAreasStates {
    AreaDescription area_id;
    Vector<FrrOspfDbNSSAExternalAreasRoutesStates> routes;
};

struct FrrOspfDbNSSAExternalStatus {
    mesa_ipv4_t router_id;
    Vector<FrrOspfDbNSSAExternalAreasStates> areas;
};

FrrRes<Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNSSAExternalStateVal>> frr_ip_ospf_db_nssa_external_get(const OspfInst &id);

// OSPF Router configuration ---------------------------------------------------

enum AbrType { AbrType_Cisco, AbrType_IBM, AbrType_Shortcut, AbrType_Standard };
struct FrrOspfRouterConf {
    vtss::Optional<mesa_ipv4_t> ospf_router_id;
    vtss::Optional<AbrType> ospf_router_abr_type;
    vtss::Optional<bool> ospf_router_rfc1583;
};

FrrRes<FrrOspfRouterConf> frr_ospf_router_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_router_conf_set(const OspfInst &id);
mesa_rc frr_ospf_router_conf_set(const OspfInst &id, const FrrOspfRouterConf &conf);
mesa_rc frr_ospf_router_conf_del(const OspfInst &id);

FrrRes<bool> frr_ospf_router_passive_if_default_get(std::string &running_conf, const OspfInst &id);
mesa_rc      frr_ospf_router_passive_if_default_set(const OspfInst &id, bool enable);
FrrRes<bool> frr_ospf_router_passive_if_conf_get(std::string &running_conf, const OspfInst &id, const vtss_ifindex_t &i);
mesa_rc      frr_ospf_router_passive_if_conf_set(const OspfInst &id, const vtss_ifindex_t &i, bool passive);
mesa_rc      frr_ospf_router_passive_if_disable_all(std::string &running_conf, const OspfInst &id);
mesa_rc      frr_ospf_router_default_metric_conf_set(const OspfInst &id, uint32_t val);
mesa_rc      frr_ospf_router_default_metric_conf_del(const OspfInst &id);

FrrRes<Optional<uint32_t>> frr_ospf_router_default_metric_conf_get(std::string &running_conf, const OspfInst &id);

enum FrrOspfRouterMetricType {MetricType_One = 1, MetricType_Two};

struct FrrOspfRouterRedistribute {
    vtss_appl_ip_route_protocol_t protocol;
    FrrOspfRouterMetricType       metric_type = MetricType_Two;
    vtss::Optional<uint32_t>      metric;
    vtss::Optional<std::string>   route_map;
};

Vector<FrrOspfRouterRedistribute> frr_ospf_router_redistribute_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_router_redistribute_conf_set(const OspfInst &id, const FrrOspfRouterRedistribute &val);
mesa_rc frr_ospf_router_redistribute_conf_del(const OspfInst &id, vtss_appl_ip_route_protocol_t val);

//----------------------------------------------------------------------------
//** OSPF administrative distance
//----------------------------------------------------------------------------
FrrRes<uint8_t> frr_ospf_router_admin_distance_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_router_admin_distance_set(const OspfInst &id, uint8_t val);
Vector<std::string> to_vty_ospf_router_admin_distance_set(uint8_t val);

//----------------------------------------------------------------------------
//** OSPF default route configuration
//----------------------------------------------------------------------------
struct FrrOspfRouterDefaultRoute {
    FrrOspfRouterDefaultRoute() = default;
    vtss::Optional<bool> always;
    vtss::Optional<uint32_t> metric;
    vtss::Optional<FrrOspfRouterMetricType> metric_type;
    vtss::Optional<std::string> route_map;
};

FrrRes<FrrOspfRouterDefaultRoute> frr_ospf_router_default_route_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_router_default_route_conf_set(const OspfInst &id, const FrrOspfRouterDefaultRoute &val);
mesa_rc frr_ospf_router_default_route_conf_del(const OspfInst &id);

//----------------------------------------------------------------------------
//** OSPF stub router
//----------------------------------------------------------------------------
struct FrrOspfRouterStubRouter {
    FrrOspfRouterStubRouter() = default;
    vtss::Optional<uint32_t> on_startup_interval;
    vtss::Optional<uint32_t> on_shutdown_interval;
    bool is_administrative;
};
FrrRes<FrrOspfRouterStubRouter> frr_ospf_router_stub_router_conf_get(std::string &running_conf, const OspfInst &id);

mesa_rc frr_ospf_router_stub_router_conf_set(const OspfInst &id, const FrrOspfRouterStubRouter &val);
mesa_rc frr_ospf_router_stub_router_conf_del(const OspfInst &id, const FrrOspfRouterStubRouter &val);

//----------------------------------------------------------------------------
//** OSPF SPF throttling
//----------------------------------------------------------------------------
struct FrrOspfRouterSPFThrotlling {
    FrrOspfRouterSPFThrotlling() = default;
    uint32_t delay;
    uint32_t init_holdtime;
    uint32_t max_holdtime;
};

FrrRes<FrrOspfRouterSPFThrotlling> frr_ospf_router_spf_throttling_conf_get(std::string &running_conf, const OspfInst &id);

mesa_rc frr_ospf_router_spf_throttling_conf_set(const OspfInst &id, const FrrOspfRouterSPFThrotlling &val);
mesa_rc frr_ospf_router_spf_throttling_conf_del(const OspfInst &id);

// OSPF Area configuration -----------------------------------------------------
struct FrrOspfAreaNetwork {
    FrrOspfAreaNetwork() = default;
    FrrOspfAreaNetwork(const mesa_ipv4_network_t &n, const mesa_ipv4_t &a)
        : net {n}, area {a} {}

    mesa_ipv4_network_t net;
    mesa_ipv4_t area;
};

Vector<FrrOspfAreaNetwork> frr_ospf_area_network_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_network_conf_set(const OspfInst &id, const FrrOspfAreaNetwork &val);
mesa_rc frr_ospf_area_network_conf_del(const OspfInst &id, const FrrOspfAreaNetwork &net);

Vector<FrrOspfAreaNetwork> frr_ospf_area_range_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_range_conf_set(const OspfInst &id, const FrrOspfAreaNetwork &val);
mesa_rc frr_ospf_area_range_conf_del(const OspfInst &id, const FrrOspfAreaNetwork &val);

Vector<FrrOspfAreaNetwork> frr_ospf_area_range_not_advertise_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_range_not_advertise_conf_set(const OspfInst &id, const FrrOspfAreaNetwork &val);
mesa_rc frr_ospf_area_range_not_advertise_conf_del(const OspfInst &id, const FrrOspfAreaNetwork &val);

struct FrrOspfAreaNetworkCost {
    FrrOspfAreaNetworkCost() = default;
    FrrOspfAreaNetworkCost(const mesa_ipv4_network_t &n, const mesa_ipv4_t &a, uint32_t c)
        : net {n}, area {a}, cost {c} {}

    mesa_ipv4_network_t net;
    mesa_ipv4_t area;
    uint32_t cost;
};

Vector<FrrOspfAreaNetworkCost> frr_ospf_area_range_cost_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_range_cost_conf_set(const OspfInst &id, const FrrOspfAreaNetworkCost &val);
mesa_rc frr_ospf_area_range_cost_conf_del(const OspfInst &id, const FrrOspfAreaNetworkCost &val);

struct FrrOspfAreaVirtualLink {
    FrrOspfAreaVirtualLink() = default;
    FrrOspfAreaVirtualLink(const mesa_ipv4_t &a, const mesa_ipv4_t &dst_ip)
        : area {a}, dst {dst_ip} {}

    mesa_ipv4_t area;
    mesa_ipv4_t dst;
    vtss::Optional<uint16_t> hello_interval;
    vtss::Optional<uint16_t> retransmit_interval;
    vtss::Optional<uint16_t> transmit_delay;
    vtss::Optional<uint16_t> dead_interval;
};

Vector<FrrOspfAreaVirtualLink> frr_ospf_area_virtual_link_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_virtual_link_conf_set(const OspfInst &id, const FrrOspfAreaVirtualLink &val);
mesa_rc frr_ospf_area_virtual_link_conf_del(const OspfInst &id, const FrrOspfAreaVirtualLink &val);

//----------------------------------------------------------------------------
//** OSPF NSSA
//----------------------------------------------------------------------------
enum FrrOspfNssaTranslatorRole {
    NssaTranslatorRoleCandidate,
    NssaTranslatorRoleAlways,
    NssaTranslatorRoleNever
};

struct FrrOspfStubArea {
    FrrOspfStubArea() = default;
    FrrOspfStubArea(const mesa_ipv4_t &a, bool n, bool s, FrrOspfNssaTranslatorRole r)
        : area {a}, is_nssa {n}, no_summary {s}, nssa_translator_role {r} {}

    mesa_ipv4_t area;
    bool is_nssa;
    bool no_summary;
    FrrOspfNssaTranslatorRole nssa_translator_role;
};

Vector<FrrOspfStubArea> frr_ospf_stub_area_conf_get(std::string &running_conf, const OspfInst &id);

enum FrrOspfAuthMode {
    FRR_OSPF_AUTH_MODE_AREA_CFG,
    FRR_OSPF_AUTH_MODE_NULL,
    FRR_OSPF_AUTH_MODE_PWD,
    FRR_OSPF_AUTH_MODE_MSG_DIGEST,
};

struct FrrOspfAreaAuth {
    FrrOspfAreaAuth() = default;
    FrrOspfAreaAuth(const mesa_ipv4_t &a, FrrOspfAuthMode mode)
        : area {a}, auth_mode {mode} {}

    mesa_ipv4_t area;
    FrrOspfAuthMode auth_mode;
};

Vector<FrrOspfAreaAuth> frr_ospf_area_authentication_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_stub_area_conf_set(const OspfInst &id, const FrrOspfStubArea &conf);
mesa_rc frr_ospf_stub_area_conf_del(const OspfInst &id, const mesa_ipv4_t &area);

//----------------------------------------------------------------------------
//** OSPF area authentication
//----------------------------------------------------------------------------
mesa_rc frr_ospf_area_authentication_conf_set(const OspfInst &id,
                                              const FrrOspfAreaAuth &val);

struct FrrOspfDigestData {
    FrrOspfDigestData(const uint8_t kid, const std::string &kval)
        : keyid {kid}, key {kval} {}

    uint8_t keyid;
    std::string key;
};

struct FrrOspfAreaVirtualLinkAuth {
    FrrOspfAreaVirtualLinkAuth() = default;
    FrrOspfAreaVirtualLinkAuth(const FrrOspfAreaVirtualLink &link,
                               FrrOspfAuthMode mode)
        : virtual_link {link}, auth_mode {mode} {}

    FrrOspfAreaVirtualLink virtual_link;
    FrrOspfAuthMode auth_mode;
};

Vector<FrrOspfAreaVirtualLinkAuth> frr_ospf_area_virtual_link_authentication_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_virtual_link_authentication_conf_set( const OspfInst &id, const FrrOspfAreaVirtualLinkAuth &val);

struct FrrOspfAreaVirtualLinkDigest {
    FrrOspfAreaVirtualLinkDigest() = default;
    FrrOspfAreaVirtualLinkDigest(const FrrOspfAreaVirtualLink &link,
                                 const FrrOspfDigestData &data)
        : virtual_link {link}, digest_data {data} {}

    FrrOspfAreaVirtualLink virtual_link;
    FrrOspfDigestData digest_data;
};

Vector<FrrOspfAreaVirtualLinkDigest> frr_ospf_area_virtual_link_message_digest_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_virtual_link_message_digest_conf_set(const OspfInst &id, const FrrOspfAreaVirtualLink &val, const FrrOspfDigestData &data);
mesa_rc frr_ospf_area_virtual_link_message_digest_conf_del(const OspfInst &id, const FrrOspfAreaVirtualLink &val, const FrrOspfDigestData &data);

struct FrrOspfAreaVirtualLinkKey {
    FrrOspfAreaVirtualLinkKey() = default;
    FrrOspfAreaVirtualLinkKey(const FrrOspfAreaVirtualLink &link, const std::string key)
        : virtual_link {link}, key_data {key} {}

    FrrOspfAreaVirtualLink virtual_link;
    std::string key_data;
};

Vector<FrrOspfAreaVirtualLinkKey> frr_ospf_area_virtual_link_authentication_key_conf_get(std::string &running_conf, const OspfInst &id);
mesa_rc frr_ospf_area_virtual_link_authentication_key_conf_set(const OspfInst &id, const FrrOspfAreaVirtualLink &val, const std::string &auth_key);
mesa_rc frr_ospf_area_virtual_link_authentication_key_conf_del(const OspfInst &id, const FrrOspfAreaVirtualLinkKey &val);

// OSPF Interface Configuration
mesa_rc frr_ospf_if_mtu_ignore_conf_set(vtss_ifindex_t i, bool);
FrrRes<bool> frr_ospf_if_mtu_ignore_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);

struct FrrOspfDeadInterval {
    FrrOspfDeadInterval() = default;
    FrrOspfDeadInterval(bool mul, uint32_t v) : multiplier {mul}, val {v} {}

    bool multiplier;
    uint32_t val;
};

FrrRes<FrrOspfDeadInterval> frr_ospf_if_dead_interval_conf_get(std::string &running_config, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_dead_interval_conf_set(vtss_ifindex_t i, uint32_t val);
mesa_rc frr_ospf_if_dead_interval_minimal_conf_set(vtss_ifindex_t ifindex, uint32_t val);
mesa_rc frr_ospf_if_dead_interval_conf_del(vtss_ifindex_t ifindex);

FrrRes<uint32_t> frr_ospf_if_hello_interval_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_hello_interval_conf_set(vtss_ifindex_t ifindex, uint32_t val);
mesa_rc frr_ospf_if_hello_interval_conf_del(vtss_ifindex_t ifindex);

FrrRes<uint32_t> frr_ospf_if_retransmit_interval_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_retransmit_interval_conf_set(vtss_ifindex_t i, uint32_t val);
mesa_rc frr_ospf_if_retransmit_interval_conf_del(vtss_ifindex_t i);

FrrRes<uint32_t> frr_ospf_if_cost_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_cost_conf_set(vtss_ifindex_t ifindex, uint32_t val);
mesa_rc frr_ospf_if_cost_conf_del(vtss_ifindex_t ifindex);

FrrRes<std::string> frr_ospf_if_authentication_key_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_authentication_key_conf_set(vtss_ifindex_t ifindex, const std::string &auth_key);
mesa_rc frr_ospf_if_authentication_key_conf_del(vtss_ifindex_t ifindex);

FrrRes<FrrOspfAuthMode> frr_ospf_if_authentication_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_authentication_conf_set(vtss_ifindex_t ifindex, FrrOspfAuthMode mode);

Vector<FrrOspfDigestData> frr_ospf_if_message_digest_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_message_digest_conf_set(vtss_ifindex_t ifindex, const FrrOspfDigestData &data);
mesa_rc frr_ospf_if_message_digest_conf_del(vtss_ifindex_t ifindex, uint8_t keyid);

FrrRes<uint8_t> frr_ospf_if_priority_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_ospf_if_priority_conf_set(vtss_ifindex_t i, uint8_t val);

mesa_rc frr_ospf_if_del(vtss_ifindex_t ifindex);

/******************************************************************************/
/** For unit-test purpose                                                     */
/******************************************************************************/
bool operator<(const APPL_FrrOspfRouteKey    &a, const APPL_FrrOspfRouteKey    &b);
bool operator<(const APPL_FrrOspfDbKey       &a, const APPL_FrrOspfDbKey       &b);
bool operator<(const APPL_FrrOspfDbCommonKey &a, const APPL_FrrOspfDbCommonKey &b);

Vector<std::string> to_vty_ospf_router_conf_del(void);
Vector<std::string> to_vty_ospf_router_conf_set(void);
Vector<std::string> to_vty_ospf_router_conf_set(const FrrOspfRouterConf &conf);
Vector<std::string> to_vty_ospf_area_network_conf_set(const FrrOspfAreaNetwork &val);
Vector<std::string> to_vty_ospf_area_network_conf_del(const FrrOspfAreaNetwork &val);
Vector<std::string> to_vty_ospf_area_authentication_conf_set(const mesa_ipv4_t &area, FrrOspfAuthMode mode);
Vector<std::string> to_vty_ospf_router_passive_if_default(bool enable);
Vector<std::string> to_vty_ospf_router_passive_if_conf_set(const std::string &ifname, bool passive);
Vector<std::string> to_vty_ospf_if_field_conf_set(const std::string &ifname, const std::string &field, uint16_t val);
Vector<std::string> to_vty_ospf_if_field_conf_del(const std::string &ifname, const std::string &field);
Vector<std::string> to_vty_ospf_area_range_conf_set(const FrrOspfAreaNetwork &val);
Vector<std::string> to_vty_ospf_area_range_conf_del(const FrrOspfAreaNetwork &val);
Vector<std::string> to_vty_ospf_area_range_not_advertise_conf_set(const FrrOspfAreaNetwork &val);
Vector<std::string> to_vty_ospf_area_range_not_advertise_conf_del(const FrrOspfAreaNetwork &val);
Vector<std::string> to_vty_ospf_area_range_cost_conf_set(const FrrOspfAreaNetworkCost &val);
Vector<std::string> to_vty_ospf_area_range_cost_conf_del(const FrrOspfAreaNetworkCost &val);
Vector<std::string> to_vty_ospf_area_virtual_link_conf_set(const FrrOspfAreaVirtualLink &val);
Vector<std::string> to_vty_ospf_area_virtual_link_conf_del(const FrrOspfAreaVirtualLink &val);
Vector<std::string> to_vty_ospf_stub_area_conf_set(const FrrOspfStubArea &conf);
Vector<std::string> to_vty_ospf_stub_area_conf_del(const mesa_ipv4_t &area);
Vector<std::string> to_vty_ospf_if_authentication_key_del(const std::string &ifname);
Vector<std::string> to_vty_ospf_if_authentication_key_set(const std::string &ifname, const std::string &key);
Vector<std::string> to_vty_ospf_if_authentication_set(const std::string &ifname, FrrOspfAuthMode mode);
Vector<std::string> to_vty_ospf_if_message_digest_set(const std::string &ifname, const FrrOspfDigestData &data);
Vector<std::string> to_vty_ospf_if_message_digest_del(const std::string &ifname, uint8_t keyid);
Vector<std::string> to_vty_ospf_area_stub_no_summary_conf_set(const mesa_ipv4_t &area);
Vector<std::string> to_vty_ospf_area_stub_no_summary_conf_del(const mesa_ipv4_t &area);
Vector<std::string> to_vty_ospf_area_virtual_link_authentication_conf_set(const FrrOspfAreaVirtualLink &val, FrrOspfAuthMode mode);
Vector<std::string> to_vty_ospf_area_virtual_link_message_digest_conf_set(const FrrOspfAreaVirtualLink &val, const FrrOspfDigestData &data);
Vector<std::string> to_vty_ospf_area_virtual_link_message_digest_conf_del(const FrrOspfAreaVirtualLink &val, const FrrOspfDigestData &data);
Vector<std::string> to_vty_ospf_area_virtual_link_authentication_key_conf_set(const FrrOspfAreaVirtualLink &val, const std::string &auth_key);
Vector<std::string> to_vty_ospf_area_virtual_link_authentication_key_conf_del(const FrrOspfAreaVirtualLinkKey &val);
Vector<std::string> to_vty_ospf_router_default_metric_conf_set(uint32_t val);
Vector<std::string> to_vty_ospf_router_default_metric_conf_del(void);
Vector<std::string> to_vty_ospf_router_default_route_conf_set(const FrrOspfRouterDefaultRoute &val);
Vector<std::string> to_vty_ospf_router_default_route_conf_del(void);
Vector<std::string> to_vty_ospf_router_redistribute_conf_set(const FrrOspfRouterRedistribute &val);
Vector<std::string> to_vty_ospf_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t val);

Vector<std::string> to_vty_ospf_router_stub_router_conf_set(const FrrOspfRouterStubRouter &val);
Vector<std::string> to_vty_ospf_router_stub_router_conf_del(const FrrOspfRouterStubRouter &val);
Vector<std::string> to_vty_ospf_router_spf_throttling_conf_set(const FrrOspfRouterSPFThrotlling &val);
Vector<std::string> to_vty_ospf_router_spf_throttling_conf_del(void);
Vector<std::string> to_vty_ospf_router_passive_if_disable_all(const std::string &running_conf);

Map<APPL_FrrOspfRouteKey, APPL_FrrOspfRouteStatus> frr_ip_ospf_route_status_parse(const std::string &s);
FrrIpOspfStatus frr_ip_ospf_status_parse(const std::string &s);
Map<vtss_ifindex_t, FrrIpOspfIfStatus> frr_ip_ospf_interface_status_parse(const std::string &s);
Map<mesa_ipv4_t, Vector<FrrIpOspfNeighborStatus>> frr_ip_ospf_neighbor_status_parse(const std::string &s);

FrrOspfDbStatus frr_ip_ospf_db_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbKey, APPL_FrrOspfDbLinkStateVal> frr_ip_ospf_db_parse(const std::string &s);

FrrOspfDbRouterStatus frr_ip_ospf_db_router_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbRouterStateVal> frr_ip_ospf_db_router_parse(const std::string &s);

FrrOspfDbNetStatus frr_ip_ospf_db_net_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNetStateVal> frr_ip_ospf_db_net_parse(const std::string &s);

FrrOspfDbSummaryStatus frr_ip_ospf_db_summary_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbSummaryStateVal> frr_ip_ospf_db_summary_parse(const std::string &s);

FrrOspfDbASBRSummaryStatus frr_ip_ospf_db_asbr_summary_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbASBRSummaryStateVal> frr_ip_ospf_db_asbr_summary_parse(const std::string &s);

FrrOspfDbExternalStatus frr_ip_ospf_db_external_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbExternalStateVal> frr_ip_ospf_db_external_parse(const std::string &s);

FrrOspfDbNSSAExternalStatus frr_ip_ospf_db_nssa_external_parse_raw(const std::string &s);
Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNSSAExternalStateVal> frr_ip_ospf_db_nssa_external_parse(const std::string &s);

}  // namespace vtss

#endif  // _FRR_OSPF_ACCESS_HXX_

