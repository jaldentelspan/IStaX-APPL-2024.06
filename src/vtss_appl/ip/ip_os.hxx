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

#ifndef _IP_OS_HXX_
#define _IP_OS_HXX_

#include "ip_api.h"
#include <vtss/basics/vector.hxx>
#include <linux/if.h> /* For IFNAMSIZ (defined as 16 including terminating '\0') */

typedef struct {
    bool link;
    bool mac_addr_list;
    bool ipv4_addr;

#if defined(VTSS_SW_OPTION_IPV6)
    bool ipv6_addr;
#endif

#if defined(VTSS_SW_OPTION_L3RT)
    bool ipv4_route;
#endif

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_SW_OPTION_IPV6)
    bool ipv6_route;
#endif

    bool ipv4_neighbor;

#if defined(VTSS_SW_OPTION_IPV6)
    bool ipv6_neighbor;
#endif
} ip_os_netlink_poll_t;

void ip_os_debug_netlink_poll(ip_os_netlink_poll_t *poll);

vtss_ifindex_t ip_os_ifindex_to_ifindex(int32_t os_ifindex); // returns VTSS_IFINDEX_NONE on error
int32_t ip_os_ifindex_from_ifindex(vtss_ifindex_t ifindex);  // returns < 0 on error

mesa_rc ip_os_global_conf_set(const vtss_appl_ip_global_conf_t *conf);

mesa_rc ip_os_if_add(vtss_ifindex_t ifidx);
mesa_rc ip_os_if_add_chip_port(uint32_t chip_port); // Debug function
mesa_rc ip_os_if_set(vtss_ifindex_t ifidx, vtss_appl_ip_if_conf_t *conf);
mesa_rc ip_os_if_ctl(vtss_ifindex_t ifidx, bool up);
mesa_rc ip_os_if_del(vtss_ifindex_t ifidx);
mesa_rc ip_os_neighbor_clear(mesa_ip_type_t type);

mesa_rc ip_os_ipv4_add(vtss_ifindex_t ifidx, const mesa_ipv4_network_t *network);
mesa_rc ip_os_ipv4_del(vtss_ifindex_t ifidx, const mesa_ipv4_network_t *network);

#if defined(VTSS_SW_OPTION_IPV6)
mesa_rc ip_os_ipv6_add(vtss_ifindex_t ifidx, const mesa_ipv6_network_t *network);
mesa_rc ip_os_ipv6_del(vtss_ifindex_t ifidx, const mesa_ipv6_network_t *network, bool if_up);
#endif // VTSS_SW_OPTION_IPV6

mesa_rc ip_os_system_statistics_ipv4_get(             vtss_appl_ip_statistics_t    *statistics);
mesa_rc ip_os_system_statistics_ipv6_get(             vtss_appl_ip_statistics_t    *statistics);
mesa_rc ip_os_if_statistics_link_get(mesa_vid_t vlan, vtss_appl_ip_if_statistics_t *statistics);
mesa_rc ip_os_if_statistics_ipv6_get(mesa_vid_t vlan, vtss_appl_ip_statistics_t    *statistics);

mesa_rc ip_os_init(void);

#endif /* _IP_OS_HXX_ */

