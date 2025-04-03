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

/**
 * \file
 * \brief MVRP icli functions
 * \details This header file describes MVRP control functions
 */

#ifndef VTSS_ICLI_MVRP_H
#define VTSS_ICLI_MVRP_H

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL mrp_icli_runtime_mvrp(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

#ifdef VTSS_SW_OPTION_MVRP
mesa_rc mvrp_icli_global_set(const i32 session_id, BOOL enable);
mesa_rc mvrp_icli_port_set(const i32 session_id, icli_stack_port_range_t *plist, BOOL enable);

mesa_rc mrp_icli_timers_set(const i32 session_id, icli_stack_port_range_t *plist,
                            BOOL has_join_time, u32 join_time,
                            BOOL has_leave_time, u32 leave_time,
                            BOOL has_leave_all_time, u32 leave_all_time);
mesa_rc mrp_icli_timers_def(const i32 session_id, icli_stack_port_range_t *plist);

mesa_rc mrp_icli_periodic_set(const i32 session_id, icli_stack_port_range_t *plist,
                              bool state);

mesa_rc mvrp_icli_vlans_set(const i32 session_id, icli_unsigned_range_t *vlist,
                            BOOL has_default, BOOL has_all, BOOL has_none,
                            BOOL has_add, BOOL has_remove, BOOL has_except);

mesa_rc mrp_icli_show_status(const i32 session_id, icli_stack_port_range_t *plist,
                             BOOL has_mvrp);

mesa_rc mvrp_icli_debug_state_machines(const i32 session_id, icli_stack_port_range_t *plist, icli_unsigned_range_t *vlist);
mesa_rc mvrp_icli_debug_msti_connected_ring(const i32 session_id, BOOL has_msti, u8 msti);
#endif

#ifdef __cplusplus
}
#endif
#endif /* VTSS_ICLI_MVRP_H */

