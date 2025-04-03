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

#ifdef VTSS_SW_OPTION_ZLS3073X

#include "synce_dpll_zl3073x.h"
#include "synce_custom_clock_api.h"
#include "zl_3073x_synce_clock_api.h"
#include "zl_3073x_api_api.h"
#include "synce_dpll_trace.h"
#include "zl303xx_DeviceSpec.h"                         // This file is included to get the definition of zl303xx_ParamsS
#include "../zl30772_fw_update/zl303xx_Dpll77xFlash.h"  // for flashing of new firmware within DPLL (notice: dependant on zl303xx_DeviceSpec.h)
#include "synce_spi_if.h"                               // For vtss::synce::dpll:clock_chip_spi_if.spi_transfer()
#include <sys/stat.h>
#include <sys/types.h>

#define Zl3073x_FIRMWARE_UPDATE_UTIL_FILE          "/etc/mscc/dpll/firmware/zl30732.utility.hex"
#define Zl3073x_FIRMWARE_UPDATE_FILE1              "/etc/mscc/dpll/firmware/zl30732.firmware1.hex"
#define Zl3073x_FIRMWARE_UPDATE_FILE2              "/etc/mscc/dpll/firmware/zl30732.firmware2.hex"
#define Zl3073x_FIRMWARE_STATUS_DIR                "/switch/dpll.zl30732/" // Limit it to one directory level to avoid problems with mkdir()
#define Zl3073x_FIRMWARE_FORCE_UPDATE_FILE         Zl3073x_FIRMWARE_STATUS_DIR "dpll_force_update"
#define Zl3073x_FIRMWARE_UPDATE_COUNT_FILE         Zl3073x_FIRMWARE_STATUS_DIR "dpll_update_count"
#define Zl3073x_FIRMWARE_UPDATE_ATTEMPT_COUNT_FILE Zl3073x_FIRMWARE_STATUS_DIR "dpll_update_attempt_count"

#define CRIT_ENTER() critd_enter(&vtss::synce::dpll::crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &vtss::synce::dpll::crit, __FILE__, __LINE__)

mesa_rc synce_dpll_zl3073x::clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_clock_selector_state_get(selector_state, clock_input);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input)
{
    if (clock_input >= clock_my_input_max) {
        return MESA_RC_ERROR;
    }

    CRIT_ENTER();
    clock_selection_mode = mode;
    mesa_rc rc = zl_3073x_clock_selection_mode_set(mode, clock_input);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_locs_state_get(const uint clock_input, bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_clock_los_get(clock_input, state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_losx_state_get(bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_clock_losx_state_get(state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_lol_state_get(bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_clock_lol_state_get(state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_dhold_state_get(bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_clock_dhold_state_get(state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_event_poll(bool interrupt, clock_event_type_t *ev_mask)
{
    CRIT_ENTER();
    zl_3073x_clock_event_poll(interrupt, ev_mask, clock_my_input_max);
    CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc synce_dpll_zl3073x::clock_station_clk_out_freq_set(const u32 freq_khz)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_station_clk_out_freq_set(freq_khz);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_ref_clk_in_freq_set(source, freq_khz);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_eec_option_set(clock_eec_option);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_features_get(sync_clock_feature_t *features)
{
    *features = SYNCE_CLOCK_FEATURE_DUAL_INDEPENDENT;

    return VTSS_RC_OK;
}

mesa_rc synce_dpll_zl3073x::clock_adj_phase_set(i32 adj)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_adj_phase_set(adj);
    T_D("adjust PTP DPLL phase %d, currently not implemented", adj);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_adjtimer_set(i64 adj)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3073x_adjtimer_set(adj);
    T_D("adjust Synce DPLL " VPRI64d, adj);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_adjtimer_enable(bool enable)
{
    mesa_rc rc = VTSS_RC_OK;

    CRIT_ENTER();
    if (enable) {
        zl3073x_clock_take_hw_nco_control();
    } else {
        zl3073x_clock_return_hw_nco_control();
    }
    T_D("adjust Synce DPLL enable %d ", enable);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_selector_map_set(const uint reference, const uint clock_input)
{
    CRIT_ENTER();
    /* set up the reference mapping in the DPLL */
    mesa_rc rc = zl_3073x_selector_map_set(reference, clock_input);
    CRIT_EXIT();

    return rc;
}

/*lint -sem(clock_startup,   thread_protected) ... We're protected */
mesa_rc synce_dpll_zl3073x::clock_startup(bool cold_init, bool pcb104_synce)
{
    /* Do zl30363 startup */
    return zl_3073x_clock_startup(cold_init, clock_my_input_max);
}

mesa_rc synce_dpll_zl3073x::clock_init(bool cold_init, void **device_ptr)
{
    mesa_rc rc;

    clock_my_input_max = CLOCK_INPUT_MAX;
    synce_my_prio_disabled = 0x0f;

    rc = zl_3073x_api_init(device_ptr);

    /* Do ZL30363 initialization */
    if (rc == VTSS_RC_OK) {
        rc = zl_3073x_clock_init(cold_init);
    }

    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_output_adjtimer_set(i64 adj)
{
    CRIT_ENTER();
    mesa_rc rc = zl3073x_clock_output_adjtimer_set(adj);
    T_I("adjust PTP DPLL " VPRI64d, adj);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    CRIT_ENTER();
    mesa_rc rc = zl3073x_clock_ptp_timer_source_set(source);
//    T_W("Select PTP source %d", (int) source);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3073x::clock_nco_assist_set(bool enable)
{
    CRIT_ENTER();
    T_I("Hybrid mode %s", enable ? "enabled" : "disabled");
    zl_3073x_clock_nco_assist_set(enable);
    CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc synce_dpll_zl3073x::trace_init(FILE *logFd)
{
    return zl_3073x_trace_init(logFd);
}

mesa_rc synce_dpll_zl3073x::trace_level_module_set(u32 module, u32 level)
{
    return zl_3073x_trace_level_module_set(module, level);
}

mesa_rc synce_dpll_zl3073x::trace_level_all_set(u32 level)
{
    return zl_3073x_trace_level_all_set(level);
}

#endif // VTSS_SW_OPTION_ZLS3073x

