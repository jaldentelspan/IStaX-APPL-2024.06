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

#if !defined(_SYNCE_DPLL_BASE_H)
#define _SYNCE_DPLL_BASE_H

#include <main_types.h>
#include <vtss/appl/synce.h>
#include "synce_constants.h"
#include "synce_types.h"
#include "synce_custom_clock_api.h"
#include "critd_api.h"
#include "lock.hxx"

#include "synce_spi_if.h" // Note: Must be included after the trace definitions as it depends on these.

#define SYNCE_CUST_RC(expr) { mesa_rc _rc_ = (expr); if (_rc_ < VTSS_RC_OK) { \
T_I("Error code: %x", _rc_); }}

namespace vtss
{
namespace synce
{
namespace dpll
{
extern critd_t crit;
extern bool pcb104;
extern bool si5328;
extern bool pcb104_synce;
} // namespace dpll
} // namespace synce
} // namespace vtss

struct synce_dpll_base {
    meba_synce_clock_hw_id_t clock_hw_id;

    vtss_appl_synce_selection_mode_t clock_selection_mode;
    uint clock_my_input_max;  /* actual number of clock sources may be less than the defined CLOCK_INPUT_MAX */
    uint synce_my_prio_disabled;

    /* functions for 'clock source nomination and state' (per clock) */
    /* get LOCS */
    virtual mesa_rc clock_locs_state_get(const uint clock_input, bool *const state) = 0;

    /* functions for 'clock selection mode and state' */
    /* get/set field 'mode' */
    virtual mesa_rc clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input) = 0;
    virtual mesa_rc clock_selection_mode_get(vtss_appl_synce_selection_mode_t *const mode);
    /* get/set field 'eec option' */
    virtual mesa_rc clock_eec_option_set(const clock_eec_option_t clock_eec_option) = 0;
    virtual mesa_rc clock_eec_option_type_get(uint *const eec_type) = 0;
    /* get field 'state' */
    virtual mesa_rc clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state) = 0;
    /* get field 'lol' */
    virtual mesa_rc clock_lol_state_get(bool *const state) = 0;
    /* get field 'dhold' */
    virtual mesa_rc clock_dhold_state_get(bool *const state) = 0;

    /* functions for 'station clock configuration and clock hardware' */
    virtual mesa_rc clock_station_clk_out_freq_set(const u32 freq_khz) = 0;
    virtual mesa_rc clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz);
    virtual mesa_rc clock_station_clock_type_get(uint *const clock_type) = 0;

    /* various init */
    virtual mesa_rc clock_startup(bool cold_init, bool pcb104_synce) = 0;
    virtual mesa_rc clock_init(bool cold_init, void **device_ptr) = 0;
    virtual mesa_rc clock_features_get(sync_clock_feature_t *features) = 0;
    virtual mesa_rc clock_shutdown(void)
    {
        return VTSS_RC_OK;
    };

    /* hw access */
    virtual mesa_rc clock_hardware_id_get(meba_synce_clock_hw_id_t *const hw_id)
    {
        *hw_id = clock_hw_id;
        return VTSS_RC_OK;
    }
    virtual mesa_rc clock_read(const uint reg, uint *const value) = 0;
    virtual mesa_rc clock_write(const uint reg, const uint value) = 0;

    /* syncE usage */
    virtual mesa_rc clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency) = 0;
    virtual mesa_rc clock_adjtimer_set(i64 adj) = 0;
    virtual mesa_rc clock_adjtimer_enable(bool enable) = 0;

    /* ptp usage */
    /* create new clock instance. 'source' is either SYNCE or INDEP. Called when a new PTP clock instance is added */
    virtual mesa_rc clock_ptp_timer_source_set(ptp_clock_source_t source) = 0;
    /* adjust PTP timer */
    virtual mesa_rc clock_output_adjtimer_set(i64 adj) = 0;
    virtual mesa_rc clock_adj_phase_set(i32 adj) = 0;

    /* polling/threads */
    virtual mesa_rc clock_event_poll(bool interrupt, clock_event_type_t *ev_mask) = 0;

    /* various */
    virtual mesa_rc clock_losx_state_get(bool *const state) = 0;
    virtual mesa_rc clock_selector_map_set(const uint reference, const uint clock_input) = 0;
    virtual mesa_rc clock_ho_frequency_offset_get(i64 *const offset);
    virtual vtss_zl_30380_dpll_type_t dpll_type() = 0;
    virtual mesa_rc clock_nco_assist_set(bool enable)
    {
        return VTSS_RC_OK;
    };
    virtual mesa_rc fw_ver_get(meba_synce_clock_fw_ver_t *fw_ver)
    {
        return VTSS_RC_ERROR;
    }
    virtual bool    fw_update(char *err_str, size_t err_str_size)
    {
        err_str[0] = '\0';
        return false;
    }
    virtual mesa_rc clock_link_state_set(uint32_t clock_input, bool link)
    {
        return MESA_RC_OK;
    };
    virtual mesa_rc clock_reset_to_defaults(void)
    {
        return MESA_RC_OK;
    }
};

#endif // _SYNCE_DPLL_BASE_H
