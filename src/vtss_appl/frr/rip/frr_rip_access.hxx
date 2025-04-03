/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _FRR_RIP_ACCESS_HXX_
#define _FRR_RIP_ACCESS_HXX_

#include <unistd.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/ip.h>
#include <vtss/appl/rip.h>
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
//----------------------------------------------------------------------------
//** Enable/Disable RIP router mode
//----------------------------------------------------------------------------
/* Enable/Disable RIP router mode */
mesa_rc frr_rip_router_conf_set(bool is_enable);

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
struct FrrRipNetwork {
    FrrRipNetwork() = default;
    FrrRipNetwork(const mesa_ipv4_network_t &n) : net {n} {}

    mesa_ipv4_network_t net;
};

Vector<FrrRipNetwork> frr_rip_network_conf_get(std::string &running_conf);
mesa_rc frr_rip_network_conf_set(const FrrRipNetwork &val);
mesa_rc frr_rip_network_conf_del(const FrrRipNetwork &net);

/** For unit-test purpose */
Vector<std::string> to_vty_rip_router_conf_set(bool is_enable);
Vector<std::string> to_vty_rip_network_conf_set(const FrrRipNetwork &val);
Vector<std::string> to_vty_rip_network_conf_del(const FrrRipNetwork &val);

//----------------------------------------------------------------------------
//** RIP passive interface
//----------------------------------------------------------------------------
/* Set RIP passive interface mode on a specific interface.
 * The function must not be invoked when RIP is disabled.
 */
mesa_rc frr_rip_router_passive_if_conf_set(const vtss_ifindex_t &i, bool passive);

/* Get RIP passive interface mode for a specific interface.
 * This function will return the passive interface mode.
 * The returned value is independent upon passive interface default mode.
 * On the other words, the caller can get the passive interface mode on the
 * interface no matter the default mode is enabled or not.
 */
FrrRes<bool> frr_rip_router_passive_if_conf_get(std::string &running_conf, const vtss_ifindex_t &ifindex);

//----------------------------------------------------------------------------
//** RIP timers configuration
//----------------------------------------------------------------------------
struct FrrRipRouterTimersConf {
    uint32_t update_timer;
    uint32_t invalid_timer;
    uint32_t garbage_collection_timer;
};

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
Set<mesa_ipv4_t> frr_rip_neighbor_conf_get(std::string &running_conf);

/* Set RIP neighbor connection.
 * The function must not be invoked when RIP is disabled.
 * FRR accepts all IPv4 addresses as the neigbhhor connection parameter, so the
 * caller must validate if 'neighbor_addr' is valid or not.
 */
mesa_rc frr_rip_neighbor_conf_set(const mesa_ipv4_t neighbor_addr);
mesa_rc frr_rip_neighbor_conf_del(const mesa_ipv4_t neighbor_addr);

/** For unit-test purpose */
Vector<std::string> to_vty_rip_neighbor_conf_set(const mesa_ipv4_t neighbor_addr);
Vector<std::string> to_vty_rip_neighbor_conf_del(const mesa_ipv4_t neighbor_addr);

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
enum FrrRipIfAuthMode {
    FrrRipIfAuthMode_Null,
    FrrRipIfAuthMode_Pwd,
    FrrRipIfAuthMode_MsgDigest
};

struct FrrRipIfAuthConfig {
    FrrRipIfAuthMode auth_mode = FrrRipIfAuthMode_Null;
    std::string simple_pwd;
    std::string keychain_name;
};

/* Set/Erase RIP authentication key-chain setting */
mesa_rc frr_rip_if_authentication_key_chain_set(const vtss_ifindex_t ifindex, const std::string &keychain_name, const bool is_delete);

/* Set/Erase RIP authentication simple password setting */
mesa_rc frr_rip_if_authentication_simple_pwd_set(const vtss_ifindex_t ifindex, const std::string &simple_pwd, const bool is_delete);

/* Get/Set RIP authentication mode */
FrrRes<FrrRipIfAuthMode> frr_rip_if_authentication_mode_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_rip_if_authentication_mode_conf_set(const vtss_ifindex_t ifindex, const FrrRipIfAuthMode mode);

/** for uni-test only */
/* Set/Erase RIP authentication key chain setting */
Vector<std::string> to_vty_rip_if_authentication_key_chain_set(const std::string &ifname, const std::string &keychain_name, const bool is_delete);

/* Set/Erase RIP authentication simple password setting */
Vector<std::string> to_vty_rip_if_authentication_simple_pwd_set(const std::string &ifname, const std::string &simple_pwd, const bool is_delete);

/* Set RIP authentication mode */
Vector<std::string> to_vty_rip_if_authentication_mode_conf_set(const std::string &ifname, const FrrRipIfAuthMode mode);

/* Get RIP authentication configuration */
FrrRes<FrrRipIfAuthConfig> frr_rip_if_authentication_conf_get(std::string &running_conf, const vtss_ifindex_t ifindex);

//----------------------------------------------------------------------------
//** RIP interface configuration: Version support
//----------------------------------------------------------------------------
/* The enumeration for the RIP version. */
enum FrrRipVer {
    FrrRipVer_None,
    FrrRipVer_1,
    FrrRipVer_2,
    FrrRipVer_Both,
    FrrRipVer_NotSpecified,
    FrrRipVer_End
};

struct FrrRipConfIntfVer {
    vtss::Optional<FrrRipVer> recv_ver;
    vtss::Optional<FrrRipVer> send_ver;
};

/* Set the RIP interface version configuration */
mesa_rc frr_rip_intf_ver_conf_set(vtss_ifindex_t ifindex, FrrRipConfIntfVer &intf_ver_conf);

/* Get the RIP interface version configuration */
FrrRes<FrrRipConfIntfVer> frr_rip_intf_ver_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);

/* (For unit-test purpose)
 * Convert RIP interface version configuration to FRR VTY command */
Vector<std::string> to_vty_rip_intf_recv_ver_set(const std::string &intf_name, FrrRipVer recv_ver);
Vector<std::string> to_vty_rip_intf_send_ver_set(const std::string &intf_name, FrrRipVer send_ver);

//----------------------------------------------------------------------------
//** RIP interface configuration: Split horizon
//----------------------------------------------------------------------------
enum FrrRipIfSplitHorizonMode {
    FRR_RIP_IF_SPLIT_HORIZON_MODE_SIMPLE,
    FRR_RIP_IF_SPLIT_HORIZON_MODE_POISONED_REVERSE,
    FRR_RIP_IF_SPLIT_HORIZON_MODE_DISABLED,
};

FrrRes<FrrRipIfSplitHorizonMode> frr_rip_if_split_horizon_conf_get(std::string &running_conf, vtss_ifindex_t ifindex);
mesa_rc frr_rip_if_split_horizon_conf_set(vtss_ifindex_t ifindex, FrrRipIfSplitHorizonMode mode);
/* for uni-test only */
Vector<std::string> to_vty_rip_if_split_horizon_set(const std::string &ifname, FrrRipIfSplitHorizonMode mode);

//----------------------------------------------------------------------------
//** RIP metric manipulation
//----------------------------------------------------------------------------
// offset-list list_name in 4 eth0
/* The enumeration for the direction of RIP metric manipulation. */
enum FrrRipOffsetListDirection {
    FrrRipOffsetListDirection_In,
    FrrRipOffsetListDirection_Out,
    FrrRipOffsetListDirection_End
};

struct FrrRipOffsetList {
    std::string name;
    FrrRipOffsetListDirection mode;
    uint8_t metric;
    vtss::Optional<vtss_ifindex_t> ifindex;
};

Vector<FrrRipOffsetList> frr_rip_offset_list_conf_get(std::string &running_conf);
mesa_rc frr_rip_offset_list_conf_set(const FrrRipOffsetList &val);
mesa_rc frr_rip_offset_list_conf_del(FrrRipOffsetList &val);

struct FrrRipOffsetListKey {
    vtss_ifindex_t ifindex;
    FrrRipOffsetListDirection mode;
};

struct FrrRipOffsetListData {
    std::string name;
    uint8_t metric;
};

using FrrRipOffsetListMap = Map<FrrRipOffsetListKey, FrrRipOffsetListData>;
FrrRipOffsetListMap frr_rip_offset_list_conf_get_map(std::string &running_conf);

/** For unit-test purpose */
Vector<std::string> to_vty_offset_list_conf_set(const FrrRipOffsetList &val, bool is_set);
bool operator<(const FrrRipOffsetListKey &a, const FrrRipOffsetListKey &b);

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
/* The data structure for the RIP general status data. */
struct FrrRipGeneralStatus {
    uint32_t updateTimer;
    uint32_t invalidTimer;
    uint32_t garbageTimer;
    uint32_t updateRemainTime;
    uint8_t default_metric;
    FrrRipVer sendVer;
    FrrRipVer recvVer;
    uint8_t default_distance;
    uint32_t globalRouteChanges;
    uint32_t globalQueries;
};

/* Get the RIP general status */
FrrRes<FrrRipGeneralStatus> frr_rip_general_status_get(void);

/* (For unit-test purpose)
 * Parse the RIP general status from VTY output string */
FrrRipGeneralStatus frr_rip_status_parse(const std::string &vty_output);

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
/* The data structure for the RIP interface status data. */
struct FrrRipActiveIfStatus {
    FrrRipVer sendVer;
    FrrRipVer recvVer;
    std::string key_chain;
    vtss_appl_rip_auth_type_t auth_type;
    bool is_passive_intf;
    uint32_t ifStatRcvBadPackets;
    uint32_t ifStatRcvBadRoutes;
    uint32_t ifStatSentUpdates;
};

/* Get the RIP interface information */
using FrrRipActiveIfStatusMap = Map<vtss_ifindex_t, FrrRipActiveIfStatus>;
FrrRes<FrrRipActiveIfStatusMap> frr_rip_interface_status_get(void);

/* (For unit-test purpose)
 * Parse the RIP interface information from VTY output string */
FrrRipActiveIfStatusMap frr_rip_interface_status_parse(const std::string &vty_output);

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
/* The data structure for the RIP peer entry data. */
struct FrrRipPeerData {
    uint32_t recv_bad_packets;
    uint32_t recv_bad_routes;
    seconds last_update_time;
    FrrRipVer rip_ver;
};

/* Get the RIP Peer information */
using FrrRipPeerMap = Map<mesa_ipv4_t, FrrRipPeerData>;
FrrRes<FrrRipPeerMap> frr_rip_peer_get(void);

/* (For unit-test purpose)
 * Parse the RIP peer information from VTY output string */
FrrRipPeerMap frr_rip_peer_parse(const std::string &vty_output);
// bool operator<(const FrrRipDbKey &a, const FrrRipDbKey &b);

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------

/* The enumeration for the RIP database protocol type. */
enum FrrRipDbProtoType {
    FrrRipDbProtoType_Rip,
    FrrRipDbProtoType_Connected,
    FrrRipDbProtoType_Static,
    FrrRipDbProtoType_Ospf
};

/* The enumeration for the RIP database protocol sub-type. */
enum FrrRIpDbProtoSubType {
    FrrRIpDbProtoSubType_Normal,
    FrrRIpDbProtoSubType_Static,
    FrrRIpDbProtoSubType_Default,
    FrrRIpDbProtoSubType_Redistribute,
    FrrRIpDbProtoSubType_Interface
};

/* The enumeration for the RIP database next hop. */
enum FrrRipDbNextHopType {
    FrrRipDbNextHopType_IPv4,
    FrrRipDbNextHopType_IPv6,
    FrrRipDbNextHopType_Ifindex,
    FrrRipDbNextHopType_Blackhole
};

/* The data structure for the RIP database entry key. */
struct FrrRipDbKey {
    mesa_ipv4_network_t network;
    mesa_ipv4_t nexthop;
};

/* The data structure for the RIP database entry data. */
struct FrrRipDbData {
    FrrRipDbProtoType type;
    FrrRIpDbProtoSubType subtype;
    bool self_intf;
    mesa_ipv4_t src_addr;
    uint8_t metric;
    uint32_t external_metric;
    uint32_t tag;
    seconds uptime;
    FrrRipDbNextHopType nexthop_type;
};

/* Get the RIP database information */
using FrrRipDbMap = Map<FrrRipDbKey, FrrRipDbData>;
FrrRes<FrrRipDbMap> frr_rip_db_get(void);

/* (For unit-test purpose)
 * Parse the RIP database information from VTY output string */
FrrRipDbMap frr_rip_db_parse(const std::string &vty_output);
bool operator<(const FrrRipDbKey &a, const FrrRipDbKey &b);

//----------------------------------------------------------------------------
//** RIP redistribution
//----------------------------------------------------------------------------
struct FrrRipRouterRedistribute {
    vtss_appl_ip_route_protocol_t protocol;
    vtss::Optional<uint8_t> metric;
    vtss::Optional<std::string> route_map;
};

Vector<FrrRipRouterRedistribute> frr_rip_router_redistribute_conf_get(std::string &running_conf);
mesa_rc frr_rip_router_redistribute_conf_set(const FrrRipRouterRedistribute &val);
mesa_rc frr_rip_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t val);

/** For unit-test purpose */
Vector<std::string> to_vty_rip_router_redistribute_conf_set(const FrrRipRouterRedistribute &val);
Vector<std::string> to_vty_rip_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t &val);

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
struct FrrRipRouterConf {
    bool router_mode;
    vtss::Optional<FrrRipVer> version;
    vtss::Optional<FrrRipRouterTimersConf> timers;
    vtss::Optional<uint8_t> redist_def_metric;
    vtss::Optional<bool> def_route_redist;
    vtss::Optional<bool> def_passive_intf;
    vtss::Optional<uint8_t> admin_distance;
};

/* Get RIP router configuration
 * The optional parameters are used for frr_rip_router_conf_set() only.
 * For the frr_rip_router_conf_get(), it always output the current
 * configuration.
 */
FrrRes<FrrRipRouterConf> frr_rip_router_conf_get(std::string &running_conf);
mesa_rc frr_rip_router_conf_set(const FrrRipRouterConf &conf);

/* Enable/Disable RIP router mode (For unit-test purpose) */
Vector<std::string> to_vty_rip_router_mode_set(bool is_enable);

/* Set RIP router global configuration (For unit-test purpose) */
Vector<std::string> to_vty_rip_router_conf_set(const FrrRipRouterConf &conf);

}  // namespace vtss

#endif  // _FRR_RIP_ACCESS_HXX_

