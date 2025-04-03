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

/**
 * \file
 * \brief TOD debug icli functions
 * \details This header file describes ptp control functions
 */

#ifndef TOD_ICLI_FUNCTIONS_H
#define TOD_ICLI_FUNCTIONS_H

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL tod_icli_runtime_phy_timestamping_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

icli_rc_t tod_icli_debug_phy_ts(i32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *v_port_type_list);
icli_rc_t tod_icli_debug_tod_monitor(i32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *v_port_type_list);
icli_rc_t tod_icli_debug_status_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *v_port_type_list);


#ifdef __cplusplus
}
#endif

#endif /* TOD_ICLI_FUNCTIONS_H */

