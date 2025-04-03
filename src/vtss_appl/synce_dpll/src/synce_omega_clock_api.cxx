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

#include "synce_constants.h"
#include "synce_omega_clock_api.h"
#include "synce_dpll_trace.h"
#include "critd_api.h"

#define CRIT_ENTER() critd_enter(&crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &crit, __FILE__, __LINE__)

static vtss_handle_t thread_handle_omega;
static vtss_thread_t thread_block_omega;
static void omega_thread(vtss_addrword_t data);

static critd_t          crit;
static meba_synce_clock_frequency_t clock_freq[CLOCK_INPUT_MAX] = {MEBA_SYNCE_CLOCK_FREQ_INVALID, MEBA_SYNCE_CLOCK_FREQ_INVALID};
static vtss_appl_synce_selection_mode_t clock_selection_mode;
static bool adjtimer_enabled = false;
static mesa_clock_selection_conf_t api_mode;
static uint clock_pri[CLOCK_INPUT_MAX];  // store clock input priorities for workaround to the problem that the Omega DPLL fails if all priorities are disabled
static bool all_dis;                     // indicate that all inputs are disabled
static ptp_clock_source_t current_source = PTP_CLOCK_SOURCE_INDEP;

#define PSL_LIMIT_MAX 9400      // phase slope limit in unlocked state
#define PSL_LIMIT_EEC1 885      // phase slope limit in locked state EEC option 1
#define PSL_LIMIT_EEC2 885      // phase slope limit in locked state EEC option 2
#define PSL_LIMIT_PTP  512      // phase slope limit in locked state PTP phase setting

#define SYNCE_EEC1_BW 35000      // 3,5 Hz SyncE clock output bandwidth in EEC option 1 in units of 100uHz
#define SYNCE_EEC2_BW 1000       // 0,1 Hz SyncE clock output bandwidth in EEC option 2 in units of 100uHz
#define SYNCE_PTP_BW  11550      // 1,155 Hz PTP clock output bandwidth in units of 100uHz

// Configure PSL limit when unlocked
static CapArray<u32, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT> psl_limit_unlocked;  // Note: Is now initialized below in vtss_omega_clock_init function

// Configure PSL limit in EEC option 1 or 2 (first index) pr port (second index)
static CapArray<u32, MEBA_CAP_SYNCE_CLOCK_EEC_OPTION_CNT, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT> psl_limit_locked;  // Note: Is now initialized below in vtss_omega_clock_init function

// Configure bandwitdh limit in EEC option 1 or 2 (first index) pr port (second index)
static CapArray<u32, MEBA_CAP_SYNCE_CLOCK_EEC_OPTION_CNT, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT> bw;  // Note: Is now initialized below in vtss_omega_clock_init function

static clock_eec_option_t my_clock_eec_option = CLOCK_EEC_OPTION_1;

#define CLOCK_API_INST NULL     // API instance pointer

#define SYNCE_CUST_RC(expr) { rc |= (expr); if (rc < VTSS_RC_OK) { \
T_WG(TRACE_GRP_OMEGA,"Error code: %x", rc); }}

/*
 * Early initialization
 * Do the API initialization
 */
mesa_rc vtss_omega_clock_init(bool  cold_init)
{
    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    /*lint --e{454} */
    /* Locking mechanism is initialized but not released - this is to protect any call to API until clock_startup has been done */
    critd_init_legacy(&crit, "synce.clock", VTSS_MODULE_ID_SYNCE_DPLL, CRITD_TYPE_MUTEX);

    // Initialize arrays that now have lengths determined at runtime and hence cannot be initialized by a literal constant.
    for (int i = 0; i < psl_limit_unlocked.size(); i++) {
        psl_limit_unlocked[i] = PSL_LIMIT_MAX;
    }

    for (int i = 0; i < psl_limit_locked.size(); i++) {
        for (int j = 0; j < psl_limit_locked[i].size(); j++) {
            if (j == 1) {
                psl_limit_locked[i][j] = PSL_LIMIT_PTP;
            } else if (i == 0) {
                psl_limit_locked[i][j] = PSL_LIMIT_EEC1;
            } else {
                psl_limit_locked[i][j] = PSL_LIMIT_EEC2;
            }
        }
    }

    for (int i = 0; i < bw.size(); i++) {
        for (int j = 0; j < bw[i].size(); j++) {
            if (j == 1) {
                bw[i][j] = SYNCE_PTP_BW;
            } else if (i == 0) {
                bw[i][j] = SYNCE_EEC1_BW;
            } else {
                bw[i][j] = SYNCE_EEC2_BW;
            }
        }
    }

    return MESA_RC_OK;
}

/*lint -sem(clock_startup,   thread_protected) ... We're protected */
/*
 * Late initialization
 * Now the synchronization process can be started
 */
mesa_rc vtss_omega_clock_startup(bool cold_init, bool pcb104_synce)
{
    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    meba_reset(board_instance, MEBA_SYNCE_DPLL_INITIALIZE);

    // create helper thread for Omega work around
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       omega_thread,
                       0,
                       "Clock API",
                       nullptr,
                       0,
                       &thread_handle_omega,
                       &thread_block_omega);

    T_IG(TRACE_GRP_OMEGA, "clock startup");

    /*lint -e(455) */
    CRIT_EXIT();

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_shutdown()
{
    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    (void) mesa_clock_shutdown(CLOCK_API_INST);

    return MESA_RC_OK;
}

/*
 * Configure selection mode in the DPLL
 *
 */
mesa_rc vtss_omega_clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input, uint clock_my_input_max)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_selector_state_t   api_selector_state;
    u8  api_clock_input;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    if (clock_input >= clock_my_input_max) {
        return MESA_RC_OK;
    }

    CRIT_ENTER();
    clock_selection_mode = mode;
    api_mode.clock_input = clock_input;

    T_WG(TRACE_GRP_OMEGA, "mode %d, input %d", mode, clock_input);
    switch (mode) {
    case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL:
        api_mode.mode = MESA_CLOCK_SELECTION_MODE_MANUEL;
        break;
    case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED:
        api_mode.mode = MESA_CLOCK_SELECTION_MODE_MANUEL;
        SYNCE_CUST_RC(mesa_clock_selector_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_selector_state, &api_clock_input));
        api_mode.clock_input = api_clock_input;
        break;
    case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE:
        api_mode.mode = MESA_CLOCK_SELECTION_MODE_AUTOMATIC_NONREVERTIVE;
        break;
    case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE:
        api_mode.mode = MESA_CLOCK_SELECTION_MODE_AUTOMATIC_REVERTIVE;
        break;
    case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER:
        api_mode.mode = MESA_CLOCK_SELECTION_MODE_FORCED_HOLDOVER;
        break;
    case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN:
        api_mode.mode = MESA_CLOCK_SELECTION_MODE_FORCED_FREE_RUN;
        break;
    default:
        break;
    }
    if (!all_dis || (api_mode.mode != MESA_CLOCK_SELECTION_MODE_AUTOMATIC_REVERTIVE && api_mode.mode != MESA_CLOCK_SELECTION_MODE_AUTOMATIC_NONREVERTIVE)) {  // only update selector state if some inputs are enabled
        SYNCE_CUST_RC(mesa_clock_selection_mode_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_mode));
    }
    CRIT_EXIT();

    return rc;
}

static vtss_appl_synce_selector_state_t api_selector_state_2_selector_state(mesa_clock_selector_state_t api_sel)
{
    switch (api_sel) {
    case MESA_CLOCK_SELECTOR_STATE_LOCKED:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED;
    case MESA_CLOCK_SELECTOR_STATE_HOLDOVER:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER;
    case MESA_CLOCK_SELECTOR_STATE_FREERUN:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
    case MESA_CLOCK_SELECTOR_STATE_DCO:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_PTP;
    case MESA_CLOCK_SELECTOR_STATE_REF_FAILED:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_REF_FAILED;
    case MESA_CLOCK_SELECTOR_STATE_ACQUIRING:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING;
    default:
        return VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
    }
}

mesa_rc vtss_omega_clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_selector_state_t   api_selector_state;
    u8  api_clock_input;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;

    SYNCE_CUST_RC(mesa_clock_selector_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_selector_state, &api_clock_input));

    *selector_state = api_selector_state_2_selector_state(api_selector_state);
    *clock_input = (uint)api_clock_input;

    return rc;
}

mesa_rc vtss_omega_clock_priority_set(const uint clock_input, const uint priority, uint clock_my_input_max, uint synce_my_prio_disabled)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_priority_selector_t  api_pri;
    mesa_clock_selection_conf_t my_api_mode;
    mesa_clock_selector_state_t api_selector_state;
    u8  api_clock_input;
    bool my_all_dis = true;
    int i;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    if (clock_input >= clock_my_input_max) {
        return MESA_RC_OK;
    }

    clock_pri[clock_input] = priority;
    for (i = 0; i < clock_my_input_max; i++) {
        if (clock_pri[i] != synce_my_prio_disabled) {
            my_all_dis = false;
        }
    }
    if (my_all_dis) { // this is awork around to the problem that the Omega DPLL fails if all priorities are disabled
        SYNCE_CUST_RC(mesa_clock_selector_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_selector_state, &api_clock_input));
        T_WG(TRACE_GRP_OMEGA, "all inputs are disabled, selector_state = %d", api_selector_state);
        if (api_selector_state == MESA_CLOCK_SELECTOR_STATE_LOCKED) {
            T_WG(TRACE_GRP_OMEGA, "all inputs are disabled, selector_state = %d", api_selector_state);
            my_api_mode.mode = MESA_CLOCK_SELECTION_MODE_FORCED_HOLDOVER;
            my_api_mode.clock_input = 0;  // Note: Not really used in this case, but must be set to a legal value or the API will complain.
            SYNCE_CUST_RC(mesa_clock_selection_mode_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &my_api_mode));
            all_dis = true;
        }
    } else {
        if (api_mode.mode != MESA_CLOCK_SELECTION_MODE_MANUEL) {  // don't touch the priority if manual mode, as the priority overrules manual selection
            api_pri.enable = (priority == synce_my_prio_disabled) ? false : true;
            api_pri.priority = priority;
            T_WG(TRACE_GRP_OMEGA, "clock input %d, priority %d, enable %d", clock_input, api_pri.priority, api_pri.enable);
            SYNCE_CUST_RC(mesa_clock_priority_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), clock_input, &api_pri));
        }
        if (all_dis) {
            T_WG(TRACE_GRP_OMEGA, "not all inputs are disabled, restore selection mode to = %d", api_mode.mode);
            SYNCE_CUST_RC(mesa_clock_selection_mode_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_mode));
            all_dis = false;
        }
    }

    return rc;
}

mesa_rc vtss_omega_clock_priority_get(const uint clock_input, uint *const priority, uint clock_my_input_max, uint synce_my_prio_disabled)
{
    mesa_clock_priority_selector_t api_pri = {};

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    *priority = 0;
    if (clock_input >= clock_my_input_max) {
        return MESA_RC_OK;
    }
    *priority = clock_pri[clock_input];
    T_WG(TRACE_GRP_OMEGA, "clock input %d, priority %d, enable %d", clock_input, *priority, api_pri.enable);

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_holdoff_time_set(const uint clock_input, const uint ho_time, uint clock_my_input_max)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_dpll_conf_t conf;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    if (clock_input >= clock_my_input_max) {
        return MESA_RC_OK;
    }
    SYNCE_CUST_RC(mesa_clock_operation_conf_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &conf));
    conf.holdoff = ho_time;
    SYNCE_CUST_RC(mesa_clock_operation_conf_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &conf));
    T_WG(TRACE_GRP_OMEGA, "clock_input %d, time %d", clock_input, ho_time);

    return rc;
}

mesa_rc vtss_omega_clock_holdoff_event(const uint clock_input)
{
    T_WG(TRACE_GRP_OMEGA, "no need to call me. clock_input %d", clock_input);

    return MESA_RC_OK;
}

mesa_rc   vtss_omega_clock_holdoff_run(bool *const active)
{
    T_WG(TRACE_GRP_OMEGA, "no need to call me. active %d", *active);

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_holdoff_active_get(const uint clock_input, bool *const active)
{
    T_WG(TRACE_GRP_OMEGA, "no need to call me. clock_input %d", clock_input);

    return MESA_RC_OK;
}

static void clock_frequency_2_khz(meba_synce_clock_frequency_t frequency, u32 *freq_khz, mesa_clock_ratio_t *ratio)
{
    switch (frequency) {
    case MEBA_SYNCE_CLOCK_FREQ_125MHZ:
        *freq_khz = 125000;
        ratio->num = 1;
        ratio->den = 1;
        break;
    case MEBA_SYNCE_CLOCK_FREQ_10MHZ:
        *freq_khz = 10000;
        ratio->num = 1;
        ratio->den = 1;
        break;
    case MEBA_SYNCE_CLOCK_FREQ_156_25MHZ:
        *freq_khz = 156250;
        ratio->num = 1;
        ratio->den = 1;
        break;
    case MEBA_SYNCE_CLOCK_FREQ_161_13MHZ:
        *freq_khz = 156250;
        ratio->num = 66;
        ratio->den = 64;
        break;
    case MEBA_SYNCE_CLOCK_FREQ_1544_KHZ:
        *freq_khz = 1544;
        ratio->num = 1;
        ratio->den = 1;
        break;
    case MEBA_SYNCE_CLOCK_FREQ_2048_KHZ:
        *freq_khz = 2048;
        ratio->num = 1;
        ratio->den = 1;
        break;
    default:
        *freq_khz = 0;
        ratio->num = 1;
        ratio->den = 1;
        break;
    }
}

/*
 * Set the expected input frequency from a recovered clock or station clock
 *
 */
mesa_rc vtss_omega_clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency, uint clock_my_input_max)
{
    mesa_rc rc = MESA_RC_OK;
    u32 freq_khz;
    mesa_clock_ratio_t ratio;
    BOOL use_internal_clock_src;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    if (clock_input >= clock_my_input_max) {
        return MESA_RC_OK;
    }

    if (clock_freq[clock_input] == frequency) {
        return MESA_RC_OK;
    }

    clock_freq[clock_input] = frequency;
    SYNCE_CUST_RC(mesa_clock_input_frequency_get(CLOCK_API_INST, clock_input, &freq_khz, &use_internal_clock_src));
    clock_frequency_2_khz(frequency, &freq_khz, &ratio);
    T_WG(TRACE_GRP_OMEGA, "station clock or recovered clock is nominated. clock_input %d, frequency %d KHz", clock_input, freq_khz);
    //SYNCE_CUST_RC(mesa_clock_input_frequency_set(CLOCK_API_INST, clock_input, freq_khz, use_internal_clock_src));
    SYNCE_CUST_RC(mesa_clock_input_frequency_ratio_set(CLOCK_API_INST, clock_input, freq_khz, &ratio, use_internal_clock_src));

    return rc;
}

mesa_rc vtss_omega_clock_locs_state_get(const uint clock_input, bool *const state)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_input_state_t input_state;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    *state = false;
    SYNCE_CUST_RC(mesa_clock_input_state_get(CLOCK_API_INST, clock_input, &input_state));
    *state = input_state.los || input_state.pfm || input_state.cfm || input_state.scm || input_state.lol;
    T_IG(TRACE_GRP_OMEGA, "locs %d, los %d, lol %d", *state, input_state.los, input_state.lol);

    return rc;
}

mesa_rc vtss_omega_clock_fos_state_get(const uint clock_input, bool *const state)
{
    *state = false;

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_losx_state_get(bool *const state)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_dpll_state_t pll_state;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    *state = false;
    SYNCE_CUST_RC(mesa_clock_dpll_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &pll_state));
    *state = pll_state.pll_losx;
    T_IG(TRACE_GRP_OMEGA, "losx %d", *state);

    return rc;
}

mesa_rc vtss_omega_clock_lol_state_get(bool *const state)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_dpll_state_t pll_state;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    *state = false;
    SYNCE_CUST_RC(mesa_clock_dpll_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &pll_state));
    *state = pll_state.pll_lol | pll_state.pll_losx;  // FIXME: Added pll_state.pll_losx term since pll_state.pll_lol can be false (OK) although pll_state.pll_losx is true (reference failing).
    T_IG(TRACE_GRP_OMEGA, "lol %d", *state);          //        Check that the above is OK.

    return rc;
}

mesa_rc vtss_omega_clock_dhold_state_get(bool *const state)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_dpll_state_t pll_state;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    *state = false;
    SYNCE_CUST_RC(mesa_clock_dpll_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &pll_state));
    *state = pll_state.pll_dig_hold_vld;
    T_IG(TRACE_GRP_OMEGA, "dhold %d", *state);

    return rc;
}

mesa_rc vtss_omega_clock_station_clk_out_freq_set(const u32 freq_khz)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

#if 0
    SYNCE_CUST_RC(mesa_clock_output_frequency_set(CLOCK_API_INST, 1, 0, 32 * freq_khz));
    T_WG(TRACE_GRP_OMEGA, "freq_khz %d", freq_khz);
#endif

    return rc;
}

mesa_rc vtss_omega_clock_station_clk_in_freq_set(const u32 freq_khz)
{
    T_WG(TRACE_GRP_OMEGA, "Not used in Omega HW");

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_station_clock_type_get(uint *const clock_type)
{
    *clock_type = 3;

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_eec_option_type_get(uint *const eec_type)
{
    *eec_type = 0;

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_features_get(uint *features)
{
    *features = 2;

    return MESA_RC_OK;
}

mesa_rc vtss_omega_clock_adjtimer_set(i64 adj)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    if (adjtimer_enabled) {
        SYNCE_CUST_RC(mesa_clock_dco_frequency_offset_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), adj));
        T_WG(TRACE_GRP_OMEGA, "adjust Synce DPLL " VPRI64d, adj);
    }

    return rc;
}

mesa_rc vtss_omega_clock_adjtimer_enable(bool enable)
{
    mesa_rc rc = MESA_RC_OK;
    static const mesa_clock_selection_conf_t my_api_conf = {MESA_CLOCK_SELECTION_MODE_FORCED_DCO, 0};

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    adjtimer_enabled = enable;
    if (enable) {
        SYNCE_CUST_RC(mesa_clock_selection_mode_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &my_api_conf));
    } else {
        SYNCE_CUST_RC(mesa_clock_selection_mode_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_mode));
    }
    T_WG(TRACE_GRP_OMEGA, "enable Synce DPLL adjustment %d", enable);

    return rc;
}

mesa_rc vtss_omega_clock_adj_phase_set(i32 phase)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    u8 clock_mask = 0, ret_val = 0;
    BOOL adj_ongoing;
    if (abs(phase) >= (1 << 16) / 10) { // TBD what the phase step granularity is
        SYNCE_CUST_RC(mesa_clock_adj_phase_get(CLOCK_API_INST, &adj_ongoing));
        if (!adj_ongoing) {
            /*For Serval-t, SYNCE_PTP_CLOCK_OUTPUT(1) is returned and so it used for comparison below to avoid coverity. */
            if ((ret_val =  meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) == 1) {
                clock_mask |= 1 << ret_val;
            }
            T_WG(TRACE_GRP_OMEGA, "set phase offset on output mask 0x%x", clock_mask);
            SYNCE_CUST_RC(mesa_clock_adj_phase_set(CLOCK_API_INST, clock_mask, phase));
            T_WG(TRACE_GRP_OMEGA, "phase set to: %d * 0.1 ns", ((phase >> 4) * 10) >> 12); // Making shift by 16 in two steps as an initial shift by 4 followed by a shift by 12.
        } else {                                                                           // If we do the complete shift by 16 at the end, (phase * 10) can overflow (phase is only 32 bits).
            T_WG(TRACE_GRP_OMEGA, "previous phase adj not completed ");
        }
    }

    return rc;
}

mesa_rc vtss_omega_clock_output_adjtimer_set(i64 adj)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    SYNCE_CUST_RC(mesa_clock_adj_frequency_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT), adj));
    T_WG(TRACE_GRP_OMEGA, "adjust Synce output 1 DPLL " VPRI64d, adj);

    return rc;
}

/*
 * if source is SYNCE, then the PTP clock output is connected to DPLL0
 * if source is FREE_RUN, then the PTP clock output is connected to Internal clock
 */
mesa_rc vtss_omega_clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_input_selector_t input;
    mesa_clock_psl_conf_t psl;
    mesa_clock_selector_state_t api_selector_state;
    u8 api_clock_input;
    uint32_t synce_clock_cap = 0;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }
    current_source = source;
    input.input_inst = 0;
    input.input_type = (source == PTP_CLOCK_SOURCE_SYNCE) ? MESA_CLOCK_INPUT_TYPE_DPLL : MESA_CLOCK_INPUT_TYPE_PURE_DCO;
    synce_clock_cap = meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT);
    if (synce_clock_cap == MESA_RC_NOT_IMPLEMENTED) {
        return MESA_RC_ERROR;
    }

    SYNCE_CUST_RC(mesa_clock_selector_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_selector_state, &api_clock_input));
    SYNCE_CUST_RC(mesa_clock_output_selector_set(CLOCK_API_INST, synce_clock_cap, &input));

    psl_limit_unlocked[synce_clock_cap] = (source == PTP_CLOCK_SOURCE_SYNCE) ? PSL_LIMIT_MAX : PSL_LIMIT_PTP;
    psl.phase_build_out_ena = FALSE;
    psl.ho_based = true;
    if (input.input_type == MESA_CLOCK_INPUT_TYPE_PURE_DCO) {
        psl.limit_ppb = 0; // PSL is disabled in pure DCO mode
    } else {
        psl.limit_ppb = (api_selector_state == MESA_CLOCK_SELECTOR_STATE_LOCKED) ? psl_limit_locked[(my_clock_eec_option == CLOCK_EEC_OPTION_1) ? 0 : 1][synce_clock_cap] : psl_limit_unlocked[synce_clock_cap];
    }
    SYNCE_CUST_RC(mesa_clock_output_psl_conf_set(CLOCK_API_INST, synce_clock_cap, &psl));
    T_WG(TRACE_GRP_OMEGA, "timer source set %d, psl limit_ppb %d ppb", source, psl.limit_ppb);

    return rc;
}

mesa_rc vtss_omega_clock_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    mesa_rc rc = MESA_RC_OK;
    u32 my_bw;
    mesa_clock_psl_conf_t psl;
    u8 clock_out;
    mesa_clock_selector_state_t api_selector_state;
    u8 api_clock_input;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    my_clock_eec_option = clock_eec_option;
    CRIT_ENTER();
    SYNCE_CUST_RC(mesa_clock_selector_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_selector_state, &api_clock_input));

    psl.ho_based = true;
    for (clock_out = 0; clock_out < meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT); clock_out++) {
        /* set DPLL bandwidth to 0,1 HZ (EEC2) or 3.5 Hz (EEC1 option) */
        my_bw = bw[(clock_eec_option == CLOCK_EEC_OPTION_1) ? 0 : 1][clock_out];
        if (current_source == PTP_CLOCK_SOURCE_INDEP && clock_out == meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) {
            psl.limit_ppb = 0;
        } else {
            psl.limit_ppb = (api_selector_state == MESA_CLOCK_SELECTOR_STATE_LOCKED) ? psl_limit_locked[(clock_eec_option == CLOCK_EEC_OPTION_1) ? 0 : 1][clock_out] : psl_limit_unlocked[clock_out];
        }
        psl.phase_build_out_ena = (clock_out == meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) ? FALSE : TRUE;
        T_WG(TRACE_GRP_OMEGA, "bw[%d] = %d, psl = %d, build_out_ena %d", clock_out, my_bw, psl.limit_ppb, psl.phase_build_out_ena);
        SYNCE_CUST_RC(mesa_clock_output_filter_bw_set(CLOCK_API_INST, clock_out, my_bw));
        SYNCE_CUST_RC(mesa_clock_output_psl_conf_set(CLOCK_API_INST, clock_out, &psl));
    }
    T_WG(TRACE_GRP_OMEGA, "eec_option %d", clock_eec_option);
    CRIT_EXIT();

    return rc;
}

mesa_rc vtss_omega_clock_selector_map_set(const uint reference,
                                          const uint clock_input)
{
    T_WG(TRACE_GRP_OMEGA, "no map set needed");

    return MESA_RC_OK;
}

/**
 * \brief get Clock frequency holdover offset
 * \param offset [OUT]    Current frequency offset stored in the holdover stack in units of scaled ppb (parts per billion) i.e. ppb*2**-16.
 *
 * \return Return code.
 */
mesa_rc vtss_omega_clock_ho_frequency_offset_get(i64                           *const offset)
{
    mesa_rc rc = MESA_RC_OK;
    SYNCE_CUST_RC(mesa_clock_ho_stack_frequency_offset_get(CLOCK_API_INST,  meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), offset));
    T_WG(TRACE_GRP_OMEGA, "ho freq " VPRI64d " from dpll %d", *offset, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL));
    return rc;
}

mesa_rc vtss_omega_clock_wtr_set(u32 wtr_time)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_dpll_conf_t conf;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    SYNCE_CUST_RC(mesa_clock_operation_conf_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &conf));
    conf.wtr = wtr_time;
    SYNCE_CUST_RC(mesa_clock_operation_conf_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &conf));
    T_WG(TRACE_GRP_OMEGA, "set wtr time to %d seconds", wtr_time);

    return rc;
}

mesa_rc vtss_omega_clock_wtr_get(u32 *const wtr_time)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_clock_dpll_conf_t conf;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    SYNCE_CUST_RC(mesa_clock_operation_conf_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &conf));
    *wtr_time = conf.wtr;
    T_IG(TRACE_GRP_OMEGA, "get wtr time: %d seconds", *wtr_time);

    return rc;
}

mesa_rc vtss_omega_clock_read(const uint reg, uint *const value)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    SYNCE_CUST_RC(mesa_clock_rd(CLOCK_API_INST, reg, value));

    return rc;
}

mesa_rc vtss_omega_clock_write(const uint reg, const uint value)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    SYNCE_CUST_RC(mesa_clock_wr(CLOCK_API_INST, reg, value));

    return rc;
}

mesa_rc vtss_omega_clock_writemasked(const uint reg, const uint value, const uint mask)
{
    mesa_rc rc = MESA_RC_OK;

    if (!fast_cap(MESA_CAP_CLOCK)) {
        return MESA_RC_ERROR;
    }

    uint tmp;
    SYNCE_CUST_RC(mesa_clock_rd(CLOCK_API_INST, reg, &tmp));
    tmp = (tmp & ~mask) | (value & mask);
    SYNCE_CUST_RC(mesa_clock_wr(CLOCK_API_INST, reg, tmp));

    return rc;
}

/*
 * This thread implements Omega work arounds.
 * The thread needs mutex, as it is sharing data with the other functions
 */
static void omega_thread(vtss_addrword_t data)
{
    mesa_rc rc = VTSS_RC_OK;
    mesa_clock_selector_state_t   api_selector_state;
    u8  api_clock_input;
    BOOL in_locked_state = FALSE;
    u8 clock_out;
    BOOL lock_completed = FALSE;
    u32 in_locked_time = 0;
    mesa_clock_ho_stack_conf_t ho_conf;
    ho_conf.ho_post_filtering_bw = meba_capability(board_instance, MEBA_CAP_SYNCE_HO_POST_FILTERING_BW);
    ho_conf.ho_qual_time_conf = 0;
    // after 4 sec it is set to 1, meaning the used holdover level is average over 2 sec. Etc. i.e. we avoid using the values from the initial pull-in period.
    static const u32 ho_qual_time_level[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    mesa_clock_psl_conf_t psl;
    u32 in_psl_lock_unlock_time = 0;
    BOOL in_psl_locked = FALSE;

    for (;;) {
        VTSS_OS_MSLEEP(1000);
        CRIT_ENTER();
        SYNCE_CUST_RC(mesa_clock_selector_state_get(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &api_selector_state, &api_clock_input));
        if (in_locked_state) {
            if (!lock_completed) {
                for (clock_out = 0; clock_out < meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT); clock_out++) {
                    SYNCE_CUST_RC(mesa_clock_output_filter_lock_fast_get(CLOCK_API_INST, clock_out, &lock_completed));
                }
                T_IG(TRACE_GRP_OMEGA, "lock fast completed: %d", lock_completed);
            }
            if (api_selector_state != MESA_CLOCK_SELECTOR_STATE_LOCKED) {
                in_locked_state = FALSE;
                lock_completed = FALSE;
                ho_conf.ho_qual_time_conf = 0;
                SYNCE_CUST_RC(mesa_clock_ho_stack_conf_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &ho_conf));
                T_WG(TRACE_GRP_OMEGA, "ho_qual_time_conf: %d", ho_conf.ho_qual_time_conf);
                T_WG(TRACE_GRP_OMEGA, "in_locked_state: %d", in_locked_state);
            } else {
                // update holdover stack during locked state
                ho_conf.ho_post_filtering_bw = meba_capability(board_instance, MEBA_CAP_SYNCE_HO_POST_FILTERING_BW);
                in_locked_time++;
                if (ho_conf.ho_qual_time_conf < (sizeof(ho_qual_time_level) / sizeof(u32) - 1) && in_locked_time >= ho_qual_time_level[ho_conf.ho_qual_time_conf]) {
                    ho_conf.ho_qual_time_conf++;
                    SYNCE_CUST_RC(mesa_clock_ho_stack_conf_set(CLOCK_API_INST, meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_DPLL), &ho_conf));
                    T_WG(TRACE_GRP_OMEGA, "ho_qual_time_conf: %d", ho_conf.ho_qual_time_conf);
                }
            }
        } else {
            T_NG(TRACE_GRP_OMEGA, "selector_state: %d", api_selector_state);
            if (api_selector_state == MESA_CLOCK_SELECTOR_STATE_LOCKED) {
                for (clock_out = 0; clock_out < meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT); clock_out++) {
                    SYNCE_CUST_RC(mesa_clock_output_filter_lock_fast_set(CLOCK_API_INST, clock_out));
                }
                in_locked_state = TRUE;
                in_locked_time = 0;
                ho_conf.ho_qual_time_conf = 0;
                T_WG(TRACE_GRP_OMEGA, "in_locked_state: %d", in_locked_state);
            }
        }
        // PSL work around
        if (in_psl_locked) {
            if (api_selector_state != MESA_CLOCK_SELECTOR_STATE_LOCKED) {
                if (in_psl_lock_unlock_time++ >= 2) {
                    in_psl_locked = FALSE;
                    in_psl_lock_unlock_time = 0;
                    // update phase slope limit
                    psl.phase_build_out_ena = TRUE;
                    psl.ho_based = true;
                    for (clock_out = 0; clock_out < meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT); clock_out++) {
                        if (current_source == PTP_CLOCK_SOURCE_INDEP && clock_out == meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) {
                            psl.limit_ppb = 0;
                        } else {
                            psl.limit_ppb = psl_limit_unlocked[clock_out];
                        }
                        psl.phase_build_out_ena = (clock_out == meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) ? FALSE : TRUE;
                        T_WG(TRACE_GRP_OMEGA, "Limit_ppb[%d] set to %d", clock_out, psl.limit_ppb);
                        SYNCE_CUST_RC(mesa_clock_output_psl_conf_set(CLOCK_API_INST, clock_out, &psl));
                    }
                }
            } else {
                in_psl_lock_unlock_time = 0;
            }
        } else {
            T_NG(TRACE_GRP_OMEGA, "selector_state: %d", api_selector_state);
            if (api_selector_state == MESA_CLOCK_SELECTOR_STATE_LOCKED) {
                if (in_psl_lock_unlock_time++ >= 5) {
                    in_psl_locked = TRUE;
                    in_psl_lock_unlock_time = 0;
                    // update phase slope limit
                    psl.phase_build_out_ena = TRUE;
                    psl.ho_based = true;
                    for (clock_out = 0; clock_out < meba_capability(board_instance, MEBA_CAP_SYNCE_CLOCK_OUTPUT_CNT); clock_out++) {
                        if (current_source == PTP_CLOCK_SOURCE_INDEP && clock_out == meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) {
                            psl.limit_ppb = 0;
                        } else {
                            psl.limit_ppb = psl_limit_locked[(my_clock_eec_option == CLOCK_EEC_OPTION_1) ? 0 : 1][clock_out]; // slope limit = configured limit
                        }
                        psl.phase_build_out_ena = (clock_out == meba_capability(board_instance, MEBA_CAP_SYNCE_PTP_CLOCK_OUTPUT)) ? FALSE : TRUE;
                        T_WG(TRACE_GRP_OMEGA, "Limit_ppb[%d] set to %d", clock_out, psl.limit_ppb);
                        SYNCE_CUST_RC(mesa_clock_output_psl_conf_set(CLOCK_API_INST, clock_out, &psl));
                    }
                }
            } else {
                in_psl_lock_unlock_time = 0;
            }
        }
        CRIT_EXIT();
    }
}
