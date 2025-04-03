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

#ifndef _IP_BASE_HXX_
#define _IP_BASE_HXX_

#include "vtss_intrusive_list.h"
#include "ip_api.h"
#include "vtss_timer_api.h"
#include <vtss/appl/interface.h>
#include <vtss/appl/ip.h>
#include <vtss/basics/map.hxx>
#include <vtss/basics/notifications/timer.hxx>

typedef struct IP_if_dhcp4c_fallback_timer_t_ {
    VTSS_LINKED_LIST_DECL_ITEM(IP_if_dhcp4c_fallback_timer_t_);
    vtss_tick_count_t timeout;
    vtss_ifindex_t    ifidx;
} ip_if_dhcp4c_fallback_timer_t;

typedef enum {
    IP_ACD4_SM_STATE_IDLE,    // No IP address setup
    IP_ACD4_SM_STATE_PROBING, // Probing for conflicting IP
    IP_ACD4_SM_STATE_BOUND    // IP address is bound
} ip_acd4_sm_state_t;

typedef struct {
    vtss::Timer         timer;
    uint32_t            count;
    mesa_ipv4_network_t net;
    ip_acd4_sm_state_t  sm_state;
} ip_acd4_state_t;

typedef enum {
    VIF_FWD_UNDEFINED,  // Initial state
    VIF_FWD_FORWARDING, // At least one port on VIF is forwarding
    VIF_FWD_BLOCKING,   // All ports on VIF are blocking
} ip_vif_fwd_t;

typedef struct {
    mesa_vid_t                  vlan;               /**< The VLAN ID this instance represents. 0 if a cpuport */
    mesa_mac_t                  mac_address;        /**< Interface MAC address */

    bool                        dhcp_v4_valid;      /**< dhcp_v4_address valid */
    mesa_ipv4_t                 dhcp_v4_gw;         /**< dhcp default gw */

    bool                        ipv4_active;
    mesa_ipv4_network_t         cur_ipv4;           /**< Current IPv4 address */

    bool                        ipv6_active;
    mesa_ipv6_network_t         cur_ipv6;           /**< Current IPv6 address */

    struct {
        bool                     active;
        mesa_ipv6_network_t      network;           /**< Current IPv6 address */
        uint64_t                 lt_valid;          /**< Valid lifetime       */
    } dhcp6c;

    // Point in time when DHCPC started
    ip_if_dhcp4c_fallback_timer_t dhcp4c_start_timer;

    // Configuration data
    struct {
        vtss_appl_ip_if_conf_t      conf;
        vtss_appl_ip_if_conf_ipv4_t ipv4;
        vtss_appl_ip_if_conf_ipv6_t ipv6;
    } if_config;

    // Address Conflict Detection (RFC5227)
    ip_acd4_state_t acd_state;

    // Per unit VID forward state
    ip_vif_fwd_t unit_fwd;

    // Combined state
    ip_vif_fwd_t combined_fwd;
} ip_if_state_t;

typedef vtss::Map<vtss_ifindex_t, ip_if_state_t> ip_if_state_map_t;
typedef ip_if_state_map_t::iterator ip_if_itr_t;

mesa_rc ip_if_exists(vtss_ifindex_t ifindex, ip_if_itr_t *if_itr = nullptr);
void ip_if_signal(vtss_ifindex_t ifindex);

// How often to poll IP statistics
#define IP_STATISTICS_REFRESH_RATE_MSECS 3000

/******************************************************************************/
// IpIfStatistics class.
// Can poll statistics per ifindex (currently only VLAN) every so many ticks.
/******************************************************************************/
template<typename T>
struct IpIfStatistics {
    typedef mesa_rc (*poll_t)(mesa_vid_t, T *);

    IpIfStatistics(poll_t p, vtss_tick_count_t r = 0)
    {
        poll = p;
        refresh_rate = r;
    }

    mesa_rc get(mesa_vid_t vid, T *s)
    {
        vtss_tick_count_t ts = vtss_current_time();

        if (cache_ts[vid] == 0 || refresh_rate == 0 || ts > cache_ts[vid] + refresh_rate) {
            VTSS_RC(poll(vid, &cache_value[vid]));
            cache_ts[vid] = ts;
        }

        *s = (cache_value[vid] - reset_value[vid]);
        return VTSS_RC_OK;
    }

    mesa_rc reset(mesa_vid_t vid)
    {
        VTSS_RC(poll(vid, &cache_value[vid]));
        reset_value[vid] = cache_value[vid];
        return VTSS_RC_OK;
    }

    poll_t poll;
    vtss_tick_count_t cache_ts[VTSS_VIDS];
    vtss_tick_count_t refresh_rate;
    T cache_value[VTSS_VIDS];
    T reset_value[VTSS_VIDS];
};

/******************************************************************************/
// IpSystemStatistics class.
// Can poll overall system statistics every so many ticks.
/******************************************************************************/
template<typename T>
struct IpSystemStatistics {
    typedef mesa_rc (*poll_t)(T *);

    IpSystemStatistics(poll_t p, vtss_tick_count_t r = 0)
    {
        poll = p;
        refresh_rate = r;
    }

    mesa_rc get(T *s)
    {
        vtss_tick_count_t ts = vtss_current_time();

        if (cache_ts == 0 || refresh_rate == 0 || ts > cache_ts + refresh_rate) {
            VTSS_RC(poll(&cache_value));
            cache_ts = ts;
        }

        *s = cache_value - reset_value;
        return VTSS_RC_OK;
    }

    mesa_rc reset()
    {
        VTSS_RC(poll(&cache_value));
        reset_value = cache_value;
        return VTSS_RC_OK;
    }

    poll_t poll;
    vtss_tick_count_t cache_ts;
    vtss_tick_count_t refresh_rate;
    T cache_value;
    T reset_value;
};

#endif // _IP_BASE_HXX_

