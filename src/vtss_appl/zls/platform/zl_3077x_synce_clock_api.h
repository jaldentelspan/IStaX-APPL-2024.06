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

#ifndef _ZL_30361_SYNCE_CLOCK_API_H_
#define _ZL_30361_SYNCE_CLOCK_API_H_

#include "vtss/appl/synce.h"
#include "main_types.h"
#include "synce_custom_clock_api.h"

#ifdef __cplusplus
extern "C" {
#endif

mesa_rc zl_3077x_api_init(void **device_ptr);

mesa_rc zl_3077x_clock_init(BOOL cold_init);

mesa_rc zl_3077x_clock_startup(BOOL cold_init, const uint clock_my_input_max);

mesa_rc zl_3077x_clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input);

mesa_rc zl_3077x_clock_selector_state_get(vtss_appl_synce_selector_state_t *const selector_state, uint *const clock_input);

mesa_rc zl_3077x_clock_los_get(const uint clock_input, bool *const los);

mesa_rc zl_3077x_clock_losx_state_get(bool *const state);

mesa_rc zl_3077x_clock_lol_state_get(bool *const state);

mesa_rc zl_3077x_clock_dhold_state_get(bool *const state);

void zl_3077x_clock_event_poll(BOOL interrupt,  clock_event_type_t *ev_mask, const uint clock_my_input_max);

mesa_rc zl_3077x_station_clk_out_freq_set(const u32 freq_khz);

mesa_rc zl_3077x_ref_clk_in_freq_set(const uint source, const u32 freq_khz);

mesa_rc zl_3077x_eec_option_set(const clock_eec_option_t clock_eec_option);

mesa_rc zl_3077x_selector_map_set(const uint reference, const uint clock_input);

mesa_rc zl_3077x_adjtimer_set(i64 adj);

void zl3077x_clock_take_hw_nco_control(void);

void zl3077x_clock_return_hw_nco_control(void);

mesa_rc zl_3077x_adj_phase_set(i32 adj);

mesa_rc zl3077x_clock_output_adjtimer_set(i64 adj);

mesa_rc zl3077x_clock_ptp_timer_source_set(ptp_clock_source_t source);

mesa_rc zl_3077x_clock_nco_assist_set(bool enable);

mesa_rc zl_3077x_trace_init(FILE *logFd);
mesa_rc zl_3077x_trace_level_module_set(u32 module, u32 level);
mesa_rc zl_3077x_trace_level_all_set(u32 level);

mesa_rc zl_3077x_1pps_ref_conf(int ref, bool enable);

mesa_rc zl_3077x_ref_input_freq_set(const uint32_t refId, const u32 freq_khz);
mesa_rc zl_3077x_dpll_force_lock_ref(uint32_t dpll, int ref, bool enable);
mesa_rc zl_3077x_clock_link_state_set(uint32_t clk_input, bool link);
mesa_rc zl_3077x_clock_reset_to_defaults(const uint clock_my_input_max);

#ifdef __cplusplus
}
#endif
#endif // _ZL_30361_SYNCE_CLOCK_API_H_

