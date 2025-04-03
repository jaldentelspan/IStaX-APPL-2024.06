/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef ZL_3077X_API_ICLI_FUNCTIONS_H
#define ZL_3077X_API_ICLI_FUNCTIONS_H

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

icli_rc_t zl_3077x_api_icli_debug_read(i32 session_id, u32 mem_page, u32 mem_addr, u32 mem_size);
icli_rc_t zl_3077x_api_icli_debug_write(i32 session_id, u32 mem_page, u32 mem_addr, u32 value);
icli_rc_t zl_3077x_api_icli_debug_pll_status(i32 session_id);
icli_rc_t zl_3077x_api_icli_debug_ref_status(i32 session_id, u32 refid);
icli_rc_t zl_3077x_api_icli_debug_ref_cfg(i32 session_id, u32 refid);
icli_rc_t zl_3077x_api_icli_debug_dpll_status(i32 session_id, u32 pllid);
icli_rc_t zl_3077x_api_icli_debug_dpll_cfg(i32 session_id, u32 pllid);
icli_rc_t zl_3077x_api_icli_debug_dco_freq_read(i32 session_id);
icli_rc_t zl_3077x_api_icli_debug_trace_level_module_set(i32 session_id, u32 v_uint_1, u32 v_uint_2);
icli_rc_t zl_3077x_api_icli_debug_trace_level_all_set(i32 session_id, u32 v_uint_1);

icli_rc_t zl_303xx_icli_spi_read(i32 session_id, u8 page, u16 addr, u8 cnt);
icli_rc_t zl_303xx_icli_spi_write(i32 session_id, u8 page, u16 addr, u8 data);
icli_rc_t zl_3077x_api_icli_print_mailbox_cfg(i32 session_id, bool is_ref, u8 id);
icli_rc_t zl_3077x_api_icli_debug_trace_level_module_set(i32 session_id, u32 module, u32 level);
icli_rc_t zl_3077x_icli_regdump(i32 session_id);
void zl_3077x_icli_pps_ref_conf(i32 session_id, int ref, bool enable);
icli_rc_t zl_3077x_icli_ref_freq_update(i32 session_id, uint8_t ref, uint32_t freq);
void zl_3077x_icli_force_lock_ref(i32 session_id, uint32_t dpll, int ref, bool enable);
#ifdef VTSS_SW_OPTION_ZLS30387
icli_rc_t zl_30380_icli_apr_dbg_cmds(i32 session_id, u16 cguId, u16 servId, u16 cmd, int val);
#endif
#ifdef __cplusplus
}
#endif

#endif /* ZL_3077X_API_ICLI_FUNCTIONS_H */

