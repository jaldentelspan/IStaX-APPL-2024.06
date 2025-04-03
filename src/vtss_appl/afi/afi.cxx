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

#include "vtss_os_wrapper.h"
#include "afi_api.h"
#include "afi.h"
#include "port_api.h" /* For port_change_register() */
#include "misc_api.h" /* For iport2uport()          */

// Cannot be static, since it's referenced from multiple C-files
critd_t                   AFI_crit;
static int                AFI_enable = 0;
static CapArray<afi_single_state_t, VTSS_APPL_CAP_AFI_SINGLE_CNT> AFI_single_state;
static CapArray<afi_multi_state_t, VTSS_APPL_CAP_AFI_MULTI_CNT> AFI_multi_state;
static CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> AFI_link_up;

// Trace registration. Initialized by afi_init()
static vtss_trace_reg_t AFI_trace_reg = {
    VTSS_TRACE_MODULE_ID, "afi", "Automatic Frame Injector"
};

#define AFI_TRACE_GRP_FRAME_PRINT 1

static vtss_trace_grp_t AFI_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [AFI_TRACE_GRP_FRAME_PRINT] = {
        "print",
        "Used when debug-printing frames",
        VTSS_TRACE_LVL_DEBUG
    }
};

VTSS_TRACE_REGISTER(&AFI_trace_reg, AFI_trace_grps);

static afi_platform_func_t afi_platform_func;

/******************************************************************************/
//
// Private functions
//
/******************************************************************************/

/****************************************************************************
 * AFI_port_mask_analyze()
 * Get the number of bits set in #mask.
 * Returns 0 if no bits are set, 1 if exactly one bit is set, 2 if at least
 * two bits are set.
 * If exactly one bit is set, bit_pos holds the bit position.
 ****************************************************************************/
static u32 AFI_port_mask_analyze(u64 mask, u32 *bit_pos)
{
    u32 i, w, p, cnt = 0;

    *bit_pos = 0;

    if (mask == 0) {
        return 0;
    }

    for (i = 0; i < 2; i ++) {
        w = (u32)(mask >> (32 * i));

        if ((p = VTSS_OS_CTZ(w)) < 32) {
            w &= ~VTSS_BIT(p);
            if (w) {
                // Still bits set in w.
                return 2;
            }

            cnt++;
            *bit_pos = p + 32 * i;
        }
    }

    return cnt > 1 ? 2 : cnt;
}

/****************************************************************************
 * AFI_tx_props_check()
 * Check whether contents of #tx_props are suitable for single-/multi-frame
 * injections.
 ****************************************************************************/
static mesa_rc AFI_tx_props_check(packet_tx_props_t *tx_props, mesa_port_no_t *dst_port, mesa_port_no_t *masquerade_port, u32 *frm_sz_bytes, BOOL first)
{
    u32               port_cnt, bit_pos;
    mesa_port_no_t    this_down_port, this_up_port;
    mesa_port_no_t    the_masquerade_port;

    port_cnt            = AFI_port_mask_analyze(tx_props->tx_info.dst_port_mask, &bit_pos);
    the_masquerade_port = tx_props->tx_info.masquerade_port;

    if (port_cnt > 1) {
        // Can't select multiple down-ports
        return AFI_RC_INVALID_DST_PORT_CNT;
    } else if (port_cnt == 0 && the_masquerade_port == VTSS_PORT_NO_NONE) {
        // No down-port and no up-port.
        return AFI_RC_NO_DST_AND_NO_MASQUERADE_PORT_SELECTED;
    } else if (port_cnt == 1 && the_masquerade_port != VTSS_PORT_NO_NONE) {
        // Both down-port and up-port
        return AFI_RC_BOTH_DST_AND_MASQUERADE_PORT_SELECTED;
    }

    if (the_masquerade_port == VTSS_PORT_NO_NONE) {
        // Down-flow, since not masquerading.
        this_down_port = bit_pos;
        this_up_port   = VTSS_PORT_NO_NONE;
    } else {
        // Up-flow, since asked to masquerade.
        this_down_port = VTSS_PORT_NO_NONE;
        this_up_port   = the_masquerade_port;
    }

    if (first) {
        // First time, we simply set the output params
        *dst_port        = this_down_port;
        *masquerade_port = this_up_port;
    } else {
        // Subsequent frames must be sent using same destination.
        if (*dst_port != this_down_port) {
            return AFI_RC_ALL_FRMS_IN_SEQUENCE_MUST_USE_SAME_DOWN_PORT;
        }

        if (*masquerade_port != this_up_port) {
            return AFI_RC_ALL_FRMS_IN_SEQUENCE_MUST_USE_SAME_UP_PORT;
        }
    }

    if (*dst_port != VTSS_PORT_NO_NONE && *dst_port >= AFI_link_up.size()) {
        return AFI_RC_DOWN_PORT_OUT_OF_RANGE;
    }

    if (*masquerade_port != VTSS_PORT_NO_NONE && *masquerade_port >= AFI_link_up.size()) {
        return AFI_RC_UP_PORT_OUT_OF_RANGE;
    }

    *frm_sz_bytes = tx_props->packet_info.len + 4 /* FCS */;

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_frm_alloc_and_cpy()
/******************************************************************************/
static mesa_rc AFI_frm_alloc_and_cpy(packet_tx_props_t *dst, packet_tx_props_t *src)
{
    u32 frm_len = src->packet_info.len;

    *dst = *src;

    if ((dst->packet_info.frm = packet_tx_alloc(frm_len)) == NULL) {
        return AFI_RC_PACKET_TX_ALLOC_FAILED;
    }

    memcpy(dst->packet_info.frm, src->packet_info.frm, frm_len);

    T_D("Frame of %u bytes excluding IFH and FCS", frm_len);
    if (frm_len > 256) {
        if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_RACKET)) {
            // Print it all.
            T_R_HEX(dst->packet_info.frm, frm_len);
        } else {
            // Only print 256 bytes, or we may end up spending too much time
            // printing, causing e.g. SAT tests to fail.
            T_D("Printing first 256 bytes");
            T_D_HEX(dst->packet_info.frm, 256);
        }
    } else {
        T_D_HEX(dst->packet_info.frm, frm_len);
    }

    // Print Tx properties
    packet_debug_tx_props_print(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_DEBUG, src);

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_frm_free()
/******************************************************************************/
static void AFI_frm_free(packet_tx_props_t *tx_props)
{
    u8 **ptr = &tx_props->packet_info.frm;

    if (*ptr) {
        packet_tx_free(*ptr);
        *ptr = NULL;
    }
}

/******************************************************************************/
// AFI_single_find()
/******************************************************************************/
static mesa_rc AFI_single_find(u32 isingle_id, afi_single_state_t **single_state)
{
    if (isingle_id >= AFI_single_state.size()) {
        return AFI_RC_INVALID_SINGLE_ID;
    }

    *single_state = &AFI_single_state[isingle_id];

    if (!(*single_state)->allocated) {
        return AFI_RC_SINGLE_ID_NOT_IN_USE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_multi_find()
/******************************************************************************/
static mesa_rc AFI_multi_find(u32 imulti_id, afi_multi_state_t **multi_state)
{
    if (imulti_id >= AFI_multi_state.size()) {
        return AFI_RC_INVALID_MULTI_ID;
    }

    *multi_state = &AFI_multi_state[imulti_id];

    if (!(*multi_state)->allocated) {
        return AFI_RC_MULTI_ID_NOT_IN_USE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// AFI_port_change_callback()
/******************************************************************************/
static void AFI_port_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    u32 isingle_id, imulti_id;

    T_I("Got port change event on uport %u: Link = %d", iport2uport(port_no), status->link);

    if (port_no >= AFI_link_up.size()) {
        T_E("Invalid port number %u", port_no);
        return;
    }

    AFI_LOCK_SCOPE();

    if (status->link == AFI_link_up[port_no]) {
        // What we've cached as the link state is the same as the port module tells us,
        // so nothing else to do.
        return;
    }

    // Gotta go through all flows on this port to see if we should pause
    // or resume transmission.

    // First the single flows
    for (isingle_id = 0; isingle_id < AFI_single_state.size(); isingle_id++) {
        afi_single_state_t *single_state = &AFI_single_state[isingle_id];

        if (!single_state->allocated || single_state->status.dst_port != port_no || !single_state->status.admin_up) {
            continue;
        }

        if (status->link) {
            // Put this back in action, since it's administrative up, and we just got link.
            (void)afi_platform_func.single_resume(single_state);
            single_state->status.oper_up = TRUE;
        } else {
            (void)afi_platform_func.single_pause(single_state);
            single_state->status.oper_up = FALSE;
        }
    }

    // Then the multi flows
    for (imulti_id = 0; imulti_id < AFI_multi_state.size(); imulti_id++) {
        afi_multi_state_t *multi_state = &AFI_multi_state[imulti_id];

        if (!multi_state->allocated || multi_state->status.dst_port != port_no || !multi_state->status.admin_up) {
            continue;
        }

        if (status->link) {
            // Put this back in action, since it's administrative up, and we just got link.
            (void)afi_platform_func.multi_resume(multi_state);
            multi_state->status.oper_up = TRUE;
        } else {
            (void)afi_platform_func.multi_pause(multi_state);
            multi_state->status.oper_up = FALSE;
        }
    }

    AFI_link_up[port_no] = status->link;
}

/******************************************************************************/
// AFI_link_is_up()
/******************************************************************************/
static BOOL AFI_link_is_up(mesa_port_no_t iport)
{
    if (iport == VTSS_PORT_NO_NONE) {
        // Link is always up when masquerading, because we're sending the frame
        // switched (and on some platforms also sending it to a loop port, which
        // can't go down).
        return TRUE;
    }

    return AFI_link_up[iport];
}

/******************************************************************************/
// AFI_trace_frame()
/******************************************************************************/
static void AFI_trace_frame(u32 id, const packet_tx_props_t *tx_props, bool is_multi)
{
    T_DG(AFI_TRACE_GRP_FRAME_PRINT, "%s AFI ID = %u, Frame length (excl FCS): %zu", is_multi ? "Multi" : "Single", id, tx_props->packet_info.len);

    // Print Tx properties
    packet_debug_tx_props_print(VTSS_TRACE_MODULE_ID, AFI_TRACE_GRP_FRAME_PRINT, VTSS_TRACE_LVL_DEBUG, tx_props);

    // Print frame
    T_DG_HEX(AFI_TRACE_GRP_FRAME_PRINT, tx_props->packet_info.frm, tx_props->packet_info.len);
}

/******************************************************************************/
//
// Semi-public functions (used by H/W-specific AFI code)
//
/******************************************************************************/

/****************************************************************************
 * afi_time_ms_get()
 ****************************************************************************/
u64 afi_time_ms_get(void)
{
    struct timespec cur_time;
    u64             result;

    if (!AFI_enable) {
        return MESA_RC_NOT_IMPLEMENTED;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &cur_time) != 0) {
        T_E("clock_gettime() failed");
        return 0;
    }

    result = cur_time.tv_sec * 1000LLU + cur_time.tv_nsec / 1000000LLU;
    return result;
}

/******************************************************************************/
//
// Public functions
//
/******************************************************************************/

/******************************************************************************/
// afi_error_txt()
/******************************************************************************/
const char *afi_error_txt(mesa_rc rc)
{
    switch (rc) {
    case AFI_RC_PARAM:
        return "Invalid parameter";

    case AFI_RC_FPH:
        return "Frames per hour too low or too high";

    case AFI_RC_BPS:
        return "Bits per second too low or too high";

    case AFI_RC_OUT_OF_SINGLE_ENTRIES:
        return "No more free single-frame flow entries";

    case AFI_RC_OUT_OF_MULTI_ENTRIES:
        return "No more free multi-frame flow entries";

    case AFI_RC_INVALID_DST_PORT_CNT:
        return "Only one destination port can be selected";

    case AFI_RC_NO_DST_AND_NO_MASQUERADE_PORT_SELECTED:
        return "No masquerade nor destination port is selected";

    case AFI_RC_BOTH_DST_AND_MASQUERADE_PORT_SELECTED:
        return "Both a masquerade and a destination port is selected";

    case AFI_RC_DOWN_PORT_OUT_OF_RANGE:
        return "Destination port is out of range";

    case AFI_RC_UP_PORT_OUT_OF_RANGE:
        return "Masquerade port is out of range";

    case AFI_RC_ALL_FRMS_IN_SEQUENCE_MUST_USE_SAME_DOWN_PORT:
        return "Not all frames in sequence use same destination port";

    case AFI_RC_ALL_FRMS_IN_SEQUENCE_MUST_USE_SAME_UP_PORT:
        return "Not all frames in sequence use same masquerade port";

    case AFI_RC_NO_FRAME_IN_SINGLE_FLOW:
        return "No frame in single-frame flow";

    case AFI_RC_NO_FRAMES_IN_MULTI_FLOW:
        return "No frames in multi-frame flow";

    case AFI_RC_PACKET_TX_ALLOC_FAILED:
        return "packet_tx_alloc() failed";

    case AFI_RC_PACKET_TX_FAILED:
        return "Unable to transmit packet";

    case AFI_RC_PACKET_TX_FDMA_FAILED:
        return "The requested rate cannot be honored at the moment.";

    case AFI_RC_PACKET_CANCEL_TIMEOUT:
        return "Cancellation of frame timed out";

    case AFI_RC_INVALID_JITTER:
        return "Invalid value of jitter";

    case AFI_RC_HIJACK_FAILED:
        return "mesa_afi_hijack() failed";

    case AFI_RC_SLOW_ALLOC_FAILED:
        return "mesa_afi_slow_inj_alloc() failed";

    case AFI_RC_SLOW_INJ_FREE:
        return "mesa_afi_slow_inj_free() failed";

    case AFI_RC_SLOW_HIJACK_FAILED:
        return "mesa_afi_slow_inj_frm_hijack() failed";

    case AFI_RC_SLOW_START_FAILED:
        return "mesa_afi_slow_inj_start() failed";

    case AFI_RC_SLOW_STOP_FAILED:
        return "mesa_afi_slow_inj_stop() failed";

    case AFI_RC_FAST_ALLOC_FAILED:
        return "mesa_afi_fast_inj_alloc() failed";

    case AFI_RC_FAST_INJ_FREE:
        return "mesa_afi_fast_inj_free() failed";

    case AFI_RC_FAST_HIJACK_FAILED:
        return "mesa_afi_fast_inj_frm_hijack() failed";

    case AFI_RC_FAST_START_FAILED:
        return "mesa_afi_fast_inj_start() failed";

    case AFI_RC_FAST_STOP_FAILED:
        return "mesa_afi_fast_inj_stop() failed";

    case AFI_RC_INVALID_SINGLE_ID:
        return "Invalid single-frame flow ID";

    case AFI_RC_INVALID_MULTI_ID:
        return "Invalid multi-frame flow ID";

    case AFI_RC_SINGLE_ID_NOT_IN_USE:
        return "Single-frame flow ID not in use";

    case AFI_RC_MULTI_ID_NOT_IN_USE:
        return "Multi-frame flow ID not in use";

    case AFI_RC_SINGLE_ALREADY_STARTED:
        return "Cannot resume single-frame flow, since it's already started";

    case AFI_RC_MULTI_ALREADY_STARTED:
        return "Cannot resume multi-frame flow, since it's already started";

    case AFI_RC_SINGLE_NOT_STARTED:
        return "Cannot pause single-frame flow, since it's not started";

    case AFI_RC_MULTI_NOT_STARTED:
        return "Cannot pause multi-frame flow, since it's not started";

    case AFI_RC_NO_MORE_SINGLE_ENTRIES:
        return "afi_single_status_get() iteration completed (not an error)";

    case AFI_RC_NO_MORE_MULTI_ENTRIES:
        return "afi_multi_status_get() iteration completed (not an error)";

    case AFI_RC_NO_SUCH_SINGLE_ENTRY:
        return "No such single-frame flow table entry";

    case AFI_RC_NO_SUCH_MULTI_ENTRY:
        return "No such multi-frame flow table entry";

    case AFI_RC_FPS_EQUAL_TO_ZERO:
        return "AFI_V1: Resulting fps == 0";

    case AFI_RC_MASQUERADING_REQUIRES_LOOP_PORT:
        return "Masquerading requires a loop port, which is not supported on this platform";

    case AFI_RC_SWITCH_CORE_FRAME_RAM_EXHAUSTED:
        return "Used allocated quantum of switch-core RAM";

    case AFI_RC_AFI_ALLOC_FAILED:
        return "mesa_afi_alloc() failed";

    case AFI_RC_AFI_CANCEL_FAILED:
        return "packet_tx_afi_cancel() failed";

    case AFI_RC_NOT_SUPPORTED:
        return "Operation not supported on this platform";

    default:
        return "Unknown AFI error";
    }
}

/**
 * Get to know whether it's possible to allocate a number of flows with the AFI
 * at hand, using a given resource pool.
 *
 * On Serval-1, there is a limit on the amount of switch core RAM, the AFI
 * may use. This function gives an indication on whether it's possible to
 * allocate N flows of each X bytes.
 *
 * \return VTSS_RC_OK if it's possible, and VTSS_RC_ERROR if not.
 */
mesa_rc afi_alloc_possible(u32 flow_cnt, u32 frame_size_bytes, BOOL alternate_resource_pool)
{
    if (!AFI_enable) {
        return MESA_RC_NOT_IMPLEMENTED;
    }

    return afi_platform_func.alloc_possible(flow_cnt, frame_size_bytes, alternate_resource_pool);
}

/****************************************************************************/
//
// Single-frame flows
//
/****************************************************************************/

/****************************************************************************/
// afi_single_conf_init()
/****************************************************************************/
mesa_rc afi_single_conf_init(afi_single_conf_t *conf)
{
    if (!AFI_enable) {
        return MESA_RC_NOT_IMPLEMENTED;
    }

    if (conf == NULL) {
        return AFI_RC_PARAM;
    }

    T_I("Enter");

    memset(conf, 0, sizeof(*conf));

    packet_tx_props_init(&conf->tx_props);

    T_I("Exit");

    return VTSS_RC_OK;
}

/****************************************************************************/
// afi_single_alloc()
/****************************************************************************/
mesa_rc afi_single_alloc(afi_single_conf_t *conf, u32 *usingle_id)
{
    u32                isingle_id;
    afi_single_state_t *single_state;
    mesa_rc            rc;
    int                dst_port;

    T_I("Enter");

    if (!AFI_enable) {
        return MESA_RC_NOT_IMPLEMENTED;
    }

    if (usingle_id == NULL) {
        return AFI_RC_PARAM;
    }

    *usingle_id = AFI_ID_NONE;

    if (conf == NULL) {
        return AFI_RC_PARAM;
    }

    if (conf->params.fph < AFI_SINGLE_RATE_FPH_MIN || conf->params.fph > AFI_SINGLE_RATE_FPH_MAX) {
        return AFI_RC_FPH;
    }

    AFI_LOCK_SCOPE();

    // Check to see if we have any free entries.
    for (isingle_id = 0; isingle_id < AFI_single_state.size(); isingle_id++) {
        if (!AFI_single_state[isingle_id].allocated) {
            break;
        }
    }

    if (isingle_id == AFI_single_state.size()) {
        return AFI_RC_OUT_OF_SINGLE_ENTRIES;
    }

    single_state = &AFI_single_state[isingle_id];
    memset(single_state, 0, sizeof(*single_state));
    single_state->status.params    = conf->params; // Save selected parameters
    single_state->status.module_id = conf->tx_props.packet_info.modid;
    single_state->status.dst_port  = VTSS_PORT_NO_NONE;

    if (conf->tx_props.packet_info.frm == NULL) {
        return AFI_RC_NO_FRAME_IN_SINGLE_FLOW;
    }

    // Check the contents of conf->tx_props.
    VTSS_RC(AFI_tx_props_check(&conf->tx_props, &single_state->status.dst_port, &single_state->status.masquerade_port, &single_state->status.frm_sz_bytes, TRUE));

    // We need to make copies of the frames for two reasons:
    // 1) We won't transmit the frames right now, and afi_api.h
    //    dictates that the frames are passed in the call to
    //    afi_single_alloc(). The main reason for this is that some
    //    platforms require properties from the frame Tx properties
    //    already at allocation time (VTSS_AFI_V2 requires the CoS,
    //    for instance).
    // 2) On some platforms, once the frames are transmitted, we
    //    can forget all about them, but on others, we must keep
    //    copies in S/W in order to be able to start and stop
    //    particular flows.
    memset(&single_state->tx_props, 0, sizeof(single_state->tx_props));

    // Allocate our own frame so that the caller may deallocate his if he wants to.
    if ((rc = AFI_frm_alloc_and_cpy(&single_state->tx_props, &conf->tx_props)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // Platform-specific invokation
    if ((rc = afi_platform_func.single_alloc(single_state)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // If we get here, platform-specific allocation succeeded too.
    // Mark entry as allocated.
    single_state->allocated = TRUE;

do_exit:
    dst_port = single_state->status.dst_port;

    if (rc != VTSS_RC_OK) {
        // Free the packet pointers we just allocated.
        AFI_frm_free(&single_state->tx_props);

        // Clear our own state (for safety)
        memset(single_state, 0, sizeof(*single_state));
    } else {
        *usingle_id = AFI_I2U_ID(isingle_id);
    }

    T_I("Exit (ID = %u, dst_port = %d, rc = %s)", *usingle_id, dst_port, error_txt(rc));

    return rc;
}

/******************************************************************************/
// afi_single_frm_to_hw()
/******************************************************************************/
mesa_rc afi_single_frm_to_hw(u32 usingle_id)
{
    afi_single_state_t *single_state;

    T_I("Enter (ID = %u)", usingle_id);

    AFI_LOCK_SCOPE();

    VTSS_RC(AFI_single_find(AFI_U2I_ID(usingle_id), &single_state));

    if (single_state->status.admin_up) {
        return AFI_RC_SINGLE_ALREADY_STARTED;
    }

    VTSS_RC(afi_platform_func.single_frm_to_hw(single_state));

    T_I("Exit (ID = %u)", usingle_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// afi_single_pause_resume()
/******************************************************************************/
mesa_rc afi_single_pause_resume(u32 usingle_id, BOOL pause)
{
    afi_single_state_t *single_state;

    if (!AFI_enable) {
        return MESA_RC_NOT_IMPLEMENTED;
    }

    T_I("Enter (ID = %u, pause = %d)", usingle_id, pause);

    AFI_LOCK_SCOPE();

    VTSS_RC(AFI_single_find(AFI_U2I_ID(usingle_id), &single_state));

    if (single_state->status.admin_up && !pause) {
        return AFI_RC_SINGLE_ALREADY_STARTED;
    } else if (!single_state->status.admin_up && pause) {
        return AFI_RC_SINGLE_NOT_STARTED;
    } else if (pause) {
        // It is only active if also operational up.
        if (single_state->status.oper_up) {
            VTSS_RC(afi_platform_func.single_pause(single_state));
            single_state->status.oper_up  = FALSE;
        }

        single_state->status.admin_up = FALSE;
    } else {
        // Only start or resume a flow if the link is up.
        if (AFI_link_is_up(single_state->status.dst_port)) {
            VTSS_RC(afi_platform_func.single_resume(single_state));
            single_state->status.oper_up = TRUE;
        }

        single_state->status.admin_up = TRUE;
    }

    T_I("Exit (ID = %u, pause = %d)", usingle_id, pause);

    return VTSS_RC_OK;
}

/******************************************************************************/
// afi_single_free()
/******************************************************************************/
mesa_rc afi_single_free(u32 usingle_id, u64 *active_time_ms)
{
    if (!AFI_enable) {
        return MESA_RC_NOT_IMPLEMENTED;
    }

    afi_single_state_t *single_state;
    mesa_rc            rc = VTSS_RC_OK;

    T_I("Enter (ID = %u)", usingle_id);

    AFI_LOCK_SCOPE();

    VTSS_RC(AFI_single_find(AFI_U2I_ID(usingle_id), &single_state));

    // On some platforms (VTSS_AFI_V2), the frame may be in H/W without the
    // flow being started, whereas this is not possible on others (VTSS_AFI_V1).
    // One thing that holds true no matter the platform is that if a flow is
    // started, then it's also in H/W.
    rc = afi_platform_func.single_free(single_state);

    // Free the frame pointer
    AFI_frm_free(&single_state->tx_props);

    if (active_time_ms) {
        *active_time_ms = single_state->platform.active_time_ms;
    }

    memset(single_state, 0, sizeof(*single_state));

    T_I("Exit (ID = %u)", usingle_id);
    return rc;
}

/******************************************************************************/
// afi_single_status_get()
/******************************************************************************/
mesa_rc afi_single_status_get(u32 *usingle_id, afi_single_status_t *status, BOOL next)
{
    u32 isingle_id_min, isingle_id_max, isingle_id_iter;

    if (usingle_id == NULL || status == NULL) {
        return AFI_RC_PARAM;
    }

    if ((next && *usingle_id != AFI_ID_NONE) || !next) {
        if (AFI_U2I_ID(*usingle_id) >= AFI_SINGLE_CNT) {
            return AFI_RC_PARAM;
        }
    }

    T_I("Enter (ID = %u, next = %d)", *usingle_id, next);

    AFI_LOCK_SCOPE();

    if (next) {
        isingle_id_min = *usingle_id == AFI_ID_NONE ? 0 : AFI_U2I_ID(*usingle_id) + 1;
        isingle_id_max = AFI_single_state.size() - 1;
    } else {
        isingle_id_min = AFI_U2I_ID(*usingle_id);
        isingle_id_max = isingle_id_min;
    }

    for (isingle_id_iter = isingle_id_min; isingle_id_iter <= isingle_id_max; isingle_id_iter++) {
        if (AFI_single_state[isingle_id_iter].allocated) {
            afi_single_state_t *s = &AFI_single_state[isingle_id_iter];

            if (s->status.oper_up && s->status.admin_up) {
                // Update the current active time (only this file touches that
                // member; the platform-specific files hold its state in
                // s->platform).
                s->status.active_time_ms = s->platform.active_time_ms + afi_time_ms_get() - s->platform.tx_time_start;
            } else {
                // Use the platform-specific times only.
                s->status.active_time_ms = s->platform.active_time_ms;
            }

            *status  = s->status;
            *usingle_id = AFI_I2U_ID(isingle_id_iter);
            return VTSS_RC_OK;
        }
    }

    T_I("Exit (ID = %u, next = %d)", *usingle_id, next);

    return next ? AFI_RC_NO_MORE_SINGLE_ENTRIES : AFI_RC_NO_SUCH_SINGLE_ENTRY;
}

/******************************************************************************/
// afi_single_trace_frame()
/******************************************************************************/
mesa_rc afi_single_trace_frame(u32 *usingle_id, BOOL next)
{
    u32 isingle_id_min, isingle_id_max, isingle_id_iter;

    if (usingle_id == NULL) {
        return AFI_RC_PARAM;
    }

    if ((next && *usingle_id != AFI_ID_NONE) || !next) {
        if (AFI_U2I_ID(*usingle_id) >= AFI_SINGLE_CNT) {
            return AFI_RC_PARAM;
        }
    }

    T_I("Enter (ID = %u, next = %d)", *usingle_id, next);

    AFI_LOCK_SCOPE();

    if (next) {
        isingle_id_min = *usingle_id == AFI_ID_NONE ? 0 : AFI_U2I_ID(*usingle_id) + 1;
        isingle_id_max = AFI_single_state.size() - 1;
    } else {
        isingle_id_min = AFI_U2I_ID(*usingle_id);
        isingle_id_max = isingle_id_min;
    }

    for (isingle_id_iter = isingle_id_min; isingle_id_iter <= isingle_id_max; isingle_id_iter++) {
        if (AFI_single_state[isingle_id_iter].allocated) {
            afi_single_state_t *s = &AFI_single_state[isingle_id_iter];
            *usingle_id = AFI_I2U_ID(isingle_id_iter);

            AFI_trace_frame(*usingle_id, &s->tx_props, false);
            return VTSS_RC_OK;
        }
    }

    T_I("Exit (ID = %u, next = %d)", *usingle_id, next);

    return next ? AFI_RC_NO_MORE_SINGLE_ENTRIES : AFI_RC_NO_SUCH_SINGLE_ENTRY;
}

/****************************************************************************/
//
// Multi-frame flows
//
/****************************************************************************/

/****************************************************************************/
// afi_multi_conf_init()
/****************************************************************************/
mesa_rc afi_multi_conf_init(afi_multi_conf_t *conf)
{
    size_t i;

    T_I("Enter");

    if (conf == NULL) {
        return AFI_RC_PARAM;
    }

    vtss_clear(*conf);

    for (i = 0; i < conf->tx_props.size(); i++) {
        packet_tx_props_init(&conf->tx_props[i]);
    }

    T_I("Exit");

    return VTSS_RC_OK;
}

/****************************************************************************/
// afi_multi_alloc()
/****************************************************************************/
mesa_rc afi_multi_alloc(afi_multi_conf_t *conf, u32 *umulti_id)
{
    size_t            i;
    afi_multi_state_t *multi_state;
    mesa_rc           rc;
    int               dst_port;
    u32               imulti_id;

    if (umulti_id == NULL) {
        return AFI_RC_PARAM;
    }

    T_I("Enter");

    *umulti_id = AFI_ID_NONE;

    if (conf == NULL) {
        return AFI_RC_PARAM;
    }

    if (conf->params.bps < AFI_MULTI_RATE_BPS_MIN || conf->params.bps > AFI_MULTI_RATE_BPS_MAX) {
        return AFI_RC_BPS;
    }

    AFI_LOCK_SCOPE();

    // Check to see if we have any free entries.
    for (imulti_id = 0; imulti_id < AFI_multi_state.size(); imulti_id++) {
        if (!AFI_multi_state[imulti_id].allocated) {
            break;
        }
    }

    if (imulti_id == AFI_multi_state.size()) {
        return AFI_RC_OUT_OF_MULTI_ENTRIES;
    }

    multi_state = &AFI_multi_state[imulti_id];
    vtss_clear(*multi_state);
    multi_state->status.params    = conf->params; // Save selected parameters
    multi_state->status.module_id = conf->tx_props[0].packet_info.modid;
    multi_state->status.dst_port  = VTSS_PORT_NO_NONE;

    // Count the number of frames in the sequence and check that only one port is in use and it's
    // pointing to the same port for all frames in the sequence.
    // Also check other properties of conf->tx_props[].
    for (i = 0; i < conf->tx_props.size(); i++) {
        packet_tx_props_t *tx_props = &conf->tx_props[i];

        if (tx_props->packet_info.frm) {
            multi_state->multi_len++;
        } else {
            // A NULL-pointer marks the end of the sequence
            // whether or not later entries have a non-NULL-pointer.
            break;
        }

        VTSS_RC(AFI_tx_props_check(tx_props, &multi_state->status.dst_port, &multi_state->status.masquerade_port, &multi_state->status.frm_sz_bytes[i], i == 0));
    }

    if (multi_state->multi_len == 0) {
        return AFI_RC_NO_FRAMES_IN_MULTI_FLOW;
    }

    // We need to make copies of the frames for two reasons:
    // 1) We won't transmit the frames right now, and afi_api.h
    //    dictates that the frames are passed in the call to
    //    afi_multi_alloc(). The main reason for this is that some
    //    platforms require properties from the frame Tx properties
    //    already at allocation time (VTSS_AFI_V2 requires the CoS,
    //    for instance).
    // 2) On some platforms, once the frames are transmitted, we
    //    can forget all about them, but on others, we must keep
    //    copies in S/W in order to be able to start and stop
    //    particular flows.
    vtss_clear(multi_state->tx_props);

    // Allocate our own frames so that the caller may deallocate his if he wants to.
    for (i = 0; i < multi_state->multi_len; i++) {
        if ((rc = AFI_frm_alloc_and_cpy(&multi_state->tx_props[i], &conf->tx_props[i])) != VTSS_RC_OK) {
            goto do_exit;
        }
    }

    // Platform-specific invokation
    if ((rc = afi_platform_func.multi_alloc(multi_state)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // If we get here, platform-specific allocation succeeded too.
    // Mark entry as allocated.
    multi_state->allocated = TRUE;

do_exit:
    dst_port = multi_state->status.dst_port;

    if (rc != VTSS_RC_OK) {
        // Free the packet pointers we just allocated.
        for (i = 0; i < multi_state->multi_len; i++) {
            AFI_frm_free(&multi_state->tx_props[i]);
        }

        // Clear our own state (for safety)
        vtss_clear(*multi_state);
    } else {
        *umulti_id = AFI_I2U_ID(imulti_id);
    }

    T_I("Exit (ID = %u, dst_port = %d, rc = %s)", *umulti_id, dst_port, error_txt(rc));

    return rc;
}

/******************************************************************************/
// afi_multi_frms_to_hw()
/******************************************************************************/
mesa_rc afi_multi_frms_to_hw(u32 umulti_id)
{
    afi_multi_state_t *multi_state;

    T_I("Enter (ID = %u)", umulti_id);

    AFI_LOCK_SCOPE();

    VTSS_RC(AFI_multi_find(AFI_U2I_ID(umulti_id), &multi_state));

    if (multi_state->status.admin_up) {
        return AFI_RC_MULTI_ALREADY_STARTED;
    }

    VTSS_RC(afi_platform_func.multi_frms_to_hw(multi_state));

    T_I("Exit (ID = %u)", umulti_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// afi_multi_pause_resume()
/******************************************************************************/
mesa_rc afi_multi_pause_resume(u32 umulti_id, BOOL pause)
{
    afi_multi_state_t *multi_state;

    T_I("Enter (ID = %u, pause = %d)", umulti_id, pause);

    AFI_LOCK_SCOPE();

    VTSS_RC(AFI_multi_find(AFI_U2I_ID(umulti_id), &multi_state));

    if (multi_state->status.admin_up && !pause) {
        return AFI_RC_MULTI_ALREADY_STARTED;
    } else if (!multi_state->status.admin_up && pause) {
        return AFI_RC_MULTI_NOT_STARTED;
    } else if (pause) {
        // It is only active if also operational up.
        if (multi_state->status.oper_up) {
            VTSS_RC(afi_platform_func.multi_pause(multi_state));
            multi_state->status.oper_up = FALSE;
        }

        multi_state->status.admin_up = FALSE;
    } else {
        // Only start or resume a flow if the link is up
        if (AFI_link_is_up(multi_state->status.dst_port)) {
            VTSS_RC(afi_platform_func.multi_resume(multi_state));
            multi_state->status.oper_up = TRUE;
        }

        multi_state->status.admin_up = TRUE;
    }

    T_I("Exit (ID = %u, pause = %d)", umulti_id, pause);

    return VTSS_RC_OK;
}

/******************************************************************************/
// afi_multi_free()
/******************************************************************************/
mesa_rc afi_multi_free(u32 umulti_id, u64 *active_time_ms)
{
    afi_multi_state_t *multi_state;
    size_t            i;
    mesa_rc           rc = VTSS_RC_OK;

    T_I("Enter (ID = %u)", umulti_id);

    AFI_LOCK_SCOPE();

    VTSS_RC(AFI_multi_find(AFI_U2I_ID(umulti_id), &multi_state));

    // On some platforms (VTSS_AFI_V2), the frames may be in H/W without the
    // flow being started, whereas this is not possible on others (VTSS_AFI_V1).
    // One thing that holds true no matter the platform is that if a flow is
    // started, then it's also in H/W.
    rc = afi_platform_func.multi_free(multi_state);

    // Free the frame pointers
    for (i = 0; i < multi_state->multi_len; i++) {
        AFI_frm_free(&multi_state->tx_props[i]);
    }

    if (active_time_ms) {
        *active_time_ms = multi_state->platform.active_time_ms;
    }

    T_I("Exit (ID = %u, active_time_ms = %llu)", umulti_id, active_time_ms ? *active_time_ms : 0LLU);

    vtss_clear(*multi_state);
    return rc;
}

/******************************************************************************/
// afi_multi_status_get()
/******************************************************************************/
mesa_rc afi_multi_status_get(u32 *umulti_id, afi_multi_status_t *status, BOOL next)
{
    u32 imulti_id_min, imulti_id_max, imulti_id_iter;

    if (umulti_id == NULL || status == NULL) {
        return AFI_RC_PARAM;
    }

    if ((next && *umulti_id != AFI_ID_NONE) || !next) {
        if (AFI_U2I_ID(*umulti_id) >= AFI_MULTI_CNT) {
            return AFI_RC_PARAM;
        }
    }

    T_I("Enter (ID = %u, next = %d)", *umulti_id, next);

    AFI_LOCK_SCOPE();

    if (next) {
        imulti_id_min = *umulti_id == AFI_ID_NONE ? 0 : AFI_U2I_ID(*umulti_id) + 1;
        imulti_id_max = AFI_multi_state.size() - 1;
    } else {
        imulti_id_min = AFI_U2I_ID(*umulti_id);
        imulti_id_max = imulti_id_min;
    }

    for (imulti_id_iter = imulti_id_min; imulti_id_iter <= imulti_id_max; imulti_id_iter++) {
        if (AFI_multi_state[imulti_id_iter].allocated) {
            afi_multi_state_t *s = &AFI_multi_state[imulti_id_iter];

            if (s->status.oper_up && s->status.admin_up) {
                // Update the current active time (only this file touches that
                // member; the platform-specific files hold its state in
                // s->platform).
                s->status.active_time_ms = s->platform.active_time_ms + afi_time_ms_get() - s->platform.tx_time_start;
            } else {
                // Use the platform-specific times only.
                s->status.active_time_ms = s->platform.active_time_ms;
            }

            *status  = s->status;
            *umulti_id = AFI_I2U_ID(imulti_id_iter);
            return VTSS_RC_OK;
        }
    }

    T_I("Exit (ID = %u, next = %d)", *umulti_id, next);

    return next ? AFI_RC_NO_MORE_MULTI_ENTRIES : AFI_RC_NO_SUCH_MULTI_ENTRY;
}

/******************************************************************************/
// afi_multi_trace_frame()
/******************************************************************************/
mesa_rc afi_multi_trace_frame(u32 *umulti_id, BOOL next)
{
    u32    imulti_id_min, imulti_id_max, imulti_id_iter;
    size_t i;

    if (umulti_id == NULL) {
        return AFI_RC_PARAM;
    }

    if ((next && *umulti_id != AFI_ID_NONE) || !next) {
        if (AFI_U2I_ID(*umulti_id) >= AFI_MULTI_CNT) {
            return AFI_RC_PARAM;
        }
    }

    T_I("Enter (ID = %u, next = %d)", *umulti_id, next);

    AFI_LOCK_SCOPE();

    if (next) {
        imulti_id_min = *umulti_id == AFI_ID_NONE ? 0 : AFI_U2I_ID(*umulti_id) + 1;
        imulti_id_max = AFI_multi_state.size() - 1;
    } else {
        imulti_id_min = AFI_U2I_ID(*umulti_id);
        imulti_id_max = imulti_id_min;
    }

    for (imulti_id_iter = imulti_id_min; imulti_id_iter <= imulti_id_max; imulti_id_iter++) {
        if (AFI_multi_state[imulti_id_iter].allocated) {
            afi_multi_state_t *s = &AFI_multi_state[imulti_id_iter];
            *umulti_id = AFI_I2U_ID(imulti_id_iter);

            for (i = 0; i < s->multi_len; i++) {
                AFI_trace_frame(*umulti_id, &s->tx_props[i], true);
            }

            return VTSS_RC_OK;
        }
    }

    T_I("Exit (ID = %u, next = %d)", *umulti_id, next);

    return next ? AFI_RC_NO_MORE_MULTI_ENTRIES : AFI_RC_NO_SUCH_MULTI_ENTRY;
}

extern "C" int afi_icli_cmd_register();

/******************************************************************************/
// afi_init()
/******************************************************************************/
mesa_rc afi_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        AFI_enable = fast_cap(VTSS_APPL_CAP_AFI);
    }

    if (!AFI_enable) {
        return MESA_RC_OK;
    }

    if (data->cmd == INIT_CMD_INIT) {
        afi_icli_cmd_register();
        if (fast_cap(MESA_CAP_AFI_V1)) {
            afi_platform_v1_create(&afi_platform_func);
        } else {
            afi_platform_v2_create(&afi_platform_func);
        }

        critd_init(&AFI_crit, "AFI", VTSS_MODULE_ID_AFI, CRITD_TYPE_MUTEX);
    } else if (data->cmd == INIT_CMD_START) {
        mesa_rc rc;

        AFI_LOCK_SCOPE();

        // Register for link-change events. This is needed so that we can pause
        // flows on ports with no link, and auto-resume them when link comes
        // back up. This is especially needed on Serval-1 running without the API's
        // FDMA (which is the case on Linux and when running on an external CPU)
        // because when pausing flows on Serval-1, one has to delete the frames
        // from the switch core, and in order to resume them, one has to know
        // the frame contents, and the only one who does that (when not using the
        // API's FDMA driver) is us.
        // The AFI_port_change_callback() will be called back once per port
        // within the next second, so no need to cache the current link status.
        if ((rc = port_change_register(VTSS_MODULE_ID_AFI, AFI_port_change_callback)) != VTSS_RC_OK) {
            T_E("Unable to register for port change events (%s)", error_txt(rc));
        }
    }

    return VTSS_RC_OK;
}


