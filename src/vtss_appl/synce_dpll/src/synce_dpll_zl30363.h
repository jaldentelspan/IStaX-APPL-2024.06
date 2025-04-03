/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#if !defined(_SYNCE_DPLL_ZL30363_H)
#define _SYNCE_DPLL_ZL30363_H

#include "synce_dpll_zl303xx.h"

struct synce_dpll_zl30363 : synce_dpll_zl303xx {
    virtual mesa_rc clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state);
    virtual mesa_rc clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input);
    virtual mesa_rc clock_locs_state_get(const uint clock_input, bool *const state);
    virtual mesa_rc clock_losx_state_get(bool *const state);
    virtual mesa_rc clock_lol_state_get(bool *const state);
    virtual mesa_rc clock_dhold_state_get(bool *const state);
    virtual mesa_rc clock_event_poll(bool interrupt, clock_event_type_t *ev_mask);
    virtual mesa_rc clock_station_clk_out_freq_set(const u32 freq_khz);
    virtual mesa_rc clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz);
    virtual mesa_rc clock_eec_option_set(const clock_eec_option_t clock_eec_option);
    virtual mesa_rc clock_features_get(sync_clock_feature_t *features);
    virtual mesa_rc clock_adj_phase_set(i32 adj);
    virtual mesa_rc clock_adjtimer_set(i64 adj);
    virtual mesa_rc clock_adjtimer_enable(bool enable);
    virtual mesa_rc clock_selector_map_set(const uint reference, const uint clock_input);
    virtual mesa_rc clock_startup(bool cold_init, bool pcb104_synce);
    virtual mesa_rc clock_init(bool cold_init, void **device_ptr);
    virtual mesa_rc clock_output_adjtimer_set(i64 adj);
    virtual mesa_rc clock_ptp_timer_source_set(ptp_clock_source_t source);
    static mesa_rc trace_init(FILE *logFd);
    static mesa_rc trace_level_module_set(u32 module, u32 level);
    static mesa_rc trace_level_all_set(u32 level);
    virtual vtss_zl_30380_dpll_type_t dpll_type()
    {
        return VTSS_ZL_30380_DPLL_ZLS3036X;
    }
};

#endif // _SYNCE_DPLL_ZL30363_H
