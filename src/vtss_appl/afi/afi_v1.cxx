/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "microchip/ethernet/switch/api.h" /* To figure out whether VTSS_AFI_V1 is defined or not */

// This file implements the AFI module for chips supporting v1
// of the switch-core-based AFI, which at the time of writing is Serval-1.

#include "vtss_os_wrapper.h"
#include "afi_api.h"
#include "afi.h"
#include "mgmt_api.h"        /* For mgmt_long2str_float() */

// Defines the number of bytes we may use of the switch core
// memory for AFI purposes. Attempts to allocate a flow that
// will cause this to be exceeded will fail.
// In order to "guarantee" service activation tests resources in the switch core
// memory even though MEPs are also using of these resources, we define two
// pools of resources. The second for SAT flows and the first for single AFI
// flows and other multi AFI flows.
// The sum of these two define the maximum, worst case allocation in the switch
// core RAM. One could lower the first, but I don't have a clear picture of
// the required amount for CCM and other flows.
static u32 AFI_switch_core_ram_left_bytes[2] = {165000, 165000};

/******************************************************************************/
//
// Private functions
//
/******************************************************************************/

/******************************************************************************/
// AFI_PLATFORM_common_headroom_get()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_common_headroom_get(mesa_port_no_t dst_port, u32 *headroom)
{
    if (dst_port == VTSS_PORT_NO_NONE) {
        // Caller wants to masquerade, which requires a loop port
        if (fast_cap(VTSS_APPL_CAP_LOOP_PORT_UP_INJ) != MESA_PORT_NO_NONE) {
            // We do indeed have a loop port. Add headroom for an additional IFH.
            *headroom = fast_cap(MESA_CAP_PACKET_HDR_SIZE) - 4 /* HDR_SIZE includes 4 bytes of possible VLAN tag, which is unused here */;
        } else {
            T_E("Masquerading requires a loop port");
            return AFI_RC_MASQUERADING_REQUIRES_LOOP_PORT;
        }
    } else {
        *headroom = 0;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_common_free()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_common_free(afi_platform_state_t *pstate, mesa_port_no_t dst_port, u32 frm_sz_bytes, BOOL free_frames, BOOL alternate_resource_pool)
{
    u32  slot, headroom, alloc_sz_bytes, pool_idx = alternate_resource_pool ? 1 : 0;
    BOOL first = TRUE;

    VTSS_RC(AFI_PLATFORM_common_headroom_get(dst_port, &headroom));
    alloc_sz_bytes = headroom + frm_sz_bytes;

    for (slot = 0; slot < pstate->slot_cnt; slot++) {
        if (pstate->slots[slot].afi_id_allocated) {
            // There has indeed been allocated slot_cnt AFI IDs. Free them.
            // This particular call causes frames to cease from being injected.
            if (mesa_afi_free(NULL, pstate->slots[slot].afi_id)) {
                T_E("mesa_afi_free(%u) failed", pstate->slots[slot].afi_id);
                // Continue because we need to free all IDs and memory.
            }

            if (first) {
                pstate->active_time_ms += (afi_time_ms_get() - pstate->tx_time_start);
                T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, pstate->active_time_ms, pstate->tx_time_start);
                first = FALSE;
            }

            pstate->slots[slot].afi_id_allocated = FALSE;
        }

        if (free_frames) {
            // Only increase/decrease the overall switch core RAM usage
            // when we really free the frame memory too.

            AFI_switch_core_ram_left_bytes[pool_idx] += alloc_sz_bytes;
            T_D("switch_core_ram_left[%u] = %u", pool_idx, AFI_switch_core_ram_left_bytes[pool_idx]);

            // Also free the frame memory - if allocated.
            if (pstate->slots[slot].frm != NULL) {
                packet_tx_free(pstate->slots[slot].frm);
                pstate->slots[slot].frm = NULL;
            }
        }
    }

    if (free_frames) {
        pstate->slot_cnt = 0;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_common_alloc()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_common_alloc(afi_platform_state_t *pstate, packet_tx_props_t *tx_props, u32 fps_requested, u32 *fps_actual, vtss_module_id_t module_id, mesa_port_no_t dst_port, u32 frm_sz_bytes, BOOL alternate_resource_pool)
{
    uint32_t loop_port_up_inj = fast_cap(VTSS_APPL_CAP_LOOP_PORT_UP_INJ);

    u32     headroom, alloc_sz_bytes, slot, pool_idx = alternate_resource_pool ? 1 : 0;
    mesa_rc rc;

    if (fps_requested < 1) {
        T_E("AFI FPS = 0 (frame size = %u bytes)", frm_sz_bytes);
        return AFI_RC_FPS_EQUAL_TO_ZERO;
    }

    // In some cases, we need an additional IFH in the beginning of the frame.
    // The size of this IFH will be reflected in #headroom.
    VTSS_RC(AFI_PLATFORM_common_headroom_get(dst_port, &headroom));
    alloc_sz_bytes = headroom + frm_sz_bytes;

    // As long as we have room in our array of AFI IDs and we aren't spot on w.r.t.
    // the number of frames to transmit per second, allocate new AFI IDs.
    pstate->slot_cnt = 0;

    *fps_actual = 0;
    while (*fps_actual < fps_requested && pstate->slot_cnt < ARRSZ(pstate->slots)) {
        u32                 fps_missing = fps_requested - *fps_actual;
        mesa_afi_frm_dscr_t dscr;
        mesa_afi_frm_dscr_actual_t actual;
        afi_slot_state_t    *slot_state = &pstate->slots[pstate->slot_cnt];

        memset(&dscr, 0, sizeof(dscr));
        dscr.fps = fps_missing;

        if (alloc_sz_bytes > AFI_switch_core_ram_left_bytes[pool_idx]) {
            T_D("Requested = %u fps, alloc_sz_bytes = %u, switch_core_ram_left[%u] = %u", fps_requested, alloc_sz_bytes, pool_idx, AFI_switch_core_ram_left_bytes[pool_idx]);
            rc = AFI_RC_SWITCH_CORE_FRAME_RAM_EXHAUSTED;
            goto do_exit;
        } else  {
            AFI_switch_core_ram_left_bytes[pool_idx] -= alloc_sz_bytes;
            rc = mesa_afi_alloc(NULL, &dscr, &actual, &slot_state->afi_id);
            T_D("AFI slot %u: Rate = %u fps, switch_core_ram_left[%u] = %u, rc = %s", slot_state->afi_id, actual.fps, pool_idx, AFI_switch_core_ram_left_bytes[pool_idx], error_txt(rc));

            if (rc != VTSS_RC_OK) {
                // Translate to something sensible
                rc = AFI_RC_AFI_ALLOC_FAILED;
                goto do_exit;
            }

            slot_state->afi_id_allocated = TRUE;

            // Remember what it could provide us, so that we can ask for it next
            // time we possible need to allocate new IDs.
            slot_state->fps = actual.fps;
        }

        // On exit from mesa_afi_alloc(), dscr.actual_fps holds the actual, achievable number of frames per second for this slot.
        *fps_actual += actual.fps;
        pstate->slot_cnt++;
    }

    // Now that we know how many slots we need, allocate and copy the frame.
    for (slot = 0; slot < pstate->slot_cnt; slot++) {
        u8 *frm = packet_tx_alloc(alloc_sz_bytes - 4 /* FCS */);
        if (frm == NULL) {
            rc = AFI_RC_PACKET_TX_ALLOC_FAILED;
            goto do_exit;
        }

        pstate->slots[slot].frm = frm;

        // Copy the frame
        memcpy(frm + headroom, tx_props->packet_info.frm, frm_sz_bytes - 4 /* FCS */);

        if (loop_port_up_inj != MESA_PORT_NO_NONE) {
            // Create inner IFH in front of frame if masquerading.
            // This is the user's IFH. When we transmit the frame, we add a "hit-the AFI"-IFH.
            if (dst_port == VTSS_PORT_NO_NONE) {
                if ((rc = mesa_packet_tx_hdr_encode(NULL, &tx_props->tx_info, headroom, frm, &headroom)) != VTSS_RC_OK) {
                    T_E("mesa_packet_tx_hdr_encode() failed (%s)", error_txt(rc));
                    goto do_exit;
                }

                // To prepare for actual transmission, create "hit-the AFI"-IFH directly into tx_props.
                packet_tx_props_init(&pstate->slots[slot].tx_props);
                pstate->slots[slot].tx_props.packet_info.modid     = module_id;
                pstate->slots[slot].tx_props.packet_info.frm       = frm;
                pstate->slots[slot].tx_props.packet_info.len       = alloc_sz_bytes - 4; /* FCS */
                pstate->slots[slot].tx_props.tx_info.dst_port_mask = VTSS_BIT64(loop_port_up_inj);
            } else {
                // Transmitting directly to a front port. Use the user's Tx properties.
                // We need to use the copied frame. The length is OK already.
                pstate->slots[slot].tx_props = *tx_props;
                pstate->slots[slot].tx_props.packet_info.frm = frm;
            }
        } else {
            // Transmitting directly to a front port. Use the user's Tx properties.
            // We need to use the copied frame. The length is OK already.
            pstate->slots[slot].tx_props = *tx_props;
            pstate->slots[slot].tx_props.packet_info.frm = frm;
        }

        // Don't free the frame.
        pstate->slots[slot].tx_props.packet_info.no_free = TRUE;
    }

do_exit:
    if (rc != VTSS_RC_OK) {
        // Free AFI IDs and frames.
        (void)AFI_PLATFORM_common_free(pstate, dst_port, frm_sz_bytes, TRUE /* Free frame pointers too */, alternate_resource_pool);
    }

    return rc;
}

/******************************************************************************/
// AFI_PLATFORM_common_realloc_afi_ids()
// Makes sure that we have allocated the AFI IDs we need.
/******************************************************************************/
static mesa_rc AFI_PLATFORM_common_realloc_afi_ids(afi_platform_state_t *pstate)
{
    u32 slot;

    for (slot = 0; slot < pstate->slot_cnt; slot++) {
        afi_slot_state_t *slot_state = &pstate->slots[slot];

        if (!slot_state->afi_id_allocated) {
            mesa_afi_frm_dscr_t dscr;
            mesa_afi_frm_dscr_actual_t actual;

            memset(&dscr, 0, sizeof(dscr));
            dscr.fps = slot_state->fps;

            if (mesa_afi_alloc(NULL, &dscr, &actual, &slot_state->afi_id) != VTSS_RC_OK) {
                // Translate to something sensible
                return AFI_RC_AFI_ALLOC_FAILED;
            }

            if (dscr.fps != actual.fps) {
                T_E("Odd that the AFI module doesn't support the same number of FPS always (first time = %u, now = %u)", dscr.fps, actual.fps);
                return VTSS_RC_ERROR;
            }

            slot_state->afi_id_allocated = TRUE;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_common_resume()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_common_resume(afi_platform_state_t *pstate)
{
    int               slot;
    u32               flag_mask = 0;

    // The following call will cater for the case where we were
    // first paused, then resumed, and not the case where we
    // were first allocated, then resumed, in which case it will do nothing.
    VTSS_RC(AFI_PLATFORM_common_realloc_afi_ids(pstate));

    // Transfer the frames to H/W. Because of the way the slots
    // are used, the highest frequency frames come first.
    // This means that if we iterate from slot 0 to slot_cnt,
    // the number of frames actually transmitted before all slots
    // are sent will be higher than if we transmit from slot_cnt
    // to 0, and since some users of the AFI module may run the
    // flow for a certain amount of time, we better make it as
    // accurate as possible.
    for (slot = pstate->slot_cnt - 1; slot >= 0; slot--) {
        afi_slot_state_t *slot_state = &pstate->slots[slot];

        T_D("Transmitting frame for slot %u", slot);

        // We may have been allocated a new AFI ID since last time, so always set it.
        slot_state->tx_props.tx_info.afi_id = slot_state->afi_id;

        flag_mask |= VTSS_BIT(slot);

        if (packet_tx(&slot_state->tx_props) != VTSS_RC_OK) {
            return AFI_RC_PACKET_TX_FAILED;
        }

        // Time to "hijack" the frame, i.e. making sure it's known by the AFI
        // H/W.
        T_D("afi_id = %u: Hijacking frame", slot_state->afi_id);
        if (mesa_afi_hijack(NULL, slot_state->afi_id) != VTSS_RC_OK) {
            return AFI_RC_HIJACK_FAILED;
        }
    }

    pstate->tx_time_start = afi_time_ms_get();
    T_D("active_time_ms = " VPRI64u ", tx_time_start = " VPRI64u, pstate->active_time_ms, pstate->tx_time_start);

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
    u32  fps_requested, fps_actual;
    char buf[20];

    fps_requested = single_state->status.params.fph / 3600LLU;
    VTSS_RC(AFI_PLATFORM_common_alloc(&single_state->platform, &single_state->tx_props, fps_requested, &fps_actual, single_state->status.module_id, single_state->status.dst_port, single_state->status.frm_sz_bytes, FALSE));
    single_state->status.fph_act = fps_actual * 3600LLU;

    mgmt_long2str_float(buf, ((fps_requested - fps_actual) * 1000LLU) / fps_requested, 1);
    T_I("Requested rate = %u fps. Actual rate = %u fps. Error = %s%%", fps_requested, fps_actual, buf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_single_frm_to_hw()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_frm_to_hw(afi_single_state_t *single_state)
{
    return AFI_RC_NOT_SUPPORTED;
}

/******************************************************************************/
// AFI_PLATFORM_single_pause()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_pause(afi_single_state_t *single_state)
{
    // Pausing the AFI is only possible by removing all frames from the AFI,
    // but keeping them in S/W for possible later re-initiation.
    return AFI_PLATFORM_common_free(&single_state->platform, single_state->status.dst_port, single_state->status.frm_sz_bytes, FALSE /* Don't free frame pointers, only AFI IDs */, FALSE);
}

/******************************************************************************/
// AFI_PLATFORM_single_resume()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_resume(afi_single_state_t *single_state)
{
    VTSS_RC(AFI_PLATFORM_common_resume(&single_state->platform));

    T_D("Started single-frame flow (actual fph = " VPRI64u ")", single_state->status.fph_act);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_single_free()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_single_free(afi_single_state_t *single_state)
{
    return AFI_PLATFORM_common_free(&single_state->platform, single_state->status.dst_port, single_state->status.frm_sz_bytes, TRUE /* Stop frames and free AFI IDs (same same) and free frame pointers */, FALSE);
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
    u32  fps_requested, fps_actual;
    u64  bps_requested, bps_actual;
    char buf[20];

    if (multi_state->multi_len != 1) {
        T_E("Internal error. Multi-frame flow length can only be one frame, but is in fact %u", multi_state->multi_len);
        return VTSS_RC_ERROR;
    }

    // Compute the number of frames to transmit per second.
    bps_requested = multi_state->status.params.bps;
    fps_requested = bps_requested / (8LLU * (multi_state->status.frm_sz_bytes[0] + (multi_state->status.params.line_rate ? 20 : 0)));

    VTSS_RC(AFI_PLATFORM_common_alloc(&multi_state->platform, &multi_state->tx_props[0], fps_requested, &fps_actual, multi_state->status.module_id, multi_state->status.dst_port, multi_state->status.frm_sz_bytes[0], multi_state->status.params.alternate_resource_pool));

    // Update the actual B/W
    bps_actual = fps_actual * 8LLU * (multi_state->status.frm_sz_bytes[0] + (multi_state->status.params.line_rate ? 20 : 0));
    multi_state->status.bps_act = bps_actual;

    mgmt_long2str_float(buf, ((bps_requested - bps_actual) * 1000LLU) / bps_requested, 1);
    T_I("Requested L%c rate = " VPRI64Fu("10") " bps = %8u fps @ frame size = %u bytes.",              multi_state->status.params.line_rate ? '1' : '2', bps_requested, fps_requested, multi_state->status.frm_sz_bytes[0]);
    T_I("Obtained  L%c rate = " VPRI64Fu("10") " bps = %8u fps @ frame size = %u bytes. Error = %s%%", multi_state->status.params.line_rate ? '1' : '2', bps_actual,    fps_actual,    multi_state->status.frm_sz_bytes[0], buf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_multi_frms_to_hw()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_frms_to_hw(afi_multi_state_t *multi_state)
{
    return AFI_RC_NOT_SUPPORTED;
}

/******************************************************************************/
// AFI_PLATFORM_multi_pause()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_pause(afi_multi_state_t *multi_state)
{
    // Pausing the AFI is only possible by removing all frames from the AFI,
    // but keeping them in S/W for possible later re-initiation.
    return AFI_PLATFORM_common_free(&multi_state->platform, multi_state->status.dst_port, multi_state->status.frm_sz_bytes[0], FALSE /* Don't free frame pointers, only AFI IDs */, multi_state->status.params.alternate_resource_pool);
}

/******************************************************************************/
// AFI_PLATFORM_multi_resume()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_resume(afi_multi_state_t *multi_state)
{
    VTSS_RC(AFI_PLATFORM_common_resume(&multi_state->platform));

    T_D("Started multi-frame flow (actual L%c rate = " VPRI64u " bps)", multi_state->status.params.line_rate ? '1' : '2', multi_state->status.bps_act);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_PLATFORM_multi_free()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_multi_free(afi_multi_state_t *multi_state)
{
    return AFI_PLATFORM_common_free(&multi_state->platform, multi_state->status.dst_port, multi_state->status.frm_sz_bytes[0], TRUE /* Stop frames and free AFI IDs (same same) and free frame pointers */, multi_state->status.params.alternate_resource_pool);
}

/******************************************************************************/
// AFI_PLATFORM_alloc_possible()
/******************************************************************************/
static mesa_rc AFI_PLATFORM_alloc_possible(u32 flow_cnt, u32 frame_size_bytes, BOOL alternate_resource_pool)
{
    u32 pool_idx = alternate_resource_pool ? 1 : 0;

    // Add worst-case two IFHs to the frame size and multiply the number of
    // expected flows by the total frame size and the max number of slots per
    // frame.
    u32 max_size = flow_cnt * (frame_size_bytes + 2 * fast_cap(MESA_CAP_PACKET_HDR_SIZE)) * AFI_V1_SLOT_CNT_MAX;

    T_D("Ask for %u flows of %u bytes => max size = %u bytes. Switch core RAM left[%u] = %u", flow_cnt, frame_size_bytes, max_size, pool_idx, AFI_switch_core_ram_left_bytes[pool_idx]);

    return AFI_switch_core_ram_left_bytes[pool_idx] >= max_size ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/******************************************************************************/
// afi_platform_v1_create()
/******************************************************************************/
void afi_platform_v1_create(afi_platform_func_t *func)
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

