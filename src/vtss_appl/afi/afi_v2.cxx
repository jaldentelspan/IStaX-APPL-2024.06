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

#include "microchip/ethernet/switch/api.h" /* To figure out whether VTSS_AFI_V2 is defined or not */

// This file implements the AFI module for chips supporting v2
// of the switch-core-based AFI, which at the time of writing is JR2B, JR2C, and ServalT.

#include "vtss_os_wrapper.h"
#include "afi_api.h"
#include "afi.h"

/******************************************************************************/
//
// Private functions
//
/******************************************************************************/

/****************************************************************************/
// AFI_PLATFORM_frm_to_hw()
/****************************************************************************/
static mesa_rc AFI_PLATFORM_frm_to_hw(packet_tx_props_t *tx_props, u32 api_id, BOOL single, u32 frm_size_bytes /* only used in multi-frame flows */)
{
    mesa_afi_fast_inj_frm_cfg_t frm_cfg;

    // Inject the frame and call the API's hijack() function to get it mapped
    // correctly into the AFI.

    // Make the frame hit the AFI.
    tx_props->tx_info.afi_id = MESA_AFI_ID_NONE + 1; // Anything but MESA_AFI_ID_NONE.

    // Avoid the frame getting deallocated by the packet module.
    tx_props->packet_info.no_free = TRUE;

    T_D("Transmitting frame");
    if (packet_tx(tx_props) != VTSS_RC_OK) {
        return AFI_RC_PACKET_TX_FAILED;
    }

    // Time to "hijack" the frame.
    if (single) {
        T_D("api_id = %u: Hijacking frame", api_id);
        if (mesa_afi_slow_inj_frm_hijack(NULL, api_id) != VTSS_RC_OK) {
            return AFI_RC_SLOW_HIJACK_FAILED;
        }
    } else {
        T_D("Hijacking frame (size = %u bytes)", frm_size_bytes);
        memset(&frm_cfg, 0, sizeof(frm_cfg));
        frm_cfg.frm_size = frm_size_bytes;
        if (mesa_afi_fast_inj_frm_hijack(NULL, api_id, &frm_cfg) != VTSS_RC_OK) {
            return AFI_RC_FAST_HIJACK_FAILED;
        }

    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_frms_to_hw_do()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_frms_to_hw_do(afi_multi_state_t *multi_state)
{
    size_t i;

    if (!multi_state->platform.frames_in_hw) {
        // Inject the frames
        for (i = 0; i < multi_state->multi_len; i++) {
            // The AFI_V2 API uses line rate, so we have to always ask for 20 bytes more per frame
            // than the user requests due to IFG and preamble. This is whether or not the user
            // asked for L1 or L2 rate. When we specify the rate towards the AFI_V2 API,
            // we convert the user-specified rate to L1 depending on whether the user asked
            // for L1 or L2.
            VTSS_RC(AFI_PLATFORM_frm_to_hw(&multi_state->tx_props[i], multi_state->platform.api_id, FALSE, multi_state->status.frm_sz_bytes[i] + 20));
        }

        multi_state->platform.frames_in_hw = TRUE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
//
// Semi-public single-frame flow functions
//
/******************************************************************************/

/******************************************************************************/
// AFI_PLATFORM_single_alloc()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_alloc(afi_single_state_t *single_state)
{
    mesa_afi_slow_inj_alloc_cfg_t alloc_cfg;

    if (single_state->status.params.jitter < 0 || single_state->status.params.jitter > AFI_SINGLE_JITTER_LAST) {
        return AFI_RC_INVALID_JITTER;
    }

    // Allocate an ID in the API.
    memset(&alloc_cfg, 0, sizeof(alloc_cfg));
    alloc_cfg.port_no            = single_state->status.dst_port; // Set to VTSS_PORT_NO_NONE to hit a special port (VD1) when masquerading.
    alloc_cfg.masquerade_port_no = single_state->status.masquerade_port;
    alloc_cfg.prio               = single_state->tx_props.tx_info.cos;

    if (mesa_afi_slow_inj_alloc(NULL, &alloc_cfg, &single_state->platform.api_id) != VTSS_RC_OK) {
        // The error codes from the API are not very descriptive, so override them.
        return AFI_RC_SLOW_ALLOC_FAILED;
    }

    T_D("api_id = %u. Allocated frame successfully", single_state->platform.api_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_single_free()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_free(afi_single_state_t *single_state)
{
    mesa_rc rc;

    if (single_state->status.oper_up) {
        // Gotta stop it prior to deleting it.
        T_D("api_id = %u: Stopping flow", single_state->platform.api_id);
        if (mesa_afi_slow_inj_stop(NULL, single_state->platform.api_id) != VTSS_RC_OK) {
            T_E("Unable to stop single-frame flow. Continuing to attempt to delete it");
        }

        single_state->platform.active_time_ms += (afi_time_ms_get() - single_state->platform.tx_time_start);
        T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, single_state->platform.active_time_ms, single_state->platform.tx_time_start);
    }

    T_D("api_id = %u: Freeing flow", single_state->platform.api_id);
    if ((rc = mesa_afi_slow_inj_free(NULL, single_state->platform.api_id)) != VTSS_RC_OK) {
        rc = AFI_RC_SLOW_INJ_FREE;
    }

    return rc;
}

/******************************************************************************/
// AFI_PLATFORM_single_frm_to_hw()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_frm_to_hw(afi_single_state_t *single_state)
{
    VTSS_RC(AFI_PLATFORM_frm_to_hw(&single_state->tx_props, single_state->platform.api_id, TRUE /* Single-frame flow */, 0 /* Unused */));
    single_state->platform.frame_in_hw = TRUE;

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_single_pause()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_pause(afi_single_state_t *single_state)
{
    T_D("api_id = %u: Pausing flow", single_state->platform.api_id);
    if (mesa_afi_slow_inj_stop(NULL, single_state->platform.api_id) != VTSS_RC_OK) {
        return AFI_RC_SLOW_STOP_FAILED;
    }

    single_state->platform.active_time_ms += (afi_time_ms_get() - single_state->platform.tx_time_start);
    T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, single_state->platform.active_time_ms, single_state->platform.tx_time_start);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_single_resume()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_resume(afi_single_state_t *single_state)
{
    mesa_afi_slow_inj_start_cfg_t start_cfg;

    if (!single_state->platform.frame_in_hw) {
        VTSS_RC(AFI_PLATFORM_frm_to_hw(&single_state->tx_props, single_state->platform.api_id, TRUE /* Single-frame flow */, 0 /* Unused */));
        single_state->platform.frame_in_hw = TRUE;
    }

    // Time to start the flow.
    memset(&start_cfg, 0, sizeof(start_cfg));
    start_cfg.fph                = single_state->status.params.fph;
    start_cfg.jitter_mode        = single_state->status.params.jitter;
    start_cfg.first_frame_urgent = single_state->status.params.first_frame_urgent;

    T_D("api_id = %u: Resuming single-frame flow (requested fph = " VPRI64u ")", single_state->platform.api_id, single_state->status.params.fph);
    if (mesa_afi_slow_inj_start(NULL, single_state->platform.api_id, &start_cfg) != VTSS_RC_OK) {
        return AFI_RC_SLOW_START_FAILED;
    }

    single_state->platform.tx_time_start = afi_time_ms_get();
    T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, single_state->platform.active_time_ms, single_state->platform.tx_time_start);

    // When the above call succeeds, AFI v2 uses the requested number
    // of frames per hour as the actual.
    single_state->status.fph_act = single_state->status.params.fph;

    T_D("Started single-frame flow (actual fph = " VPRI64u ")", single_state->status.fph_act);

    return VTSS_RC_OK;
}

/******************************************************************************/
//
// Semi-public multi-frame flow functions
//
/******************************************************************************/

/******************************************************************************/
// AFI_PLATFORM_multi_alloc()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_alloc(afi_multi_state_t *multi_state)
{
    mesa_afi_fast_inj_alloc_cfg_t alloc_cfg;

    // Allocate an ID in the API.
    memset(&alloc_cfg, 0, sizeof(alloc_cfg));
    alloc_cfg.port_no            = multi_state->status.dst_port; // Set to VTSS_PORT_NO_NONE to hit a special port (VD1) when masquerading.
    alloc_cfg.masquerade_port_no = multi_state->status.masquerade_port;
    alloc_cfg.prio               = multi_state->tx_props[0].tx_info.cos;
    alloc_cfg.frm_cnt            = multi_state->multi_len;

    if (mesa_afi_fast_inj_alloc(NULL, &alloc_cfg, &multi_state->platform.api_id) != VTSS_RC_OK) {
        // The error codes from the API are not very descriptive, so override them.
        return AFI_RC_FAST_ALLOC_FAILED;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_multi_frms_to_hw()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_frms_to_hw(afi_multi_state_t *multi_state)
{
    return AFI_PLATFORM_frms_to_hw_do(multi_state);
}

/******************************************************************************/
// AFI_PLATFORM_multi_pause()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_pause(afi_multi_state_t *multi_state)
{
    if (mesa_afi_fast_inj_stop(NULL, multi_state->platform.api_id) != VTSS_RC_OK) {
        return AFI_RC_FAST_STOP_FAILED;
    }

    multi_state->platform.active_time_ms += (afi_time_ms_get() - multi_state->platform.tx_time_start);
    T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, multi_state->platform.active_time_ms, multi_state->platform.tx_time_start);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_multi_resume()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_resume(afi_multi_state_t *multi_state)
{
    mesa_afi_fast_inj_start_cfg_t    start_cfg;
    mesa_afi_fast_inj_start_actual_t actual;
    size_t                           i;
    u64                              accumulated_frm_sz = 0, accumulated_ifg_and_preamble_sz = 0;

    VTSS_RC(AFI_PLATFORM_frms_to_hw_do(multi_state));

    // Time to start the flow.
    memset(&start_cfg, 0, sizeof(start_cfg));
    start_cfg.seq_cnt = multi_state->status.params.seq_cnt;

    if (multi_state->status.params.line_rate) {
        // If user asked for L1 rate, then there's a one-to-one correspondance
        // between this and what AFI_V2 expects.
        start_cfg.bps = multi_state->status.params.bps;
    } else {
        // If user asked for L2 rate, we need to convert to L1 prior to
        // invoking AFI_V2.
        // Compute the number of bytes in the emix.
        for (i = 0; i < multi_state->multi_len; i++) {
            accumulated_frm_sz += multi_state->status.frm_sz_bytes[i];
        }

        accumulated_ifg_and_preamble_sz = 20LLU * multi_state->multi_len;

        VTSS_ASSERT(accumulated_frm_sz != 0);
        start_cfg.bps = ((accumulated_frm_sz + accumulated_ifg_and_preamble_sz) * multi_state->status.params.bps) / accumulated_frm_sz;
    }

    T_D("Starting multi-frame flow (User-requested L%c rate = " VPRI64u " bps. API-requested L1 rate = " VPRI64u " bps)", multi_state->status.params.line_rate ? '1' : '2', multi_state->status.params.bps, start_cfg.bps);

    if (mesa_afi_fast_inj_start(NULL, multi_state->platform.api_id, &start_cfg, &actual) != VTSS_RC_OK) {
        return AFI_RC_FAST_START_FAILED;
    }

    multi_state->platform.tx_time_start = afi_time_ms_get();
    T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, multi_state->platform.active_time_ms, multi_state->platform.tx_time_start);

    // The rate requested to - and reported back by - AFI v2 is in L1.
    if (multi_state->status.params.line_rate) {
        // User specified it in L1. Report it back in L1.
        multi_state->status.bps_act = actual.bps;
    } else {
        // User specified it in L2. Convert the AFI-reported value back from L1 to L2.
        multi_state->status.bps_act = (accumulated_frm_sz * actual.bps) / (accumulated_frm_sz + accumulated_ifg_and_preamble_sz);
    }

    T_D("Started multi-frame flow (User-requested L%c rate = " VPRI64u " bps, API-reported L1 rate = " VPRI64u " bps. User-achieved L%c rate = " VPRI64u " bps)", multi_state->status.params.line_rate ? '1' : '2', multi_state->status.params.bps, actual.bps, multi_state->status.params.line_rate ? '1' : '2', multi_state->status.bps_act);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_multi_free()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_free(afi_multi_state_t *multi_state)
{
    mesa_rc rc;

    if (multi_state->status.oper_up) {
        // Gotta stop it prior to deleting it.
        if (mesa_afi_fast_inj_stop(NULL, multi_state->platform.api_id) != VTSS_RC_OK) {
            T_E("Unable to stop multi-frame flow. Continuing to attempt to delete it");
        }

        multi_state->platform.active_time_ms += (afi_time_ms_get() - multi_state->platform.tx_time_start);
        T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, multi_state->platform.active_time_ms, multi_state->platform.tx_time_start);
    }

    if ((rc = mesa_afi_fast_inj_free(NULL, multi_state->platform.api_id)) != VTSS_RC_OK) {
        rc = AFI_RC_FAST_INJ_FREE;
    }

    return rc;
}

/******************************************************************************/
// AFI_PLATFORM_alloc_possible()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_alloc_possible(u32 flow_cnt, u32 frame_size_bytes, BOOL alternate_resource_pool)
{
    return VTSS_RC_OK;
}

/******************************************************************************/
// afi_platform_v2_create()
/******************************************************************************/
void afi_platform_v2_create(afi_platform_func_t *func)
{
    func->single_alloc     = AFI_PLATFORM_single_alloc;
    func->single_free      = AFI_PLATFORM_single_free;
    func->single_pause     = AFI_PLATFORM_single_pause;
    func->single_resume    = AFI_PLATFORM_single_resume;
    func->single_frm_to_hw = AFI_PLATFORM_single_frm_to_hw;
    func->multi_alloc      = AFI_PLATFORM_multi_alloc;
    func->multi_free       = AFI_PLATFORM_multi_free;
    func->multi_pause      = AFI_PLATFORM_multi_pause;
    func->multi_resume     = AFI_PLATFORM_multi_resume;
    func->multi_frms_to_hw = AFI_PLATFORM_multi_frms_to_hw;
    func->alloc_possible   = AFI_PLATFORM_alloc_possible;
}

