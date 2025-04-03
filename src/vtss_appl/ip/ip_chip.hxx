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

#ifndef _IP_CHIP_HXX_
#define _IP_CHIP_HXX_

#include "main_types.h"
#include <vtss/appl/ip.h>

mesa_rc vtss_ip_chip_init(void);
mesa_rc vtss_ip_chip_get_ready(const mesa_mac_t *mac);
mesa_rc vtss_ip_chip_mac_subscribe(  mesa_vid_t vlan, const mesa_mac_t *mac);
mesa_rc vtss_ip_chip_mac_unsubscribe(mesa_vid_t vlan, const mesa_mac_t *mac);
mesa_rc vtss_ip_chip_routing_enable(bool enable);
mesa_rc vtss_ip_chip_rleg_add(mesa_vid_t vlan);
mesa_rc vtss_ip_chip_rleg_del(mesa_vid_t vlan);
mesa_rc vtss_ip_chip_route_add(const mesa_routing_entry_t *rt);
mesa_rc vtss_ip_chip_route_del(const mesa_routing_entry_t *rt);
mesa_rc vtss_ip_chip_route_bulk_add(uint32_t cnt, const mesa_routing_entry_t *rt, uint32_t *cnt_out);
mesa_rc vtss_ip_chip_route_bulk_del(uint32_t cnt, const mesa_routing_entry_t *rt, uint32_t *cnt_out);
mesa_rc vtss_ip_chip_neighbor_add(const vtss_appl_ip_neighbor_key_t &k, const vtss_appl_ip_neighbor_status_t &v);
mesa_rc vtss_ip_chip_neighbor_del(const vtss_appl_ip_neighbor_key_t &k, const vtss_appl_ip_neighbor_status_t &v);
mesa_rc vtss_ip_chip_counters_vlan_get(mesa_vid_t vlan, mesa_l3_counters_t *counters);
mesa_rc vtss_ip_chip_counters_vlan_clear(mesa_vid_t vlan);
mesa_rc vtss_ip_chip_counters_system_get(mesa_l3_counters_t *counters);

#endif /* _IP_CHIP_HXX_ */

