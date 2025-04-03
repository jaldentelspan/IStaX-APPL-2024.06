/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#if !defined(VTSS_SW_OPTION_FRR)
#error "This module only compiles with FRR"
#endif

#include <stdlib.h>
#include <net/if.h>
#include "ip_api.h"
#include "ip_acd4.hxx"
#include "ip_base.hxx"
#include "ip_dhcp6c.hxx"
#include "ip_lock.hxx"
#include "ip_utils.hxx"
#include "ip_os.hxx"
#include "ip_chip.hxx"
#include "ip_trace.h"
#include "ip_expose.hxx"
#include "ip_filter_api.hxx"
#include "vtss_trace_api.h"
#include "conf_api.h"
#include "critd_api.h"
#include "misc_api.h"           /* misc_ipv4_txt(), misc_ipv6_txt() */
#include "vtss_intrusive_list.h"
#include "dhcp_client_api.h"
#include "vlan_api.h"
#include "port_iter.hxx" /* For port_iter_t */
#include <vtss/basics/memory.hxx>
#include <vtss/basics/expose/table-status.hxx>
#include <vtss/basics/list.hxx>
#include <vtss/basics/trace.hxx>
#include "subject.hxx"
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>
#include <vtss/basics/types.hxx>
#include "frr_ip_route.hxx"

#ifdef VTSS_SW_OPTION_IP_MISC
#include "ip_misc_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICLI
#include "ip_icli_priv.h"
#endif

#ifdef VTSS_SW_OPTION_SNMP
#include "ip_snmp.h"
#endif

#ifdef VTSS_SW_OPTION_DNS
#include "ip_dns_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#ifdef VTSS_SW_OPTION_LLDP
#include "lldp_api.h"
#endif

#ifdef VTSS_SW_OPTION_CPUPORT
#include "cpuport_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_ICLI)
#include "icli_api.h"   // For icli_session_printf_to_all()
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP

// Our mutex
critd_t IP_crit;

static vtss_trace_reg_t trace_reg = {
    VTSS_MODULE_ID_IP, "ip", "IP"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_API] = {
        "chip",
        "Chip/MESA layer",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_OS] = {
        "os",
        "OS layer",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_NETLINK] = {
        "netlink",
        "Netlink",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_SERIALIZER] = {
        "serializer",
        "Serializer",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_FILTER] = {
        "filter",
        "Netlink filter",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_DYING_GASP] = {
        "dying_gasp",
        "Dying gasp netlink",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_ACD] = {
        "acd",
        "ACD",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_DHCP4C] = {
        "dhcp4c",
        "DHCP4 Client",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_DHCP6C] = {
        "dhcp6c",
        "DHCP6 Client",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_ICLI] = {
        "icli",
        "ICLI/ICFG",
        VTSS_TRACE_LVL_ERROR
    },
    [IP_TRACE_GRP_SNMP] = {
        "snmp",
        "SNMP",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define IP_CRIT_ASSERT_LOCKED() critd_assert_locked(&IP_crit, __FILE__, __LINE__)

// Generally DHCP is less trusty than routing protocols
#define IP_ROUTE_DHCP_DISTANCE 253

#define IP_IF_SUBSCRIBER_CNT_MAX 32
static vtss_ip_if_callback_t IP_subscribers[IP_IF_SUBSCRIBER_CNT_MAX];
static vtss::List<vtss_ifindex_t> IP_vlan_notify;

vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamVal<vtss_appl_ip_if_status_dhcp4c_t *>> status_if_dhcp4c("status_if_dhcp4c", VTSS_MODULE_ID_IP);

static vtss_appl_ip_global_conf_t IP_global_conf;
static vtss_appl_ip_if_conf_t     IP_if_default_conf;

// The IP_static_route_map *only* contains user-defined routes (both IPv4/IPv6).
// DHCP-defined routes are kept per I/F.
// If both DHCP and the user installes the same route, the appropriate code
// looks at the route's distance and installs the one with the smallest into
// FRR.
// When retrieving route status from FRR, vtss_appl_ip_route_status_get_all()
// possibly duplicates routes that are installed by both DHCP and the user.
typedef vtss::Map<vtss_appl_ip_route_key_t, vtss_appl_ip_route_conf_t> ip_static_route_map_t;
typedef ip_static_route_map_t::iterator                                ip_static_route_itr_t;
static ip_static_route_map_t                                           IP_static_route_map;

// Use memcmp() to judge whether a route's configuration has changed.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_route_conf_t);

// Our capabilities.
static vtss_appl_ip_capabilities_t IP_cap;
static vtss_appl_ip_route_conf_t   IP_route_conf_default;

// Cached route status
static vtss_appl_ip_route_status_map_t IP_route_status_map;

/******************************************************************************/
// IP_cap_init()
/******************************************************************************/
static void IP_cap_init(void)
{
    IP_cap.has_ipv4_host_capabilities = true;

#if defined(VTSS_SW_OPTION_IPV6)
    IP_cap.has_ipv6_host_capabilities = true;
#else
    IP_cap.has_ipv6_host_capabilities = false;
#endif

#if defined(VTSS_SW_OPTION_L3RT)
    IP_cap.has_ipv4_unicast_routing_capabilities = true;
#else
    IP_cap.has_ipv4_unicast_routing_capabilities = false;
#endif

    IP_cap.has_ipv6_unicast_routing_capabilities = IP_cap.has_ipv6_host_capabilities && IP_cap.has_ipv4_unicast_routing_capabilities;

    if (fast_cap(MESA_CAP_L3)) {
        IP_cap.has_ipv4_unicast_hw_routing_capabilities = IP_cap.has_ipv4_unicast_routing_capabilities;
        IP_cap.has_ipv6_unicast_hw_routing_capabilities = IP_cap.has_ipv6_unicast_routing_capabilities;
        IP_cap.lpm_hw_entry_cnt_max                     = fast_cap(MESA_CAP_L3_LPM_CNT);
    } else {
        IP_cap.has_ipv4_unicast_hw_routing_capabilities = false;
        IP_cap.has_ipv6_unicast_hw_routing_capabilities = false;
        IP_cap.lpm_hw_entry_cnt_max                     = 0;
    }

    IP_cap.interface_cnt_max    = fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT);
    IP_cap.static_route_cnt_max = fast_cap(VTSS_APPL_CAP_IP_ROUTE_CNT);
}

/******************************************************************************/
// IP_if_conf_default_init()
/******************************************************************************/
static void IP_if_conf_default_init(void)
{
#if defined(SW_OPTION_NPI)
    // Cater for worst-case (JR2) encapsulation overhead (16 bytes MAC + 32 bytes IFH)
    IP_if_default_conf.mtu = 1500 - 48;
#else
    IP_if_default_conf.mtu = 1500;
#endif
}

/******************************************************************************/
// IP_route_conf_default_init()
/******************************************************************************/
static void IP_route_conf_default_init(void)
{
    IP_route_conf_default.distance = 1;
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_ifindex_to_vlan()
/******************************************************************************/
static mesa_rc IP_ifindex_to_vlan(vtss_ifindex_t ifindex, mesa_vid_t *vlan)
{
    vtss_ifindex_elm_t e;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));

    if (e.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Interface %s is not a vlan interface", ifindex);
        *vlan = 0;
        return VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN;
    }

    *vlan = e.ordinal;

    return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_vlan_to_ifindex()
/******************************************************************************/
vtss_ifindex_t IP_vlan_to_ifindex(mesa_vid_t v)
{
    vtss_ifindex_t ifindex;
    (void)vtss_ifindex_from_vlan(v, &ifindex);
    return ifindex;
}

// Intrusive list
typedef VTSS_LINKED_LIST_TYPE(IP_if_dhcp4c_fallback_timer_t_) ip_if_dhcp4c_fallback_timer_list_t;
static ip_if_dhcp4c_fallback_timer_list_t IP_if_dhcp4c_fallback_timer_list;

static vtss_handle_t IP_thread_handle;
static vtss_thread_t IP_thread_block;

#define IP_CTLFLAG_IFCHANGE            VTSS_BIT(0)
#define IP_CTLFLAG_IFNOTIFY            VTSS_BIT(1)
#define IP_CTLFLAG_THREAD_WAKEUP       VTSS_BIT(2)
#define IP_CTLFLAG_VLAN_CHANGE         VTSS_BIT(3)

static vtss_flag_t IP_control_flags;

typedef vtss::Map<vtss_ifindex_t, uint32_t> ip_ifindex2ifno_map_t;
typedef ip_ifindex2ifno_map_t::iterator ip_ifindex2ifno_itr_t;

static mesa_mac_t IP_main_mac;

static ip_if_state_map_t IP_if_state_map;
static bool IP_vlan_up[MESA_VIDS];

/******************************************************************************/
// ip_if_exists()
// Shared, internal function.
/******************************************************************************/
mesa_rc ip_if_exists(vtss_ifindex_t ifindex, ip_if_itr_t *if_itr)
{
    ip_if_itr_t tmp_if_itr;

    IP_CRIT_ASSERT_LOCKED();

    if ((tmp_if_itr = IP_if_state_map.find(ifindex)) == IP_if_state_map.end()) {
        return VTSS_APPL_IP_RC_IF_NOT_FOUND;
    }

    if (if_itr) {
        *if_itr = tmp_if_itr;
    }

    return VTSS_RC_OK;
}

#if VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO
/******************************************************************************/
// IP_vif_fwd_to_str()
/******************************************************************************/
static const char *IP_vif_fwd_to_str(ip_vif_fwd_t fwd)
{
    switch (fwd) {
    case VIF_FWD_UNDEFINED:
        return "Undefined";

    case VIF_FWD_FORWARDING:
        return "Forwarding";

    case VIF_FWD_BLOCKING:
        return "Blocking";

    default:
        T_E("Invalid ip_vif_fwd_t (%d)", fwd);
        return "Unknown";
    }
}
#endif

/******************************************************************************/
// IP_poll_fwdstate()
// Figures out whether VLAN (and CPU) interfaces change state from up to down
// or vice versa, depending on port link states and VLAN memberships.
/******************************************************************************/
static void IP_poll_fwdstate(void)
{
    ip_if_itr_t             if_itr;
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
    port_iter_t             pit;
    mesa_packet_port_info_t info;
    uint32_t                change_cnt = 0;
    uint32_t                port_up_cnt;
    ip_vif_fwd_t            new_fwd;
    mesa_rc                 rc;

    IP_CRIT_ASSERT_LOCKED();

    T_N("Enter");

    (void)mesa_packet_port_info_init(&info);
    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
#ifdef VTSS_SW_OPTION_CPUPORT
        if (vtss_ifindex_is_cpu(if_itr->first)) {
            short flags = 0;

            if (vtss_cpuport_get_interface_flags(if_itr->first, &flags) != VTSS_RC_OK) {
                T_E("Interface %s not implemented.", if_itr->first);
            }

            new_fwd = (flags & IFF_UP) ? VIF_FWD_FORWARDING : VIF_FWD_BLOCKING;
            if (new_fwd != if_itr->second.unit_fwd) {
                T_I("%s: Fwd state changed from %s to %s", if_itr->first, IP_vif_fwd_to_str(if_itr->second.unit_fwd), IP_vif_fwd_to_str(new_fwd));
                if_itr->second.unit_fwd = new_fwd;
                change_cnt++;
            }

            continue;
        }
#endif // VTSS_SW_OPTION_CPUPORT

        if (!vtss_ifindex_is_vlan(if_itr->first) || if_itr->second.vlan == 0) {
            T_E("Only VLAN interfaces expected, but got ifindex = %s", if_itr->first);
            continue;
        }

        filter.clear();
        info.vid = if_itr->second.vlan;

        if ((rc = mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data())) != VTSS_RC_OK) {
            T_E("mesa_packet_port_filter_get(VLAN = %u) failed: %s", info.vid, error_txt(rc));
            continue;
        }

        (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_UP);
        port_up_cnt = 0;

        new_fwd = VIF_FWD_BLOCKING;
        while (port_iter_getnext(&pit)) {
            vtss_appl_ip_if_status_link_t if_status_link;

            port_up_cnt++;

            if (filter[pit.iport].filter                                        != MESA_PACKET_FILTER_DISCARD &&
                vtss_appl_ip_if_status_link_get(if_itr->first, &if_status_link) == VTSS_RC_OK                 &&
                (if_status_link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP)) {
                T_N("Port %u on VLAN %u is FWD => IP interface is UP", pit.uport, info.vid);
                new_fwd = VIF_FWD_FORWARDING;

                // If one port is up and os-layer ip interface is up, then the
                // interface is up.
                break;
            } else {
                T_N("Port %u on VLAN = %u is BLOCKING", pit.uport, info.vid);
            }
        }

        if (!port_up_cnt) {
            T_R("No ports UP for vid = %u", info.vid);
        }

        if (new_fwd != if_itr->second.unit_fwd) {
            T_I("%s: Fwd state changed from %s to %s", if_itr->first, IP_vif_fwd_to_str(if_itr->second.unit_fwd), IP_vif_fwd_to_str(new_fwd));
            if_itr->second.unit_fwd = new_fwd;
            change_cnt++;
        }
    }

    if (change_cnt) {
        // Have main thread update combined state
        T_I("Signalling I/F changes (change count = %u)", change_cnt);
        vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_IFCHANGE);
    }
}

/******************************************************************************/
// IP_if_cache_update()
/******************************************************************************/
static void IP_if_cache_update(void)
{
#ifdef VTSS_SW_OPTION_LLDP
    lldp_something_has_changed();
#endif /* VTSS_SW_OPTION_LLDP */

    IP_CRIT_ASSERT_LOCKED();

    // Something changed, all nodes must publish new fwd state
    IP_poll_fwdstate();
}

/******************************************************************************/
// ip_if_signal()
// Shared, internal function.
/******************************************************************************/
void ip_if_signal(vtss_ifindex_t ifindex)
{
    IP_CRIT_ASSERT_LOCKED();

    T_D("Signal interface %u", ifindex);

    // Update cached if status
    IP_if_cache_update();

    IP_vlan_notify.push_back(ifindex);

    // Sync routes when interfaces change
    vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_IFNOTIFY);
}

/******************************************************************************/
// IP_if_status_get()
/******************************************************************************/
static mesa_rc IP_if_status_get(ip_if_itr_t if_itr, vtss_appl_ip_if_status_type_t type, vtss::Vector<vtss_appl_ip_if_status_t> &status)
{
    vtss_appl_ip_if_status_t s       = {};
    vtss_ifindex_t           ifindex = if_itr->first;
    mesa_rc                  rc;

    T_R("Considering interface %s", ifindex);
    s.ifindex = ifindex;

    if (type == VTSS_APPL_IP_IF_STATUS_TYPE_ANY || type == VTSS_APPL_IP_IF_STATUS_TYPE_LINK) {
        s.type = VTSS_APPL_IP_IF_STATUS_TYPE_LINK;
        if (status_if_link.get(ifindex, &s.u.link) == VTSS_RC_OK) {
            status.push_back(s);
        }
    }

    if (type == VTSS_APPL_IP_IF_STATUS_TYPE_ANY || type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
        vtss_appl_ip_if_key_ipv4_t key = {};
        auto                       lock = status_if_ipv4.lock_get(__FILE__, __LINE__);
        auto                       ref  = status_if_ipv4.ref(lock);
        auto                       e    = ref.end();

        s.type = VTSS_APPL_IP_IF_STATUS_TYPE_IPV4;

        key.ifindex = ifindex;
        for (auto i = ref.greater_than(key); i != e && i->first.ifindex == ifindex; ++i) {
            s.u.ipv4.net  = i->first.addr;
            s.u.ipv4.info = i->second;
            status.push_back(s);
        }
    }

#if defined(VTSS_SW_OPTION_IPV6)
    if (type == VTSS_APPL_IP_IF_STATUS_TYPE_ANY || type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6) {
        vtss_appl_ip_if_key_ipv6_t key = {};
        auto                       lock = status_if_ipv6.lock_get(__FILE__, __LINE__);
        auto                       ref  = status_if_ipv6.ref(lock);
        auto                       e    = ref.end();

        s.type = VTSS_APPL_IP_IF_STATUS_TYPE_IPV6;

        key.ifindex = ifindex;
        for (auto i = ref.greater_than(key); i != e && i->first.ifindex == ifindex; ++i) {
            s.u.ipv6.net  = i->first.addr;
            s.u.ipv6.info = i->second;
            status.push_back(s);
        }
    }
#endif // VTSS_SW_OPTION_IPV6

    if (type == VTSS_APPL_IP_IF_STATUS_TYPE_ANY || type == VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C) {
        // Check if this interface uses a DHCPv4 client
        if (!if_itr->second.if_config.ipv4.enable || !if_itr->second.if_config.ipv4.dhcpc_enable) {
            // DHCPv4 not enabled.
            return VTSS_RC_OK;
        }

        s.type = VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C;

        // Get DHCPv4 client status
        if ((rc = vtss::dhcp::client_status(ifindex, &s.u.dhcp4c)) != VTSS_RC_OK) {
            T_EG(IP_TRACE_GRP_DHCP4C, "Failed to get DHCP status for interface %s: %s", ifindex, error_txt(rc));
            return rc;
        }

        status.push_back(s);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_if_status_get_all()
/******************************************************************************/
static mesa_rc IP_if_status_get_all(vtss_appl_ip_if_status_type_t type, vtss::Vector<vtss_appl_ip_if_status_t> &status)
{
    ip_if_itr_t if_itr;

    IP_CRIT_ASSERT_LOCKED();

    // Get status for all interfaces
    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
        if (IP_if_status_get(if_itr, type, status) != VTSS_RC_OK) {
            // Error message already printed
            continue;
        }
    }

    return VTSS_RC_OK;
}

static void conv_ipv4_to_mesa(const mesa_ipv4_t &a, mesa_ip_addr_t &b)
{
    b.type = MESA_IP_TYPE_IPV4;
    b.addr.ipv4 = a;
}

static void conv_ipv4nw_to_mesa(const mesa_ipv4_network_t &a, mesa_ip_network_t &b)
{
    conv_ipv4_to_mesa(a.address, b.address);
    b.prefix_size = a.prefix_size;
}

static void conv_ipv6_to_mesa(const mesa_ipv6_t &a, mesa_ip_addr_t &b)
{
    b.type = MESA_IP_TYPE_IPV6;
    b.addr.ipv6 = a;
}

static void conv_ipv6nw_to_mesa(const mesa_ipv6_network_t &a, mesa_ip_network_t &b)
{
    conv_ipv6_to_mesa(a.address, b.address);
    b.prefix_size = a.prefix_size;
}

/******************************************************************************/
// IP_ipv4_conf_static_address_active()
/******************************************************************************/
bool IP_ipv4_conf_static_address_active(const vtss_appl_ip_if_conf_ipv4_t &c)
{
    return c.enable && (!c.dhcpc_enable || (c.dhcpc_enable && c.fallback_enable));
}

/******************************************************************************/
// IP_ipv6_conf_static_address_active()
/******************************************************************************/
bool IP_ipv6_conf_static_address_active(const vtss_appl_ip_if_conf_ipv6_t &c)
{
    return c.enable;
}

/******************************************************************************/
// IP_route_conflict_subnet()
// A route must not be equal to an interface route (for systems without metric
// support)
/******************************************************************************/
static mesa_rc IP_route_conflict_subnet(const vtss_appl_ip_route_key_t &key)
{
    ip_if_itr_t       if_itr;
    bool              is_ipv4;
    mesa_ip_network_t net;
    vtss::IpNetwork   if_addr;
    bool active;

    IP_CRIT_ASSERT_LOCKED();

    if (key.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
        is_ipv4 = true;
        conv_ipv4nw_to_mesa(key.route.ipv4_uc.network, net);
    } else if (key.type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC) {
        is_ipv4 = false;
        conv_ipv6nw_to_mesa(key.route.ipv6_uc.network, net);
    } else {
        T_E("Unsupported address type (%d)", key.type);
        return VTSS_APPL_IP_RC_INTERNAL_ERROR;
    }

    vtss::Vector<vtss_appl_ip_if_status_t> st(IP_cap.interface_cnt_max);
    if (!st.capacity()) {
        T_E("Alloc error");
        return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
    }

    // Check against active IP address, regardless where they come from

    // Get status IPs
    if (IP_if_status_get_all(is_ipv4 ? VTSS_APPL_IP_IF_STATUS_TYPE_IPV4 : VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, st) != VTSS_RC_OK) {
        T_E("Failed to get if status, is_ipv4 = %d", is_ipv4);
        return VTSS_APPL_IP_RC_INTERNAL_ERROR;
    }

    for (const auto &e : st) {
        mesa_ip_network_t tmp;

        if (is_ipv4) {
            conv_ipv4nw_to_mesa(e.u.ipv4.net, tmp);
        } else {
            conv_ipv6nw_to_mesa(e.u.ipv6.net, tmp);
        }

        if (vtss_ip_net_equal(&net, &tmp)) {
            T_I("Route %s conflicts with network %s at interface %s", net, tmp, e.ifindex);
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_CONFLICTS_WITH_ACTIVE_IP;
        }
    }

    // Check against static configured addresses
    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
        if (is_ipv4) {
            active  = IP_ipv4_conf_static_address_active(if_itr->second.if_config.ipv4);
            if_addr = if_itr->second.if_config.ipv4.network;
        } else {
            active  = IP_ipv6_conf_static_address_active(if_itr->second.if_config.ipv6);
            if_addr = if_itr->second.if_config.ipv6.network;
        }

        // Not active -> no conflict
        if (!active) {
            continue;
        }

        // check static ip address
        if (vtss_ip_net_equal(&net, &if_addr.as_api_type())) {
            T_I("Route %s conflicts with network %s", net, if_addr.as_api_type());
            return VTSS_APPL_IP_RC_INTERNAL_ERROR;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_route_conflict_dest()
// Check if IP route destination address conflicts with existing interface IP
// address.
/******************************************************************************/
static mesa_rc IP_route_conflict_dest(const vtss_appl_ip_route_key_t &rt)
{
    // Allocate resource
    vtss::Vector<vtss_appl_ip_if_status_t> st(IP_cap.interface_cnt_max);
    vtss_appl_ip_if_status_type_t          status_type = rt.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC ? VTSS_APPL_IP_IF_STATUS_TYPE_IPV4 : VTSS_APPL_IP_IF_STATUS_TYPE_IPV6;
    const mesa_ipv4_t                      *ipv4_addr  = &rt.route.ipv4_uc.destination;
    const mesa_ipv6_t                      *ipv6_addr  = &rt.route.ipv6_uc.destination;

    if (!st.capacity()) {
        T_E("Alloc error");
        return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
    }

    // Walk through all existing interface IPv4/IPv6 addresses
    if (IP_if_status_get_all(status_type, st) != VTSS_RC_OK) {
        T_E("Failed to get if status");
        return VTSS_APPL_IP_RC_INTERNAL_ERROR;
    }

    for (const auto &e : st) {
        if (status_type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
            // IPv4
            if (e.u.ipv4.net.address == *ipv4_addr) {
                T_D("%s: Conflict with local I/F", ipv4_addr);
                return VTSS_APPL_IP_RC_ROUTE_DEST_CONFLICT_WITH_LOCAL_IF;
            }
        } else {
            // IPv6
            if (e.u.ipv6.net.address == *ipv6_addr) {
                T_D("%s: Conflict with local /IF", ipv6_addr);
                return VTSS_APPL_IP_RC_ROUTE_DEST_CONFLICT_WITH_LOCAL_IF;
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_route_type_validate()
/******************************************************************************/
static mesa_rc IP_route_type_validate(vtss_appl_ip_route_type_t type, bool allow_any)
{
    if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
        return VTSS_RC_OK;
    }

    if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC) {
#if defined(VTSS_SW_OPTION_IPV6)
        return VTSS_RC_OK;
#else
        return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif
    }

    if (allow_any && type == VTSS_APPL_IP_ROUTE_TYPE_ANY) {
        return VTSS_RC_OK;
    }

    return VTSS_APPL_IP_RC_ROUTE_TYPE_INVALID;
}

/******************************************************************************/
// IP_route_protocol_validate()
/******************************************************************************/
static mesa_rc IP_route_protocol_validate(vtss_appl_ip_route_protocol_t protocol, bool allow_any)
{
    if (allow_any) {
        if (protocol >= (vtss_appl_ip_route_protocol_t)0 && protocol <= VTSS_APPL_IP_ROUTE_PROTOCOL_ANY) {
            return VTSS_RC_OK;
        }
    } else {
        if (protocol >= (vtss_appl_ip_route_protocol_t)0 && protocol < VTSS_APPL_IP_ROUTE_PROTOCOL_ANY) {
            return VTSS_RC_OK;
        }
    }

    return VTSS_APPL_IP_RC_ROUTE_PROTOCOL_INVALID;
}

/******************************************************************************/
// IP_route_is_dhcp_installed()
/******************************************************************************/
static bool IP_route_is_dhcp_installed(const vtss_appl_ip_route_key_t &rt, ip_if_itr_t *result_if_itr)
{
    ip_if_itr_t if_itr;

    // RBNTBD:
    // Currently, we don't support lookup of DHCP6C-installed routes.
    if (rt.type != VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
        return false;
    }

    if (rt.route.ipv4_uc.network.address != 0 || rt.route.ipv4_uc.network.prefix_size != 0) {
        // This is not a default route, so it can't be DHCP-assigned.
        return false;
    }

    // Loop through all interfaces to see if this route is installed by DHCP
    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
        if (!if_itr->second.dhcp_v4_valid || if_itr->second.dhcp_v4_gw == 0) {
            // DHCP has not installed current IPv4 address or no default gateway
            // is assigned.
            continue;
        }

        if (rt.route.ipv4_uc.destination == if_itr->second.dhcp_v4_gw) {
            if (result_if_itr) {
                *result_if_itr = if_itr;
            }

            return true;
        }
    }

    return false;
}

/******************************************************************************/
// IP_route_valid()
/******************************************************************************/
static mesa_rc IP_route_valid(const vtss_appl_ip_route_key_t &key)
{
    mesa_ipv4_t ipv4_mask;
#if defined(VTSS_SW_OPTION_IPV6)
    mesa_ipv6_t ipv6_mask;
    mesa_vid_t  vlan;
    int         i;
#endif

    VTSS_RC(IP_route_type_validate(key.type, false));

    switch (key.type) {
    case VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC:
        if (key.route.ipv4_uc.network.prefix_size > 32) {
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_PREFIX_SIZE;
        }

        ipv4_mask = vtss_ipv4_prefix_to_mask(key.route.ipv4_uc.network.prefix_size);

        if (key.route.ipv4_uc.network.address & ~ipv4_mask) {
            // Network has bits set outside of prefix
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_BITS_SET_OUTSIDE_OF_PREFIX;
        }

        if (vtss_ipv4_addr_is_loopback( &key.route.ipv4_uc.network.address) ||
            vtss_ipv4_addr_is_multicast(&key.route.ipv4_uc.network.address)) {
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_MUST_BE_UNICAST;
        }

        if (!vtss_ipv4_addr_is_routable(&key.route.ipv4_uc.network.address)) {
            // This network is not routable (a route that catches such networks
            // is already installed by vtss_ip_chip_init()
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_NOT_ROUTABLE;
        }

        if (vtss_ipv4_addr_is_zero(&key.route.ipv4_uc.destination)) {
            return VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_ZERO;
        }

        if (vtss_ipv4_addr_is_loopback( &key.route.ipv4_uc.destination) ||
            vtss_ipv4_addr_is_multicast(&key.route.ipv4_uc.destination)) {
            // IPv4 unicast address only
            return VTSS_APPL_IP_RC_ROUTE_DEST_MUST_BE_UNICAST;
        }

        break;

    case VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC:
#if defined(VTSS_SW_OPTION_IPV6)
        if (vtss_conv_prefix_to_ipv6mask(key.route.ipv6_uc.network.prefix_size, &ipv6_mask) != VTSS_RC_OK) {
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_PREFIX_SIZE;
        }

        for (i = 0; i < sizeof(ipv6_mask.addr); i++) {
            if (key.route.ipv6_uc.network.address.addr[i] & ~ipv6_mask.addr[i]) {
                // Network has bits set outside of prefix
                return VTSS_APPL_IP_RC_ROUTE_SUBNET_BITS_SET_OUTSIDE_OF_PREFIX;
            }
        }

        if (!vtss_ipv6_addr_is_routable(&key.route.ipv6_uc.network.address)) {
            // This network is not routable (a route that catches such networks
            // is already installed by vtss_ip_chip_init()
            return VTSS_APPL_IP_RC_ROUTE_SUBNET_NOT_ROUTABLE;
        }

        if (vtss_ipv6_addr_is_link_local(&key.route.ipv6_uc.destination)) {
            // VLAN must be valid
            if (IP_ifindex_to_vlan(key.vlan_ifindex, &vlan) != VTSS_RC_OK) {
                return VTSS_APPL_IP_RC_ROUTE_DEST_VLAN_INVALID;
            }

            if (vlan < VTSS_APPL_VLAN_ID_MIN || vlan > VTSS_APPL_VLAN_ID_MAX) {
                return VTSS_APPL_IP_RC_ROUTE_DEST_VLAN_INVALID;
            }
        } else {
            // Not link-local address. Then it may not be a loopback address or
            // on IPv4 form (not sure what the latter actually means, but this
            // is how the former version of ipv6.icli did it).
            if (!vtss_ipv6_addr_is_mgmt_support(&key.route.ipv6_uc.destination)) {
                return VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_LOOPBACK_OR_IPV4_FORM;
            }
        }

        if (vtss_ipv6_addr_is_zero(&key.route.ipv6_uc.destination)) {
            return VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_ZERO;
        }

        if (key.route.ipv6_uc.destination != vtss_ipv6_blackhole_route) {
            // The blackhole route is a special route that we allow. Other
            // multicast routes are disallowed.
            if (vtss_ipv6_addr_is_multicast(&key.route.ipv6_uc.destination)) {
                return VTSS_APPL_IP_RC_ROUTE_DEST_MUST_BE_UNICAST;
            }
        }

        break;
#else
        return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // (!)VTSS_SW_OPTION_IPV6

    default:
        // Route type already validated at the top of this function
        break;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_route_add()
/******************************************************************************/
static mesa_rc IP_route_add(const vtss_appl_ip_route_key_t &rt, const vtss_appl_ip_route_conf_t &conf, bool is_dhcp)
{
    ip_if_itr_t           if_itr;
    ip_static_route_itr_t static_route_itr;
    uint32_t              old_user_dist, old_dhcp_dist, old_dist;
    uint32_t              new_user_dist, new_dhcp_dist, new_dist;
    mesa_rc               rc;

    IP_CRIT_ASSERT_LOCKED();

    old_user_dist = 0xFFFFFFFF;
    old_dhcp_dist = 0xFFFFFFFF;

    if (is_dhcp) {
        // Route is not validated yet when being added using DHCP.
        VTSS_RC(IP_route_valid(rt));
    }

    // Now it's time to check for conflicts.
    // A route must not be equal to an interface route (for systems without
    // metric support).
    VTSS_RC(IP_route_conflict_subnet(rt));

    // Check whether the next-hop is our own address.
    VTSS_RC(IP_route_conflict_dest(rt));

    // Check if we have this route in our static map already.
    if ((static_route_itr = IP_static_route_map.find(rt)) != IP_static_route_map.end()) {
        old_user_dist = static_route_itr->second.distance;
    } else {
        // User route doesn't exist.
        if (!is_dhcp) {
            // The user is installing route now, so make sure there's room.
            if (IP_static_route_map.size() >= IP_cap.static_route_cnt_max) {
                return VTSS_APPL_IP_RC_ROUTE_MAX_CNT_REACHED;
            }
        }
    }

    // Check if we have a DHCP-installed route already
    if (IP_route_is_dhcp_installed(rt, &if_itr)) {
        old_dhcp_dist = IP_ROUTE_DHCP_DISTANCE;
    }

    new_dhcp_dist = old_dhcp_dist;
    new_user_dist = old_user_dist;

    if (is_dhcp) {
        if (conf.distance != old_dhcp_dist) {
            new_dhcp_dist = conf.distance;
        }
    } else {
        if (conf.distance != old_user_dist) {
            new_user_dist = conf.distance;
        }
    }

    old_dist = MIN(old_user_dist, old_dhcp_dist);
    new_dist = MIN(new_user_dist, new_dhcp_dist);

    if (old_dist != new_dist) {
        // Install or update the route in Zebra
        T_I("Adding/updating %s route (%s) to Zebra", rt, is_dhcp ? "DHCP" : "Static");
        if ((rc = frr_ip_route_conf_set(rt, conf)) != VTSS_RC_OK) {
            T_E("frr_ip_route_conf_set(%s) failed: %s", rt, error_txt(rc));
            return rc;
        }
    }

    if (!is_dhcp) {
        // Add the route to our static route map, if it's a user-created route.
        T_I("Adding/updating route (%s, distance %u) in static route map", rt, conf.distance);
        if (!IP_static_route_map.set(rt, conf)) {
            T_E("Internal error: Unable to add/update static route (%s) map", rt);
            return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_route_del()
/******************************************************************************/
static mesa_rc IP_route_del(const vtss_appl_ip_route_key_t &rt, bool is_dhcp)
{
    ip_if_itr_t               if_itr;
    ip_static_route_itr_t     static_route_itr;
    vtss_appl_ip_route_conf_t route_conf;
    bool                      user_installed_route_exists;
    bool                      replace_route = false;
    mesa_rc                   rc;

    IP_CRIT_ASSERT_LOCKED();

    // Whether it's DHCP or user that uninstalls this route, figure out whether
    // a user-installed route exists.
    static_route_itr = IP_static_route_map.find(rt);
    user_installed_route_exists = static_route_itr != IP_static_route_map.end();

    if (is_dhcp) {
        if (user_installed_route_exists) {
            // We might have to change the route already installed, if the
            // DHCP-assigned route has smaller distance than the user-installed.
            if (static_route_itr->second.distance > IP_ROUTE_DHCP_DISTANCE) {
                route_conf    = static_route_itr->second;
                replace_route = true;
            } else {
                // Keep user-installed route. Nothing else to do.
                return VTSS_RC_OK;
            }
        } else {
            // Just get it out of FRR.
        }
    } else {
        if (!user_installed_route_exists) {
            // Cannot remove a user-installed route that doesn't exist
            return VTSS_APPL_IP_RC_ROUTE_DOESNT_EXIST;
        }

        // Figure out whether a DHCP-installed route exists.
        if (IP_route_is_dhcp_installed(rt, &if_itr)) {
            // A DCHP-installed route exists. For simplicity, just replace the
            // current route with the DHCP-installed whether or not the DHCP
            // distance is higher or lower than the currently installed.
            vtss_clear(route_conf);
            route_conf.distance = IP_ROUTE_DHCP_DISTANCE;
            replace_route = true;
        }

        // Get it out of our map.
        IP_static_route_map.erase(static_route_itr);
    }

    if (replace_route) {
        T_I("Replacing route %s", rt);
        if ((rc = frr_ip_route_conf_set(rt, route_conf)) != VTSS_RC_OK) {
            T_E("frr_ip_route_conf_set(%s) failed: %s", rt, error_txt(rc));
            return rc;
        }
    } else {
        // Get it out of FRR
        if ((rc = frr_ip_route_conf_del(rt)) != VTSS_RC_OK) {
            T_E("frr_ip_route_conf_del(%s) failed: %s", rt, error_txt(rc));
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_if_flags_force_sync()
/******************************************************************************/
static void IP_if_flags_force_sync(vtss_ifindex_t ifindex)
{
    ip_if_itr_t if_itr;

    IP_CRIT_ASSERT_LOCKED();

    // Force sync of iff-flags
    T_I("%s: Poke IP_CTLFLAG_IFCHANGE", ifindex);

    if (ip_if_exists(ifindex, &if_itr) == VTSS_RC_OK) {
        if_itr->second.combined_fwd = VIF_FWD_UNDEFINED;
    }

    vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_IFCHANGE);
}

/******************************************************************************/
// IP_if_ipv4_del()
/******************************************************************************/
static mesa_rc IP_if_ipv4_del(ip_if_itr_t if_itr)
{
    ip_if_state_t *if_state = &if_itr->second;
    mesa_rc       rc;

    ip_acd4_sm_stop(if_itr);

    if (!if_state->ipv4_active) {
        return VTSS_RC_OK;
    }

    T_I("Deleting IPv4 address %s on %s", if_state->cur_ipv4, if_itr->first);
    if ((rc = ip_os_ipv4_del(if_itr->first, &if_state->cur_ipv4)) == VTSS_RC_OK) {
        // Update cache on success
        if_state->ipv4_active = false;
        vtss_clear(if_state->cur_ipv4);
        IP_if_cache_update();
    } else {
        rc = VTSS_RC_OK;
        T_E("%s: Failed to delete IPv4 address %s: %s", if_itr->first, if_state->cur_ipv4, error_txt(rc));
    }

    return VTSS_RC_OK;
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_ipv6_dhcp_del()
/******************************************************************************/
static mesa_rc IP_if_ipv6_dhcp_del(ip_if_itr_t if_itr)
{
    mesa_rc rc;

    IP_CRIT_ASSERT_LOCKED();

    if (!if_itr->second.dhcp6c.active) {
        return VTSS_RC_OK;
    }

    T_IG(IP_TRACE_GRP_DHCP6C, "Deleting IPv6 DHCP address %s on %s. Interface is %s", if_itr->second.dhcp6c.network, if_itr->first, IP_vlan_up[if_itr->second.vlan] ? "Up" : "Down");

    // If the VLAN interface is down, the kernel has already auto-deleted all
    // the IPv6 addresses (but not IPv4!). Therefore, the following function
    // needs to know whether the interface is up or down. If up, it will also
    // attempt to remove the IPv6 address from the kernel. It will always poll
    // the addresses afterwards to get an updated status_if_ipv6.
    rc = ip_os_ipv6_del(if_itr->first, &if_itr->second.dhcp6c.network, IP_vlan_up[if_itr->second.vlan]);

    if_itr->second.dhcp6c.active = false;
    vtss_clear(if_itr->second.dhcp6c.network);

    return rc;
}
#endif // VTSS_SW_OPTION_IPV6

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_ipv6_static_del()
/******************************************************************************/
static mesa_rc IP_if_ipv6_static_del(ip_if_itr_t if_itr)
{
    mesa_rc rc;

    IP_CRIT_ASSERT_LOCKED();

    if (!if_itr->second.ipv6_active) {
        return VTSS_RC_OK;
    }

    T_I("Deleting IPv6 static address %s on %s. Interface is %s", if_itr->second.cur_ipv6, if_itr->first, IP_vlan_up[if_itr->second.vlan] ? "Up" : "Down");

    // If the VLAN interface is down, the kernel has already auto-deleted all
    // the IPv6 addresses (but not IPv4!). Therefore, the following function
    // needs to know whether the interface is up or down. If up, it will also
    // attempt to remove the IPv6 address from the kernel. It will always poll
    // the addresses afterwards to get an updated status_if_ipv6.
    rc = ip_os_ipv6_del(if_itr->first, &if_itr->second.cur_ipv6, IP_vlan_up[if_itr->second.vlan]);

    if_itr->second.ipv6_active = false;
    vtss_clear(if_itr->second.cur_ipv6);

    return rc;
}
#endif // VTSS_SW_OPTION_IPV6

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_ipv6_del()
/******************************************************************************/
static void IP_if_ipv6_del(ip_if_itr_t if_itr)
{
    (void)IP_if_ipv6_dhcp_del(if_itr);
    (void)IP_if_ipv6_static_del(if_itr);
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_if_dhcp4c_gw_clear()
/******************************************************************************/
static void IP_if_dhcp4c_gw_clear(ip_if_itr_t if_itr)
{
    vtss_appl_ip_route_key_t rt;

    IP_CRIT_ASSERT_LOCKED();

    if (!if_itr->second.dhcp_v4_gw) {
        return;
    }

    rt.type                              = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
    rt.route.ipv4_uc.network.address     = 0;
    rt.route.ipv4_uc.network.prefix_size = 0;
    rt.route.ipv4_uc.destination         = if_itr->second.dhcp_v4_gw;
    if_itr->second.dhcp_v4_gw            = 0;

    if (IP_route_del(rt, true /* DHCP is deleting */) != VTSS_RC_OK) {
        T_I("Failed to delete route %s", rt);
    } else {
        T_I("Deleted route %s", rt);
    }
}

/******************************************************************************/
// IP_if_dhcp4c_clear()
/******************************************************************************/
static mesa_rc IP_if_dhcp4c_clear(ip_if_itr_t if_itr)
{
    mesa_rc rc = VTSS_RC_OK;

    IP_CRIT_ASSERT_LOCKED();

    T_D("%s", if_itr->first);

    IP_if_dhcp4c_gw_clear(if_itr);

    if (if_itr->second.dhcp_v4_valid) {
        rc = IP_if_ipv4_del(if_itr);
        if_itr->second.dhcp_v4_valid = 0;
    }

    return rc;
}

/******************************************************************************/
// IP_network_to_route()
/******************************************************************************/
static void IP_network_to_route(const mesa_ip_network_t *addr, vtss_appl_ip_route_key_t *rt)
{
    mesa_ipv4_t ipv4_mask;
    mesa_ipv6_t ipv6_mask;

    vtss_clear(*rt);

    switch (addr->address.type) {
    case MESA_IP_TYPE_IPV4: {
        ipv4_mask = vtss_ipv4_prefix_to_mask(addr->prefix_size);
        rt->type = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
        rt->route.ipv4_uc.network.prefix_size = addr->prefix_size;
        rt->route.ipv4_uc.network.address = (addr->address.addr.ipv4 & ipv4_mask);
        break;
    }

    case MESA_IP_TYPE_IPV6: {
        (void)vtss_conv_prefix_to_ipv6mask(addr->prefix_size, &ipv6_mask);
        for (size_t i = 0; i < sizeof(ipv6_mask.addr); i++) {
            rt->route.ipv6_uc.network.address.addr[i] = ipv6_mask.addr[i] & addr->address.addr.ipv6.addr[i];
        }

        rt->type = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
        rt->route.ipv6_uc.network.prefix_size = addr->prefix_size;
        break;
    }

    default:
        return;
    }
}

/******************************************************************************/
// IP_if_ip_check()
/******************************************************************************/
static bool IP_if_ip_check(const mesa_ip_network_t *addr, vtss_ifindex_t ignore_this_ifindex)
{
    ip_if_itr_t     if_itr;
    vtss::IpNetwork if_addr;
    bool            is_ipv4, active, rc;

    IP_CRIT_ASSERT_LOCKED();
    T_D("Checking IP address: %s ignoring I/F index %s", *addr, ignore_this_ifindex);

    if (addr->address.type == MESA_IP_TYPE_IPV4) {
        is_ipv4 = true;
    } else if (addr->address.type == MESA_IP_TYPE_IPV6) {
        is_ipv4 = false;
    } else {
        T_E("Unsupported address type (%d)", addr->address.type);
        return false;
    }

    vtss::Vector<vtss_appl_ip_if_status_t> st(IP_cap.interface_cnt_max);
    if (!st.capacity()) {
        T_E("Alloc error");
        return FALSE;
    }

    // Check against active IP address, regardless where they come from
    if (IP_if_status_get_all(is_ipv4 ? VTSS_APPL_IP_IF_STATUS_TYPE_IPV4 : VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, st) != VTSS_RC_OK) {
        T_E("Failed to get if status, is_ipv4 = %d", is_ipv4);
        rc = false;
        goto DONE;
    }

    rc = true;
    for (const auto &e : st) {
        mesa_ip_network_t tmp;

        if (e.ifindex == ignore_this_ifindex) {
            continue;
        }

        if (is_ipv4) {
            tmp.address.type = MESA_IP_TYPE_IPV4;
            tmp.address.addr.ipv4 = e.u.ipv4.net.address;
            tmp.prefix_size = e.u.ipv4.net.prefix_size;
        } else {
            tmp.address.type = MESA_IP_TYPE_IPV6;
            tmp.address.addr.ipv6 = e.u.ipv6.net.address;
            tmp.prefix_size = e.u.ipv6.net.prefix_size;
        }

        if (vtss_ip_net_overlap(addr, &tmp)) {
            T_I("Address %s conflicts with %s", addr, tmp);
            rc = false;
            goto DONE;
        }
    }

    // Check against static configured addresses
    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
        active = false;

        if (if_itr->first == ignore_this_ifindex) {
            continue;
        }

        if (is_ipv4) {
            active  = IP_ipv4_conf_static_address_active(if_itr->second.if_config.ipv4);
            if_addr = if_itr->second.if_config.ipv4.network;
        } else {
            active  = IP_ipv6_conf_static_address_active(if_itr->second.if_config.ipv6);
            if_addr = if_itr->second.if_config.ipv6.network;
        }

        // Not active -> no conflict
        if (!active) {
            continue;
        }

        // Check static ip address
        if (vtss_ip_net_overlap(addr, &if_addr.as_api_type())) {
            T_I("Address %s conflicts with %s on interface %s", *addr, if_addr.as_api_type(), if_itr->first);
            rc = false;
            goto DONE;
        }
    }

    // Check against static routes.
    // The network of an interface must not be equal to the network of a route
    // (for systems with no metrics support).
    do {
        vtss_appl_ip_route_key_t r;
        ip_static_route_itr_t    static_route_itr;
        bool                     conflict;

        IP_network_to_route(addr, &r);

        // We can use .greater_than() because destination is all-zeros in r.
        if ((static_route_itr = IP_static_route_map.greater_than(r)) == IP_static_route_map.end()) {
            break;
        }

        if (static_route_itr->first.type != r.type) {
            break;
        }

        T_I("Possible match: %s", static_route_itr->first);
        conflict = false;

        switch (r.type) {
        case VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC:
            if (vtss_ipv4_net_equal(&static_route_itr->first.route.ipv4_uc.network, &r.route.ipv4_uc.network)) {
                conflict = true;
            }

            break;

        case VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC:
            if (vtss_ipv6_net_equal(&static_route_itr->first.route.ipv6_uc.network, &r.route.ipv6_uc.network)) {
                conflict = true;
            }

            break;

        default:
            T_E("Unknown type (%d)", r.type);
            rc = false;
            goto DONE;
        }

        if (conflict) {
            T_I("Address %s conflicts with route %s", *addr, static_route_itr->first);
            rc = false;
            goto DONE;
        }
    } while (0);

DONE:
    T_I("Returning %s", rc ? "OK" : "Failure");
    return rc;
}

/******************************************************************************/
// IP_if_ipv4_check()
/******************************************************************************/
static bool IP_if_ipv4_check(const mesa_ipv4_network_t *v4, vtss_ifindex_t ignore_this_ifindex)
{
    mesa_ip_network_t n;

    n.address.type      = MESA_IP_TYPE_IPV4;
    n.prefix_size       = v4->prefix_size;
    n.address.addr.ipv4 = v4->address;

    return (vtss_ip_ipv4_ifaddr_valid(v4) && IP_if_ip_check(&n, ignore_this_ifindex));
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_ipv6_check()
/******************************************************************************/
static BOOL IP_if_ipv6_check(const mesa_ipv6_network_t *v6, vtss_ifindex_t ignore_this_ifindex)
{
    mesa_ip_network_t n;

    n.address.type      = MESA_IP_TYPE_IPV6;
    n.prefix_size       = v6->prefix_size;
    n.address.addr.ipv6 = v6->address;

    return IP_if_ip_check(&n, ignore_this_ifindex);
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_if_ipv4_addr_conflict_static_route_check()
// Check if the interface IPv4 address conflicts with existing static routes.
// Return true if conflict detected, false otherwise.
/******************************************************************************/
static mesa_rc IP_if_ipv4_addr_conflict_static_route_check(mesa_ipv4_t intf_ipv4_addr)
{
    ip_static_route_itr_t static_route_itr;

    IP_CRIT_ASSERT_LOCKED();

    for (static_route_itr = IP_static_route_map.begin(); static_route_itr != IP_static_route_map.end(); ++static_route_itr) {
        if (static_route_itr->first.type != VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
            // Not an IPv4 route
            continue;
        }

        if (static_route_itr->first.route.ipv4_uc.destination == intf_ipv4_addr) {
            T_D("%s: Conflict with static route %s", intf_ipv4_addr, static_route_itr->first);
            return VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_STATIC_ROUTE;
        }
    }

    return VTSS_RC_OK;
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_ipv6_addr_conflict_static_route_check()
// Check if the interface IPv6 address conflicts with existing static routes.
// Return true if conflict detected, false otherwise.
/******************************************************************************/
static mesa_rc IP_if_ipv6_addr_conflict_static_route_check(const mesa_ipv6_t &if_ipv6_addr)
{
    ip_static_route_itr_t static_route_itr;

    IP_CRIT_ASSERT_LOCKED();

    for (static_route_itr = IP_static_route_map.begin(); static_route_itr != IP_static_route_map.end(); ++static_route_itr) {
        if (static_route_itr->first.type != VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC) {
            // Not an IPv6 route
            continue;
        }

        if (static_route_itr->first.route.ipv6_uc.destination == if_ipv6_addr) {
            T_D("%s: Conflict with static route %s", if_ipv6_addr, static_route_itr->first);
            return VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_STATIC_ROUTE;
        }
    }

    return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_if_dhcp4c_signal()
/******************************************************************************/
static void IP_if_dhcp4c_signal(ip_if_itr_t if_itr)
{
    vtss_appl_ip_if_status_dhcp4c_t dhcp4c_status;
    mesa_rc                         rc;

    IP_CRIT_ASSERT_LOCKED();

    if ((rc = vtss::dhcp::client_status(if_itr->first, &dhcp4c_status)) != VTSS_RC_OK) {
        status_if_dhcp4c.del(if_itr->first);
    } else {
        status_if_dhcp4c.set(if_itr->first, &dhcp4c_status);
    }
}

/******************************************************************************/
// IP_if_ipv4_set()
/******************************************************************************/
static mesa_rc IP_if_ipv4_set(ip_if_itr_t if_itr, const mesa_ipv4_network_t &net)
{
    ip_if_state_t       *if_state = &if_itr->second;
    mesa_ipv4_network_t cur = if_state->cur_ipv4;
    mesa_rc             rc = VTSS_RC_OK;

    if (net == cur && if_state->ipv4_active) {
        T_D("Ignore interface update due to cache on I/F %s", if_itr->first);
        return VTSS_RC_OK;
    }

    ip_acd4_sm_stop(if_itr);

    if (if_state->ipv4_active) {
        T_I("Deleting IPv4 address %s on I/F %s", cur, if_itr->first);

        if ((rc = ip_os_ipv4_del(if_itr->first, &if_state->cur_ipv4)) == VTSS_RC_OK) {
            if_state->ipv4_active = false;
        } else {
            T_E("Failed to delete IPv4 address %s on I/F %s", cur, if_itr->first);
        }
    }

    if (vtss_ip_ipv4_ifaddr_valid(&net)) {
        T_I("Set new IPv4 address %s on I/F %s", net, if_itr->first);
        ip_acd4_sm_start(if_itr, &net);
    }

    // Update cache on success
    if (rc == VTSS_RC_OK) {
        if_state->cur_ipv4 = net;
        IP_if_cache_update();
    }

    return rc;
}

/******************************************************************************/
// IP_if_dhcp4c_cb_apply()
/******************************************************************************/
static void IP_if_dhcp4c_cb_apply(ip_if_itr_t if_itr)
{
    ip_if_state_t             *if_state = &if_itr->second;
    vtss_appl_ip_route_key_t  rt;
    vtss_appl_ip_route_conf_t route_conf;
    vtss::dhcp::ConfPacket    fields;
    mesa_ipv4_network_t       ip;
    mesa_rc                   rc;

    IP_CRIT_ASSERT_LOCKED();

    // Get the new IP address (if we got one)
    if (vtss::dhcp::client_fields_get(if_itr->first, &fields) != VTSS_RC_OK) {
        return;
    }

    if (!IP_if_ipv4_check(&fields.ip.as_api_type(), if_itr->first)) {
        T_WG(IP_TRACE_GRP_DHCP4C, "%s: The ack'ed DHCP IP address (%s) is not valid on this system", if_itr->first, &fields.ip);
        (void)vtss::dhcp::client_release(if_itr->first);
        return;
    }

    // Apply IP settings
    ip = fields.ip.as_api_type();
    if (IP_if_ipv4_set(if_itr, ip) != VTSS_RC_OK) {
        T_E("Failed to set IPv4 interface (%s) on I/F %s", &ip, if_itr->first);
        return;
    } else {
        if_state->dhcp_v4_valid = true;
    }

    // Apply default GW
    if (fields.default_gateway.valid() && if_state->dhcp_v4_gw != fields.default_gateway.get()) {
        // Current GW setting is valid. Delete the existing GW
        IP_if_dhcp4c_gw_clear(if_itr);

        vtss_clear(route_conf);
        route_conf.distance = IP_ROUTE_DHCP_DISTANCE;

        vtss_clear(rt);
        rt.type                              = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
        rt.route.ipv4_uc.network.address     = 0;
        rt.route.ipv4_uc.network.prefix_size = 0;
        rt.route.ipv4_uc.destination = fields.default_gateway.get();

        T_IG(IP_TRACE_GRP_DHCP4C, "Using default route from DHCP: %s", rt);

        if ((rc = IP_route_add(rt, route_conf, true /* DHCP-assigned */)) == VTSS_RC_OK) {
            if_state->dhcp_v4_gw = rt.route.ipv4_uc.destination;
        } else {
            T_I("Failed to add default route. I/F: %s, route: %s: %s", if_itr->first, rt, error_txt(rc));
        }
    } else if (!fields.default_gateway.valid()) {
        T_IG(IP_TRACE_GRP_DHCP4C, "Delete GW settings on I/F %s", if_itr->first);
        IP_if_dhcp4c_gw_clear(if_itr);
    } else {
        T_NG(IP_TRACE_GRP_DHCP4C, "DHCP GW settings on I/F %s needs no update", if_itr->first);
    }
}

/******************************************************************************/
// IP_if_dhcp4c_cb_ack()
/******************************************************************************/
static void IP_if_dhcp4c_cb_ack(ip_if_itr_t if_itr)
{
    ip_if_state_t          *if_state = &if_itr->second;
    vtss::dhcp::ConfPacket list[VTSS_DHCP_MAX_OFFERS];
    size_t                 valid_offers;
    int                    i;
    mesa_rc                rc;

    IP_CRIT_ASSERT_LOCKED();

    if ((rc = vtss::dhcp::client_offers_get(if_itr->first, VTSS_DHCP_MAX_OFFERS, &valid_offers, list)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_DHCP4C, "Failed to get list of DHCP offers on I/F %s", if_itr->first);
        return;
    }

    T_IG(IP_TRACE_GRP_DHCP4C, "Got %zu offers from DHCP client on I/F %s", valid_offers, if_itr->first);

    for (i = 0; i < valid_offers; i++) {
        T_DG(IP_TRACE_GRP_DHCP4C, "Flags[%d]: %s. vendor-specific-information-size %zu boot-file-name size: %zu",
             i,
             if_state->if_config.ipv4.dhcpc_params.dhcpc_flags,
             list[i].vendor_specific_information.size(),
             list[i].boot_file_name.size());

        // Filter out offers that don't have the required options
        if ((if_state->if_config.ipv4.dhcpc_params.dhcpc_flags & VTSS_APPL_IP_DHCP4C_FLAG_OPTION_43) && !list[i].vendor_specific_information.size()) {
            T_DG(IP_TRACE_GRP_DHCP4C, "No VTSS_APPL_IP_DHCP4C_FLAG_OPTION_43 found");
            continue;
        }

        if ((if_state->if_config.ipv4.dhcpc_params.dhcpc_flags & VTSS_APPL_IP_DHCP4C_FLAG_OPTION_67) && !list[i].boot_file_name.size()) {
            T_DG(IP_TRACE_GRP_DHCP4C, "No VTSS_APPL_IP_DHCP4C_FLAG_OPTION_67 found");
            continue;
        }

        if (IP_if_ipv4_check(&list[i].ip.as_api_type(), if_itr->first)) {
            T_IG(IP_TRACE_GRP_DHCP4C, "Accepting offer #%d, which is %s", i, &list[i].ip);
            (void)vtss::dhcp::client_offer_accept(if_itr->first, i);
            return;
        } else {
            T_DG(IP_TRACE_GRP_DHCP4C, "IP_if_ipv4_check failed!");
        }
    }

    T_IG(IP_TRACE_GRP_DHCP4C, "No offer was accepted");
}

/******************************************************************************/
// IP_if_dhcp4c_state_change()
/******************************************************************************/
static void IP_if_dhcp4c_state_change(ip_if_itr_t if_itr)
{
    ip_if_state_t                   *if_state = &if_itr->second;
    vtss_appl_ip_if_status_dhcp4c_t status;
    mesa_rc                         rc;

    // Derive the interface address based on configuration and DHCP client state

    T_DG(IP_TRACE_GRP_DHCP4C, "%s: enable: %d, DHCP: %s", if_itr->first, if_state->if_config.ipv4.enable, if_state->if_config.ipv4.dhcpc_enable ? "Yes" : "No");

    // Delete the DHCP client if the interface is not active
    if (!if_state->if_config.ipv4.enable) {
        T_IG(IP_TRACE_GRP_DHCP4C, "Interface is not active");
        (void)IP_if_dhcp4c_clear(if_itr);
        (void)IP_if_ipv4_del(if_itr);
        return;
    }

    // No DHCP means static configuration
    if (!if_state->if_config.ipv4.dhcpc_enable) {
        T_IG(IP_TRACE_GRP_DHCP4C, "Static config");
        (void)IP_if_ipv4_set(if_itr, if_state->if_config.ipv4.network);
        return;
    }

    // DHCP is active
    if ((rc = vtss::dhcp::client_status(if_itr->first, &status)) != VTSS_RC_OK) {
        T_IG(IP_TRACE_GRP_DHCP4C, "vtss::dhcp::client_status(%s) failed: %s", if_itr->first, error_txt(rc));
        (void)IP_if_dhcp4c_clear(if_itr);
        return;
    }

    T_IG(IP_TRACE_GRP_DHCP4C, "%s: DHCP state = %s", if_itr->first, dhcp4c_state_to_txt(status.state));

    switch (status.state) {
    case VTSS_APPL_IP_DHCP4C_STATE_SELECTING:
        T_IG(IP_TRACE_GRP_DHCP4C, "Selecting");
        (void)IP_if_dhcp4c_clear(if_itr);
        IP_if_dhcp4c_cb_ack(if_itr);
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK:
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        T_IG(IP_TRACE_GRP_DHCP4C, "Apply DHCP client addresses");
        IP_if_dhcp4c_cb_apply(if_itr);
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_FALLBACK:
        T_IG(IP_TRACE_GRP_DHCP4C, "Set fallback address");
        if (if_state->if_config.ipv4.fallback_enable) {
            (void)IP_if_ipv4_set(if_itr, if_state->if_config.ipv4.network);
        } else {
            // Should not be in fallback state when there is no fallback timer
            if (vtss::dhcp::client_start(if_itr->first, &if_state->if_config.ipv4.dhcpc_params) != VTSS_RC_OK) {
                T_EG(IP_TRACE_GRP_DHCP4C, "%s: Failed to start DHCP server", if_itr->first);
            }

            // Signal the interface
            IP_if_dhcp4c_state_change(if_itr);
        }

        break;

    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:
        T_IG(IP_TRACE_GRP_DHCP4C, "Ignoring BOUND");
        break;

    default:
        T_IG(IP_TRACE_GRP_DHCP4C, "DHCP client has no IP address. Clearing all addresses");
        (void)IP_if_dhcp4c_clear(if_itr);
        break;
    }

#ifdef VTSS_SW_OPTION_DNS
    // DNS settings must be applied after routes has been configured.
    T_IG(IP_TRACE_GRP_DHCP4C, "%s: Signaling DNS", if_itr->first);
    vtss_dns_signal();
#endif /* VTSS_SW_OPTION_DNS */

    T_IG(IP_TRACE_GRP_DHCP4C, "Done");
}

/******************************************************************************/
// IP_dhcp4c_fallback_timer_start()
/******************************************************************************/
static void IP_dhcp4c_fallback_timer_start(vtss_tick_count_t timeout, ip_if_dhcp4c_fallback_timer_t *timer)
{
    ip_if_dhcp4c_fallback_timer_t *i;
    vtss_tick_count_t             cur = vtss_current_time();

    VTSS_LINKED_LIST_UNLINK(*timer);

    T_IG(IP_TRACE_GRP_DHCP4C, "%s: DHCPC: Start fallback timer %s", timer->ifidx, timeout);
    timer->timeout = cur + timeout;

    VTSS_LINKED_LIST_FOREACH(IP_if_dhcp4c_fallback_timer_list, i) {
        if (i->timeout > timer->timeout) {
            break;
        }
    }

    VTSS_LINKED_LIST_INSERT_BEFORE(IP_if_dhcp4c_fallback_timer_list, i, *timer);
    vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_THREAD_WAKEUP);
}

/******************************************************************************/
// IP_if_dhcp4c_fallback_timer_consider()
/******************************************************************************/
static void IP_if_dhcp4c_fallback_timer_consider(ip_if_itr_t if_itr, bool reinit)
{
    ip_if_state_t                   *if_state = &if_itr->second;
    vtss_appl_ip_if_status_dhcp4c_t dhcp4c_status;

    if (!if_state->if_config.ipv4.enable) {
        goto UNLINK;
    }

    if (!if_state->if_config.ipv4.fallback_enable) {
        goto UNLINK;
    }

    if (vtss::dhcp::client_status(if_itr->first, &dhcp4c_status) != VTSS_RC_OK) {
        goto UNLINK;
    }

    if (dhcp4c_status.state != VTSS_APPL_IP_DHCP4C_STATE_SELECTING) {
        goto UNLINK;
    }

    if (!if_state->if_config.ipv4.dhcpc_enable) {
        goto UNLINK;
    }

    if (!VTSS_LINKED_LIST_IS_LINKED(if_state->dhcp4c_start_timer) || reinit) {
        vtss_tick_count_t timeout;

        timeout = VTSS_OS_MSEC2TICK(if_state->if_config.ipv4.fallback_timeout_secs * 1000);
        IP_dhcp4c_fallback_timer_start(timeout, &if_state->dhcp4c_start_timer);
    }

    return;

UNLINK:
    if (VTSS_LINKED_LIST_IS_LINKED(if_state->dhcp4c_start_timer)) {
        T_IG(IP_TRACE_GRP_DHCP4C, "Unlink timer on I/F %s", if_itr->first);
        VTSS_LINKED_LIST_UNLINK(if_state->dhcp4c_start_timer);
    }
}

/******************************************************************************/
// IP_if_dhcp4c_fallback_timer_update()
/******************************************************************************/
static void IP_if_dhcp4c_fallback_timer_update(ip_if_itr_t if_itr)
{
    // Delete existing timer
    if (!if_itr->second.if_config.ipv4.fallback_enable) {
        VTSS_LINKED_LIST_UNLINK(if_itr->second.dhcp4c_start_timer);
        return;
    }

    // Set new timeout
    IP_if_dhcp4c_fallback_timer_consider(if_itr, true);
}

/******************************************************************************/
// IP_if_dhcp4c_fallback_timer_evaluate()
/******************************************************************************/
static vtss_tick_count_t IP_if_dhcp4c_fallback_timer_evaluate(vtss_tick_count_t t)
{
    ip_if_itr_t                   if_itr;
    ip_if_dhcp4c_fallback_timer_t *i;
    vtss_tick_count_t             cur = vtss_current_time();

    while (!VTSS_LINKED_LIST_EMPTY(IP_if_dhcp4c_fallback_timer_list)) {
        i = VTSS_LINKED_LIST_FRONT(IP_if_dhcp4c_fallback_timer_list);

        if (i->timeout <= cur) {
            // Time is up
            T_DG(IP_TRACE_GRP_DHCP4C, "%s: Time is up (%s, %s)", i->ifidx, i->timeout, cur);

            VTSS_LINKED_LIST_POP_FRONT(IP_if_dhcp4c_fallback_timer_list);
            if (ip_if_exists(i->ifidx, &if_itr) != VTSS_RC_OK) {
                T_EG(IP_TRACE_GRP_DHCP4C, "Unable to find I/F state for %s", i->ifidx);
                return t;
            }

            // Put the DHCP client in fallback mode
            if (vtss::dhcp::client_fallback(i->ifidx) != VTSS_RC_OK) {
                T_EG(IP_TRACE_GRP_DHCP4C, "Failed to stop DHCP server on %s", i->ifidx);
            }

            // Signal the interface
            IP_if_dhcp4c_state_change(if_itr);
        } else {
            // We got a new timeout
            return (t < i->timeout) ? t : i->timeout;
        }
    }

    // No timers. Return input as minimum
    return t;
}

/******************************************************************************/
// IP_if_dhcp4c_callback()
// Invoked by DHCP client.
/******************************************************************************/
static void IP_if_dhcp4c_callback(vtss_ifindex_t ifindex)
{
    ip_if_itr_t if_itr;

    T_DG(IP_TRACE_GRP_DHCP4C, "Got DHCP callback on %s", ifindex);

    IP_LOCK_SCOPE();

    if (ip_if_exists(ifindex, &if_itr) != VTSS_RC_OK) {
        T_DG(IP_TRACE_GRP_DHCP4C, "Unable to find I/F state for %s", ifindex);
        return;
    }

    IP_if_dhcp4c_signal(if_itr);

    // The fallback timer must react on current state. It must be activated when
    // we go into SELECTING, and otherwise disabled
    IP_if_dhcp4c_fallback_timer_consider(if_itr, false);

    // Update interface settings
    IP_if_dhcp4c_state_change(if_itr);
}

/******************************************************************************/
// IP_if_dhcp4c_start()
/******************************************************************************/
static mesa_rc IP_if_dhcp4c_start(ip_if_itr_t if_itr, const vtss_appl_ip_if_conf_ipv4_t *conf)
{
    mesa_rc rc;

    IP_CRIT_ASSERT_LOCKED();

    T_IG(IP_TRACE_GRP_DHCP4C, "%s: Starting DHCP client", if_itr->first);

    (void)IP_if_ipv4_del(if_itr);

    if ((rc = vtss::dhcp::client_start(if_itr->first, &conf->dhcpc_params)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_DHCP4C, "vtss::dhcp::client_start(%s) failed: %s", if_itr->first, error_txt(rc));
        return rc;
    }

    T_I("Adding callback");
    if ((rc = vtss::dhcp::client_callback_add(if_itr->first, IP_if_dhcp4c_callback)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_DHCP4C, "vtss::dhcp::client_callback_add(%s) failed: %s", if_itr->first, error_txt(rc));
        return rc;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_if_dhcp4c_kill()
/******************************************************************************/
static mesa_rc IP_if_dhcp4c_kill(ip_if_itr_t if_itr)
{
    mesa_rc rc;

    T_DG(IP_TRACE_GRP_DHCP4C, "Enter (%s)", if_itr->first);

    IP_CRIT_ASSERT_LOCKED();

    if ((rc = IP_if_dhcp4c_clear(if_itr)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_DHCP4C, "%s: Failed to clear DHCP settings", if_itr->first);
    }

    rc = vtss::dhcp::client_kill(if_itr->first);
    IP_if_dhcp4c_signal(if_itr);

#ifdef VTSS_SW_OPTION_DNS
    T_IG(IP_TRACE_GRP_DHCP4C, "%s: Signaling DNS", if_itr->first);
    vtss_dns_signal();
#endif /* VTSS_SW_OPTION_DNS */

    return rc;
}

/******************************************************************************/
// IP_if_teardown()
// Stop interface - OS & chip
// Caller is responsible for freeing if_state.
/******************************************************************************/
static mesa_rc IP_if_teardown(ip_if_itr_t if_itr)
{
    mesa_rc rc;

    T_I("%s: Delete IP interface", if_itr->first);

    IP_CRIT_ASSERT_LOCKED();

    VTSS_LINKED_LIST_UNLINK(if_itr->second.dhcp4c_start_timer);

    if (if_itr->second.if_config.ipv4.dhcpc_enable) {
        // Kill DHCP client
        (void)IP_if_dhcp4c_kill(if_itr);
    }

    (void)IP_if_ipv4_del(if_itr);

#if defined(VTSS_SW_OPTION_IPV6)
    IP_if_ipv6_del(if_itr);
#endif

    rc = ip_os_if_del(if_itr->first);

    IP_if_flags_force_sync(if_itr->first);

    return rc;
}

/******************************************************************************/
// IP_if_teardown_all()
/******************************************************************************/
static void IP_if_teardown_all(void)
{
    ip_if_itr_t if_itr, next_if_itr;
    mesa_rc     rc;

    IP_CRIT_ASSERT_LOCKED();

    if_itr = IP_if_state_map.begin();
    while (if_itr != IP_if_state_map.end()) {
        next_if_itr = if_itr;
        ++next_if_itr;

        if ((rc = IP_if_teardown(if_itr)) != VTSS_RC_OK) {
            T_E("IP_if_teardown(%s) failed: %s", if_itr->first, error_txt(rc));
        }

        IP_if_state_map.erase(if_itr);
        if_itr = next_if_itr;
    }
}

/******************************************************************************/
// IP_static_routes_reset()
/******************************************************************************/
static void IP_static_routes_reset(void)
{
    ip_static_route_itr_t itr, next_itr;

    IP_CRIT_ASSERT_LOCKED();

    T_I("Emptying static routes");

    itr = IP_static_route_map.begin();
    while (itr != IP_static_route_map.end()) {
        next_itr = itr;
        ++next_itr;

        (void)IP_route_del(itr->first, false /* User-installed route */);
        itr = next_itr;
    }
}

/******************************************************************************/
// IP_global_conf_set()
/******************************************************************************/
static mesa_rc IP_global_conf_set(const vtss_appl_ip_global_conf_t *conf)
{
    mesa_rc rc;

    if (memcmp(conf, &IP_global_conf, sizeof(*conf)) == 0) {
        return VTSS_RC_OK;
    }

    IP_global_conf = *conf;
    if ((rc = ip_os_global_conf_set(conf)) != VTSS_RC_OK) {
        T_E("ip_os_global_conf_set() failed: %s", error_txt(rc));
    }

#ifdef VTSS_SW_OPTION_SNMP
    // we need to update a timer when forwarding is enabled/disabled
    vtss_ip_snmp_signal_global_changes();
#endif

    return rc;
}

/******************************************************************************/
// IP_conf_default()
/******************************************************************************/
static void IP_conf_default(void)
{
    vtss_appl_ip_global_conf_t global_conf;

    IP_CRIT_ASSERT_LOCKED();

    T_I("Set default config");
    IP_if_teardown_all();
    IP_static_routes_reset();

    // Disable routing
    memset(&global_conf, 0, sizeof(global_conf));
    (void)IP_global_conf_set(&global_conf);

#ifdef VTSS_SW_OPTION_DNS
    T_I("Signaling DNS");
    vtss_dns_signal();
#endif /* VTSS_SW_OPTION_DNS */
}

/******************************************************************************/
// IP_if_change_callback()
/******************************************************************************/
static void IP_if_change_callback(void)
{
    vtss::List<vtss_ifindex_t> notify;
    vtss_ip_if_callback_t      cb;
    uint32_t                   i;

    {
        IP_LOCK_SCOPE();
        notify = IP_vlan_notify;
        IP_vlan_notify.clear();
    }

    for (vtss_ifindex_t ifidx : notify) {
        T_I("Callback I/F %s", ifidx);
        for (i = 0; i < IP_IF_SUBSCRIBER_CNT_MAX; i++) {
            // We don't have a vtss_ip_if_callback_del() function so there are
            // no race conditions here (anymore).
            {
                IP_LOCK_SCOPE();
                cb = IP_subscribers[i];
            }

            if (cb == nullptr) {
                continue;
            }

            (*cb)(ifidx);
        }
    }
}

/******************************************************************************/
// IP_if_poll_fwd_state()
/******************************************************************************/
inline static void IP_if_poll_fwd_state(ip_if_itr_t if_itr)
{
    ip_vif_fwd_t new_fwd = VIF_FWD_BLOCKING; // Assume blocking
    ip_vif_fwd_t old_fwd = if_itr->second.combined_fwd;

    IP_CRIT_ASSERT_LOCKED();

    if (if_itr->second.unit_fwd == VIF_FWD_FORWARDING) {
        new_fwd = VIF_FWD_FORWARDING;
    }

    if (old_fwd == new_fwd) {
        T_D("%s: No change (%s)", if_itr->first, IP_vif_fwd_to_str(new_fwd));
        return;
    }

    T_I("%s: %s->%s", if_itr->first, IP_vif_fwd_to_str(old_fwd), IP_vif_fwd_to_str(new_fwd));

    if_itr->second.combined_fwd = new_fwd;

    ip_dhcp6c_fwd_change(if_itr->first, old_fwd == VIF_FWD_FORWARDING, new_fwd == VIF_FWD_FORWARDING);

#ifdef VTSS_SW_OPTION_SYSLOG
    char buf[64];
    S_N("LINK-UPDOWN: IP Interface %s changed state to %s.", vtss_ifindex2str(buf, sizeof(buf), if_itr->first), new_fwd == VIF_FWD_FORWARDING ? "up" : "down");
#endif /* VTSS_SW_OPTION_SYSLOG */

    ip_if_signal(if_itr->first);
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_dhcp6c_timer()
/******************************************************************************/
static void IP_if_dhcp6c_timer(uint32_t timeout_secs)
{
    ip_if_itr_t if_itr;

    IP_LOCK_SCOPE();

    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
        if (!if_itr->second.dhcp6c.active) {
            continue;
        }

        if (if_itr->second.dhcp6c.lt_valid <= timeout_secs) {
            T_IG(IP_TRACE_GRP_DHCP6C, "DHCPv6 timeout, I/F %s, network %s", if_itr->first, if_itr->second.dhcp6c.network);
            (void)IP_if_ipv6_dhcp_del(if_itr);
        } else {
            if_itr->second.dhcp6c.lt_valid -= timeout_secs;
        }
    }
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_thread()
/******************************************************************************/
static void IP_thread(vtss_addrword_t data)
{
    mesa_packet_vlan_status_t vlan_status;
    ip_if_itr_t               if_itr;
    vtss_flag_value_t         flags;
    vtss_tick_count_t         wakeup;
    vtss_tick_count_t         sleep_time;
    uint32_t                  timeout_secs = 3;

    wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(timeout_secs * 1000);

    while (1) {
        T_N("TICK");

        wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(timeout_secs * 1000);

        {
            IP_LOCK_SCOPE();
            IP_poll_fwdstate();
            sleep_time = IP_if_dhcp4c_fallback_timer_evaluate(wakeup);
        }

        T_N("sleep_time %s", sleep_time);

        do {
            flags = vtss_flag_timed_wait(&IP_control_flags, 0xffff, VTSS_FLAG_WAITMODE_OR_CLR, sleep_time);

            if (flags & IP_CTLFLAG_IFCHANGE) {
                IP_LOCK_SCOPE();

                for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
                    IP_if_poll_fwd_state(if_itr);
                }
            }

            if (flags & IP_CTLFLAG_IFNOTIFY) {
                IP_if_change_callback();
            }

            if (flags & IP_CTLFLAG_VLAN_CHANGE) {
                (void)vtss::appl::ip::filter::port_conf_update();
            }
        } while (flags);

        // Check for VLAN ingress filtering changes
        if (mesa_packet_vlan_status_get(NULL, &vlan_status) == VTSS_RC_OK && vlan_status.changed) {
            vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_VLAN_CHANGE);
        }

#if defined(VTSS_SW_OPTION_IPV6)
        IP_if_dhcp6c_timer(timeout_secs);
#endif
    }
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_if_dhcp6c_cmd()
/******************************************************************************/
static mesa_rc IP_if_dhcp6c_cmd(bool add, mesa_vid_t vid, const mesa_ipv6_network_t *network, uint64_t lt_valid)
{
    ip_if_itr_t    if_itr;
    vtss_ifindex_t ifindex = IP_vlan_to_ifindex(vid);

    if (network == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    IP_LOCK_SCOPE();

    T_I("cmd = %s, vid = %u, network = %s", add ? "add" : "del", vid, *network);

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    if (add) {
        (void)IP_if_ipv6_dhcp_del(if_itr);
        if_itr->second.dhcp6c.active  = true;
        if_itr->second.dhcp6c.network = *network;
        if_itr->second.dhcp6c.lt_valid = lt_valid;
        if (IP_vlan_up[vid]) {
            return ip_os_ipv6_add(ifindex, network);
        }
    } else {
        if (if_itr->second.dhcp6c.active) {
            return IP_if_ipv6_dhcp_del(if_itr);
        }
    }

    return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_if_statistics_link_poll()
/******************************************************************************/
static mesa_rc IP_if_statistics_link_poll(mesa_vid_t vlan, vtss_appl_ip_if_statistics_t *statistics)
{
    T_D("VLAN = %u", vlan);
    return ip_os_if_statistics_link_get(vlan, statistics);
}

/******************************************************************************/
// IP_if_statistics_link
/******************************************************************************/
static IpIfStatistics<vtss_appl_ip_if_statistics_t> IP_if_statistics_link(&IP_if_statistics_link_poll, VTSS_OS_MSEC2TICK(IP_STATISTICS_REFRESH_RATE_MSECS));

/******************************************************************************/
// IP_if_statistics_ipv6_poll()
/******************************************************************************/
static mesa_rc IP_if_statistics_ipv6_poll(mesa_vid_t vlan, vtss_appl_ip_statistics_t *statistics)
{
    T_D("VLAN = %u", vlan);
    return ip_os_if_statistics_ipv6_get(vlan, statistics);
}

/******************************************************************************/
// IP_if_statistics_ipv6
/******************************************************************************/
static IpIfStatistics<vtss_appl_ip_statistics_t> IP_if_statistics_ipv6(&IP_if_statistics_ipv6_poll, VTSS_OS_MSEC2TICK(IP_STATISTICS_REFRESH_RATE_MSECS));

/******************************************************************************/
// IP_system_statistics_ipv4_poll()
/******************************************************************************/
static mesa_rc IP_system_statistics_ipv4_poll(vtss_appl_ip_statistics_t *statistics)
{
    mesa_l3_counters_t chip_stat;

    VTSS_RC(ip_os_system_statistics_ipv4_get(statistics));
    VTSS_RC(vtss_ip_chip_counters_system_get(&chip_stat));
    statistics->HCInReceives   += chip_stat.ipv4uc_received_frames;
    statistics->HCInOctets     += chip_stat.ipv4uc_received_octets;
    statistics->HCOutTransmits += chip_stat.ipv4uc_transmitted_frames;
    statistics->HCOutOctets    += chip_stat.ipv4uc_transmitted_octets;

    // TODO, update 32bit counters
    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_system_statistics_ipv4
/******************************************************************************/
static IpSystemStatistics<vtss_appl_ip_statistics_t> IP_system_statistics_ipv4(&IP_system_statistics_ipv4_poll, VTSS_OS_MSEC2TICK(IP_STATISTICS_REFRESH_RATE_MSECS));

/******************************************************************************/
// IP_system_statistics_ipv6_poll()
/******************************************************************************/
static mesa_rc IP_system_statistics_ipv6_poll(vtss_appl_ip_statistics_t *statistics)
{
    mesa_l3_counters_t chip_stat;

    VTSS_RC(ip_os_system_statistics_ipv6_get(statistics));
    VTSS_RC(vtss_ip_chip_counters_system_get(&chip_stat));
    statistics->HCInReceives   += chip_stat.ipv6uc_received_frames;
    statistics->HCInOctets     += chip_stat.ipv6uc_received_octets;
    statistics->HCOutTransmits += chip_stat.ipv6uc_transmitted_frames;
    statistics->HCOutOctets    += chip_stat.ipv6uc_transmitted_octets;

    // TODO, update 32bit counters
    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_system_statistics_ipv6
/******************************************************************************/
static IpSystemStatistics<vtss_appl_ip_statistics_t> IP_system_statistics_ipv6(&IP_system_statistics_ipv6_poll, VTSS_OS_MSEC2TICK(IP_STATISTICS_REFRESH_RATE_MSECS));

/******************************************************************************/
// IP_vlan_global_conf_change_callback()
/******************************************************************************/
static void IP_vlan_global_conf_change_callback(mesa_etype_t tpid)
{
    vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_VLAN_CHANGE);
}

/******************************************************************************/
// IP_vlan_port_conf_change_callback()
/******************************************************************************/
static void IP_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *conf)
{
    vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_VLAN_CHANGE);
}

/******************************************************************************/
// IP_route_status_map_update()
/******************************************************************************/
static mesa_rc IP_route_status_map_update(void)
{
    static uint64_t last_poll_time;
    uint64_t        now;

    // The IP mutex must not have been taken prior to calling this function,
    // because vtss_appl_ip_route_status_get_all() takes it.

    now  = vtss::uptime_seconds();
    if (last_poll_time == 0 || now - last_poll_time > 10) {
        // Cache route status. We cache all routes whether we are asked for IPv4
        // or IPv6.
        VTSS_RC(vtss_appl_ip_route_status_get_all(IP_route_status_map, VTSS_APPL_IP_ROUTE_TYPE_ANY));
        last_poll_time = now;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IMPLEMENTATION OF PUBLIC API (vtss_appl_ip_XXX()) BEGIN
/******************************************************************************/

/******************************************************************************/
// vtss_appl_ip_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_capabilities_get(vtss_appl_ip_capabilities_t *cap)
{
    if (cap == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    *cap = IP_cap;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_global_conf_get(vtss_appl_ip_global_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    IP_LOCK_SCOPE();

    *conf = IP_global_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ip_global_conf_set(const vtss_appl_ip_global_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

#if !defined(VTSS_SW_OPTION_L3RT)
    if (conf->routing_enable) {
        return VTSS_APPL_IP_RC_ROUTING_NOT_SUPPORTED;
    }
#endif

    IP_LOCK_SCOPE();

    return IP_global_conf_set(conf);
}

/******************************************************************************/
// vtss_appl_ip_if_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_itr(const vtss_ifindex_t *const in, vtss_ifindex_t *const out, mesa_bool_t vlan_interfaces_only)
{
    ip_if_itr_t    if_itr;
    vtss_ifindex_t start_ifindex;

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    start_ifindex = in ? *in : VTSS_IFINDEX_NONE;

    IP_LOCK_SCOPE();

    if ((if_itr = IP_if_state_map.greater_than(start_ifindex)) == IP_if_state_map.end()) {
        // No more interfaces
        return VTSS_RC_ERROR;
    }

    // We take advantage of the fact that CPU port interfaces have a greater
    // ordinal than VLAN interfaces here, and therefore come last in the
    // IP_if_state_map.
    if (vlan_interfaces_only && vtss_ifindex_as_vlan(if_itr->first) == 0) {
        // Not a VLAN interface
        return VTSS_RC_ERROR;
    }

    *out = if_itr->first;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_exists()
/******************************************************************************/
mesa_bool_t vtss_appl_ip_if_exists(vtss_ifindex_t ifindex)
{
    T_N("I/F %s", ifindex);
    IP_LOCK_SCOPE();
    return ip_if_exists(ifindex) == VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_default_get(vtss_appl_ip_if_conf_t *default_conf)
{
    if (default_conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    *default_conf = IP_if_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_t *conf)
{
    ip_if_itr_t if_itr;

    T_D("I/F %s", ifindex);

    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_N("ifindex = %s", ifindex);

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    *conf = if_itr->second.if_config.conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_set()
// Create a VLAN interface.
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_t *conf)
{
    ip_if_itr_t            if_itr;
    vtss_appl_ip_if_conf_t local_conf;
    bool                   is_vlan_if, newly_created = false;
    mesa_rc                rc = VTSS_RC_OK;

    local_conf = conf == nullptr ? IP_if_default_conf : *conf;

    T_D("ifindex = %s, conf = %s", ifindex, local_conf);

    if (local_conf.mtu < 1280 || local_conf.mtu > IP_if_default_conf.mtu) {
        return VTSS_APPL_IP_RC_IF_MTU_INVALID;
    }

    is_vlan_if = vtss_ifindex_is_vlan(ifindex);

    if (!is_vlan_if) {
#ifdef VTSS_SW_OPTION_CPUPORT
        if (!vtss_ifindex_is_cpu(ifindex))
#endif
        {
            return VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN_OR_CPU;
        }
    }

    IP_LOCK_SCOPE();

    if (ip_if_exists(ifindex, &if_itr) != VTSS_RC_OK) {
        newly_created = true;
        T_I("%s: Adding new I/F", ifindex);

        // Add new interface if there's room
        if (IP_if_state_map.size() >= IP_cap.interface_cnt_max) {
            return VTSS_APPL_IP_RC_IF_LIMIT_REACHED;
        }

        // Create a new entry in our map
        if ((if_itr = IP_if_state_map.get(ifindex)) == IP_if_state_map.end()) {
            return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
        }

        vtss_clear(if_itr->second);
        if_itr->second.vlan                     = vtss_ifindex_as_vlan(ifindex);
        if_itr->second.dhcp4c_start_timer.ifidx = ifindex;
        if_itr->second.mac_address              = IP_main_mac;
        ip_acd4_sm_init(if_itr);

        // Force re-evaluation of I/F state
        IP_if_flags_force_sync(ifindex);

        if (is_vlan_if) {
            if ((rc = ip_os_if_add(ifindex)) != VTSS_RC_OK) {
                T_E("Add of %s failed: %s", ifindex, error_txt(rc));
                goto do_exit;
            }

            // The administrative mode is UP if the VLAN is UP
            if (IP_vlan_up[if_itr->second.vlan]) {
                if ((rc = ip_os_if_ctl(ifindex, true)) != VTSS_RC_OK) {
                    T_E("ip_os_if_ctl(%s) failed: %s", ifindex, error_txt(rc));
                    (void)ip_os_if_del(ifindex);
                    goto do_exit;
                }
            }
        }
    }

    if (is_vlan_if) {
        // Update interface settings
        if ((rc = ip_os_if_set(ifindex, &local_conf)) != VTSS_RC_OK) {
            T_E("ip_os_if_set(%s) failed: %s", ifindex, error_txt(rc));
            goto do_exit;
        }
    }

    if_itr->second.if_config.conf = local_conf;
    ip_if_signal(ifindex);

do_exit:
    if (newly_created && rc != VTSS_RC_OK) {
        IP_if_state_map.erase(ifindex);
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_del(vtss_ifindex_t ifindex)
{
    ip_if_itr_t if_itr;
    mesa_rc     rc;

    T_D("ifindex = %s", ifindex);

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    ip_dhcp6c_if_del(ifindex);

    rc = IP_if_teardown(if_itr);

    IP_if_state_map.erase(if_itr);

    // Notify here, as we can not convert ifindex to if_state after this
    ip_if_signal(ifindex);

    return rc;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv4_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_ipv4_default_get(vtss_appl_ip_if_conf_ipv4_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);
    conf->fallback_timeout_secs = 60;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv4_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_ipv4_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_ipv4_t *conf)
{
    ip_if_itr_t if_itr;

    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_D("ifindex = %s", ifindex);

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    *conf = if_itr->second.if_config.ipv4;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv4_set()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_ipv4_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_ipv4_t *conf)
{
    ip_if_itr_t                 if_itr;
    vtss_appl_ip_if_conf_ipv4_t *cur, local_conf;
    vtss_ifindex_elm_t          elm;
    bool                        update_timer;
    size_t                      len;
    uint32_t                    known_dhcpc_flags;
    mesa_rc                     rc = VTSS_RC_OK;

    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_D("ifindex = %s, conf = %s", ifindex, *conf);

    if (conf->enable) {
        if (conf->dhcpc_enable) {
            known_dhcpc_flags = VTSS_APPL_IP_DHCP4C_FLAG_NONE      |
                                VTSS_APPL_IP_DHCP4C_FLAG_OPTION_43 |
                                VTSS_APPL_IP_DHCP4C_FLAG_OPTION_60 |
                                VTSS_APPL_IP_DHCP4C_FLAG_OPTION_67;

            if (conf->dhcpc_params.dhcpc_flags & ~known_dhcpc_flags) {
                return VTSS_APPL_IP_RC_IF_DHCP_UNSUPPORTED_FLAGS;
            }

            // Check client-id
            switch (conf->dhcpc_params.client_id.type) {
            case VTSS_APPL_IP_DHCP4C_ID_TYPE_AUTO:
                // Do nothing
                break;

            case VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC:
                if (vtss_ifindex_decompose(conf->dhcpc_params.client_id.if_mac, &elm) != VTSS_RC_OK ||
                    (elm.iftype != VTSS_IFINDEX_TYPE_NONE && elm.iftype != VTSS_IFINDEX_TYPE_PORT)) {
                    // Only allow none or port type of ifindex
                    return VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_IF_MAC;
                }

                break;

            case VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII:
                len = strlen(conf->dhcpc_params.client_id.ascii);
                if (len < VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH || len > VTSS_APPL_IP_DHCP4C_ID_MAX_LENGTH) {
                    return VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_ASCII;
                }

                break;

            case VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX:
                len = strlen(conf->dhcpc_params.client_id.ascii);
                if (len < 2 * VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH ||
                    len > 2 * VTSS_APPL_IP_DHCP4C_ID_MAX_LENGTH ||
                    misc_str_is_hex(conf->dhcpc_params.client_id.hex) != VTSS_RC_OK) {
                    return VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_HEX;
                }

                break;

            default: // Unknown types
                return VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID;
            }

            // Check hostname
            len = strlen(conf->dhcpc_params.hostname);
            if (len && misc_str_is_domainname(conf->dhcpc_params.hostname) != VTSS_RC_OK) {
                return VTSS_APPL_IP_RC_IF_DHCP_CLIENT_HOSTNAME;
            }

            if (conf->fallback_enable) {
                if (conf->fallback_timeout_secs < 1 || conf->fallback_timeout_secs > 4294967295) {
                    return VTSS_APPL_IP_RC_IF_DHCP_FALLBACK_TIMEOUT;
                }
            }
        }

        if (!conf->dhcpc_enable || (conf->dhcpc_enable && conf->fallback_enable)) {
            // If DHCP is not enabled or DHCP is enabled with a fallback, the
            // IPv4 network address must be valid.
            if (!vtss_ip_ipv4_ifaddr_valid(&conf->network)) {
                return VTSS_APPL_IP_RC_IF_INVALID_NETWORK_ADDRESS;
            }
        }

        local_conf = *conf;
        if (!local_conf.dhcpc_enable) {
            local_conf.fallback_enable = false;
        }
    } else {
        // IPv4 is not enabled, so get a normalized configuration to continue
        // with, that is, one where both enable and dhcpc_enable are false.
        vtss_clear(local_conf);
    }

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    cur = &if_itr->second.if_config.ipv4;

    T_D("%s: conf: {enable = %d. dhcp4c_enable: %s}, cur: {enable = %d, dhcp4c_enable: %s}", ifindex, local_conf.enable, local_conf.dhcpc_enable, cur->enable, cur->dhcpc_enable);

    // Further validation, that requires our mutex to be taken.
    if (local_conf.enable && (!local_conf.dhcpc_enable || (local_conf.dhcpc_enable && local_conf.fallback_enable))) {
        // IPv4 is enabled and either DHCP is not enabled or it is enabled with
        // a fallback address. In this case, the network must not conflict with
        // existing address.
        if (!IP_if_ipv4_check(&local_conf.network, ifindex)) {
            return VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_EXISTING;
        }

        VTSS_RC(IP_if_ipv4_addr_conflict_static_route_check(local_conf.network.address));
    }

    // Enable/disable DHCP client
    if (cur->enable != local_conf.enable || cur->dhcpc_enable != local_conf.dhcpc_enable) {
        if (local_conf.enable && local_conf.dhcpc_enable) {
            T_I("Start DHCP client on %s", ifindex);
            rc = IP_if_dhcp4c_start(if_itr, &local_conf);
        } else if (cur->dhcpc_enable && cur->enable) {
            // DHCP client is currently active. Disable it.
            T_I("Stop DHCP client on %s", ifindex);
            rc = IP_if_dhcp4c_kill(if_itr);
        }
    }

    // Fallback timeout
    if (cur->enable != local_conf.enable || cur->fallback_enable != local_conf.fallback_enable || cur->fallback_timeout_secs != local_conf.fallback_timeout_secs) {
        update_timer = true;
    } else {
        update_timer = false;
    }

    *cur = local_conf;
    if (update_timer) {
        T_I("Updating fallback timer on %s", ifindex);
        IP_if_dhcp4c_fallback_timer_update(if_itr);
    }

    // Time to (un)install IP address
    IP_if_dhcp4c_state_change(if_itr);

    IP_if_flags_force_sync(ifindex);
    return rc;
}

/******************************************************************************/
// vtss_appl_ip_if_ipv6_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_ipv6_conf_default_get(vtss_appl_ip_if_conf_ipv6_t *conf)
{
#if defined(VTSS_SW_OPTION_IPV6)
    if (!conf) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    return VTSS_RC_OK;
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv6_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_ipv6_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_ipv6_t *conf)
{
#if defined(VTSS_SW_OPTION_IPV6)
    ip_if_itr_t if_itr;

    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_D("ifindex = %s", ifindex);

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    *conf = if_itr->second.if_config.ipv6;
    return VTSS_RC_OK;
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv6_set()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_conf_ipv6_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_ipv6_t *conf)
{
#if defined(VTSS_SW_OPTION_IPV6)
    ip_if_itr_t                 if_itr, if_itr2;
    vtss_appl_ip_if_conf_ipv6_t *cur, local_conf;
    mesa_rc                     rc;

    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_D("ifindex = %s, conf = %s", ifindex, *conf);

    if (conf->enable) {
        if (!vtss_ip_ipv6_ifaddr_valid(&conf->network)) {
            return VTSS_APPL_IP_RC_IF_INVALID_NETWORK_ADDRESS;
        }

        local_conf = *conf;
    } else {
        // IPv6 is not enabled, so get a normalized configuration to continue
        // with.
        vtss_clear(local_conf);
    }

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    cur = &if_itr->second.if_config.ipv6;
    T_D("%s: enable = %d->%d, network = %s->%s}", ifindex, cur->enable, local_conf.enable, cur->network, local_conf.network);

    if (memcmp(&local_conf, cur, sizeof(local_conf)) == 0) {
        // No changes
        return VTSS_RC_OK;
    }

    // Further validation, that requires our mutex to be taken.
    if (local_conf.enable) {
        if (!IP_if_ipv6_check(&local_conf.network, ifindex)) {
            return VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_EXISTING;
        }

        VTSS_RC(IP_if_ipv6_addr_conflict_static_route_check(local_conf.network.address));

        // Set static address
        for (if_itr2 = IP_if_state_map.begin(); if_itr2 != IP_if_state_map.end(); ++if_itr2) {
            if (if_itr2 == if_itr) {
                // Don't check against ourselves
                continue;
            }

            if (vtss_ipv6_net_equal(&local_conf.network, &if_itr2->second.cur_ipv6)) {
                T_I("%s: Trying to add identical IP network (%s) as on %u", ifindex, local_conf.network, if_itr2->first);
                return VTSS_APPL_IP_RC_IF_ADDRESS_ALREADY_EXISTS_ON_ANOTHER_INTERFACE;
            }
        }
    }

    rc = VTSS_RC_OK;

    if (local_conf.enable) {
        // Rid old address, if any
        (void)IP_if_ipv6_static_del(if_itr);

        // Install address if the VLAN is UP
        if (vtss_ifindex_is_vlan(ifindex)) {
            // Whether or not the VLAN is UP, we must set the address as active
            // so that - should it be DOWN - the address will be installed once
            // it comes up (see also comment in vtss_ip_vlan_state_update()).
            if_itr->second.ipv6_active = true;
            if_itr->second.cur_ipv6 = local_conf.network;
            if (IP_vlan_up[if_itr->second.vlan]) {
                (void)ip_os_ipv6_add(ifindex, &local_conf.network);
            }
        } else {
            T_E("No implementation for %s", ifindex);
            rc = VTSS_APPL_IP_RC_INTERNAL_ERROR;
        }
    } else if (cur->enable) {
        // Clear all IPv6 addresses
        T_I("Clearing IPv6 addresses on I/F %s", ifindex);
        rc = IP_if_ipv6_static_del(if_itr);
    }

    if (rc == VTSS_RC_OK) {
        *cur = local_conf;
        IP_if_flags_force_sync(ifindex);
    }

    return rc;
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_if_status_link_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_link_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_status_link_t *status)
{
    if (status == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    return status_if_link.get(ifindex, status);
}

/******************************************************************************/
// vtss_appl_ip_if_status_ipv4_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_ipv4_itr(const vtss_appl_ip_if_key_ipv4_t *in, vtss_appl_ip_if_key_ipv4_t *out)
{
    vtss_appl_ip_if_info_ipv4_t info;

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (in) {
        *out = *in;
    } else {
        vtss_clear(*out);
    }

    return status_if_ipv4.get_next(out, &info);
}

/******************************************************************************/
// vtss_appl_ip_if_status_ipv4_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_ipv4_get(const vtss_appl_ip_if_key_ipv4_t *key, vtss_appl_ip_if_status_ipv4_t *status)
{
    mesa_rc rc;

    if (key == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if ((rc = status_if_ipv4.get(key, &status->info)) == VTSS_RC_OK) {
        status->net = key->addr;
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_ip_if_status_ipv6_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_ipv6_itr(const vtss_appl_ip_if_key_ipv6_t *in, vtss_appl_ip_if_key_ipv6_t *out)
{
#if defined(VTSS_SW_OPTION_IPV6)
    vtss_appl_ip_if_info_ipv6_t info;

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (in) {
        *out = *in;
    } else {
        vtss_clear(*out);
    }

    return status_if_ipv6.get_next(out, &info);
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_if_status_ipv6_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_ipv6_get(const vtss_appl_ip_if_key_ipv6_t *key, vtss_appl_ip_if_status_ipv6_t *status)
{
#if defined(VTSS_SW_OPTION_IPV6)
    mesa_rc rc;

    if (key == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if ((rc = status_if_ipv6.get(key, &status->info)) == VTSS_RC_OK) {
        status->net = key->addr;
    }

    return rc;
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_if_status_dhcp4c_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_dhcp4c_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_status_dhcp4c_t *status)
{
    if (status == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    return status_if_dhcp4c.get(ifindex, status);
}

/******************************************************************************/
// vtss_appl_ip_if_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_status_type_t type, uint32_t max, uint32_t *cnt, vtss_appl_ip_if_status_t *status)
{
    ip_if_itr_t if_itr;

    if (type != VTSS_APPL_IP_IF_STATUS_TYPE_ANY  &&
        type != VTSS_APPL_IP_IF_STATUS_TYPE_LINK &&
        type != VTSS_APPL_IP_IF_STATUS_TYPE_IPV4 &&
#if defined(VTSS_SW_OPTION_IPV6)
        type != VTSS_APPL_IP_IF_STATUS_TYPE_IPV6 &&
#endif
        type != VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (status == nullptr || max < 1) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (max > 1 && cnt == nullptr) {
        // User must get to know how many entries were returned if he wants more
        // than just one.
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_N("I/F = %s, type = %s, max = %u", ifindex, type, max);

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    vtss::Vector<vtss_appl_ip_if_status_t> v(max);
    v.max_size(max);

    // Get status
    VTSS_RC(IP_if_status_get(if_itr, type, v));

    if (v.size() == 0) {
        // At least one must be returned.
        return VTSS_RC_ERROR;
    }

    if (cnt != nullptr) {
        *cnt = v.size();
    }

    for (const auto &e : v) {
        *(status++) = e;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_if_status_find()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_status_find(const mesa_ip_addr_t *ip, vtss_appl_ip_if_status_t *status)
{
    if (ip == nullptr || status == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (ip->type != MESA_IP_TYPE_IPV4 && ip->type != MESA_IP_TYPE_IPV6) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*status);

    if (ip->type == MESA_IP_TYPE_IPV4) {
        auto lock = status_if_ipv4.lock_get(__FILE__, __LINE__);
        auto ref  = status_if_ipv4.ref(lock);
        auto i    = ref.begin();
        auto e    = ref.end();

        for (; i != e; ++i) {
            if (i->first.addr.address == ip->addr.ipv4) {
                status->type        = VTSS_APPL_IP_IF_STATUS_TYPE_IPV4;
                status->ifindex     = i->first.ifindex;
                status->u.ipv4.net  = i->first.addr;
                status->u.ipv4.info = i->second;
                return VTSS_RC_OK;
            }
        }
    } else {
#if defined(VTSS_SW_OPTION_IPV6)
        auto lock = status_if_ipv6.lock_get(__FILE__, __LINE__);
        auto ref  = status_if_ipv6.ref(lock);
        auto i    = ref.begin();
        auto e    = ref.end();

        for (; i != e; ++i) {
            if (i->first.addr.address == ip->addr.ipv6) {
                status->type        = VTSS_APPL_IP_IF_STATUS_TYPE_IPV6;
                status->ifindex     = i->first.ifindex;
                status->u.ipv6.net  = i->first.addr;
                status->u.ipv6.info = i->second;
                return VTSS_RC_OK;
            }
        }
#else
        return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
    }

    return VTSS_RC_ERROR;
}

//******************************************************************************
// vtss_appl_ip_global_notification_status_get()
//******************************************************************************
mesa_rc vtss_appl_ip_global_notification_status_get(vtss_appl_ip_global_notification_status_t *const global_notif_status)
{
    if (!global_notif_status) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    // No need to lock scope, because the psec_global_notification_status
    // structure incorporates its own locking mechanism.
    return ip_global_notification_status.get(global_notif_status);
}

/******************************************************************************/
// vtss_appl_ip_if_statistics_link_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_statistics_link_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_statistics_t *statistics)
{
    ip_if_itr_t if_itr;

    if (statistics == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    if (if_itr->second.vlan == 0) {
        // This is a CPU port, but this function currently only supports VLAN
        // IP interfaces.
        return VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN;
    }

    return IP_if_statistics_link.get(if_itr->second.vlan, statistics);
}

/******************************************************************************/
// vtss_appl_ip_if_statistics_link_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_statistics_link_clear(vtss_ifindex_t ifindex)
{
    ip_if_itr_t if_itr;

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    if (if_itr->second.vlan == 0) {
        // This is a CPU port, but this function currently only supports VLAN
        // IP interfaces.
        return VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN;
    }

    return IP_if_statistics_link.reset(if_itr->second.vlan);
}

/******************************************************************************/
// vtss_appl_ip_if_statistics_ipv4_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_statistics_ipv4_get(vtss_ifindex_t ifindex, vtss_appl_ip_statistics_t *statistics)
{
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ip_if_statistics_ipv4_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_statistics_ipv4_clear(vtss_ifindex_t ifindex)
{
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ip_if_statistics_ipv6_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_statistics_ipv6_get(vtss_ifindex_t ifindex, vtss_appl_ip_statistics_t *statistics)
{
#if defined(VTSS_SW_OPTION_IPV6)
    ip_if_itr_t if_itr;

    if (statistics == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    if (if_itr->second.vlan == 0) {
        // This is a CPU port, but this function currently only supports VLAN
        // IP interfaces.
        return VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN;
    }

    return IP_if_statistics_ipv6.get(if_itr->second.vlan, statistics);
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_if_statistics_ipv6_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_statistics_ipv6_clear(vtss_ifindex_t ifindex)
{
#if defined(VTSS_SW_OPTION_IPV6)
    ip_if_itr_t if_itr;

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    if (if_itr->second.vlan == 0) {
        // This is a CPU port, but this function currently only supports VLAN
        // IP interfaces.
        return VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN;
    }

    return IP_if_statistics_ipv6.reset(if_itr->second.vlan);
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_system_statistics_ipv4_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_system_statistics_ipv4_get(vtss_appl_ip_statistics_t *statistics)
{
    IP_LOCK_SCOPE();
    return IP_system_statistics_ipv4.get(statistics);
}
/******************************************************************************/
// vtss_appl_ip_system_statistics_ipv4_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_system_statistics_ipv4_clear(void)
{
    IP_LOCK_SCOPE();
    return IP_system_statistics_ipv4.reset();
}

/******************************************************************************/
// vtss_appl_ip_system_statistics_ipv6_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_system_statistics_ipv6_get(vtss_appl_ip_statistics_t *statistics)
{
#if defined(VTSS_SW_OPTION_IPV6)
    IP_LOCK_SCOPE();
    return IP_system_statistics_ipv6.get(statistics);
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_system_statistics_ipv6_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_system_statistics_ipv6_clear(void)
{
#if defined(VTSS_SW_OPTION_IPV6)
    IP_LOCK_SCOPE();
    return IP_system_statistics_ipv6.reset();
#else
    return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif // VTSS_SW_OPTION_IPV6
}

/******************************************************************************/
// vtss_appl_ip_acd_status_ipv4_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_acd_status_ipv4_itr(const vtss_appl_ip_acd_status_ipv4_key_t *in, vtss_appl_ip_acd_status_ipv4_key_t *out)
{
    vtss_appl_ip_acd_status_ipv4_t status;

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (in == nullptr) {
        return status_acd_ipv4.get_first(out, &status);
    }

    *out = *in;
    return status_acd_ipv4.get_next(out, &status);
}

/******************************************************************************/
// vtss_appl_ip_acd_status_ipv4_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_acd_status_ipv4_get(const vtss_appl_ip_acd_status_ipv4_key_t *key, vtss_appl_ip_acd_status_ipv4_t *status)
{
    return status_acd_ipv4.get(key, status);
}

/******************************************************************************/
// vtss_appl_ip_acd_status_ipv4_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_acd_status_ipv4_clear(const vtss_appl_ip_acd_status_ipv4_key_t *key)
{
    if (key) {
        return status_acd_ipv4.del(key);
    }

    status_acd_ipv4.clear();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_neighbor_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_neighbor_itr(const vtss_appl_ip_neighbor_key_t *in, vtss_appl_ip_neighbor_key_t *out, mesa_ip_type_t type)
{
    vtss_appl_ip_neighbor_status_t status;

#if defined(VTSS_SW_OPTION_IPV6)
    vtss_appl_ip_neighbor_key_t    ipv4_out, ipv6_out;
    bool                           ipv4_ok,  ipv6_ok;
#endif

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

#if !defined(VTSS_SW_OPTION_IPV6)
    if (type == MESA_IP_TYPE_NONE) {
        type = MESA_IP_TYPE_IPV4;
    } else if (type == MESA_IP_TYPE_IPV6) {
        return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
    }
#endif // !defined(VTSS_SW_OPTION_IPV6)

#if defined(VTSS_SW_OPTION_IPV6)
    if (type == MESA_IP_TYPE_NONE) {
        // Gotta combine both IPv4 and IPv6. In order to support RFC4293, the
        // key must be sorted in this order:
        // 1) by ifindex
        // 2) by type (IPv4/IPv6)
        // 3) by IP address

        // The underlying status_nb_ipv4 and status_nb_ipv6 tables are sorted
        // in this order
        // 1) by ifindex
        // 2) by IP address.
        // Strictly speaking they also are sorted by type after ifindex, but
        // they only hold one type each.
        if (in == nullptr) {
            // Get the first from both the IPv4 and the IPv6 table.
            ipv4_ok = status_nb_ipv4.get_first(&ipv4_out, &status) == VTSS_RC_OK;
            ipv6_ok = status_nb_ipv6.get_first(&ipv6_out, &status) == VTSS_RC_OK;
        } else {
            // Here, we have an input key to get next from. How to get next
            // depends on the input type.
            if (in->dip.type <= MESA_IP_TYPE_IPV4) {
                // We can do the IPv4 lookup directly on the input key.
                ipv4_out = *in;
                ipv4_ok = status_nb_ipv4.get_next(&ipv4_out, &status) == VTSS_RC_OK;

                // The IPv6 lookup has got to be on a modified input key with
                // cleared IP address and ifindex set to the input's, since IPv6
                // addresses come after IPv4 addresses on the same ifindex.
                // Basically, we are doing a "get-first-ipv6-on-a-particular-
                // interface".
                vtss_clear(ipv6_out);
                ipv6_out.dip.type = MESA_IP_TYPE_IPV6;
                ipv6_out.ifindex  = in->ifindex;
                ipv6_ok = status_nb_ipv6.get_next(&ipv6_out, &status) == VTSS_RC_OK;
            } else if (in->dip.type == MESA_IP_TYPE_IPV6) {
                // Input is an IPv6 address, so we can do a direct lookup of the
                // next IPv6 address.
                ipv6_out = *in;
                ipv6_ok = status_nb_ipv6.get_next(&ipv6_out, &status) == VTSS_RC_OK;

                // However, the IPv4 address lookup must be on the next ifindex,
                // because IPv4 addresses come before IPv6 addresses in this
                // combined iterator.
                vtss_clear(ipv4_out);
                ipv4_out.dip.type = MESA_IP_TYPE_IPV4;
                ipv4_out.ifindex  = in->ifindex + 1;
                ipv4_ok = status_nb_ipv4.get_next(&ipv4_out, &status) == VTSS_RC_OK;
            } else {
                // Unknown input.
                return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
            }
        }

        // Time to combine the results we've obtained so far.
        if (ipv4_ok && ipv6_ok) {
            // Both succeeded. Return the one with the lowest ifindex, IPv4 if
            // equal.
            if (ipv4_out.ifindex <= ipv6_out.ifindex) {
                *out = ipv4_out;
            } else {
                *out = ipv6_out;
            }
        } else if (ipv4_ok) {
            *out = ipv4_out;
        } else if (ipv6_ok) {
            *out = ipv6_out;
        } else {
            // No (more) entries.
            return VTSS_RC_ERROR;
        }

        return VTSS_RC_OK;
    }
#endif // defined(VTSS_SW_OPTION_IPV6)

    if (type == MESA_IP_TYPE_IPV4) {
        if (in == nullptr) {
            return status_nb_ipv4.get_first(out, &status);
        } else {
            *out = *in;
            return status_nb_ipv4.get_next(out, &status);
        }
    }

#if defined(VTSS_SW_OPTION_IPV6)
    if (type == MESA_IP_TYPE_IPV6) {
        if (in == nullptr) {
            return status_nb_ipv6.get_first(out, &status);
        } else {
            *out = *in;
            return status_nb_ipv6.get_next(out, &status);
        }
    }
#endif // VTSS_SW_OPTION_IPV6

    // Unknown type to iterate across.
    return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
}

/******************************************************************************/
// vtss_appl_ip_neighbor_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_neighbor_status_get(const vtss_appl_ip_neighbor_key_t *key, vtss_appl_ip_neighbor_status_t *status)
{
    if (key == nullptr || status == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    if (key->dip.type == MESA_IP_TYPE_IPV4) {
        return status_nb_ipv4.get(key, status);
    } else if (key->dip.type == MESA_IP_TYPE_IPV6) {
#if defined(VTSS_SW_OPTION_IPV6)
        return status_nb_ipv6.get(key, status);
#else
        return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
#endif
    }

    return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
}

/******************************************************************************/
// vtss_appl_ip_neighbor_clear()
/******************************************************************************/
mesa_rc vtss_appl_ip_neighbor_clear(mesa_ip_type_t type)
{
    if (type != MESA_IP_TYPE_NONE && type != MESA_IP_TYPE_IPV4 && type != MESA_IP_TYPE_IPV6) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

#if !defined(VTSS_SW_OPTION_IPV6)
    if (type == MESA_IP_TYPE_IPV6) {
        return VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED;
    }
#endif // !VTSS_SW_OPTION_IPV6

    return ip_os_neighbor_clear(type);
}

/******************************************************************************/
// vtss_appl_ip_if_dhcp4c_control_restart()
/******************************************************************************/
mesa_rc vtss_appl_ip_if_dhcp4c_control_restart(vtss_ifindex_t ifindex)
{
    ip_if_itr_t if_itr;

    IP_LOCK_SCOPE();

    VTSS_RC(ip_if_exists(ifindex, &if_itr));

    if (!if_itr->second.if_config.ipv4.dhcpc_enable) {
        return VTSS_APPL_IP_RC_IF_NOT_USING_DHCP;
    }

    if (if_itr->second.ipv4_active) {
        (void)IP_if_ipv4_del(if_itr);
    }

    return vtss::dhcp::client_start(ifindex, &if_itr->second.if_config.ipv4.dhcpc_params);
}

/******************************************************************************/
// vtss_appl_ip_route_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_conf_default_get(vtss_appl_ip_route_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    *conf = IP_route_conf_default;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_route_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_conf_get(const vtss_appl_ip_route_key_t *rt, vtss_appl_ip_route_conf_t *conf)
{
    ip_static_route_itr_t static_route_itr;

    if (rt == nullptr || conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    IP_LOCK_SCOPE();

    if ((static_route_itr = IP_static_route_map.find(*rt)) == IP_static_route_map.end()) {
        return VTSS_APPL_IP_RC_ROUTE_DOESNT_EXIST;
    }

    *conf = static_route_itr->second;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_route_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_conf_set(const vtss_appl_ip_route_key_t *key, const vtss_appl_ip_route_conf_t *conf)
{
    vtss_appl_ip_route_key_t  local_key;
    vtss_appl_ip_route_conf_t local_conf;

    if (key == nullptr || conf == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    T_D("route = %s", *key);

    // Check the route
    VTSS_RC(IP_route_valid(*key));

    // Take a copy, so that we can modify it locally if needed.
    local_key  = *key;
    local_conf = *conf;

    // Check the configuration
    if (conf->distance < 1 || conf->distance > 255) {
        return VTSS_APPL_IP_RC_ROUTE_DISTANCE_INVALID;
    }

#if defined(VTSS_SW_OPTION_IPV6)
    // Also clear the VLAN if we the destination is not link-local
    if (key->type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC && !vtss_ipv6_addr_is_link_local(&key->route.ipv6_uc.destination)) {
        (void)vtss_ifindex_from_vlan(0, &local_key.vlan_ifindex);
    }
#endif

    IP_LOCK_SCOPE();

    return IP_route_add(local_key, local_conf, false /* Not DHCP-assigned */);
}

/******************************************************************************/
// vtss_appl_ip_route_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_conf_del(const vtss_appl_ip_route_key_t *key)
{
    if (key == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    VTSS_RC(IP_route_type_validate(key->type, false));

    T_D("route = %s", *key);

    IP_LOCK_SCOPE();

    return IP_route_del(*key, false /* It's not DHCP that is deleting */);
}

/******************************************************************************/
// vtss_appl_ip_route_conf_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_conf_itr(const vtss_appl_ip_route_key_t *in, vtss_appl_ip_route_key_t *out, vtss_appl_ip_route_type_t type)
{
    ip_static_route_itr_t itr;

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    VTSS_RC(IP_route_type_validate(type, true));

    IP_LOCK_SCOPE();

    if (in) {
        itr = IP_static_route_map.greater_than(*in);
    } else {
        itr = IP_static_route_map.begin();
    }

    while (itr != IP_static_route_map.end()) {
        if (type == VTSS_APPL_IP_ROUTE_TYPE_ANY || type == itr->first.type) {
            *out = itr->first;
            return VTSS_RC_OK;
        }

        ++itr;
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ip_route_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_status_get(const vtss_appl_ip_route_status_key_t *key, vtss_appl_ip_route_status_t *status)
{
    vtss_appl_ip_route_status_map_itr_t itr;

    if (key == nullptr || status == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    // Potentially update our cached routes.
    // Don't lock scope before calling this function.
    VTSS_RC(IP_route_status_map_update());

    IP_LOCK_SCOPE();

    if ((itr = IP_route_status_map.find(*key)) == IP_route_status_map.end()) {
        return VTSS_RC_ERROR;
    }

    *status = itr->second;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ip_route_status_itr()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_status_itr(const vtss_appl_ip_route_status_key_t *in, vtss_appl_ip_route_status_key_t *out, vtss_appl_ip_route_type_t type)
{
    vtss_appl_ip_route_status_map_itr_t itr;

    if (out == nullptr) {
        return VTSS_APPL_IP_RC_INVALID_ARGUMENT;
    }

    VTSS_RC(IP_route_type_validate(type, true));

    // Potentially update our cached routes.
    // Don't lock scope before calling this function.
    VTSS_RC(IP_route_status_map_update());

    IP_LOCK_SCOPE();

    if (in) {
        itr = IP_route_status_map.greater_than(*in);
    } else {
        itr = IP_route_status_map.begin();
    }

    while (itr != IP_route_status_map.end()) {
        if (type == VTSS_APPL_IP_ROUTE_TYPE_ANY || type == itr->first.route.type) {
            *out = itr->first;
            return VTSS_RC_OK;
        }

        ++itr;
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ip_route_status_get_all()
/******************************************************************************/
mesa_rc vtss_appl_ip_route_status_get_all(vtss_appl_ip_route_status_map_t &routes, vtss_appl_ip_route_type_t type, vtss_appl_ip_route_protocol_t protocol)
{
    vtss_appl_ip_route_status_map_itr_t itr;
    ip_static_route_itr_t               user_route_itr;
    vtss_appl_ip_route_key_t            tmp_rt;
    vtss_appl_ip_route_protocol_t       proto;
    vtss_appl_ip_route_status_key_t     user_duplicate_route,  dhcp_duplicate_route;
    vtss_appl_ip_route_status_t         user_duplicate_status, dhcp_duplicate_status;
    bool                                user_route_found,      dhcp_route_found;

    VTSS_RC(IP_route_type_validate(type, true));
    VTSS_RC(IP_route_protocol_validate(protocol, true));

    if (protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP) {
        // FRR doesn't have a notion of DHCP-installed routes, as they are
        // installed as static routes, so that's what we ask for.
        // Later on in this function, we filter non-DHCP-assigned routes out.
        proto = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
    } else {
        proto = protocol;
    }

    // We need to lock the scope, because we need to make sure that what we get
    // from the daemon is consistent with what DHCP and/or user have configured.
    IP_LOCK_SCOPE();

    VTSS_RC(frr_ip_route_status_get(routes, type, proto));

    if (protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_ANY    &&
        protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC &&
        protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP) {
        // Non-static routes don't need special treatment.
        return VTSS_RC_OK;
    }

    // Gotta run through the routes and figure out whether a route is user- or
    // DHCP-installed, and filter depending on what the user asks for - or even
    // add the appropriate route. The IP_static_route_map only contains
    // user-defined routes, whereas IP_if_state_map contains potential
    // DHCP-defined routes.
    itr = routes.begin();
    while (itr != routes.end()) {
        // Since we may erase and add routes in this loop, save a pointer to the
        // next entry.
        auto next_itr = itr;
        ++next_itr;

        if (itr->first.protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC) {
            // Don't care about non-static routes.
            itr = next_itr;
            continue;
        }

        // Before being able to look-up IPv6 routes in our static route map, we
        // must clear the vlan_ifindex of the route key unless it's a link-local
        // address, where it must be there. The thing is that the FRR code
        // returns in vlan_ifindex the actual VLAN on which this route is
        // installed, whereas our static route map has no notion of VLAN
        // interface unless it's link-local.
        tmp_rt = itr->first.route;
        if (tmp_rt.type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC && !vtss_ipv6_addr_is_link_local(&tmp_rt.route.ipv6_uc.destination)) {
            tmp_rt.vlan_ifindex = VTSS_IFINDEX_NONE;
        }

        user_route_itr   = IP_static_route_map.find(tmp_rt);
        user_route_found = user_route_itr != IP_static_route_map.end();
        dhcp_route_found = IP_route_is_dhcp_installed(itr->first.route, nullptr);

        T_D("Route %s: user route: %d, DHCP route: %d", itr->first.route, user_route_found, dhcp_route_found);

        if (user_route_found && dhcp_route_found) {
            // Both a user- and a DHCP-installed route is found.
            // Since we can't modify the key of the routes map, we can just as
            // well erase the current key and add two new - one for DHCP- and
            // one for the user-installed route.
            user_duplicate_route = itr->first;
            dhcp_duplicate_route = itr->first;
            user_duplicate_route.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
            dhcp_duplicate_route.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP;

            if (user_route_itr->second.distance <= IP_ROUTE_DHCP_DISTANCE) {
                // It's the user-installed static route that is installed in
                // FRR.  Create an inactive DHCP-installed route status.
                user_duplicate_status = itr->second;
                vtss_clear(dhcp_duplicate_status);
                dhcp_duplicate_status.distance = IP_ROUTE_DHCP_DISTANCE;
            } else {
                // It's the DHCP-installed static route that is installed in
                // FRR. Create an inactive user-installed route status.
                dhcp_duplicate_status = itr->second;
                vtss_clear(user_duplicate_status);
                user_duplicate_status.distance = user_route_itr->second.distance;
            }

            // Erase the current route.
            routes.erase(itr);

            if (protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_ANY || protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC) {
                // The user-installed route must be present in returned route
                // map.
                if ((itr = routes.get(user_duplicate_route)) == routes.end()) {
                    T_I("Out of memory");
                    return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
                }

                itr->second = user_duplicate_status;
            }

            if (protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_ANY || protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP) {
                // The DHCP-installed route must be present in returned route
                // map.
                if ((itr = routes.get(dhcp_duplicate_route)) == routes.end()) {
                    T_I("Out of memory");
                    return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
                }

                itr->second = dhcp_duplicate_status;
            }
        } else if (user_route_found) {
            // No DHCP-installed route found.
            if (protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP) {
                // User asked for DHCP-installed protocols only. Erase this one.
                routes.erase(itr);
            }
        } else if (dhcp_route_found) {
            // DHCP-installed, but no user-installed route found.
            // Potentially replace the key with a DHCP key, which requires that
            // we erase the current key.
            dhcp_duplicate_route          = itr->first;
            dhcp_duplicate_route.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP;
            dhcp_duplicate_status         = itr->second;

            routes.erase(itr);

            if (protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC) {
                // The user wants DHCP or ANY route protocol. Add the DHCP one.
                if ((itr = routes.get(dhcp_duplicate_route)) == routes.end()) {
                    T_I("Out of memory");
                    return VTSS_APPL_IP_RC_OUT_OF_MEMORY;
                }

                itr->second = dhcp_duplicate_status;
            }
        } else if (itr->second.flags & VTSS_APPL_IP_ROUTE_STATUS_FLAG_UNREACHABLE) {
            // This may happen if you e.g. add a default route (::/0 via
            // 2002::1) *and* make 2002::1 a blackhole. Then ::/0 will have two
            // nexthops: One with a nexthop of 2002::1 and the recursive flag
            // set, and another with no nexthop IP, but the unreachable flag set
            // We just leave it as is.
        } else {
            // Neither DHCP nor user installed this route. That's an error.
            T_E("Neither DHCP nor user installed this route: %s:%s", itr->first, itr->second);

            // Just erase it.
            routes.erase(itr);
        }

        itr = next_itr;
    }

    return VTSS_RC_OK;
}

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
extern "C" void vtss_ip_mib_init();
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC)
void vtss_appl_ip_json_init();
#endif

/******************************************************************************/
// IMPLEMENTATION OF SEMI_PUBLIC API (vtss_ip_XXX()) BEGIN
/******************************************************************************/

/******************************************************************************/
// vtss_ip_if_callback_add()
/******************************************************************************/
mesa_rc vtss_ip_if_callback_add(const vtss_ip_if_callback_t cb)
{
    int i;

    IP_LOCK_SCOPE();
    for (i = 0; i < IP_IF_SUBSCRIBER_CNT_MAX; ++i) {
        if (IP_subscribers[i] != NULL) {
            continue;
        }

        IP_subscribers[i] = cb;
        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_ip_vlan_state_update()
// Invoked from ip_filter_api.cxx#port_conf_update(), only.
/******************************************************************************/
void vtss_ip_vlan_state_update(bool vlan_up[MESA_VIDS])
{
    ip_if_itr_t if_itr;
    mesa_vid_t  vid;
    bool        new_up;
    mesa_rc     rc;

    IP_LOCK_SCOPE();

    for (if_itr = IP_if_state_map.begin(); if_itr != IP_if_state_map.end(); ++if_itr) {
        vid = if_itr->second.vlan;

        if (vid == 0) {
            // Not a VLAN IP interface, but a CPU port interface
            continue;
        }

        new_up = vlan_up[vid];

        if (IP_vlan_up[vid] == new_up) {
            // No change. Continue
            continue;
        }

        T_I("VLAN %u state change to %s", vid, new_up ? "UP" : "DOWN");

        if ((rc = ip_os_if_ctl(if_itr->first, new_up)) != VTSS_RC_OK) {
            T_E("ip_os_if_ctl(%u, %d) failed: %s", vid, new_up, error_txt(rc));
        } else if (new_up) {
#if defined(VTSS_SW_OPTION_IPV6)
            // When an interface goes down, the IPv4 address assigned to it is
            // still present after it goes down. That's not the case with IPv6
            // addresses, so when it comes back up, we need to reinstall those
            // addresses.
            if (if_itr->second.ipv6_active && (rc = ip_os_ipv6_add(if_itr->first, &if_itr->second.cur_ipv6)) != VTSS_RC_OK) {
                // Install IPv6 address when interface comes up
                T_E("ip_os_ipv6_add(%u, %s) failed: %s", vid, if_itr->second.cur_ipv6, error_txt(rc));
            }

            if (if_itr->second.dhcp6c.active && ip_os_ipv6_add(if_itr->first, &if_itr->second.dhcp6c.network) != VTSS_RC_OK) {
                // Install IPv6 address when interface comes up
                T_E("ip_os_ipv6_add(%u, %s) [DHCP] failed: %s", vid, if_itr->second.dhcp6c.network, error_txt(rc));
            }
#endif // VTSS_SW_OPTION_IPV6
        }
    }

    memcpy(IP_vlan_up, vlan_up, sizeof(IP_vlan_up));
}

/******************************************************************************/
// vtss_ip_if_address_valid()
// Used by DHCP6 client.
// This function is used to validate the input network (n) of an existing IP
// VLAN interface.
// It mainly checks network overlapping w.r.t the given interface's network in
// our system.
/******************************************************************************/
bool vtss_ip_if_address_valid(mesa_vid_t vlan, const mesa_ip_network_t *n)
{
    vtss_ifindex_t ifindex = IP_vlan_to_ifindex(vlan);

    if (n->address.type == MESA_IP_TYPE_NONE) {
        return true;
    } else {
        if (n->address.type != MESA_IP_TYPE_IPV4 && n->address.type != MESA_IP_TYPE_IPV6) {
            return false;
        }
    }

    IP_LOCK_SCOPE();

    if (ip_if_exists(ifindex) != VTSS_RC_OK) {
        return false;
    }

    return IP_if_ip_check(n, ifindex);
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// ip_dhcp6c_add()
/******************************************************************************/
mesa_rc ip_dhcp6c_add(mesa_vid_t vid, const mesa_ipv6_network_t *network, uint64_t lt_valid)
{
    return IP_if_dhcp6c_cmd(true, vid, network, lt_valid);
}
#endif // VTSS_SW_OPTION_IPV6

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// ip_dhcp6c_del()
/******************************************************************************/
mesa_rc ip_dhcp6c_del(mesa_vid_t vid, const mesa_ipv6_network_t *network)
{
    return IP_if_dhcp6c_cmd(false, vid, network, 0);
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// ip_error_txt()
/******************************************************************************/
const char *ip_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_IP_RC_INVALID_ARGUMENT:
        return "Invalid argument";

    case VTSS_APPL_IP_RC_ROUTE_DEST_CONFLICT_WITH_LOCAL_IF:
        return "Next hop address is this router's own";

    case VTSS_APPL_IP_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_IP_RC_INTERNAL_ERROR:
        return "Internal error. Check console/crashlog for details";

    case VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED:
        return "IPv6 not supported on this platform";

    case VTSS_APPL_IP_RC_ROUTING_NOT_SUPPORTED:
        return "Routing not supported on this platform";

    case VTSS_APPL_IP_RC_IF_NOT_FOUND:
        return "No such IP interface";

    case VTSS_APPL_IP_RC_OS_IF_NOT_FOUND:
        return "No such OS interface";

    case VTSS_APPL_IP_RC_IF_MTU_INVALID:
        return "Invalid MTU for IP interface";

    case VTSS_APPL_IP_RC_IF_NOT_USING_DHCP:
        return "The IP interface is not using DHCP";

    case VTSS_APPL_IP_RC_IF_DHCP_UNSUPPORTED_FLAGS:
        return "Invalid DHCP flags set in mask";

    case VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_IF_MAC:
        return "Invalid DHCP client ID interface MAC value. It must be of type port or none";

    case VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_ASCII:
        return "Invalid DHCP client ID ASCII value. It must be between 1 and 31 characters long";

    case VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_HEX:
        return "Invalid DHCP client ID Hex value. It must be between 2 and 64 characters long and of even length and only consist of hexadecimal characters";

    case VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID:
        return "Invalid DHCP client ID";

    case VTSS_APPL_IP_RC_IF_DHCP_CLIENT_HOSTNAME:
        return "Invalid DHCP client hostname. "
               "A valid name consist of a sequence of domain labels "
               "separated by \".\". Each domain label starts and ends "
               "with an alphanumeric character and possibly also "
               "contains \"-\" characters. The length of a domain label "
               "must be 63 characters or less";

    case VTSS_APPL_IP_RC_IF_DHCP_FALLBACK_TIMEOUT:
        return "Invalid fallback timeout. Valid range is 1 to 4294967295 seconds";

    case VTSS_APPL_IP_RC_IF_INVALID_NETWORK_ADDRESS:
        return "Invalid IP address/network mask";

    case VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_EXISTING:
        return "IP address conflicts with existing interface address";

    case VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_STATIC_ROUTE:
        return "IP address conflicts with existing IP address in static routing table";

    case VTSS_APPL_IP_RC_IF_ADDRESS_ALREADY_EXISTS_ON_ANOTHER_INTERFACE:
        return "IP address already exists on another interface";

    case VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN:
        return "Interface index must be of type VLAN";

    case VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN_OR_CPU:
#ifdef VTSS_SW_OPTION_CPUPORT
        return "Interface index must be of type VLAN or CPU";
#else
        return "Interface index must of type VLAN";
#endif

    case VTSS_APPL_IP_RC_IF_LIMIT_REACHED:
        return "Number of allowed IP interfaces reached";

    case VTSS_APPL_IP_RC_ROUTE_DOESNT_EXIST:
        return "Route does not exist";

    case VTSS_APPL_IP_RC_ROUTE_TYPE_INVALID:
        return "Invalid route type";

    case VTSS_APPL_IP_RC_ROUTE_PROTOCOL_INVALID:
        return "Invalid route protocol";

    case VTSS_APPL_IP_RC_ROUTE_SUBNET_MUST_BE_UNICAST:
        return "Route subnet must be unicast";

    case VTSS_APPL_IP_RC_ROUTE_SUBNET_PREFIX_SIZE:
        return "Invalid subnet prefix size";

    case VTSS_APPL_IP_RC_ROUTE_SUBNET_BITS_SET_OUTSIDE_OF_PREFIX:
        return "Route subnet has bits set outside of prefix";

    case VTSS_APPL_IP_RC_ROUTE_SUBNET_CONFLICTS_WITH_ACTIVE_IP:
        return "Route subnet conflicts with active IP address";

    case VTSS_APPL_IP_RC_ROUTE_SUBNET_NOT_ROUTABLE:
        return "Route subnet is not routable";

    case VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_ZERO:
        return "Route destination must not be all-zeros";

    case VTSS_APPL_IP_RC_ROUTE_DEST_MUST_BE_UNICAST:
        return "Route destination must be a unicast address";

    case VTSS_APPL_IP_RC_ROUTE_DEST_VLAN_INVALID:
        return "Invalid VLAN specified for link-local address";

    case VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_LOOPBACK_OR_IPV4_FORM:
        return "Route destination must not be loopback address";

    case VTSS_APPL_IP_RC_ROUTE_DISTANCE_INVALID:
        return "Invalid route distance";

    case VTSS_APPL_IP_RC_ROUTE_MAX_CNT_REACHED:
        return "Maximum number of static routes is reached";

    default:
        return "IP: \?\?\?";
    }
}

#if defined(VTSS_SW_OPTION_IPV6)
extern "C" int ipv6_icli_cmd_register();
#endif

extern "C" int ip_icli_cmd_register();

/******************************************************************************/
// vtss_ip_init()
/******************************************************************************/
mesa_rc vtss_ip_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        IP_cap_init();
        IP_if_conf_default_init();
        IP_route_conf_default_init();

        ip_dhcp6c_init();

        critd_init(&IP_crit, "ip", VTSS_MODULE_ID_IP, CRITD_TYPE_MUTEX);
        IP_crit.max_lock_time = 300;

        (void)ip_os_init();
        (void)vtss_ip_chip_init();
        vtss_flag_init(&IP_control_flags);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           IP_thread,
                           0,
                           "IP",
                           nullptr,
                           0,
                           &IP_thread_handle,
                           &IP_thread_block);

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        vtss_ip_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_ip_json_init();
#endif

        ip_icli_cmd_register();

#if defined(VTSS_SW_OPTION_IPV6)
        ipv6_icli_cmd_register();
#endif
        break;

    case INIT_CMD_START:
        ip_acd4_init();

#ifdef VTSS_SW_OPTION_SNMP
        (void)vtss_ip_snmp_init();
#endif

#if defined(VTSS_SW_OPTION_ICFG)
        mesa_rc vtss_ip_ipv4_icfg_init(void);
        VTSS_RC(vtss_ip_ipv4_icfg_init());

#if defined(VTSS_SW_OPTION_IPV6)
        mesa_rc vtss_ip_ipv6_icfg_init(void);
        VTSS_RC(vtss_ip_ipv6_icfg_init());
#endif // VTSS_SW_OPTION_IPV6

#endif // VTSS_SW_OPTION_ICFG

        (void)vtss_appl_ip_system_statistics_ipv4_clear();
        (void)vtss_appl_ip_system_statistics_ipv6_clear();

        // VLAN configuration change registration
        vlan_s_custom_etype_change_register(VTSS_MODULE_ID_IP, IP_vlan_global_conf_change_callback);
        vlan_port_conf_change_register(VTSS_MODULE_ID_IP, IP_vlan_port_conf_change_callback, FALSE);
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_GLOBAL) {
            // Reset global configuration (no local or per switch configuration
            // here)
            IP_LOCK_SCOPE();
            IP_conf_default();
        }

        // We need to update the list of vlans that the secondary devices must
        // monitor for forwarding changes.
        {
            IP_LOCK_SCOPE();
            IP_poll_fwdstate();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        IP_LOCK_SCOPE();
        (void)conf_mgmt_mac_addr_get(IP_main_mac.addr, 0);
        (void)vtss_ip_chip_get_ready(&IP_main_mac);
        vtss_flag_setbits(&IP_control_flags, IP_CTLFLAG_THREAD_WAKEUP);
        break;
    }

    case INIT_CMD_ICFG_LOADING_POST: {
        IP_LOCK_SCOPE();
        IP_poll_fwdstate();
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

