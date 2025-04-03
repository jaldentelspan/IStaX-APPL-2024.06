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

#if defined(VTSS_SW_OPTION_SYNCE) || defined(VTSS_SW_OPTION_PTP)

#include "synce_constants.h"
#include "synce_custom_clock_api.h"
#include "zl_3073x_synce_clock_api.h"
#include "zl_3073x_api.h"
#include "zl303xx.h"
#include "zl303xx_AddressMap73x.h"
#include "zl303xx_Dpll73xGlobal.h"
#include "zl_3073x_synce_support.h"
#include "main.h"
#include "microchip/ethernet/switch/api.h"
#include "critd_api.h"
#include <vtss_trace_lvl_api.h>
#include <stdio.h>
#include "board_if.h"
#include "zl303xx_Params.h"
#include "zl303xx_Var.h"
#include "zl303xx_DeviceSpec.h"
#include "synce_spi_if.h"

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

static vtss_trace_reg_t trace_reg = {
    VTSS_MODULE_ID_ZL_3034X_API, "zl_api", "ZL3073x DPLL API."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default (ZL3073x core)",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_OS_PORT] = {
        "os_port",
        "ZL OS porting functions",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_SYNC_INTF] = {
        "sync_intf",
        "ZL-Synce application interfacefunctions",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_ZL_TRACE] = {
        "zl-trace",
        "ZL-code trace level",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
};

#define SYNTH_ID_PTP 2  // Synthesizer used for PTP
#define SYNCE_DPLL_ID ZL303XX_DPLL_ID_1
#define PTP_DPLL_ID   ZL303XX_DPLL_ID_2
#define PTP_HYBRID_DPLL_ID   ZL303XX_DPLL_ID_6

zl303xx_ParamsS *zl_3073x_zl303xx_synce_dpll;      /* device instance returned from zl303xx_CreateDeviceInstance,used for SYNCE*/
zl303xx_ParamsS *zl_3073x_zl303xx_ptp_dpll; /* device instance returned from zl303xx_CreateDeviceInstance,used for PTP,g8265*/
zl303xx_ParamsS *zl_3073x_zl303xx_nco_asst_dpll; /* device instance returned from zl303xx_CreateDeviceInstance,used for PTP,g8275*/

static Uint32T cur_refId;
static BOOL cur_locked = FALSE;
static BOOL cur_holdover = FALSE;
static BOOL cur_ref_failed = FALSE;
static zl303xx_RefIdE cur_selected = ZL303XX_REF_AUTO; // current reference selected by application.

static uint cur_pri[ZL303XX_DPLL_NUM_REFS];
static BOOL cur_locs_failed[CLOCK_INPUT_MAX] = {FALSE, FALSE, FALSE};
static BOOL cur_pfm_failed[CLOCK_INPUT_MAX] = {FALSE, FALSE, FALSE};
/* holds the current mapping from clock_input to DPLL reference number */
static zl303xx_RefIdE cur_ref[CLOCK_INPUT_MAX] = {ZL303XX_REF_ID_8, ZL303XX_REF_ID_1, ZL303XX_REF_ID_2};
static ZLS3073X_DpllHWModeE current_mode = ZLS3073X_DPLL_MODE_FREERUN;
static zl303xx_RefIdE current_ref = ZL303XX_REF_ID_0;
static vtss_appl_synce_selection_mode_t my_selection_mode; // actual selection mode, if the DPLL is not in NCO mode
static uint my_selected_clock_input;                       // actual selected ref, if the DPLL is in manual mode
static bool nco_mode_enabled;                              // indicates if the NCO mode is enabled
static bool nco_assist_mode_enabled = false;                       // indicates if the NCO assistance mode is enabled
static Sint32T nco_ho_freqOffsetUppm = 0;                  // holds the actual offset at the time when the NCO mode is entered

static BOOL zl3073x_adjtimer_enabled;   // indicates if the PTP is allowed to control the synceDPLL frequency
static char mode_to_txt[5][15] = { "FREERUN", "HOLDOVER", "FORCED_REF", "AUTO", "NCO"};

int zl_3073x_clock_input2ref_id(uint clock_input)
{
    switch (clock_input) {
        case 0:
        case 1:
        case 2: return cur_ref[clock_input];
        default: return ZL303XX_REF_ID_0;
    }
}

static uint ref_id2clock_input(zl303xx_RefIdE ref_id)
{
    uint i;
    for (i = 0; i < CLOCK_INPUT_MAX; i++) {
        if (cur_ref[i] == ref_id) return i;
    }
    return 0;
}

/* zl priorities: 0 = highest, 0xe = lowest, 0xf = disabled */
/* synce priorities: 0 = highest, 0x1 = lowest */
static Uint32T sync_pri2zl_pri(uint pri)
{
    return pri;
}

static uint zl_pri2sync_pri(Uint32T pri)
{
    return pri;
}

mesa_rc zl_3073x_api_init(void **device_ptr)
{
    // Since zls303xx comes in several flavors, we cannot use
    // VTSS_TRACE_REGISTER() to do a constructor-based trace initialization,
    // because that would cause an assertion when the second instance attempts
    // to register with the same module ID.
    // Therefore, we need to do it like this:
    vtss_trace_reg_init(&trace_reg, trace_grps, ARRSZ(trace_grps));
    vtss_trace_register(&trace_reg);

    zl303xx_SetDefaultDeviceType(ZL3073X_DEVICETYPE);
    ZL_3073X_CHECK(zl303xx_ReadWriteInit());
    T_D("apr_env_init finished");

    /* Create a HW Driver for SYNCE alone operations*/
    ZL_3073X_CHECK(zl303xx_CreateDeviceInstance(&zl_3073x_zl303xx_synce_dpll));
    T_D("device instance created %p", zl_3073x_zl303xx_synce_dpll);

    zl_3073x_zl303xx_synce_dpll->deviceType = ZL3073X_DEVICETYPE;
    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
    ZL_3073X_CHECK(zl303xx_InitDeviceIdAndRev(zl_3073x_zl303xx_synce_dpll));
    zl303xx_Dpll73xParamsMutexInit(zl_3073x_zl303xx_synce_dpll);
    zl303xx_Dpll73xUpdateAllMailboxCopies(zl_3073x_zl303xx_synce_dpll);
    T_I("device instance created %p", zl_3073x_zl303xx_synce_dpll);

    /* Create a HW Driver for PTP g8265 operations*/
    ZL_3073X_CHECK(zl303xx_CreateDeviceInstance(&zl_3073x_zl303xx_ptp_dpll));
    T_D("device instance created %p", zl_3073x_zl303xx_ptp_dpll);

    zl_3073x_zl303xx_ptp_dpll->deviceType = ZL3073X_DEVICETYPE;

    zl_3073x_zl303xx_ptp_dpll->pllParams.pllId = PTP_DPLL_ID;
    ZL_3073X_CHECK(zl303xx_InitDeviceIdAndRev(zl_3073x_zl303xx_ptp_dpll));
    zl303xx_Dpll73xParamsMutexInit(zl_3073x_zl303xx_ptp_dpll);
    zl303xx_Dpll73xUpdateAllMailboxCopies(zl_3073x_zl303xx_ptp_dpll);
    *device_ptr = zl_3073x_zl303xx_ptp_dpll;
    /* Create a HW Driver for PTP g8275 operations*/
    ZL_3073X_CHECK(zl303xx_CreateDeviceInstance(&zl_3073x_zl303xx_nco_asst_dpll));
    T_D("device instance created %p", zl_3073x_zl303xx_nco_asst_dpll);

    zl_3073x_zl303xx_nco_asst_dpll->deviceType = ZL3073X_DEVICETYPE;

    zl_3073x_zl303xx_nco_asst_dpll->pllParams.pllId = PTP_HYBRID_DPLL_ID;
    ZL_3073X_CHECK(zl303xx_InitDeviceIdAndRev(zl_3073x_zl303xx_nco_asst_dpll));
    zl303xx_Dpll73xParamsMutexInit(zl_3073x_zl303xx_nco_asst_dpll);
    zl303xx_Dpll73xUpdateAllMailboxCopies(zl_3073x_zl303xx_nco_asst_dpll);

    return VTSS_RC_OK;
}

mesa_rc zl_3073x_clock_init(BOOL  cold_init)
{
    uint i;
    for (i = 0; i < ZL303XX_DPLL_NUM_REFS; i++) {
        cur_pri[i] = 0x10;
    }
    mesa_rc rc=VTSS_RC_OK;
    return(rc);
}

mesa_rc zl_3073x_clock_startup(BOOL cold_init, const uint clock_my_input_max)
{
    // switch (vtss_board_type()) {
    // case VTSS_BOARD_LAN9694_PCB8398:
    //     vtss::synce::dpll::clock_chip_spi_if.zl_3034x_load_file("/etc/mscc/dpll/cfg/EV23X71A_ZL80732_all_regs.mfg");
    //     break;
    // case VTSS_BOARD_LAN9668_EDS2_REF:
    //     vtss::synce::dpll::clock_chip_spi_if.zl_3034x_load_file("/etc/mscc/dpll/cfg/UNG8385_ZL30732_all_regs_v3.mfg");
    //         break;
    // default:
    //     T_E("Unrecognized board for Azurite DPLL");
    // }
    /* set Multiple to 5000 , to achieve expected frequency of 125Mhz */
    ZL_3073X_CHECK(zl303xx_Ref73xFreqMultipleSet(zl_3073x_zl303xx_synce_dpll, ZL303XX_REF_ID_0, 5000));
    ZL_3073X_CHECK(zl303xx_Ref73xFreqMultipleSet(zl_3073x_zl303xx_synce_dpll, ZL303XX_REF_ID_1, 5000));
    
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthSet(zl_3073x_zl303xx_ptp_dpll, ZLS3073X_BW_14Hz));
    //Set-up bandwidth, pull-in range and phase slope limit
    // These values are imported from zl3077x file.
    (void)zl_3073x_apr_set_up_for_bc_full_on_path_phase_synce();

    // Set-up holdover filter of 54.2mhz (0x9) and HO storage delay of 15seconds
    (void)zl3073x_DpllHoldoverFilterBandwidthSet(zl_3073x_zl303xx_synce_dpll, SYNCE_DPLL_ID, 0x9);
    (void)zl3073x_DpllHoldoverDelayStorageSet(zl_3073x_zl303xx_synce_dpll, SYNCE_DPLL_ID, 133);

    // Set-up holdover filter of 1.7mhz and HO storage delay of 1000 seconds for NCO assist dpll to support better holdover in 8275.
    (void)zl3073x_DpllHoldoverFilterBandwidthSet(zl_3073x_zl303xx_nco_asst_dpll, PTP_HYBRID_DPLL_ID, 0xE);
    (void)zl3073x_DpllHoldoverDelayStorageSet(zl_3073x_zl303xx_nco_asst_dpll, PTP_HYBRID_DPLL_ID, 192);

    // On Laguna Rev-B boards, there was no feedback loop from dpll output to ref 2N input by default.
    // To configure dpll settings after feedback loop rework, below function is used.
    zl303xx_Ref73xDpllFeedbackRefInit(zl_3073x_zl303xx_synce_dpll);
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input)
{
    ZLS3073X_DpllHWModeE val = (ZLS3073X_DpllHWModeE)-1;
    my_selection_mode = mode;
    my_selected_clock_input = clock_input;

    if (nco_mode_enabled) {
        T_IG(TRACE_GRP_SYNC_INTF,"DPLL mode: %d, ref %d saved while in NCO mode", mode, zl_3073x_clock_input2ref_id(clock_input));
    } else {
        switch (mode)
        {
            case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL:
                val = ZLS3073X_DPLL_MODE_REFLOCK;
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED:
                // FIXME: This is an illegal value, we should return an error message.
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE:
            case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE:
                val = ZLS3073X_DPLL_MODE_AUTO_LOCK;
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER:
                val = ZLS3073X_DPLL_MODE_HOLDOVER;
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN:
                // THE ZL3073x api does not support FREE_RUN mode. Therefore freerun is simulated as NCO with 0 frequency offset
                // ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xSetFreq(zl_3073x_zl303xx_synce_dpll, 0));
                val = ZLS3073X_DPLL_MODE_FREERUN;
                break;
            default: break;
        }
        T_IG(TRACE_GRP_SYNC_INTF,"DPLL latest mode: %s, old mode %s ref %d input_mode %d clock_input %d", mode_to_txt[val],mode_to_txt[current_mode], zl_3073x_clock_input2ref_id(clock_input), mode, clock_input);
        if (val != current_mode || (val == ZLS3073X_DPLL_MODE_REFLOCK && current_ref != (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input))) {
            ZLS3073X_DpllHWModeE cur_mode;
            current_ref = (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input);
            //T_IG(TRACE_GRP_SYNC_INTF,"DPLL mode: changed to %d", val);

            ZL_3073X_CHECK(zl303xx_Dpll73xModeGet(zl_3073x_zl303xx_synce_dpll, &cur_mode));
            if (cur_mode != ZLS3073X_DPLL_MODE_NCO)
            {
                if (val == ZLS3073X_DPLL_MODE_REFLOCK) {
                    // set manually selected reference
                    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
                    ZL_3073X_CHECK(zl303xx_Dpll73xHWModeSet(zl_3073x_zl303xx_synce_dpll, val, current_ref, 0));
                    zl303xx_Dpll73xCurrRefSet(zl_3073x_zl303xx_synce_dpll, current_ref);
                } else {
                    ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(zl_3073x_zl303xx_synce_dpll, val));
                }
            } else {
                zl_3073x_zl303xx_synce_dpll->pllParams.d73x.pllPriorMode = val;
                if (val == ZLS3073X_DPLL_MODE_REFLOCK) {
                    zl_3073x_zl303xx_synce_dpll->pllParams.selectedRef = current_ref;
                }
            }
            /* Store the current selected reference. This is needed because in holdover mode
               reference 'get' from dpll is shown as ZL303XX_REF_AUTO. */
            if ((val == ZLS3073X_DPLL_MODE_REFLOCK) || (val == ZLS3073X_DPLL_MODE_HOLDOVER)) {
                cur_selected = (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input);
            } else {
                cur_selected = ZL303XX_REF_AUTO;
            }
            current_mode = val;
            T_IG(TRACE_GRP_SYNC_INTF,"DPLL mode: current %d changed to %d", cur_mode, val);
        }
        T_DG(TRACE_GRP_SYNC_INTF,"DPLL mode: %d, ref %d", val, zl_3073x_clock_input2ref_id(clock_input));
    }
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_clock_selector_state_get(vtss_appl_synce_selector_state_t *const selector_state,
                                 uint                    *const clock_input)
{
    zl303xx_BooleanE refFailed = (zl303xx_BooleanE)false;
    zl303xx_DpllStatusS par;
    ZLS3073X_DpllHWModeE mode;
    Sint32T freqOffsetUppm;
    par.Id = SYNCE_DPLL_ID;
    // the holdover and locked bits are stichy in the DPLL chip, this means that it cannot be read 25 ms after the latest read.
    // therefore the register is only read in the event poll function, and the status is saved for other use.
    par.holdover = cur_holdover ? ZL303XX_DPLL_HOLD_TRUE : ZL303XX_DPLL_HOLD_FALSE;
    par.locked = cur_locked ? ZL303XX_DPLL_LOCK_TRUE : ZL303XX_DPLL_LOCK_FALSE;
    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
    ZL_3073X_CHECK(zl303xx_Dpll73xModeGet(zl_3073x_zl303xx_synce_dpll, &mode));
    // read actual holdover value
    if ( ZL303XX_REF_AUTO != (zl303xx_RefIdE)cur_refId  || mode == ZLS3073X_DPLL_MODE_NCO) {
        // read actual holdover value
        ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xGetFreq(zl_3073x_zl303xx_synce_dpll, &freqOffsetUppm, ZLS3073X_NORMAL_I_PART));

        if (mode == ZLS3073X_DPLL_MODE_NCO) {
            if (freqOffsetUppm == 0) {
                // force freerun is simulated as NCO mode with frequency offset == 0
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
            } else {
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_PTP;
            }
        } else if (par.holdover == ZL303XX_DPLL_HOLD_FALSE &&
                par.locked == ZL303XX_DPLL_LOCK_TRUE) {
            *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED;
        } else if (par.holdover == ZL303XX_DPLL_HOLD_TRUE) {
            if (freqOffsetUppm == 0) {
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
            } else {
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER;
            }
        } else if (par.locked == ZL303XX_DPLL_LOCK_FALSE && par.holdover == ZL303XX_DPLL_HOLD_FALSE) {
            if (mode == ZLS3073X_DPLL_MODE_FREERUN) {
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
            } else if (cur_ref_failed == ZL303XX_TRUE) {
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_REF_FAILED;
                T_DG(TRACE_GRP_SYNC_INTF,"CLOCK_SELECTOR_STATE_REF_FAILED");
            } else {
                *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING;
                T_DG(TRACE_GRP_SYNC_INTF,"acquiring lock, mode %d", mode);
            }
        } else {
            T_WG(TRACE_GRP_SYNC_INTF,"inconsistent selector state");
            *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED;
        }
        T_DG(TRACE_GRP_SYNC_INTF,"refFailed: %d, holdover %d, locked %d", refFailed , par.holdover, par.locked);
        T_DG(TRACE_GRP_SYNC_INTF,"freqOffsetUppm: %d", freqOffsetUppm);

        *clock_input = ref_id2clock_input((zl303xx_RefIdE) cur_refId);
    } else {
        if (mode == ZLS3073X_DPLL_MODE_HOLDOVER) {
            *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER;
        } else {
            *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
        }
        *clock_input = ref_id2clock_input((zl303xx_RefIdE) cur_refId);
    }
    T_DG(TRACE_GRP_SYNC_INTF,"selector state: %d, ref %d,cur_ref_failed %s selection Mode %d frm_hw = %s", *selector_state, cur_refId, cur_ref_failed? "failed" : "good",mode, mode_to_txt[mode]);
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_clock_priority_set(const uint   clock_input,
                                    const uint   priority)
{
    if (cur_ref[clock_input] < ZL303XX_DPLL_NUM_REFS) {
        if (priority != cur_pri[cur_ref[clock_input]]) {

            ZL_3073X_CHECK(zl3073x_DpllRefPrioritySet(zl_3073x_zl303xx_ptp_dpll, PTP_DPLL_ID, (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input), sync_pri2zl_pri(priority)));
            ZL_3073X_CHECK(zl3073x_DpllRefPrioritySet(zl_3073x_zl303xx_synce_dpll, SYNCE_DPLL_ID, (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input), sync_pri2zl_pri(priority)));
            T_IG(TRACE_GRP_SYNC_INTF,"Priority: %d, clock_input %d", sync_pri2zl_pri(priority), zl_3073x_clock_input2ref_id(clock_input));
            cur_pri[cur_ref[clock_input]] = priority;
        }
    } else {
        T_EG(TRACE_GRP_SYNC_INTF,"invalid DPLL reference no: %d",  cur_ref[clock_input]);
    }
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_clock_priority_get(const uint clock_input, uint *const priority)
{
    Uint32T zl_pri;
    ZL_3073X_CHECK(zl3073x_DpllRefPriorityGet(zl_3073x_zl303xx_synce_dpll, PTP_DPLL_ID, (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input), &zl_pri));
    *priority = zl_pri2sync_pri(zl_pri);
    T_DG(TRACE_GRP_SYNC_INTF,"Priority: %d, clock_input %d", zl_pri, (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(clock_input));
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_clock_los_get(const uint clock_input, bool *const los)
{
    *los = (bool) cur_locs_failed[clock_input];
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_los: %d, clock_input %d", *los, zl_3073x_clock_input2ref_id(clock_input));
    return(VTSS_RC_OK);
}

int zl_3073x_cur_los_get()
{
    BOOL los = cur_locs_failed[ref_id2clock_input((zl303xx_RefIdE)cur_refId)];
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_los: %d, clock_input %d", los, cur_refId);
    return (zl303xx_BooleanE)los;
}

int zl_3073x_cur_pfm_get()
{
    BOOL pfm = cur_pfm_failed[ref_id2clock_input((zl303xx_RefIdE)cur_refId)];
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_los: %d, clock_input %d", pfm, cur_refId);
    return (zl303xx_BooleanE)pfm;
}

int zl_3073x_cur_lock_status_get()
{
    if (cur_locked) return ZL303XX_LOCK_STATUS_LOCKED;
    if (cur_holdover) return ZL303XX_LOCK_STATUS_HOLDOVER;
    if (cur_ref_failed) return ZL303XX_LOCK_STATUS_REF_FAILED;

    return ZL303XX_LOCK_STATUS_ACQUIRING;
}

mesa_rc zl_3073x_clock_losx_state_get(bool *const state)
{
    zl303xx_BooleanE val = (zl303xx_BooleanE)true;
    //ZL_3073X_CHECK(zl303xx_DpllCurRefFailStatusGet(zl_3073x_zl303xx_synce_dpll, SYNCE_DPLL_ID, &val));
    *state = (bool) val;
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_losx: %d", val);
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_clock_lol_state_get(bool *const state)
{
    *state = (bool) cur_ref_failed;
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_lol_state: %d", cur_ref_failed);
    return(VTSS_RC_OK);
}

// state set to false means there is synce holdover or synce output cannot be used.
// synce output should not be used when dpll state is aquiring or holdover.
mesa_rc zl_3073x_clock_dhold_state_get(bool *const state)
{
    // the holdover and locked bits are stichy in the DPLL chip, this means that it cannot be read 25 ms after the latest read.
    // therefore the register is only read in the event poll function, and the status is saved for other use.
    if ((cur_locked == ZL303XX_DPLL_LOCK_FALSE && cur_holdover == ZL303XX_DPLL_HOLD_FALSE) || (cur_holdover == ZL303XX_DPLL_HOLD_TRUE)) {
        *state = false;
    } else {
        *state = true;
    }
    T_DG(TRACE_GRP_SYNC_INTF, "dhold %d", *state);
    return(VTSS_RC_OK);
}

void zl_3073x_clock_event_poll(BOOL interrupt,  clock_event_type_t *ev_mask, const uint clock_my_input_max)
{
    zl303xx_BooleanE refFailed = (zl303xx_BooleanE)cur_ref_failed;
    zl303xx_RefIsrStatusS ref_par;
    zl303xx_DpllStatusS status_par;
    status_par.Id = SYNCE_DPLL_ID;
    static clock_event_type_t old_mask = 0x0;
    clock_event_type_t new_mask = 0x0;
    uint source;
    *ev_mask = 0;

    // Get the currently selected reference
    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
    zl303xx_Dpll73xCurrRefGet(zl_3073x_zl303xx_synce_dpll, &cur_refId);
    for (source = 0; source < clock_my_input_max; ++source) {
        ref_par.Id = (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(source);
        ZL_3073X_CHECK(zl3073x_RefIsrStatusGet(zl_3073x_zl303xx_synce_dpll, &ref_par));
        if (ref_par.refFail != cur_locs_failed[source]) {
            *ev_mask |= CLOCK_LOCS1_EV | CLOCK_LOCS2_EV;
            cur_locs_failed[source] = ref_par.refFail;
        }
        cur_pfm_failed[source] = ref_par.pfmFail;
        // If reference is also the currently selected one then update refFailed.
        if (ref_par.Id == cur_refId) {
            refFailed = ref_par.refFail;
        } else if ((cur_refId == ZL303XX_REF_AUTO) && (cur_selected == ref_par.Id)) {
            refFailed = ref_par.refFail;
        }
        T_DG(TRACE_GRP_SYNC_INTF,"ref %d, refFail %d, scmFail %d, cfmFail %d, pfmFail %d, gstFail %d hw_ref_selected %d", ref_par.Id, ref_par.refFail, ref_par.scmFail, ref_par.cfmFail, ref_par.pfmFail, ref_par.gstFail, cur_refId);
    }
    //ZL_3073X_CHECK(zl303xx_DpllHoldLockStatusGet(zl_3073x_zl303xx_synce_dpll, &status_par));
    status_par.locked = ZL303XX_DPLL_LOCK_FALSE;
    status_par.holdover = ZL303XX_DPLL_HOLD_FALSE;
    ZL_3073X_CHECK(zl303xx_Ref73xDpllIsrStatusGet(zl_3073x_zl303xx_synce_dpll, &status_par));
    if (status_par.locked != cur_locked || status_par.holdover != cur_holdover || refFailed != cur_ref_failed) {
        *ev_mask |= CLOCK_LOL_EV;
        cur_locked = status_par.locked;
        cur_holdover = status_par.holdover;
        cur_ref_failed = refFailed;
    }
    T_DG(TRACE_GRP_SYNC_INTF,"locked %d, holdover %d, refFailed %d", status_par.locked, status_par.holdover, refFailed);
    T_NG(TRACE_GRP_SYNC_INTF,"old_ev: %x, cur_ev %x", old_mask, new_mask);

    T_NG(TRACE_GRP_SYNC_INTF,"interrupt: %d, ev_mask %x", interrupt, *ev_mask);

    ref_par.Id = ZL303XX_REF_ID_5;
    ZL_3073X_CHECK(zl3073x_RefIsrStatusGet(zl_3073x_zl303xx_ptp_dpll, &ref_par));
    T_DG(TRACE_GRP_SYNC_INTF,"ptp dpll ref %d, refFail %d, scmFail %d, cfmFail %d, pfmFail %d, gstFail %d ", ref_par.Id, ref_par.refFail, ref_par.scmFail, ref_par.cfmFail, ref_par.pfmFail, ref_par.gstFail);
    status_par.locked = ZL303XX_DPLL_LOCK_FALSE;
    status_par.holdover = ZL303XX_DPLL_HOLD_FALSE;
    ZL_3073X_CHECK(zl303xx_Ref73xDpllIsrStatusGet(zl_3073x_zl303xx_ptp_dpll, &status_par));
    T_DG(TRACE_GRP_SYNC_INTF,"ptp dpll locked %d, holdover %d, ", status_par.locked, status_par.holdover);
}

void zl_3073x_clock_event_enable(clock_event_type_t ev_mask, const uint clock_my_input_max)
{
    zl303xx_DpllIsrConfigS dpll_par;
    zl303xx_RefIsrConfigS ref_par;
    //uint source;

    T_DG(TRACE_GRP_SYNC_INTF,"enable ev_mask %x", ev_mask);
    //return;
    //for (source = 0; source < clock_my_input_max; ++source) {
    /* Enabling interrupts on all the input references */
    for (int i = 0; i < ZL303XX_DPLL_NUM_REFS-1; i++) {
        T_DG(TRACE_GRP_SYNC_INTF,"enable ev_mask %x on ref %u", ev_mask,i);
        //ref_par.Id = (zl303xx_RefIdE)zl_3073x_clock_input2ref_id(source);
        ref_par.Id = (zl303xx_RefIdE)i;
        zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
        /* pr reference events */
        ZL_3073X_CHECK(zl3073x_RefIsrConfigGet(zl_3073x_zl303xx_synce_dpll, &ref_par));
        if (ev_mask & (CLOCK_LOCS1_EV | CLOCK_LOCS2_EV)) {
            ref_par.refIsrEn = ZL303XX_TRUE;
            ref_par.scmIsrEn = ZL303XX_TRUE;
            ref_par.cfmIsrEn = ZL303XX_TRUE;
            ref_par.pfmIsrEn = ZL303XX_TRUE;
            ref_par.gstIsrEn = ZL303XX_TRUE;
        }
        if (ev_mask & (CLOCK_FOS1_EV | CLOCK_FOS1_EV)) {
            /* TBD */
        }
        ZL_3073X_CHECK(zl3073x_RefIsrConfigSet(zl_3073x_zl303xx_synce_dpll, &ref_par));
    }

    /* Enabling interrupts on SYNCE DPLL */
    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
    dpll_par.Id = SYNCE_DPLL_ID;
    ZL_3073X_CHECK(zl303xx_Ref73xDpllIsrMaskGet(zl_3073x_zl303xx_synce_dpll, &dpll_par));
//    if (ev_mask & CLOCK_LOSX_EV) {                                                // FIXME: The zl30363 does not support the generation of interrupts upon "DPLL Locked"
//        dpll_par.lockIsrEn = ZL303XX_TRUE;                                        //        Therefore in a "Loss of Lock" condition we need to poll the chip to determine
//    }                                                                             //        when the "Loss of Lock" condition is gone and then reenable its interrupt.
    if (ev_mask & CLOCK_LOL_EV) {
        dpll_par.lostLockIsrEn = ZL303XX_TRUE;
        dpll_par.lockIsrEn     = ZL303XX_TRUE;
        dpll_par.holdoverIsrEn = ZL303XX_TRUE;
    }
    ZL_3073X_CHECK(zl303xx_Ref73xDpllIsrMaskSet(zl_3073x_zl303xx_synce_dpll, &dpll_par));
}

mesa_rc zl_3073x_station_clk_out_freq_set(const u32 freq_khz)
{
    // station clock is Synthesizer1, HPOUT4P (depending on board design)
    uint synthId = 1;
    uint outId = 0;

    /* Disable synthesizer-1 ,while modifying divider settings */
    ZL_3073X_CHECK(zl303xx_Ref73xSynthEnableSet(zl_3073x_zl303xx_synce_dpll, synthId, ZL303XX_FALSE));
    /* configure synthesizer-1 frequency to it default 3.75 GHz */
    ZL_3073X_CHECK(zl303xx_Ref73xSynthBaseFreqSet(zl_3073x_zl303xx_synce_dpll, synthId, 0xDF847580));
    ZL_3073X_CHECK(zl303xx_Ref73xSynthRatioMRegSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x1));
    ZL_3073X_CHECK(zl303xx_Ref73xSynthRatioNRegSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x1));
    if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_SPARX5 ||
        fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X ||
        fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN969X) {
        outId  = 6;

         /*  using HPOUT3P, which is connected to fractional clock divider
         */
        if (freq_khz != 0) {
            if (vtss_board_type() == VTSS_BOARD_FIREANT_PCB134_REF) {
                switch (freq_khz) {
                    case 1544:
                        /* fractional div = 154.4Mhz */
                        ZL_3073X_CHECK(zl303xx_Ref73xSynthFracDivBaseSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x0933f500));
                        /* configure medium speed clock divider 100 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x64));
                        /* configure low speed clock divider    1 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x1));
                        break;
                    case 2048:
                        /* fractional div = 154.4Mhz */
                        ZL_3073X_CHECK(zl303xx_Ref73xSynthFracDivBaseSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x0c350000));
                        /* configure medium speed clock divider 100 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x64));
                        /* configure low speed clock divider     1 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x1));
                        break;
                    case 10000:
                        ZL_3073X_CHECK(zl303xx_Ref73xSynthHsDivSet(zl_3073x_zl303xx_synce_dpll, synthId,0x4 ));
                        /* fractional div = 200Mhz */
                        ZL_3073X_CHECK(zl303xx_Ref73xSynthFracDivBaseSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x0bebc200));
                        /* configure medium speed clock divider 20*/
                        ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x14));
                        /* configure low speed clock divider 1*/
                        ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x1));
                        break;
                    default: T_W("Unsupported output frequency for station clock (frequency not set)");
                }
            } else if (vtss_board_type() == VTSS_BOARD_FIREANT_PCB135_REF || vtss_board_type() == VTSS_BOARD_LAN9668_8PORT_REF) {
                /* PCB-135 uses integer divider. Integer divider output is 500Mhz. This must be divided by
                   configured values in medium speed and low speed dividers to obtain expected output.*/
                switch(freq_khz) {
                    case 1544:
                        /* 500 * 1000 / 1544 = 323.83; 323 = 19 * 17 */
                        /* configure medium speed clock divider 19 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x13));
                        /* configure low speed clock divider 17 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x11));
                        break;
                    case 2048:
                        /* 500 * 1000 / 2048 = 244.1406; 244 = 61 * 4 */
                        /* configure medium speed clock divider 61 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x3d));
                        /* configure low speed clock divider 4 */
                        ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x4));
                        break;
                    case 10000:
                        /* 500 / 10 = 50 */
                        /* configure medium speed clock divider 50*/
                        ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x32));
                        /* configure low speed clock divider 1*/
                        ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x1));
                        break;
                    default: T_W("Unsupported output frequency for station clock (frequency not set)");
                }
            }
        } else {
            /* Restore default values. */
            if (vtss_board_type() == VTSS_BOARD_FIREANT_PCB134_REF) {
                ZL_3073X_CHECK(zl303xx_Ref73xSynthHsDivSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x4));
                ZL_3073X_CHECK(zl303xx_Ref73xSynthFracDivBaseSet(zl_3073x_zl303xx_synce_dpll, synthId, 0x0ee6b280));
            }
        }
    } else {
    /*
       HPOUT4P integer clock divider is enabled , which is
       set to => 4(bit field value) =>(maps to) 6(actual value)
       Resultant freq = 3.75Ghz / 6 = 625Mhz
     */
        outId = 8;
        if (freq_khz != 0) {
            switch (freq_khz) {
                case 1544:
                    /* below setting produces 625Mhz/(50*12) = 1.04Mhz */
                    /* configure medium speed clock divider 50 */
                    ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x32));
                    /* configure low speed clock divider    12 */
                    ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0xC));
                    break;
                case 2048:
                    /* below setting produces 625Mhz/(50*6) = 2.08Mhz */
                    /* configure medium speed clock divider 50 */
                    ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x32));
                    /* configure low speed clock divider     6 */
                    ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x6));
                    break;
                case 10000:
                    /* below setting produces 625Mhz/50 = 12.5Mhz */
                    /* configure medium speed clock divider 50*/
                    ZL_3073X_CHECK(zl303xx_Ref73xHpMsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x32));
                    /* configure low speed clock divider 1*/
                    ZL_3073X_CHECK(zl303xx_Ref73xHpLsDiv(zl_3073x_zl303xx_synce_dpll, outId, 0x1));
                    break;
                default: T_W("Unsupported output frequency for station clock (frequency not set)");
            }
        }
    }
    /* Re-enable synthesizer-1 ,after modifying divider settings */
    ZL_3073X_CHECK(zl303xx_Ref73xSynthEnableSet(zl_3073x_zl303xx_synce_dpll, synthId, ZL303XX_TRUE));
    T_IG(TRACE_GRP_SYNC_INTF,"Station clock out freq: %d kHz synthesizer %u output id %u", freq_khz,synthId,outId);
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_station_clk_in_freq_set(const u32 freq_khz)
{
    return zl_3073x_ref_clk_in_freq_set(STATION_CLOCK_SOURCE_NO, freq_khz);
}

mesa_rc zl_3073x_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    int refId = zl_3073x_clock_input2ref_id(source);
    T_IG(TRACE_GRP_SYNC_INTF, "Configuring for freq %d khz using source %d\n", freq_khz, source);
    return zl_3073x_ref_input_freq_set(refId, freq_khz);
}

mesa_rc zl_3073x_ref_input_freq_set(const uint32_t ref, const u32 freq_khz)
{
    u16 Br, Kr, Mr, Nr;
    zl303xx_RefIdE refId = (zl303xx_RefIdE)ref;
    switch (freq_khz) {
        case 1544:
            /* initialize reference 7 for 1.544 MHZ input */
            Br = 8000;
            Kr = 193;
            Mr = 1;
            Nr = 1;
            break;
        case 2048:
            /* initialize reference for 2.048 MHZ input */
            Br = 8000;
            Kr = 256;
            Mr = 1;
            Nr = 1;
            break;
        case 10000:
            /* initialize reference for 10 MHZ input */
            Br = 25000;
            Kr = 400;
            Mr = 1;
            Nr = 1;
            break;
        case 25000:
            /* initialize reference for 25 MHZ input */
            Br = 25000;
            Kr = 1000;
            Mr = 1;
            Nr = 1;
            break;
        case 31250:
            /* initialize reference for 31.25 MHZ input */
            Br = 15625;
            Kr = 20000;
            Mr = 1;
            Nr = 10;
            break;
        case 32226:
            /* initialize reference for 32.226 MHZ input (actually 32.2265625 to be precise) */
            Br = 15625;
            Kr = 20625;
            Mr = 1;
            Nr = 10;
            break;
        case 39062:
            Br = 15625;
            Kr = 10000;
            Mr = 1;
            Nr = 4;
            break;
        case 40283:
            Br = 15625;
            Kr = 20625;
            Mr = 1;
            Nr = 8;
            break;
        case 60606:
            /* initialize reference for 60.606 MHZ input */
            Br = 25000;
            Kr = 5000;
            Mr = 32;
            Nr = 66;
            break;
        case 62500:
            /* initialize reference for 62.500 MHZ input */
            Br = 25000;
            Kr = 5000;
            Mr = 1;
            Nr = 2;
            break;
        case 78125:
            /* initialize reference for 78.125 MHZ input */
            Br = 15625;
            Kr = 1000;
            Mr = 5;
            Nr = 1;
            break;
        case 80566:
            /* initialize reference for 80.565 MHZ input (actually 80.56640625 to be precise) */
            Br = 15625;
            Kr = 20625;
            Mr = 1;
            Nr = 4;
            break;
        case 125000:
            /* initialize reference for 125 MHZ input */
            Br = 25000;
            Kr = 5000;
            Mr = 1;
            Nr = 1;
            break;
        case 156250:
            /* initialize reference for 156.25 MHZ input */
            Br = 10000;
            Kr = 15625;
            Mr = 1;
            Nr = 1;
            break;
        case 161130:
            /* initialize reference for 161.13 MHZ input (actually 161.1328125 to be precise) */
            Br = 15625;
            Kr = 20625;
            Mr = 1;
            Nr = 2;
            break;
        case 312500:
            Br = 15625;
            Kr = 20000;
            Mr = 1;
            Nr = 1;
            break;
        case 322265:
            /* initialize reference for 322.265 MHZ input */
            Br = 25000;
            Kr = 12890;
            Mr = 1;
            Nr = 1;
            break;
        default:
            /* The reference can not be disabled, so we leave it as it is */
            T_IG(TRACE_GRP_SYNC_INTF,"clock input: disable ?");
            return MESA_RC_ERROR;
    }

    ZL_3073X_CHECK(zl303xx_Ref73xBaseFreqSet(zl_3073x_zl303xx_synce_dpll, refId, Br));
    ZL_3073X_CHECK(zl303xx_Ref73xFreqMultipleSet(zl_3073x_zl303xx_synce_dpll, refId, Kr));
    ZL_3073X_CHECK(zl303xx_Ref73xRatioMRegSet(zl_3073x_zl303xx_synce_dpll, refId, Mr));
    ZL_3073X_CHECK(zl303xx_Ref73xRatioNRegSet(zl_3073x_zl303xx_synce_dpll, refId, Nr));
    T_IG(TRACE_GRP_SYNC_INTF,"clock input: Enable ref %d for %d kHz", refId, (Br * Kr * Mr / Nr + 500) / 1000);

    return MESA_RC_OK;
}

mesa_rc zl_3073x_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    /* set DPLL bandwidth to 0,1 HZ (EEC2) or 3.5 Hz (EEC1 option) */
    /* The value 0x92 corresponds roughly to 3.5Hz and the value 0x60 corresponds to 0.1 Hz (see datasheet) */
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthCustomSet(zl_3073x_zl303xx_synce_dpll, (clock_eec_option == CLOCK_EEC_OPTION_1)? 0x92 : 0x60));
    /* set phase slope limit to  885 ns/s in option 2 and 7500ns/s  option 1 */
    ZL_3073X_CHECK(zl303xx_Ref73xDpllPhaseSlopeLimitSet(zl_3073x_zl303xx_synce_dpll, SYNCE_DPLL_ID, clock_eec_option == CLOCK_EEC_OPTION_1 ? 7500: 885));  // The value 2 corresponds to 0.885 us/s (see datasheet)

    // Bandwidth of synce dpll and nco assist dpll should fall in same range.
    // For eec option1, nco assist dpll bandwidth is set to 1.7Hz instead of 3.5Hz to support more accurate holdover in 8275 profile.
    // For eec option2, nco assist dpll bandwidth is set to 80mHz instead of 100mHz to support more accurate holdover in 8275 profile.
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthCustomSet(zl_3073x_zl303xx_nco_asst_dpll, (clock_eec_option == CLOCK_EEC_OPTION_1)? 0x87 : 92));
    T_IG(TRACE_GRP_SYNC_INTF,"clock_eec_option %d", clock_eec_option);
    return(VTSS_RC_OK);
}

mesa_rc zl_3073x_selector_map_set(const uint reference, const uint clock_input)
{
    const zl303xx_RefIdE ref_map [] = {
        ZL303XX_REF_ID_0, ZL303XX_REF_ID_1, ZL303XX_REF_ID_2, ZL303XX_REF_ID_3,
        ZL303XX_REF_ID_4, ZL303XX_REF_ID_5, ZL303XX_REF_ID_6, ZL303XX_REF_ID_7,
        ZL303XX_REF_ID_8, ZL303XX_REF_ID_9};

    T_IG(TRACE_GRP_SYNC_INTF,"clock_selector_map: ref %d, clock_input %d", reference, clock_input);
    if (reference < sizeof(ref_map)/sizeof(zl303xx_RefIdE) && clock_input < CLOCK_INPUT_MAX) {
        cur_ref[clock_input] = ref_map[reference];
        return(VTSS_RC_OK);
    } else {
        return(VTSS_RC_ERROR);
    }
}

mesa_rc zl_3073x_adj_phase_set(i32 adj)
{
    T_EG(TRACE_GRP_SYNC_INTF,"To be implemented");
    mesa_rc rc = VTSS_RC_OK;
#if 0
    if (labs(adj) >= (1<<16)/10) {  // TBD what the phase step granularity is
        T_DG(TRACE_GRP_SYNC_INTF,"set phase offset dpll 0");
        ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xOutputPhaseStepWrite(zl_3073x_zl303xx_synce_dpll, PTP_DPLL_ID, adj, ZL303XX_FALSE,  /* AKA StepTime() */
                0));
        //ZL_3073X_CHECK(zl303xx_synth_adj_phase_get(&adj_ongoing));
        //if (!adj_ongoing) {
        //    clock_mask |= 1<<SYNTH_ID_PTP;
        //    T_WG(TRACE_GRP_SYNC_INTF,"set phase offset on output mask 0x%x", clock_mask);
        //    ZL_3073X_CHECK(zl303xx_synth_adj_phase_set(clock_mask, phase));
        //    T_WG(TRACE_GRP_SYNC_INTF,"phase set to: %d * 0.1 ns", ((phase >> 4) * 10) >> 12);  // Making shift by 16 in two steps as an initial shift by 4 followed by a shift by 12.
        //} else {                                                                           // If we do the complete shift by 16 at the end, (phase * 10) can overflow (phase is only 32 bits).
        //    T_WG(TRACE_GRP_SYNC_INTF,"previous phase adj not completed ");
        //}
    }
#endif
    return(rc);
}

mesa_rc zl_3073x_adjtimer_set(i64 adj)
{
    Sint32T freq_offset_uppm;
    Uint32T current_pllId;

    current_pllId = zl_3073x_zl303xx_synce_dpll->pllParams.pllId;
    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = SYNCE_DPLL_ID;
    // the frequency offset is set relative to the holdover value, saved when the NCO mode was entered.
    freq_offset_uppm = (adj*1000LL)/(1<<16) + nco_ho_freqOffsetUppm;
    ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xSetFreq(zl_3073x_zl303xx_synce_dpll, freq_offset_uppm));
    zl_3073x_zl303xx_synce_dpll->pllParams.dcoFreq = freq_offset_uppm;
    T_DG(TRACE_GRP_SYNC_INTF,"set DCO frequency %d uppm", freq_offset_uppm);
    zl_3073x_zl303xx_synce_dpll->pllParams.pllId = current_pllId;
    return VTSS_RC_OK;
}

mesa_rc zl3073x_clock_output_adjtimer_set(i64 adj)
{
    Sint32T freq_offset_uppm;
    freq_offset_uppm = (adj*1000LL)/(1<<16);

    ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xSetFreq(zl_3073x_zl303xx_ptp_dpll, freq_offset_uppm));
    T_IG(TRACE_GRP_SYNC_INTF,"set DCO frequency %d uppm", freq_offset_uppm);
    return VTSS_RC_OK;
}

// When synce is clock source, feedback clock out HPOUT0 is driven by Synce i.e synthesizer 1.
// When PTP is clock source (default), feedback clock out HPOUT0 is driven by PTP i.e synthesizer 2.
// NCO assist dpll is set to always lock to feedback clock.
mesa_rc zl3073x_clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    /* To be viried: In Azurite, PTP Dpll would always drive the PTP synthesizer.
                     This is done during initialisation. */

    zl303xx_DpllIdE dpllId = (source == PTP_CLOCK_SOURCE_SYNCE) ? SYNCE_DPLL_ID : PTP_DPLL_ID;
    T_IG(TRACE_GRP_SYNC_INTF,"zl303xx_SynthDrivePll %d source %d", dpllId, source);
    if (source == PTP_CLOCK_SOURCE_SYNCE) {
        ZL_3073X_CHECK((zlStatusE)zl303xx_Ref73xSynthDrivePll(zl_3073x_zl303xx_synce_dpll, SYNTH_ID_PTP, SYNCE_DPLL_ID));
    } else {
        ZL_3073X_CHECK((zlStatusE)zl303xx_Ref73xSynthDrivePll(zl_3073x_zl303xx_ptp_dpll, SYNTH_ID_PTP, PTP_DPLL_ID));
    }
    return MESA_RC_OK;
}

// If PTP hybrid mode is enabled, NCO assist dpll must be prepared to lock to synce reference.
// currently, set to feedback reference instead of synce reference.
// Repeating it after executing same at end of zl_3073x_clock_startup because after registering with servo, NCO assist dpll goes out of lock.
mesa_rc zl3073x_prepare_nco_assist_dpll_for_hybrid_mode(bool enable)
{
    // Set NCO assist dpll to lock to ptp feedback ref.
    ZL_3073X_CHECK(zl303xx_Dpll73xCurrRefSet(zl_3073x_zl303xx_nco_asst_dpll, ZL303XX_REF_ID_8));
    ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(zl_3073x_zl303xx_nco_asst_dpll, ZLS3073X_DPLL_MODE_REFLOCK));

    return MESA_RC_OK;
}
mesa_rc zl_3073x_clock_nco_assist_set(bool enable)
{
    mesa_rc rc = VTSS_RC_OK;
    ZLS3073X_DpllHWModeE cur_mode, ptp_mode;
    Uint32T current_ref = -1;

    /* set Association between PTP dpll and NCO assisting DPLL */
    T_IG(TRACE_GRP_SYNC_INTF,"zl_3073x_clock_nco_assist_set  %s", enable? "enable" : "disabled");
    if (enable && !nco_assist_mode_enabled) {
        ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xSetNCOAssistParamsSAssociation(zl_3073x_zl303xx_ptp_dpll,zl_3073x_zl303xx_nco_asst_dpll));
        /* Get reference input of synce_dpll and apply the same to
           nco_asst_dpll */
        ZL_3073X_CHECK(zl303xx_Dpll73xModeGet(zl_3073x_zl303xx_synce_dpll, &cur_mode));
        ZL_3073X_CHECK(zl303xx_Dpll73xCurrRefGet(zl_3073x_zl303xx_synce_dpll, &current_ref));
        T_IG(TRACE_GRP_SYNC_INTF,"SYNCE DPLL mode: %d current_ref %u", cur_mode, current_ref);

        if ((ZLS3073X_CHECK_REF_ID(current_ref) == ZL303XX_OK) &&
            (cur_mode == ZLS3073X_DPLL_MODE_REFLOCK)) {
            ZL_3073X_CHECK(zl303xx_Dpll73xCurrRefSet(zl_3073x_zl303xx_nco_asst_dpll, current_ref));
            ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(zl_3073x_zl303xx_nco_asst_dpll, cur_mode));
            ZL_3073X_CHECK(zl303xx_Dpll73xModeGet(zl_3073x_zl303xx_ptp_dpll, &ptp_mode));
            nco_assist_mode_enabled = true;
            if (ptp_mode != ZLS3073X_DPLL_MODE_NCO) {
                ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(zl_3073x_zl303xx_ptp_dpll, ZLS3073X_DPLL_MODE_NCO));
            }
            T_IG(TRACE_GRP_SYNC_INTF," NCO mode set ptp dpll prev_mode %d", ptp_mode);
        } else {
            T_IG(TRACE_GRP_SYNC_INTF,"SYNCE DPLL has not been configured as expected");
        }
    } else if (!enable) {
        nco_assist_mode_enabled = false;
        ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(zl_3073x_zl303xx_nco_asst_dpll, ZLS3073X_DPLL_MODE_HOLDOVER));
    }

    return rc;
}
#ifdef __cplusplus
extern "C" {
#endif

void zl3073x_clock_take_hw_nco_control(void)
{
    ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xTakeHwNcoControl(zl_3073x_zl303xx_ptp_dpll));
    T_DG(TRACE_GRP_SYNC_INTF, "take hw nco control");
}

void zl3073x_clock_return_hw_nco_control(void)
{
    ZL_3073X_CHECK((zlStatusE)zl303xx_Dpll73xReturnHwNcoControl(zl_3073x_zl303xx_ptp_dpll));
    T_DG(TRACE_GRP_SYNC_INTF, "return hw nco control");
}

// return true if both DPLL's have to be controlled in NCO mode.
int zl3073x_clock_return_common_control(void)
{
    // if zl3073x_adjtimer_enabled return true else return false
    return (int)zl3073x_adjtimer_enabled;
}

#ifdef __cplusplus
}
#endif

mesa_rc zl_3073x_apr_set_up_for_bc_full_on_path_phase_synce()
{
    /* configuring PTP DPLL */
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthSet(zl_3073x_zl303xx_ptp_dpll,ZLS3073X_BW_14Hz));
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthCustomSet(zl_3073x_zl303xx_ptp_dpll, 0));
    ZL_3073X_CHECK(zl303xx_Dpll73xPullInRangeSet(zl_3073x_zl303xx_ptp_dpll, 0x78));
    ZL_3073X_CHECK(zl303xx_Dpll73xPhaseSlopeLimitSet(zl_3073x_zl303xx_ptp_dpll, 0));

    /* configuring SYNCE DPLL */
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthSet(zl_3073x_zl303xx_synce_dpll, ZLS3073X_BW_custom));
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthCustomSet(zl_3073x_zl303xx_synce_dpll, 0x60));
    ZL_3073X_CHECK(zl303xx_Dpll73xPullInRangeSet(zl_3073x_zl303xx_synce_dpll, 0x78));
    ZL_3073X_CHECK(zl303xx_Dpll73xPhaseSlopeLimitSet(zl_3073x_zl303xx_synce_dpll, 0x1D4C));

    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthSet(zl_3073x_zl303xx_nco_asst_dpll, ZLS3073X_BW_custom));
    ZL_3073X_CHECK(zl303xx_Dpll73xBandwidthCustomSet(zl_3073x_zl303xx_nco_asst_dpll, 0x60));
    ZL_3073X_CHECK(zl303xx_Dpll73xPullInRangeSet(zl_3073x_zl303xx_nco_asst_dpll, 0x78));
    ZL_3073X_CHECK(zl303xx_Dpll73xPhaseSlopeLimitSet(zl_3073x_zl303xx_nco_asst_dpll, 0x1D4C));
    return VTSS_RC_OK;
}

// If 'enable is true', dpll is configured to lock to reference. Otherwise, dpll is set in NCO mode.
mesa_rc zl_3073x_dpll_force_lock_ref(uint32_t dpll, int ref, bool enable)
{
    zl303xx_ParamsS *dpll_params;

    dpll_params = (dpll == 1) ? zl_3073x_zl303xx_synce_dpll : (dpll == 2) ? zl_3073x_zl303xx_ptp_dpll : zl_3073x_zl303xx_nco_asst_dpll;

    if (enable) {
        ZL_3073X_CHECK(zl303xx_Dpll73xCurrRefSet(dpll_params, (zl303xx_RefIdE)ref));
        ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(dpll_params, ZLS3073X_DPLL_MODE_REFLOCK));
    } else {
        ZL_3073X_CHECK(zl303xx_Dpll73xModeSet(dpll_params, ZLS3073X_DPLL_MODE_NCO));
    }
    return MESA_RC_OK;
}

mesa_rc zl_3073x_1pps_ref_conf(int ref, bool enable)
{
    zl303xx_RefIdE refId = (zl303xx_RefIdE)ref; // 1pps ref on 2N for zls30732.

    /* Maximum references possible is 15. */
    if (ref > 15) {
        return MESA_RC_OK;
    }
    if (enable) {
        ZL_3073X_CHECK(zl303xx_Ref73xBaseFreqSet(zl_3073x_zl303xx_ptp_dpll, refId, 1));
        ZL_3073X_CHECK(zl303xx_Ref73xFreqMultipleSet(zl_3073x_zl303xx_ptp_dpll, refId, 1));
        ZL_3073X_CHECK(zl303xx_Ref73xRatioMRegSet(zl_3073x_zl303xx_ptp_dpll, refId, 1));
        ZL_3073X_CHECK(zl303xx_Ref73xRatioNRegSet(zl_3073x_zl303xx_ptp_dpll, refId, 1));
    } else {
        /* set to 125Mhz. */
        ZL_3073X_CHECK(zl303xx_Ref73xBaseFreqSet(zl_3073x_zl303xx_ptp_dpll, refId, 25000));
        ZL_3073X_CHECK(zl303xx_Ref73xFreqMultipleSet(zl_3073x_zl303xx_ptp_dpll, refId, 5000));
    }
    return MESA_RC_OK;
}

#endif //defined (VTSS_OPTION_SYNCE) || defined(VTSS_SW_OPTION_PTP)
