/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __XXRP_TYPES_H__
#define __XXRP_TYPES_H__

#include <icli_api.h>

#ifdef __cplusplus
extern "C" {
#endif
mesa_rc gvrp_global_enable(int enable, int mac_vlans);
mesa_rc gvrp_max_vlans_set(int max_vlans);
int gvrp_max_vlans_get(void);
int gvrp_enable_get(void);

void gvrp_protocol_state(u32 session_id, icli_stack_port_range_t *v_port_type_list, icli_unsigned_range_t *v_vlan_list);

void gvrp_join_request(icli_stack_port_range_t *plist, icli_unsigned_range_t *v_vlan_list);
void gvrp_leave_request(icli_stack_port_range_t *plist, icli_unsigned_range_t *v_vlan_list);

void gvrp_port_enable(icli_stack_port_range_t *plist, int enable);
mesa_rc gvrp_icli_debug_global_print(const i32 session_id);
mesa_rc gvrp_icli_debug_msti(const i32 session_id);
mesa_rc gvrp_icli_debug_internal_statistic(const i32 session_id);
mesa_rc gvrp_icli_timer_conf_set(const i32 session_id, bool has_join_time, u32 join_time,
                                 bool has_leave_time, u32 leave_time,
                                 bool has_leave_all_time, u32 leave_all_time);
mesa_rc gvrp_icli_timer_conf_def(const i32 session_id);

#ifdef __cplusplus
}
#endif
#endif
