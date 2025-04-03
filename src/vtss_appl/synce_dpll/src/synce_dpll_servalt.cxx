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

#include "synce_dpll_servalt.h"
#include "synce_custom_clock_api.h"
#include "synce_omega_clock_api.h"
#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl303xx_DeviceSpec.h"    // This file is included to get the definition of zl303xx_ParamsS
#endif

#include "synce_dpll_trace.h"

#define CRIT_ENTER() critd_enter(&vtss::synce::dpll::crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &vtss::synce::dpll::crit, __FILE__, __LINE__)

/*
 * This function implements the automatic mode if holdoff is configured.
 * I.e. When LOS is detected in automatic mode the switchover to an other source is postponed until the holdoff timer expires
 * pseudo code:
 * if automatic mode {
 *   if (los && holdoff timer active) keep selected source.
 *   if (los && holdof timer expired) select the best clock without active LOS.
 *   if new selected is not configured for holdoff, then enter automatic mode in hw
 * }
 */
void synce_dpll_servalt::control_selector(void)
{
    bool found, los = FALSE;
    u32 i, best_clock;

    T_I("clock_selection_mode %d", clock_selection_mode);
    if ((clock_selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE) || (clock_selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE)) {
        best_clock = 0;
        found = false;

        for (i = 0; i < clock_my_input_max; ++i) {
            SYNCE_CUST_RC(vtss_omega_clock_locs_state_get(i, &los));
            if (!los) { /* check for better priority on clock without active LOS */
                found = true;
                best_clock = i;
            }
        }

        T_D("found %d, best_clock %d", found, best_clock);
        if (found) { /* Best clock found */
            vtss_appl_synce_selector_state_t selector_state;
            uint clock_input;
            SYNCE_CUST_RC(vtss_omega_clock_selector_state_get(&clock_input, &selector_state));
            T_I("selector state %d, clock_input %d", selector_state, clock_input);
            SYNCE_CUST_RC(vtss_omega_clock_selection_mode_set(clock_selection_mode, best_clock, clock_my_input_max));
            T_I("Set selection mode to %d, best_clock %d", clock_selection_mode, best_clock);
        } else {
            SYNCE_CUST_RC(vtss_omega_clock_selection_mode_set(clock_selection_mode, best_clock, clock_my_input_max));
            T_I("Set selection mode to %d, best_clock %d", clock_selection_mode, best_clock);
        }
    }
}

mesa_rc synce_dpll_servalt::clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state)
{
    *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
    *clock_input = 0;

    return vtss_omega_clock_selector_state_get(clock_input, selector_state);
}

mesa_rc synce_dpll_servalt::clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input)
{
    mesa_rc rc = MESA_RC_OK;
    if (clock_input >= clock_my_input_max) {
        return MESA_RC_ERROR;
    }

    CRIT_ENTER();
    clock_selection_mode = mode;
    rc = vtss_omega_clock_selection_mode_set(mode, clock_input, clock_my_input_max);
    if (mode == VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE || mode == VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE) {
        control_selector();
    }
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_servalt::clock_locs_state_get(const uint clock_input, bool *const state)
{
    *state = false;

    return vtss_omega_clock_locs_state_get(clock_input, state);
}

mesa_rc synce_dpll_servalt::clock_losx_state_get(bool *const state)
{
    *state = false;

    return vtss_omega_clock_losx_state_get(state);
}

mesa_rc synce_dpll_servalt::clock_lol_state_get(bool *const state)
{
    *state = false;

    return vtss_omega_clock_lol_state_get(state);
}

mesa_rc synce_dpll_servalt::clock_dhold_state_get(bool *const state)
{
    *state = false;

    return vtss_omega_clock_dhold_state_get(state);
}

mesa_rc synce_dpll_servalt::clock_event_poll(bool interrupt, clock_event_type_t *ev_mask)
{
    return MESA_RC_ERROR;
}

mesa_rc synce_dpll_servalt::clock_station_clk_out_freq_set(const u32 freq_khz)
{
    return vtss_omega_clock_station_clk_out_freq_set(freq_khz);
}

mesa_rc synce_dpll_servalt::clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    meba_synce_clock_frequency_t frequency = MEBA_SYNCE_CLOCK_FREQ_UNKNOWN;

    switch (freq_khz) {
    case 1544:
        /* initialize reference for 1.544 MHZ input */
        frequency = MEBA_SYNCE_CLOCK_FREQ_1544_KHZ;
        break;
    case 2048:
        /* initialize reference for 2.048 MHZ input */
        frequency = MEBA_SYNCE_CLOCK_FREQ_2048_KHZ;
        break;
    case 10000:
        /* initialize reference for 10 MHZ input */
        frequency = MEBA_SYNCE_CLOCK_FREQ_10MHZ;
        break;
    case 25000:
        /* initialize reference for 25 MHZ input */
        frequency = MEBA_SYNCE_CLOCK_FREQ_25MHZ;
        break;
    case 80566:
        /* initialize reference for 80.565 MHZ input (actually 80.56640625 to be precise) */
        frequency = MEBA_SYNCE_CLOCK_FREQ_80_565MHZ;
        break;
    case 125000:
        /* initialize reference for 125 MHZ input */
        frequency = MEBA_SYNCE_CLOCK_FREQ_125MHZ;
        break;
    case 156250:
        /* initialize reference for 156.25 MHZ input */
        frequency = MEBA_SYNCE_CLOCK_FREQ_156_25MHZ;
        break;
    case 161130:
        /* initialize reference for 161.13 MHZ input (actually 161.1328125 to be precise) */
        frequency = MEBA_SYNCE_CLOCK_FREQ_161_13MHZ;
        break;
    case 32226:
        /* initialize reference for 32.226 MHZ input (actually 32.2265625 to be precise) */
        frequency = MEBA_SYNCE_CLOCK_FREQ_32_226MHZ;
        break;
    default:
        /* The reference can not be disabled, so we leave it as it is */
        frequency = MEBA_SYNCE_CLOCK_FREQ_INVALID;
        T_I("clock input: disable ?");
        return MESA_RC_ERROR;
    }

    CRIT_ENTER();
    mesa_rc rc = vtss_omega_clock_frequency_set(source, frequency, clock_my_input_max);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_servalt::clock_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    return vtss_omega_clock_eec_option_set(clock_eec_option);
}

mesa_rc synce_dpll_servalt::clock_eec_option_type_get(uint *const eec_type)
{
    return vtss_omega_clock_eec_option_type_get(eec_type);
}

mesa_rc synce_dpll_servalt::clock_adjtimer_set(i64 adj)
{
    return vtss_omega_clock_adjtimer_set(adj);

}

mesa_rc synce_dpll_servalt::clock_adjtimer_enable(bool enable)
{
    return vtss_omega_clock_adjtimer_enable(enable);

}

mesa_rc synce_dpll_servalt::clock_selector_map_set(const uint reference, const uint clock_input)
{
    return vtss_omega_clock_selector_map_set(reference, clock_input);
}

mesa_rc synce_dpll_servalt::clock_ho_frequency_offset_get(i64 *const offset)
{
    return vtss_omega_clock_ho_frequency_offset_get(offset);
}

/*lint -sem(clock_startup,   thread_protected) ... We're protected */
mesa_rc synce_dpll_servalt::clock_startup(bool cold_init, bool pcb104_synce)
{
    vtss_omega_clock_startup(cold_init, pcb104_synce);

    return MESA_RC_OK;
}

mesa_rc synce_dpll_servalt::clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency)
{
    if (clock_input >= clock_my_input_max) {
        return MESA_RC_ERROR;
    }

    return vtss_omega_clock_frequency_set(clock_input, frequency, clock_my_input_max);
}

mesa_rc synce_dpll_servalt::clock_init(bool cold_init, void **device_ptr)
{
    mesa_rc rc = MESA_RC_ERROR;

    clock_my_input_max = CLOCK_INPUT_MAX - 1;
    synce_my_prio_disabled = CLOCK_INPUT_MAX - 1;

    if (fast_cap(MESA_CAP_CLOCK)) {
        rc = vtss_omega_clock_init(cold_init); /* be prepared for Omega DPLL either internal (SERVAL_T) or external (ROLEX) */
    }

#if defined(VTSS_SW_OPTION_ZLS30387)
    *device_ptr = malloc(sizeof(zl303xx_ParamsS));
    if (*device_ptr == NULL) {
        return MESA_RC_ERROR;
    }
//    ((zl303xx_ParamsS *)*device_ptr)->deviceType = CUSTOM_DEVICETYPE;
#else
    *device_ptr = NULL;  // If no variant of the MS-PDV is compiled in, no memory shall be allocated for the zl303xx_ParamsS structure as that would then cause a memory-leak.
#endif

    return rc;
}

mesa_rc synce_dpll_servalt::clock_station_clock_type_get(uint *const clock_type)
{
    return vtss_omega_clock_station_clock_type_get(clock_type);
}

mesa_rc synce_dpll_servalt::clock_features_get(sync_clock_feature_t *features)
{
    uint omega_features;
    mesa_rc rc = vtss_omega_clock_features_get(&omega_features);
    switch (omega_features) {
    case 1:
        *features = SYNCE_CLOCK_FEATURE_SINGLE;
        break;
    case 2:
        *features = SYNCE_CLOCK_FEATURE_DUAL;
        break;
    default:
        *features = SYNCE_CLOCK_FEATURE_NONE;
        break;
    }
    return rc;
}

mesa_rc synce_dpll_servalt::clock_adj_phase_set(i32 adj)
{
    return vtss_omega_clock_adj_phase_set(adj);
}

mesa_rc synce_dpll_servalt::clock_output_adjtimer_set(i64 adj)
{
    return vtss_omega_clock_output_adjtimer_set(adj);
}

mesa_rc synce_dpll_servalt::clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    return vtss_omega_clock_ptp_timer_source_set(source);
}

mesa_rc synce_dpll_servalt::clock_read(const uint reg, uint *const value)
{
    return vtss_omega_clock_read(reg, value);
}

mesa_rc synce_dpll_servalt::clock_write(const uint reg, const uint value)
{
    return vtss_omega_clock_write(reg, value);
}

mesa_rc synce_dpll_servalt::clock_shutdown(void)
{
    //return vtss_omega_clock_shutdown();
    // this feature is not needed any more
    return MESA_RC_OK;
}

