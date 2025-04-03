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

#if defined(VTSS_SW_OPTION_SYNCE) || defined(VTSS_SW_OPTION_PTP)

#include "synce_constants.h"
#include "synce_custom_clock_api.h"
#include "zl_30361_synce_clock_api.h"
#include "zl_30361_api.h"
#include "zl303xx.h"
#include "zl303xx_AddressMap36x.h"
#include "zl_30361_synce_support.h"
#include "zl303xx_Dpll36x.h"
#include "zl303xx_Dpll361.h"
#include "main.h"
#include "microchip/ethernet/switch/api.h"
#include "critd_api.h"
#include <vtss_trace_lvl_api.h>
#include <stdio.h>
#include "board_if.h"
#include "zl303xx_Params.h"
#include "zl303xx_Var.h"
#include "zl303xx_DeviceSpec.h"

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "zl_api", "ZL3036x DPLL API."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default (ZL3036x core)",
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

#define SYNTH_ID_PTP 1  // Synthesizer used for PTP

zl303xx_ParamsS *zl_3036x_zl303xx_params; /* device instance returned from zl303xx_CreateDeviceInstance*/
zl303xx_ParamsS *zl_3036x_zl303xx_synce; /* device instance returned from zl303xx_CreateDeviceInstance*/

static Uint32T cur_refId;
static BOOL cur_locked = FALSE;
static BOOL cur_holdover = FALSE;
static BOOL cur_ref_failed = FALSE;

static uint cur_pri[ZL303XX_DPLL_NUM_REFS];
static BOOL cur_locs_failed[CLOCK_INPUT_MAX] = {FALSE, FALSE, FALSE};
static BOOL cur_pfm_failed[CLOCK_INPUT_MAX] = {FALSE, FALSE, FALSE};
/* holds the current mapping from clock_input to DPLL reference number */
static zl303xx_RefIdE cur_ref[CLOCK_INPUT_MAX] = {ZL303XX_REF_ID_4, ZL303XX_REF_ID_5, ZL303XX_REF_ID_6};
static ZLS3036X_DpllModeE current_mode = ZLS3036X_DPLL_MODE_AUTO;
static zl303xx_RefIdE current_ref = ZL303XX_REF_ID_4;
static vtss_appl_synce_selection_mode_t my_selection_mode; // actual selection mode, if the DPLL is not in NCO mode
static uint my_selected_clock_input;                       // actual selected ref, if the DPLL is in manual mode 
static bool nco_mode_enabled;                              // indicates if the NCO mode is enabled
static Sint32T nco_ho_freqOffsetUppm = 0;                  // holds the actual offset at the time when the NCO mode is entered

static mesa_rc zl_3036x_apr_set_up_for_bc_full_on_path_phase_synce();
static void zl_3036x_clock_event_enable(clock_event_type_t ev_mask, const uint clock_my_input_max);
static int zl_3036x_clock_input2ref_id(uint clock_input);


static int zl_3036x_clock_input2ref_id(uint clock_input)
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

mesa_rc zl_3036x_api_init(void **device_ptr)
{
    // Since zls303xx comes in several flavors, we cannot use
    // VTSS_TRACE_REGISTER() to do a constructor-based trace initialization,
    // because that would cause an assertion when the second instance attempts
    // to register with the same module ID.
    // Therefore, we need to do it like this:
    vtss_trace_reg_init(&trace_reg, trace_grps, ARRSZ(trace_grps));
    vtss_trace_register(&trace_reg);

    ZL_3036X_CHECK(zl303xx_ReadWriteInit());
    T_D("apr_env_init finished");

    /* Create a HW Driver for SYNCE alone operations*/
    ZL_3036X_CHECK(zl303xx_CreateDeviceInstance(&zl_3036x_zl303xx_synce));
    T_D("device instance created %p", zl_3036x_zl303xx_synce);
    zl_3036x_zl303xx_synce->deviceType = ZL3036X_DEVICETYPE;
    zl_3036x_zl303xx_synce->pllParams.pllId = ZL303XX_DPLL_ID_1;
    ZL_3036X_CHECK(zl303xx_InitDeviceIdAndRev(zl_3036x_zl303xx_synce));

    /* Create a HW Driver for PTP g8265 operations*/
    ZL_3036X_CHECK(zl303xx_CreateDeviceInstance(&zl_3036x_zl303xx_params));
    *device_ptr = zl_3036x_zl303xx_params;
    T_D("device instance created %p", zl_3036x_zl303xx_params);

    zl_3036x_zl303xx_params->deviceType = ZL3036X_DEVICETYPE;
    zl_3036x_zl303xx_params->pllParams.pllId = ZL303XX_DPLL_ID_2;

    ZL_3036X_CHECK(zl303xx_InitDeviceIdAndRev(zl_3036x_zl303xx_params));
    T_I("device instance created %p", zl_3036x_zl303xx_params);
    
    

    return VTSS_RC_OK;
}

mesa_rc zl_3036x_clock_init(BOOL  cold_init)
{
    uint i;
    for (i = 0; i < ZL303XX_DPLL_NUM_REFS; i++) {
        cur_pri[i] = 0x10;
    }
    mesa_rc rc=VTSS_RC_OK;
    return(rc);
}

mesa_rc zl_3036x_clock_startup(BOOL cold_init, const uint clock_my_input_max)
{
    if (cold_init) {
        /* Set the DPLL mode to ZLS3036X_DPLL_MODE_REFLOCK which is the value of current_mode since this is a cold_init */
        zl_3036x_zl303xx_synce->pllParams.pllId = ZL303XX_DPLL_ID_1;
        ZL_3036X_CHECK(zl303xx_Dpll36xModeSet(zl_3036x_zl303xx_synce, current_mode));
        T_IG(TRACE_GRP_SYNC_INTF,"DPLL[%d] mode: changed to %d priorMode is %d", zl_3036x_zl303xx_params->pllParams.pllId, current_mode, zl_3036x_zl303xx_params->pllParams.pllPriorMode);

        /* disable all references (the DPLL has 11 references, but the API only accepts 9 (0..8)) */
        for (int refId = ZL303XX_REF_ID_0; refId <= ZL303XX_REF_ID_8; refId++) {
            zl_3036x_zl303xx_params->pllParams.pllId = ZL303XX_DPLL_ID_2;
            ZL_3036X_CHECK(zl30361_DpllRefPrioritySet(zl_3036x_zl303xx_params, ZL303XX_DPLL_ID_2, (zl303xx_RefIdE)refId, 0xf));
            T_IG(TRACE_GRP_SYNC_INTF,"ref %d, disabled", refId);
        }
    }

    // Initialize ref. 0-5 (Phy- and SFP-ports) and ref. 7 (Station Clock)
    for (int i = 0; i <= 6; i++) {
        zl303xx_RefIdE refId = (zl303xx_RefIdE)i;

        // Handle special case of the Station Clock
        if (i == 6) {
            refId = ZL303XX_REF_ID_7;
        }

        // Setup ref. for 125 MHz input except ref. 2 and 3 that are setup for 25 MHz for compatibility with the zl30343 implementation.
        ZL_3036X_CHECK(zl303xx_Ref36xBaseFreqSet(zl_3036x_zl303xx_params, refId, 25000));
        if (refId == 2 || refId == 3) {
            ZL_3036X_CHECK(zl303xx_Ref36xFreqMultipleSet(zl_3036x_zl303xx_synce, refId, 1000));
        } else {
            ZL_3036X_CHECK(zl303xx_Ref36xFreqMultipleSet(zl_3036x_zl303xx_synce, refId, 5000));
        }
        ZL_3036X_CHECK(zl303xx_Ref36xRatioMRegSet(zl_3036x_zl303xx_synce, refId, 1));
        ZL_3036X_CHECK(zl303xx_Ref36xRatioNRegSet(zl_3036x_zl303xx_synce, refId, 1));

        // Setup Single Cycle Monitor (SCM), Coarse Frequency Monitor (CFM) and Precise Frequency Monitor (PFM).
        ZL_3036X_CHECK(zl303xx_Ref36xScmLimitSet(zl_3036x_zl303xx_synce, refId, 7));  // The value 7 corresponds to a tolerance of +/-0.5 period
        ZL_3036X_CHECK(zl303xx_Ref36xCfmLimitSet(zl_3036x_zl303xx_synce, refId, 3));  // The value 3 corresponds to a tolerance of +/-2% (Note: With zl30343 a tolerance of +/-3% was used, but this is not possible with the zl30363).
        ZL_3036X_CHECK(zl303xx_Ref36xPfmLimitSet(zl_3036x_zl303xx_synce, refId, 0));  // The value 0 corresponds to a tolerance of 9-12 ppm.

        // Set the GST Disqualification Time. Note: With the zl30343 this time was 100ms, but that is not possible with the zl30343. The value 50ms is the closest value possible.
        ZL_3036X_CHECK(zl303xx_Ref36xGstDisqualifyTimeSet(zl_3036x_zl303xx_synce, refId, 2));  // The value 2 corresponds to a time of 50ms.

        // Set the GST Qualification Time to 200ms. Note: This is the same value as with the zl30343.
        ZL_3036X_CHECK(zl303xx_Ref36xGstQualifyTimeSet(zl_3036x_zl303xx_synce, refId, 1));  // The value 1 corresponds to a time of 200ms.
    }
    T_IG(TRACE_GRP_SYNC_INTF,"clock startup: Configured ref. 0,1,4,5 and 7 for 125MHz. Configured ref. 2,3 for 25MHz. Station clock at ref. 7 is disabled at startup. It should be reconfigured for other frequency before being enabled.");

    (void)zl_3036x_apr_set_up_for_bc_full_on_path_phase_synce();  // Setup PSL and bandwidth parameters as required for bc_full_on_path_phase_synce profile.

    // Configure HpOutClk2 to be a low frequency 1 Hz clock
    ZL_3036X_CHECK(zl303xx_SynthPostDivSet(zl_3036x_zl303xx_synce, 1, ZLS3036X_POST_DIV_C, 0xF2C350));

    // Enable HpOutClk2
    ZL_3036X_CHECK(zl303xx_HpCmosEnableSet(zl_3036x_zl303xx_synce, 2, 1));

    // Set phase step mask for HpOutClk2 so that the MS-PDV will use this output as a dummy output for step phase operations.
    ZL_3036X_CHECK(zl303xx_Dpll36xPhaseStepMaskSet(zl_3036x_zl303xx_synce, 1, 4));

    Uint32T dpllId;
    // DPLL_ID_1 is used for PTP, therefore it is set into NCO mode
    /* set DPLL into NCO mode */
    dpllId = zl_3036x_zl303xx_params->pllParams.pllId;
    zl_3036x_zl303xx_params->pllParams.pllId = ZL303XX_DPLL_ID_2;
    ZL_3036X_CHECK(zl303xx_Dpll36xModeSet(zl_3036x_zl303xx_params, ZLS3036X_DPLL_MODE_NCO));
    zl_3036x_zl303xx_params->pllParams.pllId = dpllId;
    T_IG(TRACE_GRP_SYNC_INTF, "DPLL %d set into NCO mode", ZL303XX_DPLL_ID_2);
    zl_3036x_zl303xx_params->pllParams.pllPriorMode = current_mode; //servo is not aware that there are more DPLL's, therefore priorMode must he set to the value for DPLL_ID_1
    /* done */

    /*TBD Irq masking */
    zl_3036x_clock_event_enable(CLOCK_LOSX_EV | CLOCK_LOL_EV | CLOCK_LOCS1_EV | CLOCK_LOCS2_EV | CLOCK_FOS1_EV | CLOCK_FOS2_EV, clock_my_input_max);

    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input)
{
    ZLS3036X_DpllModeE val = (ZLS3036X_DpllModeE)-1;
    my_selection_mode = mode;
    my_selected_clock_input = clock_input;
    if (nco_mode_enabled) {
        T_DG(TRACE_GRP_SYNC_INTF,"DPLL mode: %d, ref %d saved while in NCO mode", mode, zl_3036x_clock_input2ref_id(clock_input));
    } else {
        switch (mode)
        {
            case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL:
                val = ZLS3036X_DPLL_MODE_REFLOCK;
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED:
                // FIXME: This is an illegal value, we should return an error message.
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE:
            case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE:
                val = ZLS3036X_DPLL_MODE_AUTO;
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER:
                val = ZLS3036X_DPLL_MODE_HOLDOVER;
                break;
            case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN:
                // THE ZL30361 api does not support FREE_RUN mode. Therefore freerun is simulated as NCO with 0 frequency offset
                // ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xSetFreq(zl_3036x_zl303xx_synce, 0));
                val = ZLS3036X_DPLL_MODE_FREERUN;
                break;
            default: break;
        }
        if (val != current_mode || (val == ZLS3036X_DPLL_MODE_REFLOCK && current_ref != (zl303xx_RefIdE)zl_3036x_clock_input2ref_id(clock_input))) {
            ZLS3036X_DpllModeE cur_mode;
            current_ref = (zl303xx_RefIdE)zl_3036x_clock_input2ref_id(clock_input);
            //T_IG(TRACE_GRP_SYNC_INTF,"DPLL mode: changed to %d", val);

            ZL_3036X_CHECK(zl303xx_Dpll36xModeGet(zl_3036x_zl303xx_synce, &cur_mode));
            if (cur_mode != ZLS3036X_DPLL_MODE_NCO)
            {
                if (val == ZLS3036X_DPLL_MODE_REFLOCK) {
                    // set manually selected reference
                    ZL_3036X_CHECK(zl303xx_Dpll36xHWModeSet(zl_3036x_zl303xx_synce, val, current_ref, 0));
                } else {
                    ZL_3036X_CHECK(zl303xx_Dpll36xModeSet(zl_3036x_zl303xx_synce, val));
                }
            } else {
                zl_3036x_zl303xx_synce->pllParams.pllPriorMode = val;
                if (val == ZLS3036X_DPLL_MODE_REFLOCK) {
                    zl_3036x_zl303xx_synce->pllParams.selectedRef = current_ref;
                }
            }
            current_mode = val;
            T_IG(TRACE_GRP_SYNC_INTF,"DPLL mode: current %d changed to %d", cur_mode, val);
        }
        T_DG(TRACE_GRP_SYNC_INTF,"DPLL mode: %d, ref %d", val, zl_3036x_clock_input2ref_id(clock_input));
    }

    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_clock_selector_state_get(vtss_appl_synce_selector_state_t *const selector_state,
                                 uint                    *const clock_input)
{
    zl303xx_BooleanE refFailed = (zl303xx_BooleanE)false;
    zl303xx_DpllStatusS par;
    ZLS3036X_DpllModeE mode;
    Sint32T freqOffsetUppm;
    par.Id = ZL303XX_DPLL_ID_1;
    // the holdover and locked bits are stichy in the DPLL chip, this means that it cannot be read 25 ms after the latest read.
    // therefore the register is only read in the event poll function, and the status is saved for other use.
    par.holdover = cur_holdover ? ZL303XX_DPLL_HOLD_TRUE : ZL303XX_DPLL_HOLD_FALSE;
    par.locked = cur_locked ? ZL303XX_DPLL_LOCK_TRUE : ZL303XX_DPLL_LOCK_FALSE;
    zl_3036x_zl303xx_synce->pllParams.pllId = ZL303XX_DPLL_ID_1;
    ZL_3036X_CHECK(zl303xx_Dpll36xModeGet(zl_3036x_zl303xx_synce, &mode));
    // read actual holdover value
    ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xGetFreq(zl_3036x_zl303xx_synce, &freqOffsetUppm, ZLS3036X_AMEM));

    if (mode == ZLS3036X_DPLL_MODE_NCO) {
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
        if (mode == ZLS3036X_DPLL_MODE_FREERUN) {
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
    T_IG(TRACE_GRP_SYNC_INTF,"selector state: %d, ref %d, selection Mode %d", *selector_state, cur_refId, mode);
    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_clock_los_get(const uint clock_input, bool *const los)
{
    *los = (bool) cur_locs_failed[clock_input];
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_los: %d, clock_input %d", *los, zl_3036x_clock_input2ref_id(clock_input));
    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_clock_losx_state_get(bool *const state)
{
    zl303xx_BooleanE val = (zl303xx_BooleanE)true;
//    ZL_3036X_CHECK(zl303xx_DpllCurRefFailStatusGet(zl_3036x_zl303xx_params, ZL303XX_DPLL_ID_2, &val));
    *state = (bool) val;
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_losx: %d", val);
    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_clock_lol_state_get(bool *const state)
{
    *state = (bool) cur_ref_failed;
    T_DG(TRACE_GRP_SYNC_INTF,"Clock_lol_state: %d", cur_ref_failed);
    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_clock_dhold_state_get(bool *const state)
{
    zl303xx_BooleanE refFailed = (zl303xx_BooleanE)false;

    // the holdover and locked bits are stichy in the DPLL chip, this means that it cannot be read 25 ms after the latest read.
    // therefore the register is only read in the event poll function, and the status is saved for other use.
    if (cur_locked == ZL303XX_DPLL_LOCK_FALSE && cur_holdover == ZL303XX_DPLL_HOLD_FALSE && refFailed == ZL303XX_FALSE) {
        *state = false;
    } else {
        *state = true;
    }
    return(VTSS_RC_OK);
}

void zl_3036x_clock_event_poll(BOOL interrupt,  clock_event_type_t *ev_mask, const uint clock_my_input_max)
{
    zl303xx_BooleanE refFailed = (zl303xx_BooleanE)cur_ref_failed;
    zl303xx_RefIsrStatusS ref_par;
    zl303xx_DpllStatusS status_par;
    status_par.Id = ZL303XX_DPLL_ID_1;
    static clock_event_type_t old_mask = 0x0;
    clock_event_type_t new_mask = 0x0;
    uint source;
    *ev_mask = 0;

    // Get the currently selected reference
    zl_3036x_zl303xx_synce->pllParams.pllId = ZL303XX_DPLL_ID_1;
    ZL_3036X_CHECK(zl303xx_Dpll36xCurrRefGet(zl_3036x_zl303xx_synce, &cur_refId));
    for (source = 0; source < clock_my_input_max; ++source) {
        ref_par.Id = (zl303xx_RefIdE)zl_3036x_clock_input2ref_id(source);
        ZL_3036X_CHECK(zl30361_RefIsrStatusGet(zl_3036x_zl303xx_synce, &ref_par));
        if (ref_par.refFail != cur_locs_failed[source]) {
            *ev_mask |= CLOCK_LOCS1_EV | CLOCK_LOCS2_EV;
            cur_locs_failed[source] = ref_par.refFail;
        }
        cur_pfm_failed[source] = ref_par.pfmFail;
        // If reference is also the currently selected one then update refFailed.
        if (ref_par.Id == cur_refId) {
            refFailed = ref_par.refFail;
        }
        T_IG(TRACE_GRP_SYNC_INTF,"ref %d, refFail %d, scmFail %d, cfmFail %d, pfmFail %d, gstFail %d", ref_par.Id, ref_par.refFail, ref_par.scmFail, ref_par.cfmFail, ref_par.pfmFail, ref_par.gstFail);
    }
    status_par.locked = ZL303XX_DPLL_LOCK_FALSE;
    status_par.holdover = ZL303XX_DPLL_HOLD_FALSE;
    ZL_3036X_CHECK(zl303xx_DpllHoldLockStatusGet(zl_3036x_zl303xx_synce, &status_par));
    if (status_par.locked != cur_locked || status_par.holdover != cur_holdover || refFailed != cur_ref_failed) {
        *ev_mask |= CLOCK_LOL_EV;
        cur_locked = status_par.locked;
        cur_holdover = status_par.holdover;
        cur_ref_failed = refFailed;
    }
    T_IG(TRACE_GRP_SYNC_INTF,"locked %d, holdover %d, refFailed %d", status_par.locked, status_par.holdover, refFailed);
    T_NG(TRACE_GRP_SYNC_INTF,"old_ev: %x, cur_ev %x", old_mask, new_mask);

    T_NG(TRACE_GRP_SYNC_INTF,"interrupt: %d, ev_mask %x", interrupt, *ev_mask);
}

static void zl_3036x_clock_event_enable(clock_event_type_t ev_mask, const uint clock_my_input_max)
{
    // zl303xx_DpllIsrConfigS dpll_par;
    // zl303xx_RefIsrConfigS ref_par;
    // uint source;

    T_DG(TRACE_GRP_SYNC_INTF,"enable ev_mask %x", ev_mask);
    return;
//     for (source = 0; source < clock_my_input_max; ++source) {
//         ref_par.Id = (zl303xx_RefIdE)zl_3036x_clock_input2ref_id(source);
//         /* pr reference events */
//         ZL_3036X_CHECK(zl30361_RefIsrConfigGet(zl_3036x_zl303xx_params, &ref_par));
//         if (ev_mask & (CLOCK_LOCS1_EV | CLOCK_LOCS2_EV)) {
//             ref_par.refIsrEn = ZL303XX_TRUE;
//             ref_par.scmIsrEn = ZL303XX_TRUE;
//             ref_par.cfmIsrEn = ZL303XX_TRUE;
//             ref_par.pfmIsrEn = ZL303XX_TRUE;
//             ref_par.gstIsrEn = ZL303XX_TRUE;
//         }
//         if (ev_mask & (CLOCK_FOS1_EV | CLOCK_FOS1_EV)) {
//             /* TBD */
//         }
//         ZL_3036X_CHECK(zl30361_RefIsrConfigSet(zl_3036x_zl303xx_params, &ref_par));
//     }

//     /* pr DPLL events */
//     dpll_par.Id = ZL303XX_DPLL_ID_2;
//     ZL_3036X_CHECK(zl303xx_DpllIsrMaskGet(zl_3036x_zl303xx_params, &dpll_par));
//     if (ev_mask & CLOCK_LOSX_EV) {                                                // FIXME: The zl30363 does not support the generation of interrupts upon "DPLL Locked"
//         dpll_par.lockIsrEn = ZL303XX_TRUE;                                        //        Therefore in a "Loss of Lock" condition we need to poll the chip to determine
//     }                                                                             //        when the "Loss of Lock" condition is gone and then reenable its interrupt.
//     if (ev_mask & CLOCK_LOL_EV) {
//         dpll_par.lostLockIsrEn = ZL303XX_TRUE;
//     }
//     ZL_3036X_CHECK(zl303xx_DpllIsrMaskSet(zl_3036x_zl303xx_params, &dpll_par));
}

mesa_rc zl_3036x_station_clk_out_freq_set(const u32 freq_khz)
{
    // station clock is either Synthesizer3, hpoutclk7 or Synthesizer2, hpoutclk4 (depending on board design)
    uint synthId[] = {3,2};
    uint divId[] = {3,2};
    uint outId[] = {7,4};
    for (int i = 0; i < sizeof(synthId)/sizeof(uint); i++) {
        if (freq_khz != 0) {
            switch (freq_khz) {
                case 1544: ZL_3036X_CHECK(zl303xx_SynthBaseFreqSet(zl_3036x_zl303xx_params, synthId[i], 8000));
                           ZL_3036X_CHECK(zl303xx_SynthFreqMultipleSet(zl_3036x_zl303xx_params, synthId[i], 9650));
                           ZL_3036X_CHECK(zl303xx_SynthPostDivSet(zl_3036x_zl303xx_params, synthId[i], divId[i], 800));
                           break;
                case 2048: ZL_3036X_CHECK(zl303xx_SynthBaseFreqSet(zl_3036x_zl303xx_params, synthId[i], 8000));
                           ZL_3036X_CHECK(zl303xx_SynthFreqMultipleSet(zl_3036x_zl303xx_params, synthId[i], 9984));
                           ZL_3036X_CHECK(zl303xx_SynthPostDivSet(zl_3036x_zl303xx_params, synthId[i], divId[i], 624));
                           break;
                case 10000: ZL_3036X_CHECK(zl303xx_SynthBaseFreqSet(zl_3036x_zl303xx_params, synthId[i], 25000));
                            ZL_3036X_CHECK(zl303xx_SynthFreqMultipleSet(zl_3036x_zl303xx_params, synthId[i], 3125));
                            ZL_3036X_CHECK(zl303xx_SynthPostDivSet(zl_3036x_zl303xx_params, synthId[i], divId[i], 125));
                            break;
                default: T_W("Unsupported output frequency for station clock (frequency not set)");
            }
        }
        ZL_3036X_CHECK(zl303xx_HpCmosEnableSet(zl_3036x_zl303xx_params, outId[i], (freq_khz != 0)));
    }
    T_IG(TRACE_GRP_SYNC_INTF,"Station clock out freq: %d kHz", freq_khz);
    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    u16 Br, Kr, Mr, Nr;
    zl303xx_RefIdE refId = (zl303xx_RefIdE)zl_3036x_clock_input2ref_id(source);
    
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
        case 80565:
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
        case 32226:
            /* initialize reference for 32.226 MHZ input (actually 32.2265625 to be precise) */
            Br = 15625;
            Kr = 20625;
            Mr = 1;
            Nr = 10;
            break;
        case 31250:
            /* initialize reference for 31.25 MHZ input */
            Br = 15625;
            Kr = 20000;
            Mr = 1;
            Nr = 10;
            break;
        default:
            /* The reference can not be disabled, so we leave it as it is */
            T_IG(TRACE_GRP_SYNC_INTF,"clock input: disable ?");
            return MESA_RC_ERROR;
    }

    ZL_3036X_CHECK(zl303xx_Ref36xBaseFreqSet(zl_3036x_zl303xx_params, refId, Br));
    ZL_3036X_CHECK(zl303xx_Ref36xFreqMultipleSet(zl_3036x_zl303xx_params, refId, Kr));
    ZL_3036X_CHECK(zl303xx_Ref36xRatioMRegSet(zl_3036x_zl303xx_params, refId, Mr));
    ZL_3036X_CHECK(zl303xx_Ref36xRatioNRegSet(zl_3036x_zl303xx_params, refId, Nr));
    T_IG(TRACE_GRP_SYNC_INTF,"clock input: Enable ref %d for %d kHz", refId, (Br * Kr * Mr / Nr + 500) / 1000);

    return MESA_RC_OK;  
}

mesa_rc zl_3036x_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    /* set DPLL bandwidth to 0,1 HZ (EEC2) or 3.5 Hz (EEC1 option) */
    ZL_3036X_CHECK(zl303xx_DpllVariableLowBandwidthSelectSet(zl_3036x_zl303xx_synce, ZL303XX_DPLL_ID_1, clock_eec_option == CLOCK_EEC_OPTION_1 ? 0x91 : 0x60));  /* The value 0x91 corresponds roughly to 3.5Hz and the value 0x60 corresponds to 0.1 Hz (see datasheet) */
    /* set phase slope limit to  0.885 us/s in option 1 or  option 2 */
    ZL_3036X_CHECK(zl303xx_DpllPhaseSlopeLimitSet(zl_3036x_zl303xx_synce, ZL303XX_DPLL_ID_1, 2));  // The value 2 corresponds to 0.885 us/s (see datasheet)
    T_IG(TRACE_GRP_SYNC_INTF,"clock_eec_option %d", clock_eec_option);
    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_selector_map_set(const uint reference, const uint clock_input)
{
    const zl303xx_RefIdE ref_map [] = {
        ZL303XX_REF_ID_0, ZL303XX_REF_ID_1, ZL303XX_REF_ID_2, ZL303XX_REF_ID_3,
        ZL303XX_REF_ID_4, ZL303XX_REF_ID_5, ZL303XX_REF_ID_6, ZL303XX_REF_ID_7,
        ZL303XX_REF_ID_8};

    T_IG(TRACE_GRP_SYNC_INTF,"clock_selector_map: ref %d, clock_input %d", reference, clock_input);
    if (reference < sizeof(ref_map)/sizeof(zl303xx_RefIdE) && clock_input < CLOCK_INPUT_MAX) {
        cur_ref[clock_input] = ref_map[reference];
      return(VTSS_RC_OK);

    } else {
        return(VTSS_RC_ERROR);
    }
}

mesa_rc zl_3036x_adj_phase_set(i32 adj)
{
    T_EG(TRACE_GRP_SYNC_INTF,"To be implemented");
    mesa_rc rc = VTSS_RC_OK;
    if (labs(adj) >= (1<<16)/10) {  // TBD what the phase step granularity is
        T_DG(TRACE_GRP_SYNC_INTF,"set phase offset dpll 0");
        ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xOutputPhaseStepWrite(zl_3036x_zl303xx_synce, ZL303XX_DPLL_ID_1, adj, ZL303XX_FALSE,  /* AKA StepTime() */
                0));
        //ZL_3036X_CHECK(zl303xx_synth_adj_phase_get(&adj_ongoing));
        //if (!adj_ongoing) {
        //    clock_mask |= 1<<SYNTH_ID_PTP;
        //    T_WG(TRACE_GRP_SYNC_INTF,"set phase offset on output mask 0x%x", clock_mask);
        //    ZL_3036X_CHECK(zl303xx_synth_adj_phase_set(clock_mask, phase));
        //    T_WG(TRACE_GRP_SYNC_INTF,"phase set to: %d * 0.1 ns", ((phase >> 4) * 10) >> 12);  // Making shift by 16 in two steps as an initial shift by 4 followed by a shift by 12.
        //} else {                                                                           // If we do the complete shift by 16 at the end, (phase * 10) can overflow (phase is only 32 bits).
        //    T_WG(TRACE_GRP_SYNC_INTF,"previous phase adj not completed ");
        //}
    }
    return(rc);
}

mesa_rc zl_3036x_adjtimer_set(i64 adj)
{
    Sint32T freq_offset_uppm;
    // the frequency offset is set relative to the holdover value, saved when the NCO mode was entered.
    freq_offset_uppm = (adj*1000LL)/(1<<16) + nco_ho_freqOffsetUppm;
    ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xSetFreq(zl_3036x_zl303xx_synce, freq_offset_uppm));
    zl_3036x_zl303xx_synce->pllParams.dcoFreq = freq_offset_uppm;
    T_DG(TRACE_GRP_SYNC_INTF,"set DCO frequency %d uppm", freq_offset_uppm);
    return VTSS_RC_OK;
}

mesa_rc zl3036x_clock_output_adjtimer_set(i64 adj)
{
    Sint32T freq_offset_uppm;
    freq_offset_uppm = (adj*1000LL)/(1<<16);
    Sint32T current_uppm;
    Uint32T current_pllId;

    current_pllId = zl_3036x_zl303xx_params->pllParams.pllId;
    current_uppm = zl_3036x_zl303xx_params->pllParams.dcoFreq;
    // switch to the PTP DPLL
    zl_3036x_zl303xx_params->pllParams.pllId = ZL303XX_DPLL_ID_2;
    ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xSetFreq(zl_3036x_zl303xx_params, freq_offset_uppm));
    // switch back to the SyncE DPLL
    zl_3036x_zl303xx_params->pllParams.dcoFreq = current_uppm;
    zl_3036x_zl303xx_params->pllParams.pllId = current_pllId;
    T_DG(TRACE_GRP_SYNC_INTF,"set DCO frequency %d uppm", freq_offset_uppm);
    return VTSS_RC_OK;
}


mesa_rc zl3036x_clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    mesa_rc rc = VTSS_RC_OK;

    zl303xx_DpllIdE dpllId = (source == PTP_CLOCK_SOURCE_SYNCE) ? ZL303XX_DPLL_ID_1 : ZL303XX_DPLL_ID_2;
    T_IG(TRACE_GRP_SYNC_INTF,"zl303xx_SynthDrivePll %d source %d", dpllId, source);
    if (source == PTP_CLOCK_SOURCE_SYNCE) {
        ZL_3036X_CHECK((zlStatusE)zl303xx_SynthDrivePll(zl_3036x_zl303xx_synce, SYNTH_ID_PTP, ZL303XX_DPLL_ID_1));
    } else {
        ZL_3036X_CHECK((zlStatusE)zl303xx_SynthDrivePll(zl_3036x_zl303xx_params, SYNTH_ID_PTP, ZL303XX_DPLL_ID_2));
    }
    return rc;
}

#ifdef __cplusplus
extern "C" {
#endif

void zl3036x_clock_take_hw_nco_control(void)
{
    ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xTakeHwNcoControl(zl_3036x_zl303xx_params));
    T_DG(TRACE_GRP_SYNC_INTF, "take hw nco control");
}

void zl3036x_clock_return_hw_nco_control(void)
{
    ZL_3036X_CHECK((zlStatusE)zl303xx_Dpll36xReturnHwNcoControl(zl_3036x_zl303xx_params));
    T_DG(TRACE_GRP_SYNC_INTF, "return hw nco control");
}

#ifdef __cplusplus
}
#endif

static mesa_rc zl_3036x_apr_set_up_for_bc_full_on_path_phase_synce()
{   
    Uint32T current_pllId;

    current_pllId = zl_3036x_zl303xx_params->pllParams.pllId;
    zl_3036x_zl303xx_params->pllParams.pllId = ZL303XX_DPLL_ID_2;
    ZL_3036X_CHECK(zl303xx_Dpll36xBandwidthSet(zl_3036x_zl303xx_params, 0x7));
    ZL_3036X_CHECK(zl303xx_Dpll36xBandwidthCustomSet(zl_3036x_zl303xx_params, 0x87));
    ZL_3036X_CHECK(zl303xx_Dpll36xPullInRangeSet(zl_3036x_zl303xx_params, 0x0));
    ZL_3036X_CHECK(zl303xx_Dpll36xPhaseSlopeLimitSet(zl_3036x_zl303xx_params, 0x4));
    zl_3036x_zl303xx_params->pllParams.pllId = ZL303XX_DPLL_ID_1;
    ZL_3036X_CHECK(zl303xx_Dpll36xBandwidthSet(zl_3036x_zl303xx_params, 0x7));
    ZL_3036X_CHECK(zl303xx_Dpll36xBandwidthCustomSet(zl_3036x_zl303xx_params, 0x82));
    ZL_3036X_CHECK(zl303xx_Dpll36xPullInRangeSet(zl_3036x_zl303xx_params, 0x0));
    ZL_3036X_CHECK(zl303xx_Dpll36xPhaseSlopeLimitSet(zl_3036x_zl303xx_params, 0x4));
    zl_3036x_zl303xx_params->pllParams.pllId = current_pllId;
    
    return VTSS_RC_OK;
}

#endif //defined (VTSS_OPTION_SYNCE) || defined(VTSS_SW_OPTION_PTP)
